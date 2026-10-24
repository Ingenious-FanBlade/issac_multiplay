use std::collections::BTreeMap;
use std::fs::{self, File, OpenOptions};
use std::io::{BufRead, BufReader, Write};
use std::path::{Path, PathBuf};

use chrono::Utc;
use serde::{Deserialize, Serialize};

use crate::BOUNDARY_TOOL_IDENTITY;
use crate::artifact::{LiveProcessSnapshot, TargetBytesDocument, file_sha256, utc_now};
use crate::boundary_ipc::{
    BOUNDARY_FLAG_GAMEPLAY_DISPATCH_SUPPRESSED, BOUNDARY_FLAG_LIFECYCLE_RETIRED,
    BOUNDARY_FLAG_LUA_NEW_ROOT, BOUNDARY_FLAG_LUA_ROOT_MATCH, BOUNDARY_FLAG_MAIN_LOOP_STATE_VALID,
    BOUNDARY_FLAG_RETIREMENT_LUA_CLOSE, BOUNDARY_FLAG_RETIREMENT_LUA_REBUILD, BoundaryEventKind,
};
use crate::error::{AppError, AppResult, UsageContext};

pub const BOUNDARY_ARTIFACT_SCHEMA_VERSION: u32 = 4;
pub const BOUNDARY_ANALYSIS_REVISION: &str = "t0-03-boundary-passive-v4-room-loading";

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct BoundaryManifest {
    pub schema_version: u32,
    pub run_id: String,
    pub experiment_identity: String,
    pub tool_version: String,
    pub probe: String,
    pub scenario: String,
    pub case_id: String,
    pub status: String,
    pub started_utc: String,
    pub finished_utc: Option<String>,
    pub variant_id: String,
    pub game_file_sha256: String,
    pub game_text_sha256: String,
    pub boundary_ledger_sha256: String,
    pub baseline_ledger_sha256: String,
    pub mode_ledger_sha256: String,
    pub gate_decision_sha256: String,
    pub pid: Option<u32>,
    pub process_creation_time_100ns: Option<u64>,
    pub exit_code: Option<u32>,
    pub file_hashes: BTreeMap<String, String>,
    pub result_detail: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct BoundaryEventLine {
    pub schema_version: u32,
    pub event_sequence: u64,
    pub native_sequence: Option<u64>,
    pub qpc: i64,
    pub generation: u64,
    pub thread_id: u32,
    pub kind: BoundaryEventKind,
    pub depth: u32,
    pub flags: u32,
    pub return_rva: String,
    pub game_frame_before: Option<u32>,
    pub game_frame_after: Option<u32>,
    pub online_frame_before: Option<u32>,
    pub online_frame_after: Option<u32>,
    pub entity_count_before: Option<u32>,
    pub entity_count_after: Option<u32>,
    pub original_calls: u32,
    pub result_value: u32,
    pub summary_before: String,
    pub summary_after: String,
    pub error_code: u32,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct BoundaryCheck {
    pub id: String,
    pub result: String,
    pub detail: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct BoundaryAnalysisReport {
    pub schema_version: u32,
    pub analysis_revision: String,
    pub run_id: String,
    pub result: String,
    pub semantic_result: String,
    pub first_divergence: Option<String>,
    pub event_count: usize,
    pub successful_gameplay_ticks: u64,
    pub measured_window_ticks: u64,
    pub non_tick_gameplay_scopes: u64,
    pub render_only_windows: u64,
    pub lua_close_events: u64,
    pub lua_created_events: u64,
    pub lua_rebuild_retirements: u64,
    pub new_room_events: u64,
    pub suppressed_input_samples: u64,
    pub room_loading_proof: bool,
    pub checks: Vec<BoundaryCheck>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct BoundaryComparisonReport {
    pub schema_version: u32,
    pub analysis_revision: String,
    pub result: String,
    pub control_run_id: String,
    pub instrumented_run_id: String,
    pub checks: Vec<BoundaryCheck>,
}

pub struct BoundaryRun {
    pub directory: PathBuf,
    pub manifest: BoundaryManifest,
    event_file: File,
    next_event_sequence: u64,
}

pub struct BoundaryRunInputs<'a> {
    pub artifacts_root: &'a Path,
    pub case_id: &'a str,
    pub probe: &'a str,
    pub scenario: &'a str,
    pub variant_id: &'a str,
    pub game_file_sha256: &'a str,
    pub game_text_sha256: &'a str,
    pub boundary_ledger_sha256: &'a str,
    pub baseline_ledger_sha256: &'a str,
    pub mode_ledger_sha256: &'a str,
    pub gate_decision_sha256: &'a str,
}

impl BoundaryRun {
    pub fn create(inputs: &BoundaryRunInputs<'_>) -> AppResult<Self> {
        validate_token(inputs.case_id, "case id")?;
        validate_token(inputs.probe, "probe")?;
        validate_token(inputs.scenario, "scenario")?;
        let timestamp = Utc::now().format("%Y%m%dT%H%M%SZ");
        let run_id = format!(
            "{timestamp}-t0-03-boundaries-{}-{}-{}",
            inputs.probe, inputs.scenario, inputs.case_id
        );
        let directory = inputs.artifacts_root.join(&run_id);
        fs::create_dir(&directory).usage_context(format!(
            "cannot create boundary artifact directory {} (overwrite is forbidden)",
            directory.display()
        ))?;
        let event_path = directory.join("endpoint.jsonl");
        let event_file = OpenOptions::new()
            .create_new(true)
            .write(true)
            .open(&event_path)
            .usage_context(format!("cannot create {}", event_path.display()))?;
        let manifest = BoundaryManifest {
            schema_version: BOUNDARY_ARTIFACT_SCHEMA_VERSION,
            run_id,
            experiment_identity: BOUNDARY_TOOL_IDENTITY.to_owned(),
            tool_version: crate::TOOL_VERSION.to_owned(),
            probe: inputs.probe.to_owned(),
            scenario: inputs.scenario.to_owned(),
            case_id: inputs.case_id.to_owned(),
            status: "RUNNING".to_owned(),
            started_utc: utc_now(),
            finished_utc: None,
            variant_id: inputs.variant_id.to_owned(),
            game_file_sha256: inputs.game_file_sha256.to_owned(),
            game_text_sha256: inputs.game_text_sha256.to_owned(),
            boundary_ledger_sha256: inputs.boundary_ledger_sha256.to_owned(),
            baseline_ledger_sha256: inputs.baseline_ledger_sha256.to_owned(),
            mode_ledger_sha256: inputs.mode_ledger_sha256.to_owned(),
            gate_decision_sha256: inputs.gate_decision_sha256.to_owned(),
            pid: None,
            process_creation_time_100ns: None,
            exit_code: None,
            file_hashes: BTreeMap::new(),
            result_detail: "capture in progress".to_owned(),
        };
        let run = Self {
            directory,
            manifest,
            event_file,
            next_event_sequence: 1,
        };
        run.write_manifest(false)?;
        run.write_notes()?;
        Ok(run)
    }

    pub fn append(&mut self, event: &BoundaryEventLine) -> AppResult<()> {
        let mut event = event.clone();
        event.event_sequence = self.next_event_sequence;
        self.next_event_sequence = self
            .next_event_sequence
            .checked_add(1)
            .ok_or_else(|| AppError::evidence("boundary artifact event sequence overflow"))?;
        serde_json::to_writer(&mut self.event_file, &event)
            .usage_context("cannot serialize boundary event")?;
        self.event_file
            .write_all(b"\n")
            .usage_context("cannot append boundary event")?;
        self.event_file
            .flush()
            .usage_context("cannot flush boundary event")
    }

    pub fn set_process(&mut self, pid: u32, creation: u64) {
        self.manifest.pid = Some(pid);
        self.manifest.process_creation_time_100ns = Some(creation);
    }

    pub fn write_target_document(
        &self,
        name: &str,
        document: &TargetBytesDocument,
    ) -> AppResult<()> {
        if !matches!(name, "target-bytes-before.json" | "target-bytes-after.json") {
            return Err(AppError::usage("invalid boundary target document name"));
        }
        let path = self.directory.join(name);
        let mut file = OpenOptions::new()
            .create_new(true)
            .write(true)
            .open(&path)
            .usage_context(format!("cannot create {}", path.display()))?;
        serde_json::to_writer_pretty(&mut file, document)
            .usage_context("cannot serialize boundary target document")?;
        file.write_all(b"\n")
            .usage_context("cannot finalize boundary target document")
    }

    pub fn finish(&mut self, status: &str, detail: impl Into<String>) -> AppResult<()> {
        self.event_file
            .flush()
            .usage_context("cannot flush boundary events")?;
        self.manifest.status = status.to_owned();
        self.manifest.result_detail = detail.into();
        self.manifest.finished_utc = Some(utc_now());
        self.refresh_hashes()?;
        self.write_manifest(true)
    }

    fn refresh_hashes(&mut self) -> AppResult<()> {
        self.manifest.file_hashes.clear();
        for name in [
            "endpoint.jsonl",
            "notes.md",
            "target-bytes-before.json",
            "target-bytes-after.json",
        ] {
            let path = self.directory.join(name);
            if path.is_file() {
                self.manifest
                    .file_hashes
                    .insert(name.to_owned(), file_sha256(&path)?);
            }
        }
        Ok(())
    }

    fn write_manifest(&self, replace: bool) -> AppResult<()> {
        let path = self.directory.join("manifest.json");
        let mut options = OpenOptions::new();
        options.write(true);
        if replace {
            options.create(true).truncate(true);
        } else {
            options.create_new(true);
        }
        let mut file = options
            .open(&path)
            .usage_context(format!("cannot write {}", path.display()))?;
        serde_json::to_writer_pretty(&mut file, &self.manifest)
            .usage_context("cannot serialize boundary manifest")?;
        file.write_all(b"\n")
            .usage_context("cannot finalize boundary manifest")
    }

    fn write_notes(&self) -> AppResult<()> {
        let path = self.directory.join("notes.md");
        let mut file = OpenOptions::new()
            .create_new(true)
            .write(true)
            .open(&path)
            .usage_context(format!("cannot create {}", path.display()))?;
        file.write_all(
            b"# T0-03 boundary capture\n\nPassive evidence only. No input injection, update skipping, return-value mutation, payload, identity, username, absolute game path, or object pointer is recorded.\n",
        )
        .usage_context("cannot write boundary notes")
    }
}

pub fn read_manifest(run_directory: &Path) -> AppResult<BoundaryManifest> {
    let path = run_directory.join("manifest.json");
    let raw = fs::read(&path).usage_context(format!("cannot read {}", path.display()))?;
    serde_json::from_slice(&raw).usage_context(format!("cannot parse {}", path.display()))
}

pub fn read_events(run_directory: &Path) -> AppResult<Vec<BoundaryEventLine>> {
    let path = run_directory.join("endpoint.jsonl");
    let file = File::open(&path).usage_context(format!("cannot read {}", path.display()))?;
    let mut events = Vec::new();
    for (index, line) in BufReader::new(file).lines().enumerate() {
        let line = line.usage_context(format!("cannot read boundary event line {}", index + 1))?;
        let event = serde_json::from_str(&line)
            .usage_context(format!("cannot parse boundary event line {}", index + 1))?;
        events.push(event);
    }
    Ok(events)
}

pub fn analyze_boundary_run(run_directory: &Path) -> AppResult<BoundaryAnalysisReport> {
    let manifest = read_manifest(run_directory)?;
    if manifest.schema_version == 0 || manifest.schema_version > BOUNDARY_ARTIFACT_SCHEMA_VERSION {
        return Err(AppError::usage(format!(
            "unsupported boundary manifest schema {}",
            manifest.schema_version
        )));
    }
    if manifest.experiment_identity != BOUNDARY_TOOL_IDENTITY {
        return Err(AppError::usage("invalid boundary artifact identity"));
    }
    if manifest.schema_version >= 2
        && run_directory.file_name().and_then(|value| value.to_str())
            != Some(manifest.run_id.as_str())
    {
        return Err(AppError::usage(
            "boundary artifact directory name does not equal manifest runId",
        ));
    }
    let events = read_events(run_directory)?;
    let mut checks = Vec::new();
    let mut first = None;
    let mut check = |id: &str, ok: bool, detail: String| {
        if !ok && first.is_none() {
            first = Some(id.to_owned());
        }
        checks.push(BoundaryCheck {
            id: id.to_owned(),
            result: if ok { "PASS" } else { "FAIL" }.to_owned(),
            detail,
        });
    };

    check(
        "manifest-complete",
        matches!(
            manifest.status.as_str(),
            "PASS" | "FAIL" | "BLOCKED_BY_EVIDENCE"
        ),
        format!("manifest status {}", manifest.status),
    );
    let sequence_ok = events
        .iter()
        .enumerate()
        .all(|(index, event)| event.event_sequence == index as u64 + 1);
    check(
        "event-sequence",
        sequence_ok,
        format!("{} events checked for gaps", events.len()),
    );
    let event_schema_ok = events
        .iter()
        .all(|event| event.schema_version == manifest.schema_version);
    check(
        "event-schema",
        event_schema_ok,
        format!(
            "event schema matches manifest schema {}",
            manifest.schema_version
        ),
    );
    let native_sequences = events
        .iter()
        .filter_map(|event| event.native_sequence)
        .collect::<Vec<_>>();
    let native_sequence_ok = native_sequences
        .iter()
        .enumerate()
        .all(|(index, sequence)| *sequence == index as u64 + 1);
    check(
        "native-event-sequence",
        native_sequence_ok,
        format!("{} native events checked for gaps", native_sequences.len()),
    );
    let hash_ok = manifest.file_hashes.iter().all(|(name, expected)| {
        let path = run_directory.join(name);
        path.is_file()
            && file_sha256(&path).is_ok_and(|actual| actual.eq_ignore_ascii_case(expected))
    });
    check(
        "artifact-hashes",
        hash_ok && manifest.file_hashes.len() == 4,
        format!(
            "verified {} immutable artifact hashes",
            manifest.file_hashes.len()
        ),
    );
    let before: Option<TargetBytesDocument> =
        fs::read(run_directory.join("target-bytes-before.json"))
            .ok()
            .and_then(|bytes| serde_json::from_slice(&bytes).ok());
    let after: Option<TargetBytesDocument> =
        fs::read(run_directory.join("target-bytes-after.json"))
            .ok()
            .and_then(|bytes| serde_json::from_slice(&bytes).ok());
    let disk_equal = before
        .as_ref()
        .zip(after.as_ref())
        .is_some_and(|(before, after)| {
            before.disk.variant_id == after.disk.variant_id
                && before.disk.image.file_sha256 == after.disk.image.file_sha256
                && before.disk.image.text_sha256 == after.disk.image.text_sha256
        });
    check(
        "disk-image-unchanged",
        disk_equal,
        "before/after disk fingerprints are identical".to_owned(),
    );
    let (live_equal, live_detail) = before
        .as_ref()
        .zip(after.as_ref())
        .map(|(before, after)| {
            compare_boundary_live(before.process.as_ref(), after.process.as_ref())
        })
        .unwrap_or_else(|| (false, "missing target snapshots".to_owned()));
    check("live-target-bytes-unchanged", live_equal, live_detail);
    let no_failure = events.iter().all(|event| {
        event.kind != BoundaryEventKind::Failure
            && event.flags & ((1 << 0) | (1 << 1)) == 0
            && event.error_code == 0
    });
    check(
        "fail-closed-signals",
        no_failure,
        "no wrong-thread, OriginalOnline, or native failure event".to_owned(),
    );
    let return_rvas_ok = events.iter().all(|event| {
        let actual = parse_hex_u32(&event.return_rva);
        match event.kind {
            BoundaryEventKind::LuaClose => actual == Some(0x0046_E5C6),
            BoundaryEventKind::LuaCreated => {
                matches!(actual, Some(0x0046_0581) | Some(0x0046_05A9))
            }
            _ => expected_return_rva(event.kind).is_none_or(|expected| actual == Some(expected)),
        }
    });
    check(
        "return-rva",
        return_rvas_ok,
        "every scoped event resolves to its exact patched call return RVA".to_owned(),
    );

    let main_thread = events
        .iter()
        .find(|event| is_scoped_game_event(event.kind))
        .map(|event| event.thread_id);
    let thread_ok = main_thread.is_some()
        && events
            .iter()
            .filter(|event| is_game_event(event.kind))
            .all(|event| Some(event.thread_id) == main_thread);
    check(
        "main-thread",
        thread_ok || manifest.probe == "control-no-native",
        format!("confirmed boundary thread {main_thread:?}"),
    );
    let window_starts = events
        .iter()
        .filter(|event| event.kind == BoundaryEventKind::WindowStart)
        .collect::<Vec<_>>();
    let window_ends = events
        .iter()
        .filter(|event| event.kind == BoundaryEventKind::WindowEnd)
        .collect::<Vec<_>>();
    let window = window_starts
        .first()
        .zip(window_ends.first())
        .map(|(start, end)| (start.event_sequence, end.event_sequence));
    let window_ok = manifest.schema_version < 2
        || (window_starts.len() == 1
            && window_ends.len() == 1
            && window.is_some_and(|(start, end)| start < end));
    check(
        "measured-window",
        window_ok,
        format!(
            "windowStart={} windowEnd={}",
            window_starts.len(),
            window_ends.len()
        ),
    );

    let mut stack: Vec<(BoundaryEventKind, u32)> = Vec::new();
    let mut pair_ok = true;
    let mut depth_ok = true;
    let mut mutation_active = 0_u32;
    let mut remove_active = 0_u32;
    let mut spawn_active = 0_u32;
    let mut post_outside_mutation = true;
    let mut post_update_active = 0_u32;
    let mut lua_callback_inside_post_update = true;
    let mut last_input_sample_exit = None;
    let mut input_order_ok = true;
    let mut pending_apply = false;
    let mut gameplay: Vec<(u64, u32)> = Vec::new();
    let mut successful_ticks = 0_u64;
    let mut non_tick_scopes = 0_u64;
    let mut post_per_gameplay = true;
    let mut retired_generation = None;
    let mut retired_ok = true;
    let mut render_scopes = 0_u64;
    let mut post_render_scopes = 0_u64;

    for event in &events {
        if let Some(generation) = retired_generation
            && event.generation == generation
            && is_scoped_game_event(event.kind)
        {
            retired_ok = false;
        }
        match event.kind {
            BoundaryEventKind::InputSampleEnter
            | BoundaryEventKind::GameplayEnter
            | BoundaryEventKind::EntityEnter
            | BoundaryEventKind::RemoveEnter
            | BoundaryEventKind::SpawnEnter
            | BoundaryEventKind::PostUpdateEnter
            | BoundaryEventKind::RenderEnter
            | BoundaryEventKind::PostRenderEnter
            | BoundaryEventKind::OnlineDispatchEnter
            | BoundaryEventKind::LuaCallbackEnter
            | BoundaryEventKind::NewRoomEnter => {
                if event.depth != stack.len() as u32 + 1 {
                    depth_ok = false;
                }
                if event.kind == BoundaryEventKind::EntityEnter {
                    mutation_active += 1;
                }
                if event.kind == BoundaryEventKind::RemoveEnter {
                    remove_active += 1;
                }
                if event.kind == BoundaryEventKind::SpawnEnter {
                    spawn_active += 1;
                }
                if event.kind == BoundaryEventKind::PostUpdateEnter {
                    if mutation_active != 0 || remove_active != 0 || spawn_active != 0 {
                        post_outside_mutation = false;
                    }
                    post_update_active += 1;
                    if let Some(last) = gameplay.last_mut() {
                        last.1 += 1;
                    } else {
                        post_per_gameplay = false;
                    }
                }
                if event.kind == BoundaryEventKind::LuaCallbackEnter && post_update_active == 0 {
                    lua_callback_inside_post_update = false;
                }
                if event.kind == BoundaryEventKind::GameplayEnter {
                    if !pending_apply {
                        input_order_ok = false;
                    }
                    pending_apply = false;
                    last_input_sample_exit = None;
                    gameplay.push((event.event_sequence, 0));
                }
                if event.kind == BoundaryEventKind::RenderEnter {
                    render_scopes += 1;
                }
                if event.kind == BoundaryEventKind::PostRenderEnter {
                    post_render_scopes += 1;
                }
                stack.push((event.kind, event.depth));
            }
            BoundaryEventKind::InputSampleExit
            | BoundaryEventKind::GameplayExit
            | BoundaryEventKind::EntityExit
            | BoundaryEventKind::RemoveExit
            | BoundaryEventKind::SpawnExit
            | BoundaryEventKind::PostUpdateExit
            | BoundaryEventKind::RenderExit
            | BoundaryEventKind::PostRenderExit
            | BoundaryEventKind::OnlineDispatchExit
            | BoundaryEventKind::LuaCallbackExit
            | BoundaryEventKind::NewRoomExit => {
                let expected = enter_kind(event.kind);
                if stack.pop().map(|value| value.0) != expected || event.original_calls != 1 {
                    pair_ok = false;
                }
                if event.kind == BoundaryEventKind::EntityExit {
                    mutation_active = mutation_active.saturating_sub(1);
                }
                if event.kind == BoundaryEventKind::RemoveExit {
                    remove_active = remove_active.saturating_sub(1);
                }
                if event.kind == BoundaryEventKind::SpawnExit {
                    spawn_active = spawn_active.saturating_sub(1);
                }
                if event.kind == BoundaryEventKind::PostUpdateExit {
                    post_update_active = post_update_active.saturating_sub(1);
                }
                if event.kind == BoundaryEventKind::InputSampleExit {
                    last_input_sample_exit = Some(event.event_sequence);
                }
                if event.kind == BoundaryEventKind::GameplayExit {
                    let Some((_, posts)) = gameplay.pop() else {
                        post_per_gameplay = false;
                        continue;
                    };
                    if posts == 1 {
                        successful_ticks += 1;
                    } else if posts == 0 {
                        non_tick_scopes += 1;
                    } else {
                        post_per_gameplay = false;
                    }
                }
            }
            BoundaryEventKind::InputApply => {
                if last_input_sample_exit.is_none_or(|sequence| sequence >= event.event_sequence) {
                    input_order_ok = false;
                }
                pending_apply = true;
            }
            BoundaryEventKind::LuaClose | BoundaryEventKind::Retired => {
                retired_generation = Some(event.generation);
            }
            _ => {}
        }
    }
    pair_ok &= stack.is_empty() && gameplay.is_empty();
    let measured_window_ticks = window.map_or(successful_ticks, |(start, end)| {
        events
            .iter()
            .filter(|event| {
                event.event_sequence > start
                    && event.event_sequence < end
                    && event.kind == BoundaryEventKind::PostUpdateEnter
            })
            .count() as u64
    });
    let retirement_in_window = window.map_or_else(
        || {
            manifest.schema_version < 2
                && events
                    .iter()
                    .any(|event| event.kind == BoundaryEventKind::Retired)
        },
        |(start, end)| {
            events.iter().any(|event| {
                event.event_sequence > start
                    && event.event_sequence < end
                    && event.kind == BoundaryEventKind::Retired
            })
        },
    );
    let in_measured_window = |event: &&BoundaryEventLine| {
        window
            .is_some_and(|(start, end)| event.event_sequence > start && event.event_sequence < end)
    };
    let lua_closes = events
        .iter()
        .filter(|event| event.kind == BoundaryEventKind::LuaClose)
        .filter(in_measured_window)
        .collect::<Vec<_>>();
    let lua_created = events
        .iter()
        .filter(|event| event.kind == BoundaryEventKind::LuaCreated)
        .filter(in_measured_window)
        .collect::<Vec<_>>();
    let retirements = events
        .iter()
        .filter(|event| event.kind == BoundaryEventKind::Retired)
        .filter(in_measured_window)
        .collect::<Vec<_>>();
    let lua_rebuild_retirements = retirements
        .iter()
        .filter(|event| event.flags & BOUNDARY_FLAG_RETIREMENT_LUA_REBUILD != 0)
        .count() as u64;
    let lua_rebuild_proof = lua_closes
        .first()
        .zip(lua_created.first())
        .zip(retirements.first())
        .is_some_and(|((close, created), retired)| {
            lua_closes.len() == 1
                && lua_created.len() == 1
                && retirements.len() == 1
                && close.event_sequence < created.event_sequence
                && created.event_sequence < retired.event_sequence
                && close.original_calls == 1
                && created.original_calls == 1
                && matches!(created.result_value, 1 | 2)
                && close.flags & BOUNDARY_FLAG_LIFECYCLE_RETIRED != 0
                && close.flags & BOUNDARY_FLAG_LUA_ROOT_MATCH != 0
                && close.flags & BOUNDARY_FLAG_RETIREMENT_LUA_CLOSE != 0
                && created.flags & BOUNDARY_FLAG_LUA_NEW_ROOT != 0
                && retired.flags & BOUNDARY_FLAG_LIFECYCLE_RETIRED != 0
                && retired.flags & BOUNDARY_FLAG_RETIREMENT_LUA_REBUILD != 0
                && created.generation == close.generation.saturating_add(1)
                && retired.generation == close.generation
                && retired.result_value as u64 == created.generation
        });
    let input_observations = events
        .iter()
        .filter(|event| {
            matches!(
                event.kind,
                BoundaryEventKind::InputSampleEnter | BoundaryEventKind::InputSampleExit
            )
        })
        .collect::<Vec<_>>();
    let main_loop_state_complete = manifest.schema_version < 4
        || manifest.probe == "control-no-native"
        || (!input_observations.is_empty()
            && input_observations
                .iter()
                .all(|event| event.flags & BOUNDARY_FLAG_MAIN_LOOP_STATE_VALID != 0));
    let suppressed_exits = input_observations
        .iter()
        .copied()
        .filter(|event| event.kind == BoundaryEventKind::InputSampleExit)
        .filter(|event| event.flags & BOUNDARY_FLAG_GAMEPLAY_DISPATCH_SUPPRESSED != 0)
        .collect::<Vec<_>>();
    let suppressed_dispatch_ok = suppressed_exits.iter().all(|suppressed| {
        events
            .iter()
            .skip_while(|event| event.event_sequence <= suppressed.event_sequence)
            .take_while(|event| event.kind != BoundaryEventKind::InputSampleEnter)
            .all(|event| {
                !matches!(
                    event.kind,
                    BoundaryEventKind::InputApply
                        | BoundaryEventKind::GameplayEnter
                        | BoundaryEventKind::PostUpdateEnter
                )
            })
    });
    let new_room_enters = events
        .iter()
        .filter(|event| event.kind == BoundaryEventKind::NewRoomEnter)
        .filter(in_measured_window)
        .collect::<Vec<_>>();
    let new_room_exits = events
        .iter()
        .filter(|event| event.kind == BoundaryEventKind::NewRoomExit)
        .filter(in_measured_window)
        .collect::<Vec<_>>();
    let suppressed_in_window = suppressed_exits
        .iter()
        .copied()
        .filter(in_measured_window)
        .collect::<Vec<_>>();
    let room_loading_proof = new_room_enters
        .first()
        .zip(new_room_exits.first())
        .and_then(|(enter, exit)| {
            suppressed_in_window
                .iter()
                .rev()
                .find(|event| event.event_sequence < enter.event_sequence)
                .map(|suppressed| (enter, exit, *suppressed))
        })
        .is_some_and(|(enter, exit, suppressed)| {
            new_room_enters.len() == 1
                && new_room_exits.len() == 1
                && enter.event_sequence < exit.event_sequence
                && events.iter().all(|event| {
                    event.event_sequence <= suppressed.event_sequence
                        || event.event_sequence >= enter.event_sequence
                        || event.kind != BoundaryEventKind::PostUpdateEnter
                })
        });
    check(
        "enter-exit-pairs",
        pair_ok,
        "all wrapper enters/exits pair and each exit reports one original call".to_owned(),
    );
    check(
        "call-depth",
        depth_ok,
        "published call depth follows strict nesting".to_owned(),
    );
    check(
        "input-before-apply",
        input_order_ok || manifest.probe == "control-no-native",
        "input-sample exit precedes every candidate apply marker".to_owned(),
    );
    check(
        "main-loop-branch-observation",
        main_loop_state_complete && suppressed_dispatch_ok,
        format!(
            "{} input events carry the verified main-loop state; {} suppressed samples produced no candidate dispatch before the next sample",
            input_observations.len(),
            suppressed_exits.len(),
        ),
    );
    check(
        "gameplay-post-update",
        post_per_gameplay,
        format!("{successful_ticks} successful tick scopes; {non_tick_scopes} non-tick scopes"),
    );
    check(
        "post-update-outside-mutation",
        post_outside_mutation,
        "post-update never entered inside entity/spawn/remove scope".to_owned(),
    );
    check(
        "lua-callback-scope",
        lua_callback_inside_post_update,
        "every observed simulation Lua callback is nested inside post-update dispatch".to_owned(),
    );
    check(
        "retired-token",
        retired_ok,
        "no game event uses a retired lifecycle generation".to_owned(),
    );
    if manifest.scenario == "lua-rebuild" {
        check(
            "lua-rebuild-lifecycle",
            manifest.probe == "control-no-native" || lua_rebuild_proof,
            format!(
                "close={} created={} retirements={} rebuildRetirements={} orderedProof={lua_rebuild_proof}",
                lua_closes.len(),
                lua_created.len(),
                retirements.len(),
                lua_rebuild_retirements,
            ),
        );
    }
    if manifest.scenario == "room-transition" {
        check(
            "room-loading-boundary",
            manifest.probe == "control-no-native" || room_loading_proof,
            format!(
                "newRoomEnter={} newRoomExit={} suppressedWindowSamples={} roomLoadingProof={room_loading_proof}",
                new_room_enters.len(),
                new_room_exits.len(),
                suppressed_in_window.len(),
            ),
        );
    }
    let scenario_count_ok = match manifest.scenario.as_str() {
        "normal" | "input" | "low-fps" => {
            manifest.probe == "control-no-native" || measured_window_ticks > 0
        }
        "room-transition" => {
            manifest.probe == "control-no-native"
                || (measured_window_ticks > 0 && room_loading_proof)
        }
        "pause" | "focus-loss" => measured_window_ticks == 0,
        "menu-return" => manifest.probe == "control-no-native" || retirement_in_window,
        "lua-rebuild" => manifest.probe == "control-no-native" || lua_rebuild_proof,
        _ => false,
    };
    check(
        "scenario-evidence",
        scenario_count_ok,
        format!(
            "scenario {} measured {measured_window_ticks} candidate ticks; retirementInWindow={retirement_in_window}; luaRebuildProof={lua_rebuild_proof}; roomLoadingProof={room_loading_proof}",
            manifest.scenario,
        ),
    );
    let hooks_restored = manifest.probe == "control-no-native"
        || events
            .iter()
            .any(|event| event.kind == BoundaryEventKind::HooksRestored);
    check(
        "safe-restore",
        hooks_restored,
        "instrumented run ends with hook restoration".to_owned(),
    );

    let result = if first.is_none() && manifest.status == "PASS" {
        "PASS"
    } else {
        "FAIL"
    };
    let semantic_result = if result == "PASS" && successful_ticks > 0 {
        "RUNTIME_OBSERVED_BOUNDARIES; REPLICA_SEPARATION_UNKNOWN"
    } else if manifest.probe == "control-no-native" && result == "PASS" {
        "CONTROL_ONLY; BOUNDARY_SEMANTICS_UNKNOWN"
    } else {
        "UNKNOWN"
    };
    Ok(BoundaryAnalysisReport {
        schema_version: BOUNDARY_ARTIFACT_SCHEMA_VERSION,
        analysis_revision: BOUNDARY_ANALYSIS_REVISION.to_owned(),
        run_id: manifest.run_id,
        result: result.to_owned(),
        semantic_result: semantic_result.to_owned(),
        first_divergence: first,
        event_count: events.len(),
        successful_gameplay_ticks: successful_ticks,
        measured_window_ticks,
        non_tick_gameplay_scopes: non_tick_scopes,
        render_only_windows: render_scopes.saturating_sub(post_render_scopes),
        lua_close_events: lua_closes.len() as u64,
        lua_created_events: lua_created.len() as u64,
        lua_rebuild_retirements,
        new_room_events: new_room_enters.len() as u64,
        suppressed_input_samples: suppressed_exits.len() as u64,
        room_loading_proof,
        checks,
    })
}

pub fn compare_boundary_runs(
    control_directory: &Path,
    instrumented_directory: &Path,
) -> AppResult<BoundaryComparisonReport> {
    let control_manifest = read_manifest(control_directory)?;
    let instrumented_manifest = read_manifest(instrumented_directory)?;
    let control = analyze_boundary_run(control_directory)?;
    let instrumented = analyze_boundary_run(instrumented_directory)?;
    let mut checks = Vec::new();
    let same_build = control_manifest.variant_id == instrumented_manifest.variant_id
        && control_manifest.game_file_sha256 == instrumented_manifest.game_file_sha256
        && control_manifest.game_text_sha256 == instrumented_manifest.game_text_sha256;
    checks.push(BoundaryCheck {
        id: "same-game-build".to_owned(),
        result: if same_build { "PASS" } else { "FAIL" }.to_owned(),
        detail: "control and instrumented fingerprints are identical".to_owned(),
    });
    let same_evidence_baseline = control_manifest.boundary_ledger_sha256
        == instrumented_manifest.boundary_ledger_sha256
        && control_manifest.baseline_ledger_sha256 == instrumented_manifest.baseline_ledger_sha256
        && control_manifest.mode_ledger_sha256 == instrumented_manifest.mode_ledger_sha256
        && control_manifest.gate_decision_sha256 == instrumented_manifest.gate_decision_sha256;
    checks.push(BoundaryCheck {
        id: "same-evidence-baseline".to_owned(),
        result: if same_evidence_baseline {
            "PASS"
        } else {
            "FAIL"
        }
        .to_owned(),
        detail: "control and passive runs bind the same T0-01/T0-02/T0-03 evidence hashes"
            .to_owned(),
    });
    let same_scenario = control_manifest.scenario == instrumented_manifest.scenario;
    checks.push(BoundaryCheck {
        id: "same-scenario".to_owned(),
        result: if same_scenario { "PASS" } else { "FAIL" }.to_owned(),
        detail: "control and passive runs use the same named scenario".to_owned(),
    });
    let roles =
        control_manifest.probe == "control-no-native" && instrumented_manifest.probe == "passive";
    checks.push(BoundaryCheck {
        id: "probe-roles".to_owned(),
        result: if roles { "PASS" } else { "FAIL" }.to_owned(),
        detail: "comparison requires control-no-native versus passive".to_owned(),
    });
    let analyses = control.result == "PASS" && instrumented.result == "PASS";
    checks.push(BoundaryCheck {
        id: "single-run-analysis".to_owned(),
        result: if analyses { "PASS" } else { "FAIL" }.to_owned(),
        detail: "both single-run analyzers must pass".to_owned(),
    });
    let result = if checks.iter().all(|check| check.result == "PASS") {
        "PASS"
    } else {
        "FAIL"
    };
    Ok(BoundaryComparisonReport {
        schema_version: BOUNDARY_ARTIFACT_SCHEMA_VERSION,
        analysis_revision: BOUNDARY_ANALYSIS_REVISION.to_owned(),
        result: result.to_owned(),
        control_run_id: control_manifest.run_id,
        instrumented_run_id: instrumented_manifest.run_id,
        checks,
    })
}

pub fn write_boundary_analysis(
    run_directory: &Path,
    report: &BoundaryAnalysisReport,
) -> AppResult<()> {
    let primary = run_directory.join("boundary-analyzer.json");
    let output = match fs::read(&primary) {
        Ok(bytes) => {
            let existing_revision = serde_json::from_slice::<serde_json::Value>(&bytes)
                .ok()
                .and_then(|value| {
                    value
                        .get("analysisRevision")
                        .and_then(serde_json::Value::as_str)
                        .map(str::to_owned)
                });
            if existing_revision.as_deref() == Some(report.analysis_revision.as_str()) {
                return Err(AppError::usage(format!(
                    "boundary analyzer report already exists at {}",
                    primary.display()
                )));
            }
            run_directory.join(format!(
                "boundary-analyzer-{}.json",
                report.analysis_revision
            ))
        }
        Err(error) if error.kind() == std::io::ErrorKind::NotFound => primary,
        Err(error) => {
            return Err(AppError::usage(format!(
                "cannot inspect existing boundary analyzer report {}: {error}",
                primary.display()
            )));
        }
    };
    write_json_create_new(&output, report)
}

pub fn write_boundary_comparison(
    output: &Path,
    report: &BoundaryComparisonReport,
) -> AppResult<()> {
    let mut file = OpenOptions::new()
        .create_new(true)
        .write(true)
        .open(output)
        .usage_context(format!("cannot create {}", output.display()))?;
    serde_json::to_writer_pretty(&mut file, report)
        .usage_context("cannot serialize boundary comparison")?;
    file.write_all(b"\n")
        .usage_context("cannot finalize boundary comparison")
}

fn write_json_create_new(path: &Path, value: &impl Serialize) -> AppResult<()> {
    let mut file = OpenOptions::new()
        .create_new(true)
        .write(true)
        .open(path)
        .usage_context(format!("cannot write {}", path.display()))?;
    serde_json::to_writer_pretty(&mut file, value).usage_context("cannot serialize JSON")?;
    file.write_all(b"\n").usage_context("cannot finalize JSON")
}

fn compare_boundary_live(
    before: Option<&LiveProcessSnapshot>,
    after: Option<&LiveProcessSnapshot>,
) -> (bool, String) {
    let Some((before, after)) = before.zip(after) else {
        return (false, "missing before or after live snapshot".to_owned());
    };
    if before.pid != after.pid
        || before.creation_time_100ns != after.creation_time_100ns
        || before.module_base != after.module_base
        || before.module_size != after.module_size
        || before.preferred_image_base != after.preferred_image_base
        || before.load_bias != after.load_bias
    {
        return (
            false,
            "process or main-module identity changed during smoke".to_owned(),
        );
    }
    if before.debugger_present || after.debugger_present {
        return (false, "debugger was present in a live snapshot".to_owned());
    }
    if !before.forbidden_modules.is_empty() || !after.forbidden_modules.is_empty() {
        return (
            false,
            format!(
                "forbidden modules observed: before={:?}, after={:?}",
                before.forbidden_modules, after.forbidden_modules
            ),
        );
    }
    if before.targets.len() != after.targets.len() {
        return (false, "target count changed during smoke".to_owned());
    }
    for (left, right) in before.targets.iter().zip(&after.targets) {
        if left.id != right.id
            || left.rva != right.rva
            || left.address != right.address
            || left.bytes != right.bytes
            || left.semantic_result != right.semantic_result
            || left.resolved_target != right.resolved_target
            || left.resolved_owner_module != right.resolved_owner_module
        {
            return (
                false,
                format!("first target mismatch: {} vs {}", left.id, right.id),
            );
        }
    }
    let before_modules = before
        .modules
        .iter()
        .map(|module| module.name.to_ascii_lowercase())
        .collect::<std::collections::BTreeSet<_>>();
    let after_modules = after
        .modules
        .iter()
        .map(|module| module.name.to_ascii_lowercase())
        .collect::<std::collections::BTreeSet<_>>();
    let added = after_modules.difference(&before_modules).count();
    let removed = before_modules.difference(&after_modules).count();
    (
        true,
        format!(
            "{} target snapshots remained identical; ordinary module delta +{added}/-{removed} is diagnostic only",
            before.targets.len()
        ),
    )
}

fn enter_kind(exit: BoundaryEventKind) -> Option<BoundaryEventKind> {
    match exit {
        BoundaryEventKind::InputSampleExit => Some(BoundaryEventKind::InputSampleEnter),
        BoundaryEventKind::GameplayExit => Some(BoundaryEventKind::GameplayEnter),
        BoundaryEventKind::EntityExit => Some(BoundaryEventKind::EntityEnter),
        BoundaryEventKind::RemoveExit => Some(BoundaryEventKind::RemoveEnter),
        BoundaryEventKind::SpawnExit => Some(BoundaryEventKind::SpawnEnter),
        BoundaryEventKind::PostUpdateExit => Some(BoundaryEventKind::PostUpdateEnter),
        BoundaryEventKind::RenderExit => Some(BoundaryEventKind::RenderEnter),
        BoundaryEventKind::PostRenderExit => Some(BoundaryEventKind::PostRenderEnter),
        BoundaryEventKind::OnlineDispatchExit => Some(BoundaryEventKind::OnlineDispatchEnter),
        BoundaryEventKind::LuaCallbackExit => Some(BoundaryEventKind::LuaCallbackEnter),
        BoundaryEventKind::NewRoomExit => Some(BoundaryEventKind::NewRoomEnter),
        _ => None,
    }
}

fn is_game_event(kind: BoundaryEventKind) -> bool {
    !matches!(
        kind,
        BoundaryEventKind::NativeInitialized
            | BoundaryEventKind::HooksInstalled
            | BoundaryEventKind::HooksRestored
            | BoundaryEventKind::WindowStart
            | BoundaryEventKind::WindowEnd
            | BoundaryEventKind::Failure
    )
}

fn is_scoped_game_event(kind: BoundaryEventKind) -> bool {
    matches!(
        kind,
        BoundaryEventKind::InputSampleEnter
            | BoundaryEventKind::InputSampleExit
            | BoundaryEventKind::InputApply
            | BoundaryEventKind::GameplayEnter
            | BoundaryEventKind::GameplayExit
            | BoundaryEventKind::EntityEnter
            | BoundaryEventKind::EntityExit
            | BoundaryEventKind::RemoveEnter
            | BoundaryEventKind::RemoveExit
            | BoundaryEventKind::SpawnEnter
            | BoundaryEventKind::SpawnExit
            | BoundaryEventKind::PostUpdateEnter
            | BoundaryEventKind::PostUpdateExit
            | BoundaryEventKind::RenderEnter
            | BoundaryEventKind::RenderExit
            | BoundaryEventKind::PostRenderEnter
            | BoundaryEventKind::PostRenderExit
            | BoundaryEventKind::OnlineDispatchEnter
            | BoundaryEventKind::OnlineDispatchExit
            | BoundaryEventKind::LuaCallbackEnter
            | BoundaryEventKind::LuaCallbackExit
            | BoundaryEventKind::NewRoomEnter
            | BoundaryEventKind::NewRoomExit
    )
}

fn expected_return_rva(kind: BoundaryEventKind) -> Option<u32> {
    match kind {
        BoundaryEventKind::InputSampleEnter | BoundaryEventKind::InputSampleExit => {
            Some(0x0055_4F89)
        }
        BoundaryEventKind::InputApply
        | BoundaryEventKind::GameplayEnter
        | BoundaryEventKind::GameplayExit => Some(0x0055_5236),
        BoundaryEventKind::EntityEnter | BoundaryEventKind::EntityExit => Some(0x002F_B521),
        BoundaryEventKind::RemoveEnter | BoundaryEventKind::RemoveExit => Some(0x002F_BB6B),
        BoundaryEventKind::PostUpdateEnter | BoundaryEventKind::PostUpdateExit => Some(0x002F_BBAA),
        BoundaryEventKind::RenderEnter | BoundaryEventKind::RenderExit => Some(0x0055_59C6),
        BoundaryEventKind::PostRenderEnter | BoundaryEventKind::PostRenderExit => Some(0x002F_C0AB),
        BoundaryEventKind::SpawnEnter | BoundaryEventKind::SpawnExit => Some(0x0002_8B42),
        BoundaryEventKind::OnlineDispatchEnter | BoundaryEventKind::OnlineDispatchExit => {
            Some(0x0051_10B0)
        }
        BoundaryEventKind::LuaCallbackEnter | BoundaryEventKind::LuaCallbackExit => {
            Some(0x0047_29D1)
        }
        BoundaryEventKind::NewRoomEnter | BoundaryEventKind::NewRoomExit => Some(0x0033_F5BE),
        _ => None,
    }
}

fn parse_hex_u32(value: &str) -> Option<u32> {
    u32::from_str_radix(
        value
            .strip_prefix("0x")
            .or_else(|| value.strip_prefix("0X"))?,
        16,
    )
    .ok()
}

fn validate_token(value: &str, label: &str) -> AppResult<()> {
    if value.is_empty()
        || value.len() > 80
        || !value
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_'))
    {
        return Err(AppError::usage(format!("invalid {label}")));
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::artifact::{LiveProcessSnapshot, TargetBytesDocument};
    use crate::ledger::{InspectedImage, StaticInspectionReport};
    use tempfile::tempdir;

    fn event(sequence: u64, kind: BoundaryEventKind, depth: u32) -> BoundaryEventLine {
        BoundaryEventLine {
            schema_version: 1,
            event_sequence: sequence,
            native_sequence: Some(sequence),
            qpc: sequence as i64,
            generation: 1,
            thread_id: 7,
            kind,
            depth,
            flags: 0,
            return_rva: match kind {
                BoundaryEventKind::LuaClose => "0x0046E5C6".to_owned(),
                BoundaryEventKind::LuaCreated => "0x00460581".to_owned(),
                _ => expected_return_rva(kind)
                    .map_or_else(|| "0x00000000".to_owned(), |rva| format!("0x{rva:08X}")),
            },
            game_frame_before: None,
            game_frame_after: None,
            online_frame_before: None,
            online_frame_after: None,
            entity_count_before: None,
            entity_count_after: None,
            original_calls: if matches!(
                kind,
                BoundaryEventKind::InputSampleExit
                    | BoundaryEventKind::GameplayExit
                    | BoundaryEventKind::EntityExit
                    | BoundaryEventKind::RemoveExit
                    | BoundaryEventKind::SpawnExit
                    | BoundaryEventKind::PostUpdateExit
                    | BoundaryEventKind::RenderExit
                    | BoundaryEventKind::PostRenderExit
                    | BoundaryEventKind::OnlineDispatchExit
                    | BoundaryEventKind::LuaCallbackExit
                    | BoundaryEventKind::NewRoomExit
                    | BoundaryEventKind::LuaClose
                    | BoundaryEventKind::LuaCreated
            ) {
                1
            } else {
                0
            },
            result_value: 0,
            summary_before: "0000000000000000".to_owned(),
            summary_after: "0000000000000000".to_owned(),
            error_code: 0,
        }
    }

    fn fixture(events: &[BoundaryEventLine]) -> tempfile::TempDir {
        let temp = tempdir().unwrap();
        let mut event_file = File::create(temp.path().join("endpoint.jsonl")).unwrap();
        for event in events {
            serde_json::to_writer(&mut event_file, event).unwrap();
            writeln!(event_file).unwrap();
        }
        drop(event_file);
        fs::write(temp.path().join("notes.md"), "fixture\n").unwrap();
        let disk = StaticInspectionReport {
            schema_version: 1,
            result: "PASS".into(),
            ledger_id: "ledger".into(),
            ledger_sha256: "A".repeat(64),
            variant_id: "variant".into(),
            static_patch_state: "fixture".into(),
            image: InspectedImage {
                file_name: "isaac-ng.exe".into(),
                machine: "x86".into(),
                timestamp: "0x00000000".into(),
                image_size: "0x00000000".into(),
                preferred_image_base: "0x00400000".into(),
                entry_point_rva: "0x00000000".into(),
                file_sha256: "1".repeat(64),
                text_sha256: "2".repeat(64),
                sections: Vec::new(),
            },
            targets: Vec::new(),
        };
        let live = LiveProcessSnapshot {
            pid: 1,
            creation_time_100ns: 1,
            module_base: "0x00400000".into(),
            module_size: 1,
            preferred_image_base: "0x00400000".into(),
            load_bias: "0x0".into(),
            debugger_present: false,
            forbidden_modules: Vec::new(),
            modules: Vec::new(),
            targets: Vec::new(),
            sampled_qpc: 1,
        };
        for (name, phase) in [
            ("target-bytes-before.json", "before"),
            ("target-bytes-after.json", "after"),
        ] {
            fs::write(
                temp.path().join(name),
                serde_json::to_vec(&TargetBytesDocument {
                    schema_version: 1,
                    phase: phase.into(),
                    captured_utc: utc_now(),
                    disk: disk.clone(),
                    process: Some(live.clone()),
                })
                .unwrap(),
            )
            .unwrap();
        }
        let file_hashes = [
            "endpoint.jsonl",
            "notes.md",
            "target-bytes-before.json",
            "target-bytes-after.json",
        ]
        .into_iter()
        .map(|name| {
            (
                name.to_owned(),
                file_sha256(&temp.path().join(name)).unwrap(),
            )
        })
        .collect();
        let manifest = BoundaryManifest {
            schema_version: 1,
            run_id: temp
                .path()
                .file_name()
                .unwrap()
                .to_string_lossy()
                .into_owned(),
            experiment_identity: BOUNDARY_TOOL_IDENTITY.into(),
            tool_version: "test".into(),
            probe: "passive".into(),
            scenario: "normal".into(),
            case_id: "case".into(),
            status: "PASS".into(),
            started_utc: utc_now(),
            finished_utc: Some(utc_now()),
            variant_id: "variant".into(),
            game_file_sha256: "A".repeat(64),
            game_text_sha256: "B".repeat(64),
            boundary_ledger_sha256: "C".repeat(64),
            baseline_ledger_sha256: "D".repeat(64),
            mode_ledger_sha256: "E".repeat(64),
            gate_decision_sha256: "F".repeat(64),
            pid: Some(1),
            process_creation_time_100ns: Some(1),
            exit_code: Some(0),
            file_hashes,
            result_detail: "fixture".into(),
        };
        fs::write(
            temp.path().join("manifest.json"),
            serde_json::to_vec(&manifest).unwrap(),
        )
        .unwrap();
        temp
    }

    fn set_scenario(directory: &Path, scenario: &str) {
        let path = directory.join("manifest.json");
        let mut manifest: BoundaryManifest =
            serde_json::from_slice(&fs::read(&path).unwrap()).unwrap();
        manifest.scenario = scenario.to_owned();
        fs::write(path, serde_json::to_vec(&manifest).unwrap()).unwrap();
    }

    fn set_schema(directory: &Path, schema_version: u32) {
        let path = directory.join("manifest.json");
        let mut manifest: BoundaryManifest =
            serde_json::from_slice(&fs::read(&path).unwrap()).unwrap();
        manifest.schema_version = schema_version;
        fs::write(path, serde_json::to_vec(&manifest).unwrap()).unwrap();
    }

    #[test]
    fn analyzer_accepts_nested_tick_and_input_order() {
        let kinds = [
            (BoundaryEventKind::NativeInitialized, 0),
            (BoundaryEventKind::HooksInstalled, 0),
            (BoundaryEventKind::InputSampleEnter, 1),
            (BoundaryEventKind::InputSampleExit, 1),
            (BoundaryEventKind::InputApply, 0),
            (BoundaryEventKind::GameplayEnter, 1),
            (BoundaryEventKind::EntityEnter, 2),
            (BoundaryEventKind::EntityExit, 2),
            (BoundaryEventKind::SpawnEnter, 2),
            (BoundaryEventKind::SpawnExit, 2),
            (BoundaryEventKind::RemoveEnter, 2),
            (BoundaryEventKind::RemoveExit, 2),
            (BoundaryEventKind::PostUpdateEnter, 2),
            (BoundaryEventKind::LuaCallbackEnter, 3),
            (BoundaryEventKind::LuaCallbackExit, 3),
            (BoundaryEventKind::PostUpdateExit, 2),
            (BoundaryEventKind::GameplayExit, 1),
            (BoundaryEventKind::HooksRestored, 0),
        ];
        let events = kinds
            .iter()
            .enumerate()
            .map(|(index, (kind, depth))| event(index as u64 + 1, *kind, *depth))
            .collect::<Vec<_>>();
        let temp = fixture(&events);
        let report = analyze_boundary_run(temp.path()).unwrap();
        assert_eq!(report.result, "PASS");
        assert_eq!(report.successful_gameplay_ticks, 1);
    }

    #[test]
    fn schema_v2_requires_markers_and_preserves_native_sequence() {
        let kinds = [
            BoundaryEventKind::NativeInitialized,
            BoundaryEventKind::InputSampleEnter,
            BoundaryEventKind::InputSampleExit,
            BoundaryEventKind::WindowStart,
            BoundaryEventKind::InputApply,
            BoundaryEventKind::GameplayEnter,
            BoundaryEventKind::PostUpdateEnter,
            BoundaryEventKind::PostUpdateExit,
            BoundaryEventKind::GameplayExit,
            BoundaryEventKind::WindowEnd,
            BoundaryEventKind::HooksRestored,
        ];
        let mut native_sequence = 0_u64;
        let events = kinds
            .iter()
            .enumerate()
            .map(|(index, kind)| {
                let depth = match kind {
                    BoundaryEventKind::InputSampleEnter
                    | BoundaryEventKind::InputSampleExit
                    | BoundaryEventKind::GameplayEnter
                    | BoundaryEventKind::GameplayExit => 1,
                    BoundaryEventKind::PostUpdateEnter | BoundaryEventKind::PostUpdateExit => 2,
                    _ => 0,
                };
                let mut event = event(index as u64 + 1, *kind, depth);
                event.schema_version = 2;
                if matches!(
                    kind,
                    BoundaryEventKind::WindowStart | BoundaryEventKind::WindowEnd
                ) {
                    event.native_sequence = None;
                } else {
                    native_sequence += 1;
                    event.native_sequence = Some(native_sequence);
                }
                event
            })
            .collect::<Vec<_>>();
        let temp = fixture(&events);
        set_schema(temp.path(), 2);
        let report = analyze_boundary_run(temp.path()).unwrap();
        assert_eq!(report.result, "PASS");
        assert_eq!(report.measured_window_ticks, 1);

        let without_markers = events
            .into_iter()
            .filter(|event| {
                !matches!(
                    event.kind,
                    BoundaryEventKind::WindowStart | BoundaryEventKind::WindowEnd
                )
            })
            .enumerate()
            .map(|(index, mut event)| {
                event.event_sequence = index as u64 + 1;
                event
            })
            .collect::<Vec<_>>();
        let missing = fixture(&without_markers);
        set_schema(missing.path(), 2);
        let report = analyze_boundary_run(missing.path()).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(
            report
                .checks
                .iter()
                .any(|check| { check.id == "measured-window" && check.result == "FAIL" })
        );
    }

    #[test]
    fn analyzer_rejects_gap_wrong_thread_order_and_mutation_overlap() {
        let mut events = vec![
            event(1, BoundaryEventKind::NativeInitialized, 0),
            event(2, BoundaryEventKind::InputApply, 0),
            event(4, BoundaryEventKind::GameplayEnter, 1),
            event(5, BoundaryEventKind::EntityEnter, 2),
            event(6, BoundaryEventKind::PostUpdateEnter, 3),
            event(7, BoundaryEventKind::PostUpdateExit, 3),
            event(8, BoundaryEventKind::EntityExit, 2),
            event(9, BoundaryEventKind::GameplayExit, 1),
            event(10, BoundaryEventKind::HooksRestored, 0),
        ];
        events[4].thread_id = 8;
        let temp = fixture(&events);
        let report = analyze_boundary_run(temp.path()).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(
            report.checks.iter().any(|check| {
                check.id == "post-update-outside-mutation" && check.result == "FAIL"
            })
        );
    }

    #[test]
    fn analyzer_rejects_tick_during_pause_and_bad_depth() {
        let kinds = [
            (BoundaryEventKind::NativeInitialized, 0),
            (BoundaryEventKind::InputSampleEnter, 1),
            (BoundaryEventKind::InputSampleExit, 1),
            (BoundaryEventKind::InputApply, 0),
            (BoundaryEventKind::GameplayEnter, 1),
            (BoundaryEventKind::PostUpdateEnter, 3),
            (BoundaryEventKind::PostUpdateExit, 3),
            (BoundaryEventKind::GameplayExit, 1),
            (BoundaryEventKind::HooksRestored, 0),
        ];
        let events = kinds
            .iter()
            .enumerate()
            .map(|(index, (kind, depth))| event(index as u64 + 1, *kind, *depth))
            .collect::<Vec<_>>();
        let temp = fixture(&events);
        set_scenario(temp.path(), "pause");
        let report = analyze_boundary_run(temp.path()).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(
            report
                .checks
                .iter()
                .any(|check| { check.id == "call-depth" && check.result == "FAIL" })
        );
        assert!(
            report
                .checks
                .iter()
                .any(|check| { check.id == "scenario-evidence" && check.result == "FAIL" })
        );
    }

    #[test]
    fn analyzer_requires_new_room_and_suppressed_dispatch_without_loading_tick() {
        let kinds = [
            (BoundaryEventKind::NativeInitialized, 0),
            (BoundaryEventKind::HooksInstalled, 0),
            (BoundaryEventKind::WindowStart, 0),
            (BoundaryEventKind::InputSampleEnter, 1),
            (BoundaryEventKind::InputSampleExit, 1),
            (BoundaryEventKind::InputApply, 0),
            (BoundaryEventKind::GameplayEnter, 1),
            (BoundaryEventKind::PostUpdateEnter, 2),
            (BoundaryEventKind::PostUpdateExit, 2),
            (BoundaryEventKind::GameplayExit, 1),
            (BoundaryEventKind::InputSampleEnter, 1),
            (BoundaryEventKind::InputSampleExit, 1),
            (BoundaryEventKind::NewRoomEnter, 1),
            (BoundaryEventKind::NewRoomExit, 1),
            (BoundaryEventKind::WindowEnd, 0),
            (BoundaryEventKind::HooksRestored, 0),
        ];
        let mut native_sequence = 0_u64;
        let events = kinds
            .iter()
            .enumerate()
            .map(|(index, (kind, depth))| {
                let mut event = event(index as u64 + 1, *kind, *depth);
                event.schema_version = 4;
                if matches!(
                    kind,
                    BoundaryEventKind::WindowStart | BoundaryEventKind::WindowEnd
                ) {
                    event.native_sequence = None;
                } else {
                    native_sequence += 1;
                    event.native_sequence = Some(native_sequence);
                }
                if matches!(
                    kind,
                    BoundaryEventKind::InputSampleEnter | BoundaryEventKind::InputSampleExit
                ) {
                    event.flags |= BOUNDARY_FLAG_MAIN_LOOP_STATE_VALID;
                    event.result_value = 2;
                    if index >= 10 {
                        event.flags |= BOUNDARY_FLAG_GAMEPLAY_DISPATCH_SUPPRESSED;
                    }
                }
                event
            })
            .collect::<Vec<_>>();
        let temp = fixture(&events);
        set_scenario(temp.path(), "room-transition");
        set_schema(temp.path(), 4);
        let report = analyze_boundary_run(temp.path()).unwrap();
        assert_eq!(report.result, "PASS");
        assert!(report.room_loading_proof);
        assert_eq!(report.new_room_events, 1);
        assert_eq!(report.suppressed_input_samples, 1);

        let mut missing_suppression = events.clone();
        for event in &mut missing_suppression {
            event.flags &= !BOUNDARY_FLAG_GAMEPLAY_DISPATCH_SUPPRESSED;
        }
        let missing = fixture(&missing_suppression);
        set_scenario(missing.path(), "room-transition");
        set_schema(missing.path(), 4);
        let report = analyze_boundary_run(missing.path()).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(
            report
                .checks
                .iter()
                .any(|check| { check.id == "room-loading-boundary" && check.result == "FAIL" })
        );

        let mut dispatch_during_suppression = events.clone();
        let apply = event(0, BoundaryEventKind::InputApply, 0);
        dispatch_during_suppression.insert(12, apply);
        let mut next_native = 0_u64;
        for (index, event) in dispatch_during_suppression.iter_mut().enumerate() {
            event.schema_version = 4;
            event.event_sequence = index as u64 + 1;
            if matches!(
                event.kind,
                BoundaryEventKind::WindowStart | BoundaryEventKind::WindowEnd
            ) {
                event.native_sequence = None;
            } else {
                next_native += 1;
                event.native_sequence = Some(next_native);
            }
        }
        let invalid = fixture(&dispatch_during_suppression);
        set_scenario(invalid.path(), "room-transition");
        set_schema(invalid.path(), 4);
        let report = analyze_boundary_run(invalid.path()).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(
            report.checks.iter().any(|check| {
                check.id == "main-loop-branch-observation" && check.result == "FAIL"
            })
        );
    }

    #[test]
    fn analyzer_requires_ordered_lua_rebuild_and_rejects_old_token_events() {
        let base = [
            (BoundaryEventKind::NativeInitialized, 0),
            (BoundaryEventKind::InputSampleEnter, 1),
            (BoundaryEventKind::InputSampleExit, 1),
            (BoundaryEventKind::WindowStart, 0),
            (BoundaryEventKind::LuaClose, 0),
            (BoundaryEventKind::LuaCreated, 0),
            (BoundaryEventKind::Retired, 0),
            (BoundaryEventKind::WindowEnd, 0),
            (BoundaryEventKind::HooksRestored, 0),
        ];
        let mut native_sequence = 0_u64;
        let events = base
            .iter()
            .enumerate()
            .map(|(index, (kind, depth))| {
                let mut event = event(index as u64 + 1, *kind, *depth);
                event.schema_version = 3;
                if matches!(
                    kind,
                    BoundaryEventKind::WindowStart | BoundaryEventKind::WindowEnd
                ) {
                    event.native_sequence = None;
                } else {
                    native_sequence += 1;
                    event.native_sequence = Some(native_sequence);
                }
                match kind {
                    BoundaryEventKind::LuaClose => {
                        event.flags = BOUNDARY_FLAG_LIFECYCLE_RETIRED
                            | BOUNDARY_FLAG_LUA_ROOT_MATCH
                            | BOUNDARY_FLAG_RETIREMENT_LUA_CLOSE;
                    }
                    BoundaryEventKind::LuaCreated => {
                        event.flags = BOUNDARY_FLAG_LUA_NEW_ROOT;
                        event.generation = 2;
                        event.result_value = 1;
                    }
                    BoundaryEventKind::Retired => {
                        event.flags =
                            BOUNDARY_FLAG_LIFECYCLE_RETIRED | BOUNDARY_FLAG_RETIREMENT_LUA_REBUILD;
                        event.result_value = 2;
                    }
                    _ => {}
                }
                event
            })
            .collect::<Vec<_>>();
        let temp = fixture(&events);
        set_scenario(temp.path(), "lua-rebuild");
        set_schema(temp.path(), 3);
        let report = analyze_boundary_run(temp.path()).unwrap();
        assert_eq!(report.result, "PASS");
        assert_eq!(report.lua_close_events, 1);
        assert_eq!(report.lua_created_events, 1);
        assert_eq!(report.lua_rebuild_retirements, 1);

        let mut revived = events.clone();
        let mut enter = event(8, BoundaryEventKind::RenderEnter, 1);
        enter.schema_version = 3;
        let mut exit = event(9, BoundaryEventKind::RenderExit, 1);
        exit.schema_version = 3;
        revived.insert(7, enter);
        revived.insert(8, exit);
        let mut next_native = 0_u64;
        for (index, event) in revived.iter_mut().enumerate() {
            event.event_sequence = index as u64 + 1;
            if matches!(
                event.kind,
                BoundaryEventKind::WindowStart | BoundaryEventKind::WindowEnd
            ) {
                event.native_sequence = None;
            } else {
                next_native += 1;
                event.native_sequence = Some(next_native);
            }
        }
        let revived_temp = fixture(&revived);
        set_scenario(revived_temp.path(), "lua-rebuild");
        set_schema(revived_temp.path(), 3);
        let report = analyze_boundary_run(revived_temp.path()).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(
            report
                .checks
                .iter()
                .any(|check| { check.id == "retired-token" && check.result == "FAIL" })
        );

        let missing_creator = events
            .iter()
            .filter(|event| event.kind != BoundaryEventKind::LuaCreated)
            .cloned()
            .collect::<Vec<_>>();
        let mut missing_creator = missing_creator;
        let mut next_native = 0_u64;
        for (index, event) in missing_creator.iter_mut().enumerate() {
            event.event_sequence = index as u64 + 1;
            if matches!(
                event.kind,
                BoundaryEventKind::WindowStart | BoundaryEventKind::WindowEnd
            ) {
                event.native_sequence = None;
            } else {
                next_native += 1;
                event.native_sequence = Some(next_native);
            }
        }
        let missing_temp = fixture(&missing_creator);
        set_scenario(missing_temp.path(), "lua-rebuild");
        set_schema(missing_temp.path(), 3);
        let report = analyze_boundary_run(missing_temp.path()).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(
            report
                .checks
                .iter()
                .any(|check| { check.id == "lua-rebuild-lifecycle" && check.result == "FAIL" })
        );

        let mut wrong_reason = events.clone();
        wrong_reason
            .iter_mut()
            .find(|event| event.kind == BoundaryEventKind::Retired)
            .unwrap()
            .flags = 0;
        let wrong_reason_temp = fixture(&wrong_reason);
        set_scenario(wrong_reason_temp.path(), "lua-rebuild");
        set_schema(wrong_reason_temp.path(), 3);
        let report = analyze_boundary_run(wrong_reason_temp.path()).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(
            report
                .checks
                .iter()
                .any(|check| { check.id == "lua-rebuild-lifecycle" && check.result == "FAIL" })
        );

        let mut wrong_order = events.clone();
        wrong_order.swap(4, 5);
        let mut next_native = 0_u64;
        for (index, event) in wrong_order.iter_mut().enumerate() {
            event.event_sequence = index as u64 + 1;
            if event.native_sequence.is_some() {
                next_native += 1;
                event.native_sequence = Some(next_native);
            }
        }
        let wrong_order_temp = fixture(&wrong_order);
        set_scenario(wrong_order_temp.path(), "lua-rebuild");
        set_schema(wrong_order_temp.path(), 3);
        let report = analyze_boundary_run(wrong_order_temp.path()).unwrap();
        assert_eq!(report.result, "FAIL");
        assert!(
            report
                .checks
                .iter()
                .any(|check| { check.id == "lua-rebuild-lifecycle" && check.result == "FAIL" })
        );
    }

    #[test]
    fn boundary_live_comparison_allows_ordinary_lazy_module_loads() {
        let temp = fixture(&[]);
        let document: TargetBytesDocument = serde_json::from_slice(
            &fs::read(temp.path().join("target-bytes-before.json")).unwrap(),
        )
        .unwrap();
        let before = document.process.unwrap();
        let mut after = before.clone();
        after.modules.push(crate::artifact::ModuleSnapshot {
            name: "d3d11.dll".into(),
            base: "0x70000000".into(),
            size: 0x1000,
        });
        let (ok, detail) = compare_boundary_live(Some(&before), Some(&after));
        assert!(ok);
        assert!(detail.contains("module delta +1/-0"));
    }

    #[test]
    fn boundary_live_comparison_rejects_debuggers_and_forbidden_modules() {
        let temp = fixture(&[]);
        let document: TargetBytesDocument = serde_json::from_slice(
            &fs::read(temp.path().join("target-bytes-before.json")).unwrap(),
        )
        .unwrap();
        let before = document.process.unwrap();

        let mut debugger = before.clone();
        debugger.debugger_present = true;
        assert!(!compare_boundary_live(Some(&before), Some(&debugger)).0);

        let mut forbidden = before.clone();
        forbidden
            .forbidden_modules
            .push("isaacsync.native.dll".into());
        assert!(!compare_boundary_live(Some(&before), Some(&forbidden)).0);
    }

    #[test]
    fn boundary_live_comparison_rejects_target_changes() {
        let temp = fixture(&[]);
        let mut document: TargetBytesDocument = serde_json::from_slice(
            &fs::read(temp.path().join("target-bytes-before.json")).unwrap(),
        )
        .unwrap();
        let mut before = document.process.take().unwrap();
        before.targets.push(crate::artifact::LiveTargetSnapshot {
            id: "gameplay-update-call".into(),
            rva: "0x00555231".into(),
            address: "0x00955231".into(),
            bytes: "E88A5BDAFF".into(),
            semantic_result: "PASS".into(),
            resolved_target: Some("0x006FADC0".into()),
            resolved_owner_module: Some("isaac-ng.exe".into()),
        });
        let mut after = before.clone();
        after.targets[0].bytes = "E900000000".into();
        assert!(!compare_boundary_live(Some(&before), Some(&after)).0);
    }

    #[test]
    fn analysis_writer_preserves_existing_reports_and_refuses_duplicate_revision() {
        let temp = fixture(&[]);
        let report = analyze_boundary_run(temp.path()).unwrap();
        let primary = temp.path().join("boundary-analyzer.json");
        let legacy = b"{\"analysisRevision\":\"legacy\"}\n";
        fs::write(&primary, legacy).unwrap();

        write_boundary_analysis(temp.path(), &report).unwrap();
        assert_eq!(fs::read(&primary).unwrap(), legacy);

        let versioned = temp.path().join(format!(
            "boundary-analyzer-{}.json",
            BOUNDARY_ANALYSIS_REVISION
        ));
        let first_versioned = fs::read(&versioned).unwrap();
        assert!(write_boundary_analysis(temp.path(), &report).is_err());
        assert_eq!(fs::read(&versioned).unwrap(), first_versioned);
    }

    #[test]
    fn artifact_creation_forbids_overwrite_and_identifying_tokens() {
        let temp = tempdir().unwrap();
        let inputs = BoundaryRunInputs {
            artifacts_root: temp.path(),
            case_id: "safe-case",
            probe: "passive",
            scenario: "normal",
            variant_id: "v",
            game_file_sha256: &"A".repeat(64),
            game_text_sha256: &"B".repeat(64),
            boundary_ledger_sha256: &"C".repeat(64),
            baseline_ledger_sha256: &"D".repeat(64),
            mode_ledger_sha256: &"E".repeat(64),
            gate_decision_sha256: &"F".repeat(64),
        };
        let run = BoundaryRun::create(&inputs).unwrap();
        assert!(!run.manifest.result_detail.contains("\\"));
        assert!(validate_token("user/name", "case id").is_err());
        assert!(fs::create_dir(&run.directory).is_err());
    }
}
