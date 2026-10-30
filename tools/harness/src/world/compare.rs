//! 显式对齐后的逐 tick/类别/对象/字段比较；未知与已知差异同时保留。
use super::{
    PHASE, Result,
    dataset::{self, Dataset, Location, Scan},
    io,
    model::*,
    profile::*,
    validate::{bits, decimal},
};
use serde::Deserialize;
use serde_json::{Value, json};
use std::{
    collections::{BTreeMap, BTreeSet},
    fs::{self, File},
    path::Path,
};
fn error(e: impl std::fmt::Display) -> String {
    e.to_string()
}
fn raw_bits(s: &str, _digits: usize) -> Result<u64> {
    bits(s).map(u64::from)
}
fn object_key(
    object: &Object,
    births: &BTreeMap<u64, usize>,
    players: &BTreeMap<String, usize>,
    single_body: bool,
) -> Option<String> {
    if object.class == "Player" {
        if let Some(ordinal) = object.ordinal {
            let key = format!("{ordinal}:{}:{}", object.local_id, object.use_generation);
            if let Some(mapped) = players.get(&key).or_else(|| {
                single_body
                    .then(|| players.get(&ordinal.to_string()))
                    .flatten()
            }) {
                return Some(format!("player:{mapped:02}"));
            }
        }
    }
    let birth = decimal(object.birth_event.as_deref()?).ok()?;
    let mapped = births.get(&birth)?;
    Some(format!("birth:{mapped:020}:{}", object.object_domain))
}

// 对齐只接受明确的一对一映射；证据文件随产物复制，出生另由生命周期行核对。
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Scope {
    run_epoch: String,
    room_epoch: String,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Pair {
    left: String,
    right: String,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct PlayerPair {
    left: u32,
    right: u32,
    #[serde(default)]
    left_entity: Option<PlayerEntity>,
    #[serde(default)]
    right_entity: Option<PlayerEntity>,
}
// 多身体必须明确配对每个Entity local_id/use，参与者ordinal不再充当身体ID。
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct PlayerEntity {
    local_id: String,
    use_generation: String,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Evidence {
    input: Pair,
    scenario: Pair,
    births: Pair,
    players: Pair,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Alignment {
    version: u32,
    left: Scope,
    right: Scope,
    ticks: Vec<Pair>,
    births: Vec<Pair>,
    players: Vec<PlayerPair>,
    evidence: Evidence,
}

// 严格比较构建与整个布局/采样声明；缺失信息只产生 INCOMPARABLE。
fn compatibility(left: &Dataset, right: &Dataset) -> Vec<String> {
    let mut reasons = Vec::new();
    if left.legacy || right.legacy {
        reasons.push("LEGACY_SEMANTIC_MAPPING_UNCONFIRMED".into());
        return reasons;
    }
    for k in [
        "domain",
        "version",
        "profile",
        "phase",
        "producer",
        "layout",
        "fields",
        "required",
        "classes",
        "sources",
        "float_comparison",
        "readiness_evidence",
        "current_player_read_rule",
        "hp_evidence",
        "unconfirmed",
        "assumptions",
        "unknowns",
        "observations",
    ] {
        if left.manifest[k] != right.manifest[k] {
            reasons.push(format!("MANIFEST_MISMATCH:{k}"));
        }
    }
    for (side, data) in [("left", left), ("right", right)] {
        for k in ["component", "build", "game", "repentogon"] {
            if data.manifest["producer"][k]
                .as_str()
                .is_none_or(str::is_empty)
            {
                reasons.push(format!("PRODUCER_UNDECLARED:{side}:{k}"));
            }
        }
        if data.manifest["layout"]["state"] != "DECLARED" {
            reasons.push(format!("LAYOUT_UNDECLARED:{side}"));
        }
    }
    reasons
}

// 先按 attempt 排元数据，避免把 writer 的不同环消费顺序误当作序号倒退。
fn index<'a>(scan: &'a Scan, scope: &Scope) -> Result<BTreeMap<u64, Vec<(&'a Location, bool)>>> {
    decimal(&scope.run_epoch)?;
    decimal(&scope.room_epoch)?;
    let mut out: BTreeMap<u64, Vec<(&Location, bool)>> = BTreeMap::new();
    let mut unknown = false;
    let mut previous = 0u64;
    let mut previous_context = None;
    let mut previous_tick = None;
    for (&attempt, loc) in &scan.attempts {
        if loc.run != scope.run_epoch || loc.room != scope.room_epoch {
            continue;
        }
        if previous != 0 && previous.checked_add(1) != Some(attempt) {
            unknown = true;
        }
        previous = attempt;
        if previous_context
            .is_some_and(|last: (u64, u64)| last.0 != loc.mutation || last.1 >= loc.outer)
        {
            unknown = true;
        }
        previous_context = Some((loc.mutation, loc.outer));
        if let Some(tick) = loc.tick {
            if previous_tick.is_some_and(|last: u64| last.checked_add(1) != Some(tick)) {
                unknown = true;
            }
            previous_tick = Some(tick);
        }
        let before = unknown;
        if let Some(tick) = loc.tick {
            out.entry(tick).or_default().push((loc, before));
        }
        // odd-only 对完整世界比较仍是未知；2,000 次稳定段计数例外属于 E2E 归约。
        if !loc.complete || loc.authority != 1 || loc.reason != "NONE" {
            unknown = true;
        }
    }
    Ok(out)
}

fn birth_mapping<'a>(
    scan: &'a Scan,
    scope: &Scope,
    birth: u64,
) -> Result<&'a dataset::BirthEvidence> {
    let event = scan
        .births
        .get(&birth)
        .ok_or_else(|| format!("mapped birth {birth} has no BIRTH_CONFIRMED event"))?;
    // 保留对象可能出生于本 run 的更早房间；当前成员关系由所选 capture 决定。
    if event.run != scope.run_epoch {
        return Err(format!("mapped birth {birth} is outside alignment scope"));
    }
    Ok(event)
}

// 单对象的错误出生关联不可参与比较；重复 key 保留整组供冲突报告，不能先拒绝而丢组。
fn verified_births(
    c: &Capture,
    scan: &Scan,
    births: &BTreeMap<u64, usize>,
) -> Result<(BTreeMap<u64, usize>, bool)> {
    let mut usable = births.clone();
    let mut unknown = false;
    let mut counts = BTreeMap::new();
    for o in &c.objects {
        if let Some(b) = &o.birth_event {
            *counts.entry(decimal(b)?).or_insert(0usize) += 1;
        }
    }
    for o in &c.objects {
        if let Some(birth) = &o.birth_event {
            let birth = decimal(birth)?;
            if !births.contains_key(&birth) {
                continue;
            }
            let e = scan.births.get(&birth).ok_or("birth evidence missing")?;
            if e.local != o.local_id
                || e.usage != o.use_generation
                || c.event_watermark
                    .as_deref()
                    .map(decimal)
                    .transpose()?
                    .is_some_and(|w| e.sequence > w)
            {
                unknown = true;
                if counts[&birth] == 1 {
                    usable.remove(&birth);
                }
            }
        }
    }
    Ok((usable, unknown))
}

pub fn run(
    left: &Path,
    right: &Path,
    alignment_path: &Path,
    out: &Path,
    legacy: bool,
) -> Result<()> {
    // legacy 选项允许显式读取旧域；混合域记录仍保持各自出处且不作兼容比较。
    let open = |p: &Path| -> Result<Dataset> {
        let m = io::read_json(&p.join("manifest.json"))?;
        if m["domain"] == super::LEGACY && legacy {
            Dataset::open(p, true)
        } else {
            Dataset::open(p, false)
        }
    };
    let ld = open(left)?;
    let rd = open(right)?;
    let value = io::read_json(alignment_path)?;
    let a: Alignment = serde_json::from_value(value.clone()).map_err(error)?;
    if a.version != 1 || a.ticks.is_empty() {
        return Err("alignment version 1 requires explicit tick pairs".into());
    }
    let mut previous = None;
    for p in &a.ticks {
        let pair = (decimal(&p.left)?, decimal(&p.right)?);
        if pair.0 == 0
            || pair.1 == 0
            || previous.is_some_and(|last: (u64, u64)| last.0 >= pair.0 || last.1 >= pair.1)
        {
            return Err("tick pairs must be positive and strictly increasing on both sides".into());
        }
        previous = Some(pair);
    }
    let mut lb = BTreeMap::new();
    let mut rb = BTreeMap::new();
    for (i, p) in a.births.iter().enumerate() {
        if lb.insert(decimal(&p.left)?, i).is_some() || rb.insert(decimal(&p.right)?, i).is_some() {
            return Err("birth mapping must be one-to-one".into());
        }
    }
    let mut lp = BTreeMap::new();
    let mut rp = BTreeMap::new();
    for (i, p) in a.players.iter().enumerate() {
        if p.left > 3 || p.right != p.left || p.left_entity.is_some() != p.right_entity.is_some() {
            return Err("Player mapping must preserve participant ordinal 0..3 and pair explicit body identities".into());
        }
        let key = |ordinal: u32, entity: &Option<PlayerEntity>| -> Result<String> {
            match entity {
                None => Ok(ordinal.to_string()),
                Some(e) => {
                    decimal(&e.local_id)?;
                    decimal(&e.use_generation)?;
                    Ok(format!("{ordinal}:{}:{}", e.local_id, e.use_generation))
                }
            }
        };
        if lp.insert(key(p.left, &p.left_entity)?, i).is_some()
            || rp.insert(key(p.right, &p.right_entity)?, i).is_some()
        {
            return Err("Player body mapping must be one-to-one".into());
        }
    }
    let evidence_root = out.join("alignment-evidence");
    fs::create_dir(&evidence_root).map_err(error)?;
    let base = alignment_path.parent().unwrap_or(Path::new("."));
    let mut resolved = Vec::new();
    for (kind, pair) in [
        ("input", &a.evidence.input),
        ("scenario", &a.evidence.scenario),
        ("births", &a.evidence.births),
        ("players", &a.evidence.players),
    ] {
        for (side, path) in [("left", &pair.left), ("right", &pair.right)] {
            if path.is_empty() {
                return Err(format!("empty alignment evidence {kind}/{side}"));
            }
            let source = base.join(path).canonicalize().map_err(error)?;
            if !source.is_file() {
                return Err(format!(
                    "alignment evidence is not a file: {}",
                    source.display()
                ));
            }
            let target = evidence_root.join(format!("{kind}-{side}.evidence"));
            fs::copy(&source, &target).map_err(error)?;
            resolved.push(json!({"kind":kind,"side":side,"source":source,"artifact":target.file_name().unwrap().to_string_lossy()}));
        }
    }
    io::save(&out.join("alignment.json"), &value)?;
    io::save(&out.join("alignment-evidence.json"), &resolved)?;
    io::save(&out.join("left-manifest.json"), &ld.manifest)?;
    io::save(&out.join("right-manifest.json"), &rd.manifest)?;
    let ls = dataset::scan(&ld, None, None, None)?;
    let rs = dataset::scan(&rd, None, None, None)?;
    let mut birth_proof = Vec::new();
    for p in &a.births {
        for (side, scan, scope, birth) in [
            ("left", &ls, &a.left, decimal(&p.left)?),
            ("right", &rs, &a.right, decimal(&p.right)?),
        ] {
            let e = birth_mapping(scan, scope, birth)?;
            birth_proof.push(json!({"side":side,"birth_event":birth.to_string(),"event_sequence":e.sequence.to_string(),"run_epoch":e.run,"birth_room_epoch":e.room,"local_id":e.local,"use_generation":e.usage}));
        }
    }
    io::save(&out.join("birth-evidence.json"), &birth_proof)?;
    let li = index(&ls, &a.left)?;
    let ri = index(&rs, &a.right)?;
    let incompatible = compatibility(&ld, &rd);
    let mut lf = File::open(left.join("captures.jsonl")).map_err(error)?;
    let mut rf = File::open(right.join("captures.jsonl")).map_err(error)?;
    let mut output = io::output(&out.join("comparisons.jsonl"))?;
    let mut collisions = io::output(&out.join("key-collisions.jsonl"))?;
    let mut earlier = ls.event_gaps
        || rs.event_gaps
        || (!ld.legacy && !ld.healthy())
        || (!rd.legacy && !rd.healthy());
    let mut different = false;
    let mut incomparable = earlier;
    let mut first = None;
    let mut known_fields = 0u64;
    let mut previous_ticks = None;
    for p in &a.ticks {
        let ticks = (decimal(&p.left)?, decimal(&p.right)?);
        if previous_ticks.is_some_and(|last: (u64, u64)| {
            last.0.checked_add(1) != Some(ticks.0) || last.1.checked_add(1) != Some(ticks.1)
        }) {
            earlier = true;
        }
        previous_ticks = Some(ticks);
        let l = li.get(&ticks.0);
        let r = ri.get(&ticks.1);
        let row = if !incompatible.is_empty() {
            json!({"record":"comparison","status":"INCOMPARABLE","reason":"INCOMPATIBLE_MANIFEST","details":incompatible,"left_tick":p.left,"right_tick":p.right,"earlier_coverage_unknown":true})
        } else if let (Some(l), Some(r)) = (l, r) {
            if l.len() != 1 || r.len() != 1 {
                json!({"record":"comparison","status":"INCOMPARABLE","reason":"ALIGNMENT_AMBIGUOUS","left_tick":p.left,"right_tick":p.right,"earlier_coverage_unknown":true})
            } else {
                earlier |= l[0].1 || r[0].1;
                let lc = ld.at(&mut lf, l[0].0)?;
                let rc = rd.at(&mut rf, r[0].0)?;
                let (local_lb, lu) = verified_births(&lc, &ls, &lb)?;
                let (local_rb, ru) = verified_births(&rc, &rs, &rb)?;
                compare_pair(
                    &lc,
                    &rc,
                    &local_lb,
                    &local_rb,
                    &lp,
                    &rp,
                    earlier || lu || ru,
                )?
            }
        } else {
            json!({"record":"comparison","status":"INCOMPARABLE","reason":"SAMPLE_GAP","left_tick":p.left,"right_tick":p.right,"earlier_coverage_unknown":true})
        };
        known_fields += row["known_field_comparisons"].as_u64().unwrap_or(0);
        if row["status"] == "DIFFERENT" {
            different = true;
            if first.is_none() {
                first = Some(row["first_observed_difference"].clone());
            }
        }
        if row["status"] == "INCOMPARABLE" || row["earlier_coverage_unknown"] == true {
            incomparable = true;
            earlier = true;
        }
        if let Some(items) = row["collisions"].as_array() {
            for item in items {
                io::line(&mut collisions, item)?;
            }
        }
        io::line(&mut output, &row)?;
    }
    io::flush(&mut output)?;
    io::flush(&mut collisions)?;
    io::save(
        &out.join("result.json"),
        &json!({"status":if different{"DIFFERENT"}else if incomparable{"INCOMPARABLE"}else{"EQUAL_WITHIN_PROFILE"},"domain":ld.manifest["domain"],"profile":PROFILE,
        "scope":"only explicitly paired ticks and explicitly mapped objects","tick_pairs":a.ticks.len(),"known_field_comparisons":known_fields.to_string(),"first_observed_difference":first,
        "earlier_coverage_unknown":earlier,"uncompared_regions":incomparable,"manifest_incompatibilities":incompatible,"left_counts":ls.counts(),"right_counts":rs.counts(),"runtime_acceptance":"NOT_RUN","flushed":true}),
    )
}
type Groups<'a> = BTreeMap<(usize, String), Vec<&'a Object>>;
type LocalKeys = BTreeMap<(String, String), Option<String>>;
fn group<'a>(
    capture: &'a Capture,
    births: &BTreeMap<u64, usize>,
    players: &BTreeMap<String, usize>,
    unknown: &mut Vec<Value>,
) -> (Groups<'a>, LocalKeys) {
    let mut groups = Groups::new();
    let mut local = LocalKeys::new();
    for object in &capture.objects {
        let single_body = object.ordinal.is_none_or(|ordinal| {
            capture
                .objects
                .iter()
                .filter(|o| o.class == "Player" && o.ordinal == Some(ordinal))
                .count()
                == 1
        });
        let key = object_key(object, births, players, single_body);
        // Reference 隐含 Entity 域；Grid 的同值编号不能覆盖实体引用。
        if object.object_domain == 0 {
            local.insert(
                (object.local_id.clone(), object.use_generation.clone()),
                if object.lifecycle >= 5 || object.class == "UnknownEntity" {
                    None
                } else {
                    key.clone()
                },
            );
        }
        if let Some(key) = key {
            let class = CLASSES.iter().position(|c| *c == object.class).unwrap_or(8);
            groups.entry((class, key)).or_default().push(object);
        } else {
            unknown.push(json!({"reason":"IDENTITY_UNRESOLVED","class":object.class,"local_id":object.local_id,"use_generation":object.use_generation}));
        }
    }
    // 冲突组的引用目标同样不可比，不能从整组中任意挑选一个对象。
    let mut counts = BTreeMap::new();
    for ((_, key), values) in &groups {
        *counts.entry(key).or_insert(0usize) += values.len();
    }
    for ((_, key), values) in &groups {
        if counts[key] > 1 {
            for object in values {
                if object.object_domain != 0 {
                    continue;
                }
                local.insert(
                    (object.local_id.clone(), object.use_generation.clone()),
                    None,
                );
            }
        }
    }
    (groups, local)
}
fn comparable(field: &Field, keys: &LocalKeys) -> Result<Option<Value>> {
    if field.status != "VALUE" {
        return Ok(None);
    }
    if field.encoding == "REFERENCE" {
        let reference = field
            .reference
            .as_ref()
            .ok_or_else(|| error("missing reference"))?;
        if reference.status == "NULL" {
            return Ok(Some(json!({"relation":"NULL"})));
        }
        if reference.status != "KNOWN_CURRENT" {
            return Ok(None);
        }
        let target = reference
            .target
            .as_ref()
            .ok_or_else(|| error("missing reference target"))?;
        return Ok(keys
            .get(&(target.local_id.clone(), target.use_generation.clone()))
            .and_then(|key| key.as_ref())
            .map(|key| json!({"relation":"KNOWN_CURRENT","target":key})));
    }
    if field.encoding == "F32_BITS" {
        let bits = raw_bits(
            field
                .raw32
                .as_deref()
                .ok_or_else(|| error("missing float bits"))?,
            8,
        )? as u32;
        if bits & 0x7f800000 == 0x7f800000 {
            return Ok(None);
        }
        return Ok(Some(
            json!({"raw32":format!("{:08x}",if bits&0x7fffffff==0{0}else{bits})}),
        ));
    }
    Ok(Some(
        json!({"raw32":field.raw32,"raw64":field.raw64,"bytes_le":field.bytes_le}),
    ))
}
fn compare_pair(
    left: &Capture,
    right: &Capture,
    lb: &BTreeMap<u64, usize>,
    rb: &BTreeMap<u64, usize>,
    lp: &BTreeMap<String, usize>,
    rp: &BTreeMap<String, usize>,
    earlier: bool,
) -> Result<Value> {
    let mut unknown = Vec::new();
    let mut differences = Vec::new();
    let mut collisions = Vec::new();
    let mut known_fields = 0u64;
    if earlier {
        unknown.push(json!({"reason":"EARLIER_COVERAGE_UNKNOWN"}));
    }
    if left.phase != PHASE
        || right.phase != left.phase
        || left.authority_kind != 1
        || right.authority_kind != 1
        || left.record != "capture"
        || right.record != "capture"
    {
        return Ok(
            json!({"record":"comparison","status":"INCOMPARABLE","reason":"ALIGNMENT_UNAVAILABLE","left_tick":left.authority_tick,"right_tick":right.authority_tick,"earlier_coverage_unknown":true}),
        );
    }
    if !left.complete || !right.complete {
        unknown.push(json!({"reason":"INCOMPLETE","left":left.reason,"right":right.reason}));
    }
    for (side, capture) in [("left", left), ("right", right)] {
        let sources: BTreeMap<_, _> = capture
            .sources
            .iter()
            .map(|source| (source.source, source))
            .collect();
        for id in 0..15 {
            if !sources.get(&id).is_some_and(|source| {
                source.status == if id < 13 { "NONE" } else { "NOT_APPLICABLE" }
            }) {
                unknown.push(json!({"reason":"SOURCE_UNAVAILABLE","side":side,"source":id}));
            }
        }
    }
    for a in &left.sources {
        if let Some(b) = right.sources.iter().find(|s| s.source == a.source) {
            if a.status == "NONE" && b.status == "NONE" && a.count != b.count {
                differences.push(json!({"path":[left.run_epoch,left.room_epoch,left.authority_tick,"sources",a.source],"field":"count","left":a.count,"right":b.count}));
            }
        }
    }
    let (lg, lk) = group(left, lb, lp, &mut unknown);
    let (rg, rk) = group(right, rb, rp, &mut unknown);
    // 当前来源/身份的完整性决定 presence；更早的未知不抹掉当前已知缺失。
    let presence_known = left.complete
        && right.complete
        && [(left, lb, lp), (right, rb, rp)].iter().all(|(c, b, p)| {
            (0..13).all(|id| {
                c.sources
                    .iter()
                    .any(|s| s.source == id && s.status == "NONE")
            }) && c.objects.iter().all(|o| {
                let single_body = o.ordinal.is_none_or(|ordinal| {
                    c.objects
                        .iter()
                        .filter(|v| v.class == "Player" && v.ordinal == Some(ordinal))
                        .count()
                        == 1
                });
                o.class != "UnknownEntity" && object_key(o, b, p, single_body).is_some()
            })
        });
    let mut key_counts = BTreeMap::new();
    for ((_, key), values) in &lg {
        key_counts.entry(key).or_insert((0usize, 0usize)).0 += values.len();
    }
    for ((_, key), values) in &rg {
        key_counts.entry(key).or_insert((0usize, 0usize)).1 += values.len();
    }
    let keys: BTreeSet<_> = lg.keys().chain(rg.keys()).cloned().collect();
    let mut domains = Vec::new();
    for (class, key) in keys {
        let l = lg.get(&(class, key.clone())).map_or(&[][..], Vec::as_slice);
        let r = rg.get(&(class, key.clone())).map_or(&[][..], Vec::as_slice);
        let path = json!([
            left.run_epoch,
            left.room_epoch,
            left.authority_tick,
            CLASSES[class],
            key
        ]);
        if key_counts[&key].0 > 1 || key_counts[&key].1 > 1 {
            let all_left: Vec<_> = lg
                .iter()
                .filter(|((_, k), _)| k == &key)
                .flat_map(|(_, v)| v.iter())
                .collect();
            let all_right: Vec<_> = rg
                .iter()
                .filter(|((_, k), _)| k == &key)
                .flat_map(|(_, v)| v.iter())
                .collect();
            let collision =
                json!({"reason":"KEY_COLLISION","path":path,"left":all_left,"right":all_right});
            collisions.push(collision.clone());
            unknown.push(collision);
            continue;
        }
        if l.is_empty() || r.is_empty() {
            if presence_known {
                differences
                    .push(json!({"path":path,"field":"presence","left":l.len(),"right":r.len()}));
            } else {
                unknown.push(json!({"reason":"PRESENCE_UNKNOWN","path":path,"left_count":l.len(),"right_count":r.len()}));
            }
            continue;
        }
        if class == 8 {
            unknown.push(json!({"reason":"UNKNOWN_CLASS","path":path}));
            continue;
        }
        let (a, b) = (l[0], r[0]);
        for (side, o) in [("left", a), ("right", b)] {
            if o.lifecycle >= 5 || o.init_evidence == 0 || o.origin == "UNSET" {
                unknown.push(json!({"reason":"OBJECT_CONTINUITY_UNAVAILABLE","path":path,"side":side,"lifecycle":o.lifecycle,"init_evidence":o.init_evidence}));
            }
        }
        // 出生描述只在双方有真实出生时比较；FIRST_OBSERVED 的冻结 ordinal 不补造出生。
        if a.birth_event.is_some()
            && b.birth_event.is_some()
            && serde_json::to_value(&a.birth).map_err(error)?
                != serde_json::to_value(&b.birth).map_err(error)?
        {
            differences.push(json!({"path":path,"field":"birth","left":a.birth,"right":b.birth}));
        }
        if a.grid_index != b.grid_index {
            differences.push(
                json!({"path":path,"field":"grid_index","left":a.grid_index,"right":b.grid_index}),
            );
        }
        let af: BTreeMap<_, _> = a.fields.iter().map(|f| (f.field_id.clone(), f)).collect();
        let bf: BTreeMap<_, _> = b.fields.iter().map(|f| (f.field_id.clone(), f)).collect();
        let before = differences.len();
        for id in required(CLASSES[class]) {
            let id_text = format!("{id:04x}");
            let av = af
                .get(&id_text)
                .map(|field| comparable(field, &lk))
                .transpose()?
                .flatten();
            let bv = bf
                .get(&id_text)
                .map(|field| comparable(field, &rk))
                .transpose()?
                .flatten();
            if av.is_some() && bv.is_some() {
                known_fields += 1;
            }
            match (av,bv) {
                (Some(av),Some(bv)) if av!=bv=>differences.push(json!({"path":[left.run_epoch,left.room_epoch,left.authority_tick,CLASSES[class],key,field_name(id)],"field_id":id_text,"left":av,"right":bv})),
                (Some(_),Some(_))=>{},
                _=>unknown.push(json!({"reason":"FIELD_UNAVAILABLE","path":path,"field_id":id_text,"left":af.get(&id_text),"right":bf.get(&id_text)})),
            }
        }
        let mut am = a.memberships.clone();
        let mut bm = b.memberships.clone();
        am.sort();
        bm.sort();
        if am != bm && left.complete && right.complete {
            differences.push(json!({"path":path,"field":"membership","left":am,"right":bm}));
        }
        domains.push(json!({"class":CLASSES[class],"match_key":key,"known_field_differences":differences.len()-before}));
    }
    let status = if !differences.is_empty() {
        "DIFFERENT"
    } else if unknown.is_empty() {
        "EQUAL_WITHIN_PROFILE"
    } else {
        "INCOMPARABLE"
    };
    let first = differences.first().cloned();
    Ok(
        json!({"record":"comparison","status":status,"left_tick":left.authority_tick,"right_tick":right.authority_tick,
        "first_observed_difference":first,"earlier_coverage_unknown":earlier || !unknown.is_empty(),"known_field_comparisons":known_fields,"differences":differences,"unknown":unknown,"collisions":collisions,"domains":domains}),
    )
}
