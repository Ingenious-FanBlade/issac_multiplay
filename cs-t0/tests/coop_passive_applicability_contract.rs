//! Independent public-CLI tests for five PASS sources plus scoped native-exit N/A.
//! All mutations target disposable copies; historical evidence is always read-only.

use std::fs;
use std::path::Path;
use std::process::Command;

use serde_json::{Value, json};

#[path = "support/coop_applicability_fixture.rs"]
mod fixture;

use fixture::{APPLICABLE, EXIT, Fixture, binary, create_new_bytes, read_json};

fn assert_rejected(mut command: Command, output_path: &Path, context: &str) {
    assert!(!output_path.exists(), "{context}: output must be new");
    let output = command
        .arg("--output")
        .arg(output_path)
        .output()
        .expect("invoke negative public CLI case");
    let written_decision = if output_path.exists() {
        let decision = read_json(output_path);
        json!({
            "status":decision["status"],
            "csT00504Unlocked":decision["csT00504Unlocked"],
            "activeInputAllowed":decision["activeInputAllowed"]
        })
    } else {
        Value::Null
    };
    assert!(
        matches!(output.status.code(), Some(1 | 2)),
        "{context}: unexpected status {:?}; decision={written_decision}; stderr={}",
        output.status.code(),
        String::from_utf8_lossy(&output.stderr)
    );
    assert!(
        !output_path.exists(),
        "{context}: rejected input wrote a decision"
    );
}

fn assert_decision_scope(fixture: &Fixture, decision: &Value, formal: bool) {
    let contract = read_json(&fixture.applicability);
    let mut permissions = contract["zeroCapabilities"].clone();
    permissions["csT00504Unlocked"] = json!(formal);
    assert_eq!(decision["permissions"], permissions);
    assert_eq!(decision["sources"], contract["sources"]);
    assert_eq!(decision["historicalExit"], contract["historicalExit"]);
    assert_eq!(
        decision["historicalExit"]["expectedResult"],
        "BLOCKED_BY_EVIDENCE"
    );
    assert_eq!(
        decision["applicability"]["status"],
        "NOT_APPLICABLE_NATIVE_ROUTE"
    );
    assert_eq!(decision["applicability"]["scope"], contract["scope"]);
    assert_eq!(
        decision["capabilityLimitations"],
        contract["capabilityLimitations"]
    );
    for field in [
        "sixSourceGateEligible",
        "historicalExitResultChanged",
        "liveAuthorizationCreated",
    ] {
        assert_eq!(decision[field], json!(false));
    }
    for field in [
        "nativeSinglePlayerRemovalVerified",
        "customEndpointExitImplemented",
        "customPlayerRemovalImplemented",
        "activeLiveAuthorized",
        "t006Unlocked",
    ] {
        assert_eq!(decision["capabilityLimitations"][field], json!(false));
    }
    assert_eq!(
        decision["capabilityLimitations"]["menuUiConfirmation"],
        "NOT_RECEIVED"
    );
    assert_eq!(
        decision["capabilityLimitations"]["menuInitialDoubleBoundRequired"],
        json!(true)
    );
    assert_eq!(
        decision["capabilityLimitations"]["deathRespawnProvenance"],
        "OPERATOR_CONFIRMED_WITH_INHERITED_LIFECYCLE_NOT_DEDICATED_DEATH_HOOK"
    );
}

#[test]
fn applicability_five_live_sources_without_verification_are_evidence_only() {
    let fixture = Fixture::new();
    let output_path = fixture.temp.path().join("evidence-only.json");
    let output = fixture.invoke(&output_path);
    assert!(
        output.status.success(),
        "public gate failed: {}",
        String::from_utf8_lossy(&output.stderr)
    );
    let decision = read_json(&output_path);
    assert_decision_scope(&fixture, &decision, false);
    assert_eq!(decision["schemaVersion"], 3);
    assert_eq!(
        decision["decisionIdentity"],
        "IsaacCsT0-CoopPassiveApplicabilityDecision-v3"
    );
    assert_eq!(decision["workPackage"], "CS-T0-05-03");
    assert_eq!(decision["status"], "PASS_EVIDENCE_ONLY");
    assert_eq!(decision["formalPassCount"], 5);
    assert_eq!(decision["requiredApplicablePassCount"], 5);
    assert_eq!(decision["notApplicableCount"], 1);
    assert_eq!(decision["csT00504Unlocked"], json!(false));
    assert_eq!(decision["t005Unlocked"], json!(false));
    assert_eq!(decision["activeInputAllowed"], json!(false));
    assert_eq!(decision["passiveHookTransactionAllowed"], json!(false));
    assert_eq!(decision["runtimeMutationAllowed"], json!(false));
    let serialized = fs::read_to_string(&output_path).unwrap();
    assert!(
        !serialized.contains(&fixture.temp.path().to_string_lossy().replace('\\', "\\\\")),
        "decision leaked the absolute disposable repository path"
    );
    fixture.assert_sources_unchanged();
}

#[test]
fn applicability_public_replays_preserve_five_passes_and_live07_blocked_bytes() {
    let fixture = Fixture::new();
    let all_sources = fixture
        .sources
        .iter()
        .zip(APPLICABLE)
        .chain(std::iter::once((&fixture.exit_source, EXIT)));
    for (source, spec) in all_sources {
        let replay = fixture
            .temp
            .path()
            .join(format!("{}-replay.json", spec.scenario));
        assert!(!replay.exists());
        let output = Command::new(binary())
            .arg("coop-analyze")
            .arg(source)
            .arg("--output")
            .arg(&replay)
            .output()
            .expect("public read-only replay");
        assert_eq!(
            output.status.success(),
            spec.scenario != EXIT.scenario,
            "unexpected replay status: {}",
            String::from_utf8_lossy(&output.stderr)
        );
        assert_eq!(
            fs::read(&replay).unwrap(),
            fs::read(source.join("analysis.json")).unwrap(),
            "historical {} analysis was reinterpreted",
            spec.scenario
        );
        assert_eq!(
            read_json(&replay)["result"],
            if spec.scenario == EXIT.scenario {
                "BLOCKED_BY_EVIDENCE"
            } else {
                "PASS"
            }
        );
        let first = fs::read(&replay).unwrap();
        let repeated = Command::new(binary())
            .arg("coop-analyze")
            .arg(source)
            .arg("--output")
            .arg(&replay)
            .output()
            .unwrap();
        assert!(!repeated.status.success());
        assert_eq!(fs::read(&replay).unwrap(), first);
    }
    fixture.assert_sources_unchanged();
}

#[test]
fn legacy_six_source_cli_still_rejects_only_five_and_applicability_switches() {
    let fixture = Fixture::new();
    for add_applicability in [false, true] {
        let output_path = fixture
            .temp
            .path()
            .join(format!("legacy-five-{add_applicability}.json"));
        let mut command = Command::new(binary());
        command.arg("coop-passive-gate");
        for (spec, source) in APPLICABLE.iter().zip(&fixture.sources) {
            command.arg(format!("--{}", spec.scenario)).arg(source);
        }
        if add_applicability {
            command.arg("--applicability").arg(&fixture.applicability);
        }
        let output = command.arg("--output").arg(&output_path).output().unwrap();
        assert!(
            !output.status.success(),
            "legacy gate silently became a five-source gate"
        );
        assert!(!output_path.exists());
    }
    fixture.assert_sources_unchanged();
}

#[test]
fn applicability_gate_output_is_create_new_even_for_valid_evidence() {
    let fixture = Fixture::new();
    let occupied = fixture.temp.path().join("occupied-decision.json");
    let sentinel = b"independent occupied output; do not overwrite";
    create_new_bytes(&occupied, sentinel);
    let output = fixture
        .command()
        .arg("--output")
        .arg(&occupied)
        .output()
        .unwrap();
    assert!(!output.status.success());
    assert_eq!(fs::read(&occupied).unwrap(), sentinel);
    fixture.assert_sources_unchanged();
}

#[test]
fn applicability_cli_rejects_unknown_skip_and_duplicate_source_flags() {
    let fixture = Fixture::new();
    for (index, flag) in [
        "--skip-native-exit",
        "--allow-skipped",
        "--ignore-unsafe",
        "--active",
        "--future-applicability",
    ]
    .into_iter()
    .enumerate()
    {
        let mut command = fixture.command();
        command.arg(flag).arg("true");
        assert_rejected(
            command,
            &fixture
                .temp
                .path()
                .join(format!("unknown-flag-{index}.json")),
            flag,
        );
    }
    for (index, spec) in APPLICABLE.iter().enumerate() {
        let mut command = fixture.command();
        command
            .arg(format!("--{}", spec.scenario))
            .arg(&fixture.sources[index]);
        assert_rejected(
            command,
            &fixture
                .temp
                .path()
                .join(format!("duplicate-flag-{index}.json")),
            spec.scenario,
        );
    }
    fixture.assert_sources_unchanged();
}

#[test]
fn applicability_cli_rejects_each_missing_applicable_source() {
    let fixture = Fixture::new();
    for (omitted, omitted_spec) in APPLICABLE.iter().enumerate() {
        let mut command = Command::new(binary());
        command
            .arg("coop-passive-applicability-gate")
            .arg("--repository-root")
            .arg(&fixture.repository_root)
            .arg("--applicability")
            .arg(&fixture.applicability)
            .arg("--native-exit-source")
            .arg(&fixture.exit_source);
        for (index, spec) in APPLICABLE.iter().enumerate() {
            if index != omitted {
                command
                    .arg(format!("--{}", spec.scenario))
                    .arg(&fixture.sources[index]);
            }
        }
        assert_rejected(
            command,
            &fixture.temp.path().join(format!("missing-{omitted}.json")),
            omitted_spec.scenario,
        );
    }
    fixture.assert_sources_unchanged();
}

#[test]
fn applicability_decisions_cannot_be_written_inside_any_source() {
    let fixture = Fixture::new();
    for source in fixture
        .sources
        .iter()
        .chain(std::iter::once(&fixture.exit_source))
    {
        let forbidden = source.join("new-gate-decision.json");
        assert_rejected(fixture.command(), &forbidden, "output inside source");
    }
    fixture.assert_sources_unchanged();
}

fn changed_scalar(value: &Value) -> Value {
    match value {
        Value::Bool(value) => json!(!value),
        Value::Number(value) => json!(value.as_u64().unwrap() + 1),
        Value::String(value) if value.len() == 64 => json!("0".repeat(64)),
        Value::String(_) => json!("UNREVIEWED_SCOPE"),
        _ => panic!("mutation requires a scalar"),
    }
}

#[test]
fn applicability_contract_rejects_identity_scope_hash_skip_and_capability_drift() {
    let fixture = Fixture::new();
    let original = fs::read(&fixture.applicability).unwrap();
    let contract: Value = serde_json::from_slice(&original).unwrap();
    let mut mutations: Vec<(String, Value)> = Vec::new();
    for pointer in [
        "/schemaVersion",
        "/applicabilityIdentity",
        "/workPackage",
        "/createNewOnly",
    ] {
        mutations.push((
            pointer.to_owned(),
            changed_scalar(contract.pointer(pointer).unwrap()),
        ));
    }
    for status in [
        "SKIPPED",
        "SKIPPED_NOT_APPLICABLE",
        "UNKNOWN",
        "PASS",
        "NOT_APPLICABLE",
    ] {
        mutations.push(("/status".to_owned(), json!(status)));
    }
    for object in ["scope", "capabilityLimitations", "zeroCapabilities"] {
        for (field, value) in contract[object].as_object().unwrap() {
            mutations.push((format!("/{object}/{field}"), changed_scalar(value)));
        }
    }
    for binding in ["baseline", "predecessorVerification", "predecessorProgress"] {
        for field in ["path", "sha256"] {
            let pointer = format!("/{binding}/{field}");
            mutations.push((
                pointer.clone(),
                changed_scalar(contract.pointer(&pointer).unwrap()),
            ));
        }
    }
    for index in 0..contract["staticArtifacts"].as_array().unwrap().len() {
        for field in ["path", "sha256"] {
            let pointer = format!("/staticArtifacts/{index}/{field}");
            mutations.push((
                pointer.clone(),
                changed_scalar(contract.pointer(&pointer).unwrap()),
            ));
        }
    }
    let mut source_pointers: Vec<_> = (0..5).map(|index| format!("/sources/{index}")).collect();
    source_pointers.push("/historicalExit".to_owned());
    for source in source_pointers {
        for field in [
            "scenario",
            "artifactIdentity",
            "sourcePath",
            "manifestSha256",
            "eventsSha256",
            "analysisSha256",
            "expectedResult",
        ] {
            let pointer = format!("{source}/{field}");
            mutations.push((
                pointer.clone(),
                changed_scalar(contract.pointer(&pointer).unwrap()),
            ));
        }
        for field in ["foundation", "producer"] {
            for index in 0..5 {
                let pointer = format!("{source}/{field}/{index}");
                mutations.push((
                    pointer.clone(),
                    changed_scalar(contract.pointer(&pointer).unwrap()),
                ));
            }
        }
    }
    for (index, (pointer, replacement)) in mutations.iter().enumerate() {
        let mut altered = contract.clone();
        *altered
            .pointer_mut(pointer)
            .expect("frozen public field exists") = replacement.clone();
        fs::write(
            &fixture.applicability,
            serde_json::to_vec_pretty(&altered).unwrap(),
        )
        .unwrap();
        let before = fixture::hash(&fixture.applicability);
        assert_rejected(
            fixture.command(),
            &fixture
                .temp
                .path()
                .join(format!("contract-mutation-{index}.json")),
            pointer,
        );
        assert_eq!(fixture::hash(&fixture.applicability), before);
    }
    fs::write(&fixture.applicability, original).unwrap();
    fixture.assert_sources_unchanged();
    eprintln!(
        "PASS applicability contract scalar mutations: {}",
        mutations.len()
    );
}

#[test]
fn applicability_contract_rejects_unknown_duplicate_whitespace_and_generic_skip_documents() {
    let fixture = Fixture::new();
    let original = fs::read(&fixture.applicability).unwrap();
    let contract: Value = serde_json::from_slice(&original).unwrap();
    let mut specimens = Vec::new();
    for parent in ["", "/scope", "/capabilityLimitations", "/historicalExit"] {
        let mut value = contract.clone();
        value
            .pointer_mut(parent)
            .unwrap()
            .as_object_mut()
            .unwrap()
            .insert("futureOverride".to_owned(), json!(true));
        specimens.push(serde_json::to_vec_pretty(&value).unwrap());
    }
    let mut duplicate = b"{\"schemaVersion\":1,".to_vec();
    duplicate.extend_from_slice(&original[1..]);
    specimens.push(duplicate);
    let mut whitespace = original.clone();
    whitespace.push(b' ');
    specimens.push(whitespace);
    specimens.push(serde_json::to_vec(&contract).unwrap());
    specimens.push(
        serde_json::to_vec(&json!({"status":"SKIPPED_NOT_APPLICABLE","userApproved":true}))
            .unwrap(),
    );
    for array in [
        "sources",
        "staticArtifacts",
        "requiredAutomaticChecks",
        "verificationArtifactPaths",
    ] {
        let mut value = contract.clone();
        value[array].as_array_mut().unwrap().pop();
        specimens.push(serde_json::to_vec_pretty(&value).unwrap());
        let mut value = contract.clone();
        let first = value[array][0].clone();
        value[array].as_array_mut().unwrap().push(first);
        specimens.push(serde_json::to_vec_pretty(&value).unwrap());
    }
    for (index, bytes) in specimens.iter().enumerate() {
        fs::write(&fixture.applicability, bytes).unwrap();
        assert_rejected(
            fixture.command(),
            &fixture
                .temp
                .path()
                .join(format!("document-shape-{index}.json")),
            "contract shape or exact bytes",
        );
        assert_eq!(&fs::read(&fixture.applicability).unwrap(), bytes);
    }
    fs::write(&fixture.applicability, original).unwrap();
    fixture.assert_sources_unchanged();
}

#[test]
fn applicability_rejects_each_source_file_rewrite_and_live07_pass_forgery() {
    let fixture = Fixture::new();
    for (source_index, source) in fixture
        .sources
        .iter()
        .chain(std::iter::once(&fixture.exit_source))
        .enumerate()
    {
        for file in fixture::FILES {
            let path = source.join(file);
            let original = fs::read(&path).unwrap();
            let mut modified = original.clone();
            if file == "events.jsonl" {
                modified.insert(0, b' ');
            } else {
                modified.push(b' ');
            }
            fs::write(&path, &modified).unwrap();
            assert_rejected(
                fixture.command(),
                &fixture
                    .temp
                    .path()
                    .join(format!("source-{source_index}-{file}-rewrite.json")),
                "semantically neutral source rewrite must lose exact binding",
            );
            assert_eq!(fs::read(&path).unwrap(), modified);
            fs::write(&path, original).unwrap();
        }
    }
    let path = fixture.exit_source.join("analysis.json");
    let original = fs::read(&path).unwrap();
    let mut forged: Value = serde_json::from_slice(&original).unwrap();
    forged["result"] = json!("PASS");
    for check in forged["checks"].as_array_mut().unwrap() {
        check["passed"] = json!(true);
    }
    fs::write(&path, serde_json::to_vec_pretty(&forged).unwrap()).unwrap();
    assert_rejected(
        fixture.command(),
        &fixture.temp.path().join("forged-live07-pass.json"),
        "N/A must never reinterpret live07 as PASS",
    );
    fs::write(&path, original).unwrap();
    fixture.assert_sources_unchanged();
}

#[test]
fn applicability_never_waives_source_safety_zero_capabilities_or_privacy() {
    let fixture = Fixture::new();
    let path = fixture.sources[4].join("manifest.json");
    let original = fs::read(&path).unwrap();
    let manifest: Value = serde_json::from_slice(&original).unwrap();
    let mut mutations = Vec::new();
    for field in [
        "unsafeUnloadRequired",
        "activeInputAllowed",
        "inputMutationAllowed",
        "worldMutationAllowed",
        "originalOnlinePositiveAllowed",
    ] {
        mutations.push((field, json!(true)));
    }
    for field in [
        "callbacksDrained",
        "producerClosed",
        "stableRegistryRetired",
    ] {
        mutations.push((field, json!(false)));
    }
    for field in [
        "overrideCount",
        "applyCount",
        "worldWriteCount",
        "schedulerCount",
        "transportCount",
        "modeLatchWriteCount",
        "overwrittenEvents",
    ] {
        mutations.push((field, json!(1)));
    }
    mutations.extend([
        ("restoredSlotCount", json!(52)),
        ("restoredHooksMask", json!("0x000FFFFFFFFFFFFF")),
        ("slotSnapshotAfterSha256", json!("9".repeat(64))),
        ("pid", json!(1234)),
        ("processCreationTime", json!(1234)),
        ("moduleBase", json!("0x12340000")),
        ("playerPointer", json!("0x12345678")),
        ("assignmentKey", json!(3)),
        ("InitSeed", json!(12345)),
        ("peerIdentity", json!("private-peer")),
        ("platformIdentity", json!("private-platform")),
        ("artifactIdentity", json!(r"C:\Users\private\source")),
    ]);
    for (index, (field, replacement)) in mutations.iter().enumerate() {
        let mut modified = manifest.clone();
        modified[*field] = replacement.clone();
        fs::write(&path, serde_json::to_vec_pretty(&modified).unwrap()).unwrap();
        assert_rejected(
            fixture.command(),
            &fixture.temp.path().join(format!("safety-{index}.json")),
            field,
        );
    }
    fs::write(&path, original).unwrap();
    fixture.assert_sources_unchanged();
}

#[test]
fn applicability_rejects_wrong_duplicate_missing_and_outside_root_source_paths() {
    let mut fixture = Fixture::new();
    for (index, spec) in APPLICABLE.iter().enumerate() {
        let original = fixture.sources[index].clone();
        fixture.sources[index] = fixture.sources[(index + 1) % 5].clone();
        assert_rejected(
            fixture.command(),
            &fixture
                .temp
                .path()
                .join(format!("duplicate-source-{index}.json")),
            "duplicate or substituted source",
        );
        fixture.sources[index] = fixture.temp.path().join("missing-source");
        assert_rejected(
            fixture.command(),
            &fixture
                .temp
                .path()
                .join(format!("absent-source-{index}.json")),
            "missing source directory",
        );
        fixture.sources[index] = fixture::repository()
            .join("artifacts/cs-t0")
            .join(spec.directory);
        assert_rejected(
            fixture.command(),
            &fixture
                .temp
                .path()
                .join(format!("escaped-source-{index}.json")),
            "same bytes outside frozen repository root",
        );
        fixture.sources[index] = original;
    }
    let original = fixture.exit_source.clone();
    fixture.exit_source = fixture.sources[0].clone();
    assert_rejected(
        fixture.command(),
        &fixture.temp.path().join("exit-source-replaced.json"),
        "exit counterevidence cannot be replaced by a positive source",
    );
    fixture.exit_source = original;
    fixture.assert_sources_unchanged();
}

#[test]
fn applicability_historical_snapshot_rejects_changed_or_missing_bytes_without_fallback() {
    let temp = tempfile::TempDir::new().unwrap();
    let snapshot = temp.path().join("disposable-history");
    let relative = "cs-t0/native/src/dllmain.cpp";
    let baseline = read_json(&fixture::repository().join(fixture::BASELINE));
    let binding = baseline["bindings"]
        .as_array()
        .unwrap()
        .iter()
        .find(|binding| binding["path"] == relative)
        .expect("native connection source belongs to the historical baseline");
    let expected = binding["sha256"].as_str().unwrap();
    let real_snapshot = fixture::repository()
        .join("artifacts/cs-t0/t0-05-04-predecessor-v1")
        .join(relative);
    let original = fs::read(&real_snapshot).unwrap();
    let disposable = snapshot.join(relative);
    fs::create_dir_all(disposable.parent().unwrap()).unwrap();
    create_new_bytes(&disposable, &original);
    let good = temp.path().join("valid-copy");
    fixture::copy_historical_baseline_file(&good, &snapshot, relative, expected).unwrap();
    assert_eq!(fs::read(good.join(relative)).unwrap(), original);

    let mut changed = original.clone();
    changed.push(b' ');
    fs::write(&disposable, changed).unwrap();
    let rejected = temp.path().join("rejected-copy");
    let error = fixture::copy_historical_baseline_file(&rejected, &snapshot, relative, expected)
        .expect_err("modified historical bytes must fail before fixture creation");
    assert!(error.contains("hash changed"));
    assert!(!rejected.exists());

    fs::remove_file(&disposable).unwrap();
    assert!(fixture::repository().join(relative).is_file());
    let error = fixture::copy_historical_baseline_file(&rejected, &snapshot, relative, expected)
        .expect_err("missing snapshot must not fall back to the current source tree");
    assert!(error.contains("cannot read frozen snapshot"));
    assert!(!rejected.exists());
    assert_eq!(fs::read(real_snapshot).unwrap(), original);
}

#[test]
fn applicability_checks_every_static_and_recursive_historical_baseline_binding() {
    let fixture = Fixture::new();
    let contract = read_json(&fixture.applicability);
    let baseline = read_json(&fixture.repository_root.join(fixture::BASELINE));
    let mut paths = std::collections::BTreeSet::new();
    paths.insert(fixture::BASELINE.to_owned());
    for bindings in [&contract["staticArtifacts"], &baseline["bindings"]] {
        for binding in bindings.as_array().unwrap() {
            paths.insert(binding["path"].as_str().unwrap().to_owned());
        }
    }
    for (index, relative) in paths.iter().enumerate() {
        let path = fixture.repository_root.join(relative);
        assert!(path.starts_with(fixture.temp.path()));
        let original = fs::read(&path).unwrap();
        let mut changed = original.clone();
        changed.push(b' ');
        fs::write(&path, &changed).unwrap();
        assert_rejected(
            fixture.command(),
            &fixture
                .temp
                .path()
                .join(format!("bound-file-drift-{index}.json")),
            relative,
        );
        assert_eq!(fs::read(&path).unwrap(), changed);
        fs::write(&path, original).unwrap();
    }
    fixture.assert_sources_unchanged();
    eprintln!(
        "PASS applicability recursive frozen-file mutations: {}",
        paths.len()
    );
}

// This synthetic document exists only in TempDir. It tests verification schema
// enforcement, not the truth of a full automatic gate, and must never be used
// as a real authorization or copied into the actual cs-t0 artifact chain.
fn test_only_verification(fixture: &Fixture) -> Value {
    let contract = read_json(&fixture.applicability);
    let artifact_bindings: Vec<_> = contract["verificationArtifactPaths"]
        .as_array()
        .unwrap()
        .iter()
        .map(|path| {
            let relative = path.as_str().unwrap();
            fixture::copy_repository_file(&fixture.repository_root, relative, None);
            json!({"path":relative,"sha256":fixture::hash(&fixture.repository_root.join(relative))})
        })
        .collect();
    let controller = fixture.repository_root.join("out/cs-t0/IsaacCsT0.exe");
    assert!(
        !controller.exists(),
        "current controller is not a frozen historical baseline file"
    );
    fs::create_dir_all(controller.parent().unwrap()).unwrap();
    create_new_bytes(&controller, &fs::read(binary()).unwrap());
    let mut production_bindings = Vec::new();
    for relative in [
        "out/cs-t0/IsaacCsT0.exe",
        "out/cs-t0/IsaacCsT0.Native.dll",
        "out/cs-t0/IsaacCsT0.Helper.exe",
    ] {
        if relative != "out/cs-t0/IsaacCsT0.exe" {
            fixture::copy_repository_file(&fixture.repository_root, relative, None);
        }
        production_bindings.push(json!({"path":relative,"sha256":fixture::hash(&fixture.repository_root.join(relative))}));
    }
    let checks: Vec<_> = contract["requiredAutomaticChecks"]
        .as_array()
        .unwrap()
        .iter()
        .map(|id| json!({"id":id,"passed":true}))
        .collect();
    json!({
        "schemaVersion":5,
        "verificationIdentity":"IsaacCsT0-CoopVerification-v5r7",
        "workPackage":"CS-T0-05-03",
        "createdUtc":"2026-08-31T00:00:00Z",
        "status":"PASS",
        "phase":"PASSIVE_APPLICABILITY_MIGRATION_AUTOMATED_GATE",
        "createNewOnly":true,
        "fullGate":true,
        "predecessorVerification":contract["predecessorVerification"],
        "artifactBindings":artifact_bindings,
        "productionBindings":production_bindings,
        "checks":checks,
        "completedLiveSourceCount":5,
        "notApplicableCount":1,
        "historicalExitResult":"BLOCKED_BY_EVIDENCE",
        "sixSourceGateEligible":false,
        "liveAuthorizationCreated":false,
        "permissions":contract["zeroCapabilities"]
    })
}

#[test]
fn applicability_formal_branch_accepts_only_complete_test_fixture_verification() {
    let fixture = Fixture::new();
    let verification = test_only_verification(&fixture);
    let report_path = fixture
        .temp
        .path()
        .join("TEST-ONLY-NOT-REAL-GATE-verification.json");
    create_new_bytes(
        &report_path,
        &serde_json::to_vec_pretty(&verification).unwrap(),
    );
    let output_path = fixture
        .temp
        .path()
        .join("TEST-ONLY-NOT-REAL-GATE-decision.json");
    let output = fixture
        .command()
        .arg("--verification")
        .arg(&report_path)
        .arg("--output")
        .arg(&output_path)
        .output()
        .unwrap();
    assert!(
        output.status.success(),
        "complete fixture rejected: {}",
        String::from_utf8_lossy(&output.stderr)
    );
    let decision = read_json(&output_path);
    assert_decision_scope(&fixture, &decision, true);
    assert_eq!(decision["status"], "PASS");
    assert_eq!(decision["schemaVersion"], 3);
    assert_eq!(decision["formalPassCount"], 5);
    assert_eq!(decision["notApplicableCount"], 1);
    assert_eq!(decision["csT00504Unlocked"], json!(true));
    for field in [
        "t005Unlocked",
        "activeInputAllowed",
        "passiveHookTransactionAllowed",
        "runtimeMutationAllowed",
    ] {
        assert_eq!(
            decision[field],
            json!(false),
            "formal qualification widened {field}"
        );
    }
    fixture.assert_sources_unchanged();
}

#[test]
fn applicability_formal_branch_rejects_old_partial_forged_and_widened_verifications() {
    let fixture = Fixture::new();
    let verification = test_only_verification(&fixture);
    let report_path = fixture
        .temp
        .path()
        .join("TEST-ONLY-INVALID-verification.json");
    let mut specimens: Vec<(String, Value)> = Vec::new();
    specimens.push((
        "old-v5r6".to_owned(),
        read_json(
            &fixture
                .repository_root
                .join("cs-t0/coop-verification-v5r6.json"),
        ),
    ));
    for pointer in [
        "/schemaVersion",
        "/verificationIdentity",
        "/workPackage",
        "/status",
        "/phase",
        "/createNewOnly",
        "/fullGate",
        "/completedLiveSourceCount",
        "/notApplicableCount",
        "/historicalExitResult",
        "/sixSourceGateEligible",
        "/liveAuthorizationCreated",
        "/predecessorVerification/path",
        "/predecessorVerification/sha256",
    ] {
        let mut changed = verification.clone();
        *changed.pointer_mut(pointer).unwrap() =
            changed_scalar(verification.pointer(pointer).unwrap());
        specimens.push((pointer.to_owned(), changed));
    }
    for field in verification.as_object().unwrap().keys() {
        let mut changed = verification.clone();
        changed.as_object_mut().unwrap().remove(field);
        specimens.push((format!("missing-{field}"), changed));
    }
    let mut changed = verification.clone();
    changed["createdUtc"] = json!("not-an-rfc3339-time");
    specimens.push(("invalid-time".to_owned(), changed));
    for parent in [
        "",
        "/predecessorVerification",
        "/checks/0",
        "/artifactBindings/0",
        "/productionBindings/0",
    ] {
        let mut changed = verification.clone();
        changed
            .pointer_mut(parent)
            .unwrap()
            .as_object_mut()
            .unwrap()
            .insert("unknownField".to_owned(), json!(true));
        specimens.push((format!("unknown-{parent}"), changed));
    }
    for index in 0..verification["checks"].as_array().unwrap().len() {
        let mut changed = verification.clone();
        changed["checks"][index]["passed"] = json!(false);
        specimens.push((format!("failed-check-{index}"), changed));
    }
    for field in ["checks", "artifactBindings", "productionBindings"] {
        let mut changed = verification.clone();
        changed[field].as_array_mut().unwrap().pop();
        specimens.push((format!("partial-{field}"), changed));
        let mut changed = verification.clone();
        let first = changed[field][0].clone();
        changed[field].as_array_mut().unwrap().push(first);
        specimens.push((format!("duplicate-{field}"), changed));
        let mut changed = verification.clone();
        changed[field].as_array_mut().unwrap().swap(0, 1);
        specimens.push((format!("reordered-{field}"), changed));
    }
    for kind in ["artifactBindings", "productionBindings"] {
        for index in 0..verification[kind].as_array().unwrap().len() {
            for field in ["path", "sha256"] {
                let mut changed = verification.clone();
                changed[kind][index][field] = changed_scalar(&verification[kind][index][field]);
                specimens.push((format!("{kind}-{index}-{field}"), changed));
            }
        }
    }
    for field in verification["permissions"].as_object().unwrap().keys() {
        let mut changed = verification.clone();
        changed["permissions"][field] = json!(true);
        specimens.push((format!("capability-{field}"), changed));
        let mut changed = verification.clone();
        changed["permissions"]
            .as_object_mut()
            .unwrap()
            .remove(field);
        specimens.push((format!("omitted-capability-{field}"), changed));
    }
    let mut changed = verification.clone();
    changed["permissions"]["futureCapability"] = json!(false);
    specimens.push(("unknown-capability".to_owned(), changed));
    for (index, (name, specimen)) in specimens.iter().enumerate() {
        fs::write(&report_path, serde_json::to_vec_pretty(specimen).unwrap()).unwrap();
        let before = fixture::hash(&report_path);
        let mut command = fixture.command();
        command.arg("--verification").arg(&report_path);
        assert_rejected(
            command,
            &fixture
                .temp
                .path()
                .join(format!("verification-{index}.json")),
            name,
        );
        assert_eq!(fixture::hash(&report_path), before);
    }
    let duplicate =
        serde_json::to_string(&verification)
            .unwrap()
            .replacen('{', "{\"schemaVersion\":5,", 1);
    fs::write(&report_path, duplicate).unwrap();
    let mut command = fixture.command();
    command.arg("--verification").arg(&report_path);
    assert_rejected(
        command,
        &fixture
            .temp
            .path()
            .join("duplicate-verification-field.json"),
        "duplicate verification field",
    );
    fixture.assert_sources_unchanged();
    eprintln!(
        "PASS applicability verification field mutations: {}",
        specimens.len() + 1
    );
}

#[test]
fn applicability_formal_verification_rejects_executable_substitution_and_duplicate_permissions() {
    let fixture = Fixture::new();
    let verification = test_only_verification(&fixture);
    let report_path = fixture
        .temp
        .path()
        .join("TEST-ONLY-ADVERSARIAL-verification.json");
    let controller = fixture.repository_root.join("out/cs-t0/IsaacCsT0.exe");
    let original_controller = fs::read(&controller).unwrap();
    fs::write(
        &controller,
        b"different controller with self-consistent file hash",
    )
    .unwrap();
    let mut forged = verification.clone();
    forged["productionBindings"][0]["sha256"] = json!(fixture::hash(&controller));
    fs::write(&report_path, serde_json::to_vec_pretty(&forged).unwrap()).unwrap();
    let mut command = fixture.command();
    command.arg("--verification").arg(&report_path);
    assert_rejected(
        command,
        &fixture
            .temp
            .path()
            .join("different-current-executable.json"),
        "self-consistent replacement must not equal the currently running verified CLI",
    );
    fs::write(&controller, original_controller).unwrap();

    // JSON objects with duplicate security keys have ambiguous interpretations.
    // A true capability must not disappear behind a later false map value.
    for (index, field) in verification["permissions"]
        .as_object()
        .unwrap()
        .keys()
        .enumerate()
    {
        let raw = serde_json::to_string(&verification).unwrap().replacen(
            "\"permissions\":{",
            &format!("\"permissions\":{{\"{field}\":true,"),
            1,
        );
        fs::write(&report_path, raw).unwrap();
        let mut command = fixture.command();
        command.arg("--verification").arg(&report_path);
        assert_rejected(
            command,
            &fixture
                .temp
                .path()
                .join(format!("duplicate-permission-{index}.json")),
            field,
        );
    }
    fixture.assert_sources_unchanged();
}

#[test]
fn applicability_powershell_invocation_preserves_distinct_sources_and_cannot_publish() {
    fn tree_snapshot(root: &Path) -> std::collections::BTreeMap<std::path::PathBuf, String> {
        fn visit(
            root: &Path,
            directory: &Path,
            entries: &mut std::collections::BTreeMap<std::path::PathBuf, String>,
        ) {
            for entry in fs::read_dir(directory).unwrap() {
                let entry = entry.unwrap();
                let path = entry.path();
                let file_type = entry.file_type().unwrap();
                assert!(
                    !file_type.is_symlink(),
                    "fixture must not alias historical files"
                );
                let relative = path.strip_prefix(root).unwrap().to_path_buf();
                if file_type.is_dir() {
                    entries.insert(relative, "DIRECTORY".to_owned());
                    visit(root, &path, entries);
                } else {
                    assert!(file_type.is_file());
                    entries.insert(relative, fixture::hash(&path));
                }
            }
        }
        let mut entries = std::collections::BTreeMap::new();
        visit(root, root, &mut entries);
        entries
    }

    fn export_command(fixture: &Fixture) -> Command {
        let mut command = Command::new("pwsh");
        command
            .arg("-NoProfile")
            .arg("-File")
            .arg(fixture::repository().join("cs-t0/verify-coop-passive-applicability.ps1"))
            .arg("-GameExe")
            .arg("TEST_ONLY_UNUSED_GAME_EXE")
            .arg("-RepositoryRoot")
            .arg(&fixture.repository_root)
            .arg("-TestOnlyExportInvocation");
        command
    }

    fn assert_pairs(actual: &[String], expected_command: &str, expected: &[(&str, &Path)]) {
        assert_eq!(actual.len(), 1 + expected.len() * 2);
        assert_eq!(actual[0], expected_command);
        for (pair, (flag, path)) in actual[1..].chunks_exact(2).zip(expected) {
            assert_eq!(&pair[0], flag, "source option was merged with its value");
            assert!(Path::new(&pair[1]).is_absolute());
            assert!(pair[1].contains("coop applicability argv "));
            assert_eq!(
                Path::new(&pair[1]).canonicalize().unwrap(),
                path.canonicalize().unwrap(),
                "export changed the complete path for {flag}"
            );
        }
    }

    let fixture = Fixture::with_spaces();
    assert!(fixture.repository_root.to_string_lossy().contains(' '));
    let before_export = tree_snapshot(&fixture.repository_root);
    let output = export_command(&fixture)
        .output()
        .expect("export public verifier invocation");
    assert!(
        output.status.success(),
        "test-only export failed: {}",
        String::from_utf8_lossy(&output.stderr)
    );
    let exported: Value = serde_json::from_slice(&output.stdout)
        .expect("test-only export must be one JSON document without tool output");
    let fields: std::collections::BTreeSet<_> = exported
        .as_object()
        .unwrap()
        .keys()
        .map(String::as_str)
        .collect();
    assert_eq!(
        fields,
        std::collections::BTreeSet::from([
            "schemaVersion",
            "identity",
            "status",
            "verificationEligible",
            "liveAuthorizationCreated",
            "gateArguments",
            "legacyArguments",
        ])
    );
    assert_eq!(exported["schemaVersion"], 1);
    assert_eq!(
        exported["identity"],
        "IsaacCsT0-CoopPassiveGateInvocation-TestOnly-v1"
    );
    assert_eq!(exported["status"], "TEST_ONLY_INVOCATION_NOT_VERIFIED");
    assert_eq!(exported["verificationEligible"], json!(false));
    assert_eq!(exported["liveAuthorizationCreated"], json!(false));
    assert_eq!(tree_snapshot(&fixture.repository_root), before_export);

    let gate: Vec<String> = serde_json::from_value(exported["gateArguments"].clone()).unwrap();
    let legacy: Vec<String> = serde_json::from_value(exported["legacyArguments"].clone()).unwrap();
    assert_eq!(gate.len(), 17);
    assert_eq!(legacy.len(), 13);
    let mut new_expected: Vec<(&str, &Path)> = vec![
        ("--repository-root", &fixture.repository_root),
        ("--applicability", &fixture.applicability),
        ("--native-exit-source", &fixture.exit_source),
    ];
    let flags: Vec<String> = APPLICABLE
        .iter()
        .map(|spec| format!("--{}", spec.scenario))
        .collect();
    for (flag, source) in flags.iter().zip(&fixture.sources) {
        new_expected.push((flag, source));
    }
    assert_pairs(&gate, "coop-passive-applicability-gate", &new_expected);
    let mut old_expected: Vec<(&str, &Path)> = flags
        .iter()
        .zip(&fixture.sources)
        .map(|(flag, source)| (flag.as_str(), source.as_path()))
        .collect();
    old_expected.push(("--native-exit-passive", &fixture.exit_source));
    assert_pairs(&legacy, "coop-passive-gate", &old_expected);

    let forbidden_verification = fixture
        .temp
        .path()
        .join("MUST-NOT-BE-CREATED-verification.json");
    for combination in 0..4 {
        let mut command = export_command(&fixture);
        match combination {
            0 => {
                command.arg("-PublishFinalArtifacts");
            }
            1 => {
                command.arg("-PublishFinalArtifacts:$false");
            }
            2 => {
                command
                    .arg("-VerificationOutput")
                    .arg(&forbidden_verification);
            }
            3 => {
                command.arg("-VerificationOutput").arg("");
            }
            _ => unreachable!(),
        }
        let rejected = command.output().unwrap();
        assert!(
            !rejected.status.success(),
            "test-only export accepted publishing combination {combination}"
        );
        assert!(!forbidden_verification.exists());
        assert_eq!(
            tree_snapshot(&fixture.repository_root),
            before_export,
            "export/publish combination changed files or directories"
        );
    }

    // Invoke the actual CLI with the exported string array, not a reconstructed
    // command line. Spaces must survive each PowerShell-to-JSON-to-process step.
    let new_output = fixture.temp.path().join("new gate evidence only.json");
    let current = Command::new(binary())
        .args(&gate)
        .arg("--output")
        .arg(&new_output)
        .output()
        .expect("execute exported new gate arguments");
    assert!(
        current.status.success(),
        "exported new gate failed: {}",
        String::from_utf8_lossy(&current.stderr)
    );
    let decision = read_json(&new_output);
    assert_eq!(decision["status"], "PASS_EVIDENCE_ONLY");
    assert_eq!(decision["csT00504Unlocked"], json!(false));
    assert_decision_scope(&fixture, &decision, false);

    let legacy_output = fixture.temp.path().join("legacy gate remains blocked.json");
    let old = Command::new(binary())
        .args(&legacy)
        .arg("--output")
        .arg(&legacy_output)
        .output()
        .expect("execute exported six-source gate arguments");
    assert_eq!(
        old.status.code(),
        Some(1),
        "legacy invocation had a CLI parsing failure: {}",
        String::from_utf8_lossy(&old.stderr)
    );
    assert!(
        legacy_output.exists(),
        "legacy six-source evidence rejection must produce its decision"
    );
    let legacy_decision = read_json(&legacy_output);
    assert_eq!(legacy_decision["status"], "BLOCKED_BY_EVIDENCE");
    assert_eq!(legacy_decision["csT00504Unlocked"], json!(false));
    fixture.assert_sources_unchanged();
}
