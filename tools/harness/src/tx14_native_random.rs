//! 剩余随机源的同tx真实进程验收；只归约实际副本，不在验证期间向游戏额外取随机数。
use crate::{Control,Result,Run,win32,remote::Remote};
use serde_json::{Value,json};
use std::{collections::{BTreeMap,BTreeSet},fs,io::{BufRead,BufReader}};
fn u(v:&Value)->Result<u64>{v.as_u64().or_else(||v.as_str().and_then(|s|s.parse().ok())).ok_or_else(||format!("unsigned:{v}"))}
fn signed(v:&Value)->Result<i64>{v.as_i64().or_else(||v.as_str().and_then(|s|s.parse().ok())).or_else(||v["integer"].as_str().and_then(|s|s.parse().ok())).ok_or_else(||format!("signed:{v}"))}
fn w(v:&Value)->Result<Vec<u32>>{v.as_array().ok_or("words array")?.iter().map(|v|u(v).map(|v|v as u32)).collect()}
fn xs(a:&mut [u32])->Result<u32>{
 if a.len()!=4||a[1]>=32||a[2]>=32||a[3]>=32{return Err("xorshift state".into())}
 let mut x=a[0];x^=x>>a[1];x^=x.wrapping_shl(a[2]);x^=x>>a[3];a[0]=x;Ok(x)
}
fn mt_seed(seed:u32)->Vec<u32>{let mut w=vec![0;625];w[0]=seed;for i in 1..624{w[i]=1812433253u32.wrapping_mul(w[i-1]^(w[i-1]>>30)).wrapping_add(i as u32)}w[624]=624;w}
fn temper(mut x:u32)->u32{x^=x>>11;x^=(x<<7)&0x9d2c5680;x^=(x<<15)&0xefc60000;x^(x>>18)}
fn mt(a:&mut Vec<u32>)->Result<u32>{
 if a.len()!=625||a[624]>625{return Err("native MT state".into())}
 if a[624]==625{*a=mt_seed(5489)}
 if a[624]==624{for i in 0..624{let y=(a[i]&0x80000000)|(a[(i+1)%624]&0x7fffffff);a[i]=a[(i+397)%624]^(y>>1)^if y&1!=0{0x9908b0df}else{0}}a[624]=0;}
 let index=a[624] as usize;a[624]+=1;Ok(temper(a[index]))
}
// MSVC引擎完整双缓冲状态：index在首字，1248字在后，尾部为Dx掩码。
fn msvc_mt(a:&mut [u32])->Result<u32>{
 if a.len()!=1250||a[0]>1248||a[1249]!=u32::MAX{return Err("MSVC MT state".into())}
 if a[0]==624{
  for i in 624..1248{let y=(a[1+i-624]&0x80000000)|(a[1+i-623]&0x7fffffff);a[1+i]=a[1+i-227]^(y>>1)^if y&1!=0{0x9908b0df}else{0};}
 }else if a[0]>=1248{
  for i in 0..624{let y=(a[1+i+624]&0x80000000)|(a[1+(i+625)%1248]&0x7fffffff);a[1+i]=a[1+(i+1021)%1248]^(y>>1)^if y&1!=0{0x9908b0df}else{0};}a[0]=0;
 }
 let i=a[0] as usize;a[0]+=1;Ok(temper(a[1+i]))
}
fn x64(v:&[u32])->Result<[u64;4]>{if v.len()!=8{return Err("xoshiro state".into())}Ok(std::array::from_fn(|i|v[2*i] as u64|((v[2*i+1] as u64)<<32)))}
fn x64_words(v:[u64;4])->Vec<u32>{v.into_iter().flat_map(|x|[x as u32,(x>>32) as u32]).collect()}
fn xoshiro(s:&mut [u64;4])->u64{
 let result=s[1].wrapping_mul(5).rotate_left(7).wrapping_mul(9);let t=s[1]<<17;s[2]^=s[0];s[3]^=s[1];s[1]^=s[2];s[0]^=s[3];s[2]^=t;s[3]=s[3].rotate_left(45);result
}
fn float53(x:u64)->f64{(x>>11) as f64*(1.0/9007199254740992.0)}
fn near(a:f64,b:f64,tolerance:f64)->Result<()>{if !a.is_finite()||!b.is_finite()||(a-b).abs()>tolerance{return Err(format!("float mismatch:{a}/{b}"))}Ok(())}
fn primitives(v:&Value)->Result<usize>{
 let mut state=w(&v["before"])?;let values=v["results"].as_array().ok_or("primitive results")?;
 for r in values{let x=xs(&mut state)?;let bits=u(&r["bits"])? as u32;
  match u(&r["kind"])?{0=>if bits!=x{return Err("Next result".into())},1=>{let max=u(&r["argument"])? as u32;if bits!=if max==0{0}else{x%max}{return Err("RandomInt result".into())}},2=>near(f32::from_bits(bits) as f64,x as f64/4294967296.0,4e-7)?,_=>return Err("primitive kind".into())}
 }if state!=w(&v["after"])?{return Err("primitive after state".into())}Ok(values.len())
}
fn mt_fixture(v:&Value)->Result<usize>{
 let mut state=w(&v["before"])?;let values=v["results"].as_array().ok_or("MT results")?;for r in values{if mt(&mut state)? as u64!=u(r)?{return Err("MT original result".into())}}
 if state!=w(&v["after"])?{return Err("MT fixture after".into())}Ok(values.len())
}
fn crt_fixture(v:&Value)->Result<usize>{
 let before=w(&v["before"]["words"])?;if before.len()!=1{return Err("CRT state unavailable".into())}
 let mut state=before[0];let values=v["results"].as_array().ok_or("CRT results")?;
 for r in values{state=state.wrapping_mul(214013).wrapping_add(2531011);if u(r)?!=((state>>16)&32767) as u64{return Err("CRT original result".into())}}
 if w(&v["after"]["words"])?!=vec![state]{return Err("CRT fixture after".into())}Ok(values.len())
}
fn runtime_fixture(v:&Value)->Result<()>{
 let mut state=w(&v["before"]["words"])?;let values=v["values"].as_array().ok_or("float results")?;
 if values.len()!=1300{return Err("float draw count".into())}
 for r in values{let x=msvc_mt(&mut state)?;near(r.as_f64().ok_or("float value")?,((x as f32/4294967296.0)*2.0-1.0) as f64,4e-7)?;}
 if state!=w(&v["after"]["words"])?{return Err("float source post-state".into())}
 let mut s=x64(&w(&v["math_before"]["words"])?)?;
 let mut seeded=[1234,255,5678,0];for _ in 0..16{xoshiro(&mut seeded);}
 if s!=seeded||signed(&v["seed1"])?!=1234||signed(&v["seed2"])?!=5678{return Err("math seed/multiple returns".into())}
 for r in v["draws"].as_array().ok_or("math draws")?{
  let mut value=xoshiro(&mut s);
  match u(&r["argc"])?{
   // 游戏json库的夹具数字仅保留14位；完整原生状态仍逐字精确比较，正式C++结果保留17位。
   0=>near(r["value"].as_f64().ok_or("math float")?,float53(value),1e-13)?,
   1=>if signed(&r["value"])? as u64!=value{return Err("math 64-bit return".into())},
   2=>{let lo=signed(&r["lo"])?;let lim=(signed(&r["hi"])?-lo) as u64;let mask=(lim+1).next_power_of_two()-1;value&=mask;let mut retries=0;while value>lim{value=xoshiro(&mut s)&mask;retries+=1;if retries>64{return Err("math projection retries".into())}}if signed(&r["value"])?!=lo+value as i64{return Err("math projected result".into())}},
   _=>return Err("math args".into())
  }
 }
 if x64_words(s)!=w(&v["error_before"]["words"])?{return Err("math pre-error state".into())}
 xoshiro(&mut s);if x64_words(s)!=w(&v["error_after"]["words"])?{return Err("math error consumption".into())}
 let recovered=xoshiro(&mut s);near(v["recovered"].as_f64().ok_or("math recovered")?,float53(recovered),1e-13)?;
 if x64_words(s)!=w(&v["math_after"]["words"])?{return Err("math after recovery".into())}Ok(())
}
fn actual(rows:&[Value])->Result<Value>{crate::tx14_global_random::actual(rows)}
fn number(s:&BTreeMap<String,String>,k:&str)->u64{s.get(k).and_then(|v|v.parse().ok()).unwrap_or(0)}
impl Run {
 // 续测只重建必要的seed/实体前提；已完成样本保留原capture关联，不重放随机消费场景。
 pub(super) fn prepare_resume_tx14_native_random(&mut self)->Result<()>{
  let Some(paths)=self.scenario["resume_from"].as_array() else{return Ok(())};
  let mut roots=BTreeSet::new();let mut samples=BTreeMap::new();
  for path in paths{
   let base=std::path::PathBuf::from(path.as_str().ok_or("native resume path")?);
   let report:Value=serde_json::from_str(&crate::read(&base.join("result.json"))?).map_err(|e|e.to_string())?;
   if report["native_random"]!=true||report["cleanup_errors"].as_array().is_none_or(|a|a.iter().any(|x|!x.is_null())){return Err("native resume requires restored environment".into())}
   if let Some(old)=report["native_resume_sources"].as_array(){for p in old{roots.insert(p.as_str().ok_or("source")?.to_string());}}
   roots.insert(base.to_string_lossy().to_string());
   for sample in report["native_rng_samples"].as_array().ok_or("native samples")?{
    let mut sample=sample.clone();let name=sample["actual"]["name"].as_str().ok_or("native scene name")?.to_string();
    match name.as_str(){"primitives"=>{primitives(&sample["actual"]["native"])?;},"mt"=>{mt_fixture(&sample["actual"])?;},"crt"=>{crt_fixture(&sample["actual"])?;},"runtime"=>{runtime_fixture(&sample["actual"])?;},_=>{}}
    if sample["capture_root"].is_null(){sample["capture_root"]=json!(base);}
    samples.insert(name,sample);
   }
  }
  let mut pending=vec!["seed","spawn"];
  for name in ["inventory","primitives","mt","crt","runtime","inline","loot","retire","next_floor","paused","record_off","observe_off","observe_restart"]{if !samples.contains_key(name){pending.push(name);}}
  self.report["native_rng_samples"]=json!(samples.into_values().collect::<Vec<_>>());self.report["native_resume_sources"]=json!(roots);
  self.scenario["tx14_pending"]=json!(pending);Ok(())
 }
 pub(super) fn scenes_tx14_native_random(&mut self,g:&win32::Game)->Result<()>{
  self.report["native_random"]=json!(true);if !self.report["native_rng_samples"].is_array(){self.report["native_rng_samples"]=json!([]);}
  let actions=self.scenario["tx14_pending"].as_array().map(|a|a.iter().map(|v|v.as_str().unwrap().to_string()).collect::<Vec<_>>()).unwrap_or_else(||["seed","inventory","primitives","mt","crt","runtime","inline","spawn","loot","retire","next_floor","paused","record_off","observe_off","observe_restart"].iter().map(|s|s.to_string()).collect());
  let action_index=|name:&str|actions.iter().position(|v|v==name).map(|i|i+1).ok_or_else(||format!("pending action absent:{name}"));
  let w=g.wait_window(90)?;let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("port")? as u16,g,90,&self.out.join("control.jsonl"))?;c.focus_window=Some(w);
  self.prepare_new_run_tx05(g,w,&mut c)?;self.sequence(w,"start_run")?;c.wait("run_active","1",30)?;
  c.command("cs world start native-rng")?;self.wait_tx14(g,&mut c,"cs world status",|s|s.get("world_state").is_some_and(|v|v=="Recording"))?;c.command("cs observe start")?;
  // 真实重建发生在观测已启用之后，才能验收池洗牌和短命生成上下文。
  self.action_tx14(g,&mut c,w,1)?;c.command(&format!("cs host {} 0",self.scenario["host_port"]))?;
  self.wait_tx14(g,&mut c,"cs status",|s|s.get("session_state").is_some_and(|v|v=="Active"))?;
  let remote=Remote::start_participant(format!("127.0.0.1:{}",self.scenario["host_port"]).parse().map_err(|e|format!("{e}"))?,&self.out.join("remote-1.jsonl"),30000,2,1,|_|Ok(()))?;
  self.wait_tx14(g,&mut c,"cs status",|s|s.get("participant.1.bound").is_some_and(|v|v=="1"))?;
  self.wait_tx14(g,&mut c,"cs observe status",|s|number(s,"random_scope")>0)?;
  if self.scenario["crt_imports"]==true{
   let value=actual(&self.action_tx14(g,&mut c,w,2)?)?;
   for source in value["providers"]["sources"].as_array().ok_or("providers")?.iter().take(2){if source["import_module_mask"]!=7||source["installed"]!=true{return Err(format!("CRT modules incomplete:{source}"))}}
   let lights=value["lights"].as_array().ok_or("REP light samples")?;if lights.len()!=8{return Err("light count".into())}
   let mut expected=BTreeSet::new();
   for light in lights{
    let before=crate::tx14_native_random::w(&light["before"]["words"])?;let after=crate::tx14_native_random::w(&light["after"]["words"])?;
    if before.len()!=1{return Err("REP CRT before".into())}let next=before[0].wrapping_mul(214013).wrapping_add(2531011);
    if after!=vec![next]{return Err("REP light CRT advance".into())}near(light["scale"].as_f64().ok_or("light scale")?,((next>>16)&32767) as f64/32767.0,2e-7)?;expected.insert((before[0],next));
   }
   c.command("cs world stop")?;self.wait_tx14(g,&mut c,"cs world status",|s|s.get("world_state").is_some_and(|v|v=="Closed"))?;
   remote.bye()?;drop(remote);c.command("cs stop")?;self.wait_tx14(g,&mut c,"cs status",|s|s.get("session_state").is_some_and(|v|v=="Idle"))?;
   c.command("cs observe stop")?;self.wait_tx14(g,&mut c,"cs observe status",|s|s.get("observation_state").is_some_and(|v|v=="Off"))?;
   self.return_to_menu(w,&mut c)?;self.report["final_status"]=json!(c.command("cs status")?);drop(c);if g.close()?!=0{return Err("CRT game exit".into())}
   let root=self.out.join("runtime-world/native-rng/observation-v2");let reader=crate::observation::read_capture(&root)?;
   if reader["supported_read_complete"]!=true||reader["delivery_complete"]!=true{return Err(format!("CRT reader:{reader}"))}
   let mut seeds=0;let mut draws=0;
   for line in BufReader::new(fs::File::open(root.join("records.jsonl")).map_err(|e|e.to_string())?).lines(){
    let row:Value=serde_json::from_str(&line.map_err(|e|e.to_string())?).map_err(|e|e.to_string())?;let p=&row["payload"];
    if row["schema"]!="isaac.cs.random.native-call"{continue}
    if p["member"]=="CRT.srand"{seeds+=1;}
    if p["member"]!="CRT.rand"{continue}draws+=1;
    if row["coverage"]["status"]!="Complete"||row["context"]["source"]!=1||!row["context"]["authority_tick"].is_null(){return Err("CRT provenance".into())}
    let before=crate::tx14_native_random::w(&p["before"]["words"])?;let after=crate::tx14_native_random::w(&p["after"]["words"])?;if before.len()!=1||after.len()!=1{return Err("CRT words".into())}
    let next=before[0].wrapping_mul(214013).wrapping_add(2531011);if after[0]!=next||u(&p["result_bits"])?!=((next>>16)&32767) as u64{return Err("CRT formal state/result".into())}expected.remove(&(before[0],after[0]));
   }
   if !expected.is_empty()||seeds==0{return Err(format!("REP CRT events absent:{expected:?} seeds={seeds}"))}
   self.case("native_rng_crt_late_module_imports",true,json!({"providers":value["providers"],"REP_lights":8,"formal_draws":draws,"formal_seeds":seeds,"reader":reader}));
   println!("tx14 CRT late module PASS module_mask=7 REP_lights=8 draws={draws} seeds={seeds}");return Ok(())
  }
  for (offset,name) in actions.iter().enumerate().skip(1).take_while(|(_,n)|n.as_str()!="paused"){
   let index=offset+1;
   let value=actual(&self.action_tx14(g,&mut c,w,index)?)?;
   let before=c.command("cs observe status")?;
   let status=self.wait_tx14(g,&mut c,"cs observe status",|s|number(s,"random_scope")>number(&before,"random_scope"))?;
   if status.get("random_coverage").map(String::as_str)!=Some("Complete"){return Err(format!("native RNG ordinary incomplete:{status:?}"))}
   self.report["native_rng_samples"].as_array_mut().unwrap().push(json!({"actual":value,"status":status,"capture_root":self.out}));self.step(&format!("native_rng_{name}"),json!(status))?;
  }
  self.wait_tx14(g,&mut c,"cs status",|s|number(s,"completed_tick")>=100)?;
  win32::key(w,"ESC")?;c.wait_pause(None)?;let before=c.command("cs observe status")?;
  let paused=actual(&self.action_tx14(g,&mut c,w,action_index("paused")?)?)?;let after=c.command("cs observe status")?;
  if before.get("random_scope")!=after.get("random_scope"){return Err("paused ordinary scope advanced".into())}
  self.report["native_rng_samples"].as_array_mut().unwrap().push(json!({"actual":paused,"status":after}));win32::key(w,"ESC")?;
  c.command("cs world stop")?;self.wait_tx14(g,&mut c,"cs world status",|s|s.get("world_state").is_some_and(|v|v=="Closed"))?;
  let file=self.out.join("runtime-world/native-rng/observation-v2/records.jsonl");let len=fs::metadata(&file).map_err(|e|e.to_string())?.len();
  let recorded_epoch=after.get("observation_epoch").cloned();
  let value=actual(&self.action_tx14(g,&mut c,w,action_index("record_off")?)?)?;let b=c.command("cs observe status")?;
  let continued=self.wait_tx14(g,&mut c,"cs observe status",|s|number(s,"random_scope")>number(&b,"random_scope"))?;
  if fs::metadata(&file).map_err(|e|e.to_string())?.len()!=len||continued.get("observation_epoch").cloned()!=recorded_epoch{return Err("recording separation".into())}
  self.report["native_rng_samples"].as_array_mut().unwrap().push(json!({"actual":value,"status":continued}));
  remote.bye()?;drop(remote);c.command("cs stop")?;self.wait_tx14(g,&mut c,"cs status",|s|s.get("session_state").is_some_and(|v|v=="Idle"))?;
  c.command("cs observe stop")?;self.wait_tx14(g,&mut c,"cs observe status",|s|s.get("observation_state").is_some_and(|v|v=="Off"))?;
  let value=actual(&self.action_tx14(g,&mut c,w,action_index("observe_off")?)?)?;self.report["native_rng_samples"].as_array_mut().unwrap().push(json!({"actual":value,"observer":"Off"}));
  c.command("cs observe start")?;self.wait_tx14(g,&mut c,"cs observe status",|s|s.get("observation_state").is_some_and(|v|v=="Active"))?;
  let value=actual(&self.action_tx14(g,&mut c,w,action_index("observe_restart")?)?)?;self.report["native_rng_samples"].as_array_mut().unwrap().push(json!({"actual":value,"observer":"Active"}));
  c.command("cs observe stop")?;self.wait_tx14(g,&mut c,"cs observe status",|s|s.get("observation_state").is_some_and(|v|v=="Off"))?;
  self.return_to_menu(w,&mut c)?;self.report["final_status"]=json!(c.command("cs status")?);drop(c);if g.close()?!=0{return Err("game exit".into())}
  self.reduce_tx14_native_random()
 }
 pub(super) fn reduce_tx14_native_random(&mut self)->Result<()>{
  let samples=self.report["native_rng_samples"].as_array().ok_or("samples")?.clone();
  let get=|name:&str|samples.iter().find(|v|v["actual"]["name"]==name).map(|v|&v["actual"]).ok_or_else(||format!("scene missing:{name}"));
  if primitives(&get("primitives")?["native"])?!=30||mt_fixture(get("mt")?)?!=1300||crt_fixture(get("crt")?)?!=64{return Err("primary draw counts".into())}
  runtime_fixture(get("runtime")?)?;
  let inline=get("inline")?;let mut state=w(&inline["before"])?;for _ in 0..9{xs(&mut state)?;}
  if state!=w(&inline["after"])?||inline["first"]["local_id"]==inline["copy"]["local_id"]||inline["first"]["local_id"]==inline["reused"]["local_id"]{return Err("inline/copy/reuse fixture".into())}
  for name in ["paused","record_off","observe_off","observe_restart"]{if primitives(&get(name)?["native"])?!=6{return Err(format!("passthrough:{name}"))}}
  crt_fixture(&get("record_off")?["crt"])?;crt_fixture(&get("observe_off")?["crt"])?;mt_fixture(&get("observe_off")?["mt"])?;
  if get("loot")?["before"]!=get("loot")?["readonly"]{return Err("loot observer advanced owner".into())}
  let mut roots=self.report["native_resume_sources"].as_array().cloned().unwrap_or_default().iter().map(|p|std::path::PathBuf::from(p.as_str().unwrap())).collect::<Vec<_>>();roots.push(self.out.clone());
  let mut readers=Vec::new();
  let mut selected=BTreeMap::<String,Value>::new();let mut members=BTreeMap::<String,u64>::new();let mut runtime=BTreeMap::<String,u64>::new();
  let mut changes=0u64;let mut retired=0u64;let mut native_calls=0;let mut nonlocal=0;let mut mt_twists=0;let mut expected_partial=0;
  for base in &roots{
  let root=base.join("runtime-world/native-rng/observation-v2");let reader=crate::observation::read_capture(&root)?;
  if reader["supported_read_complete"]!=true||reader["delivery_complete"]!=true{return Err(format!("reader:{reader}"))}readers.push(reader);
  let scopes=samples.iter().filter(|s|s["capture_root"].as_str().map(std::path::Path::new).unwrap_or(&self.out)==base).filter_map(|s|s["status"]["random_scope"].as_str().map(String::from)).collect::<BTreeSet<_>>();
  let mut seen_calls=BTreeSet::new();
  for line in BufReader::new(fs::File::open(root.join("records.jsonl")).map_err(|e|e.to_string())?).lines(){
   let line=line.map_err(|e|e.to_string())?;let row:Value=serde_json::from_str(&line).map_err(|e|e.to_string())?;let p=&row["payload"];
   let schema=row["schema"].as_str().unwrap_or("");
   if schema.starts_with("isaac.cs.random.native-")||schema=="isaac.cs.random.runtime-call"{
    if !row["context"]["authority_tick"].is_null()||row["context"]["source"]!=1{return Err("native source borrowed tick or Lua provenance".into())}
   }
   if schema=="isaac.cs.random.native-call"{
    if row["coverage"]["status"]!="Complete"||p["completion"]!="Returned"{return Err(format!("native call incomplete:{p}"))}
    let id=u(&p["call"])?;if !seen_calls.insert(id){return Err("duplicate native call".into())}
    let member=p["member"].as_str().ok_or("member")?;*members.entry(member.into()).or_default()+=1;native_calls+=1;
    let mut before=w(&p["before"]["words"])?;let after=w(&p["after"]["words"])?;
    if member=="RNG.Next"||member=="RNG.RandomInt"||member=="RNG.RandomFloat"{
      let x=xs(&mut before)?;let bits=u(&p["result_bits"])? as u32;
      if member=="RNG.Next"&&bits!=x{return Err("formal Next result".into())}
      if member=="RNG.RandomInt"{let max=u(&p["argument"])? as u32;if bits!=if max==0{0}else{x%max}{return Err("formal RandomInt result".into())}}
      if member=="RNG.RandomFloat"{near(f32::from_bits(bits) as f64,x as f64/4294967296.0,4e-7)?;}
      if before!=after{return Err(format!("formal xorshift post:{member}"))}
    }else if member=="MTRNG.Next"{
      if before.get(624).is_some_and(|v|*v>=624){mt_twists+=1}
      if mt(&mut before)? as u64!=u(&p["result_bits"])?||before!=after{return Err("formal native MT result/state".into())}
    }else if member=="MTRNG.Init"{if after!=mt_seed(u(&p["argument"])? as u32){return Err("formal MT seed".into())}}
    else if member=="CRT.rand"{
      if before.len()!=1{return Err("formal CRT before".into())}let x=before[0].wrapping_mul(214013).wrapping_add(2531011);
      if after!=vec![x]||u(&p["result_bits"])?!=((x>>16)&32767) as u64{return Err("formal CRT result/state".into())}
    }else if member=="CRT.srand"{if after!=vec![u(&p["argument"])? as u32]{return Err("formal CRT seed".into())}}
    else if member=="RNG.SetSeed"||member=="RNG.constructor"{if after.len()!=4||after[0]!=u(&p["argument"])? as u32{return Err("formal xorshift initialize".into())}}
   }else if schema=="isaac.cs.random.runtime-call"{
    let member=p["member"].as_str().ok_or("runtime member")?;*runtime.entry(member.into()).or_default()+=1;
    if p["completion"]=="NonLocalLuaExit;return-unavailable"{nonlocal+=1;expected_partial+=1;continue}
    if row["coverage"]["status"]!="Complete"{return Err(format!("runtime state unavailable:{p}"))}
    if member=="C::RandomFloat"{
      let mut state=w(&p["before"]["words"])?;let x=msvc_mt(&mut state)?;
      if state!=w(&p["after"]["words"])?{return Err("formal MSVC MT advance".into())}
      near(p["returns"][0].as_f64().ok_or("formal float result")?,((x as f32/4294967296.0)*2.0-1.0) as f64,4e-7)?;
    }else if member=="C::math.random"{
      let mut state=x64(&w(&p["before"]["words"])?)?;let mut value=xoshiro(&mut state);let args=p["arguments"].as_array().ok_or("math arguments")?;
      if args.is_empty(){near(p["returns"][0].as_f64().ok_or("math float return")?,float53(value),1e-15)?}
      else{let (lo,hi)=if args.len()==1{(1,signed(&args[0])?)}else{(signed(&args[0])?,signed(&args[1])?)};
       if args.len()==1&&hi==0{if signed(&p["returns"][0])? as u64!=value{return Err("formal math 64-bit".into())}}
       else{let lim=(hi-lo) as u64;let mask=if lim==0{0}else{u64::MAX>>(lim.leading_zeros())};let mut attempts=0;value&=mask;while value>lim{value=xoshiro(&mut state)&mask;attempts+=1;if attempts>128{return Err("formal projection bound".into())}}if signed(&p["returns"][0])?!=lo+value as i64{return Err("formal math projection".into())}}
      }if x64_words(state)!=w(&p["after"]["words"])?{return Err("formal xoshiro post".into())}
    }else if member=="C::math.randomseed"{
      let mut state=[signed(&p["returns"][0])? as u64,255,signed(&p["returns"][1])? as u64,0];for _ in 0..16{xoshiro(&mut state);}
      if x64_words(state)!=w(&p["after"]["words"])?{return Err("formal xoshiro seed".into())}
    }
   }else if schema=="isaac.cs.random.native-state-change"{if p["before"]["words"]==p["after"]["words"]{return Err("invented state change".into())}changes+=1}
   else if schema=="isaac.cs.random.native-retire"{retired+=1}
   else if schema=="isaac.cs.random"&&row["context"]["scope"].as_str().is_some_and(|s|scopes.contains(s)){selected.insert(format!("{}:{}",base.display(),row["context"]["scope"].as_str().unwrap()),p.clone());}
  }
  }
  let inventory=samples.iter().find(|s|s["actual"]["name"]=="inventory").ok_or("inventory sample")?;
  let scope=inventory["status"]["random_scope"].as_str().ok_or("scope")?;let source=inventory["capture_root"].as_str().map(std::path::Path::new).unwrap_or(&self.out);let current=&selected.get(&format!("{}:{scope}",source.display())).ok_or("formal inventory absent")?["native_sources"];
  if current["complete"]!=true{return Err("formal source inventory incomplete".into())}
  let entries=current["streams"].as_array().ok_or("streams")?;let mut kinds=BTreeMap::<String,u64>::new();let mut player_counts=BTreeMap::<String,[u64;4]>::new();
  for e in entries{
   let purpose=e["purpose"].as_str().ok_or("purpose")?;*kinds.entry(purpose.into()).or_default()+=1;
   if e["state"]["status"]!="Value"{return Err("owner state unavailable".into())}
   if purpose.starts_with("EntityPlayer."){let kind=if purpose.contains("Collectible"){0}else if purpose.contains("Trinket"){1}else if purpose.contains("Pill"){2}else{3};player_counts.entry(e["owner"]["local_id"].as_str().ok_or("player owner id")?.into()).or_insert([0;4])[kind]+=1;}
  }
  for name in ["Game.GenerationRNG","Seeds.InternalRNG","ItemPool.GlobalRNG","PlayerManager.InternalRNG","Ambush.InternalRNG","Backdrop.RNG"]{if !kinds.contains_key(name){return Err(format!("owner absent:{name}"))}}
  if kinds.keys().filter(|k|k.starts_with("ItemPool[")).count()!=62||kinds.keys().filter(|k|k.starts_with("BossPool[")).count()!=37{return Err("pool RNG membership".into())}
  let mut expected=get("inventory")?["owners"]["players"].as_array().ok_or("actual player counts")?.iter().map(|p|p["counts"].as_array().unwrap().iter().map(|x|x.as_u64().unwrap()).collect::<Vec<_>>()).collect::<Vec<_>>();
  let mut observed=player_counts.values().map(|x|x.to_vec()).collect::<Vec<_>>();expected.sort();observed.sort();if expected!=observed{return Err(format!("Player RNG vector membership:{expected:?}/{observed:?}"))}
  for member in ["RNG.constructor","RNG.SetSeed","RNG.Next","RNG.RandomInt","RNG.RandomFloat","MTRNG.Init","MTRNG.Next","CRT.srand","CRT.rand","Level.generate_dungeon.argument","Level.generate_dungeon.GameGenerationRNG","LevelGenerator.Generate"]{if members.get(member).copied().unwrap_or(0)==0{return Err(format!("native member not exercised:{member}"))}}
  if runtime.get("C::RandomFloat").copied().unwrap_or(0)<1300||runtime.get("C::math.random").copied().unwrap_or(0)<13||nonlocal!=1||mt_twists<2||changes==0||retired==0{return Err(format!("native coverage: runtime={runtime:?} nonlocal={nonlocal} twists={mt_twists} changes={changes} retired={retired}"))}
  self.case("native_rng_owner_inventory",true,json!({"streams":entries.len(),"player_counts":observed,"pool_streams":99}));
  self.case("native_rng_cpp_primitives_mt_crt",true,json!({"calls":native_calls,"members":members,"mt_twists":mt_twists}));
  self.case("native_rng_runtime_float_math",true,json!({"calls":runtime,"expected_error_partial":expected_partial}));
  self.case("native_rng_inline_copy_lifecycle_generation",true,json!({"state_changes":changes,"retired":retired}));
  self.case("native_rng_pause_recording_off_passthrough",true,json!({"scenes":samples.len(),"optional_recording":true}));
  self.case("native_rng_reader_delivery",true,json!(readers));println!("tx14 remaining RNG PASS calls={native_calls} owner_streams={} state_changes={changes} retired={retired}",entries.len());Ok(())
 }
}
