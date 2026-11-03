//! tx-04-15统一E2E：新采集只覆盖复制链路，不重跑09—14已通过玩法矩阵。
use crate::{Control,Result,Run,win32,remote::Remote};
use serde_json::{Value,json};
use std::{collections::{BTreeMap,BTreeSet},fs,io::{BufRead,BufReader},path::Path,process::Command,thread,time::{Duration,Instant}};
fn number(s:&BTreeMap<String,String>,k:&str)->u64{s.get(k).and_then(|v|v.parse().ok()).unwrap_or(0)}
// 第二语言独立读取规范的类型标签和小端数值；不调用C++解码或对比摘要。
struct Wire<'a>{b:&'a[u8],p:usize,nodes:usize}
impl Wire<'_>{
 fn u(&mut self,n:usize)->Result<u64>{let end=self.p.checked_add(n).ok_or("offset overflow")?;let b=self.b.get(self.p..end).ok_or("short wire")?;self.p=end;Ok(b.iter().enumerate().fold(0,|a,(i,b)|a|u64::from(*b)<<(8*i)))}
 fn string(&mut self)->Result<String>{let n=self.u(4)? as usize;if n>2*1024*1024{return Err("string budget".into())}let end=self.p.checked_add(n).ok_or("offset overflow")?;let s=std::str::from_utf8(self.b.get(self.p..end).ok_or("short string")?).map_err(|e|e.to_string())?.to_string();self.p=end;Ok(s)}
 fn value(&mut self,depth:usize)->Result<Value>{self.nodes+=1;if depth>64||self.nodes>1048576{return Err("value budget".into())}match self.u(1)?{
  0=>Ok(Value::Null),1=>Ok(json!(false)),2=>Ok(json!(true)),3=>Ok(json!(self.u(8)?)),4=>Ok(json!(self.u(8)? as i64)),5=>{let v=f64::from_bits(self.u(8)?);if !v.is_finite(){return Err("nonfinite".into())}Ok(json!(v))},6=>Ok(json!(self.string()?)),
  7=>{let count=self.u(4)? as usize;if count>1048576-self.nodes{return Err("array budget".into())}let mut a=Vec::new();for _ in 0..count{a.push(self.value(depth+1)?)}Ok(Value::Array(a))},
  8=>{let count=self.u(4)? as usize;if count>(1048576-self.nodes)/2{return Err("object budget".into())}let mut o=serde_json::Map::new();for _ in 0..count{let key=self.string()?;self.nodes+=1;if o.contains_key(&key){return Err("duplicate wire key".into())}o.insert(key,self.value(depth+1)?);}Ok(Value::Object(o))},_=>Err("unknown tag".into())}}
}
fn independent_decode(root:&Path)->Result<Value>{
 fn difference(a:&Value,b:&Value,path:&str)->Option<String>{
  if a==b{return None}if let (Some(a),Some(b))=(a.as_object(),b.as_object()){for (k,v) in a{if let Some(d)=difference(v,b.get(k).unwrap_or(&Value::Null),&format!("{path}.{k}")){return Some(d)}}}
  if let (Some(a),Some(b))=(a.as_array(),b.as_array()){for (i,(a,b)) in a.iter().zip(b).enumerate(){if let Some(d)=difference(a,b,&format!("{path}[{i}]")){return Some(d)}}}
  Some(format!("{path}:wire={a};text={b}"))
 }
 let file=fs::File::open(root.join("expected.jsonl")).map_err(|e|e.to_string())?;let mut count=0;let mut kinds=BTreeSet::new();let mut bytes=0;
 for line in BufReader::new(file).lines(){let row:Value=serde_json::from_str(&line.map_err(|e|e.to_string())?).map_err(|e|e.to_string())?;let name=row["file"].as_str().ok_or("wire file")?;
  if Path::new(name).components().count()!=1{return Err("wire path".into())}let b=fs::read(root.join(name)).map_err(|e|e.to_string())?;if b.len()>32*1024*1024||b.get(..4)!=Some(b"ICSR"){return Err("header".into())}let mut r=Wire{b:&b,p:4,nodes:0};
  if r.u(2)?!=1||r.u(1)?!=row["kind"].as_u64().ok_or("kind")?{return Err("version kind".into())}let lane=r.u(1)?;let kind=row["kind"].as_u64().unwrap();let expected_lane=match kind{7|12=>1,3=>3,4|11=>4,5=>5,_=>2};
  if lane!=expected_lane||r.u(4)?!=0||r.u(4)? as usize!=b.len()-24||r.u(8)?.to_string()!=row["sequence"].as_str().ok_or("sequence")?{return Err("header fields".into())}
  let decoded=r.value(0)?;if r.p!=b.len()||decoded!=row["body"]{return Err(format!("independent decoder mismatch:{name}:{}",difference(&decoded,&row["body"],"body").unwrap_or_else(||format!("length:{}:{}",r.p,b.len()))))}count+=1;bytes+=b.len();kinds.insert(kind);
 }
 if count<10||![1,3,4,5,6,7,8,11].iter().all(|k|kinds.contains(k)){return Err("missing wire categories".into())}
 Ok(json!({"messages":count,"bytes":bytes,"kinds":kinds,"pass":true}))
}
// 独立核对投影：逐叶比较真实源值与复制值；身份单独检查稳定性／唯一性，归属按产品目录逐项读取。
fn audit_projection(repo:&Path,capture:&Path,root:&Path)->Result<Value>{
 let catalog:Value=serde_json::from_str(&crate::read(&repo.join("out/repentogon/replication-catalog.json"))?).map_err(|e|e.to_string())?;
 let mut policies=BTreeMap::new();for row in catalog["members"].as_array().ok_or("catalog members")?{let id=row["id"].as_str().ok_or("catalog id")?.to_string();if !policies.contains_key(&id)||row["source_catalog"]=="entity-catalog.json"{policies.insert(id,row["ownership"].as_str().ok_or("ownership")?.to_string());}}
 let mut wanted=BTreeMap::new();let mut models=Vec::new();
 for line in BufReader::new(fs::File::open(root.join("expected.jsonl")).map_err(|e|e.to_string())?).lines(){let row:Value=serde_json::from_str(&line.map_err(|e|e.to_string())?).map_err(|e|e.to_string())?;if row["file"].as_str().is_some_and(|s|s.starts_with("real-")&&s!="real-delta.icsr")&&row["kind"]==3{let model=row["body"].clone();for (domain,c) in model["components"].as_object().ok_or("components")?{wanted.insert(c["capture"]["scope"].as_str().ok_or("source scope")?.to_string(),(domain.clone(),c.clone()));}models.push(model);}}
 fn compare(source:&Value,target:&Value,policy:&BTreeMap<String,String>,identities:&mut BTreeMap<String,Value>,leaves:&mut u64,path:&str)->Result<()> {
  if let Some(o)=source.as_object(){
   // 模型v2按目录省略成员，NotScheduled只传状态；失败对象仍逐叶比较。
   if source["status"]=="NotScheduled"{if target!=&json!({"status":"NotScheduled"}){return Err(format!("scheduled placeholder:{path}"))}*leaves+=1;return Ok(())}
   if o.contains_key("space")&&o.contains_key("local_id"){
    if target["type"]!="reference"{return Err(format!("reference projection:{path}"))}let status=source["resolution"].as_str().unwrap_or_else(||match source["presence"].as_str(){Some("NotApplicable")=>"NotApplicable",Some("Unknown")=>"Unknown",_=>"KnownCurrent"});if target["status"]!=status{return Err(format!("reference status:{path}"))}
    if target["target"].is_object(){let id=format!("{}:{}:{}",source["space"],source["local_id"],source["use"].as_str().unwrap_or(""));if let Some(old)=identities.insert(id,target["target"].clone()){if old!=target["target"]{return Err(format!("unstable network identity:{path}"))}}
     if target["target"]["space"]!=source["space"]||target["target"]["id"].as_str().and_then(|s|s.parse::<u64>().ok()).is_none_or(|n|n==0){return Err("network identity domain/id".into())}}
    for k in ["control_relation","control_group_membership"]{if o.contains_key(k){compare(&source[k],&target[k],policy,identities,leaves,&format!("{path}.{k}"))?}}*leaves+=1;return Ok(())
   }
   for (key,value) in o{
    if ["session","epoch"].contains(&key.as_str()){continue}
    // table定义的无引用编号允许被投影裁掉；graph_ref的目标编号仍必须相等。
    if key=="node"&&source["type"]=="table"&&target.get(key).is_none(){continue}
    let at=format!("{path}.{key}");
    if key=="fields"{for (member,value) in value.as_object().ok_or("source fields")?{
      let ownership=policy.get(member).map(String::as_str).unwrap_or("Unsupported");if ownership=="NotReplicated"{if target[key].get(member).is_some(){return Err(format!("omitted field transmitted:{at}.{member}"))}*leaves+=1;}
      else if ownership!="ReplicatedState"{if target[key][member]["status"]!=ownership{return Err(format!("field ownership:{at}.{member}"))}*leaves+=1;}
      else{compare(value,&target[key][member],policy,identities,leaves,&format!("{at}.{member}"))?}}
    }else if policy.get(key).is_some_and(|s|s=="NotReplicated"){if target.get(key).is_some(){return Err(format!("omitted member transmitted:{at}"))}}
    else if policy.get(key).is_some_and(|s|s!="ReplicatedState"){if target[key]["status"]!=policy[key]{return Err(format!("ownership:{at}"))}}
    else if key=="type"&&value=="Sprite"{if target[key]!="resource_pose"||target["resource"]["status"]!="Unverified"{return Err("resource pose descriptor".into())}}
    else if ["participant_local_id","control_group_local_id","floor","dimension","descriptor","room_instance","run"].contains(&key.as_str())&&value.as_str().is_some_and(|s|s!="0") {if !target[key].is_object()||target[key]["id"].as_str().is_none(){return Err(format!("related identity:{at}"))}*leaves+=1;}
    else{compare(value,&target[key],policy,identities,leaves,&at)?}
   }return Ok(())
  }
  if let Some(a)=source.as_array(){let b=target.as_array().ok_or_else(||format!("array lost:{path}"))?;if a.len()!=b.len(){return Err(format!("array membership:{path}"))}for (i,(a,b)) in a.iter().zip(b).enumerate(){compare(a,b,policy,identities,leaves,&format!("{path}[{i}]"))?}return Ok(())}
  if source!=target{return Err(format!("source value mismatch:{path}: {source} != {target}"))}*leaves+=1;Ok(())
 }
 let mut identities=BTreeMap::new();let mut leaves=0;let mut scopes=0;
 for line in BufReader::new(fs::File::open(capture.join("records.jsonl")).map_err(|e|e.to_string())?).lines(){let row:Value=serde_json::from_str(&line.map_err(|e|e.to_string())?).map_err(|e|e.to_string())?;let Some(scope)=row["context"]["scope"].as_str()else{continue};let Some((domain,target))=wanted.remove(scope)else{continue};
  if target["coverage"]!=row["coverage"]["status"]||target["capture"]["authority_tick"]!=row["context"]["authority_tick"]{return Err("source coverage/time mismatch".into())}
  if domain=="random"{if target["state"]["status"]!="AuthorityOnly"{return Err("gameplay RNG replicated".into())}}
  else{compare(&row["payload"],&target["state"],&policies,&mut identities,&mut leaves,&domain)?}scopes+=1;
 }
 if !wanted.is_empty()||scopes<33||leaves<1000{return Err("missing source projection coverage".into())}
 if !models.iter().any(|m|m["components"]["player"]["state"]["control_groups"].as_array().is_some_and(|a|a.len()==4)&&m["components"]["player"]["state"]["bodies"].as_array().is_some_and(|a|a.len()>=5)){return Err("actual four-participant multi-body model absent".into())}
 Ok(json!({"pass":true,"source_scopes":scopes,"compared_leaves":leaves,"identity_aliases":identities.len()}))
}
impl Run {
 fn samples_tx15(&mut self,g:&win32::Game,c:&mut Control,count:u64)->Result<()> {
  let before=c.command("cs observe status")?;let end=Instant::now()+Duration::from_secs(30);
  loop{self.watch_tx05(g,c,0)?;let s=c.command("cs observe status")?;
   if number(&s,"lua_attempts")>=number(&before,"lua_attempts")+count{
    for domain in ["entity","grid","room","level","exploration","room_save","player","run","random","presentation","mod"]{if s.get(&format!("{domain}_coverage")).map(String::as_str)!=Some("Complete"){return Err(format!("tx15 source {domain}:{s:?}"))}}
    self.step("replica_source_window",json!(s))?;return Ok(())}
   if Instant::now()>=end{return Err(format!("source deadline:{s:?}"))}thread::sleep(Duration::from_millis(80));}
 }
 fn action_tx15(&mut self,g:&win32::Game,c:&mut Control,w:usize,index:u64)->Result<()> {
  win32::key(w,"F8")?;let end=Instant::now()+Duration::from_secs(30);loop{self.watch_tx05(g,c,0)?;let rows=self.live_fixture()?;if let Some(e)=rows.iter().find(|r|r["event"]=="fixture_error"){return Err(e.to_string())}
   if rows.iter().any(|r|r["event"]=="ready"&&r["action"]==index){return Ok(())}if Instant::now()>=end{return Err("fixture deadline".into())}thread::sleep(Duration::from_millis(80));}
 }
 pub(super) fn scenes_tx15(&mut self,g:&win32::Game)->Result<()> {
  let w=g.wait_window(90)?;let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("port")? as u16,g,90,&self.out.join("control.jsonl"))?;c.focus_window=Some(w);
  self.prepare_new_run_tx05(g,w,&mut c)?;self.sequence(w,"start_run")?;c.wait("run_active","1",30)?;self.action_tx15(g,&mut c,w,1)?;
  c.command("cs world start replica-contract")?;self.wait_tx14(g,&mut c,"cs world status",|s|s.get("world_state").is_some_and(|s|s=="Recording"))?;c.command("cs observe start")?;
  c.command(&format!("cs host {} 0",self.scenario["host_port"]))?;c.wait("session_state","Active",30)?;
  let mut remotes=Vec::new();for ordinal in 1..=3{if ordinal==1{c.command("cs role 1 19")?;}let remote=Remote::start_participant(format!("127.0.0.1:{}",self.scenario["host_port"]).parse().map_err(|e|format!("{e}"))?,&self.out.join(format!("remote-{ordinal}.jsonl")),30000,2,ordinal,|_|Ok(()))?;
   self.wait_tx14(g,&mut c,"cs status",|s|s.get(&format!("participant.{ordinal}.bound")).is_some_and(|v|v=="1"))?;remotes.push(remote);}
  self.samples_tx15(g,&mut c,2)?;self.action_tx15(g,&mut c,w,2)?;self.samples_tx15(g,&mut c,2)?;self.action_tx15(g,&mut c,w,3)?;self.samples_tx15(g,&mut c,2)?;
  c.command("cs world stop")?;self.wait_tx14(g,&mut c,"cs world status",|s|s.get("world_state").is_some_and(|s|s=="Closed"))?;
  for remote in &remotes{remote.bye()?;}drop(remotes);c.command("cs stop")?;c.wait("session_state","Idle",30)?;c.command("cs observe stop")?;
  self.return_to_menu(w,&mut c)?;self.report["final_status"]=json!(c.command("cs status")?);drop(c);if g.close()?!=0{return Err("game exit".into())}self.report["tx15_scenes_completed"]=json!(true);
  self.reduce_tx15()
 }
 pub(super) fn reduce_tx15(&mut self)->Result<()> {
  let capture=self.out.join("runtime-world/replica-contract/observation-v2");let read=crate::observation::read_capture(&capture)?;if read["delivery_complete"]!=true||read["supported_read_complete"]!=true{return Err(format!("source delivery:{read}"))}
  let root=self.out.join("wire");
  if self.report["cases"]["all_domain_model_and_contract"]["pass"]!=true||self.scenario["retest_contract"]==true {
   let output=Command::new(self.repo.join("out/repentogon/replica-contract.exe")).arg(capture.join("records.jsonl")).arg(&root).output().map_err(|e|e.to_string())?;
   if !output.status.success(){return Err(format!("contract codec:{}{}",String::from_utf8_lossy(&output.stdout),String::from_utf8_lossy(&output.stderr)))}
   let summary:Value=serde_json::from_slice(&output.stdout).map_err(|e|e.to_string())?;self.case("all_domain_model_and_contract",summary["pass"]==true,summary);
  }
  // 先保存本次已通过的独立组；后续仅修归约器时不重复codec和实机动作。
  crate::write_json(&self.out.join("result.json"),&self.report)?;
  if self.report["cases"]["independent_rust_decode"]["pass"]!=true {let independent=independent_decode(&root)?;self.case("independent_rust_decode",true,independent);crate::write_json(&self.out.join("result.json"),&self.report)?;}
  let audit=audit_projection(&self.repo,&capture,&root)?;self.case("source_projection_and_identity",true,audit);self.case("actual_observation_delivery",true,read);Ok(())
 }
}
// 编解码／离线归约修复后只使用已恢复环境的实际样本，不重复启动游戏。
pub(crate) fn reduce_existing(base:&Path,retest_contract:bool)->Result<()> {
 let report:Value=serde_json::from_str(&crate::read(&base.join("result.json"))?).map_err(|e|e.to_string())?;
 if report["tx"]!="tx-04-15"||report["tx15_scenes_completed"]!=true||report["cleanup_errors"].as_array().is_none_or(|a|a.iter().any(|v|!v.is_null())){return Err("requires completed scenes and restored environment".into())}
 let repo=std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR")).parent().unwrap().parent().unwrap().to_path_buf();let game=std::path::PathBuf::from(report["environment"]["game"].as_str().ok_or("game")?);let save=std::path::PathBuf::from(report["environment"]["save_roots"]["documents"].as_str().ok_or("save")?);
 let mut run=Run{repo,game,out:base.to_path_buf(),save,scenario:json!({"retest_contract":retest_contract}),report,steps:fs::OpenOptions::new().append(true).open(base.join("steps.jsonl")).map_err(|e|e.to_string())?,deployed:false,fixture_owned:false,saves:None,deployment:vec![]};
 if retest_contract {run.report["cases"]=json!({});}
 run.reduce_tx15()?;let mut changes=crate::saves::changes(&base.join("save_before"),&base.join("save_after"))?;run.save_fields_tx10(&mut changes)?;if changes.as_array().ok_or("save changes")?.iter().any(|v|v["allowed"]!=true){return Err("save changes outside scope".into())}
 let crashes=run.crash_files()?;if crashes.as_object().is_some_and(|o|o.iter().any(|(n,v)|n.to_lowercase().ends_with(".dmp")&&run.report["crash_before"][n]!=*v)){return Err("new crash dump".into())}
 run.report["save_changes"]=changes;run.report["pass"]=json!(true);run.report["failure"]=Value::Null;run.report["evidence_error"]=Value::Null;crate::write_json(&base.join("result.json"),&run.report)?;println!("tx-04-15 reduction PASS {}",run.report["cases"]);Ok(())
}
