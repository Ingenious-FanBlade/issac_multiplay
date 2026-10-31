//! tx-04-18统一实机应用验收。受控Authority纯值经过正式应用入口，不构造Client本地run。
use crate::{Control,Result,Run,win32};
use serde_json::{Value,json};
use std::{collections::BTreeMap,fs,net::UdpSocket,thread,time::{Duration,Instant}};
type Status=BTreeMap<String,String>;
fn n(s:&Status,k:&str)->u64{s.get(k).and_then(|s|s.parse().ok()).unwrap_or(0)}
// 几何值允许JSON整数/浮点的等值表示，身份和64位整数仍按原值严格比较。
fn geometry_equal(a:&Value,b:&Value)->bool{
 if a.is_number()&&b.is_number(){return a.as_f64()==b.as_f64();}
 if let (Some(a),Some(b))=(a.as_array(),b.as_array()){return a.len()==b.len()&&a.iter().zip(b).all(|(a,b)|geometry_equal(a,b));}
 if let (Some(a),Some(b))=(a.as_object(),b.as_object()){return a.len()==b.len()&&a.iter().all(|(k,a)|b.get(k).is_some_and(|b|geometry_equal(a,b)));}
 a==b
}
const CLASSES:[&str;28]=["","Decoration","Rock","Rock","Rock","Rock","Rock","Pit","Spikes","Spikes","Web","Lock","TNT","Fire","Poop","Wall","Door","TrapDoor","Stairs","Gravity","PressurePlate","Statue","Rock","Teleporter","Rock","Rock","Rock","Rock"];
fn key(id:u64)->Value{json!({"run":"1","space":"2","id":id.to_string()})}
fn pose()->Value{json!({"animation":"Main","frame":0,"overlay":null,"overlay_first":false,
 "offset":[0,0],"scale":[1,1],"rotation":0,"color":[1,1,1,1,0,0,0,0,0,0,0],"champion_color":[1,1,1,1,0,0,0,0,0,0,0],
 "flip_x":false,"flip_y":false,"flags":0,"layers":[],"custom_shader":null,"champion_shader":null})}
fn node(id:u64,pass:u64,x:i32,y:i32,orange:bool)->Value{json!({"id":id.to_string(),"pass":pass,"screen":false,
 "position":[x,y],"order":y,"clamp_top":[0,0],"clamp_bottom":[0,0],"anm2":"gfx/isaac-cs-tx17/scene.anm2",
 "sheets":if orange{json!([{"id":0,"path":"gfx/isaac-cs-tx17/b.png"},{"id":1,"path":"gfx/isaac-cs-tx17/b.png"}])}else{json!([])},"pose":pose()})}
// 每个实际inside格使用一个由Authority给定的地面部件，缺角直接没有节点；不用全屏背景伪装房形。
fn rebuild_floor(world:&mut Value){
 world["nodes"].as_array_mut().unwrap().retain(|n|n["id"].as_str().unwrap().parse::<u64>().unwrap()<1000);
 world["room"]["parts"].as_array_mut().unwrap().retain(|p|p["role"]!="floor");
 let cells=world["room"]["cells"].as_array().unwrap().clone();
 for cell in cells.iter().filter(|c|c["inside"]==true){let i=cell["index"].as_u64().unwrap();let id=1000+i;
  let mut tile=node(id,0,cell["position"][0].as_i64().unwrap() as i32,cell["position"][1].as_i64().unwrap() as i32,false);
  tile["anm2"]=json!("gfx/isaac-cs-tx18/tile.anm2");tile["pose"]["scale"]=json!([1.25,1.25]);
  world["nodes"].as_array_mut().unwrap().push(tile);
  world["room"]["parts"].as_array_mut().unwrap().push(json!({"role":"floor","node":id.to_string(),"cells":[i]}));
 }
}
// 目录枚举只构造明确受控的Authority状态，不调用任何游戏玩法setter；所有值都要从Client读回。
fn fields(catalog:&Value,owner:&str)->Value{
 let mut fields=serde_json::Map::new();
 for m in catalog["members"].as_array().unwrap().iter().filter(|m|m["owner"]==owner){
  let category=m["category"].as_str().unwrap_or("");if !["field","query","parameter_query"].contains(&category){continue;}
  let ret=m["returns"].as_str().unwrap_or("");let member=m["member"].as_str().unwrap_or("");
  let v=if category=="parameter_query"{json!({"kind":"parameter_results","values":[{"parameters":[0],"value":false}]})}
   else if ret=="boolean"{json!(false)}else if ret=="string"{json!("Idle")}
   else if ret=="Vector"{json!({"type":"vector","x":0,"y":0})}
   else if ret=="RNG"||member.contains("RNG"){json!({"status":"AuthorityOnly","reason":"No gameplay RNG on Client"})}
   else if ret=="Sprite"{json!({"type":"resource_pose","resource":{"logical_path":"gfx/isaac-cs-tx17/scene.anm2","content_version":"tx18-controlled-v1"},"pose":pose()})}
   else if ret=="GridEntityDesc"{json!({"type":"GridEntityDesc","fields":{"GridEntityDesc.Initialized":true,"GridEntityDesc.SpawnCount":1,"GridEntityDesc.SpawnSeed":123,"GridEntityDesc.State":0,"GridEntityDesc.Type":1,"GridEntityDesc.VarData":0,"GridEntityDesc.VariableSeed":456,"GridEntityDesc.Variant":0}})}
   else{json!(0)};
  fields.insert(m["id"].as_str().unwrap().to_string(),v);
 }
 json!({"type":owner,"fields":fields})
}
fn world(catalog:&Value,connection:u64,revision:u64)->Value{
 let mut nodes=vec![node(2,2,560,40,false)];nodes[0]["pose"]["scale"]=json!([34,1]);
 let mut grids=Vec::new();
 for t in 1..28{
  let class=format!("GridEntity{}",CLASSES[t]);let mut common=fields(catalog,"GridEntity");let mut derived=fields(catalog,&class);
  common["fields"]["GridEntity.GetType"]=json!(t);common["fields"]["GridEntity.GetVariant"]=json!(0);
  common["fields"]["GridEntity.GetGridIndex"]=json!(t+30);
  common["fields"]["GridEntity.GetSaveState"]["fields"]["GridEntityDesc.Type"]=json!(t);
  common["fields"]["GridEntity.Desc"]=common["fields"]["GridEntity.GetSaveState"].clone();
  let id=t as u64+100;let (x,y)=(40+((t+30)%28) as i32*40,80+((t+30)/28) as i32*40);
  common["fields"]["GridEntity.Position"]=json!({"type":"vector","x":x,"y":y});common["fields"]["GridEntity.GetRenderPosition"]=common["fields"]["GridEntity.Position"].clone();
  nodes.push(node(id,4,x,y,true));
  let mut parts=vec![json!({"role":"main","node":id.to_string()})];
  if t==16{derived["fields"]["GridEntityDoor.ExtraVisible"]=json!(true);derived["fields"]["GridEntityDoor.Slot"]=json!(0);
   nodes.push(node(500,4,420,190,true));parts.push(json!({"role":"extra","node":"500"}));}
  if t==20{nodes.push(node(501,4,120,250,true));parts.push(json!({"role":"timer","node":"501"}));
   nodes.push(json!({"id":"503","pass":4,"position":[120,280],"screen":false,"order":280,"text":"00:37","text_scale":[1,1],"text_color":[1,0.3,0.2,1]}));
   parts.push(json!({"role":"timer_text","node":"503"}));}
  if t==2{nodes.push(node(502,5,120,90,true));parts.push(json!({"role":"top","node":"502"}));}
  let memberships=if t==16{json!([{"source":"grid","index":t+30},{"source":"door","index":0}])}else{json!([{"source":"grid","index":t+30}])};
  grids.push(json!({"key":key(t as u64),"type":t,"variant":0,"class":class,"index":t+30,"memberships":memberships,
    "visible":true,"save":common["fields"]["GridEntity.GetSaveState"],"state":{"common":common,"derived":derived},"parts":parts}));
 }
 let cells:Vec<_>=(0..448).map(|i|json!({"index":i,"inside":true,"position":[40+(i%28)*40,80+(i/28)*40],"collision":i%6,"path":i*3,"rail":i%7,"occlusion":i%4})).collect();
 let doors:Vec<_>=(0..8).map(|i|json!({"slot":i,"allowed":true,"descriptor_enabled":i==0,"target":{"kind":"MapRoom","index":65},"grid":if i==0{key(16)}else{Value::Null}})).collect();
 let mut result=json!({"format":"isaac.cs.room-grid","version":1,"connection":connection.to_string(),"revision":revision.to_string(),
 "scope":{"session":"1","connection":"1","run":"1","floor":"1","dimension":"0","room":"1","scene":"1"},
 "membership_complete":true,"room":{"shape":8,"size":[1200,800],"cells":cells,"state":{"type":"Room","fields":{"Room.GetRoomShape":8}},
 "config":{"type":"RoomConfigRoom","controlled":true},"override_config":null,
 "environment":{"backdrop":1,"floor_color":[1,1,1,1],"wall_color":[1,1,1,1],"water":{"amount":0,"color":[0.1,0.2,0.3,1],"current":[0,0]},"fx":{"rain":false,"lava":false}},
 "doors":doors,"parts":[{"role":"wall","node":"2"}]},
 "camera":{"origin":[0,0],"zoom":0.45},"hud":"Authority Room/Grid dedicated display - no local simulation","grids":grids,"nodes":nodes});
 rebuild_floor(&mut result);result
}
// 真实经过15规范模型的Room/Grid component接入；其他九域不宣称已经应用。
fn source_model(display:&Value)->Value{
 let mut room_state=display["room"]["state"].clone();
 for (member,field) in [("Room.GetGridPosition","position"),("Room.GetGridCollision","collision"),("Room.GetGridPath","path"),("Room.GetRail","rail")]{
  let values:Vec<_>=display["room"]["cells"].as_array().unwrap().iter().map(|c|json!({"parameters":[c["index"]],
   "value":if field=="position"{json!({"type":"vector","x":c[field][0],"y":c[field][1]})}else{c[field].clone()}})).collect();
  room_state["fields"][member]=json!({"kind":"parameter_results","values":values});
 }
 let objects:Vec<_>=display["grids"].as_array().unwrap().iter().map(|g|{
  let memberships:Vec<_>=g["memberships"].as_array().unwrap().iter().map(|m|json!({"source":if m["source"]=="grid"{11}else{12},"index":m["index"]})).collect();
  let mut bodies=serde_json::Map::new();bodies.insert(g["class"].as_str().unwrap().into(),g["state"]["derived"].clone());
  json!({"subject":{"type":"reference","status":"KnownCurrent","target":g["key"]},"type":g["type"],"variant":g["variant"],"index":g["index"],"class":g["class"],"memberships":memberships,
    "common":g["state"]["common"],"bodies":bodies})
 }).collect();
 let doors:Vec<_>=display["room"]["doors"].as_array().unwrap().iter().map(|d|json!({"slot":d["slot"],"shape_allowed":d["allowed"],"descriptor_enabled":d["descriptor_enabled"],"actual_connection":d["target"],
  "actual":if d["grid"].is_null(){json!({"type":"reference","status":"Null"})}else{json!({"type":"reference","status":"KnownCurrent","target":d["grid"]})}})).collect();
 let mut components=serde_json::Map::new();
 for (name,domain,scope,state) in [("grid",2,1,json!({"objects":objects})),("room",3,2,json!({"state":room_state,"config":display["room"]["config"],"override_config":display["room"]["override_config"],"doors":doors}))]{
  components.insert(name.into(),json!({"domain":domain.to_string(),"version":display["revision"],"coverage":"Complete","reasons":[],"ownership":"ReplicatedState","membership_complete":true,
   "capture":{"scope":scope.to_string(),"authority_tick":"100","phase":"OrdinaryPost"},"state":state}));
 }
 json!({"format":"isaac.cs.replica.model","scope":display["scope"],"revision":display["revision"],"event_watermark":"0","native_applied":false,"publishable":false,
 "missing_domains":["entity","level","exploration","room_save","player","run","random","presentation","mod"],"components":components})
}
fn pixels(path:&std::path::Path)->Result<(u64,u64)>{
 let b=fs::read(path).map_err(|e|e.to_string())?;if b.get(..2)!=Some(b"BM"){return Err("BMP".into())}
 let offset=u32::from_le_bytes(b.get(10..14).ok_or("BMP offset")?.try_into().unwrap()) as usize;
 let mut teal=0;let mut orange=0;for p in b.get(offset..).ok_or("BMP body")?.chunks_exact(4){let (r,g,blue)=(u32::from(p[2]),u32::from(p[1]),u32::from(p[0]));
  if blue>r*2&&g>r*2&&blue>60{teal+=1;}if r>blue*2&&r>g&&r>80{orange+=1;}}
 Ok((teal,orange))
}
impl Run {
 fn wait_tx18(&mut self,g:&win32::Game,c:&mut Control,label:&str,p:impl Fn(&Status)->bool)->Result<Status>{
  let end=Instant::now()+Duration::from_secs(30);loop{
   if let Some(code)=g.exited(){return Err(format!("{label}: exited {code}"))}let s=c.command("cs status")?;
   if p(&s){self.step(label,json!(s))?;return Ok(s)}if Instant::now()>=end{return Err(format!("{label}: {s:?}"))}thread::sleep(Duration::from_millis(65));
  }
 }
 fn submit_tx18(&mut self,c:&mut Control,name:&str,value:&Value)->Result<()>{
  let root=self.out.join("runtime-world/presentation");fs::create_dir_all(&root).map_err(|e|e.to_string())?;
  crate::write_json(&root.join(format!("{name}.json")),value)?;
  c.command(&format!("cs presentation_test {} {name}",self.report["token"].as_str().ok_or("token")?))?;Ok(())
 }
 fn object_tx18(&mut self,c:&mut Control,id:&str)->Result<Value>{
  let r=c.command(&format!("cs presentation_test {} room-grid-object-{id}",self.report["token"].as_str().ok_or("token")?))?;
  serde_json::from_str(r.get("room_grid.object").ok_or("object response")?).map_err(|e|e.to_string())
 }
 fn capture_tx18(&mut self,g:&win32::Game,c:&mut Control,name:&str)->Result<()>{
  let count=n(&c.command("cs status")?,"presentation.captures");
  c.command(&format!("cs presentation_test {} capture-{name}",self.report["token"].as_str().unwrap()))?;
  self.wait_tx18(g,c,&format!("capture_{name}"),|s|n(s,"presentation.captures")>count)?;
  fs::copy(self.out.join(format!("runtime-world/presentation/capture-{name}.bmp")),self.out.join(format!("{name}.bmp"))).map_err(|e|e.to_string())?;Ok(())
 }
 pub(super) fn execute_tx18(&mut self)->Result<()>{
  if let Some(source)=self.scenario["resume_from"].as_str().map(std::path::PathBuf::from){
   let prior:Value=serde_json::from_str(&crate::read(&source.join("result.json"))?).map_err(|e|e.to_string())?;
   if prior["tx"]!="tx-04-18"{return Err("tx18 resume source".into())}
   let retest=self.scenario["retest_cases"].as_array().cloned().unwrap_or_default();
   for (name,value) in prior["cases"].as_object().ok_or("prior cases")?{
    if value["pass"]==true&&!retest.iter().any(|v|v==name){self.report["cases"][name]=value.clone();}
   }
   let mut objects=serde_json::Map::new();let mut cells=serde_json::Map::new();let mut cursor=Some(source.clone());let mut visited=std::collections::BTreeSet::new();
   while let Some(path)=cursor.take(){if !visited.insert(path.clone())||visited.len()>8{return Err("cyclic tx18 continuation".into())}
   for line in crate::read(&path.join("control.jsonl"))?.lines(){let row:Value=serde_json::from_str(line).map_err(|e|e.to_string())?;
    if row["event"]=="send"&&row["text"].as_str().is_some_and(|s|s.ends_with(" states")){break;}
    if row["event"]!="response"||row["response"]["ok"]!="1"{continue;}
    if let Some(id)=row["text"].as_str().and_then(|s|s.split("room-grid-object-").nth(1)){
     if let Some(text)=row["response"]["room_grid.object"].as_str(){let value:Value=serde_json::from_str(text).map_err(|e|e.to_string())?;
      if id.parse::<u64>().is_ok(){objects.entry(id.to_string()).or_insert(value);}
      else if let Some(index)=id.strip_prefix("cell-"){if index.parse::<usize>().is_ok(){cells.entry(index.to_string()).or_insert(value);}}
     }
    }
   }
   let ancestor:Value=serde_json::from_str(&crate::read(&path.join("result.json"))?).map_err(|e|e.to_string())?;
   cursor=ancestor["reused_unaffected"]["source"].as_str().map(std::path::PathBuf::from);
   }
   if !retest.iter().any(|v|v=="all_types_derived_save_members_and_native_pixels"){self.scenario["reused_objects"]=json!(objects);}
   self.scenario["reused_cells"]=json!(cells);
   self.report["reused_unaffected"]=json!({"source":source,"cases":self.report["cases"],"object_readbacks":objects.len(),"cell_readbacks":cells.len(),"retest":retest});
  }
  win32::set_auto_focus(false);self.environment()?;self.report["crash_before"]=self.crash_files()?;self.snapshot("save_before")?;
  self.prepare_restore_tx08(1)?;self.disable_helper_tx08()?;self.deploy()?;
  let game=self.launch("enabled")?;self.report["pid"]=json!(game.pid());
  let result=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||self.scenes_tx18(&game))).unwrap_or_else(|_|Err("tx18 scenario panic".into()));
  if let Err(e)=&result{self.capture_failure(&game,"enabled",e);let _=game.close();}self.collect(game.pid(),"enabled")?;result
 }
 fn scenes_tx18(&mut self,g:&win32::Game)->Result<()>{
  let w=g.wait_window(90)?;let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("port")? as u16,g,90,&self.out.join("control.jsonl"))?;
  c.wait_startup(g,w,90)?;c.focus_window=None;c.wait("client_ui_available","1",20)?;self.snapshot("save-client-before")?;
  let blackhole=UdpSocket::bind("127.0.0.1:0").map_err(|e|e.to_string())?;let port=blackhole.local_addr().map_err(|e|e.to_string())?.port();
  c.command(&format!("cs client 127.0.0.1 {port} 0"))?;let initial=c.wait("role","Client",10)?;
  let before=c.command("cs counters")?;let token=self.report["token"].as_str().unwrap().to_string();
  let callbacks=c.command(&format!("cs presentation_test {token} callbacks"))?;
  let catalog:Value=serde_json::from_str(&crate::read(&self.repo.join("mod/observation/isaac-cs-observation/grid-catalog.json"))?).map_err(|e|e.to_string())?;
  let mut current=world(&catalog,n(&initial,"client.connection"),1);
  let request=json!({"format":"isaac.cs.room-grid-source","model":source_model(&current),"display":current});
  self.submit_tx18(&mut c,"all-types",&request)?;
  let ready=self.wait_tx18(g,&mut c,"room_grid_initial",|s|n(s,"room_grid.revision")==1&&n(s,"presentation.draws")>=4)?;
  if n(&ready,"room_grid.types")!=27||n(&ready,"room_grid.classes")!=17||n(&ready,"room_grid.cells")!=448||n(&ready,"room_grid.doors")!=1{return Err(format!("type/geometry boundary {ready:?}"))}
  let reused_initial=self.report["cases"]["all_types_derived_save_members_and_native_pixels"]["pass"]==true;
  if !reused_initial{for expected in current["grids"].as_array().unwrap(){let id=expected["key"]["id"].as_str().unwrap();
   let actual=if let Some(value)=self.scenario["reused_objects"].get(id){value.clone()}else{self.object_tx18(&mut c,id)?};
   if actual!=*expected{return Err(format!("full derived/save state mismatch {}",expected["key"]["id"]))}}
  for i in 0..448{let cell=if let Some(v)=self.scenario["reused_cells"].get(i.to_string()){v.clone()}else{self.object_tx18(&mut c,&format!("cell-{i}"))?};if !geometry_equal(&cell,&current["room"]["cells"][i]){return Err(format!("cell state {i}"))}}
  }
  let pixel=if reused_initial{let p=&self.report["cases"]["all_types_derived_save_members_and_native_pixels"]["detail"]["pixels"];
   (p[0].as_u64().ok_or("reused pixels")?,p[1].as_u64().ok_or("reused pixels")?)}else{self.capture_tx18(g,&mut c,"room-grid")?;pixels(&self.out.join("room-grid.bmp"))?};
  if pixel.0<1000||pixel.1<1000{return Err(format!("real native room/grid pixels {pixel:?}"))}
  if !reused_initial{self.case("all_types_derived_save_members_and_native_pixels",true,json!({"status":ready,"pixels":pixel,"types":27,"classes":17,"full_state_readback":true}));}
  // 真实门、坑桥、破坏、TimerPlate、岩石top等最终状态只应用纯值，不重演裁决。
  current["revision"]=json!("2");current["grids"][6]["state"]["derived"]["fields"]["GridEntityPit.HasLadder"]=json!(true);
  current["grids"][6]["state"]["common"]["fields"]["GridEntity.State"]=json!(1);current["room"]["cells"][37]["collision"]=json!(0);
  current["grids"][13]["state"]["common"]["fields"]["GridEntity.State"]=json!(1000);
  current["grids"][11]["state"]["common"]["fields"]["GridEntity.State"]=json!(4);
  current["grids"][15]["state"]["derived"]["fields"]["GridEntityDoor.IsOpen"]=json!(true);
  current["grids"][15]["state"]["derived"]["fields"]["GridEntityDoor.TargetRoomIndex"]=json!(-100);
  current["room"]["doors"][0]["target"]=json!({"kind":"DimensionRoute","index":-100,"route":"Mirror"});
  current["grids"][6]["state"]["derived"]["bridge_parent"]=json!({"type":"reference","status":"KnownCurrent","target":key(2)});
  self.submit_tx18(&mut c,"states",&current)?;self.wait_tx18(g,&mut c,"states_published",|s|n(s,"room_grid.revision")==2&&n(s,"presentation.retiring")==0)?;
  if self.report["cases"]["pit_bridge_destruction_door_timer_top_and_reference"]["pass"]!=true{
  for id in [7,12,14,16,20]{if self.object_tx18(&mut c,&id.to_string())?!=current["grids"][id-1]{return Err(format!("state transition {id}"))}}
  self.case("pit_bridge_destruction_door_timer_top_and_reference",true,json!({"state_readback":true,"native_actions_replayed":false}));
  }
  current["revision"]=json!("3");current["grids"][1]["key"]=key(2002);
  current["grids"][6]["state"]["derived"]["bridge_parent"]["target"]=key(2002);
  self.submit_tx18(&mut c,"replace",&current)?;self.wait_tx18(g,&mut c,"same_slot_replaced",|s|n(s,"room_grid.revision")==3&&n(s,"presentation.retiring")==0)?;
  if self.report["cases"]["same_slot_replacement_and_retirement"]["pass"]!=true{
  if !self.object_tx18(&mut c,"2")?.is_null()||self.object_tx18(&mut c,"2002")?!=current["grids"][1]{return Err("same-slot identity resurrection".into())}
  self.case("same_slot_replacement_and_retirement",true,json!({"old":"2","new":"2002","same_index":32}));
  }
  // 所有房形使用Authority显式mask；客户端不能把形状标签换算成随机房间。
  let mut revision=3;
  let reuse_shapes=self.report["cases"]["all_room_shapes_explicit_geometry_and_eight_door_slots"]["pass"]==true;
  for shape in 1..=12{revision+=1;current["revision"]=json!(revision.to_string());current["room"]["shape"]=json!(shape);
   current["room"]["state"]["fields"]["Room.GetRoomShape"]=json!(shape);
   for i in 0..448{let width=if shape<=5{15}else{28};current["room"]["cells"][i]["position"]=json!([40+(i%width)*40,80+(i/width)*40]);
    current["room"]["cells"][i]["inside"]=json!(match shape{1=>i<135,2=>i<135&&i/15>2&&i/15<6,3=>i<135&&i%15>4&&i%15<10,4=>i<240,5=>i<240&&i%15>4&&i%15<10,6=>i<252,7=>i<252&&i/28>2&&i/28<6,8=>true,9=>i%28>=14||i/28>=8,10=>i%28<14||i/28>=8,11=>i%28>=14||i/28<8,_=>i%28<14||i/28<8});}
   rebuild_floor(&mut current);
   if reuse_shapes{if shape==12{self.submit_tx18(&mut c,"shape-context",&current)?;self.wait_tx18(g,&mut c,"restore_shape_context",|s|n(s,"room_grid.revision")==revision)?;}continue;}
   self.submit_tx18(&mut c,&format!("shape-{shape}"),&current)?;self.wait_tx18(g,&mut c,&format!("shape_{shape}"),|s|n(s,"room_grid.revision")==revision)?;
   let mut metadata=current["room"].clone();metadata.as_object_mut().unwrap().remove("cells");if self.object_tx18(&mut c,"metadata")?!=metadata{return Err(format!("shape/environment/doors {shape}"))}
   if [1,8,9].contains(&shape){self.capture_tx18(g,&mut c,&format!("shape-{shape}"))?;let p=pixels(&self.out.join(format!("shape-{shape}.bmp")))?;
    if shape!=8&&p.0>=pixel.0{return Err(format!("shape {shape} retained full rectangular floor"))}}
  }
  if !reuse_shapes{self.case("all_room_shapes_explicit_geometry_and_eight_door_slots",true,json!({"shapes":12,"cells":448,"door_slots":8,"initial_all_cells_readback":true}));}
  // 所有失败候选均应保留整个已发布Room/Grid与资源版本。
  let mut bads=Vec::new();let mut bad=current.clone();bad["grids"][0]["class"]=json!("UnknownGrid");bads.push(("unknown-class",bad));
  let mut bad=current.clone();bad["grids"][0]["type"]=json!(28);bads.push(("unknown-type",bad));
  let mut bad=current.clone();bad["membership_complete"]=json!(false);bads.push(("partial-members",bad));
  let mut bad=current.clone();bad["room"]["cells"].as_array_mut().unwrap().pop();bads.push(("short-cells",bad));
  let mut bad=current.clone();bad["grids"][0]["state"]["common"]["fields"].as_object_mut().unwrap().remove("GridEntity.State");bads.push(("missing-field",bad));
  let mut bad=current.clone();bad["grids"][6]["state"]["derived"]["bridge_parent"]["target"]=key(999);bads.push(("missing-ref",bad));
  let mut bad=current.clone();bad["nodes"][4]["anm2"]=json!("gfx/isaac-cs-tx17/missing.anm2");bads.push(("missing-resource",bad));
  let mut bad=current.clone();bad["room"]["doors"][1]=bad["room"]["doors"][0].clone();bads.push(("duplicate-door",bad));
  let mut bad=current.clone();bad["nodes"][1]["pose"]["flags"]=json!(2);bads.push(("random-uv",bad));
  let mut bad=current.clone();bad["nodes"][1]["pose"]["flags"]=json!(128);bads.push(("local-time-effect",bad));
  let mut rejected=n(&c.command("cs status")?,"presentation.rejected");
  if self.report["cases"]["unknown_partial_missing_reference_and_resource_atomic_rejection"]["pass"]!=true{
  for (name,mut bad) in bads{bad["revision"]=json!((revision+1).to_string());self.submit_tx18(&mut c,name,&bad)?;
   let s=self.wait_tx18(g,&mut c,name,|s|n(s,"presentation.rejected")>rejected)?;rejected=n(&s,"presentation.rejected");
   if n(&s,"room_grid.revision")!=revision||n(&s,"presentation.revision")!=revision||self.object_tx18(&mut c,"2002")?!=current["grids"][1]{return Err(format!("partial candidate published {name}"))}
  }
  self.case("unknown_partial_missing_reference_and_resource_atomic_rejection",true,json!({"rejected":10,"old_revision":revision}));
  }
  revision+=1;current["revision"]=json!(revision.to_string());current["room"]["environment"]["water"]["amount"]=json!(0.5);
  let mut water=node(600,3,400,160,false);water["pose"]["scale"]=json!([4,1]);water["pose"]["color"]=json!([0.2,0.4,1,0.7,0,0,0,0,0,0,0]);
  current["nodes"].as_array_mut().unwrap().push(water);current["room"]["parts"].as_array_mut().unwrap().push(json!({"role":"water","node":"600"}));
  current["room"]["environment"]["floor_color"]=json!([0.3,0.5,0.8,1]);
  current["camera"]=json!({"origin":[5,3],"zoom":1.05});current["nodes"][3]["clamp_bottom"]=json!([0,12]);
  for tile in current["nodes"].as_array_mut().unwrap().iter_mut().filter(|n|n["pass"]==0){tile["pose"]["color"]=json!([0.3,0.5,0.8,1,0,0,0,0,0,0,0]);}
  self.submit_tx18(&mut c,"environment",&current)?;self.wait_tx18(g,&mut c,"environment_published",|s|n(s,"room_grid.revision")==revision&&n(s,"presentation.retiring")==0)?;
  if self.report["cases"]["environment_camera_water_clamp_native_pixels"]["pass"]!=true{
  self.capture_tx18(g,&mut c,"room-environment")?;let changed=pixels(&self.out.join("room-environment.bmp"))?;
  if changed==pixel{return Err("environment/camera/clamp made no pixel change".into())}
  self.case("environment_camera_water_clamp_native_pixels",true,json!({"before":pixel,"after":changed}));
  }
  revision+=1;current["revision"]=json!(revision.to_string());
  let mut layers=Vec::new();for id in 0..2{layers.push(json!({"id":id,"frame":0,"overlay_frame":null,"visible":true,
   "offset":[0,0],"scale":[1,1],"rotation":0,"color":[1,1,1,1,0,0,0,0,0,0,0],"flip_x":false,"flip_y":false,
   "crop":[3,2],"flags":2,"blend":[0,1,7,1,7],"custom_shader":null,"champion_shader":null}));}
  current["nodes"][1]["pose"]["flags"]=json!(2);current["nodes"][1]["pose"]["resolved_random_uv"]=json!(true);current["nodes"][1]["pose"]["layers"]=json!(layers);
  self.submit_tx18(&mut c,"resolved-random",&current)?;
  let resolved=self.wait_tx18(g,&mut c,"resolved_uv_published",|s|n(s,"room_grid.revision")==revision&&n(s,"presentation.rng_checks")>20)?;
  if n(&resolved,"presentation.rng_changes")!=0{return Err("resolved UV drew from local RNG".into())}
  self.case("authority_resolved_random_uv_without_local_rng",true,json!({"checks":n(&resolved,"presentation.rng_checks"),"changes":0}));
  let counters=c.command("cs counters")?;for name in ["start_game","game_update","room_update","entity_list","odd_helper","restore","post_update","player_update","join_pass"]{let k=format!("hook.{name}");if n(&before,&k)!=n(&counters,&k){return Err(format!("local gameplay entered {name}"))}}
  if callbacks!=c.command(&format!("cs presentation_test {token} callbacks"))?{return Err("gameplay Lua resumed".into())}
  let isolated=c.command("cs status")?;
  if n(&isolated,"client_world_applied")!=0{return Err("domain apply became whole-world Applied".into())}
  if n(&isolated,"presentation.rng_checks")==0||n(&isolated,"presentation.rng_changes")!=0{return Err("Client draw changed global render RNG".into())}
  self.case("application_keeps_client_simulation_and_persistence_isolated",true,json!({"native_before":before,"native_after":counters,"whole_world_applied":false}));
  // 完整空成员集才退休所有Grid；没有遗留门别名、旧主体或旧视觉部件。
  revision+=1;let mut empty=current.clone();empty["revision"]=json!(revision.to_string());empty["grids"]=json!([]);
  empty["nodes"].as_array_mut().unwrap().retain(|n|{let id=n["id"].as_str().unwrap().parse::<u64>().unwrap();id==2||id==600||id>=1000});
  for door in empty["room"]["doors"].as_array_mut().unwrap(){door["grid"]=Value::Null;}
  self.submit_tx18(&mut c,"remove-members",&empty)?;self.wait_tx18(g,&mut c,"all_grid_members_removed",|s|n(s,"room_grid.revision")==revision&&n(s,"room_grid.grids")==0&&n(s,"presentation.retiring")==0)?;
  if !self.object_tx18(&mut c,"2002")?.is_null(){return Err("removed member query survived".into())}
  self.case("complete_member_deletion_without_ghosts",true,json!({"grids":0,"old_member":null}));
  c.command("cs stop")?;self.wait_tx18(g,&mut c,"stop_drains_domain",|s|s.get("role").map(String::as_str)==Some("Off")&&n(s,"presentation.drained")==1&&n(s,"room_grid.cells")==0)?;
  c.command(&format!("cs client 127.0.0.1 {port} 0"))?;let next=c.wait("role","Client",10)?;
  current["connection"]=json!(n(&next,"client.connection").to_string());current["revision"]=json!("1");self.submit_tx18(&mut c,"normal-close",&current)?;
  self.wait_tx18(g,&mut c,"close_room_ready",|s|n(s,"room_grid.revision")==1)?;self.report["final_status"]=json!(c.command("cs status")?);drop(c);
  if g.close()?!=0{return Err("normal close".into())}self.snapshot("save-process-exit")?;
  let changes=crate::saves::changes(&self.out.join("save-client-before"),&self.out.join("save-process-exit"))?;
  if changes.as_array().ok_or("save changes")?.iter().any(|r|{let p=r["path"].as_str().unwrap_or("").to_lowercase();r["bytes_changed"]==true&&(p.contains("persistentgamedata")||p.contains("gamestate")||p.contains("savesyncstatus"))}){return Err("visitor progress changed".into())}
  self.case("stop_reconnect_domain_retirement_and_normal_exit",true,json!({"changes":changes,"exit":0}));Ok(())
 }
}
