use std::collections::{BTreeMap, BTreeSet};
use std::fs::{self, OpenOptions};
use std::io::{BufRead, BufReader, BufWriter, Write};
use std::path::{Component, Path, PathBuf};

use serde::{Deserialize, Serialize};

use crate::active_catalog::{
    ACTIVE_SCENARIOS, ACTIVE_SLOT_COUNT, FIXED_TARGET_SHA256, ValidatedActivePlan,
    expected_hook_slots, validate_active_plan,
};
use crate::active_ipc::{
    ACTIVE_FLAG_CONFLICTING_STATE, ACTIVE_FLAG_QUERY_OVERRIDDEN, ACTIVE_KNOWN_FLAGS,
    ACTIVE_NO_ACTION, ActiveConsumer, ActiveDecision, ActiveErrorCode, ActiveEventKind,
    ActiveLifecycleKind, ActivePhase, ActiveQueryKind, ActiveRejectReason, ActiveSourceDevice,
    ActiveWitnessKind, NativeActiveEventRecord,
};
use crate::active_verification::{
    ACTIVE_RUNTIME_CONTRACT_COMPLETE, ACTIVE_SEMANTIC_CONTRACT_COMPLETE,
    ValidatedActiveVerification, read_runtime_active_verification_v2,
};
use crate::artifact::file_sha256;
use crate::error::{AppError, AppResult, UsageContext};
use crate::input_artifact::{EVIDENCE_DISARMED_CONTROL, EVIDENCE_PIPELINE_RUNTIME};
use crate::input_core::{INPUT_FUTURE_WINDOW, INPUT_LEAD_TICKS, InputStateDraftV1};
use crate::loop_ipc::LOOP_HOOK_MASK;
use crate::loop_verification::{
    LOOP_EVIDENCE_CLASS, LOOP_TOOL_IDENTITY, LoopAnalysisReport, LoopEvent, LoopManifest,
    replay_loop_run,
};
use crate::pe::sha256_hex;

pub const ACTIVE_ARTIFACT_SCHEMA_VERSION: u32 = 4;
pub const ACTIVE_IPC_SCHEMA_VERSION: u32 = 4;
pub const ACTIVE_MATRIX_SCHEMA_VERSION: u32 = 2;
pub const ACTIVE_MATRIX_IDENTITY: &str = "IsaacCsT0-ActiveMatrix-v2";
pub const ACTIVE_EXPERIMENT_IDENTITY: &str = "IsaacCsT0-ActiveLoopback-v1";
pub const ACTIVE_IPC_SOURCE_FORMAT: &str = "ICSI04V1";
pub const ACTIVE_CONTROL_SOURCE_FORMAT: &str = "NO_ACTIVE_IPC_CONTROL_V1";
pub const ORIGINAL_ONLINE_OBSERVATION_SCHEMA_VERSION: u32 = 1;
pub const ACTIVE_HOOK_MASK: u64 = (1_u64 << 13) - 1;
pub const ACTIVE_RESTORED_SLOT_COUNT: u32 = 45;
const ACTIVE_EVENT_BUFFER_CAPACITY_BYTES: usize = 4 * 1024 * 1024;

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ActiveTargetSlot {
    pub ordinal: u32,
    pub id: String,
    pub rva: String,
    pub bytes_hex: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct OriginalOnlineDisarmedObservation {
    pub schema_version: u32,
    pub observation_count: u32,
    pub qpc_before: u64,
    pub qpc_after: u64,
    pub game_root_identity_sha256: String,
    pub manager_identity_sha256: String,
    pub network_backend_identity_sha256: String,
    pub roster_identity_sha256: String,
    pub roster_count: u32,
    pub online_frame_before: u32,
    pub online_frame_after: u32,
    pub game_root_stable: bool,
    pub run_active: bool,
    pub manager_identity_stable: bool,
    pub network_backend_present: bool,
    pub roster_stable: bool,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ActiveManifest {
    pub schema_version: u32,
    pub experiment_identity: String,
    pub evidence_class: String,
    pub state: String,
    pub case_id: String,
    pub probe: String,
    pub scenario: String,
    pub endpoint: String,
    pub target_sha256: String,
    pub observed_target_sha256: String,
    pub target_equivalence_sha256: String,
    pub mode_gate_sha256: String,
    pub loop_gate_sha256: String,
    pub raw_prereq_sha256: String,
    pub active_catalog_sha256: String,
    pub active_plan_sha256: String,
    pub input_script_sha256: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub original_online_observation: Option<OriginalOnlineDisarmedObservation>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub companion_loop_artifact_path: Option<PathBuf>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub companion_loop_manifest_sha256: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub companion_loop_events_sha256: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub companion_loop_analysis_sha256: Option<String>,
    pub artifact_identity: String,
    pub ipc_magic: String,
    pub ipc_schema_version: Option<u32>,
    pub arm_token_sha256: Option<String>,
    pub arm_token_consumed: bool,
    pub session_id: String,
    pub client_id: u64,
    pub main_thread_id: u32,
    pub apply_generation: u64,
    pub installed_active_hooks_mask: String,
    pub restored_active_hooks_mask: String,
    pub restored_slot_count: u32,
    pub overwritten_events: u64,
    pub producer_closed: bool,
    pub target_slots_before: Vec<ActiveTargetSlot>,
    pub target_slots_after: Vec<ActiveTargetSlot>,
    pub target_bytes_restored: bool,
    pub initial_snapshot_neutral: bool,
    pub client_retired: bool,
    pub failure_reason: Option<String>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ActiveEvent {
    pub event_sequence: u64,
    pub stage: String,
    pub phase: String,
    pub qpc: u64,
    pub thread_id: u32,
    pub authority_tick: u64,
    pub loop_iteration: u64,
    pub attempt: u32,
    pub session_id: Option<String>,
    pub client_id: Option<u64>,
    pub command_sequence: Option<u64>,
    pub target_tick: Option<u64>,
    pub canonical_state_hex: Option<String>,
    pub edge_flags: Option<u32>,
    pub decision: Option<String>,
    pub reject_reason: Option<String>,
    pub apply_generation: Option<u64>,
    pub apply_kind: Option<String>,
    pub source_device: Option<String>,
    pub owner_before: Option<u32>,
    pub owner_after: Option<u32>,
    pub owner_generation: Option<u64>,
    pub lifecycle_kind: Option<String>,
    pub error_code: Option<u32>,
    pub error_name: Option<String>,
    pub failure_site_rva: Option<String>,
    pub queue_depth: Option<u32>,
    pub previous_command_sequence: Option<u64>,
    pub conflicting_state_hex: Option<String>,
    pub action: Option<u32>,
    pub query_kind: Option<String>,
    pub consumer: Option<String>,
    pub original_calls: Option<u32>,
    pub original_result_bits: Option<u32>,
    pub effective_result_bits: Option<u32>,
    pub query_overridden: Option<bool>,
    pub sample_component_count: Option<u32>,
    pub witness_kind: Option<String>,
    pub world_witness_before: Option<String>,
    pub world_witness_after: Option<String>,
}

#[derive(Clone, Debug)]
pub struct ActiveRunConfig {
    pub case_id: String,
    pub scenario: String,
    pub target_sha256: String,
    pub observed_target_sha256: String,
    pub target_equivalence_sha256: String,
    pub mode_gate_sha256: String,
    pub loop_gate_sha256: String,
    pub raw_prereq_sha256: String,
    pub active_catalog_sha256: String,
    pub active_plan_sha256: String,
    pub input_script_sha256: Option<String>,
    pub companion_loop_artifact_path: PathBuf,
    pub artifact_identity: String,
    pub arm_token_sha256: String,
    pub session_id: [u8; 16],
    pub client_id: u64,
    pub target_slots_before: Vec<ActiveTargetSlot>,
}

#[derive(Clone, Debug)]
pub struct DisarmedControlRunConfig {
    pub case_id: String,
    pub probe: String,
    pub scenario: String,
    pub target_sha256: String,
    pub observed_target_sha256: String,
    pub target_equivalence_sha256: String,
    pub mode_gate_sha256: String,
    pub loop_gate_sha256: String,
    pub raw_prereq_sha256: String,
    pub active_catalog_sha256: String,
    pub active_plan_sha256: String,
    pub original_online_observation: Option<OriginalOnlineDisarmedObservation>,
    pub artifact_identity: String,
    pub observation_id: [u8; 16],
    pub observer_id: u64,
    pub observation_qpc: u64,
    pub target_slots_before: Vec<ActiveTargetSlot>,
    pub target_slots_after: Vec<ActiveTargetSlot>,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct ActiveRunFinish {
    pub main_thread_id: u32,
    pub apply_generation: u64,
    pub installed_active_hooks_mask: u64,
    pub restored_active_hooks_mask: u64,
    pub restored_slot_count: u32,
    pub overwritten_events: u64,
    pub producer_closed: bool,
    pub initial_snapshot_neutral: bool,
    pub client_retired: bool,
}

impl Default for ActiveRunFinish {
    fn default() -> Self {
        Self {
            main_thread_id: 0,
            apply_generation: 1,
            installed_active_hooks_mask: 0,
            restored_active_hooks_mask: 0,
            restored_slot_count: 0,
            overwritten_events: 0,
            producer_closed: false,
            initial_snapshot_neutral: true,
            client_retired: false,
        }
    }
}

pub struct ActiveRun {
    directory: PathBuf,
    manifest: ActiveManifest,
    events_file: BufWriter<fs::File>,
    next_event_sequence: u64,
    session_id: [u8; 16],
    target_slots_before: Vec<ActiveTargetSlot>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ActiveCheck {
    pub id: String,
    pub passed: bool,
    pub detail: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ActiveAnalysisReport {
    pub schema_version: u32,
    pub evidence_class: String,
    pub result: String,
    pub eligible_for_active_gate: bool,
    pub case_id: String,
    pub probe: String,
    pub scenario: String,
    pub artifact_identity: String,
    pub event_count: usize,
    pub accepted_count: usize,
    pub rejected_count: usize,
    pub apply_count: usize,
    pub semantic_trace_sha256: String,
    pub canonical_trace_sha256: String,
    pub decision_trace_sha256: String,
    pub apply_trace_sha256: String,
    pub checks: Vec<ActiveCheck>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ActiveComparisonReport {
    pub schema_version: u32,
    pub evidence_class: String,
    pub result: String,
    pub eligible_for_active_gate: bool,
    pub first_artifact_identity: String,
    pub second_artifact_identity: String,
    pub first_analysis_sha256: String,
    pub second_analysis_sha256: String,
    pub semantic_trace_sha256: String,
    pub checks: Vec<ActiveCheck>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ActiveMatrixCase {
    pub case_name: String,
    pub analysis_path: PathBuf,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ActiveMatrixV2 {
    pub schema_version: u32,
    pub matrix_identity: String,
    pub cases: Vec<ActiveMatrixCase>,
    pub repeatability_analyses: Vec<PathBuf>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ActiveAnalysisBinding {
    pub case_name: String,
    pub artifact_identity: String,
    pub analysis_path: PathBuf,
    pub analysis_sha256: String,
    pub manifest_sha256: String,
    pub events_sha256: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ActiveRepeatabilityBinding {
    pub source_index: u32,
    pub artifact_identity: String,
    pub analysis_path: PathBuf,
    pub analysis_sha256: String,
    pub manifest_sha256: String,
    pub events_sha256: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ActiveGateDecision {
    pub schema_version: u32,
    pub status: String,
    pub t0_04_unlocked: bool,
    pub active_input_allowed: bool,
    pub semantic_contract_complete: bool,
    pub runtime_contract_complete: bool,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub active_verification_sha256: Option<String>,
    pub target_sha256: String,
    pub observed_target_sha256: String,
    pub target_equivalence_sha256: String,
    pub mode_gate_sha256: String,
    pub loop_gate_sha256: String,
    pub raw_prereq_sha256: String,
    pub active_catalog_sha256: String,
    pub active_plan_sha256: String,
    pub matrix_sha256: String,
    pub repeatability_sha256: String,
    pub required_cases: Vec<String>,
    pub missing_cases: Vec<String>,
    pub analysis_bindings: Vec<ActiveAnalysisBinding>,
    pub repeatability_bindings: Vec<ActiveRepeatabilityBinding>,
    pub repeatability_first_artifact_identity: String,
    pub repeatability_second_artifact_identity: String,
}

#[derive(Clone, Debug, Eq, Ord, PartialEq, PartialOrd)]
struct CommandIdentity {
    session_id: String,
    client_id: u64,
    sequence: u64,
}

pub fn validate_active_run_config(config: &ActiveRunConfig) -> AppResult<()> {
    if config.case_id.is_empty()
        || config.case_id.len() > 64
        || !config
            .case_id
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_'))
    {
        return Err(AppError::usage("invalid active case id"));
    }
    if !ACTIVE_SCENARIOS.contains(&config.scenario.as_str())
        || matches!(
            config.scenario.as_str(),
            "control-no-native" | "original-online-disarmed"
        )
    {
        return Err(AppError::usage(
            "active runtime writer requires one of the 16 pipeline scenarios",
        ));
    }
    for (value, label) in [
        (&config.target_sha256, "target SHA-256"),
        (&config.observed_target_sha256, "observed target SHA-256"),
        (
            &config.target_equivalence_sha256,
            "target equivalence SHA-256",
        ),
        (&config.mode_gate_sha256, "mode gate SHA-256"),
        (&config.loop_gate_sha256, "loop gate SHA-256"),
        (&config.raw_prereq_sha256, "raw prerequisite SHA-256"),
        (&config.active_catalog_sha256, "active catalog SHA-256"),
        (&config.active_plan_sha256, "active plan SHA-256"),
        (&config.arm_token_sha256, "arm token SHA-256"),
    ] {
        if !valid_sha256(value) {
            return Err(AppError::usage(format!("invalid {label}")));
        }
    }
    if config.target_sha256 != FIXED_TARGET_SHA256
        || config.observed_target_sha256
            != crate::target_equivalence::APPROVED_UNPACKED_TARGET_SHA256
        || config.session_id.iter().all(|byte| *byte == 0)
        || config.client_id == 0
        || config.artifact_identity.trim().is_empty()
        || !target_slots_valid(&config.target_slots_before)
        || config.companion_loop_artifact_path.as_os_str().is_empty()
        || config.companion_loop_artifact_path.is_absolute()
        || config
            .companion_loop_artifact_path
            .components()
            .any(|component| !matches!(component, Component::Normal(_)))
    {
        return Err(AppError::usage(
            "active runtime writer has invalid identity, slots, session, client, or companion path",
        ));
    }
    match (
        config.scenario.as_str(),
        config.input_script_sha256.as_deref(),
    ) {
        ("loopback", Some(hash)) if valid_sha256(hash) => {}
        ("loopback", _) => {
            return Err(AppError::usage(
                "loopback active writer requires the frozen input script SHA-256",
            ));
        }
        (_, None) => {}
        (_, Some(_)) => {
            return Err(AppError::usage(
                "only loopback active sources may bind an input script",
            ));
        }
    }
    Ok(())
}

pub fn original_online_observation_valid(observation: &OriginalOnlineDisarmedObservation) -> bool {
    observation.schema_version == ORIGINAL_ONLINE_OBSERVATION_SCHEMA_VERSION
        && observation.observation_count >= 2
        && observation.qpc_before > 0
        && observation.qpc_after > observation.qpc_before
        && valid_sha256(&observation.game_root_identity_sha256)
        && valid_sha256(&observation.manager_identity_sha256)
        && valid_sha256(&observation.network_backend_identity_sha256)
        && valid_sha256(&observation.roster_identity_sha256)
        && (2..=4).contains(&observation.roster_count)
        && observation.online_frame_before != u32::MAX
        && observation.online_frame_after != u32::MAX
        && observation.online_frame_after >= observation.online_frame_before
        && observation.game_root_stable
        && observation.run_active
        && observation.manager_identity_stable
        && observation.network_backend_present
        && observation.roster_stable
}

pub fn validate_disarmed_control_run_config(config: &DisarmedControlRunConfig) -> AppResult<()> {
    if config.case_id.is_empty()
        || config.case_id.len() > 64
        || !config
            .case_id
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_'))
        || !matches!(
            (config.probe.as_str(), config.scenario.as_str()),
            ("control-no-native", "control-no-native")
                | ("original-online-disarmed", "original-online-disarmed")
        )
    {
        return Err(AppError::usage(
            "disarmed control writer requires a matching control scenario and valid case id",
        ));
    }
    for (value, label) in [
        (&config.target_sha256, "target SHA-256"),
        (&config.observed_target_sha256, "observed target SHA-256"),
        (
            &config.target_equivalence_sha256,
            "target equivalence SHA-256",
        ),
        (&config.mode_gate_sha256, "mode gate SHA-256"),
        (&config.loop_gate_sha256, "loop gate SHA-256"),
        (&config.raw_prereq_sha256, "raw prerequisite SHA-256"),
        (&config.active_catalog_sha256, "active catalog SHA-256"),
        (&config.active_plan_sha256, "active plan SHA-256"),
    ] {
        if !valid_sha256(value) {
            return Err(AppError::usage(format!("invalid {label}")));
        }
    }
    if config.target_sha256 != FIXED_TARGET_SHA256
        || config.observed_target_sha256
            != crate::target_equivalence::APPROVED_UNPACKED_TARGET_SHA256
        || config.artifact_identity.trim().is_empty()
        || config.observation_id.iter().all(|byte| *byte == 0)
        || config.observer_id == 0
        || config.observation_qpc == 0
        || !target_slots_valid(&config.target_slots_before)
        || !target_slots_valid(&config.target_slots_after)
        || config.target_slots_before != config.target_slots_after
    {
        return Err(AppError::usage(
            "disarmed control writer has invalid identity, observation clock, or 45-slot snapshots",
        ));
    }
    match (config.probe.as_str(), &config.original_online_observation) {
        ("control-no-native", None) => {}
        ("original-online-disarmed", Some(observation))
            if original_online_observation_valid(observation) => {}
        _ => {
            return Err(AppError::usage(
                "disarmed control writer has an invalid Original Online observation binding",
            ));
        }
    }
    Ok(())
}

pub fn write_disarmed_control_run(
    output_directory: &Path,
    config: DisarmedControlRunConfig,
) -> AppResult<ActiveAnalysisReport> {
    validate_disarmed_control_run_config(&config)?;
    let endpoint = if config.probe == "original-online-disarmed" {
        "original-online-read-only"
    } else {
        "disarmed-control"
    };
    fs::create_dir(output_directory).usage_context(format!(
        "refusing to reuse disarmed control artifact {}",
        output_directory.display()
    ))?;

    let event = ActiveEvent {
        event_sequence: 1,
        stage: "control-observation".to_owned(),
        phase: "OUTSIDE_AUTHORITY".to_owned(),
        qpc: config.observation_qpc,
        thread_id: 0,
        authority_tick: 0,
        loop_iteration: 0,
        attempt: 0,
        session_id: None,
        client_id: None,
        command_sequence: None,
        target_tick: None,
        canonical_state_hex: None,
        edge_flags: None,
        decision: None,
        reject_reason: None,
        apply_generation: None,
        apply_kind: None,
        source_device: None,
        owner_before: None,
        owner_after: None,
        owner_generation: None,
        lifecycle_kind: None,
        error_code: None,
        error_name: None,
        failure_site_rva: None,
        queue_depth: None,
        previous_command_sequence: None,
        conflicting_state_hex: None,
        action: None,
        query_kind: None,
        consumer: None,
        original_calls: None,
        original_result_bits: None,
        effective_result_bits: None,
        query_overridden: None,
        sample_component_count: None,
        witness_kind: None,
        world_witness_before: None,
        world_witness_after: None,
    };
    let events_path = output_directory.join("events.jsonl");
    let events = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(&events_path)
        .usage_context("cannot create disarmed control events")?;
    let mut events = BufWriter::new(events);
    serde_json::to_writer(&mut events, &event)
        .usage_context("cannot serialize disarmed control event")?;
    events
        .write_all(b"\n")
        .usage_context("cannot append disarmed control event")?;
    events
        .flush()
        .usage_context("cannot flush disarmed control events")?;

    let manifest = ActiveManifest {
        schema_version: ACTIVE_ARTIFACT_SCHEMA_VERSION,
        experiment_identity: ACTIVE_EXPERIMENT_IDENTITY.to_owned(),
        evidence_class: EVIDENCE_DISARMED_CONTROL.to_owned(),
        state: "COMPLETE".to_owned(),
        case_id: config.case_id,
        probe: config.probe,
        scenario: config.scenario,
        endpoint: endpoint.to_owned(),
        target_sha256: config.target_sha256,
        observed_target_sha256: config.observed_target_sha256,
        target_equivalence_sha256: config.target_equivalence_sha256,
        mode_gate_sha256: config.mode_gate_sha256,
        loop_gate_sha256: config.loop_gate_sha256,
        raw_prereq_sha256: config.raw_prereq_sha256,
        active_catalog_sha256: config.active_catalog_sha256,
        active_plan_sha256: config.active_plan_sha256,
        input_script_sha256: None,
        original_online_observation: config.original_online_observation,
        companion_loop_artifact_path: None,
        companion_loop_manifest_sha256: None,
        companion_loop_events_sha256: None,
        companion_loop_analysis_sha256: None,
        artifact_identity: config.artifact_identity,
        ipc_magic: ACTIVE_CONTROL_SOURCE_FORMAT.to_owned(),
        ipc_schema_version: None,
        arm_token_sha256: None,
        arm_token_consumed: false,
        session_id: hex::encode_upper(config.observation_id),
        client_id: config.observer_id,
        main_thread_id: 0,
        apply_generation: 1,
        installed_active_hooks_mask: "0x0".to_owned(),
        restored_active_hooks_mask: "0x0".to_owned(),
        restored_slot_count: ACTIVE_RESTORED_SLOT_COUNT,
        overwritten_events: 0,
        producer_closed: true,
        target_slots_before: config.target_slots_before,
        target_slots_after: config.target_slots_after,
        target_bytes_restored: true,
        initial_snapshot_neutral: true,
        client_retired: true,
        failure_reason: None,
    };
    write_active_manifest(&output_directory.join("manifest.json"), &manifest, true)?;
    analyze_active_run(output_directory)
}

impl ActiveRun {
    pub fn create(output_directory: &Path, config: ActiveRunConfig) -> AppResult<Self> {
        validate_active_run_config(&config)?;
        fs::create_dir(output_directory).usage_context(format!(
            "refusing to reuse active artifact {}",
            output_directory.display()
        ))?;
        let events = OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(output_directory.join("events.jsonl"))
            .usage_context("cannot create active events")?;
        let events_file = BufWriter::with_capacity(ACTIVE_EVENT_BUFFER_CAPACITY_BYTES, events);
        let session_id_hex = hex::encode_upper(config.session_id);
        let manifest = ActiveManifest {
            schema_version: ACTIVE_ARTIFACT_SCHEMA_VERSION,
            experiment_identity: ACTIVE_EXPERIMENT_IDENTITY.to_owned(),
            evidence_class: EVIDENCE_PIPELINE_RUNTIME.to_owned(),
            state: "RUNNING".to_owned(),
            case_id: config.case_id,
            probe: "loopback".to_owned(),
            scenario: config.scenario,
            endpoint: "offline-local".to_owned(),
            target_sha256: config.target_sha256,
            observed_target_sha256: config.observed_target_sha256,
            target_equivalence_sha256: config.target_equivalence_sha256,
            mode_gate_sha256: config.mode_gate_sha256,
            loop_gate_sha256: config.loop_gate_sha256,
            raw_prereq_sha256: config.raw_prereq_sha256,
            active_catalog_sha256: config.active_catalog_sha256,
            active_plan_sha256: config.active_plan_sha256,
            input_script_sha256: config.input_script_sha256,
            original_online_observation: None,
            companion_loop_artifact_path: Some(config.companion_loop_artifact_path),
            companion_loop_manifest_sha256: None,
            companion_loop_events_sha256: None,
            companion_loop_analysis_sha256: None,
            artifact_identity: config.artifact_identity,
            ipc_magic: ACTIVE_IPC_SOURCE_FORMAT.to_owned(),
            ipc_schema_version: Some(ACTIVE_IPC_SCHEMA_VERSION),
            arm_token_sha256: Some(config.arm_token_sha256),
            arm_token_consumed: true,
            session_id: session_id_hex,
            client_id: config.client_id,
            main_thread_id: 0,
            apply_generation: 1,
            installed_active_hooks_mask: "0x0".to_owned(),
            restored_active_hooks_mask: "0x0".to_owned(),
            restored_slot_count: 0,
            overwritten_events: 0,
            producer_closed: false,
            target_slots_before: config.target_slots_before.clone(),
            target_slots_after: Vec::new(),
            target_bytes_restored: false,
            initial_snapshot_neutral: true,
            client_retired: false,
            failure_reason: None,
        };
        write_active_manifest(&output_directory.join("manifest.json"), &manifest, true)?;
        Ok(Self {
            directory: output_directory.to_owned(),
            manifest,
            events_file,
            next_event_sequence: 1,
            session_id: config.session_id,
            target_slots_before: config.target_slots_before,
        })
    }

    pub fn directory(&self) -> &Path {
        &self.directory
    }

    pub fn append_native(&mut self, record: &NativeActiveEventRecord) -> AppResult<()> {
        let event = active_event_from_native(record, self.next_event_sequence, &self.session_id)?;
        serde_json::to_writer(&mut self.events_file, &event)
            .usage_context("cannot serialize active event")?;
        self.events_file
            .write_all(b"\n")
            .usage_context("cannot append active event")?;
        self.next_event_sequence = self
            .next_event_sequence
            .checked_add(1)
            .ok_or_else(|| AppError::evidence("active artifact sequence overflow"))?;
        Ok(())
    }

    pub fn finish(
        mut self,
        target_slots_after: Vec<ActiveTargetSlot>,
        summary: ActiveRunFinish,
    ) -> AppResult<ActiveAnalysisReport> {
        if !target_slots_valid(&target_slots_after) {
            return Err(AppError::evidence(
                "active after snapshot does not contain the exact ordered 45 slots",
            ));
        }
        self.events_file
            .flush()
            .usage_context("cannot flush active events")?;
        self.bind_summary(summary);
        self.manifest.target_slots_after = target_slots_after;
        self.manifest.target_bytes_restored =
            self.manifest.target_slots_after == self.target_slots_before;
        self.bind_companion_hashes(true)?;
        self.manifest.state = "COMPLETE".to_owned();
        self.manifest.failure_reason = None;
        write_active_manifest(&self.directory.join("manifest.json"), &self.manifest, false)?;
        analyze_active_run(&self.directory)
    }

    pub fn fail(
        mut self,
        reason: &str,
        target_slots_after: Option<Vec<ActiveTargetSlot>>,
        summary: ActiveRunFinish,
    ) -> AppResult<()> {
        if reason.trim().is_empty() {
            return Err(AppError::usage(
                "active failure artifact requires a nonempty reason",
            ));
        }
        self.events_file
            .flush()
            .usage_context("cannot flush failed active events")?;
        self.bind_summary(summary);
        if let Some(after) = target_slots_after {
            if !target_slots_valid(&after) {
                return Err(AppError::evidence(
                    "failed active after snapshot does not contain the exact ordered 45 slots",
                ));
            }
            self.manifest.target_slots_after = after;
            self.manifest.target_bytes_restored =
                self.manifest.target_slots_after == self.target_slots_before;
        }
        self.bind_companion_hashes(false)?;
        self.manifest.state = "FAILED".to_owned();
        self.manifest.failure_reason = Some(reason.to_owned());
        write_active_manifest(&self.directory.join("manifest.json"), &self.manifest, false)
    }

    fn bind_summary(&mut self, summary: ActiveRunFinish) {
        self.manifest.main_thread_id = summary.main_thread_id;
        self.manifest.apply_generation = summary.apply_generation;
        self.manifest.installed_active_hooks_mask =
            format!("0x{:X}", summary.installed_active_hooks_mask);
        self.manifest.restored_active_hooks_mask =
            format!("0x{:X}", summary.restored_active_hooks_mask);
        self.manifest.restored_slot_count = summary.restored_slot_count;
        self.manifest.overwritten_events = summary.overwritten_events;
        self.manifest.producer_closed = summary.producer_closed;
        self.manifest.initial_snapshot_neutral = summary.initial_snapshot_neutral;
        self.manifest.client_retired = summary.client_retired;
    }

    fn bind_companion_hashes(&mut self, required: bool) -> AppResult<()> {
        let companion = self
            .manifest
            .companion_loop_artifact_path
            .as_ref()
            .map(|path| self.directory.join(path))
            .ok_or_else(|| AppError::evidence("active runtime lacks companion path"))?;
        let paths = [
            companion.join("manifest.json"),
            companion.join("events.jsonl"),
            companion.join("analysis.json"),
        ];
        if paths.iter().all(|path| path.is_file()) {
            let hashes = (|| {
                Ok::<_, AppError>((
                    file_sha256(&paths[0])?,
                    file_sha256(&paths[1])?,
                    file_sha256(&paths[2])?,
                ))
            })();
            match hashes {
                Ok((manifest, events, analysis)) => {
                    self.manifest.companion_loop_manifest_sha256 = Some(manifest);
                    self.manifest.companion_loop_events_sha256 = Some(events);
                    self.manifest.companion_loop_analysis_sha256 = Some(analysis);
                }
                Err(error) if required => return Err(error),
                Err(_) => {}
            }
        } else if required {
            return Err(AppError::evidence(
                "active runtime cannot finish without all three companion V5 source files",
            ));
        }
        Ok(())
    }
}

fn write_active_manifest(
    path: &Path,
    manifest: &ActiveManifest,
    create_new: bool,
) -> AppResult<()> {
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
    serde_json::to_writer_pretty(&mut file, manifest)
        .usage_context(format!("cannot serialize {}", path.display()))?;
    file.write_all(b"\n")
        .usage_context(format!("cannot finish {}", path.display()))
}

pub fn active_event_from_native(
    record: &NativeActiveEventRecord,
    event_sequence: u64,
    session_id: &[u8; 16],
) -> AppResult<ActiveEvent> {
    if record.sequence == 0
        || record.sequence != event_sequence
        || record.event.published_sequence != record.sequence as i64
        || record.event.qpc <= 0
        || record.event.flags & !ACTIVE_KNOWN_FLAGS != 0
        || session_id.iter().all(|byte| *byte == 0)
    {
        return Err(AppError::evidence(
            "active native event has invalid sequence, session, QPC, or flags",
        ));
    }
    let kind = ActiveEventKind::from_raw(record.event.kind)?;
    let phase = ActivePhase::from_raw(record.event.phase)?;
    let command = matches!(
        kind,
        ActiveEventKind::PhysicalSample
            | ActiveEventKind::Canonical
            | ActiveEventKind::Send
            | ActiveEventKind::Receive
            | ActiveEventKind::Accept
            | ActiveEventKind::Reject
            | ActiveEventKind::Duplicate
            | ActiveEventKind::Resolve
            | ActiveEventKind::ApplyCommit
            | ActiveEventKind::Suppression
            | ActiveEventKind::QueryOverride
            | ActiveEventKind::Witness
    );
    let stage = match kind {
        ActiveEventKind::PhysicalSample => "sample",
        ActiveEventKind::Canonical => "canonical",
        ActiveEventKind::Send => "send",
        ActiveEventKind::Receive => "receive",
        ActiveEventKind::Accept => "accept",
        ActiveEventKind::Reject => "reject",
        ActiveEventKind::Duplicate => "duplicate",
        ActiveEventKind::Resolve => "resolve",
        ActiveEventKind::ApplyCommit => "apply-commit",
        ActiveEventKind::Suppression => "suppression",
        ActiveEventKind::QueryOverride => "query-override",
        ActiveEventKind::Witness => "witness",
        _ => "lifecycle",
    };
    let expected_phase = match kind {
        ActiveEventKind::PhysicalSample | ActiveEventKind::Canonical | ActiveEventKind::Send => {
            ActivePhase::PhysicalSample
        }
        ActiveEventKind::Receive
        | ActiveEventKind::Accept
        | ActiveEventKind::Reject
        | ActiveEventKind::Duplicate => ActivePhase::DrainBeforeResolve,
        ActiveEventKind::Resolve => ActivePhase::Resolve,
        ActiveEventKind::ApplyCommit => ActivePhase::Apply,
        ActiveEventKind::Suppression | ActiveEventKind::QueryOverride => ActivePhase::InputQuery,
        ActiveEventKind::Witness => ActivePhase::Witness,
        _ => ActivePhase::OutsideAuthority,
    };
    if phase != expected_phase {
        return Err(AppError::evidence(
            "active phase code is inconsistent with its event kind",
        ));
    }
    let decision = ActiveDecision::from_raw(record.event.decision)?;
    let expected_decision = match kind {
        ActiveEventKind::Accept => ActiveDecision::Accepted,
        ActiveEventKind::Reject => ActiveDecision::Rejected,
        ActiveEventKind::Duplicate => ActiveDecision::Duplicate,
        _ => ActiveDecision::None,
    };
    if decision != expected_decision {
        return Err(AppError::evidence(
            "active decision code is inconsistent with its event kind",
        ));
    }
    let reject = ActiveRejectReason::from_raw(record.event.reject_reason)?;
    if (kind == ActiveEventKind::Reject) != (reject != ActiveRejectReason::None) {
        return Err(AppError::evidence(
            "active reject reason is inconsistent with its event kind",
        ));
    }
    let source = ActiveSourceDevice::from_raw(record.event.source_device)?;
    let query = ActiveQueryKind::from_raw(record.event.query_kind)?;
    let consumer = ActiveConsumer::from_raw(record.event.consumer)?;
    let lifecycle = match kind {
        ActiveEventKind::Initialized => Some("INITIALIZED"),
        ActiveEventKind::HooksInstalled => Some("HOOKS_INSTALLED"),
        ActiveEventKind::OwnerSwitch => Some("OWNER_SWITCH"),
        ActiveEventKind::LifecycleClear => {
            ActiveLifecycleKind::from_raw(record.event.action)?.as_str()
        }
        ActiveEventKind::ClientRetired => Some("CLIENT_RETIRED"),
        ActiveEventKind::HooksRestored => Some("HOOKS_RESTORED"),
        ActiveEventKind::ProducerClosed => Some("PRODUCER_CLOSED"),
        ActiveEventKind::Failure => Some("FAILURE"),
        _ => None,
    };
    let witness = if kind == ActiveEventKind::Witness {
        ActiveWitnessKind::from_raw(record.event.query_kind)?.as_str()
    } else {
        None
    };
    let failure_error = if kind == ActiveEventKind::Failure {
        let error = ActiveErrorCode::from_raw(record.event.error_code as i32)?;
        if record.event.error_code == 0 {
            return Err(AppError::evidence(
                "active failure event lacks a numeric error code",
            ));
        }
        Some(error)
    } else if record.event.error_code != 0 {
        return Err(AppError::evidence(
            "non-failure active event contains an error code",
        ));
    } else {
        None
    };
    if command {
        let decoded =
            InputStateDraftV1::decode(&record.event.canonical_state).map_err(|error| {
                AppError::evidence(format!("invalid active canonical state: {error}"))
            })?;
        if record.event.client_id == 0
            || record.event.command_sequence == 0
            || record.event.edge_flags != u32::from(decoded.edge_flags())
        {
            return Err(AppError::evidence(
                "active command event has zero identity or mismatched canonical edge flags",
            ));
        }
    }
    let canonical = command.then(|| hex::encode_upper(record.event.canonical_state));
    let conflicting = (command && record.event.flags & ACTIVE_FLAG_CONFLICTING_STATE != 0)
        .then(|| hex::encode_upper(record.event.conflicting_state));
    Ok(ActiveEvent {
        event_sequence,
        stage: stage.to_owned(),
        phase: phase.as_str().to_owned(),
        qpc: record.event.qpc as u64,
        thread_id: record.event.thread_id,
        authority_tick: record.event.authority_tick,
        loop_iteration: record.event.loop_iteration,
        attempt: record.event.attempt,
        session_id: command.then(|| hex::encode_upper(session_id)),
        client_id: command.then_some(record.event.client_id),
        command_sequence: command.then_some(record.event.command_sequence),
        target_tick: command.then_some(record.event.target_tick),
        canonical_state_hex: canonical,
        edge_flags: command.then_some(record.event.edge_flags),
        decision: decision.as_str().map(str::to_owned),
        reject_reason: if kind == ActiveEventKind::Accept
            || kind == ActiveEventKind::Reject
            || kind == ActiveEventKind::Duplicate
        {
            Some(reject.as_str().to_owned())
        } else {
            None
        },
        apply_generation: command.then_some(record.event.apply_generation),
        apply_kind: (kind == ActiveEventKind::ApplyCommit).then(|| "SYNTHETIC_SNAPSHOT".to_owned()),
        source_device: match source {
            ActiveSourceDevice::None => None,
            ActiveSourceDevice::Keyboard => Some("KEYBOARD".to_owned()),
            ActiveSourceDevice::Gamepad => Some("GAMEPAD".to_owned()),
        },
        owner_before: matches!(kind, ActiveEventKind::OwnerSwitch)
            .then_some(record.event.owner_before),
        owner_after: matches!(kind, ActiveEventKind::OwnerSwitch)
            .then_some(record.event.owner_after),
        owner_generation: (command
            || matches!(
                kind,
                ActiveEventKind::OwnerSwitch | ActiveEventKind::LifecycleClear
            ))
        .then_some(record.event.owner_generation),
        lifecycle_kind: lifecycle.map(str::to_owned),
        error_code: failure_error.map(|_| record.event.error_code),
        error_name: failure_error.map(|error| error.as_str().to_owned()),
        failure_site_rva: (kind == ActiveEventKind::Failure && record.event.hook_site_rva != 0)
            .then(|| format!("0x{:08X}", record.event.hook_site_rva)),
        queue_depth: command.then_some(record.event.queue_depth),
        previous_command_sequence: (record.event.previous_command_sequence != 0)
            .then_some(record.event.previous_command_sequence),
        conflicting_state_hex: conflicting,
        action: (command && record.event.action != ACTIVE_NO_ACTION).then_some(record.event.action),
        query_kind: matches!(
            kind,
            ActiveEventKind::Suppression | ActiveEventKind::QueryOverride
        )
        .then(|| query.as_str())
        .flatten()
        .map(str::to_owned),
        consumer: matches!(
            kind,
            ActiveEventKind::Suppression
                | ActiveEventKind::QueryOverride
                | ActiveEventKind::Witness
        )
        .then(|| consumer.as_str())
        .flatten()
        .map(str::to_owned),
        original_calls: matches!(
            kind,
            ActiveEventKind::Suppression | ActiveEventKind::QueryOverride
        )
        .then_some(record.event.original_calls),
        original_result_bits: matches!(
            kind,
            ActiveEventKind::Suppression | ActiveEventKind::QueryOverride
        )
        .then_some(record.event.original_result_bits),
        effective_result_bits: matches!(
            kind,
            ActiveEventKind::Suppression | ActiveEventKind::QueryOverride
        )
        .then_some(record.event.effective_result_bits),
        query_overridden: matches!(
            kind,
            ActiveEventKind::Suppression | ActiveEventKind::QueryOverride
        )
        .then_some(record.event.flags & ACTIVE_FLAG_QUERY_OVERRIDDEN != 0),
        sample_component_count: matches!(
            kind,
            ActiveEventKind::PhysicalSample | ActiveEventKind::ApplyCommit
        )
        .then_some(record.event.sample_component_count),
        witness_kind: witness.map(str::to_owned),
        world_witness_before: (kind == ActiveEventKind::Witness)
            .then(|| hex::encode_upper(record.event.witness_before)),
        world_witness_after: (kind == ActiveEventKind::Witness)
            .then(|| hex::encode_upper(record.event.witness_after)),
    })
}

pub fn active_run_identity(run_directory: &Path) -> AppResult<(u32, String)> {
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

pub fn read_active_run(run_directory: &Path) -> AppResult<(ActiveManifest, Vec<ActiveEvent>)> {
    let manifest_path = run_directory.join("manifest.json");
    let manifest: ActiveManifest = serde_json::from_slice(
        &fs::read(&manifest_path)
            .usage_context(format!("cannot read {}", manifest_path.display()))?,
    )
    .usage_context(format!("cannot parse {}", manifest_path.display()))?;
    validate_manifest(&manifest)?;
    let events_path = run_directory.join("events.jsonl");
    let file = fs::File::open(&events_path)
        .usage_context(format!("cannot read {}", events_path.display()))?;
    let mut events = Vec::new();
    for (index, line) in BufReader::new(file).lines().enumerate() {
        events.push(
            serde_json::from_str(
                &line.usage_context(format!("cannot read active event line {}", index + 1))?,
            )
            .usage_context(format!("cannot parse active event line {}", index + 1))?,
        );
    }
    Ok((manifest, events))
}

pub fn analyze_active_run(run_directory: &Path) -> AppResult<ActiveAnalysisReport> {
    let report = analyze_active_run_internal(run_directory)?;
    write_new_json(&run_directory.join("analysis.json"), &report)?;
    Ok(report)
}

pub fn replay_active_run(run_directory: &Path) -> AppResult<ActiveAnalysisReport> {
    analyze_active_run_internal(run_directory)
}

fn analyze_active_run_internal(run_directory: &Path) -> AppResult<ActiveAnalysisReport> {
    let (manifest, events) = read_active_run(run_directory)?;
    if manifest.state != "COMPLETE" {
        return Err(AppError::evidence("active artifact is not COMPLETE"));
    }
    let companion_loop_bound = if manifest.evidence_class == EVIDENCE_PIPELINE_RUNTIME {
        validate_pipeline_companion_binding(run_directory, &manifest, &events)?;
        true
    } else {
        true
    };
    let nonempty = !events.is_empty();
    let contiguous = events
        .iter()
        .enumerate()
        .all(|(index, event)| event.event_sequence == index as u64 + 1);
    let qpc_monotonic = events.iter().try_fold(0_u64, |previous, event| {
        (event.qpc > 0 && event.qpc >= previous).then_some(event.qpc)
    });
    let known = events.iter().all(event_shape_known);
    let thread_scope = events.iter().all(|event| {
        event.stage == "lifecycle"
            || event.stage == "control-observation"
            || event.thread_id == manifest.main_thread_id
    });
    let action_partition = events
        .iter()
        .all(|event| event.action.is_none_or(|action| action <= 11));
    let canonical_valid = events.iter().all(|event| {
        event
            .canonical_state_hex
            .as_deref()
            .is_none_or(validate_state_hex)
            && event
                .conflicting_state_hex
                .as_deref()
                .is_none_or(validate_state_hex)
    });
    let command_payload_contract = events.iter().all(command_payload_valid);

    let mut groups = BTreeMap::<CommandIdentity, Vec<&ActiveEvent>>::new();
    let mut missing_identity = false;
    for event in events.iter().filter(|event| is_command_stage(&event.stage)) {
        if let Some(identity) = identity(event) {
            groups.entry(identity).or_default().push(event);
        } else {
            missing_identity = true;
        }
    }
    let mut identity_consistent = !missing_identity;
    let mut attempt_ordered = true;
    let mut cardinality_valid = true;
    let mut reject_unchanged = true;
    let mut apply_on_target = true;
    let mut one_apply = true;
    let mut accepted_count = 0;
    let mut rejected_count = 0;
    for trace in groups.values() {
        let first = trace[0];
        let owner_generations = trace
            .iter()
            .filter_map(|event| event.owner_generation)
            .collect::<BTreeSet<_>>();
        identity_consistent &= first.session_id.as_deref() == Some(manifest.session_id.as_str())
            && first.client_id == Some(manifest.client_id)
            && trace.iter().all(|event| {
                event.session_id == first.session_id
                    && event.client_id == first.client_id
                    && event.command_sequence == first.command_sequence
                    && event.target_tick == first.target_tick
                    && event.apply_generation == Some(manifest.apply_generation)
            })
            && owner_generations.len() == 1
            && owner_generations
                .first()
                .is_some_and(|generation| *generation != 0)
            && submitted_payload_contract(trace, &manifest.scenario);
        let mut attempts = BTreeMap::<u32, Vec<&ActiveEvent>>::new();
        for event in trace {
            attempts.entry(event.attempt).or_default().push(*event);
        }
        attempt_ordered &= attempts.values().all(|attempt| {
            let ranks = attempt
                .iter()
                .filter_map(|event| transport_stage_rank(&event.stage))
                .collect::<Vec<_>>();
            !ranks.is_empty() && ranks.windows(2).all(|pair| pair[0] <= pair[1])
        });
        let accepts = trace.iter().filter(|event| event.stage == "accept").count();
        let rejects = trace.iter().filter(|event| event.stage == "reject").count();
        let duplicates = trace
            .iter()
            .filter(|event| event.stage == "duplicate")
            .count();
        let resolves = trace
            .iter()
            .filter(|event| event.stage == "resolve")
            .count();
        let applies = trace
            .iter()
            .filter(|event| event.stage == "apply-commit")
            .count();
        accepted_count += accepts;
        rejected_count += rejects;
        one_apply &= applies <= 1;
        let fatal_identity_conflict = manifest.scenario == "conflict"
            && accepts == 1
            && rejects == 1
            && trace.iter().any(|event| {
                event.stage == "reject"
                    && event.reject_reason.as_deref() == Some("IDENTITY_CONFLICT")
                    && event.attempt > 0
            });
        let primary = attempts.get(&0);
        cardinality_valid &= primary.is_some_and(|events| {
            count(events, "sample") == 1
                && count(events, "canonical") == 1
                && count(events, "send") == 1
                && count(events, "receive") == 1
                && (accepts + rejects == 1 || fatal_identity_conflict)
                && if fatal_identity_conflict {
                    attempts.len() == 2 && duplicates == 0
                } else {
                    duplicates == attempts.len().saturating_sub(1)
                }
                && attempts
                    .iter()
                    .filter(|(attempt, _)| **attempt != 0)
                    .all(|(_, retry)| {
                        count(retry, "sample") == 0
                            && count(retry, "canonical") == 0
                            && count(retry, "send") == 1
                            && count(retry, "receive") == 1
                            && count(retry, "accept") == 0
                            && count(retry, "reject") + count(retry, "duplicate") == 1
                    })
                && if rejects != 0 {
                    resolves == 0 && applies == 0
                } else {
                    true
                }
        });
        for event in trace.iter().filter(|event| event.stage == "apply-commit") {
            apply_on_target &= event.target_tick == Some(event.authority_tick)
                && event.apply_generation == Some(manifest.apply_generation)
                && event.apply_kind.as_deref() == Some("SYNTHETIC_SNAPSHOT");
        }
        if rejects != 0 {
            let witnesses = trace
                .iter()
                .filter(|event| event.stage == "witness")
                .collect::<Vec<_>>();
            reject_unchanged &= applies == 0
                && !witnesses.is_empty()
                && witnesses.iter().all(|event| {
                    event.world_witness_before.is_some()
                        && event.world_witness_before == event.world_witness_after
                });
        }
    }
    cardinality_valid &= accepted_bucket_contract(&manifest, &events, &groups);
    let generation_timeline = generation_timeline_valid(&events);
    let disarmed = manifest.evidence_class == EVIDENCE_DISARMED_CONTROL;
    let disarmed_clean = !disarmed
        || events
            .iter()
            .all(|event| !is_command_stage(&event.stage) && event.stage != "query-override");
    let pipeline_nonempty = disarmed || !groups.is_empty();
    let query_original_once = events
        .iter()
        .filter(|event| event.stage == "query-override")
        .all(|event| {
            event.original_calls == Some(1)
                && event.query_overridden == Some(true)
                && event.action.is_some_and(|action| action <= 11)
                && event.consumer.as_deref().is_some_and(allowed_consumer)
                && matches!(
                    event.query_kind.as_deref(),
                    Some("PRESSED" | "TRIGGERED" | "SCALAR")
                )
        });
    let scenario_valid = scenario_predicate(&manifest, &events, &groups);

    let semantic = semantic_trace(&events);
    let canonical = canonical_trace(&events);
    let decisions = decision_trace(&events);
    let applies = apply_trace(&events);
    let checks = vec![
        check(
            "active-manifest-binding",
            true,
            "schema-4 source, gate, arm, companion and 45-slot restoration identity",
        ),
        check(
            "companion-loop-context",
            companion_loop_bound,
            "pipeline sources replay the exact common V5 contract and scenario-authority policy in the same loop context; disarmed sources bind none",
        ),
        check(
            "non-empty-events",
            nonempty,
            "active artifact has source events",
        ),
        check("event-sequence", contiguous, "event sequence is contiguous"),
        check(
            "event-qpc-order",
            qpc_monotonic.is_some(),
            "active events have positive monotonic QPC in stream order",
        ),
        check(
            "event-shape",
            known,
            "all stages, phases and scoped fields are defined",
        ),
        check(
            "main-thread-scope",
            thread_scope,
            "active work remains on the bound main thread",
        ),
        check(
            "action-partition",
            action_partition,
            "action 12+ never enters active scope",
        ),
        check(
            "canonical-codec",
            canonical_valid,
            "canonical bytes are strict InputStateDraftV1",
        ),
        check(
            "command-payload-contract",
            command_payload_contract,
            "command canonical, edge, decision and reject fields are mutually consistent",
        ),
        check(
            "command-identity",
            identity_consistent,
            "identity and submitted payload are immutable; merged effective payload is derived",
        ),
        check(
            "generation-timeline",
            generation_timeline,
            "owner generation increments at clear/switch boundaries and never reappears stale",
        ),
        check(
            "attempt-order",
            attempt_ordered,
            "each transport attempt has valid stage order",
        ),
        check(
            "stage-cardinality",
            cardinality_valid,
            "each command has exact primary and retry cardinality",
        ),
        check(
            "apply-target-tick",
            apply_on_target,
            "ApplyCommit occurs only at authorityTick == targetTick",
        ),
        check(
            "one-apply-per-command",
            one_apply,
            "a command applies at most once",
        ),
        check(
            "reject-world-unchanged",
            reject_unchanged,
            "rejected commands have zero apply and unchanged witness",
        ),
        check(
            "query-original-once",
            query_original_once,
            "query override calls original exactly once in an allowed consumer",
        ),
        check(
            "disarmed-clean",
            disarmed_clean,
            "disarmed controls contain no active pipeline",
        ),
        check(
            "pipeline-nonempty",
            pipeline_nonempty,
            "active pipeline artifacts contain command identities",
        ),
        check(
            "scenario-predicate",
            scenario_valid,
            "the dedicated scenario predicate is satisfied",
        ),
    ];
    let passed = checks.iter().all(|check| check.passed);
    Ok(ActiveAnalysisReport {
        schema_version: ACTIVE_ARTIFACT_SCHEMA_VERSION,
        evidence_class: manifest.evidence_class,
        result: if passed { "PASS" } else { "FAIL" }.to_owned(),
        eligible_for_active_gate: passed,
        case_id: manifest.case_id,
        probe: manifest.probe,
        scenario: manifest.scenario,
        artifact_identity: manifest.artifact_identity,
        event_count: events.len(),
        accepted_count,
        rejected_count,
        apply_count: events
            .iter()
            .filter(|event| event.stage == "apply-commit")
            .count(),
        semantic_trace_sha256: sha256_hex(semantic.as_bytes()),
        canonical_trace_sha256: sha256_hex(canonical.as_bytes()),
        decision_trace_sha256: sha256_hex(decisions.as_bytes()),
        apply_trace_sha256: sha256_hex(applies.as_bytes()),
        checks,
    })
}

pub fn compare_active_runs(
    first: &Path,
    second: &Path,
    output: Option<&Path>,
) -> AppResult<ActiveComparisonReport> {
    let (first_manifest, first_events) = read_active_run(first)?;
    let (second_manifest, second_events) = read_active_run(second)?;
    let first_analysis = replay_active_run(first)?;
    let second_analysis = replay_active_run(second)?;
    let semantic_first = repeatability_semantic_trace(&first_events);
    let semantic_second = repeatability_semantic_trace(&second_events);
    let independent = first_manifest.artifact_identity != second_manifest.artifact_identity
        && first_manifest.arm_token_sha256 != second_manifest.arm_token_sha256
        && first_manifest.arm_token_sha256.is_some()
        && second_manifest.arm_token_sha256.is_some();
    let same_scope = first_manifest.probe == second_manifest.probe
        && first_manifest.scenario == second_manifest.scenario
        && first_manifest.scenario == "loopback"
        && first_manifest.target_sha256 == second_manifest.target_sha256
        && first_manifest.observed_target_sha256 == second_manifest.observed_target_sha256
        && first_manifest.target_equivalence_sha256 == second_manifest.target_equivalence_sha256
        && first_manifest.mode_gate_sha256 == second_manifest.mode_gate_sha256
        && first_manifest.loop_gate_sha256 == second_manifest.loop_gate_sha256
        && first_manifest.raw_prereq_sha256 == second_manifest.raw_prereq_sha256
        && first_manifest.active_catalog_sha256 == second_manifest.active_catalog_sha256
        && first_manifest.active_plan_sha256 == second_manifest.active_plan_sha256
        && first_manifest.input_script_sha256 == second_manifest.input_script_sha256
        && first_manifest.input_script_sha256.is_some();
    let analyses_pass = first_analysis.result == "PASS"
        && second_analysis.result == "PASS"
        && first_analysis.eligible_for_active_gate
        && second_analysis.eligible_for_active_gate;
    let semantic_equal = semantic_first == semantic_second;
    let checks = vec![
        check(
            "independent-runs",
            independent,
            "artifact and one-time arm identities differ",
        ),
        check(
            "repeatability-scope",
            same_scope,
            "probe, scenario, gates, plan and frozen script match",
        ),
        check(
            "source-analysis-pass",
            analyses_pass,
            "both schema-4 source runs replay PASS",
        ),
        check(
            "semantic-trace-equal",
            semantic_equal,
            "normalized closed semantics match after stripping only redundant neutral shutdown-tail submissions",
        ),
    ];
    let passed = checks.iter().all(|check| check.passed);
    let report = ActiveComparisonReport {
        schema_version: ACTIVE_ARTIFACT_SCHEMA_VERSION,
        evidence_class: EVIDENCE_PIPELINE_RUNTIME.to_owned(),
        result: if passed { "PASS" } else { "FAIL" }.to_owned(),
        eligible_for_active_gate: passed,
        first_artifact_identity: first_manifest.artifact_identity,
        second_artifact_identity: second_manifest.artifact_identity,
        first_analysis_sha256: sha256_hex(
            &serde_json::to_vec(&first_analysis).usage_context("encode first active analysis")?,
        ),
        second_analysis_sha256: sha256_hex(
            &serde_json::to_vec(&second_analysis).usage_context("encode second active analysis")?,
        ),
        semantic_trace_sha256: sha256_hex(semantic_first.as_bytes()),
        checks,
    };
    if let Some(path) = output {
        write_new_json(path, &report)?;
    }
    Ok(report)
}

pub fn validate_active_matrix_shape(matrix: &ActiveMatrixV2) -> AppResult<()> {
    if matrix.schema_version != ACTIVE_MATRIX_SCHEMA_VERSION
        || matrix.matrix_identity != ACTIVE_MATRIX_IDENTITY
        || matrix.cases.len() != ACTIVE_SCENARIOS.len()
        || matrix.repeatability_analyses.len() != 2
        || !matrix
            .cases
            .iter()
            .map(|item| item.case_name.as_str())
            .eq(ACTIVE_SCENARIOS)
    {
        return Err(AppError::evidence(
            "active input matrix is not the exact schema-2 ordered 18+2 identity",
        ));
    }
    let mut primary_paths = BTreeSet::new();
    for item in &matrix.cases {
        validate_matrix_path(&item.analysis_path)?;
        if !primary_paths.insert(item.analysis_path.as_path()) {
            return Err(AppError::evidence(
                "active matrix duplicates a primary analysis path",
            ));
        }
    }
    let mut repeatability_paths = BTreeSet::new();
    for path in &matrix.repeatability_analyses {
        validate_matrix_path(path)?;
        if primary_paths.contains(path.as_path()) || !repeatability_paths.insert(path.as_path()) {
            return Err(AppError::evidence(
                "active matrix repeatability paths must be two extra unique sources",
            ));
        }
    }
    Ok(())
}

#[allow(clippy::too_many_arguments)]
pub fn write_active_gate(
    mode_gate: &Path,
    loop_gate: &Path,
    raw_prereq: &Path,
    active_plan: &Path,
    matrix_path: &Path,
    repeatability_path: &Path,
    output: &Path,
) -> AppResult<ActiveGateDecision> {
    write_active_gate_internal(
        mode_gate,
        loop_gate,
        raw_prereq,
        active_plan,
        matrix_path,
        repeatability_path,
        None,
        output,
    )
}

#[allow(clippy::too_many_arguments)]
pub fn write_active_gate_with_verification(
    mode_gate: &Path,
    loop_gate: &Path,
    raw_prereq: &Path,
    active_plan: &Path,
    matrix_path: &Path,
    repeatability_path: &Path,
    verification_path: &Path,
    output: &Path,
) -> AppResult<ActiveGateDecision> {
    if output.exists() {
        return Err(AppError::usage(format!(
            "refusing to overwrite {}",
            output.display()
        )));
    }
    let verification = read_runtime_active_verification_v2(verification_path)?;
    write_active_gate_internal(
        mode_gate,
        loop_gate,
        raw_prereq,
        active_plan,
        matrix_path,
        repeatability_path,
        Some(&verification),
        output,
    )
}

#[allow(clippy::too_many_arguments)]
fn write_active_gate_internal(
    mode_gate: &Path,
    loop_gate: &Path,
    raw_prereq: &Path,
    active_plan: &Path,
    matrix_path: &Path,
    repeatability_path: &Path,
    verification: Option<&ValidatedActiveVerification>,
    output: &Path,
) -> AppResult<ActiveGateDecision> {
    if output.exists() {
        return Err(AppError::usage(format!(
            "refusing to overwrite {}",
            output.display()
        )));
    }
    if matrix_path.parent().unwrap_or_else(|| Path::new("."))
        != output.parent().unwrap_or_else(|| Path::new("."))
    {
        return Err(AppError::usage(
            "active matrix and gate output must share a directory so evidence bindings remain replayable",
        ));
    }
    let matrix_bytes = fs::read(matrix_path).usage_context(format!(
        "cannot read active matrix {}",
        matrix_path.display()
    ))?;
    let matrix: ActiveMatrixV2 = serde_json::from_slice(&matrix_bytes).usage_context(format!(
        "cannot parse active matrix {}",
        matrix_path.display()
    ))?;
    validate_active_matrix_shape(&matrix)?;
    let validated = validate_active_plan(active_plan, mode_gate, loop_gate, raw_prereq)?;
    let parent = matrix_path.parent().unwrap_or_else(|| Path::new("."));
    let mut seen_cases = BTreeSet::new();
    let mut seen_artifacts = BTreeSet::<String>::new();
    let mut seen_tokens = BTreeSet::<String>::new();
    let mut seen_equivalences = BTreeSet::<String>::new();
    let mut bindings = Vec::new();
    for item in &matrix.cases {
        if !seen_cases.insert(item.case_name.as_str()) {
            return Err(AppError::evidence(
                "active matrix duplicates a primary case after shape validation",
            ));
        }
        let path = parent.join(&item.analysis_path);
        let (stored, replay, manifest) = load_bound_analysis(&path, &validated)?;
        seen_equivalences.insert(manifest.target_equivalence_sha256.clone());
        if stored.scenario != item.case_name
            || !seen_artifacts.insert(stored.artifact_identity.clone())
        {
            return Err(AppError::evidence(
                "active matrix primary case or artifact identity is duplicated",
            ));
        }
        let expected = if matches!(
            item.case_name.as_str(),
            "control-no-native" | "original-online-disarmed"
        ) {
            EVIDENCE_DISARMED_CONTROL
        } else {
            EVIDENCE_PIPELINE_RUNTIME
        };
        if replay.evidence_class != expected {
            return Err(AppError::evidence(
                "active matrix case has the wrong evidence class",
            ));
        }
        if expected == EVIDENCE_PIPELINE_RUNTIME
            && !manifest
                .arm_token_sha256
                .as_ref()
                .is_some_and(|token| seen_tokens.insert(token.clone()))
        {
            return Err(AppError::evidence(
                "active matrix reuses a one-time arm token across primary sources",
            ));
        }
        bindings.push(ActiveAnalysisBinding {
            case_name: item.case_name.clone(),
            artifact_identity: replay.artifact_identity,
            analysis_path: item.analysis_path.clone(),
            analysis_sha256: file_sha256(&path)?,
            manifest_sha256: file_sha256(
                &path
                    .parent()
                    .expect("analysis has parent")
                    .join("manifest.json"),
            )?,
            events_sha256: file_sha256(
                &path
                    .parent()
                    .expect("analysis has parent")
                    .join("events.jsonl"),
            )?,
        });
    }
    let required = ACTIVE_SCENARIOS.into_iter().collect::<BTreeSet<_>>();
    let missing = required
        .difference(&seen_cases)
        .map(|value| (*value).to_owned())
        .collect::<Vec<_>>();

    let repeat_paths = matrix
        .repeatability_analyses
        .iter()
        .map(|relative| Ok(parent.join(relative)))
        .collect::<AppResult<Vec<_>>>()?;
    let (_, first_replay, first_manifest) = load_bound_analysis(&repeat_paths[0], &validated)?;
    let (_, second_replay, second_manifest) = load_bound_analysis(&repeat_paths[1], &validated)?;
    seen_equivalences.insert(first_manifest.target_equivalence_sha256.clone());
    seen_equivalences.insert(second_manifest.target_equivalence_sha256.clone());
    if first_replay.scenario != "loopback"
        || second_replay.scenario != "loopback"
        || seen_artifacts.contains(&first_replay.artifact_identity)
        || seen_artifacts.contains(&second_replay.artifact_identity)
        || first_replay.artifact_identity == second_replay.artifact_identity
        || seen_equivalences.len() != 1
        || !first_manifest
            .arm_token_sha256
            .as_ref()
            .is_some_and(|token| seen_tokens.insert(token.clone()))
        || !second_manifest
            .arm_token_sha256
            .as_ref()
            .is_some_and(|token| seen_tokens.insert(token.clone()))
    {
        return Err(AppError::evidence(
            "active matrix repeatability sources must use extra artifact and one-time token identities",
        ));
    }
    let repeatability_bytes = fs::read(repeatability_path).usage_context(format!(
        "cannot read active repeatability {}",
        repeatability_path.display()
    ))?;
    let stored_repeatability: ActiveComparisonReport = serde_json::from_slice(&repeatability_bytes)
        .usage_context(format!(
            "cannot parse active repeatability {}",
            repeatability_path.display()
        ))?;
    let replayed_repeatability = compare_active_runs(
        repeat_paths[0].parent().unwrap(),
        repeat_paths[1].parent().unwrap(),
        None,
    )?;
    if stored_repeatability != replayed_repeatability
        || replayed_repeatability.result != "PASS"
        || !replayed_repeatability.eligible_for_active_gate
    {
        return Err(AppError::evidence(
            "active repeatability report does not match its two source replays",
        ));
    }
    let repeatability_bindings = repeat_paths
        .iter()
        .zip(matrix.repeatability_analyses.iter())
        .enumerate()
        .map(|(index, (absolute, relative))| {
            let run = absolute
                .parent()
                .expect("repeatability analysis has parent");
            Ok(ActiveRepeatabilityBinding {
                source_index: index as u32 + 1,
                artifact_identity: if index == 0 {
                    first_replay.artifact_identity.clone()
                } else {
                    second_replay.artifact_identity.clone()
                },
                analysis_path: relative.clone(),
                analysis_sha256: file_sha256(absolute)?,
                manifest_sha256: file_sha256(&run.join("manifest.json"))?,
                events_sha256: file_sha256(&run.join("events.jsonl"))?,
            })
        })
        .collect::<AppResult<Vec<_>>>()?;
    let evidence_complete = missing.is_empty();
    let semantic_contract_complete = ACTIVE_SEMANTIC_CONTRACT_COMPLETE;
    let runtime_contract_complete = ACTIVE_RUNTIME_CONTRACT_COMPLETE;
    if verification.is_some_and(|verification| {
        verification.semantic_contract_complete != semantic_contract_complete
            || verification.runtime_contract_complete != runtime_contract_complete
    }) {
        return Err(AppError::evidence(
            "active verification disagrees with the compiled semantic/runtime contract",
        ));
    }
    let verification_complete = verification.is_some_and(|verification| {
        verification.semantic_contract_complete
            && verification.runtime_contract_complete
            && !verification.active_input_allowed
    });
    let unlocked = evidence_complete
        && semantic_contract_complete
        && runtime_contract_complete
        && verification_complete;
    let decision = ActiveGateDecision {
        schema_version: ACTIVE_ARTIFACT_SCHEMA_VERSION,
        status: if unlocked {
            "PASS"
        } else if evidence_complete && !semantic_contract_complete {
            "BLOCKED_BY_UNIMPLEMENTED_ACTIVE_SEMANTICS"
        } else if evidence_complete && !runtime_contract_complete {
            "BLOCKED_BY_DEFERRED_SCHEMA4_NATIVE"
        } else if evidence_complete && !verification_complete {
            "BLOCKED_BY_ACTIVE_VERIFICATION"
        } else {
            "BLOCKED_BY_EVIDENCE"
        }
        .to_owned(),
        t0_04_unlocked: unlocked,
        active_input_allowed: unlocked,
        semantic_contract_complete,
        runtime_contract_complete,
        active_verification_sha256: verification.map(|value| value.sha256.clone()),
        target_sha256: validated.target_sha256,
        observed_target_sha256: crate::target_equivalence::APPROVED_UNPACKED_TARGET_SHA256
            .to_owned(),
        target_equivalence_sha256: seen_equivalences
            .into_iter()
            .next()
            .expect("active matrix has at least one target equivalence"),
        mode_gate_sha256: validated.mode_gate_sha256,
        loop_gate_sha256: validated.loop_gate_sha256,
        raw_prereq_sha256: validated.raw_prereq_sha256,
        active_catalog_sha256: validated.catalog_sha256,
        active_plan_sha256: validated.plan_sha256,
        matrix_sha256: sha256_hex(&matrix_bytes),
        repeatability_sha256: sha256_hex(&repeatability_bytes),
        required_cases: ACTIVE_SCENARIOS
            .iter()
            .map(|value| (*value).to_owned())
            .collect(),
        missing_cases: missing,
        analysis_bindings: bindings,
        repeatability_bindings,
        repeatability_first_artifact_identity: first_replay.artifact_identity,
        repeatability_second_artifact_identity: second_replay.artifact_identity,
    };
    write_new_json(output, &decision)?;
    Ok(decision)
}

fn load_bound_analysis(
    analysis_path: &Path,
    validated: &ValidatedActivePlan,
) -> AppResult<(ActiveAnalysisReport, ActiveAnalysisReport, ActiveManifest)> {
    let stored: ActiveAnalysisReport =
        serde_json::from_slice(&fs::read(analysis_path).usage_context(format!(
            "cannot read active analysis {}",
            analysis_path.display()
        ))?)
        .usage_context(format!(
            "cannot parse active analysis {}",
            analysis_path.display()
        ))?;
    let run = analysis_path.parent().ok_or_else(|| {
        AppError::usage(format!(
            "active analysis {} has no run",
            analysis_path.display()
        ))
    })?;
    let replay = replay_active_run(run)?;
    let (manifest, _) = read_active_run(run)?;
    if stored != replay
        || replay.schema_version != ACTIVE_ARTIFACT_SCHEMA_VERSION
        || replay.result != "PASS"
        || !replay.eligible_for_active_gate
        || replay.checks.iter().any(|check| !check.passed)
        || manifest.target_sha256 != validated.target_sha256
        || manifest.mode_gate_sha256 != validated.mode_gate_sha256
        || manifest.loop_gate_sha256 != validated.loop_gate_sha256
        || manifest.raw_prereq_sha256 != validated.raw_prereq_sha256
        || manifest.active_catalog_sha256 != validated.catalog_sha256
        || manifest.active_plan_sha256 != validated.plan_sha256
        || if manifest.scenario == "loopback" {
            manifest.input_script_sha256.as_deref()
                != Some(validated.repeatability_input_script_sha256.as_str())
        } else {
            manifest.input_script_sha256.is_some()
        }
    {
        return Err(AppError::evidence(format!(
            "active analysis {} is not an exact bound PASS replay",
            analysis_path.display()
        )));
    }
    Ok((stored, replay, manifest))
}

fn validate_pipeline_companion_binding(
    run_directory: &Path,
    manifest: &ActiveManifest,
    active_events: &[ActiveEvent],
) -> AppResult<()> {
    let relative = manifest
        .companion_loop_artifact_path
        .as_deref()
        .ok_or_else(|| AppError::evidence("active pipeline manifest lacks companion loop path"))?;
    validate_matrix_path(relative)?;
    if relative != Path::new("loop-context") {
        return Err(AppError::evidence(
            "active pipeline companion must use the exact loop-context path",
        ));
    }
    let companion = run_directory.join(relative);
    let manifest_path = companion.join("manifest.json");
    let events_path = companion.join("events.jsonl");
    let analysis_path = companion.join("analysis.json");
    let manifest_bytes = fs::read(&manifest_path).usage_context(format!(
        "cannot read active companion loop manifest {}",
        manifest_path.display()
    ))?;
    let events_bytes = fs::read(&events_path).usage_context(format!(
        "cannot read active companion loop events {}",
        events_path.display()
    ))?;
    let analysis_bytes = fs::read(&analysis_path).usage_context(format!(
        "cannot read active companion loop analysis {}",
        analysis_path.display()
    ))?;
    let loop_manifest: LoopManifest = serde_json::from_slice(&manifest_bytes)
        .usage_context("cannot parse active companion loop manifest")?;
    if loop_manifest.schema_version != 2
        || loop_manifest.experiment_identity != LOOP_TOOL_IDENTITY
        || loop_manifest.evidence_class != LOOP_EVIDENCE_CLASS
        || loop_manifest.probe != "passive"
        || loop_manifest.scenario != manifest.scenario
        || loop_manifest.state != "COMPLETE"
        || loop_manifest.active_input_allowed
        || loop_manifest.target_sha256 != manifest.target_sha256
        || loop_manifest.main_thread_id != manifest.main_thread_id
        || loop_manifest.hooks_mask != LOOP_HOOK_MASK
        || loop_manifest.overwritten_events != 0
        || !loop_manifest.target_bytes_restored
        || loop_manifest.target_bytes_before_sha256 != loop_manifest.target_bytes_after_sha256
    {
        return Err(AppError::evidence(
            "active companion V5 manifest lacks exact passive/restore/32-slot identity",
        ));
    }
    let stored: LoopAnalysisReport = serde_json::from_slice(&analysis_bytes)
        .usage_context("cannot parse active companion loop analysis")?;
    let replayed = replay_loop_run(&companion)?;
    let companion_manifest_sha256 = sha256_hex(&manifest_bytes);
    let companion_events_sha256 = sha256_hex(&events_bytes);
    let companion_analysis_sha256 = sha256_hex(&analysis_bytes);
    if stored != replayed
        || stored.target_sha256 != manifest.target_sha256
        || stored.manifest_sha256 != companion_manifest_sha256
        || stored.events_sha256 != companion_events_sha256
        || manifest.companion_loop_manifest_sha256.as_deref()
            != Some(companion_manifest_sha256.as_str())
        || manifest.companion_loop_events_sha256.as_deref()
            != Some(companion_events_sha256.as_str())
        || manifest.companion_loop_analysis_sha256.as_deref()
            != Some(companion_analysis_sha256.as_str())
    {
        return Err(AppError::evidence(
            "active companion V5 analysis or source hashes do not replay exactly",
        ));
    }
    if !companion_analysis_policy_valid(&manifest.scenario, &replayed) {
        return Err(AppError::evidence(
            "active companion V5 analysis violates the common-check or scenario-authority policy",
        ));
    }
    let mut loop_events = Vec::new();
    let mut loop_bounds = BTreeMap::<u64, (i64, i64)>::new();
    for (index, line) in events_bytes.split(|byte| *byte == b'\n').enumerate() {
        if line.is_empty() {
            continue;
        }
        let event: LoopEvent = serde_json::from_slice(line).usage_context(format!(
            "cannot parse active companion loop event line {}",
            index + 1
        ))?;
        if event.thread_id == loop_manifest.main_thread_id && event.loop_iteration != 0 {
            loop_bounds
                .entry(event.loop_iteration)
                .and_modify(|bounds| {
                    bounds.0 = bounds.0.min(event.qpc);
                    bounds.1 = bounds.1.max(event.qpc);
                })
                .or_insert((event.qpc, event.qpc));
        }
        loop_events.push(event);
    }
    if loop_bounds.is_empty()
        || !active_control_plane_matches_companion(active_events, &loop_events)
        || !active_data_plane_has_loop_context(active_events, &loop_bounds, manifest.main_thread_id)
    {
        return Err(AppError::evidence(
            "active control ordering or data-plane same-main-thread/QPC companion V5 context is invalid",
        ));
    }
    if !lifecycle_companion_shape_valid(manifest, active_events, &loop_events) {
        return Err(AppError::evidence(
            "active lifecycle labels are not derived from the required V5 transaction shape",
        ));
    }
    Ok(())
}

fn companion_analysis_policy_valid(scenario: &str, report: &LoopAnalysisReport) -> bool {
    let failed = report
        .checks
        .iter()
        .filter(|check| !check.passed)
        .collect::<Vec<_>>();
    if matches!(scenario, "pause" | "loading" | "room" | "menu") || failed.is_empty() {
        return report.result == "PASS" && report.eligible_for_active_gate && failed.is_empty();
    }
    report.result == "FAIL"
        && !report.eligible_for_active_gate
        && failed.len() == 1
        && failed[0].id == "scenario-evidence"
}

const ACTIVE_CONTROL_LIFECYCLES: [&str; 4] = [
    "INITIALIZED",
    "HOOKS_INSTALLED",
    "HOOKS_RESTORED",
    "PRODUCER_CLOSED",
];

fn active_control_lifecycle(event: &ActiveEvent) -> Option<&str> {
    if event.stage != "lifecycle" || event.phase != "OUTSIDE_AUTHORITY" {
        return None;
    }
    event
        .lifecycle_kind
        .as_deref()
        .filter(|kind| ACTIVE_CONTROL_LIFECYCLES.contains(kind))
}

fn active_data_plane_has_loop_context(
    active_events: &[ActiveEvent],
    loop_bounds: &BTreeMap<u64, (i64, i64)>,
    main_thread_id: u32,
) -> bool {
    active_events
        .iter()
        .filter(|event| active_control_lifecycle(event).is_none())
        .all(|event| {
            let Some((minimum, maximum)) = loop_bounds.get(&event.loop_iteration) else {
                return false;
            };
            event.thread_id == main_thread_id
                && event.loop_iteration != 0
                && event.qpc <= i64::MAX as u64
                && (event.qpc as i64) >= *minimum
                && (event.qpc as i64) <= *maximum
        })
}

fn active_control_plane_matches_companion(
    active_events: &[ActiveEvent],
    loop_events: &[LoopEvent],
) -> bool {
    let active_control = active_events
        .iter()
        .filter_map(|event| active_control_lifecycle(event).map(|kind| (kind, event)))
        .collect::<Vec<_>>();
    if active_control.len() != ACTIVE_CONTROL_LIFECYCLES.len()
        || active_control
            .iter()
            .map(|(kind, _)| *kind)
            .ne(ACTIVE_CONTROL_LIFECYCLES)
    {
        return false;
    }
    let exact_loop_control = |kind: &str| {
        let mut matches = loop_events.iter().filter(|event| event.kind == kind);
        let first = matches.next()?;
        matches.next().is_none().then_some(first)
    };
    let Some(loop_initialized) = exact_loop_control("NATIVE_INITIALIZED") else {
        return false;
    };
    let Some(loop_installed) = exact_loop_control("HOOKS_INSTALLED") else {
        return false;
    };
    let Some(loop_restored) = exact_loop_control("HOOKS_RESTORED") else {
        return false;
    };
    let initialized = active_control[0].1;
    let installed = active_control[1].1;
    let restored = active_control[2].1;
    let closed = active_control[3].1;
    loop_initialized.qpc >= 0
        && loop_installed.qpc >= 0
        && loop_restored.qpc >= 0
        && initialized.qpc <= i64::MAX as u64
        && installed.qpc <= i64::MAX as u64
        && restored.qpc <= i64::MAX as u64
        && closed.qpc <= i64::MAX as u64
        && initialized.qpc < loop_initialized.qpc as u64
        && loop_installed.qpc as u64 <= installed.qpc
        && loop_restored.qpc as u64 <= restored.qpc
        && restored.qpc < closed.qpc
        && initialized.thread_id == loop_initialized.thread_id
        && installed.thread_id == loop_installed.thread_id
        && restored.thread_id == loop_restored.thread_id
        && closed.thread_id == loop_restored.thread_id
        && initialized.loop_iteration == loop_initialized.loop_iteration
        && installed.loop_iteration == loop_installed.loop_iteration
        && restored.loop_iteration == loop_restored.loop_iteration
        && closed.loop_iteration == loop_restored.loop_iteration
}

fn lifecycle_companion_shape_valid(
    manifest: &ActiveManifest,
    active_events: &[ActiveEvent],
    loop_events: &[LoopEvent],
) -> bool {
    match manifest.scenario.as_str() {
        "pause" => pause_gap_matches(active_events, loop_events),
        "loading" => lifecycle_transaction_matches(
            active_events,
            loop_events,
            "LOADING_ENTER",
            "LOADING_EXIT",
            &[
                "ALTERNATE_OWNER_ENTER",
                "ALTERNATE_STATE_APPLY_ENTER",
                "ALTERNATE_STATE_APPLY_EXIT",
                "ALTERNATE_ROOM_ENTER",
                "ENTITY_LIST_ENTER",
                "ENTITY_LIST_EXIT",
                "ALTERNATE_ROOM_EXIT",
                "ALTERNATE_TAIL_ENTER",
                "ALTERNATE_TAIL_EXIT",
                "ALTERNATE_OWNER_EXIT",
            ],
        ),
        "room" => lifecycle_transaction_matches(
            active_events,
            loop_events,
            "ROOM_ENTER",
            "ROOM_EXIT",
            &[
                "ROOM_CHANGE_OWNER_ENTER",
                "NEW_ROOM_ENTER",
                "NEW_ROOM_EXIT",
                "ROOM_CHANGE_SIMULATION_ENTER",
                "ENTITY_LIST_ENTER",
                "ENTITY_LIST_EXIT",
                "ROOM_CHANGE_SIMULATION_EXIT",
                "ROOM_CHANGE_OWNER_EXIT",
            ],
        ),
        "menu" => menu_retirement_matches(active_events, loop_events),
        _ => true,
    }
}

fn active_lifecycle_pair<'a>(
    active_events: &'a [ActiveEvent],
    enter: &str,
    exit: &str,
) -> Option<(&'a ActiveEvent, &'a ActiveEvent)> {
    let enters = active_events
        .iter()
        .filter(|event| {
            event.stage == "lifecycle" && event.lifecycle_kind.as_deref() == Some(enter)
        })
        .collect::<Vec<_>>();
    let exits = active_events
        .iter()
        .filter(|event| event.stage == "lifecycle" && event.lifecycle_kind.as_deref() == Some(exit))
        .collect::<Vec<_>>();
    (enters.len() == 1 && exits.len() == 1 && enters[0].qpc < exits[0].qpc)
        .then_some((enters[0], exits[0]))
}

fn ordered_loop_transaction<'a>(
    loop_events: &'a [LoopEvent],
    loop_iteration: u64,
    kinds: &[&str],
) -> Option<Vec<&'a LoopEvent>> {
    let mut cursor = 0_usize;
    let mut matched = Vec::with_capacity(kinds.len());
    for kind in kinds {
        let (offset, event) = loop_events[cursor..]
            .iter()
            .enumerate()
            .find(|(_, event)| event.loop_iteration == loop_iteration && event.kind == *kind)?;
        cursor += offset + 1;
        matched.push(event);
    }
    Some(matched)
}

fn lifecycle_transaction_matches(
    active_events: &[ActiveEvent],
    loop_events: &[LoopEvent],
    enter: &str,
    exit: &str,
    kinds: &[&str],
) -> bool {
    let Some((active_enter, active_exit)) = active_lifecycle_pair(active_events, enter, exit)
    else {
        return false;
    };
    if active_enter.loop_iteration != active_exit.loop_iteration {
        return false;
    }
    ordered_loop_transaction(loop_events, active_enter.loop_iteration, kinds).is_some_and(
        |events| {
            active_enter.qpc >= events[0].qpc as u64
                && active_exit.qpc <= events[events.len() - 1].qpc as u64
        },
    )
}

fn pause_gap_matches(active_events: &[ActiveEvent], loop_events: &[LoopEvent]) -> bool {
    let Some((enter, exit)) = active_lifecycle_pair(active_events, "PAUSE_ENTER", "PAUSE_EXIT")
    else {
        return false;
    };
    let update_loops = loop_events
        .iter()
        .filter(|event| event.kind == "UPDATE_ENTER")
        .map(|event| event.loop_iteration)
        .collect::<BTreeSet<_>>();
    let authority_loops = loop_events
        .iter()
        .filter(|event| event.kind == "STANDARD_ROOM_ENTER")
        .map(|event| event.loop_iteration)
        .collect::<BTreeSet<_>>();
    let ordered_updates = update_loops.iter().copied().collect::<Vec<_>>();
    let Some(enter_position) = ordered_updates
        .iter()
        .position(|iteration| *iteration == enter.loop_iteration)
    else {
        return false;
    };
    if enter_position < 2 || !update_loops.contains(&exit.loop_iteration) {
        return false;
    }
    let authority_before_gap = ordered_updates[enter_position - 2];
    let first_missing = ordered_updates[enter_position - 1];
    let first_authority_after_enter = authority_loops
        .range((
            std::ops::Bound::Excluded(enter.loop_iteration),
            std::ops::Bound::Unbounded,
        ))
        .next()
        .copied();
    let exact_loop_qpc = |kind: &str, iteration: u64| {
        let mut matches = loop_events
            .iter()
            .filter(|event| event.kind == kind && event.loop_iteration == iteration);
        let first = matches.next()?;
        matches.next().is_none().then_some(first.qpc)
    };
    let enter_update_qpc = exact_loop_qpc("UPDATE_ENTER", enter.loop_iteration);
    let enter_update_exit_qpc = exact_loop_qpc("UPDATE_EXIT", enter.loop_iteration);
    let exit_update_qpc = exact_loop_qpc("UPDATE_ENTER", exit.loop_iteration);
    let exit_update_exit_qpc = exact_loop_qpc("UPDATE_EXIT", exit.loop_iteration);
    let exit_authority_qpc = exact_loop_qpc("STANDARD_ROOM_ENTER", exit.loop_iteration);
    let exit_authority_exit_qpc = exact_loop_qpc("STANDARD_ROOM_EXIT", exit.loop_iteration);
    let ordered_qpc_contains = |minimum: Option<i64>, value: u64, maximum: Option<i64>| {
        minimum.zip(maximum).is_some_and(|(minimum, maximum)| {
            minimum >= 0
                && minimum <= maximum
                && value <= i64::MAX as u64
                && minimum as u64 <= value
                && value <= maximum as u64
        })
    };
    let update_range_well_formed =
        update_loops
            .range(first_missing..=exit.loop_iteration)
            .all(|iteration| {
                exact_loop_qpc("UPDATE_ENTER", *iteration)
                    .zip(exact_loop_qpc("UPDATE_EXIT", *iteration))
                    .is_some_and(|(enter_qpc, exit_qpc)| enter_qpc >= 0 && enter_qpc < exit_qpc)
            });
    enter.loop_iteration < exit.loop_iteration
        && update_range_well_formed
        && !authority_loops.contains(&first_missing)
        && !authority_loops.contains(&enter.loop_iteration)
        && authority_loops.contains(&authority_before_gap)
        && update_loops
            .range(first_missing..exit.loop_iteration)
            .all(|iteration| !authority_loops.contains(iteration))
        && first_authority_after_enter == Some(exit.loop_iteration)
        && ordered_qpc_contains(enter_update_qpc, enter.qpc, enter_update_exit_qpc)
        && ordered_qpc_contains(exit_update_qpc, exit.qpc, exit_update_exit_qpc)
        && ordered_qpc_contains(exit_authority_qpc, exit.qpc, exit_authority_exit_qpc)
}

fn menu_retirement_matches(active_events: &[ActiveEvent], loop_events: &[LoopEvent]) -> bool {
    let boundaries = active_events
        .iter()
        .filter(|event| {
            event.stage == "lifecycle"
                && matches!(
                    event.lifecycle_kind.as_deref(),
                    Some("RUN_TEARDOWN" | "MENU_ENTER")
                )
        })
        .collect::<Vec<_>>();
    if boundaries.len() != 1 {
        return false;
    }
    let boundary = boundaries[0];
    let enters = loop_events
        .iter()
        .filter(|event| {
            event.loop_iteration == boundary.loop_iteration && event.kind == "RUN_TEARDOWN_ENTER"
        })
        .collect::<Vec<_>>();
    let exits = loop_events
        .iter()
        .filter(|event| {
            event.loop_iteration == boundary.loop_iteration && event.kind == "RUN_TEARDOWN_EXIT"
        })
        .collect::<Vec<_>>();
    enters.len() == 1
        && exits.len() == 1
        && enters[0].qpc <= boundary.qpc as i64
        && boundary.qpc as i64 <= exits[0].qpc
}

fn validate_manifest(manifest: &ActiveManifest) -> AppResult<()> {
    let pipeline = manifest.evidence_class == EVIDENCE_PIPELINE_RUNTIME;
    let disarmed = manifest.evidence_class == EVIDENCE_DISARMED_CONTROL;
    if manifest.schema_version != ACTIVE_ARTIFACT_SCHEMA_VERSION
        || manifest.experiment_identity != ACTIVE_EXPERIMENT_IDENTITY
        || (!pipeline && !disarmed)
        || manifest.state != "COMPLETE"
        || manifest.case_id.is_empty()
        || manifest.artifact_identity.is_empty()
        || manifest.target_sha256 != FIXED_TARGET_SHA256
        || manifest.observed_target_sha256
            != crate::target_equivalence::APPROVED_UNPACKED_TARGET_SHA256
        || !valid_sha256(&manifest.target_equivalence_sha256)
        || !valid_sha256(&manifest.mode_gate_sha256)
        || !valid_sha256(&manifest.loop_gate_sha256)
        || !valid_sha256(&manifest.raw_prereq_sha256)
        || !valid_sha256(&manifest.active_catalog_sha256)
        || !valid_sha256(&manifest.active_plan_sha256)
        || manifest.session_id.len() != 32
        || !manifest
            .session_id
            .bytes()
            .all(|byte| byte.is_ascii_hexdigit())
        || manifest.client_id == 0
        || manifest.failure_reason.is_some()
        || manifest.target_slots_before.len() != ACTIVE_SLOT_COUNT
        || manifest.target_slots_before != manifest.target_slots_after
        || !target_slots_valid(&manifest.target_slots_before)
        || !manifest.target_bytes_restored
        || manifest.restored_slot_count != ACTIVE_RESTORED_SLOT_COUNT
        || manifest.overwritten_events != 0
        || !manifest.producer_closed
        || !manifest.initial_snapshot_neutral
    {
        return Err(AppError::evidence(
            "invalid schema-4 active manifest identity",
        ));
    }
    if pipeline {
        if manifest.probe != "loopback"
            || manifest.endpoint != "offline-local"
            || manifest.ipc_magic != ACTIVE_IPC_SOURCE_FORMAT
            || manifest.ipc_schema_version != Some(ACTIVE_IPC_SCHEMA_VERSION)
            || manifest.original_online_observation.is_some()
            || manifest
                .arm_token_sha256
                .as_deref()
                .is_none_or(|hash| !valid_sha256(hash))
            || !manifest.arm_token_consumed
            || manifest.main_thread_id == 0
            || manifest.apply_generation == 0
            || parse_mask(&manifest.installed_active_hooks_mask) != Some(ACTIVE_HOOK_MASK)
            || parse_mask(&manifest.restored_active_hooks_mask) != Some(ACTIVE_HOOK_MASK)
            || manifest.companion_loop_artifact_path.as_deref() != Some(Path::new("loop-context"))
            || manifest
                .companion_loop_manifest_sha256
                .as_deref()
                .is_none_or(|hash| !valid_sha256(hash))
            || manifest
                .companion_loop_events_sha256
                .as_deref()
                .is_none_or(|hash| !valid_sha256(hash))
            || manifest
                .companion_loop_analysis_sha256
                .as_deref()
                .is_none_or(|hash| !valid_sha256(hash))
        {
            return Err(AppError::evidence(
                "active pipeline manifest lacks exact arm, IPC, main-thread or 13-slot identity",
            ));
        }
    } else if !matches!(
        manifest.probe.as_str(),
        "control-no-native" | "original-online-disarmed"
    ) || manifest.ipc_magic != ACTIVE_CONTROL_SOURCE_FORMAT
        || manifest.ipc_schema_version.is_some()
        || manifest.arm_token_sha256.is_some()
        || manifest.arm_token_consumed
        || manifest.companion_loop_artifact_path.is_some()
        || manifest.companion_loop_manifest_sha256.is_some()
        || manifest.companion_loop_events_sha256.is_some()
        || manifest.companion_loop_analysis_sha256.is_some()
        || parse_mask(&manifest.installed_active_hooks_mask) != Some(0)
        || parse_mask(&manifest.restored_active_hooks_mask) != Some(0)
    {
        return Err(AppError::evidence(
            "schema-4 disarmed control contains active runtime identity",
        ));
    }
    if disarmed {
        let valid_disarmed_binding = match (
            manifest.probe.as_str(),
            manifest.scenario.as_str(),
            manifest.endpoint.as_str(),
            manifest.original_online_observation.as_ref(),
        ) {
            ("control-no-native", "control-no-native", "disarmed-control", None) => true,
            (
                "original-online-disarmed",
                "original-online-disarmed",
                "original-online-read-only",
                Some(observation),
            ) => original_online_observation_valid(observation),
            _ => false,
        };
        if !valid_disarmed_binding {
            return Err(AppError::evidence(
                "schema-4 disarmed control has an invalid endpoint or Original Online observation",
            ));
        }
    }
    if manifest.scenario == "loopback"
        && manifest
            .input_script_sha256
            .as_deref()
            .is_none_or(|hash| !valid_sha256(hash))
    {
        return Err(AppError::evidence(
            "loopback repeatability source lacks a frozen input script hash",
        ));
    }
    if manifest.scenario != "loopback" && manifest.input_script_sha256.is_some() {
        return Err(AppError::evidence(
            "non-repeatability active source contains an input script hash",
        ));
    }
    Ok(())
}

fn valid_sha256(value: &str) -> bool {
    value.len() == 64 && value.bytes().all(|byte| byte.is_ascii_hexdigit())
}

fn target_slots_valid(slots: &[ActiveTargetSlot]) -> bool {
    slots.len() == ACTIVE_SLOT_COUNT
        && slots
            .iter()
            .zip(expected_hook_slots())
            .all(|(actual, expected)| {
                actual.ordinal == expected.ordinal
                    && actual.id == expected.id
                    && actual.rva == expected.rva
                    && hex::decode(&actual.bytes_hex)
                        .is_ok_and(|bytes| bytes.len() == expected.width as usize)
            })
}

fn event_shape_known(event: &ActiveEvent) -> bool {
    let known_stage = matches!(
        event.stage.as_str(),
        "lifecycle"
            | "control-observation"
            | "sample"
            | "canonical"
            | "send"
            | "receive"
            | "accept"
            | "reject"
            | "duplicate"
            | "resolve"
            | "apply-commit"
            | "suppression"
            | "query-override"
            | "witness"
    );
    let phase_matches = match event.stage.as_str() {
        "lifecycle" | "control-observation" => event.phase == "OUTSIDE_AUTHORITY",
        "sample" | "canonical" | "send" => event.phase == "PHYSICAL_SAMPLE",
        "receive" | "accept" | "reject" | "duplicate" => event.phase == "DRAIN_BEFORE_RESOLVE",
        "resolve" => event.phase == "RESOLVE",
        "apply-commit" => event.phase == "APPLY",
        "suppression" | "query-override" => event.phase == "INPUT_QUERY",
        "witness" => event.phase == "WITNESS",
        _ => false,
    };
    known_stage
        && phase_matches
        && event
            .decision
            .as_deref()
            .is_none_or(|value| matches!(value, "ACCEPTED" | "REJECTED" | "DUPLICATE"))
        && event.reject_reason.as_deref().is_none_or(|value| {
            matches!(
                value,
                "NONE"
                    | "QUEUE_FULL"
                    | "EXPIRED"
                    | "TOO_FAR"
                    | "STALE_SEQUENCE"
                    | "IDENTITY_CONFLICT"
            )
        })
}

fn command_payload_valid(event: &ActiveEvent) -> bool {
    if !is_command_stage(&event.stage) {
        return event.session_id.is_none()
            && event.client_id.is_none()
            && event.command_sequence.is_none()
            && event.target_tick.is_none()
            && event.canonical_state_hex.is_none()
            && event.edge_flags.is_none()
            && event.decision.is_none()
            && event.reject_reason.is_none()
            && event.conflicting_state_hex.is_none();
    }
    let Some(state) = event.canonical_state_hex.as_deref().and_then(decode_state) else {
        return false;
    };
    if event.session_id.as_deref().is_none_or(str::is_empty)
        || event.client_id.is_none_or(|client| client == 0)
        || event.command_sequence.is_none_or(|sequence| sequence == 0)
        || event.target_tick.is_none()
        || event
            .edge_flags
            .is_none_or(|edges| edges > 0xF || edges != u32::from(state.edge_flags()))
    {
        return false;
    }
    match event.stage.as_str() {
        "accept" => {
            event.decision.as_deref() == Some("ACCEPTED")
                && event.reject_reason.as_deref() == Some("NONE")
        }
        "reject" => {
            event.decision.as_deref() == Some("REJECTED")
                && matches!(
                    event.reject_reason.as_deref(),
                    Some(
                        "QUEUE_FULL"
                            | "EXPIRED"
                            | "TOO_FAR"
                            | "STALE_SEQUENCE"
                            | "IDENTITY_CONFLICT"
                    )
                )
        }
        "duplicate" => {
            event.decision.as_deref() == Some("DUPLICATE")
                && event.reject_reason.as_deref() == Some("NONE")
        }
        _ => event.decision.is_none() && event.reject_reason.is_none(),
    }
}

fn scenario_predicate(
    manifest: &ActiveManifest,
    events: &[ActiveEvent],
    groups: &BTreeMap<CommandIdentity, Vec<&ActiveEvent>>,
) -> bool {
    match manifest.scenario.as_str() {
        "control-no-native" => disarmed_control(manifest, events, "control-no-native"),
        "original-online-disarmed" => {
            disarmed_control(manifest, events, "original-online-disarmed")
        }
        "keyboard" => owner_actions(events, "KEYBOARD"),
        "controller" => owner_actions(events, "GAMEPAD"),
        "simultaneous" => events.iter().any(|event| {
            event.stage == "apply-commit"
                && event.sample_component_count.is_some_and(|count| count >= 2)
                && event
                    .canonical_state_hex
                    .as_deref()
                    .and_then(decode_state)
                    .is_some_and(|state| {
                        let (move_x, move_y) = state.move_axes();
                        let (aim_x, aim_y) = state.aim_axes();
                        (state.shoot_held()
                            && (move_x != 0 || move_y != 0 || aim_x != 0 || aim_y != 0))
                            || move_x != 0 && move_y != 0
                            || aim_x != 0 && aim_y != 0
                    })
        }),
        "rapid-tap" => (8_u32..=11).all(|action| {
            let bit = 1_u32 << (action - 8);
            let targets = events
                .iter()
                .filter(|event| {
                    event.stage == "apply-commit"
                        && event.edge_flags.is_some_and(|edges| edges & bit != 0)
                })
                .filter_map(|event| event.target_tick)
                .collect::<BTreeSet<_>>();
            targets.len() >= 2
        }),
        "device-switch" => owner_switch_isolated(events),
        "pause" => lifecycle_clear(events, "PAUSE_ENTER", "PAUSE_EXIT"),
        "loading" => {
            lifecycle_clear(events, "LOADING_ENTER", "LOADING_EXIT")
                && loading_ownerless_window_clear(events)
                && loading_post_exit_keyboard_closed_loop(manifest, events)
        }
        "room" => lifecycle_clear(events, "ROOM_ENTER", "ROOM_EXIT"),
        "menu" => events
            .iter()
            .position(|event| {
                event.stage == "lifecycle"
                    && matches!(
                        event.lifecycle_kind.as_deref(),
                        Some("RUN_TEARDOWN" | "MENU_ENTER")
                    )
            })
            .is_some_and(|index| {
                manifest.client_retired
                    && events[index + 1..]
                        .iter()
                        .all(|event| !is_command_stage(&event.stage))
            }),
        "queue-full" => exact_reject(events, "QUEUE_FULL", |event| event.queue_depth == Some(256)),
        "expired" => exact_reject(events, "EXPIRED", |event| {
            event
                .target_tick
                .is_some_and(|target| target < event.authority_tick)
        }),
        "too-far" => exact_reject(events, "TOO_FAR", |event| {
            event
                .target_tick
                .is_some_and(|target| target > event.authority_tick.saturating_add(64))
        }),
        "stale" => exact_reject(events, "STALE_SEQUENCE", |event| {
            event.previous_command_sequence.is_some_and(|previous| {
                event
                    .command_sequence
                    .is_some_and(|current| current < previous)
            })
        }),
        "conflict" => {
            manifest.client_retired
                && exact_reject(events, "IDENTITY_CONFLICT", |event| {
                    event
                        .conflicting_state_hex
                        .as_ref()
                        .zip(event.canonical_state_hex.as_ref())
                        .is_some_and(|(conflicting, canonical)| conflicting != canonical)
                })
        }
        "duplicate" => groups.values().any(|trace| {
            trace.iter().filter(|event| event.stage == "accept").count() == 1
                && trace
                    .iter()
                    .filter(|event| event.stage == "duplicate")
                    .count()
                    == 1
                && trace
                    .iter()
                    .filter(|event| event.stage == "apply-commit")
                    .count()
                    == 1
                && trace
                    .iter()
                    .find(|event| event.stage == "duplicate")
                    .is_some_and(|event| {
                        event.attempt > 0
                            && event.decision.as_deref() == Some("DUPLICATE")
                            && event.reject_reason.as_deref() == Some("NONE")
                    })
        }),
        "loopback" => loopback_scenario_complete(groups),
        _ => false,
    }
}

fn disarmed_control(manifest: &ActiveManifest, events: &[ActiveEvent], probe: &str) -> bool {
    manifest.evidence_class == EVIDENCE_DISARMED_CONTROL
        && manifest.probe == probe
        && !events.is_empty()
        && events
            .iter()
            .all(|event| matches!(event.stage.as_str(), "lifecycle" | "control-observation"))
        && events
            .iter()
            .any(|event| event.stage == "control-observation")
}

fn owner_actions(events: &[ActiveEvent], expected: &str) -> bool {
    let queries = events
        .iter()
        .filter(|event| event.stage == "query-override")
        .collect::<Vec<_>>();
    let generations = queries
        .iter()
        .filter_map(|event| event.owner_generation)
        .collect::<BTreeSet<_>>();
    let actions = events
        .iter()
        .filter(|event| {
            event.stage == "query-override"
                && event.source_device.as_deref() == Some(expected)
                && event
                    .owner_generation
                    .is_some_and(|generation| generation != 0)
                && event.original_calls == Some(1)
                && event.query_overridden == Some(true)
        })
        .filter_map(|event| event.action)
        .collect::<BTreeSet<_>>();
    !queries.is_empty()
        && queries.iter().all(|event| {
            event.source_device.as_deref() == Some(expected)
                && event
                    .owner_generation
                    .is_some_and(|generation| generation != 0)
        })
        && generations.len() == 1
        && actions == (0_u32..=11).collect()
        && events.iter().any(|event| event.stage == "apply-commit")
}

fn lifecycle_clear(events: &[ActiveEvent], enter: &str, exit: &str) -> bool {
    let Some(start) = events.iter().position(|event| {
        event.stage == "lifecycle" && event.lifecycle_kind.as_deref() == Some(enter)
    }) else {
        return false;
    };
    let Some(end) = events
        .iter()
        .enumerate()
        .skip(start + 1)
        .find_map(|(index, event)| {
            (event.stage == "lifecycle" && event.lifecycle_kind.as_deref() == Some(exit))
                .then_some(index)
        })
    else {
        return false;
    };
    events[start + 1..end]
        .iter()
        .all(|event| !is_command_stage(&event.stage))
        && no_stale_generation_apply(events)
}

fn loading_ownerless_window_clear(events: &[ActiveEvent]) -> bool {
    let Some(exit) = events.iter().position(|event| {
        event.stage == "lifecycle" && event.lifecycle_kind.as_deref() == Some("LOADING_EXIT")
    }) else {
        return false;
    };
    events[..=exit]
        .iter()
        .all(|event| !is_command_stage(&event.stage))
}

fn loading_post_exit_keyboard_closed_loop(
    manifest: &ActiveManifest,
    events: &[ActiveEvent],
) -> bool {
    let Some((exit_index, exit_generation)) =
        events.iter().enumerate().find_map(|(index, event)| {
            (event.stage == "lifecycle" && event.lifecycle_kind.as_deref() == Some("LOADING_EXIT"))
                .then_some((index, event.owner_generation?))
        })
    else {
        return false;
    };
    if exit_generation == 0 {
        return false;
    }
    let Some((sample_index, sample)) = events
        .iter()
        .enumerate()
        .skip(exit_index + 1)
        .find(|(_, event)| is_command_stage(&event.stage))
    else {
        return false;
    };
    if sample.stage != "sample"
        || sample.source_device.as_deref() != Some("KEYBOARD")
        || sample.owner_generation != Some(exit_generation)
    {
        return false;
    }
    let Some(command_identity) = identity(sample) else {
        return false;
    };
    let trace = events
        .iter()
        .enumerate()
        .skip(sample_index)
        .filter(|(_, event)| identity(event).as_ref() == Some(&command_identity))
        .collect::<Vec<_>>();
    if trace.iter().any(|(_, event)| {
        let source_invalid = if event.stage == "resolve" {
            // Resolve is an authority/queue result, not a physical-source event.
            // The Native contract intentionally publishes SourceDevice::None here.
            event.source_device.is_some()
        } else {
            event.source_device.as_deref() != Some("KEYBOARD")
        };
        source_invalid
            || event.owner_generation != Some(exit_generation)
            || event.apply_generation != Some(manifest.apply_generation)
    }) {
        return false;
    }
    let stage_after = |stage: &str, after: usize| {
        trace
            .iter()
            .find_map(|(index, event)| (*index > after && event.stage == stage).then_some(*index))
    };
    let Some(accept) = stage_after("accept", sample_index) else {
        return false;
    };
    let Some(apply) = stage_after("apply-commit", accept) else {
        return false;
    };
    let Some(query) = stage_after("query-override", apply) else {
        return false;
    };
    let Some(witness) = stage_after("witness", query) else {
        return false;
    };
    let query_shape = trace.iter().any(|(index, event)| {
        *index == query
            && event.original_calls == Some(1)
            && event.query_overridden == Some(true)
            && event.action.is_some_and(|action| action <= 11)
            && event.consumer.as_deref().is_some_and(allowed_consumer)
    });
    let witness_shape = trace.iter().any(|(index, event)| {
        *index == witness
            && event.world_witness_before.is_some()
            && event.world_witness_after.is_some()
    });
    query_shape && witness_shape
}

fn no_stale_generation_apply(events: &[ActiveEvent]) -> bool {
    generation_timeline_valid(events)
}

fn owner_switch_isolated(events: &[ActiveEvent]) -> bool {
    let switches = events
        .iter()
        .enumerate()
        .filter(|(_, event)| {
            event.stage == "lifecycle" && event.lifecycle_kind.as_deref() == Some("OWNER_SWITCH")
        })
        .collect::<Vec<_>>();
    if switches.len() != 1 {
        return false;
    }
    let (index, switch) = switches[0];
    let Some((before, after, generation)) = switch
        .owner_before
        .zip(switch.owner_after)
        .zip(switch.owner_generation)
        .map(|((before, after), generation)| (before, after, generation))
    else {
        return false;
    };
    before != after
        && events[index + 1..].iter().all(|event| {
            event.stage != "apply-commit" || event.owner_generation == Some(generation)
        })
        && events[index + 1..].iter().any(|event| {
            event.stage == "apply-commit" && event.owner_generation == Some(generation)
        })
}

fn loopback_trace_complete(trace: &[&ActiveEvent]) -> bool {
    let suppression = trace.iter().position(|event| {
        event.stage == "suppression"
            && event.query_overridden == Some(true)
            && event.original_calls == Some(1)
            && event.original_result_bits.is_some_and(|value| value != 0)
            && event.effective_result_bits == Some(0)
            && event
                .target_tick
                .is_some_and(|target| event.authority_tick < target)
    });
    let apply = trace
        .iter()
        .enumerate()
        .filter(|(_, event)| event.stage == "apply-commit")
        .collect::<Vec<_>>();
    let query = trace.iter().position(|event| {
        event.stage == "query-override"
            && event.query_overridden == Some(true)
            && event.original_calls == Some(1)
            && event.effective_result_bits.is_some_and(|bits| bits != 0)
            && event.target_tick == Some(event.authority_tick)
    });
    suppression.zip(query).is_some_and(|(suppression, query)| {
        apply.len() == 1 && suppression < apply[0].0 && apply[0].0 < query
    })
}

fn loopback_scenario_complete(groups: &BTreeMap<CommandIdentity, Vec<&ActiveEvent>>) -> bool {
    groups.values().any(|trace| loopback_trace_complete(trace))
        || groups.iter().any(|(neutral_identity, neutral_trace)| {
            groups.iter().any(|(pending_identity, pending_trace)| {
                neutral_identity != pending_identity
                    && loopback_applied_neutral_handoff_complete(neutral_trace, pending_trace)
            })
        })
}

fn loopback_applied_neutral_handoff_complete(
    neutral_trace: &[&ActiveEvent],
    pending_trace: &[&ActiveEvent],
) -> bool {
    neutral_trace
        .iter()
        .enumerate()
        .filter(|(_, event)| {
            event.stage == "query-override"
                && event.query_overridden == Some(true)
                && event.original_calls == Some(1)
                && event.original_result_bits.is_some_and(|bits| bits != 0)
                && event.effective_result_bits == Some(0)
                && event.target_tick == Some(event.authority_tick)
                && event.source_device.as_deref() == Some("KEYBOARD")
                && event.query_kind.as_deref() == Some("SCALAR")
                && event.consumer.as_deref() == Some("MOVEMENT")
                && event.action.is_some_and(|action| action <= 3)
                && event
                    .owner_generation
                    .is_some_and(|generation| generation != 0)
                && event.canonical_state_hex.as_deref().and_then(decode_state)
                    == Some(InputStateDraftV1::neutral())
        })
        .any(|(neutral_query_index, neutral_query)| {
            let neutral_applies = neutral_trace
                .iter()
                .enumerate()
                .filter(|(_, event)| event.stage == "apply-commit")
                .collect::<Vec<_>>();
            if neutral_applies.len() != 1 || neutral_applies[0].0 >= neutral_query_index {
                return false;
            }
            let Some(pending_target) = neutral_query.authority_tick.checked_add(1) else {
                return false;
            };
            let pending_accept_precedes_suppression = pending_trace.iter().any(|event| {
                event.stage == "accept"
                    && event.decision.as_deref() == Some("ACCEPTED")
                    && event.target_tick == Some(pending_target)
                    && event.event_sequence < neutral_query.event_sequence
            });
            if !pending_accept_precedes_suppression {
                return false;
            }
            let pending_applies = pending_trace
                .iter()
                .enumerate()
                .filter(|(_, event)| event.stage == "apply-commit")
                .collect::<Vec<_>>();
            if pending_applies.len() != 1
                || pending_applies[0].1.authority_tick != pending_target
                || pending_applies[0].1.target_tick != Some(pending_target)
                || pending_applies[0].1.event_sequence <= neutral_query.event_sequence
            {
                return false;
            }
            pending_trace
                .iter()
                .enumerate()
                .skip(pending_applies[0].0 + 1)
                .any(|(_, event)| {
                    event.stage == "query-override"
                        && event.query_overridden == Some(true)
                        && event.original_calls == Some(1)
                        && event.original_result_bits == neutral_query.original_result_bits
                        && event.effective_result_bits == neutral_query.original_result_bits
                        && event.target_tick == Some(pending_target)
                        && event.authority_tick == pending_target
                        && event.source_device == neutral_query.source_device
                        && event.action == neutral_query.action
                        && event.query_kind == neutral_query.query_kind
                        && event.consumer == neutral_query.consumer
                        && event.owner_generation == neutral_query.owner_generation
                        && event.apply_generation == neutral_query.apply_generation
                        && event.event_sequence > neutral_query.event_sequence
                        && event
                            .canonical_state_hex
                            .as_deref()
                            .and_then(decode_state)
                            .is_some_and(|state| state != InputStateDraftV1::neutral())
                })
        })
}

fn exact_reject(
    events: &[ActiveEvent],
    reason: &str,
    predicate: impl Fn(&ActiveEvent) -> bool,
) -> bool {
    let rejected = events
        .iter()
        .filter(|event| event.stage == "reject")
        .collect::<Vec<_>>();
    rejected.len() == 1
        && rejected[0].decision.as_deref() == Some("REJECTED")
        && rejected[0].reject_reason.as_deref() == Some(reason)
        && predicate(rejected[0])
        && events.iter().all(|event| event.stage != "apply-commit")
}

fn repeatability_semantic_trace(events: &[ActiveEvent]) -> String {
    let ignored = redundant_neutral_shutdown_tail_indices(events);
    semantic_trace_excluding(events, &ignored)
}

fn redundant_neutral_shutdown_tail_indices(events: &[ActiveEvent]) -> BTreeSet<usize> {
    const TAIL_STAGES: [&str; 5] = ["sample", "canonical", "send", "receive", "accept"];
    let ignored = BTreeSet::new();
    let Some((last_apply_index, last_apply)) = events
        .iter()
        .enumerate()
        .rfind(|(_, event)| event.stage == "apply-commit")
    else {
        return ignored;
    };
    if last_apply.edge_flags != Some(0)
        || last_apply
            .canonical_state_hex
            .as_deref()
            .and_then(decode_state)
            != Some(InputStateDraftV1::neutral())
    {
        return ignored;
    }
    let Some(hooks_restored_index) = events
        .iter()
        .enumerate()
        .skip(last_apply_index + 1)
        .find_map(|(index, event)| {
            (event.stage == "lifecycle"
                && event.lifecycle_kind.as_deref() == Some("HOOKS_RESTORED"))
            .then_some(index)
        })
    else {
        return ignored;
    };
    if hooks_restored_index + 2 != events.len()
        || events[hooks_restored_index + 1].stage != "lifecycle"
        || events[hooks_restored_index + 1].lifecycle_kind.as_deref() != Some("PRODUCER_CLOSED")
    {
        return ignored;
    }

    let mut ignored = ignored;
    let mut end = hooks_restored_index;
    while let Some(start) = end.checked_sub(TAIL_STAGES.len()) {
        if start <= last_apply_index {
            break;
        }
        let group = &events[start..end];
        let Some(tail_identity) = identity(&group[0]) else {
            break;
        };
        let Some(last_apply_identity) = identity(last_apply) else {
            break;
        };
        let expected_target = last_apply.authority_tick.saturating_add(INPUT_LEAD_TICKS);
        let group_matches = group
            .iter()
            .zip(TAIL_STAGES)
            .all(|(event, stage)| event.stage == stage)
            && group
                .windows(2)
                .all(|pair| pair[0].event_sequence.checked_add(1) == Some(pair[1].event_sequence))
            && group.iter().all(|event| {
                identity(event).as_ref() == Some(&tail_identity)
                    && event.attempt == 0
                    && event.authority_tick == last_apply.authority_tick
                    && event.loop_iteration == group[0].loop_iteration
                    && event.target_tick == Some(expected_target)
                    && event.canonical_state_hex == last_apply.canonical_state_hex
                    && event.edge_flags == Some(0)
                    && event.apply_generation == last_apply.apply_generation
                    && event.owner_generation == last_apply.owner_generation
                    && event.source_device == last_apply.source_device
                    && event.action == last_apply.action
            })
            && tail_identity.session_id == last_apply_identity.session_id
            && tail_identity.client_id == last_apply_identity.client_id
            && tail_identity.sequence > last_apply_identity.sequence
            && group[0].loop_iteration > last_apply.loop_iteration
            && group[0].sample_component_count == Some(0)
            && group[0..3]
                .iter()
                .all(|event| event.phase == "PHYSICAL_SAMPLE")
            && group[3..5]
                .iter()
                .all(|event| event.phase == "DRAIN_BEFORE_RESOLVE")
            && group[0..4].iter().all(|event| event.decision.is_none())
            && group[4].decision.as_deref() == Some("ACCEPTED")
            && group[4].reject_reason.as_deref() == Some("NONE");
        if !group_matches {
            break;
        }
        ignored.extend(start..end);
        end = start;
    }
    ignored
}

fn semantic_trace(events: &[ActiveEvent]) -> String {
    semantic_trace_excluding(events, &BTreeSet::new())
}

fn semantic_trace_excluding(events: &[ActiveEvent], ignored: &BTreeSet<usize>) -> String {
    let mut client_ids = BTreeMap::<u64, usize>::new();
    let mut first_sequence = BTreeMap::<u64, u64>::new();
    events
        .iter()
        .enumerate()
        .filter(|(index, _)| !ignored.contains(index))
        .map(|(_, event)| event)
        .map(|event| {
            let client = event.client_id.map(|raw| {
                let next = client_ids.len();
                *client_ids.entry(raw).or_insert(next)
            });
            let sequence = event
                .client_id
                .zip(event.command_sequence)
                .map(|(client, sequence)| {
                    let first = *first_sequence.entry(client).or_insert(sequence);
                    sequence.saturating_sub(first)
                });
            let tick_delta = event
                .target_tick
                .map(|target| target as i128 - event.authority_tick as i128);
            let witness_relation = match (
                event.world_witness_before.as_ref(),
                event.world_witness_after.as_ref(),
            ) {
                (Some(before), Some(after)) if before == after => "UNCHANGED",
                (Some(_), Some(_)) => "CHANGED",
                _ => "NONE",
            };
            format!(
                "{}|{}|{}|{:?}|{:?}|{:?}|{:?}|{:?}|{:?}|{:?}|{:?}|{}",
                event.stage,
                event.phase,
                event.attempt,
                client,
                sequence,
                tick_delta,
                event.canonical_state_hex,
                event.edge_flags,
                event.decision,
                event.reject_reason,
                event.action,
                witness_relation
            )
        })
        .collect::<Vec<_>>()
        .join("\n")
}

fn canonical_trace(events: &[ActiveEvent]) -> String {
    events
        .iter()
        .filter_map(|event| {
            Some(format!(
                "{}:{}:{}",
                event.command_sequence?,
                event.target_tick?,
                event.canonical_state_hex.as_deref()?
            ))
        })
        .collect::<Vec<_>>()
        .join("\n")
}

fn decision_trace(events: &[ActiveEvent]) -> String {
    events
        .iter()
        .filter(|event| matches!(event.stage.as_str(), "accept" | "reject" | "duplicate"))
        .map(|event| {
            format!(
                "{}:{}:{}:{}",
                event.command_sequence.unwrap_or(0),
                event.target_tick.unwrap_or(0),
                event.decision.as_deref().unwrap_or(""),
                event.reject_reason.as_deref().unwrap_or("")
            )
        })
        .collect::<Vec<_>>()
        .join("\n")
}

fn apply_trace(events: &[ActiveEvent]) -> String {
    events
        .iter()
        .filter(|event| event.stage == "apply-commit")
        .map(|event| {
            format!(
                "{}:{}:{}:{}",
                event.command_sequence.unwrap_or(0),
                event.target_tick.unwrap_or(0),
                event.edge_flags.unwrap_or(0),
                event.canonical_state_hex.as_deref().unwrap_or("")
            )
        })
        .collect::<Vec<_>>()
        .join("\n")
}

fn identity(event: &ActiveEvent) -> Option<CommandIdentity> {
    Some(CommandIdentity {
        session_id: event.session_id.clone()?,
        client_id: event.client_id?,
        sequence: event.command_sequence?,
    })
}

fn is_command_stage(stage: &str) -> bool {
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
            | "apply-commit"
            | "suppression"
            | "query-override"
            | "witness"
    )
}

fn transport_stage_rank(stage: &str) -> Option<u8> {
    Some(match stage {
        "sample" => 0,
        "canonical" => 1,
        "send" => 2,
        "receive" => 3,
        "accept" | "reject" | "duplicate" => 4,
        _ => return None,
    })
}

fn count(events: &[&ActiveEvent], stage: &str) -> usize {
    events.iter().filter(|event| event.stage == stage).count()
}

fn submitted_payload_contract(trace: &[&ActiveEvent], scenario: &str) -> bool {
    let Some(primary) = trace
        .iter()
        .find(|event| event.stage == "sample" && event.attempt == 0)
    else {
        return false;
    };
    let primary_payload = primary.canonical_state_hex.as_deref();
    let primary_edges = primary.edge_flags;
    let accepted = trace.iter().any(|event| event.stage == "accept");
    let submitted = trace.iter().filter(|event| {
        transport_stage_rank(&event.stage).is_some()
            || event.stage == "suppression"
            || (!accepted && event.stage == "witness")
    });
    if !submitted.into_iter().all(|event| {
        event.canonical_state_hex.as_deref() == primary_payload && event.edge_flags == primary_edges
    }) {
        return false;
    }

    let conflict_retry = trace
        .iter()
        .filter(|event| {
            event.attempt > 0 && matches!(event.stage.as_str(), "send" | "receive" | "reject")
        })
        .collect::<Vec<_>>();
    if scenario == "conflict" {
        let alternatives = conflict_retry
            .iter()
            .filter_map(|event| event.conflicting_state_hex.as_deref())
            .collect::<BTreeSet<_>>();
        !conflict_retry.is_empty()
            && alternatives.len() == 1
            && alternatives
                .first()
                .is_some_and(|alternative| Some(*alternative) != primary_payload)
            && conflict_retry
                .iter()
                .all(|event| event.conflicting_state_hex.is_some())
            && trace.iter().all(|event| {
                event.conflicting_state_hex.is_none()
                    || (event.attempt > 0
                        && matches!(event.stage.as_str(), "send" | "receive" | "reject"))
            })
    } else {
        trace
            .iter()
            .all(|event| event.conflicting_state_hex.is_none())
    }
}

fn generation_boundary(event: &ActiveEvent) -> bool {
    // Only OwnerSwitch/LifecycleClear records expose owner_generation. A
    // LifecycleClear with lifecycleKind=None is the schema-4 generic clear used
    // by owner-only room guards; it remains a first-class generation boundary
    // without becoming a scenario lifecycle label.
    event.stage == "lifecycle" && event.owner_generation.is_some()
}

fn generation_timeline_valid(events: &[ActiveEvent]) -> bool {
    let mut current: Option<u64> = None;
    for event in events {
        if generation_boundary(event) {
            let Some(next) = event.owner_generation else {
                return false;
            };
            if next == 0
                || current.is_some_and(|generation| generation.checked_add(1) != Some(next))
            {
                return false;
            }
            current = Some(next);
        } else if is_command_stage(&event.stage) {
            let Some(observed) = event.owner_generation else {
                return false;
            };
            if observed == 0 || current.is_some_and(|generation| generation != observed) {
                return false;
            }
            current = Some(observed);
        }
    }
    true
}

fn accepted_bucket_contract(
    manifest: &ActiveManifest,
    events: &[ActiveEvent],
    groups: &BTreeMap<CommandIdentity, Vec<&ActiveEvent>>,
) -> bool {
    let mut buckets =
        BTreeMap::<(String, u64, u64, u64), Vec<(&CommandIdentity, &Vec<&ActiveEvent>)>>::new();
    let mut maximum_authority_by_owner = BTreeMap::<u64, u64>::new();
    for event in events.iter().filter(|event| is_command_stage(&event.stage)) {
        if let Some(owner_generation) = event.owner_generation {
            maximum_authority_by_owner
                .entry(owner_generation)
                .and_modify(|maximum| *maximum = (*maximum).max(event.authority_tick))
                .or_insert(event.authority_tick);
        }
    }
    for (command, trace) in groups {
        if trace.iter().any(|event| event.stage == "accept")
            && !trace.iter().any(|event| event.stage == "reject")
        {
            let first = trace[0];
            let Some(session_id) = first.session_id.clone() else {
                return false;
            };
            let Some(client_id) = first.client_id else {
                return false;
            };
            let Some(owner_generation) = first.owner_generation else {
                return false;
            };
            let Some(target_tick) = first.target_tick else {
                return false;
            };
            buckets
                .entry((session_id, client_id, owner_generation, target_tick))
                .or_default()
                .push((command, trace));
        }
    }
    let final_generation = events.iter().rev().find_map(|event| event.owner_generation);

    buckets
        .into_iter()
        .all(|((_, _, owner_generation, target_tick), bucket)| {
            let Some(winner) = bucket.iter().max_by_key(|(command, _)| command.sequence) else {
                return false;
            };
            let mut combined_edges = 0_u32;
            let mut accepted_records = Vec::with_capacity(bucket.len());
            for (_, trace) in &bucket {
                let Some(accept) = trace.iter().find(|event| {
                    event.stage == "accept"
                        && event.attempt == 0
                        && event.decision.as_deref() == Some("ACCEPTED")
                }) else {
                    return false;
                };
                let Some(edges) = accept.edge_flags else {
                    return false;
                };
                if combined_edges & edges != 0 {
                    return false;
                }
                combined_edges |= edges;
                accepted_records.push(*accept);
            }
            let Some(winner_accept) = winner
                .1
                .iter()
                .find(|event| event.stage == "accept" && event.attempt == 0)
            else {
                return false;
            };
            let Some(winner_state) = winner_accept
                .canonical_state_hex
                .as_deref()
                .and_then(decode_state)
            else {
                return false;
            };
            let (move_x, move_y) = winner_state.move_axes();
            let (aim_x, aim_y) = winner_state.aim_axes();
            let Ok(effective_state) = InputStateDraftV1::from_q15(
                winner_state.shoot_held(),
                combined_edges as u8,
                move_x,
                move_y,
                aim_x,
                aim_y,
            ) else {
                return false;
            };
            let Ok(effective_bytes) = effective_state.encode() else {
                return false;
            };
            let effective_hex = hex::encode_upper(effective_bytes);
            let winner_source = winner_accept.source_device.as_deref();
            if winner_source.is_none()
                || accepted_records
                    .iter()
                    .any(|event| event.source_device.as_deref() != winner_source)
                || accepted_records.iter().any(|accept| {
                    accept
                        .authority_tick
                        .checked_add(INPUT_LEAD_TICKS)
                        .is_none_or(|minimum| target_tick < minimum)
                        || accept
                            .authority_tick
                            .checked_add(INPUT_FUTURE_WINDOW)
                            .is_none_or(|maximum| target_tick > maximum)
                })
            {
                return false;
            }

            let resolves = bucket
                .iter()
                .flat_map(|(_, trace)| trace.iter().copied())
                .filter(|event| event.stage == "resolve")
                .collect::<Vec<_>>();
            let applies = bucket
                .iter()
                .flat_map(|(_, trace)| trace.iter().copied())
                .filter(|event| event.stage == "apply-commit")
                .collect::<Vec<_>>();
            let maximum_authority = maximum_authority_by_owner
                .get(&owner_generation)
                .copied()
                .unwrap_or(0);
            let reached = maximum_authority >= target_tick;
            if reached {
                if resolves.len() != 1
                    || applies.len() != 1
                    || count(winner.1, "resolve") != 1
                    || count(winner.1, "apply-commit") != 1
                {
                    return false;
                }
                let resolve = resolves[0];
                let apply = applies[0];
                let latest_accept_sequence = accepted_records
                    .iter()
                    .map(|event| event.event_sequence)
                    .max()
                    .unwrap_or(0);
                let latest_accept_qpc = accepted_records
                    .iter()
                    .map(|event| event.qpc)
                    .max()
                    .unwrap_or(0);
                if resolve.event_sequence <= latest_accept_sequence
                    || resolve.qpc <= latest_accept_qpc
                    || apply.event_sequence != resolve.event_sequence + 1
                    || apply.qpc <= resolve.qpc
                {
                    return false;
                }
                for event in bucket.iter().flat_map(|(_, trace)| trace.iter().copied()) {
                    let uses_effective = matches!(
                        event.stage.as_str(),
                        "resolve" | "apply-commit" | "query-override"
                    ) || (event.stage == "witness"
                        && event.authority_tick >= target_tick);
                    if uses_effective
                        && (event.command_sequence != Some(winner.0.sequence)
                            || event.authority_tick != target_tick
                            || event.canonical_state_hex.as_deref() != Some(effective_hex.as_str())
                            || event.edge_flags != Some(combined_edges)
                            || event.owner_generation != Some(owner_generation)
                            || event.apply_generation != Some(manifest.apply_generation)
                            || (event.stage != "resolve"
                                && event.source_device.as_deref() != winner_source))
                    {
                        return false;
                    }
                }
            } else {
                if !resolves.is_empty() || !applies.is_empty() {
                    return false;
                }
                let latest_accept_sequence = accepted_records
                    .iter()
                    .map(|event| event.event_sequence)
                    .max()
                    .unwrap_or(0);
                let cancelled = events.iter().any(|event| {
                    generation_boundary(event)
                        && event.event_sequence > latest_accept_sequence
                        && event
                            .owner_generation
                            .is_some_and(|generation| generation > owner_generation)
                });
                let valid_tail = !cancelled
                    && manifest.producer_closed
                    && final_generation == Some(owner_generation)
                    && target_tick > maximum_authority;
                if !cancelled && !valid_tail {
                    return false;
                }
            }

            for (_, trace) in &bucket {
                let Some(accept) = trace.iter().find(|event| event.stage == "accept") else {
                    return false;
                };
                for event in trace.iter().filter(|event| {
                    event.stage == "suppression"
                        || (event.stage == "witness" && event.authority_tick < target_tick)
                }) {
                    if event.event_sequence <= accept.event_sequence
                        || event.authority_tick >= target_tick
                        || event.canonical_state_hex != accept.canonical_state_hex
                        || event.edge_flags != accept.edge_flags
                        || event.owner_generation != accept.owner_generation
                        || event.source_device != accept.source_device
                    {
                        return false;
                    }
                }
            }
            true
        })
}

fn allowed_consumer(value: &str) -> bool {
    matches!(
        value,
        "MOVEMENT" | "BOMB" | "ITEM_SLOT" | "PILL_CARD" | "SHOOTING" | "DROP"
    )
}

fn decode_state(value: &str) -> Option<InputStateDraftV1> {
    let bytes = hex::decode(value).ok()?;
    InputStateDraftV1::decode(&bytes).ok()
}

fn validate_state_hex(value: &str) -> bool {
    decode_state(value).is_some()
}

fn parse_mask(value: &str) -> Option<u64> {
    value
        .strip_prefix("0x")
        .and_then(|digits| u64::from_str_radix(digits, 16).ok())
}

fn check(id: &str, passed: bool, detail: &str) -> ActiveCheck {
    ActiveCheck {
        id: id.to_owned(),
        passed,
        detail: detail.to_owned(),
    }
}

fn validate_matrix_path(path: &Path) -> AppResult<()> {
    if path.as_os_str().is_empty()
        || path.is_absolute()
        || path.components().any(|component| {
            matches!(
                component,
                Component::ParentDir
                    | Component::CurDir
                    | Component::RootDir
                    | Component::Prefix(_)
            )
        })
    {
        return Err(AppError::usage(
            "active matrix paths must be normalized relative paths",
        ));
    }
    Ok(())
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

#[cfg(test)]
mod tests {
    use super::*;
    use crate::active_ipc::{
        ACTIVE_FLAG_CLIENT_RETIRED, ACTIVE_FLAG_ORIGINAL_CALLED, ACTIVE_FLAG_QUERY_OVERRIDDEN,
        ActiveConsumer, ActiveDecision, ActiveErrorCode, ActiveEventKind, ActiveLifecycleKind,
        ActivePhase, ActiveQueryKind, ActiveSourceDevice, ActiveWitnessKind, NativeActiveEvent,
    };

    const LOOP_CHECK_IDS: [&str; 18] = [
        "non-empty",
        "sequence-contiguous",
        "qpc-monotonic",
        "schema-known",
        "event-contract",
        "authority-owned",
        "authority-evidence-nonempty",
        "authority-transaction-order",
        "physical-sample-envelope",
        "balanced-depth",
        "main-thread-owned",
        "loop-order",
        "ui-partition",
        "message-pump-owner",
        "original-once",
        "native-lifecycle",
        "target-restored",
        "scenario-evidence",
    ];

    fn companion_report(scenario: &str, failed: &[&str]) -> LoopAnalysisReport {
        let passed = failed.is_empty();
        LoopAnalysisReport {
            schema_version: 2,
            result: if passed { "PASS" } else { "FAIL" }.to_owned(),
            evidence_class: LOOP_EVIDENCE_CLASS.to_owned(),
            probe: "passive".to_owned(),
            scenario: scenario.to_owned(),
            target_sha256: FIXED_TARGET_SHA256.to_owned(),
            loop_ledger_sha256: "A".repeat(64),
            manifest_sha256: "B".repeat(64),
            events_sha256: "C".repeat(64),
            event_count: 1,
            loop_iteration_count: 1,
            complete_loop_count: 1,
            input_query_count: 0,
            message_pump_count: 1,
            eligible_for_active_gate: passed,
            checks: LOOP_CHECK_IDS
                .iter()
                .map(|id| crate::loop_verification::CheckResult {
                    id: (*id).to_owned(),
                    passed: !failed.contains(id),
                    detail: (*id).to_owned(),
                })
                .collect(),
        }
    }

    #[test]
    fn companion_policy_allows_only_scenario_scope_failure_outside_lifecycle() {
        for scenario in ACTIVE_SCENARIOS.into_iter().filter(|scenario| {
            !matches!(*scenario, "control-no-native" | "original-online-disarmed")
        }) {
            assert!(companion_analysis_policy_valid(
                scenario,
                &companion_report(scenario, &[])
            ));
            let scope_only = companion_report(scenario, &["scenario-evidence"]);
            assert_eq!(
                companion_analysis_policy_valid(scenario, &scope_only),
                !matches!(scenario, "pause" | "loading" | "room" | "menu"),
                "unexpected scenario-evidence policy for {scenario}"
            );
        }

        for common in LOOP_CHECK_IDS
            .iter()
            .copied()
            .filter(|id| *id != "scenario-evidence")
        {
            assert!(
                !companion_analysis_policy_valid(
                    "simultaneous",
                    &companion_report("simultaneous", &[common])
                ),
                "common companion failure {common} was accepted"
            );
            assert!(!companion_analysis_policy_valid(
                "simultaneous",
                &companion_report("simultaneous", &[common, "scenario-evidence"])
            ));
        }

        let mut incoherent = companion_report("simultaneous", &["scenario-evidence"]);
        incoherent.result = "PASS".to_owned();
        assert!(!companion_analysis_policy_valid(
            "simultaneous",
            &incoherent
        ));
        incoherent.result = "FAIL".to_owned();
        incoherent.eligible_for_active_gate = true;
        assert!(!companion_analysis_policy_valid(
            "simultaneous",
            &incoherent
        ));
    }

    fn runtime_command_event(kind: ActiveEventKind, phase: ActivePhase) -> NativeActiveEvent {
        NativeActiveEvent {
            published_sequence: 1,
            qpc: 10,
            authority_tick: 8,
            loop_iteration: 9,
            thread_id: 11,
            kind: kind as u32,
            phase: phase as u32,
            client_id: 7,
            command_sequence: 1,
            target_tick: 8,
            apply_generation: 9,
            owner_generation: 9,
            action: ACTIVE_NO_ACTION,
            canonical_state: InputStateDraftV1::neutral().encode().unwrap(),
            ..NativeActiveEvent::default()
        }
    }

    #[test]
    fn native_event_writer_injects_sixteen_byte_session_and_rejects_codebook_mutations() {
        let mut native = NativeActiveEvent {
            published_sequence: 1,
            qpc: 10,
            authority_tick: 8,
            loop_iteration: 9,
            thread_id: 11,
            kind: ActiveEventKind::Accept as u32,
            phase: ActivePhase::DrainBeforeResolve as u32,
            client_id: 7,
            command_sequence: 1,
            target_tick: 10,
            decision: ActiveDecision::Accepted as u32,
            apply_generation: 9,
            owner_generation: 9,
            action: ACTIVE_NO_ACTION,
            canonical_state: InputStateDraftV1::neutral().encode().unwrap(),
            ..NativeActiveEvent::default()
        };
        let record = NativeActiveEventRecord {
            sequence: 1,
            event: native,
        };
        let event = active_event_from_native(&record, 1, &[0xAB; 16]).unwrap();
        assert_eq!(event.stage, "accept");
        assert_eq!(event.phase, "DRAIN_BEFORE_RESOLVE");
        assert_eq!(
            event.session_id.as_deref(),
            Some("ABABABABABABABABABABABABABABABAB")
        );
        assert_eq!(event.decision.as_deref(), Some("ACCEPTED"));
        assert_eq!(event.reject_reason.as_deref(), Some("NONE"));

        native.decision = ActiveDecision::Rejected as u32;
        assert!(
            active_event_from_native(
                &NativeActiveEventRecord {
                    sequence: 1,
                    event: native,
                },
                1,
                &[0xAB; 16],
            )
            .is_err()
        );

        native.decision = ActiveDecision::Accepted as u32;
        native.phase = 99;
        assert!(
            active_event_from_native(
                &NativeActiveEventRecord {
                    sequence: 1,
                    event: native,
                },
                1,
                &[0xAB; 16],
            )
            .is_err()
        );

        native.phase = ActivePhase::OutsideAuthority as u32;
        native.kind = ActiveEventKind::LifecycleClear as u32;
        native.decision = ActiveDecision::None as u32;
        native.action = ActiveLifecycleKind::PauseEnter as u32;
        let lifecycle = active_event_from_native(
            &NativeActiveEventRecord {
                sequence: 1,
                event: native,
            },
            1,
            &[0xAB; 16],
        )
        .unwrap();
        assert_eq!(lifecycle.lifecycle_kind.as_deref(), Some("PAUSE_ENTER"));
        assert!(lifecycle.session_id.is_none());
        assert_eq!(lifecycle.owner_generation, Some(native.owner_generation));
    }

    #[test]
    fn native_runtime_apply_query_witness_lifecycle_and_terminal_codes_convert() {
        let session = [0xAB; 16];
        let convert = |native| {
            active_event_from_native(
                &NativeActiveEventRecord {
                    sequence: 1,
                    event: native,
                },
                1,
                &session,
            )
            .unwrap()
        };

        let mut apply = runtime_command_event(ActiveEventKind::ApplyCommit, ActivePhase::Apply);
        apply.source_device = ActiveSourceDevice::Keyboard as u32;
        apply.sample_component_count = 1;
        let converted = convert(apply);
        assert_eq!(converted.stage, "apply-commit");
        assert!(converted.decision.is_none());
        assert_eq!(converted.apply_kind.as_deref(), Some("SYNTHETIC_SNAPSHOT"));

        let mut query =
            runtime_command_event(ActiveEventKind::QueryOverride, ActivePhase::InputQuery);
        query.source_device = ActiveSourceDevice::Keyboard as u32;
        query.action = 1;
        query.query_kind = ActiveQueryKind::Scalar as u32;
        query.consumer = ActiveConsumer::Movement as u32;
        query.original_calls = 1;
        query.flags = ACTIVE_FLAG_ORIGINAL_CALLED | ACTIVE_FLAG_QUERY_OVERRIDDEN;
        let converted = convert(query);
        assert_eq!(converted.query_kind.as_deref(), Some("SCALAR"));
        assert_eq!(converted.original_calls, Some(1));

        let mut witness = runtime_command_event(ActiveEventKind::Witness, ActivePhase::Witness);
        witness.query_kind = ActiveWitnessKind::WorldState as u32;
        witness.consumer = ActiveConsumer::Movement as u32;
        witness.witness_before = [1; 32];
        witness.witness_after = [2; 32];
        let converted = convert(witness);
        assert_eq!(converted.witness_kind.as_deref(), Some("WORLD_STATE"));

        witness.query_kind = ActiveWitnessKind::RejectedUnchanged as u32;
        witness.witness_after = witness.witness_before;
        let converted = convert(witness);
        assert_eq!(
            converted.witness_kind.as_deref(),
            Some("REJECTED_UNCHANGED")
        );

        let mut lifecycle = runtime_command_event(
            ActiveEventKind::LifecycleClear,
            ActivePhase::OutsideAuthority,
        );
        lifecycle.action = ActiveLifecycleKind::RunTeardown as u32;
        let converted = convert(lifecycle);
        assert_eq!(converted.lifecycle_kind.as_deref(), Some("RUN_TEARDOWN"));
        assert_eq!(converted.owner_generation, Some(lifecycle.owner_generation));

        let mut retired = runtime_command_event(
            ActiveEventKind::ClientRetired,
            ActivePhase::OutsideAuthority,
        );
        retired.flags = ACTIVE_FLAG_CLIENT_RETIRED;
        let converted = convert(retired);
        assert_eq!(converted.lifecycle_kind.as_deref(), Some("CLIENT_RETIRED"));

        let mut failure =
            runtime_command_event(ActiveEventKind::Failure, ActivePhase::OutsideAuthority);
        failure.error_code = ActiveErrorCode::AuthorityOverflow as u32;
        failure.hook_site_rva = 0x0038_5BC2;
        let converted = convert(failure);
        assert_eq!(converted.lifecycle_kind.as_deref(), Some("FAILURE"));
        assert_eq!(
            converted.error_code,
            Some(ActiveErrorCode::AuthorityOverflow as u32)
        );
        assert_eq!(converted.error_name.as_deref(), Some("AUTHORITY_OVERFLOW"));
        assert_eq!(converted.failure_site_rva.as_deref(), Some("0x00385BC2"));

        let mut legacy_json = serde_json::to_value(&converted).unwrap();
        let legacy_object = legacy_json.as_object_mut().unwrap();
        legacy_object.remove("errorCode");
        legacy_object.remove("errorName");
        legacy_object.remove("failureSiteRva");
        let legacy: ActiveEvent = serde_json::from_value(legacy_json).unwrap();
        assert!(legacy.error_code.is_none());
        assert!(legacy.error_name.is_none());
        assert!(legacy.failure_site_rva.is_none());
    }

    fn state_hex(state: InputStateDraftV1) -> String {
        hex::encode_upper(state.encode().unwrap())
    }

    fn target_slots() -> Vec<ActiveTargetSlot> {
        expected_hook_slots()
            .into_iter()
            .map(|slot| ActiveTargetSlot {
                ordinal: slot.ordinal,
                id: slot.id,
                rva: slot.rva,
                bytes_hex: "00".repeat(slot.width as usize),
            })
            .collect()
    }

    fn original_online_observation() -> OriginalOnlineDisarmedObservation {
        OriginalOnlineDisarmedObservation {
            schema_version: ORIGINAL_ONLINE_OBSERVATION_SCHEMA_VERSION,
            observation_count: 2,
            qpc_before: 10,
            qpc_after: 20,
            game_root_identity_sha256: "1".repeat(64),
            manager_identity_sha256: "2".repeat(64),
            network_backend_identity_sha256: "3".repeat(64),
            roster_identity_sha256: "4".repeat(64),
            roster_count: 2,
            online_frame_before: 100,
            online_frame_after: 101,
            game_root_stable: true,
            run_active: true,
            manager_identity_stable: true,
            network_backend_present: true,
            roster_stable: true,
        }
    }

    fn manifest(scenario: &str, pipeline: bool) -> ActiveManifest {
        let slots = target_slots();
        ActiveManifest {
            schema_version: ACTIVE_ARTIFACT_SCHEMA_VERSION,
            experiment_identity: ACTIVE_EXPERIMENT_IDENTITY.to_owned(),
            evidence_class: if pipeline {
                EVIDENCE_PIPELINE_RUNTIME
            } else {
                EVIDENCE_DISARMED_CONTROL
            }
            .to_owned(),
            state: "COMPLETE".to_owned(),
            case_id: format!("fixture-{scenario}"),
            probe: if pipeline { "loopback" } else { scenario }.to_owned(),
            scenario: scenario.to_owned(),
            endpoint: if pipeline {
                "offline-local"
            } else if scenario == "original-online-disarmed" {
                "original-online-read-only"
            } else {
                "disarmed-control"
            }
            .to_owned(),
            target_sha256: FIXED_TARGET_SHA256.to_owned(),
            observed_target_sha256: crate::target_equivalence::APPROVED_UNPACKED_TARGET_SHA256
                .to_owned(),
            target_equivalence_sha256: "9".repeat(64),
            mode_gate_sha256: "A".repeat(64),
            loop_gate_sha256: "B".repeat(64),
            raw_prereq_sha256: "C".repeat(64),
            active_catalog_sha256: "D".repeat(64),
            active_plan_sha256: "E".repeat(64),
            input_script_sha256: (scenario == "loopback").then(|| "F".repeat(64)),
            original_online_observation: (scenario == "original-online-disarmed")
                .then(original_online_observation),
            companion_loop_artifact_path: pipeline.then(|| PathBuf::from("loop-context")),
            companion_loop_manifest_sha256: pipeline.then(|| "1".repeat(64)),
            companion_loop_events_sha256: pipeline.then(|| "2".repeat(64)),
            companion_loop_analysis_sha256: pipeline.then(|| "3".repeat(64)),
            artifact_identity: format!("active-fixture-{scenario}"),
            ipc_magic: if pipeline {
                ACTIVE_IPC_SOURCE_FORMAT
            } else {
                ACTIVE_CONTROL_SOURCE_FORMAT
            }
            .to_owned(),
            ipc_schema_version: pipeline.then_some(ACTIVE_IPC_SCHEMA_VERSION),
            arm_token_sha256: pipeline.then(|| "4".repeat(64)),
            arm_token_consumed: pipeline,
            session_id: "AB".repeat(16),
            client_id: 7,
            main_thread_id: u32::from(pipeline) * 11,
            apply_generation: u64::from(pipeline) * 9,
            installed_active_hooks_mask: if pipeline { "0x1FFF" } else { "0x0" }.to_owned(),
            restored_active_hooks_mask: if pipeline { "0x1FFF" } else { "0x0" }.to_owned(),
            restored_slot_count: ACTIVE_RESTORED_SLOT_COUNT,
            overwritten_events: 0,
            producer_closed: true,
            target_slots_before: slots.clone(),
            target_slots_after: slots,
            target_bytes_restored: true,
            initial_snapshot_neutral: true,
            client_retired: false,
            failure_reason: None,
        }
    }

    fn event(stage: &str, phase: &str, state: InputStateDraftV1) -> ActiveEvent {
        let command = is_command_stage(stage);
        ActiveEvent {
            event_sequence: 0,
            stage: stage.to_owned(),
            phase: phase.to_owned(),
            qpc: 1,
            thread_id: 11,
            authority_tick: 10,
            loop_iteration: 1,
            attempt: 0,
            session_id: command.then(|| "AB".repeat(16)),
            client_id: command.then_some(7),
            command_sequence: command.then_some(1),
            target_tick: command.then_some(12),
            canonical_state_hex: command.then(|| state_hex(state)),
            edge_flags: command.then_some(u32::from(state.edge_flags())),
            decision: None,
            reject_reason: None,
            apply_generation: command.then_some(9),
            apply_kind: None,
            source_device: None,
            owner_before: None,
            owner_after: None,
            owner_generation: command.then_some(9),
            lifecycle_kind: None,
            error_code: None,
            error_name: None,
            failure_site_rva: None,
            queue_depth: None,
            previous_command_sequence: None,
            conflicting_state_hex: None,
            action: None,
            query_kind: None,
            consumer: None,
            original_calls: None,
            original_result_bits: None,
            effective_result_bits: None,
            query_overridden: None,
            sample_component_count: None,
            witness_kind: None,
            world_witness_before: None,
            world_witness_after: None,
        }
    }

    fn resequence(events: &mut [ActiveEvent]) {
        for (index, event) in events.iter_mut().enumerate() {
            event.event_sequence = index as u64 + 1;
            event.qpc = event.event_sequence;
        }
    }

    fn accepted_trace(state: InputStateDraftV1) -> Vec<ActiveEvent> {
        let mut events = vec![
            event("sample", "PHYSICAL_SAMPLE", state),
            event("canonical", "PHYSICAL_SAMPLE", state),
            event("send", "PHYSICAL_SAMPLE", state),
            event("receive", "DRAIN_BEFORE_RESOLVE", state),
            event("accept", "DRAIN_BEFORE_RESOLVE", state),
            event("resolve", "RESOLVE", state),
            event("apply-commit", "APPLY", state),
            event("witness", "WITNESS", state),
        ];
        events[4].decision = Some("ACCEPTED".to_owned());
        events[4].reject_reason = Some("NONE".to_owned());
        events[5].authority_tick = 12;
        events[6].authority_tick = 12;
        events[6].apply_generation = Some(9);
        events[6].apply_kind = Some("SYNTHETIC_SNAPSHOT".to_owned());
        events[7].authority_tick = 12;
        events[7].world_witness_before = Some("before".to_owned());
        events[7].world_witness_after = Some("after".to_owned());
        for event in &mut events {
            event.source_device = Some("KEYBOARD".to_owned());
        }
        events[5].source_device = None;
        resequence(&mut events);
        events
    }

    fn rejected_trace(reason: &str, state: InputStateDraftV1) -> Vec<ActiveEvent> {
        let mut events = vec![
            event("sample", "PHYSICAL_SAMPLE", state),
            event("canonical", "PHYSICAL_SAMPLE", state),
            event("send", "PHYSICAL_SAMPLE", state),
            event("receive", "DRAIN_BEFORE_RESOLVE", state),
            event("reject", "DRAIN_BEFORE_RESOLVE", state),
            event("witness", "WITNESS", state),
        ];
        events[4].decision = Some("REJECTED".to_owned());
        events[4].reject_reason = Some(reason.to_owned());
        events[5].world_witness_before = Some("unchanged".to_owned());
        events[5].world_witness_after = Some("unchanged".to_owned());
        for event in &mut events {
            event.source_device = Some("KEYBOARD".to_owned());
        }
        resequence(&mut events);
        events
    }

    fn control_event() -> ActiveEvent {
        event(
            "control-observation",
            "OUTSIDE_AUTHORITY",
            InputStateDraftV1::neutral(),
        )
    }

    fn lifecycle(kind: &str, generation: u64) -> ActiveEvent {
        let mut event = event(
            "lifecycle",
            "OUTSIDE_AUTHORITY",
            InputStateDraftV1::neutral(),
        );
        event.lifecycle_kind = Some(kind.to_owned());
        event.owner_generation = Some(generation);
        event
    }

    fn groups(events: &[ActiveEvent]) -> BTreeMap<CommandIdentity, Vec<&ActiveEvent>> {
        let mut groups = BTreeMap::<CommandIdentity, Vec<&ActiveEvent>>::new();
        for event in events.iter().filter(|event| is_command_stage(&event.stage)) {
            if let Some(identity) = identity(event) {
                groups.entry(identity).or_default().push(event);
            }
        }
        groups
    }

    fn owner_fixture(scenario: &str, device: &str) -> (ActiveManifest, Vec<ActiveEvent>) {
        let mut events = accepted_trace(InputStateDraftV1::neutral());
        let mut witness = events.pop().unwrap();
        for event in &mut events {
            event.source_device = Some(device.to_owned());
        }
        witness.source_device = Some(device.to_owned());
        for action in 0..=11 {
            let mut query = event(
                "query-override",
                "INPUT_QUERY",
                InputStateDraftV1::neutral(),
            );
            query.action = Some(action);
            query.query_kind = Some(if action < 8 { "SCALAR" } else { "TRIGGERED" }.to_owned());
            query.consumer = Some(
                if action < 4 {
                    "MOVEMENT"
                } else if action < 8 {
                    "SHOOTING"
                } else {
                    "BOMB"
                }
                .to_owned(),
            );
            query.source_device = Some(device.to_owned());
            query.authority_tick = 12;
            query.original_calls = Some(1);
            query.original_result_bits = Some(0);
            query.effective_result_bits = Some(0);
            query.query_overridden = Some(true);
            events.push(query);
        }
        events.push(witness);
        resequence(&mut events);
        (manifest(scenario, true), events)
    }

    fn lifecycle_fixture(
        scenario: &str,
        enter: &str,
        exit: &str,
    ) -> (ActiveManifest, Vec<ActiveEvent>) {
        let mut events = accepted_trace(InputStateDraftV1::neutral());
        events.push(lifecycle(enter, 10));
        events.push(lifecycle(exit, 11));
        let mut resumed = accepted_trace(InputStateDraftV1::neutral());
        for event in &mut resumed {
            if event.command_sequence.is_some() {
                event.command_sequence = Some(2);
                event.owner_generation = Some(11);
            }
        }
        events.extend(resumed);
        resequence(&mut events);
        (manifest(scenario, true), events)
    }

    fn scenario_fixture(scenario: &str) -> (ActiveManifest, Vec<ActiveEvent>) {
        let neutral = InputStateDraftV1::neutral();
        match scenario {
            "control-no-native" | "original-online-disarmed" => {
                (manifest(scenario, false), vec![control_event()])
            }
            "keyboard" => owner_fixture(scenario, "KEYBOARD"),
            "controller" => owner_fixture(scenario, "GAMEPAD"),
            "simultaneous" => {
                let state = InputStateDraftV1::from_q15(false, 0, 20_000, 20_000, 0, 0).unwrap();
                let mut events = accepted_trace(state);
                events
                    .iter_mut()
                    .find(|event| event.stage == "apply-commit")
                    .unwrap()
                    .sample_component_count = Some(2);
                (manifest(scenario, true), events)
            }
            "rapid-tap" => {
                let mut events = Vec::new();
                for action in 8..=11 {
                    for cycle in 0..2 {
                        let state =
                            InputStateDraftV1::from_q15(false, 1 << (action - 8), 0, 0, 0, 0)
                                .unwrap();
                        let sequence = (action * 2 + cycle) as u64;
                        let target = (action * 10 + cycle) as u64;
                        let mut trace = accepted_trace(state);
                        for event in &mut trace {
                            if event.command_sequence.is_some() {
                                event.command_sequence = Some(sequence);
                                event.target_tick = Some(target);
                            }
                            if matches!(
                                event.stage.as_str(),
                                "resolve" | "apply-commit" | "witness"
                            ) {
                                event.authority_tick = target;
                            }
                        }
                        events.extend(trace);
                    }
                }
                resequence(&mut events);
                (manifest(scenario, true), events)
            }
            "device-switch" => {
                let mut switch = lifecycle("OWNER_SWITCH", 9);
                switch.owner_before = Some(1);
                switch.owner_after = Some(2);
                let mut events = vec![switch];
                events.extend(accepted_trace(neutral));
                resequence(&mut events);
                (manifest(scenario, true), events)
            }
            "pause" => lifecycle_fixture(scenario, "PAUSE_ENTER", "PAUSE_EXIT"),
            "loading" => {
                let mut events = vec![
                    lifecycle("LOADING_ENTER", 10),
                    lifecycle("LOADING_EXIT", 11),
                ];
                let mut resumed = accepted_trace(InputStateDraftV1::neutral());
                let witness = resumed.pop().unwrap();
                let mut query = event(
                    "query-override",
                    "INPUT_QUERY",
                    InputStateDraftV1::neutral(),
                );
                query.authority_tick = 12;
                query.action = Some(0);
                query.query_kind = Some("SCALAR".to_owned());
                query.consumer = Some("MOVEMENT".to_owned());
                query.source_device = Some("KEYBOARD".to_owned());
                query.original_calls = Some(1);
                query.original_result_bits = Some(0);
                query.effective_result_bits = Some(0);
                query.query_overridden = Some(true);
                resumed.extend([query, witness]);
                for event in &mut resumed {
                    if event.command_sequence.is_some() {
                        event.command_sequence = Some(2);
                        event.owner_generation = Some(11);
                    }
                }
                events.extend(resumed);
                resequence(&mut events);
                (manifest(scenario, true), events)
            }
            "room" => lifecycle_fixture(scenario, "ROOM_ENTER", "ROOM_EXIT"),
            "menu" => {
                let mut manifest = manifest(scenario, true);
                manifest.client_retired = true;
                let mut events = accepted_trace(neutral);
                events.push(lifecycle("RUN_TEARDOWN", 10));
                resequence(&mut events);
                (manifest, events)
            }
            "queue-full" => {
                let mut events = rejected_trace("QUEUE_FULL", neutral);
                events
                    .iter_mut()
                    .find(|event| event.stage == "reject")
                    .unwrap()
                    .queue_depth = Some(256);
                (manifest(scenario, true), events)
            }
            "expired" => {
                let mut events = rejected_trace("EXPIRED", neutral);
                for event in &mut events {
                    if event.target_tick.is_some() {
                        event.target_tick = Some(9);
                    }
                    if event.stage == "reject" {
                        event.authority_tick = 10;
                    }
                }
                (manifest(scenario, true), events)
            }
            "too-far" => {
                let mut events = rejected_trace("TOO_FAR", neutral);
                for event in &mut events {
                    if event.target_tick.is_some() {
                        event.target_tick = Some(75);
                    }
                    if event.stage == "reject" {
                        event.authority_tick = 10;
                    }
                }
                (manifest(scenario, true), events)
            }
            "stale" => {
                let mut events = rejected_trace("STALE_SEQUENCE", neutral);
                for event in &mut events {
                    if event.command_sequence.is_some() {
                        event.command_sequence = Some(4);
                    }
                    if event.stage == "reject" {
                        event.previous_command_sequence = Some(5);
                    }
                }
                (manifest(scenario, true), events)
            }
            "conflict" => {
                let mut manifest = manifest(scenario, true);
                manifest.client_retired = true;
                let mut events = accepted_trace(neutral);
                events.truncate(5);
                let mut retry_send = event("send", "PHYSICAL_SAMPLE", neutral);
                retry_send.attempt = 1;
                let mut retry_receive = event("receive", "DRAIN_BEFORE_RESOLVE", neutral);
                retry_receive.attempt = 1;
                let mut reject = event("reject", "DRAIN_BEFORE_RESOLVE", neutral);
                reject.attempt = 1;
                reject.decision = Some("REJECTED".to_owned());
                reject.reject_reason = Some("IDENTITY_CONFLICT".to_owned());
                let mut witness = event("witness", "WITNESS", neutral);
                witness.attempt = 1;
                witness.world_witness_before = Some("unchanged".to_owned());
                witness.world_witness_after = Some("unchanged".to_owned());
                let conflicting = InputStateDraftV1::from_q15(false, 1, 0, 0, 0, 0).unwrap();
                let conflicting_hex = state_hex(conflicting);
                retry_send.conflicting_state_hex = Some(conflicting_hex.clone());
                retry_receive.conflicting_state_hex = Some(conflicting_hex.clone());
                reject.conflicting_state_hex = Some(state_hex(conflicting));
                events.extend([retry_send, retry_receive, reject, witness]);
                resequence(&mut events);
                (manifest, events)
            }
            "duplicate" => {
                let mut events = accepted_trace(neutral);
                let mut retry_send = event("send", "PHYSICAL_SAMPLE", neutral);
                retry_send.attempt = 1;
                let mut retry_receive = event("receive", "DRAIN_BEFORE_RESOLVE", neutral);
                retry_receive.attempt = 1;
                let mut duplicate = event("duplicate", "DRAIN_BEFORE_RESOLVE", neutral);
                duplicate.attempt = 1;
                duplicate.decision = Some("DUPLICATE".to_owned());
                duplicate.reject_reason = Some("NONE".to_owned());
                events.extend([retry_send, retry_receive, duplicate]);
                resequence(&mut events);
                (manifest(scenario, true), events)
            }
            "loopback" => {
                let loopback_state =
                    InputStateDraftV1::from_q15(false, 0, 20_000, 0, 0, 0).unwrap();
                let mut events = accepted_trace(loopback_state);
                let apply_index = events
                    .iter()
                    .position(|event| event.stage == "apply-commit")
                    .unwrap();
                let mut suppression = event("suppression", "INPUT_QUERY", loopback_state);
                suppression.original_calls = Some(1);
                suppression.original_result_bits = Some(1);
                suppression.effective_result_bits = Some(0);
                suppression.query_overridden = Some(true);
                let mut query = event("query-override", "INPUT_QUERY", loopback_state);
                query.authority_tick = 12;
                query.action = Some(0);
                query.query_kind = Some("SCALAR".to_owned());
                query.consumer = Some("MOVEMENT".to_owned());
                query.original_calls = Some(1);
                query.original_result_bits = Some(0);
                query.effective_result_bits = Some(1);
                query.query_overridden = Some(true);
                events.insert(apply_index, suppression);
                events.insert(apply_index + 2, query);
                resequence(&mut events);
                (manifest(scenario, true), events)
            }
            _ => panic!("unknown fixture scenario {scenario}"),
        }
    }

    fn applied_neutral_handoff_fixture() -> (ActiveManifest, Vec<ActiveEvent>) {
        let neutral = InputStateDraftV1::neutral();
        let pressed = InputStateDraftV1::from_q15(false, 0, i16::MAX, 0, 0, 0).unwrap();
        let mut neutral_trace = accepted_trace(neutral);
        let mut pending_trace = accepted_trace(pressed);
        for event in &mut neutral_trace {
            event.command_sequence = Some(3);
            event.target_tick = Some(4);
            event.authority_tick =
                if matches!(event.stage.as_str(), "resolve" | "apply-commit" | "witness") {
                    4
                } else {
                    2
                };
        }
        for event in &mut pending_trace {
            event.command_sequence = Some(5);
            event.target_tick = Some(5);
            event.authority_tick =
                if matches!(event.stage.as_str(), "resolve" | "apply-commit" | "witness") {
                    5
                } else {
                    3
                };
        }

        let mut suppressed = event("query-override", "INPUT_QUERY", neutral);
        suppressed.command_sequence = Some(3);
        suppressed.target_tick = Some(4);
        suppressed.authority_tick = 4;
        suppressed.action = Some(1);
        suppressed.query_kind = Some("SCALAR".to_owned());
        suppressed.consumer = Some("MOVEMENT".to_owned());
        suppressed.source_device = Some("KEYBOARD".to_owned());
        suppressed.original_calls = Some(1);
        suppressed.original_result_bits = Some(1.0_f32.to_bits());
        suppressed.effective_result_bits = Some(0);
        suppressed.query_overridden = Some(true);

        let mut effective = event("query-override", "INPUT_QUERY", pressed);
        effective.command_sequence = Some(5);
        effective.target_tick = Some(5);
        effective.authority_tick = 5;
        effective.action = Some(1);
        effective.query_kind = Some("SCALAR".to_owned());
        effective.consumer = Some("MOVEMENT".to_owned());
        effective.source_device = Some("KEYBOARD".to_owned());
        effective.original_calls = Some(1);
        effective.original_result_bits = Some(1.0_f32.to_bits());
        effective.effective_result_bits = Some(1.0_f32.to_bits());
        effective.query_overridden = Some(true);

        let mut events = Vec::new();
        events.extend(neutral_trace.drain(..5));
        events.extend(pending_trace.drain(..5));
        events.extend(neutral_trace.drain(..2));
        events.push(suppressed);
        events.extend(neutral_trace);
        events.extend(pending_trace.drain(..2));
        events.push(effective);
        events.extend(pending_trace);
        resequence(&mut events);
        (manifest("loopback", true), events)
    }

    fn loop_event(kind: &str, loop_iteration: u64, qpc: i64) -> LoopEvent {
        LoopEvent {
            sequence: qpc as u64,
            native_sequence: qpc as u64,
            qpc,
            loop_iteration,
            phase: "FIXTURE".to_owned(),
            kind: kind.to_owned(),
            thread_id: 11,
            action: None,
            value_bits: None,
            flags: 0,
            return_rva: "0x00000000".to_owned(),
            original_calls: 1,
            error_code: 0,
            device_key: None,
            device_kind: None,
            install_failure_stage: None,
            install_failure_index: None,
            install_failure_win32_error: None,
        }
    }

    fn active_lifecycle(kind: &str, loop_iteration: u64, qpc: u64) -> ActiveEvent {
        let mut event = lifecycle(kind, 9);
        event.loop_iteration = loop_iteration;
        event.qpc = qpc;
        event
    }

    fn matrix_v2() -> ActiveMatrixV2 {
        ActiveMatrixV2 {
            schema_version: ACTIVE_MATRIX_SCHEMA_VERSION,
            matrix_identity: ACTIVE_MATRIX_IDENTITY.to_owned(),
            cases: ACTIVE_SCENARIOS
                .iter()
                .map(|scenario| ActiveMatrixCase {
                    case_name: (*scenario).to_owned(),
                    analysis_path: PathBuf::from(format!("primary/{scenario}/analysis.json")),
                })
                .collect(),
            repeatability_analyses: vec![
                PathBuf::from("repeatability/loopback-a/analysis.json"),
                PathBuf::from("repeatability/loopback-b/analysis.json"),
            ],
        }
    }

    #[test]
    fn active_matrix_v2_freezes_ordered_eighteen_plus_two_sources() {
        validate_active_matrix_shape(&matrix_v2()).unwrap();
    }

    #[test]
    fn checked_in_matrix_template_has_the_exact_ordered_shape() {
        let path =
            Path::new(env!("CARGO_MANIFEST_DIR")).join("input-active-matrix-v2.template.json");
        let matrix: ActiveMatrixV2 = serde_json::from_slice(&fs::read(path).unwrap()).unwrap();
        validate_active_matrix_shape(&matrix).unwrap();
    }

    #[test]
    fn active_matrix_v2_mutations_fail_closed() {
        let mut matrix = matrix_v2();
        matrix.schema_version = 1;
        assert!(validate_active_matrix_shape(&matrix).is_err());

        let mut matrix = matrix_v2();
        matrix.matrix_identity = "IsaacCsT0-ActiveMatrix-v1".to_owned();
        assert!(validate_active_matrix_shape(&matrix).is_err());

        let mut matrix = matrix_v2();
        matrix.cases.swap(0, 1);
        assert!(validate_active_matrix_shape(&matrix).is_err());

        let mut matrix = matrix_v2();
        matrix.cases[1].analysis_path = matrix.cases[0].analysis_path.clone();
        assert!(validate_active_matrix_shape(&matrix).is_err());

        let mut matrix = matrix_v2();
        matrix.cases[0].analysis_path = PathBuf::from("../escape/analysis.json");
        assert!(validate_active_matrix_shape(&matrix).is_err());

        let mut matrix = matrix_v2();
        matrix.repeatability_analyses[1] = matrix.repeatability_analyses[0].clone();
        assert!(validate_active_matrix_shape(&matrix).is_err());

        let mut matrix = matrix_v2();
        matrix.repeatability_analyses[0] = matrix.cases[17].analysis_path.clone();
        assert!(validate_active_matrix_shape(&matrix).is_err());
    }

    #[test]
    fn eighteen_scenario_semantics_have_table_driven_positive_fixtures() {
        for scenario in ACTIVE_SCENARIOS {
            let (manifest, events) = scenario_fixture(scenario);
            let groups = groups(&events);
            assert!(
                scenario_predicate(&manifest, &events, &groups),
                "scenario fixture {scenario} did not satisfy its dedicated predicate"
            );
        }
    }

    #[test]
    fn disarmed_control_writer_creates_gate_eligible_schema4_source_and_rejects_drift() {
        let directory = tempfile::tempdir().unwrap();
        let slots = target_slots();
        let config = DisarmedControlRunConfig {
            case_id: "control-no-native-writer".to_owned(),
            probe: "control-no-native".to_owned(),
            scenario: "control-no-native".to_owned(),
            target_sha256: FIXED_TARGET_SHA256.to_owned(),
            observed_target_sha256: crate::target_equivalence::APPROVED_UNPACKED_TARGET_SHA256
                .to_owned(),
            target_equivalence_sha256: "9".repeat(64),
            mode_gate_sha256: "A".repeat(64),
            loop_gate_sha256: "B".repeat(64),
            raw_prereq_sha256: "C".repeat(64),
            active_catalog_sha256: "D".repeat(64),
            active_plan_sha256: "E".repeat(64),
            original_online_observation: None,
            artifact_identity: "disarmed-control-writer".to_owned(),
            observation_id: [0xA5; 16],
            observer_id: 7,
            observation_qpc: 11,
            target_slots_before: slots.clone(),
            target_slots_after: slots,
        };
        let output = directory.path().join("source");
        let report = write_disarmed_control_run(&output, config.clone()).unwrap();
        assert_eq!(report.result, "PASS");
        assert!(report.eligible_for_active_gate);
        assert_eq!(report.event_count, 1);
        assert_eq!(report.accepted_count, 0);
        assert_eq!(report.apply_count, 0);
        assert_eq!(report.checks.len(), 21);
        assert!(report.checks.iter().all(|check| check.passed));

        let (manifest, events) = read_active_run(&output).unwrap();
        assert_eq!(manifest.evidence_class, EVIDENCE_DISARMED_CONTROL);
        assert_eq!(manifest.ipc_magic, ACTIVE_CONTROL_SOURCE_FORMAT);
        assert!(manifest.ipc_schema_version.is_none());
        assert!(manifest.arm_token_sha256.is_none());
        assert!(!manifest.arm_token_consumed);
        assert!(manifest.companion_loop_artifact_path.is_none());
        assert_eq!(manifest.installed_active_hooks_mask, "0x0");
        assert_eq!(events.len(), 1);
        assert_eq!(events[0].stage, "control-observation");
        assert!(events[0].session_id.is_none());

        let mut drifted = config;
        drifted.target_slots_after[0].bytes_hex = "FF".repeat(5);
        assert!(validate_disarmed_control_run_config(&drifted).is_err());
        assert!(!directory.path().join("drifted").exists());
    }

    #[test]
    fn original_online_disarmed_writer_requires_stable_positive_observation() {
        let directory = tempfile::tempdir().unwrap();
        let slots = target_slots();
        let config = DisarmedControlRunConfig {
            case_id: "original-online-writer".to_owned(),
            probe: "original-online-disarmed".to_owned(),
            scenario: "original-online-disarmed".to_owned(),
            target_sha256: FIXED_TARGET_SHA256.to_owned(),
            observed_target_sha256: crate::target_equivalence::APPROVED_UNPACKED_TARGET_SHA256
                .to_owned(),
            target_equivalence_sha256: "9".repeat(64),
            mode_gate_sha256: "A".repeat(64),
            loop_gate_sha256: "B".repeat(64),
            raw_prereq_sha256: "C".repeat(64),
            active_catalog_sha256: "D".repeat(64),
            active_plan_sha256: "E".repeat(64),
            original_online_observation: Some(original_online_observation()),
            artifact_identity: "original-online-disarmed-writer".to_owned(),
            observation_id: [0xA6; 16],
            observer_id: 8,
            observation_qpc: 20,
            target_slots_before: slots.clone(),
            target_slots_after: slots,
        };
        let output = directory.path().join("source");
        let report = write_disarmed_control_run(&output, config.clone()).unwrap();
        assert_eq!(report.result, "PASS");
        assert_eq!(report.scenario, "original-online-disarmed");
        assert_eq!(report.event_count, 1);
        assert!(report.checks.iter().all(|check| check.passed));

        let (manifest, _) = read_active_run(&output).unwrap();
        assert_eq!(manifest.endpoint, "original-online-read-only");
        assert!(manifest.original_online_observation.is_some());

        let mut missing = config.clone();
        missing.original_online_observation = None;
        assert!(validate_disarmed_control_run_config(&missing).is_err());

        let mut invalid = config;
        invalid
            .original_online_observation
            .as_mut()
            .unwrap()
            .roster_count = 1;
        assert!(validate_disarmed_control_run_config(&invalid).is_err());
    }

    #[test]
    fn semantic_mutations_of_reason_target_canonical_edge_and_attempt_fail_closed() {
        let (manifest, mut events) = scenario_fixture("queue-full");
        events
            .iter_mut()
            .find(|event| event.stage == "reject")
            .unwrap()
            .reject_reason = Some("TOO_FAR".to_owned());
        assert!(!scenario_predicate(&manifest, &events, &groups(&events)));

        let (manifest, mut events) = scenario_fixture("expired");
        let reject = events
            .iter_mut()
            .find(|event| event.stage == "reject")
            .unwrap();
        reject.target_tick = Some(reject.authority_tick);
        assert!(!scenario_predicate(&manifest, &events, &groups(&events)));

        let (_, mut events) = scenario_fixture("keyboard");
        let canonical = events
            .iter_mut()
            .find(|event| event.stage == "canonical")
            .unwrap();
        canonical.canonical_state_hex = Some("00".repeat(24));
        assert!(!command_payload_valid(canonical));

        let (_, mut events) = scenario_fixture("rapid-tap");
        events[0].edge_flags = Some(0);
        assert!(!command_payload_valid(&events[0]));

        let (manifest, mut events) = scenario_fixture("duplicate");
        events
            .iter_mut()
            .find(|event| event.stage == "duplicate")
            .unwrap()
            .attempt = 0;
        assert!(!scenario_predicate(&manifest, &events, &groups(&events)));
    }

    #[test]
    fn merged_effective_bucket_lead_tail_and_generation_cancel_fail_closed() {
        let first_state = InputStateDraftV1::from_q15(false, 1, 0, 0, 0, 0).unwrap();
        let winner_state = InputStateDraftV1::from_q15(false, 2, 20_000, 0, 0, 0).unwrap();
        let effective_state = InputStateDraftV1::from_q15(false, 3, 20_000, 0, 0, 0).unwrap();
        let mut first = accepted_trace(first_state);
        first.retain(|event| {
            matches!(
                event.stage.as_str(),
                "sample" | "canonical" | "send" | "receive" | "accept"
            )
        });
        let mut winner = accepted_trace(winner_state);
        for event in &mut winner {
            if event.command_sequence.is_some() {
                event.command_sequence = Some(2);
            }
            if matches!(event.stage.as_str(), "resolve" | "apply-commit" | "witness") {
                event.canonical_state_hex = Some(state_hex(effective_state));
                event.edge_flags = Some(3);
            }
        }
        let mut merged = first;
        merged.extend(winner);
        resequence(&mut merged);
        let merged_manifest = manifest("keyboard", true);
        assert!(generation_timeline_valid(&merged));
        assert!(accepted_bucket_contract(
            &merged_manifest,
            &merged,
            &groups(&merged)
        ));

        let mut dropped_edge = merged.clone();
        let apply = dropped_edge
            .iter_mut()
            .find(|event| event.stage == "apply-commit")
            .unwrap();
        apply.canonical_state_hex = Some(state_hex(winner_state));
        apply.edge_flags = Some(2);
        assert!(!accepted_bucket_contract(
            &merged_manifest,
            &dropped_edge,
            &groups(&dropped_edge)
        ));

        let mut overlapping_edge = merged.clone();
        for event in overlapping_edge.iter_mut().filter(|event| {
            event.command_sequence == Some(1)
                && matches!(
                    event.stage.as_str(),
                    "sample" | "canonical" | "send" | "receive" | "accept"
                )
        }) {
            event.canonical_state_hex = Some(state_hex(
                InputStateDraftV1::from_q15(false, 2, 0, 0, 0, 0).unwrap(),
            ));
            event.edge_flags = Some(2);
        }
        assert!(!accepted_bucket_contract(
            &merged_manifest,
            &overlapping_edge,
            &groups(&overlapping_edge)
        ));

        let mut tail = accepted_trace(InputStateDraftV1::neutral());
        tail.retain(|event| {
            matches!(
                event.stage.as_str(),
                "sample" | "canonical" | "send" | "receive" | "accept"
            )
        });
        resequence(&mut tail);
        assert!(accepted_bucket_contract(
            &merged_manifest,
            &tail,
            &groups(&tail)
        ));
        let mut too_far_tail = tail.clone();
        for event in &mut too_far_tail {
            event.target_tick = Some(75);
        }
        assert!(!accepted_bucket_contract(
            &merged_manifest,
            &too_far_tail,
            &groups(&too_far_tail)
        ));

        let mut cancelled = tail;
        cancelled.push(lifecycle("PAUSE_ENTER", 10));
        resequence(&mut cancelled);
        assert!(generation_timeline_valid(&cancelled));
        assert!(accepted_bucket_contract(
            &merged_manifest,
            &cancelled,
            &groups(&cancelled)
        ));
        let mut stale = cancelled;
        stale.push(event(
            "sample",
            "PHYSICAL_SAMPLE",
            InputStateDraftV1::neutral(),
        ));
        resequence(&mut stale);
        assert!(!generation_timeline_valid(&stale));
    }

    #[test]
    fn lifecycle_and_loopback_order_mutations_fail_closed() {
        let (manifest, mut events) = scenario_fixture("pause");
        let enter = events
            .iter()
            .position(|event| event.lifecycle_kind.as_deref() == Some("PAUSE_ENTER"))
            .unwrap();
        events.insert(
            enter + 1,
            event("send", "PHYSICAL_SAMPLE", InputStateDraftV1::neutral()),
        );
        assert!(!scenario_predicate(&manifest, &events, &groups(&events)));

        let (manifest, mut events) = scenario_fixture("loading");
        assert!(
            events
                .iter()
                .find(|event| event.stage == "resolve")
                .is_some_and(|event| event.source_device.is_none())
        );
        assert!(scenario_predicate(&manifest, &events, &groups(&events)));
        events
            .iter_mut()
            .find(|event| event.stage == "resolve")
            .unwrap()
            .source_device = Some("KEYBOARD".to_owned());
        assert!(!scenario_predicate(&manifest, &events, &groups(&events)));

        let (manifest, mut events) = scenario_fixture("loading");
        events
            .iter_mut()
            .find(|event| event.stage == "apply-commit")
            .unwrap()
            .source_device = None;
        assert!(!scenario_predicate(&manifest, &events, &groups(&events)));

        let (manifest, mut events) = scenario_fixture("loading");
        events.insert(
            0,
            event("sample", "PHYSICAL_SAMPLE", InputStateDraftV1::neutral()),
        );
        resequence(&mut events);
        assert!(!scenario_predicate(&manifest, &events, &groups(&events)));

        let (manifest, mut events) = scenario_fixture("loading");
        events.retain(|event| event.stage != "query-override");
        resequence(&mut events);
        assert!(!scenario_predicate(&manifest, &events, &groups(&events)));

        let (manifest, mut events) = scenario_fixture("loading");
        let exit = events
            .iter()
            .position(|event| event.lifecycle_kind.as_deref() == Some("LOADING_EXIT"))
            .unwrap();
        events.truncate(exit + 1);
        assert!(!scenario_predicate(&manifest, &events, &groups(&events)));

        let (manifest, mut events) = scenario_fixture("loading");
        events
            .iter_mut()
            .find(|event| event.stage == "sample")
            .unwrap()
            .source_device = Some("GAMEPAD".to_owned());
        assert!(!scenario_predicate(&manifest, &events, &groups(&events)));

        let (manifest, mut events) = scenario_fixture("loading");
        events
            .iter_mut()
            .find(|event| event.stage == "sample")
            .unwrap()
            .owner_generation = Some(12);
        assert!(!scenario_predicate(&manifest, &events, &groups(&events)));

        let (manifest, mut events) = scenario_fixture("loopback");
        let suppression = events
            .iter()
            .position(|event| event.stage == "suppression")
            .unwrap();
        let query = events
            .iter()
            .position(|event| event.stage == "query-override")
            .unwrap();
        events.swap(suppression, query);
        assert!(!scenario_predicate(&manifest, &events, &groups(&events)));
    }

    #[test]
    fn loopback_accepts_applied_neutral_handoff_and_rejects_broken_proof() {
        let (manifest, events) = applied_neutral_handoff_fixture();
        assert!(scenario_predicate(&manifest, &events, &groups(&events)));

        let mut late_accept = events.clone();
        let suppression_sequence = late_accept
            .iter()
            .find(|event| {
                event.command_sequence == Some(3)
                    && event.stage == "query-override"
                    && event.original_result_bits == Some(1.0_f32.to_bits())
            })
            .unwrap()
            .event_sequence;
        late_accept
            .iter_mut()
            .find(|event| event.command_sequence == Some(5) && event.stage == "accept")
            .unwrap()
            .event_sequence = suppression_sequence + 1;
        assert!(!scenario_predicate(
            &manifest,
            &late_accept,
            &groups(&late_accept)
        ));

        let mut wrong_action = events.clone();
        wrong_action
            .iter_mut()
            .find(|event| {
                event.command_sequence == Some(5)
                    && event.stage == "query-override"
                    && event.effective_result_bits == Some(1.0_f32.to_bits())
            })
            .unwrap()
            .action = Some(0);
        assert!(!scenario_predicate(
            &manifest,
            &wrong_action,
            &groups(&wrong_action)
        ));

        let mut missing_neutral_apply = events;
        missing_neutral_apply
            .retain(|event| event.command_sequence != Some(3) || event.stage != "apply-commit");
        resequence(&mut missing_neutral_apply);
        assert!(!scenario_predicate(
            &manifest,
            &missing_neutral_apply,
            &groups(&missing_neutral_apply)
        ));
    }

    #[test]
    fn lifecycle_labels_require_reverse_confirmed_v5_transaction_shapes() {
        let loading_kinds = [
            "ALTERNATE_OWNER_ENTER",
            "ALTERNATE_STATE_APPLY_ENTER",
            "ALTERNATE_STATE_APPLY_EXIT",
            "ALTERNATE_ROOM_ENTER",
            "ENTITY_LIST_ENTER",
            "ENTITY_LIST_EXIT",
            "ALTERNATE_ROOM_EXIT",
            "ALTERNATE_TAIL_ENTER",
            "ALTERNATE_TAIL_EXIT",
            "ALTERNATE_OWNER_EXIT",
        ];
        let loading_loop = loading_kinds
            .iter()
            .enumerate()
            .map(|(index, kind)| loop_event(kind, 5, 10 + index as i64 * 10))
            .collect::<Vec<_>>();
        let loading_active = vec![
            active_lifecycle("LOADING_ENTER", 5, 15),
            active_lifecycle("LOADING_EXIT", 5, 95),
        ];
        assert!(lifecycle_companion_shape_valid(
            &manifest("loading", true),
            &loading_active,
            &loading_loop,
        ));
        let mut missing_entity = loading_loop.clone();
        missing_entity.retain(|event| event.kind != "ENTITY_LIST_EXIT");
        assert!(!lifecycle_companion_shape_valid(
            &manifest("loading", true),
            &loading_active,
            &missing_entity,
        ));

        let room_kinds = [
            "ROOM_CHANGE_OWNER_ENTER",
            "NEW_ROOM_ENTER",
            "NEW_ROOM_EXIT",
            "ROOM_CHANGE_SIMULATION_ENTER",
            "ENTITY_LIST_ENTER",
            "ENTITY_LIST_EXIT",
            "ROOM_CHANGE_SIMULATION_EXIT",
            "ROOM_CHANGE_OWNER_EXIT",
        ];
        let room_loop = room_kinds
            .iter()
            .enumerate()
            .map(|(index, kind)| loop_event(kind, 8, 10 + index as i64 * 10))
            .collect::<Vec<_>>();
        let room_active = vec![
            active_lifecycle("ROOM_ENTER", 8, 15),
            active_lifecycle("ROOM_EXIT", 8, 75),
        ];
        assert!(lifecycle_companion_shape_valid(
            &manifest("room", true),
            &room_active,
            &room_loop,
        ));
        let mut owner_attempts = room_loop.clone();
        for attempt in 0..8_u64 {
            owner_attempts.push(loop_event(
                "ROOM_CHANGE_OWNER_ENTER",
                10 + attempt * 2,
                100 + attempt as i64 * 20,
            ));
            owner_attempts.push(loop_event(
                "ROOM_CHANGE_OWNER_EXIT",
                10 + attempt * 2,
                110 + attempt as i64 * 20,
            ));
        }
        assert!(lifecycle_companion_shape_valid(
            &manifest("room", true),
            &room_active,
            &owner_attempts,
        ));
        let mut duplicate_label = room_active.clone();
        duplicate_label.push(active_lifecycle("ROOM_ENTER", 10, 105));
        duplicate_label.push(active_lifecycle("ROOM_EXIT", 10, 109));
        assert!(!lifecycle_companion_shape_valid(
            &manifest("room", true),
            &duplicate_label,
            &owner_attempts,
        ));
        let mut wrong_order = room_loop.clone();
        wrong_order.swap(3, 6);
        assert!(!lifecycle_companion_shape_valid(
            &manifest("room", true),
            &room_active,
            &wrong_order,
        ));

        let mut generation_events = vec![event(
            "sample",
            "PHYSICAL_SAMPLE",
            InputStateDraftV1::neutral(),
        )];
        generation_events[0].owner_generation = Some(1);
        for generation in 2..=19_u64 {
            let mut clear = active_lifecycle("ROOM_ENTER", 8, generation);
            clear.lifecycle_kind = if generation == 3 {
                Some("ROOM_ENTER".to_owned())
            } else if generation == 4 {
                Some("ROOM_EXIT".to_owned())
            } else {
                None
            };
            clear.owner_generation = Some(generation);
            generation_events.push(clear);
        }
        let mut post_attempt_sample =
            event("sample", "PHYSICAL_SAMPLE", InputStateDraftV1::neutral());
        post_attempt_sample.owner_generation = Some(19);
        generation_events.push(post_attempt_sample);
        assert!(generation_timeline_valid(&generation_events));

        let menu_loop = vec![
            loop_event("RUN_TEARDOWN_ENTER", 13, 20),
            loop_event("RUN_TEARDOWN_EXIT", 13, 40),
        ];
        let menu_active = vec![active_lifecycle("RUN_TEARDOWN", 13, 25)];
        assert!(lifecycle_companion_shape_valid(
            &manifest("menu", true),
            &menu_active,
            &menu_loop,
        ));
        let label_only = vec![loop_event("LOOP_BEGIN", 13, 20)];
        assert!(!lifecycle_companion_shape_valid(
            &manifest("menu", true),
            &menu_active,
            &label_only,
        ));
    }

    #[test]
    fn pause_lifecycle_requires_a_bracketed_standard_transaction_gap() {
        let loop_events = vec![
            loop_event("UPDATE_ENTER", 1, 10),
            loop_event("STANDARD_ROOM_ENTER", 1, 12),
            loop_event("STANDARD_ROOM_EXIT", 1, 18),
            loop_event("UPDATE_EXIT", 1, 19),
            loop_event("UPDATE_ENTER", 2, 20),
            loop_event("UPDATE_EXIT", 2, 29),
            loop_event("UPDATE_ENTER", 3, 30),
            loop_event("UPDATE_EXIT", 3, 39),
            loop_event("UPDATE_ENTER", 4, 40),
            loop_event("STANDARD_ROOM_ENTER", 4, 45),
            loop_event("STANDARD_ROOM_EXIT", 4, 55),
            loop_event("UPDATE_EXIT", 4, 59),
        ];
        let active_events = vec![
            active_lifecycle("PAUSE_ENTER", 3, 38),
            active_lifecycle("PAUSE_EXIT", 4, 46),
        ];
        assert!(lifecycle_companion_shape_valid(
            &manifest("pause", true),
            &active_events,
            &loop_events,
        ));
        let mut unbracketed = loop_events.clone();
        unbracketed
            .retain(|event| event.loop_iteration != 1 || !event.kind.starts_with("STANDARD_ROOM_"));
        assert!(!lifecycle_companion_shape_valid(
            &manifest("pause", true),
            &active_events,
            &unbracketed,
        ));

        let one_missing_active = vec![
            active_lifecycle("PAUSE_ENTER", 2, 28),
            active_lifecycle("PAUSE_EXIT", 3, 35),
        ];
        let one_missing_loop = vec![
            loop_event("UPDATE_ENTER", 1, 10),
            loop_event("STANDARD_ROOM_ENTER", 1, 12),
            loop_event("STANDARD_ROOM_EXIT", 1, 18),
            loop_event("UPDATE_EXIT", 1, 19),
            loop_event("UPDATE_ENTER", 2, 20),
            loop_event("UPDATE_EXIT", 2, 29),
            loop_event("UPDATE_ENTER", 3, 30),
            loop_event("STANDARD_ROOM_ENTER", 3, 34),
            loop_event("STANDARD_ROOM_EXIT", 3, 38),
            loop_event("UPDATE_EXIT", 3, 39),
        ];
        assert!(!lifecycle_companion_shape_valid(
            &manifest("pause", true),
            &one_missing_active,
            &one_missing_loop,
        ));

        let mut delayed_enter_loop = loop_events.clone();
        delayed_enter_loop.splice(
            8..8,
            [
                loop_event("UPDATE_ENTER", 4, 40),
                loop_event("UPDATE_EXIT", 4, 49),
            ],
        );
        for event in delayed_enter_loop.iter_mut().skip(10) {
            event.loop_iteration = 5;
            event.qpc += 10;
        }
        let delayed_enter_active = vec![
            active_lifecycle("PAUSE_ENTER", 4, 48),
            active_lifecycle("PAUSE_EXIT", 5, 56),
        ];
        assert!(!lifecycle_companion_shape_valid(
            &manifest("pause", true),
            &delayed_enter_active,
            &delayed_enter_loop,
        ));
    }

    #[test]
    fn companion_control_plane_is_ordered_on_worker_threads_while_data_stays_in_loop_bounds() {
        let mut initialized = active_lifecycle("INITIALIZED", 0, 10);
        initialized.thread_id = 21;
        let mut installed = active_lifecycle("HOOKS_INSTALLED", 0, 31);
        installed.thread_id = 21;
        let mut data = event("sample", "PHYSICAL_SAMPLE", InputStateDraftV1::neutral());
        data.loop_iteration = 7;
        data.qpc = 50;
        data.thread_id = 11;
        let mut pause = active_lifecycle("PAUSE_ENTER", 7, 60);
        pause.thread_id = 11;
        let mut restored = active_lifecycle("HOOKS_RESTORED", 7, 90);
        restored.thread_id = 31;
        let mut closed = active_lifecycle("PRODUCER_CLOSED", 7, 91);
        closed.thread_id = 31;
        let active = vec![initialized, installed, data, pause, restored, closed];
        let mut loop_initialized = loop_event("NATIVE_INITIALIZED", 0, 20);
        loop_initialized.thread_id = 21;
        let mut loop_installed = loop_event("HOOKS_INSTALLED", 0, 30);
        loop_installed.thread_id = 21;
        let mut loop_restored = loop_event("HOOKS_RESTORED", 7, 89);
        loop_restored.thread_id = 31;
        let loop_events = vec![loop_initialized, loop_installed, loop_restored];
        let loop_bounds = BTreeMap::from([(7, (40, 80))]);
        assert!(active_control_plane_matches_companion(
            &active,
            &loop_events,
        ));
        assert!(active_data_plane_has_loop_context(
            &active,
            &loop_bounds,
            11,
        ));

        let mut wrong_shutdown_thread = active.clone();
        wrong_shutdown_thread.last_mut().unwrap().thread_id = 32;
        assert!(!active_control_plane_matches_companion(
            &wrong_shutdown_thread,
            &loop_events,
        ));
        let mut duplicate_control = active.clone();
        duplicate_control.insert(2, duplicate_control[1].clone());
        assert!(!active_control_plane_matches_companion(
            &duplicate_control,
            &loop_events,
        ));
        let mut spoofed_stage = active.clone();
        spoofed_stage[0].stage = "sample".to_owned();
        assert!(!active_control_plane_matches_companion(
            &spoofed_stage,
            &loop_events,
        ));
        let mut spoofed_phase = active.clone();
        spoofed_phase[1].phase = "PHYSICAL_SAMPLE".to_owned();
        assert!(!active_control_plane_matches_companion(
            &spoofed_phase,
            &loop_events,
        ));
        let mut data_without_context = active;
        data_without_context[2].loop_iteration = 0;
        assert!(!active_data_plane_has_loop_context(
            &data_without_context,
            &loop_bounds,
            11,
        ));
        data_without_context[2].loop_iteration = 7;
        data_without_context[3].loop_iteration = 0;
        assert!(!active_data_plane_has_loop_context(
            &data_without_context,
            &loop_bounds,
            11,
        ));
    }

    #[test]
    fn semantic_repeatability_normalizes_runtime_identity_but_not_meaning() {
        let (_, first) = scenario_fixture("loopback");
        let mut second = first.clone();
        for event in &mut second {
            event.qpc += 10_000;
            event.authority_tick += 100;
            if let Some(target) = &mut event.target_tick {
                *target += 100;
            }
            if let Some(client) = &mut event.client_id {
                *client += 80;
            }
            if let Some(sequence) = &mut event.command_sequence {
                *sequence += 500;
            }
            if let Some(session) = &mut event.session_id {
                *session = "independent-session".to_owned();
            }
        }
        assert_eq!(semantic_trace(&first), semantic_trace(&second));
        second
            .iter_mut()
            .find(|event| event.stage == "query-override")
            .unwrap()
            .action = Some(1);
        assert_ne!(semantic_trace(&first), semantic_trace(&second));
    }

    fn shutdown_repeatability_pair() -> (Vec<ActiveEvent>, Vec<ActiveEvent>) {
        let neutral = InputStateDraftV1::neutral();
        let mut without_tail = accepted_trace(neutral);
        let mut hooks_restored = lifecycle("HOOKS_RESTORED", 9);
        hooks_restored.authority_tick = 12;
        hooks_restored.loop_iteration = 2;
        let mut producer_closed = lifecycle("PRODUCER_CLOSED", 9);
        producer_closed.authority_tick = 12;
        producer_closed.loop_iteration = 2;
        without_tail.extend([hooks_restored, producer_closed]);
        resequence(&mut without_tail);

        let mut tail = accepted_trace(neutral);
        tail.truncate(5);
        for event in &mut tail {
            event.command_sequence = Some(2);
            event.target_tick = Some(12 + INPUT_LEAD_TICKS);
            event.authority_tick = 12;
            event.loop_iteration = 2;
        }
        tail[0].sample_component_count = Some(0);
        let mut with_tail = without_tail.clone();
        let insertion = with_tail.len() - 2;
        with_tail.splice(insertion..insertion, tail);
        resequence(&mut with_tail);
        (without_tail, with_tail)
    }

    #[test]
    fn repeatability_ignores_only_strict_redundant_neutral_shutdown_tail() {
        let (without_tail, with_tail) = shutdown_repeatability_pair();
        assert_ne!(semantic_trace(&without_tail), semantic_trace(&with_tail));
        assert_eq!(
            repeatability_semantic_trace(&without_tail),
            repeatability_semantic_trace(&with_tail)
        );
        let ignored = redundant_neutral_shutdown_tail_indices(&with_tail);
        assert_eq!(ignored.len(), 5);
        assert_eq!(
            ignored.iter().copied().collect::<Vec<_>>(),
            vec![8, 9, 10, 11, 12]
        );
    }

    #[test]
    fn repeatability_shutdown_tail_mutations_remain_semantically_visible() {
        let (without_tail, with_tail) = shutdown_repeatability_pair();
        let expected = repeatability_semantic_trace(&without_tail);

        let mut changed_state = with_tail.clone();
        let changed = state_hex(InputStateDraftV1::from_q15(false, 0, i16::MAX, 0, 0, 0).unwrap());
        for event in changed_state
            .iter_mut()
            .filter(|event| event.command_sequence == Some(2))
        {
            event.canonical_state_hex = Some(changed.clone());
        }
        assert_ne!(expected, repeatability_semantic_trace(&changed_state));

        let mut wrong_target = with_tail.clone();
        for event in wrong_target
            .iter_mut()
            .filter(|event| event.command_sequence == Some(2))
        {
            event.target_tick = Some(13);
        }
        assert_ne!(expected, repeatability_semantic_trace(&wrong_target));

        let mut same_loop = with_tail.clone();
        for event in same_loop
            .iter_mut()
            .filter(|event| event.command_sequence == Some(2))
        {
            event.loop_iteration = 1;
        }
        assert_ne!(expected, repeatability_semantic_trace(&same_loop));

        let mut non_neutral_sample = with_tail.clone();
        non_neutral_sample
            .iter_mut()
            .find(|event| event.command_sequence == Some(2) && event.stage == "sample")
            .unwrap()
            .sample_component_count = Some(1);
        assert_ne!(expected, repeatability_semantic_trace(&non_neutral_sample));

        let mut resolved = with_tail;
        let hooks = resolved
            .iter()
            .position(|event| event.lifecycle_kind.as_deref() == Some("HOOKS_RESTORED"))
            .unwrap();
        let mut resolve = event("resolve", "RESOLVE", InputStateDraftV1::neutral());
        resolve.command_sequence = Some(2);
        resolve.target_tick = Some(14);
        resolve.authority_tick = 12;
        resolve.loop_iteration = 2;
        resolve.source_device = Some("KEYBOARD".to_owned());
        resolved.insert(hooks, resolve);
        resequence(&mut resolved);
        assert_ne!(expected, repeatability_semantic_trace(&resolved));
    }

    #[test]
    fn pipeline_requires_companion_hashes_and_disarmed_forbids_them() {
        let pipeline = manifest("keyboard", true);
        validate_manifest(&pipeline).unwrap();
        let mut missing = pipeline.clone();
        missing.companion_loop_events_sha256 = None;
        assert!(validate_manifest(&missing).is_err());

        let disarmed = manifest("control-no-native", false);
        validate_manifest(&disarmed).unwrap();
        let mut polluted = disarmed;
        polluted.companion_loop_artifact_path = Some(PathBuf::from("loop-context"));
        polluted.companion_loop_manifest_sha256 = Some("1".repeat(64));
        polluted.companion_loop_events_sha256 = Some("2".repeat(64));
        polluted.companion_loop_analysis_sha256 = Some("3".repeat(64));
        assert!(validate_manifest(&polluted).is_err());
    }

    #[test]
    fn pipeline_replay_fails_closed_when_loop_context_is_absent() {
        let directory = tempfile::tempdir().unwrap();
        let (manifest, events) = scenario_fixture("keyboard");
        fs::write(
            directory.path().join("manifest.json"),
            serde_json::to_vec_pretty(&manifest).unwrap(),
        )
        .unwrap();
        let events = events
            .iter()
            .map(|event| serde_json::to_string(event).unwrap())
            .collect::<Vec<_>>()
            .join("\n");
        fs::write(directory.path().join("events.jsonl"), events).unwrap();
        assert!(replay_active_run(directory.path()).is_err());
    }

    #[test]
    fn active_gate_refuses_existing_output_before_replay() {
        let directory = tempfile::tempdir().unwrap();
        let output = directory.path().join("existing.json");
        fs::write(&output, b"preserve").unwrap();
        assert!(
            write_active_gate(
                Path::new("missing-mode-gate.json"),
                Path::new("missing-loop-gate.json"),
                Path::new("missing-raw-prereq.json"),
                Path::new("missing-active-plan.json"),
                Path::new("missing-matrix.json"),
                Path::new("missing-repeatability.json"),
                &output,
            )
            .is_err()
        );
        assert_eq!(fs::read(output).unwrap(), b"preserve");
    }

    #[test]
    fn active_gate_requires_runtime_complete_verification_v2_before_replay() {
        let directory = tempfile::tempdir().unwrap();
        let verification = directory.path().join("verification-v1.json");
        fs::write(
            &verification,
            serde_json::to_vec_pretty(&crate::active_verification::frozen_active_verification_v1())
                .unwrap(),
        )
        .unwrap();
        let output = directory.path().join("gate.json");
        assert!(
            write_active_gate_with_verification(
                Path::new("missing-mode-gate.json"),
                Path::new("missing-loop-gate.json"),
                Path::new("missing-raw-prereq.json"),
                Path::new("missing-active-plan.json"),
                Path::new("missing-matrix.json"),
                Path::new("missing-repeatability.json"),
                &verification,
                &output,
            )
            .is_err()
        );
        assert!(!output.exists());
    }
}
