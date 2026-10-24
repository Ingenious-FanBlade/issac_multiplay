#[cfg(windows)]
mod live {
    use std::collections::BTreeMap;
    use std::fs::{self, File, OpenOptions};
    use std::io::{self, Write};
    use std::path::{Path, PathBuf};
    use std::process::Command;
    use std::sync::mpsc;
    use std::thread;
    use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

    use chrono::Utc;
    use serde::Serialize;
    use sha2::{Digest, Sha256};

    use crate::artifact::file_sha256;
    use crate::error::{AppError, AppResult, UsageContext};
    use crate::loop_ipc::{
        LOOP_FLAG_GAMEPAD_DEVICE, LOOP_FLAG_KEYBOARD_DEVICE, LOOP_HOOK_MASK, LOOP_IPC_VERSION,
        LOOP_NO_ACTION, LoopMapping, LoopNativeEventKind, LoopNativeState, LoopPhase,
        NativeLoopEventRecord,
    };
    use crate::loop_verification::{
        LOOP_EVIDENCE_CLASS, LOOP_TOOL_IDENTITY, LoopEvent, LoopManifest, analyze_loop_run,
        inspect_loop,
    };
    use crate::win32::{self, ObservedProcess};

    const READY_TIMEOUT: Duration = Duration::from_secs(15);
    const MAX_CAPTURE: Duration = Duration::from_secs(600);

    const CALL_SITES: &[(u32, usize)] = &[
        (0x0053_1231, 5),
        (0x0053_12EC, 5),
        (0x0055_5231, 5),
        (0x002F_BBA5, 5),
        (0x0055_9696, 5),
        (0x005A_B84A, 5),
        (0x0033_F5B9, 5),
        (0x0053_12F1, 5),
        (0x0055_4F7D, 5),
        (0x0061_FC97, 5),
        (0x0055_4D5E, 5),
        (0x0055_9B4D, 5),
        (0x0055_9B82, 5),
        (0x002F_ADFA, 5),
        (0x002F_AE09, 5),
        (0x002F_BA4D, 5),
        (0x0040_3E70, 5),
        (0x0055_9B21, 5),
        (0x002F_580B, 5),
        (0x002F_61FF, 5),
        (0x002F_72C9, 5),
        (0x002F_7721, 5),
        (0x002F_7A10, 5),
        (0x002F_B4F0, 5),
        (0x0052_FC77, 5),
        (0x002F_B51C, 5),
        (0x002F_E37C, 5),
    ];
    const POINTER_SLOTS: &[u32] = &[
        0x007A_1678 + 0x78,
        0x0078_2C80 + 0x78,
        0x0071_8714,
        0x0071_8650,
        0x0071_85B8,
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
                    "unknown T0-03R loop scenario {value}"
                ))),
            }
        }
    }

    #[derive(Serialize)]
    #[serde(rename_all = "camelCase")]
    pub(crate) struct TargetBytesDocument {
        pub(crate) entries: BTreeMap<String, String>,
    }

    #[derive(Clone, Copy, Debug, Eq, PartialEq)]
    pub(crate) struct LoopInstallFailureDiagnostic {
        stage: u32,
        index: u32,
        win32_error: u32,
    }

    #[derive(Clone, Copy, Debug, Eq, PartialEq)]
    enum CaptureStopReason {
        User,
        BoundedWindow,
    }

    pub(crate) struct LoopRun {
        pub(crate) directory: PathBuf,
        manifest: LoopManifest,
        events: File,
        next_sequence: u64,
    }

    impl LoopRun {
        #[allow(clippy::too_many_arguments)]
        fn create(
            artifacts_root: &Path,
            case_id: &str,
            scenario: &str,
            target_sha256: &str,
            loop_ledger_sha256: &str,
            native_sha256: &str,
            helper_sha256: &str,
        ) -> AppResult<Self> {
            validate_case_id(case_id)?;
            let timestamp = Utc::now().format("%Y%m%dT%H%M%SZ");
            let directory = artifacts_root.join(format!(
                "{timestamp}-t0-03r-loop-passive-{scenario}-{case_id}"
            ));
            Self::create_in(
                directory,
                case_id,
                scenario,
                target_sha256,
                loop_ledger_sha256,
                native_sha256,
                helper_sha256,
            )
        }

        #[allow(clippy::too_many_arguments)]
        pub(crate) fn create_in(
            directory: PathBuf,
            case_id: &str,
            scenario: &str,
            target_sha256: &str,
            loop_ledger_sha256: &str,
            native_sha256: &str,
            helper_sha256: &str,
        ) -> AppResult<Self> {
            validate_case_id(case_id)?;
            fs::create_dir(&directory).usage_context(format!(
                "cannot create loop artifact directory {} (overwrite is forbidden)",
                directory.display()
            ))?;
            let events = OpenOptions::new()
                .create_new(true)
                .write(true)
                .open(directory.join("events.jsonl"))
                .usage_context("cannot create loop events")?;
            let manifest = LoopManifest {
                schema_version: 2,
                experiment_identity: LOOP_TOOL_IDENTITY.to_owned(),
                evidence_class: LOOP_EVIDENCE_CLASS.to_owned(),
                case_id: case_id.to_owned(),
                probe: "passive".to_owned(),
                scenario: scenario.to_owned(),
                target_sha256: target_sha256.to_owned(),
                loop_ledger_sha256: loop_ledger_sha256.to_owned(),
                state: "RUNNING".to_owned(),
                active_input_allowed: false,
                native_sha256: native_sha256.to_owned(),
                helper_sha256: helper_sha256.to_owned(),
                main_thread_id: 0,
                hooks_mask: 0,
                overwritten_events: 0,
                target_bytes_before_sha256: String::new(),
                target_bytes_after_sha256: String::new(),
                target_bytes_restored: false,
            };
            let run = Self {
                directory,
                manifest,
                events,
                next_sequence: 1,
            };
            run.write_manifest(false)?;
            Ok(run)
        }

        pub(crate) fn append(&mut self, record: NativeLoopEventRecord) -> AppResult<()> {
            if record.sequence != self.next_sequence {
                return Err(AppError::evidence(format!(
                    "native loop sequence gap: expected {}, got {}",
                    self.next_sequence, record.sequence
                )));
            }
            let event = event_from_native(record)?;
            serde_json::to_writer(&mut self.events, &event)
                .usage_context("cannot serialize loop event")?;
            self.events
                .write_all(b"\n")
                .usage_context("cannot append loop event")?;
            self.next_sequence += 1;
            Ok(())
        }

        fn write_target_bytes(
            &mut self,
            name: &str,
            document: &TargetBytesDocument,
        ) -> AppResult<String> {
            let mut bytes = serde_json::to_vec_pretty(document)
                .usage_context("cannot serialize loop target bytes")?;
            bytes.push(b'\n');
            let mut file = OpenOptions::new()
                .create_new(true)
                .write(true)
                .open(self.directory.join(name))
                .usage_context(format!("cannot create {name}"))?;
            file.write_all(&bytes)
                .usage_context(format!("cannot write {name}"))?;
            Ok(crate::pe::sha256_hex(&bytes))
        }

        pub(crate) fn finish(
            &mut self,
            mapping: &LoopMapping,
            before: &TargetBytesDocument,
            after: &TargetBytesDocument,
        ) -> AppResult<()> {
            self.events
                .flush()
                .usage_context("cannot flush loop events")?;
            let before_sha = self.write_target_bytes("target-bytes-before.json", before)?;
            let after_sha = self.write_target_bytes("target-bytes-after.json", after)?;
            self.manifest.state = "COMPLETE".to_owned();
            self.manifest.main_thread_id = mapping.main_thread_id();
            self.manifest.hooks_mask = LOOP_HOOK_MASK;
            self.manifest.overwritten_events = mapping.overwritten_events();
            self.manifest.target_bytes_before_sha256 = before_sha;
            self.manifest.target_bytes_after_sha256 = after_sha;
            self.manifest.target_bytes_restored = before.entries == after.entries;
            self.write_manifest(true)
        }

        #[allow(clippy::too_many_arguments)]
        pub(crate) fn fail_with_evidence(
            &mut self,
            main_thread_id: u32,
            hooks_mask: u64,
            overwritten_events: u64,
            before: &TargetBytesDocument,
            after: Option<&TargetBytesDocument>,
        ) -> AppResult<()> {
            self.events
                .flush()
                .usage_context("cannot flush failed loop events")?;
            let before_sha = self.write_target_bytes("target-bytes-before.json", before)?;
            let after_sha = after
                .map(|document| self.write_target_bytes("target-bytes-after.json", document))
                .transpose()?;
            self.manifest.state = "FAIL".to_owned();
            self.manifest.main_thread_id = main_thread_id;
            self.manifest.hooks_mask = hooks_mask;
            self.manifest.overwritten_events = overwritten_events;
            self.manifest.target_bytes_before_sha256 = before_sha;
            self.manifest.target_bytes_after_sha256 = after_sha.unwrap_or_default();
            self.manifest.target_bytes_restored =
                after.is_some_and(|document| before.entries == document.entries);
            self.write_manifest(true)
        }

        pub(crate) fn fail(&mut self) {
            self.manifest.state = "FAIL".to_owned();
            let _ = self.write_manifest(true);
        }

        fn write_manifest(&self, replace: bool) -> AppResult<()> {
            let path = self.directory.join("manifest.json");
            let mut options = OpenOptions::new();
            options.write(true);
            if replace {
                options.create(true).truncate(true);
            } else {
                options.create_new(true);
            }
            let mut file = options
                .open(&path)
                .usage_context(format!("cannot write {}", path.display()))?;
            serde_json::to_writer_pretty(&mut file, &self.manifest)
                .usage_context("cannot serialize loop manifest")?;
            file.write_all(b"\n")
                .usage_context("cannot finish loop manifest")
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
                let _ = run_helper(self.helper, "--unload-loop", self.pid, self.native, None);
            }
        }
    }

    #[allow(clippy::too_many_arguments)]
    pub fn run(
        scenario_value: &str,
        game_exe: &Path,
        ledger_path: &Path,
        expected_variant: &str,
        artifacts_root: &Path,
        case_id: &str,
    ) -> AppResult<()> {
        let scenario = Scenario::parse(scenario_value)?;
        let canonical_game = fs::canonicalize(game_exe).usage_context(format!(
            "cannot resolve game executable {}",
            game_exe.display()
        ))?;
        let inspection = inspect_loop(&canonical_game, ledger_path, expected_variant)?;
        let tool = std::env::current_exe().usage_context("cannot locate T0 controller")?;
        let distribution = tool
            .parent()
            .ok_or_else(|| AppError::usage("controller has no parent directory"))?;
        let native = distribution.join("IsaacCsT0.Native.dll");
        let helper = distribution.join("IsaacCsT0.Helper.exe");
        ensure_file(&native, "T0-03R Native DLL")?;
        ensure_file(&helper, "T0-03R x86 helper")?;

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
                "loop-smoke requires exactly one already-running isaac-ng.exe",
            ));
        }
        let process = ObservedProcess::open_exact(pids[0], &canonical_game)?;
        if process.debugger_present()? || !process.forbidden_modules().is_empty() {
            return Err(AppError::evidence(
                "loop-smoke refuses a debugged or instrumented target",
            ));
        }
        let before = capture_target_bytes(&process)?;
        let mut run = LoopRun::create(
            artifacts_root,
            case_id,
            scenario_value,
            &inspection.file_sha256,
            &inspection.ledger_sha256,
            &file_sha256(&native)?,
            &file_sha256(&helper)?,
        )?;
        let result = observe(
            &mut run,
            &process,
            scenario,
            &canonical_game,
            &native,
            &helper,
            &before,
        );
        if let Err(error) = result {
            run.fail();
            return Err(error);
        }
        let report = analyze_loop_run(&run.directory)?;
        println!(
            "{}",
            serde_json::to_string_pretty(&report)
                .usage_context("cannot serialize loop analysis")?
        );
        if report.result == "PASS" {
            Ok(())
        } else {
            Err(AppError::evidence("T0-03R loop analysis failed"))
        }
    }

    fn observe(
        run: &mut LoopRun,
        process: &ObservedProcess,
        scenario: Scenario,
        canonical_game: &Path,
        native: &Path,
        helper: &Path,
        before: &TargetBytesDocument,
    ) -> AppResult<()> {
        let nonce = entropy(process.pid, scenario as u32)?;
        let mapping_name = format!(
            "Local\\IsaacCsT0.Loop.v{}.{}.{}",
            LOOP_IPC_VERSION,
            process.pid,
            &hex::encode_upper(nonce)[..16]
        );
        let mapping = LoopMapping::create(mapping_name, process.pid, scenario as u32, nonce)?;
        let mut next = 0_u64;
        if let Err(helper_error) = run_helper(
            helper,
            "--inject-loop",
            process.pid,
            native,
            Some(mapping.name()),
        ) {
            let diagnostic = drain(run, &mapping, &mut next)?;
            let state = mapping.state()?;
            let error_code = mapping.error_code();
            let after = if process.wait(0)? {
                None
            } else {
                Some(capture_target_bytes(process)?)
            };
            run.fail_with_evidence(
                mapping.main_thread_id(),
                mapping.hooks_mask(),
                mapping.overwritten_events(),
                before,
                after.as_ref(),
            )?;
            return Err(AppError::evidence(format!(
                "{helper_error}; Native mapping state={state:?} errorCode={error_code} {}",
                format_install_failure(diagnostic)
            )));
        }
        let mut guard = InjectionGuard {
            helper,
            native,
            pid: process.pid,
            armed: true,
        };
        let capture_result = capture_window(run, process, &mapping, &mut next);

        // From this point onward cleanup is explicit. A helper failure is not retried blindly:
        // the mapping and target snapshot are preserved so a human can diagnose the state.
        guard.disarm();
        let mut failures = Vec::new();
        match capture_result {
            Ok(CaptureStopReason::User) => {}
            Ok(CaptureStopReason::BoundedWindow) => println!(
                "T0-03R bounded ten-minute window completed; stopping and restoring hooks."
            ),
            Err(error) => failures.push(format!("capture failed: {error}")),
        }

        let target_exited = match process.wait(0) {
            Ok(exited) => exited,
            Err(error) => {
                failures.push(format!("cannot query target before restoration: {error}"));
                false
            }
        };
        let unload_succeeded = if target_exited {
            failures.push("target exited before loop hooks could be restored".to_owned());
            false
        } else {
            match run_helper(helper, "--unload-loop", process.pid, native, None) {
                Ok(()) => true,
                Err(error) => {
                    failures.push(format!(
                        "explicit loop unload failed; automatic retry is forbidden: {error}"
                    ));
                    false
                }
            }
        };

        if unload_succeeded {
            let stop_deadline = Instant::now() + READY_TIMEOUT;
            loop {
                if let Err(error) = drain(run, &mapping, &mut next) {
                    failures.push(format!("cannot drain restored loop events: {error}"));
                    break;
                }
                match mapping.state() {
                    Ok(LoopNativeState::Stopped) => break,
                    Ok(_) if Instant::now() >= stop_deadline => {
                        failures.push(
                            "T0-03R Native did not publish Stopped after restoration".to_owned(),
                        );
                        break;
                    }
                    Ok(_) => thread::sleep(Duration::from_millis(5)),
                    Err(error) => {
                        failures.push(format!(
                            "cannot read Native state after restoration: {error}"
                        ));
                        break;
                    }
                }
            }
        } else if let Err(error) = drain(run, &mapping, &mut next) {
            failures.push(format!(
                "cannot drain failure events after unload error: {error}"
            ));
        }

        let mut after = None;
        if !target_exited {
            match ObservedProcess::open_exact(process.pid, canonical_game) {
                Ok(refreshed) if refreshed.creation_time_100ns == process.creation_time_100ns => {
                    match capture_target_bytes(&refreshed) {
                        Ok(document) => after = Some(document),
                        Err(error) => failures.push(format!(
                            "cannot capture target bytes after restoration attempt: {error}"
                        )),
                    }
                }
                Ok(_) => failures.push("target PID was reused during loop capture".to_owned()),
                Err(error) => failures.push(format!(
                    "cannot reopen exact target after restoration attempt: {error}"
                )),
            }
        }
        if after
            .as_ref()
            .is_some_and(|document| before.entries != document.entries)
        {
            failures.push("T0-03R target call/IAT/vtable bytes were not fully restored".to_owned());
        }

        if failures.is_empty() {
            let after = after.as_ref().ok_or_else(|| {
                AppError::evidence("restored capture completed without an after snapshot")
            })?;
            return run.finish(&mapping, before, after);
        }

        run.fail_with_evidence(
            mapping.main_thread_id(),
            mapping.hooks_mask(),
            mapping.overwritten_events(),
            before,
            after.as_ref(),
        )?;
        Err(AppError::evidence(failures.join("; ")))
    }

    fn capture_window(
        run: &mut LoopRun,
        process: &ObservedProcess,
        mapping: &LoopMapping,
        next: &mut u64,
    ) -> AppResult<CaptureStopReason> {
        let ready_deadline = Instant::now() + READY_TIMEOUT;
        loop {
            let _ = drain(run, mapping, next)?;
            match mapping.state()? {
                LoopNativeState::Ready => break,
                LoopNativeState::Failed | LoopNativeState::Unsafe => {
                    return Err(AppError::evidence(format!(
                        "T0-03R Native initialization failed with code {}",
                        mapping.error_code()
                    )));
                }
                _ if Instant::now() >= ready_deadline => {
                    return Err(AppError::evidence(
                        "T0-03R Native did not become ready within 15 seconds",
                    ));
                }
                _ => thread::sleep(Duration::from_millis(5)),
            }
        }
        if mapping.hooks_mask() != LOOP_HOOK_MASK {
            return Err(AppError::evidence(format!(
                "T0-03R Native hook mask is incomplete: 0x{:X}",
                mapping.hooks_mask()
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
            "READY T0-03R passive {} capture is active; press Enter to stop and restore (automatic bounded stop after ten minutes).",
            run.manifest.scenario
        );
        let started = Instant::now();
        loop {
            if process.wait(0)? {
                return Err(AppError::evidence(
                    "target exited before loop hooks could be restored",
                ));
            }
            let _ = drain(run, mapping, next)?;
            if mapping.overwritten_events() != 0 {
                return Err(AppError::evidence(
                    "T0-03R event ring reported an overwrite",
                ));
            }
            if matches!(
                mapping.state()?,
                LoopNativeState::Failed | LoopNativeState::Unsafe
            ) {
                return Err(AppError::evidence(format!(
                    "T0-03R Native failed closed with code {}",
                    mapping.error_code()
                )));
            }
            if let Some(reason) =
                capture_stop_reason(stop_rx.try_recv().is_ok(), started.elapsed(), MAX_CAPTURE)
            {
                return Ok(reason);
            }
            thread::sleep(Duration::from_millis(5));
        }
    }

    fn capture_stop_reason(
        user_requested: bool,
        elapsed: Duration,
        limit: Duration,
    ) -> Option<CaptureStopReason> {
        if user_requested {
            Some(CaptureStopReason::User)
        } else if elapsed >= limit {
            Some(CaptureStopReason::BoundedWindow)
        } else {
            None
        }
    }

    pub(crate) fn drain(
        run: &mut LoopRun,
        mapping: &LoopMapping,
        next: &mut u64,
    ) -> AppResult<Option<LoopInstallFailureDiagnostic>> {
        let mut diagnostic = None;
        for record in mapping.drain(next)? {
            if record.event.kind == LoopNativeEventKind::Failure as u32
                && record.event.action != LOOP_NO_ACTION
            {
                diagnostic = Some(LoopInstallFailureDiagnostic {
                    stage: record.event.action,
                    index: record.event.value_bits,
                    win32_error: record.event.device_key,
                });
            }
            run.append(record)?;
        }
        Ok(diagnostic)
    }

    fn event_from_native(record: NativeLoopEventRecord) -> AppResult<LoopEvent> {
        let event = record.event;
        let kind = LoopNativeEventKind::from_raw(event.kind)?;
        let phase = LoopPhase::from_raw(event.phase)?;
        let action_input = matches!(
            kind,
            LoopNativeEventKind::InputPressed
                | LoopNativeEventKind::InputTriggered
                | LoopNativeEventKind::InputScalar
        );
        let device_event = action_input
            || matches!(
                kind,
                LoopNativeEventKind::InputDeviceUpdateEnter
                    | LoopNativeEventKind::InputDeviceUpdateExit
                    | LoopNativeEventKind::InputPhysicalPollEnter
                    | LoopNativeEventKind::InputPhysicalPollExit
            );
        let device_kind = if device_event {
            match (
                event.flags & LOOP_FLAG_KEYBOARD_DEVICE != 0,
                event.flags & LOOP_FLAG_GAMEPAD_DEVICE != 0,
            ) {
                (true, false) => Some("KEYBOARD".to_owned()),
                (false, true) => Some("GAMEPAD".to_owned()),
                _ => None,
            }
        } else {
            None
        };
        let install_diagnostic = (kind == LoopNativeEventKind::Failure
            && event.action != LOOP_NO_ACTION)
            .then_some(LoopInstallFailureDiagnostic {
                stage: event.action,
                index: event.value_bits,
                win32_error: event.device_key,
            });
        Ok(LoopEvent {
            sequence: record.sequence,
            native_sequence: record.sequence,
            qpc: event.qpc,
            loop_iteration: event.loop_iteration,
            phase: serde_json::to_value(phase)
                .usage_context("cannot encode loop phase")?
                .as_str()
                .unwrap_or_default()
                .to_owned(),
            kind: serde_json::to_value(kind)
                .usage_context("cannot encode loop event kind")?
                .as_str()
                .unwrap_or_default()
                .to_owned(),
            thread_id: event.thread_id,
            action: (action_input && event.action != LOOP_NO_ACTION).then_some(event.action),
            value_bits: matches!(
                kind,
                LoopNativeEventKind::InputScalar
                    | LoopNativeEventKind::DispatchMessage
                    | LoopNativeEventKind::AlternateOwnerEnter
                    | LoopNativeEventKind::AlternateOwnerExit
            )
            .then_some(event.value_bits),
            flags: event.flags,
            return_rva: format!("0x{:08X}", event.return_rva),
            original_calls: event.original_calls,
            error_code: event.error_code,
            device_key: device_kind.as_ref().map(|_| event.device_key),
            device_kind,
            install_failure_stage: install_diagnostic.map(|value| value.stage),
            install_failure_index: install_diagnostic.map(|value| value.index),
            install_failure_win32_error: install_diagnostic.map(|value| value.win32_error),
        })
    }

    fn install_failure_stage_name(stage: u32) -> &'static str {
        match stage {
            1 => "CALL_TARGET",
            2 => "IAT_LOOKUP",
            3 => "IAT_RVA",
            4 => "IAT_OWNER",
            5 => "VTABLE_COL",
            6 => "VTABLE_QUERY",
            33 => "PATCH_INVALID_ARGUMENTS",
            34 => "PATCH_PREPARE_RELATIVE",
            35 => "PATCH_PREPARE_POINTER",
            36 => "THREAD_SNAPSHOT",
            37 => "THREAD_FIRST",
            38 => "THREAD_OPEN",
            39 => "THREAD_SUSPEND",
            40 => "THREAD_CAPACITY",
            41 => "PATCH_APPLY_RELATIVE",
            42 => "PATCH_APPLY_POINTER",
            43 => "PATCH_ROLLBACK_POINTER",
            44 => "PATCH_ROLLBACK_RELATIVE",
            _ => "UNKNOWN",
        }
    }

    fn format_install_failure(diagnostic: Option<LoopInstallFailureDiagnostic>) -> String {
        diagnostic.map_or_else(
            || "installDiagnostic=NONE".to_owned(),
            |value| {
                format!(
                    "installDiagnostic={} stage={} index={} win32Error={}",
                    install_failure_stage_name(value.stage),
                    value.stage,
                    value.index,
                    value.win32_error
                )
            },
        )
    }

    pub(crate) fn capture_target_bytes(
        process: &ObservedProcess,
    ) -> AppResult<TargetBytesDocument> {
        let base = process.main_module.base;
        let mut entries = BTreeMap::new();
        for (rva, size) in CALL_SITES {
            entries.insert(
                format!("call:0x{rva:08X}"),
                hex::encode_upper(process.read_memory(base + u64::from(*rva), *size)?),
            );
        }
        for rva in POINTER_SLOTS {
            entries.insert(
                format!("pointer:0x{rva:08X}"),
                hex::encode_upper(process.read_memory(base + u64::from(*rva), 4)?),
            );
        }
        Ok(TargetBytesDocument { entries })
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
            .usage_context("cannot run independent T0-03R helper")?;
        let stdout = String::from_utf8_lossy(&output.stdout);
        let stderr = String::from_utf8_lossy(&output.stderr);
        if !output.status.success() || !stdout.trim_start().starts_with("PASS") {
            return Err(AppError::evidence(format!(
                "T0-03R helper {operation} failed (exit={:?}): {}{}",
                output.status.code(),
                stdout.trim(),
                stderr.trim()
            )));
        }
        Ok(())
    }

    pub(crate) fn entropy(pid: u32, scenario: u32) -> AppResult<[u8; 32]> {
        let time = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map_err(|_| AppError::evidence("system clock is before Unix epoch"))?
            .as_nanos()
            .to_le_bytes();
        let qpc = win32::qpc_now()?.to_le_bytes();
        let mut hasher = Sha256::new();
        hasher.update(pid.to_le_bytes());
        hasher.update(scenario.to_le_bytes());
        hasher.update(time);
        hasher.update(qpc);
        Ok(hasher.finalize().into())
    }

    fn validate_case_id(value: &str) -> AppResult<()> {
        if value.is_empty()
            || value.len() > 64
            || !value
                .bytes()
                .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_'))
        {
            return Err(AppError::usage("invalid loop case id"));
        }
        Ok(())
    }

    fn ensure_file(path: &Path, label: &str) -> AppResult<()> {
        if !path.is_file() {
            return Err(AppError::usage(format!(
                "{label} is missing from the controller distribution"
            )));
        }
        Ok(())
    }

    #[cfg(test)]
    mod tests {
        use super::*;
        use crate::loop_ipc::NativeLoopEvent;

        #[test]
        fn v5_capture_snapshot_set_is_exact_unique_and_mask_bound() {
            assert_eq!(CALL_SITES.len(), 27);
            assert!(CALL_SITES.iter().all(|(_, size)| *size == 5));
            assert_eq!(POINTER_SLOTS.len(), 5);
            assert_eq!(CALL_SITES.len() + POINTER_SLOTS.len(), 32);
            assert_eq!(LOOP_HOOK_MASK, (1_u64 << 32) - 1);
            assert_eq!(&CALL_SITES[25..], &[(0x002F_B51C, 5), (0x002F_E37C, 5)]);

            let call_rvas = CALL_SITES
                .iter()
                .map(|(rva, _)| *rva)
                .collect::<std::collections::BTreeSet<_>>();
            let pointer_rvas = POINTER_SLOTS
                .iter()
                .copied()
                .collect::<std::collections::BTreeSet<_>>();
            assert_eq!(call_rvas.len(), CALL_SITES.len());
            assert_eq!(pointer_rvas.len(), POINTER_SLOTS.len());
            assert_eq!(
                POINTER_SLOTS,
                &[
                    0x007A_1678 + 0x78,
                    0x0078_2C80 + 0x78,
                    0x0071_8714,
                    0x0071_8650,
                    0x0071_85B8,
                ]
            );
        }

        #[test]
        fn native_failure_payload_maps_to_dedicated_install_fields() {
            let event = event_from_native(NativeLoopEventRecord {
                sequence: 2,
                event: NativeLoopEvent {
                    published_sequence: 2,
                    qpc: 10,
                    loop_iteration: 0,
                    thread_id: 77,
                    kind: LoopNativeEventKind::Failure as u32,
                    phase: LoopPhase::OutsideEnvelope as u32,
                    action: 38,
                    value_bits: 1234,
                    flags: 0,
                    return_rva: 0,
                    original_calls: 0,
                    error_code: 4,
                    device_key: 5,
                },
            })
            .unwrap();

            assert_eq!(event.action, None);
            assert_eq!(event.value_bits, None);
            assert_eq!(event.device_key, None);
            assert_eq!(event.install_failure_stage, Some(38));
            assert_eq!(event.install_failure_index, Some(1234));
            assert_eq!(event.install_failure_win32_error, Some(5));
            assert_eq!(
                format_install_failure(Some(LoopInstallFailureDiagnostic {
                    stage: 38,
                    index: 1234,
                    win32_error: 5,
                })),
                "installDiagnostic=THREAD_OPEN stage=38 index=1234 win32Error=5"
            );
            assert_eq!(install_failure_stage_name(999), "UNKNOWN");
        }

        #[test]
        fn failed_run_persists_mapping_and_target_snapshots() {
            let root = tempfile::tempdir().unwrap();
            let mut run = LoopRun::create(
                root.path(),
                "diagnostic-regression",
                "keyboard",
                "target",
                "ledger",
                "native",
                "helper",
            )
            .unwrap();
            let before = TargetBytesDocument {
                entries: BTreeMap::from([("call:0x00000001".to_owned(), "AA".to_owned())]),
            };
            let after = TargetBytesDocument {
                entries: before.entries.clone(),
            };
            run.fail_with_evidence(17, 0x12, 0, &before, Some(&after))
                .unwrap();

            let manifest: LoopManifest =
                serde_json::from_slice(&fs::read(run.directory.join("manifest.json")).unwrap())
                    .unwrap();
            assert_eq!(manifest.state, "FAIL");
            assert_eq!(manifest.main_thread_id, 17);
            assert_eq!(manifest.hooks_mask, 0x12);
            assert!(manifest.target_bytes_restored);
            assert!(!manifest.target_bytes_before_sha256.is_empty());
            assert!(!manifest.target_bytes_after_sha256.is_empty());
            assert!(run.directory.join("target-bytes-before.json").is_file());
            assert!(run.directory.join("target-bytes-after.json").is_file());
        }

        #[test]
        fn bounded_capture_window_is_a_normal_stop_reason() {
            assert_eq!(
                capture_stop_reason(false, MAX_CAPTURE, MAX_CAPTURE),
                Some(CaptureStopReason::BoundedWindow)
            );
            assert_eq!(
                capture_stop_reason(true, Duration::ZERO, MAX_CAPTURE),
                Some(CaptureStopReason::User)
            );
            assert_eq!(
                capture_stop_reason(false, MAX_CAPTURE - Duration::from_millis(1), MAX_CAPTURE),
                None
            );
        }
    }
}

#[cfg(windows)]
pub(crate) use live::{
    LoopRun as CompanionLoopRun, capture_target_bytes as capture_loop_target_bytes,
    drain as drain_companion_loop, entropy as loop_entropy,
};

#[cfg(windows)]
#[allow(clippy::too_many_arguments)]
pub fn loop_smoke_command(
    probe: &str,
    scenario: &str,
    game_exe: &Path,
    ledger: &Path,
    expected_variant: &str,
    artifacts_root: &Path,
    case_id: &str,
) -> AppResult<()> {
    if probe != "passive" {
        return Err(AppError::usage(
            "native T0-03R loop capture requires --probe passive; control-no-native remains a disarmed checkpoint",
        ));
    }
    live::run(
        scenario,
        game_exe,
        ledger,
        expected_variant,
        artifacts_root,
        case_id,
    )
}

#[cfg(not(windows))]
#[allow(clippy::too_many_arguments)]
pub fn loop_smoke_command(
    _probe: &str,
    _scenario: &str,
    _game_exe: &Path,
    _ledger: &Path,
    _expected_variant: &str,
    _artifacts_root: &Path,
    _case_id: &str,
) -> AppResult<()> {
    Err(AppError::usage("loop-smoke requires Windows"))
}

use std::path::Path;

use crate::error::{AppError, AppResult};
