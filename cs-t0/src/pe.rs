use std::collections::BTreeMap;
use std::fs;
use std::path::Path;

use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use thiserror::Error;

const DOS_SIGNATURE: u16 = 0x5A4D;
const PE_SIGNATURE: u32 = 0x0000_4550;
const PE32_MAGIC: u16 = 0x010B;
const MAX_SECTIONS: usize = 96;
const IMAGE_ORDINAL_FLAG32: u32 = 0x8000_0000;

type ParsedImports = (
    BTreeMap<String, u32>,
    BTreeMap<String, String>,
    Vec<ImportInfo>,
);

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct SectionInfo {
    pub name: String,
    pub virtual_size: u32,
    pub rva: u32,
    pub raw_size: u32,
    pub raw_offset: u32,
    pub characteristics: u32,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ImportInfo {
    pub module: String,
    pub symbol: String,
    pub iat_rva: u32,
}

#[derive(Clone, Debug)]
pub struct PeImage {
    bytes: Vec<u8>,
    pub machine: u16,
    pub timestamp: u32,
    pub image_size: u32,
    pub image_base: u32,
    pub entry_point_rva: u32,
    pub sections: Vec<SectionInfo>,
    pub imports: BTreeMap<String, u32>,
    pub import_modules: BTreeMap<String, String>,
    pub import_entries: Vec<ImportInfo>,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ImageHashes {
    pub file_sha256: String,
    pub text_sha256: String,
}

#[derive(Debug, Error)]
pub enum PeError {
    #[error("cannot read PE: {0}")]
    Io(#[from] std::io::Error),
    #[error("invalid PE: {0}")]
    Invalid(&'static str),
    #[error("invalid PE: {0}")]
    Detail(String),
}

impl PeImage {
    pub fn read(path: &Path) -> Result<Self, PeError> {
        Self::parse(fs::read(path)?)
    }

    pub fn parse(bytes: Vec<u8>) -> Result<Self, PeError> {
        if bytes.len() < 0x40 || u16_at(&bytes, 0)? != DOS_SIGNATURE {
            return Err(PeError::Invalid("missing MZ header"));
        }
        let pe = usize_from_u32(u32_at(&bytes, 0x3C)?)?;
        if u32_at(&bytes, pe)? != PE_SIGNATURE {
            return Err(PeError::Invalid("missing PE signature"));
        }
        let machine = u16_at(&bytes, pe + 4)?;
        let section_count = usize::from(u16_at(&bytes, pe + 6)?);
        if section_count == 0 || section_count > MAX_SECTIONS {
            return Err(PeError::Invalid("invalid section count"));
        }
        let timestamp = u32_at(&bytes, pe + 8)?;
        let optional_size = usize::from(u16_at(&bytes, pe + 20)?);
        let optional = pe
            .checked_add(24)
            .ok_or(PeError::Invalid("optional header overflow"))?;
        let optional_end = optional
            .checked_add(optional_size)
            .ok_or(PeError::Invalid("optional header overflow"))?;
        if optional_end > bytes.len() || optional_size < 108 {
            return Err(PeError::Invalid("optional header out of bounds"));
        }
        if u16_at(&bytes, optional)? != PE32_MAGIC {
            return Err(PeError::Invalid("target is not PE32"));
        }
        let entry_point_rva = u32_at(&bytes, optional + 16)?;
        let image_base = u32_at(&bytes, optional + 28)?;
        let image_size = u32_at(&bytes, optional + 56)?;
        let number_of_directories = u32_at(&bytes, optional + 92)?;
        let import_rva = if number_of_directories > 1 {
            u32_at(&bytes, optional + 104)?
        } else {
            0
        };

        let section_table = optional_end;
        let table_size = section_count
            .checked_mul(40)
            .ok_or(PeError::Invalid("section table overflow"))?;
        if section_table
            .checked_add(table_size)
            .is_none_or(|end| end > bytes.len())
        {
            return Err(PeError::Invalid("section table out of bounds"));
        }
        let mut sections = Vec::with_capacity(section_count);
        for index in 0..section_count {
            let offset = section_table + index * 40;
            let raw_name = &bytes[offset..offset + 8];
            let name_end = raw_name.iter().position(|byte| *byte == 0).unwrap_or(8);
            let name = std::str::from_utf8(&raw_name[..name_end])
                .map_err(|_| PeError::Invalid("section name is not ASCII"))?
                .to_owned();
            let section = SectionInfo {
                name,
                virtual_size: u32_at(&bytes, offset + 8)?,
                rva: u32_at(&bytes, offset + 12)?,
                raw_size: u32_at(&bytes, offset + 16)?,
                raw_offset: u32_at(&bytes, offset + 20)?,
                characteristics: u32_at(&bytes, offset + 36)?,
            };
            validate_raw_range(&bytes, section.raw_offset, section.raw_size)?;
            sections.push(section);
        }
        for (left_index, left) in sections.iter().enumerate() {
            let left_start = u64::from(left.rva);
            let left_end = left_start + u64::from(left.virtual_size.max(left.raw_size));
            for right in &sections[left_index + 1..] {
                let right_start = u64::from(right.rva);
                let right_end = right_start + u64::from(right.virtual_size.max(right.raw_size));
                if left_start < right_end && right_start < left_end {
                    return Err(PeError::Invalid("overlapping section RVA ranges"));
                }
            }
        }

        let mut image = Self {
            bytes,
            machine,
            timestamp,
            image_size,
            image_base,
            entry_point_rva,
            sections,
            imports: BTreeMap::new(),
            import_modules: BTreeMap::new(),
            import_entries: Vec::new(),
        };
        (image.imports, image.import_modules, image.import_entries) =
            image.parse_imports(import_rva)?;
        Ok(image)
    }

    pub fn hashes(&self) -> Result<ImageHashes, PeError> {
        let text = self
            .sections
            .iter()
            .find(|section| section.name == ".text")
            .ok_or(PeError::Invalid("missing .text section"))?;
        if text.raw_size == 0 {
            return Err(PeError::Invalid("empty .text section"));
        }
        let text_bytes = raw_range(&self.bytes, text.raw_offset, text.raw_size)?;
        Ok(ImageHashes {
            file_sha256: sha256_hex(&self.bytes),
            text_sha256: sha256_hex(text_bytes),
        })
    }

    #[must_use]
    pub fn bytes(&self) -> &[u8] {
        &self.bytes
    }

    pub fn section(&self, name: &str) -> Result<&SectionInfo, PeError> {
        self.sections
            .iter()
            .find(|section| section.name == name)
            .ok_or_else(|| PeError::Detail(format!("missing section {name}")))
    }

    pub fn rva_to_offset(&self, rva: u32, size: usize) -> Result<usize, PeError> {
        let size_u32 = u32::try_from(size).map_err(|_| PeError::Invalid("range too large"))?;
        for section in &self.sections {
            let span = section.virtual_size.max(section.raw_size);
            if rva < section.rva || rva.checked_add(size_u32).is_none() {
                continue;
            }
            let relative = rva - section.rva;
            if relative > span || size_u32 > span - relative || relative > section.raw_size {
                continue;
            }
            if size_u32 > section.raw_size - relative {
                return Err(PeError::Invalid("RVA range has no raw bytes"));
            }
            let offset = section
                .raw_offset
                .checked_add(relative)
                .ok_or(PeError::Invalid("raw offset overflow"))?;
            validate_raw_range(&self.bytes, offset, size_u32)?;
            return usize_from_u32(offset);
        }
        Err(PeError::Detail(format!(
            "RVA 0x{rva:08X} is not backed by a section"
        )))
    }

    pub fn bytes_at_rva(&self, rva: u32, size: usize) -> Result<&[u8], PeError> {
        let offset = self.rva_to_offset(rva, size)?;
        Ok(&self.bytes[offset..offset + size])
    }

    pub fn scan_masked(
        &self,
        section_name: &str,
        pattern: &[u8],
        mask: &[u8],
    ) -> Result<Vec<u32>, PeError> {
        if pattern.is_empty() || pattern.len() != mask.len() {
            return Err(PeError::Invalid("invalid masked pattern"));
        }
        let section = self.section(section_name)?;
        let raw = raw_range(&self.bytes, section.raw_offset, section.raw_size)?;
        if raw.len() < pattern.len() {
            return Ok(Vec::new());
        }
        let mut hits = Vec::new();
        for offset in 0..=raw.len() - pattern.len() {
            if pattern
                .iter()
                .zip(mask)
                .enumerate()
                .all(|(index, (expected, keep))| (raw[offset + index] & keep) == (expected & keep))
            {
                hits.push(
                    section
                        .rva
                        .checked_add(u32::try_from(offset).unwrap())
                        .ok_or(PeError::Invalid("match RVA overflow"))?,
                );
            }
        }
        Ok(hits)
    }

    fn parse_imports(&self, import_rva: u32) -> Result<ParsedImports, PeError> {
        let mut imports = BTreeMap::new();
        let mut modules = BTreeMap::new();
        let mut entries = Vec::new();
        if import_rva == 0 {
            return Ok((imports, modules, entries));
        }
        let mut descriptor_rva = import_rva;
        for _ in 0..4096 {
            let descriptor = self.bytes_at_rva(descriptor_rva, 20)?;
            let original_first_thunk = u32_from_slice(descriptor, 0)?;
            let name_rva = u32_from_slice(descriptor, 12)?;
            let first_thunk = u32_from_slice(descriptor, 16)?;
            if descriptor.iter().all(|byte| *byte == 0) {
                return Ok((imports, modules, entries));
            }
            if name_rva == 0 || first_thunk == 0 {
                return Err(PeError::Invalid("malformed import descriptor"));
            }
            let dll_name = self.c_string_at_rva(name_rva)?;
            let lookup = if original_first_thunk != 0 {
                original_first_thunk
            } else {
                first_thunk
            };
            for index in 0..65_536_u32 {
                let lookup_rva = lookup
                    .checked_add(
                        index
                            .checked_mul(4)
                            .ok_or(PeError::Invalid("thunk overflow"))?,
                    )
                    .ok_or(PeError::Invalid("thunk overflow"))?;
                let value = u32_from_slice(self.bytes_at_rva(lookup_rva, 4)?, 0)?;
                if value == 0 {
                    break;
                }
                if value & IMAGE_ORDINAL_FLAG32 != 0 {
                    continue;
                }
                let name = self.c_string_at_rva(
                    value
                        .checked_add(2)
                        .ok_or(PeError::Invalid("import name overflow"))?,
                )?;
                let iat_rva = first_thunk
                    .checked_add(
                        index
                            .checked_mul(4)
                            .ok_or(PeError::Invalid("IAT overflow"))?,
                    )
                    .ok_or(PeError::Invalid("IAT overflow"))?;
                if imports.insert(name.clone(), iat_rva).is_some() {
                    return Err(PeError::Detail(format!("duplicate import name {name}")));
                }
                modules.insert(name, dll_name.clone());
                entries.push(ImportInfo {
                    module: dll_name.clone(),
                    symbol: self.c_string_at_rva(
                        value
                            .checked_add(2)
                            .ok_or(PeError::Invalid("import name overflow"))?,
                    )?,
                    iat_rva,
                });
            }
            descriptor_rva = descriptor_rva
                .checked_add(20)
                .ok_or(PeError::Invalid("import descriptor overflow"))?;
        }
        Err(PeError::Invalid("unterminated import descriptors"))
    }

    fn c_string_at_rva(&self, rva: u32) -> Result<String, PeError> {
        let offset = self.rva_to_offset(rva, 1)?;
        let end = self.bytes[offset..]
            .iter()
            .position(|byte| *byte == 0)
            .map(|length| offset + length)
            .ok_or(PeError::Invalid("unterminated import string"))?;
        if end - offset > 4096 {
            return Err(PeError::Invalid("import string too long"));
        }
        std::str::from_utf8(&self.bytes[offset..end])
            .map(str::to_owned)
            .map_err(|_| PeError::Invalid("import string is not ASCII"))
    }
}

#[must_use]
pub fn sha256_hex(bytes: &[u8]) -> String {
    hex::encode_upper(Sha256::digest(bytes))
}

fn u16_at(bytes: &[u8], offset: usize) -> Result<u16, PeError> {
    let value = bytes
        .get(offset..offset + 2)
        .ok_or(PeError::Invalid("u16 field out of bounds"))?;
    Ok(u16::from_le_bytes(value.try_into().unwrap()))
}

fn u32_at(bytes: &[u8], offset: usize) -> Result<u32, PeError> {
    let value = bytes
        .get(offset..offset + 4)
        .ok_or(PeError::Invalid("u32 field out of bounds"))?;
    Ok(u32::from_le_bytes(value.try_into().unwrap()))
}

fn u32_from_slice(bytes: &[u8], offset: usize) -> Result<u32, PeError> {
    u32_at(bytes, offset)
}

fn usize_from_u32(value: u32) -> Result<usize, PeError> {
    usize::try_from(value).map_err(|_| PeError::Invalid("integer does not fit usize"))
}

fn validate_raw_range(bytes: &[u8], offset: u32, size: u32) -> Result<(), PeError> {
    let offset = usize_from_u32(offset)?;
    let size = usize_from_u32(size)?;
    if offset.checked_add(size).is_none_or(|end| end > bytes.len()) {
        return Err(PeError::Invalid("section raw range out of bounds"));
    }
    Ok(())
}

fn raw_range(bytes: &[u8], offset: u32, size: u32) -> Result<&[u8], PeError> {
    validate_raw_range(bytes, offset, size)?;
    let offset = usize_from_u32(offset)?;
    let size = usize_from_u32(size)?;
    Ok(&bytes[offset..offset + size])
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn rejects_non_pe_and_truncated_headers() {
        assert!(matches!(
            PeImage::parse(b"not a pe".to_vec()),
            Err(PeError::Invalid(_))
        ));
        let mut mz = vec![0_u8; 0x40];
        mz[0..2].copy_from_slice(&DOS_SIGNATURE.to_le_bytes());
        mz[0x3C..0x40].copy_from_slice(&0x100_u32.to_le_bytes());
        assert!(matches!(PeImage::parse(mz), Err(PeError::Invalid(_))));
    }

    #[test]
    fn masked_scan_requires_unique_callers_to_enforce_uniqueness() {
        let image = minimal_pe(&[0xAA, 0x11, 0xCC, 0xAA, 0x22, 0xCC]);
        let parsed = PeImage::parse(image).unwrap();
        let hits = parsed
            .scan_masked(".text", &[0xAA, 0, 0xCC], &[0xFF, 0, 0xFF])
            .unwrap();
        assert_eq!(hits, vec![0x1000, 0x1003]);
    }

    #[test]
    fn rejects_section_raw_range_past_end_of_file() {
        let mut image = minimal_pe(&[0x90]);
        let section = 0x178;
        image[section + 20..section + 24].copy_from_slice(&0x300_u32.to_le_bytes());
        assert!(matches!(PeImage::parse(image), Err(PeError::Invalid(_))));
    }

    pub(crate) fn minimal_pe(text: &[u8]) -> Vec<u8> {
        let raw_size = 0x200_usize;
        let mut bytes = vec![0_u8; 0x200 + raw_size];
        bytes[0..2].copy_from_slice(&DOS_SIGNATURE.to_le_bytes());
        bytes[0x3C..0x40].copy_from_slice(&0x80_u32.to_le_bytes());
        bytes[0x80..0x84].copy_from_slice(&PE_SIGNATURE.to_le_bytes());
        bytes[0x84..0x86].copy_from_slice(&0x014C_u16.to_le_bytes());
        bytes[0x86..0x88].copy_from_slice(&1_u16.to_le_bytes());
        bytes[0x88..0x8C].copy_from_slice(&1_u32.to_le_bytes());
        bytes[0x94..0x96].copy_from_slice(&0xE0_u16.to_le_bytes());
        let optional = 0x98;
        bytes[optional..optional + 2].copy_from_slice(&PE32_MAGIC.to_le_bytes());
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
        bytes
    }
}
