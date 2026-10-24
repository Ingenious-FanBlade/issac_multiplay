use std::path::Path;

#[cfg(not(windows))]
use crate::error::AppError;
use crate::error::AppResult;

#[cfg(windows)]
mod live {
    use std::io;
    use std::mem::size_of;
    use std::path::{Path, PathBuf};
    use std::process::Command;
    use std::sync::mpsc;
    use std::thread;
    use std::time::{Duration, Instant};

    use chrono::Utc;
    use sha2::{Digest, Sha256};

    use crate::INPUT_TOOL_IDENTITY;
    use crate::artifact::file_sha256;
    use crate::error::{AppError, AppResult, UsageContext};
    use crate::input_artifact::{
        PASSIVE_INPUT_TARGET_BYTES_SCHEMA_VERSION, PassiveInputFinish, PassiveInputRun,
        PassiveInputRunConfig, PassiveInputTargetBytesDocument, PassiveInputTargetSlot,
        validate_loop_gate_reference,
    };
    use crate::input_ipc::{
        INPUT_HOOK_MASK, INPUT_IPC_SCHEMA_VERSION, InputMapping, InputNativeState, InputProbe,
    };
    use crate::loop_commands::{
        CompanionLoopRun, capture_loop_target_bytes, drain_companion_loop, loop_entropy,
    };
    use crate::loop_ipc::{LOOP_HOOK_MASK, LOOP_IPC_VERSION, LoopMapping, LoopNativeState};
    use crate::loop_verification::{analyze_loop_run, inspect_loop};
    use crate::win32::{self, ObservedProcess};

    const READY_TIMEOUT: Duration = Duration::from_secs(15);
    const MAX_CAPTURE: Duration = Duration::from_secs(600);
    const POLL_INTERVAL: Duration = Duration::from_millis(5);

    const INPUT_POINTER_SLOTS: &[(&str, u32)] = &[
        ("manager-pressed", 0x0078_2950 + 0x40),
        ("manager-triggered", 0x0078_2950 + 0x44),
        ("manager-scalar", 0x0078_2950 + 0x48),
        ("keyboard-logical-pressed", 0x007A_1678 + 0x30),
        ("keyboard-logical-triggered", 0x007A_1678 + 0x34),
        ("keyboard-logical-scalar", 0x007A_1678 + 0x38),
        ("gamepad-logical-pressed", 0x0078_2C80 + 0x30),
        ("gamepad-logical-triggered", 0x0078_2C80 + 0x34),
        ("gamepad-logical-scalar", 0x0078_2C80 + 0x38),
        ("keyboard-logical-setter", 0x007A_1678 + 0x4C),
        ("gamepad-logical-setter", 0x0078_2C80 + 0x4C),
    ];

    #[derive(Clone, Copy)]
    enum Scenario {
        Keyboard = 1,
        Controller = 2,
        Simultaneous = 3,
        RapidTap = 4,
        DeviceSwitch = 5,
        Pause = 6,
        Focus = 7,
        Loading = 8,
        Menu = 9,
        Room = 10,
    }

    impl Scenario {
        fn parse(value: &str) -> AppResult<Self> {
            match value {
                "keyboard" => Ok(Self::Keyboard),
                "controller" => Ok(Self::Controller),
                "simultaneous" => Ok(Self::Simultaneous),
                "rapid-tap" => Ok(Self::RapidTap),
                "device-switch" => Ok(Self::DeviceSwitch),
                "pause" => Ok(Self::Pause),
                "focus" => Ok(Self::Focus),
                "loading" => Ok(Self::Loading),
                "menu" => Ok(Self::Menu),
                "room" => Ok(Self::Room),
                _ => Err(AppError::usage(format!(
                    "unknown observation-only input scenario {value}"
                ))),
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
        loop_gate: Option<&Path>,
        expected_variant: &str,
        artifacts_root: &Path,
        case_id: &str,
        arm_token: Option<&str>,
    ) -> AppResult<()> {
        if probe != "passive" {
            return Err(AppError::usage(
                "native observation-only input capture requires --probe passive",
            ));
        }
        if arm_token.is_some() {
            return Err(AppError::usage(
                "passive input capture never accepts an arm token",
            ));
        }
        let loop_gate = loop_gate.ok_or_else(|| {
            AppError::usage("passive input capture requires the replayable derived V5 --loop-gate")
        })?;
        validate_case_and_root(artifacts_root, case_id)?;
        let scenario = Scenario::parse(scenario_value)?;
        let canonical_game = std::fs::canonicalize(game_exe).usage_context(format!(
            "cannot resolve game executable {}",
            game_exe.display()
        ))?;
        let inspection = inspect_loop(&canonical_game, loop_ledger, expected_variant)?;
        let gate = validate_loop_gate_reference(loop_gate)?;
        if gate.target_sha256 != inspection.file_sha256
            || gate.loop_ledger_sha256 != inspection.ledger_sha256
        {
            return Err(AppError::evidence(
                "derived V5 loop gate does not bind the selected target and frozen ledger",
            ));
        }
        crate::input_core::run_codec_self_test()
            .map_err(|error| AppError::evidence(error.to_string()))?;

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
                "input-smoke passive requires exactly one already-running isaac-ng.exe",
            ));
        }
        let process = ObservedProcess::open_exact(pids[0], &canonical_game)?;
        if process.debugger_present()? || !process.forbidden_modules().is_empty() {
            return Err(AppError::evidence(
                "input-smoke passive refuses a debugged or instrumented target",
            ));
        }

        let nonce = loop_entropy(process.pid, scenario as u32)?;
        let suffix = &hex::encode_upper(nonce)[..16];
        let loop_mapping_name = format!(
            "Local\\IsaacCsT0.Loop.v{}.{}.{}",
            LOOP_IPC_VERSION, process.pid, suffix
        );
        let input_mapping_name = format!(
            "Local\\IsaacCsT0.Input.v{}.{}.{}",
            INPUT_IPC_SCHEMA_VERSION, process.pid, suffix
        );
        let loop_mapping =
            LoopMapping::create(loop_mapping_name, process.pid, scenario as u32, nonce)?;
        let input_mapping = InputMapping::create(
            input_mapping_name,
            process.pid,
            InputProbe::Passive,
            scenario as u32,
            nonce,
            decode_hash(&inspection.file_sha256)?,
            decode_hash(&gate.sha256)?,
            loop_mapping.name(),
        )?;

        let loop_before = capture_loop_target_bytes(&process)?;
        let input_before = capture_input_target_bytes(&process, &inspection.file_sha256)?;
        let timestamp = Utc::now().format("%Y%m%dT%H%M%SZ");
        let output_directory = artifacts_root.join(format!(
            "{timestamp}-t0-04-input-passive-{scenario_value}-{case_id}"
        ));
        let companion_path = PathBuf::from("loop-context");
        let artifact_identity = artifact_identity(
            case_id,
            scenario_value,
            &inspection.file_sha256,
            &gate.sha256,
            &nonce,
        );
        let mut input_run = PassiveInputRun::create(
            &output_directory,
            PassiveInputRunConfig {
                case_id: case_id.to_owned(),
                scenario: scenario_value.to_owned(),
                target_sha256: inspection.file_sha256.clone(),
                loop_gate_sha256: gate.sha256.clone(),
                artifact_identity,
                companion_loop_artifact_path: companion_path.clone(),
                native_sha256: native_sha256.clone(),
                helper_sha256: helper_sha256.clone(),
                target_bytes_before: input_before.clone(),
            },
        )?;
        let mut loop_run = CompanionLoopRun::create_in(
            output_directory.join(&companion_path),
            case_id,
            scenario_value,
            &inspection.file_sha256,
            &inspection.ledger_sha256,
            &native_sha256,
            &helper_sha256,
        )?;
        let mut next_input = 0_u64;
        let mut next_loop = 0_u64;

        let capture = observe(
            &mut input_run,
            &mut loop_run,
            &input_mapping,
            &loop_mapping,
            &process,
            &native,
            &helper,
            &mut next_input,
            &mut next_loop,
            scenario_value,
        );
        let final_drain = drain_all(
            &mut input_run,
            &mut loop_run,
            &input_mapping,
            &loop_mapping,
            &mut next_input,
            &mut next_loop,
        );
        let summary = finish_summary(&input_mapping);
        let mut failures = Vec::new();
        if let Err(error) = capture {
            failures.push(error.to_string());
        }
        if let Err(error) = final_drain {
            failures.push(format!(
                "cannot perform final observation-ring drain: {error}"
            ));
        }
        let target_alive = match process.wait(0) {
            Ok(exited) => !exited,
            Err(error) => {
                failures.push(format!(
                    "cannot query target state after restoration: {error}"
                ));
                false
            }
        };
        let input_after = if target_alive {
            match capture_input_target_bytes(&process, &inspection.file_sha256) {
                Ok(document) => Some(document),
                Err(error) => {
                    failures.push(format!(
                        "cannot capture input target bytes after restoration: {error}"
                    ));
                    None
                }
            }
        } else {
            failures.push("target exited before input restoration could be verified".to_owned());
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
            input_run.fail(&reason, input_after.as_ref(), summary)?;
            return Err(AppError::evidence(reason));
        }

        let input_after = input_after.ok_or_else(|| {
            AppError::evidence("target exited before input restoration could be verified")
        })?;
        let loop_after = loop_after.ok_or_else(|| {
            AppError::evidence("target exited before loop restoration could be verified")
        })?;
        if input_before != input_after || loop_before.entries != loop_after.entries {
            let reason = "one or more of the 43 observation slots were not restored";
            loop_run.fail_with_evidence(
                loop_mapping.main_thread_id(),
                loop_mapping.hooks_mask(),
                loop_mapping.overwritten_events(),
                &loop_before,
                Some(&loop_after),
            )?;
            input_run.fail(reason, Some(&input_after), summary)?;
            return Err(AppError::evidence(reason));
        }
        if input_mapping.main_thread_id() == 0
            || input_mapping.main_thread_id() != loop_mapping.main_thread_id()
        {
            let reason = "input and companion loop did not bind the same nonzero main thread";
            loop_run.fail_with_evidence(
                loop_mapping.main_thread_id(),
                loop_mapping.hooks_mask(),
                loop_mapping.overwritten_events(),
                &loop_before,
                Some(&loop_after),
            )?;
            input_run.fail(reason, Some(&input_after), summary)?;
            return Err(AppError::evidence(reason));
        }

        if let Err(error) = loop_run.finish(&loop_mapping, &loop_before, &loop_after) {
            loop_run.fail();
            input_run.fail(&error.to_string(), Some(&input_after), summary)?;
            return Err(error);
        }
        if let Err(error) = analyze_loop_run(&loop_run.directory) {
            loop_run.fail();
            input_run.fail(&error.to_string(), Some(&input_after), summary)?;
            return Err(error);
        }
        let report = input_run.finish(&input_after, summary)?;
        println!(
            "{}",
            serde_json::to_string_pretty(&report)
                .usage_context("cannot serialize passive input analysis")?
        );
        if report.result == "PASS" {
            Ok(())
        } else {
            Err(AppError::evidence(
                "observation-only input passive analysis failed",
            ))
        }
    }

    #[allow(clippy::too_many_arguments)]
    fn observe(
        input_run: &mut PassiveInputRun,
        loop_run: &mut CompanionLoopRun,
        input_mapping: &InputMapping,
        loop_mapping: &LoopMapping,
        process: &ObservedProcess,
        native: &Path,
        helper: &Path,
        next_input: &mut u64,
        next_loop: &mut u64,
        scenario: &str,
    ) -> AppResult<()> {
        run_helper(
            helper,
            "--inject-input",
            process.pid,
            native,
            Some(input_mapping.name()),
        )?;
        let mut guard = InjectionGuard {
            helper,
            native,
            pid: process.pid,
            armed: true,
        };
        let capture_result = capture_window(
            input_run,
            loop_run,
            input_mapping,
            loop_mapping,
            process,
            next_input,
            next_loop,
            scenario,
        );
        let target_state = process.wait(0);
        guard.disarm();
        let unload_result = match target_state {
            Ok(true) => Err(AppError::evidence(
                "target exited before observation hooks could be restored",
            )),
            Ok(false) => run_helper(helper, "--unload-input", process.pid, native, None),
            Err(wait_error) => {
                let restore = run_helper(helper, "--unload-input", process.pid, native, None);
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
        let stopped_result = wait_for_stopped(
            input_run,
            loop_run,
            input_mapping,
            loop_mapping,
            next_input,
            next_loop,
        );
        capture_result?;
        unload_result?;
        stopped_result
    }

    #[allow(clippy::too_many_arguments)]
    fn capture_window(
        input_run: &mut PassiveInputRun,
        loop_run: &mut CompanionLoopRun,
        input_mapping: &InputMapping,
        loop_mapping: &LoopMapping,
        process: &ObservedProcess,
        next_input: &mut u64,
        next_loop: &mut u64,
        scenario: &str,
    ) -> AppResult<()> {
        let deadline = Instant::now() + READY_TIMEOUT;
        loop {
            drain_all(
                input_run,
                loop_run,
                input_mapping,
                loop_mapping,
                next_input,
                next_loop,
            )?;
            match (input_mapping.state()?, loop_mapping.state()?) {
                (InputNativeState::Ready, LoopNativeState::Ready) => break,
                (InputNativeState::Failed | InputNativeState::Unsafe, _)
                | (_, LoopNativeState::Failed | LoopNativeState::Unsafe) => {
                    return Err(AppError::evidence(format!(
                        "observation runtime initialization failed: input={} loop={}",
                        input_mapping.error_code(),
                        loop_mapping.error_code()
                    )));
                }
                _ if Instant::now() >= deadline => {
                    return Err(AppError::evidence(
                        "input and companion loop did not become ready within 15 seconds",
                    ));
                }
                _ => thread::sleep(POLL_INTERVAL),
            }
        }
        if input_mapping.installed_hooks_mask() != INPUT_HOOK_MASK
            || loop_mapping.hooks_mask() != LOOP_HOOK_MASK
        {
            return Err(AppError::evidence(format!(
                "observation hook mask incomplete: input=0x{:X} loop=0x{:X}",
                input_mapping.installed_hooks_mask(),
                loop_mapping.hooks_mask()
            )));
        }

        let (stop_tx, stop_rx) = mpsc::channel();
        thread::spawn(move || {
            let mut line = String::new();
            if io::stdin().read_line(&mut line).is_ok() {
                let _ = stop_tx.send(());
            }
        });
        println!(
            "READY T0-04 observation-only {scenario} capture is active; press Enter to stop and restore (bounded at ten minutes)."
        );
        let started = Instant::now();
        loop {
            if process.wait(0)? {
                return Err(AppError::evidence(
                    "target exited before observation hooks could be restored",
                ));
            }
            drain_all(
                input_run,
                loop_run,
                input_mapping,
                loop_mapping,
                next_input,
                next_loop,
            )?;
            if input_mapping.overwritten_events() != 0 || loop_mapping.overwritten_events() != 0 {
                return Err(AppError::evidence(
                    "input or companion loop event ring reported an overwrite",
                ));
            }
            if matches!(
                input_mapping.state()?,
                InputNativeState::Failed | InputNativeState::Unsafe
            ) || matches!(
                loop_mapping.state()?,
                LoopNativeState::Failed | LoopNativeState::Unsafe
            ) {
                return Err(AppError::evidence(format!(
                    "observation runtime failed closed: input={} loop={}",
                    input_mapping.error_code(),
                    loop_mapping.error_code()
                )));
            }
            if stop_rx.try_recv().is_ok() || started.elapsed() >= MAX_CAPTURE {
                return Ok(());
            }
            thread::sleep(POLL_INTERVAL);
        }
    }

    #[allow(clippy::too_many_arguments)]
    fn wait_for_stopped(
        input_run: &mut PassiveInputRun,
        loop_run: &mut CompanionLoopRun,
        input_mapping: &InputMapping,
        loop_mapping: &LoopMapping,
        next_input: &mut u64,
        next_loop: &mut u64,
    ) -> AppResult<()> {
        let deadline = Instant::now() + READY_TIMEOUT;
        loop {
            drain_all(
                input_run,
                loop_run,
                input_mapping,
                loop_mapping,
                next_input,
                next_loop,
            )?;
            if input_mapping.state()? == InputNativeState::Stopped
                && loop_mapping.state()? == LoopNativeState::Stopped
                && input_mapping.producer_closed()
            {
                if input_mapping.restored_hooks_mask() != INPUT_HOOK_MASK {
                    return Err(AppError::evidence(format!(
                        "input restored hook mask is incomplete: 0x{:X}",
                        input_mapping.restored_hooks_mask()
                    )));
                }
                return Ok(());
            }
            if matches!(
                input_mapping.state()?,
                InputNativeState::Failed | InputNativeState::Unsafe
            ) || matches!(
                loop_mapping.state()?,
                LoopNativeState::Failed | LoopNativeState::Unsafe
            ) {
                return Err(AppError::evidence(format!(
                    "observation runtime failed during restoration: input={} loop={}",
                    input_mapping.error_code(),
                    loop_mapping.error_code()
                )));
            }
            if Instant::now() >= deadline {
                return Err(AppError::evidence(
                    "observation runtime did not publish stopped state within 15 seconds",
                ));
            }
            thread::sleep(POLL_INTERVAL);
        }
    }

    fn drain_all(
        input_run: &mut PassiveInputRun,
        loop_run: &mut CompanionLoopRun,
        input_mapping: &InputMapping,
        loop_mapping: &LoopMapping,
        next_input: &mut u64,
        next_loop: &mut u64,
    ) -> AppResult<()> {
        for record in input_mapping.drain(next_input)? {
            input_run.append_native(&record)?;
        }
        let _ = drain_companion_loop(loop_run, loop_mapping, next_loop)?;
        Ok(())
    }

    fn finish_summary(mapping: &InputMapping) -> PassiveInputFinish {
        PassiveInputFinish {
            main_thread_id: mapping.main_thread_id(),
            installed_hooks_mask: mapping.installed_hooks_mask(),
            restored_hooks_mask: mapping.restored_hooks_mask(),
            overwritten_events: mapping.overwritten_events(),
            redacted_events: mapping.redacted_events(),
            producer_closed: mapping.producer_closed(),
        }
    }

    fn capture_input_target_bytes(
        process: &ObservedProcess,
        target_sha256: &str,
    ) -> AppResult<PassiveInputTargetBytesDocument> {
        let base = process.main_module.base;
        let mut slots = Vec::with_capacity(INPUT_POINTER_SLOTS.len());
        for (id, rva) in INPUT_POINTER_SLOTS {
            slots.push(PassiveInputTargetSlot {
                id: (*id).to_owned(),
                rva: format!("0x{rva:08X}"),
                bytes_hex: hex::encode_upper(
                    process.read_memory(base + u64::from(*rva), size_of::<u32>())?,
                ),
            });
        }
        Ok(PassiveInputTargetBytesDocument {
            schema_version: PASSIVE_INPUT_TARGET_BYTES_SCHEMA_VERSION,
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
        hasher.update(INPUT_TOOL_IDENTITY.as_bytes());
        hasher.update(case_id.as_bytes());
        hasher.update(scenario.as_bytes());
        hasher.update(target_sha256.as_bytes());
        hasher.update(loop_gate_sha256.as_bytes());
        hasher.update(nonce);
        format!("passive-runtime-{}", hex::encode_upper(hasher.finalize()))
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
                "passive input capture requires an existing artifact root and valid case id",
            ));
        }
        Ok(())
    }
}

#[cfg(windows)]
#[allow(clippy::too_many_arguments)]
pub fn input_smoke_command(
    probe: &str,
    scenario: &str,
    game_exe: &Path,
    loop_ledger: &Path,
    loop_gate: Option<&Path>,
    expected_variant: &str,
    artifacts_root: &Path,
    case_id: &str,
    arm_token: Option<&str>,
) -> AppResult<()> {
    live::run(
        probe,
        scenario,
        game_exe,
        loop_ledger,
        loop_gate,
        expected_variant,
        artifacts_root,
        case_id,
        arm_token,
    )
}

#[cfg(not(windows))]
#[allow(clippy::too_many_arguments)]
pub fn input_smoke_command(
    _probe: &str,
    _scenario: &str,
    _game_exe: &Path,
    _loop_ledger: &Path,
    _loop_gate: Option<&Path>,
    _expected_variant: &str,
    _artifacts_root: &Path,
    _case_id: &str,
    _arm_token: Option<&str>,
) -> AppResult<()> {
    Err(AppError::usage(
        "observation-only input passive capture requires Windows",
    ))
}
