use std::fs::{self, OpenOptions};
use std::io::Write;
use std::path::Path;

use serde::{Deserialize, Serialize};

use crate::active_catalog::{
    ACTIVE_CATALOG_IDENTITY, ACTIVE_CATALOG_SCHEMA_VERSION, ACTIVE_CATALOG_SHA256,
    ACTIVE_PLAN_IDENTITY, ACTIVE_PLAN_SCHEMA_VERSION, ACTIVE_PLAN_SHA256, ACTIVE_SLOT_COUNT,
    ActiveCatalog, ActivePlan, FIXED_TARGET_SHA256, expected_hook_slots,
};
use crate::artifact::file_sha256;
use crate::error::{AppError, AppResult, UsageContext};
use crate::pe::{PeImage, SectionInfo, sha256_hex};

pub const TARGET_EQUIVALENCE_SCHEMA_VERSION: u32 = 1;
pub const TARGET_EQUIVALENCE_IDENTITY: &str = "IsaacCsT0-UnpackedTargetEquivalence-v1";
pub const TARGET_EQUIVALENCE_STATUS: &str = "PASS_EQUIVALENT_READ_ONLY";
pub const APPROVED_UNPACKED_TARGET_SHA256: &str =
    "1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C";
pub const FIXED_LOOP_LEDGER_SHA256: &str =
    "8E4C5676358EBBDA831BCC847DE129DF1A9C0D5470F01FAC90F4A298E2E83858";
pub const FIXED_TARGET_LEDGER_SHA256: &str =
    "23AB89605DCB41EC63823B4F762B32F93BEBC9BD00B7ECB7BE522ACBC3CCBE32";
const TRANSFORMATION: &str = "REMOVE_EXACT_TERMINAL_BIND_SECTION_WITH_APPROVED_HEADER_REWRITE_ONLY";
const EXPECTED_ENTRY_POINT_RVA: u32 = 0x006E_FC46;
const EXPECTED_HEADER_SHA256: &str =
    "642A7B1DD6F2FF0B0464ADCB5B744C8BA428299112EF6DA902F7FC323BAB4DF2";
const EXPECTED_IMPORT_COUNT: usize = 622;
const EXPECTED_IMPORT_BYTES: usize = 31_328;
const EXPECTED_IMPORT_DIGEST: &str =
    "C966E88D1BF328CC0A8D79466C53C96FB07E55551D8298F2829AF7B26003E7E0";
const EXPECTED_SLOT_RAW_DIGEST: &str =
    "4A655B0D139269A2D1E58EBF7075658C9384971856DE271A6907C405D622FBDA";
const RETAINED_SECTION_HASHES: [(&str, &str); 5] = [
    (
        ".text",
        "197175428117EC352324F22FA44CF42D60511F0A4887BF6DC5FECFBF9A6EE758",
    ),
    (
        ".rdata",
        "D073F542E476C24D42BA2CEE67A44B5EC56E650AEFEE3EDCA72509CD0681A3E6",
    ),
    (
        ".data",
        "BB7C0D2BB43FB3B10AAF37F5AD994AE42B15944785704F4E6FF962AAB9E3FAA8",
    ),
    (
        ".rsrc",
        "5CC85CC63BD87B9208C287180DD8D1B9B2798AC46214B97E6AB8ECD389207E01",
    ),
    (
        ".reloc",
        "EF8EA5CA7695403957043292A59E3898BB27E275700287C3D02F5D7686EA5022",
    ),
];

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct RequiredImport {
    pub module: String,
    pub symbol: String,
    pub iat_rva: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct RetainedSectionDigest {
    pub name: String,
    pub raw_sha256: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct ColOwnerBinding {
    pub owner: String,
    pub vtable_rva: String,
    pub col_rva: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct TargetEquivalenceDocument {
    pub schema_version: u32,
    pub equivalence_identity: String,
    pub status: String,
    pub canonical_target_sha256: String,
    pub observed_target_sha256: String,
    pub transformation: String,
    pub loop_ledger_sha256: String,
    pub target_ledger_sha256: String,
    pub active_catalog_sha256: String,
    pub active_plan_sha256: String,
    pub machine: String,
    pub timestamp: String,
    pub preferred_image_base: String,
    pub entry_point_rva: String,
    pub canonical_image_size: String,
    pub observed_image_size: String,
    pub text_sha256: String,
    pub observed_header_sha256: String,
    pub retained_section_digests: Vec<RetainedSectionDigest>,
    pub canonical_sections: Vec<SectionInfo>,
    pub observed_sections: Vec<SectionInfo>,
    pub removed_section: SectionInfo,
    pub hook_slots_sha256: String,
    pub hook_slot_raw_sha256: String,
    pub hook_slot_count: usize,
    pub rel32_call_count: usize,
    pub image_pointer_count: usize,
    pub import_pointer_count: usize,
    pub required_imports: Vec<RequiredImport>,
    pub import_count: usize,
    pub normalized_import_bytes: usize,
    pub normalized_import_sha256: String,
    pub col_owner_bindings: Vec<ColOwnerBinding>,
    pub static_proof_complete: bool,
    pub passive_attestation_allowed: bool,
    pub eligible_for_active_gate: bool,
    pub active_input_allowed: bool,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct ValidatedTargetEquivalence {
    pub canonical_target_sha256: String,
    pub observed_target_sha256: String,
    pub document_sha256: String,
    pub passive_attestation_allowed: bool,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
struct LoopLedger {
    schema_version: u32,
    fixed_variant: LoopFixedVariant,
    baseline: LoopBaseline,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
struct LoopFixedVariant {
    file_name: String,
    file_sha256: String,
    text_sha256: String,
    machine: String,
    timestamp: String,
    image_size: String,
    preferred_image_base: String,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
struct LoopBaseline {
    sha256: String,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
struct TargetLedger {
    schema_version: u32,
    image: TargetImage,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
struct TargetImage {
    file_name: String,
    machine: String,
    timestamp: String,
    image_size: String,
    preferred_image_base: String,
    sections: Vec<TargetSection>,
    variants: Vec<TargetVariant>,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
struct TargetSection {
    name: String,
    virtual_size: String,
    rva: String,
    raw_size: String,
    raw_offset: String,
    characteristics: String,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
struct TargetVariant {
    file_sha256: String,
    text_sha256: String,
}

#[allow(clippy::too_many_arguments)]
pub fn create_target_equivalence(
    game_exe: &Path,
    loop_ledger: &Path,
    target_ledger: &Path,
    active_catalog: &Path,
    active_plan: &Path,
    output: &Path,
) -> AppResult<TargetEquivalenceDocument> {
    if output.exists() {
        return Err(AppError::usage(format!(
            "refusing to overwrite {}",
            output.display()
        )));
    }
    let document = build_document(
        game_exe,
        loop_ledger,
        target_ledger,
        active_catalog,
        active_plan,
    )?;
    write_new_json(output, &document)?;
    Ok(document)
}

#[allow(clippy::too_many_arguments)]
pub fn validate_target_equivalence(
    document_path: &Path,
    game_exe: &Path,
    loop_ledger: &Path,
    target_ledger: &Path,
    active_catalog: &Path,
    active_plan: &Path,
) -> AppResult<ValidatedTargetEquivalence> {
    let bytes = fs::read(document_path).usage_context(format!(
        "cannot read target equivalence {}",
        document_path.display()
    ))?;
    let stored: TargetEquivalenceDocument =
        serde_json::from_slice(&bytes).usage_context(format!(
            "cannot parse target equivalence {}",
            document_path.display()
        ))?;
    validate_document_invariants(&stored)?;
    let expected = build_document(
        game_exe,
        loop_ledger,
        target_ledger,
        active_catalog,
        active_plan,
    )?;
    if stored != expected {
        return Err(AppError::evidence(
            "target equivalence no longer matches the observed executable or frozen bundle",
        ));
    }
    Ok(ValidatedTargetEquivalence {
        canonical_target_sha256: stored.canonical_target_sha256,
        observed_target_sha256: stored.observed_target_sha256,
        document_sha256: sha256_hex(&bytes),
        passive_attestation_allowed: stored.passive_attestation_allowed,
    })
}

#[allow(clippy::too_many_arguments)]
fn build_document(
    game_exe: &Path,
    loop_ledger_path: &Path,
    target_ledger_path: &Path,
    active_catalog_path: &Path,
    active_plan_path: &Path,
) -> AppResult<TargetEquivalenceDocument> {
    require_file_hash(loop_ledger_path, FIXED_LOOP_LEDGER_SHA256, "loop ledger")?;
    require_file_hash(
        target_ledger_path,
        FIXED_TARGET_LEDGER_SHA256,
        "target ledger",
    )?;
    require_file_hash(active_catalog_path, ACTIVE_CATALOG_SHA256, "active catalog")?;
    require_file_hash(active_plan_path, ACTIVE_PLAN_SHA256, "active plan")?;

    let loop_ledger: LoopLedger = read_json(loop_ledger_path, "loop ledger")?;
    let target_ledger: TargetLedger = read_json(target_ledger_path, "target ledger")?;
    let catalog: ActiveCatalog = read_json(active_catalog_path, "active catalog")?;
    let plan: ActivePlan = read_json(active_plan_path, "active plan")?;
    validate_frozen_bundle(&loop_ledger, &target_ledger, &catalog, &plan)?;

    let canonical_sections = target_ledger
        .image
        .sections
        .iter()
        .map(parse_section)
        .collect::<AppResult<Vec<_>>>()?;
    let image = PeImage::read(game_exe).map_err(|error| AppError::evidence(error.to_string()))?;
    let hashes = image
        .hashes()
        .map_err(|error| AppError::evidence(error.to_string()))?;
    validate_observed_image(
        &image,
        &hashes.file_sha256,
        &hashes.text_sha256,
        &loop_ledger,
        &canonical_sections,
    )?;
    let (
        rel32_call_count,
        image_pointer_count,
        import_pointer_count,
        required_imports,
        hook_slot_raw_sha256,
    ) = validate_hook_slots(&image, &catalog)?;
    let retained_section_digests = validate_retained_section_hashes(&image)?;
    let (import_count, normalized_import_bytes, normalized_import_sha256) =
        validate_import_digest(&image)?;
    let col_owner_bindings = validate_col_owners(&image)?;
    let observed_header_sha256 = sha256_hex(
        image
            .bytes()
            .get(..0x400)
            .ok_or_else(|| AppError::evidence("unpacked PE header is truncated"))?,
    );
    if observed_header_sha256 != EXPECTED_HEADER_SHA256 {
        return Err(AppError::evidence("unpacked PE header digest mismatch"));
    }

    let hook_slots_sha256 = sha256_hex(
        &serde_json::to_vec(&catalog.hook_slots)
            .usage_context("cannot serialize hook slots for target equivalence")?,
    );
    let removed_section = canonical_sections
        .last()
        .cloned()
        .ok_or_else(|| AppError::evidence("canonical section list is empty"))?;
    let document = TargetEquivalenceDocument {
        schema_version: TARGET_EQUIVALENCE_SCHEMA_VERSION,
        equivalence_identity: TARGET_EQUIVALENCE_IDENTITY.to_owned(),
        status: TARGET_EQUIVALENCE_STATUS.to_owned(),
        canonical_target_sha256: FIXED_TARGET_SHA256.to_owned(),
        observed_target_sha256: APPROVED_UNPACKED_TARGET_SHA256.to_owned(),
        transformation: TRANSFORMATION.to_owned(),
        loop_ledger_sha256: FIXED_LOOP_LEDGER_SHA256.to_owned(),
        target_ledger_sha256: FIXED_TARGET_LEDGER_SHA256.to_owned(),
        active_catalog_sha256: ACTIVE_CATALOG_SHA256.to_owned(),
        active_plan_sha256: ACTIVE_PLAN_SHA256.to_owned(),
        machine: format!("0x{:04X}", image.machine),
        timestamp: format!("0x{:08X}", image.timestamp),
        preferred_image_base: format!("0x{:08X}", image.image_base),
        entry_point_rva: format!("0x{:08X}", image.entry_point_rva),
        canonical_image_size: format!(
            "0x{:08X}",
            parse_hex(
                &loop_ledger.fixed_variant.image_size,
                "canonical image size"
            )?
        ),
        observed_image_size: format!("0x{:08X}", image.image_size),
        text_sha256: hashes.text_sha256,
        observed_header_sha256,
        retained_section_digests,
        canonical_sections,
        observed_sections: image.sections.clone(),
        removed_section,
        hook_slots_sha256,
        hook_slot_raw_sha256,
        hook_slot_count: catalog.hook_slots.len(),
        rel32_call_count,
        image_pointer_count,
        import_pointer_count,
        required_imports,
        import_count,
        normalized_import_bytes,
        normalized_import_sha256,
        col_owner_bindings,
        static_proof_complete: true,
        passive_attestation_allowed: true,
        eligible_for_active_gate: false,
        active_input_allowed: false,
    };
    validate_document_invariants(&document)?;
    Ok(document)
}

fn validate_frozen_bundle(
    loop_ledger: &LoopLedger,
    target_ledger: &TargetLedger,
    catalog: &ActiveCatalog,
    plan: &ActivePlan,
) -> AppResult<()> {
    let fixed = &loop_ledger.fixed_variant;
    if loop_ledger.schema_version != 1
        || target_ledger.schema_version != 1
        || fixed.file_name != "isaac-ng.exe"
        || target_ledger.image.file_name != fixed.file_name
        || fixed.file_sha256 != FIXED_TARGET_SHA256
        || loop_ledger.baseline.sha256 != FIXED_TARGET_LEDGER_SHA256
        || target_ledger.image.machine != fixed.machine
        || target_ledger.image.timestamp != fixed.timestamp
        || target_ledger.image.image_size != fixed.image_size
        || target_ledger.image.preferred_image_base != fixed.preferred_image_base
        || !target_ledger.image.variants.iter().any(|variant| {
            variant.file_sha256 == fixed.file_sha256 && variant.text_sha256 == fixed.text_sha256
        })
    {
        return Err(AppError::evidence(
            "frozen loop and target ledgers are not the approved canonical pair",
        ));
    }
    if catalog.schema_version != ACTIVE_CATALOG_SCHEMA_VERSION
        || catalog.catalog_identity != ACTIVE_CATALOG_IDENTITY
        || catalog.target_sha256 != FIXED_TARGET_SHA256
        || catalog.hook_slots != expected_hook_slots()
        || catalog.total_slot_count != ACTIVE_SLOT_COUNT
        || catalog.production_active_input_allowed
        || plan.schema_version != ACTIVE_PLAN_SCHEMA_VERSION
        || plan.plan_identity != ACTIVE_PLAN_IDENTITY
        || plan.target_sha256 != FIXED_TARGET_SHA256
        || plan.catalog_sha256 != ACTIVE_CATALOG_SHA256
        || plan.total_slot_count != ACTIVE_SLOT_COUNT
        || plan.production_active_input_allowed
    {
        return Err(AppError::evidence(
            "active catalog or plan is not the approved frozen v2 bundle",
        ));
    }
    Ok(())
}

fn validate_observed_image(
    image: &PeImage,
    file_sha256: &str,
    text_sha256: &str,
    loop_ledger: &LoopLedger,
    canonical_sections: &[SectionInfo],
) -> AppResult<()> {
    let fixed = &loop_ledger.fixed_variant;
    if !file_sha256.eq_ignore_ascii_case(APPROVED_UNPACKED_TARGET_SHA256) {
        return Err(AppError::evidence(format!(
            "unpacked target SHA-256 is not the explicitly approved pair member: {file_sha256}"
        )));
    }
    if image.machine != parse_hex_u16(&fixed.machine, "machine")?
        || image.timestamp != parse_hex(&fixed.timestamp, "timestamp")?
        || image.image_base != parse_hex(&fixed.preferred_image_base, "image base")?
        || image.entry_point_rva != EXPECTED_ENTRY_POINT_RVA
        || !text_sha256.eq_ignore_ascii_case(&fixed.text_sha256)
    {
        return Err(AppError::evidence(
            "unpacked target code fingerprint or PE identity differs from the canonical target",
        ));
    }
    validate_bindless_sections(
        canonical_sections,
        &image.sections,
        image.image_size,
        image.bytes().len(),
        parse_hex(&fixed.image_size, "canonical image size")?,
    )
}

fn validate_bindless_sections(
    canonical: &[SectionInfo],
    observed: &[SectionInfo],
    observed_image_size: u32,
    observed_file_size: usize,
    canonical_image_size: u32,
) -> AppResult<()> {
    if canonical.len() != 6
        || observed.len() != 5
        || canonical[..5] != *observed
        || canonical[5].name != ".bind"
    {
        return Err(AppError::evidence(
            "only exact removal of the canonical terminal .bind section is allowed",
        ));
    }
    let bind = &canonical[5];
    let bind_end = align_up(
        bind.rva
            .checked_add(bind.virtual_size.max(bind.raw_size))
            .ok_or_else(|| AppError::evidence("canonical .bind range overflow"))?,
        0x1000,
    )?;
    let preceding_end = canonical[4]
        .rva
        .checked_add(canonical[4].virtual_size.max(canonical[4].raw_size))
        .ok_or_else(|| AppError::evidence("canonical preceding section range overflow"))?;
    if align_up(preceding_end, 0x1000)? != bind.rva
        || observed_image_size != bind.rva
        || canonical_image_size != bind_end
        || observed_file_size != usize::try_from(bind.raw_offset).unwrap()
    {
        return Err(AppError::evidence(
            "bindless target image/file boundary does not equal the canonical .bind boundary",
        ));
    }
    Ok(())
}

fn validate_hook_slots(
    image: &PeImage,
    catalog: &ActiveCatalog,
) -> AppResult<(usize, usize, usize, Vec<RequiredImport>, String)> {
    if catalog.hook_slots.len() != ACTIVE_SLOT_COUNT {
        return Err(AppError::evidence("active hook-slot count is not 45"));
    }
    let mut rel32 = 0;
    let mut pointers = 0;
    let mut imports = 0;
    let mut required_imports = Vec::new();
    let mut ordered_raw_bytes = Vec::new();
    for (index, slot) in catalog.hook_slots.iter().enumerate() {
        if slot.ordinal != index as u32 {
            return Err(AppError::evidence(
                "active hook-slot ordinals are not contiguous",
            ));
        }
        let rva = parse_hex(&slot.rva, "hook-slot RVA")?;
        ordered_raw_bytes.extend_from_slice(
            image
                .bytes_at_rva(rva, slot.width as usize)
                .map_err(pe_evidence)?,
        );
        match slot.patch_kind.as_str() {
            "REL32_CALL" => {
                rel32 += 1;
                if slot.width != 5 || section_name_for(image, rva, 5)? != ".text" {
                    return Err(AppError::evidence(
                        "REL32 hook slot is not a five-byte .text call",
                    ));
                }
                let expected_target = parse_hex(
                    slot.expected_target_rva.as_deref().ok_or_else(|| {
                        AppError::evidence("REL32 hook slot has no expected target")
                    })?,
                    "REL32 expected target",
                )?;
                validate_rel32(
                    image.bytes_at_rva(rva, 5).map_err(pe_evidence)?,
                    rva,
                    expected_target,
                )?;
                if section_name_for(image, expected_target, 1)? != ".text" {
                    return Err(AppError::evidence("REL32 target is outside .text"));
                }
            }
            "IMAGE_POINTER" => {
                pointers += 1;
                if slot.width != 4 || section_name_for(image, rva, 4)? != ".rdata" {
                    return Err(AppError::evidence(
                        "image pointer hook slot is not in .rdata",
                    ));
                }
                let expected_target = parse_hex(
                    slot.expected_target_rva.as_deref().ok_or_else(|| {
                        AppError::evidence("image pointer hook slot has no expected target")
                    })?,
                    "image pointer expected target",
                )?;
                if section_name_for(image, expected_target, 1)? != ".text" {
                    return Err(AppError::evidence("image pointer target is outside .text"));
                }
                let actual = u32::from_le_bytes(
                    image
                        .bytes_at_rva(rva, 4)
                        .map_err(pe_evidence)?
                        .try_into()
                        .unwrap(),
                );
                let expected = image
                    .image_base
                    .checked_add(expected_target)
                    .ok_or_else(|| AppError::evidence("image pointer target overflow"))?;
                if actual != expected {
                    return Err(AppError::evidence(format!(
                        "image pointer {} target mismatch",
                        slot.id
                    )));
                }
            }
            "IMPORT_POINTER" => {
                imports += 1;
                if slot.width != 4 || section_name_for(image, rva, 4)? != ".rdata" {
                    return Err(AppError::evidence(
                        "import pointer hook slot is not in .rdata",
                    ));
                }
                let expected_symbol = slot.expected_symbol.as_deref().ok_or_else(|| {
                    AppError::evidence("import pointer hook slot has no expected symbol")
                })?;
                let (module, symbol) = expected_symbol.split_once('!').ok_or_else(|| {
                    AppError::evidence("import pointer symbol is not module-qualified")
                })?;
                let actual_rva = image.imports.get(symbol).copied().ok_or_else(|| {
                    AppError::evidence(format!("required import {expected_symbol} is missing"))
                })?;
                let actual_module = image.import_modules.get(symbol).ok_or_else(|| {
                    AppError::evidence(format!("required import {expected_symbol} has no owner"))
                })?;
                if actual_rva != rva
                    || !actual_module
                        .trim_end_matches(".dll")
                        .eq_ignore_ascii_case(module)
                {
                    return Err(AppError::evidence(format!(
                        "required import {expected_symbol} IAT or module owner mismatch"
                    )));
                }
                required_imports.push(RequiredImport {
                    module: actual_module.to_ascii_uppercase(),
                    symbol: symbol.to_owned(),
                    iat_rva: format!("0x{rva:08X}"),
                });
            }
            _ => return Err(AppError::evidence("unknown active hook-slot patch kind")),
        }
    }
    if (rel32, pointers, imports) != (33, 9, 3) {
        return Err(AppError::evidence(
            "active hook-slot kind counts are not the frozen 33+9+3 split",
        ));
    }
    let raw_digest = sha256_hex(&ordered_raw_bytes);
    if raw_digest != EXPECTED_SLOT_RAW_DIGEST || ordered_raw_bytes.len() != 213 {
        return Err(AppError::evidence(
            "ordered 45-slot raw-byte digest mismatch",
        ));
    }
    Ok((rel32, pointers, imports, required_imports, raw_digest))
}

fn validate_rel32(bytes: &[u8], call_rva: u32, expected_target: u32) -> AppResult<()> {
    if bytes.len() != 5 || bytes[0] != 0xE8 {
        return Err(AppError::evidence("REL32 hook slot opcode mismatch"));
    }
    let displacement = i32::from_le_bytes(bytes[1..5].try_into().unwrap());
    let actual = i64::from(call_rva) + 5 + i64::from(displacement);
    if actual != i64::from(expected_target) {
        return Err(AppError::evidence("REL32 hook slot target mismatch"));
    }
    Ok(())
}

fn section_name_for(image: &PeImage, rva: u32, size: u32) -> AppResult<&str> {
    image
        .sections
        .iter()
        .find(|section| {
            let span = section.virtual_size.max(section.raw_size);
            rva >= section.rva
                && rva
                    .checked_add(size)
                    .is_some_and(|end| end <= section.rva.saturating_add(span))
        })
        .map(|section| section.name.as_str())
        .ok_or_else(|| AppError::evidence(format!("RVA 0x{rva:08X} has no section owner")))
}

fn validate_retained_section_hashes(image: &PeImage) -> AppResult<Vec<RetainedSectionDigest>> {
    let mut digests = Vec::with_capacity(RETAINED_SECTION_HASHES.len());
    for (name, expected) in RETAINED_SECTION_HASHES {
        let section = image
            .section(name)
            .map_err(|error| AppError::evidence(error.to_string()))?;
        let actual = sha256_hex(
            image
                .bytes_at_rva(section.rva, section.raw_size as usize)
                .map_err(pe_evidence)?,
        );
        if actual != expected {
            return Err(AppError::evidence(format!(
                "retained section {name} raw digest mismatch"
            )));
        }
        digests.push(RetainedSectionDigest {
            name: name.to_owned(),
            raw_sha256: actual,
        });
    }
    Ok(digests)
}

fn validate_import_digest(image: &PeImage) -> AppResult<(usize, usize, String)> {
    let normalized = image
        .import_entries
        .iter()
        .map(|entry| {
            format!(
                "{}|0x{:08X}|{}",
                entry.module.to_ascii_uppercase(),
                entry.iat_rva,
                entry.symbol
            )
        })
        .collect::<Vec<_>>()
        .join("\n");
    let digest = sha256_hex(normalized.as_bytes());
    let first = image.import_entries.first().map(|entry| {
        format!(
            "{}|0x{:08X}|{}",
            entry.module.to_ascii_uppercase(),
            entry.iat_rva,
            entry.symbol
        )
    });
    let last = image.import_entries.last().map(|entry| {
        format!(
            "{}|0x{:08X}|{}",
            entry.module.to_ascii_uppercase(),
            entry.iat_rva,
            entry.symbol
        )
    });
    if image.import_entries.len() != EXPECTED_IMPORT_COUNT
        || normalized.len() != EXPECTED_IMPORT_BYTES
        || digest != EXPECTED_IMPORT_DIGEST
        || first.as_deref()
            != Some("STEAM_API.DLL|0x007189FC|SteamInternal_FindOrCreateUserInterface")
        || last.as_deref()
            != Some("API-MS-WIN-CRT-LOCALE-L1-1-0.DLL|0x007187F0|_configthreadlocale")
    {
        return Err(AppError::evidence(
            "ordered import descriptor/thunk digest mismatch",
        ));
    }
    Ok((image.import_entries.len(), normalized.len(), digest))
}

fn validate_col_owners(image: &PeImage) -> AppResult<Vec<ColOwnerBinding>> {
    let expected = [
        ("KEYBOARD", 0x007A_1678, 0x007B_191C),
        ("GAMEPAD", 0x0078_2C80, 0x007B_0460),
        ("PLAYER_DERIVED", 0x0076_BDD0, 0x007A_E31C),
    ];
    let mut bindings = Vec::with_capacity(expected.len());
    for (owner, vtable_rva, col_rva) in expected {
        if section_name_for(image, vtable_rva, 4)? != ".rdata"
            || section_name_for(image, col_rva, 4)? != ".rdata"
        {
            return Err(AppError::evidence("COL or vtable owner is outside .rdata"));
        }
        let col_pointer_rva = vtable_rva
            .checked_sub(4)
            .ok_or_else(|| AppError::evidence("vtable COL pointer underflow"))?;
        let actual = u32::from_le_bytes(
            image
                .bytes_at_rva(col_pointer_rva, 4)
                .map_err(pe_evidence)?
                .try_into()
                .unwrap(),
        );
        if actual != image.image_base + col_rva {
            return Err(AppError::evidence(format!(
                "{owner} vtable COL binding mismatch"
            )));
        }
        bindings.push(ColOwnerBinding {
            owner: owner.to_owned(),
            vtable_rva: format!("0x{vtable_rva:08X}"),
            col_rva: format!("0x{col_rva:08X}"),
        });
    }
    Ok(bindings)
}

fn validate_document_invariants(document: &TargetEquivalenceDocument) -> AppResult<()> {
    if document.schema_version != TARGET_EQUIVALENCE_SCHEMA_VERSION
        || document.equivalence_identity != TARGET_EQUIVALENCE_IDENTITY
        || document.status != TARGET_EQUIVALENCE_STATUS
        || document.canonical_target_sha256 != FIXED_TARGET_SHA256
        || document.observed_target_sha256 != APPROVED_UNPACKED_TARGET_SHA256
        || document.transformation != TRANSFORMATION
        || document.loop_ledger_sha256 != FIXED_LOOP_LEDGER_SHA256
        || document.target_ledger_sha256 != FIXED_TARGET_LEDGER_SHA256
        || document.active_catalog_sha256 != ACTIVE_CATALOG_SHA256
        || document.active_plan_sha256 != ACTIVE_PLAN_SHA256
        || document.hook_slot_count != 45
        || document.rel32_call_count != 33
        || document.image_pointer_count != 9
        || document.import_pointer_count != 3
        || document.required_imports.len() != 3
        || document.observed_header_sha256 != EXPECTED_HEADER_SHA256
        || document.retained_section_digests.len() != 5
        || document.hook_slot_raw_sha256 != EXPECTED_SLOT_RAW_DIGEST
        || document.import_count != EXPECTED_IMPORT_COUNT
        || document.normalized_import_bytes != EXPECTED_IMPORT_BYTES
        || document.normalized_import_sha256 != EXPECTED_IMPORT_DIGEST
        || document.col_owner_bindings.len() != 3
        || !document.static_proof_complete
        || !document.passive_attestation_allowed
        || document.eligible_for_active_gate
        || document.active_input_allowed
    {
        return Err(AppError::evidence(
            "target equivalence document is not a passive-only complete proof",
        ));
    }
    Ok(())
}

fn parse_section(section: &TargetSection) -> AppResult<SectionInfo> {
    Ok(SectionInfo {
        name: section.name.clone(),
        virtual_size: parse_hex(&section.virtual_size, "section virtual size")?,
        rva: parse_hex(&section.rva, "section RVA")?,
        raw_size: parse_hex(&section.raw_size, "section raw size")?,
        raw_offset: parse_hex(&section.raw_offset, "section raw offset")?,
        characteristics: parse_hex(&section.characteristics, "section characteristics")?,
    })
}

fn parse_hex(value: &str, label: &str) -> AppResult<u32> {
    let digits = value
        .strip_prefix("0x")
        .or_else(|| value.strip_prefix("0X"))
        .unwrap_or(value);
    u32::from_str_radix(digits, 16)
        .map_err(|_| AppError::usage(format!("invalid {label}: {value}")))
}

fn parse_hex_u16(value: &str, label: &str) -> AppResult<u16> {
    u16::try_from(parse_hex(value, label)?)
        .map_err(|_| AppError::usage(format!("invalid {label}: {value}")))
}

fn align_up(value: u32, alignment: u32) -> AppResult<u32> {
    value
        .checked_add(alignment - 1)
        .map(|sum| sum & !(alignment - 1))
        .ok_or_else(|| AppError::evidence("PE alignment overflow"))
}

fn pe_evidence(error: crate::pe::PeError) -> AppError {
    AppError::evidence(error.to_string())
}

fn require_file_hash(path: &Path, expected: &str, label: &str) -> AppResult<()> {
    let actual = file_sha256(path)?;
    if !actual.eq_ignore_ascii_case(expected) {
        return Err(AppError::evidence(format!(
            "{label} SHA-256 mismatch: expected {expected}, got {actual}"
        )));
    }
    Ok(())
}

fn read_json<T: for<'de> Deserialize<'de>>(path: &Path, label: &str) -> AppResult<T> {
    serde_json::from_slice(
        &fs::read(path).usage_context(format!("cannot read {label} {}", path.display()))?,
    )
    .usage_context(format!("cannot parse {label} {}", path.display()))
}

fn write_new_json<T: Serialize>(path: &Path, value: &T) -> AppResult<()> {
    let bytes = serde_json::to_vec_pretty(value).usage_context("cannot serialize JSON")?;
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
    use super::*;

    fn section(
        name: &str,
        rva: u32,
        virtual_size: u32,
        raw_offset: u32,
        raw_size: u32,
    ) -> SectionInfo {
        SectionInfo {
            name: name.to_owned(),
            virtual_size,
            rva,
            raw_size,
            raw_offset,
            characteristics: 0x4000_0040,
        }
    }

    #[test]
    fn bindless_shape_accepts_only_exact_terminal_removal() {
        let canonical = vec![
            section(".text", 0x1000, 0x1000, 0x400, 0x1000),
            section(".rdata", 0x2000, 0x1000, 0x1400, 0x1000),
            section(".data", 0x3000, 0x1000, 0x2400, 0x1000),
            section(".rsrc", 0x4000, 0x1000, 0x3400, 0x1000),
            section(".reloc", 0x5000, 0x1000, 0x4400, 0x1000),
            section(".bind", 0x6000, 0x1800, 0x5400, 0x1800),
        ];
        assert!(
            validate_bindless_sections(&canonical, &canonical[..5], 0x6000, 0x5400, 0x8000).is_ok()
        );
        let mut changed = canonical[..5].to_vec();
        changed[2].rva += 4;
        assert!(validate_bindless_sections(&canonical, &changed, 0x6000, 0x5400, 0x8000).is_err());
        assert!(
            validate_bindless_sections(&canonical, &canonical[..5], 0x7000, 0x5400, 0x8000)
                .is_err()
        );
        assert!(
            validate_bindless_sections(&canonical, &canonical[..5], 0x6000, 0x5401, 0x8000)
                .is_err()
        );
    }

    #[test]
    fn rel32_requires_call_opcode_and_exact_target() {
        let call_rva = 0x1000_u32;
        let target = 0x1800_u32;
        let displacement = (i64::from(target) - i64::from(call_rva) - 5) as i32;
        let mut bytes = vec![0xE8];
        bytes.extend_from_slice(&displacement.to_le_bytes());
        assert!(validate_rel32(&bytes, call_rva, target).is_ok());
        bytes[0] = 0xE9;
        assert!(validate_rel32(&bytes, call_rva, target).is_err());
        bytes[0] = 0xE8;
        assert!(validate_rel32(&bytes, call_rva, target + 1).is_err());
    }

    #[test]
    fn create_new_writer_does_not_overwrite() {
        let directory = tempfile::tempdir().unwrap();
        let path = directory.path().join("equivalence.json");
        write_new_json(&path, &serde_json::json!({"first": true})).unwrap();
        let before = fs::read(&path).unwrap();
        assert!(write_new_json(&path, &serde_json::json!({"second": true})).is_err());
        assert_eq!(fs::read(path).unwrap(), before);
    }
}
