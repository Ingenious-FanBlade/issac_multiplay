use std::collections::BTreeSet;
use std::path::Path;

use crate::active_live::{RepeatabilityInputSink, RepeatabilityScript};
#[cfg(not(windows))]
use crate::error::AppError;
use crate::error::AppResult;

const ACTIVE_RUNTIME_SCENARIOS: [&str; 16] = [
    "keyboard",
    "controller",
    "simultaneous",
    "rapid-tap",
    "device-switch",
    "pause",
    "loading",
    "room",
    "menu",
    "queue-full",
    "expired",
    "too-far",
    "stale",
    "conflict",
    "duplicate",
    "loopback",
];

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum ReadyBindingStatus {
    Complete,
    AwaitingMainThread,
    Invalid,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
struct ReadyBindingSnapshot {
    active_hooks: u64,
    loop_hooks: u64,
    active_main_thread: u32,
    loop_main_thread: u32,
    active_overwrites: u64,
    loop_overwrites: u64,
}

impl ReadyBindingSnapshot {
    fn status(self) -> ReadyBindingStatus {
        ready_binding_status(
            self.active_hooks,
            self.loop_hooks,
            self.active_main_thread,
            self.loop_main_thread,
            self.active_overwrites,
            self.loop_overwrites,
        )
    }
}

fn ready_binding_status(
    active_hooks: u64,
    loop_hooks: u64,
    active_main_thread: u32,
    loop_main_thread: u32,
    active_overwrites: u64,
    loop_overwrites: u64,
) -> ReadyBindingStatus {
    if active_hooks != crate::active_artifact::ACTIVE_HOOK_MASK
        || loop_hooks != crate::loop_ipc::LOOP_HOOK_MASK
        || active_overwrites != 0
        || loop_overwrites != 0
    {
        return ReadyBindingStatus::Invalid;
    }
    if active_main_thread == 0 {
        return ReadyBindingStatus::AwaitingMainThread;
    }
    if loop_main_thread == 0 {
        return ReadyBindingStatus::Invalid;
    }
    if active_main_thread != loop_main_thread {
        return ReadyBindingStatus::Invalid;
    }
    ReadyBindingStatus::Complete
}

#[derive(Clone, Copy, Debug, Eq, Ord, PartialEq, PartialOrd)]
enum ScriptKey {
    A,
    D,
    Right,
}

impl ScriptKey {
    fn parse(value: &str) -> AppResult<Self> {
        match value {
            "A" => Ok(Self::A),
            "D" => Ok(Self::D),
            "RIGHT" => Ok(Self::Right),
            _ => Err(crate::error::AppError::evidence(format!(
                "repeatability input sink received unsupported key {value}"
            ))),
        }
    }

    #[cfg(windows)]
    const fn virtual_key(self) -> u16 {
        match self {
            Self::A => 0x41,
            Self::D => 0x44,
            Self::Right => 0x27,
        }
    }
}

trait KeyEmitter {
    fn emit(&mut self, key: ScriptKey, down: bool) -> AppResult<()>;
}

struct KeyStateSink<E: KeyEmitter> {
    emitter: E,
    down: BTreeSet<ScriptKey>,
}

impl<E: KeyEmitter> KeyStateSink<E> {
    fn new(emitter: E) -> Self {
        Self {
            emitter,
            down: BTreeSet::new(),
        }
    }

    fn release_all_inner(&mut self) -> AppResult<()> {
        let keys = self.down.iter().copied().collect::<Vec<_>>();
        let mut first_error = None;
        for key in keys {
            match self.emitter.emit(key, false) {
                Ok(()) => {
                    self.down.remove(&key);
                }
                Err(error) if first_error.is_none() => first_error = Some(error),
                Err(_) => {}
            }
        }
        match first_error {
            Some(error) => Err(error),
            None => Ok(()),
        }
    }
}

impl<E: KeyEmitter> RepeatabilityInputSink for KeyStateSink<E> {
    fn set_keys(&mut self, keys_down: &[String]) -> AppResult<()> {
        let desired = keys_down
            .iter()
            .map(|key| ScriptKey::parse(key))
            .collect::<AppResult<BTreeSet<_>>>()?;
        let releases = self.down.difference(&desired).copied().collect::<Vec<_>>();
        let presses = desired.difference(&self.down).copied().collect::<Vec<_>>();
        for key in releases {
            self.emitter.emit(key, false)?;
            self.down.remove(&key);
        }
        for key in presses {
            self.emitter.emit(key, true)?;
            self.down.insert(key);
        }
        Ok(())
    }

    fn release_all(&mut self) -> AppResult<()> {
        self.release_all_inner()
    }
}

impl<E: KeyEmitter> Drop for KeyStateSink<E> {
    fn drop(&mut self) {
        let _ = self.release_all_inner();
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum ControllerStage {
    StaticValidated,
    VerificationV2,
    TokenCas,
    CompanionReady,
    ActiveMapping,
    HelperInjected,
    RingsDrained,
    HelperUnloaded,
    RestorationVerified,
    ArtifactComplete,
}

const CONTROLLER_ORDER: [ControllerStage; 10] = [
    ControllerStage::StaticValidated,
    ControllerStage::VerificationV2,
    ControllerStage::TokenCas,
    ControllerStage::CompanionReady,
    ControllerStage::ActiveMapping,
    ControllerStage::HelperInjected,
    ControllerStage::RingsDrained,
    ControllerStage::HelperUnloaded,
    ControllerStage::RestorationVerified,
    ControllerStage::ArtifactComplete,
];

struct ControllerOrder {
    next: usize,
}

impl ControllerOrder {
    fn after_static_validation() -> Self {
        Self { next: 1 }
    }

    fn advance(&mut self, stage: ControllerStage) -> AppResult<()> {
        if CONTROLLER_ORDER.get(self.next) != Some(&stage) {
            return Err(crate::error::AppError::evidence(format!(
                "active Controller order violation: expected {:?}, got {stage:?}",
                CONTROLLER_ORDER.get(self.next)
            )));
        }
        self.next += 1;
        Ok(())
    }

    fn current(&self) -> Option<ControllerStage> {
        self.next
            .checked_sub(1)
            .map(|index| CONTROLLER_ORDER[index])
    }
}

fn runtime_scenario_code(value: &str) -> AppResult<u32> {
    ACTIVE_RUNTIME_SCENARIOS
        .iter()
        .position(|scenario| *scenario == value)
        .map(|index| index as u32 + 1)
        .ok_or_else(|| {
            crate::error::AppError::usage(format!(
                "active runtime does not support scenario {value}"
            ))
        })
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum HelperOutcome {
    Pass,
    UnsafeDoNotUnload,
    Failure,
}

fn classify_helper_outcome(
    success: bool,
    pass_prefix: bool,
    exit_code: Option<i32>,
) -> HelperOutcome {
    if success && pass_prefix {
        HelperOutcome::Pass
    } else if exit_code == Some(8) {
        HelperOutcome::UnsafeDoNotUnload
    } else {
        HelperOutcome::Failure
    }
}

fn automatic_unload_allowed(
    active_state_known_safe: bool,
    companion_state_known_safe: bool,
    active_error: i32,
    companion_error: i32,
) -> bool {
    active_state_known_safe
        && companion_state_known_safe
        && active_error != 8
        && companion_error != 8
}

#[cfg(windows)]
mod live {
    use std::ffi::c_int;
    use std::fs;
    use std::io::{self, BufRead};
    use std::mem::size_of;
    use std::path::{Path, PathBuf};
    use std::process::Command;
    use std::sync::mpsc;
    use std::thread;
    use std::time::{Duration, Instant};

    use chrono::Utc;
    use sha2::{Digest, Sha256};

    use super::*;
    use crate::INPUT_TOOL_IDENTITY;
    use crate::active_artifact::{
        ACTIVE_HOOK_MASK, ActiveRun, ActiveRunConfig, ActiveRunFinish, ActiveTargetSlot,
        DisarmedControlRunConfig, ORIGINAL_ONLINE_OBSERVATION_SCHEMA_VERSION,
        OriginalOnlineDisarmedObservation, write_disarmed_control_run,
    };
    use crate::active_catalog::{ValidatedActivePlan, expected_hook_slots, validate_active_plan};
    use crate::active_ipc::{ACTIVE_IPC_SCHEMA_VERSION, ActiveMapping, ActiveNativeState};
    use crate::active_live::{
        ClaimedActiveRun, RepeatabilityScriptGuard, load_repeatability_script,
        verify_active_execution_before_cas,
    };
    use crate::arm_token::{arm_token_sha256, validate_arm_grant};
    use crate::artifact::file_sha256;
    use crate::error::{AppError, AppResult, UsageContext};
    use crate::loop_commands::{
        CompanionLoopRun, capture_loop_target_bytes, drain_companion_loop, loop_entropy,
    };
    use crate::loop_ipc::{LOOP_IPC_VERSION, LoopMapping, LoopNativeState};
    use crate::loop_verification::{analyze_loop_run, inspect_loop_equivalent};
    use crate::target_equivalence::validate_target_equivalence;
    use crate::win32::ObservedProcess;

    const READY_TIMEOUT: Duration = Duration::from_secs(15);
    const MAX_CAPTURE: Duration = Duration::from_secs(600);
    const POLL_INTERVAL: Duration = Duration::from_millis(5);
    const POST_SCRIPT_AUTHORITY_TICKS: u64 = 2;
    const UNSAFE_HELPER_RESULT: i32 = 8;
    const ORIGINAL_ONLINE_OBSERVATION_GAP: Duration = Duration::from_millis(100);

    struct CollectedDisarmedObservation {
        process: ObservedProcess,
        target_slots_before: Vec<ActiveTargetSlot>,
        target_slots_after: Vec<ActiveTargetSlot>,
        observation_qpc: u64,
        original_online_observation: Option<OriginalOnlineDisarmedObservation>,
        nonce: [u8; 32],
    }

    fn read_explicit_stop_line(reader: &mut impl BufRead) -> io::Result<bool> {
        let mut line = String::new();
        let bytes_read = reader.read_line(&mut line)?;
        Ok(bytes_read != 0 && line.ends_with('\n'))
    }

    #[repr(C)]
    struct MouseInput {
        dx: i32,
        dy: i32,
        mouse_data: u32,
        flags: u32,
        time: u32,
        extra_info: usize,
    }

    #[repr(C)]
    struct KeyboardInput {
        virtual_key: u16,
        scan_code: u16,
        flags: u32,
        time: u32,
        extra_info: usize,
    }

    #[repr(C)]
    struct HardwareInput {
        message: u32,
        parameter_low: u16,
        parameter_high: u16,
    }

    #[repr(C)]
    union InputPayload {
        mouse: std::mem::ManuallyDrop<MouseInput>,
        keyboard: std::mem::ManuallyDrop<KeyboardInput>,
        hardware: std::mem::ManuallyDrop<HardwareInput>,
    }

    #[repr(C)]
    struct Win32Input {
        kind: u32,
        payload: InputPayload,
    }

    #[link(name = "user32")]
    unsafe extern "system" {
        fn SendInput(count: u32, inputs: *const Win32Input, size: c_int) -> u32;
    }

    struct SendInputEmitter;

    impl KeyEmitter for SendInputEmitter {
        fn emit(&mut self, key: ScriptKey, down: bool) -> AppResult<()> {
            const INPUT_KEYBOARD: u32 = 1;
            const KEYEVENTF_KEYUP: u32 = 0x0002;
            let input = Win32Input {
                kind: INPUT_KEYBOARD,
                payload: InputPayload {
                    keyboard: std::mem::ManuallyDrop::new(KeyboardInput {
                        virtual_key: key.virtual_key(),
                        scan_code: 0,
                        flags: if down { 0 } else { KEYEVENTF_KEYUP },
                        time: 0,
                        extra_info: 0,
                    }),
                },
            };
            let sent = unsafe {
                SendInput(
                    1,
                    &raw const input,
                    c_int::try_from(size_of::<Win32Input>()).expect("Win32 INPUT size fits c_int"),
                )
            };
            if sent != 1 {
                return Err(AppError::evidence(format!(
                    "SendInput failed while {} key {:?}: {}",
                    if down { "pressing" } else { "releasing" },
                    key,
                    io::Error::last_os_error()
                )));
            }
            Ok(())
        }
    }

    type SendInputSink = KeyStateSink<SendInputEmitter>;

    struct InjectionGuard<'a> {
        helper: &'a Path,
        native: &'a Path,
        pid: u32,
        active: &'a ActiveMapping,
        companion: &'a LoopMapping,
        armed: bool,
    }

    impl InjectionGuard<'_> {
        fn unload(&mut self) -> AppResult<HelperOutcome> {
            self.armed = false;
            run_helper(
                self.helper,
                "--unload-input-active",
                self.pid,
                self.native,
                None,
            )
        }

        fn safe_to_unload(&self) -> bool {
            automatic_unload_allowed(
                self.active
                    .state()
                    .is_ok_and(|state| state != ActiveNativeState::Unsafe),
                self.companion
                    .state()
                    .is_ok_and(|state| state != LoopNativeState::Unsafe),
                self.active.error_code(),
                self.companion.error_code(),
            )
        }
    }

    impl Drop for InjectionGuard<'_> {
        fn drop(&mut self) {
            if self.armed && self.safe_to_unload() {
                self.armed = false;
                let _ = run_helper(
                    self.helper,
                    "--unload-input-active",
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
        offline_prereq: &Path,
        arm_grant: &Path,
        active_verification: &Path,
        input_script: Option<&Path>,
        expected_variant: &str,
        artifacts_root: &Path,
        case_id: &str,
        arm_token: &str,
    ) -> AppResult<()> {
        validate_case_and_root(artifacts_root, case_id)?;
        let native_scenario = runtime_scenario_code(scenario)?;
        let canonical_game = fs::canonicalize(game_exe).usage_context(format!(
            "cannot resolve game executable {}",
            game_exe.display()
        ))?;
        crate::input_core::run_codec_self_test()
            .map_err(|error| AppError::evidence(error.to_string()))?;
        let plan = validate_active_plan(active_plan, mode_gate, loop_gate, raw_prereq)?;
        let equivalence = validate_target_equivalence(
            target_equivalence,
            &canonical_game,
            loop_ledger,
            target_ledger,
            active_catalog,
            active_plan,
        )?;
        let loop_inspection =
            inspect_loop_equivalent(&canonical_game, loop_ledger, expected_variant, &equivalence)?;
        if plan.target_sha256 != equivalence.canonical_target_sha256
            || loop_inspection.file_sha256 != equivalence.observed_target_sha256
            || plan.loop_ledger_sha256 != loop_inspection.ledger_sha256
        {
            return Err(AppError::evidence(
                "active input bundle does not bind the selected target and frozen loop ledger",
            ));
        }
        let script = load_bound_script(scenario, input_script, &plan)?;
        let preflight = crate::active_preflight::run_read_only_preflight(
            &canonical_game,
            &equivalence.observed_target_sha256,
            scenario,
        )?;
        let observable_preflight_passed = preflight
            .checks
            .iter()
            .filter(|check| check.id != "offline-session-attestation")
            .all(|check| check.passed);
        if preflight.result != "BLOCKED_BY_UNIMPLEMENTED_OFFLINE_ATTESTATION"
            || !observable_preflight_passed
        {
            return Err(AppError::evidence(
                "active read-only preflight did not reach the offline prerequisite boundary",
            ));
        }
        let prerequisite =
            crate::offline_attestation::validate_offline_prerequisite(offline_prereq, Utc::now())?;
        if prerequisite.target_sha256 != plan.target_sha256
            || prerequisite.observed_target_sha256 != equivalence.observed_target_sha256
            || prerequisite.target_equivalence_sha256 != equivalence.document_sha256
            || prerequisite.mode_gate_sha256 != plan.mode_gate_sha256
            || prerequisite.loop_gate_sha256 != plan.loop_gate_sha256
            || prerequisite.raw_prereq_sha256 != plan.raw_prereq_sha256
            || prerequisite.active_plan_sha256 != plan.plan_sha256
            || prerequisite.case_id != case_id
            || prerequisite.scenario != scenario
            || prerequisite.process_id != preflight.process_id
            || prerequisite.process_creation_time_100ns != preflight.process_creation_time_100ns
        {
            return Err(AppError::evidence(
                "offline prerequisite no longer binds the current target, process, case, scenario, or frozen bundle",
            ));
        }
        crate::offline_attestation_commands::recheck_offline_continuity_command(
            &canonical_game,
            &plan.target_sha256,
            scenario,
            offline_prereq,
        )?;
        let _grant = validate_arm_grant(
            arm_grant,
            offline_prereq,
            arm_token,
            case_id,
            scenario,
            Utc::now(),
        )?;

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
        let process = ObservedProcess::open_exact(preflight.process_id, &canonical_game)?;
        let target_slots_before = capture_active_target_slots(&process)?;
        let nonce = loop_entropy(process.pid, native_scenario)?;
        let session_id: [u8; 16] = nonce[..16]
            .try_into()
            .expect("16-byte nonce prefix is exact");
        let mut client_id = u64::from_le_bytes(
            nonce[16..24]
                .try_into()
                .expect("8-byte nonce client id is exact"),
        );
        if client_id == 0 {
            client_id = 1;
        }
        let suffix = &hex::encode_upper(nonce)[..16];
        let loop_mapping_name = format!(
            "Local\\IsaacCsT0.Loop.v{}.{}.{}",
            LOOP_IPC_VERSION, process.pid, suffix
        );
        let active_mapping_name = format!(
            "Local\\IsaacCsT0.Active.v{}.{}.{}",
            ACTIVE_IPC_SCHEMA_VERSION, process.pid, suffix
        );
        let timestamp = Utc::now().format("%Y%m%dT%H%M%SZ");
        let output_directory = artifacts_root.join(format!(
            "{timestamp}-t0-04-input-active-{scenario}-{case_id}"
        ));
        let companion_path = PathBuf::from("loop-context");
        let arm_sha256 = arm_token_sha256(arm_token)?;
        let artifact_identity = artifact_identity(
            case_id,
            scenario,
            &plan,
            &equivalence.observed_target_sha256,
            &equivalence.document_sha256,
            &nonce,
            &arm_sha256,
        );
        let run_config = ActiveRunConfig {
            case_id: case_id.to_owned(),
            scenario: scenario.to_owned(),
            target_sha256: plan.target_sha256.clone(),
            observed_target_sha256: equivalence.observed_target_sha256.clone(),
            target_equivalence_sha256: equivalence.document_sha256.clone(),
            mode_gate_sha256: plan.mode_gate_sha256.clone(),
            loop_gate_sha256: plan.loop_gate_sha256.clone(),
            raw_prereq_sha256: plan.raw_prereq_sha256.clone(),
            active_catalog_sha256: plan.catalog_sha256.clone(),
            active_plan_sha256: plan.plan_sha256.clone(),
            input_script_sha256: (scenario == "loopback")
                .then(|| plan.repeatability_input_script_sha256.clone()),
            companion_loop_artifact_path: companion_path.clone(),
            artifact_identity,
            arm_token_sha256: arm_sha256.clone(),
            session_id,
            client_id,
            target_slots_before: target_slots_before.clone(),
        };
        let finalize_failure_config = run_config.clone();
        // Decode every mapping identity before verification/CAS so malformed static
        // inputs cannot leave a consumed token with an unfinalized companion run.
        let target_hash = decode_hash(&plan.target_sha256)?;
        let observed_target_hash = decode_hash(&equivalence.observed_target_sha256)?;
        let mode_gate_hash = decode_hash(&plan.mode_gate_sha256)?;
        let loop_gate_hash = decode_hash(&plan.loop_gate_sha256)?;
        let raw_prereq_hash = decode_hash(&plan.raw_prereq_sha256)?;
        let target_equivalence_hash = decode_hash(&equivalence.document_sha256)?;
        let catalog_hash = decode_hash(&plan.catalog_sha256)?;
        let plan_hash = decode_hash(&plan.plan_sha256)?;
        let input_script_hash = decode_hash(&plan.repeatability_input_script_sha256)?;
        let arm_hash = decode_hash(&arm_sha256)?;

        let mut order = ControllerOrder::after_static_validation();
        let verified = verify_active_execution_before_cas(active_verification)?;
        order.advance(ControllerStage::VerificationV2)?;
        let mut active_run = verified.consume_and_begin(
            arm_grant,
            offline_prereq,
            arm_token,
            case_id,
            scenario,
            Utc::now(),
            &output_directory,
            run_config,
        )?;
        order.advance(ControllerStage::TokenCas)?;

        let mut loop_run = CompanionLoopRun::create_in(
            output_directory.join(&companion_path),
            case_id,
            scenario,
            &plan.target_sha256,
            &plan.loop_ledger_sha256,
            &native_sha256,
            &helper_sha256,
        )?;
        let loop_mapping =
            match LoopMapping::create(loop_mapping_name, process.pid, native_scenario, nonce) {
                Ok(mapping) => mapping,
                Err(error) => {
                    loop_run.fail();
                    persist_active_failure(
                        active_run,
                        &format!("cannot create companion V5 mapping after token CAS: {error}"),
                        None,
                        ActiveRunFinish::default(),
                    )?;
                    return Err(error);
                }
            };
        let loop_before = match capture_loop_target_bytes(&process) {
            Ok(snapshot) => snapshot,
            Err(error) => {
                loop_run.fail();
                persist_active_failure(
                    active_run,
                    &format!("cannot capture companion V5 slots after token CAS: {error}"),
                    None,
                    ActiveRunFinish::default(),
                )?;
                return Err(error);
            }
        };
        order.advance(ControllerStage::CompanionReady)?;

        let active_mapping = match ActiveMapping::create(
            active_mapping_name,
            process.pid,
            native_scenario,
            process.creation_time_100ns,
            nonce,
            target_hash,
            observed_target_hash,
            target_equivalence_hash,
            mode_gate_hash,
            loop_gate_hash,
            raw_prereq_hash,
            catalog_hash,
            plan_hash,
            input_script_hash,
            arm_hash,
            loop_mapping.name(),
            session_id,
            client_id,
        ) {
            Ok(mapping) => mapping,
            Err(error) => {
                let _ = loop_run.fail_with_evidence(0, 0, 0, &loop_before, None);
                persist_active_failure(
                    active_run,
                    &format!("cannot create schema-4 Active mapping after token CAS: {error}"),
                    None,
                    ActiveRunFinish::default(),
                )?;
                return Err(error);
            }
        };
        if !active_mapping.effective_state_neutral() {
            let _ = loop_run.fail_with_evidence(0, 0, 0, &loop_before, None);
            persist_active_failure(
                active_run,
                "schema-4 Active mapping did not begin with an all-neutral immutable snapshot",
                None,
                ActiveRunFinish::default(),
            )?;
            return Err(AppError::evidence(
                "schema-4 Active mapping initial snapshot is not neutral",
            ));
        }
        order.advance(ControllerStage::ActiveMapping)?;

        let mut next_active = 0_u64;
        let mut next_loop = 0_u64;
        let runtime = capture_runtime(
            &mut active_run,
            &mut loop_run,
            &active_mapping,
            &loop_mapping,
            &process,
            &native,
            &helper,
            &mut next_active,
            &mut next_loop,
            script,
            &mut order,
        );
        let final_drain = drain_both(
            &mut active_run,
            &mut loop_run,
            &active_mapping,
            &loop_mapping,
            &mut next_active,
            &mut next_loop,
        );
        let summary = finish_summary(&active_mapping);
        let mut failures = Vec::new();
        let ready_binding = match runtime {
            Ok(binding) => Some(binding),
            Err(error) => {
                failures.push(error.to_string());
                None
            }
        };
        if let Err(error) = final_drain {
            failures.push(format!("final Active/V5 ring drain failed: {error}"));
        }
        let target_alive = match process.wait(0) {
            Ok(exited) => !exited,
            Err(error) => {
                failures.push(format!(
                    "cannot query target after active shutdown: {error}"
                ));
                false
            }
        };
        let target_slots_after = if target_alive {
            match capture_active_target_slots(&process) {
                Ok(snapshot) => Some(snapshot),
                Err(error) => {
                    failures.push(format!(
                        "cannot capture 45 slots after active shutdown: {error}"
                    ));
                    None
                }
            }
        } else {
            None
        };
        let loop_after = if target_alive {
            match capture_loop_target_bytes(&process) {
                Ok(snapshot) => Some(snapshot),
                Err(error) => {
                    failures.push(format!(
                        "cannot capture companion V5 slots after shutdown: {error}"
                    ));
                    None
                }
            }
        } else {
            None
        };
        if !target_alive {
            failures.push("target exited before 45-slot restoration could be verified".to_owned());
        }
        if active_mapping.state().ok() != Some(ActiveNativeState::Stopped)
            || loop_mapping.state().ok() != Some(LoopNativeState::Stopped)
        {
            failures.push(format!(
                "runtime did not publish Stopped: active={:?}/{} loop={:?}/{}",
                active_mapping.state().ok(),
                active_mapping.error_code(),
                loop_mapping.state().ok(),
                loop_mapping.error_code()
            ));
        }
        if target_slots_after.as_ref() != Some(&target_slots_before)
            || summary.restored_slot_count != 45
            || summary.restored_active_hooks_mask != ACTIVE_HOOK_MASK
            || !summary.producer_closed
        {
            failures.push("schema-4 runtime did not restore and close all 45 slots".to_owned());
        }
        if let Some(binding) = ready_binding
            && (binding.status() != ReadyBindingStatus::Complete
                || active_mapping.main_thread_id() != binding.active_main_thread
                || loop_mapping.main_thread_id() != binding.loop_main_thread
                || active_mapping.installed_hooks_mask() != binding.active_hooks
                || loop_mapping.hooks_mask() != 0
                || active_mapping.overwritten_events() != 0
                || loop_mapping.overwritten_events() != 0)
        {
            failures.push(
                "Active/V5 runtime changed its trusted Ready binding or retained hooks after shutdown"
                    .to_owned(),
            );
        }

        if !failures.is_empty() {
            let reason = failures.join("; ");
            let _ = loop_run.fail_with_evidence(
                loop_mapping.main_thread_id(),
                loop_mapping.hooks_mask(),
                loop_mapping.overwritten_events(),
                &loop_before,
                loop_after.as_ref(),
            );
            persist_active_failure(active_run, &reason, target_slots_after, summary)?;
            return Err(AppError::evidence(reason));
        }

        let loop_after = loop_after.expect("target alive and restoration checked above");
        if let Err(error) = loop_run.finish(&loop_mapping, &loop_before, &loop_after) {
            loop_run.fail();
            persist_active_failure(
                active_run,
                &format!("cannot complete companion V5 artifact: {error}"),
                target_slots_after,
                summary,
            )?;
            return Err(error);
        }
        if let Err(error) = analyze_loop_run(&loop_run.directory) {
            loop_run.fail();
            persist_active_failure(
                active_run,
                &format!("companion V5 analysis failed: {error}"),
                target_slots_after,
                summary,
            )?;
            return Err(error);
        }
        order.advance(ControllerStage::RestorationVerified)?;
        let restored_slots = target_slots_after.expect("45 restored slots checked above");
        let report = match active_run.finish(restored_slots.clone(), summary) {
            Ok(report) => report,
            Err(error) => {
                let fallback = output_directory.with_file_name(format!(
                    "{}.finalize-failed",
                    output_directory
                        .file_name()
                        .and_then(|name| name.to_str())
                        .unwrap_or("active-artifact")
                ));
                let reason =
                    format!("active artifact finalization failed after token CAS: {error}");
                let fallback_result = ActiveRun::create(&fallback, finalize_failure_config)
                    .and_then(|run| run.fail(&reason, Some(restored_slots), summary));
                return match fallback_result {
                    Ok(()) => Err(AppError::evidence(format!(
                        "{reason}; create-new FAILED diagnostic={}",
                        fallback.display()
                    ))),
                    Err(fallback_error) => Err(AppError::evidence(format!(
                        "{reason}; cannot create fallback FAILED diagnostic: {fallback_error}"
                    ))),
                };
            }
        };
        order.advance(ControllerStage::ArtifactComplete)?;
        println!(
            "{}",
            serde_json::to_string_pretty(&report)
                .usage_context("cannot serialize active runtime analysis")?
        );
        if report.result == "PASS" {
            Ok(())
        } else {
            Err(AppError::evidence(
                "schema-4 active runtime analysis failed",
            ))
        }
    }

    #[allow(clippy::too_many_arguments)]
    pub(super) fn run_disarmed_control(
        probe: &str,
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
    ) -> AppResult<()> {
        if !matches!(
            (probe, scenario),
            ("control-no-native", "control-no-native")
                | ("original-online-disarmed", "original-online-disarmed")
        ) {
            return Err(AppError::usage(
                "the read-only disarmed collector requires a matching control-no-native or original-online-disarmed scenario",
            ));
        }
        validate_case_and_root(artifacts_root, case_id)?;
        let canonical_game = fs::canonicalize(game_exe).usage_context(format!(
            "cannot resolve game executable {}",
            game_exe.display()
        ))?;
        crate::input_core::run_codec_self_test()
            .map_err(|error| AppError::evidence(error.to_string()))?;
        let plan = validate_active_plan(active_plan, mode_gate, loop_gate, raw_prereq)?;
        let equivalence = validate_target_equivalence(
            target_equivalence,
            &canonical_game,
            loop_ledger,
            target_ledger,
            active_catalog,
            active_plan,
        )?;
        let loop_inspection =
            inspect_loop_equivalent(&canonical_game, loop_ledger, expected_variant, &equivalence)?;
        if plan.target_sha256 != equivalence.canonical_target_sha256
            || loop_inspection.file_sha256 != equivalence.observed_target_sha256
            || plan.loop_ledger_sha256 != loop_inspection.ledger_sha256
        {
            return Err(AppError::evidence(
                "disarmed control bundle does not bind the selected target and frozen loop ledger",
            ));
        }

        let collected = if probe == "control-no-native" {
            collect_control_no_native_observation(
                &canonical_game,
                &equivalence.observed_target_sha256,
                scenario,
            )?
        } else {
            collect_original_online_disarmed_observation(&canonical_game)?
        };
        let CollectedDisarmedObservation {
            process,
            target_slots_before,
            target_slots_after,
            observation_qpc,
            original_online_observation,
            nonce,
        } = collected;
        let observation_id: [u8; 16] = nonce[..16]
            .try_into()
            .expect("16-byte nonce prefix is exact");
        let mut observer_id = u64::from_le_bytes(
            nonce[16..24]
                .try_into()
                .expect("8-byte observer identity is exact"),
        );
        if observer_id == 0 {
            observer_id = 1;
        }
        let artifact_identity = disarmed_artifact_identity(
            case_id,
            probe,
            &plan,
            &equivalence.observed_target_sha256,
            &equivalence.document_sha256,
            process.pid,
            process.creation_time_100ns,
            &nonce,
            original_online_observation.as_ref(),
        );
        let timestamp = Utc::now().format("%Y%m%dT%H%M%SZ");
        let output_directory = artifacts_root.join(format!(
            "{timestamp}-t0-04-input-disarmed-{scenario}-{case_id}"
        ));
        let report = write_disarmed_control_run(
            &output_directory,
            DisarmedControlRunConfig {
                case_id: case_id.to_owned(),
                probe: probe.to_owned(),
                scenario: scenario.to_owned(),
                target_sha256: plan.target_sha256,
                observed_target_sha256: equivalence.observed_target_sha256,
                target_equivalence_sha256: equivalence.document_sha256,
                mode_gate_sha256: plan.mode_gate_sha256,
                loop_gate_sha256: plan.loop_gate_sha256,
                raw_prereq_sha256: plan.raw_prereq_sha256,
                active_catalog_sha256: plan.catalog_sha256,
                active_plan_sha256: plan.plan_sha256,
                original_online_observation,
                artifact_identity,
                observation_id,
                observer_id,
                observation_qpc,
                target_slots_before,
                target_slots_after,
            },
        )?;
        println!(
            "{}",
            serde_json::to_string_pretty(&report)
                .usage_context("cannot serialize disarmed control analysis")?
        );
        if report.result == "PASS" {
            Ok(())
        } else {
            Err(AppError::evidence(format!(
                "schema-4 {scenario} analysis failed"
            )))
        }
    }

    #[allow(clippy::too_many_arguments)]
    fn capture_runtime(
        active_run: &mut ClaimedActiveRun,
        loop_run: &mut CompanionLoopRun,
        active_mapping: &ActiveMapping,
        loop_mapping: &LoopMapping,
        process: &ObservedProcess,
        native: &Path,
        helper: &Path,
        next_active: &mut u64,
        next_loop: &mut u64,
        script: Option<RepeatabilityScript>,
        order: &mut ControllerOrder,
    ) -> AppResult<ReadyBindingSnapshot> {
        match run_helper(
            helper,
            "--inject-input-active",
            process.pid,
            native,
            Some(active_mapping.name()),
        )? {
            HelperOutcome::Pass => {}
            HelperOutcome::UnsafeDoNotUnload => {
                return Err(AppError::evidence(
                    "active Helper injection returned result 8; FAILED persisted and automatic unload is forbidden",
                ));
            }
            HelperOutcome::Failure => unreachable!("run_helper converts failures to AppError"),
        }
        order.advance(ControllerStage::HelperInjected)?;
        let mut injection = InjectionGuard {
            helper,
            native,
            pid: process.pid,
            active: active_mapping,
            companion: loop_mapping,
            armed: true,
        };
        let mut pending_script = script;
        let mut input_guard: Option<RepeatabilityScriptGuard<SendInputSink>> = None;
        let mut completed_at = None;
        let deadline = Instant::now() + READY_TIMEOUT;
        let ready_binding = loop {
            let latest = drain_both(
                active_run,
                loop_run,
                active_mapping,
                loop_mapping,
                next_active,
                next_loop,
            )?;
            if order.current() == Some(ControllerStage::HelperInjected) {
                order.advance(ControllerStage::RingsDrained)?;
            }
            match (active_mapping.state()?, loop_mapping.state()?) {
                (ActiveNativeState::Ready, LoopNativeState::Ready) => {
                    match ready_binding_status(
                        active_mapping.installed_hooks_mask(),
                        loop_mapping.hooks_mask(),
                        active_mapping.main_thread_id(),
                        loop_mapping.main_thread_id(),
                        active_mapping.overwritten_events(),
                        loop_mapping.overwritten_events(),
                    ) {
                        ReadyBindingStatus::Complete => {}
                        ReadyBindingStatus::AwaitingMainThread if Instant::now() < deadline => {
                            thread::sleep(POLL_INTERVAL);
                            continue;
                        }
                        ReadyBindingStatus::AwaitingMainThread => {
                            return Err(AppError::evidence(
                                "Active/V5 runtime became Ready but did not bind a trusted main thread within 15 seconds",
                            ));
                        }
                        ReadyBindingStatus::Invalid => {
                            return Err(AppError::evidence(
                                "Active/V5 runtime became Ready without exact hook/thread/ring binding",
                            ));
                        }
                    }
                    if let Some(tick) = latest.filter(|tick| *tick != 0)
                        && let Some(script) = pending_script.take()
                    {
                        input_guard = Some(RepeatabilityScriptGuard::new(
                            script,
                            SendInputSink::new(SendInputEmitter),
                            tick,
                        ));
                    }
                    break ReadyBindingSnapshot {
                        active_hooks: active_mapping.installed_hooks_mask(),
                        loop_hooks: loop_mapping.hooks_mask(),
                        active_main_thread: active_mapping.main_thread_id(),
                        loop_main_thread: loop_mapping.main_thread_id(),
                        active_overwrites: active_mapping.overwritten_events(),
                        loop_overwrites: loop_mapping.overwritten_events(),
                    };
                }
                (ActiveNativeState::Unsafe, _) | (_, LoopNativeState::Unsafe) => {
                    return Err(AppError::evidence(format!(
                        "Active/V5 initialization latched Unsafe; automatic unload forbidden: active={} loop={}",
                        active_mapping.error_code(),
                        loop_mapping.error_code()
                    )));
                }
                (ActiveNativeState::Failed, _) | (_, LoopNativeState::Failed) => {
                    return Err(AppError::evidence(format!(
                        "Active/V5 initialization failed: active={} loop={}",
                        active_mapping.error_code(),
                        loop_mapping.error_code()
                    )));
                }
                _ if Instant::now() >= deadline => {
                    return Err(AppError::evidence(
                        "Active/V5 runtime did not become Ready within 15 seconds",
                    ));
                }
                _ => thread::sleep(POLL_INTERVAL),
            }
        };

        let (stop_tx, stop_rx) = mpsc::channel();
        if input_guard.is_none() && pending_script.is_none() {
            thread::spawn(move || {
                let stdin = io::stdin();
                let mut stdin = stdin.lock();
                if matches!(read_explicit_stop_line(&mut stdin), Ok(true)) {
                    let _ = stop_tx.send(());
                }
            });
            println!(
                "READY T0-04 schema-4 active capture; press Enter to stop and restore (bounded at ten minutes)."
            );
        }
        let started = Instant::now();
        loop {
            if process.wait(0)? {
                return Err(AppError::evidence(
                    "target exited before active hooks could be restored",
                ));
            }
            let latest = drain_both(
                active_run,
                loop_run,
                active_mapping,
                loop_mapping,
                next_active,
                next_loop,
            )?;
            if input_guard.is_none()
                && let Some(tick) = latest.filter(|tick| *tick != 0)
                && let Some(script) = pending_script.take()
            {
                input_guard = Some(RepeatabilityScriptGuard::new(
                    script,
                    SendInputSink::new(SendInputEmitter),
                    tick,
                ));
            }
            if active_mapping.overwritten_events() != 0 || loop_mapping.overwritten_events() != 0 {
                return Err(AppError::evidence(
                    "Active or companion V5 event ring reported overwrite",
                ));
            }
            if active_mapping.state()? == ActiveNativeState::Unsafe
                || loop_mapping.state()? == LoopNativeState::Unsafe
                || active_mapping.error_code() == UNSAFE_HELPER_RESULT
                || loop_mapping.error_code() == UNSAFE_HELPER_RESULT
            {
                return Err(AppError::evidence(
                    "Active/V5 runtime latched Unsafe; automatic unload forbidden",
                ));
            }
            if active_mapping.state()? == ActiveNativeState::Failed
                || loop_mapping.state()? == LoopNativeState::Failed
            {
                return Err(AppError::evidence(format!(
                    "Active/V5 runtime failed: active={} loop={}",
                    active_mapping.error_code(),
                    loop_mapping.error_code()
                )));
            }
            if let (Some(guard), Some(tick)) = (input_guard.as_mut(), latest) {
                guard.advance(tick)?;
                if guard.complete() {
                    let completion = completed_at.get_or_insert(tick);
                    if tick.saturating_sub(*completion) >= POST_SCRIPT_AUTHORITY_TICKS {
                        break;
                    }
                }
            } else if pending_script.is_none() && stop_rx.try_recv().is_ok() {
                break;
            }
            if started.elapsed() >= MAX_CAPTURE {
                return Err(AppError::evidence(
                    "active capture reached the ten-minute fail-closed bound",
                ));
            }
            thread::sleep(POLL_INTERVAL);
        }
        drop(input_guard);
        match injection.unload()? {
            HelperOutcome::Pass => order.advance(ControllerStage::HelperUnloaded)?,
            HelperOutcome::UnsafeDoNotUnload => {
                return Err(AppError::evidence(
                    "active Helper shutdown returned result 8; DLL remains loaded and retry unload is forbidden",
                ));
            }
            HelperOutcome::Failure => unreachable!("run_helper converts failures to AppError"),
        }
        wait_for_stopped(
            active_run,
            loop_run,
            active_mapping,
            loop_mapping,
            next_active,
            next_loop,
        )?;
        Ok(ready_binding)
    }

    #[allow(clippy::too_many_arguments)]
    fn wait_for_stopped(
        active_run: &mut ClaimedActiveRun,
        loop_run: &mut CompanionLoopRun,
        active_mapping: &ActiveMapping,
        loop_mapping: &LoopMapping,
        next_active: &mut u64,
        next_loop: &mut u64,
    ) -> AppResult<()> {
        let deadline = Instant::now() + READY_TIMEOUT;
        loop {
            drain_both(
                active_run,
                loop_run,
                active_mapping,
                loop_mapping,
                next_active,
                next_loop,
            )?;
            if active_mapping.state()? == ActiveNativeState::Stopped
                && loop_mapping.state()? == LoopNativeState::Stopped
                && active_mapping.producer_closed()
                && loop_mapping.producer_closed()
            {
                return Ok(());
            }
            if active_mapping.state()? == ActiveNativeState::Unsafe
                || loop_mapping.state()? == LoopNativeState::Unsafe
                || active_mapping.error_code() == UNSAFE_HELPER_RESULT
                || loop_mapping.error_code() == UNSAFE_HELPER_RESULT
            {
                return Err(AppError::evidence(
                    "Active/V5 restoration latched Unsafe; no second unload is permitted",
                ));
            }
            if Instant::now() >= deadline {
                return Err(AppError::evidence(
                    "Active/V5 runtime did not publish Stopped within 15 seconds",
                ));
            }
            thread::sleep(POLL_INTERVAL);
        }
    }

    fn drain_both(
        active_run: &mut ClaimedActiveRun,
        loop_run: &mut CompanionLoopRun,
        active_mapping: &ActiveMapping,
        loop_mapping: &LoopMapping,
        next_active: &mut u64,
        next_loop: &mut u64,
    ) -> AppResult<Option<u64>> {
        let active_result = active_mapping.drain(next_active).and_then(|records| {
            let mut latest = None;
            for record in &records {
                latest = Some(latest.unwrap_or(0).max(record.event.authority_tick));
                active_run.append_native(record)?;
            }
            Ok(latest)
        });
        let loop_result = drain_companion_loop(loop_run, loop_mapping, next_loop);
        match (active_result, loop_result) {
            (Ok(latest), Ok(_)) => Ok(latest),
            (Err(active), Ok(_)) => Err(active),
            (Ok(_), Err(companion)) => Err(companion),
            (Err(active), Err(companion)) => Err(AppError::evidence(format!(
                "Active and companion V5 drains both failed: active={active}; companion={companion}"
            ))),
        }
    }

    fn finish_summary(mapping: &ActiveMapping) -> ActiveRunFinish {
        ActiveRunFinish {
            main_thread_id: mapping.main_thread_id(),
            apply_generation: mapping.apply_generation(),
            installed_active_hooks_mask: mapping.installed_hooks_mask(),
            restored_active_hooks_mask: mapping.restored_hooks_mask(),
            restored_slot_count: mapping.restored_slot_count(),
            overwritten_events: mapping.overwritten_events(),
            producer_closed: mapping.producer_closed(),
            initial_snapshot_neutral: true,
            client_retired: mapping.client_retired(),
        }
    }

    fn collect_control_no_native_observation(
        canonical_game: &Path,
        observed_target_sha256: &str,
        scenario: &str,
    ) -> AppResult<CollectedDisarmedObservation> {
        let preflight = crate::active_preflight::run_read_only_preflight(
            canonical_game,
            observed_target_sha256,
            scenario,
        )?;
        let observable_preflight_passed = preflight
            .checks
            .iter()
            .filter(|check| check.id != "offline-session-attestation")
            .all(|check| check.passed);
        if preflight.result != "BLOCKED_BY_UNIMPLEMENTED_OFFLINE_ATTESTATION"
            || !observable_preflight_passed
        {
            return Err(AppError::evidence(
                "control-no-native did not pass every read-only observable preflight check",
            ));
        }

        let process = ObservedProcess::open_exact(preflight.process_id, canonical_game)?;
        if process.creation_time_100ns != preflight.process_creation_time_100ns {
            return Err(AppError::evidence(
                "control-no-native process creation identity changed after preflight",
            ));
        }
        require_clean_disarmed_process(&process, "control-no-native")?;
        let target_slots_before = capture_active_target_slots(&process)?;
        let observation_qpc = u64::try_from(crate::win32::qpc_now()?)
            .map_err(|_| AppError::evidence("control observation QPC is not positive"))?;
        let target_slots_after = capture_active_target_slots(&process)?;
        if target_slots_before != target_slots_after {
            return Err(AppError::evidence(
                "control-no-native 45-slot snapshots changed during the read-only observation",
            ));
        }
        let nonce = loop_entropy(process.pid, 0)?;
        Ok(CollectedDisarmedObservation {
            process,
            target_slots_before,
            target_slots_after,
            observation_qpc,
            original_online_observation: None,
            nonce,
        })
    }

    fn collect_original_online_disarmed_observation(
        canonical_game: &Path,
    ) -> AppResult<CollectedDisarmedObservation> {
        let blockers_before = crate::win32::running_blockers()?;
        if !blockers_before.is_empty() {
            return Err(AppError::evidence(format!(
                "original-online-disarmed refuses blocker processes: {}",
                blockers_before.join(", ")
            )));
        }
        let pids = crate::win32::find_target_pids()?;
        if pids.len() != 1 {
            return Err(AppError::evidence(format!(
                "original-online-disarmed requires exactly one isaac-ng.exe process, found {}",
                pids.len()
            )));
        }
        let first_process = ObservedProcess::open_exact(pids[0], canonical_game)?;
        require_clean_disarmed_process(&first_process, "original-online-disarmed")?;
        let target_slots_before = capture_active_target_slots(&first_process)?;
        let first = crate::input_rpm::original_online_positive_snapshot(&first_process)?;

        thread::sleep(ORIGINAL_ONLINE_OBSERVATION_GAP);

        if crate::win32::running_blockers()? != blockers_before
            || crate::win32::find_target_pids()? != pids
        {
            return Err(AppError::evidence(
                "blocker or isaac-ng.exe process set changed during Original Online observation",
            ));
        }
        let process = ObservedProcess::open_exact(pids[0], canonical_game)?;
        require_clean_disarmed_process(&process, "original-online-disarmed")?;
        if process.creation_time_100ns != first_process.creation_time_100ns
            || process.modules != first_process.modules
        {
            return Err(AppError::evidence(
                "Original Online PID creation identity or module set changed between observations",
            ));
        }
        let second = crate::input_rpm::original_online_positive_snapshot(&process)?;
        let target_slots_after = capture_active_target_slots(&process)?;
        if target_slots_before != target_slots_after {
            return Err(AppError::evidence(
                "original-online-disarmed 45-slot snapshots changed during the read-only observation",
            ));
        }
        if first.game_root != second.game_root
            || first.manager != second.manager
            || first.network_backend != second.network_backend
            || first.roster_identities != second.roster_identities
            || !first.run_active
            || !second.run_active
            || second.online_frame < first.online_frame
            || second.qpc <= first.qpc
        {
            return Err(AppError::evidence(
                "Original Online positive signals were not stable across two read-only observations",
            ));
        }
        if process.wait(0)? {
            return Err(AppError::evidence(
                "target exited during Original Online read-only observation",
            ));
        }

        let nonce = loop_entropy(process.pid, 0)?;
        let identity_digest = |label: &[u8], value: &[u8]| {
            let mut hasher = Sha256::new();
            hasher.update(INPUT_TOOL_IDENTITY.as_bytes());
            hasher.update(label);
            hasher.update(process.pid.to_le_bytes());
            hasher.update(process.creation_time_100ns.to_le_bytes());
            hasher.update(nonce);
            hasher.update(value);
            hex::encode_upper(hasher.finalize())
        };
        let roster_bytes = first
            .roster_identities
            .iter()
            .flat_map(|identity| identity.to_le_bytes())
            .collect::<Vec<_>>();
        let original_online_observation = OriginalOnlineDisarmedObservation {
            schema_version: ORIGINAL_ONLINE_OBSERVATION_SCHEMA_VERSION,
            observation_count: 2,
            qpc_before: first.qpc,
            qpc_after: second.qpc,
            game_root_identity_sha256: identity_digest(
                b"original-online-game-root",
                &first.game_root.to_le_bytes(),
            ),
            manager_identity_sha256: identity_digest(
                b"original-online-manager",
                &first.manager.to_le_bytes(),
            ),
            network_backend_identity_sha256: identity_digest(
                b"original-online-network-backend",
                &first.network_backend.to_le_bytes(),
            ),
            roster_identity_sha256: identity_digest(b"original-online-roster", &roster_bytes),
            roster_count: u32::try_from(first.roster_identities.len())
                .map_err(|_| AppError::evidence("Original Online roster count overflow"))?,
            online_frame_before: first.online_frame,
            online_frame_after: second.online_frame,
            game_root_stable: true,
            run_active: true,
            manager_identity_stable: true,
            network_backend_present: true,
            roster_stable: true,
        };
        Ok(CollectedDisarmedObservation {
            process,
            target_slots_before,
            target_slots_after,
            observation_qpc: second.qpc,
            original_online_observation: Some(original_online_observation),
            nonce,
        })
    }

    fn require_clean_disarmed_process(process: &ObservedProcess, scenario: &str) -> AppResult<()> {
        if process.wait(0)?
            || process.debugger_present()?
            || !process.forbidden_modules().is_empty()
        {
            return Err(AppError::evidence(format!(
                "{scenario} process is not alive and clean or contains a debugger/forbidden module"
            )));
        }
        Ok(())
    }

    fn capture_active_target_slots(process: &ObservedProcess) -> AppResult<Vec<ActiveTargetSlot>> {
        expected_hook_slots()
            .into_iter()
            .map(|slot| {
                let rva = u32::from_str_radix(
                    slot.rva
                        .strip_prefix("0x")
                        .ok_or_else(|| AppError::evidence("active slot RVA lacks 0x prefix"))?,
                    16,
                )
                .map_err(|_| AppError::evidence("active slot RVA is not hexadecimal"))?;
                let address = process
                    .main_module
                    .base
                    .checked_add(u64::from(rva))
                    .ok_or_else(|| AppError::evidence("active slot address overflow"))?;
                let width = usize::try_from(slot.width)
                    .map_err(|_| AppError::evidence("active slot width overflow"))?;
                Ok(ActiveTargetSlot {
                    ordinal: slot.ordinal,
                    id: slot.id,
                    rva: slot.rva,
                    bytes_hex: hex::encode_upper(process.read_memory(address, width)?),
                })
            })
            .collect()
    }

    fn persist_active_failure(
        run: ClaimedActiveRun,
        reason: &str,
        slots_after: Option<Vec<ActiveTargetSlot>>,
        summary: ActiveRunFinish,
    ) -> AppResult<()> {
        run.fail(reason, slots_after, summary)
    }

    fn load_bound_script(
        scenario: &str,
        input_script: Option<&Path>,
        plan: &ValidatedActivePlan,
    ) -> AppResult<Option<RepeatabilityScript>> {
        match (scenario, input_script) {
            ("loopback", Some(path)) => {
                load_repeatability_script(path, &plan.repeatability_input_script_sha256).map(Some)
            }
            ("loopback", None) => Err(AppError::usage(
                "loopback repeatability scenario requires --input-script",
            )),
            (_, None) => Ok(None),
            (_, Some(_)) => Err(AppError::usage(
                "--input-script is accepted only by the loopback repeatability scenario",
            )),
        }
    }

    fn run_helper(
        helper: &Path,
        operation: &str,
        pid: u32,
        native: &Path,
        mapping: Option<&str>,
    ) -> AppResult<HelperOutcome> {
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
            .usage_context("cannot run independent T0-04 active helper")?;
        let stdout = String::from_utf8_lossy(&output.stdout);
        match classify_helper_outcome(
            output.status.success(),
            stdout.trim_start().starts_with("PASS"),
            output.status.code(),
        ) {
            HelperOutcome::Pass => return Ok(HelperOutcome::Pass),
            HelperOutcome::UnsafeDoNotUnload => {
                return Ok(HelperOutcome::UnsafeDoNotUnload);
            }
            HelperOutcome::Failure => {}
        }
        Err(AppError::evidence(format!(
            "T0-04 helper {operation} failed (exit={:?}): {}{}",
            output.status.code(),
            stdout.trim(),
            String::from_utf8_lossy(&output.stderr).trim()
        )))
    }

    fn artifact_identity(
        case_id: &str,
        scenario: &str,
        plan: &ValidatedActivePlan,
        observed_target_sha256: &str,
        target_equivalence_sha256: &str,
        nonce: &[u8; 32],
        arm_sha256: &str,
    ) -> String {
        let mut hasher = Sha256::new();
        hasher.update(INPUT_TOOL_IDENTITY.as_bytes());
        hasher.update(case_id.as_bytes());
        hasher.update(scenario.as_bytes());
        hasher.update(plan.target_sha256.as_bytes());
        hasher.update(observed_target_sha256.as_bytes());
        hasher.update(target_equivalence_sha256.as_bytes());
        hasher.update(plan.plan_sha256.as_bytes());
        hasher.update(arm_sha256.as_bytes());
        hasher.update(nonce);
        format!("active-runtime-{}", hex::encode_upper(hasher.finalize()))
    }

    #[allow(clippy::too_many_arguments)]
    fn disarmed_artifact_identity(
        case_id: &str,
        probe: &str,
        plan: &ValidatedActivePlan,
        observed_target_sha256: &str,
        target_equivalence_sha256: &str,
        pid: u32,
        creation_time_100ns: u64,
        nonce: &[u8; 32],
        original_online_observation: Option<&OriginalOnlineDisarmedObservation>,
    ) -> String {
        let mut hasher = Sha256::new();
        hasher.update(INPUT_TOOL_IDENTITY.as_bytes());
        hasher.update(b"DISARMED_CONTROL");
        hasher.update(case_id.as_bytes());
        hasher.update(probe.as_bytes());
        hasher.update(plan.target_sha256.as_bytes());
        hasher.update(observed_target_sha256.as_bytes());
        hasher.update(target_equivalence_sha256.as_bytes());
        hasher.update(plan.plan_sha256.as_bytes());
        hasher.update(pid.to_le_bytes());
        hasher.update(creation_time_100ns.to_le_bytes());
        hasher.update(nonce);
        if let Some(observation) = original_online_observation {
            hasher.update(
                serde_json::to_vec(observation)
                    .expect("Original Online observation serialization is infallible"),
            );
        }
        format!("disarmed-control-{}", hex::encode_upper(hasher.finalize()))
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
                "active input capture requires an existing artifact root and valid case id",
            ));
        }
        Ok(())
    }

    #[cfg(test)]
    mod tests {
        use super::*;

        #[test]
        fn send_input_abi_layout_matches_pointer_width() {
            assert_eq!(
                size_of::<Win32Input>(),
                if size_of::<usize>() == 8 { 40 } else { 28 }
            );
        }

        #[test]
        fn active_capture_stop_requires_input_bytes_and_ignores_eof() {
            let mut enter = io::Cursor::new(b"\n");
            assert!(read_explicit_stop_line(&mut enter).unwrap());

            let mut text_then_enter = io::Cursor::new(b"stop\r\n");
            assert!(read_explicit_stop_line(&mut text_then_enter).unwrap());

            let mut eof = io::Cursor::new(Vec::<u8>::new());
            assert!(!read_explicit_stop_line(&mut eof).unwrap());

            let mut partial_line_then_eof = io::Cursor::new(b"stop");
            assert!(!read_explicit_stop_line(&mut partial_line_then_eof).unwrap());
        }
    }
}

#[cfg(windows)]
#[allow(clippy::too_many_arguments)]
pub fn input_disarmed_control_command(
    probe: &str,
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
) -> AppResult<()> {
    live::run_disarmed_control(
        probe,
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
#[allow(clippy::too_many_arguments)]
pub fn input_disarmed_control_command(
    _probe: &str,
    _scenario: &str,
    _game_exe: &Path,
    _loop_ledger: &Path,
    _target_ledger: &Path,
    _target_equivalence: &Path,
    _mode_gate: &Path,
    _loop_gate: &Path,
    _raw_prereq: &Path,
    _active_catalog: &Path,
    _active_plan: &Path,
    _expected_variant: &str,
    _artifacts_root: &Path,
    _case_id: &str,
) -> AppResult<()> {
    Err(AppError::usage(
        "control-no-native live capture requires Windows",
    ))
}

#[cfg(windows)]
#[allow(clippy::too_many_arguments)]
pub fn input_active_smoke_command(
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
    offline_prereq: &Path,
    arm_grant: &Path,
    active_verification: &Path,
    input_script: Option<&Path>,
    expected_variant: &str,
    artifacts_root: &Path,
    case_id: &str,
    arm_token: &str,
) -> AppResult<()> {
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
        offline_prereq,
        arm_grant,
        active_verification,
        input_script,
        expected_variant,
        artifacts_root,
        case_id,
        arm_token,
    )
}

#[cfg(not(windows))]
#[allow(clippy::too_many_arguments)]
pub fn input_active_smoke_command(
    _scenario: &str,
    _game_exe: &Path,
    _loop_ledger: &Path,
    _target_ledger: &Path,
    _target_equivalence: &Path,
    _mode_gate: &Path,
    _loop_gate: &Path,
    _raw_prereq: &Path,
    _active_catalog: &Path,
    _active_plan: &Path,
    _offline_prereq: &Path,
    _arm_grant: &Path,
    _active_verification: &Path,
    _input_script: Option<&Path>,
    _expected_variant: &str,
    _artifacts_root: &Path,
    _case_id: &str,
    _arm_token: &str,
) -> AppResult<()> {
    Err(AppError::usage(
        "schema-4 active input capture requires Windows",
    ))
}

#[cfg(test)]
mod tests {
    use std::cell::RefCell;
    use std::rc::Rc;

    use super::*;

    #[derive(Clone, Default)]
    struct RecordingEmitter(Rc<RefCell<Vec<(ScriptKey, bool)>>>);

    impl KeyEmitter for RecordingEmitter {
        fn emit(&mut self, key: ScriptKey, down: bool) -> AppResult<()> {
            self.0.borrow_mut().push((key, down));
            Ok(())
        }
    }

    #[test]
    fn runtime_scenario_code_is_exact_and_excludes_disarmed_cases() {
        for (index, scenario) in ACTIVE_RUNTIME_SCENARIOS.iter().enumerate() {
            assert_eq!(runtime_scenario_code(scenario).unwrap(), index as u32 + 1);
        }
        assert!(runtime_scenario_code("control-no-native").is_err());
        assert!(runtime_scenario_code("original-online-disarmed").is_err());
    }

    #[test]
    fn controller_order_is_frozen_and_rejects_skips() {
        let mut order = ControllerOrder::after_static_validation();
        for stage in CONTROLLER_ORDER.iter().copied().skip(1) {
            order.advance(stage).unwrap();
        }
        assert_eq!(order.current(), Some(ControllerStage::ArtifactComplete));

        let mut skipped = ControllerOrder::after_static_validation();
        assert!(skipped.advance(ControllerStage::TokenCas).is_err());
    }

    #[test]
    fn key_state_sink_emits_deltas_and_raii_releases_every_pressed_key() {
        let events = Rc::new(RefCell::new(Vec::new()));
        {
            let emitter = RecordingEmitter(events.clone());
            let mut sink = KeyStateSink::new(emitter);
            sink.set_keys(&["A".to_owned(), "RIGHT".to_owned()])
                .unwrap();
            sink.set_keys(&["D".to_owned(), "RIGHT".to_owned()])
                .unwrap();
        }
        assert_eq!(
            *events.borrow(),
            vec![
                (ScriptKey::A, true),
                (ScriptKey::Right, true),
                (ScriptKey::A, false),
                (ScriptKey::D, true),
                (ScriptKey::D, false),
                (ScriptKey::Right, false),
            ]
        );
    }

    #[test]
    fn key_state_sink_rejects_unknown_keys_without_emitting() {
        let events = Rc::new(RefCell::new(Vec::new()));
        let emitter = RecordingEmitter(events.clone());
        let mut sink = KeyStateSink::new(emitter);
        assert!(sink.set_keys(&["SPACE".to_owned()]).is_err());
        assert!(events.borrow().is_empty());
    }

    #[test]
    fn helper_result_eight_and_unknown_state_forbid_automatic_unload() {
        assert_eq!(
            classify_helper_outcome(false, false, Some(8)),
            HelperOutcome::UnsafeDoNotUnload
        );
        assert_eq!(
            classify_helper_outcome(true, true, Some(0)),
            HelperOutcome::Pass
        );
        assert_eq!(
            classify_helper_outcome(false, false, Some(7)),
            HelperOutcome::Failure
        );
        assert!(automatic_unload_allowed(true, true, 0, 0));
        assert!(!automatic_unload_allowed(true, true, 8, 0));
        assert!(!automatic_unload_allowed(true, true, 0, 8));
        assert!(!automatic_unload_allowed(false, true, 0, 0));
        assert!(!automatic_unload_allowed(true, false, 0, 0));
    }

    #[test]
    fn ready_binding_waits_for_trusted_anchor_and_rejects_mismatch() {
        assert_eq!(
            ready_binding_status(
                crate::active_artifact::ACTIVE_HOOK_MASK,
                crate::loop_ipc::LOOP_HOOK_MASK,
                0,
                2024,
                0,
                0
            ),
            ReadyBindingStatus::AwaitingMainThread
        );
        assert_eq!(
            ready_binding_status(
                crate::active_artifact::ACTIVE_HOOK_MASK,
                crate::loop_ipc::LOOP_HOOK_MASK,
                2024,
                2024,
                0,
                0
            ),
            ReadyBindingStatus::Complete
        );
        assert_eq!(
            ready_binding_status(
                crate::active_artifact::ACTIVE_HOOK_MASK,
                crate::loop_ipc::LOOP_HOOK_MASK,
                2024,
                2025,
                0,
                0
            ),
            ReadyBindingStatus::Invalid
        );
        assert_eq!(
            ready_binding_status(
                crate::active_artifact::ACTIVE_HOOK_MASK,
                crate::loop_ipc::LOOP_HOOK_MASK,
                2024,
                2024,
                1,
                0
            ),
            ReadyBindingStatus::Invalid
        );
        assert_eq!(
            ready_binding_status(
                crate::active_artifact::ACTIVE_HOOK_MASK,
                crate::loop_ipc::LOOP_HOOK_MASK,
                2024,
                0,
                0,
                0
            ),
            ReadyBindingStatus::Invalid
        );
        assert_eq!(
            ReadyBindingSnapshot {
                active_hooks: crate::active_artifact::ACTIVE_HOOK_MASK,
                loop_hooks: crate::loop_ipc::LOOP_HOOK_MASK,
                active_main_thread: 2024,
                loop_main_thread: 2024,
                active_overwrites: 0,
                loop_overwrites: 0,
            }
            .status(),
            ReadyBindingStatus::Complete
        );
    }
}
