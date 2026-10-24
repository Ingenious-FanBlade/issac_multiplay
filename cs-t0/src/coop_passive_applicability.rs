//! Scoped post-live applicability gate. The legacy six-source gate/analyzers stay immutable.
use std::collections::BTreeSet;
use std::fs::{self, OpenOptions};
use std::io::{BufRead, BufReader, Write};
use std::path::{Component, Path, PathBuf};

use serde::{Deserialize, Serialize};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};

use crate::artifact::{file_sha256, utc_now};
use crate::coop_passive::{PassiveEvent, PassiveManifest, replay_passive_run};
use crate::error::{AppError, AppResult, EvidenceContext};

const CONTRACT: &[u8] = include_bytes!("../coop-native-exit-applicability-v1.json");
const VERIFICATION_ID: &str = "IsaacCsT0-CoopVerification-v5r7";
const VERIFICATION_PHASE: &str = "PASSIVE_APPLICABILITY_MIGRATION_AUTOMATED_GATE";

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
struct Binding {
    path: String,
    sha256: String,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
struct Check {
    id: String,
    passed: bool,
}

// A map would silently take the last value of duplicate security keys.
// Derived struct deserialization rejects duplicates as well as unknown/missing fields.
#[derive(Debug, Eq, PartialEq, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
struct Permissions {
    passive_hook_transaction_allowed: bool,
    passive_observation_allowed: bool,
    active_input_allowed: bool,
    input_mutation_allowed: bool,
    world_mutation_allowed: bool,
    runtime_mutation_allowed: bool,
    override_allowed: bool,
    transport_allowed: bool,
    scheduler_allowed: bool,
    apply_allowed: bool,
    mode_latch_allowed: bool,
    original_online_positive_allowed: bool,
    t005_unlocked: bool,
    cs_t00504_unlocked: bool,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
struct Verification {
    schema_version: u32,
    verification_identity: String,
    work_package: String,
    created_utc: String,
    status: String,
    phase: String,
    create_new_only: bool,
    full_gate: bool,
    predecessor_verification: Binding,
    artifact_bindings: Vec<Binding>,
    production_bindings: Vec<Binding>,
    checks: Vec<Check>,
    completed_live_source_count: u32,
    not_applicable_count: u32,
    historical_exit_result: String,
    six_source_gate_eligible: bool,
    live_authorization_created: bool,
    permissions: Permissions,
}

fn require(ok: bool, detail: &str) -> AppResult<()> {
    if ok {
        Ok(())
    } else {
        Err(AppError::evidence(format!(
            "passive applicability: {detail}"
        )))
    }
}

fn read_json<T: serde::de::DeserializeOwned>(path: &Path) -> AppResult<T> {
    serde_json::from_slice(&fs::read(path).evidence_context("cannot read bound JSON")?)
        .evidence_context("invalid bound JSON schema")
}

fn sha(bytes: &[u8]) -> String {
    hex::encode_upper(Sha256::digest(bytes))
}

fn text(value: &Value, field: &str) -> String {
    value[field]
        .as_str()
        .expect("compiled contract string")
        .to_owned()
}

fn binding(value: &Value) -> Binding {
    serde_json::from_value(value.clone()).expect("compiled contract binding")
}

fn canonical(path: &Path) -> AppResult<PathBuf> {
    path.canonicalize()
        .evidence_context("bound path is missing")
}

// Every persisted path comes from the compiled contract, never a caller's absolute path.
fn bound_path(root: &Path, relative: &str) -> AppResult<PathBuf> {
    require(
        !relative.contains('\\')
            && !relative.is_empty()
            && Path::new(relative)
                .components()
                .all(|part| matches!(part, Component::Normal(_))),
        "binding must be a normalized repository-relative path",
    )?;
    let path = canonical(&root.join(relative))?;
    require(path.starts_with(root), "binding escapes repository root")?;
    let actual = path
        .strip_prefix(root)
        .expect("checked root")
        .to_string_lossy()
        .replace('\\', "/");
    require(
        actual == relative,
        "binding redirects or aliases its frozen path",
    )?;
    Ok(path)
}

fn check_binding(root: &Path, expected: &Binding) -> AppResult<()> {
    let path = bound_path(root, &expected.path)?;
    require(
        file_sha256(&path)? == expected.sha256,
        "frozen file hash changed",
    )
}

fn verify_static_inputs(root: &Path, contract: &Value) -> AppResult<()> {
    for item in contract["staticArtifacts"]
        .as_array()
        .expect("compiled artifacts")
    {
        check_binding(root, &binding(item))?;
    }
    let baseline_path = bound_path(root, &text(&contract["baseline"], "path"))?;
    check_binding(root, &binding(&contract["baseline"]))?;
    let baseline: Value = read_json(&baseline_path)?;
    for item in baseline["bindings"]
        .as_array()
        .ok_or_else(|| AppError::evidence("missing baseline bindings"))?
    {
        let expected: Binding =
            serde_json::from_value(item.clone()).evidence_context("invalid baseline binding")?;
        check_binding(root, &expected)?;
    }
    Ok(())
}

fn source_checks(root: &Path, directory: &Path, expected: &Value) -> AppResult<Vec<PassiveEvent>> {
    let source = bound_path(root, &text(expected, "sourcePath"))?;
    require(
        canonical(directory)? == source,
        "source is not in its exact frozen scenario slot",
    )?;
    for (name, field) in [
        ("manifest.json", "manifestSha256"),
        ("events.jsonl", "eventsSha256"),
        ("analysis.json", "analysisSha256"),
    ] {
        require(
            file_sha256(&source.join(name))? == text(expected, field),
            "source triple hash mismatch",
        )?;
    }
    let fresh = replay_passive_run(&source)?;
    require(
        fresh.result == text(expected, "expectedResult")
            && fresh.scenario == text(expected, "scenario")
            && fresh.artifact_identity == text(expected, "artifactIdentity"),
        "source replay verdict, scenario or identity mismatch",
    )?;
    let manifest: PassiveManifest = read_json(&source.join("manifest.json"))?;
    let foundation = json!([
        manifest.target_sha256,
        manifest.observed_target_sha256,
        manifest.target_equivalence_sha256,
        manifest.layout_evidence_sha256,
        manifest.coop_graph_sha256
    ]);
    let producer = json!([
        manifest.coop_catalog_sha256,
        manifest.coop_plan_sha256,
        manifest.coop_verification_sha256,
        manifest.native_sha256,
        manifest.helper_sha256
    ]);
    require(
        foundation == expected["foundation"] && producer == expected["producer"],
        "target or exact producer lineage changed",
    )?;
    let reader = BufReader::new(
        fs::File::open(source.join("events.jsonl"))
            .evidence_context("cannot read source events")?,
    );
    reader
        .lines()
        .map(|line| {
            serde_json::from_str(&line.evidence_context("cannot read event")?)
                .evidence_context("invalid event schema")
        })
        .collect()
}

fn menu_precondition(events: &[PassiveEvent]) -> AppResult<()> {
    let teardown = events
        .iter()
        .position(|e| e.kind == "LIFECYCLE_ENTER" && e.phase == 8)
        .ok_or_else(|| AppError::evidence("menu teardown witness missing"))?;
    let prefix = &events[..teardown];
    let bound_a = prefix
        .iter()
        .rposition(|e| e.kind == "BINDING_BOUND" && e.local_ordinal == Some(0))
        .ok_or_else(|| AppError::evidence("menu initial A BOUND missing"))?;
    let a = &prefix[bound_a];
    let b = prefix.iter().find(|e| {
        e.kind == "BINDING_BOUND"
            && e.local_ordinal == Some(1)
            && e.owner_generation == a.owner_generation
            && e.loop_iteration == a.loop_iteration
            && e.topology_digest == a.topology_digest
    });
    require(
        b.is_some()
            && prefix[bound_a..]
                .iter()
                .all(|e| !matches!(e.kind.as_str(), "BINDING_UNBOUND" | "BINDING_RETIRED")),
        "menu requires current double BOUND before teardown",
    )
}

fn verification_checks(root: &Path, path: &Path, contract: &Value) -> AppResult<Binding> {
    let path = canonical(path)?;
    require(
        path.starts_with(root),
        "verification must remain repository-relative",
    )?;
    let report: Verification = read_json(&path)?;
    require(
        report.schema_version == 5
            && report.verification_identity == VERIFICATION_ID
            && report.work_package == "CS-T0-05-03"
            && report.phase == VERIFICATION_PHASE
            && report.status == "PASS"
            && report.create_new_only
            && report.full_gate
            && report.completed_live_source_count == 5
            && report.not_applicable_count == 1
            && report.historical_exit_result == "BLOCKED_BY_EVIDENCE"
            && !report.six_source_gate_eligible
            && !report.live_authorization_created,
        "verification identity, phase, counts or safety flags invalid",
    )?;
    require(
        chrono::DateTime::parse_from_rfc3339(&report.created_utc).is_ok(),
        "verification timestamp invalid",
    )?;
    require(
        report.predecessor_verification == binding(&contract["predecessorVerification"]),
        "verification predecessor drift",
    )?;
    check_binding(root, &report.predecessor_verification)?;
    let expected_permissions: Permissions =
        serde_json::from_value(contract["zeroCapabilities"].clone())
            .expect("compiled zero capabilities");
    require(
        report.permissions == expected_permissions,
        "verification widened or omitted capability",
    )?;
    let expected_checks: Vec<String> =
        serde_json::from_value(contract["requiredAutomaticChecks"].clone())
            .expect("compiled checks");
    require(
        report
            .checks
            .iter()
            .map(|c| &c.id)
            .eq(expected_checks.iter())
            && report.checks.iter().all(|c| c.passed),
        "verification checks must be exact, complete, ordered and PASS",
    )?;
    let expected_artifacts: Vec<String> =
        serde_json::from_value(contract["verificationArtifactPaths"].clone())
            .expect("compiled verification paths");
    require(
        report
            .artifact_bindings
            .iter()
            .map(|b| &b.path)
            .eq(expected_artifacts.iter()),
        "verification artifact set differs",
    )?;
    for item in &report.artifact_bindings {
        check_binding(root, item)?;
    }
    let production = [
        "out/cs-t0/IsaacCsT0.exe",
        "out/cs-t0/IsaacCsT0.Native.dll",
        "out/cs-t0/IsaacCsT0.Helper.exe",
    ];
    require(
        report
            .production_bindings
            .iter()
            .map(|b| b.path.as_str())
            .eq(production),
        "verification production set differs",
    )?;
    for item in &report.production_bindings {
        check_binding(root, item)?;
    }
    require(
        file_sha256(
            &std::env::current_exe().evidence_context("cannot resolve current controller")?,
        )? == report.production_bindings[0].sha256,
        "current controller is not the verified production executable",
    )?;
    Ok(Binding {
        path: path
            .strip_prefix(root)
            .expect("checked root")
            .to_string_lossy()
            .replace('\\', "/"),
        sha256: file_sha256(&path)?,
    })
}

/// No verification means evidence-only success, never permission to advance.
pub fn gate_to(
    root: &Path,
    applicability: &Path,
    exit_source: &Path,
    sources: [&Path; 5],
    verification: Option<&Path>,
    output: &Path,
) -> AppResult<Value> {
    require(
        !output
            .try_exists()
            .evidence_context("cannot inspect output")?,
        "create-new output already exists",
    )?;
    let root = canonical(root)?;
    let parent = output
        .parent()
        .filter(|p| !p.as_os_str().is_empty())
        .unwrap_or(Path::new("."));
    let parent = canonical(parent)?;
    for directory in sources.into_iter().chain([exit_source]) {
        require(
            !parent.starts_with(canonical(directory)?),
            "output cannot modify any source directory",
        )?;
    }
    let contract_bytes =
        fs::read(applicability).evidence_context("cannot read applicability contract")?;
    require(
        sha(&contract_bytes) == sha(CONTRACT),
        "applicability contract differs from compiled exact identity/schema/hash/scope",
    )?;
    let contract: Value =
        serde_json::from_slice(CONTRACT).expect("compiled applicability contract");
    let verification_binding = verification
        .map(|path| verification_checks(&root, path, &contract))
        .transpose()?;
    verify_static_inputs(&root, &contract)?;
    let expected_sources = contract["sources"].as_array().expect("compiled sources");
    let mut identities = BTreeSet::new();
    for (directory, expected) in sources.into_iter().zip(expected_sources) {
        require(
            identities.insert(text(expected, "artifactIdentity")),
            "duplicate source identity",
        )?;
        let events = source_checks(&root, directory, expected)?;
        if expected["scenario"] == "menu-teardown-passive" {
            menu_precondition(&events)?;
        }
    }
    let exit_events = source_checks(&root, exit_source, &contract["historicalExit"])?;
    require(
        exit_events.iter().all(|e| {
            !matches!(
                e.kind.as_str(),
                "EXIT_BEGIN" | "EXIT_RETURN_STABLE" | "EXIT_HUD_BEGIN" | "EXIT_HUD_END"
            )
        }),
        "historical exit is not the frozen no-exit counterevidence",
    )?;
    // Rehash the complete immutable baseline after every source replay and before publication.
    verify_static_inputs(&root, &contract)?;
    if let (Some(path), Some(expected)) = (verification, &verification_binding) {
        require(
            verification_checks(&root, path, &contract)? == *expected,
            "verification changed during gate",
        )?;
    }
    let formal = verification_binding.is_some();
    let mut permissions = contract["zeroCapabilities"].clone();
    permissions["csT00504Unlocked"] = json!(formal);
    let decision = json!({
        "schemaVersion": 3,
        "decisionIdentity": "IsaacCsT0-CoopPassiveApplicabilityDecision-v3",
        "workPackage": "CS-T0-05-03",
        "createdUtc": utc_now(),
        "status": if formal { "PASS" } else { "PASS_EVIDENCE_ONLY" },
        "createNewOnly": true,
        "supersedes": null,
        "predecessorProgress": contract["predecessorProgress"],
        "applicability": {"identity": contract["applicabilityIdentity"], "sha256": sha(CONTRACT), "status": contract["status"], "scope": contract["scope"]},
        "staticArtifacts": contract["staticArtifacts"],
        "baseline": contract["baseline"],
        "sources": expected_sources,
        "historicalExit": contract["historicalExit"],
        "capabilityLimitations": contract["capabilityLimitations"],
        "verification": verification_binding,
        "formalPassCount": 5,
        "requiredApplicablePassCount": 5,
        "notApplicableCount": 1,
        "sixSourceGateEligible": false,
        "historicalExitResultChanged": false,
        "liveAuthorizationCreated": false,
        "csT00504Unlocked": formal,
        "activeInputAllowed": false,
        "passiveHookTransactionAllowed": false,
        "runtimeMutationAllowed": false,
        "t005Unlocked": false,
        "permissions": permissions,
        "checks": [
            {"id":"frozen-applicability-static-and-baseline", "passed":true},
            {"id":"five-distinct-exact-source-replays", "passed":true},
            {"id":"menu-double-bound-before-retirement", "passed":true},
            {"id":"historical-exit-remains-blocked", "passed":true},
            {"id":"immutable-inputs-and-no-live-authority", "passed":true}
        ]
    });
    let mut bytes =
        serde_json::to_vec_pretty(&decision).evidence_context("cannot serialize decision")?;
    bytes.push(b'\n');
    let mut file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(output)
        .evidence_context("cannot create decision output")?;
    file.write_all(&bytes)
        .evidence_context("cannot publish complete decision")?;
    file.sync_all().evidence_context("cannot flush decision")?;
    Ok(decision)
}
