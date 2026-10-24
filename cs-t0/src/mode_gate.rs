use std::collections::BTreeMap;
use std::fs::{self, OpenOptions};
use std::io::Write;
use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};

use crate::artifact::{RunManifest, analyze_run, file_sha256, utc_now};
use crate::error::{AppError, AppResult, UsageContext};
use crate::mode_artifact::{
    MODE_ARTIFACT_SCHEMA_VERSION, ModeComparisonReport, ModeRunManifest, analyze_mode_run,
    compare_mode_runs,
};

pub const T0_02_GATE_SCHEMA_VERSION: u32 = 1;
pub const T0_02_GATE_REVISION: &str = "t0-02-packet-pairing-v2";

#[derive(Clone, Debug)]
pub struct GateInputs<'a> {
    pub baseline: &'a Path,
    pub offline_control: &'a Path,
    pub offline_instrumented: &'a Path,
    pub control_a: &'a Path,
    pub control_b: &'a Path,
    pub instrumented_a: &'a Path,
    pub instrumented_b: &'a Path,
    pub comparison: &'a Path,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct GateDecision {
    pub schema_version: u32,
    pub gate: String,
    pub status: String,
    pub analysis_revision: String,
    pub packet_rule: String,
    pub rejected_hypothesis: String,
    pub sources: Vec<GateSource>,
    pub checks: Vec<GateCheck>,
    #[serde(rename = "t0_03Unlocked", alias = "t003Unlocked")]
    pub t0_03_unlocked: bool,
    pub generated_utc: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct GateSource {
    pub label: String,
    pub run_id: String,
    pub files: BTreeMap<String, String>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct GateCheck {
    pub id: String,
    pub result: String,
    pub detail: String,
}

pub fn write_gate_decision(inputs: &GateInputs<'_>, output: &Path) -> AppResult<GateDecision> {
    let decision = build_gate_decision(inputs)?;
    let bytes = serde_json::to_vec_pretty(&decision)
        .usage_context("cannot serialize T0-02 gate decision")?;
    let mut file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(output)
        .usage_context(format!("cannot create gate decision {}", output.display()))?;
    file.write_all(&bytes)
        .usage_context(format!("cannot write gate decision {}", output.display()))?;
    Ok(decision)
}

pub fn build_gate_decision(inputs: &GateInputs<'_>) -> AppResult<GateDecision> {
    let baseline_report = analyze_run(inputs.baseline)?;
    require_pass("T0-01 baseline", &baseline_report.result)?;

    let offline_control = read_mode_manifest(inputs.offline_control)?;
    let offline_instrumented = read_mode_manifest(inputs.offline_instrumented)?;
    let control_a = read_mode_manifest(inputs.control_a)?;
    let control_b = read_mode_manifest(inputs.control_b)?;
    let instrumented_a = read_mode_manifest(inputs.instrumented_a)?;
    let instrumented_b = read_mode_manifest(inputs.instrumented_b)?;
    for (label, directory) in [
        ("offline-control", inputs.offline_control),
        ("offline-instrumented", inputs.offline_instrumented),
        ("original-online-control-a", inputs.control_a),
        ("original-online-control-b", inputs.control_b),
        ("original-online-instrumented-a", inputs.instrumented_a),
        ("original-online-instrumented-b", inputs.instrumented_b),
    ] {
        require_pass(label, &analyze_mode_run(directory)?.result)?;
    }
    require_mode(&offline_control, "control-no-native", "solo")?;
    require_mode(&offline_instrumented, "offline-disarmed", "solo")?;
    require_mode(&control_a, "control-no-native", "a")?;
    require_mode(&control_b, "control-no-native", "b")?;
    require_mode(&instrumented_a, "original-online", "a")?;
    require_mode(&instrumented_b, "original-online", "b")?;

    let offline_comparison =
        compare_mode_runs(inputs.offline_control, inputs.offline_instrumented, None)?;
    require_pass("offline C0/C1 comparison", &offline_comparison.result)?;
    let comparison: ModeComparisonReport = read_json(inputs.comparison)?;
    require_pass("OriginalOnline C2 comparison", &comparison.result)?;
    if comparison.analysis_revision != T0_02_GATE_REVISION
        || comparison.control_run_id != control_a.run_id
        || comparison.instrumented_run_id != instrumented_a.run_id
        || comparison.peer_run_id.as_deref() != Some(instrumented_b.run_id.as_str())
        || comparison.checks.iter().any(|check| check.result != "PASS")
    {
        return Err(AppError::evidence(
            "T0-02 packet comparison revision, run identity or checks do not match the gate",
        ));
    }
    let packet = comparison
        .packet_pairing
        .as_ref()
        .ok_or_else(|| AppError::evidence("T0-02 comparison has no packet audit"))?;
    for direction in [&packet.a_to_b, &packet.b_to_a] {
        if direction.matched_packets < 2
            || direction.unmatched_reads_inside != 0
            || direction.reliable_missing_sends_inside != 0
            || direction.invalid_or_missing_send_modes != 0
        {
            return Err(AppError::evidence(
                "T0-02 packet audit does not satisfy origin and delivery semantics",
            ));
        }
    }
    require_same_build(&[&offline_control, &offline_instrumented])?;
    require_same_build(&[&control_a, &control_b, &instrumented_a, &instrumented_b])?;

    let baseline_manifest: RunManifest = read_json(&inputs.baseline.join("manifest.json"))?;
    let sources = vec![
        source("t0-01-baseline", inputs.baseline, &baseline_manifest.run_id)?,
        source(
            "offline-control",
            inputs.offline_control,
            &offline_control.run_id,
        )?,
        source(
            "offline-instrumented",
            inputs.offline_instrumented,
            &offline_instrumented.run_id,
        )?,
        source(
            "original-online-control-a",
            inputs.control_a,
            &control_a.run_id,
        )?,
        source(
            "original-online-control-b",
            inputs.control_b,
            &control_b.run_id,
        )?,
        source(
            "original-online-instrumented-a",
            inputs.instrumented_a,
            &instrumented_a.run_id,
        )?,
        source(
            "original-online-instrumented-b",
            inputs.instrumented_b,
            &instrumented_b.run_id,
        )?,
        GateSource {
            label: "packet-comparison-v2".to_owned(),
            run_id: comparison.instrumented_run_id.clone(),
            files: BTreeMap::from([(
                inputs
                    .comparison
                    .file_name()
                    .and_then(|name| name.to_str())
                    .unwrap_or("comparison.json")
                    .to_owned(),
                file_sha256(inputs.comparison)?,
            )]),
        },
    ];
    let checks = vec![
        gate_check("t0-01-baseline", "baseline analyzer PASS"),
        gate_check(
            "offline-lifecycle",
            "offline C0/C1 and lifecycle analyzer PASS",
        ),
        gate_check(
            "original-online-controls",
            "both C0 endpoint analyzers PASS",
        ),
        gate_check(
            "original-online-instrumented",
            "both C2 endpoint analyzers PASS",
        ),
        gate_check(
            "packet-origin-and-delivery-semantics",
            "both directions have at least two anchors, zero internal unknown Read, zero reliable missing Send and zero invalid send mode",
        ),
        gate_check(
            "safe-unload-and-restore",
            "all instrumented endpoint analyzers PASS restoration checks",
        ),
    ];
    Ok(GateDecision {
        schema_version: T0_02_GATE_SCHEMA_VERSION,
        gate: "T0-02-mode-isolation".to_owned(),
        status: "PASS".to_owned(),
        analysis_revision: T0_02_GATE_REVISION.to_owned(),
        packet_rule: "event-sequence anchors; every internal Read requires an anonymous-peer/size/SHA-256 Send; mode 0/1 loss is diagnostic; mode 2/3 loss fails".to_owned(),
        rejected_hypothesis: "every successful unreliable Send must be observed as a Read".to_owned(),
        sources,
        checks,
        t0_03_unlocked: true,
        generated_utc: utc_now(),
    })
}

fn gate_check(id: &str, detail: &str) -> GateCheck {
    GateCheck {
        id: id.to_owned(),
        result: "PASS".to_owned(),
        detail: detail.to_owned(),
    }
}

fn require_pass(label: &str, result: &str) -> AppResult<()> {
    if result == "PASS" {
        Ok(())
    } else {
        Err(AppError::evidence(format!(
            "{label} is {result}, expected PASS"
        )))
    }
}

fn require_mode(manifest: &ModeRunManifest, scenario: &str, endpoint: &str) -> AppResult<()> {
    if manifest.schema_version == MODE_ARTIFACT_SCHEMA_VERSION
        && manifest.scenario == scenario
        && manifest.endpoint == endpoint
    {
        Ok(())
    } else {
        Err(AppError::evidence(format!(
            "run {} expected scenario {scenario} endpoint {endpoint}",
            manifest.run_id
        )))
    }
}

fn require_same_build(manifests: &[&ModeRunManifest]) -> AppResult<()> {
    let Some(first) = manifests.first() else {
        return Err(AppError::usage("same-build check requires a run"));
    };
    if manifests.iter().all(|manifest| {
        manifest.game.file_sha256 == first.game.file_sha256
            && manifest.game.text_sha256 == first.game.text_sha256
            && manifest.game.variant_id == first.game.variant_id
    }) {
        Ok(())
    } else {
        Err(AppError::evidence(
            "T0-02 gate inputs do not use the same game build",
        ))
    }
}

fn source(label: &str, directory: &Path, run_id: &str) -> AppResult<GateSource> {
    let mut files = BTreeMap::new();
    for entry in fs::read_dir(directory).usage_context(format!(
        "cannot enumerate gate source {}",
        directory.display()
    ))? {
        let entry = entry.usage_context("cannot enumerate gate source entry")?;
        let path = entry.path();
        if !path.is_file() {
            continue;
        }
        let extension = path.extension().and_then(|value| value.to_str());
        if !matches!(extension, Some("json" | "jsonl" | "md")) {
            continue;
        }
        let name = path
            .file_name()
            .and_then(|value| value.to_str())
            .ok_or_else(|| AppError::usage("gate source file name is not Unicode"))?;
        files.insert(name.to_owned(), file_sha256(&path)?);
    }
    if files.is_empty() {
        return Err(AppError::usage("gate source has no evidence files"));
    }
    Ok(GateSource {
        label: label.to_owned(),
        run_id: run_id.to_owned(),
        files,
    })
}

fn read_mode_manifest(directory: &Path) -> AppResult<ModeRunManifest> {
    read_json(&directory.join("manifest.json"))
}

fn read_json<T: for<'de> Deserialize<'de>>(path: &Path) -> AppResult<T> {
    let bytes = fs::read(path).usage_context(format!("cannot read {}", path.display()))?;
    serde_json::from_slice(&bytes).usage_context(format!("cannot parse {}", path.display()))
}

pub fn default_gate_output(instrumented_a: &Path) -> PathBuf {
    instrumented_a.join("t0-02-gate-decision.json")
}
