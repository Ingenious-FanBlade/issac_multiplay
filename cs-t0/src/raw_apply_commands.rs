use std::path::Path;

use crate::artifact::file_sha256;
use crate::error::{AppError, AppResult, UsageContext};
use crate::raw_apply_artifact::{
    RAW_APPLY_CAPTURE_PLAN_SHA256, RAW_APPLY_CATALOG_SHA256, raw_apply_observed_sources_valid,
};

fn final_observed_sources_valid(scenario: &str, observed: u64) -> bool {
    raw_apply_observed_sources_valid(scenario, observed)
}

pub const RAW_APPLY_TARGET_SHA256: &str =
    "C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30";
const FROZEN_CATALOG: &[u8] = include_bytes!("../raw-apply-catalog-v4.json");
const FROZEN_CAPTURE_PLAN: &[u8] = include_bytes!("../raw-apply-capture-plan-v4.json");

pub fn validate_raw_apply_documents(catalog: &Path, capture_plan: &Path) -> AppResult<()> {
    let catalog_bytes = std::fs::read(catalog).usage_context(format!(
        "cannot read raw/apply catalog {}",
        catalog.display()
    ))?;
    let plan_bytes = std::fs::read(capture_plan).usage_context(format!(
        "cannot read raw/apply capture plan {}",
        capture_plan.display()
    ))?;
    if file_sha256(catalog)? != RAW_APPLY_CATALOG_SHA256
        || file_sha256(capture_plan)? != RAW_APPLY_CAPTURE_PLAN_SHA256
    {
        return Err(AppError::evidence(
            "raw/apply catalog or capture plan SHA-256 differs from the frozen documents",
        ));
    }
    let supplied_catalog: serde_json::Value =
        serde_json::from_slice(&catalog_bytes).usage_context("cannot parse raw/apply catalog")?;
    let frozen_catalog: serde_json::Value = serde_json::from_slice(FROZEN_CATALOG)
        .usage_context("cannot parse embedded raw/apply catalog")?;
    let supplied_plan: serde_json::Value =
        serde_json::from_slice(&plan_bytes).usage_context("cannot parse raw/apply capture plan")?;
    let frozen_plan: serde_json::Value = serde_json::from_slice(FROZEN_CAPTURE_PLAN)
        .usage_context("cannot parse embedded raw/apply capture plan")?;
    if supplied_catalog != frozen_catalog
        || supplied_plan != frozen_plan
        || supplied_catalog["targetSha256"] != RAW_APPLY_TARGET_SHA256
        || supplied_catalog["identity"] != "isaac-cs-t0-04-passive-raw-apply-catalog-v4"
        || supplied_catalog["consumerHooksMask"] != "0x000000000000007F"
        || supplied_catalog["rawSourceMask"] != "0x0000000000000003"
        || supplied_catalog["legacyInputHooksMask"] != "0x00000000000007FF"
        || supplied_plan["targetSha256"] != RAW_APPLY_TARGET_SHA256
        || supplied_plan["evidenceClass"] != "PASSIVE_RAW_APPLY_RUNTIME"
        || supplied_plan["eligibleForActiveGate"] != false
    {
        return Err(AppError::evidence(
            "raw/apply catalog or capture plan structure is not the exact frozen identity",
        ));
    }
    Ok(())
}

#[cfg(windows)]
mod live {
    use std::io;
    use std::path::{Path, PathBuf};
    use std::process::Command;
    use std::sync::mpsc;
    use std::thread;
    use std::time::{Duration, Instant};

    use chrono::Utc;
    use sha2::{Digest, Sha256};

    use super::*;
    use crate::artifact::file_sha256;
    use crate::input_artifact::validate_loop_gate_reference;
    use crate::loop_commands::{
        CompanionLoopRun, capture_loop_target_bytes, drain_companion_loop, loop_entropy,
    };
    use crate::loop_ipc::{LOOP_HOOK_MASK, LOOP_IPC_VERSION, LoopMapping, LoopNativeState};
    use crate::loop_verification::{analyze_loop_run, inspect_loop};
    use crate::raw_apply_artifact::{
        RAW_APPLY_TARGET_BYTES_SCHEMA_VERSION, RAW_APPLY_TARGET_SLOTS, RawApplyFinish, RawApplyRun,
        RawApplyRunConfig, RawApplyTargetBytesDocument, RawApplyTargetSlot,
    };
    use crate::raw_apply_ipc::{
        RAW_APPLY_CONSUMER_HOOK_MASK, RAW_APPLY_IPC_SCHEMA_VERSION, RAW_APPLY_LEGACY_HOOK_MASK,
        RAW_APPLY_SOURCE_MASK, RawApplyMapping, RawApplyNativeState, RawApplyProbe,
    };
    use crate::win32::{self, ObservedProcess};

    const READY_TIMEOUT: Duration = Duration::from_secs(15);
    const MAX_CAPTURE: Duration = Duration::from_secs(600);
    const POLL_INTERVAL: Duration = Duration::from_millis(5);

    #[derive(Clone, Copy)]
    enum Scenario {
        KeyboardHeldRelease = 1,
        KeyboardEdge = 2,
        ControllerHeldRelease = 3,
        ControllerEdge = 4,
    }

    impl Scenario {
        fn parse(value: &str) -> AppResult<Self> {
            match value {
                "keyboard-held-release" => Ok(Self::KeyboardHeldRelease),
                "keyboard-edge" => Ok(Self::KeyboardEdge),
                "controller-held-release" => Ok(Self::ControllerHeldRelease),
                "controller-edge" => Ok(Self::ControllerEdge),
                _ => Err(AppError::usage(format!(
                    "unsupported passive raw/apply scenario {value}"
                ))),
            }
        }

        fn companion_scenario(self) -> &'static str {
            match self {
                Self::KeyboardHeldRelease => "keyboard",
                Self::KeyboardEdge | Self::ControllerEdge => "rapid-tap",
                Self::ControllerHeldRelease => "controller",
            }
        }
    }

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
                let _ = run_helper(self.helper, "--unload-input", self.pid, self.native, None);
            }
        }
    }

    #[allow(clippy::too_many_arguments)]
    pub(super) fn run(
        probe: &str,
        scenario_value: &str,
        game_exe: &Path,
        loop_ledger: &Path,
        loop_gate: &Path,
        raw_apply_catalog: &Path,
        capture_plan: &Path,
        expected_variant: &str,
        artifacts_root: &Path,
        case_id: &str,
    ) -> AppResult<()> {
        if probe != "passive" {
            return Err(AppError::usage("input-raw-smoke requires --probe passive"));
        }
        validate_raw_apply_documents(raw_apply_catalog, capture_plan)?;
        validate_case_and_root(artifacts_root, case_id)?;
        let scenario = Scenario::parse(scenario_value)?;
        let canonical_game = std::fs::canonicalize(game_exe).usage_context(format!(
            "cannot resolve game executable {}",
            game_exe.display()
        ))?;
        let inspection = inspect_loop(&canonical_game, loop_ledger, expected_variant)?;
        if inspection.file_sha256 != RAW_APPLY_TARGET_SHA256 {
            return Err(AppError::evidence(
                "raw/apply capture refuses a target outside the frozen isaac-ng.exe SHA-256",
            ));
        }
        let gate = validate_loop_gate_reference(loop_gate)?;
        if gate.target_sha256 != inspection.file_sha256
            || gate.loop_ledger_sha256 != inspection.ledger_sha256
        {
            return Err(AppError::evidence(
                "derived V5 loop gate does not bind the frozen target and ledger",
            ));
        }

        let tool = std::env::current_exe().usage_context("cannot locate T0 controller")?;
        let distribution = tool
            .parent()
            .ok_or_else(|| AppError::usage("controller has no parent directory"))?;
        let native = distribution.join("IsaacCsT0.Native.dll");
        let helper = distribution.join("IsaacCsT0.Helper.exe");
        ensure_file(&native, "T0-04 Native DLL")?;
        ensure_file(&helper, "T0-04 x86 helper")?;
        let native_sha256 = file_sha256(&native)?;
        let helper_sha256 = file_sha256(&helper)?;

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
                "input-raw-smoke requires exactly one already-running isaac-ng.exe",
            ));
        }
        let process = ObservedProcess::open_exact(pids[0], &canonical_game)?;
        if process.debugger_present()? || !process.forbidden_modules().is_empty() {
            return Err(AppError::evidence(
                "input-raw-smoke refuses a debugged or instrumented target",
            ));
        }

        let nonce = loop_entropy(process.pid, scenario as u32)?;
        let suffix = &hex::encode_upper(nonce)[..16];
        let loop_mapping_name = format!(
            "Local\\IsaacCsT0.Loop.v{}.{}.{}",
            LOOP_IPC_VERSION, process.pid, suffix
        );
        let raw_mapping_name = format!(
            "Local\\IsaacCsT0.Input.v{}.{}.{}",
            RAW_APPLY_IPC_SCHEMA_VERSION, process.pid, suffix
        );
        let loop_mapping =
            LoopMapping::create(loop_mapping_name, process.pid, scenario as u32, nonce)?;
        let raw_mapping = RawApplyMapping::create(
            raw_mapping_name,
            process.pid,
            RawApplyProbe::Passive,
            scenario as u32,
            nonce,
            decode_hash(&inspection.file_sha256)?,
            decode_hash(&gate.sha256)?,
            decode_hash(RAW_APPLY_CATALOG_SHA256)?,
            decode_hash(RAW_APPLY_CAPTURE_PLAN_SHA256)?,
            loop_mapping.name(),
        )?;

        let loop_before = capture_loop_target_bytes(&process)?;
        let raw_before = capture_raw_apply_target_bytes(&process, &inspection.file_sha256)?;
        let timestamp = Utc::now().format("%Y%m%dT%H%M%SZ");
        let output_directory = artifacts_root.join(format!(
            "{timestamp}-t0-04-input-raw-passive-{scenario_value}-{case_id}"
        ));
        let companion_path = PathBuf::from("loop-context");
        let mut raw_run = RawApplyRun::create(
            &output_directory,
            RawApplyRunConfig {
                case_id: case_id.to_owned(),
                scenario: scenario_value.to_owned(),
                target_sha256: inspection.file_sha256.clone(),
                loop_gate_sha256: gate.sha256.clone(),
                artifact_identity: artifact_identity(
                    case_id,
                    scenario_value,
                    &inspection.file_sha256,
                    &gate.sha256,
                    &nonce,
                ),
                companion_loop_artifact_path: companion_path.clone(),
                native_sha256: native_sha256.clone(),
                helper_sha256: helper_sha256.clone(),
                target_bytes_before: raw_before.clone(),
            },
        )?;
        let mut loop_run = CompanionLoopRun::create_in(
            output_directory.join(&companion_path),
            case_id,
            scenario.companion_scenario(),
            &inspection.file_sha256,
            &inspection.ledger_sha256,
            &native_sha256,
            &helper_sha256,
        )?;
        let mut next_raw = 0;
        let mut next_loop = 0;
        let capture = observe(
            &mut raw_run,
            &mut loop_run,
            &raw_mapping,
            &loop_mapping,
            &process,
            &native,
            &helper,
            &mut next_raw,
            &mut next_loop,
            scenario_value,
        );
        let final_drain = drain_all(
            &mut raw_run,
            &mut loop_run,
            &raw_mapping,
            &loop_mapping,
            &mut next_raw,
            &mut next_loop,
        );
        let summary = finish_summary(&raw_mapping);
        let mut failures = Vec::new();
        if let Err(error) = capture {
            failures.push(error.to_string());
        }
        if let Err(error) = final_drain {
            failures.push(format!("cannot perform final raw/apply drain: {error}"));
        }
        if !final_observed_sources_valid(scenario_value, summary.observed_raw_source_mask) {
            failures.push(format!(
                "raw/apply observed source mask 0x{:X} does not contain the {scenario_value} owner source or contains unknown bits",
                summary.observed_raw_source_mask
            ));
        }
        let target_alive = match process.wait(0) {
            Ok(exited) => {
                if exited {
                    failures.push("target exited before restoration verification".to_owned());
                }
                !exited
            }
            Err(error) => {
                failures.push(format!(
                    "cannot query target state after restoration: {error}"
                ));
                false
            }
        };
        let raw_after = if target_alive {
            match capture_raw_apply_target_bytes(&process, &inspection.file_sha256) {
                Ok(document) => Some(document),
                Err(error) => {
                    failures.push(format!(
                        "cannot capture raw/apply target bytes after restoration: {error}"
                    ));
                    None
                }
            }
        } else {
            None
        };
        let loop_after = if target_alive {
            match capture_loop_target_bytes(&process) {
                Ok(document) => Some(document),
                Err(error) => {
                    failures.push(format!(
                        "cannot capture companion loop target bytes after restoration: {error}"
                    ));
                    None
                }
            }
        } else {
            None
        };
        if !failures.is_empty() {
            let reason = failures.join("; ");
            let _ = loop_run.fail_with_evidence(
                loop_mapping.main_thread_id(),
                loop_mapping.hooks_mask(),
                loop_mapping.overwritten_events(),
                &loop_before,
                loop_after.as_ref(),
            );
            raw_run.fail(&reason, raw_after.as_ref(), summary)?;
            return Err(AppError::evidence(reason));
        }
        let raw_after = raw_after.ok_or_else(|| AppError::evidence("missing after snapshot"))?;
        let loop_after = loop_after.ok_or_else(|| AppError::evidence("missing loop snapshot"))?;
        if raw_before != raw_after || loop_before.entries != loop_after.entries {
            let reason = "one or more of the 50 observation slots were not restored";
            loop_run.fail_with_evidence(
                loop_mapping.main_thread_id(),
                loop_mapping.hooks_mask(),
                loop_mapping.overwritten_events(),
                &loop_before,
                Some(&loop_after),
            )?;
            raw_run.fail(reason, Some(&raw_after), summary)?;
            return Err(AppError::evidence(reason));
        }
        if raw_mapping.main_thread_id() == 0
            || raw_mapping.main_thread_id() != loop_mapping.main_thread_id()
        {
            let reason = "raw/apply and companion loop did not bind the same main thread";
            loop_run.fail_with_evidence(
                loop_mapping.main_thread_id(),
                loop_mapping.hooks_mask(),
                loop_mapping.overwritten_events(),
                &loop_before,
                Some(&loop_after),
            )?;
            raw_run.fail(reason, Some(&raw_after), summary)?;
            return Err(AppError::evidence(reason));
        }
        if let Err(error) = loop_run.finish(&loop_mapping, &loop_before, &loop_after) {
            loop_run.fail();
            raw_run.fail(&error.to_string(), Some(&raw_after), summary)?;
            return Err(error);
        }
        if let Err(error) = analyze_loop_run(&loop_run.directory) {
            loop_run.fail();
            raw_run.fail(&error.to_string(), Some(&raw_after), summary)?;
            return Err(error);
        }
        let report = raw_run.finish(&raw_after, summary)?;
        println!(
            "{}",
            serde_json::to_string_pretty(&report)
                .usage_context("cannot serialize raw/apply analysis")?
        );
        if report.result == "PASS" {
            Ok(())
        } else {
            Err(AppError::evidence(
                "observation-only raw/apply analysis failed",
            ))
        }
    }

    #[allow(clippy::too_many_arguments)]
    fn observe(
        raw_run: &mut RawApplyRun,
        loop_run: &mut CompanionLoopRun,
        raw_mapping: &RawApplyMapping,
        loop_mapping: &LoopMapping,
        process: &ObservedProcess,
        native: &Path,
        helper: &Path,
        next_raw: &mut u64,
        next_loop: &mut u64,
        scenario: &str,
    ) -> AppResult<()> {
        run_helper(
            helper,
            "--inject-input",
            process.pid,
            native,
            Some(raw_mapping.name()),
        )?;
        let mut guard = InjectionGuard {
            helper,
            native,
            pid: process.pid,
            armed: true,
        };
        let capture_result = capture_window(
            raw_run,
            loop_run,
            raw_mapping,
            loop_mapping,
            process,
            next_raw,
            next_loop,
            scenario,
        );
        let unload_result = match process.wait(0) {
            Ok(true) => {
                guard.disarm();
                Err(AppError::evidence(
                    "target exited before raw/apply hooks could be restored",
                ))
            }
            Ok(false) => {
                let result = run_helper(helper, "--unload-input", process.pid, native, None);
                guard.disarm();
                result
            }
            Err(wait_error) => {
                let restore = run_helper(helper, "--unload-input", process.pid, native, None);
                guard.disarm();
                match restore {
                    Ok(()) => Err(AppError::evidence(format!(
                        "cannot query target state before restoration: {wait_error}"
                    ))),
                    Err(restore_error) => Err(AppError::evidence(format!(
                        "cannot query target state before restoration: {wait_error}; restoration also failed: {restore_error}"
                    ))),
                }
            }
        };
        let stopped = wait_for_stopped(
            raw_run,
            loop_run,
            raw_mapping,
            loop_mapping,
            next_raw,
            next_loop,
        );
        capture_result?;
        unload_result?;
        stopped
    }

    #[allow(clippy::too_many_arguments)]
    fn capture_window(
        raw_run: &mut RawApplyRun,
        loop_run: &mut CompanionLoopRun,
        raw_mapping: &RawApplyMapping,
        loop_mapping: &LoopMapping,
        process: &ObservedProcess,
        next_raw: &mut u64,
        next_loop: &mut u64,
        scenario: &str,
    ) -> AppResult<()> {
        let deadline = Instant::now() + READY_TIMEOUT;
        loop {
            drain_all(
                raw_run,
                loop_run,
                raw_mapping,
                loop_mapping,
                next_raw,
                next_loop,
            )?;
            match (raw_mapping.state()?, loop_mapping.state()?) {
                (RawApplyNativeState::Ready, LoopNativeState::Ready) => break,
                (RawApplyNativeState::Failed | RawApplyNativeState::Unsafe, _)
                | (_, LoopNativeState::Failed | LoopNativeState::Unsafe) => {
                    return Err(AppError::evidence(format!(
                        "raw/apply runtime initialization failed: raw={} loop={}",
                        raw_mapping.error_code(),
                        loop_mapping.error_code()
                    )));
                }
                _ if Instant::now() >= deadline => {
                    return Err(AppError::evidence(
                        "raw/apply and companion loop did not become ready within 15 seconds",
                    ));
                }
                _ => thread::sleep(POLL_INTERVAL),
            }
        }
        if raw_mapping.legacy_installed_hooks_mask() != RAW_APPLY_LEGACY_HOOK_MASK
            || raw_mapping.required_raw_source_mask() != RAW_APPLY_SOURCE_MASK
            || raw_mapping.required_consumer_hooks_mask() != RAW_APPLY_CONSUMER_HOOK_MASK
            || raw_mapping.installed_consumer_hooks_mask() != RAW_APPLY_CONSUMER_HOOK_MASK
            || loop_mapping.hooks_mask() != LOOP_HOOK_MASK
        {
            return Err(AppError::evidence("raw/apply Ready masks are incomplete"));
        }
        let (stop_tx, stop_rx) = mpsc::channel();
        thread::spawn(move || {
            let mut line = String::new();
            if io::stdin().read_line(&mut line).is_ok() {
                let _ = stop_tx.send(());
            }
        });
        println!(
            "READY T0-04 observation-only {scenario} raw/apply capture is active; press Enter once to stop and restore (bounded at ten minutes)."
        );
        let started = Instant::now();
        loop {
            if process.wait(0)? {
                return Err(AppError::evidence(
                    "target exited before raw/apply hooks could be restored",
                ));
            }
            drain_all(
                raw_run,
                loop_run,
                raw_mapping,
                loop_mapping,
                next_raw,
                next_loop,
            )?;
            if raw_mapping.overwritten_events() != 0 || loop_mapping.overwritten_events() != 0 {
                return Err(AppError::evidence(
                    "raw/apply or companion loop ring reported overwrite",
                ));
            }
            if matches!(
                raw_mapping.state()?,
                RawApplyNativeState::Failed | RawApplyNativeState::Unsafe
            ) || matches!(
                loop_mapping.state()?,
                LoopNativeState::Failed | LoopNativeState::Unsafe
            ) {
                return Err(AppError::evidence("raw/apply runtime failed closed"));
            }
            if stop_rx.try_recv().is_ok() || started.elapsed() >= MAX_CAPTURE {
                return Ok(());
            }
            thread::sleep(POLL_INTERVAL);
        }
    }

    fn wait_for_stopped(
        raw_run: &mut RawApplyRun,
        loop_run: &mut CompanionLoopRun,
        raw_mapping: &RawApplyMapping,
        loop_mapping: &LoopMapping,
        next_raw: &mut u64,
        next_loop: &mut u64,
    ) -> AppResult<()> {
        let deadline = Instant::now() + READY_TIMEOUT;
        loop {
            drain_all(
                raw_run,
                loop_run,
                raw_mapping,
                loop_mapping,
                next_raw,
                next_loop,
            )?;
            if raw_mapping.state()? == RawApplyNativeState::Stopped
                && loop_mapping.state()? == LoopNativeState::Stopped
                && raw_mapping.producer_closed()
                && loop_mapping.producer_closed()
            {
                if raw_mapping.legacy_restored_hooks_mask() != RAW_APPLY_LEGACY_HOOK_MASK
                    || raw_mapping.restored_consumer_hooks_mask() != RAW_APPLY_CONSUMER_HOOK_MASK
                    || raw_mapping.active_hooks() != 0
                {
                    return Err(AppError::evidence(
                        "raw/apply shutdown masks or active hook count are invalid",
                    ));
                }
                return Ok(());
            }
            if matches!(
                raw_mapping.state()?,
                RawApplyNativeState::Failed | RawApplyNativeState::Unsafe
            ) || matches!(
                loop_mapping.state()?,
                LoopNativeState::Failed | LoopNativeState::Unsafe
            ) {
                return Err(AppError::evidence(
                    "raw/apply runtime entered failed/unsafe during restoration",
                ));
            }
            if Instant::now() >= deadline {
                return Err(AppError::evidence(
                    "raw/apply runtime did not stop and close producers within 15 seconds",
                ));
            }
            thread::sleep(POLL_INTERVAL);
        }
    }

    fn drain_all(
        raw_run: &mut RawApplyRun,
        loop_run: &mut CompanionLoopRun,
        raw_mapping: &RawApplyMapping,
        loop_mapping: &LoopMapping,
        next_raw: &mut u64,
        next_loop: &mut u64,
    ) -> AppResult<()> {
        for record in raw_mapping.drain(next_raw)? {
            raw_run.append_native(&record)?;
        }
        let _ = drain_companion_loop(loop_run, loop_mapping, next_loop)?;
        Ok(())
    }

    fn finish_summary(mapping: &RawApplyMapping) -> RawApplyFinish {
        RawApplyFinish {
            main_thread_id: mapping.main_thread_id(),
            legacy_installed_hooks_mask: mapping.legacy_installed_hooks_mask(),
            legacy_restored_hooks_mask: mapping.legacy_restored_hooks_mask(),
            required_raw_source_mask: mapping.required_raw_source_mask(),
            observed_raw_source_mask: mapping.observed_raw_source_mask(),
            required_consumer_hooks_mask: mapping.required_consumer_hooks_mask(),
            installed_consumer_hooks_mask: mapping.installed_consumer_hooks_mask(),
            restored_consumer_hooks_mask: mapping.restored_consumer_hooks_mask(),
            overwritten_events: mapping.overwritten_events(),
            redacted_events: mapping.redacted_events(),
            producer_closed: mapping.producer_closed(),
        }
    }

    fn capture_raw_apply_target_bytes(
        process: &ObservedProcess,
        target_sha256: &str,
    ) -> AppResult<RawApplyTargetBytesDocument> {
        let base = process.main_module.base;
        let mut slots = Vec::with_capacity(RAW_APPLY_TARGET_SLOTS.len());
        for (id, rva, size) in RAW_APPLY_TARGET_SLOTS {
            slots.push(RawApplyTargetSlot {
                id: id.to_owned(),
                rva: format!("0x{rva:08X}"),
                bytes_hex: hex::encode_upper(process.read_memory(base + u64::from(rva), size)?),
            });
        }
        Ok(RawApplyTargetBytesDocument {
            schema_version: RAW_APPLY_TARGET_BYTES_SCHEMA_VERSION,
            target_sha256: target_sha256.to_ascii_uppercase(),
            slots,
        })
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
            .usage_context("cannot run independent T0-04 helper")?;
        let stdout = String::from_utf8_lossy(&output.stdout);
        let stderr = String::from_utf8_lossy(&output.stderr);
        if !output.status.success() || !stdout.trim_start().starts_with("PASS") {
            return Err(AppError::evidence(format!(
                "T0-04 helper {operation} failed (exit={:?}): {}{}",
                output.status.code(),
                stdout.trim(),
                stderr.trim()
            )));
        }
        Ok(())
    }

    fn artifact_identity(
        case_id: &str,
        scenario: &str,
        target_sha256: &str,
        loop_gate_sha256: &str,
        nonce: &[u8; 32],
    ) -> String {
        let mut hasher = Sha256::new();
        hasher.update(b"IsaacCsT0-RawApply-v1");
        hasher.update(case_id.as_bytes());
        hasher.update(scenario.as_bytes());
        hasher.update(target_sha256.as_bytes());
        hasher.update(loop_gate_sha256.as_bytes());
        hasher.update(RAW_APPLY_CATALOG_SHA256.as_bytes());
        hasher.update(RAW_APPLY_CAPTURE_PLAN_SHA256.as_bytes());
        hasher.update(nonce);
        format!("passive-raw-apply-{}", hex::encode_upper(hasher.finalize()))
    }

    fn decode_hash(value: &str) -> AppResult<[u8; 32]> {
        let bytes = hex::decode(value)
            .map_err(|_| AppError::evidence("bound SHA-256 is not hexadecimal"))?;
        bytes
            .try_into()
            .map_err(|_| AppError::evidence("bound SHA-256 is not exactly 32 bytes"))
    }

    fn ensure_file(path: &Path, label: &str) -> AppResult<()> {
        if !path.is_file() {
            return Err(AppError::usage(format!(
                "{label} is missing: {}",
                path.display()
            )));
        }
        Ok(())
    }

    fn validate_case_and_root(root: &Path, case_id: &str) -> AppResult<()> {
        if !root.is_dir()
            || case_id.is_empty()
            || case_id.len() > 64
            || !case_id
                .bytes()
                .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_'))
        {
            return Err(AppError::usage(
                "raw/apply capture requires an existing artifact root and valid case id",
            ));
        }
        Ok(())
    }
}

#[cfg(windows)]
#[allow(clippy::too_many_arguments)]
pub fn input_raw_smoke_command(
    probe: &str,
    scenario: &str,
    game_exe: &Path,
    loop_ledger: &Path,
    loop_gate: &Path,
    raw_apply_catalog: &Path,
    capture_plan: &Path,
    expected_variant: &str,
    artifacts_root: &Path,
    case_id: &str,
) -> AppResult<()> {
    live::run(
        probe,
        scenario,
        game_exe,
        loop_ledger,
        loop_gate,
        raw_apply_catalog,
        capture_plan,
        expected_variant,
        artifacts_root,
        case_id,
    )
}

#[cfg(not(windows))]
#[allow(clippy::too_many_arguments)]
pub fn input_raw_smoke_command(
    _probe: &str,
    _scenario: &str,
    _game_exe: &Path,
    _loop_ledger: &Path,
    _loop_gate: &Path,
    _raw_apply_catalog: &Path,
    _capture_plan: &Path,
    _expected_variant: &str,
    _artifacts_root: &Path,
    _case_id: &str,
) -> AppResult<()> {
    Err(AppError::usage(
        "observation-only raw/apply capture requires Windows",
    ))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn frozen_catalog_and_plan_are_exact_hash_bound_documents() {
        let root = Path::new(env!("CARGO_MANIFEST_DIR"));
        validate_raw_apply_documents(
            &root.join("raw-apply-catalog-v4.json"),
            &root.join("raw-apply-capture-plan-v4.json"),
        )
        .unwrap();

        assert!(
            validate_raw_apply_documents(
                &root.join("raw-apply-catalog-v3.json"),
                &root.join("raw-apply-capture-plan-v3.json"),
            )
            .is_err()
        );

        let temporary = tempfile::tempdir().unwrap();
        let forged = temporary.path().join("catalog.json");
        let mut value: serde_json::Value = serde_json::from_slice(FROZEN_CATALOG).unwrap();
        value["consumerHooks"][1]["siteRva"] = "0x00385BC3".into();
        std::fs::write(&forged, serde_json::to_vec_pretty(&value).unwrap()).unwrap();
        assert!(
            validate_raw_apply_documents(&forged, &root.join("raw-apply-capture-plan-v4.json"))
                .is_err()
        );
    }

    #[test]
    fn controller_final_source_check_is_scenario_specific() {
        assert!(final_observed_sources_valid("keyboard-held-release", 0x1));
        assert!(final_observed_sources_valid("keyboard-edge", 0x3));
        assert!(!final_observed_sources_valid("keyboard-edge", 0x2));
        assert!(final_observed_sources_valid("controller-held-release", 0x2));
        assert!(final_observed_sources_valid("controller-edge", 0x3));
        assert!(!final_observed_sources_valid("controller-edge", 0x1));
        assert!(!final_observed_sources_valid("controller-edge", 0x6));
    }
}
