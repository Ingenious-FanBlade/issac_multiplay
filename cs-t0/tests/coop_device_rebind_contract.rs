//! Independent public-CLI contracts; fixtures never ask production code whether they are valid.
#![recursion_limit = "256"]
use std::fs;
use std::path::Path;
use std::process::Command;

use serde_json::{Value, json};
use tempfile::TempDir;

#[path = "support/device_v2_fixture.rs"]
mod specimen;

fn analyze(source: &Path, output: &Path) -> Option<Value> {
    assert!(!output.exists(), "test must use create-new output");
    let result = Command::new(env!("CARGO_BIN_EXE_IsaacCsT0"))
        .arg("coop-analyze")
        .arg(source)
        .arg("--output")
        .arg(output)
        .output()
        .expect("execute public coop-analyze");
    if output.exists() {
        Some(serde_json::from_slice(&fs::read(output).unwrap()).expect("CLI analysis JSON"))
    } else {
        assert!(
            !result.status.success(),
            "CLI succeeded without its required output"
        );
        None
    }
}

fn rejected(source: &Path, output: &Path, name: &str) {
    let before = ["manifest.json", "events.jsonl"].map(|f| specimen::hash(&source.join(f)));
    let result = analyze(source, output);
    assert!(
        result
            .as_ref()
            .is_none_or(|a| a["result"] == "BLOCKED_BY_EVIDENCE"),
        "accepted mutation {name}: {result:?}"
    );
    let after = ["manifest.json", "events.jsonl"].map(|f| specimen::hash(&source.join(f)));
    assert_eq!(before, after, "analyze modified its input: {name}");
}

fn index(events: &[Value], kind: &str) -> usize {
    events.iter().position(|e| e["kind"] == kind).unwrap()
}

fn last_index(events: &[Value], kind: &str) -> usize {
    events.iter().rposition(|e| e["kind"] == kind).unwrap()
}

#[test]
fn menu_disconnect_and_same_device_have_exact_v2_identity_and_read_only_replay() {
    let root = TempDir::new().unwrap();
    for (name, menu, changed) in [
        ("menu", true, true),
        ("disconnect", false, true),
        ("same-device", false, false),
    ] {
        let source = specimen::create(root.path(), name, menu, changed);
        let output = source.join("analysis.json");
        let analysis = analyze(&source, &output).expect("positive v2 analysis");
        assert_eq!(analysis["result"], "PASS", "{name}: {analysis}");
        assert_eq!(analysis["schemaVersion"], 2);
        assert_eq!(
            analysis["analysisIdentity"],
            "IsaacCsT0-CoopPassiveAnalysis-v2"
        );
        assert_eq!(analysis["scenario"], "device-rebind-passive");
        let original = fs::read(&output).unwrap();
        let snapshots = ["manifest.json", "events.jsonl", "analysis.json"]
            .map(|f| specimen::hash(&source.join(f)));
        let replay = root.path().join(format!("{name}-replay.json"));
        assert_eq!(analyze(&source, &replay).unwrap(), analysis);
        assert_eq!(fs::read(replay).unwrap(), original, "byte-for-byte replay");
        let collision = Command::new(env!("CARGO_BIN_EXE_IsaacCsT0"))
            .arg("coop-analyze")
            .arg(&source)
            .arg("--output")
            .arg(&output)
            .output()
            .unwrap();
        assert!(!collision.status.success());
        assert_eq!(
            snapshots,
            ["manifest.json", "events.jsonl", "analysis.json"]
                .map(|f| specimen::hash(&source.join(f)))
        );
    }
}

#[test]
fn rejects_causal_boundary_identity_topology_generation_and_order_mutations() {
    type Mutation = (&'static str, Box<dyn Fn(&mut Vec<Value>)>);
    let mut cases: Vec<Mutation> = Vec::new();
    for kind in [
        "DEVICE_UNASSIGN_BEGIN",
        "DEVICE_UNASSIGN_END",
        "DEVICE_REASSIGN_BEGIN",
        "DEVICE_REASSIGN_END",
        "RELATION_CHANGE",
    ] {
        cases.push((
            kind,
            Box::new(move |events| {
                let i = index(events, kind);
                events.remove(i);
            }),
        ));
    }
    cases.extend([
        (
            "old-counterexample-no-native-witness",
            Box::new(|e: &mut Vec<Value>| {
                e.retain(|x| !x["kind"].as_str().unwrap().starts_with("DEVICE_"))
            }) as Box<dyn Fn(&mut Vec<Value>)>,
        ),
        (
            "swapped-unassign-return",
            Box::new(|e| {
                let i = index(e, "DEVICE_UNASSIGN_BEGIN");
                e.swap(i, i + 1);
            }),
        ),
        (
            "nested-reassign-before-unassign-return",
            Box::new(|e| {
                let a = index(e, "DEVICE_UNASSIGN_END");
                let b = index(e, "DEVICE_REASSIGN_BEGIN");
                e.swap(a, b);
            }),
        ),
        (
            "duplicate-native-transaction",
            Box::new(|e| {
                let a = index(e, "DEVICE_UNASSIGN_BEGIN");
                let b = index(e, "DEVICE_REASSIGN_END");
                let copy = e[a..=b].to_vec();
                e.splice(b + 1..b + 1, copy);
            }),
        ),
        (
            "duplicate-menu-idempotent-unassign",
            Box::new(|e| {
                let a = index(e, "DEVICE_UNASSIGN_BEGIN") + 2;
                let copy = e[a..a + 2].to_vec();
                e.splice(a..a, copy);
            }),
        ),
        (
            "menu-without-idempotent-unassign",
            Box::new(|e| {
                let a = index(e, "DEVICE_UNASSIGN_BEGIN") + 2;
                e.drain(a..a + 2);
            }),
        ),
        (
            "unknown-callsite",
            Box::new(|e| {
                let i = index(e, "DEVICE_UNASSIGN_BEGIN");
                e[i]["slotOrdinal"] = json!(56);
            }),
        ),
        (
            "ordinary-assignment-is-not-device-cause",
            Box::new(|e| {
                let i = index(e, "DEVICE_UNASSIGN_BEGIN");
                e[i]["kind"] = json!("ORDINARY_ASSIGNMENT_BEGIN");
                e[i]["slotOrdinal"] = json!(49);
            }),
        ),
        (
            "wrong-thread",
            Box::new(|e| {
                let i = index(e, "DEVICE_REASSIGN_END");
                e[i]["mainThread"] = json!(false);
            }),
        ),
        (
            "wrong-depth",
            Box::new(|e| {
                let i = index(e, "DEVICE_REASSIGN_END");
                e[i]["depth"] = json!(1);
            }),
        ),
        (
            "wrong-phase",
            Box::new(|e| {
                let i = index(e, "DEVICE_REASSIGN_END");
                e[i]["phase"] = json!(9);
            }),
        ),
        (
            "cross-player",
            Box::new(|e| {
                let i = index(e, "DEVICE_REASSIGN_END");
                e[i]["playerDigest"] = json!(specimen::digest('1'));
            }),
        ),
        (
            "cross-ordinal",
            Box::new(|e| {
                let i = index(e, "DEVICE_REASSIGN_END");
                e[i]["localOrdinal"] = json!(0);
                e[i]["endpoint"] = json!("ENDPOINT_A");
            }),
        ),
        (
            "third-ordinal",
            Box::new(|e| {
                let i = index(e, "DEVICE_REASSIGN_END");
                e[i]["localOrdinal"] = json!(2);
            }),
        ),
        (
            "endpoint-swap",
            Box::new(|e| {
                let i = index(e, "DEVICE_REASSIGN_END");
                e[i]["endpoint"] = json!("ENDPOINT_A");
            }),
        ),
        (
            "original-zero",
            Box::new(|e| {
                let i = index(e, "DEVICE_REASSIGN_END");
                e[i]["originalCalls"] = json!(0);
            }),
        ),
        (
            "original-twice",
            Box::new(|e| {
                let i = index(e, "DEVICE_REASSIGN_END");
                e[i]["originalCalls"] = json!(2);
            }),
        ),
        (
            "original-at-entry",
            Box::new(|e| {
                let i = index(e, "DEVICE_UNASSIGN_BEGIN");
                e[i]["originalCalls"] = json!(1);
                e[i]["originalCalled"] = json!(true);
            }),
        ),
        (
            "partial-claims-topology",
            Box::new(|e| {
                let i = index(e, "DEVICE_UNASSIGN_END");
                e[i]["topologyDigest"] = json!(specimen::digest('A'));
            }),
        ),
        (
            "partial-claims-device",
            Box::new(|e| {
                let i = index(e, "DEVICE_REASSIGN_END");
                e[i]["deviceDigest"] = json!(specimen::digest('9'));
            }),
        ),
        (
            "partial-claims-assignment-match",
            Box::new(|e| {
                let i = index(e, "DEVICE_UNASSIGN_END");
                e[i]["assignmentMatches"] = json!(true);
            }),
        ),
        (
            "detach-readback-equals-old",
            Box::new(|e| {
                for x in e
                    .iter_mut()
                    .filter(|x| x["assignmentDigest"] == specimen::digest('F'))
                {
                    x["assignmentDigest"] = json!(specimen::digest('6'));
                }
            }),
        ),
        (
            "detach-changes-midway",
            Box::new(|e| {
                let i = index(e, "DEVICE_REASSIGN_BEGIN");
                e[i]["assignmentDigest"] = json!(specimen::digest('E'));
            }),
        ),
        (
            "reassign-remains-detached",
            Box::new(|e| {
                let i = index(e, "DEVICE_REASSIGN_END");
                e[i]["assignmentDigest"] = json!(specimen::digest('F'));
            }),
        ),
        (
            "relation-not-reassign-readback",
            Box::new(|e| {
                let i = index(e, "RELATION_CHANGE");
                e[i]["assignmentDigest"] = json!(specimen::digest('7'));
            }),
        ),
        (
            "unclassified-topology-is-not-device-rebind",
            Box::new(|e| {
                let i = index(e, "RELATION_CHANGE");
                e[i]["kind"] = json!("UNCLASSIFIED_RELATION_CHANGE");
            }),
        ),
        (
            "duplicate-relation",
            Box::new(|e| {
                let i = index(e, "RELATION_CHANGE");
                e.insert(i, e[i].clone());
            }),
        ),
        (
            "relation-before-native-return",
            Box::new(|e| {
                let i = index(e, "RELATION_CHANGE");
                let x = e.remove(i);
                let j = index(e, "DEVICE_REASSIGN_END");
                e.insert(j, x);
            }),
        ),
        (
            "relation-without-manager-return",
            Box::new(|e| {
                let i = index(e, "RELATION_CHANGE");
                e.drain(i - 2..i);
            }),
        ),
        (
            "relation-wrong-pass",
            Box::new(|e| {
                let i = index(e, "RELATION_CHANGE");
                e[i]["loopIteration"] = json!(99);
            }),
        ),
        (
            "causal-manager-rejected-topology",
            Box::new(|e| {
                let i = index(e, "RELATION_CHANGE") - 1;
                assert_eq!(e[i]["kind"], "MANAGER_PASS_END");
                e[i]["topologyDigest"] = json!(specimen::digest('E'));
            }),
        ),
        (
            "causal-manager-missing-topology",
            Box::new(|e| {
                let i = index(e, "RELATION_CHANGE") - 1;
                e[i]["topologyDigest"] = Value::Null;
            }),
        ),
        (
            "confirmation-manager-rejected-topology",
            Box::new(|e| {
                let i = last_index(e, "MANAGER_PASS_END");
                e[i]["topologyDigest"] = json!(specimen::digest('E'));
            }),
        ),
        (
            "candidate-before-causal-relation",
            Box::new(|e| {
                let i = index(e, "RELATION_CHANGE");
                e.swap(i, i + 1);
            }),
        ),
        (
            "single-pass-bound",
            Box::new(|e| {
                let i = last_index(e, "MANAGER_PASS_BEGIN");
                e.drain(i..i + 2);
                for x in e
                    .iter_mut()
                    .filter(|x| x["ownerGeneration"] == 2 && x["kind"] == "BINDING_BOUND")
                {
                    x["loopIteration"] = json!(4);
                }
            }),
        ),
        (
            "nonconsecutive-manager-pass",
            Box::new(|e| {
                for x in e.iter_mut().filter(|x| x["loopIteration"] == 5) {
                    x["loopIteration"] = json!(6);
                }
            }),
        ),
        (
            "target-binding-keeps-old-assignment",
            Box::new(|e| {
                let i = last_index(e, "BINDING_BOUND");
                e[i]["assignmentDigest"] = json!(specimen::digest('6'));
            }),
        ),
        (
            "unrelated-endpoint-changed",
            Box::new(|e| {
                for x in e.iter_mut().filter(|x| {
                    x["ownerGeneration"] == 2
                        && x["localOrdinal"] == 0
                        && matches!(
                            x["kind"].as_str(),
                            Some("BINDING_CANDIDATE" | "BINDING_BOUND")
                        )
                }) {
                    x["deviceDigest"] = json!(specimen::digest('9'));
                }
            }),
        ),
        (
            "old-generation-resurrected",
            Box::new(|e| {
                let i = last_index(e, "BINDING_BOUND");
                e[i]["ownerGeneration"] = json!(1);
            }),
        ),
        (
            "generation-skipped",
            Box::new(|e| {
                for x in e.iter_mut().filter(|x| x["ownerGeneration"] == 2) {
                    x["ownerGeneration"] = json!(3);
                }
            }),
        ),
        (
            "only-one-invalidation",
            Box::new(|e| {
                let i = index(e, "BINDING_UNBOUND");
                e.remove(i);
            }),
        ),
        (
            "prior-single-player-only",
            Box::new(|e| {
                e.retain(|x| !(x["ownerGeneration"] == 1 && x["localOrdinal"] == 0));
            }),
        ),
        (
            "lifecycle-interruption",
            Box::new(|e| {
                let i = index(e, "DEVICE_REASSIGN_BEGIN");
                e.insert(i, specimen::event("LIFECYCLE_ENTER", 2, 3));
                e.insert(i + 1, specimen::event("LIFECYCLE_EXIT", 2, 3));
            }),
        ),
        (
            "producer-close-before-bound",
            Box::new(|e| {
                let i = index(e, "PRODUCER_CLOSED");
                let x = e.remove(i);
                let j = last_index(e, "BINDING_BOUND");
                e.insert(j, x);
            }),
        ),
    ]);
    for flag in [
        "topologyCoherent",
        "bindingUnique",
        "assignmentMatches",
        "hudMatches",
        "luaVectorEquivalent",
        "relationChanged",
    ] {
        cases.push((
            flag,
            Box::new(move |e| {
                let i = index(e, "RELATION_CHANGE");
                e[i][flag] = json!(false);
            }),
        ));
    }
    let root = TempDir::new().unwrap();
    for (name, mutate) in cases {
        let source = specimen::create(root.path(), name, true, true);
        let mut events = specimen::events(true, true);
        mutate(&mut events);
        specimen::resequence(&mut events);
        specimen::write_events(&source, &events);
        specimen::write_manifest(&source, &mut specimen::manifest(name));
        rejected(&source, &root.path().join(format!("{name}.json")), name);
    }
}

#[test]
fn strict_profile_restore_privacy_and_zero_capability_contract() {
    let root = TempDir::new().unwrap();
    let mut mutations: Vec<(&str, &str, Value)> = vec![
        ("schema-v1", "schemaVersion", json!(1)),
        ("schema-future", "schemaVersion", json!(3)),
        (
            "tool-v1",
            "toolIdentity",
            json!("IsaacCsT0-CoopPassiveObserver-v1"),
        ),
        (
            "tool-future",
            "toolIdentity",
            json!("IsaacCsT0-CoopPassiveObserver-v3"),
        ),
        ("wrong-scenario", "scenario", json!("room-rebind-passive")),
        ("old-53-slots", "slotCount", json!(53)),
        ("unknown-57-slots", "slotCount", json!(57)),
        (
            "old-53-mask",
            "installedHooksMask",
            json!("0x001FFFFFFFFFFFFF"),
        ),
        (
            "mask-superset",
            "restoredHooksMask",
            json!("0x01FFFFFFFFFFFFFF"),
        ),
        ("old-restore-count", "restoredSlotCount", json!(53)),
        ("ring-overwrite", "overwrittenEvents", json!(1)),
        ("undrained", "callbacksDrained", json!(false)),
        ("unclosed", "producerClosed", json!(false)),
        ("unsafe", "unsafeUnloadRequired", json!(true)),
        (
            "snapshot-mismatch",
            "slotSnapshotAfterSha256",
            json!(specimen::digest('8')),
        ),
        (
            "missing-event-hash",
            "eventsSha256",
            json!(specimen::digest('0')),
        ),
    ];
    for field in [
        "overrideCount",
        "applyCount",
        "worldWriteCount",
        "schedulerCount",
        "transportCount",
        "modeLatchWriteCount",
    ] {
        mutations.push((field, field, json!(1)));
    }
    for field in [
        "inputMutationAllowed",
        "worldMutationAllowed",
        "activeInputAllowed",
        "originalOnlinePositiveAllowed",
    ] {
        mutations.push((field, field, json!(true)));
    }
    for (name, field, value) in mutations {
        let source = specimen::create(root.path(), name, false, true);
        let mut manifest = specimen::manifest(name);
        manifest[field] = value.clone();
        specimen::write_manifest(&source, &mut manifest);
        if field == "eventsSha256" {
            manifest[field] = value;
            fs::write(
                source.join("manifest.json"),
                serde_json::to_vec_pretty(&manifest).unwrap(),
            )
            .unwrap();
        }
        rejected(&source, &root.path().join(format!("{name}.json")), name);
    }
    for field in [
        "pid",
        "processCreationTime",
        "moduleBase",
        "playerPointer",
        "assignmentKey",
        "InitSeed",
        "platformIdentity",
        "peerIdentity",
        "userPath",
        "syntheticInput",
        "futureCapability",
    ] {
        let source = specimen::create(root.path(), field, false, true);
        let mut manifest = specimen::manifest(field);
        manifest[field] = json!("C:\\Users\\private\\0x12345678");
        specimen::write_manifest(&source, &mut manifest);
        rejected(&source, &root.path().join(format!("{field}.json")), field);
    }
    for (name, event_mutation) in [
        ("unknown-event-field", 0),
        ("v1-event", 1),
        ("sequence-gap", 2),
        ("qpc-backwards", 3),
        ("unknown-event-code", 4),
        ("raw-address-in-player", 5),
    ] {
        let source = specimen::create(root.path(), name, false, true);
        let mut events = specimen::events(false, true);
        let i = index(&events, "DEVICE_REASSIGN_END");
        match event_mutation {
            0 => events[i]["futureFlag"] = json!(true),
            1 => events[i]["schemaVersion"] = json!(1),
            2 => events[i]["sequence"] = json!(999),
            3 => events[i]["qpc"] = json!(1),
            4 => events[i]["kind"] = json!("DEVICE_UNKNOWN"),
            _ => events[i]["playerDigest"] = json!("0x12345678"),
        }
        specimen::write_events(&source, &events);
        specimen::write_manifest(&source, &mut specimen::manifest(name));
        rejected(&source, &root.path().join(format!("{name}.json")), name);
    }
    for variant in 0..8 {
        let name = format!("restore-order-{variant}");
        let source = specimen::create(root.path(), &name, false, true);
        let mut manifest = specimen::manifest(&name);
        match variant {
            0 => manifest["installOrder"]
                .as_array_mut()
                .unwrap()
                .swap(53, 55),
            1 => manifest["restoreOrder"].as_array_mut().unwrap().swap(0, 2),
            2 => manifest["targetSlots"][55]["afterProof"] = json!(specimen::digest('9')),
            3 => manifest["targetSlots"][55]["ordinal"] = json!(56),
            4 => {
                for key in ["expectedProof", "beforeProof", "afterProof"] {
                    manifest["targetSlots"][55][key] = json!(specimen::digest('9'));
                }
            }
            5 => manifest["targetSlots"][55]["id"] = json!("unfrozen-target"),
            6 => manifest["targetSlots"][55]["rva"] = json!("0x00100000"),
            _ => manifest["targetSlots"][55]["patchKind"] = json!("IMAGE_OR_IMPORT_POINTER"),
        }
        specimen::write_manifest(&source, &mut manifest);
        rejected(&source, &root.path().join(format!("{name}.json")), &name);
    }
}

#[test]
fn four_immutable_v1_false_positive_sources_are_now_blocked() {
    let repository = Path::new(env!("CARGO_MANIFEST_DIR")).parent().unwrap();
    let fixtures =
        repository.join("out/cs-t0/device-rebind-independent-audit-20260831-v1/fixtures");
    let root = TempDir::new().unwrap();
    let mut count = 0;
    for entry in fs::read_dir(fixtures).expect("frozen four-counterexample audit") {
        let source = entry.unwrap().path();
        if !source.is_dir() {
            continue;
        }
        let before = ["manifest.json", "events.jsonl", "analysis.json"]
            .map(|f| specimen::hash(&source.join(f)));
        rejected(
            &source,
            &root.path().join(format!("counterexample-{count}.json")),
            source.file_name().unwrap().to_str().unwrap(),
        );
        assert_eq!(
            before,
            ["manifest.json", "events.jsonl", "analysis.json"]
                .map(|f| specimen::hash(&source.join(f)))
        );
        count += 1;
    }
    assert_eq!(count, 4);
}

#[test]
fn approved_historical_sources_replay_byte_for_byte_without_rewriting() {
    let repository = Path::new(env!("CARGO_MANIFEST_DIR")).parent().unwrap();
    let root = TempDir::new().unwrap();
    let cases = [
        (
            "20260828T072118Z-t0-05-03-native-join-passive-t0-05-03-native-join-passive-live-06",
            "BD2A8C6E8EE18B7D2E0B684062193731B39750DE6835403358AE196827B1FD53",
        ),
        (
            "20260830T182858Z-t0-05-03-death-respawn-passive-t0-05-03-death-respawn-passive-live-08",
            "B4113180004E9F765EE88F117D84D095909D230155DB3BF809A45AF828711C78",
        ),
        (
            "20260830T185508Z-t0-05-03-room-rebind-passive-t0-05-03-room-rebind-passive-live-09",
            "FABD4445747902C73B73E7907AC2B9F6A290807F6CAC4182795FC62711E7DACA",
        ),
    ];
    for (i, (name, analysis_hash)) in cases.into_iter().enumerate() {
        let source = repository.join("artifacts/cs-t0").join(name);
        let snapshots = ["manifest.json", "events.jsonl", "analysis.json"]
            .map(|f| specimen::hash(&source.join(f)));
        assert_eq!(snapshots[2], analysis_hash);
        let output = root.path().join(format!("history-{i}.json"));
        let analysis = analyze(&source, &output).unwrap();
        assert_eq!(analysis["result"], "PASS");
        assert_eq!(analysis["schemaVersion"], 1);
        assert_eq!(specimen::hash(&output), analysis_hash);
        assert_eq!(
            snapshots,
            ["manifest.json", "events.jsonl", "analysis.json"]
                .map(|f| specimen::hash(&source.join(f)))
        );
    }
}

#[test]
fn live10_terminal_generation_mismatch_remains_immutable_and_blocked() {
    let repository = Path::new(env!("CARGO_MANIFEST_DIR")).parent().unwrap();
    let source = repository.join("artifacts/cs-t0/20260831T055322Z-t0-05-03-device-rebind-passive-t0-05-03-device-rebind-passive-live-10");
    let names = ["manifest.json", "events.jsonl", "analysis.json"];
    let expected = [
        "09E3C363E761DD79BC6583817F227321A0B658CBBD6C51655F3C3DAFAF93C8E5",
        "03A88940CE7BCCEAFFDBA84BF0A7163A54A36209710BE50AC75B4151C4BBF489",
        "1E611E206C1DDDC9574CB75F1F5BC76BBBC775B1106F58B417103BB638EF56A1",
    ];
    assert_eq!(
        names.map(|name| specimen::hash(&source.join(name))),
        expected
    );
    let root = TempDir::new().unwrap();
    let replay = root.path().join("live10-replay.json");
    let analysis = analyze(&source, &replay).expect("historical failure remains analyzable");
    assert_eq!(analysis["result"], "BLOCKED_BY_EVIDENCE");
    let failed: Vec<_> = analysis["checks"]
        .as_array()
        .unwrap()
        .iter()
        .filter(|check| check["passed"] == false)
        .map(|check| check["id"].as_str().unwrap())
        .collect();
    assert_eq!(failed, ["generation-ordinal-binding", "scenario-contract"]);
    assert_eq!(
        fs::read(&replay).unwrap(),
        fs::read(source.join("analysis.json")).unwrap()
    );
    assert_eq!(
        names.map(|name| specimen::hash(&source.join(name))),
        expected
    );
}

#[test]
fn shutdown_does_not_waive_incomplete_cause_or_terminal_generation_drift() {
    let root = TempDir::new().unwrap();
    for variant in 0..4 {
        let name = format!("shutdown-negative-{variant}");
        let source = specimen::create(root.path(), &name, false, true);
        let mut events = specimen::events(false, true);
        match variant {
            0 => {
                for event in events.iter_mut().rev().take(2) {
                    event["ownerGeneration"] = json!(3);
                }
            }
            1 => {
                events.last_mut().unwrap()["ownerGeneration"] = json!(3);
            }
            2 => {
                let begin = index(&events, "DEVICE_REASSIGN_BEGIN");
                let end = index(&events, "HOOKS_RESTORED");
                events.drain(begin..end);
            }
            _ => {
                events.retain(|e| !(e["ownerGeneration"] == 2 && e["kind"] == "BINDING_BOUND"));
            }
        }
        specimen::resequence(&mut events);
        specimen::write_events(&source, &events);
        specimen::write_manifest(&source, &mut specimen::manifest(&name));
        rejected(&source, &root.path().join(format!("{name}.json")), &name);
    }
}
