//! Black-box contract tests for the public CS-T0-05-02 C0 API.

use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

use isaac_cs_t0::active_artifact::{ActiveGateDecision, analyze_active_run, replay_active_run};
use isaac_cs_t0::artifact::file_sha256;
use isaac_cs_t0::coop_c0::{
    C0_ANALYSIS_IDENTITY, C0_COMPARISON_IDENTITY, C0_EVIDENCE_CLASS, C0_GATE_IDENTITY,
    C0_OBSERVER_ACCESS_BITS, C0_PROBE, C0_SCHEMA_VERSION, C0_SLOT_COUNT, C0_TOOL_IDENTITY,
    C0_WORK_PACKAGE, C0GraphObservation, C0Manifest, C0PlayerObservation, C0Sample,
    CodeObservation, DEFAULT_DURATION_MILLISECONDS, DEFAULT_INTERVAL_MILLISECONDS,
    ExpectedTargetDocument, ExpectedTargetSlot, MAX_RETRIES, ObservedTargetDocument,
    ObservedTargetSlot, SCENARIO_LOCAL_DUAL, SCENARIO_OFFLINE_SINGLE, SCENARIO_ORIGINAL_ONLINE,
    analyze, analyze_to, compare, compare_to, gate_to, validate_capture_options, validate_scenario,
};
use isaac_cs_t0::loop_verification::{LoopGateDecision, analyze_loop_run, replay_loop_run};
use serde::Serialize;
use serde::de::DeserializeOwned;
use serde_json::{Value, json};
use tempfile::TempDir;

const STATUS_COHERENT: &str = "COHERENT_C0_READ_ONLY";
const STATUS_UNSTABLE: &str = "DROPPED_UNSTABLE";
const LUA_STATIC_ORDER: &str = "STATIC_VECTOR_EQUIVALENT";
const TARGET_EQUIVALENCE_SHA256: &str =
    "8B3FB097ED32B18DE6A78F535045ACD525185569ACA029A6370494511851F138";
const LAYOUT_EVIDENCE_SHA256: &str =
    "921EE4C4AA1FDCE86E21F1CFF1B38D5802E13E715242EC6006FA79B5F6316059";
const COOP_CATALOG_SHA256: &str =
    "1059711778DAD09870D37118571EE7D5E67761EC28FFAB84C7CF5AC5998A0224";
const COOP_GRAPH_SHA256: &str = "F87DA1DF383B7B1AE3724A4DB4EAAE791B9F42B68AF9210AD1727F71D9458934";
const COOP_PLAN_SHA256: &str = "12BE7D22A1103A57923A1B6894B3D6C3433A165E431B06C44B28CB1DF21C6850";
const OBSERVED_TARGET_SHA256: &str =
    "1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C";

type GraphMutation = (&'static str, Box<dyn Fn(&mut C0GraphObservation)>);

fn digest(byte: char) -> String {
    std::iter::repeat_n(byte.to_ascii_uppercase(), 64).collect()
}

fn write_json(path: &Path, value: &impl Serialize) {
    fs::write(
        path,
        serde_json::to_vec_pretty(value).expect("serialize fixture"),
    )
    .expect("write fixture");
}

fn read_json<T: DeserializeOwned>(path: &Path) -> T {
    serde_json::from_slice(&fs::read(path).expect("read fixture")).expect("parse fixture")
}

fn write_samples_file(path: &Path, samples: &[C0Sample]) {
    let mut bytes = Vec::new();
    for sample in samples {
        serde_json::to_writer(&mut bytes, sample).expect("serialize sample");
        bytes.push(b'\n');
    }
    fs::write(path, bytes).expect("write samples");
}

fn code_observations() -> Vec<CodeObservation> {
    vec![
        CodeObservation {
            id: "steady-standard-frame-store".to_owned(),
            rva: "0x002FBA0B".to_owned(),
            width: 6,
            bytes_hex: "89B7F8640200".to_owned(),
            semantic: "GAME_ROOT_FRAME_PRE_ATTEMPT_STORE".to_owned(),
        },
        CodeObservation {
            id: "steady-standard-room-update".to_owned(),
            rva: "0x002FBA4D".to_owned(),
            width: 5,
            bytes_hex: "E82E6F1000".to_owned(),
            semantic: "REL32_TARGET:0x00402980".to_owned(),
        },
        CodeObservation {
            id: "steady-standard-post-tail".to_owned(),
            rva: "0x002FBBA5".to_owned(),
            width: 5,
            bytes_hex: "E8F64B1600".to_owned(),
            semantic: "REL32_TARGET:0x004607A0".to_owned(),
        },
    ]
}

fn target_documents() -> (
    ExpectedTargetDocument,
    ObservedTargetDocument,
    ObservedTargetDocument,
) {
    let expected_slots = (0..C0_SLOT_COUNT)
        .map(|ordinal| {
            let pointer_slot = ordinal == 0;
            ExpectedTargetSlot {
                ordinal: ordinal as u32,
                id: format!("slot-{ordinal:02}"),
                owner: "FROZEN_T0_04_OR_T0_05".to_owned(),
                patch_kind: if pointer_slot {
                    "IMPORT_POINTER"
                } else {
                    "REL32_CALL"
                }
                .to_owned(),
                rva: format!("0x{:08X}", 0x0010_0000 + ordinal * 4),
                width: 4,
                expected_bytes_proof: Some(if pointer_slot {
                    format!("OPAQUE-{:032X}", ordinal + 1)
                } else {
                    format!("BYTE-{ordinal:08X}")
                }),
                expected_semantic: format!("SEMANTIC_{ordinal:02}"),
            }
        })
        .collect::<Vec<_>>();
    let observed_slots = expected_slots
        .iter()
        .map(|slot| ObservedTargetSlot {
            ordinal: slot.ordinal,
            id: slot.id.clone(),
            rva: slot.rva.clone(),
            bytes_proof: slot
                .expected_bytes_proof
                .clone()
                .expect("fixture byte proof"),
            semantic: slot.expected_semantic.clone(),
        })
        .collect::<Vec<_>>();
    let expected = ExpectedTargetDocument {
        schema_version: C0_SCHEMA_VERSION,
        document_identity: "IsaacCsT0-CoopC0ExpectedTargets-v1".to_owned(),
        slot_count: C0_SLOT_COUNT,
        slots: expected_slots,
        tick_path: code_observations(),
    };
    let before = ObservedTargetDocument {
        schema_version: C0_SCHEMA_VERSION,
        document_identity: "IsaacCsT0-CoopC0ObservedTargets-v1".to_owned(),
        phase: "BEFORE".to_owned(),
        slot_count: C0_SLOT_COUNT,
        slots: observed_slots.clone(),
        tick_path: code_observations(),
    };
    let after = ObservedTargetDocument {
        phase: "AFTER".to_owned(),
        slots: observed_slots,
        ..before.clone()
    };
    (expected, before, after)
}

fn player(ordinal: u32, device_kind: Option<&str>, token: &str) -> C0PlayerObservation {
    C0PlayerObservation {
        ordinal,
        object_token: token.to_owned(),
        device_kind: device_kind.map(str::to_owned),
        unique_assignment: true,
        ordinal_field_matches: true,
        hud_primary_slot_matches: true,
        lua_order_evidence: LUA_STATIC_ORDER.to_owned(),
        init_seed_stable: true,
        init_seed_nonzero: true,
        init_seed_distinct: true,
        twin_relation_absent: true,
    }
}

fn graph(scenario: &str) -> C0GraphObservation {
    match scenario {
        SCENARIO_OFFLINE_SINGLE => C0GraphObservation {
            topology_sha256: digest('A'),
            run_active: true,
            online_mode: false,
            manager_present: true,
            network_backend_present: false,
            net_input_present: false,
            player_count: 1,
            device_count: 1,
            roster_count: 0,
            players: vec![player(0, Some("KEYBOARD"), "O-OFFLINE-P0")],
            unassigned_device_kinds: Vec::new(),
            primary_hud_consistent: true,
            twin_hud_empty: true,
        },
        SCENARIO_LOCAL_DUAL => C0GraphObservation {
            topology_sha256: digest('B'),
            run_active: true,
            online_mode: false,
            manager_present: true,
            network_backend_present: false,
            net_input_present: false,
            player_count: 2,
            device_count: 2,
            roster_count: 0,
            players: vec![
                player(0, Some("KEYBOARD"), "O-LOCAL-P0"),
                player(1, Some("GAMEPAD"), "O-LOCAL-P1"),
            ],
            unassigned_device_kinds: Vec::new(),
            primary_hud_consistent: true,
            twin_hud_empty: true,
        },
        SCENARIO_ORIGINAL_ONLINE => {
            let mut players = vec![
                player(0, None, "O-ONLINE-P0"),
                player(1, None, "O-ONLINE-P1"),
            ];
            for player in &mut players {
                player.unique_assignment = false;
            }
            C0GraphObservation {
                topology_sha256: digest('C'),
                run_active: true,
                online_mode: true,
                manager_present: true,
                network_backend_present: true,
                net_input_present: true,
                player_count: 2,
                device_count: 2,
                roster_count: 2,
                players,
                unassigned_device_kinds: vec!["NET_INPUT".to_owned(), "NET_INPUT".to_owned()],
                primary_hud_consistent: true,
                twin_hud_empty: true,
            }
        }
        _ => panic!("unsupported fixture scenario"),
    }
}

fn coherent_sample(sequence: u64, scenario: &str, delta: u32) -> C0Sample {
    C0Sample {
        schema_version: C0_SCHEMA_VERSION,
        observer_sequence: sequence,
        qpc: 10_000 + sequence as i64,
        status: STATUS_COHERENT.to_owned(),
        retry_count: 0,
        reason: None,
        game_frame_delta: Some(delta),
        online_frame_delta: (scenario == SCENARIO_ORIGINAL_ONLINE).then_some(delta),
        graph: Some(graph(scenario)),
    }
}

fn dropped_sample(sequence: u64) -> C0Sample {
    C0Sample {
        schema_version: C0_SCHEMA_VERSION,
        observer_sequence: sequence,
        qpc: 10_000 + sequence as i64,
        status: STATUS_UNSTABLE.to_owned(),
        retry_count: MAX_RETRIES,
        reason: Some("STRUCTURAL_DOUBLE_READ_DRIFT".to_owned()),
        game_frame_delta: None,
        online_frame_delta: None,
        graph: None,
    }
}

fn samples(scenario: &str) -> Vec<C0Sample> {
    if scenario == SCENARIO_LOCAL_DUAL {
        vec![
            coherent_sample(1, scenario, 0),
            dropped_sample(2),
            coherent_sample(3, scenario, 1),
        ]
    } else {
        vec![
            coherent_sample(1, scenario, 0),
            coherent_sample(2, scenario, 1),
        ]
    }
}

fn materialize_run(parent: &Path, name: &str, scenario: &str, artifact_identity: &str) -> PathBuf {
    let run = parent.join(name);
    fs::create_dir(&run).expect("create run fixture");
    let (expected, before, after) = target_documents();
    write_json(&run.join("target-bytes-expected.json"), &expected);
    write_json(&run.join("target-bytes-before.json"), &before);
    write_json(&run.join("target-bytes-after.json"), &after);
    write_samples_file(&run.join("samples.jsonl"), &samples(scenario));

    let manifest = C0Manifest {
        schema_version: C0_SCHEMA_VERSION,
        evidence_class: C0_EVIDENCE_CLASS.to_owned(),
        tool_identity: C0_TOOL_IDENTITY.to_owned(),
        artifact_identity: artifact_identity.to_owned(),
        work_package: C0_WORK_PACKAGE.to_owned(),
        case_id: name.to_owned(),
        probe: C0_PROBE.to_owned(),
        scenario: scenario.to_owned(),
        started_utc: "2026-08-27T00:00:00Z".to_owned(),
        ended_utc: "2026-08-27T00:00:05Z".to_owned(),
        target_sha256: OBSERVED_TARGET_SHA256.to_owned(),
        target_equivalence_sha256: TARGET_EQUIVALENCE_SHA256.to_owned(),
        layout_evidence_sha256: LAYOUT_EVIDENCE_SHA256.to_owned(),
        coop_catalog_sha256: COOP_CATALOG_SHA256.to_owned(),
        coop_graph_sha256: COOP_GRAPH_SHA256.to_owned(),
        coop_plan_sha256: COOP_PLAN_SHA256.to_owned(),
        observer_access_bits: C0_OBSERVER_ACCESS_BITS.to_owned(),
        requested_duration_milliseconds: DEFAULT_DURATION_MILLISECONDS,
        interval_milliseconds: DEFAULT_INTERVAL_MILLISECONDS,
        max_retries: MAX_RETRIES,
        qpc_frequency: 10_000_000,
        process_identity_token: format!("P-{name}"),
        module_set_sha256: digest('A'),
        blockers_absent: true,
        debugger_absent: true,
        forbidden_modules_absent: true,
        process_identity_stable: true,
        module_set_stable: true,
        target_disk_stable: true,
        expected_targets_sha256: file_sha256(&run.join("target-bytes-expected.json"))
            .expect("hash expected targets"),
        target_bytes_before_sha256: file_sha256(&run.join("target-bytes-before.json"))
            .expect("hash before targets"),
        target_bytes_after_sha256: file_sha256(&run.join("target-bytes-after.json"))
            .expect("hash after targets"),
        samples_sha256: file_sha256(&run.join("samples.jsonl")).expect("hash samples"),
        hook_installed: false,
        ipc_created: false,
        mapping_created: false,
        helper_started: false,
        native_loaded: false,
        scheduler_started: false,
        apply_attempted: false,
        status: "CAPTURE_COMPLETE".to_owned(),
    };
    write_json(&run.join("manifest.json"), &manifest);
    run
}

fn read_manifest(run: &Path) -> C0Manifest {
    read_json(&run.join("manifest.json"))
}

fn write_manifest(run: &Path, manifest: &C0Manifest) {
    write_json(&run.join("manifest.json"), manifest);
}

fn read_samples(run: &Path) -> Vec<C0Sample> {
    fs::read_to_string(run.join("samples.jsonl"))
        .expect("read samples")
        .lines()
        .map(|line| serde_json::from_str(line).expect("parse sample"))
        .collect()
}

fn rewrite_samples(run: &Path, samples: &[C0Sample]) {
    write_samples_file(&run.join("samples.jsonl"), samples);
    let mut manifest = read_manifest(run);
    manifest.samples_sha256 = file_sha256(&run.join("samples.jsonl")).expect("hash samples");
    write_manifest(run, &manifest);
}

fn rewrite_after(run: &Path, after: &ObservedTargetDocument) {
    write_json(&run.join("target-bytes-after.json"), after);
    let mut manifest = read_manifest(run);
    manifest.target_bytes_after_sha256 =
        file_sha256(&run.join("target-bytes-after.json")).expect("hash after targets");
    write_manifest(run, &manifest);
}

fn rewrite_all_target_documents(
    run: &Path,
    expected: &ExpectedTargetDocument,
    before: &ObservedTargetDocument,
    after: &ObservedTargetDocument,
) {
    write_json(&run.join("target-bytes-expected.json"), expected);
    write_json(&run.join("target-bytes-before.json"), before);
    write_json(&run.join("target-bytes-after.json"), after);
    let mut manifest = read_manifest(run);
    manifest.expected_targets_sha256 =
        file_sha256(&run.join("target-bytes-expected.json")).expect("hash expected targets");
    manifest.target_bytes_before_sha256 =
        file_sha256(&run.join("target-bytes-before.json")).expect("hash before targets");
    manifest.target_bytes_after_sha256 =
        file_sha256(&run.join("target-bytes-after.json")).expect("hash after targets");
    write_manifest(run, &manifest);
}

fn create_analysis(run: &Path) {
    let analysis = analyze_to(run, &run.join("analysis.json")).expect("create analysis");
    assert_eq!(analysis.result, "PASS");
}

fn three_runs(root: &Path) -> (PathBuf, PathBuf, PathBuf) {
    (
        materialize_run(
            root,
            "offline",
            SCENARIO_OFFLINE_SINGLE,
            "C0-OFFLINE-UNIQUE",
        ),
        materialize_run(root, "local", SCENARIO_LOCAL_DUAL, "C0-LOCAL-UNIQUE"),
        materialize_run(root, "online", SCENARIO_ORIGINAL_ONLINE, "C0-ONLINE-UNIQUE"),
    )
}

fn binary() -> &'static str {
    env!("CARGO_BIN_EXE_IsaacCsT0")
}

#[test]
fn public_capture_options_and_real_cli_parser_are_fail_closed() {
    for scenario in [
        SCENARIO_OFFLINE_SINGLE,
        SCENARIO_LOCAL_DUAL,
        SCENARIO_ORIGINAL_ONLINE,
    ] {
        assert!(validate_scenario(scenario).is_ok());
        assert!(
            validate_capture_options(
                C0_PROBE,
                scenario,
                "case-1",
                DEFAULT_DURATION_MILLISECONDS,
                DEFAULT_INTERVAL_MILLISECONDS,
            )
            .is_ok()
        );
    }
    assert!(validate_scenario("online-dual-c0").is_err());
    assert!(validate_capture_options("passive", SCENARIO_LOCAL_DUAL, "case", 5_000, 50).is_err());
    assert!(validate_capture_options(C0_PROBE, SCENARIO_LOCAL_DUAL, "../case", 5_000, 50).is_err());
    assert!(validate_capture_options(C0_PROBE, SCENARIO_LOCAL_DUAL, "case", 999, 50).is_err());
    assert!(validate_capture_options(C0_PROBE, SCENARIO_LOCAL_DUAL, "case", 30_001, 50).is_err());
    assert!(validate_capture_options(C0_PROBE, SCENARIO_LOCAL_DUAL, "case", 5_000, 9).is_err());
    assert!(validate_capture_options(C0_PROBE, SCENARIO_LOCAL_DUAL, "case", 5_000, 1_001).is_err());

    let required = [
        "--scenario",
        SCENARIO_LOCAL_DUAL,
        "--game-exe",
        "game.exe",
        "--target-equivalence",
        "target.json",
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
        "case",
    ];
    let mut passive = Command::new(binary());
    passive.args(["coop-smoke", "--probe", "passive"]);
    passive.args(required);
    let passive_output = passive.output().expect("run CLI");
    assert!(!passive_output.status.success());
    assert!(String::from_utf8_lossy(&passive_output.stderr).contains("control-no-native"));

    let mut forbidden = Command::new(binary());
    forbidden.args(["coop-smoke", "--probe", C0_PROBE]);
    forbidden.args(required);
    forbidden.args(["--arm-token", "forbidden"]);
    assert!(!forbidden.output().expect("run CLI").status.success());
}

#[test]
fn real_analyzer_accepts_all_modes_and_cli_analyze_is_create_new() {
    let temp = TempDir::new().expect("create temp root");
    let (offline, local, online) = three_runs(temp.path());
    for (run, scenario) in [
        (&offline, SCENARIO_OFFLINE_SINGLE),
        (&local, SCENARIO_LOCAL_DUAL),
        (&online, SCENARIO_ORIGINAL_ONLINE),
    ] {
        let analysis = analyze(run).expect("analyze valid fixture");
        assert_eq!(analysis.analysis_identity, C0_ANALYSIS_IDENTITY);
        assert_eq!(analysis.scenario, scenario);
        assert_eq!(
            analysis.result,
            "PASS",
            "failed checks for {scenario}: {:?}",
            analysis
                .checks
                .iter()
                .filter(|check| !check.passed)
                .collect::<Vec<_>>()
        );
        assert_eq!(analysis.coherent_samples, 2);
        assert!(analysis.no_mutation_passed);
        assert!(!analysis.direct_post_commit_observed);
    }
    let local_analysis = analyze(&local).expect("analyze local");
    assert_eq!(
        local_analysis.tick_proxy_status,
        "PASS_STEADY_STANDARD_PRE_ATTEMPT_MONOTONIC"
    );
    assert_eq!(
        local_analysis.authority_commit_status,
        "PENDING_INHERITED_GATE"
    );

    let online_analysis = analyze(&online).expect("analyze OriginalOnline");
    assert_eq!(online_analysis.player_count, Some(2));
    assert_eq!(online_analysis.roster_count, Some(2));
    assert!(!online_analysis.unique_assignment);
    assert!(online_analysis.online_frame_monotonic);
    let online_samples = fs::read_to_string(online.join("samples.jsonl")).expect("read samples");
    assert!(online_samples.contains("NET_INPUT"));
    for forbidden in [
        "assignmentKey",
        "peerIdentity",
        "identityObject",
        "-2147483648",
    ] {
        assert!(
            !online_samples.contains(forbidden),
            "OriginalOnline artifact leaked {forbidden}"
        );
    }

    let output = offline.join("analysis.json");
    let cli_output = Command::new(binary())
        .args(["coop-analyze"])
        .arg(&offline)
        .args(["--output"])
        .arg(&output)
        .output()
        .expect("run coop-analyze CLI");
    assert!(cli_output.status.success());
    assert!(output.is_file());
    assert!(analyze_to(&offline, &output).is_err());
}

#[test]
fn real_deserializers_and_privacy_checks_reject_schema_and_identity_leaks() {
    let temp = TempDir::new().expect("create temp root");
    let unknown = materialize_run(
        temp.path(),
        "unknown",
        SCENARIO_OFFLINE_SINGLE,
        "C0-UNKNOWN",
    );
    let manifest_path = unknown.join("manifest.json");
    let mut manifest_value: Value = read_json(&manifest_path);
    manifest_value["futureField"] = json!(true);
    write_json(&manifest_path, &manifest_value);
    assert!(analyze(&unknown).is_err());

    let nested = materialize_run(temp.path(), "nested", SCENARIO_OFFLINE_SINGLE, "C0-NESTED");
    let samples_path = nested.join("samples.jsonl");
    let text = fs::read_to_string(&samples_path).expect("read samples");
    let mut values = text
        .lines()
        .map(|line| serde_json::from_str::<Value>(line).expect("parse sample value"))
        .collect::<Vec<_>>();
    values[0]["graph"]["players"][0]["futurePlayerField"] = json!(false);
    let mut bytes = Vec::new();
    for value in values {
        serde_json::to_writer(&mut bytes, &value).expect("serialize sample value");
        bytes.push(b'\n');
    }
    fs::write(&samples_path, bytes).expect("rewrite samples");
    let mut nested_manifest = read_manifest(&nested);
    nested_manifest.samples_sha256 = file_sha256(&samples_path).expect("hash samples");
    write_manifest(&nested, &nested_manifest);
    assert!(analyze(&nested).is_err());

    let peer_leak = materialize_run(
        temp.path(),
        "peer-leak",
        SCENARIO_ORIGINAL_ONLINE,
        "C0-PEER-LEAK",
    );
    let peer_samples_path = peer_leak.join("samples.jsonl");
    let peer_text = fs::read_to_string(&peer_samples_path).expect("read peer samples");
    let mut peer_values = peer_text
        .lines()
        .map(|line| serde_json::from_str::<Value>(line).expect("parse peer sample"))
        .collect::<Vec<_>>();
    peer_values[0]["graph"]["peerIdentity"] = json!(7_656_119_800_000_000_u64);
    let mut peer_bytes = Vec::new();
    for value in peer_values {
        serde_json::to_writer(&mut peer_bytes, &value).expect("serialize peer leak");
        peer_bytes.push(b'\n');
    }
    fs::write(&peer_samples_path, peer_bytes).expect("rewrite peer samples");
    let mut peer_manifest = read_manifest(&peer_leak);
    peer_manifest.samples_sha256 = file_sha256(&peer_samples_path).expect("hash peer samples");
    write_manifest(&peer_leak, &peer_manifest);
    assert!(analyze(&peer_leak).is_err());

    for (name, leaked_identity) in [
        ("absolute-path", r"C:\Users\local-user\case"),
        ("raw-address", "0x12345678"),
        ("file-url", "file://private/source"),
    ] {
        let run = materialize_run(temp.path(), name, SCENARIO_OFFLINE_SINGLE, leaked_identity);
        let analysis = analyze(&run).expect("privacy leaks produce replayable blocked analysis");
        assert_eq!(analysis.result, "BLOCKED_BY_EVIDENCE");
        assert!(
            analysis
                .checks
                .iter()
                .any(|check| check.id == "artifact-privacy" && !check.passed)
        );
    }
}

#[test]
fn real_analyzer_detects_permission_and_target_tampering() {
    let temp = TempDir::new().expect("create temp root");
    let permission = materialize_run(
        temp.path(),
        "permission",
        SCENARIO_LOCAL_DUAL,
        "C0-PERMISSION",
    );
    let mut manifest = read_manifest(&permission);
    manifest.hook_installed = true;
    write_manifest(&permission, &manifest);
    let analysis = analyze(&permission).expect("analyze permission tamper");
    assert_eq!(analysis.result, "BLOCKED_BY_EVIDENCE");
    assert!(!analysis.no_mutation_passed);

    let slot = materialize_run(temp.path(), "slot", SCENARIO_LOCAL_DUAL, "C0-SLOT");
    let mut after: ObservedTargetDocument = read_json(&slot.join("target-bytes-after.json"));
    after.slots[52].bytes_proof = "BYTE-FFFFFFFF".to_owned();
    rewrite_after(&slot, &after);
    let analysis = analyze(&slot).expect("analyze slot tamper");
    assert_eq!(analysis.result, "BLOCKED_BY_EVIDENCE");
    assert!(!analysis.no_mutation_passed);

    let tick = materialize_run(
        temp.path(),
        "tick-path",
        SCENARIO_LOCAL_DUAL,
        "C0-TICK-PATH",
    );
    let mut after: ObservedTargetDocument = read_json(&tick.join("target-bytes-after.json"));
    after.tick_path[1].bytes_hex = "E800000000".to_owned();
    rewrite_after(&tick, &after);
    let analysis = analyze(&tick).expect("analyze tick-path tamper");
    assert_eq!(analysis.result, "BLOCKED_BY_EVIDENCE");
    assert!(!analysis.no_mutation_passed);

    let synchronized_tick = materialize_run(
        temp.path(),
        "synchronized-tick-path",
        SCENARIO_LOCAL_DUAL,
        "C0-SYNCHRONIZED-TICK-PATH",
    );
    let mut expected: ExpectedTargetDocument =
        read_json(&synchronized_tick.join("target-bytes-expected.json"));
    let mut before: ObservedTargetDocument =
        read_json(&synchronized_tick.join("target-bytes-before.json"));
    let mut after: ObservedTargetDocument =
        read_json(&synchronized_tick.join("target-bytes-after.json"));
    for tick_path in [
        &mut expected.tick_path,
        &mut before.tick_path,
        &mut after.tick_path,
    ] {
        tick_path[0].bytes_hex = "FF87F8640200".to_owned();
        tick_path[0].semantic = "LEGACY_INCREMENT_GUESS".to_owned();
    }
    rewrite_all_target_documents(&synchronized_tick, &expected, &before, &after);
    let analysis = analyze(&synchronized_tick).expect("analyze synchronized tick drift");
    assert_eq!(analysis.result, "BLOCKED_BY_EVIDENCE");
    assert!(!analysis.no_mutation_passed);
}

#[test]
fn real_analyzer_rejects_sequence_qpc_and_drop_shape_tampering() {
    let temp = TempDir::new().expect("create temp root");
    let sequence = materialize_run(temp.path(), "sequence", SCENARIO_LOCAL_DUAL, "C0-SEQUENCE");
    let mut events = read_samples(&sequence);
    events[2].observer_sequence = 4;
    rewrite_samples(&sequence, &events);
    assert_eq!(
        analyze(&sequence).expect("analyze sequence tamper").result,
        "BLOCKED_BY_EVIDENCE"
    );

    let qpc = materialize_run(temp.path(), "qpc", SCENARIO_LOCAL_DUAL, "C0-QPC");
    let mut events = read_samples(&qpc);
    events[2].qpc = events[0].qpc - 1;
    rewrite_samples(&qpc, &events);
    assert_eq!(
        analyze(&qpc).expect("analyze QPC tamper").result,
        "BLOCKED_BY_EVIDENCE"
    );

    let drop_payload = materialize_run(
        temp.path(),
        "drop-payload",
        SCENARIO_LOCAL_DUAL,
        "C0-DROP-PAYLOAD",
    );
    let mut events = read_samples(&drop_payload);
    events[1].graph = events[0].graph.clone();
    rewrite_samples(&drop_payload, &events);
    assert_eq!(
        analyze(&drop_payload).expect("analyze drop payload").result,
        "BLOCKED_BY_EVIDENCE"
    );

    let all_dropped = materialize_run(
        temp.path(),
        "all-dropped",
        SCENARIO_LOCAL_DUAL,
        "C0-ALL-DROPPED",
    );
    rewrite_samples(&all_dropped, &[dropped_sample(1), dropped_sample(2)]);
    assert_eq!(
        analyze(&all_dropped).expect("analyze all-dropped").result,
        "BLOCKED_BY_EVIDENCE"
    );
}

#[test]
fn real_analyzer_rejects_graph_join_partition_and_tick_proxy_tampering() {
    let temp = TempDir::new().expect("create temp root");
    let mut cases: Vec<GraphMutation> = vec![
        (
            "duplicate-device",
            Box::new(|graph| graph.players[1].device_kind = Some("KEYBOARD".to_owned())),
        ),
        (
            "ambiguous-assignment",
            Box::new(|graph| graph.players[1].unique_assignment = false),
        ),
        (
            "duplicate-player-token",
            Box::new(|graph| graph.players[1].object_token = graph.players[0].object_token.clone()),
        ),
        (
            "hud-mismatch",
            Box::new(|graph| graph.players[1].hud_primary_slot_matches = false),
        ),
        (
            "lua-mismatch",
            Box::new(|graph| graph.players[1].lua_order_evidence = "UNSTABLE".to_owned()),
        ),
        (
            "online-partition",
            Box::new(|graph| {
                graph.online_mode = true;
                graph.network_backend_present = true;
                graph.net_input_present = true;
            }),
        ),
        (
            "net-input-cannot-map-local",
            Box::new(|graph| {
                graph.net_input_present = true;
                graph.unassigned_device_kinds =
                    vec!["NET_INPUT".to_owned(), "NET_INPUT".to_owned()];
                for player in &mut graph.players {
                    player.device_kind = None;
                    player.unique_assignment = false;
                }
            }),
        ),
    ];
    for (name, mutate) in cases.drain(..) {
        let run = materialize_run(
            temp.path(),
            name,
            SCENARIO_LOCAL_DUAL,
            &format!("C0-{name}"),
        );
        let mut events = read_samples(&run);
        for event in &mut events {
            if let Some(graph) = &mut event.graph {
                mutate(graph);
            }
        }
        rewrite_samples(&run, &events);
        assert_eq!(
            analyze(&run).expect("analyze graph tamper").result,
            "BLOCKED_BY_EVIDENCE",
            "accepted {name}"
        );
    }

    let regression = materialize_run(
        temp.path(),
        "frame-regression",
        SCENARIO_LOCAL_DUAL,
        "C0-FRAME-REGRESSION",
    );
    let mut events = read_samples(&regression);
    events[0].game_frame_delta = Some(2);
    events[2].game_frame_delta = Some(1);
    rewrite_samples(&regression, &events);
    let analysis = analyze(&regression).expect("analyze frame regression");
    assert_eq!(analysis.result, "BLOCKED_BY_EVIDENCE");
    assert_eq!(analysis.tick_proxy_status, "BLOCKED_BY_EVIDENCE");
    assert!(!analysis.direct_post_commit_observed);

    let online_stationary = materialize_run(
        temp.path(),
        "online-stationary",
        SCENARIO_ORIGINAL_ONLINE,
        "C0-ONLINE-STATIONARY",
    );
    let mut events = read_samples(&online_stationary);
    for event in &mut events {
        event.online_frame_delta = Some(0);
    }
    rewrite_samples(&online_stationary, &events);
    let analysis = analyze(&online_stationary).expect("analyze stationary online frame");
    assert_eq!(analysis.result, "BLOCKED_BY_EVIDENCE");
    assert!(!analysis.online_frame_monotonic);
}

#[test]
fn real_compare_replays_three_sources_and_keeps_commit_inherited() {
    let temp = TempDir::new().expect("create temp root");
    let (offline, local, online) = three_runs(temp.path());
    create_analysis(&offline);
    create_analysis(&local);
    create_analysis(&online);

    let comparison = compare(&offline, &local, &online).expect("compare three sources");
    assert_eq!(comparison.comparison_identity, C0_COMPARISON_IDENTITY);
    assert_eq!(comparison.result, "PASS");
    assert!(comparison.three_unique_roles);
    assert!(comparison.local_positive_isolated_from_original_online);
    assert!(comparison.composite_no_mutation_passed);
    assert!(comparison.dual_player_unique_join_passed);
    assert!(!comparison.direct_post_commit_observed);
    assert_eq!(comparison.authority_commit_status, "PENDING_INHERITED_GATE");

    let output = temp.path().join("comparison.json");
    let written = compare_to(&offline, &local, &online, &output).expect("write comparison");
    assert_eq!(written, comparison);
    assert!(compare_to(&offline, &local, &online, &output).is_err());
    assert!(compare(&local, &offline, &online).is_err());

    let occupied_gate = temp.path().join("occupied-decision.json");
    fs::write(&occupied_gate, b"occupied").expect("occupy gate output");
    assert!(
        gate_to(
            &offline,
            &local,
            &online,
            &output,
            Path::new("missing-loop-gate.json"),
            Path::new("missing-active-decision.json"),
            &occupied_gate,
        )
        .is_err()
    );

    let blocked_output = temp.path().join("blocked-decision.json");
    let blocked = gate_to(
        &offline,
        &local,
        &online,
        &output,
        &temp.path().join("missing-loop-gate.json"),
        &temp.path().join("missing-active-decision.json"),
        &blocked_output,
    )
    .expect("missing inherited evidence must create a blocked decision");
    assert_eq!(blocked.decision_identity, C0_GATE_IDENTITY);
    assert_eq!(blocked.status, "BLOCKED_BY_EVIDENCE");
    assert_eq!(blocked.authority_commit_status, "BLOCKED_BY_EVIDENCE");
    assert_eq!(blocked.loop_gate_sha256, "MISSING");
    assert_eq!(blocked.active_decision_sha256, "MISSING");
    assert!(!blocked.loop_gate_replayed);
    assert!(!blocked.active_sources_replayed);
    assert!(!blocked.cs_t00502_passed);
    assert!(!blocked.cs_t00503_unlocked);
    assert!(!blocked.t005_unlocked);
    assert!(!blocked.active_input_allowed);
    assert!(!blocked.passive_probe_allowed);
    assert!(!blocked.runtime_mutation_allowed);
    assert!(blocked_output.is_file());
}

#[test]
fn real_compare_detects_stored_analysis_target_and_identity_tampering() {
    let temp = TempDir::new().expect("create temp root");
    let (offline, local, online) = three_runs(temp.path());
    create_analysis(&offline);
    create_analysis(&local);
    create_analysis(&online);

    let mut stored: Value = read_json(&local.join("analysis.json"));
    stored["scenario"] = json!(SCENARIO_OFFLINE_SINGLE);
    write_json(&local.join("analysis.json"), &stored);
    assert!(compare(&offline, &local, &online).is_err());

    let second = TempDir::new().expect("create second temp root");
    let (offline, local, online) = three_runs(second.path());
    let mut online_manifest = read_manifest(&online);
    online_manifest.target_sha256 = digest('A');
    write_manifest(&online, &online_manifest);
    create_analysis(&offline);
    create_analysis(&local);
    let online_analysis =
        analyze_to(&online, &online.join("analysis.json")).expect("analyze target drift");
    assert_eq!(online_analysis.result, "BLOCKED_BY_EVIDENCE");
    let comparison = compare(&offline, &local, &online).expect("compare target drift");
    assert_eq!(comparison.result, "BLOCKED_BY_EVIDENCE");
    assert!(
        comparison
            .checks
            .iter()
            .any(|check| check.id == "shared-static-bindings" && !check.passed)
    );

    let third = TempDir::new().expect("create third temp root");
    let offline = materialize_run(
        third.path(),
        "offline",
        SCENARIO_OFFLINE_SINGLE,
        "C0-DUPLICATE",
    );
    let local = materialize_run(third.path(), "local", SCENARIO_LOCAL_DUAL, "C0-DUPLICATE");
    let online = materialize_run(
        third.path(),
        "online",
        SCENARIO_ORIGINAL_ONLINE,
        "C0-ONLINE-UNIQUE",
    );
    create_analysis(&offline);
    create_analysis(&local);
    create_analysis(&online);
    let comparison = compare(&offline, &local, &online).expect("compare duplicate identity");
    assert_eq!(comparison.result, "BLOCKED_BY_EVIDENCE");
    assert!(!comparison.three_unique_roles);
}

#[test]
fn inherited_gate_replays_frozen_analyses_without_overwriting_them() {
    let manifest_dir = Path::new(env!("CARGO_MANIFEST_DIR"));
    let repository = manifest_dir.parent().expect("repository root");
    let loop_gate_path = repository.join("artifacts/cs-t0/20260821-t0-03r-v5-loop-gate-7of7.json");
    let active_decision_path =
        repository.join("out/cs-t0/formal-active-gate-20260827-r1/active-gate-decision.json");
    assert!(
        loop_gate_path.is_file(),
        "frozen T0-03R loop gate is required"
    );
    assert!(
        active_decision_path.is_file(),
        "frozen T0-04 active decision is required"
    );

    let loop_gate: LoopGateDecision = read_json(&loop_gate_path);
    let active_decision: ActiveGateDecision = read_json(&active_decision_path);
    let loop_parent = loop_gate_path.parent().expect("loop gate parent");
    let active_parent = active_decision_path
        .parent()
        .expect("active decision parent");
    let loop_analyses = loop_gate
        .analysis_bindings
        .iter()
        .map(|binding| loop_parent.join(&binding.analysis_path))
        .collect::<Vec<_>>();
    let active_analyses = active_decision
        .analysis_bindings
        .iter()
        .map(|binding| active_parent.join(&binding.analysis_path))
        .chain(
            active_decision
                .repeatability_bindings
                .iter()
                .map(|binding| active_parent.join(&binding.analysis_path)),
        )
        .collect::<Vec<_>>();
    assert!(!loop_analyses.is_empty());
    assert!(!active_analyses.is_empty());
    let frozen_hashes = loop_analyses
        .iter()
        .chain(&active_analyses)
        .map(|path| {
            assert!(path.is_file(), "missing frozen analysis {}", path.display());
            (
                path.clone(),
                file_sha256(path).expect("hash frozen analysis"),
            )
        })
        .collect::<Vec<_>>();

    let loop_run = loop_analyses[0].parent().expect("loop run directory");
    let loop_hash = file_sha256(&loop_analyses[0]).expect("hash loop analysis");
    assert!(
        analyze_loop_run(loop_run).is_err(),
        "create-new loop analyze API must refuse an existing analysis"
    );
    assert_eq!(
        file_sha256(&loop_analyses[0]).expect("rehash loop analysis"),
        loop_hash
    );
    assert_eq!(
        replay_loop_run(loop_run)
            .expect("read-only loop replay")
            .result,
        "PASS"
    );

    let active_run = active_analyses[0].parent().expect("active run directory");
    let active_hash = file_sha256(&active_analyses[0]).expect("hash active analysis");
    assert!(
        analyze_active_run(active_run).is_err(),
        "create-new active analyze API must refuse an existing analysis"
    );
    assert_eq!(
        file_sha256(&active_analyses[0]).expect("rehash active analysis"),
        active_hash
    );
    assert_eq!(
        replay_active_run(active_run)
            .expect("read-only active replay")
            .result,
        "PASS"
    );

    let temp = TempDir::new().expect("create C0 gate fixture root");
    let (offline, local, online) = three_runs(temp.path());
    create_analysis(&offline);
    create_analysis(&local);
    create_analysis(&online);
    let comparison_path = temp.path().join("comparison.json");
    compare_to(&offline, &local, &online, &comparison_path).expect("create comparison");
    let decision_path = temp.path().join("decision.json");
    let decision = gate_to(
        &offline,
        &local,
        &online,
        &comparison_path,
        &loop_gate_path,
        &active_decision_path,
        &decision_path,
    )
    .expect("gate read-only replays inherited evidence");
    assert_eq!(decision.status, "PASS");
    assert_eq!(decision.authority_commit_status, "PASS_INHERITED_CONTRACT");
    assert!(decision.loop_gate_replayed);
    assert!(decision.active_sources_replayed);
    assert!(decision.cs_t00502_passed);
    assert!(decision.cs_t00503_unlocked);
    assert!(!decision.t005_unlocked);
    assert!(!decision.active_input_allowed);
    assert!(!decision.passive_probe_allowed);
    assert!(!decision.runtime_mutation_allowed);

    for (path, expected_hash) in frozen_hashes {
        assert_eq!(
            file_sha256(&path).expect("rehash inherited analysis"),
            expected_hash,
            "inherited replay changed {}",
            path.display()
        );
    }
}
