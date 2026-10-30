//! tx-04-14同一验收内的全局MT补测；由真实调用记录反算状态，不推进游戏随机源。
use crate::{Control,Result,Run,win32,remote::Remote};
use serde_json::{Value,json};
use std::{collections::{BTreeMap,BTreeSet},fs,io::{BufRead,BufReader}};
fn u(v:&Value)->Result<u64>{v.as_u64().or_else(||v.as_str().and_then(|s|s.parse().ok())).ok_or_else(||format!("expected unsigned:{v}"))}
fn words(v:&Value)->Result<Vec<u32>>{
 let a=v.as_array().ok_or("MT words absent")?;if a.len()!=624{return Err("MT state must contain 624 words".into())}
 a.iter().map(|v|u(v).and_then(|n|u32::try_from(n).map_err(|e|e.to_string()))).collect()
}
fn seed(value:u32)->Vec<u32>{
 let mut w=vec![0;624];w[0]=value;for i in 1..624{w[i]=1812433253u32.wrapping_mul(w[i-1]^(w[i-1]>>30)).wrapping_add(i as u32)}w
}
// 独立重算标准MT19937：twist是原地更新，后227项引用已经更新的前段。
fn draw(w:&mut Vec<u32>,index:&mut usize)->Result<u32>{
 if *index==625{*w=seed(5489);*index=624}
 if *index>624{return Err("invalid MT index".into())}
 if *index==624{
  for i in 0..624{let y=(w[i]&0x80000000)|(w[(i+1)%624]&0x7fffffff);w[i]=w[(i+397)%624]^(y>>1)^if y&1!=0{0x9908b0df}else{0};}*index=0;
 }
 let mut y=w[*index];*index+=1;y^=y>>11;y^=(y<<7)&0x9d2c5680;y^=(y<<15)&0xefc60000;y^=y>>18;Ok(y)
}
fn actual(rows:&[Value])->Result<Value>{
 if let Some(v)=rows.iter().find(|r|r["event"]=="actual"){return Ok(v.clone())}
 let parts=rows.iter().filter(|r|r["event"]=="native_rng_chunk").collect::<Vec<_>>();
 let first=parts.first().ok_or("fixture actual absent")?;let total=u(&first["total"])? as usize;
 if total==0||total>64||parts.len()!=total{return Err("fixture state chunks missing".into())}
 let mut text=String::new();for i in 1..=total{
  let p=parts.iter().filter(|p|u(&p["part"]).ok()==Some(i as u64)).collect::<Vec<_>>();
  if p.len()!=1||p[0]["serial"]!=first["serial"]||p[0]["total"]!=first["total"]{return Err("fixture state chunks duplicated/mixed".into())}
  text.push_str(p[0]["data"].as_str().ok_or("fixture chunk text")?);
 }
 let v:Value=serde_json::from_str(&text).map_err(|e|e.to_string())?;if v["event"]!="actual"{return Err("fixture chunk event".into())}Ok(v)
}
fn check_fixture(v:&Value,reseed:Option<u32>)->Result<usize>{
 let before=&v["before"];let after=&v["after"];
 if before["state"]["mode"]!="MT19937"||after["state"]["mode"]!="MT19937"{return Err("unexpected CRT mode".into())}
 let mut w=if let Some(s)=reseed{seed(s)}else{words(&before["words"])?};
 let mut index=if reseed.is_some(){624}else{u(&before["state"]["index"])? as usize};
 let values=v["results"].as_array().ok_or("actual return values absent")?;
 for value in values{if u(value)?!=u64::from(draw(&mut w,&mut index)?){return Err(format!("actual original return mismatch:{}",v["name"]))}}
 if w!=words(&after["words"])?||index!=u(&after["state"]["index"])? as usize{return Err("fixture post-state mismatch".into())}
 Ok(values.len())
}
impl Run {
 pub(super) fn scenes_tx14_global_random(&mut self,g:&win32::Game)->Result<()>{
  self.report["global_random"]=json!(true);
  let w=g.wait_window(90)?;let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("port")? as u16,g,90,&self.out.join("control.jsonl"))?;c.focus_window=Some(w);
  self.prepare_new_run_tx05(g,w,&mut c)?;self.sequence(w,"start_run")?;c.wait("run_active","1",30)?;
  self.action_tx14(g,&mut c,w,1)?;
  c.command("cs world start native-global")?;self.wait_tx14(g,&mut c,"cs world status",|s|s.get("world_state").is_some_and(|v|v=="Recording"))?;
  c.command("cs observe start")?;c.command(&format!("cs host {} 0",self.scenario["host_port"]))?;
  self.wait_tx14(g,&mut c,"cs status",|s|s.get("session_state").is_some_and(|v|v=="Active"))?;
  let remote=Remote::start_participant(format!("127.0.0.1:{}",self.scenario["host_port"]).parse().map_err(|e|format!("{e}"))?,&self.out.join("remote-1.jsonl"),30000,2,1,|_|Ok(()))?;
  self.wait_tx14(g,&mut c,"cs status",|s|s.get("participant.1.bound").is_some_and(|v|v=="1"))?;
  let mut samples=Vec::new();
  samples.push(actual(&self.action_tx14(g,&mut c,w,2)?)?);
  // Join后的入场动画会屏蔽暂停键；等稳定模拟，避免把丢弃的物理按键当成暂停。
  self.wait_tx14(g,&mut c,"cs status",|s|s.get("completed_tick").and_then(|v|v.parse::<u64>().ok()).is_some_and(|v|v>=100))?;
  // 暂停中触发真实Lua取值；ActualCall可前进，普通Authority采集保持不变。
  win32::key(w,"ESC")?;c.wait_pause(None)?;let paused=c.command("cs observe status")?;
  samples.push(actual(&self.action_tx14(g,&mut c,w,3)?)?);let still=c.command("cs observe status")?;
  if paused.get("random_scope")!=still.get("random_scope"){return Err("paused ordinary Random scope advanced".into())}
  win32::key(w,"ESC")?;
  for index in 4..=7{samples.push(actual(&self.action_tx14(g,&mut c,w,index)?)?);}
  let status=self.wait_tx14(g,&mut c,"cs observe status",|s|s.get("random_coverage").is_some_and(|v|v=="Complete")&&s.get("random_scope").is_some_and(|v|v!="0"))?;
  for k in ["loss_sequence","record_loss_sequence"]{if status.get(k).is_some_and(|v|v!="0"){return Err(format!("native Random loss:{status:?}"))}}
  self.report["native_random_recorded_status"]=json!(status);
  c.command("cs world stop")?;self.wait_tx14(g,&mut c,"cs world status",|s|s.get("world_state").is_some_and(|v|v=="Closed"))?;
  let file=self.out.join("runtime-world/native-global/observation-v2/records.jsonl");
  let length=fs::metadata(&file).map_err(|e|e.to_string())?.len();
  samples.push(actual(&self.action_tx14(g,&mut c,w,8)?)?);
  if fs::metadata(&file).map_err(|e|e.to_string())?.len()!=length{return Err("closed recorder appended native calls".into())}
  remote.bye()?;drop(remote);c.command("cs stop")?;self.wait_tx14(g,&mut c,"cs status",|s|s.get("session_state").is_some_and(|v|v=="Idle"))?;
  c.command("cs observe stop")?;self.wait_tx14(g,&mut c,"cs observe status",|s|s.get("observation_state").is_some_and(|v|v=="Off"))?;
  samples.push(actual(&self.action_tx14(g,&mut c,w,9)?)?);
  c.command("cs observe start")?;self.wait_tx14(g,&mut c,"cs observe status",|s|s.get("observation_state").is_some_and(|v|v=="Active"))?;
  samples.push(actual(&self.action_tx14(g,&mut c,w,10)?)?);
  c.command("cs observe stop")?;self.wait_tx14(g,&mut c,"cs observe status",|s|s.get("observation_state").is_some_and(|v|v=="Off"))?;
  self.report["native_random_samples"]=json!(samples);
  self.return_to_menu(w,&mut c)?;self.report["final_status"]=json!(c.command("cs status")?);drop(c);
  if g.close()?!=0{return Err("game exit".into())}self.reduce_tx14_global_random()
 }
 pub(super) fn reduce_tx14_global_random(&mut self)->Result<()>{
  let samples=self.report["native_random_samples"].as_array().ok_or("native RNG samples")?.clone();
  let get=|name:&str|samples.iter().find(|r|r["name"]==name).ok_or_else(||format!("scene absent:{name}"));
  let a=get("readonly")?;if a["before"]!=a["after"]{return Err("read-only native state observation changed state".into())}
  for(name,count,s)in [("lua_draws",2,None),("native_twists",1300,Some(305419896)),("zero_seed",1,Some(0)),("record_off",24,None),("observe_off",24,None),("observe_restart",24,None)]{
   if check_fixture(get(name)?,s)?!=count{return Err(format!("draw count:{name}"))}
  }
  let f=get("record_off")?;if f["before"]["enabled"]!=true||u(&f["after"]["calls"])?-u(&f["before"]["calls"])?!=24{return Err("formal native Random stopped with recorder".into())}
  let f=get("observe_off")?;if f["before"]["enabled"]!=false||f["after"]["enabled"]!=false{return Err("native observer not disabled".into())}
  if get("observe_restart")?["after"]["enabled"]!=true{return Err("native observer failed to restart".into())}
  if get("damocles_spawn")?["passive"]!=true||u(&get("damocles_damage")?["subtype"])?==0{return Err("real damaged Damocles not active".into())}
  let root=self.out.join("runtime-world/native-global/observation-v2");let read=crate::observation::read_capture(&root)?;
  if read["supported_read_complete"]!=true||read["delivery_complete"]!=true{return Err(format!("reader:{read}"))}
  let mut blocks=BTreeMap::<(u64,u64),Vec<u32>>::new();let mut calls=BTreeMap::<u64,Value>::new();let mut ordinary=0;let mut epochs=BTreeSet::new();
  for line in BufReader::new(fs::File::open(root.join("records.jsonl")).map_err(|e|e.to_string())?).lines(){
   let line=line.map_err(|e|e.to_string())?;let row:Value=serde_json::from_str(&line).map_err(|e|e.to_string())?;let p=&row["payload"];
   if row["schema"]=="isaac.cs.random.native-global-block"{
    if row["coverage"]["status"]!="Complete"{return Err("native MT block incomplete".into())}
    let key=(u(&p["stream"])?,u(&p["block"])?);let w=words(&p["words"])?;
    if let Some(old)=blocks.insert(key,w.clone()){if old!=w{return Err("native block revision reused".into())}}
   }else if row["schema"]=="isaac.cs.random.native-global-call"{
    if row["coverage"]["status"]!="Complete"||!row["context"]["authority_tick"].is_null()||row["context"]["source"]!=1{return Err("native call context or coverage".into())}
    if calls.insert(u(&p["call"])?,p.clone()).is_some(){return Err("duplicate native call".into())}
    epochs.insert(row["context"]["epoch"].to_string());
   }else if row["schema"]=="isaac.cs.random"{
    let s=&p["native_global_random"];if s["complete"]!=true||words(&s["words"]).is_err(){return Err("ordinary full native MT state absent".into())}ordinary+=1;
   }
  }
  let mut draws=0;let mut seeds=0;let mut twists=0;let mut damocles=0;let mut native_other=0;
  for p in calls.values(){
   let stream=u(&p["stream"])?;let b=&p["before"];let a=&p["after"];
   let mut state=blocks.get(&(stream,u(&b["block"])?)).ok_or("before block absent")?.clone();let mut index=u(&b["index"])? as usize;
   let after=blocks.get(&(stream,u(&a["block"])?)).ok_or("after block absent")?;
   if p["member"]=="::SeedGlobalRandom"{state=seed(u(&p["seed_argument"])? as u32);index=624;seeds+=1;if u(&p["result"])?!=624{return Err("seed original return".into())}}
   else{
    if index>=624{twists+=1}if u(&p["result"])?!=u64::from(draw(&mut state,&mut index)?){return Err(format!("native original return mismatch:{p}"))}draws+=1;
    let caller=p["game_callers_rva"].as_array().ok_or("native callers")?;
    if caller.iter().any(|v|v.as_u64()==Some(0x241a05)){damocles+=1}
    if !caller.is_empty(){native_other+=1}
   }
   if state!=*after||index!=u(&a["index"])? as usize{return Err(format!("native MT post-state mismatch:{}",p["call"]))}
  }
  // Lua/cached两次返回值必须各对应一个原生记录，避免只验到Lua wrapper事件。
  let lua=get("lua_draws")?;let first=u(&lua["before"]["calls"])?;let last=u(&lua["after"]["calls"])?;
  if last-first!=2{return Err("Lua/cache calls missing or duplicated natively".into())}
  for value in lua["results"].as_array().unwrap(){if !calls.values().any(|p|p["member"]=="::Random"&&p["result"]==*value){return Err("Lua result absent from native calls".into())}}
  if draws<1303||seeds<2||twists<4||damocles<30||native_other<30||ordinary==0||epochs.len()!=1{return Err(format!("native coverage insufficient:draws={draws} seeds={seeds} twists={twists} damocles={damocles} ordinary={ordinary}"))}
  self.case("global_random_native_mt_replay",true,json!({"draws":draws,"seeds":seeds,"twists":twists,"blocks":blocks.len(),"ordinary":ordinary}));
  self.case("global_random_lua_cached_pause_readonly",true,json!({"draws":2,"observer_extra_draws":0}));
  self.case("global_random_damocles_native_calls",true,json!({"native_callsite_rva":"0x241A00","native_return_rva":"0x241A05","calls":damocles,"native_callers":native_other}));
  self.case("global_random_recorder_off_passthrough_restart",true,json!({"record_off_draws":24,"disabled_draws":24,"restart_draws":24}));
  self.case("global_random_reader_delivery",true,read);println!("tx14 native global RNG PASS draws={draws} seeds={seeds} twists={twists} damocles={damocles}");Ok(())
 }
}
