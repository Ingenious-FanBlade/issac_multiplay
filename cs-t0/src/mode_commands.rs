use std::path::Path;

use serde::Serialize;

use crate::commands::inspect_static;
use crate::error::{AppError, AppResult, UsageContext};
use crate::mode_artifact::{
    analyze_mode_run, compare_mode_runs, write_mode_analysis, write_mode_comparison,
};
use crate::mode_ledger::{LoadedModeLedger, ModeInspectionReport};

pub fn mode_inspect_command(
    game_exe: &Path,
    mode_ledger_path: &Path,
    expected_variant: &str,
) -> AppResult<()> {
    let (_, report) = inspect_mode_static(game_exe, mode_ledger_path, expected_variant)?;
    print_json(&report)
}

pub fn mode_analyze_command(run_directory: &Path) -> AppResult<()> {
    let report = analyze_mode_run(run_directory)?;
    write_mode_analysis(run_directory, &report)?;
    print_json(&report)?;
    if report.result == "PASS" {
        Ok(())
    } else {
        Err(AppError::evidence(format!(
            "T0-02 artifact analysis failed at {}",
            report
                .first_divergence
                .as_deref()
                .unwrap_or("unknown-check")
        )))
    }
}

pub fn mode_compare_command(
    control: &Path,
    instrumented: &Path,
    peer: Option<&Path>,
    output: Option<&Path>,
) -> AppResult<()> {
    let report = compare_mode_runs(control, instrumented, peer)?;
    let default_output = instrumented.join("mode-comparison.json");
    write_mode_comparison(output.unwrap_or(&default_output), &report)?;
    print_json(&report)?;
    if report.result == "PASS" {
        Ok(())
    } else {
        Err(AppError::evidence(format!(
            "T0-02 run comparison failed at {}",
            report
                .first_divergence
                .as_deref()
                .unwrap_or("unknown-check")
        )))
    }
}

fn inspect_mode_static(
    game_exe: &Path,
    mode_ledger_path: &Path,
    expected_variant: &str,
) -> AppResult<(LoadedModeLedger, ModeInspectionReport)> {
    let mode = LoadedModeLedger::read(mode_ledger_path)?;
    if expected_variant != mode.ledger.fixed_variant {
        return Err(AppError::evidence(format!(
            "T0-02 is fixed to variant {}, got {expected_variant}",
            mode.ledger.fixed_variant
        )));
    }
    let (_, image, baseline_report) =
        inspect_static(game_exe, &mode.baseline_path, expected_variant)?;
    if baseline_report.ledger_sha256 != mode.baseline.raw_sha256 {
        return Err(AppError::evidence(
            "baseline ledger changed between mode-ledger validation and PE inspection",
        ));
    }
    let candidate_report =
        mode.target_ledger()
            .inspect(&image, expected_variant, &mode.raw_sha256)?;
    let report = mode.report(baseline_report, candidate_report.targets);
    Ok((mode, report))
}

#[cfg(not(windows))]
#[allow(clippy::too_many_arguments)]
pub fn mode_smoke_command(
    _scenario: &str,
    _endpoint: &str,
    _game_exe: &Path,
    _mode_ledger_path: &Path,
    _expected_variant: &str,
    _artifacts_root: &Path,
    _case_id: &str,
) -> AppResult<()> {
    Err(AppError::usage("mode-smoke is supported only on Windows"))
}

#[cfg(windows)]
mod live {
    use std::io;
    use std::process::Command;
    use std::sync::mpsc;
    use std::thread;
    use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

    use sha2::{Digest, Sha256};

    use super::*;
    use crate::artifact::{TargetBytesDocument, file_sha256, filetime_100ns_to_utc, utc_now};
    use crate::commands::capture_live_snapshot;
    use crate::mode_artifact::{ModeArtifactRun, ModeEvent};
    use crate::mode_ipc::{
        EndpointRole, ModeMapping, NativeScenario, NativeState, PAIRING_KEY_ENV, parse_pairing_key,
    };
    use crate::pe::PeImage;
    use crate::win32::{self, ObservedProcess};

    const WAIT_FOR_PROCESS: Duration = Duration::from_secs(300);
    const WAIT_FOR_NATIVE: Duration = Duration::from_secs(15);
    const MAX_OBSERVATION: Duration = Duration::from_secs(30 * 60);

    #[derive(Clone, Copy, Debug, Eq, PartialEq)]
    enum Scenario {
        ControlNoNative,
        OfflineDisarmed,
        OriginalOnline,
    }

    struct InjectionGuard<'a> {
        helper: &'a Path,
        native: &'a Path,
        pid: u32,
        mapping: &'a ModeMapping,
        armed: bool,
    }

    impl InjectionGuard<'_> {
        fn disarm(&mut self) {
            self.armed = false;
        }
    }

    impl Drop for InjectionGuard<'_> {
        fn drop(&mut self) {
            if !self.armed {
                return;
            }
            let state = self.mapping.state().ok();
            if state != Some(NativeState::Unsafe) {
                let _ = run_helper(self.helper, "--unload", self.pid, self.native, None);
            }
        }
    }

    impl Scenario {
        fn parse(value: &str) -> AppResult<Self> {
            match value {
                "control-no-native" => Ok(Self::ControlNoNative),
                "offline-disarmed" => Ok(Self::OfflineDisarmed),
                "original-online" => Ok(Self::OriginalOnline),
                _ => Err(AppError::usage(format!("unknown T0-02 scenario {value}"))),
            }
        }

        const fn name(self) -> &'static str {
            match self {
                Self::ControlNoNative => "control-no-native",
                Self::OfflineDisarmed => "offline-disarmed",
                Self::OriginalOnline => "original-online",
            }
        }

        const fn native(self) -> Option<NativeScenario> {
            match self {
                Self::ControlNoNative => None,
                Self::OfflineDisarmed => Some(NativeScenario::OfflineDisarmed),
                Self::OriginalOnline => Some(NativeScenario::OriginalOnline),
            }
        }

        const fn expected_mode(self) -> &'static str {
            match self {
                Self::ControlNoNative => "Unresolved",
                Self::OfflineDisarmed => "Offline",
                Self::OriginalOnline => "OriginalOnline",
            }
        }

        const fn requested_mode_raw(self) -> u32 {
            match self {
                Self::ControlNoNative => 0,
                Self::OfflineDisarmed => 1,
                Self::OriginalOnline => 2,
            }
        }
    }

    fn parse_endpoint(value: &str, scenario: Scenario) -> AppResult<EndpointRole> {
        let endpoint = match value {
            "solo" => EndpointRole::Solo,
            "a" => EndpointRole::A,
            "b" => EndpointRole::B,
            _ => return Err(AppError::usage(format!("unknown endpoint role {value}"))),
        };
        if scenario == Scenario::OriginalOnline && endpoint == EndpointRole::Solo {
            return Err(AppError::usage(
                "original-online requires endpoint role a or b",
            ));
        }
        if scenario == Scenario::OfflineDisarmed && endpoint != EndpointRole::Solo {
            return Err(AppError::usage(
                "offline-disarmed requires endpoint role solo",
            ));
        }
        Ok(endpoint)
    }

    #[allow(clippy::too_many_arguments)]
    pub fn run(
        scenario_value: &str,
        endpoint_value: &str,
        game_exe: &Path,
        mode_ledger_path: &Path,
        expected_variant: &str,
        artifacts_root: &Path,
        case_id: &str,
    ) -> AppResult<()> {
        let scenario = Scenario::parse(scenario_value)?;
        let endpoint = parse_endpoint(endpoint_value, scenario)?;
        let canonical_game = std::fs::canonicalize(game_exe).usage_context(format!(
            "cannot resolve game executable {}",
            game_exe.display()
        ))?;
        let (mode_ledger, inspection) =
            inspect_mode_static(&canonical_game, mode_ledger_path, expected_variant)?;
        let (_, image, disk_before) = inspect_static(
            &canonical_game,
            &mode_ledger.baseline_path,
            expected_variant,
        )?;

        let tool = std::env::current_exe().usage_context("cannot locate T0 controller")?;
        let distribution = tool
            .parent()
            .ok_or_else(|| AppError::usage("controller has no parent directory"))?;
        let native = distribution.join("IsaacCsT0.Native.dll");
        let helper = distribution.join("IsaacCsT0.Helper.exe");
        if scenario.native().is_some() {
            ensure_file(&native, "T0-02 Native DLL")?;
            ensure_file(&helper, "T0-02 x86 helper")?;
        }
        let mut run = ModeArtifactRun::create(
            artifacts_root,
            case_id,
            scenario.name(),
            endpoint_value,
            scenario.expected_mode(),
            &inspection,
            file_sha256(&tool)?,
            scenario.native().map(|_| native.as_path()),
            scenario.native().map(|_| helper.as_path()),
        )?;
        let controller_thread = win32::current_thread_id();
        run.write_event(ModeEvent::controller(
            win32::qpc_now()?,
            endpoint_value,
            "observer_started",
            "PASS",
            "T0-02 controller ready; no game process has been opened",
        ))?;

        let result = observe(
            &mut run,
            scenario,
            endpoint,
            endpoint_value,
            &canonical_game,
            &mode_ledger,
            &image,
            &disk_before,
            &native,
            &helper,
            controller_thread,
            expected_variant,
        );
        if let Err(error) = result {
            if run.manifest.status == "RUNNING" {
                let _ = run.write_event(ModeEvent::controller(
                    win32::qpc_now().unwrap_or(0),
                    endpoint_value,
                    "evidence_failure",
                    "FAIL",
                    error.message.clone(),
                ));
                let _ = run.finish("FAIL", error.message.clone());
            }
            return Err(error);
        }
        let directory = run.directory.clone();
        drop(run);
        let report = analyze_mode_run(&directory)?;
        write_mode_analysis(&directory, &report)?;
        print_json(&report)?;
        if report.result == "PASS" {
            Ok(())
        } else {
            Err(AppError::evidence(format!(
                "T0-02 smoke failed at {}",
                report
                    .first_divergence
                    .as_deref()
                    .unwrap_or("unknown-check")
            )))
        }
    }

    #[allow(clippy::too_many_arguments)]
    fn observe(
        run: &mut ModeArtifactRun,
        scenario: Scenario,
        endpoint: EndpointRole,
        endpoint_value: &str,
        canonical_game: &Path,
        mode_ledger: &LoadedModeLedger,
        image: &PeImage,
        disk_before: &crate::ledger::StaticInspectionReport,
        native: &Path,
        helper: &Path,
        _controller_thread: u32,
        expected_variant: &str,
    ) -> AppResult<()> {
        if !win32::find_target_pids()?.is_empty() {
            return Err(AppError::evidence(
                "isaac-ng.exe was already running when mode-smoke started",
            ));
        }
        let blockers = win32::running_blockers()?;
        if !blockers.is_empty() {
            return Err(AppError::evidence(format!(
                "blocked instrumentation process present: {}",
                blockers.join(", ")
            )));
        }
        run.write_event(ModeEvent::controller(
            win32::qpc_now()?,
            endpoint_value,
            "waiting_for_process",
            "PASS",
            "waiting for user-started isaac-ng.exe; the controller cannot launch it",
        ))?;
        let process = wait_for_exact_process(canonical_game)?;
        let snapshot_ledger = mode_ledger.combined_snapshot_ledger();
        let before_live = wait_for_snapshot(&process, canonical_game, image, &snapshot_ledger)?;
        run.set_process(
            &before_live,
            filetime_100ns_to_utc(process.creation_time_100ns),
        );
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
            let mut line = String::new();
            let _ = io::stdin().read_line(&mut line);
            let _ = stop_tx.send(());
        });
        println!(
            "T0-02 {} / {} is observing. Complete the documented scenario, then press Enter to stop and restore hooks.",
            scenario.name(),
            endpoint_value
        );

        let entropy = entropy(process.pid, scenario.name(), endpoint_value)?;
        let nonce = sha256_parts(&[b"mapping", &entropy]);
        let pairing_key = if scenario == Scenario::OriginalOnline {
            let raw = std::env::var(PAIRING_KEY_ENV).map_err(|_| {
                AppError::usage(format!(
                    "original-online requires a shared 32-byte hex key in {PAIRING_KEY_ENV}"
                ))
            })?;
            parse_pairing_key(&raw)?
        } else {
            sha256_parts(&[b"offline-pairing", &entropy])
        };
        let mapping_name = format!(
            "Local\\IsaacCsT0.Mode.v1.{}.{}",
            process.pid,
            &hex::encode_upper(nonce)[..16]
        );
        let mut mapping = scenario
            .native()
            .map(|native_scenario| {
                ModeMapping::create(
                    mapping_name,
                    process.pid,
                    native_scenario,
                    endpoint,
                    scenario.requested_mode_raw(),
                    nonce,
                    pairing_key,
                )
            })
            .transpose()?;
        let mut native_next = 0_u64;
        let mut injected = false;
        if let Some(mapping) = mapping.as_ref() {
            run.write_event(ModeEvent::controller(
                win32::qpc_now()?,
                endpoint_value,
                "injection_requested",
                "PASS",
                "explicit T0-02 helper request issued for the already-running exact target",
            ))?;
            let inject_result = run_helper(
                helper,
                "--inject",
                process.pid,
                native,
                Some(mapping.name()),
            );
            if let Err(error) = inject_result {
                if mapping.state().ok() != Some(NativeState::Unsafe) {
                    let _ = run_helper(helper, "--unload", process.pid, native, None);
                }
                return Err(error);
            }
            injected = true;
            let ready_result = (|| -> AppResult<()> {
                let deadline = Instant::now() + WAIT_FOR_NATIVE;
                loop {
                    drain_native(run, mapping, endpoint_value, &mut native_next)?;
                    match mapping.state()? {
                        NativeState::Ready => return Ok(()),
                        NativeState::Failed | NativeState::Unsafe => {
                            return Err(AppError::evidence(format!(
                                "T0-02 Native initialization failed with code {}",
                                mapping.error_code()
                            )));
                        }
                        _ if Instant::now() >= deadline => {
                            return Err(AppError::timeout(
                                "T0-02 Native did not become ready within 15 seconds",
                            ));
                        }
                        _ => thread::sleep(Duration::from_millis(10)),
                    }
                }
            })();
            if let Err(error) = ready_result {
                if mapping.state().ok() != Some(NativeState::Unsafe) {
                    let _ = run_helper(helper, "--unload", process.pid, native, None);
                    let _ = drain_native(run, mapping, endpoint_value, &mut native_next);
                }
                return Err(error);
            }
            run.set_native_ready(mapping.hooks_mask());
            run.write_manifest()?;
        }
        let mut injection_guard =
            mapping
                .as_ref()
                .filter(|_| injected)
                .map(|mapping| InjectionGuard {
                    helper,
                    native,
                    pid: process.pid,
                    mapping,
                    armed: true,
                });

        let observation_started = Instant::now();
        let mut next_control_sample = Instant::now();
        loop {
            if process.wait(0)? {
                return Err(AppError::evidence(
                    "target exited before target restoration could be proven",
                ));
            }
            if let Some(mapping) = mapping.as_ref() {
                drain_native(run, mapping, endpoint_value, &mut native_next)?;
                if mapping.overwritten_events() != 0 {
                    return Err(AppError::evidence("native event ring reported overwrite"));
                }
                if matches!(mapping.state()?, NativeState::Failed | NativeState::Unsafe) {
                    return Err(AppError::evidence(format!(
                        "native observer failed during run with code {}",
                        mapping.error_code()
                    )));
                }
            } else if Instant::now() >= next_control_sample {
                write_control_sample(run, &process, endpoint_value)?;
                next_control_sample = Instant::now() + Duration::from_millis(500);
            }
            if stop_rx.try_recv().is_ok() {
                break;
            }
            if observation_started.elapsed() >= MAX_OBSERVATION {
                return Err(AppError::timeout(
                    "T0-02 observation exceeded the 30 minute limit",
                ));
            }
            thread::sleep(Duration::from_millis(10));
        }

        if injected {
            let mapping_ref = mapping
                .as_ref()
                .ok_or_else(|| AppError::evidence("native mapping disappeared"))?;
            run_helper(helper, "--unload", process.pid, native, None)?;
            let deadline = Instant::now() + WAIT_FOR_NATIVE;
            loop {
                drain_native(run, mapping_ref, endpoint_value, &mut native_next)?;
                if mapping_ref.state()? == NativeState::Stopped {
                    break;
                }
                if Instant::now() >= deadline {
                    return Err(AppError::timeout(
                        "native shutdown did not publish Stopped within 15 seconds",
                    ));
                }
                thread::sleep(Duration::from_millis(10));
            }
            if mapping_ref.overwritten_events() != 0 {
                return Err(AppError::evidence(
                    "native event overwrite invalidates evidence",
                ));
            }
            run.set_local_peer_token(mapping_ref.local_peer_token());
            run.set_safe_unload("PASS_RESTORED");
            if let Some(guard) = injection_guard.as_mut() {
                guard.disarm();
            }
        }
        drop(injection_guard);
        drop(mapping.take());

        let refreshed = ObservedProcess::open_exact(process.pid, canonical_game)?;
        if refreshed.creation_time_100ns != process.creation_time_100ns {
            return Err(AppError::evidence(
                "target PID was reused during T0-02 smoke",
            ));
        }
        let after_live = wait_for_snapshot(&refreshed, canonical_game, image, &snapshot_ledger)?;
        let (_, _, disk_after) =
            inspect_static(canonical_game, &mode_ledger.baseline_path, expected_variant)?;
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
        run.write_event(ModeEvent::controller(
            win32::qpc_now()?,
            endpoint_value,
            "observation_completed",
            "PASS",
            "target remained live and every hook target was recaptured after restoration",
        ))?;
        run.finish("PASS", "T0-02 endpoint evidence captured")
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
                    "more than one isaac-ng.exe process is visible to this endpoint controller",
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
        canonical_game: &Path,
        image: &PeImage,
        baseline: &crate::ledger::LoadedLedger,
    ) -> AppResult<crate::artifact::LiveProcessSnapshot> {
        let deadline = Instant::now() + Duration::from_secs(30);
        loop {
            let refreshed = ObservedProcess::open_exact(process.pid, canonical_game)?;
            if refreshed.creation_time_100ns != process.creation_time_100ns {
                return Err(AppError::evidence(
                    "target PID was reused while waiting for a complete module snapshot",
                ));
            }
            match capture_live_snapshot(&refreshed, image, baseline) {
                Ok(snapshot) => return Ok(snapshot),
                Err(error) if Instant::now() < deadline => {
                    if refreshed.wait(0)? {
                        return Err(AppError::evidence(
                            "target exited before a live target snapshot could be captured",
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
        run: &mut ModeArtifactRun,
        mapping: &ModeMapping,
        endpoint: &str,
        next: &mut u64,
    ) -> AppResult<()> {
        for record in mapping.drain(next)? {
            run.write_event(ModeEvent::from_native(record, endpoint)?)?;
        }
        Ok(())
    }

    fn write_control_sample(
        run: &mut ModeArtifactRun,
        process: &ObservedProcess,
        endpoint: &str,
    ) -> AppResult<()> {
        let manager_slot = process.main_module.base + 0x0087_169C;
        let bytes = process.read_memory(manager_slot, 4)?;
        let manager = u64::from(u32::from_le_bytes(bytes.try_into().unwrap()));
        let online_frame = if manager == 0 {
            None
        } else {
            process
                .read_memory(manager + 0x0004_B3E4, 4)
                .ok()
                .map(|bytes| u32::from_le_bytes(bytes.try_into().unwrap()))
        };
        let mut event = ModeEvent::controller(
            win32::qpc_now()?,
            endpoint,
            "control_sample",
            "PASS",
            if manager == 0 {
                "manager slot is null"
            } else {
                "manager slot is non-null; raw pointer intentionally omitted"
            },
        );
        event.online_before = online_frame;
        run.write_event(event)
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
            .usage_context("cannot run independent T0-02 helper")?;
        let stdout = String::from_utf8_lossy(&output.stdout);
        let stderr = String::from_utf8_lossy(&output.stderr);
        if !output.status.success() || !stdout.trim_start().starts_with("PASS") {
            return Err(AppError::evidence(format!(
                "T0-02 helper {operation} failed (exit={:?}): {}{}",
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
                "{label} is missing from the controller distribution: {}",
                path.display()
            )));
        }
        Ok(())
    }

    fn entropy(pid: u32, scenario: &str, endpoint: &str) -> AppResult<[u8; 32]> {
        let time = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .map_err(|_| AppError::evidence("system clock is before Unix epoch"))?
            .as_nanos()
            .to_le_bytes();
        let qpc = win32::qpc_now()?.to_le_bytes();
        Ok(sha256_parts(&[
            &pid.to_le_bytes(),
            &time,
            &qpc,
            scenario.as_bytes(),
            endpoint.as_bytes(),
        ]))
    }

    fn sha256_parts(parts: &[&[u8]]) -> [u8; 32] {
        let mut hasher = Sha256::new();
        for part in parts {
            hasher.update(part);
        }
        hasher.finalize().into()
    }
}

#[cfg(windows)]
pub use live::run as mode_smoke_command;

fn print_json(value: &impl Serialize) -> AppResult<()> {
    let json =
        serde_json::to_string_pretty(value).usage_context("cannot serialize command result")?;
    println!("{json}");
    Ok(())
}
