use std::fs::{self, OpenOptions};
use std::io::Write;
use std::path::{Path, PathBuf};

use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};

use crate::artifact::file_sha256;
use crate::error::{AppError, AppResult, UsageContext};
use crate::offline_attestation::{OfflinePrerequisiteDecision, validate_offline_prerequisite};

pub const ARM_GRANT_IDENTITY: &str = "IsaacCsT0-ActiveArmGrant-v1";
pub const ARM_CLAIM_IDENTITY: &str = "IsaacCsT0-ActiveArmClaim-v1";
pub const ARM_TOKEN_SCHEMA_VERSION: u32 = 1;

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ArmGrant {
    pub schema_version: u32,
    pub grant_identity: String,
    pub issued_at_utc: DateTime<Utc>,
    pub expires_at_utc: DateTime<Utc>,
    pub token_sha256: String,
    pub prerequisite_sha256: String,
    pub target_sha256: String,
    pub observed_target_sha256: String,
    pub target_equivalence_sha256: String,
    pub mode_gate_sha256: String,
    pub loop_gate_sha256: String,
    pub raw_prereq_sha256: String,
    pub active_plan_sha256: String,
    pub case_id: String,
    pub scenario: String,
    pub process_id: u32,
    pub process_creation_time_100ns: u64,
    pub generation: u64,
    pub game_root_identity_sha256: String,
    pub lua_generation_identity_sha256: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ArmClaim {
    pub schema_version: u32,
    pub claim_identity: String,
    pub consumed_at_utc: DateTime<Utc>,
    pub grant_sha256: String,
    pub token_sha256: String,
    pub prerequisite_sha256: String,
    pub case_id: String,
    pub scenario: String,
    pub process_id: u32,
    pub process_creation_time_100ns: u64,
    pub generation: u64,
}

pub fn parse_arm_token(value: &str) -> AppResult<[u8; 32]> {
    if value.len() != 64
        || !value.bytes().all(|byte| byte.is_ascii_hexdigit())
        || value.bytes().all(|byte| byte == b'0')
    {
        return Err(AppError::usage(
            "arm token must be a nonzero 32-byte hexadecimal value",
        ));
    }
    hex::decode(value)
        .map_err(|_| AppError::usage("arm token is not hexadecimal"))?
        .try_into()
        .map_err(|_| AppError::usage("arm token is not exactly 32 bytes"))
}

pub fn arm_token_sha256(value: &str) -> AppResult<String> {
    let token = parse_arm_token(value)?;
    Ok(hex::encode_upper(Sha256::digest(token)))
}

pub fn create_arm_grant(
    prerequisite_path: &Path,
    case_id: &str,
    scenario: &str,
    arm_token: &str,
    output: &Path,
    now: DateTime<Utc>,
) -> AppResult<ArmGrant> {
    validate_case(case_id)?;
    validate_scenario(scenario)?;
    let prerequisite = validate_offline_prerequisite(prerequisite_path, now)?;
    if prerequisite.case_id != case_id || prerequisite.scenario != scenario {
        return Err(AppError::evidence(
            "arm grant case and scenario do not match the offline prerequisite",
        ));
    }
    let grant = ArmGrant {
        schema_version: ARM_TOKEN_SCHEMA_VERSION,
        grant_identity: ARM_GRANT_IDENTITY.to_owned(),
        issued_at_utc: now,
        expires_at_utc: prerequisite.expires_at_utc,
        token_sha256: arm_token_sha256(arm_token)?,
        prerequisite_sha256: file_sha256(prerequisite_path)?,
        target_sha256: prerequisite.target_sha256.clone(),
        observed_target_sha256: prerequisite.observed_target_sha256.clone(),
        target_equivalence_sha256: prerequisite.target_equivalence_sha256.clone(),
        mode_gate_sha256: prerequisite.mode_gate_sha256.clone(),
        loop_gate_sha256: prerequisite.loop_gate_sha256.clone(),
        raw_prereq_sha256: prerequisite.raw_prereq_sha256.clone(),
        active_plan_sha256: prerequisite.active_plan_sha256.clone(),
        case_id: case_id.to_owned(),
        scenario: scenario.to_owned(),
        process_id: prerequisite.process_id,
        process_creation_time_100ns: prerequisite.process_creation_time_100ns,
        generation: prerequisite.generation,
        game_root_identity_sha256: prerequisite.game_root_identity_sha256.clone(),
        lua_generation_identity_sha256: prerequisite.lua_generation_identity_sha256.clone(),
    };
    validate_grant(&grant, &prerequisite, arm_token, prerequisite_path, now)?;
    write_new_json(output, &grant)?;
    Ok(grant)
}

pub fn validate_arm_grant(
    grant_path: &Path,
    prerequisite_path: &Path,
    arm_token: &str,
    case_id: &str,
    scenario: &str,
    now: DateTime<Utc>,
) -> AppResult<(ArmGrant, OfflinePrerequisiteDecision)> {
    let prerequisite = validate_offline_prerequisite(prerequisite_path, now)?;
    let grant: ArmGrant = serde_json::from_slice(
        &fs::read(grant_path).usage_context(format!("cannot read {}", grant_path.display()))?,
    )
    .usage_context("cannot parse arm grant")?;
    validate_grant(&grant, &prerequisite, arm_token, prerequisite_path, now)?;
    if grant.case_id != case_id || grant.scenario != scenario {
        return Err(AppError::evidence(
            "arm grant does not bind the requested case and scenario",
        ));
    }
    Ok((grant, prerequisite))
}

pub fn consume_arm_token(
    grant_path: &Path,
    prerequisite_path: &Path,
    arm_token: &str,
    case_id: &str,
    scenario: &str,
    now: DateTime<Utc>,
) -> AppResult<(ArmClaim, PathBuf)> {
    let (grant, prerequisite) = validate_arm_grant(
        grant_path,
        prerequisite_path,
        arm_token,
        case_id,
        scenario,
        now,
    )?;
    let parent = grant_path.parent().unwrap_or_else(|| Path::new("."));
    let claim_path = parent.join(format!("claim-{}.json", grant.token_sha256));
    let claim = ArmClaim {
        schema_version: ARM_TOKEN_SCHEMA_VERSION,
        claim_identity: ARM_CLAIM_IDENTITY.to_owned(),
        consumed_at_utc: now,
        grant_sha256: file_sha256(grant_path)?,
        token_sha256: grant.token_sha256,
        prerequisite_sha256: file_sha256(prerequisite_path)?,
        case_id: grant.case_id,
        scenario: grant.scenario,
        process_id: prerequisite.process_id,
        process_creation_time_100ns: prerequisite.process_creation_time_100ns,
        generation: prerequisite.generation,
    };
    write_new_json(&claim_path, &claim).map_err(|error| {
        AppError::evidence(format!(
            "arm token is already consumed or cannot be claimed atomically: {error}"
        ))
    })?;
    Ok((claim, claim_path))
}

fn validate_grant(
    grant: &ArmGrant,
    prerequisite: &OfflinePrerequisiteDecision,
    arm_token: &str,
    prerequisite_path: &Path,
    now: DateTime<Utc>,
) -> AppResult<()> {
    if grant.schema_version != ARM_TOKEN_SCHEMA_VERSION
        || grant.grant_identity != ARM_GRANT_IDENTITY
        || grant.issued_at_utc < prerequisite.issued_at_utc
        || grant.expires_at_utc > prerequisite.expires_at_utc
        || now < grant.issued_at_utc
        || now >= grant.expires_at_utc
        || grant.token_sha256 != arm_token_sha256(arm_token)?
        || grant.prerequisite_sha256 != file_sha256(prerequisite_path)?
        || grant.target_sha256 != prerequisite.target_sha256
        || grant.observed_target_sha256 != prerequisite.observed_target_sha256
        || grant.target_equivalence_sha256 != prerequisite.target_equivalence_sha256
        || grant.mode_gate_sha256 != prerequisite.mode_gate_sha256
        || grant.loop_gate_sha256 != prerequisite.loop_gate_sha256
        || grant.raw_prereq_sha256 != prerequisite.raw_prereq_sha256
        || grant.active_plan_sha256 != prerequisite.active_plan_sha256
        || grant.case_id != prerequisite.case_id
        || grant.scenario != prerequisite.scenario
        || grant.process_id != prerequisite.process_id
        || grant.process_creation_time_100ns != prerequisite.process_creation_time_100ns
        || grant.generation != prerequisite.generation
        || grant.game_root_identity_sha256 != prerequisite.game_root_identity_sha256
        || grant.lua_generation_identity_sha256 != prerequisite.lua_generation_identity_sha256
    {
        return Err(AppError::evidence(
            "arm grant is expired, mutated, or bound to a different prerequisite/session",
        ));
    }
    Ok(())
}

fn validate_case(case_id: &str) -> AppResult<()> {
    if case_id.is_empty()
        || case_id.len() > 64
        || !case_id
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_'))
    {
        return Err(AppError::usage("invalid active case id"));
    }
    Ok(())
}

fn validate_scenario(scenario: &str) -> AppResult<()> {
    if !crate::active_catalog::ACTIVE_SCENARIOS.contains(&scenario) {
        return Err(AppError::usage("unknown active scenario"));
    }
    Ok(())
}

fn write_new_json<T: Serialize>(path: &Path, value: &T) -> AppResult<()> {
    let bytes = serde_json::to_vec_pretty(value).usage_context("cannot encode token JSON")?;
    let mut file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .usage_context(format!("refusing to overwrite {}", path.display()))?;
    file.write_all(&bytes)
        .usage_context(format!("cannot write {}", path.display()))?;
    file.write_all(b"\n")
        .usage_context(format!("cannot finish {}", path.display()))
}

#[cfg(test)]
mod tests {
    use std::sync::{Arc, Barrier};

    use chrono::Duration;

    use super::*;
    use crate::offline_attestation::{
        OFFLINE_ATTESTATION_IDENTITY, OFFLINE_ATTESTATION_SCHEMA_VERSION,
        OFFLINE_PREREQUISITE_IDENTITY, OfflineAttestationAnalysis, OfflineAttestationManifest,
    };

    const TOKEN: &str = "11AA22BB33CC44DD55EE66FF7788990011AA22BB33CC44DD55EE66FF77889900";

    fn write_prerequisite(path: &Path, now: DateTime<Utc>) -> OfflinePrerequisiteDecision {
        let parent = path.parent().unwrap();
        fs::write(parent.join("events.jsonl"), b"events\n").unwrap();
        let manifest = OfflineAttestationManifest {
            schema_version: OFFLINE_ATTESTATION_SCHEMA_VERSION,
            attestation_identity: OFFLINE_ATTESTATION_IDENTITY.to_owned(),
            state: "PASS".to_owned(),
            case_id: "case-01".to_owned(),
            scenario: "keyboard".to_owned(),
            target_sha256: crate::active_catalog::FIXED_TARGET_SHA256.to_owned(),
            observed_target_sha256: crate::target_equivalence::APPROVED_UNPACKED_TARGET_SHA256
                .to_owned(),
            target_equivalence_sha256: "8".repeat(64),
            mode_gate_sha256: "B".repeat(64),
            loop_gate_sha256: "C".repeat(64),
            raw_prereq_sha256: "D".repeat(64),
            active_plan_sha256: "E".repeat(64),
            process_id: 7,
            process_creation_time_100ns: 8,
            native_sha256: "F".repeat(64),
            helper_sha256: "1".repeat(64),
            observation_policy: "ACTIVE_RUN".to_owned(),
            clock_kind: crate::offline_attestation::OFFLINE_CLOCK_SIMULATION.to_owned(),
            generation_identity_kind: crate::offline_attestation::OFFLINE_GENERATION_LUA.to_owned(),
            expected_window_thread_id: 0,
            main_thread_id: 9,
            generation: 10,
            simulation_callbacks: 60,
            menu_heartbeats: 0,
            heartbeat_return_rva: None,
            game_root_identity_sha256: "1".repeat(64),
            lua_generation_identity_sha256: "2".repeat(64),
            identity_salt: "9".repeat(64),
            steam_observer_armed: true,
            teardown_observed: false,
            manager_signal_count: 0,
            roster_signal_count: 0,
            channel_zero_count: 0,
            nonzero_channel_count: 0,
            installed_hooks_mask: "0x000000000000003F".to_owned(),
            restored_hooks_mask: "0x000000000000003F".to_owned(),
            restored_slot_count: 6,
            overwritten_events: 0,
            producer_closed: true,
            target_identity_stable: true,
            slot_snapshot_before_sha256: "3".repeat(64),
            slot_snapshot_after_sha256: "3".repeat(64),
            failure_reason: None,
        };
        fs::write(
            parent.join("manifest.json"),
            serde_json::to_vec_pretty(&manifest).unwrap(),
        )
        .unwrap();
        let analysis = OfflineAttestationAnalysis {
            schema_version: OFFLINE_ATTESTATION_SCHEMA_VERSION,
            attestation_identity: OFFLINE_ATTESTATION_IDENTITY.to_owned(),
            result: "PASS".to_owned(),
            event_count: 60,
            clock_kind: crate::offline_attestation::OFFLINE_CLOCK_SIMULATION.to_owned(),
            simulation_callbacks: 60,
            menu_heartbeats: 0,
            checks: Vec::new(),
            manifest_sha256: file_sha256(&parent.join("manifest.json")).unwrap(),
            events_sha256: file_sha256(&parent.join("events.jsonl")).unwrap(),
        };
        fs::write(
            parent.join("analysis.json"),
            serde_json::to_vec_pretty(&analysis).unwrap(),
        )
        .unwrap();
        let decision = OfflinePrerequisiteDecision {
            schema_version: OFFLINE_ATTESTATION_SCHEMA_VERSION,
            decision_identity: OFFLINE_PREREQUISITE_IDENTITY.to_owned(),
            status: "PASS".to_owned(),
            issued_at_utc: now,
            expires_at_utc: now + Duration::minutes(5),
            target_sha256: crate::active_catalog::FIXED_TARGET_SHA256.to_owned(),
            observed_target_sha256: crate::target_equivalence::APPROVED_UNPACKED_TARGET_SHA256
                .to_owned(),
            target_equivalence_sha256: "8".repeat(64),
            mode_gate_sha256: "B".repeat(64),
            loop_gate_sha256: "C".repeat(64),
            raw_prereq_sha256: "D".repeat(64),
            active_plan_sha256: "E".repeat(64),
            case_id: "case-01".to_owned(),
            scenario: "keyboard".to_owned(),
            process_id: 7,
            process_creation_time_100ns: 8,
            observation_policy: "ACTIVE_RUN".to_owned(),
            clock_kind: crate::offline_attestation::OFFLINE_CLOCK_SIMULATION.to_owned(),
            generation_identity_kind: crate::offline_attestation::OFFLINE_GENERATION_LUA.to_owned(),
            expected_window_thread_id: 0,
            simulation_callbacks: 60,
            menu_heartbeats: 0,
            heartbeat_return_rva: None,
            main_thread_id: 9,
            generation: 10,
            game_root_identity_sha256: "1".repeat(64),
            lua_generation_identity_sha256: "2".repeat(64),
            identity_salt: "9".repeat(64),
            manifest_sha256: file_sha256(&parent.join("manifest.json")).unwrap(),
            events_sha256: file_sha256(&parent.join("events.jsonl")).unwrap(),
            analysis_sha256: file_sha256(&parent.join("analysis.json")).unwrap(),
            eligible_for_active_gate: false,
            active_input_allowed: false,
            ready_for_arm_token_cas: false,
            arm_token_consumed: false,
        };
        write_new_json(path, &decision).unwrap();
        decision
    }

    #[test]
    fn grant_and_claim_bind_the_exact_prerequisite() {
        let root = tempfile::tempdir().unwrap();
        let now = Utc::now();
        let prerequisite = root.path().join("prerequisite.json");
        write_prerequisite(&prerequisite, now);
        let grant_path = root.path().join("grant.json");
        create_arm_grant(
            &prerequisite,
            "case-01",
            "keyboard",
            TOKEN,
            &grant_path,
            now + Duration::seconds(1),
        )
        .unwrap();
        assert!(
            create_arm_grant(
                &prerequisite,
                "case-01",
                "keyboard",
                TOKEN,
                &grant_path,
                now + Duration::seconds(1),
            )
            .is_err()
        );
        let (_, claim_path) = consume_arm_token(
            &grant_path,
            &prerequisite,
            TOKEN,
            "case-01",
            "keyboard",
            now + Duration::seconds(2),
        )
        .unwrap();
        assert!(claim_path.is_file());
        assert!(
            consume_arm_token(
                &grant_path,
                &prerequisite,
                TOKEN,
                "case-01",
                "keyboard",
                now + Duration::seconds(3),
            )
            .is_err()
        );
    }

    #[test]
    fn concurrent_consumers_have_exactly_one_winner() {
        let root = tempfile::tempdir().unwrap();
        let now = Utc::now();
        let prerequisite = root.path().join("prerequisite.json");
        write_prerequisite(&prerequisite, now);
        let grant_path = root.path().join("grant.json");
        create_arm_grant(
            &prerequisite,
            "case-01",
            "keyboard",
            TOKEN,
            &grant_path,
            now + Duration::seconds(1),
        )
        .unwrap();
        let barrier = Arc::new(Barrier::new(3));
        let mut workers = Vec::new();
        for _ in 0..2 {
            let barrier = Arc::clone(&barrier);
            let prerequisite = prerequisite.clone();
            let grant_path = grant_path.clone();
            workers.push(std::thread::spawn(move || {
                barrier.wait();
                consume_arm_token(
                    &grant_path,
                    &prerequisite,
                    TOKEN,
                    "case-01",
                    "keyboard",
                    now + Duration::seconds(2),
                )
                .is_ok()
            }));
        }
        barrier.wait();
        assert_eq!(
            workers
                .into_iter()
                .map(|worker| worker.join().unwrap())
                .filter(|won| *won)
                .count(),
            1
        );
    }

    #[test]
    fn expired_wrong_case_token_and_mutated_session_fail_before_claim() {
        let root = tempfile::tempdir().unwrap();
        let now = Utc::now();
        let prerequisite = root.path().join("prerequisite.json");
        let decision = write_prerequisite(&prerequisite, now);
        let grant_path = root.path().join("grant.json");
        create_arm_grant(
            &prerequisite,
            "case-01",
            "keyboard",
            TOKEN,
            &grant_path,
            now + Duration::seconds(1),
        )
        .unwrap();
        for result in [
            validate_arm_grant(
                &grant_path,
                &prerequisite,
                &"22".repeat(32),
                "case-01",
                "keyboard",
                now + Duration::seconds(2),
            ),
            validate_arm_grant(
                &grant_path,
                &prerequisite,
                TOKEN,
                "case-01",
                "controller",
                now + Duration::seconds(2),
            ),
            validate_arm_grant(
                &grant_path,
                &prerequisite,
                TOKEN,
                "wrong-case",
                "keyboard",
                now + Duration::seconds(2),
            ),
            validate_arm_grant(
                &grant_path,
                &prerequisite,
                TOKEN,
                "case-01",
                "keyboard",
                decision.expires_at_utc,
            ),
        ] {
            assert!(result.is_err());
        }
        assert!(
            !root
                .path()
                .join(format!("claim-{}.json", arm_token_sha256(TOKEN).unwrap()))
                .exists()
        );
    }

    #[test]
    fn pid_creation_generation_and_opaque_identity_mutations_fail_before_claim() {
        type Mutation = Box<dyn Fn(&mut ArmGrant)>;
        let mutations: Vec<Mutation> = vec![
            Box::new(|grant| grant.process_id += 1),
            Box::new(|grant| grant.process_creation_time_100ns += 1),
            Box::new(|grant| grant.generation += 1),
            Box::new(|grant| grant.game_root_identity_sha256 = "6".repeat(64)),
            Box::new(|grant| grant.lua_generation_identity_sha256 = "7".repeat(64)),
            Box::new(|grant| grant.prerequisite_sha256 = "8".repeat(64)),
        ];
        for (index, mutate) in mutations.into_iter().enumerate() {
            let root = tempfile::tempdir().unwrap();
            let now = Utc::now();
            let prerequisite = root.path().join("prerequisite.json");
            write_prerequisite(&prerequisite, now);
            let grant_path = root.path().join(format!("grant-{index}.json"));
            let mut grant = create_arm_grant(
                &prerequisite,
                "case-01",
                "keyboard",
                TOKEN,
                &grant_path,
                now + Duration::seconds(1),
            )
            .unwrap();
            mutate(&mut grant);
            fs::write(&grant_path, serde_json::to_vec_pretty(&grant).unwrap()).unwrap();
            assert!(
                validate_arm_grant(
                    &grant_path,
                    &prerequisite,
                    TOKEN,
                    "case-01",
                    "keyboard",
                    now + Duration::seconds(2),
                )
                .is_err()
            );
            assert!(
                !root
                    .path()
                    .join(format!("claim-{}.json", arm_token_sha256(TOKEN).unwrap()))
                    .exists()
            );
        }
    }
}
