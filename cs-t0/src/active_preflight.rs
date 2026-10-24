use std::path::Path;

use serde::Serialize;

use crate::error::AppResult;

pub const ACTIVE_PREFLIGHT_SCHEMA_VERSION: u32 = 1;
const ACTIVE_OFFLINE_ATTESTATION_IMPLEMENTED: bool = false;

fn loading_menu_start(scenario: &str) -> bool {
    scenario == "loading"
}

fn scenario_run_state_allowed(scenario: &str, run_active: bool) -> bool {
    if loading_menu_start(scenario) {
        !run_active
    } else {
        run_active
    }
}

fn owner_shape_allowed(
    scenario: &str,
    player_count: usize,
    unique_owner_count: usize,
    keyboard_device_count: usize,
) -> bool {
    if loading_menu_start(scenario) {
        player_count == 0 && unique_owner_count == 0 && keyboard_device_count == 1
    } else {
        player_count == 1 && unique_owner_count == 1
    }
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ActivePreflightCheck {
    pub id: String,
    pub passed: bool,
    pub detail: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ActivePreflightSlot {
    pub ordinal: u32,
    pub id: String,
    pub owner: String,
    pub patch_kind: String,
    pub rva: String,
    pub bytes_hex: String,
    pub semantic_identity: String,
}

#[derive(Clone, Debug, Eq, PartialEq, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ActivePreflightReport {
    pub schema_version: u32,
    pub result: String,
    pub scenario: String,
    pub target_sha256: String,
    pub process_id: u32,
    pub process_creation_time_100ns: u64,
    pub observer_access_bits: String,
    pub module_count: usize,
    pub selected_device_kind: Option<String>,
    pub player_count: usize,
    pub unique_owner_count: usize,
    pub slots_verified: usize,
    pub slot_snapshot_sha256: String,
    pub iat_symbol_resolution: String,
    pub offline_negative_screen_passed: bool,
    pub offline_session_attested: bool,
    pub checks: Vec<ActivePreflightCheck>,
    pub active_input_allowed: bool,
    pub ready_for_arm_token_cas: bool,
    pub arm_token_consumed: bool,
    pub mapping_created: bool,
    pub native_injected: bool,
    pub artifact_created: bool,
    pub next_checkpoint: String,
}

#[derive(Clone, Debug)]
struct ActivePreflightFacts {
    scenario: String,
    target_sha256: String,
    process_id: u32,
    process_creation_time_100ns: u64,
    module_count: usize,
    selected_device_kind: Option<String>,
    player_count: usize,
    unique_owner_count: usize,
    slots: Vec<ActivePreflightSlot>,
    slot_snapshot_sha256: String,
    disk_hash_stable: bool,
    target_sha_matches: bool,
    single_target_process: bool,
    exact_process_identity: bool,
    process_alive: bool,
    process_creation_stable: bool,
    module_set_stable: bool,
    remote_pe_identity_matches: bool,
    blockers_absent: bool,
    debugger_absent: bool,
    forbidden_modules_absent: bool,
    slot_semantics_valid: bool,
    slot_snapshots_stable: bool,
    owner_snapshot_coherent: bool,
    owner_snapshot_stable: bool,
    device_kind_matches_scenario: bool,
    game_root_present: bool,
    run_active: bool,
    online_negative_screen_passed: bool,
    offline_session_attested: bool,
}

pub fn run_read_only_preflight(
    game_exe: &Path,
    expected_target_sha256: &str,
    scenario: &str,
) -> AppResult<ActivePreflightReport> {
    platform::collect(game_exe, expected_target_sha256, scenario).map(evaluate)
}

fn evaluate(facts: ActivePreflightFacts) -> ActivePreflightReport {
    let run_state_allowed = scenario_run_state_allowed(&facts.scenario, facts.run_active);
    let offline_negative_screen =
        facts.game_root_present && run_state_allowed && facts.online_negative_screen_passed;
    let offline_attested = ACTIVE_OFFLINE_ATTESTATION_IMPLEMENTED
        && facts.offline_session_attested
        && offline_negative_screen;
    let attestation_detail = if facts.scenario == "loading" {
        "requires a pre-bound game-window thread, at least 60 exact menu pump heartbeats, an explicit Lua-unbound menu epoch and sustained zero manager/roster/channel signals"
    } else {
        "requires same-session Lua generation, at least 60 simulation callbacks and sustained zero manager/roster/channel signals"
    };
    let checks = vec![
        check(
            "target-file-sha",
            facts.target_sha_matches,
            "disk image matches the frozen active target SHA-256",
        ),
        check(
            "target-file-stable",
            facts.disk_hash_stable,
            "disk image SHA-256 is unchanged across live preflight",
        ),
        check(
            "single-target-process",
            facts.single_target_process,
            "exactly one isaac-ng.exe process exists",
        ),
        check(
            "exact-process-identity",
            facts.exact_process_identity,
            "loaded image path and x86 identity match the inspected target",
        ),
        check(
            "process-alive",
            facts.process_alive,
            "target remains alive throughout the read-only snapshots",
        ),
        check(
            "process-creation-stable",
            facts.process_creation_stable,
            "PID creation time is unchanged after reopening the target",
        ),
        check(
            "remote-pe-identity",
            facts.remote_pe_identity_matches,
            "remote PE timestamp, SizeOfImage and module size match disk",
        ),
        check(
            "blocker-processes-absent",
            facts.blockers_absent,
            "no debugger, Cheat Engine or conflicting sync process is running",
        ),
        check(
            "debugger-absent",
            facts.debugger_absent,
            "no debugger is attached to either exact-process snapshot",
        ),
        check(
            "forbidden-modules-absent",
            facts.forbidden_modules_absent,
            "no T0, IsaacSync, debugger or Tractor Beam hook module is loaded",
        ),
        check(
            "module-set-stable",
            facts.module_set_stable,
            "the complete module set is identical across both snapshots",
        ),
        check(
            "active-45-slot-observer-identity",
            facts.slot_semantics_valid && facts.slots.len() == 45,
            "33 REL32 and nine image pointers are exact; three fixed disk import slots retain USER32 live ownership",
        ),
        check(
            "active-45-slot-stable",
            facts.slot_snapshots_stable,
            "all 45 slot tuples and bytes are identical across snapshots",
        ),
        check(
            "owner-snapshot-coherent",
            facts.owner_snapshot_coherent,
            "RPM structure snapshot matches the scenario start policy without partial ownership",
        ),
        check(
            "owner-snapshot-stable",
            facts.owner_snapshot_stable,
            "Player, device, reference owner and ownership join remain identical",
        ),
        check(
            "scenario-device-kind",
            facts.device_kind_matches_scenario,
            "keyboard/controller scenarios bind their matching concrete device kind",
        ),
        check(
            "game-root-present",
            facts.game_root_present,
            "a stable game root is present",
        ),
        check(
            "scenario-run-state",
            run_state_allowed,
            "loading starts in a stable inactive menu; every other scenario starts in an active Run",
        ),
        check(
            "online-activity-negative-screen",
            facts.online_negative_screen_passed,
            "the manager identity is stable while network backend, roster and online frame remain canonical zero in both snapshots",
        ),
        check(
            "offline-session-attestation",
            offline_attested,
            attestation_detail,
        ),
        check(
            "arm-token-unconsumed",
            true,
            "the arm token is not passed to or consumed by read-only preflight",
        ),
    ];
    let observable_pass = checks
        .iter()
        .filter(|item| item.id != "offline-session-attestation")
        .all(|item| item.passed);
    let result = if !observable_pass {
        "BLOCKED_BY_PREFLIGHT"
    } else if !offline_attested {
        "BLOCKED_BY_UNIMPLEMENTED_OFFLINE_ATTESTATION"
    } else {
        "PASS_READ_ONLY_PREFLIGHT"
    };
    let ready_for_arm = observable_pass && offline_attested;
    ActivePreflightReport {
        schema_version: ACTIVE_PREFLIGHT_SCHEMA_VERSION,
        result: result.to_owned(),
        scenario: facts.scenario,
        target_sha256: facts.target_sha256,
        process_id: facts.process_id,
        process_creation_time_100ns: facts.process_creation_time_100ns,
        observer_access_bits: format!("0x{:08X}", observer_access_bits()),
        module_count: facts.module_count,
        selected_device_kind: facts.selected_device_kind,
        player_count: facts.player_count,
        unique_owner_count: facts.unique_owner_count,
        slots_verified: facts.slots.len(),
        slot_snapshot_sha256: facts.slot_snapshot_sha256,
        iat_symbol_resolution: "DISK_IMPORT_SLOT_PLUS_LIVE_MODULE_OWNER_ONLY".to_owned(),
        offline_negative_screen_passed: offline_negative_screen,
        offline_session_attested: offline_attested,
        checks,
        active_input_allowed: false,
        ready_for_arm_token_cas: ready_for_arm,
        arm_token_consumed: false,
        mapping_created: false,
        native_injected: false,
        artifact_created: false,
        next_checkpoint: if ready_for_arm {
            "ONE_TIME_ARM_TOKEN_CAS"
        } else {
            "PASSIVE_OFFLINE_SESSION_ATTESTATION"
        }
        .to_owned(),
    }
}

fn check(id: &str, passed: bool, detail: &str) -> ActivePreflightCheck {
    ActivePreflightCheck {
        id: id.to_owned(),
        passed,
        detail: detail.to_owned(),
    }
}

#[cfg(windows)]
fn observer_access_bits() -> u32 {
    crate::win32::OBSERVER_ACCESS_BITS
}

#[cfg(not(windows))]
fn observer_access_bits() -> u32 {
    0
}

#[cfg(windows)]
mod platform {
    use std::collections::BTreeMap;
    use std::path::Path;

    use crate::active_catalog::{ACTIVE_SLOT_COUNT, HookSlot, expected_hook_slots};
    use crate::artifact::file_sha256;
    use crate::error::{AppError, AppResult, UsageContext};
    use crate::input_rpm::{self, ActivePreflightOwnershipSnapshot};
    use crate::pe::{PeImage, sha256_hex};
    use crate::win32::{self, ObservedProcess};

    use super::{
        ActivePreflightFacts, ActivePreflightSlot, loading_menu_start, owner_shape_allowed,
        scenario_run_state_allowed,
    };

    pub(super) fn collect(
        game_exe: &Path,
        expected_target_sha256: &str,
        scenario: &str,
    ) -> AppResult<ActivePreflightFacts> {
        let canonical_game = std::fs::canonicalize(game_exe).usage_context(format!(
            "cannot resolve game executable {}",
            game_exe.display()
        ))?;
        let disk_hash_before = file_sha256(&canonical_game)?;
        if !disk_hash_before.eq_ignore_ascii_case(expected_target_sha256) {
            return Err(AppError::evidence(format!(
                "active preflight is fixed to target SHA-256 {expected_target_sha256}, got {disk_hash_before}; arm token was not consumed"
            )));
        }
        let image = PeImage::read(&canonical_game)
            .map_err(|error| AppError::evidence(error.to_string()))?;

        let blockers_before = win32::running_blockers()?;
        if !blockers_before.is_empty() {
            return Err(AppError::evidence(format!(
                "active preflight refuses blocker processes: {}; arm token was not consumed",
                blockers_before.join(", ")
            )));
        }
        let pids = win32::find_target_pids()?;
        if pids.len() != 1 {
            return Err(AppError::evidence(format!(
                "active preflight requires exactly one isaac-ng.exe process, found {}; arm token was not consumed",
                pids.len()
            )));
        }
        let first = ObservedProcess::open_exact(pids[0], &canonical_game)?;
        require_clean_process(&first)?;
        let remote_identity = remote_pe_identity(&first)?;
        if remote_identity != (image.timestamp, image.image_size)
            || first.main_module.size != image.image_size
        {
            return Err(AppError::evidence(
                "remote PE timestamp or SizeOfImage differs from disk; arm token was not consumed",
            ));
        }
        let first_slots = capture_slots(&first, &image)?;
        let first_owner = input_rpm::active_preflight_ownership_snapshot(&first)?;
        validate_owner_snapshot(scenario, &first_owner)?;
        if first.wait(0)? {
            return Err(AppError::evidence(
                "target exited during active preflight; arm token was not consumed",
            ));
        }

        let blockers_after = win32::running_blockers()?;
        if !blockers_after.is_empty() || blockers_after != blockers_before {
            return Err(AppError::evidence(
                "blocker process set changed during active preflight; arm token was not consumed",
            ));
        }
        let second_pids = win32::find_target_pids()?;
        if second_pids != pids {
            return Err(AppError::evidence(
                "isaac-ng.exe process set changed during active preflight; arm token was not consumed",
            ));
        }
        let second = ObservedProcess::open_exact(pids[0], &canonical_game)?;
        require_clean_process(&second)?;
        if second.creation_time_100ns != first.creation_time_100ns {
            return Err(AppError::evidence(
                "target PID was reused during active preflight; arm token was not consumed",
            ));
        }
        if second.modules != first.modules {
            return Err(AppError::evidence(
                "module set changed during active preflight; arm token was not consumed",
            ));
        }
        if remote_pe_identity(&second)? != remote_identity {
            return Err(AppError::evidence(
                "remote PE identity changed during active preflight; arm token was not consumed",
            ));
        }
        let second_slots = capture_slots(&second, &image)?;
        if second_slots != first_slots {
            return Err(AppError::evidence(
                "one or more active target slots changed during preflight; arm token was not consumed",
            ));
        }
        let second_owner = input_rpm::active_preflight_ownership_snapshot(&second)?;
        validate_owner_snapshot(scenario, &second_owner)?;
        if second_owner != first_owner {
            return Err(AppError::evidence(
                "Player or concrete device ownership changed during preflight; arm token was not consumed",
            ));
        }
        if second.wait(0)? {
            return Err(AppError::evidence(
                "target exited before active preflight completed; arm token was not consumed",
            ));
        }
        let disk_hash_after = file_sha256(&canonical_game)?;
        if disk_hash_after != disk_hash_before {
            return Err(AppError::evidence(
                "target disk image changed during active preflight; arm token was not consumed",
            ));
        }
        let unique = first_owner
            .ownership
            .iter()
            .filter(|join| join.result == "UNIQUE_OWNER_MATCH")
            .collect::<Vec<_>>();
        let keyboard_devices = first_owner
            .devices
            .iter()
            .filter(|device| device.kind == "KEYBOARD")
            .collect::<Vec<_>>();
        let selected_device_kind =
            unique
                .first()
                .map(|join| join.device_kind.clone())
                .or_else(|| {
                    (loading_menu_start(scenario) && keyboard_devices.len() == 1)
                        .then(|| "KEYBOARD".to_owned())
                });
        let slot_bytes = serde_json::to_vec(&first_slots)
            .usage_context("cannot encode active preflight slot snapshot")?;
        Ok(ActivePreflightFacts {
            scenario: scenario.to_owned(),
            target_sha256: disk_hash_before,
            process_id: first.pid,
            process_creation_time_100ns: first.creation_time_100ns,
            module_count: first.modules.len(),
            selected_device_kind: selected_device_kind.clone(),
            player_count: first_owner.players.len(),
            unique_owner_count: unique.len(),
            slots: first_slots,
            slot_snapshot_sha256: sha256_hex(&slot_bytes),
            disk_hash_stable: true,
            target_sha_matches: true,
            single_target_process: true,
            exact_process_identity: true,
            process_alive: true,
            process_creation_stable: true,
            module_set_stable: true,
            remote_pe_identity_matches: true,
            blockers_absent: true,
            debugger_absent: true,
            forbidden_modules_absent: true,
            slot_semantics_valid: true,
            slot_snapshots_stable: true,
            owner_snapshot_coherent: owner_shape_allowed(
                scenario,
                first_owner.players.len(),
                unique.len(),
                keyboard_devices.len(),
            ),
            owner_snapshot_stable: true,
            device_kind_matches_scenario: expected_device_kind(scenario)
                .is_none_or(|expected| selected_device_kind.as_deref() == Some(expected)),
            game_root_present: first_owner.game_root_present,
            run_active: first_owner.run_active,
            online_negative_screen_passed: first_owner.online_negative_screen_passed,
            offline_session_attested: false,
        })
    }

    fn require_clean_process(process: &ObservedProcess) -> AppResult<()> {
        if process.wait(0)? {
            return Err(AppError::evidence(
                "target is not running; arm token was not consumed",
            ));
        }
        if process.debugger_present()? {
            return Err(AppError::evidence(
                "active preflight refuses a debugged target; arm token was not consumed",
            ));
        }
        let forbidden = process.forbidden_modules();
        if !forbidden.is_empty() {
            return Err(AppError::evidence(format!(
                "active preflight refuses loaded conflict modules: {}; arm token was not consumed",
                forbidden.join(", ")
            )));
        }
        Ok(())
    }

    fn validate_owner_snapshot(
        scenario: &str,
        snapshot: &ActivePreflightOwnershipSnapshot,
    ) -> AppResult<()> {
        let unique = snapshot
            .ownership
            .iter()
            .filter(|join| join.result == "UNIQUE_OWNER_MATCH")
            .collect::<Vec<_>>();
        let keyboard_device_count = snapshot
            .devices
            .iter()
            .filter(|device| device.kind == "KEYBOARD")
            .count();
        if !owner_shape_allowed(
            scenario,
            snapshot.players.len(),
            unique.len(),
            keyboard_device_count,
        ) {
            return Err(AppError::evidence(if loading_menu_start(scenario) {
                "active loading preflight requires exactly zero Players, zero unique owners and one concrete keyboard in a stable menu; arm token was not consumed"
            } else {
                "active preflight requires exactly one Player and one unique concrete device owner; arm token was not consumed"
            }));
        }
        let observed_device_kind = unique
            .first()
            .map(|join| join.device_kind.as_str())
            .or_else(|| {
                (loading_menu_start(scenario) && keyboard_device_count == 1).then_some("KEYBOARD")
            });
        if let Some(expected) = expected_device_kind(scenario)
            && observed_device_kind != Some(expected)
        {
            return Err(AppError::evidence(format!(
                "active scenario {scenario} requires {expected} input identity, got {}; arm token was not consumed",
                observed_device_kind.unwrap_or("NONE")
            )));
        }
        if !snapshot.game_root_present
            || snapshot.game_root_identity == 0
            || snapshot.manager_identity & 3 != 0
            || !scenario_run_state_allowed(scenario, snapshot.run_active)
        {
            return Err(AppError::evidence(
                "active preflight requires the scenario start run-state, a stable game root and aligned manager identity; arm token was not consumed",
            ));
        }
        if !snapshot.online_negative_screen_passed {
            return Err(AppError::evidence(
                "online activity negative screen failed; backend, roster or online frame is active or unresolved and arm token was not consumed",
            ));
        }
        Ok(())
    }

    fn expected_device_kind(scenario: &str) -> Option<&'static str> {
        match scenario {
            "keyboard" | "loading" => Some("KEYBOARD"),
            "controller" => Some("GAMEPAD"),
            _ => None,
        }
    }

    fn capture_slots(
        process: &ObservedProcess,
        image: &PeImage,
    ) -> AppResult<Vec<ActivePreflightSlot>> {
        let slots = expected_hook_slots();
        if slots.len() != ACTIVE_SLOT_COUNT {
            return Err(AppError::evidence(
                "active catalog does not contain exactly 45 slots",
            ));
        }
        slots
            .iter()
            .map(|slot| capture_slot(process, image, slot))
            .collect()
    }

    fn capture_slot(
        process: &ObservedProcess,
        image: &PeImage,
        slot: &HookSlot,
    ) -> AppResult<ActivePreflightSlot> {
        let rva = parse_rva(&slot.rva)?;
        let address = checked_add(process.main_module.base, u64::from(rva))?;
        let width = usize::try_from(slot.width)
            .map_err(|_| AppError::evidence("active slot width overflow"))?;
        let bytes = process.read_memory(address, width)?;
        let semantic_identity = match slot.patch_kind.as_str() {
            "REL32_CALL" => {
                if bytes.len() != 5 || bytes[0] != 0xE8 {
                    return Err(slot_error(slot, "live opcode is not E8 rel32"));
                }
                let displacement = i32::from_le_bytes(bytes[1..5].try_into().unwrap());
                let next = address
                    .checked_add(5)
                    .ok_or_else(|| slot_error(slot, "call-next address overflow"))?;
                let target = i128::from(next) + i128::from(displacement);
                let expected = checked_add(
                    process.main_module.base,
                    u64::from(expected_target_rva(slot)?),
                )?;
                if target != i128::from(expected) {
                    return Err(slot_error(
                        slot,
                        "rel32 target differs from the frozen target",
                    ));
                }
                format!(
                    "REL32_TARGET:{}",
                    slot.expected_target_rva.as_deref().unwrap()
                )
            }
            "IMAGE_POINTER" => {
                if bytes.len() != 4 {
                    return Err(slot_error(slot, "image pointer width is not four"));
                }
                let actual = u64::from(u32::from_le_bytes(bytes[..4].try_into().unwrap()));
                let expected = checked_add(
                    process.main_module.base,
                    u64::from(expected_target_rva(slot)?),
                )?;
                if actual != expected || process.owner_module(actual) != Some(&process.main_module)
                {
                    return Err(slot_error(slot, "image pointer target or owner differs"));
                }
                format!(
                    "IMAGE_TARGET:{}",
                    slot.expected_target_rva.as_deref().unwrap()
                )
            }
            "IMPORT_POINTER" => {
                if bytes.len() != 4 {
                    return Err(slot_error(slot, "import pointer width is not four"));
                }
                let actual = u64::from(u32::from_le_bytes(bytes[..4].try_into().unwrap()));
                let owner = process
                    .owner_module(actual)
                    .ok_or_else(|| slot_error(slot, "live import pointer has no module owner"))?;
                validate_import_slot_observation(
                    &slot.id,
                    slot.expected_symbol.as_deref(),
                    &image.imports,
                    rva,
                    &owner.name,
                )?
            }
            _ => return Err(slot_error(slot, "unknown active patch kind")),
        };
        Ok(ActivePreflightSlot {
            ordinal: slot.ordinal,
            id: slot.id.clone(),
            owner: slot.owner.clone(),
            patch_kind: slot.patch_kind.clone(),
            rva: slot.rva.clone(),
            bytes_hex: hex::encode_upper(bytes),
            semantic_identity,
        })
    }

    pub(super) fn validate_import_slot_observation(
        slot_id: &str,
        expected_symbol: Option<&str>,
        disk_imports: &BTreeMap<String, u32>,
        expected_rva: u32,
        live_owner: &str,
    ) -> AppResult<String> {
        let expected_symbol = expected_symbol.ok_or_else(|| {
            AppError::evidence(format!(
                "active slot {slot_id}: import slot lacks expected symbol"
            ))
        })?;
        let (module, symbol) = expected_symbol.split_once('!').ok_or_else(|| {
            AppError::evidence(format!(
                "active slot {slot_id}: expected import symbol is malformed"
            ))
        })?;
        if !module.eq_ignore_ascii_case("USER32") {
            return Err(AppError::evidence(format!(
                "active slot {slot_id}: expected import module is not USER32"
            )));
        }
        if disk_imports.get(symbol) != Some(&expected_rva) {
            return Err(AppError::evidence(format!(
                "active slot {slot_id}: disk import symbol does not map to this IAT RVA"
            )));
        }
        if !live_owner.eq_ignore_ascii_case("USER32.dll") {
            return Err(AppError::evidence(format!(
                "active slot {slot_id}: live import pointer is not owned by USER32.dll"
            )));
        }
        Ok(format!(
            "IMPORT_SLOT:{expected_symbol}:LIVE_OWNER:USER32.dll:SYMBOL_ADDRESS_UNRESOLVED"
        ))
    }

    fn remote_pe_identity(process: &ObservedProcess) -> AppResult<(u32, u32)> {
        let base = process.main_module.base;
        let dos = process.read_memory(base, 0x40)?;
        if dos.get(..2) != Some(b"MZ") {
            return Err(AppError::evidence("remote main module lacks MZ identity"));
        }
        let pe_offset = u32::from_le_bytes(dos[0x3C..0x40].try_into().unwrap());
        let header = process.read_memory(checked_add(base, u64::from(pe_offset))?, 84)?;
        if header.get(..4) != Some(&[b'P', b'E', 0, 0])
            || u16::from_le_bytes(header[24..26].try_into().unwrap()) != 0x010B
        {
            return Err(AppError::evidence("remote main module is not PE32"));
        }
        Ok((
            u32::from_le_bytes(header[8..12].try_into().unwrap()),
            u32::from_le_bytes(header[80..84].try_into().unwrap()),
        ))
    }

    fn expected_target_rva(slot: &HookSlot) -> AppResult<u32> {
        parse_rva(
            slot.expected_target_rva
                .as_deref()
                .ok_or_else(|| slot_error(slot, "slot lacks expected target RVA"))?,
        )
    }

    fn parse_rva(value: &str) -> AppResult<u32> {
        value
            .strip_prefix("0x")
            .or_else(|| value.strip_prefix("0X"))
            .and_then(|raw| u32::from_str_radix(raw, 16).ok())
            .ok_or_else(|| AppError::evidence(format!("invalid active RVA {value}")))
    }

    fn checked_add(base: u64, offset: u64) -> AppResult<u64> {
        base.checked_add(offset)
            .ok_or_else(|| AppError::evidence("active preflight address overflow"))
    }

    fn slot_error(slot: &HookSlot, detail: &str) -> AppError {
        AppError::evidence(format!("active slot {}: {detail}", slot.id))
    }
}

#[cfg(not(windows))]
mod platform {
    use std::path::Path;

    use crate::error::{AppError, AppResult};

    use super::ActivePreflightFacts;

    pub(super) fn collect(
        _game_exe: &Path,
        _expected_target_sha256: &str,
        _scenario: &str,
    ) -> AppResult<ActivePreflightFacts> {
        Err(AppError::usage("active dynamic preflight requires Windows"))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    type FactMutation = Box<dyn Fn(&mut ActivePreflightFacts)>;

    fn passing_observable_facts() -> ActivePreflightFacts {
        ActivePreflightFacts {
            scenario: "keyboard".to_owned(),
            target_sha256: "A".repeat(64),
            process_id: 1,
            process_creation_time_100ns: 2,
            module_count: 100,
            selected_device_kind: Some("KEYBOARD".to_owned()),
            player_count: 1,
            unique_owner_count: 1,
            slots: (0..45)
                .map(|ordinal| ActivePreflightSlot {
                    ordinal,
                    id: format!("slot-{ordinal}"),
                    owner: if ordinal < 32 { "V5" } else { "ACTIVE" }.to_owned(),
                    patch_kind: "REL32_CALL".to_owned(),
                    rva: format!("0x{ordinal:08X}"),
                    bytes_hex: "E800000000".to_owned(),
                    semantic_identity: "REL32_TARGET:0x00000000".to_owned(),
                })
                .collect(),
            slot_snapshot_sha256: "B".repeat(64),
            disk_hash_stable: true,
            target_sha_matches: true,
            single_target_process: true,
            exact_process_identity: true,
            process_alive: true,
            process_creation_stable: true,
            module_set_stable: true,
            remote_pe_identity_matches: true,
            blockers_absent: true,
            debugger_absent: true,
            forbidden_modules_absent: true,
            slot_semantics_valid: true,
            slot_snapshots_stable: true,
            owner_snapshot_coherent: true,
            owner_snapshot_stable: true,
            device_kind_matches_scenario: true,
            game_root_present: true,
            run_active: true,
            online_negative_screen_passed: true,
            offline_session_attested: false,
        }
    }

    #[test]
    fn observable_preflight_stops_before_offline_attestation_and_arm() {
        let report = evaluate(passing_observable_facts());
        assert_eq!(
            report.result,
            "BLOCKED_BY_UNIMPLEMENTED_OFFLINE_ATTESTATION"
        );
        assert!(report.offline_negative_screen_passed);
        assert_eq!(
            report.iat_symbol_resolution,
            "DISK_IMPORT_SLOT_PLUS_LIVE_MODULE_OWNER_ONLY"
        );
        assert!(!report.offline_session_attested);
        assert!(!report.ready_for_arm_token_cas);
        assert!(!report.arm_token_consumed);
        assert!(!report.mapping_created);
        assert!(!report.native_injected);
        assert!(!report.artifact_created);
    }

    #[test]
    fn preflight_mutations_fail_before_offline_attestation() {
        let mut mutations: Vec<FactMutation> = vec![
            Box::new(|facts| facts.target_sha_matches = false),
            Box::new(|facts| facts.disk_hash_stable = false),
            Box::new(|facts| facts.single_target_process = false),
            Box::new(|facts| facts.exact_process_identity = false),
            Box::new(|facts| facts.process_alive = false),
            Box::new(|facts| facts.process_creation_stable = false),
            Box::new(|facts| facts.remote_pe_identity_matches = false),
            Box::new(|facts| facts.blockers_absent = false),
            Box::new(|facts| facts.debugger_absent = false),
            Box::new(|facts| facts.forbidden_modules_absent = false),
            Box::new(|facts| facts.module_set_stable = false),
            Box::new(|facts| facts.slot_semantics_valid = false),
            Box::new(|facts| facts.slot_snapshots_stable = false),
            Box::new(|facts| facts.owner_snapshot_coherent = false),
            Box::new(|facts| facts.owner_snapshot_stable = false),
            Box::new(|facts| facts.device_kind_matches_scenario = false),
            Box::new(|facts| facts.game_root_present = false),
            Box::new(|facts| facts.run_active = false),
            Box::new(|facts| facts.online_negative_screen_passed = false),
            Box::new(|facts| {
                facts.slots.pop();
            }),
        ];
        for mutate in mutations.drain(..) {
            let mut facts = passing_observable_facts();
            mutate(&mut facts);
            let report = evaluate(facts);
            assert_eq!(report.result, "BLOCKED_BY_PREFLIGHT");
            assert!(!report.ready_for_arm_token_cas);
            assert!(!report.arm_token_consumed);
        }
    }

    #[test]
    fn hypothetical_offline_flag_cannot_bypass_unimplemented_attestation() {
        let mut facts = passing_observable_facts();
        facts.offline_session_attested = true;
        let report = evaluate(facts);
        assert_eq!(
            report.result,
            "BLOCKED_BY_UNIMPLEMENTED_OFFLINE_ATTESTATION"
        );
        assert!(!report.ready_for_arm_token_cas);
    }

    #[test]
    fn loading_menu_requires_exact_ownerless_keyboard_shape() {
        let mut facts = passing_observable_facts();
        facts.scenario = "loading".to_owned();
        facts.selected_device_kind = Some("KEYBOARD".to_owned());
        facts.player_count = 0;
        facts.unique_owner_count = 0;
        facts.owner_snapshot_coherent = true;
        facts.run_active = false;
        let report = evaluate(facts.clone());
        assert_eq!(
            report.result,
            "BLOCKED_BY_UNIMPLEMENTED_OFFLINE_ATTESTATION"
        );
        assert!(
            report
                .checks
                .iter()
                .filter(|check| check.id != "offline-session-attestation")
                .all(|check| check.passed)
        );

        let mutations: [fn(&mut ActivePreflightFacts); 3] = [
            |facts| facts.player_count = 1,
            |facts| facts.unique_owner_count = 1,
            |facts| facts.run_active = true,
        ];
        for mutate in mutations {
            let mut invalid = facts.clone();
            mutate(&mut invalid);
            invalid.owner_snapshot_coherent = owner_shape_allowed(
                &invalid.scenario,
                invalid.player_count,
                invalid.unique_owner_count,
                1,
            );
            assert_eq!(evaluate(invalid).result, "BLOCKED_BY_PREFLIGHT");
        }

        let mut keyboard = passing_observable_facts();
        keyboard.player_count = 0;
        keyboard.unique_owner_count = 0;
        keyboard.owner_snapshot_coherent = false;
        assert_eq!(evaluate(keyboard).result, "BLOCKED_BY_PREFLIGHT");
    }

    #[cfg(windows)]
    #[test]
    fn iat_observer_semantics_reject_wrong_slot_symbol_module_and_owner() {
        let imports = std::collections::BTreeMap::from([
            ("PeekMessageW".to_owned(), 0x0071_8714),
            ("TranslateMessage".to_owned(), 0x0071_8650),
        ]);
        let identity = platform::validate_import_slot_observation(
            "peek-message",
            Some("USER32!PeekMessageW"),
            &imports,
            0x0071_8714,
            "USER32.dll",
        )
        .unwrap();
        assert_eq!(
            identity,
            "IMPORT_SLOT:USER32!PeekMessageW:LIVE_OWNER:USER32.dll:SYMBOL_ADDRESS_UNRESOLVED"
        );
        for result in [
            platform::validate_import_slot_observation(
                "peek-message",
                Some("USER32!PeekMessageW"),
                &imports,
                0x0071_8650,
                "USER32.dll",
            ),
            platform::validate_import_slot_observation(
                "peek-message",
                Some("USER32!DispatchMessageW"),
                &imports,
                0x0071_8714,
                "USER32.dll",
            ),
            platform::validate_import_slot_observation(
                "peek-message",
                Some("KERNEL32!PeekMessageW"),
                &imports,
                0x0071_8714,
                "USER32.dll",
            ),
            platform::validate_import_slot_observation(
                "peek-message",
                Some("USER32!PeekMessageW"),
                &imports,
                0x0071_8714,
                "KERNEL32.dll",
            ),
            platform::validate_import_slot_observation(
                "peek-message",
                None,
                &imports,
                0x0071_8714,
                "USER32.dll",
            ),
        ] {
            assert!(result.is_err());
        }
    }
}
