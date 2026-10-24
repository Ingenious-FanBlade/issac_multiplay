use std::collections::{BTreeMap, BTreeSet, HashMap};
use std::fs::{self, File, OpenOptions};
use std::io::{BufRead, BufReader, BufWriter, Write};
use std::path::{Path, PathBuf};

use chrono::Utc;
use serde::{Deserialize, Serialize};

use crate::MODE_TOOL_IDENTITY;
use crate::TOOL_VERSION;
use crate::artifact::{
    AnalyzerCheck, GameManifest, LiveProcessSnapshot, ProcessManifest, TargetBytesDocument,
    ToolManifest, file_sha256, utc_now,
};
use crate::error::{AppError, AppResult, UsageContext};
use crate::mode_ipc::{
    EVENT_FLAG_FRAME_REGRESSED, EVENT_FLAG_LUA_CLOSED, EVENT_FLAG_MANAGER_CHANGED,
    EVENT_FLAG_MENU_RETURN, EVENT_FLAG_NONZERO_CHANNEL, EVENT_FLAG_PAYLOAD_HASH_AFTER,
    EVENT_FLAG_RUN_ACTIVE_AFTER_ZERO, EVENT_FLAG_RUN_ACTIVE_BEFORE, EVENT_FLAG_WRONG_THREAD,
    NativeEventKind, NativeEventRecord,
};
use crate::mode_ledger::ModeInspectionReport;
use crate::pe::sha256_hex;

pub const MODE_ARTIFACT_SCHEMA_VERSION: u32 = 2;

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ModeRunManifest {
    pub schema_version: u32,
    pub run_id: String,
    pub case_id: String,
    pub experiment_identity: String,
    pub status: String,
    pub started_utc: String,
    pub ended_utc: Option<String>,
    pub end_reason: Option<String>,
    pub scenario: String,
    pub endpoint: String,
    pub expected_mode: String,
    pub game: GameManifest,
    pub tool: ToolManifest,
    pub baseline_ledger_id: String,
    pub baseline_ledger_sha256: String,
    pub mode_ledger_id: String,
    pub mode_ledger_sha256: String,
    pub process: Option<ProcessManifest>,
    pub native_dll_sha256: Option<String>,
    pub helper_sha256: Option<String>,
    pub installed_hooks: Vec<String>,
    pub native_probe_installed: bool,
    pub safe_unload: String,
    pub pairing_key_stored: bool,
    pub local_peer_token: Option<String>,
    pub artifact_hashes: BTreeMap<String, String>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ModeEvent {
    pub schema_version: u32,
    pub event_sequence: u64,
    pub source: String,
    pub native_sequence: Option<u64>,
    pub qpc: i64,
    pub thread_id: u32,
    pub endpoint: String,
    pub network_mode: String,
    pub generation: u64,
    pub attempt_id: u64,
    pub event_kind: String,
    pub result: String,
    pub result_value: Option<i32>,
    #[serde(default)]
    pub lifecycle_argument: Option<u32>,
    pub original_calls: Option<u32>,
    pub online_before: Option<u32>,
    pub online_after: Option<u32>,
    pub channel: Option<i32>,
    pub send_mode: Option<i32>,
    pub payload_size: Option<u32>,
    pub output_size: Option<u32>,
    #[serde(default)]
    pub active_before: Option<u32>,
    #[serde(default)]
    pub active_after: Option<u32>,
    pub peer_token: Option<String>,
    pub payload_hash: Option<String>,
    pub payload_hash_after: Option<String>,
    pub flags: u32,
    pub signal_mask: u32,
    pub error_code: Option<u32>,
    pub detail: String,
}

impl ModeEvent {
    #[must_use]
    pub fn controller(
        qpc: i64,
        endpoint: &str,
        kind: &str,
        result: &str,
        detail: impl Into<String>,
    ) -> Self {
        Self {
            schema_version: MODE_ARTIFACT_SCHEMA_VERSION,
            event_sequence: 0,
            source: "controller".to_owned(),
            native_sequence: None,
            qpc,
            thread_id: 0,
            endpoint: endpoint.to_owned(),
            network_mode: "Unresolved".to_owned(),
            generation: 0,
            attempt_id: 0,
            event_kind: kind.to_owned(),
            result: result.to_owned(),
            result_value: None,
            lifecycle_argument: None,
            original_calls: None,
            online_before: None,
            online_after: None,
            channel: None,
            send_mode: None,
            payload_size: None,
            output_size: None,
            active_before: None,
            active_after: None,
            peer_token: None,
            payload_hash: None,
            payload_hash_after: None,
            flags: 0,
            signal_mask: 0,
            error_code: None,
            detail: detail.into(),
        }
    }

    pub fn from_native(record: NativeEventRecord, endpoint: &str) -> AppResult<Self> {
        let raw = record.event;
        let kind = NativeEventKind::from_raw(raw.kind)?;
        let lifecycle = matches!(
            kind,
            NativeEventKind::PostGameStart | NativeEventKind::RunTeardown
        );
        let payload_hash = (raw.payload_size != 0).then(|| hex::encode_upper(raw.payload_hash));
        let payload_hash_after = (raw.flags & EVENT_FLAG_PAYLOAD_HASH_AFTER != 0)
            .then(|| hex::encode_upper(raw.payload_hash_after));
        Ok(Self {
            schema_version: MODE_ARTIFACT_SCHEMA_VERSION,
            event_sequence: 0,
            source: "native".to_owned(),
            native_sequence: Some(record.sequence),
            qpc: raw.qpc,
            thread_id: raw.thread_id,
            endpoint: endpoint.to_owned(),
            network_mode: mode_name(raw.mode)?.to_owned(),
            generation: raw.generation,
            attempt_id: raw.attempt_id,
            event_kind: serde_json::to_value(kind)
                .ok()
                .and_then(|value| value.as_str().map(str::to_owned))
                .unwrap_or_else(|| "unknown".to_owned()),
            result: if raw.error_code == 0 { "PASS" } else { "FAIL" }.to_owned(),
            result_value: (!lifecycle).then_some(raw.result_value),
            lifecycle_argument: lifecycle.then_some(raw.result_value as u32),
            original_calls: Some(raw.original_calls),
            online_before: (raw.online_before != u32::MAX).then_some(raw.online_before),
            online_after: (raw.online_after != u32::MAX).then_some(raw.online_after),
            channel: (raw.channel >= 0).then_some(raw.channel),
            send_mode: (raw.send_mode >= 0).then_some(raw.send_mode),
            payload_size: (raw.payload_size != 0).then_some(raw.payload_size),
            output_size: (raw.output_size != u32::MAX).then_some(raw.output_size),
            active_before: (raw.active_before != u32::MAX).then_some(raw.active_before),
            active_after: (raw.active_after != u32::MAX).then_some(raw.active_after),
            peer_token: (raw.peer_token != 0).then(|| format!("peer:{:016X}", raw.peer_token)),
            payload_hash,
            payload_hash_after,
            flags: raw.flags,
            signal_mask: raw.signal_mask,
            error_code: (raw.error_code != 0).then_some(raw.error_code),
            detail: native_detail(kind),
        })
    }
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ModeAnalyzerReport {
    pub schema_version: u32,
    pub run_id: String,
    pub case_id: String,
    pub result: String,
    pub evidence_label: String,
    pub checks: Vec<AnalyzerCheck>,
    pub first_divergence: Option<String>,
    pub event_counts: BTreeMap<String, u64>,
    pub generated_utc: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ModeComparisonReport {
    pub schema_version: u32,
    pub analysis_revision: String,
    pub result: String,
    pub control_run_id: String,
    pub instrumented_run_id: String,
    pub peer_run_id: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub packet_pairing: Option<DualEndpointPacketAudit>,
    pub checks: Vec<AnalyzerCheck>,
    pub first_divergence: Option<String>,
    pub generated_utc: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct DualEndpointPacketAudit {
    pub rule_version: String,
    pub a_to_b: PacketDirectionAudit,
    pub b_to_a: PacketDirectionAudit,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct PacketDirectionAudit {
    pub matched_packets: u64,
    pub send_modes: Vec<i32>,
    pub first_matched_send_sequence: Option<u64>,
    pub last_matched_send_sequence: Option<u64>,
    pub first_matched_read_sequence: Option<u64>,
    pub last_matched_read_sequence: Option<u64>,
    pub edge_reads_excluded: u64,
    pub unmatched_reads_inside: u64,
    pub edge_sends_excluded: u64,
    pub unreliable_missing_sends_inside: u64,
    pub reliable_missing_sends_inside: u64,
    pub invalid_or_missing_send_modes: u64,
}

impl PacketDirectionAudit {
    fn passed(&self) -> bool {
        self.matched_packets >= 2
            && self.unmatched_reads_inside == 0
            && self.reliable_missing_sends_inside == 0
            && self.invalid_or_missing_send_modes == 0
    }
}

pub struct ModeArtifactRun {
    pub directory: PathBuf,
    pub manifest: ModeRunManifest,
    writer: ModeEventWriter,
}

struct ModeEventWriter {
    writer: BufWriter<File>,
    next_sequence: u64,
}

impl ModeArtifactRun {
    #[allow(clippy::too_many_arguments)]
    pub fn create(
        artifacts_root: &Path,
        case_id: &str,
        scenario: &str,
        endpoint: &str,
        expected_mode: &str,
        inspection: &ModeInspectionReport,
        tool_sha256: String,
        native_dll: Option<&Path>,
        helper: Option<&Path>,
    ) -> AppResult<Self> {
        validate_component(case_id, "case-id")?;
        validate_component(endpoint, "endpoint")?;
        fs::create_dir_all(artifacts_root).usage_context(format!(
            "cannot create artifacts root {}",
            artifacts_root.display()
        ))?;
        let run_id = format!(
            "{}-t0-02-{}-{}",
            Utc::now().format("%Y%m%dT%H%M%SZ"),
            case_id,
            endpoint
        );
        let directory = artifacts_root.join(&run_id);
        fs::create_dir(&directory).usage_context(format!(
            "cannot create new mode artifact directory {}",
            directory.display()
        ))?;
        let writer = ModeEventWriter::create(&directory.join("endpoint.jsonl"))?;
        let baseline = &inspection.baseline;
        let manifest = ModeRunManifest {
            schema_version: MODE_ARTIFACT_SCHEMA_VERSION,
            run_id,
            case_id: case_id.to_owned(),
            experiment_identity: MODE_TOOL_IDENTITY.to_owned(),
            status: "RUNNING".to_owned(),
            started_utc: utc_now(),
            ended_utc: None,
            end_reason: None,
            scenario: scenario.to_owned(),
            endpoint: endpoint.to_owned(),
            expected_mode: expected_mode.to_owned(),
            game: GameManifest {
                file_name: baseline.image.file_name.clone(),
                machine: baseline.image.machine.clone(),
                timestamp: baseline.image.timestamp.clone(),
                image_size: baseline.image.image_size.clone(),
                file_sha256: baseline.image.file_sha256.clone(),
                text_sha256: baseline.image.text_sha256.clone(),
                variant_id: baseline.variant_id.clone(),
                static_patch_state: baseline.static_patch_state.clone(),
            },
            tool: ToolManifest {
                identity: MODE_TOOL_IDENTITY.to_owned(),
                version: TOOL_VERSION.to_owned(),
                executable_sha256: tool_sha256,
            },
            baseline_ledger_id: baseline.ledger_id.clone(),
            baseline_ledger_sha256: baseline.ledger_sha256.clone(),
            mode_ledger_id: inspection.ledger_id.clone(),
            mode_ledger_sha256: inspection.ledger_sha256.clone(),
            process: None,
            native_dll_sha256: native_dll.map(file_sha256).transpose()?,
            helper_sha256: helper.map(file_sha256).transpose()?,
            installed_hooks: Vec::new(),
            native_probe_installed: false,
            safe_unload: if native_dll.is_some() {
                "PENDING".to_owned()
            } else {
                "NOT_APPLICABLE_CONTROL".to_owned()
            },
            pairing_key_stored: false,
            local_peer_token: None,
            artifact_hashes: BTreeMap::new(),
        };
        let run = Self {
            directory,
            manifest,
            writer,
        };
        run.write_manifest()?;
        Ok(run)
    }

    pub fn write_event(&mut self, event: ModeEvent) -> AppResult<()> {
        self.writer.write(event)
    }

    pub fn set_process(
        &mut self,
        snapshot: &LiveProcessSnapshot,
        creation_time_utc: Option<String>,
    ) {
        self.manifest.process = Some(ProcessManifest {
            pid: snapshot.pid,
            creation_time_100ns: snapshot.creation_time_100ns,
            role: self.manifest.endpoint.clone(),
            module_base: snapshot.module_base.clone(),
            creation_time_utc,
        });
    }

    pub fn set_native_ready(&mut self, hooks_mask: u32) {
        self.manifest.native_probe_installed = true;
        self.manifest.installed_hooks = hook_names(hooks_mask);
    }

    pub fn set_local_peer_token(&mut self, token: u64) {
        self.manifest.local_peer_token = (token != 0).then(|| format!("peer:{token:016X}"));
    }

    pub fn set_safe_unload(&mut self, value: &str) {
        self.manifest.safe_unload = value.to_owned();
    }

    pub fn write_target_document(
        &self,
        name: &str,
        document: &TargetBytesDocument,
    ) -> AppResult<()> {
        write_pretty_json(&self.directory.join(name), document)
    }

    pub fn finish(&mut self, status: &str, reason: impl Into<String>) -> AppResult<()> {
        self.writer.flush()?;
        self.manifest.status = status.to_owned();
        self.manifest.ended_utc = Some(utc_now());
        self.manifest.end_reason = Some(reason.into());
        for name in [
            "endpoint.jsonl",
            "target-bytes-before.json",
            "target-bytes-after.json",
        ] {
            let path = self.directory.join(name);
            if path.is_file() {
                let bytes = fs::read(&path)
                    .usage_context(format!("cannot hash artifact {}", path.display()))?;
                self.manifest
                    .artifact_hashes
                    .insert(name.to_owned(), sha256_hex(&bytes));
            }
        }
        self.write_manifest()
    }

    pub fn write_manifest(&self) -> AppResult<()> {
        write_pretty_json(&self.directory.join("manifest.json"), &self.manifest)
    }
}

impl ModeEventWriter {
    fn create(path: &Path) -> AppResult<Self> {
        let file = OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(path)
            .usage_context(format!("cannot create {}", path.display()))?;
        Ok(Self {
            writer: BufWriter::new(file),
            next_sequence: 1,
        })
    }

    fn write(&mut self, mut event: ModeEvent) -> AppResult<()> {
        event.event_sequence = self.next_sequence;
        serde_json::to_writer(&mut self.writer, &event)
            .usage_context("cannot serialize mode event")?;
        self.writer
            .write_all(b"\n")
            .usage_context("cannot write mode event")?;
        self.writer
            .flush()
            .usage_context("cannot flush mode event")?;
        self.next_sequence = self
            .next_sequence
            .checked_add(1)
            .ok_or_else(|| AppError::evidence("mode event sequence overflow"))?;
        Ok(())
    }

    fn flush(&mut self) -> AppResult<()> {
        self.writer
            .flush()
            .usage_context("cannot flush mode event log")
    }
}

pub fn analyze_mode_run(run_directory: &Path) -> AppResult<ModeAnalyzerReport> {
    let manifest: ModeRunManifest = read_json(&run_directory.join("manifest.json"))?;
    if manifest.schema_version != MODE_ARTIFACT_SCHEMA_VERSION
        || manifest.experiment_identity != MODE_TOOL_IDENTITY
    {
        return Err(AppError::usage("not a supported T0-02 mode artifact"));
    }
    if run_directory.file_name().and_then(|name| name.to_str()) != Some(&manifest.run_id) {
        return Err(AppError::usage(
            "mode artifact directory name does not equal manifest runId",
        ));
    }
    let events = read_events(&run_directory.join("endpoint.jsonl"))?;
    let mut checks = Vec::new();
    let mut first_divergence = None;
    let mut push = |id: &str, passed: bool, detail: String| {
        if !passed && first_divergence.is_none() {
            first_divergence = Some(id.to_owned());
        }
        checks.push(AnalyzerCheck {
            id: id.to_owned(),
            result: if passed { "PASS" } else { "FAIL" }.to_owned(),
            detail,
        });
    };

    let contiguous = events
        .iter()
        .enumerate()
        .all(|(index, event)| event.event_sequence == index as u64 + 1);
    push(
        "endpoint-event-sequence",
        contiguous && !events.is_empty(),
        format!("events={}, contiguous={contiguous}", events.len()),
    );
    let native_sequences = events
        .iter()
        .filter_map(|event| event.native_sequence)
        .collect::<Vec<_>>();
    let native_contiguous = native_sequences
        .iter()
        .enumerate()
        .all(|(index, sequence)| *sequence == index as u64 + 1);
    let needs_native = manifest.scenario != "control-no-native";
    push(
        "native-event-sequence",
        (!needs_native && native_sequences.is_empty())
            || (needs_native && !native_sequences.is_empty() && native_contiguous),
        format!(
            "nativeEvents={}, contiguous={native_contiguous}",
            native_sequences.len()
        ),
    );

    let forbidden = events
        .iter()
        .filter(|event| {
            matches!(
                event.event_kind.as_str(),
                "cs_session"
                    | "cs_send"
                    | "cs_read"
                    | "cs_drain"
                    | "cs_input_queue"
                    | "cs_state_hash"
            )
        })
        .count();
    push(
        "custom-cs-events-zero",
        forbidden == 0,
        format!("forbidden C/S events={forbidden}"),
    );

    let wrapper_kinds = [
        "lua_simulation_pcall",
        "lua_close",
        "frame",
        "dispatch",
        "steam_send",
        "steam_available",
        "steam_read",
        "post_game_start",
        "run_teardown",
    ];
    let wrapper_bad = events
        .iter()
        .filter(|event| wrapper_kinds.contains(&event.event_kind.as_str()))
        .filter(|event| event.original_calls != Some(1))
        .count();
    push(
        "passthrough-original-called-once",
        !needs_native || wrapper_bad == 0,
        format!("wrapper events with originalCalls!=1: {wrapper_bad}"),
    );

    let start_events = events
        .iter()
        .filter(|event| event.event_kind == "post_game_start")
        .collect::<Vec<_>>();
    let teardown_events = events
        .iter()
        .filter(|event| event.event_kind == "run_teardown")
        .collect::<Vec<_>>();
    let start_bad = start_events
        .iter()
        .filter(|event| {
            event.attempt_id == 0
                || event.lifecycle_argument.is_none_or(|argument| argument > 1)
                || event.active_before.is_none_or(|value| value & 0xFF == 0)
                || event.active_after.is_none_or(|value| value & 0xFF == 0)
                || event.flags & EVENT_FLAG_RUN_ACTIVE_BEFORE == 0
        })
        .count();
    let teardown_bad = teardown_events
        .iter()
        .filter(|event| {
            event.active_before.is_none_or(|value| value & 0xFF == 0)
                || event.active_after != Some(0)
                || event.flags & EVENT_FLAG_RUN_ACTIVE_BEFORE == 0
                || event.flags & EVENT_FLAG_RUN_ACTIVE_AFTER_ZERO == 0
        })
        .count();
    let start_ids = start_events
        .iter()
        .map(|event| event.attempt_id)
        .collect::<Vec<_>>();
    let lifecycle_ordered = start_events.windows(2).all(|pair| {
        pair[1].attempt_id == pair[0].attempt_id.saturating_add(1)
            && teardown_events
                .iter()
                .filter(|teardown| {
                    teardown.attempt_id == pair[0].attempt_id
                        && teardown.event_sequence > pair[0].event_sequence
                        && teardown.event_sequence < pair[1].event_sequence
                })
                .count()
                == 1
    });
    let offline_boundaries_exact = teardown_events.len().checked_add(1) == Some(start_events.len());
    let offline_starts_are_new = start_events
        .iter()
        .all(|event| event.lifecycle_argument == Some(0));
    let lifecycle_candidate_ok = match manifest.scenario.as_str() {
        "control-no-native" => start_events.is_empty() && teardown_events.is_empty(),
        "offline-disarmed" => {
            start_events.len() >= 2
                && offline_boundaries_exact
                && offline_starts_are_new
                && start_bad == 0
                && teardown_bad == 0
                && lifecycle_ordered
        }
        "original-online" => !start_events.is_empty() && start_bad == 0 && teardown_bad == 0,
        _ => false,
    };
    push(
        "run-lifecycle-candidate",
        lifecycle_candidate_ok,
        format!(
            "starts={} ids={start_ids:?}, teardowns={}, invalidStarts={start_bad}, invalidTeardowns={teardown_bad}, ordered={lifecycle_ordered}, exactOfflineBoundaries={offline_boundaries_exact}, offlineStartsAreNew={offline_starts_are_new}",
            start_events.len(),
            teardown_events.len()
        ),
    );

    let steam_events = events
        .iter()
        .filter(|event| {
            matches!(
                event.event_kind.as_str(),
                "steam_send" | "steam_available" | "steam_read"
            )
        })
        .collect::<Vec<_>>();
    let channel_bad = steam_events
        .iter()
        .filter(|event| event.channel != Some(0))
        .count();
    let payload_bad = steam_events
        .iter()
        .filter(|event| event.event_kind == "steam_send")
        .filter(|event| {
            event.payload_size.unwrap_or(0) != 0
                && (event.payload_hash.is_none()
                    || event.payload_hash_after.is_none()
                    || event.payload_hash != event.payload_hash_after)
        })
        .count();
    push(
        "original-steam-channel-zero",
        manifest.scenario != "original-online" || (!steam_events.is_empty() && channel_bad == 0),
        format!("calls={}, nonzeroChannel={channel_bad}", steam_events.len()),
    );
    push(
        "send-payload-unchanged",
        payload_bad == 0,
        format!("before/after digest mismatches={payload_bad}"),
    );

    let dispatch_counts = events
        .iter()
        .filter(|event| event.event_kind == "dispatch")
        .fold(HashMap::<u64, u32>::new(), |mut counts, event| {
            *counts.entry(event.attempt_id).or_default() += 1;
            counts
        });
    let mut stalled = 0_u64;
    let mut advanced = 0_u64;
    let mut frame_bad = 0_u64;
    let mut thread_bad = 0_u64;
    for event in events.iter().filter(|event| event.event_kind == "frame") {
        let dispatches = dispatch_counts.get(&event.attempt_id).copied().unwrap_or(0);
        thread_bad += events
            .iter()
            .filter(|candidate| {
                candidate.event_kind == "dispatch"
                    && candidate.attempt_id == event.attempt_id
                    && candidate.thread_id != event.thread_id
            })
            .count() as u64;
        let before = event.online_before;
        let after = event.online_after;
        if event.result_value == Some(0) {
            stalled += 1;
            if before != after || dispatches != 0 {
                frame_bad += 1;
            }
        } else {
            advanced += 1;
            if before
                .zip(after)
                .is_none_or(|(before, after)| after != before.wrapping_add(1))
                || dispatches != 1
            {
                frame_bad += 1;
            }
        }
    }
    push(
        "ready-stall-semantics",
        manifest.scenario != "original-online" || (stalled != 0 && advanced != 0 && frame_bad == 0),
        format!("stalls={stalled}, advanced={advanced}, frame/dispatch violations={frame_bad}"),
    );
    let simulation_threads = events
        .iter()
        .filter(|event| event.event_kind == "lua_simulation_pcall")
        .map(|event| event.thread_id)
        .collect::<BTreeSet<_>>();
    let frame_threads = events
        .iter()
        .filter(|event| event.event_kind == "frame")
        .map(|event| event.thread_id)
        .collect::<BTreeSet<_>>();
    let lifecycle_threads = events
        .iter()
        .filter(|event| {
            matches!(
                event.event_kind.as_str(),
                "post_game_start" | "run_teardown"
            )
        })
        .map(|event| event.thread_id)
        .collect::<BTreeSet<_>>();
    let wrong_thread_flags = events
        .iter()
        .filter(|event| event.flags & EVENT_FLAG_WRONG_THREAD != 0)
        .count();
    let thread_contract = manifest.scenario != "original-online"
        || (simulation_threads.len() == 1
            && frame_threads.len() == 1
            && simulation_threads == frame_threads
            && thread_bad == 0
            && wrong_thread_flags == 0);
    push(
        "simulation-thread-contract",
        thread_contract,
        format!(
            "simulationThreads={simulation_threads:?}, frameThreads={frame_threads:?}, crossThreadDispatches={thread_bad}, wrongThreadFlags={wrong_thread_flags}"
        ),
    );
    let lifecycle_thread_contract = manifest.scenario == "control-no-native"
        || (simulation_threads.len() == 1
            && lifecycle_threads.len() == 1
            && simulation_threads == lifecycle_threads
            && wrong_thread_flags == 0);
    push(
        "lifecycle-thread-contract",
        lifecycle_thread_contract,
        format!(
            "simulationThreads={simulation_threads:?}, lifecycleThreads={lifecycle_threads:?}, wrongThreadFlags={wrong_thread_flags}"
        ),
    );

    let frozen_modes = events
        .iter()
        .filter(|event| event.event_kind == "mode_frozen")
        .map(|event| (event.generation, event.network_mode.as_str()))
        .collect::<Vec<_>>();
    let mode_ok = if manifest.scenario == "control-no-native" {
        frozen_modes.is_empty()
    } else {
        !frozen_modes.is_empty()
            && frozen_modes
                .iter()
                .all(|(_, mode)| *mode == manifest.expected_mode)
            && frozen_modes.windows(2).all(|pair| pair[0].0 != pair[1].0)
    };
    push(
        "mode-freeze-no-hot-switch",
        mode_ok,
        format!(
            "expected={}, frozen={frozen_modes:?}",
            manifest.expected_mode
        ),
    );

    let retirement_generations = events
        .iter()
        .filter(|event| event.event_kind == "mode_retired")
        .map(|event| event.generation)
        .collect::<Vec<_>>();
    let lifecycle_mask = EVENT_FLAG_LUA_CLOSED
        | EVENT_FLAG_MENU_RETURN
        | EVENT_FLAG_MANAGER_CHANGED
        | EVENT_FLAG_FRAME_REGRESSED
        | EVENT_FLAG_WRONG_THREAD
        | EVENT_FLAG_NONZERO_CHANNEL;
    let retirement_without_cause = events
        .iter()
        .filter(|event| event.event_kind == "mode_retired")
        .filter(|event| event.flags & lifecycle_mask == 0)
        .count();
    let offline_run_retirements = events
        .iter()
        .filter(|event| {
            event.event_kind == "mode_retired"
                && event.flags
                    & (EVENT_FLAG_LUA_CLOSED | EVENT_FLAG_MENU_RETURN | EVENT_FLAG_MANAGER_CHANGED)
                    != 0
        })
        .count();
    let frozen_generations = frozen_modes
        .iter()
        .map(|(generation, _)| *generation)
        .collect::<BTreeSet<_>>();
    let lifecycle_ok = if manifest.scenario == "offline-disarmed" {
        retirement_without_cause == 0
            && offline_run_retirements != 0
            && frozen_generations.len() >= 2
            && lifecycle_candidate_ok
    } else {
        retirement_without_cause == 0
    };
    push(
        "generation-lifecycle-evidence",
        lifecycle_ok,
        format!(
            "retirements={}, missingCause={retirement_without_cause}, runBoundaryRetirements={offline_run_retirements}, frozenGenerations={} ",
            retirement_generations.len(),
            frozen_generations.len()
        ),
    );
    let revived = retirement_generations.iter().any(|generation| {
        events.iter().any(|event| {
            event.generation == *generation
                && event.event_kind == "mode_frozen"
                && event.event_sequence
                    > events
                        .iter()
                        .find(|candidate| {
                            candidate.generation == *generation
                                && candidate.event_kind == "mode_retired"
                        })
                        .map_or(u64::MAX, |candidate| candidate.event_sequence)
        })
    });
    push(
        "retired-generation-never-revives",
        !revived,
        format!("retired generations={retirement_generations:?}, revived={revived}"),
    );

    push(
        "safe-unload",
        !needs_native
            || (manifest.safe_unload == "PASS_RESTORED"
                && events
                    .iter()
                    .filter(|event| event.event_kind == "hooks_restored")
                    .count()
                    == 1),
        format!("safeUnload={}", manifest.safe_unload),
    );
    let native_failures = events
        .iter()
        .filter(|event| event.event_kind == "failure")
        .count();
    push(
        "native-failures-zero",
        native_failures == 0,
        format!("native failure events={native_failures}"),
    );
    verify_target_documents(run_directory, &mut push)?;
    for (name, expected) in &manifest.artifact_hashes {
        let actual = fs::read(run_directory.join(name))
            .usage_context(format!("cannot verify artifact {name}"))
            .map(|bytes| sha256_hex(&bytes))?;
        push(
            &format!("artifact-hash-{name}"),
            actual.eq_ignore_ascii_case(expected),
            format!("expected={expected} actual={actual}"),
        );
    }

    let mut counts = BTreeMap::new();
    for event in &events {
        *counts.entry(event.event_kind.clone()).or_insert(0) += 1;
    }
    let passed = manifest.status == "PASS" && checks.iter().all(|check| check.result == "PASS");
    Ok(ModeAnalyzerReport {
        schema_version: MODE_ARTIFACT_SCHEMA_VERSION,
        run_id: manifest.run_id,
        case_id: manifest.case_id,
        result: if passed { "PASS" } else { "FAIL" }.to_owned(),
        evidence_label: if passed {
            "RUNTIME_OBSERVED"
        } else {
            "UNKNOWN"
        }
        .to_owned(),
        checks,
        first_divergence,
        event_counts: counts,
        generated_utc: utc_now(),
    })
}

pub fn compare_mode_runs(
    control: &Path,
    instrumented: &Path,
    peer: Option<&Path>,
) -> AppResult<ModeComparisonReport> {
    let control_manifest: ModeRunManifest = read_json(&control.join("manifest.json"))?;
    let instrumented_manifest: ModeRunManifest = read_json(&instrumented.join("manifest.json"))?;
    if control_manifest.scenario != "control-no-native" {
        return Err(AppError::usage(
            "--control must name a control-no-native run",
        ));
    }
    let control_report = analyze_mode_run(control)?;
    let instrumented_report = analyze_mode_run(instrumented)?;
    let mut checks = Vec::new();
    let mut first_divergence = None;
    let mut push = |id: &str, passed: bool, detail: String| {
        if !passed && first_divergence.is_none() {
            first_divergence = Some(id.to_owned());
        }
        checks.push(AnalyzerCheck {
            id: id.to_owned(),
            result: if passed { "PASS" } else { "FAIL" }.to_owned(),
            detail,
        });
    };
    push(
        "input-runs-pass",
        control_report.result == "PASS" && instrumented_report.result == "PASS",
        format!(
            "control={}, instrumented={}",
            control_report.result, instrumented_report.result
        ),
    );
    push(
        "same-target-build",
        control_manifest.game.file_sha256 == instrumented_manifest.game.file_sha256
            && control_manifest.game.text_sha256 == instrumented_manifest.game.text_sha256,
        format!(
            "control={} instrumented={}",
            control_manifest.game.text_sha256, instrumented_manifest.game.text_sha256
        ),
    );
    let control_events = read_events(&control.join("endpoint.jsonl"))?;
    let instrumented_events = read_events(&instrumented.join("endpoint.jsonl"))?;
    let control_frames = control_events
        .iter()
        .filter(|event| event.event_kind == "control_sample")
        .filter_map(|event| event.online_before)
        .collect::<Vec<_>>();
    let instrumented_frames = instrumented_events
        .iter()
        .filter(|event| event.event_kind == "frame")
        .filter_map(|event| event.online_after.map(|frame| (event.generation, frame)))
        .collect::<Vec<_>>();
    let control_monotonic = control_frames.windows(2).all(|pair| pair[1] >= pair[0]);
    let instrumented_monotonic = instrumented_frames
        .windows(2)
        .all(|pair| pair[1].0 != pair[0].0 || pair[1].1 >= pair[0].1);
    let online_progress = instrumented_manifest.scenario != "original-online"
        || (control_frames.len() >= 2
            && control_frames.windows(2).any(|pair| pair[1] > pair[0])
            && instrumented_frames.len() >= 2
            && instrumented_frames
                .windows(2)
                .any(|pair| pair[1].0 == pair[0].0 && pair[1].1 > pair[0].1));
    push(
        "observable-frame-progression",
        control_monotonic && instrumented_monotonic && online_progress,
        format!(
            "controlSamples={} controlMonotonic={control_monotonic}; instrumentedFrames={} instrumentedMonotonic={instrumented_monotonic}; progress={online_progress}",
            control_frames.len(),
            instrumented_frames.len()
        ),
    );

    let mut peer_run_id = None;
    let mut packet_pairing = None;
    if let Some(peer) = peer {
        let peer_manifest: ModeRunManifest = read_json(&peer.join("manifest.json"))?;
        let peer_report = analyze_mode_run(peer)?;
        peer_run_id = Some(peer_manifest.run_id.clone());
        push(
            "peer-run-pass",
            peer_report.result == "PASS"
                && instrumented_manifest.scenario == "original-online"
                && peer_manifest.scenario == "original-online",
            format!(
                "peer={} scenario={}",
                peer_report.result, peer_manifest.scenario
            ),
        );
        let left = instrumented_events.clone();
        let right = read_events(&peer.join("endpoint.jsonl"))?;
        let left_local = instrumented_manifest.local_peer_token.as_deref();
        let right_local = peer_manifest.local_peer_token.as_deref();
        push(
            "pair-identities-anonymous-and-present",
            left_local.is_some() && right_local.is_some() && left_local != right_local,
            format!(
                "leftTokenPresent={} rightTokenPresent={} distinct={}",
                left_local.is_some(),
                right_local.is_some(),
                left_local != right_local
            ),
        );
        let a_to_b = packet_direction_audit(
            &left,
            &right,
            "steam_send",
            "steam_read",
            right_local,
            left_local,
        );
        let b_to_a = packet_direction_audit(
            &right,
            &left,
            "steam_send",
            "steam_read",
            left_local,
            right_local,
        );
        let audit = DualEndpointPacketAudit {
            rule_version: "event-anchor-delivery-semantics-v2".to_owned(),
            a_to_b,
            b_to_a,
        };
        push(
            "dual-endpoint-packet-origin-and-delivery-semantics",
            audit.a_to_b.passed() && audit.b_to_a.passed(),
            format!(
                "A->B matched={} edgeReads={} unknownReads={} unreliableMissing={} reliableMissing={} invalidModes={}; B->A matched={} edgeReads={} unknownReads={} unreliableMissing={} reliableMissing={} invalidModes={}",
                audit.a_to_b.matched_packets,
                audit.a_to_b.edge_reads_excluded,
                audit.a_to_b.unmatched_reads_inside,
                audit.a_to_b.unreliable_missing_sends_inside,
                audit.a_to_b.reliable_missing_sends_inside,
                audit.a_to_b.invalid_or_missing_send_modes,
                audit.b_to_a.matched_packets,
                audit.b_to_a.edge_reads_excluded,
                audit.b_to_a.unmatched_reads_inside,
                audit.b_to_a.unreliable_missing_sends_inside,
                audit.b_to_a.reliable_missing_sends_inside,
                audit.b_to_a.invalid_or_missing_send_modes,
            ),
        );
        packet_pairing = Some(audit);
    }
    let passed = checks.iter().all(|check| check.result == "PASS");
    Ok(ModeComparisonReport {
        schema_version: MODE_ARTIFACT_SCHEMA_VERSION,
        analysis_revision: "t0-02-packet-pairing-v2".to_owned(),
        result: if passed { "PASS" } else { "FAIL" }.to_owned(),
        control_run_id: control_manifest.run_id,
        instrumented_run_id: instrumented_manifest.run_id,
        peer_run_id,
        packet_pairing,
        checks,
        first_divergence,
        generated_utc: utc_now(),
    })
}

pub fn write_mode_analysis(run_directory: &Path, report: &ModeAnalyzerReport) -> AppResult<()> {
    write_pretty_json(&run_directory.join("analyzer.json"), report)?;
    let notes = format!(
        "# Isaac C/S T0-02 mode isolation\n\n- Run: `{}`\n- Case: `{}`\n- Result: **{}**\n- Evidence: `{}`\n- First divergence: `{}`\n\nThe artifact contains no payload, full SteamID, user name, pairing key, or absolute game path.\n",
        report.run_id,
        report.case_id,
        report.result,
        report.evidence_label,
        report.first_divergence.as_deref().unwrap_or("none")
    );
    fs::write(run_directory.join("notes.md"), notes).usage_context("cannot write mode notes")
}

pub fn write_mode_comparison(path: &Path, report: &ModeComparisonReport) -> AppResult<()> {
    let bytes = serde_json::to_vec_pretty(report)
        .usage_context(format!("cannot serialize {}", path.display()))?;
    let mut file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .usage_context(format!("cannot create comparison {}", path.display()))?;
    file.write_all(&bytes)
        .usage_context(format!("cannot write comparison {}", path.display()))
}

#[derive(Clone, Debug, Eq, Ord, PartialEq, PartialOrd)]
struct PacketIdentity {
    payload_hash: String,
    payload_size: u32,
}

#[derive(Clone, Copy, Debug)]
struct SendObservation {
    event_sequence: u64,
    send_mode: Option<i32>,
}

fn packet_direction_audit(
    sends: &[ModeEvent],
    reads: &[ModeEvent],
    send_peer: &str,
    read_peer: &str,
    expected_send_peer: Option<&str>,
    expected_read_peer: Option<&str>,
) -> PacketDirectionAudit {
    let mut send_observations = Vec::<SendObservation>::new();
    let mut send_indices = BTreeMap::<PacketIdentity, Vec<usize>>::new();
    for event in sends.iter().filter(|event| {
        event.event_kind == send_peer
            && event.result_value == Some(1)
            && expected_send_peer.is_none_or(|token| event.peer_token.as_deref() == Some(token))
    }) {
        if let Some(hash) = &event.payload_hash {
            let index = send_observations.len();
            send_observations.push(SendObservation {
                event_sequence: event.event_sequence,
                send_mode: event.send_mode,
            });
            send_indices
                .entry(PacketIdentity {
                    payload_hash: hash.clone(),
                    payload_size: event.payload_size.unwrap_or(0),
                })
                .or_default()
                .push(index);
        }
    }
    let mut next_send = BTreeMap::<PacketIdentity, usize>::new();
    let mut matched_sends = vec![false; send_observations.len()];
    let mut matched_read_sequences = Vec::<u64>::new();
    let mut unmatched_read_sequences = Vec::<u64>::new();
    for event in reads.iter().filter(|event| {
        event.event_kind == read_peer
            && event.result_value == Some(1)
            && expected_read_peer.is_none_or(|token| event.peer_token.as_deref() == Some(token))
    }) {
        if let Some(hash) = &event.payload_hash {
            let identity = PacketIdentity {
                payload_hash: hash.clone(),
                payload_size: event.output_size.unwrap_or(0),
            };
            let occurrence = next_send.entry(identity.clone()).or_default();
            if let Some(index) = send_indices
                .get(&identity)
                .and_then(|indices| indices.get(*occurrence))
            {
                matched_sends[*index] = true;
                *occurrence += 1;
                matched_read_sequences.push(event.event_sequence);
            } else {
                unmatched_read_sequences.push(event.event_sequence);
            }
        }
    }

    let matched_send_sequences = send_observations
        .iter()
        .zip(&matched_sends)
        .filter_map(|(event, matched)| matched.then_some(event.event_sequence))
        .collect::<Vec<_>>();
    let first_matched_send_sequence = matched_send_sequences.iter().copied().min();
    let last_matched_send_sequence = matched_send_sequences.iter().copied().max();
    let first_matched_read_sequence = matched_read_sequences.iter().copied().min();
    let last_matched_read_sequence = matched_read_sequences.iter().copied().max();
    let inside = |sequence: u64, first: Option<u64>, last: Option<u64>| {
        first.is_some_and(|first| sequence >= first) && last.is_some_and(|last| sequence <= last)
    };
    let unmatched_reads_inside = unmatched_read_sequences
        .iter()
        .filter(|sequence| {
            inside(
                **sequence,
                first_matched_read_sequence,
                last_matched_read_sequence,
            )
        })
        .count() as u64;
    let edge_reads_excluded = unmatched_read_sequences.len() as u64 - unmatched_reads_inside;

    let mut send_modes = BTreeSet::new();
    let mut edge_sends_excluded = 0_u64;
    let mut unreliable_missing_sends_inside = 0_u64;
    let mut reliable_missing_sends_inside = 0_u64;
    let mut invalid_or_missing_send_modes = 0_u64;
    for (event, matched) in send_observations.iter().zip(&matched_sends) {
        match event.send_mode {
            Some(mode @ 0..=3) => {
                send_modes.insert(mode);
            }
            _ => invalid_or_missing_send_modes += 1,
        }
        if *matched {
            continue;
        }
        if !inside(
            event.event_sequence,
            first_matched_send_sequence,
            last_matched_send_sequence,
        ) {
            edge_sends_excluded += 1;
            continue;
        }
        match event.send_mode {
            Some(0 | 1) => unreliable_missing_sends_inside += 1,
            Some(2 | 3) => reliable_missing_sends_inside += 1,
            _ => {}
        }
    }

    PacketDirectionAudit {
        matched_packets: matched_read_sequences.len() as u64,
        send_modes: send_modes.into_iter().collect(),
        first_matched_send_sequence,
        last_matched_send_sequence,
        first_matched_read_sequence,
        last_matched_read_sequence,
        edge_reads_excluded,
        unmatched_reads_inside,
        edge_sends_excluded,
        unreliable_missing_sends_inside,
        reliable_missing_sends_inside,
        invalid_or_missing_send_modes,
    }
}

fn verify_target_documents(
    run_directory: &Path,
    push: &mut impl FnMut(&str, bool, String),
) -> AppResult<()> {
    let before_path = run_directory.join("target-bytes-before.json");
    let after_path = run_directory.join("target-bytes-after.json");
    if !before_path.is_file() || !after_path.is_file() {
        push(
            "target-snapshots-present",
            false,
            "missing target-bytes-before.json or target-bytes-after.json".to_owned(),
        );
        return Ok(());
    }
    let before: TargetBytesDocument = read_json(&before_path)?;
    let after: TargetBytesDocument = read_json(&after_path)?;
    let disk_equal = before.disk.image.file_sha256 == after.disk.image.file_sha256
        && before.disk.image.text_sha256 == after.disk.image.text_sha256;
    push(
        "disk-image-unchanged",
        disk_equal,
        format!(
            "before={} after={}",
            before.disk.image.file_sha256, after.disk.image.file_sha256
        ),
    );
    let live_equal = before
        .process
        .as_ref()
        .zip(after.process.as_ref())
        .is_some_and(|(before, after)| {
            before.pid == after.pid
                && before.creation_time_100ns == after.creation_time_100ns
                && before.module_base == after.module_base
                && before.targets.len() == after.targets.len()
                && before
                    .targets
                    .iter()
                    .zip(&after.targets)
                    .all(|(left, right)| {
                        left.id == right.id
                            && left.rva == right.rva
                            && left.address == right.address
                            && left.bytes == right.bytes
                            && left.semantic_result == right.semantic_result
                            && left.resolved_target == right.resolved_target
                            && left.resolved_owner_module == right.resolved_owner_module
                    })
        });
    push(
        "live-target-bytes-restored",
        live_equal,
        "every T0-01 baseline and T0-02 candidate target matches after the observation/unload"
            .to_owned(),
    );
    Ok(())
}

fn read_events(path: &Path) -> AppResult<Vec<ModeEvent>> {
    let file = File::open(path).usage_context(format!("cannot open {}", path.display()))?;
    BufReader::new(file)
        .lines()
        .enumerate()
        .map(|(index, line)| {
            let line = line.usage_context(format!("cannot read line {}", index + 1))?;
            if line.trim().is_empty() {
                return Err(AppError::usage(format!("empty JSONL line {}", index + 1)));
            }
            serde_json::from_str(&line)
                .usage_context(format!("invalid mode event line {}", index + 1))
        })
        .collect()
}

fn write_pretty_json(path: &Path, value: &impl Serialize) -> AppResult<()> {
    let bytes = serde_json::to_vec_pretty(value)
        .usage_context(format!("cannot serialize {}", path.display()))?;
    fs::write(path, bytes).usage_context(format!("cannot write {}", path.display()))
}

fn read_json<T: for<'de> Deserialize<'de>>(path: &Path) -> AppResult<T> {
    let bytes = fs::read(path).usage_context(format!("cannot read {}", path.display()))?;
    serde_json::from_slice(&bytes).usage_context(format!("cannot parse {}", path.display()))
}

fn validate_component(value: &str, label: &str) -> AppResult<()> {
    if value.is_empty()
        || value.len() > 64
        || !value
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_'))
    {
        return Err(AppError::usage(format!(
            "{label} must be 1-64 ASCII alphanumeric, '-' or '_' characters"
        )));
    }
    Ok(())
}

fn mode_name(value: u32) -> AppResult<&'static str> {
    match value {
        0 => Ok("Unresolved"),
        1 => Ok("Offline"),
        2 => Ok("OriginalOnline"),
        3 => Ok("CustomAuthorityServer"),
        4 => Ok("CustomAuthorityClient"),
        5 => Ok("Retired"),
        _ => Err(AppError::evidence(format!(
            "native event has invalid mode {value}"
        ))),
    }
}

fn native_detail(kind: NativeEventKind) -> String {
    format!("T0-02 native event {kind:?}")
}

fn hook_names(mask: u32) -> Vec<String> {
    [
        (1 << 0, "input-advance-call"),
        (1 << 1, "online-dispatch-call-5110ab"),
        (1 << 2, "lua_newstate-IAT"),
        (1 << 3, "luaL_newstate-IAT"),
        (1 << 4, "lua_close-IAT"),
        (1 << 5, "lua_pcallk-IAT"),
        (1 << 6, "SteamInternal_ContextInit-IAT"),
        (1 << 7, "SteamNetworking006-vtable-0-2"),
        (1 << 8, "post-game-start-callers"),
        (1 << 9, "run-teardown-callers"),
    ]
    .into_iter()
    .filter(|(bit, _)| mask & bit != 0)
    .map(|(_, name)| name.to_owned())
    .collect()
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::artifact::{LiveTargetSnapshot, ModuleSnapshot};
    use crate::ledger::{InspectedImage, StaticInspectionReport, TargetInspection};

    #[test]
    fn existing_c2_artifacts_replay_packet_v2_read_only() {
        let root = Path::new(env!("CARGO_MANIFEST_DIR"))
            .parent()
            .unwrap()
            .join("artifacts")
            .join("cs-t0");
        let control = root.join("20260819T075721Z-t0-02-c0-original-online-a-final-a");
        let instrumented = root.join("20260819T080844Z-t0-02-c2-original-online-a-a");
        let peer = root.join("20260819T081006Z-t0-02-c2-original-online-b-b");
        let report = compare_mode_runs(&control, &instrumented, Some(&peer)).unwrap();
        assert_eq!(report.analysis_revision, "t0-02-packet-pairing-v2");
        assert_eq!(report.result, "PASS");
        let audit = report.packet_pairing.unwrap();
        assert_eq!(audit.a_to_b.unmatched_reads_inside, 0);
        assert_eq!(audit.b_to_a.unmatched_reads_inside, 0);
        assert_eq!(audit.a_to_b.reliable_missing_sends_inside, 0);
        assert_eq!(audit.b_to_a.reliable_missing_sends_inside, 0);
        assert!(audit.a_to_b.unreliable_missing_sends_inside > 0);
        assert!(audit.b_to_a.unreliable_missing_sends_inside > 0);
    }

    fn base_event(sequence: u64, native: Option<u64>, kind: &str) -> ModeEvent {
        ModeEvent {
            schema_version: MODE_ARTIFACT_SCHEMA_VERSION,
            event_sequence: sequence,
            source: if native.is_some() {
                "native"
            } else {
                "controller"
            }
            .into(),
            native_sequence: native,
            qpc: sequence as i64,
            thread_id: 1,
            endpoint: "a".into(),
            network_mode: "Unresolved".into(),
            generation: 1,
            attempt_id: 0,
            event_kind: kind.into(),
            result: "PASS".into(),
            result_value: None,
            lifecycle_argument: None,
            original_calls: None,
            online_before: None,
            online_after: None,
            channel: None,
            send_mode: None,
            payload_size: None,
            output_size: None,
            active_before: None,
            active_after: None,
            peer_token: None,
            payload_hash: None,
            payload_hash_after: None,
            flags: 0,
            signal_mask: 0,
            error_code: None,
            detail: kind.into(),
        }
    }

    fn packet_event(
        sequence: u64,
        kind: &str,
        hash_byte: &str,
        peer: Option<&str>,
        send_mode: Option<i32>,
    ) -> ModeEvent {
        let mut event = base_event(sequence, Some(sequence), kind);
        event.result_value = Some(1);
        event.channel = Some(0);
        event.peer_token = peer.map(str::to_owned);
        event.payload_hash = Some(hash_byte.repeat(64));
        if kind == "steam_send" {
            event.payload_size = Some(4);
            event.payload_hash_after = event.payload_hash.clone();
            event.send_mode = send_mode;
        } else {
            event.output_size = Some(4);
        }
        event
    }

    fn static_inspection() -> StaticInspectionReport {
        StaticInspectionReport {
            schema_version: 1,
            result: "PASS".into(),
            ledger_id: "baseline".into(),
            ledger_sha256: "A".repeat(64),
            variant_id: "online-mod-patched-2026-04-21".into(),
            static_patch_state: "ONLINE_MOD_TWO_PATCHES_ENABLED".into(),
            image: InspectedImage {
                file_name: "isaac-ng.exe".into(),
                machine: "0x014C".into(),
                timestamp: "0x69E6E3A7".into(),
                image_size: "0x0092C000".into(),
                preferred_image_base: "0x00400000".into(),
                entry_point_rva: "0x008FE310".into(),
                file_sha256: "B".repeat(64),
                text_sha256: "C".repeat(64),
                sections: Vec::new(),
            },
            targets: vec![TargetInspection {
                id: "frame".into(),
                semantic: "frame".into(),
                kind: "relativeCall".into(),
                rva: "0x00000010".into(),
                feature_rva: "0x00000010".into(),
                feature_matches: 1,
                original_bytes: "E800000000".into(),
                resolved_target: Some("RVA 0x00000020".into()),
                import_name: None,
                evidence_label: "STATIC_CONFIRMED".into(),
                result: "PASS".into(),
            }],
        }
    }

    fn live_snapshot() -> LiveProcessSnapshot {
        LiveProcessSnapshot {
            pid: 7,
            creation_time_100ns: 8,
            module_base: "0x00400000".into(),
            module_size: 0x92C000,
            preferred_image_base: "0x00400000".into(),
            load_bias: "0x0".into(),
            debugger_present: false,
            forbidden_modules: Vec::new(),
            modules: vec![ModuleSnapshot {
                name: "isaac-ng.exe".into(),
                base: "0x00400000".into(),
                size: 0x92C000,
            }],
            targets: vec![LiveTargetSnapshot {
                id: "frame".into(),
                rva: "0x00000010".into(),
                address: "0x00400010".into(),
                bytes: "E800000000".into(),
                semantic_result: "relative call target verified".into(),
                resolved_target: Some("0x00400020".into()),
                resolved_owner_module: Some("isaac-ng.exe".into()),
            }],
            sampled_qpc: 9,
        }
    }

    fn write_fixture(mut events: Vec<ModeEvent>) -> (tempfile::TempDir, PathBuf) {
        for (index, event) in events.iter_mut().enumerate() {
            event.event_sequence = index as u64 + 1;
        }
        let temporary = tempfile::tempdir().unwrap();
        let run_id = "20260818T000000Z-t0-02-fixture-a";
        let directory = temporary.path().join(run_id);
        fs::create_dir(&directory).unwrap();
        let inspection = static_inspection();
        let manifest = ModeRunManifest {
            schema_version: MODE_ARTIFACT_SCHEMA_VERSION,
            run_id: run_id.into(),
            case_id: "fixture".into(),
            experiment_identity: MODE_TOOL_IDENTITY.into(),
            status: "PASS".into(),
            started_utc: utc_now(),
            ended_utc: Some(utc_now()),
            end_reason: Some("fixture".into()),
            scenario: "original-online".into(),
            endpoint: "a".into(),
            expected_mode: "OriginalOnline".into(),
            game: GameManifest {
                file_name: inspection.image.file_name.clone(),
                machine: inspection.image.machine.clone(),
                timestamp: inspection.image.timestamp.clone(),
                image_size: inspection.image.image_size.clone(),
                file_sha256: inspection.image.file_sha256.clone(),
                text_sha256: inspection.image.text_sha256.clone(),
                variant_id: inspection.variant_id.clone(),
                static_patch_state: inspection.static_patch_state.clone(),
            },
            tool: ToolManifest {
                identity: MODE_TOOL_IDENTITY.into(),
                version: "0".into(),
                executable_sha256: "D".repeat(64),
            },
            baseline_ledger_id: "baseline".into(),
            baseline_ledger_sha256: "A".repeat(64),
            mode_ledger_id: "mode".into(),
            mode_ledger_sha256: "E".repeat(64),
            process: None,
            native_dll_sha256: Some("F".repeat(64)),
            helper_sha256: Some("1".repeat(64)),
            installed_hooks: vec!["frame".into()],
            native_probe_installed: true,
            safe_unload: "PASS_RESTORED".into(),
            pairing_key_stored: false,
            local_peer_token: Some("peer:AAAA".into()),
            artifact_hashes: BTreeMap::new(),
        };
        write_pretty_json(&directory.join("manifest.json"), &manifest).unwrap();
        let mut log = BufWriter::new(File::create(directory.join("endpoint.jsonl")).unwrap());
        for event in events {
            serde_json::to_writer(&mut log, &event).unwrap();
            log.write_all(b"\n").unwrap();
        }
        log.flush().unwrap();
        for (name, phase) in [
            ("target-bytes-before.json", "before"),
            ("target-bytes-after.json", "after"),
        ] {
            write_pretty_json(
                &directory.join(name),
                &TargetBytesDocument {
                    schema_version: 1,
                    phase: phase.into(),
                    captured_utc: utc_now(),
                    disk: inspection.clone(),
                    process: Some(live_snapshot()),
                },
            )
            .unwrap();
        }
        (temporary, directory)
    }

    fn passing_events() -> Vec<ModeEvent> {
        let mut initialized = base_event(0, Some(1), "native_initialized");
        initialized.generation = 0;
        let mut installed = base_event(0, Some(2), "hooks_installed");
        installed.generation = 0;
        let mut start = base_event(0, Some(3), "post_game_start");
        start.generation = 0;
        start.attempt_id = 1;
        start.lifecycle_argument = Some(0);
        start.original_calls = Some(1);
        start.active_before = Some(1);
        start.active_after = Some(1);
        start.flags = EVENT_FLAG_RUN_ACTIVE_BEFORE;
        let mut simulation = base_event(0, Some(4), "lua_simulation_pcall");
        simulation.original_calls = Some(1);
        simulation.result_value = Some(0);
        let mut frozen = base_event(0, Some(5), "mode_frozen");
        frozen.network_mode = "OriginalOnline".into();
        let mut send = base_event(0, Some(6), "steam_send");
        send.network_mode = "OriginalOnline".into();
        send.original_calls = Some(1);
        send.result_value = Some(1);
        send.channel = Some(0);
        send.payload_size = Some(3);
        send.payload_hash = Some("AA".repeat(32));
        send.payload_hash_after = send.payload_hash.clone();
        let mut stalled = base_event(0, Some(7), "frame");
        stalled.network_mode = "OriginalOnline".into();
        stalled.attempt_id = 1;
        stalled.original_calls = Some(1);
        stalled.result_value = Some(0);
        stalled.online_before = Some(10);
        stalled.online_after = Some(10);
        let mut dispatch = base_event(0, Some(8), "dispatch");
        dispatch.network_mode = "OriginalOnline".into();
        dispatch.attempt_id = 2;
        dispatch.original_calls = Some(1);
        dispatch.result_value = Some(7);
        let mut advanced = base_event(0, Some(9), "frame");
        advanced.network_mode = "OriginalOnline".into();
        advanced.attempt_id = 2;
        advanced.original_calls = Some(1);
        advanced.result_value = Some(1);
        advanced.online_before = Some(10);
        advanced.online_after = Some(11);
        let mut restored = base_event(0, Some(10), "hooks_restored");
        restored.network_mode = "OriginalOnline".into();
        vec![
            initialized,
            installed,
            start,
            simulation,
            frozen,
            send,
            stalled,
            dispatch,
            advanced,
            restored,
        ]
    }

    fn passing_offline_events() -> Vec<ModeEvent> {
        let mut initialized = base_event(0, Some(1), "native_initialized");
        initialized.generation = 0;
        let mut installed = base_event(0, Some(2), "hooks_installed");
        installed.generation = 0;
        let mut first_start = base_event(0, Some(3), "post_game_start");
        first_start.generation = 0;
        first_start.attempt_id = 1;
        first_start.lifecycle_argument = Some(0);
        first_start.original_calls = Some(1);
        first_start.active_before = Some(1);
        first_start.active_after = Some(1);
        first_start.flags = EVENT_FLAG_RUN_ACTIVE_BEFORE;
        let mut first_callback = base_event(0, Some(4), "lua_simulation_pcall");
        first_callback.original_calls = Some(1);
        let mut first_frozen = base_event(0, Some(5), "mode_frozen");
        first_frozen.network_mode = "Offline".into();
        let mut retired = base_event(0, Some(6), "mode_retired");
        retired.network_mode = "Retired".into();
        retired.flags = EVENT_FLAG_MENU_RETURN;
        let mut teardown = base_event(0, Some(7), "run_teardown");
        teardown.network_mode = "Retired".into();
        teardown.attempt_id = 1;
        teardown.lifecycle_argument = Some(0);
        teardown.original_calls = Some(1);
        teardown.active_before = Some(1);
        teardown.active_after = Some(0);
        teardown.flags = EVENT_FLAG_RUN_ACTIVE_BEFORE | EVENT_FLAG_RUN_ACTIVE_AFTER_ZERO;
        let mut second_start = base_event(0, Some(8), "post_game_start");
        second_start.network_mode = "Retired".into();
        second_start.attempt_id = 2;
        second_start.lifecycle_argument = Some(0);
        second_start.original_calls = Some(1);
        second_start.active_before = Some(1);
        second_start.active_after = Some(1);
        second_start.flags = EVENT_FLAG_RUN_ACTIVE_BEFORE;
        let mut second_callback = base_event(0, Some(9), "lua_simulation_pcall");
        second_callback.generation = 2;
        second_callback.original_calls = Some(1);
        let mut second_frozen = base_event(0, Some(10), "mode_frozen");
        second_frozen.generation = 2;
        second_frozen.network_mode = "Offline".into();
        let mut restored = base_event(0, Some(11), "hooks_restored");
        restored.generation = 2;
        restored.network_mode = "Offline".into();
        vec![
            initialized,
            installed,
            first_start,
            first_callback,
            first_frozen,
            retired,
            teardown,
            second_start,
            second_callback,
            second_frozen,
            restored,
        ]
    }

    fn write_offline_fixture(events: Vec<ModeEvent>) -> (tempfile::TempDir, PathBuf) {
        let (temporary, directory) = write_fixture(events);
        let manifest_path = directory.join("manifest.json");
        let mut manifest: ModeRunManifest = read_json(&manifest_path).unwrap();
        manifest.scenario = "offline-disarmed".into();
        manifest.endpoint = "solo".into();
        manifest.expected_mode = "Offline".into();
        manifest.local_peer_token = None;
        write_pretty_json(&manifest_path, &manifest).unwrap();
        (temporary, directory)
    }

    #[test]
    fn event_writer_assigns_contiguous_sequence_and_rejects_overwrite() {
        let temporary = tempfile::tempdir().unwrap();
        let path = temporary.path().join("endpoint.jsonl");
        let mut writer = ModeEventWriter::create(&path).unwrap();
        writer
            .write(ModeEvent::controller(1, "solo", "one", "PASS", "one"))
            .unwrap();
        writer
            .write(ModeEvent::controller(2, "solo", "two", "PASS", "two"))
            .unwrap();
        writer.flush().unwrap();
        let events = read_events(&path).unwrap();
        assert_eq!(events[0].event_sequence, 1);
        assert_eq!(events[1].event_sequence, 2);
        assert!(ModeEventWriter::create(&path).is_err());
    }

    #[test]
    fn mode_event_json_has_no_pairing_key_or_identity_fields() {
        let json = serde_json::to_string(&ModeEvent::controller(
            1,
            "a",
            "observer_started",
            "PASS",
            "anonymous",
        ))
        .unwrap();
        assert!(!json.to_ascii_lowercase().contains("pairingkey"));
        assert!(!json.to_ascii_lowercase().contains("steamid"));
        assert!(!json.contains("C:\\"));
    }

    #[test]
    fn analyzer_accepts_passthrough_stall_and_restoration_fixture() {
        let (_temporary, directory) = write_fixture(passing_events());
        let report = analyze_mode_run(&directory).unwrap();
        assert_eq!(report.result, "PASS");
        assert!(report.first_divergence.is_none());
    }

    #[test]
    fn analyzer_fails_closed_on_nonzero_channel_and_stall_dispatch() {
        let mut events = passing_events();
        events[5].channel = Some(1);
        events[7].attempt_id = 1;
        let (_temporary, directory) = write_fixture(events);
        let report = analyze_mode_run(&directory).unwrap();
        assert_eq!(report.result, "FAIL");
        assert_eq!(
            report.first_divergence.as_deref(),
            Some("original-steam-channel-zero")
        );
        assert!(
            report
                .checks
                .iter()
                .any(|check| check.id == "ready-stall-semantics" && check.result == "FAIL")
        );
    }

    #[test]
    fn analyzer_rejects_native_sequence_gap() {
        let mut events = passing_events();
        events[5].native_sequence = Some(99);
        let (_temporary, directory) = write_fixture(events);
        let report = analyze_mode_run(&directory).unwrap();
        assert_eq!(
            report.first_divergence.as_deref(),
            Some("native-event-sequence")
        );
    }

    #[test]
    fn analyzer_accepts_observed_teardown_as_offline_generation_boundary() {
        let (_temporary, directory) = write_offline_fixture(passing_offline_events());
        let report = analyze_mode_run(&directory).unwrap();
        assert_eq!(report.result, "PASS");
        assert!(report.first_divergence.is_none());
    }

    #[test]
    fn analyzer_rejects_manager_only_retirement_without_teardown_boundary() {
        let mut events = passing_offline_events();
        events.remove(6);
        for (index, event) in events.iter_mut().enumerate() {
            event.native_sequence = Some(index as u64 + 1);
        }
        let (_temporary, directory) = write_offline_fixture(events);
        let report = analyze_mode_run(&directory).unwrap();
        assert_eq!(report.result, "FAIL");
        assert_eq!(
            report.first_divergence.as_deref(),
            Some("run-lifecycle-candidate")
        );
    }

    #[test]
    fn analyzer_rejects_teardown_without_active_to_zero_transition() {
        let mut events = passing_offline_events();
        events[6].active_after = Some(1);
        events[6].flags &= !EVENT_FLAG_RUN_ACTIVE_AFTER_ZERO;
        let (_temporary, directory) = write_offline_fixture(events);
        let report = analyze_mode_run(&directory).unwrap();
        assert_eq!(report.result, "FAIL");
        assert_eq!(
            report.first_divergence.as_deref(),
            Some("run-lifecycle-candidate")
        );
    }

    #[test]
    fn packet_audit_allows_unreliable_loss_inside_anchored_window() {
        let sends = vec![
            packet_event(1, "steam_send", "A", Some("peer:B"), Some(1)),
            packet_event(2, "steam_send", "B", Some("peer:B"), Some(1)),
            packet_event(3, "steam_send", "C", Some("peer:B"), Some(1)),
        ];
        let reads = vec![
            packet_event(10, "steam_read", "A", Some("peer:A"), None),
            packet_event(12, "steam_read", "C", Some("peer:A"), None),
        ];
        let audit = packet_direction_audit(
            &sends,
            &reads,
            "steam_send",
            "steam_read",
            Some("peer:B"),
            Some("peer:A"),
        );
        assert!(audit.passed());
        assert_eq!(audit.matched_packets, 2);
        assert_eq!(audit.unreliable_missing_sends_inside, 1);
        assert_eq!(audit.reliable_missing_sends_inside, 0);
    }

    #[test]
    fn packet_audit_rejects_unknown_read_inside_anchors_but_excludes_edges() {
        let sends = vec![
            packet_event(1, "steam_send", "A", Some("peer:B"), Some(1)),
            packet_event(2, "steam_send", "C", Some("peer:B"), Some(1)),
        ];
        let interior_reads = vec![
            packet_event(9, "steam_read", "D", Some("peer:A"), None),
            packet_event(10, "steam_read", "A", Some("peer:A"), None),
            packet_event(11, "steam_read", "B", Some("peer:A"), None),
            packet_event(12, "steam_read", "C", Some("peer:A"), None),
            packet_event(13, "steam_read", "E", Some("peer:A"), None),
        ];
        let audit = packet_direction_audit(
            &sends,
            &interior_reads,
            "steam_send",
            "steam_read",
            Some("peer:B"),
            Some("peer:A"),
        );
        assert!(!audit.passed());
        assert_eq!(audit.unmatched_reads_inside, 1);
        assert_eq!(audit.edge_reads_excluded, 2);
    }

    #[test]
    fn packet_audit_rejects_reliable_loss_and_invalid_modes() {
        let reliable_sends = vec![
            packet_event(1, "steam_send", "A", Some("peer:B"), Some(2)),
            packet_event(2, "steam_send", "B", Some("peer:B"), Some(2)),
            packet_event(3, "steam_send", "C", Some("peer:B"), Some(2)),
        ];
        let reads = vec![
            packet_event(10, "steam_read", "A", Some("peer:A"), None),
            packet_event(12, "steam_read", "C", Some("peer:A"), None),
        ];
        let reliable = packet_direction_audit(
            &reliable_sends,
            &reads,
            "steam_send",
            "steam_read",
            Some("peer:B"),
            Some("peer:A"),
        );
        assert!(!reliable.passed());
        assert_eq!(reliable.reliable_missing_sends_inside, 1);

        let mut invalid_sends = reliable_sends;
        invalid_sends[1].send_mode = None;
        let invalid = packet_direction_audit(
            &invalid_sends,
            &[
                packet_event(10, "steam_read", "A", Some("peer:A"), None),
                packet_event(11, "steam_read", "B", Some("peer:A"), None),
                packet_event(12, "steam_read", "C", Some("peer:A"), None),
            ],
            "steam_send",
            "steam_read",
            Some("peer:B"),
            Some("peer:A"),
        );
        assert!(!invalid.passed());
        assert_eq!(invalid.invalid_or_missing_send_modes, 1);
    }

    #[test]
    fn packet_audit_handles_duplicate_payload_multiplicity_and_requires_two_anchors() {
        let sends = vec![
            packet_event(1, "steam_send", "A", Some("peer:B"), Some(1)),
            packet_event(2, "steam_send", "A", Some("peer:B"), Some(1)),
            packet_event(3, "steam_send", "C", Some("peer:B"), Some(1)),
        ];
        let reads = vec![
            packet_event(10, "steam_read", "A", Some("peer:A"), None),
            packet_event(12, "steam_read", "C", Some("peer:A"), None),
        ];
        let duplicate = packet_direction_audit(
            &sends,
            &reads,
            "steam_send",
            "steam_read",
            Some("peer:B"),
            Some("peer:A"),
        );
        assert!(duplicate.passed());
        assert_eq!(duplicate.unreliable_missing_sends_inside, 1);

        let insufficient = packet_direction_audit(
            &sends,
            &reads[..1],
            "steam_send",
            "steam_read",
            Some("peer:B"),
            Some("peer:A"),
        );
        assert!(!insufficient.passed());
        assert_eq!(insufficient.matched_packets, 1);

        let missing_identity = packet_direction_audit(
            &sends,
            &[
                packet_event(10, "steam_read", "A", None, None),
                packet_event(12, "steam_read", "C", None, None),
            ],
            "steam_send",
            "steam_read",
            Some("peer:B"),
            Some("peer:A"),
        );
        assert!(!missing_identity.passed());
        assert_eq!(missing_identity.matched_packets, 0);
    }
}
