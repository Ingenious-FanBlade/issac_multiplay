use std::collections::{BTreeMap, BTreeSet};
use std::fs::{self, OpenOptions};
use std::io::{BufWriter, Write};
use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};

use crate::artifact::{file_sha256, utc_now};
use crate::coop_device_rebind as device;
use crate::coop_passive_ipc::{
    NativePassiveRecord, PASSIVE_COMPLETE_HOOK_MASK, PASSIVE_FLAG_ASSIGNMENT_MATCH,
    PASSIVE_FLAG_BINDING_UNIQUE, PASSIVE_FLAG_HUD_MATCH, PASSIVE_FLAG_LUA_VECTOR_EQUIVALENT,
    PASSIVE_FLAG_MAIN_THREAD, PASSIVE_FLAG_ORIGINAL_CALLED, PASSIVE_FLAG_RELATION_CHANGED,
    PASSIVE_FLAG_STABLE_REGISTRY_RETIRED, PASSIVE_FLAG_TOPOLOGY_COHERENT, PASSIVE_NO_ORDINAL,
    PASSIVE_SLOT_COUNT, PassiveEventKind,
};
use crate::error::{AppError, AppResult, UsageContext};

pub const PASSIVE_SCHEMA_VERSION: u32 = 1;
pub const PASSIVE_EVIDENCE_CLASS: &str = "COOP_PASSIVE_LIFECYCLE";
pub const PASSIVE_TOOL_IDENTITY: &str = "IsaacCsT0-CoopPassiveObserver-v1";
pub const PASSIVE_ANALYSIS_IDENTITY: &str = "IsaacCsT0-CoopPassiveAnalysis-v1";
pub const PASSIVE_GATE_IDENTITY: &str = "IsaacCsT0-CoopPassiveDecision-v1";
pub const PASSIVE_WORK_PACKAGE: &str = "CS-T0-05-03";
pub const PASSIVE_PROBE: &str = "passive";
pub const PASSIVE_DEFAULT_DURATION_MILLISECONDS: u64 = 180_000;
pub const PASSIVE_MIN_DURATION_MILLISECONDS: u64 = 10_000;
pub const PASSIVE_MAX_DURATION_MILLISECONDS: u64 = 600_000;

pub const SCENARIO_NATIVE_JOIN: &str = "native-join-passive";
pub const SCENARIO_NATIVE_EXIT: &str = "native-exit-passive";
pub const SCENARIO_DEATH_RESPAWN: &str = "death-respawn-passive";
pub const SCENARIO_ROOM_REBIND: &str = "room-rebind-passive";
pub const SCENARIO_DEVICE_REBIND: &str = "device-rebind-passive";
pub const SCENARIO_MENU_TEARDOWN: &str = "menu-teardown-passive";
pub const PASSIVE_SCENARIOS: [&str; 6] = [
    SCENARIO_NATIVE_JOIN,
    SCENARIO_NATIVE_EXIT,
    SCENARIO_DEATH_RESPAWN,
    SCENARIO_ROOM_REBIND,
    SCENARIO_DEVICE_REBIND,
    SCENARIO_MENU_TEARDOWN,
];

pub const STATE_UNBOUND: &str = "UNBOUND";
pub const STATE_CANDIDATE: &str = "CANDIDATE";
pub const STATE_BOUND: &str = "BOUND";
pub const STATE_RETIRED: &str = "RETIRED";
pub const ENDPOINT_A: &str = "ENDPOINT_A";
pub const ENDPOINT_B: &str = "ENDPOINT_B";

const EVENT_KINDS: [&str; 32] = [
    "NATIVE_INITIALIZED",
    "HOOKS_INSTALLED",
    "MANAGER_PASS_BEGIN",
    "MANAGER_PASS_END",
    "ORDINARY_SPAWN_BEGIN",
    "ORDINARY_SPAWN_END",
    "POSSESSOR_SPAWN_STOP",
    "EXIT_BEGIN",
    "EXIT_RETURN_STABLE",
    "ORDINARY_ASSIGNMENT_BEGIN",
    "ORDINARY_ASSIGNMENT_END",
    "POSSESSOR_ASSIGNMENT_STOP",
    "JOIN_HUD_BEGIN",
    "JOIN_HUD_END",
    "EXIT_HUD_BEGIN",
    "EXIT_HUD_END",
    "BINDING_UNBOUND",
    "BINDING_CANDIDATE",
    "BINDING_BOUND",
    "BINDING_RETIRED",
    "LUA_VECTOR_EQUIVALENT",
    "LIFECYCLE_ENTER",
    "LIFECYCLE_EXIT",
    "RELATION_CHANGE",
    "HOOKS_RESTORED",
    "PRODUCER_CLOSED",
    "FAILURE",
    "DEVICE_UNASSIGN_BEGIN",
    "DEVICE_UNASSIGN_END",
    "DEVICE_REASSIGN_BEGIN",
    "DEVICE_REASSIGN_END",
    "UNCLASSIFIED_RELATION_CHANGE",
];

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct PassiveTargetSlot {
    pub ordinal: u32,
    pub id: String,
    pub patch_kind: String,
    pub rva: String,
    pub expected_proof: String,
    pub before_proof: String,
    pub after_proof: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct PassiveManifest {
    pub schema_version: u32,
    pub evidence_class: String,
    pub tool_identity: String,
    pub artifact_identity: String,
    pub work_package: String,
    pub case_id: String,
    pub probe: String,
    pub scenario: String,
    pub started_utc: String,
    pub ended_utc: String,
    pub state: String,
    pub failure_reason: Option<String>,
    pub target_sha256: String,
    pub observed_target_sha256: String,
    pub target_equivalence_sha256: String,
    pub layout_evidence_sha256: String,
    pub coop_catalog_sha256: String,
    pub coop_graph_sha256: String,
    pub coop_plan_sha256: String,
    pub coop_verification_sha256: String,
    pub native_sha256: String,
    pub helper_sha256: String,
    pub process_identity_token: String,
    pub events_sha256: String,
    pub slot_count: u32,
    pub installed_hooks_mask: String,
    pub restored_hooks_mask: String,
    pub restored_slot_count: u32,
    pub install_order: Vec<u32>,
    pub restore_order: Vec<u32>,
    pub target_slots: Vec<PassiveTargetSlot>,
    pub slot_snapshot_before_sha256: String,
    pub slot_snapshot_after_sha256: String,
    pub main_thread_id_token: String,
    pub final_owner_generation: u64,
    pub overwritten_events: u64,
    pub callbacks_drained: bool,
    pub producer_closed: bool,
    pub unsafe_unload_required: bool,
    pub stable_registry_retired: bool,
    pub override_count: u64,
    pub apply_count: u64,
    pub world_write_count: u64,
    pub scheduler_count: u64,
    pub transport_count: u64,
    pub mode_latch_write_count: u64,
    pub input_mutation_allowed: bool,
    pub world_mutation_allowed: bool,
    pub active_input_allowed: bool,
    pub original_online_positive_allowed: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct PassiveEvent {
    pub schema_version: u32,
    pub sequence: u64,
    pub qpc: i64,
    pub loop_iteration: u64,
    pub kind: String,
    pub slot_ordinal: Option<u32>,
    pub owner_thread_token: String,
    pub main_thread: bool,
    pub phase: u32,
    pub depth: u32,
    pub owner_generation: u64,
    pub local_ordinal: Option<u32>,
    pub endpoint: Option<String>,
    pub topology_digest: Option<String>,
    pub player_digest: Option<String>,
    pub device_digest: Option<String>,
    pub assignment_digest: Option<String>,
    pub topology_coherent: bool,
    pub binding_unique: bool,
    pub assignment_matches: bool,
    pub hud_matches: bool,
    pub lua_vector_equivalent: bool,
    pub relation_changed: bool,
    pub stable_registry_retired: bool,
    pub binding_state: Option<String>,
    pub original_calls: u32,
    pub original_called: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct PassiveCheck {
    pub id: String,
    pub passed: bool,
    pub detail: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct PassiveAnalysis {
    pub schema_version: u32,
    pub analysis_identity: String,
    pub work_package: String,
    pub artifact_identity: String,
    pub scenario: String,
    pub result: String,
    pub manifest_sha256: String,
    pub events_sha256: String,
    pub event_count: usize,
    pub final_owner_generation: u64,
    pub checks: Vec<PassiveCheck>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct PassiveGateSource {
    pub scenario: String,
    pub artifact_identity: String,
    pub source_path: String,
    pub manifest_sha256: String,
    pub events_sha256: String,
    pub analysis_sha256: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct PassiveGateDecision {
    pub schema_version: u32,
    pub decision_identity: String,
    pub work_package: String,
    pub created_utc: String,
    pub status: String,
    pub sources: Vec<PassiveGateSource>,
    pub checks: Vec<PassiveCheck>,
    pub cs_t00504_unlocked: bool,
    pub active_input_allowed: bool,
    pub passive_hook_transaction_allowed: bool,
    pub runtime_mutation_allowed: bool,
    pub t005_unlocked: bool,
}

#[derive(Clone, Debug)]
pub struct PassiveRunConfig {
    pub case_id: String,
    pub scenario: String,
    pub started_utc: String,
    pub target_sha256: String,
    pub observed_target_sha256: String,
    pub target_equivalence_sha256: String,
    pub layout_evidence_sha256: String,
    pub coop_catalog_sha256: String,
    pub coop_graph_sha256: String,
    pub coop_plan_sha256: String,
    pub coop_verification_sha256: String,
    pub native_sha256: String,
    pub helper_sha256: String,
    pub process_identity_token: String,
    pub main_thread_id_token: String,
    pub target_slots: Vec<PassiveTargetSlot>,
}

#[derive(Clone, Debug)]
pub struct PassiveRunFinish {
    pub state: String,
    pub failure_reason: Option<String>,
    pub installed_hooks_mask: u64,
    pub restored_hooks_mask: u64,
    pub restored_slot_count: u32,
    pub slot_snapshot_before_sha256: String,
    pub slot_snapshot_after_sha256: String,
    pub final_owner_generation: u64,
    pub overwritten_events: u64,
    pub callbacks_drained: bool,
    pub producer_closed: bool,
    pub unsafe_unload_required: bool,
    pub stable_registry_retired: bool,
    pub zero_capability_counts: [u64; 6],
}

pub fn validate_passive_scenario(scenario: &str) -> AppResult<()> {
    if PASSIVE_SCENARIOS.contains(&scenario) {
        Ok(())
    } else {
        Err(AppError::usage(format!(
            "unsupported coop passive scenario {scenario}"
        )))
    }
}

pub fn validate_passive_capture_options(
    probe: &str,
    scenario: &str,
    case_id: &str,
    duration_milliseconds: u64,
) -> AppResult<()> {
    if probe != PASSIVE_PROBE {
        return Err(AppError::usage(
            "coop passive capture requires --probe passive",
        ));
    }
    validate_passive_scenario(scenario)?;
    if case_id.is_empty()
        || case_id.len() > 96
        || !case_id
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_'))
    {
        return Err(AppError::usage(
            "coop passive case id must be a simple nonempty identifier",
        ));
    }
    if !(PASSIVE_MIN_DURATION_MILLISECONDS..=PASSIVE_MAX_DURATION_MILLISECONDS)
        .contains(&duration_milliseconds)
    {
        return Err(AppError::usage(format!(
            "coop passive duration must be {}..={} milliseconds",
            PASSIVE_MIN_DURATION_MILLISECONDS, PASSIVE_MAX_DURATION_MILLISECONDS
        )));
    }
    Ok(())
}

pub fn write_passive_run(
    directory: &Path,
    config: PassiveRunConfig,
    records: &[NativePassiveRecord],
    finish: PassiveRunFinish,
) -> AppResult<PassiveAnalysis> {
    validate_passive_capture_options(
        PASSIVE_PROBE,
        &config.scenario,
        &config.case_id,
        PASSIVE_DEFAULT_DURATION_MILLISECONDS,
    )?;
    fs::create_dir(directory).usage_context(format!(
        "cannot create new coop passive source {}",
        directory.display()
    ))?;
    let version = if config.scenario == SCENARIO_DEVICE_REBIND {
        2
    } else {
        1
    };
    let slot_count = if version == 2 { 56 } else { PASSIVE_SLOT_COUNT };
    let events = records
        .iter()
        .map(|record| convert_native_record(record, version, slot_count))
        .collect::<AppResult<Vec<_>>>()?;
    let events_path = directory.join("events.jsonl");
    write_events_new(&events_path, &events)?;
    let events_sha256 = file_sha256(&events_path)?;
    let artifact_identity = format!(
        "coop-passive-{}",
        file_identity(&config.case_id, &config.scenario, &events_sha256)
    );
    let install_order = (0..slot_count).collect::<Vec<_>>();
    let restore_order = (0..slot_count).rev().collect::<Vec<_>>();
    let manifest = PassiveManifest {
        schema_version: version,
        evidence_class: PASSIVE_EVIDENCE_CLASS.to_owned(),
        tool_identity: if version == 2 {
            device::TOOL_IDENTITY
        } else {
            PASSIVE_TOOL_IDENTITY
        }
        .to_owned(),
        artifact_identity,
        work_package: PASSIVE_WORK_PACKAGE.to_owned(),
        case_id: config.case_id,
        probe: PASSIVE_PROBE.to_owned(),
        scenario: config.scenario,
        started_utc: config.started_utc,
        ended_utc: utc_now(),
        state: finish.state,
        failure_reason: finish.failure_reason,
        target_sha256: config.target_sha256,
        observed_target_sha256: config.observed_target_sha256,
        target_equivalence_sha256: config.target_equivalence_sha256,
        layout_evidence_sha256: config.layout_evidence_sha256,
        coop_catalog_sha256: config.coop_catalog_sha256,
        coop_graph_sha256: config.coop_graph_sha256,
        coop_plan_sha256: config.coop_plan_sha256,
        coop_verification_sha256: config.coop_verification_sha256,
        native_sha256: config.native_sha256,
        helper_sha256: config.helper_sha256,
        process_identity_token: config.process_identity_token,
        events_sha256,
        slot_count,
        installed_hooks_mask: format!("0x{:016X}", finish.installed_hooks_mask),
        restored_hooks_mask: format!("0x{:016X}", finish.restored_hooks_mask),
        restored_slot_count: finish.restored_slot_count,
        install_order,
        restore_order,
        target_slots: config.target_slots,
        slot_snapshot_before_sha256: finish.slot_snapshot_before_sha256,
        slot_snapshot_after_sha256: finish.slot_snapshot_after_sha256,
        main_thread_id_token: config.main_thread_id_token,
        final_owner_generation: finish.final_owner_generation,
        overwritten_events: finish.overwritten_events,
        callbacks_drained: finish.callbacks_drained,
        producer_closed: finish.producer_closed,
        unsafe_unload_required: finish.unsafe_unload_required,
        stable_registry_retired: finish.stable_registry_retired,
        override_count: finish.zero_capability_counts[0],
        apply_count: finish.zero_capability_counts[1],
        world_write_count: finish.zero_capability_counts[2],
        scheduler_count: finish.zero_capability_counts[3],
        transport_count: finish.zero_capability_counts[4],
        mode_latch_write_count: finish.zero_capability_counts[5],
        input_mutation_allowed: false,
        world_mutation_allowed: false,
        active_input_allowed: false,
        original_online_positive_allowed: false,
    };
    write_json_new(&directory.join("manifest.json"), &manifest)?;
    let analysis = analyze_passive_run(directory)?;
    write_json_new(&directory.join("analysis.json"), &analysis)?;
    Ok(analysis)
}

pub fn analyze_passive_run(directory: &Path) -> AppResult<PassiveAnalysis> {
    let manifest_path = directory.join("manifest.json");
    let events_path = directory.join("events.jsonl");
    let manifest: PassiveManifest = read_json(&manifest_path, "coop passive manifest")?;
    let events = read_events(&events_path)?;
    let manifest_sha256 = file_sha256(&manifest_path)?;
    let events_sha256 = file_sha256(&events_path)?;
    let v2 = device::is_v2(&manifest);
    let slot_count = if v2 { 56 } else { PASSIVE_SLOT_COUNT };
    let mask = if v2 {
        0x00FF_FFFF_FFFF_FFFF
    } else {
        PASSIVE_COMPLETE_HOOK_MASK
    };
    let mut checks = Vec::new();
    let mut check = |id: &str, passed: bool, detail: &str| {
        checks.push(PassiveCheck {
            id: id.to_owned(),
            passed,
            detail: detail.to_owned(),
        });
    };

    check(
        "manifest-identity",
        (v2 || (manifest.schema_version == PASSIVE_SCHEMA_VERSION
            && manifest.tool_identity == PASSIVE_TOOL_IDENTITY))
            && manifest.evidence_class == PASSIVE_EVIDENCE_CLASS
            && manifest.work_package == PASSIVE_WORK_PACKAGE
            && manifest.probe == PASSIVE_PROBE
            && validate_passive_scenario(&manifest.scenario).is_ok()
            && !manifest.artifact_identity.is_empty()
            && manifest.state == "PASS"
            && manifest.failure_reason.is_none(),
        "exact passive source identity and PASS terminal state",
    );
    check(
        "events-hash-binding",
        manifest.events_sha256 == events_sha256
            && [
                &manifest.target_sha256,
                &manifest.observed_target_sha256,
                &manifest.target_equivalence_sha256,
                &manifest.layout_evidence_sha256,
                &manifest.coop_catalog_sha256,
                &manifest.coop_graph_sha256,
                &manifest.coop_plan_sha256,
                &manifest.coop_verification_sha256,
                &manifest.native_sha256,
                &manifest.helper_sha256,
                &manifest.process_identity_token,
                &manifest.events_sha256,
                &manifest.slot_snapshot_before_sha256,
                &manifest.slot_snapshot_after_sha256,
                &manifest.main_thread_id_token,
            ]
            .iter()
            .all(|value| is_nonzero_sha256(value))
            && manifest.target_slots.iter().all(|slot| {
                is_nonzero_sha256(&slot.expected_proof)
                    && is_nonzero_sha256(&slot.before_proof)
                    && is_nonzero_sha256(&slot.after_proof)
            }),
        "events and every persisted identity/proof have exact nonzero SHA-256 bindings",
    );
    check(
        "privacy-and-zero-capability",
        privacy_passes(directory)?
            && !manifest.input_mutation_allowed
            && !manifest.world_mutation_allowed
            && !manifest.active_input_allowed
            && !manifest.original_online_positive_allowed
            && [
                manifest.override_count,
                manifest.apply_count,
                manifest.world_write_count,
                manifest.scheduler_count,
                manifest.transport_count,
                manifest.mode_latch_write_count,
            ] == [0; 6],
        "source contains no active capability or forbidden identity material",
    );
    let exact_order = manifest.install_order == (0..slot_count).collect::<Vec<_>>()
        && manifest.restore_order == (0..slot_count).rev().collect::<Vec<_>>();
    let exact_slots = manifest.target_slots.len() == slot_count as usize
        && manifest
            .target_slots
            .iter()
            .enumerate()
            .all(|(index, slot)| {
                slot.ordinal == index as u32
                    && slot.expected_proof == slot.before_proof
                    && slot.before_proof == slot.after_proof
            });
    check(
        if v2 {
            "ordered-56-slot-transaction"
        } else {
            "ordered-53-slot-transaction"
        },
        manifest.slot_count == slot_count
            && parse_mask(&manifest.installed_hooks_mask) == Some(mask)
            && parse_mask(&manifest.restored_hooks_mask) == Some(mask)
            && manifest.restored_slot_count == slot_count
            && exact_order
            && exact_slots
            && manifest.slot_snapshot_before_sha256.len() == 64
            && manifest.slot_snapshot_before_sha256 == manifest.slot_snapshot_after_sha256,
        if v2 {
            "all 56 slots install in ordinal order and restore exactly in reverse"
        } else {
            "all 53 slots install in ordinal order and restore exactly in reverse"
        },
    );
    check(
        "ring-drain-producer-close",
        manifest.overwritten_events == 0
            && manifest.callbacks_drained
            && manifest.producer_closed
            && !manifest.unsafe_unload_required
            && events.iter().any(|event| event.kind == "HOOKS_RESTORED")
            && events
                .last()
                .is_some_and(|event| event.kind == "PRODUCER_CLOSED"),
        "ring is contiguous, callbacks drain, hooks restore and producer closes last",
    );

    let structural = validate_event_structure(&events, if v2 { 2 } else { 1 });
    check(
        "event-structure-original-once",
        structural.is_ok(),
        structural
            .as_ref()
            .err()
            .map(String::as_str)
            .unwrap_or("event structure and original-once are exact"),
    );
    let bindings = validate_binding_machine(&events, &manifest.scenario);
    check(
        "generation-ordinal-binding",
        bindings.is_ok(),
        bindings
            .as_ref()
            .err()
            .map(String::as_str)
            .unwrap_or("generation-local bindings are unique and fail closed"),
    );
    let scenario = validate_scenario_semantics(&events, &manifest);
    check(
        "scenario-contract",
        scenario.is_ok(),
        scenario
            .as_ref()
            .err()
            .map(String::as_str)
            .unwrap_or("scenario-specific lifecycle contract is closed"),
    );
    let result = if checks.iter().all(|item| item.passed) {
        "PASS"
    } else {
        "BLOCKED_BY_EVIDENCE"
    };
    Ok(PassiveAnalysis {
        schema_version: if v2 { 2 } else { PASSIVE_SCHEMA_VERSION },
        analysis_identity: if v2 {
            device::ANALYSIS_IDENTITY
        } else {
            PASSIVE_ANALYSIS_IDENTITY
        }
        .to_owned(),
        work_package: PASSIVE_WORK_PACKAGE.to_owned(),
        artifact_identity: manifest.artifact_identity,
        scenario: manifest.scenario,
        result: result.to_owned(),
        manifest_sha256,
        events_sha256,
        event_count: events.len(),
        final_owner_generation: manifest.final_owner_generation,
        checks,
    })
}

pub fn is_passive_run(directory: &Path) -> AppResult<bool> {
    let path = directory.join("manifest.json");
    let value: serde_json::Value = serde_json::from_slice(
        &fs::read(&path).usage_context(format!("cannot read coop manifest {}", path.display()))?,
    )
    .usage_context("cannot parse coop manifest identity")?;
    Ok(value
        .get("evidenceClass")
        .and_then(serde_json::Value::as_str)
        == Some(PASSIVE_EVIDENCE_CLASS)
        && value
            .get("toolIdentity")
            .and_then(serde_json::Value::as_str)
            .is_some_and(|identity| {
                [PASSIVE_TOOL_IDENTITY, device::TOOL_IDENTITY].contains(&identity)
            }))
}

pub fn replay_passive_run(directory: &Path) -> AppResult<PassiveAnalysis> {
    let before = recursive_file_hashes(directory)?;
    let fresh = analyze_passive_run(directory)?;
    let stored: PassiveAnalysis = read_json(&directory.join("analysis.json"), "passive analysis")?;
    if stored != fresh {
        return Err(AppError::evidence(
            "stored coop passive analysis differs from fresh replay",
        ));
    }
    if before != recursive_file_hashes(directory)? {
        return Err(AppError::evidence(
            "coop passive replay modified its source directory",
        ));
    }
    Ok(fresh)
}

pub fn analyze_passive_run_to(directory: &Path, output: &Path) -> AppResult<PassiveAnalysis> {
    refuse_existing(output)?;
    let analysis = analyze_passive_run(directory)?;
    write_json_new(output, &analysis)?;
    Ok(analysis)
}

pub fn passive_gate_to(sources: [&Path; 6], output: &Path) -> AppResult<PassiveGateDecision> {
    refuse_existing(output)?;
    let mut gate_sources = Vec::new();
    let mut checks = Vec::new();
    let mut identities = BTreeSet::new();
    let mut frozen_bindings: Option<Vec<String>> = None;
    let mut foundational_bindings: Option<Vec<String>> = None;
    for (expected, directory) in PASSIVE_SCENARIOS.iter().zip(sources) {
        let analysis = replay_passive_run(directory)?;
        let manifest: PassiveManifest =
            read_json(&directory.join("manifest.json"), "coop passive manifest")?;
        let scenario_ok = analysis.scenario == *expected
            && analysis.result == "PASS"
            && (*expected != SCENARIO_DEVICE_REBIND || device::is_v2(&manifest));
        let analysis_sha256 = file_sha256(&directory.join("analysis.json"))?;
        let approved_legacy = device::approved_legacy(
            expected,
            &analysis.manifest_sha256,
            &analysis.events_sha256,
            &analysis_sha256,
        );
        checks.push(PassiveCheck {
            id: format!("source-{expected}"),
            passed: scenario_ok,
            detail: "exact scenario source replays PASS".to_owned(),
        });
        let distinct = identities.insert(analysis.artifact_identity.clone());
        checks.push(PassiveCheck {
            id: format!("distinct-{expected}"),
            passed: distinct,
            detail: "source artifact identity is unique".to_owned(),
        });
        let foundation = vec![
            manifest.target_sha256,
            manifest.observed_target_sha256,
            manifest.target_equivalence_sha256,
            manifest.layout_evidence_sha256,
            manifest.coop_graph_sha256,
        ];
        let foundation_match = foundational_bindings
            .as_ref()
            .is_none_or(|frozen| frozen == &foundation);
        if foundational_bindings.is_none() {
            foundational_bindings = Some(foundation);
        }
        let bindings = vec![
            manifest.coop_catalog_sha256,
            manifest.coop_plan_sha256,
            manifest.coop_verification_sha256,
            manifest.native_sha256,
            manifest.helper_sha256,
        ];
        let bindings_match = foundation_match
            && (approved_legacy
                || frozen_bindings
                    .as_ref()
                    .is_none_or(|frozen| frozen == &bindings));
        if !approved_legacy && frozen_bindings.is_none() {
            frozen_bindings = Some(bindings);
        }
        checks.push(PassiveCheck {
            id: format!("frozen-bindings-{expected}"),
            passed: bindings_match,
            detail: "common target/layout/graph; exact producer bundle or immutable approved historical source triple".to_owned(),
        });
        gate_sources.push(PassiveGateSource {
            scenario: analysis.scenario.clone(),
            artifact_identity: analysis.artifact_identity,
            source_path: normalize_source_path(directory),
            manifest_sha256: analysis.manifest_sha256,
            events_sha256: analysis.events_sha256,
            analysis_sha256,
        });
    }
    let pass =
        checks.iter().all(|check| check.passed) && gate_sources.len() == PASSIVE_SCENARIOS.len();
    let decision = PassiveGateDecision {
        schema_version: 2,
        decision_identity: device::GATE_IDENTITY.to_owned(),
        work_package: PASSIVE_WORK_PACKAGE.to_owned(),
        created_utc: utc_now(),
        status: if pass { "PASS" } else { "BLOCKED_BY_EVIDENCE" }.to_owned(),
        sources: gate_sources,
        checks,
        cs_t00504_unlocked: pass,
        active_input_allowed: false,
        passive_hook_transaction_allowed: false,
        runtime_mutation_allowed: false,
        t005_unlocked: false,
    };
    write_json_new(output, &decision)?;
    Ok(decision)
}

fn convert_native_record(
    record: &NativePassiveRecord,
    version: u32,
    slot_count: u32,
) -> AppResult<PassiveEvent> {
    let native = &record.event;
    let kind = event_kind_name(PassiveEventKind::from_raw(native.kind)?).to_owned();
    if (version == 1 && native.kind > 27)
        || (native.slot_ordinal != PASSIVE_NO_ORDINAL && native.slot_ordinal >= slot_count)
    {
        return Err(AppError::evidence(
            "native passive event is outside frozen profile",
        ));
    }
    let endpoint = match native.endpoint {
        0 => None,
        1 => Some(ENDPOINT_A.to_owned()),
        2 => Some(ENDPOINT_B.to_owned()),
        value => {
            return Err(AppError::evidence(format!(
                "coop passive event has unknown endpoint {value}"
            )));
        }
    };
    let local_ordinal =
        (native.local_ordinal != PASSIVE_NO_ORDINAL).then_some(native.local_ordinal);
    let binding_state = match native.kind {
        17 => Some(STATE_UNBOUND.to_owned()),
        18 => Some(STATE_CANDIDATE.to_owned()),
        19 => Some(STATE_BOUND.to_owned()),
        20 => Some(STATE_RETIRED.to_owned()),
        _ => None,
    };
    Ok(PassiveEvent {
        schema_version: version,
        sequence: record.sequence,
        qpc: native.qpc,
        loop_iteration: native.loop_iteration,
        kind,
        slot_ordinal: (native.slot_ordinal != PASSIVE_NO_ORDINAL).then_some(native.slot_ordinal),
        owner_thread_token: token_u32(native.thread_id, b"thread"),
        main_thread: native.flags & PASSIVE_FLAG_MAIN_THREAD != 0
            && native.thread_id == native.main_thread_id
            && native.thread_id != 0,
        phase: native.phase,
        depth: native.depth,
        owner_generation: native.owner_generation,
        local_ordinal,
        endpoint,
        topology_digest: optional_digest(native.topology_digest),
        player_digest: optional_digest(native.player_digest),
        device_digest: optional_digest(native.device_digest),
        assignment_digest: optional_digest(native.assignment_digest),
        topology_coherent: native.flags & PASSIVE_FLAG_TOPOLOGY_COHERENT != 0,
        binding_unique: native.flags & PASSIVE_FLAG_BINDING_UNIQUE != 0,
        assignment_matches: native.flags & PASSIVE_FLAG_ASSIGNMENT_MATCH != 0,
        hud_matches: native.flags & PASSIVE_FLAG_HUD_MATCH != 0,
        lua_vector_equivalent: native.flags & PASSIVE_FLAG_LUA_VECTOR_EQUIVALENT != 0,
        relation_changed: native.flags & PASSIVE_FLAG_RELATION_CHANGED != 0,
        stable_registry_retired: native.flags & PASSIVE_FLAG_STABLE_REGISTRY_RETIRED != 0,
        binding_state,
        original_calls: native.original_calls,
        original_called: native.flags & PASSIVE_FLAG_ORIGINAL_CALLED != 0,
    })
}

fn validate_event_structure(events: &[PassiveEvent], version: u32) -> Result<(), String> {
    if events.is_empty() {
        return Err("passive event stream is empty".to_owned());
    }
    let terminal = [
        "NATIVE_INITIALIZED",
        "HOOKS_INSTALLED",
        "HOOKS_RESTORED",
        "PRODUCER_CLOSED",
    ];
    let completed_original = [
        "MANAGER_PASS_END",
        "ORDINARY_SPAWN_END",
        "EXIT_RETURN_STABLE",
        "ORDINARY_ASSIGNMENT_END",
        "JOIN_HUD_END",
        "EXIT_HUD_END",
        "DEVICE_UNASSIGN_END",
        "DEVICE_REASSIGN_END",
    ];
    let mut previous_qpc = i64::MIN;
    let mut stack = Vec::<&str>::new();
    for (index, event) in events.iter().enumerate() {
        if event.schema_version != version || event.sequence != index as u64 {
            return Err("passive event schema or sequence is not contiguous".to_owned());
        }
        if !EVENT_KINDS[..if version == 2 { 32 } else { 27 }].contains(&event.kind.as_str())
            || event.qpc < previous_qpc
        {
            return Err("passive event codebook is unknown or QPC regressed".to_owned());
        }
        previous_qpc = event.qpc;
        if !terminal.contains(&event.kind.as_str()) && !event.main_thread {
            return Err("passive wrapper event is not on the inherited main thread".to_owned());
        }
        if completed_original.contains(&event.kind.as_str())
            && (event.original_calls != 1 || !event.original_called)
        {
            return Err(
                "completed passive wrapper did not report original exactly once".to_owned(),
            );
        }
        if event.kind == "FAILURE"
            || event.kind == "POSSESSOR_SPAWN_STOP"
            || event.kind == "POSSESSOR_ASSIGNMENT_STOP"
        {
            return Err("passive source contains a failure or possessor stop".to_owned());
        }
        match event.kind.as_str() {
            "MANAGER_PASS_BEGIN" => stack.push("MANAGER"),
            "ORDINARY_SPAWN_BEGIN" => stack.push("SPAWN"),
            "ORDINARY_ASSIGNMENT_BEGIN" => stack.push("ASSIGN"),
            "JOIN_HUD_BEGIN" => stack.push("JOIN_HUD"),
            "EXIT_BEGIN" => stack.push("EXIT"),
            "EXIT_HUD_BEGIN" => stack.push("EXIT_HUD"),
            "LIFECYCLE_ENTER" => stack.push("LIFECYCLE"),
            "DEVICE_UNASSIGN_BEGIN" => stack.push("DEVICE_UNASSIGN"),
            "DEVICE_REASSIGN_BEGIN" => stack.push("DEVICE_REASSIGN"),
            "MANAGER_PASS_END" => pop_expected(&mut stack, "MANAGER")?,
            "ORDINARY_SPAWN_END" => pop_expected(&mut stack, "SPAWN")?,
            "ORDINARY_ASSIGNMENT_END" => pop_expected(&mut stack, "ASSIGN")?,
            "JOIN_HUD_END" => pop_expected(&mut stack, "JOIN_HUD")?,
            "EXIT_RETURN_STABLE" => pop_expected(&mut stack, "EXIT")?,
            "EXIT_HUD_END" => pop_expected(&mut stack, "EXIT_HUD")?,
            "LIFECYCLE_EXIT" => pop_expected(&mut stack, "LIFECYCLE")?,
            "DEVICE_UNASSIGN_END" => pop_expected(&mut stack, "DEVICE_UNASSIGN")?,
            "DEVICE_REASSIGN_END" => pop_expected(&mut stack, "DEVICE_REASSIGN")?,
            _ => {}
        }
    }
    if !stack.is_empty() {
        return Err("passive wrapper transaction did not close".to_owned());
    }
    Ok(())
}

fn validate_binding_machine(events: &[PassiveEvent], scenario: &str) -> Result<(), String> {
    #[derive(Clone)]
    struct Candidate {
        generation: u64,
        topology: String,
        pass_index: u64,
    }
    let mut current_generation = 0_u64;
    let mut manager_pass = 0_u64;
    let mut candidates = BTreeMap::<u32, Candidate>::new();
    let mut bound = BTreeMap::<u32, u64>::new();
    let mut retired = BTreeSet::<u32>::new();
    let mut stable_retired = false;
    for event in events {
        if event.owner_generation == 0
            && !matches!(
                event.kind.as_str(),
                "NATIVE_INITIALIZED" | "HOOKS_INSTALLED" | "HOOKS_RESTORED" | "PRODUCER_CLOSED"
            )
        {
            return Err("passive runtime event has zero generation".to_owned());
        }
        if event.owner_generation < current_generation {
            return Err("passive owner generation regressed".to_owned());
        }
        if event.owner_generation > current_generation {
            current_generation = event.owner_generation;
            candidates.clear();
            bound.retain(|_, generation| *generation == current_generation);
        }
        if event.kind == "MANAGER_PASS_END" {
            manager_pass += 1;
        }
        if event.stable_registry_retired {
            stable_retired = true;
        }
        let Some(ordinal) = event.local_ordinal else {
            continue;
        };
        if ordinal > 1 {
            return Err("passive source contains an ordinal outside {0,1}".to_owned());
        }
        let expected_endpoint = if ordinal == 0 { ENDPOINT_A } else { ENDPOINT_B };
        if event.endpoint.as_deref() != Some(expected_endpoint) {
            return Err("endpoint to ordinal mapping changed".to_owned());
        }
        match event.kind.as_str() {
            "BINDING_UNBOUND" => {
                candidates.remove(&ordinal);
                bound.remove(&ordinal);
                retired.remove(&ordinal);
            }
            "BINDING_CANDIDATE" => {
                if stable_retired
                    || event.binding_state.as_deref() != Some(STATE_CANDIDATE)
                    || !event.topology_coherent
                    || !event.binding_unique
                    || !event.assignment_matches
                    || !event.hud_matches
                    || !event.lua_vector_equivalent
                {
                    return Err("invalid passive binding candidate".to_owned());
                }
                let topology = event
                    .topology_digest
                    .clone()
                    .ok_or_else(|| "binding candidate has no topology digest".to_owned())?;
                candidates.insert(
                    ordinal,
                    Candidate {
                        generation: event.owner_generation,
                        topology,
                        pass_index: manager_pass,
                    },
                );
            }
            "BINDING_BOUND" => {
                if stable_retired || event.binding_state.as_deref() != Some(STATE_BOUND) {
                    return Err("retired or malformed binding became bound".to_owned());
                }
                let candidate = candidates
                    .remove(&ordinal)
                    .ok_or_else(|| "binding became bound without a candidate".to_owned())?;
                if candidate.generation != event.owner_generation
                    || candidate.pass_index + 1 != manager_pass
                    || event.topology_digest.as_deref() != Some(candidate.topology.as_str())
                {
                    return Err(
                        "binding did not use two consecutive coherent manager passes".to_owned(),
                    );
                }
                bound.insert(ordinal, event.owner_generation);
                retired.remove(&ordinal);
            }
            "BINDING_RETIRED" => {
                candidates.remove(&ordinal);
                bound.remove(&ordinal);
                retired.insert(ordinal);
                if event.binding_state.as_deref() != Some(STATE_RETIRED) {
                    return Err("retirement event has the wrong state".to_owned());
                }
            }
            _ => {}
        }
    }
    if scenario == SCENARIO_MENU_TEARDOWN {
        if !stable_retired || !bound.is_empty() || retired != BTreeSet::from([0, 1]) {
            return Err("menu teardown did not retire the stable registry and bindings".to_owned());
        }
    } else if stable_retired {
        return Err("non-teardown source retired the stable registry".to_owned());
    } else {
        let expected = if scenario == SCENARIO_NATIVE_EXIT {
            BTreeSet::from([0])
        } else {
            BTreeSet::from([0, 1])
        };
        let actual = bound
            .iter()
            .filter_map(|(ordinal, generation)| {
                (*generation == current_generation).then_some(*ordinal)
            })
            .collect::<BTreeSet<_>>();
        if actual != expected {
            return Err(format!(
                "final generation binding cardinality is {actual:?}, expected {expected:?}"
            ));
        }
    }
    Ok(())
}

fn validate_scenario_semantics(
    events: &[PassiveEvent],
    manifest: &PassiveManifest,
) -> Result<(), String> {
    let kinds = events
        .iter()
        .map(|event| event.kind.as_str())
        .collect::<Vec<_>>();
    let require = |required: &[&str]| -> Result<(), String> {
        if ordered_subsequence(&kinds, required) {
            Ok(())
        } else {
            Err(format!("missing ordered scenario sequence {required:?}"))
        }
    };
    match manifest.scenario.as_str() {
        SCENARIO_NATIVE_JOIN => require(&[
            "MANAGER_PASS_BEGIN",
            "ORDINARY_SPAWN_BEGIN",
            "ORDINARY_SPAWN_END",
            "ORDINARY_ASSIGNMENT_BEGIN",
            "ORDINARY_ASSIGNMENT_END",
            "JOIN_HUD_BEGIN",
            "JOIN_HUD_END",
            "MANAGER_PASS_END",
            "BINDING_CANDIDATE",
            "MANAGER_PASS_BEGIN",
            "MANAGER_PASS_END",
            "BINDING_BOUND",
        ]),
        SCENARIO_NATIVE_EXIT => {
            require(&[
                "BINDING_UNBOUND",
                "EXIT_BEGIN",
                "EXIT_HUD_BEGIN",
                "EXIT_HUD_END",
                "EXIT_RETURN_STABLE",
            ])?;
            if events.iter().any(|event| {
                event.local_ordinal == Some(1)
                    && event.kind == "BINDING_BOUND"
                    && event.owner_generation == manifest.final_owner_generation
            }) {
                Err("endpoint B rebound in the exit generation".to_owned())
            } else {
                Ok(())
            }
        }
        SCENARIO_DEATH_RESPAWN => {
            require(&["BINDING_UNBOUND", "BINDING_CANDIDATE", "BINDING_BOUND"])?;
            if kinds.iter().any(|kind| {
                matches!(
                    *kind,
                    "ORDINARY_SPAWN_BEGIN" | "EXIT_BEGIN" | "LIFECYCLE_ENTER"
                )
            }) {
                Ok(())
            } else {
                Err("death/respawn has no frozen lifecycle source".to_owned())
            }
        }
        SCENARIO_ROOM_REBIND => require(&[
            "LIFECYCLE_ENTER",
            "BINDING_UNBOUND",
            "LIFECYCLE_EXIT",
            "BINDING_CANDIDATE",
            "BINDING_BOUND",
        ]),
        SCENARIO_DEVICE_REBIND => device::validate(events, manifest),
        SCENARIO_MENU_TEARDOWN => {
            require(&[
                "LIFECYCLE_ENTER",
                "BINDING_RETIRED",
                "HOOKS_RESTORED",
                "PRODUCER_CLOSED",
            ])?;
            if manifest.stable_registry_retired {
                Ok(())
            } else {
                Err("menu teardown manifest did not retire the stable registry".to_owned())
            }
        }
        _ => Err("unknown passive scenario".to_owned()),
    }
}

fn pop_expected(stack: &mut Vec<&str>, expected: &str) -> Result<(), String> {
    if stack.pop() == Some(expected) {
        Ok(())
    } else {
        Err(format!("passive wrapper nesting did not close {expected}"))
    }
}

fn ordered_subsequence(haystack: &[&str], needle: &[&str]) -> bool {
    let mut position = 0;
    for item in haystack {
        if position < needle.len() && item == &needle[position] {
            position += 1;
        }
    }
    position == needle.len()
}

fn event_kind_name(kind: PassiveEventKind) -> &'static str {
    EVENT_KINDS[kind as usize - 1]
}

fn optional_digest(value: [u8; 32]) -> Option<String> {
    value
        .iter()
        .any(|byte| *byte != 0)
        .then(|| hex::encode_upper(value))
}

fn token_u32(value: u32, domain: &[u8]) -> String {
    let mut hasher = Sha256::new();
    hasher.update(domain);
    hasher.update(value.to_le_bytes());
    hex::encode_upper(hasher.finalize())
}

fn file_identity(case_id: &str, scenario: &str, events_sha256: &str) -> String {
    let mut hasher = Sha256::new();
    hasher.update(case_id.as_bytes());
    hasher.update([0]);
    hasher.update(scenario.as_bytes());
    hasher.update([0]);
    hasher.update(events_sha256.as_bytes());
    hex::encode_upper(hasher.finalize())
}

fn parse_mask(value: &str) -> Option<u64> {
    value
        .strip_prefix("0x")
        .or_else(|| value.strip_prefix("0X"))
        .and_then(|hex| u64::from_str_radix(hex, 16).ok())
}

fn is_nonzero_sha256(value: &str) -> bool {
    value.len() == 64
        && value.bytes().all(|byte| byte.is_ascii_hexdigit())
        && value.bytes().any(|byte| byte != b'0')
}

fn normalize_source_path(path: &Path) -> String {
    path.file_name()
        .map(|value| value.to_string_lossy().replace('\\', "/"))
        .unwrap_or_else(|| "source".to_owned())
}

fn privacy_passes(directory: &Path) -> AppResult<bool> {
    for entry in fs::read_dir(directory).usage_context(format!(
        "cannot enumerate passive source {}",
        directory.display()
    ))? {
        let path = entry
            .usage_context("cannot enumerate passive source entry")?
            .path();
        if !path.is_file() {
            continue;
        }
        let name = path
            .file_name()
            .and_then(|value| value.to_str())
            .unwrap_or("");
        if !name.ends_with(".json") && !name.ends_with(".jsonl") {
            continue;
        }
        let text = fs::read_to_string(&path).usage_context(format!(
            "cannot read passive privacy input {}",
            path.display()
        ))?;
        if text.contains("file://") || text.contains("C:\\\\Users\\\\") {
            return Ok(false);
        }
        if name.ends_with(".jsonl") {
            for line in text.lines() {
                let value: serde_json::Value = serde_json::from_str(line)
                    .usage_context("cannot parse passive privacy JSONL")?;
                if json_contains_private_value(&value, None) {
                    return Ok(false);
                }
            }
        } else {
            let value: serde_json::Value =
                serde_json::from_str(&text).usage_context("cannot parse passive privacy JSON")?;
            if json_contains_private_value(&value, None) {
                return Ok(false);
            }
        }
    }
    Ok(true)
}

fn json_contains_private_value(value: &serde_json::Value, key: Option<&str>) -> bool {
    match value {
        serde_json::Value::Object(map) => map.iter().any(|(child_key, child)| {
            is_forbidden_identity_key(child_key)
                || json_contains_private_value(child, Some(child_key))
        }),
        serde_json::Value::Array(items) => items
            .iter()
            .any(|child| json_contains_private_value(child, key)),
        serde_json::Value::String(text) => {
            let allowed_hex = matches!(
                key,
                Some("rva" | "installedHooksMask" | "restoredHooksMask")
            );
            (!allowed_hex && looks_like_raw_address(text)) || looks_like_absolute_user_path(text)
        }
        _ => false,
    }
}

fn is_forbidden_identity_key(key: &str) -> bool {
    matches!(
        key,
        "pid"
            | "processId"
            | "processCreation"
            | "creationTime"
            | "moduleBase"
            | "pointer"
            | "virtualAddress"
            | "assignmentKey"
            | "initSeed"
            | "platformIdentity"
            | "peerIdentity"
            | "absolutePath"
            | "fileUrl"
    )
}

fn looks_like_raw_address(value: &str) -> bool {
    let Some(hex) = value
        .strip_prefix("0x")
        .or_else(|| value.strip_prefix("0X"))
    else {
        return false;
    };
    matches!(hex.len(), 8 | 16) && hex.bytes().all(|byte| byte.is_ascii_hexdigit())
}

fn looks_like_absolute_user_path(value: &str) -> bool {
    let bytes = value.as_bytes();
    (bytes.len() >= 3
        && bytes[0].is_ascii_alphabetic()
        && bytes[1] == b':'
        && matches!(bytes[2], b'\\' | b'/'))
        || value.starts_with("/Users/")
        || value.starts_with("/home/")
}

fn recursive_file_hashes(directory: &Path) -> AppResult<BTreeMap<PathBuf, String>> {
    let mut output = BTreeMap::new();
    for entry in fs::read_dir(directory).usage_context(format!(
        "cannot enumerate passive source {}",
        directory.display()
    ))? {
        let path = entry
            .usage_context("cannot enumerate passive replay input")?
            .path();
        if path.is_file() {
            output.insert(
                path.file_name().map(PathBuf::from).unwrap_or_default(),
                file_sha256(&path)?,
            );
        }
    }
    Ok(output)
}

fn read_events(path: &Path) -> AppResult<Vec<PassiveEvent>> {
    let text = fs::read_to_string(path)
        .usage_context(format!("cannot read passive events {}", path.display()))?;
    text.lines()
        .enumerate()
        .map(|(index, line)| {
            serde_json::from_str::<PassiveEvent>(line)
                .usage_context(format!("cannot parse passive event line {}", index + 1))
        })
        .collect()
}

fn write_events_new(path: &Path, events: &[PassiveEvent]) -> AppResult<()> {
    let file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .usage_context(format!("cannot create passive events {}", path.display()))?;
    let mut writer = BufWriter::new(file);
    for event in events {
        serde_json::to_writer(&mut writer, event)
            .usage_context("cannot serialize passive event")?;
        writer
            .write_all(b"\n")
            .usage_context("cannot write passive event")?;
    }
    writer
        .flush()
        .usage_context("cannot flush passive events")?;
    Ok(())
}

fn read_json<T: for<'de> Deserialize<'de>>(path: &Path, label: &str) -> AppResult<T> {
    serde_json::from_slice(
        &fs::read(path).usage_context(format!("cannot read {label} {}", path.display()))?,
    )
    .usage_context(format!("cannot parse {label}"))
}

fn write_json_new<T: Serialize>(path: &Path, value: &T) -> AppResult<()> {
    let file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .usage_context(format!("cannot create new JSON {}", path.display()))?;
    serde_json::to_writer_pretty(BufWriter::new(file), value)
        .usage_context(format!("cannot serialize JSON {}", path.display()))
}

fn refuse_existing(path: &Path) -> AppResult<()> {
    if path.exists() {
        Err(AppError::evidence(format!(
            "create-new output already exists: {}",
            path.display()
        )))
    } else {
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn passive_capture_options_are_exact() {
        for scenario in PASSIVE_SCENARIOS {
            assert!(
                validate_passive_capture_options(
                    PASSIVE_PROBE,
                    scenario,
                    "case-01",
                    PASSIVE_DEFAULT_DURATION_MILLISECONDS,
                )
                .is_ok()
            );
        }
        assert!(
            validate_passive_capture_options(
                "control-no-native",
                SCENARIO_NATIVE_JOIN,
                "case",
                PASSIVE_DEFAULT_DURATION_MILLISECONDS,
            )
            .is_err()
        );
        assert!(
            validate_passive_capture_options(
                PASSIVE_PROBE,
                "unknown",
                "case",
                PASSIVE_DEFAULT_DURATION_MILLISECONDS,
            )
            .is_err()
        );
    }

    #[test]
    fn ordered_subsequence_is_strict() {
        assert!(ordered_subsequence(&["A", "B", "C"], &["A", "C"]));
        assert!(!ordered_subsequence(&["B", "A", "C"], &["A", "B"]));
    }
}
