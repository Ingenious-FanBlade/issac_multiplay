use serde::{Deserialize, Serialize};

use crate::error::{AppError, AppResult};

pub const INPUT_IPC_MAGIC: u64 = 0x3156_3430_4953_4349;
pub const INPUT_IPC_SCHEMA_VERSION: u32 = 2;
pub const INPUT_EVENT_CAPACITY: usize = 65_536;
pub const INPUT_HOOK_MASK: u64 = (1_u64 << 11) - 1;
pub const INPUT_NO_ACTION: u32 = u32::MAX;
pub const INPUT_NO_DEVICE_KEY: u32 = u32::MAX;
pub const INPUT_NO_CONTROLLER_ID: u32 = u32::MAX;
pub const INPUT_NO_PRIMITIVE: u32 = u32::MAX;
pub const INPUT_LOOP_MAPPING_NAME_CAPACITY: usize = 128;

#[repr(u32)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum InputProbe {
    Passive = 1,
    Loopback = 2,
    OriginalOnlineDisarmed = 3,
}

impl InputProbe {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            1 => Ok(Self::Passive),
            2 => Ok(Self::Loopback),
            3 => Ok(Self::OriginalOnlineDisarmed),
            _ => Err(AppError::evidence(format!(
                "input native shared block has unknown probe {value}"
            ))),
        }
    }
}

#[repr(i32)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum InputNativeState {
    Empty = 0,
    Initializing = 1,
    Ready = 2,
    Failed = 3,
    ShuttingDown = 4,
    Stopped = 5,
    Unsafe = 6,
}

impl InputNativeState {
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
                "input native shared state has unknown value {value}"
            ))),
        }
    }
}

#[repr(i32)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum InputNativeError {
    None = 0,
    MappingInvalid = 1,
    TargetMismatch = 2,
    DebuggerPresent = 3,
    InvalidProbe = 4,
    CompanionLoopInvalid = 5,
    HookTargetMismatch = 6,
    HookInstallFailed = 7,
    WrongThread = 8,
    RingOverflow = 9,
    RestoreFailed = 10,
    UnknownAction = 11,
    ScopeViolation = 12,
}

impl InputNativeError {
    pub fn from_raw(value: i32) -> AppResult<Self> {
        match value {
            0 => Ok(Self::None),
            1 => Ok(Self::MappingInvalid),
            2 => Ok(Self::TargetMismatch),
            3 => Ok(Self::DebuggerPresent),
            4 => Ok(Self::InvalidProbe),
            5 => Ok(Self::CompanionLoopInvalid),
            6 => Ok(Self::HookTargetMismatch),
            7 => Ok(Self::HookInstallFailed),
            8 => Ok(Self::WrongThread),
            9 => Ok(Self::RingOverflow),
            10 => Ok(Self::RestoreFailed),
            11 => Ok(Self::UnknownAction),
            12 => Ok(Self::ScopeViolation),
            _ => Err(AppError::evidence(format!(
                "input native shared block has unknown error {value}"
            ))),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
#[repr(u32)]
pub enum InputNativeEventKind {
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
}

impl InputNativeEventKind {
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
            _ => Err(AppError::evidence(format!(
                "input native event has unknown kind {value}"
            ))),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
#[repr(u32)]
pub enum InputClassification {
    Unknown = 0,
    UniqueGameplay = 1,
    NoMatch = 2,
    AmbiguousOrOutsideGameplay = 3,
    InvalidLayout = 4,
    UnstableSnapshot = 5,
    ExplicitGameplay = 6,
    LogicalOverrideDiagnostic = 7,
}

impl InputClassification {
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
            _ => Err(AppError::evidence(format!(
                "input native event has unknown classification {value}"
            ))),
        }
    }
}

pub const INPUT_FLAG_LOOP_KNOWN: u32 = 1 << 0;
pub const INPUT_FLAG_MAIN_THREAD: u32 = 1 << 1;
pub const INPUT_FLAG_WRONG_THREAD: u32 = 1 << 2;
pub const INPUT_FLAG_ORIGINAL_CALLED: u32 = 1 << 3;
pub const INPUT_FLAG_RESULT_TRUE: u32 = 1 << 4;
pub const INPUT_FLAG_KEYBOARD_DEVICE: u32 = 1 << 5;
pub const INPUT_FLAG_GAMEPAD_DEVICE: u32 = 1 << 6;
pub const INPUT_FLAG_VALUE_VALID: u32 = 1 << 7;
pub const INPUT_FLAG_REDACTED: u32 = 1 << 8;
pub const INPUT_KNOWN_FLAGS: u32 = (1 << 9) - 1;

#[derive(Clone, Copy, Debug)]
#[repr(C, align(8))]
pub struct NativeInputEvent {
    pub published_sequence: i64,
    pub qpc: i64,
    pub loop_iteration: u64,
    pub thread_id: u32,
    pub kind: u32,
    pub phase: u32,
    pub flags: u32,
    pub return_rva: u32,
    pub original_calls: u32,
    pub device_key: u32,
    pub controller_id: u32,
    pub selected_device_key: u32,
    pub action: u32,
    pub primitive: u32,
    pub value_bits: u32,
    pub classification: u32,
    pub error_code: u32,
}

#[repr(C, align(8))]
pub struct SharedInputBlock {
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
    pub loop_mapping_name: [u16; INPUT_LOOP_MAPPING_NAME_CAPACITY],
    pub state: i32,
    pub error_code: i32,
    pub installed_hooks_mask: u64,
    pub restored_hooks_mask: u64,
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
    pub events: [NativeInputEvent; INPUT_EVENT_CAPACITY],
}

#[derive(Clone, Debug)]
pub struct NativeInputEventRecord {
    pub sequence: u64,
    pub event: NativeInputEvent,
}

pub fn validate_input_block_header(
    magic: u64,
    schema_version: u32,
    struct_size: u32,
) -> AppResult<()> {
    if magic != INPUT_IPC_MAGIC
        || schema_version != INPUT_IPC_SCHEMA_VERSION
        || struct_size as usize != std::mem::size_of::<SharedInputBlock>()
    {
        return Err(AppError::evidence(
            "input IPC header is not the exact ICSI04V1 schema-2 layout",
        ));
    }
    Ok(())
}

fn validate_create_inputs(
    name: &str,
    target_pid: u32,
    nonce: &[u8; 32],
    target_sha256: &[u8; 32],
    loop_gate_sha256: &[u8; 32],
    loop_mapping_name: &str,
) -> AppResult<Vec<u16>> {
    if name.is_empty()
        || target_pid == 0
        || nonce.iter().all(|byte| *byte == 0)
        || target_sha256.iter().all(|byte| *byte == 0)
        || loop_gate_sha256.iter().all(|byte| *byte == 0)
        || loop_mapping_name.is_empty()
        || loop_mapping_name.contains('\0')
    {
        return Err(AppError::usage(
            "input mapping requires nonzero target/identities and a companion loop mapping",
        ));
    }
    let loop_wide = loop_mapping_name.encode_utf16().collect::<Vec<_>>();
    if loop_wide.len() >= INPUT_LOOP_MAPPING_NAME_CAPACITY {
        return Err(AppError::usage("companion loop mapping name is too long"));
    }
    Ok(loop_wide)
}

#[cfg(test)]
fn drain_snapshot(
    slots: &[NativeInputEvent],
    capacity: usize,
    produced: u64,
    overwritten: u64,
    next_sequence: &mut u64,
) -> AppResult<Vec<NativeInputEventRecord>> {
    if capacity == 0 || slots.len() < capacity {
        return Err(AppError::usage("input ring snapshot has invalid capacity"));
    }
    if overwritten != 0 || produced.saturating_sub(*next_sequence) > capacity as u64 {
        return Err(AppError::evidence(
            "input event ring overflowed before controller drain",
        ));
    }
    let mut records = Vec::new();
    while *next_sequence < produced {
        let sequence = next_sequence
            .checked_add(1)
            .ok_or_else(|| AppError::evidence("input event sequence overflow"))?;
        let index = usize::try_from((sequence - 1) % capacity as u64)
            .map_err(|_| AppError::evidence("input ring index overflow"))?;
        let event = slots[index];
        if event.published_sequence != sequence as i64 {
            break;
        }
        records.push(NativeInputEventRecord { sequence, event });
        *next_sequence = sequence;
    }
    Ok(records)
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

    pub struct InputMapping {
        handle: HANDLE,
        view: NonNull<SharedInputBlock>,
        name: String,
    }

    impl InputMapping {
        #[allow(clippy::too_many_arguments)]
        pub fn create(
            name: String,
            target_pid: u32,
            probe: InputProbe,
            scenario: u32,
            nonce: [u8; 32],
            target_sha256: [u8; 32],
            loop_gate_sha256: [u8; 32],
            loop_mapping_name: &str,
        ) -> AppResult<Self> {
            let loop_wide = validate_create_inputs(
                &name,
                target_pid,
                &nonce,
                &target_sha256,
                &loop_gate_sha256,
                loop_mapping_name,
            )?;
            let wide = name
                .encode_utf16()
                .chain(std::iter::once(0))
                .collect::<Vec<_>>();
            let size = size_of::<SharedInputBlock>();
            let size_low =
                u32::try_from(size).map_err(|_| AppError::usage("input mapping is too large"))?;
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
            let Some(view) = NonNull::new(raw.Value.cast::<SharedInputBlock>()) else {
                let _ = unsafe { CloseHandle(handle) };
                return Err(AppError::evidence("MapViewOfFile returned null"));
            };
            unsafe {
                let block = view.as_ptr();
                write_volatile(&raw mut (*block).magic, INPUT_IPC_MAGIC);
                write_volatile(&raw mut (*block).schema_version, INPUT_IPC_SCHEMA_VERSION);
                write_volatile(&raw mut (*block).struct_size, size_low);
                write_volatile(&raw mut (*block).target_pid, target_pid);
                write_volatile(&raw mut (*block).probe, probe as u32);
                write_volatile(&raw mut (*block).scenario, scenario);
                (*block).nonce = nonce;
                (*block).target_sha256 = target_sha256;
                (*block).loop_gate_sha256 = loop_gate_sha256;
                (&mut (*block).loop_mapping_name)[..loop_wide.len()].copy_from_slice(&loop_wide);
                write_volatile(&raw mut (*block).generation, 1);
            }
            Ok(Self { handle, view, name })
        }

        pub fn name(&self) -> &str {
            &self.name
        }

        fn validate_header(&self) -> AppResult<()> {
            let block = self.view.as_ptr();
            validate_input_block_header(
                unsafe { read_volatile(&raw const (*block).magic) },
                unsafe { read_volatile(&raw const (*block).schema_version) },
                unsafe { read_volatile(&raw const (*block).struct_size) },
            )
        }

        pub fn state(&self) -> AppResult<InputNativeState> {
            self.validate_header()?;
            InputNativeState::from_raw(unsafe {
                read_volatile(&raw const (*self.view.as_ptr()).state)
            })
        }

        pub fn error_code(&self) -> i32 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).error_code) }
        }

        pub fn installed_hooks_mask(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).installed_hooks_mask) }
        }

        pub fn restored_hooks_mask(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).restored_hooks_mask) }
        }

        pub fn overwritten_events(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).overwritten_events) }
        }

        pub fn redacted_events(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).redacted_events) }
        }

        pub fn main_thread_id(&self) -> u32 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).main_thread_id) }
        }

        pub fn producer_closed(&self) -> bool {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).producer_closed) != 0 }
        }

        #[cfg(test)]
        pub(super) fn generation(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).generation) }
        }

        pub fn drain(&self, next_sequence: &mut u64) -> AppResult<Vec<NativeInputEventRecord>> {
            self.validate_header()?;
            let block = self.view.as_ptr();
            if self.overwritten_events() != 0 {
                return Err(AppError::evidence(
                    "input event ring reports overwritten events before controller drain",
                ));
            }
            let produced = unsafe { read_volatile(&raw const (*block).next_sequence) };
            if produced.saturating_sub(*next_sequence) > INPUT_EVENT_CAPACITY as u64 {
                return Err(AppError::evidence(
                    "input event ring overflowed before controller drain",
                ));
            }
            let mut records = Vec::new();
            while *next_sequence < produced {
                let sequence = next_sequence
                    .checked_add(1)
                    .ok_or_else(|| AppError::evidence("input event sequence overflow"))?;
                let index = usize::try_from((sequence - 1) % INPUT_EVENT_CAPACITY as u64)
                    .map_err(|_| AppError::evidence("input ring index overflow"))?;
                let slot = unsafe { &raw const (*block).events[index] };
                let published_before =
                    unsafe { read_volatile(&raw const (*slot).published_sequence) };
                if published_before != sequence as i64 {
                    break;
                }
                fence(Ordering::Acquire);
                let event = unsafe { read_volatile(slot) };
                fence(Ordering::Acquire);
                let published_after =
                    unsafe { read_volatile(&raw const (*slot).published_sequence) };
                if published_before != published_after
                    || published_after != sequence as i64
                    || event.published_sequence != sequence as i64
                {
                    break;
                }
                records.push(NativeInputEventRecord { sequence, event });
                *next_sequence = sequence;
            }
            let produced_after = unsafe { read_volatile(&raw const (*block).next_sequence) };
            if self.overwritten_events() != 0
                || produced_after.saturating_sub(*next_sequence) > INPUT_EVENT_CAPACITY as u64
            {
                return Err(AppError::evidence(
                    "input event ring overflowed during controller drain",
                ));
            }
            unsafe {
                write_volatile(&raw mut (*block).controller_sequence, *next_sequence);
            }
            Ok(records)
        }
    }

    impl Drop for InputMapping {
        fn drop(&mut self) {
            let _ = unsafe {
                UnmapViewOfFile(MEMORY_MAPPED_VIEW_ADDRESS {
                    Value: self.view.as_ptr().cast(),
                })
            };
            let _ = unsafe { CloseHandle(self.handle) };
        }
    }
}

#[cfg(windows)]
pub use windows_mapping::InputMapping;

#[cfg(test)]
mod tests {
    use super::*;

    fn event(sequence: i64) -> NativeInputEvent {
        NativeInputEvent {
            published_sequence: sequence,
            qpc: sequence,
            loop_iteration: sequence as u64,
            thread_id: 1,
            kind: InputNativeEventKind::LogicalPressedExit as u32,
            phase: crate::loop_ipc::LoopPhase::InputQuery as u32,
            flags: INPUT_FLAG_LOOP_KNOWN
                | INPUT_FLAG_MAIN_THREAD
                | INPUT_FLAG_ORIGINAL_CALLED
                | INPUT_FLAG_VALUE_VALID,
            return_rva: 1,
            original_calls: 1,
            device_key: 0,
            controller_id: 0,
            selected_device_key: 0,
            action: 0,
            primitive: 0x1234,
            value_bits: 1,
            classification: InputClassification::ExplicitGameplay as u32,
            error_code: 0,
        }
    }

    #[test]
    fn schema_two_input_ipc_layout_is_stable() {
        assert_eq!(INPUT_IPC_MAGIC.to_le_bytes(), *b"ICSI04V1");
        assert_eq!(INPUT_IPC_SCHEMA_VERSION, 2);
        assert_eq!(INPUT_HOOK_MASK, 0x7FF);
        assert_eq!(std::mem::align_of::<NativeInputEvent>(), 8);
        assert_eq!(std::mem::size_of::<NativeInputEvent>(), 80);
        assert_eq!(std::mem::align_of::<SharedInputBlock>(), 8);
        assert_eq!(
            std::mem::offset_of!(SharedInputBlock, installed_hooks_mask),
            392
        );
        assert_eq!(std::mem::offset_of!(SharedInputBlock, events), 472);
        assert_eq!(std::mem::size_of::<SharedInputBlock>(), 5_243_352);
    }

    #[test]
    fn old_schema_and_unknown_enums_fail_closed() {
        assert!(validate_input_block_header(INPUT_IPC_MAGIC, 1, 136).is_err());
        assert!(InputProbe::from_raw(99).is_err());
        assert!(InputNativeState::from_raw(99).is_err());
        assert!(InputNativeError::from_raw(99).is_err());
        assert!(InputNativeEventKind::from_raw(99).is_err());
        assert!(InputClassification::from_raw(99).is_err());
        assert!(crate::loop_ipc::LoopPhase::from_raw(99).is_err());
    }

    #[test]
    fn ring_drain_is_contiguous_and_fails_closed_on_overwrite() {
        let slots = [event(1), event(2), event(0), event(0)];
        let mut next = 0;
        let records = drain_snapshot(&slots, slots.len(), 2, 0, &mut next).unwrap();
        assert_eq!(records.len(), 2);
        assert_eq!(next, 2);

        let mut stalled = 0;
        let incomplete = [event(1), event(0), event(0), event(0)];
        let records = drain_snapshot(&incomplete, incomplete.len(), 2, 0, &mut stalled).unwrap();
        assert_eq!(records.len(), 1);
        assert_eq!(stalled, 1);

        let mut zero = 0;
        assert!(drain_snapshot(&slots, slots.len(), 5, 0, &mut zero).is_err());
        assert!(drain_snapshot(&slots, slots.len(), 2, 1, &mut zero).is_err());
    }

    #[test]
    fn create_identity_rejects_empty_or_oversized_companion_names() {
        let hash = [1_u8; 32];
        assert!(validate_create_inputs("map", 1, &hash, &hash, &hash, "loop").is_ok());
        assert!(validate_create_inputs("map", 0, &hash, &hash, &hash, "loop").is_err());
        assert!(validate_create_inputs("map", 1, &hash, &hash, &hash, "").is_err());
        assert!(
            validate_create_inputs(
                "map",
                1,
                &hash,
                &hash,
                &hash,
                &"x".repeat(INPUT_LOOP_MAPPING_NAME_CAPACITY),
            )
            .is_err()
        );
    }

    #[cfg(windows)]
    #[test]
    fn created_mapping_initializes_the_native_generation() {
        let name = format!("Local\\IsaacCsT0.Input.Test.{}", std::process::id());
        let mapping = InputMapping::create(
            name,
            std::process::id(),
            InputProbe::Passive,
            1,
            [1; 32],
            [2; 32],
            [3; 32],
            "Local\\IsaacCsT0.Loop.Test",
        )
        .unwrap();
        assert_eq!(mapping.generation(), 1);
        assert_eq!(mapping.state().unwrap(), InputNativeState::Empty);
        assert_eq!(mapping.installed_hooks_mask(), 0);
        assert_eq!(mapping.restored_hooks_mask(), 0);
        assert!(!mapping.producer_closed());
    }
}
