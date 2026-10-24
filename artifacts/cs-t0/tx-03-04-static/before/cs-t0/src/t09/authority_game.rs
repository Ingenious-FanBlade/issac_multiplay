//! Real-game Authority driver support for T0-09-B.
//!
//! This module creates the SharedV7 event/script mapping and companion loop
//! mapping expected by the existing coop-active Native runtime, injects the
//! resident helper, drains events, and performs one normal helper stop. It does
//! not itself access Player/device/Lua/world memory.

use std::ffi::c_void;
use std::fs::{File, OpenOptions};
use std::io::{BufWriter, Seek, SeekFrom, Write};
use std::mem::{offset_of, size_of};
use std::path::{Path, PathBuf};
use std::process::Command;
use std::ptr::{NonNull, read_volatile, write_volatile};
use std::sync::atomic::{fence, AtomicI32, AtomicU64, Ordering};

use serde::Serialize;
use serde_json::json;
use windows::core::PCWSTR;
use windows::Win32::Foundation::{
    CloseHandle, GetLastError, ERROR_ALREADY_EXISTS, HANDLE, INVALID_HANDLE_VALUE,
};
use windows::Win32::System::Memory::{
    CreateFileMappingW, MapViewOfFile, UnmapViewOfFile, FILE_MAP_ALL_ACCESS,
    MEMORY_MAPPED_VIEW_ADDRESS, PAGE_READWRITE,
};

use crate::error::{AppError, AppResult};
use crate::loop_ipc::LoopMapping;
use crate::t09::manifest::Manifest;
use crate::win32::ObservedProcess;
use crate::coop_active::InputStateDraftV2;

const MAGIC: u64 = 0x3156_4341_4F4F_4349; // ICOOACV1
const EVENT_CAPACITY: usize = 32768;
const SCRIPT_CAPACITY: usize = 256;
const WORLD_EXTENSION_SIZE: usize = 192;
const INPUT_CONFIG_SIZE: usize = 48;
const EVENTS_OFFSET: usize = size_of::<Header>() + size_of::<NativeStep>() * SCRIPT_CAPACITY;
const BASE_MAPPING_SIZE: usize = EVENTS_OFFSET + size_of::<Event>() * EVENT_CAPACITY;
const INPUT_CONFIG_OFFSET: usize = BASE_MAPPING_SIZE + WORLD_EXTENSION_SIZE;
const V5_MAPPING_SIZE: usize = 4_205_144;
const T09_CONFIG_SIZE: usize = 96;
const V7_MAPPING_SIZE: usize = V5_MAPPING_SIZE + T09_CONFIG_SIZE;

#[repr(C, align(8))]
struct Header {
    magic: u64,
    version: u32,
    size: u32,
    pid: u32,
    script_count: u32,
    session: [u8; 16],
    nonce: [u8; 32],
    clients: [u64; 2],
    creation_time: u64,
    controller_pid: u32,
    b_character: i32,
    loop_name: [u16; 96],
    state: AtomicI32,
    error: AtomicI32,
    stop: AtomicI32,
    retire_b: AtomicI32,
    main_thread: AtomicI32,
    active_callbacks: AtomicI32,
    pinned: AtomicI32,
    restored: AtomicI32,
    produced: AtomicU64,
    consumed: AtomicU64,
    completed_tick: AtomicU64,
    overwritten: AtomicU64,
}

#[repr(C)]
#[derive(Clone, Copy)]
struct NativeStep {
    tick: u64,
    state: InputStateDraftV2,
    fault: u32,
    shoot_directions: u32,
}

#[repr(C, align(8))]
#[derive(Clone, Copy)]
struct NativeInputConfig {
    version: u32,
    size: u32,
    input_lead_ticks: u64,
    future_window_ticks: u64,
    max_input_age_ticks: u64,
    script_drain_budget: u32,
    admission_attempt_budget: u32,
    reserved: u64,
}

#[repr(C, align(8))]
#[derive(Clone, Copy, Serialize)]
struct Event {
    published: i64,
    tick: u64,
    generation: u64,
    command: u64,
    target: u64,
    kind: u32,
    ordinal: u32,
    action: u32,
    query: u32,
    decision: u32,
    detail: u32,
    scalar: f32,
    boolean: u32,
    state: InputStateDraftV2,
    before: [u32; 4],
    after: [u32; 4],
}

#[repr(C, align(8))]
#[derive(Clone, Copy)]
struct T09AuthorityConfig {
    version: u32,
    size: u32,
    role: u32,
    flags: u32,
    run_nonce: [u8; 16],
    run_ticks: u64,
    build: [u8; 32],
    reserved: [u8; 24],
}

const _: () = assert!(size_of::<Header>() == 360);
const _: () = assert!(size_of::<NativeStep>() == 40);
const _: () = assert!(size_of::<Event>() == 128);
const _: () = assert!(offset_of!(Event, state) == 72);
const _: () = assert!(size_of::<NativeInputConfig>() == INPUT_CONFIG_SIZE);
const _: () = assert!(size_of::<T09AuthorityConfig>() == T09_CONFIG_SIZE);
const _: () = assert!(EVENTS_OFFSET == 10_600);
const _: () = assert!(BASE_MAPPING_SIZE == 4_204_904);
const _: () = assert!(INPUT_CONFIG_OFFSET == 4_205_096);
const _: () = assert!(V5_MAPPING_SIZE == 4_205_144);
const _: () = assert!(V7_MAPPING_SIZE == 4_205_240);

struct SharedV7 {
    handle: HANDLE,
    view: NonNull<u8>,
}

impl Drop for SharedV7 {
    fn drop(&mut self) {
        // SAFETY: this view was returned by MapViewOfFile and is unmapped once.
        unsafe {
            let _ = UnmapViewOfFile(MEMORY_MAPPED_VIEW_ADDRESS {
                Value: self.view.as_ptr().cast::<c_void>(),
            });
            let _ = CloseHandle(self.handle);
        }
    }
}

impl SharedV7 {
    fn create(name: String) -> AppResult<Self> {
        let wide: Vec<u16> = name.encode_utf16().chain(Some(0)).collect();
        // SAFETY: the mapping has the exact size declared by SharedV7.
        let handle = unsafe {
            CreateFileMappingW(
                INVALID_HANDLE_VALUE,
                None,
                PAGE_READWRITE,
                0,
                V7_MAPPING_SIZE as u32,
                PCWSTR(wide.as_ptr()),
            )
        }
        .map_err(|error| AppError::evidence(format!("create SharedV7 mapping: {error}")))?;
        // SAFETY: GetLastError reads only this thread's last error.
        if unsafe { GetLastError() } == ERROR_ALREADY_EXISTS {
            // SAFETY: handle is owned and no view exists.
            let _ = unsafe { CloseHandle(handle) };
            return Err(AppError::evidence(
                "SharedV7 mapping already exists; old session is not reused",
            ));
        }
        let raw = unsafe { MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, V7_MAPPING_SIZE) };
        let Some(view) = NonNull::new(raw.Value.cast::<u8>()) else {
            // SAFETY: construction failed before ownership transfer.
            let _ = unsafe { CloseHandle(handle) };
            return Err(AppError::evidence("MapViewOfFile failed for SharedV7"));
        };
        Ok(Self { handle, view })
    }

    fn header(&self) -> &Header {
        // SAFETY: the mapping base is at least sizeof(Header) and properly aligned.
        unsafe { &*self.view.as_ptr().cast::<Header>() }
    }

    fn initialize(
        &self,
        process: &ObservedProcess,
        manifest: &Manifest,
        companion_name: &str,
        input_lead_ticks: u64,
        future_window_ticks: u64,
        script_drain_budget: u32,
        admission_attempt_budget: u32,
    ) -> AppResult<()> {
        let identity = manifest.wire_identity()?;
        let mut nonce = [0u8; 32];
        nonce[..16].copy_from_slice(&identity.authority_nonce);
        nonce[16..].copy_from_slice(&identity.remote_nonce);
        let loop_wide: Vec<u16> = companion_name.encode_utf16().collect();
        if loop_wide.is_empty() || loop_wide.len() >= 96 {
            return Err(AppError::usage("companion loop mapping name too long"));
        }
        let header = self.view.as_ptr().cast::<Header>();
        unsafe {
            write_volatile(&raw mut (*header).magic, MAGIC);
            write_volatile(&raw mut (*header).version, 7);
            write_volatile(&raw mut (*header).size, V7_MAPPING_SIZE as u32);
            write_volatile(&raw mut (*header).pid, process.pid);
            write_volatile(&raw mut (*header).script_count, 0);
            write_volatile(&raw mut (*header).creation_time, process.creation_time_100ns);
            write_volatile(&raw mut (*header).controller_pid, std::process::id());
            write_volatile(&raw mut (*header).b_character, 0); // explicit Isaac
            std::ptr::copy_nonoverlapping(
                identity.session.as_ptr(),
                (&raw mut (*header).session).cast::<u8>(),
                16,
            );
            write_volatile(&raw mut (*header).nonce, nonce);
            write_volatile(&raw mut (*header).clients, manifest.wire.clients);
            std::ptr::copy_nonoverlapping(
                loop_wide.as_ptr(),
                (&raw mut (*header).loop_name).cast::<u16>(),
                loop_wide.len(),
            );
            (*header).state.store(0, Ordering::Release);
            (*header).error.store(0, Ordering::Release);
            (*header).stop.store(0, Ordering::Release);
            (*header).produced.store(0, Ordering::Release);
            (*header).consumed.store(0, Ordering::Release);
            (*header).overwritten.store(0, Ordering::Release);
            (*header).restored.store(0, Ordering::Release);
        }
        // v4 world slot stays empty for P1.
        let input = NativeInputConfig {
            version: 1,
            size: INPUT_CONFIG_SIZE as u32,
            input_lead_ticks,
            future_window_ticks,
            max_input_age_ticks: 0,
            script_drain_budget,
            admission_attempt_budget,
            reserved: 0,
        };
        unsafe {
            std::ptr::write(
                self.view.as_ptr().add(INPUT_CONFIG_OFFSET).cast::<NativeInputConfig>(),
                input,
            );
            let mut build = [0u8; 32];
            let label = manifest.build_label.as_bytes();
            if label.len() > build.len() {
                return Err(AppError::usage("build label exceeds T09 identity"));
            }
            build[..label.len()].copy_from_slice(label);
            let t09 = T09AuthorityConfig {
                version: 1,
                size: T09_CONFIG_SIZE as u32,
                role: 1,
                flags: 0,
                run_nonce: identity.run_nonce,
                run_ticks: manifest.script.last_tick,
                build,
                reserved: [0; 24],
            };
            std::ptr::write(self.view.as_ptr().add(V5_MAPPING_SIZE).cast::<T09AuthorityConfig>(), t09);
        }
        fence(Ordering::Release);
        Ok(())
    }

    fn drain_events(
        &self,
        next: &mut u64,
        file: &mut BufWriter<File>,
        restored_rows: &mut u64,
        restored_catalog: &mut bool,
        restored_catalog_count: &mut u32,
    ) -> AppResult<()> {
        let header = self.header();
        let produced = header.produced.load(Ordering::Acquire);
        if header.overwritten.load(Ordering::Acquire) != 0
            || produced.saturating_sub(*next) > EVENT_CAPACITY as u64
        {
            return Err(AppError::evidence("co-op event ring overflow"));
        }
        while *next < produced {
            let sequence = *next + 1;
            let slot = unsafe {
                self.view
                    .as_ptr()
                    .add(EVENTS_OFFSET)
                    .cast::<Event>()
                    .add(((sequence - 1) % EVENT_CAPACITY as u64) as usize)
            };
            let published = unsafe { read_volatile(&raw const (*slot).published) };
            if published != sequence as i64 {
                break;
            }
            fence(Ordering::Acquire);
            let event = unsafe { read_volatile(slot) };
            fence(Ordering::Acquire);
            if unsafe { read_volatile(&raw const (*slot).published) } != published
                || event.published != published
            {
                break;
            }
            if event.kind == 15 {
                if event.action != u32::MAX && event.boolean == 1 && event.before == event.after {
                    *restored_rows = restored_rows.saturating_add(1);
                }
                if event.action == u32::MAX && event.boolean == 1 {
                    *restored_catalog = true;
                    *restored_catalog_count = event.detail;
                }
            }
            serde_json::to_writer(&mut *file, &event)
                .map_err(|error| AppError::evidence(format!("serialize co-op event: {error}")))?;
            file.write_all(b"\n")
                .map_err(|error| AppError::evidence(format!("write co-op event: {error}")))?;
            *next = sequence;
            header.consumed.store(*next, Ordering::Release);
        }
        file.flush()
            .map_err(|error| AppError::evidence(format!("flush co-op events: {error}")))
    }

    fn request_stop(&self) {
        self.header().stop.store(1, Ordering::Release);
    }
}

// SharedV7 is only touched by the controller main thread. LoopMapping is stored
// behind the same ownership and is passed to drain without crossing threads.
struct AuthorityGameShared {
    shared: SharedV7,
    companion: LoopMapping,
    events: BufWriter<File>,
    loops: BufWriter<File>,
    event_next: u64,
    loop_next: u64,
    restored_rows: u64,
    restored_catalog: bool,
    restored_catalog_count: u32,
}

pub struct AuthorityGameDriver {
    process: ObservedProcess,
    game_exe: PathBuf,
    native_dll: PathBuf,
    helper: PathBuf,
    stdout_path: PathBuf,
    stderr_path: PathBuf,
    shared: AuthorityGameShared,
    stop_attempted: bool,
    stop_success: bool,
}

impl AuthorityGameDriver {
    #[allow(clippy::too_many_arguments)]
    pub fn start(
        process: ObservedProcess,
        game_exe: PathBuf,
        native_dll: PathBuf,
        helper: PathBuf,
        manifest: &Manifest,
        artifact: &Path,
        _wire_mapping_name: &str,
    ) -> AppResult<Self> {
        let identity = manifest.wire_identity()?;
        let suffix = hex::encode(identity.run_nonce);
        let shared_name = format!("Local\\IsaacCsT0.CoopActive.v7.{suffix}");
        let companion_name = format!("Local\\IsaacCsT0.CoopActiveLoop.{suffix}");
        let companion = LoopMapping::create(
            companion_name.clone(),
            process.pid,
            1,
            {
                let mut nonce = [0u8; 32];
                nonce[..16].copy_from_slice(&identity.authority_nonce);
                nonce[16..].copy_from_slice(&identity.remote_nonce);
                nonce
            },
        )?;
        let shared = SharedV7::create(shared_name.clone())?;
        shared.initialize(
            &process,
            manifest,
            companion.name(),
            manifest.script.lead_ticks,
            64,
            16,
            258,
        )?;
        let stdout_path = artifact.join("driver.stdout.log");
        let stderr_path = artifact.join("driver.stderr.log");
        let events_path = artifact.join("events.jsonl");
        let loops_path = artifact.join("loop-events.jsonl");
        let mut events = BufWriter::with_capacity(
            256 * 1024,
            OpenOptions::new()
                .write(true)
                .create_new(true)
                .open(events_path)
                .map_err(|error| AppError::evidence(format!("create events.jsonl: {error}")))?,
        );
        let mut loops = BufWriter::with_capacity(
            256 * 1024,
            OpenOptions::new()
                .write(true)
                .create_new(true)
                .open(loops_path)
                .map_err(|error| AppError::evidence(format!("create loop-events.jsonl: {error}")))?,
        );
        let injection = Command::new(&helper)
            .args([
                "--inject-coop-active",
                &process.pid.to_string(),
                &native_dll.to_string_lossy(),
                &shared_name,
            ])
            .output()
            .map_err(|error| AppError::evidence(format!("run T09 authority inject helper: {error}")))?;
        append_output(&stdout_path, &injection.stdout)?;
        append_output(&stderr_path, &injection.stderr)?;
        if !injection.status.success() {
            let mut failed = AuthorityGameShared {
                shared,
                companion,
                events,
                loops,
                event_next: 0,
                loop_next: 0,
                restored_rows: 0,
                restored_catalog: false,
                restored_catalog_count: 0,
            };
            let _ = failed.shared.drain_events(
                &mut failed.event_next,
                &mut failed.events,
                &mut failed.restored_rows,
                &mut failed.restored_catalog,
                &mut failed.restored_catalog_count,
            );
            let _ = drain_loop_events(
                &failed.companion,
                &mut failed.loop_next,
                &mut failed.loops,
            );
            let _ = failed.events.flush();
            let _ = failed.loops.flush();
            let header = failed.shared.header();
            let native_state = header.state.load(Ordering::Acquire);
            let native_error = header.error.load(Ordering::Acquire);
            let native_restored = header.restored.load(Ordering::Acquire);
            let event_overwritten = header.overwritten.load(Ordering::Acquire);
            let diagnostic = json!({
                "helperExit":injection.status.code(),
                "nativeState":native_state,
                "nativeError":native_error,
                "nativeRestored":native_restored,
                "eventOverwritten":event_overwritten,
                "eventRecords":failed.event_next,
                "loopRecords":failed.loop_next,
                "automaticRetry":false
            });
            super::manifest::write_new_json(
                &artifact.join("native-init-failure.json"),
                &diagnostic,
            )?;
            return Err(AppError::evidence(format!(
                "T09 authority injection helper failed: {}; nativeState={} nativeError={} restored={} eventOverwritten={} events={} loops={}",
                injection.status,
                native_state,
                native_error,
                native_restored,
                event_overwritten,
                failed.event_next,
                failed.loop_next
            )));
        }
        Ok(Self {
            process,
            game_exe,
            native_dll,
            helper,
            stdout_path,
            stderr_path,
            shared: AuthorityGameShared {
                shared,
                companion,
                events,
                loops,
                event_next: 0,
                loop_next: 0,
                restored_rows: 0,
                restored_catalog: false,
                restored_catalog_count: 0,
            },
            stop_attempted: false,
            stop_success: false,
        })
    }

    pub fn pid(&self) -> u32 {
        self.process.pid
    }

    pub fn creation_time_100ns(&self) -> u64 {
        self.process.creation_time_100ns
    }

    pub fn pump_auxiliary(&mut self) -> AppResult<()> {
        self.shared.shared.drain_events(
            &mut self.shared.event_next,
            &mut self.shared.events,
            &mut self.shared.restored_rows,
            &mut self.shared.restored_catalog,
            &mut self.shared.restored_catalog_count,
        )?;
        drain_loop_events(
            &self.shared.companion,
            &mut self.shared.loop_next,
            &mut self.shared.loops,
        )
    }

    pub fn request_stop(&mut self) -> AppResult<bool> {
        if self.stop_attempted {
            return Ok(self.stop_success);
        }
        self.stop_attempted = true;
        self.shared.shared.request_stop();
        let output = Command::new(&self.helper)
            .args([
                "--stop-coop-active",
                &self.process.pid.to_string(),
                &self.native_dll.to_string_lossy(),
            ])
            .output()
            .map_err(|error| AppError::evidence(format!("run T09 authority stop helper: {error}")))?;
        append_output(&self.stdout_path, &output.stdout)?;
        append_output(&self.stderr_path, &output.stderr)?;
        self.stop_success = output.status.success();
        Ok(self.stop_success)
    }

    pub fn game_alive(&self) -> AppResult<bool> {
        Ok(!self.process.wait(0)?)
    }

    pub fn stop_success(&self) -> bool {
        self.stop_attempted && self.stop_success
    }

    pub fn finish(&mut self) -> AppResult<AuthorityFinish> {
        self.pump_auxiliary()?;
        self.shared.events.flush().ok();
        self.shared.loops.flush().ok();
        let header = self.shared.shared.header();
        let same_process = ObservedProcess::open_exact(self.process.pid, &self.game_exe)
            .map(|fresh| fresh.creation_time_100ns == self.process.creation_time_100ns)
            .unwrap_or(false);
        let dll_name = self
            .native_dll
            .file_name()
            .map(|name| name.to_string_lossy().to_ascii_lowercase())
            .unwrap_or_default();
        let dll_resident = ObservedProcess::open_exact(self.process.pid, &self.game_exe)
            .map(|fresh| {
                fresh
                    .modules
                    .iter()
                    .any(|module| module.name.to_ascii_lowercase() == dll_name)
            })
            .unwrap_or(false);
        let native_restored = header.restored.load(Ordering::Acquire) != 0;
        let native_error = header.error.load(Ordering::Acquire);
        let overwritten = header.overwritten.load(Ordering::Acquire);
        let summary = AuthorityFinish {
            same_process,
            dll_resident,
            native_restored,
            native_error,
            event_overwritten: overwritten,
            restored_rows: self.shared.restored_rows,
            restored_catalog: self.shared.restored_catalog,
            restored_catalog_count: self.shared.restored_catalog_count,
            stop_success: self.stop_success,
            loop_records: self.shared.loop_next,
            event_records: self.shared.event_next,
        };
        super::manifest::write_new_json(
            &self
                .stdout_path
                .parent()
                .ok_or_else(|| AppError::evidence("driver artifact directory missing"))?
                .join("restoration.json"),
            &serde_json::to_value(&summary)
                .map_err(|error| AppError::evidence(format!("serialize restoration: {error}")))?,
        )?;
        Ok(summary)
    }
}

#[derive(Clone, Debug, Serialize)]
pub struct AuthorityFinish {
    pub same_process: bool,
    pub dll_resident: bool,
    pub native_restored: bool,
    pub native_error: i32,
    pub event_overwritten: u64,
    pub restored_rows: u64,
    pub restored_catalog: bool,
    pub restored_catalog_count: u32,
    pub stop_success: bool,
    pub loop_records: u64,
    pub event_records: u64,
}

impl AuthorityFinish {
    pub fn clean(&self) -> bool {
        self.same_process
            && self.dll_resident
            && self.native_restored
            && self.native_error == 0
            && self.event_overwritten == 0
            && self.restored_catalog_count > 0
            && self.restored_rows == u64::from(self.restored_catalog_count)
            && self.restored_catalog
            && self.stop_success
    }
}

fn drain_loop_events(
    companion: &LoopMapping,
    next: &mut u64,
    file: &mut BufWriter<File>,
) -> AppResult<()> {
    for record in companion.drain(next)? {
        let event = record.event;
        serde_json::to_writer(
            &mut *file,
            &json!({
                "sequence":record.sequence,
                "kind":event.kind,
                "phase":event.phase,
                "loopIteration":event.loop_iteration,
                "threadId":event.thread_id,
                "returnRva":event.return_rva,
                "originalCalls":event.original_calls,
                "flags":event.flags,
                "errorCode":event.error_code,
                "action":event.action,
                "deviceKey":event.device_key
            }),
        )
        .map_err(|error| AppError::evidence(format!("serialize loop event: {error}")))?;
        file.write_all(b"
")
            .map_err(|error| AppError::evidence(format!("write loop event: {error}")))?;
    }
    file.flush()
        .map_err(|error| AppError::evidence(format!("flush loop events: {error}")))
}

fn append_output(path: &Path, bytes: &[u8]) -> AppResult<()> {
    if bytes.is_empty() {
        return Ok(());
    }
    let mut file = OpenOptions::new()
        .write(true)
        .open(path)
        .map_err(|error| AppError::evidence(format!("append {}: {error}", path.display())))?;
    file.seek(SeekFrom::End(0))
        .map_err(|error| AppError::evidence(format!("append seek {}: {error}", path.display())))?;
    file.write_all(bytes)
        .map_err(|error| AppError::evidence(format!("append {}: {error}", path.display())))
}
