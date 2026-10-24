//! Independent serialized device-v2 specimens. No production event constructors or validators.
#![allow(dead_code)]

use std::fs::{self, OpenOptions};
use std::io::Write;
use std::path::{Path, PathBuf};

use serde_json::{Value, json};
use sha2::{Digest, Sha256};

pub fn digest(c: char) -> String {
    std::iter::repeat_n(c, 64).collect()
}

pub fn hash(path: &Path) -> String {
    hex::encode_upper(Sha256::digest(
        fs::read(path).expect("read hashed specimen"),
    ))
}

pub fn event(kind: &str, generation: u64, pass: u64) -> Value {
    json!({
        "schemaVersion": 2, "sequence": 0, "qpc": 0, "loopIteration": pass,
        "kind": kind, "slotOrdinal": null, "ownerThreadToken": digest('D'),
        "mainThread": !matches!(kind, "NATIVE_INITIALIZED" | "HOOKS_INSTALLED" | "HOOKS_RESTORED" | "PRODUCER_CLOSED"),
        "phase": 2, "depth": 1, "ownerGeneration": generation,
        "localOrdinal": null, "endpoint": null, "topologyDigest": null,
        "playerDigest": null, "deviceDigest": null, "assignmentDigest": null,
        "topologyCoherent": false, "bindingUnique": false, "assignmentMatches": false,
        "hudMatches": false, "luaVectorEquivalent": false, "relationChanged": false,
        "stableRegistryRetired": false, "bindingState": null,
        "originalCalls": 0, "originalCalled": false
    })
}

pub fn relation(kind: &str, generation: u64, pass: u64, ordinal: u32, changed: bool) -> Value {
    let mut e = event(kind, generation, pass);
    e["localOrdinal"] = json!(ordinal);
    e["endpoint"] = json!(if ordinal == 0 {
        "ENDPOINT_A"
    } else {
        "ENDPOINT_B"
    });
    e["topologyDigest"] = json!(digest(if changed { 'B' } else { 'A' }));
    e["playerDigest"] = json!(digest(if ordinal == 0 { '1' } else { '2' }));
    e["deviceDigest"] = json!(digest(if ordinal == 0 {
        '3'
    } else if changed {
        '9'
    } else {
        '4'
    }));
    e["assignmentDigest"] = json!(digest(if ordinal == 0 {
        '5'
    } else if changed {
        '8'
    } else {
        '6'
    }));
    for key in [
        "topologyCoherent",
        "bindingUnique",
        "assignmentMatches",
        "hudMatches",
        "luaVectorEquivalent",
    ] {
        e[key] = json!(true);
    }
    e["bindingState"] = match kind {
        "BINDING_CANDIDATE" => json!("CANDIDATE"),
        "BINDING_BOUND" => json!("BOUND"),
        "BINDING_UNBOUND" => json!("UNBOUND"),
        _ => Value::Null,
    };
    e
}

pub fn manager(events: &mut Vec<Value>, generation: u64, pass: u64, changed: bool) {
    let mut begin = event("MANAGER_PASS_BEGIN", generation, pass);
    begin["slotOrdinal"] = json!(45);
    events.push(begin);
    let mut end = event("MANAGER_PASS_END", generation, pass);
    end["slotOrdinal"] = json!(45);
    end["originalCalls"] = json!(1);
    end["originalCalled"] = json!(true);
    end["topologyDigest"] = json!(digest(if changed { 'B' } else { 'A' }));
    end["topologyCoherent"] = json!(true);
    end["bindingUnique"] = json!(true);
    events.push(end);
}

pub fn pair(events: &mut Vec<Value>, kind: &str, generation: u64, pass: u64, changed: bool) {
    for ordinal in 0..2 {
        events.push(relation(kind, generation, pass, ordinal, changed));
    }
}

fn assignment(kind: &str, slot: u32, full_old: bool, assigned: bool, changed: bool) -> Value {
    let mut e = relation(kind, 2, 3, 1, changed && assigned);
    e["slotOrdinal"] = json!(slot);
    e["phase"] = json!(10);
    e["depth"] = json!(0);
    if !full_old {
        e["topologyDigest"] = Value::Null;
        e["deviceDigest"] = Value::Null;
        for key in [
            "topologyCoherent",
            "bindingUnique",
            "assignmentMatches",
            "hudMatches",
            "luaVectorEquivalent",
        ] {
            e[key] = json!(false);
        }
        e["assignmentDigest"] = json!(digest(if assigned {
            if changed { '8' } else { '6' }
        } else {
            'F'
        }));
    }
    if kind.ends_with("_END") {
        e["originalCalls"] = json!(1);
        e["originalCalled"] = json!(true);
    }
    e
}

pub fn resequence(events: &mut [Value]) {
    for (index, e) in events.iter_mut().enumerate() {
        e["sequence"] = json!(index);
        e["qpc"] = json!(10_000 + index);
    }
}

pub fn events(menu: bool, changed_device: bool) -> Vec<Value> {
    let mut events = vec![
        event("NATIVE_INITIALIZED", 0, 0),
        event("HOOKS_INSTALLED", 0, 0),
    ];
    manager(&mut events, 1, 1, false);
    pair(&mut events, "BINDING_CANDIDATE", 1, 1, false);
    manager(&mut events, 1, 2, false);
    pair(&mut events, "BINDING_BOUND", 1, 2, false);
    pair(&mut events, "BINDING_UNBOUND", 2, 3, false);
    for e in events.iter_mut().rev().take(2) {
        e["phase"] = json!(10);
    }
    let primary = if menu { 53 } else { 54 };
    events.push(assignment(
        "DEVICE_UNASSIGN_BEGIN",
        primary,
        true,
        false,
        false,
    ));
    events.push(assignment(
        "DEVICE_UNASSIGN_END",
        primary,
        false,
        false,
        false,
    ));
    if menu {
        events.push(assignment("DEVICE_UNASSIGN_BEGIN", 54, false, false, false));
        events.push(assignment("DEVICE_UNASSIGN_END", 54, false, false, false));
    }
    events.push(assignment("DEVICE_REASSIGN_BEGIN", 55, false, false, false));
    events.push(assignment(
        "DEVICE_REASSIGN_END",
        55,
        false,
        true,
        changed_device,
    ));
    manager(&mut events, 2, 4, changed_device);
    let mut change = relation("RELATION_CHANGE", 2, 4, 1, changed_device);
    change["slotOrdinal"] = json!(55);
    change["phase"] = json!(10);
    change["relationChanged"] = json!(true);
    events.push(change);
    pair(&mut events, "BINDING_CANDIDATE", 2, 4, changed_device);
    manager(&mut events, 2, 5, changed_device);
    pair(&mut events, "BINDING_BOUND", 2, 5, changed_device);
    events.push(event("HOOKS_RESTORED", 2, 5));
    events.push(event("PRODUCER_CLOSED", 2, 5));
    resequence(&mut events);
    events
}

pub fn manifest(name: &str) -> Value {
    let catalog: Value = serde_json::from_slice(
        &fs::read(Path::new(env!("CARGO_MANIFEST_DIR")).join("coop-catalog-v4.json"))
            .expect("read public frozen v4 catalog"),
    )
    .unwrap();
    let slots: Vec<Value> = catalog["orderedHookTransaction"]["slots"].as_array().unwrap().iter().map(|slot| {
        let ordinal=slot["ordinal"].as_u64().unwrap();
        let proof=slot.get("expectedProof").or_else(||slot.get("expectedBytes")).unwrap().as_str().unwrap();
        let mut hasher=Sha256::new();
        hasher.update(b"slot"); hasher.update([ordinal as u8;32]); hasher.update(proof.as_bytes());
        let expected=hex::encode_upper(hasher.finalize());
        json!({"ordinal":ordinal,"id":slot["id"],"patchKind":slot["patchKind"],"rva":slot["rva"],
            "expectedProof":expected,"beforeProof":expected,"afterProof":expected})
    }).collect();
    json!({
        "schemaVersion": 2, "evidenceClass": "COOP_PASSIVE_LIFECYCLE",
        "toolIdentity": "IsaacCsT0-CoopPassiveObserver-v2", "artifactIdentity": format!("PASSIVE-{name}"),
        "workPackage": "CS-T0-05-03", "caseId": name, "probe": "passive",
        "scenario": "device-rebind-passive", "startedUtc": "2026-08-31T00:00:00Z",
        "endedUtc": "2026-08-31T00:03:00Z", "state": "PASS", "failureReason": null,
        "targetSha256": digest('A'), "observedTargetSha256": digest('B'),
        "targetEquivalenceSha256": digest('C'), "layoutEvidenceSha256": digest('D'),
        "coopCatalogSha256": digest('E'), "coopGraphSha256": digest('F'),
        "coopPlanSha256": digest('1'), "coopVerificationSha256": digest('2'),
        "nativeSha256": digest('3'), "helperSha256": digest('4'),
        "processIdentityToken": digest('5'), "eventsSha256": digest('0'),
        "slotCount": 56, "installedHooksMask": "0x00FFFFFFFFFFFFFF",
        "restoredHooksMask": "0x00FFFFFFFFFFFFFF", "restoredSlotCount": 56,
        "installOrder": (0..56).collect::<Vec<_>>(), "restoreOrder": (0..56).rev().collect::<Vec<_>>(),
        "targetSlots": slots,
        "slotSnapshotBeforeSha256": digest('6'), "slotSnapshotAfterSha256": digest('6'),
        "mainThreadIdToken": digest('7'), "finalOwnerGeneration": 2,
        "overwrittenEvents": 0, "callbacksDrained": true, "producerClosed": true,
        "unsafeUnloadRequired": false, "stableRegistryRetired": false,
        "overrideCount": 0, "applyCount": 0, "worldWriteCount": 0, "schedulerCount": 0,
        "transportCount": 0, "modeLatchWriteCount": 0, "inputMutationAllowed": false,
        "worldMutationAllowed": false, "activeInputAllowed": false, "originalOnlinePositiveAllowed": false
    })
}

pub fn write_events(run: &Path, events: &[Value]) {
    let mut bytes = Vec::new();
    for e in events {
        serde_json::to_writer(&mut bytes, e).expect("serialize independent event");
        bytes.push(b'\n');
    }
    fs::write(run.join("events.jsonl"), bytes).expect("write disposable events");
}

pub fn write_manifest(run: &Path, manifest: &mut Value) {
    manifest["eventsSha256"] = json!(hash(&run.join("events.jsonl")));
    fs::write(
        run.join("manifest.json"),
        serde_json::to_vec_pretty(manifest).unwrap(),
    )
    .unwrap();
}

pub fn create(root: &Path, name: &str, menu: bool, changed_device: bool) -> PathBuf {
    let run = root.join(name);
    fs::create_dir(&run).expect("create-new independent source");
    write_events(&run, &events(menu, changed_device));
    write_manifest(&run, &mut manifest(name));
    run
}

pub fn create_new_json(path: &Path, value: &Value) {
    let mut file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .unwrap();
    file.write_all(&serde_json::to_vec_pretty(value).unwrap())
        .unwrap();
}
