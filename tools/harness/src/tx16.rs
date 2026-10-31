//! tx-04-16统一入口的真实游戏节点；没有Rust输入客户端或离线协议替身。
use crate::{Control,Result,Run,win32};
use serde_json::{Value,json};
use std::{collections::{BTreeMap,BTreeSet},env,fs,net::UdpSocket,thread,time::{Duration,Instant}};
type Status=BTreeMap<String,String>;
fn n(s:&Status,k:&str)->u64{s.get(k).and_then(|v|v.parse().ok()).unwrap_or(0)}
impl Run {
    // 每个节点用同一启动/备份/恢复流程；成功节点材料仅在等待同轮四端合并时临时保留。
    pub(super) fn execute_tx16(&mut self)->Result<()> {
        // 用户使用虚拟机，需要自由切换窗口；本tx不自动置前、恢复或循环抢占焦点。
        // 真实SendInput仍要求游戏已经在前台，前台不符则停止发键，避免误输给虚拟机/其它程序。
        win32::set_auto_focus(false);
        self.report["node_role"]=self.scenario["node_role"].clone();
        self.report["cohort"]=self.scenario["cohort"].clone();
        self.report["peer_count"]=self.scenario["peer_count"].clone();
        self.report["machine"]=json!(env::var("COMPUTERNAME").map_err(|e|e.to_string())?);
        self.environment()?;self.report["crash_before"]=self.crash_files()?;self.snapshot("save_before")?;
        self.prepare_restore_tx08(1)?;self.disable_helper_tx08()?;
        if self.report["helper_marker_owned"]==true {
            if let Some(mods)=self.report["environment"]["mods"].as_array_mut(){for m in mods {if m["directory"]=="repentogonreleasesbackup_3643104060" {m["enabled"]=json!(false);}}}
        }
        self.input_environment_tx05()?;self.deploy()?;
        if self.scenario["node_role"]=="Host" {self.background_options_tx16()?;}
        let game=self.launch("enabled")?;self.report["pid"]=json!(game.pid());
        let result=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||self.scenes_tx16(&game)))
            .unwrap_or_else(|_|Err("tx16 node panic".into()));
        if let Err(e)=&result{self.capture_failure(&game,"enabled",e);let _=game.close();}
        self.collect(game.pid(),"enabled")?;result
    }
    // VM测试允许Host在后台继续原生模拟；先完整备份两个同步文件，再仅改当前失焦暂停选项。
    fn background_options_tx16(&mut self)->Result<()> {
        let source=self.save.join("options.ini");let text=crate::read(&source)?;
        if !text.lines().any(|line|line.trim()=="PauseOnFocusLost=1"||line.trim()=="PauseOnFocusLost=0") {
            return Err("PauseOnFocusLost option missing; no background configuration guessed".into());
        }
        let root=self.out.join("background-options-before");fs::create_dir_all(&root).map_err(|e|e.to_string())?;
        let mut entries=vec![];
        for (i,relative) in ["options.ini","Repentogon/vanilla_options.ini"].iter().enumerate() {
            let path=self.save.join(relative);let backup=root.join(format!("{i}.ini"));
            if path.exists() {
                if fs::symlink_metadata(&path).map_err(|e|e.to_string())?.file_type().is_symlink(){return Err("options symlink refused".into())}
                fs::copy(&path,&backup).map_err(|e|e.to_string())?;
            }
            entries.push(json!({"relative":relative,"backup":backup,"existed":path.exists()}));
        }
        self.report["background_options_restore"]=json!(entries);
        crate::write_json(&self.out.join("background-options-restore.json"),&self.report["background_options_restore"])?;
        self.report["background_options_changed"]=json!(true);
        let mut patched=String::new();
        for line in text.split_inclusive('\n') {
            if line.trim().starts_with("PauseOnFocusLost=") {
                patched.push_str("PauseOnFocusLost=0");
                if line.ends_with("\r\n"){patched.push_str("\r\n")}else if line.ends_with('\n'){patched.push('\n')}
            }else{patched.push_str(line)}
        }
        fs::write(source,patched).map_err(|e|e.to_string())?;
        self.step("host_background_run",json!({"PauseOnFocusLost":0,"automatic_focus":false,"restore_on_finish":true}))
    }
    // 游戏退出后恢复实际before文件，包括REPENTOGON启动/退出时同步的副本；失败不计PASS。
    pub(super) fn restore_background_options_tx16(&mut self)->Result<()> {
        if self.report["background_options_changed"]!=true{return Ok(())}
        let save=self.save.canonicalize().map_err(|e|e.to_string())?;
        for entry in self.report["background_options_restore"].as_array().ok_or("options restore journal")? {
            let relative=entry["relative"].as_str().ok_or("options relative path")?;
            if !matches!(relative,"options.ini"|"Repentogon/vanilla_options.ini"){return Err("unknown options restore path".into())}
            let target=self.save.join(relative);let parent=target.parent().ok_or("options parent")?.canonicalize().map_err(|e|e.to_string())?;
            if !parent.starts_with(&save){return Err("options restore escaped save directory".into())}
            if target.exists()&&fs::symlink_metadata(&target).map_err(|e|e.to_string())?.file_type().is_symlink(){return Err("options restore symlink refused".into())}
            if entry["existed"]==true {
                fs::copy(entry["backup"].as_str().ok_or("options backup")?,&target).map_err(|e|e.to_string())?;
            }else if target.exists(){fs::remove_file(&target).map_err(|e|e.to_string())?;}
        }
        self.report["background_options_changed"]=json!(false);
        self.step("host_background_options_restored",json!({"restored":true}))
    }
    // 仅轮询现有前台，不激活窗口；给用户切换宿主/VM的明确操作窗口，禁止误输其它程序。
    fn manual_foreground_tx16(&mut self,g:&win32::Game,w:usize,stage:&str)->Result<()> {
        self.step("manual_foreground_required",json!({"stage":stage,"window":w,"timeout_seconds":180}))?;
        println!("Select the game window manually; automatic focus is disabled. Stage: {stage}");
        let deadline=Instant::now()+Duration::from_secs(180);
        loop {
            if let Some(code)=g.exited(){return Err(format!("game exited waiting for manual foreground: {code}"))}
            if win32::focus_state(w)["active"]==true {return self.step("manual_foreground_ready",json!({"stage":stage}));}
            if Instant::now()>=deadline{return Err(format!("manual game foreground required: {stage}"))}
            thread::sleep(Duration::from_millis(150));
        }
    }
    fn wait_tx16(&mut self,g:&win32::Game,c:&mut Control,label:&str,seconds:u64,p:impl Fn(&Status)->bool)->Result<Status>{
        let end=Instant::now()+Duration::from_secs(seconds);
        loop{let s=self.watch_tx05(g,c,0)?;if p(&s){self.step(label,json!(s))?;return Ok(s)}
            if s.get("client_state").is_some_and(|v|v=="Failed")&&label!="expected_timeout"{return Err(format!("{label}: {s:?}"))}
            if Instant::now()>=end{return Err(format!("{label} timeout: {s:?}"))}thread::sleep(Duration::from_millis(80));}
    }
    // 检查Client完整窗口内没有进入原生模拟/Start/Restore，而不只看最后run_active。
    fn unchanged_simulation(&mut self,c:&mut Control,before:&Status)->Result<()> {
        let after=c.command("cs counters")?;
        for name in ["start_game","game_update","room_update","entity_list","odd_helper","restore","post_update","player_update","join_pass"] {
            let key=format!("hook.{name}");if n(before,&key)!=n(&after,&key){return Err(format!("Client entered {name}"))}
        }
        self.case("client_native_chain_stopped",true,json!({"before":before,"after":after}));Ok(())
    }
    fn scenes_tx16(&mut self,g:&win32::Game)->Result<()> {
        let w=g.wait_window(90)?;let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("port")? as u16,g,90,&self.out.join("control.jsonl"))?;
        c.wait_startup(g,w,90)?;
        // 控制口只读快照/排队主线程命令，不要求窗口焦点；实际发键和截图仍独立确认焦点。
        // Client等待房主时可能长时间失焦，不能因此把已建立的网络连接提前结束。
        c.focus_window=None;
        c.wait("client_ui_available","1",15)?;
        if self.scenario["node_role"]=="Host" {return self.host_tx16(g,w,&mut c)}
        if self.scenario["node_role"]=="Local" {
            // 绑定真实UDP黑洞以覆盖超时；不冒充Host，也不声称完成真实四端验收。
            let blackhole=UdpSocket::bind("127.0.0.1:0").map_err(|e|e.to_string())?;
            let port=blackhole.local_addr().map_err(|e|e.to_string())?.port();
            let resume=self.scenario["resume_local"].as_str().map(std::path::PathBuf::from);
            if let Some(source)=&resume {
                let old:Value=serde_json::from_str(&crate::read(&source.join("result.json"))?).map_err(|e|e.to_string())?;
                if old["tx"]!="tx-04-16"||old["node_role"]!="Local"{return Err("wrong local continuation source".into())}
                for name in ["client_native_chain_stopped"] {
                    if old["cases"][name]["pass"]!=true{return Err(format!("source {name} not passed"))}
                    self.report["cases"][name]=old["cases"][name].clone();
                }
                self.report["reused_unaffected"]=json!({"source":source,"cases":["client_native_chain_stopped"]});
            }
            c.command(&format!("cs client 127.0.0.1 {port} 0"))?;c.wait("role","Client",10)?;
            let before=c.command("cs counters")?;
            let initial=c.command("cs status")?;
            for key in ["ENTER","RIGHT","ESC"] {win32::key(w,key)?;}
            let failed=self.wait_tx16(g,&mut c,"expected_timeout",10,|s|s.get("client_state").is_some_and(|v|v=="Failed"))?;
            if failed.get("run_active").map(String::as_str)!=Some("0")||failed.get("menu_id")!=initial.get("menu_id")||n(&failed,"client.manager_skipped")==0||n(&failed,"client.sampled")==0||n(&failed,"client.ui_frames")==0 {
                return Err(format!("Client menu/poll isolation: {failed:?}"));
            }
            if resume.is_none(){self.unchanged_simulation(&mut c,&before)?;}
            win32::screenshot(w,&self.out.join("client-failed-wait.bmp"))?;
            self.case("menu_wait_timeout_and_physical_poll",true,json!({"initial":initial,"failed":failed}));
            c.command("cs stop")?;c.wait("role","Off",10)?;
            c.command(&format!("cs client 127.0.0.1 {port} 0"))?;c.wait("role","Client",10)?;
            c.command("cs stop")?;c.wait("role","Off",10)?;
            self.case("stop_connecting_and_new_connection",true,json!(c.command("cs status")?));
            // 同一DLL从已排空Client切换Host再Off；不启动本地run。
            c.command(&format!("cs host {}",self.scenario["host_port"]))?;c.wait("role","Host",10)?;
            c.command("cs stop")?;c.wait("session_state","Idle",10)?;
            self.case("client_host_off_switch",true,json!(c.command("cs status")?));
        } else {
            let address=self.scenario["host_address"].as_str().ok_or("host IPv4")?.to_string();
            c.command(&format!("cs client {address} {} 0",self.scenario["host_port"]))?;
            c.wait("role","Client",10)?;let before=c.command("cs counters")?;
            let peers=self.scenario["peer_count"].as_u64().ok_or("peer_count")?;
            let mask=(1u64<<(peers+1))-1;
            let ready=self.wait_tx16(g,&mut c,"client_roster",600,|s|n(s,"client.roster")==mask&&n(s,"client.generation")>0)?;
            let ordinal=n(&ready,"client.participant");if !(1..=3).contains(&ordinal){return Err("participant identity".into())}
            self.report["participant"]=json!(ordinal);self.report["wire_session"]=json!(ready["client.wire_session"]);
            self.case("negotiation_and_roster",true,json!(ready));
            // 名单/代次不等于输入已经开放；先确认前台、UI关闭、时序就绪及真实中性输入被接纳。
            println!("Client connected; keep the game in foreground while waiting for Authority input readiness.");
            self.manual_foreground_tx16(g,w,"Client physical input")?;
            self.wait_tx16(g,&mut c,"client_input_ready",90,|s|n(s,"client.input_ready")==1&&n(s,"client.accepted")>0)?;
            let movement=match ordinal {1=>"D",2=>"A",_=>"S"};
            let expected_movement=match ordinal {1=>"32767,0",2=>"-32767,0",_=>"0,32767"};
            // 真实物理按键经过产品采样、canonical、UDP、Host原生consumer；不直接写状态。
            let mut keys=win32::HeldKeys::new(w);keys.press(movement)?;keys.press("RIGHT")?;
            thread::sleep(Duration::from_millis(700));
            self.wait_tx16(g,&mut c,"physical_movement_and_right_shoot",5,|s|s.get("client.sample_move").map(String::as_str)==Some(expected_movement)&&s.get("client.sample_aim").map(String::as_str)==Some("32767,0"))?;
            keys.press("LEFT")?;thread::sleep(Duration::from_millis(500));
            self.wait_tx16(g,&mut c,"physical_later_left_shoot",5,|s|s.get("client.sample_aim").map(String::as_str)==Some("-32767,0"))?;
            keys.release("LEFT")?;thread::sleep(Duration::from_millis(400));
            self.wait_tx16(g,&mut c,"physical_right_shoot_fallback",5,|s|s.get("client.sample_aim").map(String::as_str)==Some("32767,0"))?;
            keys.release("RIGHT")?;keys.release(movement)?;
            for action in ["E","SPACE","Q","CTRL"] {win32::key(w,action)?;thread::sleep(Duration::from_millis(300));}
            self.wait_tx16(g,&mut c,"all_physical_edges_sent",5,|s|n(s,"client.sent_edge_mask")==15)?;
            let sent=c.command("cs status")?;let sequence=n(&sent,"client.sequence");
            let ack=self.wait_tx16(g,&mut c,"actual_input_processed_ack",20,|s|n(s,"client.processed")>=sequence&&sequence>0)?;
            self.case("physical_input_and_processed_ack",true,json!(ack));self.unchanged_simulation(&mut c,&before)?;
            win32::screenshot(w,&self.out.join("client-authority-wait.bmp"))?;
            // 保留到Host主动结束，覆盖真正的服务端断开而非主动结束后忽略剩余消息。
            self.wait_tx16(g,&mut c,"expected_timeout",90,|s|s.get("client_state").is_some_and(|v|v=="Failed"))?;
            c.command("cs stop")?;c.wait("role","Off",10)?;
            self.case("server_disconnect_and_safe_off",true,json!(c.command("cs status")?));
        }
        self.report["final_status"]=json!(c.command("cs status")?);drop(c);
        if g.close()?!=0{return Err("game exit".into())}Ok(())
    }
    fn host_tx16(&mut self,g:&win32::Game,w:usize,c:&mut Control)->Result<()> {
        let holding=self.scenario["host_hold"].as_str().map(std::path::PathBuf::from);
        if let Some(signal)=&holding {
            // 用户要求先监听、配置穿透后再测试：菜单Host独立等待，不提前New Run或抢回焦点。
            if signal.parent()!=Some(self.out.as_path()) && signal.parent().and_then(|p|p.canonicalize().ok()).as_ref()!=Some(&self.out) {
                return Err("host hold path must stay in this temporary directory".into());
            }
            c.command(&format!("cs host {}",self.scenario["host_port"]))?;c.wait("role","Host",15)?;
            c.command("cs role 1 19")?;c.focus_window=None;
            crate::write_json(&self.out.join("host-ready.json"),&json!({"phase":"HostListeningAwaitingPairStart","pid":g.pid(),"window":w,
                "udp_port":self.scenario["host_port"],"cohort":self.scenario["cohort"],"signal":signal,"status":c.command("cs status")?}))?;
            println!("Host listening; waiting for explicit pair start: {}",signal.display());
            while !signal.exists() {
                if let Some(code)=g.exited(){return Err(format!("Host exited while waiting: {code}"))}
                let status=c.command("cs status")?;
                if status.get("role").map(String::as_str)!=Some("Host"){return Err("Host stopped while waiting for pair start".into())}
                thread::sleep(Duration::from_millis(1000));
            }
            self.step("pair_start_requested",json!({"signal":signal}))?;
        }
        // 由用户选择实机操作窗口；后台状态查询不维持前台，不妨碍虚拟机操作。
        c.focus_window=None;
        self.manual_foreground_tx16(g,w,"Host start run")?;
        self.prepare_new_run_tx05(g,w,c)?;self.sequence(w,"start_run")?;c.wait("run_active","1",30)?;
        // 测试局仅通过原生控制台打开无敌，避免等远端时死亡；不保存访客世界或解锁。
        win32::key(w,"CONSOLE")?;thread::sleep(Duration::from_millis(400));win32::text(w,"debug 10")?;win32::key(w,"ENTER")?;win32::key(w,"ESC")?;
        // 关闭控制台或切换前台可能已打开原生暂停页，不能在暂停状态等待Client动作。
        if n(&c.command("cs status")?,"pause_state")==1 {win32::key(w,"ESC")?;c.wait("pause_state","0",10)?;}
        if holding.is_none(){c.command(&format!("cs host {}",self.scenario["host_port"]))?;c.command("cs role 1 19")?;}
        c.wait("session_state","Active",30)?;
        let peers=self.scenario["peer_count"].as_u64().ok_or("peer_count")?;
        let ready=self.wait_tx16(g,c,"actual_participant_join",600,|s|n(s,"participants")==peers+1&&n(s,"active_bodies")>=peers+2&&(1..=peers).all(|i|n(s,&format!("participant.{i}.bound"))==1))?;
        self.report["wire_session"]=json!(ready["host.wire_session"]);self.case("real_clients_multi_body",true,json!(ready));
        // 只读取正式trace中的实际consumer结果；不能用收到包数冒充世界应用。
        let deadline=Instant::now()+Duration::from_secs(45);let mut witnessed=BTreeSet::new();
        loop {
            self.watch_tx05(g,c,0)?;
            let text=fs::read_to_string(self.trace_file(g.pid())).map_err(|e|e.to_string())?;
            for line in text.lines(){let Ok(row)=serde_json::from_str::<Value>(line)else{continue};
                if row["ev"]!="tick"{continue}let Some(detail)=row["detail"].as_str()else{continue};
                let fields=detail.split_whitespace().filter_map(|p|p.split_once('=')).collect::<BTreeMap<_,_>>();
                let i=fields.get("ordinal").and_then(|s|s.parse::<u64>().ok()).unwrap_or(0);
                if i>peers {continue}
                let expected=match i{1=>"32767,0",2=>"-32767,0",3=>"0,32767",_=>continue};
                if fields.get("move")==Some(&expected)&&fields.get("native_commit")==Some(&"1")&&fields.get("input_sequence")!=Some(&"0") {witnessed.insert(i);}
            }
            if witnessed.len()==peers as usize{break}if Instant::now()>=deadline{return Err(format!("native Client consumer witnesses missing: {witnessed:?}"))}
            thread::sleep(Duration::from_millis(150));
        }
        // 等待每个Client边沿组发完和ACK回程，之后统一由Host停止。
        thread::sleep(Duration::from_secs(6));let progress=c.command("cs status")?;
        if !(1..=peers).all(|i|n(&progress,&format!("participant.{i}.processed_sequence"))>0){return Err("Host processed progress".into())}
        if !(1..=peers).all(|i|(0..4).all(|a|n(&progress,&format!("participant.{i}.edge.{a}.processed_count"))>0)) {
            return Err("Host missing actual processed edge group".into());
        }
        self.case("all_control_groups_native_input",true,json!({"ordinals":witnessed,"progress":progress}));
        c.command("cs stop")?;c.wait("session_state","Idle",30)?;
        self.manual_foreground_tx16(g,w,"Host return to menu")?;
        self.return_to_menu(w,c)?;self.report["final_status"]=json!(c.command("cs status")?);
        if g.close()?!=0{return Err("host exit".into())}Ok(())
    }
}
