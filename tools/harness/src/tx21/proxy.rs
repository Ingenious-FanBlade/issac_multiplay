//! 有界用户态代理；转发真实ICRF v2/ICRB v2，不创建有效世界或代替Client。
use crate::Result;
use serde_json::{Value,json};
use std::{collections::VecDeque,fs::File,io::Write,net::{SocketAddr,UdpSocket,IpAddr},path::Path,
 sync::{Arc,atomic::{AtomicBool,Ordering},mpsc::{self,SyncSender,Receiver}},thread::{self,JoinHandle},time::{Duration,Instant}};

#[derive(Clone,Copy,Debug,PartialEq)]
pub enum Profile{N0,N1,DownlinkStop,Slow}
#[derive(Clone,Copy,Debug,PartialEq)]
pub enum Direction{Down,Up}
#[derive(Clone,Copy,Debug)]
pub enum Mutation{OldRoute,WrongConnection,WrongTransfer,Conflict,SameSequenceInput,Length,Count,Reserved,Version,CompressedFrame,CompressedWindow,Expansion,FutureAck}
#[derive(Clone,Copy,Debug)]
pub enum Effect{Drop,Duplicate,Delay(u64),Mutate(Mutation)}
#[derive(Clone,Copy,Debug)]
pub enum Fragment{Any,First,Middle,Last}
#[derive(Clone,Debug)]
pub struct Rule{pub peer:usize,pub direction:Direction,pub kind:Option<u16>,pub lane:Option<u8>,pub fragment:Fragment,pub transfer:Option<u64>,pub effect:Effect,pub remaining:u16}
#[derive(Clone)]
pub struct Link{pub listen:SocketAddr,pub host:SocketAddr,pub client_ip:IpAddr}
#[derive(Clone,Debug)]
pub struct Part{pub kind:u16,pub lane:u8,pub transfer:u64,pub index:u32,pub count:u32,pub start:usize,pub payload:usize,pub codec:u8}
fn read(b:&[u8],at:usize,n:usize)->Result<u64>{let end=at.checked_add(n).ok_or("proxy_offset_overflow")?;let s=b.get(at..end).ok_or("proxy_truncated")?;Ok(s.iter().enumerate().fold(0,|v,(i,x)|v|((*x as u64)<<(8*i))))}
// 只解析本地已知固定头；长度/消息数有界后才保存任何延迟副本。
pub fn parts(b:&[u8])->Result<Vec<Part>>{
 if b.len()>1200{return Err("proxy_datagram_limit".into())}
 if b.starts_with(b"ICRF"){
  if b.len()<64||read(b,4,2)?!=2||read(b,6,2)?!=64||read(b,52,2)? as usize!=b.len()-64{return Err("proxy_fragment_header".into())}
  let index=read(b,40,4)? as u32;let count=read(b,44,4)? as u32;if count==0||count>32768||index>=count{return Err("proxy_fragment_count".into())}
  Ok(vec![Part{kind:read(b,48,2)? as u16,lane:b[50],transfer:read(b,24,8)?,index,count,start:0,payload:64,codec:b[51]}])
 }else if b.starts_with(b"ICRB"){
  if b.len()<28||read(b,4,2)?!=2||read(b,6,2)?!=28{return Err("proxy_bundle_header".into())}let count=read(b,24,2)?;if count==0||count>16{return Err("proxy_bundle_count".into())}let mut at=28;let mut out=Vec::new();
  for _ in 0..count{let n=read(b,at,2)? as usize;if n<2||at+22+n>b.len(){return Err("proxy_bundle_length".into())}out.push(Part{kind:read(b,at+2,2)? as u16,lane:b[at+4],transfer:read(b,at+14,8)?,index:0,count:1,start:at,payload:at+22,codec:0});at+=22+n;}
  if at!=b.len(){return Err("proxy_bundle_trailing".into())}Ok(out)
 }else{Err("proxy_unknown_framing".into())}
}
fn put(b:&mut[u8],at:usize,n:usize,v:u64)->Result<()>{b.get_mut(at..at+n).ok_or("proxy_mutation_bounds")?.copy_from_slice(&v.to_le_bytes()[..n]);Ok(())}
// ACK中的versions和results.version必须同时改成同一新值，才能真实检查Host的已发送历史。
// 泛型树只遍历已有小控制包，保留全部其他字节；深度和节点数均有固定界限。
fn ack_tree(b:&[u8],at:&mut usize,out:&mut Vec<u8>,revision_key:bool,old:u64,new:u64,depth:usize,nodes:&mut usize,hits:&mut usize)->Result<()>{
 if depth>32||*nodes>=4096{return Err("proxy_ack_tree_budget".into())}*nodes+=1;let tag=read(b,*at,1)? as u8;*at+=1;out.push(tag);match tag{
  0..=2=>{},3..=5=>{let v=read(b,*at,8)?;*at+=8;let v=if revision_key&&tag==3&&v==old{*hits+=1;new}else{v};out.extend_from_slice(&v.to_le_bytes());},
  6=>{let len=read(b,*at,4)? as usize;if len>4096{return Err("proxy_ack_string_limit".into())}*at+=4;let text=b.get(*at..*at+len).ok_or("proxy_ack_string")?;*at+=len;if revision_key&&text==old.to_string().as_bytes(){let text=new.to_string();out.extend_from_slice(&(text.len() as u32).to_le_bytes());out.extend_from_slice(text.as_bytes());*hits+=1;}else{out.extend_from_slice(&(len as u32).to_le_bytes());out.extend_from_slice(text);}},
  7|8=>{let count=read(b,*at,4)? as usize;*at+=4;if count>4096{return Err("proxy_ack_container".into())}out.extend_from_slice(&(count as u32).to_le_bytes());for _ in 0..count{let mut is_revision=false;if tag==8{let key=read(b,*at,1)? as u8;*at+=1;out.push(key);is_revision=key==14;if key==0{let len=read(b,*at,4)? as usize;let end=(*at).checked_add(4+len).ok_or("proxy_ack_key_length")?;let raw=b.get(*at..end).ok_or("proxy_ack_key")?;is_revision=&raw[4..]==b"revision";out.extend_from_slice(raw);*at=end;}}ack_tree(b,at,out,is_revision,old,new,depth+1,nodes,hits)?;}},
  _=>return Err("proxy_ack_tree_tag".into())
 }Ok(())
}
fn mutate(b:&mut Vec<u8>,part:&Part,m:Mutation)->Result<()>{
 let bundle=b.starts_with(b"ICRB");match m{
  Mutation::OldRoute=>{let old=read(b,16,8)?;put(b,16,8,old.saturating_sub(1))?;},
  Mutation::WrongConnection=>{let old=read(b,8,8)?;put(b,8,8,old.wrapping_add(1))?;},
  Mutation::WrongTransfer=>put(b,if bundle{part.start+14}else{24},8,part.transfer.wrapping_add(1000000))?,
  Mutation::Length=>{let at=if bundle{part.start}else{52};let old=read(b,at,2)?;put(b,at,2,old+1)?;},
  Mutation::Reserved=>{let at=if bundle{part.start+5}else{54};b[at]=1;},
  Mutation::Version=>b[4]=255,
  Mutation::Conflict=>{let last=b.len()-1;b[last]^=1;},
  Mutation::SameSequenceInput=>{if part.kind!=17||part.codec!=0||part.count!=1{return Err("proxy_requires_complete_uncompressed_input".into())}let at=if bundle{part.payload}else{104};if read(b,at,2)?!=1{return Err("proxy_input_schema".into())}let x=read(b,at+34,2)? as u16 as i16;let y=read(b,at+36,2)? as u16 as i16;let changed=if x!=0{-x}else if y==0{32767}else{0};if x==0&&y!=0{put(b,at+36,2,(-y)as u16 as u64)?;}else{put(b,at+34,2,changed as u16 as u64)?;}},
  Mutation::Count=>{if bundle{put(b,24,2,17)?;}else{put(b,44,4,u32::MAX as u64)?;}},
  Mutation::CompressedFrame=>{if bundle||part.codec!=1||part.index!=0{return Err("proxy_requires_first_compressed_fragment".into())}b[64]^=0xff;},
  Mutation::CompressedWindow=>{if bundle||part.codec!=1||part.index!=0||b.len()<70{return Err("proxy_requires_compressed_header".into())}b[68]&=!0x20;b[69]=0xff;},
  Mutation::Expansion=>{if bundle||part.codec!=1{return Err("proxy_requires_compressed_fragment".into())}put(b,36,4,u32::MAX as u64)?;},
  Mutation::FutureAck=>{
   if part.kind!=9||part.codec!=0||part.count!=1{return Err("proxy_requires_complete_uncompressed_applied_ack".into())}
   // 紧凑payload schema1 + 六个world u64 + versions数；只改真实版本声明，不猜末字节语义。
   let payload=if bundle{part.payload}else{104};if read(b,payload,2)?!=1||read(b,payload+50,1)?==0{return Err("proxy_ack_schema_or_empty".into())}
   let count=read(b,payload+50,1)? as usize;if count>64{return Err("proxy_ack_versions_limit".into())}let mut at=payload+51;let mut revision=0;for i in 0..count{let len=read(b,at+1,4)? as usize;if len>128{return Err("proxy_ack_range_name".into())}let r=at+5+len+17;if i==0{revision=r;}let _=read(b,r,8)?;at=r+8;}
   let old=read(b,revision,8)?;let new=old.checked_add(1000000).filter(|v|*v<u64::MAX).ok_or("proxy_ack_revision_overflow")?;put(b,revision,8,new)?;
   let start=at;let mut rewritten=Vec::new();let mut nodes=0;let mut hits=0;ack_tree(b,&mut at,&mut rewritten,false,old,new,0,&mut nodes,&mut hits)?;if hits==0{return Err("proxy_ack_result_revision_not_found".into())}
   let before=b.len();b.splice(start..at,rewritten);let delta=b.len() as i64-before as i64;if b.len()>1200{return Err("proxy_mutated_ack_requires_fragmentation".into())}
   if bundle{let size=read(b,part.start,2)? as i64+delta;put(b,part.start,2,size as u64)?;}else{let wire=read(b,32,4)? as i64+delta;let inner=read(b,76,4)? as i64+delta;let payload_size=b.len()-64;put(b,32,4,wire as u64)?;put(b,36,4,wire as u64)?;put(b,52,2,payload_size as u64)?;put(b,76,4,inner as u64)?;}
  },
 }Ok(())
}
struct Bucket{rate:f64,burst:f64,tokens:f64,at:Instant}
impl Bucket{fn new(rate:u64,burst:u64)->Self{Self{rate:rate as f64,burst:burst as f64,tokens:burst as f64,at:Instant::now()}}fn fill(&mut self,now:Instant){self.tokens=(self.tokens+now.duration_since(self.at).as_secs_f64()*self.rate).min(self.burst);self.at=now;}fn has(&mut self,n:usize,now:Instant)->bool{self.fill(now);self.tokens>=n as f64}fn take(&mut self,n:usize){self.tokens-=n as f64}}
struct Peer{front:UdpSocket,back:UdpSocket,client:Option<SocketAddr>,ip:IpAddr,profile:Profile,until:Option<Instant>,down:Bucket,up:Bucket,random:[u64;2]}
struct Pending{due:Instant,peer:usize,direction:Direction,bytes:Vec<u8>,arrival:u64,copy:u16}
enum Request{Profile(usize,Profile,u64,mpsc::Sender<Result<Value>>),Rule(Rule,mpsc::Sender<Result<Value>>)}
pub struct Proxy{tx:SyncSender<Request>,stop:Arc<AtomicBool>,worker:Option<JoinHandle<Result<Value>>>,pub addresses:Vec<SocketAddr>}
impl Proxy{
 pub fn start(links:Vec<Link>,seed:u64,log:&Path)->Result<Self>{
  if links.is_empty()||links.len()>3||seed==0{return Err("proxy_nodes_or_seed".into())}let mut peers=Vec::new();let mut addresses=Vec::new();
  for (i,l) in links.iter().enumerate(){if !l.listen.is_ipv4()||!l.host.is_ipv4()||!l.client_ip.is_ipv4(){return Err("proxy_ipv4_only".into())}let front=UdpSocket::bind(l.listen).map_err(|e|e.to_string())?;let back=UdpSocket::bind(SocketAddr::new(l.listen.ip(),0)).map_err(|e|e.to_string())?;back.connect(l.host).map_err(|e|e.to_string())?;front.set_nonblocking(true).map_err(|e|e.to_string())?;back.set_nonblocking(true).map_err(|e|e.to_string())?;addresses.push(front.local_addr().map_err(|e|e.to_string())?);peers.push(Peer{front,back,client:None,ip:l.client_ip,profile:Profile::N0,until:None,down:Bucket::new(312500,8192),up:Bucket::new(50000,8192),random:[seed.wrapping_add(i as u64*2+1).max(1),seed.wrapping_add(i as u64*2+2).max(1)]});}
  let log=File::options().write(true).create_new(true).open(log).map_err(|e|e.to_string())?;let (tx,rx)=mpsc::sync_channel(64);let stop=Arc::new(AtomicBool::new(false));let flag=stop.clone();
  let topology=links.iter().map(|l|json!({"listen":l.listen,"host":l.host,"client_ip":l.client_ip})).collect::<Vec<_>>();
  let worker=thread::Builder::new().name("tx21-real-udp".into()).spawn(move||{let mut w=Worker{peers,log,clock:Instant::now(),total:Bucket::new(1250000,24576),queue:VecDeque::new(),bytes:0,rules:vec![],serial:0,counts:[0;7],round:0};w.record(json!({"event":"start","seed":seed,"topology":topology,"prng":"xorshift64-per-peer-direction","peer_byte_s":312500,"host_total_byte_s":1250000}))?;w.run(rx,flag)}).map_err(|e|e.to_string())?;
  Ok(Self{tx,stop,worker:Some(worker),addresses})
 }
 pub fn profile(&self,peer:usize,p:Profile,duration_ms:u64)->Result<Value>{if peer>=self.addresses.len()||matches!(p,Profile::DownlinkStop|Profile::Slow)&&duration_ms==0{return Err("proxy_profile_arguments".into())}let(tx,rx)=mpsc::channel();self.tx.try_send(Request::Profile(peer,p,duration_ms,tx)).map_err(|e|e.to_string())?;rx.recv_timeout(Duration::from_secs(3)).map_err(|e|e.to_string())?}
 pub fn rule(&self,r:Rule)->Result<Value>{if r.peer>=self.addresses.len()||r.remaining==0||r.remaining>20{return Err("proxy_rule_bound".into())}let(tx,rx)=mpsc::channel();self.tx.try_send(Request::Rule(r,tx)).map_err(|e|e.to_string())?;rx.recv_timeout(Duration::from_secs(3)).map_err(|e|e.to_string())?}
 pub fn finish(mut self)->Result<Value>{self.stop.store(true,Ordering::Release);self.worker.take().ok_or("proxy_finished")?.join().map_err(|_|"proxy_panic".to_string())?}
}
impl Drop for Proxy{fn drop(&mut self){self.stop.store(true,Ordering::Release);if let Some(w)=self.worker.take(){let _=w.join();}}}
struct Worker{peers:Vec<Peer>,log:File,clock:Instant,total:Bucket,queue:VecDeque<Pending>,bytes:usize,rules:Vec<Rule>,serial:u64,counts:[u64;7],round:usize}
impl Worker{
 fn record(&mut self,mut v:Value)->Result<()>{v["proxy_us"]=json!(self.clock.elapsed().as_micros() as u64);writeln!(self.log,"{v}").map_err(|e|e.to_string())}
 fn random(&mut self,peer:usize,d:Direction)->u64{let s=&mut self.peers[peer].random[usize::from(d==Direction::Up)];*s^=*s<<13;*s^=*s>>7;*s^=*s<<17;*s}
 fn receive(&mut self,peer:usize,direction:Direction,bytes:Vec<u8>)->Result<()>{
  self.serial+=1;self.counts[0]+=1;let arrival=self.serial;let decoded=parts(&bytes);self.record(json!({"event":"receive","peer":peer,"direction":format!("{direction:?}"),"arrival":arrival,"wire_bytes":bytes.len()+28,"framing_valid":decoded.is_ok()}))?;
  if direction==Direction::Down&&self.peers[peer].profile==Profile::DownlinkStop{self.counts[1]+=1;return self.record(json!({"event":"outage_drop","peer":peer,"arrival":arrival}))}
  let mut bytes=bytes;let mut delay=0;let mut duplicate=false;let mut effect=None;
  if let Ok(ref ps)=decoded{for rule in &mut self.rules{if rule.remaining==0||rule.peer!=peer||rule.direction!=direction{continue}if let Some(p)=ps.iter().find(|p|rule.kind.is_none_or(|x|x==p.kind)&&rule.lane.is_none_or(|x|x==p.lane)&&rule.transfer.is_none_or(|x|x==p.transfer)&&match rule.fragment{Fragment::Any=>true,Fragment::First=>p.index==0,Fragment::Middle=>p.count>2&&p.index==p.count/2,Fragment::Last=>p.index+1==p.count}){rule.remaining-=1;effect=Some((rule.effect,p.clone()));break;}}}
  if let Some((action,p))=effect{self.counts[5]+=1;self.record(json!({"event":"rule_hit","peer":peer,"arrival":arrival,"effect":format!("{action:?}"),"transfer":p.transfer,"part":p.index,"parts":p.count,"kind":p.kind,"lane":p.lane,"collateral_messages":decoded.as_ref().map(|p|p.len()-1).unwrap_or(0)}))?;match action{Effect::Drop=>{self.counts[1]+=1;return Ok(())},Effect::Duplicate=>duplicate=true,Effect::Delay(ms)=>delay=ms,Effect::Mutate(m)=>{let original=bytes.clone();mutate(&mut bytes,&p,m)?;if matches!(m,Mutation::Conflict|Mutation::SameSequenceInput){self.enqueue(peer,direction,original,delay,arrival,0)?;}}}}
  if self.peers[peer].profile==Profile::N1{let r=self.random(peer,direction);if r%10000<50{self.counts[1]+=1;return self.record(json!({"event":"random_drop","peer":peer,"arrival":arrival}))}delay+=40+(self.random(peer,direction)%21)-10;if self.random(peer,direction)%10000<100{duplicate=true;self.counts[3]+=1;self.record(json!({"event":"random_duplicate","peer":peer,"arrival":arrival}))?;}if self.random(peer,direction)%10000<200{delay+=50;self.counts[4]+=1;self.record(json!({"event":"random_reorder_delay","peer":peer,"arrival":arrival}))?;}}
  if duplicate{self.enqueue(peer,direction,bytes.clone(),delay+1,arrival,1)?}self.enqueue(peer,direction,bytes,delay,arrival,0)
 }
 fn enqueue(&mut self,peer:usize,direction:Direction,bytes:Vec<u8>,delay:u64,arrival:u64,copy:u16)->Result<()>{if self.queue.len()>=8192||self.bytes+bytes.len()>8*1024*1024{self.counts[2]+=1;return self.record(json!({"event":"queue_overflow","peer":peer,"arrival":arrival}))}self.bytes+=bytes.len();self.queue.push_back(Pending{peer,direction,bytes,due:Instant::now()+Duration::from_millis(delay.min(2000)),arrival,copy});Ok(())}
 fn run(&mut self,rx:Receiver<Request>,stop:Arc<AtomicBool>)->Result<Value>{
  let mut buffer=[0u8;1201];while !stop.load(Ordering::Acquire){
   for request in rx.try_iter(){match request{Request::Profile(i,p,ms,reply)=>{self.peers[i].profile=p;self.peers[i].until=if ms==0{None}else{Some(Instant::now()+Duration::from_millis(ms))};let rate=if p==Profile::Slow{62500}else{312500};self.peers[i].down=Bucket::new(rate,8192);self.record(json!({"event":"profile","peer":i,"profile":format!("{p:?}"),"duration_ms":ms,"down_byte_s":rate}))?;let _=reply.send(Ok(json!({"applied_proxy_us":self.clock.elapsed().as_micros() as u64,"profile":format!("{p:?}")})));},Request::Rule(r,reply)=>{if self.rules.len()>=128{self.rules.retain(|r|r.remaining>0);}if self.rules.len()>=128{return Err("proxy_rule_capacity".into())}self.rules.push(r);let _=reply.send(Ok(json!({"armed_proxy_us":self.clock.elapsed().as_micros() as u64})));}}}
   for i in 0..self.peers.len(){if self.peers[i].until.is_some_and(|t|Instant::now()>=t){self.peers[i].profile=Profile::N0;self.peers[i].until=None;self.peers[i].down=Bucket::new(312500,8192);self.record(json!({"event":"profile_restored","peer":i}))?;}
    for d in [Direction::Down,Direction::Up]{for _ in 0..128{let packet=if d==Direction::Down{self.peers[i].back.recv(&mut buffer).map(|n|(n,None))}else{self.peers[i].front.recv_from(&mut buffer).map(|(n,a)|(n,Some(a)))};match packet{Ok((n,remote))=>{if n>1200{self.record(json!({"event":"oversize_drop","peer":i}))?;continue}if let Some(a)=remote{if a.ip()!=self.peers[i].ip||self.peers[i].client.is_some_and(|locked|locked!=a){self.record(json!({"event":"foreign_drop","peer":i}))?;continue}self.peers[i].client=Some(a);}self.receive(i,d,buffer[..n].to_vec())?;},Err(e) if e.kind()==std::io::ErrorKind::WouldBlock=>break,Err(e) if matches!(e.kind(),std::io::ErrorKind::ConnectionReset|std::io::ErrorKind::ConnectionRefused)=>{self.record(json!({"event":"udp_notice","peer":i,"error":e.to_string()}))?;break;},Err(e)=>return Err(format!("proxy_receive:{e}"))}}}
   }
   for _ in 0..128{let mut selected=None;for offset in 0..self.peers.len(){let peer=(self.round+offset)%self.peers.len();let now=Instant::now();if let Some(index)=self.queue.iter().position(|p|p.peer==peer&&p.due<=now&&(p.direction!=Direction::Down||self.peers[peer].profile!=Profile::DownlinkStop)){let p=&self.queue[index];let wire=p.bytes.len()+28;let ready=if p.direction==Direction::Down{self.total.has(wire,now)&&self.peers[peer].down.has(wire,now)&&self.peers[peer].client.is_some()}else{self.peers[peer].up.has(wire,now)};if ready{selected=Some(index);self.round=(peer+1)%self.peers.len();break}}}let Some(index)=selected else{break};let p=self.queue.remove(index).unwrap();self.bytes-=p.bytes.len();let wire=p.bytes.len()+28;let result=if p.direction==Direction::Down{self.peers[p.peer].front.send_to(&p.bytes,self.peers[p.peer].client.unwrap())}else{self.peers[p.peer].back.send(&p.bytes)};match result{Ok(n) if n==p.bytes.len()=>{if p.direction==Direction::Down{self.total.take(wire);self.peers[p.peer].down.take(wire)}else{self.peers[p.peer].up.take(wire)}self.counts[6]+=1;self.record(json!({"event":"send","peer":p.peer,"direction":format!("{:?}",p.direction),"wire_bytes":wire,"arrival":p.arrival,"copy":p.copy}))?;},Err(e) if e.kind()==std::io::ErrorKind::WouldBlock=>{self.bytes+=p.bytes.len();self.queue.push_front(p);break;},v=>return Err(format!("proxy_send:{v:?}"))}}
   thread::sleep(Duration::from_millis(1));
  }
  self.record(json!({"event":"stop","pending":self.queue.len(),"bytes":self.bytes,"unhit_rules":self.rules.iter().filter(|r|r.remaining>0).count()}))?;self.log.flush().map_err(|e|e.to_string())?;
  Ok(json!({"counts":self.counts,"pending_discarded":self.queue.len(),"unhit_rules":self.rules.iter().filter(|r|r.remaining>0).count(),"sockets_closed_on_return":true}))
 }
}
