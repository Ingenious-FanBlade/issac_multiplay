use serde::{Deserialize, Serialize};

use crate::error::{AppError, AppResult};

pub const LOOP_IPC_MAGIC: u32 = 0x5233_3054;
pub const LOOP_IPC_VERSION: u32 = 5;
pub const LOOP_EVENT_CAPACITY: usize = 65_536;
pub const LOOP_HOOK_MASK: u64 = (1_u64 << 32) - 1;
pub const LOOP_NO_ACTION: u32 = u32::MAX;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum NativeLoopProbe {
    Passive = 1,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(i32)]
pub enum LoopNativeState {
    Empty = 0,
    Initializing = 1,
    Ready = 2,
    Failed = 3,
    ShuttingDown = 4,
    Stopped = 5,
    Unsafe = 6,
}

impl LoopNativeState {
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
                "loop native shared state has unknown value {value}"
            ))),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
#[repr(u32)]
pub enum LoopPhase {
    OutsideEnvelope = 0,
    LoopBoundary = 1,
    MessagePump = 2,
    InputQuery = 3,
    GameplayUpdate = 4,
    EntityMutation = 5,
    PostUpdate = 6,
    Render = 7,
    RoomLifecycle = 8,
    RunLifecycle = 9,
}

impl LoopPhase {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            0 => Ok(Self::OutsideEnvelope),
            1 => Ok(Self::LoopBoundary),
            2 => Ok(Self::MessagePump),
            3 => Ok(Self::InputQuery),
            4 => Ok(Self::GameplayUpdate),
            5 => Ok(Self::EntityMutation),
            6 => Ok(Self::PostUpdate),
            7 => Ok(Self::Render),
            8 => Ok(Self::RoomLifecycle),
            9 => Ok(Self::RunLifecycle),
            _ => Err(AppError::evidence(format!(
                "loop native event has unknown phase {value}"
            ))),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
#[repr(u32)]
pub enum LoopNativeEventKind {
    NativeInitialized = 1,
    HooksInstalled = 2,
    LoopBegin = 3,
    LoopClockExit = 4,
    UpdateEnter = 5,
    UpdateExit = 6,
    GameplayEnter = 7,
    GameplayExit = 8,
    EntityEnter = 9,
    EntityExit = 10,
    RemoveEnter = 11,
    RemoveExit = 12,
    PostUpdateEnter = 13,
    PostUpdateExit = 14,
    SpawnEnter = 15,
    SpawnExit = 16,
    NewRoomEnter = 17,
    NewRoomExit = 18,
    RunTeardownEnter = 19,
    RunTeardownExit = 20,
    RenderEnter = 21,
    RenderExit = 22,
    PeekMessage = 23,
    TranslateMessage = 24,
    DispatchMessage = 25,
    InputPressed = 26,
    InputTriggered = 27,
    InputScalar = 28,
    HooksRestored = 29,
    Failure = 30,
    InputManagerSampleEnter = 31,
    InputManagerSampleExit = 32,
    InputDeviceUpdateEnter = 33,
    InputDeviceUpdateExit = 34,
    InputPhysicalPollEnter = 35,
    InputPhysicalPollExit = 36,
    ConditionalAttemptEnter = 37,
    ConditionalAttemptExit = 38,
    AlternateOwnerEnter = 39,
    AlternateOwnerExit = 40,
    AlternateRoomEnter = 41,
    AlternateRoomExit = 42,
    AlternateTailEnter = 43,
    AlternateTailExit = 44,
    StandardPredicateEnter = 45,
    StandardPredicateExit = 46,
    StandardBodyEnter = 47,
    StandardBodyExit = 48,
    StandardRoomEnter = 49,
    StandardRoomExit = 50,
    StandardTailEnter = 51,
    StandardTailExit = 52,
    EntityListEnter = 53,
    EntityListExit = 54,
    AlternateStateApplyEnter = 55,
    AlternateStateApplyExit = 56,
    AlternateOutcomeEnter = 57,
    AlternateOutcomeExit = 58,
    StandardRoomLifecycleOwnerEnter = 59,
    StandardRoomLifecycleOwnerExit = 60,
    StandardRoomLifecycleSimulationEnter = 61,
    StandardRoomLifecycleSimulationExit = 62,
    RoomChangeOwnerEnter = 63,
    RoomChangeOwnerExit = 64,
    RoomChangeSimulationEnter = 65,
    RoomChangeSimulationExit = 66,
}

impl LoopNativeEventKind {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            1 => Ok(Self::NativeInitialized),
            2 => Ok(Self::HooksInstalled),
            3 => Ok(Self::LoopBegin),
            4 => Ok(Self::LoopClockExit),
            5 => Ok(Self::UpdateEnter),
            6 => Ok(Self::UpdateExit),
            7 => Ok(Self::GameplayEnter),
            8 => Ok(Self::GameplayExit),
            9 => Ok(Self::EntityEnter),
            10 => Ok(Self::EntityExit),
            11 => Ok(Self::RemoveEnter),
            12 => Ok(Self::RemoveExit),
            13 => Ok(Self::PostUpdateEnter),
            14 => Ok(Self::PostUpdateExit),
            15 => Ok(Self::SpawnEnter),
            16 => Ok(Self::SpawnExit),
            17 => Ok(Self::NewRoomEnter),
            18 => Ok(Self::NewRoomExit),
            19 => Ok(Self::RunTeardownEnter),
            20 => Ok(Self::RunTeardownExit),
            21 => Ok(Self::RenderEnter),
            22 => Ok(Self::RenderExit),
            23 => Ok(Self::PeekMessage),
            24 => Ok(Self::TranslateMessage),
            25 => Ok(Self::DispatchMessage),
            26 => Ok(Self::InputPressed),
            27 => Ok(Self::InputTriggered),
            28 => Ok(Self::InputScalar),
            29 => Ok(Self::HooksRestored),
            30 => Ok(Self::Failure),
            31 => Ok(Self::InputManagerSampleEnter),
            32 => Ok(Self::InputManagerSampleExit),
            33 => Ok(Self::InputDeviceUpdateEnter),
            34 => Ok(Self::InputDeviceUpdateExit),
            35 => Ok(Self::InputPhysicalPollEnter),
            36 => Ok(Self::InputPhysicalPollExit),
            37 => Ok(Self::ConditionalAttemptEnter),
            38 => Ok(Self::ConditionalAttemptExit),
            39 => Ok(Self::AlternateOwnerEnter),
            40 => Ok(Self::AlternateOwnerExit),
            41 => Ok(Self::AlternateRoomEnter),
            42 => Ok(Self::AlternateRoomExit),
            43 => Ok(Self::AlternateTailEnter),
            44 => Ok(Self::AlternateTailExit),
            45 => Ok(Self::StandardPredicateEnter),
            46 => Ok(Self::StandardPredicateExit),
            47 => Ok(Self::StandardBodyEnter),
            48 => Ok(Self::StandardBodyExit),
            49 => Ok(Self::StandardRoomEnter),
            50 => Ok(Self::StandardRoomExit),
            51 => Ok(Self::StandardTailEnter),
            52 => Ok(Self::StandardTailExit),
            53 => Ok(Self::EntityListEnter),
            54 => Ok(Self::EntityListExit),
            55 => Ok(Self::AlternateStateApplyEnter),
            56 => Ok(Self::AlternateStateApplyExit),
            57 => Ok(Self::AlternateOutcomeEnter),
            58 => Ok(Self::AlternateOutcomeExit),
            59 => Ok(Self::StandardRoomLifecycleOwnerEnter),
            60 => Ok(Self::StandardRoomLifecycleOwnerExit),
            61 => Ok(Self::StandardRoomLifecycleSimulationEnter),
            62 => Ok(Self::StandardRoomLifecycleSimulationExit),
            63 => Ok(Self::RoomChangeOwnerEnter),
            64 => Ok(Self::RoomChangeOwnerExit),
            65 => Ok(Self::RoomChangeSimulationEnter),
            66 => Ok(Self::RoomChangeSimulationExit),
            _ => Err(AppError::evidence(format!(
                "loop native event has unknown kind {value}"
            ))),
        }
    }
}

pub const LOOP_FLAG_LOOP_KNOWN: u32 = 1 << 0;
pub const LOOP_FLAG_MAIN_THREAD: u32 = 1 << 1;
pub const LOOP_FLAG_WRONG_THREAD: u32 = 1 << 2;
pub const LOOP_FLAG_ORIGINAL_CALLED: u32 = 1 << 3;
pub const LOOP_FLAG_RESULT_TRUE: u32 = 1 << 4;
pub const LOOP_FLAG_ACTION_GAMEPLAY: u32 = 1 << 5;
pub const LOOP_FLAG_KEYBOARD_DEVICE: u32 = 1 << 6;
pub const LOOP_FLAG_GAMEPAD_DEVICE: u32 = 1 << 7;

#[derive(Clone, Copy, Debug)]
#[repr(C, align(8))]
pub struct NativeLoopEvent {
    pub published_sequence: i64,
    pub qpc: i64,
    pub loop_iteration: u64,
    pub thread_id: u32,
    pub kind: u32,
    pub phase: u32,
    pub action: u32,
    pub value_bits: u32,
    pub flags: u32,
    pub return_rva: u32,
    pub original_calls: u32,
    pub error_code: u32,
    pub device_key: u32,
}

#[repr(C, align(8))]
pub struct SharedLoopBlock {
    pub magic: u32,
    pub version: u32,
    pub size: u32,
    pub target_pid: u32,
    pub probe: u32,
    pub scenario: u32,
    pub flags: u32,
    pub reserved0: u32,
    pub nonce: [u8; 32],
    pub state: i32,
    pub error_code: i32,
    pub hooks_mask: u64,
    pub main_thread_id: u32,
    pub current_loop_iteration: u64,
    pub next_sequence: u64,
    pub controller_sequence: u64,
    pub overwritten_events: u64,
    pub active_hooks: i32,
    pub producer_closed: i32,
    pub events: [NativeLoopEvent; LOOP_EVENT_CAPACITY],
}

#[derive(Clone, Debug)]
pub struct NativeLoopEventRecord {
    pub sequence: u64,
    pub event: NativeLoopEvent,
}

#[cfg(windows)]
mod windows_mapping {
    use std::mem::size_of;
    use std::ptr::{NonNull, read_volatile, write_volatile};
    use std::sync::atomic::{Ordering, fence};

    use windows::Win32::Foundation::{
        CloseHandle, ERROR_ALREADY_EXISTS, GetLastError, HANDLE, INVALID_HANDLE_VALUE,
    };
    use windows::Win32::System::Memory::{
        CreateFileMappingW, FILE_MAP_ALL_ACCESS, MEMORY_MAPPED_VIEW_ADDRESS, MapViewOfFile,
        PAGE_READWRITE, UnmapViewOfFile,
    };
    use windows::core::PCWSTR;

    use super::*;

    pub struct LoopMapping {
        handle: HANDLE,
        view: NonNull<SharedLoopBlock>,
        name: String,
    }

    impl LoopMapping {
        pub fn create(
            name: String,
            target_pid: u32,
            scenario: u32,
            nonce: [u8; 32],
        ) -> AppResult<Self> {
            let wide = name
                .encode_utf16()
                .chain(std::iter::once(0))
                .collect::<Vec<_>>();
            let size = size_of::<SharedLoopBlock>();
            let size_low =
                u32::try_from(size).map_err(|_| AppError::usage("loop mapping is too large"))?;
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
            if unsafe { GetLastError() } == ERROR_ALREADY_EXISTS {
                let _ = unsafe { CloseHandle(handle) };
                return Err(AppError::evidence(
                    "loop mapping name already exists; create-new identity is required",
                ));
            }
            let raw = unsafe { MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, size) };
            let Some(view) = NonNull::new(raw.Value.cast::<SharedLoopBlock>()) else {
                let _ = unsafe { CloseHandle(handle) };
                return Err(AppError::evidence("MapViewOfFile returned null"));
            };
            unsafe {
                let block = view.as_ptr();
                write_volatile(&raw mut (*block).magic, LOOP_IPC_MAGIC);
                write_volatile(&raw mut (*block).version, LOOP_IPC_VERSION);
                write_volatile(&raw mut (*block).size, size_low);
                write_volatile(&raw mut (*block).target_pid, target_pid);
                write_volatile(&raw mut (*block).probe, NativeLoopProbe::Passive as u32);
                write_volatile(&raw mut (*block).scenario, scenario);
                (*block).nonce = nonce;
            }
            Ok(Self { handle, view, name })
        }

        pub fn name(&self) -> &str {
            &self.name
        }

        pub fn state(&self) -> AppResult<LoopNativeState> {
            LoopNativeState::from_raw(unsafe {
                read_volatile(&raw const (*self.view.as_ptr()).state)
            })
        }

        pub fn error_code(&self) -> i32 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).error_code) }
        }

        pub fn hooks_mask(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).hooks_mask) }
        }

        pub fn overwritten_events(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).overwritten_events) }
        }

        pub fn main_thread_id(&self) -> u32 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).main_thread_id) }
        }

        pub fn producer_closed(&self) -> bool {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).producer_closed) != 0 }
        }

        pub fn drain(&self, next_sequence: &mut u64) -> AppResult<Vec<NativeLoopEventRecord>> {
            let block = self.view.as_ptr();
            if self.overwritten_events() != 0 {
                return Err(AppError::evidence(
                    "loop event ring reports overwritten events before controller drain",
                ));
            }
            let produced = unsafe { read_volatile(&raw const (*block).next_sequence) };
            if produced.saturating_sub(*next_sequence) > LOOP_EVENT_CAPACITY as u64 {
                return Err(AppError::evidence(
                    "loop event ring overflowed before controller drain",
                ));
            }
            let mut records = Vec::new();
            while *next_sequence < produced {
                let sequence = next_sequence
                    .checked_add(1)
                    .ok_or_else(|| AppError::evidence("loop event sequence overflow"))?;
                let index = usize::try_from((sequence - 1) % LOOP_EVENT_CAPACITY as u64)
                    .map_err(|_| AppError::evidence("loop ring index overflow"))?;
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
                records.push(NativeLoopEventRecord { sequence, event });
                *next_sequence = sequence;
            }
            let produced_after = unsafe { read_volatile(&raw const (*block).next_sequence) };
            if self.overwritten_events() != 0
                || produced_after.saturating_sub(*next_sequence) > LOOP_EVENT_CAPACITY as u64
            {
                return Err(AppError::evidence(
                    "loop event ring overflowed during controller drain",
                ));
            }
            unsafe {
                write_volatile(&raw mut (*block).controller_sequence, *next_sequence);
            }
            Ok(records)
        }
    }

    impl Drop for LoopMapping {
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
pub use windows_mapping::LoopMapping;

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn loop_ipc_layout_is_stable() {
        assert_eq!(LOOP_IPC_VERSION, 5);
        assert_eq!(LOOP_HOOK_MASK, 0xFFFF_FFFF);
        assert_eq!(std::mem::align_of::<NativeLoopEvent>(), 8);
        assert_eq!(std::mem::size_of::<NativeLoopEvent>(), 64);
        assert_eq!(std::mem::align_of::<SharedLoopBlock>(), 8);
        assert_eq!(std::mem::size_of::<SharedLoopBlock>(), 4_194_432);
        assert_eq!(std::mem::offset_of!(SharedLoopBlock, hooks_mask), 72);
        assert_eq!(std::mem::offset_of!(SharedLoopBlock, main_thread_id), 80);
        assert_eq!(
            std::mem::offset_of!(SharedLoopBlock, current_loop_iteration),
            88
        );
        assert_eq!(std::mem::offset_of!(SharedLoopBlock, events), 128);
    }

    #[test]
    fn native_loop_enums_reject_unknown_values() {
        assert_eq!(LoopPhase::from_raw(3).unwrap(), LoopPhase::InputQuery);
        assert_eq!(
            LoopNativeEventKind::from_raw(28).unwrap(),
            LoopNativeEventKind::InputScalar
        );
        assert_eq!(
            LoopNativeEventKind::from_raw(59).unwrap(),
            LoopNativeEventKind::StandardRoomLifecycleOwnerEnter
        );
        assert_eq!(
            LoopNativeEventKind::from_raw(62).unwrap(),
            LoopNativeEventKind::StandardRoomLifecycleSimulationExit
        );
        assert_eq!(
            LoopNativeEventKind::from_raw(63).unwrap(),
            LoopNativeEventKind::RoomChangeOwnerEnter
        );
        assert_eq!(
            LoopNativeEventKind::from_raw(66).unwrap(),
            LoopNativeEventKind::RoomChangeSimulationExit
        );
        assert!(LoopPhase::from_raw(99).is_err());
        assert!(LoopNativeEventKind::from_raw(99).is_err());
        assert!(LoopNativeState::from_raw(99).is_err());
    }
}
