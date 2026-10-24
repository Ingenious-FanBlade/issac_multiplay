use std::collections::BTreeSet;
use std::fs::{self, OpenOptions};
use std::io::Write;
use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};

use crate::error::{AppError, AppResult, UsageContext};
use crate::loop_ipc::{
    LOOP_FLAG_ACTION_GAMEPLAY, LOOP_FLAG_GAMEPAD_DEVICE, LOOP_FLAG_KEYBOARD_DEVICE,
    LOOP_FLAG_LOOP_KNOWN, LOOP_FLAG_MAIN_THREAD, LOOP_FLAG_ORIGINAL_CALLED, LOOP_FLAG_RESULT_TRUE,
    LOOP_FLAG_WRONG_THREAD,
};
use crate::pe::{PeImage, sha256_hex};

pub const LOOP_LEDGER_SCHEMA_VERSION: u32 = 1;
pub const LOOP_TOOL_IDENTITY: &str = "IsaacCsT0-LoopEnvelopeResearch-v5";
pub const LOOP_EVIDENCE_CLASS: &str = "NATIVE_PASSIVE_LOOP_V5";
pub const REQUIRED_VARIANT: &str = "online-mod-patched-2026-04-21";
const REQUIRED_LOOP_GATE_SCENARIOS: &[&str] = &[
    "keyboard",
    "controller",
    "device-switch",
    "pause",
    "loading",
    "menu",
    "room",
];

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LoopLedger {
    pub schema_version: u32,
    pub ledger_id: String,
    pub experiment_identity: String,
    pub fixed_variant: FixedVariant,
    pub gate: EmbeddedGate,
    #[serde(default)]
    pub corrections: Vec<EvidenceEntry>,
    #[serde(default)]
    pub targets: Vec<EvidenceEntry>,
    #[serde(default)]
    pub action_oracles: Vec<EvidenceEntry>,
    #[serde(default)]
    pub loop_model: Vec<EvidenceEntry>,
    #[serde(default)]
    pub unresolved: Vec<EvidenceEntry>,
    #[serde(default)]
    pub constraints: Vec<EvidenceEntry>,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct FixedVariant {
    pub file_name: String,
    pub file_sha256: String,
    pub machine: String,
    pub timestamp: String,
    pub image_size: String,
    pub preferred_image_base: String,
    pub text_sha256: String,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct EmbeddedGate {
    pub status: String,
    pub reason: String,
    pub active_input_allowed: bool,
    #[serde(default)]
    pub required_evidence: Vec<String>,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct EvidenceEntry {
    #[serde(default)]
    pub id: String,
    #[serde(default)]
    pub target_id: String,
    #[serde(default)]
    pub phase: String,
    #[serde(default)]
    pub semantic: String,
    #[serde(default)]
    pub kind: String,
    #[serde(default)]
    pub evidence_label: String,
    #[serde(default)]
    pub rva: Option<String>,
    #[serde(default)]
    pub expected_bytes: Option<String>,
    #[serde(default)]
    pub expected_target_rva: Option<String>,
    #[serde(default)]
    pub rvas: Vec<String>,
    #[serde(default)]
    pub expected_bytes_by_rva: std::collections::BTreeMap<String, String>,
    #[serde(default)]
    pub evidence_sources: Vec<String>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LoopStaticCheck {
    pub id: String,
    pub evidence_label: String,
    pub rva: Option<String>,
    pub result: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LoopInspectionReport {
    pub schema_version: u32,
    pub result: String,
    pub ledger_id: String,
    pub ledger_sha256: String,
    pub variant_id: String,
    pub file_sha256: String,
    pub text_sha256: String,
    pub static_checks: Vec<LoopStaticCheck>,
    pub unresolved_evidence: Vec<String>,
    pub gate_status: String,
    pub gate_reason: String,
    pub passive_probe_allowed: bool,
    pub active_input_allowed: bool,
}

#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LoopManifest {
    pub schema_version: u32,
    pub experiment_identity: String,
    pub evidence_class: String,
    pub case_id: String,
    pub probe: String,
    pub scenario: String,
    pub target_sha256: String,
    pub loop_ledger_sha256: String,
    pub state: String,
    pub active_input_allowed: bool,
    pub native_sha256: String,
    pub helper_sha256: String,
    pub main_thread_id: u32,
    pub hooks_mask: u64,
    pub overwritten_events: u64,
    pub target_bytes_before_sha256: String,
    pub target_bytes_after_sha256: String,
    pub target_bytes_restored: bool,
}

#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LoopEvent {
    pub sequence: u64,
    pub native_sequence: u64,
    pub qpc: i64,
    pub loop_iteration: u64,
    pub phase: String,
    pub kind: String,
    pub thread_id: u32,
    #[serde(default)]
    pub action: Option<u32>,
    #[serde(default)]
    pub value_bits: Option<u32>,
    pub flags: u32,
    pub return_rva: String,
    pub original_calls: u32,
    pub error_code: u32,
    #[serde(default)]
    pub device_key: Option<u32>,
    #[serde(default)]
    pub device_kind: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub install_failure_stage: Option<u32>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub install_failure_index: Option<u32>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub install_failure_win32_error: Option<u32>,
}

#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CheckResult {
    pub id: String,
    pub passed: bool,
    pub detail: String,
}

#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LoopAnalysisReport {
    pub schema_version: u32,
    pub result: String,
    pub evidence_class: String,
    pub probe: String,
    pub scenario: String,
    pub target_sha256: String,
    pub loop_ledger_sha256: String,
    pub manifest_sha256: String,
    pub events_sha256: String,
    pub event_count: usize,
    pub loop_iteration_count: usize,
    pub complete_loop_count: usize,
    pub input_query_count: usize,
    pub message_pump_count: usize,
    pub eligible_for_active_gate: bool,
    pub checks: Vec<CheckResult>,
}

#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LoopGateDecision {
    pub schema_version: u32,
    pub status: String,
    pub active_input_allowed: bool,
    pub matrix_sha256: String,
    pub required_scenarios: Vec<String>,
    pub missing_scenarios: Vec<String>,
    pub analysis_bindings: Vec<LoopAnalysisBinding>,
}

#[derive(Clone, Debug, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LoopAnalysisBinding {
    pub scenario: String,
    pub analysis_path: PathBuf,
    pub analysis_sha256: String,
    pub manifest_sha256: String,
    pub events_sha256: String,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
struct LoopMatrix {
    analyses: Vec<PathBuf>,
}

pub fn inspect_loop(
    game_exe: &Path,
    ledger_path: &Path,
    expected_variant: &str,
) -> AppResult<LoopInspectionReport> {
    inspect_loop_for_observed_target(game_exe, ledger_path, expected_variant, None)
}

pub fn inspect_loop_equivalent(
    game_exe: &Path,
    ledger_path: &Path,
    expected_variant: &str,
    equivalence: &crate::target_equivalence::ValidatedTargetEquivalence,
) -> AppResult<LoopInspectionReport> {
    if equivalence.canonical_target_sha256 != crate::active_catalog::FIXED_TARGET_SHA256
        || !equivalence.passive_attestation_allowed
    {
        return Err(AppError::evidence(
            "target equivalence is not bound to the passive canonical target",
        ));
    }
    inspect_loop_for_observed_target(
        game_exe,
        ledger_path,
        expected_variant,
        Some(&equivalence.observed_target_sha256),
    )
}

fn inspect_loop_for_observed_target(
    game_exe: &Path,
    ledger_path: &Path,
    expected_variant: &str,
    equivalent_observed_sha256: Option<&str>,
) -> AppResult<LoopInspectionReport> {
    if expected_variant != REQUIRED_VARIANT {
        return Err(AppError::usage(format!(
            "loop inspection is fixed to {REQUIRED_VARIANT}"
        )));
    }
    let raw = fs::read(ledger_path)
        .usage_context(format!("cannot read loop ledger {}", ledger_path.display()))?;
    let ledger: LoopLedger = serde_json::from_slice(&raw).usage_context(format!(
        "cannot parse loop ledger {}",
        ledger_path.display()
    ))?;
    validate_ledger(&ledger)?;
    let image = PeImage::read(game_exe).map_err(|error| AppError::evidence(error.to_string()))?;
    let hashes = image
        .hashes()
        .map_err(|error| AppError::evidence(error.to_string()))?;
    if let Some(observed) = equivalent_observed_sha256 {
        validate_equivalent_image(
            &ledger.fixed_variant,
            &image,
            &hashes.file_sha256,
            &hashes.text_sha256,
            observed,
        )?;
    } else {
        validate_image(
            &ledger.fixed_variant,
            &image,
            &hashes.file_sha256,
            &hashes.text_sha256,
        )?;
    }

    let mut ids = BTreeSet::new();
    let mut static_checks = Vec::new();
    let entries = ledger
        .corrections
        .iter()
        .chain(&ledger.targets)
        .chain(&ledger.action_oracles);
    for entry in entries {
        let effective_id = entry.effective_id();
        if effective_id.is_empty() || !ids.insert(effective_id) {
            return Err(AppError::usage(format!(
                "duplicate loop evidence id {}",
                effective_id
            )));
        }
        validate_entry_bytes(&image, entry)?;
        static_checks.push(LoopStaticCheck {
            id: effective_id.to_owned(),
            evidence_label: entry.evidence_label.clone(),
            rva: entry.rva.clone(),
            result: "PASS".to_owned(),
        });
    }
    for entry in &ledger.loop_model {
        if entry.target_id.is_empty() || !ids.contains(entry.target_id.as_str()) {
            return Err(AppError::evidence(format!(
                "loop model phase {} references missing target {}",
                entry.phase, entry.target_id
            )));
        }
        if entry.phase.is_empty() || entry.evidence_label.is_empty() {
            return Err(AppError::usage("loop model phase is incomplete"));
        }
        static_checks.push(LoopStaticCheck {
            id: entry.phase.clone(),
            evidence_label: entry.evidence_label.clone(),
            rva: None,
            result: "PASS".to_owned(),
        });
    }
    if ledger
        .gate
        .required_evidence
        .iter()
        .any(|item| item.trim().is_empty())
    {
        return Err(AppError::usage(
            "loop gate contains an empty evidence requirement",
        ));
    }
    let unresolved_evidence = ledger
        .unresolved
        .iter()
        .filter(|entry| entry.evidence_label != "RUNTIME_OBSERVED")
        .map(|entry| entry.id.clone())
        .collect::<Vec<_>>();
    if ledger.gate.active_input_allowed || ledger.gate.status != "BLOCKED_BY_EVIDENCE" {
        return Err(AppError::evidence(
            "design-only loop ledger must keep active input blocked before passive evidence",
        ));
    }
    Ok(LoopInspectionReport {
        schema_version: LOOP_LEDGER_SCHEMA_VERSION,
        result: "PASS_STATIC".to_owned(),
        ledger_id: ledger.ledger_id,
        ledger_sha256: sha256_hex(&raw),
        variant_id: expected_variant.to_owned(),
        file_sha256: hashes.file_sha256,
        text_sha256: hashes.text_sha256,
        static_checks,
        unresolved_evidence,
        gate_status: ledger.gate.status,
        gate_reason: ledger.gate.reason,
        passive_probe_allowed: true,
        active_input_allowed: false,
    })
}

pub fn analyze_loop_run(run_directory: &Path) -> AppResult<LoopAnalysisReport> {
    let report = analyze_loop_run_internal(run_directory)?;
    let analysis_path = run_directory.join("analysis.json");
    write_new_json(&analysis_path, &report)?;
    Ok(report)
}

pub fn replay_loop_run(run_directory: &Path) -> AppResult<LoopAnalysisReport> {
    analyze_loop_run_internal(run_directory)
}

pub(crate) fn analyze_loop_run_internal(run_directory: &Path) -> AppResult<LoopAnalysisReport> {
    let manifest_path = run_directory.join("manifest.json");
    let events_path = run_directory.join("events.jsonl");
    let target_before_path = run_directory.join("target-bytes-before.json");
    let target_after_path = run_directory.join("target-bytes-after.json");
    let manifest_bytes = fs::read(&manifest_path)
        .usage_context(format!("cannot read {}", manifest_path.display()))?;
    let events_bytes =
        fs::read(&events_path).usage_context(format!("cannot read {}", events_path.display()))?;
    let target_before_bytes = fs::read(&target_before_path)
        .usage_context(format!("cannot read {}", target_before_path.display()))?;
    let target_after_bytes = fs::read(&target_after_path)
        .usage_context(format!("cannot read {}", target_after_path.display()))?;
    let manifest: LoopManifest = serde_json::from_slice(&manifest_bytes)
        .usage_context(format!("cannot parse {}", manifest_path.display()))?;
    if manifest.schema_version != 2
        || manifest.experiment_identity != LOOP_TOOL_IDENTITY
        || manifest.evidence_class != LOOP_EVIDENCE_CLASS
        || manifest.probe != "passive"
        || manifest.state != "COMPLETE"
        || manifest.active_input_allowed
        || !manifest.target_bytes_restored
        || manifest.target_bytes_before_sha256 != manifest.target_bytes_after_sha256
    {
        return Err(AppError::evidence(
            "loop run is not a complete T0-03R artifact",
        ));
    }
    let mut events = Vec::new();
    for (index, line) in events_bytes.split(|byte| *byte == b'\n').enumerate() {
        if line.is_empty() {
            continue;
        }
        let line = std::str::from_utf8(line)
            .usage_context(format!("event line {} is not UTF-8", index + 1))?;
        let event: LoopEvent = serde_json::from_str(line)
            .usage_context(format!("cannot parse event line {}", index + 1))?;
        events.push(event);
    }
    let sequence_ok = events.iter().enumerate().all(|(index, event)| {
        event.sequence == index as u64 + 1 && event.native_sequence == index as u64 + 1
    });
    let mut qpc_by_thread = std::collections::BTreeMap::<u32, i64>::new();
    let qpc_ok = events.iter().all(|event| {
        let previous = qpc_by_thread.entry(event.thread_id).or_default();
        let valid = event.qpc > 0 && event.qpc >= *previous;
        *previous = event.qpc;
        valid
    });
    let iterations = events
        .iter()
        .filter(|event| event.loop_iteration != 0)
        .map(|event| event.loop_iteration)
        .collect::<BTreeSet<_>>();
    let phases_ok = events.iter().all(|event| {
        matches!(
            event.phase.as_str(),
            "OUTSIDE_ENVELOPE"
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

    let known_kinds = events.iter().all(|event| is_known_kind(&event.kind));
    let mut depth_ok = true;
    let mut main_thread_ok = manifest.main_thread_id != 0;
    let mut gameplay_owned = true;
    let mut original_calls_ok = true;
    let mut input_scope_ok = true;
    let mut loop_order_ok = true;
    let mut authority_transaction_order_ok = true;
    let mut previous_loop = 0_u64;
    let mut update_depth = 0_i32;
    let mut gameplay_depth = 0_i32;
    let mut entity_depth = 0_i32;
    let mut remove_depth = 0_i32;
    let mut post_update_depth = 0_i32;
    let mut spawn_depth = 0_i32;
    let mut room_depth = 0_i32;
    let mut run_depth = 0_i32;
    let mut render_depth = 0_i32;
    let mut input_manager_depth = 0_i32;
    let mut input_device_depth = 0_i32;
    let mut physical_poll_depth = 0_i32;
    let mut input_device_identity = None::<(String, u32)>;
    let mut physical_poll_identity = None::<(String, u32)>;
    let mut conditional_depth = 0_i32;
    let mut alternate_owner_depth = 0_i32;
    let mut alternate_outcome_depth = 0_i32;
    let mut alternate_room_depth = 0_i32;
    let mut alternate_state_apply_depth = 0_i32;
    let mut alternate_tail_depth = 0_i32;
    let mut standard_predicate_depth = 0_i32;
    let mut standard_body_depth = 0_i32;
    let mut standard_room_depth = 0_i32;
    let mut standard_tail_depth = 0_i32;
    let mut standard_lifecycle_owner_depth = 0_i32;
    let mut standard_lifecycle_simulation_depth = 0_i32;
    let mut transition_room_owner_depth = 0_i32;
    let mut transition_room_simulation_depth = 0_i32;
    let mut entity_list_depth = 0_i32;
    let mut standard_attempt_loop = 0_u64;
    let mut standard_attempt_order_ok = true;
    let mut standard_progress = 0_u8;
    let mut standard_entity_list_seen = false;
    let mut standard_lifecycle_entity_list_seen = false;
    let mut standard_lifecycle_simulation_complete = false;
    let mut transition_attempt_loop = 0_u64;
    let mut transition_attempt_order_ok = true;
    let mut transition_owner_seen = false;
    let mut transition_progress = 0_u8;
    let mut transition_complete_pending = false;
    let mut alternate_attempt_loop = 0_u64;
    let mut alternate_attempt_order_ok = true;
    let mut alternate_pre_active = false;
    let mut alternate_progress = 0_u8;
    let mut alternate_internal_seen = false;
    let mut alternate_entity_list_seen = false;
    let mut alternate_outcome_complete = false;
    let mut alternate_outcome_return_rva = None::<String>;
    let mut coherent_standard_transaction_count = 0_usize;
    let mut coherent_standard_lifecycle_transaction_count = 0_usize;
    let mut coherent_transition_room_transaction_count = 0_usize;
    let mut coherent_alternate_transaction_count = 0_usize;
    let mut standard_transaction_loops = BTreeSet::new();
    let mut standard_lifecycle_owner_loops = BTreeSet::new();
    let mut standard_lifecycle_transaction_loops = BTreeSet::new();
    let mut transition_room_owner_loops = BTreeSet::new();
    let mut transition_room_transaction_loops = BTreeSet::new();
    let mut alternate_simulation_transaction_loops = BTreeSet::new();
    let mut loop_anchors = std::collections::BTreeMap::<u64, [Option<u64>; 6]>::new();
    let mut input_query_count = 0_usize;
    let mut message_pump_count = 0_usize;
    let mut main_peek_candidate = false;
    let mut input_manager_count = 0_usize;
    let mut input_device_count = 0_usize;
    let mut physical_poll_count = 0_usize;
    let event_contract_ok = events
        .iter()
        .all(|event| event_contract_valid(event, manifest.main_thread_id));
    let (device_switch_refresh_sequences, coherent_device_switch_refresh_count) =
        device_switch_direct_refresh_sequences(&events, manifest.main_thread_id);
    let mut observed_direct_device_switch_refresh_enters = 0_usize;

    for event in &events {
        original_calls_ok &= expected_original_calls(&event.kind) == Some(event.original_calls);
        let main_required = matches!(
            event.kind.as_str(),
            "LOOP_BEGIN"
                | "LOOP_CLOCK_EXIT"
                | "UPDATE_ENTER"
                | "UPDATE_EXIT"
                | "GAMEPLAY_ENTER"
                | "GAMEPLAY_EXIT"
                | "ENTITY_ENTER"
                | "ENTITY_EXIT"
                | "REMOVE_ENTER"
                | "REMOVE_EXIT"
                | "POST_UPDATE_ENTER"
                | "POST_UPDATE_EXIT"
                | "SPAWN_ENTER"
                | "SPAWN_EXIT"
                | "NEW_ROOM_ENTER"
                | "NEW_ROOM_EXIT"
                | "RUN_TEARDOWN_ENTER"
                | "RUN_TEARDOWN_EXIT"
                | "RENDER_ENTER"
                | "RENDER_EXIT"
                | "INPUT_PRESSED"
                | "INPUT_TRIGGERED"
                | "INPUT_SCALAR"
                | "INPUT_MANAGER_SAMPLE_ENTER"
                | "INPUT_MANAGER_SAMPLE_EXIT"
                | "INPUT_DEVICE_UPDATE_ENTER"
                | "INPUT_DEVICE_UPDATE_EXIT"
                | "INPUT_PHYSICAL_POLL_ENTER"
                | "INPUT_PHYSICAL_POLL_EXIT"
                | "CONDITIONAL_ATTEMPT_ENTER"
                | "CONDITIONAL_ATTEMPT_EXIT"
                | "ALTERNATE_OWNER_ENTER"
                | "ALTERNATE_OWNER_EXIT"
                | "ALTERNATE_ROOM_ENTER"
                | "ALTERNATE_ROOM_EXIT"
                | "ALTERNATE_TAIL_ENTER"
                | "ALTERNATE_TAIL_EXIT"
                | "STANDARD_PREDICATE_ENTER"
                | "STANDARD_PREDICATE_EXIT"
                | "STANDARD_BODY_ENTER"
                | "STANDARD_BODY_EXIT"
                | "STANDARD_ROOM_ENTER"
                | "STANDARD_ROOM_EXIT"
                | "STANDARD_TAIL_ENTER"
                | "STANDARD_TAIL_EXIT"
                | "ENTITY_LIST_ENTER"
                | "ENTITY_LIST_EXIT"
                | "ALTERNATE_STATE_APPLY_ENTER"
                | "ALTERNATE_STATE_APPLY_EXIT"
                | "ALTERNATE_OUTCOME_ENTER"
                | "ALTERNATE_OUTCOME_EXIT"
                | "STANDARD_ROOM_LIFECYCLE_OWNER_ENTER"
                | "STANDARD_ROOM_LIFECYCLE_OWNER_EXIT"
                | "STANDARD_ROOM_LIFECYCLE_SIMULATION_ENTER"
                | "STANDARD_ROOM_LIFECYCLE_SIMULATION_EXIT"
                | "ROOM_CHANGE_OWNER_ENTER"
                | "ROOM_CHANGE_OWNER_EXIT"
                | "ROOM_CHANGE_SIMULATION_ENTER"
                | "ROOM_CHANGE_SIMULATION_EXIT"
        );
        if main_required {
            main_thread_ok &= event.thread_id == manifest.main_thread_id;
            gameplay_owned &= event.loop_iteration != 0;
        }
        if event.kind == "LOOP_BEGIN" {
            loop_order_ok &= event.loop_iteration != 0
                && (previous_loop == 0 || event.loop_iteration == previous_loop + 1);
            previous_loop = event.loop_iteration;
        }
        let anchor_index = match event.kind.as_str() {
            "LOOP_BEGIN" => Some(0),
            "LOOP_CLOCK_EXIT" => Some(1),
            "UPDATE_ENTER" => Some(2),
            "UPDATE_EXIT" => Some(3),
            "RENDER_ENTER" => Some(4),
            "RENDER_EXIT" => Some(5),
            _ => None,
        };
        if let Some(index) = anchor_index
            && event.loop_iteration != 0
        {
            let slot = &mut loop_anchors.entry(event.loop_iteration).or_default()[index];
            loop_order_ok &= slot.is_none();
            *slot = Some(event.sequence);
        }

        match event.kind.as_str() {
            "UPDATE_ENTER" => {
                depth_ok &= update_depth == 0;
                update_depth += 1;
            }
            "UPDATE_EXIT" => {
                depth_ok &= update_depth == 1
                    && gameplay_depth == 0
                    && entity_depth == 0
                    && remove_depth == 0
                    && post_update_depth == 0
                    && spawn_depth == 0
                    && input_manager_depth == 0
                    && input_device_depth == 0
                    && physical_poll_depth == 0
                    && conditional_depth == 0
                    && alternate_owner_depth == 0
                    && alternate_outcome_depth == 0
                    && alternate_state_apply_depth == 0
                    && standard_predicate_depth == 0
                    && standard_body_depth == 0
                    && standard_room_depth == 0
                    && standard_tail_depth == 0
                    && standard_lifecycle_owner_depth == 0
                    && standard_lifecycle_simulation_depth == 0
                    && transition_room_owner_depth == 0
                    && transition_room_simulation_depth == 0
                    && entity_list_depth == 0;
                update_depth -= 1;
            }
            "GAMEPLAY_ENTER" => {
                gameplay_owned &= update_depth > 0;
                gameplay_depth += 1;
            }
            "GAMEPLAY_EXIT" => {
                gameplay_owned &= update_depth > 0;
                depth_ok &= gameplay_depth > 0;
                gameplay_depth -= 1;
            }
            "ENTITY_ENTER" => {
                gameplay_owned &= update_depth > 0 || room_depth > 0 || run_depth > 0;
                entity_depth += 1;
            }
            "ENTITY_EXIT" => {
                gameplay_owned &= update_depth > 0 || room_depth > 0 || run_depth > 0;
                depth_ok &= entity_depth > 0;
                entity_depth -= 1;
            }
            "REMOVE_ENTER" => {
                gameplay_owned &= update_depth > 0 || room_depth > 0 || run_depth > 0;
                remove_depth += 1;
            }
            "REMOVE_EXIT" => {
                gameplay_owned &= update_depth > 0 || room_depth > 0 || run_depth > 0;
                depth_ok &= remove_depth > 0;
                remove_depth -= 1;
            }
            "POST_UPDATE_ENTER" => {
                gameplay_owned &= update_depth > 0;
                post_update_depth += 1;
            }
            "POST_UPDATE_EXIT" => {
                gameplay_owned &= update_depth > 0;
                depth_ok &= post_update_depth > 0;
                post_update_depth -= 1;
            }
            "SPAWN_ENTER" => {
                gameplay_owned &= update_depth > 0 || room_depth > 0 || run_depth > 0;
                spawn_depth += 1;
            }
            "SPAWN_EXIT" => {
                gameplay_owned &= update_depth > 0 || room_depth > 0 || run_depth > 0;
                depth_ok &= spawn_depth > 0;
                spawn_depth -= 1;
            }
            "NEW_ROOM_ENTER" => {
                depth_ok &= room_depth == 0;
                if transition_room_owner_depth > 0 {
                    let ordered = event.loop_iteration == transition_attempt_loop
                        && conditional_depth > 0
                        && transition_room_owner_depth == 1
                        && transition_room_simulation_depth == 0
                        && transition_progress == 0;
                    authority_transaction_order_ok &= ordered;
                    transition_attempt_order_ok &= ordered;
                    transition_progress = 1;
                }
                room_depth += 1;
            }
            "NEW_ROOM_EXIT" => {
                depth_ok &=
                    room_depth == 1 && entity_depth == 0 && remove_depth == 0 && spawn_depth == 0;
                if transition_room_owner_depth > 0 {
                    let ordered = event.loop_iteration == transition_attempt_loop
                        && conditional_depth > 0
                        && transition_room_owner_depth == 1
                        && transition_room_simulation_depth == 0
                        && transition_progress == 1;
                    authority_transaction_order_ok &= ordered;
                    transition_attempt_order_ok &= ordered;
                    transition_progress = 2;
                }
                room_depth -= 1;
            }
            "RUN_TEARDOWN_ENTER" => {
                depth_ok &= run_depth == 0;
                run_depth += 1;
            }
            "RUN_TEARDOWN_EXIT" => {
                depth_ok &=
                    run_depth == 1 && entity_depth == 0 && remove_depth == 0 && spawn_depth == 0;
                run_depth -= 1;
            }
            "RENDER_ENTER" => {
                depth_ok &= render_depth == 0 && update_depth == 0;
                render_depth += 1;
            }
            "RENDER_EXIT" => {
                depth_ok &= render_depth == 1;
                render_depth -= 1;
            }
            "INPUT_MANAGER_SAMPLE_ENTER" => {
                gameplay_owned &= update_depth > 0;
                depth_ok &= input_manager_depth == 0;
                input_manager_count += 1;
                input_query_count += 1;
                input_manager_depth += 1;
            }
            "INPUT_MANAGER_SAMPLE_EXIT" => {
                gameplay_owned &= update_depth > 0;
                depth_ok &=
                    input_manager_depth == 1 && input_device_depth == 0 && physical_poll_depth == 0;
                input_scope_ok &=
                    input_device_identity.is_none() && physical_poll_identity.is_none();
                input_manager_depth -= 1;
            }
            "INPUT_DEVICE_UPDATE_ENTER" => {
                gameplay_owned &= update_depth > 0;
                let direct_device_switch_refresh = manifest.scenario == "device-switch"
                    && input_manager_depth == 0
                    && device_switch_refresh_sequences.contains(&event.sequence);
                depth_ok &= (input_manager_depth > 0 || direct_device_switch_refresh)
                    && input_device_depth == 0;
                if direct_device_switch_refresh {
                    observed_direct_device_switch_refresh_enters += 1;
                }
                input_device_count += 1;
                input_query_count += 1;
                input_device_depth += 1;
                input_scope_ok &=
                    matches!(event.device_kind.as_deref(), Some("KEYBOARD" | "GAMEPAD"))
                        && event.device_key.is_some();
                input_device_identity = event.device_kind.clone().zip(event.device_key);
            }
            "INPUT_DEVICE_UPDATE_EXIT" => {
                gameplay_owned &= update_depth > 0;
                depth_ok &= input_device_depth == 1 && physical_poll_depth == 0;
                input_scope_ok &= event.device_kind.clone().zip(event.device_key)
                    == input_device_identity
                    && physical_poll_identity.is_none();
                input_device_identity = None;
                input_device_depth -= 1;
            }
            "INPUT_PHYSICAL_POLL_ENTER" => {
                gameplay_owned &= update_depth > 0;
                depth_ok &= input_device_depth > 0 && physical_poll_depth == 0;
                physical_poll_count += 1;
                input_query_count += 1;
                physical_poll_depth += 1;
                input_scope_ok &=
                    matches!(event.device_kind.as_deref(), Some("KEYBOARD" | "GAMEPAD"))
                        && event.device_key.is_some();
                physical_poll_identity = event.device_kind.clone().zip(event.device_key);
                input_scope_ok &= physical_poll_identity == input_device_identity;
            }
            "INPUT_PHYSICAL_POLL_EXIT" => {
                gameplay_owned &= update_depth > 0;
                depth_ok &= physical_poll_depth == 1;
                input_scope_ok &=
                    event.device_kind.clone().zip(event.device_key) == physical_poll_identity;
                physical_poll_identity = None;
                physical_poll_depth -= 1;
            }
            "CONDITIONAL_ATTEMPT_ENTER" => {
                gameplay_owned &= update_depth > 0;
                depth_ok &= conditional_depth == 0;
                conditional_depth += 1;
                standard_attempt_loop = event.loop_iteration;
                standard_attempt_order_ok = true;
                standard_progress = 0;
                standard_entity_list_seen = false;
                standard_lifecycle_entity_list_seen = false;
                standard_lifecycle_simulation_complete = false;
                transition_attempt_loop = event.loop_iteration;
                transition_attempt_order_ok = true;
                transition_owner_seen = false;
                transition_progress = 0;
                transition_complete_pending = false;
            }
            "CONDITIONAL_ATTEMPT_EXIT" => {
                gameplay_owned &= update_depth > 0;
                depth_ok &= conditional_depth == 1
                    && standard_predicate_depth == 0
                    && standard_body_depth == 0
                    && standard_room_depth == 0
                    && standard_tail_depth == 0
                    && standard_lifecycle_owner_depth == 0
                    && standard_lifecycle_simulation_depth == 0
                    && transition_room_owner_depth == 0
                    && transition_room_simulation_depth == 0;
                let ordered = event.loop_iteration == standard_attempt_loop;
                authority_transaction_order_ok &= ordered;
                standard_attempt_order_ok &= ordered;
                let transition_ordered = event.loop_iteration == transition_attempt_loop
                    && transition_room_owner_depth == 0
                    && transition_room_simulation_depth == 0;
                authority_transaction_order_ok &= transition_ordered;
                transition_attempt_order_ok &= transition_ordered;
                if standard_attempt_order_ok && standard_progress == 4 && standard_entity_list_seen
                {
                    coherent_standard_transaction_count += 1;
                    standard_transaction_loops.insert(event.loop_iteration);
                }
                if standard_attempt_order_ok
                    && standard_progress == 5
                    && standard_lifecycle_simulation_complete
                    && standard_lifecycle_entity_list_seen
                {
                    coherent_standard_lifecycle_transaction_count += 1;
                    standard_lifecycle_transaction_loops.insert(event.loop_iteration);
                }
                if transition_attempt_order_ok && transition_complete_pending {
                    coherent_transition_room_transaction_count += 1;
                    transition_room_transaction_loops.insert(event.loop_iteration);
                }
                conditional_depth -= 1;
            }
            "ALTERNATE_OWNER_ENTER" => {
                gameplay_owned &= update_depth > 0;
                depth_ok &= alternate_owner_depth == 0;
                alternate_owner_depth += 1;
                alternate_attempt_loop = event.loop_iteration;
                alternate_attempt_order_ok = true;
                alternate_pre_active = event.value_bits == Some(1);
                let ordered = matches!(event.value_bits, Some(0 | 1));
                authority_transaction_order_ok &= ordered;
                alternate_attempt_order_ok &= ordered;
                alternate_progress = 0;
                alternate_internal_seen = false;
                alternate_entity_list_seen = false;
                alternate_outcome_complete = false;
                alternate_outcome_return_rva = None;
            }
            "ALTERNATE_OWNER_EXIT" => {
                gameplay_owned &= update_depth > 0;
                depth_ok &= alternate_owner_depth == 1
                    && alternate_room_depth == 0
                    && alternate_tail_depth == 0
                    && alternate_state_apply_depth == 0
                    && alternate_outcome_depth == 0;
                let ordered = event.loop_iteration == alternate_attempt_loop
                    && matches!(event.value_bits, Some(0 | 1));
                authority_transaction_order_ok &= ordered;
                alternate_attempt_order_ok &= ordered;
                let complete_simulation = alternate_attempt_order_ok
                    && alternate_pre_active
                    && event.value_bits == Some(0)
                    && alternate_progress == 3
                    && alternate_entity_list_seen
                    && !alternate_outcome_complete;
                let complete_outcome = alternate_attempt_order_ok
                    && alternate_pre_active
                    && event.value_bits == Some(0)
                    && alternate_progress == 0
                    && !alternate_entity_list_seen
                    && alternate_outcome_complete;
                if complete_simulation {
                    coherent_alternate_transaction_count += 1;
                    alternate_simulation_transaction_loops.insert(event.loop_iteration);
                } else if !complete_outcome
                    && (alternate_pre_active
                        || alternate_internal_seen
                        || event.value_bits != Some(0))
                {
                    authority_transaction_order_ok = false;
                }
                alternate_owner_depth -= 1;
            }
            "ALTERNATE_STATE_APPLY_ENTER" => {
                gameplay_owned &= update_depth > 0 && alternate_owner_depth > 0;
                depth_ok &= alternate_state_apply_depth == 0;
                let ordered = event.loop_iteration == alternate_attempt_loop
                    && alternate_pre_active
                    && !alternate_outcome_complete
                    && alternate_progress == 0;
                authority_transaction_order_ok &= ordered;
                alternate_attempt_order_ok &= ordered;
                alternate_internal_seen = true;
                alternate_state_apply_depth += 1;
            }
            "ALTERNATE_STATE_APPLY_EXIT" => {
                depth_ok &= alternate_state_apply_depth == 1;
                let ordered =
                    event.loop_iteration == alternate_attempt_loop && alternate_progress == 0;
                authority_transaction_order_ok &= ordered;
                alternate_attempt_order_ok &= ordered;
                alternate_progress = 1;
                alternate_state_apply_depth -= 1;
            }
            "ALTERNATE_OUTCOME_ENTER" => {
                gameplay_owned &= update_depth > 0 && alternate_owner_depth > 0;
                depth_ok &= alternate_outcome_depth == 0;
                let ordered = event.loop_iteration == alternate_attempt_loop
                    && alternate_pre_active
                    && !alternate_internal_seen
                    && alternate_progress == 0;
                authority_transaction_order_ok &= ordered;
                alternate_attempt_order_ok &= ordered;
                alternate_internal_seen = true;
                alternate_outcome_return_rva = Some(event.return_rva.clone());
                alternate_outcome_depth += 1;
            }
            "ALTERNATE_OUTCOME_EXIT" => {
                depth_ok &= alternate_outcome_depth == 1;
                let ordered = event.loop_iteration == alternate_attempt_loop
                    && alternate_progress == 0
                    && alternate_outcome_return_rva.as_deref() == Some(event.return_rva.as_str());
                authority_transaction_order_ok &= ordered;
                alternate_attempt_order_ok &= ordered;
                alternate_outcome_complete = true;
                alternate_outcome_depth -= 1;
            }
            "ALTERNATE_ROOM_ENTER" => {
                gameplay_owned &= update_depth > 0 && alternate_owner_depth > 0;
                depth_ok &= alternate_room_depth == 0;
                let ordered =
                    event.loop_iteration == alternate_attempt_loop && alternate_progress == 1;
                authority_transaction_order_ok &= ordered;
                alternate_attempt_order_ok &= ordered;
                alternate_internal_seen = true;
                alternate_room_depth += 1;
            }
            "ALTERNATE_ROOM_EXIT" => {
                depth_ok &= alternate_room_depth == 1 && entity_list_depth == 0;
                let ordered =
                    event.loop_iteration == alternate_attempt_loop && alternate_progress == 1;
                authority_transaction_order_ok &= ordered;
                alternate_attempt_order_ok &= ordered;
                alternate_progress = 2;
                alternate_room_depth -= 1;
            }
            "ALTERNATE_TAIL_ENTER" => {
                gameplay_owned &= update_depth > 0 && alternate_owner_depth > 0;
                depth_ok &= alternate_tail_depth == 0;
                let ordered =
                    event.loop_iteration == alternate_attempt_loop && alternate_progress == 2;
                authority_transaction_order_ok &= ordered;
                alternate_attempt_order_ok &= ordered;
                alternate_internal_seen = true;
                alternate_tail_depth += 1;
            }
            "ALTERNATE_TAIL_EXIT" => {
                depth_ok &= alternate_tail_depth == 1;
                let ordered =
                    event.loop_iteration == alternate_attempt_loop && alternate_progress == 2;
                authority_transaction_order_ok &= ordered;
                alternate_attempt_order_ok &= ordered;
                alternate_progress = 3;
                alternate_tail_depth -= 1;
            }
            "STANDARD_PREDICATE_ENTER" => {
                gameplay_owned &= update_depth > 0 && conditional_depth > 0;
                depth_ok &= standard_predicate_depth == 0;
                let ordered =
                    event.loop_iteration == standard_attempt_loop && standard_progress == 0;
                authority_transaction_order_ok &= ordered;
                standard_attempt_order_ok &= ordered;
                standard_predicate_depth += 1;
            }
            "STANDARD_PREDICATE_EXIT" => {
                depth_ok &= standard_predicate_depth == 1;
                let ordered =
                    event.loop_iteration == standard_attempt_loop && standard_progress == 0;
                authority_transaction_order_ok &= ordered;
                standard_attempt_order_ok &= ordered;
                standard_progress = if event.flags & LOOP_FLAG_RESULT_TRUE == 0 {
                    1
                } else {
                    u8::MAX
                };
                standard_predicate_depth -= 1;
            }
            "STANDARD_BODY_ENTER" => {
                gameplay_owned &= update_depth > 0 && conditional_depth > 0;
                depth_ok &= standard_body_depth == 0;
                let ordered =
                    event.loop_iteration == standard_attempt_loop && standard_progress == 1;
                authority_transaction_order_ok &= ordered;
                standard_attempt_order_ok &= ordered;
                standard_body_depth += 1;
            }
            "STANDARD_BODY_EXIT" => {
                depth_ok &= standard_body_depth == 1;
                let ordered =
                    event.loop_iteration == standard_attempt_loop && standard_progress == 1;
                authority_transaction_order_ok &= ordered;
                standard_attempt_order_ok &= ordered;
                standard_progress = 2;
                standard_body_depth -= 1;
            }
            "STANDARD_ROOM_LIFECYCLE_OWNER_ENTER" => {
                gameplay_owned &= update_depth > 0 && conditional_depth > 0;
                depth_ok &=
                    standard_lifecycle_owner_depth == 0 && standard_lifecycle_simulation_depth == 0;
                let ordered = event.loop_iteration == standard_attempt_loop
                    && standard_progress == 2
                    && !standard_lifecycle_simulation_complete
                    && !standard_lifecycle_entity_list_seen;
                authority_transaction_order_ok &= ordered;
                standard_attempt_order_ok &= ordered;
                standard_lifecycle_owner_loops.insert(event.loop_iteration);
                standard_lifecycle_owner_depth += 1;
            }
            "STANDARD_ROOM_LIFECYCLE_OWNER_EXIT" => {
                depth_ok &= standard_lifecycle_owner_depth == 1
                    && standard_lifecycle_simulation_depth == 0
                    && entity_list_depth == 0;
                let ordered = event.loop_iteration == standard_attempt_loop
                    && standard_progress == 2
                    && (!standard_lifecycle_entity_list_seen
                        || standard_lifecycle_simulation_complete);
                authority_transaction_order_ok &= ordered;
                standard_attempt_order_ok &= ordered;
                standard_progress = 5;
                standard_lifecycle_owner_depth -= 1;
            }
            "STANDARD_ROOM_LIFECYCLE_SIMULATION_ENTER" => {
                gameplay_owned &=
                    update_depth > 0 && conditional_depth > 0 && standard_lifecycle_owner_depth > 0;
                depth_ok &= standard_lifecycle_simulation_depth == 0;
                let ordered = event.loop_iteration == standard_attempt_loop
                    && standard_progress == 2
                    && !standard_lifecycle_simulation_complete
                    && !standard_lifecycle_entity_list_seen;
                authority_transaction_order_ok &= ordered;
                standard_attempt_order_ok &= ordered;
                standard_lifecycle_simulation_depth += 1;
            }
            "STANDARD_ROOM_LIFECYCLE_SIMULATION_EXIT" => {
                depth_ok &= standard_lifecycle_simulation_depth == 1 && entity_list_depth == 0;
                let ordered = event.loop_iteration == standard_attempt_loop
                    && standard_progress == 2
                    && standard_lifecycle_owner_depth == 1
                    && standard_lifecycle_entity_list_seen
                    && !standard_lifecycle_simulation_complete;
                authority_transaction_order_ok &= ordered;
                standard_attempt_order_ok &= ordered;
                standard_lifecycle_simulation_complete = true;
                standard_lifecycle_simulation_depth -= 1;
            }
            "ROOM_CHANGE_OWNER_ENTER" => {
                gameplay_owned &= update_depth > 0 && conditional_depth > 0;
                depth_ok &=
                    transition_room_owner_depth == 0 && transition_room_simulation_depth == 0;
                let ordered = event.loop_iteration == transition_attempt_loop
                    && event.loop_iteration == standard_attempt_loop
                    && standard_progress == 2
                    && !transition_owner_seen
                    && !transition_complete_pending
                    && transition_progress == 0;
                authority_transaction_order_ok &= ordered;
                transition_attempt_order_ok &= ordered;
                transition_owner_seen = true;
                transition_room_owner_loops.insert(event.loop_iteration);
                transition_room_owner_depth += 1;
            }
            "ROOM_CHANGE_OWNER_EXIT" => {
                gameplay_owned &= update_depth > 0 && conditional_depth > 0;
                depth_ok &= transition_room_owner_depth == 1
                    && transition_room_simulation_depth == 0
                    && room_depth == 0
                    && entity_list_depth == 0;
                let owner_only = transition_progress == 0;
                let complete = transition_progress == 6;
                let ordered = event.loop_iteration == transition_attempt_loop
                    && event.loop_iteration == standard_attempt_loop
                    && standard_progress == 2
                    && transition_owner_seen
                    && !transition_complete_pending
                    && (owner_only || complete);
                authority_transaction_order_ok &= ordered;
                transition_attempt_order_ok &= ordered;
                transition_complete_pending = ordered && complete;
                transition_room_owner_depth -= 1;
                transition_progress = 0;
            }
            "ROOM_CHANGE_SIMULATION_ENTER" => {
                gameplay_owned &=
                    update_depth > 0 && conditional_depth > 0 && transition_room_owner_depth > 0;
                depth_ok &= transition_room_simulation_depth == 0;
                let ordered = event.loop_iteration == transition_attempt_loop
                    && event.loop_iteration == standard_attempt_loop
                    && transition_room_owner_depth == 1
                    && transition_progress == 2;
                authority_transaction_order_ok &= ordered;
                transition_attempt_order_ok &= ordered;
                transition_progress = 3;
                transition_room_simulation_depth += 1;
            }
            "ROOM_CHANGE_SIMULATION_EXIT" => {
                depth_ok &= transition_room_simulation_depth == 1 && entity_list_depth == 0;
                let ordered = event.loop_iteration == transition_attempt_loop
                    && event.loop_iteration == standard_attempt_loop
                    && transition_room_owner_depth == 1
                    && transition_progress == 5;
                authority_transaction_order_ok &= ordered;
                transition_attempt_order_ok &= ordered;
                transition_progress = 6;
                transition_room_simulation_depth -= 1;
            }
            "STANDARD_ROOM_ENTER" => {
                gameplay_owned &= update_depth > 0 && conditional_depth > 0;
                depth_ok &= standard_room_depth == 0;
                let ordered =
                    event.loop_iteration == standard_attempt_loop && standard_progress == 2;
                authority_transaction_order_ok &= ordered;
                standard_attempt_order_ok &= ordered;
                standard_room_depth += 1;
            }
            "STANDARD_ROOM_EXIT" => {
                depth_ok &= standard_room_depth == 1 && entity_list_depth == 0;
                let ordered =
                    event.loop_iteration == standard_attempt_loop && standard_progress == 2;
                authority_transaction_order_ok &= ordered;
                standard_attempt_order_ok &= ordered;
                standard_progress = 3;
                standard_room_depth -= 1;
            }
            "STANDARD_TAIL_ENTER" => {
                gameplay_owned &= update_depth > 0 && conditional_depth > 0;
                depth_ok &= standard_tail_depth == 0;
                let ordered =
                    event.loop_iteration == standard_attempt_loop && standard_progress == 3;
                authority_transaction_order_ok &= ordered;
                standard_attempt_order_ok &= ordered;
                standard_tail_depth += 1;
            }
            "STANDARD_TAIL_EXIT" => {
                depth_ok &= standard_tail_depth == 1;
                let ordered =
                    event.loop_iteration == standard_attempt_loop && standard_progress == 3;
                authority_transaction_order_ok &= ordered;
                standard_attempt_order_ok &= ordered;
                standard_progress = 4;
                standard_tail_depth -= 1;
            }
            "ENTITY_LIST_ENTER" => {
                gameplay_owned &= update_depth > 0
                    && (standard_room_depth > 0
                        || alternate_room_depth > 0
                        || standard_lifecycle_simulation_depth > 0
                        || transition_room_simulation_depth > 0);
                let entity_list_owner_count = u8::from(standard_room_depth > 0)
                    + u8::from(alternate_room_depth > 0)
                    + u8::from(standard_lifecycle_simulation_depth > 0)
                    + u8::from(transition_room_simulation_depth > 0);
                depth_ok &= entity_list_depth == 0 && entity_list_owner_count == 1;
                if standard_room_depth > 0 {
                    let ordered = event.loop_iteration == standard_attempt_loop;
                    authority_transaction_order_ok &= ordered;
                    standard_attempt_order_ok &= ordered;
                    standard_entity_list_seen = true;
                }
                if alternate_room_depth > 0 {
                    let ordered = event.loop_iteration == alternate_attempt_loop;
                    authority_transaction_order_ok &= ordered;
                    alternate_attempt_order_ok &= ordered;
                    alternate_entity_list_seen = true;
                }
                if standard_lifecycle_simulation_depth > 0 {
                    let ordered = event.loop_iteration == standard_attempt_loop
                        && standard_lifecycle_owner_depth == 1
                        && standard_progress == 2
                        && !standard_lifecycle_entity_list_seen;
                    authority_transaction_order_ok &= ordered;
                    standard_attempt_order_ok &= ordered;
                    standard_lifecycle_entity_list_seen = true;
                }
                if transition_room_simulation_depth > 0 {
                    let ordered = event.loop_iteration == transition_attempt_loop
                        && event.loop_iteration == standard_attempt_loop
                        && transition_room_owner_depth == 1
                        && transition_progress == 3;
                    authority_transaction_order_ok &= ordered;
                    transition_attempt_order_ok &= ordered;
                    transition_progress = 4;
                }
                entity_list_depth += 1;
            }
            "ENTITY_LIST_EXIT" => {
                depth_ok &= entity_list_depth == 1;
                if standard_room_depth > 0 {
                    let ordered = event.loop_iteration == standard_attempt_loop;
                    authority_transaction_order_ok &= ordered;
                    standard_attempt_order_ok &= ordered;
                }
                if alternate_room_depth > 0 {
                    let ordered = event.loop_iteration == alternate_attempt_loop;
                    authority_transaction_order_ok &= ordered;
                    alternate_attempt_order_ok &= ordered;
                }
                if standard_lifecycle_simulation_depth > 0 {
                    let ordered = event.loop_iteration == standard_attempt_loop
                        && standard_lifecycle_owner_depth == 1
                        && standard_progress == 2
                        && standard_lifecycle_entity_list_seen;
                    authority_transaction_order_ok &= ordered;
                    standard_attempt_order_ok &= ordered;
                }
                if transition_room_simulation_depth > 0 {
                    let ordered = event.loop_iteration == transition_attempt_loop
                        && event.loop_iteration == standard_attempt_loop
                        && transition_room_owner_depth == 1
                        && transition_progress == 4;
                    authority_transaction_order_ok &= ordered;
                    transition_attempt_order_ok &= ordered;
                    transition_progress = 5;
                }
                entity_list_depth -= 1;
            }
            "INPUT_PRESSED" | "INPUT_TRIGGERED" | "INPUT_SCALAR" => {
                input_query_count += 1;
                input_scope_ok &= event.action.is_some_and(|action| action <= 11)
                    && matches!(event.device_kind.as_deref(), Some("KEYBOARD" | "GAMEPAD"))
                    && event.device_key.is_some();
                if event.kind == "INPUT_SCALAR" {
                    input_scope_ok &= event.value_bits.is_some_and(|bits| {
                        let value = f32::from_bits(bits);
                        value.is_finite() && (0.0..=1.0).contains(&value)
                    });
                }
            }
            "PEEK_MESSAGE" | "TRANSLATE_MESSAGE" | "DISPATCH_MESSAGE" => {
                message_pump_count += 1;
                if event.kind == "PEEK_MESSAGE"
                    && event.thread_id == manifest.main_thread_id
                    && main_peek_return_site(&event.return_rva)
                {
                    main_peek_candidate = true;
                }
            }
            _ => {
                input_scope_ok &= event.action.is_none()
                    && event.device_kind.is_none()
                    && event.device_key.is_none();
            }
        }
    }
    depth_ok &= [
        update_depth,
        gameplay_depth,
        entity_depth,
        remove_depth,
        post_update_depth,
        spawn_depth,
        room_depth,
        run_depth,
        render_depth,
        input_manager_depth,
        input_device_depth,
        physical_poll_depth,
        conditional_depth,
        alternate_owner_depth,
        alternate_outcome_depth,
        alternate_room_depth,
        alternate_state_apply_depth,
        alternate_tail_depth,
        standard_predicate_depth,
        standard_body_depth,
        standard_room_depth,
        standard_tail_depth,
        standard_lifecycle_owner_depth,
        standard_lifecycle_simulation_depth,
        transition_room_owner_depth,
        transition_room_simulation_depth,
        entity_list_depth,
    ]
    .iter()
    .all(|depth| *depth == 0);

    let complete_loops = loop_anchors
        .values()
        .filter(|anchors| anchors.iter().all(Option::is_some))
        .filter(|anchors| {
            anchors
                .iter()
                .map(|position| position.unwrap_or_default())
                .is_sorted()
        })
        .count();
    loop_order_ok &= loop_anchors.values().all(|anchors| {
        let positions = anchors.iter().flatten().copied().collect::<Vec<_>>();
        positions.is_sorted()
    });
    let lifecycle_ok = events
        .iter()
        .filter(|event| event.kind == "NATIVE_INITIALIZED")
        .count()
        == 1
        && events
            .iter()
            .filter(|event| event.kind == "HOOKS_INSTALLED")
            .count()
            == 1
        && events
            .iter()
            .filter(|event| event.kind == "HOOKS_RESTORED")
            .count()
            == 1
        && !events.iter().any(|event| event.kind == "FAILURE")
        && manifest.hooks_mask == crate::loop_ipc::LOOP_HOOK_MASK
        && manifest.overwritten_events == 0
        && events
            .first()
            .is_some_and(|event| event.kind == "NATIVE_INITIALIZED")
        && events
            .get(1)
            .is_some_and(|event| event.kind == "HOOKS_INSTALLED")
        && events
            .last()
            .is_some_and(|event| event.kind == "HOOKS_RESTORED");
    authority_transaction_order_ok &= standard_transaction_loops
        .is_disjoint(&alternate_simulation_transaction_loops)
        && standard_lifecycle_owner_loops.is_disjoint(&standard_transaction_loops)
        && standard_lifecycle_owner_loops.is_disjoint(&alternate_simulation_transaction_loops)
        && standard_lifecycle_transaction_loops.is_disjoint(&standard_transaction_loops)
        && standard_lifecycle_transaction_loops
            .is_disjoint(&alternate_simulation_transaction_loops)
        && transition_room_owner_loops.is_disjoint(&standard_transaction_loops)
        && transition_room_owner_loops.is_disjoint(&alternate_simulation_transaction_loops)
        && transition_room_owner_loops.is_disjoint(&standard_lifecycle_owner_loops)
        && transition_room_transaction_loops.is_disjoint(&standard_transaction_loops)
        && transition_room_transaction_loops.is_disjoint(&alternate_simulation_transaction_loops)
        && transition_room_transaction_loops.is_disjoint(&standard_lifecycle_transaction_loops);
    let authority_evidence_nonempty = coherent_standard_transaction_count > 0
        || coherent_standard_lifecycle_transaction_count > 0
        || coherent_transition_room_transaction_count > 0
        || coherent_alternate_transaction_count > 0;
    let physical_sample_evidence =
        input_manager_count > 0 && input_device_count > 0 && physical_poll_count > 0;
    let restoration_ok = target_before_bytes == target_after_bytes
        && manifest.target_bytes_before_sha256 == sha256_hex(&target_before_bytes)
        && manifest.target_bytes_after_sha256 == sha256_hex(&target_after_bytes);
    let scenario_ok = scenario_evidence(&manifest.scenario, &events, &standard_transaction_loops)
        && (manifest.scenario != "room" || coherent_transition_room_transaction_count > 0)
        && (manifest.scenario != "device-switch"
            || (coherent_device_switch_refresh_count > 0
                && observed_direct_device_switch_refresh_enters
                    == coherent_device_switch_refresh_count * 2));
    let checks = vec![
        check("non-empty", !events.is_empty(), "native loop events exist"),
        check(
            "sequence-contiguous",
            sequence_ok,
            "artifact and native sequences start at one and have no gaps",
        ),
        check(
            "qpc-monotonic",
            qpc_ok,
            "QPC values are positive and nondecreasing within each producer thread",
        ),
        check(
            "schema-known",
            phases_ok && known_kinds,
            "every event kind and phase is defined",
        ),
        check(
            "event-contract",
            event_contract_ok,
            "kind, phase, flags, payload, return site, and thread attribution agree",
        ),
        check(
            "authority-owned",
            gameplay_owned,
            "Authority mutations belong to an update or explicit lifecycle transaction",
        ),
        check(
            "authority-evidence-nonempty",
            authority_evidence_nonempty,
            "a coherent steady-standard, room-lifecycle, room-change, or alternate Authority transaction was observed",
        ),
        check(
            "authority-transaction-order",
            authority_transaction_order_ok,
            "Authority evidence is ordered within one outer attempt and no loop has conflicting transaction paths",
        ),
        check(
            "physical-sample-envelope",
            physical_sample_evidence,
            "InputManager sample, per-device update, and physical poll were all observed",
        ),
        check("balanced-depth", depth_ok, "enter/exit scopes are balanced"),
        check(
            "main-thread-owned",
            main_thread_ok,
            "loop, input, update, mutation, lifecycle, and render use the main thread",
        ),
        check(
            "loop-order",
            loop_order_ok && complete_loops >= 30,
            "at least thirty complete ordered loop iterations were observed",
        ),
        check(
            "ui-partition",
            input_scope_ok,
            "only gameplay actions 0..11 have device-attributed input-query payloads",
        ),
        check(
            "message-pump-owner",
            message_pump_count > 0 && main_peek_candidate,
            "a statically catalogued PeekMessageW candidate ran on the loop main thread",
        ),
        check(
            "original-once",
            original_calls_ok,
            "every passive wrapper records the expected original-call count",
        ),
        check(
            "native-lifecycle",
            lifecycle_ok,
            "hooks installed and restored once with no failure or ring overwrite",
        ),
        check(
            "target-restored",
            restoration_ok,
            "all patched call, IAT, and vtable bytes exactly match their pre-capture values",
        ),
        check(
            "scenario-evidence",
            scenario_ok,
            "the event stream contains the minimum evidence for its declared scenario",
        ),
    ];
    let passed = checks.iter().all(|check| check.passed);
    let report = LoopAnalysisReport {
        schema_version: 2,
        result: if passed { "PASS" } else { "FAIL" }.to_owned(),
        evidence_class: manifest.evidence_class,
        probe: manifest.probe,
        scenario: manifest.scenario,
        target_sha256: manifest.target_sha256,
        loop_ledger_sha256: manifest.loop_ledger_sha256,
        manifest_sha256: sha256_hex(&manifest_bytes),
        events_sha256: sha256_hex(&events_bytes),
        event_count: events.len(),
        loop_iteration_count: iterations.len(),
        complete_loop_count: complete_loops,
        input_query_count,
        message_pump_count,
        eligible_for_active_gate: passed,
        checks,
    };
    Ok(report)
}

pub fn write_loop_gate(matrix_path: &Path, output: &Path) -> AppResult<LoopGateDecision> {
    if matrix_path.parent().unwrap_or_else(|| Path::new("."))
        != output.parent().unwrap_or_else(|| Path::new("."))
    {
        return Err(AppError::usage(
            "loop matrix and gate output must share a directory so evidence bindings remain replayable",
        ));
    }
    let raw = fs::read(matrix_path)
        .usage_context(format!("cannot read loop matrix {}", matrix_path.display()))?;
    let matrix: LoopMatrix = serde_json::from_slice(&raw).usage_context(format!(
        "cannot parse loop matrix {}",
        matrix_path.display()
    ))?;
    let required = REQUIRED_LOOP_GATE_SCENARIOS;
    let parent = matrix_path.parent().unwrap_or_else(|| Path::new("."));
    let mut passed = BTreeSet::new();
    let mut bindings = Vec::new();
    let mut target_sha256 = None::<String>;
    let mut loop_ledger_sha256 = None::<String>;
    for relative in &matrix.analyses {
        validate_relative_artifact_path(relative)?;
        let path = parent.join(relative);
        let bytes = fs::read(&path)
            .usage_context(format!("cannot read loop analysis {}", path.display()))?;
        let stored: LoopAnalysisReport = serde_json::from_slice(&bytes)
            .usage_context(format!("cannot parse loop analysis {}", path.display()))?;
        let run_directory = path.parent().ok_or_else(|| {
            AppError::usage(format!(
                "loop analysis {} has no run directory",
                path.display()
            ))
        })?;
        let replayed = analyze_loop_run_internal(run_directory)?;
        if stored != replayed {
            return Err(AppError::evidence(format!(
                "loop analysis {} does not match replayed source artifacts",
                path.display()
            )));
        }
        if replayed.result != "PASS"
            || !replayed.eligible_for_active_gate
            || replayed.evidence_class != LOOP_EVIDENCE_CLASS
            || replayed.probe != "passive"
            || !replayed.checks.iter().all(|check| check.passed)
        {
            return Err(AppError::evidence(format!(
                "loop analysis {} is not PASS",
                path.display()
            )));
        }
        if !required.contains(&replayed.scenario.as_str())
            || !passed.insert(replayed.scenario.clone())
        {
            return Err(AppError::evidence(format!(
                "loop matrix has an unknown or duplicate scenario {}",
                replayed.scenario
            )));
        }
        if target_sha256.get_or_insert_with(|| replayed.target_sha256.clone())
            != &replayed.target_sha256
            || loop_ledger_sha256.get_or_insert_with(|| replayed.loop_ledger_sha256.clone())
                != &replayed.loop_ledger_sha256
        {
            return Err(AppError::evidence(
                "loop matrix mixes target or ledger identities",
            ));
        }
        bindings.push(LoopAnalysisBinding {
            scenario: replayed.scenario,
            analysis_path: relative.clone(),
            analysis_sha256: sha256_hex(&bytes),
            manifest_sha256: replayed.manifest_sha256,
            events_sha256: replayed.events_sha256,
        });
    }
    let missing = required
        .iter()
        .filter(|scenario| !passed.contains(**scenario))
        .map(|scenario| (*scenario).to_owned())
        .collect::<Vec<_>>();
    let decision = LoopGateDecision {
        schema_version: 2,
        status: if missing.is_empty() {
            "PASS"
        } else {
            "BLOCKED_BY_EVIDENCE"
        }
        .to_owned(),
        active_input_allowed: missing.is_empty(),
        matrix_sha256: sha256_hex(&raw),
        required_scenarios: required.iter().map(|value| (*value).to_owned()).collect(),
        missing_scenarios: missing,
        analysis_bindings: bindings,
    };
    write_new_json(output, &decision)?;
    Ok(decision)
}

fn is_known_kind(kind: &str) -> bool {
    matches!(
        kind,
        "NATIVE_INITIALIZED"
            | "HOOKS_INSTALLED"
            | "LOOP_BEGIN"
            | "LOOP_CLOCK_EXIT"
            | "UPDATE_ENTER"
            | "UPDATE_EXIT"
            | "GAMEPLAY_ENTER"
            | "GAMEPLAY_EXIT"
            | "ENTITY_ENTER"
            | "ENTITY_EXIT"
            | "REMOVE_ENTER"
            | "REMOVE_EXIT"
            | "POST_UPDATE_ENTER"
            | "POST_UPDATE_EXIT"
            | "SPAWN_ENTER"
            | "SPAWN_EXIT"
            | "NEW_ROOM_ENTER"
            | "NEW_ROOM_EXIT"
            | "RUN_TEARDOWN_ENTER"
            | "RUN_TEARDOWN_EXIT"
            | "RENDER_ENTER"
            | "RENDER_EXIT"
            | "PEEK_MESSAGE"
            | "TRANSLATE_MESSAGE"
            | "DISPATCH_MESSAGE"
            | "INPUT_PRESSED"
            | "INPUT_TRIGGERED"
            | "INPUT_SCALAR"
            | "HOOKS_RESTORED"
            | "FAILURE"
            | "INPUT_MANAGER_SAMPLE_ENTER"
            | "INPUT_MANAGER_SAMPLE_EXIT"
            | "INPUT_DEVICE_UPDATE_ENTER"
            | "INPUT_DEVICE_UPDATE_EXIT"
            | "INPUT_PHYSICAL_POLL_ENTER"
            | "INPUT_PHYSICAL_POLL_EXIT"
            | "CONDITIONAL_ATTEMPT_ENTER"
            | "CONDITIONAL_ATTEMPT_EXIT"
            | "ALTERNATE_OWNER_ENTER"
            | "ALTERNATE_OWNER_EXIT"
            | "ALTERNATE_ROOM_ENTER"
            | "ALTERNATE_ROOM_EXIT"
            | "ALTERNATE_TAIL_ENTER"
            | "ALTERNATE_TAIL_EXIT"
            | "STANDARD_PREDICATE_ENTER"
            | "STANDARD_PREDICATE_EXIT"
            | "STANDARD_BODY_ENTER"
            | "STANDARD_BODY_EXIT"
            | "STANDARD_ROOM_ENTER"
            | "STANDARD_ROOM_EXIT"
            | "STANDARD_TAIL_ENTER"
            | "STANDARD_TAIL_EXIT"
            | "ENTITY_LIST_ENTER"
            | "ENTITY_LIST_EXIT"
            | "ALTERNATE_STATE_APPLY_ENTER"
            | "ALTERNATE_STATE_APPLY_EXIT"
            | "ALTERNATE_OUTCOME_ENTER"
            | "ALTERNATE_OUTCOME_EXIT"
            | "STANDARD_ROOM_LIFECYCLE_OWNER_ENTER"
            | "STANDARD_ROOM_LIFECYCLE_OWNER_EXIT"
            | "STANDARD_ROOM_LIFECYCLE_SIMULATION_ENTER"
            | "STANDARD_ROOM_LIFECYCLE_SIMULATION_EXIT"
            | "ROOM_CHANGE_OWNER_ENTER"
            | "ROOM_CHANGE_OWNER_EXIT"
            | "ROOM_CHANGE_SIMULATION_ENTER"
            | "ROOM_CHANGE_SIMULATION_EXIT"
    )
}

fn expected_original_calls(kind: &str) -> Option<u32> {
    match kind {
        "NATIVE_INITIALIZED"
        | "HOOKS_INSTALLED"
        | "LOOP_BEGIN"
        | "UPDATE_ENTER"
        | "GAMEPLAY_ENTER"
        | "ENTITY_ENTER"
        | "REMOVE_ENTER"
        | "POST_UPDATE_ENTER"
        | "SPAWN_ENTER"
        | "NEW_ROOM_ENTER"
        | "RUN_TEARDOWN_ENTER"
        | "RENDER_ENTER"
        | "HOOKS_RESTORED"
        | "FAILURE"
        | "INPUT_MANAGER_SAMPLE_ENTER"
        | "INPUT_DEVICE_UPDATE_ENTER"
        | "INPUT_PHYSICAL_POLL_ENTER"
        | "CONDITIONAL_ATTEMPT_ENTER"
        | "ALTERNATE_OWNER_ENTER"
        | "ALTERNATE_ROOM_ENTER"
        | "ALTERNATE_TAIL_ENTER"
        | "STANDARD_PREDICATE_ENTER"
        | "STANDARD_BODY_ENTER"
        | "STANDARD_ROOM_ENTER"
        | "STANDARD_TAIL_ENTER"
        | "ENTITY_LIST_ENTER"
        | "ALTERNATE_STATE_APPLY_ENTER"
        | "STANDARD_ROOM_LIFECYCLE_OWNER_ENTER"
        | "STANDARD_ROOM_LIFECYCLE_SIMULATION_ENTER"
        | "ROOM_CHANGE_OWNER_ENTER"
        | "ROOM_CHANGE_SIMULATION_ENTER" => Some(0),
        "ALTERNATE_OUTCOME_ENTER" => Some(0),
        "LOOP_CLOCK_EXIT"
        | "UPDATE_EXIT"
        | "GAMEPLAY_EXIT"
        | "ENTITY_EXIT"
        | "REMOVE_EXIT"
        | "POST_UPDATE_EXIT"
        | "SPAWN_EXIT"
        | "NEW_ROOM_EXIT"
        | "RUN_TEARDOWN_EXIT"
        | "RENDER_EXIT"
        | "PEEK_MESSAGE"
        | "TRANSLATE_MESSAGE"
        | "DISPATCH_MESSAGE"
        | "INPUT_PRESSED"
        | "INPUT_TRIGGERED"
        | "INPUT_SCALAR"
        | "INPUT_MANAGER_SAMPLE_EXIT"
        | "INPUT_DEVICE_UPDATE_EXIT"
        | "INPUT_PHYSICAL_POLL_EXIT"
        | "CONDITIONAL_ATTEMPT_EXIT"
        | "ALTERNATE_OWNER_EXIT"
        | "ALTERNATE_ROOM_EXIT"
        | "ALTERNATE_TAIL_EXIT"
        | "STANDARD_PREDICATE_EXIT"
        | "STANDARD_BODY_EXIT"
        | "STANDARD_ROOM_EXIT"
        | "STANDARD_TAIL_EXIT"
        | "ENTITY_LIST_EXIT"
        | "ALTERNATE_STATE_APPLY_EXIT"
        | "STANDARD_ROOM_LIFECYCLE_OWNER_EXIT"
        | "STANDARD_ROOM_LIFECYCLE_SIMULATION_EXIT"
        | "ROOM_CHANGE_OWNER_EXIT"
        | "ROOM_CHANGE_SIMULATION_EXIT" => Some(1),
        "ALTERNATE_OUTCOME_EXIT" => Some(1),
        _ => None,
    }
}

fn expected_phase(kind: &str) -> Option<&'static str> {
    Some(match kind {
        "NATIVE_INITIALIZED" | "HOOKS_INSTALLED" | "HOOKS_RESTORED" | "FAILURE" => {
            "OUTSIDE_ENVELOPE"
        }
        "LOOP_BEGIN" | "LOOP_CLOCK_EXIT" => "LOOP_BOUNDARY",
        "UPDATE_ENTER"
        | "UPDATE_EXIT"
        | "GAMEPLAY_ENTER"
        | "GAMEPLAY_EXIT"
        | "CONDITIONAL_ATTEMPT_ENTER"
        | "CONDITIONAL_ATTEMPT_EXIT"
        | "ALTERNATE_OWNER_ENTER"
        | "ALTERNATE_OWNER_EXIT"
        | "STANDARD_PREDICATE_ENTER"
        | "STANDARD_PREDICATE_EXIT"
        | "STANDARD_BODY_ENTER"
        | "STANDARD_BODY_EXIT" => "GAMEPLAY_UPDATE",
        "ENTITY_ENTER"
        | "ENTITY_EXIT"
        | "REMOVE_ENTER"
        | "REMOVE_EXIT"
        | "SPAWN_ENTER"
        | "SPAWN_EXIT"
        | "ALTERNATE_ROOM_ENTER"
        | "ALTERNATE_ROOM_EXIT"
        | "STANDARD_ROOM_ENTER"
        | "STANDARD_ROOM_EXIT"
        | "ENTITY_LIST_ENTER"
        | "ENTITY_LIST_EXIT"
        | "ALTERNATE_STATE_APPLY_ENTER"
        | "ALTERNATE_STATE_APPLY_EXIT" => "ENTITY_MUTATION",
        "POST_UPDATE_ENTER" | "POST_UPDATE_EXIT" | "STANDARD_TAIL_ENTER" | "STANDARD_TAIL_EXIT" => {
            "POST_UPDATE"
        }
        "NEW_ROOM_ENTER"
        | "NEW_ROOM_EXIT"
        | "STANDARD_ROOM_LIFECYCLE_OWNER_ENTER"
        | "STANDARD_ROOM_LIFECYCLE_OWNER_EXIT"
        | "STANDARD_ROOM_LIFECYCLE_SIMULATION_ENTER"
        | "STANDARD_ROOM_LIFECYCLE_SIMULATION_EXIT"
        | "ROOM_CHANGE_OWNER_ENTER"
        | "ROOM_CHANGE_OWNER_EXIT"
        | "ROOM_CHANGE_SIMULATION_ENTER"
        | "ROOM_CHANGE_SIMULATION_EXIT" => "ROOM_LIFECYCLE",
        "RUN_TEARDOWN_ENTER"
        | "RUN_TEARDOWN_EXIT"
        | "ALTERNATE_TAIL_ENTER"
        | "ALTERNATE_TAIL_EXIT"
        | "ALTERNATE_OUTCOME_ENTER"
        | "ALTERNATE_OUTCOME_EXIT" => "RUN_LIFECYCLE",
        "RENDER_ENTER" | "RENDER_EXIT" => "RENDER",
        "PEEK_MESSAGE" | "TRANSLATE_MESSAGE" | "DISPATCH_MESSAGE" => "MESSAGE_PUMP",
        "INPUT_PRESSED"
        | "INPUT_TRIGGERED"
        | "INPUT_SCALAR"
        | "INPUT_MANAGER_SAMPLE_ENTER"
        | "INPUT_MANAGER_SAMPLE_EXIT"
        | "INPUT_DEVICE_UPDATE_ENTER"
        | "INPUT_DEVICE_UPDATE_EXIT"
        | "INPUT_PHYSICAL_POLL_ENTER"
        | "INPUT_PHYSICAL_POLL_EXIT" => "INPUT_QUERY",
        _ => return None,
    })
}

fn valid_return_rva(value: &str) -> bool {
    value.len() == 10
        && value.starts_with("0x")
        && value[2..].bytes().all(|byte| byte.is_ascii_hexdigit())
        && u32::from_str_radix(&value[2..], 16)
            .is_ok_and(|rva| (0x0000_1000..0x0071_7134).contains(&rva))
}

fn main_peek_return_site(value: &str) -> bool {
    matches!(
        value,
        "0x0065C107" | "0x00681494" | "0x006814C9" | "0x0065E68C"
    )
}

fn device_switch_direct_refresh_sequences(
    events: &[LoopEvent],
    main_thread_id: u32,
) -> (BTreeSet<u64>, usize) {
    const DIRECT_KINDS: [&str; 12] = [
        "PEEK_MESSAGE",
        "TRANSLATE_MESSAGE",
        "DISPATCH_MESSAGE",
        "PEEK_MESSAGE",
        "INPUT_DEVICE_UPDATE_ENTER",
        "INPUT_PHYSICAL_POLL_ENTER",
        "INPUT_PHYSICAL_POLL_EXIT",
        "INPUT_DEVICE_UPDATE_EXIT",
        "INPUT_DEVICE_UPDATE_ENTER",
        "INPUT_PHYSICAL_POLL_ENTER",
        "INPUT_PHYSICAL_POLL_EXIT",
        "INPUT_DEVICE_UPDATE_EXIT",
    ];
    const DIRECT_RETURN_RVAS: [&str; 12] = [
        "0x0065E68C",
        "0x0065E6D6",
        "0x0065E6E1",
        "0x0065E6F0",
        "0x0061FC9C",
        "0x0061F30C",
        "0x0061F30C",
        "0x0061FC9C",
        "0x0061FC9C",
        "0x0061F30C",
        "0x0061F30C",
        "0x0061FC9C",
    ];
    const POST_MANAGER_KINDS: [&str; 23] = [
        "PEEK_MESSAGE",
        "TRANSLATE_MESSAGE",
        "DISPATCH_MESSAGE",
        "PEEK_MESSAGE",
        "INPUT_MANAGER_SAMPLE_ENTER",
        "INPUT_DEVICE_UPDATE_ENTER",
        "INPUT_PHYSICAL_POLL_ENTER",
        "INPUT_PHYSICAL_POLL_EXIT",
        "INPUT_DEVICE_UPDATE_EXIT",
        "INPUT_DEVICE_UPDATE_ENTER",
        "INPUT_PHYSICAL_POLL_ENTER",
        "INPUT_PHYSICAL_POLL_EXIT",
        "INPUT_DEVICE_UPDATE_EXIT",
        "INPUT_MANAGER_SAMPLE_EXIT",
        "PEEK_MESSAGE",
        "INPUT_DEVICE_UPDATE_ENTER",
        "INPUT_PHYSICAL_POLL_ENTER",
        "INPUT_PHYSICAL_POLL_EXIT",
        "INPUT_DEVICE_UPDATE_EXIT",
        "INPUT_DEVICE_UPDATE_ENTER",
        "INPUT_PHYSICAL_POLL_ENTER",
        "INPUT_PHYSICAL_POLL_EXIT",
        "INPUT_DEVICE_UPDATE_EXIT",
    ];
    const POST_MANAGER_RETURN_RVAS: [&str; 23] = [
        "0x0065E68C",
        "0x0065E6D6",
        "0x0065E6E1",
        "0x0065E6F0",
        "0x00554F82",
        "0x0061FC9C",
        "0x0061F30C",
        "0x0061F30C",
        "0x0061FC9C",
        "0x0061FC9C",
        "0x0061F30C",
        "0x0061F30C",
        "0x0061FC9C",
        "0x00554F82",
        "0x0065E68C",
        "0x0061FC9C",
        "0x0061F30C",
        "0x0061F30C",
        "0x0061FC9C",
        "0x0061FC9C",
        "0x0061F30C",
        "0x0061F30C",
        "0x0061FC9C",
    ];

    let mut sequences = BTreeSet::new();
    let mut coherent_count = 0_usize;
    for window in events.windows(DIRECT_KINDS.len()) {
        let loop_iteration = window[0].loop_iteration;
        if loop_iteration == 0
            || !window.iter().all(|event| {
                event.loop_iteration == loop_iteration && event.thread_id == main_thread_id
            })
            || !window
                .iter()
                .zip(DIRECT_KINDS)
                .all(|(event, kind)| event.kind == kind)
            || !window
                .iter()
                .zip(DIRECT_RETURN_RVAS)
                .all(|(event, return_rva)| event.return_rva == return_rva)
        {
            continue;
        }

        if !device_refresh_group_coherent(window, 4, "KEYBOARD")
            || !device_refresh_group_coherent(window, 8, "GAMEPAD")
        {
            continue;
        }

        sequences.extend(window[4..].iter().map(|event| event.sequence));
        coherent_count += 1;
    }

    for window in events.windows(POST_MANAGER_KINDS.len()) {
        let loop_iteration = window[0].loop_iteration;
        if loop_iteration == 0
            || !window.iter().all(|event| {
                event.loop_iteration == loop_iteration && event.thread_id == main_thread_id
            })
            || !window
                .iter()
                .zip(POST_MANAGER_KINDS)
                .all(|(event, kind)| event.kind == kind)
            || !window
                .iter()
                .zip(POST_MANAGER_RETURN_RVAS)
                .all(|(event, return_rva)| event.return_rva == return_rva)
            || !device_refresh_group_coherent(window, 5, "KEYBOARD")
            || !device_refresh_group_coherent(window, 9, "GAMEPAD")
            || !device_refresh_group_coherent(window, 15, "KEYBOARD")
            || !device_refresh_group_coherent(window, 19, "GAMEPAD")
            || window[5].device_kind.as_deref().zip(window[5].device_key)
                != window[15].device_kind.as_deref().zip(window[15].device_key)
            || window[9].device_kind.as_deref().zip(window[9].device_key)
                != window[19].device_kind.as_deref().zip(window[19].device_key)
        {
            continue;
        }

        sequences.extend(window[15..].iter().map(|event| event.sequence));
        coherent_count += 1;
    }
    (sequences, coherent_count)
}

fn device_refresh_group_coherent(window: &[LoopEvent], start: usize, expected_kind: &str) -> bool {
    let identity = window[start]
        .device_kind
        .as_deref()
        .zip(window[start].device_key);
    identity.is_some_and(|identity| {
        identity.0 == expected_kind
            && window[start..start + 4]
                .iter()
                .all(|event| event.device_kind.as_deref().zip(event.device_key) == Some(identity))
    })
}

fn return_site_valid(kind: &str, value: &str) -> bool {
    match kind {
        "NATIVE_INITIALIZED" | "HOOKS_INSTALLED" | "HOOKS_RESTORED" | "FAILURE" => {
            value == "0x00000000"
        }
        "LOOP_BEGIN" | "LOOP_CLOCK_EXIT" => value == "0x00531236",
        "UPDATE_ENTER" | "UPDATE_EXIT" => value == "0x005312F1",
        "GAMEPLAY_ENTER" | "GAMEPLAY_EXIT" => value == "0x00555236",
        "ENTITY_ENTER" | "ENTITY_EXIT" => value == "0x002FB521",
        "REMOVE_ENTER" | "REMOVE_EXIT" => value == "0x002FBB6B",
        "POST_UPDATE_ENTER" | "POST_UPDATE_EXIT" => value == "0x002FBBAA",
        "SPAWN_ENTER" | "SPAWN_EXIT" => value == "0x00028B42",
        "NEW_ROOM_ENTER" | "NEW_ROOM_EXIT" => value == "0x0033F5BE",
        "RUN_TEARDOWN_ENTER" | "RUN_TEARDOWN_EXIT" => {
            matches!(value, "0x0055969B" | "0x005AB84F")
        }
        "RENDER_ENTER" | "RENDER_EXIT" => value == "0x005312F6",
        "PEEK_MESSAGE" | "TRANSLATE_MESSAGE" | "DISPATCH_MESSAGE" | "INPUT_PRESSED"
        | "INPUT_TRIGGERED" | "INPUT_SCALAR" => valid_return_rva(value),
        "INPUT_MANAGER_SAMPLE_ENTER" | "INPUT_MANAGER_SAMPLE_EXIT" => value == "0x00554F82",
        "INPUT_DEVICE_UPDATE_ENTER" | "INPUT_DEVICE_UPDATE_EXIT" => value == "0x0061FC9C",
        "INPUT_PHYSICAL_POLL_ENTER" | "INPUT_PHYSICAL_POLL_EXIT" => value == "0x0061F30C",
        "CONDITIONAL_ATTEMPT_ENTER" | "CONDITIONAL_ATTEMPT_EXIT" => value == "0x00555236",
        "ALTERNATE_OWNER_ENTER" | "ALTERNATE_OWNER_EXIT" => value == "0x00554D63",
        "ALTERNATE_ROOM_ENTER" | "ALTERNATE_ROOM_EXIT" => value == "0x00559B52",
        "ALTERNATE_TAIL_ENTER" | "ALTERNATE_TAIL_EXIT" => value == "0x00559B87",
        "STANDARD_PREDICATE_ENTER" | "STANDARD_PREDICATE_EXIT" => value == "0x002FADFF",
        "STANDARD_BODY_ENTER" | "STANDARD_BODY_EXIT" => value == "0x002FAE0E",
        "STANDARD_ROOM_ENTER" | "STANDARD_ROOM_EXIT" => value == "0x002FBA52",
        "STANDARD_TAIL_ENTER" | "STANDARD_TAIL_EXIT" => value == "0x002FBBAA",
        "ENTITY_LIST_ENTER" | "ENTITY_LIST_EXIT" => value == "0x00403E75",
        "ALTERNATE_STATE_APPLY_ENTER" | "ALTERNATE_STATE_APPLY_EXIT" => value == "0x00559B26",
        "ALTERNATE_OUTCOME_ENTER" | "ALTERNATE_OUTCOME_EXIT" => matches!(
            value,
            "0x002F5810" | "0x002F6204" | "0x002F72CE" | "0x002F7726" | "0x002F7A15"
        ),
        "STANDARD_ROOM_LIFECYCLE_OWNER_ENTER" | "STANDARD_ROOM_LIFECYCLE_OWNER_EXIT" => {
            value == "0x002FB4F5"
        }
        "STANDARD_ROOM_LIFECYCLE_SIMULATION_ENTER" | "STANDARD_ROOM_LIFECYCLE_SIMULATION_EXIT" => {
            value == "0x0052FC7C"
        }
        "ROOM_CHANGE_OWNER_ENTER" | "ROOM_CHANGE_OWNER_EXIT" => value == "0x002FB521",
        "ROOM_CHANGE_SIMULATION_ENTER" | "ROOM_CHANGE_SIMULATION_EXIT" => value == "0x002FE381",
        _ => false,
    }
}

fn event_contract_valid(event: &LoopEvent, main_thread_id: u32) -> bool {
    const KNOWN_FLAGS: u32 = LOOP_FLAG_LOOP_KNOWN
        | LOOP_FLAG_MAIN_THREAD
        | LOOP_FLAG_WRONG_THREAD
        | LOOP_FLAG_ORIGINAL_CALLED
        | LOOP_FLAG_RESULT_TRUE
        | LOOP_FLAG_ACTION_GAMEPLAY
        | LOOP_FLAG_KEYBOARD_DEVICE
        | LOOP_FLAG_GAMEPAD_DEVICE;
    if event.thread_id == 0
        || event.flags & !KNOWN_FLAGS != 0
        || expected_phase(&event.kind) != Some(event.phase.as_str())
        || expected_original_calls(&event.kind) != Some(event.original_calls)
        || !return_site_valid(&event.kind, &event.return_rva)
    {
        return false;
    }

    let loop_known = event.flags & LOOP_FLAG_LOOP_KNOWN != 0;
    let main = event.flags & LOOP_FLAG_MAIN_THREAD != 0;
    let wrong = event.flags & LOOP_FLAG_WRONG_THREAD != 0;
    if loop_known != (event.loop_iteration != 0) || main && wrong {
        return false;
    }
    if event.loop_iteration == 0 {
        if main || wrong {
            return false;
        }
    } else if event.thread_id == main_thread_id {
        if !main || wrong {
            return false;
        }
    } else if main || !wrong {
        return false;
    }

    let original = event.flags & LOOP_FLAG_ORIGINAL_CALLED != 0;
    if original != (event.original_calls == 1) {
        return false;
    }
    let action_input = matches!(
        event.kind.as_str(),
        "INPUT_PRESSED" | "INPUT_TRIGGERED" | "INPUT_SCALAR"
    );
    let device_event = action_input
        || matches!(
            event.kind.as_str(),
            "INPUT_DEVICE_UPDATE_ENTER"
                | "INPUT_DEVICE_UPDATE_EXIT"
                | "INPUT_PHYSICAL_POLL_ENTER"
                | "INPUT_PHYSICAL_POLL_EXIT"
        );
    if (event.flags & LOOP_FLAG_ACTION_GAMEPLAY != 0) != action_input {
        return false;
    }
    let device_flags = event.flags & (LOOP_FLAG_KEYBOARD_DEVICE | LOOP_FLAG_GAMEPAD_DEVICE);
    if action_input {
        if event.action.is_none_or(|action| action > 11) || event.device_key.is_none() {
            return false;
        }
        match event.device_kind.as_deref() {
            Some("KEYBOARD") if device_flags == LOOP_FLAG_KEYBOARD_DEVICE => {}
            Some("GAMEPAD") if device_flags == LOOP_FLAG_GAMEPAD_DEVICE => {}
            _ => return false,
        }
    } else if device_event {
        if event.action.is_some() {
            return false;
        }
        match event.device_kind.as_deref() {
            Some("KEYBOARD")
                if device_flags == LOOP_FLAG_KEYBOARD_DEVICE && event.device_key.is_some() => {}
            Some("GAMEPAD")
                if device_flags == LOOP_FLAG_GAMEPAD_DEVICE && event.device_key.is_some() => {}
            None if device_flags == 0 && event.device_key.is_none() => {}
            _ => return false,
        }
    } else if event.action.is_some()
        || event.device_key.is_some()
        || event.device_kind.is_some()
        || device_flags != 0
    {
        return false;
    }

    let install_diagnostic = (
        event.install_failure_stage,
        event.install_failure_index,
        event.install_failure_win32_error,
    );
    if event.kind == "FAILURE" {
        if !matches!(install_diagnostic, (None, None, None))
            && !matches!(install_diagnostic, (Some(stage), Some(_), Some(_)) if stage != 0)
        {
            return false;
        }
    } else if !matches!(install_diagnostic, (None, None, None)) {
        return false;
    }

    let value_shape = match event.kind.as_str() {
        "INPUT_SCALAR" => event.value_bits.is_some_and(|bits| {
            let value = f32::from_bits(bits);
            value.is_finite() && (0.0..=1.0).contains(&value)
        }),
        "DISPATCH_MESSAGE" => event.value_bits.is_some(),
        "ALTERNATE_OWNER_ENTER" | "ALTERNATE_OWNER_EXIT" => {
            matches!(event.value_bits, Some(0 | 1))
        }
        _ => event.value_bits.is_none(),
    };
    let result_flag_allowed = matches!(
        event.kind.as_str(),
        "INPUT_PRESSED"
            | "INPUT_TRIGGERED"
            | "PEEK_MESSAGE"
            | "TRANSLATE_MESSAGE"
            | "INPUT_DEVICE_UPDATE_EXIT"
            | "INPUT_PHYSICAL_POLL_EXIT"
            | "STANDARD_PREDICATE_EXIT"
    );
    value_shape
        && (event.flags & LOOP_FLAG_RESULT_TRUE == 0 || result_flag_allowed)
        && if event.kind == "FAILURE" {
            event.error_code != 0
        } else {
            event.error_code == 0
        }
}

fn scenario_evidence(
    scenario: &str,
    events: &[LoopEvent],
    standard_transaction_loops: &BTreeSet<u64>,
) -> bool {
    let device_seen = |name: &str| {
        events
            .iter()
            .any(|event| event.device_kind.as_deref() == Some(name))
    };
    let scalar_positive = |event: &LoopEvent, range: std::ops::RangeInclusive<u32>| {
        event.kind == "INPUT_SCALAR"
            && event.action.is_some_and(|action| range.contains(&action))
            && event
                .value_bits
                .is_some_and(|bits| f32::from_bits(bits) > 0.0)
    };
    match scenario {
        "keyboard" => device_seen("KEYBOARD"),
        "controller" => device_seen("GAMEPAD"),
        "simultaneous" => {
            let mut move_loops = BTreeSet::new();
            let mut aim_loops = BTreeSet::new();
            for event in events {
                if scalar_positive(event, 0..=3) {
                    move_loops.insert(event.loop_iteration);
                }
                if scalar_positive(event, 4..=7) {
                    aim_loops.insert(event.loop_iteration);
                }
            }
            move_loops
                .iter()
                .any(|iteration| aim_loops.contains(iteration))
        }
        "rapid-tap" => {
            let mut states = std::collections::BTreeMap::<u64, bool>::new();
            for event in events {
                if event.loop_iteration != 0 && scalar_positive(event, 4..=7) {
                    states.insert(event.loop_iteration, true);
                }
            }
            let Some((&first, _)) = states.first_key_value() else {
                return false;
            };
            let Some((&last, _)) = states.last_key_value() else {
                return false;
            };
            let mut previous = false;
            let mut rises = 0_u32;
            for iteration in first..=last {
                let current = states.get(&iteration).copied().unwrap_or(false);
                if current && !previous {
                    rises += 1;
                }
                previous = current;
            }
            rises >= 2
        }
        "device-switch" => device_seen("KEYBOARD") && device_seen("GAMEPAD"),
        "pause" | "focus" => {
            has_bracketed_standard_transaction_gap(events, standard_transaction_loops)
        }
        "loading" | "room" => {
            events.iter().any(|event| event.kind == "NEW_ROOM_ENTER")
                && events.iter().any(|event| event.kind == "NEW_ROOM_EXIT")
        }
        "menu" => {
            events
                .iter()
                .any(|event| event.kind == "RUN_TEARDOWN_ENTER")
                && events.iter().any(|event| event.kind == "RUN_TEARDOWN_EXIT")
        }
        _ => false,
    }
}

fn has_bracketed_standard_transaction_gap(
    events: &[LoopEvent],
    standard_transaction_loops: &BTreeSet<u64>,
) -> bool {
    let attempt_loops = events
        .iter()
        .filter(|event| event.kind == "CONDITIONAL_ATTEMPT_ENTER")
        .map(|event| event.loop_iteration)
        .collect::<BTreeSet<_>>()
        .into_iter()
        .collect::<Vec<_>>();
    if attempt_loops.len() < 32 {
        return false;
    }
    let physical_poll_loops = events
        .iter()
        .filter(|event| event.kind == "INPUT_PHYSICAL_POLL_ENTER")
        .map(|event| event.loop_iteration)
        .collect::<BTreeSet<_>>();
    let update_loops = events
        .iter()
        .filter(|event| event.kind == "UPDATE_ENTER")
        .map(|event| event.loop_iteration)
        .collect::<BTreeSet<_>>();
    let render_loops = events
        .iter()
        .filter(|event| event.kind == "RENDER_ENTER")
        .map(|event| event.loop_iteration)
        .collect::<BTreeSet<_>>();
    let lifecycle_loops = events
        .iter()
        .filter(|event| {
            matches!(
                event.kind.as_str(),
                "NEW_ROOM_ENTER"
                    | "RUN_TEARDOWN_ENTER"
                    | "ROOM_CHANGE_OWNER_ENTER"
                    | "STANDARD_ROOM_LIFECYCLE_OWNER_ENTER"
                    | "ALTERNATE_STATE_APPLY_ENTER"
                    | "ALTERNATE_ROOM_ENTER"
            )
        })
        .map(|event| event.loop_iteration)
        .collect::<BTreeSet<_>>();

    let mut index = 0_usize;
    while index < attempt_loops.len() {
        if standard_transaction_loops.contains(&attempt_loops[index]) {
            index += 1;
            continue;
        }
        let gap_start_index = index;
        while index < attempt_loops.len()
            && !standard_transaction_loops.contains(&attempt_loops[index])
        {
            index += 1;
        }
        let gap_end_index = index - 1;
        let bracketed = gap_start_index > 0
            && index < attempt_loops.len()
            && standard_transaction_loops.contains(&attempt_loops[gap_start_index - 1])
            && standard_transaction_loops.contains(&attempt_loops[index]);
        if !bracketed || gap_end_index - gap_start_index + 1 < 30 {
            continue;
        }

        let first_loop = attempt_loops[gap_start_index];
        let last_loop = attempt_loops[gap_end_index];
        let passive_envelope_continues = (first_loop..=last_loop).all(|iteration| {
            physical_poll_loops.contains(&iteration)
                && update_loops.contains(&iteration)
                && render_loops.contains(&iteration)
                && !lifecycle_loops.contains(&iteration)
        });
        if passive_envelope_continues {
            return true;
        }
    }
    false
}

fn validate_relative_artifact_path(path: &Path) -> AppResult<()> {
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
        return Err(AppError::usage(
            "loop matrix paths must be normalized relative paths",
        ));
    }
    Ok(())
}

fn validate_ledger(ledger: &LoopLedger) -> AppResult<()> {
    if ledger.schema_version != LOOP_LEDGER_SCHEMA_VERSION
        || ledger.experiment_identity != LOOP_TOOL_IDENTITY
        || ledger.ledger_id.trim().is_empty()
    {
        return Err(AppError::usage("invalid T0-03R loop ledger identity"));
    }
    if !ledger
        .fixed_variant
        .file_name
        .eq_ignore_ascii_case("isaac-ng.exe")
    {
        return Err(AppError::usage("loop ledger target must be isaac-ng.exe"));
    }
    for (value, label) in [
        (&ledger.fixed_variant.file_sha256, "file SHA-256"),
        (&ledger.fixed_variant.text_sha256, ".text SHA-256"),
    ] {
        if value.len() != 64 || !value.bytes().all(|byte| byte.is_ascii_hexdigit()) {
            return Err(AppError::usage(format!("invalid {label}")));
        }
    }
    if ledger.targets.is_empty() || ledger.loop_model.is_empty() || ledger.constraints.is_empty() {
        return Err(AppError::usage("loop ledger is incomplete"));
    }
    Ok(())
}

fn validate_image(
    expected: &FixedVariant,
    image: &PeImage,
    file_sha256: &str,
    text_sha256: &str,
) -> AppResult<()> {
    let machine = parse_hex(&expected.machine, "machine")?;
    let timestamp = parse_hex(&expected.timestamp, "timestamp")?;
    let image_size = parse_hex(&expected.image_size, "image size")?;
    let image_base = parse_hex(&expected.preferred_image_base, "image base")?;
    if u32::from(image.machine) != machine
        || image.timestamp != timestamp
        || image.image_size != image_size
        || image.image_base != image_base
        || !file_sha256.eq_ignore_ascii_case(&expected.file_sha256)
        || !text_sha256.eq_ignore_ascii_case(&expected.text_sha256)
    {
        return Err(AppError::evidence(
            "T0-03R fixed target fingerprint or PE identity mismatch",
        ));
    }
    Ok(())
}

fn validate_equivalent_image(
    expected: &FixedVariant,
    image: &PeImage,
    file_sha256: &str,
    text_sha256: &str,
    observed_sha256: &str,
) -> AppResult<()> {
    let machine = parse_hex(&expected.machine, "machine")?;
    let timestamp = parse_hex(&expected.timestamp, "timestamp")?;
    let image_base = parse_hex(&expected.preferred_image_base, "image base")?;
    if u32::from(image.machine) != machine
        || image.timestamp != timestamp
        || image.image_base != image_base
        || !file_sha256.eq_ignore_ascii_case(observed_sha256)
        || !text_sha256.eq_ignore_ascii_case(&expected.text_sha256)
    {
        return Err(AppError::evidence(
            "T0-03R equivalent target fingerprint or PE identity mismatch",
        ));
    }
    Ok(())
}

fn validate_entry_bytes(image: &PeImage, entry: &EvidenceEntry) -> AppResult<()> {
    let effective_id = entry.effective_id();
    if effective_id.is_empty() || entry.evidence_label.trim().is_empty() {
        return Err(AppError::usage("loop evidence entry is incomplete"));
    }
    if let (Some(rva), Some(expected)) = (&entry.rva, &entry.expected_bytes) {
        let rva = parse_hex(rva, &format!("{} RVA", effective_id))?;
        let expected = decode_hex(expected, &format!("{} expectedBytes", effective_id))?;
        let actual = image
            .bytes_at_rva(rva, expected.len())
            .map_err(|error| AppError::evidence(error.to_string()))?;
        if actual != expected {
            return Err(AppError::evidence(format!(
                "loop evidence {} original bytes mismatch",
                effective_id
            )));
        }
        if let Some(target) = &entry.expected_target_rva {
            let (instruction_size, displacement) = match expected.as_slice() {
                [0xE8, displacement @ ..] if displacement.len() >= 4 => (
                    5_i64,
                    i32::from_le_bytes(displacement[..4].try_into().unwrap()),
                ),
                [0x0F, opcode, displacement @ ..]
                    if (0x80..=0x8F).contains(opcode) && displacement.len() >= 4 =>
                {
                    (
                        6_i64,
                        i32::from_le_bytes(displacement[..4].try_into().unwrap()),
                    )
                }
                _ => {
                    return Err(AppError::usage(format!(
                        "loop evidence {effective_id} target requires a rel32 call or branch"
                    )));
                }
            };
            let actual_target = i64::from(rva) + instruction_size + i64::from(displacement);
            let expected_target = i64::from(parse_hex(target, "expected target RVA")?);
            if actual_target != expected_target {
                return Err(AppError::evidence(format!(
                    "loop evidence {} rel32 target mismatch",
                    effective_id
                )));
            }
        }
    }
    if entry.rvas.len() != entry.expected_bytes_by_rva.len() {
        return Err(AppError::usage(format!(
            "loop evidence {effective_id} callsite byte map is incomplete"
        )));
    }
    for rva in &entry.rvas {
        let expected = entry.expected_bytes_by_rva.get(rva).ok_or_else(|| {
            AppError::usage(format!(
                "loop evidence {effective_id} has no bytes for {rva}"
            ))
        })?;
        let rva_value = parse_hex(rva, &format!("{effective_id} RVA"))?;
        let expected = decode_hex(expected, &format!("{effective_id} expectedBytes"))?;
        let actual = image
            .bytes_at_rva(rva_value, expected.len())
            .map_err(|error| AppError::evidence(error.to_string()))?;
        if actual != expected {
            return Err(AppError::evidence(format!(
                "loop evidence {effective_id} original bytes mismatch at {rva}"
            )));
        }
    }
    Ok(())
}

impl EvidenceEntry {
    fn effective_id(&self) -> &str {
        if !self.id.is_empty() {
            &self.id
        } else if !self.target_id.is_empty() {
            &self.target_id
        } else {
            &self.phase
        }
    }
}

fn parse_hex(value: &str, label: &str) -> AppResult<u32> {
    let digits = value
        .strip_prefix("0x")
        .or_else(|| value.strip_prefix("0X"))
        .ok_or_else(|| AppError::usage(format!("{label} must use 0x hexadecimal form")))?;
    u32::from_str_radix(digits, 16)
        .map_err(|error| AppError::usage(format!("invalid {label}: {error}")))
}

fn decode_hex(value: &str, label: &str) -> AppResult<Vec<u8>> {
    let compact = value
        .chars()
        .filter(|character| !character.is_ascii_whitespace())
        .collect::<String>();
    hex::decode(compact).map_err(|error| AppError::usage(format!("invalid {label}: {error}")))
}

fn check(id: &str, passed: bool, detail: &str) -> CheckResult {
    CheckResult {
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

#[cfg(test)]
mod tests {
    use super::*;

    fn write_keyboard_loop_fixture(root: &Path) -> PathBuf {
        let run = root.join("keyboard-run");
        fs::create_dir(&run).unwrap();
        let target = b"{}\n";
        fs::write(run.join("target-bytes-before.json"), target).unwrap();
        fs::write(run.join("target-bytes-after.json"), target).unwrap();
        let mut events = Vec::new();
        let mut sequence = 1_u64;
        let mut push = |loop_iteration: u64,
                        phase: &str,
                        kind: &str,
                        original_calls: u32,
                        action: Option<u32>,
                        value_bits: Option<u32>,
                        return_rva: &str| {
            let device = action.is_some()
                || matches!(
                    kind,
                    "INPUT_DEVICE_UPDATE_ENTER"
                        | "INPUT_DEVICE_UPDATE_EXIT"
                        | "INPUT_PHYSICAL_POLL_ENTER"
                        | "INPUT_PHYSICAL_POLL_EXIT"
                );
            let mut flags = 0_u32;
            if loop_iteration != 0 {
                flags |= LOOP_FLAG_LOOP_KNOWN | LOOP_FLAG_MAIN_THREAD;
            }
            if original_calls == 1 {
                flags |= LOOP_FLAG_ORIGINAL_CALLED;
            }
            if action.is_some() {
                flags |= LOOP_FLAG_ACTION_GAMEPLAY;
            }
            if device {
                flags |= LOOP_FLAG_KEYBOARD_DEVICE;
            }
            events.push(LoopEvent {
                sequence,
                native_sequence: sequence,
                qpc: i64::try_from(sequence).unwrap(),
                loop_iteration,
                phase: phase.to_owned(),
                kind: kind.to_owned(),
                thread_id: 7,
                action,
                value_bits,
                flags,
                return_rva: return_rva.to_owned(),
                original_calls,
                error_code: 0,
                device_key: device.then_some(0),
                device_kind: device.then_some("KEYBOARD".to_owned()),
                install_failure_stage: None,
                install_failure_index: None,
                install_failure_win32_error: None,
            });
            sequence += 1;
        };
        push(
            0,
            "OUTSIDE_ENVELOPE",
            "NATIVE_INITIALIZED",
            0,
            None,
            None,
            "0x00000000",
        );
        push(
            0,
            "OUTSIDE_ENVELOPE",
            "HOOKS_INSTALLED",
            0,
            None,
            None,
            "0x00000000",
        );
        for iteration in 1..=30 {
            push(
                iteration,
                "LOOP_BOUNDARY",
                "LOOP_BEGIN",
                0,
                None,
                None,
                "0x00531236",
            );
            push(
                iteration,
                "LOOP_BOUNDARY",
                "LOOP_CLOCK_EXIT",
                1,
                None,
                None,
                "0x00531236",
            );
            push(
                iteration,
                "MESSAGE_PUMP",
                "PEEK_MESSAGE",
                1,
                None,
                None,
                "0x00681494",
            );
            push(
                iteration,
                "GAMEPLAY_UPDATE",
                "UPDATE_ENTER",
                0,
                None,
                None,
                "0x005312F1",
            );
            if iteration == 1 {
                for (phase, kind, original_calls, return_rva, _device) in [
                    (
                        "INPUT_QUERY",
                        "INPUT_MANAGER_SAMPLE_ENTER",
                        0,
                        "0x00554F82",
                        false,
                    ),
                    (
                        "INPUT_QUERY",
                        "INPUT_DEVICE_UPDATE_ENTER",
                        0,
                        "0x0061FC9C",
                        true,
                    ),
                    (
                        "INPUT_QUERY",
                        "INPUT_PHYSICAL_POLL_ENTER",
                        0,
                        "0x0061F30C",
                        true,
                    ),
                    (
                        "INPUT_QUERY",
                        "INPUT_PHYSICAL_POLL_EXIT",
                        1,
                        "0x0061F30C",
                        true,
                    ),
                    (
                        "INPUT_QUERY",
                        "INPUT_DEVICE_UPDATE_EXIT",
                        1,
                        "0x0061FC9C",
                        true,
                    ),
                    (
                        "INPUT_QUERY",
                        "INPUT_MANAGER_SAMPLE_EXIT",
                        1,
                        "0x00554F82",
                        false,
                    ),
                    (
                        "GAMEPLAY_UPDATE",
                        "CONDITIONAL_ATTEMPT_ENTER",
                        0,
                        "0x00555236",
                        false,
                    ),
                    (
                        "GAMEPLAY_UPDATE",
                        "STANDARD_PREDICATE_ENTER",
                        0,
                        "0x002FADFF",
                        false,
                    ),
                    (
                        "GAMEPLAY_UPDATE",
                        "STANDARD_PREDICATE_EXIT",
                        1,
                        "0x002FADFF",
                        false,
                    ),
                    (
                        "GAMEPLAY_UPDATE",
                        "STANDARD_BODY_ENTER",
                        0,
                        "0x002FAE0E",
                        false,
                    ),
                    (
                        "GAMEPLAY_UPDATE",
                        "STANDARD_BODY_EXIT",
                        1,
                        "0x002FAE0E",
                        false,
                    ),
                    (
                        "ENTITY_MUTATION",
                        "STANDARD_ROOM_ENTER",
                        0,
                        "0x002FBA52",
                        false,
                    ),
                    (
                        "ENTITY_MUTATION",
                        "ENTITY_LIST_ENTER",
                        0,
                        "0x00403E75",
                        false,
                    ),
                    (
                        "ENTITY_MUTATION",
                        "ENTITY_LIST_EXIT",
                        1,
                        "0x00403E75",
                        false,
                    ),
                    (
                        "ENTITY_MUTATION",
                        "STANDARD_ROOM_EXIT",
                        1,
                        "0x002FBA52",
                        false,
                    ),
                    ("POST_UPDATE", "STANDARD_TAIL_ENTER", 0, "0x002FBBAA", false),
                    ("POST_UPDATE", "STANDARD_TAIL_EXIT", 1, "0x002FBBAA", false),
                    (
                        "GAMEPLAY_UPDATE",
                        "CONDITIONAL_ATTEMPT_EXIT",
                        1,
                        "0x00555236",
                        false,
                    ),
                ] {
                    push(
                        iteration,
                        phase,
                        kind,
                        original_calls,
                        None,
                        None,
                        return_rva,
                    );
                }
            }
            push(
                iteration,
                "GAMEPLAY_UPDATE",
                "GAMEPLAY_ENTER",
                0,
                None,
                None,
                "0x00555236",
            );
            push(
                iteration,
                "INPUT_QUERY",
                "INPUT_SCALAR",
                1,
                Some(0),
                Some(1.0_f32.to_bits()),
                "0x00500000",
            );
            push(
                iteration,
                "ENTITY_MUTATION",
                "ENTITY_ENTER",
                0,
                None,
                None,
                "0x002FB521",
            );
            push(
                iteration,
                "ENTITY_MUTATION",
                "ENTITY_EXIT",
                1,
                None,
                None,
                "0x002FB521",
            );
            push(
                iteration,
                "GAMEPLAY_UPDATE",
                "GAMEPLAY_EXIT",
                1,
                None,
                None,
                "0x00555236",
            );
            push(
                iteration,
                "POST_UPDATE",
                "POST_UPDATE_ENTER",
                0,
                None,
                None,
                "0x002FBBAA",
            );
            push(
                iteration,
                "POST_UPDATE",
                "POST_UPDATE_EXIT",
                1,
                None,
                None,
                "0x002FBBAA",
            );
            push(
                iteration,
                "GAMEPLAY_UPDATE",
                "UPDATE_EXIT",
                1,
                None,
                None,
                "0x005312F1",
            );
            push(
                iteration,
                "RENDER",
                "RENDER_ENTER",
                0,
                None,
                None,
                "0x005312F6",
            );
            push(
                iteration,
                "RENDER",
                "RENDER_EXIT",
                1,
                None,
                None,
                "0x005312F6",
            );
        }
        push(
            30,
            "OUTSIDE_ENVELOPE",
            "HOOKS_RESTORED",
            0,
            None,
            None,
            "0x00000000",
        );
        let mut event_bytes = Vec::new();
        for event in &events {
            serde_json::to_writer(&mut event_bytes, event).unwrap();
            event_bytes.push(b'\n');
        }
        fs::write(run.join("events.jsonl"), event_bytes).unwrap();
        let target_sha = sha256_hex(target);
        let manifest = LoopManifest {
            schema_version: 2,
            experiment_identity: LOOP_TOOL_IDENTITY.to_owned(),
            evidence_class: LOOP_EVIDENCE_CLASS.to_owned(),
            case_id: "fixture".to_owned(),
            probe: "passive".to_owned(),
            scenario: "keyboard".to_owned(),
            target_sha256: "A".repeat(64),
            loop_ledger_sha256: "B".repeat(64),
            state: "COMPLETE".to_owned(),
            active_input_allowed: false,
            native_sha256: "C".repeat(64),
            helper_sha256: "D".repeat(64),
            main_thread_id: 7,
            hooks_mask: crate::loop_ipc::LOOP_HOOK_MASK,
            overwritten_events: 0,
            target_bytes_before_sha256: target_sha.clone(),
            target_bytes_after_sha256: target_sha,
            target_bytes_restored: true,
        };
        fs::write(
            run.join("manifest.json"),
            serde_json::to_vec_pretty(&manifest).unwrap(),
        )
        .unwrap();
        run
    }

    fn read_fixture_events(run: &Path) -> Vec<LoopEvent> {
        fs::read(run.join("events.jsonl"))
            .unwrap()
            .split(|byte| *byte == b'\n')
            .filter(|line| !line.is_empty())
            .map(|line| serde_json::from_slice(line).unwrap())
            .collect()
    }

    fn write_fixture_events(run: &Path, events: &mut [LoopEvent]) {
        let mut bytes = Vec::new();
        for (index, event) in events.iter_mut().enumerate() {
            let sequence = u64::try_from(index).unwrap() + 1;
            event.sequence = sequence;
            event.native_sequence = sequence;
            event.qpc = i64::try_from(sequence).unwrap();
            serde_json::to_writer(&mut bytes, event).unwrap();
            bytes.push(b'\n');
        }
        fs::write(run.join("events.jsonl"), bytes).unwrap();
    }

    fn set_fixture_scenario(run: &Path, scenario: &str) {
        let path = run.join("manifest.json");
        let mut manifest: LoopManifest = serde_json::from_slice(&fs::read(&path).unwrap()).unwrap();
        manifest.scenario = scenario.to_owned();
        fs::write(path, serde_json::to_vec_pretty(&manifest).unwrap()).unwrap();
    }

    fn check_passed(report: &LoopAnalysisReport, id: &str) -> bool {
        report
            .checks
            .iter()
            .find(|check| check.id == id)
            .unwrap()
            .passed
    }

    fn scope_event(
        template: &LoopEvent,
        kind: &str,
        phase: &str,
        original_calls: u32,
        return_rva: &str,
        value_bits: Option<u32>,
    ) -> LoopEvent {
        let mut event = template.clone();
        event.kind = kind.to_owned();
        event.phase = phase.to_owned();
        event.original_calls = original_calls;
        event.return_rva = return_rva.to_owned();
        event.action = None;
        event.value_bits = value_bits;
        event.device_key = None;
        event.device_kind = None;
        event.flags = LOOP_FLAG_LOOP_KNOWN
            | LOOP_FLAG_MAIN_THREAD
            | if original_calls == 1 {
                LOOP_FLAG_ORIGINAL_CALLED
            } else {
                0
            };
        event
    }

    fn device_scope_event(
        template: &LoopEvent,
        kind: &str,
        original_calls: u32,
        return_rva: &str,
        device_kind: &str,
        device_key: u32,
    ) -> LoopEvent {
        let mut event = scope_event(
            template,
            kind,
            "INPUT_QUERY",
            original_calls,
            return_rva,
            None,
        );
        event.device_kind = Some(device_kind.to_owned());
        event.device_key = Some(device_key);
        event.flags |= if device_kind == "KEYBOARD" {
            LOOP_FLAG_KEYBOARD_DEVICE
        } else {
            LOOP_FLAG_GAMEPAD_DEVICE
        };
        event
    }

    fn device_switch_refresh_transaction(template: &LoopEvent) -> Vec<LoopEvent> {
        let message = |kind, return_rva, value_bits| {
            scope_event(template, kind, "MESSAGE_PUMP", 1, return_rva, value_bits)
        };
        let device = |kind, original_calls, return_rva, device_kind, device_key| {
            device_scope_event(
                template,
                kind,
                original_calls,
                return_rva,
                device_kind,
                device_key,
            )
        };
        vec![
            message("PEEK_MESSAGE", "0x0065E68C", None),
            message("TRANSLATE_MESSAGE", "0x0065E6D6", None),
            message("DISPATCH_MESSAGE", "0x0065E6E1", Some(0)),
            message("PEEK_MESSAGE", "0x0065E6F0", None),
            device("INPUT_DEVICE_UPDATE_ENTER", 0, "0x0061FC9C", "KEYBOARD", 0),
            device("INPUT_PHYSICAL_POLL_ENTER", 0, "0x0061F30C", "KEYBOARD", 0),
            device("INPUT_PHYSICAL_POLL_EXIT", 1, "0x0061F30C", "KEYBOARD", 0),
            device("INPUT_DEVICE_UPDATE_EXIT", 1, "0x0061FC9C", "KEYBOARD", 0),
            device("INPUT_DEVICE_UPDATE_ENTER", 0, "0x0061FC9C", "GAMEPAD", 3),
            device("INPUT_PHYSICAL_POLL_ENTER", 0, "0x0061F30C", "GAMEPAD", 3),
            device("INPUT_PHYSICAL_POLL_EXIT", 1, "0x0061F30C", "GAMEPAD", 3),
            device("INPUT_DEVICE_UPDATE_EXIT", 1, "0x0061FC9C", "GAMEPAD", 3),
        ]
    }

    fn device_switch_post_manager_refresh_transaction(template: &LoopEvent) -> Vec<LoopEvent> {
        let direct = device_switch_refresh_transaction(template);
        let mut events = direct[..4].to_vec();
        events.push(scope_event(
            template,
            "INPUT_MANAGER_SAMPLE_ENTER",
            "INPUT_QUERY",
            0,
            "0x00554F82",
            None,
        ));
        events.extend_from_slice(&direct[4..]);
        events.push(scope_event(
            template,
            "INPUT_MANAGER_SAMPLE_EXIT",
            "INPUT_QUERY",
            1,
            "0x00554F82",
            None,
        ));
        events.push(scope_event(
            template,
            "PEEK_MESSAGE",
            "MESSAGE_PUMP",
            1,
            "0x0065E68C",
            None,
        ));
        events.extend_from_slice(&direct[4..]);
        events
    }

    fn replace_steady_standard_with_room_lifecycle(events: &mut Vec<LoopEvent>) {
        let first = events
            .iter()
            .position(|event| event.kind == "STANDARD_ROOM_ENTER")
            .unwrap();
        let last = events
            .iter()
            .position(|event| event.kind == "STANDARD_TAIL_EXIT")
            .unwrap();
        let template = events[first].clone();
        events.splice(
            first..=last,
            [
                scope_event(
                    &template,
                    "STANDARD_ROOM_LIFECYCLE_OWNER_ENTER",
                    "ROOM_LIFECYCLE",
                    0,
                    "0x002FB4F5",
                    None,
                ),
                scope_event(
                    &template,
                    "STANDARD_ROOM_LIFECYCLE_SIMULATION_ENTER",
                    "ROOM_LIFECYCLE",
                    0,
                    "0x0052FC7C",
                    None,
                ),
                scope_event(
                    &template,
                    "ENTITY_LIST_ENTER",
                    "ENTITY_MUTATION",
                    0,
                    "0x00403E75",
                    None,
                ),
                scope_event(
                    &template,
                    "ENTITY_LIST_EXIT",
                    "ENTITY_MUTATION",
                    1,
                    "0x00403E75",
                    None,
                ),
                scope_event(
                    &template,
                    "STANDARD_ROOM_LIFECYCLE_SIMULATION_EXIT",
                    "ROOM_LIFECYCLE",
                    1,
                    "0x0052FC7C",
                    None,
                ),
                scope_event(
                    &template,
                    "STANDARD_ROOM_LIFECYCLE_OWNER_EXIT",
                    "ROOM_LIFECYCLE",
                    1,
                    "0x002FB4F5",
                    None,
                ),
            ],
        );
    }

    fn room_change_transaction(template: &LoopEvent) -> Vec<LoopEvent> {
        vec![
            scope_event(
                template,
                "ROOM_CHANGE_OWNER_ENTER",
                "ROOM_LIFECYCLE",
                0,
                "0x002FB521",
                None,
            ),
            scope_event(
                template,
                "NEW_ROOM_ENTER",
                "ROOM_LIFECYCLE",
                0,
                "0x0033F5BE",
                None,
            ),
            scope_event(
                template,
                "NEW_ROOM_EXIT",
                "ROOM_LIFECYCLE",
                1,
                "0x0033F5BE",
                None,
            ),
            scope_event(
                template,
                "ROOM_CHANGE_SIMULATION_ENTER",
                "ROOM_LIFECYCLE",
                0,
                "0x002FE381",
                None,
            ),
            scope_event(
                template,
                "ENTITY_LIST_ENTER",
                "ENTITY_MUTATION",
                0,
                "0x00403E75",
                None,
            ),
            scope_event(
                template,
                "ENTITY_LIST_EXIT",
                "ENTITY_MUTATION",
                1,
                "0x00403E75",
                None,
            ),
            scope_event(
                template,
                "ROOM_CHANGE_SIMULATION_EXIT",
                "ROOM_LIFECYCLE",
                1,
                "0x002FE381",
                None,
            ),
            scope_event(
                template,
                "ROOM_CHANGE_OWNER_EXIT",
                "ROOM_LIFECYCLE",
                1,
                "0x002FB521",
                None,
            ),
        ]
    }

    fn replace_steady_standard_with_room_change(events: &mut Vec<LoopEvent>) {
        let first = events
            .iter()
            .position(|event| event.kind == "STANDARD_ROOM_ENTER")
            .unwrap();
        let last = events
            .iter()
            .position(|event| event.kind == "STANDARD_TAIL_EXIT")
            .unwrap();
        let template = events[first].clone();
        events.splice(first..=last, room_change_transaction(&template));
    }

    fn alternate_room_transaction(template: &LoopEvent) -> Vec<LoopEvent> {
        vec![
            scope_event(
                template,
                "ALTERNATE_OWNER_ENTER",
                "GAMEPLAY_UPDATE",
                0,
                "0x00554D63",
                Some(1),
            ),
            scope_event(
                template,
                "ALTERNATE_STATE_APPLY_ENTER",
                "ENTITY_MUTATION",
                0,
                "0x00559B26",
                None,
            ),
            scope_event(
                template,
                "ALTERNATE_STATE_APPLY_EXIT",
                "ENTITY_MUTATION",
                1,
                "0x00559B26",
                None,
            ),
            scope_event(
                template,
                "ALTERNATE_ROOM_ENTER",
                "ENTITY_MUTATION",
                0,
                "0x00559B52",
                None,
            ),
            scope_event(
                template,
                "ENTITY_LIST_ENTER",
                "ENTITY_MUTATION",
                0,
                "0x00403E75",
                None,
            ),
            scope_event(
                template,
                "ENTITY_LIST_EXIT",
                "ENTITY_MUTATION",
                1,
                "0x00403E75",
                None,
            ),
            scope_event(
                template,
                "ALTERNATE_ROOM_EXIT",
                "ENTITY_MUTATION",
                1,
                "0x00559B52",
                None,
            ),
            scope_event(
                template,
                "ALTERNATE_TAIL_ENTER",
                "RUN_LIFECYCLE",
                0,
                "0x00559B87",
                None,
            ),
            scope_event(
                template,
                "ALTERNATE_TAIL_EXIT",
                "RUN_LIFECYCLE",
                1,
                "0x00559B87",
                None,
            ),
            scope_event(
                template,
                "ALTERNATE_OWNER_EXIT",
                "GAMEPLAY_UPDATE",
                1,
                "0x00554D63",
                Some(0),
            ),
        ]
    }

    #[test]
    fn event_partition_rejects_ui_action() {
        let event = LoopEvent {
            sequence: 1,
            native_sequence: 1,
            qpc: 1,
            loop_iteration: 1,
            phase: "INPUT_QUERY".to_owned(),
            kind: "INPUT_PRESSED".to_owned(),
            thread_id: 1,
            action: Some(12),
            value_bits: None,
            flags: 0,
            return_rva: "0x00000000".to_owned(),
            original_calls: 1,
            error_code: 0,
            device_key: Some(0),
            device_kind: Some("KEYBOARD".to_owned()),
            install_failure_stage: None,
            install_failure_index: None,
            install_failure_win32_error: None,
        };
        assert!(event.action.is_some_and(|action| action > 11));
    }

    #[test]
    fn event_contract_accepts_only_complete_install_diagnostics_on_failure() {
        let event = LoopEvent {
            sequence: 1,
            native_sequence: 1,
            qpc: 1,
            loop_iteration: 0,
            phase: "OUTSIDE_ENVELOPE".to_owned(),
            kind: "FAILURE".to_owned(),
            thread_id: 1,
            action: None,
            value_bits: None,
            flags: 0,
            return_rva: "0x00000000".to_owned(),
            original_calls: 0,
            error_code: 4,
            device_key: None,
            device_kind: None,
            install_failure_stage: Some(38),
            install_failure_index: Some(1234),
            install_failure_win32_error: Some(5),
        };
        assert!(event_contract_valid(&event, 0));

        let mut partial = event.clone();
        partial.install_failure_win32_error = None;
        assert!(!event_contract_valid(&partial, 0));

        let mut misplaced = event;
        misplaced.kind = "NATIVE_INITIALIZED".to_owned();
        misplaced.error_code = 0;
        assert!(!event_contract_valid(&misplaced, 0));
    }

    #[test]
    fn gate_output_is_create_new() {
        let directory = tempfile::tempdir().unwrap();
        let path = directory.path().join("gate.json");
        write_new_json(&path, &serde_json::json!({"status":"x"})).unwrap();
        assert!(write_new_json(&path, &serde_json::json!({"status":"y"})).is_err());
    }

    #[test]
    fn loop_gate_requires_only_passively_provable_envelope_scenarios() {
        assert_eq!(
            REQUIRED_LOOP_GATE_SCENARIOS,
            &[
                "keyboard",
                "controller",
                "device-switch",
                "pause",
                "loading",
                "menu",
                "room",
            ]
        );
        assert!(!REQUIRED_LOOP_GATE_SCENARIOS.contains(&"simultaneous"));
        assert!(!REQUIRED_LOOP_GATE_SCENARIOS.contains(&"rapid-tap"));
        assert!(!REQUIRED_LOOP_GATE_SCENARIOS.contains(&"focus"));
    }

    #[test]
    fn native_loop_fixture_replays_and_gate_rejects_tampered_analysis() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        let report = analyze_loop_run(&run).unwrap();
        assert_eq!(report.result, "PASS");
        assert!(report.eligible_for_active_gate);
        let analysis = run.join("analysis.json");
        let original_analysis = fs::read(&analysis).unwrap();
        let replay = replay_loop_run(&run).unwrap();
        assert_eq!(replay.result, "PASS");
        assert_eq!(fs::read(&analysis).unwrap(), original_analysis);
        let mut tampered: serde_json::Value =
            serde_json::from_slice(&fs::read(&analysis).unwrap()).unwrap();
        tampered["completeLoopCount"] = serde_json::json!(999);
        fs::write(&analysis, serde_json::to_vec_pretty(&tampered).unwrap()).unwrap();
        let matrix = directory.path().join("matrix.json");
        fs::write(&matrix, br#"{"analyses":["keyboard-run/analysis.json"]}"#).unwrap();
        assert!(write_loop_gate(&matrix, &directory.path().join("gate.json")).is_err());
    }

    #[test]
    fn analyzer_rejects_vacuous_authority_evidence() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        let mut events = read_fixture_events(&run);
        events.retain(|event| {
            !matches!(
                event.kind.as_str(),
                "GAMEPLAY_ENTER"
                    | "GAMEPLAY_EXIT"
                    | "ENTITY_ENTER"
                    | "ENTITY_EXIT"
                    | "POST_UPDATE_ENTER"
                    | "POST_UPDATE_EXIT"
                    | "CONDITIONAL_ATTEMPT_ENTER"
                    | "CONDITIONAL_ATTEMPT_EXIT"
                    | "STANDARD_PREDICATE_ENTER"
                    | "STANDARD_PREDICATE_EXIT"
                    | "STANDARD_BODY_ENTER"
                    | "STANDARD_BODY_EXIT"
                    | "STANDARD_ROOM_ENTER"
                    | "STANDARD_ROOM_EXIT"
                    | "STANDARD_TAIL_ENTER"
                    | "STANDARD_TAIL_EXIT"
                    | "ENTITY_LIST_ENTER"
                    | "ENTITY_LIST_EXIT"
            )
        });
        write_fixture_events(&run, &mut events);

        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-evidence-nonempty"));
    }

    #[test]
    fn analyzer_rejects_legacy_only_events_under_v5_manifest() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        let mut events = read_fixture_events(&run);
        events.retain(|event| {
            !matches!(
                event.kind.as_str(),
                "INPUT_MANAGER_SAMPLE_ENTER"
                    | "INPUT_MANAGER_SAMPLE_EXIT"
                    | "INPUT_DEVICE_UPDATE_ENTER"
                    | "INPUT_DEVICE_UPDATE_EXIT"
                    | "INPUT_PHYSICAL_POLL_ENTER"
                    | "INPUT_PHYSICAL_POLL_EXIT"
                    | "CONDITIONAL_ATTEMPT_ENTER"
                    | "CONDITIONAL_ATTEMPT_EXIT"
                    | "STANDARD_PREDICATE_ENTER"
                    | "STANDARD_PREDICATE_EXIT"
                    | "STANDARD_BODY_ENTER"
                    | "STANDARD_BODY_EXIT"
                    | "STANDARD_ROOM_ENTER"
                    | "STANDARD_ROOM_EXIT"
                    | "STANDARD_TAIL_ENTER"
                    | "STANDARD_TAIL_EXIT"
                    | "ENTITY_LIST_ENTER"
                    | "ENTITY_LIST_EXIT"
            )
        });
        write_fixture_events(&run, &mut events);

        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-evidence-nonempty"));
        assert!(!check_passed(&report, "physical-sample-envelope"));
    }

    #[test]
    fn analyzer_rejects_v1_through_v4_identity_or_evidence_class_under_v5() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        analyze_loop_run(&run).unwrap();
        let matrix_path = directory.path().join("matrix.json");
        fs::write(
            &matrix_path,
            br#"{"analyses":["keyboard-run/analysis.json"]}"#,
        )
        .unwrap();
        let manifest_path = run.join("manifest.json");
        let current: LoopManifest =
            serde_json::from_slice(&fs::read(&manifest_path).unwrap()).unwrap();

        for (identity, evidence_class) in [
            (
                "IsaacCsT0-LoopEnvelopeResearch-v2",
                "NATIVE_PASSIVE_LOOP_V2",
            ),
            (
                "IsaacCsT0-LoopEnvelopeResearch-v1",
                "NATIVE_PASSIVE_LOOP_V1",
            ),
            (
                "IsaacCsT0-LoopEnvelopeResearch-v3",
                "NATIVE_PASSIVE_LOOP_V3",
            ),
            (
                "IsaacCsT0-LoopEnvelopeResearch-v4",
                "NATIVE_PASSIVE_LOOP_V4",
            ),
            (LOOP_TOOL_IDENTITY, "NATIVE_PASSIVE_LOOP_V2"),
            (LOOP_TOOL_IDENTITY, "NATIVE_PASSIVE_LOOP_V3"),
            (LOOP_TOOL_IDENTITY, "NATIVE_PASSIVE_LOOP_V4"),
            ("IsaacCsT0-LoopEnvelopeResearch-v4", LOOP_EVIDENCE_CLASS),
        ] {
            let mut manifest = current.clone();
            manifest.experiment_identity = identity.to_owned();
            manifest.evidence_class = evidence_class.to_owned();
            fs::write(
                &manifest_path,
                serde_json::to_vec_pretty(&manifest).unwrap(),
            )
            .unwrap();
            assert!(analyze_loop_run_internal(&run).is_err());
            assert!(write_loop_gate(&matrix_path, &directory.path().join("gate.json")).is_err());
        }
    }

    #[test]
    fn analyzer_accepts_complete_room_change_transaction() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        set_fixture_scenario(&run, "room");
        let mut events = read_fixture_events(&run);
        replace_steady_standard_with_room_change(&mut events);
        write_fixture_events(&run, &mut events);

        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "PASS");
        for id in [
            "event-contract",
            "authority-owned",
            "authority-evidence-nonempty",
            "authority-transaction-order",
            "balanced-depth",
            "main-thread-owned",
            "scenario-evidence",
        ] {
            assert!(check_passed(&report, id), "{id}");
        }
    }

    #[test]
    fn room_change_entity_internal_cardinality_does_not_change_outer_envelope() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        set_fixture_scenario(&run, "room");
        let mut no_entities = read_fixture_events(&run);
        replace_steady_standard_with_room_change(&mut no_entities);
        write_fixture_events(&run, &mut no_entities);
        let no_entities_report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(no_entities_report.result, "PASS");

        let mut with_entities = no_entities;
        let entity_list_exit = with_entities
            .iter()
            .position(|event| event.kind == "ENTITY_LIST_EXIT")
            .unwrap();
        let template = with_entities[entity_list_exit].clone();
        with_entities.splice(
            entity_list_exit..entity_list_exit,
            [
                scope_event(
                    &template,
                    "ENTITY_ENTER",
                    "ENTITY_MUTATION",
                    0,
                    "0x002FB521",
                    None,
                ),
                scope_event(
                    &template,
                    "ENTITY_EXIT",
                    "ENTITY_MUTATION",
                    1,
                    "0x002FB521",
                    None,
                ),
                scope_event(
                    &template,
                    "ENTITY_ENTER",
                    "ENTITY_MUTATION",
                    0,
                    "0x002FB521",
                    None,
                ),
                scope_event(
                    &template,
                    "ENTITY_EXIT",
                    "ENTITY_MUTATION",
                    1,
                    "0x002FB521",
                    None,
                ),
            ],
        );
        write_fixture_events(&run, &mut with_entities);
        let with_entities_report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(with_entities_report.result, "PASS");
        assert_eq!(
            with_entities_report.loop_iteration_count,
            no_entities_report.loop_iteration_count
        );
        assert_eq!(
            with_entities_report.complete_loop_count,
            no_entities_report.complete_loop_count
        );
        assert_eq!(
            with_entities_report.input_query_count,
            no_entities_report.input_query_count
        );
        assert!(check_passed(
            &with_entities_report,
            "authority-transaction-order"
        ));
        assert!(check_passed(&with_entities_report, "balanced-depth"));
        assert!(check_passed(&with_entities_report, "scenario-evidence"));
    }

    #[test]
    fn analyzer_keeps_room_change_owner_only_legal_but_non_authoritative() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        let mut events = read_fixture_events(&run);
        let first = events
            .iter()
            .position(|event| event.kind == "STANDARD_ROOM_ENTER")
            .unwrap();
        let last = events
            .iter()
            .position(|event| event.kind == "STANDARD_TAIL_EXIT")
            .unwrap();
        let template = events[first].clone();
        events.splice(
            first..=last,
            [
                scope_event(
                    &template,
                    "ROOM_CHANGE_OWNER_ENTER",
                    "ROOM_LIFECYCLE",
                    0,
                    "0x002FB521",
                    None,
                ),
                scope_event(
                    &template,
                    "ROOM_CHANGE_OWNER_EXIT",
                    "ROOM_LIFECYCLE",
                    1,
                    "0x002FB521",
                    None,
                ),
            ],
        );
        write_fixture_events(&run, &mut events);

        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(check_passed(&report, "event-contract"));
        assert!(check_passed(&report, "authority-owned"));
        assert!(!check_passed(&report, "authority-evidence-nonempty"));
        assert!(check_passed(&report, "authority-transaction-order"));
        assert!(check_passed(&report, "balanced-depth"));
    }

    #[test]
    fn analyzer_rejects_real_v4_room_shape_without_room_change_owner() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        set_fixture_scenario(&run, "room");
        let mut events = read_fixture_events(&run);
        let first = events
            .iter()
            .position(|event| event.kind == "STANDARD_ROOM_ENTER")
            .unwrap();
        let last = events
            .iter()
            .position(|event| event.kind == "STANDARD_TAIL_EXIT")
            .unwrap();
        let template = events[first].clone();
        events.splice(
            first..=last,
            [
                scope_event(
                    &template,
                    "NEW_ROOM_ENTER",
                    "ROOM_LIFECYCLE",
                    0,
                    "0x0033F5BE",
                    None,
                ),
                scope_event(
                    &template,
                    "NEW_ROOM_EXIT",
                    "ROOM_LIFECYCLE",
                    1,
                    "0x0033F5BE",
                    None,
                ),
                scope_event(
                    &template,
                    "ENTITY_LIST_ENTER",
                    "ENTITY_MUTATION",
                    0,
                    "0x00403E75",
                    None,
                ),
                scope_event(
                    &template,
                    "ENTITY_LIST_EXIT",
                    "ENTITY_MUTATION",
                    1,
                    "0x00403E75",
                    None,
                ),
            ],
        );
        write_fixture_events(&run, &mut events);

        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-owned"));
        assert!(!check_passed(&report, "balanced-depth"));
        assert!(!check_passed(&report, "authority-evidence-nonempty"));
        assert!(!check_passed(&report, "scenario-evidence"));

        let manifest_path = run.join("manifest.json");
        let mut manifest: LoopManifest =
            serde_json::from_slice(&fs::read(&manifest_path).unwrap()).unwrap();
        manifest.experiment_identity = "IsaacCsT0-LoopEnvelopeResearch-v4".to_owned();
        manifest.evidence_class = "NATIVE_PASSIVE_LOOP_V4".to_owned();
        fs::write(manifest_path, serde_json::to_vec_pretty(&manifest).unwrap()).unwrap();
        assert!(analyze_loop_run_internal(&run).is_err());
    }

    #[test]
    fn analyzer_rejects_incomplete_cross_loop_or_duplicate_room_change_transaction() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        set_fixture_scenario(&run, "room");
        let mut valid = read_fixture_events(&run);
        replace_steady_standard_with_room_change(&mut valid);
        let room_change_loop = valid
            .iter()
            .find(|event| event.kind == "ROOM_CHANGE_OWNER_ENTER")
            .unwrap()
            .loop_iteration;

        let mut missing_new_room = valid.clone();
        missing_new_room
            .retain(|event| !matches!(event.kind.as_str(), "NEW_ROOM_ENTER" | "NEW_ROOM_EXIT"));
        write_fixture_events(&run, &mut missing_new_room);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-evidence-nonempty"));
        assert!(!check_passed(&report, "authority-transaction-order"));

        let mut missing_entity_list = valid.clone();
        missing_entity_list.retain(|event| {
            event.loop_iteration != room_change_loop
                || !matches!(
                    event.kind.as_str(),
                    "ENTITY_LIST_ENTER" | "ENTITY_LIST_EXIT"
                )
        });
        write_fixture_events(&run, &mut missing_entity_list);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-evidence-nonempty"));
        assert!(!check_passed(&report, "authority-transaction-order"));

        let mut cross_loop = valid.clone();
        for event in cross_loop.iter_mut().filter(|event| {
            event.loop_iteration == room_change_loop
                && matches!(
                    event.kind.as_str(),
                    "ROOM_CHANGE_SIMULATION_ENTER"
                        | "ROOM_CHANGE_SIMULATION_EXIT"
                        | "ENTITY_LIST_ENTER"
                        | "ENTITY_LIST_EXIT"
                )
        }) {
            event.loop_iteration = room_change_loop + 1;
        }
        write_fixture_events(&run, &mut cross_loop);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-evidence-nonempty"));
        assert!(!check_passed(&report, "authority-transaction-order"));

        let mut duplicate_entity = valid;
        let entity_exit = duplicate_entity
            .iter()
            .position(|event| event.kind == "ENTITY_LIST_EXIT")
            .unwrap();
        let pair = [
            duplicate_entity[entity_exit - 1].clone(),
            duplicate_entity[entity_exit].clone(),
        ];
        duplicate_entity.splice(entity_exit + 1..entity_exit + 1, pair);
        write_fixture_events(&run, &mut duplicate_entity);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-evidence-nonempty"));
        assert!(!check_passed(&report, "authority-transaction-order"));
    }

    #[test]
    fn analyzer_rejects_room_change_contract_forgery() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        set_fixture_scenario(&run, "room");
        let mut valid = read_fixture_events(&run);
        replace_steady_standard_with_room_change(&mut valid);
        let index = valid
            .iter()
            .position(|event| event.kind == "ROOM_CHANGE_SIMULATION_ENTER")
            .unwrap();

        for mutation in 0..5 {
            let mut events = valid.clone();
            match mutation {
                0 => events[index].phase = "ENTITY_MUTATION".to_owned(),
                1 => events[index].return_rva = "0x002FE382".to_owned(),
                2 => events[index].value_bits = Some(0),
                3 => events[index].flags |= LOOP_FLAG_RESULT_TRUE,
                4 => events[index].original_calls = 1,
                _ => unreachable!(),
            }
            write_fixture_events(&run, &mut events);
            let report = analyze_loop_run_internal(&run).unwrap();
            assert_eq!(report.result, "FAIL");
            assert!(!check_passed(&report, "event-contract"));
        }

        let mut wrong_thread = valid;
        wrong_thread[index].thread_id = 8;
        wrong_thread[index].flags &= !LOOP_FLAG_MAIN_THREAD;
        wrong_thread[index].flags |= LOOP_FLAG_WRONG_THREAD;
        write_fixture_events(&run, &mut wrong_thread);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(check_passed(&report, "event-contract"));
        assert!(!check_passed(&report, "main-thread-owned"));
    }

    #[test]
    fn analyzer_rejects_room_change_mixed_with_other_authority_paths() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        let original = read_fixture_events(&run);

        let mut with_steady = original.clone();
        let steady = with_steady
            .iter()
            .position(|event| event.kind == "STANDARD_ROOM_ENTER")
            .unwrap();
        let template = with_steady[steady].clone();
        with_steady.splice(steady..steady, room_change_transaction(&template));
        write_fixture_events(&run, &mut with_steady);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-transaction-order"));

        let mut with_lifecycle = original.clone();
        replace_steady_standard_with_room_lifecycle(&mut with_lifecycle);
        let lifecycle = with_lifecycle
            .iter()
            .position(|event| event.kind == "STANDARD_ROOM_LIFECYCLE_OWNER_ENTER")
            .unwrap();
        let template = with_lifecycle[lifecycle].clone();
        with_lifecycle.splice(lifecycle..lifecycle, room_change_transaction(&template));
        write_fixture_events(&run, &mut with_lifecycle);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-transaction-order"));

        let mut with_alternate = original;
        replace_steady_standard_with_room_change(&mut with_alternate);
        let conditional = with_alternate
            .iter()
            .position(|event| event.kind == "CONDITIONAL_ATTEMPT_ENTER")
            .unwrap();
        let template = with_alternate[conditional].clone();
        with_alternate.splice(
            conditional..conditional,
            alternate_room_transaction(&template),
        );
        write_fixture_events(&run, &mut with_alternate);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-transaction-order"));
    }

    #[test]
    fn analyzer_accepts_complete_standard_room_lifecycle_transaction() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        let mut events = read_fixture_events(&run);
        replace_steady_standard_with_room_lifecycle(&mut events);
        write_fixture_events(&run, &mut events);

        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "PASS");
        assert!(check_passed(&report, "event-contract"));
        assert!(check_passed(&report, "authority-evidence-nonempty"));
        assert!(check_passed(&report, "authority-transaction-order"));
        assert!(check_passed(&report, "balanced-depth"));
        assert!(check_passed(&report, "main-thread-owned"));
    }

    #[test]
    fn analyzer_keeps_room_lifecycle_owner_only_non_authoritative() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        let mut events = read_fixture_events(&run);
        replace_steady_standard_with_room_lifecycle(&mut events);
        events.retain(|event| {
            !matches!(
                event.kind.as_str(),
                "STANDARD_ROOM_LIFECYCLE_SIMULATION_ENTER"
                    | "STANDARD_ROOM_LIFECYCLE_SIMULATION_EXIT"
                    | "ENTITY_LIST_ENTER"
                    | "ENTITY_LIST_EXIT"
            )
        });
        write_fixture_events(&run, &mut events);

        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(check_passed(&report, "event-contract"));
        assert!(!check_passed(&report, "authority-evidence-nonempty"));
        assert!(check_passed(&report, "authority-transaction-order"));
        assert!(check_passed(&report, "balanced-depth"));
    }

    #[test]
    fn analyzer_rejects_incomplete_or_cross_loop_room_lifecycle_simulation() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        let mut valid = read_fixture_events(&run);
        replace_steady_standard_with_room_lifecycle(&mut valid);

        let mut missing_entity_pair = valid.clone();
        missing_entity_pair.retain(|event| {
            !matches!(
                event.kind.as_str(),
                "ENTITY_LIST_ENTER" | "ENTITY_LIST_EXIT"
            )
        });
        write_fixture_events(&run, &mut missing_entity_pair);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-evidence-nonempty"));
        assert!(!check_passed(&report, "authority-transaction-order"));

        let mut cross_loop = valid;
        for event in cross_loop.iter_mut().filter(|event| {
            matches!(
                event.kind.as_str(),
                "STANDARD_ROOM_LIFECYCLE_SIMULATION_ENTER"
                    | "STANDARD_ROOM_LIFECYCLE_SIMULATION_EXIT"
                    | "ENTITY_LIST_ENTER"
                    | "ENTITY_LIST_EXIT"
            )
        }) {
            event.loop_iteration = 2;
        }
        write_fixture_events(&run, &mut cross_loop);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-evidence-nonempty"));
        assert!(!check_passed(&report, "authority-transaction-order"));
    }

    #[test]
    fn analyzer_rejects_room_lifecycle_event_contract_forgery() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        let mut valid = read_fixture_events(&run);
        replace_steady_standard_with_room_lifecycle(&mut valid);
        let index = valid
            .iter()
            .position(|event| event.kind == "STANDARD_ROOM_LIFECYCLE_SIMULATION_ENTER")
            .unwrap();

        for mutation in 0..5 {
            let mut events = valid.clone();
            match mutation {
                0 => events[index].phase = "ENTITY_MUTATION".to_owned(),
                1 => events[index].return_rva = "0x0052FC7D".to_owned(),
                2 => events[index].value_bits = Some(0),
                3 => events[index].flags |= LOOP_FLAG_RESULT_TRUE,
                4 => events[index].original_calls = 1,
                _ => unreachable!(),
            }
            write_fixture_events(&run, &mut events);
            let report = analyze_loop_run_internal(&run).unwrap();
            assert_eq!(report.result, "FAIL");
            assert!(!check_passed(&report, "event-contract"));
        }

        let mut wrong_thread = valid;
        wrong_thread[index].thread_id = 8;
        wrong_thread[index].flags &= !LOOP_FLAG_MAIN_THREAD;
        wrong_thread[index].flags |= LOOP_FLAG_WRONG_THREAD;
        write_fixture_events(&run, &mut wrong_thread);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(check_passed(&report, "event-contract"));
        assert!(!check_passed(&report, "main-thread-owned"));
    }

    #[test]
    fn analyzer_rejects_room_lifecycle_mixed_with_other_simulation_paths() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        let mut lifecycle = read_fixture_events(&run);
        replace_steady_standard_with_room_lifecycle(&mut lifecycle);

        let mut with_steady = lifecycle.clone();
        let conditional_exit = with_steady
            .iter()
            .position(|event| event.kind == "CONDITIONAL_ATTEMPT_EXIT")
            .unwrap();
        let template = with_steady[conditional_exit].clone();
        with_steady.splice(
            conditional_exit..conditional_exit,
            [
                scope_event(
                    &template,
                    "STANDARD_ROOM_ENTER",
                    "ENTITY_MUTATION",
                    0,
                    "0x002FBA52",
                    None,
                ),
                scope_event(
                    &template,
                    "ENTITY_LIST_ENTER",
                    "ENTITY_MUTATION",
                    0,
                    "0x00403E75",
                    None,
                ),
                scope_event(
                    &template,
                    "ENTITY_LIST_EXIT",
                    "ENTITY_MUTATION",
                    1,
                    "0x00403E75",
                    None,
                ),
                scope_event(
                    &template,
                    "STANDARD_ROOM_EXIT",
                    "ENTITY_MUTATION",
                    1,
                    "0x002FBA52",
                    None,
                ),
                scope_event(
                    &template,
                    "STANDARD_TAIL_ENTER",
                    "POST_UPDATE",
                    0,
                    "0x002FBBAA",
                    None,
                ),
                scope_event(
                    &template,
                    "STANDARD_TAIL_EXIT",
                    "POST_UPDATE",
                    1,
                    "0x002FBBAA",
                    None,
                ),
            ],
        );
        write_fixture_events(&run, &mut with_steady);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-transaction-order"));

        let mut with_alternate = lifecycle;
        let conditional_enter = with_alternate
            .iter()
            .position(|event| event.kind == "CONDITIONAL_ATTEMPT_ENTER")
            .unwrap();
        let template = with_alternate[conditional_enter].clone();
        with_alternate.splice(
            conditional_enter..conditional_enter,
            [
                scope_event(
                    &template,
                    "ALTERNATE_OWNER_ENTER",
                    "GAMEPLAY_UPDATE",
                    0,
                    "0x00554D63",
                    Some(1),
                ),
                scope_event(
                    &template,
                    "ALTERNATE_STATE_APPLY_ENTER",
                    "ENTITY_MUTATION",
                    0,
                    "0x00559B26",
                    None,
                ),
                scope_event(
                    &template,
                    "ALTERNATE_STATE_APPLY_EXIT",
                    "ENTITY_MUTATION",
                    1,
                    "0x00559B26",
                    None,
                ),
                scope_event(
                    &template,
                    "ALTERNATE_ROOM_ENTER",
                    "ENTITY_MUTATION",
                    0,
                    "0x00559B52",
                    None,
                ),
                scope_event(
                    &template,
                    "ENTITY_LIST_ENTER",
                    "ENTITY_MUTATION",
                    0,
                    "0x00403E75",
                    None,
                ),
                scope_event(
                    &template,
                    "ENTITY_LIST_EXIT",
                    "ENTITY_MUTATION",
                    1,
                    "0x00403E75",
                    None,
                ),
                scope_event(
                    &template,
                    "ALTERNATE_ROOM_EXIT",
                    "ENTITY_MUTATION",
                    1,
                    "0x00559B52",
                    None,
                ),
                scope_event(
                    &template,
                    "ALTERNATE_TAIL_ENTER",
                    "RUN_LIFECYCLE",
                    0,
                    "0x00559B87",
                    None,
                ),
                scope_event(
                    &template,
                    "ALTERNATE_TAIL_EXIT",
                    "RUN_LIFECYCLE",
                    1,
                    "0x00559B87",
                    None,
                ),
                scope_event(
                    &template,
                    "ALTERNATE_OWNER_EXIT",
                    "GAMEPLAY_UPDATE",
                    1,
                    "0x00554D63",
                    Some(0),
                ),
            ],
        );
        write_fixture_events(&run, &mut with_alternate);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(check_passed(&report, "authority-evidence-nonempty"));
        assert!(!check_passed(&report, "authority-transaction-order"));
    }

    #[test]
    fn analyzer_accepts_v5_physical_sample_and_standard_transaction() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        let original = read_fixture_events(&run);
        let mut events = Vec::new();
        for event in original {
            if event.kind == "GAMEPLAY_ENTER" {
                let make = |kind: &str,
                            phase: &str,
                            original_calls: u32,
                            return_rva: &str,
                            device: bool| {
                    let mut value = event.clone();
                    value.kind = kind.to_owned();
                    value.phase = phase.to_owned();
                    value.original_calls = original_calls;
                    value.return_rva = return_rva.to_owned();
                    value.action = None;
                    value.value_bits = None;
                    value.device_key = device.then_some(0);
                    value.device_kind = device.then_some("KEYBOARD".to_owned());
                    value.flags = LOOP_FLAG_LOOP_KNOWN
                        | LOOP_FLAG_MAIN_THREAD
                        | if original_calls == 1 {
                            LOOP_FLAG_ORIGINAL_CALLED
                        } else {
                            0
                        }
                        | if device { LOOP_FLAG_KEYBOARD_DEVICE } else { 0 };
                    value
                };
                events.extend([
                    make(
                        "INPUT_MANAGER_SAMPLE_ENTER",
                        "INPUT_QUERY",
                        0,
                        "0x00554F82",
                        false,
                    ),
                    make(
                        "INPUT_DEVICE_UPDATE_ENTER",
                        "INPUT_QUERY",
                        0,
                        "0x0061FC9C",
                        true,
                    ),
                    make(
                        "INPUT_PHYSICAL_POLL_ENTER",
                        "INPUT_QUERY",
                        0,
                        "0x0061F30C",
                        true,
                    ),
                    make(
                        "INPUT_PHYSICAL_POLL_EXIT",
                        "INPUT_QUERY",
                        1,
                        "0x0061F30C",
                        true,
                    ),
                    make(
                        "INPUT_DEVICE_UPDATE_EXIT",
                        "INPUT_QUERY",
                        1,
                        "0x0061FC9C",
                        true,
                    ),
                    make(
                        "INPUT_MANAGER_SAMPLE_EXIT",
                        "INPUT_QUERY",
                        1,
                        "0x00554F82",
                        false,
                    ),
                    make(
                        "CONDITIONAL_ATTEMPT_ENTER",
                        "GAMEPLAY_UPDATE",
                        0,
                        "0x00555236",
                        false,
                    ),
                    make(
                        "STANDARD_PREDICATE_ENTER",
                        "GAMEPLAY_UPDATE",
                        0,
                        "0x002FADFF",
                        false,
                    ),
                    make(
                        "STANDARD_PREDICATE_EXIT",
                        "GAMEPLAY_UPDATE",
                        1,
                        "0x002FADFF",
                        false,
                    ),
                    make(
                        "STANDARD_BODY_ENTER",
                        "GAMEPLAY_UPDATE",
                        0,
                        "0x002FAE0E",
                        false,
                    ),
                    make(
                        "STANDARD_BODY_EXIT",
                        "GAMEPLAY_UPDATE",
                        1,
                        "0x002FAE0E",
                        false,
                    ),
                    make(
                        "STANDARD_ROOM_ENTER",
                        "ENTITY_MUTATION",
                        0,
                        "0x002FBA52",
                        false,
                    ),
                    make(
                        "ENTITY_LIST_ENTER",
                        "ENTITY_MUTATION",
                        0,
                        "0x00403E75",
                        false,
                    ),
                    make(
                        "ENTITY_LIST_EXIT",
                        "ENTITY_MUTATION",
                        1,
                        "0x00403E75",
                        false,
                    ),
                    make(
                        "STANDARD_ROOM_EXIT",
                        "ENTITY_MUTATION",
                        1,
                        "0x002FBA52",
                        false,
                    ),
                    make("STANDARD_TAIL_ENTER", "POST_UPDATE", 0, "0x002FBBAA", false),
                    make("STANDARD_TAIL_EXIT", "POST_UPDATE", 1, "0x002FBBAA", false),
                    make(
                        "CONDITIONAL_ATTEMPT_EXIT",
                        "GAMEPLAY_UPDATE",
                        1,
                        "0x00555236",
                        false,
                    ),
                ]);
            } else if !matches!(
                event.kind.as_str(),
                "GAMEPLAY_EXIT"
                    | "ENTITY_ENTER"
                    | "ENTITY_EXIT"
                    | "POST_UPDATE_ENTER"
                    | "POST_UPDATE_EXIT"
            ) {
                events.push(event);
            }
        }
        write_fixture_events(&run, &mut events);

        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "PASS");
        assert!(check_passed(&report, "physical-sample-envelope"));
        assert!(check_passed(&report, "authority-evidence-nonempty"));
        assert!(check_passed(&report, "authority-transaction-order"));

        let valid = events.clone();
        let poll_exit = events
            .iter_mut()
            .find(|event| event.kind == "INPUT_PHYSICAL_POLL_EXIT")
            .unwrap();
        poll_exit.device_kind = Some("GAMEPAD".to_owned());
        poll_exit.device_key = Some(1);
        poll_exit.flags &= !LOOP_FLAG_KEYBOARD_DEVICE;
        poll_exit.flags |= LOOP_FLAG_GAMEPAD_DEVICE;
        write_fixture_events(&run, &mut events);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(check_passed(&report, "event-contract"));
        assert!(!check_passed(&report, "ui-partition"));

        events = valid.clone();
        for event in events
            .iter_mut()
            .filter(|event| event.kind == "STANDARD_PREDICATE_EXIT")
        {
            event.flags |= LOOP_FLAG_RESULT_TRUE;
        }
        write_fixture_events(&run, &mut events);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-evidence-nonempty"));
        assert!(!check_passed(&report, "authority-transaction-order"));

        events = valid.clone();
        events.retain(|event| {
            !matches!(
                event.kind.as_str(),
                "STANDARD_PREDICATE_ENTER"
                    | "STANDARD_PREDICATE_EXIT"
                    | "STANDARD_BODY_ENTER"
                    | "STANDARD_BODY_EXIT"
                    | "STANDARD_ROOM_ENTER"
                    | "STANDARD_ROOM_EXIT"
                    | "STANDARD_TAIL_ENTER"
                    | "STANDARD_TAIL_EXIT"
                    | "ENTITY_LIST_ENTER"
                    | "ENTITY_LIST_EXIT"
            )
        });
        write_fixture_events(&run, &mut events);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-evidence-nonempty"));
        assert!(check_passed(&report, "authority-transaction-order"));

        let mut fragmented = Vec::new();
        for event in &valid {
            fragmented.push(event.clone());
            if matches!(
                event.kind.as_str(),
                "STANDARD_PREDICATE_EXIT" | "STANDARD_BODY_EXIT" | "STANDARD_ROOM_EXIT"
            ) {
                fragmented.push(
                    valid
                        .iter()
                        .find(|candidate| {
                            candidate.loop_iteration == event.loop_iteration
                                && candidate.kind == "CONDITIONAL_ATTEMPT_EXIT"
                        })
                        .unwrap()
                        .clone(),
                );
                fragmented.push(
                    valid
                        .iter()
                        .find(|candidate| {
                            candidate.loop_iteration == event.loop_iteration
                                && candidate.kind == "CONDITIONAL_ATTEMPT_ENTER"
                        })
                        .unwrap()
                        .clone(),
                );
            }
        }
        write_fixture_events(&run, &mut fragmented);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-evidence-nonempty"));
        assert!(!check_passed(&report, "authority-transaction-order"));

        let mut with_outcomes = Vec::new();
        for event in &valid {
            if event.kind == "CONDITIONAL_ATTEMPT_ENTER" {
                let make = |kind: &str,
                            phase: &str,
                            original_calls: u32,
                            return_rva: &str,
                            value_bits: Option<u32>| {
                    let mut value = event.clone();
                    value.kind = kind.to_owned();
                    value.phase = phase.to_owned();
                    value.original_calls = original_calls;
                    value.return_rva = return_rva.to_owned();
                    value.action = None;
                    value.value_bits = value_bits;
                    value.device_key = None;
                    value.device_kind = None;
                    value.flags = LOOP_FLAG_LOOP_KNOWN
                        | LOOP_FLAG_MAIN_THREAD
                        | if original_calls == 1 {
                            LOOP_FLAG_ORIGINAL_CALLED
                        } else {
                            0
                        };
                    value
                };
                with_outcomes.extend([
                    make(
                        "ALTERNATE_OWNER_ENTER",
                        "GAMEPLAY_UPDATE",
                        0,
                        "0x00554D63",
                        Some(1),
                    ),
                    make(
                        "ALTERNATE_OUTCOME_ENTER",
                        "RUN_LIFECYCLE",
                        0,
                        "0x002F5810",
                        None,
                    ),
                    make(
                        "ALTERNATE_OUTCOME_EXIT",
                        "RUN_LIFECYCLE",
                        1,
                        "0x002F5810",
                        None,
                    ),
                    make(
                        "ALTERNATE_OWNER_EXIT",
                        "GAMEPLAY_UPDATE",
                        1,
                        "0x00554D63",
                        Some(0),
                    ),
                ]);
            }
            with_outcomes.push(event.clone());
        }
        write_fixture_events(&run, &mut with_outcomes);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "PASS");
        assert!(check_passed(&report, "authority-evidence-nonempty"));
        assert!(check_passed(&report, "authority-transaction-order"));

        let mut outcome_only = with_outcomes.clone();
        outcome_only.retain(|event| {
            !matches!(
                event.kind.as_str(),
                "STANDARD_PREDICATE_ENTER"
                    | "STANDARD_PREDICATE_EXIT"
                    | "STANDARD_BODY_ENTER"
                    | "STANDARD_BODY_EXIT"
                    | "STANDARD_ROOM_ENTER"
                    | "STANDARD_ROOM_EXIT"
                    | "STANDARD_TAIL_ENTER"
                    | "STANDARD_TAIL_EXIT"
                    | "ENTITY_LIST_ENTER"
                    | "ENTITY_LIST_EXIT"
            )
        });
        write_fixture_events(&run, &mut outcome_only);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-evidence-nonempty"));
        assert!(check_passed(&report, "authority-transaction-order"));

        with_outcomes.retain(|event| {
            !matches!(
                event.kind.as_str(),
                "ALTERNATE_OUTCOME_ENTER" | "ALTERNATE_OUTCOME_EXIT"
            )
        });
        write_fixture_events(&run, &mut with_outcomes);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "authority-transaction-order"));
    }

    #[test]
    fn analyzer_accepts_observed_generic_peek_return_and_rejects_old_guess() {
        assert!(main_peek_return_site("0x0065E68C"));
        assert!(!main_peek_return_site("0x0065E690"));

        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        let mut events = read_fixture_events(&run);
        for event in events
            .iter_mut()
            .filter(|event| event.kind == "PEEK_MESSAGE")
        {
            event.return_rva = "0x0065E68C".to_owned();
        }
        write_fixture_events(&run, &mut events);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "PASS");
        assert!(check_passed(&report, "message-pump-owner"));

        for event in events
            .iter_mut()
            .filter(|event| event.kind == "PEEK_MESSAGE")
        {
            event.return_rva = "0x0065E690".to_owned();
        }
        write_fixture_events(&run, &mut events);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "message-pump-owner"));
    }

    #[test]
    fn analyzer_accepts_only_observed_device_switch_direct_refresh_shapes() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        set_fixture_scenario(&run, "device-switch");
        let baseline = read_fixture_events(&run);
        let insertion = baseline
            .iter()
            .position(|event| event.kind == "CONDITIONAL_ATTEMPT_ENTER")
            .unwrap();
        let template = baseline[insertion].clone();
        let mut events = baseline.clone();
        events.splice(
            insertion..insertion,
            device_switch_refresh_transaction(&template),
        );
        write_fixture_events(&run, &mut events);

        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "PASS");
        assert!(check_passed(&report, "event-contract"));
        assert!(check_passed(&report, "balanced-depth"));
        assert!(check_passed(&report, "scenario-evidence"));

        let mut post_manager_events = baseline.clone();
        post_manager_events.splice(
            insertion..insertion,
            device_switch_post_manager_refresh_transaction(&template),
        );
        write_fixture_events(&run, &mut post_manager_events);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "PASS");
        assert!(check_passed(&report, "event-contract"));
        assert!(check_passed(&report, "balanced-depth"));
        assert!(check_passed(&report, "scenario-evidence"));

        let mut wrong_post_manager_peek = device_switch_post_manager_refresh_transaction(&template);
        wrong_post_manager_peek[14].return_rva = "0x0065E68D".to_owned();
        let mut wrong_post_manager_events = baseline.clone();
        wrong_post_manager_events.splice(insertion..insertion, wrong_post_manager_peek);
        write_fixture_events(&run, &mut wrong_post_manager_events);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(check_passed(&report, "event-contract"));
        assert!(!check_passed(&report, "balanced-depth"));
        assert!(!check_passed(&report, "scenario-evidence"));

        let mut wrong_follow_up = events.clone();
        wrong_follow_up
            .iter_mut()
            .find(|event| event.return_rva == "0x0065E6F0")
            .unwrap()
            .return_rva = "0x0065E6F1".to_owned();
        write_fixture_events(&run, &mut wrong_follow_up);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(check_passed(&report, "event-contract"));
        assert!(!check_passed(&report, "balanced-depth"));
        assert!(!check_passed(&report, "scenario-evidence"));

        set_fixture_scenario(&run, "keyboard");
        write_fixture_events(&run, &mut events);
        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "balanced-depth"));
    }

    #[test]
    fn pause_gap_requires_brackets_continuing_passive_envelope_and_no_lifecycle() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        let template = read_fixture_events(&run)
            .into_iter()
            .find(|event| event.kind == "CONDITIONAL_ATTEMPT_ENTER")
            .unwrap();
        let mut events = Vec::new();
        for iteration in 1..=100 {
            for (kind, phase, return_rva) in [
                ("CONDITIONAL_ATTEMPT_ENTER", "GAMEPLAY_UPDATE", "0x00555236"),
                ("INPUT_PHYSICAL_POLL_ENTER", "INPUT_QUERY", "0x0061F30C"),
                ("UPDATE_ENTER", "GAMEPLAY_UPDATE", "0x005312F1"),
                ("RENDER_ENTER", "RENDER", "0x005312F6"),
            ] {
                let mut event = scope_event(&template, kind, phase, 0, return_rva, None);
                event.loop_iteration = iteration;
                events.push(event);
            }
        }
        let bracketed_transactions = (1..=30).chain(71..=100).collect::<BTreeSet<_>>();
        assert!(has_bracketed_standard_transaction_gap(
            &events,
            &bracketed_transactions
        ));

        let mut missing_poll = events.clone();
        missing_poll.retain(|event| {
            event.loop_iteration != 50 || event.kind != "INPUT_PHYSICAL_POLL_ENTER"
        });
        assert!(!has_bracketed_standard_transaction_gap(
            &missing_poll,
            &bracketed_transactions
        ));

        let mut lifecycle_mixed = events.clone();
        let mut lifecycle = scope_event(
            &template,
            "NEW_ROOM_ENTER",
            "ROOM_LIFECYCLE",
            0,
            "0x0033F5BE",
            None,
        );
        lifecycle.loop_iteration = 50;
        lifecycle_mixed.push(lifecycle);
        assert!(!has_bracketed_standard_transaction_gap(
            &lifecycle_mixed,
            &bracketed_transactions
        ));

        let trailing_only = (1..=60).collect::<BTreeSet<_>>();
        assert!(!has_bracketed_standard_transaction_gap(
            &events,
            &trailing_only
        ));
    }

    #[test]
    fn analyzer_rejects_phase_flag_and_payload_forgery() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        let original = read_fixture_events(&run);
        let input_index = original
            .iter()
            .position(|event| event.kind == "INPUT_SCALAR")
            .unwrap();

        for mutate in 0..5 {
            let mut events = original.clone();
            match mutate {
                0 => events[input_index].phase = "RENDER".to_owned(),
                1 => events[input_index].flags |= 1 << 31,
                2 => events[input_index].device_kind = Some("GAMEPAD".to_owned()),
                3 => events[input_index].return_rva = "0x00000000".to_owned(),
                4 => events[input_index].thread_id = 0,
                _ => unreachable!(),
            }
            write_fixture_events(&run, &mut events);
            let report = analyze_loop_run_internal(&run).unwrap();
            assert_eq!(report.result, "FAIL");
            assert!(!check_passed(&report, "event-contract"));
        }
    }

    #[test]
    fn analyzer_rejects_out_of_order_native_lifecycle() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        let mut events = read_fixture_events(&run);
        events.swap(0, 1);
        write_fixture_events(&run, &mut events);

        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(!check_passed(&report, "native-lifecycle"));
    }

    #[test]
    fn authority_mutation_is_allowed_inside_room_lifecycle() {
        let directory = tempfile::tempdir().unwrap();
        let run = write_keyboard_loop_fixture(directory.path());
        let mut events = read_fixture_events(&run);
        events.retain(|event| !matches!(event.kind.as_str(), "ENTITY_ENTER" | "ENTITY_EXIT"));
        let restored = events.pop().unwrap();
        let base_flags = LOOP_FLAG_LOOP_KNOWN | LOOP_FLAG_MAIN_THREAD;
        let make = |kind: &str, phase: &str, original_calls: u32, return_rva: &str| LoopEvent {
            sequence: 0,
            native_sequence: 0,
            qpc: 0,
            loop_iteration: 30,
            phase: phase.to_owned(),
            kind: kind.to_owned(),
            thread_id: 7,
            action: None,
            value_bits: None,
            flags: base_flags
                | if original_calls == 1 {
                    LOOP_FLAG_ORIGINAL_CALLED
                } else {
                    0
                },
            return_rva: return_rva.to_owned(),
            original_calls,
            error_code: 0,
            device_key: None,
            device_kind: None,
            install_failure_stage: None,
            install_failure_index: None,
            install_failure_win32_error: None,
        };
        events.extend([
            make("NEW_ROOM_ENTER", "ROOM_LIFECYCLE", 0, "0x0033F5BE"),
            make("ENTITY_ENTER", "ENTITY_MUTATION", 0, "0x002FB521"),
            make("ENTITY_EXIT", "ENTITY_MUTATION", 1, "0x002FB521"),
            make("NEW_ROOM_EXIT", "ROOM_LIFECYCLE", 1, "0x0033F5BE"),
            restored,
        ]);
        write_fixture_events(&run, &mut events);

        let report = analyze_loop_run_internal(&run).unwrap();
        assert_eq!(report.result, "PASS");
        assert!(check_passed(&report, "authority-owned"));
        assert!(check_passed(&report, "authority-evidence-nonempty"));
    }
}
