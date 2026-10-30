//! 文件结构与 profile 校验；缺字段/未知值可以保留，但不能借 complete 声称可比。
use super::{DOMAIN, LEGACY, PHASE, Result, io, model::*, profile::*};
use serde_json::{Value, json};
use std::collections::BTreeSet;

pub fn decimal(v: &str) -> Result<u64> {
    let n = v
        .parse::<u64>()
        .map_err(|_| format!("invalid u64 decimal {v}"))?;
    if n.to_string() != v {
        return Err("u64 requires canonical decimal string".into());
    }
    Ok(n)
}
pub fn number(v: &Value) -> Result<u64> {
    decimal(v.as_str().ok_or("u64 must be a decimal string")?)
}
pub fn hex(v: &str, digits: usize) -> Result<()> {
    if v.len() != digits
        || !v
            .bytes()
            .all(|c| c.is_ascii_digit() || (b'a'..=b'f').contains(&c))
    {
        return Err("invalid fixed-width lowercase hex".into());
    }
    Ok(())
}
pub fn bits(v: &str) -> Result<u32> {
    hex(v, 8)?;
    u32::from_str_radix(v, 16).map_err(|e| e.to_string())
}

// 新增 RNG 诊断不参与世界比较 key；出现时严格保留四个原始 word 与各自状态。
fn rng_words(v: &Value) -> Result<()> {
    if v.is_null() { return Ok(()); }
    io::keys(v,&["before_status","after_status","before","after"],&[])?;
    for side in ["before","after"] {
        let status=v[format!("{side}_status")].as_u64().ok_or("rng status")?;
        if status>2 || (status==1)!=v[side].is_array() { return Err("rng payload/status mismatch".into()); }
        if status!=1 && !v[side].is_null() {return Err("unavailable RNG carries words".into());}
        if let Some(words)=v[side].as_array() {
            if words.len()!=4 {return Err("RNG requires all four words".into());}
            for word in words {hex(word.as_str().ok_or("rng word")?,8)?;}
        }
    }
    Ok(())
}

// manifest 既校验固定语义，也保留 producer/layout/未知和历史事实，不生成新出处。
pub fn manifest(v: &Value, legacy: bool) -> Result<()> {
    let domain = if legacy { LEGACY } else { DOMAIN };
    if v["domain"] != domain
        || v["version"] != 1
        || v["profile"] != PROFILE
        || v["phase"] != PHASE
        || v["session"].as_str().is_none_or(str::is_empty)
    {
        return Err("world manifest domain/version/profile/phase/session mismatch (legacy requires --legacy-t0)".into());
    }
    if !legacy {
        io::keys(
            v,
            &[
                "domain",
                "version",
                "profile",
                "session",
                "phase",
                "recording_generation",
                "producer",
                "hook_manifest",
                "observations",
                "classes",
                "sources",
                "fields",
                "required",
                "layout",
                "budgets",
                "event_kinds",
                "readiness_evidence",
                "lifecycle",
                "origin",
                "relation",
                "field_status",
                "encoding",
                "float_comparison",
                "hp_evidence",
                "current_player_read_rule",
                "unconfirmed",
                "assumptions",
                "unknowns",
                "native_diagnostics_excluded",
                "runtime_acceptance",
            ],
            &[],
        )?;
        if v["recording_generation"]
            .as_u64()
            .is_none_or(|n| n == 0 || n > u32::MAX as u64)
        {
            return Err("invalid recording generation".into());
        }
        io::keys(
            &v["producer"],
            &["component", "build", "game", "repentogon"],
            &[],
        )?;
        for k in ["component", "build", "game", "repentogon"] {
            if !v["producer"][k].is_string() {
                return Err(format!("invalid producer {k}"));
            }
        }
        io::keys(
            &v["layout"],
            &["state", "declared_fields", "fields", "sources", "classes"],
            &[],
        )?;
        let fields = v["layout"]["fields"]
            .as_array()
            .ok_or("layout fields missing")?;
        let mut ids = BTreeSet::new();
        for f in fields {
            io::keys(f, &["field_id", "offset", "evidence"], &[])?;
            let id = f["field_id"].as_str().ok_or("invalid layout id")?;
            hex(id, 4)?;
            if field_name(u16::from_str_radix(id, 16).map_err(|e| e.to_string())?)
                == "unknown_field"
                || !ids.insert(id)
                || f["offset"].as_u64().is_none_or(|n| n > u32::MAX as u64)
                || !f["evidence"].is_string()
            {
                return Err("invalid layout field".into());
            }
        }
        if v["layout"]["declared_fields"] != fields.len() as u64 || fields.len() > 78 {
            return Err("layout field count mismatch".into());
        }
        let expected = if fields.is_empty() {
            "UNDECLARED"
        } else if fields.len() == 78 {
            "DECLARED"
        } else {
            "INCOMPLETE"
        };
        if v["layout"]["state"] != expected {
            return Err("layout declaration state mismatch".into());
        }
        for (name, keys) in [
            (
                "sources",
                vec!["source", "location", "capacity", "evidence"],
            ),
            ("classes", vec!["class", "descriptor_rva", "evidence"]),
        ] {
            let entries = v["layout"][name]
                .as_array()
                .ok_or("invalid layout entries")?;
            let mut seen = BTreeSet::new();
            for e in entries {
                io::keys(e, &keys, &[])?;
                let key = if name == "sources" {
                    e["source"].to_string()
                } else {
                    e["descriptor_rva"].to_string()
                };
                if !seen.insert(key) {
                    return Err("duplicate layout entry".into());
                }
            }
        }
    }
    if v["classes"] != json!(CLASSES) || v["sources"] != json!(SOURCES) {
        return Err("manifest classes/sources mismatch".into());
    }
    if v["readiness_evidence"]
        != json!({"0":"NO_OBSERVED_INIT","1":"BASE_INIT_RETURNED","2":"KNOWN_CREATION_OWNER_COMPLETED","3":"USER_ACCEPTED_CURRENT_PLAYER_UPDATE_WITNESS"})
        || v["hp_evidence"]
            != "USER_ACCEPTED_ASSUMPTION: generic Entity HP represents life; Player hearts remain separate"
        || v["current_player_read_rule"]
            != "exact pair remains valid across a normal native PlayerUpdate in an Authority simulation window; FIRST_OBSERVED and missing birth history remain unchanged"
    {
        return Err("manifest reading evidence semantics mismatch".into());
    }
    if !legacy {
        for (key, expected) in [
            (
                "field_status",
                json!([
                    "VALUE",
                    "UNCONFIRMED",
                    "NOT_APPLICABLE",
                    "NOT_READY",
                    "READ_FAILED"
                ]),
            ),
            (
                "encoding",
                json!(["RAW_LE", "F32_BITS", "REFERENCE", "DERIVED_RAW32"]),
            ),
            (
                "relation",
                json!([
                    "NULL",
                    "KNOWN_CURRENT",
                    "UNRESOLVED",
                    "RETIRED",
                    "AMBIGUOUS_REUSE"
                ]),
            ),
            (
                "origin",
                json!({"0":"UNSET","1":"FIRST_OBSERVED","2":"OBSERVED_CREATION"}),
            ),
            (
                "lifecycle",
                json!({"0":"UNKNOWN","1":"ACQUIRED","2":"PUBLISHED","3":"CONSTRUCTED","4":"REMOVE_REQUESTED","5":"RETIRING","6":"DESTROYED"}),
            ),
            (
                "event_kinds",
                json!({"0":"NATIVE_HOOK","1":"BIRTH_CONFIRMED","2":"LIFECYCLE_BOUNDARY","3":"GAP_OR_UNKNOWN","4":"GRID_PHYSICAL_RETIRED","6":"CURRENT_PLAYER_READ_WITNESS","7":"SOURCE_MEMBERSHIP_SNAPSHOT","8":"NATIVE_BODY"}),
            ),
        ] {
            if v[key] != expected {
                return Err(format!("manifest {key} semantics mismatch"));
            }
        }
        io::keys(&v["observations"], &["declared", "enabled"], &[])?;
        let enabled = v["observations"]["enabled"]
            .as_array()
            .ok_or("invalid enabled observations")?;
        let mut seen = BTreeSet::new();
        if v["observations"]["declared"] != 19 {
            return Err("invalid observation count".into());
        }
        for entry in enabled {
            let name = entry.as_str().ok_or("invalid observation name")?;
            if !HOOKS.contains(&name) || !seen.insert(name) {
                return Err("invalid/duplicate enabled observation".into());
            }
        }
        for key in [
            "assumptions",
            "unknowns",
            "unconfirmed",
            "native_diagnostics_excluded",
        ] {
            if !v[key]
                .as_array()
                .is_some_and(|a| a.iter().all(Value::is_string))
            {
                return Err(format!("invalid manifest {key}"));
            }
        }
    }
    io::keys(&v["required"], &CLASSES, &[])?;
    let mut expected_ids = BTreeSet::new();
    for class in CLASSES {
        let ids = if class == "UnknownEntity" {
            if legacy { vec![] } else { vec![1, 2, 3, 4] }
        } else {
            required(class)
        };
        let actual = v["required"][class]
            .as_array()
            .ok_or("missing required profile class")?;
        let mut actual = actual
            .iter()
            .map(|s| s.as_str().ok_or("invalid required field".to_string()))
            .collect::<Result<Vec<_>>>()?;
        actual.sort();
        if actual != ids.iter().map(|id| format!("{id:04x}")).collect::<Vec<_>>() {
            return Err(format!("manifest required differs: {class}"));
        }
        expected_ids.extend(ids);
    }
    let fields = v["fields"].as_array().ok_or("missing field declarations")?;
    let mut seen = BTreeSet::new();
    for f in fields {
        io::keys(f, &["field_id", "name", "width", "encoding"], &[])?;
        let text = f["field_id"]
            .as_str()
            .ok_or("invalid field declaration id")?;
        hex(text, 4)?;
        let id = u16::from_str_radix(text, 16).map_err(|e| e.to_string())?;
        let (width, encoding) = expected_field(id);
        if !expected_ids.contains(&id)
            || !seen.insert(id)
            || f["width"] != width
            || f["encoding"] != encoding
            || f["name"] != field_name(id)
        {
            return Err("manifest field declaration mismatch".into());
        }
    }
    if seen != expected_ids {
        return Err("incomplete manifest profile".into());
    }
    if v["float_comparison"] != "finite exact bits; normalize signed zero; retain raw bits" {
        return Err("unsupported float semantics".into());
    }
    Ok(())
}

pub fn capture(bytes: &[u8], m: &Value, legacy: bool) -> Result<Capture> {
    let v = io::parse(bytes)?;
    let mut header_keys = vec![
        "record",
        "domain",
        "version",
        "profile",
        "session",
        "run_epoch",
        "room_epoch",
        "mutation_epoch",
        "attempt_seq",
        "capture_seq",
        "outer_token",
        "phase",
        "authority_tick",
        "authority_kind",
        "scope_token",
        "complete",
        "reason",
        "sources",
        "objects",
        "required_objects",
        "required_memberships",
        "diagnostics",
    ];
    if !legacy {
        header_keys.extend(["recording_generation", "event_watermark"]);
    }
    io::keys(&v, &header_keys, &[])?;
    if !v["diagnostics"].is_object() {
        return Err("capture diagnostics must be an object".into());
    }
    for o in v["objects"].as_array().ok_or("objects must be an array")? {
        let mut object_keys = vec![
            "local_id",
            "use_generation",
            "revision",
            "object_domain",
            "class",
            "origin",
            "lifecycle",
            "init_evidence",
            "birth_event",
            "birth",
            "ordinal",
            "fields",
            "memberships",
            "diagnostics",
        ];
        if !legacy {
            object_keys.push("grid_index");
        }
        io::keys(o, &object_keys, &[])?;
        if !o["diagnostics"].is_object() {
            return Err("object diagnostics must be an object".into());
        }
        for f in o["fields"].as_array().ok_or("fields must be an array")? {
            io::keys(
                f,
                &["field_id", "status", "encoding", "width"],
                &["raw32", "raw64", "bytes_le", "reference", "diagnostics"],
            )?;
            for k in ["raw32", "raw64", "bytes_le", "reference"] {
                if f.get(k).is_some_and(Value::is_null) {
                    return Err("explicit null field payload is not allowed".into());
                }
            }
            if f.get("diagnostics").is_some_and(|d| !d.is_object()) {
                return Err("field diagnostics must be an object".into());
            }
        }
    }
    if !legacy {
        for k in ["recording_generation", "event_watermark"] {
            if v.get(k).is_none() || v[k].is_null() {
                return Err(format!("missing {k}"));
            }
        }
        for o in v["objects"].as_array().ok_or("missing objects")? {
            if o.get("grid_index").is_none() {
                return Err("missing grid_index".into());
            }
        }
    } else {
        if v.get("recording_generation").is_some()
            || v.get("event_watermark").is_some()
            || v["objects"]
                .as_array()
                .is_some_and(|a| a.iter().any(|o| o.get("grid_index").is_some()))
        {
            return Err("new-format columns in legacy capture".into());
        }
    }
    let c: Capture = serde_json::from_value(v).map_err(|e| e.to_string())?;
    if c.domain != m["domain"]
        || c.version != 1
        || c.profile != PROFILE
        || c.session != m["session"]
        || c.phase != m["phase"]
        || !["capture", "gap"].contains(&c.record.as_str())
        || c.authority_kind > 2
        || !REASONS.contains(&c.reason.as_str())
        || c.objects.len() > 16384
        || c.sources.len() > 15
    {
        return Err("invalid world capture header/budget".into());
    }
    for n in [
        &c.run_epoch,
        &c.room_epoch,
        &c.mutation_epoch,
        &c.attempt_seq,
        &c.outer_token,
        &c.scope_token,
    ] {
        decimal(n)?;
    }
    if decimal(&c.attempt_seq)? == 0 {
        return Err("zero attempt sequence".into());
    }
    for n in [&c.capture_seq, &c.authority_tick, &c.event_watermark]
        .into_iter()
        .flatten()
    {
        decimal(n)?;
    }
    if !legacy && c.recording_generation.map(u64::from) != m["recording_generation"].as_u64() {
        return Err("capture recording generation mismatch".into());
    }
    // 原研究编码器 encode_batch 总是序列化15个 NativeSource，零初始化 gap 也如此。
    // 只在显式旧域容纳该原始空头，不丢弃诊断、不把它解释为真实枚举；新版仍须 sources=[]。
    // 非零计数/descriptor、异常状态、缺头或重号不能借 legacy 绕过 gap 约束。
    let legacy_gap_headers = legacy && c.sources.len()==15 && c.sources.iter().all(|s|
        s.count==0 && s.status=="NONE" && s.diagnostics==json!({"descriptor":[0,0,0,0]}));
    if c.record == "gap"
        && (c.complete
            || c.capture_seq.is_some()
            || !c.objects.is_empty()
            || (!c.sources.is_empty() && !legacy_gap_headers)
            || c.reason == "NONE"
            || c.required_objects != 0
            || c.required_memberships != 0)
    {
        return Err("gap contains capture data".into());
    }
    if c.record == "capture"
        && c.capture_seq
            .as_deref()
            .map(decimal)
            .transpose()?
            .is_none_or(|n| n == 0)
    {
        return Err("capture sequence missing/zero".into());
    }
    if c.complete
        && (c.record != "capture"
            || c.reason != "NONE"
            || c.authority_kind != 1
            || c.authority_tick
                .as_deref()
                .map(decimal)
                .transpose()?
                .is_none_or(|n| n == 0))
    {
        return Err("invalid complete capture credentials".into());
    }
    let mut sources = BTreeSet::new();
    for s in &c.sources {
        if s.source >= 15
            || !sources.insert(s.source)
            || (!REASONS.contains(&s.status.as_str()) && s.status != "NOT_APPLICABLE")
        {
            return Err("invalid/duplicate source".into());
        }
    }
    let mut identities = BTreeSet::new();
    let mut memberships = BTreeSet::new();
    let mut membership_count = 0;
    for o in &c.objects {
        if !legacy {rng_words(&o.diagnostics["rng_window"])?;}
        if decimal(&o.local_id)? == 0 || decimal(&o.use_generation)? == 0 {
            return Err("zero local identity".into());
        }
        decimal(&o.revision)?;
        if let Some(b) = &o.birth_event {
            if decimal(b)? == 0 {
                return Err("zero birth identity".into());
            }
        }
        if !identities.insert((o.object_domain, &o.local_id, &o.use_generation))
            || !CLASSES.contains(&o.class.as_str())
            || o.object_domain > 1
            || (o.class == "Grid" && o.object_domain != 1)
            || (o.object_domain == 1 && !["Grid", "UnknownEntity"].contains(&o.class.as_str()))
            || o.fields.len() > 52
            || o.lifecycle > 6
            || o.init_evidence > 3
            || !["UNSET", "FIRST_OBSERVED", "OBSERVED_CREATION"].contains(&o.origin.as_str())
            || o.ordinal
                .is_some_and(|n| n > 3 || !["Player", "UnknownEntity"].contains(&o.class.as_str()))
            || o.grid_index
                .is_some_and(|n| n >= 448 || o.object_domain != 1)
        {
            return Err("invalid object identity/class/state/budget".into());
        }
        for membership in &o.memberships {
            membership_count += 1;
            if membership_count > 65536
                || membership.source >= 15
                || !memberships.insert((membership.source, membership.index))
                || (membership.source == 11 && membership.index >= 448)
                || (membership.source == 12 && membership.index >= 8)
            {
                return Err("invalid/duplicate source slot or membership budget".into());
            }
        }
        let mut ids = BTreeSet::new();
        let allowed = if o.class == "UnknownEntity" {
            vec![1, 2, 3, 4]
        } else {
            required(&o.class)
        };
        for f in &o.fields {
            hex(&f.field_id, 4)?;
            let id = u16::from_str_radix(&f.field_id, 16).map_err(|e| e.to_string())?;
            let (width, encoding) = expected_field(id);
            if !ids.insert(id)
                || !allowed.contains(&id)
                || f.width != width
                || f.encoding != encoding
                || ![
                    "VALUE",
                    "UNCONFIRMED",
                    "NOT_APPLICABLE",
                    "NOT_READY",
                    "READ_FAILED",
                ]
                .contains(&f.status.as_str())
            {
                return Err("invalid/duplicate field schema".into());
            }
            let payloads = usize::from(f.raw32.is_some())
                + usize::from(f.raw64.is_some())
                + usize::from(f.bytes_le.is_some())
                + usize::from(f.reference.is_some());
            if f.status != "VALUE" {
                if payloads != 0 {
                    return Err("unavailable field carries value".into());
                }
                continue;
            }
            if payloads != 1 {
                return Err("VALUE requires exactly one payload".into());
            }
            match encoding {
                "REFERENCE" => {
                    let r = f.reference.as_ref().ok_or("reference payload missing")?;
                    if ![
                        "NULL",
                        "KNOWN_CURRENT",
                        "UNRESOLVED",
                        "RETIRED",
                        "AMBIGUOUS_REUSE",
                    ]
                    .contains(&r.status.as_str())
                        || (r.status == "KNOWN_CURRENT") != r.target.is_some()
                    {
                        return Err("invalid reference state/target".into());
                    }
                    if let Some(t) = &r.target {
                        if decimal(&t.local_id)? == 0 || decimal(&t.use_generation)? == 0 {
                            return Err("zero reference target".into());
                        }
                    }
                }
                _ if width == 4 => hex(f.raw32.as_deref().ok_or("missing raw32")?, 8)?,
                _ if width == 8 => hex(f.raw64.as_deref().ok_or("missing raw64")?, 16)?,
                _ => hex(
                    f.bytes_le.as_deref().ok_or("missing bytes_le")?,
                    width as usize * 2,
                )?,
            }
        }
    }
    if c.complete
        && (c.required_objects as usize != c.objects.len()
            || c.required_memberships as usize != membership_count)
    {
        return Err("complete record counts inconsistent".into());
    }
    Ok(c)
}

// 只排序无语义顺序的集合，保留 raw 位、状态、诊断、u64 字符串与文件行顺序。
pub fn canonicalize(c: &mut Capture) {
    c.sources.sort_by_key(|s| s.source);
    for o in &mut c.objects {
        o.fields.sort_by(|a, b| a.field_id.cmp(&b.field_id));
        o.memberships.sort();
    }
    c.objects.sort_by_key(|o| {
        (
            CLASSES.iter().position(|c| *c == o.class).unwrap_or(8),
            o.object_domain,
            decimal(&o.local_id).unwrap_or(0),
            decimal(&o.use_generation).unwrap_or(0),
        )
    });
}

// recorder 终态是输入事实；仅校验形状，失败/未收口仍能 replay 且不会变成成功录制。
pub fn terminal(v: &Value) -> Result<()> {
    io::keys(
        v,
        &[
            "status",
            "domain",
            "version",
            "profile",
            "session",
            "recording_generation",
            "readiness",
            "counts",
            "reasons",
            "undelivered",
            "failure",
            "diagnostics",
            "runtime_acceptance",
        ],
        &[],
    )?;
    io::keys(
        &v["readiness"],
        &["producer", "reader", "writer", "drain", "flush", "outcome"],
        &[],
    )?;
    for k in ["producer", "reader", "writer", "drain", "flush", "outcome"] {
        if !v["readiness"][k].is_string() {
            return Err("invalid terminal readiness".into());
        }
    }
    io::keys(
        &v["counts"],
        &[
            "attempts",
            "captures",
            "complete_captures",
            "gaps",
            "batches_copied",
            "batches_written",
            "events_produced",
            "events_copied",
            "events_written",
            "gaps_produced",
            "gaps_copied",
            "gaps_written",
            "backpressure",
            "budget_waits",
            "callbacks",
            "callback_faults",
            "discarded_items",
        ],
        &["checkpoints_produced", "checkpoints_written"],
    )?;
    io::keys(
        &v["undelivered"],
        &[
            "last_attempt",
            "last_capture",
            "last_event",
            "events_not_written",
            "gaps_not_written",
            "batches_not_written",
            "queue_items",
            "queue_bytes",
        ],
        &["checkpoints_not_written"],
    )?;
    for k in ["counts", "undelivered", "reasons"] {
        for (key, value) in v[k].as_object().ok_or("invalid terminal counts")? {
            number(value)?;
            if k == "reasons" && !REASONS.contains(&key.as_str()) {
                return Err("invalid terminal reason".into());
            }
        }
    }
    if !v["failure"].is_null() {
        io::keys(
            &v["failure"],
            &["reason", "source", "code", "stage", "at_ms", "write_error"],
            &[],
        )?;
        number(&v["failure"]["at_ms"])?;
    }
    if !v["diagnostics"].is_object() {
        return Err("invalid terminal diagnostics".into());
    }
    Ok(())
}

// 诊断有固定外壳和分种类列，checkpoint 仅验证已有 watermark，不进行额外采样。
pub fn diagnostic(v: &Value, m: &Value) -> Result<()> {
    let mut keys = vec!["record", "kind", "at_ms", "recording_generation"];
    let mut decimal_keys = Vec::new();
    match v["kind"].as_str().ok_or("invalid diagnostic kind")? {
        "prepare" => {
            keys.extend(["label", "budgets", "allocated_bytes", "planned_bytes"]);
            decimal_keys.extend(["allocated_bytes", "planned_bytes"]);
        }
        "close" => {
            keys.extend([
                "reader",
                "state",
                "attempts",
                "captures",
                "gaps",
                "backpressure",
                "queue_items",
                "queue_bytes",
                "producer_done",
            ]);
            decimal_keys.extend([
                "attempts",
                "captures",
                "gaps",
                "backpressure",
                "queue_items",
                "queue_bytes",
            ]);
        }
        "backpressure" => {
            keys.extend([
                "attempts",
                "slots_exhausted",
                "budget_waits",
                "queue_peak_items",
                "queue_peak_bytes",
                "queue_limited_items",
                "queue_limited_bytes",
                "last_attempt",
                "captures_not_written",
                "events_not_written",
                "gaps_not_written",
            ]);
            decimal_keys.extend([
                "attempts",
                "slots_exhausted",
                "budget_waits",
                "queue_peak_items",
                "queue_peak_bytes",
                "queue_limited_bytes",
                "last_attempt",
                "captures_not_written",
                "events_not_written",
                "gaps_not_written",
            ]);
        }
        "fault" if v.get("injected").is_some() => keys.extend(["injected", "target", "effect"]),
        "test_injection" => {
            keys.extend(["injected", "session", "mode", "value", "applied", "rejection", "before", "after", "attempt", "event_watermark"]);
            decimal_keys.extend(["attempt", "event_watermark"]);
            let mode = v["mode"].as_u64().filter(|n|(1..=11).contains(n)).ok_or("invalid test injection mode")?;
            if v["injected"] != "TEST_INJECTED" || v["session"] != m["session"]
                || v["applied"].as_u64().is_none_or(|v|v>1)
                || !REASONS.contains(&v["rejection"].as_str().unwrap_or(""))
                || ["before","after"].iter().any(|k|v[k].as_u64().is_none_or(|n|n>u32::MAX as u64))
                || v["value"].as_u64().is_none_or(|n|if mode==1 {!(100..=500).contains(&n)}else{n!=1}) {
                return Err("invalid test injection values".into());
            }
        }
        "fault" => {
            keys.extend([
                "reason",
                "source",
                "code",
                "stage",
                "fault_at_ms",
                "write_error",
            ]);
            decimal_keys.push("fault_at_ms");
        }
        "checkpoint" => {
            keys.extend([
                "label",
                "request",
                "sequence",
                "run",
                "room",
                "outer",
                "mutation",
                "attempt",
                "capture",
                "event_watermark",
            ]);
            decimal_keys.extend([
                "request",
                "sequence",
                "run",
                "room",
                "outer",
                "mutation",
                "attempt",
                "capture",
                "event_watermark",
            ]);
        }
        _ => return Err("unsupported diagnostic kind".into()),
    }
    io::keys(v, &keys, &[])?;
    if v["record"] != "diagnostic"
        || v["at_ms"].as_u64().is_none()
        || v["recording_generation"] != m["recording_generation"]
    {
        return Err("invalid diagnostic header".into());
    }
    for k in decimal_keys {
        number(&v[k])?;
    }
    Ok(())
}
