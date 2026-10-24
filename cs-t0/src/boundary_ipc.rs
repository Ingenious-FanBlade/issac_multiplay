use serde::{Deserialize, Serialize};

use crate::error::{AppError, AppResult};

pub const BOUNDARY_IPC_MAGIC: u32 = 0x3342_5343;
pub const BOUNDARY_IPC_VERSION: u32 = 3;
pub const BOUNDARY_EVENT_CAPACITY: usize = 4096;
pub const UNAVAILABLE_U32: u32 = u32::MAX;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum NativeBoundaryProbe {
    Passive = 1,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(i32)]
pub enum BoundaryNativeState {
    Empty = 0,
    Initializing = 1,
    Ready = 2,
    Failed = 3,
    ShuttingDown = 4,
    Stopped = 5,
    Unsafe = 6,
}

impl BoundaryNativeState {
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
                "boundary native shared state has unknown value {value}"
            ))),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
#[repr(u32)]
pub enum BoundaryEventKind {
    NativeInitialized = 1,
    HooksInstalled = 2,
    InputSampleEnter = 3,
    InputSampleExit = 4,
    InputApply = 5,
    GameplayEnter = 6,
    GameplayExit = 7,
    EntityEnter = 8,
    EntityExit = 9,
    RemoveEnter = 10,
    RemoveExit = 11,
    PostUpdateEnter = 12,
    PostUpdateExit = 13,
    RenderEnter = 14,
    RenderExit = 15,
    PostRenderEnter = 16,
    PostRenderExit = 17,
    LuaClose = 18,
    Retired = 19,
    HooksRestored = 20,
    Failure = 21,
    SpawnEnter = 22,
    SpawnExit = 23,
    OnlineDispatchEnter = 24,
    OnlineDispatchExit = 25,
    LuaCallbackEnter = 26,
    LuaCallbackExit = 27,
    WindowStart = 28,
    WindowEnd = 29,
    LuaCreated = 30,
    NewRoomEnter = 31,
    NewRoomExit = 32,
}

impl BoundaryEventKind {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            1 => Ok(Self::NativeInitialized),
            2 => Ok(Self::HooksInstalled),
            3 => Ok(Self::InputSampleEnter),
            4 => Ok(Self::InputSampleExit),
            5 => Ok(Self::InputApply),
            6 => Ok(Self::GameplayEnter),
            7 => Ok(Self::GameplayExit),
            8 => Ok(Self::EntityEnter),
            9 => Ok(Self::EntityExit),
            10 => Ok(Self::RemoveEnter),
            11 => Ok(Self::RemoveExit),
            12 => Ok(Self::PostUpdateEnter),
            13 => Ok(Self::PostUpdateExit),
            14 => Ok(Self::RenderEnter),
            15 => Ok(Self::RenderExit),
            16 => Ok(Self::PostRenderEnter),
            17 => Ok(Self::PostRenderExit),
            18 => Ok(Self::LuaClose),
            19 => Ok(Self::Retired),
            20 => Ok(Self::HooksRestored),
            21 => Ok(Self::Failure),
            22 => Ok(Self::SpawnEnter),
            23 => Ok(Self::SpawnExit),
            24 => Ok(Self::OnlineDispatchEnter),
            25 => Ok(Self::OnlineDispatchExit),
            26 => Ok(Self::LuaCallbackEnter),
            27 => Ok(Self::LuaCallbackExit),
            28 => Ok(Self::WindowStart),
            29 => Ok(Self::WindowEnd),
            30 => Ok(Self::LuaCreated),
            31 => Ok(Self::NewRoomEnter),
            32 => Ok(Self::NewRoomExit),
            _ => Err(AppError::evidence(format!(
                "boundary native event has unknown kind {value}"
            ))),
        }
    }
}

pub const BOUNDARY_FLAG_WRONG_THREAD: u32 = 1 << 0;
pub const BOUNDARY_FLAG_ORIGINAL_ONLINE: u32 = 1 << 1;
pub const BOUNDARY_FLAG_LIFECYCLE_RETIRED: u32 = 1 << 2;
pub const BOUNDARY_FLAG_SUMMARY_VALID: u32 = 1 << 3;
pub const BOUNDARY_FLAG_GAME_FRAME_VALID: u32 = 1 << 4;
pub const BOUNDARY_FLAG_ONLINE_FRAME_VALID: u32 = 1 << 5;
pub const BOUNDARY_FLAG_ENTITY_COUNT_VALID: u32 = 1 << 6;
pub const BOUNDARY_FLAG_RETIREMENT_MENU_RETURN: u32 = 1 << 7;
pub const BOUNDARY_FLAG_RETIREMENT_LUA_CLOSE: u32 = 1 << 8;
pub const BOUNDARY_FLAG_RETIREMENT_LUA_REBUILD: u32 = 1 << 9;
pub const BOUNDARY_FLAG_LUA_ROOT_MATCH: u32 = 1 << 10;
pub const BOUNDARY_FLAG_LUA_NEW_ROOT: u32 = 1 << 11;
pub const BOUNDARY_FLAG_GAMEPLAY_DISPATCH_SUPPRESSED: u32 = 1 << 12;
pub const BOUNDARY_FLAG_MAIN_LOOP_STATE_VALID: u32 = 1 << 13;

#[derive(Clone, Copy, Debug)]
#[repr(C, align(8))]
pub struct NativeBoundaryEvent {
    pub published_sequence: i64,
    pub qpc: i64,
    pub generation: u64,
    pub summary_before: u64,
    pub summary_after: u64,
    pub thread_id: u32,
    pub kind: u32,
    pub depth: u32,
    pub flags: u32,
    pub return_rva: u32,
    pub game_frame_before: u32,
    pub game_frame_after: u32,
    pub online_frame_before: u32,
    pub online_frame_after: u32,
    pub entity_count_before: u32,
    pub entity_count_after: u32,
    pub original_calls: u32,
    pub result_value: u32,
    pub error_code: u32,
    pub reserved: u32,
}

#[repr(C, align(8))]
pub struct SharedBoundaryBlock {
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
    pub hooks_mask: u32,
    pub main_thread_id: u32,
    pub generation: u64,
    pub retired_generation: u64,
    pub next_sequence: u64,
    pub controller_sequence: u64,
    pub overwritten_events: u64,
    pub active_hooks: i32,
    pub producer_closed: i32,
    pub events: [NativeBoundaryEvent; BOUNDARY_EVENT_CAPACITY],
}

#[derive(Clone, Debug)]
pub struct NativeBoundaryEventRecord {
    pub sequence: u64,
    pub event: NativeBoundaryEvent,
}

#[cfg(windows)]
mod windows_mapping {
    use std::mem::size_of;
    use std::ptr::{NonNull, read_volatile, write_volatile};

    use windows::Win32::Foundation::{CloseHandle, HANDLE, INVALID_HANDLE_VALUE};
    use windows::Win32::System::Memory::{
        CreateFileMappingW, FILE_MAP_ALL_ACCESS, MEMORY_MAPPED_VIEW_ADDRESS, MapViewOfFile,
        PAGE_READWRITE, UnmapViewOfFile,
    };
    use windows::core::PCWSTR;

    use super::*;

    pub struct BoundaryMapping {
        handle: HANDLE,
        view: NonNull<SharedBoundaryBlock>,
        name: String,
    }

    impl BoundaryMapping {
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
            let size = size_of::<SharedBoundaryBlock>();
            let size_low = u32::try_from(size)
                .map_err(|_| AppError::usage("boundary mapping is too large"))?;
            // SAFETY: INVALID_HANDLE_VALUE requests an exact-size pagefile mapping.
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
            // SAFETY: the handle names the new mapping and size is exact.
            let raw = unsafe { MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, size) };
            let Some(view) = NonNull::new(raw.Value.cast::<SharedBoundaryBlock>()) else {
                let _ = unsafe { CloseHandle(handle) };
                return Err(AppError::evidence("MapViewOfFile returned null"));
            };
            // SAFETY: a new pagefile mapping is zero initialized and covers the block.
            unsafe {
                let block = view.as_ptr();
                write_volatile(&raw mut (*block).magic, BOUNDARY_IPC_MAGIC);
                write_volatile(&raw mut (*block).version, BOUNDARY_IPC_VERSION);
                write_volatile(&raw mut (*block).size, size_low);
                write_volatile(&raw mut (*block).target_pid, target_pid);
                write_volatile(&raw mut (*block).probe, NativeBoundaryProbe::Passive as u32);
                write_volatile(&raw mut (*block).scenario, scenario);
                (*block).nonce = nonce;
            }
            Ok(Self { handle, view, name })
        }

        pub fn name(&self) -> &str {
            &self.name
        }

        pub fn state(&self) -> AppResult<BoundaryNativeState> {
            // SAFETY: aligned field remains mapped for self's lifetime.
            BoundaryNativeState::from_raw(unsafe {
                read_volatile(&raw const (*self.view.as_ptr()).state)
            })
        }

        pub fn error_code(&self) -> i32 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).error_code) }
        }

        pub fn hooks_mask(&self) -> u32 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).hooks_mask) }
        }

        pub fn overwritten_events(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).overwritten_events) }
        }

        pub fn drain(&self, next_sequence: &mut u64) -> AppResult<Vec<NativeBoundaryEventRecord>> {
            let block = self.view.as_ptr();
            let produced = unsafe { read_volatile(&raw const (*block).next_sequence) };
            if produced.saturating_sub(*next_sequence) > BOUNDARY_EVENT_CAPACITY as u64 {
                return Err(AppError::evidence(
                    "boundary event ring overflowed before controller drain",
                ));
            }
            let mut records = Vec::new();
            while *next_sequence < produced {
                let sequence = next_sequence
                    .checked_add(1)
                    .ok_or_else(|| AppError::evidence("boundary event sequence overflow"))?;
                let index = usize::try_from((sequence - 1) % BOUNDARY_EVENT_CAPACITY as u64)
                    .map_err(|_| AppError::evidence("boundary ring index overflow"))?;
                let event = unsafe { read_volatile(&raw const (*block).events[index]) };
                if event.published_sequence != sequence as i64 {
                    break;
                }
                records.push(NativeBoundaryEventRecord { sequence, event });
                *next_sequence = sequence;
            }
            unsafe {
                write_volatile(&raw mut (*block).controller_sequence, *next_sequence);
            }
            Ok(records)
        }
    }

    impl Drop for BoundaryMapping {
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
pub use windows_mapping::BoundaryMapping;

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn boundary_ipc_layout_is_stable() {
        assert_eq!(BOUNDARY_IPC_VERSION, 3);
        assert_eq!(std::mem::align_of::<NativeBoundaryEvent>(), 8);
        assert_eq!(std::mem::size_of::<NativeBoundaryEvent>(), 104);
        assert_eq!(std::mem::align_of::<SharedBoundaryBlock>(), 8);
        assert_eq!(std::mem::size_of::<SharedBoundaryBlock>(), 426_112);
    }

    #[test]
    fn event_kind_and_state_reject_unknown_values() {
        assert_eq!(
            BoundaryEventKind::from_raw(12).unwrap(),
            BoundaryEventKind::PostUpdateEnter
        );
        assert_eq!(
            BoundaryEventKind::from_raw(30).unwrap(),
            BoundaryEventKind::LuaCreated
        );
        assert_eq!(
            BoundaryEventKind::from_raw(32).unwrap(),
            BoundaryEventKind::NewRoomExit
        );
        assert!(BoundaryEventKind::from_raw(99).is_err());
        assert!(BoundaryNativeState::from_raw(99).is_err());
    }
}
