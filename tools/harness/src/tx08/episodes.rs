//! C 项 episode 索引：保留动作、真实 watermark、native 序号和对象前后成员状态。
//! 缺少原生事件或只能按时间窗关联时明确写出，不伪造短命/复用/Replace 覆盖。
use super::*;
use crate::world::dataset::{self, Dataset};
use std::collections::BTreeSet;

fn dec(v: &Value) -> Option<u64> { v.as_str()?.parse().ok() }
fn hex(v: &Value) -> Option<u64> { u64::from_str_radix(v.as_str()?, 16).ok() }

// 删除后重建单列两次 owner：旧身份的真实退休必须先于同格新出生，不声称原生直接覆盖。
fn grid_recreation(spec: &Value, related: &[&Value], terminals: &[&Value],
    prior: &BTreeSet<(String, String)>) -> Value {
    if spec["name"] != "grid_replace" { return Value::Null; }
    let normal = |e: &Value, kind: u64, observation: &str| e["event_kind"]==kind
        && e["observation"]==observation && e["entering"]==false && e["exceptional"]==false;
    let body = |owner: &Value, observation: &str| related.iter().any(|e|
        e["call"]==owner["call"] && normal(e,8,observation)
        && e["diagnostics"]["slot_before"]==owner["diagnostics"]["slot_before"]
        && e["diagnostics"]["slot_after"]==owner["diagnostics"]["slot_after"]
        && (observation!="GridSpawn" || e["diagnostics"]["result"]=="00000001"));
    let mut pairs=vec![];
    for free in related.iter().filter(|e|normal(e,0,"GridFree") && body(e,"GridFree")
        && hex(&e["diagnostics"]["slot_before"]).is_some_and(|a|a!=0)
        && e["diagnostics"]["slot_after"]=="00000000") {
        let retired=related.iter().filter(|e|e["event_kind"]==4 && e["call"]==free["call"]
            && prior.contains(&(e["local_id"].to_string(),e["use_generation"].to_string())))
            .map(|e|e["sequence"].clone()).collect::<Vec<_>>();
        for spawn in related.iter().filter(|e|normal(e,0,"GridSpawn") && body(e,"GridSpawn")
            && e["diagnostics"]["result"]=="00000001"
            && e["diagnostics"]["slot_before"]=="00000000"
            && hex(&e["diagnostics"]["slot_after"]).is_some_and(|a|a!=0)) {
            let entered=related.iter().any(|e|e["call"]==spawn["call"] && e["event_kind"]==0
                && e["entering"]==true && dec(&e["sequence"]).zip(dec(&free["sequence"]))
                    .is_some_and(|(s,f)|s>f));
            let births=related.iter().filter(|e|e["event_kind"]==1 && e["observation"]=="GridSpawn"
                && e["call"]==spawn["call"] && !e["local_id"].is_null()
                && !prior.contains(&(e["local_id"].to_string(),e["use_generation"].to_string())))
                .map(|e|e["sequence"].clone()).collect::<Vec<_>>();
            pairs.push(json!({"free_call":free["call"],"spawn_call":spawn["call"],
                "retirement_sequences":retired,"birth_sequences":births,
                "ordered":entered,"pass":entered && retired.len()==1 && births.len()==1}));
        }
    }
    let lua=terminals.len()==1 && spec["replacement_mode"]=="remove_then_spawn"
        && terminals[0]["detail"]["route"]=="remove_then_spawn"
        && terminals[0]["detail"]["removed"]["present"]==false
        && terminals[0]["detail"]["after"]["present"]==true
        && terminals[0]["detail"]["after"]["type"]==spec["type"]
        && terminals[0]["detail"]["after"]["seed"]==spec["seed"]
        && terminals[0]["detail"]["after"]["var_data"]==spec["var_data"]
        && terminals[0]["detail"]["raw_return"]["type"]=="boolean"
        && terminals[0]["detail"]["raw_return"]["value"]=="true"
        && (spec["type"]!=14 || terminals[0]["detail"]["poop_accessor"]==true);
    // 请求实参仍精确关联原生调用；实际 variant 仅记录，不参与 Grid 后态通过条件。
    json!({"route":"remove_then_spawn","direct_native_overwrite_claimed":false,"lua_after_matches":lua,
        "variant_policy":"OBSERVE_ONLY","requested_variant":spec["variant"],
        "observed_variant":terminals.first().map(|t|t["detail"]["after"]["variant"].clone()),
        "pairs":pairs,"pass":lua && pairs.len()==1 && pairs[0]["pass"]==true})
}

pub(super) fn reduce(out: &Path, actions: &Value, fixture: &[Value]) -> Result<Value> {
    let root = out.join("runtime-world/primary");
    let events = super::evidence::rows(&root.join("lifecycle-events.jsonl"))?;
    let marks = super::evidence::rows(&root.join("diagnostics.jsonl"))?;
    let data = Dataset::open(&root, false)?;
    let scan = dataset::scan(&data, None, None, None)?;
    let mut input = fs::File::open(root.join("captures.jsonl")).map_err(|e|e.to_string())?;
    let mut result = vec![];
    let mut labels = BTreeMap::<String, BTreeSet<(String, String)>>::new();
    let active = actions.as_array().ok_or("episode actions")?.iter().filter(|a|a["enabled"]!=false);
    for (index, spec) in active.enumerate() {
        if spec["phase"] == "setup" { continue; }
        let step = index + 1;
        let record = (|| -> Result<Value> {
        let post=spec["phase"]=="post_continue";
        let post_root=out.join("runtime-world/continue");
        let post_data=if post {Some(Dataset::open(&post_root,false)?)}else{None};
        let post_scan=post_data.as_ref().map(|d|dataset::scan(d,None,None,None)).transpose()?;
        let mut post_input=if post {Some(fs::File::open(post_root.join("captures.jsonl")).map_err(|e|e.to_string())?)}else{None};
        let post_events=if post {Some(super::evidence::rows(&post_root.join("lifecycle-events.jsonl"))?)}else{None};
        let post_marks=if post {Some(super::evidence::rows(&post_root.join("diagnostics.jsonl"))?)}else{None};
        let episode_data=post_data.as_ref().unwrap_or(&data);
        let episode_scan=post_scan.as_ref().unwrap_or(&scan);
        let episode_events=post_events.as_ref().unwrap_or(&events);
        let episode_marks=post_marks.as_ref().unwrap_or(&marks);
        let episode_input=post_input.as_mut().unwrap_or(&mut input);
        let find_mark = |side: &str| -> Result<&Value> {
            let label = format!("p1.action{step}.{side}");
            let rows = episode_marks.iter().filter(|r|r["kind"]=="checkpoint" && r["label"]==label).collect::<Vec<_>>();
            if rows.len()!=1 { return Err(format!("episode_watermark:{label}")); }
            Ok(rows[0])
        };
            let begin = find_mark("begin")?; let end = find_mark("end")?;
            let low = dec(&begin["event_watermark"]).ok_or("episode low watermark")?;
            let high = dec(&end["event_watermark"]).ok_or("episode high watermark")?;
            if high < low { return Err("episode_reverse_watermark".into()); }
            let lua = fixture.iter().filter(|r|r["step"]==step).cloned().collect::<Vec<_>>();
            let terminals = lua.iter().filter(|r|r["event"]=="action" && r["phase"]=="arrived").collect::<Vec<_>>();
            let native = episode_events.iter().filter(|r|dec(&r["sequence"]).is_some_and(|s|s>low && s<=high)).collect::<Vec<_>>();
            // 先匹配公开动作的原始实参，再沿真实 parent 关系扩展；时间窗中无关调用单列。
            let mut calls = BTreeSet::new();
            let spawn_grid = |event: &Value| event["observation"]=="GridSpawn"
                && event["diagnostics"]["args"].as_array().is_some_and(|a|a.len()==5
                    && hex(&a[0])==spec["index"].as_u64() && hex(&a[1])==spec["type"].as_u64()
                    && hex(&a[2])==spec["variant"].as_u64() && hex(&a[3])==spec["seed"].as_u64()
                    && hex(&a[4])==spec["var_data"].as_i64().map(|v|v as i32 as u32 as u64));
            let free_grid = |event: &Value| event["observation"]=="GridFree"
                && event["diagnostics"]["args"].as_array().is_some_and(|a|a.len()==3
                    && hex(&a[0])==spec["index"].as_u64()
                    && hex(&a[1])==spec["path_trail"].as_i64().map(|v|v as i32 as u32 as u64)
                    && hex(&a[2])==spec["keep_decoration"].as_bool().map(u64::from));
            for event in &native {
                let args = event["diagnostics"]["args"].as_array();
                let matched = match spec["name"].as_str().unwrap_or("") {
                    "spawn"|"spawn_short_lived"|"pool_cycle" => event["observation"]=="Spawn" && args.is_some_and(|a|a.len()==8
                        && hex(&a[0])==spec["type"].as_u64() && hex(&a[1])==spec["variant"].as_u64()
                        && hex(&a[5])==spec["subtype"].as_u64()
                        && if spec["name"]=="pool_cycle" {
                            hex(&a[6]).zip(spec["seed"].as_u64()).is_some_and(|(seed,base)|
                                seed>=base && seed-base<spec["count"].as_u64().unwrap_or(0))
                        } else {hex(&a[6])==spec["seed"].as_u64()}),
                    "grid_spawn" => spawn_grid(event),
                    "grid_replace" => spawn_grid(event) || free_grid(event),
                    "morph_npc"|"morph_pickup" => {
                        let observation=if spec["name"]=="morph_npc"{"NpcMorph"}else{"PickupMorph"};
                        event["observation"]==observation && args.is_some_and(|a|a.len()>=4
                            && hex(&a[0])==spec["type"].as_u64() && hex(&a[1])==spec["variant"].as_u64()
                            && hex(&a[2])==spec["subtype"].as_u64())
                            && spec["id"].as_str().is_some_and(|label|labels.get(label).is_some_and(|ids|
                                ids.contains(&(event["local_id"].to_string(),event["use_generation"].to_string()))))
                    },
                    "remove" => event["observation"]=="Remove" && spec["id"].as_str().is_some_and(|label|
                        labels.get(label).is_some_and(|ids|ids.contains(&(event["local_id"].to_string(),event["use_generation"].to_string())))),
                    "grid_remove" => free_grid(event),
                    "player_esau" => event["observation"]=="Replace" && args.is_some_and(|a|a.len()==2
                        && hex(&a[0]).is_some_and(|v|v!=0) && hex(&a[1]).is_some_and(|v|v!=0) && a[0]!=a[1]),
                    _ => false,
                };
                if matched { if let Some(call)=dec(&event["call"]) { calls.insert(call); } }
            }
            let parameter_calls=calls.clone();
            // parent 必须来自原始文件；固定点只遍历有限的本 episode 调用集合。
            loop {
                let old = calls.len();
                for event in &native {
                    if dec(&event["parent"]).is_some_and(|p|calls.contains(&p)) {
                        if let Some(call)=dec(&event["call"]) { calls.insert(call); }
                    }
                }
                if old==calls.len() { break; }
            }
            let mut identities = BTreeSet::new();
            let mut addresses = BTreeSet::new();
            for e in &native {
                if dec(&e["call"]).is_some_and(|c|calls.contains(&c)) {
                    // Replace 两侧均由原始参数关联；旧对象退出 active roster 不等于销毁。
                    if e["observation"]=="Replace" {
                        if let Some(args)=e["diagnostics"]["args"].as_array() {
                            for a in args.iter().take(2) {if let Some(a)=a.as_str() {addresses.insert(a.to_string());}}
                        }
                    }
                    if !e["local_id"].is_null() { identities.insert((e["local_id"].clone().to_string(),e["use_generation"].clone().to_string())); }
                    if e["event_kind"]==0 && e["entering"]==false && e["diagnostics"]["result"]!="00000000" {
                        if let Some(a)=e["diagnostics"]["result"].as_str() { addresses.insert(a.to_string()); }
                    }
                }
            }
            let mut prior_grid_ids=BTreeSet::new();
            if let Some(label)=spec["id"].as_str() {
                if matches!(spec["name"].as_str(),Some("spawn"|"spawn_short_lived")) {
                    labels.insert(label.to_string(),identities.clone());
                } else if matches!(spec["name"].as_str(),Some("grid_spawn"|"grid_replace")) {
                    if let Some(old)=labels.get(label) {
                        prior_grid_ids=old.clone(); identities.extend(old.iter().cloned());
                    }
                    // Grid 标签只继承本次 owner 的新出生，旧身份另存，避免后续 Remove 误配。
                    let born=native.iter().filter(|e|e["event_kind"]==1 && e["observation"]=="GridSpawn"
                        && !e["local_id"].is_null() && dec(&e["call"]).is_some_and(|c|calls.contains(&c)))
                        .map(|e|(e["local_id"].to_string(),e["use_generation"].to_string())).collect();
                    labels.insert(label.to_string(),born);
                } else if let Some(old)=labels.get(label) { identities.extend(old.iter().cloned()); }
            }
            // Remove/Clear/Destroy 在 Spawn 返回后可能是新的根调用；以已关联身份接回同 call 的 body。
            for event in &native {
                if identities.contains(&(event["local_id"].to_string(),event["use_generation"].to_string())) {
                    if let Some(call)=dec(&event["call"]) { calls.insert(call); }
                }
            }
            loop {
                let old=calls.len();
                for event in &native {
                    if dec(&event["parent"]).is_some_and(|p|calls.contains(&p)) {
                        if let Some(call)=dec(&event["call"]) { calls.insert(call); }
                    }
                }
                if old==calls.len() { break; }
            }
            let low_attempt=dec(&begin["attempt"]).ok_or("episode begin attempt")?;
            let high_attempt=dec(&end["attempt"]).ok_or("episode end attempt")?;
            let mut snapshots=vec![];
            // 包含前一份真实 capture，不能把未出现对象当已退休。
            // checkpoint 前最近一条可能是 odd gap；必须回到最近真实副本，不能把 gap 当作前态缺失。
            let mut prior=low_attempt;
            for (&attempt,location) in episode_scan.attempts.range(..=low_attempt).rev() {
                if episode_data.at(episode_input,location)?.record=="capture" {prior=attempt;break;}
            }
            for (&attempt, location) in episode_scan.attempts.range(prior..=high_attempt) {
                let capture=episode_data.at(episode_input,location)?;
                for o in &capture.objects {
                    let linked=identities.contains(&(json!(o.local_id).to_string(),json!(o.use_generation).to_string()))
                        || o.diagnostics["address"].as_str().is_some_and(|a|addresses.contains(a));
                    // 生命周期场景同时保存 A/B，健康、basis 与 generation 不混为一项。
                    if linked || o.class=="Player" {
                        snapshots.push(json!({"attempt":attempt.to_string(),"capture":capture.capture_seq,"tick":capture.authority_tick,
                            "run":capture.run_epoch,"room":capture.room_epoch,"mutation":capture.mutation_epoch,
                            "object":o,"link":if linked{"native_identity_or_return_address"}else{"player_lifecycle_context"}}));
                    }
                }
            }
            let related=native.iter().filter(|e|dec(&e["call"]).is_some_and(|c|calls.contains(&c))
                || identities.contains(&(e["local_id"].to_string(),e["use_generation"].to_string()))).copied().collect::<Vec<_>>();
            // 公开动作、成功原生返回和 active A 的两侧地址必须共同成立，不能只凭角色类型变化。
            let replacements=related.iter().filter(|e|e["observation"]=="Replace" && e["event_kind"]==0
                && e["entering"]==false && e["exceptional"]==false && e["diagnostics"]["result"]=="00000001")
                .filter_map(|e| {
                    let args=e["diagnostics"]["args"].as_array()?;
                    if args.len()!=2 {return None;}
                    let before=snapshots.iter().rev().find(|s|dec(&s["attempt"]).is_some_and(|a|a<=low_attempt)
                        && s["object"]["ordinal"]==0 && s["object"]["diagnostics"]["address"]==args[0]);
                    let after=snapshots.iter().find(|s|dec(&s["attempt"]).is_some_and(|a|a>low_attempt)
                        && s["object"]["ordinal"]==0 && s["object"]["diagnostics"]["address"]==args[1]);
                    let retained=snapshots.iter().find(|s|dec(&s["attempt"]).is_some_and(|a|a>low_attempt)
                        && s["object"]["diagnostics"]["address"]==args[0] && s["object"]["ordinal"].is_null()
                        && s["object"]["memberships"].as_array().is_some_and(|m|m.iter().any(|m|m["source"]==9)));
                    let body=related.iter().any(|b|b["call"]==e["call"] && b["event_kind"]==8
                        && b["entering"]==false && b["exceptional"]==false && b["diagnostics"]["result"]=="00000001");
                    Some(json!({"call":e["call"],"old_address":args[0],"replacement_address":args[1],"native_body_success":body,
                        "before":before,"after":after,"retained_old":retained,
                        "pass":body && before.is_some() && after.is_some() && retained.is_some()}))
                }).collect::<Vec<_>>();
            let item_input=terminals.len()==1 && lua.iter().any(|r|r["event"]=="world-fixture" && r["phase"]=="input_prepared"
                && r["action_id"]==terminals[0]["action_id"] && r["item"]==703 && r["slot"]==0 && r["charge"]==1
                && r["item_state"]==0 && r["input_key"]=="SPACE" && r["input_count"]==1)
                && (if spec["expect_new_player"]==false {terminals[0]["detail"]["roster_address_changed"]==true}
                    else {terminals[0]["detail"]["new_player_init_callbacks"].as_u64().is_some_and(|n|n>0)});
            let has=|kind: u64, observation: &str| related.iter().any(|e|e["event_kind"]==kind && e["observation"]==observation
                && e["exceptional"]==false && (kind!=0 && kind!=8 || e["entering"]==false));
            let short=spec["name"]=="spawn_short_lived";
            // 池对象 Remove/ClearReferences 不经过物理析构；短命性与 Destroy 必须分别判定。
            let cleared=related.iter().any(|e|e["event_kind"]==0 && e["observation"]=="ClearReferences"
                && e["entering"]==false && e["exceptional"]==false
                && related.iter().any(|r|r["event_kind"]==0 && r["observation"]=="Remove"
                    && r["entering"]==false && r["exceptional"]==false && r["local_id"]==e["local_id"]
                    && r["use_generation"]==e["use_generation"]
                    && dec(&r["sequence"]).zip(dec(&e["sequence"])).is_some_and(|(r,e)|r<e)));
            let removed=cleared && has(8,"Remove") && has(8,"ClearReferences")
                && terminals.len()==1 && terminals[0]["detail"]["after"]["exists"]==false;
            let short_proof=short && has(1,"Spawn") && removed;
            let mut publications=BTreeMap::<String,u64>::new();
            for event in &related {
                if event["event_kind"]==0 && event["observation"]=="Publish" && event["entering"]==false
                    && event["exceptional"]==false && !event["local_id"].is_null() {
                    *publications.entry(format!("{}:{}",event["local_id"],event["use_generation"])).or_default()+=1;
                }
            }
            Ok(json!({"step":step,"spec":spec,"recording":if post{"continue"}else{"primary"},"action_id":terminals.first().map(|v|v["action_id"].clone()),
                "status":if terminals.len()==1{"INDEXED"}else{"NOT_OBSERVED"},"begin":begin,"end":end,
                "lua":lua,"native_events":native,"parameter_matched_calls":parameter_calls,
                "parent_or_identity_linked_calls":calls,"publication_return_counts":publications,"snapshots":snapshots,
                "player_replace":{"input_prepared":item_input,"pairs":replacements,
                    "pass":item_input && replacements.len()==1 && replacements[0]["pass"]==true},
                "grid_recreation":grid_recreation(spec,&related,&terminals,&prior_grid_ids),
                "related_event_sequences":related.iter().map(|e|e["sequence"].clone()).collect::<Vec<_>>(),
                "coverage":{"owner_birth":has(1,"Spawn")||has(1,"GridSpawn"),"remove":has(0,"Remove"),
                    "clear_references":has(0,"ClearReferences"),"destroy":has(0,"Destroy"),
                    "logical_removal_and_clear":removed,
                    "short_lived_chain":short_proof,"grid_retirement":related.iter().any(|e|e["event_kind"]==4),
                    "use_gt_one_observed":related.iter().any(|e|dec(&e["use_generation"]).is_some_and(|u|u>1))},
                "limitations":["time window alone does not prove causal ownership","absence from capture is not physical retirement",
                    "reuse requires actual factory/use evidence; seed/index/address coincidence is insufficient"]}))
        })();
        result.push(record.unwrap_or_else(|e|json!({"step":step,"spec":spec,"status":"NOT_OBSERVED","error":e})));
    }
    let continuation = out.join("runtime-world/continue/lifecycle-events.jsonl");
    let restore = if continuation.exists() { super::evidence::rows(&continuation)?.into_iter().filter(|e|
        e["observation"]=="ManagerRestore" || e["observation"]=="PlayerRestore" || e["event_kind"]==6 || e["event_kind"]==2).collect::<Vec<_>>() } else {vec![]};
    // 跨房间的重复发布/池复用可能发生在无自有Lua标签的对象上；保留全录制真实身份链。
    let mut publications=BTreeMap::<String,Vec<Value>>::new(); let mut reuse=vec![];
    let mut destructions=vec![];
    // 先索引同 call 的正常 native body 返回，避免每次 Factory 再扫描全事件。
    let native_returns = events.iter().filter(|e|e["event_kind"]==8
        && e["entering"]==false && e["exceptional"]==false)
        .map(|e|e["call"].to_string()).collect::<BTreeSet<_>>();
    for e in &events {
        if e["event_kind"]==0 && e["entering"]==false && e["exceptional"]==false && !e["local_id"].is_null() {
            let key=format!("{}:{}:{}",e["run_epoch"],e["local_id"],e["use_generation"]);
            if e["observation"]=="Publish" {publications.entry(key).or_default().push(e.clone());}
            if e["observation"]=="Factory" && dec(&e["use_generation"]).is_some_and(|u|u>1)
                && native_returns.contains(&e["call"].to_string()) {
                reuse.push(e.clone());
            }
            if e["observation"]=="Destroy" && native_returns.contains(&e["call"].to_string())
                && e["diagnostics"]["lifecycle"].as_u64().is_some_and(|n|n>=4) {
                destructions.push(e.clone());
            }
        }
    }
    // Esau Jr 第二次正常使用会返回已有 backup；重复 Publish 只能在同一录制/use 内计数。
    // Continue 的原生发布也必须纳入，不能只扫描 primary 而漏掉返回旧 Player 的真实路径。
    for label in ["comparison","healthy","continue"] {
        let path=out.join("runtime-world").join(label).join("lifecycle-events.jsonl");
        if !path.exists() {continue;}
        let rows=super::evidence::rows(&path)?;
        let bodies=rows.iter().filter(|e|e["event_kind"]==8 && e["entering"]==false && e["exceptional"]==false)
            .map(|e|e["call"].to_string()).collect::<BTreeSet<_>>();
        for e in rows.iter().filter(|e|e["event_kind"]==0 && e["entering"]==false
            && e["exceptional"]==false && !e["local_id"].is_null() && bodies.contains(&e["call"].to_string())) {
            if e["observation"]=="Publish" {
                publications.entry(format!("{label}:{}:{}:{}",e["run_epoch"],e["local_id"],e["use_generation"]))
                    .or_default().push(e.clone());
            }
        }
    }
    let value=json!({"episodes":result,"continue_native_events":restore,"reuse":"REQUIRES_ACTUAL_OBSERVATION",
        "publication_events_by_identity":publications,"actual_factory_reuse_events":reuse,
        "physical_destruction_events":destructions,
        "comparison_basis":"same recording and watermark, actual args/call/parent/local/use/birth/revision; Lua stage preserved"});
    write_json(&out.join("evidence/episodes.json"), &value)?;
    Ok(value)
}

// V5 专项仍读取原始文件与实际回调；原生四个边界必须逐 call 配对，不能以 Lua arrived 替代。
pub(super) fn evaluate(out: &Path, actions: &Value, token: &str) -> Result<(Value,Value)> {
    mkdir(&out.join("evidence"))?;
    let fixture=crate::tx05::fixture_records(&read(&out.join("process-1/log.txt"))?,&format!("{token}-p1"))?;
    if fixture.is_empty() {return Err("missing_primary_fixture_records".into());}
    let proof=reduce(out,actions,&fixture)?;
    let mut result=super::obligations::episodes(&json!({"episodes":{"proof":proof}}));
    let mut calls_proof=json!({});
    let mut errors=vec![];
    for label in ["primary","comparison","healthy","continue"] {
        let data=Dataset::open(&out.join("runtime-world").join(label),false)?;
        let scan=dataset::scan(&data,None,None,None)?;
        let rows=super::evidence::rows(&data.root.join("lifecycle-events.jsonl"))?;
        let mut calls=BTreeMap::<String,[u64;4]>::new();
        let mut stack=vec![];
        for e in &rows {
            let kind=e["event_kind"].as_u64().unwrap_or(u64::MAX);
            if !matches!(kind,0|8) {continue;}
            let call=e["call"].as_str().ok_or("native call id")?.to_owned();
            let entering=e["entering"]==true;
            let slot=if kind==0 {0}else{2}+usize::from(!entering);
            calls.entry(call.clone()).or_insert([0;4])[slot]+=1;
            if e["exceptional"]==true {errors.push(json!({"recording":label,"call":call,"reason":"EXCEPTIONAL_NATIVE_CALL"}));}
            let scope=(call.clone(),kind);
            if entering {stack.push(scope);} else if stack.pop()!=Some(scope) {
                errors.push(json!({"recording":label,"call":call,"reason":"NATIVE_SCOPE_ORDER"}));
            }
        }
        let mismatches=calls.iter().filter(|(_,n)|**n!=[1;4]).map(|(c,n)|json!({"call":c,"counts":n})).collect::<Vec<_>>();
        let pass=data.healthy() && !scan.event_gaps && stack.is_empty() && mismatches.is_empty();
        if !pass {errors.push(json!({"recording":label,"reason":"NATIVE_RECORDING_INCOMPLETE"}));}
        calls_proof[label]=json!({"pass":pass,"call_groups":calls.len(),"mismatches":mismatches,
            "event_gaps":scan.event_gaps,"open_scopes":stack,"healthy":data.healthy()});
    }
    let pass=result["pass"]==true && errors.is_empty();
    result["pass"]=json!(pass);result["status"]=json!(if pass{"PASS"}else{"FAIL"});
    result["detail"]["native_calls"]=calls_proof;result["detail"]["native_errors"]=json!(errors);
    Ok((result,proof))
}
