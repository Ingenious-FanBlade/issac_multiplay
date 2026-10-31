//! tx-04-18统一实机验收：真实Authority后态经生产Room/Grid入口进入真实原生Client。
//! 普通输入只允许换成本连接代次；损坏模型及版本改写只用于明确的负向／恢复用例。
use crate::{Control,Result,Run,win32};
use serde_json::{Value,json};
use std::{collections::{BTreeMap,BTreeSet},fs,net::UdpSocket,path::Path,thread,time::{Duration,Instant}};
type Status=BTreeMap<String,String>;
fn n(s:&Status,k:&str)->u64{s.get(k).and_then(|s|s.parse().ok()).unwrap_or(0)}
fn integer(v:&Value)->Result<i64>{v.as_i64().or_else(||v.as_str().and_then(|s|s.parse().ok())).or_else(||v["value"].as_str().and_then(|s|s.parse().ok())).ok_or_else(||format!("native scalar {v}"))}
fn equal(a:&Value,b:&Value)->bool{
 if a.is_number()&&b.is_number(){return (a.as_f64().unwrap()-b.as_f64().unwrap()).abs()<0.00001;}
 if b["type"]=="int64"{return integer(a).ok()==integer(b).ok();}
 if let (Some(a),Some(b))=(a.as_array(),b.as_array()){return a.len()==b.len()&&a.iter().zip(b).all(|(a,b)|equal(a,b));}
 if let (Some(a),Some(b))=(a.as_object(),b.as_object()){return a.len()==b.len()&&a.iter().all(|(k,a)|b.get(k).is_some_and(|b|equal(a,b)));}a==b
}
// Client姿态由Authority快照驱动，IsPlaying／事件消费被主动关闭；核对其实际视觉状态而非要求本地动画运行。
fn sprite_equal(a:&Value,b:&Value)->bool{let a=&a["fields"];let b=&b["fields"];
 for key in ["Sprite.GetFilename","Sprite.IsLoaded"]{if !equal(&a[key],&b[key]){return false;}}
 if b["Sprite.IsLoaded"]!=true{return true;}
 ["Sprite.GetAnimation","Sprite.GetFrame","Sprite.GetOverlayAnimation","Sprite.GetOverlayFrame","Sprite.Color","Sprite.FlipX","Sprite.FlipY","Sprite.Offset","Sprite.Scale","Sprite.Rotation","Sprite.PlaybackSpeed","Sprite.GetRenderFlags","Sprite.GetAllLayers"].into_iter().all(|key|equal(&a[key],&b[key]))
}
fn revision(v:&Value)->Result<u64>{v["revision"].as_str().ok_or("revision")?.parse().map_err(|e|format!("revision: {e}"))}
// 负向输入有意采用新版本；所有组件／依赖版本一起改，避免被无关header错误提前拦截。
fn negative_revision(v:&mut Value,r:u64){v["revision"]=json!(r.to_string());for (_,c) in v["components"].as_object_mut().unwrap(){c["version"]=json!(r.to_string());}
 fn walk(v:&mut Value,r:u64){if let Some(a)=v.as_array_mut(){for e in a{walk(e,r)}}else if let Some(o)=v.as_object_mut(){if o.contains_key("required_revision"){o.insert("required_revision".into(),json!(r.to_string()));}for (_,e) in o{walk(e,r)}}}walk(&mut v["components"],r);
}
fn sources()->Vec<(String,String)>{let mut out=vec![("natural","prepare"),("types","types"),("states","states"),("states-return","states-return"),("destroy","destroy"),("replace-old","replace-old"),("replace-new","replace-new"),("empty","empty")].into_iter().map(|(a,b)|(a.into(),b.into())).collect::<Vec<_>>();
 for shape in 1..=12{out.push((format!("shape-{shape}"),format!("shape-{shape}")));}for name in ["boss","cellar","flooded","water","womb"]{out.push((name.into(),name.into()));}out
}
// 画面只核对自然内容与明确场景，不把逐像素相等当作Room/Grid域成功定义。
fn bitmap(path:&Path)->Result<(usize,usize,Vec<u8>)>{let b=fs::read(path).map_err(|e|e.to_string())?;
 let u=|p|->Result<u32>{Ok(u32::from_le_bytes(b.get(p..p+4).ok_or("BMP header")?.try_into().unwrap()))};
 if b.get(..2)!=Some(b"BM")||b.get(28..30)!=Some(&32u16.to_le_bytes()){return Err("native BMP format".into())}
 let w=u(18)? as usize;let raw_h=u(22)? as i32;let h=raw_h.unsigned_abs() as usize;if w<32||h<64||w*h>8*1024*1024{return Err("native BMP dimensions".into())}
 let offset=u(10)? as usize;let mut pixels=b.get(offset..offset+w*h*4).ok_or("BMP body")?.to_vec();if raw_h>0{for y in 0..h/2{for x in 0..w*4{pixels.swap(y*w*4+x,(h-1-y)*w*4+x)}}}Ok((w,h,pixels))
}
fn natural_pixels(authority:&Path,client:&Path)->Result<Value>{let (w,h,a)=bitmap(authority)?;let (cw,ch,b)=bitmap(client)?;if (w,h)!=(cw,ch){return Err("native Authority/Client render size".into())}
 let mut bins=BTreeSet::new();let mut colored=0u64;let mut error=0u64;let mut count=0u64;
 for y in 40..h-8{for x in 8..w-8{let at=(y*w+x)*4;let p=&b[at..at+3];if p.iter().any(|&v|v>16){colored+=1;bins.insert((p[0]/4,p[1]/4,p[2]/4));}
  for c in 0..3{error+=u64::from(a[at+c].abs_diff(b[at+c]));count+=1;}
 }}let mean=error as f64/count as f64;if colored<2000||bins.len()<12||mean>20.0{return Err(format!("native visual mismatch colored={colored} colors={} mean={mean}",bins.len()))}
 Ok(json!({"colored":colored,"colors":bins.len(),"mean_channel_difference":mean}))
}
impl Run {
 fn native_command(&self,c:&mut Control,action:&str)->Result<Status>{c.command(&format!("cs native_room_test {} {action}",self.report["token"].as_str().ok_or("token")?))}
 fn native_action(&self,c:&mut Control,action:&str)->Result<Value>{let response=self.native_command(c,&format!("action-{action}"))?;serde_json::from_str(response.get("native.fixture").ok_or("native fixture response")?).map_err(|e|e.to_string())}
 fn native_wait(&mut self,g:&win32::Game,c:&mut Control,label:&str,p:impl Fn(&Status)->bool)->Result<Status>{let end=Instant::now()+Duration::from_secs(45);loop{
  if let Some(code)=g.exited(){return Err(format!("native {label} process exit {code}"))}let s=c.command("cs status")?;if p(&s){self.step(label,json!(s))?;return Ok(s)}
  if Instant::now()>=end{return Err(format!("native {label}: {s:?}"))}thread::sleep(Duration::from_millis(80));
 }}
 fn native_submit(&self,c:&mut Control,name:&str,model:&Value)->Result<()>{let dir=self.out.join("runtime-world/native");fs::create_dir_all(&dir).map_err(|e|e.to_string())?;
  // 与正式DTO保持紧凑编码；大房间的缩进文本可膨胀数倍，不能因此错误触发生产字节上限。
  fs::write(dir.join(format!("{name}.json")),serde_json::to_vec(model).map_err(|e|e.to_string())?).map_err(|e|e.to_string())?;
  let operation=if model.get("baseline").is_some(){"reconstruct"}else{"submit"};self.native_command(c,&format!("{operation}-{name}"))?;Ok(())}
 fn native_object(&self,c:&mut Control,id:&str)->Result<Value>{let s=self.native_command(c,&format!("object-{id}"))?;
  let text=if let Some(file)=s.get("native.object_file"){if *file!=format!("readback-{id}.json"){return Err("native readback path".into())}crate::read(&self.out.join("runtime-world/native").join(file))?}else{s.get("native.object").ok_or("actual native object response")?.clone()};serde_json::from_str(&text).map_err(|e|e.to_string())}
 fn native_isolated(&self,c:&mut Control,before:&Status,callbacks:&Status)->Result<Status>{let after=c.command("cs counters")?;
  for entry in ["game_update","room_update","entity_list","odd_helper","player_update","start_game","restore","join_pass"]{let key=format!("hook.{entry}");if n(before,&key)!=n(&after,&key){return Err(format!("native Client gameplay entered {entry}"))}}
  let token=self.report["token"].as_str().ok_or("token")?;if *callbacks!=c.command(&format!("cs presentation_test {token} callbacks"))?{return Err("native Client dispatched gameplay Lua".into())}
  let status=c.command("cs status")?;if n(&status,"client_world_applied")!=0||n(&status,"presentation.progress_changed")!=0{return Err("native domain crossed world/persistence boundary".into())}Ok(after)
 }
 fn native_readback(&mut self,c:&mut Control,model:&Value,name:&str)->Result<()>{let source=&model["components"]["room"]["state"];let f=&source["state"]["fields"];let room=self.native_object(c,"room")?;
  for (key,member) in [("shape","Room.GetRoomShape"),("width","Room.GetGridWidth"),("height","Room.GetGridHeight"),("backdrop","Room.GetBackdropType"),("water","Room.GetWaterAmount")]{if !equal(&room[key],&f[member]){return Err(format!("native {name} Room {key}"))}}
  let cells=room["cells"].as_array().ok_or("native actual cells")?;if cells.len()!=448{return Err("native actual 448-cell membership".into())}
  let objects=model["components"]["grid"]["state"]["objects"].as_array().ok_or("native source objects")?;
  let positions=self.native_object(c,"geometry")?;
  if !equal(&self.native_object(c,"clocks")?,&source["native_context"]["grid_birth"]){return Err(format!("native {name} Grid creation clocks"))}
  for (i,cell) in cells.iter().enumerate(){for (member,key) in [("Room.GetGridCollision","collision"),("Room.GetGridPath","path"),("Room.GetRail","rail")]{let expected=&f[member]["values"][i]["value"];
   let expected=if expected["status"]=="Null"{255}else{integer(expected)?};if integer(&cell[key])?!=expected{return Err(format!("native {name} cell {i} {key}"))}}
   if cell["occupied"]!=json!(objects.iter().any(|o|o["index"].as_u64()==Some(i as u64))){return Err(format!("native {name} occupied {i}"))}
   for axis in ["x","y"]{if !equal(&positions[i][axis],&f["Room.GetGridPosition"]["values"][i]["value"][axis]){return Err(format!("native {name} actual position {i}"))}}
   if cell["inside"]!=source["native_context"]["grid_inside"][i]{return Err(format!("native {name} actual shape mask {i}"))}
  }
  for door in source["doors"].as_array().ok_or("source doors")?{let slot=door["slot"].as_u64().ok_or("door slot")? as usize;let expected=door["actual"]["target"]["id"].as_str().unwrap_or("0");if room["doors"][slot]!=expected{return Err(format!("native {name} door alias {slot}"))}}
  for object in objects{let id=object["subject"]["target"]["id"].as_str().ok_or("source id")?;let actual=self.native_object(c,id)?;let common=&object["common"]["fields"];let desc=&common["GridEntity.GetSaveState"]["fields"];
   for key in ["type","variant","index"]{if actual[key]!=object[key]{return Err(format!("native {name} Grid {id} {key}"))}}
   for (key,member) in [("state","GridEntity.State"),("collision","GridEntity.CollisionClass"),("water","GridEntity.GetWaterClipFlags")]{if !equal(&actual[key],&common[member]){return Err(format!("native {name} Grid {id} {key}"))}}
   for axis in ["x","y"]{if !equal(&actual["position"][axis],&common["GridEntity.Position"][axis]){return Err(format!("native {name} Grid {id} position {axis}"))}}
   for key in ["Type","Variant","State","SpawnCount","SpawnSeed","VariableSeed","VarData","Initialized"]{if !equal(&actual["save"][key],&desc[format!("GridEntityDesc.{key}")]){return Err(format!("native {name} Grid {id} save {key}"))}}
   for (key,member) in [("filename","Sprite.GetFilename"),("animation","Sprite.GetAnimation"),("frame","Sprite.GetFrame")]{if !equal(&actual["sprite"][key],&common["GridEntity.GetSprite"]["fields"][member]){return Err(format!("native {name} Grid {id} sprite {key}"))}}
   let class=object["class"].as_str().ok_or("source class")?;
   // Lua JSON把无字段的空表编码为[]；仅源类字段本来为空时接纳，不能漏验非空派生状态。
   if actual["derived"].as_array().is_some_and(|a|a.is_empty())&&object["bodies"][class]["fields"].as_object().is_some_and(|o|o.is_empty()){}
   else{for (key,value) in actual["derived"].as_object().ok_or("actual derived values")?{if !equal(value,&object["bodies"][class]["fields"][key]){return Err(format!("native {name} Grid {id} derived {key}"))}}}
  }
  let state=self.native_object(c,"room-state")?;for (key,value) in state.as_object().ok_or("actual Room state")?{let expected=&f[key];if value.is_object(){for (field,v) in value.as_object().unwrap(){if !equal(v,&expected[field]){return Err(format!("native {name} {key} {field}"))}}}else if !equal(value,expected){return Err(format!("native {name} {key}"))}}
  let motion=self.native_object(c,"motion")?;if !equal(&motion,&source["native_context"]["motion"]){return Err(format!("native {name} animation phase/track state"))}
  if !equal(&self.native_object(c,"water-lerp")?,&source["native_context"]["water_lerp"]){return Err(format!("native {name} water interpolation state"))}
  let mut env=self.native_object(c,"environment")?;let mut expected=source["native_context"]["environment"].clone();
  for i in 0..2{if !sprite_equal(&env["sprites"][i],&expected["sprites"][i]){return Err(format!("native {name} FX sprite {i}"))}}
  env.as_object_mut().ok_or("actual environment")?.remove("sprites");expected.as_object_mut().ok_or("expected environment")?.remove("sprites");
  if !equal(&env,&expected){return Err(format!("native {name} FX resources/state"))}
  self.step(&format!("native_readback_{name}"),json!({"cells":448,"door_slots":8,"grids":objects.len(),"source_fields":true,"actual_objects":true,"motion":true,"environment":true}))?;Ok(())
 }
 fn native_capture(&mut self,g:&win32::Game,c:&mut Control,name:&str)->Result<Value>{let end=Instant::now()+Duration::from_secs(45);loop{
  match self.native_command(c,&format!("source-{name}")){Ok(_)=>break,Err(e) if e.contains("native_source_incomplete_window")||e.contains("native_source_inactive")=>{if Instant::now()>=end{return Err(e)}self.watch_tx05(g,c,150)?;},Err(e)=>return Err(e)}
 }
 let model:Value=serde_json::from_str(&crate::read(&self.out.join(format!("runtime-world/native/{name}.json")))?).map_err(|e|e.to_string())?;
 self.native_command(c,&format!("reference-{name}"))?;let path=self.out.join(format!("runtime-world/presentation/source-{name}.bmp"));let end=Instant::now()+Duration::from_secs(15);
 while !path.exists(){if Instant::now()>=end{return Err(format!("native Authority reference {name}"))}self.watch_tx05(g,c,100)?;}Ok(model)
 }
 pub(super) fn execute_tx18(&mut self)->Result<()>{self.report["native_state_version"]=json!(8);
  if let Some(source)=self.scenario["resume_from"].as_str().map(std::path::PathBuf::from){let prior:Value=serde_json::from_str(&crate::read(&source.join("result.json"))?).map_err(|e|e.to_string())?;
   if prior["native_state_version"]!=8{return Err("native continuation requires two-frame domain references, actual wall choices and complete native context".into())}
   if source.join("source-scenes.json").exists(){let list:Value=serde_json::from_str(&crate::read(&source.join("source-scenes.json"))?).map_err(|e|e.to_string())?;
    if list.as_array().is_some_and(|a|a.len()==sources().len()){for folder in ["native","presentation"]{let dest=self.out.join("runtime-world").join(folder);fs::create_dir_all(&dest).map_err(|e|e.to_string())?;
      for (name,_) in sources(){let file=if folder=="native"{format!("{name}.json")}else{format!("source-{name}.bmp")};fs::copy(source.join("runtime-world").join(folder).join(&file),dest.join(&file)).map_err(|e|e.to_string())?;}}
     crate::write_json(&self.out.join("source-scenes.json"),&list)?;self.report["reused_authority_sources"]=json!(source);
    }
   }
  }
  self.environment()?;self.report["crash_before"]=self.crash_files()?;self.snapshot("save_before")?;self.prepare_restore_tx08(1)?;self.disable_helper_tx08()?;self.deploy()?;
  let game=self.launch("enabled")?;self.report["pid"]=json!(game.pid());let result=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||self.native_scenes(&game))).unwrap_or_else(|_|Err("native scenario panic".into()));
  if let Err(e)=&result{self.capture_failure(&game,"enabled",e);let _=game.close();}self.collect(game.pid(),"enabled")?;result
 }
 fn native_scenes(&mut self,g:&win32::Game)->Result<()>{let w=g.wait_window(90)?;let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("port")? as u16,g,90,&self.out.join("control.jsonl"))?;c.wait_startup(g,w,90)?;
  let index=self.out.join("source-scenes.json");if !index.exists(){win32::set_auto_focus(true);c.focus_window=Some(w);self.prepare_new_run_tx05(g,w,&mut c)?;self.sequence(w,"start_run")?;c.wait("run_active","1",25)?;
   self.native_action(&mut c,"seed")?;self.watch_tx05(g,&mut c,1800)?;c.command("cs observe start")?;c.command(&format!("cs host {}",self.scenario["host_port"]))?;c.wait("session_state","Active",25)?;
   let mut entries=Vec::new();for (name,action) in sources(){let requested=self.native_action(&mut c,&action)?;self.watch_tx05(g,&mut c,900)?;
    if name.starts_with("shape-")||["boss","cellar","flooded","water","womb"].contains(&name.as_str()){self.native_action(&mut c,"prepare")?;self.watch_tx05(g,&mut c,450)?;}
    if name=="water"{self.native_action(&mut c,"water")?;self.watch_tx05(g,&mut c,450)?;}
    let model=self.native_capture(g,&mut c,&name)?;let fact=self.native_action(&mut c,"facts")?;
    // 用真实Authority独立列举核对场景成员，防止周期采集延迟使上一动作的快照冒充本场景。
    let mut captured=Vec::new();for o in model["components"]["grid"]["state"]["objects"].as_array().ok_or("captured members")?{captured.push((integer(&o["index"])?,integer(&o["type"])?,integer(&o["variant"])?));}
    let mut actual=Vec::new();for o in fact["grids"].as_array().ok_or("Authority actual members")?{actual.push((integer(&o["index"])?,integer(&o["type"])?,integer(&o["variant"])?));}
    captured.sort();actual.sort();if captured!=actual{return Err(format!("native Authority scene snapshot was stale: {name}"))}
    if let Some(shape)=name.strip_prefix("shape-"){if fact["shape"].as_u64()!=shape.parse::<u64>().ok(){return Err(format!("Authority actual requested {name}: {fact}"))}}
    entries.push(json!({"name":name,"pid":g.pid(),"scope":model["scope"],"revision":model["revision"],"fact":fact,"requested":requested}));crate::write_json(&index,&json!(entries))?;
   }
   c.command("cs stop")?;c.wait("session_state","Idle",25)?;c.command("cs observe stop")?;let end=Instant::now()+Duration::from_secs(20);
   loop{let s=c.command("cs observe status")?;if s.get("observation_state").is_some_and(|s|s=="Off"){break;}if Instant::now()>=end{return Err("Authority observation retirement".into())}self.watch_tx05(g,&mut c,100)?;}
   self.return_to_menu(w,&mut c)?;
  }
  // 顺序续测直接进入Client时也等待启动通知消退，避免Steam叠加提示污染画面对比。
  self.watch_tx05(g,&mut c,12000)?;
  c.focus_window=None;win32::set_auto_focus(false);c.wait("client_ui_available","1",20)?;self.snapshot("save-client-before")?;
  let blackhole=UdpSocket::bind("127.0.0.1:0").map_err(|e|e.to_string())?;let port=blackhole.local_addr().map_err(|e|e.to_string())?.port();
  let entries:Value=serde_json::from_str(&crate::read(&index)?).map_err(|e|e.to_string())?;let mut previous=Value::Null;let mut baseline=Value::Null;let mut before=Status::new();let mut callbacks=Status::new();let mut connection=0;
  let mut types=BTreeSet::new();let mut classes=BTreeSet::new();let mut shapes=BTreeSet::new();let mut backdrops=BTreeSet::new();let mut last=Value::Null;
  for entry in entries.as_array().ok_or("native source list")?{let name=entry["name"].as_str().ok_or("scene name")?;let mut model:Value=serde_json::from_str(&crate::read(&self.out.join(format!("runtime-world/native/{name}.json")))?).map_err(|e|e.to_string())?;
   if connection==0||previous!=entry["scope"]{if connection!=0{self.native_isolated(&mut c,&before,&callbacks)?;c.command("cs stop")?;c.wait("role","Off",25)?;}
    c.command(&format!("cs client 127.0.0.1 {port} 0"))?;let s=c.wait("role","Client",10)?;connection=n(&s,"client.connection");before=c.command("cs counters")?;callbacks=c.command(&format!("cs presentation_test {} callbacks",self.report["token"].as_str().unwrap()))?;previous=entry["scope"].clone();baseline=Value::Null;
   }
   model["scope"]["connection"]=json!(connection.to_string());let r=revision(&model)?;let start=c.command("cs status")?;let rejected=n(&start,"native.rejected");let draws=n(&start,"native.draws");
   let input=if baseline.is_null(){baseline=model.clone();model.clone()}else{json!({"baseline":baseline,"target":model})};self.native_submit(&mut c,&format!("client-{name}"),&input)?;
   let end=Instant::now()+Duration::from_secs(45);loop{let s=c.command("cs status")?;if n(&s,"native.rejected")>rejected{return Err(format!("native {name} rejected: {:?}",s.get("native.error")))}
    if n(&s,"native.revision")==r&&n(&s,"native.healthy")==1&&n(&s,"native.draws")>draws+3{break;}if Instant::now()>=end{return Err(format!("native {name} apply timeout {s:?}"))}self.watch_tx05(g,&mut c,80)?;}
   self.native_readback(&mut c,&model,name)?;let count=n(&c.command("cs status")?,"presentation.captures");c.command(&format!("cs presentation_test {} capture-{name}",self.report["token"].as_str().unwrap()))?;
   self.native_wait(g,&mut c,&format!("native_capture_{name}"),|s|n(s,"presentation.captures")>count)?;
   let pixels=natural_pixels(&self.out.join(format!("runtime-world/presentation/source-{name}.bmp")),&self.out.join(format!("runtime-world/presentation/capture-{name}.bmp")))?;
   for object in model["components"]["grid"]["state"]["objects"].as_array().unwrap(){types.insert(integer(&object["type"])?);classes.insert(object["class"].as_str().ok_or("class")?.to_string());}
   shapes.insert(integer(&model["components"]["room"]["state"]["state"]["fields"]["Room.GetRoomShape"])?);backdrops.insert(integer(&model["components"]["room"]["state"]["state"]["fields"]["Room.GetBackdropType"])?);
   self.native_isolated(&mut c,&before,&callbacks)?;self.case(&format!("native_{name}"),true,json!({"pixels":pixels,"actual_objects":true,"scope":model["scope"],"revision":r}));last=model;
  }
  if types!=(1..=27).collect::<BTreeSet<_>>()||classes.len()!=17||shapes!=(1..=12).collect::<BTreeSet<_>>()||backdrops.len()<3{return Err(format!("native coverage types={types:?} classes={classes:?} shapes={shapes:?} backdrops={backdrops:?}"))}
  self.case("native_complete_type_shape_environment_coverage",true,json!({"types":types,"classes":classes,"shapes":shapes,"backdrops":backdrops}));
  self.native_negative(g,&mut c,&last)?;self.native_isolated(&mut c,&before,&callbacks)?;c.command("cs stop")?;c.wait("role","Off",25)?;
  // 新连接以完整正常目标重建，随后在有载体和GPU资源的情况下直接走正常退出。
  c.command(&format!("cs client 127.0.0.1 {port} 0"))?;let s=c.wait("role","Client",10)?;last["scope"]["connection"]=json!(n(&s,"client.connection").to_string());self.native_submit(&mut c,"normal-close",&last)?;
  self.native_wait(g,&mut c,"native_normal_close_ready",|s|n(s,"native.healthy")==1)?;self.report["final_status"]=json!(c.command("cs status")?);drop(c);if g.close()?!=0{return Err("native Client normal exit".into())}self.snapshot("save-process-exit")?;
  let changes=crate::saves::changes(&self.out.join("save-client-before"),&self.out.join("save-process-exit"))?;
  if changes.as_array().ok_or("visitor save changes")?.iter().any(|v|{let p=v["path"].as_str().unwrap_or("").to_lowercase();v["bytes_changed"]==true&&(p.contains("persistentgamedata")||p.contains("gamestate")||p.contains("savesyncstatus"))}){return Err("native Client visitor persistence changed".into())}
  self.case("native_stop_reconnect_exit_and_visitor_persistence",true,json!({"changes":changes,"normal_exit":true,"whole_world_applied":false}));Ok(())
 }
 fn native_negative(&mut self,g:&win32::Game,c:&mut Control,real:&Value)->Result<()>{let r=revision(real)?;let mut bads=Vec::new();
  let mut bad=real.clone();bad["components"]["grid"]["coverage"]=json!("Partial");bad["components"]["grid"]["membership_complete"]=json!(false);bads.push(("partial",bad));
  let mut bad=real.clone();bad["components"]["room"]["state"].as_object_mut().unwrap().remove("config");bads.push(("missing-config",bad));
  let mut bad=real.clone();bad["components"]["grid"]["state"]["objects"][0]["class"]=json!("UnknownGrid");bads.push(("unknown-class",bad));
  let mut bad=real.clone();bad["components"]["grid"]["state"]["objects"][0]["type"]=json!(28);bads.push(("unknown-type",bad));
  let mut bad=real.clone();bad["components"]["grid"]["state"]["objects"][0]["common"]["fields"].as_object_mut().unwrap().remove("GridEntity.State");bads.push(("missing-state",bad));
  let mut bad=real.clone();bad["components"]["room"]["state"]["grids"][0]["subject"]["target"]["id"]=json!("999999999");bads.push(("missing-reference",bad));
  let mut bad=real.clone();let object=bad["components"]["grid"]["state"]["objects"].as_array_mut().unwrap().iter_mut().find(|o|o["common"]["fields"]["GridEntity.GetSprite"]["fields"]["Sprite.IsLoaded"]==true).ok_or("negative source needs a loaded native sprite")?;
  let sprite=&mut object["common"]["fields"]["GridEntity.GetSprite"];sprite["fields"]["Sprite.GetFilename"]=json!("gfx/native-tx18-missing.anm2");sprite["resource"]["logical_path"]=json!("gfx/native-tx18-missing.anm2");bads.push(("missing-resource",bad));
  let mut bad=real.clone();bad["components"]["room"]["state"]["native_context"]["descriptors"]=json!([]);bads.push(("missing-descriptor",bad));
  let mut bad=real.clone();bad["scope"]["scene"]=json!("999999999");bads.push(("foreign-scope",bad));
  let mut bad=real.clone();bad["components"]["room"]["state"]["state"]["fields"]["Room.GetBackdropType"]=json!(1);bads.push(("same-version-conflict",bad));
  for (name,mut bad) in bads{if name!="same-version-conflict"{negative_revision(&mut bad,r+1);}let start=c.command("cs status")?;let rejected=n(&start,"native.rejected");self.native_submit(c,&format!("negative-{name}"),&bad)?;
   let s=self.native_wait(g,c,name,|s|n(s,"native.rejected")>rejected)?;if n(&s,"native.revision")!=r||n(&s,"native.healthy")!=1{return Err(format!("native invalid candidate changed committed world: {name}"))}
  }
  self.case("native_invalid_partial_resource_reference_atomic_rejection",true,json!({"cases":10,"previous_revision":r}));
  let start=c.command("cs status")?;self.native_submit(c,"duplicate",real)?;let d=n(&start,"native.duplicates");self.native_wait(g,c,"native_duplicate",|s|n(s,"native.duplicates")>d)?;
  let mut old=real.clone();negative_revision(&mut old,r-1);self.native_submit(c,"stale",&old)?;let d=n(&start,"native.stale");self.native_wait(g,c,"native_stale",|s|n(s,"native.stale")>d)?;
  // 所有故障输入均是标记过的真实目标副本；每次完整恢复后重新读实际原生对象。
  let mut current=real.clone();for (i,phase) in ["create","state","refresh"].into_iter().enumerate(){self.native_command(c,"fail-apply")?;self.native_command(c,&format!("fail-next-{phase}"))?;
   negative_revision(&mut current,r+1+i as u64);let rejected=n(&c.command("cs status")?,"native.rejected");self.native_submit(c,&format!("failure-{phase}"),&current)?;
   let s=self.native_wait(g,c,&format!("native_partial_{phase}"),|s|n(s,"native.rejected")>rejected)?;if n(&s,"native.healthy")!=0||!self.native_object(c,"room")?.is_null(){return Err("native half-applied world remained visible".into())}
   self.native_submit(c,&format!("recovery-{phase}"),&current)?;let expected=revision(&current)?;self.native_wait(g,c,&format!("native_recovered_{phase}"),|s|n(s,"native.revision")==expected&&n(s,"native.healthy")==1)?;self.native_readback(c,&current,&format!("recovery-{phase}"))?;
  }
  self.case("native_duplicate_stale_partial_failure_full_recovery",true,json!({"failure_phases":3,"recovered_actual_state":true,"domain_only":true}));Ok(())
 }
}
