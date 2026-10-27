//! replay 是严格解码后 canonical 重编码，保留故障录制的原始终态，不伪造运行 PASS。
use super::{
    Result,
    dataset::{self, Dataset},
    io,
};
use serde_json::json;
use std::{path::Path, time::Instant};

pub fn run(source: &Path, out: &Path, legacy: bool) -> Result<()> {
    let started = Instant::now();
    let data = Dataset::open(source, legacy)?;
    io::save(&out.join("manifest.json"), &data.manifest)?;
    if let Some(r) = &data.terminal {
        io::save(&out.join("source-result.json"), r)?;
    }
    // 原型独有的消费/恢复/失败记录按原值转存，不升级为新版本的验收事实。
    if legacy {
        for name in [
            "consumer.json",
            "restoration.json",
            "consumer-failure.json",
            "controller-failure.json",
        ] {
            let path = source.join(name);
            if path.exists() {
                io::save(&out.join(name), &io::read_json(&path)?)?;
            }
        }
    }
    let mut captures = io::output(&out.join("captures.jsonl"))?;
    let mut events = io::output(&out.join("lifecycle-events.jsonl"))?;
    let mut diagnostics = if legacy {
        None
    } else {
        Some(io::output(&out.join("diagnostics.jsonl"))?)
    };
    let scan = dataset::scan(
        &data,
        Some(&mut captures),
        Some(&mut events),
        diagnostics.as_mut(),
    )?;
    io::flush(&mut captures)?;
    io::flush(&mut events)?;
    if let Some(d) = &mut diagnostics {
        io::flush(d)?;
    }
    let output_bytes = captures.get_ref().bytes
        + events.get_ref().bytes
        + diagnostics.as_ref().map_or(0, |d| d.get_ref().bytes);
    let writes = captures.get_ref().writes
        + events.get_ref().writes
        + diagnostics.as_ref().map_or(0, |d| d.get_ref().writes);
    let seconds = started.elapsed().as_secs_f64();
    io::save(
        &out.join("result.json"),
        &json!({"status":"REENCODED","domain":data.manifest["domain"],"version":1,"profile":data.manifest["profile"],"legacy_t0":legacy,
        "source_unchanged":true,"runtime_acceptance":"NOT_RUN","counts":scan.counts(),"source_recording_healthy":data.healthy(),"event_coverage_unknown":scan.event_gaps,
        "io":{"buffer_bytes":io::BUFFER,"input_bytes":scan.bytes.to_string(),"output_bytes":output_bytes.to_string(),"file_write_calls":writes.to_string(),"elapsed_ms":started.elapsed().as_millis().to_string(),"mib_per_second":if seconds>0.0{scan.bytes as f64/1048576.0/seconds}else{0.0},"flushed":true}}),
    )
}
