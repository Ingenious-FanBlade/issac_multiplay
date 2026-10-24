use serde::{Deserialize, Serialize};

use crate::error::{AppError, AppResult};

pub const MODE_IPC_MAGIC: u32 = 0x324D_5343;
pub const MODE_IPC_VERSION: u32 = 2;
pub const MODE_EVENT_CAPACITY: usize = 4096;
pub const PAIRING_KEY_ENV: &str = "ISAAC_CS_T0_PAIR_KEY";

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum NativeScenario {
    ControlNoNative = 0,
    OfflineDisarmed = 1,
    OriginalOnline = 2,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum EndpointRole {
    Solo = 0,
    A = 1,
    B = 2,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(i32)]
pub enum NativeState {
    Empty = 0,
    Initializing = 1,
    Ready = 2,
    Failed = 3,
    ShuttingDown = 4,
    Stopped = 5,
    Unsafe = 6,
}

impl NativeState {
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
                "native shared state has unknown value {value}"
            ))),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
#[repr(u32)]
pub enum NativeEventKind {
    NativeInitialized = 1,
    HooksInstalled = 2,
    SteamHooksInstalled = 3,
    LuaCreated = 4,
    LuaSimulationPcall = 5,
    LuaClose = 6,
    Frame = 7,
    Dispatch = 8,
    SteamSend = 9,
    SteamAvailable = 10,
    SteamRead = 11,
    ModeFrozen = 12,
    ModeRetired = 13,
    HooksRestored = 14,
    Failure = 15,
    PostGameStart = 16,
    RunTeardown = 17,
}

impl NativeEventKind {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            1 => Ok(Self::NativeInitialized),
            2 => Ok(Self::HooksInstalled),
            3 => Ok(Self::SteamHooksInstalled),
            4 => Ok(Self::LuaCreated),
            5 => Ok(Self::LuaSimulationPcall),
            6 => Ok(Self::LuaClose),
            7 => Ok(Self::Frame),
            8 => Ok(Self::Dispatch),
            9 => Ok(Self::SteamSend),
            10 => Ok(Self::SteamAvailable),
            11 => Ok(Self::SteamRead),
            12 => Ok(Self::ModeFrozen),
            13 => Ok(Self::ModeRetired),
            14 => Ok(Self::HooksRestored),
            15 => Ok(Self::Failure),
            16 => Ok(Self::PostGameStart),
            17 => Ok(Self::RunTeardown),
            _ => Err(AppError::evidence(format!(
                "native event has unknown kind {value}"
            ))),
        }
    }
}

pub const EVENT_FLAG_RESULT_BOOL: u32 = 1 << 0;
pub const EVENT_FLAG_MANAGER_IDENTITY: u32 = 1 << 1;
pub const EVENT_FLAG_STABLE_ROSTER: u32 = 1 << 2;
pub const EVENT_FLAG_CHANNEL_ZERO: u32 = 1 << 3;
pub const EVENT_FLAG_PAYLOAD_HASH_AFTER: u32 = 1 << 4;
pub const EVENT_FLAG_ROOT_MATCH: u32 = 1 << 5;
pub const EVENT_FLAG_WRONG_THREAD: u32 = 1 << 6;
pub const EVENT_FLAG_FRAME_REGRESSED: u32 = 1 << 7;
pub const EVENT_FLAG_MANAGER_CHANGED: u32 = 1 << 8;
pub const EVENT_FLAG_LUA_CLOSED: u32 = 1 << 9;
pub const EVENT_FLAG_MENU_RETURN: u32 = 1 << 10;
pub const EVENT_FLAG_NONZERO_CHANNEL: u32 = 1 << 11;
pub const EVENT_FLAG_RUN_ACTIVE_BEFORE: u32 = 1 << 12;
pub const EVENT_FLAG_RUN_ACTIVE_AFTER_ZERO: u32 = 1 << 13;
pub const EVENT_FLAG_CONTINUED: u32 = 1 << 14;

#[derive(Clone, Copy, Debug)]
#[repr(C, align(8))]
pub struct NativeEvent {
    pub published_sequence: i64,
    pub qpc: i64,
    pub generation: u64,
    pub attempt_id: u64,
    pub thread_id: u32,
    pub kind: u32,
    pub mode: u32,
    pub flags: u32,
    pub online_before: u32,
    pub online_after: u32,
    pub channel: i32,
    pub send_mode: i32,
    pub payload_size: u32,
    pub output_size: u32,
    pub original_calls: u32,
    pub result_value: i32,
    pub signal_mask: u32,
    pub error_code: u32,
    pub active_before: u32,
    pub active_after: u32,
    pub peer_token: u64,
    pub payload_hash: [u8; 32],
    pub payload_hash_after: [u8; 32],
}

#[repr(C, align(8))]
pub struct SharedModeBlock {
    pub magic: u32,
    pub version: u32,
    pub size: u32,
    pub target_pid: u32,
    pub scenario: u32,
    pub endpoint: u32,
    pub requested_mode: u32,
    pub flags: u32,
    pub nonce: [u8; 32],
    pub pairing_key: [u8; 32],
    pub state: i32,
    pub error_code: i32,
    pub network_mode: i32,
    pub hooks_mask: u32,
    pub generation: u64,
    pub next_sequence: u64,
    pub controller_sequence: u64,
    pub overwritten_events: u64,
    pub active_hooks: i32,
    pub producer_closed: i32,
    pub main_thread_id: u32,
    pub reserved: u32,
    pub local_peer_token: u64,
    pub events: [NativeEvent; MODE_EVENT_CAPACITY],
}

#[derive(Clone, Debug)]
pub struct NativeEventRecord {
    pub sequence: u64,
    pub event: NativeEvent,
}

pub fn parse_pairing_key(value: &str) -> AppResult<[u8; 32]> {
    let decoded = hex::decode(value.trim())
        .map_err(|_| AppError::usage("pairing key must be 64 hexadecimal characters"))?;
    decoded
        .try_into()
        .map_err(|_| AppError::usage("pairing key must be exactly 32 bytes"))
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

    pub struct ModeMapping {
        handle: HANDLE,
        view: NonNull<SharedModeBlock>,
        name: String,
    }

    impl ModeMapping {
        pub fn create(
            name: String,
            target_pid: u32,
            scenario: NativeScenario,
            endpoint: EndpointRole,
            requested_mode: u32,
            nonce: [u8; 32],
            pairing_key: [u8; 32],
        ) -> AppResult<Self> {
            let wide = name
                .encode_utf16()
                .chain(std::iter::once(0))
                .collect::<Vec<_>>();
            let size = size_of::<SharedModeBlock>();
            let size_low =
                u32::try_from(size).map_err(|_| AppError::usage("mode mapping is too large"))?;
            // SAFETY: INVALID_HANDLE_VALUE requests a pagefile-backed mapping; the name is nul-terminated.
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
            // SAFETY: handle names the new read/write mapping and size is its exact allocation size.
            let raw = unsafe { MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, size) };
            let Some(view) = NonNull::new(raw.Value.cast::<SharedModeBlock>()) else {
                // SAFETY: ownership has not been transferred.
                let _ = unsafe { CloseHandle(handle) };
                return Err(AppError::evidence("MapViewOfFile returned a null view"));
            };
            // SAFETY: a new pagefile mapping is zero initialized and covers SharedModeBlock.
            unsafe {
                let block = view.as_ptr();
                write_volatile(&raw mut (*block).magic, MODE_IPC_MAGIC);
                write_volatile(&raw mut (*block).version, MODE_IPC_VERSION);
                write_volatile(&raw mut (*block).size, size_low);
                write_volatile(&raw mut (*block).target_pid, target_pid);
                write_volatile(&raw mut (*block).scenario, scenario as u32);
                write_volatile(&raw mut (*block).endpoint, endpoint as u32);
                write_volatile(&raw mut (*block).requested_mode, requested_mode);
                (*block).nonce = nonce;
                (*block).pairing_key = pairing_key;
            }
            Ok(Self { handle, view, name })
        }

        #[must_use]
        pub fn name(&self) -> &str {
            &self.name
        }

        pub fn state(&self) -> AppResult<NativeState> {
            // SAFETY: view stays mapped for self's lifetime; native uses an interlocked aligned write.
            NativeState::from_raw(unsafe { read_volatile(&raw const (*self.view.as_ptr()).state) })
        }

        #[must_use]
        pub fn error_code(&self) -> i32 {
            // SAFETY: view stays mapped for self's lifetime.
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).error_code) }
        }

        #[must_use]
        pub fn hooks_mask(&self) -> u32 {
            // SAFETY: view stays mapped for self's lifetime.
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).hooks_mask) }
        }

        #[must_use]
        pub fn overwritten_events(&self) -> u64 {
            // SAFETY: aligned field is read after native interlocked updates.
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).overwritten_events) }
        }

        #[must_use]
        pub fn local_peer_token(&self) -> u64 {
            // SAFETY: aligned field is written once after the Steam context becomes available.
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).local_peer_token) }
        }

        pub fn drain(&self, next_sequence: &mut u64) -> AppResult<Vec<NativeEventRecord>> {
            let block = self.view.as_ptr();
            // SAFETY: view stays mapped and next_sequence is a monotonic producer counter.
            let produced = unsafe { read_volatile(&raw const (*block).next_sequence) };
            if produced.saturating_sub(*next_sequence) > MODE_EVENT_CAPACITY as u64 {
                return Err(AppError::evidence(
                    "native event ring overflowed before the controller drained it",
                ));
            }
            let mut records = Vec::new();
            while *next_sequence < produced {
                let sequence = next_sequence
                    .checked_add(1)
                    .ok_or_else(|| AppError::evidence("native event sequence overflow"))?;
                let index = usize::try_from((sequence - 1) % MODE_EVENT_CAPACITY as u64)
                    .map_err(|_| AppError::evidence("native ring index overflow"))?;
                // SAFETY: index is bounded and NativeEvent is Copy. published_sequence is written last.
                let event = unsafe { read_volatile(&raw const (*block).events[index]) };
                if event.published_sequence != sequence as i64 {
                    break;
                }
                records.push(NativeEventRecord { sequence, event });
                *next_sequence = sequence;
            }
            // SAFETY: the controller is the only writer of this aligned acknowledgement field.
            unsafe {
                write_volatile(&raw mut (*block).controller_sequence, *next_sequence);
            }
            Ok(records)
        }
    }

    impl Drop for ModeMapping {
        fn drop(&mut self) {
            // SAFETY: both resources are uniquely owned by self.
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
pub use windows_mapping::ModeMapping;

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn ipc_layout_is_stable_and_aligned() {
        assert_eq!(std::mem::align_of::<NativeEvent>(), 8);
        assert_eq!(std::mem::align_of::<SharedModeBlock>(), 8);
        assert_eq!(std::mem::size_of::<NativeEvent>(), 168);
        assert_eq!(std::mem::size_of::<SharedModeBlock>(), 688_296);
    }

    #[test]
    fn pairing_key_parser_is_exact() {
        assert_eq!(parse_pairing_key(&"A5".repeat(32)).unwrap(), [0xA5; 32]);
        assert!(parse_pairing_key("00").is_err());
        assert!(parse_pairing_key(&"Z0".repeat(32)).is_err());
    }
}
