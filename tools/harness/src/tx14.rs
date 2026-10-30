//! tx-04-14统一E2E：用真实框架和实际动作，对照四个正式域；不重测09—13既有清单。
use crate::{Control,Result,Run,win32,remote::Remote};
use serde_json::{Value,json};
use std::{collections::{BTreeMap,BTreeSet},fs,thread,time::{Duration,Instant}};
type Status=BTreeMap<String,String>;
const DOMAINS:[&str;4]=["run","random","presentation","mod"];
const ACTIONS:[&str;21]=["initial","rules","seed_effect","random_calls","pool_changes","pill_identify","hud","audio","graph","graph_limits","graph_recover","stage_a","stage_mutate","grid_remove","grid_rebuild","stage_b","stage_return","next_floor","query","retire","registry_budget"];
fn number(s:&Status,k:&str)->u64{s.get(k).and_then(|s|s.parse().ok()).unwrap_or(0)}
fn n(v:&Value)->Option<f64>{v.as_f64().or_else(||v.as_str().and_then(|s|s.parse().ok())).or_else(||v["value"].as_str().and_then(|s|s.parse().ok()))}
// 仅投影有界图用于断言；共享/循环引用保留$ref，不在reader内展开循环。
fn graph(v:&Value)->Value {
 fn inventory(v:&Value,nodes:&mut BTreeMap<u64,Value>){if v["type"]=="table"{if let Some(id)=v["node"].as_u64(){nodes.insert(id,v.clone());}}if let Some(o)=v.as_object(){for x in o.values(){inventory(x,nodes)}}if let Some(a)=v.as_array(){for x in a{inventory(x,nodes)}}}
 fn copy(v:&Value,nodes:&BTreeMap<u64,Value>,active:&mut BTreeSet<u64>)->Value {
  if v["type"]=="int64"{return v["value"].clone()}
  if v["type"]=="graph_ref"{if let Some(id)=v["node"].as_u64(){if active.contains(&id){return json!({"$ref":id})}if let Some(target)=nodes.get(&id){return copy(target,nodes,active)}}return json!({"$ref":v["node"]})}
  if v["type"]=="table"{let id=v["node"].as_u64().unwrap_or(0);active.insert(id);let mut out=serde_json::Map::new();if let Some(a)=v["entries"].as_array(){for e in a{let k=copy(&e["key"],nodes,active);let key=k.as_str().map(String::from).unwrap_or_else(||k.to_string());out.insert(key,copy(&e["value"],nodes,active));}}active.remove(&id);return Value::Object(out)}
  if let Some(a)=v.as_array(){return Value::Array(a.iter().map(|v|copy(v,nodes,active)).collect())}
  if let Some(o)=v.as_object(){return Value::Object(o.iter().map(|(k,v)|(k.clone(),copy(v,nodes,active))).collect())}v.clone()
 }
 let mut nodes=BTreeMap::new();inventory(v,&mut nodes);copy(v,&nodes,&mut BTreeSet::new())
}
fn adapter(v:&Value,name:&str)->Value {v["adapters"].as_array().and_then(|a|a.iter().find(|s|s["namespace"]==name)).map(|a|graph(&a["state"]["root"])).unwrap_or(Value::Null)}
impl Run {
 pub(super) fn prepare_resume_tx14(&mut self)->Result<()> {
  let Some(paths)=self.scenario["resume_from"].as_array() else{return Ok(())};let mut sources=Vec::new();let mut done=BTreeSet::new();
  for p in paths {let base=std::path::PathBuf::from(p.as_str().ok_or("resume path")?);let report:Value=serde_json::from_str(&crate::read(&base.join("result.json"))?).map_err(|e|e.to_string())?;
   if report["tx"]!="tx-04-14"||report["cleanup_errors"].as_array().is_none_or(|a|a.iter().any(|x|!x.is_null())){return Err("resume requires restored environment".into());}
   if let Some(old)=report["run_resume_sources"].as_array(){sources.extend(old.iter().cloned());}sources.push(json!(base));
  }
  sources.sort_by_key(|p|p.as_str().unwrap_or("").to_string());sources.dedup();
  for p in &sources {let base=std::path::PathBuf::from(p.as_str().unwrap());let report:Value=serde_json::from_str(&crate::read(&base.join("result.json"))?).map_err(|e|e.to_string())?;
   if report["cases"]["run_pause_and_optional_recording"]["pass"]==true&&report["evidence_error"].is_null(){self.report["cases"]["run_pause_and_optional_recording"]=report["cases"]["run_pause_and_optional_recording"].clone();self.report["reuse_run_tail"]=json!(true);self.report["run_tail_source"]=p.clone();}
   for s in report["run_samples"].as_array().ok_or("resume samples")?{done.insert(s["name"].as_str().ok_or("name")?.to_string());}}
  let retest=self.scenario["retest_ids"].as_array().cloned().unwrap_or_default();
  let pending=ACTIONS.iter().filter(|s|!done.contains(**s)||retest.iter().any(|r|r==**s)).map(|s|s.to_string()).collect::<Vec<_>>();
  self.report["run_resume_sources"]=json!(sources);self.scenario["tx14_pending"]=json!(pending);Ok(())
 }

 pub(super) fn wait_tx14(&mut self,g:&win32::Game,c:&mut Control,command:&str,p:impl Fn(&Status)->bool)->Result<Status>{
  let end=Instant::now()+Duration::from_secs(90);
  loop {if let Some(code)=g.exited(){return Err(format!("tx14 process exit:{code}"));}
   if let Some(e)=self.live_fixture()?.iter().find(|r|r["event"]=="fixture_error"){return Err(format!("tx14 fixture:{e}"));}
   let focus=c.focus_window.take();let response=c.request(command);c.focus_window=focus;let s=response?;
   if s.get("error").map(String::as_str)==Some("main_thread_timeout"){if Instant::now()>=end{return Err("tx14 loading deadline".into());}continue;}
   if s.get("ok").map(String::as_str)!=Some("1"){return Err(format!("tx14 command:{s:?}"));}
   if p(&s){return Ok(s)}if Instant::now()>=end{return Err(format!("tx14 wait:{command}:{s:?}"));}thread::sleep(Duration::from_millis(80));
  }
 }
 pub(super) fn action_tx14(&mut self,g:&win32::Game,c:&mut Control,w:usize,index:usize)->Result<Vec<Value>>{
  win32::key(w,"F8")?;let end=Instant::now()+Duration::from_secs(90);
  loop{self.wait_tx14(g,c,"cs status",|_|true)?;let rows=self.live_fixture()?;
   if rows.iter().any(|r|r["event"]=="ready"&&r["action"]==index){return Ok(rows.into_iter().filter(|r|r["action"]==index).collect())}
   if Instant::now()>=end{return Err(format!("tx14 actual result timeout:{index}"));}thread::sleep(Duration::from_millis(80));}
 }
 fn scopes_tx14(&mut self,g:&win32::Game,c:&mut Control,partial:&str)->Result<Status>{
  let b=c.command("cs observe status")?;
  let s=self.wait_tx14(g,c,"cs observe status",|s|DOMAINS.iter().all(|d|number(s,&format!("{d}_scope"))>number(&b,&format!("{d}_scope"))))?;
  for d in DOMAINS {let expected=if d==partial{"Partial"}else{"Complete"};if s.get(&format!("{d}_coverage")).map(String::as_str)!=Some(expected){return Err(format!("tx14 {d} expected {expected}:{s:?}"));}}
  if number(&s,"loss_sequence")!=0||number(&s,"record_loss_sequence")!=0{return Err("tx14 publication loss".into());}Ok(s)
 }
 pub(super) fn scenes_tx14(&mut self,g:&win32::Game)->Result<()> {
  if self.scenario["global_random"]==true{return self.scenes_tx14_global_random(g)}
  let w=g.wait_window(90)?;let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("port")? as u16,g,90,&self.out.join("control.jsonl"))?;c.focus_window=Some(w);
  self.prepare_new_run_tx05(g,w,&mut c)?;self.sequence(w,"start_run")?;c.wait("run_active","1",30)?;
  self.action_tx14(g,&mut c,w,1)?;
  c.command("cs world start run-full")?;self.wait_tx14(g,&mut c,"cs world status",|s|s.get("world_state").is_some_and(|s|s=="Recording"))?;
  c.command("cs observe start")?;c.command(&format!("cs host {} 0",self.scenario["host_port"]))?;
  self.wait_tx14(g,&mut c,"cs status",|s|s.get("session_state").is_some_and(|s|s=="Active"))?;
  let remote=Remote::start_participant(format!("127.0.0.1:{}",self.scenario["host_port"]).parse().map_err(|e|format!("{e}"))?,&self.out.join("remote-1.jsonl"),30000,2,1,|_|Ok(()))?;
  self.wait_tx14(g,&mut c,"cs status",|s|s.get("participant.1.bound").is_some_and(|v|v=="1"))?;
  self.report["run_samples"]=json!([]);
  let actions=self.scenario["tx14_pending"].as_array().map(|a|a.iter().map(|s|s.as_str().unwrap().to_string()).collect::<Vec<_>>()).unwrap_or_else(||ACTIONS.iter().map(|s|s.to_string()).collect());
  let resumed=self.scenario["tx14_pending"].is_array();if resumed{self.action_tx14(g,&mut c,w,2)?;self.scopes_tx14(g,&mut c,"")?;}
  for (i,name) in actions.iter().enumerate(){
   let actual=self.action_tx14(g,&mut c,w,i+if resumed{3}else{2})?;
   let status=self.scopes_tx14(g,&mut c,if name=="graph_limits"{"mod"}else if name=="registry_budget"{"random"}else{""})?;
   self.report["run_samples"].as_array_mut().unwrap().push(json!({"name":name,"status":status,"actual":actual}));self.step(name,json!(status))?;
  }
  if self.report["reuse_run_tail"]!=true {
  win32::key(w,"ESC")?;c.wait_pause(None)?;let b=c.command("cs observe status")?;thread::sleep(Duration::from_millis(400));let a=c.command("cs observe status")?;
  if DOMAINS.iter().any(|d|b.get(&format!("{d}_scope"))!=a.get(&format!("{d}_scope"))){return Err("paused ordinary scopes advanced".into());}
  win32::key(w,"ESC")?;self.scopes_tx14(g,&mut c,"")?;
  }
  c.command("cs world stop")?;self.wait_tx14(g,&mut c,"cs world status",|s|s.get("world_state").is_some_and(|s|s=="Closed"))?;
  if self.report["reuse_run_tail"]!=true {
  let file=self.out.join("runtime-world/run-full/observation-v2/records.jsonl");let length=fs::metadata(&file).map_err(|e|e.to_string())?.len();let before=c.command("cs observe status")?;let after=self.scopes_tx14(g,&mut c,"")?;
  if before["observation_epoch"]!=after["observation_epoch"]||fs::metadata(file).map_err(|e|e.to_string())?.len()!=length{return Err("recording separation".into());}
  self.case("run_pause_and_optional_recording",true,json!({"before":before,"after":after}));
  }
  remote.bye()?;drop(remote);
  c.command("cs stop")?;self.wait_tx14(g,&mut c,"cs status",|s|s.get("session_state").is_some_and(|s|s=="Idle"))?;
  c.command("cs observe stop")?;self.wait_tx14(g,&mut c,"cs observe status",|s|s.get("observation_state").is_some_and(|s|s=="Off"))?;
  self.return_to_menu(w,&mut c)?;let final_status=c.command("cs status")?;
  for k in ["trace_dropped","query_mismatches","cleanup_failed"]{if number(&final_status,k)!=0{return Err(format!("final {k}"));}}
  self.report["final_status"]=json!(final_status);drop(c);if g.close()?!=0{return Err("game exit".into());}self.reduce_tx14()
 }
 fn reduce_tx14(&mut self)->Result<()> {
  let mut roots=self.report["run_resume_sources"].as_array().cloned().unwrap_or_default().iter().map(|p|std::path::PathBuf::from(p.as_str().unwrap())).collect::<Vec<_>>();roots.push(self.out.clone());
  let mut summaries=Vec::new();let mut samples=BTreeMap::<String,(usize,Value)>::new();
  for (source,base) in roots.iter().enumerate(){let report=if base==&self.out{self.report.clone()}else{serde_json::from_str::<Value>(&crate::read(&base.join("result.json"))?).map_err(|e|e.to_string())?};
   for sample in report["run_samples"].as_array().ok_or("source samples")?{samples.insert(sample["name"].as_str().ok_or("source name")?.into(),(source,sample.clone()));}
  }
  let mut selected=BTreeMap::<String,Value>::new();let mut calls=BTreeMap::<String,usize>::new();let mut events=BTreeMap::<String,usize>::new();let mut render=0;let mut graph_refs=0;let mut removals=BTreeMap::<usize,BTreeSet<String>>::new();
  for (source,base) in roots.iter().enumerate(){
  let root=base.join("runtime-world/run-full/observation-v2");let manifest:Value=serde_json::from_str(&crate::read(&root.join("manifest.json"))?).map_err(|e|e.to_string())?;
  if root.join("result.json").exists(){let summary=crate::observation::read_capture(&root)?;if summary["supported_read_complete"]!=true||summary["delivery_complete"]!=true{return Err(format!("reader:{summary}"));}summaries.push(json!({"source":source,"read":summary}));}
  else {summaries.push(json!({"source":source,"delivery_complete":false,"reason":"interrupted_process_prefix;only_explicit_selected_delivered_scopes_reused"}));}
  let mut scope_ids=BTreeSet::new();
  let raw=fs::read(root.join("records.jsonl")).map_err(|e|e.to_string())?;
  let end=if root.join("result.json").exists(){raw.len()}else{raw.iter().rposition(|b|*b==b'\n').map(|i|i+1).unwrap_or(0)};
  if end<raw.len(){summaries.last_mut().unwrap()["truncated_final_line_bytes"]=json!(raw.len()-end);}
  for text in std::str::from_utf8(&raw[..end]).map_err(|e|e.to_string())?.lines(){let r:Value=serde_json::from_str(&text).map_err(|e|e.to_string())?;
   if r["context"]["session"]!=manifest["session"]||r["context"]["epoch"]!=manifest["observation_epoch"]||!scope_ids.insert(r["context"]["scope"].clone().to_string()){return Err("source session/epoch/duplicate scope".into());}
   if DOMAINS.iter().any(|d|r["schema"].as_str().unwrap_or("").starts_with(&format!("isaac.cs.{d}"))){
    let raw=text.split_once(",\"payload\":").ok_or("payload")?.1;if r["coverage"]["bytes"].as_u64()!=Some(raw.len().saturating_sub(1) as u64){return Err("payload byte accounting".into());}
    if r["context"]["phase"]!="OrdinaryPost"&&!r["context"]["authority_tick"].is_null(){return Err("independent scope borrowed tick".into());}
    if r["schema"].as_str().is_some_and(|s|s.ends_with(".actual-call")){*calls.entry(r["payload"]["member"].as_str().unwrap_or("").into()).or_default()+=1;}
    if let Some(name)=r["payload"]["event"].as_str(){*events.entry(name.into()).or_default()+=1;
     if name=="POST_REMOVE_CUSTOM_GRID"{let d=graph(&r["payload"]["data"]["root"]);if let Some(id)=d["subject"]["local_id"].as_str(){removals.entry(source).or_default().insert(id.into());}}
    }
    if r["schema"]=="isaac.cs.presentation.render"{render+=1;}
    graph_refs+=text.matches("graph_ref").count();
   }
   selected.insert(format!("{}:{}",source,r["context"]["scope"].as_str().ok_or("scope")?),r);
  }
  }
  let mut seen=BTreeSet::new();let mut grid_ids=BTreeMap::new();let mut rebuilt=None;let mut actual_types=BTreeMap::new();let mut selected_count=0;
  for (source,sample) in samples.values(){
   let name=sample["name"].as_str().ok_or("name")?;seen.insert(name.to_string());
   let actual=sample["actual"].as_array().and_then(|a|a.iter().find(|a|a["event"]=="actual")).ok_or("actual result absent")?;
   let mut domains=BTreeMap::new();let mut window=None;
   for d in DOMAINS {let scope=sample["status"][format!("{d}_scope")].as_str().ok_or("scope id")?;let r=selected.get(&format!("{source}:{scope}")).ok_or("formal scope absent")?;
    if r["schema"]!=format!("isaac.cs.{d}")||r["coverage"]["status"]!=if d=="mod"&&name=="graph_limits"||d=="random"&&name=="registry_budget"{"Partial"}else{"Complete"}{return Err(format!("formal coverage:{name}:{d}"));}
    let w=json!([r["context"]["authority_tick"],r["context"]["outer"],r["context"]["owner"]]);if let Some(old)=&window{if old!=&w{return Err("domain window mismatch".into());}}else{window=Some(w);}
    domains.insert(d,&r["payload"]);selected_count+=1;
   }
   let run=domains["run"];let random=domains["random"];let presentation=domains["presentation"];let mods=domains["mod"];
   for (a,b) in [(n(&run["state"]["fields"]["Game.GetShopVisits"]),n(&actual["shops"])),(n(&run["probabilities"]["angel_modifier"]["value"]),n(&actual["angel_modifier"])),(n(&run["probabilities"]["planetarium_final"]["value"]),n(&actual["planetarium"])),(n(&run["seeds"]["fields"]["Seeds.GetStartSeed"]),n(&actual["seed"]))]{if a.zip(b).is_none_or(|(a,b)|(a-b).abs()>1e-8){return Err(format!("run source mismatch:{name}:{a:?}/{b:?}"));}}
   if presentation["hud"]["fields"]["HUD.IsVisible"]!=actual["hud_visible"]{return Err(format!("HUD mismatch:{name}"));}
   if name=="registry_budget"&&(random["registry_status"]!="Truncated"||n(&random["registration_failures"]).is_none_or(|v|v<=0.0)){return Err("registry budget was not explicitly reported".into());}
   let pill=random["pills"].as_array().and_then(|p|p.first()).ok_or("pills")?;if pill["identified"]!=actual["pill_identified"]||n(&pill["effect"])!=n(&actual["pill_effect"]){return Err(format!("pill mismatch:{name}"));}
   let pool=random["pools"].as_array().and_then(|p|p.first()).ok_or("pool")?;
   if pool["items"]["entries"].as_array().map(Vec::len)!=n(&actual["pool_size"]).map(|n|n as usize){return Err(format!("pool membership mismatch:{name}"));}
   let state=adapter(mods,"isaac.cs.e2e.tx14");if name!="retire"&&n(&state["revision"])!=n(&actual["revision"]){return Err(format!("mod revision:{name}:{state}"));}
   let stage=adapter(mods,"stageapi");if stage.is_null(){return Err("StageAPI adapter absent".into());}
   if name.starts_with("stage_")||name.starts_with("grid_"){
    actual_types.insert(name.to_string(),actual["native_type"].clone());
    if ["stage_a","stage_return"].contains(&name)&&actual["logical_type"]!="tx14.room.A"||name=="stage_b"&&actual["logical_type"]!="tx14.room.B"{return Err(format!("logical room type:{name}"));}
    let grid_scope=sample["status"]["grid_scope"].as_str().ok_or("native grid scope")?;let native_grids=&selected.get(&format!("{source}:{grid_scope}")).ok_or("native grid scope missing")?["payload"]["objects"];
    if ["stage_b","stage_return"].contains(&name){
      let rooms=stage["rooms"].as_object().ok_or("StageAPI logical rooms")?;
      if rooms.len()<2||rooms.values().any(|r|r["native_descriptor"]["resolution"]!="KnownCurrent"){return Err("logical/native descriptor references incomplete".into());}
    }
    let formal=stage["custom_grids"].as_object().ok_or("StageAPI grid set")?;let expected=actual["stage_grids"].as_array().ok_or("actual grids")?;
    if formal.len()!=expected.len(){return Err(format!("StageAPI membership:{name}"));}
    for e in expected {let matched=formal.values().find(|f|f["subject"]["local_id"]==e["subject"]["local_id"]).ok_or("StageAPI instance id")?;
     if matched["config"]["Name"]!=e["name"]||n(&matched["index"])!=n(&e["index"]){return Err("StageAPI config/index".into());}
     if !e["data_value"].is_null()&&n(&matched["data"]["tx14"])!=n(&e["data_value"]){return Err("CustomGrid Data change absent".into());}
     if n(&matched["persistent_data"]["tx14"])!=n(&e["persist_value"]){return Err("CustomGrid persistent state".into());}
     if (e["carrier"]["resolution"]=="Null"&&matched["native_carrier"]["resolution"]!="Null")||(!e["carrier"]["local_id"].is_null()&&matched["native_carrier"]["local_id"]!=e["carrier"]["local_id"]){return Err("CustomGrid native relation".into());}
     if !e["carrier"]["local_id"].is_null(){
      let native=native_grids.as_array().and_then(|a|a.iter().find(|g|g["subject"]["local_id"]==matched["native_carrier"]["local_id"])).ok_or("carrier absent from canonical native grid membership")?;
      if n(&native["type"])!=n(&matched["config"]["BaseType"])||n(&native["variant"])!=n(&matched["config"]["BaseVariant"]){return Err("carrier type/variant mismatch".into());}
     }
     if name=="grid_rebuild"&&n(&e["index"])==Some(70.0){rebuilt=Some((*source,e["subject"]["local_id"].as_str().ok_or("rebuilt id")?.to_string()));}
     if e["name"]=="tx14.logic.only"&&matched["native_carrier"]["resolution"]!="Null"{return Err("carrierless grid fabricated carrier".into());}
     if n(&e["index"])==Some(70.0){grid_ids.insert(name.to_string(),e["subject"]["local_id"].clone());}
    }
   }
  }
  if seen.len()!=ACTIONS.len(){return Err("unexecuted tx14 scene".into());}
  let (source,id)=rebuilt.ok_or("rebuilt instance missing")?;
  if removals.get(&source).is_none_or(|ids|ids.is_empty()||ids.contains(&id)){return Err("same-process remove/rebuild lifecycle identity missing or reused".into());}
  if actual_types.get("stage_a")!=actual_types.get("stage_b"){return Err("custom room types did not share native RoomType".into());}
  for member in ["RNG.RandomInt","RNG.RandomFloat","RNG.RandomVector","RNG.Next","ItemPool.GetCollectible","ItemPool.AddTemporaryCollectible","ItemPool.AddRoomBlacklist","HUD.ShowItemText","SFXManager.Play"]{if calls.get(member).copied().unwrap_or(0)==0{return Err(format!("real call missing:{member};calls={calls:?}"));}}
  for name in ["POST_SPAWN_CUSTOM_GRID","POST_REMOVE_CUSTOM_GRID","POST_CUSTOM_GRID_UNLOAD","MC_POST_SFX_PLAY"]{if events.get(name).copied().unwrap_or(0)==0{return Err(format!("actual framework/native event absent:{name}"));}}
  if render==0||graph_refs==0{return Err("render or graph reference absent".into());}
  self.case("run_rules_clocks_probabilities",true,json!({"scenes":seen.len(),"selected_scopes":selected_count}));
  self.case("random_streams_selection_pools_pills",true,json!({"actual_calls":calls}));
  self.case("hud_independent_presentation_audio",true,json!({"render_scopes":render,"events":events}));
  self.case("mod_graph_stageapi_lifecycle",true,json!({"graph_references":graph_refs,"grid_ids":grid_ids,"native_types":actual_types}));
  self.case("run_reader_same_window_delivery",true,json!({"sources":summaries,"selected_scopes":selected_count,"selected_delivery_complete":true}));println!("tx14 scenes={} selected_scopes={} render_scopes={} graph_references={}",seen.len(),selected_count,render,graph_refs);Ok(())
 }
}

// 只归约已执行并恢复环境的同tx正式记录，不启动游戏、不重发场景动作。
pub(crate) fn reduce_existing(base:&std::path::Path)->Result<()> {
 let report:Value=serde_json::from_str(&crate::read(&base.join("result.json"))?).map_err(|e|e.to_string())?;
 if report["tx"]!="tx-04-14"||report["final_status"]["session_state"]!="Idle"||(report["global_random"]!=true&&report["cases"]["run_pause_and_optional_recording"]["pass"]!=true)||report["cleanup_errors"].as_array().is_none_or(|a|a.iter().any(|v|!v.is_null())){return Err("reduction requires completed scenes and restored environment".into());}
 let repo=std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR")).parent().unwrap().parent().unwrap().to_path_buf();
 let game=std::path::PathBuf::from(report["environment"]["game"].as_str().ok_or("game root")?);let save=std::path::PathBuf::from(report["environment"]["save_roots"]["documents"].as_str().ok_or("save root")?);
 let mut run=Run{repo,game,out:base.to_path_buf(),save,scenario:Value::Null,report,steps:fs::OpenOptions::new().append(true).open(base.join("steps.jsonl")).map_err(|e|e.to_string())?,deployed:false,fixture_owned:false,saves:None,deployment:vec![]};
 if run.report["global_random"]==true{run.reduce_tx14_global_random()?}else{run.reduce_tx14()?}
 let mut changes=crate::saves::changes(&base.join("save_before"),&base.join("save_after"))?;run.save_fields_tx10(&mut changes)?;
 if changes.as_array().ok_or("save changes")?.iter().any(|v|v["allowed"]!=true){return Err("save changes outside authorized scope".into());}
 let crashes=run.crash_files()?;if crashes.as_object().is_some_and(|o|o.iter().any(|(name,state)|name.to_lowercase().ends_with(".dmp")&&run.report["crash_before"][name]!=*state)){return Err("new native crash dump".into());}
 run.report["save_changes"]=changes;run.report["evidence_error"]=Value::Null;
 run.report["pass"]=json!(true);run.report["failure"]=Value::Null;run.report["reduction_completed_after_restore"]=json!(true);
 crate::write_json(&base.join("result.json"),&run.report)?;println!("tx-04-14 reduction PASS cases={}",run.report["cases"].as_object().map_or(0,|o|o.len()));Ok(())
}
