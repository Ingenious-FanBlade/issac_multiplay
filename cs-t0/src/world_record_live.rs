//! Live transport only. The reader owns raw copies before returning shared slots;
//! conversion, validation and I/O run exclusively on the writer thread.
use super::*;
use std::collections::VecDeque;
use std::ffi::c_void;
use std::io::BufWriter;
use std::mem::{offset_of, size_of};
use std::ptr::{NonNull, read_volatile};
use std::sync::atomic::{AtomicBool, AtomicI32, AtomicU64, Ordering};
use std::sync::{Arc, Condvar, Mutex, MutexGuard};
use std::thread::{self, JoinHandle};
use std::time::{Duration, Instant};
use windows::core::PCWSTR;
use windows::Win32::Foundation::{CloseHandle, ERROR_ALREADY_EXISTS, GetLastError, HANDLE, INVALID_HANDLE_VALUE, WAIT_FAILED};
use windows::Win32::System::Memory::{CreateFileMappingW, MapViewOfFile, UnmapViewOfFile, MEMORY_MAPPED_VIEW_ADDRESS, FILE_MAP_ALL_ACCESS, PAGE_READWRITE};
use windows::Win32::System::Threading::{CreateEventW, SetEvent, WaitForSingleObject};

const OBJECTS: usize = 16384;
const MEMBERSHIPS: usize = 65536;
const EVENTS: usize = 65536;
const QUEUE_BYTES: usize = 256 * 1024 * 1024;
const QUEUE_ITEMS: usize = 256;
const EVENT_CHUNK: usize = 1024;
const OUTPUT_BUFFER: usize = 1024 * 1024;

#[repr(C)] #[derive(Clone, Copy)]
struct NativeField { id:u16, status:u8, encoding:u8, length:u32, relation:u32, reserved:u32, target_id:u64, target_use:u64, raw:[u8;16] }
#[repr(C)] #[derive(Clone, Copy)]
struct NativeObject { id:u64, usage:u64, birth:u64, revision:u64, domain:u32, kind:u32, address:u32, vptr:u32, origin:u32, lifecycle:u32, init:u32, birth_kind:u32, birth_type:u32, birth_variant:u32, birth_subtype:u32, birth_seed:u32, birth_known:u32, ordinal:u32, field_count:u32, reserved:u32, fields:[NativeField;52] }
#[repr(C)] #[derive(Clone, Copy)]
struct NativeMembership { object:u32, source:u32, index:u32, reserved:u32 }
#[repr(C)] #[derive(Clone, Copy)]
struct NativeSource { status:u32, count:u32, descriptor:[u32;4] }
#[repr(C, align(8))]
struct Batch { state:AtomicI32, reason:u32, run:u64, room:u64, mutation:u64, attempt:u64, capture:u64, outer:u64, tick:u64, scope:u64, authority:u32, commit_kind:u32, thread:u32, complete:u32, object_count:u32, membership_count:u32, required_objects:u32, required_memberships:u32, sources:[NativeSource;15], objects:[NativeObject;OBJECTS], memberships:[NativeMembership;MEMBERSHIPS] }
#[repr(C, align(8))] #[derive(Clone, Copy)]
struct Event { published:i64, run:u64, room:u64, outer:u64, id:u64, usage:u64, birth:u64, revision:u64, hook:u32, entering:u32, address:u32, result:u32, kind:u32, reason:u32 }
#[repr(C, align(8))]
struct Header { magic:u64, version:u32, size:u32, pid:u32, controller_pid:u32, nonce:[u8;32], enabled:AtomicI32, error:AtomicI32, callbacks:AtomicI32, closed:AtomicI32, produced:AtomicU64, consumed:AtomicU64, backpressure:AtomicU64, attempts:AtomicU64, captures:AtomicU64, enters:[u64;19], returns:[u64;19] }
const BATCH_OFFSET: usize = size_of::<Header>() + EVENTS * size_of::<Event>();
const MAPPING_SIZE: usize = BATCH_OFFSET + 3 * size_of::<Batch>();
const _: () = assert!(size_of::<NativeField>() == 48 && size_of::<NativeObject>() == 2592 && size_of::<Event>() == 88 && size_of::<Header>() == 416);
const _: () = assert!(offset_of!(Batch, objects) == 464 && size_of::<Batch>() == 43516368 && MAPPING_SIZE == 136316688);

struct OwnedHandle(HANDLE);
impl Drop for OwnedHandle { fn drop(&mut self) { let _ = unsafe { CloseHandle(self.0) }; } }
struct SharedView { _mapping:OwnedHandle, view:NonNull<u8>, ready:OwnedHandle }
// The mapping outlives both joined workers. Only the reader touches READY batches
// under the native release/acquire ownership protocol; header counters are atomic.
unsafe impl Send for SharedView {}
unsafe impl Sync for SharedView {}
impl SharedView {
    fn header(&self) -> &Header { unsafe { &*self.view.as_ptr().cast::<Header>() } }
    fn batch(&self, index:usize) -> &Batch { unsafe { &*self.view.as_ptr().add(BATCH_OFFSET + index * size_of::<Batch>()).cast::<Batch>() } }
    fn signal(&self) -> AppResult<()> { unsafe { SetEvent(self.ready.0) }.map_err(error) }
    fn disable(&self) {
        self.header().enabled.store(0, Ordering::Release);
        let _ = self.header().error.compare_exchange(0, 14, Ordering::AcqRel, Ordering::Relaxed);
    }
}
impl Drop for SharedView {
    fn drop(&mut self) { let _ = unsafe { UnmapViewOfFile(MEMORY_MAPPED_VIEW_ADDRESS { Value:self.view.as_ptr().cast::<c_void>() }) }; }
}

#[derive(Clone, Copy)]
struct BatchMeta { reason:u32, run:u64, room:u64, mutation:u64, attempt:u64, capture:u64, outer:u64, tick:u64, scope:u64, authority:u32, commit_kind:u32, thread:u32, complete:u32, required_objects:u32, required_memberships:u32, sources:[NativeSource;15] }
struct OwnedBatch { meta:BatchMeta, objects:Vec<NativeObject>, memberships:Vec<NativeMembership> }
enum Payload { Batch(OwnedBatch), Events(Vec<Event>) }
struct Queued { payload:Payload, bytes:usize }
#[derive(Default)]
struct QueueState {
    items:VecDeque<Queued>, held_bytes:usize, held_items:usize, peak_bytes:usize, peak_items:usize,
    budget_waits:u64, batches_copied:u64, events_copied:u64, batches_written:u64, events_written:u64,
    max_copy_ns:u128, notifications:u64, failure:Option<String>, reader_done:bool, files_flushed:bool,
}
struct Pipeline { state:Mutex<QueueState>, available:Condvar, stop:AtomicBool }
impl Pipeline {
    fn new() -> Self { Self { state:Mutex::new(QueueState::default()), available:Condvar::new(), stop:AtomicBool::new(false) } }
    fn lock(&self) -> MutexGuard<'_, QueueState> { self.state.lock().unwrap_or_else(|poison|poison.into_inner()) }
    fn reserve(&self, bytes:usize) -> bool {
        let mut state = self.lock();
        if state.held_items == QUEUE_ITEMS || bytes > QUEUE_BYTES - state.held_bytes {
            state.budget_waits += 1; return false;
        }
        state.held_items += 1; state.held_bytes += bytes;
        state.peak_bytes = state.peak_bytes.max(state.held_bytes);
        state.peak_items = state.peak_items.max(state.held_items);
        true
    }
    fn release(&self, bytes:usize) {
        let mut state = self.lock(); state.held_items -= 1; state.held_bytes -= bytes;
    }
    fn push(&self, payload:Payload, bytes:usize, copy_ns:u128) {
        let mut state = self.lock();
        match &payload { Payload::Batch(_) => state.batches_copied += 1, Payload::Events(values) => state.events_copied += values.len() as u64 }
        state.max_copy_ns = state.max_copy_ns.max(copy_ns);
        state.items.push_back(Queued { payload, bytes }); self.available.notify_one();
    }
    fn take(&self) -> (Option<Queued>, bool) {
        let mut state = self.lock();
        if state.items.is_empty() && !state.reader_done {
            state = self.available.wait_timeout(state, Duration::from_millis(100)).unwrap_or_else(|poison|poison.into_inner()).0;
        }
        let item = state.items.pop_front(); let done = state.reader_done && item.is_none(); (item, done)
    }
    fn fail(&self, view:&SharedView, failure:String) {
        view.disable(); self.stop.store(true, Ordering::Release);
        { let mut state = self.lock(); if state.failure.is_none() { state.failure = Some(failure); } }
        self.available.notify_all(); let _ = view.signal();
    }
    fn failed(&self) -> bool { self.lock().failure.is_some() }
    fn reader_done(&self) { self.lock().reader_done = true; self.available.notify_all(); }
    fn report(&self) -> Value {
        let state = self.lock();
        json!({"tx":"tx-01-75","world_mapping_version":2,"reader":"DEDICATED_NAMED_EVENT","shared_release":"RAW_COPY_COMPLETE_BEFORE_ENCODING",
            "queue_limit_bytes":QUEUE_BYTES,"queue_limit_items":QUEUE_ITEMS,"budget_includes":"reader_reserved + queued + writer_inflight raw copies; events included",
            "queue_peak_bytes":state.peak_bytes,"queue_peak_items":state.peak_items,"budget_waits":state.budget_waits.to_string(),
            "batches_copied":state.batches_copied.to_string(),"batches_written":state.batches_written.to_string(),
            "events_copied":state.events_copied.to_string(),"events_written":state.events_written.to_string(),"notifications":state.notifications.to_string(),
            "max_raw_copy_ns":state.max_copy_ns.to_string(),"outstanding_items":state.held_items,"outstanding_bytes":state.held_bytes,
            "reader_done":state.reader_done,"files_flushed":state.files_flushed,"failure":state.failure,"output_buffer_bytes_per_file":OUTPUT_BUFFER,"runtime_acceptance":"NOT_RUN"})
    }
}

// On every copy failure/unwind the claimed shared slot is returned as well.
struct ReadSlot<'a>(&'a Batch);
impl Drop for ReadSlot<'_> { fn drop(&mut self) { self.0.state.store(0, Ordering::Release); } }
fn copy_values<T:Copy>(source:*const T, count:usize) -> AppResult<Vec<T>> {
    let mut values = Vec::new(); values.try_reserve_exact(count).map_err(error)?;
    // Caller holds the shared slot, or the unconsumed event sequence, until this
    // copy completes. Bounds have been checked against the fixed mapping layout.
    unsafe { std::ptr::copy_nonoverlapping(source, values.as_mut_ptr(), count); values.set_len(count); }
    Ok(values)
}
enum ReadResult { Empty, Copied, Budget }
fn read_batch(view:&SharedView, pipeline:&Pipeline) -> AppResult<ReadResult> {
    let candidate = (0..3).filter(|&i|view.batch(i).state.load(Ordering::Acquire) == 2)
        .min_by_key(|&i|view.batch(i).attempt);
    let Some(index) = candidate else { return Ok(ReadResult::Empty); };
    let batch = view.batch(index);
    let objects = batch.object_count as usize; let memberships = batch.membership_count as usize;
    if objects > OBJECTS || memberships > MEMBERSHIPS { return Err(error("native batch exceeded bounds")); }
    let bytes = size_of::<Queued>() + objects * size_of::<NativeObject>() + memberships * size_of::<NativeMembership>();
    if !pipeline.reserve(bytes) { return Ok(ReadResult::Budget); }
    if batch.state.compare_exchange(2, 3, Ordering::Acquire, Ordering::Relaxed).is_err() {
        pipeline.release(bytes); return Ok(ReadResult::Empty);
    }
    let slot = ReadSlot(batch); let start = Instant::now();
    let copied = (|| {
        let meta = BatchMeta { reason:batch.reason, run:batch.run, room:batch.room, mutation:batch.mutation, attempt:batch.attempt,
            capture:batch.capture, outer:batch.outer, tick:batch.tick, scope:batch.scope, authority:batch.authority, commit_kind:batch.commit_kind,
            thread:batch.thread, complete:batch.complete, required_objects:batch.required_objects, required_memberships:batch.required_memberships, sources:batch.sources };
        Ok(OwnedBatch { meta, objects:copy_values(batch.objects.as_ptr(), objects)?, memberships:copy_values(batch.memberships.as_ptr(), memberships)? })
    })();
    drop(slot); // No shared batch reference is carried to conversion or file I/O.
    let copy_ns = start.elapsed().as_nanos();
    match copied {
        Ok(value) => pipeline.push(Payload::Batch(value), bytes, copy_ns),
        Err(failure) => { pipeline.release(bytes); return Err(failure); }
    }
    Ok(ReadResult::Copied)
}
fn read_events(view:&SharedView, pipeline:&Pipeline) -> AppResult<ReadResult> {
    let next = view.header().consumed.load(Ordering::Acquire);
    let produced = view.header().produced.load(Ordering::Acquire);
    if produced < next || produced - next > EVENTS as u64 { return Err(error("world event sequence overrun")); }
    let count = ((produced - next) as usize).min(EVENT_CHUNK);
    if count == 0 { return Ok(ReadResult::Empty); }
    let bytes = size_of::<Queued>() + count * size_of::<Event>();
    if !pipeline.reserve(bytes) { return Ok(ReadResult::Budget); }
    let start = Instant::now();
    let copied = (|| {
        let mut values = Vec::new(); values.try_reserve_exact(count).map_err(error)?;
        for offset in 0..count {
            let sequence = next + offset as u64;
            let pointer = unsafe { view.view.as_ptr().add(size_of::<Header>()).cast::<Event>().add((sequence % EVENTS as u64) as usize) };
            let value = unsafe { read_volatile(pointer) };
            if value.published != sequence as i64 + 1 { return Err(error("world event publication gap")); }
            values.push(value);
        }
        Ok(values)
    })();
    match copied {
        Ok(values) => {
            // Only owned raw copies are acknowledged. Persisted counts are kept
            // separately and cannot report a failed write as durable consumption.
            view.header().consumed.store(next + count as u64, Ordering::Release);
            pipeline.push(Payload::Events(values), bytes, start.elapsed().as_nanos());
        }
        Err(failure) => { pipeline.release(bytes); return Err(failure); }
    }
    Ok(ReadResult::Copied)
}
fn reader_loop(view:&SharedView, pipeline:&Pipeline) -> AppResult<()> {
    loop {
        if pipeline.failed() { return Ok(()); }
        let mut progress = false; let mut blocked = false;
        // Bound each pass so lifecycle consumption is not starved by new batches.
        for _ in 0..3 {
            match read_batch(view, pipeline)? {
                ReadResult::Copied => progress = true,
                ReadResult::Empty => break,
                ReadResult::Budget => { blocked = true; break; }
            }
        }
        if !blocked {
            match read_events(view, pipeline)? {
                ReadResult::Copied => progress = true,
                ReadResult::Budget => blocked = true,
                ReadResult::Empty => {}
            }
        }
        if progress { continue; }
        if !blocked && pipeline.stop.load(Ordering::Acquire) { return Ok(()); }
        // Ready publication and released queue credit signal this auto-reset
        // event. Timeout is a failure/stop fallback, not the service interval.
        let waited = unsafe { WaitForSingleObject(view.ready.0, 100) };
        if waited == WAIT_FAILED { return Err(error(format!("world ready wait: {:?}", unsafe { GetLastError() }))); }
        if waited.0 == 0 { pipeline.lock().notifications += 1; }
    }
}

fn native_field(field:NativeField) -> AppResult<Field> {
    let status = ["VALUE","UNCONFIRMED","NOT_APPLICABLE","NOT_READY","READ_FAILED"].get(field.status as usize).ok_or_else(||error("native field status"))?;
    let encoding = ["RAW_LE","F32_BITS","REFERENCE","DERIVED_RAW32"].get(field.encoding as usize).ok_or_else(||error("native field encoding"))?;
    let mut value = Field { field_id:format!("{:04x}",field.id), status:(*status).into(), encoding:(*encoding).into(), width:field.length,
        raw32:None, raw64:None, bytes_le:None, reference:None, diagnostics:None };
    if field.status == 0 {
        if field.encoding == 2 {
            let relation = ["NULL","KNOWN_CURRENT","UNRESOLVED","RETIRED","AMBIGUOUS_REUSE"].get(field.relation as usize).ok_or_else(||error("native reference status"))?;
            value.reference = Some(Reference { status:(*relation).into(), target:(field.relation == 1).then(||LocalTarget { local_id:field.target_id.to_string(), use_generation:field.target_use.to_string() }) });
            value.diagnostics = Some(json!({"native_address":format!("{:08x}",u32::from_le_bytes(field.raw[..4].try_into().map_err(error)?))}));
        } else if field.length == 4 { value.raw32 = Some(format!("{:08x}",u32::from_le_bytes(field.raw[..4].try_into().map_err(error)?))); }
        else if field.length == 8 { value.raw64 = Some(format!("{:016x}",u64::from_le_bytes(field.raw[..8].try_into().map_err(error)?))); }
        else if field.length <= 16 { value.bytes_le = Some(hex::encode(&field.raw[..field.length as usize])); }
        else { return Err(error("native field length")); }
    }
    Ok(value)
}
fn encode_batch(batch:&OwnedBatch, session:&str) -> AppResult<Capture> {
    let mut output = Vec::with_capacity(batch.objects.len());
    for native in &batch.objects {
        if native.field_count > 52 { return Err(error("native field count")); }
        let fields = native.fields[..native.field_count as usize].iter().map(|field|native_field(*field)).collect::<AppResult<Vec<_>>>()?;
        output.push(Object { local_id:native.id.to_string(), use_generation:native.usage.to_string(), revision:native.revision.to_string(), object_domain:native.domain,
            class:CLASSES.get(native.kind as usize).ok_or_else(||error("native class"))?.to_string(), origin:if native.origin == 2 {"OBSERVED_CREATION"} else {"FIRST_OBSERVED"}.into(),
            lifecycle:native.lifecycle, init_evidence:native.init, birth_event:(native.birth_known != 0).then(||native.birth.to_string()),
            birth:Birth { kind:native.birth_kind, entity_type:native.birth_type, variant:native.birth_variant, subtype:native.birth_subtype, seed:native.birth_seed }, ordinal:(native.ordinal < 2).then_some(native.ordinal),
            fields, memberships:Vec::new(), diagnostics:json!({"address":format!("{:08x}",native.address),"vptr":format!("{:08x}",native.vptr)}) });
    }
    for value in &batch.memberships {
        output.get_mut(value.object as usize).ok_or_else(||error("native membership object index"))?.memberships.push(Membership { source:value.source, index:value.index });
    }
    let meta = &batch.meta;
    let sources = meta.sources.iter().enumerate().map(|(i,value)|Source { source:i as u32, status:if value.status == u32::MAX {"NOT_APPLICABLE"} else {reason(value.status)}.into(), count:value.count, diagnostics:json!({"descriptor":value.descriptor}) }).collect();
    let mut record = Capture { record:if meta.capture != 0 {"capture"} else {"gap"}.into(), domain:DOMAIN.into(), version:1, profile:PROFILE.into(), session:session.into(),
        run_epoch:meta.run.to_string(), room_epoch:meta.room.to_string(), mutation_epoch:meta.mutation.to_string(), attempt_seq:meta.attempt.to_string(), capture_seq:(meta.capture != 0).then(||meta.capture.to_string()),
        outer_token:meta.outer.to_string(), phase:"POST_STANDARD_UPDATE_PRE_TOP_EXIT".into(), authority_tick:(meta.authority != 0).then(||meta.tick.to_string()), authority_kind:meta.commit_kind, scope_token:meta.scope.to_string(),
        complete:meta.complete != 0, reason:reason(meta.reason).into(), sources, objects:output, required_objects:meta.required_objects, required_memberships:meta.required_memberships, diagnostics:json!({"thread":meta.thread}) };
    validate(&record)?; canonicalize(&mut record); Ok(record)
}
fn buffered_line(file:&mut BufWriter<File>, value:&impl Serialize) -> AppResult<()> {
    serde_json::to_writer(&mut *file, value).map_err(error)?; file.write_all(b"\n").map_err(error)
}
fn writer_loop(view:&SharedView, pipeline:&Pipeline, session:&str, records:File, events:File) -> AppResult<()> {
    let mut records = BufWriter::with_capacity(OUTPUT_BUFFER, records);
    let mut events = BufWriter::with_capacity(OUTPUT_BUFFER, events);
    let mut last_flush = Instant::now();
    loop {
        let (item, done) = pipeline.take();
        if let Some(item) = item {
            let written = match &item.payload {
                Payload::Batch(batch) => encode_batch(batch, session).and_then(|record|buffered_line(&mut records, &record)),
                Payload::Events(values) => values.iter().try_for_each(|event|buffered_line(&mut events, &json!({"record":"lifecycle_event","sequence":event.published.to_string(),
                    "run_epoch":event.run.to_string(),"room_epoch":event.room.to_string(),"outer_token":event.outer.to_string(),"local_id":(event.id!=0).then(||event.id.to_string()),
                    "use_generation":(event.id!=0).then(||event.usage.to_string()),"birth_event":(event.birth!=0).then(||event.birth.to_string()),"revision":(event.id!=0).then(||event.revision.to_string()),
                    "hook":HOOKS.get(event.hook as usize),"entering":event.entering!=0,"event_kind":event.kind,"reason":reason(event.reason),
                    "diagnostics":{"address":format!("{:08x}",event.address),"result":format!("{:08x}",event.result)}}))),
            };
            if written.is_ok() {
                let mut state = pipeline.lock();
                match &item.payload { Payload::Batch(_) => state.batches_written += 1, Payload::Events(values) => state.events_written += values.len() as u64 }
            }
            let bytes = item.bytes; drop(item); pipeline.release(bytes);
            // Released raw-copy credit wakes the reader even if no native
            // publication happened while the queue was full.
            view.signal()?; written?;
        }
        if done || last_flush.elapsed() >= Duration::from_millis(250) {
            records.flush().map_err(error)?; events.flush().map_err(error)?; last_flush = Instant::now();
        }
        if done { pipeline.lock().files_flushed = true; return Ok(()); }
    }
}
fn worker_result(result:std::thread::Result<AppResult<()>>, label:&str) -> AppResult<()> {
    match result { Ok(result) => result, Err(_) => Err(error(format!("world {label} thread panicked"))) }
}

pub struct Mapping {
    shared:Arc<SharedView>, pipeline:Arc<Pipeline>, reader:Option<JoinHandle<()>>, writer:Option<JoinHandle<()>>,
    pub name:String, directory:PathBuf, finished:bool, failure_reported:bool,
}
impl Mapping {
    pub fn create(name:String, directory:&Path, pid:u32, nonce:[u8;32]) -> AppResult<Self> {
        fs::create_dir(directory).map_err(error)?;
        let session = hex::encode(&nonce[..16]); let mut profile = manifest(&session);
        profile["consumer"] = json!({"tx":"tx-01-75","world_mapping_version":2,"notification":"NAMED_AUTO_RESET_EVENT","queue_bytes":QUEUE_BYTES,"queue_items":QUEUE_ITEMS,
            "release_shared":"AFTER_RAW_COPY_BEFORE_ENCODING","full_queue":"RETAIN_READY_AND_NATIVE_BACKPRESSURE"});
        save(&directory.join("manifest.json"), &profile)?;
        let records = new_log(&directory.join("captures.jsonl"))?; let events = new_log(&directory.join("lifecycle-events.jsonl"))?;
        let wide:Vec<_> = name.encode_utf16().chain(Some(0)).collect();
        let mapping = OwnedHandle(unsafe { CreateFileMappingW(INVALID_HANDLE_VALUE, None, PAGE_READWRITE, 0, MAPPING_SIZE as u32, PCWSTR(wide.as_ptr())) }.map_err(error)?);
        if unsafe { GetLastError() } == ERROR_ALREADY_EXISTS { return Err(error("world mapping already exists")); }
        let raw = unsafe { MapViewOfFile(mapping.0, FILE_MAP_ALL_ACCESS, 0, 0, MAPPING_SIZE) };
        let view = NonNull::new(raw.Value.cast::<u8>()).ok_or_else(||error("map world view failed"))?;
        let event_name:Vec<_> = format!("{name}.Ready").encode_utf16().chain(Some(0)).collect();
        let ready = match unsafe { CreateEventW(None, false, false, PCWSTR(event_name.as_ptr())) } {
            Ok(handle) => OwnedHandle(handle),
            Err(failure) => { let _ = unsafe { UnmapViewOfFile(raw) }; return Err(error(failure)); }
        };
        let existed = unsafe { GetLastError() } == ERROR_ALREADY_EXISTS;
        let shared = Arc::new(SharedView { _mapping:mapping, view, ready });
        if existed { return Err(error("world notification already exists")); }
        unsafe {
            let header = view.as_ptr().cast::<Header>();
            std::ptr::write(&raw mut (*header).magic, 0x31524f5743534949);
            std::ptr::write(&raw mut (*header).version, 2); std::ptr::write(&raw mut (*header).size, MAPPING_SIZE as u32);
            std::ptr::write(&raw mut (*header).pid, pid); std::ptr::write(&raw mut (*header).controller_pid, std::process::id()); std::ptr::write(&raw mut (*header).nonce, nonce);
        }
        let pipeline = Arc::new(Pipeline::new());
        let writer_view = shared.clone(); let writer_pipeline = pipeline.clone();
        let writer = thread::Builder::new().name("world-writer".into()).spawn(move || {
            let result = worker_result(std::panic::catch_unwind(std::panic::AssertUnwindSafe(||writer_loop(&writer_view, &writer_pipeline, &session, records, events))), "writer");
            if let Err(failure) = result { writer_pipeline.fail(&writer_view, failure.to_string()); }
        }).map_err(error)?;
        let reader_view = shared.clone(); let reader_pipeline = pipeline.clone();
        let reader = thread::Builder::new().name("world-reader".into()).spawn(move || {
            let result = worker_result(std::panic::catch_unwind(std::panic::AssertUnwindSafe(||reader_loop(&reader_view, &reader_pipeline))), "reader");
            if let Err(failure) = result { reader_pipeline.fail(&reader_view, failure.to_string()); }
            reader_pipeline.reader_done();
        });
        let reader = match reader {
            Ok(reader) => reader,
            Err(failure) => { pipeline.reader_done(); let _ = writer.join(); return Err(error(failure)); }
        };
        Ok(Self { shared, pipeline, reader:Some(reader), writer:Some(writer), name, directory:directory.to_owned(), finished:false, failure_reported:false })
    }
    pub fn poll_failure(&mut self) {
        let failure = self.pipeline.lock().failure.clone();
        if let Some(failure) = failure.filter(|_|!self.failure_reported) {
            self.failure_reported = true; eprintln!("world observation disabled: {failure}; coop remains active");
            let _ = save(&self.directory.join("consumer-failure.json"), &json!({"status":"CAPTURE_ERROR","reason":failure,"coop_scheduler_failed":false,"automatic_retry":false}));
        }
    }
    fn join_workers(&mut self) {
        self.shared.header().enabled.store(0, Ordering::Release);
        self.pipeline.stop.store(true, Ordering::Release);
        if let Err(failure) = self.shared.signal() { self.pipeline.fail(&self.shared, failure.to_string()); }
        if let Some(reader) = self.reader.take() { if reader.join().is_err() { self.pipeline.fail(&self.shared, "world reader join panicked".into()); self.pipeline.reader_done(); } }
        if let Some(writer) = self.writer.take() { if writer.join().is_err() { self.pipeline.fail(&self.shared, "world writer join panicked".into()); } }
    }
    pub fn finish(&mut self, restoration:&Value) -> AppResult<()> {
        self.join_workers(); self.poll_failure();
        let consumer = self.pipeline.report(); save(&self.directory.join("consumer.json"), &consumer)?;
        let header = self.shared.header(); let closed = header.closed.load(Ordering::Acquire) != 0;
        let enters = if closed { Some(unsafe { read_volatile(&raw const header.enters) }.map(|value|value.to_string())) } else { None };
        let returns = if closed { Some(unsafe { read_volatile(&raw const header.returns) }.map(|value|value.to_string())) } else { None };
        let failed = header.error.load(Ordering::Acquire) != 0 || self.pipeline.failed();
        save(&self.directory.join("result.json"), &json!({"status":if failed {"CAPTURE_ERROR"} else if self.healthy_closed() {"CAPTURE_CLOSED"} else {"CAPTURE_UNCONFIRMED"},
            "native_error":reason(header.error.load(Ordering::Acquire) as u32),"producer_closed":closed,"callbacks":header.callbacks.load(Ordering::Acquire),
            "attempts":header.attempts.load(Ordering::Acquire).to_string(),"captures":header.captures.load(Ordering::Acquire).to_string(),"backpressure":header.backpressure.load(Ordering::Acquire).to_string(),
            "events_produced":header.produced.load(Ordering::Acquire).to_string(),"events_consumed":header.consumed.load(Ordering::Acquire).to_string(),
            "hook_enters":enters,"hook_returns":returns,"consumer":consumer,"runtime_acceptance":"NOT_RUN"}))?;
        save(&self.directory.join("restoration.json"), restoration)?; self.finished = true; Ok(())
    }
    pub fn healthy_closed(&self) -> bool {
        let header = self.shared.header(); let state = self.pipeline.lock();
        header.closed.load(Ordering::Acquire) != 0 && header.error.load(Ordering::Acquire) == 0 && header.callbacks.load(Ordering::Acquire) == 0 &&
            state.failure.is_none() && state.reader_done && state.files_flushed && state.held_items == 0 && state.batches_copied == state.batches_written &&
            state.events_copied == state.events_written && state.events_written == header.produced.load(Ordering::Acquire) &&
            header.attempts.load(Ordering::Acquire).checked_sub(header.backpressure.load(Ordering::Acquire)) == Some(state.batches_written) &&
            (0..3).all(|i|self.shared.batch(i).state.load(Ordering::Acquire) == 0)
    }
}
impl Drop for Mapping {
    fn drop(&mut self) {
        self.join_workers();
        if !self.finished {
            let _ = save(&self.directory.join("controller-failure.json"), &json!({"status":"UNCONFIRMED","reason":"controller exited before world finalization","consumer":self.pipeline.report(),"automatic_retry":false}));
        }
    }
}
