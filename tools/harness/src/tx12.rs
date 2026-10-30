//! tx-04-12完整目标的统一E2E；检查实际状态、分层身份、引用和交付，不以夹具请求充当结果。
use crate::{Control,Result,Run,win32};
use serde_json::{Value,json};
use std::{collections::{BTreeMap,BTreeSet},fs,io::{BufRead,BufReader},path::PathBuf,thread,time::{Duration,Instant}};
type Status=BTreeMap<String,String>;
fn number(s:&Status,k:&str)->u64{s.get(k).and_then(|v|v.parse().ok()).unwrap_or(0)}
fn n(v:&Value)->Option<f64>{v.as_f64().or_else(||v.as_str().and_then(|s|s.parse().ok())).or_else(||v["value"].as_str().and_then(|s|s.parse().ok()))}
impl Run {
 // 同一tx续测按实际已完成角色选择，依赖旧正式录制而非布尔通过标记；不校验摘要。
 pub(super) fn prepare_resume_tx12(&mut self)->Result<()> {
  let Some(paths)=self.scenario["resume_from"].as_array() else{return Ok(())};
  fn visit(path:PathBuf,seen:&mut BTreeSet<PathBuf>,sources:&mut Vec<PathBuf>)->Result<()> {
   let path=path.canonicalize().map_err(|e|e.to_string())?;
   if !seen.insert(path.clone()){return Ok(())}
   let report:Value=serde_json::from_str(&crate::read(&path.join("result.json"))?).map_err(|e|e.to_string())?;
   if report["tx"]!="tx-04-12"||!report["evidence_error"].is_null()||report["cleanup_errors"].as_array().is_some_and(|a|a.iter().any(|e|!e.is_null())){return Err("resume source tx/environment did not recover".into());}
   if let Some(prior)=report["resume_sources"].as_array(){for p in prior {visit(PathBuf::from(p.as_str().ok_or("resume source path")?),seen,sources)?;}}
   if report["coverage_samples"].as_array().is_some_and(|a|!a.is_empty()) {
    let closed:Value=serde_json::from_str(&crate::read(&path.join("runtime-world/room-level-full/observation-v2/result.json"))?).map_err(|e|e.to_string())?;
    if closed["closed"]!=true||closed["complete_delivery"]!=true{return Err("resume source delivery incomplete".into());}
    sources.push(path);
   }
   Ok(())
  }
  let mut sources=Vec::new();let mut seen=BTreeSet::new();
  for path in paths {visit(PathBuf::from(path.as_str().ok_or("resume path")?),&mut seen,&mut sources)?;}
  let mut done=BTreeMap::<String,BTreeSet<String>>::new();
  for path in &sources {
   let report:Value=serde_json::from_str(&crate::read(&path.join("result.json"))?).map_err(|e|e.to_string())?;
   for sample in report["coverage_samples"].as_array().ok_or("resume samples")? {done.entry(sample["id"].as_str().ok_or("resume id")?.into()).or_default().insert(sample["role"].as_str().ok_or("resume role")?.into());}
  }
  let checklist:Value=serde_json::from_str(include_str!("../scenarios/tx-04-12-special-coverage.json")).map_err(|e|e.to_string())?;
  let retest=self.scenario["retest_ids"].as_array().cloned().unwrap_or_default();let mut pending=Vec::new();
  for spec in checklist["cases"].as_array().unwrap() {
   let id=spec["id"].as_str().unwrap();let roles=done.get(id);
   let complete=["case","return","reenter","return2"].iter().all(|role| {
    let required=*role=="case"||*role=="return"&&spec["return_required"]==true||["reenter","return2"].contains(role)&&spec["reenter_required"]==true;
    !required||roles.is_some_and(|r|r.contains(*role))
   });
   if !complete||retest.iter().any(|v|v==id){pending.push(id.to_string());}
  }
  self.report["resume_sources"]=json!(sources);self.scenario["tx12_pending_ids"]=json!(pending);
  crate::write_json(&self.out.join("scenario.json"),&self.scenario)?;
  self.step("resume_plan",json!({"reused_cases":checklist["cases"].as_array().unwrap().len()-pending.len(),"pending_ids":pending,"sources":sources}))
 }
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
 // 每项都等待实际游戏结果，再取得新的正式普通scope；请求日志不能作验收依据。
 fn coverage_action_tx12(&mut self,g:&win32::Game,c:&mut Control,w:usize,index:u64,id:&str,role:&str)->Result<()> {
  let command=format!("{role}:{id}"); self.action_tx12(g,c,w,index,&command)?;
  let until=Instant::now()+Duration::from_secs(60);
  let actual=loop {
   self.watch_tx05(g,c,80)?;
   let records=self.live_fixture()?;
   if let Some(e)=records.iter().find(|v|v["event"]=="fixture_error"){return Err(format!("coverage {command}: {e}"));}
   if let Some(r)=records.iter().find(|v|v["event"]=="coverage_ready"&&v["id"]==id&&v["role"]==role){break r["actual"].clone();}
   if Instant::now()>=until{return Err(format!("coverage actual result timeout:{command}; last={:?}",records.last()));}
  };
  let before=c.command("cs observe status")?;
  let status=self.wait_tx12(g,c,&command,"cs observe status",|s|number(s,"lua_attempts")>number(&before,"lua_attempts")&&number(s,"room_scope")>number(&before,"room_scope")&&number(s,"level_scope")>number(&before,"level_scope"))?;
  if status.get("room_coverage").map(String::as_str)!=Some("Complete")||status.get("level_coverage").map(String::as_str)!=Some("Complete"){return Err(format!("coverage partial:{command}:{status:?}"));}
  let recorder=c.command("cs world status")?;
  if recorder.get("world_state").map(String::as_str)!=Some("Recording"){return Err(format!("coverage recorder stopped:{command}:{recorder:?}"));}
  self.report["coverage_samples"].as_array_mut().ok_or("coverage samples")?.push(json!({"id":id,"role":role,"room_scope":status["room_scope"],"level_scope":status["level_scope"],"actual":actual}));
  Ok(())
 }
 pub(super) fn scenes_tx12(&mut self,g:&win32::Game)->Result<()>{
  let w=g.wait_window(90)?;let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("port")? as u16,g,90,&self.out.join("control.jsonl"))?;c.focus_window=Some(w);
  self.prepare_new_run_tx05(g,w,&mut c)?;self.sequence(w,"start_run")?;c.wait("run_active","1",25)?;
  self.action_tx12(g,&mut c,w,1,"seed")?;self.watch_tx05(g,&mut c,1500)?;
  c.command("cs world start room-level-full")?;self.wait_tx12(g,&mut c,"recorder","cs world status",|s|s.get("world_state").is_some_and(|v|v=="Recording"))?;
  c.command("cs observe start")?;c.command("cs solo start")?;self.wait_tx12(g,&mut c,"session","cs status",|s|s.get("session_state").is_some_and(|v|v=="Active"))?;
  let resume=self.scenario["tx12_pending_ids"].is_array();
  if !resume { for (i,name) in ["initial","progress_dirty","progress_clear","environment","queries","leave","revisit","shape2","shapeL","sacrifice","shop","challenge","downpour","mirror","normal","mines","death_certificate","death_return","ascent"].iter().enumerate(){
   self.action_tx12(g,&mut c,w,i as u64+2,name)?;self.watch_tx05(g,&mut c,1100)?;self.scopes_tx12(g,&mut c)?;
   self.step(name,json!({"actual":self.live_fixture()?.last()}))?;
  }}
  // 保留旧回归后执行同一tx补全清单；静态清单不是RoomType数量或变体全排列。
  let checklist:Value=serde_json::from_str(include_str!("../scenarios/tx-04-12-special-coverage.json")).map_err(|e|e.to_string())?;
  self.report["coverage_samples"]=json!([]);
  let reset=if resume {2}else{21};
  self.action_tx12(g,&mut c,w,reset,"coverage_reset")?;self.watch_tx05(g,&mut c,900)?;self.scopes_tx12(g,&mut c)?;
  let mut action=reset+1;
  for spec in checklist["cases"].as_array().ok_or("coverage cases")? {
   let id=spec["id"].as_str().ok_or("coverage id")?;
   if resume&&!self.scenario["tx12_pending_ids"].as_array().unwrap().iter().any(|v|v==id){continue;}
   self.coverage_action_tx12(g,&mut c,w,action,id,"case")?;action+=1;
   if spec["return_required"]==true{self.coverage_action_tx12(g,&mut c,w,action,id,"return")?;action+=1;}
   if spec["reenter_required"]==true{for role in ["reenter","return2"]{self.coverage_action_tx12(g,&mut c,w,action,id,role)?;action+=1;}}
  }
  self.action_tx12(g,&mut c,w,action,"coverage_finish")?;self.watch_tx05(g,&mut c,900)?;self.scopes_tx12(g,&mut c)?;
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
  // 每个进程独立核对session/epoch/scope，再以来源命名空间聚合，绝不跨进程拼接身份。
  let mut roots=self.report["resume_sources"].as_array().cloned().unwrap_or_default().iter().map(|v|PathBuf::from(v.as_str().unwrap())).collect::<Vec<_>>();roots.push(self.out.clone());
  let mut summaries=Vec::new();let mut samples=Vec::new();let mut total_required=0usize;
  let mut facts=BTreeSet::<String>::new();let mut shapes=BTreeSet::<u64>::new();let mut types=BTreeSet::<u64>::new();let mut dimensions=BTreeSet::<i64>::new();let mut floors=BTreeSet::<String>::new();let mut room_instances=BTreeSet::<String>::new();let mut counts=BTreeMap::<String,u64>::new();let mut descriptors=BTreeMap::<String,(String,String)>::new();let mut revisit=BTreeMap::<String,BTreeSet<String>>::new();let mut ordinary=BTreeMap::<String,Value>::new();let mut max_nodes=0u64;
  let mut seen_samples=BTreeSet::<String>::new();let mut offgrid_slots=BTreeSet::<i64>::new();let mut constructions=BTreeMap::<String,BTreeSet<String>>::new();
  for (source,base) in roots.iter().enumerate() {
  let root=base.join("runtime-world/room-level-full/observation-v2");let summary=crate::observation::read_capture(&root)?;
  if summary["delivery_complete"]!=true||summary["supported_read_complete"]!=true{return Err(format!("reader/delivery source {source}:{summary}"));}
  summaries.push(json!({"source":source,"read":summary}));
  let report=if base==&self.out {self.report.clone()}else{serde_json::from_str::<Value>(&crate::read(&base.join("result.json"))?).map_err(|e|e.to_string())?};
  let source_samples=report["coverage_samples"].as_array().ok_or("coverage samples")?.clone();samples.extend(source_samples.clone());
  let mut required=BTreeMap::<String,(String,Value)>::new();
  for sample in &source_samples {for (field,domain) in [("room_scope","room"),("level_scope","level")] {
   if required.insert(sample[field].as_str().ok_or("sample scope")?.into(),(domain.into(),sample.clone())).is_some(){return Err("coverage reused a sample scope".into());}
  }}
  total_required+=required.len();
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
   floors.insert(format!("{source}:{}",ctx["floor"]["local_id"].as_str().unwrap()));room_instances.insert(format!("{source}:{}",ctx["room_instance"]["local_id"].as_str().unwrap()));
   if p["sampling"]["cache_reused"]!=false||p["sample_revision"]!=ctx["scope"]{return Err("freshness/revision mismatch".into());}
   let capture=format!("{source}:{}",ctx["authority_tick"].as_str().unwrap());
   if let Some(old)=ordinary.insert(capture,ctx.clone()){for k in ["run","floor","dimension","room_descriptor","room_instance"]{if old[k]!=ctx[k]{return Err("mixed-domain generation".into());}}}
   if let Some((domain,sample))=required.get(ctx["scope"].as_str().unwrap_or("")) {
    let a=&sample["actual"];
    if (domain=="room")!=(schema=="isaac.cs.room"){return Err("coverage wrong domain scope".into());}
    if domain=="room" {
     for (member,key) in [("Room.GetType","type"),("Room.GetRoomShape","shape")] {
      if n(&p["state"]["fields"][member])!=n(&a[key]){return Err(format!("coverage actual {key} mismatch:{sample}"));}
     }
     if p["room_index"]!=a["room"]||p["dimension_index"]!=a["dimension"]||p["descriptor"]["grid_index"]!=a["grid"]||p["descriptor"]["list_index"]!=a["list"]||n(&p["config"]["state"]["fields"]["RoomConfigRoom.Subtype"])!=n(&a["subtype"])||n(&p["config"]["state"]["fields"]["RoomConfigRoom.Variant"])!=n(&a["variant"]){return Err(format!("coverage location/config mismatch:{sample}"));}
    } else {
     if n(&p["state"]["fields"]["Level.GetStage"])!=n(&a["stage"])||n(&p["state"]["fields"]["Level.GetStageType"])!=n(&a["stage_type"]){return Err(format!("coverage stage mismatch:{sample}"));}
     // 波次可在ready后、下一普通scope前自然递增；同房同层要求已观测进度不倒退。
     let wave=n(&p["state"]["fields"]["Level.GreedModeWave"]);let actual_wave=n(&a["wave"]);
     if wave.is_none()||actual_wave.is_none()||wave.unwrap()<actual_wave.unwrap(){return Err(format!("coverage greed wave mismatch:{sample}"));}
     if p["off_grid"]["complete"]!=true||p["off_grid"]["entries"].as_array().map(Vec::len)!=Some(19){return Err("off-grid membership missing".into());}
     if sample["id"]=="red_room"&&(p["membership"]["size"]!=a["room_count_after"]||n(&a["room_count_after"]).unwrap_or(0.0)<=n(&a["room_count_before"]).unwrap_or(f64::MAX)){return Err("red room formal membership did not grow".into());}
     if a["room"].as_i64().is_some_and(|v|v<0) {
      let slot=p["off_grid"]["entries"].as_array().unwrap().iter().find(|e|e["slot"]==a["room"]).ok_or("active off-grid slot not enumerated")?;
      if slot["status"]!="Initialized"||slot["subject"]["local_id"]!=ctx["room_descriptor"]["local_id"]||slot["subject"]["construction_revision"]!=p["current"]["construction_revision"]{return Err("off-grid current identity/revision mismatch".into());}
      offgrid_slots.insert(a["room"].as_i64().unwrap());
     }
    }
    seen_samples.insert(format!("{source}:{}",ctx["scope"].as_str().unwrap()));
   }
   if schema=="isaac.cs.room"{
    let f=&p["state"]["fields"];if n(&f["Room.GetRoomShape"]).is_some(){shapes.insert(n(&f["Room.GetRoomShape"]).unwrap() as u64);}if let Some(t)=n(&f["Room.GetType"]){types.insert(t as u64);}
    let rev=p["descriptor"]["construction_revision"].as_str().ok_or("construction revision")?;
    if rev=="0"{return Err("zero construction revision".into());}
    constructions.entry(format!("{source}:{}",ctx["room_descriptor"]["local_id"].as_str().unwrap())).or_default().insert(rev.into());
    let room=p["location"]["room_instance"].as_str().unwrap_or("");if ctx["room_instance"]["local_id"]!=room||ctx["floor"]["local_id"]!=p["location"]["floor"]||ctx["room_descriptor"]["local_id"]!=p["location"]["descriptor"]{return Err("payload/context identity mismatch".into());}
    let key=format!("{source}:{}:{}:{}",ctx["floor"]["local_id"],ctx["dimension"]["local_id"],ctx["room_descriptor"]["local_id"]);revisit.entry(key).or_default().insert(room.into());
    if f["Room.IsClear"]==false{facts.insert("unclear".into());}if f["Room.IsClear"]==true{facts.insert("clear".into());}if f["Room.IsSacrificeDone"]==true{facts.insert("sacrifice_progress".into());}
    if n(&f["Room.GetWaterAmount"])==Some(0.75)&&n(&f["Room.GetWaterCurrent"]["x"])==Some(1.25){facts.insert("water_current".into());}
    if n(&f["Room.GetFXParams"]["fields"]["FXParams.ShadowAlpha"])==Some(0.375){facts.insert("fx_light_shadow".into());}
    if p["config"]["state"]["fields"]["RoomConfigRoom.Spawns"]["kind"]=="array"{facts.insert("config_spawns".into());}
    if let Some(doors)=p["doors"].as_array(){for d in doors{if d["actual"]["resolution"]=="KnownCurrent"{facts.insert("actual_grid_door_reference".into());}if d["shape_allowed"]==false&&d["actual"]["resolution"]=="Null"{facts.insert("shape_vs_actual".into());}}}
   }else{
    if p["membership"]["complete"]!=true{return Err("level membership incomplete".into());}
    if p["state"]["fields"]["Level.IsAscent"]==true{facts.insert("ascent".into());}
    let entries=p["descriptors"].as_array().ok_or("descriptors")?;if entries.len()!=p["membership"]["size"].as_u64().ok_or("size")? as usize{return Err("descriptor membership count".into());}
    for d in entries{let id=&d["subject"];let dim=id["dimension_index"].as_i64().ok_or("descriptor dimension")?;dimensions.insert(dim);let key=format!("{source}:{}",id["local_id"].as_str().ok_or("descriptor id")?);let owner=(ctx["floor"]["local_id"].to_string(),id["dimension_index"].to_string());if let Some(previous)=descriptors.insert(key,owner.clone()){if previous!=owner{return Err("descriptor reused across floor/dimension".into());}}}
    if p["connections"].as_array().is_some_and(|a|a.iter().any(|c|c["neighbor"]["resolution"]=="KnownCurrent")){facts.insert("adjacency".into());}
    let f=&p["current_descriptor"]["fields"];if f["RoomDescriptor.NoReward"]==true&&n(&f["RoomDescriptor.ShopItemIdx"])==Some(7.0){facts.insert("descriptor_progress".into());}
   }
  }
  }
  if !(1..=12).all(|s|shapes.contains(&s))||!(1..=29).all(|s|types.contains(&s))||types.contains(&30){return Err(format!("actual shape/type coverage:{shapes:?} {types:?}"));}
  if floors.len()<4||!dimensions.contains(&0)||!dimensions.contains(&1)||!dimensions.contains(&2)||!revisit.values().any(|v|v.len()>1){return Err(format!("hierarchy/transitions:{floors:?} {dimensions:?} revisit={revisit:?}"));}
  for expected in ["transition","isaac.cs.room.query","isaac.cs.level.query","unclear","clear","sacrifice_progress","water_current","fx_light_shadow","config_spawns","actual_grid_door_reference","shape_vs_actual","descriptor_progress","adjacency","actual_clear_event","ascent"]{if !facts.contains(expected){return Err(format!("missing actual observation:{expected}; facts={facts:?}"));}}
  if seen_samples.len()!=total_required{return Err(format!("coverage missing formal samples:{} of {}",seen_samples.len(),total_required));}
  let checklist:Value=serde_json::from_str(include_str!("../scenarios/tx-04-12-special-coverage.json")).map_err(|e|e.to_string())?;
  for spec in checklist["cases"].as_array().unwrap(){
   let id=spec["id"].as_str().unwrap();
   for role in ["case","return","reenter","return2"] {
    if role=="return"&&spec["return_required"]!=true||["reenter","return2"].contains(&role)&&spec["reenter_required"]!=true{continue;}
    if !samples.iter().any(|s|s["id"]==id&&s["role"]==role){return Err(format!("missing coverage role:{id}:{role}"));}
   }
  }
  if !(-20..=-1).filter(|i|*i!=-19).all(|i|offgrid_slots.contains(&i))||!constructions.values().any(|v|v.len()>2){return Err("off-grid slots or reconstructed descriptor revisions missing".into());}
  self.case("special_room_complete_checklist",true,json!({"required_cases":checklist["cases"].as_array().unwrap().len(),"actual_samples":samples.len(),"formal_scopes":seen_samples.len()}));
  self.case("special_room_returns_revisits_and_versions",true,json!({"off_grid_slots":offgrid_slots,"reconstructed_subjects":constructions.values().filter(|s|s.len()>1).count()}));
  self.case("room_identity_config_shape_and_membership",true,json!({"shapes":shapes,"types":types,"room_instances":room_instances.len(),"descriptor_identities":descriptors.len()}));
  self.case("room_progress_environment_events_and_queries",true,json!({"facts":facts}));
  self.case("level_floor_dimension_structure_and_transitions",true,json!({"floors":floors,"dimensions":dimensions,"max_nodes":max_nodes,"schemas":counts}));
  let summary=json!({"captures":summaries,"delivery_complete":true,"supported_read_complete":true,"world_complete":false});
  self.case("reader_delivery_and_same_window_context",true,summary.clone());self.report["observation_read"]=summary;
  println!("room_level_acceptance {}",self.report["cases"]);Ok(())
 }
}
