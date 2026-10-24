use crate::error::{AppError, AppResult};

pub const OFFLINE_ATTESTATION_IPC_MAGIC: u64 = 0x3156_3141_4F54_5349;
pub const OFFLINE_ATTESTATION_IPC_VERSION: u32 = 2;
pub const OFFLINE_ATTESTATION_EVENT_CAPACITY: usize = 4096;
pub const OFFLINE_ATTESTATION_EVENT_SIZE: usize = 128;
pub const OFFLINE_ATTESTATION_ACTIVE_RUN_HOOK_MASK: u64 = 0x3f;
pub const OFFLINE_ATTESTATION_LOADING_MENU_HOOK_MASK: u64 = 0x7f;
pub const OFFLINE_ATTESTATION_ACTIVE_RUN_SLOT_COUNT: u32 = 6;
pub const OFFLINE_ATTESTATION_LOADING_MENU_SLOT_COUNT: u32 = 7;
pub const OFFLINE_ATTESTATION_MENU_RETURN_RVA: u32 = 0x0065_E68C;

pub const OFFLINE_FLAG_MAIN_THREAD: u32 = 1 << 0;
pub const OFFLINE_FLAG_ORIGINAL_CALLED: u32 = 1 << 1;
pub const OFFLINE_FLAG_ROOT_MATCH: u32 = 1 << 2;
pub const OFFLINE_FLAG_MANAGER_ACTIVITY: u32 = 1 << 3;
pub const OFFLINE_FLAG_STABLE_ROSTER: u32 = 1 << 4;
pub const OFFLINE_FLAG_CHANNEL_ZERO: u32 = 1 << 5;
pub const OFFLINE_FLAG_NONZERO_CHANNEL: u32 = 1 << 6;
pub const OFFLINE_FLAG_STEAM_ARMED: u32 = 1 << 7;
pub const OFFLINE_FLAG_TEARDOWN: u32 = 1 << 8;
pub const OFFLINE_FLAG_LOADING_MENU_INACTIVE: u32 = 1 << 9;
pub const OFFLINE_FLAG_EXACT_RETURN: u32 = 1 << 10;
pub const OFFLINE_FLAG_WINDOW_MAIN_THREAD: u32 = 1 << 11;
pub const OFFLINE_FLAG_RUN_INACTIVE_BEFORE: u32 = 1 << 12;
pub const OFFLINE_FLAG_RUN_INACTIVE_AFTER: u32 = 1 << 13;
pub const OFFLINE_FLAG_GAME_ROOT_STABLE: u32 = 1 << 14;
pub const OFFLINE_FLAG_MANAGER_NEGATIVE: u32 = 1 << 15;
pub const OFFLINE_FLAG_LUA_UNBOUND_MENU: u32 = 1 << 16;
pub const OFFLINE_KNOWN_FLAGS: u32 = (1 << 17) - 1;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum OfflineAttestationPolicy {
    ActiveRun = 0,
    LoadingMenuInactive = 1,
}

impl OfflineAttestationPolicy {
    pub fn for_scenario(scenario: &str) -> Self {
        if scenario == "loading" {
            Self::LoadingMenuInactive
        } else {
            Self::ActiveRun
        }
    }

    pub const fn expected_hook_mask(self) -> u64 {
        match self {
            Self::ActiveRun => OFFLINE_ATTESTATION_ACTIVE_RUN_HOOK_MASK,
            Self::LoadingMenuInactive => OFFLINE_ATTESTATION_LOADING_MENU_HOOK_MASK,
        }
    }

    pub const fn expected_slot_count(self) -> u32 {
        match self {
            Self::ActiveRun => OFFLINE_ATTESTATION_ACTIVE_RUN_SLOT_COUNT,
            Self::LoadingMenuInactive => OFFLINE_ATTESTATION_LOADING_MENU_SLOT_COUNT,
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(i32)]
pub enum OfflineAttestationNativeState {
    Empty = 0,
    Initializing = 1,
    Ready = 2,
    Failed = 3,
    ShuttingDown = 4,
    Stopped = 5,
    Unsafe = 6,
}

impl OfflineAttestationNativeState {
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
                "offline attestation IPC has unknown native state {value}"
            ))),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum OfflineAttestationEventKind {
    NativeInitialized = 1,
    HooksInstalled = 2,
    SteamObserverArmed = 3,
    SimulationCallback = 4,
    LuaClose = 5,
    SteamSend = 6,
    SteamAvailable = 7,
    SteamRead = 8,
    HooksRestored = 9,
    ProducerClosed = 10,
    Failure = 11,
    MenuHeartbeatPeek = 12,
    GenericLuaPcallDiagnostic = 13,
}

impl OfflineAttestationEventKind {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            1 => Ok(Self::NativeInitialized),
            2 => Ok(Self::HooksInstalled),
            3 => Ok(Self::SteamObserverArmed),
            4 => Ok(Self::SimulationCallback),
            5 => Ok(Self::LuaClose),
            6 => Ok(Self::SteamSend),
            7 => Ok(Self::SteamAvailable),
            8 => Ok(Self::SteamRead),
            9 => Ok(Self::HooksRestored),
            10 => Ok(Self::ProducerClosed),
            11 => Ok(Self::Failure),
            12 => Ok(Self::MenuHeartbeatPeek),
            13 => Ok(Self::GenericLuaPcallDiagnostic),
            _ => Err(AppError::evidence(format!(
                "offline attestation IPC has unknown event kind {value}"
            ))),
        }
    }
}

#[derive(Clone, Copy, Debug)]
#[repr(C, align(8))]
pub struct NativeOfflineAttestationEvent {
    pub published_sequence: i64,
    pub qpc: i64,
    pub callback_sequence: u64,
    pub generation: u64,
    pub thread_id: u32,
    pub kind: u32,
    pub flags: u32,
    pub error_code: u32,
    pub original_calls: u32,
    pub channel: i32,
    pub return_rva: u32,
    pub result_bits: u32,
    pub game_root_digest: [u8; 32],
    pub lua_generation_digest: [u8; 32],
}

#[repr(C, align(8))]
pub struct SharedOfflineAttestationBlock {
    pub magic: u64,
    pub version: u32,
    pub struct_size: u32,
    pub target_pid: u32,
    pub reserved0: u32,
    pub expected_window_thread_id: u32,
    pub reserved1: u32,
    pub process_creation_time_100ns: u64,
    pub nonce: [u8; 32],
    pub canonical_target_sha256: [u8; 32],
    pub observed_target_sha256: [u8; 32],
    pub target_equivalence_sha256: [u8; 32],
    pub state: i32,
    pub error_code: i32,
    pub installed_hooks_mask: u64,
    pub restored_hooks_mask: u64,
    pub restored_slot_count: u32,
    pub main_thread_id: u32,
    pub generation: u64,
    pub simulation_callbacks: u64,
    pub menu_heartbeats: u64,
    pub next_sequence: u64,
    pub controller_sequence: u64,
    pub overwritten_events: u64,
    pub active_hooks: i32,
    pub producer_closed: i32,
    pub steam_observer_armed: i32,
    pub teardown_observed: i32,
    pub manager_signal_count: u64,
    pub roster_signal_count: u64,
    pub channel_zero_count: u64,
    pub nonzero_channel_count: u64,
    pub slot_snapshot_before_sha256: [u8; 32],
    pub slot_snapshot_after_sha256: [u8; 32],
    pub events: [NativeOfflineAttestationEvent; OFFLINE_ATTESTATION_EVENT_CAPACITY],
}

#[derive(Clone, Debug)]
pub struct NativeOfflineAttestationRecord {
    pub sequence: u64,
    pub event: NativeOfflineAttestationEvent,
}

pub fn validate_offline_attestation_header(
    magic: u64,
    version: u32,
    struct_size: u32,
) -> AppResult<()> {
    if magic != OFFLINE_ATTESTATION_IPC_MAGIC
        || version != OFFLINE_ATTESTATION_IPC_VERSION
        || struct_size as usize != std::mem::size_of::<SharedOfflineAttestationBlock>()
    {
        return Err(AppError::evidence(
            "offline attestation IPC header is not the exact independent v2 layout",
        ));
    }
    Ok(())
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

    pub struct OfflineAttestationMapping {
        handle: HANDLE,
        view: NonNull<SharedOfflineAttestationBlock>,
        name: String,
    }

    pub struct OfflineAttestationMappingIdentity {
        pub target_pid: u32,
        pub process_creation_time_100ns: u64,
        pub policy: OfflineAttestationPolicy,
        pub expected_window_thread_id: u32,
        pub nonce: [u8; 32],
        pub canonical_target_sha256: [u8; 32],
        pub observed_target_sha256: [u8; 32],
        pub target_equivalence_sha256: [u8; 32],
    }

    impl OfflineAttestationMapping {
        pub fn create(
            name: String,
            identity: OfflineAttestationMappingIdentity,
        ) -> AppResult<Self> {
            let OfflineAttestationMappingIdentity {
                target_pid,
                process_creation_time_100ns,
                policy,
                expected_window_thread_id,
                nonce,
                canonical_target_sha256,
                observed_target_sha256,
                target_equivalence_sha256,
            } = identity;
            if name.is_empty()
                || name.contains('\0')
                || target_pid == 0
                || process_creation_time_100ns == 0
                || (policy == OfflineAttestationPolicy::LoadingMenuInactive
                    && expected_window_thread_id == 0)
                || (policy == OfflineAttestationPolicy::ActiveRun && expected_window_thread_id != 0)
                || nonce.iter().all(|value| *value == 0)
                || canonical_target_sha256.iter().all(|value| *value == 0)
                || observed_target_sha256.iter().all(|value| *value == 0)
                || target_equivalence_sha256.iter().all(|value| *value == 0)
            {
                return Err(AppError::usage(
                    "offline attestation mapping requires nonzero process and identity fields",
                ));
            }
            let wide = name
                .encode_utf16()
                .chain(std::iter::once(0))
                .collect::<Vec<_>>();
            let size = size_of::<SharedOfflineAttestationBlock>();
            let size_low = u32::try_from(size)
                .map_err(|_| AppError::usage("offline attestation mapping is too large"))?;
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
                    "offline attestation mapping identity is already in use",
                ));
            }
            let raw = unsafe { MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, size) };
            let Some(view) = NonNull::new(raw.Value.cast::<SharedOfflineAttestationBlock>()) else {
                let _ = unsafe { CloseHandle(handle) };
                return Err(AppError::evidence("MapViewOfFile returned a null view"));
            };
            unsafe {
                let block = view.as_ptr();
                write_volatile(&raw mut (*block).magic, OFFLINE_ATTESTATION_IPC_MAGIC);
                write_volatile(&raw mut (*block).version, OFFLINE_ATTESTATION_IPC_VERSION);
                write_volatile(&raw mut (*block).struct_size, size_low);
                write_volatile(&raw mut (*block).target_pid, target_pid);
                write_volatile(&raw mut (*block).reserved0, policy as u32);
                write_volatile(
                    &raw mut (*block).expected_window_thread_id,
                    expected_window_thread_id,
                );
                write_volatile(
                    &raw mut (*block).process_creation_time_100ns,
                    process_creation_time_100ns,
                );
                (*block).nonce = nonce;
                (*block).canonical_target_sha256 = canonical_target_sha256;
                (*block).observed_target_sha256 = observed_target_sha256;
                (*block).target_equivalence_sha256 = target_equivalence_sha256;
            }
            Ok(Self { handle, view, name })
        }

        pub fn name(&self) -> &str {
            &self.name
        }

        pub fn state(&self) -> AppResult<OfflineAttestationNativeState> {
            OfflineAttestationNativeState::from_raw(unsafe {
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

        pub fn restored_slot_count(&self) -> u32 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).restored_slot_count) }
        }

        pub fn main_thread_id(&self) -> u32 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).main_thread_id) }
        }

        pub fn simulation_callbacks(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).simulation_callbacks) }
        }

        pub fn menu_heartbeats(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).menu_heartbeats) }
        }

        pub fn generation(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).generation) }
        }

        pub fn overwritten_events(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).overwritten_events) }
        }

        pub fn producer_closed(&self) -> bool {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).producer_closed) != 0 }
        }

        pub fn steam_observer_armed(&self) -> bool {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).steam_observer_armed) != 0 }
        }

        pub fn teardown_observed(&self) -> bool {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).teardown_observed) != 0 }
        }

        pub fn signal_counts(&self) -> [u64; 4] {
            unsafe {
                [
                    read_volatile(&raw const (*self.view.as_ptr()).manager_signal_count),
                    read_volatile(&raw const (*self.view.as_ptr()).roster_signal_count),
                    read_volatile(&raw const (*self.view.as_ptr()).channel_zero_count),
                    read_volatile(&raw const (*self.view.as_ptr()).nonzero_channel_count),
                ]
            }
        }

        pub fn slot_snapshot_hashes(&self) -> ([u8; 32], [u8; 32]) {
            unsafe {
                (
                    std::ptr::read_volatile(
                        &raw const (*self.view.as_ptr()).slot_snapshot_before_sha256,
                    ),
                    std::ptr::read_volatile(
                        &raw const (*self.view.as_ptr()).slot_snapshot_after_sha256,
                    ),
                )
            }
        }

        pub fn drain(&self, next: &mut u64) -> AppResult<Vec<NativeOfflineAttestationRecord>> {
            let produced = unsafe { read_volatile(&raw const (*self.view.as_ptr()).next_sequence) };
            if produced.saturating_sub(*next) > OFFLINE_ATTESTATION_EVENT_CAPACITY as u64 {
                return Err(AppError::evidence(
                    "offline attestation event ring overwrote unread records",
                ));
            }
            let mut records = Vec::new();
            while *next < produced {
                let index = (*next as usize) % OFFLINE_ATTESTATION_EVENT_CAPACITY;
                let source = unsafe { &(*self.view.as_ptr()).events[index] };
                let published = unsafe { read_volatile(&raw const source.published_sequence) };
                if published != (*next as i64 + 1) {
                    break;
                }
                fence(Ordering::Acquire);
                let event = unsafe { std::ptr::read_volatile(source) };
                if event.flags & !OFFLINE_KNOWN_FLAGS != 0 {
                    return Err(AppError::evidence(
                        "offline attestation event contains unknown flags",
                    ));
                }
                OfflineAttestationEventKind::from_raw(event.kind)?;
                records.push(NativeOfflineAttestationRecord {
                    sequence: *next,
                    event,
                });
                *next += 1;
            }
            unsafe {
                write_volatile(&raw mut (*self.view.as_ptr()).controller_sequence, *next);
            }
            Ok(records)
        }
    }

    impl Drop for OfflineAttestationMapping {
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
pub use windows_mapping::{OfflineAttestationMapping, OfflineAttestationMappingIdentity};

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn offline_ipc_layout_and_event_are_frozen() {
        assert_eq!(std::mem::size_of::<NativeOfflineAttestationEvent>(), 128);
        assert_eq!(std::mem::align_of::<NativeOfflineAttestationEvent>(), 8);
        assert_eq!(
            std::mem::offset_of!(NativeOfflineAttestationEvent, return_rva),
            56
        );
        assert_eq!(
            std::mem::offset_of!(NativeOfflineAttestationEvent, game_root_digest),
            64
        );
        assert_eq!(
            std::mem::offset_of!(SharedOfflineAttestationBlock, reserved0),
            20
        );
        assert_eq!(
            std::mem::offset_of!(SharedOfflineAttestationBlock, expected_window_thread_id),
            24
        );
        assert_eq!(
            std::mem::offset_of!(SharedOfflineAttestationBlock, events),
            360
        );
        assert_eq!(
            std::mem::size_of::<SharedOfflineAttestationBlock>(),
            360 + OFFLINE_ATTESTATION_EVENT_CAPACITY * OFFLINE_ATTESTATION_EVENT_SIZE
        );
    }

    #[test]
    fn offline_ipc_rejects_unknown_state_event_and_header() {
        assert!(OfflineAttestationNativeState::from_raw(99).is_err());
        assert!(OfflineAttestationEventKind::from_raw(99).is_err());
        assert!(validate_offline_attestation_header(0, 1, 1).is_err());
        validate_offline_attestation_header(
            OFFLINE_ATTESTATION_IPC_MAGIC,
            OFFLINE_ATTESTATION_IPC_VERSION,
            std::mem::size_of::<SharedOfflineAttestationBlock>() as u32,
        )
        .unwrap();
    }

    #[test]
    fn offline_policy_selects_exact_slot_contract() {
        assert_eq!(
            OfflineAttestationPolicy::ActiveRun.expected_hook_mask(),
            OFFLINE_ATTESTATION_ACTIVE_RUN_HOOK_MASK
        );
        assert_eq!(
            OfflineAttestationPolicy::ActiveRun.expected_slot_count(),
            OFFLINE_ATTESTATION_ACTIVE_RUN_SLOT_COUNT
        );
        assert_eq!(
            OfflineAttestationPolicy::LoadingMenuInactive.expected_hook_mask(),
            OFFLINE_ATTESTATION_LOADING_MENU_HOOK_MASK
        );
        assert_eq!(
            OfflineAttestationPolicy::LoadingMenuInactive.expected_slot_count(),
            OFFLINE_ATTESTATION_LOADING_MENU_SLOT_COUNT
        );
        assert_eq!(OFFLINE_KNOWN_FLAGS, 0x0001_FFFF);
    }
}
