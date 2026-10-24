use std::collections::BTreeSet;
use std::fs::{self, OpenOptions};
use std::io::Write;
use std::path::{Component, Path, PathBuf};

use serde::{Deserialize, Serialize};

use crate::artifact::file_sha256;
use crate::error::{AppError, AppResult, UsageContext};
use crate::mode_artifact::ModeRunManifest;
use crate::mode_gate::{GateDecision, T0_02_GATE_REVISION, T0_02_GATE_SCHEMA_VERSION};
use crate::pe::sha256_hex;
use crate::raw_apply_artifact::{
    EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME, RAW_APPLY_ARTIFACT_SCHEMA_VERSION,
    RAW_APPLY_CAPTURE_PLAN_SHA256, RAW_APPLY_CATALOG_SHA256, read_raw_apply_run,
    replay_raw_apply_run_for_revision,
};

pub const ACTIVE_CATALOG_SCHEMA_VERSION: u32 = 2;
pub const ACTIVE_PLAN_SCHEMA_VERSION: u32 = 2;
pub const RAW_PREREQ_MATRIX_SCHEMA_VERSION: u32 = 1;
pub const RAW_PREREQ_SCHEMA_VERSION: u32 = 1;
pub const ACTIVE_CATALOG_IDENTITY: &str = "IsaacCsT0-ActiveLoopbackCatalog-v2";
pub const ACTIVE_CATALOG_SUPERSEDES: &str = "IsaacCsT0-ActiveLoopbackCatalog-v1";
pub const ACTIVE_CATALOG_SUPERSEDES_SHA256: &str =
    "AAB3A81D70BB5F8495C88811E37D6005D5CF9B313D76CDC96ED7F96F2DF0AC83";
pub const ACTIVE_CATALOG_SHA256: &str =
    "A89A74A251EEAA2510FC0C681247ABD6B0A31A7CC3ED60833C78EF36C9B0DC77";
pub const ACTIVE_PLAN_IDENTITY: &str = "IsaacCsT0-ActiveLoopbackPlan-v2";
pub const ACTIVE_PLAN_SUPERSEDES: &str = "IsaacCsT0-ActiveLoopbackPlan-v1";
pub const ACTIVE_PLAN_SUPERSEDES_SHA256: &str =
    "2F3F45CE2D27A67BCA5F0345825CF6371759738974D64EB5C75FBBBF7B16AA25";
pub const ACTIVE_PLAN_SHA256: &str =
    "FE4FC7ABDE09CBB280DA342BD384AB50107CA948A052AC02BA6ADE37BB1DF3BA";
pub const ACTIVE_INPUT_SCRIPT_SHA256: &str =
    "7F2DAC1FF124B7DB2BC71FE886B6EE3AE49ADE23D5C464AE14973F05A9E53E33";
pub const RAW_PREREQ_MATRIX_IDENTITY: &str = "IsaacCsT0-RawPrerequisiteMatrix-v1";
pub const RAW_PREREQ_IDENTITY: &str = "IsaacCsT0-RawPrerequisite-v1";
const RAW_APPLY_V3_CATALOG_SHA256: &str =
    "258549E9B107316FDE02C44580741862476AAB7CCABE57AA1958F06729304F51";
const RAW_APPLY_V3_CAPTURE_PLAN_SHA256: &str =
    "8908DABFF44E299185438CBC6092F7A3A6B5E308FB95C75BDFA795F0E0042C93";
pub const FIXED_TARGET_SHA256: &str =
    "C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30";
pub const ACTIVE_SLOT_COUNT: usize = 45;
pub const ACTIVE_OWNED_SLOT_COUNT: usize = 13;
pub const V5_SLOT_COUNT: usize = 32;

pub const ACTIVE_SCENARIOS: [&str; 18] = [
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

const RAW_PREREQ_SCENARIOS: [&str; 4] = [
    "keyboard-held-release",
    "keyboard-edge",
    "controller-held-release",
    "controller-edge",
];

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct HookSlot {
    pub ordinal: u32,
    pub id: String,
    pub owner: String,
    pub patch_kind: String,
    pub rva: String,
    pub width: u32,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub expected_target_rva: Option<String>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub expected_symbol: Option<String>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct PassiveSourceBinding {
    pub scenario: String,
    pub run_path: PathBuf,
    pub artifact_identity: String,
    pub manifest_sha256: String,
    pub events_sha256: String,
    pub analysis_sha256: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ActiveCatalog {
    pub schema_version: u32,
    pub catalog_identity: String,
    pub supersedes: String,
    pub supersedes_sha256: String,
    pub target_sha256: String,
    pub mode_gate_sha256: String,
    pub loop_gate_sha256: String,
    pub passive_catalog_sha256: String,
    pub passive_capture_plan_sha256: String,
    pub authority_call_rva: String,
    pub authority_target_rva: String,
    pub authority_path: String,
    pub v5_slot_count: usize,
    pub active_owned_slot_count: usize,
    pub total_slot_count: usize,
    pub hook_slots: Vec<HookSlot>,
    pub passive_sources: Vec<PassiveSourceBinding>,
    pub production_active_input_allowed: bool,
    pub experiment_requires_one_time_arm: bool,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ActivePlan {
    pub schema_version: u32,
    pub plan_identity: String,
    pub supersedes: String,
    pub supersedes_sha256: String,
    pub catalog_path: PathBuf,
    pub catalog_sha256: String,
    pub target_sha256: String,
    pub mode_gate_sha256: String,
    pub loop_gate_sha256: String,
    pub raw_prereq_matrix_sha256: String,
    pub authority_call_rva: String,
    pub supported_authority_path: String,
    pub fail_closed_paths: Vec<String>,
    pub required_scenarios: Vec<String>,
    pub repeatability_scenario: String,
    pub repeatability_source_count: usize,
    pub repeatability_input_script_sha256: String,
    pub total_slot_count: usize,
    pub production_active_input_allowed: bool,
    pub experiment_requires_one_time_arm: bool,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct RawPrereqMatrix {
    pub schema_version: u32,
    pub matrix_identity: String,
    pub target_sha256: String,
    pub loop_gate_sha256: String,
    pub passive_catalog_sha256: String,
    pub passive_capture_plan_sha256: String,
    pub sources: Vec<PassiveSourceBinding>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct RawPrereqSourceDecision {
    pub scenario: String,
    pub artifact_identity: String,
    pub run_path: PathBuf,
    pub manifest_sha256: String,
    pub events_sha256: String,
    pub analysis_sha256: String,
    pub source_passive_catalog_sha256: String,
    pub source_passive_capture_plan_sha256: String,
    pub replay_result: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct RawPrereqDecision {
    pub schema_version: u32,
    pub prerequisite_identity: String,
    pub status: String,
    pub active_prerequisite_satisfied: bool,
    pub eligible_for_active_gate: bool,
    pub active_input_allowed: bool,
    pub target_sha256: String,
    pub loop_gate_sha256: String,
    pub passive_catalog_sha256: String,
    pub passive_capture_plan_sha256: String,
    pub matrix_sha256: String,
    pub sources: Vec<RawPrereqSourceDecision>,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct ValidatedActivePlan {
    pub target_sha256: String,
    pub mode_gate_sha256: String,
    pub loop_gate_sha256: String,
    pub loop_ledger_sha256: String,
    pub raw_prereq_sha256: String,
    pub catalog_sha256: String,
    pub plan_sha256: String,
    pub repeatability_input_script_sha256: String,
}

pub fn write_raw_prerequisite(matrix_path: &Path, output: &Path) -> AppResult<RawPrereqDecision> {
    if output.exists() {
        return Err(AppError::usage(format!(
            "refusing to overwrite {}",
            output.display()
        )));
    }
    let matrix_bytes = fs::read(matrix_path).usage_context(format!(
        "cannot read raw prerequisite matrix {}",
        matrix_path.display()
    ))?;
    let matrix: RawPrereqMatrix = serde_json::from_slice(&matrix_bytes).usage_context(format!(
        "cannot parse raw prerequisite matrix {}",
        matrix_path.display()
    ))?;
    validate_raw_prereq_matrix(&matrix)?;
    let base = containing_directory(matrix_path);
    let mut seen = BTreeSet::new();
    let mut seen_artifacts = BTreeSet::new();
    let mut seen_runs = BTreeSet::new();
    let mut sources = Vec::with_capacity(matrix.sources.len());
    for binding in &matrix.sources {
        if !seen.insert(binding.scenario.as_str()) {
            return Err(AppError::evidence(
                "raw prerequisite matrix duplicates a scenario",
            ));
        }
        if !seen_artifacts.insert(binding.artifact_identity.as_str()) {
            return Err(AppError::evidence(
                "raw prerequisite matrix duplicates an artifact identity",
            ));
        }
        if !seen_runs.insert(binding.run_path.clone()) {
            return Err(AppError::evidence(
                "raw prerequisite matrix duplicates a source run path",
            ));
        }
        let run = resolve_bounded_path(base, &binding.run_path)?;
        let manifest_path = run.join("manifest.json");
        let events_path = run.join("events.jsonl");
        let analysis_path = run.join("analysis.json");
        let manifest_hash = file_sha256(&manifest_path)?;
        let events_hash = file_sha256(&events_path)?;
        let analysis_hash = file_sha256(&analysis_path)?;
        if manifest_hash != binding.manifest_sha256
            || events_hash != binding.events_sha256
            || analysis_hash != binding.analysis_sha256
        {
            return Err(AppError::evidence(format!(
                "raw prerequisite source {} hash mismatch",
                binding.scenario
            )));
        }
        let (manifest, _) = read_raw_apply_run(&run)?;
        let (source_catalog_sha256, source_capture_plan_sha256) =
            expected_raw_prereq_source_revision(&binding.scenario)?;
        let replay = replay_raw_apply_run_for_revision(
            &run,
            source_catalog_sha256,
            source_capture_plan_sha256,
        )?;
        let mut invalid = Vec::new();
        if manifest.schema_version != RAW_APPLY_ARTIFACT_SCHEMA_VERSION {
            invalid.push("schema");
        }
        if manifest.evidence_class != EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME {
            invalid.push("evidence-class");
        }
        if manifest.scenario != binding.scenario {
            invalid.push("scenario");
        }
        if manifest.artifact_identity != binding.artifact_identity {
            invalid.push("artifact-identity");
        }
        if manifest.target_sha256 != matrix.target_sha256 {
            invalid.push("target");
        }
        if manifest.loop_gate_sha256 != matrix.loop_gate_sha256 {
            invalid.push("loop-gate");
        }
        if manifest.passive_catalog_sha256 != source_catalog_sha256 {
            invalid.push("source-catalog");
        }
        if manifest.passive_capture_plan_sha256 != source_capture_plan_sha256 {
            invalid.push("source-capture-plan");
        }
        if manifest.eligible_for_active_gate {
            invalid.push("manifest-active-eligibility");
        }
        if replay.result != "PASS" {
            invalid.push("replay-result");
        }
        if replay.eligible_for_active_gate {
            invalid.push("replay-active-eligibility");
        }
        let failed_checks = replay
            .checks
            .iter()
            .filter(|check| !check.passed)
            .map(|check| check.id.as_str())
            .collect::<Vec<_>>();
        if !failed_checks.is_empty() {
            invalid.push("replay-checks");
        }
        if !invalid.is_empty() {
            return Err(AppError::evidence(format!(
                "raw prerequisite source {} is not an exact passive PASS replay: {}; failed checks: {}",
                binding.scenario,
                invalid.join(","),
                failed_checks.join(",")
            )));
        }
        let stored: crate::raw_apply_artifact::RawApplyAnalysisReport =
            serde_json::from_slice(&fs::read(&analysis_path).usage_context(format!(
                "cannot read stored raw analysis {}",
                analysis_path.display()
            ))?)
            .usage_context(format!(
                "cannot parse stored raw analysis {}",
                analysis_path.display()
            ))?;
        if stored != replay {
            return Err(AppError::evidence(format!(
                "raw prerequisite source {} stored analysis differs from replay",
                binding.scenario
            )));
        }
        sources.push(RawPrereqSourceDecision {
            scenario: binding.scenario.clone(),
            artifact_identity: binding.artifact_identity.clone(),
            run_path: binding.run_path.clone(),
            manifest_sha256: manifest_hash,
            events_sha256: events_hash,
            analysis_sha256: analysis_hash,
            source_passive_catalog_sha256: manifest.passive_catalog_sha256,
            source_passive_capture_plan_sha256: manifest.passive_capture_plan_sha256,
            replay_result: replay.result,
        });
    }
    let expected = RAW_PREREQ_SCENARIOS.into_iter().collect::<BTreeSet<_>>();
    if seen != expected {
        return Err(AppError::evidence(
            "raw prerequisite matrix does not bind the exact four passive scenarios",
        ));
    }
    let decision = RawPrereqDecision {
        schema_version: RAW_PREREQ_SCHEMA_VERSION,
        prerequisite_identity: RAW_PREREQ_IDENTITY.to_owned(),
        status: "PASS".to_owned(),
        active_prerequisite_satisfied: true,
        eligible_for_active_gate: false,
        active_input_allowed: false,
        target_sha256: matrix.target_sha256,
        loop_gate_sha256: matrix.loop_gate_sha256,
        passive_catalog_sha256: matrix.passive_catalog_sha256,
        passive_capture_plan_sha256: matrix.passive_capture_plan_sha256,
        matrix_sha256: sha256_hex(&matrix_bytes),
        sources,
    };
    write_new_json(output, &decision)?;
    Ok(decision)
}

pub fn validate_raw_prerequisite(path: &Path) -> AppResult<(RawPrereqDecision, String)> {
    let bytes =
        fs::read(path).usage_context(format!("cannot read raw prerequisite {}", path.display()))?;
    let decision: RawPrereqDecision = serde_json::from_slice(&bytes)
        .usage_context(format!("cannot parse raw prerequisite {}", path.display()))?;
    if decision.schema_version != RAW_PREREQ_SCHEMA_VERSION
        || decision.prerequisite_identity != RAW_PREREQ_IDENTITY
        || decision.status != "PASS"
        || !decision.active_prerequisite_satisfied
        || decision.eligible_for_active_gate
        || decision.active_input_allowed
        || decision.target_sha256 != FIXED_TARGET_SHA256
        || !is_sha256(&decision.loop_gate_sha256)
        || decision.passive_catalog_sha256 != RAW_APPLY_CATALOG_SHA256
        || decision.passive_capture_plan_sha256 != RAW_APPLY_CAPTURE_PLAN_SHA256
        || !is_sha256(&decision.matrix_sha256)
        || decision.sources.len() != RAW_PREREQ_SCENARIOS.len()
        || decision.sources.iter().any(|source| {
            let expected_revision = expected_raw_prereq_source_revision(&source.scenario).ok();
            source.replay_result != "PASS"
                || source.artifact_identity.is_empty()
                || !is_sha256(&source.manifest_sha256)
                || !is_sha256(&source.events_sha256)
                || !is_sha256(&source.analysis_sha256)
                || expected_revision.as_ref().is_none_or(|(catalog, plan)| {
                    source.source_passive_catalog_sha256 != *catalog
                        || source.source_passive_capture_plan_sha256 != *plan
                })
        })
    {
        return Err(AppError::evidence(
            "raw prerequisite is not the exact passive-only PASS identity",
        ));
    }
    let scenarios = decision
        .sources
        .iter()
        .map(|source| source.scenario.as_str())
        .collect::<BTreeSet<_>>();
    let artifact_identities = decision
        .sources
        .iter()
        .map(|source| source.artifact_identity.as_str())
        .collect::<BTreeSet<_>>();
    let run_paths = decision
        .sources
        .iter()
        .map(|source| source.run_path.as_path())
        .collect::<BTreeSet<_>>();
    if scenarios != RAW_PREREQ_SCENARIOS.into_iter().collect()
        || artifact_identities.len() != decision.sources.len()
        || run_paths.len() != decision.sources.len()
        || decision
            .sources
            .iter()
            .any(|source| !is_bounded_relative_path(&source.run_path))
    {
        return Err(AppError::evidence(
            "raw prerequisite does not contain four unique bounded passive sources",
        ));
    }
    Ok((decision, sha256_hex(&bytes)))
}

pub fn validate_active_plan(
    plan_path: &Path,
    mode_gate_path: &Path,
    loop_gate_path: &Path,
    raw_prereq_path: &Path,
) -> AppResult<ValidatedActivePlan> {
    let plan_bytes = fs::read(plan_path)
        .usage_context(format!("cannot read active plan {}", plan_path.display()))?;
    let plan: ActivePlan = serde_json::from_slice(&plan_bytes)
        .usage_context(format!("cannot parse active plan {}", plan_path.display()))?;
    let plan_sha256 = sha256_hex(&plan_bytes);
    if plan_sha256 != ACTIVE_PLAN_SHA256 {
        return Err(AppError::evidence(
            "active plan file does not match the frozen v2 SHA-256",
        ));
    }
    let plan_base = containing_directory(plan_path);
    let catalog_path = resolve_bounded_path(plan_base, &plan.catalog_path)?;
    let catalog_bytes = fs::read(&catalog_path).usage_context(format!(
        "cannot read active catalog {}",
        catalog_path.display()
    ))?;
    let catalog: ActiveCatalog = serde_json::from_slice(&catalog_bytes).usage_context(format!(
        "cannot parse active catalog {}",
        catalog_path.display()
    ))?;
    validate_catalog(&catalog)?;
    validate_plan_shape(&plan)?;
    let mode = validate_mode_gate_reference(mode_gate_path)?;
    let loop_gate = crate::input_artifact::validate_loop_gate_reference(loop_gate_path)?;
    let (raw, raw_sha256) = validate_raw_prerequisite(raw_prereq_path)?;
    let catalog_sha256 = sha256_hex(&catalog_bytes);
    if plan.catalog_sha256 != catalog_sha256
        || plan.target_sha256 != catalog.target_sha256
        || plan.mode_gate_sha256 != catalog.mode_gate_sha256
        || plan.loop_gate_sha256 != catalog.loop_gate_sha256
        || plan.total_slot_count != catalog.total_slot_count
        || plan.target_sha256 != mode.1
        || plan.mode_gate_sha256 != mode.0
        || plan.target_sha256 != loop_gate.target_sha256
        || plan.loop_gate_sha256 != loop_gate.sha256
        || plan.target_sha256 != raw.target_sha256
        || plan.loop_gate_sha256 != raw.loop_gate_sha256
        || plan.raw_prereq_matrix_sha256 != raw.matrix_sha256
        || raw.passive_catalog_sha256 != catalog.passive_catalog_sha256
        || raw.passive_capture_plan_sha256 != catalog.passive_capture_plan_sha256
        || !same_passive_sources(&catalog.passive_sources, &raw.sources)
    {
        return Err(AppError::evidence(
            "active plan, catalog, gates, raw prerequisite, and target bindings disagree",
        ));
    }
    Ok(ValidatedActivePlan {
        target_sha256: plan.target_sha256,
        mode_gate_sha256: mode.0,
        loop_gate_sha256: loop_gate.sha256,
        loop_ledger_sha256: loop_gate.loop_ledger_sha256,
        raw_prereq_sha256: raw_sha256,
        catalog_sha256,
        plan_sha256,
        repeatability_input_script_sha256: plan.repeatability_input_script_sha256,
    })
}

pub fn validate_active_input_script(path: &Path, expected_sha256: &str) -> AppResult<String> {
    let bytes = fs::read(path).usage_context(format!(
        "cannot read active input script {}",
        path.display()
    ))?;
    let actual = sha256_hex(&bytes);
    if expected_sha256 != ACTIVE_INPUT_SCRIPT_SHA256 || actual != expected_sha256 {
        return Err(AppError::evidence(
            "active input script does not match the frozen repeatability identity",
        ));
    }
    Ok(actual)
}

pub fn validate_mode_gate_reference(path: &Path) -> AppResult<(String, String)> {
    let bytes =
        fs::read(path).usage_context(format!("cannot read T0-02 mode gate {}", path.display()))?;
    let gate: GateDecision = serde_json::from_slice(&bytes)
        .usage_context(format!("cannot parse T0-02 mode gate {}", path.display()))?;
    let expected_labels = [
        "t0-01-baseline",
        "offline-control",
        "offline-instrumented",
        "original-online-control-a",
        "original-online-control-b",
        "original-online-instrumented-a",
        "original-online-instrumented-b",
        "packet-comparison-v2",
    ]
    .into_iter()
    .collect::<BTreeSet<_>>();
    let labels = gate
        .sources
        .iter()
        .map(|source| source.label.as_str())
        .collect::<BTreeSet<_>>();
    if gate.schema_version != T0_02_GATE_SCHEMA_VERSION
        || gate.gate != "T0-02-mode-isolation"
        || gate.status != "PASS"
        || gate.analysis_revision != T0_02_GATE_REVISION
        || !gate.t0_03_unlocked
        || gate.checks.len() != 6
        || gate.checks.iter().any(|check| check.result != "PASS")
        || gate.sources.len() != expected_labels.len()
        || labels != expected_labels
    {
        return Err(AppError::evidence(
            "T0-02 mode gate does not have the exact complete PASS identity",
        ));
    }
    for source in &gate.sources {
        validate_leaf(&source.run_id, "mode gate run id")?;
        for name in source.files.keys() {
            validate_leaf(name, "mode gate file")?;
        }
    }
    let parent = containing_directory(path);
    let mut source_roots = [
        Some(parent.to_path_buf()),
        parent.parent().map(Path::to_path_buf),
    ]
    .into_iter()
    .flatten()
    .filter(|candidate| {
        gate.sources
            .iter()
            .all(|source| candidate.join(&source.run_id).is_dir())
    })
    .collect::<Vec<_>>();
    source_roots.sort();
    source_roots.dedup();
    if source_roots.len() != 1 {
        return Err(AppError::evidence(
            "T0-02 mode gate source root is missing or ambiguous",
        ));
    }
    let source_root = source_roots.remove(0);
    let mut target = None::<String>;
    for source in &gate.sources {
        validate_leaf(&source.run_id, "mode gate run id")?;
        if source.files.is_empty() {
            return Err(AppError::evidence("T0-02 mode gate source has no files"));
        }
        let run = source_root.join(&source.run_id);
        for (name, expected_hash) in &source.files {
            validate_leaf(name, "mode gate file")?;
            if file_sha256(&run.join(name))? != *expected_hash {
                return Err(AppError::evidence(format!(
                    "T0-02 mode gate source {} hash mismatch",
                    source.label
                )));
            }
        }
        if source.label != "t0-01-baseline" && source.label != "packet-comparison-v2" {
            let manifest: ModeRunManifest = serde_json::from_slice(
                &fs::read(run.join("manifest.json"))
                    .usage_context("cannot read bound T0-02 manifest")?,
            )
            .usage_context("cannot parse bound T0-02 manifest")?;
            if manifest.run_id != source.run_id
                || manifest.status != "PASS"
                || target.get_or_insert_with(|| manifest.game.file_sha256.clone())
                    != &manifest.game.file_sha256
            {
                return Err(AppError::evidence(
                    "T0-02 mode gate source identity or target differs",
                ));
            }
        }
    }
    let target =
        target.ok_or_else(|| AppError::evidence("T0-02 mode gate has no target binding"))?;
    if target != FIXED_TARGET_SHA256 {
        return Err(AppError::evidence(
            "T0-02 mode gate is not bound to the fixed target",
        ));
    }
    Ok((sha256_hex(&bytes), target))
}

fn validate_raw_prereq_matrix(matrix: &RawPrereqMatrix) -> AppResult<()> {
    if matrix.schema_version != RAW_PREREQ_MATRIX_SCHEMA_VERSION
        || matrix.matrix_identity != RAW_PREREQ_MATRIX_IDENTITY
        || matrix.target_sha256 != FIXED_TARGET_SHA256
        || !is_sha256(&matrix.loop_gate_sha256)
        || matrix.passive_catalog_sha256 != RAW_APPLY_CATALOG_SHA256
        || matrix.passive_capture_plan_sha256 != RAW_APPLY_CAPTURE_PLAN_SHA256
        || !passive_sources_are_unique(&matrix.sources)
    {
        return Err(AppError::evidence(
            "raw prerequisite matrix has an invalid frozen identity",
        ));
    }
    Ok(())
}

fn validate_catalog(catalog: &ActiveCatalog) -> AppResult<()> {
    if catalog.schema_version != ACTIVE_CATALOG_SCHEMA_VERSION
        || catalog.catalog_identity != ACTIVE_CATALOG_IDENTITY
        || catalog.supersedes != ACTIVE_CATALOG_SUPERSEDES
        || catalog.supersedes_sha256 != ACTIVE_CATALOG_SUPERSEDES_SHA256
        || catalog.target_sha256 != FIXED_TARGET_SHA256
        || !is_sha256(&catalog.mode_gate_sha256)
        || !is_sha256(&catalog.loop_gate_sha256)
        || catalog.passive_catalog_sha256 != RAW_APPLY_CATALOG_SHA256
        || catalog.passive_capture_plan_sha256 != RAW_APPLY_CAPTURE_PLAN_SHA256
        || catalog.authority_call_rva != "0x002FBA4D"
        || catalog.authority_target_rva != "0x00402980"
        || catalog.authority_path != "STEADY_STANDARD_ONLY"
        || catalog.v5_slot_count != V5_SLOT_COUNT
        || catalog.active_owned_slot_count != ACTIVE_OWNED_SLOT_COUNT
        || catalog.total_slot_count != ACTIVE_SLOT_COUNT
        || catalog.hook_slots != expected_hook_slots()
        || !passive_sources_are_unique(&catalog.passive_sources)
        || catalog.production_active_input_allowed
        || !catalog.experiment_requires_one_time_arm
    {
        return Err(AppError::evidence(
            "active catalog does not match the frozen 45-slot steady-standard identity",
        ));
    }
    Ok(())
}

fn validate_plan_shape(plan: &ActivePlan) -> AppResult<()> {
    let scenarios = plan
        .required_scenarios
        .iter()
        .map(String::as_str)
        .collect::<BTreeSet<_>>();
    if plan.schema_version != ACTIVE_PLAN_SCHEMA_VERSION
        || plan.plan_identity != ACTIVE_PLAN_IDENTITY
        || plan.supersedes != ACTIVE_PLAN_SUPERSEDES
        || plan.supersedes_sha256 != ACTIVE_PLAN_SUPERSEDES_SHA256
        || plan.catalog_path != Path::new("input-active-catalog-v2.json")
        || plan.catalog_sha256 != ACTIVE_CATALOG_SHA256
        || plan.target_sha256 != FIXED_TARGET_SHA256
        || plan.authority_call_rva != "0x002FBA4D"
        || plan.supported_authority_path != "STEADY_STANDARD_ONLY"
        || plan.fail_closed_paths
            != [
                "ALTERNATE",
                "LOADING",
                "ROOM_LIFECYCLE",
                "RUN_TEARDOWN",
                "MENU",
            ]
        || plan.required_scenarios.len() != ACTIVE_SCENARIOS.len()
        || scenarios != ACTIVE_SCENARIOS.into_iter().collect()
        || plan.repeatability_scenario != "loopback"
        || plan.repeatability_source_count != 2
        || !is_sha256(&plan.catalog_sha256)
        || !is_sha256(&plan.mode_gate_sha256)
        || !is_sha256(&plan.loop_gate_sha256)
        || !is_sha256(&plan.raw_prereq_matrix_sha256)
        || plan.repeatability_input_script_sha256 != ACTIVE_INPUT_SCRIPT_SHA256
        || plan.total_slot_count != ACTIVE_SLOT_COUNT
        || plan.production_active_input_allowed
        || !plan.experiment_requires_one_time_arm
    {
        return Err(AppError::evidence(
            "active plan does not match the frozen experimental scope",
        ));
    }
    Ok(())
}

fn same_passive_sources(
    catalog: &[PassiveSourceBinding],
    prereq: &[RawPrereqSourceDecision],
) -> bool {
    catalog.len() == prereq.len()
        && catalog.iter().all(|left| {
            prereq.iter().any(|right| {
                left.scenario == right.scenario
                    && left.run_path == right.run_path
                    && left.artifact_identity == right.artifact_identity
                    && left.manifest_sha256 == right.manifest_sha256
                    && left.events_sha256 == right.events_sha256
                    && left.analysis_sha256 == right.analysis_sha256
            })
        })
}

fn passive_sources_are_unique(sources: &[PassiveSourceBinding]) -> bool {
    if sources.len() != RAW_PREREQ_SCENARIOS.len() {
        return false;
    }
    let scenarios = sources
        .iter()
        .map(|source| source.scenario.as_str())
        .collect::<BTreeSet<_>>();
    let identities = sources
        .iter()
        .map(|source| source.artifact_identity.as_str())
        .collect::<BTreeSet<_>>();
    let paths = sources
        .iter()
        .map(|source| source.run_path.as_path())
        .collect::<BTreeSet<_>>();
    scenarios == RAW_PREREQ_SCENARIOS.into_iter().collect()
        && identities.len() == sources.len()
        && paths.len() == sources.len()
        && sources.iter().all(|source| {
            !source.artifact_identity.is_empty()
                && is_sha256(&source.manifest_sha256)
                && is_sha256(&source.events_sha256)
                && is_sha256(&source.analysis_sha256)
                && is_bounded_relative_path(&source.run_path)
        })
}

fn expected_raw_prereq_source_revision(scenario: &str) -> AppResult<(&'static str, &'static str)> {
    match scenario {
        "keyboard-held-release" | "keyboard-edge" => Ok((
            RAW_APPLY_V3_CATALOG_SHA256,
            RAW_APPLY_V3_CAPTURE_PLAN_SHA256,
        )),
        "controller-held-release" | "controller-edge" => {
            Ok((RAW_APPLY_CATALOG_SHA256, RAW_APPLY_CAPTURE_PLAN_SHA256))
        }
        _ => Err(AppError::evidence(format!(
            "raw prerequisite scenario {scenario} has no frozen source revision",
        ))),
    }
}

fn is_sha256(value: &str) -> bool {
    value.len() == 64 && value.bytes().all(|byte| byte.is_ascii_hexdigit())
}

fn resolve_bounded_path(base: &Path, relative: &Path) -> AppResult<PathBuf> {
    if !is_bounded_relative_path(relative) {
        return Err(AppError::usage(
            "active evidence path must be a normalized relative path with at most one parent",
        ));
    }
    let resolved_base = base.canonicalize().usage_context(format!(
        "cannot resolve active evidence base {}",
        base.display()
    ))?;
    let candidate = resolved_base
        .join(relative)
        .canonicalize()
        .usage_context(format!(
            "cannot resolve active evidence path {}",
            relative.display()
        ))?;
    let boundary = resolved_base.parent().unwrap_or(&resolved_base);
    if !candidate.starts_with(boundary) {
        return Err(AppError::usage(
            "active evidence path escapes the repository evidence boundary",
        ));
    }
    Ok(candidate)
}

fn containing_directory(path: &Path) -> &Path {
    path.parent()
        .filter(|parent| !parent.as_os_str().is_empty())
        .unwrap_or_else(|| Path::new("."))
}

fn is_bounded_relative_path(relative: &Path) -> bool {
    !relative.as_os_str().is_empty()
        && !relative.is_absolute()
        && !relative.components().any(|part| {
            matches!(
                part,
                Component::CurDir | Component::RootDir | Component::Prefix(_)
            )
        })
        && relative
            .components()
            .filter(|part| matches!(part, Component::ParentDir))
            .count()
            <= 1
}

fn validate_leaf(value: &str, label: &str) -> AppResult<()> {
    let path = Path::new(value);
    if value.is_empty()
        || path.components().count() != 1
        || matches!(
            path.components().next(),
            Some(
                Component::CurDir
                    | Component::ParentDir
                    | Component::RootDir
                    | Component::Prefix(_)
            )
        )
    {
        return Err(AppError::evidence(format!("{label} is not a leaf path")));
    }
    Ok(())
}

#[allow(clippy::too_many_arguments)]
fn slot(
    ordinal: u32,
    id: &str,
    owner: &str,
    patch_kind: &str,
    rva: u32,
    width: u32,
    target: Option<u32>,
    symbol: Option<&str>,
) -> HookSlot {
    HookSlot {
        ordinal,
        id: id.to_owned(),
        owner: owner.to_owned(),
        patch_kind: patch_kind.to_owned(),
        rva: format!("0x{rva:08X}"),
        width,
        expected_target_rva: target.map(|value| format!("0x{value:08X}")),
        expected_symbol: symbol.map(str::to_owned),
    }
}

pub fn expected_hook_slots() -> Vec<HookSlot> {
    let calls = [
        ("loop-clock", 0x00531231, 0x006266A0),
        ("top-level-update", 0x005312EC, 0x005AB6D0),
        ("conditional-attempt", 0x00555231, 0x002FADC0),
        ("standard-tail", 0x002FBBA5, 0x004607A0),
        ("run-teardown-1", 0x00559696, 0x002FA0C0),
        ("run-teardown-2", 0x005AB84A, 0x002FA0C0),
        ("new-room", 0x0033F5B9, 0x00465180),
        ("render-owner", 0x005312F1, 0x005555C0),
        ("input-manager-sample", 0x00554F7D, 0x0061FC00),
        ("input-device-update", 0x0061FC97, 0x0061F280),
        ("alternate-owner", 0x00554D5E, 0x00559720),
        ("alternate-room", 0x00559B4D, 0x00402980),
        ("alternate-tail", 0x00559B82, 0x00464EE0),
        ("standard-predicate", 0x002FADFA, 0x0028BBA0),
        ("standard-body", 0x002FAE09, 0x00308630),
        ("standard-room-authority", 0x002FBA4D, 0x00402980),
        ("entity-list", 0x00403E70, 0x000186C0),
        ("alternate-state-apply", 0x00559B21, 0x002F8140),
        ("alternate-outcome-0", 0x002F580B, 0x00464EE0),
        ("alternate-outcome-1", 0x002F61FF, 0x00464EE0),
        ("alternate-outcome-2", 0x002F72C9, 0x00464EE0),
        ("alternate-outcome-3", 0x002F7721, 0x00464EE0),
        ("alternate-outcome-4", 0x002F7A10, 0x00464EE0),
        ("standard-room-lifecycle-owner", 0x002FB4F0, 0x0052F1C0),
        ("standard-room-lifecycle-simulation", 0x0052FC77, 0x00402980),
        ("room-change-owner", 0x002FB51C, 0x004318A0),
        ("room-change-simulation", 0x002FE37C, 0x00402980),
    ];
    let mut slots = calls
        .into_iter()
        .enumerate()
        .map(|(index, (id, rva, target))| {
            slot(
                index as u32,
                id,
                "V5",
                "REL32_CALL",
                rva,
                5,
                Some(target),
                None,
            )
        })
        .collect::<Vec<_>>();
    slots.push(slot(
        27,
        "keyboard-physical-poll",
        "V5",
        "IMAGE_POINTER",
        0x007A16F0,
        4,
        Some(0x0066C650),
        None,
    ));
    slots.push(slot(
        28,
        "gamepad-physical-poll",
        "V5",
        "IMAGE_POINTER",
        0x00782CF8,
        4,
        Some(0x00622940),
        None,
    ));
    slots.push(slot(
        29,
        "peek-message",
        "V5",
        "IMPORT_POINTER",
        0x00718714,
        4,
        None,
        Some("USER32!PeekMessageW"),
    ));
    slots.push(slot(
        30,
        "translate-message",
        "V5",
        "IMPORT_POINTER",
        0x00718650,
        4,
        None,
        Some("USER32!TranslateMessage"),
    ));
    slots.push(slot(
        31,
        "dispatch-message",
        "V5",
        "IMPORT_POINTER",
        0x007185B8,
        4,
        None,
        Some("USER32!DispatchMessageW"),
    ));
    let queries = [
        ("keyboard-logical-pressed", 0x007A16A8, 0x0061E8D0),
        ("keyboard-logical-triggered", 0x007A16AC, 0x0061E970),
        ("keyboard-logical-scalar", 0x007A16B0, 0x0061EA20),
        ("gamepad-logical-pressed", 0x00782CB0, 0x0061E8D0),
        ("gamepad-logical-triggered", 0x00782CB4, 0x0061E970),
        ("gamepad-logical-scalar", 0x00782CB8, 0x0061EA20),
        ("player-update-owner", 0x0076BDDC, 0x00382AF0),
    ];
    slots.extend(
        queries
            .into_iter()
            .enumerate()
            .map(|(index, (id, rva, target))| {
                slot(
                    32 + index as u32,
                    id,
                    "ACTIVE",
                    "IMAGE_POINTER",
                    rva,
                    4,
                    Some(target),
                    None,
                )
            }),
    );
    let consumers = [
        ("player-movement-consumer", 0x00385BC2, 0x00379B40),
        ("player-bomb-consumer", 0x00385BC9, 0x00380F40),
        ("player-item-slot-consumer", 0x00385BD2, 0x00391A40),
        ("player-pill-card-consumer", 0x00385BD9, 0x003A3D30),
        ("player-shooting-consumer", 0x00385BE0, 0x0037B020),
        ("player-drop-consumer", 0x00385BE7, 0x003A42B0),
    ];
    slots.extend(
        consumers
            .into_iter()
            .enumerate()
            .map(|(index, (id, rva, target))| {
                slot(
                    39 + index as u32,
                    id,
                    "ACTIVE",
                    "REL32_CALL",
                    rva,
                    5,
                    Some(target),
                    None,
                )
            }),
    );
    slots
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

    fn manifest_path(name: &str) -> PathBuf {
        Path::new(env!("CARGO_MANIFEST_DIR")).join(name)
    }

    fn read_json<T: for<'de> Deserialize<'de>>(name: &str) -> T {
        serde_json::from_slice(&fs::read(manifest_path(name)).unwrap()).unwrap()
    }

    #[test]
    fn active_v2_catalog_and_plan_freeze_the_correct_address_map() {
        assert_eq!(
            containing_directory(Path::new("matrix.json")),
            Path::new(".")
        );
        let catalog_bytes = fs::read(manifest_path("input-active-catalog-v2.json")).unwrap();
        let catalog: ActiveCatalog = serde_json::from_slice(&catalog_bytes).unwrap();
        validate_catalog(&catalog).unwrap();
        assert_eq!(catalog.hook_slots.len(), ACTIVE_SLOT_COUNT);
        assert_eq!(catalog.hook_slots, expected_hook_slots());
        let expected_targets = [
            "0x00382AF0",
            "0x00379B40",
            "0x00380F40",
            "0x00391A40",
            "0x003A3D30",
            "0x0037B020",
            "0x003A42B0",
        ];
        for (slot, target) in catalog.hook_slots[38..].iter().zip(expected_targets) {
            assert_eq!(slot.expected_target_rva.as_deref(), Some(target));
        }

        let plan: ActivePlan = read_json("input-active-plan-v2.json");
        validate_plan_shape(&plan).unwrap();
        assert_eq!(
            sha256_hex(&fs::read(manifest_path("input-active-plan-v2.json")).unwrap()),
            ACTIVE_PLAN_SHA256
        );
        assert_eq!(plan.catalog_path, Path::new("input-active-catalog-v2.json"));
        assert_eq!(plan.catalog_sha256, sha256_hex(&catalog_bytes));
        assert_eq!(
            validate_active_input_script(
                &manifest_path("input-active-repeatability-script-v1.json"),
                &plan.repeatability_input_script_sha256,
            )
            .unwrap(),
            ACTIVE_INPUT_SCRIPT_SHA256
        );

        assert!(
            serde_json::from_slice::<ActiveCatalog>(
                &fs::read(manifest_path("input-active-catalog-v1.json")).unwrap()
            )
            .is_err()
        );
        assert!(
            serde_json::from_slice::<ActivePlan>(
                &fs::read(manifest_path("input-active-plan-v1.json")).unwrap()
            )
            .is_err()
        );
    }

    #[test]
    fn production_active_static_bundle_replays_read_only() {
        let repository = manifest_path("..");
        let validated = validate_active_plan(
            &manifest_path("input-active-plan-v2.json"),
            &repository.join(
                "artifacts/cs-t0/20260819T080844Z-t0-02-c2-original-online-a-a/t0-02-gate-decision.json",
            ),
            &repository.join("artifacts/cs-t0/20260821-t0-03r-v5-loop-gate-7of7.json"),
            &repository
                .join("artifacts/cs-t0/20260823-t0-04-input-raw-prerequisite-4of4.json"),
        )
        .unwrap();
        assert_eq!(validated.target_sha256, FIXED_TARGET_SHA256);
        assert_eq!(validated.plan_sha256, ACTIVE_PLAN_SHA256);
        assert_eq!(validated.catalog_sha256, ACTIVE_CATALOG_SHA256);
        assert_eq!(
            validated.repeatability_input_script_sha256,
            ACTIVE_INPUT_SCRIPT_SHA256
        );
    }

    #[test]
    fn active_v2_catalog_and_plan_mutations_fail_closed() {
        let mut catalog: ActiveCatalog = read_json("input-active-catalog-v2.json");
        catalog.hook_slots[38].expected_target_rva = Some("0x00382450".to_owned());
        assert!(validate_catalog(&catalog).is_err());

        let mut catalog: ActiveCatalog = read_json("input-active-catalog-v2.json");
        catalog.passive_sources[1].artifact_identity =
            catalog.passive_sources[0].artifact_identity.clone();
        assert!(validate_catalog(&catalog).is_err());

        let mut catalog: ActiveCatalog = read_json("input-active-catalog-v2.json");
        catalog.production_active_input_allowed = true;
        assert!(validate_catalog(&catalog).is_err());

        let mut plan: ActivePlan = read_json("input-active-plan-v2.json");
        plan.required_scenarios[1] = plan.required_scenarios[0].clone();
        assert!(validate_plan_shape(&plan).is_err());

        let mut plan: ActivePlan = read_json("input-active-plan-v2.json");
        plan.experiment_requires_one_time_arm = false;
        assert!(validate_plan_shape(&plan).is_err());

        let mut plan: ActivePlan = read_json("input-active-plan-v2.json");
        plan.catalog_path = PathBuf::from("../input-active-catalog-v2.json");
        assert!(validate_plan_shape(&plan).is_err());

        let mut plan: ActivePlan = read_json("input-active-plan-v2.json");
        plan.catalog_sha256 = "0".repeat(64);
        assert!(validate_plan_shape(&plan).is_err());

        let mut plan: ActivePlan = read_json("input-active-plan-v2.json");
        plan.repeatability_input_script_sha256 = "0".repeat(64);
        assert!(validate_plan_shape(&plan).is_err());

        let directory = tempfile::tempdir().unwrap();
        let script = directory.path().join("mutated-input-script.json");
        fs::write(&script, b"{}").unwrap();
        assert!(validate_active_input_script(&script, ACTIVE_INPUT_SCRIPT_SHA256).is_err());

        let plan_path = directory.path().join("mutated-plan-v2.json");
        let mut plan_bytes = fs::read(manifest_path("input-active-plan-v2.json")).unwrap();
        plan_bytes.push(b'\n');
        fs::write(&plan_path, plan_bytes).unwrap();
        let error = validate_active_plan(
            &plan_path,
            Path::new("missing-mode-gate.json"),
            Path::new("missing-loop-gate.json"),
            Path::new("missing-raw-prerequisite.json"),
        )
        .unwrap_err();
        assert_eq!(
            error.message,
            "active plan file does not match the frozen v2 SHA-256"
        );
    }

    fn raw_prereq_decision() -> RawPrereqDecision {
        let matrix_bytes = fs::read(manifest_path("input-raw-prereq-matrix-v1.json")).unwrap();
        let matrix: RawPrereqMatrix = serde_json::from_slice(&matrix_bytes).unwrap();
        RawPrereqDecision {
            schema_version: RAW_PREREQ_SCHEMA_VERSION,
            prerequisite_identity: RAW_PREREQ_IDENTITY.to_owned(),
            status: "PASS".to_owned(),
            active_prerequisite_satisfied: true,
            eligible_for_active_gate: false,
            active_input_allowed: false,
            target_sha256: matrix.target_sha256,
            loop_gate_sha256: matrix.loop_gate_sha256,
            passive_catalog_sha256: matrix.passive_catalog_sha256,
            passive_capture_plan_sha256: matrix.passive_capture_plan_sha256,
            matrix_sha256: sha256_hex(&matrix_bytes),
            sources: matrix
                .sources
                .into_iter()
                .map(|source| RawPrereqSourceDecision {
                    source_passive_catalog_sha256: expected_raw_prereq_source_revision(
                        &source.scenario,
                    )
                    .unwrap()
                    .0
                    .to_owned(),
                    source_passive_capture_plan_sha256: expected_raw_prereq_source_revision(
                        &source.scenario,
                    )
                    .unwrap()
                    .1
                    .to_owned(),
                    scenario: source.scenario,
                    artifact_identity: source.artifact_identity,
                    run_path: source.run_path,
                    manifest_sha256: source.manifest_sha256,
                    events_sha256: source.events_sha256,
                    analysis_sha256: source.analysis_sha256,
                    replay_result: "PASS".to_owned(),
                })
                .collect(),
        }
    }

    fn write_decision(
        directory: &tempfile::TempDir,
        name: &str,
        decision: &RawPrereqDecision,
    ) -> PathBuf {
        let path = directory.path().join(name);
        fs::write(&path, serde_json::to_vec_pretty(decision).unwrap()).unwrap();
        path
    }

    #[test]
    fn raw_prerequisite_decision_mutations_fail_closed() {
        let directory = tempfile::tempdir().unwrap();
        let existing_output = directory.path().join("existing.json");
        fs::write(&existing_output, b"preserve").unwrap();
        assert!(
            write_raw_prerequisite(Path::new("missing-matrix.json"), &existing_output).is_err()
        );
        assert_eq!(fs::read(&existing_output).unwrap(), b"preserve");

        let decision = raw_prereq_decision();
        let valid = write_decision(&directory, "valid.json", &decision);
        validate_raw_prerequisite(&valid).unwrap();

        let mut mutated = decision.clone();
        mutated.eligible_for_active_gate = true;
        let path = write_decision(&directory, "eligible.json", &mutated);
        assert!(validate_raw_prerequisite(&path).is_err());

        let mut mutated = decision.clone();
        mutated.sources[1].artifact_identity = mutated.sources[0].artifact_identity.clone();
        let path = write_decision(&directory, "duplicate.json", &mutated);
        assert!(validate_raw_prerequisite(&path).is_err());

        let mut mutated = decision.clone();
        mutated.sources[0].run_path = PathBuf::from("../../escape");
        let path = write_decision(&directory, "escape.json", &mutated);
        assert!(validate_raw_prerequisite(&path).is_err());

        let mut mutated = decision;
        mutated.sources[0].analysis_sha256 = "not-a-sha".to_owned();
        let path = write_decision(&directory, "hash.json", &mutated);
        assert!(validate_raw_prerequisite(&path).is_err());
    }

    #[test]
    fn raw_prerequisite_matrix_mutations_fail_closed() {
        let matrix: RawPrereqMatrix = read_json("input-raw-prereq-matrix-v1.json");
        validate_raw_prereq_matrix(&matrix).unwrap();

        let mut mutated = matrix.clone();
        mutated.sources[1].scenario = mutated.sources[0].scenario.clone();
        assert!(validate_raw_prereq_matrix(&mutated).is_err());

        let mut mutated = matrix.clone();
        mutated.sources[1].artifact_identity = mutated.sources[0].artifact_identity.clone();
        assert!(validate_raw_prereq_matrix(&mutated).is_err());

        let mut mutated = matrix.clone();
        mutated.sources[0].run_path = PathBuf::from("../../escape");
        assert!(validate_raw_prereq_matrix(&mutated).is_err());

        let mut mutated = matrix;
        mutated.sources[0].manifest_sha256 = "not-a-sha".to_owned();
        assert!(validate_raw_prereq_matrix(&mutated).is_err());
    }

    #[test]
    fn production_raw_prerequisite_decision_is_passive_only_and_hash_bound() {
        let path =
            manifest_path("../artifacts/cs-t0/20260823-t0-04-input-raw-prerequisite-4of4.json");
        let (decision, _) = validate_raw_prerequisite(&path).unwrap();
        assert_eq!(decision.status, "PASS");
        assert!(decision.active_prerequisite_satisfied);
        assert!(!decision.eligible_for_active_gate);
        assert!(!decision.active_input_allowed);
        assert_eq!(decision.sources.len(), RAW_PREREQ_SCENARIOS.len());
    }
}
