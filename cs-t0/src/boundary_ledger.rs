use std::collections::{BTreeMap, BTreeSet};
use std::fs;
use std::path::{Component, Path, PathBuf};

use serde::{Deserialize, Serialize};

use crate::BOUNDARY_TOOL_IDENTITY;
use crate::error::{AppError, AppResult, UsageContext};
use crate::ledger::{
    LoadedLedger, StaticInspectionReport, TargetInspection, TargetLedger, TargetSpec, target_map,
};
use crate::mode_ledger::LoadedModeLedger;
use crate::pe::sha256_hex;

pub const BOUNDARY_LEDGER_SCHEMA_VERSION: u32 = 1;
pub const REQUIRED_BOUNDARY_VARIANT: &str = "online-mod-patched-2026-04-21";

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct BoundaryLedger {
    pub schema_version: u32,
    pub ledger_id: String,
    pub experiment_identity: String,
    pub fixed_variant: String,
    pub baseline: LedgerReference,
    pub mode_baseline: LedgerReference,
    pub gate_decision: GateDecisionReference,
    pub targets: Vec<TargetSpec>,
    pub probes: Vec<BoundaryProbeSpec>,
    pub constraints: Vec<BoundaryConstraint>,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct LedgerReference {
    pub file: String,
    pub ledger_id: String,
    pub sha256: String,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct GateDecisionReference {
    pub file: String,
    pub sha256: String,
    pub required_status: String,
    pub required_analysis_revision: String,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct BoundaryProbeSpec {
    pub id: String,
    pub classification: String,
    pub target_ids: Vec<String>,
    pub hypothesis: String,
    pub evidence_label: String,
    pub evidence_sources: Vec<String>,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct BoundaryConstraint {
    pub id: String,
    pub rule: String,
    pub action: String,
}

#[derive(Clone, Debug)]
pub struct LoadedBoundaryLedger {
    pub ledger: BoundaryLedger,
    pub raw_sha256: String,
    pub baseline: LoadedLedger,
    pub mode: LoadedModeLedger,
    pub gate_path: PathBuf,
    pub gate_sha256: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct BoundaryInspectionReport {
    pub schema_version: u32,
    pub result: String,
    pub ledger_id: String,
    pub ledger_sha256: String,
    pub fixed_variant: String,
    pub baseline_ledger_sha256: String,
    pub mode_ledger_sha256: String,
    pub gate_decision_sha256: String,
    pub gate_status: String,
    pub baseline: StaticInspectionReport,
    pub targets: Vec<TargetInspection>,
    pub probe_count: usize,
    pub semantic_result: String,
}

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
struct GateDecisionDocument {
    status: String,
    analysis_revision: String,
    #[serde(default, alias = "t003Unlocked", rename = "t0_03Unlocked")]
    t0_03_unlocked: bool,
}

impl LoadedBoundaryLedger {
    pub fn read(path: &Path) -> AppResult<Self> {
        let raw = fs::read(path)
            .usage_context(format!("cannot read boundary ledger {}", path.display()))?;
        let ledger: BoundaryLedger = serde_json::from_slice(&raw)
            .usage_context(format!("cannot parse boundary ledger {}", path.display()))?;
        ledger.validate_shape()?;
        let parent = path.parent().unwrap_or_else(|| Path::new("."));

        let baseline_path = resolve_relative(parent, &ledger.baseline.file, "baseline ledger")?;
        let baseline = LoadedLedger::read(&baseline_path)?;
        verify_reference(
            &ledger.baseline,
            &baseline.ledger.ledger_id,
            &baseline.raw_sha256,
            "baseline ledger",
        )?;

        let mode_path = resolve_relative(parent, &ledger.mode_baseline.file, "mode ledger")?;
        let mode = LoadedModeLedger::read(&mode_path)?;
        verify_reference(
            &ledger.mode_baseline,
            &mode.ledger.ledger_id,
            &mode.raw_sha256,
            "mode ledger",
        )?;
        if mode.baseline.raw_sha256 != baseline.raw_sha256 {
            return Err(AppError::evidence(
                "boundary baseline and T0-02 baseline do not identify the same T0-01 ledger",
            ));
        }

        let gate_path = resolve_relative(parent, &ledger.gate_decision.file, "gate decision")?;
        let gate_raw = fs::read(&gate_path)
            .usage_context(format!("cannot read gate decision {}", gate_path.display()))?;
        let gate_sha256 = sha256_hex(&gate_raw);
        if !gate_sha256.eq_ignore_ascii_case(&ledger.gate_decision.sha256) {
            return Err(AppError::evidence(format!(
                "gate decision SHA-256 mismatch: expected {}, got {gate_sha256}",
                ledger.gate_decision.sha256
            )));
        }
        let gate: GateDecisionDocument = serde_json::from_slice(&gate_raw).usage_context(
            format!("cannot parse gate decision {}", gate_path.display()),
        )?;
        if gate.status != ledger.gate_decision.required_status
            || gate.analysis_revision != ledger.gate_decision.required_analysis_revision
            || !gate.t0_03_unlocked
        {
            return Err(AppError::evidence(
                "T0-02 gate decision does not authorize T0-03",
            ));
        }

        let target_ledger = ledger.target_ledger(&baseline);
        target_ledger.validate_schema()?;
        ledger.validate_targets(&baseline, &mode)?;
        Ok(Self {
            ledger,
            raw_sha256: sha256_hex(&raw),
            baseline,
            mode,
            gate_path,
            gate_sha256,
        })
    }

    pub fn target_ledger(&self) -> TargetLedger {
        self.ledger.target_ledger(&self.baseline)
    }

    pub fn combined_snapshot_ledger(&self) -> LoadedLedger {
        let mut ledger = self.baseline.ledger.clone();
        ledger.ledger_id = format!(
            "{}+{}+{}",
            ledger.ledger_id, self.mode.ledger.ledger_id, self.ledger.ledger_id
        );
        ledger.targets.extend(self.mode.ledger.targets.clone());
        ledger.targets.extend(self.ledger.targets.clone());
        LoadedLedger {
            ledger,
            raw_sha256: self.raw_sha256.clone(),
        }
    }

    pub fn report(
        &self,
        baseline: StaticInspectionReport,
        targets: Vec<TargetInspection>,
    ) -> BoundaryInspectionReport {
        BoundaryInspectionReport {
            schema_version: BOUNDARY_LEDGER_SCHEMA_VERSION,
            result: "PASS".to_owned(),
            ledger_id: self.ledger.ledger_id.clone(),
            ledger_sha256: self.raw_sha256.clone(),
            fixed_variant: self.ledger.fixed_variant.clone(),
            baseline_ledger_sha256: self.baseline.raw_sha256.clone(),
            mode_ledger_sha256: self.mode.raw_sha256.clone(),
            gate_decision_sha256: self.gate_sha256.clone(),
            gate_status: "PASS".to_owned(),
            baseline,
            targets,
            probe_count: self.ledger.probes.len(),
            semantic_result: "UNKNOWN_UNTIL_PASSIVE_SMOKE".to_owned(),
        }
    }
}

impl BoundaryLedger {
    fn validate_shape(&self) -> AppResult<()> {
        if self.schema_version != BOUNDARY_LEDGER_SCHEMA_VERSION {
            return Err(AppError::usage(format!(
                "unsupported boundary ledger schema {}",
                self.schema_version
            )));
        }
        if self.ledger_id.trim().is_empty() || self.experiment_identity != BOUNDARY_TOOL_IDENTITY {
            return Err(AppError::usage("invalid T0-03 experiment identity"));
        }
        if self.fixed_variant != REQUIRED_BOUNDARY_VARIANT {
            return Err(AppError::usage(format!(
                "T0-03 boundary ledger must remain fixed to {REQUIRED_BOUNDARY_VARIANT}"
            )));
        }
        for (value, label) in [
            (&self.baseline.sha256, "baseline SHA-256"),
            (&self.mode_baseline.sha256, "mode SHA-256"),
            (&self.gate_decision.sha256, "gate SHA-256"),
        ] {
            validate_sha256(value, label)?;
        }
        if self.gate_decision.required_status != "PASS"
            || self.gate_decision.required_analysis_revision != "t0-02-packet-pairing-v2"
        {
            return Err(AppError::usage(
                "boundary ledger must require the packet-v2 T0-02 PASS gate",
            ));
        }
        if self.targets.is_empty() || self.probes.is_empty() || self.constraints.is_empty() {
            return Err(AppError::usage("boundary ledger is incomplete"));
        }
        let mut probe_ids = BTreeSet::new();
        for probe in &self.probes {
            if !probe_ids.insert(probe.id.as_str())
                || probe.target_ids.is_empty()
                || probe.hypothesis.trim().is_empty()
                || probe.evidence_sources.is_empty()
                || !matches!(
                    probe.classification.as_str(),
                    "INPUT" | "GAMEPLAY" | "ENTITY" | "DRAIN" | "LUA" | "RENDER" | "DIAGNOSTIC"
                )
                || !matches!(
                    probe.evidence_label.as_str(),
                    "STATIC_CONFIRMED" | "RUNTIME_OBSERVED" | "REJECTED_HYPOTHESIS" | "UNKNOWN"
                )
            {
                return Err(AppError::usage(format!(
                    "boundary probe {} is incomplete or invalid",
                    probe.id
                )));
            }
        }
        let mut constraint_ids = BTreeSet::new();
        for constraint in &self.constraints {
            if !constraint_ids.insert(constraint.id.as_str())
                || constraint.rule.trim().is_empty()
                || !matches!(
                    constraint.action.as_str(),
                    "FAIL" | "DISARM_RESTORE" | "REPORT_UNKNOWN"
                )
            {
                return Err(AppError::usage(format!(
                    "boundary constraint {} is incomplete or invalid",
                    constraint.id
                )));
            }
        }
        Ok(())
    }

    fn validate_targets(&self, baseline: &LoadedLedger, mode: &LoadedModeLedger) -> AppResult<()> {
        let baseline_targets = target_map(&baseline.ledger);
        let mode_targets = mode
            .ledger
            .targets
            .iter()
            .map(|target| (target.id.as_str(), target))
            .collect::<BTreeMap<_, _>>();
        let boundary_targets = self
            .targets
            .iter()
            .map(|target| (target.id.as_str(), target))
            .collect::<BTreeMap<_, _>>();
        if boundary_targets.len() != self.targets.len()
            || boundary_targets
                .keys()
                .any(|id| baseline_targets.contains_key(id) || mode_targets.contains_key(id))
        {
            return Err(AppError::usage(
                "boundary target ids must be unique and independent of T0-01/T0-02",
            ));
        }
        let mut referenced = BTreeSet::new();
        for probe in &self.probes {
            for id in &probe.target_ids {
                if !boundary_targets.contains_key(id.as_str())
                    && !baseline_targets.contains_key(id.as_str())
                    && !mode_targets.contains_key(id.as_str())
                {
                    return Err(AppError::evidence(format!(
                        "boundary probe {} references unknown target {id}",
                        probe.id
                    )));
                }
                referenced.insert(id.as_str());
            }
        }
        for required in [
            "input-sample-candidate-call",
            "gameplay-dispatch-suppression-branch",
            "gameplay-update-call",
            "entity-update-owner-call",
            "entity-list-update-entry",
            "remove-finalize-call",
            "post-update-dispatch-call",
            "post-new-room-dispatch-call",
            "render-call",
            "post-render-dispatch-call",
            "spawn-entity-entry",
            "online-dispatch-call-5110ab",
        ] {
            if !referenced.contains(required) {
                return Err(AppError::evidence(format!(
                    "boundary ledger does not bind required target {required}"
                )));
            }
        }
        Ok(())
    }

    fn target_ledger(&self, baseline: &LoadedLedger) -> TargetLedger {
        TargetLedger {
            schema_version: baseline.ledger.schema_version,
            ledger_id: format!("{}-targets", self.ledger_id),
            experiment_identity: self.experiment_identity.clone(),
            image: baseline.ledger.image.clone(),
            targets: self.targets.clone(),
            candidates: Vec::new(),
            diagnostics: Vec::new(),
        }
    }
}

fn verify_reference(
    reference: &LedgerReference,
    actual_id: &str,
    actual_hash: &str,
    label: &str,
) -> AppResult<()> {
    if reference.ledger_id != actual_id || !reference.sha256.eq_ignore_ascii_case(actual_hash) {
        return Err(AppError::evidence(format!(
            "{label} identity or SHA-256 mismatch"
        )));
    }
    Ok(())
}

fn resolve_relative(parent: &Path, value: &str, label: &str) -> AppResult<PathBuf> {
    let relative = Path::new(value);
    if relative.is_absolute()
        || relative.components().any(|component| {
            matches!(
                component,
                Component::Prefix(_) | Component::RootDir | Component::CurDir
            )
        })
    {
        return Err(AppError::usage(format!("{label} path must be relative")));
    }
    Ok(parent.join(relative))
}

fn validate_sha256(value: &str, label: &str) -> AppResult<()> {
    if value.len() != 64 || !value.bytes().all(|byte| byte.is_ascii_hexdigit()) {
        return Err(AppError::usage(format!("{label} is not a SHA-256")));
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn production_boundary_ledger_is_self_consistent() {
        let path =
            Path::new(env!("CARGO_MANIFEST_DIR")).join("ledger/isaac-cs-t0-boundary-ledger.json");
        let loaded = LoadedBoundaryLedger::read(&path).unwrap();
        assert_eq!(loaded.ledger.fixed_variant, REQUIRED_BOUNDARY_VARIANT);
        assert_eq!(loaded.gate_sha256, loaded.ledger.gate_decision.sha256);
        assert!(loaded.ledger.targets.iter().all(|target| {
            matches!(
                target.evidence_label.as_str(),
                "STATIC_CONFIRMED" | "UNKNOWN"
            )
        }));
    }

    #[test]
    fn boundary_paths_reject_absolute_and_curdir_forms() {
        assert!(resolve_relative(Path::new("x"), "C:/game", "x").is_err());
        assert!(resolve_relative(Path::new("x"), "./gate.json", "x").is_err());
        assert!(resolve_relative(Path::new("x"), "../gate.json", "x").is_ok());
    }
}
