use std::path::{Path, PathBuf};

use serde::Serialize;

use crate::coop_passive::validate_passive_capture_options;
use crate::error::{AppError, AppResult};

pub struct PassiveCaptureRequest<'a> {
    pub probe: &'a str,
    pub scenario: &'a str,
    pub game_exe: &'a Path,
    pub target_equivalence: &'a Path,
    pub layout_evidence: &'a Path,
    pub catalog: &'a Path,
    pub graph: &'a Path,
    pub plan: &'a Path,
    pub verification: &'a Path,
    pub artifacts_root: &'a Path,
    pub case_id: &'a str,
    pub duration_milliseconds: u64,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct PassiveCaptureSummary {
    pub schema_version: u32,
    pub work_package: String,
    pub result: String,
    pub scenario: String,
    pub source_directory: PathBuf,
    pub analysis_result: String,
    pub restored_slot_count: u32,
    pub passive_hook_transaction_allowed_after_run: bool,
    pub active_input_allowed: bool,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum PassiveFailureStage {
    Inject,
    Unsafe,
    Capture,
    Unload,
    WaitStopped,
    ProcessIdentity,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct PassiveFinalizeInput {
    pub inject_failed: bool,
    pub native_unsafe: bool,
    pub capture_failed: bool,
    pub unload_failed: bool,
    pub wait_failed: bool,
    pub process_identity_changed: bool,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct PassiveFinalizeDecision {
    pub should_unload: bool,
    pub should_wait_stopped: bool,
    pub should_persist: bool,
    pub unsafe_unload_required: bool,
    pub failure_stage: Option<PassiveFailureStage>,
}

#[must_use]
pub fn passive_finalize_decision(input: PassiveFinalizeInput) -> PassiveFinalizeDecision {
    let failure_stage = if input.inject_failed {
        Some(PassiveFailureStage::Inject)
    } else if input.native_unsafe {
        Some(PassiveFailureStage::Unsafe)
    } else if input.capture_failed {
        Some(PassiveFailureStage::Capture)
    } else if input.unload_failed {
        Some(PassiveFailureStage::Unload)
    } else if input.wait_failed {
        Some(PassiveFailureStage::WaitStopped)
    } else if input.process_identity_changed {
        Some(PassiveFailureStage::ProcessIdentity)
    } else {
        None
    };
    let should_unload = !input.inject_failed && !input.native_unsafe;
    PassiveFinalizeDecision {
        should_unload,
        should_wait_stopped: should_unload && !input.unload_failed,
        should_persist: true,
        unsafe_unload_required: input.native_unsafe || input.unload_failed || input.wait_failed,
        failure_stage,
    }
}

pub fn capture(request: &PassiveCaptureRequest<'_>) -> AppResult<PassiveCaptureSummary> {
    validate_passive_capture_options(
        request.probe,
        request.scenario,
        request.case_id,
        request.duration_milliseconds,
    )?;
    if !request.artifacts_root.is_dir() {
        return Err(AppError::usage(format!(
            "coop passive artifact root does not exist: {}",
            request.artifacts_root.display()
        )));
    }
    #[cfg(windows)]
    {
        live::capture(request)
    }
    #[cfg(not(windows))]
    {
        let _ = request;
        Err(AppError::usage(
            "coop passive capture requires the Windows x86 Native distribution",
        ))
    }
}

#[cfg(windows)]
mod live {
    use std::fs;
    use std::path::Path;
    use std::process::Command;
    use std::thread;
    use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

    use chrono::Utc;
    use serde_json::Value;
    use sha2::{Digest, Sha256};

    use super::*;
    use crate::artifact::{file_sha256, utc_now};
    use crate::coop_passive::{
        PassiveRunConfig, PassiveRunFinish, PassiveTargetSlot, write_passive_run,
    };
    use crate::coop_passive_ipc::{
        PassiveMapping, PassiveMappingIdentity, PassiveNativeState, PassiveScenario,
    };
    use crate::error::UsageContext;
    use crate::loop_ipc::{LOOP_HOOK_MASK, LoopMapping, LoopNativeState};
    use crate::win32::{self, ObservedProcess};

    const READY_TIMEOUT: Duration = Duration::from_secs(20);
    const STOP_TIMEOUT: Duration = Duration::from_secs(20);
    const POLL_INTERVAL: Duration = Duration::from_millis(5);

    #[derive(Clone)]
    struct FrozenBundle {
        target_sha256: String,
        observed_target_sha256: String,
        target_equivalence_sha256: String,
        layout_evidence_sha256: String,
        catalog_sha256: String,
        graph_sha256: String,
        plan_sha256: String,
        verification_sha256: String,
        controller_sha256: String,
        native_sha256: String,
        helper_sha256: String,
        target_slots: Vec<PassiveTargetSlot>,
    }

    pub(super) fn capture(request: &PassiveCaptureRequest<'_>) -> AppResult<PassiveCaptureSummary> {
        // Validate the complete immutable bundle before creating either mapping or loading Native.
        let bundle = validate_bundle(request)?;
        let started_utc = utc_now();
        let source_directory = request.artifacts_root.join(format!(
            "{}-t0-05-03-{}-{}",
            Utc::now().format("%Y%m%dT%H%M%SZ"),
            request.scenario,
            request.case_id
        ));
        if source_directory.exists() {
            return Err(AppError::usage(format!(
                "coop passive source already exists: {}",
                source_directory.display()
            )));
        }
        let canonical_game = fs::canonicalize(request.game_exe).usage_context(format!(
            "cannot resolve game executable {}",
            request.game_exe.display()
        ))?;
        if file_sha256(&canonical_game)? != bundle.observed_target_sha256 {
            return Err(AppError::evidence(
                "coop passive target hash differs from the verified observed target",
            ));
        }
        let blockers = win32::running_blockers()?;
        if !blockers.is_empty() {
            return Err(AppError::evidence(format!(
                "coop passive preflight found blocked processes: {}",
                blockers.join(", ")
            )));
        }
        let pids = win32::find_target_pids()?;
        if pids.len() != 1 {
            return Err(AppError::evidence(
                "coop passive requires exactly one already-running isaac-ng.exe",
            ));
        }
        let process = ObservedProcess::open_exact(pids[0], &canonical_game)?;
        if process.debugger_present()? || !process.forbidden_modules().is_empty() {
            return Err(AppError::evidence(
                "coop passive refuses a debugged or instrumented target",
            ));
        }
        let tool = std::env::current_exe().usage_context("cannot locate T0 controller")?;
        let distribution = tool
            .parent()
            .ok_or_else(|| AppError::usage("controller has no parent directory"))?;
        let native = distribution.join("IsaacCsT0.Native.dll");
        let helper = distribution.join("IsaacCsT0.Helper.exe");
        ensure_file(&native, "coop passive Native DLL")?;
        ensure_file(&helper, "coop passive helper")?;
        if file_sha256(&tool)? != bundle.controller_sha256
            || file_sha256(&native)? != bundle.native_sha256
            || file_sha256(&helper)? != bundle.helper_sha256
        {
            return Err(AppError::evidence(
                "coop passive production binaries differ from verification v5r6",
            ));
        }

        let scenario = scenario_code(request.scenario)?;
        let nonce = capture_nonce(process.pid, scenario as u32)?;
        let nonce_label = &hex::encode_upper(nonce)[..16];
        let loop_name = format!(
            "Local\\IsaacCsT0.CoopPassiveLoop.v5.{}.{}",
            process.pid, nonce_label
        );
        let passive_name = format!(
            "Local\\IsaacCsT0.CoopPassive.v{}.{}.{}",
            scenario_code(request.scenario)?.ipc_version(),
            process.pid,
            nonce_label
        );
        let loop_mapping =
            LoopMapping::create(loop_name.clone(), process.pid, scenario as u32, nonce)?;
        let passive_mapping = PassiveMapping::create(
            passive_name,
            PassiveMappingIdentity {
                target_pid: process.pid,
                scenario,
                process_creation_time_100ns: process.creation_time_100ns,
                nonce,
                canonical_target_sha256: decode_hash(&bundle.target_sha256)?,
                observed_target_sha256: decode_hash(&bundle.observed_target_sha256)?,
                target_equivalence_sha256: decode_hash(&bundle.target_equivalence_sha256)?,
                catalog_sha256: decode_hash(&bundle.catalog_sha256)?,
                graph_sha256: decode_hash(&bundle.graph_sha256)?,
                plan_sha256: decode_hash(&bundle.plan_sha256)?,
                verification_sha256: decode_hash(&bundle.verification_sha256)?,
                loop_mapping_name: loop_name,
            },
        )?;

        let mut passive_next = 0_u64;
        let mut loop_next = 0_u64;
        let mut records = Vec::new();
        let make_config = |main_thread_id: u32| PassiveRunConfig {
            case_id: request.case_id.to_owned(),
            scenario: request.scenario.to_owned(),
            started_utc: started_utc.clone(),
            target_sha256: bundle.target_sha256.clone(),
            observed_target_sha256: bundle.observed_target_sha256.clone(),
            target_equivalence_sha256: bundle.target_equivalence_sha256.clone(),
            layout_evidence_sha256: bundle.layout_evidence_sha256.clone(),
            coop_catalog_sha256: bundle.catalog_sha256.clone(),
            coop_graph_sha256: bundle.graph_sha256.clone(),
            coop_plan_sha256: bundle.plan_sha256.clone(),
            coop_verification_sha256: bundle.verification_sha256.clone(),
            native_sha256: bundle.native_sha256.clone(),
            helper_sha256: bundle.helper_sha256.clone(),
            process_identity_token: salted_token(
                &nonce,
                &process.creation_time_100ns.to_le_bytes(),
                b"process",
            ),
            main_thread_id_token: salted_token(&nonce, &main_thread_id.to_le_bytes(), b"thread"),
            target_slots: bundle.target_slots.clone(),
        };

        let inject_result = run_helper(
            &helper,
            "--inject-coop-passive",
            process.pid,
            &native,
            Some(passive_mapping.name()),
        );
        if inject_result.is_err() {
            if let Ok(drained) = passive_mapping.drain(&mut passive_next) {
                records.extend(drained);
            }
            let _ = loop_mapping.drain(&mut loop_next);
            let decision = passive_finalize_decision(PassiveFinalizeInput {
                inject_failed: true,
                native_unsafe: mapping_is_unsafe(&passive_mapping, &loop_mapping),
                capture_failed: false,
                unload_failed: false,
                wait_failed: false,
                process_identity_changed: false,
            });
            return persist_capture(
                &source_directory,
                request,
                make_config(passive_mapping.main_thread_id()),
                &records,
                finish_from_mappings(
                    &passive_mapping,
                    &loop_mapping,
                    decision,
                    passive_mapping.error_code(),
                ),
            );
        }
        let capture_result = capture_window(
            &process,
            &passive_mapping,
            &loop_mapping,
            &mut passive_next,
            &mut loop_next,
            &mut records,
            request.duration_milliseconds,
        );
        let native_error_code = passive_mapping.error_code();
        let native_unsafe = mapping_is_unsafe(&passive_mapping, &loop_mapping);
        let before_unload = passive_finalize_decision(PassiveFinalizeInput {
            inject_failed: false,
            native_unsafe,
            capture_failed: capture_result.is_err(),
            unload_failed: false,
            wait_failed: false,
            process_identity_changed: false,
        });
        let unload_failed = before_unload.should_unload
            && run_helper(&helper, "--unload-coop-passive", process.pid, &native, None).is_err();
        let before_wait = passive_finalize_decision(PassiveFinalizeInput {
            inject_failed: false,
            native_unsafe,
            capture_failed: capture_result.is_err(),
            unload_failed,
            wait_failed: false,
            process_identity_changed: false,
        });
        let wait_failed = before_wait.should_wait_stopped
            && wait_stopped(
                &passive_mapping,
                &loop_mapping,
                &mut passive_next,
                &mut loop_next,
                &mut records,
            )
            .is_err();
        if let Ok(drained) = passive_mapping.drain(&mut passive_next) {
            records.extend(drained);
        }
        let _ = loop_mapping.drain(&mut loop_next);
        let process_identity_changed =
            match ObservedProcess::open_exact(process.pid, &canonical_game) {
                Ok(refreshed) => refreshed.creation_time_100ns != process.creation_time_100ns,
                Err(_) => true,
            };
        let decision = passive_finalize_decision(PassiveFinalizeInput {
            inject_failed: false,
            native_unsafe,
            capture_failed: capture_result.is_err(),
            unload_failed,
            wait_failed,
            process_identity_changed,
        });
        persist_capture(
            &source_directory,
            request,
            make_config(passive_mapping.main_thread_id()),
            &records,
            finish_from_mappings(&passive_mapping, &loop_mapping, decision, native_error_code),
        )
    }

    fn mapping_is_unsafe(passive: &PassiveMapping, loop_mapping: &LoopMapping) -> bool {
        matches!(passive.state(), Ok(PassiveNativeState::Unsafe) | Err(_))
            || matches!(loop_mapping.state(), Ok(LoopNativeState::Unsafe) | Err(_))
    }

    fn finish_from_mappings(
        passive: &PassiveMapping,
        loop_mapping: &LoopMapping,
        decision: PassiveFinalizeDecision,
        native_error_code: i32,
    ) -> PassiveRunFinish {
        let (before, after) = passive.slot_snapshot_hashes();
        let failure_reason = decision.failure_stage.map(|stage| {
            format!(
                "{}_NATIVE_ERROR_{native_error_code}",
                failure_stage_name(stage)
            )
        });
        PassiveRunFinish {
            state: if decision.failure_stage.is_none() {
                "PASS"
            } else if decision.unsafe_unload_required {
                "UNSAFE"
            } else {
                "FAILED"
            }
            .to_owned(),
            failure_reason,
            installed_hooks_mask: passive.installed_hooks_mask(),
            restored_hooks_mask: passive.restored_hooks_mask(),
            restored_slot_count: passive.restored_slot_count(),
            slot_snapshot_before_sha256: hex::encode_upper(before),
            slot_snapshot_after_sha256: hex::encode_upper(after),
            final_owner_generation: passive.owner_generation(),
            overwritten_events: passive.overwritten_events(),
            callbacks_drained: passive.active_hooks() == 0,
            producer_closed: passive.producer_closed() && loop_mapping.producer_closed(),
            unsafe_unload_required: decision.unsafe_unload_required,
            stable_registry_retired: passive.stable_registry_retired(),
            zero_capability_counts: passive.zero_capability_counts(),
        }
    }

    fn failure_stage_name(stage: PassiveFailureStage) -> &'static str {
        match stage {
            PassiveFailureStage::Inject => "INJECT_FAILED",
            PassiveFailureStage::Unsafe => "NATIVE_UNSAFE",
            PassiveFailureStage::Capture => "CAPTURE_FAILED",
            PassiveFailureStage::Unload => "UNLOAD_FAILED",
            PassiveFailureStage::WaitStopped => "WAIT_STOPPED_FAILED",
            PassiveFailureStage::ProcessIdentity => "PROCESS_IDENTITY_CHANGED",
        }
    }

    fn persist_capture(
        source_directory: &Path,
        request: &PassiveCaptureRequest<'_>,
        config: PassiveRunConfig,
        records: &[crate::coop_passive_ipc::NativePassiveRecord],
        finish: PassiveRunFinish,
    ) -> AppResult<PassiveCaptureSummary> {
        let restored_slot_count = finish.restored_slot_count;
        let analysis = write_passive_run(source_directory, config, records, finish)?;
        let source_directory_label = source_directory
            .file_name()
            .map(PathBuf::from)
            .ok_or_else(|| AppError::evidence("coop passive source directory has no file name"))?;
        Ok(PassiveCaptureSummary {
            schema_version: 1,
            work_package: "CS-T0-05-03".to_owned(),
            result: if analysis.result == "PASS" {
                "CAPTURE_COMPLETE"
            } else {
                "BLOCKED_BY_EVIDENCE"
            }
            .to_owned(),
            scenario: request.scenario.to_owned(),
            source_directory: source_directory_label,
            analysis_result: analysis.result,
            restored_slot_count,
            passive_hook_transaction_allowed_after_run: false,
            active_input_allowed: false,
        })
    }

    fn capture_window(
        process: &ObservedProcess,
        passive: &PassiveMapping,
        loop_mapping: &LoopMapping,
        passive_next: &mut u64,
        loop_next: &mut u64,
        records: &mut Vec<crate::coop_passive_ipc::NativePassiveRecord>,
        duration_milliseconds: u64,
    ) -> AppResult<()> {
        let ready_deadline = Instant::now() + READY_TIMEOUT;
        loop {
            records.extend(passive.drain(passive_next)?);
            let _ = loop_mapping.drain(loop_next)?;
            match (passive.state()?, loop_mapping.state()?) {
                (PassiveNativeState::Ready, LoopNativeState::Ready)
                    if passive.installed_hooks_mask() == passive.expected_hooks_mask()
                        && loop_mapping.hooks_mask() == LOOP_HOOK_MASK =>
                {
                    break;
                }
                (PassiveNativeState::Failed | PassiveNativeState::Unsafe, _)
                | (_, LoopNativeState::Failed | LoopNativeState::Unsafe) => {
                    return Err(native_failure(passive, "initialization"));
                }
                _ if Instant::now() >= ready_deadline => {
                    return Err(AppError::evidence(
                        "coop passive Native did not become ready within 20 seconds",
                    ));
                }
                _ => thread::sleep(POLL_INTERVAL),
            }
        }
        let end = Instant::now() + Duration::from_millis(duration_milliseconds);
        while Instant::now() < end {
            if process.wait(0)? {
                return Err(AppError::evidence(
                    "target exited before the passive scenario window completed",
                ));
            }
            records.extend(passive.drain(passive_next)?);
            let _ = loop_mapping.drain(loop_next)?;
            match passive.state()? {
                PassiveNativeState::Ready => thread::sleep(POLL_INTERVAL),
                PassiveNativeState::Failed | PassiveNativeState::Unsafe => {
                    return Err(native_failure(passive, "observation"));
                }
                _ => {
                    return Err(AppError::evidence(
                        "coop passive Native left Ready before explicit shutdown",
                    ));
                }
            }
        }
        Ok(())
    }

    fn wait_stopped(
        passive: &PassiveMapping,
        loop_mapping: &LoopMapping,
        passive_next: &mut u64,
        loop_next: &mut u64,
        records: &mut Vec<crate::coop_passive_ipc::NativePassiveRecord>,
    ) -> AppResult<()> {
        let deadline = Instant::now() + STOP_TIMEOUT;
        loop {
            records.extend(passive.drain(passive_next)?);
            let _ = loop_mapping.drain(loop_next)?;
            match (passive.state()?, loop_mapping.state()?) {
                (PassiveNativeState::Stopped, LoopNativeState::Stopped) => break,
                (PassiveNativeState::Failed | PassiveNativeState::Unsafe, _)
                | (_, LoopNativeState::Failed | LoopNativeState::Unsafe) => {
                    return Err(native_failure(passive, "shutdown"));
                }
                _ if Instant::now() >= deadline => {
                    return Err(AppError::evidence(
                        "coop passive Native did not reach Stopped after the single shutdown attempt",
                    ));
                }
                _ => thread::sleep(POLL_INTERVAL),
            }
        }
        records.extend(passive.drain(passive_next)?);
        let _ = loop_mapping.drain(loop_next)?;
        if passive.active_hooks() != 0
            || !passive.producer_closed()
            || !loop_mapping.producer_closed()
        {
            return Err(AppError::evidence(
                "coop passive shutdown did not drain callbacks and close both producers",
            ));
        }
        Ok(())
    }

    fn validate_bundle(request: &PassiveCaptureRequest<'_>) -> AppResult<FrozenBundle> {
        let equivalence = read_json(request.target_equivalence, "target equivalence")?;
        let layout = read_json(request.layout_evidence, "coop layout evidence")?;
        let catalog = read_json(request.catalog, "coop catalog v4")?;
        let graph = read_json(request.graph, "coop graph")?;
        let plan = read_json(request.plan, "coop plan v5r1")?;
        let verification = read_json(request.verification, "coop verification v5r6")?;
        require_identity(&catalog, "catalogIdentity", "IsaacCsT0-CoopCatalog-v4")?;
        require_identity(&plan, "planIdentity", "IsaacCsT0-CoopPlan-v5r1")?;
        let frozen_catalog: Value = serde_json::from_str(include_str!("../coop-catalog-v4.json"))
            .expect("compiled catalog");
        let frozen_plan: Value =
            serde_json::from_str(include_str!("../coop-plan-v5r1.json")).expect("compiled plan");
        if catalog != frozen_catalog || plan != frozen_plan {
            return Err(AppError::evidence(
                "device v2 catalog/plan differs from the compiled frozen contract",
            ));
        }
        require_identity(
            &verification,
            "verificationIdentity",
            "IsaacCsT0-CoopVerification-v5r6",
        )?;
        validate_exact_permissions(&plan, "coop plan v5")?;
        let target_equivalence_sha256 = file_sha256(request.target_equivalence)?;
        let layout_evidence_sha256 = file_sha256(request.layout_evidence)?;
        let catalog_sha256 = file_sha256(request.catalog)?;
        let graph_sha256 = file_sha256(request.graph)?;
        let plan_sha256 = file_sha256(request.plan)?;
        let bundle_root = request
            .plan
            .parent()
            .ok_or_else(|| AppError::evidence("coop plan v5 has no parent directory"))?;
        let verifier_path = bundle_root.join("verify-coop-passive.ps1");
        let backend_screen_path = bundle_root.join("coop-passive-backend-screen-v1.json");
        let predecessor_path = bundle_root.join("coop-verification-v5r5.json");
        let shutdown_repair_path = bundle_root.join("coop-device-rebind-shutdown-repair-v1.json");
        ensure_file(&verifier_path, "coop passive automated verifier")?;
        ensure_file(&backend_screen_path, "coop passive backend screen")?;
        ensure_file(&predecessor_path, "coop verification v5r5 predecessor")?;
        ensure_file(&shutdown_repair_path, "device shutdown repair contract")?;
        if file_sha256(&shutdown_repair_path)?
            != hex::encode_upper(Sha256::digest(include_bytes!(
                "../coop-device-rebind-shutdown-repair-v1.json"
            )))
        {
            return Err(AppError::evidence(
                "device shutdown repair differs from the compiled frozen contract",
            ));
        }
        let repository_root = bundle_root
            .parent()
            .ok_or_else(|| AppError::evidence("coop bundle has no repository root"))?;
        validate_backend_screen(
            &read_json(&backend_screen_path, "coop passive backend screen")?,
            &layout_evidence_sha256,
            &graph_sha256,
            repository_root,
        )?;
        let (controller_sha256, native_sha256, helper_sha256) = validate_verification_document(
            &verification,
            &target_equivalence_sha256,
            &layout_evidence_sha256,
            &graph_sha256,
            &catalog_sha256,
            &plan_sha256,
            &file_sha256(&backend_screen_path)?,
            &file_sha256(&verifier_path)?,
            &file_sha256(&predecessor_path)?,
        )?;
        let target_sha256 =
            required_hash(&equivalence, &["canonicalTargetSha256", "canonicalSha256"])?;
        let observed_target_sha256 = required_hash(
            &equivalence,
            &["observedTargetSha256", "observedApprovedUnpackedSha256"],
        )?;
        let slots = locate_slots(&catalog)?;
        if slots.len() != 56 {
            return Err(AppError::evidence(
                "coop catalog v4 does not contain the exact 56 ordered slots",
            ));
        }
        let target_slots = slots
            .iter()
            .take(scenario_code(request.scenario)?.slot_count() as usize)
            .enumerate()
            .map(|(index, slot)| parse_slot(index, slot))
            .collect::<AppResult<Vec<_>>>()?;
        let _ = layout;
        let _ = graph;
        Ok(FrozenBundle {
            target_sha256,
            observed_target_sha256,
            target_equivalence_sha256,
            layout_evidence_sha256,
            catalog_sha256,
            graph_sha256,
            plan_sha256,
            verification_sha256: file_sha256(request.verification)?,
            controller_sha256,
            native_sha256,
            helper_sha256,
            target_slots,
        })
    }

    fn validate_backend_screen(
        decision: &Value,
        layout_sha256: &str,
        graph_sha256: &str,
        repository_root: &Path,
    ) -> AppResult<()> {
        const TOP_LEVEL: [&str; 13] = [
            "schemaVersion",
            "decisionIdentity",
            "workPackage",
            "createdUtc",
            "status",
            "createNewOnly",
            "scope",
            "predecessorLayout",
            "liveEvidence",
            "staticEvidence",
            "scopedRule",
            "generationRule",
            "permissions",
        ];
        let object = decision
            .as_object()
            .ok_or_else(|| AppError::evidence("coop passive backend screen is not an object"))?;
        if object.len() != TOP_LEVEL.len()
            || TOP_LEVEL.iter().any(|field| !object.contains_key(*field))
            || decision.get("schemaVersion").and_then(Value::as_u64) != Some(1)
            || decision.get("decisionIdentity").and_then(Value::as_str)
                != Some("IsaacCsT0-CoopPassiveBackendScreen-v1")
            || decision.get("workPackage").and_then(Value::as_str) != Some("CS-T0-05-03")
            || decision.get("status").and_then(Value::as_str) != Some("PASS_SCOPED_REPAIR")
            || decision.get("createNewOnly").and_then(Value::as_bool) != Some(true)
            || decision.get("scope").and_then(Value::as_str)
                != Some("COOP_PASSIVE_NONZERO_BACKEND_LOCAL_ROUTE")
            || decision
                .get("createdUtc")
                .and_then(Value::as_str)
                .is_none_or(|value| !value.ends_with('Z'))
        {
            return Err(AppError::evidence(
                "coop passive backend screen identity is not exact",
            ));
        }
        let predecessor = decision
            .get("predecessorLayout")
            .and_then(Value::as_object)
            .ok_or_else(|| AppError::evidence("backend screen lacks predecessor layout"))?;
        if predecessor.len() != 4
            || predecessor.get("path").and_then(Value::as_str)
                != Some("cs-t0/coop-c0-layout-evidence-v1.json")
            || predecessor.get("identity").and_then(Value::as_str)
                != Some("IsaacCsT0-CoopC0LayoutEvidence-v1")
            || predecessor.get("sha256").and_then(Value::as_str) != Some(layout_sha256)
            || predecessor.get("unchanged").and_then(Value::as_bool) != Some(true)
        {
            return Err(AppError::evidence(
                "backend screen predecessor layout binding is not exact",
            ));
        }
        let static_evidence = decision
            .get("staticEvidence")
            .and_then(Value::as_object)
            .ok_or_else(|| AppError::evidence("backend screen lacks static evidence"))?;
        if static_evidence.len() != 8
            || static_evidence.get("graphPath").and_then(Value::as_str)
                != Some("cs-t0/coop-static-graph-v1.json")
            || static_evidence.get("graphIdentity").and_then(Value::as_str)
                != Some("IsaacCsT0-CoopStaticGraph-v1")
            || static_evidence.get("graphSha256").and_then(Value::as_str) != Some(graph_sha256)
            || static_evidence
                .get("localBodyGuardRva")
                .and_then(Value::as_str)
                != Some("0x005BAC6D")
            || static_evidence
                .get("nonemptyRosterTargetRva")
                .and_then(Value::as_str)
                != Some("0x005BB574")
            || static_evidence
                .get("localBodyRange")
                .and_then(Value::as_str)
                != Some("0x005BAC85..0x005BB56E")
            || static_evidence
                .get("steamBackendVtableRva")
                .and_then(Value::as_str)
                != Some("0x007A2440")
            || static_evidence.get("ordinaryRouteSlots") != Some(&serde_json::json!([46, 49, 51]))
        {
            return Err(AppError::evidence(
                "backend screen static evidence is not exact",
            ));
        }
        let source = decision
            .get("liveEvidence")
            .and_then(Value::as_object)
            .ok_or_else(|| AppError::evidence("backend screen lacks live evidence"))?;
        let source_directory = source
            .get("sourceDirectory")
            .and_then(Value::as_str)
            .ok_or_else(|| AppError::evidence("backend screen lacks source directory"))?;
        let source_root = repository_root.join(source_directory);
        let manifest_sha256 = file_sha256(&source_root.join("manifest.json"))?;
        let events_sha256 = file_sha256(&source_root.join("events.jsonl"))?;
        let analysis_path = source_root.join("analysis.json");
        let analysis_sha256 = file_sha256(&analysis_path)?;
        let analysis = read_json(&analysis_path, "historical live-04 passive analysis")?;
        if source.len() != 9
            || source_directory
                != "artifacts/cs-t0/20260827T204247Z-t0-05-03-native-join-passive-t0-05-03-native-join-passive-live-04"
            || source.get("analysisResult").and_then(Value::as_str) != Some("BLOCKED_BY_EVIDENCE")
            || source.get("managerEndCount").and_then(Value::as_u64) != Some(8177)
            || source.get("managerDiagnosticCode").and_then(Value::as_u64) != Some(3)
            || source.get("ordinaryRouteObserved").and_then(Value::as_bool) != Some(true)
            || source
                .get("oldSourceResultImmutable")
                .and_then(Value::as_bool)
                != Some(true)
            || source.get("manifestSha256").and_then(Value::as_str)
                != Some(manifest_sha256.as_str())
            || source.get("manifestSha256").and_then(Value::as_str)
                != Some("9DBB82E822BCF91A175B8CE836835F44984ABF532BC1C6A64076BD1784B00B97")
            || source.get("eventsSha256").and_then(Value::as_str) != Some(events_sha256.as_str())
            || source.get("eventsSha256").and_then(Value::as_str)
                != Some("22BF8F43D698C6912B53BCB4016D82D7145045E672429194315198A4974A3AF0")
            || source.get("analysisSha256").and_then(Value::as_str)
                != Some(analysis_sha256.as_str())
            || source.get("analysisSha256").and_then(Value::as_str)
                != Some("2E16F4E0DCDB0AE3040D2332DA01097C044EF9E97E3067DA416188D1C02B4F1F")
            || analysis.get("analysisIdentity").and_then(Value::as_str)
                != Some("IsaacCsT0-CoopPassiveAnalysis-v1")
            || analysis.get("scenario").and_then(Value::as_str) != Some("native-join-passive")
            || analysis.get("result").and_then(Value::as_str) != Some("BLOCKED_BY_EVIDENCE")
        {
            return Err(AppError::evidence(
                "backend screen live evidence binding is not exact",
            ));
        }
        let scoped = decision
            .get("scopedRule")
            .and_then(Value::as_object)
            .ok_or_else(|| AppError::evidence("backend screen lacks scoped rule"))?;
        const TRUE_RULES: [&str; 11] = [
            "nullBackendAllowedWithoutRouteWitness",
            "nonzeroBackendRequiresAlignedReadablePointer",
            "nonzeroBackendRequiresExactSteamVtable",
            "onlineManagerRosterBeginMustBeZero",
            "onlineManagerRosterEndMustBeZero",
            "onlineManagerRosterCapacityEndMustBeZero",
            "onlineFrameMustBeZero",
            "ordinaryRouteMustBeInsideOneManagerTransaction",
            "ordinaryRouteMustMatchCurrentGeneration",
            "nextConsecutiveManagerPassMayConfirmSameCandidate",
            "staleGenerationRouteRejected",
        ];
        const FALSE_RULES: [&str; 3] = [
            "nonzeroBackendPresenceAloneIsPositiveOnlineEvidence",
            "newHookAllowed",
            "analyzerRelaxationAllowed",
        ];
        if scoped.len() != 17
            || TRUE_RULES
                .iter()
                .any(|field| scoped.get(*field).and_then(Value::as_bool) != Some(true))
            || FALSE_RULES
                .iter()
                .any(|field| scoped.get(*field).and_then(Value::as_bool) != Some(false))
            || scoped
                .get("possessorOrExitCannotAuthorizeOrdinaryRoute")
                .and_then(Value::as_bool)
                != Some(true)
            || scoped
                .get("unknownBackendOrAnyOnlineSignalKeepsUnbound")
                .and_then(Value::as_bool)
                != Some(true)
            || scoped.get("ordinaryRouteMustBeExact")
                != Some(&serde_json::json!([
                    "ORDINARY_SPAWN_BEGIN",
                    "ORDINARY_SPAWN_END",
                    "ORDINARY_ASSIGNMENT_BEGIN",
                    "ORDINARY_ASSIGNMENT_END",
                    "JOIN_HUD_BEGIN",
                    "JOIN_HUD_END"
                ]))
        {
            return Err(AppError::evidence(
                "backend screen scoped rule is not exact",
            ));
        }
        let generation = decision
            .get("generationRule")
            .and_then(Value::as_object)
            .ok_or_else(|| AppError::evidence("backend screen lacks generation rule"))?;
        const GENERATION_RULES: [&str; 3] = [
            "ordinaryJoinInvalidatesExactlyOnce",
            "ordinaryAssignmentInsideTheSameJoinRouteDoesNotInvalidateAgain",
            "unrelatedAssignmentStillInvalidates",
        ];
        if generation.len() != GENERATION_RULES.len()
            || GENERATION_RULES
                .iter()
                .any(|field| generation.get(*field).and_then(Value::as_bool) != Some(true))
        {
            return Err(AppError::evidence(
                "backend screen generation rule is not exact",
            ));
        }
        let permissions = decision
            .get("permissions")
            .and_then(Value::as_object)
            .ok_or_else(|| AppError::evidence("backend screen lacks permissions"))?;
        const PERMISSIONS: [&str; 13] = [
            "passiveObservationAllowed",
            "activeInputAllowed",
            "inputMutationAllowed",
            "worldMutationAllowed",
            "runtimeMutationAllowed",
            "overrideAllowed",
            "transportAllowed",
            "schedulerAllowed",
            "applyAllowed",
            "modeLatchAllowed",
            "originalOnlinePositiveAllowed",
            "t005Unlocked",
            "csT00504Unlocked",
        ];
        if permissions.len() != PERMISSIONS.len()
            || PERMISSIONS
                .iter()
                .any(|field| permissions.get(*field).and_then(Value::as_bool) != Some(false))
        {
            return Err(AppError::evidence(
                "backend screen permissions are not all false",
            ));
        }
        Ok(())
    }

    fn validate_exact_permissions(document: &Value, description: &str) -> AppResult<()> {
        const PERMISSIONS: [(&str, bool); 14] = [
            ("passiveHookTransactionAllowed", true),
            ("passiveObservationAllowed", false),
            ("activeInputAllowed", false),
            ("inputMutationAllowed", false),
            ("worldMutationAllowed", false),
            ("runtimeMutationAllowed", false),
            ("overrideAllowed", false),
            ("transportAllowed", false),
            ("schedulerAllowed", false),
            ("applyAllowed", false),
            ("modeLatchAllowed", false),
            ("originalOnlinePositiveAllowed", false),
            ("t005Unlocked", false),
            ("csT00504Unlocked", false),
        ];
        let permissions = document
            .get("permissions")
            .and_then(Value::as_object)
            .ok_or_else(|| AppError::evidence(format!("{description} lacks permissions")))?;
        if permissions.len() != PERMISSIONS.len()
            || PERMISSIONS.iter().any(|(name, expected)| {
                permissions.get(*name).and_then(Value::as_bool) != Some(*expected)
            })
        {
            return Err(AppError::evidence(format!(
                "{description} does not grant exactly the passive hook transaction"
            )));
        }
        Ok(())
    }

    #[allow(clippy::too_many_arguments)]
    fn validate_verification_document(
        verification: &Value,
        target_equivalence_sha256: &str,
        layout_evidence_sha256: &str,
        graph_sha256: &str,
        catalog_sha256: &str,
        plan_sha256: &str,
        backend_screen_sha256: &str,
        verifier_sha256: &str,
        predecessor_sha256: &str,
    ) -> AppResult<(String, String, String)> {
        let shutdown_repair = verification
            .get("verificationIdentity")
            .and_then(Value::as_str)
            == Some("IsaacCsT0-CoopVerification-v5r6");
        let revised = shutdown_repair
            || verification
                .get("verificationIdentity")
                .and_then(Value::as_str)
                == Some("IsaacCsT0-CoopVerification-v5r5");
        const TOP_LEVEL_FIELDS: [&str; 21] = [
            "schemaVersion",
            "verificationIdentity",
            "workPackage",
            "createdUtc",
            "status",
            "phase",
            "createNewOnly",
            "supersedes",
            "supersedesSha256",
            "staticContractComplete",
            "runtimeContractComplete",
            "liveEvidenceComplete",
            "completedLiveSourceCount",
            "offlinePassiveFixtureCount",
            "sixSourceGateEligible",
            "liveAuthorizationCreated",
            "nextRequiredAuthorization",
            "artifactBindings",
            "productionBindings",
            "checks",
            "permissions",
        ];
        const CHECKS: [&str; 24] = [
            "frozen-artifact-bindings",
            "rust-test-all-targets-284",
            "rust-fmt-check",
            "rust-check-all-targets",
            "rust-clippy-deny-warnings",
            "rust-release-build",
            "coop-c0-contract-9",
            "passive-contract-mutation-failed-source-10",
            "legacy-native-debug-release-39",
            "active-native-debug-release-262",
            "passive-native-debug-release-407",
            "passive-topology-diagnostic-digest-25",
            "passive-manager-sentinel-original-once-fault-matrix",
            "passive-manager-end-before-binding-transition",
            "passive-approved-local-coop-callsite-graph",
            "passive-backend-screen-scoped-repair",
            "passive-backend-witness-negative-matrix",
            "passive-ordinary-join-single-generation",
            "passive-live04-replay-remains-blocked",
            "passive-cross-language-debug-release",
            "bootstrap-regression",
            "static-and-c0-hash-bound-rust-regression",
            "t0-04-active-build-and-test-regression",
            "six-offline-passive-fixtures",
        ];
        let revised_checks = [
            "frozen-artifact-bindings",
            "rust-test-all-targets",
            "rust-fmt-check",
            "rust-check-all-targets",
            "rust-clippy-deny-warnings",
            "rust-release-build",
            "legacy-native-debug-release-39",
            "active-native-debug-release-262",
            "passive-native-debug-release-407",
            "device-v2-native-debug-release",
            "passive-cross-language-debug-release",
            "device-v2-cross-language-debug-release",
            "device-v2-causal-contract-and-mutations",
            "legacy-device-counterexamples-rejected",
            "historical-live-replay-immutable",
            "bootstrap-regression",
            "static-c0-regression",
            "t0-04-active-regression",
            "six-offline-passive-fixtures-and-gate",
        ];
        let mut expected_checks = if revised {
            revised_checks.to_vec()
        } else {
            CHECKS.to_vec()
        };
        if shutdown_repair {
            expected_checks.extend([
                "device-v2-real-shutdown-native-debug-release",
                "device-v2-real-shutdown-public-cli-debug-release",
                "device-live10-replay-remains-blocked",
            ]);
        }
        let object = verification
            .as_object()
            .ok_or_else(|| AppError::evidence("coop verification v5r4 is not an object"))?;
        if object.len() != TOP_LEVEL_FIELDS.len()
            || TOP_LEVEL_FIELDS
                .iter()
                .any(|field| !object.contains_key(*field))
        {
            return Err(AppError::evidence(
                "coop verification v5r4 has missing or unknown top-level fields",
            ));
        }
        let exact = verification.get("schemaVersion").and_then(Value::as_u64) == Some(5)
            && verification
                .get("verificationIdentity")
                .and_then(Value::as_str)
                == Some(if shutdown_repair {
                    "IsaacCsT0-CoopVerification-v5r6"
                } else if revised {
                    "IsaacCsT0-CoopVerification-v5r5"
                } else {
                    "IsaacCsT0-CoopVerification-v5r4"
                })
            && verification.get("workPackage").and_then(Value::as_str) == Some("CS-T0-05-03")
            && verification.get("status").and_then(Value::as_str) == Some("PASS")
            && verification.get("phase").and_then(Value::as_str)
                == Some(if shutdown_repair {
                    "PRE_LIVE_DEVICE_V2_SHUTDOWN_REPAIR_AUTOMATED_GATE"
                } else if revised {
                    "PRE_LIVE_DEVICE_V2_CAUSAL_AUTOMATED_GATE"
                } else {
                    "PRE_LIVE_BACKEND_SCREEN_REPAIR_AUTOMATED_GATE"
                })
            && verification.get("createNewOnly").and_then(Value::as_bool) == Some(true)
            && verification.get("supersedes").and_then(Value::as_str)
                == Some(if shutdown_repair {
                    "coop-verification-v5r5.json"
                } else if revised {
                    "coop-verification-v5r4.json"
                } else {
                    "coop-verification-v5r3.json"
                })
            && verification.get("supersedesSha256").and_then(Value::as_str)
                == Some(predecessor_sha256)
            && verification
                .get("staticContractComplete")
                .and_then(Value::as_bool)
                == Some(true)
            && verification
                .get("runtimeContractComplete")
                .and_then(Value::as_bool)
                == Some(true)
            && verification
                .get("liveEvidenceComplete")
                .and_then(Value::as_bool)
                == Some(false)
            && verification
                .get("completedLiveSourceCount")
                .and_then(Value::as_u64)
                == Some(if revised { 3 } else { 0 })
            && verification
                .get("offlinePassiveFixtureCount")
                .and_then(Value::as_u64)
                == Some(6)
            && verification
                .get("sixSourceGateEligible")
                .and_then(Value::as_bool)
                == Some(false)
            && verification
                .get("liveAuthorizationCreated")
                .and_then(Value::as_bool)
                == Some(false)
            && verification
                .get("nextRequiredAuthorization")
                .and_then(Value::as_str)
                == Some(if revised {
                    "device-rebind-passive"
                } else {
                    "native-join-passive"
                })
            && verification
                .get("createdUtc")
                .and_then(Value::as_str)
                .is_some_and(|value| value.ends_with('Z'));
        if !exact {
            return Err(AppError::evidence(
                "coop verification v5r4 pre-live backend screen repair fields are not exact",
            ));
        }
        validate_exact_permissions(verification, "coop verification v5r4")?;

        let artifacts = verification
            .get("artifactBindings")
            .and_then(Value::as_array)
            .ok_or_else(|| AppError::evidence("coop verification v5r4 lacks artifact bindings"))?;
        let shutdown_repair_sha256 = hex::encode_upper(Sha256::digest(include_bytes!(
            "../coop-device-rebind-shutdown-repair-v1.json"
        )));
        let mut expected_artifacts = vec![
            (
                "out/cs-t0/target-equivalence-inspect/unpacked-target-equivalence-v1-release-final.json",
                "IsaacCsT0-UnpackedTargetEquivalence-v1",
                target_equivalence_sha256,
            ),
            (
                "cs-t0/coop-c0-layout-evidence-v1.json",
                "IsaacCsT0-CoopC0LayoutEvidence-v1",
                layout_evidence_sha256,
            ),
            (
                "cs-t0/coop-static-graph-v1.json",
                "IsaacCsT0-CoopStaticGraph-v1",
                graph_sha256,
            ),
            (
                if revised {
                    "cs-t0/coop-catalog-v4.json"
                } else {
                    "cs-t0/coop-catalog-v3.json"
                },
                if revised {
                    "IsaacCsT0-CoopCatalog-v4"
                } else {
                    "IsaacCsT0-CoopCatalog-v3"
                },
                catalog_sha256,
            ),
            (
                if revised {
                    "cs-t0/coop-plan-v5r1.json"
                } else {
                    "cs-t0/coop-plan-v5.json"
                },
                if revised {
                    "IsaacCsT0-CoopPlan-v5r1"
                } else {
                    "IsaacCsT0-CoopPlan-v5"
                },
                plan_sha256,
            ),
            (
                "cs-t0/coop-passive-backend-screen-v1.json",
                "IsaacCsT0-CoopPassiveBackendScreen-v1",
                backend_screen_sha256,
            ),
            (
                "cs-t0/verify-coop-passive.ps1",
                if shutdown_repair {
                    "IsaacCsT0-CoopPassiveAutomatedGate-v6"
                } else if revised {
                    "IsaacCsT0-CoopPassiveAutomatedGate-v5"
                } else {
                    "IsaacCsT0-CoopPassiveAutomatedGate-v4"
                },
                verifier_sha256,
            ),
        ];
        if shutdown_repair {
            expected_artifacts.push((
                "cs-t0/coop-device-rebind-shutdown-repair-v1.json",
                "IsaacCsT0-CoopDeviceRebindShutdownRepair-v1",
                shutdown_repair_sha256.as_str(),
            ));
        }
        if artifacts.len() != expected_artifacts.len() {
            return Err(AppError::evidence(
                "coop verification v5r4 artifact binding count is not exact",
            ));
        }
        for (binding, (path, identity, sha256)) in artifacts.iter().zip(expected_artifacts) {
            validate_binding(binding, path, Some(identity), sha256)?;
        }

        let production = verification
            .get("productionBindings")
            .and_then(Value::as_array)
            .ok_or_else(|| {
                AppError::evidence("coop verification v5r4 lacks production bindings")
            })?;
        let production_paths = [
            "out/cs-t0/IsaacCsT0.exe",
            "out/cs-t0/IsaacCsT0.Native.dll",
            "out/cs-t0/IsaacCsT0.Helper.exe",
        ];
        if production.len() != production_paths.len() {
            return Err(AppError::evidence(
                "coop verification v5r4 production binding count is not exact",
            ));
        }
        let mut production_hashes = Vec::with_capacity(production.len());
        for (binding, path) in production.iter().zip(production_paths) {
            production_hashes.push(validate_binding(binding, path, None, "")?);
        }

        let checks = verification
            .get("checks")
            .and_then(Value::as_array)
            .ok_or_else(|| AppError::evidence("coop verification v5r4 lacks checks"))?;
        if checks.len() != expected_checks.len()
            || checks
                .iter()
                .zip(expected_checks.iter().copied())
                .any(|(check, expected)| {
                    check.as_object().is_none_or(|object| object.len() != 2)
                        || check.get("id").and_then(Value::as_str) != Some(expected)
                        || check.get("passed").and_then(Value::as_bool) != Some(true)
                })
        {
            return Err(AppError::evidence(
                "coop verification v5r4 automated checks are not exact",
            ));
        }
        Ok((
            production_hashes[0].clone(),
            production_hashes[1].clone(),
            production_hashes[2].clone(),
        ))
    }

    fn validate_binding(
        binding: &Value,
        expected_path: &str,
        expected_identity: Option<&str>,
        expected_sha256: &str,
    ) -> AppResult<String> {
        let object = binding
            .as_object()
            .ok_or_else(|| AppError::evidence("coop verification binding is not an object"))?;
        let expected_fields = if expected_identity.is_some() { 3 } else { 2 };
        let sha256 = binding
            .get("sha256")
            .and_then(Value::as_str)
            .filter(|value| is_hash(value))
            .ok_or_else(|| AppError::evidence("coop verification binding lacks SHA-256"))?;
        if object.len() != expected_fields
            || binding.get("path").and_then(Value::as_str) != Some(expected_path)
            || expected_identity.is_some_and(|identity| {
                binding.get("identity").and_then(Value::as_str) != Some(identity)
            })
            || (!expected_sha256.is_empty() && sha256 != expected_sha256)
        {
            return Err(AppError::evidence(
                "coop verification v5r4 binding is not exact",
            ));
        }
        Ok(sha256.to_owned())
    }

    fn locate_slots(value: &Value) -> AppResult<&Vec<Value>> {
        [
            "/orderedHookTransaction/slots",
            "/hookTransaction/slots",
            "/orderedSlots",
            "/hookSlots",
        ]
        .iter()
        .find_map(|pointer| value.pointer(pointer).and_then(Value::as_array))
        .ok_or_else(|| AppError::evidence("coop catalog v3 has no ordered slot array"))
    }

    fn parse_slot(index: usize, value: &Value) -> AppResult<PassiveTargetSlot> {
        let ordinal = value
            .get("ordinal")
            .and_then(Value::as_u64)
            .and_then(|value| u32::try_from(value).ok())
            .ok_or_else(|| AppError::evidence("coop catalog slot has no valid ordinal"))?;
        if ordinal != index as u32 {
            return Err(AppError::evidence(
                "coop catalog slots are not in exact ordinal order",
            ));
        }
        let expected = value
            .get("expectedProof")
            .and_then(Value::as_str)
            .or_else(|| value.get("expectedBytes").and_then(Value::as_str))
            .ok_or_else(|| AppError::evidence("coop catalog slot has no expected proof"))?;
        let proof = salted_token(&[ordinal as u8; 32], expected.as_bytes(), b"slot");
        Ok(PassiveTargetSlot {
            ordinal,
            id: required_string(value, "id")?,
            patch_kind: required_string(value, "patchKind")?,
            rva: required_string(value, "rva")?,
            expected_proof: proof.clone(),
            before_proof: proof.clone(),
            after_proof: proof,
        })
    }

    fn scenario_code(value: &str) -> AppResult<PassiveScenario> {
        match value {
            "native-join-passive" => Ok(PassiveScenario::NativeJoin),
            "native-exit-passive" => Ok(PassiveScenario::NativeExit),
            "death-respawn-passive" => Ok(PassiveScenario::DeathRespawn),
            "room-rebind-passive" => Ok(PassiveScenario::RoomRebind),
            "device-rebind-passive" => Ok(PassiveScenario::DeviceRebind),
            "menu-teardown-passive" => Ok(PassiveScenario::MenuTeardown),
            _ => Err(AppError::usage("unknown coop passive scenario")),
        }
    }

    fn read_json(path: &Path, description: &str) -> AppResult<Value> {
        serde_json::from_slice(
            &fs::read(path)
                .usage_context(format!("cannot read {description} {}", path.display()))?,
        )
        .usage_context(format!("cannot parse {description} {}", path.display()))
    }

    fn require_identity(value: &Value, field: &str, expected: &str) -> AppResult<()> {
        if value.get(field).and_then(Value::as_str) == Some(expected) {
            Ok(())
        } else {
            Err(AppError::evidence(format!(
                "frozen artifact requires exact {field}={expected}"
            )))
        }
    }

    fn required_hash(value: &Value, names: &[&str]) -> AppResult<String> {
        fn find(value: &Value, names: &[&str]) -> Option<String> {
            match value {
                Value::Object(map) => {
                    for name in names {
                        if let Some(text) = map.get(*name).and_then(Value::as_str)
                            && is_hash(text)
                        {
                            return Some(text.to_ascii_uppercase());
                        }
                    }
                    map.values().find_map(|child| find(child, names))
                }
                Value::Array(items) => items.iter().find_map(|child| find(child, names)),
                _ => None,
            }
        }
        find(value, names)
            .ok_or_else(|| AppError::evidence("artifact has no exact nonzero SHA-256 binding"))
    }

    fn required_string(value: &Value, field: &str) -> AppResult<String> {
        value
            .get(field)
            .and_then(Value::as_str)
            .filter(|value| !value.is_empty())
            .map(ToOwned::to_owned)
            .ok_or_else(|| AppError::evidence(format!("catalog slot lacks {field}")))
    }

    fn is_hash(value: &str) -> bool {
        value.len() == 64
            && value.bytes().all(|byte| byte.is_ascii_hexdigit())
            && value.bytes().any(|byte| byte != b'0')
    }

    fn decode_hash(value: &str) -> AppResult<[u8; 32]> {
        if !is_hash(value) {
            return Err(AppError::evidence("invalid SHA-256 binding"));
        }
        let decoded = hex::decode(value)
            .map_err(|error| AppError::evidence(format!("invalid SHA-256 binding: {error}")))?;
        decoded
            .try_into()
            .map_err(|_| AppError::evidence("SHA-256 binding is not 32 bytes"))
    }

    fn capture_nonce(pid: u32, scenario: u32) -> AppResult<[u8; 32]> {
        let now = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map_err(|_| AppError::evidence("system clock predates Unix epoch"))?;
        let mut hasher = Sha256::new();
        hasher.update(now.as_nanos().to_le_bytes());
        hasher.update(pid.to_le_bytes());
        hasher.update(scenario.to_le_bytes());
        hasher.update(std::process::id().to_le_bytes());
        Ok(hasher.finalize().into())
    }

    fn salted_token(nonce: &[u8; 32], value: &[u8], domain: &[u8]) -> String {
        let mut hasher = Sha256::new();
        hasher.update(domain);
        hasher.update(nonce);
        hasher.update(value);
        hex::encode_upper(hasher.finalize())
    }

    fn ensure_file(path: &Path, description: &str) -> AppResult<()> {
        if path.is_file() {
            Ok(())
        } else {
            Err(AppError::usage(format!(
                "{description} is missing: {}",
                path.display()
            )))
        }
    }

    fn run_helper(
        helper: &Path,
        operation: &str,
        pid: u32,
        native: &Path,
        mapping: Option<&str>,
    ) -> AppResult<()> {
        let mut command = Command::new(helper);
        command
            .arg(operation)
            .arg(pid.to_string())
            .arg(native.as_os_str());
        if let Some(mapping) = mapping {
            command.arg(mapping);
        }
        let output = command
            .output()
            .usage_context("cannot run coop passive helper")?;
        let stdout = String::from_utf8_lossy(&output.stdout);
        let stderr = String::from_utf8_lossy(&output.stderr);
        if !output.status.success() || !stdout.trim_start().starts_with("PASS") {
            return Err(AppError::evidence(format!(
                "coop passive helper {operation} failed (exit={:?}): {}{}",
                output.status.code(),
                stdout.trim(),
                stderr.trim()
            )));
        }
        Ok(())
    }

    fn native_failure(mapping: &PassiveMapping, phase: &str) -> AppError {
        AppError::evidence(format!(
            "coop passive Native {phase} failed closed with code {}",
            mapping.error_code()
        ))
    }

    #[cfg(test)]
    mod tests {
        use super::*;

        #[test]
        fn backend_screen_v1_is_exact_and_live04_hash_bound() {
            let cs_root = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
            let repository_root = cs_root.parent().expect("repository root");
            let mut decision = read_json(
                &cs_root.join("coop-passive-backend-screen-v1.json"),
                "test backend screen v1",
            )
            .expect("backend screen v1");
            let validate = |value: &Value| {
                validate_backend_screen(
                    value,
                    &file_sha256(&cs_root.join("coop-c0-layout-evidence-v1.json"))
                        .expect("layout hash"),
                    &file_sha256(&cs_root.join("coop-static-graph-v1.json")).expect("graph hash"),
                    repository_root,
                )
            };
            assert!(validate(&decision).is_ok());
            decision["scopedRule"]["nonzeroBackendPresenceAloneIsPositiveOnlineEvidence"] =
                Value::Bool(true);
            assert!(validate(&decision).is_err());
        }

        #[test]
        fn shutdown_verification_v5r6_requires_exact_repair_and_all_new_checks() {
            let cs_root = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
            let repository_root = cs_root.parent().expect("repository root");
            let predecessor = cs_root.join("coop-verification-v5r5.json");
            let mut verification = read_json(&predecessor, "v5r5 predecessor").unwrap();
            verification["verificationIdentity"] = json_value("IsaacCsT0-CoopVerification-v5r6");
            verification["phase"] = json_value("PRE_LIVE_DEVICE_V2_SHUTDOWN_REPAIR_AUTOMATED_GATE");
            verification["supersedes"] = json_value("coop-verification-v5r5.json");
            verification["supersedesSha256"] = json_value(&file_sha256(&predecessor).unwrap());
            verification["artifactBindings"][6]["identity"] =
                json_value("IsaacCsT0-CoopPassiveAutomatedGate-v6");
            verification["artifactBindings"][6]["sha256"] =
                json_value(&file_sha256(&cs_root.join("verify-coop-passive.ps1")).unwrap());
            verification["artifactBindings"].as_array_mut().unwrap().push(serde_json::json!({
                "path": "cs-t0/coop-device-rebind-shutdown-repair-v1.json",
                "identity": "IsaacCsT0-CoopDeviceRebindShutdownRepair-v1",
                "sha256": file_sha256(&cs_root.join("coop-device-rebind-shutdown-repair-v1.json")).unwrap()
            }));
            for id in [
                "device-v2-real-shutdown-native-debug-release",
                "device-v2-real-shutdown-public-cli-debug-release",
                "device-live10-replay-remains-blocked",
            ] {
                verification["checks"]
                    .as_array_mut()
                    .unwrap()
                    .push(serde_json::json!({ "id": id, "passed": true }));
            }
            let validate = |value: &Value| {
                validate_verification_document(
                    value,
                    &file_sha256(&repository_root.join(
                        "out/cs-t0/target-equivalence-inspect/unpacked-target-equivalence-v1-release-final.json",
                    )).unwrap(),
                    &file_sha256(&cs_root.join("coop-c0-layout-evidence-v1.json")).unwrap(),
                    &file_sha256(&cs_root.join("coop-static-graph-v1.json")).unwrap(),
                    &file_sha256(&cs_root.join("coop-catalog-v4.json")).unwrap(),
                    &file_sha256(&cs_root.join("coop-plan-v5r1.json")).unwrap(),
                    &file_sha256(&cs_root.join("coop-passive-backend-screen-v1.json")).unwrap(),
                    &file_sha256(&cs_root.join("verify-coop-passive.ps1")).unwrap(),
                    &file_sha256(&predecessor).unwrap(),
                )
            };
            assert!(validate(&verification).is_ok());
            for field in [
                "liveAuthorizationCreated",
                "sixSourceGateEligible",
                "liveEvidenceComplete",
            ] {
                let mut mutation = verification.clone();
                mutation[field] = Value::Bool(true);
                assert!(validate(&mutation).is_err(), "{field}");
            }
            for index in 19..22 {
                let mut mutation = verification.clone();
                mutation["checks"][index]["passed"] = Value::Bool(false);
                assert!(validate(&mutation).is_err());
                mutation["checks"].as_array_mut().unwrap().remove(index);
                assert!(validate(&mutation).is_err());
            }
            let mut mutation = verification.clone();
            mutation["artifactBindings"][7]["sha256"] = json_value(&"A".repeat(64));
            assert!(validate(&mutation).is_err());
            mutation = verification.clone();
            mutation["artifactBindings"].as_array_mut().unwrap().pop();
            assert!(validate(&mutation).is_err());
            mutation = verification.clone();
            mutation["supersedesSha256"] = json_value(&"B".repeat(64));
            assert!(validate(&mutation).is_err());
            mutation = verification;
            mutation["unknownField"] = Value::Bool(false);
            assert!(validate(&mutation).is_err());
        }

        fn json_value(text: &str) -> Value {
            Value::String(text.to_owned())
        }

        #[test]
        fn verification_v5r4_is_exact_hash_bound_and_not_identity_only() {
            let cs_root = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
            let repository_root = cs_root.parent().expect("repository root");
            let predecessor_path = cs_root.join("coop-verification-v5r3.json");
            let mut verification = read_json(&predecessor_path, "test verification v5r3")
                .expect("verification v5r3 predecessor");
            verification["verificationIdentity"] =
                Value::String("IsaacCsT0-CoopVerification-v5r4".to_owned());
            verification["phase"] =
                Value::String("PRE_LIVE_BACKEND_SCREEN_REPAIR_AUTOMATED_GATE".to_owned());
            verification["supersedes"] = Value::String("coop-verification-v5r3.json".to_owned());
            verification["supersedesSha256"] =
                Value::String(file_sha256(&predecessor_path).expect("predecessor hash"));
            verification["artifactBindings"]
                .as_array_mut()
                .expect("artifact binding array")
                .insert(
                    5,
                    serde_json::json!({
                        "path": "cs-t0/coop-passive-backend-screen-v1.json",
                        "identity": "IsaacCsT0-CoopPassiveBackendScreen-v1",
                        "sha256": file_sha256(&cs_root.join("coop-passive-backend-screen-v1.json"))
                            .expect("backend screen hash")
                    }),
                );
            verification["artifactBindings"][6]["identity"] =
                Value::String("IsaacCsT0-CoopPassiveAutomatedGate-v4".to_owned());
            verification["artifactBindings"][6]["sha256"] = Value::String(
                file_sha256(&cs_root.join("verify-coop-passive.ps1")).expect("verifier hash"),
            );
            verification["productionBindings"][0]["sha256"] = Value::String(
                file_sha256(&repository_root.join("out/cs-t0/IsaacCsT0.exe"))
                    .expect("controller hash"),
            );
            verification["productionBindings"][1]["sha256"] = Value::String(
                file_sha256(&repository_root.join("out/cs-t0/IsaacCsT0.Native.dll"))
                    .expect("native hash"),
            );
            verification["productionBindings"][2]["sha256"] = Value::String(
                file_sha256(&repository_root.join("out/cs-t0/IsaacCsT0.Helper.exe"))
                    .expect("helper hash"),
            );
            verification["checks"][7]["id"] =
                Value::String("passive-contract-mutation-failed-source-10".to_owned());
            verification["checks"][1]["id"] = Value::String("rust-test-all-targets-284".to_owned());
            verification["checks"][10]["id"] =
                Value::String("passive-native-debug-release-407".to_owned());
            let checks = verification["checks"]
                .as_array_mut()
                .expect("verification checks array");
            for (offset, id) in [
                "passive-backend-screen-scoped-repair",
                "passive-backend-witness-negative-matrix",
                "passive-ordinary-join-single-generation",
                "passive-live04-replay-remains-blocked",
            ]
            .into_iter()
            .enumerate()
            {
                checks.insert(15 + offset, serde_json::json!({ "id": id, "passed": true }));
            }
            let validate = |value: &Value| {
                validate_verification_document(
                    value,
                    &file_sha256(&repository_root.join(
                        "out/cs-t0/target-equivalence-inspect/unpacked-target-equivalence-v1-release-final.json",
                    ))
                    .expect("target equivalence hash"),
                    &file_sha256(&cs_root.join("coop-c0-layout-evidence-v1.json"))
                        .expect("layout hash"),
                    &file_sha256(&cs_root.join("coop-static-graph-v1.json"))
                        .expect("graph hash"),
                    &file_sha256(&cs_root.join("coop-catalog-v3.json")).expect("catalog hash"),
                    &file_sha256(&cs_root.join("coop-plan-v5.json")).expect("plan hash"),
                    &file_sha256(&cs_root.join("coop-passive-backend-screen-v1.json"))
                        .expect("backend screen hash"),
                    &file_sha256(&cs_root.join("verify-coop-passive.ps1"))
                        .expect("verifier hash"),
                    &file_sha256(&cs_root.join("coop-verification-v5r3.json"))
                        .expect("predecessor hash"),
                )
            };
            assert!(validate(&verification).is_ok());
            assert!(
                validate(&serde_json::json!({
                    "verificationIdentity": "IsaacCsT0-CoopVerification-v5r4",
                    "status": "PASS"
                }))
                .is_err()
            );

            verification["liveAuthorizationCreated"] = Value::Bool(true);
            assert!(validate(&verification).is_err());
            verification["liveAuthorizationCreated"] = Value::Bool(false);
            verification["artifactBindings"][4]["sha256"] = Value::String("A".repeat(64));
            assert!(validate(&verification).is_err());
        }
    }
}
