//! I38：依实施计划失败模型，真实Source和独立冷Client共用正式Scene；不构建假Player。
use crate::{Control,Result,Run,win32};
use crate::tx19::{num,n,drawn,load,image_pair,projection};
use serde_json::{Value,json};
use std::{collections::{BTreeMap,BTreeSet},process::Command,time::Instant};
type Status=BTreeMap<String,String>;
// 正式目标按线协议紧凑编码；缩进不能消耗32MiB的输入预算，也不能遮蔽具体负向判据。
fn write_target(path:&std::path::Path,value:&Value)->Result<()>{
 std::fs::write(path,serde_json::to_vec(value).map_err(|e|e.to_string())?).map_err(|e|e.to_string())
}
fn bodies(t:&Value)->Result<&Vec<Value>>{t["components"]["player"]["state"]["bodies"].as_array().ok_or("complete Player bodies required".into())}
// 场景的预期必须在正式来源中实际成立；label或公开动作返回不替代这些判据。
pub(super) fn source_scene(s:&Value,t:&Value)->Result<()> {
 if !s["kind"].as_str().unwrap_or("").starts_with("player-"){crate::tx19::scene_source(s,t,&json!([]))?;}
 let ps=&t["components"]["player"]["state"];let bs=bodies(t)?;
 if ps["application_source"]["contract"]!="player-source-v2"||bs.is_empty(){return Err("Player formal source contract missing".into())}
 let types=bs.iter().map(|b|num(&b["relations"]["player_type"])).collect::<BTreeSet<_>>();
 if let Some(expected)=s["expect_types"].as_array(){for e in expected {if !types.contains(&num(e)){return Err(format!("{} actual PlayerType {} missing: {types:?}",s["label"],num(e)))}}}
 if let Some(count)=s["expect_roster"].as_u64(){if ps["roster"].as_array().map(|r|r.len()as u64)!=Some(count){return Err(format!("{} actual roster count",s["label"]))}}
 if let Some(count)=s["participants"].as_u64(){if ps["control_groups"].as_array().map(|r|r.len()as u64)!=Some(count){return Err(format!("{} actual control groups",s["label"]))}}
 if let Some(count)=s["expect_groups"].as_u64(){if ps["control_groups"].as_array().map(|r|r.len()as u64)!=Some(count){return Err(format!("{} actual remaining control groups",s["label"]))}}
 if let Some(count)=s["expect_backups"].as_u64(){let actual=ps["backup_slots"].as_array().ok_or("backup slots")?.iter().filter(|v|v["body"]["status"]=="KnownCurrent").count()as u64;if actual!=count{return Err(format!("{} real backup slots {actual}/{count}",s["label"]))}}
 if s["kind"]=="player-weapon"{
  let wanted=num(&s["weapon_type"]);let slot=if wanted==11||wanted==12{0}else{1};let charge=s["charge"].as_f64().unwrap_or(0.0);
  // 同一个真实槽的私有动态型、公开型和实际充能共同符合；11/12按Wiki为备用槽0，其余为主槽1。
  let found=crate::tx19::objects(t)?.iter().filter(|o|o["class"]=="EntityPlayer").any(|o|{
   let native=&o["body"]["application_player_equipment"]["weapons"][slot];let public=&o["body"]["fields"]["EntityPlayer.GetWeapon"]["values"][slot]["value"]["fields"];
   native["present"]==true&&num(&native["type"])==wanted&&num(&public["Weapon.GetWeaponType"])==wanted&&public["Weapon.GetCharge"].as_f64().is_some_and(|v|(v-charge).abs()<0.00001)
  });
  if !found{return Err(format!("{} actual weapon slot/type/charge {slot}/{wanted}/{charge} absent",s["label"]))}
 }
 // 专用场景必须制造实际有限槽／依赖，公开动作返回或角色label不能替代覆盖。
 let rows=crate::tx19::objects(t)?;let kind=s["kind"].as_str().unwrap_or("");
 if let Some(expected)=s["expect_roster_types"].as_array(){let mut actual=Vec::new();for r in ps["roster"].as_array().ok_or("roster types")?{let o=rows.iter().find(|o|o["subject"]["target"]==r["body"]["target"]).ok_or("roster body")?;actual.push(num(&o["body"]["fields"]["EntityPlayer.GetPlayerType"]));}actual.sort();let mut expected=expected.iter().map(num).collect::<Vec<_>>();expected.sort();if actual!=expected{return Err(format!("{} actual roster types {actual:?}/{expected:?}",s["label"]))}}
 if kind=="player-stress"{
  let roster=ps["roster"].as_array().ok_or("stress roster")?;let mut weapons=BTreeSet::new();let mut role_counts=[0,0];
  for r in roster{let o=rows.iter().find(|o|o["subject"]["target"]==r["body"]["target"]).ok_or("stress current body")?;let e=&o["body"]["application_player_equipment"];
   if !matches!(num(&o["body"]["fields"]["EntityPlayer.GetPlayerType"]),19|20){return Err("stress roster contains a non-Jacob/Esau body".into())}
   role_counts[(num(&o["body"]["fields"]["EntityPlayer.GetPlayerType"])-19)as usize]+=1;
   if !o["body"]["fields"]["EntityPlayer.GetCostumeSpriteDescs"]["entries"].as_array().is_some_and(|v|v.len()>=3){return Err("stress body lacks actual complex costume container".into())}
   let ws=e["weapons"].as_array().ok_or("stress weapon slots")?;let w=ws.iter().find(|w|w["present"]==true).ok_or("stress body weapon missing")?;weapons.insert(num(&w["type"]));
  }if role_counts!=[4,4]||weapons.len()<6{return Err(format!("stress actual roles/weapons incomplete: {role_counts:?}/{weapons:?}"))}
 }
 if matches!(kind,"player-equipment-final"|"player-equipment-empty"){
  let key=&ps["roster"][0]["body"]["target"];let o=rows.iter().find(|o|o["subject"]["target"]==*key).ok_or("equipment root")?;let state=&o["body"]["application_player_state"];
  for (class,persistent_id,timed_id) in [("collectible_spoof",3,4),("trinket_spoof",1,2)]{
   let spoof=&state[class];let groups=spoof["groups"].as_array().map(Vec::as_slice).unwrap_or(&[]);
   let blocked=spoof["blocked"].as_array().is_some_and(|v|v.iter().any(|v|num(v)==1));
   if kind=="player-equipment-final"{
    let permanent=groups.iter().find(|g|g["key"]=="tx20-permanent").ok_or("actual persistent innate group missing")?;
    let timed=groups.iter().find(|g|g["key"]=="tx20-timed").ok_or("actual timed innate group missing")?;
    if !blocked||!permanent["persistent"].as_array().is_some_and(|v|v.iter().any(|r|num(&r["id"])==persistent_id&&num(&r["count"])==2))||!timed["temporary"].as_array().is_some_and(|v|v.iter().any(|r|num(&r["id"])==timed_id&&num(&r["count"])==1&&num(&r["timer"])>0)){return Err(format!("actual {class} innate state differs"))}
   }else if blocked||groups.iter().filter(|g|matches!(g["key"].as_str(),Some("tx20-permanent"|"tx20-timed"))).any(|g|["persistent","temporary"].iter().any(|k|g[k].as_array().is_some_and(|a|!a.is_empty()))){return Err("named innate/blocked state did not clear".into())}
  }
  let active=state["active"].as_array().ok_or("active slots")?;let pocket=state["pocket"].as_array().ok_or("pocket slots")?;
  if kind=="player-equipment-final"{
   if !active.iter().zip([34,33,105,36]).all(|(v,id)|num(&v["item"])==id)||active[0]["partial"].as_f64()!=Some(0.5)||num(&active[1]["battery"])!=1||num(&active[0]["sub"])!=2||num(&active[0]["cooldown"])==0||!pocket.iter().zip([(3,2),(4,2),(1,1),(1,0)]).all(|(v,(id,ty))|num(&v["id"])==id&&num(&v["type"])==ty){return Err(format!("actual mixed pockets/active fractional state differs: {active:?}/{pocket:?}"))}
  }else if active.iter().any(|v|num(&v["item"])!=0)||pocket.iter().any(|v|num(&v["id"])!=0){return Err("actual active/pocket slots did not clear".into())}
 }
 if matches!(kind,"player-death"|"player-ghost"|"player-revive"){
  let key=&ps["roster"][0]["body"]["target"];let body=rows.iter().find(|o|o["class"]=="EntityPlayer"&&o["subject"]["target"]==*key).ok_or("life scene actual roster root missing")?;
  let dead=body["common"]["fields"]["Entity.IsDead"]==true;let ghost=body["body"]["fields"]["EntityPlayer.IsCoopGhost"]==true;
  if body["common"]["fields"]["Entity.Exists"]!=true||match kind{"player-death"=>!dead,"player-ghost"=>!ghost,_=>dead||ghost}{return Err(format!("{} actual life state failed: dead={dead},ghost={ghost}",s["label"]))}
 }
 if matches!(kind,"player-hold"|"player-throw"|"player-land"){
  let body=rows.iter().find(|o|o["class"]=="EntityPlayer"&&num(&o["body"]["fields"]["EntityPlayer.GetPlayerType"])==35&&o["common"]["fields"]["Entity.Exists"]==true).ok_or("live Tainted Forgotten missing")?;
  let soul=rows.iter().find(|o|o["class"]=="EntityPlayer"&&num(&o["body"]["fields"]["EntityPlayer.GetPlayerType"])==40&&o["common"]["fields"]["Entity.Exists"]==true&&o["body"]["fields"]["EntityPlayer.GetOtherTwin"]["target"]==body["subject"]["target"]).ok_or("actual Tainted Soul twin missing")?;
  let flags=num(&body["common"]["fields"]["Entity.GetEntityFlags"]);let held=flags&(1u64<<57)!=0;let thrown=flags&(1u64<<58)!=0;
  let link=&soul["body"]["fields"]["EntityPlayer.GetHeldEntity"];
  let velocity=&body["common"]["fields"]["Entity.Velocity"];let speed=velocity["x"].as_f64().ok_or("throw velocity x")?.hypot(velocity["y"].as_f64().ok_or("throw velocity y")?);
  let valid=match kind{"player-hold"=>held&&!thrown&&link["status"]=="KnownCurrent"&&link["target"]==body["subject"]["target"],"player-throw"=>!held&&thrown&&link["status"]=="Null"&&speed>0.1,_=>!held&&!thrown&&link["status"]=="Null"&&speed<=0.1&&body["common"]["fields"]["Entity.PositionOffset"]["y"]==0.0};
  if !valid{return Err(format!("{} actual hold/throw/land state failed: flags={flags},speed={speed},held={link}",s["label"]))}
 }
 if kind=="player-birthright-clear"&&rows.iter().filter(|o|o["class"]=="EntityPlayer").any(|o|num(&o["body"]["application_player_state"]["collectibles"][619])!=0){return Err("Birthright still held after explicit removal".into())}
 if kind=="player-flash"{
  let key=&ps["roster"][0]["body"]["target"];let body=rows.iter().find(|o|o["subject"]["target"]==*key).ok_or("flash current roster body")?;
  let cooldown=num(&body["body"]["application_player_state"]["values"]["EntityPlayer.GetDamageCooldown"]);
  if (num(&s["cooldown"])==0)!=(cooldown==0){return Err(format!("{} actual damage cooldown {cooldown}",s["label"]))}
 }
 if kind=="player-crafting"||kind=="player-poop"{
  let (name,count)=if kind=="player-crafting"{("crafting",8)}else{("poop",6)};
  if !rows.iter().filter(|o|o["class"]=="EntityPlayer").any(|o|o["body"]["application_player_special"][name].as_array().is_some_and(|a|a.len()==count&&a.iter().enumerate().all(|(i,v)|num(v)==i as u64+1))){return Err(format!("{} actual {name} slots absent",s["label"]))}
 }
 let variant=match kind{"player-clot"=>Some(238),"player-wisp"=>Some(206),"player-locust"=>Some(231),_=>None};
 if let Some(variant)=variant{if !rows.iter().any(|o|o["class"]=="EntityFamiliar"&&num(&o["common"]["fields"]["Entity.Variant"])==variant&&o["common"]["fields"]["Entity.Exists"]!=false){return Err(format!("{} actual familiar variant {variant} absent",s["label"]))}}
 if kind=="player-dark-esau"&&!rows.iter().any(|o|num(&o["common"]["fields"]["Entity.Type"])==866&&o["common"]["fields"]["Entity.Exists"]!=false){return Err("actual Dark Esau absent".into())}
 if kind=="player-minecart"&&!rows.iter().filter(|o|o["class"]=="EntityPlayer").any(|o|{
  let r=&o["common"]["fields"]["Entity.GetMinecart"];r["status"]=="KnownCurrent"&&rows.iter().any(|cart|cart["class"]=="EntityNPC"&&cart["subject"]["target"]==r["target"])
 }){return Err("actual Player minecart NPC relationship absent".into())}
 Ok(())
}
impl Run {
 // 复用18真实环境的完整Source，不使用其旧单Isaac代表投影；只在20统一入口显式启用。
 pub(super) fn player_environment_tx20(&mut self,g:&win32::Game,w:usize,c:&mut Control,source:&std::path::Path)->Result<()>{
  let mut results=Vec::new();
  for label in ["all-grids","water","dense","mirror_floor","mirror_inside"]{
   c.command("cs stop")?;c.wait("role","Off",60)?;
   let t=load(&source.join(format!("runtime-world/client-targets/source-full-{label}.json")))?;
   if t["components"]["player"]["state"]["application_source"]["contract"]!="player-source-v2"{return Err(format!("{label} full Player source missing"))}
   let name=format!("player-env-{label}");write_target(&self.out.join(format!("runtime-world/client-targets/{name}.json")),&t)?;
   let at=Instant::now();let status=self.apply_tx20(g,c,&name,true)?;let native=self.inspect_tx20(c,&t,&name)?;
   self.wait_tx19(g,c,&name,false,|s|drawn(s)>=drawn(&status)+2)?;
   let counters=c.command("cs counters")?;for key in ["start_game","game_update","room_update","entity_list","odd_helper","restore","post_update","player_update","join_pass"]{if n(&status,&format!("client.original_begin.{key}"))!=n(&counters,&format!("hook.{key}")){return Err(format!("{name} full Player environment gameplay leaked {key}"))}}
   for _ in 0..2{let duplicate=self.apply_tx20(g,c,&name,false)?;for key in ["client.native_attempt","client.resource_candidate_loads"]{if n(&duplicate,key)!=n(&status,key){return Err(format!("{name} duplicate reconstructed {key}"))}}}
   if self.inspect_tx20(c,&t,&name)?!=native{return Err(format!("{name} duplicate changed actual owners"))}
   let screenshot=self.out.join(format!("{name}.bmp"));win32::screenshot(w,&screenshot)?;let pixels=image_pair(&source.join(format!("source-{label}.bmp")),&screenshot)?;
   results.push(json!({"label":label,"source_kind":"source-full;complete Player owner","status":status,"native":native,"pixels":pixels,"wall_ms_including_IPC":at.elapsed().as_secs_f64()*1000.0,"visual_review":"pending"}));
   crate::write_json(&self.out.join("player-environment-progress.json"),&json!(results))?;
  }
  self.case("tx20.E20.full_player_environment",true,json!({"source":source,"scenes":results}));Ok(())
 }
 // 真正的拥有槽使用走A/B相同canonical路径；每组独立等待原生替换和重绑，避免同栈撤销其他组。
 pub(super) fn esau_inputs_tx20(&mut self,g:&win32::Game,c:&mut Control,w:usize,remotes:&[crate::remote::Remote],label:&str,action:u64)->Result<()>{
  if remotes.len()!=3{return Err("four-participant Esau input requires three remotes".into())}
  let until=Instant::now()+std::time::Duration::from_secs(60);
  loop{self.watch_tx05(g,c,30)?;let rows=self.live_fixture()?;if rows.iter().any(|e|e["event"]=="esau_prepared"&&e["label"]==label&&num(&e["action"])==action){break}
   if let Some(e)=rows.iter().find(|e|e["event"]=="fixture_error"){return Err(format!("Esau preparation: {e}"))}if Instant::now()>=until{return Err("Esau preparation timeout".into())}}
  let mut uses=Vec::new();for ordinal in 0..4usize{
   let prefix=format!("participant.{ordinal}.");let ready=self.wait_tx09(g,c,"esau_input_generation",|s|n(s,"blocks")==0&&n(s,&format!("{prefix}bound"))==1&&(ordinal==0||remotes[ordinal-1].snapshot().ack.is_some_and(|a|a.state==3&&a.generation==n(s,&format!("{prefix}generation")))))?;
   let generation=n(&ready,&format!("{prefix}generation"));let address=ready.get(&format!("{prefix}body.0.address")).ok_or("Esau root address")?.clone();
   // 每次替换后重新观察连续提交的owner节奏；冻结/生命周期期间不沿用旧奇偶预测。
   let(mut parity,mut parity_start)=(None,0);let stable_until=Instant::now()+std::time::Duration::from_secs(15);
   let standard_parity=loop{let s=self.watch_tx05(g,c,10)?;let tick=n(&s,"completed_tick");let p=if n(&s,"completed_standard")==1{Some(tick%2)}else if n(&s,"completed_odd")==1{Some(1-tick%2)}else{None};
    if n(&s,"blocks")!=0||n(&s,&format!("{prefix}bound"))!=1||n(&s,&format!("{prefix}generation"))!=generation{return Err("Esau binding changed before scheduled input".into())}
    if p!=parity{parity=p;parity_start=tick;}if let Some(p)=p{if tick>=parity_start+8{break p}}
    if Instant::now()>=stable_until{return Err("Esau ordinary consumer cadence not stable".into())}
   };
   // 异步安装相对最新Ack；只隔一tick会在同次采样跨过按下/松开，形成edge有而held=0。
   // 保持八tick连续量但只有首个边沿，给真实原生主动consumer完整按下窗口。
   if ordinal==0{win32::key(w,"SPACE")?;}else{remotes[ordinal-1].script_on_tick_parity("tx20-owned-esau",vec![crate::remote::Step{after:0,state:crate::remote::protocol::State{held:2,edge:2,..Default::default()}},crate::remote::Step{after:8,state:Default::default()}],standard_parity)?;}
   let after=self.wait_tx09(g,c,&format!("esau_native_replacement.{ordinal}"),|s|n(s,&format!("{prefix}bound"))==1&&n(s,&format!("{prefix}generation"))>generation&&s.get(&format!("{prefix}body.0.address")).is_some_and(|v|v!=&address))?;
   for remote in remotes{if let Some(e)=remote.snapshot().error{return Err(format!("Esau remote: {e}"))}}
   uses.push(json!({"ordinal":ordinal,"generation_before":generation,"generation_after":n(&after,&format!("{prefix}generation")),"ordinary_target_tick_parity":standard_parity,"actual_root_changed":true}));
   self.report["esau_inputs"][label]=json!(uses);
  }
  self.report["esau_inputs"][label]=json!(uses);Ok(())
 }
 fn apply_tx20(&mut self,g:&win32::Game,c:&mut Control,label:&str,begin:bool)->Result<Status>{
  let t=load(&self.out.join(format!("runtime-world/client-targets/{label}.json")))?;let r=num(&t["revision"]);let token=self.report["token"].as_str().ok_or("token")?;
  let reply=c.command(&format!("cs client_test {token} {} {label}",if begin{"begin"}else{"target"}))?;
  if reply.get("accepted").map(String::as_str)!=Some("1"){return Err(format!("{label} admission: {reply:?}; {:?}",c.command("cs status")?))}
  let s=self.wait_tx19(g,c,label,false,|s|n(s,"client_world_applied")==1&&n(s,"client.applied_revision")==r)?;
  for k in ["full_entity_applied","player_bodies_applied","player_relations_applied","player_resources_applied","player_domain_applied"]{if n(&s,&format!("client.{k}"))!=1{return Err(format!("{label} incomplete actual range {k}: {s:?}"))}}
  // DomainId从1开始：Entity1/Grid2/Room3/Player7恰为142；其余域未原生应用，不能置位或外推网络ACK。
  if n(&s,"client.native_committed_domains")!=142{return Err(format!("Player application committed range differs: {:?}",s.get("client.native_committed_domains")))}
  if n(&s,"client.pending_owners")!=0||n(&s,"client.native_pending_owners")!=0{return Err(format!("{label} committed with undrained candidate owners"))}
  Ok(s)
 }
 fn inspect_tx20(&mut self,c:&mut Control,t:&Value,label:&str)->Result<Value>{
  let token=self.report["token"].as_str().ok_or("token")?;let r=c.command(&format!("cs client_test {token} inspect"))?;
  if r.get("native_checked").map(String::as_str)!=Some("1")||r.get("player_native_checked").map(String::as_str)!=Some("1")||n(&r,"player_revision")!=num(&t["revision"]){return Err(format!("{label} real native inspect: {r:?}"))}
  let ps:Value=serde_json::from_str(r.get("players").ok_or("native players")?).map_err(|e|e.to_string())?;
  let es:Value=serde_json::from_str(r.get("objects").ok_or("native objects")?).map_err(|e|e.to_string())?;
  let mut actual=BTreeSet::new();for row in es.as_array().ok_or("objects array")?{actual.insert((num(&row["run"]),num(&row["id"])));}
  let mut players=BTreeSet::new();
  for row in ps.as_array().ok_or("players array")?{
   let key=(num(&row["run"]),num(&row["id"]));if !players.insert(key)||num(&row["instance"])==0||row["controller_index"]!=-1{return Err(format!("{label} native identity/input isolation: {row}"))}
   actual.insert(key);let body=bodies(t)?.iter().find(|b|(num(&b["subject"]["target"]["run"]),num(&b["subject"]["target"]["id"]))==key).ok_or("unexpected native body")?;
   if num(&row["player_type"])!=num(&body["relations"]["player_type"]){return Err(format!("{label} actual Player type differs"))}
   let m=&body["native_membership"];for (native,source) in [("room_members","room_owners"),("render_members","render_members"),("update_members","update_members")]{if num(&row[native])!=num(&m[source]){return Err(format!("{label} native {native} differs"))}}
  }
  // 非活动/备份身体可以合法Exists=false；Player当前成员由两域身体全集决定，不能按可见性删掉。
  // Exists=false不等于身份已退休；完整Entity owner也必须保留仍KnownCurrent的离场终态。
  let mut expected=BTreeSet::new();for row in crate::tx19::objects(t)?{if row["subject"]["status"]=="KnownCurrent"{expected.insert((num(&row["subject"]["target"]["run"]),num(&row["subject"]["target"]["id"])));}}
  for body in bodies(t)?{expected.insert((num(&body["subject"]["target"]["run"]),num(&body["subject"]["target"]["id"])));}
  if players.len()!=bodies(t)?.len()||actual!=expected{return Err(format!("{label} native body/Entity inventory differs"))}
  Ok(json!({"players":ps,"objects":es,"read_only_check":true}))
 }
 // 恶意纯值目标经正式入口；每个写前拒绝必须保持A和真实owner不变。
 fn negatives_tx20(&mut self,g:&win32::Game,c:&mut Control,base:&Value)->Result<()> {
  let token=self.report["token"].as_str().ok_or("token")?.to_owned();let r=num(&base["revision"]);let before=c.command("cs status")?;let native=self.inspect_tx20(c,base,"negative-before")?;let mut results=Vec::new();
  for name in ["old","conflict","partial","source-version","mixed-window","cross-scope","missing-body","duplicate-body","unknown-role","width","unresolved","backup-slot","missing-final-state","missing-resource","history-gap","budget","wrong-group","wrong-shape","missing-twin","self-held","native-claim"]{
   let mut v=base.clone();crate::tx18::revision(&mut v,if name=="old"{r.saturating_sub(1)}else if name=="conflict"{r}else{r+1});let p=&mut v["components"]["player"];
   match name{
    "old"=>(),"conflict"=>p["state"]["marker"]=json!("same revision conflict"),"partial"=>p["coverage"]=json!("Partial"),
    "source-version"=>p["state"]["application_source"]["version"]=json!(999),"mixed-window"=>p["capture"]["owner"]=json!("999999"),
    "cross-scope"=>p["state"]["bodies"][0]["subject"]["target"]["run"]=json!("999999"),
    "missing-body"=>{p["state"]["bodies"].as_array_mut().ok_or("bodies")?.pop();},
    "duplicate-body"=>{let b=p["state"]["bodies"][0].clone();p["state"]["bodies"].as_array_mut().ok_or("bodies")?.push(b);p["state"]["membership"]["count"]=json!(p["state"]["bodies"].as_array().unwrap().len());},
    "unknown-role"=>p["state"]["bodies"][0]["relations"]["player_type"]=json!(999),
    "width"=>{let body=p["state"]["bodies"].as_array_mut().ok_or("bodies")?.iter_mut().find(|b|b["relations"]["membership"]["control_group_local_id"].is_object()).ok_or("actual known membership")?;body["relations"]["membership"]["ordinal"]=json!(4294967296u64);},
    "unresolved"=>p["state"]["bodies"][0]["relations"]["other_twin"]=json!({"status":"Unresolved","reason":"E2E explicit missing relationship"}),
    "backup-slot"=>p["state"]["backup_slots"][0]["index"]=json!(4),
    "history-gap"=>p["state"]["bodies"][0]["history"]=json!({"status":"ReadFailed","reason":"E2E"}),
    "wrong-group"=>{let index=p["state"]["bodies"].as_array().ok_or("bodies")?.iter().position(|b|b["relations"]["membership"]["control_group_local_id"].is_object()).ok_or("actual known membership")?;let original=p["state"]["bodies"][index]["relations"]["membership"]["control_group_local_id"].clone();let other=p["state"]["control_groups"].as_array().ok_or("groups")?.iter().find(|g|g["subject"]["target"]!=original).ok_or("second actual group")?["subject"]["target"].clone();p["state"]["bodies"][index]["relations"]["membership"]["control_group_local_id"]=other;},
    "wrong-shape"=>{let shape=num(&p["state"]["bodies"][0]["subject"]["shape_revision"]);p["state"]["bodies"][0]["subject"]["shape_revision"]=json!(shape+1);},
    "missing-twin"=>{
     let key=p["state"]["bodies"].as_array().ok_or("bodies")?.iter().find(|b|b["relations"]["in_public_roster"]==true&&num(&b["relations"]["player_type"])==40).ok_or("actual Tainted Soul twin")?["subject"]["target"].clone();
     let ps=&mut p["state"];ps["bodies"].as_array_mut().ok_or("bodies")?.retain(|b|b["subject"]["target"]!=key);ps["membership"]["count"]=json!(ps["bodies"].as_array().unwrap().len());
     let roster=ps["roster"].as_array_mut().ok_or("roster")?;roster.retain(|b|b["body"]["target"]!=key);for(i,b)in roster.iter_mut().enumerate(){b["index"]=json!(i);}
     for g in ps["control_groups"].as_array_mut().ok_or("groups")?{g["memberships"].as_array_mut().ok_or("memberships")?.retain(|b|b["subject"]["target"]!=key);g["bodies"].as_array_mut().ok_or("anchors")?.retain(|b|b["target"]!=key);}
     let es=&mut v["components"]["entity"]["state"];es["objects"].as_array_mut().ok_or("objects")?.retain(|b|b["subject"]["target"]!=key);es["membership"]["count"]=json!(es["objects"].as_array().unwrap().len());
    },
    "self-held"=>{let row=v["components"]["entity"]["state"]["objects"].as_array_mut().ok_or("objects")?.iter_mut().find(|o|o["class"]=="EntityPlayer").ok_or("actual Player")?;row["body"]["fields"]["EntityPlayer.GetHeldEntity"]=row["subject"].clone();},
    "native-claim"=>v["native_applied"]=json!(true),
    "budget"=>{let b=p["state"]["bodies"][0].clone();for _ in 0..257{p["state"]["bodies"].as_array_mut().ok_or("bodies")?.push(b.clone());}p["state"]["membership"]["count"]=json!(p["state"]["bodies"].as_array().unwrap().len());},
    _=>{let rows=v["components"]["entity"]["state"]["objects"].as_array_mut().ok_or("objects")?;let b=rows.iter_mut().find(|v|v["class"]=="EntityPlayer").ok_or("Player")?;
     b["body"].as_object_mut().ok_or("body")?.remove(if name=="missing-final-state"{"application_player_state"}else{"application_player_resources"});}
   }
   let label=format!("bad-{name}");write_target(&self.out.join(format!("runtime-world/client-targets/{label}.json")),&v)?;
   let result=c.request(&format!("cs client_test {token} target {label}"))?;let after=c.command("cs status")?;
   if result.get("accepted").map(String::as_str)==Some("1")||n(&after,"client.applied_revision")!=r||n(&after,"client_world_applied")!=1||n(&after,"client.native_attempt")!=n(&before,"client.native_attempt")||self.inspect_tx20(c,base,name)?!=native{return Err(format!("{name} write-before reject failed: {result:?}; {after:?}"))}
   // codec拒绝不会重写native首因；优先记录本次入口错误，不能沿用上一个负向目标的诊断。
   let reason=result.get("error").or_else(||result.get("reason")).or_else(||after.get("client.native_reason")).cloned();
   if (name=="budget"&&reason.as_deref()!=Some("player:membership:count_or_budget"))||(name=="duplicate-body"&&reason.as_deref()!=Some("player:body:missing_entity_or_duplicate")){return Err(format!("{name} did not reach its specific reject: {reason:?}"))}
   results.push(json!({"name":name,"reply":result,"reason":reason}));self.watch_tx05(g,c,0)?;
  }
  // 版本拒绝使用正式协商入口，分别改变Player合同、来源和目录，不能靠未知JSON键代测。
  let caps_path=self.out.join("runtime-world/client-targets/player-caps.json");
  let generated=Command::new(self.repo.join("out/repentogon/e2e/client-fixture.exe")).arg("--capabilities").arg(&caps_path).output().map_err(|e|e.to_string())?;
  if !generated.status.success(){return Err("Player capability source generation".into())}let caps=load(&caps_path)?;
  for name in ["id","source","catalog"]{let mut old=caps.clone();old["model_categories"]["player_target"][name]=if name=="catalog"{json!(0)}else{json!("incompatible-e2e-contract")};let label=format!("player-caps-{name}");
   write_target(&self.out.join(format!("runtime-world/client-targets/{label}.json")),&old)?;let reply=c.command(&format!("cs client_test {token} negotiate {label}"))?;
   if reply.get("accepted").map(String::as_str)!=Some("0"){return Err(format!("Player {name} version accepted: {reply:?}"))}results.push(json!({"compatibility":name,"reply":reply}));
  }
  self.case("E22",true,json!(results));Ok(())
 }
 // 每个真实原生前缀的一次故障；自动安全恢复不抹去故障记录，也不假设可恢复未知SEH。
 fn faults_tx20(&mut self,g:&win32::Game,c:&mut Control,label:&str)->Result<()> {
  let token=self.report["token"].as_str().ok_or("token")?.to_owned();let t=load(&self.out.join(format!("runtime-world/client-targets/{label}.json")))?;let mut results=Vec::new();
  c.command("cs stop")?;c.wait("role","Off",60)?;
  for point in ["allocate","ctor","init","fields","resources","hud","roster","publish","weapon","relations","check","draw"]{for side in ["before","after"]{
   let point=format!("player-{point}-{side}");let arm=c.command(&format!("cs client_test {token} player-fault {point}"))?;if arm.get("armed")!=Some(&point){return Err(format!("fault arm {arm:?}"))}
   let start=c.command(&format!("cs client_test {token} begin {label}"))?;if start.get("accepted").map(String::as_str)!=Some("1"){return Err(format!("{point} begin rejected: {start:?}"))}
   // 命中记录常驻至下一arm；不能仅等待可能被自动恢复掩盖的一帧Failed。
   let end=Instant::now()+std::time::Duration::from_secs(90);let hit=loop{
    let hit=c.command(&format!("cs client_test {token} player-fault-status"))?;if hit.get("hit")==Some(&point)&&n(&hit,"hits")==1{break hit}
    self.watch_tx05(g,c,60)?;if Instant::now()>=end{return Err(format!("{point} real prefix not reached: {hit:?}; {:?}",c.command("cs status")?))}
   };
   let state=self.wait_tx19(g,c,&point,true,|s|n(s,"client_world_applied")==1||s.get("client.native_phase").map(String::as_str)==Some("retired"))?;
   if n(&state,"client_world_applied")==0{
    let mut recovery=t.clone();crate::tx18::revision(&mut recovery,num(&t["revision"])+1);write_target(&self.out.join("runtime-world/client-targets/player-recover.json"),&recovery)?;
    self.apply_tx20(g,c,"player-recover",false)?;self.inspect_tx20(c,&recovery,&point)?;
   }else{self.inspect_tx20(c,&t,&point)?;}
   results.push(json!({"point":point,"hit":hit,"state":state,"recovered":true}));crate::write_json(&self.out.join("player-fault-progress.json"),&json!(results))?;
   c.command("cs stop")?;c.wait("role","Off",60)?;
  }}
  for mode in ["begin-cancel","begin-cancel-native","begin-cancel-grid","begin-cancel-display"]{let r=c.command(&format!("cs client_test {token} {mode} {label}"))?;if r.get("accepted").map(String::as_str)!=Some("1"){return Err(format!("cancel rejected: {r:?}"))}c.wait("role","Off",60)?;}
  self.case("E23",true,json!({"faults":results,"cancel_stages":4}));Ok(())
 }
 pub(super) fn scenes_tx20(&mut self,g:&win32::Game)->Result<()> {
  let product=self.out.join("product-package");let r=Command::new("pwsh").args(["-NoProfile","-File"]).arg(self.repo.join("scripts/package.ps1")).arg("-OutputDirectory").arg(&product).output().map_err(|e|e.to_string())?;if !r.status.success(){return Err(format!("package: {}",String::from_utf8_lossy(&r.stderr)))}
  let mut files=Vec::new();crate::tx18::product_files(&product,&product,&mut files)?;
  let config=load(&product.join("Repentogon/isaac-cs/config.json"))?;let flavor=load(&product.join("Repentogon/isaac-cs/build-flavor.json"))?;
  if files.iter().any(|p|p.extension().is_some_and(|v|v=="exe")||p.to_string_lossy().contains("isaac-cs-test"))||flavor["e2e_tools"]!=false||flavor["flavor"]!="product"||config["enabled"]!=false||config["world_record"]["enabled"]!=false||config["diagnostics"]["enabled"]!=false||config["control_port"]["enabled"]!=false||!config["world_record"]["e2e_token"].is_null(){return Err("product release boundary".into())}
  let dll=std::fs::read(product.join("Repentogon/zhlIsaacCS.dll")).map_err(|e|e.to_string())?;for s in [b"player_fault_already_armed".as_slice(),b"player_inspect_allocation_or_read".as_slice()]{if dll.windows(s.len()).any(|v|v==s){return Err("E2E Player code leaked into product DLL".into())}}
  self.case("E29.product",true,json!({"files":files,"config":config,"flavor":flavor}));
  win32::set_auto_focus(true);let w=g.wait_window(90)?;let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("port")?as u16,g,90,&self.out.join("control.jsonl"))?;c.focus_window=Some(w);self.prepare_new_run_tx05(g,w,&mut c)?;
  self.report["node_role"]=self.scenario["node_role"].clone();let scenes=self.scenario["scenes"].as_array().ok_or("scenes")?.clone();
  if self.scenario["node_role"]=="Source"{self.source_tx19(g,w,&mut c,&scenes)?;drop(c);if g.close()?!=0{return Err("Source abnormal exit".into())}return Ok(())}
  let source=std::path::PathBuf::from(self.scenario["source_run"].as_str().ok_or("source_run")?);let report=load(&source.join("result.json"))?;
  if report["tx"]!="tx-04-20"||report["node_role"]!="Source"||report["pass"]!=true{return Err("completed tx20 Source required".into())}
  let samples=report["source_samples"].as_array().ok_or("samples")?;if samples.len()!=scenes.len(){return Err("Source matrix count".into())}
  crate::copy_tree(&source.join("runtime-world/client-targets"),&self.out.join("runtime-world/client-targets"))?;self.snapshot("client_before")?;self.report["source_run"]=json!(source);
  let catalog=load(&self.repo.join("out/repentogon/e2e/replication-catalog.json"))?;
  let mut applied=Vec::new();let mut session=Value::Null;let mut types=BTreeSet::new();let mut covered=BTreeSet::new();
  // 同一整体验收只复用明确未受修改影响且正常清理的前缀；冷进程仍实建baseline和末态。
  let mut previous=Vec::<Value>::new();let mut previous_path=None;
  if let Some(path)=self.scenario["client_previous_run"].as_str(){let dir=std::path::PathBuf::from(path);let old=load(&dir.join("result.json"))?;
   if old["tx"]!="tx-04-20"||old["node_role"]!="Client"||!old["evidence_error"].is_null()||old["cleanup_errors"].as_array().is_none_or(|v|v.iter().any(|v|!v.is_null())){return Err("Client reuse requires clean prior cleanup".into())}
   previous=load(&dir.join("client-progress.json"))?.as_array().ok_or("Client prefix samples")?.clone();let count=num(&self.scenario["client_reuse_prefix"])as usize;
   if count==0||count>previous.len()||count>scenes.len(){return Err("explicit unaffected Client prefix size required".into())}previous.truncate(count);
   // 新Source显式复制的旧前缀保留原Scope；只可复用指向该旧Source且全部有来源标记的Client结果。
   if old["source_run"]!=self.report["source_run"]&&(report["source_resume"]["directory"]!=old["source_run"]||num(&report["source_resume"]["reused_prefix"])<(count as u64)||samples.iter().take(count).any(|s|s["reused_from"]!=old["source_run"])){return Err("Client prefix Source provenance differs".into())}
   for(i,v)in previous.iter().enumerate(){if v["label"]!=scenes[i]["label"]||num(&v["status"]["client.player_domain_applied"])!=1||v["native"].is_null(){return Err("Client prefix identity or native result missing".into())}}
   self.report["client_resume"]=json!({"directory":dir,"reused_prefix":count,"cold_rebuild":["baseline","last-prefix-target"]});previous_path=Some(dir);
  }
  for (i,spec) in scenes.iter().enumerate(){let label=spec["label"].as_str().ok_or("label")?;if samples[i]["label"]!=label{return Err("Source scene order".into())}
   let root=self.out.join("runtime-world/client-targets");let t=load(&root.join(format!("{label}.json")))?;source_scene(spec,&t)?;
   let full=load(&root.join(format!("source-full-{label}.json")))?;let audit=projection(&full,&t,&catalog)?;
   for b in bodies(&t)?{types.insert(num(&b["relations"]["player_type"]));}for e in spec["covers"].as_array().ok_or("covers")?{covered.insert(e.as_str().ok_or("coverage")?.to_owned());}
   if i<previous.len()&&i!=0&&i+1!=previous.len(){
    std::fs::copy(previous_path.as_ref().ok_or("Client prefix path")?.join(format!("client-{label}.bmp")),self.out.join(format!("client-{label}.bmp"))).map_err(|e|e.to_string())?;
    let mut value=previous[i].clone();value["reused_from"]=json!(previous_path);applied.push(value);continue;
   }
   let begin=i==0||session!=t["scope"]["session"];if i>0&&begin{c.command("cs stop")?;c.wait("role","Off",60)?;}session=t["scope"]["session"].clone();
   let at=Instant::now();let status=self.apply_tx20(g,&mut c,label,begin)?;let apply_ms=at.elapsed().as_secs_f64()*1000.0;let native=self.inspect_tx20(&mut c,&t,label)?;
   self.wait_tx19(g,&mut c,&format!("draw-{label}"),false,|s|drawn(s)>=drawn(&status)+2)?;
   let counters=c.command("cs counters")?;for name in ["start_game","game_update","room_update","entity_list","odd_helper","restore","post_update","player_update","join_pass"]{if n(&status,&format!("client.original_begin.{name}"))!=n(&counters,&format!("hook.{name}")){return Err(format!("{label} Client gameplay leaked: {name}"))}}
   for _ in 0..if label=="four-jacobs"{20}else{2}{let duplicate=self.apply_tx20(g,&mut c,label,false)?;for k in ["client.native_attempt","client.resource_candidate_loads"]{if n(&duplicate,k)!=n(&status,k){return Err(format!("{label} duplicate reconstructed {k}"))}}if self.inspect_tx20(&mut c,&t,label)?!=native{return Err("duplicate altered native instance".into())}}
   let screenshot=self.out.join(format!("client-{label}.bmp"));win32::screenshot(w,&screenshot)?;let pixels=image_pair(&source.join(format!("source-{label}.bmp")),&screenshot)?;
   applied.push(json!({"label":label,"status":status,"native":native,"projection":audit,"memory":win32::process_memory(g.pid())?,"application_wall_ms_including_IPC":apply_ms,"pixels":pixels,"visual_review":"pending"}));crate::write_json(&self.out.join("client-progress.json"),&json!(applied))?;
  }
  if types!=(0..=40).collect(){return Err(format!("actual PlayerType matrix incomplete: {types:?}"))}
  // Source的真实writer故障后纯值目标交给完整Player owner，不能只借19的窄Player范围。
  if report["cases"]["E26.recorder_failure"]["pass"]!=true{return Err("Source recorder failure proof missing".into())}
  let after_writer=load(&self.out.join("runtime-world/client-targets/source-full-recorder-recovery.json"))?;
  write_target(&self.out.join("runtime-world/client-targets/player-recorder-recovery.json"),&after_writer)?;
  c.command("cs stop")?;c.wait("role","Off",60)?;let recording=self.apply_tx20(g,&mut c,"player-recorder-recovery",true)?;
  let native_recording=self.inspect_tx20(&mut c,&after_writer,"player-recorder-recovery")?;
  self.wait_tx19(g,&mut c,"player-recorder-draw",false,|s|drawn(s)>=drawn(&recording)+2)?;
  for _ in 0..2{let duplicate=self.apply_tx20(g,&mut c,"player-recorder-recovery",false)?;for k in ["client.native_attempt","client.resource_candidate_loads"]{if n(&duplicate,k)!=n(&recording,k){return Err("recorder recovery duplicate reconstructed owner".into())}}}
  if self.inspect_tx20(&mut c,&after_writer,"player-recorder-recovery")?!=native_recording{return Err("recorder recovery duplicate changed native owners".into())}
  let counters=c.command("cs counters")?;for name in ["start_game","game_update","room_update","entity_list","odd_helper","restore","post_update","player_update","join_pass"]{if n(&recording,&format!("client.original_begin.{name}"))!=n(&counters,&format!("hook.{name}")){return Err(format!("recorder recovery Client gameplay leaked {name}"))}}
  self.case("E29.recording",true,json!({"source_writer_failure":report["cases"]["E26.recorder_failure"],"full_player_native":native_recording,"status":recording}));
  let base=load(&self.out.join("runtime-world/client-targets/resources.json"))?;c.command("cs stop")?;c.wait("role","Off",60)?;self.apply_tx20(g,&mut c,"resources",true)?;self.negatives_tx20(g,&mut c,&base)?;
  self.faults_tx20(g,&mut c,"resources")?;let active=self.apply_tx20(g,&mut c,"resources",true)?;let token=self.report["token"].as_str().ok_or("token")?.to_owned();let before=c.command("cs status")?;let isolation=c.command(&format!("cs client_test {token} exercise native"))?;let after=c.command("cs status")?;
  if isolation.get("isolation_exercise").map(String::as_str)!=Some("1")||n(&after,"persist_client_denied_native")<=n(&before,"persist_client_denied_native"){return Err(format!("Player whole-window isolation: {isolation:?}"))}
  self.case("E24",true,json!({"before":before,"after":after,"isolation":isolation,"active":active}));c.command("cs stop")?;c.wait("role","Off",60)?;self.apply_tx20(g,&mut c,"resources",true)?;
  win32::key(w,"CONSOLE")?;let paused=self.wait_tx19(g,&mut c,"Player paused",false,|s|n(s,"client.ui_blocked")==1&&n(s,"client_world_applied")==1)?;self.wait_tx19(g,&mut c,"Player paused draw",false,|s|drawn(s)>=drawn(&paused)+2)?;win32::key(w,"ESC")?;
  c.command("cs stop")?;c.wait("role","Off",60)?;let files=self.files_tx18("client_after")?;self.apply_tx20(g,&mut c,"resources",true)?;
  // 累计CPU测量与IPC墙钟、整个进程内存分列；复用前缀不冒充本进程执行的114次成本。
  self.report["client_timings"]=json!(c.command(&format!("cs client_test {token} fault-status"))?);
  self.report["client_memory"]=json!(win32::process_memory(g.pid())?);
  drop(c);if g.close()?!=0{return Err("Client abnormal exit".into())}let exit=self.files_tx18("client_exit")?;
  self.case("E25",true,json!({"stop":files,"exit":exit}));self.report["applied_samples"]=json!(applied);self.report["actual_player_types"]=json!(types);self.report["declared_matrix_coverage"]=json!(covered);self.report["acceptance_stage"]=json!("automatic_checks_complete_visual_and_full_matrix_review_pending");Ok(())
 }
}
