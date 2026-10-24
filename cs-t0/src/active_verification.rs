use std::collections::{BTreeMap, BTreeSet};
use std::fs;
use std::fs::OpenOptions;
use std::io::Write;
use std::path::Path;

use serde::{Deserialize, Serialize};

use crate::active_catalog::{ACTIVE_OWNED_SLOT_COUNT, ACTIVE_SCENARIOS, ACTIVE_SLOT_COUNT};
use crate::error::{AppError, AppResult, UsageContext};
use crate::pe::sha256_hex;

pub const ACTIVE_VERIFICATION_SCHEMA_VERSION: u32 = 1;
pub const ACTIVE_VERIFICATION_IDENTITY: &str = "IsaacCsT0-ActiveVerification-v1";
pub const ACTIVE_VERIFICATION_STATUS: &str = "BLOCKED_BY_DEFERRED_SCHEMA4_NATIVE";
pub const ACTIVE_SEMANTIC_CONTRACT_COMPLETE: bool = true;
pub const ACTIVE_RUNTIME_CONTRACT_COMPLETE: bool = true;
const ACTIVE_VERIFICATION_V1_RUNTIME_CONTRACT_COMPLETE: bool = false;

pub const ACTIVE_VERIFICATION_V2_SCHEMA_VERSION: u32 = 2;
pub const ACTIVE_VERIFICATION_V2_IDENTITY: &str = "IsaacCsT0-ActiveVerification-v2";
pub const ACTIVE_VERIFICATION_V2_READY_STATUS: &str =
    "AUTOMATED_RUNTIME_COMPLETE_AWAITING_ACTIVE_EVIDENCE";
pub const ACTIVE_VERIFICATION_V2_BLOCKED_STATUS: &str = "BLOCKED_BY_RUNTIME_VERIFICATION";

pub const ACTIVE_VERIFICATION_V2_CHECK_IDS: [&str; 27] = [
    "rust-test",
    "rust-fmt",
    "rust-check-all-targets",
    "rust-clippy-deny-warnings",
    "isaac-test-mcp",
    "schema4-manifest-companion-binding",
    "schema4-loop-context-replay",
    "schema4-disarmed-companion-forbidden",
    "schema4-ordered-eighteen-scenario-semantics",
    "schema4-repeatability-normalization",
    "schema4-mutation-fail-closed",
    "schema4-active-event-288-byte-static-assert",
    "schema4-active-session-id-16-byte-static-assert",
    "schema4-active-shared-block-720-offset-static-assert",
    "schema4-active-numeric-codebook",
    "schema4-active-process-creation-binding",
    "schema4-active-unpacked-equivalence-binding",
    "schema4-native-debug-harness",
    "schema4-native-release-harness",
    "schema4-45-slot-install-transaction",
    "schema4-relative-mid-install-rollback",
    "schema4-pointer-mid-install-rollback",
    "schema4-bool-x87-original-once-abi",
    "schema4-staged-shutdown-recovery",
    "schema4-active-helper-cli",
    "schema4-rust-live-orchestration",
    "schema4-cross-language-real-mapping",
];

const ACTIVE_VERIFICATION_V2_SEMANTIC_CHECK_COUNT: usize = 11;

const ACTIVE_VERIFICATION_CHECKS: [(&str, bool); 15] = [
    ("rust-test", true),
    ("rust-fmt", true),
    ("rust-check-all-targets", true),
    ("rust-clippy-deny-warnings", true),
    ("offline-native-debug-harness", true),
    ("offline-native-release-harness", true),
    ("schema4-active-event-288-byte-static-assert", true),
    ("isaac-test-mcp", true),
    ("schema4-manifest-companion-binding", true),
    ("schema4-loop-context-replay", true),
    ("schema4-disarmed-companion-forbidden", true),
    ("schema4-ordered-eighteen-scenario-semantics", true),
    ("schema4-repeatability-normalization", true),
    ("schema4-mutation-fail-closed", true),
    ("schema4-native-runtime", false),
];

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ActiveVerificationCheck {
    pub id: String,
    pub passed: bool,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ActiveVerificationV1 {
    pub schema_version: u32,
    pub verification_identity: String,
    pub status: String,
    pub semantic_contract_complete: bool,
    pub runtime_contract_complete: bool,
    pub active_artifact_schema_version: u32,
    pub active_ipc_schema_version: u32,
    pub active_matrix_schema_version: u32,
    pub active_matrix_identity: String,
    pub required_scenarios: Vec<String>,
    pub companion_loop_required_for_pipeline: bool,
    pub companion_loop_forbidden_for_disarmed: bool,
    pub restored_slot_count: u32,
    pub active_owned_slot_count: u32,
    pub checks: Vec<ActiveVerificationCheck>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ActiveVerificationV2 {
    pub schema_version: u32,
    pub verification_identity: String,
    pub status: String,
    pub semantic_contract_complete: bool,
    pub runtime_contract_complete: bool,
    pub active_input_allowed: bool,
    pub active_artifact_schema_version: u32,
    pub active_ipc_schema_version: u32,
    pub active_matrix_schema_version: u32,
    pub active_matrix_identity: String,
    pub required_scenarios: Vec<String>,
    pub companion_loop_required_for_pipeline: bool,
    pub companion_loop_forbidden_for_disarmed: bool,
    pub active_event_size: u32,
    pub active_session_id_size: u32,
    pub active_events_offset: u32,
    pub active_shared_block_size: u32,
    pub restored_slot_count: u32,
    pub v5_owned_slot_count: u32,
    pub active_owned_slot_count: u32,
    pub suite_evidence: Vec<ActiveVerificationSuiteEvidence>,
    pub checks: Vec<ActiveVerificationCheck>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ActiveVerificationSuiteEvidence {
    pub id: String,
    pub configuration: String,
    pub exit_code: i32,
    pub case_count: u32,
    pub binary_sha256: String,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct ValidatedActiveVerification {
    pub sha256: String,
    pub schema_version: u32,
    pub verification_identity: String,
    pub status: String,
    pub semantic_contract_complete: bool,
    pub runtime_contract_complete: bool,
    pub active_input_allowed: bool,
}

pub fn frozen_active_verification_v1() -> ActiveVerificationV1 {
    ActiveVerificationV1 {
        schema_version: ACTIVE_VERIFICATION_SCHEMA_VERSION,
        verification_identity: ACTIVE_VERIFICATION_IDENTITY.to_owned(),
        status: ACTIVE_VERIFICATION_STATUS.to_owned(),
        semantic_contract_complete: ACTIVE_SEMANTIC_CONTRACT_COMPLETE,
        runtime_contract_complete: ACTIVE_VERIFICATION_V1_RUNTIME_CONTRACT_COMPLETE,
        active_artifact_schema_version: 4,
        active_ipc_schema_version: 4,
        active_matrix_schema_version: 2,
        active_matrix_identity: "IsaacCsT0-ActiveMatrix-v2".to_owned(),
        required_scenarios: ACTIVE_SCENARIOS
            .iter()
            .map(|scenario| (*scenario).to_owned())
            .collect(),
        companion_loop_required_for_pipeline: true,
        companion_loop_forbidden_for_disarmed: true,
        restored_slot_count: ACTIVE_SLOT_COUNT as u32,
        active_owned_slot_count: ACTIVE_OWNED_SLOT_COUNT as u32,
        checks: ACTIVE_VERIFICATION_CHECKS
            .iter()
            .map(|(id, passed)| ActiveVerificationCheck {
                id: (*id).to_owned(),
                passed: *passed,
            })
            .collect(),
    }
}

pub fn validate_active_verification(document: &ActiveVerificationV1) -> AppResult<()> {
    if document != &frozen_active_verification_v1() {
        return Err(AppError::evidence(
            "active verification is not the exact frozen v1 semantic-complete/runtime-blocked identity",
        ));
    }
    Ok(())
}

pub fn generate_active_verification_v2(
    results: &BTreeMap<String, bool>,
    suite_evidence: &[ActiveVerificationSuiteEvidence],
) -> AppResult<ActiveVerificationV2> {
    let expected = ACTIVE_VERIFICATION_V2_CHECK_IDS
        .iter()
        .map(|id| (*id).to_owned())
        .collect::<BTreeSet<_>>();
    let actual = results.keys().cloned().collect::<BTreeSet<_>>();
    if actual != expected {
        return Err(AppError::evidence(
            "active verification v2 results do not contain the exact required subgate set",
        ));
    }
    validate_active_suite_evidence(suite_evidence)?;
    let checks = ACTIVE_VERIFICATION_V2_CHECK_IDS
        .iter()
        .map(|id| ActiveVerificationCheck {
            id: (*id).to_owned(),
            passed: results.get(*id).copied().unwrap_or(false),
        })
        .collect::<Vec<_>>();
    let semantic_contract_complete = checks[..ACTIVE_VERIFICATION_V2_SEMANTIC_CHECK_COUNT]
        .iter()
        .all(|check| check.passed);
    let runtime_contract_complete = semantic_contract_complete
        && checks[ACTIVE_VERIFICATION_V2_SEMANTIC_CHECK_COUNT..]
            .iter()
            .all(|check| check.passed);
    Ok(ActiveVerificationV2 {
        schema_version: ACTIVE_VERIFICATION_V2_SCHEMA_VERSION,
        verification_identity: ACTIVE_VERIFICATION_V2_IDENTITY.to_owned(),
        status: if runtime_contract_complete {
            ACTIVE_VERIFICATION_V2_READY_STATUS
        } else {
            ACTIVE_VERIFICATION_V2_BLOCKED_STATUS
        }
        .to_owned(),
        semantic_contract_complete,
        runtime_contract_complete,
        active_input_allowed: false,
        active_artifact_schema_version: 4,
        active_ipc_schema_version: 4,
        active_matrix_schema_version: 2,
        active_matrix_identity: "IsaacCsT0-ActiveMatrix-v2".to_owned(),
        required_scenarios: ACTIVE_SCENARIOS
            .iter()
            .map(|scenario| (*scenario).to_owned())
            .collect(),
        companion_loop_required_for_pipeline: true,
        companion_loop_forbidden_for_disarmed: true,
        active_event_size: 288,
        active_session_id_size: 16,
        active_events_offset: 720,
        active_shared_block_size: 18_875_088,
        restored_slot_count: ACTIVE_SLOT_COUNT as u32,
        v5_owned_slot_count: (ACTIVE_SLOT_COUNT - ACTIVE_OWNED_SLOT_COUNT) as u32,
        active_owned_slot_count: ACTIVE_OWNED_SLOT_COUNT as u32,
        suite_evidence: suite_evidence.to_vec(),
        checks,
    })
}

fn validate_active_suite_evidence(
    suite_evidence: &[ActiveVerificationSuiteEvidence],
) -> AppResult<()> {
    let expected = [
        ("schema4-native-debug", "Debug"),
        ("schema4-native-release-ltcg", "Release-LTCG"),
    ];
    if suite_evidence.len() != expected.len() {
        return Err(AppError::evidence(
            "active verification v2 requires exact Debug and Release/LTCG suite evidence",
        ));
    }
    for (suite, (id, configuration)) in suite_evidence.iter().zip(expected) {
        let valid_hash = suite.binary_sha256.len() == 64
            && suite
                .binary_sha256
                .bytes()
                .all(|byte| byte.is_ascii_digit() || (b'A'..=b'F').contains(&byte))
            && suite.binary_sha256.bytes().any(|byte| byte != b'0');
        if suite.id != id
            || suite.configuration != configuration
            || suite.exit_code != 0
            || suite.case_count != 262
            || !valid_hash
        {
            return Err(AppError::evidence(
                "active verification v2 suite evidence has the wrong identity, result, case count, or binary SHA-256",
            ));
        }
    }
    if suite_evidence[0].binary_sha256 == suite_evidence[1].binary_sha256 {
        return Err(AppError::evidence(
            "Debug and Release/LTCG active harness evidence must bind distinct binaries",
        ));
    }
    Ok(())
}

pub fn validate_active_verification_v2(document: &ActiveVerificationV2) -> AppResult<()> {
    let mut results = BTreeMap::new();
    for check in &document.checks {
        if results.insert(check.id.clone(), check.passed).is_some() {
            return Err(AppError::evidence(
                "active verification v2 duplicates a subgate id",
            ));
        }
    }
    let expected = generate_active_verification_v2(&results, &document.suite_evidence)?;
    if document != &expected {
        return Err(AppError::evidence(
            "active verification is not the exact generated v2 implementation-gate identity",
        ));
    }
    Ok(())
}

pub fn write_active_verification_v2(
    output: &Path,
    results: &BTreeMap<String, bool>,
    suite_evidence: &[ActiveVerificationSuiteEvidence],
) -> AppResult<ActiveVerificationV2> {
    let document = generate_active_verification_v2(results, suite_evidence)?;
    validate_active_verification_v2(&document)?;
    let mut file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(output)
        .usage_context(format!("refusing to overwrite {}", output.display()))?;
    serde_json::to_writer_pretty(&mut file, &document)
        .usage_context(format!("cannot serialize {}", output.display()))?;
    file.write_all(b"\n")
        .usage_context(format!("cannot finish {}", output.display()))?;
    Ok(document)
}

pub fn read_active_verification(path: &Path) -> AppResult<ValidatedActiveVerification> {
    let bytes = fs::read(path).usage_context(format!(
        "cannot read active verification {}",
        path.display()
    ))?;
    #[derive(Deserialize)]
    #[serde(rename_all = "camelCase")]
    struct VersionProbe {
        schema_version: u32,
        verification_identity: String,
    }
    let probe: VersionProbe = serde_json::from_slice(&bytes).usage_context(format!(
        "cannot parse active verification {}",
        path.display()
    ))?;
    let sha256 = sha256_hex(&bytes);
    match (probe.schema_version, probe.verification_identity.as_str()) {
        (ACTIVE_VERIFICATION_SCHEMA_VERSION, ACTIVE_VERIFICATION_IDENTITY) => {
            let document: ActiveVerificationV1 = serde_json::from_slice(&bytes)
                .usage_context("cannot parse active verification v1")?;
            validate_active_verification(&document)?;
            Ok(ValidatedActiveVerification {
                sha256,
                schema_version: document.schema_version,
                verification_identity: document.verification_identity,
                status: document.status,
                semantic_contract_complete: document.semantic_contract_complete,
                runtime_contract_complete: document.runtime_contract_complete,
                active_input_allowed: false,
            })
        }
        (ACTIVE_VERIFICATION_V2_SCHEMA_VERSION, ACTIVE_VERIFICATION_V2_IDENTITY) => {
            let document: ActiveVerificationV2 = serde_json::from_slice(&bytes)
                .usage_context("cannot parse active verification v2")?;
            validate_active_verification_v2(&document)?;
            Ok(ValidatedActiveVerification {
                sha256,
                schema_version: document.schema_version,
                verification_identity: document.verification_identity,
                status: document.status,
                semantic_contract_complete: document.semantic_contract_complete,
                runtime_contract_complete: document.runtime_contract_complete,
                active_input_allowed: document.active_input_allowed,
            })
        }
        _ => Err(AppError::evidence(
            "active verification has an unknown schema or identity",
        )),
    }
}

pub fn read_runtime_active_verification_v2(path: &Path) -> AppResult<ValidatedActiveVerification> {
    let validated = read_active_verification(path)?;
    if validated.schema_version != ACTIVE_VERIFICATION_V2_SCHEMA_VERSION
        || validated.verification_identity != ACTIVE_VERIFICATION_V2_IDENTITY
        || validated.status != ACTIVE_VERIFICATION_V2_READY_STATUS
        || !validated.semantic_contract_complete
        || !validated.runtime_contract_complete
        || validated.active_input_allowed
    {
        return Err(AppError::evidence(
            "active live execution requires exact runtime-complete verification v2 with activeInputAllowed=false",
        ));
    }
    Ok(validated)
}

#[cfg(test)]
mod tests {
    use super::*;

    type VerificationMutation = Box<dyn Fn(&mut ActiveVerificationV1)>;

    fn v2_results(passed: bool) -> BTreeMap<String, bool> {
        ACTIVE_VERIFICATION_V2_CHECK_IDS
            .iter()
            .map(|id| ((*id).to_owned(), passed))
            .collect()
    }

    fn v2_suites() -> Vec<ActiveVerificationSuiteEvidence> {
        vec![
            ActiveVerificationSuiteEvidence {
                id: "schema4-native-debug".to_owned(),
                configuration: "Debug".to_owned(),
                exit_code: 0,
                case_count: 262,
                binary_sha256: "A".repeat(64),
            },
            ActiveVerificationSuiteEvidence {
                id: "schema4-native-release-ltcg".to_owned(),
                configuration: "Release-LTCG".to_owned(),
                exit_code: 0,
                case_count: 262,
                binary_sha256: "B".repeat(64),
            },
        ]
    }

    #[test]
    fn frozen_verification_is_semantic_complete_and_runtime_blocked() {
        let document = frozen_active_verification_v1();
        validate_active_verification(&document).unwrap();
        assert!(document.semantic_contract_complete);
        assert!(!document.runtime_contract_complete);
        assert_ne!(
            ACTIVE_RUNTIME_CONTRACT_COMPLETE,
            document.runtime_contract_complete
        );
        assert_eq!(document.status, ACTIVE_VERIFICATION_STATUS);
        assert_eq!(document.required_scenarios.len(), 18);
        assert_eq!(document.restored_slot_count, 45);
        assert_eq!(document.active_owned_slot_count, 13);
    }

    #[test]
    fn verification_mutations_fail_closed() {
        let mutations: Vec<VerificationMutation> = vec![
            Box::new(|document| document.schema_version = 2),
            Box::new(|document| document.verification_identity.push_str("-mutation")),
            Box::new(|document| document.status = "PASS".to_owned()),
            Box::new(|document| document.semantic_contract_complete = false),
            Box::new(|document| document.runtime_contract_complete = true),
            Box::new(|document| document.required_scenarios.swap(0, 1)),
            Box::new(|document| document.companion_loop_required_for_pipeline = false),
            Box::new(|document| document.companion_loop_forbidden_for_disarmed = false),
            Box::new(|document| document.restored_slot_count = 44),
            Box::new(|document| document.checks[0].passed = false),
        ];
        for mutate in mutations {
            let mut document = frozen_active_verification_v1();
            mutate(&mut document);
            assert!(validate_active_verification(&document).is_err());
        }
    }

    #[test]
    fn verification_file_hash_binds_exact_bytes() {
        let directory = tempfile::tempdir().unwrap();
        let path = directory.path().join("active-verification-v1.json");
        let bytes = serde_json::to_vec_pretty(&frozen_active_verification_v1()).unwrap();
        fs::write(&path, &bytes).unwrap();
        let validated = read_active_verification(&path).unwrap();
        assert_eq!(validated.sha256, sha256_hex(&bytes));
        assert!(validated.semantic_contract_complete);
        assert!(!validated.runtime_contract_complete);
    }

    #[test]
    fn checked_in_verification_matches_the_frozen_model() {
        let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("input-active-verification-v1.json");
        let validated = read_active_verification(&path).unwrap();
        assert!(validated.semantic_contract_complete);
        assert!(!validated.runtime_contract_complete);
    }

    #[test]
    fn verification_v2_generation_is_runtime_complete_but_never_arms_input() {
        let document = generate_active_verification_v2(&v2_results(true), &v2_suites()).unwrap();
        validate_active_verification_v2(&document).unwrap();
        assert_eq!(document.status, ACTIVE_VERIFICATION_V2_READY_STATUS);
        assert!(document.semantic_contract_complete);
        assert!(document.runtime_contract_complete);
        assert!(!document.active_input_allowed);
        assert_eq!(document.active_session_id_size, 16);
        assert_eq!(document.active_events_offset, 720);
        assert_eq!(document.active_shared_block_size, 18_875_088);
        assert_eq!(document.v5_owned_slot_count, 32);
        assert_eq!(document.active_owned_slot_count, 13);
        assert_eq!(document.restored_slot_count, 45);
    }

    #[test]
    fn verification_v2_runtime_failure_and_mutations_fail_closed() {
        let mut results = v2_results(true);
        results.insert("schema4-native-release-harness".to_owned(), false);
        let blocked = generate_active_verification_v2(&results, &v2_suites()).unwrap();
        assert_eq!(blocked.status, ACTIVE_VERIFICATION_V2_BLOCKED_STATUS);
        assert!(blocked.semantic_contract_complete);
        assert!(!blocked.runtime_contract_complete);
        validate_active_verification_v2(&blocked).unwrap();

        let mut missing = v2_results(true);
        missing.remove("schema4-active-helper-cli");
        assert!(generate_active_verification_v2(&missing, &v2_suites()).is_err());

        let mut mutated = generate_active_verification_v2(&v2_results(true), &v2_suites()).unwrap();
        mutated.active_input_allowed = true;
        assert!(validate_active_verification_v2(&mutated).is_err());
        let mut mutated = generate_active_verification_v2(&v2_results(true), &v2_suites()).unwrap();
        mutated.checks.swap(0, 1);
        assert!(validate_active_verification_v2(&mutated).is_err());
        let mut mutated = generate_active_verification_v2(&v2_results(true), &v2_suites()).unwrap();
        mutated.active_events_offset = 712;
        assert!(validate_active_verification_v2(&mutated).is_err());
        let mut mutated = generate_active_verification_v2(&v2_results(true), &v2_suites()).unwrap();
        mutated.suite_evidence[0].case_count = 261;
        assert!(validate_active_verification_v2(&mutated).is_err());
    }

    #[test]
    fn verification_v2_writer_is_create_new_and_live_reader_requires_ready() {
        let directory = tempfile::tempdir().unwrap();
        let path = directory.path().join("active-verification-v2.json");
        write_active_verification_v2(&path, &v2_results(true), &v2_suites()).unwrap();
        let validated = read_runtime_active_verification_v2(&path).unwrap();
        assert_eq!(validated.schema_version, 2);
        assert!(!validated.active_input_allowed);
        assert!(write_active_verification_v2(&path, &v2_results(true), &v2_suites()).is_err());

        let blocked_path = directory.path().join("active-verification-v2-blocked.json");
        write_active_verification_v2(&blocked_path, &v2_results(false), &v2_suites()).unwrap();
        assert!(read_runtime_active_verification_v2(&blocked_path).is_err());
    }
}
