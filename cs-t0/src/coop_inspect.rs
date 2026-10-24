use std::fs;
use std::path::{Component, Path, PathBuf};

use serde::Deserialize;

use crate::artifact::file_sha256;
use crate::coop_static::{
    CoopStaticInspection, FeatureObservation, InspectionBinding, InspectionCheck, build_inspection,
    parse_catalog_json, parse_graph_json, parse_ledger_json, validate_catalog, validate_graph,
    validate_inspection, validate_ledger, write_json_create_new,
};
use crate::error::{AppError, AppResult, UsageContext};
use crate::pe::PeImage;
use crate::target_equivalence::{
    APPROVED_UNPACKED_TARGET_SHA256, TARGET_EQUIVALENCE_IDENTITY,
    TARGET_EQUIVALENCE_SCHEMA_VERSION, TARGET_EQUIVALENCE_STATUS, TargetEquivalenceDocument,
};

const BOOTSTRAP_DECISION_SHA256: &str =
    "7EB5004E368A478EBB3CC83149BB05E70F7876C4384A511557EEC9188D1D7598";
const BOOTSTRAP_DECISION_IDENTITY: &str = "IsaacCsT0-CoopBootstrapDecision-v1";
const LEDGER_V1_SHA256: &str = "DC2E8F5B81929B84AB281779CF3F056F96BAE5446B7F3ED2173200A52421227C";
const CATALOG_V1_SHA256: &str = "206C715996B262A7A185E92652D0DC3108DC4E97553B5B7E902C620CD51E109A";

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
struct BootstrapArtifactBinding {
    path: String,
    identity: String,
    sha256: String,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
struct BootstrapChecks {
    target_equivalence_and_gates_unique: bool,
    predecessor_hash_binding_count: usize,
    predecessor_hashes_closed: bool,
    new_artifact_identities_unique: bool,
    new_candidates_unknown_count: usize,
    fabricated_rva_or_abi_count: usize,
    known_runtime_gap_count: usize,
    frozen_evidence_label_count: usize,
    base_primary_case_count: usize,
    conditional_primary_case_count: usize,
    repeatability_case_count: usize,
    repository_relative_paths_only: bool,
    raw_peer_identity_absent: bool,
    live_credentials_absent: bool,
    t004_protected_inputs_unchanged: bool,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
struct BootstrapDecision {
    schema_version: u32,
    decision_identity: String,
    work_package: String,
    created_utc: String,
    status: String,
    predecessor_status: String,
    predecessor_binding_sha256: String,
    artifact_bindings: Vec<BootstrapArtifactBinding>,
    checks: BootstrapChecks,
    active_input_allowed: bool,
    runtime_mutation_allowed: bool,
    t005_unlocked: bool,
    cs_t00501_unlocked: bool,
    next_work_package: String,
}

#[allow(clippy::too_many_arguments)]
pub fn inspect_coop_static(
    game_exe: &Path,
    target_equivalence: &Path,
    bootstrap_decision: &Path,
    ledger_path: &Path,
    catalog_path: &Path,
    graph_path: &Path,
    output: Option<&Path>,
) -> AppResult<CoopStaticInspection> {
    if output.is_some_and(Path::exists) {
        return Err(AppError::usage(format!(
            "refusing to overwrite {}",
            output.expect("checked output").display()
        )));
    }

    let graph_bytes = read_bytes(graph_path, "coop static graph")?;
    let catalog_bytes = read_bytes(catalog_path, "coop catalog")?;
    let ledger_bytes = read_bytes(ledger_path, "coop ledger")?;
    let equivalence_bytes = read_bytes(target_equivalence, "target equivalence")?;
    let bootstrap_bytes = read_bytes(bootstrap_decision, "bootstrap decision")?;

    let graph = parse_graph_json(&graph_bytes)?;
    let catalog = parse_catalog_json(&catalog_bytes)?;
    let ledger = parse_ledger_json(&ledger_bytes)?;
    let equivalence: TargetEquivalenceDocument = serde_json::from_slice(&equivalence_bytes)
        .map_err(|error| AppError::evidence(format!("invalid target equivalence: {error}")))?;
    let bootstrap: BootstrapDecision = serde_json::from_slice(&bootstrap_bytes)
        .map_err(|error| AppError::evidence(format!("invalid bootstrap decision: {error}")))?;

    validate_graph(&graph)?;
    validate_catalog(&catalog, &graph)?;
    validate_ledger(&ledger)?;

    let graph_sha256 = crate::pe::sha256_hex(&graph_bytes);
    let catalog_sha256 = crate::pe::sha256_hex(&catalog_bytes);
    let ledger_sha256 = crate::pe::sha256_hex(&ledger_bytes);
    let equivalence_sha256 = crate::pe::sha256_hex(&equivalence_bytes);
    let bootstrap_sha256 = crate::pe::sha256_hex(&bootstrap_bytes);

    validate_bootstrap(&bootstrap, &bootstrap_sha256)?;
    validate_equivalence(&equivalence, &equivalence_sha256)?;
    validate_bundle_bindings(
        graph_path,
        catalog_path,
        ledger_path,
        bootstrap_decision,
        &graph,
        &catalog,
        &ledger,
        &graph_sha256,
        &catalog_sha256,
        &bootstrap_sha256,
    )?;

    if graph.target.canonical_sha256 != catalog.target.canonical_sha256
        || graph.target.observed_approved_unpacked_sha256
            != catalog.target.observed_approved_unpacked_sha256
        || graph.target.machine != catalog.target.machine
        || graph.target.preferred_image_base != catalog.target.preferred_image_base
        || graph.target.canonical_sha256 != ledger.target_identity.canonical_sha256
        || graph.target.observed_approved_unpacked_sha256
            != ledger.target_identity.observed_approved_unpacked_sha256
        || graph.target.equivalence_sha256.as_deref() != Some(equivalence_sha256.as_str())
        || catalog.target.equivalence_document_sha256.as_deref()
            != Some(equivalence_sha256.as_str())
        || ledger.target_identity.equivalence_document_sha256 != equivalence_sha256
    {
        return Err(AppError::evidence(
            "coop graph, catalog, ledger and equivalence target bindings differ",
        ));
    }

    let image = PeImage::read(game_exe)
        .map_err(|error| AppError::evidence(format!("cannot inspect disk PE: {error}")))?;
    let image_hashes = image
        .hashes()
        .map_err(|error| AppError::evidence(format!("cannot hash disk PE: {error}")))?;
    if image_hashes.file_sha256 != APPROVED_UNPACKED_TARGET_SHA256
        || image_hashes.file_sha256 != equivalence.observed_target_sha256
        || image.machine != 0x014C
        || image.image_base != 0x0040_0000
    {
        return Err(AppError::evidence(
            "disk PE is not the hash-bound approved-unpacked x86 target",
        ));
    }

    validate_graph_locators(&image, &graph)?;
    let feature_results = observe_features(&image, &catalog)?;
    let evidence_sha256 = verify_indirect_evidence(graph_path, catalog_path, &graph, &catalog)?;

    let bindings = vec![
        inspection_binding(
            "BOOTSTRAP_DECISION",
            BOOTSTRAP_DECISION_IDENTITY,
            bootstrap_sha256,
        ),
        inspection_binding("CATALOG", &catalog.catalog_identity, catalog_sha256),
        inspection_binding("GRAPH", &graph.graph_identity, graph_sha256),
        inspection_binding("LEDGER", &ledger.ledger_identity, ledger_sha256),
        inspection_binding(
            "STATIC_EVIDENCE",
            "IsaacCsT0-CoopStaticEvidence-v1",
            evidence_sha256,
        ),
        inspection_binding(
            "TARGET_EQUIVALENCE",
            &equivalence.equivalence_identity,
            equivalence_sha256,
        ),
        inspection_binding(
            "TARGET_PE",
            "IsaacCsT0-ApprovedUnpackedPE",
            image_hashes.file_sha256,
        ),
    ];
    let checks = [
        "approved-unpacked-equivalence",
        "artifact-hash-bindings",
        "bootstrap-pass-and-05-01-unlocked",
        "bytes-opcode-rel32-targets",
        "graph-no-dangling-edges",
        "join-and-exit-routes-complete",
        "masked-features-unique",
        "online-partition-isolated",
        "permissions-remain-disabled",
        "repository-relative-artifact-policy",
        "slot-45-through-52-single-transaction",
        "strict-schema-identities",
        "supersedes-hashes-match",
    ]
    .into_iter()
    .map(|id| InspectionCheck {
        id: id.to_owned(),
        passed: true,
    })
    .collect();

    let inspection = build_inspection(
        &graph,
        &catalog,
        feature_results,
        bindings,
        checks,
        ledger.static_contract.missing_evidence.clone(),
    )?;
    validate_inspection(&inspection)?;
    if let Some(path) = output {
        write_json_create_new(path, &inspection)?;
    }
    Ok(inspection)
}

fn validate_bootstrap(decision: &BootstrapDecision, actual_sha256: &str) -> AppResult<()> {
    if actual_sha256 != BOOTSTRAP_DECISION_SHA256
        || decision.schema_version != 1
        || decision.decision_identity != BOOTSTRAP_DECISION_IDENTITY
        || decision.work_package != "CS-T0-05-00"
        || decision.created_utc.is_empty()
        || decision.status != "PASS"
        || decision.predecessor_status != "PASS"
        || decision.predecessor_binding_sha256.len() != 64
        || decision.next_work_package != "CS-T0-05-01"
        || !decision.cs_t00501_unlocked
        || decision.active_input_allowed
        || decision.runtime_mutation_allowed
        || decision.t005_unlocked
        || decision.artifact_bindings.len() != 8
    {
        return Err(AppError::evidence(
            "bootstrap decision is not the frozen PASS that unlocks 05-01",
        ));
    }
    for binding in &decision.artifact_bindings {
        require_relative(&binding.path)?;
        if binding.identity.is_empty() || !is_sha256(&binding.sha256) {
            return Err(AppError::evidence("invalid bootstrap artifact binding"));
        }
    }
    let checks = &decision.checks;
    if !checks.target_equivalence_and_gates_unique
        || checks.predecessor_hash_binding_count != 13
        || !checks.predecessor_hashes_closed
        || !checks.new_artifact_identities_unique
        || checks.new_candidates_unknown_count != 6
        || checks.fabricated_rva_or_abi_count != 0
        || checks.known_runtime_gap_count != 2
        || checks.frozen_evidence_label_count != 5
        || checks.base_primary_case_count != 20
        || checks.conditional_primary_case_count != 2
        || checks.repeatability_case_count != 2
        || !checks.repository_relative_paths_only
        || !checks.raw_peer_identity_absent
        || !checks.live_credentials_absent
        || !checks.t004_protected_inputs_unchanged
    {
        return Err(AppError::evidence(
            "bootstrap decision checks differ from the frozen gate",
        ));
    }
    Ok(())
}

fn validate_equivalence(
    document: &TargetEquivalenceDocument,
    document_sha256: &str,
) -> AppResult<()> {
    if document.schema_version != TARGET_EQUIVALENCE_SCHEMA_VERSION
        || document.equivalence_identity != TARGET_EQUIVALENCE_IDENTITY
        || document.status != TARGET_EQUIVALENCE_STATUS
        || document.observed_target_sha256 != APPROVED_UNPACKED_TARGET_SHA256
        || document_sha256 != "8B3FB097ED32B18DE6A78F535045ACD525185569ACA029A6370494511851F138"
        || document.machine != "0x014C"
        || document.preferred_image_base != "0x00400000"
        || !document.static_proof_complete
        || !document.passive_attestation_allowed
        || document.eligible_for_active_gate
        || document.active_input_allowed
    {
        return Err(AppError::evidence(
            "target equivalence is not the frozen approved-unpacked read-only proof",
        ));
    }
    Ok(())
}

#[allow(clippy::too_many_arguments)]
fn validate_bundle_bindings(
    graph_path: &Path,
    catalog_path: &Path,
    ledger_path: &Path,
    bootstrap_path: &Path,
    graph: &crate::coop_static::CoopStaticGraph,
    catalog: &crate::coop_static::CoopCatalog,
    ledger: &crate::coop_static::CoopLedger,
    graph_sha256: &str,
    catalog_sha256: &str,
    bootstrap_sha256: &str,
) -> AppResult<()> {
    if catalog.graph_sha256 != graph_sha256
        || ledger.supersedes_sha256 != LEDGER_V1_SHA256
        || catalog.supersedes_sha256 != CATALOG_V1_SHA256
        || ledger.predecessor.bootstrap_decision_sha256 != bootstrap_sha256
    {
        return Err(AppError::evidence(
            "coop supersedes or direct artifact hash mismatch",
        ));
    }

    let graph_root = containing_directory(graph_path);
    for binding in &graph.source_provenance {
        let actual = if binding.identity == BOOTSTRAP_DECISION_IDENTITY {
            if file_sha256(bootstrap_path)? != binding.sha256 {
                return Err(AppError::evidence("graph bootstrap binding mismatch"));
            }
            binding.sha256.clone()
        } else {
            let resolved = resolve_bounded(graph_root, graph_root, &binding.path)?;
            file_sha256(&resolved)?
        };
        if actual != binding.sha256 {
            return Err(AppError::evidence(format!(
                "graph provenance hash mismatch: {}",
                binding.role
            )));
        }
    }

    let catalog_root = containing_directory(catalog_path);
    let graph_resolved = resolve_bounded(catalog_root, catalog_root, &catalog.graph_path)?;
    let evidence_resolved = resolve_bounded(catalog_root, catalog_root, &catalog.evidence_path)?;
    if file_sha256(&graph_resolved)? != catalog.graph_sha256
        || file_sha256(&evidence_resolved)? != catalog.evidence_sha256
    {
        return Err(AppError::evidence(
            "catalog graph/evidence binding mismatch",
        ));
    }

    let ledger_base = containing_directory(ledger_path);
    let ledger_root = containing_directory(ledger_base);
    for binding in &ledger.artifact_bindings {
        let resolved = resolve_bounded(ledger_root, ledger_base, &binding.path)?;
        if file_sha256(&resolved)? != binding.sha256 {
            return Err(AppError::evidence(format!(
                "ledger artifact hash mismatch: {}",
                binding.identity
            )));
        }
    }
    if file_sha256(graph_path)? != graph_sha256 || file_sha256(catalog_path)? != catalog_sha256 {
        return Err(AppError::evidence("artifact changed during inspection"));
    }
    Ok(())
}

fn verify_indirect_evidence(
    graph_path: &Path,
    catalog_path: &Path,
    graph: &crate::coop_static::CoopStaticGraph,
    catalog: &crate::coop_static::CoopCatalog,
) -> AppResult<String> {
    let binding = graph
        .source_provenance
        .iter()
        .find(|binding| binding.role == "STATIC_EVIDENCE")
        .ok_or_else(|| AppError::evidence("graph lacks STATIC_EVIDENCE provenance"))?;
    if binding.identity != "IsaacCsT0-CoopStaticEvidence-v1"
        || binding.sha256 != catalog.evidence_sha256
    {
        return Err(AppError::evidence("static evidence identity/hash mismatch"));
    }
    let root = containing_directory(graph_path);
    let evidence = resolve_bounded(root, root, &binding.path)?;
    let catalog_root = containing_directory(catalog_path);
    let catalog_evidence = resolve_bounded(catalog_root, catalog_root, &catalog.evidence_path)?;
    if evidence != catalog_evidence {
        return Err(AppError::evidence(
            "graph and catalog select different evidence files",
        ));
    }
    file_sha256(&evidence)
}

fn validate_graph_locators(
    image: &PeImage,
    graph: &crate::coop_static::CoopStaticGraph,
) -> AppResult<()> {
    for node in &graph.nodes {
        let expected = node.locator.expected_bytes.as_deref();
        if let Some(expected) = expected {
            let bytes = decode_hex(expected, "graph locator bytes")?;
            let rva = parse_rva(&node.locator.rva)?;
            let actual = image.bytes_at_rva(rva, bytes.len()).map_err(|error| {
                AppError::evidence(format!("graph locator is outside PE: {error}"))
            })?;
            if actual != bytes {
                return Err(AppError::evidence(format!(
                    "graph locator bytes differ: {}",
                    node.id
                )));
            }
            if node.locator.kind == "CALL_REL32" {
                let target = decode_rel32_target(rva, actual)?;
                if Some(format_rva(target).as_str()) != node.locator.target_rva.as_deref() {
                    return Err(AppError::evidence(format!(
                        "graph REL32 target differs: {}",
                        node.id
                    )));
                }
            }
        }
    }
    Ok(())
}

fn observe_features(
    image: &PeImage,
    catalog: &crate::coop_static::CoopCatalog,
) -> AppResult<Vec<FeatureObservation>> {
    let mut results = Vec::new();
    for slot in &catalog.inherited_hook_transaction.new_slots {
        let pattern = decode_hex(&slot.feature.pattern_hex, "masked feature pattern")?;
        let mask = decode_hex(&slot.feature.mask_hex, "masked feature mask")?;
        let hits = image
            .scan_masked(&slot.feature.section, &pattern, &mask)
            .map_err(|error| AppError::evidence(format!("feature scan failed: {error}")))?;
        let call_rva = parse_rva(&slot.rva)?;
        let call_bytes = image
            .bytes_at_rva(call_rva, slot.width)
            .map_err(|error| AppError::evidence(format!("observer call is outside PE: {error}")))?;
        let observed_target = if call_bytes.len() == 5 && call_bytes[0] == 0xE8 {
            Some(format_rva(decode_rel32_target(call_rva, call_bytes)?))
        } else {
            None
        };
        results.push(FeatureObservation {
            slot_id: slot.id.clone(),
            match_rvas: hits.into_iter().map(format_rva).collect(),
            observed_bytes: Some(hex::encode_upper(call_bytes)),
            observed_target_rva: observed_target,
        });
    }
    crate::coop_static::validate_feature_observations(catalog, &results)?;
    Ok(results)
}

fn inspection_binding(role: &str, identity: &str, sha256: String) -> InspectionBinding {
    InspectionBinding {
        role: role.to_owned(),
        identity: identity.to_owned(),
        sha256,
    }
}

fn read_bytes(path: &Path, label: &str) -> AppResult<Vec<u8>> {
    fs::read(path).usage_context(format!("cannot read {label} {}", path.display()))
}

fn containing_directory(path: &Path) -> &Path {
    path.parent()
        .filter(|parent| !parent.as_os_str().is_empty())
        .unwrap_or_else(|| Path::new("."))
}

fn resolve_bounded(root: &Path, base: &Path, relative: &str) -> AppResult<PathBuf> {
    require_relative(relative)?;
    let root = fs::canonicalize(root)
        .usage_context(format!("cannot resolve artifact root {}", root.display()))?;
    let candidate = fs::canonicalize(base.join(relative))
        .usage_context(format!("cannot resolve bound artifact {relative}"))?;
    if !candidate.starts_with(&root) {
        return Err(AppError::evidence(
            "artifact binding escapes the bundle root",
        ));
    }
    Ok(candidate)
}

fn require_relative(value: &str) -> AppResult<()> {
    let path = Path::new(value);
    if path.is_absolute()
        || path
            .components()
            .any(|component| matches!(component, Component::Prefix(_) | Component::RootDir))
    {
        return Err(AppError::evidence(
            "artifact path must be repository relative",
        ));
    }
    Ok(())
}

fn parse_rva(value: &str) -> AppResult<u32> {
    let digits = value
        .strip_prefix("0x")
        .ok_or_else(|| AppError::evidence("RVA must use 0x notation"))?;
    u32::from_str_radix(digits, 16).map_err(|_| AppError::evidence("invalid RVA"))
}

fn decode_hex(value: &str, label: &str) -> AppResult<Vec<u8>> {
    hex::decode(value).map_err(|error| AppError::evidence(format!("invalid {label}: {error}")))
}

fn decode_rel32_target(call_rva: u32, bytes: &[u8]) -> AppResult<u32> {
    if bytes.len() != 5 || bytes[0] != 0xE8 {
        return Err(AppError::evidence("observer opcode is not CALL REL32"));
    }
    let displacement = i32::from_le_bytes(bytes[1..5].try_into().unwrap()) as i64;
    let target = i64::from(call_rva) + 5 + displacement;
    u32::try_from(target).map_err(|_| AppError::evidence("REL32 target is outside PE32 RVA range"))
}

fn format_rva(rva: u32) -> String {
    format!("0x{rva:08X}")
}

fn is_sha256(value: &str) -> bool {
    value.len() == 64 && value.bytes().all(|byte| byte.is_ascii_hexdigit())
}
