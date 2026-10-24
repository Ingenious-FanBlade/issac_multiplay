//! Evidence prerequisite for native join. This module has no process, IPC,
//! injection or arm path. Persistence evidence must close before a runtime can
//! be added; successful static checks never authorize a join.

use std::collections::BTreeSet;
use std::fs::{self, OpenOptions};
use std::io::Write;
use std::path::{Component, Path, PathBuf};

use serde::Serialize;
use serde_json::Value;

use crate::artifact::file_sha256;
use crate::error::{AppError, AppResult, EvidenceContext};
use crate::pe::{PeImage, sha256_hex};

const STATIC_PATH: &str = "cs-t0/coop-native-join-static-v1.json";
const CATALOG_PATH: &str = "cs-t0/coop-native-join-catalog-v1.json";
const PREDECESSOR_PATH: &str = "cs-t0/coop-native-join-predecessor-v1.json";
const STATIC_BYTES: &[u8] = include_bytes!("../coop-native-join-static-v1.json");
const CATALOG_BYTES: &[u8] = include_bytes!("../coop-native-join-catalog-v1.json");
const PREDECESSOR_BYTES: &[u8] = include_bytes!("../coop-native-join-predecessor-v1.json");
const PLAN_BYTES: &[u8] = include_bytes!("../coop-plan-v6.json");
const APPLICABILITY_BYTES: &[u8] = include_bytes!("../coop-native-exit-applicability-v1.json");
const OBSERVED_TARGET: &str = "1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C";

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct JoinPrerequisiteReport {
    pub schema_version: u32,
    pub identity: &'static str,
    pub work_package: &'static str,
    pub status: &'static str,
    pub phase: &'static str,
    pub target_sha256: String,
    pub static_evidence_sha256: String,
    pub catalog_sha256: String,
    pub predecessor_sha256: String,
    pub static_anchors_verified: usize,
    pub catalog_slot_count: usize,
    pub callback_operands_verified: usize,
    pub frozen_predecessor_files_verified: usize,
    pub historical_pass_sources_replayed: usize,
    pub historical_exit_result: String,
    pub missing_evidence: Vec<String>,
    pub runtime_implemented: bool,
    pub runtime_complete: bool,
    pub process_inspected: bool,
    pub mapping_created: bool,
    pub native_injected: bool,
    pub arm_eligible: bool,
    pub claim_consumed: bool,
    pub active_join_eligible: bool,
    pub live_authorization_created: bool,
    pub cs_t00505_unlocked: bool,
    pub t005_unlocked: bool,
}

fn require(condition: bool, message: &str) -> AppResult<()> {
    if condition {
        Ok(())
    } else {
        Err(AppError::evidence(message))
    }
}

fn string<'a>(value: &'a Value, key: &str) -> AppResult<&'a str> {
    value[key]
        .as_str()
        .ok_or_else(|| AppError::evidence(format!("missing string: {key}")))
}

fn array<'a>(value: &'a Value, key: &str) -> AppResult<&'a [Value]> {
    value[key]
        .as_array()
        .map(Vec::as_slice)
        .ok_or_else(|| AppError::evidence(format!("missing array: {key}")))
}

fn parse(bytes: &[u8]) -> AppResult<Value> {
    serde_json::from_slice(bytes).evidence_context("invalid frozen join JSON")
}

fn bound_path(root: &Path, relative: &str) -> AppResult<PathBuf> {
    require(
        !relative.is_empty()
            && !relative.contains('\\')
            && Path::new(relative)
                .components()
                .all(|part| matches!(part, Component::Normal(_))),
        "join binding must be a normalized relative path",
    )?;
    let path = root
        .join(relative)
        .canonicalize()
        .evidence_context("missing join binding")?;
    require(path.starts_with(root), "join binding escapes repository")?;
    let normalized = path
        .strip_prefix(root)
        .expect("checked root")
        .to_string_lossy()
        .replace('\\', "/");
    require(
        normalized == relative,
        "join binding aliases its frozen path",
    )?;
    Ok(path)
}

fn check_bytes(root: &Path, relative: &str, expected: &[u8]) -> AppResult<()> {
    let path = bound_path(root, relative)?;
    require(
        fs::read(path).evidence_context("cannot read join binding")? == expected,
        "join contract differs from compiled evidence; no permissions granted",
    )
}

fn check_binding(root: &Path, binding: &Value) -> AppResult<()> {
    let path = bound_path(root, string(binding, "path")?)?;
    require(
        file_sha256(&path)? == string(binding, "sha256")?,
        "frozen join predecessor hash mismatch",
    )
}

fn rva(text: &str) -> AppResult<u32> {
    text.strip_prefix("0x")
        .and_then(|value| u32::from_str_radix(value, 16).ok())
        .ok_or_else(|| AppError::evidence("invalid frozen RVA"))
}

fn verify_history(root: &Path, predecessor: &Value, plan: &Value) -> AppResult<PathBuf> {
    check_bytes(root, PREDECESSOR_PATH, PREDECESSOR_BYTES)?;
    check_bytes(root, "cs-t0/coop-plan-v6.json", PLAN_BYTES)?;
    for binding in array(plan, "artifactBindings")? {
        check_binding(root, binding)?;
    }
    let snapshot = bound_path(root, string(predecessor, "snapshotRoot")?)?;
    require(
        snapshot.is_dir(),
        "join predecessor snapshot is not a directory",
    )?;
    let mut paths = BTreeSet::new();
    for binding in array(predecessor, "bindings")? {
        require(
            paths.insert(string(binding, "path")?),
            "duplicate predecessor binding",
        )?;
        check_binding(&snapshot, binding)?;
    }
    require(
        paths.len() == 164 && predecessor["baselineFileCount"] == 149,
        "join predecessor does not preserve the complete frozen baseline",
    )?;
    check_bytes(&snapshot, "cs-t0/coop-plan-v6.json", PLAN_BYTES)?;
    check_bytes(
        &snapshot,
        "cs-t0/coop-native-exit-applicability-v1.json",
        APPLICABILITY_BYTES,
    )?;
    Ok(snapshot)
}

fn replay_history(snapshot: &Path) -> AppResult<(usize, String)> {
    let applicability = parse(APPLICABILITY_BYTES)?;
    let sources = array(&applicability, "sources")?;
    require(
        sources.len() == 5,
        "expected five applicable predecessor sources",
    )?;
    let mut pass_count = 0;
    let mut exit_result = String::new();
    for expected in sources
        .iter()
        .chain(std::iter::once(&applicability["historicalExit"]))
    {
        let source = bound_path(snapshot, string(expected, "sourcePath")?)?;
        let fresh = crate::coop_passive::replay_passive_run(&source)?;
        require(
            fresh.result == string(expected, "expectedResult")?
                && fresh.scenario == string(expected, "scenario")?
                && fresh.artifact_identity == string(expected, "artifactIdentity")?,
            "join predecessor replay changed the historical result",
        )?;
        if fresh.scenario == "native-exit-passive" {
            exit_result = fresh.result;
        } else {
            require(fresh.result == "PASS", "applicable source is not PASS")?;
            pass_count += 1;
        }
    }
    require(
        pass_count == 5 && exit_result == "BLOCKED_BY_EVIDENCE",
        "historical applicability scope changed",
    )?;
    Ok((pass_count, exit_result))
}

fn validate_output(root: &Path, snapshot: &Path, output: &Path) -> AppResult<()> {
    // Win32 interprets ':' as an alternate stream and reserved device names
    // independently of the parent directory. CreateNew alone cannot establish
    // that the requested diagnostic is a new ordinary file.
    let name = output
        .file_name()
        .and_then(|name| name.to_str())
        .ok_or_else(|| AppError::evidence("join output must name a new JSON file"))?;
    require(
        name.len() > 5
            && name.to_ascii_lowercase().ends_with(".json")
            && name
                .bytes()
                .all(|byte| byte.is_ascii_alphanumeric() || b"._-".contains(&byte)),
        "join output requires a simple ASCII JSON filename; streams and aliases are forbidden",
    )?;
    let base = name
        .split('.')
        .next()
        .unwrap_or_default()
        .to_ascii_uppercase();
    require(
        !matches!(base.as_str(), "CON" | "PRN" | "AUX" | "NUL")
            && !(base.len() == 4
                && (base.starts_with("COM") || base.starts_with("LPT"))
                && matches!(base.as_bytes()[3], b'1'..=b'9')),
        "join output cannot name a Win32 device",
    )?;
    require(
        !output
            .try_exists()
            .evidence_context("cannot inspect join output")?,
        "join output is create-new only",
    )?;
    let parent = output
        .parent()
        .filter(|p| !p.as_os_str().is_empty())
        .unwrap_or(Path::new("."))
        .canonicalize()
        .evidence_context("join output parent must exist")?;
    require(
        parent.starts_with(root.join("out/cs-t0"))
            || parent.starts_with(root.join("artifacts/cs-t0")),
        "join diagnostic output must remain under repository out/cs-t0 or artifacts/cs-t0",
    )?;
    require(
        !parent.starts_with(snapshot),
        "join output cannot modify frozen history",
    )?;
    // All historical sources are immutable, including failures not listed in
    // the five-source applicability gate. Never add diagnostics beneath one.
    for ancestor in parent.ancestors().take_while(|path| path.starts_with(root)) {
        require(
            !["manifest.json", "events.jsonl", "analysis.json"]
                .iter()
                .any(|name| ancestor.join(name).is_file()),
            "join output cannot modify an existing source directory",
        )?;
    }
    let applicability = parse(APPLICABILITY_BYTES)?;
    for source in array(&applicability, "sources")?
        .iter()
        .chain(std::iter::once(&applicability["historicalExit"]))
    {
        require(
            !parent.starts_with(root.join(string(source, "sourcePath")?)),
            "join output cannot modify a historical source",
        )?;
    }
    Ok(())
}

/// Valid static input produces a BLOCKED report, not an armable preflight.
/// Unknown or mutated evidence is an error and produces no report file.
pub fn preflight(
    game_exe: &Path,
    repository_root: &Path,
    output: Option<&Path>,
) -> AppResult<JoinPrerequisiteReport> {
    let root = repository_root
        .canonicalize()
        .evidence_context("cannot resolve repository root")?;
    check_bytes(&root, STATIC_PATH, STATIC_BYTES)?;
    check_bytes(&root, CATALOG_PATH, CATALOG_BYTES)?;
    let evidence = parse(STATIC_BYTES)?;
    let catalog = parse(CATALOG_BYTES)?;
    let predecessor = parse(PREDECESSOR_BYTES)?;
    let plan = parse(PLAN_BYTES)?;
    require(
        evidence["schemaVersion"] == 1
            && evidence["identity"] == "IsaacCsT0-CoopNativeJoinStatic-v1"
            && evidence["workPackage"] == "CS-T0-05-04",
        "wrong native join static identity",
    )?;
    let image = PeImage::read(game_exe).evidence_context("cannot inspect join target PE")?;
    require(
        image.machine == 0x14c
            && image.image_base == 0x400000
            && sha256_hex(image.bytes()) == OBSERVED_TARGET,
        "join target differs from frozen approved x86 image",
    )?;
    require(
        evidence["target"]["observedSha256"] == OBSERVED_TARGET,
        "static evidence target mismatch",
    )?;
    let snapshot = verify_history(&root, &predecessor, &plan)?;
    for binding in array(&evidence, "predecessorSources")? {
        check_binding(&root, binding)?;
    }
    if let Some(output) = output {
        validate_output(&root, &snapshot, output)?;
    }
    let mut ids = BTreeSet::new();
    let anchors = array(&evidence, "anchors")?;
    require(
        !anchors.is_empty(),
        "join static evidence has no raw anchors",
    )?;
    for anchor in anchors {
        require(
            ids.insert(string(anchor, "id")?),
            "duplicate join static anchor",
        )?;
        let bytes = hex::decode(string(anchor, "expectedBytes")?)
            .evidence_context("invalid anchor bytes")?;
        require(!bytes.is_empty(), "empty join static anchor")?;
        require(
            image
                .bytes_at_rva(rva(string(anchor, "rva")?)?, bytes.len())
                .evidence_context("join anchor is outside disk PE")?
                == bytes,
            "join static anchor bytes differ",
        )?;
    }
    check_binding(&snapshot, &catalog["inheritedTransaction"])?;
    let inherited = parse(
        &fs::read(bound_path(
            &snapshot,
            string(&catalog["inheritedTransaction"], "path")?,
        )?)
        .evidence_context("cannot read inherited catalog")?,
    )?;
    let inherited_slots = array(&inherited["orderedHookTransaction"], "slots")?;
    require(
        inherited_slots.len() == 53
            && inherited_slots
                .iter()
                .enumerate()
                .all(|(i, item)| item["ordinal"] == i),
        "inherited 53-slot catalog is not contiguous",
    )?;
    let slots = array(&catalog, "additionalSlots")?;
    require(
        slots.len() == 2 && catalog["slotCount"] == 55,
        "native join must use exactly 53+2 slots",
    )?;
    for (slot, (ordinal, instruction, action)) in
        slots.iter().zip([(53, 0x5bb057, 19), (54, 0x5bb38b, 14)])
    {
        require(
            slot["ordinal"] == ordinal
                && slot["action"] == action
                && slot["patchKind"] == "PUSH_IMM32_CALLBACK",
            "join callback slot identity changed",
        )?;
        let operand = rva(string(slot, "operandRva")?)?;
        require(
            rva(string(slot, "instructionRva")?)? == instruction
                && operand == instruction + 1
                && operand % 4 == 0,
            "join callback operand is not the frozen aligned PUSH operand",
        )?;
        let bytes = image
            .bytes_at_rva(instruction, 5)
            .evidence_context("callback instruction outside PE")?;
        require(
            bytes == [0x68, 0xe0, 0x09, 0xa2, 0x00]
                && rva(string(slot, "originalTargetRva")?)? == 0x6209e0,
            "join callback instruction or target mismatch",
        )?;
    }
    let (pass_count, exit_result) = replay_history(&snapshot)?;
    let mut missing: Vec<String> = array(&evidence, "missingEvidence")?
        .iter()
        .map(|item| {
            item.as_str()
                .map(str::to_owned)
                .ok_or_else(|| AppError::evidence("invalid missing-evidence code"))
        })
        .collect::<AppResult<_>>()?;
    missing.extend(
        [
            "ACTIVE_RUNTIME_NOT_IMPLEMENTED",
            "ACTIVE_RUNTIME_INDEPENDENT_VERIFICATION_NOT_COMPLETE",
            "FRESH_SINGLE_RUN_AUTHORIZATION_NOT_ISSUED",
        ]
        .map(str::to_owned),
    );
    missing.sort();
    missing.dedup();
    let report = JoinPrerequisiteReport {
        schema_version: 1,
        identity: "IsaacCsT0-CoopNativeJoinPreflight-v1",
        work_package: "CS-T0-05-04",
        status: "BLOCKED_BY_EVIDENCE",
        phase: "STATIC_PREREQUISITE",
        target_sha256: OBSERVED_TARGET.to_owned(),
        static_evidence_sha256: sha256_hex(STATIC_BYTES),
        catalog_sha256: sha256_hex(CATALOG_BYTES),
        predecessor_sha256: sha256_hex(PREDECESSOR_BYTES),
        static_anchors_verified: anchors.len(),
        catalog_slot_count: 55,
        callback_operands_verified: 2,
        frozen_predecessor_files_verified: array(&predecessor, "bindings")?.len(),
        historical_pass_sources_replayed: pass_count,
        historical_exit_result: exit_result,
        missing_evidence: missing,
        runtime_implemented: false,
        runtime_complete: false,
        process_inspected: false,
        mapping_created: false,
        native_injected: false,
        arm_eligible: false,
        claim_consumed: false,
        active_join_eligible: false,
        live_authorization_created: false,
        cs_t00505_unlocked: false,
        t005_unlocked: false,
    };
    // Repeat binding checks after replay; an input changed during analysis cannot
    // acquire a freshly published diagnostic even though that diagnostic is blocked.
    verify_history(&root, &predecessor, &plan)?;
    check_bytes(&root, STATIC_PATH, STATIC_BYTES)?;
    check_bytes(&root, CATALOG_PATH, CATALOG_BYTES)?;
    for binding in array(&evidence, "predecessorSources")? {
        check_binding(&root, binding)?;
    }
    require(
        file_sha256(game_exe)? == OBSERVED_TARGET,
        "join target changed during preflight",
    )?;
    if let Some(output) = output {
        validate_output(&root, &snapshot, output)?;
        let mut bytes = serde_json::to_vec_pretty(&report)
            .evidence_context("cannot serialize join diagnostic")?;
        bytes.push(b'\n');
        let mut file = OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(output)
            .evidence_context("join output is create-new only")?;
        file.write_all(&bytes)
            .evidence_context("cannot write join diagnostic")?;
        file.sync_all()
            .evidence_context("cannot flush join diagnostic")?;
    }
    Ok(report)
}
