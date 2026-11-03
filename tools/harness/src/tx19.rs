//! I33：依据F01—F15及具体失败模型编排；I34前不执行。
//! Source只提供真实变化；故障/恶意目标单列，Client始终走正式codec/Scene和原生后态。
use crate::{Control,Result,Run,win32};
use serde_json::{Value,json};
use std::{collections::{BTreeMap,BTreeSet},process::Command,thread,time::{Duration,Instant}};
type Status=BTreeMap<String,String>;
fn num(v:&Value)->u64{v.as_u64().or_else(||v.as_str().and_then(|v|v.parse().ok())).unwrap_or(0)}
fn n(s:&Status,k:&str)->u64{s.get(k).and_then(|v|v.parse().ok()).unwrap_or(0)}
fn drawn(s:&Status)->u64{s.get("client.dispatch.presentation_draw").and_then(|v|v.split(',').next()).and_then(|v|v.parse().ok()).unwrap_or(0)}
fn load(path:&std::path::Path)->Result<Value>{serde_json::from_str(&crate::read(path)?).map_err(|e|e.to_string())}
// 同一Source token的图像只报告可比尺寸及像素差，最终派生draw/分层正确性由I34实际审图。
fn image_pair(source:&std::path::Path,client:&std::path::Path)->Result<Value>{
 let a=std::fs::read(source).map_err(|e|e.to_string())?;let b=std::fs::read(client).map_err(|e|e.to_string())?;
 if a.len()<54||b.len()<54||a.get(18..26)!=b.get(18..26)||a.len()!=b.len(){return Err("Source/Client BMP dimensions differ".into())}
 let(mut changed,mut neutral,mut total,mut error)=(0u64,0u64,0u64,0u64);
 for(x,y)in a[54..].chunks_exact(4).zip(b[54..].chunks_exact(4)){
  total+=1;changed+=u64::from(x[..3]!=y[..3]);neutral+=u64::from(y[..3].iter().all(|v|*v>250)||y[..3].iter().all(|v|*v<5));
  for i in 0..3{error+=u64::from(x[i].abs_diff(y[i]));}
 }
 if total==0||neutral*100>total*98{return Err("Client renderer capture is blank".into())}
 Ok(json!({"pixels":total,"changed_pixels":changed,"mean_RGB_absolute_difference":error as f64/(total*3) as f64,"visual_review":"pending;no automatic equality claim"}))
}
fn objects(v:&Value)->Result<&Vec<Value>>{v["components"]["entity"]["state"]["objects"].as_array().ok_or("Entity objects required".into())}
fn live(v:&Value)->bool{v["common"]["fields"]["Entity.Exists"]!=false}
fn keys(v:&Value)->Result<BTreeSet<(u64,u64)>>{Ok(objects(v)?.iter().filter(|o|live(o)).map(|o|(num(&o["subject"]["target"]["run"]),num(&o["subject"]["target"]["id"]))).collect())}
// Source标签不能替代真实类别/子状态；缺目标后态就中止该次验收，不能无声缩小矩阵。
fn scene_source(spec:&Value,target:&Value,independent:&Value)->Result<()> {
 let label=spec["label"].as_str().ok_or("label")?;let rows=objects(target)?;
 let has=|class:&str|rows.iter().any(|o|o["class"]==class&&live(o));
 let expected=match spec["kind"].as_str().unwrap_or(""){
  "npc"=>vec!["EntityNPC"],"delirium"|"delirium-morph"=>vec!["EntityNPC"],
  "tear-projectile"=>vec!["EntityTear","EntityProjectile"],"bomb"=>vec!["EntityBomb"],"laser"=>vec!["EntityLaser"],
  "knife"|"knife-out"|"knife-return"=>vec!["EntityKnife"],"pickup"|"pickup-cycle"|"pickup-morph"=>vec!["EntityPickup"],
  "familiar"|"familiar-owner"=>vec!["EntityFamiliar"],"effects"=>vec!["EntityEffect"],"slot"=>vec!["EntitySlot"],
  "native-text"=>vec!["NativeText"],"native-dummy"=>vec!["NativeDummy"],"lifted"|"lifted-change"=>vec!["EntityEffect"],_=>vec![]};
 for cl in expected{if !has(cl){return Err(format!("{label} did not produce actual {cl}"))}}
 if spec["kind"]=="slot"&&!rows.iter().any(|o|o["class"]=="EntitySlot"&&o["common"]["fields"]["Entity.Variant"]==spec["variant"]){return Err(format!("{label} actual Slot variant"))}
 if spec["kind"]=="delirium-morph"&&!rows.iter().any(|o|o["special"]["fields"]["EntityDelirium.BossType"]==20){return Err("real Delirium transform not observed".into())}
 if spec["kind"]=="dead"&&!rows.iter().any(|o|live(o)&&o["common"]["fields"]["Entity.IsDead"]==true){return Err("dead-but-existing Source state missing".into())}
 if spec["kind"]=="lifted"||spec["kind"]=="lifted-change"{
  if target["components"]["grid"]["state"]["lifted_descriptors"].as_array().is_none_or(Vec::is_empty){return Err("actual Effect136 full descriptor missing".into())}
 }
 if spec["kind"]=="short"||spec["kind"]=="queries"{
  let phase=if spec["kind"]=="short"{"Lifecycle"}else{"ActualCall"};
  if !independent.as_array().ok_or("independent records")?.iter().any(|m|m["body"]["capture_kind"]==phase){return Err(format!("{label} actual independent {phase} record missing"))}
 }
 Ok(())
}
// 组件局部图：不借其它component相同node，也不把悬空引用从结果删掉。
fn graph(v:&Value)->Result<()> {
 fn walk(v:&Value,defs:&mut BTreeMap<u64,usize>,refs:&mut BTreeSet<u64>){
  if v["type"]=="table"&&!v["node"].is_null(){*defs.entry(num(&v["node"])).or_default()+=1;}
  if v["type"]=="graph_ref"{refs.insert(num(&v["node"]));}
  match v{Value::Object(o)=>for v in o.values(){walk(v,defs,refs)},Value::Array(a)=>for v in a{walk(v,defs,refs)},_=>()}
 }
 let(mut defs,mut refs)=(BTreeMap::new(),BTreeSet::new());walk(v,&mut defs,&mut refs);
 if refs.iter().any(|r|*r==0||defs.get(r)!=Some(&1)){return Err("projection component graph is not closed".into())}Ok(())
}
// 逐成员发送规则不随值变化；所有真实失败、动态层和精确pose仍由原始目标承载。
fn projection(source:&Value,target:&Value,catalog:&Value)->Result<Value>{
 let omitted=catalog["members"].as_array().ok_or("replication catalog")?.iter().filter(|r|r["ownership"]=="NotReplicated")
  .filter_map(|r|r["id"].as_str().map(str::to_owned)).collect::<BTreeSet<_>>();
 let mut retained=BTreeMap::<String,u64>::new();
 fn walk(v:&Value,omitted:&BTreeSet<String>,retained:&mut BTreeMap<String,u64>)->Result<()> {
  match v{Value::Object(o)=>for(k,v)in o{
   if omitted.contains(k){return Err(format!("omitted member leaked: {k}"))}
   if ["Sprite.HasCustomShader","Sprite.HasCustomChampionShader","LayerState.HasCustomShader","LayerState.HasCustomChampionShader","Sprite.GetAllLayers","pose_runtime"].contains(&k.as_str()){*retained.entry(k.clone()).or_default()+=1;}
   walk(v,omitted,retained)?;
  },Value::Array(a)=>for v in a{walk(v,omitted,retained)?},_=>()}Ok(())
 }
 if source["components"]["entity"]!=target["components"]["entity"]{return Err("Entity fixture changed complete formal projection".into())}
 for c in target["components"].as_object().ok_or("components")?.values(){graph(&c["state"])?;walk(&c["state"],&omitted,&mut retained)?;}
 for k in ["Sprite.HasCustomShader","Sprite.HasCustomChampionShader","LayerState.HasCustomShader","LayerState.HasCustomChampionShader","Sprite.GetAllLayers","pose_runtime"]{if !retained.contains_key(k){return Err(format!("required resource input missing: {k}"))}}
 Ok(json!({"omitted_catalog_members":omitted.len(),"retained_inputs":retained,"entity_projection_unchanged":true}))
}
impl Run {
 fn wait_tx19(&mut self,g:&win32::Game,c:&mut Control,label:&str,allow_failure:bool,p:impl Fn(&Status)->bool)->Result<Status>{
  let until=Instant::now()+Duration::from_secs(90);
  loop{let s=self.watch_tx05(g,c,0)?;if p(&s){self.step(label,json!(s))?;return Ok(s)}
   if !allow_failure&&s.get("client.error").is_some_and(|v|!v.is_empty()){return Err(format!("{label} native failure: {s:?}"))}
   if Instant::now()>=until{return Err(format!("{label} timeout: {s:?}"))}thread::sleep(Duration::from_millis(60));}
 }
 fn apply_tx19(&mut self,g:&win32::Game,c:&mut Control,label:&str,begin:bool)->Result<Status>{
  let target=load(&self.out.join(format!("runtime-world/client-targets/{label}.json")))?;let revision=num(&target["revision"]);
  let token=self.report["token"].as_str().ok_or("token")?.to_owned();let mode=if begin{"begin"}else{"target"};
  let accepted=c.command(&format!("cs client_test {token} {mode} {label}"))?;
  if accepted.get("accepted").map(String::as_str)!=Some("1"){return Err(format!("{label} not admitted: {accepted:?}"))}
  let s=self.wait_tx19(g,c,label,false,|s|n(s,"client_world_applied")==1&&n(s,"client.applied_revision")==revision)?;
  if n(&s,"client.non_player_entity_applied")!=1||n(&s,"client.player_support_applied")!=1||n(&s,"client.full_entity_applied")!=0||n(&s,"client.native_committed_domains")&(1<<1)!=0{
   return Err(format!("{label} inaccurate Entity applied range: {s:?}"))
  }
  Ok(s)
 }
 fn readback_tx19(&mut self,c:&mut Control,target:&Value,label:&str)->Result<Value>{
  let token=self.report["token"].as_str().ok_or("token")?;let native=c.command(&format!("cs client_test {token} inspect"))?;
  if native.get("native_checked").map(String::as_str)!=Some("1"){return Err(format!("{label} native check missing"))}
  let rows:Value=serde_json::from_str(native.get("objects").ok_or("native rows")?).map_err(|e|e.to_string())?;
  let actual=rows.as_array().ok_or("native row array")?.iter().map(|r|(num(&r["run"]),num(&r["id"]))).collect::<BTreeSet<_>>();
  if actual!=keys(target)?||actual.len()!=rows.as_array().unwrap().len(){return Err(format!("{label} native inventory differs from complete T"))}
  Ok(rows)
 }
 fn source_tx19(&mut self,g:&win32::Game,w:usize,c:&mut Control,scenes:&[Value])->Result<()> {
  self.sequence(w,"start_run")?;c.wait("run_active","1",30)?;c.command("cs observe start")?;
  c.command(&format!("cs host {} 0",self.scenario["host_port"]))?;c.wait("session_state","Active",30)?;
  let token=self.report["token"].as_str().ok_or("token")?.to_owned();let mut samples=Vec::new();
  for(i,spec)in scenes.iter().enumerate(){let label=spec["label"].as_str().ok_or("scene label")?;
   c.command(&format!("cs client_test {token} sampling capture19"))?;win32::key(w,"F8")?;
   let until=Instant::now()+Duration::from_secs(90);
   let mut restarted_host=false;
   let actual=loop{let current=self.watch_tx05(g,c,40)?;
    if spec["kind"]=="familiar-owner"&&!restarted_host&&current.get("session_state").map(String::as_str)==Some("Idle")&&n(&current,"run_active")==1{
     c.command(&format!("cs host {} 0",self.scenario["host_port"]))?;restarted_host=true;
    }
    let rows=self.live_fixture()?;
    if let Some(e)=rows.iter().find(|e|e["event"]=="fixture_error"){return Err(format!("Source {label}: {e}"))}
    if let Some(e)=rows.iter().find(|e|e["event"]=="ready"&&e["label"]==label&&num(&e["action"])==i as u64+1){break e.clone()}
    if Instant::now()>=until{return Err(format!("Source {label} ready timeout"))}
   };
   let until=Instant::now()+Duration::from_secs(45);
   let exported=loop{let r=c.command(&format!("cs client_test {token} export19 {label}"))?;
    if r.get("exported").map(String::as_str)==Some("1"){break r}
    if Instant::now()>=until{return Err(format!("Source export {label}: {r:?}"))}self.watch_tx05(g,c,60)?;
   };
   if exported.get("capture_token").map(String::as_str)!=actual["capture_token"].as_str(){return Err(format!("{label} Source snapshot changed between ready and export"))}
   let root=self.out.join("runtime-world/client-targets");let raw=load(&root.join(format!("observation-{label}.json")))?;
   let entity=raw.as_array().ok_or("raw observations")?.iter().find(|r|r["schema"]=="isaac.cs.entity").ok_or("raw Entity envelope")?;
   let source_ids=entity["payload"]["objects"].as_array().ok_or("raw entity inventory")?.iter().map(|r|num(&r["subject"]["local_id"])).collect::<BTreeSet<_>>();
   for e in actual["entities"].as_array().ok_or("raw Source inventory")?{let id=num(&e["subject"]["local_id"]);if id==0||!source_ids.contains(&id){return Err(format!("{label} live Source entity missing from formal inventory: {e}"))}}
   let world=c.command("cs world status")?;if world.get("world_state").map(String::as_str)==Some("Recording"){return Err("Source unexpectedly depends on recorder".into())}
   win32::screenshot(w,&self.out.join(format!("source-{label}.bmp")))?;
   samples.push(json!({"label":label,"actual":actual,"export":exported,"recorder":world}));
   crate::write_json(&self.out.join("source-progress.json"),&json!(samples))?;
  }
  // E26：真实writer句柄故障不能切断正式纯值consumer；仅测试录制器自有文件。
  let before=c.command("cs observe status")?;c.command("cs world start tx19-writer-fault")?;
  let until=Instant::now()+Duration::from_secs(30);
  let recording=loop{let s=c.command("cs world status")?;if s.get("world_state").map(String::as_str)==Some("Recording"){break s}
   if Instant::now()>=until{return Err(format!("recorder start timeout: {s:?}"))}self.watch_tx05(g,c,60)?;};
  let generation=n(&recording,"generation");if generation==0{return Err("recorder generation missing".into())}
  c.command(&format!("cs world_test {token} {generation} file 1"))?;win32::key(w,"F9")?;
  let until=Instant::now()+Duration::from_secs(30);
  let failure=loop{self.watch_tx05(g,c,60)?;let s=c.command("cs world status")?;
   if s.get("reason").map(String::as_str)==Some("FILE_ERROR"){break s}
   if Instant::now()>=until{return Err("writer handle fault was not observed".into())}};
  c.command("cs world stop")?;
  let until=Instant::now()+Duration::from_secs(30);
  loop{let s=c.command("cs world status")?;if s.get("world_state").map(String::as_str)==Some("Closed"){break}
   if Instant::now()>=until{return Err("failed recorder did not close".into())}self.watch_tx05(g,c,60)?;}
  let after=c.command("cs observe status")?;
  if after.get("formal_consumer").map(String::as_str)!=Some("1")||n(&after,"publication_count")<=n(&before,"publication_count"){return Err("recorder fault stopped formal consumer".into())}
  let recovery=c.command(&format!("cs client_test {token} export19 recorder-recovery"))?;
  if recovery.get("exported").map(String::as_str)!=Some("1"){return Err(format!("formal export after recorder fault: {recovery:?}"))}
  self.case("E26.recorder_failure",true,json!({"before":before,"failure":failure,"after":after,"recovery":recovery}));
  self.report["source_timings"]=json!(c.command(&format!("cs client_test {token} fault-status"))?);
  c.command("cs stop")?;c.wait("session_state","Idle",30)?;c.command("cs observe stop")?;
  self.return_to_menu(w,c)?;self.report["source_samples"]=json!(samples);
  self.report["acceptance_stage"]=json!("source_ready_not_Client_PASS");Ok(())
 }
 // 只在显式E2E文件内制造恶意T，逐项记录修改位置及写前保A，不伪装自然Source。
 fn negatives_tx19(&mut self,g:&win32::Game,c:&mut Control,base:&Value)->Result<()> {
  let token=self.report["token"].as_str().ok_or("token")?.to_owned();let r=num(&base["revision"]);let before=c.command("cs status")?;
  let invalids=["old-revision","same-revision-conflict","partial","source-version","field-read-failed","field-truncated","shader-read-failed","bad-class","bad-number","bad-bitset","bad-reference","missing-owner","dangling-graph","bad-resource","pool-budget","player-scope"];
  for name in invalids{
   let mut v=base.clone();crate::tx18::revision(&mut v,if name=="old-revision"{r.saturating_sub(1)}else if name=="same-revision-conflict"{r}else{r+1});
   let entity=&mut v["components"]["entity"];
   match name{
    "old-revision"=>(),
    "partial"=>entity["coverage"]=json!("Partial"),
    "source-version"=>entity["state"]["application_source"]["version"]=json!(999),
    "dangling-graph"=>entity["state"]["negative_graph"]=json!({"type":"graph_ref","node":99999999}),
    _=>{let rows=entity["state"]["objects"].as_array_mut().ok_or("negative objects")?;
     let obj=rows.iter_mut().find(|o|if name=="player-scope"{o["class"]=="EntityPlayer"}else{o["class"]!="EntityPlayer"&&live(o)}).ok_or("negative requires actual class")?;
     match name{
      "bad-class"=>obj["class"]=json!("UnknownEntity"),
      "bad-number"=>obj["common"]["fields"]["Entity.Position"]["x"]=json!("nonfinite"),
      "bad-bitset"=>obj["common"]["fields"]["Entity.GetEntityFlags"]=json!({"type":"bits128","low":"broken","high":"0"}),
      "bad-reference"|"missing-owner"=>{let mut x=obj["subject"].clone();x["target"]["id"]=json!("999999999");if name=="bad-reference"{x["target"]["space"]=json!(2);}obj["common"]["fields"]["Entity.Parent"]=x;},
      "field-read-failed"|"field-truncated"=>obj["common"]["fields"]["Entity.Position"]=json!({"status":if name=="field-truncated"{"Truncated"}else{"ReadFailed"},"reason":"E2E injected required field"}),
      "shader-read-failed"=>obj["common"]["fields"]["Entity.GetSprite"]["fields"]["Sprite.HasCustomShader"]=json!({"status":"ReadFailed","reason":"E2E retained shader query failure"}),
      "bad-resource"=>obj["common"]["fields"]["Entity.GetSprite"]["fields"]["Sprite.GetFilename"]=json!("../invalid.anm2"),
      "player-scope"=>obj["body"]["fields"]["EntityPlayer.GetPlayerType"]=json!(20),
      "pool-budget"=>{let template=obj.clone();for i in 0..513{let mut x=template.clone();x["subject"]["target"]["id"]=json!((700000+i).to_string());rows.push(x);}},
      _=>obj["common"]["fields"]["Entity.Position"]["x"]=json!(251),
     }
    }
   }
   let label=format!("negative-{name}");crate::write_json(&self.out.join(format!("runtime-world/client-targets/{label}.json")),&v)?;
   let result=c.request(&format!("cs client_test {token} target {label}"))?;let after=c.command("cs status")?;
   if result.get("accepted").map(String::as_str)==Some("1")||n(&after,"client.applied_revision")!=r||n(&after,"client_world_applied")!=1||n(&after,"client.native_attempt")!=n(&before,"client.native_attempt"){
    return Err(format!("{name} write-before rejection failed: {result:?} {after:?}"))
   }
   self.step(&label,json!({"injection":"E2E pure-value target only","reply":result,"status":after}))?;
  }
  // 版本拒绝走正式Negotiate；既不换旧DLL也不将模型JSON中的未知字段当版本号。
  let caps=self.out.join("runtime-world/client-targets/caps-v2.json");
  let status=Command::new(self.repo.join("out/repentogon/client-fixture.exe")).arg("--capabilities").arg(&caps).output().map_err(|e|e.to_string())?;
  if !status.status.success(){return Err("capability source generation failed".into())}
  let caps=load(&caps)?;
  for name in ["model","catalog"]{let mut old=caps.clone();if name=="model"{old["model"]=json!(1)}else{old["model_categories"]["entity_target"]["catalog"]=json!(0)}
   let label=format!("caps-old-{name}");crate::write_json(&self.out.join(format!("runtime-world/client-targets/{label}.json")),&old)?;
   let reply=c.command(&format!("cs client_test {token} negotiate {label}"))?;if reply.get("accepted").map(String::as_str)!=Some("0"){return Err(format!("{name} compatibility silently accepted"))}
  }
  self.case("E22",true,json!({"invalid_targets":invalids.len(),"compatibility":2,"A_preserved":true}));
  self.watch_tx05(g,c,0)?;Ok(())
 }
 fn failures_tx19(&mut self,g:&win32::Game,c:&mut Control,label:&str)->Result<()> {
  let token=self.report["token"].as_str().ok_or("token")?.to_owned();c.command("cs stop")?;c.wait("role","Off",45)?;
  let points=["acquire-before","acquire-after","init-before","init-after","fields-before","fields-after","resources-before","resources-after","publish-before","publish-after","references-before","references-after","check-before","check-after","draw-before","draw-after"];
  let mut results=Vec::new();
  for point in points{
   c.command(&format!("cs client_test {token} fault {point}"))?;
   let accepted=c.command(&format!("cs client_test {token} begin {label}"))?;if accepted.get("accepted").map(String::as_str)!=Some("1"){return Err(format!("{point} not admitted"))}
   let failed=self.wait_tx19(g,c,point,true,|s|n(s,"client_world_applied")==0&&s.get("client.native_phase").map(String::as_str)==Some("retired"))?;
   let hit=c.command(&format!("cs client_test {token} fault-status"))?;
   if hit.get("hit").map(String::as_str)!=Some(point)||n(&hit,"hits")!=1||n(&failed,"client.full_entity_applied")!=0{return Err(format!("{point} injection did not reach actual prefix: {hit:?} {failed:?}"))}
   // 已写失败后同一tx交付完整T重建，不从旧A接着差分。
   let mut recovery=load(&self.out.join(format!("runtime-world/client-targets/{label}.json")))?;
   let r=num(&recovery["revision"])+1;crate::tx18::revision(&mut recovery,r);
   crate::write_json(&self.out.join("runtime-world/client-targets/fault-recover.json"),&recovery)?;
   self.apply_tx19(g,c,"fault-recover",false)?;self.readback_tx19(c,&recovery,"fault-recover")?;
   results.push(json!({"point":point,"failure":failed,"hit":hit,"recovery_revision":r}));
   c.command("cs stop")?;c.wait("role","Off",45)?;
   crate::write_json(&self.out.join("fault-progress.json"),&json!(results))?;
  }
  self.case("E19",true,json!(results));
  for mode in ["begin-cancel","begin-cancel-native","begin-cancel-grid","begin-cancel-display"]{
   let reply=c.command(&format!("cs client_test {token} {mode} {label}"))?;if reply.get("accepted").map(String::as_str)!=Some("1"){return Err(format!("{mode} rejected"))}c.wait("role","Off",45)?;
  }
  self.case("E21.stop_prefixes",true,json!({"stages":4}));Ok(())
 }
 // 独立Query仍是自己的scope；以真实记录为输入，只修改具名协议条件来覆盖负向路径。
 fn independent_tx19(&mut self,c:&mut Control,base:&Value)->Result<()> {
  let root=self.out.join("runtime-world/client-targets");let token=self.report["token"].as_str().ok_or("token")?.to_owned();
  let source=load(&root.join("independent-queries.json"))?;
  let original=source.as_array().ok_or("independent Source")?.iter().find(|v|v["body"]["capture_kind"]=="ActualCall").ok_or("actual Query Source required")?.clone();
  let mut first=original.clone();first["body"]["required_state"]=base["revision"].clone();
  crate::write_json(&root.join("independent-one.json"),&json!([first]))?;
  let first_reply=c.command(&format!("cs client_test {token} independent one"))?;
  if first_reply.get("accepted").map(String::as_str)!=Some("1"){return Err("actual independent Query was not admitted".into())}
  let before=c.command("cs status")?;let duplicate=c.command(&format!("cs client_test {token} independent one"))?;let after=c.command("cs status")?;
  if duplicate.get("accepted").map(String::as_str)!=Some("1")||n(&before,"client.entity_independent_pending")!=n(&after,"client.entity_independent_pending")||n(&after,"client.entity_independent_presented")!=0{return Err("independent duplicate replayed/acknowledged".into())}
  for name in ["stale-scope","sequence-conflict","wrong-domain","partial"]{
   let mut bad=first.clone();match name{
    "stale-scope"=>bad["body"]["scope"]["scene"]=json!((num(&base["scope"]["scene"])+1).to_string()),
    "sequence-conflict"=>bad["body"]["value"]["negative_marker"]=json!("E2E explicit sequence conflict"),
    "wrong-domain"=>bad["body"]["domain"]=json!(2),_=>bad["body"]["coverage"]=json!("Partial")}
   let label=format!("negative-{name}");crate::write_json(&root.join(format!("independent-{label}.json")),&json!([bad]))?;
   let reply=c.command(&format!("cs client_test {token} independent {label}"))?;let s=c.command("cs status")?;
   if reply.get("accepted").map(String::as_str)!=Some("0")||n(&s,"client.applied_revision")!=num(&base["revision"])||n(&s,"client_world_applied")!=1||n(&s,"client.entity_independent_presented")!=0{return Err(format!("independent {name} altered ordinary A or acknowledged result"))}
  }
  self.case("E17.independent",true,json!({"source":"actual Source Query;explicit protocol mutations","duplicate":duplicate,"not_presented":true,"ordinary_state_preserved":true}));Ok(())
 }
 pub(super) fn scenes_tx19(&mut self,g:&win32::Game)->Result<()> {
  // 显式product清单与编译说明共同覆盖发布边界，E2E DLL保存在另一输出目录。
  let product=self.out.join("product-package");
  let packaged=Command::new("pwsh").args(["-NoProfile","-File"]).arg(self.repo.join("scripts/package.ps1")).arg("-OutputDirectory").arg(&product).output().map_err(|e|e.to_string())?;
  if !packaged.status.success(){return Err(format!("product package failed: {}",String::from_utf8_lossy(&packaged.stderr)))}
  let mut files=Vec::new();crate::tx18::product_files(&product,&product,&mut files)?;
  if files.iter().any(|p|p.to_string_lossy().contains("isaac-cs-test")||p.extension().is_some_and(|v|v=="exe")){return Err("E2E files leaked into product".into())}
  let flavor=load(&product.join("Repentogon/isaac-cs/build-flavor.json"))?;let config=load(&product.join("Repentogon/isaac-cs/config.json"))?;
  if flavor["flavor"]!="product"||flavor["e2e_tools"]!=false||config["enabled"]!=false||config["world_record"]["enabled"]!=false||config["diagnostics"]["enabled"]!=false||config["control_port"]["enabled"]!=false||!config["world_record"]["e2e_token"].is_null(){return Err("product flavor/default config".into())}
  self.case("E26.product",true,json!({"files":files,"flavor":flavor,"config":config}));
  win32::set_auto_focus(true);let w=g.wait_window(90)?;
  let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("control port")? as u16,g,90,&self.out.join("control.jsonl"))?;c.focus_window=Some(w);
  self.prepare_new_run_tx05(g,w,&mut c)?;self.report["node_role"]=self.scenario["node_role"].clone();
  let scenes=self.scenario["scenes"].as_array().ok_or("scenes")?.clone();
  if self.scenario["node_role"]=="Source"{self.source_tx19(g,w,&mut c,&scenes)?;drop(c);if g.close()?!=0{return Err("Source abnormal exit".into())}return Ok(())}
  let source=std::path::PathBuf::from(self.scenario["source_run"].as_str().ok_or("Source run")?);let report=load(&source.join("result.json"))?;
  if report["tx"]!="tx-04-19"||report["node_role"]!="Source"||report["pass"]!=true{return Err("completed tx19 Source required".into())}
  let samples=report["source_samples"].as_array().ok_or("source samples")?;if samples.len()!=scenes.len(){return Err("Source scene count".into())}
  crate::copy_tree(&source.join("runtime-world/client-targets"),&self.out.join("runtime-world/client-targets"))?;
  self.report["source_run"]=json!(source);self.snapshot("client_before")?;
  let catalog=load(&self.repo.join("out/repentogon/replication-catalog.json"))?;
  let token=self.report["token"].as_str().ok_or("token")?.to_owned();let mut session=Value::Null;let mut applied=Vec::new();let mut classes=BTreeSet::new();let mut slots=BTreeSet::new();let mut previous_players=BTreeSet::new();
  for(i,spec)in scenes.iter().enumerate(){let label=spec["label"].as_str().ok_or("label")?;if samples[i]["label"]!=label{return Err("Source/Client sequence mismatch".into())}
   let root=self.out.join("runtime-world/client-targets");let target=load(&root.join(format!("{label}.json")))?;
   let source=load(&root.join(format!("source-full-{label}.json")))?;let audit=projection(&source,&target,&catalog)?;
   let player_keys=objects(&target)?.iter().filter(|o|o["class"]=="EntityPlayer").map(|o|(num(&o["subject"]["target"]["run"]),num(&o["subject"]["target"]["id"]))).collect::<BTreeSet<_>>();
   if spec["kind"]=="familiar-owner"&&player_keys==previous_players{return Err("Familiar owner did not actually change Player Key".into())}previous_players=player_keys;
   let actual_independent=load(&root.join(format!("independent-{label}.json")))?;scene_source(spec,&target,&actual_independent)?;
   // 独立音画消费属于27；不能伪确认来腾出队列。必要时正常退休整个场景，显式记录新的冷重建。
   let rotate=i>0&&n(&c.command("cs status")?,"client.entity_independent_pending")>64;
   if i>0&&(session!=target["scope"]["session"]||rotate){c.command("cs stop")?;c.wait("role","Off",45)?;}
   let begin=i==0||session!=target["scope"]["session"]||rotate;session=target["scope"]["session"].clone();
   let clock=Instant::now();let result=self.apply_tx19(g,&mut c,label,begin)?;let apply_ms=clock.elapsed().as_secs_f64()*1000.0;
   let readback=self.readback_tx19(&mut c,&target,label)?;
   let draw_clock=Instant::now();self.wait_tx19(g,&mut c,&format!("draw-{label}"),false,|s|drawn(s)>=drawn(&result)+2)?;
   let counters=c.command("cs counters")?;for name in ["start_game","game_update","room_update","entity_list","odd_helper","restore","post_update","player_update","join_pass"]{
    if n(&result,&format!("client.original_begin.{name}"))!=n(&counters,&format!("hook.{name}")){return Err(format!("{label} Client gameplay leaked: {name}"))}
   }
   for o in objects(&target)?.iter().filter(|o|live(o)){classes.insert(o["class"].as_str().ok_or("class")?.to_owned());if o["class"]=="EntitySlot"{slots.insert(num(&o["common"]["fields"]["Entity.Variant"]));}}
   let independent=c.command(&format!("cs client_test {token} independent {label}"))?;if independent.get("accepted").map(String::as_str)!=Some("1")&&independent.get("reason").map(String::as_str)!=Some("entity_independent_backpressure"){return Err(format!("{label} independent record refused: {independent:?}"))}
   // 有界背压是明确的负向覆盖，未接纳后缀仍在Source自有值里，不标成播放/完成。
   if independent.get("accepted").map(String::as_str)==Some("0"){self.step("independent-backpressure",json!({"label":label,"reply":independent,"unconsumed_owner":"tx-04-27"}))?;}
   let after_independent=c.command("cs status")?;if n(&after_independent,"client.entity_independent_presented")!=0{return Err("independent result falsely acknowledged as displayed".into())}
   let repeats=if label=="dense"{20}else{2};for _ in 0..repeats{let duplicate=self.apply_tx19(g,&mut c,label,false)?;
    for k in ["client.native_attempt","client.native_executed_prefix","client.resource_candidate_loads"]{if n(&duplicate,k)!=n(&result,k){return Err(format!("{label} duplicate rebuilt owners: {k}"))}}
   }
   if n(&result,"client.renderer_window")!=w as u64{return Err("capture/native renderer window mismatch".into())}
   let screenshot=self.out.join(format!("client-{label}.bmp"));win32::screenshot(w,&screenshot)?;
   let pixels=image_pair(&std::path::PathBuf::from(self.scenario["source_run"].as_str().ok_or("Source path")?).join(format!("source-{label}.bmp")),&screenshot)?;
   let metrics=load(&root.join(format!("metrics-{label}.json")))?;let timings=c.command(&format!("cs client_test {token} fault-status"))?;
   applied.push(json!({"label":label,"status":result,"native":readback,"projection":audit,"source_sizes":metrics,
    "application_wall_ms_including_IPC":apply_ms,"two_draw_windows_wall_ms":draw_clock.elapsed().as_secs_f64()*1000.0,
    "memory":win32::process_memory(g.pid())?,"cold_rebuild_for_independent_budget":rotate,"native_timings":timings,"pixels":pixels,"independent":independent,"visual_review":"pending"}));
   crate::write_json(&self.out.join("client-progress.json"),&json!(applied))?;
  }
  for cl in ["EntityNPC","EntityTear","EntityProjectile","EntityBomb","EntityLaser","EntityKnife","EntityPickup","EntityFamiliar","EntityEffect","EntitySlot","NativeText","NativeDummy"]{if !classes.contains(cl){return Err(format!("Source real category missing: {cl}"))}}
  if slots!=(1..=18).collect(){return Err(format!("actual Slot variants incomplete: {slots:?}"))}
  let _recording_independent=self.apply_tx19(g,&mut c,"recorder-recovery",false)?;
  let recorded=load(&self.out.join("runtime-world/client-targets/recorder-recovery.json"))?;self.readback_tx19(&mut c,&recorded,"recorder-recovery")?;
  self.case("E26",true,json!({"formal_after_writer_failure_applied":true,"recording_off_scenes":scenes.len()}));
  let base=load(&self.out.join("runtime-world/client-targets/resources.json"))?;
  // 写前拒绝以同一scope的当前完整T为A，避免旧revision拒绝掩盖真正缺陷。
  c.command("cs stop")?;c.wait("role","Off",45)?;self.apply_tx19(g,&mut c,"resources",true)?;self.negatives_tx19(g,&mut c,&base)?;self.independent_tx19(&mut c,&base)?;
  let ownership=c.command(&format!("cs client_test {token} exercise ownership"))?;if ownership.get("ownership_exercise").map(String::as_str)!=Some("1"){return Err("ownership exercise".into())}
  self.failures_tx19(g,&mut c,"resources")?;
  let active=self.apply_tx19(g,&mut c,"resources",true)?;let before=c.command("cs status")?;
  c.command(&format!("cs client_test {token} exercise native"))?;let after=c.command("cs status")?;
  if n(&after,"persist_client_denied_native")<=n(&before,"persist_client_denied_native")||n(&after,"persist_client_denied_lua")<n(&before,"persist_client_denied_lua")+6{return Err("whole-window progress isolation not observed".into())}
  win32::key(w,"ESC")?;self.wait_tx19(g,&mut c,"paused-display",false,|s|n(s,"client.ui_blocked")==1)?;
  win32::key(w,"ESC")?;self.wait_tx19(g,&mut c,"resumed-display",false,|s|n(s,"client.ui_blocked")==0)?;
  self.case("E20",true,json!({"before":before,"after":after,"active":active}));
  c.command("cs stop")?;c.wait("role","Off",45)?;let files=self.files_tx18("client_after")?;
  self.apply_tx19(g,&mut c,"resources",true)?;drop(c);if g.close()?!=0{return Err("Client abnormal exit".into())}
  let exit=self.files_tx18("client_exit")?;self.case("E21",true,json!({"stop":files,"shutdown":exit}));
  self.report["applied_samples"]=json!(applied);self.report["actual_classes"]=json!(classes);self.report["actual_slot_variants"]=json!(slots);
  // 单场景标签不等于E项全部通过；像素、特殊形态几何、独立事件和18联合结果留给I34实际审阅。
  self.report["acceptance_stage"]=json!("automatic_checks_complete_visual_and_matrix_review_pending");
  self.report["unreviewed_matrix"] = json!((1..=26).filter(|i|![19,20,21,22,26].contains(i)).map(|i|format!("E{i:02}")).collect::<Vec<_>>());
  Ok(())
 }
}
