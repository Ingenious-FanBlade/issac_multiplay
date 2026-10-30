//! V6专项：逐份原始Player副本核对依据、当前绑定更新见证及撤销，不改变世界完整标志。
use super::{evidence, trace_index::TraceIndex, *};
use crate::tx05::evidence::parse_events;
use crate::world::{dataset::{self, Dataset}, model::Capture};
use std::{collections::{BTreeMap, BTreeSet}, fs::File, io::BufWriter};

type Identity = (String, String, String);
fn identity(v: &Value) -> Option<Identity> {
    Some((v["run_epoch"].as_str()?.into(), v["local_id"].as_str()?.into(),
        v["use_generation"].as_str()?.into()))
}
fn dec(v: &Value) -> Option<u64> { v.as_str()?.parse().ok() }

// 其他Entity缺字段可使世界不完整；Player资格仍须来自同一完整普通复制窗口。
// 来源/root变化、污染窗口和无Authority的gap均不能成为资格成功样本。
fn ordinary_window(c: &Capture) -> bool {
    c.record == "capture" && c.authority_kind == 1 && c.authority_tick.is_some()
        && c.diagnostics["rng_window"]["set_stable"] == true
        && c.diagnostics["rng_window"]["thread_before"] == c.diagnostics["thread"]
        && c.diagnostics["rng_window"]["thread_after"] == c.diagnostics["thread"]
        && c.diagnostics["phase_witness"].as_array().is_some_and(|phases|
            phases.len()==3 && phases.iter().enumerate().all(|(i,p)|p["stage"]==i+1
                && p["entered"]==1 && p["returned"]==1 && p["scope"]==c.scope_token))
        && !matches!(c.reason.as_str(), "LIFECYCLE" | "ROOT_CHANGED" | "SOURCE_CHANGED")
}

pub(super) fn evaluate(out: &Path) -> Result<Value> {
    mkdir(&out.join("evidence"))?;
    let path = out.join("evidence/v6-player-readiness-index.jsonl");
    let mut index = BufWriter::new(File::create(&path).map_err(|e| e.to_string())?);
    let mut violations = vec![];
    let mut per_recording = json!({});
    let mut first_observed = false;
    let mut witnesses = 0u64;
    let mut blocked = 0u64;
    let mut revoked = BTreeSet::new();
    let mut rebuilt = BTreeSet::new();
    let mut revoked_ordinals = BTreeSet::new();
    let mut rebuilt_ordinals = BTreeSet::new();
    for label in ["primary", "comparison", "healthy", "continue"] {
        let data = Dataset::open(&out.join("runtime-world").join(label), false)?;
        let scan = dataset::scan(&data, None, None, None)?;
        let process = if label == "comparison" { 2 } else { 1 };
        let trace = parse_events(&out.join(format!("process-{process}/trace.jsonl")))?;
        let traces = TraceIndex::new(&trace);
        let events = evidence::rows(&data.root.join("lifecycle-events.jsonl"))?;
        let mut cursor = 0;
        let mut current = BTreeMap::<Identity, Value>::new();
        let mut births = BTreeMap::<Identity, String>::new();
        let mut revoked_here = BTreeSet::new();
        let mut ordinals = BTreeSet::new();
        let mut copies = 0u64;
        let mut incomplete_copies = 0u64;
        let mut input = File::open(data.root.join("captures.jsonl")).map_err(|e| e.to_string())?;
        if scan.event_gaps { violations.push(json!({"recording":label,"reason":"LIFECYCLE_EVENT_GAP"})); }
        for (&attempt, location) in &scan.attempts {
            let c = data.at(&mut input, location)?;
            let watermark = c.event_watermark.as_deref().and_then(|v|v.parse::<u64>().ok()).unwrap_or(0);
            while cursor < events.len() && dec(&events[cursor]["sequence"]).is_some_and(|s|s<=watermark) {
                let e = &events[cursor]; cursor += 1;
                let Some(key) = identity(e) else { continue; };
                if e["event_kind"] == 1 {
                    if let Some(b) = e["birth_event"].as_str() { births.insert(key.clone(), b.into()); }
                }
                if e["event_kind"] == 6 {
                    let tag = format!("{label}:{key:?}");
                    if e["reason"] == "NONE" {
                        if revoked_here.contains(&key) {
                            rebuilt.insert(tag);
                            if let Some(n)=e["diagnostics"]["ordinal"].as_u64().filter(|n|*n<2) { rebuilt_ordinals.insert(n); }
                        }
                        current.insert(key.clone(), e.clone());
                    } else {
                        revoked_here.insert(key.clone()); revoked.insert(tag); current.remove(&key);
                        if let Some(n)=e["diagnostics"]["ordinal"].as_u64().filter(|n|*n<2) { revoked_ordinals.insert(n); }
                    }
                }
                if e["event_kind"] == 3 || (e["event_kind"] == 0 && e["entering"] == true
                    && matches!(e["observation"].as_str(),Some("Init"|"PlayerRestore"|"Replace"|"ManagerRestore"|"Destroy"))) {
                    current.remove(&key);
                }
            }
            if c.record != "capture" { continue; }
            let phase = traces.phase_counts(c.authority_tick.as_deref().and_then(|v|v.parse().ok()),
                dec(&c.diagnostics["loop"]),c.scope_token.parse().ok());
            let window = ordinary_window(&c) && phase.pre==1 && phase.commit==1 && phase.standard_publish==1;
            for o in c.objects.iter().filter(|o|o.class=="Player") {
                copies += 1;
                if !c.complete { incomplete_copies += 1; }
                let key = (c.run_epoch.clone(),o.local_id.clone(),o.use_generation.clone());
                let w = current.get(&key);
                let native = w.is_some_and(|w| {
                    w["run_epoch"]==c.run_epoch && w["room_epoch"]==c.room_epoch
                        && w["diagnostics"]["mutation"]==c.mutation_epoch
                        && w["diagnostics"]["address"]==o.diagnostics["address"]
                        && w["diagnostics"]["ordinal"].as_u64()==o.ordinal.map(u64::from)
                        && o.ordinal.is_some()
                        && dec(&w["diagnostics"]["session"]).is_some_and(|n|n>0)
                        && w["diagnostics"]["device"]!="00000000"
                        && w["diagnostics"]["assignment_key"].as_i64().is_some_and(|n|n>=0)
                        && w["diagnostics"]["thread"]==c.diagnostics["thread"]
                        && traces.native_player(o.ordinal.map(u64::from),dec(&w["diagnostics"]["candidate_tick"]),
                            dec(&w["diagnostics"]["session"]),dec(&w["diagnostics"]["participant_generation"]))
                });
                let copied = o.fields.iter().any(|f|f.status=="VALUE"
                    && (f.field_id=="000a" || ("0101"..="0110").contains(&f.field_id.as_str())));
                let public_byte = o.fields.iter().any(|f|f.field_id=="000a" && f.status=="VALUE");
                let owner = o.init_evidence==2 && o.birth_event.as_ref().is_some_and(|b|births.get(&key)==Some(b));
                let reason = if o.origin=="FIRST_OBSERVED" && o.birth_event.is_some() {
                    Some("FIRST_OBSERVED_WITH_FABRICATED_BIRTH")
                } else if window && copied && o.init_evidence==3 && !native {
                    Some("PLAYER_BASIS_WITHOUT_CURRENT_NATIVE_WITNESS")
                } else if window && copied && o.init_evidence==2 && !owner {
                    Some("PLAYER_CREATION_BASIS_WITHOUT_OWNER_BIRTH")
                } else if copied && o.init_evidence<2 {
                    Some("PLAYER_FIELDS_COPIED_WITHOUT_READINESS")
                } else { None };
                if let Some(reason)=reason { violations.push(json!({"recording":label,"attempt":attempt.to_string(),
                    "local":o.local_id,"ordinal":o.ordinal,"reason":reason})); }
                if !copied && o.init_evidence<2 { blocked += 1; }
                let qualified = window && native && public_byte && reason.is_none();
                if qualified {
                    witnesses += 1;
                    if let Some(ordinal)=o.ordinal { ordinals.insert(ordinal); }
                    first_observed |= o.init_evidence==3 && o.origin=="FIRST_OBSERVED" && o.birth_event.is_none();
                }
                writeln!(index,"{}",json!({"recording":label,"attempt":attempt.to_string(),"local":o.local_id,
                    "use":o.use_generation,"ordinal":o.ordinal,"init_evidence":o.init_evidence,
                    "origin":o.origin,"birth":o.birth_event,"world_complete":c.complete,"world_reason":c.reason,
                    "ordinary_copy_window":window,"owner_birth":owner,"witness_sequence":w.map(|w|&w["sequence"]),
                    "native":native,"public_byte_value":public_byte,"qualified":qualified,"violation":reason}))
                    .map_err(|e|e.to_string())?;
            }
        }
        per_recording[label] = json!({"ordinals":ordinals,"player_copies":copies,
            "from_incomplete_world":incomplete_copies,"event_gaps":scan.event_gaps});
    }
    index.flush().map_err(|e|e.to_string())?;
    let covered = ["primary","comparison","healthy","continue"].iter().all(|label|
        per_recording[*label]["ordinals"].as_array().is_some_and(|s|s.contains(&json!(0)) && s.contains(&json!(1))));
    let both_rebuilt = [0,1].iter().all(|n|revoked_ordinals.contains(n) && rebuilt_ordinals.contains(n));
    let pass = covered && first_observed && witnesses>0 && blocked>0 && both_rebuilt && violations.is_empty();
    Ok(json!({"status":if pass{"PASS"}else if witnesses>0{"FAIL"}else{"NOT_OBSERVED"},"pass":pass,
        "detail":{"witnesses":witnesses,"per_recording":per_recording,"first_observed_without_birth":first_observed,
            "unqualified_player_copies_blocked":blocked,"revocations":revoked,"rebuilds":rebuilt,"violations":violations,
            "revoked_ordinals":revoked_ordinals,"rebuilt_ordinals":rebuilt_ordinals,
            "index":"evidence/v6-player-readiness-index.jsonl",
            "scope":"Player readiness only; incomplete worlds retain their original incomplete status"}}))
}
