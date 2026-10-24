use std::collections::{BTreeMap, BTreeSet};
use std::fs::OpenOptions;
use std::io::Write;
use std::path::Path;

use serde::{Deserialize, Serialize};

use crate::error::{AppError, AppResult};

pub const COOP_GRAPH_SCHEMA_VERSION: u32 = 1;
pub const COOP_GRAPH_IDENTITY: &str = "IsaacCsT0-CoopStaticGraph-v1";
pub const COOP_CATALOG_SCHEMA_VERSION: u32 = 2;
pub const COOP_CATALOG_IDENTITY: &str = "IsaacCsT0-CoopCatalog-v2";
pub const COOP_WORK_PACKAGE: &str = "CS-T0-05-01";

const REQUIRED_PARTITIONS: [(&str, bool); 3] = [
    ("LOCAL_COOP_POSITIVE", true),
    ("OFFLINE_SHARED", true),
    ("ORIGINAL_ONLINE_FORBIDDEN", false),
];
const REQUIRED_ROLES: [&str; 13] = [
    "CONTROLLER_ASSIGNMENT",
    "HUD_REGISTER",
    "HUD_REMOVE",
    "LOCAL_COOP_MANAGER",
    "LUA_PLAYER_PUBLICATION",
    "LUA_PLAYER_RETIREMENT",
    "NATIVE_EXIT",
    "NATIVE_JOIN",
    "ONLINE_GUARD",
    "PLAYER_INDEX_UPDATE",
    "SECOND_PLAYER_CREATE",
    "PLAYER_VECTOR_INSERT",
    "PLAYER_VECTOR_REMOVE",
];

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct TargetIdentity {
    pub canonical_sha256: String,
    pub observed_approved_unpacked_sha256: String,
    #[serde(default)]
    pub equivalence_identity: Option<String>,
    #[serde(default)]
    pub equivalence_sha256: Option<String>,
    #[serde(default)]
    pub equivalence_document_sha256: Option<String>,
    pub machine: String,
    pub preferred_image_base: String,
    #[serde(default)]
    pub runtime_address_rule: Option<String>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ArtifactBinding {
    pub role: String,
    pub path: String,
    pub identity: String,
    pub sha256: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct GraphPartition {
    pub id: String,
    pub description: String,
    pub positive_route_allowed: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct GraphLocator {
    pub kind: String,
    pub rva: String,
    pub expected_bytes: Option<String>,
    pub target_rva: Option<String>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct StaticAbi {
    pub calling_convention: String,
    pub return_type: String,
    pub this_type: String,
    pub register_args: Vec<String>,
    pub stack_args: Vec<String>,
    pub callee_stack_bytes: u32,
    pub return_use: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct GraphNode {
    pub id: String,
    pub partition: String,
    pub role: String,
    pub evidence_label: String,
    pub owner: String,
    pub locator: GraphLocator,
    pub abi: Option<StaticAbi>,
    pub thread_constraint: String,
    pub preconditions: Vec<String>,
    pub postconditions: Vec<String>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct GraphEdge {
    pub id: String,
    pub from: String,
    pub to: String,
    pub kind: String,
    pub positive: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct RequiredRoute {
    pub id: String,
    pub nodes: Vec<String>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct GraphPermissions {
    pub active_input_allowed: bool,
    pub runtime_mutation_allowed: bool,
    pub t005_unlocked: bool,
    pub cs_t00502_unlocked: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct CoopStaticGraph {
    pub schema_version: u32,
    pub graph_identity: String,
    pub work_package: String,
    pub created_utc: String,
    pub target: TargetIdentity,
    pub source_provenance: Vec<ArtifactBinding>,
    pub partitions: Vec<GraphPartition>,
    pub nodes: Vec<GraphNode>,
    pub edges: Vec<GraphEdge>,
    pub required_routes: Vec<RequiredRoute>,
    pub unresolved_claims: Vec<String>,
    pub design_unknowns: Vec<String>,
    pub permissions: GraphPermissions,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct CatalogCandidate {
    pub id: String,
    pub resolution: String,
    pub evidence_label: String,
    pub node_ids: Vec<String>,
    pub reason: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct MaskedFeature {
    pub section: String,
    pub pattern_hex: String,
    pub mask_hex: String,
    pub unique_match_count: usize,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ObserverSlot {
    pub ordinal: usize,
    pub id: String,
    pub node_id: String,
    pub owner: String,
    pub patch_kind: String,
    pub rva: String,
    pub width: usize,
    pub expected_bytes: String,
    pub target_rva: String,
    pub feature: MaskedFeature,
    pub abi: StaticAbi,
    pub thread_constraint: String,
    pub observation_boundary: String,
    pub positive_proof_eligible: bool,
    pub event_kinds: Vec<String>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct HookTransaction {
    pub catalog_path: String,
    pub catalog_sha256: String,
    pub existing_slot_count: usize,
    pub new_slots: Vec<ObserverSlot>,
    pub combined_slot_count: usize,
    pub install_order: Vec<String>,
    pub restore_order: Vec<String>,
    pub single_install_rollback_restore_owner_required: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ExcludedObservation {
    pub id: String,
    pub evidence_label: String,
    pub rva: Option<String>,
    pub abi: Option<StaticAbi>,
    pub reason: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct CatalogPermissions {
    pub active_input_allowed: bool,
    pub live_authorization_created: bool,
    pub runtime_mutation_allowed: bool,
    pub production_active_input_allowed: bool,
    pub t005_unlocked: bool,
    pub cs_t00502_unlocked: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct CoopCatalog {
    pub schema_version: u32,
    pub catalog_identity: String,
    pub work_package: String,
    pub created_utc: String,
    pub status: String,
    pub supersedes: String,
    pub supersedes_sha256: String,
    pub create_new_only: bool,
    pub target: TargetIdentity,
    pub evidence_path: String,
    pub evidence_sha256: String,
    pub graph_path: String,
    pub graph_sha256: String,
    pub candidates: Vec<CatalogCandidate>,
    pub inherited_hook_transaction: HookTransaction,
    pub excluded_observations: Vec<ExcludedObservation>,
    pub permissions: CatalogPermissions,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct LedgerPredecessor {
    pub bootstrap_decision_path: String,
    pub bootstrap_decision_identity: String,
    pub bootstrap_decision_sha256: String,
    pub status: String,
    pub cs_t00501_unlocked: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct LedgerTargetIdentity {
    pub canonical_sha256: String,
    pub observed_approved_unpacked_sha256: String,
    pub equivalence_document_sha256: String,
    pub canonical_and_observed_may_not_be_mixed: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct LedgerArtifactBinding {
    pub path: String,
    pub identity: String,
    pub sha256: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct CandidateResolution {
    pub id: String,
    pub resolution: String,
    pub evidence_label: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct StaticContract {
    pub manager_owner_closed: bool,
    pub victory_lap_caller_classified: bool,
    pub ordinary_join_route_closed: bool,
    pub ordinary_exit_route_closed: bool,
    pub player_vector_insert_remove_closed: bool,
    pub assignment_closed: bool,
    pub hud_and_index_closed: bool,
    pub lua_publication_retirement_closed: bool,
    pub online_vector_guard_closed: bool,
    pub online_partition_excluded: bool,
    pub observer_slot_count: usize,
    pub combined_slot_count: usize,
    pub missing_evidence: Vec<String>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ArtifactRules {
    pub create_new_only: bool,
    pub supersedes_and_prior_sha_required_for_revision: bool,
    pub source_and_analysis_overwrite_allowed: bool,
    pub all_formal_inputs_hash_bound: bool,
    pub repository_relative_paths_only: bool,
    pub absolute_user_paths_allowed: bool,
    pub raw_peer_identity_allowed: bool,
    pub process_pointers_allowed: bool,
    pub platform_handles_allowed: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ExitGate {
    pub static_evidence_complete: bool,
    pub graph_routes_complete: bool,
    pub online_partition_isolated: bool,
    pub fabricated_unknown_field_count: usize,
    pub missing_evidence_count: usize,
    pub result: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct CoopLedger {
    pub schema_version: u32,
    pub ledger_identity: String,
    pub work_package: String,
    pub created_utc: String,
    pub status: String,
    pub supersedes: String,
    pub supersedes_sha256: String,
    pub evidence_domain: String,
    pub predecessor: LedgerPredecessor,
    pub target_identity: LedgerTargetIdentity,
    pub artifact_bindings: Vec<LedgerArtifactBinding>,
    pub frozen_evidence_labels: Vec<String>,
    pub candidate_resolutions: Vec<CandidateResolution>,
    pub static_contract: StaticContract,
    pub artifact_rules: ArtifactRules,
    pub permissions: CatalogPermissions,
    pub exit_gate: ExitGate,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct FeatureObservation {
    pub slot_id: String,
    pub match_rvas: Vec<String>,
    pub observed_bytes: Option<String>,
    pub observed_target_rva: Option<String>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct InspectionPermissions {
    pub active_input_allowed: bool,
    pub runtime_mutation_allowed: bool,
    pub t005_unlocked: bool,
    pub cs_t00502_unlocked: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct InspectionBinding {
    pub role: String,
    pub identity: String,
    pub sha256: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct InspectionCheck {
    pub id: String,
    pub passed: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct GraphSummary {
    pub partition_count: usize,
    pub node_count: usize,
    pub edge_count: usize,
    pub required_route_count: usize,
    pub new_slot_count: usize,
    pub combined_slot_count: usize,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct CoopStaticInspection {
    pub schema_version: u32,
    pub inspection_identity: String,
    pub work_package: String,
    pub result: String,
    pub target: TargetIdentity,
    pub artifact_bindings: Vec<InspectionBinding>,
    pub checks: Vec<InspectionCheck>,
    pub graph_identity: String,
    pub catalog_identity: String,
    pub graph_summary: GraphSummary,
    pub feature_results: Vec<FeatureObservation>,
    pub missing_evidence: Vec<String>,
    pub permissions: InspectionPermissions,
}

pub fn parse_graph_json(bytes: &[u8]) -> AppResult<CoopStaticGraph> {
    serde_json::from_slice(bytes)
        .map_err(|error| AppError::evidence(format!("invalid coop static graph: {error}")))
}

pub fn parse_catalog_json(bytes: &[u8]) -> AppResult<CoopCatalog> {
    serde_json::from_slice(bytes)
        .map_err(|error| AppError::evidence(format!("invalid coop catalog: {error}")))
}

pub fn parse_ledger_json(bytes: &[u8]) -> AppResult<CoopLedger> {
    serde_json::from_slice(bytes)
        .map_err(|error| AppError::evidence(format!("invalid coop ledger: {error}")))
}

pub fn validate_graph(graph: &CoopStaticGraph) -> AppResult<()> {
    require(
        graph.schema_version == COOP_GRAPH_SCHEMA_VERSION,
        "unsupported graph schema",
    )?;
    require(
        graph.graph_identity == COOP_GRAPH_IDENTITY,
        "unexpected graph identity",
    )?;
    require(
        graph.work_package == COOP_WORK_PACKAGE,
        "unexpected graph work package",
    )?;
    validate_target(&graph.target)?;
    validate_permissions(
        graph.permissions.active_input_allowed,
        graph.permissions.runtime_mutation_allowed,
        graph.permissions.t005_unlocked,
    )?;
    require(
        graph.permissions.cs_t00502_unlocked == graph.unresolved_claims.is_empty(),
        "05-02 graph permission conflicts with unresolved claims",
    )?;
    for binding in &graph.source_provenance {
        require(is_sha256(&binding.sha256), "invalid graph source SHA")?;
        validate_sensitive_strings(std::iter::once(&binding.path))?;
    }

    let actual_partitions: BTreeMap<_, _> = graph
        .partitions
        .iter()
        .map(|partition| (partition.id.as_str(), partition.positive_route_allowed))
        .collect();
    let required_partitions: BTreeMap<_, _> = REQUIRED_PARTITIONS.into_iter().collect();
    require(
        actual_partitions == required_partitions,
        "graph partitions are not the frozen set",
    )?;

    let mut node_ids = BTreeSet::new();
    let mut roles = BTreeSet::new();
    for node in &graph.nodes {
        require(node_ids.insert(node.id.as_str()), "duplicate graph node id")?;
        require(
            actual_partitions.contains_key(node.partition.as_str()),
            "node has unknown partition",
        )?;
        reject_authority(&node.id)?;
        reject_authority(&node.role)?;
        validate_locator(node)?;
        validate_sensitive_strings(
            node.preconditions
                .iter()
                .chain(node.postconditions.iter())
                .chain(std::iter::once(&node.owner)),
        )?;
        if node.evidence_label == "UNKNOWN" {
            require(
                node.locator.rva.is_empty(),
                "UNKNOWN node must not guess an RVA",
            )?;
            require(
                node.locator.expected_bytes.is_none(),
                "UNKNOWN node must not guess bytes",
            )?;
            require(
                node.locator.target_rva.is_none(),
                "UNKNOWN node must not guess a target",
            )?;
            require(node.abi.is_none(), "UNKNOWN node must not guess an ABI")?;
        }
        roles.insert(node.role.as_str());
    }
    for role in REQUIRED_ROLES {
        require(
            roles.contains(role),
            format!("required graph role is missing: {role}"),
        )?;
    }

    let mut positive_edges = BTreeSet::new();
    let mut edge_ids = BTreeSet::new();
    for edge in &graph.edges {
        require(edge_ids.insert(edge.id.as_str()), "duplicate graph edge id")?;
        require(
            node_ids.contains(edge.from.as_str()),
            "edge.from is dangling",
        )?;
        require(node_ids.contains(edge.to.as_str()), "edge.to is dangling")?;
        if edge.positive {
            positive_edges.insert((edge.from.as_str(), edge.to.as_str()));
            let from = graph
                .nodes
                .iter()
                .find(|node| node.id == edge.from)
                .unwrap();
            let to = graph.nodes.iter().find(|node| node.id == edge.to).unwrap();
            require(
                from.partition != "ORIGINAL_ONLINE_FORBIDDEN"
                    && to.partition != "ORIGINAL_ONLINE_FORBIDDEN",
                "positive edge crosses the online partition",
            )?;
        }
    }

    let routes: BTreeMap<_, _> = graph
        .required_routes
        .iter()
        .map(|route| (route.id.as_str(), route))
        .collect();
    for route_id in [
        "ordinary-local-join-publication",
        "ordinary-local-exit-retirement",
    ] {
        let route = routes
            .get(route_id)
            .ok_or_else(|| AppError::evidence(format!("required route is missing: {route_id}")))?;
        require(route.nodes.len() >= 2, "required route is incomplete")?;
        for id in &route.nodes {
            let node = graph
                .nodes
                .iter()
                .find(|node| &node.id == id)
                .ok_or_else(|| {
                    AppError::evidence(format!("required route contains dangling node: {id}"))
                })?;
            require(
                node.partition != "ORIGINAL_ONLINE_FORBIDDEN",
                "required route crosses online partition",
            )?;
        }
        for pair in route.nodes.windows(2) {
            require(
                positive_edges.contains(&(pair[0].as_str(), pair[1].as_str())),
                "required route lacks a positive connecting edge",
            )?;
        }
    }
    let expected_join = [
        "manager-owner-call",
        "manager-core-forward",
        "manager-local-dispatch",
        "ordinary-spawn-call",
        "player-create",
        "player-vector-insert",
        "ordinary-assignment-call",
        "join-hud-rebuild-call",
        "hud-index-helper",
        "lua-player-publication",
    ];
    let expected_exit = [
        "manager-local-dispatch",
        "native-exit-call",
        "player-vector-remove",
        "survivor-ordinal-rewrite",
        "exit-hud-rebuild-call",
        "lua-player-retirement",
    ];
    require(
        routes["ordinary-local-join-publication"]
            .nodes
            .iter()
            .map(String::as_str)
            .eq(expected_join),
        "ordinary join route differs from the frozen route",
    )?;
    require(
        routes["ordinary-local-exit-retirement"]
            .nodes
            .iter()
            .map(String::as_str)
            .eq(expected_exit),
        "ordinary exit route differs from the frozen route",
    )?;
    validate_sensitive_strings(graph.design_unknowns.iter())?;
    Ok(())
}

pub fn validate_catalog(catalog: &CoopCatalog, graph: &CoopStaticGraph) -> AppResult<()> {
    require(
        catalog.schema_version == COOP_CATALOG_SCHEMA_VERSION,
        "unsupported catalog schema",
    )?;
    require(
        catalog.catalog_identity == COOP_CATALOG_IDENTITY,
        "unexpected catalog identity",
    )?;
    require(
        catalog.work_package == COOP_WORK_PACKAGE,
        "unexpected catalog work package",
    )?;
    require(catalog.create_new_only, "catalog must be create-new only")?;
    require(
        !catalog.supersedes.is_empty() && is_sha256(&catalog.supersedes_sha256),
        "invalid supersedes binding",
    )?;
    validate_target(&catalog.target)?;
    require(
        catalog.target.canonical_sha256 == graph.target.canonical_sha256
            && catalog.target.observed_approved_unpacked_sha256
                == graph.target.observed_approved_unpacked_sha256,
        "catalog and graph target identities differ",
    )?;
    validate_sensitive_strings(
        [
            &catalog.supersedes,
            &catalog.evidence_path,
            &catalog.graph_path,
            &catalog.inherited_hook_transaction.catalog_path,
        ]
        .into_iter(),
    )?;
    validate_permissions(
        catalog.permissions.active_input_allowed,
        catalog.permissions.runtime_mutation_allowed,
        catalog.permissions.t005_unlocked,
    )?;
    require(
        !catalog.permissions.live_authorization_created,
        "live authorization is forbidden",
    )?;
    require(
        !catalog.permissions.production_active_input_allowed,
        "production active input is forbidden",
    )?;

    for candidate in &catalog.candidates {
        reject_authority(&candidate.id)?;
        if candidate.resolution == "UNKNOWN" || candidate.evidence_label == "UNKNOWN" {
            require(
                candidate.node_ids.is_empty(),
                "UNKNOWN candidate must not guess node bindings",
            )?;
        } else {
            require(
                candidate.resolution == "RESOLVED",
                "invalid candidate resolution",
            )?;
            for node_id in &candidate.node_ids {
                require(
                    graph.nodes.iter().any(|node| &node.id == node_id),
                    "candidate node is dangling",
                )?;
            }
        }
    }
    for excluded in &catalog.excluded_observations {
        require(
            excluded.evidence_label == "UNKNOWN",
            "excluded observation must remain UNKNOWN",
        )?;
        require(
            excluded.rva.is_none() && excluded.abi.is_none(),
            "UNKNOWN observation guessed RVA or ABI",
        )?;
    }

    let transaction = &catalog.inherited_hook_transaction;
    require(
        transaction.existing_slot_count == 45,
        "existing slot count must be 45",
    )?;
    require(
        transaction.combined_slot_count == 53,
        "combined slot count must be 53",
    )?;
    require(
        transaction.new_slots.len() == 8,
        "exactly eight coop slots are required",
    )?;
    require(
        transaction.single_install_rollback_restore_owner_required,
        "single transaction owner is required",
    )?;
    let mut ids = BTreeSet::new();
    let mut ranges: Vec<(u32, u32)> = Vec::new();
    for (offset, slot) in transaction.new_slots.iter().enumerate() {
        require(
            slot.ordinal == 45 + offset,
            "slot ordinals must be continuous 45..52",
        )?;
        require(ids.insert(slot.id.as_str()), "duplicate slot id")?;
        require(
            slot.patch_kind == "CALL_REL32" && slot.width == 5,
            "slot must be a five-byte REL32 call",
        )?;
        require(
            graph.nodes.iter().any(|node| node.id == slot.node_id),
            "slot node is dangling",
        )?;
        require(
            slot.feature.unique_match_count == 1,
            "masked feature must have exactly one match",
        )?;
        validate_masked_feature(&slot.feature)?;
        validate_rel32(&slot.rva, &slot.expected_bytes, &slot.target_rva)?;
        let start = parse_rva(&slot.rva)?;
        let end = start
            .checked_add(slot.width as u32)
            .ok_or_else(|| AppError::evidence("observer slot range overflows"))?;
        require(
            ranges
                .iter()
                .all(|(other_start, other_end)| end <= *other_start || start >= *other_end),
            "observer slots overlap",
        )?;
        ranges.push((start, end));
        reject_authority(&slot.id)?;
        validate_sensitive_strings(std::iter::once(&slot.owner))?;
    }
    let expected_install: Vec<_> = transaction
        .new_slots
        .iter()
        .map(|slot| slot.id.as_str())
        .collect();
    require(
        transaction
            .install_order
            .iter()
            .map(String::as_str)
            .eq(expected_install.iter().copied()),
        "install order must follow increasing ordinal",
    )?;
    require(
        transaction
            .restore_order
            .iter()
            .map(String::as_str)
            .eq(expected_install.iter().rev().copied()),
        "restore order must be the strict reverse of install order",
    )?;
    Ok(())
}

pub fn validate_ledger(ledger: &CoopLedger) -> AppResult<()> {
    require(ledger.schema_version == 2, "unsupported coop ledger schema")?;
    require(
        ledger.ledger_identity == "IsaacCsT0-CoopLedger-v2",
        "unexpected ledger identity",
    )?;
    require(
        ledger.work_package == COOP_WORK_PACKAGE,
        "unexpected ledger work package",
    )?;
    require(
        matches!(
            ledger.status.as_str(),
            "PASS" | "BLOCKED_BY_EVIDENCE" | "FAIL"
        ),
        "invalid ledger status",
    )?;
    require(
        !ledger.supersedes.is_empty() && is_sha256(&ledger.supersedes_sha256),
        "invalid ledger supersedes binding",
    )?;
    require(
        ledger.predecessor.status == "PASS" && ledger.predecessor.cs_t00501_unlocked,
        "bootstrap predecessor did not unlock 05-01",
    )?;
    require(
        is_sha256(&ledger.predecessor.bootstrap_decision_sha256),
        "invalid predecessor SHA",
    )?;
    require(
        is_sha256(&ledger.target_identity.canonical_sha256)
            && is_sha256(&ledger.target_identity.observed_approved_unpacked_sha256)
            && is_sha256(&ledger.target_identity.equivalence_document_sha256)
            && ledger
                .target_identity
                .canonical_and_observed_may_not_be_mixed,
        "invalid ledger target identity",
    )?;
    require(
        !ledger.artifact_bindings.is_empty(),
        "ledger has no artifact bindings",
    )?;
    for binding in &ledger.artifact_bindings {
        require(is_sha256(&binding.sha256), "invalid artifact binding SHA")?;
        validate_sensitive_strings(std::iter::once(&binding.path))?;
    }
    validate_permissions(
        ledger.permissions.active_input_allowed,
        ledger.permissions.runtime_mutation_allowed,
        ledger.permissions.t005_unlocked,
    )?;
    require(
        !ledger.permissions.live_authorization_created,
        "ledger creates live authorization",
    )?;
    require(
        !ledger.permissions.production_active_input_allowed,
        "ledger enables production input",
    )?;
    let contract = &ledger.static_contract;
    require(
        contract.observer_slot_count == 8 && contract.combined_slot_count == 53,
        "ledger slot counts are not frozen",
    )?;
    let all_static_claims = contract.manager_owner_closed
        && contract.victory_lap_caller_classified
        && contract.ordinary_join_route_closed
        && contract.ordinary_exit_route_closed
        && contract.player_vector_insert_remove_closed
        && contract.assignment_closed
        && contract.hud_and_index_closed
        && contract.lua_publication_retirement_closed
        && contract.online_vector_guard_closed
        && contract.online_partition_excluded;
    let evidence_complete = all_static_claims && contract.missing_evidence.is_empty();
    require(
        ledger.exit_gate.missing_evidence_count == contract.missing_evidence.len(),
        "ledger missing-evidence count mismatch",
    )?;
    require(
        ledger.exit_gate.fabricated_unknown_field_count == 0,
        "ledger reports fabricated UNKNOWN data",
    )?;
    match ledger.status.as_str() {
        "PASS" => {
            require(
                evidence_complete,
                "PASS ledger has incomplete static contract",
            )?;
            require(
                ledger.exit_gate.result == "PASS",
                "PASS ledger has inconsistent exit gate",
            )?;
            require(
                ledger.permissions.cs_t00502_unlocked,
                "PASS ledger must unlock 05-02",
            )?;
        }
        "BLOCKED_BY_EVIDENCE" => {
            require(
                !contract.missing_evidence.is_empty(),
                "blocked ledger must name missing evidence",
            )?;
            require(
                !ledger.permissions.cs_t00502_unlocked,
                "blocked ledger cannot unlock 05-02",
            )?;
        }
        "FAIL" => require(
            !ledger.permissions.cs_t00502_unlocked,
            "FAIL ledger cannot unlock 05-02",
        )?,
        _ => unreachable!(),
    }
    require(
        ledger.artifact_rules.create_new_only,
        "ledger artifacts must be create-new only",
    )?;
    require(
        ledger.artifact_rules.repository_relative_paths_only,
        "ledger paths must be repository relative",
    )?;
    require(
        !ledger.artifact_rules.absolute_user_paths_allowed,
        "absolute paths are forbidden",
    )?;
    require(
        !ledger.artifact_rules.raw_peer_identity_allowed,
        "raw peer identity is forbidden",
    )?;
    require(
        !ledger.artifact_rules.process_pointers_allowed,
        "process pointers are forbidden",
    )?;
    require(
        !ledger.artifact_rules.platform_handles_allowed,
        "platform handles are forbidden",
    )
}

pub fn validate_feature_observations(
    catalog: &CoopCatalog,
    observations: &[FeatureObservation],
) -> AppResult<()> {
    let by_id: BTreeMap<_, _> = observations
        .iter()
        .map(|item| (item.slot_id.as_str(), item))
        .collect();
    require(
        by_id.len() == observations.len(),
        "duplicate feature observation",
    )?;
    for slot in &catalog.inherited_hook_transaction.new_slots {
        let observation = by_id.get(slot.id.as_str()).ok_or_else(|| {
            AppError::evidence(format!("missing feature observation: {}", slot.id))
        })?;
        require(
            observation.match_rvas.len() == 1,
            "feature must have exactly one observed match",
        )?;
        require(
            observation.match_rvas[0] == slot.rva,
            "feature matched the wrong RVA",
        )?;
        require(
            observation.observed_bytes.as_deref() == Some(slot.expected_bytes.as_str()),
            "observed call bytes differ from catalog",
        )?;
        require(
            observation.observed_target_rva.as_deref() == Some(slot.target_rva.as_str()),
            "observed REL32 target differs from catalog",
        )?;
    }
    Ok(())
}

pub fn build_inspection(
    graph: &CoopStaticGraph,
    catalog: &CoopCatalog,
    feature_results: Vec<FeatureObservation>,
    mut artifact_bindings: Vec<InspectionBinding>,
    mut checks: Vec<InspectionCheck>,
    missing_evidence: Vec<String>,
) -> AppResult<CoopStaticInspection> {
    validate_graph(graph)?;
    validate_catalog(catalog, graph)?;
    validate_feature_observations(catalog, &feature_results)?;
    let mut missing_evidence = missing_evidence;
    missing_evidence.extend(graph.unresolved_claims.iter().cloned());
    missing_evidence.sort();
    missing_evidence.dedup();
    let result = if missing_evidence.is_empty() {
        "PASS"
    } else {
        "BLOCKED_BY_EVIDENCE"
    };
    let unlocked = result == "PASS";
    require(
        catalog.permissions.cs_t00502_unlocked == unlocked
            && graph.permissions.cs_t00502_unlocked == unlocked,
        "05-02 permission conflicts with evidence result",
    )?;
    artifact_bindings.sort_by(|left, right| left.role.cmp(&right.role));
    checks.sort_by(|left, right| left.id.cmp(&right.id));
    Ok(CoopStaticInspection {
        schema_version: 1,
        inspection_identity: "IsaacCsT0-CoopStaticInspection-v1".to_owned(),
        work_package: COOP_WORK_PACKAGE.to_owned(),
        result: result.to_owned(),
        target: catalog.target.clone(),
        artifact_bindings,
        checks,
        graph_identity: graph.graph_identity.clone(),
        catalog_identity: catalog.catalog_identity.clone(),
        graph_summary: GraphSummary {
            partition_count: graph.partitions.len(),
            node_count: graph.nodes.len(),
            edge_count: graph.edges.len(),
            required_route_count: graph.required_routes.len(),
            new_slot_count: catalog.inherited_hook_transaction.new_slots.len(),
            combined_slot_count: catalog.inherited_hook_transaction.combined_slot_count,
        },
        feature_results,
        missing_evidence,
        permissions: InspectionPermissions {
            active_input_allowed: false,
            runtime_mutation_allowed: false,
            t005_unlocked: false,
            cs_t00502_unlocked: unlocked,
        },
    })
}

pub fn validate_inspection(inspection: &CoopStaticInspection) -> AppResult<()> {
    validate_target(&inspection.target)?;
    validate_permissions(
        inspection.permissions.active_input_allowed,
        inspection.permissions.runtime_mutation_allowed,
        inspection.permissions.t005_unlocked,
    )?;
    require(
        !inspection.artifact_bindings.is_empty(),
        "inspection has no artifact bindings",
    )?;
    require(
        inspection
            .missing_evidence
            .windows(2)
            .all(|pair| pair[0] < pair[1]),
        "missing evidence must be sorted and unique",
    )?;
    for binding in &inspection.artifact_bindings {
        require(
            !binding.role.is_empty() && !binding.identity.is_empty() && is_sha256(&binding.sha256),
            "invalid inspection binding",
        )?;
    }
    require(!inspection.checks.is_empty(), "inspection has no checks")?;
    require(
        inspection.checks.iter().all(|check| check.passed),
        "inspection contains a failed check",
    )?;
    require(
        inspection.graph_summary.partition_count == 3
            && inspection.graph_summary.required_route_count == 2
            && inspection.graph_summary.new_slot_count == 8
            && inspection.graph_summary.combined_slot_count == 53,
        "inspection graph summary is inconsistent",
    )?;
    match inspection.result.as_str() {
        "PASS" => {
            require(
                inspection.missing_evidence.is_empty(),
                "PASS cannot contain missing evidence",
            )?;
            require(
                inspection.permissions.cs_t00502_unlocked,
                "PASS must unlock 05-02",
            )?;
        }
        "BLOCKED_BY_EVIDENCE" => {
            require(
                !inspection.missing_evidence.is_empty(),
                "blocked result needs missing evidence",
            )?;
            require(
                !inspection.permissions.cs_t00502_unlocked,
                "blocked result cannot unlock 05-02",
            )?;
        }
        "FAIL" => require(
            !inspection.permissions.cs_t00502_unlocked,
            "FAIL cannot unlock 05-02",
        )?,
        _ => return Err(AppError::evidence("invalid inspection result")),
    }
    Ok(())
}

pub fn write_json_create_new(path: &Path, value: &impl Serialize) -> AppResult<()> {
    let bytes = serde_json::to_vec_pretty(value)
        .map_err(|error| AppError::evidence(format!("serialize coop artifact: {error}")))?;
    let mut file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .map_err(|error| AppError::evidence(format!("create {}: {error}", path.display())))?;
    file.write_all(&bytes)
        .and_then(|()| file.write_all(b"\n"))
        .map_err(|error| AppError::evidence(format!("write {}: {error}", path.display())))
}

fn validate_target(target: &TargetIdentity) -> AppResult<()> {
    require(
        is_sha256(&target.canonical_sha256),
        "invalid canonical target hash",
    )?;
    require(
        is_sha256(&target.observed_approved_unpacked_sha256),
        "invalid unpacked target hash",
    )?;
    require(target.machine == "0x014C", "target must be x86 PE")?;
    require(
        target.preferred_image_base == "0x00400000",
        "unexpected preferred image base",
    )
}

fn validate_permissions(active: bool, mutation: bool, t005: bool) -> AppResult<()> {
    require(!active, "active input permission must remain false")?;
    require(!mutation, "runtime mutation permission must remain false")?;
    require(!t005, "T0-05 permission must remain false")
}

fn validate_locator(node: &GraphNode) -> AppResult<()> {
    match node.locator.kind.as_str() {
        "CALL_REL32" => {
            let expected = node
                .locator
                .expected_bytes
                .as_deref()
                .ok_or_else(|| AppError::evidence("REL32 locator lacks expected bytes"))?;
            let target = node
                .locator
                .target_rva
                .as_deref()
                .ok_or_else(|| AppError::evidence("REL32 locator lacks target RVA"))?;
            validate_rel32(&node.locator.rva, expected, target)
        }
        "FUNCTION_ENTRY" => {
            require(
                node.locator
                    .expected_bytes
                    .as_deref()
                    .is_some_and(|bytes| !bytes.is_empty()),
                "function entry lacks bytes",
            )?;
            require(
                node.locator.target_rva.is_none(),
                "function entry must not have a REL32 target",
            )
        }
        "DATA_ACCESS" => Ok(()),
        _ => Err(AppError::evidence("unsupported graph locator kind")),
    }
}

fn validate_masked_feature(feature: &MaskedFeature) -> AppResult<()> {
    require(
        feature.section == ".text",
        "observer feature must be in .text",
    )?;
    let pattern = decode_hex(&feature.pattern_hex, "feature pattern")?;
    let mask = decode_hex(&feature.mask_hex, "feature mask")?;
    require(
        !pattern.is_empty() && pattern.len() == mask.len(),
        "feature bytes/mask length mismatch",
    )?;
    require(
        mask.iter().all(|byte| matches!(byte, 0x00 | 0xFF)),
        "feature mask must use 00/FF bytes",
    )?;
    require(
        mask.contains(&0xFF),
        "feature mask cannot be fully wildcarded",
    )
}

fn validate_rel32(rva: &str, expected_bytes: &str, target_rva: &str) -> AppResult<()> {
    let call_rva = parse_rva(rva)?;
    let target = parse_rva(target_rva)?;
    let bytes = decode_hex(expected_bytes, "REL32 expected bytes")?;
    require(
        bytes.len() == 5 && bytes[0] == 0xE8,
        "expected bytes are not CALL REL32",
    )?;
    let displacement = i32::from_le_bytes(bytes[1..5].try_into().unwrap()) as i64;
    let decoded = call_rva as i64 + 5 + displacement;
    require(
        decoded >= 0 && decoded as u32 == target,
        "REL32 target does not match expected bytes",
    )
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

fn is_sha256(value: &str) -> bool {
    value.len() == 64 && value.bytes().all(|byte| byte.is_ascii_hexdigit())
}

fn reject_authority(value: &str) -> AppResult<()> {
    require(
        !value.to_ascii_lowercase().contains("authority"),
        "unproven Authority naming is forbidden",
    )
}

fn validate_sensitive_strings<'a>(values: impl Iterator<Item = &'a String>) -> AppResult<()> {
    for value in values {
        let lower = value.to_ascii_lowercase();
        require(
            !(value.len() > 2
                && value.as_bytes()[1] == b':'
                && matches!(value.as_bytes()[2], b'\\' | b'/')),
            "absolute path leak",
        )?;
        require(!value.starts_with("/"), "absolute path leak")?;
        require(
            !lower.contains("pid=")
                && !lower.contains("process pointer")
                && !lower.contains("platform handle"),
            "process identity or pointer leak",
        )?;
    }
    Ok(())
}

fn require(condition: bool, message: impl Into<String>) -> AppResult<()> {
    if condition {
        Ok(())
    } else {
        Err(AppError::evidence(message))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const GRAPH_JSON: &[u8] = include_bytes!("../coop-static-graph-v1.json");
    const CATALOG_JSON: &[u8] = include_bytes!("../coop-catalog-v2.json");
    const LEDGER_JSON: &[u8] = include_bytes!("../ledger/isaac-cs-t0-coop-ledger-v2.json");

    fn fixtures() -> (CoopStaticGraph, CoopCatalog) {
        (
            parse_graph_json(GRAPH_JSON).unwrap(),
            parse_catalog_json(CATALOG_JSON).unwrap(),
        )
    }

    fn observations(catalog: &CoopCatalog) -> Vec<FeatureObservation> {
        catalog
            .inherited_hook_transaction
            .new_slots
            .iter()
            .map(|slot| FeatureObservation {
                slot_id: slot.id.clone(),
                match_rvas: vec![slot.rva.clone()],
                observed_bytes: Some(slot.expected_bytes.clone()),
                observed_target_rva: Some(slot.target_rva.clone()),
            })
            .collect()
    }

    fn bindings() -> Vec<InspectionBinding> {
        vec![InspectionBinding {
            role: "GRAPH".to_owned(),
            identity: COOP_GRAPH_IDENTITY.to_owned(),
            sha256: "F87DA1DF383B7B1AE3724A4DB4EAAE791B9F42B68AF9210AD1727F71D9458934".to_owned(),
        }]
    }

    fn checks() -> Vec<InspectionCheck> {
        vec![InspectionCheck {
            id: "STATIC_GRAPH".to_owned(),
            passed: true,
        }]
    }

    #[test]
    fn frozen_graph_and_catalog_validate() {
        let (graph, catalog) = fixtures();
        validate_graph(&graph).unwrap();
        validate_catalog(&catalog, &graph).unwrap();
        let inspection = build_inspection(
            &graph,
            &catalog,
            observations(&catalog),
            bindings(),
            checks(),
            vec![],
        )
        .unwrap();
        assert_eq!(inspection.result, "PASS");
        validate_inspection(&inspection).unwrap();
        validate_ledger(&parse_ledger_json(LEDGER_JSON).unwrap()).unwrap();
    }

    #[test]
    fn strict_schema_rejects_unknown_field() {
        let mut json: serde_json::Value = serde_json::from_slice(GRAPH_JSON).unwrap();
        json["surprise"] = serde_json::json!(true);
        assert!(parse_graph_json(&serde_json::to_vec(&json).unwrap()).is_err());
    }

    #[test]
    fn rel32_byte_and_target_tampering_is_rejected() {
        let (mut graph, _) = fixtures();
        let node = graph
            .nodes
            .iter_mut()
            .find(|node| node.id == "native-exit-call")
            .unwrap();
        node.locator.expected_bytes = Some("E8D0EDFFFF".to_owned());
        assert!(validate_graph(&graph).is_err());
        let (mut graph, _) = fixtures();
        graph
            .nodes
            .iter_mut()
            .find(|node| node.id == "native-exit-call")
            .unwrap()
            .locator
            .target_rva = Some("0x005BA001".to_owned());
        assert!(validate_graph(&graph).is_err());
    }

    #[test]
    fn masked_feature_tampering_and_non_unique_matches_are_rejected() {
        let (graph, mut catalog) = fixtures();
        catalog.inherited_hook_transaction.new_slots[0]
            .feature
            .mask_hex = "FF".to_owned();
        assert!(validate_catalog(&catalog, &graph).is_err());
        let (_graph, catalog) = fixtures();
        let mut scanned = observations(&catalog);
        scanned[0].match_rvas.clear();
        assert!(validate_feature_observations(&catalog, &scanned).is_err());
        scanned[0].match_rvas = vec!["0x002FADA7".to_owned(), "0x002FADA8".to_owned()];
        assert!(validate_feature_observations(&catalog, &scanned).is_err());
    }

    #[test]
    fn missing_exit_hud_or_lua_nodes_breaks_required_routes() {
        for node_id in [
            "native-exit-call",
            "join-hud-rebuild-call",
            "lua-player-publication",
        ] {
            let (mut graph, _) = fixtures();
            graph.nodes.retain(|node| node.id != node_id);
            assert!(validate_graph(&graph).is_err(), "{node_id}");
        }
    }

    #[test]
    fn online_partition_cannot_enter_positive_route() {
        let (mut graph, _) = fixtures();
        graph
            .nodes
            .iter_mut()
            .find(|node| node.id == "native-exit-call")
            .unwrap()
            .partition = "ORIGINAL_ONLINE_FORBIDDEN".to_owned();
        assert!(validate_graph(&graph).is_err());
    }

    #[test]
    fn authority_naming_is_rejected() {
        let (mut graph, _) = fixtures();
        graph.nodes[0].role = "AUTHORITY_TICK".to_owned();
        assert!(validate_graph(&graph).is_err());
    }

    #[test]
    fn unknown_observation_cannot_guess_rva_or_abi() {
        let (graph, mut catalog) = fixtures();
        catalog.excluded_observations[0].rva = Some("0x00554F84".to_owned());
        assert!(validate_catalog(&catalog, &graph).is_err());
    }

    #[test]
    fn permissions_must_remain_false() {
        let (mut graph, catalog) = fixtures();
        graph.permissions.active_input_allowed = true;
        assert!(validate_graph(&graph).is_err());
        let (graph, mut catalog) = (parse_graph_json(GRAPH_JSON).unwrap(), catalog);
        catalog.permissions.runtime_mutation_allowed = true;
        assert!(validate_catalog(&catalog, &graph).is_err());
    }

    #[test]
    fn slots_must_be_45_through_52_and_restore_in_reverse() {
        let (graph, mut catalog) = fixtures();
        catalog.inherited_hook_transaction.new_slots[1].ordinal = 45;
        assert!(validate_catalog(&catalog, &graph).is_err());
        let (graph, mut catalog) = fixtures();
        catalog.inherited_hook_transaction.restore_order.swap(0, 1);
        assert!(validate_catalog(&catalog, &graph).is_err());
        let (graph, mut catalog) = fixtures();
        catalog.inherited_hook_transaction.new_slots[1].rva = "0x002FADA8".to_owned();
        catalog.inherited_hook_transaction.new_slots[1].expected_bytes = "E893FE2B00".to_owned();
        catalog.inherited_hook_transaction.new_slots[1].target_rva = "0x005BAC40".to_owned();
        assert!(validate_catalog(&catalog, &graph).is_err());
    }

    #[test]
    fn absolute_paths_pointer_identity_and_handles_are_rejected() {
        for leaked in [
            "C:\\private\\isaac.exe",
            "/private/isaac",
            "PID=99",
            "process pointer 0x123",
            "platform handle 7",
        ] {
            let (mut graph, _) = fixtures();
            graph.nodes[0].owner = leaked.to_owned();
            assert!(validate_graph(&graph).is_err(), "{leaked}");
        }
    }

    #[test]
    fn missing_evidence_cannot_be_reported_as_pass() {
        let (graph, mut catalog) = fixtures();
        catalog.permissions.cs_t00502_unlocked = false;
        let mut blocked = build_inspection(
            &graph,
            &catalog,
            observations(&catalog),
            bindings(),
            checks(),
            vec!["exit-second-argument".to_owned()],
        )
        .unwrap_err();
        assert!(blocked.message.contains("permission conflicts"));

        let mut inspection = CoopStaticInspection {
            schema_version: 1,
            inspection_identity: "IsaacCsT0-CoopStaticInspection-v1".to_owned(),
            work_package: COOP_WORK_PACKAGE.to_owned(),
            result: "PASS".to_owned(),
            target: catalog.target,
            artifact_bindings: bindings(),
            checks: checks(),
            graph_identity: graph.graph_identity,
            catalog_identity: catalog.catalog_identity,
            graph_summary: GraphSummary {
                partition_count: graph.partitions.len(),
                node_count: graph.nodes.len(),
                edge_count: graph.edges.len(),
                required_route_count: graph.required_routes.len(),
                new_slot_count: 8,
                combined_slot_count: 53,
            },
            feature_results: vec![],
            missing_evidence: vec!["missing".to_owned()],
            permissions: InspectionPermissions {
                active_input_allowed: false,
                runtime_mutation_allowed: false,
                t005_unlocked: false,
                cs_t00502_unlocked: true,
            },
        };
        assert!(validate_inspection(&inspection).is_err());
        inspection.result = "BLOCKED_BY_EVIDENCE".to_owned();
        inspection.permissions.cs_t00502_unlocked = false;
        validate_inspection(&inspection).unwrap();
        blocked.message.clear();
    }

    #[test]
    fn pass_ledger_rejects_missing_evidence_and_permission_mismatch() {
        let mut ledger = parse_ledger_json(LEDGER_JSON).unwrap();
        ledger.static_contract.missing_evidence = vec!["opaque-exit-argument".to_owned()];
        ledger.exit_gate.missing_evidence_count = 1;
        assert!(validate_ledger(&ledger).is_err());
        let mut ledger = parse_ledger_json(LEDGER_JSON).unwrap();
        ledger.permissions.cs_t00502_unlocked = false;
        assert!(validate_ledger(&ledger).is_err());
    }

    #[test]
    fn create_new_writer_refuses_overwrite_without_changing_file() {
        let directory = tempfile::tempdir().unwrap();
        let path = directory.path().join("inspection.json");
        let value = serde_json::json!({"result": "PASS"});
        write_json_create_new(&path, &value).unwrap();
        let before = std::fs::read(&path).unwrap();
        assert!(write_json_create_new(&path, &serde_json::json!({"result": "FAIL"})).is_err());
        assert_eq!(std::fs::read(&path).unwrap(), before);
    }
}
