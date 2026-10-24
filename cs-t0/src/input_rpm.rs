use std::collections::BTreeMap;
use std::fs::{self, OpenOptions};
use std::io::{BufWriter, Write};
use std::mem::size_of;
use std::path::{Path, PathBuf};
use std::thread;
use std::time::{Duration, Instant};

use serde::{Deserialize, Serialize};

use crate::artifact::{file_sha256, filetime_100ns_to_utc, utc_now};
use crate::error::{AppError, AppResult, UsageContext};
use crate::win32::{self, OBSERVER_ACCESS_BITS, ObservedProcess};

pub const RPM_SCHEMA_VERSION: u32 = 2;
pub const RPM_EVIDENCE_CLASS: &str = "RPM_DIAGNOSTIC_ONLY";
pub const FIXED_TARGET_SHA256: &str =
    "C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30";
const DIAGNOSTIC_CLAIM: &str =
    "best-effort coherent diagnostic only; not gapless, per-tick, canonical, or apply evidence";

const INPUT_MANAGER_RVA: u64 = 0x0085_7B18;
const GAME_ROOT_SLOT_RVA: u64 = 0x0087_1678;
// Read-only Original Online layout, cross-checked against the original IsaacSync
// runtime_target.hpp/runtime_roster.cpp and the schema-4 Native mode gate.
const ONLINE_MANAGER_SLOT_RVA: u64 = 0x0087_169C;
const NETWORK_BACKEND_SLOT_RVA: u64 = 0x0087_9A58;
const STEAM_BACKEND_VTABLE_RVA: u64 = 0x007A_2440;
const SYNC_MANAGER_OFFSET: u64 = 0x0004_B3D8;
const GAME_PLAYER_VECTOR_OFFSET: u64 = 0x0001_BAA8;
const GAME_RUN_ACTIVE_OFFSET: u64 = 0x0002_658A;
const PLAYER_DEVICE_KEY_OFFSET: u64 = 0x1618;
const ONLINE_DEVICE_IDENTITY_OFFSET: u64 = 0x374;
const ONLINE_IDENTITY_VALUE_OFFSET: u64 = 8;
const DEVICE_KEY_OFFSET: usize = 0x0C;
const DEVICE_BINDING_BEGIN_OFFSET: usize = 0x10;
const DEVICE_BINDING_END_OFFSET: usize = 0x14;
const DEVICE_CACHED_CURRENT_OFFSET: usize = 0xC0;
const DEVICE_CACHED_PREVIOUS_OFFSET: usize = 0xC4;
const DEVICE_THRESHOLD_OFFSET: usize = 0xD4;
const GAMEPAD_PLATFORM_ID_OFFSET: u64 = 0x370;
const DEVICE_CORE_SIZE: usize = 0xD8;
const LOGICAL_ACTION_COUNT: usize = 12;
const MAX_DEVICE_COUNT: usize = 32;
const MAX_PLAYER_COUNT: usize = 16;
const MAX_BINDING_COUNT: usize = 128;
const DEFAULT_RETRIES: u32 = 4;

#[derive(Clone, Copy)]
struct DeviceProfile {
    kind: &'static str,
    vtable_rva: u64,
    col_rva: u64,
    primitive_pressed_rva: u64,
    primitive_triggered_rva: u64,
    primitive_value_rva: u64,
    gamepad: bool,
}

const DEVICE_PROFILES: [DeviceProfile; 2] = [
    DeviceProfile {
        kind: "KEYBOARD",
        vtable_rva: 0x007A_1678,
        col_rva: 0x007B_191C,
        primitive_pressed_rva: 0x0066_C510,
        primitive_triggered_rva: 0x0066_C540,
        primitive_value_rva: 0x0066_C580,
        gamepad: false,
    },
    DeviceProfile {
        kind: "GAMEPAD",
        vtable_rva: 0x0078_2C80,
        col_rva: 0x007B_0460,
        primitive_pressed_rva: 0x0061_E610,
        primitive_triggered_rva: 0x0061_E640,
        primitive_value_rva: 0x0061_E790,
        gamepad: true,
    },
];

const QUERY_PRESSED_RVA: u64 = 0x0061_E8D0;
const QUERY_TRIGGERED_RVA: u64 = 0x0061_E970;
const QUERY_VALUE_RVA: u64 = 0x0061_EA20;

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputRpmManifest {
    pub schema_version: u32,
    pub evidence_class: String,
    pub tool_identity: String,
    pub started_utc: String,
    pub ended_utc: Option<String>,
    pub target_sha256: String,
    pub process_id: u32,
    pub process_creation_utc: Option<String>,
    pub module_base: String,
    pub module_size: u32,
    pub debugger_present: bool,
    pub observer_access_bits: String,
    pub interval_milliseconds: u64,
    pub requested_duration_milliseconds: u64,
    pub qpc_frequency: i64,
    pub guarantees: Vec<String>,
    pub limitations: Vec<String>,
    pub eligible_for_active_gate: bool,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputRpmDevice {
    pub object: String,
    pub reference_owner: String,
    pub kind: String,
    pub device_key: i32,
    pub enabled_byte: u8,
    pub binding_count: usize,
    pub cached_current: Vec<f32>,
    pub cached_previous: Vec<f32>,
    pub threshold: f32,
    pub platform_controller_id: Option<i32>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputRpmPlayer {
    pub object: String,
    pub device_key: i32,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputRpmOwnershipJoin {
    pub device_key: i32,
    pub device_kind: String,
    pub result: String,
    pub matching_player_count: usize,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputRpmEvent {
    pub observer_sequence: u64,
    pub qpc: i64,
    pub status: String,
    pub retry_count: u32,
    pub reason: Option<String>,
    pub devices: Vec<InputRpmDevice>,
    pub players: Vec<InputRpmPlayer>,
    pub ownership: Vec<InputRpmOwnershipJoin>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputRpmSummary {
    pub schema_version: u32,
    pub evidence_class: String,
    pub result: String,
    pub output_directory: Option<PathBuf>,
    pub process_id: u32,
    pub requested_samples: u64,
    pub coherent_samples: u64,
    pub unstable_drops: u64,
    pub read_error_drops: u64,
    pub ownership_results: BTreeMap<String, u64>,
    pub active_input_allowed: bool,
    pub claim: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputRpmCheck {
    pub id: String,
    pub passed: bool,
    pub detail: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InputRpmAnalysis {
    pub schema_version: u32,
    pub evidence_class: String,
    pub result: String,
    pub event_count: usize,
    pub coherent_samples: u64,
    pub unstable_drops: u64,
    pub read_error_drops: u64,
    pub checks: Vec<InputRpmCheck>,
    pub eligible_for_active_gate: bool,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
struct VectorHeader {
    begin: u32,
    end: u32,
    capacity_end: u32,
}

enum AttemptFailure {
    Unstable(String),
    Read(String),
}

type SnapshotData = (
    Vec<InputRpmDevice>,
    Vec<InputRpmPlayer>,
    Vec<InputRpmOwnershipJoin>,
);

#[derive(Clone, Debug, Eq, PartialEq)]
pub(crate) struct ActivePreflightDeviceIdentity {
    pub object: String,
    pub reference_owner: String,
    pub kind: String,
    pub device_key: i32,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub(crate) struct ActivePreflightPlayerIdentity {
    pub object: String,
    pub device_key: i32,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub(crate) struct ActivePreflightOwnershipSnapshot {
    pub devices: Vec<ActivePreflightDeviceIdentity>,
    pub players: Vec<ActivePreflightPlayerIdentity>,
    pub ownership: Vec<InputRpmOwnershipJoin>,
    pub game_root_identity: u32,
    pub manager_identity: u32,
    pub game_root_present: bool,
    pub run_active: bool,
    pub online_negative_screen_passed: bool,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub(crate) struct OriginalOnlinePositiveSnapshot {
    pub qpc: u64,
    pub game_root: u32,
    pub run_active: bool,
    pub manager: u32,
    pub network_backend: u32,
    pub roster_identities: Vec<u64>,
    pub online_frame: u32,
}

pub fn inspect(game_exe: &Path, pid: Option<u32>) -> AppResult<InputRpmSummary> {
    let process = open_target(game_exe, pid)?;
    validate_runtime_profile(&process)?;
    let event = collect_with_retries(&process, 1, DEFAULT_RETRIES)?;
    let mut summary = new_summary(process.pid, None, 1);
    incorporate(&mut summary, &event);
    summary.result = if event.status == "COHERENT_BEST_EFFORT" {
        "PASS_DIAGNOSTIC".to_owned()
    } else {
        "BLOCKED_BY_UNSTABLE_SNAPSHOT".to_owned()
    };
    Ok(summary)
}

pub fn capture(
    game_exe: &Path,
    pid: Option<u32>,
    output_directory: &Path,
    duration_milliseconds: u64,
    interval_milliseconds: u64,
) -> AppResult<InputRpmSummary> {
    if duration_milliseconds == 0 || duration_milliseconds > 300_000 {
        return Err(AppError::usage(
            "duration-ms must be in the inclusive range 1..300000",
        ));
    }
    if interval_milliseconds == 0 || interval_milliseconds > 1_000 {
        return Err(AppError::usage(
            "interval-ms must be in the inclusive range 1..1000",
        ));
    }
    if output_directory.exists() {
        return Err(AppError::usage(format!(
            "RPM output directory already exists: {}",
            output_directory.display()
        )));
    }
    let parent = output_directory.parent().ok_or_else(|| {
        AppError::usage("RPM output directory must have an existing parent directory")
    })?;
    if !parent.is_dir() {
        return Err(AppError::usage(format!(
            "RPM output parent does not exist: {}",
            parent.display()
        )));
    }

    let process = open_target(game_exe, pid)?;
    validate_runtime_profile(&process)?;
    fs::create_dir(output_directory).usage_context(format!(
        "cannot create RPM output directory {}",
        output_directory.display()
    ))?;

    let mut manifest = InputRpmManifest {
        schema_version: RPM_SCHEMA_VERSION,
        evidence_class: RPM_EVIDENCE_CLASS.to_owned(),
        tool_identity: "IsaacCsT0-InputRpmObserver-v2".to_owned(),
        started_utc: utc_now(),
        ended_utc: None,
        target_sha256: FIXED_TARGET_SHA256.to_owned(),
        process_id: process.pid,
        process_creation_utc: filetime_100ns_to_utc(process.creation_time_100ns),
        module_base: address(process.main_module.base),
        module_size: process.main_module.size,
        debugger_present: process.debugger_present()?,
        observer_access_bits: format!("0x{OBSERVER_ACCESS_BITS:08X}"),
        interval_milliseconds,
        requested_duration_milliseconds: duration_milliseconds,
        qpc_frequency: win32::qpc_frequency()?,
        guarantees: vec![
            "PROCESS_QUERY/VM_READ/SYNCHRONIZE only; no remote execution or write access"
                .to_owned(),
            "active vector entries only; capacity tail is never interpreted".to_owned(),
            "headers, structures, and cached arrays are double-collected".to_owned(),
            "unknown vtables and concurrent changes drop the sample; ambiguous ownership is reported as UNKNOWN and cannot unlock active input"
                .to_owned(),
        ],
        limitations: vec![
            "best-effort coherent snapshots are not gapless and cannot exclude ABA".to_owned(),
            "polling can miss short edges between samples".to_owned(),
            "samples have no proven loopIteration or Authority tick ownership".to_owned(),
            "cached logical magnitudes are diagnostic evidence, not canonical input bytes"
                .to_owned(),
        ],
        eligible_for_active_gate: false,
    };
    write_new_pretty(&output_directory.join("rpm-manifest.json"), &manifest)?;
    let events_path = output_directory.join("rpm-events.jsonl");
    let events_file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(&events_path)
        .usage_context(format!("cannot create {}", events_path.display()))?;
    let mut writer = BufWriter::new(events_file);
    let interval = Duration::from_millis(interval_milliseconds);
    let deadline = Instant::now() + Duration::from_millis(duration_milliseconds);
    let mut sequence = 1_u64;
    let mut summary = new_summary(process.pid, Some(output_directory.to_path_buf()), 0);
    loop {
        let event = collect_with_retries(&process, sequence, DEFAULT_RETRIES)?;
        serde_json::to_writer(&mut writer, &event)
            .usage_context(format!("cannot serialize {}", events_path.display()))?;
        writer
            .write_all(b"\n")
            .usage_context(format!("cannot write {}", events_path.display()))?;
        incorporate(&mut summary, &event);
        summary.requested_samples = summary.requested_samples.saturating_add(1);
        sequence = sequence
            .checked_add(1)
            .ok_or_else(|| AppError::evidence("RPM observer sequence overflow"))?;
        if Instant::now() >= deadline || process.wait(0)? {
            break;
        }
        thread::sleep(interval);
    }
    writer
        .flush()
        .usage_context(format!("cannot flush {}", events_path.display()))?;
    manifest.ended_utc = Some(utc_now());
    replace_pretty(&output_directory.join("rpm-manifest.json"), &manifest)?;
    summary.result = if summary.coherent_samples > 0 {
        "PASS_DIAGNOSTIC".to_owned()
    } else {
        "BLOCKED_BY_UNSTABLE_SNAPSHOT".to_owned()
    };
    write_new_pretty(&output_directory.join("rpm-summary.json"), &summary)?;
    Ok(summary)
}

pub fn analyze(run_directory: &Path) -> AppResult<InputRpmAnalysis> {
    let manifest_path = run_directory.join("rpm-manifest.json");
    let events_path = run_directory.join("rpm-events.jsonl");
    let summary_path = run_directory.join("rpm-summary.json");
    let manifest: InputRpmManifest = serde_json::from_slice(
        &fs::read(&manifest_path)
            .usage_context(format!("cannot read {}", manifest_path.display()))?,
    )
    .usage_context(format!("cannot parse {}", manifest_path.display()))?;
    let summary: InputRpmSummary = serde_json::from_slice(
        &fs::read(&summary_path)
            .usage_context(format!("cannot read {}", summary_path.display()))?,
    )
    .usage_context(format!("cannot parse {}", summary_path.display()))?;
    let text = fs::read_to_string(&events_path)
        .usage_context(format!("cannot read {}", events_path.display()))?;
    let mut events = Vec::new();
    for (index, line) in text.lines().enumerate() {
        if line.trim().is_empty() {
            continue;
        }
        events.push(
            serde_json::from_str::<InputRpmEvent>(line).usage_context(format!(
                "cannot parse {} line {}",
                events_path.display(),
                index + 1
            ))?,
        );
    }

    let manifest_valid = manifest.schema_version == RPM_SCHEMA_VERSION
        && manifest.evidence_class == RPM_EVIDENCE_CLASS
        && manifest.tool_identity == "IsaacCsT0-InputRpmObserver-v2"
        && manifest
            .target_sha256
            .eq_ignore_ascii_case(FIXED_TARGET_SHA256)
        && manifest.ended_utc.is_some()
        && manifest.observer_access_bits == format!("0x{OBSERVER_ACCESS_BITS:08X}")
        && manifest.process_id != 0
        && manifest.module_size != 0
        && valid_address_text(&manifest.module_base)
        && manifest.interval_milliseconds != 0
        && manifest.interval_milliseconds <= 1_000
        && manifest.requested_duration_milliseconds != 0
        && manifest.requested_duration_milliseconds <= 300_000
        && manifest.qpc_frequency > 0
        && !manifest.eligible_for_active_gate;
    let nonempty = !events.is_empty();
    let sequence_valid = events
        .iter()
        .enumerate()
        .all(|(index, event)| event.observer_sequence == index as u64 + 1);
    let qpc_valid = events.iter().all(|event| event.qpc > 0)
        && events.windows(2).all(|pair| pair[0].qpc <= pair[1].qpc);
    let shape_valid = events.iter().all(event_shape_valid);

    let coherent_samples = events
        .iter()
        .filter(|event| event.status == "COHERENT_BEST_EFFORT")
        .count() as u64;
    let unstable_drops = events
        .iter()
        .filter(|event| event.status == "DROPPED_UNSTABLE")
        .count() as u64;
    let read_error_drops = events
        .iter()
        .filter(|event| event.status == "DROPPED_READ_ERROR")
        .count() as u64;
    let mut ownership_results = BTreeMap::new();
    for event in &events {
        for join in &event.ownership {
            *ownership_results
                .entry(format!("{}:{}", join.device_kind, join.result))
                .or_insert(0_u64) += 1;
        }
    }
    let summary_valid = summary.schema_version == RPM_SCHEMA_VERSION
        && summary.evidence_class == RPM_EVIDENCE_CLASS
        && summary.process_id == manifest.process_id
        && summary.requested_samples == events.len() as u64
        && summary.coherent_samples == coherent_samples
        && summary.unstable_drops == unstable_drops
        && summary.read_error_drops == read_error_drops
        && summary.ownership_results == ownership_results
        && !summary.active_input_allowed
        && summary.result
            == if coherent_samples > 0 {
                "PASS_DIAGNOSTIC"
            } else {
                "BLOCKED_BY_UNSTABLE_SNAPSHOT"
            };
    let diagnostic_only = !manifest.eligible_for_active_gate
        && !summary.active_input_allowed
        && manifest
            .limitations
            .iter()
            .any(|item| item.contains("not gapless"))
        && summary.claim == DIAGNOSTIC_CLAIM;

    let checks = vec![
        rpm_check(
            "manifest_identity",
            manifest_valid,
            "fixed schema/tool/target/access rights and completed diagnostic-only manifest",
        ),
        rpm_check(
            "events_nonempty",
            nonempty,
            "capture must contain at least one event",
        ),
        rpm_check(
            "coherent_samples",
            coherent_samples > 0,
            "capture must contain at least one coherent best-effort sample",
        ),
        rpm_check(
            "observer_sequence",
            sequence_valid,
            "observerSequence must start at one and remain contiguous",
        ),
        rpm_check(
            "qpc_monotonic",
            qpc_valid,
            "QPC timestamps must be positive and nondecreasing",
        ),
        rpm_check(
            "event_shape",
            shape_valid,
            "coherent events require validated devices/players/joins; drops must be empty and explained",
        ),
        rpm_check(
            "summary_replay",
            summary_valid,
            "summary counts and result must replay exactly from events",
        ),
        rpm_check(
            "diagnostic_only",
            diagnostic_only,
            "RPM evidence must remain explicitly ineligible for the active gate",
        ),
    ];
    let passed = checks.iter().all(|check| check.passed);
    Ok(InputRpmAnalysis {
        schema_version: RPM_SCHEMA_VERSION,
        evidence_class: RPM_EVIDENCE_CLASS.to_owned(),
        result: if passed { "PASS_DIAGNOSTIC" } else { "FAIL" }.to_owned(),
        event_count: events.len(),
        coherent_samples,
        unstable_drops,
        read_error_drops,
        checks,
        eligible_for_active_gate: false,
    })
}

fn open_target(game_exe: &Path, pid: Option<u32>) -> AppResult<ObservedProcess> {
    let hash = file_sha256(game_exe)?;
    if !hash.eq_ignore_ascii_case(FIXED_TARGET_SHA256) {
        return Err(AppError::evidence(format!(
            "RPM observer is fixed to target SHA-256 {FIXED_TARGET_SHA256}, got {hash}"
        )));
    }
    let pid = match pid {
        Some(pid) => pid,
        None => {
            let pids = win32::find_target_pids()?;
            if pids.len() != 1 {
                return Err(AppError::evidence(format!(
                    "RPM observer requires exactly one isaac-ng.exe process, found {}",
                    pids.len()
                )));
            }
            pids[0]
        }
    };
    ObservedProcess::open_exact(pid, game_exe)
}

pub(crate) fn validate_runtime_profile(process: &ObservedProcess) -> AppResult<()> {
    let base = process.main_module.base;
    for profile in DEVICE_PROFILES {
        let vtable = checked_add(base, profile.vtable_rva)?;
        let col = read_u32(process, vtable - 4)?;
        expect_address(
            col,
            checked_add(base, profile.col_rva)?,
            profile.kind,
            "COL",
        )?;
        expect_address(
            read_u32(process, vtable + 0x24)?,
            checked_add(base, profile.primitive_pressed_rva)?,
            profile.kind,
            "primitive +0x24",
        )?;
        expect_address(
            read_u32(process, vtable + 0x28)?,
            checked_add(base, profile.primitive_triggered_rva)?,
            profile.kind,
            "primitive +0x28",
        )?;
        expect_address(
            read_u32(process, vtable + 0x2C)?,
            checked_add(base, profile.primitive_value_rva)?,
            profile.kind,
            "primitive +0x2C",
        )?;
        expect_address(
            read_u32(process, vtable + 0x30)?,
            checked_add(base, QUERY_PRESSED_RVA)?,
            profile.kind,
            "logical bool +0x30",
        )?;
        expect_address(
            read_u32(process, vtable + 0x34)?,
            checked_add(base, QUERY_TRIGGERED_RVA)?,
            profile.kind,
            "logical triggered +0x34",
        )?;
        expect_address(
            read_u32(process, vtable + 0x38)?,
            checked_add(base, QUERY_VALUE_RVA)?,
            profile.kind,
            "logical float +0x38",
        )?;
    }
    Ok(())
}

pub(crate) fn active_preflight_ownership_snapshot(
    process: &ObservedProcess,
) -> AppResult<ActivePreflightOwnershipSnapshot> {
    validate_runtime_profile(process)?;
    let mode_before = read_active_negative_screen(process)?;
    let event = collect_with_retries(process, 1, DEFAULT_RETRIES)?;
    if event.status != "COHERENT_BEST_EFFORT" {
        return Err(AppError::evidence(format!(
            "active owner snapshot is not coherent: {}",
            event.reason.as_deref().unwrap_or(event.status.as_str())
        )));
    }
    let mode_after = read_active_negative_screen(process)?;
    if mode_before != mode_after {
        return Err(AppError::evidence(
            "game root, run-active or online-manager negative screen changed during owner collection",
        ));
    }
    Ok(ActivePreflightOwnershipSnapshot {
        devices: event
            .devices
            .iter()
            .map(|device| ActivePreflightDeviceIdentity {
                object: device.object.clone(),
                reference_owner: device.reference_owner.clone(),
                kind: device.kind.clone(),
                device_key: device.device_key,
            })
            .collect(),
        players: event
            .players
            .iter()
            .map(|player| ActivePreflightPlayerIdentity {
                object: player.object.clone(),
                device_key: player.device_key,
            })
            .collect(),
        ownership: event.ownership,
        game_root_identity: mode_before.game_root,
        manager_identity: mode_before.manager,
        game_root_present: mode_before.game_root != 0,
        run_active: mode_before.run_active,
        online_negative_screen_passed: mode_before.online_negative_screen_passed(),
    })
}

pub(crate) fn original_online_positive_snapshot(
    process: &ObservedProcess,
) -> AppResult<OriginalOnlinePositiveSnapshot> {
    let before = read_active_negative_screen(process)?;
    validate_original_online_screen(before)?;
    validate_original_online_backend(process, before)?;
    let roster_before = read_original_online_roster(process, before)?;
    let after = read_active_negative_screen(process)?;
    validate_original_online_screen(after)?;
    validate_original_online_backend(process, after)?;
    let roster_after = read_original_online_roster(process, after)?;
    if !original_online_screens_stable(before, after) || roster_before != roster_after {
        return Err(AppError::evidence(
            "Original Online manager, backend, roster, game root, or run state changed, or the online frame regressed during one read-only observation",
        ));
    }
    let qpc = u64::try_from(win32::qpc_now()?)
        .map_err(|_| AppError::evidence("Original Online observation QPC is not positive"))?;
    Ok(OriginalOnlinePositiveSnapshot {
        qpc,
        game_root: before.game_root,
        run_active: before.run_active,
        manager: before.manager,
        network_backend: before.network_backend,
        roster_identities: roster_before,
        online_frame: after.online_frame,
    })
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
struct ActiveNegativeScreen {
    game_root: u32,
    run_active: bool,
    manager: u32,
    network_backend: u32,
    roster_first: u32,
    roster_last: u32,
    roster_capacity: u32,
    online_frame: u32,
}

impl ActiveNegativeScreen {
    fn online_negative_screen_passed(self) -> bool {
        self.manager & 3 == 0
            && self.network_backend == 0
            && self.roster_first == 0
            && self.roster_last == 0
            && self.roster_capacity == 0
            && self.online_frame == 0
    }
}

fn validate_original_online_screen(screen: ActiveNegativeScreen) -> AppResult<()> {
    if screen.game_root == 0
        || screen.game_root & 3 != 0
        || !screen.run_active
        || screen.manager == 0
        || screen.manager & 3 != 0
        || screen.network_backend == 0
        || screen.network_backend & 3 != 0
        || screen.online_frame == u32::MAX
    {
        return Err(AppError::evidence(
            "Original Online positive screen requires aligned nonzero game/manager/backend identities, an active Run, and a readable online frame",
        ));
    }
    let count = validated_count(
        screen.roster_first,
        screen.roster_last,
        screen.roster_capacity,
        4,
        4,
    )
    .map_err(|reason| AppError::evidence(format!("invalid Original Online roster: {reason}")))?;
    if !(2..=4).contains(&count) {
        return Err(AppError::evidence(format!(
            "Original Online positive screen requires 2-4 roster entries, found {count}"
        )));
    }
    Ok(())
}

fn original_online_screens_stable(
    before: ActiveNegativeScreen,
    after: ActiveNegativeScreen,
) -> bool {
    before.game_root == after.game_root
        && before.run_active == after.run_active
        && before.manager == after.manager
        && before.network_backend == after.network_backend
        && before.roster_first == after.roster_first
        && before.roster_last == after.roster_last
        && before.roster_capacity == after.roster_capacity
        && after.online_frame >= before.online_frame
}

fn validate_original_online_backend(
    process: &ObservedProcess,
    screen: ActiveNegativeScreen,
) -> AppResult<()> {
    let live_vtable = read_u32(process, u64::from(screen.network_backend))?;
    let expected_vtable = checked_add(process.main_module.base, STEAM_BACKEND_VTABLE_RVA)?;
    if u64::from(live_vtable) != expected_vtable {
        return Err(AppError::evidence(
            "Original Online network backend does not have the frozen Steam backend vtable",
        ));
    }
    Ok(())
}

fn read_original_online_roster(
    process: &ObservedProcess,
    screen: ActiveNegativeScreen,
) -> AppResult<Vec<u64>> {
    let count = validated_count(
        screen.roster_first,
        screen.roster_last,
        screen.roster_capacity,
        4,
        4,
    )
    .map_err(|reason| AppError::evidence(format!("invalid Original Online roster: {reason}")))?;
    let entries = process.read_memory(u64::from(screen.roster_first), count * 4)?;
    let mut identities = Vec::with_capacity(count);
    for entry in entries.chunks_exact(4) {
        let device = u32::from_le_bytes(entry.try_into().unwrap());
        if device == 0 || device & 3 != 0 {
            return Err(AppError::evidence(
                "Original Online roster contains a null or misaligned device identity",
            ));
        }
        let identity = read_u32(
            process,
            checked_add(u64::from(device), ONLINE_DEVICE_IDENTITY_OFFSET)?,
        )?;
        if identity == 0 || identity & 3 != 0 {
            return Err(AppError::evidence(
                "Original Online roster device has no aligned peer identity object",
            ));
        }
        let raw = process.read_memory(
            checked_add(u64::from(identity), ONLINE_IDENTITY_VALUE_OFFSET)?,
            8,
        )?;
        let value = u64::from_le_bytes(raw.try_into().unwrap());
        if value == 0 {
            return Err(AppError::evidence(
                "Original Online roster contains a zero peer identity",
            ));
        }
        identities.push(value);
    }
    identities.sort_unstable();
    if identities.windows(2).any(|pair| pair[0] == pair[1]) {
        return Err(AppError::evidence(
            "Original Online roster contains duplicate peer identities",
        ));
    }
    Ok(identities)
}

fn read_active_negative_screen(process: &ObservedProcess) -> AppResult<ActiveNegativeScreen> {
    let base = process.main_module.base;
    let game_root = read_u32(process, checked_add(base, GAME_ROOT_SLOT_RVA)?)?;
    let run_active = if game_root == 0 {
        false
    } else {
        let bytes = process.read_memory(
            checked_add(u64::from(game_root), GAME_RUN_ACTIVE_OFFSET)?,
            2,
        )?;
        u16::from_le_bytes(bytes.try_into().unwrap()) & 0x00FF != 0
    };
    let manager = read_u32(process, checked_add(base, ONLINE_MANAGER_SLOT_RVA)?)?;
    let network_backend = read_u32(process, checked_add(base, NETWORK_BACKEND_SLOT_RVA)?)?;
    let (roster_first, roster_last, roster_capacity, online_frame) = if manager == 0 {
        (0, 0, 0, 0)
    } else {
        let sync = checked_add(u64::from(manager), SYNC_MANAGER_OFFSET)?;
        let bytes = process.read_memory(sync, 16)?;
        (
            u32::from_le_bytes(bytes[0..4].try_into().unwrap()),
            u32::from_le_bytes(bytes[4..8].try_into().unwrap()),
            u32::from_le_bytes(bytes[8..12].try_into().unwrap()),
            u32::from_le_bytes(bytes[12..16].try_into().unwrap()),
        )
    };
    Ok(ActiveNegativeScreen {
        game_root,
        run_active,
        manager,
        network_backend,
        roster_first,
        roster_last,
        roster_capacity,
        online_frame,
    })
}

fn collect_with_retries(
    process: &ObservedProcess,
    observer_sequence: u64,
    retries: u32,
) -> AppResult<InputRpmEvent> {
    let qpc = win32::qpc_now()?;
    let mut last_failure = AttemptFailure::Unstable("no snapshot attempt".to_owned());
    for retry in 0..=retries {
        match collect_once(process) {
            Ok((devices, players, ownership)) => {
                return Ok(InputRpmEvent {
                    observer_sequence,
                    qpc,
                    status: "COHERENT_BEST_EFFORT".to_owned(),
                    retry_count: retry,
                    reason: None,
                    devices,
                    players,
                    ownership,
                });
            }
            Err(failure) => last_failure = failure,
        }
    }
    let (status, reason) = match last_failure {
        AttemptFailure::Unstable(reason) => ("DROPPED_UNSTABLE", reason),
        AttemptFailure::Read(reason) => ("DROPPED_READ_ERROR", reason),
    };
    Ok(InputRpmEvent {
        observer_sequence,
        qpc,
        status: status.to_owned(),
        retry_count: retries,
        reason: Some(reason),
        devices: Vec::new(),
        players: Vec::new(),
        ownership: Vec::new(),
    })
}

fn collect_once(process: &ObservedProcess) -> Result<SnapshotData, AttemptFailure> {
    let base = process.main_module.base;
    let manager = checked_add_attempt(base, INPUT_MANAGER_RVA)?;
    let input_header_a = read_vector_header(process, manager + 8, 8, MAX_DEVICE_COUNT)?;
    let input_entries = read_active_span(process, input_header_a, 8)?;
    let mut devices = Vec::with_capacity(input_entries.len() / 8);
    for entry in input_entries.chunks_exact(8) {
        let object = u32::from_le_bytes(entry[0..4].try_into().unwrap());
        let reference_owner = u32::from_le_bytes(entry[4..8].try_into().unwrap());
        devices.push(read_device(process, base, object, reference_owner)?);
    }

    let game_slot_address = checked_add_attempt(base, GAME_ROOT_SLOT_RVA)?;
    let game_root_a = read_u32_attempt(process, game_slot_address)?;
    let mut players = Vec::new();
    let mut player_header_a = None;
    let mut player_bytes_a = Vec::new();
    if game_root_a != 0 {
        let player_vector = checked_add_attempt(u64::from(game_root_a), GAME_PLAYER_VECTOR_OFFSET)?;
        let header = read_vector_header(process, player_vector, 4, MAX_PLAYER_COUNT)?;
        player_bytes_a = read_active_span(process, header, 4)?;
        for raw in player_bytes_a.chunks_exact(4) {
            let object = u32::from_le_bytes(raw.try_into().unwrap());
            if object == 0 {
                return Err(AttemptFailure::Unstable(
                    "active player vector contained a null object".to_owned(),
                ));
            }
            let key_address = checked_add_attempt(u64::from(object), PLAYER_DEVICE_KEY_OFFSET)?;
            players.push(InputRpmPlayer {
                object: address(u64::from(object)),
                device_key: read_i32_attempt(process, key_address)?,
            });
        }
        player_header_a = Some(header);
    }

    let input_header_b = read_vector_header(process, manager + 8, 8, MAX_DEVICE_COUNT)?;
    if input_header_a != input_header_b {
        return Err(AttemptFailure::Unstable(
            "input manager vector changed during collection".to_owned(),
        ));
    }
    let input_entries_b = read_active_span(process, input_header_b, 8)?;
    if input_entries != input_entries_b {
        return Err(AttemptFailure::Unstable(
            "input manager active entries changed during collection".to_owned(),
        ));
    }
    let game_root_b = read_u32_attempt(process, game_slot_address)?;
    if game_root_a != game_root_b {
        return Err(AttemptFailure::Unstable(
            "game root changed during collection".to_owned(),
        ));
    }
    if let Some(header_a) = player_header_a {
        let player_vector = checked_add_attempt(u64::from(game_root_a), GAME_PLAYER_VECTOR_OFFSET)?;
        let header_b = read_vector_header(process, player_vector, 4, MAX_PLAYER_COUNT)?;
        if header_a != header_b {
            return Err(AttemptFailure::Unstable(
                "player vector changed during collection".to_owned(),
            ));
        }
        let player_bytes_b = read_active_span(process, header_b, 4)?;
        if player_bytes_a != player_bytes_b {
            return Err(AttemptFailure::Unstable(
                "player vector active entries changed during collection".to_owned(),
            ));
        }
        for player in &players {
            let object = parse_address_text(&player.object)?;
            let key_address = checked_add_attempt(object, PLAYER_DEVICE_KEY_OFFSET)?;
            if read_i32_attempt(process, key_address)? != player.device_key {
                return Err(AttemptFailure::Unstable(
                    "player assignment key changed during collection".to_owned(),
                ));
            }
        }
    }

    let ownership = ownership_joins(&devices, &players);
    Ok((devices, players, ownership))
}

fn read_device(
    process: &ObservedProcess,
    module_base: u64,
    object: u32,
    reference_owner: u32,
) -> Result<InputRpmDevice, AttemptFailure> {
    if object == 0
        || reference_owner == 0
        || !object.is_multiple_of(4)
        || !reference_owner.is_multiple_of(4)
    {
        return Err(AttemptFailure::Unstable(
            "active input entry contained an invalid shared pointer".to_owned(),
        ));
    }
    let object_address = u64::from(object);
    let core_a = read_attempt(process, object_address, DEVICE_CORE_SIZE)?;
    let vtable = read_u32_from(&core_a, 0)?;
    let profile = DEVICE_PROFILES
        .iter()
        .find(|profile| {
            checked_add(module_base, profile.vtable_rva)
                .ok()
                .is_some_and(|expected| u64::from(vtable) == expected)
        })
        .copied()
        .ok_or_else(|| {
            AttemptFailure::Unstable(format!(
                "active input object {} has an unknown vtable 0x{vtable:08X}",
                address(object_address)
            ))
        })?;
    let device_key = read_i32_from(&core_a, DEVICE_KEY_OFFSET)?;
    let binding_begin = read_u32_from(&core_a, DEVICE_BINDING_BEGIN_OFFSET)?;
    let binding_end = read_u32_from(&core_a, DEVICE_BINDING_END_OFFSET)?;
    let binding_count = validated_count(
        binding_begin,
        binding_end,
        binding_end,
        16,
        MAX_BINDING_COUNT,
    )
    .map_err(AttemptFailure::Unstable)?;
    if binding_count < LOGICAL_ACTION_COUNT {
        return Err(AttemptFailure::Unstable(format!(
            "{} binding table has only {binding_count} logical actions",
            profile.kind
        )));
    }
    let current = read_u32_from(&core_a, DEVICE_CACHED_CURRENT_OFFSET)?;
    let previous = read_u32_from(&core_a, DEVICE_CACHED_PREVIOUS_OFFSET)?;
    if current == 0 || previous == 0 || current % 4 != 0 || previous % 4 != 0 {
        return Err(AttemptFailure::Unstable(format!(
            "{} cached magnitude pointers are invalid",
            profile.kind
        )));
    }
    let current_a = read_attempt(
        process,
        u64::from(current),
        LOGICAL_ACTION_COUNT * size_of::<f32>(),
    )?;
    let previous_a = read_attempt(
        process,
        u64::from(previous),
        LOGICAL_ACTION_COUNT * size_of::<f32>(),
    )?;
    let platform_controller_id_a = if profile.gamepad {
        Some(read_i32_attempt(
            process,
            checked_add_attempt(object_address, GAMEPAD_PLATFORM_ID_OFFSET)?,
        )?)
    } else {
        None
    };
    let core_b = read_attempt(process, object_address, DEVICE_CORE_SIZE)?;
    let current_b = read_attempt(
        process,
        u64::from(current),
        LOGICAL_ACTION_COUNT * size_of::<f32>(),
    )?;
    let previous_b = read_attempt(
        process,
        u64::from(previous),
        LOGICAL_ACTION_COUNT * size_of::<f32>(),
    )?;
    if core_a != core_b || current_a != current_b || previous_a != previous_b {
        return Err(AttemptFailure::Unstable(format!(
            "{} object or cached magnitudes changed during collection",
            profile.kind
        )));
    }
    let platform_controller_id_b = if profile.gamepad {
        Some(read_i32_attempt(
            process,
            checked_add_attempt(object_address, GAMEPAD_PLATFORM_ID_OFFSET)?,
        )?)
    } else {
        None
    };
    if platform_controller_id_a != platform_controller_id_b {
        return Err(AttemptFailure::Unstable(
            "gamepad platform controller id changed during collection".to_owned(),
        ));
    }
    let cached_current = parse_magnitudes(&current_a, profile.kind)?;
    let cached_previous = parse_magnitudes(&previous_a, profile.kind)?;
    let threshold = f32::from_bits(read_u32_from(&core_a, DEVICE_THRESHOLD_OFFSET)?);
    if !threshold.is_finite() || !(0.0..=1.0).contains(&threshold) {
        return Err(AttemptFailure::Unstable(format!(
            "{} threshold is outside the finite 0..1 range",
            profile.kind
        )));
    }
    Ok(InputRpmDevice {
        object: address(object_address),
        reference_owner: address(u64::from(reference_owner)),
        kind: profile.kind.to_owned(),
        device_key,
        enabled_byte: core_a[8],
        binding_count,
        cached_current,
        cached_previous,
        threshold,
        platform_controller_id: platform_controller_id_a,
    })
}

fn ownership_joins(
    devices: &[InputRpmDevice],
    players: &[InputRpmPlayer],
) -> Vec<InputRpmOwnershipJoin> {
    devices
        .iter()
        .map(|device| {
            let count = players
                .iter()
                .filter(|player| player.device_key == device.device_key)
                .count();
            InputRpmOwnershipJoin {
                device_key: device.device_key,
                device_kind: device.kind.clone(),
                result: if device.device_key < 0 || count == 0 {
                    "UNKNOWN_NO_PLAYER_MATCH"
                } else if count == 1 {
                    "UNIQUE_OWNER_MATCH"
                } else {
                    "UNKNOWN_AMBIGUOUS_PLAYER_MATCH"
                }
                .to_owned(),
                matching_player_count: count,
            }
        })
        .collect()
}

fn read_vector_header(
    process: &ObservedProcess,
    address: u64,
    element_size: usize,
    maximum_count: usize,
) -> Result<VectorHeader, AttemptFailure> {
    let bytes = read_attempt(process, address, 12)?;
    let header = VectorHeader {
        begin: read_u32_from(&bytes, 0)?,
        end: read_u32_from(&bytes, 4)?,
        capacity_end: read_u32_from(&bytes, 8)?,
    };
    validated_count(
        header.begin,
        header.end,
        header.capacity_end,
        element_size,
        maximum_count,
    )
    .map_err(AttemptFailure::Unstable)?;
    Ok(header)
}

fn validated_count(
    begin: u32,
    end: u32,
    capacity_end: u32,
    element_size: usize,
    maximum_count: usize,
) -> Result<usize, String> {
    if element_size == 0 {
        return Err("vector element size is zero".to_owned());
    }
    if begin == 0 && end == 0 && capacity_end == 0 {
        return Ok(0);
    }
    let alignment = u32::try_from(element_size.min(4)).unwrap();
    if begin == 0
        || begin > end
        || end > capacity_end
        || !begin.is_multiple_of(alignment)
        || !end.is_multiple_of(alignment)
        || !capacity_end.is_multiple_of(alignment)
    {
        return Err("vector header is unordered, null, or misaligned".to_owned());
    }
    let bytes = usize::try_from(end - begin).map_err(|_| "vector span overflow".to_owned())?;
    if !bytes.is_multiple_of(element_size) {
        return Err("vector span is not an exact element multiple".to_owned());
    }
    let count = bytes / element_size;
    if count > maximum_count {
        return Err(format!(
            "vector count {count} exceeds diagnostic bound {maximum_count}"
        ));
    }
    Ok(count)
}

fn read_active_span(
    process: &ObservedProcess,
    header: VectorHeader,
    element_size: usize,
) -> Result<Vec<u8>, AttemptFailure> {
    let count = validated_count(
        header.begin,
        header.end,
        header.capacity_end,
        element_size,
        usize::MAX,
    )
    .map_err(AttemptFailure::Unstable)?;
    if count == 0 {
        Ok(Vec::new())
    } else {
        read_attempt(
            process,
            u64::from(header.begin),
            count.saturating_mul(element_size),
        )
    }
}

fn parse_magnitudes(bytes: &[u8], kind: &str) -> Result<Vec<f32>, AttemptFailure> {
    let mut values = Vec::with_capacity(LOGICAL_ACTION_COUNT);
    for raw in bytes.chunks_exact(4) {
        let value = f32::from_le_bytes(raw.try_into().unwrap());
        if !value.is_finite() || !(-0.001..=1.001).contains(&value) {
            return Err(AttemptFailure::Unstable(format!(
                "{kind} cached logical magnitude is outside the finite 0..1 range"
            )));
        }
        values.push(value);
    }
    Ok(values)
}

fn read_attempt(
    process: &ObservedProcess,
    address: u64,
    size: usize,
) -> Result<Vec<u8>, AttemptFailure> {
    process
        .read_memory(address, size)
        .map_err(|error| AttemptFailure::Read(error.message))
}

fn read_u32_attempt(process: &ObservedProcess, address: u64) -> Result<u32, AttemptFailure> {
    let bytes = read_attempt(process, address, 4)?;
    read_u32_from(&bytes, 0)
}

fn read_i32_attempt(process: &ObservedProcess, address: u64) -> Result<i32, AttemptFailure> {
    Ok(i32::from_le_bytes(
        read_attempt(process, address, 4)?
            .try_into()
            .map_err(|_| AttemptFailure::Read("short i32 read".to_owned()))?,
    ))
}

fn read_u32(process: &ObservedProcess, address: u64) -> AppResult<u32> {
    let bytes = process.read_memory(address, 4)?;
    Ok(u32::from_le_bytes(bytes.try_into().unwrap()))
}

fn read_u32_from(bytes: &[u8], offset: usize) -> Result<u32, AttemptFailure> {
    let end = offset
        .checked_add(4)
        .ok_or_else(|| AttemptFailure::Read("u32 offset overflow".to_owned()))?;
    let raw = bytes
        .get(offset..end)
        .ok_or_else(|| AttemptFailure::Read("short structure read".to_owned()))?;
    Ok(u32::from_le_bytes(raw.try_into().unwrap()))
}

fn read_i32_from(bytes: &[u8], offset: usize) -> Result<i32, AttemptFailure> {
    Ok(i32::from_le_bytes(
        read_u32_from(bytes, offset)?.to_le_bytes(),
    ))
}

fn checked_add(base: u64, offset: u64) -> AppResult<u64> {
    base.checked_add(offset)
        .ok_or_else(|| AppError::evidence("runtime address overflow"))
}

fn checked_add_attempt(base: u64, offset: u64) -> Result<u64, AttemptFailure> {
    base.checked_add(offset)
        .ok_or_else(|| AttemptFailure::Read("runtime address overflow".to_owned()))
}

fn expect_address(actual: u32, expected: u64, kind: &str, field: &str) -> AppResult<()> {
    if u64::from(actual) != expected {
        Err(AppError::evidence(format!(
            "{kind} {field} mismatch: expected {}, got 0x{actual:08X}",
            address(expected)
        )))
    } else {
        Ok(())
    }
}

fn address(value: u64) -> String {
    format!("0x{value:08X}")
}

fn new_summary(
    pid: u32,
    output_directory: Option<PathBuf>,
    requested_samples: u64,
) -> InputRpmSummary {
    InputRpmSummary {
        schema_version: RPM_SCHEMA_VERSION,
        evidence_class: RPM_EVIDENCE_CLASS.to_owned(),
        result: "IN_PROGRESS".to_owned(),
        output_directory,
        process_id: pid,
        requested_samples,
        coherent_samples: 0,
        unstable_drops: 0,
        read_error_drops: 0,
        ownership_results: BTreeMap::new(),
        active_input_allowed: false,
        claim: DIAGNOSTIC_CLAIM.to_owned(),
    }
}

fn incorporate(summary: &mut InputRpmSummary, event: &InputRpmEvent) {
    match event.status.as_str() {
        "COHERENT_BEST_EFFORT" => summary.coherent_samples += 1,
        "DROPPED_UNSTABLE" => summary.unstable_drops += 1,
        _ => summary.read_error_drops += 1,
    }
    for join in &event.ownership {
        *summary
            .ownership_results
            .entry(format!("{}:{}", join.device_kind, join.result))
            .or_default() += 1;
    }
}

fn event_shape_valid(event: &InputRpmEvent) -> bool {
    if event.retry_count > DEFAULT_RETRIES {
        return false;
    }
    if event.status != "COHERENT_BEST_EFFORT" {
        return matches!(
            event.status.as_str(),
            "DROPPED_UNSTABLE" | "DROPPED_READ_ERROR"
        ) && event
            .reason
            .as_deref()
            .is_some_and(|reason| !reason.is_empty())
            && event.devices.is_empty()
            && event.players.is_empty()
            && event.ownership.is_empty();
    }
    if event.reason.is_some() {
        return false;
    }
    if event.devices.iter().any(|device| {
        !matches!(device.kind.as_str(), "KEYBOARD" | "GAMEPAD")
            || device.binding_count < LOGICAL_ACTION_COUNT
            || device.binding_count > MAX_BINDING_COUNT
            || device.cached_current.len() != LOGICAL_ACTION_COUNT
            || device.cached_previous.len() != LOGICAL_ACTION_COUNT
            || device
                .cached_current
                .iter()
                .chain(&device.cached_previous)
                .any(|value| !value.is_finite() || !(-0.001..=1.001).contains(value))
            || !device.threshold.is_finite()
            || !(0.0..=1.0).contains(&device.threshold)
            || !valid_address_text(&device.object)
            || !valid_address_text(&device.reference_owner)
    }) {
        return false;
    }
    if event
        .players
        .iter()
        .any(|player| !valid_address_text(&player.object))
    {
        return false;
    }
    event.ownership == ownership_joins(&event.devices, &event.players)
}

fn valid_address_text(value: &str) -> bool {
    value.len() == 10
        && value.starts_with("0x")
        && value[2..].bytes().all(|byte| byte.is_ascii_hexdigit())
}

fn parse_address_text(value: &str) -> Result<u64, AttemptFailure> {
    if !valid_address_text(value) {
        return Err(AttemptFailure::Read(
            "internally formatted address is invalid".to_owned(),
        ));
    }
    u64::from_str_radix(&value[2..], 16)
        .map_err(|_| AttemptFailure::Read("internally formatted address overflow".to_owned()))
}

fn rpm_check(id: &str, passed: bool, detail: &str) -> InputRpmCheck {
    InputRpmCheck {
        id: id.to_owned(),
        passed,
        detail: detail.to_owned(),
    }
}

fn write_new_pretty(path: &Path, value: &impl Serialize) -> AppResult<()> {
    let file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .usage_context(format!("cannot create {}", path.display()))?;
    serde_json::to_writer_pretty(BufWriter::new(file), value)
        .usage_context(format!("cannot serialize {}", path.display()))
}

fn replace_pretty(path: &Path, value: &impl Serialize) -> AppResult<()> {
    let file = OpenOptions::new()
        .write(true)
        .truncate(true)
        .open(path)
        .usage_context(format!("cannot reopen {}", path.display()))?;
    serde_json::to_writer_pretty(BufWriter::new(file), value)
        .usage_context(format!("cannot serialize {}", path.display()))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn vector_validation_rejects_capacity_tail_and_bad_alignment() {
        assert_eq!(validated_count(0x1000, 0x1010, 0x1020, 8, 4).unwrap(), 2);
        assert!(validated_count(0x1000, 0x1028, 0x1020, 8, 4).is_err());
        assert!(validated_count(0x1000, 0x1004, 0x1020, 8, 4).is_err());
        assert!(validated_count(0x1002, 0x1010, 0x1020, 8, 4).is_err());
    }

    #[test]
    fn original_online_positive_screen_requires_two_to_four_active_peers() {
        let valid = ActiveNegativeScreen {
            game_root: 0x1000,
            run_active: true,
            manager: 0x2000,
            network_backend: 0x3000,
            roster_first: 0x4000,
            roster_last: 0x4008,
            roster_capacity: 0x4010,
            online_frame: 7,
        };
        validate_original_online_screen(valid).unwrap();

        let mut invalid = valid;
        invalid.network_backend = 0;
        assert!(validate_original_online_screen(invalid).is_err());

        let mut invalid = valid;
        invalid.roster_last = invalid.roster_first + 4;
        assert!(validate_original_online_screen(invalid).is_err());

        let mut invalid = valid;
        invalid.run_active = false;
        assert!(validate_original_online_screen(invalid).is_err());

        let mut invalid = valid;
        invalid.online_frame = u32::MAX;
        assert!(validate_original_online_screen(invalid).is_err());
    }

    #[test]
    fn original_online_snapshot_allows_frame_progress_but_rejects_regression_or_identity_drift() {
        let before = ActiveNegativeScreen {
            game_root: 0x1000,
            run_active: true,
            manager: 0x2000,
            network_backend: 0x3000,
            roster_first: 0x4000,
            roster_last: 0x4008,
            roster_capacity: 0x4010,
            online_frame: 7,
        };
        let mut after = before;
        after.online_frame = 8;
        assert!(original_online_screens_stable(before, after));

        after.online_frame = 6;
        assert!(!original_online_screens_stable(before, after));

        after = before;
        after.manager = 0x5000;
        assert!(!original_online_screens_stable(before, after));
    }

    #[test]
    fn ownership_join_is_unique_or_unknown() {
        let device = InputRpmDevice {
            object: "0x1".to_owned(),
            reference_owner: "0x2".to_owned(),
            kind: "KEYBOARD".to_owned(),
            device_key: 7,
            enabled_byte: 2,
            binding_count: 12,
            cached_current: vec![0.0; 12],
            cached_previous: vec![0.0; 12],
            threshold: 0.5,
            platform_controller_id: None,
        };
        let unique = ownership_joins(
            std::slice::from_ref(&device),
            &[InputRpmPlayer {
                object: "0x3".to_owned(),
                device_key: 7,
            }],
        );
        assert_eq!(unique[0].result, "UNIQUE_OWNER_MATCH");
        let ambiguous = ownership_joins(
            &[device],
            &[
                InputRpmPlayer {
                    object: "0x3".to_owned(),
                    device_key: 7,
                },
                InputRpmPlayer {
                    object: "0x4".to_owned(),
                    device_key: 7,
                },
            ],
        );
        assert_eq!(ambiguous[0].result, "UNKNOWN_AMBIGUOUS_PLAYER_MATCH");
    }

    #[test]
    fn rpm_manifest_cannot_be_gate_eligible() {
        let mut summary = new_summary(1, None, 1);
        summary.result = "PASS_DIAGNOSTIC".to_owned();
        assert!(!summary.active_input_allowed);
        assert_eq!(RPM_EVIDENCE_CLASS, "RPM_DIAGNOSTIC_ONLY");
    }

    #[test]
    fn analyzer_replays_coherent_diagnostic_and_rejects_drops_zero_qpc_and_empty_events() {
        let temporary = tempfile::tempdir().unwrap();
        let run = temporary.path().join("run");
        fs::create_dir(&run).unwrap();
        let manifest = InputRpmManifest {
            schema_version: RPM_SCHEMA_VERSION,
            evidence_class: RPM_EVIDENCE_CLASS.to_owned(),
            tool_identity: "IsaacCsT0-InputRpmObserver-v2".to_owned(),
            started_utc: "2026-08-20T00:00:00.000Z".to_owned(),
            ended_utc: Some("2026-08-20T00:00:01.000Z".to_owned()),
            target_sha256: FIXED_TARGET_SHA256.to_owned(),
            process_id: 7,
            process_creation_utc: None,
            module_base: "0x003B0000".to_owned(),
            module_size: 9_617_408,
            debugger_present: true,
            observer_access_bits: format!("0x{OBSERVER_ACCESS_BITS:08X}"),
            interval_milliseconds: 10,
            requested_duration_milliseconds: 10,
            qpc_frequency: 10_000_000,
            guarantees: vec!["read only".to_owned()],
            limitations: vec!["best-effort is not gapless".to_owned()],
            eligible_for_active_gate: false,
        };
        let device = InputRpmDevice {
            object: "0x00001000".to_owned(),
            reference_owner: "0x00002000".to_owned(),
            kind: "KEYBOARD".to_owned(),
            device_key: 0,
            enabled_byte: 2,
            binding_count: 32,
            cached_current: vec![0.0; LOGICAL_ACTION_COUNT],
            cached_previous: vec![0.0; LOGICAL_ACTION_COUNT],
            threshold: 0.3,
            platform_controller_id: None,
        };
        let event = InputRpmEvent {
            observer_sequence: 1,
            qpc: 100,
            status: "COHERENT_BEST_EFFORT".to_owned(),
            retry_count: 0,
            reason: None,
            devices: vec![device],
            players: vec![InputRpmPlayer {
                object: "0x00003000".to_owned(),
                device_key: 0,
            }],
            ownership: vec![InputRpmOwnershipJoin {
                device_key: 0,
                device_kind: "KEYBOARD".to_owned(),
                result: "UNIQUE_OWNER_MATCH".to_owned(),
                matching_player_count: 1,
            }],
        };
        let mut summary = new_summary(7, Some(run.clone()), 1);
        incorporate(&mut summary, &event);
        summary.result = "PASS_DIAGNOSTIC".to_owned();
        fs::write(
            run.join("rpm-manifest.json"),
            serde_json::to_vec(&manifest).unwrap(),
        )
        .unwrap();
        fs::write(
            run.join("rpm-events.jsonl"),
            format!("{}\n", serde_json::to_string(&event).unwrap()),
        )
        .unwrap();
        fs::write(
            run.join("rpm-summary.json"),
            serde_json::to_vec(&summary).unwrap(),
        )
        .unwrap();
        assert_eq!(analyze(&run).unwrap().result, "PASS_DIAGNOSTIC");

        let dropped = InputRpmEvent {
            observer_sequence: 1,
            qpc: 101,
            status: "DROPPED_UNSTABLE".to_owned(),
            retry_count: DEFAULT_RETRIES,
            reason: Some("concurrent change".to_owned()),
            devices: Vec::new(),
            players: Vec::new(),
            ownership: Vec::new(),
        };
        let mut blocked_summary = new_summary(7, Some(run.clone()), 1);
        incorporate(&mut blocked_summary, &dropped);
        blocked_summary.result = "BLOCKED_BY_UNSTABLE_SNAPSHOT".to_owned();
        fs::write(
            run.join("rpm-events.jsonl"),
            format!("{}\n", serde_json::to_string(&dropped).unwrap()),
        )
        .unwrap();
        fs::write(
            run.join("rpm-summary.json"),
            serde_json::to_vec(&blocked_summary).unwrap(),
        )
        .unwrap();
        let all_dropped = analyze(&run).unwrap();
        assert_eq!(all_dropped.result, "FAIL");
        assert!(
            all_dropped
                .checks
                .iter()
                .any(|check| { check.id == "summary_replay" && check.passed })
        );
        assert!(
            all_dropped
                .checks
                .iter()
                .any(|check| { check.id == "coherent_samples" && !check.passed })
        );

        let mut zero_qpc = event.clone();
        zero_qpc.qpc = 0;
        fs::write(
            run.join("rpm-events.jsonl"),
            format!("{}\n", serde_json::to_string(&zero_qpc).unwrap()),
        )
        .unwrap();
        fs::write(
            run.join("rpm-summary.json"),
            serde_json::to_vec(&summary).unwrap(),
        )
        .unwrap();
        let zero_qpc_analysis = analyze(&run).unwrap();
        assert_eq!(zero_qpc_analysis.result, "FAIL");
        assert!(
            zero_qpc_analysis
                .checks
                .iter()
                .any(|check| { check.id == "qpc_monotonic" && !check.passed })
        );

        fs::write(run.join("rpm-events.jsonl"), "").unwrap();
        assert_eq!(analyze(&run).unwrap().result, "FAIL");
    }
}
