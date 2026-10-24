//! Black-box and serialized-contract tests for CS-T0-05-03 passive lifecycle evidence.
#![recursion_limit = "256"]

use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

use isaac_cs_t0::artifact::file_sha256;
use isaac_cs_t0::coop_passive::{
    ENDPOINT_A, ENDPOINT_B, PASSIVE_ANALYSIS_IDENTITY, PASSIVE_DEFAULT_DURATION_MILLISECONDS,
    PASSIVE_EVIDENCE_CLASS, PASSIVE_PROBE, PASSIVE_SCENARIOS, PASSIVE_SCHEMA_VERSION,
    PASSIVE_TOOL_IDENTITY, PASSIVE_WORK_PACKAGE, PassiveAnalysis, PassiveEvent, PassiveManifest,
    PassiveRunConfig, PassiveRunFinish, PassiveTargetSlot, SCENARIO_DEATH_RESPAWN,
    SCENARIO_DEVICE_REBIND, SCENARIO_MENU_TEARDOWN, SCENARIO_NATIVE_EXIT, SCENARIO_NATIVE_JOIN,
    SCENARIO_ROOM_REBIND, STATE_BOUND, STATE_CANDIDATE, STATE_RETIRED, STATE_UNBOUND,
    analyze_passive_run, analyze_passive_run_to, passive_gate_to, replay_passive_run,
    validate_passive_capture_options, write_passive_run,
};
use isaac_cs_t0::coop_passive_commands::{
    PassiveFailureStage, PassiveFinalizeInput, passive_finalize_decision,
};
use isaac_cs_t0::coop_passive_ipc::{
    NativePassiveEvent, NativePassiveRecord, PASSIVE_COMPLETE_HOOK_MASK, PASSIVE_FLAG_FAILURE,
    PASSIVE_FLAG_MAIN_THREAD, PASSIVE_NO_ORDINAL, PASSIVE_SLOT_COUNT, PassiveEventKind,
};
use serde::Serialize;
use serde::de::DeserializeOwned;
use serde_json::{Value, json};
use tempfile::TempDir;

#[path = "support/device_v2_fixture.rs"]
mod device_v2_fixture;

fn digest(byte: char) -> String {
    std::iter::repeat_n(byte.to_ascii_uppercase(), 64).collect()
}

fn write_json(path: &Path, value: &impl Serialize) {
    fs::write(
        path,
        serde_json::to_vec_pretty(value).expect("serialize JSON fixture"),
    )
    .expect("write JSON fixture");
}

fn read_json<T: DeserializeOwned>(path: &Path) -> T {
    serde_json::from_slice(&fs::read(path).expect("read JSON fixture")).expect("parse JSON fixture")
}

fn write_events(path: &Path, events: &[PassiveEvent]) {
    let mut bytes = Vec::new();
    for event in events {
        serde_json::to_writer(&mut bytes, event).expect("serialize passive event");
        bytes.push(b'\n');
    }
    fs::write(path, bytes).expect("write passive events");
}

fn target_slots() -> Vec<PassiveTargetSlot> {
    (0..PASSIVE_SLOT_COUNT)
        .map(|ordinal| PassiveTargetSlot {
            ordinal,
            id: format!("slot-{ordinal:02}"),
            patch_kind: if (27..=38).contains(&ordinal) {
                "IMAGE_OR_IMPORT_POINTER"
            } else {
                "REL32_CALL"
            }
            .to_owned(),
            rva: format!("0x{:08X}", 0x0010_0000 + ordinal * 8),
            expected_proof: digest('A'),
            before_proof: digest('A'),
            after_proof: digest('A'),
        })
        .collect()
}

fn event(kind: &str, generation: u64) -> PassiveEvent {
    PassiveEvent {
        schema_version: PASSIVE_SCHEMA_VERSION,
        sequence: 0,
        qpc: 0,
        loop_iteration: 1,
        kind: kind.to_owned(),
        slot_ordinal: None,
        owner_thread_token: digest('D'),
        main_thread: !matches!(
            kind,
            "NATIVE_INITIALIZED" | "HOOKS_INSTALLED" | "HOOKS_RESTORED" | "PRODUCER_CLOSED"
        ),
        phase: 1,
        depth: 1,
        owner_generation: generation,
        local_ordinal: None,
        endpoint: None,
        topology_digest: None,
        player_digest: None,
        device_digest: None,
        assignment_digest: None,
        topology_coherent: false,
        binding_unique: false,
        assignment_matches: false,
        hud_matches: false,
        lua_vector_equivalent: false,
        relation_changed: false,
        stable_registry_retired: false,
        binding_state: None,
        original_calls: 0,
        original_called: false,
    }
}

fn wrapper(kind: &str, generation: u64, slot: u32) -> PassiveEvent {
    let mut value = event(kind, generation);
    value.slot_ordinal = Some(slot);
    if matches!(
        kind,
        "MANAGER_PASS_END"
            | "ORDINARY_SPAWN_END"
            | "EXIT_RETURN_STABLE"
            | "ORDINARY_ASSIGNMENT_END"
            | "JOIN_HUD_END"
            | "EXIT_HUD_END"
    ) {
        value.original_calls = 1;
        value.original_called = true;
    }
    value
}

fn relation(kind: &str, generation: u64, ordinal: u32, topology: char) -> PassiveEvent {
    let mut value = event(kind, generation);
    value.local_ordinal = Some(ordinal);
    value.endpoint = Some(if ordinal == 0 { ENDPOINT_A } else { ENDPOINT_B }.to_owned());
    value.topology_digest = Some(digest(topology));
    value.player_digest = Some(digest(if ordinal == 0 { '1' } else { '2' }));
    value.device_digest = Some(digest(if ordinal == 0 { '3' } else { '4' }));
    value.assignment_digest = Some(digest(if ordinal == 0 { '5' } else { '6' }));
    value.topology_coherent = true;
    value.binding_unique = true;
    value.assignment_matches = true;
    value.hud_matches = true;
    value.lua_vector_equivalent = true;
    value.binding_state = match kind {
        "BINDING_UNBOUND" => Some(STATE_UNBOUND.to_owned()),
        "BINDING_CANDIDATE" => Some(STATE_CANDIDATE.to_owned()),
        "BINDING_BOUND" => Some(STATE_BOUND.to_owned()),
        "BINDING_RETIRED" => Some(STATE_RETIRED.to_owned()),
        _ => None,
    };
    value
}

fn manager_pass(events: &mut Vec<PassiveEvent>, generation: u64) {
    events.push(wrapper("MANAGER_PASS_BEGIN", generation, 45));
    events.push(wrapper("MANAGER_PASS_END", generation, 45));
}

fn bind_both(events: &mut Vec<PassiveEvent>, generation: u64, topology: char) {
    manager_pass(events, generation);
    events.push(relation("BINDING_CANDIDATE", generation, 0, topology));
    events.push(relation("BINDING_CANDIDATE", generation, 1, topology));
    manager_pass(events, generation);
    events.push(relation("BINDING_BOUND", generation, 0, topology));
    events.push(relation("BINDING_BOUND", generation, 1, topology));
}

fn scenario_events(scenario: &str) -> Vec<PassiveEvent> {
    let mut events = vec![event("NATIVE_INITIALIZED", 0), event("HOOKS_INSTALLED", 0)];
    match scenario {
        SCENARIO_NATIVE_JOIN => {
            events.push(wrapper("MANAGER_PASS_BEGIN", 1, 45));
            events.push(wrapper("ORDINARY_SPAWN_BEGIN", 1, 46));
            events.push(wrapper("ORDINARY_SPAWN_END", 1, 46));
            events.push(wrapper("ORDINARY_ASSIGNMENT_BEGIN", 1, 49));
            events.push(wrapper("ORDINARY_ASSIGNMENT_END", 1, 49));
            events.push(wrapper("JOIN_HUD_BEGIN", 1, 51));
            events.push(wrapper("JOIN_HUD_END", 1, 51));
            events.push(wrapper("MANAGER_PASS_END", 1, 45));
            events.push(relation("BINDING_CANDIDATE", 1, 0, 'A'));
            events.push(relation("BINDING_CANDIDATE", 1, 1, 'A'));
            manager_pass(&mut events, 1);
            events.push(relation("BINDING_BOUND", 1, 0, 'A'));
            events.push(relation("BINDING_BOUND", 1, 1, 'A'));
        }
        SCENARIO_NATIVE_EXIT => {
            bind_both(&mut events, 1, 'A');
            events.push(relation("BINDING_UNBOUND", 2, 1, 'B'));
            events.push(wrapper("EXIT_BEGIN", 2, 48));
            events.push(wrapper("EXIT_HUD_BEGIN", 2, 52));
            events.push(wrapper("EXIT_HUD_END", 2, 52));
            events.push(wrapper("EXIT_RETURN_STABLE", 2, 48));
            manager_pass(&mut events, 2);
            events.push(relation("BINDING_CANDIDATE", 2, 0, 'B'));
            manager_pass(&mut events, 2);
            events.push(relation("BINDING_BOUND", 2, 0, 'B'));
        }
        SCENARIO_DEATH_RESPAWN => {
            bind_both(&mut events, 1, 'A');
            events.push(relation("BINDING_UNBOUND", 2, 0, 'B'));
            events.push(relation("BINDING_UNBOUND", 2, 1, 'B'));
            events.push(wrapper("ORDINARY_SPAWN_BEGIN", 2, 46));
            events.push(wrapper("ORDINARY_SPAWN_END", 2, 46));
            bind_both(&mut events, 2, 'B');
        }
        SCENARIO_ROOM_REBIND => {
            bind_both(&mut events, 1, 'A');
            events.push(event("LIFECYCLE_ENTER", 2));
            events.push(relation("BINDING_UNBOUND", 2, 0, 'B'));
            events.push(relation("BINDING_UNBOUND", 2, 1, 'B'));
            events.push(event("LIFECYCLE_EXIT", 2));
            bind_both(&mut events, 2, 'B');
        }
        SCENARIO_DEVICE_REBIND => {
            bind_both(&mut events, 1, 'A');
            events.push(relation("BINDING_UNBOUND", 2, 0, 'B'));
            events.push(relation("BINDING_UNBOUND", 2, 1, 'B'));
            let mut changed = relation("RELATION_CHANGE", 2, 1, 'B');
            changed.relation_changed = true;
            changed.device_digest = Some(digest('9'));
            changed.assignment_digest = Some(digest('8'));
            events.push(changed);
            bind_both(&mut events, 2, 'B');
        }
        SCENARIO_MENU_TEARDOWN => {
            bind_both(&mut events, 1, 'A');
            events.push(event("LIFECYCLE_ENTER", 2));
            for ordinal in 0..=1 {
                let mut retired = relation("BINDING_RETIRED", 2, ordinal, 'B');
                retired.stable_registry_retired = true;
                events.push(retired);
            }
            events.push(event("LIFECYCLE_EXIT", 2));
        }
        _ => panic!("unsupported fixture scenario"),
    }
    let terminal_generation = if scenario == SCENARIO_NATIVE_JOIN {
        1
    } else {
        2
    };
    events.push(event("HOOKS_RESTORED", terminal_generation));
    events.push(event("PRODUCER_CLOSED", terminal_generation));
    for (index, event) in events.iter_mut().enumerate() {
        event.sequence = index as u64;
        event.qpc = 10_000 + index as i64;
        event.loop_iteration = index as u64 + 1;
    }
    events
}

fn materialize_run(parent: &Path, name: &str, scenario: &str, create_analysis: bool) -> PathBuf {
    if scenario == SCENARIO_DEVICE_REBIND {
        let run = device_v2_fixture::create(parent, name, false, true);
        if create_analysis {
            let analysis = analyze_passive_run_to(&run, &run.join("analysis.json")).unwrap();
            assert_eq!(
                analysis.result, "PASS",
                "device v2 fixture: {:?}",
                analysis.checks
            );
        }
        return run;
    }
    let run = parent.join(name);
    fs::create_dir(&run).expect("create passive run fixture");
    let events = scenario_events(scenario);
    write_events(&run.join("events.jsonl"), &events);
    let generation = events
        .iter()
        .map(|event| event.owner_generation)
        .max()
        .unwrap_or(0);
    let manifest = PassiveManifest {
        schema_version: PASSIVE_SCHEMA_VERSION,
        evidence_class: PASSIVE_EVIDENCE_CLASS.to_owned(),
        tool_identity: PASSIVE_TOOL_IDENTITY.to_owned(),
        artifact_identity: format!("PASSIVE-{name}"),
        work_package: PASSIVE_WORK_PACKAGE.to_owned(),
        case_id: name.to_owned(),
        probe: PASSIVE_PROBE.to_owned(),
        scenario: scenario.to_owned(),
        started_utc: "2026-08-28T00:00:00Z".to_owned(),
        ended_utc: "2026-08-28T00:03:00Z".to_owned(),
        state: "PASS".to_owned(),
        failure_reason: None,
        target_sha256: digest('A'),
        observed_target_sha256: digest('B'),
        target_equivalence_sha256: digest('C'),
        layout_evidence_sha256: digest('D'),
        coop_catalog_sha256: digest('E'),
        coop_graph_sha256: digest('F'),
        coop_plan_sha256: digest('1'),
        coop_verification_sha256: digest('2'),
        native_sha256: digest('3'),
        helper_sha256: digest('4'),
        process_identity_token: digest('5'),
        events_sha256: file_sha256(&run.join("events.jsonl")).expect("hash passive events"),
        slot_count: PASSIVE_SLOT_COUNT,
        installed_hooks_mask: format!("0x{PASSIVE_COMPLETE_HOOK_MASK:016X}"),
        restored_hooks_mask: format!("0x{PASSIVE_COMPLETE_HOOK_MASK:016X}"),
        restored_slot_count: PASSIVE_SLOT_COUNT,
        install_order: (0..PASSIVE_SLOT_COUNT).collect(),
        restore_order: (0..PASSIVE_SLOT_COUNT).rev().collect(),
        target_slots: target_slots(),
        slot_snapshot_before_sha256: digest('6'),
        slot_snapshot_after_sha256: digest('6'),
        main_thread_id_token: digest('7'),
        final_owner_generation: generation,
        overwritten_events: 0,
        callbacks_drained: true,
        producer_closed: true,
        unsafe_unload_required: false,
        stable_registry_retired: scenario == SCENARIO_MENU_TEARDOWN,
        override_count: 0,
        apply_count: 0,
        world_write_count: 0,
        scheduler_count: 0,
        transport_count: 0,
        mode_latch_write_count: 0,
        input_mutation_allowed: false,
        world_mutation_allowed: false,
        active_input_allowed: false,
        original_online_positive_allowed: false,
    };
    write_json(&run.join("manifest.json"), &manifest);
    if create_analysis {
        let output = run.join("analysis.json");
        let analysis = analyze_passive_run_to(&run, &output).expect("create passive analysis");
        assert_eq!(
            analysis.result,
            "PASS",
            "positive fixture {scenario} failed checks: {:?}",
            analysis
                .checks
                .iter()
                .filter(|check| !check.passed)
                .collect::<Vec<_>>()
        );
    }
    run
}

fn read_manifest(run: &Path) -> PassiveManifest {
    read_json(&run.join("manifest.json"))
}

fn rewrite_manifest(run: &Path, manifest: &PassiveManifest) {
    write_json(&run.join("manifest.json"), manifest);
}

fn read_event_file(run: &Path) -> Vec<PassiveEvent> {
    fs::read_to_string(run.join("events.jsonl"))
        .expect("read passive events")
        .lines()
        .map(|line| serde_json::from_str(line).expect("parse passive event"))
        .collect()
}

fn rewrite_event_file(run: &Path, events: &[PassiveEvent]) {
    write_events(&run.join("events.jsonl"), events);
    let mut manifest = read_manifest(run);
    manifest.events_sha256 = file_sha256(&run.join("events.jsonl")).expect("rehash events");
    rewrite_manifest(run, &manifest);
}

fn six_runs(root: &Path, create_analysis: bool) -> [PathBuf; 6] {
    fs::create_dir_all(root).expect("create six-source fixture root");
    std::array::from_fn(|index| {
        materialize_run(
            root,
            &format!("source-{index}-{}", PASSIVE_SCENARIOS[index]),
            PASSIVE_SCENARIOS[index],
            create_analysis,
        )
    })
}

fn binary() -> &'static str {
    env!("CARGO_BIN_EXE_IsaacCsT0")
}

#[test]
fn public_capture_scope_accepts_exactly_six_passive_scenarios() {
    for scenario in PASSIVE_SCENARIOS {
        assert!(
            validate_passive_capture_options(
                PASSIVE_PROBE,
                scenario,
                "case-01",
                PASSIVE_DEFAULT_DURATION_MILLISECONDS,
            )
            .is_ok()
        );
    }
    for (probe, scenario, case_id, duration) in [
        ("control-no-native", SCENARIO_NATIVE_JOIN, "case", 180_000),
        (PASSIVE_PROBE, "unknown", "case", 180_000),
        (PASSIVE_PROBE, SCENARIO_NATIVE_JOIN, "../case", 180_000),
        (PASSIVE_PROBE, SCENARIO_NATIVE_JOIN, "case", 9_999),
        (PASSIVE_PROBE, SCENARIO_NATIVE_JOIN, "case", 600_001),
    ] {
        assert!(validate_passive_capture_options(probe, scenario, case_id, duration).is_err());
    }

    let required = [
        "--scenario",
        SCENARIO_NATIVE_JOIN,
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
        "--verification",
        "verification.json",
        "--artifacts-root",
        "artifacts",
        "--case-id",
        "case-01",
    ];
    let mut forbidden = Command::new(binary());
    forbidden.args(["coop-smoke", "--probe", PASSIVE_PROBE]);
    forbidden.args(required);
    forbidden.args(["--arm-token", "forbidden"]);
    let output = forbidden.output().expect("run passive CLI parser");
    assert!(!output.status.success());
}

#[test]
fn six_serialized_sources_analyze_replay_and_gate() {
    let temp = TempDir::new().expect("create passive fixture root");
    let runs = six_runs(temp.path(), true);
    for (index, run) in runs.iter().enumerate() {
        let analysis = analyze_passive_run(run).expect("analyze passive fixture");
        assert_eq!(
            analysis.analysis_identity,
            if index == 4 {
                "IsaacCsT0-CoopPassiveAnalysis-v2"
            } else {
                PASSIVE_ANALYSIS_IDENTITY
            }
        );
        assert_eq!(analysis.scenario, PASSIVE_SCENARIOS[index]);
        assert_eq!(analysis.result, "PASS");
        assert_eq!(replay_passive_run(run).expect("replay source"), analysis);
    }
    let output = temp.path().join("passive-decision.json");
    let refs = runs.each_ref().map(PathBuf::as_path);
    let decision = passive_gate_to(refs, &output).expect("create passive gate");
    assert_eq!(
        decision.decision_identity,
        "IsaacCsT0-CoopPassiveDecision-v2"
    );
    assert_eq!(decision.status, "PASS");
    assert!(decision.cs_t00504_unlocked);
    assert!(!decision.active_input_allowed);
    assert!(!decision.passive_hook_transaction_allowed);
    assert!(!decision.runtime_mutation_allowed);
    assert!(!decision.t005_unlocked);

    let cli_output_path = temp.path().join("passive-decision-cli.json");
    let cli = Command::new(binary())
        .args(["coop-passive-gate", "--native-join-passive"])
        .arg(&runs[0])
        .args(["--native-exit-passive"])
        .arg(&runs[1])
        .args(["--death-respawn-passive"])
        .arg(&runs[2])
        .args(["--room-rebind-passive"])
        .arg(&runs[3])
        .args(["--device-rebind-passive"])
        .arg(&runs[4])
        .args(["--menu-teardown-passive"])
        .arg(&runs[5])
        .args(["--output"])
        .arg(&cli_output_path)
        .output()
        .expect("run coop-passive-gate CLI");
    assert!(
        cli.status.success(),
        "coop-passive-gate stderr: {}",
        String::from_utf8_lossy(&cli.stderr)
    );
    let cli_decision: isaac_cs_t0::coop_passive::PassiveGateDecision = read_json(&cli_output_path);
    assert_eq!(cli_decision.status, "PASS");
}

#[test]
fn coop_analyze_dispatch_and_all_outputs_are_create_new() {
    let temp = TempDir::new().expect("create passive CLI fixture root");
    let run = materialize_run(temp.path(), "cli-source", SCENARIO_NATIVE_JOIN, false);
    let analysis_path = run.join("analysis.json");
    let output = Command::new(binary())
        .args(["coop-analyze"])
        .arg(&run)
        .args(["--output"])
        .arg(&analysis_path)
        .output()
        .expect("run coop-analyze");
    assert!(
        output.status.success(),
        "coop-analyze stderr: {}",
        String::from_utf8_lossy(&output.stderr)
    );
    let first_hash = file_sha256(&analysis_path).expect("hash analysis");
    assert!(analyze_passive_run_to(&run, &analysis_path).is_err());
    assert_eq!(
        file_sha256(&analysis_path).expect("rehash analysis"),
        first_hash
    );

    let runs = six_runs(&temp.path().join("gate-sources"), true);
    let occupied = temp.path().join("occupied-decision.json");
    fs::write(&occupied, b"historical decision").expect("occupy gate output");
    let refs = runs.each_ref().map(PathBuf::as_path);
    assert!(passive_gate_to(refs, &occupied).is_err());
    assert_eq!(
        fs::read(&occupied).expect("read occupied output"),
        b"historical decision"
    );
}

#[test]
fn analyzer_rejects_event_stream_original_once_and_generation_mutations() {
    type EventMutation = (&'static str, Box<dyn Fn(&mut Vec<PassiveEvent>)>);
    let mutations: Vec<EventMutation> = vec![
        ("sequence-gap", Box::new(|events| events[5].sequence += 1)),
        ("qpc-regression", Box::new(|events| events[5].qpc = -1)),
        (
            "original-zero",
            Box::new(|events| {
                let event = events
                    .iter_mut()
                    .find(|event| event.kind == "ORDINARY_SPAWN_END")
                    .expect("spawn end");
                event.original_calls = 0;
            }),
        ),
        (
            "original-twice",
            Box::new(|events| {
                let event = events
                    .iter_mut()
                    .find(|event| event.kind == "ORDINARY_ASSIGNMENT_END")
                    .expect("assignment end");
                event.original_calls = 2;
            }),
        ),
        (
            "wrong-thread",
            Box::new(|events| {
                events
                    .iter_mut()
                    .find(|event| event.kind == "JOIN_HUD_END")
                    .expect("HUD end")
                    .main_thread = false;
            }),
        ),
        (
            "unknown-kind",
            Box::new(|events| events[5].kind = "FUTURE_EVENT".to_owned()),
        ),
        (
            "possessor-stop",
            Box::new(|events| events.insert(3, event("POSSESSOR_SPAWN_STOP", 1))),
        ),
        (
            "third-ordinal",
            Box::new(|events| {
                let event = events
                    .iter_mut()
                    .find(|event| event.kind == "BINDING_CANDIDATE")
                    .expect("candidate");
                event.local_ordinal = Some(2);
            }),
        ),
        (
            "endpoint-swap",
            Box::new(|events| {
                let event = events
                    .iter_mut()
                    .find(|event| event.local_ordinal == Some(0))
                    .expect("ordinal zero");
                event.endpoint = Some(ENDPOINT_B.to_owned());
            }),
        ),
        (
            "generation-regression",
            Box::new(|events| events.last_mut().expect("last event").owner_generation = 0),
        ),
        (
            "one-pass-bind",
            Box::new(|events| {
                let second_begin = events
                    .iter()
                    .rposition(|event| event.kind == "MANAGER_PASS_BEGIN")
                    .expect("second manager begin");
                events.remove(second_begin + 1);
                events.remove(second_begin);
            }),
        ),
        (
            "topology-changed",
            Box::new(|events| {
                events
                    .iter_mut()
                    .find(|event| event.kind == "BINDING_BOUND")
                    .expect("bound")
                    .topology_digest = Some(digest('F'));
            }),
        ),
        (
            "non-unique-candidate",
            Box::new(|events| {
                events
                    .iter_mut()
                    .find(|event| event.kind == "BINDING_CANDIDATE")
                    .expect("candidate")
                    .binding_unique = false;
            }),
        ),
        (
            "hud-mismatch",
            Box::new(|events| {
                events
                    .iter_mut()
                    .find(|event| event.kind == "BINDING_CANDIDATE")
                    .expect("candidate")
                    .hud_matches = false;
            }),
        ),
        (
            "lua-mismatch",
            Box::new(|events| {
                events
                    .iter_mut()
                    .find(|event| event.kind == "BINDING_CANDIDATE")
                    .expect("candidate")
                    .lua_vector_equivalent = false;
            }),
        ),
    ];
    let temp = TempDir::new().expect("create mutation root");
    for (name, mutate) in mutations {
        let run = materialize_run(temp.path(), name, SCENARIO_NATIVE_JOIN, false);
        let mut events = read_event_file(&run);
        mutate(&mut events);
        for (index, event) in events.iter_mut().enumerate() {
            if name != "sequence-gap" {
                event.sequence = index as u64;
            }
        }
        rewrite_event_file(&run, &events);
        let analysis = analyze_passive_run(&run).expect("mutation remains replayable");
        assert_eq!(analysis.result, "BLOCKED_BY_EVIDENCE", "accepted {name}");
    }
}

#[test]
fn analyzer_rejects_restore_capability_privacy_and_unknown_field_mutations() {
    type ManifestMutation = (&'static str, Box<dyn Fn(&mut PassiveManifest)>);
    let mutations: Vec<ManifestMutation> = vec![
        (
            "event-hash",
            Box::new(|manifest| manifest.events_sha256 = digest('0')),
        ),
        ("slot-count", Box::new(|manifest| manifest.slot_count = 52)),
        (
            "installed-mask",
            Box::new(|manifest| manifest.installed_hooks_mask = "0x000FFFFFFFFFFFFF".to_owned()),
        ),
        (
            "restored-mask",
            Box::new(|manifest| manifest.restored_hooks_mask = "0x000FFFFFFFFFFFFF".to_owned()),
        ),
        (
            "restore-count",
            Box::new(|manifest| manifest.restored_slot_count = 52),
        ),
        (
            "install-order",
            Box::new(|manifest| manifest.install_order.swap(51, 52)),
        ),
        (
            "restore-order",
            Box::new(|manifest| manifest.restore_order.swap(0, 1)),
        ),
        (
            "restored-byte",
            Box::new(|manifest| manifest.target_slots[52].after_proof = digest('9')),
        ),
        (
            "snapshot",
            Box::new(|manifest| manifest.slot_snapshot_after_sha256 = digest('9')),
        ),
        (
            "overwrite",
            Box::new(|manifest| manifest.overwritten_events = 1),
        ),
        (
            "callbacks",
            Box::new(|manifest| manifest.callbacks_drained = false),
        ),
        (
            "producer",
            Box::new(|manifest| manifest.producer_closed = false),
        ),
        (
            "unsafe",
            Box::new(|manifest| manifest.unsafe_unload_required = true),
        ),
        ("override", Box::new(|manifest| manifest.override_count = 1)),
        ("apply", Box::new(|manifest| manifest.apply_count = 1)),
        (
            "world-write",
            Box::new(|manifest| manifest.world_write_count = 1),
        ),
        (
            "scheduler",
            Box::new(|manifest| manifest.scheduler_count = 1),
        ),
        (
            "transport",
            Box::new(|manifest| manifest.transport_count = 1),
        ),
        (
            "mode-latch",
            Box::new(|manifest| manifest.mode_latch_write_count = 1),
        ),
        (
            "active-permission",
            Box::new(|manifest| manifest.active_input_allowed = true),
        ),
        (
            "original-online",
            Box::new(|manifest| manifest.original_online_positive_allowed = true),
        ),
        (
            "absolute-path",
            Box::new(|manifest| manifest.artifact_identity = r"C:\Users\private\source".to_owned()),
        ),
        (
            "raw-address",
            Box::new(|manifest| manifest.artifact_identity = "0x12345678".to_owned()),
        ),
        (
            "malformed-target-hash",
            Box::new(|manifest| manifest.target_sha256 = "not-a-sha256".to_owned()),
        ),
    ];
    let temp = TempDir::new().expect("create manifest mutation root");
    for (name, mutate) in mutations {
        let run = materialize_run(temp.path(), name, SCENARIO_NATIVE_JOIN, false);
        let mut manifest = read_manifest(&run);
        mutate(&mut manifest);
        rewrite_manifest(&run, &manifest);
        let analysis = analyze_passive_run(&run).expect("manifest mutation remains replayable");
        assert_eq!(analysis.result, "BLOCKED_BY_EVIDENCE", "accepted {name}");
    }

    let unknown = materialize_run(temp.path(), "unknown-field", SCENARIO_NATIVE_JOIN, false);
    let manifest_path = unknown.join("manifest.json");
    let mut value: Value = read_json(&manifest_path);
    value["futureCapability"] = json!(true);
    write_json(&manifest_path, &value);
    assert!(analyze_passive_run(&unknown).is_err());
}

#[test]
fn scenario_contracts_reject_missing_endpoint_and_lifecycle_evidence() {
    let temp = TempDir::new().expect("create scenario mutation root");
    for (scenario, remove_kind) in [
        (SCENARIO_NATIVE_JOIN, "ORDINARY_ASSIGNMENT_END"),
        (SCENARIO_NATIVE_EXIT, "EXIT_RETURN_STABLE"),
        (SCENARIO_DEATH_RESPAWN, "ORDINARY_SPAWN_BEGIN"),
        (SCENARIO_ROOM_REBIND, "LIFECYCLE_EXIT"),
        (SCENARIO_DEVICE_REBIND, "RELATION_CHANGE"),
        (SCENARIO_MENU_TEARDOWN, "BINDING_RETIRED"),
    ] {
        let run = materialize_run(temp.path(), scenario, scenario, false);
        let mut events = read_event_file(&run);
        events.retain(|event| event.kind != remove_kind);
        for (index, event) in events.iter_mut().enumerate() {
            event.sequence = index as u64;
        }
        rewrite_event_file(&run, &events);
        assert_eq!(
            analyze_passive_run(&run)
                .expect("analyze missing scenario evidence")
                .result,
            "BLOCKED_BY_EVIDENCE",
            "accepted missing {remove_kind} in {scenario}"
        );
    }

    for (scenario, name, missing_ordinal) in [
        (SCENARIO_NATIVE_JOIN, "join-missing-b", 1),
        (SCENARIO_NATIVE_EXIT, "exit-missing-a", 0),
        (SCENARIO_DEATH_RESPAWN, "respawn-missing-b", 1),
        (SCENARIO_ROOM_REBIND, "room-missing-b", 1),
        (SCENARIO_DEVICE_REBIND, "device-missing-b", 1),
        (SCENARIO_MENU_TEARDOWN, "teardown-missing-b", 1),
    ] {
        let run = materialize_run(temp.path(), name, scenario, false);
        let mut events = read_event_file(&run);
        let final_generation = events
            .iter()
            .map(|event| event.owner_generation)
            .max()
            .expect("final generation");
        events.retain(|event| {
            !(event.local_ordinal == Some(missing_ordinal)
                && event.owner_generation == final_generation
                && matches!(
                    event.kind.as_str(),
                    "BINDING_BOUND" | "BINDING_RETIRED" | "RELATION_CHANGE"
                ))
        });
        for (index, event) in events.iter_mut().enumerate() {
            event.sequence = index as u64;
        }
        rewrite_event_file(&run, &events);
        assert_eq!(
            analyze_passive_run(&run)
                .expect("analyze missing endpoint evidence")
                .result,
            "BLOCKED_BY_EVIDENCE",
            "accepted {name}"
        );
    }
}

#[test]
fn gate_replays_hash_bound_distinct_sources_and_rejects_swaps_or_drift() {
    let temp = TempDir::new().expect("create passive gate mutation root");
    let runs = six_runs(temp.path(), true);

    let mut swapped = runs.clone();
    swapped.swap(0, 1);
    let swapped_output = temp.path().join("swapped.json");
    let refs = swapped.each_ref().map(PathBuf::as_path);
    let decision = passive_gate_to(refs, &swapped_output).expect("swapped gate is replayable");
    assert_eq!(decision.status, "BLOCKED_BY_EVIDENCE");
    assert!(!decision.cs_t00504_unlocked);

    let mut duplicate = runs.clone();
    duplicate[5] = duplicate[0].clone();
    let duplicate_output = temp.path().join("duplicate.json");
    let refs = duplicate.each_ref().map(PathBuf::as_path);
    let decision = passive_gate_to(refs, &duplicate_output).expect("duplicate gate is replayable");
    assert_eq!(decision.status, "BLOCKED_BY_EVIDENCE");

    let stored_path = runs[2].join("analysis.json");
    let mut stored: PassiveAnalysis = read_json(&stored_path);
    stored.result = "BLOCKED_BY_EVIDENCE".to_owned();
    write_json(&stored_path, &stored);
    let refs = runs.each_ref().map(PathBuf::as_path);
    assert!(passive_gate_to(refs, &temp.path().join("stored-tamper.json")).is_err());

    let second = TempDir::new().expect("create binding drift root");
    let drifted = six_runs(second.path(), true);
    let mut manifest = read_manifest(&drifted[4]);
    manifest.coop_catalog_sha256 = digest('9');
    rewrite_manifest(&drifted[4], &manifest);
    let replacement = analyze_passive_run(&drifted[4]).expect("reanalyze drifted binding");
    write_json(&drifted[4].join("analysis.json"), &replacement);
    let refs = drifted.each_ref().map(PathBuf::as_path);
    let decision = passive_gate_to(refs, &second.path().join("binding-drift.json"))
        .expect("binding drift gate is replayable");
    assert_eq!(decision.status, "BLOCKED_BY_EVIDENCE");
}

fn gate_cli(runs: &[PathBuf; 6], output: &Path) -> Value {
    assert!(!output.exists());
    let mut command = Command::new(binary());
    command.arg("coop-passive-gate");
    for (scenario, run) in PASSIVE_SCENARIOS.iter().zip(runs) {
        command.arg(format!("--{scenario}")).arg(run);
    }
    let status = command.arg("--output").arg(output).output().unwrap();
    assert!(
        output.exists(),
        "gate did not create decision: {}",
        String::from_utf8_lossy(&status.stderr)
    );
    read_json(output)
}

#[test]
fn mixed_gate_exempts_only_exact_historical_triples_and_never_foundation_drift() {
    let repository = Path::new(env!("CARGO_MANIFEST_DIR")).parent().unwrap();
    let historical = [
        (
            0,
            "20260828T072118Z-t0-05-03-native-join-passive-t0-05-03-native-join-passive-live-06",
        ),
        (
            2,
            "20260830T182858Z-t0-05-03-death-respawn-passive-t0-05-03-death-respawn-passive-live-08",
        ),
        (
            3,
            "20260830T185508Z-t0-05-03-room-rebind-passive-t0-05-03-room-rebind-passive-live-09",
        ),
    ];
    let root = TempDir::new().unwrap();
    let mut runs = six_runs(&root.path().join("new-sources"), false);
    for (index, name) in historical {
        runs[index] = repository.join("artifacts/cs-t0").join(name);
    }
    let historical_base: Value = read_json(&runs[0].join("manifest.json"));
    let foundation = [
        "targetSha256",
        "observedTargetSha256",
        "targetEquivalenceSha256",
        "layoutEvidenceSha256",
        "coopGraphSha256",
    ];
    for index in [1, 4, 5] {
        let mut manifest: Value = read_json(&runs[index].join("manifest.json"));
        for field in foundation {
            manifest[field] = historical_base[field].clone();
        }
        write_json(&runs[index].join("manifest.json"), &manifest);
        let analysis =
            analyze_passive_run_to(&runs[index], &runs[index].join("analysis.json")).unwrap();
        assert_eq!(analysis.result, "PASS");
    }
    assert_eq!(
        gate_cli(&runs, &root.path().join("approved-mixed.json"))["status"],
        "PASS"
    );
    for field in [
        "nativeSha256",
        "helperSha256",
        "coopCatalogSha256",
        "coopPlanSha256",
        "coopVerificationSha256",
    ] {
        let saved_manifest = fs::read(runs[4].join("manifest.json")).unwrap();
        let saved_analysis = fs::read(runs[4].join("analysis.json")).unwrap();
        let mut manifest: Value = serde_json::from_slice(&saved_manifest).unwrap();
        manifest[field] = json!(digest('9'));
        write_json(&runs[4].join("manifest.json"), &manifest);
        write_json(
            &runs[4].join("analysis.json"),
            &analyze_passive_run(&runs[4]).unwrap(),
        );
        assert_eq!(
            gate_cli(&runs, &root.path().join(format!("new-{field}.json")))["status"],
            "BLOCKED_BY_EVIDENCE",
            "new source exemption expanded to {field}"
        );
        fs::write(runs[4].join("manifest.json"), saved_manifest).unwrap();
        fs::write(runs[4].join("analysis.json"), saved_analysis).unwrap();
    }
    for field in foundation {
        // All current-producer sources agree with each other. The unchanged,
        // approved historical source must still veto foundation inequality.
        let saved: Vec<_> = [1, 4, 5]
            .into_iter()
            .map(|i| {
                (
                    i,
                    fs::read(runs[i].join("manifest.json")).unwrap(),
                    fs::read(runs[i].join("analysis.json")).unwrap(),
                )
            })
            .collect();
        for (i, manifest_bytes, _) in &saved {
            let mut manifest: Value = serde_json::from_slice(manifest_bytes).unwrap();
            manifest[field] = json!(digest('9'));
            write_json(&runs[*i].join("manifest.json"), &manifest);
            write_json(
                &runs[*i].join("analysis.json"),
                &analyze_passive_run(&runs[*i]).unwrap(),
            );
        }
        assert_eq!(
            gate_cli(&runs, &root.path().join(format!("base-{field}.json")))["status"],
            "BLOCKED_BY_EVIDENCE"
        );
        for (i, manifest_bytes, analysis_bytes) in saved {
            fs::write(runs[i].join("manifest.json"), manifest_bytes).unwrap();
            fs::write(runs[i].join("analysis.json"), analysis_bytes).unwrap();
        }
    }
    for file in ["manifest.json", "events.jsonl", "analysis.json"] {
        let copy = root
            .path()
            .join(format!("changed-history-{}", file.replace('.', "-")));
        fs::create_dir(&copy).unwrap();
        for name in ["manifest.json", "events.jsonl", "analysis.json"] {
            fs::copy(runs[0].join(name), copy.join(name)).unwrap();
        }
        let mut bytes = fs::read(copy.join(file)).unwrap();
        if file == "events.jsonl" {
            // Leading JSON whitespace is semantically neutral but changes the
            // exact events hash and thus must lose historical exemption.
            bytes.insert(0, b' ');
        } else {
            bytes.push(b' ');
        }
        fs::write(copy.join(file), bytes).unwrap();
        if file != "analysis.json" {
            if file == "events.jsonl" {
                let mut manifest: Value = read_json(&copy.join("manifest.json"));
                manifest["eventsSha256"] = json!(file_sha256(&copy.join("events.jsonl")).unwrap());
                write_json(&copy.join("manifest.json"), &manifest);
            }
            write_json(
                &copy.join("analysis.json"),
                &analyze_passive_run(&copy).unwrap(),
            );
        }
        let mut changed = runs.clone();
        changed[0] = copy;
        assert_eq!(
            gate_cli(&changed, &root.path().join(format!("no-exemption-{file}")))["status"],
            "BLOCKED_BY_EVIDENCE",
            "non-exact {file} obtained historical exemption"
        );
    }
}

fn native_failure_record(
    sequence: u64,
    kind: PassiveEventKind,
    slot_ordinal: u32,
    phase: u32,
) -> NativePassiveRecord {
    NativePassiveRecord {
        sequence,
        event: NativePassiveEvent {
            published_sequence: sequence as i64 + 1,
            qpc: sequence as i64 + 1,
            loop_iteration: sequence + 1,
            owner_generation: 1,
            topology_digest: [0; 32],
            player_digest: [0; 32],
            device_digest: [0; 32],
            assignment_digest: [0; 32],
            thread_id: 7,
            main_thread_id: 7,
            kind: kind as u32,
            slot_ordinal,
            phase,
            depth: 0,
            local_ordinal: PASSIVE_NO_ORDINAL,
            endpoint: 0,
            original_calls: 0,
            flags: PASSIVE_FLAG_MAIN_THREAD
                | if kind == PassiveEventKind::Failure {
                    PASSIVE_FLAG_FAILURE
                } else {
                    0
                },
        },
    }
}

fn failed_run_config() -> PassiveRunConfig {
    PassiveRunConfig {
        case_id: "topology-ambiguous-live-01".to_owned(),
        scenario: SCENARIO_NATIVE_JOIN.to_owned(),
        started_utc: "2026-08-28T00:00:00Z".to_owned(),
        target_sha256: digest('1'),
        observed_target_sha256: digest('2'),
        target_equivalence_sha256: digest('3'),
        layout_evidence_sha256: digest('4'),
        coop_catalog_sha256: digest('5'),
        coop_graph_sha256: digest('6'),
        coop_plan_sha256: digest('7'),
        coop_verification_sha256: digest('8'),
        native_sha256: digest('9'),
        helper_sha256: digest('A'),
        process_identity_token: digest('B'),
        main_thread_id_token: digest('C'),
        target_slots: target_slots(),
    }
}

fn safely_restored_failure() -> PassiveRunFinish {
    PassiveRunFinish {
        state: "FAILED".to_owned(),
        failure_reason: Some("TOPOLOGY_AMBIGUOUS:ASSIGNMENT_NOT_UNIQUE".to_owned()),
        installed_hooks_mask: PASSIVE_COMPLETE_HOOK_MASK,
        restored_hooks_mask: PASSIVE_COMPLETE_HOOK_MASK,
        restored_slot_count: PASSIVE_SLOT_COUNT,
        slot_snapshot_before_sha256: digest('D'),
        slot_snapshot_after_sha256: digest('D'),
        final_owner_generation: 1,
        overwritten_events: 0,
        callbacks_drained: true,
        producer_closed: true,
        unsafe_unload_required: false,
        stable_registry_retired: false,
        zero_capability_counts: [0; 6],
    }
}

#[test]
fn failed_observation_is_create_new_replayable_and_safely_restored() {
    let temp = TempDir::new().expect("create failed passive source root");
    let source = temp.path().join("failed-source");
    let records = [
        native_failure_record(0, PassiveEventKind::NativeInitialized, u32::MAX, 0),
        native_failure_record(1, PassiveEventKind::HooksInstalled, u32::MAX, 0),
        native_failure_record(2, PassiveEventKind::Failure, 45, 9),
        native_failure_record(3, PassiveEventKind::HooksRestored, u32::MAX, 0),
        native_failure_record(4, PassiveEventKind::ProducerClosed, u32::MAX, 0),
    ];
    let analysis = write_passive_run(
        &source,
        failed_run_config(),
        &records,
        safely_restored_failure(),
    )
    .expect("persist failed observation source");
    assert_eq!(analysis.result, "BLOCKED_BY_EVIDENCE");
    assert_eq!(
        replay_passive_run(&source)
            .expect("replay failed observation source")
            .result,
        "BLOCKED_BY_EVIDENCE"
    );

    let manifest: PassiveManifest = read_json(&source.join("manifest.json"));
    assert_eq!(manifest.state, "FAILED");
    assert_eq!(
        manifest.failure_reason.as_deref(),
        Some("TOPOLOGY_AMBIGUOUS:ASSIGNMENT_NOT_UNIQUE")
    );
    assert_eq!(manifest.installed_hooks_mask, "0x001FFFFFFFFFFFFF");
    assert_eq!(manifest.restored_hooks_mask, "0x001FFFFFFFFFFFFF");
    assert_eq!(manifest.restored_slot_count, PASSIVE_SLOT_COUNT);
    assert!(manifest.callbacks_drained);
    assert!(manifest.producer_closed);
    assert!(!manifest.unsafe_unload_required);
    let events = read_event_file(&source);
    assert_eq!(events[2].kind, "FAILURE");
    assert_eq!(events[2].slot_ordinal, Some(45));
    assert_eq!(events[2].phase, 9);
    assert_eq!(events[3].kind, "HOOKS_RESTORED");
    assert_eq!(events[4].kind, "PRODUCER_CLOSED");

    let before = ["events.jsonl", "manifest.json", "analysis.json"]
        .map(|name| file_sha256(&source.join(name)).expect("hash failed source"));
    assert!(
        write_passive_run(
            &source,
            failed_run_config(),
            &records,
            safely_restored_failure(),
        )
        .is_err()
    );
    let after = ["events.jsonl", "manifest.json", "analysis.json"]
        .map(|name| file_sha256(&source.join(name)).expect("rehash failed source"));
    assert_eq!(before, after);
}

#[test]
fn finalize_policy_persists_every_terminal_path_and_never_unloads_unsafe() {
    let decide = |inject_failed,
                  native_unsafe,
                  capture_failed,
                  unload_failed,
                  wait_failed,
                  process_identity_changed| {
        passive_finalize_decision(PassiveFinalizeInput {
            inject_failed,
            native_unsafe,
            capture_failed,
            unload_failed,
            wait_failed,
            process_identity_changed,
        })
    };

    let pass = decide(false, false, false, false, false, false);
    assert!(pass.should_unload && pass.should_wait_stopped && pass.should_persist);
    assert!(!pass.unsafe_unload_required);
    assert_eq!(pass.failure_stage, None);

    let capture = decide(false, false, true, false, false, false);
    assert!(capture.should_unload && capture.should_wait_stopped && capture.should_persist);
    assert!(!capture.unsafe_unload_required);
    assert_eq!(capture.failure_stage, Some(PassiveFailureStage::Capture));

    let unsafe_native = decide(false, true, true, false, false, false);
    assert!(!unsafe_native.should_unload && !unsafe_native.should_wait_stopped);
    assert!(unsafe_native.should_persist && unsafe_native.unsafe_unload_required);
    assert_eq!(
        unsafe_native.failure_stage,
        Some(PassiveFailureStage::Unsafe)
    );

    let unload = decide(false, false, false, true, false, false);
    assert!(unload.should_unload && !unload.should_wait_stopped && unload.should_persist);
    assert!(unload.unsafe_unload_required);
    assert_eq!(unload.failure_stage, Some(PassiveFailureStage::Unload));

    let wait = decide(false, false, false, false, true, false);
    assert!(wait.should_unload && wait.should_wait_stopped && wait.should_persist);
    assert!(wait.unsafe_unload_required);
    assert_eq!(wait.failure_stage, Some(PassiveFailureStage::WaitStopped));

    let inject = decide(true, false, false, false, false, false);
    assert!(!inject.should_unload && !inject.should_wait_stopped && inject.should_persist);
    assert!(!inject.unsafe_unload_required);
    assert_eq!(inject.failure_stage, Some(PassiveFailureStage::Inject));

    let identity = decide(false, false, false, false, false, true);
    assert!(identity.should_unload && identity.should_wait_stopped && identity.should_persist);
    assert_eq!(
        identity.failure_stage,
        Some(PassiveFailureStage::ProcessIdentity)
    );

    let precedence = decide(false, false, true, true, true, true);
    assert_eq!(precedence.failure_stage, Some(PassiveFailureStage::Capture));
    assert!(precedence.unsafe_unload_required && precedence.should_persist);
}

#[test]
fn live04_nonzero_backend_source_replays_immutable_and_remains_blocked() {
    let repository = Path::new(env!("CARGO_MANIFEST_DIR"))
        .parent()
        .expect("cs-t0 has a repository parent");
    let source = repository.join(
        "artifacts/cs-t0/20260827T204247Z-t0-05-03-native-join-passive-t0-05-03-native-join-passive-live-04",
    );
    let frozen_replay = repository.join(
        "artifacts/cs-t0/20260827T204247Z-t0-05-03-native-join-passive-live-04-replay-01.json",
    );
    let before = ["events.jsonl", "manifest.json", "analysis.json"]
        .map(|name| file_sha256(&source.join(name)).expect("hash live-04 source"));

    assert_eq!(
        before,
        [
            "22BF8F43D698C6912B53BCB4016D82D7145045E672429194315198A4974A3AF0",
            "9DBB82E822BCF91A175B8CE836835F44984ABF532BC1C6A64076BD1784B00B97",
            "2E16F4E0DCDB0AE3040D2332DA01097C044EF9E97E3067DA416188D1C02B4F1F",
        ]
    );
    let replay = replay_passive_run(&source).expect("replay immutable live-04 source");
    let stored: PassiveAnalysis = read_json(&source.join("analysis.json"));
    let frozen: PassiveAnalysis = read_json(&frozen_replay);
    assert_eq!(replay, stored);
    assert_eq!(replay, frozen);
    assert_eq!(replay.result, "BLOCKED_BY_EVIDENCE");
    assert_eq!(replay.event_count, 16_368);
    assert_eq!(replay.final_owner_generation, 3);
    assert_eq!(
        replay.artifact_identity,
        "coop-passive-9899271CC057C7623CC62F90C7ABFA48717B92E5F2A957A867F554F756C517D5"
    );
    assert!(replay.checks.iter().any(|check| {
        check.id == "generation-ordinal-binding"
            && !check.passed
            && check.detail.contains("cardinality is {}")
    }));
    assert!(
        replay
            .checks
            .iter()
            .any(|check| check.id == "scenario-contract" && !check.passed)
    );

    let manifest: PassiveManifest = read_json(&source.join("manifest.json"));
    assert_eq!(manifest.state, "PASS");
    assert_eq!(manifest.final_owner_generation, 3);
    assert_eq!(manifest.installed_hooks_mask, "0x001FFFFFFFFFFFFF");
    assert_eq!(manifest.restored_hooks_mask, "0x001FFFFFFFFFFFFF");
    assert_eq!(manifest.restored_slot_count, 53);
    assert_eq!(manifest.install_order, (0..53).collect::<Vec<_>>());
    assert_eq!(manifest.restore_order, (0..53).rev().collect::<Vec<_>>());
    assert_eq!(
        manifest.slot_snapshot_before_sha256,
        manifest.slot_snapshot_after_sha256
    );
    assert_eq!(manifest.overwritten_events, 0);
    assert!(manifest.callbacks_drained && manifest.producer_closed);
    assert!(!manifest.unsafe_unload_required);

    let events = read_event_file(&source);
    let manager_end = events
        .iter()
        .filter(|event| event.kind == "MANAGER_PASS_END")
        .collect::<Vec<_>>();
    assert_eq!(manager_end.len(), 8_177);
    assert!(manager_end.iter().all(|event| {
        event.topology_digest.as_deref()
            == Some("070AC84FE78800A813F68B14CD53837B09982983191DC61A6C914C8C4BF78760")
            && event.original_calls == 1
            && event.original_called
    }));
    let kinds = events
        .iter()
        .map(|event| event.kind.as_str())
        .collect::<Vec<_>>();
    let required = [
        "MANAGER_PASS_BEGIN",
        "ORDINARY_SPAWN_BEGIN",
        "ORDINARY_SPAWN_END",
        "ORDINARY_ASSIGNMENT_BEGIN",
        "ORDINARY_ASSIGNMENT_END",
        "JOIN_HUD_BEGIN",
        "JOIN_HUD_END",
        "MANAGER_PASS_END",
    ];
    let mut next = 0;
    for kind in kinds {
        if next < required.len() && kind == required[next] {
            next += 1;
        }
    }
    assert_eq!(next, required.len());
    assert!(!events.iter().any(|event| matches!(
        event.kind.as_str(),
        "BINDING_CANDIDATE" | "BINDING_BOUND" | "FAILURE"
    )));

    let after = ["events.jsonl", "manifest.json", "analysis.json"]
        .map(|name| file_sha256(&source.join(name)).expect("rehash live-04 source"));
    assert_eq!(before, after);
}
