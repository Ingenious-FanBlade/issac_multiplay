use std::collections::{BTreeMap, BTreeSet};
use std::fs;
use std::path::{Component, Path, PathBuf};

use serde::{Deserialize, Serialize};

use crate::MODE_TOOL_IDENTITY;
use crate::error::{AppError, AppResult, UsageContext};
use crate::ledger::{
    LoadedLedger, StaticInspectionReport, TargetInspection, TargetLedger, TargetSpec, target_map,
};
use crate::pe::sha256_hex;

const MODE_LEDGER_SCHEMA_VERSION: u32 = 2;
const REQUIRED_FIXED_VARIANT: &str = "online-mod-patched-2026-04-21";

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ModeLedger {
    pub schema_version: u32,
    pub ledger_id: String,
    pub experiment_identity: String,
    pub fixed_variant: String,
    pub baseline: BaselineReference,
    pub targets: Vec<TargetSpec>,
    pub signals: Vec<ModeSignalSpec>,
    pub wrappers: Vec<WrapperSpec>,
    pub conflicts: Vec<ConflictSpec>,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct BaselineReference {
    pub file: String,
    pub ledger_id: String,
    pub sha256: String,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ModeSignalSpec {
    pub id: String,
    pub classification: String,
    pub target_ids: Vec<String>,
    pub rule: String,
    pub minimum_observations: u32,
    pub evidence_label: String,
    pub evidence_sources: Vec<String>,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct WrapperSpec {
    pub id: String,
    pub patch_kind: String,
    pub target_ids: Vec<String>,
    pub runtime_import: Option<String>,
    pub runtime_owner: String,
    pub abi: String,
    pub thread: String,
    pub original_calls_required: u32,
    pub mutates_arguments: bool,
    pub evidence_sources: Vec<String>,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ConflictSpec {
    pub id: String,
    pub action: String,
    pub reason: String,
}

#[derive(Clone, Debug)]
pub struct LoadedModeLedger {
    pub ledger: ModeLedger,
    pub raw_sha256: String,
    pub baseline_path: PathBuf,
    pub baseline: LoadedLedger,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ModeInspectionReport {
    pub schema_version: u32,
    pub result: String,
    pub ledger_id: String,
    pub ledger_sha256: String,
    pub fixed_variant: String,
    pub baseline: StaticInspectionReport,
    pub targets: Vec<TargetInspection>,
    pub signal_count: usize,
    pub wrapper_count: usize,
    pub corroborating_original_online_signals: usize,
    pub offline_positive_signals: usize,
    pub live_custom_modes_disarmed: bool,
}

impl LoadedModeLedger {
    pub fn read(path: &Path) -> AppResult<Self> {
        let raw =
            fs::read(path).usage_context(format!("cannot read mode ledger {}", path.display()))?;
        let ledger: ModeLedger = serde_json::from_slice(&raw)
            .usage_context(format!("cannot parse mode ledger {}", path.display()))?;
        ledger.validate_shape()?;

        let baseline_component = Path::new(&ledger.baseline.file);
        if baseline_component.is_absolute()
            || baseline_component
                .components()
                .any(|part| !matches!(part, Component::Normal(_)))
        {
            return Err(AppError::usage(
                "mode ledger baseline file must be one relative file name",
            ));
        }
        let baseline_path = path
            .parent()
            .unwrap_or_else(|| Path::new("."))
            .join(baseline_component);
        let baseline = LoadedLedger::read(&baseline_path)?;
        if baseline.ledger.ledger_id != ledger.baseline.ledger_id {
            return Err(AppError::evidence(format!(
                "baseline ledger id mismatch: expected {}, got {}",
                ledger.baseline.ledger_id, baseline.ledger.ledger_id
            )));
        }
        if !baseline
            .raw_sha256
            .eq_ignore_ascii_case(&ledger.baseline.sha256)
        {
            return Err(AppError::evidence(format!(
                "baseline ledger SHA-256 mismatch: expected {}, got {}",
                ledger.baseline.sha256, baseline.raw_sha256
            )));
        }
        ledger.target_ledger(&baseline).validate_schema()?;
        ledger.validate_against_baseline(&baseline)?;
        Ok(Self {
            ledger,
            raw_sha256: sha256_hex(&raw),
            baseline_path,
            baseline,
        })
    }

    pub fn report(
        &self,
        baseline: StaticInspectionReport,
        targets: Vec<TargetInspection>,
    ) -> ModeInspectionReport {
        ModeInspectionReport {
            schema_version: MODE_LEDGER_SCHEMA_VERSION,
            result: "PASS".to_owned(),
            ledger_id: self.ledger.ledger_id.clone(),
            ledger_sha256: self.raw_sha256.clone(),
            fixed_variant: self.ledger.fixed_variant.clone(),
            targets,
            signal_count: self.ledger.signals.len(),
            wrapper_count: self.ledger.wrappers.len(),
            corroborating_original_online_signals: self
                .ledger
                .signals
                .iter()
                .filter(|signal| signal.classification == "ORIGINAL_ONLINE")
                .count(),
            offline_positive_signals: self
                .ledger
                .signals
                .iter()
                .filter(|signal| signal.classification == "OFFLINE_POSITIVE")
                .count(),
            live_custom_modes_disarmed: true,
            baseline,
        }
    }

    pub fn target_ledger(&self) -> TargetLedger {
        self.ledger.target_ledger(&self.baseline)
    }

    pub fn combined_snapshot_ledger(&self) -> LoadedLedger {
        let mut ledger = self.baseline.ledger.clone();
        ledger.ledger_id = format!("{}+{}", ledger.ledger_id, self.ledger.ledger_id);
        ledger.targets.extend(self.ledger.targets.clone());
        LoadedLedger {
            ledger,
            raw_sha256: self.raw_sha256.clone(),
        }
    }
}

impl ModeLedger {
    fn validate_shape(&self) -> AppResult<()> {
        if self.schema_version != MODE_LEDGER_SCHEMA_VERSION {
            return Err(AppError::usage(format!(
                "unsupported mode ledger schema {}",
                self.schema_version
            )));
        }
        if self.ledger_id.trim().is_empty() || self.experiment_identity != MODE_TOOL_IDENTITY {
            return Err(AppError::usage("invalid T0-02 experiment identity"));
        }
        if self.fixed_variant != REQUIRED_FIXED_VARIANT {
            return Err(AppError::usage(format!(
                "T0-02 mode ledger must remain fixed to {REQUIRED_FIXED_VARIANT}"
            )));
        }
        validate_sha256(&self.baseline.sha256, "baseline ledger SHA-256")?;
        if self.targets.is_empty() {
            return Err(AppError::usage(
                "mode ledger has no independent candidate targets",
            ));
        }
        let mut signal_ids = BTreeSet::new();
        for signal in &self.signals {
            if !signal_ids.insert(signal.id.as_str()) {
                return Err(AppError::usage(format!(
                    "duplicate mode signal {}",
                    signal.id
                )));
            }
            if signal.target_ids.is_empty()
                || signal.rule.trim().is_empty()
                || signal.minimum_observations == 0
                || signal.evidence_sources.is_empty()
            {
                return Err(AppError::usage(format!(
                    "mode signal {} is incomplete",
                    signal.id
                )));
            }
            if !matches!(
                signal.classification.as_str(),
                "ORIGINAL_ONLINE" | "OFFLINE_POSITIVE" | "LIFECYCLE"
            ) {
                return Err(AppError::usage(format!(
                    "mode signal {} has invalid classification {}",
                    signal.id, signal.classification
                )));
            }
        }
        if self
            .signals
            .iter()
            .filter(|signal| signal.classification == "ORIGINAL_ONLINE")
            .count()
            < 3
        {
            return Err(AppError::evidence(
                "mode ledger has fewer than three OriginalOnline signals",
            ));
        }
        if self
            .signals
            .iter()
            .filter(|signal| signal.classification == "OFFLINE_POSITIVE")
            .count()
            < 2
        {
            return Err(AppError::evidence(
                "mode ledger has fewer than two offline positive signals",
            ));
        }
        let mut wrapper_ids = BTreeSet::new();
        for wrapper in &self.wrappers {
            if !wrapper_ids.insert(wrapper.id.as_str())
                || wrapper.original_calls_required != 1
                || wrapper.mutates_arguments
                || wrapper.abi.trim().is_empty()
                || wrapper.thread.trim().is_empty()
                || wrapper.evidence_sources.is_empty()
                || (wrapper.target_ids.is_empty() && wrapper.runtime_import.is_none())
            {
                return Err(AppError::usage(format!(
                    "wrapper {} violates passthrough ledger rules",
                    wrapper.id
                )));
            }
        }
        if self.conflicts.is_empty()
            || self
                .conflicts
                .iter()
                .any(|conflict| conflict.action != "RETIRE_AND_DISARM")
        {
            return Err(AppError::usage(
                "every T0-02 conflict must retire and disarm",
            ));
        }
        Ok(())
    }

    fn validate_against_baseline(&self, baseline: &LoadedLedger) -> AppResult<()> {
        if !baseline
            .ledger
            .image
            .variants
            .iter()
            .any(|variant| variant.id == self.fixed_variant)
        {
            return Err(AppError::evidence(format!(
                "fixed variant {} is absent from baseline ledger",
                self.fixed_variant
            )));
        }
        let baseline_targets = target_map(&baseline.ledger);
        let mode_targets = self
            .targets
            .iter()
            .map(|target| (target.id.as_str(), target))
            .collect::<BTreeMap<_, _>>();
        if mode_targets.len() != self.targets.len()
            || mode_targets
                .keys()
                .any(|id| baseline_targets.contains_key(id))
        {
            return Err(AppError::usage(
                "mode target ids must be unique and must not shadow the T0-01 baseline",
            ));
        }
        let mut referenced = BTreeMap::new();
        for signal in &self.signals {
            for id in &signal.target_ids {
                if !baseline_targets.contains_key(id.as_str())
                    && !mode_targets.contains_key(id.as_str())
                {
                    return Err(AppError::evidence(format!(
                        "mode signal {} references unknown target {id}",
                        signal.id
                    )));
                }
                referenced.insert(id.as_str(), signal.id.as_str());
            }
        }
        for wrapper in &self.wrappers {
            for id in &wrapper.target_ids {
                let target = baseline_targets
                    .get(id.as_str())
                    .copied()
                    .or_else(|| mode_targets.get(id.as_str()).copied());
                let Some(target) = target else {
                    return Err(AppError::evidence(format!(
                        "wrapper {} references unknown target {id}",
                        wrapper.id
                    )));
                };
                if baseline_targets.contains_key(id.as_str())
                    && target.evidence_label != "STATIC_CONFIRMED"
                {
                    return Err(AppError::evidence(format!(
                        "wrapper {} baseline target {id} is not STATIC_CONFIRMED",
                        wrapper.id
                    )));
                }
                if mode_targets.contains_key(id.as_str())
                    && (target.kind != "relativeCall"
                        || !matches!(
                            target.evidence_label.as_str(),
                            "UNKNOWN" | "RUNTIME_OBSERVED"
                        ))
                {
                    return Err(AppError::evidence(format!(
                        "wrapper {} candidate target {id} is not a bounded relative-call experiment",
                        wrapper.id
                    )));
                }
            }
        }
        for required in [
            "manager-global-slot",
            "network-backend-global-slot",
            "steam-networking-context-descriptor",
            "steam-backend-vtable",
            "input-advance-call",
            "online-dispatch-call-5110ab",
            "lua-newstate-creator-call",
            "lua-lnewstate-creator-call",
            "lua-close-call",
            "lua-simulation-pcall",
        ] {
            if !referenced.contains_key(required)
                && !self
                    .wrappers
                    .iter()
                    .any(|wrapper| wrapper.target_ids.iter().any(|id| id == required))
            {
                return Err(AppError::evidence(format!(
                    "T0-02 ledger does not bind required target {required}"
                )));
            }
        }
        for required in [
            "game-global-slot",
            "post-game-start-dispatcher",
            "pre-game-exit-dispatcher",
            "pre-game-exit-call",
            "post-game-start-call-new-1",
            "post-game-start-call-new-2",
            "post-game-start-call-new-3",
            "post-game-start-call-continued-1",
            "post-game-start-call-new-4",
            "post-game-start-call-continued-2",
            "run-teardown-entry",
            "run-teardown-call-state",
            "run-teardown-call-shutdown",
        ] {
            if !mode_targets.contains_key(required) || !referenced.contains_key(required) {
                return Err(AppError::evidence(format!(
                    "T0-02 ledger does not bind required lifecycle candidate {required}"
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

fn validate_sha256(value: &str, label: &str) -> AppResult<()> {
    if value.len() != 64 || !value.bytes().all(|byte| byte.is_ascii_hexdigit()) {
        return Err(AppError::usage(format!(
            "{label} is not a SHA-256 hex value"
        )));
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn production_mode_ledger_is_self_consistent() {
        let path =
            Path::new(env!("CARGO_MANIFEST_DIR")).join("ledger/isaac-cs-t0-mode-ledger.json");
        let loaded = LoadedModeLedger::read(&path).unwrap();
        assert_eq!(loaded.ledger.fixed_variant, REQUIRED_FIXED_VARIANT);
        assert!(
            loaded.ledger.wrappers.iter().all(|wrapper| {
                wrapper.original_calls_required == 1 && !wrapper.mutates_arguments
            })
        );
        assert!(loaded.ledger.targets.iter().all(|target| {
            matches!(
                target.evidence_label.as_str(),
                "UNKNOWN" | "RUNTIME_OBSERVED"
            )
        }));
    }

    #[test]
    fn mode_ledger_rejects_live_custom_identity_or_weak_signal_set() {
        let mut ledger = ModeLedger {
            schema_version: MODE_LEDGER_SCHEMA_VERSION,
            ledger_id: "x".into(),
            experiment_identity: "wrong".into(),
            fixed_variant: REQUIRED_FIXED_VARIANT.into(),
            baseline: BaselineReference {
                file: "x.json".into(),
                ledger_id: "x".into(),
                sha256: "A".repeat(64),
            },
            targets: Vec::new(),
            signals: Vec::new(),
            wrappers: Vec::new(),
            conflicts: Vec::new(),
        };
        assert!(ledger.validate_shape().is_err());
        ledger.experiment_identity = MODE_TOOL_IDENTITY.into();
        assert!(ledger.validate_shape().is_err());
    }
}
