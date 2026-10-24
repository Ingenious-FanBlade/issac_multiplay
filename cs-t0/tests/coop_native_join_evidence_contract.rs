//! Independent black-box checks for the intentionally blocked join prerequisite.
//! No fixture here can authorize an active run. All mutations are disposable.

use std::collections::BTreeMap;
use std::fs;
use std::path::{Path, PathBuf};
use std::process::{Command, Output};

use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use tempfile::TempDir;

const STATIC: &str = "cs-t0/coop-native-join-static-v1.json";
const CATALOG: &str = "cs-t0/coop-native-join-catalog-v1.json";
const PREDECESSOR: &str = "cs-t0/coop-native-join-predecessor-v1.json";
const PLAN: &str = "cs-t0/coop-plan-v6.json";
const TARGET_SHA256: &str = "1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C";
const SNAPSHOT: &str = "artifacts/cs-t0/t0-05-04-predecessor-v1";
const LIVE10: &str = "artifacts/cs-t0/20260831T055322Z-t0-05-03-device-rebind-passive-t0-05-03-device-rebind-passive-live-10";

fn repository() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .parent()
        .unwrap()
        .to_path_buf()
}

fn binary() -> &'static str {
    env!("CARGO_BIN_EXE_IsaacCsT0")
}

fn hash(path: &Path) -> String {
    hex::encode_upper(Sha256::digest(fs::read(path).unwrap()))
}

fn read(path: &Path) -> Value {
    serde_json::from_slice(&fs::read(path).unwrap()).unwrap()
}

fn target_case(name: &str, test: impl FnOnce(&Path)) {
    let Some(path) = std::env::var_os("ISAAC_CS_T0_NATIVE_JOIN_GAME_EXE") else {
        eprintln!(
            "NOT_EXECUTED native-join-target-case {name}: verifier must supply the disk target"
        );
        return;
    };
    let target = PathBuf::from(path);
    assert_eq!(hash(&target), TARGET_SHA256, "independent target SHA");
    test(&target);
    eprintln!("EXECUTED native-join-target-case {name}");
}

fn copy(source: &Path, destination: &Path) {
    fs::create_dir_all(destination.parent().unwrap()).unwrap();
    assert!(!destination.exists());
    fs::copy(source, destination).unwrap();
}

struct Fixture {
    temp: TempDir,
}

impl Fixture {
    fn new() -> Self {
        let temp = tempfile::Builder::new()
            .prefix("native join evidence ")
            .tempdir()
            .unwrap();
        let source_root = repository();
        for relative in [STATIC, CATALOG, PREDECESSOR, PLAN] {
            copy(&source_root.join(relative), &temp.path().join(relative));
        }
        let plan = read(&temp.path().join(PLAN));
        for binding in plan["artifactBindings"].as_array().unwrap() {
            let relative = binding["path"].as_str().unwrap();
            copy(&source_root.join(relative), &temp.path().join(relative));
        }
        let evidence = read(&temp.path().join(STATIC));
        for binding in evidence["predecessorSources"].as_array().unwrap() {
            let relative = binding["path"].as_str().unwrap();
            let source = source_root.join(relative);
            assert_eq!(hash(&source), binding["sha256"].as_str().unwrap());
            if !temp.path().join(relative).exists() {
                copy(&source, &temp.path().join(relative));
            }
        }
        let predecessor = read(&temp.path().join(PREDECESSOR));
        assert_eq!(predecessor["snapshotRoot"], SNAPSHOT);
        assert_eq!(predecessor["bindings"].as_array().unwrap().len(), 164);
        for binding in predecessor["bindings"].as_array().unwrap() {
            let relative = Path::new(SNAPSHOT).join(binding["path"].as_str().unwrap());
            let source = source_root.join(&relative);
            assert_eq!(hash(&source), binding["sha256"].as_str().unwrap());
            copy(&source, &temp.path().join(relative));
        }
        fs::create_dir_all(temp.path().join("out/cs-t0")).unwrap();
        Self { temp }
    }

    fn root(&self) -> &Path {
        self.temp.path()
    }

    fn command(&self, target: &Path) -> Command {
        let mut command = Command::new(binary());
        command
            .args(["coop-join-preflight", "--game-exe"])
            .arg(target)
            .arg("--repository-root")
            .arg(self.root());
        command
    }

    fn output_path(&self, name: &str) -> PathBuf {
        self.root().join("out/cs-t0").join(name)
    }

    fn reject(&self, target: &Path, context: &str) {
        let output_path = self.output_path("must-not-exist.json");
        assert!(!output_path.exists());
        let output = self
            .command(target)
            .arg("--output")
            .arg(&output_path)
            .output()
            .unwrap();
        assert_eq!(output.status.code(), Some(1), "{context}: {output:?}");
        assert!(
            output.stdout.is_empty(),
            "invalid evidence must not produce a report: {context}"
        );
        assert!(
            !output_path.exists(),
            "invalid evidence wrote a report: {context}"
        );
    }
}

fn tree(root: &Path) -> BTreeMap<PathBuf, String> {
    fn visit(root: &Path, path: &Path, result: &mut BTreeMap<PathBuf, String>) {
        for entry in fs::read_dir(path).unwrap() {
            let path = entry.unwrap().path();
            if path.is_dir() {
                visit(root, &path, result);
            } else {
                result.insert(path.strip_prefix(root).unwrap().to_path_buf(), hash(&path));
            }
        }
    }
    let mut result = BTreeMap::new();
    visit(root, root, &mut result);
    result
}

fn blocked(output: Output) -> Value {
    assert_eq!(
        output.status.code(),
        Some(1),
        "blocked prerequisite must never exit PASS: {output:?}"
    );
    let report: Value = serde_json::from_slice(&output.stdout).unwrap_or_else(|error| {
        panic!(
            "blocked static JSON report: {error}; stderr={}",
            String::from_utf8_lossy(&output.stderr)
        )
    });
    assert_eq!(report["schemaVersion"], 1);
    assert_eq!(report["identity"], "IsaacCsT0-CoopNativeJoinPreflight-v1");
    assert_eq!(report["workPackage"], "CS-T0-05-04");
    assert_eq!(report["status"], "BLOCKED_BY_EVIDENCE");
    assert_eq!(report["phase"], "STATIC_PREREQUISITE");
    assert_eq!(report["targetSha256"], TARGET_SHA256);
    assert_eq!(report["staticAnchorsVerified"], 53);
    assert_eq!(report["catalogSlotCount"], 55);
    assert_eq!(report["callbackOperandsVerified"], 2);
    assert_eq!(report["frozenPredecessorFilesVerified"], 164);
    assert_eq!(report["historicalPassSourcesReplayed"], 5);
    assert_eq!(report["historicalExitResult"], "BLOCKED_BY_EVIDENCE");
    for field in [
        "runtimeImplemented",
        "runtimeComplete",
        "processInspected",
        "mappingCreated",
        "nativeInjected",
        "armEligible",
        "claimConsumed",
        "activeJoinEligible",
        "liveAuthorizationCreated",
        "csT00505Unlocked",
        "t005Unlocked",
    ] {
        assert_eq!(report[field], false, "blocked evidence widened {field}");
    }
    let mut missing: Vec<_> = report["missingEvidence"]
        .as_array()
        .unwrap()
        .iter()
        .map(|value| value.as_str().unwrap())
        .collect();
    missing.sort();
    assert_eq!(
        missing,
        vec![
            "ACTIVE_RUNTIME_INDEPENDENT_VERIFICATION_NOT_COMPLETE",
            "ACTIVE_RUNTIME_NOT_IMPLEMENTED",
            "FRESH_SINGLE_RUN_AUTHORIZATION_NOT_ISSUED",
            "PERSISTENCE_DISK_CLOUD_STEAM_EXCLUSION_UNPROVEN",
            "PERSISTENCE_INIT_TAIL_WRITE_CLOSURE_UNPROVEN",
            "PERSISTENCE_NATIVE_NONWRITING_SCENARIO_UNPROVEN",
            "PREJOIN_INDEPENDENT_ACTIVE_OFFLINE_UNPROVEN",
        ]
    );
    assert!(
        !report.to_string().contains("isaac-ng.exe"),
        "report leaked the target path"
    );
    report
}

#[test]
fn join_preflight_valid_target_is_blocked_without_process_access() {
    target_case("valid", |target| {
        let fixture = Fixture::new();
        let before = tree(fixture.root());
        let report = blocked(fixture.command(target).output().unwrap());
        assert_eq!(
            tree(fixture.root()),
            before,
            "stdout-only preflight changed files"
        );
        for (field, relative) in [
            ("staticEvidenceSha256", STATIC),
            ("catalogSha256", CATALOG),
            ("predecessorSha256", PREDECESSOR),
        ] {
            assert_eq!(report[field], hash(&fixture.root().join(relative)));
        }
    });
}

#[test]
fn join_preflight_same_target_copy_preserves_static_result() {
    target_case("target-copy", |target| {
        let fixture = Fixture::new();
        let copied = fixture.root().join("disk target with spaces.exe");
        copy(target, &copied);
        let first = blocked(fixture.command(target).output().unwrap());
        let second = blocked(fixture.command(&copied).output().unwrap());
        assert_eq!(
            first, second,
            "same disk bytes must not need a running process/image name"
        );
        assert_eq!(hash(&copied), TARGET_SHA256);
    });
}

#[test]
fn join_preflight_rejects_wrong_target_and_directory() {
    target_case("wrong-target", |target| {
        let fixture = Fixture::new();
        blocked(fixture.command(target).output().unwrap());
        let target_copy = fixture.root().join("changed-target.exe");
        let mut bytes = fs::read(target).unwrap();
        bytes.push(0);
        fs::write(&target_copy, bytes).unwrap();
        fixture.reject(&target_copy, "valid PE with different exact hash");
        fixture.reject(fixture.root(), "directory is not target PE");
        fixture.reject(&fixture.root().join("absent.exe"), "missing target");
        fs::write(&target_copy, b"not PE").unwrap();
        fixture.reject(&target_copy, "malformed target");
    });
}

#[test]
fn join_preflight_output_is_create_new_and_never_source() {
    target_case("output", |target| {
        let fixture = Fixture::new();
        let output = fixture.output_path("blocked-report.json");
        let report = blocked(
            fixture
                .command(target)
                .arg("--output")
                .arg(&output)
                .output()
                .unwrap(),
        );
        assert_eq!(read(&output), report);
        let original = fs::read(&output).unwrap();
        let duplicate = fixture
            .command(target)
            .arg("--output")
            .arg(&output)
            .output()
            .unwrap();
        assert_eq!(duplicate.status.code(), Some(1));
        assert!(duplicate.stdout.is_empty());
        assert_eq!(fs::read(&output).unwrap(), original);
        let sources = read(
            &fixture
                .root()
                .join(SNAPSHOT)
                .join("cs-t0/coop-native-exit-applicability-v1.json"),
        );
        let mut forbidden = vec![
            fixture.root().join("cs-t0/new.json"),
            fixture.root().join(SNAPSHOT).join("new.json"),
            fixture.root().join("out/cs-t0/missing-parent/new.json"),
        ];
        for source in sources["sources"]
            .as_array()
            .unwrap()
            .iter()
            .chain(std::iter::once(&sources["historicalExit"]))
        {
            let parent = fixture.root().join(source["sourcePath"].as_str().unwrap());
            fs::create_dir_all(&parent).unwrap();
            forbidden.push(parent.join("new.json"));
        }
        let old_failed = fixture.root().join(LIVE10);
        fs::create_dir_all(&old_failed).unwrap();
        copy(
            &fixture
                .root()
                .join(SNAPSHOT)
                .join(LIVE10)
                .join("manifest.json"),
            &old_failed.join("manifest.json"),
        );
        forbidden.push(old_failed.join("new.json"));
        let unknown_source = fixture
            .root()
            .join("artifacts/cs-t0/future-existing-source");
        fs::create_dir_all(&unknown_source).unwrap();
        fs::write(unknown_source.join("manifest.json"), b"{}").unwrap();
        forbidden.push(unknown_source.join("new.json"));
        let nested_source = unknown_source.join("nested");
        fs::create_dir_all(&nested_source).unwrap();
        forbidden.push(nested_source.join("new.json"));
        let ads_base = fixture.output_path("ads-base.json");
        fs::write(&ads_base, b"untouched base file").unwrap();
        let ads = fixture.output_path("ads-base.json:diagnostic");
        forbidden.push(ads.clone());
        for name in [
            "NUL",
            "NUL.json",
            "nul.JSON",
            "CON",
            "CON.txt",
            "con.json",
            "CONIN$.json",
            "CONOUT$.json",
            "AUX",
            "PRN.json",
            "COM1",
            "COM9.txt",
            "com1.json",
            "COM¹.json",
            "LPT1",
            "LPT9.json",
            "LPT².json",
            "diagnostic.json.",
            "diagnostic.json ",
        ] {
            forbidden.push(fixture.output_path(name));
        }
        for path in forbidden {
            let result = fixture
                .command(target)
                .arg("--output")
                .arg(&path)
                .output()
                .unwrap();
            assert_eq!(
                result.status.code(),
                Some(1),
                "forbidden output {}",
                path.display()
            );
            assert!(
                result.stdout.is_empty(),
                "forbidden output reported success {}",
                path.display()
            );
            assert!(
                !path.exists(),
                "wrote into forbidden output {}",
                path.display()
            );
        }
        assert_eq!(fs::read(&ads_base).unwrap(), b"untouched base file");
        assert!(fs::read(&ads).is_err(), "alternate data stream was created");
    });
}

#[test]
fn join_preflight_rejects_cli_scope_and_duplicate_options() {
    let root = TempDir::new().unwrap();
    let base = [
        "coop-join-preflight",
        "--game-exe",
        "nonexistent.exe",
        "--repository-root",
    ];
    for flag in [
        "--arm-token",
        "--probe",
        "--active",
        "--ignore-persistence",
        "--scenario",
        "--game-exe",
        "--repository-root",
    ] {
        let output = Command::new(binary())
            .args(base)
            .arg(root.path())
            .args([flag, "forbidden"])
            .output()
            .unwrap();
        assert_eq!(
            output.status.code(),
            Some(2),
            "unexpected acceptance of {flag}"
        );
        assert!(output.stdout.is_empty());
    }
    for args in [
        vec!["coop-join-preflight"],
        vec!["coop-join-preflight", "--game-exe", "x"],
        vec!["coop-join-preflight", "--repository-root", "x"],
        vec!["coop-join-preflight", "--game-exe"],
    ] {
        let output = Command::new(binary()).args(args).output().unwrap();
        assert_eq!(output.status.code(), Some(2));
        assert!(output.stdout.is_empty());
    }
    assert!(tree(root.path()).is_empty());
}

#[test]
fn join_preflight_rejects_catalog_static_and_predecessor_mutations() {
    target_case("contract-mutations", |target| {
        let fixture = Fixture::new();
        blocked(fixture.command(target).output().unwrap());
        for relative in [STATIC, CATALOG, PREDECESSOR, PLAN] {
            let path = fixture.root().join(relative);
            let original = fs::read(&path).unwrap();
            let mut whitespace = original.clone();
            whitespace.push(b' ');
            fs::write(&path, whitespace).unwrap();
            fixture.reject(target, &format!("exact byte drift: {relative}"));
            let mut duplicate = b"{\"schemaVersion\":1,".to_vec();
            duplicate.extend_from_slice(&original[1..]);
            fs::write(&path, duplicate).unwrap();
            fixture.reject(target, &format!("duplicate JSON key: {relative}"));
            fs::write(&path, &original).unwrap();
            let mut document: Value = serde_json::from_slice(&original).unwrap();
            document["activeJoinEligible"] = json!(true);
            document["runtimeComplete"] = json!(true);
            document["missingEvidence"] = json!([]);
            fs::write(&path, serde_json::to_vec(&document).unwrap()).unwrap();
            fixture.reject(target, &format!("fabricated readiness: {relative}"));
            fs::write(path, original).unwrap();
        }
    });
}

#[test]
fn join_preflight_rejects_snapshot_and_plan_binding_mutations() {
    target_case("binding-mutations", |target| {
        let fixture = Fixture::new();
        blocked(fixture.command(target).output().unwrap());
        let predecessor = read(&fixture.root().join(PREDECESSOR));
        let plan = read(&fixture.root().join(PLAN));
        let selected = [
            "cs-t0/native/src/dllmain.cpp",
            "out/cs-t0/IsaacCsT0.exe",
            "cs-t0/coop-plan-v6.json",
            "cs-t0/coop-native-exit-applicability-v1.json",
        ];
        let mut paths = Vec::new();
        for relative in selected {
            assert!(
                predecessor["bindings"]
                    .as_array()
                    .unwrap()
                    .iter()
                    .any(|b| b["path"] == relative)
            );
            paths.push(fixture.root().join(SNAPSHOT).join(relative));
        }
        let source = predecessor["bindings"]
            .as_array()
            .unwrap()
            .iter()
            .find(|binding| binding["path"].as_str().unwrap().ends_with("events.jsonl"))
            .unwrap();
        paths.push(
            fixture
                .root()
                .join(SNAPSHOT)
                .join(source["path"].as_str().unwrap()),
        );
        for binding in plan["artifactBindings"].as_array().unwrap() {
            paths.push(fixture.root().join(binding["path"].as_str().unwrap()));
        }
        let evidence = read(&fixture.root().join(STATIC));
        for binding in evidence["predecessorSources"].as_array().unwrap() {
            let path = fixture.root().join(binding["path"].as_str().unwrap());
            if !paths.contains(&path) {
                paths.push(path);
            }
        }
        for path in paths {
            let original = fs::read(&path).unwrap();
            let mut changed = original.clone();
            changed.push(b' ');
            fs::write(&path, changed).unwrap();
            fixture.reject(target, "pinned snapshot/plan binding drift");
            fs::remove_file(&path).unwrap();
            fixture.reject(
                target,
                "missing snapshot/plan binding has no current-tree fallback",
            );
            fs::write(path, original).unwrap();
        }
    });
}

#[test]
fn join_legacy_entrypoints_cannot_arm_or_smoke_active() {
    let temp = TempDir::new().unwrap();
    let output = temp.path().join("must-not-exist.json");
    let invocations = [
        vec!["coop-join-arm"],
        vec!["coop-native-join-gate"],
        vec![
            "coop-smoke",
            "--probe",
            "native-join-active",
            "--scenario",
            "native-join-active",
        ],
        vec![
            "input-smoke",
            "--probe",
            "native-join-active",
            "--scenario",
            "native-join-active",
        ],
        vec![
            "input-arm-grant",
            "--offline-prereq",
            "coop-plan-v6.json",
            "--case",
            "join-test",
            "--scenario",
            "native-join-active",
            "--arm-token",
            "1111111111111111111111111111111111111111111111111111111111111111",
        ],
    ];
    for invocation in invocations {
        let result = Command::new(binary())
            .args(&invocation)
            .arg("--output")
            .arg(&output)
            .output()
            .unwrap();
        assert_eq!(
            result.status.code(),
            Some(2),
            "legacy/absent active entry accepted: {invocation:?}"
        );
        assert!(result.stdout.is_empty());
        assert!(!output.exists());
    }
    assert!(tree(temp.path()).is_empty());
}
