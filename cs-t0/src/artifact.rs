use std::collections::BTreeMap;
use std::fs::{self, File, OpenOptions};
use std::io::{BufRead, BufReader, BufWriter, Write};
use std::path::{Path, PathBuf};

use chrono::{DateTime, SecondsFormat, Utc};
use serde::{Deserialize, Serialize};

use crate::error::{AppError, AppResult, UsageContext};
use crate::ledger::StaticInspectionReport;
use crate::pe::sha256_hex;
use crate::{TOOL_IDENTITY, TOOL_VERSION};

pub const ARTIFACT_SCHEMA_VERSION: u32 = 1;

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct RunManifest {
    pub schema_version: u32,
    pub run_id: String,
    pub case_id: String,
    pub experiment_identity: String,
    pub status: String,
    pub started_utc: String,
    pub ended_utc: Option<String>,
    pub end_reason: Option<String>,
    pub game: GameManifest,
    pub tool: ToolManifest,
    pub target_ledger: LedgerManifest,
    pub process: Option<ProcessManifest>,
    pub installed_hooks: Vec<String>,
    pub native_probe_installed: bool,
    pub safe_unload: String,
    pub transport: TransportManifest,
    pub input_script: Option<String>,
    pub run_seed: Option<String>,
    pub mod_summary: ModSummary,
    pub artifact_hashes: BTreeMap<String, String>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct GameManifest {
    pub file_name: String,
    pub machine: String,
    pub timestamp: String,
    pub image_size: String,
    pub file_sha256: String,
    pub text_sha256: String,
    pub variant_id: String,
    pub static_patch_state: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ToolManifest {
    pub identity: String,
    pub version: String,
    pub executable_sha256: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LedgerManifest {
    pub id: String,
    pub sha256: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ProcessManifest {
    pub pid: u32,
    pub creation_time_100ns: u64,
    pub role: String,
    pub module_base: String,
    pub creation_time_utc: Option<String>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct TransportManifest {
    pub kind: String,
    pub delay_ms: u32,
    pub jitter_ms: u32,
    pub loss_percent: f64,
    pub duplicate_percent: f64,
    pub reorder_percent: f64,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ModSummary {
    pub identifiers_included: bool,
    pub static_online_mod_patch: String,
    pub runtime_module_names_recorded: bool,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CommonEvent {
    pub schema_version: u32,
    pub event_sequence: u64,
    pub qpc: i64,
    pub thread_id: u32,
    pub process_role: String,
    pub network_mode: String,
    pub session_id: Option<String>,
    pub server_tick: Option<u64>,
    pub game_frame: Option<u32>,
    pub online_frame: Option<u32>,
    pub event_kind: String,
    pub result: String,
    pub error_code: Option<String>,
    pub detail: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub observed_pid: Option<u32>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct TargetBytesDocument {
    pub schema_version: u32,
    pub phase: String,
    pub captured_utc: String,
    pub disk: StaticInspectionReport,
    pub process: Option<LiveProcessSnapshot>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LiveProcessSnapshot {
    pub pid: u32,
    pub creation_time_100ns: u64,
    pub module_base: String,
    pub module_size: u32,
    pub preferred_image_base: String,
    pub load_bias: String,
    pub debugger_present: bool,
    pub forbidden_modules: Vec<String>,
    pub modules: Vec<ModuleSnapshot>,
    pub targets: Vec<LiveTargetSnapshot>,
    pub sampled_qpc: i64,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ModuleSnapshot {
    pub name: String,
    pub base: String,
    pub size: u32,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LiveTargetSnapshot {
    pub id: String,
    pub rva: String,
    pub address: String,
    pub bytes: String,
    pub semantic_result: String,
    pub resolved_target: Option<String>,
    pub resolved_owner_module: Option<String>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct AnalyzerReport {
    pub schema_version: u32,
    pub run_id: String,
    pub case_id: String,
    pub result: String,
    pub evidence_label: String,
    pub checks: Vec<AnalyzerCheck>,
    pub first_divergence: Option<String>,
    pub generated_utc: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct AnalyzerCheck {
    pub id: String,
    pub result: String,
    pub detail: String,
}

pub struct ArtifactRun {
    pub directory: PathBuf,
    pub manifest: RunManifest,
    authority: EventLogWriter,
}

pub struct EventLogWriter {
    path: PathBuf,
    writer: BufWriter<File>,
    next_sequence: u64,
}

impl ArtifactRun {
    pub fn create(
        artifacts_root: &Path,
        case_id: &str,
        inspection: &StaticInspectionReport,
        tool_sha256: String,
    ) -> AppResult<Self> {
        validate_case_id(case_id)?;
        fs::create_dir_all(artifacts_root).usage_context(format!(
            "cannot create artifacts root {}",
            artifacts_root.display()
        ))?;
        let run_id = format!("{}-{case_id}", Utc::now().format("%Y%m%dT%H%M%SZ"));
        let directory = artifacts_root.join(&run_id);
        fs::create_dir(&directory).usage_context(format!(
            "cannot create new artifact directory {}",
            directory.display()
        ))?;
        let authority = EventLogWriter::create(&directory.join("authority.jsonl"))?;
        create_empty(&directory.join("client.jsonl"))?;
        let manifest = RunManifest {
            schema_version: ARTIFACT_SCHEMA_VERSION,
            run_id,
            case_id: case_id.to_owned(),
            experiment_identity: TOOL_IDENTITY.to_owned(),
            status: "RUNNING".to_owned(),
            started_utc: utc_now(),
            ended_utc: None,
            end_reason: None,
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
                identity: TOOL_IDENTITY.to_owned(),
                version: TOOL_VERSION.to_owned(),
                executable_sha256: tool_sha256,
            },
            target_ledger: LedgerManifest {
                id: inspection.ledger_id.clone(),
                sha256: inspection.ledger_sha256.clone(),
            },
            process: None,
            installed_hooks: Vec::new(),
            native_probe_installed: false,
            safe_unload: "NOT_APPLICABLE_READ_ONLY".to_owned(),
            transport: TransportManifest {
                kind: "NONE_READ_ONLY".to_owned(),
                delay_ms: 0,
                jitter_ms: 0,
                loss_percent: 0.0,
                duplicate_percent: 0.0,
                reorder_percent: 0.0,
            },
            input_script: None,
            run_seed: None,
            mod_summary: ModSummary {
                identifiers_included: false,
                static_online_mod_patch: inspection.static_patch_state.clone(),
                runtime_module_names_recorded: true,
            },
            artifact_hashes: BTreeMap::new(),
        };
        let run = Self {
            directory,
            manifest,
            authority,
        };
        run.write_manifest()?;
        Ok(run)
    }

    #[allow(clippy::too_many_arguments)]
    pub fn event(
        &mut self,
        qpc: i64,
        thread_id: u32,
        kind: &str,
        result: &str,
        error_code: Option<&str>,
        detail: impl Into<String>,
        observed_pid: Option<u32>,
    ) -> AppResult<()> {
        self.authority.write(CommonEvent {
            schema_version: ARTIFACT_SCHEMA_VERSION,
            event_sequence: 0,
            qpc,
            thread_id,
            process_role: "EvidenceObserver".to_owned(),
            network_mode: "Unresolved".to_owned(),
            session_id: None,
            server_tick: None,
            game_frame: None,
            online_frame: None,
            event_kind: kind.to_owned(),
            result: result.to_owned(),
            error_code: error_code.map(str::to_owned),
            detail: detail.into(),
            observed_pid,
        })
    }

    pub fn write_target_document(
        &self,
        name: &str,
        document: &TargetBytesDocument,
    ) -> AppResult<()> {
        write_pretty_json(&self.directory.join(name), document)
    }

    pub fn set_process(
        &mut self,
        process: &LiveProcessSnapshot,
        creation_time_utc: Option<String>,
    ) {
        self.manifest.process = Some(ProcessManifest {
            pid: process.pid,
            creation_time_100ns: process.creation_time_100ns,
            role: "ObservedTarget".to_owned(),
            module_base: process.module_base.clone(),
            creation_time_utc,
        });
    }

    pub fn finish(&mut self, status: &str, reason: impl Into<String>) -> AppResult<()> {
        self.authority.flush()?;
        self.manifest.status = status.to_owned();
        self.manifest.ended_utc = Some(utc_now());
        self.manifest.end_reason = Some(reason.into());
        for name in [
            "authority.jsonl",
            "client.jsonl",
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

impl EventLogWriter {
    pub fn create(path: &Path) -> AppResult<Self> {
        let file = OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(path)
            .usage_context(format!("cannot create event log {}", path.display()))?;
        Ok(Self {
            path: path.to_path_buf(),
            writer: BufWriter::new(file),
            next_sequence: 1,
        })
    }

    pub fn write(&mut self, mut event: CommonEvent) -> AppResult<()> {
        event.event_sequence = self.next_sequence;
        serde_json::to_writer(&mut self.writer, &event).usage_context(format!(
            "cannot serialize event for {}",
            self.path.display()
        ))?;
        self.writer
            .write_all(b"\n")
            .usage_context(format!("cannot write event log {}", self.path.display()))?;
        self.writer
            .flush()
            .usage_context(format!("cannot flush event log {}", self.path.display()))?;
        self.next_sequence = self
            .next_sequence
            .checked_add(1)
            .ok_or_else(|| AppError::evidence("event sequence overflow"))?;
        Ok(())
    }

    pub fn flush(&mut self) -> AppResult<()> {
        self.writer
            .flush()
            .usage_context(format!("cannot flush event log {}", self.path.display()))
    }
}

pub fn analyze_run(run_directory: &Path) -> AppResult<AnalyzerReport> {
    let manifest: RunManifest = read_json(&run_directory.join("manifest.json"))?;
    if manifest.schema_version != ARTIFACT_SCHEMA_VERSION {
        return Err(AppError::usage(format!(
            "unsupported manifest schema {}",
            manifest.schema_version
        )));
    }
    if run_directory.file_name().and_then(|name| name.to_str()) != Some(&manifest.run_id) {
        return Err(AppError::usage(
            "artifact directory name does not equal manifest runId",
        ));
    }
    let mut checks = Vec::new();
    let mut first_divergence = None;
    let mut push_check = |id: &str, passed: bool, detail: String| {
        if !passed && first_divergence.is_none() {
            first_divergence = Some(id.to_owned());
        }
        checks.push(AnalyzerCheck {
            id: id.to_owned(),
            result: if passed { "PASS" } else { "FAIL" }.to_owned(),
            detail,
        });
    };

    let sequence = validate_event_sequence(&run_directory.join("authority.jsonl"))?;
    push_check(
        "authority-event-sequence",
        sequence.gaps == 0 && sequence.events > 0,
        format!("events={}, gaps={}", sequence.events, sequence.gaps),
    );
    let client_bytes =
        fs::read(run_directory.join("client.jsonl")).usage_context("cannot read client.jsonl")?;
    push_check(
        "single-endpoint-client-log",
        client_bytes.is_empty(),
        format!("client.jsonl bytes={}", client_bytes.len()),
    );

    let before: TargetBytesDocument = read_json(&run_directory.join("target-bytes-before.json"))?;
    let after: TargetBytesDocument = read_json(&run_directory.join("target-bytes-after.json"))?;
    let disk_equal = before.disk.image.file_sha256 == after.disk.image.file_sha256
        && before.disk.image.text_sha256 == after.disk.image.text_sha256
        && before.disk.variant_id == after.disk.variant_id;
    push_check(
        "disk-image-unchanged",
        disk_equal,
        format!(
            "before={} after={}",
            before.disk.image.file_sha256, after.disk.image.file_sha256
        ),
    );

    let (live_equal, live_detail) = compare_live(before.process.as_ref(), after.process.as_ref());
    push_check("live-target-bytes-unchanged", live_equal, live_detail);
    let debugger_clear = before
        .process
        .as_ref()
        .zip(after.process.as_ref())
        .is_some_and(|(before, after)| !before.debugger_present && !after.debugger_present);
    push_check(
        "debugger-absent",
        debugger_clear,
        "CheckRemoteDebuggerPresent remained false".to_owned(),
    );
    let modules_clear = before
        .process
        .as_ref()
        .zip(after.process.as_ref())
        .is_some_and(|(before, after)| {
            before.forbidden_modules.is_empty() && after.forbidden_modules.is_empty()
        });
    push_check(
        "forbidden-instrumentation-absent",
        modules_clear,
        "no known injector, T0 Native, or debugger module observed".to_owned(),
    );
    let relocation_valid = before.process.as_ref().is_some_and(|process| {
        process.targets.iter().all(|target| {
            parse_hex_u64(&target.address).is_some_and(|address| {
                parse_hex_u64(&process.module_base)
                    .zip(parse_hex_u64(&target.rva))
                    .is_some_and(|(base, rva)| address == base + rva)
            })
        })
    });
    push_check(
        "rva-relocation",
        relocation_valid,
        "every live target address equals moduleBase + RVA".to_owned(),
    );

    for (name, expected_hash) in &manifest.artifact_hashes {
        let path = run_directory.join(name);
        let actual = fs::read(&path)
            .usage_context(format!("cannot verify artifact {}", path.display()))
            .map(|bytes| sha256_hex(&bytes))?;
        push_check(
            &format!("artifact-hash-{name}"),
            actual.eq_ignore_ascii_case(expected_hash),
            format!("expected={expected_hash} actual={actual}"),
        );
    }

    let passed = checks.iter().all(|check| check.result == "PASS") && manifest.status == "PASS";
    Ok(AnalyzerReport {
        schema_version: ARTIFACT_SCHEMA_VERSION,
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
        generated_utc: utc_now(),
    })
}

pub fn write_analysis(run_directory: &Path, report: &AnalyzerReport) -> AppResult<()> {
    write_pretty_json(&run_directory.join("analyzer.json"), report)?;
    let notes = format!(
        "# Isaac C/S T0-01 baseline\n\n- Run: `{}`\n- Case: `{}`\n- Result: **{}**\n- Evidence: `{}`\n- First divergence: `{}`\n\nThis artifact was produced by a read-only observer. It installed no hooks, sent no packets, and did not modify the game image.\n",
        report.run_id,
        report.case_id,
        report.result,
        report.evidence_label,
        report.first_divergence.as_deref().unwrap_or("none")
    );
    fs::write(run_directory.join("notes.md"), notes).usage_context("cannot write notes.md")?;
    Ok(())
}

pub fn utc_now() -> String {
    Utc::now().to_rfc3339_opts(SecondsFormat::Millis, true)
}

pub fn file_sha256(path: &Path) -> AppResult<String> {
    let bytes = fs::read(path).usage_context(format!("cannot read {}", path.display()))?;
    Ok(sha256_hex(&bytes))
}

pub fn filetime_100ns_to_utc(value: u64) -> Option<String> {
    const WINDOWS_TO_UNIX_SECONDS: i64 = 11_644_473_600;
    let seconds = i64::try_from(value / 10_000_000).ok()? - WINDOWS_TO_UNIX_SECONDS;
    let nanos = u32::try_from((value % 10_000_000) * 100).ok()?;
    DateTime::<Utc>::from_timestamp(seconds, nanos)
        .map(|time| time.to_rfc3339_opts(SecondsFormat::Millis, true))
}

fn validate_case_id(case_id: &str) -> AppResult<()> {
    if case_id.is_empty()
        || case_id.len() > 64
        || !case_id
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_'))
    {
        return Err(AppError::usage(
            "case-id must be 1-64 ASCII alphanumeric, '-' or '_' characters",
        ));
    }
    Ok(())
}

fn create_empty(path: &Path) -> AppResult<()> {
    OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .usage_context(format!("cannot create {}", path.display()))?;
    Ok(())
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

struct SequenceSummary {
    events: u64,
    gaps: u64,
}

fn validate_event_sequence(path: &Path) -> AppResult<SequenceSummary> {
    let file = File::open(path).usage_context(format!("cannot open {}", path.display()))?;
    let mut expected = 1_u64;
    let mut events = 0_u64;
    let mut gaps = 0_u64;
    for (line_number, line) in BufReader::new(file).lines().enumerate() {
        let line = line.usage_context(format!("cannot read line {}", line_number + 1))?;
        if line.trim().is_empty() {
            return Err(AppError::usage(format!(
                "empty JSONL line {}",
                line_number + 1
            )));
        }
        let event: CommonEvent = serde_json::from_str(&line)
            .usage_context(format!("invalid event line {}", line_number + 1))?;
        if event.event_sequence != expected {
            gaps += 1;
        }
        expected = event
            .event_sequence
            .checked_add(1)
            .ok_or_else(|| AppError::usage("event sequence overflow"))?;
        events += 1;
    }
    Ok(SequenceSummary { events, gaps })
}

pub(crate) fn compare_live(
    before: Option<&LiveProcessSnapshot>,
    after: Option<&LiveProcessSnapshot>,
) -> (bool, String) {
    let Some((before, after)) = before.zip(after) else {
        return (false, "missing before or after live snapshot".to_owned());
    };
    if before.pid != after.pid
        || before.creation_time_100ns != after.creation_time_100ns
        || before.module_base != after.module_base
    {
        return (false, "process identity changed during smoke".to_owned());
    }
    if before.targets.len() != after.targets.len() {
        return (false, "target count changed during smoke".to_owned());
    }
    if before.modules != after.modules {
        return (false, "loaded module set changed during smoke".to_owned());
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
    (
        true,
        format!(
            "{} target snapshots remained identical",
            before.targets.len()
        ),
    )
}

fn parse_hex_u64(value: &str) -> Option<u64> {
    let trimmed = value.trim();
    u64::from_str_radix(
        trimmed
            .strip_prefix("0x")
            .or_else(|| trimmed.strip_prefix("0X"))?,
        16,
    )
    .ok()
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ledger::InspectedImage;

    fn inspection() -> StaticInspectionReport {
        StaticInspectionReport {
            schema_version: 1,
            result: "PASS".into(),
            ledger_id: "ledger".into(),
            ledger_sha256: "A".repeat(64),
            variant_id: "variant".into(),
            static_patch_state: "KNOWN".into(),
            image: InspectedImage {
                file_name: "isaac-ng.exe".into(),
                machine: "0x014C".into(),
                timestamp: "0x00000001".into(),
                image_size: "0x00002000".into(),
                preferred_image_base: "0x00400000".into(),
                entry_point_rva: "0x00001000".into(),
                file_sha256: "B".repeat(64),
                text_sha256: "C".repeat(64),
                sections: Vec::new(),
            },
            targets: Vec::new(),
        }
    }

    fn live_snapshot() -> LiveProcessSnapshot {
        LiveProcessSnapshot {
            pid: 42,
            creation_time_100ns: 116_444_736_000_000_000,
            module_base: "0x00400000".into(),
            module_size: 0x2000,
            preferred_image_base: "0x00400000".into(),
            load_bias: "0x0".into(),
            debugger_present: false,
            forbidden_modules: Vec::new(),
            modules: vec![ModuleSnapshot {
                name: "isaac-ng.exe".into(),
                base: "0x00400000".into(),
                size: 0x2000,
            }],
            targets: vec![LiveTargetSnapshot {
                id: "target".into(),
                rva: "0x00001000".into(),
                address: "0x00401000".into(),
                bytes: "90".into(),
                semantic_result: "PASS".into(),
                resolved_target: Some("0x00401100".into()),
                resolved_owner_module: Some("isaac-ng.exe".into()),
            }],
            sampled_qpc: 1,
        }
    }

    #[test]
    fn case_ids_do_not_allow_paths_or_identifiers() {
        assert!(validate_case_id("t0-01-baseline").is_ok());
        assert!(validate_case_id("../escape").is_err());
        assert!(validate_case_id("name with spaces").is_err());
    }

    #[test]
    fn event_writer_assigns_contiguous_sequences() {
        let temp = tempfile::tempdir().unwrap();
        let path = temp.path().join("events.jsonl");
        let mut writer = EventLogWriter::create(&path).unwrap();
        for sequence in 0..3 {
            writer
                .write(CommonEvent {
                    schema_version: 1,
                    event_sequence: 999,
                    qpc: sequence,
                    thread_id: 1,
                    process_role: "EvidenceObserver".into(),
                    network_mode: "Unresolved".into(),
                    session_id: None,
                    server_tick: None,
                    game_frame: None,
                    online_frame: None,
                    event_kind: "test".into(),
                    result: "ok".into(),
                    error_code: None,
                    detail: "none".into(),
                    observed_pid: None,
                })
                .unwrap();
        }
        writer.flush().unwrap();
        let summary = validate_event_sequence(&path).unwrap();
        assert_eq!(summary.events, 3);
        assert_eq!(summary.gaps, 0);
    }

    #[test]
    fn artifact_files_use_create_new_and_refuse_overwrite() {
        let temp = tempfile::tempdir().unwrap();
        let path = temp.path().join("authority.jsonl");
        let _writer = EventLogWriter::create(&path).unwrap();
        assert!(EventLogWriter::create(&path).is_err());
    }

    #[test]
    fn manifest_is_anonymous_and_contains_no_source_path_or_identity() {
        let temp = tempfile::tempdir().unwrap();
        let run =
            ArtifactRun::create(temp.path(), "baseline", &inspection(), "D".repeat(64)).unwrap();
        let json = fs::read_to_string(run.directory.join("manifest.json")).unwrap();
        assert!(!json.contains("C:\\Users\\Example"));
        assert!(!json.contains("7656119"));
        assert!(!json.contains("SteamLibrary"));
        assert!(json.contains("isaac-ng.exe"));
    }

    #[test]
    fn analyzer_replays_hashes_sequence_relocation_and_target_equality() {
        let temp = tempfile::tempdir().unwrap();
        let inspection = inspection();
        let live = live_snapshot();
        let mut run =
            ArtifactRun::create(temp.path(), "baseline", &inspection, "D".repeat(64)).unwrap();
        run.event(1, 7, "observer_started", "PASS", None, "test", None)
            .unwrap();
        run.set_process(&live, Some("1970-01-01T00:00:00.000Z".into()));
        for phase in ["before", "after"] {
            run.write_target_document(
                &format!("target-bytes-{phase}.json"),
                &TargetBytesDocument {
                    schema_version: 1,
                    phase: phase.into(),
                    captured_utc: "1970-01-01T00:00:00.000Z".into(),
                    disk: inspection.clone(),
                    process: Some(live.clone()),
                },
            )
            .unwrap();
        }
        run.finish("PASS", "test complete").unwrap();
        let directory = run.directory.clone();
        drop(run);
        let report = analyze_run(&directory).unwrap();
        assert_eq!(report.result, "PASS");
        assert!(report.first_divergence.is_none());
    }

    #[test]
    fn target_comparison_includes_resolved_semantics() {
        let before = live_snapshot();
        let mut after = before.clone();
        after.targets[0].resolved_target = Some("0x00401200".into());
        assert!(!compare_live(Some(&before), Some(&after)).0);
    }

    #[test]
    fn filetime_conversion_matches_unix_epoch() {
        assert_eq!(
            filetime_100ns_to_utc(116_444_736_000_000_000).as_deref(),
            Some("1970-01-01T00:00:00.000Z")
        );
    }
}
