use std::collections::{BTreeMap, BTreeSet};
use std::ffi::OsString;
use std::fs;
use std::path::{Path, PathBuf};
use std::thread;
use std::time::{Duration, Instant};

use crate::artifact::{
    ARTIFACT_SCHEMA_VERSION, ArtifactRun, LiveProcessSnapshot, LiveTargetSnapshot, ModuleSnapshot,
    TargetBytesDocument, analyze_run, file_sha256, filetime_100ns_to_utc, utc_now, write_analysis,
};
use crate::error::{AppError, AppResult, UsageContext};
use crate::ledger::{LoadedLedger, StaticInspectionReport, TargetSpec};
use crate::pe::PeImage;
#[cfg(windows)]
use crate::win32::{self, ObservedProcess};

const WAIT_FOR_PROCESS: Duration = Duration::from_secs(300);
const READY_STABILITY: Duration = Duration::from_secs(2);
const SAMPLE_INTERVAL: Duration = Duration::from_millis(500);
const MAX_OBSERVATION: Duration = Duration::from_secs(30 * 60);
const FIXED_SMOKE_VARIANT: &str = "online-mod-patched-2026-04-21";

pub enum Command {
    CoopJoinPreflight {
        game_exe: PathBuf,
        repository_root: PathBuf,
        output: Option<PathBuf>,
    },
    Inspect {
        game_exe: PathBuf,
        ledger: PathBuf,
        expected_variant: String,
    },
    Smoke {
        game_exe: PathBuf,
        ledger: PathBuf,
        expected_variant: String,
        artifacts_root: PathBuf,
        case_id: String,
    },
    Analyze {
        run_directory: PathBuf,
    },
    ModeInspect {
        game_exe: PathBuf,
        ledger: PathBuf,
        expected_variant: String,
    },
    ModeSmoke {
        scenario: String,
        endpoint: String,
        game_exe: PathBuf,
        ledger: PathBuf,
        expected_variant: String,
        artifacts_root: PathBuf,
        case_id: String,
    },
    ModeAnalyze {
        run_directory: PathBuf,
    },
    ModeCompare {
        control: PathBuf,
        instrumented: PathBuf,
        peer: Option<PathBuf>,
        output: Option<PathBuf>,
    },
    ModeGate {
        baseline: PathBuf,
        offline_control: PathBuf,
        offline_instrumented: PathBuf,
        control_a: PathBuf,
        control_b: PathBuf,
        instrumented_a: PathBuf,
        instrumented_b: PathBuf,
        comparison: PathBuf,
        output: PathBuf,
    },
    BoundaryInspect {
        game_exe: PathBuf,
        ledger: PathBuf,
        expected_variant: String,
    },
    BoundarySmoke {
        probe: String,
        scenario: String,
        game_exe: PathBuf,
        ledger: PathBuf,
        expected_variant: String,
        artifacts_root: PathBuf,
        case_id: String,
    },
    BoundaryAnalyze {
        run_directory: PathBuf,
    },
    BoundaryCompare {
        control: PathBuf,
        instrumented: PathBuf,
        output: Option<PathBuf>,
    },
    LoopInspect {
        game_exe: PathBuf,
        ledger: PathBuf,
        expected_variant: String,
    },
    LoopSmoke {
        probe: String,
        scenario: String,
        game_exe: PathBuf,
        ledger: PathBuf,
        expected_variant: String,
        artifacts_root: PathBuf,
        case_id: String,
    },
    LoopAnalyze {
        run_directory: PathBuf,
    },
    LoopGate {
        matrix: PathBuf,
        output: PathBuf,
    },
    CoopInspect {
        game_exe: PathBuf,
        target_equivalence: PathBuf,
        bootstrap_decision: PathBuf,
        ledger: PathBuf,
        catalog: PathBuf,
        graph: PathBuf,
        output: Option<PathBuf>,
    },
    CoopSmoke {
        probe: String,
        scenario: String,
        game_exe: PathBuf,
        target_equivalence: PathBuf,
        layout_evidence: PathBuf,
        catalog: PathBuf,
        graph: PathBuf,
        plan: PathBuf,
        verification: Option<PathBuf>,
        artifacts_root: PathBuf,
        case_id: String,
        duration_milliseconds: u64,
        interval_milliseconds: u64,
    },
    CoopAnalyze {
        run_directory: PathBuf,
        output: PathBuf,
    },
    CoopCompare {
        offline_single: PathBuf,
        local_coop_dual: PathBuf,
        original_online_dual: PathBuf,
        output: PathBuf,
    },
    CoopC0Gate {
        offline_single: PathBuf,
        local_coop_dual: PathBuf,
        original_online_dual: PathBuf,
        comparison: PathBuf,
        loop_gate: PathBuf,
        active_decision: PathBuf,
        output: PathBuf,
    },
    CoopPassiveGate {
        native_join: PathBuf,
        native_exit: PathBuf,
        death_respawn: PathBuf,
        room_rebind: PathBuf,
        device_rebind: PathBuf,
        menu_teardown: PathBuf,
        output: PathBuf,
    },
    CoopPassiveApplicabilityGate {
        repository_root: PathBuf,
        applicability: PathBuf,
        native_exit_source: PathBuf,
        sources: [PathBuf; 5],
        verification: Option<PathBuf>,
        output: PathBuf,
    },
    InputInspect {
        game_exe: PathBuf,
        loop_ledger: PathBuf,
        expected_variant: String,
    },
    InputTargetEquivalence {
        game_exe: PathBuf,
        loop_ledger: PathBuf,
        target_ledger: PathBuf,
        active_catalog: PathBuf,
        active_plan: PathBuf,
        output: PathBuf,
    },
    InputSmoke {
        probe: String,
        scenario: String,
        game_exe: PathBuf,
        loop_ledger: PathBuf,
        target_ledger: Option<PathBuf>,
        target_equivalence: Option<PathBuf>,
        mode_gate: Option<PathBuf>,
        loop_gate: Option<PathBuf>,
        raw_prereq: Option<PathBuf>,
        active_catalog: Option<PathBuf>,
        active_plan: Option<PathBuf>,
        offline_prereq: Option<PathBuf>,
        arm_grant: Option<PathBuf>,
        active_verification: Option<PathBuf>,
        input_script: Option<PathBuf>,
        expected_variant: String,
        artifacts_root: PathBuf,
        case_id: String,
        arm_token: Option<String>,
    },
    InputOfflineAttest {
        scenario: String,
        game_exe: PathBuf,
        loop_ledger: PathBuf,
        target_ledger: PathBuf,
        target_equivalence: PathBuf,
        mode_gate: PathBuf,
        loop_gate: PathBuf,
        raw_prereq: PathBuf,
        active_catalog: PathBuf,
        active_plan: PathBuf,
        expected_variant: String,
        artifacts_root: PathBuf,
        case_id: String,
    },
    InputArmGrant {
        offline_prereq: PathBuf,
        case_id: String,
        scenario: String,
        arm_token: String,
        output: PathBuf,
    },
    InputRawSmoke {
        probe: String,
        scenario: String,
        game_exe: PathBuf,
        loop_ledger: PathBuf,
        loop_gate: PathBuf,
        raw_apply_catalog: PathBuf,
        capture_plan: PathBuf,
        expected_variant: String,
        artifacts_root: PathBuf,
        case_id: String,
    },
    InputRawPrereq {
        matrix: PathBuf,
        output: PathBuf,
    },
    InputAnalyze {
        run_directory: PathBuf,
    },
    InputImportWitness {
        witness_log: PathBuf,
        output: PathBuf,
        scenario: String,
        case_id: String,
        target_sha256: String,
        loop_gate_sha256: String,
        window_min: Option<u64>,
        window_max: Option<u64>,
        controller_index: Option<i32>,
    },
    InputRpmInspect {
        game_exe: PathBuf,
        pid: Option<u32>,
    },
    InputRpmCapture {
        game_exe: PathBuf,
        pid: Option<u32>,
        output: PathBuf,
        duration_milliseconds: u64,
        interval_milliseconds: u64,
    },
    InputRpmAnalyze {
        run_directory: PathBuf,
    },
    InputCompare {
        first: PathBuf,
        second: PathBuf,
        output: Option<PathBuf>,
    },
    InputGate {
        mode_gate: Option<PathBuf>,
        loop_gate: PathBuf,
        raw_prereq: Option<PathBuf>,
        active_plan: Option<PathBuf>,
        active_verification: Option<PathBuf>,
        matrix: PathBuf,
        repeatability: PathBuf,
        output: PathBuf,
    },
}

#[derive(serde::Serialize)]
#[serde(rename_all = "camelCase")]
struct InputInspectionOutput {
    schema_version: u32,
    result: String,
    codec_self_test: String,
    core_manifest: crate::input_artifact::InputConstants,
    loop_inspection: crate::loop_verification::LoopInspectionReport,
    passive_probe_allowed: bool,
    active_input_allowed: bool,
    next_checkpoint: String,
}

#[derive(serde::Serialize)]
#[serde(rename_all = "camelCase")]
struct ManualCheckpointOutput {
    schema_version: u32,
    result: String,
    probe: String,
    scenario: String,
    game_started: bool,
    native_injected: bool,
    mod_deployed: bool,
    required_user_actions: Vec<String>,
}

#[derive(serde::Serialize)]
#[serde(rename_all = "camelCase")]
struct ActiveControllerPreflightOutput {
    schema_version: u32,
    result: String,
    probe: String,
    scenario: String,
    static_bundle_validated: bool,
    selected_target_bound: bool,
    selected_loop_ledger_bound: bool,
    input_script_required: bool,
    input_script_validated: bool,
    offline_prerequisite_validated: bool,
    generation_continuity_binding_validated: bool,
    arm_grant_validated: bool,
    active_verification_validated: bool,
    semantic_contract_complete: bool,
    runtime_contract_complete: bool,
    active_input_allowed: bool,
    game_started: bool,
    native_injected: bool,
    arm_token_consumed: bool,
    mapping_created: bool,
    artifact_created: bool,
    next_checkpoint: String,
    dynamic_preflight: crate::active_preflight::ActivePreflightReport,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum InputArtifactDispatch {
    InputV2,
    RawApplyV3,
    ActiveV4,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum InputGateDispatch {
    Legacy,
    ActiveV2,
}

fn classify_input_artifact(
    schema_version: u32,
    evidence_class: &str,
) -> AppResult<InputArtifactDispatch> {
    match (schema_version, evidence_class) {
        (
            crate::active_artifact::ACTIVE_ARTIFACT_SCHEMA_VERSION,
            crate::input_artifact::EVIDENCE_PIPELINE_RUNTIME
            | crate::input_artifact::EVIDENCE_DISARMED_CONTROL,
        ) => Ok(InputArtifactDispatch::ActiveV4),
        (crate::active_artifact::ACTIVE_ARTIFACT_SCHEMA_VERSION, _) => Err(AppError::evidence(
            "schema-4 input artifact has an invalid active evidence class",
        )),
        (
            crate::raw_apply_artifact::RAW_APPLY_ARTIFACT_SCHEMA_VERSION,
            crate::raw_apply_artifact::EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME,
        ) => Ok(InputArtifactDispatch::RawApplyV3),
        (crate::raw_apply_artifact::RAW_APPLY_ARTIFACT_SCHEMA_VERSION, _)
        | (_, crate::raw_apply_artifact::EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME) => Err(
            AppError::evidence("input artifact crossed schema/evidence identity"),
        ),
        (crate::input_artifact::INPUT_ARTIFACT_SCHEMA_VERSION, _) => {
            Ok(InputArtifactDispatch::InputV2)
        }
        _ => Err(AppError::evidence("unknown input artifact schema")),
    }
}

fn classify_input_gate_matrix(path: &Path) -> AppResult<InputGateDispatch> {
    let bytes =
        fs::read(path).usage_context(format!("cannot read input matrix {}", path.display()))?;
    let value: serde_json::Value = serde_json::from_slice(&bytes)
        .usage_context(format!("cannot parse input matrix {}", path.display()))?;
    match (value.get("schemaVersion"), value.get("matrixIdentity")) {
        (None, None) => Ok(InputGateDispatch::Legacy),
        (Some(serde_json::Value::Number(number)), Some(serde_json::Value::String(identity)))
            if number.as_u64()
                == Some(crate::active_artifact::ACTIVE_MATRIX_SCHEMA_VERSION.into())
                && identity == crate::active_artifact::ACTIVE_MATRIX_IDENTITY =>
        {
            Ok(InputGateDispatch::ActiveV2)
        }
        _ => Err(AppError::evidence(
            "input gate matrix has an unknown or malformed schema identity",
        )),
    }
}

fn validate_input_gate_option_scope(
    dispatch: InputGateDispatch,
    mode_gate: Option<&Path>,
    raw_prereq: Option<&Path>,
    active_plan: Option<&Path>,
    active_verification: Option<&Path>,
) -> AppResult<()> {
    match dispatch {
        InputGateDispatch::ActiveV2 => {
            for (name, present) in [
                ("--mode-gate", mode_gate.is_some()),
                ("--raw-prereq", raw_prereq.is_some()),
                ("--active-plan", active_plan.is_some()),
                ("--active-verification", active_verification.is_some()),
            ] {
                if !present {
                    return Err(AppError::usage(format!(
                        "schema-4 active input gate requires {name}"
                    )));
                }
            }
        }
        InputGateDispatch::Legacy => {
            if mode_gate.is_some()
                || raw_prereq.is_some()
                || active_plan.is_some()
                || active_verification.is_some()
            {
                return Err(AppError::usage(
                    "--mode-gate, --raw-prereq, --active-plan, and --active-verification are accepted only by schema-2 active matrices",
                ));
            }
        }
    }
    Ok(())
}

pub fn run_from_env() -> AppResult<()> {
    run(parse_args(std::env::args_os().skip(1).collect())?)
}

pub fn run(command: Command) -> AppResult<()> {
    match command {
        Command::CoopJoinPreflight {
            game_exe,
            repository_root,
            output,
        } => {
            let report =
                crate::coop_native_join::preflight(&game_exe, &repository_root, output.as_deref())?;
            print_json(&report)?;
            Err(AppError::evidence(
                "native join remains BLOCKED_BY_EVIDENCE at the static prerequisite; no process, arm or injection performed",
            ))
        }
        Command::Inspect {
            game_exe,
            ledger,
            expected_variant,
        } => inspect_command(&game_exe, &ledger, &expected_variant),
        Command::Smoke {
            game_exe,
            ledger,
            expected_variant,
            artifacts_root,
            case_id,
        } => smoke_command(
            &game_exe,
            &ledger,
            &expected_variant,
            &artifacts_root,
            &case_id,
        ),
        Command::Analyze { run_directory } => analyze_command(&run_directory),
        Command::ModeInspect {
            game_exe,
            ledger,
            expected_variant,
        } => crate::mode_commands::mode_inspect_command(&game_exe, &ledger, &expected_variant),
        Command::ModeSmoke {
            scenario,
            endpoint,
            game_exe,
            ledger,
            expected_variant,
            artifacts_root,
            case_id,
        } => crate::mode_commands::mode_smoke_command(
            &scenario,
            &endpoint,
            &game_exe,
            &ledger,
            &expected_variant,
            &artifacts_root,
            &case_id,
        ),
        Command::ModeAnalyze { run_directory } => {
            crate::mode_commands::mode_analyze_command(&run_directory)
        }
        Command::ModeCompare {
            control,
            instrumented,
            peer,
            output,
        } => crate::mode_commands::mode_compare_command(
            &control,
            &instrumented,
            peer.as_deref(),
            output.as_deref(),
        ),
        Command::ModeGate {
            baseline,
            offline_control,
            offline_instrumented,
            control_a,
            control_b,
            instrumented_a,
            instrumented_b,
            comparison,
            output,
        } => {
            let decision = crate::mode_gate::write_gate_decision(
                &crate::mode_gate::GateInputs {
                    baseline: &baseline,
                    offline_control: &offline_control,
                    offline_instrumented: &offline_instrumented,
                    control_a: &control_a,
                    control_b: &control_b,
                    instrumented_a: &instrumented_a,
                    instrumented_b: &instrumented_b,
                    comparison: &comparison,
                },
                &output,
            )?;
            print_json(&decision)
        }
        Command::BoundaryInspect {
            game_exe,
            ledger,
            expected_variant,
        } => crate::boundary_commands::boundary_inspect_command(
            &game_exe,
            &ledger,
            &expected_variant,
        ),
        Command::BoundarySmoke {
            probe,
            scenario,
            game_exe,
            ledger,
            expected_variant,
            artifacts_root,
            case_id,
        } => crate::boundary_commands::boundary_smoke_command(
            &probe,
            &scenario,
            &game_exe,
            &ledger,
            &expected_variant,
            &artifacts_root,
            &case_id,
        ),
        Command::BoundaryAnalyze { run_directory } => {
            crate::boundary_commands::boundary_analyze_command(&run_directory)
        }
        Command::BoundaryCompare {
            control,
            instrumented,
            output,
        } => crate::boundary_commands::boundary_compare_command(
            &control,
            &instrumented,
            output.as_deref(),
        ),
        Command::LoopInspect {
            game_exe,
            ledger,
            expected_variant,
        } => {
            let report =
                crate::loop_verification::inspect_loop(&game_exe, &ledger, &expected_variant)?;
            print_json(&report)
        }
        Command::LoopSmoke {
            probe,
            scenario,
            game_exe,
            ledger,
            expected_variant,
            artifacts_root,
            case_id,
        } => {
            if probe == "passive" {
                crate::loop_commands::loop_smoke_command(
                    &probe,
                    &scenario,
                    &game_exe,
                    &ledger,
                    &expected_variant,
                    &artifacts_root,
                    &case_id,
                )
            } else {
                manual_loop_checkpoint(
                    &probe,
                    &scenario,
                    &game_exe,
                    &ledger,
                    &expected_variant,
                    &artifacts_root,
                    &case_id,
                )
            }
        }
        Command::LoopAnalyze { run_directory } => {
            let report = crate::loop_verification::replay_loop_run(&run_directory)?;
            print_json(&report)?;
            if report.result == "PASS" {
                Ok(())
            } else {
                Err(AppError::evidence("T0-03R loop analysis failed"))
            }
        }
        Command::LoopGate { matrix, output } => {
            let decision = crate::loop_verification::write_loop_gate(&matrix, &output)?;
            print_json(&decision)?;
            if decision.status == "PASS" {
                Ok(())
            } else {
                Err(AppError::evidence("T0-03R remains BLOCKED_BY_EVIDENCE"))
            }
        }
        Command::CoopInspect {
            game_exe,
            target_equivalence,
            bootstrap_decision,
            ledger,
            catalog,
            graph,
            output,
        } => {
            let inspection = crate::coop_inspect::inspect_coop_static(
                &game_exe,
                &target_equivalence,
                &bootstrap_decision,
                &ledger,
                &catalog,
                &graph,
                output.as_deref(),
            )?;
            print_json(&inspection)
        }
        Command::CoopSmoke {
            probe,
            scenario,
            game_exe,
            target_equivalence,
            layout_evidence,
            catalog,
            graph,
            plan,
            verification,
            artifacts_root,
            case_id,
            duration_milliseconds,
            interval_milliseconds,
        } => match verification {
            Some(verification) => {
                let summary = crate::coop_passive_commands::capture(
                    &crate::coop_passive_commands::PassiveCaptureRequest {
                        probe: &probe,
                        scenario: &scenario,
                        game_exe: &game_exe,
                        target_equivalence: &target_equivalence,
                        layout_evidence: &layout_evidence,
                        catalog: &catalog,
                        graph: &graph,
                        plan: &plan,
                        verification: &verification,
                        artifacts_root: &artifacts_root,
                        case_id: &case_id,
                        duration_milliseconds,
                    },
                )?;
                print_json(&summary)?;
                if summary.result == "CAPTURE_COMPLETE" {
                    Ok(())
                } else {
                    Err(AppError::evidence(
                        "coop passive capture is BLOCKED_BY_EVIDENCE",
                    ))
                }
            }
            None => {
                let summary = crate::coop_c0::capture(&crate::coop_c0::CaptureRequest {
                    probe: &probe,
                    scenario: &scenario,
                    game_exe: &game_exe,
                    target_equivalence: &target_equivalence,
                    layout_evidence: &layout_evidence,
                    catalog: &catalog,
                    graph: &graph,
                    plan: &plan,
                    artifacts_root: &artifacts_root,
                    case_id: &case_id,
                    duration_milliseconds,
                    interval_milliseconds,
                })?;
                print_json(&summary)?;
                if summary.result == "CAPTURE_COMPLETE" {
                    Ok(())
                } else {
                    Err(AppError::evidence("coop C0 capture is BLOCKED_BY_EVIDENCE"))
                }
            }
        },
        Command::CoopAnalyze {
            run_directory,
            output,
        } => {
            let report = if crate::coop_passive::is_passive_run(&run_directory)? {
                serde_json::to_value(crate::coop_passive::analyze_passive_run_to(
                    &run_directory,
                    &output,
                )?)
                .usage_context("cannot serialize coop passive analysis")?
            } else {
                serde_json::to_value(crate::coop_c0::analyze_to(&run_directory, &output)?)
                    .usage_context("cannot serialize coop C0 analysis")?
            };
            print_json(&report)?;
            if report.get("result").and_then(serde_json::Value::as_str) == Some("PASS") {
                Ok(())
            } else {
                Err(AppError::evidence("coop analysis is BLOCKED_BY_EVIDENCE"))
            }
        }
        Command::CoopCompare {
            offline_single,
            local_coop_dual,
            original_online_dual,
            output,
        } => {
            let report = crate::coop_c0::compare_to(
                &offline_single,
                &local_coop_dual,
                &original_online_dual,
                &output,
            )?;
            print_json(&report)?;
            if report.result == "PASS" {
                Ok(())
            } else {
                Err(AppError::evidence(
                    "coop C0 comparison is BLOCKED_BY_EVIDENCE",
                ))
            }
        }
        Command::CoopC0Gate {
            offline_single,
            local_coop_dual,
            original_online_dual,
            comparison,
            loop_gate,
            active_decision,
            output,
        } => {
            let decision = crate::coop_c0::gate_to(
                &offline_single,
                &local_coop_dual,
                &original_online_dual,
                &comparison,
                &loop_gate,
                &active_decision,
                &output,
            )?;
            print_json(&decision)?;
            if decision.status == "PASS" {
                Ok(())
            } else {
                Err(AppError::evidence(
                    "CS-T0-05-02 remains BLOCKED_BY_EVIDENCE",
                ))
            }
        }
        Command::CoopPassiveGate {
            native_join,
            native_exit,
            death_respawn,
            room_rebind,
            device_rebind,
            menu_teardown,
            output,
        } => {
            let decision = crate::coop_passive::passive_gate_to(
                [
                    &native_join,
                    &native_exit,
                    &death_respawn,
                    &room_rebind,
                    &device_rebind,
                    &menu_teardown,
                ],
                &output,
            )?;
            print_json(&decision)?;
            if decision.status == "PASS" {
                Ok(())
            } else {
                Err(AppError::evidence(
                    "CS-T0-05-03 remains BLOCKED_BY_EVIDENCE",
                ))
            }
        }
        Command::CoopPassiveApplicabilityGate {
            repository_root,
            applicability,
            native_exit_source,
            sources,
            verification,
            output,
        } => {
            let decision = crate::coop_passive_applicability::gate_to(
                &repository_root,
                &applicability,
                &native_exit_source,
                sources.each_ref().map(PathBuf::as_path),
                verification.as_deref(),
                &output,
            )?;
            print_json(&decision)
        }
        Command::InputInspect {
            game_exe,
            loop_ledger,
            expected_variant,
        } => {
            let loop_inspection =
                crate::loop_verification::inspect_loop(&game_exe, &loop_ledger, &expected_variant)?;
            crate::input_core::run_codec_self_test()
                .map_err(|error| AppError::evidence(error.to_string()))?;
            print_json(&InputInspectionOutput {
                schema_version: 1,
                result: "PASS_STATIC".to_owned(),
                codec_self_test: "PASS".to_owned(),
                core_manifest: crate::input_artifact::InputConstants::default(),
                passive_probe_allowed: loop_inspection.passive_probe_allowed,
                active_input_allowed: false,
                next_checkpoint: "OBSERVATION_ONLY_PASSIVE_CANARY".to_owned(),
                loop_inspection,
            })
        }
        Command::InputTargetEquivalence {
            game_exe,
            loop_ledger,
            target_ledger,
            active_catalog,
            active_plan,
            output,
        } => {
            let document = crate::target_equivalence::create_target_equivalence(
                &game_exe,
                &loop_ledger,
                &target_ledger,
                &active_catalog,
                &active_plan,
                &output,
            )?;
            print_json(&document)
        }
        Command::InputSmoke {
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
            offline_prereq,
            arm_grant,
            active_verification,
            input_script,
            expected_variant,
            artifacts_root,
            case_id,
            arm_token,
        } => {
            validate_input_smoke_option_scope(
                &probe,
                &scenario,
                target_ledger.as_deref(),
                target_equivalence.as_deref(),
                mode_gate.as_deref(),
                loop_gate.as_deref(),
                raw_prereq.as_deref(),
                active_catalog.as_deref(),
                active_plan.as_deref(),
                offline_prereq.as_deref(),
                arm_grant.as_deref(),
                active_verification.as_deref(),
                arm_token.as_deref(),
                input_script.as_deref(),
            )?;
            if probe == "passive" {
                crate::input_commands::input_smoke_command(
                    &probe,
                    &scenario,
                    &game_exe,
                    &loop_ledger,
                    loop_gate.as_deref(),
                    &expected_variant,
                    &artifacts_root,
                    &case_id,
                    arm_token.as_deref(),
                )
            } else if probe == "loopback" {
                crate::active_commands::input_active_smoke_command(
                    &scenario,
                    &game_exe,
                    &loop_ledger,
                    target_ledger
                        .as_deref()
                        .expect("loopback target ledger checked by option scope"),
                    target_equivalence
                        .as_deref()
                        .expect("loopback target equivalence checked by option scope"),
                    mode_gate
                        .as_deref()
                        .expect("loopback mode gate checked by option scope"),
                    loop_gate
                        .as_deref()
                        .expect("loopback loop gate checked by option scope"),
                    raw_prereq
                        .as_deref()
                        .expect("loopback raw prerequisite checked by option scope"),
                    active_catalog
                        .as_deref()
                        .expect("loopback active catalog checked by option scope"),
                    active_plan
                        .as_deref()
                        .expect("loopback active plan checked by option scope"),
                    offline_prereq
                        .as_deref()
                        .expect("loopback offline prerequisite checked by option scope"),
                    arm_grant
                        .as_deref()
                        .expect("loopback arm grant checked by option scope"),
                    active_verification
                        .as_deref()
                        .expect("loopback verification checked by option scope"),
                    input_script.as_deref(),
                    &expected_variant,
                    &artifacts_root,
                    &case_id,
                    arm_token
                        .as_deref()
                        .expect("loopback arm token checked by option scope"),
                )
            } else if matches!(
                probe.as_str(),
                "control-no-native" | "original-online-disarmed"
            ) {
                crate::active_commands::input_disarmed_control_command(
                    &probe,
                    &scenario,
                    &game_exe,
                    &loop_ledger,
                    target_ledger
                        .as_deref()
                        .expect("disarmed target ledger checked by option scope"),
                    target_equivalence
                        .as_deref()
                        .expect("disarmed target equivalence checked by option scope"),
                    mode_gate
                        .as_deref()
                        .expect("disarmed mode gate checked by option scope"),
                    loop_gate
                        .as_deref()
                        .expect("disarmed loop gate checked by option scope"),
                    raw_prereq
                        .as_deref()
                        .expect("disarmed raw prerequisite checked by option scope"),
                    active_catalog
                        .as_deref()
                        .expect("disarmed active catalog checked by option scope"),
                    active_plan
                        .as_deref()
                        .expect("disarmed active plan checked by option scope"),
                    &expected_variant,
                    &artifacts_root,
                    &case_id,
                )
            } else {
                manual_input_checkpoint(
                    &probe,
                    &scenario,
                    &game_exe,
                    &loop_ledger,
                    mode_gate.as_deref(),
                    loop_gate.as_deref(),
                    raw_prereq.as_deref(),
                    active_plan.as_deref(),
                    offline_prereq.as_deref(),
                    arm_grant.as_deref(),
                    active_verification.as_deref(),
                    input_script.as_deref(),
                    &expected_variant,
                    &artifacts_root,
                    &case_id,
                    arm_token.as_deref(),
                )
            }
        }
        Command::InputOfflineAttest {
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
        } => {
            let decision = crate::offline_attestation_commands::input_offline_attest_command(
                &scenario,
                &game_exe,
                &loop_ledger,
                &target_ledger,
                &target_equivalence,
                &mode_gate,
                &loop_gate,
                &raw_prereq,
                &active_catalog,
                &active_plan,
                &expected_variant,
                &artifacts_root,
                &case_id,
            )?;
            print_json(&decision)
        }
        Command::InputArmGrant {
            offline_prereq,
            case_id,
            scenario,
            arm_token,
            output,
        } => {
            let grant = crate::arm_token::create_arm_grant(
                &offline_prereq,
                &case_id,
                &scenario,
                &arm_token,
                &output,
                chrono::Utc::now(),
            )?;
            print_json(&grant)
        }
        Command::InputRawSmoke {
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
        } => crate::raw_apply_commands::input_raw_smoke_command(
            &probe,
            &scenario,
            &game_exe,
            &loop_ledger,
            &loop_gate,
            &raw_apply_catalog,
            &capture_plan,
            &expected_variant,
            &artifacts_root,
            &case_id,
        ),
        Command::InputRawPrereq { matrix, output } => {
            let decision = crate::active_catalog::write_raw_prerequisite(&matrix, &output)?;
            print_json(&decision)
        }
        Command::InputAnalyze { run_directory } => {
            let (schema, evidence) =
                crate::raw_apply_artifact::raw_apply_run_identity(&run_directory)?;
            match classify_input_artifact(schema, &evidence)? {
                InputArtifactDispatch::RawApplyV3 => {
                    let report = crate::raw_apply_artifact::analyze_raw_apply_run(&run_directory)?;
                    print_json(&report)?;
                    if report.result == "PASS" {
                        Ok(())
                    } else {
                        Err(AppError::evidence("T0-04 raw/apply analysis failed"))
                    }
                }
                InputArtifactDispatch::InputV2 => {
                    let report = crate::input_artifact::analyze_input_run(&run_directory)?;
                    print_json(&report)?;
                    if report.result == "PASS" {
                        Ok(())
                    } else {
                        Err(AppError::evidence("T0-04 input analysis failed"))
                    }
                }
                InputArtifactDispatch::ActiveV4 => {
                    let report = crate::active_artifact::analyze_active_run(&run_directory)?;
                    print_json(&report)?;
                    if report.result == "PASS" {
                        Ok(())
                    } else {
                        Err(AppError::evidence("T0-04 schema-4 active analysis failed"))
                    }
                }
            }
        }
        Command::InputImportWitness {
            witness_log,
            output,
            scenario,
            case_id,
            target_sha256,
            loop_gate_sha256,
            window_min,
            window_max,
            controller_index,
        } => {
            let report = crate::input_artifact::import_witness_log_filtered(
                &witness_log,
                &output,
                &scenario,
                &case_id,
                &target_sha256,
                &loop_gate_sha256,
                crate::input_artifact::WitnessImportFilter {
                    window_min,
                    window_max,
                    controller_index,
                },
            )?;
            print_json(&report)
        }
        Command::InputRpmInspect { game_exe, pid } => {
            #[cfg(windows)]
            {
                let report = crate::input_rpm::inspect(&game_exe, pid)?;
                print_json(&report)?;
                require_rpm_diagnostic_pass(&report.result, "inspect")
            }
            #[cfg(not(windows))]
            {
                let _ = (game_exe, pid);
                Err(AppError::usage("input-rpm-inspect requires Windows"))
            }
        }
        Command::InputRpmCapture {
            game_exe,
            pid,
            output,
            duration_milliseconds,
            interval_milliseconds,
        } => {
            #[cfg(windows)]
            {
                let report = crate::input_rpm::capture(
                    &game_exe,
                    pid,
                    &output,
                    duration_milliseconds,
                    interval_milliseconds,
                )?;
                print_json(&report)?;
                require_rpm_diagnostic_pass(&report.result, "capture")
            }
            #[cfg(not(windows))]
            {
                let _ = (
                    game_exe,
                    pid,
                    output,
                    duration_milliseconds,
                    interval_milliseconds,
                );
                Err(AppError::usage("input-rpm-capture requires Windows"))
            }
        }
        Command::InputRpmAnalyze { run_directory } => {
            #[cfg(windows)]
            {
                let report = crate::input_rpm::analyze(&run_directory)?;
                print_json(&report)?;
                if report.result == "PASS_DIAGNOSTIC" {
                    Ok(())
                } else {
                    Err(AppError::evidence("T0-04 RPM diagnostic analysis failed"))
                }
            }
            #[cfg(not(windows))]
            {
                let _ = run_directory;
                Err(AppError::usage("input-rpm-analyze requires Windows"))
            }
        }
        Command::InputCompare {
            first,
            second,
            output,
        } => {
            let first_identity = crate::raw_apply_artifact::raw_apply_run_identity(&first)?;
            let second_identity = crate::raw_apply_artifact::raw_apply_run_identity(&second)?;
            let first_dispatch = classify_input_artifact(first_identity.0, &first_identity.1)?;
            let second_dispatch = classify_input_artifact(second_identity.0, &second_identity.1)?;
            if first_dispatch != second_dispatch {
                return Err(AppError::evidence(
                    "input comparison sources use different artifact schemas",
                ));
            }
            match first_dispatch {
                InputArtifactDispatch::ActiveV4 => {
                    let report = crate::active_artifact::compare_active_runs(
                        &first,
                        &second,
                        output.as_deref(),
                    )?;
                    print_json(&report)?;
                    if report.result == "PASS" {
                        Ok(())
                    } else {
                        Err(AppError::evidence(
                            "T0-04 schema-4 repeatability comparison failed",
                        ))
                    }
                }
                InputArtifactDispatch::InputV2 => {
                    let report = crate::input_artifact::compare_input_runs(
                        &first,
                        &second,
                        output.as_deref(),
                    )?;
                    print_json(&report)?;
                    if report.result == "PASS" {
                        Ok(())
                    } else {
                        Err(AppError::evidence("T0-04 repeatability comparison failed"))
                    }
                }
                InputArtifactDispatch::RawApplyV3 => Err(AppError::evidence(
                    "passive raw/apply artifacts do not support input-compare",
                )),
            }
        }
        Command::InputGate {
            mode_gate,
            loop_gate,
            raw_prereq,
            active_plan,
            active_verification,
            matrix,
            repeatability,
            output,
        } => {
            let dispatch = classify_input_gate_matrix(&matrix)?;
            validate_input_gate_option_scope(
                dispatch,
                mode_gate.as_deref(),
                raw_prereq.as_deref(),
                active_plan.as_deref(),
                active_verification.as_deref(),
            )?;
            match dispatch {
                InputGateDispatch::Legacy => {
                    let decision = crate::input_artifact::write_input_gate(
                        &loop_gate,
                        &matrix,
                        &repeatability,
                        &output,
                    )?;
                    print_json(&decision)?;
                    if decision.status == "PASS" {
                        Ok(())
                    } else {
                        Err(AppError::evidence("T0-04 remains BLOCKED_BY_EVIDENCE"))
                    }
                }
                InputGateDispatch::ActiveV2 => {
                    let decision = crate::active_artifact::write_active_gate_with_verification(
                        mode_gate
                            .expect("active gate mode input checked above")
                            .as_path(),
                        &loop_gate,
                        raw_prereq
                            .expect("active gate raw prerequisite checked above")
                            .as_path(),
                        active_plan
                            .expect("active gate plan input checked above")
                            .as_path(),
                        &matrix,
                        &repeatability,
                        active_verification
                            .expect("active gate verification input checked above")
                            .as_path(),
                        &output,
                    )?;
                    print_json(&decision)?;
                    if decision.status == "PASS" {
                        Ok(())
                    } else {
                        Err(AppError::evidence(
                            "T0-04 schema-4 gate remains BLOCKED_BY_EVIDENCE",
                        ))
                    }
                }
            }
        }
    }
}

fn manual_loop_checkpoint(
    probe: &str,
    scenario: &str,
    game_exe: &Path,
    ledger: &Path,
    expected_variant: &str,
    artifacts_root: &Path,
    case_id: &str,
) -> AppResult<()> {
    if !matches!(probe, "control-no-native" | "passive") {
        return Err(AppError::usage(
            "loop-smoke --probe must be control-no-native or passive",
        ));
    }
    validate_case_and_root(artifacts_root, case_id)?;
    let _ = crate::loop_verification::inspect_loop(game_exe, ledger, expected_variant)?;
    let required = if probe == "passive" {
        vec![
            "Connect an XInput dual-stick controller.".to_owned(),
            "Copy and enable test-mod/isaac-cs-t0-input-witness manually.".to_owned(),
            "Start the exact fixed isaac-ng.exe build, then confirm in this task.".to_owned(),
        ]
    } else {
        vec!["Start the exact fixed isaac-ng.exe build, then confirm in this task.".to_owned()]
    };
    print_json(&ManualCheckpointOutput {
        schema_version: 1,
        result: "WAITING_FOR_USER".to_owned(),
        probe: probe.to_owned(),
        scenario: scenario.to_owned(),
        game_started: false,
        native_injected: false,
        mod_deployed: false,
        required_user_actions: required,
    })?;
    Err(AppError::evidence(
        "manual checkpoint reached; no game process was started or modified",
    ))
}

#[allow(clippy::too_many_arguments)]
fn manual_input_checkpoint(
    probe: &str,
    scenario: &str,
    game_exe: &Path,
    loop_ledger: &Path,
    mode_gate: Option<&Path>,
    loop_gate: Option<&Path>,
    raw_prereq: Option<&Path>,
    active_plan: Option<&Path>,
    offline_prereq: Option<&Path>,
    arm_grant: Option<&Path>,
    active_verification: Option<&Path>,
    input_script: Option<&Path>,
    expected_variant: &str,
    artifacts_root: &Path,
    case_id: &str,
    arm_token: Option<&str>,
) -> AppResult<()> {
    if probe == "loopback" {
        debug_assert!(arm_token.is_some());
        debug_assert!(mode_gate.is_some());
        debug_assert!(loop_gate.is_some());
        debug_assert!(raw_prereq.is_some());
        debug_assert!(active_plan.is_some());
        debug_assert!(offline_prereq.is_some());
        debug_assert!(arm_grant.is_some());
        debug_assert!(active_verification.is_some());
    }
    validate_case_and_root(artifacts_root, case_id)?;
    let loop_report =
        crate::loop_verification::inspect_loop(game_exe, loop_ledger, expected_variant)?;
    crate::input_core::run_codec_self_test()
        .map_err(|error| AppError::evidence(error.to_string()))?;
    if probe == "loopback" {
        let validated = crate::active_catalog::validate_active_plan(
            active_plan.expect("loopback active plan checked above"),
            mode_gate.expect("loopback mode gate checked above"),
            loop_gate.expect("loopback gate checked above"),
            raw_prereq.expect("loopback raw prerequisite checked above"),
        )?;
        if validated.target_sha256 != loop_report.file_sha256
            || validated.loop_ledger_sha256 != loop_report.ledger_sha256
        {
            return Err(AppError::evidence(
                "active input bundle does not bind the selected target and frozen loop ledger",
            ));
        }
        let input_script_required = scenario == "loopback";
        let input_script_validated = if input_script_required {
            crate::active_catalog::validate_active_input_script(
                input_script.expect("loopback repeatability input script checked above"),
                &validated.repeatability_input_script_sha256,
            )?;
            true
        } else {
            false
        };
        let dynamic_preflight = crate::active_preflight::run_read_only_preflight(
            game_exe,
            &validated.target_sha256,
            scenario,
        )?;
        let observable_preflight_passed = dynamic_preflight
            .checks
            .iter()
            .filter(|check| check.id != "offline-session-attestation")
            .all(|check| check.passed);
        if dynamic_preflight.result != "BLOCKED_BY_UNIMPLEMENTED_OFFLINE_ATTESTATION"
            || !observable_preflight_passed
        {
            return Err(AppError::evidence(
                "active read-only preflight did not reach the offline prerequisite boundary",
            ));
        }

        let offline_path = offline_prereq.expect("loopback offline prerequisite checked above");
        let prerequisite = crate::offline_attestation::validate_offline_prerequisite(
            offline_path,
            chrono::Utc::now(),
        )?;
        let generation_continuity_binding_validated = prerequisite.target_sha256
            == validated.target_sha256
            && prerequisite.mode_gate_sha256 == validated.mode_gate_sha256
            && prerequisite.loop_gate_sha256 == validated.loop_gate_sha256
            && prerequisite.raw_prereq_sha256 == validated.raw_prereq_sha256
            && prerequisite.active_plan_sha256 == validated.plan_sha256
            && prerequisite.case_id == case_id
            && prerequisite.scenario == scenario
            && prerequisite.process_id == dynamic_preflight.process_id
            && prerequisite.process_creation_time_100ns
                == dynamic_preflight.process_creation_time_100ns;
        if !generation_continuity_binding_validated {
            return Err(AppError::evidence(
                "offline prerequisite no longer binds the current target, process, case, scenario, or frozen bundle",
            ));
        }
        crate::offline_attestation_commands::recheck_offline_continuity_command(
            game_exe,
            &validated.target_sha256,
            scenario,
            offline_path,
        )?;

        let token = arm_token.expect("loopback arm token checked above");
        let _ = crate::arm_token::validate_arm_grant(
            arm_grant.expect("loopback arm grant checked above"),
            offline_path,
            token,
            case_id,
            scenario,
            chrono::Utc::now(),
        )?;
        let verification = crate::active_verification::read_active_verification(
            active_verification.expect("loopback active verification checked above"),
        )?;
        if !verification.semantic_contract_complete {
            return Err(AppError::evidence(
                "active semantic contract verification is incomplete",
            ));
        }
        print_json(&ActiveControllerPreflightOutput {
            schema_version: 1,
            result: crate::active_verification::ACTIVE_VERIFICATION_STATUS.to_owned(),
            probe: probe.to_owned(),
            scenario: scenario.to_owned(),
            static_bundle_validated: true,
            selected_target_bound: true,
            selected_loop_ledger_bound: true,
            input_script_required,
            input_script_validated,
            offline_prerequisite_validated: true,
            generation_continuity_binding_validated,
            arm_grant_validated: true,
            active_verification_validated: true,
            semantic_contract_complete: verification.semantic_contract_complete,
            runtime_contract_complete: verification.runtime_contract_complete,
            active_input_allowed: false,
            game_started: false,
            native_injected: false,
            arm_token_consumed: false,
            mapping_created: false,
            artifact_created: false,
            next_checkpoint: "SCHEMA4_NATIVE_RUNTIME_AND_45_SLOT_TRANSACTION".to_owned(),
            dynamic_preflight,
        })?;
        return Err(AppError::evidence(
            "active verification is blocked by the deferred schema-4 Native runtime; the arm token was validated but not consumed, and no mapping, Native load, or artifact was created",
        ));
    }
    manual_loop_checkpoint(
        if probe == "passive" {
            "passive"
        } else {
            "control-no-native"
        },
        scenario,
        game_exe,
        loop_ledger,
        expected_variant,
        artifacts_root,
        case_id,
    )
}

#[allow(clippy::too_many_arguments)]
fn validate_input_smoke_option_scope(
    probe: &str,
    scenario: &str,
    target_ledger: Option<&Path>,
    target_equivalence: Option<&Path>,
    mode_gate: Option<&Path>,
    loop_gate: Option<&Path>,
    raw_prereq: Option<&Path>,
    active_catalog: Option<&Path>,
    active_plan: Option<&Path>,
    offline_prereq: Option<&Path>,
    arm_grant: Option<&Path>,
    active_verification: Option<&Path>,
    arm_token: Option<&str>,
    input_script: Option<&Path>,
) -> AppResult<()> {
    if !matches!(
        probe,
        "control-no-native" | "passive" | "loopback" | "original-online-disarmed"
    ) {
        return Err(AppError::usage("unknown input-smoke probe"));
    }
    if probe == "loopback" {
        if !crate::active_catalog::ACTIVE_SCENARIOS.contains(&scenario)
            || matches!(scenario, "control-no-native" | "original-online-disarmed")
        {
            return Err(AppError::usage(
                "loopback probe requires one of the 16 active runtime scenarios",
            ));
        }
        for (name, present) in [
            ("--target-ledger", target_ledger.is_some()),
            ("--target-equivalence", target_equivalence.is_some()),
            ("--mode-gate", mode_gate.is_some()),
            ("--loop-gate", loop_gate.is_some()),
            ("--raw-prereq", raw_prereq.is_some()),
            ("--active-catalog", active_catalog.is_some()),
            ("--active-plan", active_plan.is_some()),
            ("--offline-prereq", offline_prereq.is_some()),
            ("--arm-grant", arm_grant.is_some()),
            ("--active-verification", active_verification.is_some()),
            ("--arm-token", arm_token.is_some()),
        ] {
            if !present {
                return Err(AppError::usage(format!("loopback requires {name}")));
            }
        }
        let token = arm_token.expect("loopback arm token checked above");
        if token.len() != 64
            || !token.bytes().all(|byte| byte.is_ascii_hexdigit())
            || token.bytes().all(|byte| byte == b'0')
        {
            return Err(AppError::usage(
                "--arm-token must be a nonzero 32-byte hexadecimal token",
            ));
        }
        if scenario == "loopback" && input_script.is_none() {
            return Err(AppError::usage(
                "loopback repeatability scenario requires --input-script",
            ));
        }
        if scenario != "loopback" && input_script.is_some() {
            return Err(AppError::usage(
                "--input-script is accepted only by the loopback repeatability scenario",
            ));
        }
    } else if matches!(probe, "control-no-native" | "original-online-disarmed") {
        for (name, present) in [
            ("--target-ledger", target_ledger.is_some()),
            ("--target-equivalence", target_equivalence.is_some()),
            ("--mode-gate", mode_gate.is_some()),
            ("--loop-gate", loop_gate.is_some()),
            ("--raw-prereq", raw_prereq.is_some()),
            ("--active-catalog", active_catalog.is_some()),
            ("--active-plan", active_plan.is_some()),
        ] {
            if !present {
                return Err(AppError::usage(format!("{probe} requires {name}")));
            }
        }
        if offline_prereq.is_some()
            || arm_grant.is_some()
            || active_verification.is_some()
            || arm_token.is_some()
            || input_script.is_some()
        {
            return Err(AppError::usage(
                "disarmed probes forbid offline prerequisite, arm, verification, token, and input-script options",
            ));
        }
    } else if target_ledger.is_some()
        || target_equivalence.is_some()
        || mode_gate.is_some()
        || raw_prereq.is_some()
        || active_catalog.is_some()
        || active_plan.is_some()
        || offline_prereq.is_some()
        || arm_grant.is_some()
        || active_verification.is_some()
        || arm_token.is_some()
        || input_script.is_some()
    {
        return Err(AppError::usage(
            "--target-ledger, --target-equivalence, --mode-gate, --raw-prereq, --active-catalog, --active-plan, --offline-prereq, --arm-grant, --active-verification, --arm-token, and --input-script are accepted only by the loopback probe",
        ));
    } else if probe != "passive" && loop_gate.is_some() {
        return Err(AppError::usage(
            "--loop-gate is accepted only by the passive and loopback probes",
        ));
    }
    if probe == "control-no-native" && scenario != "control-no-native"
        || probe == "original-online-disarmed" && scenario != "original-online-disarmed"
    {
        return Err(AppError::usage(
            "disarmed input-smoke probes require their matching scenario",
        ));
    }
    Ok(())
}

fn validate_case_and_root(artifacts_root: &Path, case_id: &str) -> AppResult<()> {
    if case_id.is_empty()
        || !case_id
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_'))
        || artifacts_root.as_os_str().is_empty()
    {
        return Err(AppError::usage("invalid artifact root or case id"));
    }
    Ok(())
}

fn inspect_command(game_exe: &Path, ledger_path: &Path, expected_variant: &str) -> AppResult<()> {
    let (_, _, report) = inspect_static(game_exe, ledger_path, expected_variant)?;
    print_json(&report)
}

fn analyze_command(run_directory: &Path) -> AppResult<()> {
    let report = analyze_run(run_directory)?;
    write_analysis(run_directory, &report)?;
    print_json(&report)?;
    if report.result == "PASS" {
        Ok(())
    } else {
        Err(AppError::evidence(format!(
            "artifact analysis failed at {}",
            report
                .first_divergence
                .as_deref()
                .unwrap_or("unknown-check")
        )))
    }
}

#[cfg(not(windows))]
fn smoke_command(
    _game_exe: &Path,
    _ledger_path: &Path,
    _expected_variant: &str,
    _artifacts_root: &Path,
    _case_id: &str,
) -> AppResult<()> {
    Err(AppError::usage("smoke is supported only on Windows"))
}

#[cfg(windows)]
fn smoke_command(
    game_exe: &Path,
    ledger_path: &Path,
    expected_variant: &str,
    artifacts_root: &Path,
    case_id: &str,
) -> AppResult<()> {
    if expected_variant != FIXED_SMOKE_VARIANT {
        return Err(AppError::usage(format!(
            "T0-01 smoke is fixed to expected variant {FIXED_SMOKE_VARIANT}"
        )));
    }
    let canonical_game = std::fs::canonicalize(game_exe).usage_context(format!(
        "cannot resolve game executable {}",
        game_exe.display()
    ))?;
    let (loaded, image, before_disk) =
        inspect_static(&canonical_game, ledger_path, expected_variant)?;
    let tool_path = std::env::current_exe().usage_context("cannot locate observer executable")?;
    let mut run = ArtifactRun::create(
        artifacts_root,
        case_id,
        &before_disk,
        file_sha256(&tool_path)?,
    )?;
    let thread_id = win32::current_thread_id();
    event(
        &mut run,
        thread_id,
        "observer_started",
        "PASS",
        None,
        "read-only observer ready",
        None,
    )?;

    let observation = (|| -> AppResult<()> {
        let initial_targets = win32::find_target_pids()?;
        if !initial_targets.is_empty() {
            return fail_run(
                &mut run,
                thread_id,
                "PREEXISTING_TARGET",
                "isaac-ng.exe was already running when the observer started",
            );
        }
        let blockers = win32::running_blockers()?;
        if !blockers.is_empty() {
            return fail_run(
                &mut run,
                thread_id,
                "FORBIDDEN_PROCESS",
                format!(
                    "blocked instrumentation process present: {}",
                    blockers.join(", ")
                ),
            );
        }

        event(
            &mut run,
            thread_id,
            "waiting_for_process",
            "PASS",
            None,
            "waiting for user-started isaac-ng.exe",
            None,
        )?;
        let wait_started = Instant::now();
        let process = loop {
            let blockers = win32::running_blockers()?;
            if !blockers.is_empty() {
                return fail_run(
                    &mut run,
                    thread_id,
                    "FORBIDDEN_PROCESS",
                    format!(
                        "blocked instrumentation process appeared: {}",
                        blockers.join(", ")
                    ),
                );
            }
            let pids = win32::find_target_pids()?;
            if pids.len() > 1 {
                return fail_run(
                    &mut run,
                    thread_id,
                    "MULTIPLE_TARGET_PROCESSES",
                    "more than one isaac-ng.exe process was observed",
                );
            }
            if let Some(pid) = pids.first() {
                break ObservedProcess::open_exact(*pid, &canonical_game)?;
            }
            if wait_started.elapsed() >= WAIT_FOR_PROCESS {
                event(
                    &mut run,
                    thread_id,
                    "waiting_for_process",
                    "FAIL",
                    Some("PROCESS_WAIT_TIMEOUT"),
                    "no user-started target appeared within 300 seconds",
                    None,
                )?;
                run.finish("BLOCKED_BY_EVIDENCE", "waiting for real process timed out")?;
                return Err(AppError::timeout("waiting for real process timed out"));
            }
            thread::sleep(Duration::from_millis(250));
        };

        if process.main_module.size != image.image_size {
            return fail_run(
                &mut run,
                thread_id,
                "MODULE_SIZE_MISMATCH",
                format!(
                    "loaded module size 0x{:08X} differs from PE size 0x{:08X}",
                    process.main_module.size, image.image_size
                ),
            );
        }
        event(
            &mut run,
            thread_id,
            "process_found",
            "PASS",
            None,
            "exact inspected target process opened with query/read/synchronize rights",
            Some(process.pid),
        )?;

        let before_live = wait_for_stable_snapshot(&process, &image, &loaded, &mut run, thread_id)?;
        run.set_process(
            &before_live,
            filetime_100ns_to_utc(process.creation_time_100ns),
        );
        let before_document = TargetBytesDocument {
            schema_version: ARTIFACT_SCHEMA_VERSION,
            phase: "before".to_owned(),
            captured_utc: utc_now(),
            disk: before_disk.clone(),
            process: Some(before_live.clone()),
        };
        run.write_target_document("target-bytes-before.json", &before_document)?;
        event(
            &mut run,
            thread_id,
            "baseline_captured",
            "PASS",
            None,
            format!(
                "{} targets captured after stability window",
                before_live.targets.len()
            ),
            Some(process.pid),
        )?;

        let observation_started = Instant::now();
        let mut after_live = before_live.clone();
        loop {
            if process.wait(SAMPLE_INTERVAL.as_millis() as u32)? {
                break;
            }
            let blockers = win32::running_blockers()?;
            if !blockers.is_empty() {
                return fail_run(
                    &mut run,
                    thread_id,
                    "FORBIDDEN_PROCESS",
                    format!(
                        "blocked instrumentation process appeared: {}",
                        blockers.join(", ")
                    ),
                );
            }
            after_live = match capture_live_snapshot(&process, &image, &loaded) {
                Ok(snapshot) => snapshot,
                Err(_) if process.wait(0)? => break,
                Err(error) => return Err(error),
            };
            if observation_started.elapsed() >= MAX_OBSERVATION {
                return fail_run(
                    &mut run,
                    thread_id,
                    "OBSERVATION_LIMIT",
                    "target did not exit within the 30 minute evidence window",
                );
            }
        }
        let exit_code = process.exit_code()?;
        if exit_code != 0 {
            return fail_run(
                &mut run,
                thread_id,
                "ABNORMAL_PROCESS_EXIT",
                format!("observed process exited with code 0x{exit_code:08X}"),
            );
        }
        event(
            &mut run,
            thread_id,
            "process_exited",
            "PASS",
            None,
            "observed process exited with code 0; no termination API was used",
            Some(process.pid),
        )?;

        let (_, _, after_disk) = inspect_static(&canonical_game, ledger_path, expected_variant)?;
        let after_document = TargetBytesDocument {
            schema_version: ARTIFACT_SCHEMA_VERSION,
            phase: "after".to_owned(),
            captured_utc: utc_now(),
            disk: after_disk,
            process: Some(after_live),
        };
        run.write_target_document("target-bytes-after.json", &after_document)?;
        event(
            &mut run,
            thread_id,
            "smoke_completed",
            "PASS",
            None,
            "read-only capture completed",
            Some(process.pid),
        )?;
        run.finish("PASS", "read-only runtime evidence captured")?;
        Ok(())
    })();
    if let Err(error) = observation {
        if run.manifest.status == "RUNNING" {
            let _ = event(
                &mut run,
                thread_id,
                "evidence_failure",
                "FAIL",
                Some("OBSERVER_ERROR"),
                error.message.clone(),
                None,
            );
            let _ = run.finish("FAIL", error.message.clone());
        }
        return Err(error);
    }
    let run_directory = run.directory.clone();

    let report = analyze_run(&run_directory)?;
    write_analysis(&run_directory, &report)?;
    print_json(&report)?;
    if report.result != "PASS" {
        run.finish(
            "FAIL",
            format!(
                "artifact analysis failed at {}",
                report
                    .first_divergence
                    .as_deref()
                    .unwrap_or("unknown-check")
            ),
        )?;
        return Err(AppError::evidence(format!(
            "smoke artifact analysis failed at {}",
            report
                .first_divergence
                .as_deref()
                .unwrap_or("unknown-check")
        )));
    }
    drop(run);
    Ok(())
}

#[cfg(windows)]
fn wait_for_stable_snapshot(
    process: &ObservedProcess,
    image: &PeImage,
    loaded: &LoadedLedger,
    run: &mut ArtifactRun,
    thread_id: u32,
) -> AppResult<LiveProcessSnapshot> {
    let deadline = Instant::now() + Duration::from_secs(30);
    let mut previous: Option<LiveProcessSnapshot> = None;
    let mut stable_since: Option<Instant> = None;
    loop {
        if process.wait(250)? {
            return fail_run(
                run,
                thread_id,
                "PROCESS_EXITED_BEFORE_BASELINE",
                "target exited before a stable baseline could be captured",
            );
        }
        match capture_live_snapshot(process, image, loaded) {
            Ok(snapshot) => {
                let same = previous
                    .as_ref()
                    .is_some_and(|prior| snapshots_equivalent(prior, &snapshot));
                if same {
                    let since = stable_since.get_or_insert_with(Instant::now);
                    if since.elapsed() >= READY_STABILITY {
                        return Ok(snapshot);
                    }
                } else {
                    stable_since = None;
                }
                previous = Some(snapshot);
            }
            Err(error) => {
                previous = None;
                stable_since = None;
                if Instant::now() >= deadline {
                    return fail_run(
                        run,
                        thread_id,
                        "TARGETS_NOT_READY",
                        format!("runtime targets did not validate within 30 seconds: {error}"),
                    );
                }
            }
        }
        if Instant::now() >= deadline {
            return fail_run(
                run,
                thread_id,
                "TARGETS_NOT_STABLE",
                "runtime targets did not remain identical for two seconds",
            );
        }
    }
}

#[cfg(windows)]
pub(crate) fn capture_live_snapshot(
    process: &ObservedProcess,
    image: &PeImage,
    loaded: &LoadedLedger,
) -> AppResult<LiveProcessSnapshot> {
    let base = process.main_module.base;
    let preferred = u64::from(image.image_base);
    let load_bias = i128::from(base) - i128::from(preferred);
    let debugger_present = process.debugger_present()?;
    if debugger_present {
        return Err(AppError::evidence(
            "a debugger is attached to the target process",
        ));
    }
    let forbidden_modules = process.forbidden_modules();
    if !forbidden_modules.is_empty() {
        return Err(AppError::evidence(format!(
            "forbidden runtime module present: {}",
            forbidden_modules.join(", ")
        )));
    }
    let targets = loaded
        .ledger
        .targets
        .iter()
        .map(|target| capture_target(process, image, target))
        .collect::<AppResult<Vec<_>>>()?;
    let modules = process
        .modules
        .iter()
        .map(|module| ModuleSnapshot {
            name: module.name.clone(),
            base: format!("0x{:08X}", module.base),
            size: module.size,
        })
        .collect();
    Ok(LiveProcessSnapshot {
        pid: process.pid,
        creation_time_100ns: process.creation_time_100ns,
        module_base: format!("0x{base:08X}"),
        module_size: process.main_module.size,
        preferred_image_base: format!("0x{preferred:08X}"),
        load_bias: if load_bias < 0 {
            format!("-0x{:X}", -load_bias)
        } else {
            format!("0x{load_bias:X}")
        },
        debugger_present,
        forbidden_modules,
        modules,
        targets,
        sampled_qpc: win32::qpc_now()?,
    })
}

#[cfg(windows)]
fn capture_target(
    process: &ObservedProcess,
    image: &PeImage,
    target: &TargetSpec,
) -> AppResult<LiveTargetSnapshot> {
    let base = process.main_module.base;
    let logical_rva = target.rva_value()?;
    let capture_rva = target.capture_rva()?;
    let address = checked_address(base, capture_rva)?;
    let bytes = process.read_memory(address, target.capture_size()?)?;
    let pattern = target.feature.pattern()?;
    let mut resolved_target = None;
    let mut resolved_owner_module = None;

    match target.kind.as_str() {
        "relativeCall" => {
            if bytes.len() != 5 || bytes[0] != 0xE8 {
                return Err(target_error(target, "runtime opcode is not E8 rel32"));
            }
            let displacement = i32::from_le_bytes(bytes[1..5].try_into().unwrap());
            let actual = i128::from(address) + 5 + i128::from(displacement);
            let expected = i128::from(checked_address(base, target.expected_target_value()?)?);
            if actual != expected {
                return Err(target_error(target, "runtime rel32 target mismatch"));
            }
            resolved_target = Some(format!("0x{actual:08X}"));
        }
        "iatCall" => {
            if bytes.len() != 6 || bytes[0..2] != [0xFF, 0x15] {
                return Err(target_error(target, "runtime opcode is not FF 15 IAT call"));
            }
            let import = target.import_name.as_deref().ok_or_else(|| {
                AppError::usage(format!("target {} has no import name", target.id))
            })?;
            let iat_rva = *image.imports.get(import).ok_or_else(|| {
                target_error(
                    target,
                    format!("import {import} is absent from static image"),
                )
            })?;
            let expected_iat = checked_address(base, iat_rva)?;
            let actual_iat = u64::from(u32::from_le_bytes(bytes[2..6].try_into().unwrap()));
            if actual_iat != expected_iat {
                return Err(target_error(target, "runtime IAT operand mismatch"));
            }
            let pointer = read_u32(process, expected_iat)?;
            let owner = process.owner_module(pointer).ok_or_else(|| {
                target_error(
                    target,
                    format!("IAT target 0x{pointer:08X} has no module owner"),
                )
            })?;
            if let Some(expected_owner) = &target.expected_owner_module
                && !owner.name.eq_ignore_ascii_case(expected_owner)
            {
                return Err(target_error(
                    target,
                    format!("IAT owner expected {expected_owner}, got {}", owner.name),
                ));
            }
            resolved_target = Some(format!("0x{pointer:08X}"));
            resolved_owner_module = Some(owner.name.clone());
        }
        "absoluteReference" => {
            if !masked_equal(&bytes, &pattern.bytes, &pattern.mask) {
                return Err(target_error(target, "runtime feature bytes mismatch"));
            }
            let offset = target.operand_offset.ok_or_else(|| {
                AppError::usage(format!("target {} has no operand offset", target.id))
            })?;
            let operand = bytes.get(offset..offset + 4).ok_or_else(|| {
                AppError::usage(format!("target {} operand is out of feature", target.id))
            })?;
            let actual = u64::from(u32::from_le_bytes(operand.try_into().unwrap()));
            let expected = checked_address(base, logical_rva)?;
            if actual != expected {
                return Err(target_error(target, "relocated absolute operand mismatch"));
            }
            resolved_target = Some(format!("0x{actual:08X}"));
        }
        "pointerValue" => {
            let actual = u64::from(u32::from_le_bytes(bytes[..4].try_into().unwrap()));
            let expected = checked_address(base, target.expected_target_value()?)?;
            if actual != expected {
                return Err(target_error(target, "relocated pointer value mismatch"));
            }
            resolved_target = Some(format!("0x{actual:08X}"));
            resolved_owner_module = process
                .owner_module(actual)
                .map(|module| module.name.clone());
        }
        "vtable" => {
            for (index, expected_rva) in target.vtable_target_rvas.iter().enumerate() {
                let expected_rva = parse_hex_u32(expected_rva)?;
                let actual = u64::from(u32::from_le_bytes(
                    bytes[index * 4..index * 4 + 4].try_into().unwrap(),
                ));
                if actual != checked_address(base, expected_rva)? {
                    return Err(target_error(
                        target,
                        format!("vtable entry {index} mismatch"),
                    ));
                }
            }
            resolved_target = Some(format!(
                "{} relocated entries",
                target.vtable_target_rvas.len()
            ));
            resolved_owner_module = Some(process.main_module.name.clone());
        }
        "signature" => {
            if !masked_equal(&bytes, &pattern.bytes, &pattern.mask) {
                return Err(target_error(target, "runtime signature mismatch"));
            }
        }
        unknown => {
            return Err(AppError::usage(format!(
                "target {} has unknown kind {unknown}",
                target.id
            )));
        }
    }

    Ok(LiveTargetSnapshot {
        id: target.id.clone(),
        rva: format!("0x{capture_rva:08X}"),
        address: format!("0x{address:08X}"),
        bytes: hex::encode_upper(bytes),
        semantic_result: "PASS".to_owned(),
        resolved_target,
        resolved_owner_module,
    })
}

#[cfg(windows)]
fn read_u32(process: &ObservedProcess, address: u64) -> AppResult<u64> {
    let bytes = process.read_memory(address, 4)?;
    Ok(u64::from(u32::from_le_bytes(bytes.try_into().unwrap())))
}

pub(crate) fn inspect_static(
    game_exe: &Path,
    ledger_path: &Path,
    expected_variant: &str,
) -> AppResult<(LoadedLedger, PeImage, StaticInspectionReport)> {
    let loaded = LoadedLedger::read(ledger_path)?;
    let image = PeImage::read(game_exe)
        .map_err(|error| AppError::evidence(format!("cannot inspect game PE: {error}")))?;
    let report = loaded
        .ledger
        .inspect(&image, expected_variant, &loaded.raw_sha256)?;
    Ok((loaded, image, report))
}

#[cfg(windows)]
fn event(
    run: &mut ArtifactRun,
    thread_id: u32,
    kind: &str,
    result: &str,
    error_code: Option<&str>,
    detail: impl Into<String>,
    pid: Option<u32>,
) -> AppResult<()> {
    run.event(
        win32::qpc_now()?,
        thread_id,
        kind,
        result,
        error_code,
        detail,
        pid,
    )
}

#[cfg(windows)]
fn fail_run<T>(
    run: &mut ArtifactRun,
    thread_id: u32,
    code: &str,
    reason: impl Into<String>,
) -> AppResult<T> {
    let reason = reason.into();
    event(
        run,
        thread_id,
        "evidence_failure",
        "FAIL",
        Some(code),
        reason.clone(),
        None,
    )?;
    run.finish("FAIL", reason.clone())?;
    Err(AppError::evidence(reason))
}

fn checked_address(base: u64, rva: u32) -> AppResult<u64> {
    base.checked_add(u64::from(rva))
        .ok_or_else(|| AppError::evidence("relocated address overflow"))
}

fn masked_equal(actual: &[u8], expected: &[u8], mask: &[u8]) -> bool {
    actual.len() == expected.len()
        && actual
            .iter()
            .zip(expected)
            .zip(mask)
            .all(|((actual, expected), mask)| (actual & mask) == (expected & mask))
}

fn snapshots_equivalent(left: &LiveProcessSnapshot, right: &LiveProcessSnapshot) -> bool {
    left.pid == right.pid
        && left.creation_time_100ns == right.creation_time_100ns
        && left.module_base == right.module_base
        && left.debugger_present == right.debugger_present
        && left.forbidden_modules == right.forbidden_modules
        && left.modules == right.modules
        && left.targets.len() == right.targets.len()
        && left
            .targets
            .iter()
            .zip(&right.targets)
            .all(|(left, right)| {
                left.id == right.id
                    && left.rva == right.rva
                    && left.address == right.address
                    && left.bytes == right.bytes
                    && left.semantic_result == right.semantic_result
                    && left.resolved_target == right.resolved_target
                    && left.resolved_owner_module == right.resolved_owner_module
            })
}

fn target_error(target: &TargetSpec, detail: impl Into<String>) -> AppError {
    AppError::evidence(format!("target {}: {}", target.id, detail.into()))
}

fn parse_hex_u32(value: &str) -> AppResult<u32> {
    let trimmed = value.trim();
    let digits = trimmed
        .strip_prefix("0x")
        .or_else(|| trimmed.strip_prefix("0X"))
        .ok_or_else(|| AppError::usage(format!("expected hexadecimal value, got {value}")))?;
    u32::from_str_radix(digits, 16)
        .map_err(|error| AppError::usage(format!("invalid hexadecimal value {value}: {error}")))
}

fn print_json(value: &impl serde::Serialize) -> AppResult<()> {
    let json =
        serde_json::to_string_pretty(value).usage_context("cannot serialize command result")?;
    println!("{json}");
    Ok(())
}

#[cfg(windows)]
fn require_rpm_diagnostic_pass(result: &str, operation: &str) -> AppResult<()> {
    if result == "PASS_DIAGNOSTIC" {
        Ok(())
    } else {
        Err(AppError::evidence(format!(
            "T0-04 RPM diagnostic {operation} did not obtain a coherent sample"
        )))
    }
}

fn parse_args(args: Vec<OsString>) -> AppResult<Command> {
    let Some(command) = args.first().and_then(|value| value.to_str()) else {
        return Err(usage());
    };
    match command {
        "inspect" => {
            let options = parse_options(&args[1..])?;
            require_only(&options, &["game-exe", "ledger", "expected-variant"])?;
            Ok(Command::Inspect {
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                ledger: PathBuf::from(required_os(&options, "ledger")?),
                expected_variant: required_string(&options, "expected-variant")?,
            })
        }
        "smoke" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &[
                    "game-exe",
                    "ledger",
                    "expected-variant",
                    "artifacts-root",
                    "case-id",
                ],
            )?;
            Ok(Command::Smoke {
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                ledger: PathBuf::from(required_os(&options, "ledger")?),
                expected_variant: required_string(&options, "expected-variant")?,
                artifacts_root: PathBuf::from(required_os(&options, "artifacts-root")?),
                case_id: required_string(&options, "case-id")?,
            })
        }
        "analyze" if args.len() == 2 => Ok(Command::Analyze {
            run_directory: PathBuf::from(&args[1]),
        }),
        "analyze" => Err(AppError::usage(
            "analyze requires exactly one run directory",
        )),
        "mode-inspect" => {
            let options = parse_options(&args[1..])?;
            require_only(&options, &["game-exe", "ledger", "expected-variant"])?;
            Ok(Command::ModeInspect {
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                ledger: PathBuf::from(required_os(&options, "ledger")?),
                expected_variant: required_string(&options, "expected-variant")?,
            })
        }
        "mode-smoke" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &[
                    "scenario",
                    "endpoint",
                    "game-exe",
                    "ledger",
                    "expected-variant",
                    "artifacts-root",
                    "case-id",
                ],
            )?;
            Ok(Command::ModeSmoke {
                scenario: required_string(&options, "scenario")?,
                endpoint: required_string(&options, "endpoint")?,
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                ledger: PathBuf::from(required_os(&options, "ledger")?),
                expected_variant: required_string(&options, "expected-variant")?,
                artifacts_root: PathBuf::from(required_os(&options, "artifacts-root")?),
                case_id: required_string(&options, "case-id")?,
            })
        }
        "mode-analyze" if args.len() == 2 => Ok(Command::ModeAnalyze {
            run_directory: PathBuf::from(&args[1]),
        }),
        "mode-analyze" => Err(AppError::usage(
            "mode-analyze requires exactly one run directory",
        )),
        "mode-compare" => {
            let options = parse_options(&args[1..])?;
            require_only(&options, &["control", "instrumented", "peer", "output"])?;
            Ok(Command::ModeCompare {
                control: PathBuf::from(required_os(&options, "control")?),
                instrumented: PathBuf::from(required_os(&options, "instrumented")?),
                peer: options.get("peer").map(PathBuf::from),
                output: options.get("output").map(PathBuf::from),
            })
        }
        "mode-gate" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &[
                    "baseline",
                    "offline-control",
                    "offline-instrumented",
                    "control-a",
                    "control-b",
                    "instrumented-a",
                    "instrumented-b",
                    "comparison",
                    "output",
                ],
            )?;
            Ok(Command::ModeGate {
                baseline: PathBuf::from(required_os(&options, "baseline")?),
                offline_control: PathBuf::from(required_os(&options, "offline-control")?),
                offline_instrumented: PathBuf::from(required_os(&options, "offline-instrumented")?),
                control_a: PathBuf::from(required_os(&options, "control-a")?),
                control_b: PathBuf::from(required_os(&options, "control-b")?),
                instrumented_a: PathBuf::from(required_os(&options, "instrumented-a")?),
                instrumented_b: PathBuf::from(required_os(&options, "instrumented-b")?),
                comparison: PathBuf::from(required_os(&options, "comparison")?),
                output: PathBuf::from(required_os(&options, "output")?),
            })
        }
        "coop-join-preflight" => {
            let options = parse_options(&args[1..])?;
            require_only(&options, &["game-exe", "repository-root", "output"])?;
            Ok(Command::CoopJoinPreflight {
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                repository_root: PathBuf::from(required_os(&options, "repository-root")?),
                output: options.get("output").map(PathBuf::from),
            })
        }
        "boundary-inspect" => {
            let options = parse_options(&args[1..])?;
            require_only(&options, &["game-exe", "ledger", "expected-variant"])?;
            Ok(Command::BoundaryInspect {
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                ledger: PathBuf::from(required_os(&options, "ledger")?),
                expected_variant: required_string(&options, "expected-variant")?,
            })
        }
        "boundary-smoke" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &[
                    "probe",
                    "scenario",
                    "game-exe",
                    "ledger",
                    "expected-variant",
                    "artifacts-root",
                    "case-id",
                ],
            )?;
            Ok(Command::BoundarySmoke {
                probe: required_string(&options, "probe")?,
                scenario: required_string(&options, "scenario")?,
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                ledger: PathBuf::from(required_os(&options, "ledger")?),
                expected_variant: required_string(&options, "expected-variant")?,
                artifacts_root: PathBuf::from(required_os(&options, "artifacts-root")?),
                case_id: required_string(&options, "case-id")?,
            })
        }
        "boundary-analyze" if args.len() == 2 => Ok(Command::BoundaryAnalyze {
            run_directory: PathBuf::from(&args[1]),
        }),
        "boundary-analyze" => Err(AppError::usage(
            "boundary-analyze requires exactly one run directory",
        )),
        "boundary-compare" => {
            let options = parse_options(&args[1..])?;
            require_only(&options, &["control", "instrumented", "output"])?;
            Ok(Command::BoundaryCompare {
                control: PathBuf::from(required_os(&options, "control")?),
                instrumented: PathBuf::from(required_os(&options, "instrumented")?),
                output: options.get("output").map(PathBuf::from),
            })
        }
        "loop-inspect" => {
            let options = parse_options(&args[1..])?;
            require_only(&options, &["game-exe", "ledger", "expected-variant"])?;
            Ok(Command::LoopInspect {
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                ledger: PathBuf::from(required_os(&options, "ledger")?),
                expected_variant: required_string(&options, "expected-variant")?,
            })
        }
        "loop-smoke" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &[
                    "probe",
                    "scenario",
                    "game-exe",
                    "ledger",
                    "expected-variant",
                    "artifacts-root",
                    "case-id",
                ],
            )?;
            Ok(Command::LoopSmoke {
                probe: required_string(&options, "probe")?,
                scenario: required_string(&options, "scenario")?,
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                ledger: PathBuf::from(required_os(&options, "ledger")?),
                expected_variant: required_string(&options, "expected-variant")?,
                artifacts_root: PathBuf::from(required_os(&options, "artifacts-root")?),
                case_id: required_string(&options, "case-id")?,
            })
        }
        "loop-analyze" if args.len() == 2 => Ok(Command::LoopAnalyze {
            run_directory: PathBuf::from(&args[1]),
        }),
        "loop-analyze" => Err(AppError::usage(
            "loop-analyze requires exactly one run directory",
        )),
        "loop-gate" => {
            let options = parse_options(&args[1..])?;
            require_only(&options, &["matrix", "output"])?;
            Ok(Command::LoopGate {
                matrix: PathBuf::from(required_os(&options, "matrix")?),
                output: PathBuf::from(required_os(&options, "output")?),
            })
        }
        "coop-inspect" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &[
                    "game-exe",
                    "target-equivalence",
                    "bootstrap-decision",
                    "ledger",
                    "catalog",
                    "graph",
                    "output",
                ],
            )?;
            Ok(Command::CoopInspect {
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                target_equivalence: PathBuf::from(required_os(&options, "target-equivalence")?),
                bootstrap_decision: PathBuf::from(required_os(&options, "bootstrap-decision")?),
                ledger: PathBuf::from(required_os(&options, "ledger")?),
                catalog: PathBuf::from(required_os(&options, "catalog")?),
                graph: PathBuf::from(required_os(&options, "graph")?),
                output: options.get("output").map(PathBuf::from),
            })
        }
        "coop-smoke" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &[
                    "probe",
                    "scenario",
                    "game-exe",
                    "target-equivalence",
                    "layout-evidence",
                    "catalog",
                    "graph",
                    "plan",
                    "verification",
                    "artifacts-root",
                    "case-id",
                    "duration-ms",
                    "interval-ms",
                ],
            )?;
            let probe = required_string(&options, "probe")?;
            let scenario = required_string(&options, "scenario")?;
            let case_id = required_string(&options, "case-id")?;
            let passive = probe == crate::coop_passive::PASSIVE_PROBE;
            let verification = options.get("verification").map(PathBuf::from);
            let duration_milliseconds =
                optional_decimal_u64(&options, "duration-ms")?.unwrap_or(if passive {
                    crate::coop_passive::PASSIVE_DEFAULT_DURATION_MILLISECONDS
                } else {
                    crate::coop_c0::DEFAULT_DURATION_MILLISECONDS
                });
            let interval_milliseconds = optional_decimal_u64(&options, "interval-ms")?
                .unwrap_or(crate::coop_c0::DEFAULT_INTERVAL_MILLISECONDS);
            if passive {
                if !crate::coop_passive::PASSIVE_SCENARIOS.contains(&scenario.as_str()) {
                    return Err(AppError::usage(
                        "unsupported passive scenario; C0 scenarios require --probe control-no-native",
                    ));
                }
                if verification.is_none() {
                    return Err(AppError::usage(
                        "coop passive capture requires --verification <verified-v5-json>",
                    ));
                }
                if options.contains_key("interval-ms") {
                    return Err(AppError::usage(
                        "coop passive capture rejects --interval-ms",
                    ));
                }
                crate::coop_passive::validate_passive_capture_options(
                    &probe,
                    &scenario,
                    &case_id,
                    duration_milliseconds,
                )?;
            } else {
                if verification.is_some() {
                    return Err(AppError::usage(
                        "--verification is accepted only by --probe passive",
                    ));
                }
                crate::coop_c0::validate_capture_options(
                    &probe,
                    &scenario,
                    &case_id,
                    duration_milliseconds,
                    interval_milliseconds,
                )?;
            }
            Ok(Command::CoopSmoke {
                probe,
                scenario,
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                target_equivalence: PathBuf::from(required_os(&options, "target-equivalence")?),
                layout_evidence: PathBuf::from(required_os(&options, "layout-evidence")?),
                catalog: PathBuf::from(required_os(&options, "catalog")?),
                graph: PathBuf::from(required_os(&options, "graph")?),
                plan: PathBuf::from(required_os(&options, "plan")?),
                verification,
                artifacts_root: PathBuf::from(required_os(&options, "artifacts-root")?),
                case_id,
                duration_milliseconds,
                interval_milliseconds,
            })
        }
        "coop-analyze" if args.len() >= 2 => {
            let run_directory = PathBuf::from(&args[1]);
            let options = parse_options(&args[2..])?;
            require_only(&options, &["output"])?;
            Ok(Command::CoopAnalyze {
                run_directory,
                output: PathBuf::from(required_os(&options, "output")?),
            })
        }
        "coop-analyze" => Err(AppError::usage(
            "coop-analyze requires one source directory and --output <new-json>",
        )),
        "coop-compare" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &[
                    "offline-single",
                    "local-coop-dual",
                    "original-online-dual",
                    "output",
                ],
            )?;
            Ok(Command::CoopCompare {
                offline_single: PathBuf::from(required_os(&options, "offline-single")?),
                local_coop_dual: PathBuf::from(required_os(&options, "local-coop-dual")?),
                original_online_dual: PathBuf::from(required_os(&options, "original-online-dual")?),
                output: PathBuf::from(required_os(&options, "output")?),
            })
        }
        "coop-c0-gate" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &[
                    "offline-single",
                    "local-coop-dual",
                    "original-online-dual",
                    "comparison",
                    "loop-gate",
                    "active-decision",
                    "output",
                ],
            )?;
            Ok(Command::CoopC0Gate {
                offline_single: PathBuf::from(required_os(&options, "offline-single")?),
                local_coop_dual: PathBuf::from(required_os(&options, "local-coop-dual")?),
                original_online_dual: PathBuf::from(required_os(&options, "original-online-dual")?),
                comparison: PathBuf::from(required_os(&options, "comparison")?),
                loop_gate: PathBuf::from(required_os(&options, "loop-gate")?),
                active_decision: PathBuf::from(required_os(&options, "active-decision")?),
                output: PathBuf::from(required_os(&options, "output")?),
            })
        }
        "coop-passive-gate" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &[
                    "native-join-passive",
                    "native-exit-passive",
                    "death-respawn-passive",
                    "room-rebind-passive",
                    "device-rebind-passive",
                    "menu-teardown-passive",
                    "output",
                ],
            )?;
            Ok(Command::CoopPassiveGate {
                native_join: PathBuf::from(required_os(&options, "native-join-passive")?),
                native_exit: PathBuf::from(required_os(&options, "native-exit-passive")?),
                death_respawn: PathBuf::from(required_os(&options, "death-respawn-passive")?),
                room_rebind: PathBuf::from(required_os(&options, "room-rebind-passive")?),
                device_rebind: PathBuf::from(required_os(&options, "device-rebind-passive")?),
                menu_teardown: PathBuf::from(required_os(&options, "menu-teardown-passive")?),
                output: PathBuf::from(required_os(&options, "output")?),
            })
        }
        "coop-passive-applicability-gate" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &[
                    "repository-root",
                    "applicability",
                    "native-exit-source",
                    "native-join-passive",
                    "death-respawn-passive",
                    "room-rebind-passive",
                    "device-rebind-passive",
                    "menu-teardown-passive",
                    "verification",
                    "output",
                ],
            )?;
            Ok(Command::CoopPassiveApplicabilityGate {
                repository_root: PathBuf::from(required_os(&options, "repository-root")?),
                applicability: PathBuf::from(required_os(&options, "applicability")?),
                native_exit_source: PathBuf::from(required_os(&options, "native-exit-source")?),
                sources: [
                    PathBuf::from(required_os(&options, "native-join-passive")?),
                    PathBuf::from(required_os(&options, "death-respawn-passive")?),
                    PathBuf::from(required_os(&options, "room-rebind-passive")?),
                    PathBuf::from(required_os(&options, "device-rebind-passive")?),
                    PathBuf::from(required_os(&options, "menu-teardown-passive")?),
                ],
                verification: options.get("verification").map(PathBuf::from),
                output: PathBuf::from(required_os(&options, "output")?),
            })
        }
        "input-inspect" => {
            let options = parse_options(&args[1..])?;
            require_only(&options, &["game-exe", "loop-ledger", "expected-variant"])?;
            Ok(Command::InputInspect {
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                loop_ledger: PathBuf::from(required_os(&options, "loop-ledger")?),
                expected_variant: required_string(&options, "expected-variant")?,
            })
        }
        "input-target-equivalence" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &[
                    "game-exe",
                    "loop-ledger",
                    "target-ledger",
                    "active-catalog",
                    "active-plan",
                    "output",
                ],
            )?;
            Ok(Command::InputTargetEquivalence {
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                loop_ledger: PathBuf::from(required_os(&options, "loop-ledger")?),
                target_ledger: PathBuf::from(required_os(&options, "target-ledger")?),
                active_catalog: PathBuf::from(required_os(&options, "active-catalog")?),
                active_plan: PathBuf::from(required_os(&options, "active-plan")?),
                output: PathBuf::from(required_os(&options, "output")?),
            })
        }
        "input-smoke" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &[
                    "probe",
                    "scenario",
                    "game-exe",
                    "loop-ledger",
                    "target-ledger",
                    "target-equivalence",
                    "mode-gate",
                    "loop-gate",
                    "raw-prereq",
                    "active-catalog",
                    "active-plan",
                    "offline-prereq",
                    "arm-grant",
                    "active-verification",
                    "input-script",
                    "expected-variant",
                    "artifacts-root",
                    "case-id",
                    "arm-token",
                ],
            )?;
            let command = Command::InputSmoke {
                probe: required_string(&options, "probe")?,
                scenario: required_string(&options, "scenario")?,
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                loop_ledger: PathBuf::from(required_os(&options, "loop-ledger")?),
                target_ledger: options.get("target-ledger").map(PathBuf::from),
                target_equivalence: options.get("target-equivalence").map(PathBuf::from),
                mode_gate: options.get("mode-gate").map(PathBuf::from),
                loop_gate: options.get("loop-gate").map(PathBuf::from),
                raw_prereq: options.get("raw-prereq").map(PathBuf::from),
                active_catalog: options.get("active-catalog").map(PathBuf::from),
                active_plan: options.get("active-plan").map(PathBuf::from),
                offline_prereq: options.get("offline-prereq").map(PathBuf::from),
                arm_grant: options.get("arm-grant").map(PathBuf::from),
                active_verification: options.get("active-verification").map(PathBuf::from),
                input_script: options.get("input-script").map(PathBuf::from),
                expected_variant: required_string(&options, "expected-variant")?,
                artifacts_root: PathBuf::from(required_os(&options, "artifacts-root")?),
                case_id: required_string(&options, "case-id")?,
                arm_token: options
                    .get("arm-token")
                    .map(|value| value.to_string_lossy().into_owned()),
            };
            let Command::InputSmoke {
                ref probe,
                ref scenario,
                ref target_ledger,
                ref target_equivalence,
                ref mode_gate,
                ref loop_gate,
                ref raw_prereq,
                ref active_catalog,
                ref active_plan,
                ref offline_prereq,
                ref arm_grant,
                ref active_verification,
                ref arm_token,
                ref input_script,
                ..
            } = command
            else {
                unreachable!();
            };
            validate_input_smoke_option_scope(
                probe,
                scenario,
                target_ledger.as_deref(),
                target_equivalence.as_deref(),
                mode_gate.as_deref(),
                loop_gate.as_deref(),
                raw_prereq.as_deref(),
                active_catalog.as_deref(),
                active_plan.as_deref(),
                offline_prereq.as_deref(),
                arm_grant.as_deref(),
                active_verification.as_deref(),
                arm_token.as_deref(),
                input_script.as_deref(),
            )?;
            Ok(command)
        }
        "input-offline-attest" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &[
                    "scenario",
                    "game-exe",
                    "loop-ledger",
                    "target-ledger",
                    "target-equivalence",
                    "mode-gate",
                    "loop-gate",
                    "raw-prereq",
                    "active-catalog",
                    "active-plan",
                    "expected-variant",
                    "artifacts-root",
                    "case-id",
                ],
            )?;
            Ok(Command::InputOfflineAttest {
                scenario: required_string(&options, "scenario")?,
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                loop_ledger: PathBuf::from(required_os(&options, "loop-ledger")?),
                target_ledger: PathBuf::from(required_os(&options, "target-ledger")?),
                target_equivalence: PathBuf::from(required_os(&options, "target-equivalence")?),
                mode_gate: PathBuf::from(required_os(&options, "mode-gate")?),
                loop_gate: PathBuf::from(required_os(&options, "loop-gate")?),
                raw_prereq: PathBuf::from(required_os(&options, "raw-prereq")?),
                active_catalog: PathBuf::from(required_os(&options, "active-catalog")?),
                active_plan: PathBuf::from(required_os(&options, "active-plan")?),
                expected_variant: required_string(&options, "expected-variant")?,
                artifacts_root: PathBuf::from(required_os(&options, "artifacts-root")?),
                case_id: required_string(&options, "case-id")?,
            })
        }
        "input-arm-grant" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &["offline-prereq", "case", "scenario", "arm-token", "output"],
            )?;
            Ok(Command::InputArmGrant {
                offline_prereq: PathBuf::from(required_os(&options, "offline-prereq")?),
                case_id: required_string(&options, "case")?,
                scenario: required_string(&options, "scenario")?,
                arm_token: required_string(&options, "arm-token")?,
                output: PathBuf::from(required_os(&options, "output")?),
            })
        }
        "input-raw-smoke" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &[
                    "probe",
                    "scenario",
                    "game-exe",
                    "loop-ledger",
                    "loop-gate",
                    "raw-apply-catalog",
                    "capture-plan",
                    "expected-variant",
                    "artifacts-root",
                    "case-id",
                ],
            )?;
            Ok(Command::InputRawSmoke {
                probe: required_string(&options, "probe")?,
                scenario: required_string(&options, "scenario")?,
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                loop_ledger: PathBuf::from(required_os(&options, "loop-ledger")?),
                loop_gate: PathBuf::from(required_os(&options, "loop-gate")?),
                raw_apply_catalog: PathBuf::from(required_os(&options, "raw-apply-catalog")?),
                capture_plan: PathBuf::from(required_os(&options, "capture-plan")?),
                expected_variant: required_string(&options, "expected-variant")?,
                artifacts_root: PathBuf::from(required_os(&options, "artifacts-root")?),
                case_id: required_string(&options, "case-id")?,
            })
        }
        "input-raw-prereq" => {
            let options = parse_options(&args[1..])?;
            require_only(&options, &["matrix", "output"])?;
            Ok(Command::InputRawPrereq {
                matrix: PathBuf::from(required_os(&options, "matrix")?),
                output: PathBuf::from(required_os(&options, "output")?),
            })
        }
        "input-analyze" if args.len() == 2 => Ok(Command::InputAnalyze {
            run_directory: PathBuf::from(&args[1]),
        }),
        "input-analyze" => Err(AppError::usage(
            "input-analyze requires exactly one run directory",
        )),
        "input-import-witness" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &[
                    "witness-log",
                    "output",
                    "scenario",
                    "case-id",
                    "target-sha256",
                    "loop-gate-sha256",
                    "window-min",
                    "window-max",
                    "controller-index",
                ],
            )?;
            Ok(Command::InputImportWitness {
                witness_log: PathBuf::from(required_os(&options, "witness-log")?),
                output: PathBuf::from(required_os(&options, "output")?),
                scenario: required_string(&options, "scenario")?,
                case_id: required_string(&options, "case-id")?,
                target_sha256: required_string(&options, "target-sha256")?,
                loop_gate_sha256: required_string(&options, "loop-gate-sha256")?,
                window_min: optional_decimal_u64(&options, "window-min")?,
                window_max: optional_decimal_u64(&options, "window-max")?,
                controller_index: optional_signed_decimal(&options, "controller-index")?,
            })
        }
        "input-rpm-inspect" => {
            let options = parse_options(&args[1..])?;
            require_only(&options, &["game-exe", "pid"])?;
            Ok(Command::InputRpmInspect {
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                pid: optional_decimal(&options, "pid")?,
            })
        }
        "input-rpm-capture" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &["game-exe", "pid", "output", "duration-ms", "interval-ms"],
            )?;
            Ok(Command::InputRpmCapture {
                game_exe: PathBuf::from(required_os(&options, "game-exe")?),
                pid: optional_decimal(&options, "pid")?,
                output: PathBuf::from(required_os(&options, "output")?),
                duration_milliseconds: required_decimal(&options, "duration-ms")?,
                interval_milliseconds: required_decimal(&options, "interval-ms")?,
            })
        }
        "input-rpm-analyze" if args.len() == 2 => Ok(Command::InputRpmAnalyze {
            run_directory: PathBuf::from(&args[1]),
        }),
        "input-rpm-analyze" => Err(AppError::usage(
            "input-rpm-analyze requires exactly one run directory",
        )),
        "input-compare" => {
            let options = parse_options(&args[1..])?;
            require_only(&options, &["first", "second", "output"])?;
            Ok(Command::InputCompare {
                first: PathBuf::from(required_os(&options, "first")?),
                second: PathBuf::from(required_os(&options, "second")?),
                output: options.get("output").map(PathBuf::from),
            })
        }
        "input-gate" => {
            let options = parse_options(&args[1..])?;
            require_only(
                &options,
                &[
                    "mode-gate",
                    "loop-gate",
                    "raw-prereq",
                    "active-plan",
                    "active-verification",
                    "matrix",
                    "repeatability",
                    "output",
                ],
            )?;
            Ok(Command::InputGate {
                mode_gate: options.get("mode-gate").map(PathBuf::from),
                loop_gate: PathBuf::from(required_os(&options, "loop-gate")?),
                raw_prereq: options.get("raw-prereq").map(PathBuf::from),
                active_plan: options.get("active-plan").map(PathBuf::from),
                active_verification: options.get("active-verification").map(PathBuf::from),
                matrix: PathBuf::from(required_os(&options, "matrix")?),
                repeatability: PathBuf::from(required_os(&options, "repeatability")?),
                output: PathBuf::from(required_os(&options, "output")?),
            })
        }
        _ => Err(usage()),
    }
}

fn parse_options(args: &[OsString]) -> AppResult<BTreeMap<String, OsString>> {
    if !args.len().is_multiple_of(2) {
        return Err(AppError::usage("every option must have one value"));
    }
    let mut options = BTreeMap::new();
    for pair in args.chunks_exact(2) {
        let option = pair[0]
            .to_str()
            .and_then(|value| value.strip_prefix("--"))
            .filter(|value| !value.is_empty())
            .ok_or_else(|| AppError::usage("expected a --named-option"))?;
        if options.insert(option.to_owned(), pair[1].clone()).is_some() {
            return Err(AppError::usage(format!("duplicate option --{option}")));
        }
    }
    Ok(options)
}

fn require_only(options: &BTreeMap<String, OsString>, allowed: &[&str]) -> AppResult<()> {
    let allowed = allowed.iter().copied().collect::<BTreeSet<_>>();
    if let Some(unknown) = options.keys().find(|key| !allowed.contains(key.as_str())) {
        return Err(AppError::usage(format!("unknown option --{unknown}")));
    }
    Ok(())
}

fn required_os(options: &BTreeMap<String, OsString>, name: &str) -> AppResult<OsString> {
    options
        .get(name)
        .cloned()
        .ok_or_else(|| AppError::usage(format!("missing --{name}")))
}

fn required_string(options: &BTreeMap<String, OsString>, name: &str) -> AppResult<String> {
    required_os(options, name)?
        .into_string()
        .map_err(|_| AppError::usage(format!("--{name} must be valid Unicode")))
}

fn required_decimal(options: &BTreeMap<String, OsString>, name: &str) -> AppResult<u64> {
    required_string(options, name)?
        .parse::<u64>()
        .map_err(|error| {
            AppError::usage(format!(
                "--{name} must be an unsigned decimal integer: {error}"
            ))
        })
}

fn optional_decimal(options: &BTreeMap<String, OsString>, name: &str) -> AppResult<Option<u32>> {
    options
        .get(name)
        .map(|value| {
            value
                .to_str()
                .ok_or_else(|| AppError::usage(format!("--{name} must be valid Unicode")))?
                .parse::<u32>()
                .map_err(|error| {
                    AppError::usage(format!(
                        "--{name} must be an unsigned 32-bit decimal integer: {error}"
                    ))
                })
        })
        .transpose()
}

fn optional_decimal_u64(
    options: &BTreeMap<String, OsString>,
    name: &str,
) -> AppResult<Option<u64>> {
    options
        .get(name)
        .map(|value| {
            value
                .to_str()
                .ok_or_else(|| AppError::usage(format!("--{name} must be valid Unicode")))?
                .parse::<u64>()
                .map_err(|error| {
                    AppError::usage(format!(
                        "--{name} must be an unsigned decimal integer: {error}"
                    ))
                })
        })
        .transpose()
}

fn optional_signed_decimal(
    options: &BTreeMap<String, OsString>,
    name: &str,
) -> AppResult<Option<i32>> {
    options
        .get(name)
        .map(|value| {
            value
                .to_str()
                .ok_or_else(|| AppError::usage(format!("--{name} must be valid Unicode")))?
                .parse::<i32>()
                .map_err(|error| {
                    AppError::usage(format!(
                        "--{name} must be a signed 32-bit decimal integer: {error}"
                    ))
                })
        })
        .transpose()
}

fn usage() -> AppError {
    AppError::usage(
        "usage: IsaacCsT0 inspect|smoke|analyze ... | mode-* ... | boundary-* ... | loop-* ... | coop-inspect|coop-join-preflight|coop-smoke|coop-analyze|coop-compare|coop-c0-gate|coop-passive-gate|coop-passive-applicability-gate ... | input-* ...; coop-join-preflight --game-exe <PE> --repository-root <root> [--output <new-json>] checks static prerequisites only and cannot arm or inject; coop-smoke accepts either the frozen C0 contract or --probe passive with one of six CS-T0-05-03 scenarios and an exact --verification artifact; passive rejects interval, arm, input, transport, scheduler and apply options",
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    fn values(items: &[&str]) -> Vec<OsString> {
        items.iter().map(OsString::from).collect()
    }

    fn loopback_input_smoke_args() -> Vec<&'static str> {
        vec![
            "input-smoke",
            "--probe",
            "loopback",
            "--scenario",
            "loopback",
            "--game-exe",
            "game",
            "--loop-ledger",
            "loop.json",
            "--target-ledger",
            "target.json",
            "--target-equivalence",
            "equivalence.json",
            "--mode-gate",
            "mode-gate.json",
            "--loop-gate",
            "loop-gate.json",
            "--raw-prereq",
            "raw-prereq.json",
            "--active-catalog",
            "input-active-catalog-v2.json",
            "--active-plan",
            "input-active-plan-v2.json",
            "--offline-prereq",
            "offline-prerequisite.json",
            "--arm-grant",
            "arm-grant.json",
            "--active-verification",
            "active-verification-v1.json",
            "--input-script",
            "input-active-repeatability-script-v1.json",
            "--expected-variant",
            "variant",
            "--artifacts-root",
            "artifacts",
            "--case-id",
            "case",
            "--arm-token",
            "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
        ]
    }

    fn control_no_native_input_smoke_args() -> Vec<&'static str> {
        vec![
            "input-smoke",
            "--probe",
            "control-no-native",
            "--scenario",
            "control-no-native",
            "--game-exe",
            "game",
            "--loop-ledger",
            "loop.json",
            "--target-ledger",
            "target.json",
            "--target-equivalence",
            "equivalence.json",
            "--mode-gate",
            "mode-gate.json",
            "--loop-gate",
            "loop-gate.json",
            "--raw-prereq",
            "raw-prereq.json",
            "--active-catalog",
            "input-active-catalog-v2.json",
            "--active-plan",
            "input-active-plan-v2.json",
            "--expected-variant",
            "variant",
            "--artifacts-root",
            "artifacts",
            "--case-id",
            "case",
        ]
    }

    fn original_online_disarmed_input_smoke_args() -> Vec<&'static str> {
        let mut args = control_no_native_input_smoke_args();
        let probe = args.iter().position(|value| *value == "--probe").unwrap();
        args[probe + 1] = "original-online-disarmed";
        let scenario = args
            .iter()
            .position(|value| *value == "--scenario")
            .unwrap();
        args[scenario + 1] = "original-online-disarmed";
        args
    }

    #[test]
    fn parser_rejects_unknown_and_duplicate_options() {
        assert!(parse_args(values(&["inspect", "--wat", "x"])).is_err());
        assert!(
            parse_args(values(&[
                "inspect",
                "--game-exe",
                "a",
                "--game-exe",
                "b",
                "--ledger",
                "l",
                "--expected-variant",
                "v",
            ]))
            .is_err()
        );
    }

    #[test]
    fn input_dispatch_classification_is_schema_strict() {
        assert_eq!(
            classify_input_artifact(
                crate::input_artifact::INPUT_ARTIFACT_SCHEMA_VERSION,
                crate::input_artifact::EVIDENCE_PIPELINE_RUNTIME,
            )
            .unwrap(),
            InputArtifactDispatch::InputV2
        );
        assert_eq!(
            classify_input_artifact(
                crate::raw_apply_artifact::RAW_APPLY_ARTIFACT_SCHEMA_VERSION,
                crate::raw_apply_artifact::EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME,
            )
            .unwrap(),
            InputArtifactDispatch::RawApplyV3
        );
        for evidence in [
            crate::input_artifact::EVIDENCE_PIPELINE_RUNTIME,
            crate::input_artifact::EVIDENCE_DISARMED_CONTROL,
        ] {
            assert_eq!(
                classify_input_artifact(
                    crate::active_artifact::ACTIVE_ARTIFACT_SCHEMA_VERSION,
                    evidence,
                )
                .unwrap(),
                InputArtifactDispatch::ActiveV4
            );
        }
        assert!(
            classify_input_artifact(
                crate::active_artifact::ACTIVE_ARTIFACT_SCHEMA_VERSION,
                crate::raw_apply_artifact::EVIDENCE_PASSIVE_RAW_APPLY_RUNTIME,
            )
            .is_err()
        );
        assert!(
            classify_input_artifact(
                crate::raw_apply_artifact::RAW_APPLY_ARTIFACT_SCHEMA_VERSION,
                crate::input_artifact::EVIDENCE_PIPELINE_RUNTIME,
            )
            .is_err()
        );
        assert!(classify_input_artifact(99, "UNKNOWN").is_err());
    }

    #[test]
    fn input_gate_matrix_and_option_scope_are_schema_strict() {
        let directory = tempfile::tempdir().unwrap();
        let legacy = directory.path().join("legacy.json");
        fs::write(&legacy, br#"{"analyses":[]}"#).unwrap();
        assert_eq!(
            classify_input_gate_matrix(&legacy).unwrap(),
            InputGateDispatch::Legacy
        );
        validate_input_gate_option_scope(InputGateDispatch::Legacy, None, None, None, None)
            .unwrap();
        assert!(
            validate_input_gate_option_scope(
                InputGateDispatch::Legacy,
                Some(Path::new("mode.json")),
                None,
                None,
                None,
            )
            .is_err()
        );

        let active = directory.path().join("active.json");
        fs::write(
            &active,
            br#"{"schemaVersion":2,"matrixIdentity":"IsaacCsT0-ActiveMatrix-v2","cases":[],"repeatabilityAnalyses":[]}"#,
        )
        .unwrap();
        assert_eq!(
            classify_input_gate_matrix(&active).unwrap(),
            InputGateDispatch::ActiveV2
        );
        assert!(
            validate_input_gate_option_scope(InputGateDispatch::ActiveV2, None, None, None, None)
                .is_err()
        );
        validate_input_gate_option_scope(
            InputGateDispatch::ActiveV2,
            Some(Path::new("mode.json")),
            Some(Path::new("raw.json")),
            Some(Path::new("plan.json")),
            Some(Path::new("verification.json")),
        )
        .unwrap();

        let bad_identity = directory.path().join("bad-identity.json");
        fs::write(
            &bad_identity,
            br#"{"schemaVersion":2,"matrixIdentity":"IsaacCsT0-ActiveMatrix-v1"}"#,
        )
        .unwrap();
        assert!(classify_input_gate_matrix(&bad_identity).is_err());
    }

    #[test]
    fn input_compare_rejects_cross_schema_before_event_replay() {
        let directory = tempfile::tempdir().unwrap();
        let input = directory.path().join("input");
        let active = directory.path().join("active");
        fs::create_dir(&input).unwrap();
        fs::create_dir(&active).unwrap();
        fs::write(
            input.join("manifest.json"),
            br#"{"schemaVersion":2,"evidenceClass":"PIPELINE_RUNTIME"}"#,
        )
        .unwrap();
        fs::write(
            active.join("manifest.json"),
            br#"{"schemaVersion":4,"evidenceClass":"PIPELINE_RUNTIME"}"#,
        )
        .unwrap();
        let error = run(Command::InputCompare {
            first: input,
            second: active,
            output: None,
        })
        .unwrap_err();
        assert_eq!(
            error.message,
            "input comparison sources use different artifact schemas"
        );
    }

    #[test]
    fn loopback_parser_requires_exact_active_only_options() {
        for required in [
            "--mode-gate",
            "--loop-gate",
            "--raw-prereq",
            "--active-plan",
            "--offline-prereq",
            "--arm-grant",
            "--active-verification",
            "--input-script",
            "--arm-token",
        ] {
            let mut args = loopback_input_smoke_args();
            let position = args.iter().position(|value| *value == required).unwrap();
            args.drain(position..=position + 1);
            assert!(
                parse_args(values(&args)).is_err(),
                "missing {required} must fail closed"
            );
        }

        let mut zero_token = loopback_input_smoke_args();
        let token = zero_token
            .iter()
            .position(|value| *value == "--arm-token")
            .unwrap();
        zero_token[token + 1] = "0000000000000000000000000000000000000000000000000000000000000000";
        assert!(parse_args(values(&zero_token)).is_err());

        let mut duplicate = loopback_input_smoke_args();
        duplicate.extend(["--active-plan", "second-plan.json"]);
        assert!(parse_args(values(&duplicate)).is_err());

        let mut ordinary_active = loopback_input_smoke_args();
        let scenario = ordinary_active
            .iter()
            .position(|value| *value == "--scenario")
            .unwrap();
        ordinary_active[scenario + 1] = "keyboard";
        let script = ordinary_active
            .iter()
            .position(|value| *value == "--input-script")
            .unwrap();
        ordinary_active.drain(script..=script + 1);
        assert!(parse_args(values(&ordinary_active)).is_ok());

        let mut unexpected_script = ordinary_active.clone();
        unexpected_script.extend(["--input-script", "script.json"]);
        assert!(parse_args(values(&unexpected_script)).is_err());

        let mut disguised_control = ordinary_active.clone();
        let scenario = disguised_control
            .iter()
            .position(|value| *value == "--scenario")
            .unwrap();
        disguised_control[scenario + 1] = "control-no-native";
        assert!(parse_args(values(&disguised_control)).is_err());

        for active_only in [
            "--mode-gate",
            "--raw-prereq",
            "--active-plan",
            "--offline-prereq",
            "--arm-grant",
            "--active-verification",
            "--input-script",
            "--arm-token",
        ] {
            let mut passive = vec![
                "input-smoke",
                "--probe",
                "passive",
                "--scenario",
                "keyboard",
                "--game-exe",
                "game",
                "--loop-ledger",
                "loop.json",
                "--loop-gate",
                "loop-gate.json",
                "--expected-variant",
                "variant",
                "--artifacts-root",
                "artifacts",
                "--case-id",
                "case",
            ];
            passive.extend([active_only, "forbidden"]);
            assert!(
                parse_args(values(&passive)).is_err(),
                "passive must reject {active_only}"
            );
        }

        assert!(
            parse_args(values(&[
                "input-smoke",
                "--probe",
                "control-no-native",
                "--scenario",
                "keyboard",
                "--game-exe",
                "game",
                "--loop-ledger",
                "loop.json",
                "--expected-variant",
                "variant",
                "--artifacts-root",
                "artifacts",
                "--case-id",
                "case",
            ]))
            .is_err()
        );
    }

    #[test]
    fn disarmed_parsers_require_read_only_bundle_and_forbid_arm_options() {
        for args in [
            control_no_native_input_smoke_args(),
            original_online_disarmed_input_smoke_args(),
        ] {
            assert!(parse_args(values(&args)).is_ok());
            for required in [
                "--target-ledger",
                "--target-equivalence",
                "--mode-gate",
                "--loop-gate",
                "--raw-prereq",
                "--active-catalog",
                "--active-plan",
            ] {
                let mut missing = args.clone();
                let position = missing.iter().position(|value| *value == required).unwrap();
                missing.drain(position..=position + 1);
                assert!(
                    parse_args(values(&missing)).is_err(),
                    "missing {required} must fail closed"
                );
            }
            for forbidden in [
                "--offline-prereq",
                "--arm-grant",
                "--active-verification",
                "--arm-token",
                "--input-script",
            ] {
                let mut armed = args.clone();
                armed.extend([forbidden, "forbidden"]);
                assert!(
                    parse_args(values(&armed)).is_err(),
                    "disarmed probe must reject {forbidden}"
                );
            }
        }
    }

    #[cfg(windows)]
    #[test]
    fn blocked_rpm_inspect_and_capture_are_command_failures() {
        assert!(require_rpm_diagnostic_pass("PASS_DIAGNOSTIC", "inspect").is_ok());
        assert!(require_rpm_diagnostic_pass("BLOCKED_BY_UNSTABLE_SNAPSHOT", "inspect").is_err());
        assert!(require_rpm_diagnostic_pass("BLOCKED_BY_UNSTABLE_SNAPSHOT", "capture").is_err());
    }

    #[test]
    fn parser_accepts_all_t0_02_interfaces_and_optional_peer() {
        assert!(matches!(
            parse_args(values(&[
                "mode-inspect",
                "--game-exe",
                "game",
                "--ledger",
                "mode.json",
                "--expected-variant",
                "variant",
            ]))
            .unwrap(),
            Command::ModeInspect { .. }
        ));
        assert!(matches!(
            parse_args(values(&[
                "mode-smoke",
                "--scenario",
                "offline-disarmed",
                "--endpoint",
                "solo",
                "--game-exe",
                "game",
                "--ledger",
                "mode.json",
                "--expected-variant",
                "variant",
                "--artifacts-root",
                "artifacts",
                "--case-id",
                "case",
            ]))
            .unwrap(),
            Command::ModeSmoke { .. }
        ));
        assert!(matches!(
            parse_args(values(&[
                "mode-compare",
                "--control",
                "c",
                "--instrumented",
                "i",
                "--peer",
                "p",
            ]))
            .unwrap(),
            Command::ModeCompare { peer: Some(_), .. }
        ));
        assert!(matches!(
            parse_args(values(&[
                "mode-gate",
                "--baseline",
                "baseline",
                "--offline-control",
                "oc",
                "--offline-instrumented",
                "oi",
                "--control-a",
                "ca",
                "--control-b",
                "cb",
                "--instrumented-a",
                "ia",
                "--instrumented-b",
                "ib",
                "--comparison",
                "comparison.json",
                "--output",
                "gate.json",
            ]))
            .unwrap(),
            Command::ModeGate { .. }
        ));
    }

    #[test]
    fn parser_accepts_all_t0_03_interfaces() {
        assert!(matches!(
            parse_args(values(&[
                "boundary-inspect",
                "--game-exe",
                "game",
                "--ledger",
                "boundary.json",
                "--expected-variant",
                "variant",
            ]))
            .unwrap(),
            Command::BoundaryInspect { .. }
        ));
        assert!(matches!(
            parse_args(values(&[
                "boundary-smoke",
                "--probe",
                "passive",
                "--scenario",
                "normal",
                "--game-exe",
                "game",
                "--ledger",
                "boundary.json",
                "--expected-variant",
                "variant",
                "--artifacts-root",
                "artifacts",
                "--case-id",
                "case",
            ]))
            .unwrap(),
            Command::BoundarySmoke { .. }
        ));
        assert!(matches!(
            parse_args(values(&[
                "boundary-compare",
                "--control",
                "control",
                "--instrumented",
                "passive",
            ]))
            .unwrap(),
            Command::BoundaryCompare { output: None, .. }
        ));
    }

    #[test]
    fn parser_accepts_t0_03r_and_t0_04_interfaces() {
        assert!(matches!(
            parse_args(values(&[
                "loop-inspect",
                "--game-exe",
                "game",
                "--ledger",
                "loop.json",
                "--expected-variant",
                "variant",
            ]))
            .unwrap(),
            Command::LoopInspect { .. }
        ));
        assert!(matches!(
            parse_args(values(&[
                "loop-smoke",
                "--probe",
                "passive",
                "--scenario",
                "keyboard",
                "--game-exe",
                "game",
                "--ledger",
                "loop.json",
                "--expected-variant",
                "variant",
                "--artifacts-root",
                "artifacts",
                "--case-id",
                "case",
            ]))
            .unwrap(),
            Command::LoopSmoke { .. }
        ));
        assert!(matches!(
            parse_args(values(&[
                "input-inspect",
                "--game-exe",
                "game",
                "--loop-ledger",
                "loop.json",
                "--expected-variant",
                "variant",
            ]))
            .unwrap(),
            Command::InputInspect { .. }
        ));
        assert!(matches!(
            parse_args(values(&[
                "input-target-equivalence",
                "--game-exe",
                "game",
                "--loop-ledger",
                "loop.json",
                "--target-ledger",
                "target.json",
                "--active-catalog",
                "catalog.json",
                "--active-plan",
                "plan.json",
                "--output",
                "equivalence.json",
            ]))
            .unwrap(),
            Command::InputTargetEquivalence { .. }
        ));
        assert!(matches!(
            parse_args(values(&loopback_input_smoke_args())).unwrap(),
            Command::InputSmoke {
                arm_token: Some(_),
                mode_gate: Some(_),
                loop_gate: Some(_),
                raw_prereq: Some(_),
                active_plan: Some(_),
                offline_prereq: Some(_),
                arm_grant: Some(_),
                active_verification: Some(_),
                input_script: Some(_),
                ..
            }
        ));
        assert!(matches!(
            parse_args(values(&[
                "input-offline-attest",
                "--scenario",
                "keyboard",
                "--game-exe",
                "game",
                "--loop-ledger",
                "loop.json",
                "--target-ledger",
                "target.json",
                "--target-equivalence",
                "equivalence.json",
                "--mode-gate",
                "mode-gate.json",
                "--loop-gate",
                "loop-gate.json",
                "--raw-prereq",
                "raw-prereq.json",
                "--active-catalog",
                "input-active-catalog-v2.json",
                "--active-plan",
                "input-active-plan-v2.json",
                "--expected-variant",
                "variant",
                "--artifacts-root",
                "artifacts",
                "--case-id",
                "case",
            ]))
            .unwrap(),
            Command::InputOfflineAttest { .. }
        ));
        assert!(matches!(
            parse_args(values(&[
                "input-arm-grant",
                "--offline-prereq",
                "prerequisite.json",
                "--case",
                "case",
                "--scenario",
                "keyboard",
                "--arm-token",
                "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
                "--output",
                "grant.json",
            ]))
            .unwrap(),
            Command::InputArmGrant { .. }
        ));
        assert!(matches!(
            parse_args(values(&[
                "input-smoke",
                "--probe",
                "passive",
                "--scenario",
                "keyboard",
                "--game-exe",
                "game",
                "--loop-ledger",
                "loop.json",
                "--loop-gate",
                "loop-gate.json",
                "--expected-variant",
                "variant",
                "--artifacts-root",
                "artifacts",
                "--case-id",
                "case",
            ]))
            .unwrap(),
            Command::InputSmoke {
                probe,
                arm_token: None,
                loop_gate: Some(_),
                ..
            } if probe == "passive"
        ));
        assert!(matches!(
            parse_args(values(&[
                "input-import-witness",
                "--witness-log",
                "log.txt",
                "--output",
                "artifact",
                "--scenario",
                "keyboard",
                "--case-id",
                "case",
                "--target-sha256",
                "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
                "--loop-gate-sha256",
                "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
            ]))
            .unwrap(),
            Command::InputImportWitness { .. }
        ));
        assert!(matches!(
            parse_args(values(&[
                "input-rpm-inspect",
                "--game-exe",
                "game",
                "--pid",
                "20224",
            ]))
            .unwrap(),
            Command::InputRpmInspect {
                pid: Some(20224),
                ..
            }
        ));
        assert!(matches!(
            parse_args(values(&[
                "input-rpm-capture",
                "--game-exe",
                "game",
                "--output",
                "rpm-artifact",
                "--duration-ms",
                "5000",
                "--interval-ms",
                "10",
            ]))
            .unwrap(),
            Command::InputRpmCapture {
                pid: None,
                duration_milliseconds: 5000,
                interval_milliseconds: 10,
                ..
            }
        ));
        assert!(matches!(
            parse_args(values(&["input-rpm-analyze", "rpm-artifact"])).unwrap(),
            Command::InputRpmAnalyze { .. }
        ));
        let raw_args = [
            "input-raw-smoke",
            "--probe",
            "passive",
            "--scenario",
            "keyboard-held-release",
            "--game-exe",
            "game",
            "--loop-ledger",
            "loop.json",
            "--loop-gate",
            "loop-gate.json",
            "--raw-apply-catalog",
            "cs-t0/raw-apply-catalog-v4.json",
            "--capture-plan",
            "cs-t0/raw-apply-capture-plan-v4.json",
            "--expected-variant",
            "variant",
            "--artifacts-root",
            "artifacts",
            "--case-id",
            "case",
        ];
        assert!(matches!(
            parse_args(values(&raw_args)).unwrap(),
            Command::InputRawSmoke { probe, .. } if probe == "passive"
        ));
        let mut armed_raw = raw_args.to_vec();
        armed_raw.extend(["--arm-token", "forbidden"]);
        assert!(parse_args(values(&armed_raw)).is_err());
        assert!(matches!(
            parse_args(values(&[
                "input-raw-prereq",
                "--matrix",
                "input-raw-prereq-matrix-v1.json",
                "--output",
                "raw-prerequisite.json",
            ]))
            .unwrap(),
            Command::InputRawPrereq { .. }
        ));
        assert!(parse_args(values(&["input-raw-prereq", "--matrix", "matrix.json",])).is_err());
        assert!(
            parse_args(values(&[
                "input-raw-prereq",
                "--matrix",
                "matrix.json",
                "--output",
                "decision.json",
                "--arm-token",
                "forbidden",
            ]))
            .is_err()
        );
        assert!(matches!(
            parse_args(values(&[
                "input-gate",
                "--loop-gate",
                "loop-gate.json",
                "--matrix",
                "matrix.json",
                "--repeatability",
                "repeat.json",
                "--output",
                "gate.json",
            ]))
            .unwrap(),
            Command::InputGate { .. }
        ));
        assert!(matches!(
            parse_args(values(&[
                "input-gate",
                "--mode-gate",
                "mode-gate.json",
                "--loop-gate",
                "loop-gate.json",
                "--raw-prereq",
                "raw-prerequisite.json",
                "--active-plan",
                "input-active-plan-v2.json",
                "--active-verification",
                "active-verification-v1.json",
                "--matrix",
                "active-matrix-v2.json",
                "--repeatability",
                "active-repeatability.json",
                "--output",
                "active-gate.json",
            ]))
            .unwrap(),
            Command::InputGate {
                mode_gate: Some(_),
                raw_prereq: Some(_),
                active_plan: Some(_),
                active_verification: Some(_),
                ..
            }
        ));
    }

    #[test]
    fn parser_accepts_coop_inspect_with_optional_create_new_output() {
        let base = [
            "coop-inspect",
            "--game-exe",
            "game.exe",
            "--target-equivalence",
            "equivalence.json",
            "--bootstrap-decision",
            "bootstrap.json",
            "--ledger",
            "ledger.json",
            "--catalog",
            "catalog.json",
            "--graph",
            "graph.json",
        ];
        assert!(matches!(
            parse_args(values(&base)).unwrap(),
            Command::CoopInspect { output: None, .. }
        ));
        let mut with_output = base.to_vec();
        with_output.extend(["--output", "inspection.json"]);
        assert!(matches!(
            parse_args(values(&with_output)).unwrap(),
            Command::CoopInspect {
                output: Some(_),
                ..
            }
        ));
        assert!(
            parse_args(values(&[
                "coop-inspect",
                "--game-exe",
                "game.exe",
                "--target-equivalence",
                "equivalence.json",
            ]))
            .is_err()
        );
    }

    #[test]
    fn parser_accepts_only_the_fixed_coop_c0_capability() {
        let capture = [
            "coop-smoke",
            "--probe",
            "control-no-native",
            "--scenario",
            "local-coop-dual-c0",
            "--game-exe",
            "game.exe",
            "--target-equivalence",
            "equivalence.json",
            "--layout-evidence",
            "layout.json",
            "--catalog",
            "catalog.json",
            "--graph",
            "graph.json",
            "--plan",
            "plan.json",
            "--artifacts-root",
            "artifacts",
            "--case-id",
            "local-dual-01",
        ];
        assert!(matches!(
            parse_args(values(&capture)).unwrap(),
            Command::CoopSmoke {
                duration_milliseconds: 5_000,
                interval_milliseconds: 50,
                ..
            }
        ));
        let mut active = capture.to_vec();
        active.extend(["--arm-token", "forbidden"]);
        assert!(parse_args(values(&active)).is_err());
        let mut wrong_probe = capture.to_vec();
        wrong_probe[2] = "loopback";
        assert!(parse_args(values(&wrong_probe)).is_err());
        let mut wrong_scenario = capture.to_vec();
        wrong_scenario[4] = "loopback";
        assert!(parse_args(values(&wrong_scenario)).is_err());

        assert!(matches!(
            parse_args(values(&[
                "coop-analyze",
                "source",
                "--output",
                "source/analysis.json",
            ]))
            .unwrap(),
            Command::CoopAnalyze { .. }
        ));
        assert!(matches!(
            parse_args(values(&[
                "coop-compare",
                "--offline-single",
                "offline",
                "--local-coop-dual",
                "local",
                "--original-online-dual",
                "online",
                "--output",
                "comparison.json",
            ]))
            .unwrap(),
            Command::CoopCompare { .. }
        ));
        assert!(matches!(
            parse_args(values(&[
                "coop-c0-gate",
                "--offline-single",
                "offline",
                "--local-coop-dual",
                "local",
                "--original-online-dual",
                "online",
                "--comparison",
                "comparison.json",
                "--loop-gate",
                "loop-gate.json",
                "--active-decision",
                "active-decision.json",
                "--output",
                "decision.json",
            ]))
            .unwrap(),
            Command::CoopC0Gate { .. }
        ));
    }

    #[test]
    fn masked_comparison_ignores_only_zero_mask_bytes() {
        assert!(masked_equal(&[1, 9, 3], &[1, 2, 3], &[0xFF, 0, 0xFF]));
        assert!(!masked_equal(&[0, 9, 3], &[1, 2, 3], &[0xFF, 0, 0xFF]));
    }
}
