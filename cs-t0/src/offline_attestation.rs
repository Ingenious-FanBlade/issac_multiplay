use std::collections::BTreeSet;
use std::fs::{self, OpenOptions};
use std::io::Write;
use std::path::{Path, PathBuf};

use chrono::{DateTime, Duration, Utc};
use serde::{Deserialize, Serialize};

use crate::artifact::file_sha256;
use crate::error::{AppError, AppResult, UsageContext};
use crate::offline_attestation_ipc::{
    NativeOfflineAttestationRecord, OFFLINE_ATTESTATION_ACTIVE_RUN_HOOK_MASK,
    OFFLINE_ATTESTATION_ACTIVE_RUN_SLOT_COUNT, OFFLINE_ATTESTATION_LOADING_MENU_HOOK_MASK,
    OFFLINE_ATTESTATION_LOADING_MENU_SLOT_COUNT, OFFLINE_ATTESTATION_MENU_RETURN_RVA,
    OFFLINE_FLAG_EXACT_RETURN, OFFLINE_FLAG_GAME_ROOT_STABLE, OFFLINE_FLAG_LOADING_MENU_INACTIVE,
    OFFLINE_FLAG_LUA_UNBOUND_MENU, OFFLINE_FLAG_MAIN_THREAD, OFFLINE_FLAG_MANAGER_NEGATIVE,
    OFFLINE_FLAG_ORIGINAL_CALLED, OFFLINE_FLAG_ROOT_MATCH, OFFLINE_FLAG_RUN_INACTIVE_AFTER,
    OFFLINE_FLAG_RUN_INACTIVE_BEFORE, OFFLINE_FLAG_STEAM_ARMED, OFFLINE_FLAG_WINDOW_MAIN_THREAD,
    OFFLINE_KNOWN_FLAGS, OfflineAttestationEventKind,
};

pub const OFFLINE_ATTESTATION_IDENTITY: &str = "IsaacCsT0-ActiveOfflineAttestation-v2";
pub const OFFLINE_PREREQUISITE_IDENTITY: &str = "IsaacCsT0-ActiveOfflinePrerequisite-v2";
pub const OFFLINE_ATTESTATION_SCHEMA_VERSION: u32 = 2;
pub const OFFLINE_ATTESTATION_MIN_CALLBACKS: u64 = 60;
pub const OFFLINE_ATTESTATION_VALIDITY_SECONDS: i64 = 300;
pub const OFFLINE_CLOCK_SIMULATION: &str = "SIMULATION_CALLBACK_EXACT";
pub const OFFLINE_CLOCK_MENU: &str = "MENU_HEARTBEAT_PEEK";
pub const OFFLINE_GENERATION_LUA: &str = "LUA_GENERATION";
pub const OFFLINE_GENERATION_MENU: &str = "LUA_UNBOUND_MENU_EPOCH";

#[derive(Clone, Debug)]
pub struct OfflineAttestationRunConfig {
    pub case_id: String,
    pub scenario: String,
    pub target_sha256: String,
    pub observed_target_sha256: String,
    pub target_equivalence_sha256: String,
    pub mode_gate_sha256: String,
    pub loop_gate_sha256: String,
    pub raw_prereq_sha256: String,
    pub active_plan_sha256: String,
    pub process_id: u32,
    pub process_creation_time_100ns: u64,
    pub native_sha256: String,
    pub helper_sha256: String,
    pub identity_salt: [u8; 32],
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct OfflineAttestationManifest {
    pub schema_version: u32,
    pub attestation_identity: String,
    pub state: String,
    pub case_id: String,
    pub scenario: String,
    pub target_sha256: String,
    pub observed_target_sha256: String,
    pub target_equivalence_sha256: String,
    pub mode_gate_sha256: String,
    pub loop_gate_sha256: String,
    pub raw_prereq_sha256: String,
    pub active_plan_sha256: String,
    pub process_id: u32,
    pub process_creation_time_100ns: u64,
    pub native_sha256: String,
    pub helper_sha256: String,
    pub observation_policy: String,
    pub clock_kind: String,
    pub generation_identity_kind: String,
    pub expected_window_thread_id: u32,
    pub main_thread_id: u32,
    pub generation: u64,
    pub simulation_callbacks: u64,
    pub menu_heartbeats: u64,
    pub heartbeat_return_rva: Option<u32>,
    pub game_root_identity_sha256: String,
    pub lua_generation_identity_sha256: String,
    pub identity_salt: String,
    pub steam_observer_armed: bool,
    pub teardown_observed: bool,
    pub manager_signal_count: u64,
    pub roster_signal_count: u64,
    pub channel_zero_count: u64,
    pub nonzero_channel_count: u64,
    pub installed_hooks_mask: String,
    pub restored_hooks_mask: String,
    pub restored_slot_count: u32,
    pub overwritten_events: u64,
    pub producer_closed: bool,
    pub target_identity_stable: bool,
    pub slot_snapshot_before_sha256: String,
    pub slot_snapshot_after_sha256: String,
    pub failure_reason: Option<String>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct OfflineAttestationEvent {
    pub event_sequence: u64,
    pub qpc: i64,
    pub callback_sequence: u64,
    pub generation: u64,
    pub thread_id: u32,
    pub kind: String,
    pub flags: String,
    pub original_calls: u32,
    pub channel: Option<i32>,
    pub return_rva: Option<u32>,
    pub result_bits: Option<u32>,
    pub error_code: u32,
    pub game_root_identity_sha256: Option<String>,
    pub lua_generation_identity_sha256: Option<String>,
}

impl OfflineAttestationEvent {
    pub fn from_native(record: &NativeOfflineAttestationRecord) -> AppResult<Self> {
        let kind = OfflineAttestationEventKind::from_raw(record.event.kind)?;
        let channel = matches!(
            kind,
            OfflineAttestationEventKind::SteamSend
                | OfflineAttestationEventKind::SteamAvailable
                | OfflineAttestationEventKind::SteamRead
        )
        .then_some(record.event.channel);
        let return_rva = (record.event.return_rva != 0).then_some(record.event.return_rva);
        let result_bits = matches!(
            kind,
            OfflineAttestationEventKind::MenuHeartbeatPeek
                | OfflineAttestationEventKind::GenericLuaPcallDiagnostic
        )
        .then_some(record.event.result_bits);
        let digest = |value: &[u8; 32]| {
            value
                .iter()
                .any(|byte| *byte != 0)
                .then(|| hex::encode_upper(value))
        };
        Ok(Self {
            event_sequence: record.sequence,
            qpc: record.event.qpc,
            callback_sequence: record.event.callback_sequence,
            generation: record.event.generation,
            thread_id: record.event.thread_id,
            kind: format!("{kind:?}").to_ascii_uppercase(),
            flags: format!("0x{:08X}", record.event.flags),
            original_calls: record.event.original_calls,
            channel,
            return_rva,
            result_bits,
            error_code: record.event.error_code,
            game_root_identity_sha256: digest(&record.event.game_root_digest),
            lua_generation_identity_sha256: digest(&record.event.lua_generation_digest),
        })
    }

    fn flags_value(&self) -> Option<u32> {
        self.flags
            .strip_prefix("0x")
            .and_then(|value| u32::from_str_radix(value, 16).ok())
    }
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct OfflineAttestationCheck {
    pub id: String,
    pub passed: bool,
    pub detail: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct OfflineAttestationAnalysis {
    pub schema_version: u32,
    pub attestation_identity: String,
    pub result: String,
    pub event_count: usize,
    pub clock_kind: String,
    pub simulation_callbacks: u64,
    pub menu_heartbeats: u64,
    pub checks: Vec<OfflineAttestationCheck>,
    pub manifest_sha256: String,
    pub events_sha256: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct OfflinePrerequisiteDecision {
    pub schema_version: u32,
    pub decision_identity: String,
    pub status: String,
    pub issued_at_utc: DateTime<Utc>,
    pub expires_at_utc: DateTime<Utc>,
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
    pub observation_policy: String,
    pub clock_kind: String,
    pub generation_identity_kind: String,
    pub expected_window_thread_id: u32,
    pub simulation_callbacks: u64,
    pub menu_heartbeats: u64,
    pub heartbeat_return_rva: Option<u32>,
    pub main_thread_id: u32,
    pub generation: u64,
    pub game_root_identity_sha256: String,
    pub lua_generation_identity_sha256: String,
    pub identity_salt: String,
    pub manifest_sha256: String,
    pub events_sha256: String,
    pub analysis_sha256: String,
    pub eligible_for_active_gate: bool,
    pub active_input_allowed: bool,
    pub ready_for_arm_token_cas: bool,
    pub arm_token_consumed: bool,
}

#[derive(Clone, Debug)]
pub struct OfflineAttestationFinish {
    pub main_thread_id: u32,
    pub generation: u64,
    pub simulation_callbacks: u64,
    pub menu_heartbeats: u64,
    pub steam_observer_armed: bool,
    pub teardown_observed: bool,
    pub signal_counts: [u64; 4],
    pub installed_hooks_mask: u64,
    pub restored_hooks_mask: u64,
    pub restored_slot_count: u32,
    pub overwritten_events: u64,
    pub producer_closed: bool,
    pub target_identity_stable: bool,
    pub slot_snapshot_before_sha256: [u8; 32],
    pub slot_snapshot_after_sha256: [u8; 32],
}

pub fn write_offline_attestation_run(
    directory: &Path,
    config: OfflineAttestationRunConfig,
    records: &[NativeOfflineAttestationRecord],
    finish: OfflineAttestationFinish,
) -> AppResult<OfflinePrerequisiteDecision> {
    fs::create_dir(directory).usage_context(format!(
        "refusing to overwrite offline attestation {}",
        directory.display()
    ))?;
    let events = records
        .iter()
        .map(OfflineAttestationEvent::from_native)
        .collect::<AppResult<Vec<_>>>()?;
    let loading_menu = config.scenario == "loading";
    let clock_kind = if loading_menu {
        OFFLINE_CLOCK_MENU
    } else {
        OFFLINE_CLOCK_SIMULATION
    };
    let generation_identity_kind = if loading_menu {
        OFFLINE_GENERATION_MENU
    } else {
        OFFLINE_GENERATION_LUA
    };
    let identities = clock_identities(&events, clock_kind);
    let manifest = OfflineAttestationManifest {
        schema_version: OFFLINE_ATTESTATION_SCHEMA_VERSION,
        attestation_identity: OFFLINE_ATTESTATION_IDENTITY.to_owned(),
        state: "PASS".to_owned(),
        case_id: config.case_id,
        scenario: config.scenario,
        target_sha256: config.target_sha256,
        observed_target_sha256: config.observed_target_sha256,
        target_equivalence_sha256: config.target_equivalence_sha256,
        mode_gate_sha256: config.mode_gate_sha256,
        loop_gate_sha256: config.loop_gate_sha256,
        raw_prereq_sha256: config.raw_prereq_sha256,
        active_plan_sha256: config.active_plan_sha256,
        process_id: config.process_id,
        process_creation_time_100ns: config.process_creation_time_100ns,
        native_sha256: config.native_sha256,
        helper_sha256: config.helper_sha256,
        observation_policy: if loading_menu {
            "LOADING_MENU_INACTIVE"
        } else {
            "ACTIVE_RUN"
        }
        .to_owned(),
        clock_kind: clock_kind.to_owned(),
        generation_identity_kind: generation_identity_kind.to_owned(),
        expected_window_thread_id: if loading_menu {
            finish.main_thread_id
        } else {
            0
        },
        main_thread_id: finish.main_thread_id,
        generation: finish.generation,
        simulation_callbacks: finish.simulation_callbacks,
        menu_heartbeats: finish.menu_heartbeats,
        heartbeat_return_rva: loading_menu.then_some(OFFLINE_ATTESTATION_MENU_RETURN_RVA),
        game_root_identity_sha256: identities.0,
        lua_generation_identity_sha256: identities.1,
        identity_salt: hex::encode_upper(config.identity_salt),
        steam_observer_armed: finish.steam_observer_armed,
        teardown_observed: finish.teardown_observed,
        manager_signal_count: finish.signal_counts[0],
        roster_signal_count: finish.signal_counts[1],
        channel_zero_count: finish.signal_counts[2],
        nonzero_channel_count: finish.signal_counts[3],
        installed_hooks_mask: format!("0x{:016X}", finish.installed_hooks_mask),
        restored_hooks_mask: format!("0x{:016X}", finish.restored_hooks_mask),
        restored_slot_count: finish.restored_slot_count,
        overwritten_events: finish.overwritten_events,
        producer_closed: finish.producer_closed,
        target_identity_stable: finish.target_identity_stable,
        slot_snapshot_before_sha256: hex::encode_upper(finish.slot_snapshot_before_sha256),
        slot_snapshot_after_sha256: hex::encode_upper(finish.slot_snapshot_after_sha256),
        failure_reason: None,
    };
    let manifest_path = directory.join("manifest.json");
    let events_path = directory.join("events.jsonl");
    write_new_json(&manifest_path, &manifest)?;
    write_new_jsonl(&events_path, &events)?;
    let analysis = analyze_offline_attestation(directory)?;
    if analysis.result != "PASS" {
        return Err(AppError::evidence(
            "offline attestation analysis failed; prerequisite was not created",
        ));
    }
    let issued = Utc::now();
    let decision = OfflinePrerequisiteDecision {
        schema_version: OFFLINE_ATTESTATION_SCHEMA_VERSION,
        decision_identity: OFFLINE_PREREQUISITE_IDENTITY.to_owned(),
        status: "PASS".to_owned(),
        issued_at_utc: issued,
        expires_at_utc: issued + Duration::seconds(OFFLINE_ATTESTATION_VALIDITY_SECONDS),
        target_sha256: manifest.target_sha256.clone(),
        observed_target_sha256: manifest.observed_target_sha256.clone(),
        target_equivalence_sha256: manifest.target_equivalence_sha256.clone(),
        mode_gate_sha256: manifest.mode_gate_sha256.clone(),
        loop_gate_sha256: manifest.loop_gate_sha256.clone(),
        raw_prereq_sha256: manifest.raw_prereq_sha256.clone(),
        active_plan_sha256: manifest.active_plan_sha256.clone(),
        case_id: manifest.case_id.clone(),
        scenario: manifest.scenario.clone(),
        process_id: manifest.process_id,
        process_creation_time_100ns: manifest.process_creation_time_100ns,
        observation_policy: manifest.observation_policy.clone(),
        clock_kind: manifest.clock_kind.clone(),
        generation_identity_kind: manifest.generation_identity_kind.clone(),
        expected_window_thread_id: manifest.expected_window_thread_id,
        simulation_callbacks: manifest.simulation_callbacks,
        menu_heartbeats: manifest.menu_heartbeats,
        heartbeat_return_rva: manifest.heartbeat_return_rva,
        main_thread_id: manifest.main_thread_id,
        generation: manifest.generation,
        game_root_identity_sha256: manifest.game_root_identity_sha256.clone(),
        lua_generation_identity_sha256: manifest.lua_generation_identity_sha256.clone(),
        identity_salt: manifest.identity_salt.clone(),
        manifest_sha256: analysis.manifest_sha256.clone(),
        events_sha256: analysis.events_sha256.clone(),
        analysis_sha256: file_sha256(&directory.join("analysis.json"))?,
        eligible_for_active_gate: false,
        active_input_allowed: false,
        ready_for_arm_token_cas: false,
        arm_token_consumed: false,
    };
    write_new_json(&directory.join("prerequisite.json"), &decision)?;
    Ok(decision)
}

pub fn analyze_offline_attestation(directory: &Path) -> AppResult<OfflineAttestationAnalysis> {
    let manifest_path = directory.join("manifest.json");
    let events_path = directory.join("events.jsonl");
    let manifest: OfflineAttestationManifest = serde_json::from_slice(
        &fs::read(&manifest_path)
            .usage_context(format!("cannot read {}", manifest_path.display()))?,
    )
    .usage_context("cannot parse offline attestation manifest")?;
    let events = read_events(&events_path)?;
    let loading_menu = manifest.scenario == "loading";
    let expected_clock = if loading_menu {
        OFFLINE_CLOCK_MENU
    } else {
        OFFLINE_CLOCK_SIMULATION
    };
    let expected_event_kind = if loading_menu {
        "MENUHEARTBEATPEEK"
    } else {
        "SIMULATIONCALLBACK"
    };
    let clock_events = events
        .iter()
        .filter(|event| event.kind == expected_event_kind)
        .collect::<Vec<_>>();
    let simulation_count = events
        .iter()
        .filter(|event| event.kind == "SIMULATIONCALLBACK")
        .count() as u64;
    let menu_count = events
        .iter()
        .filter(|event| event.kind == "MENUHEARTBEATPEEK")
        .count() as u64;
    let contiguous = events
        .iter()
        .enumerate()
        .all(|(index, event)| event.event_sequence == index as u64);
    let callback_contiguous = clock_events
        .iter()
        .enumerate()
        .all(|(index, event)| event.callback_sequence == index as u64 + 1);
    let qpc_strict = clock_events
        .windows(2)
        .all(|pair| pair[0].qpc < pair[1].qpc);
    let threads = clock_events
        .iter()
        .map(|event| event.thread_id)
        .collect::<BTreeSet<_>>();
    let generations = clock_events
        .iter()
        .map(|event| event.generation)
        .collect::<BTreeSet<_>>();
    let roots = clock_events
        .iter()
        .filter_map(|event| event.game_root_identity_sha256.as_ref())
        .collect::<BTreeSet<_>>();
    let lua = clock_events
        .iter()
        .filter_map(|event| event.lua_generation_identity_sha256.as_ref())
        .collect::<BTreeSet<_>>();
    let active_required = OFFLINE_FLAG_MAIN_THREAD
        | OFFLINE_FLAG_ORIGINAL_CALLED
        | OFFLINE_FLAG_ROOT_MATCH
        | OFFLINE_FLAG_STEAM_ARMED;
    let menu_required = active_required
        | OFFLINE_FLAG_LOADING_MENU_INACTIVE
        | OFFLINE_FLAG_EXACT_RETURN
        | OFFLINE_FLAG_WINDOW_MAIN_THREAD
        | OFFLINE_FLAG_RUN_INACTIVE_BEFORE
        | OFFLINE_FLAG_RUN_INACTIVE_AFTER
        | OFFLINE_FLAG_GAME_ROOT_STABLE
        | OFFLINE_FLAG_MANAGER_NEGATIVE
        | OFFLINE_FLAG_LUA_UNBOUND_MENU;
    let callback_shape = clock_events.iter().all(|event| {
        let required = if loading_menu {
            menu_required
        } else {
            active_required
        };
        event.original_calls == 1
            && event.error_code == 0
            && event.flags_value().is_some_and(|flags| {
                flags & required == required
                    && (flags & OFFLINE_FLAG_LOADING_MENU_INACTIVE != 0) == loading_menu
                    && (flags & OFFLINE_FLAG_LUA_UNBOUND_MENU != 0) == loading_menu
            })
            && if loading_menu {
                event.return_rva == Some(OFFLINE_ATTESTATION_MENU_RETURN_RVA)
                    && event.result_bits.is_some()
            } else {
                event.return_rva.is_none() && event.result_bits.is_none()
            }
    });
    let no_forbidden_events = events.iter().all(|event| {
        !matches!(
            event.kind.as_str(),
            "LUACLOSE" | "STEAMSEND" | "STEAMAVAILABLE" | "STEAMREAD" | "FAILURE"
        ) && if loading_menu {
            event.kind != "SIMULATIONCALLBACK"
        } else {
            !matches!(
                event.kind.as_str(),
                "MENUHEARTBEATPEEK" | "GENERICLUAPCALLDIAGNOSTIC"
            )
        }
    });
    let protocol_shape = events.iter().all(|event| {
        event.error_code == 0
            && event
                .flags_value()
                .is_some_and(|flags| flags & !OFFLINE_KNOWN_FLAGS == 0)
            && (event.kind != "GENERICLUAPCALLDIAGNOSTIC"
                || (loading_menu
                    && event.original_calls == 1
                    && event.flags_value().is_some_and(|flags| {
                        flags
                            & (OFFLINE_FLAG_ORIGINAL_CALLED
                                | OFFLINE_FLAG_STEAM_ARMED
                                | OFFLINE_FLAG_LUA_UNBOUND_MENU)
                            == (OFFLINE_FLAG_ORIGINAL_CALLED
                                | OFFLINE_FLAG_STEAM_ARMED
                                | OFFLINE_FLAG_LUA_UNBOUND_MENU)
                    })
                    && event.callback_sequence == 0
                    && event.game_root_identity_sha256.is_none()
                    && event.lua_generation_identity_sha256.is_none()))
    });
    let (expected_mask, expected_slots) = if loading_menu {
        (
            OFFLINE_ATTESTATION_LOADING_MENU_HOOK_MASK,
            OFFLINE_ATTESTATION_LOADING_MENU_SLOT_COUNT,
        )
    } else {
        (
            OFFLINE_ATTESTATION_ACTIVE_RUN_HOOK_MASK,
            OFFLINE_ATTESTATION_ACTIVE_RUN_SLOT_COUNT,
        )
    };
    let masks = parse_mask(&manifest.installed_hooks_mask) == Some(expected_mask)
        && parse_mask(&manifest.restored_hooks_mask) == Some(expected_mask)
        && manifest.restored_slot_count == expected_slots;
    let mode_identity = if loading_menu {
        manifest.observation_policy == "LOADING_MENU_INACTIVE"
            && manifest.clock_kind == OFFLINE_CLOCK_MENU
            && manifest.generation_identity_kind == OFFLINE_GENERATION_MENU
            && manifest.expected_window_thread_id == manifest.main_thread_id
            && manifest.heartbeat_return_rva == Some(OFFLINE_ATTESTATION_MENU_RETURN_RVA)
            && manifest.simulation_callbacks == 0
            && manifest.menu_heartbeats == menu_count
    } else {
        manifest.observation_policy == "ACTIVE_RUN"
            && manifest.clock_kind == OFFLINE_CLOCK_SIMULATION
            && manifest.generation_identity_kind == OFFLINE_GENERATION_LUA
            && manifest.expected_window_thread_id == 0
            && manifest.heartbeat_return_rva.is_none()
            && manifest.simulation_callbacks == simulation_count
            && manifest.menu_heartbeats == 0
    };
    let checks = vec![
        check(
            "manifest-identity",
            manifest.schema_version == OFFLINE_ATTESTATION_SCHEMA_VERSION
                && manifest.attestation_identity == OFFLINE_ATTESTATION_IDENTITY
                && manifest.state == "PASS"
                && manifest.failure_reason.is_none(),
            "exact offline attestation v2 manifest identity",
        ),
        check(
            "observation-mode",
            mode_identity,
            "scenario selects one explicit clock and generation identity mode",
        ),
        check("event-sequence", contiguous, "event sequence is contiguous"),
        check(
            "callback-count",
            clock_events.len() as u64 >= OFFLINE_ATTESTATION_MIN_CALLBACKS,
            "at least 60 exact policy clock events are present",
        ),
        check(
            "callback-sequence",
            callback_contiguous,
            "policy clock sequence is contiguous",
        ),
        check(
            "clock-qpc",
            qpc_strict,
            "policy clock QPC values increase strictly",
        ),
        check(
            "main-thread",
            threads.len() == 1 && threads.contains(&manifest.main_thread_id),
            "clock events remain on the pre-bound nonzero main thread",
        ),
        check(
            "generation-stable",
            manifest.generation != 0
                && generations.len() == 1
                && generations.contains(&manifest.generation),
            "one nonzero observer generation spans the window",
        ),
        check(
            "opaque-identities",
            roots.len() == 1
                && lua.len() == 1
                && roots.contains(&manifest.game_root_identity_sha256)
                && lua.contains(&manifest.lua_generation_identity_sha256)
                && manifest.identity_salt.len() == 64
                && manifest
                    .identity_salt
                    .bytes()
                    .all(|byte| byte.is_ascii_hexdigit())
                && manifest.identity_salt.bytes().any(|byte| byte != b'0'),
            "game root and policy generation identities are stable opaque digests",
        ),
        check(
            "steam-observer-armed",
            manifest.steam_observer_armed && callback_shape,
            "Steam observer is armed and every clock event is exact original-once",
        ),
        check(
            "online-signals-zero",
            manifest.manager_signal_count == 0
                && manifest.roster_signal_count == 0
                && manifest.channel_zero_count == 0
                && manifest.nonzero_channel_count == 0,
            "manager, roster, channel-zero and nonzero-channel signals are all zero",
        ),
        check(
            "no-lifecycle-or-network-events",
            no_forbidden_events && protocol_shape && !manifest.teardown_observed,
            "no lifecycle, cross-policy clock, network or failure event occurs",
        ),
        check(
            "safe-restoration",
            masks
                && manifest.overwritten_events == 0
                && manifest.producer_closed
                && manifest.target_identity_stable
                && manifest.slot_snapshot_before_sha256.len() == 64
                && manifest.slot_snapshot_before_sha256 == manifest.slot_snapshot_after_sha256
                && manifest
                    .slot_snapshot_before_sha256
                    .bytes()
                    .any(|byte| byte != b'0'),
            "all policy slots restored, ring closed and target identity stable",
        ),
    ];
    let passed = checks.iter().all(|item| item.passed);
    let report = OfflineAttestationAnalysis {
        schema_version: OFFLINE_ATTESTATION_SCHEMA_VERSION,
        attestation_identity: OFFLINE_ATTESTATION_IDENTITY.to_owned(),
        result: if passed { "PASS" } else { "FAIL" }.to_owned(),
        event_count: events.len(),
        clock_kind: expected_clock.to_owned(),
        simulation_callbacks: simulation_count,
        menu_heartbeats: menu_count,
        checks,
        manifest_sha256: file_sha256(&manifest_path)?,
        events_sha256: file_sha256(&events_path)?,
    };
    write_new_json(&directory.join("analysis.json"), &report)?;
    Ok(report)
}

pub fn validate_offline_prerequisite(
    path: &Path,
    now: DateTime<Utc>,
) -> AppResult<OfflinePrerequisiteDecision> {
    let decision: OfflinePrerequisiteDecision = serde_json::from_slice(
        &fs::read(path).usage_context(format!("cannot read {}", path.display()))?,
    )
    .usage_context("cannot parse offline prerequisite decision")?;
    if decision.schema_version != OFFLINE_ATTESTATION_SCHEMA_VERSION
        || decision.decision_identity != OFFLINE_PREREQUISITE_IDENTITY
        || decision.status != "PASS"
        || decision.eligible_for_active_gate
        || decision.active_input_allowed
        || decision.ready_for_arm_token_cas
        || decision.arm_token_consumed
        || decision.process_id == 0
        || decision.process_creation_time_100ns == 0
        || !matches!(
            decision.observation_policy.as_str(),
            "ACTIVE_RUN" | "LOADING_MENU_INACTIVE"
        )
        || !matches!(
            decision.clock_kind.as_str(),
            OFFLINE_CLOCK_SIMULATION | OFFLINE_CLOCK_MENU
        )
        || !matches!(
            decision.generation_identity_kind.as_str(),
            OFFLINE_GENERATION_LUA | OFFLINE_GENERATION_MENU
        )
        || decision.main_thread_id == 0
        || decision.generation == 0
        || decision.expires_at_utc <= decision.issued_at_utc
        || now < decision.issued_at_utc
        || now >= decision.expires_at_utc
        || decision.expires_at_utc - decision.issued_at_utc
            > Duration::seconds(OFFLINE_ATTESTATION_VALIDITY_SECONDS)
        || decision.case_id.is_empty()
        || !decision
            .case_id
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_'))
        || !crate::active_catalog::ACTIVE_SCENARIOS.contains(&decision.scenario.as_str())
        || decision.target_sha256 != crate::active_catalog::FIXED_TARGET_SHA256
        || decision.observed_target_sha256
            != crate::target_equivalence::APPROVED_UNPACKED_TARGET_SHA256
        || if decision.scenario == "loading" {
            decision.observation_policy != "LOADING_MENU_INACTIVE"
                || decision.clock_kind != OFFLINE_CLOCK_MENU
                || decision.generation_identity_kind != OFFLINE_GENERATION_MENU
                || decision.expected_window_thread_id != decision.main_thread_id
                || decision.simulation_callbacks != 0
                || decision.menu_heartbeats < OFFLINE_ATTESTATION_MIN_CALLBACKS
                || decision.heartbeat_return_rva != Some(OFFLINE_ATTESTATION_MENU_RETURN_RVA)
        } else {
            decision.observation_policy != "ACTIVE_RUN"
                || decision.clock_kind != OFFLINE_CLOCK_SIMULATION
                || decision.generation_identity_kind != OFFLINE_GENERATION_LUA
                || decision.expected_window_thread_id != 0
                || decision.simulation_callbacks < OFFLINE_ATTESTATION_MIN_CALLBACKS
                || decision.menu_heartbeats != 0
                || decision.heartbeat_return_rva.is_some()
        }
    {
        return Err(AppError::evidence(
            "offline prerequisite is invalid, expired, or not the exact passive-only identity",
        ));
    }
    for value in [
        &decision.target_sha256,
        &decision.observed_target_sha256,
        &decision.target_equivalence_sha256,
        &decision.mode_gate_sha256,
        &decision.loop_gate_sha256,
        &decision.raw_prereq_sha256,
        &decision.active_plan_sha256,
        &decision.game_root_identity_sha256,
        &decision.lua_generation_identity_sha256,
        &decision.identity_salt,
        &decision.manifest_sha256,
        &decision.events_sha256,
        &decision.analysis_sha256,
    ] {
        if value.len() != 64
            || !value.bytes().all(|byte| byte.is_ascii_hexdigit())
            || value.bytes().all(|byte| byte == b'0')
        {
            return Err(AppError::evidence(
                "offline prerequisite contains an invalid SHA-256 identity",
            ));
        }
    }
    let parent = path.parent().unwrap_or_else(|| Path::new("."));
    for (name, expected) in [
        ("manifest.json", &decision.manifest_sha256),
        ("events.jsonl", &decision.events_sha256),
        ("analysis.json", &decision.analysis_sha256),
    ] {
        if file_sha256(&parent.join(name))? != *expected {
            return Err(AppError::evidence(format!(
                "offline prerequisite source {name} hash mismatch"
            )));
        }
    }
    let source_manifest: OfflineAttestationManifest = serde_json::from_slice(
        &fs::read(parent.join("manifest.json"))
            .usage_context("cannot read offline prerequisite source manifest")?,
    )
    .usage_context("cannot parse offline prerequisite source manifest")?;
    let source_analysis: OfflineAttestationAnalysis = serde_json::from_slice(
        &fs::read(parent.join("analysis.json"))
            .usage_context("cannot read offline prerequisite source analysis")?,
    )
    .usage_context("cannot parse offline prerequisite source analysis")?;
    if source_analysis.schema_version != OFFLINE_ATTESTATION_SCHEMA_VERSION
        || source_analysis.attestation_identity != OFFLINE_ATTESTATION_IDENTITY
        || source_analysis.result != "PASS"
        || source_analysis.clock_kind != decision.clock_kind
        || source_analysis.manifest_sha256 != decision.manifest_sha256
        || source_analysis.events_sha256 != decision.events_sha256
        || source_manifest.schema_version != OFFLINE_ATTESTATION_SCHEMA_VERSION
        || source_manifest.attestation_identity != OFFLINE_ATTESTATION_IDENTITY
        || source_manifest.state != "PASS"
        || source_manifest.failure_reason.is_some()
        || source_manifest.target_sha256 != decision.target_sha256
        || source_manifest.observed_target_sha256 != decision.observed_target_sha256
        || source_manifest.target_equivalence_sha256 != decision.target_equivalence_sha256
        || source_manifest.mode_gate_sha256 != decision.mode_gate_sha256
        || source_manifest.loop_gate_sha256 != decision.loop_gate_sha256
        || source_manifest.raw_prereq_sha256 != decision.raw_prereq_sha256
        || source_manifest.active_plan_sha256 != decision.active_plan_sha256
        || source_manifest.case_id != decision.case_id
        || source_manifest.scenario != decision.scenario
        || source_manifest.process_id != decision.process_id
        || source_manifest.process_creation_time_100ns != decision.process_creation_time_100ns
        || source_manifest.observation_policy != decision.observation_policy
        || source_manifest.clock_kind != decision.clock_kind
        || source_manifest.generation_identity_kind != decision.generation_identity_kind
        || source_manifest.expected_window_thread_id != decision.expected_window_thread_id
        || source_manifest.simulation_callbacks != decision.simulation_callbacks
        || source_manifest.menu_heartbeats != decision.menu_heartbeats
        || source_manifest.heartbeat_return_rva != decision.heartbeat_return_rva
        || source_manifest.main_thread_id != decision.main_thread_id
        || source_manifest.generation != decision.generation
        || source_manifest.game_root_identity_sha256 != decision.game_root_identity_sha256
        || source_manifest.lua_generation_identity_sha256 != decision.lua_generation_identity_sha256
        || source_manifest.identity_salt != decision.identity_salt
    {
        return Err(AppError::evidence(
            "offline prerequisite does not replay its exact PASS source identities",
        ));
    }
    Ok(decision)
}

fn clock_identities(events: &[OfflineAttestationEvent], clock_kind: &str) -> (String, String) {
    let event_kind = if clock_kind == OFFLINE_CLOCK_MENU {
        "MENUHEARTBEATPEEK"
    } else {
        "SIMULATIONCALLBACK"
    };
    events
        .iter()
        .find(|event| event.kind == event_kind)
        .map(|event| {
            (
                event.game_root_identity_sha256.clone().unwrap_or_default(),
                event
                    .lua_generation_identity_sha256
                    .clone()
                    .unwrap_or_default(),
            )
        })
        .unwrap_or_default()
}

fn read_events(path: &Path) -> AppResult<Vec<OfflineAttestationEvent>> {
    let text = fs::read_to_string(path)
        .usage_context(format!("cannot read offline events {}", path.display()))?;
    text.lines()
        .filter(|line| !line.trim().is_empty())
        .map(|line| {
            serde_json::from_str(line).usage_context("cannot parse offline attestation event")
        })
        .collect()
}

fn parse_mask(value: &str) -> Option<u64> {
    value
        .strip_prefix("0x")
        .and_then(|digits| u64::from_str_radix(digits, 16).ok())
}

fn check(id: &str, passed: bool, detail: &str) -> OfflineAttestationCheck {
    OfflineAttestationCheck {
        id: id.to_owned(),
        passed,
        detail: detail.to_owned(),
    }
}

fn write_new_json<T: Serialize>(path: &Path, value: &T) -> AppResult<()> {
    let bytes = serde_json::to_vec_pretty(value).usage_context("cannot encode JSON")?;
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

fn write_new_jsonl<T: Serialize>(path: &Path, values: &[T]) -> AppResult<()> {
    let mut file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .usage_context(format!("refusing to overwrite {}", path.display()))?;
    for value in values {
        serde_json::to_writer(&mut file, value).usage_context("cannot encode JSONL event")?;
        file.write_all(b"\n")
            .usage_context(format!("cannot write {}", path.display()))?;
    }
    Ok(())
}

pub fn prerequisite_path(run_directory: &Path) -> PathBuf {
    run_directory.join("prerequisite.json")
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::offline_attestation_ipc::{
        NativeOfflineAttestationEvent, OFFLINE_ATTESTATION_ACTIVE_RUN_HOOK_MASK,
        OFFLINE_ATTESTATION_ACTIVE_RUN_SLOT_COUNT, OFFLINE_ATTESTATION_LOADING_MENU_HOOK_MASK,
        OFFLINE_ATTESTATION_LOADING_MENU_SLOT_COUNT, OFFLINE_FLAG_EXACT_RETURN,
        OFFLINE_FLAG_GAME_ROOT_STABLE, OFFLINE_FLAG_LOADING_MENU_INACTIVE,
        OFFLINE_FLAG_LUA_UNBOUND_MENU, OFFLINE_FLAG_MAIN_THREAD, OFFLINE_FLAG_MANAGER_NEGATIVE,
        OFFLINE_FLAG_ORIGINAL_CALLED, OFFLINE_FLAG_ROOT_MATCH, OFFLINE_FLAG_RUN_INACTIVE_AFTER,
        OFFLINE_FLAG_RUN_INACTIVE_BEFORE, OFFLINE_FLAG_STEAM_ARMED,
        OFFLINE_FLAG_WINDOW_MAIN_THREAD,
    };

    fn record(sequence: u64, callbacks: u64) -> NativeOfflineAttestationRecord {
        NativeOfflineAttestationRecord {
            sequence,
            event: NativeOfflineAttestationEvent {
                published_sequence: sequence as i64 + 1,
                qpc: sequence as i64,
                callback_sequence: callbacks,
                generation: 7,
                thread_id: 11,
                kind: OfflineAttestationEventKind::SimulationCallback as u32,
                flags: OFFLINE_FLAG_MAIN_THREAD
                    | OFFLINE_FLAG_ORIGINAL_CALLED
                    | OFFLINE_FLAG_ROOT_MATCH
                    | OFFLINE_FLAG_STEAM_ARMED,
                error_code: 0,
                original_calls: 1,
                channel: -1,
                return_rva: 0,
                result_bits: 0,
                game_root_digest: [0xAA; 32],
                lua_generation_digest: [0xBB; 32],
            },
        }
    }

    fn menu_record(sequence: u64, callbacks: u64) -> NativeOfflineAttestationRecord {
        let mut value = record(sequence, callbacks);
        value.event.kind = OfflineAttestationEventKind::MenuHeartbeatPeek as u32;
        value.event.flags = OFFLINE_FLAG_MAIN_THREAD
            | OFFLINE_FLAG_ORIGINAL_CALLED
            | OFFLINE_FLAG_ROOT_MATCH
            | OFFLINE_FLAG_STEAM_ARMED
            | OFFLINE_FLAG_LOADING_MENU_INACTIVE
            | OFFLINE_FLAG_EXACT_RETURN
            | OFFLINE_FLAG_WINDOW_MAIN_THREAD
            | OFFLINE_FLAG_RUN_INACTIVE_BEFORE
            | OFFLINE_FLAG_RUN_INACTIVE_AFTER
            | OFFLINE_FLAG_GAME_ROOT_STABLE
            | OFFLINE_FLAG_MANAGER_NEGATIVE
            | OFFLINE_FLAG_LUA_UNBOUND_MENU;
        value.event.return_rva = OFFLINE_ATTESTATION_MENU_RETURN_RVA;
        value.event.result_bits = 0;
        value
    }

    fn fixture(directory: &Path, callbacks: u64) -> AppResult<OfflinePrerequisiteDecision> {
        let records = (0..callbacks)
            .map(|index| record(index, index + 1))
            .collect::<Vec<_>>();
        write_offline_attestation_run(
            directory,
            OfflineAttestationRunConfig {
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
                process_id: 1,
                process_creation_time_100ns: 2,
                native_sha256: "F".repeat(64),
                helper_sha256: "1".repeat(64),
                identity_salt: [0x99; 32],
            },
            &records,
            OfflineAttestationFinish {
                main_thread_id: 11,
                generation: 7,
                simulation_callbacks: callbacks,
                menu_heartbeats: 0,
                steam_observer_armed: true,
                teardown_observed: false,
                signal_counts: [0; 4],
                installed_hooks_mask: OFFLINE_ATTESTATION_ACTIVE_RUN_HOOK_MASK,
                restored_hooks_mask: OFFLINE_ATTESTATION_ACTIVE_RUN_HOOK_MASK,
                restored_slot_count: OFFLINE_ATTESTATION_ACTIVE_RUN_SLOT_COUNT,
                overwritten_events: 0,
                producer_closed: true,
                target_identity_stable: true,
                slot_snapshot_before_sha256: [0xCC; 32],
                slot_snapshot_after_sha256: [0xCC; 32],
            },
        )
    }

    fn loading_fixture(directory: &Path, callbacks: u64) -> AppResult<OfflinePrerequisiteDecision> {
        let records = (0..callbacks)
            .map(|index| menu_record(index, index + 1))
            .collect::<Vec<_>>();
        write_offline_attestation_run(
            directory,
            OfflineAttestationRunConfig {
                case_id: "loading-01".to_owned(),
                scenario: "loading".to_owned(),
                target_sha256: crate::active_catalog::FIXED_TARGET_SHA256.to_owned(),
                observed_target_sha256: crate::target_equivalence::APPROVED_UNPACKED_TARGET_SHA256
                    .to_owned(),
                target_equivalence_sha256: "8".repeat(64),
                mode_gate_sha256: "B".repeat(64),
                loop_gate_sha256: "C".repeat(64),
                raw_prereq_sha256: "D".repeat(64),
                active_plan_sha256: "E".repeat(64),
                process_id: 1,
                process_creation_time_100ns: 2,
                native_sha256: "F".repeat(64),
                helper_sha256: "1".repeat(64),
                identity_salt: [0x99; 32],
            },
            &records,
            OfflineAttestationFinish {
                main_thread_id: 11,
                generation: 7,
                simulation_callbacks: 0,
                menu_heartbeats: callbacks,
                steam_observer_armed: true,
                teardown_observed: false,
                signal_counts: [0; 4],
                installed_hooks_mask: OFFLINE_ATTESTATION_LOADING_MENU_HOOK_MASK,
                restored_hooks_mask: OFFLINE_ATTESTATION_LOADING_MENU_HOOK_MASK,
                restored_slot_count: OFFLINE_ATTESTATION_LOADING_MENU_SLOT_COUNT,
                overwritten_events: 0,
                producer_closed: true,
                target_identity_stable: true,
                slot_snapshot_before_sha256: [0xCC; 32],
                slot_snapshot_after_sha256: [0xCC; 32],
            },
        )
    }

    #[test]
    fn loading_menu_policy_is_scenario_bound_and_required() {
        let root = tempfile::tempdir().unwrap();
        loading_fixture(&root.path().join("loading"), 60).unwrap();
        let ordinary = root.path().join("ordinary");
        fixture(&ordinary, 60).unwrap();
        fs::remove_file(ordinary.join("analysis.json")).unwrap();
        fs::remove_file(ordinary.join("prerequisite.json")).unwrap();
        let mut manifest: OfflineAttestationManifest =
            serde_json::from_slice(&fs::read(ordinary.join("manifest.json")).unwrap()).unwrap();
        manifest.scenario = "loading".to_owned();
        fs::remove_file(ordinary.join("manifest.json")).unwrap();
        write_new_json(&ordinary.join("manifest.json"), &manifest).unwrap();
        assert_eq!(
            analyze_offline_attestation(&ordinary).unwrap().result,
            "FAIL"
        );

        let loading = root.path().join("flag-on-active-run");
        fixture(&loading, 60).unwrap();
        fs::remove_file(loading.join("analysis.json")).unwrap();
        fs::remove_file(loading.join("prerequisite.json")).unwrap();
        let mut events = read_events(&loading.join("events.jsonl")).unwrap();
        for event in &mut events {
            event.flags = format!(
                "0x{:08X}",
                event.flags_value().unwrap() | OFFLINE_FLAG_LOADING_MENU_INACTIVE
            );
        }
        fs::remove_file(loading.join("events.jsonl")).unwrap();
        write_new_jsonl(&loading.join("events.jsonl"), &events).unwrap();
        assert_eq!(
            analyze_offline_attestation(&loading).unwrap().result,
            "FAIL"
        );
    }

    #[test]
    fn sixty_and_sixty_one_callbacks_pass_but_fifty_nine_fail() {
        for callbacks in [60, 61] {
            let directory = tempfile::tempdir().unwrap();
            let run = directory.path().join(format!("run-{callbacks}"));
            let decision = fixture(&run, callbacks).unwrap();
            assert_eq!(decision.status, "PASS");
            assert!(!decision.active_input_allowed);
            assert!(!decision.ready_for_arm_token_cas);

            let loading_root = tempfile::tempdir().unwrap();
            let loading_run = loading_root.path().join(format!("loading-{callbacks}"));
            let loading_decision = loading_fixture(&loading_run, callbacks).unwrap();
            assert_eq!(loading_decision.clock_kind, OFFLINE_CLOCK_MENU);
            assert_eq!(
                loading_decision.generation_identity_kind,
                OFFLINE_GENERATION_MENU
            );
        }

        let directory = tempfile::tempdir().unwrap();
        let run = directory.path().join("run-59");
        assert!(fixture(&run, 59).is_err());
        let analysis: OfflineAttestationAnalysis =
            serde_json::from_slice(&fs::read(run.join("analysis.json")).unwrap()).unwrap();
        assert_eq!(analysis.result, "FAIL");

        let loading_root = tempfile::tempdir().unwrap();
        assert!(loading_fixture(&loading_root.path().join("loading-59"), 59).is_err());
    }

    #[test]
    fn loading_menu_clock_and_lua_unbound_contract_fail_closed() {
        type Mutation =
            Box<dyn Fn(&mut OfflineAttestationManifest, &mut Vec<OfflineAttestationEvent>)>;
        let mutations: Vec<Mutation> = vec![
            Box::new(|_, events| events[1].return_rva = Some(0x0065_E68D)),
            Box::new(|_, events| events[1].result_bits = None),
            Box::new(|_, events| {
                events[1].flags = format!(
                    "0x{:08X}",
                    events[1].flags_value().unwrap() & !OFFLINE_FLAG_EXACT_RETURN
                )
            }),
            Box::new(|_, events| {
                events[1].flags = format!(
                    "0x{:08X}",
                    events[1].flags_value().unwrap() & !OFFLINE_FLAG_WINDOW_MAIN_THREAD
                )
            }),
            Box::new(|_, events| {
                events[1].flags = format!(
                    "0x{:08X}",
                    events[1].flags_value().unwrap() & !OFFLINE_FLAG_LUA_UNBOUND_MENU
                )
            }),
            Box::new(|_, events| events[1].kind = "SIMULATIONCALLBACK".to_owned()),
            Box::new(|manifest, _| manifest.menu_heartbeats -= 1),
            Box::new(|manifest, _| manifest.simulation_callbacks = 1),
            Box::new(|manifest, _| manifest.expected_window_thread_id += 1),
            Box::new(|manifest, _| manifest.clock_kind = OFFLINE_CLOCK_SIMULATION.to_owned()),
            Box::new(|manifest, _| {
                manifest.generation_identity_kind = OFFLINE_GENERATION_LUA.to_owned()
            }),
            Box::new(|manifest, _| manifest.heartbeat_return_rva = None),
            Box::new(|manifest, _| manifest.installed_hooks_mask = "0x3F".to_owned()),
            Box::new(|manifest, _| manifest.restored_slot_count = 6),
        ];
        for (index, mutate) in mutations.into_iter().enumerate() {
            let root = tempfile::tempdir().unwrap();
            let run = root.path().join(format!("loading-mutation-{index}"));
            loading_fixture(&run, 60).unwrap();
            fs::remove_file(run.join("analysis.json")).unwrap();
            fs::remove_file(run.join("prerequisite.json")).unwrap();
            let mut manifest: OfflineAttestationManifest =
                serde_json::from_slice(&fs::read(run.join("manifest.json")).unwrap()).unwrap();
            let mut events = read_events(&run.join("events.jsonl")).unwrap();
            mutate(&mut manifest, &mut events);
            fs::remove_file(run.join("manifest.json")).unwrap();
            fs::remove_file(run.join("events.jsonl")).unwrap();
            write_new_json(&run.join("manifest.json"), &manifest).unwrap();
            write_new_jsonl(&run.join("events.jsonl"), &events).unwrap();
            assert_eq!(analyze_offline_attestation(&run).unwrap().result, "FAIL");
        }
    }

    #[test]
    fn wrong_thread_gap_signals_unarmed_and_restore_mutations_fail() {
        type Mutation =
            Box<dyn Fn(&mut OfflineAttestationManifest, &mut Vec<OfflineAttestationEvent>)>;
        let mutations: Vec<Mutation> = vec![
            Box::new(|_, events| events[1].event_sequence += 1),
            Box::new(|_, events| events[1].callback_sequence += 1),
            Box::new(|_, events| events[1].thread_id += 1),
            Box::new(|_, events| events[1].generation += 1),
            Box::new(|_, events| events[1].game_root_identity_sha256 = Some("1".repeat(64))),
            Box::new(|_, events| events[1].lua_generation_identity_sha256 = Some("2".repeat(64))),
            Box::new(|_, events| events[1].original_calls = 0),
            Box::new(|_, events| events[1].flags = "0x00000007".to_owned()),
            Box::new(|manifest, _| manifest.main_thread_id += 1),
            Box::new(|manifest, _| manifest.generation += 1),
            Box::new(|manifest, _| manifest.simulation_callbacks -= 1),
            Box::new(|manifest, _| manifest.identity_salt = "0".repeat(64)),
            Box::new(|manifest, _| manifest.manager_signal_count = 1),
            Box::new(|manifest, _| manifest.roster_signal_count = 1),
            Box::new(|manifest, _| manifest.channel_zero_count = 1),
            Box::new(|manifest, _| manifest.nonzero_channel_count = 1),
            Box::new(|manifest, _| manifest.steam_observer_armed = false),
            Box::new(|manifest, _| manifest.teardown_observed = true),
            Box::new(|manifest, _| manifest.installed_hooks_mask = "0x0".to_owned()),
            Box::new(|manifest, _| manifest.restored_hooks_mask = "0x0".to_owned()),
            Box::new(|manifest, _| manifest.restored_slot_count = 5),
            Box::new(|manifest, _| manifest.overwritten_events = 1),
            Box::new(|manifest, _| manifest.producer_closed = false),
            Box::new(|manifest, _| manifest.target_identity_stable = false),
            Box::new(|manifest, _| manifest.slot_snapshot_before_sha256 = "0".repeat(64)),
            Box::new(|manifest, _| manifest.slot_snapshot_after_sha256 = "0".repeat(64)),
        ];
        for (index, mutate) in mutations.into_iter().enumerate() {
            let root = tempfile::tempdir().unwrap();
            let run = root.path().join(format!("run-{index}"));
            fixture(&run, 60).unwrap();
            fs::remove_file(run.join("analysis.json")).unwrap();
            fs::remove_file(run.join("prerequisite.json")).unwrap();
            let mut manifest: OfflineAttestationManifest =
                serde_json::from_slice(&fs::read(run.join("manifest.json")).unwrap()).unwrap();
            let mut events = read_events(&run.join("events.jsonl")).unwrap();
            mutate(&mut manifest, &mut events);
            fs::remove_file(run.join("manifest.json")).unwrap();
            fs::remove_file(run.join("events.jsonl")).unwrap();
            write_new_json(&run.join("manifest.json"), &manifest).unwrap();
            write_new_jsonl(&run.join("events.jsonl"), &events).unwrap();
            assert_eq!(analyze_offline_attestation(&run).unwrap().result, "FAIL");
        }
    }

    #[test]
    fn prerequisite_expiry_is_fail_closed() {
        let root = tempfile::tempdir().unwrap();
        let run = root.path().join("run");
        let decision = fixture(&run, 60).unwrap();
        validate_offline_prerequisite(
            &run.join("prerequisite.json"),
            decision.issued_at_utc + Duration::seconds(1),
        )
        .unwrap();
        assert!(
            validate_offline_prerequisite(&run.join("prerequisite.json"), decision.expires_at_utc)
                .is_err()
        );
    }

    #[test]
    fn prerequisite_replays_all_three_source_hashes() {
        let root = tempfile::tempdir().unwrap();
        let run = root.path().join("run");
        let decision = fixture(&run, 60).unwrap();
        validate_offline_prerequisite(
            &run.join("prerequisite.json"),
            decision.issued_at_utc + Duration::seconds(1),
        )
        .unwrap();
        fs::write(run.join("events.jsonl"), b"mutated\n").unwrap();
        assert!(
            validate_offline_prerequisite(
                &run.join("prerequisite.json"),
                decision.issued_at_utc + Duration::seconds(2),
            )
            .is_err()
        );
    }
}
