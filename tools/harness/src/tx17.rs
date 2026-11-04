//! tx17统一实机验收；失败条件包括未提交、漏隔离、旧身份复活、退休未退尽和进度写入。
//! 真实Host观测提供来源；具名profile进入真实Client公共applier，Rust不模拟Client世界。
use crate::{Control,Result,Run,win32};
use serde_json::json;
use std::{collections::BTreeMap,process::Command,thread,time::{Duration,Instant}};
type Status=BTreeMap<String,String>;
fn n(s:&Status,k:&str)->u64{s.get(k).and_then(|v|v.parse().ok()).unwrap_or(0)}
fn drawn(s:&Status)->u64{s.get("client.dispatch.presentation_draw").and_then(|v|v.split(',').next()).and_then(|v|v.parse().ok()).unwrap_or(0)}
// 完整状态仍保存在control记录；失败摘要只展示可定位的原生阶段，不淹没首个原因。
fn diagnosis(s:&Status)->String{["role","client.scene_state","client.native_phase","client.native_reason","client.native_written","client.native_executed_prefix","client.pending_owners","client.applied_revision","client.error"].iter().map(|k|format!("{k}={}",s.get(*k).map(String::as_str).unwrap_or("missing"))).collect::<Vec<_>>().join(" ")}
impl Run {
 fn wait_tx17(&mut self,g:&win32::Game,c:&mut Control,label:&str,p:impl Fn(&Status)->bool)->Result<Status>{
  let end=Instant::now()+Duration::from_secs(45);
  loop{let s=self.watch_tx05(g,c,0)?;if p(&s){self.step(label,json!(s))?;return Ok(s)}
   if s.get("client.native_phase").map(String::as_str)==Some("retired")&&s.get("client.error").is_some_and(|e|!e.is_empty()) {return Err(format!("{label} retired after failure: {}",diagnosis(&s)))}
   if Instant::now()>=end{return Err(format!("{label} timeout: {}",diagnosis(&s)))}thread::sleep(Duration::from_millis(70));}
 }
 fn target_tx17(&mut self,g:&win32::Game,c:&mut Control,label:&str,revision:u64,begin:bool)->Result<Status>{
  let token=self.report["token"].as_str().ok_or("token")?;
  let accepted=c.command(&format!("cs client_test {token} {} {label}",if begin{"begin"}else{"target"}))?;
  if accepted.get("accepted").map(String::as_str)!=Some("1"){return Err(format!("target {label} refused: {}",diagnosis(&c.command("cs status")?)))}
  self.wait_tx17(g,c,label,|s|n(s,"client_world_applied")==1&&n(s,"client.applied_revision")==revision)
 }
 pub(super) fn scenes_tx17(&mut self,g:&win32::Game)->Result<()> {
  // 本轮用户明确授权直接启动并自动测试；只激活本harness刚创建的游戏窗口。
  win32::set_auto_focus(true);
  let w=g.wait_window(90)?;let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("control port")? as u16,g,90,&self.out.join("control.jsonl"))?;
  c.focus_window=Some(w);self.prepare_new_run_tx05(g,w,&mut c)?;
  self.report["node_role"]=self.scenario["node_role"].clone();
  if self.scenario["node_role"]=="Source" {
  self.sequence(w,"start_run")?;c.wait("run_active","1",30)?;
  for action in 1..=2 {
  win32::key(w,"F8")?;
  let end=Instant::now()+Duration::from_secs(15);
  loop{let rows=self.live_fixture()?;if rows.iter().any(|r|r["event"]=="fixture_error"){return Err(format!("source fixture error: {rows:?}"))}
   if rows.iter().any(|r|r["event"]=="ready"&&r["action"]==action){break}if Instant::now()>=end{return Err("source fixture timeout".into())}thread::sleep(Duration::from_millis(100));}
  if action==1 {thread::sleep(Duration::from_millis(1200));}
  }
  c.command("cs world start tx17-source")?;
  let end=Instant::now()+Duration::from_secs(15);
  loop{let s=c.command("cs world status")?;if s.get("world_state").map(String::as_str)==Some("Recording"){break}if Instant::now()>=end{return Err("capture start".into())}thread::sleep(Duration::from_millis(80));}
  c.command("cs observe start")?;c.command(&format!("cs host {} 0",self.scenario["host_port"]))?;c.wait("session_state","Active",20)?;
  let before=c.command("cs observe status")?;let end=Instant::now()+Duration::from_secs(45);
  loop{self.watch_tx05(g,&mut c,0)?;let s=c.command("cs observe status")?;
   if n(&s,"lua_attempts")>=n(&before,"lua_attempts")+3{
    for domain in ["entity","grid","room","level","exploration","room_save","player","run","random","presentation","mod"]{
     if s.get(&format!("{domain}_coverage")).map(String::as_str)!=Some("Complete"){return Err(format!("source {domain}: {s:?}"))}}
    self.case("host_ordinary_source",true,json!(s));break;}
   if Instant::now()>=end{return Err("source capture deadline".into())}thread::sleep(Duration::from_millis(100));}
  win32::screenshot(w,&self.out.join("host-source.bmp"))?;
  c.command("cs world stop")?;let end=Instant::now()+Duration::from_secs(20);
  loop{let s=c.command("cs world status")?;if s.get("world_state").map(String::as_str)==Some("Closed"){break}if Instant::now()>=end{return Err("capture close".into())}thread::sleep(Duration::from_millis(80));}
  c.command("cs observe stop")?;c.command("cs stop")?;c.wait("session_state","Idle",20)?;self.return_to_menu(w,&mut c)?;
  let targets=self.out.join("runtime-world/client-targets");
  let generated=Command::new(self.repo.join("out/repentogon/e2e/client-fixture.exe")).arg(self.out.join("runtime-world/tx17-source/observation-v2/records.jsonl")).arg(&targets).output().map_err(|e|e.to_string())?;
  if !generated.status.success(){return Err(format!("source model: {}",String::from_utf8_lossy(&generated.stderr)))}
  self.step("formal_source_projection",json!({"output":String::from_utf8_lossy(&generated.stdout)}))?;
  // 原生Manager析构完成合法Host版本同步，随后Client才有独立、稳定的持久基线。
  drop(c);if g.close()?!=0{return Err("source game exit".into())}
  self.report["acceptance_stage"]=json!("source_ready_for_independent_client");return Ok(());
  }
  let source=std::path::PathBuf::from(self.scenario["source_run"].as_str().ok_or("source run")?);
  let source_report:serde_json::Value=serde_json::from_str(&crate::read(&source.join("result.json"))?).map_err(|e|e.to_string())?;
  if source_report["tx"]!="tx-04-17"||source_report["node_role"]!="Source"||source_report["pass"]!=true||source_report["cases"]["host_ordinary_source"]["pass"]!=true{return Err("source run did not finish normally".into())}
  crate::copy_tree(&source.join("runtime-world/client-targets"),&self.out.join("runtime-world/client-targets"))?;
  std::fs::copy(source.join("host-source.bmp"),self.out.join("host-source.bmp")).map_err(|e|e.to_string())?;
  self.report["source_run"]=json!(source);
  self.case("host_source_from_closed_process",true,source_report["cases"]["host_ordinary_source"]["detail"].clone());
  self.snapshot("client_before")?;c.focus_window=None;let before=c.command("cs counters")?;
  let initial=self.target_tx17(g,&mut c,"baseline",1,true)?;
  thread::sleep(Duration::from_millis(700));win32::screenshot(w,&self.out.join("client-world.bmp"))?;
  let token=self.report["token"].as_str().ok_or("token")?.to_owned();
  self.case("client_native_scene",true,json!({"initial":initial}));
  // 真实键盘打开/关闭REPENTOGON控制台，覆盖本地UI暂停输入期间的持续展示与恢复。
  win32::key(w,"CONSOLE")?;
  let paused=self.wait_tx17(g,&mut c,"client_ui_paused",|s|n(s,"client.ui_blocked")==1&&n(s,"client_world_applied")==1)?;
  self.wait_tx17(g,&mut c,"client_ui_paused_draw",|s|drawn(s)>=drawn(&paused)+2&&n(s,"client.applied_revision")==1)?;
  // 控制台中的反引号是文本输入；Esc才是原生关闭操作。
  win32::key(w,"ESC")?;
  let resumed=self.wait_tx17(g,&mut c,"client_ui_resumed",|s|n(s,"client.ui_blocked")==0&&n(s,"client_world_applied")==1)?;
  self.case("actual_ui_pause_and_resume",true,json!({"paused":paused,"resumed":resumed}));
  let duplicate=self.target_tx17(g,&mut c,"baseline",1,false)?;
  if n(&duplicate,"client.native_attempt")!=n(&initial,"client.native_attempt"){return Err("duplicate executed".into())}
  for (label,revision) in [("change",2),("back-to-baseline",3),("replace",4),("remove",5)]{self.target_tx17(g,&mut c,label,revision,false)?;}
  self.case("create_change_replace_remove_duplicate",true,json!(c.command("cs status")?));
  let accepted=c.command(&format!("cs client_test {token} target missing-resource"))?;
  if accepted.get("accepted").map(String::as_str)!=Some("1"){return Err("missing resource never reached native preflight".into())}
  let failed=self.wait_tx17(g,&mut c,"missing_resource_retired",|s|n(s,"client_world_applied")==0&&s.get("client.native_phase").map(String::as_str)==Some("retired"))?;
  self.case("partial_resource_failure_retirement",true,json!(failed));
  self.target_tx17(g,&mut c,"recover",7,false)?;
  let invalid=c.command(&format!("cs client_test {token} target invalid-pose"))?;
  if invalid.get("accepted").map(String::as_str)!=Some("0"){return Err("invalid pose accepted".into())}
  let intact=c.command("cs status")?;if n(&intact,"client_world_applied")!=1||n(&intact,"client.applied_revision")!=7{return Err("preflight failure lost valid display".into())}
  let pose=self.target_tx17(g,&mut c,"pose-and-reference",8,false)?;
  // Applied报告逻辑提交；截图另等真实draw，避免把前一目标的最后画面当作新姿态。
  self.wait_tx17(g,&mut c,"pose_drawn",|s|drawn(s)>=drawn(&pose)+2)?;
  win32::screenshot(w,&self.out.join("client-pose.bmp"))?;
  self.target_tx17(g,&mut c,"event-pose",9,false)?;
  thread::sleep(Duration::from_millis(400));
  self.target_tx17(g,&mut c,"clear-pose-reference",10,false)?;
  let budget=c.command(&format!("cs client_test {token} target resource-budget"))?;
  let retained=c.command("cs status")?;
  if budget.get("accepted").map(String::as_str)!=Some("0")||n(&retained,"client_world_applied")!=1||n(&retained,"client.applied_revision")!=10{return Err("resource budget did not preserve committed scene".into())}
  self.case("overlay_fractional_phase_dynamic_sheet_reference_clear_budget",true,json!(retained));
  for label in ["missing-reference","recover"] {
   let rejected=c.command(&format!("cs client_test {token} target {label}"))?;let current=c.command("cs status")?;
   if rejected.get("accepted").map(String::as_str)!=Some("0")||n(&current,"client_world_applied")!=1||n(&current,"client.applied_revision")!=10{return Err(format!("invalid reference/stale revision changed display: {label}"))}
  }
  self.target_tx17(g,&mut c,"new-scene",11,false)?;
  let stale=c.command(&format!("cs client_test {token} target recover"))?;if stale.get("accepted").map(String::as_str)!=Some("0"){return Err("retired scope revived".into())}
  self.case("scope_and_preflight_recovery",true,json!(c.command("cs status")?));
  let ownership=c.command(&format!("cs client_test {token} exercise ownership"))?;
  if ownership.get("ownership_exercise").map(String::as_str)!=Some("1"){return Err("thread/context/borrow ownership exercise".into())}
  self.case("actual_thread_context_and_released_borrow",true,json!(ownership));
  // 已发布世界外突然进入玩法root属于故障；必须先阻断，下一Manager真实退休。
  let denied_before=c.command("cs status")?;
  let isolated=c.command(&format!("cs client_test {token} exercise native"))?;
  if isolated.get("isolation_exercise").map(String::as_str)!=Some("1")||isolated.get("loading_exercise").map(String::as_str)!=Some("1"){return Err("native/loading isolation exercise".into())}
  self.wait_tx17(g,&mut c,"unexpected_native_root_retired",|s|n(s,"client_world_applied")==0&&s.get("client.native_phase").map(String::as_str)==Some("retired"))?;
  self.case("client_native_and_persistence_isolation",true,json!(isolated));
  let after=c.command("cs counters")?;
  let denied_after=c.command("cs status")?;
  if n(&denied_after,"persist_client_denied_native")<=n(&denied_before,"persist_client_denied_native")||n(&denied_after,"persist_client_denied_lua")<n(&denied_before,"persist_client_denied_lua")+6{return Err("native/Lua/ModData persistence front guard not observed".into())}
  for name in ["start_game","game_update","room_update","entity_list","odd_helper","restore","post_update","player_update","join_pass"]{
   let key=format!("hook.{name}");if n(&initial,&format!("client.original_begin.{name}"))!=n(&after,&key){return Err(format!("Client gameplay entered {name}"))}}
  let status=c.command("cs status")?;for (key,value) in &status{if key.starts_with("client.dispatch.")&&value.split(',').nth(2)!=Some("0")&&!["client.dispatch.game_update","client.dispatch.player_update","client.dispatch.door_update","client.dispatch.room_update","client.dispatch.odd_helper","client.dispatch.room_init"].contains(&key.as_str()){return Err(format!("unplanned dispatch fault {key}={value}"))}}
  if n(&status,"client.callback_fault")!=0||n(&status,"client.callback_skipped")==0{return Err("callback isolation witness".into())}
  self.case("no_simulation_or_callback_leak",true,json!({"before":before,"after":after,"status":status}));
  c.command("cs stop")?;c.wait("role","Off",20)?;self.snapshot("client_after")?;
  let changes=crate::saves::changes(&self.out.join("client_before"),&self.out.join("client_after"))?;
  // Client窗口不允许任何进度文件变更；启动/source局的允许变更由独立收尾规则检查。
  if changes.as_array().ok_or("client file changes")?.iter().any(|v|v["bytes_changed"]==true&&v["path"].as_str()!=Some("documents/log.txt")){return Err(format!("Client progress changes: {changes}"))}
  self.case("client_window_persistence",true,json!({"files":changes}));
  let preparing=c.command(&format!("cs client_test {token} begin-cancel baseline"))?;
  if preparing.get("cancelled_before_pump").map(String::as_str)!=Some("1"){return Err("prepare cancellation not admitted".into())}
  c.wait("role","Off",20)?;
  self.case("stop_during_preparation",true,json!(preparing));
  self.target_tx17(g,&mut c,"baseline",1,true)?;c.command("cs stop")?;c.wait("role","Off",20)?;
  c.command(&format!("cs host {}",self.scenario["host_port"]))?;c.wait("role","Host",10)?;c.command("cs stop")?;c.wait("role","Off",10)?;
  // 在真实Client展示中关闭窗口，实际触发退休协调及Shutdown保护，不先人为切Off。
  self.target_tx17(g,&mut c,"baseline",1,true)?;
  self.report["final_status"]=json!(c.command("cs status")?);drop(c);if g.close()?!=0{return Err("game exit".into())}
  self.snapshot("client_exit")?;
  let exit_changes=crate::saves::changes(&self.out.join("client_before"),&self.out.join("client_exit"))?;
  // 正常窗口关闭允许具名本地Options落盘；进度/Continue/官方JSON仍不允许Client改变。
  if exit_changes.as_array().ok_or("client exit changes")?.iter().any(|v|v["bytes_changed"]==true&&
     !matches!(v["path"].as_str(),Some("documents/log.txt"|"documents/options.ini"))){return Err(format!("Client exit progress changes: {exit_changes}"))}
  self.case("client_shutdown_persistence",true,json!(exit_changes));
  self.case("retire_reconnect_host_off_exit",true,json!({"exit":0}));
  self.report["acceptance_stage"]=json!("automatic_checks_complete_visual_review_pending");Ok(())
 }
}
