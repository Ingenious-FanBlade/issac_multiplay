//! One invocation owns E1-E7, including both launches. A failed run always writes
//! its artifacts and never retries. Build scripts do not call this program.
mod win32;
use serde_json::{Value,json};
use std::{collections::BTreeMap,env,fs,io::{BufRead,BufReader,Write},net::{TcpStream,SocketAddr},path::{Path,PathBuf},thread,time::{Duration,Instant}};
type Result<T> = std::result::Result<T,String>;

fn write_json(path:&Path,value:&Value)->Result<()> {fs::write(path,serde_json::to_vec_pretty(value).map_err(|e|e.to_string())?).map_err(|e|e.to_string())}
fn read(path:&Path)->Result<String> {fs::read_to_string(path).map_err(|e|format!("{}: {e}",path.display()))}
fn mkdir(path:&Path)->Result<()> {fs::create_dir_all(path).map_err(|e|e.to_string())}
fn copy_tree(source:&Path,dest:&Path)->Result<()> {
    mkdir(dest)?;
    for item in fs::read_dir(source).map_err(|e|e.to_string())? {
        let item=item.map_err(|e|e.to_string())?;let kind=item.file_type().map_err(|e|e.to_string())?;
        if kind.is_symlink(){return Err(format!("refusing symlink {}",item.path().display()));}
        let dst=dest.join(item.file_name());
        if kind.is_dir(){copy_tree(&item.path(),&dst)?;}else{fs::copy(item.path(),dst).map_err(|e|e.to_string())?;}
    }Ok(())
}
fn files(root:&Path)->Result<BTreeMap<String,Vec<u8>>> {
    fn walk(root:&Path,p:&Path,out:&mut BTreeMap<String,Vec<u8>>)->Result<()> {
        for entry in fs::read_dir(p).map_err(|e|e.to_string())? {
            let e=entry.map_err(|e|e.to_string())?;let t=e.file_type().map_err(|e|e.to_string())?;
            if t.is_symlink(){return Err("symlink in snapshot".into());}
            if t.is_dir(){walk(root,&e.path(),out)?;}else{out.insert(e.path().strip_prefix(root).unwrap().to_string_lossy().replace('\\',"/"),fs::read(e.path()).map_err(|e|e.to_string())?);}
        }Ok(())
    }
    let mut result=BTreeMap::new();walk(root,root,&mut result)?;Ok(result)
}
fn save_changes(before:&Path,after:&Path)->Result<Value> {
    // Compare bytes directly. No digest is calculated for any input or artifact.
    let a=files(before)?;let b=files(after)?;let mut changed=vec![];
    let names:std::collections::BTreeSet<_>=a.keys().chain(b.keys()).collect();
    for name in names {if a.get(name)!=b.get(name) {
        let base=Path::new(name).file_name().unwrap().to_string_lossy().to_lowercase();
        let allowed=base=="persistentgamedata2.dat" || base=="gamestate2.dat" ||
            base.ends_with("_persistentgamedata2.dat") || base.ends_with("_gamestate2.dat") ||
            base=="log.txt" || base=="cmd_history.txt" || base.contains("options") || base.starts_with("inputconfigs") || base.ends_with(".ini");
        changed.push(json!({"path":name,"allowed":allowed,"before_size":a.get(name).map(Vec::len),"after_size":b.get(name).map(Vec::len)}));
    }}Ok(json!(changed))
}
fn pe_timestamp(path:&Path)->Result<u32> {
    let bytes=fs::read(path).map_err(|e|e.to_string())?;
    let word=|n:usize|->Result<u32>{let b=bytes.get(n..n+4).ok_or("truncated PE")?;Ok(u32::from_le_bytes(b.try_into().unwrap()))};
    if bytes.get(..2)!=Some(b"MZ"){return Err("missing MZ header".into());}
    let pe=word(0x3C)? as usize;if bytes.get(pe..pe+4)!=Some(b"PE\0\0"){return Err("missing PE header".into());}word(pe+8)
}
struct Control { stream:BufReader<TcpStream> }
impl Control {
    fn connect(port:u16,game:&win32::Game,seconds:u64)->Result<Self> {
        let end=Instant::now()+Duration::from_secs(seconds);
        while Instant::now()<end {
            if let Some(code)=game.exited(){return Err(format!("early process exit {code}"));}
            if let Ok(s)=TcpStream::connect_timeout(&SocketAddr::from(([127,0,0,1],port)),Duration::from_millis(250)) {
                s.set_read_timeout(Some(Duration::from_secs(6))).map_err(|e|e.to_string())?;
                s.set_write_timeout(Some(Duration::from_secs(6))).map_err(|e|e.to_string())?;
                return Ok(Self{stream:BufReader::new(s)});
            }
            thread::sleep(Duration::from_millis(100));
        }Err("control port timeout".into())
    }
    fn command(&mut self,text:&str)->Result<BTreeMap<String,String>> {
        writeln!(self.stream.get_mut(),"{text}").map_err(|e|e.to_string())?;
        let mut response=BTreeMap::new();let mut count=0;
        loop {
            let mut line=String::new();if self.stream.read_line(&mut line).map_err(|e|e.to_string())?==0{return Err("control disconnected".into());}
            count+=line.len();if count>16000{return Err("oversize response".into());}
            let line=line.trim_end();if line=="."{break;}if let Some((k,v))=line.split_once('='){response.insert(k.into(),v.into());}
        }
        if response.get("ok").map(String::as_str)!=Some("1"){return Err(format!("{text}: {response:?}"));}Ok(response)
    }
    fn wait(&mut self,key:&str,value:&str,seconds:u64)->Result<BTreeMap<String,String>> {
        let until=Instant::now()+Duration::from_secs(seconds);let mut last=BTreeMap::new();
        while Instant::now()<until {
            last=self.command("cs status")?;if last.get(key).map(String::as_str)==Some(value){return Ok(last);}
            thread::sleep(Duration::from_millis(150));
        }Err(format!("status timeout {key}={value}: {last:?}"))
    }
}
struct Run {
    repo:PathBuf, game:PathBuf, out:PathBuf, save:PathBuf, scenario:Value,
    report:Value, steps:fs::File, deployed:bool, fixture_owned:bool,
}
impl Run {
    fn step(&mut self,name:&str,detail:Value)->Result<()> {
        let item=json!({"step":name,"detail":detail});writeln!(self.steps,"{item}").map_err(|e|e.to_string())?;self.steps.flush().map_err(|e|e.to_string())?;println!("{name}");Ok(())
    }
    fn case(&mut self,id:&str,pass:bool,detail:Value) {self.report["cases"][id]=json!({"pass":pass,"detail":detail});}
    fn config(&self,enabled:bool)->Result<()> {
        let p=self.game.join("Repentogon/isaac-cs");mkdir(&p)?;
        write_json(&p.join("config.json"),&json!({"schema":1,"enabled":enabled,"log_level":"info","trace_dir":"isaac-cs/traces","control_port":{"enabled":enabled,"port":self.scenario["control_port"]},"host":{"default_port":27090}}))
    }
    fn deploy(&mut self)->Result<()> {
        let fixture=self.game.join("mods/isaac-cs-test");
        if fixture.exists(){return Err("existing test mod must be preserved; deployment refused".into());}
        mkdir(&self.out.join("deployment_before"))?;
        let targets=[("out/repentogon/zhlIsaacCS.dll","Repentogon/zhlIsaacCS.dll"),
            ("ui/isaac-cs-ui","mods/isaac-cs-ui"),("tests/e2e-mods/isaac-cs-test","mods/isaac-cs-test")];
        let mut manifest=vec![];
        let existing=self.game.join("Repentogon/isaac-cs/config.json");
        if existing.exists(){fs::copy(&existing,self.out.join("deployment_before/config.json")).map_err(|e|e.to_string())?;}
        for (src,dst) in targets {
            let s=self.repo.join(src);let d=self.game.join(dst);
            if d.exists(){let backup=self.out.join("deployment_before").join(d.file_name().unwrap());if d.is_dir(){copy_tree(&d,&backup)?;}else{fs::copy(&d,&backup).map_err(|e|e.to_string())?;}}
            if dst.ends_with("isaac-cs-test"){self.fixture_owned=true;}
            if s.is_dir(){copy_tree(&s,&d)?;}else{fs::copy(&s,&d).map_err(|e|e.to_string())?;}
            manifest.push(json!({"source":s,"destination":d}));
        }
        self.deployed=true;self.config(true)?;
        write_json(&self.out.join("deployment.json"),&json!(manifest))?;
        self.step("deployment",json!(manifest))
    }
    fn environment(&mut self)->Result<()> {
        let names=win32::processes()?;let steam=names.iter().any(|n|n.eq_ignore_ascii_case("steam.exe"));
        let running=names.iter().any(|n|n.eq_ignore_ascii_case("isaac-ng.exe"));
        let timestamp=pe_timestamp(&self.game.join("Repentogon/isaac-ng.exe"))?;
        let home=PathBuf::from(env::var("USERPROFILE").map_err(|e|e.to_string())?);
        let launcher=read(&home.join("Documents/My Games/repentogon_launcher.ini")).unwrap_or_default();
        let version=read(&self.game.join("Repentogon/rgon_changelog.txt")).unwrap_or_default();
        let mut mods=vec![];
        for entry in fs::read_dir(self.game.join("mods")).map_err(|e|e.to_string())? {
            let e=entry.map_err(|e|e.to_string())?;if e.path().is_dir(){mods.push(json!({"directory":e.file_name().to_string_lossy(),"enabled":!e.path().join("disable.it").exists()}));}
        }
        self.report["environment"]=json!({"game":self.game,"pe_timestamp":format!("0x{timestamp:08X}"),"steam_running":steam,"game_running":running,"launcher":launcher,"installed_version_excerpt":version.chars().take(500).collect::<String>(),"mods":mods,"architecture":"i686","save_slot":2});
        self.step("environment",self.report["environment"].clone())?;
        if timestamp!=0x68154D8D{return Err("wrong game version; no injection".into());}
        if !steam{return Err("Steam is not running; no injection".into());}
        if running{return Err("a game process already exists; no injection".into());}Ok(())
    }
    fn keys(&mut self,w:usize,keys:&[String])->Result<()> {
        let delay=self.scenario["key_settle_ms"].as_u64().unwrap_or(1400);
        for k in keys {self.step("physical_key",json!(k))?;win32::key(w,k)?;thread::sleep(Duration::from_millis(delay));}Ok(())
    }
    fn sequence(&mut self,w:usize,name:&str)->Result<()> {
        let keys=self.scenario[name].as_array().ok_or("missing scenario keys")?.iter().map(|v|v.as_str().unwrap_or("").to_string()).collect::<Vec<_>>();
        self.keys(w,&keys)
    }
    fn console(&mut self,w:usize,token:&str)->Result<()> {
        win32::key(w,"CONSOLE")?;thread::sleep(Duration::from_millis(500));
        win32::text(w,&format!("cs echo {token}"))?;win32::key(w,"ENTER")?;thread::sleep(Duration::from_millis(500));
        win32::screenshot(w,&self.out.join(format!("{token}.bmp")))?;
        self.step("console",json!({"token":token,"screenshot":format!("{token}.bmp")}))?;
        win32::key(w,"ESC")?;thread::sleep(Duration::from_millis(500));Ok(())
    }
    fn collect(&self,pid:u32,label:&str)->Result<()> {
        let dest=self.out.join(label);mkdir(&dest)?;
        for (s,d) in [(self.game.join("Repentogon/repentogon.log"),dest.join("repentogon.log")),(self.save.join("log.txt"),dest.join("log.txt")),(self.game.join(format!("Repentogon/isaac-cs/traces/boot-{pid}.jsonl")),dest.join("trace.jsonl"))] {
            if s.exists(){fs::copy(s,d).map_err(|e|e.to_string())?;}
        }Ok(())
    }
    fn trace(&self,label:&str)->Vec<Value> {
        read(&self.out.join(label).join("trace.jsonl")).unwrap_or_default().lines().filter_map(|l|serde_json::from_str(l).ok()).collect()
    }
    fn enabled(&mut self)->Result<()> {
        self.step("launch_enabled",json!({"loader":"zhlLoader.dll","Launch":0}))?;
        let game=win32::Game::launch(&self.game.join("Repentogon"))?;let pid=game.pid();
        // Collect even on a scenario failure; Game's drop closes only this child.
        let result=self.enabled_scene(&game);
        if result.is_err(){let _=game.close();}
        drop(game);thread::sleep(Duration::from_millis(300));self.collect(pid,"enabled")?;
        result
    }
    fn enabled_scene(&mut self,game:&win32::Game)->Result<()> {
        let w=game.wait_window(90)?;
        let mut control=Control::connect(self.scenario["control_port"].as_u64().unwrap_or(27091) as u16,game,90)?;
        // Wait until the title/file-selection menu has actually rendered.
        let mut initial=control.command("cs status")?;
        for _ in 0..100 {
            let id=initial.get("menu_id").map(String::as_str).unwrap_or("-1");
            if id=="1" || id=="2"{break;}
            thread::sleep(Duration::from_millis(150));initial=control.command("cs status")?;
        }
        self.step("menu_status",json!(initial))?;
        self.report["q14"]=json!({"g_game":initial.get("g_game"),"lua_menu":"observe enabled/log.txt; empty is observation, not failure"});
        self.console(w,"console-menu")?;
        if initial.get("menu_id").map(String::as_str)==Some("1"){self.sequence(w,"to_file_select")?;}
        control.wait("menu_id","2",15)?;
        // Read the current selection instead of assuming left clamps or wraps.
        // SDK Menu_Save.SelectedSave is zero-based, and slot 2 is value 1.
        for _ in 0..3 {
            let s=control.command("cs status")?;
            let slot=s.get("selected_save").ok_or("missing selected_save")?.parse::<i32>().map_err(|e|e.to_string())?;
            if slot==1{break;}
            self.keys(w,&[if slot<1{"RIGHT"}else{"LEFT"}.into()])?;
        }
        let selected=control.wait("selected_save","1",5)?;
        self.step("selected_second_save",json!(selected))?;
        win32::screenshot(w,&self.out.join("save-slot-2.bmp"))?;
        self.keys(w,&["ENTER".into()])?;control.wait("menu_id","3",15)?;
        // Native Menu_Game::Update case 0 opens CHARACTER (5). Selection wraps;
        // only press confirm after observing case 0, so Online is never entered.
        for _ in 0..6 {
            let s=control.command("cs status")?;
            if s.get("menu_game_selection").map(String::as_str)==Some("0"){break;}
            self.keys(w,&["UP".into()])?;
        }
        control.wait("menu_game_selection","0",5)?;
        self.sequence(w,"open_character_menu")?;control.wait("menu_id","5",15)?;
        self.sequence(w,"start_run")?;let running=control.wait("run_active","1",20)?;
        self.step("run_status",json!(running))?;
        thread::sleep(Duration::from_secs(self.scenario["run_seconds"].as_u64().unwrap_or(10)));
        self.console(w,"console-run")?;
        let echo=control.command("cs echo port-run")?;self.step("port_echo",json!(echo))?;
        if echo.get("echo").map(String::as_str)!=Some("port-run"){return Err("port echo mismatch".into());}
        let host=control.command("cs host 27090")?;let hosting=control.wait("state","Hosting",5)?;
        let stop=control.command("cs stop")?;let idle=control.wait("state","Idle",5)?;
        let host_frame=host.get("frame").and_then(|x|x.parse::<u64>().ok());
        let active_frame=hosting.get("frame").and_then(|x|x.parse::<u64>().ok());
        self.case("E4",matches!((host_frame,active_frame),(Some(a),Some(b)) if b>a),json!({"host":host,"hosting":hosting,"stop":stop,"idle":idle}));
        let counters=control.command("cs counters")?;
        write_json(&self.out.join("counters.json"),&json!(counters))?;
        let manifest:Value=serde_json::from_str(&read(&self.repo.join("out/repentogon/hook-manifest.json"))?).map_err(|e|e.to_string())?;
        let required=manifest["hooks"].as_array().unwrap().iter().filter(|h|h["required_nonzero"]==true && h["id"]!="game_exit").map(|h|h["id"].as_str().unwrap()).chain(["keyboard_35"]).collect::<Vec<_>>();
        let missing=required.into_iter().filter(|id|counters.get(&format!("hook.{id}")).and_then(|v|v.parse::<u64>().ok()).unwrap_or(0)==0).collect::<Vec<_>>();
        let foreign=counters.iter().any(|(k,v)|k.ends_with(".foreign") && v!="0");
        let count=manifest["hooks"].as_array().unwrap().len().to_string();
        let status=control.command("cs status")?;
        let caps=["loop","input","player","join","control","device"].iter().all(|g|status.get(&format!("capability.{g}")).map(String::as_str)==Some("1"));
        self.case("E1",caps,json!({"status":status,"pid":game.pid(),"Launch":0}));
        self.case("E2",missing.is_empty() && !foreign && status.get("registered")==Some(&count) && status.get("pointers").map(String::as_str)==Some("8") && status.get("trace_dropped").map(String::as_str)==Some("0") && status.get("context_faults").map(String::as_str)==Some("0"),json!({"required_missing":missing,"foreign":foreign,"expected_registered":count,"status":status}));
        let before=counters.get("hook.game_exit").and_then(|x|x.parse::<u64>().ok()).unwrap_or(0);
        self.sequence(w,"return_menu")?;control.wait("run_active","0",20)?;
        let after=control.command("cs counters")?.get("hook.game_exit").and_then(|x|x.parse::<u64>().ok()).unwrap_or(0);
        self.report["exit_hook"]=json!({"before":before,"after":after,"increased":after>before});
        drop(control);let code=game.close()?;self.report["enabled_exit"]=json!(code);
        if code!=0{return Err(format!("enabled process exit code {code}"));}Ok(())
    }
    fn disabled(&mut self)->Result<()> {
        self.config(false)?;self.step("launch_disabled",json!({"enabled":false}))?;
        let game=win32::Game::launch(&self.game.join("Repentogon"))?;let pid=game.pid();
        let result=(||->Result<u32>{let w=game.wait_window(90)?;thread::sleep(Duration::from_secs(12));win32::screenshot(w,&self.out.join("disabled-menu.bmp"))?;game.close()})();
        drop(game);thread::sleep(Duration::from_millis(300));self.collect(pid,"disabled")?;
        let code=result?;self.report["disabled_exit"]=json!(code);
        let events=self.trace("disabled");
        self.case("E6",code==0 && events.len()==1 && events[0]["ev"]=="boot" && events[0]["detail"].as_str().unwrap_or("").contains("enabled=0 registered=0 pointers=0"),json!({"events":events,"exit":code,"menu_observation":"disabled-menu.bmp"}));Ok(())
    }
    fn evaluate_logs(&mut self) {
        let events=self.trace("enabled");let log=read(&self.out.join("enabled/log.txt")).unwrap_or_default();
        let details=events.iter().filter(|e|e["ev"]=="command").filter_map(|e|e["detail"].as_str()).collect::<Vec<_>>();
        let bridge=[("console","console-menu"),("console","console-run"),("lua","lua-run"),("port","port-run")].iter().all(|(source,token)|details.iter().any(|d|d.contains(&format!("source={source}")) && d.contains(&format!("text=cs echo {token}")) && d.contains(&format!("echo={token}"))));
        self.case("E3",bridge && log.contains("[isaac-cs-test] lua-run") && log.contains("echo=lua-run"),json!({"bridge":bridge,"q14_menu_log":log.lines().filter(|s|s.contains("lua-menu")).collect::<Vec<_>>() }));
        self.case("E5",log.contains("[isaac-cs-ui] window-render") && self.out.join("console-menu.bmp").exists(),json!({"render_marker":log.contains("[isaac-cs-ui] window-render"),"screenshot":"console-menu.bmp"}));
        // Status polling can skip frames. The state trace must still show that
        // each queued mutation ran at exactly the next Manager boundary.
        let mut queue_timing=true;
        for verb in ["host","stop"] {
            let queued=&self.report["cases"]["E4"]["detail"][verb];
            let id=queued["queued"].as_str().unwrap_or("");
            let frame=queued["frame"].as_str().and_then(|s|s.parse::<u64>().ok());
            queue_timing &= frame.is_some_and(|f|events.iter().any(|e|e["ev"]=="state" &&
                e["detail"].as_str().is_some_and(|s|s.starts_with(&format!("id={id} ")) && s.contains(&format!("frame={} ",f+1)))));
        }
        self.report["cases"]["E4"]["detail"]["exact_next_frame"]=json!(queue_timing);
        if !queue_timing{self.report["cases"]["E4"]["pass"]=json!(false);}
        let enabled=read(&self.out.join("enabled/repentogon.log")).unwrap_or_default();
        let disabled=read(&self.out.join("disabled/repentogon.log")).unwrap_or_default();
        let errors=|text:&str|text.lines().filter(|l|l.contains("[ERROR]") || l.contains("[FATAL]")).map(str::to_string).collect::<Vec<_>>();
        let new_errors=errors(&enabled).into_iter().filter(|e|!errors(&disabled).contains(e)).collect::<Vec<_>>();
        self.report["repentogon_errors"]=json!({"new_in_enabled":new_errors,"enabled":errors(&enabled),"disabled":errors(&disabled)});
    }
    fn cleanup(&mut self)->Result<()> {
        if self.fixture_owned {
            // Only remove the fixture directory this invocation created, under
            // the explicitly named game mods directory; no shell path expansion.
            let target=self.game.join("mods/isaac-cs-test").canonicalize().map_err(|e|e.to_string())?;
            let mods=self.game.join("mods").canonicalize().map_err(|e|e.to_string())?;
            if target.parent()!=Some(mods.as_path()){return Err("fixture cleanup escaped mods directory".into());}
            fs::remove_dir_all(&target).map_err(|e|e.to_string())?;
        }
        if self.deployed{self.config(false)?;}Ok(())
    }
    fn execute(&mut self)->Result<()> {
        self.environment()?;copy_tree(&self.save,&self.out.join("save_before"))?;
        self.step("save_before",json!({"slot":2,"backup":"save_before"}))?;
        self.deploy()?;self.enabled()?;self.disabled()?;self.evaluate_logs();Ok(())
    }
    fn finish(&mut self,result:Result<()>)->Result<()> {
        self.report["failure"]=result.err().map(Value::String).unwrap_or(Value::Null);
        // Cleanup comes before report analysis so even an unreadable artifact
        // cannot leave the temporary test mod enabled.
        if let Err(e)=self.cleanup(){self.report["cleanup_error"]=json!(e);}
        if self.out.join("save_before").exists() {
            let backup=copy_tree(&self.save,&self.out.join("save_after"));
            if let Err(e)=backup {self.report["save_backup_error"]=json!(e);}
            else {
                let changes=match save_changes(&self.out.join("save_before"),&self.out.join("save_after")) {
                    Ok(changes)=>changes,
                    Err(e)=>{self.report["save_compare_error"]=json!(e);json!([])}
                };
                let allowed=changes.as_array().unwrap().iter().all(|c|c["allowed"]==true);
                let dumps=changes.as_array().unwrap().iter().filter(|c|c["path"].as_str().unwrap_or("").to_lowercase().contains("crash")).collect::<Vec<_>>();
                let exit_ok=self.report["enabled_exit"]==0 && self.report["disabled_exit"]==0 && self.report["exit_hook"]["increased"]==true;
                let logs_ok=self.report["repentogon_errors"]["new_in_enabled"].as_array().is_some_and(Vec::is_empty);
                self.case("E7",allowed && dumps.is_empty() && exit_ok && logs_ok && self.report["save_compare_error"].is_null(),json!({"changes":changes,"crash_changes":dumps,"normal_exits":exit_ok,"repentogon_log_ok":logs_ok}));
                write_json(&self.out.join("save-changes.json"),&changes)?;
            }
        }
        let pass=self.report["failure"].is_null() && self.report["cleanup_error"].is_null() && ["E1","E2","E3","E4","E5","E6","E7"].iter().all(|id|self.report["cases"][id]["pass"]==true);
        self.report["pass"]=json!(pass);self.report["automatic_retry"]=json!(false);
        write_json(&self.out.join("report.json"),&self.report)?;
        let mut md=format!("# tx-04-04 E2E\n\nResult: **{}**\n\n",if pass{"PASS"}else{"FAIL"});
        for id in ["E1","E2","E3","E4","E5","E6","E7"] {let c=&self.report["cases"][id];md+=&format!("- {id}: {}\n",if c.is_null(){"NOT_RUN"}else if c["pass"]==true{"PASS"}else{"FAIL"});}
        md+=&format!("\nFailure: {}\n\nFull evidence: report.json, steps.jsonl, enabled/, disabled/, screenshots, save_before/, save_after/. No automatic retry. No save restoration.\n",self.report["failure"]);
        fs::write(self.out.join("report.md"),md).map_err(|e|e.to_string())?;
        if pass{Ok(())}else{Err(format!("E2E failed; see {}",self.out.join("report.json").display()))}
    }
}
fn main() {
    if let Err(e)=entry(){eprintln!("{e}");std::process::exit(1);}
}
fn entry()->Result<()> {
    let args=env::args().skip(1).collect::<Vec<_>>();
    if args.get(0).map(String::as_str)!=Some("e2e") || args.get(1).map(String::as_str)!=Some("tx-04-04"){return Err("usage: harness e2e tx-04-04 --game <game-root> --out <new-artifact-directory> [--scenario <json>]".into());}
    let option=|key:&str|args.iter().position(|a|a==key).and_then(|i|args.get(i+1)).cloned();
    let game=PathBuf::from(option("--game").ok_or("--game required")?).canonicalize().map_err(|e|e.to_string())?;
    let out=PathBuf::from(option("--out").ok_or("--out required")?);
    if out.exists(){return Err("artifact directory already exists; no automatic rerun".into());}
    mkdir(&out)?;let out=out.canonicalize().map_err(|e|e.to_string())?;
    let repo=PathBuf::from(env!("CARGO_MANIFEST_DIR")).parent().unwrap().parent().unwrap().to_path_buf();
    let scenario_path=option("--scenario").map(PathBuf::from).unwrap_or_else(||repo.join("tools/harness/scenarios/tx-04-04.json"));
    let scenario:Value=serde_json::from_str(&read(&scenario_path)?).map_err(|e|e.to_string())?;
    write_json(&out.join("scenario.json"),&scenario)?;
    let save=PathBuf::from(env::var("USERPROFILE").map_err(|e|e.to_string())?).join("Documents/My Games/Binding of Isaac Repentance+");
    let mut run=Run{repo,game,out:out.clone(),save,scenario,report:json!({"tx":"tx-04-04","cases":{},"cleanup_error":null}),steps:fs::File::create(out.join("steps.jsonl")).map_err(|e|e.to_string())?,deployed:false,fixture_owned:false};
    // Capture unexpected Rust panics as a failed report and perform cleanup.
    let result=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||run.execute())).unwrap_or_else(|_|Err("harness panic".into()));
    run.finish(result)
}
