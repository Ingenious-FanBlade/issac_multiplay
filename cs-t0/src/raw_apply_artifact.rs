use std::collections::{BTreeMap, BTreeSet, HashMap, HashSet};
use std::fs::{self, OpenOptions};
use std::io::{BufRead, BufReader, BufWriter, Write};
use std::path::{Component, Path, PathBuf};

use serde::{Deserialize, Serialize};

use crate::artifact::file_sha256;
use crate::error::UsageContext;
use crate::error::{AppError, AppResult};
use crate::input_artifact::EVIDENCE_PASSIVE_INPUT_RUNTIME;
use crate::loop_ipc::LoopPhase;
use crate::raw_apply_ipc::{
    NativeRawApplyEventRecord, PassiveWitnessKind, RAW_APPLY_CONSUMER_HOOK_MASK,
    RAW_APPLY_FLAG_CONSUMER_KNOWN, RAW_APPLY_FLAG_CONTEXT_KNOWN, RAW_APPLY_FLAG_GAMEPAD_DEVICE,
    RAW_APPLY_FLAG_KEYBOARD_DEVICE, RAW_APPLY_FLAG_LOOP_KNOWN, RAW_APPLY_FLAG_MAIN_THREAD,
    RAW_APPLY_FLAG_ORIGINAL_CALLED, RAW_APPLY_FLAG_PLAYER_ORDINAL_VALID,
    RAW_APPLY_FLAG_RAW_FALLING, RAW_APPLY_FLAG_RAW_RISING, RAW_APPLY_FLAG_REDACTED,
    RAW_APPLY_FLAG_RESULT_TRUE, RAW_APPLY_FLAG_SAMPLE_KNOWN, RAW_APPLY_FLAG_VALUE_VALID,
    RAW_APPLY_FLAG_WITNESS_VALID, RAW_APPLY_FLAG_WRONG_THREAD, RAW_APPLY_IPC_MAGIC,
    RAW_APPLY_IPC_SCHEMA_VERSION, RAW_APPLY_KNOWN_FLAGS, RAW_APPLY_LEGACY_HOOK_MASK,
    RAW_APPLY_NO_ACTION, RAW_APPLY_NO_CONTROLLER_ID, RAW_APPLY_NO_DEVICE_KEY,
    RAW_APPLY_NO_GENERATION, RAW_APPLY_NO_PLAYER_ORDINAL, RAW_APPLY_NO_PRIMITIVE,
    RAW_APPLY_SOURCE_MASK, RawApplyClassification, RawApplyEventKind, RawChannelKind,
};

pub const RAW_APPLY_ARTIFACT_SCHEMA_VERSION: u32 = 3;
pub const EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME: &str = "PASSIVE_RAW_APPLY_RUNTIME";
pub const RAW_APPLY_IPC_SOURCE_FORMAT: &str = "ICSI04V1";
pub const RAW_APPLY_CATALOG_SHA256: &str =
    "E22323047082E6289891393DE0B467CA5038E15EBD45310F39B5051FD0EB13D6";
pub const RAW_APPLY_CAPTURE_PLAN_SHA256: &str =
    "203F64825EA24EE4EFA21B731FD2CB8E15AB11DA87145F7654C545B676A45562";
const RAW_APPLY_EVENT_BUFFER_CAPACITY_BYTES: usize = 4 * 1024 * 1024;
pub const RAW_APPLY_RESTORED_SLOT_COUNT: u32 = 50;
pub const RAW_APPLY_TARGET_BYTES_SCHEMA_VERSION: u32 = 1;
pub const RAW_APPLY_OWNER_HOOK_SITE_RVA: u32 = 0x0076_BDDC;
pub const RAW_APPLY_KEYBOARD_POLL_HOOK_SITE_RVA: u32 = 0x007A_16F0;
pub const RAW_APPLY_GAMEPAD_POLL_HOOK_SITE_RVA: u32 = 0x0078_2CF8;
pub const RAW_APPLY_PHYSICAL_POLL_RETURN_RVA: u32 = 0x0061_F30C;
pub const RAW_APPLY_CONSUMER_HOOK_SITE_RVAS: [u32; 6] = [
    0x0038_5BC2,
    0x0038_5BC9,
    0x0038_5BD2,
    0x0038_5BD9,
    0x0038_5BE0,
    0x0038_5BE7,
];
pub const RAW_APPLY_TARGET_SLOTS: [(&str, u32, usize); 18] = [
    ("manager-pressed", 0x0078_2990, 4),
    ("manager-triggered", 0x0078_2994, 4),
    ("manager-scalar", 0x0078_2998, 4),
    ("keyboard-logical-pressed", 0x007A_16A8, 4),
    ("keyboard-logical-triggered", 0x007A_16AC, 4),
    ("keyboard-logical-scalar", 0x007A_16B0, 4),
    ("gamepad-logical-pressed", 0x0078_2CB0, 4),
    ("gamepad-logical-triggered", 0x0078_2CB4, 4),
    ("gamepad-logical-scalar", 0x0078_2CB8, 4),
    ("keyboard-logical-setter", 0x007A_16C4, 4),
    ("gamepad-logical-setter", 0x0078_2CCC, 4),
    ("player-update-envelope", RAW_APPLY_OWNER_HOOK_SITE_RVA, 4),
    ("player-movement-consumer", 0x0038_5BC2, 5),
    ("player-bomb-consumer", 0x0038_5BC9, 5),
    ("player-item-slot-consumer", 0x0038_5BD2, 5),
    ("player-pill-card-consumer", 0x0038_5BD9, 5),
    ("player-shooting-consumer", 0x0038_5BE0, 5),
    ("player-drop-consumer", 0x0038_5BE7, 5),
];

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct RawApplyManifest {
    pub schema_version: u32,
    pub evidence_class: String,
    pub ipc_source_format: String,
    pub ipc_magic: String,
    pub ipc_schema_version: u32,
    pub eligible_for_active_gate: bool,
    pub state: String,
    pub case_id: String,
    pub probe: String,
    pub scenario: String,
    pub target_sha256: String,
    pub loop_gate_sha256: String,
    pub artifact_identity: String,
    pub native_sha256: String,
    pub helper_sha256: String,
    pub companion_loop_artifact_path: PathBuf,
    pub companion_loop_manifest_sha256: Option<String>,
    pub companion_loop_events_sha256: Option<String>,
    pub companion_loop_analysis_sha256: Option<String>,
    pub passive_catalog_sha256: String,
    pub passive_capture_plan_sha256: String,
    pub passive_legacy_installed_hooks_mask: String,
    pub passive_legacy_restored_hooks_mask: String,
    pub passive_required_raw_source_mask: String,
    pub passive_observed_raw_source_mask: String,
    pub passive_required_consumer_hooks_mask: String,
    pub passive_installed_consumer_hooks_mask: String,
    pub passive_restored_consumer_hooks_mask: String,
    pub passive_restored_slot_count: u32,
    pub passive_target_bytes_before_sha256: String,
    pub passive_target_bytes_after_sha256: String,
    pub passive_target_bytes_restored: bool,
    pub passive_main_thread_id: u32,
    pub passive_overwritten_events: u64,
    pub passive_redacted_events: u64,
    pub passive_producer_closed: bool,
    pub failure_reason: Option<String>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct RawApplyTargetSlot {
    pub id: String,
    pub rva: String,
    pub bytes_hex: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct RawApplyTargetBytesDocument {
    pub schema_version: u32,
    pub target_sha256: String,
    pub slots: Vec<RawApplyTargetSlot>,
}

#[derive(Clone, Debug)]
pub struct RawApplyRunConfig {
    pub case_id: String,
    pub scenario: String,
    pub target_sha256: String,
    pub loop_gate_sha256: String,
    pub artifact_identity: String,
    pub companion_loop_artifact_path: PathBuf,
    pub native_sha256: String,
    pub helper_sha256: String,
    pub target_bytes_before: RawApplyTargetBytesDocument,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct RawApplyFinish {
    pub main_thread_id: u32,
    pub legacy_installed_hooks_mask: u64,
    pub legacy_restored_hooks_mask: u64,
    pub required_raw_source_mask: u64,
    pub observed_raw_source_mask: u64,
    pub required_consumer_hooks_mask: u64,
    pub installed_consumer_hooks_mask: u64,
    pub restored_consumer_hooks_mask: u64,
    pub overwritten_events: u64,
    pub redacted_events: u64,
    pub producer_closed: bool,
}

pub struct RawApplyRun {
    directory: PathBuf,
    manifest: RawApplyManifest,
    events_file: BufWriter<fs::File>,
    next_event_sequence: u64,
    target_bytes_before: RawApplyTargetBytesDocument,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct RawApplyArtifactEvent {
    pub event_sequence: u64,
    pub stage: String,
    pub phase: LoopPhase,
    pub native_kind: RawApplyEventKind,
    pub native_sequence: u64,
    pub qpc: u64,
    pub loop_iteration: u64,
    pub thread_id: u32,
    pub flags: u32,
    pub classification: RawApplyClassification,
    pub error_code: Option<u32>,

    pub passive_hook_site_rva: Option<String>,
    pub passive_return_rva: Option<String>,
    pub passive_original_calls: Option<u32>,
    pub passive_device_key: Option<u32>,
    pub passive_controller_id_raw: Option<u32>,
    pub passive_selected_device_key: Option<u32>,
    pub passive_player_ordinal: Option<u32>,
    pub passive_action: Option<u32>,
    pub passive_primitive: Option<u32>,
    pub passive_value_bits: Option<u32>,
    pub passive_sample_generation: Option<u64>,
    pub passive_consumer_transaction: Option<u64>,
    pub passive_context_generation: Option<u64>,
    pub passive_raw_channel_kind: Option<RawChannelKind>,
    pub passive_raw_channel_id: Option<u32>,
    pub passive_raw_value_bits: Option<u32>,
    pub passive_witness_kind: Option<PassiveWitnessKind>,
    pub passive_witness_value_bits: Option<u32>,
    pub passive_witness_aux_bits: Option<u32>,

    // These legacy pipeline fields remain representable only so the schema-3
    // validator can prove that passive evidence never reuses their semantics.
    pub session_id: Option<String>,
    pub client_id: Option<String>,
    pub command_sequence: Option<u64>,
    pub target_tick: Option<u64>,
    pub canonical_state_hex: Option<String>,
    pub identity_key: Option<String>,
    pub queue_sequence: Option<u64>,
    pub resolution: Option<String>,
    pub applied_state_hex: Option<String>,
    pub world_witness_before: Option<String>,
    pub world_witness_after: Option<String>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct RawApplyAnalysisCheck {
    pub id: String,
    pub passed: bool,
    pub detail: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct RawApplyAnalysisReport {
    pub schema_version: u32,
    pub evidence_class: String,
    pub result: String,
    pub eligible_for_active_gate: bool,
    pub checks: Vec<RawApplyAnalysisCheck>,
}

impl RawApplyTargetBytesDocument {
    pub fn validate(&self, expected_target_sha256: &str) -> AppResult<()> {
        if self.schema_version != RAW_APPLY_TARGET_BYTES_SCHEMA_VERSION
            || self.target_sha256 != expected_target_sha256
            || self.slots.len() != RAW_APPLY_TARGET_SLOTS.len()
        {
            return Err(AppError::evidence(
                "raw/apply target-byte document has invalid identity",
            ));
        }
        for (slot, (id, rva, size)) in self.slots.iter().zip(RAW_APPLY_TARGET_SLOTS) {
            if slot.id != id
                || !slot.rva.starts_with("0x")
                || u32::from_str_radix(&slot.rva[2..], 16).ok() != Some(rva)
                || hex::decode(&slot.bytes_hex).as_ref().map(Vec::len) != Ok(size)
            {
                return Err(AppError::evidence(
                    "raw/apply target-byte document does not bind the exact ordered 18-slot target set",
                ));
            }
        }
        Ok(())
    }
}

impl RawApplyRun {
    pub fn create(output_directory: &Path, config: RawApplyRunConfig) -> AppResult<Self> {
        validate_case_id(&config.case_id)?;
        for (value, label) in [
            (&config.target_sha256, "target SHA-256"),
            (&config.loop_gate_sha256, "loop gate SHA-256"),
            (&config.native_sha256, "native SHA-256"),
            (&config.helper_sha256, "helper SHA-256"),
        ] {
            validate_sha256(value, label)?;
        }
        if !matches!(
            config.scenario.as_str(),
            "keyboard-held-release"
                | "keyboard-edge"
                | "controller-held-release"
                | "controller-edge"
        ) {
            return Err(AppError::usage("unsupported passive raw/apply scenario"));
        }
        validate_relative_path(&config.companion_loop_artifact_path)?;
        config.target_bytes_before.validate(&config.target_sha256)?;
        if config.artifact_identity.trim().is_empty() {
            return Err(AppError::usage(
                "raw/apply artifact identity cannot be empty",
            ));
        }
        fs::create_dir(output_directory).usage_context(format!(
            "refusing to reuse raw/apply artifact {}",
            output_directory.display()
        ))?;
        let before_path = output_directory.join("target-bytes-before.json");
        write_json(&before_path, &config.target_bytes_before, true)?;
        let events_file = OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(output_directory.join("events.jsonl"))
            .usage_context("cannot create raw/apply events")?;
        let events_file =
            BufWriter::with_capacity(RAW_APPLY_EVENT_BUFFER_CAPACITY_BYTES, events_file);
        let manifest = RawApplyManifest {
            schema_version: RAW_APPLY_ARTIFACT_SCHEMA_VERSION,
            evidence_class: EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME.to_owned(),
            ipc_source_format: RAW_APPLY_IPC_SOURCE_FORMAT.to_owned(),
            ipc_magic: format!("0x{RAW_APPLY_IPC_MAGIC:016X}"),
            ipc_schema_version: RAW_APPLY_IPC_SCHEMA_VERSION,
            eligible_for_active_gate: false,
            state: "RUNNING".to_owned(),
            case_id: config.case_id,
            probe: "passive".to_owned(),
            scenario: config.scenario,
            target_sha256: config.target_sha256,
            loop_gate_sha256: config.loop_gate_sha256,
            artifact_identity: config.artifact_identity,
            native_sha256: config.native_sha256,
            helper_sha256: config.helper_sha256,
            companion_loop_artifact_path: config.companion_loop_artifact_path,
            companion_loop_manifest_sha256: None,
            companion_loop_events_sha256: None,
            companion_loop_analysis_sha256: None,
            passive_catalog_sha256: RAW_APPLY_CATALOG_SHA256.to_owned(),
            passive_capture_plan_sha256: RAW_APPLY_CAPTURE_PLAN_SHA256.to_owned(),
            passive_legacy_installed_hooks_mask: "0x0".to_owned(),
            passive_legacy_restored_hooks_mask: "0x0".to_owned(),
            passive_required_raw_source_mask: format!("0x{RAW_APPLY_SOURCE_MASK:X}"),
            passive_observed_raw_source_mask: "0x0".to_owned(),
            passive_required_consumer_hooks_mask: format!("0x{RAW_APPLY_CONSUMER_HOOK_MASK:X}"),
            passive_installed_consumer_hooks_mask: "0x0".to_owned(),
            passive_restored_consumer_hooks_mask: "0x0".to_owned(),
            passive_restored_slot_count: 0,
            passive_target_bytes_before_sha256: file_sha256(&before_path)?,
            passive_target_bytes_after_sha256: String::new(),
            passive_target_bytes_restored: false,
            passive_main_thread_id: 0,
            passive_overwritten_events: 0,
            passive_redacted_events: 0,
            passive_producer_closed: false,
            failure_reason: None,
        };
        write_json(&output_directory.join("manifest.json"), &manifest, true)?;
        Ok(Self {
            directory: output_directory.to_owned(),
            manifest,
            events_file,
            next_event_sequence: 1,
            target_bytes_before: config.target_bytes_before,
        })
    }

    pub fn append_native(&mut self, record: &NativeRawApplyEventRecord) -> AppResult<()> {
        let event = raw_apply_event_from_native(record, self.next_event_sequence)?;
        serde_json::to_writer(&mut self.events_file, &event)
            .usage_context("cannot serialize raw/apply event")?;
        self.events_file
            .write_all(b"\n")
            .usage_context("cannot append raw/apply event")?;
        self.next_event_sequence = self
            .next_event_sequence
            .checked_add(1)
            .ok_or_else(|| AppError::evidence("raw/apply artifact sequence overflow"))?;
        Ok(())
    }

    pub fn finish(
        mut self,
        target_bytes_after: &RawApplyTargetBytesDocument,
        summary: RawApplyFinish,
    ) -> AppResult<RawApplyAnalysisReport> {
        target_bytes_after.validate(&self.manifest.target_sha256)?;
        self.events_file
            .flush()
            .usage_context("cannot flush raw/apply events")?;
        let after_path = self.directory.join("target-bytes-after.json");
        write_json(&after_path, target_bytes_after, true)?;
        self.bind_summary(target_bytes_after, summary, None)?;
        analyze_raw_apply_run(&self.directory)
    }

    pub fn fail(
        mut self,
        reason: &str,
        target_bytes_after: Option<&RawApplyTargetBytesDocument>,
        summary: RawApplyFinish,
    ) -> AppResult<()> {
        if reason.trim().is_empty() {
            return Err(AppError::usage(
                "raw/apply failure requires a nonempty reason",
            ));
        }
        self.events_file
            .flush()
            .usage_context("cannot flush failed raw/apply events")?;
        if let Some(after) = target_bytes_after {
            after.validate(&self.manifest.target_sha256)?;
            write_json(&self.directory.join("target-bytes-after.json"), after, true)?;
            self.bind_summary(after, summary, Some(reason))?;
        } else {
            self.apply_summary(summary);
            self.manifest.state = "FAILED".to_owned();
            self.manifest.failure_reason = Some(reason.to_owned());
            write_json(&self.directory.join("manifest.json"), &self.manifest, false)?;
        }
        Ok(())
    }

    fn bind_summary(
        &mut self,
        target_bytes_after: &RawApplyTargetBytesDocument,
        summary: RawApplyFinish,
        failure_reason: Option<&str>,
    ) -> AppResult<()> {
        self.apply_summary(summary);
        let after_path = self.directory.join("target-bytes-after.json");
        self.manifest.passive_target_bytes_after_sha256 = file_sha256(&after_path)?;
        self.manifest.passive_target_bytes_restored =
            self.target_bytes_before == *target_bytes_after;
        self.manifest.passive_restored_slot_count = if self.manifest.passive_target_bytes_restored {
            RAW_APPLY_RESTORED_SLOT_COUNT
        } else {
            0
        };
        match companion_loop_hashes(&self.directory, &self.manifest) {
            Ok((manifest, events, analysis)) => {
                self.manifest.companion_loop_manifest_sha256 = Some(manifest);
                self.manifest.companion_loop_events_sha256 = Some(events);
                self.manifest.companion_loop_analysis_sha256 = Some(analysis);
            }
            Err(error) if failure_reason.is_none() => return Err(error),
            Err(_) => {}
        }
        self.manifest.failure_reason = failure_reason.map(str::to_owned);
        self.manifest.state = if failure_reason.is_some() {
            "FAILED"
        } else {
            "COMPLETE"
        }
        .to_owned();
        write_json(&self.directory.join("manifest.json"), &self.manifest, false)
    }

    fn apply_summary(&mut self, summary: RawApplyFinish) {
        self.manifest.passive_main_thread_id = summary.main_thread_id;
        self.manifest.passive_legacy_installed_hooks_mask =
            format!("0x{:X}", summary.legacy_installed_hooks_mask);
        self.manifest.passive_legacy_restored_hooks_mask =
            format!("0x{:X}", summary.legacy_restored_hooks_mask);
        self.manifest.passive_required_raw_source_mask =
            format!("0x{:X}", summary.required_raw_source_mask);
        self.manifest.passive_observed_raw_source_mask =
            format!("0x{:X}", summary.observed_raw_source_mask);
        self.manifest.passive_required_consumer_hooks_mask =
            format!("0x{:X}", summary.required_consumer_hooks_mask);
        self.manifest.passive_installed_consumer_hooks_mask =
            format!("0x{:X}", summary.installed_consumer_hooks_mask);
        self.manifest.passive_restored_consumer_hooks_mask =
            format!("0x{:X}", summary.restored_consumer_hooks_mask);
        self.manifest.passive_overwritten_events = summary.overwritten_events;
        self.manifest.passive_redacted_events = summary.redacted_events;
        self.manifest.passive_producer_closed = summary.producer_closed;
    }
}

fn validate_case_id(case_id: &str) -> AppResult<()> {
    if case_id.is_empty()
        || case_id.len() > 64
        || !case_id
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_'))
    {
        return Err(AppError::usage("invalid raw/apply case id"));
    }
    Ok(())
}

fn validate_sha256(value: &str, label: &str) -> AppResult<()> {
    if value.len() != 64 || !value.bytes().all(|byte| byte.is_ascii_hexdigit()) {
        return Err(AppError::usage(format!("invalid {label}")));
    }
    Ok(())
}

fn validate_relative_path(path: &Path) -> AppResult<()> {
    if path.as_os_str().is_empty()
        || path.is_absolute()
        || path
            .components()
            .any(|component| !matches!(component, Component::Normal(_)))
    {
        return Err(AppError::usage(
            "raw/apply companion path must be normalized and relative",
        ));
    }
    Ok(())
}

fn write_json<T: Serialize>(path: &Path, value: &T, create_new: bool) -> AppResult<()> {
    let mut options = OpenOptions::new();
    options.write(true);
    if create_new {
        options.create_new(true);
    } else {
        options.create(true).truncate(true);
    }
    let mut file = options
        .open(path)
        .usage_context(format!("cannot write {}", path.display()))?;
    serde_json::to_writer_pretty(&mut file, value)
        .usage_context(format!("cannot serialize {}", path.display()))?;
    file.write_all(b"\n")
        .usage_context(format!("cannot finish {}", path.display()))
}

fn optional_sentinel(value: u32, sentinel: u32) -> Option<u32> {
    (value != sentinel).then_some(value)
}

fn optional_generation(value: u64) -> Option<u64> {
    (value != RAW_APPLY_NO_GENERATION).then_some(value)
}

fn rva(value: u32) -> Option<String> {
    (value != 0).then(|| format!("0x{value:08X}"))
}

fn stage_for(kind: RawApplyEventKind) -> &'static str {
    match kind {
        RawApplyEventKind::NativeInitialized
        | RawApplyEventKind::HooksInstalled
        | RawApplyEventKind::HooksRestored
        | RawApplyEventKind::NativeRetired
        | RawApplyEventKind::Failure => "native-lifecycle",
        RawApplyEventKind::LogicalPressedEnter
        | RawApplyEventKind::LogicalPressedExit
        | RawApplyEventKind::LogicalTriggeredEnter
        | RawApplyEventKind::LogicalTriggeredExit
        | RawApplyEventKind::LogicalScalarEnter
        | RawApplyEventKind::LogicalScalarExit => "passive-logical-query",
        RawApplyEventKind::ManagerPressedEnter
        | RawApplyEventKind::ManagerPressedExit
        | RawApplyEventKind::ManagerTriggeredEnter
        | RawApplyEventKind::ManagerTriggeredExit
        | RawApplyEventKind::ManagerScalarEnter
        | RawApplyEventKind::ManagerScalarExit => "passive-manager-query",
        RawApplyEventKind::LogicalSetterEnter | RawApplyEventKind::LogicalSetterExit => {
            "passive-logical-setter"
        }
        RawApplyEventKind::RawPollEnter
        | RawApplyEventKind::RawValue
        | RawApplyEventKind::RawPollExit => "passive-raw-sample",
        RawApplyEventKind::PlayerConsumerEnter | RawApplyEventKind::PlayerConsumerExit => {
            "passive-player-consumer"
        }
        RawApplyEventKind::WorldWitnessBefore | RawApplyEventKind::WorldWitnessAfter => {
            "passive-world-witness"
        }
        RawApplyEventKind::ContextOwner => "passive-context-owner",
        RawApplyEventKind::ContextBinding => "passive-context-binding",
    }
}

fn is_enter(kind: RawApplyEventKind) -> bool {
    matches!(
        kind,
        RawApplyEventKind::LogicalPressedEnter
            | RawApplyEventKind::LogicalTriggeredEnter
            | RawApplyEventKind::LogicalScalarEnter
            | RawApplyEventKind::ManagerPressedEnter
            | RawApplyEventKind::ManagerTriggeredEnter
            | RawApplyEventKind::ManagerScalarEnter
            | RawApplyEventKind::LogicalSetterEnter
            | RawApplyEventKind::RawPollEnter
            | RawApplyEventKind::PlayerConsumerEnter
    )
}

fn is_exit(kind: RawApplyEventKind) -> bool {
    matches!(
        kind,
        RawApplyEventKind::LogicalPressedExit
            | RawApplyEventKind::LogicalTriggeredExit
            | RawApplyEventKind::LogicalScalarExit
            | RawApplyEventKind::ManagerPressedExit
            | RawApplyEventKind::ManagerTriggeredExit
            | RawApplyEventKind::ManagerScalarExit
            | RawApplyEventKind::LogicalSetterExit
            | RawApplyEventKind::RawPollExit
            | RawApplyEventKind::PlayerConsumerExit
    )
}

fn matching_exit(enter: RawApplyEventKind) -> Option<RawApplyEventKind> {
    Some(match enter {
        RawApplyEventKind::LogicalPressedEnter => RawApplyEventKind::LogicalPressedExit,
        RawApplyEventKind::LogicalTriggeredEnter => RawApplyEventKind::LogicalTriggeredExit,
        RawApplyEventKind::LogicalScalarEnter => RawApplyEventKind::LogicalScalarExit,
        RawApplyEventKind::ManagerPressedEnter => RawApplyEventKind::ManagerPressedExit,
        RawApplyEventKind::ManagerTriggeredEnter => RawApplyEventKind::ManagerTriggeredExit,
        RawApplyEventKind::ManagerScalarEnter => RawApplyEventKind::ManagerScalarExit,
        RawApplyEventKind::LogicalSetterEnter => RawApplyEventKind::LogicalSetterExit,
        RawApplyEventKind::RawPollEnter => RawApplyEventKind::RawPollExit,
        RawApplyEventKind::PlayerConsumerEnter => RawApplyEventKind::PlayerConsumerExit,
        _ => return None,
    })
}

fn lifecycle_payload_empty(record: &NativeRawApplyEventRecord, kind: RawApplyEventKind) -> bool {
    let event = &record.event;
    event.phase == LoopPhase::OutsideEnvelope as u32
        && event.flags == 0
        && event.loop_iteration == 0
        && event.sample_generation == 0
        && event.consumer_transaction == 0
        && event.context_generation == 0
        && event.hook_site_rva == 0
        && event.return_rva == 0
        && event.original_calls == 0
        && event.device_key == RAW_APPLY_NO_DEVICE_KEY
        && event.controller_id_raw == RAW_APPLY_NO_CONTROLLER_ID
        && event.selected_device_key == RAW_APPLY_NO_DEVICE_KEY
        && event.player_ordinal == RAW_APPLY_NO_PLAYER_ORDINAL
        && event.action == RAW_APPLY_NO_ACTION
        && event.primitive == RAW_APPLY_NO_PRIMITIVE
        && event.raw_channel_kind == RawChannelKind::None as u32
        && event.raw_channel_id == 0
        && event.value_bits == 0
        && event.classification == RawApplyClassification::Unknown as u32
        && event.witness_kind == PassiveWitnessKind::None as u32
        && event.witness_aux_bits == 0
        && if kind == RawApplyEventKind::Failure {
            event.error_code != 0
        } else {
            event.error_code == 0
        }
}

fn flags_linkage_matches(event: &crate::raw_apply_ipc::NativeRawApplyEvent) -> bool {
    let linked = [
        (RAW_APPLY_FLAG_SAMPLE_KNOWN, event.sample_generation != 0),
        (
            RAW_APPLY_FLAG_CONSUMER_KNOWN,
            event.consumer_transaction != 0,
        ),
        (RAW_APPLY_FLAG_CONTEXT_KNOWN, event.context_generation != 0),
        (
            RAW_APPLY_FLAG_PLAYER_ORDINAL_VALID,
            event.player_ordinal != RAW_APPLY_NO_PLAYER_ORDINAL,
        ),
        (
            RAW_APPLY_FLAG_WITNESS_VALID,
            event.witness_kind != PassiveWitnessKind::None as u32,
        ),
    ];
    linked
        .into_iter()
        .all(|(flag, present)| (event.flags & flag != 0) == present)
}

fn runtime_payload_valid(
    record: &NativeRawApplyEventRecord,
    kind: RawApplyEventKind,
    classification: RawApplyClassification,
    raw_kind: RawChannelKind,
    witness_kind: PassiveWitnessKind,
) -> bool {
    let event = &record.event;
    if event.error_code != 0
        || event.flags & !RAW_APPLY_KNOWN_FLAGS != 0
        || event.flags & (RAW_APPLY_FLAG_WRONG_THREAD | RAW_APPLY_FLAG_REDACTED) != 0
        || event.flags & (RAW_APPLY_FLAG_LOOP_KNOWN | RAW_APPLY_FLAG_MAIN_THREAD)
            != RAW_APPLY_FLAG_LOOP_KNOWN | RAW_APPLY_FLAG_MAIN_THREAD
        || !flags_linkage_matches(event)
        || event.flags & RAW_APPLY_FLAG_RAW_RISING != 0
            && event.flags & RAW_APPLY_FLAG_RAW_FALLING != 0
    {
        return false;
    }
    let device_flags =
        event.flags & (RAW_APPLY_FLAG_KEYBOARD_DEVICE | RAW_APPLY_FLAG_GAMEPAD_DEVICE);
    if device_flags != RAW_APPLY_FLAG_KEYBOARD_DEVICE
        && device_flags != RAW_APPLY_FLAG_GAMEPAD_DEVICE
    {
        return false;
    }
    if event.device_key == RAW_APPLY_NO_DEVICE_KEY
        || (event.action != RAW_APPLY_NO_ACTION && event.action > 11)
        || event.primitive != RAW_APPLY_NO_PRIMITIVE
    {
        return false;
    }

    let original = event.flags & RAW_APPLY_FLAG_ORIGINAL_CALLED != 0;
    let value_valid = event.flags & RAW_APPLY_FLAG_VALUE_VALID != 0;
    let result_true = event.flags & RAW_APPLY_FLAG_RESULT_TRUE != 0;
    if is_enter(kind) && (event.original_calls != 0 || original) {
        return false;
    }
    if is_exit(kind) && (event.original_calls != 1 || !original) {
        return false;
    }

    let is_bool_query = matches!(
        kind,
        RawApplyEventKind::LogicalPressedEnter
            | RawApplyEventKind::LogicalPressedExit
            | RawApplyEventKind::LogicalTriggeredEnter
            | RawApplyEventKind::LogicalTriggeredExit
            | RawApplyEventKind::ManagerPressedEnter
            | RawApplyEventKind::ManagerPressedExit
            | RawApplyEventKind::ManagerTriggeredEnter
            | RawApplyEventKind::ManagerTriggeredExit
    );
    let is_scalar_query = matches!(
        kind,
        RawApplyEventKind::LogicalScalarEnter
            | RawApplyEventKind::LogicalScalarExit
            | RawApplyEventKind::ManagerScalarEnter
            | RawApplyEventKind::ManagerScalarExit
    );
    if is_bool_query || is_scalar_query {
        if is_enter(kind) && (value_valid || result_true || event.value_bits != 0) {
            return false;
        }
        if is_exit(kind) && !value_valid {
            return false;
        }
        if is_exit(kind)
            && is_bool_query
            && (event.value_bits > 1 || result_true != (event.value_bits == 1))
        {
            return false;
        }
        if is_exit(kind) && is_scalar_query {
            let value = f32::from_bits(event.value_bits);
            if result_true != (value != 0.0) {
                return false;
            }
        }
    }

    if matches!(
        kind,
        RawApplyEventKind::LogicalSetterEnter | RawApplyEventKind::LogicalSetterExit
    ) && (!value_valid || result_true)
    {
        return false;
    }

    match kind {
        RawApplyEventKind::LogicalPressedEnter
        | RawApplyEventKind::LogicalPressedExit
        | RawApplyEventKind::LogicalTriggeredEnter
        | RawApplyEventKind::LogicalTriggeredExit
        | RawApplyEventKind::LogicalScalarEnter
        | RawApplyEventKind::LogicalScalarExit => {
            event.action <= 11
                && classification == RawApplyClassification::ExplicitGameplay
                && raw_kind == RawChannelKind::None
                && witness_kind == PassiveWitnessKind::None
        }
        RawApplyEventKind::ManagerPressedEnter
        | RawApplyEventKind::ManagerPressedExit
        | RawApplyEventKind::ManagerTriggeredEnter
        | RawApplyEventKind::ManagerTriggeredExit
        | RawApplyEventKind::ManagerScalarEnter
        | RawApplyEventKind::ManagerScalarExit => {
            event.action <= 11
                && classification == RawApplyClassification::UniqueGameplay
                && raw_kind == RawChannelKind::None
                && witness_kind == PassiveWitnessKind::None
        }
        RawApplyEventKind::LogicalSetterEnter | RawApplyEventKind::LogicalSetterExit => {
            event.action <= 11
                && classification == RawApplyClassification::LogicalOverrideDiagnostic
                && raw_kind == RawChannelKind::None
                && witness_kind == PassiveWitnessKind::None
        }
        RawApplyEventKind::RawPollEnter
        | RawApplyEventKind::RawValue
        | RawApplyEventKind::RawPollExit => {
            let physical_boundary = event.phase == LoopPhase::InputQuery as u32
                && event.return_rva == RAW_APPLY_PHYSICAL_POLL_RETURN_RVA
                && match event.hook_site_rva {
                    RAW_APPLY_KEYBOARD_POLL_HOOK_SITE_RVA => {
                        device_flags == RAW_APPLY_FLAG_KEYBOARD_DEVICE
                    }
                    RAW_APPLY_GAMEPAD_POLL_HOOK_SITE_RVA => {
                        device_flags == RAW_APPLY_FLAG_GAMEPAD_DEVICE
                    }
                    _ => false,
                };
            let value_shape = match kind {
                RawApplyEventKind::RawValue => {
                    event.original_calls == 1
                        && original
                        && value_valid
                        && event.action <= 11
                        && event.raw_channel_id == event.action
                        && classification == RawApplyClassification::ExplicitGameplay
                }
                RawApplyEventKind::RawPollEnter => {
                    !value_valid
                        && !result_true
                        && event.value_bits == 0
                        && event.flags & (RAW_APPLY_FLAG_RAW_RISING | RAW_APPLY_FLAG_RAW_FALLING)
                            == 0
                }
                RawApplyEventKind::RawPollExit => {
                    value_valid
                        && event.value_bits <= 1
                        && result_true == (event.value_bits == 1)
                        && event.flags & (RAW_APPLY_FLAG_RAW_RISING | RAW_APPLY_FLAG_RAW_FALLING)
                            == 0
                }
                _ => false,
            };
            physical_boundary
                && event.sample_generation != 0
                && event.consumer_transaction == 0
                && event.player_ordinal == RAW_APPLY_NO_PLAYER_ORDINAL
                && (kind == RawApplyEventKind::RawValue || event.action == RAW_APPLY_NO_ACTION)
                && raw_kind != RawChannelKind::None
                && witness_kind == PassiveWitnessKind::None
                && (kind == RawApplyEventKind::RawValue
                    || classification == RawApplyClassification::RawDeviceOnly)
                && value_shape
        }
        RawApplyEventKind::PlayerConsumerEnter | RawApplyEventKind::PlayerConsumerExit => {
            event.consumer_transaction != 0
                && event.context_generation != 0
                && event.player_ordinal != RAW_APPLY_NO_PLAYER_ORDINAL
                && event.action == RAW_APPLY_NO_ACTION
                && RAW_APPLY_CONSUMER_HOOK_SITE_RVAS.contains(&event.hook_site_rva)
                && !value_valid
                && !result_true
                && event.value_bits == 0
                && raw_kind == RawChannelKind::None
                && witness_kind == PassiveWitnessKind::None
                && classification == RawApplyClassification::PlayerConsumerObserved
        }
        RawApplyEventKind::WorldWitnessBefore | RawApplyEventKind::WorldWitnessAfter => {
            let witness_original_shape = match kind {
                RawApplyEventKind::WorldWitnessBefore => event.original_calls == 0 && !original,
                RawApplyEventKind::WorldWitnessAfter => event.original_calls == 1 && original,
                _ => false,
            };
            event.consumer_transaction != 0
                && event.context_generation != 0
                && event.player_ordinal != RAW_APPLY_NO_PLAYER_ORDINAL
                && (event.action == RAW_APPLY_NO_ACTION || event.action <= 11)
                && witness_allowed(event.hook_site_rva, witness_kind)
                && value_valid
                && !result_true
                && raw_kind == RawChannelKind::None
                && witness_kind != PassiveWitnessKind::None
                && classification == RawApplyClassification::PassiveWorldWitness
                && witness_original_shape
        }
        RawApplyEventKind::ContextOwner => {
            event.consumer_transaction == 0
                && event.context_generation != 0
                && event.player_ordinal != RAW_APPLY_NO_PLAYER_ORDINAL
                && event.action == RAW_APPLY_NO_ACTION
                && event.hook_site_rva == RAW_APPLY_OWNER_HOOK_SITE_RVA
                && matches!(event.original_calls, 0 | 1)
                && original == (event.original_calls == 1)
                && !value_valid
                && raw_kind == RawChannelKind::None
                && witness_kind == PassiveWitnessKind::None
                && classification == RawApplyClassification::PlayerConsumerObserved
        }
        RawApplyEventKind::ContextBinding => {
            event.consumer_transaction == 0
                && event.context_generation != 0
                && event.player_ordinal != RAW_APPLY_NO_PLAYER_ORDINAL
                && event.action <= 11
                && event.original_calls == 0
                && !original
                && !value_valid
                && raw_kind == RawChannelKind::None
                && witness_kind == PassiveWitnessKind::None
                && classification == RawApplyClassification::ExplicitGameplay
        }
        _ => false,
    }
}

fn witness_allowed(hook_site_rva: u32, witness_kind: PassiveWitnessKind) -> bool {
    match hook_site_rva {
        0x0038_5BC2 => matches!(
            witness_kind,
            PassiveWitnessKind::MovementDirection | PassiveWitnessKind::PlayerVelocityF32x2
        ),
        0x0038_5BC9 => witness_kind == PassiveWitnessKind::BombIntentBit,
        0x0038_5BE0 => matches!(
            witness_kind,
            PassiveWitnessKind::ShootingDirection
                | PassiveWitnessKind::AimVector
                | PassiveWitnessKind::AimState
        ),
        0x0038_5BE7 => witness_kind == PassiveWitnessKind::DropHoldCounter,
        _ => false,
    }
}

pub fn raw_apply_event_from_native(
    record: &NativeRawApplyEventRecord,
    event_sequence: u64,
) -> AppResult<RawApplyArtifactEvent> {
    if record.sequence == 0 || record.event.published_sequence != record.sequence {
        return Err(AppError::evidence(
            "raw/apply native sequence is zero or does not match its published sequence",
        ));
    }
    let kind = RawApplyEventKind::from_raw(record.event.kind)?;
    let phase = LoopPhase::from_raw(record.event.phase)?;
    let classification = RawApplyClassification::from_raw(record.event.classification)?;
    let raw_kind = RawChannelKind::from_raw(record.event.raw_channel_kind)?;
    let witness_kind = PassiveWitnessKind::from_raw(record.event.witness_kind)?;
    if kind.is_lifecycle() {
        if !lifecycle_payload_empty(record, kind) {
            return Err(AppError::evidence(
                "raw/apply lifecycle event contains payload or invalid error state",
            ));
        }
    } else if !runtime_payload_valid(record, kind, classification, raw_kind, witness_kind) {
        return Err(AppError::evidence(format!(
            "raw/apply native {:?} event violates passive field scope",
            kind
        )));
    }

    let is_raw = matches!(
        kind,
        RawApplyEventKind::RawPollEnter
            | RawApplyEventKind::RawValue
            | RawApplyEventKind::RawPollExit
    );
    let is_witness = matches!(
        kind,
        RawApplyEventKind::WorldWitnessBefore | RawApplyEventKind::WorldWitnessAfter
    );
    let has_regular_value = !kind.is_lifecycle()
        && !is_raw
        && !is_witness
        && record.event.flags & RAW_APPLY_FLAG_VALUE_VALID != 0;

    Ok(RawApplyArtifactEvent {
        event_sequence,
        stage: stage_for(kind).to_owned(),
        phase,
        native_kind: kind,
        native_sequence: record.sequence,
        qpc: record.event.qpc,
        loop_iteration: record.event.loop_iteration,
        thread_id: record.event.thread_id,
        flags: record.event.flags,
        classification,
        error_code: (record.event.error_code != 0).then_some(record.event.error_code),
        passive_hook_site_rva: rva(record.event.hook_site_rva),
        passive_return_rva: rva(record.event.return_rva),
        passive_original_calls: (!kind.is_lifecycle()).then_some(record.event.original_calls),
        passive_device_key: optional_sentinel(record.event.device_key, RAW_APPLY_NO_DEVICE_KEY),
        passive_controller_id_raw: (!kind.is_lifecycle()).then_some(record.event.controller_id_raw),
        passive_selected_device_key: optional_sentinel(
            record.event.selected_device_key,
            RAW_APPLY_NO_DEVICE_KEY,
        ),
        passive_player_ordinal: optional_sentinel(
            record.event.player_ordinal,
            RAW_APPLY_NO_PLAYER_ORDINAL,
        ),
        passive_action: optional_sentinel(record.event.action, RAW_APPLY_NO_ACTION),
        passive_primitive: optional_sentinel(record.event.primitive, RAW_APPLY_NO_PRIMITIVE),
        passive_value_bits: has_regular_value.then_some(record.event.value_bits),
        passive_sample_generation: optional_generation(record.event.sample_generation),
        passive_consumer_transaction: optional_generation(record.event.consumer_transaction),
        passive_context_generation: optional_generation(record.event.context_generation),
        passive_raw_channel_kind: is_raw.then_some(raw_kind),
        passive_raw_channel_id: is_raw.then_some(record.event.raw_channel_id),
        passive_raw_value_bits: matches!(
            kind,
            RawApplyEventKind::RawValue | RawApplyEventKind::RawPollExit
        )
        .then_some(record.event.value_bits),
        passive_witness_kind: is_witness.then_some(witness_kind),
        passive_witness_value_bits: is_witness.then_some(record.event.value_bits),
        passive_witness_aux_bits: is_witness.then_some(record.event.witness_aux_bits),
        session_id: None,
        client_id: None,
        command_sequence: None,
        target_tick: None,
        canonical_state_hex: None,
        identity_key: None,
        queue_sequence: None,
        resolution: None,
        applied_state_hex: None,
        world_witness_before: None,
        world_witness_after: None,
    })
}

pub fn raw_apply_event_scope_valid(event: &RawApplyArtifactEvent) -> bool {
    let forbidden_pipeline_empty = event.session_id.is_none()
        && event.client_id.is_none()
        && event.command_sequence.is_none()
        && event.target_tick.is_none()
        && event.canonical_state_hex.is_none()
        && event.identity_key.is_none()
        && event.queue_sequence.is_none()
        && event.resolution.is_none()
        && event.applied_state_hex.is_none()
        && event.world_witness_before.is_none()
        && event.world_witness_after.is_none();
    if !forbidden_pipeline_empty
        || event.passive_action.is_some_and(|action| action > 11)
        || event.passive_primitive.is_some()
    {
        return false;
    }
    let original = event.flags & RAW_APPLY_FLAG_ORIGINAL_CALLED != 0;
    let value_valid = event.flags & RAW_APPLY_FLAG_VALUE_VALID != 0;
    let result_true = event.flags & RAW_APPLY_FLAG_RESULT_TRUE != 0;
    let family_scope = match event.native_kind {
        RawApplyEventKind::RawPollEnter => {
            event.passive_action.is_none()
                && event.classification == RawApplyClassification::RawDeviceOnly
                && event.passive_original_calls == Some(0)
                && !original
                && !value_valid
                && event.passive_raw_value_bits.is_none()
        }
        RawApplyEventKind::RawPollExit => {
            event.passive_action.is_none()
                && event.classification == RawApplyClassification::RawDeviceOnly
                && event.passive_original_calls == Some(1)
                && original
                && value_valid
                && event
                    .passive_raw_value_bits
                    .is_some_and(|value| value <= 1 && result_true == (value == 1))
        }
        RawApplyEventKind::RawValue => {
            event.passive_action.is_some()
                && event.classification == RawApplyClassification::ExplicitGameplay
                && event.passive_original_calls == Some(1)
                && original
                && value_valid
                && event.passive_raw_value_bits.is_some()
        }
        RawApplyEventKind::PlayerConsumerEnter | RawApplyEventKind::PlayerConsumerExit => {
            event.passive_action.is_none()
                && event
                    .passive_hook_site_rva
                    .as_deref()
                    .and_then(parse_rva)
                    .is_some_and(|site| RAW_APPLY_CONSUMER_HOOK_SITE_RVAS.contains(&site))
        }
        RawApplyEventKind::WorldWitnessBefore | RawApplyEventKind::WorldWitnessAfter => event
            .passive_hook_site_rva
            .as_deref()
            .and_then(parse_rva)
            .zip(event.passive_witness_kind)
            .is_some_and(|(site, kind)| witness_allowed(site, kind)),
        RawApplyEventKind::ContextOwner => {
            event.passive_action.is_none()
                && event.passive_hook_site_rva.as_deref().and_then(parse_rva)
                    == Some(RAW_APPLY_OWNER_HOOK_SITE_RVA)
        }
        _ => true,
    };
    if !family_scope {
        return false;
    }
    let is_raw = matches!(
        event.native_kind,
        RawApplyEventKind::RawPollEnter
            | RawApplyEventKind::RawValue
            | RawApplyEventKind::RawPollExit
    );
    let is_witness = matches!(
        event.native_kind,
        RawApplyEventKind::WorldWitnessBefore | RawApplyEventKind::WorldWitnessAfter
    );
    (!is_raw
        && event.passive_raw_channel_kind.is_none()
        && event.passive_raw_channel_id.is_none()
        && event.passive_raw_value_bits.is_none()
        || is_raw
            && event.passive_raw_channel_kind.is_some()
            && event.passive_raw_channel_id.is_some()
            && (!matches!(
                event.native_kind,
                RawApplyEventKind::RawValue | RawApplyEventKind::RawPollExit
            ) || event.passive_raw_value_bits.is_some()))
        && (!is_witness
            && event.passive_witness_kind.is_none()
            && event.passive_witness_value_bits.is_none()
            && event.passive_witness_aux_bits.is_none()
            || is_witness
                && event.passive_witness_kind.is_some()
                && event.passive_witness_value_bits.is_some()
                && event.passive_witness_aux_bits.is_some())
        && (!event.native_kind.is_lifecycle()
            || event.passive_hook_site_rva.is_none()
                && event.passive_return_rva.is_none()
                && event.passive_original_calls.is_none()
                && event.passive_device_key.is_none()
                && event.passive_controller_id_raw.is_none()
                && event.passive_selected_device_key.is_none()
                && event.passive_player_ordinal.is_none()
                && event.passive_action.is_none()
                && event.passive_primitive.is_none()
                && event.passive_value_bits.is_none()
                && event.passive_sample_generation.is_none()
                && event.passive_consumer_transaction.is_none()
                && event.passive_context_generation.is_none())
}

fn parse_rva(value: &str) -> Option<u32> {
    value
        .strip_prefix("0x")
        .and_then(|digits| u32::from_str_radix(digits, 16).ok())
}

fn immutable_pair_fields_equal(
    enter: &RawApplyArtifactEvent,
    exit: &RawApplyArtifactEvent,
) -> bool {
    enter.qpc <= exit.qpc
        && enter.loop_iteration == exit.loop_iteration
        && enter.phase == exit.phase
        && enter.thread_id == exit.thread_id
        && enter.passive_hook_site_rva == exit.passive_hook_site_rva
        && enter.passive_return_rva == exit.passive_return_rva
        && enter.passive_device_key == exit.passive_device_key
        && enter.passive_controller_id_raw == exit.passive_controller_id_raw
        && enter.passive_selected_device_key == exit.passive_selected_device_key
        && enter.passive_player_ordinal == exit.passive_player_ordinal
        && enter.passive_action == exit.passive_action
        && enter.passive_primitive == exit.passive_primitive
        && enter.passive_sample_generation == exit.passive_sample_generation
        && enter.passive_consumer_transaction == exit.passive_consumer_transaction
        && enter.passive_context_generation == exit.passive_context_generation
        && enter.passive_raw_channel_kind == exit.passive_raw_channel_kind
        && enter.passive_raw_channel_id == exit.passive_raw_channel_id
        && enter.passive_witness_kind == exit.passive_witness_kind
        && enter.classification == exit.classification
        && enter.flags & (RAW_APPLY_FLAG_KEYBOARD_DEVICE | RAW_APPLY_FLAG_GAMEPAD_DEVICE)
            == exit.flags & (RAW_APPLY_FLAG_KEYBOARD_DEVICE | RAW_APPLY_FLAG_GAMEPAD_DEVICE)
        && enter.passive_original_calls == Some(0)
        && exit.passive_original_calls == Some(1)
        && enter.flags & RAW_APPLY_FLAG_ORIGINAL_CALLED == 0
        && exit.flags & RAW_APPLY_FLAG_ORIGINAL_CALLED != 0
}

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
struct CompletedPollKey {
    thread_id: u32,
    loop_iteration: u64,
    sample_generation: Option<u64>,
    device_key: Option<u32>,
    controller_id_raw: Option<u32>,
    selected_device_key: Option<u32>,
}

impl CompletedPollKey {
    fn from_event(event: &RawApplyArtifactEvent) -> Self {
        Self {
            thread_id: event.thread_id,
            loop_iteration: event.loop_iteration,
            sample_generation: event.passive_sample_generation,
            device_key: event.passive_device_key,
            controller_id_raw: event.passive_controller_id_raw,
            selected_device_key: event.passive_selected_device_key,
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
struct WitnessPairKey<'a> {
    thread_id: u32,
    loop_iteration: u64,
    device_key: Option<u32>,
    controller_id_raw: Option<u32>,
    selected_device_key: Option<u32>,
    player_ordinal: Option<u32>,
    sample_generation: Option<u64>,
    consumer_transaction: Option<u64>,
    context_generation: Option<u64>,
    action: Option<u32>,
    hook_site_rva: Option<&'a str>,
    witness_kind: Option<u32>,
    phase: u32,
}

impl<'a> WitnessPairKey<'a> {
    fn from_event(event: &'a RawApplyArtifactEvent) -> Self {
        Self {
            thread_id: event.thread_id,
            loop_iteration: event.loop_iteration,
            device_key: event.passive_device_key,
            controller_id_raw: event.passive_controller_id_raw,
            selected_device_key: event.passive_selected_device_key,
            player_ordinal: event.passive_player_ordinal,
            sample_generation: event.passive_sample_generation,
            consumer_transaction: event.passive_consumer_transaction,
            context_generation: event.passive_context_generation,
            action: event.passive_action,
            hook_site_rva: event.passive_hook_site_rva.as_deref(),
            witness_kind: event.passive_witness_kind.map(|kind| kind as u32),
            phase: event.phase as u32,
        }
    }
}

#[derive(Default)]
struct WitnessPairQpcs {
    before: Vec<u64>,
    after: Vec<u64>,
}

pub fn raw_apply_pairs_valid(events: &[RawApplyArtifactEvent]) -> bool {
    let mut stacks: BTreeMap<u32, Vec<&RawApplyArtifactEvent>> = BTreeMap::new();
    let mut completed_poll_qpcs: HashMap<CompletedPollKey, Vec<u64>> = HashMap::new();
    let mut witness_pair_qpcs: HashMap<WitnessPairKey<'_>, WitnessPairQpcs> = HashMap::new();
    let mut raw_value_identities = HashSet::new();
    for event in events {
        match event.native_kind {
            RawApplyEventKind::RawPollExit => completed_poll_qpcs
                .entry(CompletedPollKey::from_event(event))
                .or_default()
                .push(event.qpc),
            RawApplyEventKind::WorldWitnessBefore => witness_pair_qpcs
                .entry(WitnessPairKey::from_event(event))
                .or_default()
                .before
                .push(event.qpc),
            RawApplyEventKind::WorldWitnessAfter => witness_pair_qpcs
                .entry(WitnessPairKey::from_event(event))
                .or_default()
                .after
                .push(event.qpc),
            _ => {}
        }
        if event.native_kind == RawApplyEventKind::ContextOwner {
            match event.passive_original_calls {
                Some(0) if event.flags & RAW_APPLY_FLAG_ORIGINAL_CALLED == 0 => {
                    stacks.entry(event.thread_id).or_default().push(event);
                }
                Some(1) if event.flags & RAW_APPLY_FLAG_ORIGINAL_CALLED != 0 => {
                    let Some(stack) = stacks.get_mut(&event.thread_id) else {
                        return false;
                    };
                    let Some(enter) = stack.pop() else {
                        return false;
                    };
                    if enter.native_kind != RawApplyEventKind::ContextOwner
                        || !immutable_pair_fields_equal(enter, event)
                    {
                        return false;
                    }
                }
                _ => return false,
            }
            continue;
        }
        if is_enter(event.native_kind) {
            stacks.entry(event.thread_id).or_default().push(event);
            continue;
        }
        if matches!(
            event.native_kind,
            RawApplyEventKind::RawValue
                | RawApplyEventKind::WorldWitnessBefore
                | RawApplyEventKind::WorldWitnessAfter
                | RawApplyEventKind::ContextBinding
        ) {
            continue;
        }
        if is_exit(event.native_kind) {
            let Some(stack) = stacks.get_mut(&event.thread_id) else {
                return false;
            };
            let Some(enter) = stack.pop() else {
                return false;
            };
            if matching_exit(enter.native_kind) != Some(event.native_kind)
                || !immutable_pair_fields_equal(enter, event)
                || enter.native_kind == RawApplyEventKind::LogicalSetterEnter
                    && enter.passive_value_bits != event.passive_value_bits
            {
                return false;
            }
        }
    }
    if !stacks.values().all(Vec::is_empty) {
        return false;
    }
    for qpcs in completed_poll_qpcs.values_mut() {
        qpcs.sort_unstable();
    }
    for qpcs in witness_pair_qpcs.values_mut() {
        qpcs.before.sort_unstable();
        qpcs.after.sort_unstable();
    }
    for raw in events
        .iter()
        .filter(|event| event.native_kind == RawApplyEventKind::RawValue)
    {
        let raw_identity = (
            CompletedPollKey::from_event(raw),
            raw.passive_action,
            raw.passive_hook_site_rva.as_deref(),
        );
        if !raw_value_identities.insert(raw_identity) {
            return false;
        }
        let completed_polls = completed_poll_qpcs
            .get(&CompletedPollKey::from_event(raw))
            .map_or(0, |qpcs| qpcs.partition_point(|qpc| *qpc <= raw.qpc));
        if completed_polls != 1 {
            return false;
        }
    }
    for witness in events.iter().filter(|event| {
        matches!(
            event.native_kind,
            RawApplyEventKind::WorldWitnessBefore | RawApplyEventKind::WorldWitnessAfter
        )
    }) {
        let Some(pair) = witness_pair_qpcs.get(&WitnessPairKey::from_event(witness)) else {
            return false;
        };
        let matches = if witness.native_kind == RawApplyEventKind::WorldWitnessBefore {
            pair.after.len() - pair.after.partition_point(|qpc| *qpc < witness.qpc)
        } else {
            pair.before.partition_point(|qpc| *qpc <= witness.qpc)
        };
        if matches != 1 {
            return false;
        }
    }
    true
}

pub fn raw_apply_lifecycle_valid(events: &[RawApplyArtifactEvent]) -> bool {
    let mut lifecycle_state = 0_u8;
    let mut last_event_sequence = 0;
    let mut last_native_sequence = 0;
    for event in events {
        if event.event_sequence <= last_event_sequence
            || event.native_sequence <= last_native_sequence
        {
            return false;
        }
        last_event_sequence = event.event_sequence;
        last_native_sequence = event.native_sequence;
        match event.native_kind {
            RawApplyEventKind::NativeInitialized if lifecycle_state == 0 => lifecycle_state = 1,
            RawApplyEventKind::HooksInstalled if lifecycle_state == 1 => lifecycle_state = 2,
            RawApplyEventKind::HooksRestored if lifecycle_state == 2 => lifecycle_state = 3,
            RawApplyEventKind::NativeRetired if lifecycle_state == 3 => lifecycle_state = 4,
            RawApplyEventKind::Failure => return false,
            kind if kind.is_lifecycle() => return false,
            _ if lifecycle_state != 2 => return false,
            _ => {}
        }
    }
    lifecycle_state == 4
}

fn parse_hex_mask(value: &str) -> Option<u64> {
    value
        .strip_prefix("0x")
        .or_else(|| value.strip_prefix("0X"))
        .and_then(|digits| u64::from_str_radix(digits, 16).ok())
}

fn expected_actions(scenario: &str) -> Option<BTreeSet<u32>> {
    Some(match scenario {
        "keyboard-held-release" | "controller-held-release" => (0..=7).collect(),
        "keyboard-edge" | "controller-edge" => (8..=11).collect(),
        _ => return None,
    })
}

fn expected_consumer_site(action: u32) -> Option<u32> {
    Some(match action {
        0..=3 => RAW_APPLY_CONSUMER_HOOK_SITE_RVAS[0],
        8 => RAW_APPLY_CONSUMER_HOOK_SITE_RVAS[1],
        9 => RAW_APPLY_CONSUMER_HOOK_SITE_RVAS[2],
        10 => RAW_APPLY_CONSUMER_HOOK_SITE_RVAS[3],
        4..=7 => RAW_APPLY_CONSUMER_HOOK_SITE_RVAS[4],
        11 => RAW_APPLY_CONSUMER_HOOK_SITE_RVAS[5],
        _ => return None,
    })
}

fn is_logical_exit(kind: RawApplyEventKind) -> bool {
    matches!(
        kind,
        RawApplyEventKind::LogicalPressedExit
            | RawApplyEventKind::LogicalTriggeredExit
            | RawApplyEventKind::LogicalScalarExit
    )
}

fn logical_result_true(event: &RawApplyArtifactEvent) -> bool {
    is_logical_exit(event.native_kind)
        && event.flags & RAW_APPLY_FLAG_VALUE_VALID != 0
        && event.flags & RAW_APPLY_FLAG_RESULT_TRUE != 0
}

fn same_runtime_identity(first: &RawApplyArtifactEvent, second: &RawApplyArtifactEvent) -> bool {
    first.thread_id == second.thread_id
        && first.loop_iteration == second.loop_iteration
        && first.passive_device_key == second.passive_device_key
        && first.passive_controller_id_raw == second.passive_controller_id_raw
        && first.passive_selected_device_key == second.passive_selected_device_key
        && first.passive_player_ordinal == second.passive_player_ordinal
        && first.passive_context_generation == second.passive_context_generation
}

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
struct RawBindingKey {
    action: u32,
    sample_generation: Option<u64>,
    thread_id: u32,
    loop_iteration: u64,
    device_key: Option<u32>,
    controller_id_raw: Option<u32>,
    selected_device_key: Option<u32>,
}

impl RawBindingKey {
    fn from_raw(event: &RawApplyArtifactEvent) -> Option<Self> {
        Some(Self {
            action: event.passive_action?,
            sample_generation: event.passive_sample_generation,
            thread_id: event.thread_id,
            loop_iteration: event.loop_iteration,
            device_key: event.passive_device_key,
            controller_id_raw: event.passive_controller_id_raw,
            selected_device_key: event.passive_selected_device_key,
        })
    }

    fn from_logical(event: &RawApplyArtifactEvent, action: u32) -> Self {
        Self {
            action,
            sample_generation: event.passive_sample_generation,
            thread_id: event.thread_id,
            loop_iteration: event.loop_iteration,
            device_key: event.passive_device_key,
            controller_id_raw: event.passive_controller_id_raw,
            selected_device_key: event.passive_selected_device_key,
        }
    }
}

fn finite_raw_projection_value(event: &RawApplyArtifactEvent) -> Option<f32> {
    let value = f32::from_bits(event.passive_raw_value_bits?);
    value.is_finite().then_some(value)
}

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
struct RawOwnerActionKey {
    action: u32,
    thread_id: u32,
    device_key: Option<u32>,
    controller_id_raw: Option<u32>,
    selected_device_key: Option<u32>,
}

impl RawOwnerActionKey {
    fn from_event(event: &RawApplyArtifactEvent, action: u32) -> Self {
        Self {
            action,
            thread_id: event.thread_id,
            device_key: event.passive_device_key,
            controller_id_raw: event.passive_controller_id_raw,
            selected_device_key: event.passive_selected_device_key,
        }
    }
}

#[derive(Default)]
struct RawTransitionState {
    last: Option<(u64, bool)>,
    saw_inactive_to_active: bool,
    saw_active_to_inactive: bool,
    invalid: bool,
}

fn held_release_transition_keys(events: &[RawApplyArtifactEvent]) -> HashSet<RawOwnerActionKey> {
    let mut states: HashMap<RawOwnerActionKey, RawTransitionState> = HashMap::new();
    for event in events
        .iter()
        .filter(|event| event.native_kind == RawApplyEventKind::RawValue)
    {
        let (Some(action), Some(generation), Some(value)) = (
            event.passive_action,
            event.passive_sample_generation,
            finite_raw_projection_value(event),
        ) else {
            continue;
        };
        let active = value != 0.0;
        let rising = event.flags & RAW_APPLY_FLAG_RAW_RISING != 0;
        let falling = event.flags & RAW_APPLY_FLAG_RAW_FALLING != 0;
        let state = states
            .entry(RawOwnerActionKey::from_event(event, action))
            .or_default();
        if let Some((last_generation, last_active)) = state.last {
            if generation <= last_generation {
                state.invalid = true;
            } else {
                let semantic_rising = !last_active && active;
                let semantic_falling = last_active && !active;
                if rising && !semantic_rising || falling && !semantic_falling {
                    state.invalid = true;
                }
                if semantic_rising {
                    state.saw_inactive_to_active = true;
                } else if semantic_falling && state.saw_inactive_to_active {
                    state.saw_active_to_inactive = true;
                }
            }
        } else if rising || falling {
            // Native history starts empty, so the first persisted value can never carry an edge.
            state.invalid = true;
        }
        state.last = Some((generation, active));
    }
    states
        .into_iter()
        .filter_map(|(key, state)| {
            (!state.invalid && state.saw_inactive_to_active && state.saw_active_to_inactive)
                .then_some(key)
        })
        .collect()
}

pub fn raw_apply_offline_closure_valid(scenario: &str, events: &[RawApplyArtifactEvent]) -> bool {
    let Some(required_actions) = expected_actions(scenario) else {
        return false;
    };
    let mut active_raw_bindings: HashMap<RawBindingKey, u64> = HashMap::new();
    for event in events
        .iter()
        .filter(|event| event.native_kind == RawApplyEventKind::RawValue)
    {
        if finite_raw_projection_value(event).is_some_and(|value| value != 0.0)
            && let Some(key) = RawBindingKey::from_raw(event)
        {
            active_raw_bindings
                .entry(key)
                .and_modify(|sequence| *sequence = (*sequence).min(event.native_sequence))
                .or_insert(event.native_sequence);
        }
    }
    let held_release = matches!(
        scenario,
        "keyboard-held-release" | "controller-held-release"
    );
    let completed_transitions = if held_release {
        held_release_transition_keys(events)
    } else {
        HashSet::new()
    };
    let mut consumer_exits: HashMap<(u32, u64), Vec<usize>> = HashMap::new();
    for (index, event) in events.iter().enumerate() {
        if event.native_kind == RawApplyEventKind::PlayerConsumerExit
            && let Some(transaction) = event.passive_consumer_transaction
        {
            consumer_exits
                .entry((event.thread_id, transaction))
                .or_default()
                .push(index);
        }
    }
    let mut proven_actions = BTreeSet::new();
    for (enter_index, consumer) in events
        .iter()
        .enumerate()
        .filter(|(_, event)| event.native_kind == RawApplyEventKind::PlayerConsumerEnter)
    {
        let Some(transaction) = consumer.passive_consumer_transaction else {
            return false;
        };
        let Some(exits) = consumer_exits.get(&(consumer.thread_id, transaction)) else {
            return false;
        };
        let exit_offset = exits.partition_point(|index| *index <= enter_index);
        let Some(exit_index) = exits.get(exit_offset).copied() else {
            return false;
        };
        let nested = &events[enter_index + 1..exit_index];
        let logical_exits = nested.iter().filter(|event| {
            is_logical_exit(event.native_kind)
                && event.passive_consumer_transaction == Some(transaction)
                && event.passive_sample_generation == consumer.passive_sample_generation
                && event.passive_sample_generation.is_some()
                && same_runtime_identity(consumer, event)
        });
        let mut transaction_actions = BTreeSet::new();
        for logical in logical_exits {
            let Some(action) = logical.passive_action else {
                return false;
            };
            let raw_bound = active_raw_bindings
                .get(&RawBindingKey::from_logical(logical, action))
                .is_some_and(|raw_sequence| *raw_sequence < logical.native_sequence);
            if !raw_bound {
                continue;
            }
            if logical_result_true(logical)
                && (!held_release
                    || completed_transitions
                        .contains(&RawOwnerActionKey::from_event(logical, action)))
            {
                transaction_actions.insert(action);
            }
        }
        let consumer_site_matches = |action| {
            consumer.passive_hook_site_rva
                == expected_consumer_site(action).map(|rva| format!("0x{rva:08X}"))
        };
        if transaction_actions.len() > 1 {
            // A natural movement/shoot consumer may observe a diagonal and therefore
            // contain multiple proof-eligible actions. Its sentinel witness cannot be
            // attributed to either action, so the complete transaction contributes no
            // proof. A cross-family or wrong-site mixture is a structural contradiction,
            // not benign ambiguity, and remains a run-level failure.
            if !transaction_actions
                .iter()
                .copied()
                .all(consumer_site_matches)
            {
                return false;
            }
            continue;
        }
        let Some(action) = transaction_actions.first().copied() else {
            continue;
        };
        if !consumer_site_matches(action) {
            return false;
        }
        if !matches!(action, 9 | 10) {
            let witness_before = nested.iter().find(|event| {
                event.native_kind == RawApplyEventKind::WorldWitnessBefore
                    && event.passive_consumer_transaction == Some(transaction)
                    && event.passive_sample_generation == consumer.passive_sample_generation
                    && event
                        .passive_action
                        .is_none_or(|witness_action| witness_action == action)
                    && same_runtime_identity(consumer, event)
            });
            let Some(before) = witness_before else {
                continue;
            };
            let witness_after = nested.iter().find(|event| {
                event.native_kind == RawApplyEventKind::WorldWitnessAfter
                    && event.passive_consumer_transaction == Some(transaction)
                    && event.passive_sample_generation == consumer.passive_sample_generation
                    && event.passive_hook_site_rva == before.passive_hook_site_rva
                    && event.passive_witness_kind == before.passive_witness_kind
                    && event
                        .passive_action
                        .is_none_or(|witness_action| witness_action == action)
                    && same_runtime_identity(consumer, event)
            });
            let Some(after) = witness_after else {
                continue;
            };
            if before.passive_witness_value_bits == after.passive_witness_value_bits
                && before.passive_witness_aux_bits == after.passive_witness_aux_bits
            {
                continue;
            }
        }
        proven_actions.insert(action);
    }
    required_actions.is_subset(&proven_actions)
}

fn companion_loop_hashes(
    run_directory: &Path,
    manifest: &RawApplyManifest,
) -> AppResult<(String, String, String)> {
    validate_relative_path(&manifest.companion_loop_artifact_path)?;
    let companion = run_directory.join(&manifest.companion_loop_artifact_path);
    let manifest_path = companion.join("manifest.json");
    let events_path = companion.join("events.jsonl");
    let analysis_path = companion.join("analysis.json");
    let loop_manifest: crate::loop_verification::LoopManifest = serde_json::from_slice(
        &fs::read(&manifest_path)
            .usage_context(format!("cannot read {}", manifest_path.display()))?,
    )
    .usage_context("cannot parse companion loop manifest")?;
    let loop_analysis: crate::loop_verification::LoopAnalysisReport = serde_json::from_slice(
        &fs::read(&analysis_path)
            .usage_context(format!("cannot read {}", analysis_path.display()))?,
    )
    .usage_context("cannot parse companion loop analysis")?;
    let manifest_sha256 = file_sha256(&manifest_path)?;
    let events_sha256 = file_sha256(&events_path)?;
    let analysis_sha256 = file_sha256(&analysis_path)?;
    if loop_manifest.state != "COMPLETE"
        || loop_manifest.active_input_allowed
        || loop_manifest.target_sha256 != manifest.target_sha256
        || loop_manifest.main_thread_id != manifest.passive_main_thread_id
        || loop_manifest.hooks_mask != crate::loop_ipc::LOOP_HOOK_MASK
        || loop_manifest.overwritten_events != 0
        || !loop_manifest.target_bytes_restored
        || loop_analysis.target_sha256 != manifest.target_sha256
        || loop_analysis.manifest_sha256 != manifest_sha256
        || loop_analysis.events_sha256 != events_sha256
    {
        return Err(AppError::evidence(
            "raw/apply companion loop artifact is not structurally restored and hash-bound",
        ));
    }
    Ok((manifest_sha256, events_sha256, analysis_sha256))
}

pub fn raw_apply_manifest_valid(manifest: &RawApplyManifest) -> bool {
    raw_apply_manifest_valid_for_revision(
        manifest,
        RAW_APPLY_CATALOG_SHA256,
        RAW_APPLY_CAPTURE_PLAN_SHA256,
    )
}

fn raw_apply_manifest_valid_for_revision(
    manifest: &RawApplyManifest,
    expected_catalog_sha256: &str,
    expected_capture_plan_sha256: &str,
) -> bool {
    let required_raw = parse_hex_mask(&manifest.passive_required_raw_source_mask);
    let observed_raw = parse_hex_mask(&manifest.passive_observed_raw_source_mask);
    manifest.schema_version == RAW_APPLY_ARTIFACT_SCHEMA_VERSION
        && manifest.evidence_class == EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME
        && manifest.ipc_source_format == RAW_APPLY_IPC_SOURCE_FORMAT
        && manifest.ipc_magic == format!("0x{RAW_APPLY_IPC_MAGIC:016X}")
        && manifest.ipc_schema_version == RAW_APPLY_IPC_SCHEMA_VERSION
        && !manifest.eligible_for_active_gate
        && manifest.state == "COMPLETE"
        && manifest.failure_reason.is_none()
        && !manifest.case_id.is_empty()
        && manifest.probe == "passive"
        && expected_actions(&manifest.scenario).is_some()
        && validate_sha256(&manifest.target_sha256, "target SHA-256").is_ok()
        && validate_sha256(&manifest.loop_gate_sha256, "loop gate SHA-256").is_ok()
        && validate_sha256(&manifest.native_sha256, "native SHA-256").is_ok()
        && validate_sha256(&manifest.helper_sha256, "helper SHA-256").is_ok()
        && !manifest.artifact_identity.is_empty()
        && validate_relative_path(&manifest.companion_loop_artifact_path).is_ok()
        && manifest
            .companion_loop_manifest_sha256
            .as_deref()
            .is_some_and(|hash| validate_sha256(hash, "companion manifest SHA-256").is_ok())
        && manifest
            .companion_loop_events_sha256
            .as_deref()
            .is_some_and(|hash| validate_sha256(hash, "companion events SHA-256").is_ok())
        && manifest
            .companion_loop_analysis_sha256
            .as_deref()
            .is_some_and(|hash| validate_sha256(hash, "companion analysis SHA-256").is_ok())
        && manifest.passive_catalog_sha256 == expected_catalog_sha256
        && manifest.passive_capture_plan_sha256 == expected_capture_plan_sha256
        && parse_hex_mask(&manifest.passive_legacy_installed_hooks_mask)
            == Some(RAW_APPLY_LEGACY_HOOK_MASK)
        && parse_hex_mask(&manifest.passive_legacy_restored_hooks_mask)
            == Some(RAW_APPLY_LEGACY_HOOK_MASK)
        && required_raw == Some(RAW_APPLY_SOURCE_MASK)
        && observed_raw
            .is_some_and(|observed| raw_apply_observed_sources_valid(&manifest.scenario, observed))
        && parse_hex_mask(&manifest.passive_required_consumer_hooks_mask)
            == Some(RAW_APPLY_CONSUMER_HOOK_MASK)
        && parse_hex_mask(&manifest.passive_installed_consumer_hooks_mask)
            == Some(RAW_APPLY_CONSUMER_HOOK_MASK)
        && parse_hex_mask(&manifest.passive_restored_consumer_hooks_mask)
            == Some(RAW_APPLY_CONSUMER_HOOK_MASK)
        && manifest.passive_restored_slot_count == RAW_APPLY_RESTORED_SLOT_COUNT
        && manifest.passive_target_bytes_restored
        && manifest.passive_target_bytes_before_sha256 == manifest.passive_target_bytes_after_sha256
        && manifest.passive_target_bytes_before_sha256.len() == 64
        && manifest.passive_main_thread_id != 0
        && manifest.passive_overwritten_events == 0
        && manifest.passive_producer_closed
}

pub fn raw_apply_observed_sources_valid(scenario: &str, observed: u64) -> bool {
    if observed & !RAW_APPLY_SOURCE_MASK != 0 {
        return false;
    }
    let owner_source = match scenario {
        "keyboard-held-release" | "keyboard-edge" => 0x1,
        "controller-held-release" | "controller-edge" => 0x2,
        _ => return false,
    };
    observed & owner_source == owner_source
}

pub fn analyze_raw_apply_evidence(
    manifest: &RawApplyManifest,
    events: &[RawApplyArtifactEvent],
) -> RawApplyAnalysisReport {
    analyze_raw_apply_evidence_for_revision(
        manifest,
        events,
        RAW_APPLY_CATALOG_SHA256,
        RAW_APPLY_CAPTURE_PLAN_SHA256,
    )
}

fn analyze_raw_apply_evidence_for_revision(
    manifest: &RawApplyManifest,
    events: &[RawApplyArtifactEvent],
    expected_catalog_sha256: &str,
    expected_capture_plan_sha256: &str,
) -> RawApplyAnalysisReport {
    let checks = vec![
        RawApplyAnalysisCheck {
            id: "passive-manifest-binding".to_owned(),
            passed: raw_apply_manifest_valid_for_revision(
                manifest,
                expected_catalog_sha256,
                expected_capture_plan_sha256,
            ),
            detail: "schema-3 hashes, masks, 50-slot restoration, close and zero-loss counters"
                .to_owned(),
        },
        RawApplyAnalysisCheck {
            id: "passive-field-scope".to_owned(),
            passed: events.iter().all(raw_apply_event_scope_valid),
            detail: "raw/consumer/witness fields remain passive-scoped and pipeline fields empty"
                .to_owned(),
        },
        RawApplyAnalysisCheck {
            id: "passive-native-lifecycle".to_owned(),
            passed: raw_apply_lifecycle_valid(events),
            detail: "one initialized/install/restore/retire bracket with no Failure".to_owned(),
        },
        RawApplyAnalysisCheck {
            id: "passive-native-lifo".to_owned(),
            passed: raw_apply_pairs_valid(events),
            detail: "per-thread enter/exit families are balanced and immutable fields match"
                .to_owned(),
        },
        RawApplyAnalysisCheck {
            id: "passive-offline-local-closure".to_owned(),
            passed: raw_apply_offline_closure_valid(&manifest.scenario, events),
            detail: "raw sample -> nested concrete logical -> consumer transaction -> allowlisted witness closure"
                .to_owned(),
        },
    ];
    let passed = checks.iter().all(|check| check.passed);
    RawApplyAnalysisReport {
        schema_version: RAW_APPLY_ARTIFACT_SCHEMA_VERSION,
        evidence_class: EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME.to_owned(),
        result: if passed { "PASS" } else { "FAIL" }.to_owned(),
        eligible_for_active_gate: false,
        checks,
    }
}

pub fn read_raw_apply_run(
    run_directory: &Path,
) -> AppResult<(RawApplyManifest, Vec<RawApplyArtifactEvent>)> {
    let manifest_path = run_directory.join("manifest.json");
    let manifest: RawApplyManifest = serde_json::from_slice(
        &fs::read(&manifest_path)
            .usage_context(format!("cannot read {}", manifest_path.display()))?,
    )
    .usage_context(format!("cannot parse {}", manifest_path.display()))?;
    if manifest.schema_version != RAW_APPLY_ARTIFACT_SCHEMA_VERSION
        || manifest.evidence_class != EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME
    {
        return Err(AppError::evidence(
            "raw/apply artifact has crossed schema/evidence identity",
        ));
    }
    let events_path = run_directory.join("events.jsonl");
    let file = fs::File::open(&events_path)
        .usage_context(format!("cannot read {}", events_path.display()))?;
    let mut events = Vec::new();
    for (index, line) in BufReader::new(file).lines().enumerate() {
        let line = line.usage_context(format!("cannot read raw/apply line {}", index + 1))?;
        events.push(
            serde_json::from_str(&line)
                .usage_context(format!("cannot parse raw/apply line {}", index + 1))?,
        );
    }
    Ok((manifest, events))
}

pub fn replay_raw_apply_run(run_directory: &Path) -> AppResult<RawApplyAnalysisReport> {
    analyze_raw_apply_run_internal(run_directory, false)
}

pub(crate) fn replay_raw_apply_run_for_revision(
    run_directory: &Path,
    expected_catalog_sha256: &str,
    expected_capture_plan_sha256: &str,
) -> AppResult<RawApplyAnalysisReport> {
    analyze_raw_apply_run_for_revision(
        run_directory,
        false,
        expected_catalog_sha256,
        expected_capture_plan_sha256,
    )
}

pub fn analyze_raw_apply_run(run_directory: &Path) -> AppResult<RawApplyAnalysisReport> {
    analyze_raw_apply_run_internal(run_directory, true)
}

fn analyze_raw_apply_run_internal(
    run_directory: &Path,
    write_analysis: bool,
) -> AppResult<RawApplyAnalysisReport> {
    analyze_raw_apply_run_for_revision(
        run_directory,
        write_analysis,
        RAW_APPLY_CATALOG_SHA256,
        RAW_APPLY_CAPTURE_PLAN_SHA256,
    )
}

fn analyze_raw_apply_run_for_revision(
    run_directory: &Path,
    write_analysis: bool,
    expected_catalog_sha256: &str,
    expected_capture_plan_sha256: &str,
) -> AppResult<RawApplyAnalysisReport> {
    let (manifest, events) = read_raw_apply_run(run_directory)?;
    let mut report = analyze_raw_apply_evidence_for_revision(
        &manifest,
        &events,
        expected_catalog_sha256,
        expected_capture_plan_sha256,
    );
    let target_binding = (|| -> AppResult<()> {
        let before_path = run_directory.join("target-bytes-before.json");
        let after_path = run_directory.join("target-bytes-after.json");
        let before: RawApplyTargetBytesDocument = serde_json::from_slice(
            &fs::read(&before_path).usage_context("cannot read raw/apply before snapshot")?,
        )
        .usage_context("cannot parse raw/apply before snapshot")?;
        let after: RawApplyTargetBytesDocument = serde_json::from_slice(
            &fs::read(&after_path).usage_context("cannot read raw/apply after snapshot")?,
        )
        .usage_context("cannot parse raw/apply after snapshot")?;
        before.validate(&manifest.target_sha256)?;
        after.validate(&manifest.target_sha256)?;
        if before != after
            || file_sha256(&before_path)? != manifest.passive_target_bytes_before_sha256
            || file_sha256(&after_path)? != manifest.passive_target_bytes_after_sha256
        {
            return Err(AppError::evidence(
                "raw/apply 18 native-owned target slots were not restored byte-for-byte",
            ));
        }
        Ok(())
    })();
    report.checks.push(RawApplyAnalysisCheck {
        id: "passive-target-restoration".to_owned(),
        passed: target_binding.is_ok(),
        detail: target_binding.err().map_or_else(
            || "18 native + 32 companion slots restored".to_owned(),
            |e| e.to_string(),
        ),
    });
    let companion = companion_loop_hashes(run_directory, &manifest);
    let companion_passed =
        companion
            .as_ref()
            .is_ok_and(|(manifest_sha, events_sha, analysis_sha)| {
                manifest.companion_loop_manifest_sha256.as_deref() == Some(manifest_sha)
                    && manifest.companion_loop_events_sha256.as_deref() == Some(events_sha)
                    && manifest.companion_loop_analysis_sha256.as_deref() == Some(analysis_sha)
            });
    report.checks.push(RawApplyAnalysisCheck {
        id: "passive-companion-loop-binding".to_owned(),
        passed: companion_passed,
        detail: companion.err().map_or_else(
            || "nested V5 manifest/events/analysis hashes match".to_owned(),
            |e| e.to_string(),
        ),
    });
    report.result = if report.checks.iter().all(|check| check.passed) {
        "PASS"
    } else {
        "FAIL"
    }
    .to_owned();
    report.eligible_for_active_gate = false;
    if write_analysis {
        write_json(&run_directory.join("analysis.json"), &report, false)?;
    }
    Ok(report)
}

pub fn raw_apply_run_identity(run_directory: &Path) -> AppResult<(u32, String)> {
    #[derive(Deserialize)]
    #[serde(rename_all = "camelCase")]
    struct Identity {
        schema_version: u32,
        evidence_class: String,
    }
    let path = run_directory.join("manifest.json");
    let identity: Identity = serde_json::from_slice(
        &fs::read(&path).usage_context(format!("cannot read {}", path.display()))?,
    )
    .usage_context(format!("cannot parse {}", path.display()))?;
    Ok((identity.schema_version, identity.evidence_class))
}

pub fn validate_input_analysis_dispatch(
    schema_version: u32,
    evidence_class: &str,
) -> AppResult<bool> {
    match (schema_version, evidence_class) {
        (RAW_APPLY_ARTIFACT_SCHEMA_VERSION, EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME) => Ok(true),
        (RAW_APPLY_ARTIFACT_SCHEMA_VERSION, _) | (_, EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME) => Err(
            AppError::evidence("input artifact crossed schema/evidence identity"),
        ),
        (crate::input_artifact::INPUT_ARTIFACT_SCHEMA_VERSION, _) => Ok(false),
        _ => Err(AppError::evidence("unknown input artifact schema")),
    }
}

pub fn reject_passive_input_gate_evidence(evidence_class: &str) -> AppResult<()> {
    if matches!(
        evidence_class,
        EVIDENCE_PASSIVE_INPUT_RUNTIME | EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME
    ) {
        return Err(AppError::evidence(format!(
            "input gate rejects passive evidence class {evidence_class}"
        )));
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::raw_apply_ipc::{
        NativeRawApplyEvent, RAW_APPLY_FLAG_CONSUMER_KNOWN, RAW_APPLY_FLAG_CONTEXT_KNOWN,
        RAW_APPLY_FLAG_PLAYER_ORDINAL_VALID, RAW_APPLY_FLAG_SAMPLE_KNOWN,
        RAW_APPLY_FLAG_WITNESS_VALID,
    };

    fn native(kind: RawApplyEventKind, sequence: u64) -> NativeRawApplyEventRecord {
        let mut record = NativeRawApplyEventRecord {
            sequence,
            event: NativeRawApplyEvent {
                published_sequence: sequence,
                qpc: sequence * 10,
                loop_iteration: 4,
                sample_generation: 0,
                consumer_transaction: 0,
                context_generation: 5,
                thread_id: 7,
                kind: kind as u32,
                phase: LoopPhase::GameplayUpdate as u32,
                flags: RAW_APPLY_FLAG_LOOP_KNOWN
                    | RAW_APPLY_FLAG_MAIN_THREAD
                    | RAW_APPLY_FLAG_KEYBOARD_DEVICE
                    | RAW_APPLY_FLAG_CONTEXT_KNOWN,
                hook_site_rva: 0x1000,
                return_rva: 0x2000,
                original_calls: 0,
                device_key: 0,
                controller_id_raw: RAW_APPLY_NO_CONTROLLER_ID,
                selected_device_key: 0,
                player_ordinal: RAW_APPLY_NO_PLAYER_ORDINAL,
                action: 4,
                primitive: RAW_APPLY_NO_PRIMITIVE,
                raw_channel_kind: RawChannelKind::None as u32,
                raw_channel_id: 0,
                value_bits: 0,
                classification: RawApplyClassification::ExplicitGameplay as u32,
                witness_kind: PassiveWitnessKind::None as u32,
                witness_aux_bits: 0,
                error_code: 0,
            },
        };
        if matches!(
            kind,
            RawApplyEventKind::RawPollEnter
                | RawApplyEventKind::RawValue
                | RawApplyEventKind::RawPollExit
        ) {
            record.event.phase = LoopPhase::InputQuery as u32;
            record.event.hook_site_rva = RAW_APPLY_KEYBOARD_POLL_HOOK_SITE_RVA;
            record.event.return_rva = RAW_APPLY_PHYSICAL_POLL_RETURN_RVA;
        }
        record
    }

    fn lifecycle(kind: RawApplyEventKind, sequence: u64) -> NativeRawApplyEventRecord {
        let mut record = native(kind, sequence);
        record.event.loop_iteration = 0;
        record.event.sample_generation = 0;
        record.event.consumer_transaction = 0;
        record.event.context_generation = 0;
        record.event.phase = LoopPhase::OutsideEnvelope as u32;
        record.event.flags = 0;
        record.event.hook_site_rva = 0;
        record.event.return_rva = 0;
        record.event.device_key = RAW_APPLY_NO_DEVICE_KEY;
        record.event.selected_device_key = RAW_APPLY_NO_DEVICE_KEY;
        record.event.action = RAW_APPLY_NO_ACTION;
        record.event.classification = RawApplyClassification::Unknown as u32;
        record
    }

    fn exit(
        mut enter: NativeRawApplyEventRecord,
        kind: RawApplyEventKind,
        sequence: u64,
    ) -> NativeRawApplyEventRecord {
        enter.sequence = sequence;
        enter.event.published_sequence = sequence;
        enter.event.qpc += 1;
        enter.event.kind = kind as u32;
        enter.event.original_calls = 1;
        enter.event.flags |= RAW_APPLY_FLAG_ORIGINAL_CALLED;
        enter
    }

    fn manifest() -> RawApplyManifest {
        RawApplyManifest {
            schema_version: 3,
            evidence_class: EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME.to_owned(),
            ipc_source_format: RAW_APPLY_IPC_SOURCE_FORMAT.to_owned(),
            ipc_magic: format!("0x{RAW_APPLY_IPC_MAGIC:016X}"),
            ipc_schema_version: 3,
            eligible_for_active_gate: false,
            state: "COMPLETE".to_owned(),
            case_id: "raw-apply-test".to_owned(),
            probe: "passive".to_owned(),
            scenario: "keyboard-held-release".to_owned(),
            target_sha256: "1".repeat(64),
            loop_gate_sha256: "2".repeat(64),
            artifact_identity: "raw-apply-test-identity".to_owned(),
            native_sha256: "3".repeat(64),
            helper_sha256: "4".repeat(64),
            companion_loop_artifact_path: PathBuf::from("loop-context"),
            companion_loop_manifest_sha256: Some("5".repeat(64)),
            companion_loop_events_sha256: Some("6".repeat(64)),
            companion_loop_analysis_sha256: Some("7".repeat(64)),
            passive_catalog_sha256: RAW_APPLY_CATALOG_SHA256.to_owned(),
            passive_capture_plan_sha256: RAW_APPLY_CAPTURE_PLAN_SHA256.to_owned(),
            passive_legacy_installed_hooks_mask: "0x7FF".to_owned(),
            passive_legacy_restored_hooks_mask: "0x7FF".to_owned(),
            passive_required_raw_source_mask: "0x3".to_owned(),
            passive_observed_raw_source_mask: "0x3".to_owned(),
            passive_required_consumer_hooks_mask: "0x7F".to_owned(),
            passive_installed_consumer_hooks_mask: "0x7F".to_owned(),
            passive_restored_consumer_hooks_mask: "0x7F".to_owned(),
            passive_restored_slot_count: 50,
            passive_target_bytes_before_sha256: "A".repeat(64),
            passive_target_bytes_after_sha256: "A".repeat(64),
            passive_target_bytes_restored: true,
            passive_main_thread_id: 7,
            passive_overwritten_events: 0,
            passive_redacted_events: 0,
            passive_producer_closed: true,
            failure_reason: None,
        }
    }

    fn target_bytes() -> RawApplyTargetBytesDocument {
        RawApplyTargetBytesDocument {
            schema_version: RAW_APPLY_TARGET_BYTES_SCHEMA_VERSION,
            target_sha256: "1".repeat(64),
            slots: RAW_APPLY_TARGET_SLOTS
                .iter()
                .map(|(id, rva, size)| RawApplyTargetSlot {
                    id: (*id).to_owned(),
                    rva: format!("0x{rva:08X}"),
                    bytes_hex: "AA".repeat(*size),
                })
                .collect(),
        }
    }

    fn run_config() -> RawApplyRunConfig {
        RawApplyRunConfig {
            case_id: "raw-writer".to_owned(),
            scenario: "keyboard-held-release".to_owned(),
            target_sha256: "1".repeat(64),
            loop_gate_sha256: "2".repeat(64),
            artifact_identity: "raw-writer-identity".to_owned(),
            companion_loop_artifact_path: PathBuf::from("loop-context"),
            native_sha256: "3".repeat(64),
            helper_sha256: "4".repeat(64),
            target_bytes_before: target_bytes(),
        }
    }

    fn edge_closure_events() -> Vec<RawApplyArtifactEvent> {
        let mut records = Vec::new();
        let mut sequence = 1_u64;
        for action in 8..=11 {
            let sample = u64::from(action) + 1;
            let transaction = u64::from(action) + 100;
            let site = expected_consumer_site(action).unwrap();

            let mut raw = native(RawApplyEventKind::RawValue, sequence);
            raw.event.sample_generation = sample;
            raw.event.context_generation = 0;
            raw.event.player_ordinal = RAW_APPLY_NO_PLAYER_ORDINAL;
            raw.event.action = action;
            raw.event.original_calls = 1;
            raw.event.raw_channel_kind = RawChannelKind::KeyboardKeyDigital as u32;
            raw.event.raw_channel_id = action;
            raw.event.value_bits = 1;
            raw.event.classification = RawApplyClassification::ExplicitGameplay as u32;
            raw.event.flags &= !RAW_APPLY_FLAG_CONTEXT_KNOWN;
            raw.event.flags |= RAW_APPLY_FLAG_SAMPLE_KNOWN
                | RAW_APPLY_FLAG_ORIGINAL_CALLED
                | RAW_APPLY_FLAG_VALUE_VALID
                | RAW_APPLY_FLAG_RESULT_TRUE;
            records.push(raw);
            sequence += 1;

            let mut consumer = native(RawApplyEventKind::PlayerConsumerEnter, sequence);
            consumer.event.sample_generation = sample;
            consumer.event.consumer_transaction = transaction;
            consumer.event.player_ordinal = 0;
            consumer.event.action = RAW_APPLY_NO_ACTION;
            consumer.event.hook_site_rva = site;
            consumer.event.classification = RawApplyClassification::PlayerConsumerObserved as u32;
            consumer.event.flags |= RAW_APPLY_FLAG_SAMPLE_KNOWN
                | RAW_APPLY_FLAG_CONSUMER_KNOWN
                | RAW_APPLY_FLAG_PLAYER_ORDINAL_VALID;
            records.push(consumer);
            sequence += 1;

            let mut logical = native(RawApplyEventKind::LogicalPressedEnter, sequence);
            logical.event.sample_generation = sample;
            logical.event.consumer_transaction = transaction;
            logical.event.player_ordinal = 0;
            logical.event.action = action;
            logical.event.flags |= RAW_APPLY_FLAG_SAMPLE_KNOWN
                | RAW_APPLY_FLAG_CONSUMER_KNOWN
                | RAW_APPLY_FLAG_PLAYER_ORDINAL_VALID;
            records.push(logical);
            sequence += 1;
            let mut logical_exit = exit(logical, RawApplyEventKind::LogicalPressedExit, sequence);
            logical_exit.event.value_bits = 1;
            logical_exit.event.flags |= RAW_APPLY_FLAG_VALUE_VALID | RAW_APPLY_FLAG_RESULT_TRUE;
            records.push(logical_exit);
            sequence += 1;

            if !matches!(action, 9 | 10) {
                let witness_kind = match action {
                    8 => PassiveWitnessKind::BombIntentBit,
                    11 => PassiveWitnessKind::DropHoldCounter,
                    _ => unreachable!(),
                };
                let mut before = native(RawApplyEventKind::WorldWitnessBefore, sequence);
                before.event.sample_generation = sample;
                before.event.consumer_transaction = transaction;
                before.event.player_ordinal = 0;
                before.event.action = action;
                before.event.hook_site_rva = site;
                before.event.classification = RawApplyClassification::PassiveWorldWitness as u32;
                before.event.witness_kind = witness_kind as u32;
                before.event.flags |= RAW_APPLY_FLAG_SAMPLE_KNOWN
                    | RAW_APPLY_FLAG_CONSUMER_KNOWN
                    | RAW_APPLY_FLAG_PLAYER_ORDINAL_VALID
                    | RAW_APPLY_FLAG_WITNESS_VALID
                    | RAW_APPLY_FLAG_VALUE_VALID;
                records.push(before);
                sequence += 1;
                let mut after = exit(before, RawApplyEventKind::WorldWitnessAfter, sequence);
                after.event.value_bits = 1;
                records.push(after);
                sequence += 1;
            }

            let consumer_exit = exit(consumer, RawApplyEventKind::PlayerConsumerExit, sequence);
            records.push(consumer_exit);
            sequence += 1;
        }
        records
            .iter()
            .enumerate()
            .map(|(index, record)| raw_apply_event_from_native(record, index as u64 + 1).unwrap())
            .collect()
    }

    struct HeldTransactionSpec {
        actions: Vec<u32>,
        site: u32,
        witness_action: Option<u32>,
        duplicate_scalar: bool,
    }

    fn held_release_closure_events(
        omitted_singletons: &[u32],
        extras: &[HeldTransactionSpec],
    ) -> Vec<RawApplyArtifactEvent> {
        fn push_raw(
            records: &mut Vec<NativeRawApplyEventRecord>,
            sequence: &mut u64,
            action: u32,
            sample: u64,
            value: f32,
            edge_flag: u32,
        ) {
            let mut raw = native(RawApplyEventKind::RawValue, *sequence);
            raw.event.sample_generation = sample;
            raw.event.context_generation = 0;
            raw.event.player_ordinal = RAW_APPLY_NO_PLAYER_ORDINAL;
            raw.event.action = action;
            raw.event.original_calls = 1;
            raw.event.raw_channel_kind = RawChannelKind::KeyboardKeyDigital as u32;
            raw.event.raw_channel_id = action;
            raw.event.value_bits = value.to_bits();
            raw.event.classification = RawApplyClassification::ExplicitGameplay as u32;
            raw.event.flags &= !RAW_APPLY_FLAG_CONTEXT_KNOWN;
            raw.event.flags |= RAW_APPLY_FLAG_SAMPLE_KNOWN
                | RAW_APPLY_FLAG_ORIGINAL_CALLED
                | RAW_APPLY_FLAG_VALUE_VALID
                | edge_flag;
            records.push(raw);
            *sequence += 1;
        }

        #[allow(clippy::too_many_arguments)]
        fn push_transaction(
            records: &mut Vec<NativeRawApplyEventRecord>,
            sequence: &mut u64,
            actions: &[u32],
            site: u32,
            transaction: u64,
            sample: u64,
            witness_action: Option<u32>,
            duplicate_scalar: bool,
        ) {
            let mut consumer = native(RawApplyEventKind::PlayerConsumerEnter, *sequence);
            consumer.event.sample_generation = sample;
            consumer.event.consumer_transaction = transaction;
            consumer.event.player_ordinal = 0;
            consumer.event.action = RAW_APPLY_NO_ACTION;
            consumer.event.hook_site_rva = site;
            consumer.event.classification = RawApplyClassification::PlayerConsumerObserved as u32;
            consumer.event.flags |= RAW_APPLY_FLAG_SAMPLE_KNOWN
                | RAW_APPLY_FLAG_CONSUMER_KNOWN
                | RAW_APPLY_FLAG_PLAYER_ORDINAL_VALID;
            records.push(consumer);
            *sequence += 1;

            for action in actions {
                let mut logical = native(RawApplyEventKind::LogicalPressedEnter, *sequence);
                logical.event.sample_generation = sample;
                logical.event.consumer_transaction = transaction;
                logical.event.player_ordinal = 0;
                logical.event.action = *action;
                logical.event.flags |= RAW_APPLY_FLAG_SAMPLE_KNOWN
                    | RAW_APPLY_FLAG_CONSUMER_KNOWN
                    | RAW_APPLY_FLAG_PLAYER_ORDINAL_VALID;
                records.push(logical);
                *sequence += 1;
                let mut logical_exit =
                    exit(logical, RawApplyEventKind::LogicalPressedExit, *sequence);
                logical_exit.event.value_bits = 1;
                logical_exit.event.flags |= RAW_APPLY_FLAG_VALUE_VALID | RAW_APPLY_FLAG_RESULT_TRUE;
                records.push(logical_exit);
                *sequence += 1;
            }
            if duplicate_scalar {
                let mut logical = native(RawApplyEventKind::LogicalScalarEnter, *sequence);
                logical.event.sample_generation = sample;
                logical.event.consumer_transaction = transaction;
                logical.event.player_ordinal = 0;
                logical.event.action = actions[0];
                logical.event.flags |= RAW_APPLY_FLAG_SAMPLE_KNOWN
                    | RAW_APPLY_FLAG_CONSUMER_KNOWN
                    | RAW_APPLY_FLAG_PLAYER_ORDINAL_VALID;
                records.push(logical);
                *sequence += 1;
                let mut logical_exit =
                    exit(logical, RawApplyEventKind::LogicalScalarExit, *sequence);
                logical_exit.event.value_bits = 1.0_f32.to_bits();
                logical_exit.event.flags |= RAW_APPLY_FLAG_VALUE_VALID | RAW_APPLY_FLAG_RESULT_TRUE;
                records.push(logical_exit);
                *sequence += 1;
            }

            let witness_kind = if site == expected_consumer_site(0).unwrap() {
                PassiveWitnessKind::MovementDirection
            } else {
                PassiveWitnessKind::ShootingDirection
            };
            let mut before = native(RawApplyEventKind::WorldWitnessBefore, *sequence);
            before.event.sample_generation = sample;
            before.event.consumer_transaction = transaction;
            before.event.player_ordinal = 0;
            before.event.action = witness_action.unwrap_or(RAW_APPLY_NO_ACTION);
            before.event.hook_site_rva = site;
            before.event.classification = RawApplyClassification::PassiveWorldWitness as u32;
            before.event.witness_kind = witness_kind as u32;
            before.event.flags |= RAW_APPLY_FLAG_SAMPLE_KNOWN
                | RAW_APPLY_FLAG_CONSUMER_KNOWN
                | RAW_APPLY_FLAG_PLAYER_ORDINAL_VALID
                | RAW_APPLY_FLAG_WITNESS_VALID
                | RAW_APPLY_FLAG_VALUE_VALID;
            records.push(before);
            *sequence += 1;
            let mut after = exit(before, RawApplyEventKind::WorldWitnessAfter, *sequence);
            after.event.value_bits = 1;
            records.push(after);
            *sequence += 1;

            records.push(exit(
                consumer,
                RawApplyEventKind::PlayerConsumerExit,
                *sequence,
            ));
            *sequence += 1;
        }

        let mut records = Vec::new();
        let mut sequence = 1_u64;
        for action in 0..=7 {
            let first_sample = u64::from(action) * 10 + 1;
            push_raw(&mut records, &mut sequence, action, first_sample, 0.0, 0);
            push_raw(
                &mut records,
                &mut sequence,
                action,
                first_sample + 1,
                1.0,
                RAW_APPLY_FLAG_RAW_RISING,
            );
            push_raw(
                &mut records,
                &mut sequence,
                action,
                first_sample + 2,
                0.0,
                RAW_APPLY_FLAG_RAW_FALLING,
            );
        }
        for action in 0..=7 {
            if omitted_singletons.contains(&action) {
                continue;
            }
            push_transaction(
                &mut records,
                &mut sequence,
                &[action],
                expected_consumer_site(action).unwrap(),
                1_000 + u64::from(action),
                u64::from(action) * 10 + 2,
                None,
                false,
            );
        }
        for (index, extra) in extras.iter().enumerate() {
            let sample = 10_000 + index as u64;
            for action in &extra.actions {
                push_raw(&mut records, &mut sequence, *action, sample, 1.0, 0);
            }
            push_transaction(
                &mut records,
                &mut sequence,
                &extra.actions,
                extra.site,
                2_000 + index as u64,
                sample,
                extra.witness_action,
                extra.duplicate_scalar,
            );
        }
        records
            .iter()
            .enumerate()
            .map(|(index, record)| raw_apply_event_from_native(record, index as u64 + 1).unwrap())
            .collect()
    }

    #[test]
    fn converter_keeps_raw_and_witness_data_out_of_legacy_pipeline_fields() {
        let mut raw = native(RawApplyEventKind::RawValue, 1);
        raw.event.sample_generation = 8;
        raw.event.flags |= RAW_APPLY_FLAG_SAMPLE_KNOWN
            | RAW_APPLY_FLAG_ORIGINAL_CALLED
            | RAW_APPLY_FLAG_VALUE_VALID
            | RAW_APPLY_FLAG_RAW_RISING;
        raw.event.original_calls = 1;
        raw.event.context_generation = 0;
        raw.event.flags &= !RAW_APPLY_FLAG_CONTEXT_KNOWN;
        raw.event.action = 4;
        raw.event.raw_channel_kind = RawChannelKind::KeyboardKeyDigital as u32;
        raw.event.raw_channel_id = 4;
        raw.event.value_bits = 1;
        raw.event.classification = RawApplyClassification::ExplicitGameplay as u32;
        let event = raw_apply_event_from_native(&raw, 1).unwrap();
        assert_eq!(event.passive_raw_value_bits, Some(1));
        assert!(event.canonical_state_hex.is_none());
        assert!(event.queue_sequence.is_none());
        assert!(event.applied_state_hex.is_none());
        assert!(event.world_witness_before.is_none());
        assert!(raw_apply_event_scope_valid(&event));
    }

    #[test]
    fn raw_apply_writer_is_create_new_and_failed_runs_remain_replayable_evidence() {
        let root = tempfile::tempdir().unwrap();
        let output = root.path().join("raw-run");
        let mut run = RawApplyRun::create(&output, run_config()).unwrap();
        assert_eq!(
            run.events_file.capacity(),
            RAW_APPLY_EVENT_BUFFER_CAPACITY_BYTES
        );
        for sequence in 1..=128 {
            run.append_native(&lifecycle(RawApplyEventKind::NativeInitialized, sequence))
                .unwrap();
        }
        assert_eq!(fs::metadata(output.join("events.jsonl")).unwrap().len(), 0);
        assert!(RawApplyRun::create(&output, run_config()).is_err());
        run.fail(
            "mock installation failure",
            None,
            RawApplyFinish {
                main_thread_id: 7,
                legacy_installed_hooks_mask: 0,
                legacy_restored_hooks_mask: 0,
                required_raw_source_mask: RAW_APPLY_SOURCE_MASK,
                observed_raw_source_mask: 0,
                required_consumer_hooks_mask: RAW_APPLY_CONSUMER_HOOK_MASK,
                installed_consumer_hooks_mask: 0,
                restored_consumer_hooks_mask: 0,
                overwritten_events: 0,
                redacted_events: 12,
                producer_closed: true,
            },
        )
        .unwrap();
        let saved: RawApplyManifest =
            serde_json::from_slice(&fs::read(output.join("manifest.json")).unwrap()).unwrap();
        assert_eq!(saved.state, "FAILED");
        assert_eq!(saved.passive_redacted_events, 12);
        let events = fs::read_to_string(output.join("events.jsonl")).unwrap();
        assert_eq!(events.lines().count(), 128);
        let first: RawApplyArtifactEvent =
            serde_json::from_str(events.lines().next().unwrap()).unwrap();
        assert_eq!(first.native_sequence, 1);
    }

    #[test]
    fn raw_apply_target_snapshot_is_exact_order_size_and_rva() {
        let valid = target_bytes();
        assert!(valid.validate(&"1".repeat(64)).is_ok());
        let mut reordered = valid.clone();
        reordered.slots.swap(0, 1);
        assert!(reordered.validate(&"1".repeat(64)).is_err());
        let mut wrong_size = valid.clone();
        wrong_size.slots[12].bytes_hex = "AA".repeat(4);
        assert!(wrong_size.validate(&"1".repeat(64)).is_err());
        let mut wrong_rva = valid;
        wrong_rva.slots[17].rva = "0x00385BE8".to_owned();
        assert!(wrong_rva.validate(&"1".repeat(64)).is_err());
    }

    #[test]
    fn offline_closure_uses_post_poll_raw_and_nested_logical_not_manager_or_setter() {
        let events = edge_closure_events();
        assert!(raw_apply_offline_closure_valid("keyboard-edge", &events));

        let mut zero_raw = events.clone();
        zero_raw
            .iter_mut()
            .find(|event| {
                event.native_kind == RawApplyEventKind::RawValue && event.passive_action == Some(8)
            })
            .unwrap()
            .passive_raw_value_bits = Some(0.0_f32.to_bits());
        assert!(!raw_apply_offline_closure_valid("keyboard-edge", &zero_raw));

        let mut nonfinite_raw = events.clone();
        nonfinite_raw
            .iter_mut()
            .find(|event| {
                event.native_kind == RawApplyEventKind::RawValue && event.passive_action == Some(8)
            })
            .unwrap()
            .passive_raw_value_bits = Some(f32::NAN.to_bits());
        assert!(!raw_apply_offline_closure_valid(
            "keyboard-edge",
            &nonfinite_raw
        ));

        let mut future_raw = events.clone();
        future_raw
            .iter_mut()
            .find(|event| {
                event.native_kind == RawApplyEventKind::RawValue && event.passive_action == Some(8)
            })
            .unwrap()
            .native_sequence = u64::MAX;
        assert!(!raw_apply_offline_closure_valid(
            "keyboard-edge",
            &future_raw
        ));

        let mut missing_raw = events.clone();
        missing_raw.retain(|event| {
            event.native_kind != RawApplyEventKind::RawValue || event.passive_action != Some(8)
        });
        assert!(!raw_apply_offline_closure_valid(
            "keyboard-edge",
            &missing_raw
        ));

        let mut wrong_context = events.clone();
        let logical = wrong_context
            .iter_mut()
            .find(|event| {
                event.native_kind == RawApplyEventKind::LogicalPressedExit
                    && event.passive_action == Some(9)
            })
            .unwrap();
        logical.passive_context_generation = Some(999);
        assert!(!raw_apply_offline_closure_valid(
            "keyboard-edge",
            &wrong_context
        ));

        let mut manager_only = events;
        for event in manager_only.iter_mut().filter(|event| {
            event.passive_action == Some(10)
                && matches!(
                    event.native_kind,
                    RawApplyEventKind::LogicalPressedEnter | RawApplyEventKind::LogicalPressedExit
                )
        }) {
            event.native_kind = if event.native_kind == RawApplyEventKind::LogicalPressedEnter {
                RawApplyEventKind::ManagerPressedEnter
            } else {
                RawApplyEventKind::ManagerPressedExit
            };
        }
        assert!(!raw_apply_offline_closure_valid(
            "keyboard-edge",
            &manager_only
        ));
    }

    #[test]
    fn closure_ignores_same_family_diagonals_but_requires_singleton_proof() {
        let movement_site = expected_consumer_site(0).unwrap();
        let shooting_site = expected_consumer_site(4).unwrap();
        let diagonals = [
            HeldTransactionSpec {
                actions: vec![0, 3],
                site: movement_site,
                witness_action: None,
                duplicate_scalar: false,
            },
            HeldTransactionSpec {
                actions: vec![5, 6],
                site: shooting_site,
                witness_action: None,
                duplicate_scalar: false,
            },
            HeldTransactionSpec {
                actions: vec![5, 7],
                site: shooting_site,
                witness_action: Some(5),
                duplicate_scalar: false,
            },
        ];
        let all_singletons_and_diagonals = held_release_closure_events(&[], &diagonals);
        assert!(raw_apply_offline_closure_valid(
            "keyboard-held-release",
            &all_singletons_and_diagonals
        ));

        let only_diagonal_for_zero_and_three =
            held_release_closure_events(&[0, 3], &diagonals[..1]);
        assert!(!raw_apply_offline_closure_valid(
            "keyboard-held-release",
            &only_diagonal_for_zero_and_three
        ));

        // Even an explicitly action-tagged changed witness inside an ambiguous
        // transaction must not prove either candidate.
        let only_action_tagged_diagonal_for_five_and_seven =
            held_release_closure_events(&[5, 7], &diagonals[2..]);
        assert!(!raw_apply_offline_closure_valid(
            "keyboard-held-release",
            &only_action_tagged_diagonal_for_five_and_seven
        ));
    }

    #[test]
    fn closure_rejects_cross_family_and_wrong_consumer_sites() {
        let movement_site = expected_consumer_site(0).unwrap();
        let shooting_site = expected_consumer_site(4).unwrap();
        for contradiction in [
            HeldTransactionSpec {
                actions: vec![0, 4],
                site: movement_site,
                witness_action: None,
                duplicate_scalar: false,
            },
            HeldTransactionSpec {
                actions: vec![0, 3],
                site: shooting_site,
                witness_action: None,
                duplicate_scalar: false,
            },
            HeldTransactionSpec {
                actions: vec![0],
                site: shooting_site,
                witness_action: None,
                duplicate_scalar: false,
            },
        ] {
            assert!(!raw_apply_offline_closure_valid(
                "keyboard-held-release",
                &held_release_closure_events(&[], &[contradiction])
            ));
        }
    }

    #[test]
    fn closure_never_reuses_an_ambiguous_or_cross_identity_witness() {
        let movement_site = expected_consumer_site(0).unwrap();
        let mismatched_action = HeldTransactionSpec {
            actions: vec![0],
            site: movement_site,
            witness_action: Some(1),
            duplicate_scalar: false,
        };
        assert!(!raw_apply_offline_closure_valid(
            "keyboard-held-release",
            &held_release_closure_events(&[0], &[mismatched_action])
        ));

        let singleton_with_two_logical_families = HeldTransactionSpec {
            actions: vec![0],
            site: movement_site,
            witness_action: None,
            duplicate_scalar: true,
        };
        let mut wrong_sample =
            held_release_closure_events(&[0], &[singleton_with_two_logical_families]);
        assert!(raw_apply_offline_closure_valid(
            "keyboard-held-release",
            &wrong_sample
        ));
        for witness in wrong_sample.iter_mut().filter(|event| {
            matches!(
                event.native_kind,
                RawApplyEventKind::WorldWitnessBefore | RawApplyEventKind::WorldWitnessAfter
            ) && event.passive_consumer_transaction == Some(2_000)
        }) {
            witness.passive_sample_generation = Some(99_999);
        }
        assert!(!raw_apply_offline_closure_valid(
            "keyboard-held-release",
            &wrong_sample
        ));
    }

    #[test]
    fn held_release_requires_owner_aligned_inactive_active_inactive_raw_transitions() {
        fn raw(action: u32, generation: u64, value: f32) -> RawApplyArtifactEvent {
            let mut record = native(RawApplyEventKind::RawValue, generation);
            record.event.sample_generation = generation;
            record.event.context_generation = 0;
            record.event.player_ordinal = RAW_APPLY_NO_PLAYER_ORDINAL;
            record.event.action = action;
            record.event.original_calls = 1;
            record.event.raw_channel_kind = RawChannelKind::DevicePacketU32 as u32;
            record.event.raw_channel_id = action;
            record.event.value_bits = value.to_bits();
            record.event.classification = RawApplyClassification::ExplicitGameplay as u32;
            record.event.flags &= !RAW_APPLY_FLAG_CONTEXT_KNOWN;
            record.event.flags |= RAW_APPLY_FLAG_SAMPLE_KNOWN
                | RAW_APPLY_FLAG_ORIGINAL_CALLED
                | RAW_APPLY_FLAG_VALUE_VALID;
            raw_apply_event_from_native(&record, generation).unwrap()
        }

        let mut events = Vec::new();
        for action in 0..=7 {
            let generation = u64::from(action) * 3 + 1;
            events.push(raw(action, generation, 0.0));
            events.push(raw(action, generation + 1, 1.0));
            events.push(raw(action, generation + 2, 0.0));
        }
        let transitions = held_release_transition_keys(&events);
        assert!((0..=7).all(|action| {
            transitions.contains(&RawOwnerActionKey::from_event(&events[0], action))
        }));

        for event in events.iter_mut().filter(|event| {
            event.passive_action == Some(3)
                && event.passive_raw_value_bits == Some(1.0_f32.to_bits())
        }) {
            event.passive_raw_value_bits = Some(0.0_f32.to_bits());
        }
        assert!(
            !held_release_transition_keys(&events)
                .contains(&RawOwnerActionKey::from_event(&events[0], 3))
        );

        let mut flagged = vec![raw(8, 99, 0.0), raw(8, 100, 1.0), raw(8, 101, 0.0)];
        flagged[1].flags |= RAW_APPLY_FLAG_RAW_RISING;
        flagged[2].flags |= RAW_APPLY_FLAG_RAW_FALLING;
        assert!(
            held_release_transition_keys(&flagged)
                .contains(&RawOwnerActionKey::from_event(&flagged[0], 8))
        );
        flagged[2].passive_raw_value_bits = Some(1.0_f32.to_bits());
        assert!(
            !held_release_transition_keys(&flagged)
                .contains(&RawOwnerActionKey::from_event(&flagged[0], 8))
        );
        let mut impossible_first_edge = vec![raw(8, 100, 1.0), raw(8, 101, 0.0)];
        impossible_first_edge[0].flags |= RAW_APPLY_FLAG_RAW_RISING;
        impossible_first_edge[1].flags |= RAW_APPLY_FLAG_RAW_FALLING;
        assert!(
            !held_release_transition_keys(&impossible_first_edge)
                .contains(&RawOwnerActionKey::from_event(&impossible_first_edge[0], 8))
        );
    }

    #[test]
    fn converter_rejects_action_twelve_and_lifecycle_payload() {
        let mut action = native(RawApplyEventKind::LogicalPressedEnter, 1);
        action.event.action = 12;
        assert!(raw_apply_event_from_native(&action, 1).is_err());

        let mut initialized = lifecycle(RawApplyEventKind::NativeInitialized, 1);
        initialized.event.context_generation = 1;
        assert!(raw_apply_event_from_native(&initialized, 1).is_err());
        let mut failure = lifecycle(RawApplyEventKind::Failure, 2);
        assert!(raw_apply_event_from_native(&failure, 2).is_err());
        failure.event.error_code = 9;
        assert!(raw_apply_event_from_native(&failure, 2).is_ok());
    }

    #[test]
    fn schema_three_privacy_rejects_every_persisted_primitive() {
        let mut manager = native(RawApplyEventKind::ManagerPressedEnter, 1);
        manager.event.classification = RawApplyClassification::UniqueGameplay as u32;
        manager.event.primitive = 0x20;
        assert!(raw_apply_event_from_native(&manager, 1).is_err());
        manager.event.primitive = RAW_APPLY_NO_PRIMITIVE;
        assert!(raw_apply_event_from_native(&manager, 1).is_ok());
    }

    #[test]
    fn raw_value_binds_sample_not_poll_channel_and_requires_explicit_action_projection() {
        let mut poll_enter = native(RawApplyEventKind::RawPollEnter, 1);
        poll_enter.event.sample_generation = 8;
        poll_enter.event.flags |= RAW_APPLY_FLAG_SAMPLE_KNOWN;
        poll_enter.event.context_generation = 0;
        poll_enter.event.flags &= !RAW_APPLY_FLAG_CONTEXT_KNOWN;
        poll_enter.event.action = RAW_APPLY_NO_ACTION;
        poll_enter.event.raw_channel_kind = RawChannelKind::DevicePacketU32 as u32;
        poll_enter.event.raw_channel_id = 99;
        poll_enter.event.classification = RawApplyClassification::RawDeviceOnly as u32;

        let mut value = native(RawApplyEventKind::RawValue, 3);
        value.event.sample_generation = 8;
        value.event.flags |= RAW_APPLY_FLAG_SAMPLE_KNOWN
            | RAW_APPLY_FLAG_ORIGINAL_CALLED
            | RAW_APPLY_FLAG_VALUE_VALID;
        value.event.original_calls = 1;
        value.event.context_generation = 0;
        value.event.flags &= !RAW_APPLY_FLAG_CONTEXT_KNOWN;
        value.event.raw_channel_kind = RawChannelKind::KeyboardKeyDigital as u32;
        value.event.raw_channel_id = 4;
        value.event.value_bits = 1;
        value.event.classification = RawApplyClassification::ExplicitGameplay as u32;

        let mut last_gameplay_action = value;
        last_gameplay_action.event.action = 11;
        last_gameplay_action.event.raw_channel_id = 11;
        assert!(raw_apply_event_from_native(&last_gameplay_action, 3).is_ok());
        let mut outside_action = value;
        outside_action.event.action = 12;
        outside_action.event.raw_channel_id = 12;
        assert!(raw_apply_event_from_native(&outside_action, 3).is_err());

        let mut poll_exit = exit(poll_enter, RawApplyEventKind::RawPollExit, 2);
        poll_exit.event.flags |= RAW_APPLY_FLAG_VALUE_VALID;
        let events = [poll_enter, poll_exit, value]
            .iter()
            .enumerate()
            .map(|(index, record)| raw_apply_event_from_native(record, index as u64 + 1).unwrap())
            .collect::<Vec<_>>();
        assert!(raw_apply_pairs_valid(&events));
        let mut duplicate_poll = events.clone();
        duplicate_poll.insert(2, events[0].clone());
        duplicate_poll.insert(3, events[1].clone());
        assert!(!raw_apply_pairs_valid(&duplicate_poll));
        let mut duplicate_value = events.clone();
        duplicate_value.push(events[2].clone());
        assert!(!raw_apply_pairs_valid(&duplicate_value));
        let mut early_value = events.clone();
        early_value[2].qpc = early_value[1].qpc - 1;
        assert!(!raw_apply_pairs_valid(&early_value));

        let mut wrong_boundary = value;
        wrong_boundary.event.phase = LoopPhase::GameplayUpdate as u32;
        assert!(raw_apply_event_from_native(&wrong_boundary, 2).is_err());
        wrong_boundary = value;
        wrong_boundary.event.return_rva = RAW_APPLY_PHYSICAL_POLL_RETURN_RVA + 1;
        assert!(raw_apply_event_from_native(&wrong_boundary, 2).is_err());
        wrong_boundary = value;
        wrong_boundary.event.hook_site_rva = RAW_APPLY_GAMEPAD_POLL_HOOK_SITE_RVA;
        assert!(raw_apply_event_from_native(&wrong_boundary, 2).is_err());
        wrong_boundary.event.flags &= !RAW_APPLY_FLAG_KEYBOARD_DEVICE;
        wrong_boundary.event.flags |= RAW_APPLY_FLAG_GAMEPAD_DEVICE;
        assert!(raw_apply_event_from_native(&wrong_boundary, 2).is_ok());
        wrong_boundary = value;
        wrong_boundary.event.raw_channel_id = 5;
        assert!(raw_apply_event_from_native(&wrong_boundary, 2).is_err());

        let mut ambiguous = value;
        ambiguous.event.classification = RawApplyClassification::AmbiguousOrOutsideGameplay as u32;
        assert!(raw_apply_event_from_native(&ambiguous, 2).is_err());
        ambiguous.event.classification = RawApplyClassification::ExplicitGameplay as u32;
        ambiguous.event.action = 12;
        assert!(raw_apply_event_from_native(&ambiguous, 2).is_err());
    }

    #[test]
    fn schema_three_pairs_are_per_thread_lifo_and_scope_immutable() {
        let outer_enter = native(RawApplyEventKind::LogicalPressedEnter, 1);
        let mut inner_enter = native(RawApplyEventKind::LogicalTriggeredEnter, 2);
        inner_enter.event.action = 5;
        inner_enter.event.return_rva = 0x2004;
        let mut inner_exit = exit(inner_enter, RawApplyEventKind::LogicalTriggeredExit, 3);
        inner_exit.event.flags |= RAW_APPLY_FLAG_VALUE_VALID;
        let mut outer_exit = exit(outer_enter, RawApplyEventKind::LogicalPressedExit, 4);
        outer_exit.event.flags |= RAW_APPLY_FLAG_VALUE_VALID;
        let events = [outer_enter, inner_enter, inner_exit, outer_exit]
            .iter()
            .enumerate()
            .map(|(index, record)| raw_apply_event_from_native(record, index as u64 + 1).unwrap())
            .collect::<Vec<_>>();
        assert!(raw_apply_pairs_valid(&events));
        let mut wrong = events.clone();
        wrong.swap(2, 3);
        assert!(!raw_apply_pairs_valid(&wrong));
        let mut mismatch = events.clone();
        mismatch[3].passive_context_generation = Some(99);
        assert!(!raw_apply_pairs_valid(&mismatch));
    }

    #[test]
    fn raw_consumer_world_pairs_bind_generations_without_apply_fields() {
        let mut consumer = native(RawApplyEventKind::PlayerConsumerEnter, 1);
        consumer.event.consumer_transaction = 44;
        consumer.event.player_ordinal = 0;
        consumer.event.action = RAW_APPLY_NO_ACTION;
        consumer.event.hook_site_rva = RAW_APPLY_CONSUMER_HOOK_SITE_RVAS[0];
        consumer.event.classification = RawApplyClassification::PlayerConsumerObserved as u32;
        consumer.event.flags |= RAW_APPLY_FLAG_CONSUMER_KNOWN | RAW_APPLY_FLAG_PLAYER_ORDINAL_VALID;
        let consumer_exit = exit(consumer, RawApplyEventKind::PlayerConsumerExit, 2);
        let pair = [consumer, consumer_exit]
            .iter()
            .enumerate()
            .map(|(index, record)| raw_apply_event_from_native(record, index as u64 + 1).unwrap())
            .collect::<Vec<_>>();
        assert!(raw_apply_pairs_valid(&pair));
        assert!(pair.iter().all(|event| event.applied_state_hex.is_none()));

        let mut before = native(RawApplyEventKind::WorldWitnessBefore, 3);
        before.event.consumer_transaction = 44;
        before.event.player_ordinal = 0;
        before.event.hook_site_rva = RAW_APPLY_CONSUMER_HOOK_SITE_RVAS[0];
        before.event.classification = RawApplyClassification::PassiveWorldWitness as u32;
        before.event.witness_kind = PassiveWitnessKind::MovementDirection as u32;
        before.event.flags |= RAW_APPLY_FLAG_CONSUMER_KNOWN
            | RAW_APPLY_FLAG_PLAYER_ORDINAL_VALID
            | RAW_APPLY_FLAG_WITNESS_VALID
            | RAW_APPLY_FLAG_VALUE_VALID;
        let after = exit(before, RawApplyEventKind::WorldWitnessAfter, 4);
        let witnesses = [before, after]
            .iter()
            .enumerate()
            .map(|(index, record)| raw_apply_event_from_native(record, index as u64 + 1).unwrap())
            .collect::<Vec<_>>();
        assert!(raw_apply_pairs_valid(&witnesses));
        assert!(witnesses.iter().all(|event| {
            event.world_witness_before.is_none() && event.world_witness_after.is_none()
        }));

        let mut consumer_with_action = consumer;
        consumer_with_action.event.action = 4;
        assert!(raw_apply_event_from_native(&consumer_with_action, 1).is_err());
        let mut forged_witness = before;
        forged_witness.event.hook_site_rva = RAW_APPLY_CONSUMER_HOOK_SITE_RVAS[2];
        assert!(raw_apply_event_from_native(&forged_witness, 3).is_err());
    }

    #[test]
    fn movement_witness_accepts_direction_and_velocity_but_keeps_scope_closed() {
        fn movement_witness(
            kind: RawApplyEventKind,
            witness: PassiveWitnessKind,
            sequence: u64,
        ) -> NativeRawApplyEventRecord {
            let mut record = native(kind, sequence);
            record.event.consumer_transaction = 44;
            record.event.player_ordinal = 0;
            record.event.action = RAW_APPLY_NO_ACTION;
            record.event.hook_site_rva = 0x0038_5BC2;
            record.event.classification = RawApplyClassification::PassiveWorldWitness as u32;
            record.event.witness_kind = witness as u32;
            record.event.flags |= RAW_APPLY_FLAG_CONSUMER_KNOWN
                | RAW_APPLY_FLAG_PLAYER_ORDINAL_VALID
                | RAW_APPLY_FLAG_WITNESS_VALID
                | RAW_APPLY_FLAG_VALUE_VALID;
            if kind == RawApplyEventKind::WorldWitnessAfter {
                record.event.original_calls = 1;
                record.event.flags |= RAW_APPLY_FLAG_ORIGINAL_CALLED;
            }
            record
        }

        let direction_before = movement_witness(
            RawApplyEventKind::WorldWitnessBefore,
            PassiveWitnessKind::MovementDirection,
            1,
        );
        let velocity_before = movement_witness(
            RawApplyEventKind::WorldWitnessBefore,
            PassiveWitnessKind::PlayerVelocityF32x2,
            2,
        );
        let mut direction_after = movement_witness(
            RawApplyEventKind::WorldWitnessAfter,
            PassiveWitnessKind::MovementDirection,
            3,
        );
        direction_after.event.qpc = 30;
        let mut velocity_after = movement_witness(
            RawApplyEventKind::WorldWitnessAfter,
            PassiveWitnessKind::PlayerVelocityF32x2,
            4,
        );
        velocity_after.event.qpc = 40;
        let records = [
            direction_before,
            velocity_before,
            direction_after,
            velocity_after,
        ];
        let events = records
            .iter()
            .enumerate()
            .map(|(index, record)| raw_apply_event_from_native(record, index as u64 + 1).unwrap())
            .collect::<Vec<_>>();
        assert!(raw_apply_pairs_valid(&events));

        let mut wrong_site = velocity_before;
        wrong_site.event.hook_site_rva = 0x0038_5BE0;
        assert!(raw_apply_event_from_native(&wrong_site, 2).is_err());

        let mut outside_action = direction_before;
        outside_action.event.action = 12;
        assert!(raw_apply_event_from_native(&outside_action, 1).is_err());
        let mut persisted_primitive = direction_before;
        persisted_primitive.event.primitive = 0x41;
        assert!(raw_apply_event_from_native(&persisted_primitive, 1).is_err());

        let mut explicit_gameplay_action = direction_before;
        explicit_gameplay_action.event.action = 0;
        assert!(raw_apply_event_from_native(&explicit_gameplay_action, 1).is_ok());
    }

    #[test]
    fn context_owner_is_a_same_kind_original_once_pair() {
        let mut owner_enter = native(RawApplyEventKind::ContextOwner, 1);
        owner_enter.event.hook_site_rva = RAW_APPLY_OWNER_HOOK_SITE_RVA;
        owner_enter.event.action = RAW_APPLY_NO_ACTION;
        owner_enter.event.player_ordinal = 0;
        owner_enter.event.classification = RawApplyClassification::PlayerConsumerObserved as u32;
        owner_enter.event.flags |= RAW_APPLY_FLAG_PLAYER_ORDINAL_VALID;
        let owner_exit = exit(owner_enter, RawApplyEventKind::ContextOwner, 2);
        let events = [owner_enter, owner_exit]
            .iter()
            .enumerate()
            .map(|(index, record)| raw_apply_event_from_native(record, index as u64 + 1).unwrap())
            .collect::<Vec<_>>();
        assert!(raw_apply_pairs_valid(&events));
        assert!(!raw_apply_pairs_valid(&events[..1]));
    }

    #[test]
    fn multiple_witness_kinds_pair_by_key_not_call_stack_order() {
        let mut direction = native(RawApplyEventKind::WorldWitnessBefore, 1);
        direction.event.sample_generation = 7;
        direction.event.consumer_transaction = 44;
        direction.event.player_ordinal = 0;
        direction.event.action = 4;
        direction.event.hook_site_rva = RAW_APPLY_CONSUMER_HOOK_SITE_RVAS[4];
        direction.event.classification = RawApplyClassification::PassiveWorldWitness as u32;
        direction.event.witness_kind = PassiveWitnessKind::ShootingDirection as u32;
        direction.event.flags |= RAW_APPLY_FLAG_SAMPLE_KNOWN
            | RAW_APPLY_FLAG_CONSUMER_KNOWN
            | RAW_APPLY_FLAG_PLAYER_ORDINAL_VALID
            | RAW_APPLY_FLAG_WITNESS_VALID
            | RAW_APPLY_FLAG_VALUE_VALID;
        let mut aim = direction;
        aim.sequence = 2;
        aim.event.published_sequence = 2;
        aim.event.qpc += 1;
        aim.event.witness_kind = PassiveWitnessKind::AimVector as u32;
        let direction_after = exit(direction, RawApplyEventKind::WorldWitnessAfter, 3);
        let aim_after = exit(aim, RawApplyEventKind::WorldWitnessAfter, 4);
        let events = [direction, aim, direction_after, aim_after]
            .iter()
            .enumerate()
            .map(|(index, record)| raw_apply_event_from_native(record, index as u64 + 1).unwrap())
            .collect::<Vec<_>>();
        assert!(raw_apply_pairs_valid(&events));
        let mut duplicate = events.clone();
        duplicate.extend(events.iter().cloned());
        assert!(!raw_apply_pairs_valid(&duplicate));
        let mut missing = events;
        missing.pop();
        assert!(!raw_apply_pairs_valid(&missing));
    }

    #[test]
    fn analysis_is_permanently_ineligible_and_manifest_hash_bound() {
        let records = [
            lifecycle(RawApplyEventKind::NativeInitialized, 1),
            lifecycle(RawApplyEventKind::HooksInstalled, 2),
            lifecycle(RawApplyEventKind::HooksRestored, 3),
            lifecycle(RawApplyEventKind::NativeRetired, 4),
        ];
        let events = records
            .iter()
            .enumerate()
            .map(|(index, record)| raw_apply_event_from_native(record, index as u64 + 1).unwrap())
            .collect::<Vec<_>>();
        let report = analyze_raw_apply_evidence(&manifest(), &events);
        assert_eq!(report.result, "FAIL");
        assert!(!report.eligible_for_active_gate);

        assert!(raw_apply_manifest_valid(&manifest()));

        let mut historical = manifest();
        historical.passive_catalog_sha256 =
            "258549E9B107316FDE02C44580741862476AAB7CCABE57AA1958F06729304F51".to_owned();
        historical.passive_capture_plan_sha256 =
            "8908DABFF44E299185438CBC6092F7A3A6B5E308FB95C75BDFA795F0E0042C93".to_owned();
        assert!(!raw_apply_manifest_valid(&historical));
        assert!(raw_apply_manifest_valid_for_revision(
            &historical,
            &historical.passive_catalog_sha256,
            &historical.passive_capture_plan_sha256,
        ));

        let mut forged = manifest();
        forged.passive_catalog_sha256 = "B".repeat(64);
        assert_eq!(analyze_raw_apply_evidence(&forged, &events).result, "FAIL");
        forged = manifest();
        forged.eligible_for_active_gate = true;
        assert_eq!(analyze_raw_apply_evidence(&forged, &events).result, "FAIL");
        forged = manifest();
        forged.passive_redacted_events = 500;
        assert!(raw_apply_manifest_valid(&forged));
    }

    #[test]
    fn observed_raw_sources_are_owner_required_and_known_extra_tolerant() {
        for scenario in ["keyboard-held-release", "keyboard-edge"] {
            assert!(raw_apply_observed_sources_valid(scenario, 0x1));
            assert!(raw_apply_observed_sources_valid(scenario, 0x3));
            assert!(!raw_apply_observed_sources_valid(scenario, 0x0));
            assert!(!raw_apply_observed_sources_valid(scenario, 0x2));
            assert!(!raw_apply_observed_sources_valid(scenario, 0x5));

            let mut owner_only = manifest();
            owner_only.scenario = scenario.to_owned();
            owner_only.passive_observed_raw_source_mask = "0x1".to_owned();
            assert!(raw_apply_manifest_valid(&owner_only));
            owner_only.passive_observed_raw_source_mask = "0x2".to_owned();
            assert!(!raw_apply_manifest_valid(&owner_only));
        }
        for scenario in ["controller-held-release", "controller-edge"] {
            assert!(raw_apply_observed_sources_valid(scenario, 0x2));
            assert!(raw_apply_observed_sources_valid(scenario, 0x3));
            assert!(!raw_apply_observed_sources_valid(scenario, 0x0));
            assert!(!raw_apply_observed_sources_valid(scenario, 0x1));
            assert!(!raw_apply_observed_sources_valid(scenario, 0x6));

            let mut owner_only = manifest();
            owner_only.scenario = scenario.to_owned();
            owner_only.passive_observed_raw_source_mask = "0x2".to_owned();
            assert!(raw_apply_manifest_valid(&owner_only));
            owner_only.passive_observed_raw_source_mask = "0x1".to_owned();
            assert!(!raw_apply_manifest_valid(&owner_only));
        }
        assert!(!raw_apply_observed_sources_valid("unknown", 0x3));
    }

    #[test]
    fn both_passive_evidence_classes_are_rejected_by_input_gate() {
        assert!(reject_passive_input_gate_evidence(EVIDENCE_PASSIVE_INPUT_RUNTIME).is_err());
        assert!(reject_passive_input_gate_evidence(EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME).is_err());
        assert!(reject_passive_input_gate_evidence("PIPELINE_RUNTIME").is_ok());
        assert!(validate_input_analysis_dispatch(3, EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME).unwrap());
        assert!(!validate_input_analysis_dispatch(2, EVIDENCE_PASSIVE_INPUT_RUNTIME).unwrap());
        assert!(validate_input_analysis_dispatch(2, EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME).is_err());
        assert!(validate_input_analysis_dispatch(3, EVIDENCE_PASSIVE_INPUT_RUNTIME).is_err());
    }
}
