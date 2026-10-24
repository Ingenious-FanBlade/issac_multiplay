use crate::error::{AppError, AppResult};

pub const ACTIVE_IPC_MAGIC: u64 = 0x3156_3430_4953_4349;
pub const ACTIVE_IPC_SCHEMA_VERSION: u32 = 4;
pub const ACTIVE_EVENT_CAPACITY: usize = 65_536;
pub const ACTIVE_LOOP_MAPPING_NAME_CAPACITY: usize = 96;
pub const ACTIVE_SESSION_ID_SIZE: usize = 16;
pub const ACTIVE_CANONICAL_SIZE: usize = 24;
pub const ACTIVE_EVENT_SIZE: usize = 288;
pub const ACTIVE_EVENT_ALIGNMENT: usize = 8;
pub const ACTIVE_EVENTS_OFFSET: usize = 720;
pub const ACTIVE_SHARED_BLOCK_SIZE: usize = 18_875_088;
pub const ACTIVE_HOOK_MASK: u64 = (1_u64 << 13) - 1;
pub const ACTIVE_SLOT_COUNT: u32 = 45;
pub const ACTIVE_NO_ACTION: u32 = u32::MAX;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum ActiveScenario {
    Keyboard = 1,
    Controller = 2,
    Simultaneous = 3,
    RapidTap = 4,
    DeviceSwitch = 5,
    Pause = 6,
    Loading = 7,
    Room = 8,
    Menu = 9,
    QueueFull = 10,
    Expired = 11,
    TooFar = 12,
    Stale = 13,
    Conflict = 14,
    Duplicate = 15,
    Loopback = 16,
}

impl ActiveScenario {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            1 => Ok(Self::Keyboard),
            2 => Ok(Self::Controller),
            3 => Ok(Self::Simultaneous),
            4 => Ok(Self::RapidTap),
            5 => Ok(Self::DeviceSwitch),
            6 => Ok(Self::Pause),
            7 => Ok(Self::Loading),
            8 => Ok(Self::Room),
            9 => Ok(Self::Menu),
            10 => Ok(Self::QueueFull),
            11 => Ok(Self::Expired),
            12 => Ok(Self::TooFar),
            13 => Ok(Self::Stale),
            14 => Ok(Self::Conflict),
            15 => Ok(Self::Duplicate),
            16 => Ok(Self::Loopback),
            _ => Err(AppError::evidence(format!(
                "active IPC has unknown scenario {value}"
            ))),
        }
    }

    pub fn parse(value: &str) -> AppResult<Self> {
        match value {
            "keyboard" => Ok(Self::Keyboard),
            "controller" => Ok(Self::Controller),
            "simultaneous" => Ok(Self::Simultaneous),
            "rapid-tap" => Ok(Self::RapidTap),
            "device-switch" => Ok(Self::DeviceSwitch),
            "pause" => Ok(Self::Pause),
            "loading" => Ok(Self::Loading),
            "room" => Ok(Self::Room),
            "menu" => Ok(Self::Menu),
            "queue-full" => Ok(Self::QueueFull),
            "expired" => Ok(Self::Expired),
            "too-far" => Ok(Self::TooFar),
            "stale" => Ok(Self::Stale),
            "conflict" => Ok(Self::Conflict),
            "duplicate" => Ok(Self::Duplicate),
            "loopback" => Ok(Self::Loopback),
            _ => Err(AppError::usage(format!(
                "unknown schema-4 active scenario {value}"
            ))),
        }
    }

    pub const fn as_str(self) -> &'static str {
        match self {
            Self::Keyboard => "keyboard",
            Self::Controller => "controller",
            Self::Simultaneous => "simultaneous",
            Self::RapidTap => "rapid-tap",
            Self::DeviceSwitch => "device-switch",
            Self::Pause => "pause",
            Self::Loading => "loading",
            Self::Room => "room",
            Self::Menu => "menu",
            Self::QueueFull => "queue-full",
            Self::Expired => "expired",
            Self::TooFar => "too-far",
            Self::Stale => "stale",
            Self::Conflict => "conflict",
            Self::Duplicate => "duplicate",
            Self::Loopback => "loopback",
        }
    }

    pub const fn uses_native_runtime(self) -> bool {
        true
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(i32)]
pub enum ActiveNativeState {
    Empty = 0,
    Initializing = 1,
    Ready = 2,
    ShuttingDown = 3,
    Stopped = 4,
    Failed = 5,
    Unsafe = 6,
}

impl ActiveNativeState {
    pub fn from_raw(value: i32) -> AppResult<Self> {
        match value {
            0 => Ok(Self::Empty),
            1 => Ok(Self::Initializing),
            2 => Ok(Self::Ready),
            3 => Ok(Self::ShuttingDown),
            4 => Ok(Self::Stopped),
            5 => Ok(Self::Failed),
            6 => Ok(Self::Unsafe),
            _ => Err(AppError::evidence(format!(
                "active IPC has unknown state {value}"
            ))),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum ActiveEventKind {
    Initialized = 1,
    HooksInstalled = 2,
    PhysicalSample = 3,
    Canonical = 4,
    Send = 5,
    Receive = 6,
    Accept = 7,
    Reject = 8,
    Duplicate = 9,
    Resolve = 10,
    ApplyCommit = 11,
    Suppression = 12,
    QueryOverride = 13,
    Witness = 14,
    OwnerSwitch = 15,
    LifecycleClear = 16,
    ClientRetired = 17,
    HooksRestored = 18,
    ProducerClosed = 19,
    Failure = 20,
}

impl ActiveEventKind {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            1 => Ok(Self::Initialized),
            2 => Ok(Self::HooksInstalled),
            3 => Ok(Self::PhysicalSample),
            4 => Ok(Self::Canonical),
            5 => Ok(Self::Send),
            6 => Ok(Self::Receive),
            7 => Ok(Self::Accept),
            8 => Ok(Self::Reject),
            9 => Ok(Self::Duplicate),
            10 => Ok(Self::Resolve),
            11 => Ok(Self::ApplyCommit),
            12 => Ok(Self::Suppression),
            13 => Ok(Self::QueryOverride),
            14 => Ok(Self::Witness),
            15 => Ok(Self::OwnerSwitch),
            16 => Ok(Self::LifecycleClear),
            17 => Ok(Self::ClientRetired),
            18 => Ok(Self::HooksRestored),
            19 => Ok(Self::ProducerClosed),
            20 => Ok(Self::Failure),
            _ => Err(AppError::evidence(format!(
                "active IPC has unknown event kind {value}"
            ))),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum ActivePhase {
    OutsideAuthority = 0,
    PhysicalSample = 1,
    DrainBeforeResolve = 2,
    Resolve = 3,
    Apply = 4,
    InputQuery = 5,
    Witness = 6,
}

impl ActivePhase {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            0 => Ok(Self::OutsideAuthority),
            1 => Ok(Self::PhysicalSample),
            2 => Ok(Self::DrainBeforeResolve),
            3 => Ok(Self::Resolve),
            4 => Ok(Self::Apply),
            5 => Ok(Self::InputQuery),
            6 => Ok(Self::Witness),
            _ => Err(AppError::evidence(format!(
                "active IPC has unknown phase {value}"
            ))),
        }
    }

    pub const fn as_str(self) -> &'static str {
        match self {
            Self::OutsideAuthority => "OUTSIDE_AUTHORITY",
            Self::PhysicalSample => "PHYSICAL_SAMPLE",
            Self::DrainBeforeResolve => "DRAIN_BEFORE_RESOLVE",
            Self::Resolve => "RESOLVE",
            Self::Apply => "APPLY",
            Self::InputQuery => "INPUT_QUERY",
            Self::Witness => "WITNESS",
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum ActiveDecision {
    None = 0,
    Accepted = 1,
    Rejected = 2,
    Duplicate = 3,
}

impl ActiveDecision {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            0 => Ok(Self::None),
            1 => Ok(Self::Accepted),
            2 => Ok(Self::Rejected),
            3 => Ok(Self::Duplicate),
            _ => Err(AppError::evidence(format!(
                "active IPC has unknown decision {value}"
            ))),
        }
    }

    pub const fn as_str(self) -> Option<&'static str> {
        match self {
            Self::None => None,
            Self::Accepted => Some("ACCEPTED"),
            Self::Rejected => Some("REJECTED"),
            Self::Duplicate => Some("DUPLICATE"),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum ActiveRejectReason {
    None = 0,
    QueueFull = 1,
    Expired = 2,
    TooFar = 3,
    StaleSequence = 4,
    IdentityConflict = 5,
}

impl ActiveRejectReason {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            0 => Ok(Self::None),
            1 => Ok(Self::QueueFull),
            2 => Ok(Self::Expired),
            3 => Ok(Self::TooFar),
            4 => Ok(Self::StaleSequence),
            5 => Ok(Self::IdentityConflict),
            _ => Err(AppError::evidence(format!(
                "active IPC has unknown reject reason {value}"
            ))),
        }
    }

    pub fn as_str(self) -> &'static str {
        match self {
            Self::None => "NONE",
            Self::QueueFull => "QUEUE_FULL",
            Self::Expired => "EXPIRED",
            Self::TooFar => "TOO_FAR",
            Self::StaleSequence => "STALE_SEQUENCE",
            Self::IdentityConflict => "IDENTITY_CONFLICT",
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum ActiveQueryKind {
    None = 0,
    Pressed = 1,
    Triggered = 2,
    Scalar = 3,
}

impl ActiveQueryKind {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            0 => Ok(Self::None),
            1 => Ok(Self::Pressed),
            2 => Ok(Self::Triggered),
            3 => Ok(Self::Scalar),
            _ => Err(AppError::evidence(format!(
                "active IPC has unknown query kind {value}"
            ))),
        }
    }

    pub const fn as_str(self) -> Option<&'static str> {
        match self {
            Self::None => None,
            Self::Pressed => Some("PRESSED"),
            Self::Triggered => Some("TRIGGERED"),
            Self::Scalar => Some("SCALAR"),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum ActiveConsumer {
    None = 0,
    Movement = 1,
    Bomb = 2,
    ItemSlot = 3,
    PillCard = 4,
    Shooting = 5,
    Drop = 6,
}

impl ActiveConsumer {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            0 => Ok(Self::None),
            1 => Ok(Self::Movement),
            2 => Ok(Self::Bomb),
            3 => Ok(Self::ItemSlot),
            4 => Ok(Self::PillCard),
            5 => Ok(Self::Shooting),
            6 => Ok(Self::Drop),
            _ => Err(AppError::evidence(format!(
                "active IPC has unknown consumer {value}"
            ))),
        }
    }

    pub const fn as_str(self) -> Option<&'static str> {
        match self {
            Self::None => None,
            Self::Movement => Some("MOVEMENT"),
            Self::Bomb => Some("BOMB"),
            Self::ItemSlot => Some("ITEM_SLOT"),
            Self::PillCard => Some("PILL_CARD"),
            Self::Shooting => Some("SHOOTING"),
            Self::Drop => Some("DROP"),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum ActiveLifecycleKind {
    None = 0,
    PauseEnter = 1,
    PauseExit = 2,
    LoadingEnter = 3,
    LoadingExit = 4,
    RoomEnter = 5,
    RoomExit = 6,
    OwnerSwitch = 7,
    RunTeardown = 8,
}

// For an `ACTIVE_EVENT_LIFECYCLE_CLEAR` record, `NativeActiveEvent::action`
// carries this codebook rather than an input action.

impl ActiveLifecycleKind {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            0 => Ok(Self::None),
            1 => Ok(Self::PauseEnter),
            2 => Ok(Self::PauseExit),
            3 => Ok(Self::LoadingEnter),
            4 => Ok(Self::LoadingExit),
            5 => Ok(Self::RoomEnter),
            6 => Ok(Self::RoomExit),
            7 => Ok(Self::OwnerSwitch),
            8 => Ok(Self::RunTeardown),
            _ => Err(AppError::evidence(format!(
                "active IPC has unknown lifecycle kind {value}"
            ))),
        }
    }

    pub const fn as_str(self) -> Option<&'static str> {
        match self {
            Self::None => None,
            Self::PauseEnter => Some("PAUSE_ENTER"),
            Self::PauseExit => Some("PAUSE_EXIT"),
            Self::LoadingEnter => Some("LOADING_ENTER"),
            Self::LoadingExit => Some("LOADING_EXIT"),
            Self::RoomEnter => Some("ROOM_ENTER"),
            Self::RoomExit => Some("ROOM_EXIT"),
            Self::OwnerSwitch => Some("OWNER_SWITCH"),
            Self::RunTeardown => Some("RUN_TEARDOWN"),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum ActiveWitnessKind {
    None = 0,
    WorldState = 1,
    RejectedUnchanged = 2,
}

// For an `ACTIVE_EVENT_WITNESS` record, `NativeActiveEvent::query_kind`
// carries this codebook rather than an input query kind.

impl ActiveWitnessKind {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            0 => Ok(Self::None),
            1 => Ok(Self::WorldState),
            2 => Ok(Self::RejectedUnchanged),
            _ => Err(AppError::evidence(format!(
                "active IPC has unknown witness kind {value}"
            ))),
        }
    }

    pub const fn as_str(self) -> Option<&'static str> {
        match self {
            Self::None => None,
            Self::WorldState => Some("WORLD_STATE"),
            Self::RejectedUnchanged => Some("REJECTED_UNCHANGED"),
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(i32)]
pub enum ActiveErrorCode {
    None = 0,
    MappingInvalid = 1,
    TargetImageMismatch = 2,
    ConflictingModuleOrDebugger = 3,
    IdentityMismatch = 4,
    HookTargetMismatch = 5,
    HookInstallFailed = 6,
    RingOverflow = 7,
    UnsafeShutdown = 8,
    WrongThread = 9,
    CompanionUnsafe = 10,
    OwnerConflict = 11,
    ProtocolViolation = 12,
    DrainTimeout = 13,
    RestoreMismatch = 14,
    AuthorityOverflow = 15,
}

impl ActiveErrorCode {
    pub fn from_raw(value: i32) -> AppResult<Self> {
        match value {
            0 => Ok(Self::None),
            1 => Ok(Self::MappingInvalid),
            2 => Ok(Self::TargetImageMismatch),
            3 => Ok(Self::ConflictingModuleOrDebugger),
            4 => Ok(Self::IdentityMismatch),
            5 => Ok(Self::HookTargetMismatch),
            6 => Ok(Self::HookInstallFailed),
            7 => Ok(Self::RingOverflow),
            8 => Ok(Self::UnsafeShutdown),
            9 => Ok(Self::WrongThread),
            10 => Ok(Self::CompanionUnsafe),
            11 => Ok(Self::OwnerConflict),
            12 => Ok(Self::ProtocolViolation),
            13 => Ok(Self::DrainTimeout),
            14 => Ok(Self::RestoreMismatch),
            15 => Ok(Self::AuthorityOverflow),
            _ => Err(AppError::evidence(format!(
                "active IPC has unknown error code {value}"
            ))),
        }
    }

    pub const fn as_str(self) -> &'static str {
        match self {
            Self::None => "NONE",
            Self::MappingInvalid => "MAPPING_INVALID",
            Self::TargetImageMismatch => "TARGET_IMAGE_MISMATCH",
            Self::ConflictingModuleOrDebugger => "CONFLICTING_MODULE_OR_DEBUGGER",
            Self::IdentityMismatch => "IDENTITY_MISMATCH",
            Self::HookTargetMismatch => "HOOK_TARGET_MISMATCH",
            Self::HookInstallFailed => "HOOK_INSTALL_FAILED",
            Self::RingOverflow => "RING_OVERFLOW",
            Self::UnsafeShutdown => "UNSAFE_SHUTDOWN",
            Self::WrongThread => "WRONG_THREAD",
            Self::CompanionUnsafe => "COMPANION_UNSAFE",
            Self::OwnerConflict => "OWNER_CONFLICT",
            Self::ProtocolViolation => "PROTOCOL_VIOLATION",
            Self::DrainTimeout => "DRAIN_TIMEOUT",
            Self::RestoreMismatch => "RESTORE_MISMATCH",
            Self::AuthorityOverflow => "AUTHORITY_OVERFLOW",
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[repr(u32)]
pub enum ActiveSourceDevice {
    None = 0,
    Keyboard = 1,
    Gamepad = 2,
}

impl ActiveSourceDevice {
    pub fn from_raw(value: u32) -> AppResult<Self> {
        match value {
            0 => Ok(Self::None),
            1 => Ok(Self::Keyboard),
            2 => Ok(Self::Gamepad),
            _ => Err(AppError::evidence(format!(
                "active IPC has unknown source device {value}"
            ))),
        }
    }
}

pub const ACTIVE_FLAG_MAIN_THREAD: u32 = 1 << 0;
pub const ACTIVE_FLAG_STEADY_STANDARD: u32 = 1 << 1;
pub const ACTIVE_FLAG_ORIGINAL_CALLED: u32 = 1 << 2;
pub const ACTIVE_FLAG_QUERY_OVERRIDDEN: u32 = 1 << 3;
pub const ACTIVE_FLAG_RESULT_TRUE: u32 = 1 << 4;
pub const ACTIVE_FLAG_CLIENT_RETIRED: u32 = 1 << 5;
pub const ACTIVE_FLAG_WITNESS_VALID: u32 = 1 << 6;
pub const ACTIVE_FLAG_CONFLICTING_STATE: u32 = 1 << 7;
pub const ACTIVE_KNOWN_FLAGS: u32 = (1 << 8) - 1;

#[derive(Clone, Copy, Debug, Default)]
#[repr(C, align(8))]
pub struct NativeActiveEvent {
    pub published_sequence: i64,
    pub qpc: i64,
    pub authority_tick: u64,
    pub loop_iteration: u64,
    pub thread_id: u32,
    pub kind: u32,
    pub phase: u32,
    pub flags: u32,
    pub attempt: u32,
    pub source_device: u32,
    pub client_id: u64,
    pub command_sequence: u64,
    pub target_tick: u64,
    pub edge_flags: u32,
    pub decision: u32,
    pub reject_reason: u32,
    pub queue_depth: u32,
    pub previous_command_sequence: u64,
    pub apply_generation: u64,
    pub owner_generation: u64,
    pub owner_before: u32,
    pub owner_after: u32,
    pub action: u32,
    pub query_kind: u32,
    pub consumer: u32,
    pub original_calls: u32,
    pub original_result_bits: u32,
    pub effective_result_bits: u32,
    pub sample_component_count: u32,
    pub hook_site_rva: u32,
    pub return_rva: u32,
    pub error_code: u32,
    pub reserved0: u32,
    pub canonical_state: [u8; ACTIVE_CANONICAL_SIZE],
    pub conflicting_state: [u8; ACTIVE_CANONICAL_SIZE],
    pub witness_before: [u8; 32],
    pub witness_after: [u8; 32],
}

#[repr(C, align(8))]
pub struct SharedActiveBlock {
    pub magic: u64,
    pub schema_version: u32,
    pub struct_size: u32,
    pub target_pid: u32,
    pub scenario: u32,
    pub process_creation_time_100ns: u64,
    pub nonce: [u8; 32],
    pub target_sha256: [u8; 32],
    pub observed_target_sha256: [u8; 32],
    pub target_equivalence_sha256: [u8; 32],
    pub mode_gate_sha256: [u8; 32],
    pub loop_gate_sha256: [u8; 32],
    pub raw_prereq_sha256: [u8; 32],
    pub active_catalog_sha256: [u8; 32],
    pub active_plan_sha256: [u8; 32],
    pub input_script_sha256: [u8; 32],
    pub arm_token_sha256: [u8; 32],
    pub loop_mapping_name: [u16; ACTIVE_LOOP_MAPPING_NAME_CAPACITY],
    pub session_id: [u8; ACTIVE_SESSION_ID_SIZE],
    pub client_id: u64,
    pub state: i32,
    pub error_code: i32,
    pub installed_active_hooks_mask: u64,
    pub restored_active_hooks_mask: u64,
    pub restored_slot_count: u32,
    pub main_thread_id: u32,
    pub apply_generation: u64,
    pub authority_tick: u64,
    pub owner_generation: u64,
    pub next_sequence: u64,
    pub controller_sequence: u64,
    pub overwritten_events: u64,
    pub active_hooks: i32,
    pub producer_closed: i32,
    pub client_retired: i32,
    pub publication_enabled: i32,
    pub effective_state: [u8; ACTIVE_CANONICAL_SIZE],
    pub events: [NativeActiveEvent; ACTIVE_EVENT_CAPACITY],
}

#[derive(Clone, Debug)]
pub struct NativeActiveEventRecord {
    pub sequence: u64,
    pub event: NativeActiveEvent,
}

pub fn validate_active_block_header(
    magic: u64,
    schema_version: u32,
    struct_size: u32,
) -> AppResult<()> {
    if magic != ACTIVE_IPC_MAGIC
        || schema_version != ACTIVE_IPC_SCHEMA_VERSION
        || struct_size as usize != std::mem::size_of::<SharedActiveBlock>()
    {
        return Err(AppError::evidence(
            "active IPC header is not the exact independent ICSI04V1 schema-4 layout",
        ));
    }
    Ok(())
}

fn identities_nonzero(values: &[[u8; 32]]) -> bool {
    values
        .iter()
        .all(|value| value.iter().any(|byte| *byte != 0))
}

#[allow(clippy::too_many_arguments)]
fn mapping_identity_inputs_valid(
    process_creation_time_100ns: u64,
    nonce: [u8; 32],
    target_sha256: [u8; 32],
    observed_target_sha256: [u8; 32],
    target_equivalence_sha256: [u8; 32],
    mode_gate_sha256: [u8; 32],
    loop_gate_sha256: [u8; 32],
    raw_prereq_sha256: [u8; 32],
    active_catalog_sha256: [u8; 32],
    active_plan_sha256: [u8; 32],
    input_script_sha256: [u8; 32],
    arm_token_sha256: [u8; 32],
) -> bool {
    process_creation_time_100ns != 0
        && identities_nonzero(&[
            nonce,
            target_sha256,
            observed_target_sha256,
            target_equivalence_sha256,
            mode_gate_sha256,
            loop_gate_sha256,
            raw_prereq_sha256,
            active_catalog_sha256,
            active_plan_sha256,
            input_script_sha256,
            arm_token_sha256,
        ])
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

    pub struct ActiveMapping {
        handle: HANDLE,
        view: NonNull<SharedActiveBlock>,
        name: String,
    }

    impl ActiveMapping {
        #[allow(clippy::too_many_arguments)]
        pub fn create(
            name: String,
            target_pid: u32,
            scenario: u32,
            process_creation_time_100ns: u64,
            nonce: [u8; 32],
            target_sha256: [u8; 32],
            observed_target_sha256: [u8; 32],
            target_equivalence_sha256: [u8; 32],
            mode_gate_sha256: [u8; 32],
            loop_gate_sha256: [u8; 32],
            raw_prereq_sha256: [u8; 32],
            active_catalog_sha256: [u8; 32],
            active_plan_sha256: [u8; 32],
            input_script_sha256: [u8; 32],
            arm_token_sha256: [u8; 32],
            loop_mapping_name: &str,
            session_id: [u8; ACTIVE_SESSION_ID_SIZE],
            client_id: u64,
        ) -> AppResult<Self> {
            if name.is_empty()
                || target_pid == 0
                || session_id.iter().all(|byte| *byte == 0)
                || client_id == 0
                || loop_mapping_name.is_empty()
                || loop_mapping_name.contains('\0')
                || !mapping_identity_inputs_valid(
                    process_creation_time_100ns,
                    nonce,
                    target_sha256,
                    observed_target_sha256,
                    target_equivalence_sha256,
                    mode_gate_sha256,
                    loop_gate_sha256,
                    raw_prereq_sha256,
                    active_catalog_sha256,
                    active_plan_sha256,
                    input_script_sha256,
                    arm_token_sha256,
                )
            {
                return Err(AppError::usage(
                    "active mapping requires nonzero process, target, session, client, prerequisite, equivalence, gates, plan, script and arm identities",
                ));
            }
            let loop_wide = loop_mapping_name.encode_utf16().collect::<Vec<_>>();
            if loop_wide.len() >= ACTIVE_LOOP_MAPPING_NAME_CAPACITY {
                return Err(AppError::usage(
                    "active companion loop mapping name is too long",
                ));
            }
            let wide = name
                .encode_utf16()
                .chain(std::iter::once(0))
                .collect::<Vec<_>>();
            let size = size_of::<SharedActiveBlock>();
            let size_low =
                u32::try_from(size).map_err(|_| AppError::usage("active mapping is too large"))?;
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
                    "active mapping name already exists; create-new identity is required",
                ));
            }
            let raw = unsafe { MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, size) };
            let Some(view) = NonNull::new(raw.Value.cast::<SharedActiveBlock>()) else {
                let _ = unsafe { CloseHandle(handle) };
                return Err(AppError::evidence("MapViewOfFile returned null"));
            };
            unsafe {
                let block = view.as_ptr();
                write_volatile(&raw mut (*block).magic, ACTIVE_IPC_MAGIC);
                write_volatile(&raw mut (*block).schema_version, ACTIVE_IPC_SCHEMA_VERSION);
                write_volatile(&raw mut (*block).struct_size, size_low);
                write_volatile(&raw mut (*block).target_pid, target_pid);
                write_volatile(&raw mut (*block).scenario, scenario);
                write_volatile(
                    &raw mut (*block).process_creation_time_100ns,
                    process_creation_time_100ns,
                );
                (*block).nonce = nonce;
                (*block).target_sha256 = target_sha256;
                (*block).observed_target_sha256 = observed_target_sha256;
                (*block).target_equivalence_sha256 = target_equivalence_sha256;
                (*block).mode_gate_sha256 = mode_gate_sha256;
                (*block).loop_gate_sha256 = loop_gate_sha256;
                (*block).raw_prereq_sha256 = raw_prereq_sha256;
                (*block).active_catalog_sha256 = active_catalog_sha256;
                (*block).active_plan_sha256 = active_plan_sha256;
                (*block).input_script_sha256 = input_script_sha256;
                (*block).arm_token_sha256 = arm_token_sha256;
                (&mut (*block).loop_mapping_name)[..loop_wide.len()].copy_from_slice(&loop_wide);
                (*block).session_id = session_id;
                write_volatile(&raw mut (*block).client_id, client_id);
                write_volatile(&raw mut (*block).apply_generation, 1);
            }
            Ok(Self { handle, view, name })
        }

        pub fn name(&self) -> &str {
            &self.name
        }

        fn validate_header(&self) -> AppResult<()> {
            let block = self.view.as_ptr();
            validate_active_block_header(
                unsafe { read_volatile(&raw const (*block).magic) },
                unsafe { read_volatile(&raw const (*block).schema_version) },
                unsafe { read_volatile(&raw const (*block).struct_size) },
            )
        }

        pub fn state(&self) -> AppResult<ActiveNativeState> {
            self.validate_header()?;
            ActiveNativeState::from_raw(unsafe {
                read_volatile(&raw const (*self.view.as_ptr()).state)
            })
        }

        pub fn error_code(&self) -> i32 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).error_code) }
        }

        pub fn installed_hooks_mask(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).installed_active_hooks_mask) }
        }

        pub fn restored_hooks_mask(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).restored_active_hooks_mask) }
        }

        pub fn restored_slot_count(&self) -> u32 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).restored_slot_count) }
        }

        pub fn main_thread_id(&self) -> u32 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).main_thread_id) }
        }

        pub fn overwritten_events(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).overwritten_events) }
        }

        pub fn producer_closed(&self) -> bool {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).producer_closed) != 0 }
        }

        pub fn client_retired(&self) -> bool {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).client_retired) != 0 }
        }

        pub fn apply_generation(&self) -> u64 {
            unsafe { read_volatile(&raw const (*self.view.as_ptr()).apply_generation) }
        }

        pub fn effective_state_neutral(&self) -> bool {
            let state = unsafe { read_volatile(&raw const (*self.view.as_ptr()).effective_state) };
            state.iter().all(|byte| *byte == 0)
        }

        pub fn drain(&self, next_sequence: &mut u64) -> AppResult<Vec<NativeActiveEventRecord>> {
            self.validate_header()?;
            let block = self.view.as_ptr();
            let overwritten = self.overwritten_events();
            let produced = unsafe { read_volatile(&raw const (*block).next_sequence) };
            if overwritten != 0
                || produced.saturating_sub(*next_sequence) > ACTIVE_EVENT_CAPACITY as u64
            {
                return Err(AppError::evidence(
                    "active event ring overflowed before controller drain",
                ));
            }
            let mut records = Vec::new();
            while *next_sequence < produced {
                let sequence = next_sequence
                    .checked_add(1)
                    .ok_or_else(|| AppError::evidence("active event sequence overflow"))?;
                let index = usize::try_from((sequence - 1) % ACTIVE_EVENT_CAPACITY as u64)
                    .map_err(|_| AppError::evidence("active event index overflow"))?;
                let slot = unsafe { &raw const (*block).events[index] };
                let before = unsafe { read_volatile(&raw const (*slot).published_sequence) };
                if before != sequence as i64 {
                    break;
                }
                fence(Ordering::Acquire);
                let event = unsafe { read_volatile(slot) };
                fence(Ordering::Acquire);
                let after = unsafe { read_volatile(&raw const (*slot).published_sequence) };
                if before != after || event.published_sequence != after {
                    break;
                }
                records.push(NativeActiveEventRecord { sequence, event });
                *next_sequence = sequence;
            }
            unsafe {
                write_volatile(&raw mut (*block).controller_sequence, *next_sequence);
            }
            Ok(records)
        }
    }

    impl Drop for ActiveMapping {
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
        use std::sync::atomic::{AtomicU64, Ordering as AtomicOrdering};

        use super::*;

        static MAPPING_SEQUENCE: AtomicU64 = AtomicU64::new(1);

        fn mapping_with_loop_name(loop_mapping_name: &str) -> AppResult<ActiveMapping> {
            let serial = MAPPING_SEQUENCE.fetch_add(1, AtomicOrdering::Relaxed);
            let name = format!(
                "Local\\IsaacCsT0.Active.CrossLanguageTest.{}.{}",
                std::process::id(),
                serial
            );
            ActiveMapping::create(
                name,
                std::process::id(),
                ActiveScenario::Keyboard as u32,
                1,
                [1; 32],
                [2; 32],
                [3; 32],
                [4; 32],
                [5; 32],
                [6; 32],
                [7; 32],
                [8; 32],
                [9; 32],
                [10; 32],
                [11; 32],
                loop_mapping_name,
                [12; ACTIVE_SESSION_ID_SIZE],
                13,
            )
        }

        fn mapping() -> ActiveMapping {
            mapping_with_loop_name("Local\\IsaacCsT0.Loop.CrossLanguageTest").unwrap()
        }

        unsafe fn publish(mapping: &ActiveMapping, sequence: u64, event: NativeActiveEvent) {
            let block = mapping.view.as_ptr();
            let index = ((sequence - 1) % ACTIVE_EVENT_CAPACITY as u64) as usize;
            let slot = unsafe { &raw mut (*block).events[index] };
            unsafe {
                write_volatile(slot, event);
                fence(Ordering::Release);
                write_volatile(&raw mut (*slot).published_sequence, sequence as i64);
                write_volatile(&raw mut (*block).next_sequence, sequence);
            }
        }

        #[test]
        fn active_mapping_binds_process_and_all_fixed_target_identities() {
            let mapping = mapping();
            let block = unsafe { &*mapping.view.as_ptr() };
            assert_eq!(block.process_creation_time_100ns, 1);
            assert_eq!(block.nonce, [1; 32]);
            assert_eq!(block.target_sha256, [2; 32]);
            assert_eq!(block.observed_target_sha256, [3; 32]);
            assert_eq!(block.target_equivalence_sha256, [4; 32]);
            assert_eq!(block.mode_gate_sha256, [5; 32]);
            assert_eq!(block.loop_gate_sha256, [6; 32]);
            assert_eq!(block.raw_prereq_sha256, [7; 32]);
            assert_eq!(block.active_catalog_sha256, [8; 32]);
            assert_eq!(block.active_plan_sha256, [9; 32]);
            assert_eq!(block.input_script_sha256, [10; 32]);
            assert_eq!(block.arm_token_sha256, [11; 32]);
            assert_eq!(block.session_id, [12; ACTIVE_SESSION_ID_SIZE]);
            assert_eq!(block.client_id, 13);
        }

        #[test]
        fn active_loop_mapping_name_capacity_is_exact_and_not_truncated() {
            let maximum = "L".repeat(ACTIVE_LOOP_MAPPING_NAME_CAPACITY - 1);
            let mapping = mapping_with_loop_name(&maximum).unwrap();
            let block = unsafe { &*mapping.view.as_ptr() };
            assert_eq!(
                block.loop_mapping_name[ACTIVE_LOOP_MAPPING_NAME_CAPACITY - 1],
                0
            );

            let too_long = "L".repeat(ACTIVE_LOOP_MAPPING_NAME_CAPACITY);
            assert!(mapping_with_loop_name(&too_long).is_err());

            let production_max = "Local\\IsaacCsT0.Loop.v5.4294967295.0123456789ABCDEF";
            assert!(production_max.encode_utf16().count() < ACTIVE_LOOP_MAPPING_NAME_CAPACITY);
            assert!(mapping_with_loop_name(production_max).is_ok());
        }

        #[test]
        fn real_mapping_drains_sentinel_half_publish_and_overwrite_fail_closed() {
            let mapping = mapping();
            let mut sentinel = NativeActiveEvent {
                qpc: 11,
                authority_tick: 12,
                loop_iteration: 13,
                thread_id: 14,
                kind: ActiveEventKind::Canonical as u32,
                phase: ActivePhase::PhysicalSample as u32,
                flags: ACTIVE_FLAG_MAIN_THREAD,
                attempt: 15,
                source_device: ActiveSourceDevice::Keyboard as u32,
                client_id: 10,
                command_sequence: 16,
                target_tick: 17,
                edge_flags: 18,
                decision: ActiveDecision::Accepted as u32,
                queue_depth: 19,
                apply_generation: 20,
                owner_generation: 21,
                action: ACTIVE_NO_ACTION,
                canonical_state: [0xA5; ACTIVE_CANONICAL_SIZE],
                witness_before: [0xB6; 32],
                witness_after: [0xC7; 32],
                ..NativeActiveEvent::default()
            };
            unsafe { publish(&mapping, 1, sentinel) };
            let mut next = 0;
            let records = mapping.drain(&mut next).unwrap();
            assert_eq!(next, 1);
            assert_eq!(records.len(), 1);
            let event = records[0].event;
            assert_eq!(event.qpc, 11);
            assert_eq!(event.authority_tick, 12);
            assert_eq!(event.loop_iteration, 13);
            assert_eq!(event.thread_id, 14);
            assert_eq!(event.attempt, 15);
            assert_eq!(event.command_sequence, 16);
            assert_eq!(event.target_tick, 17);
            assert_eq!(event.edge_flags, 18);
            assert_eq!(event.queue_depth, 19);
            assert_eq!(event.apply_generation, 20);
            assert_eq!(event.owner_generation, 21);
            assert_eq!(event.canonical_state, [0xA5; ACTIVE_CANONICAL_SIZE]);
            assert_eq!(event.witness_before, [0xB6; 32]);
            assert_eq!(event.witness_after, [0xC7; 32]);

            sentinel.qpc = 22;
            let block = mapping.view.as_ptr();
            unsafe {
                let slot = &raw mut (*block).events[1];
                write_volatile(slot, sentinel);
                write_volatile(&raw mut (*slot).published_sequence, 0);
                write_volatile(&raw mut (*block).next_sequence, 2);
            }
            assert!(mapping.drain(&mut next).unwrap().is_empty());
            assert_eq!(next, 1);
            unsafe {
                let slot = &raw mut (*block).events[1];
                fence(Ordering::Release);
                write_volatile(&raw mut (*slot).published_sequence, 2);
            }
            let records = mapping.drain(&mut next).unwrap();
            assert_eq!(records.len(), 1);
            assert_eq!(records[0].event.qpc, 22);
            assert_eq!(next, 2);

            unsafe { write_volatile(&raw mut (*block).overwritten_events, 1) };
            assert!(mapping.drain(&mut next).is_err());
        }

        #[test]
        fn active_mapping_name_is_create_new() {
            let first = mapping();
            let duplicate = ActiveMapping::create(
                first.name().to_owned(),
                std::process::id(),
                ActiveScenario::Keyboard as u32,
                1,
                [1; 32],
                [2; 32],
                [3; 32],
                [4; 32],
                [5; 32],
                [6; 32],
                [7; 32],
                [8; 32],
                [9; 32],
                [10; 32],
                [11; 32],
                "Local\\IsaacCsT0.Loop.CrossLanguageTest",
                [12; ACTIVE_SESSION_ID_SIZE],
                13,
            );
            assert!(duplicate.is_err());
        }
    }
}

#[cfg(windows)]
pub use windows_mapping::ActiveMapping;

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn active_schema_four_layout_and_enums_are_fail_closed() {
        assert_eq!(ACTIVE_IPC_MAGIC.to_le_bytes(), *b"ICSI04V1");
        assert_eq!(ACTIVE_IPC_SCHEMA_VERSION, 4);
        assert_eq!(ACTIVE_HOOK_MASK, 0x1FFF);
        assert_eq!(
            std::mem::align_of::<NativeActiveEvent>(),
            ACTIVE_EVENT_ALIGNMENT
        );
        assert_eq!(std::mem::size_of::<NativeActiveEvent>(), ACTIVE_EVENT_SIZE);
        assert_eq!(
            std::mem::offset_of!(NativeActiveEvent, canonical_state),
            172
        );
        assert_eq!(
            std::mem::offset_of!(NativeActiveEvent, conflicting_state),
            196
        );
        assert_eq!(std::mem::offset_of!(NativeActiveEvent, witness_before), 220);
        assert_eq!(std::mem::offset_of!(NativeActiveEvent, witness_after), 252);
        assert_eq!(std::mem::align_of::<SharedActiveBlock>(), 8);
        assert_eq!(
            std::mem::offset_of!(SharedActiveBlock, process_creation_time_100ns),
            24
        );
        assert_eq!(std::mem::offset_of!(SharedActiveBlock, nonce), 32);
        assert_eq!(std::mem::offset_of!(SharedActiveBlock, target_sha256), 64);
        assert_eq!(
            std::mem::offset_of!(SharedActiveBlock, observed_target_sha256),
            96
        );
        assert_eq!(
            std::mem::offset_of!(SharedActiveBlock, target_equivalence_sha256),
            128
        );
        assert_eq!(
            std::mem::offset_of!(SharedActiveBlock, mode_gate_sha256),
            160
        );
        assert_eq!(
            std::mem::offset_of!(SharedActiveBlock, loop_gate_sha256),
            192
        );
        assert_eq!(
            std::mem::offset_of!(SharedActiveBlock, raw_prereq_sha256),
            224
        );
        assert_eq!(
            std::mem::offset_of!(SharedActiveBlock, active_catalog_sha256),
            256
        );
        assert_eq!(
            std::mem::offset_of!(SharedActiveBlock, active_plan_sha256),
            288
        );
        assert_eq!(
            std::mem::offset_of!(SharedActiveBlock, input_script_sha256),
            320
        );
        assert_eq!(
            std::mem::offset_of!(SharedActiveBlock, arm_token_sha256),
            352
        );
        assert_eq!(
            std::mem::offset_of!(SharedActiveBlock, loop_mapping_name),
            384
        );
        assert_eq!(std::mem::offset_of!(SharedActiveBlock, session_id), 576);
        assert_eq!(
            std::mem::offset_of!(SharedActiveBlock, events),
            ACTIVE_EVENTS_OFFSET
        );
        assert_eq!(
            std::mem::size_of::<SharedActiveBlock>(),
            ACTIVE_SHARED_BLOCK_SIZE
        );
        assert!(validate_active_block_header(ACTIVE_IPC_MAGIC, 3, 0).is_err());
        assert!(ActiveNativeState::from_raw(99).is_err());
        assert!(ActiveEventKind::from_raw(99).is_err());
        assert!(ActiveRejectReason::from_raw(99).is_err());
        assert!(ActiveSourceDevice::from_raw(99).is_err());
        assert!(ActiveScenario::from_raw(99).is_err());
        assert!(ActivePhase::from_raw(99).is_err());
        assert!(ActiveDecision::from_raw(99).is_err());
        assert!(ActiveQueryKind::from_raw(99).is_err());
        assert!(ActiveConsumer::from_raw(99).is_err());
        assert!(ActiveLifecycleKind::from_raw(99).is_err());
        assert!(ActiveWitnessKind::from_raw(99).is_err());
        assert!(ActiveErrorCode::from_raw(99).is_err());
    }

    #[test]
    fn active_numeric_codebook_is_exact_and_round_trips_scenarios() {
        let runtime_scenarios = crate::active_catalog::ACTIVE_SCENARIOS
            .iter()
            .copied()
            .filter(|name| !matches!(*name, "control-no-native" | "original-online-disarmed"));
        for (index, name) in runtime_scenarios.enumerate() {
            let scenario = ActiveScenario::parse(name).unwrap();
            assert_eq!(scenario as usize, index + 1);
            assert_eq!(scenario.as_str(), name);
            assert_eq!(ActiveScenario::from_raw(scenario as u32).unwrap(), scenario);
        }
        assert!(ActiveScenario::parse("control-no-native").is_err());
        assert!(ActiveScenario::parse("original-online-disarmed").is_err());
        assert!(ActiveScenario::Loopback.uses_native_runtime());
        assert_eq!(ActivePhase::Apply.as_str(), "APPLY");
        assert_eq!(ActiveDecision::Accepted.as_str(), Some("ACCEPTED"));
        assert_eq!(ActiveQueryKind::Scalar.as_str(), Some("SCALAR"));
        assert_eq!(ActiveConsumer::ItemSlot.as_str(), Some("ITEM_SLOT"));
        assert_eq!(
            ActiveLifecycleKind::RunTeardown.as_str(),
            Some("RUN_TEARDOWN")
        );
        assert_eq!(ActiveWitnessKind::WorldState.as_str(), Some("WORLD_STATE"));
        for raw in 1..=20 {
            assert_eq!(ActiveEventKind::from_raw(raw).unwrap() as u32, raw);
        }
        for raw in 0..=6 {
            assert_eq!(ActivePhase::from_raw(raw).unwrap() as u32, raw);
            assert_eq!(ActiveConsumer::from_raw(raw).unwrap() as u32, raw);
        }
        for raw in 0..=3 {
            assert_eq!(ActiveDecision::from_raw(raw).unwrap() as u32, raw);
            assert_eq!(ActiveQueryKind::from_raw(raw).unwrap() as u32, raw);
        }
        for raw in 0..=8 {
            assert_eq!(ActiveLifecycleKind::from_raw(raw).unwrap() as u32, raw);
        }
        for raw in 0..=2 {
            assert_eq!(ActiveWitnessKind::from_raw(raw).unwrap() as u32, raw);
            assert_eq!(ActiveSourceDevice::from_raw(raw).unwrap() as u32, raw);
        }
        for raw in 0..=15 {
            assert_eq!(ActiveErrorCode::from_raw(raw).unwrap() as i32, raw);
        }
        assert_eq!(ActiveErrorCode::UnsafeShutdown as i32, 8);
    }

    #[test]
    fn reject_reason_names_are_exact() {
        assert_eq!(ActiveRejectReason::QueueFull.as_str(), "QUEUE_FULL");
        assert_eq!(ActiveRejectReason::Expired.as_str(), "EXPIRED");
        assert_eq!(ActiveRejectReason::TooFar.as_str(), "TOO_FAR");
        assert_eq!(ActiveRejectReason::StaleSequence.as_str(), "STALE_SEQUENCE");
        assert_eq!(
            ActiveRejectReason::IdentityConflict.as_str(),
            "IDENTITY_CONFLICT"
        );
    }

    #[test]
    fn active_mapping_identity_inputs_fail_closed_on_zero_creation_or_digest() {
        let valid = |creation: u64, identities: [[u8; 32]; 11]| {
            mapping_identity_inputs_valid(
                creation,
                identities[0],
                identities[1],
                identities[2],
                identities[3],
                identities[4],
                identities[5],
                identities[6],
                identities[7],
                identities[8],
                identities[9],
                identities[10],
            )
        };
        let identities = [[1; 32]; 11];
        assert!(valid(1, identities));
        assert!(!valid(0, identities));
        for index in 0..identities.len() {
            let mut mutated = identities;
            mutated[index] = [0; 32];
            assert!(!valid(1, mutated), "zero digest at index {index} passed");
        }
    }
}
