//! 流式读取完整文件，内存只保留序号/位置/出生元数据；世界对象逐条释放。
use super::{
    Result, io,
    model::Capture,
    profile::{HOOKS, REASONS},
    validate::{self, decimal, number},
};
use serde_json::{Value, json};
use std::{
    collections::{BTreeMap, BTreeSet},
    fs::File,
    io::{Read, Seek, SeekFrom},
    path::{Path, PathBuf},
};

// 一份文件域和其真实生产终态；replay 的状态与原 recorder 状态分开保存。
pub struct Dataset {
    pub root: PathBuf,
    pub manifest: Value,
    pub terminal: Option<Value>,
    pub legacy: bool,
}
impl Dataset {
    pub fn open(root: &Path, legacy: bool) -> Result<Self> {
        let manifest = io::read_json(&root.join("manifest.json"))?;
        validate::manifest(&manifest, legacy)?;
        // replay 的工具结果不能替代原始 recorder 终态；原件单独保留并继续沿用。
        let terminal_path = if root.join("source-result.json").exists() {
            root.join("source-result.json")
        } else {
            root.join("result.json")
        };
        let terminal = if terminal_path.exists() {
            Some(io::read_json(&terminal_path)?)
        } else {
            None
        };
        if !legacy {
            let r = terminal
                .as_ref()
                .ok_or("new world artifact requires recorder result.json")?;
            validate::terminal(r)?;
            for k in [
                "domain",
                "version",
                "profile",
                "session",
                "recording_generation",
            ] {
                if r[k] != manifest[k] {
                    return Err(format!("recorder result mismatch at {k}"));
                }
            }
            if !["CAPTURE_CLOSED", "CAPTURE_ERROR", "CAPTURE_UNCONFIRMED"]
                .contains(&r["status"].as_str().unwrap_or(""))
            {
                return Err("unsupported recorder result".into());
            }
        }
        Ok(Self {
            root: root.into(),
            manifest,
            terminal,
            legacy,
        })
    }
    pub fn healthy(&self) -> bool {
        let Some(r) = &self.terminal else {
            return false;
        };
        if self.legacy {
            return false;
        } // 历史流水线事实保留，不推导新版收口资格。
        r["status"] == "CAPTURE_CLOSED"
            && r["failure"].is_null()
            && r["readiness"]["drain"] == "COMPLETE"
            && r["readiness"]["flush"] == "FLUSHED"
            && r["diagnostics"]["files_flushed"] == true
            && r["counts"]["discarded_items"] == "0"
            && r["counts"]["callback_faults"] == "0"
            && [
                ("events_produced", "events_written"),
                ("events_copied", "events_written"),
                ("gaps_produced", "gaps_written"),
                ("gaps_copied", "gaps_written"),
                ("batches_copied", "batches_written"),
            ]
            .iter()
            .all(|(a, b)| r["counts"][a] == r["counts"][b])
            && [
                "events_not_written",
                "gaps_not_written",
                "batches_not_written",
                "queue_items",
                "queue_bytes",
            ]
            .iter()
            .all(|k| r["undelivered"][k] == "0")
            && r["undelivered"]
                .get("checkpoints_not_written")
                .is_none_or(|v| v == "0")
    }
    pub fn at(&self, file: &mut File, location: &Location) -> Result<Capture> {
        file.seek(SeekFrom::Start(location.offset))
            .map_err(|e| e.to_string())?;
        let mut bytes = vec![0; location.length];
        file.read_exact(&mut bytes).map_err(|e| e.to_string())?;
        validate::capture(&bytes, &self.manifest, self.legacy)
    }
}

// 单条索引只保存定位和覆盖所需的值，不能缓存整份世界。
pub struct Location {
    pub offset: u64,
    pub length: usize,
    pub run: String,
    pub room: String,
    pub tick: Option<u64>,
    pub mutation: u64,
    pub outer: u64,
    pub authority: u32,
    pub complete: bool,
    pub reason: String,
    pub watermark: u64,
}
// 出生编号不是 lifecycle sequence；二者都保存，按 watermark 检查先后关系。
pub struct BirthEvidence {
    pub sequence: u64,
    pub run: String,
    pub room: String,
    pub local: String,
    pub usage: String,
}
// 索引持有轻量元数据与累计计数，不持有对象字段或世界快照。
pub struct Scan {
    pub attempts: BTreeMap<u64, Location>,
    pub births: BTreeMap<u64, BirthEvidence>,
    pub records: u64,
    pub captures: u64,
    pub gaps: u64,
    pub events: u64,
    pub diagnostics: u64,
    pub complete: u64,
    pub event_gaps: bool,
    pub bytes: u64,
}
impl Scan {
    pub fn counts(&self) -> Value {
        json!({"records":self.records.to_string(),"captures":self.captures.to_string(),"gaps":self.gaps.to_string(),"complete_captures":self.complete.to_string(),"events":self.events.to_string(),"diagnostics":self.diagnostics.to_string()})
    }
}

pub fn scan(
    data: &Dataset,
    mut capture_out: Option<&mut io::Output>,
    mut events_out: Option<&mut io::Output>,
    mut diagnostics_out: Option<&mut io::Output>,
) -> Result<Scan> {
    let mut scan = Scan {
        attempts: BTreeMap::new(),
        births: BTreeMap::new(),
        records: 0,
        captures: 0,
        gaps: 0,
        events: 0,
        diagnostics: 0,
        complete: 0,
        event_gaps: false,
        bytes: 0,
    };
    let mut input = io::reader(&data.root.join("captures.jsonl"))?;
    let mut bytes = Vec::new();
    let mut offset = 0;
    let mut captures = BTreeSet::new();
    loop {
        let length = io::read_line(&mut input, &mut bytes)?;
        if length == 0 {
            break;
        }
        let mut c = validate::capture(&bytes, &data.manifest, data.legacy)
            .map_err(|e| format!("captures.jsonl offset {offset}: {e}"))?;
        let attempt = decimal(&c.attempt_seq)?;
        if let Some(seq) = &c.capture_seq {
            if !captures.insert(decimal(seq)?) {
                return Err("duplicate capture sequence".into());
            }
        }
        let loc = Location {
            offset,
            length,
            run: c.run_epoch.clone(),
            room: c.room_epoch.clone(),
            tick: c.authority_tick.as_deref().map(decimal).transpose()?,
            mutation: decimal(&c.mutation_epoch)?,
            outer: decimal(&c.outer_token)?,
            authority: c.authority_kind,
            complete: c.complete,
            reason: c.reason.clone(),
            watermark: c
                .event_watermark
                .as_deref()
                .map(decimal)
                .transpose()?
                .unwrap_or(0),
        };
        if scan.attempts.insert(attempt, loc).is_some() {
            return Err("duplicate attempt sequence".into());
        }
        scan.records += 1;
        if c.record == "capture" {
            scan.captures += 1;
        } else {
            scan.gaps += 1;
        }
        if c.complete {
            scan.complete += 1;
        }
        if let Some(out) = capture_out.as_deref_mut() {
            validate::canonicalize(&mut c);
            let mut value = serde_json::to_value(&c).map_err(|e| e.to_string())?;
            if data.legacy {
                for o in value["objects"].as_array_mut().ok_or("missing objects")? {
                    o.as_object_mut()
                        .ok_or("invalid object")?
                        .remove("grid_index");
                }
            }
            io::line(out, &value)?;
        }
        offset = offset
            .checked_add(length as u64)
            .ok_or("capture offset overflow")?;
    }
    scan.bytes = offset;
    let event_path = data.root.join("lifecycle-events.jsonl");
    // 完整 legacy 材料与新版均读取生命周期文件；不启动旧 runtime 补事件。
    let mut input = io::reader(&event_path)?;
    let mut previous = 0u64;
    while io::read_line(&mut input, &mut bytes)? != 0 {
        let event = io::parse(&bytes)?;
        validate_event(&event, data.legacy)?;
        let seq = number(&event["sequence"])?;
        if seq <= previous {
            return Err("non-increasing lifecycle sequence".into());
        }
        if previous.checked_add(1) != Some(seq) {
            scan.event_gaps = true;
        }
        previous = seq;
        if event["event_kind"] == 1 {
            let birth = number(&event["birth_event"])?;
            let e = BirthEvidence {
                sequence: seq,
                run: event["run_epoch"].as_str().unwrap().into(),
                room: event["room_epoch"].as_str().unwrap().into(),
                local: event["local_id"]
                    .as_str()
                    .ok_or("birth has no local id")?
                    .into(),
                usage: event["use_generation"]
                    .as_str()
                    .ok_or("birth has no use generation")?
                    .into(),
            };
            if scan.births.insert(birth, e).is_some() {
                return Err("duplicate confirmed birth event".into());
            }
        }
        if let Some(out) = events_out.as_deref_mut() {
            io::line(out, &event)?;
        }
        scan.events += 1;
    }
    scan.bytes += input_bytes(&event_path)?;
    if !data.legacy {
        for loc in scan.attempts.values() {
            if loc.watermark > previous {
                scan.event_gaps = true;
            }
        }
        let path = data.root.join("diagnostics.jsonl");
        let mut input = io::reader(&path)?;
        let mut checkpoint_sequence = 0u64;
        let mut checkpoints = 0u64;
        while io::read_line(&mut input, &mut bytes)? != 0 {
            let d = io::parse(&bytes)?;
            validate::diagnostic(&d, &data.manifest)?;
            if d["kind"] == "checkpoint" {
                let sequence = number(&d["sequence"])?;
                if sequence <= checkpoint_sequence {
                    return Err("non-increasing checkpoint sequence".into());
                }
                checkpoint_sequence = sequence;
                checkpoints += 1;
                if number(&d["event_watermark"])? > previous {
                    scan.event_gaps = true;
                }
            }
            if let Some(out) = diagnostics_out.as_deref_mut() {
                io::line(out, &d)?;
            }
            scan.diagnostics += 1;
        }
        scan.bytes += input_bytes(&path)?;
        let terminal = data.terminal.as_ref().unwrap();
        if let Some(count) = terminal["counts"].get("checkpoints_written") {
            if number(count)? != checkpoints {
                return Err("checkpoint file/count mismatch".into());
            }
        }
        for (key, count) in [
            ("batches_written", scan.captures),
            ("gaps_written", scan.gaps),
            ("events_written", scan.events),
        ] {
            if number(&terminal["counts"][key])? != count {
                return Err(format!("recorder {key} disagrees with actual file rows"));
            }
        }
    }
    Ok(scan)
}
fn input_bytes(path: &Path) -> Result<u64> {
    std::fs::metadata(path)
        .map(|m| m.len())
        .map_err(|e| e.to_string())
}

// 新旧事件列明确隔离；数值/可空标识逐列检查，诊断不参与跨进程配对。
fn validate_event(e: &Value, legacy: bool) -> Result<()> {
    let mut keys = vec![
        "record",
        "sequence",
        "run_epoch",
        "room_epoch",
        "outer_token",
        "local_id",
        "use_generation",
        "birth_event",
        "revision",
        "entering",
        "event_kind",
        "reason",
        "diagnostics",
    ];
    if legacy {
        keys.push("hook");
    } else {
        keys.extend([
            "call",
            "parent",
            "observation",
            "exceptional",
            "memberships",
        ]);
    }
    io::keys(e, &keys, &[])?;
    if e["record"] != "lifecycle_event"
        || number(&e["sequence"])? == 0
        || !e["entering"].is_boolean()
        || !REASONS.contains(&e["reason"].as_str().unwrap_or(""))
    {
        return Err("invalid lifecycle event header".into());
    }
    for k in ["run_epoch", "room_epoch", "outer_token"] {
        number(&e[k])?;
    }
    for k in ["local_id", "use_generation", "birth_event", "revision"] {
        if !e[k].is_null() {
            number(&e[k])?;
        }
    }
    if e["local_id"].is_null() != e["use_generation"].is_null()
        || e["local_id"].is_null() != e["revision"].is_null()
    {
        return Err("incomplete event identity".into());
    }
    let kind = e["event_kind"].as_u64().ok_or("invalid event kind")?;
    if kind > if legacy { 7 } else { 8 } || kind == 5 {
        return Err("unsupported lifecycle event kind".into());
    }
    let hook = if legacy {
        &e["hook"]
    } else {
        &e["observation"]
    };
    if !hook.is_null() && !HOOKS.contains(&hook.as_str().unwrap_or("")) {
        return Err("unknown lifecycle observation".into());
    }
    if !legacy {
        for k in ["call", "parent"] {
            if !e[k].is_null() && number(&e[k])? > u32::MAX as u64 {
                return Err("event call exceeds u32".into());
            }
        }
        if !e["exceptional"].is_boolean()
            || e["memberships"]
                .as_u64()
                .is_none_or(|n| n > u32::MAX as u64)
        {
            return Err("invalid event flags".into());
        }
    }
    Ok(())
}
