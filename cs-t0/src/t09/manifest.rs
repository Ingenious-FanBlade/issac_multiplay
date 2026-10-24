use std::collections::BTreeSet;
use std::fs;
use std::net::SocketAddr;
use std::path::{Component, Path, PathBuf};

use serde::{Deserialize, Serialize};

use crate::error::{AppError, AppResult};

pub const MANIFEST_MAGIC: &str = "T9M1";
pub const MANIFEST_VERSION: u32 = 1;

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct Manifest {
    pub magic: String,
    pub version: u32,
    pub run_id: String,
    pub case: String,
    pub repeat: u32,
    pub build_label: String,
    pub wire: WireManifest,
    pub endpoints: EndpointsManifest,
    pub script: ScriptManifest,
    pub relay: RelayManifest,
    pub control: ControlManifest,
    pub artifact_root: String,
    pub user_preconditions: Preconditions,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct WireManifest {
    pub version: u16,
    pub session: String,
    pub run_nonce: String,
    pub authority_nonce: String,
    pub remote_nonce: String,
    pub clients: [u64; 2],
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct EndpointsManifest {
    pub a: EndpointManifest,
    pub b: EndpointManifest,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct EndpointManifest {
    pub os: String,
    pub role: String,
    pub artifact_subdir: String,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ScriptManifest {
    pub id: String,
    pub first_tick: u64,
    pub last_tick: u64,
    pub lead_ticks: u64,
    pub tick_interval_ms: u64,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RelayManifest {
    pub a: RelayFace,
    pub b: RelayFace,
    pub faults: FaultProfile,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RelayFace {
    pub bind: String,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct FaultProfile {
    pub version: u32,
    pub seed: u64,
    pub base_delay_ms: u32,
    pub jitter_ms: u32,
    pub loss_ppm: u32,
    pub duplicate_ppm: u32,
    pub reorder_ppm: u32,
    pub reorder_extra_ms: u32,
    pub outage_after_ms: u64,
    pub outage_ms: u64,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ControlManifest {
    pub bind: String,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct Preconditions {
    pub safe_run: bool,
    pub only_a: bool,
    pub no_original_online: bool,
    pub real_game_authorized: bool,
}

#[derive(Clone, Debug)]
pub struct WireIdentity {
    pub session: [u8; 16],
    pub run_nonce: [u8; 16],
    pub authority_nonce: [u8; 16],
    pub remote_nonce: [u8; 16],
}

impl Manifest {
    pub fn load(path: &Path) -> AppResult<Self> {
        let bytes = fs::read(path)
            .map_err(|error| AppError::usage(format!("manifest {}: {error}", path.display())))?;
        let value: Self = serde_json::from_slice(&bytes)
            .map_err(|error| AppError::usage(format!("manifest {}: {error}", path.display())))?;
        value.validate()?;
        Ok(value)
    }

    pub fn wire_identity(&self) -> AppResult<WireIdentity> {
        Ok(WireIdentity {
            session: parse_hex16(&self.wire.session, "wire.session")?,
            run_nonce: parse_hex16(&self.wire.run_nonce, "wire.run_nonce")?,
            authority_nonce: parse_hex16(&self.wire.authority_nonce, "wire.authority_nonce")?,
            remote_nonce: parse_hex16(&self.wire.remote_nonce, "wire.remote_nonce")?,
        })
    }

    pub fn validate(&self) -> AppResult<()> {
        if self.magic != MANIFEST_MAGIC {
            return Err(AppError::usage("manifest magic must be T9M1"));
        }
        if self.version != MANIFEST_VERSION {
            return Err(AppError::usage(format!(
                "manifest version must be {MANIFEST_VERSION}"
            )));
        }
        validate_identifier(&self.run_id, "run_id", 128)?;
        validate_identifier(&self.case, "case", 32)?;
        if self.repeat == 0 {
            return Err(AppError::usage("manifest repeat must be nonzero"));
        }
        validate_build_label(&self.build_label)?;
        if self.wire.version != 2 {
            return Err(AppError::usage("manifest wire.version must be 2"));
        }
        let wire = self.wire_identity()?;
        for (name, value) in [
            ("wire.session", &wire.session),
            ("wire.run_nonce", &wire.run_nonce),
            ("wire.authority_nonce", &wire.authority_nonce),
            ("wire.remote_nonce", &wire.remote_nonce),
        ] {
            if value.iter().all(|byte| *byte == 0) {
                return Err(AppError::usage(format!("{name} must be nonzero")));
            }
        }
        if wire.authority_nonce == wire.remote_nonce {
            return Err(AppError::usage(
                "wire.authority_nonce and wire.remote_nonce must differ",
            ));
        }
        if self.wire.clients[0] == 0
            || self.wire.clients[1] == 0
            || self.wire.clients[0] == self.wire.clients[1]
        {
            return Err(AppError::usage(
                "wire.clients must be two distinct nonzero identities",
            ));
        }
        validate_endpoint(&self.endpoints.a, "authority")?;
        validate_endpoint(&self.endpoints.b, "remote")?;
        if !safe_relative_component(&self.endpoints.a.artifact_subdir)
            || !safe_relative_component(&self.endpoints.b.artifact_subdir)
            || self.endpoints.a.artifact_subdir.eq_ignore_ascii_case(&self.endpoints.b.artifact_subdir)
        {
            return Err(AppError::usage(
                "endpoint artifact_subdir values must be distinct safe relative components",
            ));
        }
        validate_identifier(&self.script.id, "script.id", 64)?;
        if self.script.last_tick <= self.script.first_tick {
            return Err(AppError::usage(
                "script.last_tick must be greater than script.first_tick",
            ));
        }
        if !(1..=256).contains(&self.script.lead_ticks) {
            return Err(AppError::usage("script.lead_ticks must be 1..=256"));
        }
        if !(1..=1000).contains(&self.script.tick_interval_ms) {
            return Err(AppError::usage(
                "script.tick_interval_ms must be 1..=1000",
            ));
        }
        parse_socket_addr(&self.relay.a.bind, "relay.a.bind")?;
        parse_socket_addr(&self.relay.b.bind, "relay.b.bind")?;
        self.relay.faults.validate()?;
        parse_socket_addr(&self.control.bind, "control.bind")?;
        if self.artifact_root.trim().is_empty() {
            return Err(AppError::usage("artifact_root must be nonempty"));
        }
        if !self.user_preconditions.safe_run
            || !self.user_preconditions.only_a
            || !self.user_preconditions.no_original_online
        {
            return Err(AppError::usage(
                "user_preconditions must confirm safe_run, only_a and no_original_online",
            ));
        }
        Ok(())
    }

    pub fn validate_f0(&self) -> AppResult<()> {
        if self.case != "F0" {
            return Err(AppError::usage(
                "the T0-09-B executable runner supports only F0",
            ));
        }
        if self.relay.faults.base_delay_ms != 0
            || self.relay.faults.jitter_ms != 0
            || self.relay.faults.loss_ppm != 0
            || self.relay.faults.duplicate_ppm != 0
            || self.relay.faults.reorder_ppm != 0
            || self.relay.faults.outage_after_ms != 0
            || self.relay.faults.outage_ms != 0
        {
            return Err(AppError::usage(
                "T0-09-B F0 requires an all-zero impairment profile",
            ));
        }
        Ok(())
    }

    pub fn wire_mapping_name(&self, role: crate::t09::control::Role) -> AppResult<String> {
        let identity = self.wire_identity()?;
        Ok(format!(
            "Local\\IsaacCsT0.Wire.{}.{}",
            hex::encode(identity.run_nonce),
            role.as_str()
        ))
    }
}

impl FaultProfile {
    pub fn validate(&self) -> AppResult<()> {
        if self.version != 1 {
            return Err(AppError::usage("relay.faults.version must be 1"));
        }
        if self.seed == 0 {
            return Err(AppError::usage("relay.faults.seed must be nonzero"));
        }
        if self.base_delay_ms > 5000
            || self.jitter_ms > 5000
            || self.reorder_extra_ms > 5000
        {
            return Err(AppError::usage(
                "relay delay/jitter/reorder components must be <= 5000 ms",
            ));
        }
        for (name, value) in [
            ("loss_ppm", self.loss_ppm),
            ("duplicate_ppm", self.duplicate_ppm),
            ("reorder_ppm", self.reorder_ppm),
        ] {
            if value > 1_000_000 {
                return Err(AppError::usage(format!(
                    "relay.faults.{name} must be <= 1000000"
                )));
            }
        }
        if self.outage_after_ms.checked_add(self.outage_ms).is_none() {
            return Err(AppError::usage("relay outage window overflows"));
        }
        Ok(())
    }

    pub fn active(&self) -> bool {
        self.base_delay_ms != 0
            || self.jitter_ms != 0
            || self.loss_ppm != 0
            || self.duplicate_ppm != 0
            || self.reorder_ppm != 0
            || self.outage_ms != 0
    }
}

pub fn parse_socket_addr(text: &str, field: &str) -> AppResult<SocketAddr> {
    text.parse::<SocketAddr>()
        .map_err(|error| AppError::usage(format!("{field} '{text}': {error}")))
}

pub fn parse_hex16(text: &str, field: &str) -> AppResult<[u8; 16]> {
    if text.len() != 32 {
        return Err(AppError::usage(format!(
            "{field} must be exactly 32 lowercase/uppercase hex characters"
        )));
    }
    let bytes = hex::decode(text)
        .map_err(|error| AppError::usage(format!("{field} is not hex: {error}")))?;
    let array = bytes
        .try_into()
        .map_err(|_| AppError::usage(format!("{field} must decode to 16 bytes")))?;
    Ok(array)
}

pub fn artifact_subdir(root: &Path, manifest: &Manifest, role: &str) -> AppResult<PathBuf> {
    let name = if role.eq_ignore_ascii_case("A") {
        &manifest.endpoints.a.artifact_subdir
    } else if role.eq_ignore_ascii_case("B") {
        &manifest.endpoints.b.artifact_subdir
    } else {
        return Err(AppError::usage("role must be A or B"));
    };
    if !safe_relative_component(name) {
        return Err(AppError::usage("unsafe artifact subdirectory"));
    }
    Ok(root.join(name))
}

pub fn create_new_directory(path: &Path, label: &str) -> AppResult<()> {
    fs::create_dir(path).map_err(|error| {
        AppError::evidence(format!("{label} directory must be new ({}): {error}", path.display()))
    })
}

pub fn write_new_json(path: &Path, value: &serde_json::Value) -> AppResult<()> {
    let text = serde_json::to_string_pretty(value)
        .map_err(|error| AppError::evidence(format!("serialize {}: {error}", path.display())))?;
    use std::io::Write;
    let mut file = fs::OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .map_err(|error| AppError::evidence(format!("create {}: {error}", path.display())))?;
    file.write_all(text.as_bytes())
        .and_then(|()| file.write_all(b"\n"))
        .map_err(|error| AppError::evidence(format!("write {}: {error}", path.display())))
}

fn validate_identifier(value: &str, name: &str, maximum: usize) -> AppResult<()> {
    if value.is_empty() || value.len() > maximum {
        return Err(AppError::usage(format!(
            "{name} must be 1..={maximum} bytes"
        )));
    }
    if value
        .bytes()
        .any(|byte| !(0x21..=0x7e).contains(&byte))
    {
        return Err(AppError::usage(format!(
            "{name} must contain printable non-space ASCII only"
        )));
    }
    Ok(())
}

fn validate_build_label(value: &str) -> AppResult<()> {
    if value.is_empty() || value.len() > 32 || value.bytes().any(|byte| !(0x21..=0x7e).contains(&byte)) {
        return Err(AppError::usage(
            "build_label must be 1..=32 printable non-space ASCII bytes",
        ));
    }
    Ok(())
}

fn validate_endpoint(endpoint: &EndpointManifest, expected_role: &str) -> AppResult<()> {
    if endpoint.os.is_empty() || endpoint.os.len() > 16 {
        return Err(AppError::usage("endpoint os must be 1..=16 bytes"));
    }
    if endpoint.role != expected_role {
        return Err(AppError::usage(format!(
            "endpoint role must be {expected_role}"
        )));
    }
    Ok(())
}

fn safe_relative_component(value: &str) -> bool {
    if value.is_empty() || value == "." || value == ".." {
        return false;
    }
    let path = Path::new(value);
    let mut components = path.components();
    matches!(components.next(), Some(Component::Normal(_))) && components.next().is_none()
}

pub fn ensure_unique_strings(values: &[&str]) -> AppResult<()> {
    let mut seen = BTreeSet::new();
    for value in values {
        if !seen.insert(*value) {
            return Err(AppError::usage("duplicate identity value"));
        }
    }
    Ok(())
}
