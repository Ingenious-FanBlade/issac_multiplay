//! tx-04-10 唯一整体验收：正式来源、域迁移、四人多身体、录制故障隔离及兼容投影。
use crate::{Control,Result,Run,win32,remote::{Remote,Step,protocol::State}};
use serde_json::{Value,json};
use std::{collections::{BTreeMap,BTreeSet},fs,io::{BufRead,BufReader},path::Path,thread,time::{Duration,Instant}};
type Status=BTreeMap<String,String>;
fn number(s:&Status,key:&str)->u64 { s.get(key).and_then(|v|v.parse().ok()).unwrap_or(0) }
fn scalar_number(v:&Value)->Option<f64> { v.as_f64().or_else(||v.as_str().and_then(|s|s.parse().ok())).or_else(||v["value"].as_str().and_then(|s|s.parse().ok())) }
fn each_row(path:&Path,mut consume:impl FnMut(Value)->Result<()>)->Result<()> {
    let file=fs::File::open(path).map_err(|e|e.to_string())?;
    for line in BufReader::new(file).lines() {
        let line=line.map_err(|e|e.to_string())?;
        if line.len()>20*1024*1024 { return Err("capture line exceeds bounded reader".into()); }
        consume(serde_json::from_str(&line).map_err(|e|e.to_string())?)?;
    }
    Ok(())
}
fn little_hex(v:&Value)->Option<String> {
    if let Some(s)=v["bytes_le"].as_str() { return Some(s.to_string()); }
    let s=v["raw32"].as_str().or_else(||v["raw64"].as_str())?;
    if s.len()%2!=0 { return None; }
    Some(s.as_bytes().chunks_exact(2).rev().map(|c|String::from_utf8_lossy(c).into_owned()).collect::<String>())
}
impl Run {
    // D20 已采纳的字段范围直接作用于本次快照，不依赖已清理的历史 artifact。
    pub(crate) fn save_fields_tx10(&self,changes:&mut Value)->Result<()> {
        for change in changes.as_array_mut().ok_or("save changes")? {
            let name=change["path"].as_str().ok_or("save change path")?.to_string();
            if change["allowed"]==true||!crate::saves::other_slot(&name) { continue; }
            if Path::new(&name).components().any(|c|!matches!(c,std::path::Component::Normal(_))) { return Err("unsafe save snapshot path".into()); }
            let a=fs::read(self.out.join("save_before").join(&name));
            let b=fs::read(self.out.join("save_after").join(&name));
            if let (Ok(a),Ok(b))=(a,b) {
                if a.len()>=8&&a.len()==b.len()&&a[..a.len()-8]==b[..b.len()-8] {
                    change["allowed"]=json!(true);
                    change["rule"]=json!("D20: actual before/after bytes differ only in trailing Counter/Checksum u32 fields; no checksum computed or validated");
                }
            }
        }
        Ok(())
    }
    fn wait_tx10(&mut self,g:&win32::Game,c:&mut Control,label:&str,command:&str,predicate:impl Fn(&Status)->bool)->Result<Status> {
        let end=Instant::now()+Duration::from_secs(30);
        loop {
            self.watch_tx05(g,c,0)?;
            if self.live_fixture()?.iter().any(|r|r["event"]=="fixture_error") { return Err(format!("tx10 fixture error during {label}")); }
            let s=c.command(command)?;
            if predicate(&s) { self.step(label,json!(s))?; return Ok(s); }
            if Instant::now()>=end { return Err(format!("{label}: timeout; {s:?}")); }
            thread::sleep(Duration::from_millis(60));
        }
    }
    fn action_tx10(&mut self,g:&win32::Game,c:&mut Control,w:usize,index:u64,name:&str)->Result<()> {
        win32::key(w,"F8")?;
        let end=Instant::now()+Duration::from_secs(30);
        loop {
            self.watch_tx05(g,c,80)?;
            let records=self.live_fixture()?;
            if let Some(error)=records.iter().find(|v|v["event"]=="fixture_error") { return Err(format!("fixture: {error}")); }
            if records.iter().any(|v|v["event"]=="action_return"&&v["action"]==index&&v["name"]==name) { return Ok(()); }
            if Instant::now()>=end { return Err(format!("fixture action {index} {name} timeout")); }
        }
    }
    fn captures_tx10(&mut self,g:&win32::Game,c:&mut Control,count:u64)->Result<Status> {
        let initial=c.command("cs observe status")?;
        self.wait_tx10(g,c,"entity_scope_progress","cs observe status",|s|number(s,"lua_attempts")>=number(&initial,"lua_attempts")+count)
    }
    pub(super) fn scenes_tx10(&mut self,g:&win32::Game)->Result<()> {
        let w=g.wait_window(90)?;
        let mut c=Control::connect(self.scenario["control_port"].as_u64().ok_or("control_port")? as u16,g,90,&self.out.join("control.jsonl"))?;
        c.focus_window=Some(w);
        self.prepare_new_run_tx05(g,w,&mut c)?;
        self.sequence(w,"start_run")?; c.wait("run_active","1",25)?;
        self.action_tx10(g,&mut c,w,1,"seed")?;
        self.watch_tx05(g,&mut c,1500)?;
        c.command("cs world start entity-full")?;
        self.wait_tx10(g,&mut c,"recorder_start","cs world status",|s|s.get("world_state").is_some_and(|v|v=="Recording"))?;
        c.command("cs observe start")?;
        c.command(&format!("cs host {} 0",self.scenario["host_port"]))?;
        self.wait_tx10(g,&mut c,"host_active","cs status",|s|s.get("session_state").is_some_and(|v|v=="Active"))?;
        let mut remotes=Vec::new();
        for ordinal in 1..=3 {
            let remote=Remote::start_participant(format!("127.0.0.1:{}",self.scenario["host_port"]).parse().map_err(|e|format!("{e}"))?,&self.out.join(format!("remote-{ordinal}.jsonl")),3000,2,ordinal,|_|Ok(()))?;
            self.wait_tx10(g,&mut c,"participant_join","cs status",|s|s.get(&format!("participant.{ordinal}.bound")).is_some_and(|v|v=="1"))?;
            remotes.push(remote);
        }
        self.action_tx10(g,&mut c,w,2,"all_classes")?; self.captures_tx10(g,&mut c,3)?;
        self.action_tx10(g,&mut c,w,3,"appearance_and_morph")?; self.captures_tx10(g,&mut c,2)?;
        self.action_tx10(g,&mut c,w,4,"short_lived")?; self.captures_tx10(g,&mut c,1)?;
        self.action_tx10(g,&mut c,w,5,"slot_interaction")?;
        let end=Instant::now()+Duration::from_secs(20);
        loop {
            self.watch_tx05(g,&mut c,100)?;
            if self.live_fixture()?.iter().any(|r|r["event"]=="slot_observation"&&r["variant"]==4&&(r["donation"].as_i64().unwrap_or(0)>0||r["touch"].as_i64().unwrap_or(0)>0)) { break; }
            if Instant::now()>=end { return Err("actual slot contact not observed".into()); }
        }
        self.captures_tx10(g,&mut c,2)?;
        self.action_tx10(g,&mut c,w,6,"stop_interaction")?;
        self.action_tx10(g,&mut c,w,7,"grid_create")?; self.captures_tx10(g,&mut c,1)?;
        self.action_tx10(g,&mut c,w,8,"grid_replace")?; self.captures_tx10(g,&mut c,1)?;
        let before=c.command("cs observe status")?;
        c.command("cs world stop")?;
        self.wait_tx10(g,&mut c,"recorder_closed","cs world status",|s|s.get("world_state").is_some_and(|v|v=="Closed"))?;
        let length=fs::metadata(self.out.join("runtime-world/entity-full/observation-v2/records.jsonl")).map_err(|e|e.to_string())?.len();
        let after=self.captures_tx10(g,&mut c,2)?;
        if before.get("observation_epoch")!=after.get("observation_epoch")||number(&after,"entity_scope")<=number(&before,"entity_scope") { return Err("recorder stop reset or stopped formal capture".into()); }
        if fs::metadata(self.out.join("runtime-world/entity-full/observation-v2/records.jsonl")).map_err(|e|e.to_string())?.len()!=length { return Err("closed recorder still writes".into()); }
        self.case("recording_off_formal_continues",true,json!({"before":before,"after":after}));
        c.command("cs world start entity-file-failure")?;
        let recording=self.wait_tx10(g,&mut c,"recording_restart","cs world status",|s|s.get("world_state").is_some_and(|v|v=="Recording"))?;
        self.captures_tx10(g,&mut c,1)?;
        c.command(&format!("cs world_test {} {} file 1",self.report["token"].as_str().ok_or("token")?,number(&recording,"generation")))?;
        self.wait_tx10(g,&mut c,"file_fault_closed","cs world status",|s|s.get("world_state").is_some_and(|v|v=="Closed")&&s.get("reason").is_some_and(|v|v=="FILE_ERROR"))?;
        let alive=self.captures_tx10(g,&mut c,2)?;
        if before.get("observation_epoch")!=alive.get("observation_epoch") { return Err("file error changed capture epoch".into()); }
        self.case("file_error_isolation",true,json!(alive));
        c.command("cs world start entity-recovered")?;
        self.wait_tx10(g,&mut c,"recording_recovered","cs world status",|s|s.get("world_state").is_some_and(|v|v=="Recording"))?;
        self.action_tx10(g,&mut c,w,9,"clear")?;
        self.action_tx10(g,&mut c,w,10,"four_twins")?;
        let twins=self.wait_tx10(g,&mut c,"four_participants_eight_bodies","cs status",|s|number(s,"participants")==4&&number(s,"active_bodies")>=8&&s.get("blocks").is_some_and(|v|v=="0"))?;
        let mut ids=BTreeSet::new();
        for ordinal in 0..4 { for body in 0..number(&twins,&format!("participant.{ordinal}.bodies")) {
            let id=number(&twins,&format!("participant.{ordinal}.body.{body}.entity_id")); if id==0||!ids.insert(id) { return Err("multi-body identity missing or duplicated".into()); }
        }}
        for (i,remote) in remotes.iter().enumerate() {
            remote.script("tx10-input",vec![Step{after:0,state:State{movement:if i==0 {[32767,0]}else{[0,-32767]},aim:[0,0],held:0,edge:0}},Step{after:6,state:State::default()}])?;
        }
        self.watch_tx05(g,&mut c,800)?; self.captures_tx10(g,&mut c,2)?;
        self.action_tx10(g,&mut c,w,11,"explicit_queries")?;
        win32::key(w,"ESC")?; c.wait_pause(None)?; self.watch_tx05(g,&mut c,200)?; win32::key(w,"ESC")?;
        self.watch_tx05(g,&mut c,400)?;
        self.action_tx10(g,&mut c,w,12,"room")?; self.captures_tx10(g,&mut c,2)?;
        self.case("four_participant_multi_body_and_transition",true,json!({"entity_ids":ids}));
        c.command("cs world stop")?;
        self.wait_tx10(g,&mut c,"recovered_closed","cs world status",|s|s.get("world_state").is_some_and(|v|v=="Closed"))?;
        self.reduce_tx10()?;
        for remote in &remotes { remote.bye()?; }
        c.command("cs stop")?;
        self.wait_tx10(g,&mut c,"input_retired","cs status",|s|s.get("session_state").is_some_and(|v|v=="Idle"))?;
        c.command("cs observe stop")?;
        self.wait_tx10(g,&mut c,"observation_stopped","cs observe status",|s|s.get("observation_state").is_some_and(|v|v=="Off"))?;
        self.return_to_menu(w,&mut c)?;
        let status=c.command("cs status")?;
        for key in ["trace_dropped","query_mismatches","cleanup_failed"] { if number(&status,key)!=0 { return Err(format!("final {key} nonzero")); } }
        self.report["final_status"]=json!(status);
        drop(remotes); drop(c);
        let code=g.close()?; if code!=0 { return Err(format!("game exit {code}")); } Ok(())
    }
    fn reduce_tx10(&mut self)->Result<()> {
        let root=self.out.join("runtime-world/entity-full");
        let summary=crate::observation::read_capture(&root.join("observation-v2"))?;
        let recovered=crate::observation::read_capture(&self.out.join("runtime-world/entity-recovered/observation-v2"))?;
        self.report["observation_read"]=json!({"initial":summary,"recovered":recovered});
        println!("observation_read {}",self.report["observation_read"]);
        if summary["delivery_complete"]!=true||summary["entity_complete_scopes"].as_u64().unwrap_or(0)==0||recovered["delivery_complete"]!=true||recovered["entity_complete_scopes"].as_u64().unwrap_or(0)==0 { return Err("v2 complete scopes or delivery missing".into()); }
        // 按行归约，只保留需要与v1配对的稀疏native样本；不把整段录制加载到32位进程。
        let mut classes=BTreeSet::<String>::new(); let mut pairs=0u64; let mut calls=BTreeMap::<u64,(u32,u32)>::new();
        let mut native=BTreeMap::<String,(Value,Vec<Value>)>::new();
        let fixture=self.live_fixture()?;
        let short=fixture.iter().find(|r|r["event"]=="short_lived").map(|r|r["object"]["subject"]["local_id"].clone()).ok_or("short-lived request")?;
        let mut short_events=BTreeSet::<String>::new(); let mut loot_calls=BTreeSet::<String>::new();
        let morphs=fixture.iter().filter(|r|r["event"]=="npc_morph"||r["event"]=="pickup_morph").collect::<Vec<_>>();
        if morphs.len()!=2 {return Err("both Morph fixture facts required".into());}
        for m in &morphs {
            if m["before"]["subject"]["local_id"]!=m["after"]["subject"]["local_id"]||m["before"]["subject"]["use"]!=m["after"]["subject"]["use"]||scalar_number(&m["after"]["subject"]["revision"]).unwrap_or(0.0)<=scalar_number(&m["before"]["subject"]["revision"]).unwrap_or(0.0) {return Err("Morph did not retain identity and advance revision".into());}
        }
        let mut morph_seen=BTreeSet::new(); let mut appearance=false; let mut slot_value=false;
        let grid_index=fixture.iter().find(|r|r["event"]=="grid_created").and_then(|r|r["index"].as_u64()).ok_or("grid fixture index")?;
        let mut grid_ids=BTreeSet::new();
        each_row(&root.join("observation-v2/records.jsonl"),|row| {
            let schema=row["schema"].as_str().unwrap_or("");
            if schema=="isaac.cs.entity.actual-call" { if let Some(member)=row["payload"]["member"].as_str() { loot_calls.insert(member.into()); } }
            if schema=="isaac.cs.entity"&&row["coverage"]["status"]=="Complete" {
                if let Some(objects)=row["payload"]["objects"].as_array() { for object in objects {
                    if let Some(class)=object["class"].as_str() { classes.insert(class.into()); }
                    if object["special"]["type"]=="EntityDelirium" { classes.insert("EntityDelirium".into()); }
                    for m in &morphs {
                        if object["subject"]["local_id"]==m["after"]["subject"]["local_id"]&&object["subject"]["revision"]==m["after"]["subject"]["revision"]&&scalar_number(&object["common"]["fields"]["Entity.Type"])==m["after"]["type"].as_f64()&&scalar_number(&object["common"]["fields"]["Entity.Variant"])==m["after"]["variant"].as_f64() {morph_seen.insert(m["event"].as_str().unwrap_or(""));}
                    }
                    let sprite=&object["common"]["fields"]["Entity.GetSprite"]["fields"];
                    if sprite["Sprite.FlipX"]==true&&scalar_number(&sprite["Sprite.Rotation"])==Some(37.0)&&scalar_number(&sprite["Sprite.Scale"]["x"]).is_some_and(|v|(v-1.15).abs()<0.001) {appearance=true;}
                    let fields=&object["body"]["fields"];
                    if object["class"]=="EntitySlot"&&scalar_number(&object["common"]["fields"]["Entity.Variant"])==Some(4.0)&&(scalar_number(&fields["EntitySlot.GetDonationValue"]).unwrap_or(0.0)>0.0||scalar_number(&fields["EntitySlot.GetTouch"]).unwrap_or(0.0)>0.0) {slot_value=true;}
                }}
            }
            if schema=="isaac.cs.entity.lua-event"&&row["payload"]["subject"]["local_id"]==short {
                if let Some(event)=row["payload"]["event"].as_str() {short_events.insert(event.into());}
            }
            if schema.ends_with(".lifecycle")&&row["payload"]["kind"]==0 {
                let call=row["payload"]["call"].as_u64().unwrap_or(0); if call>0 {let pair=calls.entry(call).or_default();if row["payload"]["entering"]==1 {pair.0+=1;}else{pair.1+=1;}}
            }
            if schema=="isaac.cs.entity.native-v1"||schema=="isaac.cs.grid.legacy" {
                if schema=="isaac.cs.grid.legacy" { if let Some(objects)=row["payload"]["objects"].as_array() {for o in objects {
                    if o["memberships"].as_array().is_some_and(|ms|ms.iter().any(|m|m["source"]==11&&m["index"].as_u64()==Some(grid_index))) {grid_ids.insert((o["local_id"].as_str().unwrap_or("").to_string(),o["use"].as_str().unwrap_or("").to_string()));}
                }}}
                let key=row["payload"]["compatibility"]["capture"].as_str().ok_or("v2 compatibility capture")?.to_string();
                let entry=native.entry(key).or_insert_with(||(row["context"]["authority_tick"].clone(),Vec::new()));
                entry.1.extend(row["payload"]["objects"].as_array().ok_or("native objects")?.iter().cloned());
            }
            Ok(())
        })?;
        each_row(&root.join("captures.jsonl"),|legacy| {
            if legacy["record"]!="capture" {return Ok(());}
            let Some((tick,objects))=native.remove(legacy["capture_seq"].as_str().unwrap_or("")) else{return Ok(());};
            if tick!=legacy["authority_tick"] {return Err("compatibility tick mismatch".into());}
            for object in objects {
                let matching=legacy["objects"].as_array().ok_or("v1 objects")?.iter().find(|o|o["local_id"]==object["local_id"]).ok_or("compatibility subject missing")?;
                if matching["use_generation"]!=object["use"]||matching["revision"]!=object["revision"] {return Err("v1/v2 identity drift".into());}
                for f in object["fields"].as_array().ok_or("v2 fields")? {
                    let id=f["id"].as_u64().ok_or("v2 field id")?;
                    let expected_id=format!("{id:04x}");
                    let old=matching["fields"].as_array().ok_or("v1 fields")?.iter().find(|v|v["field_id"]==expected_id).ok_or("v1 projection field missing")?;
                    let status=f["status"].as_u64().ok_or("v2 status")? as usize;
                    let names=["VALUE","UNCONFIRMED","NOT_APPLICABLE","NOT_READY","READ_FAILED"];
                    if names.get(status).copied()!=old["status"].as_str() {return Err("v1/v2 field validity drift".into());}
                    if f["encoding"]!=2&&status==0&&little_hex(old).as_deref()!=f["bytes_le"].as_str() {return Err("v1/v2 raw field drift".into());}
                }
                pairs+=1;
            }
            Ok(())
        })?;
        if !native.is_empty() {return Err("v2 native sample lacks same-read v1 projection".into());}
        if !short_events.contains("MC_POST_TEAR_INIT")||!short_events.contains("MC_POST_ENTITY_REMOVE") {return Err(format!("short-lived lifecycle not captured: {short_events:?}"));}
        for member in ["EntityPickup.GetLootList","EntityNPC.GetFireplaceLoot","EntityNPC.GetShopkeeperLoot"] {
            if !loot_calls.contains(member) { return Err(format!("actual returned loot missing: {member}; observed={loot_calls:?}")); }
        }
        let expected=["EntityPlayer","EntityNPC","EntityTear","EntityProjectile","EntityBomb","EntityLaser","EntityKnife","EntityPickup","EntityFamiliar","EntityEffect","EntitySlot","EntityDelirium","NativeText"];
        let missing=expected.iter().filter(|c|!classes.contains(**c)).collect::<Vec<_>>();
        if !missing.is_empty() { return Err(format!("classes missing from complete Entity scopes: {missing:?}; read-result retained")); }
        if morph_seen.len()!=2||!appearance||!slot_value||grid_ids.len()<2 {return Err(format!("semantic capture missing: morph={morph_seen:?}, appearance={appearance}, slot={slot_value}, grid={grid_ids:?}"));}
        let mut eight_body_scope=false;
        each_row(&self.out.join("runtime-world/entity-recovered/observation-v2/records.jsonl"),|row| {
            if row["schema"]=="isaac.cs.entity"&&row["coverage"]["status"]=="Complete" {
                let mut groups=BTreeMap::<u64,BTreeSet<String>>::new();
                if let Some(objects)=row["payload"]["objects"].as_array() {for o in objects {if o["class"]=="EntityPlayer"&&o["subject"]["space"]==1 {
                    if let (Some(ordinal),Some(id))=(o["subject"]["control_relation"]["ordinal"].as_u64(),o["subject"]["local_id"].as_str()) {groups.entry(ordinal).or_default().insert(id.into());}
                }}}
                if groups.len()==4&&groups.values().all(|g|g.len()>=2) {eight_body_scope=true;}
            }
            Ok(())
        })?;
        if !eight_body_scope {return Err("no complete Entity scope contains all four control groups and eight Entity bodies".into());}
        if pairs==0||calls.is_empty()||calls.values().any(|p|*p!=(1,1)) { return Err(format!("compatibility or original-once pairing failed: pairs={pairs}, calls={}",calls.len())); }
        if !fixture.iter().any(|r|r["event"]=="short_lived"&&r["exists_after"]==false)||!fixture.iter().any(|r|r["event"]=="readonly_rng"&&r["before"]==r["after"]) { return Err("short-life or RNG observation missing".into()); }
        self.case("entity_field_semantics_and_eight_body_scope",true,json!({"morph":morph_seen,"sprite_transform":appearance,"slot_actual_value":slot_value,"same_grid_slot_identities":grid_ids,"complete_eight_body_scope":eight_body_scope}));
        self.case("full_entity_compatibility_lifecycle",true,json!({"classes":classes,"same_read_object_pairs":pairs,"paired_calls":calls.len()}));
        println!("entity_acceptance {}",self.report["cases"]); Ok(())
    }
}
