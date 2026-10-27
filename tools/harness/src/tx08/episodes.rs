//! C 项 episode 索引：保留动作、真实 watermark、native 序号和对象前后成员状态。
//! 缺少原生事件或只能按时间窗关联时明确写出，不伪造短命/复用/Replace 覆盖。
use super::*;
use crate::world::dataset::{self, Dataset};
use std::collections::BTreeSet;

fn dec(v: &Value) -> Option<u64> { v.as_str()?.parse().ok() }
fn hex(v: &Value) -> Option<u64> { u64::from_str_radix(v.as_str()?, 16).ok() }

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
        let find_mark = |side: &str| -> Result<&Value> {
            let label = format!("p1.action{step}.{side}");
            let rows = marks.iter().filter(|r|r["kind"]=="checkpoint" && r["label"]==label).collect::<Vec<_>>();
            if rows.len()!=1 { return Err(format!("episode_watermark:{label}")); }
            Ok(rows[0])
        };
        let record = (|| -> Result<Value> {
            let begin = find_mark("begin")?; let end = find_mark("end")?;
            let low = dec(&begin["event_watermark"]).ok_or("episode low watermark")?;
            let high = dec(&end["event_watermark"]).ok_or("episode high watermark")?;
            if high < low { return Err("episode_reverse_watermark".into()); }
            let lua = fixture.iter().filter(|r|r["step"]==step).cloned().collect::<Vec<_>>();
            let terminals = lua.iter().filter(|r|r["event"]=="action" && r["phase"]=="arrived").collect::<Vec<_>>();
            let native = events.iter().filter(|r|dec(&r["sequence"]).is_some_and(|s|s>low && s<=high)).collect::<Vec<_>>();
            // 先匹配公开动作的原始实参，再沿真实 parent 关系扩展；时间窗中无关调用单列。
            let mut calls = BTreeSet::new();
            for event in &native {
                let args = event["diagnostics"]["args"].as_array();
                let matched = match spec["name"].as_str().unwrap_or("") {
                    "spawn"|"spawn_short_lived" => event["observation"]=="Spawn" && args.is_some_and(|a|a.len()==8
                        && hex(&a[0])==spec["type"].as_u64() && hex(&a[1])==spec["variant"].as_u64()
                        && hex(&a[5])==spec["subtype"].as_u64() && hex(&a[6])==spec["seed"].as_u64()),
                    "grid_spawn"|"grid_replace" => event["observation"]=="GridSpawn" && args.is_some_and(|a|a.len()==5
                        && hex(&a[0])==spec["index"].as_u64() && hex(&a[1])==spec["type"].as_u64()
                        && hex(&a[3])==spec["seed"].as_u64()),
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
                    "grid_remove" => event["observation"]=="GridFree" && args.is_some_and(|a|!a.is_empty() && hex(&a[0])==spec["index"].as_u64()),
                    "player_esau" => event["observation"]=="Replace",
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
                    if !e["local_id"].is_null() { identities.insert((e["local_id"].clone().to_string(),e["use_generation"].clone().to_string())); }
                    if e["event_kind"]==0 && e["entering"]==false && e["diagnostics"]["result"]!="00000000" {
                        if let Some(a)=e["diagnostics"]["result"].as_str() { addresses.insert(a.to_string()); }
                    }
                }
            }
            if let Some(label)=spec["id"].as_str() {
                if matches!(spec["name"].as_str(),Some("spawn"|"spawn_short_lived")) {
                    labels.insert(label.to_string(),identities.clone());
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
            let prior=scan.attempts.range(..=low_attempt).next_back().map(|(&a,_)|a).unwrap_or(low_attempt);
            for (&attempt, location) in scan.attempts.range(prior..=high_attempt) {
                let capture=data.at(&mut input,location)?;
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
            let has=|kind: u64, observation: &str| related.iter().any(|e|e["event_kind"]==kind && e["observation"]==observation
                && e["exceptional"]==false && (kind!=0 && kind!=8 || e["entering"]==false));
            let short=spec["name"]=="spawn_short_lived";
            let short_proof=short && has(1,"Spawn") && has(0,"Remove") && has(8,"Remove")
                && has(0,"ClearReferences") && has(8,"ClearReferences") && has(0,"Destroy") && has(8,"Destroy");
            let mut publications=BTreeMap::<String,u64>::new();
            for event in &related {
                if event["event_kind"]==0 && event["observation"]=="Publish" && event["entering"]==false
                    && event["exceptional"]==false && !event["local_id"].is_null() {
                    *publications.entry(format!("{}:{}",event["local_id"],event["use_generation"])).or_default()+=1;
                }
            }
            Ok(json!({"step":step,"spec":spec,"action_id":terminals.first().map(|v|v["action_id"].clone()),
                "status":if terminals.len()==1{"INDEXED"}else{"NOT_OBSERVED"},"begin":begin,"end":end,
                "lua":lua,"native_events":native,"parameter_matched_calls":parameter_calls,
                "parent_or_identity_linked_calls":calls,"publication_return_counts":publications,"snapshots":snapshots,
                "related_event_sequences":related.iter().map(|e|e["sequence"].clone()).collect::<Vec<_>>(),
                "coverage":{"owner_birth":has(1,"Spawn")||has(1,"GridSpawn"),"remove":has(0,"Remove"),
                    "clear_references":has(0,"ClearReferences"),"destroy":has(0,"Destroy"),
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
    let value=json!({"episodes":result,"continue_native_events":restore,"reuse":"REQUIRES_ACTUAL_OBSERVATION",
        "comparison_basis":"same recording and watermark, actual args/call/parent/local/use/birth/revision; Lua stage preserved"});
    write_json(&out.join("evidence/episodes.json"), &value)?;
    Ok(value)
}
