use std::collections::BTreeSet;
use std::fs;
use std::path::{Path, PathBuf};

use chrono::{DateTime, Utc};
use serde::Deserialize;

use crate::active_artifact::{
    ActiveAnalysisReport, ActiveRun, ActiveRunConfig, ActiveRunFinish, ActiveTargetSlot,
    validate_active_run_config,
};
use crate::active_catalog::validate_active_input_script;
use crate::active_ipc::NativeActiveEventRecord;
use crate::active_verification::{
    ValidatedActiveVerification, read_runtime_active_verification_v2,
};
use crate::arm_token::{ArmClaim, arm_token_sha256, consume_arm_token};
use crate::error::{AppError, AppResult, UsageContext};

#[derive(Clone, Debug)]
pub struct VerifiedActiveExecution {
    pub verification: ValidatedActiveVerification,
}

pub fn verify_active_execution_before_cas(
    verification_path: &Path,
) -> AppResult<VerifiedActiveExecution> {
    Ok(VerifiedActiveExecution {
        verification: read_runtime_active_verification_v2(verification_path)?,
    })
}

pub struct ClaimedActiveRun {
    claim: ArmClaim,
    claim_path: PathBuf,
    run: Option<ActiveRun>,
    fallback_summary: ActiveRunFinish,
}

impl VerifiedActiveExecution {
    #[allow(clippy::too_many_arguments)]
    pub fn consume_and_begin(
        self,
        grant_path: &Path,
        prerequisite_path: &Path,
        arm_token: &str,
        case_id: &str,
        scenario: &str,
        now: DateTime<Utc>,
        output_directory: &Path,
        run_config: ActiveRunConfig,
    ) -> AppResult<ClaimedActiveRun> {
        let _ = self.verification;
        validate_active_run_config(&run_config)?;
        if run_config.case_id != case_id
            || run_config.scenario != scenario
            || run_config.arm_token_sha256 != arm_token_sha256(arm_token)?
            || output_directory.exists()
        {
            return Err(AppError::usage(
                "active run configuration must bind the unconsumed arm token, case, scenario, and a new output directory",
            ));
        }
        let (claim, claim_path) = consume_arm_token(
            grant_path,
            prerequisite_path,
            arm_token,
            case_id,
            scenario,
            now,
        )?;
        match ActiveRun::create(output_directory, run_config.clone()) {
            Ok(run) => Ok(ClaimedActiveRun {
                claim,
                claim_path,
                run: Some(run),
                fallback_summary: ActiveRunFinish::default(),
            }),
            Err(error) => {
                let reason =
                    format!("active artifact initialization failed after arm token CAS: {error}");
                let fallback = post_claim_fallback_directory(output_directory);
                let fallback_result = ActiveRun::create(&fallback, run_config)
                    .and_then(|run| run.fail(&reason, None, ActiveRunFinish::default()));
                match fallback_result {
                    Ok(()) => Err(AppError::evidence(format!(
                        "{reason}; create-new FAILED artifact={}",
                        fallback.display()
                    ))),
                    Err(fallback_error) => Err(AppError::evidence(format!(
                        "{reason}; claim={} remains the durable consume-once record; cannot create fallback FAILED artifact: {fallback_error}",
                        claim_path.display()
                    ))),
                }
            }
        }
    }
}

impl ClaimedActiveRun {
    pub fn claim(&self) -> &ArmClaim {
        &self.claim
    }

    pub fn claim_path(&self) -> &Path {
        &self.claim_path
    }

    pub fn directory(&self) -> &Path {
        self.run
            .as_ref()
            .expect("claimed active run remains armed")
            .directory()
    }

    pub fn append_native(&mut self, record: &NativeActiveEventRecord) -> AppResult<()> {
        self.run
            .as_mut()
            .ok_or_else(|| AppError::evidence("claimed active run is already finalized"))?
            .append_native(record)
    }

    pub fn update_failure_summary(&mut self, summary: ActiveRunFinish) {
        self.fallback_summary = summary;
    }

    pub fn fail(
        mut self,
        reason: &str,
        target_slots_after: Option<Vec<ActiveTargetSlot>>,
        summary: ActiveRunFinish,
    ) -> AppResult<()> {
        self.fallback_summary = summary;
        let run = self
            .run
            .take()
            .ok_or_else(|| AppError::evidence("claimed active run is already finalized"))?;
        run.fail(reason, target_slots_after, summary)
    }

    pub fn finish(
        mut self,
        target_slots_after: Vec<ActiveTargetSlot>,
        summary: ActiveRunFinish,
    ) -> AppResult<ActiveAnalysisReport> {
        self.fallback_summary = summary;
        let run = self
            .run
            .take()
            .ok_or_else(|| AppError::evidence("claimed active run is already finalized"))?;
        run.finish(target_slots_after, summary)
    }
}

impl Drop for ClaimedActiveRun {
    fn drop(&mut self) {
        if let Some(run) = self.run.take() {
            let _ = run.fail(
                "active orchestration abandoned after arm token CAS",
                None,
                self.fallback_summary,
            );
        }
    }
}

fn post_claim_fallback_directory(output_directory: &Path) -> PathBuf {
    let leaf = output_directory
        .file_name()
        .and_then(|name| name.to_str())
        .unwrap_or("active-artifact");
    output_directory.with_file_name(format!("{leaf}.post-cas-failed"))
}

#[cfg(windows)]
#[allow(dead_code)] // Wired by the schema-4 live capture path once Native/Helper exports exist.
pub(crate) fn drain_active_and_companion(
    run: &mut ClaimedActiveRun,
    companion_run: &mut crate::loop_commands::CompanionLoopRun,
    active_mapping: &crate::active_ipc::ActiveMapping,
    loop_mapping: &crate::loop_ipc::LoopMapping,
    next_active: &mut u64,
    next_loop: &mut u64,
) -> AppResult<()> {
    let active_result = active_mapping.drain(next_active).and_then(|records| {
        for record in &records {
            run.append_native(record)?;
        }
        Ok(())
    });
    let loop_result =
        crate::loop_commands::drain_companion_loop(companion_run, loop_mapping, next_loop)
            .map(|_| ());
    match (active_result, loop_result) {
        (Ok(()), Ok(())) => Ok(()),
        (Err(active), Ok(())) => Err(active),
        (Ok(()), Err(loop_error)) => Err(loop_error),
        (Err(active), Err(loop_error)) => Err(AppError::evidence(format!(
            "active ring drain failed: {active}; companion V5 ring drain also failed: {loop_error}"
        ))),
    }
}

#[derive(Clone, Debug, Eq, PartialEq, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct RepeatabilityScript {
    pub schema_version: u32,
    pub script_identity: String,
    pub authority_clock: String,
    pub steps: Vec<RepeatabilityStep>,
}

#[derive(Clone, Debug, Eq, PartialEq, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct RepeatabilityStep {
    pub relative_tick: u64,
    pub keys_down: Vec<String>,
}

pub fn load_repeatability_script(
    path: &Path,
    expected_sha256: &str,
) -> AppResult<RepeatabilityScript> {
    validate_active_input_script(path, expected_sha256)?;
    let bytes = fs::read(path).usage_context(format!(
        "cannot read active repeatability script {}",
        path.display()
    ))?;
    let script: RepeatabilityScript =
        serde_json::from_slice(&bytes).usage_context("cannot parse active repeatability script")?;
    validate_repeatability_script(&script)?;
    Ok(script)
}

fn validate_repeatability_script(script: &RepeatabilityScript) -> AppResult<()> {
    if script.schema_version != 1
        || script.script_identity != "IsaacCsT0-ActiveInputScript-v1"
        || script.authority_clock != "RELATIVE_AUTHORITY_TICK"
        || script.steps.is_empty()
        || script.steps[0].relative_tick != 0
        || !script.steps[0].keys_down.is_empty()
        || !script
            .steps
            .last()
            .is_some_and(|step| step.keys_down.is_empty())
        || !script
            .steps
            .windows(2)
            .all(|pair| pair[0].relative_tick < pair[1].relative_tick)
    {
        return Err(AppError::evidence(
            "active repeatability script has an invalid identity or tick order",
        ));
    }
    for step in &script.steps {
        let keys = step
            .keys_down
            .iter()
            .map(String::as_str)
            .collect::<BTreeSet<_>>();
        if keys.len() != step.keys_down.len()
            || keys.iter().any(|key| !matches!(*key, "A" | "D" | "RIGHT"))
        {
            return Err(AppError::evidence(
                "active repeatability script contains duplicate or unsupported keys",
            ));
        }
    }
    Ok(())
}

pub trait RepeatabilityInputSink {
    fn set_keys(&mut self, keys_down: &[String]) -> AppResult<()>;
    fn release_all(&mut self) -> AppResult<()>;
}

pub struct RepeatabilityScriptGuard<S: RepeatabilityInputSink> {
    script: RepeatabilityScript,
    sink: S,
    authority_tick_base: u64,
    next_step: usize,
}

impl<S: RepeatabilityInputSink> RepeatabilityScriptGuard<S> {
    pub fn new(script: RepeatabilityScript, sink: S, authority_tick_base: u64) -> Self {
        Self {
            script,
            sink,
            authority_tick_base,
            next_step: 0,
        }
    }

    pub fn advance(&mut self, authority_tick: u64) -> AppResult<()> {
        let relative_tick = authority_tick
            .checked_sub(self.authority_tick_base)
            .ok_or_else(|| {
                AppError::evidence("repeatability authority tick regressed before its base")
            })?;
        while self
            .script
            .steps
            .get(self.next_step)
            .is_some_and(|step| step.relative_tick <= relative_tick)
        {
            let step = &self.script.steps[self.next_step];
            self.sink.set_keys(&step.keys_down)?;
            self.next_step += 1;
        }
        Ok(())
    }

    pub fn complete(&self) -> bool {
        self.next_step == self.script.steps.len()
    }
}

impl<S: RepeatabilityInputSink> Drop for RepeatabilityScriptGuard<S> {
    fn drop(&mut self) {
        let _ = self.sink.release_all();
    }
}

#[cfg(test)]
mod tests {
    use std::cell::RefCell;
    use std::rc::Rc;

    use super::*;
    use crate::active_artifact::ActiveManifest;
    use crate::active_catalog::{FIXED_TARGET_SHA256, expected_hook_slots};
    use crate::active_verification::{
        ACTIVE_VERIFICATION_V2_CHECK_IDS, ActiveVerificationSuiteEvidence,
        generate_active_verification_v2, write_active_verification_v2,
    };

    fn slots() -> Vec<ActiveTargetSlot> {
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

    fn run_config() -> ActiveRunConfig {
        ActiveRunConfig {
            case_id: "post-cas-guard".to_owned(),
            scenario: "keyboard".to_owned(),
            target_sha256: FIXED_TARGET_SHA256.to_owned(),
            observed_target_sha256: crate::target_equivalence::APPROVED_UNPACKED_TARGET_SHA256
                .to_owned(),
            target_equivalence_sha256: "9".repeat(64),
            mode_gate_sha256: "1".repeat(64),
            loop_gate_sha256: "2".repeat(64),
            raw_prereq_sha256: "3".repeat(64),
            active_catalog_sha256: "4".repeat(64),
            active_plan_sha256: "5".repeat(64),
            input_script_sha256: None,
            companion_loop_artifact_path: PathBuf::from("loop-context"),
            artifact_identity: "claimed-active-run".to_owned(),
            arm_token_sha256: "6".repeat(64),
            session_id: [0xA5; 16],
            client_id: 7,
            target_slots_before: slots(),
        }
    }

    fn verification_suites() -> Vec<ActiveVerificationSuiteEvidence> {
        vec![
            ActiveVerificationSuiteEvidence {
                id: "schema4-native-debug".to_owned(),
                configuration: "Debug".to_owned(),
                exit_code: 0,
                case_count: 262,
                binary_sha256: "A".repeat(64),
            },
            ActiveVerificationSuiteEvidence {
                id: "schema4-native-release-ltcg".to_owned(),
                configuration: "Release-LTCG".to_owned(),
                exit_code: 0,
                case_count: 262,
                binary_sha256: "B".repeat(64),
            },
        ]
    }

    #[derive(Default)]
    struct RecordingState {
        states: Vec<Vec<String>>,
        released: bool,
    }

    struct RecordingSink(Rc<RefCell<RecordingState>>);

    impl RepeatabilityInputSink for RecordingSink {
        fn set_keys(&mut self, keys_down: &[String]) -> AppResult<()> {
            self.0.borrow_mut().states.push(keys_down.to_vec());
            Ok(())
        }

        fn release_all(&mut self) -> AppResult<()> {
            let mut state = self.0.borrow_mut();
            state.released = true;
            state.states.push(Vec::new());
            Ok(())
        }
    }

    #[test]
    fn verification_v2_capability_is_required_before_cas_api_is_available() {
        let directory = tempfile::tempdir().unwrap();
        let path = directory.path().join("verification-v2.json");
        let results = ACTIVE_VERIFICATION_V2_CHECK_IDS
            .iter()
            .map(|id| ((*id).to_owned(), true))
            .collect();
        let expected = generate_active_verification_v2(&results, &verification_suites()).unwrap();
        write_active_verification_v2(&path, &results, &verification_suites()).unwrap();
        let verified = verify_active_execution_before_cas(&path).unwrap();
        assert_eq!(verified.verification.status, expected.status);

        let v1 = Path::new(env!("CARGO_MANIFEST_DIR")).join("input-active-verification-v1.json");
        assert!(verify_active_execution_before_cas(&v1).is_err());
    }

    #[test]
    fn claimed_run_drop_persists_create_new_failed_artifact() {
        let directory = tempfile::tempdir().unwrap();
        let output = directory.path().join("active-run");
        let run = ActiveRun::create(&output, run_config()).unwrap();
        let companion = output.join("loop-context");
        fs::create_dir(&companion).unwrap();
        for name in ["manifest.json", "events.jsonl", "analysis.json"] {
            fs::write(companion.join(name), format!("fixture-{name}\n")).unwrap();
        }
        let guard = ClaimedActiveRun {
            claim: ArmClaim {
                schema_version: 1,
                claim_identity: "IsaacCsT0-ActiveArmClaim-v1".to_owned(),
                consumed_at_utc: Utc::now(),
                grant_sha256: "1".repeat(64),
                token_sha256: "6".repeat(64),
                prerequisite_sha256: "2".repeat(64),
                case_id: "post-cas-guard".to_owned(),
                scenario: "keyboard".to_owned(),
                process_id: 1,
                process_creation_time_100ns: 2,
                generation: 3,
            },
            claim_path: directory.path().join("claim.json"),
            run: Some(run),
            fallback_summary: ActiveRunFinish::default(),
        };
        drop(guard);
        let manifest: ActiveManifest =
            serde_json::from_slice(&fs::read(output.join("manifest.json")).unwrap()).unwrap();
        assert_eq!(manifest.state, "FAILED");
        assert_eq!(
            manifest.failure_reason.as_deref(),
            Some("active orchestration abandoned after arm token CAS")
        );
        assert!(manifest.arm_token_consumed);
        assert!(manifest.companion_loop_manifest_sha256.is_some());
        assert!(manifest.companion_loop_events_sha256.is_some());
        assert!(manifest.companion_loop_analysis_sha256.is_some());
    }

    #[test]
    fn repeatability_script_guard_applies_relative_steps_and_releases_on_drop() {
        let script: RepeatabilityScript = serde_json::from_slice(include_bytes!(
            "../input-active-repeatability-script-v1.json"
        ))
        .unwrap();
        validate_repeatability_script(&script).unwrap();
        let state = Rc::new(RefCell::new(RecordingState::default()));
        let mut guard =
            RepeatabilityScriptGuard::new(script, RecordingSink(Rc::clone(&state)), 100);
        guard.advance(100).unwrap();
        guard.advance(106).unwrap();
        assert_eq!(guard.next_step, 3);
        assert_eq!(state.borrow().states[1], vec!["D"]);
        guard.advance(122).unwrap();
        assert!(guard.complete());
        drop(guard);
        assert!(state.borrow().released);
        assert_eq!(state.borrow().states.last(), Some(&Vec::new()));
    }
}
