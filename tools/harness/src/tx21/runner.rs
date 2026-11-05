//! I38唯一协调owner；P2所有实际结果完成后才启动P4游戏，节点始终为真实游戏。
use super::{Driver,cases,compare,reduce,regression,proxy::{self,Proxy,Profile},node::{Peer,string,number}};
use crate::{Result,read,write_json,mkdir};
use serde_json::{Value,json};
use std::{collections::{BTreeMap,BTreeSet},fs::{self,File},io::{self,Write,BufRead,BufReader},path::{Path,PathBuf},thread,time::{Duration,Instant}};

// 人工只确认实际准备/画面/玩法语义；数值合同和缺测由程序判定，不能人工覆写失败。
fn prompt(kind:&str,instruction:&str)->Result<String>{let text=if let Some(answer)=super::interaction::prompt(kind,instruction)?{answer}else{println!("\n{instruction}");io::stdout().flush().map_err(|e|e.to_string())?;let mut text=String::new();if io::stdin().read_line(&mut text).map_err(|e|e.to_string())?==0{return Err("必要人工配合缺席；保留同tx恢复信息".into())}text.trim().to_owned()};if text=="stop"{return Err("用户请求必要中断".into())}Ok(text)}
fn confirm(instruction:&str)->Result<()>{if prompt("ready",&format!("{instruction}\n准备好后输入 ready；stop 仅在需要中断时使用。"))?!="ready"{return Err("准备尚未确认，未启动或发键".into())}Ok(())}
fn map(v:Value)->Result<BTreeMap<String,String>>{serde_json::from_value(v).map_err(|e|e.to_string())}
fn n(s:&BTreeMap<String,String>,key:&str)->Result<u64>{s.get(key).ok_or_else(||format!("缺少真实字段:{key}"))?.parse().map_err(|e|format!("{key}:{e}"))}
fn load_events(path:&Path)->Result<Vec<Value>>{let mut rows=Vec::new();let mut bytes=0usize;for line in BufReader::new(File::open(path).map_err(|e|e.to_string())?).lines(){let line=line.map_err(|e|e.to_string())?;bytes+=line.len();if rows.len()>=2000000||bytes>128*1024*1024||line.len()>1048576{return Err("case trace requires predeclared smaller segment;no truncation".into())}rows.push(serde_json::from_str(&line).map_err(|e|e.to_string())?);}Ok(rows)}

struct Live {
 nodes:Vec<Peer>,proxy:Option<Proxy>,directory:PathBuf,phase:String,case:Value,token:String,network:String,
 source:Option<Value>,native:BTreeMap<String,Value>,render:BTreeMap<String,Value>,windows:Vec<Value>,reviews:Vec<Value>,
 armed:Vec<Value>,measurements:Vec<Value>,closed:bool,product:bool,capacity:Option<Value>,
}
// 任一准备/观察异常也尝试关闭本方游戏；失败材料留在本次目录，绝不强杀外部进程。
impl Drop for Live {fn drop(&mut self){if !self.closed{let ids=self.nodes.iter().filter_map(|p|p.config["id"].as_str().map(str::to_owned)).collect::<Vec<_>>();for id in ids{if let Ok(peer)=self.node(&id){let _=peer.call("physical",json!({"keys":[],"hold_ms":0}));let _=peer.call("finish",json!({}));}}}}}
impl Live {
 fn node(&mut self,id:&str)->Result<&mut Peer>{self.nodes.iter_mut().find(|p|p.config["id"]==id).ok_or_else(||format!("真实节点缺席:{id}"))}
 fn command_ok(&mut self,id:&str,text:&str)->Result<BTreeMap<String,String>>{let r=self.command(id,text)?;if r.get("ok").map(String::as_str)!=Some("1"){return Err(format!("{id} {text}:{r:?}"))}Ok(r)}
 fn status(&mut self,id:&str)->Result<BTreeMap<String,String>>{self.command_ok(id,"cs status")}
 fn clients(&self)->Vec<String>{self.nodes.iter().filter(|p|p.config["role"]=="Client").filter_map(|p|p.config["id"].as_str().map(str::to_owned)).collect()}
 fn capacity(&mut self)->Result<Value>{
  // 只占用菜单握手槽的负向探针，不发送Ready，不产生参与者身体或有效世界。
  let address=format!("{}:{}",string(&self.nodes[0].config,"game_ipv4")?,number(&self.nodes[0].config,"host_port")?);
  let mut probes=Vec::new();let mut facts=Vec::new();let fill=3usize.saturating_sub(self.clients().len());let token=self.token.clone();
  for i in 0..fill+1{let socket=std::net::UdpSocket::bind("0.0.0.0:0").map_err(|e|e.to_string())?;socket.connect(&address).map_err(|e|e.to_string())?;socket.set_read_timeout(Some(Duration::from_millis(if i<fill{5000}else{500}))).map_err(|e|e.to_string())?;
   let hello=self.command_ok("host",&format!("cs stream_test {token} hello {}",0x21210001u64+i as u64))?;
   let before=self.status("host")?;let packets=hello.iter().filter(|(k,_)|k.starts_with("packet.")).map(|(_,v)|super::node::unhex(v)).collect::<Result<Vec<_>>>()?;if packets.is_empty(){return Err("容量探针没有真实编码Hello".into())}
   for packet in &packets{if socket.send(packet).map_err(|e|e.to_string())?!=packet.len(){return Err("容量探针未完整发送".into())}}
   let mut bytes=[0u8;1201];let received=socket.recv(&mut bytes);
   if i<fill{let count=received.map_err(|e|format!("菜单槽未产生真实Welcome:{e}"))?;let part=proxy::parts(&bytes[..count])?;if !part.iter().any(|p|p.kind==2){return Err("容量探针未获Welcome，不能冒称已填满".into())}facts.push(json!({"probe":i,"welcome":true,"bytes":count,"source":socket.local_addr().map_err(|e|e.to_string())?}));}
   else{if received.is_ok(){return Err("满菜单仍回复/接纳了第四个endpoint".into())}let until=Instant::now()+Duration::from_secs(5);let after=loop{let s=self.status("host")?;if n(&s,"net_foreign")?.saturating_sub(n(&before,"net_foreign")?)>=packets.len()as u64{break s}if Instant::now()>=until{return Err("第四Hello没有实际Host容量拒绝计数，不能按超时通过".into())}thread::sleep(Duration::from_millis(100));};facts.push(json!({"probe":i,"rejected":true,"before":before,"after":after,"sent_fragments":packets.len()}));}
   probes.push(socket);
  }let result=json!({"pass":true,"actual":facts,"real_game_clients":self.clients().len(),"probes":"negative handshake only;not gameplay/positive Client substitutes"});self.capacity=Some(result.clone());Ok(result)
 }
 fn wait(&mut self,id:&str,key:&str,value:&str,timeout:u64)->Result<BTreeMap<String,String>>{let until=Instant::now()+Duration::from_millis(timeout);loop{let s=self.status(id)?;if s.get(key).map(String::as_str)==Some(value){return Ok(s)}if Instant::now()>=until{return Err(format!("{id}等待{key}={value}超时:{s:?}"))}thread::sleep(Duration::from_millis(100));}}
 fn log(&mut self,id:&str)->Result<String>{let path=self.directory.join(format!("{id}-game.log"));self.node(id)?.download("game-log",&path)?;read(&path)}
 fn fixture_ready(&mut self,id:&str,action:&str)->Result<Value>{
  let until=Instant::now()+Duration::from_secs(90);loop{let log=self.log(id)?;for line in log.lines().rev(){if !line.contains(&self.token){continue}if let Some(at)=line.find('{'){if let Ok(row)=serde_json::from_str::<Value>(&line[at..]){if row["event"]=="fixture_error"{return Err(format!("真实Host夹具失败:{row}"))}if row["event"]=="action"&&row["kind"]==action||row["event"]=="ready"&&row["label"]==action{return Ok(row)}}}}if Instant::now()>=until{return Err(format!("Host真实动作未完成:{action}"))}thread::sleep(Duration::from_millis(200));}
 }
 fn collect(&mut self)->Result<Vec<Value>>{
  let mut rows=Vec::new();for i in 0..self.nodes.len(){let id=string(&self.nodes[i].config,"id")?.to_owned();let path=self.directory.join(format!("{id}-events.jsonl"));self.nodes[i].download("events",&path)?;let mut node_rows=load_events(&path)?;for row in &mut node_rows{row["case_id"]=self.case["id"].clone();row["network"]=json!(self.network);}
   rows.extend(node_rows);
  }Ok(rows)
 }
 fn save_facts(&mut self,name:&str,extra:Value)->Result<Value>{
  let mut nodes=Vec::new();for i in 0..self.nodes.len(){
   let id=string(&self.nodes[i].config,"id")?.to_owned();let window=self.node(&id)?.call("window",json!({}))?;
   if !window["exited"].is_null(){let log=self.log(&id)?;nodes.push(json!({"node":id,"window":window,"already_exited":true,"log_bytes":log.len(),"IPC_after_exit":"unavailable,never synthesized"}));}
   else if self.product{let screen=self.screenshot(&id,&Value::Null)?;let log=self.log(&id)?;nodes.push(json!({"node":id,"window":window,"screen":screen,"log_bytes":log.len(),"product_external_only":true}));}
   else{nodes.push(json!({"node":id,"window":window,"status":self.status(&id)?,"stream":self.command(&id,if i==0{"cs stream source 0"}else{"cs stream client 0"})?,"counters":self.command_ok(&id,"cs counters")?}));}
  }
  let value=json!({"check":name,"case":self.case["id"],"nodes":nodes,"extra":extra,"observation":"raw real game values;missing fields never default to zero"});let path=self.directory.join(format!("facts-{}.json",self.reviews.len()));write_json(&path,&value)?;Ok(json!({"path":path,"facts":value}))
 }
 fn review(&mut self,name:&str,extra:Value)->Result<Value>{
  let evidence=self.save_facts(name,extra)?;
  let answer=prompt("observed",&format!("{} / {name}：请审阅 {} 中的具名真实事实。核对本用例的动作、前后态、Key/版本、拒绝首因及实际画面。\n输入 observed 后跟具体观察（至少12个字符）；未观察到、缺测或不一致输入 fail 原因。不得把配置/调用成功当成实际结果。",self.case["id"],evidence["path"]))?;
  let note=answer.strip_prefix("observed ").ok_or_else(||format!("{name}实际观察未通过:{answer}"))?;if note.chars().count()<12{return Err("具体观察说明不足".into())}
  let result=json!({"pass":true,"method":"explicit-human-semantic-review","check":name,"evidence":evidence,"observed":note});self.reviews.push(result.clone());write_json(&self.directory.join("reviews.json"),&json!(self.reviews))?;Ok(result)
 }
 fn isolate(&mut self)->Result<Value>{
  let mut facts=Vec::new();for id in self.clients(){let s=self.status(&id)?;let c=self.command_ok(&id,"cs counters")?;
   for name in ["start_game","game_update","room_update","entity_list","odd_helper","restore","post_update","player_update","join_pass"]{if n(&s,&format!("client.original_begin.{name}"))?!=n(&c,&format!("hook.{name}"))?{return Err(format!("{id}非法玩法原调用:{name}"))}}
   if n(&s,"client.isolation.persistence_blocked")?!=1||n(&s,"client.player_random_denied")?>0{return Err(format!("{id}持久/随机隔离异常:{s:?}"))}
   facts.push(json!({"node":id,"status":s,"counters":c}));
  }Ok(json!({"pass":!facts.is_empty(),"facts":facts,"requires_final_files_and_shutdown_witness":true}))
 }
 fn index(&mut self,id:&str,revision:Option<&Value>,timeout:u64)->Result<Value>{let until=Instant::now()+Duration::from_millis(timeout);loop{let token=self.token.clone();let r=self.command_ok(id,&format!("cs stream_test {token} snapshot 0 0"))?;let rows:Value=serde_json::from_str(r.get("data").ok_or("snapshot_index")?).map_err(|e|e.to_string())?;if let Some(row)=rows.as_array().ok_or("snapshot_index_array")?.iter().find(|r|revision.is_none_or(|v|r["revision"]==*v)){return Ok(row.clone())}if Instant::now()>=until{return Err(format!("{id}精确版本未取得"))}thread::sleep(Duration::from_millis(50));}}
 fn aligned(&mut self,kind:u64)->Result<Value>{
  // 真实源继续推进；仅本次Client视觉中性段留住一个提交向量，不混入性能窗口。
  for id in self.clients(){self.physical(&id,&[],0)?;}self.physical("host",&[],0)?;
  if let Some(p)=&self.proxy{for i in 0..self.clients().len(){p.profile(i,Profile::VisualDelay,10000)?;}}
  let token=self.token.clone();self.command_ok("host",&format!("cs stream_test {token} arm source {kind} 0"))?;
  self.command_ok("host",&format!("cs stream_test {token} visual on"))?;
  let source_index=self.index("host",None,10000)?;let revision=source_index["revision"].clone();
  for id in self.clients(){self.command_ok(&id,&format!("cs stream_test {token} visual on"))?;self.command_ok(&id,&format!("cs stream_test {token} arm native {kind} {}",revision.as_str().map(str::to_owned).unwrap_or_else(||revision.to_string())))?;}
  let (source,_)=compare::read_snapshot(self,"host",&token,number(&source_index,"id")?)?;
  self.source=Some(source.clone());write_json(&self.directory.join("aligned-source.json"),&source)?;
  let result=(||->Result<Value>{let mut pairs=Vec::new();for id in self.clients(){let index=self.index(&id,Some(&revision),10000)?;let(snapshot,mut render_text)=compare::read_snapshot(self,&id,&token,number(&index,"id")?)?;
   if render_text.is_empty(){let rendered=self.index(&id,Some(&revision),10000)?;render_text=string(&rendered,"render")?.to_owned();}if render_text.is_empty(){return Err("匹配提交向量的真实Render未完成".into())}
   let render=json!({"connection":snapshot["connection"],"route":snapshot["route"],"world":snapshot["world"],"root_revision":snapshot["root"]["revision"],"actual_render":true,"witness":render_text});
   let comparison=compare::snapshot(&source,&snapshot,&render,false)?;
   // 先执行真实值/成员检查；画面由明确人工审阅补上，不能覆写机器差异。
   let numeric=compare::snapshot(&source,&snapshot,&render,true)?;if numeric["pass"]!=true{return Err(format!("同版本原生对照失败:{numeric}"))}
   self.native.insert(id.clone(),snapshot.clone());self.render.insert(id.clone(),render.clone());write_json(&self.directory.join(format!("aligned-{id}.json")),&snapshot)?;
   let screen=self.screenshot(&id,&render)?;let review=self.review("same-version-render",json!({"comparison":comparison,"machine":numeric,"screen":screen,"render":render}))?;
   pairs.push(json!({"node":id,"machine":numeric,"visual":review}));
  }Ok(json!({"pass":true,"pairs":pairs,"visual_neutral":true}))})();
  for id in self.clients(){let _=self.command_ok(&id,&format!("cs stream_test {token} visual off"));}
  let _=self.command_ok("host",&format!("cs stream_test {token} visual off"));
  if let Some(p)=&self.proxy{for i in 0..self.clients().len(){let _=p.profile(i,Profile::N0,0);}}
  result
 }
 fn fault(&mut self,point:&str,player:bool)->Result<Value>{
  let id=self.clients().first().cloned().ok_or("real Client required")?;let token=self.token.clone();
  self.command_ok(&id,&format!("cs client_test {token} {} {point}",if player{"player-fault"}else{"fault"}))?;
  self.review("reach-armed-native-boundary",json!({"point":point,"instruction":"Host触发相应结构/资源/最终值变化；必要时同连接完整恢复，禁止本地目标导入"}))?;
  let until=Instant::now()+Duration::from_secs(60);loop{let hit=self.command_ok(&id,&format!("cs client_test {token} {}",if player{"player-fault-status"}else{"fault-status"}))?;
   if hit.get("hit").map(String::as_str)==Some(point)&&n(&hit,"hits")?==1{return Ok(json!({"pass":true,"point":point,"hit":hit}))}
   if Instant::now()>=until{return Err(format!("真实故障边界未命中:{point}:{hit:?}"))}thread::sleep(Duration::from_millis(100));}
 }
 fn stop_stage(&mut self,p:&Value)->Result<Value>{
  let stage=string(p,"stage")?;let cause=string(p,"cause")?;
  self.review("position-real-stop-stage",json!({"stage":stage,"instruction":"以真实状态/原生前缀或所选transfer定位阶段，不能只按等待时长猜测"}))?;
  let id=self.clients().first().cloned().ok_or("client")?;
  match cause{"client-stop"=>{self.command_ok(&id,"cs stop")?;},"host-stop"=>{self.command_ok("host","cs stop")?;},"window-close"=>{self.node(&id)?.call("finish",json!({}))?;},"network-error"=>{if let Some(p)=&self.proxy{p.profile(0,Profile::DownlinkStop,31000)?;}thread::sleep(Duration::from_secs(31));},_=>return Err("stop cause".into())}
  self.review("actual-stop-and-retirement",json!({"stage":stage,"cause":cause}))
 }
 fn regression(&mut self,p:&Value)->Result<Value>{
  let row=&p["entry"];match string(row,"polarity")?{
   "positive"=>{let scenes=row["prefix"].as_array().ok_or("regression_prefix_required")?;for scene in scenes{let label=string(scene,"label")?;self.physical("host",&["F8".into()],60)?;self.physical("host",&[],60)?;self.fixture_ready("host",label)?;}self.aligned(5)},
   "fault"=>self.fault(string(row,"point")?,row["tx"]==20),
   "negative"=>{
    let name=string(row,"negative")?;let tx=number(row,"tx")?;let token=self.token.clone();let id=self.clients().first().cloned().ok_or("真实Client")?;
    if name.starts_with("cancel-")||matches!(name,"released-borrow"|"wrong-thread"|"wrong-context"|"secondary-vm"|"missing-grid-resource"){
     if matches!(name,"released-borrow"|"wrong-thread"|"wrong-context"){let actual=self.command_ok(&id,&format!("cs client_test {token} exercise ownership"))?;if actual.get("ownership_exercise").map(String::as_str)!=Some("1"){return Err(format!("真实owner检查失败:{actual:?}"))}return Ok(json!({"pass":true,"entry":row,"actual":actual}))}
     if name=="secondary-vm"{let actual=self.command_ok(&id,&format!("cs client_test {token} exercise vm"))?;if actual.get("vm_exercise").map(String::as_str)!=Some("1"){return Err(format!("真实VM检查失败:{actual:?}"))}return Ok(json!({"pass":true,"entry":row,"actual":actual}))}
     return self.review("named-native-retirement-stage",json!({"entry":row,"instruction":"按原场景定位真实前缀/缺Grid资源，实际触发正式停止或退休；命中与owner必须逐项审阅"}));
    }
    self.review("network-negative-prerequisites",json!({"entry":row,"instruction":"确认所需NPC、通用FX、sub/twin/backup及控制组确实在Host源和Client闭包中；缺前置必须先由Host布置"}))?;
    let actual=self.command_ok(&id,&format!("cs stream_test {token} reject {tx} {name}"))?;
    if actual.get("rejected").map(String::as_str)!=Some("1")||actual.get("A_preserved").map(String::as_str)!=Some("1")||actual.get("reason").is_none_or(|s|s.is_empty()){return Err(format!("真实网络派生负向未拒绝或破坏A:{actual:?}"))}
    self.review("network-negative-specific-reason-and-A",json!({"entry":row,"actual":actual,"layer":"native prewrite/capability;wire corruption separately in E01/E13/E17"}))
   },_=>Err("regression polarity".into())
  }
 }
 fn performance(&mut self)->Result<Value>{
  let rows=self.collect()?;let mut results=Vec::new();for mark in &self.windows{let result=reduce::window(&rows,mark)?;if result["pass"]!=true{return Err(format!("完整性能合同未达到:{result}"))}results.push(result);}
  if results.is_empty(){return Err("性能窗口缺测".into())}Ok(json!({"pass":true,"windows":results,"pipeline":reduce::pipeline(&rows)}))
 }
 fn finish(&mut self)->Result<Value>{
  if self.closed{return Ok(json!({"already_closed":true}))}self.restore()?;
  // 客户端先正式退休，再关闭真实进程；失败继续保留隔离，不强杀进程。
  let mut facts=Vec::new();let ids=self.nodes.iter().map(|p|p.config["id"].as_str().unwrap_or("").to_owned()).collect::<Vec<_>>();
  for id in ids.iter().rev(){let r=self.node(id)?.call("finish",json!({}))?;
   if r["game_exited"]!=true{return Err(format!("{id}真实进程尚未退出"))}
   if r["exit_code"]!=0{return Err(format!("{id}退出状态异常/缺测:{r}"))}
   if id!="host"{for row in r["save_changes"].as_array().ok_or("Client save comparison missing")?{let path=string(row,"path")?.to_lowercase();let volatile=path.ends_with("/log.txt")||path.ends_with("/cmd_history.txt")||path.ends_with(".ini")||path.contains("inputconfigs");if row["bytes_changed"]==true&&!volatile{return Err(format!("{id}访客持久字节改变:{row}"))}}}
   let trace_path=self.directory.join(format!("{id}-events.jsonl"));self.node(id)?.download("events",&trace_path)?;let _=self.log(id)?;facts.push(json!({"node":id,"result":r}));
  }
  if let Some(p)=self.proxy.take(){let summary=p.finish()?;write_json(&self.directory.join("proxy-result.json"),&summary)?;facts.push(summary);}
  self.closed=true;Ok(json!({"pass":true,"facts":facts}))
 }
}

impl Live {
 // 各机先取得真实连续空闲的Manager窗口；用户确实关闭UI后才固定采样起点。
 fn quiet_ui(&mut self,id:&str)->Result<BTreeMap<String,String>>{
  let token=self.token.clone();let mut wait_until=Instant::now()+Duration::from_secs(3);loop{
   let before=self.command_ok(id,&format!("cs stream_test {token} clock"))?;
   thread::sleep(Duration::from_millis(100));
   let after=self.command_ok(id,&format!("cs stream_test {token} clock"))?;
   if super::interaction::ui_unchanged(&before,&after)?{return Ok(after)}
   if Instant::now()<wait_until{continue}
   confirm(&format!("{id}：采样前请关闭控制台/ImGui和所有固定显示的本方面板，回到游戏前台并松开操作键；确认后保持关闭直到下一条操作提示。"))?;
   wait_until=Instant::now()+Duration::from_secs(3);
  }
 }
}
impl Driver for Live {
 fn command(&mut self,node:&str,text:&str)->Result<BTreeMap<String,String>>{map(self.node(node)?.call("command",json!({"text":text}))?)}
 fn physical(&mut self,node:&str,keys:&[String],hold_ms:u64)->Result<()>{
  if !keys.is_empty(){let state=self.node(node)?.call("window",json!({}))?;if state["focus"]["active"]!=true{confirm(&format!("请将 {node} 的已核实游戏窗口置前；停止手动输入后由编排器发送 {:?}。",keys))?;}}
  self.node(node)?.call("physical",json!({"keys":keys,"hold_ms":hold_ms}))?;Ok(())
 }
 fn human(&mut self,instruction:&str)->Result<()>{confirm(instruction)?;let answer=prompt("done","操作完成后关闭控制台/ImGui及固定显示窗口，松开操作键并停止手动操作，再输入 done；未完成输入 stop。");if answer?!="done"{return Err("人工窗口未完成".into())}Ok(())}
 fn sample(&mut self,milliseconds:u64,mark:&Value)->Result<Value>{
  let mut marks=Vec::new();let token=self.token.clone();let phase=self.phase.clone();let load=string(mark,"load")?;
  let expected=cases::loads();let spec=&expected[load];
  // 期望范围在计时前取完整已知目录；资格分母在结束后由真实source/witness计数填入。
  for i in 0..self.nodes.len(){let id=string(&self.nodes[i].config,"id")?.to_owned();let clock=self.command_ok(&id,&format!("cs stream_test {token} clock"))?;let start=n(&clock,"local_us")?;
   let mut flows=Vec::new();let target=if i==0{"source"}else{"client"};for page in 0..32{let r=self.command_ok(&id,&format!("cs stream {target} {page}"))?;let prefix=if i==0{"stream.source."}else{"stream.native."};for index in page*4..page*4+4{let base=format!("{prefix}range.{index}.");if let Some(kind)=r.get(&format!("{base}kind")).and_then(|s|s.parse::<u64>().ok()){if matches!(kind,5|6){let body=if kind==6{json!({"run":r.get(&format!("{base}body_run")),"id":r.get(&format!("{base}body_id")),"space":r.get(&format!("{base}body_space"))})}else{Value::Null};let range=json!({"kind":kind.to_string(),"name":r.get(&format!("{base}name")),"body":body});flows.push(json!({"range":range,"stage":if i==0{2}else{6}}));}}}}
   if flows.is_empty(){return Err(format!("{id}计时前完整范围目录缺席"))}
   marks.push(json!({"node":id,"role":self.nodes[i].config["role"],"case_id":self.case["id"],"phase":phase,"load":load,"network":self.network,"class":if load=="L4"{"damage"}else if load=="recovered"{"recovery"}else{"stable"},"start_us":start,"end_us":start+milliseconds*1000,"fixed_before_start":true,"expected_flows":flows,"expected60_bodies":[],"body_deltas":[]}));
  }
  // 目录枚举结束后才固定各机自己的时钟区间；不把枚举成本算作无来源缺口。
  // 所有需要人工关闭UI的准备先完成；固定首台起点后不再在别的机器弹准备提示。
  for mark in &marks{let id=string(mark,"node")?.to_owned();self.quiet_ui(&id)?;}
  for mark in &mut marks{let id=string(mark,"node")?.to_owned();let clock=self.command_ok(&id,&format!("cs stream_test {token} clock"))?;if !super::interaction::ui_quiet(&clock)?{return Err(format!("{id}固定采样起点时UI重新打开:{clock:?}"))}let start=n(&clock,"local_us")?;mark["start_us"]=json!(start);mark["end_us"]=json!(start+milliseconds*1000);mark["ui_before"]=json!(clock);}
  write_json(&self.directory.join("windows-pending.json"),&json!(marks))?;
  let deadline=Instant::now()+Duration::from_millis(milliseconds);let mut loads=Vec::new();let mut next_update=Instant::now();
  while Instant::now()<deadline{
   let log=self.log("host")?;let actual=log.lines().rev().filter_map(|l|l.find('{').and_then(|at|serde_json::from_str::<Value>(&l[at..]).ok())).find(|r|r["event"]=="load"&&r["token"]==self.token);
   if let Some(actual)=actual{if !spec.is_null(){for(key,target)in [("participants","participants"),("npc","npc_min"),("attacks","attack_min")]{if let Some(min)=spec[target].as_u64(){if number(&actual,key)?<min{return Err(format!("{phase}真实负载不足:{key}:{actual}"))}}}loads.push(actual);}else if !spec.is_null(){return Err("稳定段缺少实际Host负载见证".into())}
   if Instant::now()>=next_update{println!("{} {phase} {load}：剩余 {} 秒，真实节点持续采集。",self.case["id"],deadline.saturating_duration_since(Instant::now()).as_secs());next_update=Instant::now()+Duration::from_secs(30);}
   thread::sleep(Duration::from_millis(250));
  }
  for mark in &mut marks{
   let id=string(mark,"node")?.to_owned();let after=self.command_ok(&id,&format!("cs stream_test {token} clock"))?;
   let before=map(mark["ui_before"].clone())?;
   if !super::interaction::ui_unchanged(&before,&after)?{
    return Err(format!("{id}采样受到控制台/ImGui或本方面板渲染干扰；该窗口无效，必须修复后同tx完整重测；before={before:?};after={after:?}"))
   }
   mark["ui_after"]=json!(after);
  }
  let rows=self.collect()?;
  let host_start=number(&marks[0],"start_us")?;let host_end=number(&marks[0],"end_us")?;
  let mut body_counts=BTreeMap::<String,[BTreeSet<u64>;2]>::new();for row in &rows{if row["node"]!="host"||row["event"]!="body-witness"{continue}let t=number(row,"local_us")?;if t<host_start||t>=host_end{continue}let stage=number(row,"stage")?as usize;if stage>1{return Err("body witness stage".into())}let key=format!("{}/{}/{}/{}",row["local"],row["use"],row["shape"],row["binding"]);body_counts.entry(key).or_default()[stage].insert(number(row,"tick")?);}
  let body_deltas=body_counts.iter().map(|(body,stages)|json!({"body":body,"actual_updates":stages[0].len(),"qualified_samples":stages[0].intersection(&stages[1]).count(),"absence_reason":"real Host main-thread body witness;ordinary/odd absence remains explicit"})).collect::<Vec<_>>();
  for m in &mut marks{let seconds=(host_end-host_start)as f64/1e6;let mut fast=Vec::new();
   for f in m["expected_flows"].as_array_mut().ok_or("expected flows")?{let qualified=rows.iter().filter(|r|r["node"]=="host"&&r["event"]=="flow"&&r["range"]==f["range"]&&r["stage"]==1).filter_map(|r|number(r,"local_us").ok().map(|t|(t,r))).filter(|(t,_)|*t>=host_start&&*t<host_end).map(|(_,r)|r["revision"].to_string()).collect::<BTreeSet<_>>().len()as u64;f["eligible_source_samples"]=json!(qualified);
    if number(&f["range"],"kind")?==6&&qualified as f64/seconds>=58.0{fast.push(f["range"]["body"].clone());}
   }
   m["expected60_bodies"]=json!(fast);
   m["body_deltas"]=json!(body_deltas);m["qualification_owner"]=json!("Host local clock interval;never cross-device clock subtraction");
  }
  self.windows.extend(marks.clone());write_json(&self.directory.join("windows.json"),&json!(self.windows))?;let value=json!({"load":load,"marks":marks,"load_witness_samples":loads.len(),"minimum_duration_ms":milliseconds});self.measurements.push(value.clone());Ok(value)
 }
 fn network(&mut self,p:&Value)->Result<Value>{
  if p["negative"]=="full-room-capacity"{return self.capacity()}
  if self.armed.contains(p){let name=string(p,"negative")?;let id=if name=="session-identity"{"host"}else{"client1"};let token=self.token.clone();let until=Instant::now()+Duration::from_secs(20);loop{let hit=self.command_ok(id,&format!("cs stream_test {token} wire-status"))?;if hit.get("hit").map(String::as_str)==Some(name)&&n(&hit,"hits")?==1{return Ok(json!({"armed_before_handshake":true,"actual_sender_hit":hit,"requires_actual_receiver_rejection":true}))}if Instant::now()>=until{return Err(format!("Hello/Welcome具名变异未命中:{hit:?}"))}thread::sleep(Duration::from_millis(50));}}
  let proxy=self.proxy.as_ref().ok_or("真实UDP代理缺席")?;let peer=p["peer"].as_u64().unwrap_or(0)as usize;
  if let Some(profile)=p["profile"].as_str(){let kind=match profile{"N0"=>Some(Profile::N0),"N1"=>Some(Profile::N1),"downlink-2s"=>Some(Profile::DownlinkStop),"slow-0.5Mbps-30s"=>Some(Profile::Slow),"entity-overload"=>None,"N2"=>None,_=>return Err(format!("未知网络档:{profile}"))};if let Some(kind)=kind{self.network=profile.to_owned();return proxy.profile(peer,kind,p["duration_ms"].as_u64().unwrap_or(0))}}
  let name=p["negative"].as_str().or_else(||p["fault"].as_str()).or_else(||p["drop_kind"].as_str()).ok_or("具体网络故障名称缺席")?;
  let mut rule=proxy::Rule{peer,direction:proxy::Direction::Down,kind:None,lane:None,fragment:proxy::Fragment::Any,transfer:None,effect:proxy::Effect::Drop,remaining:p["max_hits"].as_u64().unwrap_or(1).min(20)as u16};
  use proxy::{Mutation as M,Effect as E,Fragment as F};
  match name{
   "first-fragment"=>{rule.kind=Some(6);rule.fragment=F::First},"middle-fragment"=>{rule.kind=Some(6);rule.fragment=F::Middle},"last-fragment"=>{rule.kind=Some(6);rule.fragment=F::Last},
   "receive-ack"|"ReceiveAck"=>{rule.kind=Some(7);rule.direction=proxy::Direction::Up},"applied-ack"|"AppliedAck"=>{rule.kind=Some(9);rule.direction=proxy::Direction::Up},"OperationAck"=>{rule.kind=Some(19);rule.direction=proxy::Direction::Up},
   "duplicate"=>rule.effect=E::Duplicate,"reorder"=>rule.effect=E::Delay(250),
   "old-route"|"old-fragment"=>rule.effect=E::Mutate(M::OldRoute),"wrong-connection"|"fake-identity"=>rule.effect=E::Mutate(M::WrongConnection),"wrong-transfer"|"cancel-transfer"=>rule.effect=E::Mutate(M::WrongTransfer),
   "same-fragment-conflict"=>rule.effect=E::Mutate(M::Conflict),"same-sequence-different-value"=>{rule.effect=E::Mutate(M::SameSequenceInput);rule.kind=Some(17);rule.direction=proxy::Direction::Up},
   "future-ACK"|"unsent-ACK"|"old-ACK"=>{rule.effect=E::Mutate(M::FutureAck);rule.kind=Some(9);rule.direction=proxy::Direction::Up},
   "length"=>rule.effect=E::Mutate(M::Length),"count"|"fragment-overflow"=>rule.effect=E::Mutate(M::Count),"reserved"=>rule.effect=E::Mutate(M::Reserved),"version"|"wire"|"framing"=>rule.effect=E::Mutate(M::Version),
   "compressed-frame"=>rule.effect=E::Mutate(M::CompressedFrame),"compressed-window"=>rule.effect=E::Mutate(M::CompressedWindow),"decompress-limit"=>rule.effect=E::Mutate(M::Expansion),
   // 没有等价变异器时保留具体未完成，不能把别的schema拒绝冒充本项命中。
   _=>return self.review("specific-real-network-injection",json!({"requested":p,"instruction":"执行本项具名原始报文注入；记录实际命中报文/层级/拒绝；不得以另一条schema错误替代"}))
  }
  proxy.rule(rule)
 }
 fn screenshot(&mut self,node:&str,version:&Value)->Result<Value>{let result=self.node(node)?.call("screen",json!({}))?;let path=self.directory.join(format!("{}-{}.bmp",node,self.reviews.len()));self.node(node)?.download("screen",&path)?;Ok(json!({"path":path,"actual_window":result,"version":version,"requires_visual_review":true}))}
 fn restore(&mut self)->Result<()>{for id in self.nodes.iter().map(|p|p.config["id"].as_str().unwrap_or("").to_owned()).collect::<Vec<_>>(){self.node(&id)?.call("physical",json!({"keys":[],"hold_ms":0}))?;let token=self.token.clone();let _=self.command(&id,&format!("cs stream_test {token} visual off"));}if let Some(p)=&self.proxy{for i in 0..self.clients().len(){p.profile(i,Profile::N0,0)?;}}Ok(())}
 fn observe(&mut self,check:&str,p:&Value,timeout:u64)->Result<Value>{
  match check{
   "negotiation-refused-before-large-allocation"=>{let id=self.clients().first().cloned().ok_or("真实Client")?;if let Some(capacity)=self.capacity.clone(){let s=self.status(&id)?;if n(&s,"client_world_applied")?!=0{return Err("容量负向在菜单前提不成立".into())}return self.review(check,json!({"status":s,"actual_capacity":capacity}))}let until=Instant::now()+Duration::from_millis(timeout);loop{let s=self.status(&id)?;if matches!(s.get("client_state").map(String::as_str),Some("Retiring"|"Failed"|"Off"))&&n(&s,"client_world_applied")?==0{return self.review(check,json!({"status":s,"wire":p}))}if Instant::now()>=until{return Err(format!("错误协商未在世界分配前拒绝:{s:?}"))}thread::sleep(Duration::from_millis(100));}},
   "host-fixture-action"=>{let action=string(p,"action")?;let result=self.fixture_ready("host",action)?;Ok(json!({"pass":true,"actual":result}))},
   "physical-input-script"=>{let ordinal=number(p,"participant")?as usize;let id=self.nodes.get(ordinal).ok_or("真实参与者缺席")?.config["id"].as_str().ok_or("node id")?.to_owned();let keys: BTreeMap<String,String>=serde_json::from_value(self.nodes[ordinal].config["keys"].clone()).map_err(|e|e.to_string())?;super::physical_script(self,&id,string(p,"script")?,&keys)?;self.review(check,p.clone())},
   "same-version-full-native-and-visual"|"source-dto-native-field-by-field"|"complete-members-relations-resources"=>self.aligned(5),
   "align-version"=>self.aligned(if p["kind"]=="player-post"{6}else{5}),
   "no-client-gameplay-random-persistence"|"client-no-second-judgement"=>self.isolate(),
   "full-window-isolation"=>{self.isolate()?;let token=self.token.clone();for id in self.clients(){let exercise=self.command_ok(&id,&format!("cs client_test {token} exercise native"))?;if exercise.get("isolation_exercise").map(String::as_str)!=Some("1"){return Err(format!("真实隔离入口演练:{exercise:?}"))}}self.review(check,p.clone())},
   "diagnostics-failure-independent"=>{let token=self.token.clone();for id in self.clients(){let before=self.status(&id)?;self.command_ok(&id,&format!("cs stream_test {token} observation-loss"))?;thread::sleep(Duration::from_millis(200));let after=self.status(&id)?;if n(&after,"client.native_attempt")?<=n(&before,"client.native_attempt")?{return Err("诊断失败后真实提交未继续".into())}}Ok(json!({"pass":true,"notification_only":true}))},
   "arm-native-fault"=>{let point=string(p,"point")?;if point=="missing-field"||point=="missing-resource"{self.review(check,p.clone())}else{self.fault(point,false)}},
   "arm-player-fault"=>self.fault(string(p,"point")?,true),
   "repeat-latest-exact-target"=>{let count=number(p,"count")?;if count==0||count>20{return Err("重复目标次数越界".into())}let proxy=self.proxy.as_ref().ok_or("实际UDP代理")?;let mut actual=Vec::new();for peer in 0..self.clients().len(){actual.push(proxy.replay(peer,count as u16)?);}self.review(check,json!({"actual":actual,"instruction":"按replay_actual的arrival范围核对每个副本实际send、重复ACK/重组计数与native构造/资源计数；排队不算送达"}))},
   "stop-at-real-stage"=>self.stop_stage(p),
   "lifetime-fault"=>{let token=self.token.clone();let id=self.clients().first().cloned().ok_or("client")?;match string(p,"kind")?{"lua-generation"=>{let r=self.command_ok(&id,&format!("cs client_test {token} exercise vm"))?;if r.get("vm_exercise").map(String::as_str)!=Some("1"){return Err(format!("VM故障:{r:?}"))}},"old-instance"|"context-generation"=>{let r=self.command_ok(&id,&format!("cs client_test {token} exercise ownership"))?;if r.get("ownership_exercise").map(String::as_str)!=Some("1"){return Err(format!("借用故障:{r:?}"))}},_=>{}}self.review(check,p.clone())},
   "network-regression-entry"=>self.regression(p),
   "frequency-coverage-gap-cpu-bandwidth-contract"=>self.performance(),
   "actual-input-consumers-and-edges"=>{let rows=self.collect()?;let result=reduce::inputs(&rows);let expected_edge=self.case["id"].as_str().is_some_and(|s|["-bomb","-active","-card-pill","-drop"].iter().any(|suffix|s.ends_with(suffix)));let base=result["expired_ratio"].as_f64().is_some_and(|r|r<0.01)&&result["native_consumers"].as_object().is_some_and(|v|!v.is_empty())&&result["missing"]==0;if !base||expected_edge&&result["pass"]!=true{return Err(format!("实际输入/边沿不满足合同:{result}"))}self.review(check,json!({"actual":result,"edge_required_this_case":expected_edge}))},
   "actual-load-through-stable-window"=>{if self.measurements.is_empty(){return Err("未执行真实稳定窗口".into())}Ok(json!({"pass":true,"measurements":self.measurements}))},
   "product-restart-without-fixture"=>{if !self.product{return Err("未实际启动产品DLL".into())}self.review(check,json!({"external_only":true,"fixture_disabled":true}))},
   _=>{let _=timeout;self.review(check,p.clone())}
  }
 }
}

// 每个case新的实际会话便于冷端、停止/故障和来源前缀完全归属；不提供单项试跑入口。
fn setup(config:&Value,case:&Value,phase:&str,directory:PathBuf)->Result<Live>{
 let count=if phase=="p2"{2}else{4};let all=config["nodes"].as_array().ok_or("真实节点目录")?;if all.len()!=4{return Err("明确Host和三个真实Client；P2只启动前两个游戏".into())}mkdir(&directory)?;
 let token=string(config,"e2e_token")?.to_owned();let mut nodes=Vec::new();let mut machines=BTreeSet::new();
 for cfg in all.iter().take(count){let mut peer=Peer::connect(cfg.clone())?;let hello=peer.call("hello",json!({}))?;if hello["role"]!=cfg["role"]||hello["node"]!=cfg["id"]||!machines.insert(string(&hello,"machine")?.to_lowercase()){return Err(format!("真实跨设备身份重复/错误:{hello}"))}nodes.push(peer);}
 let host_address=config["host_endpoint"].as_str().ok_or("真实Host socket")?.parse().map_err(|e|format!("Host endpoint:{e}"))?;
 let mut links=Vec::new();for peer in nodes.iter().skip(1){links.push(proxy::Link{listen:string(&peer.config,"proxy_listen")?.parse().map_err(|e|format!("proxy listen:{e}"))?,host:host_address,client_ip:string(&peer.config,"game_ipv4")?.parse().map_err(|e|format!("client IP:{e}"))?});}
 let proxy=Proxy::start(links,number(config,"proxy_seed")?,&directory.join("proxy.jsonl"))?;
 let mut live=Live{nodes,proxy:Some(proxy),directory,phase:phase.into(),case:case.clone(),token,network:"N0".into(),source:None,native:BTreeMap::new(),render:BTreeMap::new(),windows:Vec::new(),reviews:Vec::new(),armed:Vec::new(),measurements:Vec::new(),closed:false,product:false,capacity:None};
 let product=case["e"]=="E24"||case["id"].as_str().is_some_and(|s|s.starts_with("E23.product-"));let flavor=if product{"product"}else{"e2e"};
 live.product=product;
 for i in 0..count{let id=string(&live.nodes[i].config,"id")?.to_owned();live.nodes[i].call("install",json!({"flavor":flavor}))?;
  confirm(&format!("{phase} {}：即将在 {id} 启动真实 {flavor} 游戏。只跳开场，到标题/选档页停止手动操作。{}",case["id"],if live.nodes[i].config["allow_background"]==true{"本次按配置暂设PauseOnFocusLost=0，结束还原原文件。"}else{"保持现有失焦设置。"}))?;
  let legacy=case[phase].as_array().into_iter().flatten().find(|s|s["check"]=="network-regression-entry").map(|s|s["entry"].clone());
  let actions=case[phase].as_array().into_iter().flatten().filter(|s|s["check"]=="host-fixture-action").map(|s|s["action"].clone()).collect::<Vec<_>>();
  let fixture=if let Some(row)=legacy.filter(|r|r["polarity"]=="positive"){json!({"tx":format!("tx-04-{}",number(&row,"tx")?),"actions":[],"scenes":row["prefix"]})}else{json!({"tx":"tx-04-21","actions":actions,"scenes":[]})};
  let launch=live.nodes[i].call("launch",json!({"participants":count,"flavor":flavor,"fixture":fixture}))?;write_json(&live.directory.join(format!("launch-{id}.json")),&launch)?;
  live.human(&format!("{id}：只跳过开场；到标题/选档页后停止操作。确认窗口/PID与 {} 一致。",launch["pid"]))?;
  if !product{live.command_ok(&id,&format!("cs echo {}",live.token.clone()))?;live.nodes[i].call("trace-on",json!({}))?;}
 }
 if product{live.human("产品构建：通过正式UI连接；Host和所有Client先在菜单完成连接，再由Host在明确选择的存档槽开始新run；停止手动操作后执行本用例。禁止夹具/控制口/文件导入。")?;return Ok(live)}
 let host_port=number(&live.nodes[0].config,"host_port")?;live.command_ok("host",&format!("cs host {host_port} 0"))?;
 let early=case[phase].as_array().into_iter().flatten().find(|s|s["op"]=="network"&&s["negative"].as_str().is_some_and(|name|["wire","stream-schema","compact-schema","resource-declaration","session-identity","framing","capability-missing"].contains(&name))).cloned();
 if let Some(step)=&early{let name=string(step,"negative")?;let sender=if name=="session-identity"{"host"}else{"client1"};let token=live.token.clone();live.command_ok(sender,&format!("cs stream_test {token} wire {name}"))?;live.armed.push(step.clone());}
 for i in 1..count{let id=string(&live.nodes[i].config,"id")?.to_owned();let address=string(&live.nodes[i].config,"proxy_connect")?.parse::<std::net::SocketAddr>().map_err(|e|e.to_string())?;let device=number(&live.nodes[i].config,"device_key")?;live.command_ok(&id,&format!("cs client {} {} {device}",address.ip(),address.port()))?;if early.is_none(){live.wait(&id,"client_state","AwaitingWorld",90000)?;}}
 if case["initial"]["load"]!="menu"&&case["e"]!="E26"{live.human("Host：在已明确的测试存档槽开始新run；所有Client保持菜单等待网络准备，不载入本地run/Continue。到Basement普通房后停止手动操作。")?;for id in live.clients(){live.wait(&id,"client_world_applied","1",90000)?;}}
 Ok(live)
}

pub(super) fn run(config_path:&Path,out:&Path)->Result<()>{
 let config:Value=serde_json::from_str(&read(config_path)?).map_err(|e|e.to_string())?;mkdir(out)?;
 super::interaction::initialize(&config)?;
 let nodes=config["nodes"].as_array().ok_or("真实拓扑节点")?;let mut endpoints=BTreeSet::new();
 if nodes.len()!=4{return Err("明确四个真实节点，P2仅启动前两台".into())}
 for(i,expected)in ["host","client1","client2","client3"].iter().enumerate(){let role=if i==0{"Host"}else{"Client"};if nodes[i]["id"]!=*expected||nodes[i]["role"]!=role||!endpoints.insert(string(&nodes[i],"agent_endpoint")?){return Err("节点顺序/角色/独立endpoint错误".into())}}
 let progress_path=out.join("progress.json");let mut progress:Value=if progress_path.exists(){serde_json::from_str(&read(&progress_path)?).map_err(|e|e.to_string())?}else{json!({"format":"isaac.cs.tx21.progress","version":1,"p2":{},"p4":{},"failures":[],"complete":false})};
 let reuse=config["reuse_passes"]==true;if reuse&&config["reuse_reason"].as_str().is_none_or(|s|s.trim().is_empty()){return Err("复用必须说明未受影响原因，不比较哈希".into())}
 for phase in ["p2","p4"]{
  if phase=="p4"{for case in cases::phase_suite("p2")?{let id=string(&case,"id")?;if progress["p2"][id]["pass"]!=true{return Err(format!("P2仍未完成:{id}；未启动P4游戏"))}}}
  let suite=cases::phase_suite(phase)?;write_json(&out.join(format!("{phase}-cases.json")),&json!(suite))?;write_json(&out.join("regression-inventory.json"),&json!(regression::inventory()?))?;
  for case in suite{let id=string(&case,"id")?.to_owned();let rerun=config["rerun"].as_array().is_some_and(|v|v.iter().any(|x|x==&json!(format!("{phase}/{id}"))||x==&json!(phase)));
   super::interaction::context(&format!("tx-04-21 / {phase} / {id}"))?;
   if reuse&&!rerun&&progress[phase][&id]["pass"]==true{continue}
   if case["e"]=="E26"{
    for prior in cases::phase_suite(phase)?.iter().filter(|c|c["e"]!="E26"){if progress[phase][string(prior,"id")?]["pass"]!=true{return Err(format!("{phase}子用例未完成:{prior}"))}}
    progress[phase][&id]=json!({"pass":true,"method":"all-declared-subcases-and-cleanup-completed","runtime_final_review":prompt("reviewed",&format!("{phase} E26：核对全部指标、图像、首因与续测及节点恢复记录；输入 reviewed 加具体结论，缺项输入 stop。"))?});
    if !progress[phase][&id]["runtime_final_review"].as_str().is_some_and(|s|s.starts_with("reviewed ")&&s.chars().count()>20){progress[phase][&id]["pass"]=json!(false);write_json(&progress_path,&progress)?;return Err("E26人工总审未完成".into())}write_json(&progress_path,&progress)?;continue;
   }
   let mut attempt=1u64;loop{
    let dir=out.join(format!("{phase}-{id}-{attempt}"));if dir.exists(){attempt+=1;continue}mkdir(&dir)?;
    progress[phase][&id]=json!({"state":"RUNNING","pass":false,"attempt":attempt,"directory":dir});write_json(&progress_path,&progress)?;
    let result=(||->Result<Value>{let mut live=setup(&config,&case,phase,dir.clone())?;let execution=super::execute(&mut live,&case,phase)?;let cleanup=live.finish();let pass=execution["pass"]==true&&cleanup.is_ok();Ok(json!({"pass":pass,"execution":execution,"cleanup":cleanup.as_ref().ok(),"cleanup_failure":cleanup.err(),"attempt":attempt,"directory":dir}))})();
    let result=match result{Ok(r)=>r,Err(e)=>json!({"pass":false,"failure":e,"attempt":attempt,"directory":dir})};write_json(&dir.join("result.json"),&result)?;progress[phase][&id]=result.clone();
    if result["pass"]==true{write_json(&progress_path,&progress)?;break}
    progress["failures"].as_array_mut().ok_or("failure history")?.push(json!({"phase":phase,"id":id,"result":result}));write_json(&progress_path,&progress)?;
    println!("同tx实际失败：{result}。请持续定位/修复；不降低负载、不跳过子项、不覆盖首因。");
    let decision=prompt("retry","修复完成且本次二进制/环境仍适合续测，输入 retry 加修复说明；需要重构建/部署或缺少配合时输入 stop，使用同一入口 -Resume 并显式列出受影响项。")?;
    if !decision.starts_with("retry ")||decision.chars().count()<12{return Err("必要中断；进度/失败/节点恢复记录已保留，同tx修复续测".into())}progress["failures"].as_array_mut().unwrap().last_mut().unwrap()["repair"]=json!(decision);write_json(&progress_path,&progress)?;attempt+=1;
   }
  }
 }
 for cfg in config["nodes"].as_array().ok_or("nodes")?{let mut peer=Peer::connect(cfg.clone())?;peer.call("shutdown",json!({}))?;}
 progress["complete"]=json!(true);write_json(&progress_path,&progress)?;
 println!("P2与P4全部实际子项已完成；返回统一脚本恢复部署与清理临时目录。只有脚本恢复成功才结束本次验收。");Ok(())
}
