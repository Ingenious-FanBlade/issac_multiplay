use std::path::Path;

use serde::Serialize;

use crate::boundary_artifact::{
    analyze_boundary_run, compare_boundary_runs, write_boundary_analysis, write_boundary_comparison,
};
use crate::boundary_ledger::{BoundaryInspectionReport, LoadedBoundaryLedger};
use crate::error::{AppError, AppResult};
use crate::pe::PeImage;

pub fn boundary_inspect_command(
    game_exe: &Path,
    ledger_path: &Path,
    expected_variant: &str,
) -> AppResult<()> {
    let (_, report) = inspect_boundary_static(game_exe, ledger_path, expected_variant)?;
    print_json(&report)
}

pub fn boundary_analyze_command(run_directory: &Path) -> AppResult<()> {
    let report = analyze_boundary_run(run_directory)?;
    write_boundary_analysis(run_directory, &report)?;
    print_json(&report)?;
    if report.result == "PASS" {
        Ok(())
    } else {
        Err(AppError::evidence(format!(
            "T0-03 artifact analysis failed at {}",
            report
                .first_divergence
                .as_deref()
                .unwrap_or("unknown-check")
        )))
    }
}

pub fn boundary_compare_command(
    control: &Path,
    instrumented: &Path,
    output: Option<&Path>,
) -> AppResult<()> {
    let report = compare_boundary_runs(control, instrumented)?;
    let default_output = instrumented.join("boundary-comparison.json");
    write_boundary_comparison(output.unwrap_or(&default_output), &report)?;
    print_json(&report)?;
    if report.result == "PASS" {
        Ok(())
    } else {
        Err(AppError::evidence("T0-03 comparison failed"))
    }
}

pub fn inspect_boundary_static(
    game_exe: &Path,
    ledger_path: &Path,
    expected_variant: &str,
) -> AppResult<(LoadedBoundaryLedger, BoundaryInspectionReport)> {
    let loaded = LoadedBoundaryLedger::read(ledger_path)?;
    if expected_variant != loaded.ledger.fixed_variant {
        return Err(AppError::evidence(format!(
            "T0-03 is fixed to variant {}, got {expected_variant}",
            loaded.ledger.fixed_variant
        )));
    }
    let image = PeImage::read(game_exe)
        .map_err(|error| AppError::evidence(format!("cannot inspect game PE: {error}")))?;
    let baseline =
        loaded
            .baseline
            .ledger
            .inspect(&image, expected_variant, &loaded.baseline.raw_sha256)?;
    let target_report =
        loaded
            .target_ledger()
            .inspect(&image, expected_variant, &loaded.raw_sha256)?;
    let report = loaded.report(baseline, target_report.targets);
    Ok((loaded, report))
}

#[cfg(not(windows))]
#[allow(clippy::too_many_arguments)]
pub fn boundary_smoke_command(
    _probe: &str,
    _scenario: &str,
    _game_exe: &Path,
    _ledger_path: &Path,
    _expected_variant: &str,
    _artifacts_root: &Path,
    _case_id: &str,
) -> AppResult<()> {
    Err(AppError::usage(
        "boundary-smoke is supported only on Windows",
    ))
}

#[cfg(windows)]
mod live {
    use std::fs;
    use std::io;
    use std::process::Command;
    use std::sync::mpsc;
    use std::thread;
    use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

    use sha2::{Digest, Sha256};

    use super::*;
    use crate::artifact::{TargetBytesDocument, utc_now};
    use crate::boundary_artifact::{BoundaryEventLine, BoundaryRun, BoundaryRunInputs};
    use crate::boundary_ipc::{
        BOUNDARY_FLAG_ENTITY_COUNT_VALID, BOUNDARY_FLAG_GAME_FRAME_VALID,
        BOUNDARY_FLAG_ONLINE_FRAME_VALID, BoundaryEventKind, BoundaryMapping, BoundaryNativeState,
        NativeBoundaryEventRecord, UNAVAILABLE_U32,
    };
    use crate::commands::capture_live_snapshot;
    use crate::error::UsageContext;
    use crate::ledger::LoadedLedger;
    use crate::win32::{self, ObservedProcess};

    const WAIT_FOR_PROCESS: Duration = Duration::from_secs(300);
    const WAIT_FOR_NATIVE: Duration = Duration::from_secs(15);
    const MAX_OBSERVATION: Duration = Duration::from_secs(30 * 60);

    #[derive(Clone, Copy, Debug, Eq, PartialEq)]
    enum Probe {
        ControlNoNative,
        Passive,
    }

    impl Probe {
        fn parse(value: &str) -> AppResult<Self> {
            match value {
                "control-no-native" => Ok(Self::ControlNoNative),
                "passive" => Ok(Self::Passive),
                _ => Err(AppError::usage(format!("unknown T0-03 probe {value}"))),
            }
        }

        const fn name(self) -> &'static str {
            match self {
                Self::ControlNoNative => "control-no-native",
                Self::Passive => "passive",
            }
        }
    }

    #[derive(Clone, Copy, Debug)]
    enum Scenario {
        Normal = 1,
        Input = 2,
        Pause = 3,
        LowFps = 4,
        FocusLoss = 5,
        RoomTransition = 6,
        MenuReturn = 7,
        LuaRebuild = 8,
    }

    impl Scenario {
        fn parse(value: &str) -> AppResult<Self> {
            match value {
                "normal" => Ok(Self::Normal),
                "input" => Ok(Self::Input),
                "pause" => Ok(Self::Pause),
                "low-fps" => Ok(Self::LowFps),
                "focus-loss" => Ok(Self::FocusLoss),
                "room-transition" => Ok(Self::RoomTransition),
                "menu-return" => Ok(Self::MenuReturn),
                "lua-rebuild" => Ok(Self::LuaRebuild),
                _ => Err(AppError::usage(format!(
                    "unknown T0-03 boundary scenario {value}"
                ))),
            }
        }

        const fn raw(self) -> u32 {
            self as u32
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
                let _ = run_helper(
                    self.helper,
                    "--unload-boundary",
                    self.pid,
                    self.native,
                    None,
                );
            }
        }
    }

    #[allow(clippy::too_many_arguments)]
    pub fn run(
        probe_value: &str,
        scenario_value: &str,
        game_exe: &Path,
        ledger_path: &Path,
        expected_variant: &str,
        artifacts_root: &Path,
        case_id: &str,
    ) -> AppResult<()> {
        let probe = Probe::parse(probe_value)?;
        let scenario = Scenario::parse(scenario_value)?;
        let canonical_game = fs::canonicalize(game_exe).usage_context(format!(
            "cannot resolve game executable {}",
            game_exe.display()
        ))?;
        let (ledger, inspection) =
            inspect_boundary_static(&canonical_game, ledger_path, expected_variant)?;
        let image = PeImage::read(&canonical_game)
            .map_err(|error| AppError::evidence(format!("cannot inspect game PE: {error}")))?;
        let disk_before = ledger.baseline.ledger.inspect(
            &image,
            expected_variant,
            &ledger.baseline.raw_sha256,
        )?;
        let tool = std::env::current_exe().usage_context("cannot locate T0 controller")?;
        let distribution = tool
            .parent()
            .ok_or_else(|| AppError::usage("controller has no parent directory"))?;
        let native = distribution.join("IsaacCsT0.Native.dll");
        let helper = distribution.join("IsaacCsT0.Helper.exe");
        if probe == Probe::Passive {
            ensure_file(&native, "T0-03 Native DLL")?;
            ensure_file(&helper, "T0-03 x86 helper")?;
        }
        let mut run = BoundaryRun::create(&BoundaryRunInputs {
            artifacts_root,
            case_id,
            probe: probe.name(),
            scenario: scenario_value,
            variant_id: expected_variant,
            game_file_sha256: &inspection.baseline.image.file_sha256,
            game_text_sha256: &inspection.baseline.image.text_sha256,
            boundary_ledger_sha256: &ledger.raw_sha256,
            baseline_ledger_sha256: &ledger.baseline.raw_sha256,
            mode_ledger_sha256: &ledger.mode.raw_sha256,
            gate_decision_sha256: &ledger.gate_sha256,
        })?;
        let result = observe(
            &mut run,
            probe,
            scenario,
            &canonical_game,
            &ledger,
            &image,
            &disk_before,
            &native,
            &helper,
            expected_variant,
        );
        if let Err(error) = result {
            if run.manifest.status == "RUNNING" {
                let _ = run.finish(
                    "FAIL",
                    "T0-03 evidence capture failed; identifying path details are omitted",
                );
            }
            return Err(error);
        }
        let directory = run.directory.clone();
        drop(run);
        let report = analyze_boundary_run(&directory)?;
        write_boundary_analysis(&directory, &report)?;
        print_json(&report)?;
        if report.result == "PASS" {
            Ok(())
        } else {
            Err(AppError::evidence(format!(
                "T0-03 smoke failed at {}",
                report
                    .first_divergence
                    .as_deref()
                    .unwrap_or("unknown-check")
            )))
        }
    }

    #[allow(clippy::too_many_arguments)]
    fn observe(
        run: &mut BoundaryRun,
        probe: Probe,
        scenario: Scenario,
        canonical_game: &Path,
        ledger: &LoadedBoundaryLedger,
        image: &PeImage,
        disk_before: &crate::ledger::StaticInspectionReport,
        native: &Path,
        helper: &Path,
        expected_variant: &str,
    ) -> AppResult<()> {
        if !win32::find_target_pids()?.is_empty() {
            return Err(AppError::evidence(
                "isaac-ng.exe was already running when boundary-smoke started",
            ));
        }
        let blockers = win32::running_blockers()?;
        if !blockers.is_empty() {
            return Err(AppError::evidence(format!(
                "blocked instrumentation process present: {}",
                blockers.join(", ")
            )));
        }
        println!(
            "T0-03 {} / {:?} is waiting for a user-started isaac-ng.exe.",
            probe.name(),
            scenario
        );
        let process = wait_for_exact_process(canonical_game)?;
        let snapshot_ledger = ledger.combined_snapshot_ledger();
        let before_live = wait_for_snapshot(&process, canonical_game, image, &snapshot_ledger)?;
        run.set_process(process.pid, process.creation_time_100ns);
        run.write_target_document(
            "target-bytes-before.json",
            &TargetBytesDocument {
                schema_version: crate::artifact::ARTIFACT_SCHEMA_VERSION,
                phase: "before".to_owned(),
                captured_utc: utc_now(),
                disk: disk_before.clone(),
                process: Some(before_live),
            },
        )?;

        let (stop_tx, stop_rx) = mpsc::channel();
        thread::spawn(move || {
            loop {
                let mut line = String::new();
                if io::stdin().read_line(&mut line).unwrap_or(0) == 0 || stop_tx.send(()).is_err() {
                    break;
                }
            }
        });
        println!(
            "Prepare the {:?} precondition, then press Enter to start the measured window.",
            scenario
        );

        let nonce = entropy(process.pid, probe.name(), scenario.raw())?;
        let mapping_name = format!(
            "Local\\IsaacCsT0.Boundary.v3.{}.{}",
            process.pid,
            &hex::encode_upper(nonce)[..16]
        );
        let mapping = if probe == Probe::Passive {
            Some(BoundaryMapping::create(
                mapping_name,
                process.pid,
                scenario.raw(),
                nonce,
            )?)
        } else {
            None
        };
        let mut native_next = 0_u64;
        let mut guard = None;
        if let Some(mapping) = mapping.as_ref() {
            if let Err(error) = run_helper(
                helper,
                "--inject-boundary",
                process.pid,
                native,
                Some(mapping.name()),
            ) {
                let _ = run_helper(helper, "--unload-boundary", process.pid, native, None);
                return Err(error);
            }
            guard = Some(InjectionGuard {
                helper,
                native,
                pid: process.pid,
                armed: true,
            });
            let deadline = Instant::now() + WAIT_FOR_NATIVE;
            loop {
                let _ = drain_native(run, mapping, &mut native_next)?;
                match mapping.state()? {
                    BoundaryNativeState::Ready => break,
                    BoundaryNativeState::Failed | BoundaryNativeState::Unsafe => {
                        return Err(AppError::evidence(format!(
                            "T0-03 Native initialization failed with code {}",
                            mapping.error_code()
                        )));
                    }
                    _ if Instant::now() >= deadline => {
                        return Err(AppError::timeout(
                            "T0-03 Native did not become ready within 15 seconds",
                        ));
                    }
                    _ => thread::sleep(Duration::from_millis(10)),
                }
            }
            if mapping.hooks_mask() != 0xFFFF {
                return Err(AppError::evidence(format!(
                    "T0-03 Native hook mask is incomplete: 0x{:X}",
                    mapping.hooks_mask()
                )));
            }
        }

        let started = Instant::now();
        let mut runtime_failure = None;
        loop {
            if process.wait(0)? {
                return Err(AppError::evidence(
                    "target exited before the measured window could start",
                ));
            }
            let retired = if let Some(mapping) = mapping.as_ref() {
                let retired = drain_native(run, mapping, &mut native_next)?;
                if mapping.overwritten_events() != 0 {
                    runtime_failure = Some(AppError::evidence(
                        "T0-03 native event ring reported an overwrite",
                    ));
                    true
                } else if matches!(
                    mapping.state()?,
                    BoundaryNativeState::Failed | BoundaryNativeState::Unsafe
                ) {
                    runtime_failure = Some(AppError::evidence(format!(
                        "T0-03 Native disarmed with code {}",
                        mapping.error_code()
                    )));
                    true
                } else {
                    retired
                }
            } else {
                false
            };
            if retired {
                if runtime_failure.is_none() {
                    runtime_failure = Some(AppError::evidence(
                        "lifecycle retired before the measured window started",
                    ));
                }
                break;
            }
            if stop_rx.try_recv().is_ok() {
                break;
            }
            if started.elapsed() >= MAX_OBSERVATION {
                return Err(AppError::timeout(
                    "T0-03 precondition wait exceeded the 30 minute limit",
                ));
            }
            thread::sleep(Duration::from_millis(10));
        }
        run.append(&controller_marker(BoundaryEventKind::WindowStart)?)?;
        if runtime_failure.is_none() {
            println!(
                "T0-03 {:?} measured window is active. Press Enter to stop and restore while the game remains open.",
                scenario
            );
        }
        loop {
            if runtime_failure.is_some() {
                break;
            }
            if process.wait(0)? {
                return Err(AppError::evidence(
                    "target exited before boundary hook restoration could be proven",
                ));
            }
            let retired = if let Some(mapping) = mapping.as_ref() {
                let retired = drain_native(run, mapping, &mut native_next)?;
                if mapping.overwritten_events() != 0 {
                    runtime_failure = Some(AppError::evidence(
                        "T0-03 native event ring reported an overwrite",
                    ));
                    true
                } else if matches!(
                    mapping.state()?,
                    BoundaryNativeState::Failed | BoundaryNativeState::Unsafe
                ) {
                    runtime_failure = Some(AppError::evidence(format!(
                        "T0-03 Native disarmed with code {}",
                        mapping.error_code()
                    )));
                    true
                } else {
                    retired
                }
            } else {
                false
            };
            if retired || stop_rx.try_recv().is_ok() {
                break;
            }
            if started.elapsed() >= MAX_OBSERVATION {
                return Err(AppError::timeout(
                    "T0-03 observation exceeded the 30 minute limit",
                ));
            }
            thread::sleep(Duration::from_millis(10));
        }
        run.append(&controller_marker(BoundaryEventKind::WindowEnd)?)?;

        if let Some(mapping) = mapping.as_ref() {
            run_helper(helper, "--unload-boundary", process.pid, native, None)?;
            let deadline = Instant::now() + WAIT_FOR_NATIVE;
            loop {
                let _ = drain_native(run, mapping, &mut native_next)?;
                if mapping.state()? == BoundaryNativeState::Stopped {
                    break;
                }
                if Instant::now() >= deadline {
                    return Err(AppError::timeout(
                        "T0-03 Native did not publish Stopped within 15 seconds",
                    ));
                }
                thread::sleep(Duration::from_millis(10));
            }
            if let Some(guard) = guard.as_mut() {
                guard.disarm();
            }
        }
        drop(guard);
        drop(mapping);

        let refreshed = ObservedProcess::open_exact(process.pid, canonical_game)?;
        if refreshed.creation_time_100ns != process.creation_time_100ns {
            return Err(AppError::evidence(
                "target PID was reused during boundary smoke",
            ));
        }
        let after_live = wait_for_snapshot(&refreshed, canonical_game, image, &snapshot_ledger)?;
        let disk_after =
            ledger
                .baseline
                .ledger
                .inspect(image, expected_variant, &ledger.baseline.raw_sha256)?;
        run.write_target_document(
            "target-bytes-after.json",
            &TargetBytesDocument {
                schema_version: crate::artifact::ARTIFACT_SCHEMA_VERSION,
                phase: "after".to_owned(),
                captured_utc: utc_now(),
                disk: disk_after,
                process: Some(after_live),
            },
        )?;
        if let Some(error) = runtime_failure {
            run.finish(
                "FAIL",
                "T0-03 fail-closed signal observed and hooks restored",
            )?;
            return Err(error);
        }
        run.finish(
            "PASS",
            "T0-03 boundary evidence captured and hooks restored",
        )
    }

    fn wait_for_exact_process(path: &Path) -> AppResult<ObservedProcess> {
        let started = Instant::now();
        loop {
            let blockers = win32::running_blockers()?;
            if !blockers.is_empty() {
                return Err(AppError::evidence(format!(
                    "blocked process appeared: {}",
                    blockers.join(", ")
                )));
            }
            let pids = win32::find_target_pids()?;
            if pids.len() > 1 {
                return Err(AppError::evidence(
                    "more than one isaac-ng.exe process is visible",
                ));
            }
            if let Some(pid) = pids.first() {
                return ObservedProcess::open_exact(*pid, path);
            }
            if started.elapsed() >= WAIT_FOR_PROCESS {
                return Err(AppError::timeout(
                    "no user-started target appeared within 300 seconds",
                ));
            }
            thread::sleep(Duration::from_millis(100));
        }
    }

    fn wait_for_snapshot(
        process: &ObservedProcess,
        path: &Path,
        image: &PeImage,
        ledger: &LoadedLedger,
    ) -> AppResult<crate::artifact::LiveProcessSnapshot> {
        let deadline = Instant::now() + Duration::from_secs(30);
        loop {
            let refreshed = ObservedProcess::open_exact(process.pid, path)?;
            if refreshed.creation_time_100ns != process.creation_time_100ns {
                return Err(AppError::evidence(
                    "target PID was reused while waiting for target bytes",
                ));
            }
            match capture_live_snapshot(&refreshed, image, ledger) {
                Ok(snapshot) => return Ok(snapshot),
                Err(error) if Instant::now() < deadline => {
                    if refreshed.wait(0)? {
                        return Err(AppError::evidence(
                            "target exited before a live snapshot was captured",
                        ));
                    }
                    let _ = error;
                    thread::sleep(Duration::from_millis(100));
                }
                Err(error) => return Err(error),
            }
        }
    }

    fn drain_native(
        run: &mut BoundaryRun,
        mapping: &BoundaryMapping,
        next: &mut u64,
    ) -> AppResult<bool> {
        let mut retired = false;
        for record in mapping.drain(next)? {
            retired |= record.event.kind == BoundaryEventKind::Retired as u32;
            run.append(&event_from_native(record)?)?;
        }
        Ok(retired)
    }

    fn event_from_native(record: NativeBoundaryEventRecord) -> AppResult<BoundaryEventLine> {
        let event = record.event;
        let optional = |value: u32, flag: u32| {
            if event.flags & flag != 0 && value != UNAVAILABLE_U32 {
                Some(value)
            } else {
                None
            }
        };
        Ok(BoundaryEventLine {
            schema_version: crate::boundary_artifact::BOUNDARY_ARTIFACT_SCHEMA_VERSION,
            event_sequence: 0,
            native_sequence: Some(record.sequence),
            qpc: event.qpc,
            generation: event.generation,
            thread_id: event.thread_id,
            kind: BoundaryEventKind::from_raw(event.kind)?,
            depth: event.depth,
            flags: event.flags,
            return_rva: format!("0x{:08X}", event.return_rva),
            game_frame_before: optional(event.game_frame_before, BOUNDARY_FLAG_GAME_FRAME_VALID),
            game_frame_after: optional(event.game_frame_after, BOUNDARY_FLAG_GAME_FRAME_VALID),
            online_frame_before: optional(
                event.online_frame_before,
                BOUNDARY_FLAG_ONLINE_FRAME_VALID,
            ),
            online_frame_after: optional(
                event.online_frame_after,
                BOUNDARY_FLAG_ONLINE_FRAME_VALID,
            ),
            entity_count_before: optional(
                event.entity_count_before,
                BOUNDARY_FLAG_ENTITY_COUNT_VALID,
            ),
            entity_count_after: optional(
                event.entity_count_after,
                BOUNDARY_FLAG_ENTITY_COUNT_VALID,
            ),
            original_calls: event.original_calls,
            result_value: event.result_value,
            summary_before: format!("{:016X}", event.summary_before),
            summary_after: format!("{:016X}", event.summary_after),
            error_code: event.error_code,
        })
    }

    fn controller_marker(kind: BoundaryEventKind) -> AppResult<BoundaryEventLine> {
        Ok(BoundaryEventLine {
            schema_version: crate::boundary_artifact::BOUNDARY_ARTIFACT_SCHEMA_VERSION,
            event_sequence: 0,
            native_sequence: None,
            qpc: win32::qpc_now()?,
            generation: 0,
            thread_id: win32::current_thread_id(),
            kind,
            depth: 0,
            flags: 0,
            return_rva: "0x00000000".to_owned(),
            game_frame_before: None,
            game_frame_after: None,
            online_frame_before: None,
            online_frame_after: None,
            entity_count_before: None,
            entity_count_after: None,
            original_calls: 0,
            result_value: 0,
            summary_before: "0000000000000000".to_owned(),
            summary_after: "0000000000000000".to_owned(),
            error_code: 0,
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
            .usage_context("cannot run independent T0-03 helper")?;
        let stdout = String::from_utf8_lossy(&output.stdout);
        let stderr = String::from_utf8_lossy(&output.stderr);
        if !output.status.success() || !stdout.trim_start().starts_with("PASS") {
            return Err(AppError::evidence(format!(
                "T0-03 helper {operation} failed (exit={:?}): {}{}",
                output.status.code(),
                stdout.trim(),
                stderr.trim()
            )));
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

    fn entropy(pid: u32, probe: &str, scenario: u32) -> AppResult<[u8; 32]> {
        let time = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map_err(|_| AppError::evidence("system clock is before Unix epoch"))?
            .as_nanos()
            .to_le_bytes();
        let qpc = win32::qpc_now()?.to_le_bytes();
        let mut hasher = Sha256::new();
        hasher.update(pid.to_le_bytes());
        hasher.update(time);
        hasher.update(qpc);
        hasher.update(probe.as_bytes());
        hasher.update(scenario.to_le_bytes());
        Ok(hasher.finalize().into())
    }
}

#[cfg(windows)]
#[allow(clippy::too_many_arguments)]
pub fn boundary_smoke_command(
    probe: &str,
    scenario: &str,
    game_exe: &Path,
    ledger_path: &Path,
    expected_variant: &str,
    artifacts_root: &Path,
    case_id: &str,
) -> AppResult<()> {
    live::run(
        probe,
        scenario,
        game_exe,
        ledger_path,
        expected_variant,
        artifacts_root,
        case_id,
    )
}

fn print_json(value: &impl Serialize) -> AppResult<()> {
    println!(
        "{}",
        serde_json::to_string_pretty(value)
            .map_err(|error| AppError::usage(format!("cannot serialize output: {error}")))?
    );
    Ok(())
}
