use crate::error::{AppError, AppResult};

pub const PASSIVE_IPC_MAGIC: u64 = 0x3156_5350_4F4F_4349;
pub const PASSIVE_IPC_VERSION: u32 = 1;
pub const DEVICE_PASSIVE_IPC_VERSION: u32 = 2;
pub const DEVICE_PASSIVE_SLOT_COUNT: u32 = 56;
pub const DEVICE_PASSIVE_COMPLETE_HOOK_MASK: u64 = 0x00FF_FFFF_FFFF_FFFF;
pub const PASSIVE_EVENT_CAPACITY: usize = 16_384;
pub const PASSIVE_SLOT_COUNT: u32 = 53;
pub const PASSIVE_COMPLETE_HOOK_MASK: u64 = 0x001F_FFFF_FFFF_FFFF;
pub const PASSIVE_NO_ORDINAL: u32 = u32::MAX;
pub const PASSIVE_DIGEST_SIZE: usize = 32;
pub const PASSIVE_LOOP_MAPPING_NAME_CAPACITY: usize = 96;

pub const PASSIVE_FLAG_MAIN_THREAD: u32 = 1 << 0;
pub const PASSIVE_FLAG_ORIGINAL_CALLED: u32 = 1 << 1;
pub const PASSIVE_FLAG_TOPOLOGY_COHERENT: u32 = 1 << 2;
pub const PASSIVE_FLAG_ENDPOINT_A: u32 = 1 << 3;
pub const PASSIVE_FLAG_ENDPOINT_B: u32 = 1 << 4;
pub const PASSIVE_FLAG_BINDING_UNIQUE: u32 = 1 << 5;
pub const PASSIVE_FLAG_HUD_MATCH: u32 = 1 << 6;
pub const PASSIVE_FLAG_ASSIGNMENT_MATCH: u32 = 1 << 7;
pub const PASSIVE_FLAG_LUA_VECTOR_EQUIVALENT: u32 = 1 << 8;
pub const PASSIVE_FLAG_RELATION_CHANGED: u32 = 1 << 9;
pub const PASSIVE_FLAG_STABLE_REGISTRY_RETIRED: u32 = 1 << 10;
pub const PASSIVE_FLAG_FAILURE: u32 = 1 << 11;
pub const PASSIVE_KNOWN_FLAGS: u32 = (1 << 12) - 1;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(i32)]
pub enum PassiveNativeState {
    Empty = 0,
    Initializing = 1,
    Ready = 2,
    Failed = 3,
    ShuttingDown = 4,
    Stopped = 5,
    Unsafe = 6,
}

impl PassiveNativeState {
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
                "coop passive IPC has unknown native state {value}"
            ))),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum PassiveScenario {
    NativeJoin = 1,
    NativeExit = 2,
    DeathRespawn = 3,
    RoomRebind = 4,
    DeviceRebind = 5,
    MenuTeardown = 6,
}

impl PassiveScenario {
    pub fn ipc_version(self) -> u32 {
        if self == Self::DeviceRebind {
            DEVICE_PASSIVE_IPC_VERSION
        } else {
            PASSIVE_IPC_VERSION
        }
    }

    pub fn slot_count(self) -> u32 {
        if self == Self::DeviceRebind {
            DEVICE_PASSIVE_SLOT_COUNT
        } else {
            PASSIVE_SLOT_COUNT
        }
    }

    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            1 => Ok(Self::NativeJoin),
            2 => Ok(Self::NativeExit),
            3 => Ok(Self::DeathRespawn),
            4 => Ok(Self::RoomRebind),
            5 => Ok(Self::DeviceRebind),
            6 => Ok(Self::MenuTeardown),
            _ => Err(AppError::evidence(format!(
                "coop passive IPC has unknown scenario {value}"
            ))),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum PassiveEventKind {
    NativeInitialized = 1,
    HooksInstalled = 2,
    ManagerPassBegin = 3,
    ManagerPassEnd = 4,
    OrdinarySpawnBegin = 5,
    OrdinarySpawnEnd = 6,
    PossessorSpawnStop = 7,
    ExitBegin = 8,
    ExitReturnStable = 9,
    OrdinaryAssignmentBegin = 10,
    OrdinaryAssignmentEnd = 11,
    PossessorAssignmentStop = 12,
    JoinHudBegin = 13,
    JoinHudEnd = 14,
    ExitHudBegin = 15,
    ExitHudEnd = 16,
    BindingUnbound = 17,
    BindingCandidate = 18,
    BindingBound = 19,
    BindingRetired = 20,
    LuaVectorEquivalent = 21,
    LifecycleEnter = 22,
    LifecycleExit = 23,
    RelationChange = 24,
    HooksRestored = 25,
    ProducerClosed = 26,
    Failure = 27,
    DeviceUnassignBegin = 28,
    DeviceUnassignEnd = 29,
    DeviceReassignBegin = 30,
    DeviceReassignEnd = 31,
    UnclassifiedRelationChange = 32,
}

impl PassiveEventKind {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            1 => Ok(Self::NativeInitialized),
            2 => Ok(Self::HooksInstalled),
            3 => Ok(Self::ManagerPassBegin),
            4 => Ok(Self::ManagerPassEnd),
            5 => Ok(Self::OrdinarySpawnBegin),
            6 => Ok(Self::OrdinarySpawnEnd),
            7 => Ok(Self::PossessorSpawnStop),
            8 => Ok(Self::ExitBegin),
            9 => Ok(Self::ExitReturnStable),
            10 => Ok(Self::OrdinaryAssignmentBegin),
            11 => Ok(Self::OrdinaryAssignmentEnd),
            12 => Ok(Self::PossessorAssignmentStop),
            13 => Ok(Self::JoinHudBegin),
            14 => Ok(Self::JoinHudEnd),
            15 => Ok(Self::ExitHudBegin),
            16 => Ok(Self::ExitHudEnd),
            17 => Ok(Self::BindingUnbound),
            18 => Ok(Self::BindingCandidate),
            19 => Ok(Self::BindingBound),
            20 => Ok(Self::BindingRetired),
            21 => Ok(Self::LuaVectorEquivalent),
            22 => Ok(Self::LifecycleEnter),
            23 => Ok(Self::LifecycleExit),
            24 => Ok(Self::RelationChange),
            25 => Ok(Self::HooksRestored),
            26 => Ok(Self::ProducerClosed),
            27 => Ok(Self::Failure),
            28 => Ok(Self::DeviceUnassignBegin),
            29 => Ok(Self::DeviceUnassignEnd),
            30 => Ok(Self::DeviceReassignBegin),
            31 => Ok(Self::DeviceReassignEnd),
            32 => Ok(Self::UnclassifiedRelationChange),
            _ => Err(AppError::evidence(format!(
                "coop passive IPC has unknown event kind {value}"
            ))),
        }
    }
}

#[derive(Clone, Copy, Debug)]
#[repr(C, align(8))]
pub struct NativePassiveEvent {
    pub published_sequence: i64,
    pub qpc: i64,
    pub loop_iteration: u64,
    pub owner_generation: u64,
    pub topology_digest: [u8; PASSIVE_DIGEST_SIZE],
    pub player_digest: [u8; PASSIVE_DIGEST_SIZE],
    pub device_digest: [u8; PASSIVE_DIGEST_SIZE],
    pub assignment_digest: [u8; PASSIVE_DIGEST_SIZE],
    pub thread_id: u32,
    pub main_thread_id: u32,
    pub kind: u32,
    pub slot_ordinal: u32,
    pub phase: u32,
    pub depth: u32,
    pub local_ordinal: u32,
    pub endpoint: u32,
    pub original_calls: u32,
    pub flags: u32,
}

#[repr(C, align(8))]
pub struct SharedPassiveBlock {
    pub magic: u64,
    pub version: u32,
    pub struct_size: u32,
    pub target_pid: u32,
    pub scenario: u32,
    pub process_creation_time_100ns: u64,
    pub nonce: [u8; 32],
    pub canonical_target_sha256: [u8; 32],
    pub observed_target_sha256: [u8; 32],
    pub target_equivalence_sha256: [u8; 32],
    pub catalog_sha256: [u8; 32],
    pub graph_sha256: [u8; 32],
    pub plan_sha256: [u8; 32],
    pub verification_sha256: [u8; 32],
    pub loop_mapping_name: [u16; PASSIVE_LOOP_MAPPING_NAME_CAPACITY],
    pub state: i32,
    pub error_code: i32,
    pub installed_hooks_mask: u64,
    pub restored_hooks_mask: u64,
    pub restored_slot_count: u32,
    pub main_thread_id: u32,
    pub owner_generation: u64,
    pub next_sequence: u64,
    pub controller_sequence: u64,
    pub overwritten_events: u64,
    pub active_hooks: i32,
    pub producer_closed: i32,
    pub publication_enabled: i32,
    pub stable_registry_retired: i32,
    pub override_count: u64,
    pub apply_count: u64,
    pub world_write_count: u64,
    pub scheduler_count: u64,
    pub transport_count: u64,
    pub mode_latch_write_count: u64,
    pub slot_snapshot_before_sha256: [u8; 32],
    pub slot_snapshot_after_sha256: [u8; 32],
    pub events: [NativePassiveEvent; PASSIVE_EVENT_CAPACITY],
}

#[derive(Clone, Debug)]
pub struct NativePassiveRecord {
    pub sequence: u64,
    pub event: NativePassiveEvent,
}

pub fn validate_passive_header(magic: u64, version: u32, struct_size: u32) -> AppResult<()> {
    if magic != PASSIVE_IPC_MAGIC
        || version != PASSIVE_IPC_VERSION
        || struct_size as usize != std::mem::size_of::<SharedPassiveBlock>()
    {
        return Err(AppError::evidence(
            "coop passive IPC header is not the exact v1 layout",
        ));
    }
    Ok(())
}

pub fn validate_passive_profile_header(
    magic: u64,
    version: u32,
    struct_size: u32,
    scenario: PassiveScenario,
) -> AppResult<()> {
    if magic != PASSIVE_IPC_MAGIC
        || version != scenario.ipc_version()
        || struct_size as usize != std::mem::size_of::<SharedPassiveBlock>()
    {
        return Err(AppError::evidence(
            "coop passive IPC header/profile mismatch",
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

    pub struct PassiveMapping {
        handle: HANDLE,
        view: NonNull<SharedPassiveBlock>,
        name: String,
        scenario: PassiveScenario,
        identity: PassiveMappingIdentity,
    }

    pub struct PassiveMappingIdentity {
        pub target_pid: u32,
        pub scenario: PassiveScenario,
        pub process_creation_time_100ns: u64,
        pub nonce: [u8; 32],
        pub canonical_target_sha256: [u8; 32],
        pub observed_target_sha256: [u8; 32],
        pub target_equivalence_sha256: [u8; 32],
        pub catalog_sha256: [u8; 32],
        pub graph_sha256: [u8; 32],
        pub plan_sha256: [u8; 32],
        pub verification_sha256: [u8; 32],
        pub loop_mapping_name: String,
    }

    impl PassiveMapping {
        pub fn create(name: String, identity: PassiveMappingIdentity) -> AppResult<Self> {
            if name.is_empty()
                || name.contains('\0')
                || identity.target_pid == 0
                || identity.process_creation_time_100ns == 0
                || identity.loop_mapping_name.is_empty()
                || identity.loop_mapping_name.contains('\0')
                || identity.nonce.iter().all(|value| *value == 0)
                || [
                    &identity.canonical_target_sha256,
                    &identity.observed_target_sha256,
                    &identity.target_equivalence_sha256,
                    &identity.catalog_sha256,
                    &identity.graph_sha256,
                    &identity.plan_sha256,
                    &identity.verification_sha256,
                ]
                .iter()
                .any(|digest| digest.iter().all(|value| *value == 0))
            {
                return Err(AppError::usage(
                    "coop passive mapping requires nonzero process and identity fields",
                ));
            }
            let loop_mapping_name = identity
                .loop_mapping_name
                .encode_utf16()
                .collect::<Vec<_>>();
            if loop_mapping_name.len() >= PASSIVE_LOOP_MAPPING_NAME_CAPACITY {
                return Err(AppError::usage(
                    "coop passive loop mapping name exceeds the frozen IPC capacity",
                ));
            }
            let wide = name
                .encode_utf16()
                .chain(std::iter::once(0))
                .collect::<Vec<_>>();
            let size = size_of::<SharedPassiveBlock>();
            let size_low = u32::try_from(size)
                .map_err(|_| AppError::usage("coop passive mapping is too large"))?;
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
                    "coop passive mapping identity is already in use",
                ));
            }
            let raw = unsafe { MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, size) };
            let Some(view) = NonNull::new(raw.Value.cast::<SharedPassiveBlock>()) else {
                let _ = unsafe { CloseHandle(handle) };
                return Err(AppError::evidence("MapViewOfFile returned a null view"));
            };
            unsafe {
                let block = view.as_ptr();
                write_volatile(&raw mut (*block).magic, PASSIVE_IPC_MAGIC);
                write_volatile(&raw mut (*block).version, identity.scenario.ipc_version());
                write_volatile(&raw mut (*block).struct_size, size_low);
                write_volatile(&raw mut (*block).target_pid, identity.target_pid);
                write_volatile(&raw mut (*block).scenario, identity.scenario as u32);
                write_volatile(
                    &raw mut (*block).process_creation_time_100ns,
                    identity.process_creation_time_100ns,
                );
                (*block).nonce = identity.nonce;
                (*block).canonical_target_sha256 = identity.canonical_target_sha256;
                (*block).observed_target_sha256 = identity.observed_target_sha256;
                (*block).target_equivalence_sha256 = identity.target_equivalence_sha256;
                (*block).catalog_sha256 = identity.catalog_sha256;
                (*block).graph_sha256 = identity.graph_sha256;
                (*block).plan_sha256 = identity.plan_sha256;
                (*block).verification_sha256 = identity.verification_sha256;
                (&mut (*block).loop_mapping_name)[..loop_mapping_name.len()]
                    .copy_from_slice(&loop_mapping_name);
            }
            Ok(Self {
                handle,
                view,
                name,
                scenario: identity.scenario,
                identity,
            })
        }

        pub fn name(&self) -> &str {
            &self.name
        }

        pub fn expected_hooks_mask(&self) -> u64 {
            (1_u64 << self.scenario.slot_count()) - 1
        }

        pub fn state(&self) -> AppResult<PassiveNativeState> {
            PassiveNativeState::from_raw(unsafe {
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

        pub fn owner_generation(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).owner_generation) }
        }

        pub fn overwritten_events(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).overwritten_events) }
        }

        pub fn active_hooks(&self) -> i32 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).active_hooks) }
        }

        pub fn producer_closed(&self) -> bool {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).producer_closed) != 0 }
        }

        pub fn stable_registry_retired(&self) -> bool {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).stable_registry_retired) != 0 }
        }

        pub fn zero_capability_counts(&self) -> [u64; 6] {
            unsafe {
                [
                    read_volatile(&raw const (*self.view.as_ptr()).override_count),
                    read_volatile(&raw const (*self.view.as_ptr()).apply_count),
                    read_volatile(&raw const (*self.view.as_ptr()).world_write_count),
                    read_volatile(&raw const (*self.view.as_ptr()).scheduler_count),
                    read_volatile(&raw const (*self.view.as_ptr()).transport_count),
                    read_volatile(&raw const (*self.view.as_ptr()).mode_latch_write_count),
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

        pub fn drain(&self, next: &mut u64) -> AppResult<Vec<NativePassiveRecord>> {
            validate_passive_profile_header(
                unsafe { read_volatile(&raw const (*self.view.as_ptr()).magic) },
                unsafe { read_volatile(&raw const (*self.view.as_ptr()).version) },
                unsafe { read_volatile(&raw const (*self.view.as_ptr()).struct_size) },
                self.scenario,
            )?;
            if unsafe { read_volatile(&raw const (*self.view.as_ptr()).scenario) }
                != self.scenario as u32
            {
                return Err(AppError::evidence("coop passive mapping scenario changed"));
            }
            self.state()?;
            if self.overwritten_events() != 0 {
                return Err(AppError::evidence(
                    "coop passive event ring reports overwritten records",
                ));
            }
            let unchanged_identity = unsafe {
                let block = self.view.as_ptr();
                read_volatile(&raw const (*block).target_pid) == self.identity.target_pid
                    && read_volatile(&raw const (*block).process_creation_time_100ns)
                        == self.identity.process_creation_time_100ns
                    && read_volatile(&raw const (*block).nonce) == self.identity.nonce
                    && read_volatile(&raw const (*block).canonical_target_sha256)
                        == self.identity.canonical_target_sha256
                    && read_volatile(&raw const (*block).observed_target_sha256)
                        == self.identity.observed_target_sha256
                    && read_volatile(&raw const (*block).target_equivalence_sha256)
                        == self.identity.target_equivalence_sha256
                    && read_volatile(&raw const (*block).catalog_sha256)
                        == self.identity.catalog_sha256
                    && read_volatile(&raw const (*block).graph_sha256) == self.identity.graph_sha256
                    && read_volatile(&raw const (*block).plan_sha256) == self.identity.plan_sha256
                    && read_volatile(&raw const (*block).verification_sha256)
                        == self.identity.verification_sha256
            };
            if !unchanged_identity {
                return Err(AppError::evidence("coop passive mapping identity changed"));
            }
            let produced = unsafe { read_volatile(&raw const (*self.view.as_ptr()).next_sequence) };
            if produced < *next {
                return Err(AppError::evidence(
                    "coop passive producer sequence regressed",
                ));
            }
            let mut expected_loop_name = [0_u16; PASSIVE_LOOP_MAPPING_NAME_CAPACITY];
            for (index, unit) in self.identity.loop_mapping_name.encode_utf16().enumerate() {
                expected_loop_name[index] = unit;
            }
            if unsafe { read_volatile(&raw const (*self.view.as_ptr()).loop_mapping_name) }
                != expected_loop_name
            {
                return Err(AppError::evidence(
                    "coop passive loop mapping identity changed",
                ));
            }
            if produced.saturating_sub(*next) > PASSIVE_EVENT_CAPACITY as u64 {
                return Err(AppError::evidence(
                    "coop passive event ring overwrote unread records",
                ));
            }
            let mut records = Vec::new();
            while *next < produced {
                let index = (*next as usize) % PASSIVE_EVENT_CAPACITY;
                let source = unsafe { &(*self.view.as_ptr()).events[index] };
                let published = unsafe { read_volatile(&raw const source.published_sequence) };
                if published != *next as i64 + 1 {
                    if self.producer_closed() || published > *next as i64 + 1 {
                        return Err(AppError::evidence("coop passive event publication gap"));
                    }
                    break;
                }
                fence(Ordering::Acquire);
                let event = unsafe { std::ptr::read_volatile(source) };
                fence(Ordering::Acquire);
                if unsafe { read_volatile(&raw const source.published_sequence) } != published {
                    return Err(AppError::evidence(
                        "coop passive record changed during read",
                    ));
                }
                if event.flags & !PASSIVE_KNOWN_FLAGS != 0 {
                    return Err(AppError::evidence(
                        "coop passive event contains unknown flags",
                    ));
                }
                PassiveEventKind::from_raw(event.kind)?;
                if self.scenario == PassiveScenario::DeviceRebind {
                    let endpoint_flags =
                        event.flags & (PASSIVE_FLAG_ENDPOINT_A | PASSIVE_FLAG_ENDPOINT_B);
                    let expected_endpoint_flags = match event.endpoint {
                        0 if event.local_ordinal == PASSIVE_NO_ORDINAL => 0,
                        1 if event.local_ordinal == 0 => PASSIVE_FLAG_ENDPOINT_A,
                        2 if event.local_ordinal == 1 => PASSIVE_FLAG_ENDPOINT_B,
                        _ => {
                            return Err(AppError::evidence(
                                "device passive endpoint/ordinal codebook mismatch",
                            ));
                        }
                    };
                    if endpoint_flags != expected_endpoint_flags
                        || event.original_calls > 1
                        || (event.flags & PASSIVE_FLAG_ORIGINAL_CALLED != 0)
                            != (event.original_calls == 1)
                    {
                        return Err(AppError::evidence(
                            "device passive flags disagree with event fields",
                        ));
                    }
                }
                if (self.scenario != PassiveScenario::DeviceRebind && event.kind > 27)
                    || (event.slot_ordinal != PASSIVE_NO_ORDINAL
                        && event.slot_ordinal >= self.scenario.slot_count())
                    || (matches!(event.kind, 28..=31)
                        && (event.phase != 10 || !matches!(event.slot_ordinal, 53..=55)))
                {
                    return Err(AppError::evidence(
                        "coop passive event/profile codebook mismatch",
                    ));
                }
                records.push(NativePassiveRecord {
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

    impl Drop for PassiveMapping {
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
pub use windows_mapping::{PassiveMapping, PassiveMappingIdentity};

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn passive_ipc_layout_and_codebooks_are_frozen() {
        assert_eq!(std::mem::size_of::<NativePassiveEvent>(), 200);
        assert_eq!(std::mem::align_of::<NativePassiveEvent>(), 8);
        assert_eq!(std::mem::size_of::<SharedPassiveBlock>(), 3_277_472);
        assert_eq!(std::mem::align_of::<SharedPassiveBlock>(), 8);
        assert_eq!(
            std::mem::offset_of!(SharedPassiveBlock, loop_mapping_name),
            288
        );
        assert_eq!(std::mem::offset_of!(SharedPassiveBlock, state), 480);
        assert_eq!(std::mem::offset_of!(SharedPassiveBlock, events), 672);
        assert_eq!(PASSIVE_COMPLETE_HOOK_MASK.count_ones(), PASSIVE_SLOT_COUNT);
        for raw in 1..=27 {
            assert!(PassiveEventKind::from_raw(raw).is_ok());
        }
        assert!(PassiveEventKind::from_raw(0).is_err());
        assert!(PassiveEventKind::from_raw(33).is_err());
    }

    #[test]
    fn passive_header_is_exact() {
        let size = std::mem::size_of::<SharedPassiveBlock>() as u32;
        assert!(validate_passive_header(PASSIVE_IPC_MAGIC, PASSIVE_IPC_VERSION, size).is_ok());
        assert!(validate_passive_header(0, PASSIVE_IPC_VERSION, size).is_err());
        assert!(validate_passive_header(PASSIVE_IPC_MAGIC, 2, size).is_err());
        assert!(validate_passive_header(PASSIVE_IPC_MAGIC, PASSIVE_IPC_VERSION, size - 1).is_err());
    }
}
