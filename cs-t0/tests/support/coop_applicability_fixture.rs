//! Immutable live specimens copied into disposable test directories.
//! No production constructors, analyzers, or gate helpers are used here.

use std::fs::{self, OpenOptions};
use std::io::Write;
use std::path::{Component, Path, PathBuf};
use std::process::{Command, Output};

use serde_json::Value;
use sha2::{Digest, Sha256};
use tempfile::TempDir;

pub const FILES: [&str; 3] = ["manifest.json", "events.jsonl", "analysis.json"];
pub const CONTRACT: &str = "cs-t0/coop-native-exit-applicability-v1.json";
pub const BASELINE: &str = "cs-t0/coop-passive-applicability-baseline-v1.json";
const CONTRACT_SHA256: &str = "23B6E5659B6060A437483831C0C02A37A00EAE11F86CDF972EF980101AB29A6B";
const BASELINE_SHA256: &str = "D56C17F366302A84A097EA40276D19B9966511CB168CCFE94B507A11CFD5EBC1";
const PREDECESSOR: &str = "cs-t0/coop-native-join-predecessor-v1.json";
const PREDECESSOR_SHA256: &str = "480699A1185C75CF89467FBA69C96278FC88E3FF56776E69FACC1C2BC61AF5CA";
const SNAPSHOT_ROOT: &str = "artifacts/cs-t0/t0-05-04-predecessor-v1";

#[derive(Clone, Copy)]
pub struct SourceSpec {
    pub scenario: &'static str,
    pub directory: &'static str,
    pub hashes: [&'static str; 3],
}

pub const APPLICABLE: [SourceSpec; 5] = [
    SourceSpec {
        scenario: "native-join-passive",
        directory: "20260828T072118Z-t0-05-03-native-join-passive-t0-05-03-native-join-passive-live-06",
        hashes: [
            "E268ECE2E43FB2F0834E797E9986D0294A67E03FC347CF39D3801D8F5064B991",
            "BE9AEE8A6AE6C96F919563AFA71C0A60A10E9A0762D7E7C12927F3BFC5CBC6B6",
            "BD2A8C6E8EE18B7D2E0B684062193731B39750DE6835403358AE196827B1FD53",
        ],
    },
    SourceSpec {
        scenario: "death-respawn-passive",
        directory: "20260830T182858Z-t0-05-03-death-respawn-passive-t0-05-03-death-respawn-passive-live-08",
        hashes: [
            "50394427D5217CC7CE7587D8C29AEBB377CA58F0DE7D7EEB5AC0B0BA16E006D2",
            "B7A479B17DE837EB662376220B17FB8207952D2E49A7DF112F98FF48A70154FB",
            "B4113180004E9F765EE88F117D84D095909D230155DB3BF809A45AF828711C78",
        ],
    },
    SourceSpec {
        scenario: "room-rebind-passive",
        directory: "20260830T185508Z-t0-05-03-room-rebind-passive-t0-05-03-room-rebind-passive-live-09",
        hashes: [
            "01FE528ABC6BE9C401A1FA5A8785D37D6DED64482C214FE42B6FB9B777882F0E",
            "5F69A683587DCC63DB73CC0C641A76208D90369B620E45B40C3936BA4C093EA3",
            "FABD4445747902C73B73E7907AC2B9F6A290807F6CAC4182795FC62711E7DACA",
        ],
    },
    SourceSpec {
        scenario: "device-rebind-passive",
        directory: "20260831T065706Z-t0-05-03-device-rebind-passive-t0-05-03-device-rebind-passive-live-11",
        hashes: [
            "53CB7F12A11929CF53DED515F7B2346BCE3EF7AF52CF17442D22FC958074139E",
            "795986B50E0BE59378E20B9CA74C425C3EE49D984A1510C0A45D8BB45DE4F650",
            "E726A3339B539712510BFE9E0676B3AE005496FCBB9F0BB304438DEB5BB8C3F0",
        ],
    },
    SourceSpec {
        scenario: "menu-teardown-passive",
        directory: "20260831T072042Z-t0-05-03-menu-teardown-passive-t0-05-03-menu-teardown-passive-live-12",
        hashes: [
            "AD06F8E6AE60F959717EE54C0833DA216F17444B0B72F0B4A74AE45772420F28",
            "8EFF7DD402CFCA959F9586D166D3FEAD0A141E14BE01A0353D584692D982962C",
            "05FC7430983B76B9919CA5F52EE6F497B110BC343D059E3D7C43320B9CDB24AA",
        ],
    },
];

pub const EXIT: SourceSpec = SourceSpec {
    scenario: "native-exit-passive",
    directory: "20260830T162513Z-t0-05-03-native-exit-passive-t0-05-03-native-exit-passive-live-07",
    hashes: [
        "15AE50FC1A5858C06BAAAE7309C4191FE8E3659E6617A8C7DBFEFAD176C7D262",
        "6322C0F24BEC2406907185BE0E718B96DB3CB16DCC6442137A436F4550243B8D",
        "FCA986F50D05C18F3F8070EE61AB281C7B2CC47292C358010F136D157A70D5B5",
    ],
};

pub fn binary() -> &'static str {
    env!("CARGO_BIN_EXE_IsaacCsT0")
}

pub fn repository() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .parent()
        .expect("crate repository parent")
        .to_path_buf()
}

pub fn hash(path: &Path) -> String {
    hex::encode_upper(Sha256::digest(fs::read(path).expect("read specimen hash")))
}

pub fn read_json(path: &Path) -> Value {
    serde_json::from_slice(&fs::read(path).expect("read specimen JSON"))
        .expect("parse specimen JSON")
}

pub fn create_new_bytes(path: &Path, bytes: &[u8]) {
    OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .expect("create-new disposable output")
        .write_all(bytes)
        .expect("write disposable output");
}

pub fn assert_pinned_source(source: &Path, spec: SourceSpec) {
    for (file, expected) in FILES.into_iter().zip(spec.hashes) {
        assert_eq!(
            hash(&source.join(file)),
            expected,
            "{} {file}",
            spec.scenario
        );
    }
}

pub fn copy_repository_file(root: &Path, relative: &str, expected: Option<&str>) {
    let relative_path = Path::new(relative);
    assert!(
        relative_path
            .components()
            .all(|component| matches!(component, Component::Normal(_))),
        "fixture binding escaped repository: {relative}"
    );
    let original = repository().join(relative_path);
    if let Some(expected) = expected {
        assert_eq!(hash(&original), expected, "frozen input {relative}");
    }
    let destination = root.join(relative_path);
    if destination.exists() {
        assert_eq!(hash(&destination), hash(&original));
    } else {
        fs::create_dir_all(destination.parent().unwrap()).unwrap();
        create_new_bytes(&destination, &fs::read(&original).unwrap());
    }
}

// Only the caller's exact 149 baseline entries use this source. There is no
// current-tree fallback: missing or changed history must fail before copying.
// Static applicability inputs continue through copy_repository_file above.
pub fn copy_historical_baseline_file(
    root: &Path,
    snapshot_root: &Path,
    relative: &str,
    expected: &str,
) -> Result<(), String> {
    let relative_path = Path::new(relative);
    if relative_path.as_os_str().is_empty()
        || !relative_path
            .components()
            .all(|component| matches!(component, Component::Normal(_)))
    {
        return Err(format!("historical binding escaped snapshot: {relative}"));
    }
    let original = snapshot_root.join(relative_path);
    let bytes = fs::read(&original)
        .map_err(|error| format!("cannot read frozen snapshot {relative}: {error}"))?;
    if hex::encode_upper(Sha256::digest(&bytes)) != expected {
        return Err(format!("frozen snapshot hash changed: {relative}"));
    }
    let destination = root.join(relative_path);
    if destination.exists() {
        if hash(&destination) != expected {
            return Err(format!(
                "historical fixture destination differs: {relative}"
            ));
        }
    } else {
        fs::create_dir_all(destination.parent().expect("relative destination parent"))
            .map_err(|error| format!("cannot create historical fixture: {error}"))?;
        create_new_bytes(&destination, &bytes);
    }
    Ok(())
}

fn copy_bound_repository(root: &Path) {
    copy_repository_file(root, CONTRACT, Some(CONTRACT_SHA256));
    copy_repository_file(root, BASELINE, Some(BASELINE_SHA256));
    let predecessor_path = repository().join(PREDECESSOR);
    assert_eq!(hash(&predecessor_path), PREDECESSOR_SHA256);
    let predecessor = read_json(&predecessor_path);
    assert_eq!(predecessor["snapshotRoot"], SNAPSHOT_ROOT);
    assert_eq!(predecessor["baselineFileCount"], 149);
    let historical_bindings = predecessor["bindings"].as_array().unwrap();
    assert_eq!(historical_bindings.len(), 164);
    let snapshot_root = repository().join(SNAPSHOT_ROOT);
    let baseline = read_json(&root.join(BASELINE));
    let bindings = baseline["bindings"].as_array().expect("baseline bindings");
    assert_eq!(bindings.len(), 149, "frozen historical baseline size");
    for binding in bindings {
        let relative = binding["path"].as_str().unwrap();
        let expected = binding["sha256"].as_str().unwrap();
        assert_eq!(
            historical_bindings
                .iter()
                .filter(|item| item["path"] == relative && item["sha256"] == expected)
                .count(),
            1,
            "exact baseline entry must be pinned by the predecessor: {relative}"
        );
        copy_historical_baseline_file(root, &snapshot_root, relative, expected)
            .expect("copy immutable historical baseline input");
    }
    let contract = read_json(&root.join(CONTRACT));
    for binding in contract["staticArtifacts"]
        .as_array()
        .expect("applicability static artifact bindings")
    {
        copy_repository_file(
            root,
            binding["path"].as_str().unwrap(),
            Some(binding["sha256"].as_str().unwrap()),
        );
    }
}

fn copied_source(root: &Path, spec: SourceSpec) -> PathBuf {
    let relative = Path::new("artifacts/cs-t0").join(spec.directory);
    let original = repository().join(&relative);
    assert_pinned_source(&original, spec);
    let copy = root.join(relative);
    assert_pinned_source(&copy, spec);
    copy
}

pub struct Fixture {
    pub temp: TempDir,
    pub repository_root: PathBuf,
    pub applicability: PathBuf,
    pub sources: [PathBuf; 5],
    pub exit_source: PathBuf,
}

impl Fixture {
    pub fn new() -> Self {
        let temp = TempDir::new().expect("disposable applicability fixture");
        Self::from_temp(temp)
    }

    pub fn with_spaces() -> Self {
        let temp = tempfile::Builder::new()
            .prefix("coop applicability argv ")
            .tempdir()
            .expect("disposable repository path containing spaces");
        Self::from_temp(temp)
    }

    fn from_temp(temp: TempDir) -> Self {
        copy_bound_repository(temp.path());
        let sources = APPLICABLE.map(|spec| copied_source(temp.path(), spec));
        let exit_source = copied_source(temp.path(), EXIT);
        let repository_root = temp.path().to_path_buf();
        let applicability = repository_root.join(CONTRACT);
        Self {
            temp,
            repository_root,
            applicability,
            sources,
            exit_source,
        }
    }

    pub fn command(&self) -> Command {
        let mut command = Command::new(binary());
        command
            .arg("coop-passive-applicability-gate")
            .arg("--repository-root")
            .arg(&self.repository_root)
            .arg("--applicability")
            .arg(&self.applicability)
            .arg("--native-exit-source")
            .arg(&self.exit_source);
        for (spec, source) in APPLICABLE.iter().zip(&self.sources) {
            command.arg(format!("--{}", spec.scenario)).arg(source);
        }
        command
    }

    pub fn invoke(&self, output: &Path) -> Output {
        assert!(!output.exists(), "test must choose create-new gate output");
        self.command()
            .arg("--output")
            .arg(output)
            .output()
            .expect("invoke public applicability gate")
    }

    pub fn assert_sources_unchanged(&self) {
        for (source, spec) in self.sources.iter().zip(APPLICABLE) {
            assert_pinned_source(source, spec);
            assert_pinned_source(
                &repository().join("artifacts/cs-t0").join(spec.directory),
                spec,
            );
        }
        assert_pinned_source(&self.exit_source, EXIT);
        assert_pinned_source(
            &repository().join("artifacts/cs-t0").join(EXIT.directory),
            EXIT,
        );
    }
}
