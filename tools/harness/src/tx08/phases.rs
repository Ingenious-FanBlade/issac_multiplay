//! V2：全量核对完整阶段与真实提交；不把字段缺失、奇数帧或暂停窗口升格为完整世界。
use super::{evidence, trace_index::TraceIndex, *};
use crate::{tx05::evidence::parse_events, world::dataset::{self, Dataset}};
use std::{collections::BTreeSet, fs::File, io::BufWriter};

fn dec(v: &Value) -> Option<u64> { v.as_str()?.parse().ok() }

// 每个录制独立核对，不使用另一段的成功替代当前段缺失。
pub(super) fn evaluate(out: &Path) -> Result<Value> {
    mkdir(&out.join("evidence"))?;
    let mut index = BufWriter::new(File::create(out.join("evidence/v2-phase-index.jsonl")).map_err(|e|e.to_string())?);
    let mut recordings = json!({});
    let mut violations = vec![];
    let (mut odd, mut no_authority, mut lifecycle) = (0u64,0u64,0u64);
    let mut pause = json!({"pass":false});
    for label in ["primary","comparison","healthy","continue"] {
        let data = Dataset::open(&out.join("runtime-world").join(label),false)?;
        let scan = dataset::scan(&data,None,None,None)?;
        let process = if label=="comparison" {2}else{1};
        let trace = parse_events(&out.join(format!("process-{process}/trace.jsonl")))?;
        let traces = TraceIndex::new(&trace);
        let marks = evidence::rows(&data.root.join("diagnostics.jsonl"))?;
        let boundary = |name: &str| -> Option<u64> {
            let rows=marks.iter().filter(|m|m["kind"]=="checkpoint" && m["label"]==name).collect::<Vec<_>>();
            if rows.len()==1 {dec(&rows[0]["attempt"])} else {None}
        };
        let pause_bounds = boundary("p1.pause.blocked.begin").zip(boundary("p1.pause.blocked.end"));
        let (mut pause_rows,mut pause_ok)=(0u64,true);
        let (mut complete,mut matched,mut incomplete,mut gaps)=(0u64,0u64,0u64,0u64);
        let mut complete_outers=BTreeSet::new();
        let mut input=File::open(data.root.join("captures.jsonl")).map_err(|e|e.to_string())?;
        for (&attempt,location) in &scan.attempts {
            let c=data.at(&mut input,location)?;
            let tick=c.authority_tick.as_deref().and_then(|v|v.parse().ok());
            let scope=c.scope_token.parse::<u64>().ok();
            let loop_id=dec(&c.diagnostics["loop"]);
            let counts=traces.phase_counts(tick,loop_id,scope);
            let stages=c.diagnostics["phase_witness"].as_array().is_some_and(|a|a.len()==3
                && a.iter().enumerate().all(|(i,s)|s["stage"]==i+1 && s["entered"]==1
                    && s["returned"]==1 && dec(&s["scope"])==scope));
            let valid=c.authority_kind==1 && tick.is_some_and(|n|n>0)
                && scope.is_some_and(|n|n>0) && loop_id.is_some_and(|n|n>0)
                && c.outer_token.parse::<u64>().is_ok_and(|n|n>0) && stages && counts.ordered();
            let mut errors=vec![];
            if c.complete {
                complete+=1;
                if c.record!="capture" || c.reason!="NONE" || !valid {errors.push("COMPLETE_WITHOUT_ORDERED_STANDARD_PHASES");}
                if !complete_outers.insert(c.outer_token.clone()) {errors.push("DUPLICATE_COMPLETE_OUTER");}
                if errors.is_empty() {matched+=1;}
            } else if c.record=="capture" {
                incomplete+=1;
                // collector 只有在普通阶段完成后才开始字段复制；不完整字段仍须保留真实阶段。
                if !valid {errors.push("COPY_WITHOUT_ORDERED_STANDARD_PHASES");}
            } else {
                gaps+=1;
                if c.capture_seq.is_some() || c.authority_tick.is_some() || !c.objects.is_empty() {errors.push("GAP_PROMOTED_TO_WORLD_COPY");}
                if c.reason=="ODD_PLAYER_ONLY" {
                    odd+=1;
                    if c.authority_kind!=2 || !traces.odd_native(dec(&c.diagnostics["candidate_tick"]),scope) {
                        errors.push("ODD_GAP_WITHOUT_CURRENT_NATIVE_COMMIT");
                    }
                }
                if c.authority_kind==0 {no_authority+=1;}
                if c.reason=="LIFECYCLE" {lifecycle+=1;}
            }
            if pause_bounds.is_some_and(|(a,b)|attempt>a && attempt<=b) {
                pause_rows+=1;
                pause_ok &= c.record=="gap" && !c.complete && c.authority_kind==0
                    && dec(&c.diagnostics["candidate_tick"])==Some(0);
            }
            for reason in &errors {violations.push(json!({"recording":label,"attempt":attempt.to_string(),"reason":reason}));}
            writeln!(index,"{}",json!({"recording":label,"attempt":attempt.to_string(),"record":c.record,
                "complete":c.complete,"reason":c.reason,"outer":c.outer_token,"tick":c.authority_tick,
                "scope":c.scope_token,"loop":loop_id,"stage_witness":c.diagnostics["phase_witness"],
                "pre":counts.pre,"commit":counts.commit,"standard_publish":counts.standard_publish,
                "trace_sequence":[counts.pre_sequence,counts.commit_sequence,counts.publish_sequence],
                "ordered_standard":valid,"violations":errors})).map_err(|e|e.to_string())?;
        }
        if label=="primary" {
            pause=json!({"bounds":pause_bounds,"gap_attempts":pause_rows,"pass":pause_bounds.is_some_and(|(a,b)|b>a) && pause_rows>0 && pause_ok});
        }
        recordings[label]=json!({"complete":complete,"matched":matched,"incomplete_copies":incomplete,
            "gaps":gaps,"event_gaps":scan.event_gaps,"healthy":data.healthy(),
            "pass":complete>0 && complete==matched && !scan.event_gaps && data.healthy()});
    }
    index.flush().map_err(|e|e.to_string())?;
    let covered=["primary","comparison","healthy","continue"].iter().all(|l|recordings[*l]["pass"]==true);
    let pass=covered && violations.is_empty() && odd>0 && no_authority>0 && lifecycle>0 && pause["pass"]==true;
    Ok(json!({"status":if pass{"PASS"}else{"FAIL"},"pass":pass,"detail":{"recordings":recordings,
        "odd_gaps_with_native_commit":odd,"no_authority_gaps":no_authority,"lifecycle_gaps":lifecycle,
        "pause":pause,"violations":violations,"index":"evidence/v2-phase-index.jsonl"}}))
}
