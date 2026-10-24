//! T0-06 owned world values and offline comparison. No target process access or
//! digest path is reachable from replay/compare.
use crate::error::{AppError, AppResult};
use serde::{Deserialize, Serialize};
use serde_json::{Value, json};
use std::collections::{BTreeMap, BTreeSet};
use std::fs::{self, File, OpenOptions};
use std::io::{BufRead, BufReader, BufWriter, Read, Seek, SeekFrom, Write};
use std::path::{Path, PathBuf};
use std::time::Instant;

pub const DOMAIN: &str = "isaac.cs.t0.world-record";
pub const PROFILE: &str = "min-read-v1";
pub const CLASSES: [&str; 9] = ["Player", "NPC", "Tear", "Projectile", "Pickup", "Familiar", "Effect", "Grid", "UnknownEntity"];
pub const SOURCES: [&str; 15] = ["main", "persistent", "update", "render", "effect", "temp", "bufferTemplate", "wisp", "roster", "esau", "player_relations", "grid", "door", "reserved_13", "reserved_14"];
pub const REASONS: [&str; 21] = ["NONE", "PHASE_INCOMPLETE", "ODD_PLAYER_ONLY", "LIFECYCLE", "READ_FAILED", "BUDGET_EXCEEDED", "RELATION_BUDGET_EXCEEDED", "BUFFER_BACKPRESSURE", "UNKNOWN_CONTINUITY", "WRONG_THREAD", "NONFINITE", "SEQUENCE_EXHAUSTED", "ROOT_CHANGED", "EVENT_OVERFLOW", "STOPPED", "UNKNOWN_CLASS", "NOT_READY", "SOURCE_CHANGED", "UNPAIRED_SCOPE", "INVALID", "INVALID"];
pub const HOOKS: [&str; 19] = ["Factory", "Init", "Publish", "Remove", "ClearReferences", "Destroy", "PlayerCtor", "PlayerCreate", "ManagerRestore", "Replace", "PlayerRestore", "NpcMorph", "PickupMorph", "GridFactory", "GridInit", "GridSpawn", "Door", "GridFree", "Spawn"];
fn error(e: impl std::fmt::Display) -> AppError { AppError::evidence(e.to_string()) }
fn decimal(value: &str) -> AppResult<u64> {
    let parsed = value.parse::<u64>().map_err(error)?;
    if parsed.to_string() != value { return Err(AppError::usage("u64 values require canonical decimal strings")); }
    Ok(parsed)
}
pub fn new_log(path: &Path) -> AppResult<File> {
    OpenOptions::new().create_new(true).write(true).open(path).map_err(error)
}
pub fn line(file: &mut impl Write, value: &impl Serialize) -> AppResult<()> {
    serde_json::to_writer(&mut *file, value).map_err(error)?;
    file.write_all(b"\n").map_err(error)
}
pub fn save(path: &Path, value: &impl Serialize) -> AppResult<()> {
    let mut file = new_log(path)?;
    serde_json::to_writer_pretty(&mut file, value).map_err(error)?;
    file.write_all(b"\n").map_err(error)
}
fn reason(value: u32) -> &'static str { REASONS.get(value as usize).copied().unwrap_or("UNKNOWN_ERROR") }
fn field_name(id: u16) -> &'static str {
    const COMMON: [&str; 30] = ["index", "type", "variant", "subtype", "flags", "valid", "visible", "exists", "dead", "removed_by_factory", "position.x", "position.y", "velocity.x", "velocity.y", "hp", "max_hp", "init_seed", "drop_rng.word0", "drop_rng.word1", "drop_rng.word2", "drop_rng.word3", "parent", "child", "target", "spawner", "spawner_type", "spawner_variant", "birth_counter", "game_frame_counter", "frame_count"];
    const PLAYER: [&str; 16] = ["player_type", "player_index", "damage_cooldown", "max_hearts", "red_hearts", "eternal_hearts", "soul_hearts", "black_hearts", "bone_hearts", "golden_hearts", "broken_hearts", "rotten_hearts", "sub_player", "twin", "backup", "replaced"];
    const NPC: [&str; 4] = ["state_frame", "state", "projectile_cooldown", "projectile_delay"];
    const FAMILIAR: [&str; 3] = ["player", "fire_cooldown", "state"];
    const EFFECT: [&str; 3] = ["state", "timeout", "lifespan"];
    const PICKUP: [&str; 3] = ["timeout", "wait", "state"];
    const GRID: [&str; 17] = ["type", "variant", "state", "spawn_count", "spawn_seed", "var_data", "initialized", "variable_seed", "grid_index", "spawn_frame", "rng.word0", "rng.word1", "rng.word2", "rng.word3", "collision_class", "position.x", "position.y"];
    match id {
        1..=0x1e => COMMON[(id-1) as usize], 0x101..=0x110 => PLAYER[(id-0x101) as usize],
        0x201..=0x204 => NPC[(id-0x201) as usize], 0x301..=0x303 => FAMILIAR[(id-0x301) as usize],
        0x401..=0x403 => EFFECT[(id-0x401) as usize], 0x501..=0x503 => PICKUP[(id-0x501) as usize],
        0x601 => "tear_flags", 0x701 => "projectile_flags", 0x801..=0x811 => GRID[(id-0x801) as usize], _ => "unknown_field",
    }
}
fn required(class: &str) -> Vec<u16> {
    let mut ids = if class == "Grid" { (0x801..=0x811).collect() } else if class == "UnknownEntity" { Vec::new() } else { (1..=0x1e).collect() };
    let (first, last) = match class { "Player" => (0x101,0x110), "NPC" => (0x201,0x204), "Familiar" => (0x301,0x303), "Effect" => (0x401,0x403), "Pickup" => (0x501,0x503), "Tear" => (0x601,0x601), "Projectile" => (0x701,0x701), _ => (1,0) };
    ids.extend(first..=last); ids
}
fn expected_field(id: u16) -> (u32, &'static str) {
    match id {
        0x0005 | 0x0701 => (8,"RAW_LE"), 0x0601 => (16,"RAW_LE"),
        0x0006..=0x000a | 0x0807 => (1,"RAW_LE"),
        0x000b..=0x0010 | 0x0810..=0x0811 => (4,"F32_BITS"),
        0x0016..=0x0019 | 0x010d..=0x0110 | 0x0301 => (4,"REFERENCE"),
        0x001e => (4,"DERIVED_RAW32"), _ => (4,"RAW_LE"),
    }
}
pub fn manifest(session: &str) -> Value {
    let profiles: BTreeMap<_,_> = CLASSES.iter().map(|class| (*class, required(class).iter().map(|id|format!("{id:04x}")).collect::<Vec<_>>())).collect();
    let fields: Vec<_> = CLASSES.iter().flat_map(|class| required(class)).collect::<BTreeSet<_>>().into_iter().map(|id| {
        let (width, encoding) = expected_field(id);
        json!({"field_id":format!("{id:04x}"),"name":field_name(id),"width":width,"encoding":encoding})
    }).collect();
    json!({"domain":DOMAIN,"version":1,"profile":PROFILE,"session":session,
        "phase":"POST_STANDARD_UPDATE_PRE_TOP_EXIT","classes":CLASSES,"sources":SOURCES,
        "fields":fields,"required":profiles,"float_comparison":"finite exact bits; normalize signed zero; retain raw bits",
        "hp_evidence":"USER_ACCEPTED_ASSUMPTION: generic Entity HP represents life; Player hearts remain separate",
        "unconfirmed":["Player.State","Player.Timeout","Tear.State","Tear.Timeout","Projectile.State","Projectile.Timeout"],
        "native_diagnostics_excluded":["address","vptr","thread","container_descriptor","wall_clock"],
        "budgets":{"objects_per_batch":16384,"memberships_per_batch":65536,"player_relation_nodes":64,"batches":3,"identities":32768,"events":65536},
        "readiness_evidence":{"0":"NO_OBSERVED_INIT","1":"BASE_INIT_RETURNED","2":"KNOWN_CREATION_OWNER_COMPLETED","3":"USER_ACCEPTED_CURRENT_PLAYER_UPDATE_WITNESS"},
        "current_player_read_rule":"exact pair remains valid across a normal native PlayerUpdate in an Authority simulation window; FIRST_OBSERVED and missing birth history remain unchanged",
        "event_kinds":{"0":"NATIVE_HOOK","1":"BIRTH_CONFIRMED","2":"LIFECYCLE_BOUNDARY","3":"GAP_OR_UNKNOWN","4":"GRID_PHYSICAL_RETIRED","6":"CURRENT_PLAYER_READ_WITNESS","7":"SOURCE_MEMBERSHIP_SNAPSHOT"},
        "hook_slots":70,"runtime_acceptance":"NOT_RUN","hashes":"NOT_USED"})
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct LocalTarget { pub local_id: String, pub use_generation: String }
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Reference {
    pub status: String,
    #[serde(skip_serializing_if="Option::is_none")] pub target: Option<LocalTarget>,
}
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Field {
    pub field_id: String, pub status: String, pub encoding: String, pub width: u32,
    #[serde(skip_serializing_if="Option::is_none")] pub raw32: Option<String>,
    #[serde(skip_serializing_if="Option::is_none")] pub raw64: Option<String>,
    #[serde(skip_serializing_if="Option::is_none")] pub bytes_le: Option<String>,
    #[serde(skip_serializing_if="Option::is_none")] pub reference: Option<Reference>,
    #[serde(skip_serializing_if="Option::is_none")] pub diagnostics: Option<Value>,
}
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Birth { pub kind: u32, pub entity_type: u32, pub variant: u32, pub subtype: u32, pub seed: u32 }
#[derive(Clone, Debug, Serialize, Deserialize, PartialEq, Eq, PartialOrd, Ord)]
#[serde(deny_unknown_fields)]
pub struct Membership { pub source: u32, pub index: u32 }
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Object {
    pub local_id: String, pub use_generation: String, pub revision: String, pub object_domain: u32,
    pub class: String, pub origin: String, pub lifecycle: u32, pub init_evidence: u32,
    pub birth_event: Option<String>, pub birth: Birth, pub ordinal: Option<u32>,
    pub fields: Vec<Field>, pub memberships: Vec<Membership>, pub diagnostics: Value,
}
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Source { pub source: u32, pub status: String, pub count: u32, pub diagnostics: Value }
#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Capture {
    pub record: String, pub domain: String, pub version: u32, pub profile: String, pub session: String,
    pub run_epoch: String, pub room_epoch: String, pub mutation_epoch: String, pub attempt_seq: String,
    pub capture_seq: Option<String>, pub outer_token: String, pub phase: String,
    pub authority_tick: Option<String>, pub authority_kind: u32, pub scope_token: String,
    pub complete: bool, pub reason: String, pub sources: Vec<Source>, pub objects: Vec<Object>,
    pub required_objects: u32, pub required_memberships: u32, pub diagnostics: Value,
}
fn field_id(field: &Field) -> AppResult<u16> {
    let id = u16::from_str_radix(&field.field_id,16).map_err(error)?;
    if format!("{id:04x}") != field.field_id { return Err(AppError::usage("field_id requires four lowercase hex digits")); }
    Ok(id)
}
fn raw_bits(text: &str, digits: usize) -> AppResult<u64> {
    if text.len()!=digits || !text.bytes().all(|c|c.is_ascii_digit() || (b'a'..=b'f').contains(&c)) {return Err(AppError::usage("invalid fixed-width raw bit string"));}
    u64::from_str_radix(text,16).map_err(error)
}
fn validate(record: &Capture) -> AppResult<()> {
    if record.domain!=DOMAIN || record.version!=1 || record.profile!=PROFILE ||
       !["capture","gap"].contains(&record.record.as_str()) || record.objects.len()>16384 || record.sources.len()>15 {
        return Err(AppError::evidence("unsupported or over-budget world record"));
    }
    for value in [&record.run_epoch,&record.room_epoch,&record.mutation_epoch,&record.attempt_seq,&record.outer_token,&record.scope_token] {decimal(value)?;}
    if let Some(value)=&record.capture_seq {decimal(value)?;}
    if let Some(value)=&record.authority_tick {decimal(value)?;}
    let mut identities=BTreeSet::new();let mut memberships=0usize;
    for object in &record.objects {
        decimal(&object.local_id)?;decimal(&object.use_generation)?;decimal(&object.revision)?;
        if let Some(value)=&object.birth_event {decimal(value)?;}
        if !identities.insert((object.object_domain,&object.local_id,&object.use_generation)) ||
            !CLASSES.contains(&object.class.as_str()) || object.fields.len()>52 || object.object_domain>1 || object.ordinal.is_some_and(|n|n>1) {
            return Err(AppError::evidence("invalid world object identity/class/budget"));
        }
        memberships=memberships.checked_add(object.memberships.len()).ok_or_else(||error("membership count overflow"))?;
        if memberships>65536 || object.memberships.iter().any(|m|m.source>=15) {return Err(error("invalid membership budget/source"));}
        let mut ids=BTreeSet::new();
        let allowed=if object.class=="UnknownEntity"{vec![1,2,3,4]}else{required(&object.class)};
        for field in &object.fields {
            let id=field_id(field)?;let (width,encoding)=expected_field(id);
            if !ids.insert(id) || !allowed.contains(&id) || field_name(id)=="unknown_field" || field.width!=width || field.encoding!=encoding ||
                !["VALUE","UNCONFIRMED","NOT_APPLICABLE","NOT_READY","READ_FAILED"].contains(&field.status.as_str()) {
                return Err(error("invalid field schema"));
            }
            if field.status!="VALUE" {
                if field.raw32.is_some() || field.raw64.is_some() || field.bytes_le.is_some() || field.reference.is_some() {return Err(error("unavailable field has a value"));}
                continue;
            }
            if encoding=="REFERENCE" {
                let reference=field.reference.as_ref().ok_or_else(||error("missing reference state"))?;
                if !["NULL","KNOWN_CURRENT","UNRESOLVED","RETIRED","AMBIGUOUS_REUSE"].contains(&reference.status.as_str()) ||
                    (reference.status=="KNOWN_CURRENT")!=reference.target.is_some() || field.raw32.is_some() || field.raw64.is_some() || field.bytes_le.is_some() {
                    return Err(error("invalid reference payload"));
                }
                if let Some(target)=&reference.target {decimal(&target.local_id)?;decimal(&target.use_generation)?;}
            } else if width==4 {
                if field.raw64.is_some() || field.bytes_le.is_some() || field.reference.is_some(){return Err(error("extra raw32 payload"));}
                raw_bits(field.raw32.as_deref().ok_or_else(||error("missing raw32"))?,8)?;
            }
            else if width==8 {
                if field.raw32.is_some() || field.bytes_le.is_some() || field.reference.is_some(){return Err(error("extra raw64 payload"));}
                raw_bits(field.raw64.as_deref().ok_or_else(||error("missing raw64"))?,16)?;
            }
            else {
                if field.raw32.is_some() || field.raw64.is_some() || field.reference.is_some(){return Err(error("extra byte payload"));}
                let bytes=field.bytes_le.as_deref().ok_or_else(||error("missing bytes_le"))?;
                if bytes.len()!=width as usize*2 || !bytes.bytes().all(|c|c.is_ascii_digit() || (b'a'..=b'f').contains(&c)) {return Err(error("invalid byte string"));}
            }
        }
    }
    if record.complete && (record.required_objects!=record.objects.len() as u32 || record.required_memberships!=memberships_count(record)) {
        return Err(error("complete record has inconsistent object/membership counts"));
    }
    Ok(())
}
fn memberships_count(record:&Capture)->u32 {record.objects.iter().map(|object|object.memberships.len() as u32).sum()}
fn canonicalize(record: &mut Capture) {
    for object in &mut record.objects {object.fields.sort_by(|a,b|a.field_id.cmp(&b.field_id));object.memberships.sort();}
    record.sources.sort_by_key(|s|s.source);
    // This is a local replay order only. No address becomes a cross-process key.
    record.objects.sort_by_key(|o| (CLASSES.iter().position(|c|*c==o.class).unwrap_or(8),decimal(&o.local_id).unwrap_or(0),decimal(&o.use_generation).unwrap_or(0)));
}
const MAX_RECORD: usize=128*1024*1024;
fn read_line_bounded(reader: &mut impl BufRead, out: &mut Vec<u8>) -> AppResult<usize> {
    out.clear();
    loop {
        let buffer=reader.fill_buf().map_err(error)?;
        if buffer.is_empty(){return Ok(out.len());}
        let take=buffer.iter().position(|c|*c==b'\n').map_or(buffer.len(),|n|n+1);
        if take>MAX_RECORD-out.len(){return Err(error("world JSONL record exceeds 128 MiB"));}
        let done=buffer[take-1]==b'\n';out.extend_from_slice(&buffer[..take]);reader.consume(take);
        if done{return Ok(out.len());}
    }
}
fn parse_record(bytes: &[u8]) -> AppResult<Capture> {
    let value: Capture=serde_json::from_slice(bytes).map_err(error)?;validate(&value)?;Ok(value)
}
fn read_manifest(directory: &Path) -> AppResult<Value> {
    let value: Value=serde_json::from_reader(File::open(directory.join("manifest.json")).map_err(error)?).map_err(error)?;
    if value["domain"]!=DOMAIN || value["version"]!=1 || value["profile"]!=PROFILE {return Err(error("world manifest schema mismatch"));}
    let expected=manifest("");
    for key in ["fields","required","classes","sources","phase","hp_evidence","readiness_evidence","current_player_read_rule"] {
        if value[key]!=expected[key]{return Err(error(format!("world manifest differs at {key}")));}
    }
    Ok(value)
}

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Scope {run_epoch:String,room_epoch:String}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct TickPair {left:String,right:String}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct BirthPair {left:String,right:String}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct PlayerPair {left:u32,right:u32}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Alignment {version:u32,left:Scope,right:Scope,ticks:Vec<TickPair>,births:Vec<BirthPair>,players:Vec<PlayerPair>}
struct Location {offset:u64,length:usize,earlier_unknown:bool,duplicate:bool}
fn index(directory: &Path, scope: &Scope, session: &str) -> AppResult<BTreeMap<u64,Location>> {
    decimal(&scope.run_epoch)?;decimal(&scope.room_epoch)?;
    let mut reader=BufReader::new(File::open(directory.join("captures.jsonl")).map_err(error)?);
    let mut bytes=Vec::new();let mut offset=0u64;let mut unknown=false;let mut output=BTreeMap::new();
    let mut previous_attempt=0;let mut previous_capture=0;
    loop {
        let length=read_line_bounded(&mut reader,&mut bytes)?;if length==0{break;}
        let record=parse_record(&bytes)?;
        if record.session!=session {return Err(error("record/manifest session mismatch"));}
        let attempt=decimal(&record.attempt_seq)?;
        if attempt<=previous_attempt{return Err(error("non-increasing attempt sequence"));}
        if attempt!=previous_attempt+1 {unknown=true;}previous_attempt=attempt;
        if let Some(value)=&record.capture_seq {let sequence=decimal(value)?;if sequence<=previous_capture{return Err(error("non-increasing capture sequence"));}previous_capture=sequence;}
        if record.run_epoch==scope.run_epoch && record.room_epoch==scope.room_epoch {
            if let Some(tick)=&record.authority_tick {
                let tick=decimal(tick)?;
                match output.entry(tick) {
                    std::collections::btree_map::Entry::Vacant(slot)=>{slot.insert(Location{offset,length,earlier_unknown:unknown,duplicate:false});},
                    std::collections::btree_map::Entry::Occupied(mut slot)=>{slot.get_mut().duplicate=true;unknown=true;},
                }
            } else {unknown=true;}
            if !record.complete || record.record!="capture" {unknown=true;}
        }
        offset=offset.checked_add(length as u64).ok_or_else(||error("capture offset overflow"))?;
    }
    Ok(output)
}
fn at(file: &mut File, location: &Location) -> AppResult<Capture> {
    file.seek(SeekFrom::Start(location.offset)).map_err(error)?;
    let mut bytes=vec![0;location.length];file.read_exact(&mut bytes).map_err(error)?;parse_record(&bytes)
}
fn object_key(object: &Object,births: &BTreeMap<u64,usize>,players: &BTreeMap<u32,usize>) -> Option<String> {
    if object.class=="Player" {
        if let Some(ordinal)=object.ordinal {if let Some(mapped)=players.get(&ordinal){return Some(format!("player:{mapped:02}"));}}
    }
    let birth=decimal(object.birth_event.as_deref()?).ok()?;
    let mapped=births.get(&birth)?;
    Some(format!("birth:{mapped:020}:{}:{}:{}:{}:{}:{}",object.object_domain,object.birth.kind,
        object.birth.entity_type,object.birth.variant,object.birth.subtype,object.birth.seed))
}
type Groups<'a>=BTreeMap<(usize,String),Vec<&'a Object>>;
type LocalKeys=BTreeMap<(String,String),Option<String>>;
fn group<'a>(capture: &'a Capture,births: &BTreeMap<u64,usize>,players: &BTreeMap<u32,usize>,unknown: &mut Vec<Value>) -> (Groups<'a>,LocalKeys) {
    let mut groups=Groups::new();let mut local=LocalKeys::new();
    for object in &capture.objects {
        let key=object_key(object,births,players);
        local.insert((object.local_id.clone(),object.use_generation.clone()),key.clone());
        if let Some(key)=key {
            let class=CLASSES.iter().position(|c|*c==object.class).unwrap_or(8);
            groups.entry((class,key)).or_default().push(object);
        } else {unknown.push(json!({"reason":"IDENTITY_UNRESOLVED","class":object.class,"local_id":object.local_id,"use_generation":object.use_generation}));}
    }
    // References to a collision group cannot acquire a seemingly unique key.
    for values in groups.values().filter(|values|values.len()!=1) {
        for object in values {local.insert((object.local_id.clone(),object.use_generation.clone()),None);}
    }
    (groups,local)
}
fn comparable(field: &Field, keys: &LocalKeys) -> AppResult<Option<Value>> {
    if field.status!="VALUE"{return Ok(None);}
    if field.encoding=="REFERENCE" {
        let reference=field.reference.as_ref().ok_or_else(||error("missing reference"))?;
        if reference.status=="NULL" {return Ok(Some(json!({"relation":"NULL"})));}
        if reference.status!="KNOWN_CURRENT"{return Ok(None);}
        let target=reference.target.as_ref().ok_or_else(||error("missing reference target"))?;
        return Ok(keys.get(&(target.local_id.clone(),target.use_generation.clone())).and_then(|key|key.as_ref()).map(|key|json!({"relation":"KNOWN_CURRENT","target":key})));
    }
    if field.encoding=="F32_BITS" {
        let bits=raw_bits(field.raw32.as_deref().ok_or_else(||error("missing float bits"))?,8)? as u32;
        if bits&0x7f800000==0x7f800000{return Ok(None);}
        return Ok(Some(json!({"raw32":format!("{:08x}",if bits&0x7fffffff==0{0}else{bits})})));
    }
    Ok(Some(json!({"raw32":field.raw32,"raw64":field.raw64,"bytes_le":field.bytes_le})))
}
fn compare_pair(left: &Capture,right: &Capture,lb: &BTreeMap<u64,usize>,rb: &BTreeMap<u64,usize>,
    lp: &BTreeMap<u32,usize>,rp: &BTreeMap<u32,usize>,earlier:bool) -> AppResult<Value> {
    let mut unknown=Vec::new();let mut differences=Vec::new();let mut collisions=Vec::new();
    if left.phase!="POST_STANDARD_UPDATE_PRE_TOP_EXIT" || right.phase!=left.phase || left.authority_kind!=1 || right.authority_kind!=1 || left.record!="capture" || right.record!="capture" {
        return Ok(json!({"record":"comparison","status":"INCOMPARABLE","reason":"ALIGNMENT_UNAVAILABLE","left_tick":left.authority_tick,"right_tick":right.authority_tick,"earlier_coverage_unknown":true}));
    }
    if !left.complete || !right.complete {unknown.push(json!({"reason":"INCOMPLETE","left":left.reason,"right":right.reason}));}
    for (side,capture) in [("left",left),("right",right)] {
        let sources:BTreeMap<_,_>=capture.sources.iter().map(|source|(source.source,source)).collect();
        for id in 0..13 {
            if !sources.get(&id).is_some_and(|source|source.status=="NONE") {unknown.push(json!({"reason":"SOURCE_UNAVAILABLE","side":side,"source":id}));}
        }
    }
    let (lg,lk)=group(left,lb,lp,&mut unknown);let (rg,rk)=group(right,rb,rp,&mut unknown);
    let keys:BTreeSet<_>=lg.keys().chain(rg.keys()).cloned().collect();
    let mut domains=Vec::new();
    for (class,key) in keys {
        let l=lg.get(&(class,key.clone())).map_or(&[][..],Vec::as_slice);
        let r=rg.get(&(class,key.clone())).map_or(&[][..],Vec::as_slice);
        let path=json!(["run","room",CLASSES[class],key]);
        if l.len()>1 || r.len()>1 {
            let collision=json!({"reason":"KEY_COLLISION","path":path,"left":l,"right":r});
            collisions.push(collision.clone());unknown.push(collision);continue;
        }
        if l.is_empty() || r.is_empty() {
            if left.complete && right.complete && unknown.is_empty() {differences.push(json!({"path":path,"field":"presence","left":l.len(),"right":r.len()}));}
            else {unknown.push(json!({"reason":"PRESENCE_UNKNOWN","path":path,"left_count":l.len(),"right_count":r.len()}));}
            continue;
        }
        if class==8 {unknown.push(json!({"reason":"UNKNOWN_CLASS","path":path}));continue;}
        let (a,b)=(l[0],r[0]);
        let af:BTreeMap<_,_>=a.fields.iter().map(|f|(f.field_id.clone(),f)).collect();
        let bf:BTreeMap<_,_>=b.fields.iter().map(|f|(f.field_id.clone(),f)).collect();
        let before=differences.len();
        for id in required(CLASSES[class]) {
            let id_text=format!("{id:04x}");
            let av=af.get(&id_text).map(|field|comparable(field,&lk)).transpose()?.flatten();
            let bv=bf.get(&id_text).map(|field|comparable(field,&rk)).transpose()?.flatten();
            match (av,bv) {
                (Some(av),Some(bv)) if av!=bv=>differences.push(json!({"path":["run","room",CLASSES[class],key,field_name(id)],"field_id":id_text,"left":av,"right":bv})),
                (Some(_),Some(_))=>{},
                _=>unknown.push(json!({"reason":"FIELD_UNAVAILABLE","path":path,"field_id":id_text,"left":af.get(&id_text),"right":bf.get(&id_text)})),
            }
        }
        let mut am=a.memberships.clone();let mut bm=b.memberships.clone();am.sort();bm.sort();
        if am!=bm && left.complete && right.complete {differences.push(json!({"path":["run","room",CLASSES[class],key,"membership"],"left":am,"right":bm}));}
        domains.push(json!({"class":CLASSES[class],"match_key":key,"known_field_differences":differences.len()-before}));
    }
    let status=if !differences.is_empty(){"DIFFERENT"}else if unknown.is_empty(){"EQUAL_WITHIN_PROFILE"}else{"INCOMPARABLE"};
    let first=differences.first().cloned();
    Ok(json!({"record":"comparison","status":status,"left_tick":left.authority_tick,"right_tick":right.authority_tick,
        "first_observed_difference":first,"earlier_coverage_unknown":earlier || !unknown.is_empty(),"differences":differences,"unknown":unknown,"collisions":collisions,"domains":domains}))
}

pub fn run_from_env() -> AppResult<()> {
    let command=std::env::args().nth(1).unwrap_or_default();
    let args:Vec<_>=std::env::args().skip(2).collect();
    if args==["--help"] {
        println!("world-replay --capture <world-dir> --artifact-dir <new-dir>\nworld-compare --left <world-dir> --right <world-dir> --alignment <json> --artifact-dir <new-dir>");return Ok(());
    }
    let mut options=BTreeMap::new();
    for pair in args.chunks(2) {
        if pair.len()!=2 || !pair[0].starts_with("--") || options.insert(pair[0].clone(),pair[1].clone()).is_some(){return Err(AppError::usage("expected unique --option value pairs"));}
    }
    let required=if command=="world-replay"{vec!["--capture","--artifact-dir"]}else{vec!["--left","--right","--alignment","--artifact-dir"]};
    if options.len()!=required.len() || required.iter().any(|key|!options.contains_key(*key)){return Err(AppError::usage("use --help for world command arguments"));}
    let directory=PathBuf::from(&options["--artifact-dir"]);fs::create_dir(&directory).map_err(error)?;
    save(&directory.join("invocation.json"),&json!({"command":command,"arguments":options,"game_operations":false,"hashes":"NOT_USED"}))?;
    let result=if command=="world-replay"{replay(Path::new(&options["--capture"]),&directory)}else{compare(&options,&directory)};
    if let Err(failure)=&result {save(&directory.join("failure.json"),&json!({"status":"FAIL","error":failure.to_string(),"automatic_retry":false}))?;}
    result
}
const REPLAY_IO_BUFFER: usize = 1024 * 1024;
// Count writes reaching File, rather than serde's many tiny writes into the buffer.
struct ReplayFile { file: File, writes: u64, bytes: u64 }
impl Write for ReplayFile {
    fn write(&mut self, bytes: &[u8]) -> std::io::Result<usize> {
        self.writes += 1;
        let written = self.file.write(bytes)?;
        self.bytes += written as u64;
        Ok(written)
    }
    fn flush(&mut self) -> std::io::Result<()> { self.file.flush() }
}
fn replay(source: &Path,directory: &Path) -> AppResult<()> {
    let started = Instant::now();
    let source_manifest=read_manifest(source)?;save(&directory.join("manifest.json"),&source_manifest)?;
    let source_file=File::open(source.join("captures.jsonl")).map_err(error)?;
    let input_bytes=source_file.metadata().map_err(error)?.len();
    let mut input=BufReader::with_capacity(REPLAY_IO_BUFFER,source_file);
    let sink=ReplayFile { file:new_log(&directory.join("captures.jsonl"))?, writes:0, bytes:0 };
    let mut output=BufWriter::with_capacity(REPLAY_IO_BUFFER,sink);let mut bytes=Vec::new();let mut count=0u64;
    while read_line_bounded(&mut input,&mut bytes)?!=0 {
        let mut record=parse_record(&bytes)?;
        if record.session!=source_manifest["session"].as_str().unwrap_or(""){return Err(error("record/manifest session mismatch"));}
        canonicalize(&mut record);line(&mut output,&record)?;count+=1;
    }
    // Drop cannot report a flush error. Never publish success with buffered bytes outstanding.
    output.flush().map_err(error)?;
    save(&directory.join("result.json"),&json!({"status":"REENCODED","records":count.to_string(),"source_unchanged":true,"runtime_acceptance":"NOT_RUN",
        "io":{"buffer_bytes":REPLAY_IO_BUFFER,"input_bytes":input_bytes.to_string(),"output_bytes":output.get_ref().bytes.to_string(),
            "file_write_calls":output.get_ref().writes.to_string(),"elapsed_ms":started.elapsed().as_millis().to_string(),"flushed":true}}))
}
fn compare(options: &BTreeMap<String,String>,directory: &Path) -> AppResult<()> {
    let left=Path::new(&options["--left"]);let right=Path::new(&options["--right"]);
    let lm=read_manifest(left)?;let rm=read_manifest(right)?;
    let alignment_bytes=fs::read(&options["--alignment"]).map_err(error)?;
    let alignment:Alignment=serde_json::from_slice(&alignment_bytes).map_err(error)?;
    if alignment.version!=1 || alignment.ticks.is_empty() {return Err(error("alignment version 1 and explicit tick pairs are required"));}
    let mut last=(None,None);
    for pair in &alignment.ticks {
        let values=(decimal(&pair.left)?,decimal(&pair.right)?);
        if last.0.is_some_and(|n|n>=values.0) || last.1.is_some_and(|n|n>=values.1){return Err(error("tick pairs must increase strictly on both sides"));}
        last=(Some(values.0),Some(values.1));
    }
    let mut lb=BTreeMap::new();let mut rb=BTreeMap::new();
    for (i,pair) in alignment.births.iter().enumerate() {
        if lb.insert(decimal(&pair.left)?,i).is_some() || rb.insert(decimal(&pair.right)?,i).is_some(){return Err(error("birth mapping is not one-to-one"));}
    }
    let mut lp=BTreeMap::new();let mut rp=BTreeMap::new();
    for (i,pair) in alignment.players.iter().enumerate() {
        if pair.left>1 || pair.right>1 || lp.insert(pair.left,i).is_some() || rp.insert(pair.right,i).is_some(){return Err(error("only one-to-one frozen A/B ordinal mapping is supported"));}
    }
    let li=index(left,&alignment.left,lm["session"].as_str().ok_or_else(||error("left manifest has no session"))?)?;
    let ri=index(right,&alignment.right,rm["session"].as_str().ok_or_else(||error("right manifest has no session"))?)?;
    fs::write(directory.join("alignment.json"),alignment_bytes).map_err(error)?;
    save(&directory.join("left-manifest.json"),&lm)?;save(&directory.join("right-manifest.json"),&rm)?;
    let mut lf=File::open(left.join("captures.jsonl")).map_err(error)?;let mut rf=File::open(right.join("captures.jsonl")).map_err(error)?;
    let mut output=new_log(&directory.join("comparisons.jsonl"))?;let mut collision_log=new_log(&directory.join("key-collisions.jsonl"))?;
    let mut earlier=false;let mut different=false;let mut incomparable=false;let mut first=None;
    for pair in &alignment.ticks {
        let l=li.get(&decimal(&pair.left)?);let r=ri.get(&decimal(&pair.right)?);
        let row=if let (Some(l),Some(r))=(l,r) {
            earlier|=l.earlier_unknown || r.earlier_unknown;
            if l.duplicate || r.duplicate {json!({"record":"comparison","status":"INCOMPARABLE","reason":"ALIGNMENT_AMBIGUOUS","left_tick":pair.left,"right_tick":pair.right,"earlier_coverage_unknown":true})}
            else {compare_pair(&at(&mut lf,l)?,&at(&mut rf,r)?,&lb,&rb,&lp,&rp,earlier)?}
        } else {json!({"record":"comparison","status":"INCOMPARABLE","reason":"SAMPLE_GAP","left_tick":pair.left,"right_tick":pair.right,"earlier_coverage_unknown":true})};
        if row["status"]=="DIFFERENT" {different=true;if first.is_none(){first=Some(row.clone());}}
        if row["status"]=="INCOMPARABLE" || row["unknown"].as_array().is_some_and(|a|!a.is_empty()){incomparable=true;earlier=true;}
        if let Some(collisions)=row["collisions"].as_array(){for collision in collisions {line(&mut collision_log,collision)?;}}
        line(&mut output,&row)?;
    }
    let status=if different{"DIFFERENT"}else if incomparable{"INCOMPARABLE"}else{"EQUAL_WITHIN_PROFILE"};
    save(&directory.join("result.json"),&json!({"status":status,"domain":DOMAIN,"profile":PROFILE,
        "scope":"only explicitly paired ticks and mapped objects","tick_pairs":alignment.ticks.len(),
        "first_observed_difference":first,"uncompared_regions":incomparable,"runtime_acceptance":"NOT_RUN"}))
}

#[cfg(windows)]
#[path = "world_record_live.rs"]
pub mod live;
