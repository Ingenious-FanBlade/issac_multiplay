use serde::{Deserialize, Serialize};

use crate::error::{AppError, AppResult};

pub const RAW_APPLY_IPC_MAGIC: u64 = 0x3156_3430_4953_4349;
pub const RAW_APPLY_IPC_SCHEMA_VERSION: u32 = 3;
pub const RAW_APPLY_EVENT_CAPACITY: usize = 65_536;
pub const RAW_APPLY_LEGACY_HOOK_MASK: u64 = 0x7ff;
pub const RAW_APPLY_SOURCE_MASK: u64 = 0x3;
pub const RAW_APPLY_CONSUMER_HOOK_MASK: u64 = 0x7f;
pub const RAW_APPLY_LOOP_MAPPING_NAME_CAPACITY: usize = 128;

pub const RAW_APPLY_NO_ACTION: u32 = u32::MAX;
pub const RAW_APPLY_NO_DEVICE_KEY: u32 = u32::MAX;
pub const RAW_APPLY_NO_CONTROLLER_ID: u32 = u32::MAX;
pub const RAW_APPLY_NO_PRIMITIVE: u32 = u32::MAX;
pub const RAW_APPLY_NO_PLAYER_ORDINAL: u32 = u32::MAX;
pub const RAW_APPLY_NO_GENERATION: u64 = 0;

pub const RAW_APPLY_FLAG_LOOP_KNOWN: u32 = 1 << 0;
pub const RAW_APPLY_FLAG_MAIN_THREAD: u32 = 1 << 1;
pub const RAW_APPLY_FLAG_WRONG_THREAD: u32 = 1 << 2;
pub const RAW_APPLY_FLAG_ORIGINAL_CALLED: u32 = 1 << 3;
pub const RAW_APPLY_FLAG_RESULT_TRUE: u32 = 1 << 4;
pub const RAW_APPLY_FLAG_KEYBOARD_DEVICE: u32 = 1 << 5;
pub const RAW_APPLY_FLAG_GAMEPAD_DEVICE: u32 = 1 << 6;
pub const RAW_APPLY_FLAG_VALUE_VALID: u32 = 1 << 7;
pub const RAW_APPLY_FLAG_REDACTED: u32 = 1 << 8;
pub const RAW_APPLY_FLAG_SAMPLE_KNOWN: u32 = 1 << 9;
pub const RAW_APPLY_FLAG_CONSUMER_KNOWN: u32 = 1 << 10;
pub const RAW_APPLY_FLAG_CONTEXT_KNOWN: u32 = 1 << 11;
pub const RAW_APPLY_FLAG_PLAYER_ORDINAL_VALID: u32 = 1 << 12;
pub const RAW_APPLY_FLAG_WITNESS_VALID: u32 = 1 << 13;
pub const RAW_APPLY_FLAG_RAW_RISING: u32 = 1 << 14;
pub const RAW_APPLY_FLAG_RAW_FALLING: u32 = 1 << 15;
pub const RAW_APPLY_KNOWN_FLAGS: u32 = 0xffff;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum RawApplyProbe {
    Passive = 1,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(i32)]
pub enum RawApplyNativeState {
    Empty = 0,
    Initializing = 1,
    Ready = 2,
    Failed = 3,
    ShuttingDown = 4,
    Stopped = 5,
    Unsafe = 6,
}

impl RawApplyNativeState {
    pub fn from_raw(value: i32) -> AppResult<Self> {
        match value {
            0 => Ok(Self::Empty),
            1 => Ok(Self::Initializing),
            2 => Ok(Self::Ready),
            3 => Ok(Self::Failed),
            4 => Ok(Self::ShuttingDown),
            5 => Ok(Self::Stopped),
            6 => Ok(Self::Unsafe),
            _ => Err(AppError::evidence(format!(
                "raw/apply native shared state has unknown value {value}"
            ))),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
#[repr(u32)]
pub enum RawApplyEventKind {
    NativeInitialized = 1,
    HooksInstalled = 2,
    LogicalPressedEnter = 3,
    LogicalPressedExit = 4,
    LogicalTriggeredEnter = 5,
    LogicalTriggeredExit = 6,
    LogicalScalarEnter = 7,
    LogicalScalarExit = 8,
    ManagerPressedEnter = 9,
    ManagerPressedExit = 10,
    ManagerTriggeredEnter = 11,
    ManagerTriggeredExit = 12,
    ManagerScalarEnter = 13,
    ManagerScalarExit = 14,
    LogicalSetterEnter = 15,
    LogicalSetterExit = 16,
    HooksRestored = 17,
    NativeRetired = 18,
    Failure = 19,
    RawPollEnter = 20,
    RawValue = 21,
    RawPollExit = 22,
    PlayerConsumerEnter = 23,
    PlayerConsumerExit = 24,
    WorldWitnessBefore = 25,
    WorldWitnessAfter = 26,
    ContextOwner = 27,
    ContextBinding = 28,
}

impl RawApplyEventKind {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            1 => Ok(Self::NativeInitialized),
            2 => Ok(Self::HooksInstalled),
            3 => Ok(Self::LogicalPressedEnter),
            4 => Ok(Self::LogicalPressedExit),
            5 => Ok(Self::LogicalTriggeredEnter),
            6 => Ok(Self::LogicalTriggeredExit),
            7 => Ok(Self::LogicalScalarEnter),
            8 => Ok(Self::LogicalScalarExit),
            9 => Ok(Self::ManagerPressedEnter),
            10 => Ok(Self::ManagerPressedExit),
            11 => Ok(Self::ManagerTriggeredEnter),
            12 => Ok(Self::ManagerTriggeredExit),
            13 => Ok(Self::ManagerScalarEnter),
            14 => Ok(Self::ManagerScalarExit),
            15 => Ok(Self::LogicalSetterEnter),
            16 => Ok(Self::LogicalSetterExit),
            17 => Ok(Self::HooksRestored),
            18 => Ok(Self::NativeRetired),
            19 => Ok(Self::Failure),
            20 => Ok(Self::RawPollEnter),
            21 => Ok(Self::RawValue),
            22 => Ok(Self::RawPollExit),
            23 => Ok(Self::PlayerConsumerEnter),
            24 => Ok(Self::PlayerConsumerExit),
            25 => Ok(Self::WorldWitnessBefore),
            26 => Ok(Self::WorldWitnessAfter),
            27 => Ok(Self::ContextOwner),
            28 => Ok(Self::ContextBinding),
            _ => Err(AppError::evidence(format!(
                "raw/apply native event has unknown kind {value}"
            ))),
        }
    }

    pub fn is_lifecycle(self) -> bool {
        matches!(
            self,
            Self::NativeInitialized
                | Self::HooksInstalled
                | Self::HooksRestored
                | Self::NativeRetired
                | Self::Failure
        )
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
#[repr(u32)]
pub enum RawApplyClassification {
    Unknown = 0,
    UniqueGameplay = 1,
    NoMatch = 2,
    AmbiguousOrOutsideGameplay = 3,
    InvalidLayout = 4,
    UnstableSnapshot = 5,
    ExplicitGameplay = 6,
    LogicalOverrideDiagnostic = 7,
    RawDeviceOnly = 8,
    PlayerConsumerObserved = 9,
    PassiveWorldWitness = 10,
    ExplicitGameplayWithOutsideAlias = 11,
}

impl RawApplyClassification {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            0 => Ok(Self::Unknown),
            1 => Ok(Self::UniqueGameplay),
            2 => Ok(Self::NoMatch),
            3 => Ok(Self::AmbiguousOrOutsideGameplay),
            4 => Ok(Self::InvalidLayout),
            5 => Ok(Self::UnstableSnapshot),
            6 => Ok(Self::ExplicitGameplay),
            7 => Ok(Self::LogicalOverrideDiagnostic),
            8 => Ok(Self::RawDeviceOnly),
            9 => Ok(Self::PlayerConsumerObserved),
            10 => Ok(Self::PassiveWorldWitness),
            11 => Ok(Self::ExplicitGameplayWithOutsideAlias),
            _ => Err(AppError::evidence(format!(
                "raw/apply native event has unknown classification {value}"
            ))),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
#[repr(u32)]
pub enum RawChannelKind {
    None = 0,
    KeyboardKeyDigital = 1,
    GamepadButtonDigital = 2,
    GamepadAxisF32 = 3,
    GamepadTriggerF32 = 4,
    DevicePacketU32 = 5,
}

impl RawChannelKind {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            0 => Ok(Self::None),
            1 => Ok(Self::KeyboardKeyDigital),
            2 => Ok(Self::GamepadButtonDigital),
            3 => Ok(Self::GamepadAxisF32),
            4 => Ok(Self::GamepadTriggerF32),
            5 => Ok(Self::DevicePacketU32),
            _ => Err(AppError::evidence(format!(
                "raw/apply native event has unknown raw channel kind {value}"
            ))),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
#[repr(u32)]
pub enum PassiveWitnessKind {
    None = 0,
    PlayerPositionF32x2 = 1,
    PlayerVelocityF32x2 = 2,
    PlayerOwnedEntityCountU32x2 = 3,
    InventorySlotU32x2 = 4,
    PlayerCounterU32x2 = 5,
    MovementDirection = 6,
    ShootingDirection = 7,
    AimVector = 8,
    AimState = 9,
    BombIntentBit = 10,
    DropHoldCounter = 11,
}

impl PassiveWitnessKind {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            0 => Ok(Self::None),
            1 => Ok(Self::PlayerPositionF32x2),
            2 => Ok(Self::PlayerVelocityF32x2),
            3 => Ok(Self::PlayerOwnedEntityCountU32x2),
            4 => Ok(Self::InventorySlotU32x2),
            5 => Ok(Self::PlayerCounterU32x2),
            6 => Ok(Self::MovementDirection),
            7 => Ok(Self::ShootingDirection),
            8 => Ok(Self::AimVector),
            9 => Ok(Self::AimState),
            10 => Ok(Self::BombIntentBit),
            11 => Ok(Self::DropHoldCounter),
            _ => Err(AppError::evidence(format!(
                "raw/apply native event has unknown witness kind {value}"
            ))),
        }
    }
}

#[derive(Clone, Copy, Debug)]
#[repr(C, align(8))]
pub struct NativeRawApplyEvent {
    pub published_sequence: u64,
    pub qpc: u64,
    pub loop_iteration: u64,
    pub sample_generation: u64,
    pub consumer_transaction: u64,
    pub context_generation: u64,
    pub thread_id: u32,
    pub kind: u32,
    pub phase: u32,
    pub flags: u32,
    pub hook_site_rva: u32,
    pub return_rva: u32,
    pub original_calls: u32,
    pub device_key: u32,
    pub controller_id_raw: u32,
    pub selected_device_key: u32,
    pub player_ordinal: u32,
    pub action: u32,
    pub primitive: u32,
    pub raw_channel_kind: u32,
    pub raw_channel_id: u32,
    pub value_bits: u32,
    pub classification: u32,
    pub witness_kind: u32,
    pub witness_aux_bits: u32,
    pub error_code: u32,
}

#[repr(C, align(8))]
pub struct SharedRawApplyBlock {
    pub magic: u64,
    pub schema_version: u32,
    pub struct_size: u32,
    pub target_pid: u32,
    pub probe: u32,
    pub scenario: u32,
    pub flags: u32,
    pub nonce: [u8; 32],
    pub target_sha256: [u8; 32],
    pub loop_gate_sha256: [u8; 32],
    pub catalog_sha256: [u8; 32],
    pub capture_plan_sha256: [u8; 32],
    pub loop_mapping_name: [u16; RAW_APPLY_LOOP_MAPPING_NAME_CAPACITY],
    pub state: i32,
    pub error_code: i32,
    pub legacy_installed_hooks_mask: u64,
    pub legacy_restored_hooks_mask: u64,
    pub required_raw_source_mask: u64,
    pub observed_raw_source_mask: u64,
    pub required_consumer_hooks_mask: u64,
    pub installed_consumer_hooks_mask: u64,
    pub restored_consumer_hooks_mask: u64,
    pub main_thread_id: u32,
    pub reserved0: u32,
    pub generation: u64,
    pub current_loop_iteration: u64,
    pub next_sequence: u64,
    pub controller_sequence: u64,
    pub overwritten_events: u64,
    pub redacted_events: u64,
    pub active_hooks: i32,
    pub producer_closed: i32,
    pub events: [NativeRawApplyEvent; RAW_APPLY_EVENT_CAPACITY],
}

#[derive(Clone, Copy, Debug)]
pub struct NativeRawApplyEventRecord {
    pub sequence: u64,
    pub event: NativeRawApplyEvent,
}

pub fn validate_raw_apply_block_header(
    magic: u64,
    schema_version: u32,
    struct_size: u32,
) -> AppResult<()> {
    if magic != RAW_APPLY_IPC_MAGIC {
        return Err(AppError::evidence(format!(
            "raw/apply IPC magic mismatch: expected 0x{RAW_APPLY_IPC_MAGIC:016X}, got 0x{magic:016X}"
        )));
    }
    if schema_version != RAW_APPLY_IPC_SCHEMA_VERSION {
        return Err(AppError::evidence(format!(
            "raw/apply IPC schema mismatch: expected {RAW_APPLY_IPC_SCHEMA_VERSION}, got {schema_version}"
        )));
    }
    let expected = u32::try_from(std::mem::size_of::<SharedRawApplyBlock>())
        .map_err(|_| AppError::evidence("raw/apply IPC block size does not fit u32"))?;
    if struct_size != expected {
        return Err(AppError::evidence(format!(
            "raw/apply IPC size mismatch: expected {expected}, got {struct_size}"
        )));
    }
    Ok(())
}

pub fn drain_raw_apply_snapshot(
    slots: &[NativeRawApplyEvent],
    produced: u64,
    overwritten: u64,
    next_sequence: &mut u64,
) -> AppResult<Vec<NativeRawApplyEventRecord>> {
    if slots.len() != RAW_APPLY_EVENT_CAPACITY {
        return Err(AppError::evidence(format!(
            "raw/apply event ring capacity mismatch: expected {RAW_APPLY_EVENT_CAPACITY}, got {}",
            slots.len()
        )));
    }
    if overwritten != 0 {
        return Err(AppError::evidence(
            "raw/apply event ring reports overwritten events before controller drain",
        ));
    }
    if produced < *next_sequence {
        return Err(AppError::evidence(
            "raw/apply event producer sequence regressed",
        ));
    }
    if produced - *next_sequence > RAW_APPLY_EVENT_CAPACITY as u64 {
        return Err(AppError::evidence(
            "raw/apply event ring overflowed before controller drain",
        ));
    }
    let mut records = Vec::new();
    while *next_sequence < produced {
        let sequence = next_sequence
            .checked_add(1)
            .ok_or_else(|| AppError::evidence("raw/apply event sequence overflow"))?;
        let index = usize::try_from((sequence - 1) % RAW_APPLY_EVENT_CAPACITY as u64)
            .map_err(|_| AppError::evidence("raw/apply ring index overflow"))?;
        let event = slots[index];
        if event.published_sequence != sequence {
            break;
        }
        records.push(NativeRawApplyEventRecord { sequence, event });
        *next_sequence = sequence;
    }
    Ok(records)
}

#[allow(clippy::too_many_arguments)]
fn validate_raw_apply_create_inputs(
    name: &str,
    target_pid: u32,
    nonce: &[u8; 32],
    target_sha256: &[u8; 32],
    loop_gate_sha256: &[u8; 32],
    catalog_sha256: &[u8; 32],
    capture_plan_sha256: &[u8; 32],
    loop_mapping_name: &str,
) -> AppResult<Vec<u16>> {
    if name.trim().is_empty()
        || target_pid == 0
        || nonce.iter().all(|byte| *byte == 0)
        || target_sha256.iter().all(|byte| *byte == 0)
        || loop_gate_sha256.iter().all(|byte| *byte == 0)
        || catalog_sha256.iter().all(|byte| *byte == 0)
        || capture_plan_sha256.iter().all(|byte| *byte == 0)
    {
        return Err(AppError::usage(
            "raw/apply mapping requires nonempty identity inputs",
        ));
    }
    let wide = loop_mapping_name
        .encode_utf16()
        .chain(std::iter::once(0))
        .collect::<Vec<_>>();
    if wide.len() <= 1 || wide.len() > RAW_APPLY_LOOP_MAPPING_NAME_CAPACITY {
        return Err(AppError::usage(
            "raw/apply companion loop mapping name is empty or oversized",
        ));
    }
    Ok(wide)
}

#[cfg(windows)]
mod windows_mapping {
    use std::mem::size_of;
    use std::ptr::{NonNull, read_volatile, write_volatile};
    use std::sync::atomic::{Ordering, fence};

    use windows::Win32::Foundation::{CloseHandle, HANDLE, INVALID_HANDLE_VALUE};
    use windows::Win32::System::Memory::{
        CreateFileMappingW, FILE_MAP_ALL_ACCESS, MEMORY_MAPPED_VIEW_ADDRESS, MapViewOfFile,
        PAGE_READWRITE, UnmapViewOfFile,
    };
    use windows::core::PCWSTR;

    use super::*;

    pub struct RawApplyMapping {
        handle: HANDLE,
        view: NonNull<SharedRawApplyBlock>,
        name: String,
    }

    impl RawApplyMapping {
        #[allow(clippy::too_many_arguments)]
        pub fn create(
            name: String,
            target_pid: u32,
            probe: RawApplyProbe,
            scenario: u32,
            nonce: [u8; 32],
            target_sha256: [u8; 32],
            loop_gate_sha256: [u8; 32],
            catalog_sha256: [u8; 32],
            capture_plan_sha256: [u8; 32],
            loop_mapping_name: &str,
        ) -> AppResult<Self> {
            let loop_wide = validate_raw_apply_create_inputs(
                &name,
                target_pid,
                &nonce,
                &target_sha256,
                &loop_gate_sha256,
                &catalog_sha256,
                &capture_plan_sha256,
                loop_mapping_name,
            )?;
            let wide = name
                .encode_utf16()
                .chain(std::iter::once(0))
                .collect::<Vec<_>>();
            let size = size_of::<SharedRawApplyBlock>();
            let size_low = u32::try_from(size)
                .map_err(|_| AppError::usage("raw/apply mapping is too large"))?;
            let handle = unsafe {
                CreateFileMappingW(
                    INVALID_HANDLE_VALUE,
                    None,
                    PAGE_READWRITE,
                    0,
                    size_low,
                    PCWSTR(wide.as_ptr()),
                )
            }
            .map_err(|error| AppError::evidence(format!("CreateFileMappingW failed: {error}")))?;
            let raw = unsafe { MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, size) };
            let Some(view) = NonNull::new(raw.Value.cast::<SharedRawApplyBlock>()) else {
                let _ = unsafe { CloseHandle(handle) };
                return Err(AppError::evidence("MapViewOfFile returned null"));
            };
            unsafe {
                let block = view.as_ptr();
                write_volatile(&raw mut (*block).magic, RAW_APPLY_IPC_MAGIC);
                write_volatile(
                    &raw mut (*block).schema_version,
                    RAW_APPLY_IPC_SCHEMA_VERSION,
                );
                write_volatile(&raw mut (*block).struct_size, size_low);
                write_volatile(&raw mut (*block).target_pid, target_pid);
                write_volatile(&raw mut (*block).probe, probe as u32);
                write_volatile(&raw mut (*block).scenario, scenario);
                (*block).nonce = nonce;
                (*block).target_sha256 = target_sha256;
                (*block).loop_gate_sha256 = loop_gate_sha256;
                (*block).catalog_sha256 = catalog_sha256;
                (*block).capture_plan_sha256 = capture_plan_sha256;
                (&mut (*block).loop_mapping_name)[..loop_wide.len()].copy_from_slice(&loop_wide);
                write_volatile(
                    &raw mut (*block).required_raw_source_mask,
                    RAW_APPLY_SOURCE_MASK,
                );
                write_volatile(
                    &raw mut (*block).required_consumer_hooks_mask,
                    RAW_APPLY_CONSUMER_HOOK_MASK,
                );
                write_volatile(&raw mut (*block).generation, 1);
            }
            Ok(Self { handle, view, name })
        }

        pub fn name(&self) -> &str {
            &self.name
        }

        fn validate_header(&self) -> AppResult<()> {
            let block = self.view.as_ptr();
            validate_raw_apply_block_header(
                unsafe { read_volatile(&raw const (*block).magic) },
                unsafe { read_volatile(&raw const (*block).schema_version) },
                unsafe { read_volatile(&raw const (*block).struct_size) },
            )
        }

        pub fn state(&self) -> AppResult<RawApplyNativeState> {
            self.validate_header()?;
            RawApplyNativeState::from_raw(unsafe {
                read_volatile(&raw const (*self.view.as_ptr()).state)
            })
        }

        pub fn error_code(&self) -> i32 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).error_code) }
        }

        pub fn legacy_installed_hooks_mask(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).legacy_installed_hooks_mask) }
        }

        pub fn legacy_restored_hooks_mask(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).legacy_restored_hooks_mask) }
        }

        pub fn required_raw_source_mask(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).required_raw_source_mask) }
        }

        pub fn observed_raw_source_mask(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).observed_raw_source_mask) }
        }

        pub fn required_consumer_hooks_mask(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).required_consumer_hooks_mask) }
        }

        pub fn installed_consumer_hooks_mask(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).installed_consumer_hooks_mask) }
        }

        pub fn restored_consumer_hooks_mask(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).restored_consumer_hooks_mask) }
        }

        pub fn main_thread_id(&self) -> u32 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).main_thread_id) }
        }

        pub fn overwritten_events(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).overwritten_events) }
        }

        pub fn redacted_events(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).redacted_events) }
        }

        pub fn producer_closed(&self) -> bool {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).producer_closed) != 0 }
        }

        pub fn active_hooks(&self) -> i32 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).active_hooks) }
        }

        #[cfg(test)]
        fn generation(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).generation) }
        }

        pub fn drain(&self, next_sequence: &mut u64) -> AppResult<Vec<NativeRawApplyEventRecord>> {
            self.validate_header()?;
            let block = self.view.as_ptr();
            if self.overwritten_events() != 0 {
                return Err(AppError::evidence(
                    "raw/apply event ring reports overwritten events before controller drain",
                ));
            }
            let produced = unsafe { read_volatile(&raw const (*block).next_sequence) };
            if produced.saturating_sub(*next_sequence) > RAW_APPLY_EVENT_CAPACITY as u64 {
                return Err(AppError::evidence(
                    "raw/apply event ring overflowed before controller drain",
                ));
            }
            let mut records = Vec::new();
            while *next_sequence < produced {
                let sequence = next_sequence
                    .checked_add(1)
                    .ok_or_else(|| AppError::evidence("raw/apply event sequence overflow"))?;
                let index = usize::try_from((sequence - 1) % RAW_APPLY_EVENT_CAPACITY as u64)
                    .map_err(|_| AppError::evidence("raw/apply ring index overflow"))?;
                let slot = unsafe { &raw const (*block).events[index] };
                let published_before =
                    unsafe { read_volatile(&raw const (*slot).published_sequence) };
                if published_before != sequence {
                    break;
                }
                fence(Ordering::Acquire);
                let event = unsafe { read_volatile(slot) };
                fence(Ordering::Acquire);
                let published_after =
                    unsafe { read_volatile(&raw const (*slot).published_sequence) };
                if published_before != published_after
                    || published_after != sequence
                    || event.published_sequence != sequence
                {
                    break;
                }
                records.push(NativeRawApplyEventRecord { sequence, event });
                *next_sequence = sequence;
            }
            let produced_after = unsafe { read_volatile(&raw const (*block).next_sequence) };
            if self.overwritten_events() != 0
                || produced_after.saturating_sub(*next_sequence) > RAW_APPLY_EVENT_CAPACITY as u64
            {
                return Err(AppError::evidence(
                    "raw/apply event ring overflowed during controller drain",
                ));
            }
            unsafe {
                write_volatile(&raw mut (*block).controller_sequence, *next_sequence);
            }
            Ok(records)
        }
    }

    impl Drop for RawApplyMapping {
        fn drop(&mut self) {
            let _ = unsafe {
                UnmapViewOfFile(MEMORY_MAPPED_VIEW_ADDRESS {
                    Value: self.view.as_ptr().cast(),
                })
            };
            let _ = unsafe { CloseHandle(self.handle) };
        }
    }

    #[cfg(test)]
    mod tests {
        use super::*;

        #[test]
        fn created_mapping_binds_required_masks_and_generation() {
            let nonce = [1; 32];
            let suffix = std::process::id();
            let mapping = RawApplyMapping::create(
                format!("Local\\IsaacCsT0.RawApply.Test.{suffix}"),
                suffix,
                RawApplyProbe::Passive,
                1,
                nonce,
                [2; 32],
                [3; 32],
                [4; 32],
                [5; 32],
                "Local\\IsaacCsT0.Loop.Test",
            )
            .unwrap();
            assert_eq!(mapping.required_raw_source_mask(), RAW_APPLY_SOURCE_MASK);
            assert_eq!(
                mapping.required_consumer_hooks_mask(),
                RAW_APPLY_CONSUMER_HOOK_MASK
            );
            assert_eq!(mapping.generation(), 1);
        }
    }
}

#[cfg(windows)]
pub use windows_mapping::RawApplyMapping;

#[cfg(test)]
mod tests {
    use super::*;

    fn empty_slots() -> Vec<NativeRawApplyEvent> {
        vec![unsafe { std::mem::zeroed() }; RAW_APPLY_EVENT_CAPACITY]
    }

    #[test]
    fn raw_apply_layout_matches_frozen_schema_three_abi() {
        assert_eq!(std::mem::align_of::<NativeRawApplyEvent>(), 8);
        assert_eq!(std::mem::size_of::<NativeRawApplyEvent>(), 128);
        assert_eq!(std::mem::offset_of!(NativeRawApplyEvent, thread_id), 48);
        assert_eq!(std::mem::offset_of!(NativeRawApplyEvent, error_code), 124);

        assert_eq!(std::mem::align_of::<SharedRawApplyBlock>(), 8);
        assert_eq!(std::mem::size_of::<SharedRawApplyBlock>(), 8_389_184);
        assert_eq!(std::mem::offset_of!(SharedRawApplyBlock, magic), 0);
        assert_eq!(std::mem::offset_of!(SharedRawApplyBlock, schema_version), 8);
        assert_eq!(std::mem::offset_of!(SharedRawApplyBlock, struct_size), 12);
        assert_eq!(std::mem::offset_of!(SharedRawApplyBlock, target_pid), 16);
        assert_eq!(std::mem::offset_of!(SharedRawApplyBlock, probe), 20);
        assert_eq!(std::mem::offset_of!(SharedRawApplyBlock, scenario), 24);
        assert_eq!(std::mem::offset_of!(SharedRawApplyBlock, flags), 28);
        assert_eq!(std::mem::offset_of!(SharedRawApplyBlock, nonce), 32);
        assert_eq!(std::mem::offset_of!(SharedRawApplyBlock, target_sha256), 64);
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, loop_gate_sha256),
            96
        );
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, catalog_sha256),
            128
        );
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, capture_plan_sha256),
            160
        );
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, loop_mapping_name),
            192
        );
        assert_eq!(std::mem::offset_of!(SharedRawApplyBlock, state), 448);
        assert_eq!(std::mem::offset_of!(SharedRawApplyBlock, error_code), 452);
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, legacy_installed_hooks_mask),
            456
        );
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, legacy_restored_hooks_mask),
            464
        );
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, required_raw_source_mask),
            472
        );
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, observed_raw_source_mask),
            480
        );
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, required_consumer_hooks_mask),
            488
        );
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, installed_consumer_hooks_mask),
            496
        );
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, restored_consumer_hooks_mask),
            504
        );
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, main_thread_id),
            512
        );
        assert_eq!(std::mem::offset_of!(SharedRawApplyBlock, reserved0), 516);
        assert_eq!(std::mem::offset_of!(SharedRawApplyBlock, generation), 520);
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, current_loop_iteration),
            528
        );
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, next_sequence),
            536
        );
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, controller_sequence),
            544
        );
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, overwritten_events),
            552
        );
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, redacted_events),
            560
        );
        assert_eq!(std::mem::offset_of!(SharedRawApplyBlock, active_hooks), 568);
        assert_eq!(
            std::mem::offset_of!(SharedRawApplyBlock, producer_closed),
            572
        );
        assert_eq!(std::mem::offset_of!(SharedRawApplyBlock, events), 576);
    }

    #[test]
    fn schema_three_header_rejects_schema_two_without_changing_v2_validator() {
        let size = std::mem::size_of::<SharedRawApplyBlock>() as u32;
        assert!(validate_raw_apply_block_header(RAW_APPLY_IPC_MAGIC, 2, size).is_err());
        assert!(validate_raw_apply_block_header(RAW_APPLY_IPC_MAGIC, 3, size).is_ok());
        assert!(
            crate::input_ipc::validate_input_block_header(
                crate::input_ipc::INPUT_IPC_MAGIC,
                crate::input_ipc::INPUT_IPC_SCHEMA_VERSION,
                std::mem::size_of::<crate::input_ipc::SharedInputBlock>() as u32,
            )
            .is_ok()
        );
    }

    #[test]
    fn raw_apply_ring_drains_contiguous_records_and_fails_closed() {
        let mut slots = empty_slots();
        slots[0].published_sequence = 1;
        slots[1].published_sequence = 2;
        let mut next = 0;
        let records = drain_raw_apply_snapshot(&slots, 2, 0, &mut next).unwrap();
        assert_eq!(records.len(), 2);
        assert_eq!(records[1].sequence, 2);
        assert_eq!(next, 2);

        next = 0;
        slots[1].published_sequence = 0;
        assert_eq!(
            drain_raw_apply_snapshot(&slots, 2, 0, &mut next)
                .unwrap()
                .len(),
            1
        );
        assert_eq!(next, 1);
        assert!(drain_raw_apply_snapshot(&slots, 2, 1, &mut next).is_err());
        assert!(drain_raw_apply_snapshot(&slots[..2], 2, 0, &mut next).is_err());
        next = 0;
        assert!(
            drain_raw_apply_snapshot(&slots, RAW_APPLY_EVENT_CAPACITY as u64 + 1, 0, &mut next)
                .is_err()
        );
    }
}
