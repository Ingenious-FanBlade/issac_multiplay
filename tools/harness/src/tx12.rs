//! tx-04-12完整目标的统一E2E；检查实际状态、分层身份、引用和交付，不以夹具请求充当结果。
use crate::{Control,Result,Run,win32};
use serde_json::{Value,json};
use std::{collections::{BTreeMap,BTreeSet},fs,io::{BufRead,BufReader},thread,time::{Duration,Instant}};
type Status=BTreeMap<String,String>;
fn number(s:&Status,k:&str)->u64{s.get(k).and_then(|v|v.parse().ok()).unwrap_or(0)}
fn n(v:&Value)->Option<f64>{v.as_f64().or_else(||v.as_str().and_then(|s|s.parse().ok())).or_else(||v["value"].as_str().and_then(|s|s.parse().ok()))}
impl Run {
 fn wait_tx12(&mut self,g:&win32::Game,c:&mut Control,label:&str,cmd:&str,p:impl Fn(&Status)->bool)->Result<Status>{
  let until=Instant::now()+Duration::from_secs(45);
  loop{self.watch_tx05(g,c,0)?;if let Some(e)=self.live_fixture()?.iter().find(|r|r["event"]=="fixture_error"){return Err(format!("fixture:{e}"));}
   let s=c.command(cmd)?;if p(&s){self.step(label,json!(s))?;return Ok(s)}
   if Instant::now()>=until{return Err(format!("{label} timeout {s:?}"));}thread::sleep(Duration::from_millis(60));}
 }
 fn action_tx12(&mut self,g:&win32::Game,c:&mut Control,w:usize,index:u64,name:&str)->Result<()>{
  win32::key(w,"F8")?;let until=Instant::now()+Duration::from_secs(30);
  loop{self.watch_tx05(g,c,80)?;let rows=self.live_fixture()?;
   if let Some(e)=rows.iter().find(|v|v["event"]=="fixture_error"){return Err(format!("fixture:{e}"));}
   if rows.iter().any(|v|v["event"]=="action_return"&&v["action"]==index&&v["name"]==name){return Ok(())}
   if Instant::now()>=until{return Err(format!("action {index} {name} timeout"));}}
 }
 fn scopes_tx12(&mut self,g:&win32::Game,c:&mut Control)->Result<Status>{
  let b=c.command("cs observe status")?;
  let s=self.wait_tx12(g,c,"room_level_progress","cs observe status",|s|number(s,"lua_attempts")>=number(&b,"lua_attempts")+2&&number(s,"room_scope")>number(&b,"room_scope")&&number(s,"level_scope")>number(&b,"level_scope"))?;
  if s.get("room_coverage").map(String::as_str)!=Some("Complete")||s.get("level_coverage").map(String::as_str)!=Some("Complete"){return Err(format!("room/level partial:{s:?}"));}Ok(s)
 }
 pub(super) fn scenes_tx12(&mut self,g:&win32::Game)->Result<()>{
  let w=g.wait_window(90)?;let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("port")? as u16,g,90,&self.out.join("control.jsonl"))?;c.focus_window=Some(w);
  self.prepare_new_run_tx05(g,w,&mut c)?;self.sequence(w,"start_run")?;c.wait("run_active","1",25)?;
  self.action_tx12(g,&mut c,w,1,"seed")?;self.watch_tx05(g,&mut c,1500)?;
  c.command("cs world start room-level-full")?;self.wait_tx12(g,&mut c,"recorder","cs world status",|s|s.get("world_state").is_some_and(|v|v=="Recording"))?;
  c.command("cs observe start")?;c.command("cs solo start")?;self.wait_tx12(g,&mut c,"session","cs status",|s|s.get("session_state").is_some_and(|v|v=="Active"))?;
  for (i,name) in ["initial","progress_dirty","progress_clear","environment","queries","leave","revisit","shape2","shapeL","sacrifice","shop","challenge","downpour","mirror","normal","mines","death_certificate","death_return","ascent"].iter().enumerate(){
   self.action_tx12(g,&mut c,w,i as u64+2,name)?;self.watch_tx05(g,&mut c,1100)?;self.scopes_tx12(g,&mut c)?;
   self.step(name,json!({"actual":self.live_fixture()?.last()}))?;
  }
  win32::key(w,"ESC")?;c.wait_pause(None)?;let b=c.command("cs observe status")?;self.watch_tx05(g,&mut c,350)?;let a=c.command("cs observe status")?;
  if b["room_scope"]!=a["room_scope"]||b["level_scope"]!=a["level_scope"]{return Err("ordinary room/level advanced during pause".into());}win32::key(w,"ESC")?;self.scopes_tx12(g,&mut c)?;
  c.command("cs world stop")?;self.wait_tx12(g,&mut c,"recorder_closed","cs world status",|s|s.get("world_state").is_some_and(|v|v=="Closed"))?;
  let file=self.out.join("runtime-world/room-level-full/observation-v2/records.jsonl");let length=fs::metadata(&file).map_err(|e|e.to_string())?.len();let before=c.command("cs observe status")?;let after=self.scopes_tx12(g,&mut c)?;
  if before["observation_epoch"]!=after["observation_epoch"]||fs::metadata(&file).map_err(|e|e.to_string())?.len()!=length{return Err("recording separation failed".into());}
  self.case("room_level_recording_separation_and_pause",true,json!({"before":before,"after":after}));self.reduce_tx12()?;
  c.command("cs stop")?;self.wait_tx12(g,&mut c,"input_retired","cs status",|s|s.get("session_state").is_some_and(|v|v=="Idle"))?;
  c.command("cs observe stop")?;self.wait_tx12(g,&mut c,"observation_retired","cs observe status",|s|s.get("observation_state").is_some_and(|v|v=="Off"))?;self.return_to_menu(w,&mut c)?;
  let status=c.command("cs status")?;for k in ["trace_dropped","query_mismatches","cleanup_failed"]{if number(&status,k)!=0{return Err(format!("final {k}"));}}self.report["final_status"]=json!(status);drop(c);if g.close()?!=0{return Err("game exit".into());}Ok(())
 }
 fn reduce_tx12(&mut self)->Result<()>{
  let root=self.out.join("runtime-world/room-level-full/observation-v2");let summary=crate::observation::read_capture(&root)?;
  if summary["delivery_complete"]!=true||summary["supported_read_complete"]!=true{return Err(format!("reader/delivery:{summary}"));}
  let mut facts=BTreeSet::<String>::new();let mut shapes=BTreeSet::<u64>::new();let mut types=BTreeSet::<u64>::new();let mut dimensions=BTreeSet::<i64>::new();let mut floors=BTreeSet::<String>::new();let mut room_instances=BTreeSet::<String>::new();let mut counts=BTreeMap::<String,u64>::new();let mut descriptors=BTreeMap::<String,(String,String)>::new();let mut revisit=BTreeMap::<String,BTreeSet<String>>::new();let mut ordinary=BTreeMap::<String,Value>::new();let mut max_nodes=0u64;
  for line in BufReader::new(fs::File::open(root.join("records.jsonl")).map_err(|e|e.to_string())?).lines(){let text=line.map_err(|e|e.to_string())?;let r:Value=serde_json::from_str(&text).map_err(|e|e.to_string())?;
   let schema=r["schema"].as_str().unwrap_or("");if !schema.starts_with("isaac.cs.room")&&!schema.starts_with("isaac.cs.level"){continue;}
   *counts.entry(schema.into()).or_default()+=1;
   let raw=text.split_once(",\"payload\":").ok_or("payload encoding")?.1;
   if r["coverage"]["bytes"].as_u64()!=Some(raw.len().saturating_sub(1) as u64){return Err("room/level encoded byte accounting mismatch".into());}
   if r["coverage"]["status"]!="Complete"{return Err(format!("incomplete room/level scope:{r}"));}
   let p=&r["payload"];let ctx=&r["context"];
   if schema=="isaac.cs.level.transition"{if !ctx["authority_tick"].is_null()||ctx["room_descriptor"]["presence"]!="Unknown"{return Err("transition borrowed ordinary context".into());}facts.insert("transition".into());continue;}
   if schema=="isaac.cs.room.query"||schema=="isaac.cs.level.query"{facts.insert(schema.into());continue;}
   if schema=="isaac.cs.room.event"&&p["event"]=="MC_POST_ROOM_TRIGGER_CLEAR"{facts.insert("actual_clear_event".into());}
   if schema!="isaac.cs.room"&&schema!="isaac.cs.level"{continue;}
   for key in ["run","floor","dimension","room_descriptor","room_instance"]{if ctx[key]["presence"]!="Known"||ctx[key]["local_id"]=="0"{return Err(format!("missing stable context:{key} {ctx}"));}}
   max_nodes=max_nodes.max(r["coverage"]["nodes"].as_u64().unwrap_or(0));
   floors.insert(ctx["floor"]["local_id"].as_str().unwrap().into());room_instances.insert(ctx["room_instance"]["local_id"].as_str().unwrap().into());
   if p["sampling"]["cache_reused"]!=false||p["sample_revision"]!=ctx["scope"]{return Err("freshness/revision mismatch".into());}
   let capture=ctx["authority_tick"].as_str().unwrap().to_string();
   if let Some(old)=ordinary.insert(capture,ctx.clone()){for k in ["run","floor","dimension","room_descriptor","room_instance"]{if old[k]!=ctx[k]{return Err("mixed-domain generation".into());}}}
   if schema=="isaac.cs.room"{
    let f=&p["state"]["fields"];if n(&f["Room.GetRoomShape"]).is_some(){shapes.insert(n(&f["Room.GetRoomShape"]).unwrap() as u64);}if let Some(t)=n(&f["Room.GetType"]){types.insert(t as u64);}
    let room=p["location"]["room_instance"].as_str().unwrap_or("");if ctx["room_instance"]["local_id"]!=room||ctx["floor"]["local_id"]!=p["location"]["floor"]||ctx["room_descriptor"]["local_id"]!=p["location"]["descriptor"]{return Err("payload/context identity mismatch".into());}
    let key=format!("{}:{}:{}",ctx["floor"]["local_id"],ctx["dimension"]["local_id"],ctx["room_descriptor"]["local_id"]);revisit.entry(key).or_default().insert(room.into());
    if f["Room.IsClear"]==false{facts.insert("unclear".into());}if f["Room.IsClear"]==true{facts.insert("clear".into());}if f["Room.IsSacrificeDone"]==true{facts.insert("sacrifice_progress".into());}
    if n(&f["Room.GetWaterAmount"])==Some(0.75)&&n(&f["Room.GetWaterCurrent"]["x"])==Some(1.25){facts.insert("water_current".into());}
    if n(&f["Room.GetFXParams"]["fields"]["FXParams.ShadowAlpha"])==Some(0.375){facts.insert("fx_light_shadow".into());}
    if p["config"]["state"]["fields"]["RoomConfigRoom.Spawns"]["kind"]=="array"{facts.insert("config_spawns".into());}
    if let Some(doors)=p["doors"].as_array(){for d in doors{if d["actual"]["resolution"]=="KnownCurrent"{facts.insert("actual_grid_door_reference".into());}if d["shape_allowed"]==false&&d["actual"]["resolution"]=="Null"{facts.insert("shape_vs_actual".into());}}}
   }else{
    if p["membership"]["complete"]!=true{return Err("level membership incomplete".into());}
    if p["state"]["fields"]["Level.IsAscent"]==true{facts.insert("ascent".into());}
    let entries=p["descriptors"].as_array().ok_or("descriptors")?;if entries.len()!=p["membership"]["size"].as_u64().ok_or("size")? as usize{return Err("descriptor membership count".into());}
    for d in entries{let id=&d["subject"];let dim=id["dimension_index"].as_i64().ok_or("descriptor dimension")?;dimensions.insert(dim);let key=id["local_id"].as_str().ok_or("descriptor id")?.to_string();let owner=(ctx["floor"]["local_id"].to_string(),id["dimension_index"].to_string());if let Some(previous)=descriptors.insert(key,owner.clone()){if previous!=owner{return Err("descriptor reused across floor/dimension".into());}}}
    if p["connections"].as_array().is_some_and(|a|a.iter().any(|c|c["neighbor"]["resolution"]=="KnownCurrent")){facts.insert("adjacency".into());}
    let f=&p["current_descriptor"]["fields"];if f["RoomDescriptor.NoReward"]==true&&n(&f["RoomDescriptor.ShopItemIdx"])==Some(7.0){facts.insert("descriptor_progress".into());}
   }
  }
  if ![1,8,9].iter().all(|s|shapes.contains(s))||![1,2,11,13].iter().all(|s|types.contains(s)){return Err(format!("actual shape/type coverage:{shapes:?} {types:?}"));}
  if floors.len()<4||!dimensions.contains(&0)||!dimensions.contains(&1)||!dimensions.contains(&2)||!revisit.values().any(|v|v.len()>1){return Err(format!("hierarchy/transitions:{floors:?} {dimensions:?} revisit={revisit:?}"));}
  for expected in ["transition","isaac.cs.room.query","isaac.cs.level.query","unclear","clear","sacrifice_progress","water_current","fx_light_shadow","config_spawns","actual_grid_door_reference","shape_vs_actual","descriptor_progress","adjacency","actual_clear_event","ascent"]{if !facts.contains(expected){return Err(format!("missing actual observation:{expected}; facts={facts:?}"));}}
  self.case("room_identity_config_shape_and_membership",true,json!({"shapes":shapes,"types":types,"room_instances":room_instances.len(),"descriptor_identities":descriptors.len()}));
  self.case("room_progress_environment_events_and_queries",true,json!({"facts":facts}));
  self.case("level_floor_dimension_structure_and_transitions",true,json!({"floors":floors,"dimensions":dimensions,"max_nodes":max_nodes,"schemas":counts}));
  self.case("reader_delivery_and_same_window_context",true,summary.clone());self.report["observation_read"]=summary;
  println!("room_level_acceptance {}",self.report["cases"]);Ok(())
 }
}
