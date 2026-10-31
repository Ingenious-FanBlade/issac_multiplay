//! tx-04-18原版资源实景E2E：真实Authority采集原图/生成surface/最终几何，真实Client消费，不造模型或贴图。
use crate::{Control,Result,Run,win32};
use serde_json::{Value,json};
use std::{collections::{BTreeMap,BTreeSet},fs,net::UdpSocket,path::Path,thread,time::{Duration,Instant}};
type Status=BTreeMap<String,String>;
fn n(s:&Status,k:&str)->u64{s.get(k).and_then(|s|s.parse().ok()).unwrap_or(0)}
const SCENES:[(&str,&str);11]=[("natural","prepare"),("types","types"),("states","states"),("destroy","destroy"),("replace-old","replace-old"),("replace-new","replace-new"),("large","shape-large"),("l-room","shape-l"),("boss","boss"),("cellar","cellar"),("water","flooded")];
// 比较真实原生GPU输出，只剔除本地FPS诊断的顶行；实际原版房间颜色/图案必须对应。
fn bitmap(path:&Path)->Result<(usize,usize,Vec<u8>)>{let b=fs::read(path).map_err(|e|e.to_string())?;
 if b.get(..2)!=Some(b"BM"){return Err("original BMP".into())}let get=|p|->Result<u32>{Ok(u32::from_le_bytes(b.get(p..p+4).ok_or("BMP header")?.try_into().unwrap()))};
 let w=get(18)? as usize;let height=get(22)? as i32;let h=height.unsigned_abs() as usize;let offset=get(10)? as usize;
 if w==0||h==0||w*h>8*1024*1024{return Err("BMP geometry".into())}let mut pixels=b.get(offset..offset+w*h*4).ok_or("BMP body")?.to_vec();
 if height>0{for y in 0..h/2{for x in 0..w*4{pixels.swap(y*w*4+x,(h-1-y)*w*4+x);}}}Ok((w,h,pixels))}
fn compare_images(authority:&Path,client:&Path)->Result<Value>{let (w,h,a)=bitmap(authority)?;let (cw,ch,b)=bitmap(client)?;
 if (w,h)!=(cw,ch){return Err(format!("Authority/Client target sizes {:?} vs {:?}",(w,h),(cw,ch)))}
 let mut relevant=0u64;let mut matched=0u64;let mut error=0u64;
 for y in 32..h.saturating_sub(4){for x in 4..w.saturating_sub(4){let at=(y*w+x)*4;let aa=&a[at..at+3];let bb=&b[at..at+3];
  if aa.iter().chain(bb).all(|&v|v<=25){continue;}relevant+=1;let d:Vec<_>=aa.iter().zip(bb).map(|(&a,&b)|a.abs_diff(b)).collect();error+=d.iter().map(|&v|u64::from(v)).sum::<u64>();if d.iter().all(|&v|v<=8){matched+=1;}
 }}
 if relevant<3000{return Err(format!("original room pixels absent: {relevant}"))}
 let ratio=matched as f64/relevant as f64;let detail=json!({"width":w,"height":h,"relevant_pixels":relevant,"matched_pixels":matched,"match_ratio":ratio,"mean_channel_error":error as f64/(relevant*3) as f64});
 if ratio<0.985{return Err(format!("original Authority/Client pixel mismatch: {detail}"))}Ok(detail)
}
impl Run {
 fn original_action(&mut self,c:&mut Control,name:&str)->Result<Value>{let result=c.command(&format!("cs original_test {} action-{name}",self.report["token"].as_str().ok_or("token")?))?;
  serde_json::from_str(result.get("original.fixture").ok_or("original fixture result")?).map_err(|e|e.to_string())}
 fn original_capture(&mut self,g:&win32::Game,c:&mut Control,name:&str)->Result<Value>{
  let before=c.command("cs status")?;let published=n(&before,"original.published");let rejected=n(&before,"original.rejected");
  c.command(&format!("cs original_test {} capture-{name}",self.report["token"].as_str().ok_or("token")?))?;
  let end=Instant::now()+Duration::from_secs(45);loop{if let Some(code)=g.exited(){return Err(format!("original capture {name} exited {code}"))}
   let status=c.command("cs status")?;if n(&status,"original.rejected")>rejected{return Err(format!("original capture {name}: {:?}",status.iter().filter(|(k,_)|k.starts_with("original.")).collect::<Vec<_>>()))}
   if n(&status,"original.published")>published{break;}if Instant::now()>=end{return Err(format!("original capture timeout {name}: {:?}",status.iter().filter(|(k,_)|k.starts_with("original.")).collect::<Vec<_>>()))}thread::sleep(Duration::from_millis(70));
  }
  let packet:Value=serde_json::from_str(&crate::read(&self.out.join(format!("runtime-world/original/{name}.json")))?).map_err(|e|e.to_string())?;
  if packet["format"]!="isaac.cs.room-grid-original"||packet["images"].as_array().is_none_or(|a|a.is_empty())||packet["draws"].as_array().is_none_or(|a|a.is_empty())||packet["unclassified_draws"]!=0{return Err(format!("incomplete original resource snapshot {name}"))}
  for image in packet["images"].as_array().unwrap(){if image["logical_path"].as_str().is_some_and(|s|s.contains("isaac-cs-tx17")||s.contains("isaac-cs-tx18")){return Err("fixture texture contaminated original E2E".into())}}
  Ok(packet)
 }
 pub(super) fn execute_tx18_original(&mut self)->Result<()>{
  self.report["original_resources"]=json!(true);
  self.report["reused_controlled_results"]=json!({"document":"docs/design/tx-04-18/tx-04-18-implementation.md","scope":"previous controlled-only 10/10; not original-resource evidence","unaffected":["field/save value retention","448-cell canonical geometry contract","identity replacement rules","unknown class/type rejection","partial membership rejection"]});
  if let Some(source)=self.scenario["resume_from"].as_str().map(std::path::PathBuf::from){
   let prior:Value=serde_json::from_str(&crate::read(&source.join("result.json"))?).map_err(|e|e.to_string())?;
   if prior["original_resources"]!=true{return Err("original continuation requires real-resource result".into())}
   let retest=self.scenario["retest_cases"].as_array().cloned().unwrap_or_default();for (name,v) in prior["cases"].as_object().ok_or("prior cases")?{if v["pass"]==true&&!retest.iter().any(|v|v==name){self.report["cases"][name]=v.clone();}}
   if source.join("source-scenes.json").exists()&&!retest.iter().any(|v|v=="source_captures"){let list:Value=serde_json::from_str(&crate::read(&source.join("source-scenes.json"))?).map_err(|e|e.to_string())?;
    let destination=self.out.join("runtime-world/original");fs::create_dir_all(&destination).map_err(|e|e.to_string())?;
    for entry in list.as_array().ok_or("original source list")?{let name=entry["name"].as_str().ok_or("source name")?;
     if !SCENES.iter().any(|(n,_)|*n==name){return Err("unsafe source scene name".into())}
     for file in [format!("{name}.json"),format!("{name}-authority.bmp")]{fs::copy(source.join("runtime-world/original").join(&file),destination.join(&file)).map_err(|e|e.to_string())?;}
    }crate::write_json(&self.out.join("source-scenes.json"),&list)?;
   }
   self.report["reused_unaffected"]=json!({"source":source,"cases":self.report["cases"],"retest":retest});
  }
  self.environment()?;self.report["crash_before"]=self.crash_files()?;self.snapshot("save_before")?;self.prepare_restore_tx08(1)?;self.disable_helper_tx08()?;self.deploy()?;
  let game=self.launch("enabled")?;self.report["pid"]=json!(game.pid());
  let result=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||self.original_scenes(&game))).unwrap_or_else(|_|Err("original scene panic".into()));
  if let Err(e)=&result{self.capture_failure(&game,"enabled",e);let _=game.close();}self.collect(game.pid(),"enabled")?;result
 }
 fn original_scenes(&mut self,g:&win32::Game)->Result<()>{
  let w=g.wait_window(90)?;let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("port")? as u16,g,90,&self.out.join("control.jsonl"))?;
  c.wait_startup(g,w,90)?;
  // 首次Client截图避开Steam启动提示自然消失前的GPU叠层，不改动Steam配置。
  self.watch_tx05(g,&mut c,18000)?;
  let index=self.out.join("source-scenes.json");let mut scenes:Vec<Value>=if index.exists(){serde_json::from_str(&crate::read(&index)?).map_err(|e|e.to_string())?}else{Vec::new()};
  if scenes.len()<SCENES.len(){
   // Authority菜单使用既有真实输入流程，随后所有场景动作走显式token控制口，Client阶段不抢焦点。
   win32::set_auto_focus(true);c.focus_window=Some(w);self.watch_tx05(g,&mut c,1500)?;self.prepare_new_run_tx05(g,w,&mut c)?;self.sequence(w,"start_run")?;c.wait("run_active","1",25)?;
   self.original_action(&mut c,"seed")?;self.watch_tx05(g,&mut c,1800)?;
   c.command("cs observe start")?;c.command(&format!("cs host {}",self.scenario["host_port"]))?;c.wait("session_state","Active",25)?;
   for (name,action) in SCENES{
    // 准备同一真实场景序列中的后续动作，但已经保存且不受影响的capture不重跑。
    let actual=self.original_action(&mut c,action)?;self.watch_tx05(g,&mut c,900)?;
    if ["large","l-room","boss","cellar","water"].contains(&name){self.original_action(&mut c,"prepare")?;self.watch_tx05(g,&mut c,600)?;}
    if name=="water"{self.original_action(&mut c,"water")?;self.watch_tx05(g,&mut c,600)?;}
    if scenes.iter().any(|s|s["name"]==name){continue;}
    let packet=self.original_capture(g,&mut c,name)?;let fact=self.original_action(&mut c,"facts")?;
    if name=="large"&&fact["shape"]!=8{return Err(format!("actual large room {fact}"))}if name=="l-room"&&fact["shape"]!=9{return Err(format!("actual L room {fact}"))}
    let entry=json!({"name":name,"source_pid":g.pid(),"fact":fact,"request":actual,"source_revision":packet["model"]["revision"],"source_scope":packet["model"]["scope"]});scenes.push(entry.clone());crate::write_json(&index,&json!(scenes))?;self.step(&format!("original_source_{name}"),entry)?;
   }
   c.command("cs stop")?;c.wait("session_state","Idle",20)?;c.command("cs observe stop")?;
   let deadline=Instant::now()+Duration::from_secs(20);loop{let state=c.command("cs observe status")?;if state.get("observation_state").is_some_and(|v|v=="Off"){break;}if Instant::now()>=deadline{return Err(format!("original observation retirement: {state:?}"))}thread::sleep(Duration::from_millis(80));}
   self.return_to_menu(w,&mut c)?;
  }
  c.focus_window=None;win32::set_auto_focus(false);c.wait("client_ui_available","1",20)?;self.snapshot("save-client-before")?;
  let blackhole=UdpSocket::bind("127.0.0.1:0").map_err(|e|e.to_string())?;let port=blackhole.local_addr().map_err(|e|e.to_string())?.port();
  let mut before=Status::new();let mut boot=0u64;let mut connection=0u64;let mut last_packet=Value::Null;
  let mut types=BTreeSet::new();let mut backdrops=BTreeSet::new();let mut source_images=0usize;
  for entry in &scenes{let name=entry["name"].as_str().ok_or("scene name")?;let case=format!("original_pixels_{name}");
   let mut packet:Value=serde_json::from_str(&crate::read(&self.out.join(format!("runtime-world/original/{name}.json")))?).map_err(|e|e.to_string())?;
   for object in packet["model"]["components"]["grid"]["state"]["objects"].as_array().ok_or("grid source objects")?{if let Some(t)=object["type"].as_u64(){types.insert(t);}}
   if let Some(t)=entry["fact"]["backdrop"].as_i64(){backdrops.insert(t);}source_images+=packet["images"].as_array().unwrap().len();
   if self.report["cases"][&case]["pass"]==true{last_packet=packet;continue;}
   let source_pid=entry["source_pid"].as_u64().ok_or("source pid")?;
   if source_pid!=boot{if connection!=0{self.original_isolation(&mut c,&before)?;c.command("cs stop")?;c.wait("role","Off",20)?;}c.command(&format!("cs client 127.0.0.1 {port} 0"))?;let s=c.wait("role","Client",10)?;connection=n(&s,"client.connection");boot=source_pid;before=c.command("cs counters")?;}
   packet["connection"]=json!(connection.to_string());packet["model"]["scope"]["connection"]=packet["connection"].clone();let revision=packet["model"]["revision"].as_str().ok_or("revision")?.parse::<u64>().map_err(|e|e.to_string())?;
   let rejected=n(&c.command("cs status")?,"presentation.rejected");self.submit_tx18(&mut c,&format!("original-{name}"),&packet)?;
   let deadline=Instant::now()+Duration::from_secs(30);let ready=loop{let s=c.command("cs status")?;
    if n(&s,"presentation.rejected")>rejected{return Err(format!("original Client {name} rejected: {:?}",s.get("presentation.error")))}
    if n(&s,"room_grid.revision")==revision&&n(&s,"original.draws")>0&&n(&s,"presentation.retiring")==0{self.step(&format!("original_client_{name}"),json!(s))?;break s;}
    if Instant::now()>=deadline{return Err(format!("original Client {name} timeout: {:?}",s.get("presentation.error")))}thread::sleep(Duration::from_millis(70));};
   self.capture_tx18(g,&mut c,&format!("original-{name}"))?;
   let pixels=compare_images(&self.out.join(format!("runtime-world/original/{name}-authority.bmp")),&self.out.join(format!("original-{name}.bmp")))?;
   self.original_readback(&mut c,&packet,name)?;
   self.case(&case,true,json!({"fact":entry["fact"],"resources":packet["images"].as_array().unwrap().len(),"draws":packet["draws"].as_array().unwrap().len(),"pixels":pixels,"status":ready}));last_packet=packet;
  }
  if !(1..=27).all(|t|types.contains(&t))||backdrops.len()<2||source_images<20{return Err(format!("original coverage types={types:?} backdrops={backdrops:?} images={source_images}"))}
  self.case("original_resource_and_type_coverage",true,json!({"types":types,"backdrops":backdrops,"image_declarations":source_images,"source":"actual original engine Image + final vertices; no fixture textures"}));
  // 新decoder与真实资源候选失败也必须保持当前完整场景，不依赖旧受控素材的失败用例。
  if connection==0{c.command(&format!("cs client 127.0.0.1 {port} 0"))?;connection=n(&c.wait("role","Client",10)?,"client.connection");before=c.command("cs counters")?;}
  if n(&c.command("cs status")?,"original.images")==0{last_packet["connection"]=json!(connection.to_string());last_packet["model"]["scope"]["connection"]=last_packet["connection"].clone();self.submit_tx18(&mut c,"original-close-context",&last_packet)?;self.wait_tx18(g,&mut c,"restore_original_context",|s|n(s,"original.images")>0)?;}
  if self.report["cases"]["original_decoder_failure_atomicity"]["pass"]!=true{
   let old=c.command("cs status")?;let mut bad=last_packet.clone();bad["connection"]=json!(connection.to_string());
   bad["images"][0]["data"]=json!("00");self.submit_tx18(&mut c,"original-invalid-image",&bad)?;let rejected=n(&old,"presentation.rejected");let failed=self.wait_tx18(g,&mut c,"original_corrupt_resource_rejected",|s|n(s,"presentation.rejected")>rejected)?;
   if n(&failed,"room_grid.revision")!=n(&old,"room_grid.revision")||n(&failed,"original.images")!=n(&old,"original.images"){return Err("original failed candidate partially published".into())}
   self.case("original_decoder_failure_atomicity",true,json!({"previous_revision":n(&old,"room_grid.revision"),"error":failed.get("presentation.error")}));
  }
  self.original_isolation(&mut c,&before)?;let after=c.command("cs counters")?;
  let status=c.command("cs status")?;if n(&status,"presentation.rng_changes")!=0||n(&status,"client_world_applied")!=0||n(&status,"presentation.progress_changed")!=0{return Err("original Client isolation/progress/RNG".into())}
  self.case("original_client_isolation_and_resource_lifetime",true,json!({"native_before":before,"native_after":after,"status":status}));
  self.report["final_status"]=json!(c.command("cs status")?);drop(c);if g.close()?!=0{return Err("original Client normal exit".into())}self.snapshot("save-process-exit")?;
  let changes=crate::saves::changes(&self.out.join("save-client-before"),&self.out.join("save-process-exit"))?;
  if changes.as_array().ok_or("save changes")?.iter().any(|r|{let p=r["path"].as_str().unwrap_or("").to_lowercase();r["bytes_changed"]==true&&(p.contains("persistentgamedata")||p.contains("gamestate")||p.contains("savesyncstatus"))}){return Err("original Client modified visitor progress".into())}
  self.case("original_normal_exit_and_visitor_persistence",true,json!({"changes":changes,"normal_exit":true}));Ok(())
 }
 // 每段真实Client窗口分别比较，重连之间原版菜单透传不混入Client隔离计数。
 fn original_isolation(&mut self,c:&mut Control,before:&Status)->Result<()>{let after=c.command("cs counters")?;
  for name in ["game_update","room_update","entity_list","odd_helper","player_update","start_game","restore","join_pass"]{let k=format!("hook.{name}");if n(before,&k)!=n(&after,&k){return Err(format!("Client original gameplay entered {name}"))}}Ok(())
 }
 // 读回真实来源的owned状态／保存描述／全部几何，不用另一份构造模型代替源事实。
 fn original_readback(&mut self,c:&mut Control,packet:&Value,name:&str)->Result<()>{
  let room=self.object_tx18(c,"room")?;let source=&packet["model"]["components"]["room"]["state"];let fields=&source["state"]["fields"];
  for key in ["state","config","override_config"]{if !crate::tx18::geometry_equal(&room[key],&source[key]){return Err(format!("original {name} Room owned readback {key}"))}}
  let cells=room["cells"].as_array().ok_or("original owned cells")?;if cells.len()!=448{return Err("original cells count".into())}
  for (index,cell) in cells.iter().enumerate(){
   let position=&fields["Room.GetGridPosition"]["values"][index]["value"];
   if cell["position"]!=json!([position["x"],position["y"]])&&!crate::tx18::geometry_equal(&cell["position"],&json!([position["x"],position["y"]])){return Err(format!("original cell position {index}"))}
   for (member,key) in [("Room.GetGridCollision","collision"),("Room.GetGridPath","path"),("Room.GetRail","rail")]{let raw=&fields[member]["values"][index]["value"];
    let expected=if raw["status"]=="Null"{255}else{raw.as_i64().or_else(||raw["value"].as_str().and_then(|v|v.parse::<i64>().ok())).ok_or("original geometry scalar")?};
    if cell[key].as_i64()!=Some(expected){return Err(format!("original cell {index} {key}"))}if raw["status"]=="Null"&&cell["rail_source"]!=*raw{return Err("original Null rail provenance".into())}
   }
  }
  let mut checked=0u64;if name=="types"{let mut seen=BTreeSet::new();for object in packet["model"]["components"]["grid"]["state"]["objects"].as_array().ok_or("original grid objects")?{
   let typ=object["type"].as_u64().ok_or("original Grid type")?;if !seen.insert(typ){continue;}let id=object["subject"]["target"]["id"].as_str().ok_or("original Grid id")?;let owned=self.object_tx18(c,id)?;
   for key in ["type","variant","index","class"]{if owned[key]!=object[key]{return Err(format!("original Grid {id} {key}"))}}
   if owned["key"]!=object["subject"]["target"]||!crate::tx18::geometry_equal(&owned["state"]["common"],&object["common"])||!crate::tx18::geometry_equal(&owned["state"]["derived"],&object["bodies"][object["class"].as_str().ok_or("original class")?])||!crate::tx18::geometry_equal(&owned["save"],&object["common"]["fields"]["GridEntity.GetSaveState"]){return Err(format!("original Grid {id} state/save values"))}checked+=1;
  }if seen.len()!=27{return Err(format!("original actual type readbacks {}",seen.len()))}}
  self.step(&format!("original_owned_readback_{name}"),json!({"cells":448,"Room_state_config":true,"actual_type_readbacks":checked}))?;Ok(())
 }
}
