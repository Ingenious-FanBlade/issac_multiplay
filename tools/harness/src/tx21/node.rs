//! 真实Windows节点：仅操作本次启动的游戏、固定控制口、物理窗口及临时文件。
use crate::{Control,Result,win32,saves,write_json,read,mkdir};
use serde_json::{Value,json};
use std::{collections::{BTreeMap,BTreeSet},fs::{self,File,OpenOptions},io::{Read,Write,Seek,SeekFrom},net::{TcpListener,TcpStream},path::{Path,PathBuf},thread,time::{Duration,Instant}};

const FILE_LIMIT:u64=128*1024*1024;
pub(super) fn string<'a>(v:&'a Value,k:&str)->Result<&'a str>{v[k].as_str().ok_or_else(||format!("node_field:{k}"))}
pub(super) fn number(v:&Value,k:&str)->Result<u64>{v[k].as_u64().or_else(||v[k].as_str()?.parse().ok()).ok_or_else(||format!("node_number:{k}"))}
pub(super) fn hex(bytes:&[u8])->String{const H:&[u8]=b"0123456789abcdef";let mut s=String::with_capacity(bytes.len()*2);for b in bytes{s.push(H[(b>>4)as usize]as char);s.push(H[(b&15)as usize]as char);}s}
pub(super) fn unhex(s:&str)->Result<Vec<u8>>{if s.len()%2!=0{return Err("node_hex_length".into())}s.as_bytes().chunks_exact(2).map(|p|{let d=|c|match c{b'0'..=b'9'=>Ok(c-b'0'),b'a'..=b'f'=>Ok(c-b'a'+10),_=>Err("node_hex_digit".to_string())};Ok(d(p[0])?*16+d(p[1])?)}).collect()}

// 所有原生进程/输入只由节点主线程持有；远端请求携带纯值，不存在任意命令执行RPC。
struct Node {
 config:Value,root:PathBuf,attempt:PathBuf,game:Option<win32::Game>,control:Option<Control>,held:Option<win32::HeldKeys>,keys:BTreeSet<String>,
 events:Option<File>,cursor:u64,rows:u64,bytes:u64,last_pump:Instant,last_status:Instant,trace_enabled:bool,
 saves:Option<saves::SaveRoots>,status:BTreeMap<String,String>,launches:u64,shutdown:bool,finished:Option<Value>,
}
impl Node {
 // 额外设置只保存一次原字节；独立于存档快照，绝不回写游戏进度来掩盖副作用。
 fn setting(&self,path:&Path,content:Option<&[u8]>)->Result<()>{
  let journal=self.root.join("settings.json");let mut rows:Vec<Value>=if journal.exists(){serde_json::from_str(&read(&journal)?).map_err(|e|e.to_string())?}else{Vec::new()};
  if !rows.iter().any(|r|r["path"]==json!(path)){
   let existed=path.is_file();let backup=self.root.join(format!("setting-{}.original",rows.len()));
   if existed{if path.symlink_metadata().map_err(|e|e.to_string())?.file_type().is_symlink(){return Err("setting_symlink".into())}fs::copy(path,&backup).map_err(|e|e.to_string())?;}
   rows.push(json!({"path":path,"backup":backup,"existed":existed}));write_json(&journal,&json!(rows))?;
  }
  if let Some(bytes)=content{if let Some(parent)=path.parent(){mkdir(parent)?;}fs::write(path,bytes).map_err(|e|e.to_string())?;}else if path.is_file(){fs::remove_file(path).map_err(|e|e.to_string())?;}Ok(())
 }
 fn restore_settings(&self)->Result<()>{
  let journal=self.root.join("settings.json");if !journal.exists(){return Ok(())}let rows:Value=serde_json::from_str(&read(&journal)?).map_err(|e|e.to_string())?;
  for row in rows.as_array().ok_or("settings_journal")?.iter().rev(){let path=Path::new(string(row,"path")?);
   // 记录只由setting写入；还原仍只接受已声明的安装/存档设置根。
   if !path.starts_with(string(&self.config,"game_root")?)&&!path.starts_with(string(&self.config,"save_directory")?){return Err("settings_restore_escaped_declared_roots".into())}
   if row["existed"]==true{fs::copy(string(row,"backup")?,path).map_err(|e|e.to_string())?;}else if path.is_file(){fs::remove_file(path).map_err(|e|e.to_string())?;}
  }Ok(())
 }
 fn install(&mut self,flavor:&str)->Result<Value>{
  if !matches!(flavor,"product"|"e2e")||self.game.as_ref().is_some_and(|g|g.exited().is_none()){return Err("flavor_requires_stopped_game".into())}
  let package=PathBuf::from(string(&self.config,if flavor=="product"{"product_package"}else{"e2e_package"})?).canonicalize().map_err(|e|e.to_string())?;
  let game=PathBuf::from(string(&self.config,"game_root")?).canonicalize().map_err(|e|e.to_string())?;
  let manifest:Value=serde_json::from_str(&read(&package.join("deployment.json"))?).map_err(|e|e.to_string())?;
  if manifest["format"]!="isaac.cs.deployment"||manifest["build"]["flavor"]!=flavor{return Err("node_package_flavor".into())}
  for row in manifest["files"].as_array().ok_or("deployment_files")?{
   let relative=Path::new(string(row,"relative")?);if relative.starts_with("e2e-tools"){continue}
   if relative.is_absolute()||relative.components().any(|c|matches!(c,std::path::Component::ParentDir|std::path::Component::Prefix(_)|std::path::Component::RootDir)){return Err("package_relative_path".into())}
   let source=package.join(relative).canonicalize().map_err(|e|e.to_string())?;if !source.starts_with(&package){return Err("package_source_escape".into())}
   let destination=game.join(relative);if !destination.starts_with(&game){return Err("package_destination_escape".into())}mkdir(destination.parent().ok_or("package_parent")?)?;fs::copy(source,destination).map_err(|e|e.to_string())?;
  }
  for name in ["isaac-cs-ui","isaac-cs-observation","isaac-cs-test"]{
   let path=PathBuf::from(string(&self.config,"game_root")?).join("mods").join(name).join("disable.it");
   self.setting(&path,if flavor=="product"&&name=="isaac-cs-test"{Some(b"tx21 product has no fixture\n")}else{None})?;
  }
  // 真实网络验收只加载本项目模组；逐目录记录原禁用标记，正常收尾恢复，避免现有玩法Mod介入裁决。
  let mods=PathBuf::from(string(&self.config,"game_root")?).join("mods");
  for entry in fs::read_dir(&mods).map_err(|e|e.to_string())?{
   let entry=entry.map_err(|e|e.to_string())?;let kind=entry.file_type().map_err(|e|e.to_string())?;
   if kind.is_symlink(){return Err("third_party_mod_symlink_requires_explicit_owner".into())}
   if !kind.is_dir(){continue}let name=entry.file_name();
   if ["isaac-cs-ui","isaac-cs-observation","isaac-cs-test"].iter().any(|own|name==std::ffi::OsStr::new(own)){continue}
   self.setting(&entry.path().join("disable.it"),Some(b"tx21 temporary isolation;restored after acceptance\n"))?;
  }
  // 产品切换不能残留E2E界面源码；原部署由统一部署备份负责最终还原。
  if flavor=="product"{let path=game.join("mods/isaac-cs-ui/e2e.lua");if path.is_file(){fs::remove_file(path).map_err(|e|e.to_string())?;}}
  Ok(json!({"flavor":flavor,"build":manifest["build"],"fixture_disabled":flavor=="product"}))
 }
 fn new(config:Value)->Result<Self>{
  let root=PathBuf::from(string(&config,"run_directory")?);mkdir(&root)?;
  if !root.is_absolute()||root.symlink_metadata().map_err(|e|e.to_string())?.file_type().is_symlink(){return Err("node_absolute_ordinary_temporary_root".into())}
  win32::set_auto_focus(false);
  Ok(Self{config,root,attempt:PathBuf::new(),game:None,control:None,held:None,keys:BTreeSet::new(),events:None,cursor:0,rows:0,bytes:0,last_pump:Instant::now(),last_status:Instant::now(),trace_enabled:false,saves:None,status:BTreeMap::new(),launches:0,shutdown:false,finished:None})
 }
 fn command(&mut self,text:&str)->Result<BTreeMap<String,String>>{
  if text.len()>512||text.chars().any(|c|matches!(c,'\r'|'\n'|'\0'))||!text.starts_with("cs "){return Err("node_control_line".into())}
  if self.control.is_none(){let g=self.game.as_ref().ok_or("node_game_missing")?;let port=number(&self.config,"control_port")?;if port==0||port>65535{return Err("node_control_port".into())}self.control=Some(Control::connect(port as u16,g,90,&self.attempt.join("control.jsonl"))?);}
  self.control.as_mut().unwrap().request(text)
 }
 fn record(&mut self,mut row:Value)->Result<()>{
  row["node"]=self.config["id"].clone();let text=serde_json::to_vec(&row).map_err(|e|e.to_string())?;
  if self.rows>=2000000||self.bytes+text.len()as u64+1>FILE_LIMIT{return Err("node_case_trace_bound_requires_new_case_segment".into())}
  let f=self.events.as_mut().ok_or("node_trace_not_open")?;f.write_all(&text).and_then(|_|f.write_all(b"\n")).map_err(|e|e.to_string())?;self.rows+=1;self.bytes+=text.len()as u64+1;Ok(())
 }
 fn pump(&mut self)->Result<()>{
  if !self.trace_enabled||self.last_pump.elapsed()<Duration::from_millis(50){return Ok(())}self.last_pump=Instant::now();
  if self.game.as_ref().and_then(|g|g.exited()).is_some(){self.trace_enabled=false;return Ok(())}
  let token=string(&self.config,"e2e_token")?.to_owned();
  // 页数和单轮时间均有界，持续产生事件时也及时处理状态、停止和退出请求。
  let drain_started=Instant::now();
  for _ in 0..16{
   let r=self.command(&format!("cs stream_test {token} trace {}",self.cursor))?;
   if r.get("ok").map(String::as_str)!=Some("1"){return Err(format!("node_trace_rejected:{r:?}"))}
   let page:Value=serde_json::from_str(r.get("data").ok_or("node_trace_data")?).map_err(|e|e.to_string())?;
   if number(&page,"lost")?>0{self.record(json!({"event":"missing","lost":page["lost"],"reason":"DLL diagnostic loss"}))?;}
   let rows=page["rows"].as_array().ok_or("node_trace_rows")?;
   if number(&page,"first")?>self.cursor.saturating_add(1)&&self.cursor>0{self.record(json!({"event":"missing","reason":"trace cursor gap"}))?;}
   for row in rows{self.record(row.clone())?;}
   let more=page["more"].as_bool().ok_or("node_trace_more_missing")?;
   let next=number(&page,"next")?;let done=!more||next==self.cursor;self.cursor=next;
   if done||drain_started.elapsed()>=Duration::from_millis(200){break}
  }
  if self.last_status.elapsed()>=Duration::from_secs(1){
   self.last_status=Instant::now();self.status=self.command("cs status")?;let clock=self.command(&format!("cs stream_test {token} clock"))?;
   let local=clock.get("local_us").and_then(|s|s.parse::<u64>().ok()).ok_or("node_clock_missing")?;
   let counters=self.command("cs counters")?;let stream=self.command(if self.config["role"]=="Host"{"cs stream source 0"}else{"cs stream client 0"})?;
   // 读取现有有界累计计时，失败收尾前也保留各域来源成本；不另触发采集。
   let source_timings=self.command(&format!("cs client_test {token} fault-status"))?;
   self.record(json!({"event":"status","local_us":local,"status":self.status,"counters":counters,"stream":stream,"ui_clock":clock,"source_timings":source_timings}))?;
   let memory=win32::process_memory(self.game.as_ref().ok_or("game")?.pid())?;
   // 原生/全进程与自有池分列；没有发布池数据时保留空，不用工作集补造。
   let pools=["values","wire","workspace"].iter().enumerate().filter_map(|(i,pool)|{
    let key=format!("stream.pool.{i}");let values=self.status.get(&key).or_else(||stream.get(&key))?.split('/').map(str::parse::<u64>).collect::<std::result::Result<Vec<_>,_>>().ok()?;
    if values.len()!=3{return None}Some(json!({"name":pool,"current":values[0],"high_water":values[1],"limit":values[2]}))
   }).collect::<Vec<_>>();
   self.record(json!({"event":"memory","local_us":local,"private_bytes":memory["private_bytes"],"working_set":memory["working_set"],"pools":pools}))?;
   // 慢主线程观测完成后再开始间隔，避免每个RPC之间连续执行整组状态命令。
   self.last_status=Instant::now();
  }
  if let Some(f)=&mut self.events{f.flush().map_err(|e|e.to_string())?;}Ok(())
 }
 fn physical(&mut self,p:&Value)->Result<Value>{
  let ms=number(p,"hold_ms")?;if ms>5000{return Err("node_physical_duration".into())}
  let wanted=p["keys"].as_array().ok_or("node_keys")?.iter().map(|k|k.as_str().map(str::to_owned).ok_or("node_key".into())).collect::<Result<BTreeSet<_>>>()?;
  if wanted.is_empty()&&self.game.as_ref().is_none_or(|g|g.exited().is_some()){self.held=None;self.keys.clear();return Ok(json!({"released_owned_keys":true,"game_already_exited":true}))}
  if wanted.len()>16{return Err("node_key_count".into())}let w=self.game.as_ref().ok_or("node_game_missing")?.window()?;
  if !wanted.is_empty()&&win32::focus_state(w)["active"]!=true{return Err("node_confirmed_window_not_foreground_no_key_sent".into())}
  if !wanted.is_empty()&&self.trace_enabled{
   // 前台窗口不等于输入已解除屏蔽；必须等待真实Manager观察到UI关闭并完成交接。
   let token=string(&self.config,"e2e_token")?.to_owned();let until=Instant::now()+Duration::from_secs(10);
   loop{let clock=self.command(&format!("cs stream_test {token} clock"))?;
    if clock.get("ok").map(String::as_str)!=Some("1"){return Err(format!("node_ui_readiness_missing:{clock:?}"))}
    if super::interaction::ui_quiet(&clock)?{break}
    if Instant::now()>=until{return Err(format!("node_ui_still_blocked_no_key_sent:{clock:?}"))}self.pump()?;thread::sleep(Duration::from_millis(50));
   }
   if win32::focus_state(w)["active"]!=true{return Err("node_focus_changed_during_ui_handoff_no_key_sent".into())}
  }
  if self.held.is_none(){self.held=Some(win32::HeldKeys::new(w));}let keys=self.keys.difference(&wanted).cloned().collect::<Vec<_>>();
  for key in keys{self.held.as_mut().unwrap().release(&key)?;self.keys.remove(&key);}
  for key in wanted.difference(&self.keys).cloned().collect::<Vec<_>>(){self.held.as_mut().unwrap().press(&key)?;self.keys.insert(key);}
  let until=Instant::now()+Duration::from_millis(ms);while Instant::now()<until{self.pump()?;thread::sleep(Duration::from_millis(10));}
  Ok(json!({"window":w,"held":self.keys,"actual_SendInput":true,"hold_ms":ms}))
 }
 fn launch(&mut self,p:&Value)->Result<Value>{
  if self.game.as_ref().is_some_and(|g|g.exited().is_none()){return Err("node_existing_owned_game".into())}
  if win32::processes()?.iter().any(|s|s.eq_ignore_ascii_case("isaac-ng.exe")){return Err("node_other_game_process_present".into())}
  // 人工确认后再次读取，Steam可能在等待期间退出；失败时尚未修改本轮选项、配置或存档快照。
  win32::steam_ready()?;
  self.control=None;self.held=None;self.keys.clear();self.trace_enabled=false;self.launches+=1;self.finished=None;
  self.attempt=self.root.join(format!("launch-{}",self.launches));mkdir(&self.attempt)?;
  self.events=Some(OpenOptions::new().create_new(true).write(true).open(self.attempt.join("events.jsonl")).map_err(|e|e.to_string())?);self.rows=0;self.bytes=0;self.cursor=0;
  let game=win32::ordinary_path(Path::new(string(&self.config,"game_root")?))?;
  if self.config["allow_background"]==true{
   let save=PathBuf::from(string(&self.config,"save_directory")?);
   for relative in ["options.ini","Repentogon/vanilla_options.ini"]{let path=save.join(relative);if path.is_file(){let text=read(&path)?;let mut found=false;let patched=text.split_inclusive('\n').map(|line|{if line.trim().starts_with("PauseOnFocusLost="){found=true;format!("PauseOnFocusLost=0{}",if line.ends_with("\r\n"){"\r\n"}else if line.ends_with('\n'){"\n"}else{""})}else{line.to_owned()}}).collect::<String>();if !found{return Err("PauseOnFocusLost_missing_no_guessed_option".into())}self.setting(&path,Some(patched.as_bytes()))?;}}
  }
  let mut roots=saves::SaveRoots::discover(PathBuf::from(string(&self.config,"save_directory")?))?;
  let data=game.join("data");if !data.exists(){mkdir(&data)?;write_json(&self.attempt.join("created-mod-data.json"),&json!({"path":data,"created_empty_for_observation":true}))?;}roots.mod_data=Some(data);
  roots.snapshot(&self.attempt.join("save-before"))?;self.saves=Some(roots);
  let product=p["flavor"]=="product";
  let mut config:Value=serde_json::from_str(include_str!("../../../../mod/config.example.json")).map_err(|e|e.to_string())?;
  config["enabled"]=json!(true);config["control_port"]=json!({"enabled":!product,"port":self.config["control_port"]});
  config["host"]=json!({"default_port":self.config["host_port"],"bind_address":"0.0.0.0","peer_timeout_ms":30000});
  config["diagnostics"]=json!({"enabled":false,"repeat_topology_reads":false});
  config["trace_dir"]=json!(self.attempt.join("runtime-trace"));
  config["world_record"]=if product{json!({"enabled":false})}else{json!({"enabled":false,"e2e_token":string(&self.config,"e2e_token")?,"output_dir":self.attempt.join("runtime-world")})};
  // GUI只在本机Host的E2E版出现；产品和远端游戏不接入本机信箱。
  if !product{config["e2e_ui"]=json!({"enabled":self.config["ui_directory"].is_string(),"directory":self.config["ui_directory"]});}
  write_json(&game.join("Repentogon/isaac-cs/config.json"),&config)?;
  if !product{
   let fixture=&p["fixture"];let tx=fixture["tx"].as_str().unwrap_or("tx-04-21");
   let role=if self.config["role"]=="Host"{if tx=="tx-04-21"{"Host"}else{"Source"}}else{"Client"};
   let lua=|v:&Value|->Result<String>{serde_json::to_string(&serde_json::to_string(v).map_err(|e|e.to_string())?).map_err(|e|e.to_string())};
   let text=format!("-- 本次统一入口的临时配置，部署还原记录负责恢复。\nreturn {{tx={tx:?},node_role={role:?},token={:?},participants={},network_regression=true,actions_json={},scenes_json={}}}\n",string(&self.config,"e2e_token")?,number(p,"participants")?,lua(&fixture["actions"])?,lua(&fixture["scenes"])?);
   fs::write(game.join("mods/isaac-cs-test/harness_config.lua"),text).map_err(|e|e.to_string())?;
  }
  // 拓扑始终填写安装根目录；原生启动必须进入REPENTOGON运行目录，和部署/配置路径保持一致。
  self.game=Some(win32::Game::launch(&game.join("Repentogon"),&self.attempt.join("launcher"))?);
  let g=self.game.as_ref().unwrap();Ok(json!({"pid":g.pid(),"windows":win32::windows(g.pid()),"launch":self.launches,"directory":self.attempt,"flavor":p["flavor"]}))
 }
 fn finish(&mut self)->Result<Value>{
  if let Some(result)=&self.finished{return Ok(result.clone())}
  self.held=None;self.keys.clear();let _=self.pump();
  let mut failure=None;
  if let Some(g)=&self.game{if g.exited().is_none(){if let Some(c)=&mut self.control{let _=c.request("cs stop");}if let Err(e)=g.close(){failure=Some(e);}}}
  // 不因超时强杀或交还Off；原生退出未完成时保留owner与恢复材料。
  if let Some(e)=failure{return Err(e)}self.trace_enabled=false;self.control=None;
  let changes=if let Some(roots)=&self.saves{roots.snapshot(&self.attempt.join("save-after"))?;saves::changes(&self.attempt.join("save-before"),&self.attempt.join("save-after"))?}else{Value::Null};
  let created=self.attempt.join("created-mod-data.json");if created.is_file(){let v:Value=serde_json::from_str(&read(&created)?).map_err(|e|e.to_string())?;let data=PathBuf::from(string(&v,"path")?);let expected=PathBuf::from(string(&self.config,"game_root")?).join("data");if data!=expected{return Err("ModData恢复路径不同".into())}if data.is_dir()&&fs::read_dir(&data).map_err(|e|e.to_string())?.next().is_none(){fs::remove_dir(data).map_err(|e|e.to_string())?;}}
  if let Some(f)=&mut self.events{f.flush().map_err(|e|e.to_string())?;}
  let result=json!({"game_exited":self.game.as_ref().is_none_or(|g|g.exited().is_some()),"exit_code":self.game.as_ref().and_then(|g|g.exited()),"save_changes":changes,"trace_bytes":self.bytes,"launch":self.launches});self.finished=Some(result.clone());Ok(result)
 }
 fn dispatch(&mut self,op:&str,p:&Value)->Result<Value>{match op{
  "hello"=>Ok(json!({"node":self.config["id"],"machine":std::env::var("COMPUTERNAME").map_err(|e|e.to_string())?,"role":self.config["role"],"game_root":self.config["game_root"],"protocol":1})),
  "install"=>self.install(string(p,"flavor")?),
  "steam-ready"=>win32::steam_ready(),
  "launch"=>self.launch(p),
  "command"=>Ok(json!(self.command(string(p,"text")?)?)),
  "trace-on"=>{let token=string(&self.config,"e2e_token")?.to_owned();let r=self.command(&format!("cs stream_test {token} on"))?;if r.get("ok").map(String::as_str)!=Some("1"){return Err(format!("trace enable:{r:?}"))}self.trace_enabled=true;self.last_pump=Instant::now()-Duration::from_secs(1);Ok(json!(r))},
  "physical"=>self.physical(p),
  "window"=>{let g=self.game.as_ref().ok_or("node_game_missing")?;Ok(json!({"pid":g.pid(),"exited":g.exited(),"windows":win32::windows(g.pid()),"focus":g.window().ok().map(win32::focus_state)}))},
  "screen"=>{let g=self.game.as_ref().ok_or("node_game_missing")?;let w=g.window()?;win32::screenshot(w,&self.attempt.join("screen.bmp"))?;Ok(json!({"pid":g.pid(),"window":w,"bytes":fs::metadata(self.attempt.join("screen.bmp")).map_err(|e|e.to_string())?.len()}))},
  // Lua DebugString的夹具事实在实际存档目录log.txt；REPENTOGON自身日志不是夹具日志。
  // 首片冻结本次读取末尾，后续增长留给下一次观察，不能无限追赶活日志。
  "artifact"=>{let path=match string(p,"name")?{"events"=>self.attempt.join("events.jsonl"),"screen"=>self.attempt.join("screen.bmp"),"game-log"=>PathBuf::from(string(&self.config,"save_directory")?).join("log.txt"),_=>return Err("node_artifact_name".into())};let mut f=File::open(path).map_err(|e|e.to_string())?;let current=f.metadata().map_err(|e|e.to_string())?.len();let total=match p.get("end").filter(|v|!v.is_null()){Some(_)=>number(p,"end")?,None=>current};if total>FILE_LIMIT||total>current{return Err("node_artifact_bound_or_shrank".into())}let offset=number(p,"offset")?;if offset>total{return Err("node_artifact_offset".into())}f.seek(SeekFrom::Start(offset)).map_err(|e|e.to_string())?;let mut b=vec![0;(total-offset).min(524288)as usize];let n=f.read(&mut b).map_err(|e|e.to_string())?;b.truncate(n);Ok(json!({"offset":offset,"next":offset+n as u64,"total":total,"hex":hex(&b)}))},
  "finish"=>self.finish(),
  "shutdown"=>{let result=self.finish();if self.game.as_ref().is_some_and(|g|g.exited().is_none()){return Err("原生进程未退休，保持隔离并保留恢复材料".into())}self.restore_settings()?;self.shutdown=true;Ok(result.unwrap_or_else(|e|json!({"game_exited":true,"diagnostic_failure":e,"settings_restored":true})))},
  _=>Err(format!("node_unknown_operation:{op}"))
 }}
}
// helper异常退出也不借Game的兜底Drop强杀游戏；留下PID和恢复路径，实际关闭仍走窗口/原生退休。
impl Drop for Node{fn drop(&mut self){self.held=None;if let Some(game)=self.game.take(){if game.exited().is_none(){let _=write_json(&self.root.join("owned-process-still-running.json"),&json!({"pid":game.pid(),"game_root":self.config["game_root"],"reason":"helper ended before confirmed native exit;no forced termination"}));std::mem::forget(game);}}}}

// 有界逐行协议；超时只泵本方观测，不把半条TCP行丢弃，也不接受任意shell/路径。
pub(super) fn serve(path:&Path)->Result<()>{
 let config:Value=serde_json::from_str(&read(path)?).map_err(|e|e.to_string())?;
 let token=string(&config,"agent_token")?.to_owned();if token.len()<32||token.len()>128{return Err("node_agent_token".into())}
 let listener=TcpListener::bind(string(&config,"agent_listen")?).map_err(|e|e.to_string())?;let mut node=Node::new(config)?;
 while !node.shutdown{
  let(mut stream,_)=listener.accept().map_err(|e|e.to_string())?;stream.set_nodelay(true).map_err(|e|e.to_string())?;stream.set_read_timeout(Some(Duration::from_millis(50))).map_err(|e|e.to_string())?;stream.set_write_timeout(Some(Duration::from_secs(10))).map_err(|e|e.to_string())?;
  let mut pending=Vec::new();let mut bytes=[0u8;8192];let mut authenticated=false;let accepted=Instant::now();
  loop{
   match stream.read(&mut bytes){Ok(0)=>break,Ok(n)=>pending.extend_from_slice(&bytes[..n]),Err(e)if matches!(e.kind(),std::io::ErrorKind::WouldBlock|std::io::ErrorKind::TimedOut)=>{},Err(e)=>return Err(e.to_string())}
   if pending.len()>1048576{return Err("node_request_bound".into())}
   while let Some(end)=pending.iter().position(|b|*b==b'\n'){
    let line=pending.drain(..=end).collect::<Vec<_>>();let request:Value=serde_json::from_slice(&line).map_err(|e|e.to_string())?;
    if request["token"]!=token{break}authenticated=true;
    let result=node.dispatch(string(&request,"op")?,&request["payload"]);
    let response=match result{Ok(data)=>json!({"id":request["id"],"ok":true,"data":data}),Err(error)=>json!({"id":request["id"],"ok":false,"error":error})};
    let mut text=serde_json::to_vec(&response).map_err(|e|e.to_string())?;text.push(b'\n');stream.write_all(&text).map_err(|e|e.to_string())?;
   }
   if authenticated{if let Err(e)=node.pump(){node.trace_enabled=false;let _=node.record(json!({"event":"missing","reason":e}));}}
   if node.shutdown||!authenticated&&accepted.elapsed()>Duration::from_secs(10){break}
  }
  // 丢协调连接先释放全部物理键；游戏通过自身连接超时/正式退休收口，保留诊断。
  node.held=None;node.keys.clear();
 }
 Ok(())
}

// 协调器只调用固定RPC；每条回复绑定请求号，网络错误不变成缺字段的成功。
pub(super) struct Peer {pub config:Value,stream:TcpStream,pending:Vec<u8>,id:u64}
impl Peer {
 pub fn connect(config:Value)->Result<Self>{let address=string(&config,"agent_endpoint")?.parse().map_err(|e|format!("agent endpoint:{e}"))?;let until=Instant::now()+Duration::from_secs(90);let stream=loop{match TcpStream::connect_timeout(&address,Duration::from_millis(500)){Ok(stream)=>break stream,Err(e)=>{if Instant::now()>=until{return Err(format!("真实节点未准备:{}:{e}",config["id"]))}thread::sleep(Duration::from_millis(100));}}};stream.set_nodelay(true).map_err(|e|e.to_string())?;stream.set_read_timeout(Some(Duration::from_secs(180))).map_err(|e|e.to_string())?;stream.set_write_timeout(Some(Duration::from_secs(10))).map_err(|e|e.to_string())?;Ok(Self{config,stream,pending:Vec::new(),id:0})}
 pub fn call(&mut self,op:&str,payload:Value)->Result<Value>{
  self.id+=1;let mut text=json!({"id":self.id,"token":self.config["agent_token"],"op":op,"payload":payload}).to_string();if text.len()>1048576{return Err("RPC request budget".into())}text.push('\n');self.stream.write_all(text.as_bytes()).map_err(|e|e.to_string())?;
  loop{if let Some(end)=self.pending.iter().position(|b|*b==b'\n'){let line=self.pending.drain(..=end).collect::<Vec<_>>();let v:Value=serde_json::from_slice(&line).map_err(|e|e.to_string())?;if number(&v,"id")?!=self.id{return Err("RPC reply identity".into())}if v["ok"]!=true{return Err(format!("{} {op}: {}",self.config["id"],v["error"]))}return Ok(v["data"].clone())}let mut b=[0u8;8192];let n=self.stream.read(&mut b).map_err(|e|e.to_string())?;if n==0{return Err("RPC disconnected".into())}self.pending.extend_from_slice(&b[..n]);if self.pending.len()>4*1024*1024{return Err("RPC reply budget".into())}}
 }
 pub fn download(&mut self,name:&str,path:&Path)->Result<()>{let mut f=File::create(path).map_err(|e|e.to_string())?;let mut offset=0;let mut end=None;loop{let r=self.call("artifact",json!({"name":name,"offset":offset,"end":end}))?;let total=number(&r,"total")?;if end.is_some_and(|old|old!=total){return Err("artifact snapshot end changed".into())}end=Some(total);let next=number(&r,"next")?;let bytes=unhex(string(&r,"hex")?)?;if next!=offset+bytes.len()as u64||next>total||total>FILE_LIMIT||next==offset&&offset!=total{return Err("artifact page mismatch".into())}f.write_all(&bytes).map_err(|e|e.to_string())?;offset=next;if offset==total{return Ok(())}}}
}

