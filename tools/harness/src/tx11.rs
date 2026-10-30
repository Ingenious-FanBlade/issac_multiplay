//! tx-04-11 全部 Grid 目标共用一次完整场景入口；失败保留续测材料，成功清理。
use crate::{Control,Result,Run,win32};
use serde_json::{Value,json};
use std::{collections::{BTreeMap,BTreeSet},fs,io::{BufRead,BufReader},thread,time::{Duration,Instant}};
type Status=BTreeMap<String,String>;
fn number(s:&Status,k:&str)->u64 { s.get(k).and_then(|v|v.parse().ok()).unwrap_or(0) }
fn n(v:&Value)->Option<f64> { v.as_f64().or_else(||v.as_str().and_then(|v|v.parse().ok())).or_else(||v["value"].as_str().and_then(|v|v.parse().ok())) }
impl Run {
    fn wait_tx11(&mut self,g:&win32::Game,c:&mut Control,label:&str,command:&str,predicate:impl Fn(&Status)->bool)->Result<Status> {
        let until=Instant::now()+Duration::from_secs(45);
        loop {
            self.watch_tx05(g,c,0)?;
            if let Some(error)=self.live_fixture()?.iter().find(|v|v["event"]=="fixture_error") { return Err(format!("tx11 fixture: {error}")); }
            let s=c.command(command)?;
            if predicate(&s) { self.step(label,json!(s))?; return Ok(s); }
            if Instant::now()>=until { return Err(format!("{label}: timeout {s:?}")); }
            thread::sleep(Duration::from_millis(50));
        }
    }
    fn action_tx11(&mut self,g:&win32::Game,c:&mut Control,w:usize,index:u64,name:&str)->Result<()> {
        win32::key(w,"F8")?;
        let until=Instant::now()+Duration::from_secs(30);
        loop {
            self.watch_tx05(g,c,60)?;
            let rows=self.live_fixture()?;
            if let Some(error)=rows.iter().find(|v|v["event"]=="fixture_error") { return Err(format!("fixture: {error}")); }
            if rows.iter().any(|v|v["event"]=="action_return"&&v["action"]==index&&v["name"]==name) { return Ok(()); }
            if Instant::now()>=until { return Err(format!("action {index} {name} timeout")); }
        }
    }
    fn scopes_tx11(&mut self,g:&win32::Game,c:&mut Control,count:u64)->Result<Status> {
        let before=c.command("cs observe status")?;
        self.wait_tx11(g,c,"grid_complete_progress","cs observe status",|s|number(s,"lua_attempts")>=number(&before,"lua_attempts")+count&&number(s,"grid_scope")>number(&before,"grid_scope")&&s.get("grid_coverage").is_some_and(|v|v=="Complete"))
    }
    pub(super) fn scenes_tx11(&mut self,g:&win32::Game)->Result<()> {
        let w=g.wait_window(90)?;
        let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("control_port")? as u16,g,90,&self.out.join("control.jsonl"))?;
        c.focus_window=Some(w); self.prepare_new_run_tx05(g,w,&mut c)?; self.sequence(w,"start_run")?; c.wait("run_active","1",25)?;
        self.action_tx11(g,&mut c,w,1,"seed")?; self.watch_tx05(g,&mut c,1500)?;
        c.command("cs world start grid-full")?;
        self.wait_tx11(g,&mut c,"recorder_start","cs world status",|s|s.get("world_state").is_some_and(|v|v=="Recording"))?;
        c.command("cs observe start")?; c.command("cs solo start")?;
        self.wait_tx11(g,&mut c,"session_active","cs status",|s|s.get("session_state").is_some_and(|v|v=="Active"))?;
        for (i,name) in ["all_grids","appearance","states","destroy","replace_old","replace_new","queries"].iter().enumerate() {
            self.action_tx11(g,&mut c,w,i as u64+2,name)?; self.scopes_tx11(g,&mut c,2)?;
        }
        win32::key(w,"ESC")?; c.wait_pause(None)?; self.watch_tx05(g,&mut c,200)?; win32::key(w,"ESC")?;
        self.watch_tx05(g,&mut c,400)?;
        for (i,name) in ["lifted","leave","revisit","shape","shape_l"].iter().enumerate() {
            self.action_tx11(g,&mut c,w,i as u64+9,name)?; self.watch_tx05(g,&mut c,1000)?; self.scopes_tx11(g,&mut c,2)?;
        }
        c.command("cs world stop")?;
        self.wait_tx11(g,&mut c,"recorder_closed","cs world status",|s|s.get("world_state").is_some_and(|v|v=="Closed"))?;
        let file=self.out.join("runtime-world/grid-full/observation-v2/records.jsonl"); let length=fs::metadata(&file).map_err(|e|e.to_string())?.len();
        let before=c.command("cs observe status")?; let after=self.scopes_tx11(g,&mut c,2)?;
        if before.get("observation_epoch")!=after.get("observation_epoch")||fs::metadata(file).map_err(|e|e.to_string())?.len()!=length {return Err("recording separation failed".into());}
        self.case("grid_recording_off_formal_continues",true,json!({"before":before,"after":after}));
        self.reduce_tx11()?;
        c.command("cs stop")?; self.wait_tx11(g,&mut c,"input_retired","cs status",|s|s.get("session_state").is_some_and(|v|v=="Idle"))?;
        c.command("cs observe stop")?; self.wait_tx11(g,&mut c,"observation_stopped","cs observe status",|s|s.get("observation_state").is_some_and(|v|v=="Off"))?;
        self.return_to_menu(w,&mut c)?;
        let status=c.command("cs status")?;
        for key in ["trace_dropped","query_mismatches","cleanup_failed"] { if number(&status,key)!=0 {return Err(format!("final {key} nonzero"));} }
        self.report["final_status"]=json!(status); drop(c);
        let code=g.close()?; if code!=0 {return Err(format!("game exit {code}"));} Ok(())
    }
    fn reduce_tx11(&mut self)->Result<()> {
        let root=self.out.join("runtime-world/grid-full/observation-v2");
        let summary=crate::observation::read_capture(&root)?;
        println!("grid observation read {summary}"); self.report["observation_read"]=summary.clone();
        if summary["delivery_complete"]!=true||summary["supported_read_complete"]!=true||summary["grid_complete_scopes"].as_u64().unwrap_or(0)<10 {return Err("complete Grid scopes or delivery missing".into());}
        let fixture=self.live_fixture()?;
        let old=fixture.iter().find(|v|v["event"]=="replace_old").ok_or("replace_old")?;
        let new=fixture.iter().find(|v|v["event"]=="replace_new").ok_or("replace_new")?;
        if old["subject"]["local_id"]==new["after"]["subject"]["local_id"]||old["index"]!=new["after"]["index"] {return Err("same-slot replacement retained identity".into());}
        let short=fixture.iter().find(|v|v["event"]=="short_grid").ok_or("short_grid fixture")?;
        let hurt=fixture.iter().find(|v|v["event"]=="poop_hurt_actual").ok_or("poop hurt fixture")?;
        let plate=fixture.iter().find(|v|v["event"]=="plate_contact").ok_or("plate contact fixture")?;
        let mut short_born=false; let mut short_retired=false;
        let mut per_schema=BTreeMap::<String,BTreeMap<String,u64>>::new();
        let mut partial_reasons=BTreeMap::<String,u64>::new();
        let mut classes=BTreeSet::<String>::new(); let mut types=BTreeSet::<u64>::new(); let mut rooms=BTreeSet::<String>::new();
        let mut calls=BTreeMap::<u64,(u64,u64)>::new(); let mut native_bodies=BTreeMap::<u64,(u64,u64)>::new();
        let mut projection=BTreeMap::<String,(Value,Vec<Value>)>::new(); let mut facts=BTreeSet::<String>::new();
        let mut shape=BTreeSet::<u64>::new(); let mut callbacks=BTreeSet::<String>::new(); let mut nodes=0u64;
        for r in &fixture { if r["event"]=="room_actual" { if let Some(s)=r["shape"].as_u64(){shape.insert(s);} } }
        let f=fs::File::open(root.join("records.jsonl")).map_err(|e|e.to_string())?;
        for row in BufReader::new(f).lines() {
            let row:Value=serde_json::from_str(&row.map_err(|e|e.to_string())?).map_err(|e|e.to_string())?;
            let schema=row["schema"].as_str().unwrap_or("");
            *per_schema.entry(schema.into()).or_default().entry(row["coverage"]["status"].as_str().unwrap_or("").into()).or_default()+=1;
            if row["coverage"]["status"]=="Partial"&&schema.starts_with("isaac.cs.grid") {
                if let Some(reasons)=row["payload"]["coverage"]["reasons"].as_object(){for (key,count) in reasons {*partial_reasons.entry(key.clone()).or_default()+=count.as_u64().unwrap_or(1);}}
            }
            if schema=="isaac.cs.grid.lifecycle" {
                if row["payload"]["request"]["seed"]==42001&&row["payload"]["request"]["var_data"]==23&&row["payload"]["returned"]==true {facts.insert("native_spawn_request_return".into());}
                if row["payload"]["local_id"]==short["subject"]["local_id"]&&row["payload"]["observed_birth"]["seed"]==42003&&row["payload"]["observed_birth"]["type"]==2 {facts.insert("short_actual_birth".into());}
            }
            if schema=="isaac.cs.grid.lifecycle"&&row["payload"]["local_id"]==short["subject"]["local_id"] {
                if row["payload"]["kind"]==1 {short_born=true;}
                if row["payload"]["kind"]==4 {short_retired=true;}
            }
            // 短暂受损／触发与引爆阶段允许由真实update独立scope见证，不能要求恰好落在30tick普通采样。
            let o=&row["payload"]["object"];
            if schema=="isaac.cs.grid.lua-event"&&row["coverage"]["status"]=="Complete" {
                let state=n(&o["common"]["fields"]["GridEntity.State"]);
                if o["subject"]["local_id"]==hurt["subject"]["local_id"]&&state==hurt["state"].as_f64(){facts.insert("poop_damaged".into());}
                if o["subject"]["local_id"]==plate["subject"]["local_id"]&&state.is_some_and(|v|v!=plate["state"].as_f64().unwrap_or(0.0)){facts.insert("plate_triggered".into());}
                if o["type"]==2&&state==Some(2.0){facts.insert("rock_destroyed".into());}
                if o["type"]==12&&state.is_some_and(|v|v>=4.0){facts.insert("tnt_destroyed".into());}
            }
            if schema=="isaac.cs.grid.lifecycle"&&(row["payload"]["kind"]==0||row["payload"]["kind"]==8) {
                if let Some(call)=row["payload"]["call"].as_u64().filter(|v|*v>0) {
                    if row["payload"]["exceptional"]==1 {return Err("exceptional native grid call".into());}
                    let map=if row["payload"]["kind"]==8 {&mut native_bodies}else{&mut calls};
                    let pair=map.entry(call).or_default(); if row["payload"]["entering"]==1 {pair.0+=1;}else{pair.1+=1;}
                }
            }
            if schema=="isaac.cs.grid.lua-event"||schema=="isaac.cs.grid.render" {if let Some(name)=row["payload"]["event"].as_str(){callbacks.insert(name.into());}}
            if schema=="isaac.cs.grid.query" { facts.insert("explicit_query".into());
                if row["payload"]["member"]=="EntityEffect.GetGridEntityDesc"&&n(&row["payload"]["result"]["fields"]["GridEntityDesc.SpawnSeed"])==Some(43001.0) {facts.insert("lifted_descriptor".into());}
            }
            if schema=="isaac.cs.grid.legacy" {
                let key=row["payload"]["compatibility"]["capture"].as_str().ok_or("v2 compatibility capture")?.to_string();
                projection.insert(key,(row["context"]["authority_tick"].clone(),row["payload"]["objects"].as_array().ok_or("native objects")?.clone()));
            }
            if schema!="isaac.cs.grid"||row["coverage"]["status"]!="Complete" {continue;}
            if let Some(room)=row["context"]["room_instance"]["local_id"].as_str(){rooms.insert(room.into());}
            nodes=nodes.max(row["coverage"]["nodes"].as_u64().unwrap_or(0));
            if let Some(objects)=row["payload"]["objects"].as_array(){for o in objects {
                if let Some(t)=o["type"].as_u64(){types.insert(t);}
                if let Some(bodies)=o["bodies"].as_object(){for class in bodies.keys(){classes.insert(class.clone());}}
                let fields=&o["common"]["fields"]; let desc=&fields["GridEntity.GetSaveState"]["fields"];
                if n(&desc["GridEntityDesc.Type"])!=o["type"].as_f64()||n(&desc["GridEntityDesc.State"])!=n(&fields["GridEntity.State"]) {return Err("descriptor/runtime state mismatch".into());}
                let sprite=&fields["GridEntity.GetSprite"]["fields"];
                if n(&sprite["Sprite.Rotation"])==Some(31.0)&&sprite["Sprite.FlipX"]==true {facts.insert("sprite".into());}
                if n(&fields["GridEntity.GetWaterClipFlags"])==Some(57.0){facts.insert("water_clip".into());}
                if fields["GridEntity.GetRNG"]["type"]=="random_state" {facts.insert("rng_state".into());}
                if o["bodies"]["GridEntityPit"]["fields"]["GridEntityPit.HasLadder"]==true {facts.insert("bridge".into());}
                if o["bodies"]["GridEntityDoor"]["fields"]["GridEntityDoor.IsLocked"]==true {facts.insert("door_locked".into());}
                if o["bodies"]["GridEntityDoor"]["fields"]["GridEntityDoor.IsOpen"]==true {facts.insert("door_open".into());}
                if o["subject"]["local_id"]==old["subject"]["local_id"] {facts.insert("replacement_old_sample".into());}
                if o["subject"]["local_id"]==new["after"]["subject"]["local_id"] {facts.insert("replacement_new_sample".into());}
            }}
        }
        let expected=["Decoration","Door","Fire","Gravity","Lock","Pit","Poop","PressurePlate","Rock","Spikes","Stairs","Statue","Teleporter","TNT","TrapDoor","Wall","Web"].map(|s|format!("GridEntity{s}")).into_iter().collect::<BTreeSet<_>>();
        if !expected.is_subset(&classes)||(1..=27).any(|t|!types.contains(&t)){return Err(format!("missing Grid classes/types: {classes:?} {types:?}"));}
        if calls.is_empty()||calls!=native_bodies||calls.values().any(|p|*p!=(1,1)){return Err("native Grid original call pairing failed".into());}
        for name in ["explicit_query","lifted_descriptor","sprite","water_clip","rng_state","bridge","door_locked","door_open","replacement_old_sample","replacement_new_sample","native_spawn_request_return","short_actual_birth","poop_damaged","plate_triggered","rock_destroyed","tnt_destroyed"] {if !facts.contains(name){return Err(format!("Grid state not observed: {name}; facts={facts:?}"));}}
        if rooms.len()<3||!shape.contains(&1)||!shape.contains(&8)||!shape.contains(&9){return Err(format!("transition/revisit/shape coverage incomplete: rooms={rooms:?} shapes={shape:?}"));}
        if !short_born||!short_retired {return Err("short Grid creation/physical retirement missing".into());}
        if !callbacks.iter().any(|s|s.contains("SPAWN"))||!callbacks.iter().any(|s|s.contains("DESTROY"))||!callbacks.iter().any(|s|s.ends_with("RENDER")){return Err(format!("Grid callbacks incomplete: {callbacks:?}"));}
        // v1与扩展native投影必须来自同一次读取；不重调游戏Getter作比较。
        let f=fs::File::open(root.parent().ok_or("capture root")?.join("captures.jsonl")).map_err(|e|e.to_string())?;
        let mut pairs=0u64;
        for line in BufReader::new(f).lines() {
            let legacy:Value=serde_json::from_str(&line.map_err(|e|e.to_string())?).map_err(|e|e.to_string())?;
            if legacy["record"]!="capture" {continue;}
            let Some((tick,objects))=projection.remove(legacy["capture_seq"].as_str().unwrap_or(""))else{continue;};
            if tick!=legacy["authority_tick"] {return Err("Grid compatibility tick mismatch".into());}
            for object in objects {
                let old=legacy["objects"].as_array().ok_or("v1 objects")?.iter().find(|v|v["local_id"]==object["local_id"]).ok_or("v1 subject missing")?;
                if old["use_generation"]!=object["use"]||old["revision"]!=object["revision"] {return Err("Grid compatibility identity drift".into());}
                for f in object["fields"].as_array().ok_or("v2 fields")? {
                    let id=format!("{:04x}",f["id"].as_u64().ok_or("field id")?);
                    let v=old["fields"].as_array().ok_or("v1 fields")?.iter().find(|v|v["field_id"]==id).ok_or("v1 field missing")?;
                    let status=f["status"].as_u64().ok_or("field status")? as usize;
                    if ["VALUE","UNCONFIRMED","NOT_APPLICABLE","NOT_READY","READ_FAILED"].get(status).copied()!=v["status"].as_str(){return Err("Grid field validity drift".into());}
                    if status==0 {
                        let raw=v["bytes_le"].as_str().map(str::to_string).or_else(||v["raw32"].as_str().or_else(||v["raw64"].as_str()).map(|s|s.as_bytes().chunks_exact(2).rev().map(|c|String::from_utf8_lossy(c).into_owned()).collect::<String>()));
                        if raw.as_deref()!=f["bytes_le"].as_str(){return Err("Grid compatibility raw value drift".into());}
                    }
                }
                pairs+=1;
            }
        }
        if pairs==0||!projection.is_empty(){return Err("Grid same-read projection incomplete".into());}
        self.case("grid_v1_same_read_compatibility",true,json!({"object_pairs":pairs}));
        self.case("grid_all_types_and_derived",true,json!({"types":types,"classes":classes,"complete":summary["grid_complete_scopes"],"max_nodes":nodes}));
        self.case("grid_values_resources_and_queries",true,json!({"facts":facts}));
        self.case("grid_lifecycle_original_and_rooms",true,json!({"native_calls":calls.len(),"short_born":short_born,"short_retired":short_retired,"callbacks":callbacks,"rooms":rooms,"shapes":shape}));
        println!("grid_acceptance {}",self.report["cases"]);
        println!("grid_scope_coverage {}",json!({"by_schema":per_schema,"partial_grid_reasons":partial_reasons}));
        Ok(())
    }
}
