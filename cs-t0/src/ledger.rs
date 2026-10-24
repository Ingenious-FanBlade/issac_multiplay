use std::collections::{BTreeMap, BTreeSet};
use std::fs;
use std::path::Path;

use serde::{Deserialize, Serialize};

use crate::error::{AppError, AppResult, UsageContext};
use crate::pe::{ImageHashes, PeImage, SectionInfo, sha256_hex};

const SUPPORTED_LEDGER_SCHEMA: u32 = 1;

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct TargetLedger {
    pub schema_version: u32,
    pub ledger_id: String,
    pub experiment_identity: String,
    pub image: ImageExpectation,
    pub targets: Vec<TargetSpec>,
    pub candidates: Vec<EvidenceItem>,
    pub diagnostics: Vec<EvidenceItem>,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ImageExpectation {
    pub file_name: String,
    pub machine: String,
    pub timestamp: String,
    pub image_size: String,
    pub preferred_image_base: String,
    pub sections: Vec<SectionExpectation>,
    pub variants: Vec<ImageVariant>,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct SectionExpectation {
    pub name: String,
    pub virtual_size: String,
    pub rva: String,
    pub raw_size: String,
    pub raw_offset: String,
    pub characteristics: String,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ImageVariant {
    pub id: String,
    pub description: String,
    pub static_patch_state: String,
    pub file_sha256: String,
    pub text_sha256: String,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct TargetSpec {
    pub id: String,
    pub semantic: String,
    pub kind: String,
    pub section: String,
    pub rva: String,
    pub feature: FeatureSpec,
    #[serde(default)]
    pub expected_bytes: Option<String>,
    #[serde(default)]
    pub expected_target_rva: Option<String>,
    #[serde(default)]
    pub import_name: Option<String>,
    #[serde(default)]
    pub expected_owner_module: Option<String>,
    #[serde(default)]
    pub operand_offset: Option<usize>,
    #[serde(default)]
    pub vtable_target_rvas: Vec<String>,
    pub abi: String,
    pub thread: String,
    pub evidence_label: String,
    pub evidence_sources: Vec<String>,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct FeatureSpec {
    pub section: String,
    pub rva: String,
    pub bytes: String,
    #[serde(default)]
    pub masked_ranges: Vec<MaskedRange>,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct MaskedRange {
    pub offset: usize,
    pub length: usize,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct EvidenceItem {
    pub id: String,
    pub description: String,
    pub evidence_label: String,
    pub evidence_sources: Vec<String>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct StaticInspectionReport {
    pub schema_version: u32,
    pub result: String,
    pub ledger_id: String,
    pub ledger_sha256: String,
    pub variant_id: String,
    pub static_patch_state: String,
    pub image: InspectedImage,
    pub targets: Vec<TargetInspection>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct InspectedImage {
    pub file_name: String,
    pub machine: String,
    pub timestamp: String,
    pub image_size: String,
    pub preferred_image_base: String,
    pub entry_point_rva: String,
    pub file_sha256: String,
    pub text_sha256: String,
    pub sections: Vec<SectionInfo>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct TargetInspection {
    pub id: String,
    pub semantic: String,
    pub kind: String,
    pub rva: String,
    pub feature_rva: String,
    pub feature_matches: u32,
    pub original_bytes: String,
    pub resolved_target: Option<String>,
    pub import_name: Option<String>,
    pub evidence_label: String,
    pub result: String,
}

#[derive(Clone, Debug)]
pub struct LoadedLedger {
    pub ledger: TargetLedger,
    pub raw_sha256: String,
}

impl LoadedLedger {
    pub fn read(path: &Path) -> AppResult<Self> {
        let raw = fs::read(path).usage_context(format!("cannot read ledger {}", path.display()))?;
        let ledger: TargetLedger = serde_json::from_slice(&raw)
            .usage_context(format!("cannot parse ledger {}", path.display()))?;
        ledger.validate_schema()?;
        Ok(Self {
            ledger,
            raw_sha256: sha256_hex(&raw),
        })
    }
}

impl TargetLedger {
    pub fn validate_schema(&self) -> AppResult<()> {
        if self.schema_version != SUPPORTED_LEDGER_SCHEMA {
            return Err(AppError::usage(format!(
                "unsupported ledger schema {}",
                self.schema_version
            )));
        }
        if self.ledger_id.trim().is_empty() || self.experiment_identity.trim().is_empty() {
            return Err(AppError::usage("ledger identity cannot be empty"));
        }
        if !self.image.file_name.eq_ignore_ascii_case("isaac-ng.exe") {
            return Err(AppError::usage("ledger target must be isaac-ng.exe"));
        }
        let mut variant_ids = BTreeSet::new();
        for variant in &self.image.variants {
            if !variant_ids.insert(variant.id.as_str()) {
                return Err(AppError::usage(format!(
                    "duplicate image variant {}",
                    variant.id
                )));
            }
            validate_sha256(&variant.file_sha256, "variant file SHA-256")?;
            validate_sha256(&variant.text_sha256, "variant .text SHA-256")?;
        }
        if self.image.variants.is_empty() {
            return Err(AppError::usage("ledger has no image variants"));
        }
        let mut target_ids = BTreeSet::new();
        for target in &self.targets {
            if !target_ids.insert(target.id.as_str()) {
                return Err(AppError::usage(format!(
                    "duplicate target id {}",
                    target.id
                )));
            }
            validate_evidence_label(&target.evidence_label)?;
            let _ = target.rva_value()?;
            let _ = target.feature.rva_value()?;
            let pattern = target.feature.pattern()?;
            if pattern.bytes.is_empty() {
                return Err(AppError::usage(format!(
                    "target {} has an empty feature",
                    target.id
                )));
            }
            if target.kind == "signature"
                && let Some(expected) = &target.expected_bytes
            {
                let expected = decode_hex(expected, "expectedBytes")?;
                if expected.len() != pattern.bytes.len() {
                    return Err(AppError::usage(format!(
                        "signature target {} expectedBytes length {} does not match feature length {}",
                        target.id,
                        expected.len(),
                        pattern.bytes.len()
                    )));
                }
            }
            if target.evidence_sources.is_empty() {
                return Err(AppError::usage(format!(
                    "target {} has no evidence sources",
                    target.id
                )));
            }
        }
        for item in self.candidates.iter().chain(&self.diagnostics) {
            validate_evidence_label(&item.evidence_label)?;
            if item.evidence_sources.is_empty() {
                return Err(AppError::usage(format!(
                    "evidence item {} has no source",
                    item.id
                )));
            }
        }
        Ok(())
    }

    pub fn inspect(
        &self,
        image: &PeImage,
        expected_variant: &str,
        ledger_sha256: &str,
    ) -> AppResult<StaticInspectionReport> {
        let expected_machine = parse_u16(&self.image.machine, "image machine")?;
        let expected_timestamp = parse_u32(&self.image.timestamp, "image timestamp")?;
        let expected_image_size = parse_u32(&self.image.image_size, "image size")?;
        let expected_image_base = parse_u32(&self.image.preferred_image_base, "image base")?;
        if image.machine != expected_machine {
            return Err(AppError::evidence(format!(
                "machine mismatch: expected 0x{expected_machine:04X}, got 0x{:04X}",
                image.machine
            )));
        }
        if image.timestamp != expected_timestamp {
            return Err(AppError::evidence(format!(
                "timestamp mismatch: expected 0x{expected_timestamp:08X}, got 0x{:08X}",
                image.timestamp
            )));
        }
        if image.image_size != expected_image_size {
            return Err(AppError::evidence(format!(
                "image size mismatch: expected 0x{expected_image_size:08X}, got 0x{:08X}",
                image.image_size
            )));
        }
        if image.image_base != expected_image_base {
            return Err(AppError::evidence(format!(
                "preferred image base mismatch: expected 0x{expected_image_base:08X}, got 0x{:08X}",
                image.image_base
            )));
        }
        self.validate_sections(&image.sections)?;
        let hashes = image
            .hashes()
            .map_err(|error| AppError::evidence(error.to_string()))?;
        let variant = self
            .image
            .variants
            .iter()
            .find(|variant| variant.id == expected_variant)
            .ok_or_else(|| {
                AppError::usage(format!("unknown expected variant {expected_variant}"))
            })?;
        validate_variant(&hashes, variant)?;

        let mut targets = Vec::with_capacity(self.targets.len());
        for target in &self.targets {
            targets.push(self.inspect_target(image, target)?);
        }
        Ok(StaticInspectionReport {
            schema_version: 1,
            result: "PASS".to_owned(),
            ledger_id: self.ledger_id.clone(),
            ledger_sha256: ledger_sha256.to_owned(),
            variant_id: variant.id.clone(),
            static_patch_state: variant.static_patch_state.clone(),
            image: InspectedImage {
                file_name: self.image.file_name.clone(),
                machine: format!("0x{:04X}", image.machine),
                timestamp: format!("0x{:08X}", image.timestamp),
                image_size: format!("0x{:08X}", image.image_size),
                preferred_image_base: format!("0x{:08X}", image.image_base),
                entry_point_rva: format!("0x{:08X}", image.entry_point_rva),
                file_sha256: hashes.file_sha256,
                text_sha256: hashes.text_sha256,
                sections: image.sections.clone(),
            },
            targets,
        })
    }

    fn validate_sections(&self, actual: &[SectionInfo]) -> AppResult<()> {
        if actual.len() != self.image.sections.len() {
            return Err(AppError::evidence(format!(
                "section count mismatch: expected {}, got {}",
                self.image.sections.len(),
                actual.len()
            )));
        }
        for (expected, actual) in self.image.sections.iter().zip(actual) {
            let normalized = SectionInfo {
                name: expected.name.clone(),
                virtual_size: parse_u32(&expected.virtual_size, "section virtual size")?,
                rva: parse_u32(&expected.rva, "section RVA")?,
                raw_size: parse_u32(&expected.raw_size, "section raw size")?,
                raw_offset: parse_u32(&expected.raw_offset, "section raw offset")?,
                characteristics: parse_u32(&expected.characteristics, "section characteristics")?,
            };
            if &normalized != actual {
                return Err(AppError::evidence(format!(
                    "section mismatch for {}: expected {:?}, got {:?}",
                    expected.name, normalized, actual
                )));
            }
        }
        Ok(())
    }

    fn inspect_target(&self, image: &PeImage, target: &TargetSpec) -> AppResult<TargetInspection> {
        let rva = target.rva_value()?;
        let feature_rva = target.feature.rva_value()?;
        ensure_rva_in_section(image, rva, &target.section, &target.id)?;
        ensure_rva_in_section(image, feature_rva, &target.feature.section, &target.id)?;
        let pattern = target.feature.pattern()?;
        let hits = image
            .scan_masked(&target.feature.section, &pattern.bytes, &pattern.mask)
            .map_err(|error| AppError::evidence(error.to_string()))?;
        if hits.len() != 1 || hits[0] != feature_rva {
            return Err(AppError::evidence(format!(
                "target {} feature mismatch: expected only 0x{feature_rva:08X}, got {:?}",
                target.id,
                hits.iter()
                    .map(|rva| format!("0x{rva:08X}"))
                    .collect::<Vec<_>>()
            )));
        }
        if let Some(expected) = &target.expected_bytes {
            let expected = decode_hex(expected, "expectedBytes")?;
            let actual = image
                .bytes_at_rva(rva, expected.len())
                .map_err(|error| AppError::evidence(error.to_string()))?;
            if actual != expected {
                return Err(AppError::evidence(format!(
                    "target {} original bytes mismatch",
                    target.id
                )));
            }
        }

        let resolved_target = match target.kind.as_str() {
            "relativeCall" => Some(validate_relative_call(image, target, rva)?),
            "iatCall" => Some(validate_iat_call(image, target, rva)?),
            "absoluteReference" => Some(validate_absolute_reference(image, target, feature_rva)?),
            "pointerValue" => Some(validate_pointer_value(image, target, rva)?),
            "vtable" => Some(validate_vtable(image, target, rva)?),
            "signature" => None,
            unknown => {
                return Err(AppError::usage(format!(
                    "target {} has unknown kind {unknown}",
                    target.id
                )));
            }
        };
        let capture_rva = target.capture_rva()?;
        let capture_size = target.capture_size()?;
        let original_bytes = image
            .bytes_at_rva(capture_rva, capture_size)
            .map_err(|error| AppError::evidence(error.to_string()))?;
        Ok(TargetInspection {
            id: target.id.clone(),
            semantic: target.semantic.clone(),
            kind: target.kind.clone(),
            rva: format!("0x{rva:08X}"),
            feature_rva: format!("0x{feature_rva:08X}"),
            feature_matches: 1,
            original_bytes: hex::encode_upper(original_bytes),
            resolved_target,
            import_name: target.import_name.clone(),
            evidence_label: target.evidence_label.clone(),
            result: "PASS".to_owned(),
        })
    }
}

impl TargetSpec {
    pub fn rva_value(&self) -> AppResult<u32> {
        parse_u32(&self.rva, &format!("target {} RVA", self.id))
    }

    pub fn expected_target_value(&self) -> AppResult<u32> {
        parse_u32(
            self.expected_target_rva.as_deref().ok_or_else(|| {
                AppError::usage(format!("target {} has no expected target RVA", self.id))
            })?,
            &format!("target {} expected target RVA", self.id),
        )
    }

    pub fn capture_size(&self) -> AppResult<usize> {
        if self.kind == "absoluteReference" {
            return Ok(self.feature.pattern()?.bytes.len());
        }
        if let Some(expected) = &self.expected_bytes {
            return Ok(decode_hex(expected, "expectedBytes")?.len());
        }
        match self.kind.as_str() {
            "relativeCall" => Ok(5),
            "iatCall" => Ok(6),
            "pointerValue" => Ok(4),
            "vtable" => self
                .vtable_target_rvas
                .len()
                .checked_mul(4)
                .ok_or_else(|| AppError::usage("vtable capture size overflow")),
            "signature" => Ok(self.feature.pattern()?.bytes.len()),
            _ => Err(AppError::usage(format!(
                "target {} has unknown capture size",
                self.id
            ))),
        }
    }

    pub fn capture_rva(&self) -> AppResult<u32> {
        match self.kind.as_str() {
            "absoluteReference" | "signature" => self.feature.rva_value(),
            _ => self.rva_value(),
        }
    }
}

impl FeatureSpec {
    pub fn rva_value(&self) -> AppResult<u32> {
        parse_u32(&self.rva, "feature RVA")
    }

    pub fn pattern(&self) -> AppResult<MaskedPattern> {
        let bytes = decode_hex(&self.bytes, "feature bytes")?;
        let mut mask = vec![0xFF; bytes.len()];
        for range in &self.masked_ranges {
            let end = range
                .offset
                .checked_add(range.length)
                .ok_or_else(|| AppError::usage("feature mask range overflow"))?;
            let slots = mask
                .get_mut(range.offset..end)
                .ok_or_else(|| AppError::usage("feature mask range out of bounds"))?;
            slots.fill(0);
        }
        Ok(MaskedPattern { bytes, mask })
    }
}

pub struct MaskedPattern {
    pub bytes: Vec<u8>,
    pub mask: Vec<u8>,
}

fn validate_variant(hashes: &ImageHashes, variant: &ImageVariant) -> AppResult<()> {
    if !hashes
        .file_sha256
        .eq_ignore_ascii_case(&variant.file_sha256)
        || !hashes
            .text_sha256
            .eq_ignore_ascii_case(&variant.text_sha256)
    {
        return Err(AppError::evidence(format!(
            "fingerprint mismatch for variant {}: file {}, .text {}",
            variant.id, hashes.file_sha256, hashes.text_sha256
        )));
    }
    Ok(())
}

fn validate_relative_call(image: &PeImage, target: &TargetSpec, rva: u32) -> AppResult<String> {
    let call = image
        .bytes_at_rva(rva, 5)
        .map_err(|error| AppError::evidence(error.to_string()))?;
    if call[0] != 0xE8 {
        return Err(AppError::evidence(format!(
            "target {} is not call rel32",
            target.id
        )));
    }
    let displacement = i32::from_le_bytes(call[1..5].try_into().unwrap());
    let actual = i64::from(rva) + 5 + i64::from(displacement);
    let expected = target.expected_target_value()?;
    if actual != i64::from(expected) {
        return Err(AppError::evidence(format!(
            "target {} call target mismatch: expected 0x{expected:08X}, got 0x{actual:08X}",
            target.id
        )));
    }
    Ok(format!("RVA 0x{expected:08X}"))
}

fn validate_iat_call(image: &PeImage, target: &TargetSpec, rva: u32) -> AppResult<String> {
    let call = image
        .bytes_at_rva(rva, 6)
        .map_err(|error| AppError::evidence(error.to_string()))?;
    if call[0..2] != [0xFF, 0x15] {
        return Err(AppError::evidence(format!(
            "target {} is not call [IAT]",
            target.id
        )));
    }
    let import = target
        .import_name
        .as_deref()
        .ok_or_else(|| AppError::usage(format!("target {} has no import name", target.id)))?;
    let iat_rva = *image.imports.get(import).ok_or_else(|| {
        AppError::evidence(format!("target {} import {import} is missing", target.id))
    })?;
    let operand = u32::from_le_bytes(call[2..6].try_into().unwrap());
    let expected_operand = image
        .image_base
        .checked_add(iat_rva)
        .ok_or_else(|| AppError::evidence("IAT address overflow"))?;
    if operand != expected_operand {
        return Err(AppError::evidence(format!(
            "target {} IAT operand mismatch: expected 0x{expected_operand:08X}, got 0x{operand:08X}",
            target.id
        )));
    }
    Ok(format!("IAT RVA 0x{iat_rva:08X} ({import})"))
}

fn validate_absolute_reference(
    image: &PeImage,
    target: &TargetSpec,
    feature_rva: u32,
) -> AppResult<String> {
    let offset = target
        .operand_offset
        .ok_or_else(|| AppError::usage(format!("target {} has no operand offset", target.id)))?;
    let operand_rva = feature_rva
        .checked_add(
            u32::try_from(offset).map_err(|_| AppError::usage("operand offset too large"))?,
        )
        .ok_or_else(|| AppError::usage("operand RVA overflow"))?;
    let operand = image
        .bytes_at_rva(operand_rva, 4)
        .map_err(|error| AppError::evidence(error.to_string()))?;
    let actual = u32::from_le_bytes(operand.try_into().unwrap());
    let referenced_rva = target.rva_value()?;
    let expected = image
        .image_base
        .checked_add(referenced_rva)
        .ok_or_else(|| AppError::evidence("absolute reference overflow"))?;
    if actual != expected {
        return Err(AppError::evidence(format!(
            "target {} absolute reference mismatch: expected 0x{expected:08X}, got 0x{actual:08X}",
            target.id
        )));
    }
    Ok(format!("RVA 0x{referenced_rva:08X}"))
}

fn validate_pointer_value(image: &PeImage, target: &TargetSpec, rva: u32) -> AppResult<String> {
    let value = image
        .bytes_at_rva(rva, 4)
        .map_err(|error| AppError::evidence(error.to_string()))?;
    let actual = u32::from_le_bytes(value.try_into().unwrap());
    let expected_target = target.expected_target_value()?;
    let expected = image
        .image_base
        .checked_add(expected_target)
        .ok_or_else(|| AppError::evidence("pointer target overflow"))?;
    if actual != expected {
        return Err(AppError::evidence(format!(
            "target {} pointer mismatch: expected 0x{expected:08X}, got 0x{actual:08X}",
            target.id
        )));
    }
    Ok(format!("RVA 0x{expected_target:08X}"))
}

fn validate_vtable(image: &PeImage, target: &TargetSpec, rva: u32) -> AppResult<String> {
    if target.vtable_target_rvas.is_empty() {
        return Err(AppError::usage(format!(
            "target {} vtable has no entries",
            target.id
        )));
    }
    for (index, expected) in target.vtable_target_rvas.iter().enumerate() {
        let entry_rva = rva
            .checked_add(u32::try_from(index * 4).unwrap())
            .ok_or_else(|| AppError::evidence("vtable RVA overflow"))?;
        let bytes = image
            .bytes_at_rva(entry_rva, 4)
            .map_err(|error| AppError::evidence(error.to_string()))?;
        let actual = u32::from_le_bytes(bytes.try_into().unwrap());
        let expected_rva = parse_u32(expected, "vtable target RVA")?;
        let expected_address = image
            .image_base
            .checked_add(expected_rva)
            .ok_or_else(|| AppError::evidence("vtable target overflow"))?;
        if actual != expected_address {
            return Err(AppError::evidence(format!(
                "target {} vtable entry {} mismatch: expected 0x{expected_address:08X}, got 0x{actual:08X}",
                target.id, index
            )));
        }
    }
    Ok(format!(
        "{} validated entries",
        target.vtable_target_rvas.len()
    ))
}

fn ensure_rva_in_section(
    image: &PeImage,
    rva: u32,
    expected_section: &str,
    target_id: &str,
) -> AppResult<()> {
    let section = image
        .sections
        .iter()
        .find(|section| {
            let span = section.virtual_size.max(section.raw_size);
            rva >= section.rva && rva < section.rva.saturating_add(span)
        })
        .ok_or_else(|| {
            AppError::evidence(format!("target {target_id} RVA 0x{rva:08X} is unmapped"))
        })?;
    if section.name != expected_section {
        return Err(AppError::evidence(format!(
            "target {target_id} expected section {expected_section}, got {}",
            section.name
        )));
    }
    Ok(())
}

fn validate_evidence_label(label: &str) -> AppResult<()> {
    const LABELS: [&str; 6] = [
        "STATIC_CONFIRMED",
        "RUNTIME_OBSERVED",
        "DUAL_ENDPOINT_CONFIRMED",
        "DESIGN_ONLY",
        "REJECTED_HYPOTHESIS",
        "UNKNOWN",
    ];
    if LABELS.contains(&label) {
        Ok(())
    } else {
        Err(AppError::usage(format!("invalid evidence label {label}")))
    }
}

fn validate_sha256(value: &str, label: &str) -> AppResult<()> {
    if value.len() != 64 || !value.bytes().all(|byte| byte.is_ascii_hexdigit()) {
        return Err(AppError::usage(format!("invalid {label}")));
    }
    Ok(())
}

fn decode_hex(value: &str, label: &str) -> AppResult<Vec<u8>> {
    let compact = value
        .chars()
        .filter(|character| !character.is_ascii_whitespace())
        .collect::<String>();
    hex::decode(compact).map_err(|error| AppError::usage(format!("invalid {label}: {error}")))
}

pub fn parse_u32(value: &str, label: &str) -> AppResult<u32> {
    let trimmed = value.trim();
    if let Some(hex) = trimmed
        .strip_prefix("0x")
        .or_else(|| trimmed.strip_prefix("0X"))
    {
        u32::from_str_radix(hex, 16)
            .map_err(|error| AppError::usage(format!("invalid {label}: {error}")))
    } else {
        trimmed
            .parse::<u32>()
            .map_err(|error| AppError::usage(format!("invalid {label}: {error}")))
    }
}

fn parse_u16(value: &str, label: &str) -> AppResult<u16> {
    let value = parse_u32(value, label)?;
    u16::try_from(value).map_err(|_| AppError::usage(format!("invalid {label}: out of range")))
}

pub fn target_map(ledger: &TargetLedger) -> BTreeMap<&str, &TargetSpec> {
    ledger
        .targets
        .iter()
        .map(|target| (target.id.as_str(), target))
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    fn minimal_pe(text: &[u8]) -> PeImage {
        let raw_size = 0x200_usize;
        let mut bytes = vec![0_u8; 0x200 + raw_size];
        bytes[0..2].copy_from_slice(&0x5A4D_u16.to_le_bytes());
        bytes[0x3C..0x40].copy_from_slice(&0x80_u32.to_le_bytes());
        bytes[0x80..0x84].copy_from_slice(&0x0000_4550_u32.to_le_bytes());
        bytes[0x84..0x86].copy_from_slice(&0x014C_u16.to_le_bytes());
        bytes[0x86..0x88].copy_from_slice(&1_u16.to_le_bytes());
        bytes[0x88..0x8C].copy_from_slice(&1_u32.to_le_bytes());
        bytes[0x94..0x96].copy_from_slice(&0xE0_u16.to_le_bytes());
        let optional = 0x98;
        bytes[optional..optional + 2].copy_from_slice(&0x010B_u16.to_le_bytes());
        bytes[optional + 16..optional + 20].copy_from_slice(&0x1000_u32.to_le_bytes());
        bytes[optional + 28..optional + 32].copy_from_slice(&0x400000_u32.to_le_bytes());
        bytes[optional + 56..optional + 60].copy_from_slice(&0x2000_u32.to_le_bytes());
        bytes[optional + 92..optional + 96].copy_from_slice(&16_u32.to_le_bytes());
        let section = optional + 0xE0;
        bytes[section..section + 5].copy_from_slice(b".text");
        bytes[section + 8..section + 12].copy_from_slice(&(text.len() as u32).to_le_bytes());
        bytes[section + 12..section + 16].copy_from_slice(&0x1000_u32.to_le_bytes());
        bytes[section + 16..section + 20].copy_from_slice(&(raw_size as u32).to_le_bytes());
        bytes[section + 20..section + 24].copy_from_slice(&0x200_u32.to_le_bytes());
        bytes[section + 36..section + 40].copy_from_slice(&0x6000_0020_u32.to_le_bytes());
        bytes[0x200..0x200 + text.len()].copy_from_slice(text);
        PeImage::parse(bytes).unwrap()
    }

    fn ledger_for(image: &PeImage, targets: Vec<TargetSpec>) -> TargetLedger {
        let hashes = image.hashes().unwrap();
        TargetLedger {
            schema_version: 1,
            ledger_id: "test-ledger".into(),
            experiment_identity: "test-experiment".into(),
            image: ImageExpectation {
                file_name: "isaac-ng.exe".into(),
                machine: "0x014C".into(),
                timestamp: "0x1".into(),
                image_size: "0x2000".into(),
                preferred_image_base: "0x400000".into(),
                sections: image
                    .sections
                    .iter()
                    .map(|section| SectionExpectation {
                        name: section.name.clone(),
                        virtual_size: format!("0x{:X}", section.virtual_size),
                        rva: format!("0x{:X}", section.rva),
                        raw_size: format!("0x{:X}", section.raw_size),
                        raw_offset: format!("0x{:X}", section.raw_offset),
                        characteristics: format!("0x{:X}", section.characteristics),
                    })
                    .collect(),
                variants: vec![ImageVariant {
                    id: "known".into(),
                    description: "test".into(),
                    static_patch_state: "TEST".into(),
                    file_sha256: hashes.file_sha256,
                    text_sha256: hashes.text_sha256,
                }],
            },
            targets,
            candidates: Vec::new(),
            diagnostics: Vec::new(),
        }
    }

    fn target(kind: &str, rva: &str, feature: &str) -> TargetSpec {
        TargetSpec {
            id: "target".into(),
            semantic: "test".into(),
            kind: kind.into(),
            section: ".text".into(),
            rva: rva.into(),
            feature: FeatureSpec {
                section: ".text".into(),
                rva: "0x1000".into(),
                bytes: feature.into(),
                masked_ranges: Vec::new(),
            },
            expected_bytes: None,
            expected_target_rva: None,
            import_name: None,
            expected_owner_module: None,
            operand_offset: None,
            vtable_target_rvas: Vec::new(),
            abi: "test".into(),
            thread: "test".into(),
            evidence_label: "STATIC_CONFIRMED".into(),
            evidence_sources: vec!["test".into()],
        }
    }

    #[test]
    fn hex_parser_accepts_explicit_hex_and_rejects_overflow() {
        assert_eq!(parse_u32("0x5110AB", "test").unwrap(), 0x5110AB);
        assert!(parse_u32("0x1FFFFFFFF", "test").is_err());
    }

    #[test]
    fn feature_mask_ranges_are_bounded() {
        let feature = FeatureSpec {
            section: ".text".into(),
            rva: "0x1000".into(),
            bytes: "AA BB CC".into(),
            masked_ranges: vec![MaskedRange {
                offset: 1,
                length: 1,
            }],
        };
        assert_eq!(feature.pattern().unwrap().mask, vec![0xFF, 0, 0xFF]);
        let invalid = FeatureSpec {
            masked_ranges: vec![MaskedRange {
                offset: 2,
                length: 2,
            }],
            ..feature
        };
        assert!(invalid.pattern().is_err());
    }

    #[test]
    fn signature_capture_and_feature_lengths_must_match() {
        let image = minimal_pe(&[0xAA, 0xBB, 0xCC]);
        let mut signature = target("signature", "0x1000", "AABB");
        signature.expected_bytes = Some("AABBCC".into());
        let ledger = ledger_for(&image, vec![signature]);
        let error = ledger.validate_schema().unwrap_err().to_string();
        assert!(error.contains("expectedBytes length"));
    }

    #[test]
    fn wrong_machine_and_unknown_variant_fail_closed() {
        let image = minimal_pe(&[0x90]);
        let mut ledger = ledger_for(&image, Vec::new());
        ledger.image.machine = "0x8664".into();
        assert!(ledger.inspect(&image, "known", "hash").is_err());
        ledger.image.machine = "0x014C".into();
        assert!(ledger.inspect(&image, "not-known", "hash").is_err());
    }

    #[test]
    fn zero_and_multiple_feature_matches_fail_closed() {
        let zero_image = minimal_pe(&[0x90, 0x90]);
        let zero_ledger = ledger_for(&zero_image, vec![target("signature", "0x1000", "AABB")]);
        assert!(zero_ledger.inspect(&zero_image, "known", "hash").is_err());

        let multi_image = minimal_pe(&[0xAA, 0xBB, 0xAA, 0xBB]);
        let multi_ledger = ledger_for(&multi_image, vec![target("signature", "0x1000", "AABB")]);
        assert!(multi_ledger.inspect(&multi_image, "known", "hash").is_err());
    }

    #[test]
    fn wrong_rel32_and_iat_operands_fail_closed() {
        let rel_image = minimal_pe(&[0xE8, 0, 0, 0, 0]);
        let mut rel_target = target("relativeCall", "0x1000", "E800000000");
        rel_target.expected_target_rva = Some("0x2000".into());
        assert!(validate_relative_call(&rel_image, &rel_target, 0x1000).is_err());

        let mut iat_image = minimal_pe(&[0xFF, 0x15, 0x34, 0x12, 0x40, 0]);
        iat_image.imports.insert("wanted".into(), 0x1100);
        let mut iat_target = target("iatCall", "0x1000", "FF1534124000");
        iat_target.import_name = Some("wanted".into());
        assert!(validate_iat_call(&iat_image, &iat_target, 0x1000).is_err());
    }
}
