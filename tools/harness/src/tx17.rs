//! tx-04-17统一真实进程E2E：产品Client/资源/原生renderer/关闭链，无离线替身。
use crate::{Control,Result,Run,win32};
use serde_json::{Value,json};
use std::{collections::BTreeMap,fs,net::UdpSocket,thread,time::{Duration,Instant}};
type Status=BTreeMap<String,String>;
fn n(s:&Status,k:&str)->u64{s.get(k).and_then(|s|s.parse().ok()).unwrap_or(0)}
// 受控场景包含Room平面、Camera、多pass世界节点与HUD；不是主菜单单Sprite。
fn scene(connection:u64,revision:u64,orange:bool)->Value {
 let color=json!([1,1,1,1,0,0,0,0,0,0,0]);
 let layers:Vec<_>=(0..2).map(|id|json!({"id":id,"frame":3-id,"overlay_frame":id,"visible":true,
   "offset":[0,0],"scale":[1,1],"rotation":0,"color":color,"flip_x":false,"flip_y":false,"crop":[0,0],"flags":0,
   "blend":[0,1,7,1,7],"custom_shader":null,"champion_shader":null})).collect();
 let pose=json!({"animation":"Main","frame":3,"overlay":{"animation":"Overlay","frame":1},"overlay_first":true,
   "offset":[0,0],"scale":[1,1],"rotation":0,"color":color,"champion_color":color,"flip_x":false,"flip_y":false,
   "flags":0,"layers":layers,"custom_shader":null,"champion_shader":null});
 let nodes:Vec<_>=(0..3).map(|id|{let mut p=pose.clone();if id==0{p["scale"]=json!([8,5]);}
   json!({"id":(id+1).to_string(),"pass":id*2,"position":[140+id*90,120+id*30],"screen":id==2,
     "anm2":"gfx/isaac-cs-tx17/scene.anm2","sheets":if orange{json!([{"id":0,"path":"gfx/isaac-cs-tx17/b.png"},{"id":1,"path":"gfx/isaac-cs-tx17/b.png"}])}else{json!([])},"pose":p})}).collect();
 json!({"connection":connection.to_string(),"epoch":"1","revision":revision.to_string(),"room":{"size":[520,280]},
   "camera":{"origin":[20,10],"zoom":1},"hud":format!("Replica room / camera / HUD - revision {revision}"),"nodes":nodes})
}
// 独立读取实机BMP像素；不凭draw计数宣称图像可见，不做摘要校验。
fn pixels(path:&std::path::Path)->Result<(u64,u64)> {
 let b=fs::read(path).map_err(|e|e.to_string())?;
 let u=|p:usize|->Result<u32>{Ok(u32::from_le_bytes(b.get(p..p+4).ok_or("short BMP")?.try_into().unwrap()))};
 if b.get(..2)!=Some(b"BM"){return Err("BMP format".into())}let offset=u(10)? as usize;
 let bits=u16::from_le_bytes(b.get(28..30).ok_or("BMP bits")?.try_into().unwrap());
 if bits!=32{return Err(format!("BMP pixel format {bits}"))}let mut teal=0;let mut orange=0;
 for p in b.get(offset..).ok_or("BMP offset")?.chunks_exact(4){
   let (r,g,blue)=(u32::from(p[2]),u32::from(p[1]),u32::from(p[0]));
   if blue>r*2&&g>r*2&&blue>60{teal+=1;}if r>blue*2&&r>g&&r>80{orange+=1;}
 }Ok((teal,orange))
}
fn background_clear(path:&std::path::Path)->Result<bool>{
 let b=fs::read(path).map_err(|e|e.to_string())?;
 let u=|p:usize|->Result<u32>{Ok(u32::from_le_bytes(b.get(p..p+4).ok_or("BMP header")?.try_into().unwrap()))};
 let width=u(18)? as usize;let height=u(22)? as i32;let offset=u(10)? as usize;
 if width<10||height.unsigned_abs()<12{return Err("BMP geometry".into())}
 let row=if height<0{10}else{height as usize-11};let at=offset+(row*width+width-5)*4;
 let p=b.get(at..at+3).ok_or("BMP background point")?;Ok(p.iter().all(|&c|c<12))
}
impl Run {
 fn wait_tx17(&mut self,g:&win32::Game,c:&mut Control,label:&str,p:impl Fn(&Status)->bool)->Result<Status>{
  let end=Instant::now()+Duration::from_secs(25);
  loop{if let Some(code)=g.exited(){return Err(format!("{label}: exited {code}"))}let s=c.command("cs status")?;
   if p(&s){self.step(label,json!(s))?;return Ok(s)}if Instant::now()>=end{return Err(format!("{label}: {s:?}"))}thread::sleep(Duration::from_millis(70));}
 }
 fn submit_tx17(&mut self,c:&mut Control,name:&str,value:&Value)->Result<()> {
  let root=self.out.join("runtime-world/presentation");fs::create_dir_all(&root).map_err(|e|e.to_string())?;
  crate::write_json(&root.join(format!("{name}.json")),value)?;
  let token=self.report["token"].as_str().ok_or("token")?;
  c.command(&format!("cs presentation_test {token} {name}"))?;Ok(())
 }
 pub(super) fn execute_tx17(&mut self)->Result<()> {
  // 沿用用户不抢焦点的偏好；本场景用控制口与真实渲染，不需要手动按键。
  win32::set_auto_focus(false);
  if let Some(source)=self.scenario["resume_from"].as_str().map(std::path::PathBuf::from){
   let prior:Value=serde_json::from_str(&crate::read(&source.join("result.json"))?).map_err(|e|e.to_string())?;
   if prior["tx"]!="tx-04-17"||prior["cases"]["scene_roots_camera_hud_native_pixels"]["pass"]!=true{return Err("tx17 continuation source lacks passed scene case".into())}
   // 新selector/背景项已有通过事实时复用；本次仅修改Mod持久入口与退出相关路径。
   let mut reused_cases=Vec::new();
   if prior["cases"]["fractional_phase_and_hidden_track"]["pass"]==true {
    for name in ["scene_roots_camera_hud_native_pixels","atomic_failures_and_scope_rejection","thread_and_os_context_owner",
      "replacement_pose_and_inflight_retirement","scene_background_isolation","fractional_phase_and_hidden_track",
      "all_update_and_gameplay_lua_isolated","safe_stop_and_host_off_passthrough"]{
     if prior["cases"][name]["pass"]==true{self.report["cases"][name]=prior["cases"][name].clone();reused_cases.push(name);}
    }
   }
   let mut bad_names=Vec::new();for line in crate::read(&source.join("steps.jsonl"))?.lines(){let row:Value=serde_json::from_str(line).map_err(|e|e.to_string())?;
    if let Some(name)=row["step"].as_str(){if ["old-connection","old-revision","missing-anm","missing-image","missing-animation","bad-frame"].contains(&name){
     let s:Status=serde_json::from_value(row["detail"].clone()).map_err(|e|e.to_string())?;
     if n(&s,"presentation.revision")!=1||n(&s,"presentation.published")!=1||s.get("role").map(String::as_str)!=Some("Client"){return Err("invalid passed rejection continuation".into())}
     bad_names.push(name.to_string());
    }}
   }
   if prior["cases"]["atomic_failures_and_scope_rejection"]["pass"]==true{
    bad_names=["old-connection","old-revision","missing-anm","missing-image","missing-animation","bad-frame","bad-layer","bad-shader"].iter().map(|s|s.to_string()).collect();
   }
   self.report["reused_unaffected"]=json!({"source":source,"cases":reused_cases,"candidate_names":bad_names,"affected_retest":["mod_persistence_native_and_saved_alias","visitor_persistence_and_normal_process_exit"]});
  }
  self.environment()?;self.report["crash_before"]=self.crash_files()?;self.snapshot("save_before")?;
  self.prepare_restore_tx08(1)?;self.disable_helper_tx08()?;self.deploy()?;
  let game=self.launch("enabled")?;self.report["pid"]=json!(game.pid());
  let result=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||self.scenes_tx17(&game))).unwrap_or_else(|_|Err("tx17 scenario panic".into()));
  if let Err(e)=&result{self.capture_failure(&game,"enabled",e);let _=game.close();}
  self.collect(game.pid(),"enabled")?;result
 }
 fn scenes_tx17(&mut self,g:&win32::Game)->Result<()> {
  let w=g.wait_window(90)?;let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("port")? as u16,g,90,&self.out.join("control.jsonl"))?;
  c.wait_startup(g,w,90)?;c.focus_window=None;c.wait("client_ui_available","1",20)?;
  // 官方启动读档/版本同步已完成后再冻结访客窗口基线，不把启动来源误判为服务器写入。
  self.snapshot("save-client-before")?;
  // UDP黑洞只触发真实Client等待/失败，不伪装Authority或声称完成世界网络下行。
  let blackhole=UdpSocket::bind("127.0.0.1:0").map_err(|e|e.to_string())?;let port=blackhole.local_addr().map_err(|e|e.to_string())?.port();
  c.command(&format!("cs client 127.0.0.1 {port} 0"))?;c.wait("role","Client",10)?;
  let before=c.command("cs counters")?;let initial=c.command("cs status")?;let connection=n(&initial,"client.connection");
  let token=self.report["token"].as_str().ok_or("token")?.to_string();
  let callbacks=c.command(&format!("cs presentation_test {token} callbacks"))?;
  let first=scene(connection,1,false);self.submit_tx17(&mut c,"first",&first)?;
  let ready=self.wait_tx17(g,&mut c,"scene_published",|s|n(s,"presentation.revision")==1&&n(s,"presentation.draws")>=5)?;
  let first_pixels=if self.report["cases"]["scene_roots_camera_hud_native_pixels"]["pass"]==true{
   let detail=&self.report["cases"]["scene_roots_camera_hud_native_pixels"]["detail"];
   (detail["teal_pixels"].as_u64().ok_or("reused teal evidence")?,detail["orange_pixels"].as_u64().ok_or("reused orange evidence")?)
  }else{
   if n(&ready,"presentation.nodes")!=3||n(&ready,"run_active")!=0||n(&ready,"presentation.progress_changed")!=0||n(&ready,"client_world_applied")!=0{return Err(format!("scene boundary {ready:?}"))}
   if n(&ready,"presentation.main_frame")!=3||n(&ready,"presentation.overlay_frame")!=1||n(&ready,"presentation.layer0_key")!=1||n(&ready,"presentation.overlay_layer0_key")!=0{return Err(format!("main/overlay/layer pose {ready:?}"))}
   win32::screenshot(w,&self.out.join("scene-first.bmp"))?;let p=pixels(&self.out.join("scene-first.bmp"))?;
   if p.0<1000{return Err(format!("native scene pixels missing {p:?}"))}
   self.case("scene_roots_camera_hud_native_pixels",true,json!({"status":ready,"teal_pixels":p.0,"orange_pixels":p.1}));p
  };
  // 同描述重发幂等，不重新Load或发布资源。
  let published=n(&ready,"presentation.published");
  if self.report["cases"]["atomic_failures_and_scope_rejection"]["pass"]!=true{
   self.submit_tx17(&mut c,"same",&first)?;thread::sleep(Duration::from_millis(180));
   if n(&c.command("cs status")?,"presentation.published")!=published{return Err("duplicate scene republished".into())}
  }
  let mut bads=Vec::new();
  let mut v=scene(connection,2,false);v["connection"]=json!((connection+1).to_string());bads.push(("old-connection",v));
  let mut v=scene(connection,1,true);bads.push(("old-revision",v.clone()));
  v["revision"]=json!("2");v["nodes"][1]["anm2"]=json!("gfx/isaac-cs-tx17/missing.anm2");bads.push(("missing-anm",v));
  let mut v=scene(connection,2,false);v["nodes"][1]["sheets"]=json!([{"id":0,"path":"gfx/isaac-cs-tx17/missing.png"}]);bads.push(("missing-image",v));
  let mut v=scene(connection,2,false);v["nodes"][1]["pose"]["animation"]=json!("Missing");bads.push(("missing-animation",v));
  let mut v=scene(connection,2,false);v["nodes"][1]["pose"]["frame"]=json!(4);bads.push(("bad-frame",v));
  let mut v=scene(connection,2,false);v["nodes"][1]["pose"]["layers"][0]["id"]=json!(63);bads.push(("bad-layer",v));
  let mut v=scene(connection,2,false);v["nodes"][1]["pose"]["custom_shader"]=json!("gfx/isaac-cs-tx17/missing.shader");bads.push(("bad-shader",v));
  let mut rejected=n(&c.command("cs status")?,"presentation.rejected");
  for (name,value) in bads{
   if self.report["reused_unaffected"]["candidate_names"].as_array().is_some_and(|a|a.iter().any(|v|v==name)){continue;}
   self.submit_tx17(&mut c,name,&value)?;
   let failed=self.wait_tx17(g,&mut c,name,|s|n(s,"presentation.rejected")>rejected)?;rejected=n(&failed,"presentation.rejected");
   if n(&failed,"presentation.revision")!=1||n(&failed,"presentation.published")!=published||failed.get("role").map(String::as_str)!=Some("Client"){return Err(format!("failed candidate published {name}"))}
  }
  if self.report["cases"]["atomic_failures_and_scope_rejection"]["pass"]!=true{self.case("atomic_failures_and_scope_rejection",true,json!({"rejections":rejected,"reused_unaffected":self.report["reused_unaffected"]["candidate_names"],"old_revision":1,"duplicate_idempotent":true}));}
  if self.report["cases"]["thread_and_os_context_owner"]["pass"]!=true{
   let wrong=c.command(&format!("cs presentation_test {token} wrong-owner"))?;if n(&wrong,"wrong_owner_blocked")!=1{return Err("wrong owner reached graphics".into())}
   self.case("thread_and_os_context_owner",true,json!({"wrong_owner":wrong,"actual":c.command("cs status")?}));
  }
  self.submit_tx17(&mut c,"replacement",&scene(connection,2,true))?;
  let changed=self.wait_tx17(g,&mut c,"replacement_published_and_retired",|s|n(s,"presentation.revision")==2&&n(s,"presentation.retired")>=1&&n(s,"presentation.retiring")==0)?;
  if self.report["cases"]["replacement_pose_and_inflight_retirement"]["pass"]!=true||self.report["cases"]["scene_background_isolation"]["pass"]!=true{
   win32::screenshot(w,&self.out.join("scene-replacement.bmp"))?;let changed_pixels=pixels(&self.out.join("scene-replacement.bmp"))?;
   if changed_pixels.1<1000||changed_pixels.1<=first_pixels.1{return Err(format!("replacement native pixels {first_pixels:?} -> {changed_pixels:?}"))}
   if !background_clear(&self.out.join("scene-replacement.bmp"))?{return Err("frozen menu pixels remained behind scene".into())}
   self.case("scene_background_isolation",true,json!({"native_target_cleared":true}));
   self.case("replacement_pose_and_inflight_retirement",true,json!({"status":changed,"orange_pixels":changed_pixels.1}));
  }
  if self.report["cases"]["fractional_phase_and_hidden_track"]["pass"]!=true{
  let mut phase=scene(connection,3,true);phase["nodes"][0]["pose"]["phase"]=json!(0.375);
  phase["nodes"][0]["pose"]["overlay"]["phase"]=json!(0.25);
  phase["nodes"][0]["pose"]["layers"][0]["frame"]=json!(2.5);
  phase["nodes"][0]["pose"]["layers"][0]["overlay_key"]=Value::Null;
  self.submit_tx17(&mut c,"phase-hidden",&phase)?;
  let posed=self.wait_tx17(g,&mut c,"phase_and_hidden_track",|s|n(s,"presentation.revision")==3&&n(s,"presentation.retiring")==0)?;
  if posed.get("presentation.main_frame").and_then(|s|s.parse::<f64>().ok())!=Some(3.375)||
     posed.get("presentation.overlay_frame").and_then(|s|s.parse::<f64>().ok())!=Some(1.25)||
     posed.get("presentation.overlay_layer0_key").map(String::as_str)!=Some("-1")||n(&posed,"presentation.layer0_key")!=1{return Err(format!("phase/hidden selector {posed:?}"))}
  self.case("fractional_phase_and_hidden_track",true,json!(posed));
  }
  let mod_persist=c.command(&format!("cs presentation_test {token} mod-persistence"))?;
  if n(&mod_persist,"client.mod_persist.0.blocked")<2||n(&mod_persist,"client.mod_persist.1.blocked")<2{return Err("ModReference/native alias persistence bypassed isolation".into())}
  for id in 2..=3{if n(&mod_persist,&format!("client.mod_persist.{id}.installed"))!=1||n(&mod_persist,&format!("client.mod_persist.{id}.blocked"))==0{return Err("ESSM persistence was not suppressed".into())}}
  self.case("mod_persistence_native_and_saved_alias",true,json!(mod_persist));
  let after=if self.report["cases"]["all_update_and_gameplay_lua_isolated"]["pass"]!=true{
   thread::sleep(Duration::from_secs(2));
   let after=c.command(&format!("cs presentation_test {token} callbacks"))?;if callbacks!=after{return Err(format!("Client dispatched generic Lua {callbacks:?} -> {after:?}"))}
   let counters=c.command("cs counters")?;
   for name in ["start_game","game_update","room_update","entity_list","odd_helper","restore","post_update","player_update","join_pass"]{let key=format!("hook.{name}");if n(&before,&key)!=n(&counters,&key){return Err(format!("Client original simulation {name}"))}}
   self.wait_tx17(g,&mut c,"failed_connection_remains_isolated",|s|s.get("client_state").map(String::as_str)==Some("Failed")&&n(s,"presentation.draws")>10)?;
   self.case("all_update_and_gameplay_lua_isolated",true,json!({"callbacks":after,"native_before":before,"native_after":counters}));after
  }else{callbacks};
  c.command("cs stop")?;let off=self.wait_tx17(g,&mut c,"resources_drained_before_off",|s|s.get("role").map(String::as_str)==Some("Off")&&n(s,"presentation.drained")==1)?;
  if self.report["cases"]["safe_stop_and_host_off_passthrough"]["pass"]!=true{
  if n(&off,"presentation.progress_changed")!=0||n(&off,"presentation.active")!=0{return Err("unsafe Off".into())}
  thread::sleep(Duration::from_millis(400));let resumed=c.command(&format!("cs presentation_test {token} callbacks"))?;
  if n(&resumed,"MC_PRE_RENDER")<=n(&after,"MC_PRE_RENDER"){return Err("Off original render callbacks not resumed".into())}
  c.command(&format!("cs host {}",self.scenario["host_port"]))?;c.wait("role","Host",10)?;c.command("cs stop")?;c.wait("role","Off",10)?;
  self.case("safe_stop_and_host_off_passthrough",true,json!({"off":off,"native_callbacks_resumed":resumed}));
  }
  // 再连接实际资源后直接WM_CLOSE，覆盖main关闭判断与Shutdown/Manager析构保护。
  c.command(&format!("cs client 127.0.0.1 {port} 0"))?;let second=c.wait("role","Client",10)?;
  self.submit_tx17(&mut c,"close-scene",&scene(n(&second,"client.connection"),1,false))?;
  self.wait_tx17(g,&mut c,"close_scene_ready",|s|n(s,"presentation.revision")==1&&n(s,"presentation.nodes")==3)?;
  self.report["final_status"]=json!(c.command("cs status")?);drop(c);
  if g.close()?!=0{return Err("Client normal process exit".into())}
  let trace=crate::read(&self.trace_file(g.pid()))?;
  for entry in ["shutdown_counters","shutdown_flush","manager_sync"]{
   let witnessed=trace.lines().filter_map(|line|serde_json::from_str::<Value>(line).ok()).filter(|row|
    row["ev"]=="client_isolation"&&row["detail"].as_str().is_some_and(|s|s.contains(&format!("entry={entry} original=0")))).count();
   if witnessed!=1{return Err(format!("normal exit protection witness {entry}: {witnessed}"))}
  }
  for source in 0..4{
   let count=trace.lines().filter_map(|line|serde_json::from_str::<Value>(line).ok()).filter(|row|
    row["ev"]=="client_isolation"&&row["detail"].as_str().is_some_and(|s|s==format!("entry=mod_persist source={source} original=0"))).count();
   let required=if source<2{4}else{2};if count<required{return Err(format!("Mod persistence during shutdown source {source}: {count}"))}
  }
  if !crate::read(&self.save.join("log.txt"))?.contains("[tx17-exit-persist] guarded"){return Err("shutdown Mod callback not witnessed".into())}
  self.snapshot("save-process-exit")?;let changes=crate::saves::changes(&self.out.join("save-client-before"),&self.out.join("save-process-exit"))?;
  // 启动期间的官方读档归属与Client窗口分开；Server没有任何持久文件写入权限。
  let unexpected:Vec<_>=changes.as_array().ok_or("save changes")?.iter().filter(|r|{
   let path=r["path"].as_str().unwrap_or("").to_ascii_lowercase();r["bytes_changed"]==true&&(path.contains("persistentgamedata")||path.contains("gamestate")||path.contains("savesyncstatus"))
  }).cloned().collect();
  if !unexpected.is_empty(){return Err(format!("visitor persistent progress changed: {unexpected:?}"))}
  self.case("visitor_persistence_and_normal_process_exit",true,json!({"changes":changes,"normal_exit":true}));
  Ok(())
 }
}
