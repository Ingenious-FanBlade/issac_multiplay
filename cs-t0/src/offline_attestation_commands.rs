use std::path::Path;

use crate::error::AppResult;

#[cfg(windows)]
mod live {
    use std::path::Path;
    use std::process::Command;
    use std::thread;
    use std::time::{Duration, Instant};

    use chrono::Utc;

    use crate::active_catalog::{ACTIVE_SCENARIOS, validate_active_plan};
    use crate::active_preflight::{ActivePreflightReport, run_read_only_preflight};
    use crate::artifact::file_sha256;
    use crate::error::{AppError, AppResult, UsageContext};
    use crate::loop_commands::loop_entropy;
    use crate::loop_verification::inspect_loop_equivalent;
    use crate::offline_attestation::{
        OFFLINE_ATTESTATION_MIN_CALLBACKS, OfflineAttestationFinish, OfflineAttestationManifest,
        OfflineAttestationRunConfig, OfflinePrerequisiteDecision, validate_offline_prerequisite,
        write_offline_attestation_run,
    };
    use crate::offline_attestation_ipc::{
        NativeOfflineAttestationRecord, OFFLINE_ATTESTATION_IPC_VERSION,
        OFFLINE_ATTESTATION_MENU_RETURN_RVA, OFFLINE_FLAG_EXACT_RETURN,
        OFFLINE_FLAG_GAME_ROOT_STABLE, OFFLINE_FLAG_LOADING_MENU_INACTIVE,
        OFFLINE_FLAG_LUA_UNBOUND_MENU, OFFLINE_FLAG_MAIN_THREAD, OFFLINE_FLAG_MANAGER_NEGATIVE,
        OFFLINE_FLAG_ORIGINAL_CALLED, OFFLINE_FLAG_ROOT_MATCH, OFFLINE_FLAG_RUN_INACTIVE_AFTER,
        OFFLINE_FLAG_RUN_INACTIVE_BEFORE, OFFLINE_FLAG_STEAM_ARMED,
        OFFLINE_FLAG_WINDOW_MAIN_THREAD, OfflineAttestationEventKind, OfflineAttestationMapping,
        OfflineAttestationMappingIdentity, OfflineAttestationNativeState, OfflineAttestationPolicy,
    };
    use crate::target_equivalence::validate_target_equivalence;
    use crate::win32::{self, ObservedProcess};

    const READY_TIMEOUT: Duration = Duration::from_secs(15);
    const OBSERVATION_TIMEOUT: Duration = Duration::from_secs(60);
    const POLL_INTERVAL: Duration = Duration::from_millis(5);

    struct InjectionGuard<'a> {
        helper: &'a Path,
        native: &'a Path,
        pid: u32,
        armed: bool,
    }

    impl InjectionGuard<'_> {
        fn disarm(&mut self) {
            self.armed = false;
        }
    }

    impl Drop for InjectionGuard<'_> {
        fn drop(&mut self) {
            if self.armed {
                let _ = run_helper(
                    self.helper,
                    "--unload-offline-attestation",
                    self.pid,
                    self.native,
                    None,
                );
            }
        }
    }

    #[allow(clippy::too_many_arguments)]
    pub(super) fn run(
        scenario: &str,
        game_exe: &Path,
        loop_ledger: &Path,
        target_ledger: &Path,
        target_equivalence: &Path,
        mode_gate: &Path,
        loop_gate: &Path,
        raw_prereq: &Path,
        active_catalog: &Path,
        active_plan: &Path,
        expected_variant: &str,
        artifacts_root: &Path,
        case_id: &str,
    ) -> AppResult<OfflinePrerequisiteDecision> {
        validate_case_scenario_root(case_id, scenario, artifacts_root)?;

        // Static bundle validation is deliberately first. A mismatched installed image therefore
        // fails before a mapping is created or the passive DLL is loaded.
        let validated = validate_active_plan(active_plan, mode_gate, loop_gate, raw_prereq)?;
        let equivalence = validate_target_equivalence(
            target_equivalence,
            game_exe,
            loop_ledger,
            target_ledger,
            active_catalog,
            active_plan,
        )?;
        let inspection =
            inspect_loop_equivalent(game_exe, loop_ledger, expected_variant, &equivalence)?;
        if validated.target_sha256 != equivalence.canonical_target_sha256
            || inspection.file_sha256 != equivalence.observed_target_sha256
            || validated.loop_ledger_sha256 != inspection.ledger_sha256
        {
            return Err(AppError::evidence(
                "offline attestation bundle does not bind the selected frozen target and ledger",
            ));
        }

        let before_preflight =
            run_read_only_preflight(game_exe, &equivalence.observed_target_sha256, scenario)?;
        require_observable_preflight(&before_preflight)?;

        let canonical_game = std::fs::canonicalize(game_exe).usage_context(format!(
            "cannot resolve game executable {}",
            game_exe.display()
        ))?;
        let process = open_clean_exact_process(&canonical_game)?;
        if process.pid != before_preflight.process_id
            || process.creation_time_100ns != before_preflight.process_creation_time_100ns
        {
            return Err(AppError::evidence(
                "target identity changed after the read-only preflight",
            ));
        }

        let tool = std::env::current_exe().usage_context("cannot locate T0 controller")?;
        let distribution = tool
            .parent()
            .ok_or_else(|| AppError::usage("controller has no parent directory"))?;
        let native = distribution.join("IsaacCsT0.Native.dll");
        let helper = distribution.join("IsaacCsT0.Helper.exe");
        ensure_file(&native, "T0-04 offline attestation Native DLL")?;
        ensure_file(&helper, "T0-04 offline attestation helper")?;

        let nonce = loop_entropy(process.pid, scenario_ordinal(scenario)?)?;
        let policy = OfflineAttestationPolicy::for_scenario(scenario);
        let expected_window_thread_id = if policy == OfflineAttestationPolicy::LoadingMenuInactive {
            win32::unique_visible_root_window_thread(process.pid)?
        } else {
            0
        };
        let mapping_name = format!(
            "Local\\IsaacCsT0.OfflineAttestation.v{}.{}.{}",
            OFFLINE_ATTESTATION_IPC_VERSION,
            process.pid,
            &hex::encode_upper(nonce)[..16]
        );
        let mapping = OfflineAttestationMapping::create(
            mapping_name,
            OfflineAttestationMappingIdentity {
                target_pid: process.pid,
                process_creation_time_100ns: process.creation_time_100ns,
                policy,
                expected_window_thread_id,
                nonce,
                canonical_target_sha256: decode_hash(&validated.target_sha256)?,
                observed_target_sha256: decode_hash(&equivalence.observed_target_sha256)?,
                target_equivalence_sha256: decode_hash(&equivalence.document_sha256)?,
            },
        )?;

        run_helper(
            &helper,
            "--inject-offline-attestation",
            process.pid,
            &native,
            Some(mapping.name()),
        )?;
        let mut guard = InjectionGuard {
            helper: &helper,
            native: &native,
            pid: process.pid,
            armed: true,
        };
        let mut next = 0_u64;
        let mut records = Vec::new();
        let capture_result = capture_window(&process, &mapping, &mut next, &mut records, policy);

        // Cleanup is a single explicit attempt. Retrying a failed restoration could overwrite a
        // third-party slot owner and is therefore forbidden.
        guard.disarm();
        let unload_result = run_helper(
            &helper,
            "--unload-offline-attestation",
            process.pid,
            &native,
            None,
        );
        capture_result?;
        unload_result?;
        wait_stopped(&mapping, &mut next, &mut records, policy)?;

        let refreshed = ObservedProcess::open_exact(process.pid, &canonical_game)?;
        if refreshed.creation_time_100ns != process.creation_time_100ns {
            return Err(AppError::evidence(
                "target PID was reused during offline attestation",
            ));
        }
        let after_preflight =
            run_read_only_preflight(game_exe, &equivalence.observed_target_sha256, scenario)?;
        require_observable_preflight(&after_preflight)?;
        let target_identity_stable = before_preflight.process_id == after_preflight.process_id
            && before_preflight.process_creation_time_100ns
                == after_preflight.process_creation_time_100ns
            && before_preflight.target_sha256 == after_preflight.target_sha256
            && before_preflight.slot_snapshot_sha256 == after_preflight.slot_snapshot_sha256;
        if !target_identity_stable {
            return Err(AppError::evidence(
                "read-only preflight identity changed across offline attestation",
            ));
        }

        let (slot_before, slot_after) = mapping.slot_snapshot_hashes();
        let output_directory = artifacts_root.join(format!(
            "{}-t0-04-offline-attestation-{}-{}",
            Utc::now().format("%Y%m%dT%H%M%SZ"),
            scenario,
            case_id
        ));
        write_offline_attestation_run(
            &output_directory,
            OfflineAttestationRunConfig {
                case_id: case_id.to_owned(),
                scenario: scenario.to_owned(),
                target_sha256: validated.target_sha256,
                observed_target_sha256: equivalence.observed_target_sha256,
                target_equivalence_sha256: equivalence.document_sha256,
                mode_gate_sha256: validated.mode_gate_sha256,
                loop_gate_sha256: validated.loop_gate_sha256,
                raw_prereq_sha256: validated.raw_prereq_sha256,
                active_plan_sha256: validated.plan_sha256,
                process_id: process.pid,
                process_creation_time_100ns: process.creation_time_100ns,
                native_sha256: file_sha256(&native)?,
                helper_sha256: file_sha256(&helper)?,
                identity_salt: nonce,
            },
            &records,
            OfflineAttestationFinish {
                main_thread_id: mapping.main_thread_id(),
                generation: mapping.generation(),
                simulation_callbacks: mapping.simulation_callbacks(),
                menu_heartbeats: mapping.menu_heartbeats(),
                steam_observer_armed: mapping.steam_observer_armed(),
                teardown_observed: mapping.teardown_observed(),
                signal_counts: mapping.signal_counts(),
                installed_hooks_mask: mapping.installed_hooks_mask(),
                restored_hooks_mask: mapping.restored_hooks_mask(),
                restored_slot_count: mapping.restored_slot_count(),
                overwritten_events: mapping.overwritten_events(),
                producer_closed: mapping.producer_closed(),
                target_identity_stable,
                slot_snapshot_before_sha256: slot_before,
                slot_snapshot_after_sha256: slot_after,
            },
        )
    }

    pub(super) fn recheck(
        game_exe: &Path,
        expected_target_sha256: &str,
        scenario: &str,
        prerequisite_path: &Path,
    ) -> AppResult<()> {
        let prerequisite = validate_offline_prerequisite(prerequisite_path, Utc::now())?;
        if prerequisite.target_sha256 != expected_target_sha256 || prerequisite.scenario != scenario
        {
            return Err(AppError::evidence(
                "offline continuity prerequisite does not bind the selected target and scenario",
            ));
        }
        let source_parent = prerequisite_path.parent().unwrap_or_else(|| Path::new("."));
        let source_manifest: OfflineAttestationManifest = serde_json::from_slice(
            &std::fs::read(source_parent.join("manifest.json"))
                .usage_context("cannot read offline attestation source manifest")?,
        )
        .usage_context("cannot parse offline attestation source manifest")?;

        let canonical_game = std::fs::canonicalize(game_exe).usage_context(format!(
            "cannot resolve game executable {}",
            game_exe.display()
        ))?;
        let process = open_clean_exact_process(&canonical_game)?;
        if process.pid != prerequisite.process_id
            || process.creation_time_100ns != prerequisite.process_creation_time_100ns
        {
            return Err(AppError::evidence(
                "offline continuity target PID or creation time changed",
            ));
        }
        let tool = std::env::current_exe().usage_context("cannot locate T0 controller")?;
        let distribution = tool
            .parent()
            .ok_or_else(|| AppError::usage("controller has no parent directory"))?;
        let native = distribution.join("IsaacCsT0.Native.dll");
        let helper = distribution.join("IsaacCsT0.Helper.exe");
        ensure_file(&native, "T0-04 offline attestation Native DLL")?;
        ensure_file(&helper, "T0-04 offline attestation helper")?;
        if file_sha256(&native)? != source_manifest.native_sha256
            || file_sha256(&helper)? != source_manifest.helper_sha256
        {
            return Err(AppError::evidence(
                "offline continuity observer build differs from the prerequisite source",
            ));
        }

        let salt = decode_hash(&prerequisite.identity_salt)?;
        let policy = OfflineAttestationPolicy::for_scenario(scenario);
        let expected_window_thread_id = if policy == OfflineAttestationPolicy::LoadingMenuInactive {
            let current = win32::unique_visible_root_window_thread(process.pid)?;
            if current != prerequisite.expected_window_thread_id {
                return Err(AppError::evidence(
                    "offline continuity visible root window thread changed",
                ));
            }
            current
        } else {
            0
        };
        let mapping_name = format!(
            "Local\\IsaacCsT0.OfflineAttestation.v{}.{}.{}",
            OFFLINE_ATTESTATION_IPC_VERSION,
            process.pid,
            &prerequisite.identity_salt[..16]
        );
        let mapping = OfflineAttestationMapping::create(
            mapping_name,
            OfflineAttestationMappingIdentity {
                target_pid: process.pid,
                process_creation_time_100ns: process.creation_time_100ns,
                policy,
                expected_window_thread_id,
                nonce: salt,
                canonical_target_sha256: decode_hash(expected_target_sha256)?,
                observed_target_sha256: decode_hash(&prerequisite.observed_target_sha256)?,
                target_equivalence_sha256: decode_hash(&prerequisite.target_equivalence_sha256)?,
            },
        )?;
        run_helper(
            &helper,
            "--inject-offline-attestation",
            process.pid,
            &native,
            Some(mapping.name()),
        )?;
        let mut guard = InjectionGuard {
            helper: &helper,
            native: &native,
            pid: process.pid,
            armed: true,
        };
        let mut next = 0_u64;
        let mut records = Vec::new();
        let continuity_result =
            capture_continuity_window(&process, &mapping, &mut next, &mut records, &prerequisite);
        guard.disarm();
        let unload_result = run_helper(
            &helper,
            "--unload-offline-attestation",
            process.pid,
            &native,
            None,
        );
        continuity_result?;
        unload_result?;
        wait_stopped(&mapping, &mut next, &mut records, policy)?;

        let after =
            run_read_only_preflight(game_exe, &prerequisite.observed_target_sha256, scenario)?;
        require_observable_preflight(&after)?;
        if after.process_id != prerequisite.process_id
            || after.process_creation_time_100ns != prerequisite.process_creation_time_100ns
        {
            return Err(AppError::evidence(
                "offline continuity process identity changed after observer restoration",
            ));
        }
        let (before_slots, after_slots) = mapping.slot_snapshot_hashes();
        if before_slots.iter().all(|byte| *byte == 0) || before_slots != after_slots {
            return Err(AppError::evidence(
                "offline continuity observer did not restore its exact policy slot snapshot",
            ));
        }
        Ok(())
    }

    fn capture_continuity_window(
        process: &ObservedProcess,
        mapping: &OfflineAttestationMapping,
        next: &mut u64,
        records: &mut Vec<NativeOfflineAttestationRecord>,
        prerequisite: &OfflinePrerequisiteDecision,
    ) -> AppResult<()> {
        let policy = OfflineAttestationPolicy::for_scenario(&prerequisite.scenario);
        let deadline = Instant::now() + READY_TIMEOUT;
        loop {
            records.extend(mapping.drain(next)?);
            match mapping.state()? {
                OfflineAttestationNativeState::Ready => break,
                OfflineAttestationNativeState::Failed | OfflineAttestationNativeState::Unsafe => {
                    return Err(native_failure(mapping, "continuity initialization"));
                }
                _ if Instant::now() >= deadline => {
                    return Err(AppError::evidence(
                        "offline continuity observer did not become ready",
                    ));
                }
                _ => thread::sleep(POLL_INTERVAL),
            }
        }
        if mapping.installed_hooks_mask() != policy.expected_hook_mask()
            || !mapping.steam_observer_armed()
        {
            return Err(AppError::evidence(
                "offline continuity observer is not armed across every policy slot",
            ));
        }
        let observation_deadline = Instant::now() + READY_TIMEOUT;
        loop {
            if process.wait(0)? {
                return Err(AppError::evidence(
                    "target exited during offline continuity recheck",
                ));
            }
            records.extend(mapping.drain(next)?);
            if mapping.overwritten_events() != 0
                || mapping.teardown_observed()
                || mapping.signal_counts() != [0; 4]
            {
                return Err(AppError::evidence(
                    "offline continuity recheck observed an unsafe signal",
                ));
            }
            if matches!(
                mapping.state()?,
                OfflineAttestationNativeState::Failed | OfflineAttestationNativeState::Unsafe
            ) {
                return Err(native_failure(mapping, "continuity observation"));
            }
            let clock_kind = if policy == OfflineAttestationPolicy::LoadingMenuInactive {
                OfflineAttestationEventKind::MenuHeartbeatPeek
            } else {
                OfflineAttestationEventKind::SimulationCallback
            };
            let clock_events = records
                .iter()
                .filter(|record| record.event.kind == clock_kind as u32)
                .collect::<Vec<_>>();
            if !clock_events.is_empty() {
                let mut required_flags = OFFLINE_FLAG_MAIN_THREAD
                    | OFFLINE_FLAG_ORIGINAL_CALLED
                    | OFFLINE_FLAG_ROOT_MATCH
                    | OFFLINE_FLAG_STEAM_ARMED;
                if policy == OfflineAttestationPolicy::LoadingMenuInactive {
                    required_flags |= OFFLINE_FLAG_LOADING_MENU_INACTIVE
                        | OFFLINE_FLAG_EXACT_RETURN
                        | OFFLINE_FLAG_WINDOW_MAIN_THREAD
                        | OFFLINE_FLAG_RUN_INACTIVE_BEFORE
                        | OFFLINE_FLAG_RUN_INACTIVE_AFTER
                        | OFFLINE_FLAG_GAME_ROOT_STABLE
                        | OFFLINE_FLAG_MANAGER_NEGATIVE
                        | OFFLINE_FLAG_LUA_UNBOUND_MENU;
                }
                let exact = clock_events.iter().all(|record| {
                    record.event.thread_id == prerequisite.main_thread_id
                        && record.event.generation == prerequisite.generation
                        && record.event.original_calls == 1
                        && record.event.flags & required_flags == required_flags
                        && (record.event.flags & OFFLINE_FLAG_LOADING_MENU_INACTIVE != 0)
                            == (policy == OfflineAttestationPolicy::LoadingMenuInactive)
                        && (policy != OfflineAttestationPolicy::LoadingMenuInactive
                            || record.event.return_rva == OFFLINE_ATTESTATION_MENU_RETURN_RVA)
                        && hex::encode_upper(record.event.game_root_digest)
                            == prerequisite.game_root_identity_sha256
                        && hex::encode_upper(record.event.lua_generation_digest)
                            == prerequisite.lua_generation_identity_sha256
                });
                if exact {
                    return Ok(());
                }
                return Err(AppError::evidence(
                    "offline continuity clock, opaque generation, root, thread, or original-once identity changed",
                ));
            }
            if Instant::now() >= observation_deadline {
                return Err(AppError::evidence(
                    "offline continuity observer did not see its exact policy clock",
                ));
            }
            thread::sleep(POLL_INTERVAL);
        }
    }

    fn capture_window(
        process: &ObservedProcess,
        mapping: &OfflineAttestationMapping,
        next: &mut u64,
        records: &mut Vec<NativeOfflineAttestationRecord>,
        policy: OfflineAttestationPolicy,
    ) -> AppResult<()> {
        let ready_deadline = Instant::now() + READY_TIMEOUT;
        loop {
            records.extend(mapping.drain(next)?);
            match mapping.state()? {
                OfflineAttestationNativeState::Ready => break,
                OfflineAttestationNativeState::Failed | OfflineAttestationNativeState::Unsafe => {
                    return Err(native_failure(mapping, "initialization"));
                }
                _ if Instant::now() >= ready_deadline => {
                    return Err(AppError::evidence(
                        "offline attestation Native did not become ready within 15 seconds",
                    ));
                }
                _ => thread::sleep(POLL_INTERVAL),
            }
        }
        if mapping.installed_hooks_mask() != policy.expected_hook_mask()
            || !mapping.steam_observer_armed()
        {
            return Err(AppError::evidence(
                "offline attestation observer is not armed across every policy slot",
            ));
        }

        let deadline = Instant::now() + OBSERVATION_TIMEOUT;
        loop {
            if process.wait(0)? {
                return Err(AppError::evidence(
                    "target exited before offline attestation restoration",
                ));
            }
            records.extend(mapping.drain(next)?);
            if mapping.overwritten_events() != 0
                || mapping.teardown_observed()
                || mapping.signal_counts() != [0; 4]
            {
                return Err(AppError::evidence(
                    "offline attestation observed overwrite, teardown, or an online signal",
                ));
            }
            if matches!(
                mapping.state()?,
                OfflineAttestationNativeState::Failed | OfflineAttestationNativeState::Unsafe
            ) {
                return Err(native_failure(mapping, "observation"));
            }
            let clock_count = if policy == OfflineAttestationPolicy::LoadingMenuInactive {
                mapping.menu_heartbeats()
            } else {
                mapping.simulation_callbacks()
            };
            if clock_count >= OFFLINE_ATTESTATION_MIN_CALLBACKS {
                return Ok(());
            }
            if Instant::now() >= deadline {
                return Err(AppError::evidence(
                    "offline attestation did not observe 60 continuous exact policy clocks within 60 seconds",
                ));
            }
            thread::sleep(POLL_INTERVAL);
        }
    }

    fn wait_stopped(
        mapping: &OfflineAttestationMapping,
        next: &mut u64,
        records: &mut Vec<NativeOfflineAttestationRecord>,
        policy: OfflineAttestationPolicy,
    ) -> AppResult<()> {
        let deadline = Instant::now() + READY_TIMEOUT;
        loop {
            records.extend(mapping.drain(next)?);
            match mapping.state()? {
                OfflineAttestationNativeState::Stopped => break,
                OfflineAttestationNativeState::Failed | OfflineAttestationNativeState::Unsafe => {
                    return Err(native_failure(mapping, "restoration"));
                }
                _ if Instant::now() >= deadline => {
                    return Err(AppError::evidence(
                        "offline attestation Native did not stop after restoration",
                    ));
                }
                _ => thread::sleep(POLL_INTERVAL),
            }
        }
        records.extend(mapping.drain(next)?);
        if mapping.restored_hooks_mask() != policy.expected_hook_mask()
            || mapping.restored_slot_count() != policy.expected_slot_count()
            || !mapping.producer_closed()
            || mapping.overwritten_events() != 0
            || mapping.teardown_observed()
            || mapping.signal_counts() != [0; 4]
        {
            return Err(AppError::evidence(
                "offline attestation restoration/drain invariant failed",
            ));
        }
        Ok(())
    }

    fn require_observable_preflight(report: &ActivePreflightReport) -> AppResult<()> {
        let non_attestation_pass = report
            .checks
            .iter()
            .filter(|check| check.id != "offline-session-attestation")
            .all(|check| check.passed);
        if report.result != "BLOCKED_BY_UNIMPLEMENTED_OFFLINE_ATTESTATION"
            || !non_attestation_pass
            || report.active_input_allowed
            || report.mapping_created
            || report.native_injected
            || report.artifact_created
            || report.arm_token_consumed
        {
            return Err(AppError::evidence(
                "read-only active preflight did not reach the exact offline-attestation boundary",
            ));
        }
        Ok(())
    }

    fn open_clean_exact_process(canonical_game: &Path) -> AppResult<ObservedProcess> {
        let blockers = win32::running_blockers()?;
        if !blockers.is_empty() {
            return Err(AppError::evidence(format!(
                "blocked instrumentation process present: {}",
                blockers.join(", ")
            )));
        }
        let pids = win32::find_target_pids()?;
        if pids.len() != 1 {
            return Err(AppError::evidence(
                "offline attestation requires exactly one already-running isaac-ng.exe",
            ));
        }
        let process = ObservedProcess::open_exact(pids[0], canonical_game)?;
        if process.debugger_present()? || !process.forbidden_modules().is_empty() {
            return Err(AppError::evidence(
                "offline attestation refuses a debugged or instrumented target",
            ));
        }
        Ok(process)
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
            .usage_context("cannot run offline attestation helper")?;
        let stdout = String::from_utf8_lossy(&output.stdout);
        let stderr = String::from_utf8_lossy(&output.stderr);
        if !output.status.success() || !stdout.trim_start().starts_with("PASS") {
            return Err(AppError::evidence(format!(
                "offline attestation helper {operation} failed (exit={:?}): {}{}",
                output.status.code(),
                stdout.trim(),
                stderr.trim()
            )));
        }
        Ok(())
    }

    fn native_failure(mapping: &OfflineAttestationMapping, phase: &str) -> AppError {
        AppError::evidence(format!(
            "offline attestation Native {phase} failed closed with code {}",
            mapping.error_code()
        ))
    }

    fn validate_case_scenario_root(
        case_id: &str,
        scenario: &str,
        artifacts_root: &Path,
    ) -> AppResult<()> {
        if case_id.is_empty()
            || case_id.len() > 64
            || !case_id
                .bytes()
                .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_'))
        {
            return Err(AppError::usage("invalid offline attestation case id"));
        }
        if !ACTIVE_SCENARIOS.contains(&scenario)
            || matches!(scenario, "control-no-native" | "original-online-disarmed")
        {
            return Err(AppError::usage(
                "offline attestation requires one of the 16 active runtime scenarios",
            ));
        }
        if !artifacts_root.is_dir() {
            return Err(AppError::usage(format!(
                "offline attestation artifact root does not exist: {}",
                artifacts_root.display()
            )));
        }
        Ok(())
    }

    fn scenario_ordinal(scenario: &str) -> AppResult<u32> {
        ACTIVE_SCENARIOS
            .iter()
            .position(|value| *value == scenario)
            .and_then(|index| u32::try_from(index + 1).ok())
            .ok_or_else(|| AppError::usage("unknown offline attestation scenario"))
    }

    fn decode_hash(value: &str) -> AppResult<[u8; 32]> {
        if value.len() != 64 || !value.bytes().all(|byte| byte.is_ascii_hexdigit()) {
            return Err(AppError::evidence("invalid SHA-256 identity"));
        }
        hex::decode(value)
            .map_err(|_| AppError::evidence("invalid hexadecimal SHA-256 identity"))?
            .try_into()
            .map_err(|_| AppError::evidence("SHA-256 identity is not 32 bytes"))
    }

    fn ensure_file(path: &Path, description: &str) -> AppResult<()> {
        if !path.is_file() {
            return Err(AppError::usage(format!(
                "{description} is missing: {}",
                path.display()
            )));
        }
        Ok(())
    }
}

#[allow(clippy::too_many_arguments)]
pub fn input_offline_attest_command(
    scenario: &str,
    game_exe: &Path,
    loop_ledger: &Path,
    target_ledger: &Path,
    target_equivalence: &Path,
    mode_gate: &Path,
    loop_gate: &Path,
    raw_prereq: &Path,
    active_catalog: &Path,
    active_plan: &Path,
    expected_variant: &str,
    artifacts_root: &Path,
    case_id: &str,
) -> AppResult<crate::offline_attestation::OfflinePrerequisiteDecision> {
    #[cfg(windows)]
    {
        live::run(
            scenario,
            game_exe,
            loop_ledger,
            target_ledger,
            target_equivalence,
            mode_gate,
            loop_gate,
            raw_prereq,
            active_catalog,
            active_plan,
            expected_variant,
            artifacts_root,
            case_id,
        )
    }
    #[cfg(not(windows))]
    {
        let _ = (
            scenario,
            game_exe,
            loop_ledger,
            target_ledger,
            target_equivalence,
            mode_gate,
            loop_gate,
            raw_prereq,
            active_catalog,
            active_plan,
            expected_variant,
            artifacts_root,
            case_id,
        );
        Err(crate::error::AppError::usage(
            "input-offline-attest requires Windows",
        ))
    }
}

pub fn recheck_offline_continuity_command(
    game_exe: &Path,
    expected_target_sha256: &str,
    scenario: &str,
    prerequisite_path: &Path,
) -> AppResult<()> {
    #[cfg(windows)]
    {
        live::recheck(
            game_exe,
            expected_target_sha256,
            scenario,
            prerequisite_path,
        )
    }
    #[cfg(not(windows))]
    {
        let _ = (
            game_exe,
            expected_target_sha256,
            scenario,
            prerequisite_path,
        );
        Err(crate::error::AppError::usage(
            "offline continuity recheck requires Windows",
        ))
    }
}
