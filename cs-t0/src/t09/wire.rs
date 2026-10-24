use std::ffi::c_void;
use std::net::Ipv4Addr;
use std::ptr::NonNull;
use std::sync::atomic::{fence, AtomicI32, AtomicI64, AtomicU32, AtomicU64, Ordering};
use std::sync::Arc;

use windows::core::PCWSTR;
use windows::Win32::Foundation::{
    CloseHandle, GetLastError, ERROR_ALREADY_EXISTS, HANDLE, INVALID_HANDLE_VALUE,
};
use windows::Win32::System::Memory::{
    CreateFileMappingW, MapViewOfFile, UnmapViewOfFile, FILE_MAP_ALL_ACCESS, MEMORY_MAPPED_VIEW_ADDRESS,
    PAGE_READWRITE,
};

use crate::error::{AppError, AppResult};
use crate::t09::control::Role;
use crate::t09::manifest::{Manifest, WireIdentity};

pub const WIRE_MAGIC: u32 = 0x4957_5439; // little-endian bytes "T9WI"
pub const WIRE_VERSION: u16 = 1;
pub const WIRE_HEADER_SIZE: usize = 320;
pub const WIRE_SLOT_SIZE: usize = 1248;
pub const WIRE_RING_CAPACITY: usize = 128;
pub const WIRE_MAPPING_SIZE: usize =
    WIRE_HEADER_SIZE + 2 * WIRE_RING_CAPACITY * WIRE_SLOT_SIZE;

pub const OFF_MAGIC: usize = 0;
pub const OFF_VERSION: usize = 4;
pub const OFF_HEADER_SIZE: usize = 6;
pub const OFF_MAPPING_SIZE: usize = 8;
pub const OFF_ROLE: usize = 12;
pub const OFF_RUN_NONCE: usize = 16;
pub const OFF_SESSION: usize = 32;
pub const OFF_AUTHORITY_NONCE: usize = 48;
pub const OFF_REMOTE_NONCE: usize = 64;
pub const OFF_CLIENTS: usize = 80;
pub const OFF_BUILD: usize = 96;
pub const OFF_INPUT_SCHEMA: usize = 128;
pub const OFF_STATE_SCHEMA: usize = 130;
pub const OFF_WIRE_VERSION: usize = 132;
pub const OFF_IDENTITY_RESERVED: usize = 134;
pub const OFF_RUN_STATE: usize = 136;
pub const OFF_DRIVER_STATE: usize = 140;
pub const OFF_DRIVER_ERROR: usize = 144;
pub const OFF_CONTROLLER_ERROR: usize = 148;
pub const OFF_PRODUCED: usize = 152;
pub const OFF_CONSUMED: usize = 168;
pub const OFF_OVERWRITTEN: usize = 184;
pub const OFF_HEARTBEAT_TICK: usize = 200;
pub const OFF_RUN_START_TICK: usize = 208;
pub const OFF_STOP_TICK: usize = 216;
pub const OFF_LOCAL_GENERATION: usize = 224;
pub const OFF_TARGET_TICK: usize = 232;
pub const OFF_DRIVER_PID: usize = 240;
pub const OFF_DRIVER_FLAGS: usize = 244;
pub const OFF_DRIVER_HEARTBEAT_QPC: usize = 248;
pub const OFF_CLOSED: usize = 256;
pub const OFF_CONTROLLER_STATE: usize = 260;
pub const OFF_CONTROLLER_HEARTBEAT_TICK: usize = 264;
pub const OFF_RESERVED: usize = 272;

pub const SLOT_OFF_PUBLISHED: usize = 0;
pub const SLOT_OFF_TOKEN: usize = 8;
pub const SLOT_OFF_GENERATION: usize = 16;
pub const SLOT_OFF_TICK: usize = 24;
pub const SLOT_OFF_DIRECTION: usize = 32;
pub const SLOT_OFF_LENGTH: usize = 36;
pub const SLOT_OFF_PACKET: usize = 40;
pub const SLOT_OFF_RESERVED: usize = 1240;

pub const RUN_CREATED: u32 = 0;
pub const RUN_BOOTSTRAP: u32 = 1;
pub const RUN_RUNNING: u32 = 2;
pub const RUN_STOP_REQUESTED: u32 = 3;
pub const RUN_STOPPED: u32 = 4;

pub const DRIVER_OPEN: u32 = 1;
pub const DRIVER_HANDSHAKE: u32 = 2;
pub const DRIVER_ESTABLISHED: u32 = 4;
pub const DRIVER_READY: u32 = 8;
pub const DRIVER_RUNNING: u32 = 16;
pub const DRIVER_STOPPED: u32 = 32;
pub const DRIVER_ERROR: u32 = 64;

pub const DRIVER_FLAG_TARGET_REACHED: u32 = 1;
pub const DRIVER_FLAG_SCRIPT_COMPLETE: u32 = 2;
pub const DRIVER_FLAG_RESTORE_COMPLETE: u32 = 4;

pub const CONTROLLER_MAPPING_CREATED: u32 = 1;
pub const CONTROLLER_UDP_READY: u32 = 2;
pub const CONTROLLER_DRIVER_SPAWNED: u32 = 4;
pub const CONTROLLER_READY_SENT: u32 = 8;
pub const CONTROLLER_ARMED_SENT: u32 = 16;
pub const CONTROLLER_STOP_RECEIVED: u32 = 32;
pub const CONTROLLER_STOPPED: u32 = 64;
pub const CONTROLLER_ERROR: u32 = 128;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Direction {
    ToAuthority = 0,
    ToRemote = 1,
}

impl Direction {
    #[must_use]
    pub const fn index(self) -> usize {
        self as usize
    }

    #[must_use]
    pub const fn as_u32(self) -> u32 {
        self as u32
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum WriterKind {
    Driver,
    Controller,
}

#[derive(Clone, Debug, Default)]
pub struct WireStatus {
    pub run_state: u32,
    pub driver_state: u32,
    pub driver_error: u32,
    pub controller_error: u32,
    pub produced: [u64; 2],
    pub consumed: [u64; 2],
    pub overwritten: [u64; 2],
    pub heartbeat_tick: u64,
    pub run_start_tick: u64,
    pub stop_tick: u64,
    pub local_generation: u64,
    pub target_tick: u64,
    pub driver_pid: u32,
    pub driver_flags: u32,
    pub driver_heartbeat_qpc: u64,
    pub closed: u32,
    pub controller_state: u32,
    pub controller_heartbeat_tick: u64,
}

pub struct WireMapping {
    handle: HANDLE,
    view: NonNull<u8>,
}

// The mapping is intentionally shared by one controller thread and one driver
// process. All mutable coordination goes through the documented atomics and
// publication protocol; ordinary packet bytes are never concurrently written
// and read in the same slot.
unsafe impl Send for WireMapping {}
unsafe impl Sync for WireMapping {}

impl Drop for WireMapping {
    fn drop(&mut self) {
        // SAFETY: this view was returned by MapViewOfFile and is unmapped once.
        unsafe {
            let address = MEMORY_MAPPED_VIEW_ADDRESS {
                Value: self.view.as_ptr().cast::<c_void>(),
            };
            let _ = UnmapViewOfFile(address);
            let _ = CloseHandle(self.handle);
        }
    }
}

impl WireMapping {
    pub fn create(
        name: &str,
        role: Role,
        manifest: &Manifest,
    ) -> AppResult<Arc<Self>> {
        let identity = manifest.wire_identity()?;
        let wide: Vec<u16> = name.encode_utf16().chain(Some(0)).collect();
        // SAFETY: the name is NUL terminated and the mapping has the exact size
        // validated by the C++ and Rust layout constants.
        let handle = unsafe {
            CreateFileMappingW(
                INVALID_HANDLE_VALUE,
                None,
                PAGE_READWRITE,
                0,
                WIRE_MAPPING_SIZE as u32,
                PCWSTR(wide.as_ptr()),
            )
        }
        .map_err(|error| AppError::evidence(format!("create Wire IPC mapping: {error}")))?;
        // SAFETY: GetLastError only reads the calling thread's last error.
        if unsafe { GetLastError() } == ERROR_ALREADY_EXISTS {
            // SAFETY: handle is owned and no view exists yet.
            let _ = unsafe { CloseHandle(handle) };
            return Err(AppError::evidence(
                "Wire IPC mapping already exists; a previous run must not be reused",
            ));
        }
        let raw = unsafe { MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, WIRE_MAPPING_SIZE) };
        let Some(view) = NonNull::new(raw.Value.cast::<u8>()) else {
            // SAFETY: construction failed before ownership transfer.
            let _ = unsafe { CloseHandle(handle) };
            return Err(AppError::evidence("MapViewOfFile failed for Wire IPC mapping"));
        };
        let mapping = Arc::new(Self { handle, view });
        mapping.initialize(role, manifest, &identity)?;
        Ok(mapping)
    }

    pub fn open(
        name: &str,
        role: Role,
        manifest: &Manifest,
    ) -> AppResult<Arc<Self>> {
        let wide: Vec<u16> = name.encode_utf16().chain(Some(0)).collect();
        // SAFETY: the name is NUL terminated and maps only the declared size.
        let handle = unsafe {
            windows::Win32::System::Memory::OpenFileMappingW(
                FILE_MAP_ALL_ACCESS.0,
                false,
                PCWSTR(wide.as_ptr()),
            )
        }
        .map_err(|error| AppError::evidence(format!("open Wire IPC mapping: {error}")))?;
        let raw = unsafe { MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, WIRE_MAPPING_SIZE) };
        let Some(view) = NonNull::new(raw.Value.cast::<u8>()) else {
            // SAFETY: construction failed before ownership transfer.
            let _ = unsafe { CloseHandle(handle) };
            return Err(AppError::evidence("MapViewOfFile failed for Wire IPC mapping"));
        };
        let mapping = Arc::new(Self { handle, view });
        mapping.validate_open(role, manifest)?;
        Ok(mapping)
    }

    fn initialize(
        &self,
        role: Role,
        manifest: &Manifest,
        identity: &WireIdentity,
    ) -> AppResult<()> {
        let role_value = match role {
            Role::A => 1u32,
            Role::B => 2u32,
        };
        self.write_u32(OFF_MAGIC, WIRE_MAGIC);
        self.write_u16(OFF_VERSION, WIRE_VERSION);
        self.write_u16(OFF_HEADER_SIZE, WIRE_HEADER_SIZE as u16);
        self.write_u32(OFF_MAPPING_SIZE, WIRE_MAPPING_SIZE as u32);
        self.write_u32(OFF_ROLE, role_value);
        self.write_bytes(OFF_RUN_NONCE, &identity.run_nonce);
        self.write_bytes(OFF_SESSION, &identity.session);
        self.write_bytes(OFF_AUTHORITY_NONCE, &identity.authority_nonce);
        self.write_bytes(OFF_REMOTE_NONCE, &identity.remote_nonce);
        self.write_u64(OFF_CLIENTS, manifest.wire.clients[0]);
        self.write_u64(OFF_CLIENTS + 8, manifest.wire.clients[1]);
        self.write_bytes(OFF_BUILD, manifest.build_label.as_bytes());
        self.write_u16(OFF_INPUT_SCHEMA, 2);
        self.write_u16(OFF_STATE_SCHEMA, 1);
        self.write_u16(OFF_WIRE_VERSION, manifest.wire.version);
        self.write_u16(OFF_IDENTITY_RESERVED, 0);
        self.write_u32(OFF_RUN_STATE, RUN_CREATED);
        self.write_u32(OFF_DRIVER_STATE, 0);
        self.write_u32(OFF_DRIVER_ERROR, 0);
        self.write_u32(OFF_CONTROLLER_ERROR, 0);
        self.write_u64(OFF_HEARTBEAT_TICK, 0);
        self.write_u64(OFF_RUN_START_TICK, 0);
        self.write_u64(OFF_STOP_TICK, 0);
        self.write_u64(OFF_LOCAL_GENERATION, 1);
        self.write_u64(OFF_TARGET_TICK, manifest.script.last_tick);
        self.write_u32(OFF_DRIVER_PID, 0);
        self.write_u32(OFF_DRIVER_FLAGS, 0);
        self.write_u64(OFF_DRIVER_HEARTBEAT_QPC, 0);
        self.write_u32(OFF_CLOSED, 0);
        self.write_u32(OFF_CONTROLLER_STATE, CONTROLLER_MAPPING_CREATED);
        self.write_u64(OFF_CONTROLLER_HEARTBEAT_TICK, 0);
        for offset in (OFF_RESERVED..OFF_RESERVED + 48).step_by(4) {
            self.write_u32(offset, 0);
        }
        for direction in [Direction::ToAuthority, Direction::ToRemote] {
            self.produced(direction).store(0, Ordering::Release);
            self.consumed(direction).store(0, Ordering::Release);
            self.overwritten(direction).store(0, Ordering::Release);
        }
        fence(Ordering::Release);
        Ok(())
    }

    fn validate_open(&self, role: Role, manifest: &Manifest) -> AppResult<()> {
        let identity = manifest.wire_identity()?;
        let expected_role = match role {
            Role::A => 1u32,
            Role::B => 2u32,
        };
        if self.read_u32(OFF_MAGIC) != WIRE_MAGIC
            || self.read_u16(OFF_VERSION) != WIRE_VERSION
            || usize::from(self.read_u16(OFF_HEADER_SIZE)) != WIRE_HEADER_SIZE
            || self.read_u32(OFF_MAPPING_SIZE) as usize != WIRE_MAPPING_SIZE
            || self.read_u32(OFF_ROLE) != expected_role
        {
            return Err(AppError::evidence("Wire IPC header magic/version/size/role mismatch"));
        }
        if self.read_bytes(OFF_RUN_NONCE, 16) != identity.run_nonce
            || self.read_bytes(OFF_SESSION, 16) != identity.session
            || self.read_bytes(OFF_AUTHORITY_NONCE, 16) != identity.authority_nonce
            || self.read_bytes(OFF_REMOTE_NONCE, 16) != identity.remote_nonce
        {
            return Err(AppError::evidence("Wire IPC identity/session/nonce mismatch"));
        }
        if self.read_u64(OFF_CLIENTS) != manifest.wire.clients[0]
            || self.read_u64(OFF_CLIENTS + 8) != manifest.wire.clients[1]
        {
            return Err(AppError::evidence("Wire IPC client identity mismatch"));
        }
        let build = self.read_bytes(OFF_BUILD, 32);
        let expected_build = manifest.build_label.as_bytes();
        if build.get(..expected_build.len()) != Some(expected_build)
            || build[expected_build.len()..].iter().any(|byte| *byte != 0)
            || self.read_u16(OFF_INPUT_SCHEMA) != 2
            || self.read_u16(OFF_STATE_SCHEMA) != 1
            || self.read_u16(OFF_WIRE_VERSION) != manifest.wire.version
            || self.read_u16(OFF_IDENTITY_RESERVED) != 0
        {
            return Err(AppError::evidence("Wire IPC schema/build identity mismatch"));
        }
        for offset in (OFF_RESERVED..OFF_RESERVED + 48).step_by(4) {
            if self.read_u32(offset) != 0 {
                return Err(AppError::evidence("Wire IPC reserved header fields are nonzero"));
            }
        }
        Ok(())
    }

    #[must_use]
    pub fn outbound_direction(role: Role) -> Direction {
        match role {
            Role::A => Direction::ToRemote,
            Role::B => Direction::ToAuthority,
        }
    }

    #[must_use]
    pub fn inbound_direction(role: Role) -> Direction {
        match role {
            Role::A => Direction::ToAuthority,
            Role::B => Direction::ToRemote,
        }
    }

    pub fn write_packet(
        &self,
        role: Role,
        writer: WriterKind,
        direction: Direction,
        packet: &[u8],
        generation: u64,
        tick: u64,
    ) -> AppResult<()> {
        if self.closed() != 0 {
            return Err(AppError::evidence("Wire IPC is closed"));
        }
        let expected_direction = match writer {
            WriterKind::Driver => Self::outbound_direction(role),
            WriterKind::Controller => Self::inbound_direction(role),
        };
        if direction != expected_direction {
            return Err(AppError::evidence(
                "Wire IPC write direction does not belong to this writer/role",
            ));
        }
        if !(64..=1200).contains(&packet.len()) {
            return Err(AppError::evidence(format!(
                "Wire IPC packet length {} is outside 64..=1200",
                packet.len()
            )));
        }
        let produced = self.produced(direction).load(Ordering::Acquire);
        let consumed = self.consumed(direction).load(Ordering::Acquire);
        if produced < consumed || produced - consumed >= WIRE_RING_CAPACITY as u64 {
            self.overwritten(direction).fetch_add(1, Ordering::AcqRel);
            self.fail(writer, "Wire IPC outbound ring is full or inconsistent");
            return Err(AppError::evidence("Wire IPC outbound ring is full"));
        }
        let token = produced
            .checked_add(1)
            .ok_or_else(|| AppError::evidence("Wire IPC token exhausted"))?;
        let index = ((token - 1) as usize) % WIRE_RING_CAPACITY;
        let slot = self.slot_address(direction, index);
        self.slot_published(slot).store(0, Ordering::Release);
        self.write_u64_at(slot + SLOT_OFF_TOKEN, token);
        self.write_u64_at(slot + SLOT_OFF_GENERATION, generation);
        self.write_u64_at(slot + SLOT_OFF_TICK, tick);
        self.write_u32_at(slot + SLOT_OFF_DIRECTION, direction.as_u32());
        self.write_u32_at(slot + SLOT_OFF_LENGTH, packet.len() as u32);
        self.write_bytes_at(slot + SLOT_OFF_PACKET, packet);
        self.write_bytes_at(slot + SLOT_OFF_RESERVED, &[0u8; 8]);
        fence(Ordering::Release);
        self.slot_published(slot).store(token as i64, Ordering::Release);
        self.produced(direction).store(token, Ordering::Release);
        Ok(())
    }

    pub fn read_packet(
        &self,
        role: Role,
        reader: WriterKind,
        direction: Direction,
    ) -> AppResult<Option<(Vec<u8>, u64, u64)>> {
        let expected_direction = match reader {
            WriterKind::Driver => Self::inbound_direction(role),
            WriterKind::Controller => Self::outbound_direction(role),
        };
        if direction != expected_direction {
            return Err(AppError::evidence(
                "Wire IPC read direction does not belong to this reader/role",
            ));
        }
        let produced = self.produced(direction).load(Ordering::Acquire);
        let consumed = self.consumed(direction).load(Ordering::Acquire);
        if consumed > produced || produced - consumed > WIRE_RING_CAPACITY as u64 {
            self.fail(reader, "Wire IPC inbound counters are inconsistent");
            return Err(AppError::evidence("Wire IPC inbound counters are inconsistent"));
        }
        if consumed == produced {
            return Ok(None);
        }
        let expected = consumed + 1;
        let index = (consumed as usize) % WIRE_RING_CAPACITY;
        let slot = self.slot_address(direction, index);
        let published = self.slot_published(slot).load(Ordering::Acquire);
        if published != expected as i64 {
            self.fail(
                reader,
                &format!(
                    "Wire IPC slot publication mismatch: expected {expected}, got {published}"
                ),
            );
            return Err(AppError::evidence(
                "Wire IPC slot publication mismatch or overwrite",
            ));
        }
        let token = self.read_u64_at(slot + SLOT_OFF_TOKEN);
        let generation = self.read_u64_at(slot + SLOT_OFF_GENERATION);
        let tick = self.read_u64_at(slot + SLOT_OFF_TICK);
        let slot_direction = self.read_u32_at(slot + SLOT_OFF_DIRECTION);
        let length = self.read_u32_at(slot + SLOT_OFF_LENGTH) as usize;
        let reserved = self.read_bytes_at(slot + SLOT_OFF_RESERVED, 8);
        if token != expected
            || slot_direction != direction.as_u32()
            || !(64..=1200).contains(&length)
            || reserved.iter().any(|byte| *byte != 0)
        {
            self.fail(reader, "Wire IPC slot metadata is invalid");
            return Err(AppError::evidence("Wire IPC slot metadata is invalid"));
        }
        let mut packet = vec![0u8; length];
        packet.copy_from_slice(&self.read_bytes_at(slot + SLOT_OFF_PACKET, length));
        fence(Ordering::Acquire);
        self.consumed(direction).store(expected, Ordering::Release);
        Ok(Some((packet, generation, tick)))
    }

    pub fn status(&self) -> WireStatus {
        WireStatus {
            run_state: self.atomic_u32(OFF_RUN_STATE).load(Ordering::Acquire),
            driver_state: self.atomic_u32(OFF_DRIVER_STATE).load(Ordering::Acquire),
            driver_error: self.atomic_u32(OFF_DRIVER_ERROR).load(Ordering::Acquire),
            controller_error: self.atomic_u32(OFF_CONTROLLER_ERROR).load(Ordering::Acquire),
            produced: [
                self.produced(Direction::ToAuthority).load(Ordering::Acquire),
                self.produced(Direction::ToRemote).load(Ordering::Acquire),
            ],
            consumed: [
                self.consumed(Direction::ToAuthority).load(Ordering::Acquire),
                self.consumed(Direction::ToRemote).load(Ordering::Acquire),
            ],
            overwritten: [
                self.overwritten(Direction::ToAuthority).load(Ordering::Acquire),
                self.overwritten(Direction::ToRemote).load(Ordering::Acquire),
            ],
            heartbeat_tick: self.atomic_u64(OFF_HEARTBEAT_TICK).load(Ordering::Acquire),
            run_start_tick: self.atomic_u64(OFF_RUN_START_TICK).load(Ordering::Acquire),
            stop_tick: self.atomic_u64(OFF_STOP_TICK).load(Ordering::Acquire),
            local_generation: self.atomic_u64(OFF_LOCAL_GENERATION).load(Ordering::Acquire),
            target_tick: self.atomic_u64(OFF_TARGET_TICK).load(Ordering::Acquire),
            driver_pid: self.atomic_u32(OFF_DRIVER_PID).load(Ordering::Acquire),
            driver_flags: self.atomic_u32(OFF_DRIVER_FLAGS).load(Ordering::Acquire),
            driver_heartbeat_qpc: self
                .atomic_u64(OFF_DRIVER_HEARTBEAT_QPC)
                .load(Ordering::Acquire),
            closed: self.atomic_u32(OFF_CLOSED).load(Ordering::Acquire),
            controller_state: self.atomic_u32(OFF_CONTROLLER_STATE).load(Ordering::Acquire),
            controller_heartbeat_tick: self
                .atomic_u64(OFF_CONTROLLER_HEARTBEAT_TICK)
                .load(Ordering::Acquire),
        }
    }

    pub fn set_driver_pid(&self, pid: u32) {
        self.atomic_u32(OFF_DRIVER_PID).store(pid, Ordering::Release);
    }

    pub fn set_driver_state(&self, bits: u32) {
        self.atomic_u32(OFF_DRIVER_STATE)
            .fetch_or(bits, Ordering::AcqRel);
    }

    pub fn set_driver_flags(&self, bits: u32) {
        self.atomic_u32(OFF_DRIVER_FLAGS)
            .fetch_or(bits, Ordering::AcqRel);
    }

    pub fn set_driver_error(&self, code: u32) {
        self.atomic_u32(OFF_DRIVER_ERROR).store(code, Ordering::Release);
        self.set_driver_state(DRIVER_ERROR);
    }

    pub fn set_controller_state(&self, bits: u32) {
        self.atomic_u32(OFF_CONTROLLER_STATE)
            .fetch_or(bits, Ordering::AcqRel);
    }

    pub fn set_controller_error(&self, code: u32) {
        self.atomic_u32(OFF_CONTROLLER_ERROR)
            .store(code, Ordering::Release);
        self.set_controller_state(CONTROLLER_ERROR);
    }

    pub fn set_run_state(&self, state: u32) {
        self.atomic_u32(OFF_RUN_STATE).store(state, Ordering::Release);
    }

    pub fn set_run_start(&self, tick: u64) {
        self.atomic_u64(OFF_RUN_START_TICK)
            .store(tick, Ordering::Release);
        self.set_run_state(RUN_RUNNING);
    }

    pub fn request_stop(&self, tick: u64) {
        self.atomic_u64(OFF_STOP_TICK).store(tick, Ordering::Release);
        self.set_run_state(RUN_STOP_REQUESTED);
    }

    pub fn set_heartbeat_tick(&self, tick: u64) {
        self.atomic_u64(OFF_HEARTBEAT_TICK)
            .store(tick, Ordering::Release);
    }

    pub fn set_local_generation(&self, generation: u64) {
        self.atomic_u64(OFF_LOCAL_GENERATION)
            .store(generation, Ordering::Release);
    }

    pub fn set_driver_heartbeat_qpc(&self, qpc: u64) {
        self.atomic_u64(OFF_DRIVER_HEARTBEAT_QPC)
            .store(qpc, Ordering::Release);
    }

    pub fn set_controller_heartbeat_tick(&self, tick: u64) {
        self.atomic_u64(OFF_CONTROLLER_HEARTBEAT_TICK)
            .store(tick, Ordering::Release);
    }

    pub fn close(&self) {
        self.atomic_u32(OFF_CLOSED).store(1, Ordering::Release);
    }

    pub fn closed(&self) -> u32 {
        self.atomic_u32(OFF_CLOSED).load(Ordering::Acquire)
    }

    pub fn fail(&self, writer: WriterKind, message: &str) {
        self.close();
        match writer {
            WriterKind::Driver => self.set_driver_error(0xE001),
            WriterKind::Controller => self.set_controller_error(0xE002),
        }
        eprintln!("Wire IPC fail-closed ({writer:?}): {message}");
    }

    pub fn identity(&self) -> AppResult<WireIdentity> {
        Ok(WireIdentity {
            session: self.read_array::<16>(OFF_SESSION),
            run_nonce: self.read_array::<16>(OFF_RUN_NONCE),
            authority_nonce: self.read_array::<16>(OFF_AUTHORITY_NONCE),
            remote_nonce: self.read_array::<16>(OFF_REMOTE_NONCE),
        })
    }

    pub fn clients(&self) -> [u64; 2] {
        [
            self.read_u64(OFF_CLIENTS),
            self.read_u64(OFF_CLIENTS + 8),
        ]
    }

    pub fn build_label(&self) -> [u8; 32] {
        self.read_array::<32>(OFF_BUILD)
    }

    fn produced(&self, direction: Direction) -> &AtomicU64 {
        self.atomic_u64(OFF_PRODUCED + direction.index() * 8)
    }

    fn consumed(&self, direction: Direction) -> &AtomicU64 {
        self.atomic_u64(OFF_CONSUMED + direction.index() * 8)
    }

    fn overwritten(&self, direction: Direction) -> &AtomicU64 {
        self.atomic_u64(OFF_OVERWRITTEN + direction.index() * 8)
    }

    fn slot_address(&self, direction: Direction, index: usize) -> usize {
        WIRE_HEADER_SIZE
            + (direction.index() * WIRE_RING_CAPACITY + index) * WIRE_SLOT_SIZE
    }

    fn slot_published(&self, slot: usize) -> &AtomicI64 {
        self.atomic_i64(slot + SLOT_OFF_PUBLISHED)
    }

    fn atomic_u32(&self, offset: usize) -> &AtomicU32 {
        // SAFETY: all atomic offsets are 4-byte aligned in the validated mapping.
        unsafe { &*self.view.as_ptr().add(offset).cast::<AtomicU32>() }
    }

    fn atomic_i32(&self, offset: usize) -> &AtomicI32 {
        // SAFETY: all atomic offsets are 4-byte aligned in the validated mapping.
        unsafe { &*self.view.as_ptr().add(offset).cast::<AtomicI32>() }
    }

    fn atomic_u64(&self, offset: usize) -> &AtomicU64 {
        // SAFETY: all atomic offsets are 8-byte aligned in the validated mapping.
        unsafe { &*self.view.as_ptr().add(offset).cast::<AtomicU64>() }
    }

    fn atomic_i64(&self, offset: usize) -> &AtomicI64 {
        // SAFETY: all atomic offsets are 8-byte aligned in the validated mapping.
        unsafe { &*self.view.as_ptr().add(offset).cast::<AtomicI64>() }
    }

    fn read_u16(&self, offset: usize) -> u16 {
        // SAFETY: the offset is within the validated mapping.
        unsafe { std::ptr::read_unaligned(self.view.as_ptr().add(offset).cast::<u16>()) }
    }

    fn read_u32(&self, offset: usize) -> u32 {
        // SAFETY: the offset is within the validated mapping.
        unsafe { std::ptr::read_unaligned(self.view.as_ptr().add(offset).cast::<u32>()) }
    }

    fn read_u64(&self, offset: usize) -> u64 {
        // SAFETY: the offset is within the validated mapping.
        unsafe { std::ptr::read_unaligned(self.view.as_ptr().add(offset).cast::<u64>()) }
    }

    fn read_u64_at(&self, absolute: usize) -> u64 {
        // SAFETY: the offset is within the validated mapping.
        unsafe { std::ptr::read_unaligned(self.view.as_ptr().add(absolute).cast::<u64>()) }
    }

    fn read_u32_at(&self, absolute: usize) -> u32 {
        // SAFETY: the offset is within the validated mapping.
        unsafe { std::ptr::read_unaligned(self.view.as_ptr().add(absolute).cast::<u32>()) }
    }

    fn read_bytes(&self, offset: usize, length: usize) -> Vec<u8> {
        self.read_bytes_at(offset, length)
    }

    fn read_array<const N: usize>(&self, offset: usize) -> [u8; N] {
        self.read_bytes(offset, N)
            .try_into()
            .expect("fixed Wire IPC read length")
    }

    fn read_bytes_at(&self, absolute: usize, length: usize) -> Vec<u8> {
        let mut output = vec![0u8; length];
        // SAFETY: source range is inside the validated mapping and does not overlap output.
        unsafe {
            std::ptr::copy_nonoverlapping(
                self.view.as_ptr().add(absolute),
                output.as_mut_ptr(),
                length,
            );
        }
        output
    }

    fn write_u16(&self, offset: usize, value: u16) {
        // SAFETY: the offset is within the freshly created mapping.
        unsafe { std::ptr::write_unaligned(self.view.as_ptr().add(offset).cast::<u16>(), value) }
    }

    fn write_u32(&self, offset: usize, value: u32) {
        // SAFETY: the offset is within the freshly created mapping.
        unsafe { std::ptr::write_unaligned(self.view.as_ptr().add(offset).cast::<u32>(), value) }
    }

    fn write_u32_at(&self, absolute: usize, value: u32) {
        // SAFETY: the offset is within the freshly created mapping.
        unsafe {
            std::ptr::write_unaligned(self.view.as_ptr().add(absolute).cast::<u32>(), value)
        }
    }

    fn write_u64(&self, offset: usize, value: u64) {
        // SAFETY: the offset is within the freshly created mapping.
        unsafe { std::ptr::write_unaligned(self.view.as_ptr().add(offset).cast::<u64>(), value) }
    }

    fn write_u64_at(&self, absolute: usize, value: u64) {
        // SAFETY: the offset is within the freshly created mapping.
        unsafe {
            std::ptr::write_unaligned(self.view.as_ptr().add(absolute).cast::<u64>(), value)
        }
    }

    fn write_bytes(&self, offset: usize, bytes: &[u8]) {
        self.write_bytes_at(offset, bytes);
    }

    fn write_bytes_at(&self, absolute: usize, bytes: &[u8]) {
        // SAFETY: source does not overlap the destination mapping and the range fits.
        unsafe {
            std::ptr::copy_nonoverlapping(
                bytes.as_ptr(),
                self.view.as_ptr().add(absolute),
                bytes.len(),
            );
        }
    }
}

#[allow(dead_code)]
fn _keep_atomic_i32(mapping: &WireMapping, offset: usize) -> &AtomicI32 {
    mapping.atomic_i32(offset)
}

#[must_use]
pub fn relay_loopback(port: u16) -> std::net::SocketAddr {
    std::net::SocketAddr::from((Ipv4Addr::LOCALHOST, port))
}
