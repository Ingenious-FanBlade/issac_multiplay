use std::collections::{BTreeMap, BTreeSet};
use std::fs::{self, OpenOptions};
use std::io::{BufWriter, Write};
use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};

use crate::active_artifact::{ActiveAnalysisReport, ActiveGateDecision, replay_active_run};
use crate::artifact::{file_sha256, utc_now};
#[cfg(windows)]
use crate::coop_static::CoopCatalog;
use crate::error::{AppError, AppResult, UsageContext};
use crate::loop_verification::{LoopAnalysisReport, LoopGateDecision, replay_loop_run};

pub const C0_SCHEMA_VERSION: u32 = 1;
pub const C0_EVIDENCE_CLASS: &str = "COOP_C0_READ_ONLY";
pub const C0_TOOL_IDENTITY: &str = "IsaacCsT0-CoopC0Observer-v1";
pub const C0_ANALYSIS_IDENTITY: &str = "IsaacCsT0-CoopC0Analysis-v1";
pub const C0_COMPARISON_IDENTITY: &str = "IsaacCsT0-CoopC0Comparison-v1";
pub const C0_GATE_IDENTITY: &str = "IsaacCsT0-CoopC0Decision-v1";
pub const C0_WORK_PACKAGE: &str = "CS-T0-05-02";
pub const C0_PROBE: &str = "control-no-native";
pub const C0_OBSERVER_ACCESS_BITS: &str = "0x00101410";
pub const DEFAULT_DURATION_MILLISECONDS: u64 = 5_000;
pub const DEFAULT_INTERVAL_MILLISECONDS: u64 = 50;
pub const MAX_RETRIES: u32 = 4;
pub const C0_SLOT_COUNT: usize = 53;

pub const SCENARIO_OFFLINE_SINGLE: &str = "offline-single-c0";
pub const SCENARIO_LOCAL_DUAL: &str = "local-coop-dual-c0";
pub const SCENARIO_ORIGINAL_ONLINE: &str = "original-online-dual-disarmed";
pub const C0_SCENARIOS: [&str; 3] = [
    SCENARIO_OFFLINE_SINGLE,
    SCENARIO_LOCAL_DUAL,
    SCENARIO_ORIGINAL_ONLINE,
];

const FIXED_TARGET_EQUIVALENCE_SHA256: &str =
    "8B3FB097ED32B18DE6A78F535045ACD525185569ACA029A6370494511851F138";
const FIXED_OBSERVED_TARGET_SHA256: &str =
    "1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C";
const FIXED_COOP_CATALOG_V2_SHA256: &str =
    "1059711778DAD09870D37118571EE7D5E67761EC28FFAB84C7CF5AC5998A0224";
const FIXED_COOP_GRAPH_V1_SHA256: &str =
    "F87DA1DF383B7B1AE3724A4DB4EAAE791B9F42B68AF9210AD1727F71D9458934";
const FIXED_COOP_PLAN_V2_SHA256: &str =
    "40DB0C0F42AC1DD2B86042633D44357F015459DBDFF276A237946F978706FB34";
const FIXED_COOP_STATIC_DECISION_SHA256: &str =
    "2CA3A9956FEAED6E08BDAA2F89F2A244DE4CF6B6CB0E2AFA0EFAEE7A37C9E459";
const FIXED_C0_LAYOUT_SHA256: &str =
    "921EE4C4AA1FDCE86E21F1CFF1B38D5802E13E715242EC6006FA79B5F6316059";
const FIXED_C0_PLAN_V3_SHA256: &str =
    "12BE7D22A1103A57923A1B6894B3D6C3433A165E431B06C44B28CB1DF21C6850";
const FIXED_LOOP_GATE_SHA256: &str =
    "CDC0CE89A7393740A775E40FE6E588910038FEC68C40EB6D6F5CDF7BC1F1A1AA";
const FIXED_ACTIVE_DECISION_SHA256: &str =
    "927561D7DC28FE4DE05680BC68C6FA1841E3C77D76B30A710CE34031072E222C";

const STATUS_COHERENT: &str = "COHERENT_C0_READ_ONLY";
const STATUS_UNSTABLE: &str = "DROPPED_UNSTABLE";
const STATUS_READ_ERROR: &str = "DROPPED_READ_ERROR";
const LUA_STATIC_ORDER: &str = "STATIC_VECTOR_EQUIVALENT";

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ArtifactBinding {
    pub path: String,
    pub identity: String,
    pub sha256: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct C0Authorization {
    pub probe: String,
    pub scenarios: Vec<String>,
    pub observer_access_bits: String,
    pub default_duration_milliseconds: u64,
    pub default_interval_milliseconds: u64,
    pub max_retries: u32,
    pub process_read_allowed: bool,
    pub runtime_mutation_allowed: bool,
    pub hook_allowed: bool,
    pub ipc_allowed: bool,
    pub mapping_allowed: bool,
    pub helper_allowed: bool,
    pub native_load_allowed: bool,
    pub scheduler_allowed: bool,
    pub apply_allowed: bool,
    pub requires_per_mode_confirmation: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct C0Permissions {
    pub active_input_allowed: bool,
    pub passive_probe_allowed: bool,
    pub runtime_mutation_allowed: bool,
    pub t005_unlocked: bool,
    pub cs_t00503_unlocked: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct C0Plan {
    pub schema_version: u32,
    pub plan_identity: String,
    pub work_package: String,
    pub created_utc: String,
    pub status: String,
    pub supersedes: String,
    pub supersedes_sha256: String,
    pub create_new_only: bool,
    pub artifact_bindings: Vec<ArtifactBinding>,
    pub c0_authorization: C0Authorization,
    pub permissions: C0Permissions,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ExpectedTargetSlot {
    pub ordinal: u32,
    pub id: String,
    pub owner: String,
    pub patch_kind: String,
    pub rva: String,
    pub width: u32,
    pub expected_bytes_proof: Option<String>,
    pub expected_semantic: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ObservedTargetSlot {
    pub ordinal: u32,
    pub id: String,
    pub rva: String,
    pub bytes_proof: String,
    pub semantic: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct CodeObservation {
    pub id: String,
    pub rva: String,
    pub width: u32,
    pub bytes_hex: String,
    pub semantic: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ExpectedTargetDocument {
    pub schema_version: u32,
    pub document_identity: String,
    pub slot_count: usize,
    pub slots: Vec<ExpectedTargetSlot>,
    pub tick_path: Vec<CodeObservation>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ObservedTargetDocument {
    pub schema_version: u32,
    pub document_identity: String,
    pub phase: String,
    pub slot_count: usize,
    pub slots: Vec<ObservedTargetSlot>,
    pub tick_path: Vec<CodeObservation>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct C0PlayerObservation {
    pub ordinal: u32,
    pub object_token: String,
    pub device_kind: Option<String>,
    pub unique_assignment: bool,
    pub ordinal_field_matches: bool,
    pub hud_primary_slot_matches: bool,
    pub lua_order_evidence: String,
    pub init_seed_stable: bool,
    pub init_seed_nonzero: bool,
    pub init_seed_distinct: bool,
    pub twin_relation_absent: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct C0GraphObservation {
    pub topology_sha256: String,
    pub run_active: bool,
    pub online_mode: bool,
    pub manager_present: bool,
    pub network_backend_present: bool,
    pub net_input_present: bool,
    pub player_count: usize,
    pub device_count: usize,
    pub roster_count: usize,
    pub players: Vec<C0PlayerObservation>,
    pub unassigned_device_kinds: Vec<String>,
    pub primary_hud_consistent: bool,
    pub twin_hud_empty: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct C0Sample {
    pub schema_version: u32,
    pub observer_sequence: u64,
    pub qpc: i64,
    pub status: String,
    pub retry_count: u32,
    pub reason: Option<String>,
    pub game_frame_delta: Option<u32>,
    pub online_frame_delta: Option<u32>,
    pub graph: Option<C0GraphObservation>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct C0Manifest {
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
    pub target_sha256: String,
    pub target_equivalence_sha256: String,
    pub layout_evidence_sha256: String,
    pub coop_catalog_sha256: String,
    pub coop_graph_sha256: String,
    pub coop_plan_sha256: String,
    pub observer_access_bits: String,
    pub requested_duration_milliseconds: u64,
    pub interval_milliseconds: u64,
    pub max_retries: u32,
    pub qpc_frequency: i64,
    pub process_identity_token: String,
    pub module_set_sha256: String,
    pub blockers_absent: bool,
    pub debugger_absent: bool,
    pub forbidden_modules_absent: bool,
    pub process_identity_stable: bool,
    pub module_set_stable: bool,
    pub target_disk_stable: bool,
    pub expected_targets_sha256: String,
    pub target_bytes_before_sha256: String,
    pub target_bytes_after_sha256: String,
    pub samples_sha256: String,
    pub hook_installed: bool,
    pub ipc_created: bool,
    pub mapping_created: bool,
    pub helper_started: bool,
    pub native_loaded: bool,
    pub scheduler_started: bool,
    pub apply_attempted: bool,
    pub status: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct C0Check {
    pub id: String,
    pub passed: bool,
    pub detail: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct C0Analysis {
    pub schema_version: u32,
    pub analysis_identity: String,
    pub evidence_class: String,
    pub result: String,
    pub scenario: String,
    pub artifact_identity: String,
    pub target_sha256: String,
    pub target_equivalence_sha256: String,
    pub layout_evidence_sha256: String,
    pub coop_catalog_sha256: String,
    pub coop_graph_sha256: String,
    pub coop_plan_sha256: String,
    pub manifest_sha256: String,
    pub samples_sha256: String,
    pub expected_targets_sha256: String,
    pub target_bytes_before_sha256: String,
    pub target_bytes_after_sha256: String,
    pub event_count: usize,
    pub coherent_samples: usize,
    pub unstable_drops: usize,
    pub read_error_drops: usize,
    pub topology_sha256: Option<String>,
    pub player_count: Option<usize>,
    pub device_kinds: Vec<String>,
    pub roster_count: Option<usize>,
    pub online_mode: Option<bool>,
    pub unique_assignment: bool,
    pub hud_consistent: bool,
    pub lua_static_order_consistent: bool,
    pub game_frame_progress: bool,
    pub online_frame_monotonic: bool,
    pub tick_proxy_status: String,
    pub direct_post_commit_observed: bool,
    pub authority_commit_status: String,
    pub no_mutation_passed: bool,
    pub checks: Vec<C0Check>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct C0SourceBinding {
    pub role: String,
    pub scenario: String,
    pub artifact_identity: String,
    pub analysis_sha256: String,
    pub manifest_sha256: String,
    pub samples_sha256: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct C0ModeShape {
    pub scenario: String,
    pub player_count: usize,
    pub device_kinds: Vec<String>,
    pub roster_count: usize,
    pub online_mode: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct C0CompositeNoMutationReport {
    pub report_identity: String,
    pub matrix_row: String,
    pub source_count: usize,
    pub source_roles: Vec<String>,
    pub target_slot_count: usize,
    pub no_fourth_live_capture: bool,
    pub all_sources_no_mutation: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct C0Comparison {
    pub schema_version: u32,
    pub comparison_identity: String,
    pub work_package: String,
    pub result: String,
    pub source_bindings: Vec<C0SourceBinding>,
    pub mode_shapes: Vec<C0ModeShape>,
    pub target_sha256: String,
    pub target_equivalence_sha256: String,
    pub layout_evidence_sha256: String,
    pub coop_catalog_sha256: String,
    pub coop_graph_sha256: String,
    pub coop_plan_sha256: String,
    pub three_unique_roles: bool,
    pub local_positive_isolated_from_original_online: bool,
    pub composite_no_mutation_report: C0CompositeNoMutationReport,
    pub composite_no_mutation_passed: bool,
    pub dual_player_unique_join_passed: bool,
    pub tick_proxy_status: String,
    pub direct_post_commit_observed: bool,
    pub authority_commit_status: String,
    pub checks: Vec<C0Check>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct C0GateDecision {
    pub schema_version: u32,
    pub decision_identity: String,
    pub work_package: String,
    pub status: String,
    pub comparison_sha256: String,
    pub loop_gate_sha256: String,
    pub active_decision_sha256: String,
    pub loop_gate_replayed: bool,
    pub active_sources_replayed: bool,
    pub tick_proxy_status: String,
    pub direct_post_commit_observed: bool,
    pub authority_commit_status: String,
    pub composite_no_mutation_passed: bool,
    pub cs_t00502_passed: bool,
    pub cs_t00503_unlocked: bool,
    pub t005_unlocked: bool,
    pub active_input_allowed: bool,
    pub passive_probe_allowed: bool,
    pub runtime_mutation_allowed: bool,
    pub checks: Vec<C0Check>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct C0CaptureSummary {
    pub schema_version: u32,
    pub evidence_class: String,
    pub result: String,
    pub scenario: String,
    pub artifact_identity: String,
    pub output_directory: PathBuf,
    pub event_count: usize,
    pub coherent_samples: usize,
    pub unstable_drops: usize,
    pub read_error_drops: usize,
    pub active_input_allowed: bool,
    pub runtime_mutation_allowed: bool,
}

pub struct CaptureRequest<'a> {
    pub probe: &'a str,
    pub scenario: &'a str,
    pub game_exe: &'a Path,
    pub target_equivalence: &'a Path,
    pub layout_evidence: &'a Path,
    pub catalog: &'a Path,
    pub graph: &'a Path,
    pub plan: &'a Path,
    pub artifacts_root: &'a Path,
    pub case_id: &'a str,
    pub duration_milliseconds: u64,
    pub interval_milliseconds: u64,
}

pub fn validate_scenario(value: &str) -> AppResult<()> {
    if C0_SCENARIOS.contains(&value) {
        Ok(())
    } else {
        Err(AppError::usage(format!(
            "coop C0 scenario must be one of {}, {}, or {}",
            SCENARIO_OFFLINE_SINGLE, SCENARIO_LOCAL_DUAL, SCENARIO_ORIGINAL_ONLINE
        )))
    }
}

pub fn validate_capture_options(
    probe: &str,
    scenario: &str,
    case_id: &str,
    duration_milliseconds: u64,
    interval_milliseconds: u64,
) -> AppResult<()> {
    if probe != C0_PROBE {
        return Err(AppError::usage(
            "coop-smoke supports only --probe control-no-native",
        ));
    }
    validate_scenario(scenario)?;
    if case_id.is_empty()
        || case_id.len() > 64
        || !case_id
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_'))
    {
        return Err(AppError::usage(
            "case-id must be 1-64 ASCII alphanumeric, '-' or '_' characters",
        ));
    }
    if !(1_000..=30_000).contains(&duration_milliseconds) {
        return Err(AppError::usage(
            "coop C0 duration-ms must be in the inclusive range 1000..30000",
        ));
    }
    if !(10..=1_000).contains(&interval_milliseconds) {
        return Err(AppError::usage(
            "coop C0 interval-ms must be in the inclusive range 10..1000",
        ));
    }
    Ok(())
}

pub fn load_and_validate_plan(
    plan_path: &Path,
    target_equivalence: &Path,
    layout_evidence: &Path,
    catalog: &Path,
    graph: &Path,
) -> AppResult<C0Plan> {
    let plan: C0Plan = read_json(plan_path, "coop C0 plan")?;
    let scenarios = C0_SCENARIOS.map(str::to_owned).to_vec();
    if plan.schema_version != 3
        || plan.plan_identity != "IsaacCsT0-CoopPlan-v3"
        || plan.work_package != C0_WORK_PACKAGE
        || plan.status != "AUTHORIZED_C0_READ_ONLY"
        || plan.supersedes != "coop-plan-v2.json"
        || !plan
            .supersedes_sha256
            .eq_ignore_ascii_case(FIXED_COOP_PLAN_V2_SHA256)
        || !plan.create_new_only
        || plan.c0_authorization.probe != C0_PROBE
        || plan.c0_authorization.scenarios != scenarios
        || plan.c0_authorization.observer_access_bits != C0_OBSERVER_ACCESS_BITS
        || plan.c0_authorization.default_duration_milliseconds != DEFAULT_DURATION_MILLISECONDS
        || plan.c0_authorization.default_interval_milliseconds != DEFAULT_INTERVAL_MILLISECONDS
        || plan.c0_authorization.max_retries != MAX_RETRIES
        || !plan.c0_authorization.process_read_allowed
        || plan.c0_authorization.runtime_mutation_allowed
        || plan.c0_authorization.hook_allowed
        || plan.c0_authorization.ipc_allowed
        || plan.c0_authorization.mapping_allowed
        || plan.c0_authorization.helper_allowed
        || plan.c0_authorization.native_load_allowed
        || plan.c0_authorization.scheduler_allowed
        || plan.c0_authorization.apply_allowed
        || !plan.c0_authorization.requires_per_mode_confirmation
        || plan.permissions.active_input_allowed
        || plan.permissions.passive_probe_allowed
        || plan.permissions.runtime_mutation_allowed
        || plan.permissions.t005_unlocked
        || plan.permissions.cs_t00503_unlocked
    {
        return Err(AppError::evidence(
            "coop C0 plan does not grant the exact read-only CS-T0-05-02 capability",
        ));
    }
    let required = [
        (
            target_equivalence,
            "IsaacCsT0-UnpackedTargetEquivalence-v1",
            FIXED_TARGET_EQUIVALENCE_SHA256,
        ),
        (
            catalog,
            "IsaacCsT0-CoopCatalog-v2",
            FIXED_COOP_CATALOG_V2_SHA256,
        ),
        (
            graph,
            "IsaacCsT0-CoopStaticGraph-v1",
            FIXED_COOP_GRAPH_V1_SHA256,
        ),
    ];
    for (path, identity, expected_hash) in required {
        let actual = file_sha256(path)?;
        if !actual.eq_ignore_ascii_case(expected_hash)
            || !plan.artifact_bindings.iter().any(|binding| {
                binding.identity == identity && binding.sha256.eq_ignore_ascii_case(&actual)
            })
        {
            return Err(AppError::evidence(format!(
                "coop C0 plan does not bind the frozen {identity}"
            )));
        }
    }
    let layout_hash = file_sha256(layout_evidence)?;
    let layout: serde_json::Value = read_json(layout_evidence, "coop C0 layout evidence")?;
    if layout
        .get("schemaVersion")
        .and_then(serde_json::Value::as_u64)
        != Some(1)
        || layout
            .get("evidenceIdentity")
            .and_then(serde_json::Value::as_str)
            != Some("IsaacCsT0-CoopC0LayoutEvidence-v1")
        || layout
            .get("workPackage")
            .and_then(serde_json::Value::as_str)
            != Some(C0_WORK_PACKAGE)
        || layout.get("status").and_then(serde_json::Value::as_str) != Some("STATIC_CONFIRMED")
        || !plan.artifact_bindings.iter().any(|binding| {
            binding.identity == "IsaacCsT0-CoopC0LayoutEvidence-v1"
                && binding.sha256.eq_ignore_ascii_case(&layout_hash)
        })
        || !plan.artifact_bindings.iter().any(|binding| {
            binding.identity == "IsaacCsT0-CoopStaticDecision-v1"
                && binding
                    .sha256
                    .eq_ignore_ascii_case(FIXED_COOP_STATIC_DECISION_SHA256)
        })
    {
        return Err(AppError::evidence(
            "coop C0 layout evidence or CS-T0-05-01 decision binding is invalid",
        ));
    }
    Ok(plan)
}

pub fn analyze(run_directory: &Path) -> AppResult<C0Analysis> {
    let manifest_path = run_directory.join("manifest.json");
    let samples_path = run_directory.join("samples.jsonl");
    let expected_path = run_directory.join("target-bytes-expected.json");
    let before_path = run_directory.join("target-bytes-before.json");
    let after_path = run_directory.join("target-bytes-after.json");
    let manifest: C0Manifest = read_json(&manifest_path, "coop C0 manifest")?;
    let expected: ExpectedTargetDocument = read_json(&expected_path, "expected targets")?;
    let before: ObservedTargetDocument = read_json(&before_path, "before targets")?;
    let after: ObservedTargetDocument = read_json(&after_path, "after targets")?;
    let samples = read_samples(&samples_path)?;

    let manifest_shape = manifest.schema_version == C0_SCHEMA_VERSION
        && manifest.evidence_class == C0_EVIDENCE_CLASS
        && manifest.tool_identity == C0_TOOL_IDENTITY
        && manifest.work_package == C0_WORK_PACKAGE
        && manifest.probe == C0_PROBE
        && C0_SCENARIOS.contains(&manifest.scenario.as_str())
        && manifest.status == "CAPTURE_COMPLETE"
        && manifest.started_utc.ends_with('Z')
        && manifest.ended_utc.ends_with('Z')
        && manifest
            .target_equivalence_sha256
            .eq_ignore_ascii_case(FIXED_TARGET_EQUIVALENCE_SHA256)
        && manifest
            .target_sha256
            .eq_ignore_ascii_case(FIXED_OBSERVED_TARGET_SHA256)
        && manifest
            .layout_evidence_sha256
            .eq_ignore_ascii_case(FIXED_C0_LAYOUT_SHA256)
        && manifest
            .coop_catalog_sha256
            .eq_ignore_ascii_case(FIXED_COOP_CATALOG_V2_SHA256)
        && manifest
            .coop_graph_sha256
            .eq_ignore_ascii_case(FIXED_COOP_GRAPH_V1_SHA256)
        && manifest
            .coop_plan_sha256
            .eq_ignore_ascii_case(FIXED_C0_PLAN_V3_SHA256)
        && manifest.observer_access_bits == C0_OBSERVER_ACCESS_BITS
        && (1_000..=30_000).contains(&manifest.requested_duration_milliseconds)
        && (10..=1_000).contains(&manifest.interval_milliseconds)
        && manifest.max_retries == MAX_RETRIES
        && manifest.qpc_frequency > 0
        && !manifest.process_identity_token.is_empty()
        && manifest.process_identity_token.len() <= 40
        && manifest.module_set_sha256.len() == 64
        && !manifest.hook_installed
        && !manifest.ipc_created
        && !manifest.mapping_created
        && !manifest.helper_started
        && !manifest.native_loaded
        && !manifest.scheduler_started
        && !manifest.apply_attempted;
    let hashes_valid = manifest
        .samples_sha256
        .eq_ignore_ascii_case(&file_sha256(&samples_path)?)
        && manifest
            .expected_targets_sha256
            .eq_ignore_ascii_case(&file_sha256(&expected_path)?)
        && manifest
            .target_bytes_before_sha256
            .eq_ignore_ascii_case(&file_sha256(&before_path)?)
        && manifest
            .target_bytes_after_sha256
            .eq_ignore_ascii_case(&file_sha256(&after_path)?);
    let target_documents_valid = validate_target_documents(&expected, &before, &after).is_ok();
    let sequence_valid = !samples.is_empty()
        && samples
            .iter()
            .enumerate()
            .all(|(index, sample)| sample.observer_sequence == index as u64 + 1)
        && samples.iter().all(|sample| sample.qpc > 0)
        && samples.windows(2).all(|pair| pair[0].qpc <= pair[1].qpc);
    let sample_shapes_valid = samples.iter().all(validate_sample_shape);
    let privacy_valid = [
        serde_json::to_value(&manifest),
        serde_json::to_value(&expected),
        serde_json::to_value(&before),
        serde_json::to_value(&after),
        serde_json::to_value(&samples),
    ]
    .into_iter()
    .all(|value| value.ok().is_some_and(|value| privacy_scan(&value).is_ok()));

    let coherent = samples
        .iter()
        .filter(|sample| sample.status == STATUS_COHERENT)
        .collect::<Vec<_>>();
    let coherent_samples = coherent.len();
    let unstable_drops = samples
        .iter()
        .filter(|sample| sample.status == STATUS_UNSTABLE)
        .count();
    let read_error_drops = samples
        .iter()
        .filter(|sample| sample.status == STATUS_READ_ERROR)
        .count();
    let topology = coherent
        .first()
        .and_then(|sample| sample.graph.as_ref())
        .map(|graph| graph.topology_sha256.clone());
    let topology_stable = topology.as_ref().is_some_and(|digest| {
        coherent.iter().all(|sample| {
            sample
                .graph
                .as_ref()
                .is_some_and(|graph| &graph.topology_sha256 == digest)
        })
    });
    let graph = coherent.first().and_then(|sample| sample.graph.as_ref());
    let scenario_pass = graph.is_some_and(|graph| scenario_graph_pass(&manifest.scenario, graph));
    let player_count = graph.map(|graph| graph.player_count);
    let roster_count = graph.map(|graph| graph.roster_count);
    let online_mode = graph.map(|graph| graph.online_mode);
    let mut device_kinds = graph
        .into_iter()
        .flat_map(|graph| &graph.players)
        .filter_map(|player| player.device_kind.clone())
        .collect::<Vec<_>>();
    device_kinds.sort();
    let unique_assignment =
        graph.is_some_and(|graph| graph.players.iter().all(|player| player.unique_assignment));
    let hud_consistent = graph.is_some_and(|graph| {
        graph.primary_hud_consistent
            && graph.twin_hud_empty
            && graph
                .players
                .iter()
                .all(|player| player.hud_primary_slot_matches)
    });
    let lua_static_order_consistent = graph.is_some_and(|graph| {
        graph.players.iter().all(|player| {
            player.lua_order_evidence == LUA_STATIC_ORDER && player.ordinal_field_matches
        })
    });
    let game_deltas = coherent
        .iter()
        .filter_map(|sample| sample.game_frame_delta)
        .collect::<Vec<_>>();
    let game_frame_monotonic =
        !game_deltas.is_empty() && game_deltas.windows(2).all(|pair| pair[0] <= pair[1]);
    let game_frame_progress = manifest.scenario == SCENARIO_LOCAL_DUAL
        && game_frame_monotonic
        && game_deltas.last().copied().unwrap_or(0) > 0;
    let online_deltas = coherent
        .iter()
        .filter_map(|sample| sample.online_frame_delta)
        .collect::<Vec<_>>();
    let online_frame_monotonic = manifest.scenario != SCENARIO_ORIGINAL_ONLINE
        || (!online_deltas.is_empty()
            && online_deltas.windows(2).all(|pair| pair[0] <= pair[1])
            && online_deltas.last().copied().unwrap_or(0) > 0);
    let no_mutation_passed = manifest.blockers_absent
        && manifest.debugger_absent
        && manifest.forbidden_modules_absent
        && manifest.process_identity_stable
        && manifest.module_set_stable
        && manifest.target_disk_stable
        && target_documents_valid
        && !manifest.hook_installed
        && !manifest.ipc_created
        && !manifest.mapping_created
        && !manifest.helper_started
        && !manifest.native_loaded
        && !manifest.scheduler_started
        && !manifest.apply_attempted;
    let tick_proxy_status = if manifest.scenario == SCENARIO_LOCAL_DUAL && game_frame_progress {
        "PASS_STEADY_STANDARD_PRE_ATTEMPT_MONOTONIC"
    } else if manifest.scenario == SCENARIO_LOCAL_DUAL {
        "BLOCKED_BY_EVIDENCE"
    } else {
        "NOT_APPLICABLE"
    }
    .to_owned();

    let checks = vec![
        check(
            "strict-manifest",
            manifest_shape,
            "manifest is the exact C0 read-only identity",
        ),
        check(
            "bound-file-hashes",
            hashes_valid,
            "all source files match manifest SHA-256 bindings",
        ),
        check(
            "target-53-slot-stability",
            target_documents_valid,
            "53 expected/before/after slot semantics and tick path are unchanged",
        ),
        check(
            "event-sequence-qpc",
            sequence_valid,
            "sequence is contiguous and QPC is positive and nondecreasing",
        ),
        check(
            "sample-shapes",
            sample_shapes_valid,
            "coherent and dropped samples have strict disjoint shapes",
        ),
        check(
            "artifact-privacy",
            privacy_valid,
            "artifact contains no raw runtime identity or local path",
        ),
        check(
            "at-least-two-coherent",
            coherent_samples >= 2,
            "at least two coherent samples exist",
        ),
        check(
            "stable-topology",
            topology_stable,
            "every coherent sample has one topology",
        ),
        check(
            "scenario-contract",
            scenario_pass,
            "object graph satisfies the selected fixed scenario",
        ),
        check(
            "no-mutation",
            no_mutation_passed,
            "observer remained clean and all targets stayed unchanged",
        ),
        check(
            "local-frame-progress",
            manifest.scenario != SCENARIO_LOCAL_DUAL || game_frame_progress,
            "local dual pre-attempt frame proxy is monotonic with positive progress",
        ),
        check(
            "online-frame-monotonic",
            online_frame_monotonic,
            "Original Online frame is monotonic",
        ),
    ];
    let passed = checks.iter().all(|item| item.passed);
    Ok(C0Analysis {
        schema_version: C0_SCHEMA_VERSION,
        analysis_identity: C0_ANALYSIS_IDENTITY.to_owned(),
        evidence_class: C0_EVIDENCE_CLASS.to_owned(),
        result: if passed {
            "PASS"
        } else {
            "BLOCKED_BY_EVIDENCE"
        }
        .to_owned(),
        scenario: manifest.scenario.clone(),
        artifact_identity: manifest.artifact_identity,
        target_sha256: manifest.target_sha256,
        target_equivalence_sha256: manifest.target_equivalence_sha256,
        layout_evidence_sha256: manifest.layout_evidence_sha256,
        coop_catalog_sha256: manifest.coop_catalog_sha256,
        coop_graph_sha256: manifest.coop_graph_sha256,
        coop_plan_sha256: manifest.coop_plan_sha256,
        manifest_sha256: file_sha256(&manifest_path)?,
        samples_sha256: manifest.samples_sha256,
        expected_targets_sha256: manifest.expected_targets_sha256,
        target_bytes_before_sha256: manifest.target_bytes_before_sha256,
        target_bytes_after_sha256: manifest.target_bytes_after_sha256,
        event_count: samples.len(),
        coherent_samples,
        unstable_drops,
        read_error_drops,
        topology_sha256: topology,
        player_count,
        device_kinds,
        roster_count,
        online_mode,
        unique_assignment,
        hud_consistent,
        lua_static_order_consistent,
        game_frame_progress,
        online_frame_monotonic,
        tick_proxy_status,
        direct_post_commit_observed: false,
        authority_commit_status: if passed && manifest.scenario == SCENARIO_LOCAL_DUAL {
            "PENDING_INHERITED_GATE"
        } else {
            "NOT_APPLICABLE"
        }
        .to_owned(),
        no_mutation_passed,
        checks,
    })
}

pub fn analyze_to(run_directory: &Path, output: &Path) -> AppResult<C0Analysis> {
    let analysis = analyze(run_directory)?;
    write_json_create_new(output, &analysis)?;
    Ok(analysis)
}

pub fn compare(
    offline_single: &Path,
    local_dual: &Path,
    original_online: &Path,
) -> AppResult<C0Comparison> {
    let sources = [
        ("OFFLINE_SINGLE", SCENARIO_OFFLINE_SINGLE, offline_single),
        ("LOCAL_COOP_DUAL", SCENARIO_LOCAL_DUAL, local_dual),
        (
            "ORIGINAL_ONLINE_DUAL",
            SCENARIO_ORIGINAL_ONLINE,
            original_online,
        ),
    ];
    let mut analyses = Vec::with_capacity(3);
    let mut bindings = Vec::with_capacity(3);
    for (role, scenario, directory) in sources {
        let stored_path = directory.join("analysis.json");
        let stored: C0Analysis = read_json(&stored_path, "stored coop C0 analysis")?;
        let replay = analyze(directory)?;
        if stored != replay || replay.scenario != scenario {
            return Err(AppError::evidence(format!(
                "coop C0 {role} analysis does not match replay or scenario"
            )));
        }
        bindings.push(C0SourceBinding {
            role: role.to_owned(),
            scenario: scenario.to_owned(),
            artifact_identity: replay.artifact_identity.clone(),
            analysis_sha256: file_sha256(&stored_path)?,
            manifest_sha256: replay.manifest_sha256.clone(),
            samples_sha256: replay.samples_sha256.clone(),
        });
        analyses.push(replay);
    }
    let shared_bindings = analyses.windows(2).all(|pair| {
        pair[0].target_sha256 == pair[1].target_sha256
            && pair[0].target_equivalence_sha256 == pair[1].target_equivalence_sha256
            && pair[0].layout_evidence_sha256 == pair[1].layout_evidence_sha256
            && pair[0].coop_catalog_sha256 == pair[1].coop_catalog_sha256
            && pair[0].coop_graph_sha256 == pair[1].coop_graph_sha256
            && pair[0].coop_plan_sha256 == pair[1].coop_plan_sha256
    });
    let unique_artifacts = bindings
        .iter()
        .map(|binding| binding.artifact_identity.as_str())
        .collect::<BTreeSet<_>>()
        .len()
        == 3;
    let all_pass = analyses.iter().all(|analysis| analysis.result == "PASS");
    let local = &analyses[1];
    let online = &analyses[2];
    let local_positive_isolated = local.online_mode == Some(false)
        && local.player_count == Some(2)
        && online.online_mode == Some(true)
        && online.roster_count == Some(2);
    let composite_no_mutation = analyses.iter().all(|analysis| analysis.no_mutation_passed);
    let unique_join = local.unique_assignment
        && local.hud_consistent
        && local.lua_static_order_consistent
        && local.device_kinds == ["GAMEPAD".to_owned(), "KEYBOARD".to_owned()];
    let mode_shapes = analyses
        .iter()
        .map(|analysis| C0ModeShape {
            scenario: analysis.scenario.clone(),
            player_count: analysis.player_count.unwrap_or(0),
            device_kinds: analysis.device_kinds.clone(),
            roster_count: analysis.roster_count.unwrap_or(0),
            online_mode: analysis.online_mode.unwrap_or(false),
        })
        .collect::<Vec<_>>();
    let checks = vec![
        check(
            "three-source-pass",
            all_pass,
            "all three analyses replay to PASS",
        ),
        check(
            "shared-static-bindings",
            shared_bindings,
            "all sources bind the same target, layout, catalog, graph, and plan",
        ),
        check(
            "three-unique-artifacts",
            unique_artifacts,
            "each role uses one unique artifact identity",
        ),
        check(
            "local-online-partition",
            local_positive_isolated,
            "local positive graph is independent of Original Online objects",
        ),
        check(
            "dual-unique-join",
            unique_join,
            "local Player/device/HUD/Lua relations are unique and consistent",
        ),
        check(
            "composite-no-mutation",
            composite_no_mutation,
            "all three sources satisfy the no-mutation contract",
        ),
        check(
            "tick-pre-attempt-proxy",
            local.tick_proxy_status == "PASS_STEADY_STANDARD_PRE_ATTEMPT_MONOTONIC",
            "local dual directly observes only the monotonic pre-attempt proxy",
        ),
    ];
    let passed = checks.iter().all(|item| item.passed);
    let composite_no_mutation_report = C0CompositeNoMutationReport {
        report_identity: "IsaacCsT0-CoopC0CompositeNoMutation-v1".to_owned(),
        matrix_row: C0_PROBE.to_owned(),
        source_count: bindings.len(),
        source_roles: bindings
            .iter()
            .map(|binding| binding.role.clone())
            .collect(),
        target_slot_count: C0_SLOT_COUNT,
        no_fourth_live_capture: true,
        all_sources_no_mutation: composite_no_mutation,
    };
    Ok(C0Comparison {
        schema_version: C0_SCHEMA_VERSION,
        comparison_identity: C0_COMPARISON_IDENTITY.to_owned(),
        work_package: C0_WORK_PACKAGE.to_owned(),
        result: if passed {
            "PASS"
        } else {
            "BLOCKED_BY_EVIDENCE"
        }
        .to_owned(),
        source_bindings: bindings,
        mode_shapes,
        target_sha256: analyses[0].target_sha256.clone(),
        target_equivalence_sha256: analyses[0].target_equivalence_sha256.clone(),
        layout_evidence_sha256: analyses[0].layout_evidence_sha256.clone(),
        coop_catalog_sha256: analyses[0].coop_catalog_sha256.clone(),
        coop_graph_sha256: analyses[0].coop_graph_sha256.clone(),
        coop_plan_sha256: analyses[0].coop_plan_sha256.clone(),
        three_unique_roles: unique_artifacts,
        local_positive_isolated_from_original_online: local_positive_isolated,
        composite_no_mutation_report,
        composite_no_mutation_passed: composite_no_mutation,
        dual_player_unique_join_passed: unique_join,
        tick_proxy_status: local.tick_proxy_status.clone(),
        direct_post_commit_observed: false,
        authority_commit_status: "PENDING_INHERITED_GATE".to_owned(),
        checks,
    })
}

pub fn compare_to(
    offline_single: &Path,
    local_dual: &Path,
    original_online: &Path,
    output: &Path,
) -> AppResult<C0Comparison> {
    let comparison = compare(offline_single, local_dual, original_online)?;
    write_json_create_new(output, &comparison)?;
    Ok(comparison)
}

pub fn gate_to(
    offline_single: &Path,
    local_dual: &Path,
    original_online: &Path,
    comparison_path: &Path,
    loop_gate_path: &Path,
    active_decision_path: &Path,
    output: &Path,
) -> AppResult<C0GateDecision> {
    if output.exists() {
        return Err(AppError::usage(format!(
            "refusing to overwrite {}",
            output.display()
        )));
    }
    let stored_comparison = read_json::<C0Comparison>(comparison_path, "coop C0 comparison").ok();
    let replayed_comparison = compare(offline_single, local_dual, original_online).ok();
    let comparison_replayed = stored_comparison
        .as_ref()
        .zip(replayed_comparison.as_ref())
        .is_some_and(|(stored, replayed)| stored == replayed);
    let loop_replayed = replay_loop_gate(loop_gate_path).unwrap_or(false);
    let active_replayed = replay_active_decision(active_decision_path).unwrap_or(false);
    let comparison_pass = comparison_replayed
        && stored_comparison
            .as_ref()
            .is_some_and(|comparison| comparison.result == "PASS");
    let inherited_pass = loop_replayed && active_replayed;
    let tick_proxy_pass = stored_comparison.as_ref().is_some_and(|comparison| {
        comparison.tick_proxy_status == "PASS_STEADY_STANDARD_PRE_ATTEMPT_MONOTONIC"
    });
    let composite_no_mutation_passed = stored_comparison
        .as_ref()
        .is_some_and(|comparison| comparison.composite_no_mutation_passed);
    let checks = vec![
        check(
            "comparison-replay",
            comparison_pass,
            "stored comparison exactly matches three-source replay",
        ),
        check(
            "t0-03r-loop-gate-replay",
            loop_replayed,
            "frozen loop gate and every bound source replay",
        ),
        check(
            "t0-04-active-decision-replay",
            active_replayed,
            "frozen active decision and every bound source replay",
        ),
        check(
            "tick-path-and-proxy",
            tick_proxy_pass,
            "C0 proxy and frozen tick path are present",
        ),
        check(
            "composite-no-mutation",
            composite_no_mutation_passed,
            "three-source no-mutation report passes",
        ),
    ];
    let passed = checks.iter().all(|item| item.passed) && inherited_pass;
    let decision = C0GateDecision {
        schema_version: C0_SCHEMA_VERSION,
        decision_identity: C0_GATE_IDENTITY.to_owned(),
        work_package: C0_WORK_PACKAGE.to_owned(),
        status: if passed {
            "PASS"
        } else {
            "BLOCKED_BY_EVIDENCE"
        }
        .to_owned(),
        comparison_sha256: file_sha256(comparison_path).unwrap_or_else(|_| "MISSING".to_owned()),
        loop_gate_sha256: file_sha256(loop_gate_path).unwrap_or_else(|_| "MISSING".to_owned()),
        active_decision_sha256: file_sha256(active_decision_path)
            .unwrap_or_else(|_| "MISSING".to_owned()),
        loop_gate_replayed: loop_replayed,
        active_sources_replayed: active_replayed,
        tick_proxy_status: stored_comparison
            .as_ref()
            .map(|comparison| comparison.tick_proxy_status.clone())
            .unwrap_or_else(|| "BLOCKED_BY_EVIDENCE".to_owned()),
        direct_post_commit_observed: false,
        authority_commit_status: if passed {
            "PASS_INHERITED_CONTRACT"
        } else {
            "BLOCKED_BY_EVIDENCE"
        }
        .to_owned(),
        composite_no_mutation_passed,
        cs_t00502_passed: passed,
        cs_t00503_unlocked: passed,
        t005_unlocked: false,
        active_input_allowed: false,
        passive_probe_allowed: false,
        runtime_mutation_allowed: false,
        checks,
    };
    write_json_create_new(output, &decision)?;
    Ok(decision)
}

fn replay_loop_gate(path: &Path) -> AppResult<bool> {
    if !file_sha256(path)?.eq_ignore_ascii_case(FIXED_LOOP_GATE_SHA256) {
        return Ok(false);
    }
    let gate: LoopGateDecision = read_json(path, "T0-03R loop gate")?;
    if gate.status != "PASS" || !gate.missing_scenarios.is_empty() {
        return Ok(false);
    }
    let parent = path.parent().unwrap_or_else(|| Path::new("."));
    for binding in &gate.analysis_bindings {
        let analysis_path = parent.join(&binding.analysis_path);
        if !file_sha256(&analysis_path)?.eq_ignore_ascii_case(&binding.analysis_sha256)
            || !file_sha256(&analysis_path.parent().unwrap().join("manifest.json"))?
                .eq_ignore_ascii_case(&binding.manifest_sha256)
            || !file_sha256(&analysis_path.parent().unwrap().join("events.jsonl"))?
                .eq_ignore_ascii_case(&binding.events_sha256)
        {
            return Ok(false);
        }
        let stored: LoopAnalysisReport = read_json(&analysis_path, "bound loop analysis")?;
        let replay = replay_loop_run(analysis_path.parent().unwrap())?;
        if stored != replay || replay.result != "PASS" {
            return Ok(false);
        }
    }
    Ok(true)
}

fn replay_active_decision(path: &Path) -> AppResult<bool> {
    if !file_sha256(path)?.eq_ignore_ascii_case(FIXED_ACTIVE_DECISION_SHA256) {
        return Ok(false);
    }
    let decision: ActiveGateDecision = read_json(path, "T0-04 active decision")?;
    if decision.status != "PASS"
        || !decision.t0_04_unlocked
        || !decision.semantic_contract_complete
        || !decision.runtime_contract_complete
        || !decision.missing_cases.is_empty()
        || !decision
            .loop_gate_sha256
            .eq_ignore_ascii_case(FIXED_LOOP_GATE_SHA256)
    {
        return Ok(false);
    }
    let parent = path.parent().unwrap_or_else(|| Path::new("."));
    for binding in &decision.analysis_bindings {
        let analysis_path = parent.join(&binding.analysis_path);
        if !replay_active_binding(
            &analysis_path,
            &binding.analysis_sha256,
            &binding.manifest_sha256,
            &binding.events_sha256,
        )? {
            return Ok(false);
        }
    }
    for binding in &decision.repeatability_bindings {
        let analysis_path = parent.join(&binding.analysis_path);
        if !replay_active_binding(
            &analysis_path,
            &binding.analysis_sha256,
            &binding.manifest_sha256,
            &binding.events_sha256,
        )? {
            return Ok(false);
        }
    }
    Ok(true)
}

fn replay_active_binding(
    analysis_path: &Path,
    analysis_sha256: &str,
    manifest_sha256: &str,
    events_sha256: &str,
) -> AppResult<bool> {
    if !file_sha256(analysis_path)?.eq_ignore_ascii_case(analysis_sha256)
        || !file_sha256(&analysis_path.parent().unwrap().join("manifest.json"))?
            .eq_ignore_ascii_case(manifest_sha256)
        || !file_sha256(&analysis_path.parent().unwrap().join("events.jsonl"))?
            .eq_ignore_ascii_case(events_sha256)
    {
        return Ok(false);
    }
    let stored: ActiveAnalysisReport = read_json(analysis_path, "bound active analysis")?;
    let replay = replay_active_run(analysis_path.parent().unwrap())?;
    Ok(stored == replay && replay.result == "PASS")
}

fn validate_target_documents(
    expected: &ExpectedTargetDocument,
    before: &ObservedTargetDocument,
    after: &ObservedTargetDocument,
) -> AppResult<()> {
    if expected.schema_version != C0_SCHEMA_VERSION
        || expected.document_identity != "IsaacCsT0-CoopC0ExpectedTargets-v1"
        || before.schema_version != C0_SCHEMA_VERSION
        || after.schema_version != C0_SCHEMA_VERSION
        || before.document_identity != "IsaacCsT0-CoopC0ObservedTargets-v1"
        || after.document_identity != "IsaacCsT0-CoopC0ObservedTargets-v1"
        || before.phase != "BEFORE"
        || after.phase != "AFTER"
        || expected.slot_count != C0_SLOT_COUNT
        || before.slot_count != C0_SLOT_COUNT
        || after.slot_count != C0_SLOT_COUNT
        || expected.slots.len() != C0_SLOT_COUNT
        || before.slots.len() != C0_SLOT_COUNT
        || after.slots.len() != C0_SLOT_COUNT
        || expected.tick_path.len() != 3
        || before.tick_path.len() != 3
        || after.tick_path.len() != 3
    {
        return Err(AppError::evidence(
            "invalid coop C0 target document identity",
        ));
    }
    for ordinal in 0..C0_SLOT_COUNT {
        let expected_slot = &expected.slots[ordinal];
        let before_slot = &before.slots[ordinal];
        let after_slot = &after.slots[ordinal];
        let proof_kind_valid = if matches!(
            expected_slot.patch_kind.as_str(),
            "IMPORT_POINTER" | "IMAGE_POINTER"
        ) {
            before_slot.bytes_proof.starts_with("OPAQUE-")
        } else {
            before_slot.bytes_proof.starts_with("BYTE-")
        };
        if expected_slot.ordinal != ordinal as u32
            || before_slot.ordinal != ordinal as u32
            || after_slot.ordinal != ordinal as u32
            || expected_slot.id != before_slot.id
            || expected_slot.id != after_slot.id
            || expected_slot.rva != before_slot.rva
            || expected_slot.rva != after_slot.rva
            || expected_slot.expected_semantic != before_slot.semantic
            || expected_slot.expected_semantic != after_slot.semantic
            || before_slot.bytes_proof != after_slot.bytes_proof
            || expected_slot.expected_bytes_proof.as_ref() != Some(&before_slot.bytes_proof)
            || !valid_bytes_proof(&before_slot.bytes_proof, expected_slot.width)
            || !proof_kind_valid
        {
            return Err(AppError::evidence(format!(
                "coop C0 target slot {ordinal} differs across expected/before/after"
            )));
        }
    }
    if expected.tick_path != before.tick_path || expected.tick_path != after.tick_path {
        return Err(AppError::evidence(
            "coop C0 steady-standard tick path differs across expected/before/after",
        ));
    }
    if expected.tick_path != fixed_tick_path_contract() {
        return Err(AppError::evidence(
            "coop C0 steady-standard tick path bytes or semantics drifted",
        ));
    }
    Ok(())
}

fn valid_bytes_proof(value: &str, width: u32) -> bool {
    if let Some(bytes) = value.strip_prefix("BYTE-") {
        bytes.len() == width as usize * 2 && bytes.bytes().all(|byte| byte.is_ascii_hexdigit())
    } else if let Some(digest) = value.strip_prefix("OPAQUE-") {
        digest.len() == 32 && digest.bytes().all(|byte| byte.is_ascii_hexdigit())
    } else {
        false
    }
}

fn fixed_tick_path_contract() -> Vec<CodeObservation> {
    vec![
        CodeObservation {
            id: "steady-standard-frame-store".to_owned(),
            rva: "0x002FBA0B".to_owned(),
            width: 6,
            bytes_hex: "89B7F8640200".to_owned(),
            semantic: "GAME_ROOT_FRAME_PRE_ATTEMPT_STORE".to_owned(),
        },
        CodeObservation {
            id: "steady-standard-room-update".to_owned(),
            rva: "0x002FBA4D".to_owned(),
            width: 5,
            bytes_hex: "E82E6F1000".to_owned(),
            semantic: "REL32_TARGET:0x00402980".to_owned(),
        },
        CodeObservation {
            id: "steady-standard-post-tail".to_owned(),
            rva: "0x002FBBA5".to_owned(),
            width: 5,
            bytes_hex: "E8F64B1600".to_owned(),
            semantic: "REL32_TARGET:0x004607A0".to_owned(),
        },
    ]
}

fn scenario_graph_pass(scenario: &str, graph: &C0GraphObservation) -> bool {
    let common = graph.run_active
        && graph.primary_hud_consistent
        && graph.twin_hud_empty
        && graph
            .players
            .iter()
            .map(|player| player.object_token.as_str())
            .collect::<BTreeSet<_>>()
            .len()
            == graph.players.len()
        && graph.players.iter().enumerate().all(|(index, player)| {
            player.ordinal == index as u32
                && player.ordinal_field_matches
                && player.hud_primary_slot_matches
                && player.lua_order_evidence == LUA_STATIC_ORDER
                && player.init_seed_stable
                && player.init_seed_nonzero
                && player.twin_relation_absent
        });
    if !common {
        return false;
    }
    match scenario {
        SCENARIO_OFFLINE_SINGLE => {
            !graph.online_mode
                && !graph.network_backend_present
                && !graph.net_input_present
                && graph.roster_count == 0
                && graph.player_count == 1
                && graph.players.len() == 1
                && graph.players[0].unique_assignment
                && graph.players[0].device_kind.is_some()
        }
        SCENARIO_LOCAL_DUAL => {
            !graph.online_mode
                && !graph.network_backend_present
                && !graph.net_input_present
                && graph.roster_count == 0
                && graph.player_count == 2
                && graph.players.len() == 2
                && graph.players.iter().all(|player| player.unique_assignment)
                && graph
                    .players
                    .iter()
                    .filter_map(|player| player.device_kind.as_deref())
                    .collect::<BTreeSet<_>>()
                    == BTreeSet::from(["GAMEPAD", "KEYBOARD"])
        }
        SCENARIO_ORIGINAL_ONLINE => {
            graph.online_mode
                && graph.manager_present
                && graph.network_backend_present
                && graph.net_input_present
                && graph.roster_count == 2
                && graph.player_count == 2
                && graph.players.len() == 2
        }
        _ => false,
    }
}

fn validate_sample_shape(sample: &C0Sample) -> bool {
    if sample.schema_version != C0_SCHEMA_VERSION
        || sample.observer_sequence == 0
        || sample.qpc <= 0
        || sample.retry_count > MAX_RETRIES
    {
        return false;
    }
    match sample.status.as_str() {
        STATUS_COHERENT => {
            sample.reason.is_none()
                && sample.graph.is_some()
                && sample.game_frame_delta.is_some()
                && sample.graph.as_ref().is_some_and(|graph| {
                    graph.player_count == graph.players.len()
                        && graph.players.iter().all(|player| {
                            !player.object_token.is_empty()
                                && player.object_token.len() <= 40
                                && !player.object_token.starts_with("0x")
                        })
                })
        }
        STATUS_UNSTABLE | STATUS_READ_ERROR => {
            sample.reason.as_deref().is_some_and(valid_reason_code)
                && sample.game_frame_delta.is_none()
                && sample.online_frame_delta.is_none()
                && sample.graph.is_none()
        }
        _ => false,
    }
}

fn valid_reason_code(reason: &str) -> bool {
    !reason.is_empty()
        && reason.len() <= 64
        && reason
            .bytes()
            .all(|byte| byte.is_ascii_uppercase() || byte.is_ascii_digit() || byte == b'_')
}

fn privacy_scan(value: &serde_json::Value) -> AppResult<()> {
    fn visit(value: &serde_json::Value, key: Option<&str>) -> bool {
        match value {
            serde_json::Value::Object(map) => map.iter().all(|(name, value)| {
                let lower = name.to_ascii_lowercase();
                let banned_key = matches!(
                    lower.as_str(),
                    "pid"
                        | "processid"
                        | "modulebase"
                        | "address"
                        | "pointer"
                        | "devicekey"
                        | "assignmentkey"
                        | "platformcontrollerid"
                        | "rawinitseed"
                        | "peeridentity"
                        | "username"
                        | "homepath"
                );
                !banned_key && visit(value, Some(name))
            }),
            serde_json::Value::Array(values) => values.iter().all(|value| visit(value, key)),
            serde_json::Value::String(text) => {
                let lower = text.to_ascii_lowercase();
                let local_path = text.as_bytes().get(1) == Some(&b':')
                    || text.starts_with("\\\\")
                    || lower.starts_with("file://")
                    || lower.contains("\\users\\")
                    || lower.contains("/home/");
                let runtime_hex = text.len() == 10
                    && text.starts_with("0x")
                    && text[2..].bytes().all(|byte| byte.is_ascii_hexdigit())
                    && key.is_none_or(|key| {
                        !key.to_ascii_lowercase().contains("rva")
                            && !key.eq_ignore_ascii_case("observerAccessBits")
                    });
                !local_path && !runtime_hex
            }
            _ => true,
        }
    }
    if visit(value, None) {
        Ok(())
    } else {
        Err(AppError::evidence(
            "coop C0 artifact contains a forbidden runtime identity or local path",
        ))
    }
}

fn check(id: &str, passed: bool, detail: &str) -> C0Check {
    C0Check {
        id: id.to_owned(),
        passed,
        detail: detail.to_owned(),
    }
}

fn read_samples(path: &Path) -> AppResult<Vec<C0Sample>> {
    let text = fs::read_to_string(path)
        .usage_context(format!("cannot read coop C0 samples {}", path.display()))?;
    text.lines()
        .enumerate()
        .filter(|(_, line)| !line.trim().is_empty())
        .map(|(index, line)| {
            serde_json::from_str(line).usage_context(format!(
                "cannot parse coop C0 samples {} line {}",
                path.display(),
                index + 1
            ))
        })
        .collect()
}

fn read_json<T: for<'de> Deserialize<'de>>(path: &Path, label: &str) -> AppResult<T> {
    let bytes = fs::read(path).usage_context(format!("cannot read {label} {}", path.display()))?;
    serde_json::from_slice(&bytes).usage_context(format!("cannot parse {label} {}", path.display()))
}

pub fn write_json_create_new(path: &Path, value: &impl Serialize) -> AppResult<()> {
    let file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .usage_context(format!("cannot create {}", path.display()))?;
    serde_json::to_writer_pretty(BufWriter::new(file), value)
        .usage_context(format!("cannot serialize {}", path.display()))
}

fn sha256_bytes(bytes: &[u8]) -> String {
    let mut hasher = Sha256::new();
    hasher.update(bytes);
    hex::encode_upper(hasher.finalize())
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
struct DeviceClassification {
    kind: &'static str,
    read_assignment_key: bool,
}

fn classify_c0_device(
    scenario: &str,
    in_online_vector: bool,
    known_physical_kind: Option<&'static str>,
) -> Option<DeviceClassification> {
    if scenario == SCENARIO_ORIGINAL_ONLINE && in_online_vector {
        Some(DeviceClassification {
            kind: "NET_INPUT",
            read_assignment_key: false,
        })
    } else {
        known_physical_kind.map(|kind| DeviceClassification {
            kind,
            read_assignment_key: true,
        })
    }
}

fn peer_identity_summary_valid(peer_identities: &[u64], roster_count: usize) -> bool {
    peer_identities.len() == roster_count
        && peer_identities.iter().all(|identity| *identity != 0)
        && peer_identities.iter().collect::<BTreeSet<_>>().len() == peer_identities.len()
}

#[cfg(windows)]
mod platform {
    use std::thread;
    use std::time::{Duration, Instant};

    use crate::active_catalog::{HookSlot, expected_hook_slots};
    use crate::pe::PeImage;
    use crate::target_equivalence::TargetEquivalenceDocument;
    use crate::win32::{self, ObservedProcess};

    use super::*;

    const INPUT_MANAGER_RVA: u64 = 0x0085_7B18;
    const GAME_ROOT_SLOT_RVA: u64 = 0x0087_1678;
    const ONLINE_MANAGER_SLOT_RVA: u64 = 0x0087_169C;
    const NETWORK_BACKEND_SLOT_RVA: u64 = 0x0087_9A58;
    const STEAM_BACKEND_VTABLE_RVA: u64 = 0x007A_2440;
    const GAME_PLAYER_VECTOR_OFFSET: u64 = 0x0001_BAA8;
    const GAME_RUN_ACTIVE_OFFSET: u64 = 0x0002_658A;
    const GAME_FRAME_OFFSET: u64 = 0x0002_64F8;
    const HUD_OFFSET: u64 = 0x0001_DA04;
    const HUD_STRIDE: u64 = 0x6DC;
    const PLAYER_VTABLE_RVA: u64 = 0x0076_BDD0;
    const PLAYER_COL_RVA: u64 = 0x007A_E31C;
    const PLAYER_INIT_SEED_OFFSET: u64 = 0x03EC;
    const PLAYER_DEVICE_KEY_OFFSET: u64 = 0x1618;
    const PLAYER_ORDINAL_OFFSET: u64 = 0x161C;
    const PLAYER_TWIN_OFFSET: u64 = 0x1E68;
    const DEVICE_KEY_OFFSET: u64 = 0x0C;
    const SYNC_MANAGER_OFFSET: u64 = 0x0004_B3D8;
    const ONLINE_FRAME_OFFSET_FROM_SYNC: u64 = 0x0C;
    const NET_INPUT_IDENTITY_OBJECT_OFFSET: u64 = 0x0374;
    const PEER_IDENTITY_VALUE_OFFSET: u64 = 0x08;
    const MAX_DEVICE_COUNT: usize = 32;
    const MAX_PLAYER_COUNT: usize = 4;
    const MAX_ROSTER_COUNT: usize = 4;

    const DEVICE_PROFILES: [(&str, u64); 2] = [("KEYBOARD", 0x007A_1678), ("GAMEPAD", 0x0078_2C80)];

    #[derive(Clone, Copy, Debug, Eq, PartialEq)]
    struct VectorHeader {
        begin: u32,
        end: u32,
        capacity_end: u32,
    }

    #[derive(Clone, Debug, Eq, PartialEq)]
    struct RawDevice {
        object: u32,
        reference_owner: u32,
        kind: String,
        assignment_key: i32,
    }

    #[derive(Clone, Debug, Eq, PartialEq)]
    struct RawPlayer {
        object: u32,
        init_seed: u32,
        assignment_key: i32,
        ordinal: u32,
        twin: u32,
    }

    #[derive(Clone, Debug, Eq, PartialEq)]
    struct RawMode {
        game_root: u32,
        run_active: bool,
        manager: u32,
        network_backend: u32,
        roster: Vec<u32>,
        peer_identities: Vec<u64>,
        game_frame: u32,
        online_frame: u32,
    }

    #[derive(Clone, Debug, Eq, PartialEq)]
    struct RawTopology {
        mode: RawMode,
        devices: Vec<RawDevice>,
        players: Vec<RawPlayer>,
        hud: [u32; 8],
    }

    enum AttemptFailure {
        Unstable(&'static str),
        Read(&'static str),
    }

    struct Tokenizer {
        secret: [u8; 32],
    }

    impl Tokenizer {
        fn new(process: &ObservedProcess) -> AppResult<Self> {
            let mut hasher = Sha256::new();
            hasher.update(b"IsaacCsT0-CoopC0-ephemeral-token-v1");
            hasher.update(process.pid.to_le_bytes());
            hasher.update(process.creation_time_100ns.to_le_bytes());
            hasher.update(win32::qpc_now()?.to_le_bytes());
            hasher.update(utc_now().as_bytes());
            Ok(Self {
                secret: hasher.finalize().into(),
            })
        }

        fn token(&self, domain: &[u8], value: u32) -> String {
            let mut hasher = Sha256::new();
            hasher.update(self.secret);
            hasher.update(domain);
            hasher.update(value.to_le_bytes());
            format!("O-{}", &hex::encode_upper(hasher.finalize())[..24])
        }

        fn bytes_proof(&self, domain: &[u8], ordinal: u32, bytes: &[u8]) -> String {
            let mut hasher = Sha256::new();
            hasher.update(self.secret);
            hasher.update(domain);
            hasher.update(ordinal.to_le_bytes());
            hasher.update(bytes);
            format!("OPAQUE-{}", &hex::encode_upper(hasher.finalize())[..32])
        }
    }

    pub(super) fn capture(request: &CaptureRequest<'_>) -> AppResult<C0CaptureSummary> {
        validate_capture_options(
            request.probe,
            request.scenario,
            request.case_id,
            request.duration_milliseconds,
            request.interval_milliseconds,
        )?;
        if !request.artifacts_root.is_dir() {
            return Err(AppError::usage(format!(
                "artifacts root does not exist: {}",
                request.artifacts_root.display()
            )));
        }
        let output_directory = request.artifacts_root.join(request.case_id);
        if output_directory.exists() {
            return Err(AppError::usage(format!(
                "coop C0 output directory already exists: {}",
                output_directory.display()
            )));
        }
        load_and_validate_plan(
            request.plan,
            request.target_equivalence,
            request.layout_evidence,
            request.catalog,
            request.graph,
        )?;
        let equivalence: TargetEquivalenceDocument =
            read_json(request.target_equivalence, "target equivalence")?;
        if !file_sha256(request.target_equivalence)?
            .eq_ignore_ascii_case(FIXED_TARGET_EQUIVALENCE_SHA256)
            || equivalence.equivalence_identity != "IsaacCsT0-UnpackedTargetEquivalence-v1"
            || equivalence.status != "PASS_EQUIVALENT_READ_ONLY"
            || !equivalence.static_proof_complete
        {
            return Err(AppError::evidence("invalid target equivalence for coop C0"));
        }
        let catalog_bytes = fs::read(request.catalog)
            .usage_context(format!("cannot read {}", request.catalog.display()))?;
        let graph_bytes = fs::read(request.graph)
            .usage_context(format!("cannot read {}", request.graph.display()))?;
        let catalog = crate::coop_static::parse_catalog_json(&catalog_bytes)?;
        let graph = crate::coop_static::parse_graph_json(&graph_bytes)?;
        crate::coop_static::validate_catalog(&catalog, &graph)?;
        let canonical_game = fs::canonicalize(request.game_exe).usage_context(format!(
            "cannot resolve game executable {}",
            request.game_exe.display()
        ))?;
        let disk_hash_before = file_sha256(&canonical_game)?;
        if !disk_hash_before.eq_ignore_ascii_case(&equivalence.observed_target_sha256) {
            return Err(AppError::evidence(
                "game executable does not match approved unpacked target",
            ));
        }
        if !win32::running_blockers()?.is_empty() {
            return Err(AppError::evidence(
                "coop C0 refuses debugger, Cheat Engine, or conflicting sync processes",
            ));
        }
        let pids = win32::find_target_pids()?;
        if pids.len() != 1 {
            return Err(AppError::evidence(format!(
                "coop C0 requires exactly one isaac-ng.exe process, found {}",
                pids.len()
            )));
        }
        let process = ObservedProcess::open_exact(pids[0], &canonical_game)?;
        require_clean(&process)?;
        crate::input_rpm::validate_runtime_profile(&process)?;
        let image = PeImage::read(&canonical_game)
            .map_err(|error| AppError::evidence(error.to_string()))?;
        let tokenizer = Tokenizer::new(&process)?;
        let process_identity_token = tokenizer.token(b"process", process.pid);
        let module_digest_before = module_digest(&process)?;
        let mut expected_targets = expected_targets(&image, &catalog)?;
        let targets_before = observed_targets(&process, &image, &catalog, &tokenizer, "BEFORE")?;
        materialize_pointer_proofs(&mut expected_targets, &targets_before)?;

        fs::create_dir(&output_directory).usage_context(format!(
            "cannot create coop C0 output directory {}",
            output_directory.display()
        ))?;
        let expected_path = output_directory.join("target-bytes-expected.json");
        let before_path = output_directory.join("target-bytes-before.json");
        let after_path = output_directory.join("target-bytes-after.json");
        let samples_path = output_directory.join("samples.jsonl");
        write_json_create_new(&expected_path, &expected_targets)?;
        write_json_create_new(&before_path, &targets_before)?;
        let samples_file = OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(&samples_path)
            .usage_context(format!("cannot create {}", samples_path.display()))?;
        let mut writer = BufWriter::new(samples_file);
        let started_utc = utc_now();
        let start = Instant::now();
        let deadline = start + Duration::from_millis(request.duration_milliseconds);
        let interval = Duration::from_millis(request.interval_milliseconds);
        let mut sequence = 1_u64;
        let mut base_game_frame = None;
        let mut base_online_frame = None;
        let mut event_count = 0_usize;
        let mut coherent_samples = 0_usize;
        let mut unstable_drops = 0_usize;
        let mut read_error_drops = 0_usize;
        loop {
            require_clean(&process)?;
            let sample = collect_with_retries(
                &process,
                request.scenario,
                sequence,
                &tokenizer,
                &mut base_game_frame,
                &mut base_online_frame,
            )?;
            match sample.status.as_str() {
                STATUS_COHERENT => coherent_samples += 1,
                STATUS_UNSTABLE => unstable_drops += 1,
                _ => read_error_drops += 1,
            }
            serde_json::to_writer(&mut writer, &sample)
                .usage_context(format!("cannot serialize {}", samples_path.display()))?;
            writer
                .write_all(b"\n")
                .usage_context(format!("cannot write {}", samples_path.display()))?;
            event_count += 1;
            sequence = sequence
                .checked_add(1)
                .ok_or_else(|| AppError::evidence("coop C0 sequence overflow"))?;
            if Instant::now() >= deadline || process.wait(0)? {
                break;
            }
            thread::sleep(interval);
        }
        writer
            .flush()
            .usage_context(format!("cannot flush {}", samples_path.display()))?;
        require_clean(&process)?;
        let targets_after = observed_targets(&process, &image, &catalog, &tokenizer, "AFTER")?;
        write_json_create_new(&after_path, &targets_after)?;
        validate_target_documents(&expected_targets, &targets_before, &targets_after)?;
        let reopened = ObservedProcess::open_exact(process.pid, &canonical_game)?;
        require_clean(&reopened)?;
        let process_identity_stable = reopened.creation_time_100ns == process.creation_time_100ns;
        let module_digest_after = module_digest(&reopened)?;
        let module_set_stable = module_digest_after == module_digest_before;
        let disk_hash_after = file_sha256(&canonical_game)?;
        let target_disk_stable = disk_hash_after == disk_hash_before;
        let artifact_identity =
            tokenizer.token(b"artifact", u32::try_from(event_count).unwrap_or(u32::MAX));
        let status = if coherent_samples >= 2
            && process_identity_stable
            && module_set_stable
            && target_disk_stable
        {
            "CAPTURE_COMPLETE"
        } else {
            "BLOCKED_BY_EVIDENCE"
        };
        let manifest = C0Manifest {
            schema_version: C0_SCHEMA_VERSION,
            evidence_class: C0_EVIDENCE_CLASS.to_owned(),
            tool_identity: C0_TOOL_IDENTITY.to_owned(),
            artifact_identity: artifact_identity.clone(),
            work_package: C0_WORK_PACKAGE.to_owned(),
            case_id: request.case_id.to_owned(),
            probe: request.probe.to_owned(),
            scenario: request.scenario.to_owned(),
            started_utc,
            ended_utc: utc_now(),
            target_sha256: disk_hash_before,
            target_equivalence_sha256: file_sha256(request.target_equivalence)?,
            layout_evidence_sha256: file_sha256(request.layout_evidence)?,
            coop_catalog_sha256: file_sha256(request.catalog)?,
            coop_graph_sha256: file_sha256(request.graph)?,
            coop_plan_sha256: file_sha256(request.plan)?,
            observer_access_bits: format!("0x{:08X}", win32::OBSERVER_ACCESS_BITS),
            requested_duration_milliseconds: request.duration_milliseconds,
            interval_milliseconds: request.interval_milliseconds,
            max_retries: MAX_RETRIES,
            qpc_frequency: win32::qpc_frequency()?,
            process_identity_token,
            module_set_sha256: module_digest_before,
            blockers_absent: true,
            debugger_absent: true,
            forbidden_modules_absent: true,
            process_identity_stable,
            module_set_stable,
            target_disk_stable,
            expected_targets_sha256: file_sha256(&expected_path)?,
            target_bytes_before_sha256: file_sha256(&before_path)?,
            target_bytes_after_sha256: file_sha256(&after_path)?,
            samples_sha256: file_sha256(&samples_path)?,
            hook_installed: false,
            ipc_created: false,
            mapping_created: false,
            helper_started: false,
            native_loaded: false,
            scheduler_started: false,
            apply_attempted: false,
            status: status.to_owned(),
        };
        write_json_create_new(&output_directory.join("manifest.json"), &manifest)?;
        Ok(C0CaptureSummary {
            schema_version: C0_SCHEMA_VERSION,
            evidence_class: C0_EVIDENCE_CLASS.to_owned(),
            result: status.to_owned(),
            scenario: request.scenario.to_owned(),
            artifact_identity,
            output_directory,
            event_count,
            coherent_samples,
            unstable_drops,
            read_error_drops,
            active_input_allowed: false,
            runtime_mutation_allowed: false,
        })
    }

    fn collect_with_retries(
        process: &ObservedProcess,
        scenario: &str,
        sequence: u64,
        tokenizer: &Tokenizer,
        base_game_frame: &mut Option<u32>,
        base_online_frame: &mut Option<u32>,
    ) -> AppResult<C0Sample> {
        let qpc = win32::qpc_now()?;
        let mut last = AttemptFailure::Unstable("NO_COHERENT_ATTEMPT");
        for retry in 0..=MAX_RETRIES {
            match collect_attempt(process, scenario, tokenizer) {
                Ok((graph, game_frame, online_frame)) => {
                    let game_base = *base_game_frame.get_or_insert(game_frame);
                    let online_base = *base_online_frame.get_or_insert(online_frame);
                    let game_frame_delta = game_frame.checked_sub(game_base).ok_or_else(|| {
                        AppError::evidence("coop C0 game frame regressed within one source")
                    })?;
                    let online_frame_delta =
                        online_frame.checked_sub(online_base).ok_or_else(|| {
                            AppError::evidence("coop C0 online frame regressed within one source")
                        })?;
                    return Ok(C0Sample {
                        schema_version: C0_SCHEMA_VERSION,
                        observer_sequence: sequence,
                        qpc,
                        status: STATUS_COHERENT.to_owned(),
                        retry_count: retry,
                        reason: None,
                        game_frame_delta: Some(game_frame_delta),
                        online_frame_delta: (scenario == SCENARIO_ORIGINAL_ONLINE)
                            .then_some(online_frame_delta),
                        graph: Some(graph),
                    });
                }
                Err(failure) => last = failure,
            }
        }
        let (status, reason) = match last {
            AttemptFailure::Unstable(reason) => (STATUS_UNSTABLE, reason),
            AttemptFailure::Read(reason) => (STATUS_READ_ERROR, reason),
        };
        Ok(C0Sample {
            schema_version: C0_SCHEMA_VERSION,
            observer_sequence: sequence,
            qpc,
            status: status.to_owned(),
            retry_count: MAX_RETRIES,
            reason: Some(reason.to_owned()),
            game_frame_delta: None,
            online_frame_delta: None,
            graph: None,
        })
    }

    fn collect_attempt(
        process: &ObservedProcess,
        scenario: &str,
        tokenizer: &Tokenizer,
    ) -> Result<(C0GraphObservation, u32, u32), AttemptFailure> {
        let forward = read_topology(process, scenario, false)?;
        let reverse = read_topology(process, scenario, true)?;
        if !topology_stable(&forward, &reverse) {
            return Err(AttemptFailure::Unstable("STRUCTURAL_DOUBLE_READ_DRIFT"));
        }
        if reverse.mode.game_frame < forward.mode.game_frame
            || reverse.mode.online_frame < forward.mode.online_frame
        {
            return Err(AttemptFailure::Unstable("FRAME_REGRESSION"));
        }
        let graph = project_graph(&forward, scenario, tokenizer)?;
        Ok((graph, reverse.mode.game_frame, reverse.mode.online_frame))
    }

    fn topology_stable(left: &RawTopology, right: &RawTopology) -> bool {
        left.devices == right.devices
            && left.players == right.players
            && left.hud == right.hud
            && left.mode.game_root == right.mode.game_root
            && left.mode.run_active == right.mode.run_active
            && left.mode.manager == right.mode.manager
            && left.mode.network_backend == right.mode.network_backend
            && left.mode.roster == right.mode.roster
            && left.mode.peer_identities == right.mode.peer_identities
    }

    fn read_topology(
        process: &ObservedProcess,
        scenario: &str,
        reverse: bool,
    ) -> Result<RawTopology, AttemptFailure> {
        let mode_before = read_mode(process)?;
        if mode_before.game_root == 0 || !mode_before.run_active {
            return Err(AttemptFailure::Unstable("RUN_NOT_ACTIVE"));
        }
        let (devices, players, hud) = if reverse {
            let (players, hud) = read_players_and_hud(process, mode_before.game_root)?;
            let devices = read_devices(process, scenario, &mode_before.roster)?;
            (devices, players, hud)
        } else {
            let devices = read_devices(process, scenario, &mode_before.roster)?;
            let (players, hud) = read_players_and_hud(process, mode_before.game_root)?;
            (devices, players, hud)
        };
        let mode_after = read_mode(process)?;
        if mode_before.game_root != mode_after.game_root
            || mode_before.run_active != mode_after.run_active
            || mode_before.manager != mode_after.manager
            || mode_before.network_backend != mode_after.network_backend
            || mode_before.roster != mode_after.roster
            || mode_before.peer_identities != mode_after.peer_identities
            || mode_after.game_frame < mode_before.game_frame
            || mode_after.online_frame < mode_before.online_frame
        {
            return Err(AttemptFailure::Unstable("MODE_ENVELOPE_DRIFT"));
        }
        Ok(RawTopology {
            mode: mode_after,
            devices,
            players,
            hud,
        })
    }

    fn read_devices(
        process: &ObservedProcess,
        scenario: &str,
        online_roster: &[u32],
    ) -> Result<Vec<RawDevice>, AttemptFailure> {
        let manager = add(process.main_module.base, INPUT_MANAGER_RVA)?;
        let header = read_vector(process, add(manager, 8)?, 8, MAX_DEVICE_COUNT)?;
        let bytes = read_span(process, header, 8)?;
        let mut devices = Vec::new();
        for entry in bytes.chunks_exact(8) {
            let object = u32::from_le_bytes(entry[0..4].try_into().unwrap());
            let reference_owner = u32::from_le_bytes(entry[4..8].try_into().unwrap());
            if object == 0 || reference_owner == 0 || object & 3 != 0 || reference_owner & 3 != 0 {
                return Err(AttemptFailure::Unstable("INVALID_DEVICE_SHARED_POINTER"));
            }
            let in_online_vector = online_roster.contains(&object);
            let vtable = read_u32(process, u64::from(object))?;
            let known_kind = DEVICE_PROFILES.iter().find_map(|(kind, rva)| {
                add(process.main_module.base, *rva)
                    .ok()
                    .is_some_and(|expected| u64::from(vtable) == expected)
                    .then_some(*kind)
            });
            let classification = classify_c0_device(scenario, in_online_vector, known_kind)
                .ok_or(AttemptFailure::Unstable("UNKNOWN_DEVICE_VTABLE"))?;
            let (kind, assignment_key) = if !classification.read_assignment_key {
                // The frozen OriginalOnline screen identifies NetInputDevice by membership in
                // manager+0x4B3D8, not by an inferred vtable. It is deliberately ineligible for
                // the local Player/device assignment join, so no device key is read.
                (classification.kind.to_owned(), i32::MIN)
            } else {
                let assignment_key = read_i32(process, add(u64::from(object), DEVICE_KEY_OFFSET)?)?;
                (classification.kind.to_owned(), assignment_key)
            };
            devices.push(RawDevice {
                object,
                reference_owner,
                kind,
                assignment_key,
            });
        }
        Ok(devices)
    }

    fn read_players_and_hud(
        process: &ObservedProcess,
        game_root: u32,
    ) -> Result<(Vec<RawPlayer>, [u32; 8]), AttemptFailure> {
        let vector = add(u64::from(game_root), GAME_PLAYER_VECTOR_OFFSET)?;
        let header = read_vector(process, vector, 4, MAX_PLAYER_COUNT)?;
        let bytes = read_span(process, header, 4)?;
        let mut players = Vec::new();
        for raw in bytes.chunks_exact(4) {
            let object = u32::from_le_bytes(raw.try_into().unwrap());
            if object == 0 || object & 3 != 0 {
                return Err(AttemptFailure::Unstable("INVALID_PLAYER_POINTER"));
            }
            let vtable = read_u32(process, u64::from(object))?;
            if u64::from(vtable) != add(process.main_module.base, PLAYER_VTABLE_RVA)? {
                return Err(AttemptFailure::Unstable("UNKNOWN_PLAYER_VTABLE"));
            }
            let col = read_u32(process, u64::from(vtable) - 4)?;
            if u64::from(col) != add(process.main_module.base, PLAYER_COL_RVA)? {
                return Err(AttemptFailure::Unstable("UNKNOWN_PLAYER_COL"));
            }
            players.push(RawPlayer {
                object,
                init_seed: read_u32(process, add(u64::from(object), PLAYER_INIT_SEED_OFFSET)?)?,
                assignment_key: read_i32(
                    process,
                    add(u64::from(object), PLAYER_DEVICE_KEY_OFFSET)?,
                )?,
                ordinal: read_u32(process, add(u64::from(object), PLAYER_ORDINAL_OFFSET)?)?,
                twin: read_u32(process, add(u64::from(object), PLAYER_TWIN_OFFSET)?)?,
            });
        }
        if players
            .iter()
            .map(|player| player.object)
            .collect::<BTreeSet<_>>()
            .len()
            != players.len()
        {
            return Err(AttemptFailure::Unstable("DUPLICATE_PLAYER_OBJECT"));
        }
        let mut hud = [0_u32; 8];
        let base = add(u64::from(game_root), HUD_OFFSET)?;
        for (index, value) in hud.iter_mut().enumerate() {
            *value = read_u32(process, add(base, HUD_STRIDE * index as u64)?)?;
        }
        Ok((players, hud))
    }

    fn read_mode(process: &ObservedProcess) -> Result<RawMode, AttemptFailure> {
        let base = process.main_module.base;
        let game_root = read_u32(process, add(base, GAME_ROOT_SLOT_RVA)?)?;
        let (run_active, game_frame) = if game_root == 0 {
            (false, 0)
        } else {
            let run = process
                .read_memory(add(u64::from(game_root), GAME_RUN_ACTIVE_OFFSET)?, 2)
                .map_err(|_| AttemptFailure::Read("READ_RUN_STATE"))?;
            (
                u16::from_le_bytes(run.try_into().unwrap()) & 0x00FF != 0,
                read_u32(process, add(u64::from(game_root), GAME_FRAME_OFFSET)?)?,
            )
        };
        let manager = read_u32(process, add(base, ONLINE_MANAGER_SLOT_RVA)?)?;
        let network_backend = read_u32(process, add(base, NETWORK_BACKEND_SLOT_RVA)?)?;
        let (roster, peer_identities, online_frame) = if manager == 0 {
            (Vec::new(), Vec::new(), 0)
        } else {
            let sync = add(u64::from(manager), SYNC_MANAGER_OFFSET)?;
            let header = read_vector(process, sync, 4, MAX_ROSTER_COUNT)?;
            let bytes = read_span(process, header, 4)?;
            let roster = bytes
                .chunks_exact(4)
                .map(|raw| u32::from_le_bytes(raw.try_into().unwrap()))
                .collect::<Vec<_>>();
            if roster.iter().any(|item| *item == 0 || *item & 3 != 0)
                || roster.iter().collect::<BTreeSet<_>>().len() != roster.len()
            {
                return Err(AttemptFailure::Unstable("INVALID_ONLINE_ROSTER"));
            }
            let mut peer_identities = Vec::with_capacity(roster.len());
            for net_input in &roster {
                let identity_object = read_u32(
                    process,
                    add(u64::from(*net_input), NET_INPUT_IDENTITY_OBJECT_OFFSET)?,
                )?;
                if identity_object == 0 || identity_object & 3 != 0 {
                    return Err(AttemptFailure::Unstable("INVALID_PEER_IDENTITY_OBJECT"));
                }
                let identity = read_u64(
                    process,
                    add(u64::from(identity_object), PEER_IDENTITY_VALUE_OFFSET)?,
                )?;
                if identity == 0 {
                    return Err(AttemptFailure::Unstable("INVALID_PEER_IDENTITY"));
                }
                peer_identities.push(identity);
            }
            peer_identities.sort_unstable();
            if !peer_identity_summary_valid(&peer_identities, roster.len()) {
                return Err(AttemptFailure::Unstable("INVALID_PEER_IDENTITY_SUMMARY"));
            }
            (
                roster,
                peer_identities,
                read_u32(process, add(sync, ONLINE_FRAME_OFFSET_FROM_SYNC)?)?,
            )
        };
        if network_backend != 0 {
            let vtable = read_u32(process, u64::from(network_backend))?;
            if u64::from(vtable) != add(base, STEAM_BACKEND_VTABLE_RVA)? {
                return Err(AttemptFailure::Unstable("UNKNOWN_NETWORK_BACKEND"));
            }
        }
        if online_frame == u32::MAX {
            return Err(AttemptFailure::Unstable("INVALID_ONLINE_FRAME"));
        }
        Ok(RawMode {
            game_root,
            run_active,
            manager,
            network_backend,
            roster,
            peer_identities,
            game_frame,
            online_frame,
        })
    }

    fn project_graph(
        raw: &RawTopology,
        scenario: &str,
        tokenizer: &Tokenizer,
    ) -> Result<C0GraphObservation, AttemptFailure> {
        let online_mode = raw.mode.network_backend != 0 || !raw.mode.roster.is_empty();
        match scenario {
            SCENARIO_OFFLINE_SINGLE | SCENARIO_LOCAL_DUAL => {
                if online_mode || raw.mode.online_frame != 0 {
                    return Err(AttemptFailure::Unstable("OFFLINE_SCREEN_POSITIVE"));
                }
            }
            SCENARIO_ORIGINAL_ONLINE => {
                if !online_mode
                    || raw.mode.manager == 0
                    || raw.mode.network_backend == 0
                    || raw.mode.roster.len() != 2
                    || raw.mode.online_frame == 0
                {
                    return Err(AttemptFailure::Unstable("ONLINE_SCREEN_NOT_DUAL"));
                }
            }
            _ => return Err(AttemptFailure::Unstable("UNKNOWN_SCENARIO")),
        }
        if raw.players.len() > 2 {
            return Err(AttemptFailure::Unstable("UNEXPECTED_THIRD_PLAYER"));
        }
        if raw.hud[4..].iter().any(|value| *value != 0)
            || raw.players.iter().any(|player| player.twin != 0)
        {
            return Err(AttemptFailure::Unstable("TWIN_OR_POSSESSOR_PRESENT"));
        }
        let distinct_seeds = raw
            .players
            .iter()
            .map(|player| player.init_seed)
            .collect::<BTreeSet<_>>()
            .len()
            == raw.players.len();
        let mut players = Vec::new();
        let mut assigned_device_objects = BTreeSet::new();
        for (index, player) in raw.players.iter().enumerate() {
            let matches = if scenario == SCENARIO_ORIGINAL_ONLINE {
                Vec::new()
            } else {
                raw.devices
                    .iter()
                    .filter(|device| {
                        player.assignment_key >= 0 && device.assignment_key == player.assignment_key
                    })
                    .collect::<Vec<_>>()
            };
            if matches.len() == 1 {
                assigned_device_objects.insert(matches[0].object);
            }
            let hud_matches = raw.hud[..4]
                .iter()
                .enumerate()
                .filter(|(_, object)| **object == player.object)
                .map(|(slot, _)| slot)
                .collect::<Vec<_>>();
            players.push(C0PlayerObservation {
                ordinal: index as u32,
                object_token: tokenizer.token(b"player", player.object),
                device_kind: if matches.len() == 1 {
                    Some(matches[0].kind.clone())
                } else {
                    None
                },
                unique_assignment: matches.len() == 1,
                ordinal_field_matches: player.ordinal == index as u32,
                hud_primary_slot_matches: hud_matches == [index],
                lua_order_evidence: LUA_STATIC_ORDER.to_owned(),
                init_seed_stable: true,
                init_seed_nonzero: player.init_seed != 0,
                init_seed_distinct: distinct_seeds,
                twin_relation_absent: player.twin == 0,
            });
        }
        let mut unassigned_device_kinds = raw
            .devices
            .iter()
            .filter(|device| !assigned_device_objects.contains(&device.object))
            .map(|device| device.kind.clone())
            .collect::<Vec<_>>();
        unassigned_device_kinds.sort();
        let primary_hud_consistent = raw.hud[..raw.players.len()]
            .iter()
            .zip(&raw.players)
            .all(|(hud, player)| *hud == player.object)
            && raw.hud[raw.players.len()..4]
                .iter()
                .all(|value| *value == 0);
        let mut topology_projection = BTreeMap::new();
        topology_projection.insert("scenario", scenario.to_owned());
        topology_projection.insert("online", online_mode.to_string());
        topology_projection.insert("players", raw.players.len().to_string());
        topology_projection.insert("devices", raw.devices.len().to_string());
        topology_projection.insert("roster", raw.mode.roster.len().to_string());
        topology_projection.insert(
            "gameRootToken",
            tokenizer.token(b"game-root", raw.mode.game_root),
        );
        topology_projection.insert(
            "managerToken",
            if raw.mode.manager != 0 {
                tokenizer.token(b"online-manager", raw.mode.manager)
            } else {
                "NONE".to_owned()
            },
        );
        topology_projection.insert(
            "backendToken",
            if raw.mode.network_backend != 0 {
                tokenizer.token(b"network-backend", raw.mode.network_backend)
            } else {
                "NONE".to_owned()
            },
        );
        topology_projection.insert(
            "deviceTokens",
            raw.devices
                .iter()
                .map(|device| {
                    format!(
                        "{}:{}:{}",
                        tokenizer.token(b"device", device.object),
                        tokenizer.token(b"assignment", device.assignment_key as u32),
                        device.kind
                    )
                })
                .collect::<Vec<_>>()
                .join("|"),
        );
        topology_projection.insert(
            "rosterTokens",
            raw.mode
                .roster
                .iter()
                .map(|entry| tokenizer.token(b"online-roster", *entry))
                .collect::<Vec<_>>()
                .join("|"),
        );
        topology_projection.insert(
            "playerTokens",
            players
                .iter()
                .map(|player| player.object_token.as_str())
                .collect::<Vec<_>>()
                .join(","),
        );
        topology_projection.insert(
            "assignments",
            players
                .iter()
                .map(|player| {
                    format!(
                        "{}:{}:{}:{}",
                        player.ordinal,
                        player.device_kind.as_deref().unwrap_or("NONE"),
                        player.unique_assignment,
                        player.hud_primary_slot_matches
                    )
                })
                .collect::<Vec<_>>()
                .join("|"),
        );
        let topology_bytes = serde_json::to_vec(&topology_projection)
            .map_err(|_| AttemptFailure::Read("ENCODE_TOPOLOGY"))?;
        Ok(C0GraphObservation {
            topology_sha256: sha256_bytes(&topology_bytes),
            run_active: raw.mode.run_active,
            online_mode,
            manager_present: raw.mode.manager != 0,
            network_backend_present: raw.mode.network_backend != 0,
            net_input_present: !raw.mode.roster.is_empty(),
            player_count: raw.players.len(),
            device_count: raw.devices.len(),
            roster_count: raw.mode.roster.len(),
            players,
            unassigned_device_kinds,
            primary_hud_consistent,
            twin_hud_empty: true,
        })
    }

    fn expected_targets(
        image: &PeImage,
        catalog: &CoopCatalog,
    ) -> AppResult<ExpectedTargetDocument> {
        let mut slots = Vec::with_capacity(C0_SLOT_COUNT);
        for slot in expected_hook_slots() {
            slots.push(expected_active_slot(image, &slot)?);
        }
        for slot in &catalog.inherited_hook_transaction.new_slots {
            slots.push(ExpectedTargetSlot {
                ordinal: u32::try_from(slot.ordinal)
                    .map_err(|_| AppError::evidence("coop slot ordinal overflow"))?,
                id: slot.id.clone(),
                owner: slot.owner.clone(),
                patch_kind: slot.patch_kind.clone(),
                rva: slot.rva.clone(),
                width: u32::try_from(slot.width)
                    .map_err(|_| AppError::evidence("coop slot width overflow"))?,
                expected_bytes_proof: Some(format!("BYTE-{}", slot.expected_bytes)),
                expected_semantic: format!("REL32_TARGET:{}", slot.target_rva),
            });
        }
        slots.sort_by_key(|slot| slot.ordinal);
        if slots.len() != C0_SLOT_COUNT {
            return Err(AppError::evidence(
                "coop C0 expected target count is not 53",
            ));
        }
        let tick_path = expected_tick_path(image)?;
        let document = ExpectedTargetDocument {
            schema_version: C0_SCHEMA_VERSION,
            document_identity: "IsaacCsT0-CoopC0ExpectedTargets-v1".to_owned(),
            slot_count: slots.len(),
            slots,
            tick_path,
        };
        Ok(document)
    }

    fn expected_active_slot(image: &PeImage, slot: &HookSlot) -> AppResult<ExpectedTargetSlot> {
        let rva = parse_rva(&slot.rva)?;
        let width = usize::try_from(slot.width)
            .map_err(|_| AppError::evidence("active slot width overflow"))?;
        let expected_bytes_proof =
            if matches!(slot.patch_kind.as_str(), "IMPORT_POINTER" | "IMAGE_POINTER") {
                None
            } else {
                Some(format!(
                    "BYTE-{}",
                    hex::encode_upper(
                        image
                            .bytes_at_rva(rva, width)
                            .map_err(|error| AppError::evidence(error.to_string()))?
                    )
                ))
            };
        let expected_semantic = match slot.patch_kind.as_str() {
            "REL32_CALL" => format!(
                "REL32_TARGET:{}",
                slot.expected_target_rva.as_deref().unwrap_or("MISSING")
            ),
            "IMAGE_POINTER" => format!(
                "IMAGE_TARGET:{}",
                slot.expected_target_rva.as_deref().unwrap_or("MISSING")
            ),
            "IMPORT_POINTER" => format!(
                "IMPORT_SLOT:{}:LIVE_OWNER:USER32.dll:SYMBOL_ADDRESS_UNRESOLVED",
                slot.expected_symbol.as_deref().unwrap_or("MISSING")
            ),
            _ => return Err(AppError::evidence("unknown active slot patch kind")),
        };
        Ok(ExpectedTargetSlot {
            ordinal: slot.ordinal,
            id: slot.id.clone(),
            owner: slot.owner.clone(),
            patch_kind: slot.patch_kind.clone(),
            rva: slot.rva.clone(),
            width: slot.width,
            expected_bytes_proof,
            expected_semantic,
        })
    }

    fn materialize_pointer_proofs(
        expected: &mut ExpectedTargetDocument,
        before: &ObservedTargetDocument,
    ) -> AppResult<()> {
        if expected.slots.len() != before.slots.len() {
            return Err(AppError::evidence(
                "coop C0 expected/before target count differs",
            ));
        }
        for (expected_slot, before_slot) in expected.slots.iter_mut().zip(&before.slots) {
            if expected_slot.expected_bytes_proof.is_none() {
                if !matches!(
                    expected_slot.patch_kind.as_str(),
                    "IMPORT_POINTER" | "IMAGE_POINTER"
                ) || !before_slot.bytes_proof.starts_with("OPAQUE-")
                {
                    return Err(AppError::evidence(
                        "coop C0 pointer slot lacks an opaque byte-equality proof",
                    ));
                }
                expected_slot.expected_bytes_proof = Some(before_slot.bytes_proof.clone());
            }
        }
        Ok(())
    }

    fn observed_targets(
        process: &ObservedProcess,
        image: &PeImage,
        catalog: &CoopCatalog,
        tokenizer: &Tokenizer,
        phase: &str,
    ) -> AppResult<ObservedTargetDocument> {
        let mut slots = Vec::with_capacity(C0_SLOT_COUNT);
        for slot in expected_hook_slots() {
            slots.push(observe_active_slot(process, image, &slot, tokenizer)?);
        }
        for slot in &catalog.inherited_hook_transaction.new_slots {
            let rva = parse_rva(&slot.rva)?;
            let address = checked_add(process.main_module.base, u64::from(rva))?;
            let bytes = process.read_memory(address, slot.width)?;
            if hex::encode_upper(&bytes) != slot.expected_bytes {
                return Err(AppError::evidence(format!(
                    "coop C0 slot {} differs from frozen bytes",
                    slot.id
                )));
            }
            validate_rel32(address, &bytes, process.main_module.base, &slot.target_rva)?;
            slots.push(ObservedTargetSlot {
                ordinal: u32::try_from(slot.ordinal)
                    .map_err(|_| AppError::evidence("coop slot ordinal overflow"))?,
                id: slot.id.clone(),
                rva: slot.rva.clone(),
                bytes_proof: format!("BYTE-{}", hex::encode_upper(bytes)),
                semantic: format!("REL32_TARGET:{}", slot.target_rva),
            });
        }
        slots.sort_by_key(|slot| slot.ordinal);
        Ok(ObservedTargetDocument {
            schema_version: C0_SCHEMA_VERSION,
            document_identity: "IsaacCsT0-CoopC0ObservedTargets-v1".to_owned(),
            phase: phase.to_owned(),
            slot_count: slots.len(),
            slots,
            tick_path: observed_tick_path(process)?,
        })
    }

    fn observe_active_slot(
        process: &ObservedProcess,
        image: &PeImage,
        slot: &HookSlot,
        tokenizer: &Tokenizer,
    ) -> AppResult<ObservedTargetSlot> {
        let rva = parse_rva(&slot.rva)?;
        let address = checked_add(process.main_module.base, u64::from(rva))?;
        let width = usize::try_from(slot.width)
            .map_err(|_| AppError::evidence("active slot width overflow"))?;
        let bytes = process.read_memory(address, width)?;
        let semantic = match slot.patch_kind.as_str() {
            "REL32_CALL" => {
                let target = slot
                    .expected_target_rva
                    .as_deref()
                    .ok_or_else(|| AppError::evidence("REL32 active slot lacks expected target"))?;
                validate_rel32(address, &bytes, process.main_module.base, target)?;
                format!("REL32_TARGET:{target}")
            }
            "IMAGE_POINTER" => {
                let target = slot.expected_target_rva.as_deref().ok_or_else(|| {
                    AppError::evidence("image pointer active slot lacks expected target")
                })?;
                if bytes.len() != 4 {
                    return Err(AppError::evidence("image pointer width is not four"));
                }
                let actual = u64::from(u32::from_le_bytes(bytes[..4].try_into().unwrap()));
                if actual != checked_add(process.main_module.base, u64::from(parse_rva(target)?))?
                    || process.owner_module(actual) != Some(&process.main_module)
                {
                    return Err(AppError::evidence("active image pointer target drift"));
                }
                format!("IMAGE_TARGET:{target}")
            }
            "IMPORT_POINTER" => {
                if bytes.len() != 4 {
                    return Err(AppError::evidence("import pointer width is not four"));
                }
                let actual = u64::from(u32::from_le_bytes(bytes[..4].try_into().unwrap()));
                let owner = process
                    .owner_module(actual)
                    .ok_or_else(|| AppError::evidence("import pointer has no module owner"))?;
                let symbol = slot
                    .expected_symbol
                    .as_deref()
                    .ok_or_else(|| AppError::evidence("import pointer lacks expected symbol"))?;
                let (_, bare_symbol) = symbol
                    .split_once('!')
                    .ok_or_else(|| AppError::evidence("malformed import symbol"))?;
                if image.imports.get(bare_symbol) != Some(&rva)
                    || !owner.name.eq_ignore_ascii_case("USER32.dll")
                {
                    return Err(AppError::evidence("active import pointer semantic drift"));
                }
                format!("IMPORT_SLOT:{symbol}:LIVE_OWNER:USER32.dll:SYMBOL_ADDRESS_UNRESOLVED")
            }
            _ => return Err(AppError::evidence("unknown active slot patch kind")),
        };
        Ok(ObservedTargetSlot {
            ordinal: slot.ordinal,
            id: slot.id.clone(),
            rva: slot.rva.clone(),
            bytes_proof: if matches!(slot.patch_kind.as_str(), "IMPORT_POINTER" | "IMAGE_POINTER") {
                tokenizer.bytes_proof(b"target-slot", slot.ordinal, &bytes)
            } else {
                format!("BYTE-{}", hex::encode_upper(bytes))
            },
            semantic,
        })
    }

    fn expected_tick_path(image: &PeImage) -> AppResult<Vec<CodeObservation>> {
        tick_path_specs()
            .into_iter()
            .map(|(id, rva, width, semantic)| {
                let bytes = image
                    .bytes_at_rva(rva, width as usize)
                    .map_err(|error| AppError::evidence(error.to_string()))?;
                if id == "steady-standard-frame-store"
                    && bytes != hex::decode("89B7F8640200").unwrap()
                {
                    return Err(AppError::evidence(
                        "steady-standard frame store bytes drifted",
                    ));
                }
                if id != "steady-standard-frame-store" {
                    let target = if id == "steady-standard-room-update" {
                        "0x00402980"
                    } else {
                        "0x004607A0"
                    };
                    validate_disk_rel32(rva, bytes, target)?;
                }
                Ok(CodeObservation {
                    id: id.to_owned(),
                    rva: format!("0x{rva:08X}"),
                    width,
                    bytes_hex: hex::encode_upper(bytes),
                    semantic: semantic.to_owned(),
                })
            })
            .collect()
    }

    fn observed_tick_path(process: &ObservedProcess) -> AppResult<Vec<CodeObservation>> {
        tick_path_specs()
            .into_iter()
            .map(|(id, rva, width, semantic)| {
                let address = checked_add(process.main_module.base, u64::from(rva))?;
                let bytes = process.read_memory(address, width as usize)?;
                if id == "steady-standard-frame-store"
                    && bytes != hex::decode("89B7F8640200").unwrap()
                {
                    return Err(AppError::evidence(
                        "live steady-standard frame store drifted",
                    ));
                }
                if id != "steady-standard-frame-store" {
                    let target = if id == "steady-standard-room-update" {
                        "0x00402980"
                    } else {
                        "0x004607A0"
                    };
                    validate_rel32(address, &bytes, process.main_module.base, target)?;
                }
                Ok(CodeObservation {
                    id: id.to_owned(),
                    rva: format!("0x{rva:08X}"),
                    width,
                    bytes_hex: hex::encode_upper(bytes),
                    semantic: semantic.to_owned(),
                })
            })
            .collect()
    }

    fn tick_path_specs() -> [(&'static str, u32, u32, &'static str); 3] {
        [
            (
                "steady-standard-frame-store",
                0x002F_BA0B,
                6,
                "GAME_ROOT_FRAME_PRE_ATTEMPT_STORE",
            ),
            (
                "steady-standard-room-update",
                0x002F_BA4D,
                5,
                "REL32_TARGET:0x00402980",
            ),
            (
                "steady-standard-post-tail",
                0x002F_BBA5,
                5,
                "REL32_TARGET:0x004607A0",
            ),
        ]
    }

    fn validate_disk_rel32(rva: u32, bytes: &[u8], expected_target: &str) -> AppResult<()> {
        if bytes.len() != 5 || bytes[0] != 0xE8 {
            return Err(AppError::evidence("disk tick path is not E8 rel32"));
        }
        let displacement = i32::from_le_bytes(bytes[1..5].try_into().unwrap());
        let target = i128::from(rva) + 5 + i128::from(displacement);
        if target != i128::from(parse_rva(expected_target)?) {
            return Err(AppError::evidence("disk tick path target drifted"));
        }
        Ok(())
    }

    fn validate_rel32(
        address: u64,
        bytes: &[u8],
        module_base: u64,
        expected_target: &str,
    ) -> AppResult<()> {
        if bytes.len() != 5 || bytes[0] != 0xE8 {
            return Err(AppError::evidence("live target is not E8 rel32"));
        }
        let displacement = i32::from_le_bytes(bytes[1..5].try_into().unwrap());
        let actual = i128::from(address) + 5 + i128::from(displacement);
        let expected = i128::from(checked_add(
            module_base,
            u64::from(parse_rva(expected_target)?),
        )?);
        if actual != expected {
            return Err(AppError::evidence("live rel32 target drifted"));
        }
        Ok(())
    }

    fn module_digest(process: &ObservedProcess) -> AppResult<String> {
        let mut modules = process
            .modules
            .iter()
            .map(|module| (module.name.to_ascii_lowercase(), module.size))
            .collect::<Vec<_>>();
        modules.sort();
        let bytes =
            serde_json::to_vec(&modules).usage_context("cannot encode coop C0 module identity")?;
        Ok(sha256_bytes(&bytes))
    }

    fn require_clean(process: &ObservedProcess) -> AppResult<()> {
        if process.wait(0)?
            || process.debugger_present()?
            || !process.forbidden_modules().is_empty()
            || !win32::running_blockers()?.is_empty()
        {
            return Err(AppError::evidence(
                "coop C0 target is not alive and clean-disarmed",
            ));
        }
        Ok(())
    }

    fn read_vector(
        process: &ObservedProcess,
        address: u64,
        element_size: usize,
        maximum_count: usize,
    ) -> Result<VectorHeader, AttemptFailure> {
        let bytes = process
            .read_memory(address, 12)
            .map_err(|_| AttemptFailure::Read("READ_VECTOR_HEADER"))?;
        let header = VectorHeader {
            begin: u32::from_le_bytes(bytes[0..4].try_into().unwrap()),
            end: u32::from_le_bytes(bytes[4..8].try_into().unwrap()),
            capacity_end: u32::from_le_bytes(bytes[8..12].try_into().unwrap()),
        };
        vector_count(header, element_size, maximum_count)?;
        Ok(header)
    }

    fn vector_count(
        header: VectorHeader,
        element_size: usize,
        maximum_count: usize,
    ) -> Result<usize, AttemptFailure> {
        if header.begin == 0 && header.end == 0 && header.capacity_end == 0 {
            return Ok(0);
        }
        let alignment = u32::try_from(element_size.min(4)).unwrap();
        if header.begin == 0
            || header.begin > header.end
            || header.end > header.capacity_end
            || !header.begin.is_multiple_of(alignment)
            || !header.end.is_multiple_of(alignment)
            || !header.capacity_end.is_multiple_of(alignment)
        {
            return Err(AttemptFailure::Unstable("INVALID_VECTOR_HEADER"));
        }
        let bytes = usize::try_from(header.end - header.begin)
            .map_err(|_| AttemptFailure::Unstable("VECTOR_SPAN_OVERFLOW"))?;
        if !bytes.is_multiple_of(element_size) || bytes / element_size > maximum_count {
            return Err(AttemptFailure::Unstable("INVALID_VECTOR_COUNT"));
        }
        Ok(bytes / element_size)
    }

    fn read_span(
        process: &ObservedProcess,
        header: VectorHeader,
        element_size: usize,
    ) -> Result<Vec<u8>, AttemptFailure> {
        let count = vector_count(header, element_size, usize::MAX)?;
        if count == 0 {
            Ok(Vec::new())
        } else {
            process
                .read_memory(u64::from(header.begin), count * element_size)
                .map_err(|_| AttemptFailure::Read("READ_VECTOR_SPAN"))
        }
    }

    fn read_u32(process: &ObservedProcess, address: u64) -> Result<u32, AttemptFailure> {
        let bytes = process
            .read_memory(address, 4)
            .map_err(|_| AttemptFailure::Read("READ_U32"))?;
        Ok(u32::from_le_bytes(bytes.try_into().unwrap()))
    }

    fn read_i32(process: &ObservedProcess, address: u64) -> Result<i32, AttemptFailure> {
        Ok(i32::from_le_bytes(
            process
                .read_memory(address, 4)
                .map_err(|_| AttemptFailure::Read("READ_I32"))?
                .try_into()
                .unwrap(),
        ))
    }

    fn read_u64(process: &ObservedProcess, address: u64) -> Result<u64, AttemptFailure> {
        Ok(u64::from_le_bytes(
            process
                .read_memory(address, 8)
                .map_err(|_| AttemptFailure::Read("READ_U64"))?
                .try_into()
                .unwrap(),
        ))
    }

    fn add(base: u64, offset: u64) -> Result<u64, AttemptFailure> {
        base.checked_add(offset)
            .ok_or(AttemptFailure::Read("ADDRESS_OVERFLOW"))
    }

    fn checked_add(base: u64, offset: u64) -> AppResult<u64> {
        base.checked_add(offset)
            .ok_or_else(|| AppError::evidence("coop C0 address overflow"))
    }

    fn parse_rva(value: &str) -> AppResult<u32> {
        value
            .strip_prefix("0x")
            .or_else(|| value.strip_prefix("0X"))
            .and_then(|raw| u32::from_str_radix(raw, 16).ok())
            .ok_or_else(|| AppError::evidence(format!("invalid coop C0 RVA {value}")))
    }
}

#[cfg(windows)]
pub fn capture(request: &CaptureRequest<'_>) -> AppResult<C0CaptureSummary> {
    platform::capture(request)
}

#[cfg(not(windows))]
pub fn capture(_request: &CaptureRequest<'_>) -> AppResult<C0CaptureSummary> {
    Err(AppError::usage("coop C0 capture requires Windows"))
}

#[cfg(test)]
mod tests {
    use super::*;

    fn player(ordinal: u32, kind: &str) -> C0PlayerObservation {
        C0PlayerObservation {
            ordinal,
            object_token: format!("O-PLAYER-{ordinal}"),
            device_kind: Some(kind.to_owned()),
            unique_assignment: true,
            ordinal_field_matches: true,
            hud_primary_slot_matches: true,
            lua_order_evidence: LUA_STATIC_ORDER.to_owned(),
            init_seed_stable: true,
            init_seed_nonzero: true,
            init_seed_distinct: true,
            twin_relation_absent: true,
        }
    }

    fn local_graph() -> C0GraphObservation {
        C0GraphObservation {
            topology_sha256: "A".repeat(64),
            run_active: true,
            online_mode: false,
            manager_present: true,
            network_backend_present: false,
            net_input_present: false,
            player_count: 2,
            device_count: 2,
            roster_count: 0,
            players: vec![player(0, "KEYBOARD"), player(1, "GAMEPAD")],
            unassigned_device_kinds: Vec::new(),
            primary_hud_consistent: true,
            twin_hud_empty: true,
        }
    }

    #[test]
    fn fixed_cli_scope_rejects_active_capabilities() {
        assert!(
            validate_capture_options(
                C0_PROBE,
                SCENARIO_LOCAL_DUAL,
                "case-1",
                DEFAULT_DURATION_MILLISECONDS,
                DEFAULT_INTERVAL_MILLISECONDS
            )
            .is_ok()
        );
        assert!(
            validate_capture_options(
                "loopback",
                SCENARIO_LOCAL_DUAL,
                "case-1",
                DEFAULT_DURATION_MILLISECONDS,
                DEFAULT_INTERVAL_MILLISECONDS
            )
            .is_err()
        );
        assert!(validate_capture_options(C0_PROBE, "loopback", "case-1", 5_000, 50).is_err());
        assert!(
            validate_capture_options(C0_PROBE, SCENARIO_LOCAL_DUAL, "../case", 5_000, 50).is_err()
        );
    }

    #[test]
    fn local_dual_requires_keyboard_gamepad_bijection() {
        let valid = local_graph();
        assert!(scenario_graph_pass(SCENARIO_LOCAL_DUAL, &valid));
        let mut duplicate = valid.clone();
        duplicate.players[1].device_kind = Some("KEYBOARD".to_owned());
        assert!(!scenario_graph_pass(SCENARIO_LOCAL_DUAL, &duplicate));
        let mut ambiguous = valid;
        ambiguous.players[1].unique_assignment = false;
        assert!(!scenario_graph_pass(SCENARIO_LOCAL_DUAL, &ambiguous));
    }

    #[test]
    fn net_input_classification_is_original_online_membership_only_and_never_assignable() {
        let net_input =
            classify_c0_device(SCENARIO_ORIGINAL_ONLINE, true, None).expect("NetInput member");
        assert_eq!(net_input.kind, "NET_INPUT");
        assert!(!net_input.read_assignment_key);

        assert!(classify_c0_device(SCENARIO_ORIGINAL_ONLINE, false, None).is_none());
        assert!(classify_c0_device(SCENARIO_LOCAL_DUAL, true, None).is_none());
        assert!(classify_c0_device(SCENARIO_OFFLINE_SINGLE, true, None).is_none());

        for scenario in [SCENARIO_OFFLINE_SINGLE, SCENARIO_LOCAL_DUAL] {
            let keyboard =
                classify_c0_device(scenario, false, Some("KEYBOARD")).expect("known keyboard");
            assert_eq!(keyboard.kind, "KEYBOARD");
            assert!(keyboard.read_assignment_key);
        }
        let gamepad = classify_c0_device(SCENARIO_ORIGINAL_ONLINE, false, Some("GAMEPAD"))
            .expect("known gamepad");
        assert_eq!(gamepad.kind, "GAMEPAD");
        assert!(gamepad.read_assignment_key);
    }

    #[test]
    fn peer_identity_summary_requires_exact_nonzero_unique_cardinality() {
        assert!(peer_identity_summary_valid(&[101, 202], 2));
        assert!(peer_identity_summary_valid(&[], 0));
        assert!(!peer_identity_summary_valid(&[0, 202], 2));
        assert!(!peer_identity_summary_valid(&[101, 101], 2));
        assert!(!peer_identity_summary_valid(&[101], 2));
        assert!(!peer_identity_summary_valid(&[101, 202, 303], 2));

        let forward = [101, 202];
        let reverse = [101, 202];
        assert!(peer_identity_summary_valid(&forward, 2));
        assert!(peer_identity_summary_valid(&reverse, 2));
        assert_eq!(
            forward, reverse,
            "double-read identities must remain stable"
        );
    }

    #[test]
    fn dropped_sample_cannot_carry_graph_payload() {
        let dropped = C0Sample {
            schema_version: C0_SCHEMA_VERSION,
            observer_sequence: 1,
            qpc: 1,
            status: STATUS_UNSTABLE.to_owned(),
            retry_count: MAX_RETRIES,
            reason: Some("STRUCTURAL_DOUBLE_READ_DRIFT".to_owned()),
            game_frame_delta: None,
            online_frame_delta: None,
            graph: None,
        };
        assert!(validate_sample_shape(&dropped));
        let mut leaked = dropped;
        leaked.graph = Some(local_graph());
        assert!(!validate_sample_shape(&leaked));
    }

    #[test]
    fn privacy_scan_rejects_runtime_identity_and_local_paths() {
        assert!(
            privacy_scan(&serde_json::json!({
                "rva": "0x002FBA4D",
                "observerAccessBits": "0x00101410",
                "objectToken": "O-ABC"
            }))
            .is_ok()
        );
        assert!(privacy_scan(&serde_json::json!({"pid": 7})).is_err());
        assert!(privacy_scan(&serde_json::json!({"value": "0x12345678"})).is_err());
        assert!(privacy_scan(&serde_json::json!({"value": "C:\\Users\\name"})).is_err());
    }

    #[test]
    fn frozen_plan_grants_only_the_bound_c0_observer() {
        let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
        let repository = manifest_dir.parent().unwrap();
        let result = load_and_validate_plan(
            &manifest_dir.join("coop-plan-v3.json"),
            &repository.join(
                "out/cs-t0/target-equivalence-inspect/unpacked-target-equivalence-v1-release-final.json",
            ),
            &manifest_dir.join("coop-c0-layout-evidence-v1.json"),
            &manifest_dir.join("coop-catalog-v2.json"),
            &manifest_dir.join("coop-static-graph-v1.json"),
        )
        .unwrap();
        assert!(result.c0_authorization.process_read_allowed);
        assert!(!result.c0_authorization.runtime_mutation_allowed);
        assert!(!result.permissions.cs_t00503_unlocked);
    }
}
