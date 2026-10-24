//! Isolated resident virtual-device controller. No legacy digest/preflight path.
use crate::error::{AppError, AppResult};
use serde::{Deserialize, Serialize};
use serde_json::json;
use std::collections::{BTreeMap,BTreeSet};
use std::fs::{self, File, OpenOptions};
use std::io::{BufRead, Write};
use std::path::{Path, PathBuf};

pub fn run_input_config_from_env() -> AppResult<()> {
    let arguments: Vec<String> = std::env::args().skip(2).collect();
    if arguments==["--help"] {
        println!("coop-input-config [--manifest <json>] [--input-lead-ticks N] [--future-window-ticks N] [--max-input-age-ticks N] [--script-drain-budget N] [--admission-attempt-budget N] --output <new-json>");
        return Ok(());
    }
    let mut options=BTreeMap::new();
    for pair in arguments.chunks(2) {
        let Some(key)=pair.first() else {return Err(AppError::usage("missing option name"));};
        if pair.len()!=2 {return Err(AppError::usage(format!("option {key} requires one value")));}
        if !key.starts_with("--") {return Err(AppError::usage(format!("unexpected argument {key}")));}
        if options.insert(key.to_owned(),pair[1].clone()).is_some() {
            return Err(AppError::usage(format!("duplicate option {key}")));
        }
    }
    let allowed=["--manifest","--output",INPUT_LEAD_FLAG,FUTURE_WINDOW_FLAG,MAX_INPUT_AGE_FLAG,
        SCRIPT_DRAIN_FLAG,ADMISSION_BUDGET_FLAG];
    if let Some(key)=options.keys().find(|key|!allowed.contains(&key.as_str())) {
        return Err(AppError::usage(format!("unknown option {key}")));
    }
    if !options.contains_key("--output") {
        return Err(AppError::usage("coop-input-config requires --output"));
    }
    let resolved=resolve_input_config(options.get("--manifest").map(String::as_str),&options)?;
    let output=PathBuf::from(&options["--output"]);
    let mut record=resolved.record.clone();
    if let Some(object)=record.as_object_mut() {
        object.insert("tx".into(),json!("tx-01-96"));
        object.insert("command".into(),json!("coop-input-config"));
        object.insert("automaticRetry".into(),json!(false));
    }
    write_json(&output,&record)?;
    println!("{}",serde_json::to_string_pretty(&record).map_err(|e|AppError::evidence(e.to_string()))?);
    Ok(())
}
pub fn run_from_env() -> AppResult<()> {
    let arguments: Vec<String> = std::env::args().skip(2).collect();
    if arguments==["--list-characters"] {
        println!("native\tUse the native Join selection (default)");
        for (name,id) in CHARACTERS {println!("{id}\t{name}");}
        println!("Names or nonnegative numeric IDs are accepted; native verifies the current selectable roster.");
        return Ok(());
    }
    if arguments==["--help"] {
        println!("coop-active --game-exe <exe> --native-dll <dll> --helper <exe> --artifact-dir <new-dir> --script <json> --authorization <text> [--b-character <name|id|native>] [--input-config <json> | input config flags]");
        println!("Input config: --input-lead-ticks N --future-window-ticks N --max-input-age-ticks 0 --script-drain-budget N --admission-attempt-budget N.");
        println!("coop-active --list-characters\ncoop-active --describe-character <name|id|native>\nB is selected before Join; A keeps the character selected in the game.");
        println!("Optional --world-record min-read-v1 writes T0-06 owned world records under artifact-dir/world.");
        println!("Optional --protocol t0-v2 [--protocol-config JSON] uses independent wire, bounded in-memory faults and raw packet logs; requires world-record.");
        return Ok(());
    }
    if arguments.len()==2 && arguments[0]=="--describe-character" {
        println!("{}",character_choice(parse_character(&arguments[1])?));
        return Ok(());
    }
    let mut options = BTreeMap::new();
    for pair in arguments.chunks(2) {
        let Some(key)=pair.first() else {return Err(AppError::usage("missing option name"));};
        if pair.len()!=2 {return Err(AppError::usage(format!("option {key} requires one value")));}
        if !key.starts_with("--") {return Err(AppError::usage(format!("unexpected argument {key}")));}
        if options.insert(key.to_owned(),pair[1].clone()).is_some() {
            return Err(AppError::usage(format!("duplicate option {key}")));
        }
    }
    let required=["--game-exe","--native-dll","--helper","--artifact-dir","--script","--authorization"];
    let config_option_keys=[INPUT_CONFIG_FLAG,INPUT_LEAD_FLAG,FUTURE_WINDOW_FLAG,MAX_INPUT_AGE_FLAG,
        SCRIPT_DRAIN_FLAG,ADMISSION_BUDGET_FLAG];
    if let Some(key)=options.keys().find(|key| {
        let text=key.as_str();
        !required.contains(&text) && text!="--b-character" && text!="--world-record" &&
            text!="--world-e2e-probe" && text!="--protocol" && text!="--protocol-config" && !config_option_keys.contains(&text)
    }) {
        return Err(AppError::usage(format!("unknown option {key}")));
    }
    if required.iter().any(|key| !options.contains_key(*key)) {
        return Err(AppError::usage("coop-active requires --game-exe --native-dll --helper --artifact-dir --script --authorization; optional --b-character and input-config fields"));
    }
    if options.get("--world-record").is_some_and(|value|value!=crate::world_record::PROFILE) {
        return Err(AppError::usage("--world-record supports only min-read-v1"));
    }
    if options.get("--protocol").is_some_and(|value|value!="t0-v2" || !options.contains_key("--world-record")) ||
       (options.contains_key("--protocol-config") && !options.contains_key("--protocol")) {
        return Err(AppError::usage("--protocol t0-v2 requires --world-record min-read-v1; --protocol-config requires --protocol"));
    }
    if options.get("--world-e2e-probe").is_some_and(|value|value!="v1" || !options.contains_key("--world-record")) {
        return Err(AppError::usage("--world-e2e-probe v1 requires --world-record min-read-v1 and the explicitly compiled E2E Native"));
    }
    if options["--authorization"].trim().is_empty() {return Err(AppError::usage("record the actual user's operation authorization"));}
    let b_character=parse_character(options.get("--b-character").map_or("native",String::as_str))?;
    let input_config=resolve_input_config(options.get(INPUT_CONFIG_FLAG).map(String::as_str),&options)?;
    #[cfg(windows)] {desktop::run(&options,b_character,&input_config)}
    #[cfg(not(windows))] {Err(AppError::usage("coop-active requires Windows"))}
}

// Local game resources/scripts/enums.lua, PlayerType. This is a name directory,
// not an unlock list: the game-thread native selector roster remains authoritative.
const CHARACTERS:[(&str,i32);41]=[
    ("isaac",0),("magdalene",1),("cain",2),("judas",3),("bluebaby",4),
    ("eve",5),("samson",6),("azazel",7),("lazarus",8),("eden",9),
    ("thelost",10),("lazarus2",11),("blackjudas",12),("lilith",13),("keeper",14),
    ("apollyon",15),("theforgotten",16),("thesoul",17),("bethany",18),("jacob",19),
    ("esau",20),("isaac-b",21),("magdalene-b",22),("cain-b",23),("judas-b",24),
    ("bluebaby-b",25),("eve-b",26),("samson-b",27),("azazel-b",28),("lazarus-b",29),
    ("eden-b",30),("thelost-b",31),("lilith-b",32),("keeper-b",33),("apollyon-b",34),
    ("theforgotten-b",35),("bethany-b",36),("jacob-b",37),("lazarus2-b",38),
    ("jacob2-b",39),("thesoul-b",40),
];
fn parse_character(value:&str)->AppResult<i32> {
    let normalized=value.trim().to_ascii_lowercase().replace('_',"-");
    let name=normalized.strip_prefix("player-").unwrap_or(&normalized);
    if name=="native" {return Ok(-1);}
    let name=if let Some(base)=name.strip_prefix("tainted-") {format!("{base}-b")}else{name.to_owned()};
    let name=match name.as_str() {
        "lost"=>"thelost","forgotten"=>"theforgotten","soul"=>"thesoul","???"=>"bluebaby",
        "lost-b"=>"thelost-b","forgotten-b"=>"theforgotten-b","soul-b"=>"thesoul-b",other=>other,
    };
    if let Some((_,id))=CHARACTERS.iter().find(|(candidate,_)|*candidate==name) {return Ok(*id);}
    if let Ok(id)=name.parse::<i32>() {if id>=0 {return Ok(id);}}
    Err(AppError::usage(format!("invalid B character '{value}'; use --list-characters, a nonnegative ID, or native")))
}
fn character_choice(id:i32)->serde_json::Value {
    json!({"mode":if id<0{"native"}else{"explicit"},"id":if id<0{None}else{Some(id)},
        "name":CHARACTERS.iter().find(|(_,candidate)|*candidate==id).map(|(name,_)|*name)})
}

const INPUT_CONFIG_FLAG:&str="--input-config";
const INPUT_LEAD_FLAG:&str="--input-lead-ticks";
const FUTURE_WINDOW_FLAG:&str="--future-window-ticks";
const MAX_INPUT_AGE_FLAG:&str="--max-input-age-ticks";
const SCRIPT_DRAIN_FLAG:&str="--script-drain-budget";
const ADMISSION_BUDGET_FLAG:&str="--admission-attempt-budget";
const INPUT_CONFIG_SCHEMA_VERSION:u32=1;
const INPUT_LEAD_MIN:u64=1;
const INPUT_LEAD_MAX:u64=256;
const FUTURE_WINDOW_MIN:u64=1;
const FUTURE_WINDOW_MAX:u64=256;
const SCRIPT_DRAIN_MIN:u32=1;
const SCRIPT_DRAIN_MAX:u32=256;
const ADMISSION_ATTEMPT_MAX:u32=4096;
const DEFAULT_INPUT_LEAD_TICKS:u64=2;
const DEFAULT_FUTURE_WINDOW_TICKS:u64=64;
const DEFAULT_SCRIPT_DRAIN_BUDGET:u32=64;
const DEFAULT_ADMISSION_ATTEMPT_BUDGET:u32=2+4*DEFAULT_SCRIPT_DRAIN_BUDGET;

#[derive(Clone,Copy,Debug)]
struct InputConfig {
    input_lead_ticks:u64,
    future_window_ticks:u64,
    max_input_age_ticks:u64,
    script_drain_budget:u32,
    admission_attempt_budget:u32,
}
impl Default for InputConfig {
    fn default()->Self {Self{input_lead_ticks:DEFAULT_INPUT_LEAD_TICKS,
        future_window_ticks:DEFAULT_FUTURE_WINDOW_TICKS,max_input_age_ticks:0,
        script_drain_budget:DEFAULT_SCRIPT_DRAIN_BUDGET,
        admission_attempt_budget:DEFAULT_ADMISSION_ATTEMPT_BUDGET}}
}
#[derive(Default,Deserialize)]
#[serde(deny_unknown_fields)]
struct InputConfigManifest {
    #[serde(default,deserialize_with="deserialize_schema_version")] schema_version:Option<u32>,
    #[serde(default,deserialize_with="deserialize_input_lead")] input_lead_ticks:Option<u64>,
    #[serde(default,deserialize_with="deserialize_future_window")] future_window_ticks:Option<u64>,
    #[serde(default,deserialize_with="deserialize_max_age")] max_input_age_ticks:Option<u64>,
    #[serde(default,deserialize_with="deserialize_script_drain")] script_drain_budget:Option<u64>,
    #[serde(default,deserialize_with="deserialize_admission_budget")] admission_attempt_budget:Option<u64>,
}
macro_rules! reject_explicit_null {
    ($name:ident,$type:ty,$field:literal) => {
        fn $name<'de,D>(deserializer:D)->Result<Option<$type>,D::Error>
        where D:serde::Deserializer<'de> {
            match Option::<$type>::deserialize(deserializer)? {
                Some(value)=>Ok(Some(value)),
                None=>Err(serde::de::Error::custom(concat!("field `",$field,"` cannot be null"))),
            }
        }
    };
}
reject_explicit_null!(deserialize_schema_version,u32,"schema_version");
reject_explicit_null!(deserialize_input_lead,u64,"input_lead_ticks");
reject_explicit_null!(deserialize_future_window,u64,"future_window_ticks");
reject_explicit_null!(deserialize_max_age,u64,"max_input_age_ticks");
reject_explicit_null!(deserialize_script_drain,u64,"script_drain_budget");
reject_explicit_null!(deserialize_admission_budget,u64,"admission_attempt_budget");
struct ResolvedInputConfig {
    config:InputConfig,
    record:serde_json::Value,
    explicit:bool,
}
fn input_config_json(config:&InputConfig)->serde_json::Value {
    json!({"input_lead_ticks":config.input_lead_ticks,
        "future_window_ticks":config.future_window_ticks,
        "max_input_age_ticks":config.max_input_age_ticks,
        "script_drain_budget":config.script_drain_budget,
        "admission_attempt_budget":config.admission_attempt_budget})
}
fn input_config_support()->serde_json::Value {
    json!({
        "input_lead_ticks":{"minimum":INPUT_LEAD_MIN,"maximum":INPUT_LEAD_MAX,"unit":"authority_ticks"},
        "future_window_ticks":{"minimum":FUTURE_WINDOW_MIN,"maximum":FUTURE_WINDOW_MAX,"unit":"authority_ticks"},
        "max_input_age_ticks":{"only":0,"unit":"authority_ticks"},
        "script_drain_budget":{"minimum":SCRIPT_DRAIN_MIN,"maximum":SCRIPT_DRAIN_MAX,"unit":"script_steps_per_authority_tick"},
        "admission_attempt_budget":{"minimum":"2 + 4 * script_drain_budget","maximum":ADMISSION_ATTEMPT_MAX,"unit":"input_admission_attempts_per_authority_tick"}
    })
}
fn option_u64(options:&BTreeMap<String,String>,flag:&str)->AppResult<Option<u64>> {
    match options.get(flag) {
        None=>Ok(None),
        Some(text)=>text.parse::<u64>().map(Some).map_err(|_|AppError::usage(format!("{flag} requires an unsigned decimal integer"))),
    }
}
fn checked_u32(value:u64,field:&str)->AppResult<u32> {
    u32::try_from(value).map_err(|_|AppError::usage(format!("{field} exceeds the supported integer range")))
}
fn validate_input_config(config:&InputConfig)->AppResult<()> {
    if !(INPUT_LEAD_MIN..=INPUT_LEAD_MAX).contains(&config.input_lead_ticks) {
        return Err(AppError::usage(format!("input_lead_ticks must be in {INPUT_LEAD_MIN}..={INPUT_LEAD_MAX} authority ticks")));
    }
    if !(FUTURE_WINDOW_MIN..=FUTURE_WINDOW_MAX).contains(&config.future_window_ticks) {
        return Err(AppError::usage(format!("future_window_ticks must be in {FUTURE_WINDOW_MIN}..={FUTURE_WINDOW_MAX} authority ticks")));
    }
    if config.input_lead_ticks>config.future_window_ticks {
        return Err(AppError::usage("input_lead_ticks must not exceed future_window_ticks"));
    }
    if config.max_input_age_ticks!=0 {
        return Err(AppError::usage("max_input_age_ticks supports only 0 in operation 7; positive age would change the immediate-neutral policy"));
    }
    if !(SCRIPT_DRAIN_MIN..=SCRIPT_DRAIN_MAX).contains(&config.script_drain_budget) {
        return Err(AppError::usage(format!("script_drain_budget must be in {SCRIPT_DRAIN_MIN}..={SCRIPT_DRAIN_MAX} script steps per authority tick")));
    }
    let minimum_admission=2u64+4u64*config.script_drain_budget as u64;
    if config.admission_attempt_budget>ADMISSION_ATTEMPT_MAX {
        return Err(AppError::usage(format!("admission_attempt_budget must be at most {ADMISSION_ATTEMPT_MAX}")));
    }
    if (config.admission_attempt_budget as u64)<minimum_admission {
        return Err(AppError::usage(format!("admission_attempt_budget must be at least 2 + 4 * script_drain_budget ({minimum_admission})")));
    }
    Ok(())
}
fn resolve_input_config(manifest_path:Option<&str>,options:&BTreeMap<String,String>)->AppResult<ResolvedInputConfig> {
    let manifest=match manifest_path {
        None=>None,
        Some(path)=>Some(serde_json::from_slice::<InputConfigManifest>(
            &fs::read(path).map_err(|e|AppError::usage(format!("input config manifest {path}: {e}")))?)
            .map_err(|e|AppError::usage(format!("input config manifest {path}: {e}")))?),
    };
    let mut config=InputConfig::default();
    let mut sources:serde_json::Map<String,serde_json::Value>=serde_json::Map::new();
    let mut manifest_fields:BTreeSet<&'static str>=BTreeSet::new();
    if let Some(manifest)=manifest.as_ref() {
        if let Some(schema)=manifest.schema_version {
            if schema!=INPUT_CONFIG_SCHEMA_VERSION {
                return Err(AppError::usage(format!("schema_version must be {INPUT_CONFIG_SCHEMA_VERSION}")));
            }
        }
        if let Some(value)=manifest.input_lead_ticks {config.input_lead_ticks=value;manifest_fields.insert("input_lead_ticks");sources.insert("input_lead_ticks".into(),json!("manifest"));}
        if let Some(value)=manifest.future_window_ticks {config.future_window_ticks=value;manifest_fields.insert("future_window_ticks");sources.insert("future_window_ticks".into(),json!("manifest"));}
        if let Some(value)=manifest.max_input_age_ticks {config.max_input_age_ticks=value;manifest_fields.insert("max_input_age_ticks");sources.insert("max_input_age_ticks".into(),json!("manifest"));}
        if let Some(value)=manifest.script_drain_budget {config.script_drain_budget=checked_u32(value,"script_drain_budget")?;manifest_fields.insert("script_drain_budget");sources.insert("script_drain_budget".into(),json!("manifest"));}
        if let Some(value)=manifest.admission_attempt_budget {config.admission_attempt_budget=checked_u32(value,"admission_attempt_budget")?;manifest_fields.insert("admission_attempt_budget");sources.insert("admission_attempt_budget".into(),json!("manifest"));}
    }
    let apply_cli_u64=|field:&'static str,flag:&str,slot:&mut u64,sources:&mut serde_json::Map<String,serde_json::Value>|->AppResult<()> {
        if let Some(value)=option_u64(options,flag)? {
            if manifest_fields.contains(field) {
                return Err(AppError::usage(format!("input config conflict for {field}: supplied by both manifest and {flag}")));
            }
            *slot=value;sources.insert(field.into(),json!("cli"));Ok(())
        } else {Ok(())}
    };
    apply_cli_u64("input_lead_ticks",INPUT_LEAD_FLAG,&mut config.input_lead_ticks,&mut sources)?;
    apply_cli_u64("future_window_ticks",FUTURE_WINDOW_FLAG,&mut config.future_window_ticks,&mut sources)?;
    apply_cli_u64("max_input_age_ticks",MAX_INPUT_AGE_FLAG,&mut config.max_input_age_ticks,&mut sources)?;
    let mut script_value=config.script_drain_budget as u64;
    let mut admission_value=config.admission_attempt_budget as u64;
    apply_cli_u64("script_drain_budget",SCRIPT_DRAIN_FLAG,&mut script_value,&mut sources)?;
    apply_cli_u64("admission_attempt_budget",ADMISSION_BUDGET_FLAG,&mut admission_value,&mut sources)?;
    config.script_drain_budget=checked_u32(script_value,"script_drain_budget")?;
    config.admission_attempt_budget=checked_u32(admission_value,"admission_attempt_budget")?;
    for field in ["input_lead_ticks","future_window_ticks","max_input_age_ticks","script_drain_budget","admission_attempt_budget"] {
        sources.entry(field).or_insert(json!("default"));
    }
    validate_input_config(&config)?;
    let record=json!({
        "schemaVersion":INPUT_CONFIG_SCHEMA_VERSION,
        "requested":input_config_json(&config),
        "effective":input_config_json(&config),
        "source":serde_json::Value::Object(sources),
        "support":input_config_support(),
        "mappingVersion":5,
        "freeze":"before_hook_publication_and_active",
        "unknownFieldPolicy":"reject",
        "conflictPolicy":"reject",
        "units":{"lead":"authority_ticks","futureWindow":"authority_ticks","maxInputAge":"authority_ticks","scriptDrain":"script_steps_per_authority_tick","admission":"input_admission_attempts_per_authority_tick"}
    });
    let explicit=manifest_path.is_some() || [INPUT_LEAD_FLAG,FUTURE_WINDOW_FLAG,MAX_INPUT_AGE_FLAG,SCRIPT_DRAIN_FLAG,ADMISSION_BUDGET_FLAG].iter().any(|flag|options.contains_key(*flag));
    Ok(ResolvedInputConfig{config,record,explicit})
}

#[derive(Clone, Copy, Debug, Serialize)]
#[repr(C)]
pub struct InputStateDraftV2 {
    pub version:u16,pub encoded_length:u16,pub held_flags:u32,pub edge_flags:u32,
    pub move_x:i16,pub move_y:i16,pub aim_x:i16,pub aim_y:i16,pub reserved:u32,
}
#[derive(Clone, Copy, Debug, Serialize)]
#[repr(C,align(8))]
pub struct InputCmdDraftV2 {
    pub session:[u8;16],pub client:u64,pub sequence:u64,pub target_tick:u64,pub state:InputStateDraftV2,
}
const _:()=assert!(std::mem::size_of::<InputStateDraftV2>()==24);
const _:()=assert!(std::mem::size_of::<InputCmdDraftV2>()==64);
impl InputStateDraftV2 {
    pub fn encode(self)->AppResult<[u8;24]> {
        if self.version!=2 || self.encoded_length!=24 || self.held_flags&!15!=0 || self.edge_flags&!15!=0 || self.reserved!=0 {
            return Err(AppError::usage("invalid V2 input state header/mask"));
        }
        crate::input_core::InputStateDraftV1::from_q15(self.held_flags&1!=0,self.edge_flags as u8,
            self.move_x,self.move_y,self.aim_x,self.aim_y).map_err(|e|AppError::usage(e.to_string()))?;
        let mut out=[0u8;24];out[0..2].copy_from_slice(&2u16.to_le_bytes());out[2..4].copy_from_slice(&24u16.to_le_bytes());
        out[4..8].copy_from_slice(&self.held_flags.to_le_bytes());out[8..12].copy_from_slice(&self.edge_flags.to_le_bytes());
        for (index,axis) in [self.move_x,self.move_y,self.aim_x,self.aim_y].iter().enumerate() {
            out[12+index*2..14+index*2].copy_from_slice(&axis.to_le_bytes());
        }
        Ok(out)
    }
    pub fn decode(bytes:&[u8])->AppResult<Self> {
        if bytes.len()!=24 {return Err(AppError::usage("V2 state must be 24 bytes"));}
        let u16_at=|n|u16::from_le_bytes([bytes[n],bytes[n+1]]);
        let u32_at=|n|u32::from_le_bytes([bytes[n],bytes[n+1],bytes[n+2],bytes[n+3]]);
        let value=Self{version:u16_at(0),encoded_length:u16_at(2),held_flags:u32_at(4),edge_flags:u32_at(8),
            move_x:u16_at(12) as i16,move_y:u16_at(14) as i16,aim_x:u16_at(16) as i16,aim_y:u16_at(18) as i16,reserved:u32_at(20)};
        value.encode()?;Ok(value)
    }
}
impl InputCmdDraftV2 {
    pub fn encode(self)->AppResult<[u8;64]> {
        let mut out=[0u8;64];out[..16].copy_from_slice(&self.session);
        for (index,value) in [self.client,self.sequence,self.target_tick].iter().enumerate() {
            out[16+index*8..24+index*8].copy_from_slice(&value.to_le_bytes());
        }
        out[40..].copy_from_slice(&self.state.encode()?);Ok(out)
    }
    pub fn decode(bytes:&[u8])->AppResult<Self> {
        if bytes.len()!=64 {return Err(AppError::usage("V2 command must be 64 bytes"));}
        let mut session=[0u8;16];session.copy_from_slice(&bytes[..16]);
        let mut words=[0u64;3];
        for (index,value) in words.iter_mut().enumerate() {
            let mut word=[0u8;8];word.copy_from_slice(&bytes[16+index*8..24+index*8]);*value=u64::from_le_bytes(word);
        }
        Ok(Self{session,client:words[0],sequence:words[1],target_tick:words[2],state:InputStateDraftV2::decode(&bytes[40..])?})
    }
}

#[derive(Clone,Deserialize,Serialize)]
#[serde(deny_unknown_fields)]
struct Step {
    tick:u64,
    #[serde(default)] held_flags:u32,
    #[serde(default)] edge_flags:u32,
    #[serde(default)] move_x:i16,
    #[serde(default)] move_y:i16,
    #[serde(default)] aim_x:i16,
    #[serde(default)] aim_y:i16,
    // Optional raw source mask: left=1, right=2, up=4, down=8. Canonical
    // commands still carry only the resolved aim, through the existing queue.
    #[serde(default,skip_serializing_if="Option::is_none")] shoot_directions:Option<u32>,
    #[serde(default)] fault:u32,
}
#[derive(Deserialize,Serialize)]
#[serde(deny_unknown_fields)]
struct Script {steps:Vec<Step>}

fn write_json(path:&Path,value:&serde_json::Value)->AppResult<()> {
    let mut file=OpenOptions::new().write(true).create_new(true).open(path)
        .map_err(|e|AppError::evidence(format!("{}: {e}",path.display())))?;
    serde_json::to_writer_pretty(&mut file,value).map_err(|e|AppError::evidence(e.to_string()))?;
    file.write_all(b"\n").map_err(|e|AppError::evidence(e.to_string()))
}

#[cfg(windows)]
mod desktop {
    use super::*;
    use crate::loop_ipc::LoopMapping;
    use crate::win32::{self,ObservedProcess};
    use std::ffi::c_void;
    use std::io::BufWriter;
    use std::mem::{size_of,offset_of};
    use std::os::windows::process::CommandExt;
    use std::ptr::{NonNull,read_volatile,write_volatile};
    use std::process::{Command,Stdio};
    use std::sync::{atomic::{AtomicI32,AtomicU64,Ordering,fence},mpsc};
    use std::thread;
    use std::time::{Duration,Instant};
    use windows::core::PCWSTR;
    use windows::Win32::Foundation::{CloseHandle,ERROR_ALREADY_EXISTS,GetLastError,HANDLE,INVALID_HANDLE_VALUE};
    use windows::Win32::System::Memory::{CreateFileMappingW,MapViewOfFile,UnmapViewOfFile,
        MEMORY_MAPPED_VIEW_ADDRESS,FILE_MAP_ALL_ACCESS,PAGE_READWRITE};
    use windows::Win32::System::Threading::CREATE_NO_WINDOW;

    const MAGIC:u64=0x315643414F4F4349;
    const EVENT_CAPACITY:usize=32768;
    const SCRIPT_CAPACITY:usize=256;
    const WORLD_EXTENSION_SIZE:usize=192;
    const INPUT_CONFIG_SIZE:usize=48;
    #[link(name="advapi32")]
    unsafe extern "system" {fn SystemFunction036(buffer:*mut c_void,length:u32)->u8;}
    #[repr(C,align(8))]
    struct Header {
        magic:u64,version:u32,size:u32,pid:u32,script_count:u32,
        session:[u8;16],nonce:[u8;32],clients:[u64;2],creation_time:u64,controller_pid:u32,b_character:i32,loop_name:[u16;96],
        state:AtomicI32,error:AtomicI32,stop:AtomicI32,retire_b:AtomicI32,
        main_thread:AtomicI32,active_callbacks:AtomicI32,pinned:AtomicI32,restored:AtomicI32,
        produced:AtomicU64,consumed:AtomicU64,completed_tick:AtomicU64,overwritten:AtomicU64,
    }
    #[repr(C)]
    #[derive(Clone,Copy)]
    struct NativeStep {tick:u64,state:InputStateDraftV2,fault:u32,shoot_directions:u32}
    #[repr(C,align(8))]
    #[derive(Clone,Copy)]
    struct NativeInputConfig {
        version:u32,size:u32,input_lead_ticks:u64,future_window_ticks:u64,
        max_input_age_ticks:u64,script_drain_budget:u32,admission_attempt_budget:u32,reserved:u64,
    }
    #[repr(C,align(8))]
    #[derive(Clone,Copy,Debug,Serialize)]
    pub(super) struct Event {
        published:i64,tick:u64,generation:u64,command:u64,target:u64,
        kind:u32,ordinal:u32,action:u32,query:u32,decision:u32,detail:u32,
        scalar:f32,boolean:u32,state:InputStateDraftV2,before:[u32;4],after:[u32;4],
    }
    const EVENTS_OFFSET:usize=size_of::<Header>()+size_of::<NativeStep>()*SCRIPT_CAPACITY;
    const MAPPING_SIZE:usize=EVENTS_OFFSET+size_of::<Event>()*EVENT_CAPACITY;
    const _:()=assert!(size_of::<Header>()==360);
    const _:()=assert!(offset_of!(Header,b_character)==100);
    const _:()=assert!(size_of::<NativeStep>()==40);
    const _:()=assert!(size_of::<Event>()==128);
    const _:()=assert!(offset_of!(Event,state)==72);
    const _:()=assert!(MAPPING_SIZE==4204904);
    const _:()=assert!(size_of::<NativeInputConfig>()==INPUT_CONFIG_SIZE);
    const _:()=assert!(offset_of!(NativeInputConfig,input_lead_ticks)==8);
    const _:()=assert!(offset_of!(NativeInputConfig,future_window_ticks)==16);
    const _:()=assert!(offset_of!(NativeInputConfig,max_input_age_ticks)==24);
    const _:()=assert!(offset_of!(NativeInputConfig,script_drain_budget)==32);
    const _:()=assert!(offset_of!(NativeInputConfig,admission_attempt_budget)==36);
    const _:()=assert!(offset_of!(NativeInputConfig,reserved)==40);
    const V5_MAPPING_SIZE:usize=MAPPING_SIZE+WORLD_EXTENSION_SIZE+INPUT_CONFIG_SIZE;
    const _:()=assert!(V5_MAPPING_SIZE==4205144);
    const PROTOCOL_TRACE_CAPACITY:usize=256;
    const PROTOCOL_COUNTER_OFFSET:usize=V5_MAPPING_SIZE+64;
    const PROTOCOL_TRACE_OFFSET:usize=PROTOCOL_COUNTER_OFFSET+24;
    const V6_MAPPING_SIZE:usize=PROTOCOL_TRACE_OFFSET+1248*PROTOCOL_TRACE_CAPACITY;
    #[repr(C,align(8))]
    #[derive(Clone,Copy,Serialize,Deserialize)]
    #[serde(default,deny_unknown_fields)]
    struct NativeFaultConfig {
        version:u32,size:u32,seed:u64,delay_ticks:u32,jitter_ticks:u32,loss_per_mille:u32,
        duplicate_per_mille:u32,reorder_per_mille:u32,reserved:u32,
        outage_start:u64,outage_ticks:u64,reserved_tail:u64,
    }
    impl Default for NativeFaultConfig {
        fn default()->Self {Self{version:1,size:64,seed:0x20008,delay_ticks:1,jitter_ticks:1,loss_per_mille:50,
            duplicate_per_mille:150,reorder_per_mille:150,reserved:0,outage_start:64,outage_ticks:20,reserved_tail:0}}
    }
    #[repr(C,align(8))]
    #[derive(Clone,Copy)]
    struct ProtocolTrace {published:i64,tick:u64,token:u64,generation:u64,action:u32,direction:u32,decision:u32,length:u32,bytes:[u8;1200]}
    const _:()=assert!(size_of::<NativeFaultConfig>()==64);
    const _:()=assert!(size_of::<ProtocolTrace>()==1248);
    const _:()=assert!(V6_MAPPING_SIZE==4524720);
    fn protocol_config(options:&BTreeMap<String,String>)->AppResult<Option<NativeFaultConfig>> {
        if !options.contains_key("--protocol") {return Ok(None);}
        let c:NativeFaultConfig=if let Some(path)=options.get("--protocol-config") {
            serde_json::from_slice(&fs::read(path).map_err(|e|AppError::usage(e.to_string()))?)
                .map_err(|e|AppError::usage(format!("protocol config: {e}")))?
        } else {NativeFaultConfig::default()};
        if c.version!=1 || c.size!=64 || c.seed==0 || c.delay_ticks>8 || c.jitter_ticks>8 || c.loss_per_mille>1000 ||
           c.duplicate_per_mille>1000 || c.reorder_per_mille>1000 || c.reserved!=0 || c.reserved_tail!=0 ||
           c.outage_start.checked_add(c.outage_ticks).is_none() {return Err(AppError::usage("invalid protocol fault config"));}
        Ok(Some(c))
    }

    struct Mapping {handle:HANDLE,view:NonNull<u8>,name:String}
    impl Mapping {
        fn create(name:String,extended:bool,use_v5:bool,use_protocol:bool)->AppResult<Self> {
            let mapping_size=if use_protocol {V6_MAPPING_SIZE} else if use_v5 {V5_MAPPING_SIZE} else {MAPPING_SIZE+if extended{WORLD_EXTENSION_SIZE}else{0}};
            let wide:Vec<_>=name.encode_utf16().chain(Some(0)).collect();
            let handle=unsafe{CreateFileMappingW(INVALID_HANDLE_VALUE,None,PAGE_READWRITE,0,mapping_size as u32,PCWSTR(wide.as_ptr()))}
                .map_err(|e|AppError::evidence(format!("create mapping: {e}")))?;
            if unsafe{GetLastError()}==ERROR_ALREADY_EXISTS {
                let _=unsafe{CloseHandle(handle)};return Err(AppError::evidence("co-op mapping already exists"));
            }
            let raw=unsafe{MapViewOfFile(handle,FILE_MAP_ALL_ACCESS,0,0,mapping_size)};
            let Some(view)=NonNull::new(raw.Value.cast::<u8>()) else {
                let _=unsafe{CloseHandle(handle)};return Err(AppError::evidence("map co-op view failed"));
            };
            Ok(Self{handle,view,name})
        }
        fn header(&self)->&Header {unsafe{&*self.view.as_ptr().cast::<Header>()}}
        fn initialize(&self,process:&ObservedProcess,nonce:[u8;32],loop_name:&str,script:&Script,
            b_character:i32,world_name:Option<&str>,input_config:&InputConfig,use_v5:bool,protocol:Option<NativeFaultConfig>)->AppResult<()> {
            let wide:Vec<_>=loop_name.encode_utf16().collect();
            if wide.len()>=96 {return Err(AppError::usage("companion name too long"));}
            let has_world=world_name.is_some();
            let version=if protocol.is_some(){6}else if use_v5 {5} else if has_world {4} else {3};
            let mapping_size=if protocol.is_some(){V6_MAPPING_SIZE}else if use_v5 {V5_MAPPING_SIZE} else {MAPPING_SIZE+if has_world {WORLD_EXTENSION_SIZE}else{0}};
            let ptr=self.view.as_ptr().cast::<Header>();
            unsafe {
                write_volatile(&raw mut (*ptr).magic,MAGIC);write_volatile(&raw mut (*ptr).version,version);
                write_volatile(&raw mut (*ptr).size,mapping_size as u32);write_volatile(&raw mut (*ptr).pid,process.pid);
                write_volatile(&raw mut (*ptr).creation_time,process.creation_time_100ns);
                write_volatile(&raw mut (*ptr).controller_pid,std::process::id());
                write_volatile(&raw mut (*ptr).b_character,b_character);
                write_volatile(&raw mut (*ptr).script_count,script.steps.len() as u32);
                std::ptr::copy_nonoverlapping(nonce.as_ptr(),(&raw mut (*ptr).session).cast::<u8>(),16);
                write_volatile(&raw mut (*ptr).nonce,nonce);write_volatile(&raw mut (*ptr).clients,[1,2]);
                std::ptr::copy_nonoverlapping(wide.as_ptr(),(&raw mut (*ptr).loop_name).cast::<u16>(),wide.len());
                let steps=self.view.as_ptr().add(size_of::<Header>()).cast::<NativeStep>();
                for (index,step) in script.steps.iter().enumerate() {
                    write_volatile(steps.add(index),NativeStep{tick:step.tick,state:InputStateDraftV2{
                        version:2,encoded_length:24,held_flags:step.held_flags,edge_flags:step.edge_flags,
                        move_x:step.move_x,move_y:step.move_y,aim_x:step.aim_x,aim_y:step.aim_y,reserved:0},
                        fault:step.fault,shoot_directions:step.shoot_directions.map_or(0,|mask|16|mask)});
                }
            }
            if let Some(name)=world_name {
                let wide:Vec<_>=name.encode_utf16().collect();
                if wide.is_empty() || wide.len()>=96{return Err(AppError::usage("world mapping name too long"));}
                unsafe{std::ptr::copy_nonoverlapping(wide.as_ptr(),self.view.as_ptr().add(MAPPING_SIZE).cast::<u16>(),wide.len());}
            }
            if use_v5 || protocol.is_some() {
                let native=NativeInputConfig{version:1,size:INPUT_CONFIG_SIZE as u32,
                    input_lead_ticks:input_config.input_lead_ticks,
                    future_window_ticks:input_config.future_window_ticks,
                    max_input_age_ticks:input_config.max_input_age_ticks,
                    script_drain_budget:input_config.script_drain_budget,
                    admission_attempt_budget:input_config.admission_attempt_budget,reserved:0};
                unsafe{std::ptr::write(self.view.as_ptr().add(MAPPING_SIZE+WORLD_EXTENSION_SIZE).cast::<NativeInputConfig>(),native);}
            }
            if let Some(config)=protocol {unsafe{std::ptr::write(self.view.as_ptr().add(V5_MAPPING_SIZE).cast::<NativeFaultConfig>(),config);}}
            fence(Ordering::Release);Ok(())
        }
        fn protocol_counters(&self)->serde_json::Value {
            let counters=unsafe{self.view.as_ptr().add(PROTOCOL_COUNTER_OFFSET).cast::<AtomicU64>()};
            json!({"produced":unsafe{(*counters).load(Ordering::Acquire)},
                "consumed":unsafe{(*counters.add(1)).load(Ordering::Acquire)},
                "lost":unsafe{(*counters.add(2)).load(Ordering::Acquire)}})
        }
        fn drain_protocol(&self,next:&mut u64,file:&mut impl Write,closing:bool)->AppResult<()> {
            let counters=unsafe{self.view.as_ptr().add(PROTOCOL_COUNTER_OFFSET).cast::<AtomicU64>()};
            let produced=unsafe{(*counters).load(Ordering::Acquire)};
            // The producer refuses full slots; closing may preserve queued records
            // after a loss, but never changes that loss into a successful capture.
            if produced<*next || produced-*next>PROTOCOL_TRACE_CAPACITY as u64 ||
                (!closing && unsafe{(*counters.add(2)).load(Ordering::Acquire)}!=0) {
                return Err(AppError::evidence("protocol trace overflow"));
            }
            let mut copied=Vec::with_capacity((produced-*next) as usize);
            while *next<produced {
                let sequence=*next+1;
                let slot=unsafe{self.view.as_ptr().add(PROTOCOL_TRACE_OFFSET).cast::<ProtocolTrace>().add(((sequence-1)%PROTOCOL_TRACE_CAPACITY as u64) as usize)};
                if unsafe{read_volatile(&raw const (*slot).published)}!=sequence as i64 {break;}
                fence(Ordering::Acquire);let record=unsafe{read_volatile(slot)};fence(Ordering::Acquire);
                if unsafe{read_volatile(&raw const (*slot).published)}!=sequence as i64 || record.published!=sequence as i64 {break;}
                if record.length>1200 {return Err(AppError::evidence("protocol trace packet length"));}
                copied.push(record);*next=sequence;
                // No shared slot reference survives this release. JSON and I/O
                // operate solely on the bounded owned copy below.
                unsafe{(*counters.add(1)).store(*next,Ordering::Release)};
            }
            for record in copied {
                line(file,&json!({"sequence":record.published,"tick":record.tick,"token":record.token,"generation":record.generation,
                    "action":record.action,"direction":record.direction,"decision":record.decision,"bytes":hex::encode(&record.bytes[..record.length as usize])}))?;
            }
            Ok(())
        }
        fn drain(&self,next:&mut u64)->AppResult<Vec<Event>> {
            let header=self.header();let produced=header.produced.load(Ordering::Acquire);
            if header.overwritten.load(Ordering::Acquire)!=0 || produced.saturating_sub(*next)>EVENT_CAPACITY as u64 {
                return Err(AppError::evidence("co-op event ring overflow"));
            }
            let mut events=Vec::new();
            while *next<produced {
                let sequence=*next+1;
                let slot=unsafe{self.view.as_ptr().add(EVENTS_OFFSET).cast::<Event>().add(((sequence-1)%EVENT_CAPACITY as u64) as usize)};
                let before=unsafe{read_volatile(&raw const (*slot).published)};
                if before!=sequence as i64 {break;}
                fence(Ordering::Acquire);let event=unsafe{read_volatile(slot)};fence(Ordering::Acquire);
                if unsafe{read_volatile(&raw const (*slot).published)}!=before || event.published!=before {break;}
                events.push(event);*next=sequence;
            }
            header.consumed.store(*next,Ordering::Release);Ok(events)
        }
    }
    impl Drop for Mapping {
        fn drop(&mut self) {
            let _=unsafe{UnmapViewOfFile(MEMORY_MAPPED_VIEW_ADDRESS{Value:self.view.as_ptr().cast()})};
            let _=unsafe{CloseHandle(self.handle)};
        }
    }
    fn line(file:&mut impl Write,value:&impl Serialize)->AppResult<()> {
        serde_json::to_writer(&mut *file,value).map_err(|e|AppError::evidence(e.to_string()))?;
        file.write_all(b"\n").map_err(|e|AppError::evidence(e.to_string()))
    }
    fn loop_line(file:&mut impl Write,record:&crate::loop_ipc::NativeLoopEventRecord)->AppResult<()> {
        line(file,&json!({"sequence":record.sequence,"kind":record.event.kind,"phase":record.event.phase,
            "loopIteration":record.event.loop_iteration,"threadId":record.event.thread_id,
            "returnRva":record.event.return_rva,"originalCalls":record.event.original_calls,
            "flags":record.event.flags,"errorCode":record.event.error_code,"valueBits":record.event.value_bits,
            "action":record.event.action,"deviceKey":record.event.device_key}))
    }
    fn new_log(path:&Path)->AppResult<File> {
        OpenOptions::new().write(true).create_new(true).open(path).map_err(|e|AppError::evidence(e.to_string()))
    }
    fn buffered_log(path:&Path)->AppResult<BufWriter<File>> {
        Ok(BufWriter::with_capacity(256*1024,new_log(path)?))
    }
    fn hidden_helper(path:&str)->Command {
        let mut command=Command::new(path);
        command.creation_flags(CREATE_NO_WINDOW.0);
        command
    }
    struct ResidentCleanup {helper:String,dll:String,pid:u32,log:PathBuf,armed:bool}
    impl Drop for ResidentCleanup {
        fn drop(&mut self) {
            if !self.armed {return;}
            // One normal stop attempt on controller failure. Never FreeLibrary or reinject.
            let result=hidden_helper(&self.helper).args(["--stop-coop-active",&self.pid.to_string(),&self.dll]).output();
            if let Ok(mut file)=OpenOptions::new().append(true).create(true).open(&self.log) {
                match result {
                    Ok(output)=>{let _=file.write_all(&output.stdout);let _=file.write_all(&output.stderr);},
                    Err(error)=>{let _=writeln!(file,"resident cleanup could not start: {error}; do not force unload");},
                }
            }
        }
    }
    pub(super) fn run(options:&BTreeMap<String,String>,b_character:i32,
        resolved_config:&ResolvedInputConfig)->AppResult<()> {
        let protocol=protocol_config(options)?;
        let script:Script=serde_json::from_slice(&fs::read(&options["--script"]).map_err(|e|AppError::usage(e.to_string()))?)
            .map_err(|e|AppError::usage(format!("script: {e}")))?;
        if script.steps.len()>SCRIPT_CAPACITY || script.steps.windows(2).any(|v|v[0].tick>=v[1].tick) {
            return Err(AppError::usage("script requires at most 256 strictly increasing tick entries"));
        }
        for step in &script.steps {
            if protocol.is_some() && step.fault!=0 {return Err(AppError::usage("protocol mode requires source faults=0; use its independent FaultTransport config"));}
            if let Some(mask)=step.shoot_directions {
                if mask&!15!=0 || step.fault>1 || step.held_flags&1!=0 || step.aim_x!=0 || step.aim_y!=0 {
                    return Err(AppError::usage("raw shooting script requires a four-bit mask, no canonical aim/Shoot held, and fault 0/1"));
                }
            }
            if step.held_flags&!15!=0 || step.edge_flags&!15!=0 || step.fault>6 ||
                crate::input_core::InputStateDraftV1::from_q15(step.held_flags&1!=0,
                    step.edge_flags as u8,step.move_x,step.move_y,step.aim_x,step.aim_y).is_err() {
                return Err(AppError::usage("script contains invalid V2 state/fault"));
            }
        }
        let directory=PathBuf::from(&options["--artifact-dir"]);
        fs::create_dir(&directory).map_err(|e|AppError::evidence(format!("artifact directory must be new: {e}")))?;
        let coop_version=if protocol.is_some(){6}else if resolved_config.explicit {5} else if options.contains_key("--world-record") {4} else {3};
        let mut input_record=resolved_config.record.clone();
        if let Some(object)=input_record.as_object_mut() {
            object.insert("mappingVersion".into(),json!(coop_version));
            object.insert("mappingSelection".into(),json!(if protocol.is_some(){"protocol-v6"}else if resolved_config.explicit {
                "explicit-v5"
            } else if options.contains_key("--world-record") {"legacy-v4-default-config"} else {"legacy-v3-default-config"}));
        }
        write_json(&directory.join("invocation.json"),&json!({"tx":"tx-01-45","authorization":options["--authorization"],
            "script":script,"bCharacter":character_choice(b_character),"coopIpcVersion":coop_version,
            "worldRecord":options.get("--world-record"),
            "protocol":protocol,
            "inputConfig":input_record,
            "writer":"Authority-pre/current; normal-post/previous","root":"resident","maxInputAgeTicks":0,
            "inputConfigFreeze":"before_hook_publication_and_active"}))?;
        let result=run_capture(options,&directory,&script,b_character,resolved_config,protocol);
        if let Err(error)=&result {
            let path=directory.join("result.json");
            if !path.exists() {write_json(&path,&json!({"status":"FAIL","error":error.to_string(),"automaticRetry":false}))?;}
        }
        result
    }
    fn run_capture(options:&BTreeMap<String,String>,directory:&Path,script:&Script,b_character:i32,
        resolved_config:&ResolvedInputConfig,protocol:Option<NativeFaultConfig>)->AppResult<()> {
        let pids=win32::find_target_pids()?;
        if pids.len()!=1 {return Err(AppError::evidence("exactly one user-started isaac-ng.exe is required"));}
        let process=ObservedProcess::open_exact(pids[0],Path::new(&options["--game-exe"]))?;
        if process.debugger_present()? || !process.forbidden_modules().is_empty() {
            return Err(AppError::evidence("debugger/conflicting module present; no injection performed"));
        }
        let mut nonce=[0u8;32];
        if unsafe{SystemFunction036(nonce.as_mut_ptr().cast(),32)}==0 {return Err(AppError::evidence("OS nonce generation failed"));}
        let suffix=hex::encode(nonce);
        let coop_version=if protocol.is_some(){6}else if resolved_config.explicit {5} else if options.contains_key("--world-record") {4} else {3};
        let name=format!("Local\\IsaacCsT0.CoopActive.v{coop_version}.{suffix}");
        let companion=LoopMapping::create(format!("Local\\IsaacCsT0.CoopActiveLoop.{suffix}"),process.pid,1,nonce)?;
        let mut world=if options.contains_key("--world-record") {
            Some(crate::world_record::live::Mapping::create(format!("Local\\IsaacCsT0.World.{suffix}"),&directory.join("world"),process.pid,nonce)?)
        } else {None};
        let mut e2e=if options.contains_key("--world-e2e-probe") {
            Some(crate::world_e2e::Probe::create(&world.as_ref().ok_or_else(||AppError::usage("E2E requires world"))?.name,
                process.pid,nonce,&directory.join("e2e"))?)
        } else {None};
        let mapping=Mapping::create(name,world.is_some(),resolved_config.explicit,protocol.is_some())?;
        mapping.initialize(&process,nonce,companion.name(),script,b_character,
            world.as_ref().map(|mapping|mapping.name.as_str()),&resolved_config.config,
            resolved_config.explicit,protocol)?;
        // tx-01-77: independent read-only acceptance can retain these mappings
        // after controller loss. This file is evidence, not an IPC/API change.
        let controller_created=unsafe {
            use windows::Win32::Foundation::FILETIME;
            use windows::Win32::System::Threading::{GetCurrentProcess,GetProcessTimes};
            let (mut created,mut exited,mut kernel,mut user)=(FILETIME::default(),FILETIME::default(),FILETIME::default(),FILETIME::default());
            GetProcessTimes(GetCurrentProcess(),&mut created,&mut exited,&mut kernel,&mut user)
                .map_err(|e|AppError::evidence(format!("controller creation time: {e}")))?;
            ((created.dwHighDateTime as u64)<<32)|created.dwLowDateTime as u64
        };
        write_json(&directory.join("observation-metadata.json"),&json!({
            "version":1,"purpose":"INDEPENDENT_READ_ONLY_E2E_OBSERVATION",
            "game_pid":process.pid,"game_creation_time_100ns":process.creation_time_100ns.to_string(),
            "controller_pid":std::process::id(),"controller_creation_time_100ns":controller_created.to_string(),
            "session":hex::encode(&nonce[..16]),"nonce":hex::encode(nonce),
            "coop_mapping":mapping.name,"coop_version":coop_version,
            "world_mapping":world.as_ref().map(|mapping|mapping.name.as_str()),"world_version":if world.is_some(){Some(2)}else{None},
            "read_only_observer_must_not_consume":true
            ,"e2e_mapping":e2e.as_ref().map(|probe|probe.name.as_str())
        }))?;
        let mut events_file=buffered_log(&directory.join("events.jsonl"))?;
        let mut protocol_file=if protocol.is_some(){Some(buffered_log(&directory.join("protocol.jsonl"))?)}else{None};
        let mut protocol_next=0;
        let mut loops_file=buffered_log(&directory.join("loop-events.jsonl"))?;
        let mut operations=buffered_log(&directory.join("operations.jsonl"))?;
        let mut helper_log=new_log(&directory.join("helper.log"))?;
        let mut cleanup=ResidentCleanup{helper:options["--helper"].clone(),dll:options["--native-dll"].clone(),
            pid:process.pid,log:directory.join("helper.log"),armed:true};
        let mut child=hidden_helper(&options["--helper"]).args(["--inject-coop-active",&process.pid.to_string(),
            &options["--native-dll"],&mapping.name]).stdout(Stdio::piped()).stderr(Stdio::piped()).spawn()
            .map_err(|e|AppError::evidence(format!("helper spawn: {e}")))?;
        let mut next=0;let mut loop_next=0;let mut events=Vec::new();let start=Instant::now();
        let (sender,receiver)=mpsc::channel();
        thread::spawn(move||{for text in std::io::stdin().lock().lines(){match text {Ok(text)=>{if sender.send(text).is_err(){break;}},Err(_)=>break}}});
        println!("coop-active: mark <operation>, status, script, queries, conflict-b, retire-b, stop; keep this controller running until restoration.");
        println!("B character: {} (fixed before Join); A uses the in-game selection.",character_choice(b_character));
        let mut initialized=false;let mut stopping=false;let mut shutdown_child=None;
        let mut transport_failure:Option<String>=None;
        let capture_result=(||->AppResult<()> {
        loop {
            if let Some(file)=protocol_file.as_mut(){mapping.drain_protocol(&mut protocol_next,file,false)?;}
            if let Some(world)=world.as_mut(){world.poll_failure();}
            if let Some(probe)=e2e.as_mut(){probe.drain()?;}
            for event in mapping.drain(&mut next)? {
                if event.kind==30 && event.action==3 {
                    println!("B character created: {} (requested match={})",event.query,event.boolean!=0);
                } else if event.kind==30 && event.action==2 && event.boolean==0 {
                    eprintln!("B character {} is unavailable in the current native selector; creation stopped.",event.decision);
                }
                line(&mut events_file,&event)?;events.push(event);
            }
            for record in companion.drain(&mut loop_next)? {
                loop_line(&mut loops_file,&record)?;
            }
            if !initialized {
                if let Some(status)=child.try_wait().map_err(|e|AppError::evidence(e.to_string()))? {
                    if let Some(mut stdout)=child.stdout.take(){std::io::copy(&mut stdout,&mut helper_log).map_err(|e|AppError::evidence(e.to_string()))?;}
                    if let Some(mut stderr)=child.stderr.take(){std::io::copy(&mut stderr,&mut helper_log).map_err(|e|AppError::evidence(e.to_string()))?;}
                    if !status.success() {transport_failure=Some(format!("initialize helper: {status}; DLL may remain resident"));break;}
                    initialized=true;
                }
            }
            while let Ok(operation)=receiver.try_recv() {
                let operation=operation.trim();line(&mut operations,&json!({"elapsedMs":start.elapsed().as_millis(),
                    "tick":mapping.header().completed_tick.load(Ordering::Acquire),"operation":operation}))?;
                match operation {
                    "retire-b"=>mapping.header().retire_b.store(1,Ordering::Release),
                    "script"=>mapping.header().retire_b.store(2,Ordering::Release),
                    "conflict-b" if protocol.is_none()=>mapping.header().retire_b.store(3,Ordering::Release),
                    "queries"=>mapping.header().retire_b.store(4,Ordering::Release),
                    "stop"=>stopping=true,
                    "status"=>println!("state={} tick={} error={}",mapping.header().state.load(Ordering::Acquire),
                        mapping.header().completed_tick.load(Ordering::Acquire),mapping.header().error.load(Ordering::Acquire)),
                    value if value.starts_with("mark ")=>{},
                    value if value.starts_with("e2e-fault ")=>{
                        if let Some(probe)=e2e.as_mut(){probe.command(&value[10..])?;}
                        else {println!("E2E probe is not enabled");}
                    },
                    _=>println!("unknown operation; use mark <operation>, status, script, queries, conflict-b, retire-b, stop"),
                }
            }
            if start.elapsed()>Duration::from_secs(1800) || mapping.header().error.load(Ordering::Acquire)!=0 {stopping=true;}
            if process.wait(0)? {transport_failure=Some("game exited before confirmed restoration".into());break;}
            if stopping && initialized && shutdown_child.is_none() {
                let mut stop_command=hidden_helper(&options["--helper"]);
                stop_command.args(["--stop-coop-active",&process.pid.to_string(),
                    &options["--native-dll"]]).stdout(Stdio::from(helper_log.try_clone().map_err(|e|AppError::evidence(e.to_string()))?))
                    .stderr(Stdio::from(helper_log.try_clone().map_err(|e|AppError::evidence(e.to_string()))?));
                cleanup.armed=false; // Even a spawn failure must not trigger a second stop.
                shutdown_child=Some(stop_command.spawn()
                    .map_err(|e|AppError::evidence(format!("shutdown helper: {e}")))?);
            }
            if let Some(shutdown)=shutdown_child.as_mut() {
                if let Some(status)=shutdown.try_wait().map_err(|e|AppError::evidence(e.to_string()))? {
                    if !status.success(){transport_failure=Some(format!("shutdown helper: {status}; do not unload or reinject"));}
                    break;
                }
            }
            if !initialized && start.elapsed()>Duration::from_secs(30) {transport_failure=Some("initialize helper timed out; do not retry injection".into());break;}
            // Publish complete JSON lines for the live orchestrator each round.
            for file in [&mut events_file,&mut loops_file,&mut operations] {
                file.flush().map_err(|e|AppError::evidence(format!("capture log flush: {e}")))?;
            }
            if let Some(file)=protocol_file.as_mut(){file.flush().map_err(|e|AppError::evidence(format!("protocol log flush: {e}")))?;}
            thread::sleep(Duration::from_millis(if protocol.is_some(){2}else{20}));
        }
        Ok(())
        })();
        if let Err(error)=capture_result {
            transport_failure=Some(error.to_string());
        }
        let mut cleanup_errors=Vec::<String>::new();
        // Complete the sole normal cleanup while mappings and logs remain alive.
        // Errors below are collected separately and cannot hide the first failure.
        if cleanup.armed {
            cleanup.armed=false;
            match hidden_helper(&options["--helper"]).args(["--stop-coop-active",&process.pid.to_string(),
                &options["--native-dll"]]).output() {
                Ok(output)=>{
                    if let Err(error)=helper_log.write_all(&output.stdout).and_then(|()|helper_log.write_all(&output.stderr)) {
                        cleanup_errors.push(format!("cleanup helper log: {error}"));
                    }
                    if !output.status.success(){cleanup_errors.push(format!("cleanup helper: {}",output.status));}
                },
                Err(error)=>cleanup_errors.push(format!("cleanup helper: {error}; do not unload or reinject")),
            }
        } else if let Some(shutdown)=shutdown_child.as_mut() {
            match shutdown.wait() {
                Ok(status) if status.success()=>{},
                Ok(status)=>cleanup_errors.push(format!("shutdown helper: {status}")),
                Err(error)=>cleanup_errors.push(format!("shutdown wait: {error}")),
            }
        }
        // Preserve events published between the last poll and helper exit,
        // including initialization failures. This does not retry initialization.
        match mapping.drain(&mut next) {
            Ok(tail)=>for event in tail {
                if let Err(error)=line(&mut events_file,&event){cleanup_errors.push(error.to_string());}
                events.push(event);
            },
            Err(error)=>cleanup_errors.push(format!("final event drain: {error}")),
        }
        if let Some(file)=protocol_file.as_mut(){
            if let Err(error)=mapping.drain_protocol(&mut protocol_next,file,true){cleanup_errors.push(format!("final protocol drain: {error}"));}
            if mapping.protocol_counters()["lost"].as_u64()!=Some(0){cleanup_errors.push("protocol trace overflow; queued tail retained, trace remains incomplete".into());}
        }
        match companion.drain(&mut loop_next) {
            Ok(tail)=>for record in tail {
                if let Err(error)=loop_line(&mut loops_file,&record){cleanup_errors.push(error.to_string());}
            },
            Err(error)=>cleanup_errors.push(format!("final loop drain: {error}")),
        }
        let restoration:Vec<_>=events.iter().filter(|e|e.kind==15).collect();
        let restoration_report=json!({"targets":restoration,
            "nativeRestored":mapping.header().restored.load(Ordering::Acquire)!=0,
            "dllResidentExpected":true,"pinned":mapping.header().pinned.load(Ordering::Acquire)!=0,
            "controllerError":transport_failure,"cleanupErrors":cleanup_errors,
            "protocolCounters":if protocol.is_some(){Some(mapping.protocol_counters())}else{None}});
        write_json(&directory.join("restoration.json"),&restoration_report)?;
        if let Some(world)=world.as_mut(){if let Err(error)=world.finish(&restoration_report){cleanup_errors.push(format!("world finish: {error}"));}}
        if let Some(probe)=e2e.as_mut(){if let Err(error)=probe.finish(){cleanup_errors.push(format!("probe finish: {error}"));}}
        for file in [&mut events_file,&mut loops_file,&mut operations] {
            if let Err(error)=file.flush(){cleanup_errors.push(format!("final log flush: {error}"));}
        }
        if let Some(file)=protocol_file.as_mut(){if let Err(error)=file.flush(){cleanup_errors.push(format!("final protocol flush: {error}"));}}
        let mut report=if let Some(world)=world.as_ref() {
            let restored=mapping.header().restored.load(Ordering::Acquire)!=0;
            let rows=events.iter().filter(|event|event.kind==15 && event.action!=u32::MAX && event.boolean==1 && event.before==event.after).count();
            let catalog=events.iter().any(|event|event.kind==15 && event.action==u32::MAX && event.detail==70 && event.boolean==1);
            let complete=world.healthy_closed() && restored && rows==70 && catalog &&
                mapping.header().error.load(Ordering::Acquire)==0 && transport_failure.is_none();
            json!({"status":if complete{"CAPTURE_CLOSED"}else{"FAIL"},"tx":"tx-01-73",
                "restoredTargets":rows,"nativeRestored":restored,"transportFailure":transport_failure,
                "nativeError":mapping.header().error.load(Ordering::Acquire),"runtimeAcceptance":"NOT_RUN"})
        } else {
            analyze(&events,transport_failure.as_deref(),mapping.header().error.load(Ordering::Acquire),
                mapping.header().pinned.load(Ordering::Acquire)!=0,mapping.header().restored.load(Ordering::Acquire)!=0,b_character)
        };
        if let Some(world)=world.as_ref() {
            report["worldRecord"]=json!({"profile":crate::world_record::PROFILE,"directory":"world","closedWithoutCaptureError":world.healthy_closed(),"runtimeAcceptance":"NOT_RUN"});
        }
        if let Some(config)=protocol {
            report["protocol"]=json!({"wireVersion":2,"faultConfig":config,"traceRecords":protocol_next,
                "counters":mapping.protocol_counters(),"ringCapacity":PROTOCOL_TRACE_CAPACITY,
                "transport":"owned in-memory logical Remote/Authority; no Steam/socket"});
        }
        report["controllerError"]=json!(transport_failure);
        report["cleanupErrors"]=json!(cleanup_errors);
        if transport_failure.is_some() || !cleanup_errors.is_empty(){report["status"]=json!("FAIL");}
        write_json(&directory.join("result.json"),&report)?;
        println!("{}",serde_json::to_string_pretty(&report).map_err(|e|AppError::evidence(e.to_string()))?);
        if report["status"]!="PASS" && report["status"]!="CAPTURE_CLOSED" {
            return Err(AppError::evidence(transport_failure.unwrap_or_else(||"E2E did not pass; inspect result.json; no automatic retry".into())));
        }
        Ok(())
    }

    fn diagnostic_name(stage:u32,reason:u32)->&'static str {
        if stage==3 || stage==4 {return if reason==0{"READY"}else{"OWNED_DEVICE_ERROR"};}
        match reason {
            0=>"READY",1=>"ROOT_ABSENT",2=>"RUN_INACTIVE",3=>"ONLINE_ROSTER",4=>"ONLINE_FRAME",
            5=>"INPUT_CALLBACK_13_ENABLED",6=>"CALLBACK_MASK_UNAVAILABLE",7=>"PLAYER_VECTOR",
            8=>"PLAYER_ENTRY",9=>"PLAYER_VTABLE",10=>"PLAYER_ORDINAL",11=>"TOPOLOGY_CHANGED",12=>"READ_FAULT",
            20=>"KEY_PENDING",21=>"PAIR_VECTOR",22=>"PAIR_ENTRY",23=>"KEY_NOT_UNIQUE",24=>"PHYSICAL_IDENTITY",
            25=>"OWNED_IDENTITY",26=>"DEAD",27=>"PLAYER_ABSENT",28=>"OWNED_PLAYER_KIND",40=>"JOIN_NOT_ELIGIBLE",41=>"JOIN_SCOPE_EXHAUSTED",
            _=>"UNKNOWN_REASON",
        }
    }
    fn analyze(events:&[Event],transport_failure:Option<&str>,native_error:i32,pinned:bool,restored:bool,b_character:i32)->serde_json::Value {
        let mut checks=Vec::new();
        let retire_tick=events.iter().find(|e|e.kind==14 && e.ordinal==1 && e.boolean==1).map(|e|e.tick);
        let mut check=|name:&str,observed:bool| {
            let sequences:Vec<_>=events.iter().filter(|e| {
                if name=="single-device-registration" {e.kind==2}
                else if name=="single-native-create-and-publish" {e.kind==11 || e.kind==12}
                else if name=="B-character-selection" {e.kind==30}
                else if name=="A-B-native-identities-match" {e.kind==2 || e.kind==3 || e.kind==12}
                else if name.starts_with("participant-") {let parts:Vec<_>=name.split('-').collect();
                    e.ordinal.to_string()==parts[1] && if parts[2]=="binding" {e.kind==3}else{e.kind==7}}
                else if name.starts_with("input-decision-") {e.kind==4 && name.ends_with(&format!("-{}",e.decision))}
                else if name.starts_with("lifecycle-") {e.kind==13 && name.ends_with(&format!("-{}",e.detail))}
                else if name.starts_with("explicit-edge-resolution-") {
                    let action=name.rsplit('-').next().and_then(|v|v.parse::<u32>().ok()).unwrap_or(32);
                    e.kind==5 && e.ordinal==1 && (8..12).contains(&action) && e.state.edge_flags&(1<<(action-8))!=0
                }
                else if name.starts_with("held-") {e.kind==6 && e.query==1 && e.boolean==1 && name.ends_with(&format!("-{}",e.action))}
                else if name.starts_with("B-native-consumer-state-") {e.kind==7 && e.ordinal==1 && e.boolean==1 && e.before!=e.after && name.ends_with(&format!("-{}",e.detail))}
                else if name=="A-physical-loopback" {e.kind==17}
                else if name=="first-current-explicit-edge-without-previous" {e.kind==6 && e.decision==1 && e.boolean==1 && e.query==2 && (8..12).contains(&e.action)}
                else if name=="B-retired-with-root" {e.kind==14}
                else if name=="A-continues-after-B" {e.kind==14 || (e.kind==5 && e.ordinal==0 && e.boolean==1 && retire_tick.is_some_and(|tick|e.tick>tick))}
                else if name=="missing-input-neutral" {e.kind==5 && e.command==0}
                else if name=="repeated-query-pure" {e.kind==6}
                else {e.kind==15}
            }).map(|e|e.published).take(64).collect();
            checks.push(json!({"name":name,"status":if observed{"PASS"}else{"NOT_OBSERVED"},
                "evidenceFile":"events.jsonl","eventSequences":sequences}));
        };
        let select=|kind:u32|events.iter().filter(move|e|e.kind==kind).collect::<Vec<_>>();
        check("single-device-registration",select(2).iter().filter(|e|e.detail==5).count()==1);
        check("single-native-create-and-publish",select(11).len()==1 && select(12).len()==1);
        let character_events=select(30);
        let selected=character_events.iter().find(|e|e.action==2 && e.boolean==1);
        let created=character_events.iter().find(|e|e.action==3 && e.boolean==1);
        let character_matches=selected.zip(created).is_some_and(|(s,c)|
            s.decision==c.decision && c.query==s.decision && c.after[2..4]==[1,0] &&
            (b_character<0 || c.decision==b_character as u32));
        check("B-character-selection",character_matches);
        let native_identities_match=select(3).iter().any(|a|a.ordinal==0 && a.before[3]==0 && a.after[0..3]==[1,0,0] &&
            select(3).iter().any(|b|b.ordinal==1 && b.before[3]==1 && b.after[0..3]==[2,0,1] && a.action!=b.action &&
                select(2).iter().any(|device|device.detail==5 && device.boolean==1 && device.before[0]==b.action &&
                    device.before[1]==b.before[1] && device.before[2]==b.before[2])));
        check("A-B-native-identities-match",native_identities_match);
        for ordinal in 0..2 {
            check(&format!("participant-{ordinal}-binding"),select(3).iter().any(|e|e.ordinal==ordinal));
            for consumer in [1,5] {
                check(&format!("participant-{ordinal}-world-{consumer}"),select(7).iter().any(|e|
                    e.ordinal==ordinal && e.detail==consumer && e.boolean==1 && e.before!=e.after &&
                    (e.state.move_x!=0 || e.state.move_y!=0 || e.state.aim_x!=0 || e.state.aim_y!=0)));
            }
        }
        for decision in [2,5,7,12,3,11] {check(&format!("input-decision-{decision}"),select(4).iter().any(|e|e.decision==decision));}
        check("A-physical-loopback",select(17).iter().any(|e|e.ordinal==0));
        check("resident-dll-and-restoration",pinned && restored && transport_failure.is_none());
        check("first-current-explicit-edge-without-previous",select(6).iter().any(|e|
            (8..12).contains(&e.action) && e.query==2 && e.boolean==1 && e.decision==1));
        check("missing-input-neutral",select(5).iter().any(|e|e.command==0 && e.state.held_flags==0 &&
            e.state.edge_flags==0 && e.state.move_x==0 && e.state.move_y==0 && e.state.aim_x==0 && e.state.aim_y==0));
        for action in 8..12 {check(&format!("explicit-edge-resolution-{action}"),select(5).iter().any(|e|
            e.ordinal==1 && e.command!=0 && e.state.edge_flags&(1<<(action-8))!=0));}
        for consumer in [2,3,4,6] {check(&format!("B-native-consumer-state-{consumer}"),select(7).iter().any(|e|
            e.ordinal==1 && e.detail==consumer && e.boolean==1 && e.decision==1 && e.before!=e.after));}
        for action in 9..12 {check(&format!("held-{action}"),select(6).iter().any(|e|e.action==action && e.query==1 && e.boolean==1));}
        for lifecycle in [1,2,3,4,5,6,7,9] {check(&format!("lifecycle-{lifecycle}"),select(13).iter().any(|e|e.detail==lifecycle));}
        let retired=select(14).into_iter().find(|e|e.ordinal==1 && e.boolean==1 && e.detail>=1);
        check("B-retired-with-root",retired.is_some());
        check("A-continues-after-B",retired.is_some_and(|r|select(5).iter().any(|e|e.ordinal==0 && e.tick>r.tick && e.boolean==1)));
        check("all-51-hooks-restored",select(15).iter().filter(|e|e.action!=u32::MAX && e.boolean==1 && e.before==e.after).count()==51 &&
            select(15).iter().any(|e|e.action==u32::MAX && e.detail==51 && e.boolean==1));
        let mut resolved=std::collections::BTreeSet::new();
        let duplicate_resolve=select(5).iter().any(|e|!resolved.insert((e.ordinal,e.tick)));
        let mut ticks=std::collections::BTreeSet::new();
        let duplicate_commit=select(8).iter().any(|e|!ticks.insert(e.tick));
        let mut reads=std::collections::BTreeMap::new();let mut inconsistent_query=false;let mut repeated_query=false;
        for e in select(6) {
            if e.decision&1==0 {if e.boolean!=0 || e.scalar!=0.0 {inconsistent_query=true;}continue;}
            let key=(e.ordinal,e.tick,e.generation,e.action,e.query,e.detail);
            let value=(e.scalar.to_bits(),e.boolean);
            if let Some(previous)=reads.insert(key,value) {repeated_query=true;if previous!=value {inconsistent_query=true;}}
            if e.query==2 && (8..12).contains(&e.action) && e.boolean!=(e.state.edge_flags&(1<<(e.action-8))!=0) as u32 {inconsistent_query=true;}
        }
        check("repeated-query-pure",repeated_query && !inconsistent_query);
        let mut consumers=std::collections::BTreeSet::new();
        let duplicate_consumer=select(7).iter().filter(|e|e.ordinal<2 && e.decision==1)
            .any(|e|!consumers.insert((e.ordinal,e.tick,e.detail)));
        let mut accepted=std::collections::BTreeSet::new();
        let duplicate_accept=select(4).iter().filter(|e|e.decision==1).any(|e|!accepted.insert((e.ordinal,e.command)));
        let failure=transport_failure.is_some() || native_error!=0 || !select(16).is_empty() ||
            character_events.iter().any(|e|e.action>=2 && e.boolean==0) ||
            duplicate_resolve || duplicate_commit || inconsistent_query || duplicate_consumer || duplicate_accept;
        let complete=checks.iter().all(|v|v["status"]=="PASS");
        let mut diagnostics=BTreeMap::new();
        for event in select(19) {
            let entry=diagnostics.entry((event.action,event.ordinal,event.detail)).or_insert((event,0usize));
            entry.0=event;entry.1+=1;
        }
        let diagnostic_summary:Vec<_>=diagnostics.into_iter().map(|((stage,ordinal,reason),(last,count))|json!({
            "stage":match stage {1=>"TOPOLOGY",2=>"BINDING",3=>"DEVICE_CREATE",4=>"DEVICE_REGISTER",5=>"JOIN",6=>"PAIR_LOOKUP",_=>"UNKNOWN_STAGE"},
            "stageCode":stage,"ordinal":ordinal,"reason":diagnostic_name(stage,reason),"reasonCode":reason,
            "transitionCount":count,"lastEventSequence":last.published,"before":last.before,"after":last.after,
        })).collect();
        json!({"tx":"tx-01-45","status":if failure{"FAIL"}else if complete{"PASS"}else{"NOT_OBSERVED"},
            "bCharacter":{"requested":character_choice(b_character),"selected":selected.map(|e|e.decision),
                "actual":created.map(|e|e.query),"matches":character_matches,"events":character_events},
            "checks":checks,"diagnostics":diagnostic_summary,"transportFailure":transport_failure,"nativeError":native_error,
            "duplicateResolve":duplicate_resolve,"duplicateWorldCommit":duplicate_commit,
            "inconsistentQuery":inconsistent_query,"duplicateConsumer":duplicate_consumer,"duplicateAccept":duplicate_accept,
            "eventCount":events.len(),"automaticRetry":false,
            "boundary":"normal sequential allocation/buffers only; no OOM/EH/worker closure"})
    }
}
