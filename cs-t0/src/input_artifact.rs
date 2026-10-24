use std::collections::{BTreeMap, BTreeSet};
use std::fs::{self, OpenOptions};
use std::io::{BufRead, BufReader, Write};
use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};

use crate::INPUT_TOOL_IDENTITY;
use crate::error::{AppError, AppResult, UsageContext};
use crate::input_ipc::{
    INPUT_FLAG_GAMEPAD_DEVICE, INPUT_FLAG_KEYBOARD_DEVICE, INPUT_FLAG_LOOP_KNOWN,
    INPUT_FLAG_MAIN_THREAD, INPUT_FLAG_ORIGINAL_CALLED, INPUT_FLAG_REDACTED,
    INPUT_FLAG_RESULT_TRUE, INPUT_FLAG_VALUE_VALID, INPUT_FLAG_WRONG_THREAD, INPUT_HOOK_MASK,
    INPUT_IPC_SCHEMA_VERSION, INPUT_KNOWN_FLAGS, INPUT_NO_ACTION, INPUT_NO_CONTROLLER_ID,
    INPUT_NO_DEVICE_KEY, INPUT_NO_PRIMITIVE, InputClassification, InputNativeEventKind,
    NativeInputEventRecord,
};
use crate::loop_ipc::LoopPhase;
use crate::pe::sha256_hex;

pub const INPUT_ARTIFACT_SCHEMA_VERSION: u32 = 2;
pub const EVIDENCE_WITNESS_QUERY_ONLY: &str = "WITNESS_QUERY_ONLY";
pub const EVIDENCE_PIPELINE_RUNTIME: &str = "PIPELINE_RUNTIME";
pub const EVIDENCE_DISARMED_CONTROL: &str = "DISARMED_CONTROL";
pub const EVIDENCE_PASSIVE_INPUT_RUNTIME: &str = "PASSIVE_INPUT_RUNTIME";
pub const WITNESS_LOG_IDENTITY: &str = "IsaacCsT0-InputWitness-v1";
pub const WITNESS_LOG_SOURCE_FORMAT: &str = "NO_IPC_WITNESS_V1";
pub const CONTROL_NO_IPC_SOURCE_FORMAT: &str = "NO_IPC_CONTROL_V1";
pub const INPUT_IPC_SOURCE_FORMAT: &str = "ICSI04V1";
pub const INPUT_STATE_VERSION: u16 = 1;
pub const INPUT_STATE_LENGTH: u16 = 24;
pub const INPUT_COMMAND_LENGTH: u16 = 64;
pub const DEAD_ZONE_Q15: i16 = 4096;
pub const INPUT_LEAD_TICKS: u64 = 2;
pub const MAX_FUTURE_TICKS: u64 = 64;
pub const TRANSPORT_CAPACITY: usize = 256;
pub const INPUT_QUEUE_CAPACITY: usize = 256;
pub const DRAIN_BUDGET: usize = 64;
pub const DEDUPE_HISTORY: usize = 4096;
const REQUIRED_LOOP_GATE_SCENARIOS: &[&str] = &[
    "keyboard",
    "controller",
    "device-switch",
    "pause",
    "loading",
    "room",
    "menu",
];
const REQUIRED_INPUT_GATE_SCENARIOS: &[&str] = &[
    "control-no-native",
    "keyboard",
    "controller",
    "simultaneous",
    "rapid-tap",
    "device-switch",
    "pause",
    "loading",
    "room",
    "menu",
    "queue-full",
    "expired",
    "too-far",
    "stale",
    "conflict",
    "duplicate",
    "original-online-disarmed",
    "loopback",
];

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputConstants {
    pub input_state_version: u16,
    pub input_state_length: u16,
    pub input_command_length: u16,
    pub dead_zone_q15: i16,
    pub input_lead_ticks: u64,
    pub max_future_ticks: u64,
    pub transport_capacity: usize,
    pub input_queue_capacity: usize,
    pub drain_budget: usize,
    pub dedupe_history: usize,
    pub experimental_not_product: bool,
}

impl Default for InputConstants {
    fn default() -> Self {
        Self {
            input_state_version: INPUT_STATE_VERSION,
            input_state_length: INPUT_STATE_LENGTH,
            input_command_length: INPUT_COMMAND_LENGTH,
            dead_zone_q15: DEAD_ZONE_Q15,
            input_lead_ticks: INPUT_LEAD_TICKS,
            max_future_ticks: MAX_FUTURE_TICKS,
            transport_capacity: TRANSPORT_CAPACITY,
            input_queue_capacity: INPUT_QUEUE_CAPACITY,
            drain_budget: DRAIN_BUDGET,
            dedupe_history: DEDUPE_HISTORY,
            experimental_not_product: true,
        }
    }
}

impl InputConstants {
    pub fn validate(&self) -> AppResult<()> {
        if self.input_state_version != INPUT_STATE_VERSION
            || self.input_state_length != INPUT_STATE_LENGTH
            || self.input_command_length != INPUT_COMMAND_LENGTH
            || self.dead_zone_q15 != DEAD_ZONE_Q15
            || self.input_lead_ticks != INPUT_LEAD_TICKS
            || self.max_future_ticks != MAX_FUTURE_TICKS
            || self.transport_capacity != TRANSPORT_CAPACITY
            || self.input_queue_capacity != INPUT_QUEUE_CAPACITY
            || self.drain_budget != DRAIN_BUDGET
            || self.dedupe_history != DEDUPE_HISTORY
            || !self.experimental_not_product
        {
            return Err(AppError::evidence(
                "input manifest constants do not match frozen T0-04 experimental constants",
            ));
        }
        Ok(())
    }
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputManifest {
    pub schema_version: u32,
    pub experiment_identity: String,
    pub case_id: String,
    pub probe: String,
    pub scenario: String,
    pub evidence_class: String,
    pub endpoint: String,
    pub target_sha256: String,
    pub loop_gate_sha256: String,
    pub artifact_identity: String,
    pub ipc_magic: String,
    pub arm_token_sha256: Option<String>,
    pub constants: InputConstants,
    pub requirements: InputEvidenceRequirements,
    #[serde(default)]
    pub source_log_sha256: Option<String>,
    #[serde(default)]
    pub source_window_min: Option<u64>,
    #[serde(default)]
    pub source_window_max: Option<u64>,
    #[serde(default)]
    pub source_controller_index: Option<i32>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub ipc_schema_version: Option<u32>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub companion_loop_artifact_path: Option<PathBuf>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub companion_loop_manifest_sha256: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub companion_loop_events_sha256: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub companion_loop_analysis_sha256: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub native_sha256: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub helper_sha256: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub main_thread_id: Option<u32>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub installed_hooks_mask: Option<u64>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub restored_hooks_mask: Option<u64>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub overwritten_events: Option<u64>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub redacted_events: Option<u64>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub producer_closed: Option<bool>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub target_bytes_before_sha256: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub target_bytes_after_sha256: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub target_bytes_restored: Option<bool>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub failure_reason: Option<String>,
    pub state: String,
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct WitnessImportFilter {
    pub window_min: Option<u64>,
    pub window_max: Option<u64>,
    pub controller_index: Option<i32>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputEvidenceRequirements {
    pub minimum_events: usize,
    pub minimum_windows: usize,
    pub minimum_observations_per_required_action: u64,
    pub required_phases: Vec<String>,
    pub required_stages: Vec<String>,
    pub required_actions: Vec<u32>,
    pub controller_scope: String,
    pub forbid_pipeline_events: bool,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputEvent {
    pub event_sequence: u64,
    pub stage: String,
    pub loop_iteration: u64,
    pub phase: String,
    pub authority_tick: u64,
    #[serde(default)]
    pub session_id: Option<String>,
    #[serde(default)]
    pub client_id: Option<u64>,
    #[serde(default)]
    pub command_sequence: Option<u64>,
    #[serde(default)]
    pub target_tick: Option<u64>,
    #[serde(default)]
    pub canonical_state_hex: Option<String>,
    #[serde(default)]
    pub decision: Option<String>,
    #[serde(default)]
    pub edge_flags: Option<u32>,
    #[serde(default)]
    pub world_witness_before: Option<String>,
    #[serde(default)]
    pub world_witness_after: Option<String>,
    #[serde(default)]
    pub action: Option<u32>,
    #[serde(default)]
    pub input_hook: Option<String>,
    #[serde(default)]
    pub controller_index: Option<i32>,
    #[serde(default)]
    pub observation_count: Option<u64>,
    #[serde(default)]
    pub render_end: Option<u64>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub native_sequence: Option<u64>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub qpc: Option<i64>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub thread_id: Option<u32>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub native_kind: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub flags: Option<u32>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub return_rva: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub original_calls: Option<u32>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub error_code: Option<u32>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub device_key: Option<u32>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub selected_device_key: Option<u32>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub controller_id_raw: Option<u32>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub primitive: Option<u32>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub value_bits: Option<u32>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub classification: Option<String>,
}

#[derive(Clone, Debug, Eq, Ord, PartialEq, PartialOrd)]
struct Identity {
    session_id: String,
    client_id: u64,
    sequence: u64,
}

#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputCheck {
    pub id: String,
    pub passed: bool,
    pub detail: String,
}

#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputAnalysisReport {
    pub schema_version: u32,
    pub result: String,
    pub case_id: String,
    pub probe: String,
    pub scenario: String,
    pub evidence_class: String,
    pub eligible_for_active_gate: bool,
    pub artifact_identity: String,
    pub event_count: usize,
    pub accepted_count: usize,
    pub rejected_count: usize,
    pub apply_count: usize,
    pub canonical_trace_sha256: String,
    pub decision_trace_sha256: String,
    pub apply_trace_sha256: String,
    pub checks: Vec<InputCheck>,
}

#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputComparisonReport {
    pub schema_version: u32,
    pub result: String,
    pub evidence_class: String,
    pub eligible_for_active_gate: bool,
    pub first_artifact_identity: String,
    pub second_artifact_identity: String,
    pub checks: Vec<InputCheck>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct WitnessImportReport {
    pub schema_version: u32,
    pub result: String,
    pub evidence_class: String,
    pub scenario: String,
    pub artifact_identity: String,
    pub source_log_sha256: String,
    pub event_count: usize,
    pub output_directory: PathBuf,
}

pub const PASSIVE_INPUT_TARGET_BYTES_SCHEMA_VERSION: u32 = 1;
pub const PASSIVE_INPUT_TARGET_SLOTS: &[(&str, u32)] = &[
    ("manager-pressed", 0x0078_2990),
    ("manager-triggered", 0x0078_2994),
    ("manager-scalar", 0x0078_2998),
    ("keyboard-logical-pressed", 0x007A_16A8),
    ("keyboard-logical-triggered", 0x007A_16AC),
    ("keyboard-logical-scalar", 0x007A_16B0),
    ("gamepad-logical-pressed", 0x0078_2CB0),
    ("gamepad-logical-triggered", 0x0078_2CB4),
    ("gamepad-logical-scalar", 0x0078_2CB8),
    ("keyboard-logical-setter", 0x007A_16C4),
    ("gamepad-logical-setter", 0x0078_2CCC),
];

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct PassiveInputTargetSlot {
    pub id: String,
    pub rva: String,
    pub bytes_hex: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct PassiveInputTargetBytesDocument {
    pub schema_version: u32,
    pub target_sha256: String,
    pub slots: Vec<PassiveInputTargetSlot>,
}

#[derive(Clone, Debug)]
pub struct PassiveInputRunConfig {
    pub case_id: String,
    pub scenario: String,
    pub target_sha256: String,
    pub loop_gate_sha256: String,
    pub artifact_identity: String,
    pub companion_loop_artifact_path: PathBuf,
    pub native_sha256: String,
    pub helper_sha256: String,
    pub target_bytes_before: PassiveInputTargetBytesDocument,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct PassiveInputFinish {
    pub main_thread_id: u32,
    pub installed_hooks_mask: u64,
    pub restored_hooks_mask: u64,
    pub overwritten_events: u64,
    pub redacted_events: u64,
    pub producer_closed: bool,
}

pub struct PassiveInputRun {
    directory: PathBuf,
    manifest: InputManifest,
    events_file: fs::File,
    next_event_sequence: u64,
    target_bytes_before: PassiveInputTargetBytesDocument,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputGateDecision {
    pub schema_version: u32,
    pub status: String,
    pub t0_04_unlocked: bool,
    pub loop_gate_sha256: String,
    pub matrix_sha256: String,
    pub repeatability_sha256: String,
    pub required_cases: Vec<String>,
    pub missing_cases: Vec<String>,
    pub analysis_bindings: Vec<InputAnalysisBinding>,
    pub repeatability_first_artifact_identity: String,
    pub repeatability_second_artifact_identity: String,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct ValidatedLoopGateReference {
    pub sha256: String,
    pub target_sha256: String,
    pub loop_ledger_sha256: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputAnalysisBinding {
    pub case_name: String,
    pub artifact_identity: String,
    pub analysis_path: PathBuf,
    pub analysis_sha256: String,
    pub manifest_sha256: String,
    pub events_sha256: String,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
struct InputMatrix {
    analyses: Vec<PathBuf>,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
struct LoopGateReference {
    schema_version: u32,
    status: String,
    active_input_allowed: bool,
    required_scenarios: Vec<String>,
    missing_scenarios: Vec<String>,
    analysis_bindings: Vec<crate::loop_verification::LoopAnalysisBinding>,
}

pub fn requirements_for(
    probe: &str,
    scenario: &str,
    evidence_class: &str,
) -> AppResult<InputEvidenceRequirements> {
    let all_actions = (0..=11).collect::<Vec<_>>();
    let edge_actions = (8..=11).collect::<Vec<_>>();
    match (probe, evidence_class) {
        ("passive", EVIDENCE_WITNESS_QUERY_ONLY) => {
            let (minimum_windows, minimum_per_action, actions, controller_scope) = match scenario {
                "keyboard" => (1, 1, all_actions, "ANY"),
                "controller" => (1, 1, all_actions, "NONNEGATIVE"),
                "rapid-tap" => (2, 2, edge_actions, "ANY"),
                "pause" | "room" | "focus" | "loading" => (2, 1, all_actions, "ANY"),
                "menu" => (2, 1, all_actions, "ENTITYLESS"),
                "simultaneous" | "device-switch" => (2, 1, all_actions, "NONNEGATIVE"),
                _ => {
                    return Err(AppError::usage(format!(
                        "unsupported passive input scenario {scenario}"
                    )));
                }
            };
            Ok(InputEvidenceRequirements {
                minimum_events: actions.len() + 1,
                minimum_windows,
                minimum_observations_per_required_action: minimum_per_action,
                required_phases: vec!["WITNESS".to_owned()],
                required_stages: vec!["witness-contract".to_owned(), "witness-query".to_owned()],
                required_actions: actions,
                controller_scope: controller_scope.to_owned(),
                forbid_pipeline_events: true,
            })
        }
        ("passive", EVIDENCE_PASSIVE_INPUT_RUNTIME) => {
            let supported = [
                "keyboard",
                "controller",
                "simultaneous",
                "rapid-tap",
                "device-switch",
                "pause",
                "focus",
                "loading",
                "room",
                "menu",
            ];
            if !supported.contains(&scenario) {
                return Err(AppError::usage(format!(
                    "unsupported passive runtime input scenario {scenario}"
                )));
            }
            Ok(InputEvidenceRequirements {
                minimum_events: 4,
                minimum_windows: 1,
                minimum_observations_per_required_action: 1,
                required_phases: vec!["OUTSIDE_ENVELOPE".to_owned()],
                required_stages: vec!["native-lifecycle".to_owned()],
                required_actions: Vec::new(),
                controller_scope: "NATIVE_RUNTIME".to_owned(),
                forbid_pipeline_events: true,
            })
        }
        ("control-no-native", EVIDENCE_DISARMED_CONTROL) if scenario == "control-no-native" => {
            Ok(disarmed_requirements())
        }
        ("original-online-disarmed", EVIDENCE_DISARMED_CONTROL)
            if scenario == "original-online-disarmed" =>
        {
            Ok(disarmed_requirements())
        }
        ("loopback", EVIDENCE_PIPELINE_RUNTIME) => {
            let supported = [
                "keyboard",
                "controller",
                "simultaneous",
                "rapid-tap",
                "device-switch",
                "pause",
                "focus",
                "loading",
                "room",
                "menu",
                "queue-full",
                "expired",
                "too-far",
                "stale",
                "conflict",
                "duplicate",
                "loopback",
            ];
            if !supported.contains(&scenario) {
                return Err(AppError::usage(format!(
                    "unsupported loopback input scenario {scenario}"
                )));
            }
            let required_actions = match scenario {
                "keyboard" | "controller" | "simultaneous" | "device-switch" | "pause"
                | "focus" | "loading" | "room" | "menu" => all_actions,
                "rapid-tap" => edge_actions,
                _ => Vec::new(),
            };
            Ok(InputEvidenceRequirements {
                minimum_events: 8 + required_actions.len(),
                minimum_windows: 1,
                minimum_observations_per_required_action: 1,
                required_phases: vec![
                    "LOCAL_PLANNER".to_owned(),
                    "TRANSPORT".to_owned(),
                    "DRAIN_BEFORE_RESOLVE".to_owned(),
                    "RESOLVE".to_owned(),
                    "APPLY".to_owned(),
                    "WITNESS".to_owned(),
                ],
                required_stages: vec![
                    "sample".to_owned(),
                    "canonical".to_owned(),
                    "send".to_owned(),
                    "receive".to_owned(),
                    "accept".to_owned(),
                    "resolve".to_owned(),
                    "apply".to_owned(),
                    "witness".to_owned(),
                ],
                required_actions,
                controller_scope: if scenario == "controller" {
                    "NONNEGATIVE".to_owned()
                } else {
                    "ANY".to_owned()
                },
                forbid_pipeline_events: false,
            })
        }
        _ => Err(AppError::usage(
            "input probe, scenario, and evidence class are inconsistent",
        )),
    }
}

fn disarmed_requirements() -> InputEvidenceRequirements {
    InputEvidenceRequirements {
        minimum_events: 1,
        minimum_windows: 0,
        minimum_observations_per_required_action: 0,
        required_phases: vec!["OUTSIDE_AUTHORITY".to_owned()],
        required_stages: vec!["control-observation".to_owned()],
        required_actions: Vec::new(),
        controller_scope: "NONE".to_owned(),
        forbid_pipeline_events: true,
    }
}

impl PassiveInputTargetBytesDocument {
    fn validate(&self, expected_target_sha256: &str) -> AppResult<()> {
        if self.schema_version != PASSIVE_INPUT_TARGET_BYTES_SCHEMA_VERSION
            || self.target_sha256 != expected_target_sha256
            || self.slots.len() != PASSIVE_INPUT_TARGET_SLOTS.len()
        {
            return Err(AppError::evidence(
                "passive input target-byte document has invalid identity",
            ));
        }
        for (slot, (expected_id, expected_rva)) in self.slots.iter().zip(PASSIVE_INPUT_TARGET_SLOTS)
        {
            let decoded = hex::decode(&slot.bytes_hex);
            if slot.id != *expected_id
                || !slot.rva.starts_with("0x")
                || u32::from_str_radix(&slot.rva[2..], 16).ok() != Some(*expected_rva)
                || decoded.as_ref().map(Vec::len) != Ok(4)
            {
                return Err(AppError::evidence(
                    "passive input target-byte document does not bind the exact ordered 11-slot/4-byte target set",
                ));
            }
        }
        Ok(())
    }
}

impl PassiveInputRun {
    pub fn create(output_directory: &Path, config: PassiveInputRunConfig) -> AppResult<Self> {
        validate_case_id(&config.case_id)?;
        validate_sha256(&config.target_sha256, "target SHA-256")?;
        validate_sha256(&config.loop_gate_sha256, "loop gate SHA-256")?;
        validate_sha256(&config.native_sha256, "native SHA-256")?;
        validate_sha256(&config.helper_sha256, "helper SHA-256")?;
        config.target_bytes_before.validate(&config.target_sha256)?;
        validate_relative_matrix_path(
            &config.companion_loop_artifact_path,
            "companion loop artifact",
        )?;
        if config.artifact_identity.trim().is_empty() {
            return Err(AppError::usage(
                "passive input artifact identity cannot be empty",
            ));
        }
        let requirements =
            requirements_for("passive", &config.scenario, EVIDENCE_PASSIVE_INPUT_RUNTIME)?;
        fs::create_dir(output_directory).usage_context(format!(
            "refusing to reuse passive input artifact {}",
            output_directory.display()
        ))?;
        let before_path = output_directory.join("target-bytes-before.json");
        write_new_json(&before_path, &config.target_bytes_before)?;
        let before_sha256 = sha256_hex(
            &fs::read(&before_path).usage_context("cannot read passive target bytes before")?,
        );
        let events_file = OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(output_directory.join("events.jsonl"))
            .usage_context("cannot create passive input events")?;
        let manifest = InputManifest {
            schema_version: INPUT_ARTIFACT_SCHEMA_VERSION,
            experiment_identity: INPUT_TOOL_IDENTITY.to_owned(),
            case_id: config.case_id,
            probe: "passive".to_owned(),
            scenario: config.scenario,
            evidence_class: EVIDENCE_PASSIVE_INPUT_RUNTIME.to_owned(),
            endpoint: "local".to_owned(),
            target_sha256: config.target_sha256,
            loop_gate_sha256: config.loop_gate_sha256,
            artifact_identity: config.artifact_identity,
            ipc_magic: INPUT_IPC_SOURCE_FORMAT.to_owned(),
            arm_token_sha256: None,
            constants: InputConstants::default(),
            requirements,
            source_log_sha256: None,
            source_window_min: None,
            source_window_max: None,
            source_controller_index: None,
            ipc_schema_version: Some(INPUT_IPC_SCHEMA_VERSION),
            companion_loop_artifact_path: Some(config.companion_loop_artifact_path),
            companion_loop_manifest_sha256: None,
            companion_loop_events_sha256: None,
            companion_loop_analysis_sha256: None,
            native_sha256: Some(config.native_sha256),
            helper_sha256: Some(config.helper_sha256),
            main_thread_id: None,
            installed_hooks_mask: None,
            restored_hooks_mask: None,
            overwritten_events: None,
            redacted_events: None,
            producer_closed: None,
            target_bytes_before_sha256: Some(before_sha256),
            target_bytes_after_sha256: None,
            target_bytes_restored: None,
            failure_reason: None,
            state: "RUNNING".to_owned(),
        };
        write_new_json(&output_directory.join("manifest.json"), &manifest)?;
        Ok(Self {
            directory: output_directory.to_owned(),
            manifest,
            events_file,
            next_event_sequence: 1,
            target_bytes_before: config.target_bytes_before,
        })
    }

    pub fn append_native(&mut self, record: &NativeInputEventRecord) -> AppResult<()> {
        let event = input_event_from_native(record, self.next_event_sequence)?;
        serde_json::to_writer(&mut self.events_file, &event)
            .usage_context("cannot serialize passive native input event")?;
        self.events_file
            .write_all(b"\n")
            .usage_context("cannot append passive native input event")?;
        self.next_event_sequence = self
            .next_event_sequence
            .checked_add(1)
            .ok_or_else(|| AppError::evidence("passive input event sequence overflow"))?;
        Ok(())
    }

    pub fn finish(
        mut self,
        target_bytes_after: &PassiveInputTargetBytesDocument,
        summary: PassiveInputFinish,
    ) -> AppResult<InputAnalysisReport> {
        target_bytes_after.validate(&self.manifest.target_sha256)?;
        self.events_file
            .flush()
            .usage_context("cannot flush passive input events")?;
        let after_path = self.directory.join("target-bytes-after.json");
        write_new_json(&after_path, target_bytes_after)?;
        self.bind_finish(target_bytes_after, summary, None)?;
        analyze_input_run(&self.directory)
    }

    pub fn fail(
        mut self,
        reason: &str,
        target_bytes_after: Option<&PassiveInputTargetBytesDocument>,
        summary: PassiveInputFinish,
    ) -> AppResult<()> {
        if reason.trim().is_empty() {
            return Err(AppError::usage(
                "passive input failure requires a nonempty reason",
            ));
        }
        self.events_file
            .flush()
            .usage_context("cannot flush failed passive input events")?;
        if let Some(document) = target_bytes_after {
            document.validate(&self.manifest.target_sha256)?;
            write_new_json(&self.directory.join("target-bytes-after.json"), document)?;
            self.bind_finish(document, summary, Some(reason))?;
        } else {
            self.manifest.main_thread_id = Some(summary.main_thread_id);
            self.manifest.installed_hooks_mask = Some(summary.installed_hooks_mask);
            self.manifest.restored_hooks_mask = Some(summary.restored_hooks_mask);
            self.manifest.overwritten_events = Some(summary.overwritten_events);
            self.manifest.redacted_events = Some(summary.redacted_events);
            self.manifest.producer_closed = Some(summary.producer_closed);
            self.manifest.failure_reason = Some(reason.to_owned());
            self.manifest.state = "FAILED".to_owned();
            replace_json(&self.directory.join("manifest.json"), &self.manifest)?;
        }
        Ok(())
    }

    fn bind_finish(
        &mut self,
        target_bytes_after: &PassiveInputTargetBytesDocument,
        summary: PassiveInputFinish,
        failure_reason: Option<&str>,
    ) -> AppResult<()> {
        let companion_hashes = companion_loop_hashes(
            &self.directory,
            self.manifest
                .companion_loop_artifact_path
                .as_deref()
                .ok_or_else(|| AppError::evidence("missing companion loop artifact path"))?,
            &self.manifest.target_sha256,
        );
        match companion_hashes {
            Ok((loop_manifest, loop_events, loop_analysis)) => {
                self.manifest.companion_loop_manifest_sha256 = Some(loop_manifest);
                self.manifest.companion_loop_events_sha256 = Some(loop_events);
                self.manifest.companion_loop_analysis_sha256 = Some(loop_analysis);
            }
            Err(error) if failure_reason.is_none() => return Err(error),
            Err(_) => {}
        }
        self.manifest.main_thread_id = Some(summary.main_thread_id);
        self.manifest.installed_hooks_mask = Some(summary.installed_hooks_mask);
        self.manifest.restored_hooks_mask = Some(summary.restored_hooks_mask);
        self.manifest.overwritten_events = Some(summary.overwritten_events);
        self.manifest.redacted_events = Some(summary.redacted_events);
        self.manifest.producer_closed = Some(summary.producer_closed);
        let after_bytes = fs::read(self.directory.join("target-bytes-after.json"))
            .usage_context("cannot read passive target bytes after")?;
        self.manifest.target_bytes_after_sha256 = Some(sha256_hex(&after_bytes));
        self.manifest.target_bytes_restored = Some(&self.target_bytes_before == target_bytes_after);
        self.manifest.failure_reason = failure_reason.map(str::to_owned);
        self.manifest.state = if failure_reason.is_some() {
            "FAILED"
        } else {
            "COMPLETE"
        }
        .to_owned();
        replace_json(&self.directory.join("manifest.json"), &self.manifest)
    }
}

pub fn input_event_from_native(
    record: &NativeInputEventRecord,
    event_sequence: u64,
) -> AppResult<InputEvent> {
    let raw = record.event;
    if event_sequence == 0
        || record.sequence == 0
        || raw.published_sequence != record.sequence as i64
        || raw.qpc <= 0
        || raw.thread_id == 0
        || raw.flags & !INPUT_KNOWN_FLAGS != 0
        || raw.flags & (INPUT_FLAG_WRONG_THREAD | INPUT_FLAG_REDACTED) != 0
    {
        return Err(AppError::evidence(
            "native passive input record violates sequence, flag, or publication scope",
        ));
    }
    let kind = InputNativeEventKind::from_raw(raw.kind)?;
    let phase = LoopPhase::from_raw(raw.phase)?;
    let classification = InputClassification::from_raw(raw.classification)?;
    let lifecycle = matches!(
        kind,
        InputNativeEventKind::NativeInitialized
            | InputNativeEventKind::HooksInstalled
            | InputNativeEventKind::HooksRestored
            | InputNativeEventKind::NativeRetired
            | InputNativeEventKind::Failure
    );
    let logical = matches!(
        kind,
        InputNativeEventKind::LogicalPressedEnter
            | InputNativeEventKind::LogicalPressedExit
            | InputNativeEventKind::LogicalTriggeredEnter
            | InputNativeEventKind::LogicalTriggeredExit
            | InputNativeEventKind::LogicalScalarEnter
            | InputNativeEventKind::LogicalScalarExit
    );
    let manager = matches!(
        kind,
        InputNativeEventKind::ManagerPressedEnter
            | InputNativeEventKind::ManagerPressedExit
            | InputNativeEventKind::ManagerTriggeredEnter
            | InputNativeEventKind::ManagerTriggeredExit
            | InputNativeEventKind::ManagerScalarEnter
            | InputNativeEventKind::ManagerScalarExit
    );
    let setter = matches!(
        kind,
        InputNativeEventKind::LogicalSetterEnter | InputNativeEventKind::LogicalSetterExit
    );
    if lifecycle {
        if phase != LoopPhase::OutsideEnvelope
            || raw.loop_iteration != 0
            || raw.flags != 0
            || raw.return_rva != 0
            || raw.original_calls != 0
            || raw.action != INPUT_NO_ACTION
            || raw.primitive != INPUT_NO_PRIMITIVE
            || raw.device_key != INPUT_NO_DEVICE_KEY
            || raw.selected_device_key != INPUT_NO_DEVICE_KEY
            || raw.controller_id != INPUT_NO_CONTROLLER_ID
            || classification != InputClassification::Unknown
            || if kind == InputNativeEventKind::Failure {
                raw.error_code == 0
            } else {
                raw.error_code != 0
            }
        {
            return Err(AppError::evidence(
                "native lifecycle event contains gameplay input payload",
            ));
        }
    } else {
        let name = native_kind_name(kind);
        let enter = name.ends_with("_ENTER");
        let exit = name.ends_with("_EXIT");
        let value_valid = raw.flags & INPUT_FLAG_VALUE_VALID != 0;
        let result_true = raw.flags & INPUT_FLAG_RESULT_TRUE != 0;
        let value_contract_valid = match kind {
            InputNativeEventKind::LogicalPressedEnter
            | InputNativeEventKind::LogicalTriggeredEnter
            | InputNativeEventKind::LogicalScalarEnter
            | InputNativeEventKind::ManagerPressedEnter
            | InputNativeEventKind::ManagerTriggeredEnter
            | InputNativeEventKind::ManagerScalarEnter => !value_valid && !result_true,
            InputNativeEventKind::LogicalPressedExit
            | InputNativeEventKind::LogicalTriggeredExit
            | InputNativeEventKind::ManagerPressedExit
            | InputNativeEventKind::ManagerTriggeredExit => {
                value_valid && raw.value_bits <= 1 && result_true == (raw.value_bits == 1)
            }
            InputNativeEventKind::LogicalScalarExit | InputNativeEventKind::ManagerScalarExit => {
                value_valid && result_true == (f32::from_bits(raw.value_bits) != 0.0)
            }
            InputNativeEventKind::LogicalSetterEnter | InputNativeEventKind::LogicalSetterExit => {
                value_valid && !result_true
            }
            _ => false,
        };
        if raw.error_code != 0
            || !enter && !exit
            || enter && (raw.original_calls != 0 || raw.flags & INPUT_FLAG_ORIGINAL_CALLED != 0)
            || exit && (raw.original_calls != 1 || raw.flags & INPUT_FLAG_ORIGINAL_CALLED == 0)
            || !value_contract_valid
            || raw.action > 11
            || raw.loop_iteration == 0
            || raw.flags & (INPUT_FLAG_LOOP_KNOWN | INPUT_FLAG_MAIN_THREAD)
                != INPUT_FLAG_LOOP_KNOWN | INPUT_FLAG_MAIN_THREAD
            || phase == LoopPhase::OutsideEnvelope
            || raw.return_rva == 0
            || !((raw.flags & INPUT_FLAG_KEYBOARD_DEVICE != 0)
                ^ (raw.flags & INPUT_FLAG_GAMEPAD_DEVICE != 0))
        {
            return Err(AppError::evidence(
                "native passive query is outside the loop/main-thread/action scope",
            ));
        }
        let scope_valid = if logical {
            classification == InputClassification::ExplicitGameplay
                && raw.device_key != INPUT_NO_DEVICE_KEY
                && raw.primitive == INPUT_NO_PRIMITIVE
        } else if manager {
            classification == InputClassification::UniqueGameplay
                && raw.device_key != INPUT_NO_DEVICE_KEY
                && raw.primitive != INPUT_NO_PRIMITIVE
        } else if setter {
            classification == InputClassification::LogicalOverrideDiagnostic
                && raw.device_key != INPUT_NO_DEVICE_KEY
                && raw.primitive == INPUT_NO_PRIMITIVE
        } else {
            false
        };
        if !scope_valid {
            return Err(AppError::evidence(
                "native passive input classification or ownership is not persistable",
            ));
        }
    }
    let stage = if lifecycle {
        "native-lifecycle"
    } else if logical {
        "passive-logical-query"
    } else if manager {
        "passive-manager-query"
    } else {
        "passive-logical-setter"
    };
    Ok(InputEvent {
        event_sequence,
        stage: stage.to_owned(),
        loop_iteration: raw.loop_iteration,
        phase: loop_phase_name(phase).to_owned(),
        authority_tick: 0,
        session_id: None,
        client_id: None,
        command_sequence: None,
        target_tick: None,
        canonical_state_hex: None,
        decision: None,
        edge_flags: None,
        world_witness_before: None,
        world_witness_after: None,
        action: (!lifecycle).then_some(raw.action),
        input_hook: (!lifecycle).then(|| native_kind_name(kind).to_owned()),
        controller_index: None,
        observation_count: (!lifecycle).then_some(1),
        render_end: None,
        native_sequence: Some(record.sequence),
        qpc: Some(raw.qpc),
        thread_id: Some(raw.thread_id),
        native_kind: Some(native_kind_name(kind).to_owned()),
        flags: Some(raw.flags),
        return_rva: (raw.return_rva != 0).then(|| format!("0x{:08X}", raw.return_rva)),
        original_calls: Some(raw.original_calls),
        error_code: Some(raw.error_code),
        device_key: (raw.device_key != INPUT_NO_DEVICE_KEY).then_some(raw.device_key),
        selected_device_key: (raw.selected_device_key != INPUT_NO_DEVICE_KEY)
            .then_some(raw.selected_device_key),
        controller_id_raw: (!lifecycle).then_some(raw.controller_id),
        primitive: (raw.primitive != INPUT_NO_PRIMITIVE).then_some(raw.primitive),
        value_bits: (raw.flags & INPUT_FLAG_VALUE_VALID != 0).then_some(raw.value_bits),
        classification: (!lifecycle).then(|| classification_name(classification).to_owned()),
    })
}

fn loop_phase_name(phase: LoopPhase) -> &'static str {
    match phase {
        LoopPhase::OutsideEnvelope => "OUTSIDE_ENVELOPE",
        LoopPhase::LoopBoundary => "LOOP_BOUNDARY",
        LoopPhase::MessagePump => "MESSAGE_PUMP",
        LoopPhase::InputQuery => "INPUT_QUERY",
        LoopPhase::GameplayUpdate => "GAMEPLAY_UPDATE",
        LoopPhase::EntityMutation => "ENTITY_MUTATION",
        LoopPhase::PostUpdate => "POST_UPDATE",
        LoopPhase::Render => "RENDER",
        LoopPhase::RoomLifecycle => "ROOM_LIFECYCLE",
        LoopPhase::RunLifecycle => "RUN_LIFECYCLE",
    }
}

fn native_kind_name(kind: InputNativeEventKind) -> &'static str {
    match kind {
        InputNativeEventKind::NativeInitialized => "NATIVE_INITIALIZED",
        InputNativeEventKind::HooksInstalled => "HOOKS_INSTALLED",
        InputNativeEventKind::LogicalPressedEnter => "LOGICAL_PRESSED_ENTER",
        InputNativeEventKind::LogicalPressedExit => "LOGICAL_PRESSED_EXIT",
        InputNativeEventKind::LogicalTriggeredEnter => "LOGICAL_TRIGGERED_ENTER",
        InputNativeEventKind::LogicalTriggeredExit => "LOGICAL_TRIGGERED_EXIT",
        InputNativeEventKind::LogicalScalarEnter => "LOGICAL_SCALAR_ENTER",
        InputNativeEventKind::LogicalScalarExit => "LOGICAL_SCALAR_EXIT",
        InputNativeEventKind::ManagerPressedEnter => "MANAGER_PRESSED_ENTER",
        InputNativeEventKind::ManagerPressedExit => "MANAGER_PRESSED_EXIT",
        InputNativeEventKind::ManagerTriggeredEnter => "MANAGER_TRIGGERED_ENTER",
        InputNativeEventKind::ManagerTriggeredExit => "MANAGER_TRIGGERED_EXIT",
        InputNativeEventKind::ManagerScalarEnter => "MANAGER_SCALAR_ENTER",
        InputNativeEventKind::ManagerScalarExit => "MANAGER_SCALAR_EXIT",
        InputNativeEventKind::LogicalSetterEnter => "LOGICAL_SETTER_ENTER",
        InputNativeEventKind::LogicalSetterExit => "LOGICAL_SETTER_EXIT",
        InputNativeEventKind::HooksRestored => "HOOKS_RESTORED",
        InputNativeEventKind::NativeRetired => "NATIVE_RETIRED",
        InputNativeEventKind::Failure => "FAILURE",
    }
}

fn classification_name(classification: InputClassification) -> &'static str {
    match classification {
        InputClassification::Unknown => "UNKNOWN",
        InputClassification::UniqueGameplay => "UNIQUE_GAMEPLAY",
        InputClassification::NoMatch => "NO_MATCH",
        InputClassification::AmbiguousOrOutsideGameplay => "AMBIGUOUS_OR_OUTSIDE_GAMEPLAY",
        InputClassification::InvalidLayout => "INVALID_LAYOUT",
        InputClassification::UnstableSnapshot => "UNSTABLE_SNAPSHOT",
        InputClassification::ExplicitGameplay => "EXPLICIT_GAMEPLAY",
        InputClassification::LogicalOverrideDiagnostic => "LOGICAL_OVERRIDE_DIAGNOSTIC",
    }
}

#[allow(clippy::too_many_arguments)]
pub fn import_witness_log(
    witness_log: &Path,
    output_directory: &Path,
    scenario: &str,
    case_id: &str,
    target_sha256: &str,
    loop_gate_sha256: &str,
) -> AppResult<WitnessImportReport> {
    import_witness_log_filtered(
        witness_log,
        output_directory,
        scenario,
        case_id,
        target_sha256,
        loop_gate_sha256,
        WitnessImportFilter::default(),
    )
}

#[allow(clippy::too_many_arguments)]
pub fn import_witness_log_filtered(
    witness_log: &Path,
    output_directory: &Path,
    scenario: &str,
    case_id: &str,
    target_sha256: &str,
    loop_gate_sha256: &str,
    filter: WitnessImportFilter,
) -> AppResult<WitnessImportReport> {
    validate_case_id(case_id)?;
    validate_sha256(target_sha256, "target SHA-256")?;
    validate_sha256(loop_gate_sha256, "loop gate SHA-256")?;
    if filter.window_min == Some(0)
        || filter.window_max == Some(0)
        || matches!((filter.window_min, filter.window_max), (Some(minimum), Some(maximum)) if minimum > maximum)
    {
        return Err(AppError::usage(
            "witness window filter must be positive and ordered",
        ));
    }
    let requirements = requirements_for("passive", scenario, EVIDENCE_WITNESS_QUERY_ONLY)?;
    let raw = fs::read(witness_log)
        .usage_context(format!("cannot read witness log {}", witness_log.display()))?;
    let text = std::str::from_utf8(&raw).map_err(|_| {
        AppError::evidence("witness log is not UTF-8 and cannot be parsed without loss")
    })?;
    let events = parse_witness_log_filtered(text, filter)?;
    let source_log_sha256 = sha256_hex(&raw);
    let artifact_identity = format!("witness-query-{case_id}-{}", &source_log_sha256[..16]);
    let manifest = InputManifest {
        schema_version: INPUT_ARTIFACT_SCHEMA_VERSION,
        experiment_identity: INPUT_TOOL_IDENTITY.to_owned(),
        case_id: case_id.to_owned(),
        probe: "passive".to_owned(),
        scenario: scenario.to_owned(),
        evidence_class: EVIDENCE_WITNESS_QUERY_ONLY.to_owned(),
        endpoint: "local".to_owned(),
        target_sha256: target_sha256.to_ascii_uppercase(),
        loop_gate_sha256: loop_gate_sha256.to_ascii_uppercase(),
        artifact_identity: artifact_identity.clone(),
        ipc_magic: WITNESS_LOG_SOURCE_FORMAT.to_owned(),
        arm_token_sha256: None,
        constants: InputConstants::default(),
        requirements,
        source_log_sha256: Some(source_log_sha256.clone()),
        source_window_min: filter.window_min,
        source_window_max: filter.window_max,
        source_controller_index: filter.controller_index,
        ipc_schema_version: None,
        companion_loop_artifact_path: None,
        companion_loop_manifest_sha256: None,
        companion_loop_events_sha256: None,
        companion_loop_analysis_sha256: None,
        native_sha256: None,
        helper_sha256: None,
        main_thread_id: None,
        installed_hooks_mask: None,
        restored_hooks_mask: None,
        overwritten_events: None,
        redacted_events: None,
        producer_closed: None,
        target_bytes_before_sha256: None,
        target_bytes_after_sha256: None,
        target_bytes_restored: None,
        failure_reason: None,
        state: "COMPLETE".to_owned(),
    };
    validate_manifest(&manifest)?;
    if output_directory.as_os_str().is_empty() {
        return Err(AppError::usage("witness output directory cannot be empty"));
    }
    fs::create_dir(output_directory).usage_context(format!(
        "witness output must be a new directory: {}",
        output_directory.display()
    ))?;
    write_new_json(&output_directory.join("manifest.json"), &manifest)?;
    write_events(&output_directory.join("events.jsonl"), &events)?;
    Ok(WitnessImportReport {
        schema_version: INPUT_ARTIFACT_SCHEMA_VERSION,
        result: "IMPORTED_WITNESS_QUERY_ONLY".to_owned(),
        evidence_class: EVIDENCE_WITNESS_QUERY_ONLY.to_owned(),
        scenario: scenario.to_owned(),
        artifact_identity,
        source_log_sha256,
        event_count: events.len(),
        output_directory: output_directory.to_path_buf(),
    })
}

#[cfg(test)]
fn parse_witness_log(text: &str) -> AppResult<Vec<InputEvent>> {
    parse_witness_log_filtered(text, WitnessImportFilter::default())
}

fn parse_witness_log_filtered(
    text: &str,
    filter: WitnessImportFilter,
) -> AppResult<Vec<InputEvent>> {
    let marker = format!("[{WITNESS_LOG_IDENTITY}]");
    let mut contract_count = 0_usize;
    let mut queries = Vec::new();
    let mut previous_window = 0_u64;
    let mut previous_render_end = 0_u64;
    for (line_index, line) in text.lines().enumerate() {
        let Some((_, payload)) = line.split_once(&marker) else {
            continue;
        };
        let payload = payload.trim();
        if payload.starts_with("passive=") {
            if payload != "passive=true actions=0..11 callbackReturn=nil flushRenders=60" {
                return Err(AppError::evidence(format!(
                    "witness contract mismatch on log line {}",
                    line_index + 1
                )));
            }
            contract_count += 1;
            continue;
        }
        if !payload.starts_with("window=") {
            return Err(AppError::evidence(format!(
                "unknown witness record on log line {}",
                line_index + 1
            )));
        }
        let fields = parse_key_value_fields(payload, 5, line_index + 1)?;
        let window = parse_field_u64(&fields, "window", line_index + 1)?;
        let render_end = parse_field_u64(&fields, "renderEnd", line_index + 1)?;
        let total = parse_field_u64(&fields, "total", line_index + 1)?;
        let dropped = parse_field_u64(&fields, "droppedUnique", line_index + 1)?;
        if window == 0
            || window <= previous_window
            || render_end <= previous_render_end
            || dropped != 0
        {
            return Err(AppError::evidence(format!(
                "witness window ordering or completeness failed on log line {}",
                line_index + 1
            )));
        }
        previous_window = window;
        previous_render_end = render_end;
        let observations = fields.get("observations").ok_or_else(|| {
            AppError::evidence(format!(
                "missing witness observations on log line {}",
                line_index + 1
            ))
        })?;
        let mut observed_total = 0_u64;
        for observation in observations.split(',') {
            let parts = parse_slash_observation(observation, line_index + 1)?;
            let action = parse_map_u32(&parts, "a", line_index + 1)?;
            let controller_index = parse_map_i32(&parts, "c", line_index + 1)?;
            let count = parse_map_u64(&parts, "n", line_index + 1)?;
            let input_hook = parts.get("h").cloned().ok_or_else(|| {
                AppError::evidence(format!("missing input hook on log line {}", line_index + 1))
            })?;
            if action > 11 || count == 0 || input_hook.is_empty() {
                return Err(AppError::evidence(format!(
                    "invalid witness observation on log line {}",
                    line_index + 1
                )));
            }
            observed_total = observed_total
                .checked_add(count)
                .ok_or_else(|| AppError::evidence("witness observation count overflow"))?;
            let within_window = filter.window_min.is_none_or(|minimum| window >= minimum)
                && filter.window_max.is_none_or(|maximum| window <= maximum);
            if !within_window
                || filter
                    .controller_index
                    .is_some_and(|expected| controller_index != expected)
            {
                continue;
            }
            queries.push(InputEvent {
                event_sequence: 0,
                stage: "witness-query".to_owned(),
                loop_iteration: window,
                phase: "WITNESS".to_owned(),
                authority_tick: 0,
                session_id: None,
                client_id: None,
                command_sequence: None,
                target_tick: None,
                canonical_state_hex: None,
                decision: None,
                edge_flags: None,
                world_witness_before: None,
                world_witness_after: None,
                action: Some(action),
                input_hook: Some(input_hook),
                controller_index: Some(controller_index),
                observation_count: Some(count),
                render_end: Some(render_end),
                native_sequence: None,
                qpc: None,
                thread_id: None,
                native_kind: None,
                flags: None,
                return_rva: None,
                original_calls: None,
                error_code: None,
                device_key: None,
                selected_device_key: None,
                controller_id_raw: None,
                primitive: None,
                value_bits: None,
                classification: None,
            });
        }
        if observed_total != total {
            return Err(AppError::evidence(format!(
                "witness total mismatch on log line {}",
                line_index + 1
            )));
        }
    }
    if contract_count != 1 {
        return Err(AppError::evidence(
            "witness log must contain exactly one passive contract record",
        ));
    }
    let mut events = vec![InputEvent {
        event_sequence: 1,
        stage: "witness-contract".to_owned(),
        loop_iteration: 0,
        phase: "WITNESS".to_owned(),
        authority_tick: 0,
        session_id: None,
        client_id: None,
        command_sequence: None,
        target_tick: None,
        canonical_state_hex: None,
        decision: None,
        edge_flags: None,
        world_witness_before: None,
        world_witness_after: None,
        action: None,
        input_hook: None,
        controller_index: None,
        observation_count: None,
        render_end: None,
        native_sequence: None,
        qpc: None,
        thread_id: None,
        native_kind: None,
        flags: None,
        return_rva: None,
        original_calls: None,
        error_code: None,
        device_key: None,
        selected_device_key: None,
        controller_id_raw: None,
        primitive: None,
        value_bits: None,
        classification: None,
    }];
    for (index, mut event) in queries.into_iter().enumerate() {
        event.event_sequence = index as u64 + 2;
        events.push(event);
    }
    Ok(events)
}

fn parse_key_value_fields(
    payload: &str,
    expected: usize,
    line: usize,
) -> AppResult<BTreeMap<String, String>> {
    let fields = payload
        .splitn(expected, ' ')
        .map(|field| {
            field
                .split_once('=')
                .map(|(key, value)| (key.to_owned(), value.to_owned()))
                .ok_or_else(|| AppError::evidence(format!("invalid witness field on line {line}")))
        })
        .collect::<AppResult<BTreeMap<_, _>>>()?;
    if fields.len() != expected {
        return Err(AppError::evidence(format!(
            "wrong witness field count on line {line}"
        )));
    }
    Ok(fields)
}

fn parse_slash_observation(value: &str, line: usize) -> AppResult<BTreeMap<String, String>> {
    let fields = value
        .split('/')
        .map(|field| {
            field
                .split_once('=')
                .map(|(key, value)| (key.to_owned(), value.to_owned()))
                .ok_or_else(|| {
                    AppError::evidence(format!("invalid witness observation on line {line}"))
                })
        })
        .collect::<AppResult<BTreeMap<_, _>>>()?;
    if fields.len() != 4
        || ["a", "h", "c", "n"]
            .iter()
            .any(|key| !fields.contains_key(*key))
    {
        return Err(AppError::evidence(format!(
            "witness observation shape mismatch on line {line}"
        )));
    }
    Ok(fields)
}

fn parse_field_u64(fields: &BTreeMap<String, String>, name: &str, line: usize) -> AppResult<u64> {
    fields
        .get(name)
        .ok_or_else(|| AppError::evidence(format!("missing {name} on witness line {line}")))?
        .parse()
        .map_err(|_| AppError::evidence(format!("invalid {name} on witness line {line}")))
}

fn parse_map_u32(fields: &BTreeMap<String, String>, name: &str, line: usize) -> AppResult<u32> {
    fields
        .get(name)
        .ok_or_else(|| AppError::evidence(format!("missing {name} on witness line {line}")))?
        .parse()
        .map_err(|_| AppError::evidence(format!("invalid {name} on witness line {line}")))
}

fn parse_map_u64(fields: &BTreeMap<String, String>, name: &str, line: usize) -> AppResult<u64> {
    fields
        .get(name)
        .ok_or_else(|| AppError::evidence(format!("missing {name} on witness line {line}")))?
        .parse()
        .map_err(|_| AppError::evidence(format!("invalid {name} on witness line {line}")))
}

fn parse_map_i32(fields: &BTreeMap<String, String>, name: &str, line: usize) -> AppResult<i32> {
    fields
        .get(name)
        .ok_or_else(|| AppError::evidence(format!("missing {name} on witness line {line}")))?
        .parse()
        .map_err(|_| AppError::evidence(format!("invalid {name} on witness line {line}")))
}

pub fn read_input_run(run_directory: &Path) -> AppResult<(InputManifest, Vec<InputEvent>)> {
    let manifest_path = run_directory.join("manifest.json");
    let events_path = run_directory.join("events.jsonl");
    let manifest: InputManifest = serde_json::from_slice(
        &fs::read(&manifest_path)
            .usage_context(format!("cannot read {}", manifest_path.display()))?,
    )
    .usage_context(format!("cannot parse {}", manifest_path.display()))?;
    validate_manifest(&manifest)?;
    let file = fs::File::open(&events_path)
        .usage_context(format!("cannot read {}", events_path.display()))?;
    let mut events = Vec::new();
    for (index, line) in BufReader::new(file).lines().enumerate() {
        let line = line.usage_context(format!("cannot read event line {}", index + 1))?;
        events.push(
            serde_json::from_str(&line)
                .usage_context(format!("cannot parse event line {}", index + 1))?,
        );
    }
    Ok((manifest, events))
}

pub fn analyze_input_run(run_directory: &Path) -> AppResult<InputAnalysisReport> {
    let report = analyze_input_run_internal(run_directory)?;
    let bytes = serde_json::to_vec_pretty(&report).usage_context("cannot serialize analysis")?;
    fs::write(run_directory.join("analysis.json"), bytes)
        .usage_context("cannot write input analysis")?;
    Ok(report)
}

fn analyze_input_run_internal(run_directory: &Path) -> AppResult<InputAnalysisReport> {
    let (manifest, events) = read_input_run(run_directory)?;
    if manifest.state != "COMPLETE" {
        return Err(AppError::evidence("input artifact is not COMPLETE"));
    }
    let nonempty = !events.is_empty();
    let sequence_contiguous = events
        .iter()
        .enumerate()
        .all(|(index, event)| event.event_sequence == index as u64 + 1);
    let known_phase = events.iter().all(|event| {
        matches!(
            event.phase.as_str(),
            "DRAIN_BEFORE_RESOLVE"
                | "RESOLVE"
                | "APPLY"
                | "WITNESS"
                | "LOCAL_PLANNER"
                | "TRANSPORT"
                | "OUTSIDE_AUTHORITY"
                | "OUTSIDE_ENVELOPE"
                | "LOOP_BOUNDARY"
                | "MESSAGE_PUMP"
                | "INPUT_QUERY"
                | "GAMEPLAY_UPDATE"
                | "ENTITY_MUTATION"
                | "POST_UPDATE"
                | "RENDER"
                | "ROOM_LIFECYCLE"
                | "RUN_LIFECYCLE"
        )
    });
    let known_stage = events.iter().all(|event| {
        matches!(
            event.stage.as_str(),
            "sample"
                | "canonical"
                | "send"
                | "receive"
                | "accept"
                | "reject"
                | "duplicate"
                | "resolve"
                | "apply"
                | "witness"
                | "witness-contract"
                | "witness-query"
                | "control-observation"
                | "native-lifecycle"
                | "passive-logical-query"
                | "passive-manager-query"
                | "passive-logical-setter"
        )
    });
    let ui_partitioned = events
        .iter()
        .all(|event| event.action.is_none_or(|action| action <= 11));
    let canonical_valid = events.iter().all(|event| {
        event
            .canonical_state_hex
            .as_deref()
            .is_none_or(validate_state_hex)
    });
    let requirements_met = requirements_observed(&events, &manifest.requirements);
    let evidence_scope_valid = evidence_scope_valid(&manifest, &events);
    let passive_runtime = manifest.evidence_class == EVIDENCE_PASSIVE_INPUT_RUNTIME;
    let passive_query_count = events
        .iter()
        .filter(|event| {
            matches!(
                event.stage.as_str(),
                "passive-logical-query" | "passive-manager-query"
            )
        })
        .count();
    let passive_setter_count = events
        .iter()
        .filter(|event| event.stage == "passive-logical-setter")
        .count();
    let passive_query_present = !passive_runtime
        || events.iter().any(|event| {
            matches!(
                event.stage.as_str(),
                "passive-logical-query" | "passive-manager-query"
            ) && event.loop_iteration != 0
                && event.phase != "OUTSIDE_ENVELOPE"
                && event.flags.is_some_and(|flags| {
                    flags & (INPUT_FLAG_LOOP_KNOWN | INPUT_FLAG_MAIN_THREAD)
                        == INPUT_FLAG_LOOP_KNOWN | INPUT_FLAG_MAIN_THREAD
                })
        });
    let passive_pairs_valid = !passive_runtime || passive_native_pairs_valid(&events);
    let passive_lifecycle_valid = !passive_runtime || passive_native_lifecycle_valid(&events);
    let passive_runtime_integrity = !passive_runtime
        || manifest.ipc_schema_version == Some(INPUT_IPC_SCHEMA_VERSION)
            && manifest.installed_hooks_mask == Some(INPUT_HOOK_MASK)
            && manifest.restored_hooks_mask == Some(INPUT_HOOK_MASK)
            && manifest.overwritten_events == Some(0)
            && manifest.producer_closed == Some(true)
            && manifest.target_bytes_restored == Some(true)
            && manifest.main_thread_id.is_some_and(|thread| thread != 0)
            && events.iter().all(|event| {
                event.native_sequence.is_some()
                    && event.qpc.is_some_and(|qpc| qpc > 0)
                    && (event.stage == "native-lifecycle"
                        || event.thread_id == manifest.main_thread_id)
                    && (event.stage == "native-lifecycle"
                        || event.flags.is_some_and(|flags| {
                            (flags & INPUT_FLAG_KEYBOARD_DEVICE != 0)
                                ^ (flags & INPUT_FLAG_GAMEPAD_DEVICE != 0)
                        }))
                    && event.native_kind.as_deref().is_some_and(|kind| {
                        if kind.ends_with("_ENTER") {
                            event.original_calls == Some(0)
                                && event
                                    .flags
                                    .is_some_and(|flags| flags & INPUT_FLAG_ORIGINAL_CALLED == 0)
                        } else if kind.ends_with("_EXIT") {
                            event.original_calls == Some(1)
                                && event
                                    .flags
                                    .is_some_and(|flags| flags & INPUT_FLAG_ORIGINAL_CALLED != 0)
                        } else {
                            true
                        }
                    })
                    && event.error_code == Some(0)
            })
            && events.iter().enumerate().all(|(index, event)| {
                event.native_sequence == Some(index as u64 + 1)
                    && event.event_sequence == index as u64 + 1
            })
            && passive_lifecycle_valid;
    let companion_binding_valid = !passive_runtime
        || validate_passive_companion_binding(run_directory, &manifest, &events).is_ok();

    let mut by_identity: BTreeMap<Identity, Vec<&InputEvent>> = BTreeMap::new();
    for event in &events {
        if let Some(identity) = identity(event) {
            by_identity.entry(identity).or_default().push(event);
        }
    }
    let mut identity_ordered = true;
    let mut rejection_unchanged = true;
    let mut duplicate_once = true;
    let mut accepted_count = 0;
    let mut rejected_count = 0;
    let mut target_applies = BTreeMap::<(String, u64, u64), usize>::new();
    for (identity, trace) in &by_identity {
        let ranks = trace
            .iter()
            .map(|event| stage_rank(&event.stage))
            .collect::<Option<Vec<_>>>();
        identity_ordered &= ranks
            .as_ref()
            .is_some_and(|ranks| ranks.windows(2).all(|pair| pair[0] <= pair[1]));
        let accepted = trace
            .iter()
            .any(|event| event.decision.as_deref() == Some("accepted"));
        let rejected = trace.iter().any(|event| {
            event
                .decision
                .as_deref()
                .is_some_and(|decision| decision != "accepted" && decision != "duplicate")
        });
        accepted_count += usize::from(accepted);
        rejected_count += usize::from(rejected);
        let apply_count = trace.iter().filter(|event| event.stage == "apply").count();
        if accepted {
            duplicate_once &= apply_count <= 1;
        }
        if rejected {
            let witness_pairs = trace
                .iter()
                .filter_map(|event| {
                    Some((
                        event.world_witness_before.as_ref()?,
                        event.world_witness_after.as_ref()?,
                    ))
                })
                .collect::<Vec<_>>();
            rejection_unchanged &= apply_count == 0
                && !witness_pairs.is_empty()
                && witness_pairs.iter().all(|(before, after)| before == after);
        }
        for event in trace.iter().filter(|event| event.stage == "apply") {
            if let Some(target) = event.target_tick {
                *target_applies
                    .entry((identity.session_id.clone(), identity.client_id, target))
                    .or_default() += 1;
            }
        }
    }
    let one_apply_per_target = target_applies.values().all(|count| *count <= 1);
    let disarmed_clean = if matches!(
        manifest.probe.as_str(),
        "control-no-native" | "original-online-disarmed"
    ) {
        events.iter().all(|event| {
            !matches!(
                event.stage.as_str(),
                "canonical" | "send" | "receive" | "resolve" | "apply"
            )
        })
    } else {
        true
    };
    let resolved_in_authority = events.iter().all(|event| {
        !matches!(event.stage.as_str(), "accept" | "reject" | "duplicate")
            || event.phase == "DRAIN_BEFORE_RESOLVE"
    });
    let phase_matches_stage = events.iter().all(|event| match event.stage.as_str() {
        "accept" | "reject" | "duplicate" => event.phase == "DRAIN_BEFORE_RESOLVE",
        "resolve" => event.phase == "RESOLVE",
        "apply" => event.phase == "APPLY",
        "witness" | "witness-contract" | "witness-query" => event.phase == "WITNESS",
        "control-observation" => event.phase == "OUTSIDE_AUTHORITY",
        "native-lifecycle" => event.phase == "OUTSIDE_ENVELOPE",
        "passive-logical-query" | "passive-manager-query" | "passive-logical-setter" => {
            event.phase != "OUTSIDE_ENVELOPE"
        }
        _ => true,
    });
    let pipeline_shape = if manifest.evidence_class == EVIDENCE_PIPELINE_RUNTIME {
        accepted_count != 0
            && events.iter().any(|event| event.stage == "apply")
            && events.iter().all(|event| {
                !matches!(
                    event.stage.as_str(),
                    "canonical"
                        | "send"
                        | "receive"
                        | "accept"
                        | "reject"
                        | "duplicate"
                        | "resolve"
                        | "apply"
                        | "witness"
                ) || identity(event).is_some() && event.target_tick.is_some()
            })
    } else {
        true
    };

    let canonical_trace = canonical_trace(&events);
    let decision_trace = decision_trace(&events);
    let apply_trace = apply_trace(&events);
    let checks = vec![
        check(
            "non-empty-events",
            nonempty,
            "artifact contains at least one source event",
        ),
        check(
            "event-sequence",
            sequence_contiguous,
            "event sequence has no gaps",
        ),
        check(
            "phase-known",
            known_phase,
            "every event has a defined phase",
        ),
        check(
            "stage-known",
            known_stage,
            "every event has a defined stage",
        ),
        check(
            "phase-stage-consistent",
            phase_matches_stage,
            "stage and phase agree",
        ),
        check(
            "scenario-minimum-evidence",
            requirements_met,
            "scenario-specific event, phase, window, and action minima are met",
        ),
        check(
            "evidence-scope",
            evidence_scope_valid,
            "evidence class contains only permitted fields and stages",
        ),
        check("ui-partition", ui_partitioned, "no action 12+ enters T0-04"),
        check(
            "canonical-codec",
            canonical_valid,
            "all canonical states are strict 24-byte V1",
        ),
        check(
            "identity-order",
            identity_ordered,
            "identity stages are ordered",
        ),
        check(
            "reject-world-unchanged",
            rejection_unchanged,
            "rejections do not apply or mutate witness",
        ),
        check(
            "duplicate-once",
            duplicate_once,
            "duplicates never apply twice",
        ),
        check(
            "one-apply-per-target",
            one_apply_per_target,
            "one adapter apply per client target",
        ),
        check(
            "drain-before-resolve",
            resolved_in_authority,
            "decisions occur only during drain-before-resolve",
        ),
        check(
            "disarmed-clean",
            disarmed_clean,
            "control and OriginalOnline create no canonical pipeline events",
        ),
        check(
            "pipeline-shape",
            pipeline_shape,
            "runtime pipeline stages carry complete command identity",
        ),
        check(
            "passive-runtime-query",
            passive_query_present,
            &format!(
                "passive runtime has {passive_query_count} safely persisted logical/manager query events"
            ),
        ),
        check(
            "passive-runtime-integrity",
            passive_runtime_integrity && passive_pairs_valid,
            "schema-2 hook masks, sequence, thread, overflow, close, lifecycle, and restore are exact",
        ),
        check(
            "passive-enter-exit-pairs",
            passive_pairs_valid,
            "per-thread passive input enter/exit records are balanced LIFO pairs with exact immutable fields",
        ),
        check(
            "passive-companion-loop-binding",
            companion_binding_valid,
            "companion V5 source hashes, restore identity, and loop context are bound",
        ),
        check(
            "passive-setter-diagnostic",
            true,
            &format!(
                "passive runtime records {passive_setter_count} logical setter diagnostic events; setter hits are optional"
            ),
        ),
    ];
    let passed = checks.iter().all(|check| check.passed);
    let eligible_for_active_gate = passed
        && matches!(
            manifest.evidence_class.as_str(),
            EVIDENCE_PIPELINE_RUNTIME | EVIDENCE_DISARMED_CONTROL
        );
    let report = InputAnalysisReport {
        schema_version: INPUT_ARTIFACT_SCHEMA_VERSION,
        result: if passed { "PASS" } else { "FAIL" }.to_owned(),
        case_id: manifest.case_id,
        probe: manifest.probe,
        scenario: manifest.scenario,
        evidence_class: manifest.evidence_class,
        eligible_for_active_gate,
        artifact_identity: manifest.artifact_identity,
        event_count: events.len(),
        accepted_count,
        rejected_count,
        apply_count: events.iter().filter(|event| event.stage == "apply").count(),
        canonical_trace_sha256: sha256_hex(canonical_trace.as_bytes()),
        decision_trace_sha256: sha256_hex(decision_trace.as_bytes()),
        apply_trace_sha256: sha256_hex(apply_trace.as_bytes()),
        checks,
    };
    Ok(report)
}

fn requirements_observed(events: &[InputEvent], requirements: &InputEvidenceRequirements) -> bool {
    if events.len() < requirements.minimum_events {
        return false;
    }
    let windows = events
        .iter()
        .filter(|event| event.loop_iteration != 0)
        .map(|event| event.loop_iteration)
        .collect::<BTreeSet<_>>();
    if windows.len() < requirements.minimum_windows {
        return false;
    }
    if requirements
        .required_phases
        .iter()
        .any(|required| !events.iter().any(|event| event.phase == *required))
        || requirements
            .required_stages
            .iter()
            .any(|required| !events.iter().any(|event| event.stage == *required))
    {
        return false;
    }
    for action in &requirements.required_actions {
        let observations = events
            .iter()
            .filter(|event| event.action == Some(*action))
            .map(|event| event.observation_count.unwrap_or(1))
            .sum::<u64>();
        if observations < requirements.minimum_observations_per_required_action {
            return false;
        }
        let controller_matches = match requirements.controller_scope.as_str() {
            "ANY" => true,
            "NONNEGATIVE" => events.iter().any(|event| {
                event.action == Some(*action)
                    && event.controller_index.is_some_and(|index| index >= 0)
            }),
            "ENTITYLESS" => events.iter().any(|event| {
                event.action == Some(*action)
                    && event.controller_index.is_some_and(|index| index < 0)
            }),
            "NATIVE_RUNTIME" => events.iter().any(|event| {
                event.action == Some(*action)
                    && event.device_key.is_some()
                    && event.controller_id_raw.is_some()
            }),
            "NONE" => false,
            _ => return false,
        };
        if !controller_matches {
            return false;
        }
    }
    if requirements.controller_scope == "NONE"
        && events.iter().any(|event| event.controller_index.is_some())
    {
        return false;
    }
    if requirements.forbid_pipeline_events
        && events.iter().any(|event| is_pipeline_stage(&event.stage))
    {
        return false;
    }
    true
}

fn evidence_scope_valid(manifest: &InputManifest, events: &[InputEvent]) -> bool {
    match manifest.evidence_class.as_str() {
        EVIDENCE_WITNESS_QUERY_ONLY => events.iter().all(|event| {
            let common_empty = identity(event).is_none()
                && event.session_id.is_none()
                && event.client_id.is_none()
                && event.command_sequence.is_none()
                && event.target_tick.is_none()
                && event.canonical_state_hex.is_none()
                && event.decision.is_none()
                && event.edge_flags.is_none()
                && event.world_witness_before.is_none()
                && event.world_witness_after.is_none()
                && event.authority_tick == 0
                && native_fields_empty(event);
            let source_filter_matches = event.stage != "witness-query"
                || manifest
                    .source_window_min
                    .is_none_or(|minimum| event.loop_iteration >= minimum)
                    && manifest
                        .source_window_max
                        .is_none_or(|maximum| event.loop_iteration <= maximum)
                    && manifest
                        .source_controller_index
                        .is_none_or(|expected| event.controller_index == Some(expected));
            common_empty
                && source_filter_matches
                && match event.stage.as_str() {
                    "witness-contract" => {
                        event.action.is_none()
                            && event.input_hook.is_none()
                            && event.controller_index.is_none()
                            && event.observation_count.is_none()
                            && event.render_end.is_none()
                    }
                    "witness-query" => {
                        event.action.is_some_and(|action| action <= 11)
                            && event
                                .input_hook
                                .as_ref()
                                .is_some_and(|hook| !hook.is_empty())
                            && event.controller_index.is_some()
                            && event.observation_count.is_some_and(|count| count != 0)
                            && event.render_end.is_some()
                            && event.loop_iteration != 0
                    }
                    _ => false,
                }
        }),
        EVIDENCE_DISARMED_CONTROL => events.iter().all(|event| {
            event.stage == "control-observation"
                && event.action.is_none()
                && event.canonical_state_hex.is_none()
                && event.decision.is_none()
                && event.session_id.is_none()
                && event.client_id.is_none()
                && event.command_sequence.is_none()
                && event.target_tick.is_none()
                && event.edge_flags.is_none()
                && event.input_hook.is_none()
                && event.controller_index.is_none()
                && event.observation_count.is_none()
                && native_fields_empty(event)
        }),
        EVIDENCE_PASSIVE_INPUT_RUNTIME => events.iter().all(|event| {
            let no_pipeline = identity(event).is_none()
                && event.session_id.is_none()
                && event.client_id.is_none()
                && event.command_sequence.is_none()
                && event.target_tick.is_none()
                && event.canonical_state_hex.is_none()
                && event.decision.is_none()
                && event.edge_flags.is_none()
                && event.world_witness_before.is_none()
                && event.world_witness_after.is_none()
                && event.authority_tick == 0
                && event.controller_index.is_none()
                && event.render_end.is_none()
                && event.native_sequence.is_some_and(|sequence| sequence != 0)
                && event.qpc.is_some_and(|qpc| qpc > 0)
                && event.thread_id.is_some_and(|thread| thread != 0)
                && event
                    .native_kind
                    .as_ref()
                    .is_some_and(|kind| !kind.is_empty())
                && event.flags.is_some_and(|flags| {
                    flags & !INPUT_KNOWN_FLAGS == 0
                        && flags & (INPUT_FLAG_WRONG_THREAD | INPUT_FLAG_REDACTED) == 0
                })
                && event.original_calls.is_some()
                && event.error_code.is_some();
            no_pipeline
                && match event.stage.as_str() {
                    "native-lifecycle" => {
                        let lifecycle_error_valid = match event.native_kind.as_deref() {
                            Some("FAILURE") => event.error_code.is_some_and(|code| code != 0),
                            Some(
                                "NATIVE_INITIALIZED" | "HOOKS_INSTALLED" | "HOOKS_RESTORED"
                                | "NATIVE_RETIRED",
                            ) => event.error_code == Some(0),
                            _ => false,
                        };
                        event.phase == "OUTSIDE_ENVELOPE"
                            && event.loop_iteration == 0
                            && event.flags == Some(0)
                            && event.return_rva.is_none()
                            && event.original_calls == Some(0)
                            && lifecycle_error_valid
                            && event.action.is_none()
                            && event.input_hook.is_none()
                            && event.observation_count.is_none()
                            && event.device_key.is_none()
                            && event.selected_device_key.is_none()
                            && event.controller_id_raw.is_none()
                            && event.primitive.is_none()
                            && event.value_bits.is_none()
                            && event.classification.is_none()
                    }
                    "passive-logical-query" => {
                        passive_payload_scope_valid(event)
                            && event.action.is_some_and(|action| action <= 11)
                            && event.device_key.is_some()
                            && event.primitive.is_none()
                            && event.classification.as_deref() == Some("EXPLICIT_GAMEPLAY")
                            && event.observation_count == Some(1)
                    }
                    "passive-manager-query" => {
                        passive_payload_scope_valid(event)
                            && event.action.is_some_and(|action| action <= 11)
                            && event.device_key.is_some()
                            && event.primitive.is_some()
                            && event.classification.as_deref() == Some("UNIQUE_GAMEPLAY")
                            && event.observation_count == Some(1)
                    }
                    "passive-logical-setter" => {
                        passive_payload_scope_valid(event)
                            && event.action.is_some_and(|action| action <= 11)
                            && event.device_key.is_some()
                            && event.primitive.is_none()
                            && event.value_bits.is_some()
                            && event.classification.as_deref()
                                == Some("LOGICAL_OVERRIDE_DIAGNOSTIC")
                            && event.observation_count == Some(1)
                    }
                    _ => false,
                }
        }),
        EVIDENCE_PIPELINE_RUNTIME => {
            manifest.source_log_sha256.is_none() && events.iter().all(native_fields_empty)
        }
        _ => false,
    }
}

fn passive_payload_scope_valid(event: &InputEvent) -> bool {
    let Some(kind) = event.native_kind.as_deref() else {
        return false;
    };
    let kind_matches_stage = passive_pair_descriptor(kind)
        .is_some_and(|(_, expected_stage, _, _)| expected_stage == event.stage);
    let flags = event.flags.unwrap_or(0);
    kind_matches_stage
        && event.input_hook.as_deref() == Some(kind)
        && event.phase != "OUTSIDE_ENVELOPE"
        && event.loop_iteration != 0
        && event.return_rva.is_some()
        && event.controller_id_raw.is_some()
        && event.error_code == Some(0)
        && flags & (INPUT_FLAG_LOOP_KNOWN | INPUT_FLAG_MAIN_THREAD)
            == INPUT_FLAG_LOOP_KNOWN | INPUT_FLAG_MAIN_THREAD
        && ((flags & INPUT_FLAG_KEYBOARD_DEVICE != 0) ^ (flags & INPUT_FLAG_GAMEPAD_DEVICE != 0))
}

fn native_fields_empty(event: &InputEvent) -> bool {
    event.native_sequence.is_none()
        && event.qpc.is_none()
        && event.thread_id.is_none()
        && event.native_kind.is_none()
        && event.flags.is_none()
        && event.return_rva.is_none()
        && event.original_calls.is_none()
        && event.error_code.is_none()
        && event.device_key.is_none()
        && event.selected_device_key.is_none()
        && event.controller_id_raw.is_none()
        && event.primitive.is_none()
        && event.value_bits.is_none()
        && event.classification.is_none()
}

fn passive_pair_descriptor(kind: &str) -> Option<(&'static str, &'static str, bool, bool)> {
    match kind {
        "LOGICAL_PRESSED_ENTER" => Some(("LOGICAL_PRESSED", "passive-logical-query", true, false)),
        "LOGICAL_PRESSED_EXIT" => Some(("LOGICAL_PRESSED", "passive-logical-query", false, false)),
        "LOGICAL_TRIGGERED_ENTER" => {
            Some(("LOGICAL_TRIGGERED", "passive-logical-query", true, false))
        }
        "LOGICAL_TRIGGERED_EXIT" => {
            Some(("LOGICAL_TRIGGERED", "passive-logical-query", false, false))
        }
        "LOGICAL_SCALAR_ENTER" => Some(("LOGICAL_SCALAR", "passive-logical-query", true, false)),
        "LOGICAL_SCALAR_EXIT" => Some(("LOGICAL_SCALAR", "passive-logical-query", false, false)),
        "MANAGER_PRESSED_ENTER" => Some(("MANAGER_PRESSED", "passive-manager-query", true, false)),
        "MANAGER_PRESSED_EXIT" => Some(("MANAGER_PRESSED", "passive-manager-query", false, false)),
        "MANAGER_TRIGGERED_ENTER" => {
            Some(("MANAGER_TRIGGERED", "passive-manager-query", true, false))
        }
        "MANAGER_TRIGGERED_EXIT" => {
            Some(("MANAGER_TRIGGERED", "passive-manager-query", false, false))
        }
        "MANAGER_SCALAR_ENTER" => Some(("MANAGER_SCALAR", "passive-manager-query", true, false)),
        "MANAGER_SCALAR_EXIT" => Some(("MANAGER_SCALAR", "passive-manager-query", false, false)),
        "LOGICAL_SETTER_ENTER" => Some(("LOGICAL_SETTER", "passive-logical-setter", true, true)),
        "LOGICAL_SETTER_EXIT" => Some(("LOGICAL_SETTER", "passive-logical-setter", false, true)),
        _ => None,
    }
}

fn passive_native_pairs_valid(events: &[InputEvent]) -> bool {
    let mut stacks = BTreeMap::<u32, Vec<(&InputEvent, &'static str, bool)>>::new();
    for event in events {
        if event.stage == "native-lifecycle" {
            continue;
        }
        let Some(thread_id) = event.thread_id else {
            return false;
        };
        let Some(kind) = event.native_kind.as_deref() else {
            return false;
        };
        let Some((family, expected_stage, enter, setter)) = passive_pair_descriptor(kind) else {
            return false;
        };
        if event.stage != expected_stage
            || event.input_hook.as_deref() != Some(kind)
            || !passive_value_contract_valid(event, kind)
        {
            return false;
        }
        let flags = event.flags.unwrap_or(u32::MAX);
        if enter {
            if event.original_calls != Some(0) || flags & INPUT_FLAG_ORIGINAL_CALLED != 0 {
                return false;
            }
            stacks
                .entry(thread_id)
                .or_default()
                .push((event, family, setter));
            continue;
        }
        if event.original_calls != Some(1) || flags & INPUT_FLAG_ORIGINAL_CALLED == 0 {
            return false;
        }
        let Some((entered, entered_family, entered_setter)) =
            stacks.get_mut(&thread_id).and_then(Vec::pop)
        else {
            return false;
        };
        if entered_family != family
            || entered_setter != setter
            || entered.stage != event.stage
            || entered.loop_iteration != event.loop_iteration
            || entered.phase != event.phase
            || entered
                .qpc
                .zip(event.qpc)
                .is_none_or(|(enter_qpc, exit_qpc)| enter_qpc > exit_qpc)
            || entered.action != event.action
            || entered.return_rva != event.return_rva
            || entered.device_key != event.device_key
            || entered.controller_id_raw != event.controller_id_raw
            || entered.selected_device_key != event.selected_device_key
            || entered.primitive != event.primitive
            || entered.classification != event.classification
            || entered
                .flags
                .map(|flags| flags & (INPUT_FLAG_KEYBOARD_DEVICE | INPUT_FLAG_GAMEPAD_DEVICE))
                != event
                    .flags
                    .map(|flags| flags & (INPUT_FLAG_KEYBOARD_DEVICE | INPUT_FLAG_GAMEPAD_DEVICE))
            || setter && (entered.value_bits.is_none() || entered.value_bits != event.value_bits)
        {
            return false;
        }
    }
    stacks.values().all(Vec::is_empty)
}

fn passive_native_lifecycle_valid(events: &[InputEvent]) -> bool {
    if events.len() < 4 {
        return false;
    }
    let lifecycle = events
        .iter()
        .enumerate()
        .filter(|(_, event)| event.stage == "native-lifecycle")
        .map(|(index, event)| (index, event.native_kind.as_deref()))
        .collect::<Vec<_>>();
    if lifecycle
        != [
            (0, Some("NATIVE_INITIALIZED")),
            (1, Some("HOOKS_INSTALLED")),
            (events.len() - 2, Some("HOOKS_RESTORED")),
            (events.len() - 1, Some("NATIVE_RETIRED")),
        ]
    {
        return false;
    }
    let (Some(initialized), Some(installed), Some(restored), Some(retired)) = (
        events[0].qpc,
        events[1].qpc,
        events[events.len() - 2].qpc,
        events[events.len() - 1].qpc,
    ) else {
        return false;
    };
    initialized <= installed
        && installed <= restored
        && restored <= retired
        && events[2..events.len() - 2].iter().all(|event| {
            event
                .qpc
                .is_some_and(|qpc| installed <= qpc && qpc <= restored)
        })
}

fn passive_value_contract_valid(event: &InputEvent, kind: &str) -> bool {
    let Some(flags) = event.flags else {
        return false;
    };
    let value_valid = flags & INPUT_FLAG_VALUE_VALID != 0;
    let result_true = flags & INPUT_FLAG_RESULT_TRUE != 0;
    match kind {
        "LOGICAL_PRESSED_ENTER"
        | "LOGICAL_TRIGGERED_ENTER"
        | "LOGICAL_SCALAR_ENTER"
        | "MANAGER_PRESSED_ENTER"
        | "MANAGER_TRIGGERED_ENTER"
        | "MANAGER_SCALAR_ENTER" => !value_valid && !result_true && event.value_bits.is_none(),
        "LOGICAL_PRESSED_EXIT"
        | "LOGICAL_TRIGGERED_EXIT"
        | "MANAGER_PRESSED_EXIT"
        | "MANAGER_TRIGGERED_EXIT" => {
            value_valid
                && event.value_bits.is_some_and(|bits| bits <= 1)
                && result_true == (event.value_bits == Some(1))
        }
        "LOGICAL_SCALAR_EXIT" | "MANAGER_SCALAR_EXIT" => {
            value_valid
                && event
                    .value_bits
                    .is_some_and(|bits| result_true == (f32::from_bits(bits) != 0.0))
        }
        "LOGICAL_SETTER_ENTER" | "LOGICAL_SETTER_EXIT" => {
            value_valid && !result_true && event.value_bits.is_some()
        }
        _ => false,
    }
}

fn is_pipeline_stage(stage: &str) -> bool {
    matches!(
        stage,
        "sample"
            | "canonical"
            | "send"
            | "receive"
            | "accept"
            | "reject"
            | "duplicate"
            | "resolve"
            | "apply"
            | "witness"
    )
}

pub fn compare_input_runs(
    first: &Path,
    second: &Path,
    output: Option<&Path>,
) -> AppResult<InputComparisonReport> {
    let (first_manifest, first_events) = read_input_run(first)?;
    let (second_manifest, second_events) = read_input_run(second)?;
    let canonical_equal = canonical_trace(&first_events) == canonical_trace(&second_events);
    let decisions_equal = decision_trace(&first_events) == decision_trace(&second_events);
    let apply_equal = apply_trace(&first_events) == apply_trace(&second_events);
    let event_trace_equal = serde_json::to_vec(&first_events)
        .usage_context("encode first events")?
        == serde_json::to_vec(&second_events).usage_context("encode second events")?;
    let evidence_class_equal = first_manifest.evidence_class == second_manifest.evidence_class;
    let nonempty = !first_events.is_empty() && !second_events.is_empty();
    let constants_equal = serde_json::to_vec(&first_manifest.constants)
        .usage_context("cannot encode constants")?
        == serde_json::to_vec(&second_manifest.constants)
            .usage_context("cannot encode constants")?;
    let checks = vec![
        check(
            "non-empty-events",
            nonempty,
            "both repeatability artifacts contain source events",
        ),
        check(
            "evidence-class-equal",
            evidence_class_equal,
            "repeatability artifacts use the same evidence class",
        ),
        check(
            "constants-equal",
            constants_equal,
            "experimental constants are identical",
        ),
        check(
            "canonical-equal",
            canonical_equal,
            "canonical byte traces are identical",
        ),
        check(
            "decision-equal",
            decisions_equal,
            "decision traces are identical",
        ),
        check("apply-equal", apply_equal, "apply traces are identical"),
        check(
            "full-event-trace-equal",
            event_trace_equal,
            "the complete event traces are identical",
        ),
    ];
    let passed = checks.iter().all(|check| check.passed);
    let eligible_for_active_gate = passed
        && first_manifest.evidence_class == EVIDENCE_PIPELINE_RUNTIME
        && second_manifest.evidence_class == EVIDENCE_PIPELINE_RUNTIME;
    let report = InputComparisonReport {
        schema_version: INPUT_ARTIFACT_SCHEMA_VERSION,
        result: if passed { "PASS" } else { "FAIL" }.to_owned(),
        evidence_class: if evidence_class_equal {
            first_manifest.evidence_class.clone()
        } else {
            "MISMATCH".to_owned()
        },
        eligible_for_active_gate,
        first_artifact_identity: first_manifest.artifact_identity,
        second_artifact_identity: second_manifest.artifact_identity,
        checks,
    };
    if let Some(path) = output {
        write_new_json(path, &report)?;
    }
    Ok(report)
}

pub fn write_input_gate(
    loop_gate_path: &Path,
    matrix_path: &Path,
    repeatability_path: &Path,
    output: &Path,
) -> AppResult<InputGateDecision> {
    if matrix_path.parent().unwrap_or_else(|| Path::new("."))
        != output.parent().unwrap_or_else(|| Path::new("."))
    {
        return Err(AppError::usage(
            "input matrix and gate output must share a directory so evidence bindings remain replayable",
        ));
    }
    let validated_loop_gate = validate_loop_gate_reference(loop_gate_path)?;
    let loop_gate_sha256 = validated_loop_gate.sha256;
    let loop_target_sha256 = validated_loop_gate.target_sha256;
    let repeatability_raw = fs::read(repeatability_path).usage_context(format!(
        "cannot read repeatability comparison {}",
        repeatability_path.display()
    ))?;
    let repeatability: InputComparisonReport = serde_json::from_slice(&repeatability_raw)
        .usage_context(format!(
            "cannot parse repeatability comparison {}",
            repeatability_path.display()
        ))?;
    reject_passive_gate_evidence(&repeatability.evidence_class)?;
    if repeatability.schema_version != INPUT_ARTIFACT_SCHEMA_VERSION
        || repeatability.result != "PASS"
        || !repeatability.eligible_for_active_gate
        || repeatability.evidence_class != EVIDENCE_PIPELINE_RUNTIME
    {
        return Err(AppError::evidence(
            "repeatability comparison is not active-pipeline eligible",
        ));
    }
    let matrix_raw = fs::read(matrix_path).usage_context(format!(
        "cannot read input matrix {}",
        matrix_path.display()
    ))?;
    let matrix: InputMatrix = serde_json::from_slice(&matrix_raw).usage_context(format!(
        "cannot parse input matrix {}",
        matrix_path.display()
    ))?;
    let required = REQUIRED_INPUT_GATE_SCENARIOS;
    let parent = matrix_path.parent().unwrap_or_else(|| Path::new("."));
    let mut passed = BTreeSet::new();
    let mut bindings = Vec::new();
    let mut run_by_identity = std::collections::BTreeMap::<String, PathBuf>::new();
    for relative in &matrix.analyses {
        validate_relative_matrix_path(relative, "input matrix")?;
        let path = parent.join(relative);
        let analysis_bytes = fs::read(&path)
            .usage_context(format!("cannot read input analysis {}", path.display()))?;
        #[derive(Deserialize)]
        #[serde(rename_all = "camelCase")]
        struct GateEvidenceIdentity {
            evidence_class: String,
        }
        let identity: GateEvidenceIdentity = serde_json::from_slice(&analysis_bytes)
            .usage_context(format!(
                "cannot parse input evidence identity {}",
                path.display()
            ))?;
        reject_passive_gate_evidence(&identity.evidence_class)?;
        let stored: InputAnalysisReport = serde_json::from_slice(&analysis_bytes)
            .usage_context(format!("cannot parse input analysis {}", path.display()))?;
        let run = path.parent().ok_or_else(|| {
            AppError::usage(format!("input analysis {} has no run", path.display()))
        })?;
        let analysis = analyze_input_run_internal(run)?;
        let (manifest, _) = read_input_run(run)?;
        reject_passive_gate_evidence(&analysis.evidence_class)?;
        if stored != analysis
            || analysis.schema_version != INPUT_ARTIFACT_SCHEMA_VERSION
            || analysis.result != "PASS"
            || !analysis.checks.iter().all(|check| check.passed)
        {
            return Err(AppError::evidence(format!(
                "input analysis {} is not an exact PASS replay",
                path.display()
            )));
        }
        if manifest.loop_gate_sha256 != loop_gate_sha256
            || manifest.target_sha256 != loop_target_sha256
        {
            return Err(AppError::evidence(format!(
                "input analysis {} is not bound to this loop gate and target",
                path.display()
            )));
        }
        let case_name = required
            .iter()
            .find(|name| {
                if matches!(**name, "control-no-native" | "original-online-disarmed") {
                    analysis.eligible_for_active_gate
                        && analysis.evidence_class == EVIDENCE_DISARMED_CONTROL
                        && analysis.probe == **name
                        && analysis.scenario == **name
                } else {
                    analysis.eligible_for_active_gate
                        && analysis.evidence_class == EVIDENCE_PIPELINE_RUNTIME
                        && analysis.probe == "loopback"
                        && analysis.scenario == **name
                }
            })
            .copied()
            .ok_or_else(|| {
                AppError::evidence(format!(
                    "input analysis {} is not eligible for any required case",
                    path.display()
                ))
            })?;
        if !passed.insert(case_name.to_owned())
            || run_by_identity
                .insert(analysis.artifact_identity.clone(), run.to_owned())
                .is_some()
        {
            return Err(AppError::evidence(format!(
                "input matrix duplicates case or artifact identity {case_name}"
            )));
        }
        bindings.push(InputAnalysisBinding {
            case_name: case_name.to_owned(),
            artifact_identity: analysis.artifact_identity.clone(),
            analysis_path: relative.clone(),
            analysis_sha256: sha256_hex(&analysis_bytes),
            manifest_sha256: sha256_hex(
                &fs::read(run.join("manifest.json"))
                    .usage_context("cannot read bound input manifest")?,
            ),
            events_sha256: sha256_hex(
                &fs::read(run.join("events.jsonl"))
                    .usage_context("cannot read bound input events")?,
            ),
        });
    }
    let first_run = run_by_identity
        .get(&repeatability.first_artifact_identity)
        .ok_or_else(|| AppError::evidence("repeatability first artifact is absent from matrix"))?;
    let second_run = run_by_identity
        .get(&repeatability.second_artifact_identity)
        .ok_or_else(|| AppError::evidence("repeatability second artifact is absent from matrix"))?;
    let replayed_repeatability = compare_input_runs(first_run, second_run, None)?;
    if repeatability != replayed_repeatability
        || !replayed_repeatability
            .checks
            .iter()
            .all(|check| check.passed)
    {
        return Err(AppError::evidence(
            "repeatability report does not match replayed source artifacts",
        ));
    }
    let missing = required
        .iter()
        .filter(|name| !passed.contains(**name))
        .map(|name| (*name).to_owned())
        .collect::<Vec<_>>();
    let decision = InputGateDecision {
        schema_version: INPUT_ARTIFACT_SCHEMA_VERSION,
        status: if missing.is_empty() {
            "PASS"
        } else {
            "BLOCKED_BY_EVIDENCE"
        }
        .to_owned(),
        t0_04_unlocked: missing.is_empty(),
        loop_gate_sha256,
        matrix_sha256: sha256_hex(&matrix_raw),
        repeatability_sha256: sha256_hex(&repeatability_raw),
        required_cases: required.iter().map(|value| (*value).to_owned()).collect(),
        missing_cases: missing,
        analysis_bindings: bindings,
        repeatability_first_artifact_identity: repeatability.first_artifact_identity,
        repeatability_second_artifact_identity: repeatability.second_artifact_identity,
    };
    write_new_json(output, &decision)?;
    Ok(decision)
}

fn reject_passive_gate_evidence(evidence_class: &str) -> AppResult<()> {
    if matches!(
        evidence_class,
        EVIDENCE_PASSIVE_INPUT_RUNTIME | "PASSIVE_RAW_APPLY_RUNTIME"
    ) {
        return Err(AppError::evidence(
            "passive observation evidence is diagnostic-only and cannot populate or unlock the T0-04 input gate",
        ));
    }
    Ok(())
}

pub fn validate_loop_gate_reference(
    loop_gate_path: &Path,
) -> AppResult<ValidatedLoopGateReference> {
    let loop_raw = fs::read(loop_gate_path).usage_context(format!(
        "cannot read loop gate {}",
        loop_gate_path.display()
    ))?;
    let loop_gate: LoopGateReference = serde_json::from_slice(&loop_raw).usage_context(format!(
        "cannot parse loop gate {}",
        loop_gate_path.display()
    ))?;
    let required_loop_scenarios = REQUIRED_LOOP_GATE_SCENARIOS;
    let declared_loop_scenarios = loop_gate
        .required_scenarios
        .iter()
        .map(String::as_str)
        .collect::<BTreeSet<_>>();
    let bound_loop_scenarios = loop_gate
        .analysis_bindings
        .iter()
        .map(|binding| binding.scenario.as_str())
        .collect::<BTreeSet<_>>();
    let expected_loop_scenarios = required_loop_scenarios
        .iter()
        .copied()
        .collect::<BTreeSet<_>>();
    if loop_gate.schema_version != 2
        || loop_gate.status != "PASS"
        || !loop_gate.active_input_allowed
        || !loop_gate.missing_scenarios.is_empty()
        || loop_gate.required_scenarios.len() != required_loop_scenarios.len()
        || loop_gate.analysis_bindings.len() != required_loop_scenarios.len()
        || declared_loop_scenarios != expected_loop_scenarios
        || bound_loop_scenarios != expected_loop_scenarios
    {
        return Err(AppError::evidence(
            "T0-03R loop gate does not bind the complete required scenario set",
        ));
    }
    let loop_gate_sha256 = sha256_hex(&loop_raw);
    let loop_parent = loop_gate_path.parent().unwrap_or_else(|| Path::new("."));
    let mut loop_target_sha256 = None::<String>;
    let mut loop_ledger_sha256 = None::<String>;
    for binding in &loop_gate.analysis_bindings {
        validate_relative_matrix_path(&binding.analysis_path, "loop gate")?;
        let path = loop_parent.join(&binding.analysis_path);
        let bytes = fs::read(&path).usage_context(format!(
            "cannot read bound loop analysis {}",
            path.display()
        ))?;
        if sha256_hex(&bytes) != binding.analysis_sha256 {
            return Err(AppError::evidence(format!(
                "bound loop analysis {} hash mismatch",
                path.display()
            )));
        }
        let stored: crate::loop_verification::LoopAnalysisReport = serde_json::from_slice(&bytes)
            .usage_context(format!(
            "cannot parse bound loop analysis {}",
            path.display()
        ))?;
        let run = path.parent().ok_or_else(|| {
            AppError::usage(format!("bound loop analysis {} has no run", path.display()))
        })?;
        let replayed = crate::loop_verification::analyze_loop_run_internal(run)?;
        if stored != replayed
            || replayed.result != "PASS"
            || !replayed.eligible_for_active_gate
            || replayed.scenario != binding.scenario
            || replayed.manifest_sha256 != binding.manifest_sha256
            || replayed.events_sha256 != binding.events_sha256
            || !replayed.checks.iter().all(|check| check.passed)
        {
            return Err(AppError::evidence(format!(
                "bound loop analysis {} does not replay",
                path.display()
            )));
        }
        if loop_target_sha256.get_or_insert_with(|| replayed.target_sha256.clone())
            != &replayed.target_sha256
            || loop_ledger_sha256.get_or_insert_with(|| replayed.loop_ledger_sha256.clone())
                != &replayed.loop_ledger_sha256
        {
            return Err(AppError::evidence(
                "bound loop analyses mix target or ledger identities",
            ));
        }
    }
    Ok(ValidatedLoopGateReference {
        sha256: loop_gate_sha256,
        target_sha256: loop_target_sha256
            .ok_or_else(|| AppError::evidence("loop gate has no replayed target identity"))?,
        loop_ledger_sha256: loop_ledger_sha256
            .ok_or_else(|| AppError::evidence("loop gate has no replayed ledger identity"))?,
    })
}

fn companion_loop_hashes(
    input_run_directory: &Path,
    relative: &Path,
    target_sha256: &str,
) -> AppResult<(String, String, String)> {
    validate_relative_matrix_path(relative, "companion loop artifact")?;
    let companion = input_run_directory.join(relative);
    let manifest_path = companion.join("manifest.json");
    let events_path = companion.join("events.jsonl");
    let analysis_path = companion.join("analysis.json");
    let manifest_bytes = fs::read(&manifest_path).usage_context(format!(
        "cannot read companion loop manifest {}",
        manifest_path.display()
    ))?;
    let events_bytes = fs::read(&events_path).usage_context(format!(
        "cannot read companion loop events {}",
        events_path.display()
    ))?;
    let analysis_bytes = fs::read(&analysis_path).usage_context(format!(
        "cannot read companion loop analysis {}",
        analysis_path.display()
    ))?;
    let manifest: crate::loop_verification::LoopManifest =
        serde_json::from_slice(&manifest_bytes).usage_context("cannot parse companion manifest")?;
    if manifest.schema_version != 2
        || manifest.state != "COMPLETE"
        || manifest.active_input_allowed
        || manifest.target_sha256 != target_sha256
        || manifest.hooks_mask != crate::loop_ipc::LOOP_HOOK_MASK
        || manifest.overwritten_events != 0
        || !manifest.target_bytes_restored
        || manifest.target_bytes_before_sha256 != manifest.target_bytes_after_sha256
        || manifest.main_thread_id == 0
    {
        return Err(AppError::evidence(
            "companion V5 loop manifest lacks exact complete/restore/32-slot identity",
        ));
    }
    let mut companion_events = Vec::new();
    for (index, line) in events_bytes.split(|byte| *byte == b'\n').enumerate() {
        if line.is_empty() {
            continue;
        }
        companion_events.push(
            serde_json::from_slice::<crate::loop_verification::LoopEvent>(line).usage_context(
                format!("cannot parse companion loop event line {}", index + 1),
            )?,
        );
    }
    if companion_events.is_empty()
        || companion_events
            .iter()
            .enumerate()
            .any(|(index, event)| event.sequence != index as u64 + 1 || event.kind == "FAILURE")
        || !companion_events.iter().any(|event| {
            event.kind == "LOOP_BEGIN"
                && event.thread_id == manifest.main_thread_id
                && event.loop_iteration != 0
        })
    {
        return Err(AppError::evidence(
            "companion V5 loop events are empty, discontinuous, failed, or lack main-thread loop context",
        ));
    }
    let stored: crate::loop_verification::LoopAnalysisReport =
        serde_json::from_slice(&analysis_bytes).usage_context("cannot parse companion analysis")?;
    let replayed = crate::loop_verification::analyze_loop_run_internal(&companion)?;
    if stored != replayed
        || stored.manifest_sha256 != sha256_hex(&manifest_bytes)
        || stored.events_sha256 != sha256_hex(&events_bytes)
        || stored.target_sha256 != target_sha256
    {
        return Err(AppError::evidence(
            "companion V5 loop analysis or source hashes do not replay exactly",
        ));
    }
    Ok((
        sha256_hex(&manifest_bytes),
        sha256_hex(&events_bytes),
        sha256_hex(&analysis_bytes),
    ))
}

fn validate_passive_companion_binding(
    input_run_directory: &Path,
    manifest: &InputManifest,
    input_events: &[InputEvent],
) -> AppResult<()> {
    let relative = manifest
        .companion_loop_artifact_path
        .as_deref()
        .ok_or_else(|| AppError::evidence("passive input manifest lacks companion path"))?;
    let hashes = companion_loop_hashes(input_run_directory, relative, &manifest.target_sha256)?;
    if manifest.companion_loop_manifest_sha256.as_deref() != Some(hashes.0.as_str())
        || manifest.companion_loop_events_sha256.as_deref() != Some(hashes.1.as_str())
        || manifest.companion_loop_analysis_sha256.as_deref() != Some(hashes.2.as_str())
    {
        return Err(AppError::evidence(
            "passive input companion hashes do not match the bound V5 run",
        ));
    }
    let companion = input_run_directory.join(relative);
    let companion_manifest: crate::loop_verification::LoopManifest = serde_json::from_slice(
        &fs::read(companion.join("manifest.json"))
            .usage_context("cannot reread companion loop manifest")?,
    )
    .usage_context("cannot parse companion loop manifest")?;
    if manifest.main_thread_id != Some(companion_manifest.main_thread_id) {
        return Err(AppError::evidence(
            "passive input and companion loop disagree on main thread",
        ));
    }
    let file = fs::File::open(companion.join("events.jsonl"))
        .usage_context("cannot open companion loop events")?;
    let mut companion_iterations = BTreeSet::new();
    for (index, line) in BufReader::new(file).lines().enumerate() {
        let line = line.usage_context(format!(
            "cannot read companion loop event line {}",
            index + 1
        ))?;
        let event: crate::loop_verification::LoopEvent = serde_json::from_str(&line)
            .usage_context(format!(
                "cannot parse companion loop event line {}",
                index + 1
            ))?;
        if event.thread_id == companion_manifest.main_thread_id && event.loop_iteration != 0 {
            companion_iterations.insert(event.loop_iteration);
        }
    }
    if input_events
        .iter()
        .filter(|event| event.stage != "native-lifecycle")
        .any(|event| !companion_iterations.contains(&event.loop_iteration))
    {
        return Err(AppError::evidence(
            "passive input query lacks the same-loop companion V5 context",
        ));
    }
    Ok(())
}

fn validate_relative_matrix_path(path: &Path, label: &str) -> AppResult<()> {
    if path.is_absolute()
        || path.components().any(|component| {
            matches!(
                component,
                std::path::Component::ParentDir
                    | std::path::Component::CurDir
                    | std::path::Component::RootDir
                    | std::path::Component::Prefix(_)
            )
        })
    {
        return Err(AppError::usage(format!(
            "{label} paths must be normalized relative paths"
        )));
    }
    Ok(())
}

fn validate_manifest(manifest: &InputManifest) -> AppResult<()> {
    if manifest.schema_version != INPUT_ARTIFACT_SCHEMA_VERSION
        || manifest.experiment_identity != INPUT_TOOL_IDENTITY
        || manifest.artifact_identity.trim().is_empty()
    {
        return Err(AppError::usage("invalid T0-04 input artifact identity"));
    }
    if !matches!(
        manifest.probe.as_str(),
        "control-no-native" | "passive" | "loopback" | "original-online-disarmed"
    ) {
        return Err(AppError::usage("unknown input probe"));
    }
    validate_case_id(&manifest.case_id)?;
    validate_sha256(&manifest.target_sha256, "target SHA-256")?;
    validate_sha256(&manifest.loop_gate_sha256, "loop gate SHA-256")?;
    manifest.constants.validate()?;
    let expected = requirements_for(
        &manifest.probe,
        &manifest.scenario,
        &manifest.evidence_class,
    )?;
    if manifest.requirements != expected {
        return Err(AppError::evidence(
            "manifest scenario evidence requirements differ from the frozen profile",
        ));
    }
    match manifest.evidence_class.as_str() {
        EVIDENCE_WITNESS_QUERY_ONLY => {
            if manifest.ipc_magic != WITNESS_LOG_SOURCE_FORMAT
                || manifest.arm_token_sha256.is_some()
                || manifest.source_window_min == Some(0)
                || manifest.source_window_max == Some(0)
                || matches!(
                    (manifest.source_window_min, manifest.source_window_max),
                    (Some(minimum), Some(maximum)) if minimum > maximum
                )
                || manifest
                    .source_log_sha256
                    .as_deref()
                    .is_none_or(|hash| validate_sha256(hash, "source log SHA-256").is_err())
                || !passive_runtime_metadata_absent(manifest)
            {
                return Err(AppError::evidence(
                    "witness-query artifact has an invalid read-only source identity",
                ));
            }
        }
        EVIDENCE_DISARMED_CONTROL => {
            let expected_source = if manifest.probe == "control-no-native" {
                CONTROL_NO_IPC_SOURCE_FORMAT
            } else {
                INPUT_IPC_SOURCE_FORMAT
            };
            if manifest.ipc_magic != expected_source
                || manifest.arm_token_sha256.is_some()
                || manifest.source_log_sha256.is_some()
                || manifest.source_window_min.is_some()
                || manifest.source_window_max.is_some()
                || manifest.source_controller_index.is_some()
                || !passive_runtime_metadata_absent(manifest)
            {
                return Err(AppError::evidence(
                    "disarmed-control artifact source identity is invalid",
                ));
            }
        }
        EVIDENCE_PIPELINE_RUNTIME => {
            if manifest.ipc_magic != INPUT_IPC_SOURCE_FORMAT
                || manifest.source_log_sha256.is_some()
                || manifest.source_window_min.is_some()
                || manifest.source_window_max.is_some()
                || manifest.source_controller_index.is_some()
                || !passive_runtime_metadata_absent(manifest)
            {
                return Err(AppError::evidence(
                    "pipeline artifact source identity is invalid",
                ));
            }
        }
        EVIDENCE_PASSIVE_INPUT_RUNTIME => {
            let hashes_valid = [
                manifest.companion_loop_manifest_sha256.as_deref(),
                manifest.companion_loop_events_sha256.as_deref(),
                manifest.companion_loop_analysis_sha256.as_deref(),
                manifest.native_sha256.as_deref(),
                manifest.helper_sha256.as_deref(),
                manifest.target_bytes_before_sha256.as_deref(),
                manifest.target_bytes_after_sha256.as_deref(),
            ]
            .into_iter()
            .all(|hash| hash.is_some_and(|hash| validate_sha256(hash, "bound SHA-256").is_ok()));
            if manifest.ipc_magic != INPUT_IPC_SOURCE_FORMAT
                || manifest.ipc_schema_version != Some(INPUT_IPC_SCHEMA_VERSION)
                || manifest.arm_token_sha256.is_some()
                || manifest.source_log_sha256.is_some()
                || manifest.source_window_min.is_some()
                || manifest.source_window_max.is_some()
                || manifest.source_controller_index.is_some()
                || manifest
                    .companion_loop_artifact_path
                    .as_deref()
                    .is_none_or(|path| {
                        validate_relative_matrix_path(path, "companion loop artifact").is_err()
                    })
                || !hashes_valid
                || manifest.main_thread_id.is_none_or(|thread| thread == 0)
                || manifest.installed_hooks_mask.is_none()
                || manifest.restored_hooks_mask.is_none()
                || manifest.overwritten_events.is_none()
                || manifest.redacted_events.is_none()
                || manifest.producer_closed.is_none()
                || manifest.target_bytes_restored.is_none()
                || manifest.failure_reason.is_some()
            {
                return Err(AppError::evidence(
                    "passive runtime input artifact is missing schema-2 source, companion, hook, or restore bindings",
                ));
            }
        }
        _ => return Err(AppError::usage("unknown input evidence class")),
    }
    if manifest.probe == "loopback" && manifest.arm_token_sha256.is_none() {
        return Err(AppError::evidence(
            "loopback artifact is missing the independent one-time arm token hash",
        ));
    } else if let Some(token) = &manifest.arm_token_sha256 {
        validate_sha256(token, "arm token SHA-256")?;
    }
    Ok(())
}

fn passive_runtime_metadata_absent(manifest: &InputManifest) -> bool {
    manifest.ipc_schema_version.is_none()
        && manifest.companion_loop_artifact_path.is_none()
        && manifest.companion_loop_manifest_sha256.is_none()
        && manifest.companion_loop_events_sha256.is_none()
        && manifest.companion_loop_analysis_sha256.is_none()
        && manifest.native_sha256.is_none()
        && manifest.helper_sha256.is_none()
        && manifest.main_thread_id.is_none()
        && manifest.installed_hooks_mask.is_none()
        && manifest.restored_hooks_mask.is_none()
        && manifest.overwritten_events.is_none()
        && manifest.redacted_events.is_none()
        && manifest.producer_closed.is_none()
        && manifest.target_bytes_before_sha256.is_none()
        && manifest.target_bytes_after_sha256.is_none()
        && manifest.target_bytes_restored.is_none()
        && manifest.failure_reason.is_none()
}

fn validate_case_id(value: &str) -> AppResult<()> {
    if value.is_empty()
        || !value
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_'))
    {
        return Err(AppError::usage("invalid input artifact case id"));
    }
    Ok(())
}

fn validate_sha256(value: &str, label: &str) -> AppResult<()> {
    if value.len() != 64 || !value.bytes().all(|byte| byte.is_ascii_hexdigit()) {
        return Err(AppError::usage(format!("invalid {label}")));
    }
    Ok(())
}

fn validate_state_hex(value: &str) -> bool {
    let Ok(bytes) = hex::decode(value) else {
        return false;
    };
    crate::input_core::InputStateDraftV1::decode(&bytes).is_ok()
}

fn identity(event: &InputEvent) -> Option<Identity> {
    Some(Identity {
        session_id: event.session_id.clone()?,
        client_id: event.client_id?,
        sequence: event.command_sequence?,
    })
}

fn stage_rank(stage: &str) -> Option<u8> {
    Some(match stage {
        "sample" => 0,
        "canonical" => 1,
        "send" => 2,
        "receive" => 3,
        "accept" | "reject" | "duplicate" => 4,
        "resolve" => 5,
        "apply" => 6,
        "witness" => 7,
        _ => return None,
    })
}

fn canonical_trace(events: &[InputEvent]) -> String {
    events
        .iter()
        .filter_map(|event| {
            event.canonical_state_hex.as_ref().map(|state| {
                format!(
                    "{}:{}:{}:{}:{}",
                    event.session_id.as_deref().unwrap_or(""),
                    event.client_id.unwrap_or(0),
                    event.command_sequence.unwrap_or(0),
                    event.target_tick.unwrap_or(0),
                    state.to_ascii_uppercase()
                )
            })
        })
        .collect::<Vec<_>>()
        .join("\n")
}

fn decision_trace(events: &[InputEvent]) -> String {
    events
        .iter()
        .filter_map(|event| {
            event.decision.as_ref().map(|decision| {
                format!(
                    "{}:{}:{}:{}",
                    event.client_id.unwrap_or(0),
                    event.command_sequence.unwrap_or(0),
                    event.target_tick.unwrap_or(0),
                    decision
                )
            })
        })
        .collect::<Vec<_>>()
        .join("\n")
}

fn apply_trace(events: &[InputEvent]) -> String {
    events
        .iter()
        .filter(|event| event.stage == "apply")
        .map(|event| {
            format!(
                "{}:{}:{}:{}:{}",
                event.client_id.unwrap_or(0),
                event.command_sequence.unwrap_or(0),
                event.target_tick.unwrap_or(0),
                event.edge_flags.unwrap_or(0),
                event.canonical_state_hex.as_deref().unwrap_or("")
            )
        })
        .collect::<Vec<_>>()
        .join("\n")
}

fn check(id: &str, passed: bool, detail: &str) -> InputCheck {
    InputCheck {
        id: id.to_owned(),
        passed,
        detail: detail.to_owned(),
    }
}

fn write_new_json<T: Serialize>(path: &Path, value: &T) -> AppResult<()> {
    let bytes = serde_json::to_vec_pretty(value).usage_context("cannot serialize JSON")?;
    let mut file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .usage_context(format!("refusing to overwrite {}", path.display()))?;
    file.write_all(&bytes)
        .usage_context(format!("cannot write {}", path.display()))?;
    file.write_all(b"\n")
        .usage_context(format!("cannot finish {}", path.display()))
}

fn replace_json<T: Serialize>(path: &Path, value: &T) -> AppResult<()> {
    let bytes = serde_json::to_vec_pretty(value).usage_context("cannot serialize JSON")?;
    fs::write(path, bytes).usage_context(format!("cannot update {}", path.display()))
}

fn write_events(path: &Path, events: &[InputEvent]) -> AppResult<()> {
    let mut file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .usage_context(format!("refusing to overwrite {}", path.display()))?;
    for event in events {
        serde_json::to_writer(&mut file, event).usage_context(format!(
            "cannot serialize witness event for {}",
            path.display()
        ))?;
        file.write_all(b"\n")
            .usage_context(format!("cannot write {}", path.display()))?;
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn state(held: u32, edges: u32, aim_x: i16) -> String {
        let mut bytes = [0_u8; 24];
        bytes[0..2].copy_from_slice(&1_u16.to_le_bytes());
        bytes[2..4].copy_from_slice(&24_u16.to_le_bytes());
        bytes[4..8].copy_from_slice(&held.to_le_bytes());
        bytes[8..12].copy_from_slice(&edges.to_le_bytes());
        bytes[16..18].copy_from_slice(&aim_x.to_le_bytes());
        hex::encode_upper(bytes)
    }

    #[test]
    fn strict_state_validation_rejects_unknown_bits_and_shoot_mismatch() {
        assert!(validate_state_hex(&state(1, 0xF, 1)));
        assert!(!validate_state_hex(&state(2, 0, 0)));
        assert!(!validate_state_hex(&state(0, 0, 1)));
        assert!(!validate_state_hex(&state(0, 0x10, 0)));
    }

    #[test]
    fn frozen_constants_validate() {
        InputConstants::default().validate().unwrap();
        let changed = InputConstants {
            max_future_ticks: 65,
            ..InputConstants::default()
        };
        assert!(changed.validate().is_err());
    }

    fn witness_log(actions: &[u32], windows: u64, count: u64, controller: i32) -> String {
        let mut output = format!(
            "[{WITNESS_LOG_IDENTITY}] passive=true actions=0..11 callbackReturn=nil flushRenders=60\n"
        );
        for window in 1..=windows {
            let observations = actions
                .iter()
                .map(|action| format!("a={action}/h=0/c={controller}/n={count}"))
                .collect::<Vec<_>>()
                .join(",");
            let total = actions.len() as u64 * count;
            output.push_str(&format!(
                "[{WITNESS_LOG_IDENTITY}] window={window} renderEnd={} total={total} droppedUnique=0 observations={observations}\n",
                window * 60
            ));
        }
        output
    }

    fn write_control_manifest(directory: &Path) {
        let manifest = InputManifest {
            schema_version: INPUT_ARTIFACT_SCHEMA_VERSION,
            experiment_identity: INPUT_TOOL_IDENTITY.to_owned(),
            case_id: "control-no-native".to_owned(),
            probe: "control-no-native".to_owned(),
            scenario: "control-no-native".to_owned(),
            evidence_class: EVIDENCE_DISARMED_CONTROL.to_owned(),
            endpoint: "local".to_owned(),
            target_sha256: "A".repeat(64),
            loop_gate_sha256: "B".repeat(64),
            artifact_identity: "control-artifact".to_owned(),
            ipc_magic: CONTROL_NO_IPC_SOURCE_FORMAT.to_owned(),
            arm_token_sha256: None,
            constants: InputConstants::default(),
            requirements: disarmed_requirements(),
            source_log_sha256: None,
            source_window_min: None,
            source_window_max: None,
            source_controller_index: None,
            ipc_schema_version: None,
            companion_loop_artifact_path: None,
            companion_loop_manifest_sha256: None,
            companion_loop_events_sha256: None,
            companion_loop_analysis_sha256: None,
            native_sha256: None,
            helper_sha256: None,
            main_thread_id: None,
            installed_hooks_mask: None,
            restored_hooks_mask: None,
            overwritten_events: None,
            redacted_events: None,
            producer_closed: None,
            target_bytes_before_sha256: None,
            target_bytes_after_sha256: None,
            target_bytes_restored: None,
            failure_reason: None,
            state: "COMPLETE".to_owned(),
        };
        write_new_json(&directory.join("manifest.json"), &manifest).unwrap();
    }

    #[test]
    fn empty_events_cannot_vacuously_pass() {
        let directory = tempfile::tempdir().unwrap();
        write_control_manifest(directory.path());
        fs::write(directory.path().join("events.jsonl"), b"").unwrap();
        let report = analyze_input_run(directory.path()).unwrap();
        assert_eq!(report.result, "FAIL");
        assert_eq!(report.event_count, 0);
        assert!(
            report
                .checks
                .iter()
                .any(|check| check.id == "non-empty-events" && !check.passed)
        );
        assert!(!report.eligible_for_active_gate);
    }

    #[test]
    fn internal_gate_replay_does_not_rewrite_analysis() {
        let directory = tempfile::tempdir().unwrap();
        write_control_manifest(directory.path());
        fs::write(directory.path().join("events.jsonl"), b"").unwrap();
        let analysis = directory.path().join("analysis.json");

        assert_eq!(
            analyze_input_run_internal(directory.path()).unwrap().result,
            "FAIL"
        );
        assert!(!analysis.exists());
        analyze_input_run(directory.path()).unwrap();
        assert!(analysis.is_file());
    }

    #[test]
    fn disarmed_control_requires_an_explicit_outside_authority_observation() {
        let directory = tempfile::tempdir().unwrap();
        write_control_manifest(directory.path());
        write_events(
            &directory.path().join("events.jsonl"),
            &[InputEvent {
                event_sequence: 1,
                stage: "control-observation".to_owned(),
                loop_iteration: 0,
                phase: "OUTSIDE_AUTHORITY".to_owned(),
                authority_tick: 0,
                session_id: None,
                client_id: None,
                command_sequence: None,
                target_tick: None,
                canonical_state_hex: None,
                decision: None,
                edge_flags: None,
                world_witness_before: Some("UNCHANGED".to_owned()),
                world_witness_after: Some("UNCHANGED".to_owned()),
                action: None,
                input_hook: None,
                controller_index: None,
                observation_count: None,
                render_end: None,
                native_sequence: None,
                qpc: None,
                thread_id: None,
                native_kind: None,
                flags: None,
                return_rva: None,
                original_calls: None,
                error_code: None,
                device_key: None,
                selected_device_key: None,
                controller_id_raw: None,
                primitive: None,
                value_bits: None,
                classification: None,
            }],
        )
        .unwrap();
        let report = analyze_input_run(directory.path()).unwrap();
        assert_eq!(report.result, "PASS");
        assert_eq!(report.evidence_class, EVIDENCE_DISARMED_CONTROL);
        assert!(report.eligible_for_active_gate);
        assert!(
            requirements_for(
                "original-online-disarmed",
                "original-online-disarmed",
                EVIDENCE_DISARMED_CONTROL,
            )
            .is_ok()
        );
    }

    #[test]
    fn witness_bridge_is_query_only_and_never_active_gate_eligible() {
        let directory = tempfile::tempdir().unwrap();
        let log = directory.path().join("log.txt");
        fs::write(&log, witness_log(&(0..=11).collect::<Vec<_>>(), 1, 1, 0)).unwrap();
        let output = directory.path().join("artifact");
        let imported = import_witness_log(
            &log,
            &output,
            "keyboard",
            "keyboard-passive",
            &"A".repeat(64),
            &"B".repeat(64),
        )
        .unwrap();
        assert_eq!(imported.evidence_class, EVIDENCE_WITNESS_QUERY_ONLY);
        let (manifest, events) = read_input_run(&output).unwrap();
        assert_eq!(manifest.ipc_magic, WITNESS_LOG_SOURCE_FORMAT);
        assert!(events.iter().all(|event| {
            event.canonical_state_hex.is_none()
                && event.session_id.is_none()
                && event.decision.is_none()
                && event.stage != "apply"
        }));
        let report = analyze_input_run(&output).unwrap();
        assert_eq!(report.result, "PASS");
        assert_eq!(report.evidence_class, EVIDENCE_WITNESS_QUERY_ONLY);
        assert!(!report.eligible_for_active_gate);
    }

    #[test]
    fn witness_bridge_filters_run_window_and_controller_without_changing_source_hash() {
        let directory = tempfile::tempdir().unwrap();
        let log = directory.path().join("log.txt");
        let actions = (0..=11).collect::<Vec<_>>();
        let mut text = witness_log(&actions, 1, 1, 1);
        let observations = actions
            .iter()
            .map(|action| format!("a={action}/h=0/c=0/n=1"))
            .collect::<Vec<_>>()
            .join(",");
        text.push_str(&format!(
            "[{WITNESS_LOG_IDENTITY}] window=2 renderEnd=120 total=12 droppedUnique=0 observations={observations}\n"
        ));
        fs::write(&log, text).unwrap();
        let output = directory.path().join("filtered");
        import_witness_log_filtered(
            &log,
            &output,
            "keyboard",
            "keyboard-filtered",
            &"A".repeat(64),
            &"B".repeat(64),
            WitnessImportFilter {
                window_min: Some(2),
                window_max: Some(2),
                controller_index: Some(0),
            },
        )
        .unwrap();
        let (manifest, events) = read_input_run(&output).unwrap();
        assert_eq!(manifest.source_window_min, Some(2));
        assert_eq!(manifest.source_window_max, Some(2));
        assert_eq!(manifest.source_controller_index, Some(0));
        assert!(
            events[1..]
                .iter()
                .all(|event| { event.loop_iteration == 2 && event.controller_index == Some(0) })
        );
        assert_eq!(analyze_input_run(&output).unwrap().result, "PASS");

        let mut tampered = events.clone();
        tampered[1].controller_index = Some(1);
        fs::write(
            output.join("events.jsonl"),
            tampered
                .iter()
                .map(|event| serde_json::to_string(event).unwrap())
                .collect::<Vec<_>>()
                .join("\n"),
        )
        .unwrap();
        let controller_tamper = analyze_input_run(&output).unwrap();
        assert_eq!(controller_tamper.result, "FAIL");
        assert!(
            controller_tamper
                .checks
                .iter()
                .any(|check| check.id == "evidence-scope" && !check.passed)
        );

        tampered[1].controller_index = Some(0);
        tampered[1].loop_iteration = 1;
        fs::write(
            output.join("events.jsonl"),
            tampered
                .iter()
                .map(|event| serde_json::to_string(event).unwrap())
                .collect::<Vec<_>>()
                .join("\n"),
        )
        .unwrap();
        let window_tamper = analyze_input_run(&output).unwrap();
        assert_eq!(window_tamper.result, "FAIL");
        assert!(
            window_tamper
                .checks
                .iter()
                .any(|check| check.id == "evidence-scope" && !check.passed)
        );
    }

    #[test]
    fn witness_scope_rejects_fabricated_canonical_or_apply_fields() {
        let requirements =
            requirements_for("passive", "keyboard", EVIDENCE_WITNESS_QUERY_ONLY).unwrap();
        let manifest = InputManifest {
            schema_version: INPUT_ARTIFACT_SCHEMA_VERSION,
            experiment_identity: INPUT_TOOL_IDENTITY.to_owned(),
            case_id: "keyboard".to_owned(),
            probe: "passive".to_owned(),
            scenario: "keyboard".to_owned(),
            evidence_class: EVIDENCE_WITNESS_QUERY_ONLY.to_owned(),
            endpoint: "local".to_owned(),
            target_sha256: "A".repeat(64),
            loop_gate_sha256: "B".repeat(64),
            artifact_identity: "witness".to_owned(),
            ipc_magic: WITNESS_LOG_SOURCE_FORMAT.to_owned(),
            arm_token_sha256: None,
            constants: InputConstants::default(),
            requirements,
            source_log_sha256: Some("C".repeat(64)),
            source_window_min: None,
            source_window_max: None,
            source_controller_index: None,
            ipc_schema_version: None,
            companion_loop_artifact_path: None,
            companion_loop_manifest_sha256: None,
            companion_loop_events_sha256: None,
            companion_loop_analysis_sha256: None,
            native_sha256: None,
            helper_sha256: None,
            main_thread_id: None,
            installed_hooks_mask: None,
            restored_hooks_mask: None,
            overwritten_events: None,
            redacted_events: None,
            producer_closed: None,
            target_bytes_before_sha256: None,
            target_bytes_after_sha256: None,
            target_bytes_restored: None,
            failure_reason: None,
            state: "COMPLETE".to_owned(),
        };
        let mut events =
            parse_witness_log(&witness_log(&(0..=11).collect::<Vec<_>>(), 1, 1, 0)).unwrap();
        events[1].canonical_state_hex = Some(state(0, 0, 0));
        assert!(!evidence_scope_valid(&manifest, &events));
        events[1].canonical_state_hex = None;
        events[1].stage = "apply".to_owned();
        assert!(!evidence_scope_valid(&manifest, &events));
    }

    #[test]
    fn passive_scenario_missing_required_action_fails() {
        let directory = tempfile::tempdir().unwrap();
        let log = directory.path().join("log.txt");
        fs::write(&log, witness_log(&(0..=10).collect::<Vec<_>>(), 1, 1, 0)).unwrap();
        let output = directory.path().join("artifact");
        import_witness_log(
            &log,
            &output,
            "keyboard",
            "keyboard-missing-drop",
            &"A".repeat(64),
            &"B".repeat(64),
        )
        .unwrap();
        let report = analyze_input_run(&output).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(
            report
                .checks
                .iter()
                .any(|check| check.id == "scenario-minimum-evidence" && !check.passed)
        );
    }

    #[test]
    fn rapid_tap_pause_room_menu_profiles_are_scenario_specific() {
        let rapid = requirements_for("passive", "rapid-tap", EVIDENCE_WITNESS_QUERY_ONLY).unwrap();
        assert_eq!(rapid.required_actions, vec![8, 9, 10, 11]);
        assert_eq!(rapid.minimum_windows, 2);
        assert_eq!(rapid.minimum_observations_per_required_action, 2);
        for scenario in ["pause", "room", "menu"] {
            let profile =
                requirements_for("passive", scenario, EVIDENCE_WITNESS_QUERY_ONLY).unwrap();
            assert_eq!(profile.minimum_windows, 2);
            assert_eq!(profile.required_actions, (0..=11).collect::<Vec<_>>());
        }
        assert_eq!(
            requirements_for("passive", "menu", EVIDENCE_WITNESS_QUERY_ONLY)
                .unwrap()
                .controller_scope,
            "ENTITYLESS"
        );
    }

    #[test]
    fn witness_parser_rejects_dropped_or_ambiguous_contract_records() {
        let duplicate_contract = format!(
            "{}{}",
            witness_log(&[0], 1, 1, 0),
            witness_log(&[1], 1, 1, 0)
        );
        assert!(parse_witness_log(&duplicate_contract).is_err());
        let dropped = witness_log(&[0], 1, 1, 0).replace("droppedUnique=0", "droppedUnique=1");
        assert!(parse_witness_log(&dropped).is_err());
    }

    #[test]
    fn witness_repeatability_cannot_unlock_active_gate() {
        let directory = tempfile::tempdir().unwrap();
        let loop_gate = directory.path().join("loop-gate.json");
        fs::write(
            &loop_gate,
            serde_json::to_vec(&serde_json::json!({
                "status": "PASS",
                "activeInputAllowed": true
            }))
            .unwrap(),
        )
        .unwrap();
        let repeatability = directory.path().join("repeatability.json");
        let repeat = InputComparisonReport {
            schema_version: INPUT_ARTIFACT_SCHEMA_VERSION,
            result: "PASS".to_owned(),
            evidence_class: EVIDENCE_WITNESS_QUERY_ONLY.to_owned(),
            eligible_for_active_gate: false,
            first_artifact_identity: "a".to_owned(),
            second_artifact_identity: "b".to_owned(),
            checks: Vec::new(),
        };
        fs::write(&repeatability, serde_json::to_vec(&repeat).unwrap()).unwrap();
        let matrix = directory.path().join("matrix.json");
        fs::write(&matrix, br#"{"analyses":[]}"#).unwrap();
        assert!(
            write_input_gate(
                &loop_gate,
                &matrix,
                &repeatability,
                &directory.path().join("gate.json"),
            )
            .is_err()
        );
    }

    #[test]
    fn partial_loop_gate_cannot_unlock_input_gate() {
        let directory = tempfile::tempdir().unwrap();
        let loop_gate = directory.path().join("loop-gate.json");
        fs::write(
            &loop_gate,
            serde_json::to_vec(&serde_json::json!({
                "schemaVersion": 2,
                "status": "PASS",
                "activeInputAllowed": true,
                "requiredScenarios": ["keyboard"],
                "missingScenarios": [],
                "analysisBindings": [{
                    "scenario": "keyboard",
                    "analysisPath": "missing/analysis.json",
                    "analysisSha256": "A".repeat(64),
                    "manifestSha256": "B".repeat(64),
                    "eventsSha256": "C".repeat(64)
                }]
            }))
            .unwrap(),
        )
        .unwrap();
        let matrix = directory.path().join("matrix.json");
        fs::write(&matrix, br#"{"analyses":[]}"#).unwrap();
        let repeatability = directory.path().join("repeatability.json");
        fs::write(&repeatability, b"{}").unwrap();

        let error = write_input_gate(
            &loop_gate,
            &matrix,
            &repeatability,
            &directory.path().join("gate.json"),
        )
        .unwrap_err();
        assert!(error.to_string().contains("complete required scenario set"));
    }

    #[test]
    fn input_gate_keeps_input_semantics_separate_from_loop_envelope() {
        assert_eq!(
            REQUIRED_LOOP_GATE_SCENARIOS,
            &[
                "keyboard",
                "controller",
                "device-switch",
                "pause",
                "loading",
                "room",
                "menu",
            ]
        );
        assert!(!REQUIRED_LOOP_GATE_SCENARIOS.contains(&"simultaneous"));
        assert!(!REQUIRED_LOOP_GATE_SCENARIOS.contains(&"rapid-tap"));
        assert!(REQUIRED_INPUT_GATE_SCENARIOS.contains(&"simultaneous"));
        assert!(REQUIRED_INPUT_GATE_SCENARIOS.contains(&"rapid-tap"));
        assert!(!REQUIRED_LOOP_GATE_SCENARIOS.contains(&"focus"));
        assert!(!REQUIRED_INPUT_GATE_SCENARIOS.contains(&"focus"));
    }

    fn passive_manifest_for_test(ipc_schema_version: u32) -> InputManifest {
        InputManifest {
            schema_version: INPUT_ARTIFACT_SCHEMA_VERSION,
            experiment_identity: INPUT_TOOL_IDENTITY.to_owned(),
            case_id: "passive-runtime".to_owned(),
            probe: "passive".to_owned(),
            scenario: "keyboard".to_owned(),
            evidence_class: EVIDENCE_PASSIVE_INPUT_RUNTIME.to_owned(),
            endpoint: "local".to_owned(),
            target_sha256: "A".repeat(64),
            loop_gate_sha256: "B".repeat(64),
            artifact_identity: "passive-runtime-artifact".to_owned(),
            ipc_magic: INPUT_IPC_SOURCE_FORMAT.to_owned(),
            arm_token_sha256: None,
            constants: InputConstants::default(),
            requirements: requirements_for("passive", "keyboard", EVIDENCE_PASSIVE_INPUT_RUNTIME)
                .unwrap(),
            source_log_sha256: None,
            source_window_min: None,
            source_window_max: None,
            source_controller_index: None,
            ipc_schema_version: Some(ipc_schema_version),
            companion_loop_artifact_path: Some(PathBuf::from("loop-context")),
            companion_loop_manifest_sha256: Some("C".repeat(64)),
            companion_loop_events_sha256: Some("D".repeat(64)),
            companion_loop_analysis_sha256: Some("E".repeat(64)),
            native_sha256: Some("F".repeat(64)),
            helper_sha256: Some("1".repeat(64)),
            main_thread_id: Some(1),
            installed_hooks_mask: Some(INPUT_HOOK_MASK),
            restored_hooks_mask: Some(INPUT_HOOK_MASK),
            overwritten_events: Some(0),
            redacted_events: Some(0),
            producer_closed: Some(true),
            target_bytes_before_sha256: Some("2".repeat(64)),
            target_bytes_after_sha256: Some("2".repeat(64)),
            target_bytes_restored: Some(true),
            failure_reason: None,
            state: "COMPLETE".to_owned(),
        }
    }

    fn write_minimal_companion_loop(input_run: &Path) {
        let companion = input_run.join("loop-context");
        fs::create_dir(&companion).unwrap();
        let target_bytes = b"{}";
        fs::write(companion.join("target-bytes-before.json"), target_bytes).unwrap();
        fs::write(companion.join("target-bytes-after.json"), target_bytes).unwrap();
        let target_bytes_sha = sha256_hex(target_bytes);
        let manifest = crate::loop_verification::LoopManifest {
            schema_version: 2,
            experiment_identity: crate::loop_verification::LOOP_TOOL_IDENTITY.to_owned(),
            evidence_class: crate::loop_verification::LOOP_EVIDENCE_CLASS.to_owned(),
            case_id: "companion".to_owned(),
            probe: "passive".to_owned(),
            scenario: "simultaneous".to_owned(),
            target_sha256: "A".repeat(64),
            loop_ledger_sha256: "9".repeat(64),
            state: "COMPLETE".to_owned(),
            active_input_allowed: false,
            native_sha256: "8".repeat(64),
            helper_sha256: "7".repeat(64),
            main_thread_id: 7,
            hooks_mask: crate::loop_ipc::LOOP_HOOK_MASK,
            overwritten_events: 0,
            target_bytes_before_sha256: target_bytes_sha.clone(),
            target_bytes_after_sha256: target_bytes_sha,
            target_bytes_restored: true,
        };
        write_new_json(&companion.join("manifest.json"), &manifest).unwrap();
        let event = |sequence: u64, loop_iteration: u64, phase: &str, kind: &str, flags: u32| {
            crate::loop_verification::LoopEvent {
                sequence,
                native_sequence: sequence,
                qpc: sequence as i64,
                loop_iteration,
                phase: phase.to_owned(),
                kind: kind.to_owned(),
                thread_id: 7,
                action: None,
                value_bits: None,
                flags,
                return_rva: "0x00000000".to_owned(),
                original_calls: 0,
                error_code: 0,
                device_key: None,
                device_kind: None,
                install_failure_stage: None,
                install_failure_index: None,
                install_failure_win32_error: None,
            }
        };
        let events = vec![
            event(1, 0, "OUTSIDE_ENVELOPE", "NATIVE_INITIALIZED", 0),
            event(2, 0, "OUTSIDE_ENVELOPE", "HOOKS_INSTALLED", 0),
            event(
                3,
                1,
                "LOOP_BOUNDARY",
                "LOOP_BEGIN",
                crate::loop_ipc::LOOP_FLAG_LOOP_KNOWN | crate::loop_ipc::LOOP_FLAG_MAIN_THREAD,
            ),
            event(4, 0, "OUTSIDE_ENVELOPE", "HOOKS_RESTORED", 0),
        ];
        write_events_as_loop(&companion.join("events.jsonl"), &events);
        let analysis = crate::loop_verification::analyze_loop_run_internal(&companion).unwrap();
        write_new_json(&companion.join("analysis.json"), &analysis).unwrap();
    }

    fn write_events_as_loop(path: &Path, events: &[crate::loop_verification::LoopEvent]) {
        let mut file = OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(path)
            .unwrap();
        for event in events {
            serde_json::to_writer(&mut file, event).unwrap();
            file.write_all(b"\n").unwrap();
        }
    }

    fn native_record(kind: InputNativeEventKind, action: u32) -> NativeInputEventRecord {
        NativeInputEventRecord {
            sequence: 1,
            event: crate::input_ipc::NativeInputEvent {
                published_sequence: 1,
                qpc: 10,
                loop_iteration: 1,
                thread_id: 7,
                kind: kind as u32,
                phase: LoopPhase::InputQuery as u32,
                flags: INPUT_FLAG_LOOP_KNOWN | INPUT_FLAG_MAIN_THREAD | INPUT_FLAG_KEYBOARD_DEVICE,
                return_rva: 0x100,
                original_calls: 0,
                device_key: 0,
                controller_id: INPUT_NO_CONTROLLER_ID,
                selected_device_key: INPUT_NO_DEVICE_KEY,
                action,
                primitive: INPUT_NO_PRIMITIVE,
                value_bits: 0,
                classification: InputClassification::ExplicitGameplay as u32,
                error_code: 0,
            },
        }
    }

    fn resequence(record: &mut NativeInputEventRecord, sequence: u64, qpc: i64) {
        record.sequence = sequence;
        record.event.published_sequence = sequence as i64;
        record.event.qpc = qpc;
    }

    fn exit_record(
        enter: &NativeInputEventRecord,
        kind: InputNativeEventKind,
        sequence: u64,
        qpc: i64,
    ) -> NativeInputEventRecord {
        let mut exit = enter.clone();
        resequence(&mut exit, sequence, qpc);
        exit.event.kind = kind as u32;
        exit.event.original_calls = 1;
        exit.event.flags |= INPUT_FLAG_ORIGINAL_CALLED;
        if !matches!(kind, InputNativeEventKind::LogicalSetterExit) {
            exit.event.flags |= INPUT_FLAG_VALUE_VALID;
            exit.event.value_bits = 0;
        }
        exit
    }

    fn lifecycle_record(
        kind: InputNativeEventKind,
        sequence: u64,
        error_code: u32,
    ) -> NativeInputEventRecord {
        NativeInputEventRecord {
            sequence,
            event: crate::input_ipc::NativeInputEvent {
                published_sequence: sequence as i64,
                qpc: sequence as i64,
                loop_iteration: 0,
                thread_id: 7,
                kind: kind as u32,
                phase: LoopPhase::OutsideEnvelope as u32,
                flags: 0,
                return_rva: 0,
                original_calls: 0,
                device_key: INPUT_NO_DEVICE_KEY,
                controller_id: INPUT_NO_CONTROLLER_ID,
                selected_device_key: INPUT_NO_DEVICE_KEY,
                action: INPUT_NO_ACTION,
                primitive: INPUT_NO_PRIMITIVE,
                value_bits: 0,
                classification: InputClassification::Unknown as u32,
                error_code,
            },
        }
    }

    fn approved_target_bytes(target_sha256: &str) -> PassiveInputTargetBytesDocument {
        PassiveInputTargetBytesDocument {
            schema_version: PASSIVE_INPUT_TARGET_BYTES_SCHEMA_VERSION,
            target_sha256: target_sha256.to_owned(),
            slots: PASSIVE_INPUT_TARGET_SLOTS
                .iter()
                .map(|(id, rva)| PassiveInputTargetSlot {
                    id: (*id).to_owned(),
                    rva: format!("0x{rva:08X}"),
                    bytes_hex: "AABBCCDD".to_owned(),
                })
                .collect(),
        }
    }

    fn passive_run_config(
        target_bytes_before: PassiveInputTargetBytesDocument,
    ) -> PassiveInputRunConfig {
        PassiveInputRunConfig {
            case_id: "passive-runtime".to_owned(),
            scenario: "keyboard".to_owned(),
            target_sha256: target_bytes_before.target_sha256.clone(),
            loop_gate_sha256: "B".repeat(64),
            artifact_identity: "passive-runtime-artifact".to_owned(),
            companion_loop_artifact_path: PathBuf::from("loop-context"),
            native_sha256: "F".repeat(64),
            helper_sha256: "1".repeat(64),
            target_bytes_before,
        }
    }

    fn passive_finish() -> PassiveInputFinish {
        PassiveInputFinish {
            main_thread_id: 7,
            installed_hooks_mask: INPUT_HOOK_MASK,
            restored_hooks_mask: INPUT_HOOK_MASK,
            overwritten_events: 0,
            redacted_events: 0,
            producer_closed: true,
        }
    }

    #[test]
    fn passive_runtime_old_ipc_schema_fails_closed() {
        assert!(validate_manifest(&passive_manifest_for_test(1)).is_err());
        assert!(validate_manifest(&passive_manifest_for_test(INPUT_IPC_SCHEMA_VERSION)).is_ok());
    }

    #[test]
    fn passive_runtime_native_conversion_rejects_action_12_and_redacted_classes() {
        let valid = input_event_from_native(
            &native_record(InputNativeEventKind::LogicalPressedEnter, 0),
            1,
        )
        .unwrap();
        assert_eq!(valid.stage, "passive-logical-query");
        assert_eq!(valid.controller_id_raw, Some(INPUT_NO_CONTROLLER_ID));
        assert!(valid.canonical_state_hex.is_none());
        assert!(valid.session_id.is_none());
        assert!(valid.world_witness_before.is_none());

        assert!(
            input_event_from_native(
                &native_record(InputNativeEventKind::LogicalPressedEnter, 12),
                1,
            )
            .is_err()
        );
        let mut redacted = native_record(InputNativeEventKind::LogicalPressedEnter, 0);
        redacted.event.classification = InputClassification::NoMatch as u32;
        assert!(input_event_from_native(&redacted, 1).is_err());
    }

    #[test]
    fn passive_runtime_pairs_are_per_thread_balanced_lifo() {
        let mut outer_enter = native_record(InputNativeEventKind::LogicalPressedEnter, 0);
        resequence(&mut outer_enter, 1, 40);
        let mut inner_enter = native_record(InputNativeEventKind::LogicalTriggeredEnter, 1);
        resequence(&mut inner_enter, 2, 50);
        inner_enter.event.return_rva = 0x104;
        let inner_exit = exit_record(
            &inner_enter,
            InputNativeEventKind::LogicalTriggeredExit,
            3,
            51,
        );
        let outer_exit = exit_record(
            &outer_enter,
            InputNativeEventKind::LogicalPressedExit,
            4,
            41,
        );
        let events = [&outer_enter, &inner_enter, &inner_exit, &outer_exit]
            .into_iter()
            .enumerate()
            .map(|(index, record)| input_event_from_native(record, index as u64 + 1).unwrap())
            .collect::<Vec<_>>();
        assert!(passive_native_pairs_valid(&events));

        let wrong_lifo = vec![
            events[0].clone(),
            events[1].clone(),
            events[3].clone(),
            events[2].clone(),
        ];
        assert!(!passive_native_pairs_valid(&wrong_lifo));

        let mut field_mismatch = events.clone();
        field_mismatch[3].selected_device_key = Some(3);
        assert!(!passive_native_pairs_valid(&field_mismatch));

        let mut loop_mismatch = events.clone();
        loop_mismatch[3].loop_iteration += 1;
        assert!(!passive_native_pairs_valid(&loop_mismatch));
        let mut phase_mismatch = events.clone();
        phase_mismatch[3].phase = "GAMEPLAY_UPDATE".to_owned();
        assert!(!passive_native_pairs_valid(&phase_mismatch));
        let mut qpc_reversed = events.clone();
        qpc_reversed[3].qpc = Some(39);
        assert!(!passive_native_pairs_valid(&qpc_reversed));
        let mut device_class_mismatch = events.clone();
        device_class_mismatch[3].flags = device_class_mismatch[3]
            .flags
            .map(|flags| flags ^ (INPUT_FLAG_KEYBOARD_DEVICE | INPUT_FLAG_GAMEPAD_DEVICE));
        assert!(!passive_native_pairs_valid(&device_class_mismatch));
    }

    #[test]
    fn passive_runtime_result_payload_flags_are_strict() {
        let enter = native_record(InputNativeEventKind::LogicalPressedEnter, 0);
        let false_exit = exit_record(&enter, InputNativeEventKind::LogicalPressedExit, 2, 11);
        assert!(input_event_from_native(&false_exit, 2).is_ok());

        let mut true_exit = false_exit.clone();
        true_exit.event.value_bits = 1;
        true_exit.event.flags |= INPUT_FLAG_RESULT_TRUE;
        assert!(input_event_from_native(&true_exit, 2).is_ok());
        let mut bool_mismatch = true_exit.clone();
        bool_mismatch.event.flags &= !INPUT_FLAG_RESULT_TRUE;
        assert!(input_event_from_native(&bool_mismatch, 2).is_err());
        let mut invalid_bool = true_exit.clone();
        invalid_bool.event.value_bits = 2;
        assert!(input_event_from_native(&invalid_bool, 2).is_err());

        let mut scalar_enter = native_record(InputNativeEventKind::LogicalScalarEnter, 1);
        resequence(&mut scalar_enter, 3, 20);
        let mut negative_zero = exit_record(
            &scalar_enter,
            InputNativeEventKind::LogicalScalarExit,
            4,
            21,
        );
        negative_zero.event.value_bits = (-0.0_f32).to_bits();
        assert!(input_event_from_native(&negative_zero, 4).is_ok());
        let mut nonzero = negative_zero.clone();
        nonzero.event.value_bits = 1.0_f32.to_bits();
        nonzero.event.flags |= INPUT_FLAG_RESULT_TRUE;
        assert!(input_event_from_native(&nonzero, 4).is_ok());
        nonzero.event.flags &= !INPUT_FLAG_RESULT_TRUE;
        assert!(input_event_from_native(&nonzero, 4).is_err());

        let mut polluted_enter = enter.clone();
        polluted_enter.event.flags |= INPUT_FLAG_VALUE_VALID;
        assert!(input_event_from_native(&polluted_enter, 1).is_err());
        polluted_enter.event.flags = enter.event.flags | INPUT_FLAG_RESULT_TRUE;
        assert!(input_event_from_native(&polluted_enter, 1).is_err());

        let mut setter = native_record(InputNativeEventKind::LogicalSetterEnter, 2);
        setter.event.classification = InputClassification::LogicalOverrideDiagnostic as u32;
        setter.event.flags |= INPUT_FLAG_VALUE_VALID | INPUT_FLAG_RESULT_TRUE;
        assert!(input_event_from_native(&setter, 1).is_err());
    }

    #[test]
    fn passive_runtime_setter_pair_requires_equal_value_bits() {
        let mut enter = native_record(InputNativeEventKind::LogicalSetterEnter, 2);
        enter.event.classification = InputClassification::LogicalOverrideDiagnostic as u32;
        enter.event.flags |= INPUT_FLAG_VALUE_VALID;
        enter.event.value_bits = 0x3F80_0000;
        let exit = exit_record(&enter, InputNativeEventKind::LogicalSetterExit, 2, 11);
        let mut events = vec![
            input_event_from_native(&enter, 1).unwrap(),
            input_event_from_native(&exit, 2).unwrap(),
        ];
        assert!(passive_native_pairs_valid(&events));
        events[1].value_bits = Some(0);
        assert!(!passive_native_pairs_valid(&events));
    }

    #[test]
    fn passive_runtime_lifecycle_is_payload_empty_and_failure_is_nonzero() {
        let initialized = lifecycle_record(InputNativeEventKind::NativeInitialized, 1, 0);
        let initialized_event = input_event_from_native(&initialized, 1).unwrap();
        assert!(evidence_scope_valid(
            &passive_manifest_for_test(INPUT_IPC_SCHEMA_VERSION),
            std::slice::from_ref(&initialized_event),
        ));

        let mut payload_lifecycle = initialized.clone();
        payload_lifecycle.event.flags = INPUT_FLAG_LOOP_KNOWN;
        assert!(input_event_from_native(&payload_lifecycle, 1).is_err());
        let zero_error_failure = lifecycle_record(InputNativeEventKind::Failure, 2, 0);
        assert!(input_event_from_native(&zero_error_failure, 2).is_err());
        let failure = lifecycle_record(InputNativeEventKind::Failure, 2, 9);
        let failure_event = input_event_from_native(&failure, 2).unwrap();
        assert_eq!(failure_event.error_code, Some(9));
        assert!(evidence_scope_valid(
            &passive_manifest_for_test(INPUT_IPC_SCHEMA_VERSION),
            std::slice::from_ref(&failure_event),
        ));
    }

    #[test]
    fn passive_runtime_lifecycle_brackets_all_queries_exactly_once() {
        let records = [
            lifecycle_record(InputNativeEventKind::NativeInitialized, 1, 0),
            lifecycle_record(InputNativeEventKind::HooksInstalled, 2, 0),
            native_record(InputNativeEventKind::LogicalPressedEnter, 0),
            native_record(InputNativeEventKind::LogicalPressedExit, 0),
            lifecycle_record(InputNativeEventKind::HooksRestored, 5, 0),
            lifecycle_record(InputNativeEventKind::NativeRetired, 6, 0),
        ];
        let mut events = records
            .iter()
            .enumerate()
            .map(|(index, record)| {
                let mut record = record.clone();
                resequence(&mut record, index as u64 + 1, index as i64 + 1);
                if index == 3 {
                    record.event.original_calls = 1;
                    record.event.flags |= INPUT_FLAG_ORIGINAL_CALLED | INPUT_FLAG_VALUE_VALID;
                }
                input_event_from_native(&record, index as u64 + 1).unwrap()
            })
            .collect::<Vec<_>>();
        assert!(passive_native_lifecycle_valid(&events));

        events.swap(1, 2);
        assert!(!passive_native_lifecycle_valid(&events));
        events.swap(1, 2);
        let mut outside_lifecycle_qpc = events.clone();
        outside_lifecycle_qpc[2].qpc = Some(7);
        assert!(!passive_native_lifecycle_valid(&outside_lifecycle_qpc));
        events.push(events.last().unwrap().clone());
        assert!(!passive_native_lifecycle_valid(&events));
    }

    #[test]
    fn passive_target_bytes_require_exact_approved_order_and_four_bytes() {
        let target = "A".repeat(64);
        let approved = approved_target_bytes(&target);
        assert!(approved.validate(&target).is_ok());

        let mut reordered = approved.clone();
        reordered.slots.swap(0, 1);
        assert!(reordered.validate(&target).is_err());
        let mut wrong_size = approved.clone();
        wrong_size.slots[0].bytes_hex = "AABBCC".to_owned();
        assert!(wrong_size.validate(&target).is_err());
        let mut extra = approved.clone();
        extra.slots.push(extra.slots[0].clone());
        assert!(extra.validate(&target).is_err());
    }

    #[test]
    fn passive_runtime_writer_analyzes_non_global_qpc_order_and_failure_never_passes() {
        let directory = tempfile::tempdir().unwrap();
        let output = directory.path().join("passive-run");
        let target_bytes = approved_target_bytes(&"A".repeat(64));
        let mut run =
            PassiveInputRun::create(&output, passive_run_config(target_bytes.clone())).unwrap();
        write_minimal_companion_loop(&output);

        let initialized = lifecycle_record(InputNativeEventKind::NativeInitialized, 1, 0);
        let installed = lifecycle_record(InputNativeEventKind::HooksInstalled, 2, 0);
        let mut logical_enter = native_record(InputNativeEventKind::LogicalPressedEnter, 0);
        resequence(&mut logical_enter, 3, 20);
        let logical_exit = exit_record(
            &logical_enter,
            InputNativeEventKind::LogicalPressedExit,
            4,
            21,
        );
        let mut manager_enter = native_record(InputNativeEventKind::ManagerPressedEnter, 0);
        resequence(&mut manager_enter, 5, 10);
        manager_enter.event.primitive = 0x41;
        manager_enter.event.classification = InputClassification::UniqueGameplay as u32;
        let manager_exit = exit_record(
            &manager_enter,
            InputNativeEventKind::ManagerPressedExit,
            6,
            22,
        );
        let mut restored = lifecycle_record(InputNativeEventKind::HooksRestored, 7, 0);
        restored.event.qpc = 30;
        let mut retired = lifecycle_record(InputNativeEventKind::NativeRetired, 8, 0);
        retired.event.qpc = 31;
        for record in [
            &initialized,
            &installed,
            &logical_enter,
            &logical_exit,
            &manager_enter,
            &manager_exit,
            &restored,
            &retired,
        ] {
            run.append_native(record).unwrap();
        }
        let report = run.finish(&target_bytes, passive_finish()).unwrap();
        assert_eq!(report.result, "PASS");
        assert!(!report.eligible_for_active_gate);

        let (_, mut events) = read_input_run(&output).unwrap();
        events[7].native_kind = Some("FAILURE".to_owned());
        events[7].error_code = Some(9);
        fs::write(
            output.join("events.jsonl"),
            events
                .iter()
                .map(|event| serde_json::to_string(event).unwrap())
                .collect::<Vec<_>>()
                .join("\n"),
        )
        .unwrap();
        let failed = analyze_input_run(&output).unwrap();
        assert_eq!(failed.result, "FAIL");
        assert!(
            failed
                .checks
                .iter()
                .any(|check| { check.id == "passive-runtime-integrity" && !check.passed })
        );
    }

    #[test]
    fn passive_runtime_failed_writer_does_not_require_complete_companion() {
        let directory = tempfile::tempdir().unwrap();
        let output = directory.path().join("failed-passive-run");
        let target_bytes = approved_target_bytes(&"A".repeat(64));
        let run =
            PassiveInputRun::create(&output, passive_run_config(target_bytes.clone())).unwrap();
        run.fail("installation failed", Some(&target_bytes), passive_finish())
            .unwrap();
        let manifest: InputManifest =
            serde_json::from_slice(&fs::read(output.join("manifest.json")).unwrap()).unwrap();
        assert_eq!(manifest.state, "FAILED");
        assert_eq!(
            manifest.failure_reason.as_deref(),
            Some("installation failed")
        );
        assert_eq!(manifest.target_bytes_restored, Some(true));
        assert!(manifest.companion_loop_manifest_sha256.is_none());
        assert!(manifest.companion_loop_events_sha256.is_none());
        assert!(manifest.companion_loop_analysis_sha256.is_none());
    }

    #[test]
    fn passive_runtime_requirements_are_safety_not_action_coverage() {
        let requirements =
            requirements_for("passive", "keyboard", EVIDENCE_PASSIVE_INPUT_RUNTIME).unwrap();
        assert_eq!(requirements.minimum_events, 4);
        assert_eq!(requirements.minimum_windows, 1);
        assert_eq!(requirements.required_phases, vec!["OUTSIDE_ENVELOPE"]);
        assert_eq!(requirements.required_stages, vec!["native-lifecycle"]);
        assert!(requirements.required_actions.is_empty());
    }

    #[test]
    fn passive_companion_paths_are_nested_normalized_and_hash_bound() {
        assert!(validate_relative_matrix_path(Path::new("loop-context"), "companion").is_ok());
        assert!(validate_relative_matrix_path(Path::new("../loop-context"), "companion").is_err());
        let directory = tempfile::tempdir().unwrap();
        write_minimal_companion_loop(directory.path());
        let hashes =
            companion_loop_hashes(directory.path(), Path::new("loop-context"), &"A".repeat(64))
                .unwrap();
        let mut manifest = passive_manifest_for_test(INPUT_IPC_SCHEMA_VERSION);
        manifest.companion_loop_manifest_sha256 = Some(hashes.0);
        manifest.companion_loop_events_sha256 = Some(hashes.1);
        manifest.companion_loop_analysis_sha256 = Some(hashes.2);
        manifest.main_thread_id = Some(7);
        assert!(validate_passive_companion_binding(directory.path(), &manifest, &[]).is_ok());

        fs::write(
            directory.path().join("loop-context/events.jsonl"),
            b"tampered\n",
        )
        .unwrap();
        assert!(validate_passive_companion_binding(directory.path(), &manifest, &[]).is_err());
    }

    #[test]
    fn passive_runtime_is_explicitly_rejected_by_input_gate() {
        assert!(reject_passive_gate_evidence(EVIDENCE_PASSIVE_INPUT_RUNTIME).is_err());
        assert!(reject_passive_gate_evidence("PASSIVE_RAW_APPLY_RUNTIME").is_err());
        assert!(reject_passive_gate_evidence(EVIDENCE_PIPELINE_RUNTIME).is_ok());
    }
}
