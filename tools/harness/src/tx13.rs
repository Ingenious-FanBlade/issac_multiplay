//! tx-04-13统一E2E：实际结果与指定正式scope对照，不重跑09—12的已通过场景。
use crate::{Control,Result,Run,win32,remote::{Remote,Step,protocol::State}};
use serde_json::{Value,json};
use std::{collections::{BTreeMap,BTreeSet},fs,io::{BufRead,BufReader},thread,time::{Duration,Instant}};
type Status=BTreeMap<String,String>;
const DOMAINS:[&str;3]=["exploration","room_save","player"];
const ACTIONS:[&str;31]=["initial","reveal","hide","mutate","leave","revisit","box_pack","box_leave","box_return","box_unpack","inventory","effects_charge","twins","twins_leave","twins_return","forms","flip","forms_leave","forms_return","esau_prepare","esau_swap","esau_restore","death","revive","downpour","mirror","normal","myosotis","next_floor","query","disconnect_ready"];
fn number(s:&Status,k:&str)->u64{s.get(k).and_then(|v|v.parse().ok()).unwrap_or(0)}
fn n(v:&Value)->Option<f64>{v.as_f64().or_else(||v.as_str().and_then(|s|s.parse().ok())).or_else(||v["value"].as_str().and_then(|s|s.parse().ok()))}
fn parameter(v:&Value,index:f64)->&Value{
 static NULL:Value=Value::Null;
 v["values"].as_array().and_then(|a|a.iter().find(|e|e["parameters"]["entries"].as_array().is_some_and(|p|p.iter().any(|p|n(&p["key"])==Some(1.0)&&n(&p["value"])==Some(index))))).map(|e|&e["value"]).unwrap_or(&NULL)
}
impl Run {
 // 复用同tx已交付的正式scope；每份录制各自验证session/epoch，跨进程不串接身份。
 pub(super) fn prepare_resume_tx13(&mut self)->Result<()> {
  let Some(paths)=self.scenario["resume_from"].as_array() else{return Ok(())};
  let mut sources=Vec::new();let mut done=BTreeSet::<String>::new();
  for p in paths {
   let path=std::path::PathBuf::from(p.as_str().ok_or("resume path")?);
   let r:Value=serde_json::from_str(&crate::read(&path.join("result.json"))?).map_err(|e|e.to_string())?;
   if r["tx"]!="tx-04-13"||!r["evidence_error"].is_null()||r["cleanup_errors"].as_array().is_some_and(|a|a.iter().any(|e|!e.is_null())){return Err("resume environment not restored".into());}
   if let Some(prior)=r["continuity_resume_sources"].as_array(){for old in prior {sources.push(old.clone());}}
   sources.push(json!(path));
  }
  sources.sort_by_key(|p|p.as_str().unwrap_or("").to_owned());sources.dedup();
  for path in &sources {let root=std::path::PathBuf::from(path.as_str().unwrap());
   let summary=crate::observation::read_capture(&root.join("runtime-world/continuity-full/observation-v2"))?;
   if summary["delivery_complete"]!=true||summary["supported_read_complete"]!=true{return Err("resume delivery incomplete".into());}
   let report:Value=serde_json::from_str(&crate::read(&root.join("result.json"))?).map_err(|e|e.to_string())?;
   for s in report["continuity_samples"].as_array().ok_or("resume samples")?{done.insert(s["name"].as_str().ok_or("resume name")?.into());}
  }
  let retest=self.scenario["retest_ids"].as_array().cloned().unwrap_or_default();
  let pending=ACTIONS.iter().filter(|name|!done.contains(**name)||retest.iter().any(|id|id==**name)).map(|s|s.to_string()).collect::<Vec<_>>();
  // 当前续测从特殊形态段开始，准备只重建后段必需的源房/四人装备，不执行前15项测试。
  if !matches!(pending.first().map(String::as_str),Some("forms"|"revive"|"downpour"|"esau_swap")){return Err("resume needs phase-specific preparation for first pending scene".into());}
  for path in &sources {let r:Value=serde_json::from_str(&crate::read(&std::path::PathBuf::from(path.as_str().unwrap()).join("result.json"))?).map_err(|e|e.to_string())?;
   if r["cases"]["continuity_pause_and_optional_recording"]["pass"]==true {self.report["cases"]["continuity_pause_and_optional_recording"]=r["cases"]["continuity_pause_and_optional_recording"].clone();self.report["reuse_continuity_tail"]=json!(true);self.report["continuity_tail_source"]=path.clone();}
  }
  self.report["continuity_resume_sources"]=json!(sources);self.scenario["tx13_pending"]=json!(pending);Ok(())
 }

 // 长原生加载可超过控制口单次5秒；只重试完整收到的只读状态超时，绝不重发动作。
 fn watch_tx13(&mut self,g:&win32::Game,c:&mut Control,ms:u64)->Result<Status>{
  let until=Instant::now()+Duration::from_millis(ms);let deadline=Instant::now()+Duration::from_secs(60);
  loop{if let Some(code)=g.exited(){return Err(format!("game exited during tx13:{code}"));}
   // Windows原生长加载可能暂时使用ghost窗口；只读轮询不需要抢焦点。
   let focus=c.focus_window.take();let response=c.request("cs status");c.focus_window=focus;
   let s=response?;
   if s.get("error").map(String::as_str)==Some("main_thread_timeout"){
    if Instant::now()>=deadline{return Err("bounded native loading status wait expired".into());}thread::sleep(Duration::from_millis(60));continue;
   }
   if s.get("ok").map(String::as_str)!=Some("1"){return Err(format!("status:{s:?}"));}
   self.report["last_observed_status"]=json!(s);
   if s.get("session_error").is_some_and(|v|!v.is_empty())||s.get("last_error").is_some_and(|v|!v.is_empty()){return Err(format!("runtime failure:{s:?}"));}
   if Instant::now()>=until{return Ok(s)}thread::sleep(Duration::from_millis(80));
  }
 }
 fn wait_tx13(&mut self,g:&win32::Game,c:&mut Control,label:&str,command:&str,p:impl Fn(&Status)->bool)->Result<Status>{
  let end=Instant::now()+Duration::from_secs(60);
  loop {self.watch_tx13(g,c,0)?;if let Some(e)=self.live_fixture()?.iter().find(|r|r["event"]=="fixture_error"){return Err(format!("{label} fixture:{e}"));}
   let s=c.command(command)?;if p(&s){self.step(label,json!(s))?;return Ok(s)}
   if Instant::now()>=end{return Err(format!("{label} timeout:{s:?}"));}thread::sleep(Duration::from_millis(60));}
 }
 fn action_tx13(&mut self,g:&win32::Game,c:&mut Control,w:usize,index:u64,name:&str,remote:Option<&Remote>)->Result<Vec<Value>>{
  win32::key(w,"F8")?;let end=Instant::now()+Duration::from_secs(60);let mut sent=false;
  loop{self.watch_tx13(g,c,80)?;let rows=self.live_fixture()?;
   if let Some(e)=rows.iter().find(|r|r["event"]=="fixture_error"){return Err(format!("{name} fixture:{e}"));}
   if !sent&&rows.iter().any(|r|r["event"]=="action_return"&&r["action"]==index&&r["name"]==name){if let Some(remote)=remote{
    remote.script(name,vec![Step{after:0,state:State{held:2,edge:2,..State::default()}},Step{after:1,state:State::default()}])?;sent=true;
   }}
   if rows.iter().any(|r|r["event"]=="ready"&&r["action"]==index&&r["name"]==name){return Ok(rows.into_iter().filter(|r|r["action"]==index).collect());}
   if Instant::now()>=end{return Err(format!("{name} actual result timeout last={:?}",rows.last()));}}
 }
 fn scopes_tx13(&mut self,g:&win32::Game,c:&mut Control)->Result<Status>{
  let before=c.command("cs observe status")?;
  let s=self.wait_tx13(g,c,"continuity_scope_progress","cs observe status",|s|DOMAINS.iter().all(|d|number(s,&format!("{d}_scope"))>number(&before,&format!("{d}_scope"))))?;
  for d in DOMAINS {if s.get(&format!("{d}_coverage")).map(String::as_str)!=Some("Complete"){return Err(format!("{d} partial:{s:?}"));}}
  if number(&s,"loss_sequence")!=0||number(&s,"record_loss_sequence")!=0{return Err(format!("continuity publication loss:{s:?}"));}Ok(s)
 }
 pub(super) fn scenes_tx13(&mut self,g:&win32::Game)->Result<()> {
  let w=g.wait_window(90)?;let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("port")? as u16,g,90,&self.out.join("control.jsonl"))?;c.focus_window=Some(w);
  self.prepare_new_run_tx05(g,w,&mut c)?;self.sequence(w,"start_run")?;c.wait("run_active","1",25)?;
  self.action_tx13(g,&mut c,w,1,"seed",None)?;
  c.command("cs world start continuity-full")?;self.wait_tx13(g,&mut c,"recorder","cs world status",|s|s.get("world_state").is_some_and(|v|v=="Recording"))?;
  c.command("cs observe start")?;c.command(&format!("cs host {} 0",self.scenario["host_port"]))?;
  self.wait_tx13(g,&mut c,"host","cs status",|s|s.get("session_state").is_some_and(|v|v=="Active"))?;
  let mut remotes=Vec::new();
  for ordinal in 1..=3 {let remote=Remote::start_participant(format!("127.0.0.1:{}",self.scenario["host_port"]).parse().map_err(|e|format!("{e}"))?,&self.out.join(format!("remote-{ordinal}.jsonl")),self.scenario["peer_timeout_ms"].as_u64().unwrap_or(30000),2,ordinal,|_|Ok(()))?;
   self.wait_tx13(g,&mut c,"join_for_observation","cs status",|s|s.get(&format!("participant.{ordinal}.bound")).is_some_and(|v|v=="1"))?;remotes.push(remote);
  }
  self.report["continuity_samples"]=json!([]);
  let resume=self.scenario["tx13_pending"].as_array().cloned();
  let actions=if let Some(pending)=&resume {pending.iter().map(|p|p.as_str().unwrap()).collect::<Vec<_>>()}else{ACTIONS.to_vec()};
  if resume.is_some(){self.action_tx13(g,&mut c,w,2,"resume_prepare",None)?;self.scopes_tx13(g,&mut c)?;}
  for (i,name) in actions.iter().enumerate() {
   let actual=self.action_tx13(g,&mut c,w,i as u64+if resume.is_some(){3}else{2},name,if ["esau_swap","esau_restore"].contains(name){remotes.get(2)}else{None})?;
   // Forgotten原生Ctrl交换也走与房主相同的canonical输入，用新鲜普通状态作结果。
   if *name=="flip" {win32::key(w,"CTRL")?;self.watch_tx13(g,&mut c,500)?;}
   let status=self.scopes_tx13(g,&mut c)?;
   let recording=c.command("cs world status")?;if recording.get("world_state").map(String::as_str)!=Some("Recording"){return Err(format!("{name} recording stopped:{recording:?}"));}
   self.report["continuity_samples"].as_array_mut().unwrap().push(json!({"name":name,"status":status,"actual":actual}));
   self.step(name,json!({"scopes":status}))?;
  }
  // 远端退出的瞬时失效必须有关系记录，不能靠普通往返代替。
  for r in &remotes {r.bye()?;}
  self.wait_tx13(g,&mut c,"disconnect","cs status",|s|s.get("participants").is_some_and(|v|v=="1"))?;
  self.scopes_tx13(g,&mut c)?;drop(remotes);
  if self.report["reuse_continuity_tail"]!=true {win32::key(w,"ESC")?;c.wait_pause(None)?;let before=c.command("cs observe status")?;self.watch_tx13(g,&mut c,350)?;let after=c.command("cs observe status")?;
  for d in DOMAINS {if before[&format!("{d}_scope")]!=after[&format!("{d}_scope")]{return Err("paused ordinary continuity scope advanced".into());}}win32::key(w,"ESC")?;self.scopes_tx13(g,&mut c)?;}
  c.command("cs world stop")?;self.wait_tx13(g,&mut c,"recorder_closed","cs world status",|s|s.get("world_state").is_some_and(|v|v=="Closed"))?;
  if self.report["reuse_continuity_tail"]!=true {let file=self.out.join("runtime-world/continuity-full/observation-v2/records.jsonl");let length=fs::metadata(&file).map_err(|e|e.to_string())?.len();let b=c.command("cs observe status")?;let a=self.scopes_tx13(g,&mut c)?;
  if b["observation_epoch"]!=a["observation_epoch"]||fs::metadata(file).map_err(|e|e.to_string())?.len()!=length{return Err("recorder separation failed".into());}
  self.case("continuity_pause_and_optional_recording",true,json!({"before":b,"after":a}));}
  c.command("cs stop")?;self.wait_tx13(g,&mut c,"input_retired","cs status",|s|s.get("session_state").is_some_and(|v|v=="Idle"))?;
  c.command("cs observe stop")?;self.wait_tx13(g,&mut c,"observation_retired","cs observe status",|s|s.get("observation_state").is_some_and(|v|v=="Off"))?;
  self.return_to_menu(w,&mut c)?;let s=c.command("cs status")?;for k in ["trace_dropped","query_mismatches","cleanup_failed"]{if number(&s,k)!=0{return Err(format!("final {k}"));}}
  self.report["final_status"]=json!(s);drop(c);if g.close()?!=0{return Err("game exit".into());}self.report["continuity_scenes_completed"]=json!(true);self.reduce_tx13()?;Ok(())
 }
 fn reduce_tx13(&mut self)->Result<()> {
  let mut roots=self.report["continuity_resume_sources"].as_array().cloned().unwrap_or_default().iter().map(|p|std::path::PathBuf::from(p.as_str().unwrap())).collect::<Vec<_>>();roots.push(self.out.clone());
  let mut summaries=Vec::new();let mut names=BTreeSet::new();let mut required_total=0;
  let mut counts=BTreeMap::<String,u64>::new();let mut coverages=BTreeMap::<String,u64>::new();let mut floors=BTreeSet::<String>::new();let mut dims=BTreeSet::<String>::new();let mut boundary=0;let mut controls=0;let mut invalidations=0;let mut max_bodies=0;
  let mut saved_nonempty=0;let mut backup_present=0;let mut backup_owned=0;let mut crossed=0;let mut history_count=0;let mut map_changes=0;
  let mut familiar_rooms=BTreeMap::<String,BTreeSet<String>>::new();
  let mut winners=BTreeMap::<String,usize>::new();
  for (source,base) in roots.iter().enumerate(){let report=if base==&self.out{self.report.clone()}else{serde_json::from_str::<Value>(&crate::read(&base.join("result.json"))?).map_err(|e|e.to_string())?};
   for s in report["continuity_samples"].as_array().ok_or("source samples")?{winners.insert(s["name"].as_str().ok_or("source name")?.into(),source);}
  }
  for (source,base) in roots.iter().enumerate(){
  let root=base.join("runtime-world/continuity-full/observation-v2");let summary=crate::observation::read_capture(&root)?;
  if summary["delivery_complete"]!=true||summary["supported_read_complete"]!=true{return Err(format!("reader/delivery:{summary}"));}summaries.push(json!({"source":source,"read":summary}));
  let report=if base==&self.out {self.report.clone()}else{serde_json::from_str::<Value>(&crate::read(&base.join("result.json"))?).map_err(|e|e.to_string())?};
  let samples=report["continuity_samples"].as_array().ok_or("continuity samples")?.iter().filter(|s|winners.get(s["name"].as_str().unwrap_or(""))==Some(&source)).cloned().collect::<Vec<_>>();
  for sample in &samples{names.insert(sample["name"].as_str().ok_or("scene name")?.to_string());}
  let mut required=BTreeSet::<String>::new();
  for s in &samples {for d in DOMAINS {required.insert(s["status"][format!("{d}_scope")].as_str().ok_or("sample scope")?.into());}required.insert(s["status"]["entity_scope"].as_str().ok_or("entity scope")?.into());}
  required_total+=required.len();let mut selected=BTreeMap::<String,Value>::new();
  for line in BufReader::new(fs::File::open(root.join("records.jsonl")).map_err(|e|e.to_string())?).lines(){let text=line.map_err(|e|e.to_string())?;let r:Value=serde_json::from_str(&text).map_err(|e|e.to_string())?;
   let schema=r["schema"].as_str().unwrap_or("");
   if required.contains(r["context"]["scope"].as_str().unwrap_or("")){selected.insert(r["context"]["scope"].as_str().unwrap().into(),r.clone());}
   if !DOMAINS.iter().any(|d|schema.starts_with(&format!("isaac.cs.{d}"))){continue;}
   *counts.entry(schema.into()).or_default()+=1;*coverages.entry(r["coverage"]["status"].as_str().unwrap_or("").into()).or_default()+=1;
   let raw=text.split_once(",\"payload\":").ok_or("payload")?.1;if r["coverage"]["bytes"].as_u64()!=Some(raw.len().saturating_sub(1) as u64){return Err("continuity payload byte accounting".into());}
   if schema.ends_with(".transition") {boundary+=1;if !r["context"]["authority_tick"].is_null()||r["payload"]["new_context"]!="UnknownUntilOrdinaryPost"{return Err("boundary borrowed ordinary tick".into());}}
   if schema=="isaac.cs.player.control"{controls+=1;if r["payload"]["after"]["resolution"]=="NotCaptured"{invalidations+=1;}}
   if schema=="isaac.cs.player"{max_bodies=max_bodies.max(r["payload"]["bodies"].as_array().map_or(0,Vec::len));}
  }
  if required.len()!=selected.len(){return Err("selected formal records absent".into());}
  for sample in &samples {
   let name=sample["name"].as_str().unwrap();let actual=sample["actual"].as_array().ok_or("actual rows")?;let location=actual.iter().find(|a|a["event"]=="actual_location").ok_or("actual location")?;
   let mut contexts=Vec::new();
   for d in DOMAINS {
    let scope=sample["status"][format!("{d}_scope")].as_str().unwrap();let row=&selected[scope];let p=&row["payload"];let ctx=&row["context"];
    if row["schema"]!=format!("isaac.cs.{d}")||row["coverage"]["status"]!="Complete"||p["sample_revision"]!=ctx["scope"]||p["location"]["scope"]!=ctx["scope"]{return Err(format!("invalid formal {name} {d}:{row}"));}
    for k in ["run","floor","dimension","room_descriptor","room_instance"] {if ctx[k]["presence"]!="Known"||ctx[k]["local_id"]=="0"{return Err(format!("missing {name} context {k}"));}}
    floors.insert(format!("{source}:{}",ctx["floor"]["local_id"].as_str().unwrap()));dims.insert(format!("{source}:{}",ctx["dimension"]["local_id"].as_str().unwrap()));contexts.push(ctx);
    if d=="exploration" {
     let desc=p["descriptors"].as_array().ok_or("exploration descriptors")?.iter().find(|r|r["subject"]["local_id"]==ctx["room_descriptor"]["local_id"]).ok_or("current knowledge missing")?;
     if n(&desc["state"]["display_flags"])!=n(&location["display_flags"])||n(&desc["state"]["visited_count"])!=n(&location["visited"])||desc["state"]["clear"]!=location["clear"]||n(&p["world_knowledge"]["curses"])!=n(&location["curses"]){return Err(format!("map actual mismatch {name}"));}
     if n(&p["local_display"]["fields"]["Minimap.GetState"])!=n(&location["minimap"]){return Err(format!("Minimap actual mismatch {name}"));}
     map_changes+=p["changes"].as_array().map_or(0,Vec::len);
    } else if d=="room_save" {
     for room in p["rooms"].as_array().ok_or("save rooms")? {for col in room["collections"].as_array().ok_or("save collections")? {
      let entries=col["entries"].as_array();let count=n(&col["count"]).unwrap_or(-1.0) as usize;if count!=entries.map_or(0,Vec::len){return Err("save membership mismatch".into());}if count>0{saved_nonempty+=1;}
      for e in entries.into_iter().flatten(){
       if col["encoding"]=="grid_columns_v2" {
        if col["entry_namespace"]["space"]!=8||e["local_id"]=="0"||e["local_id"].as_str().is_none()||col["entry_relation"]["resolution"]!="Unresolved"||col["columns"].as_array().map(Vec::len)!=Some(8)||e["values"].as_array().map(Vec::len)!=Some(8){return Err("column save entry incomplete".into());}
       }else if e["subject"]["space"]!=8||e["subject"]["local_id"]=="0"||e["relation"]["resolution"]!="Unresolved"{return Err("save borrowed active entity identity".into());}
      }
     }}
     for a in actual.iter().filter(|a|a["event"]=="actual_saved_grid") {
      let room=p["rooms"].as_array().unwrap().iter().find(|r|r["subject"]["grid_index"]==a["source"]&&r["subject"]["dimension_index"]==a["dimension"]).ok_or("actual saved grid descriptor missing")?;
      let col=room["collections"].as_array().unwrap().iter().find(|c|c["kind"]=="grid").ok_or("actual saved grid collection missing")?;
      let entry=col["entries"].as_array().ok_or("actual saved grid rows")?.iter().find(|e|e["ordinal"]==a["ordinal"]).ok_or("actual saved grid ordinal missing")?;
      for (i,key) in ["GridEntityDesc.Initialized","GridEntityDesc.SpawnCount","GridEntityDesc.SpawnSeed","GridEntityDesc.State","GridEntityDesc.Type","GridEntityDesc.VarData","GridEntityDesc.VariableSeed","GridEntityDesc.Variant"].iter().enumerate(){
       let value=if col["encoding"]=="grid_columns_v2" {if col["columns"][i]!=*key{return Err("save column declaration mismatch".into());}&entry["values"][i]}else{&entry["value"]["fields"][*key]};
       if (i==0&&*value!=a["values"][i])||(i>0&&n(value)!=n(&a["values"][i])){return Err(format!("saved Grid column actual mismatch {name} {key}"));}
      }
     }
     if location["dimension"]==0 {if let Some(src)=p["rooms"].as_array().unwrap().iter().find(|r|r["subject"]["grid_index"]==location["source"]&&r["subject"]["dimension_index"]==0){
      for (kind,key) in [("entity","entities"),("grid","grids"),("decoration","deco")] {let col=src["collections"].as_array().unwrap().iter().find(|c|c["kind"]==kind).ok_or("save kind")?;if !location["source_saved"][key].is_null()&&col["count"]!=location["source_saved"][key]{return Err(format!("save actual {name}:{key}"));}}
     }}
    } else {
     let bodies=p["bodies"].as_array().ok_or("bodies")?;
     if ["initial","leave","revisit"].contains(&name){
      let entity=&selected[sample["status"]["entity_scope"].as_str().unwrap()];
      for object in entity["payload"]["objects"].as_array().ok_or("canonical objects")?{if object["class"]=="EntityFamiliar"{
       let owner=&object["body"]["fields"]["EntityFamiliar.Player"];
       if owner["resolution"]!="KnownCurrent"||!bodies.iter().any(|b|b["subject"]["local_id"]==owner["local_id"]){return Err("familiar owner unresolved".into());}
       familiar_rooms.entry(format!("{source}:{}:{}",object["subject"]["local_id"],object["subject"]["use"])).or_default().insert(ctx["room_instance"]["local_id"].as_str().unwrap().into());
      }}
     }
     for body in bodies {
      if body["canonical_state"]["scope"]!=sample["status"]["entity_scope"] {return Err("canonical body scope mismatch".into());}
      let entity=&selected[body["canonical_state"]["scope"].as_str().unwrap()];
      for k in ["authority_tick","run","floor","dimension","room_descriptor","room_instance"]{if entity["context"][k]!=ctx[k]{return Err("body mixed capture context".into());}}
      let object=entity["payload"]["objects"].as_array().ok_or("canonical objects")?.iter().find(|o|o["subject"]["local_id"]==body["subject"]["local_id"]&&o["subject"]["use"]==body["subject"]["use"]).ok_or("canonical body missing")?;
      if body["relations"]["membership"]["body_role"]==5&&body["relations"]["membership"]["can_consume"]==false {backup_owned+=1;}
      if body["continuity"]["cross_room"]==true{crossed+=1;}history_count+=body["history"]["fields"]["History.GetCollectiblesHistory"]["entries"].as_array().map_or(0,Vec::len);
      if let Some(a)=actual.iter().find(|a|a["event"]=="actual_player"&&a["subject"]["local_id"]==body["subject"]["local_id"]) {
       let f=&object["body"]["fields"];
       // Ctrl/Flip的实际交换发生在ready之后；该动作只检查关系与同窗口规范引用。
       if name!="flip" {
        if body["relations"]["dead"]!=a["dead"] {return Err(format!("life state actual {name}"));}
        let history=body["history"]["fields"]["History.GetCollectiblesHistory"]["entries"].as_array().map_or(0,Vec::len);
        if Some(history as f64)!=n(&a["history"]){return Err(format!("history actual {name}"));}
        let inventory=&f["EntityPlayer.GetCollectiblesList"]["entries"];
        let onion=inventory.as_array().and_then(|es|es.iter().find(|e|n(&e["key"])==Some(1.0))).and_then(|e|n(&e["value"])).unwrap_or(0.0);
        if Some(onion)!=n(&a["sad_onion"]){return Err(format!("inventory actual {name}"));}
        let effects=&f["EntityPlayer.GetEffects"]["fields"]["TemporaryEffects.GetEffectsList"]["values"];
        let wafer=effects.as_array().and_then(|es|es.iter().find(|e|n(&e["fields"]["TemporaryEffect.Item"]["fields"]["ItemConfigItem.ID"])==Some(108.0))).and_then(|e|n(&e["fields"]["TemporaryEffect.Count"])).unwrap_or(0.0);
        if Some(wafer)!=n(&a["wafer_effect"]){return Err(format!("temporary effects actual {name}"));}
        for (member,key) in [("GetPlayerType","player_type"),("GetNumCoins","coins"),("GetNumKeys","keys"),("GetNumBombs","bombs"),("GetHearts","hearts"),("GetSoulHearts","soul")] {
        if n(&f[format!("EntityPlayer.{member}")])!=n(&a[key]){return Err(format!("canonical actual {name} {member}: formal={} actual={}",f[format!("EntityPlayer.{member}")],a[key]));}}
        for (member,key,index) in [("GetCard","card",0.0),("GetPill","pill",1.0),("GetTrinket","trinket",0.0),("GetActiveItem","active",0.0),("GetActiveCharge","charge",0.0)]{if n(parameter(&f[format!("EntityPlayer.{member}")],index))!=n(&a[key]){return Err(format!("parameter actual {name} {member}"));}}
       }
      }
     }
     for b in p["backup_slots"].as_array().ok_or("backups")?{if b["body"]["local_id"].as_str().is_some_and(|id|id!="0"){backup_present+=1;}}
    }
   }
   for pair in contexts.windows(2){for k in ["authority_tick","run","floor","dimension","room_descriptor","room_instance"]{if pair[0][k]!=pair[1][k]{return Err(format!("mixed {name} contexts"));}}}
  }
  }
  if names.len()!=ACTIONS.len(){return Err(format!("missing continuity scene:{names:?}"));}
  let summary=json!({"captures":summaries,"delivery_complete":true,"supported_read_complete":true,"world_complete":false});
  let continuing_familiars=familiar_rooms.values().filter(|rooms|rooms.len()>=2).count();
  if continuing_familiars<4{return Err("Familiar cross-room subjects/owners missing".into());}
  if saved_nonempty==0||backup_present==0||backup_owned==0||crossed==0||history_count==0||map_changes==0||boundary==0||controls==0||invalidations==0||max_bodies<8||floors.len()<2||dims.len()<2{return Err(format!("missing actual observations saves={saved_nonempty} backups={backup_present} cross={crossed} history={history_count} changes={map_changes} boundary={boundary} controls={controls} invalidations={invalidations} bodies={max_bodies} floors={floors:?} dims={dims:?}"));}
  self.case("exploration_knowledge_map_curse_dimension_history",true,json!({"changes":map_changes,"dimensions":dims,"floors":floors}));
  self.case("room_save_collections_and_separate_identity",true,json!({"nonempty_collections":saved_nonempty,"identity":"SaveEntry;unresolved per-object provenance"}));
  self.case("player_canonical_state_control_and_cross_room_history",true,json!({"max_bodies":max_bodies,"cross_room_links":crossed,"history_items":history_count,"backup_samples":backup_present,"owned_inactive_backups":backup_owned,"control_changes":controls,"invalidations":invalidations,"continuing_familiars":continuing_familiars}));
  self.case("continuity_reader_delivery_and_scope_integrity",true,summary.clone());self.report["observation_read"]=summary;self.report["continuity_coverage"]=json!({"scenes":names.len(),"required_scopes":required_total,"schemas":counts,"coverage":coverages});
  println!("continuity_acceptance {}",self.report["continuity_coverage"]);Ok(())
 }
}


// 正常完成实机/恢复后，归约器修复只重读原正式记录；不重复已完成的玩法场景。
pub(crate) fn reduce_existing(root:&std::path::Path)->Result<()> {
 let report:Value=serde_json::from_str(&crate::read(&root.join("result.json"))?).map_err(|e|e.to_string())?;
 if report["tx"]!="tx-04-13"||report["continuity_scenes_completed"]!=true||!report["evidence_error"].is_null()||report["cleanup_errors"].as_array().is_none_or(|a|a.iter().any(|v|!v.is_null())){return Err("offline reduction requires completed scenes and restored environment".into());}
 let repo=std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR")).parent().unwrap().parent().unwrap().to_path_buf();
 let mut run=Run{repo,game:std::path::PathBuf::new(),out:root.to_path_buf(),save:std::path::PathBuf::new(),scenario:json!({}),report,steps:fs::OpenOptions::new().append(true).open(root.join("steps.jsonl")).map_err(|e|e.to_string())?,deployed:false,fixture_owned:false,saves:None,deployment:vec![]};
 run.reduce_tx13()?;
 let failure=run.report["failure"].clone();run.report["prior_reduction_failure"]=failure;run.report["failure"]=Value::Null;run.report["pass"]=json!(true);crate::write_json(&root.join("result.json"),&run.report)?;
 println!("tx-04-13 reduction PASS cases={}",run.report["cases"].as_object().map_or(0,|c|c.len()));Ok(())
}
