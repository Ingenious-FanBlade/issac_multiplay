use std::cmp::Ordering as CmpOrdering;
use std::collections::BinaryHeap;
use std::fs::{self, File, OpenOptions};
use std::io::{BufWriter, Write};
use std::net::{SocketAddr, UdpSocket};
use std::path::{Path, PathBuf};
use std::time::{Duration, Instant};

use serde_json::json;

use crate::error::{AppError, AppResult};
use crate::t09::manifest::{parse_socket_addr, Manifest};
use crate::t09::RUNNER_IDENTITY;

const MAX_PENDING_BOOTSTRAP: usize = 256;
const MAX_DATAGRAM: usize = 1200;
const MIN_DATAGRAM: usize = 64;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum Face {
    A,
    B,
}

impl Face {
    const fn peer(self) -> Self {
        match self {
            Self::A => Self::B,
            Self::B => Self::A,
        }
    }

    const fn label(self) -> &'static str {
        match self {
            Self::A => "A",
            Self::B => "B",
        }
    }

    const fn direction(self) -> &'static str {
        match self {
            Self::A => "A_TO_B",
            Self::B => "B_TO_A",
        }
    }

    const fn seed_tag(self) -> u64 {
        match self {
            Self::A => 0x41_2d_54_4f_42,
            Self::B => 0x42_2d_54_4f_41,
        }
    }
}

#[derive(Debug)]
struct PacketPrefix {
    kind: u16,
    role: u16,
    sender: u64,
    sequence: u64,
    payload_size: u32,
    authoritative_bootstrap: bool,
}

#[derive(Debug)]
struct Scheduled {
    due: Instant,
    order: u64,
    destination: Face,
    raw: Vec<u8>,
    ordinal: u64,
    decision: &'static str,
    copy: u32,
}

impl Eq for Scheduled {}

impl PartialEq for Scheduled {
    fn eq(&self, other: &Self) -> bool {
        self.due == other.due && self.order == other.order
    }
}

impl Ord for Scheduled {
    fn cmp(&self, other: &Self) -> CmpOrdering {
        // Reverse ordering so BinaryHeap::pop returns the earliest due item.
        other
            .due
            .cmp(&self.due)
            .then_with(|| other.order.cmp(&self.order))
    }
}

impl PartialOrd for Scheduled {
    fn partial_cmp(&self, other: &Self) -> Option<CmpOrdering> {
        Some(self.cmp(other))
    }
}

struct Rng(u64);

impl Rng {
    fn next(&mut self) -> u64 {
        self.0 ^= self.0 << 13;
        self.0 ^= self.0 >> 7;
        self.0 ^= self.0 << 17;
        self.0
    }

    fn micros(&mut self, maximum_inclusive: u64) -> u64 {
        if maximum_inclusive == 0 {
            0
        } else {
            self.next() % maximum_inclusive.saturating_add(1)
        }
    }

    fn ppm(&mut self, ppm: u32) -> bool {
        ppm != 0 && self.next() % 1_000_000 < u64::from(ppm)
    }
}

struct RelayState {
    manifest: Manifest,
    sockets: [UdpSocket; 2],
    peers: [Option<SocketAddr>; 2],
    bootstrap_pending: [Vec<Vec<u8>>; 2],
    scheduled: BinaryHeap<Scheduled>,
    rng: [Rng; 2],
    ordinal: [u64; 2],
    order: u64,
    started: Instant,
    log: BufWriter<File>,
    counters: RelayCounters,
}

#[derive(Default, Debug)]
struct RelayCounters {
    received: [u64; 2],
    forwarded: [u64; 2],
    dropped: [u64; 2],
    duplicates: [u64; 2],
    rejected: u64,
    bootstrap_cached: u64,
    send_errors: u64,
}

impl RelayState {
    fn log(&mut self, value: serde_json::Value) -> AppResult<()> {
        serde_json::to_writer(&mut self.log, &value)
            .map_err(|error| AppError::evidence(format!("serialize relay log: {error}")))?;
        self.log
            .write_all(b"\n")
            .map_err(|error| AppError::evidence(format!("write relay log: {error}")))
    }

    fn receive(&mut self, face: Face, raw: Vec<u8>, now: Instant) -> AppResult<()> {
        let index = face_index(face);
        self.counters.received[index] = self.counters.received[index].saturating_add(1);
        if !(MIN_DATAGRAM..=MAX_DATAGRAM).contains(&raw.len()) {
            self.counters.rejected = self.counters.rejected.saturating_add(1);
            self.log(json!({
                "event":"reject","face":face.label(),"direction":face.direction(),
                "reason":"length","length":raw.len(),"elapsedUs":elapsed_micros(self.started,now)
            }))?;
            return Ok(());
        }
        let prefix = match parse_prefix(&raw) {
            Ok(prefix) => prefix,
            Err(reason) => {
                self.counters.rejected = self.counters.rejected.saturating_add(1);
                self.log(json!({
                    "event":"reject","face":face.label(),"direction":face.direction(),
                    "reason":reason,"length":raw.len(),"elapsedUs":elapsed_micros(self.started,now)
                }))?;
                return Ok(());
            }
        };
        self.log(json!({
            "event":"receive","face":face.label(),"direction":face.direction(),
            "length":raw.len(),"kind":prefix.kind,"role":prefix.role,"sender":prefix.sender,
            "sequence":prefix.sequence,"payloadSize":prefix.payload_size,
            "bootstrap":prefix.authoritative_bootstrap,"elapsedUs":elapsed_micros(self.started,now),
            "raw":hex::encode(&raw)
        }))?;
        let destination = face.peer();
        if self.peers[face_index(destination)].is_none() {
            if self.bootstrap_pending[face_index(face)].len() >= MAX_PENDING_BOOTSTRAP {
                return Err(AppError::evidence(
                    "relay bootstrap peer-address cache exceeded its bound",
                ));
            }
            self.bootstrap_pending[face_index(face)].push(raw);
            self.counters.bootstrap_cached = self.counters.bootstrap_cached.saturating_add(1);
            self.log(json!({
                "event":"bootstrap-cache","face":face.label(),"direction":face.direction(),
                "length":prefix.payload_size,"queueLength":self.bootstrap_pending[face_index(face)].len()
            }))?;
            return Ok(());
        }
        self.decide_and_schedule(face, raw, prefix.authoritative_bootstrap, now)
    }

    fn learn_peer_and_flush(&mut self, face: Face, address: SocketAddr, now: Instant) -> AppResult<()> {
        if self.peers[face_index(face)] == Some(address) {
            return Ok(());
        }
        self.peers[face_index(face)] = Some(address);
        self.log(json!({
            "event":"peer-learned","face":face.label(),"address":address.to_string(),
            "elapsedUs":elapsed_micros(self.started,now)
        }))?;
        for source in [Face::A, Face::B] {
            if self.peers[face_index(source)].is_none()
                || self.peers[face_index(source.peer())].is_none()
            {
                continue;
            }
            let pending = std::mem::take(&mut self.bootstrap_pending[face_index(source)]);
            for raw in pending {
                let prefix = parse_prefix(&raw).map_err(|reason| {
                    AppError::evidence(format!("relay bootstrap packet prefix: {reason}"))
                })?;
                self.decide_and_schedule(source, raw, prefix.authoritative_bootstrap, now)?;
            }
        }
        Ok(())
    }

    fn decide_and_schedule(
        &mut self,
        source: Face,
        raw: Vec<u8>,
        bootstrap: bool,
        now: Instant,
    ) -> AppResult<()> {
        let index = face_index(source);
        let ordinal = {
            self.ordinal[index] = self.ordinal[index].saturating_add(1);
            self.ordinal[index]
        };
        let faults = self.manifest.relay.faults.clone();
        if bootstrap || !faults.active() {
            self.order = self.order.saturating_add(1);
            self.scheduled.push(Scheduled {
                due: now,
                order: self.order,
                destination: source.peer(),
                raw,
                ordinal,
                decision: if bootstrap {
                    "bootstrap-forward"
                } else {
                    "forward"
                },
                copy: 0,
            });
            self.log(json!({
                "event":"decision","face":source.label(),"direction":source.direction(),
                "ordinal":ordinal,"decision":if bootstrap {"bootstrap-forward"}else{"forward"},
                "bootstrap":bootstrap,"elapsedUs":elapsed_micros(self.started,now)
            }))?;
            return Ok(());
        }
        let elapsed_ms = now.duration_since(self.started).as_millis() as u64;
        let outage = faults.outage_ms != 0
            && elapsed_ms >= faults.outage_after_ms
            && elapsed_ms < faults.outage_after_ms.saturating_add(faults.outage_ms);
        if outage || self.rng[index].ppm(faults.loss_ppm) {
            self.counters.dropped[index] = self.counters.dropped[index].saturating_add(1);
            self.log(json!({
                "event":"decision","face":source.label(),"direction":source.direction(),
                "ordinal":ordinal,"decision":if outage{"outage-drop"}else{"loss-drop"},
                "elapsedUs":elapsed_micros(self.started,now)
            }))?;
            return Ok(());
        }
        let duplicate = self.rng[index].ppm(faults.duplicate_ppm);
        let jitter_us = self.rng[index].micros(u64::from(faults.jitter_ms) * 1000);
        let reorder = self.rng[index].ppm(faults.reorder_ppm);
        let base_us = u64::from(faults.base_delay_ms) * 1000;
        let reorder_us = if reorder {
            u64::from(faults.reorder_extra_ms) * 1000
        } else {
            0
        };
        let copies = if duplicate { 2 } else { 1 };
        if duplicate {
            self.counters.duplicates[index] = self.counters.duplicates[index].saturating_add(1);
        }
        for copy in 0..copies {
            self.order = self.order.saturating_add(1);
            let due = now
                .checked_add(Duration::from_micros(base_us + jitter_us + reorder_us + (copy as u64) * 1000))
                .ok_or_else(|| AppError::evidence("relay due time overflow"))?;
            self.scheduled.push(Scheduled {
                due,
                order: self.order,
                destination: source.peer(),
                raw: raw.clone(),
                ordinal,
                decision: if duplicate {
                    "duplicate"
                } else if reorder {
                    "reorder"
                } else {
                    "forward"
                },
                copy,
            });
        }
        self.log(json!({
            "event":"decision","face":source.label(),"direction":source.direction(),
            "ordinal":ordinal,"decision":if duplicate{"duplicate"}else if reorder{"reorder"}else{"forward"},
            "copies":copies,"jitterUs":jitter_us,"reorder":reorder,
            "elapsedUs":elapsed_micros(self.started,now)
        }))?;
        Ok(())
    }

    fn flush_due(&mut self, now: Instant) -> AppResult<()> {
        loop {
            let Some(next) = self.scheduled.peek() else {
                break;
            };
            if next.due > now {
                break;
            }
            let Some(item) = self.scheduled.pop() else {
                break;
            };
            let Some(address) = self.peers[face_index(item.destination)] else {
                return Err(AppError::evidence(
                    "relay scheduled a packet before destination peer was known",
                ));
            };
            let sent = self.sockets[face_index(item.destination)].send_to(&item.raw, address);
            match sent {
                Ok(length) if length == item.raw.len() => {
                    let source = item.destination.peer();
                    let index = face_index(source);
                    self.counters.forwarded[index] = self.counters.forwarded[index].saturating_add(1);
                    self.log(json!({
                        "event":"forward","to":item.destination.label(),
                        "direction":format!("{}_TO_{}",source.label(),item.destination.label()),
                        "ordinal":item.ordinal,"copy":item.copy,"decision":item.decision,
                        "length":length,"dueElapsedUs":elapsed_micros(self.started,item.due),
                        "elapsedUs":elapsed_micros(self.started,now)
                    }))?;
                }
                Ok(length) => {
                    self.counters.send_errors = self.counters.send_errors.saturating_add(1);
                    return Err(AppError::evidence(format!(
                        "relay UDP short send: expected {}, sent {length}",
                        item.raw.len()
                    )));
                }
                Err(error) => {
                    self.counters.send_errors = self.counters.send_errors.saturating_add(1);
                    return Err(AppError::evidence(format!("relay UDP send failed: {error}")));
                }
            }
        }
        Ok(())
    }
}

pub fn run(arguments: &[String]) -> AppResult<()> {
    let options = parse_options(arguments)?;
    let manifest = Manifest::load(Path::new(&options.manifest))?;
    let artifact = PathBuf::from(&options.artifact_dir);
    fs::create_dir(&artifact).map_err(|error| {
        AppError::evidence(format!(
            "relay artifact directory must be new ({}): {error}",
            artifact.display()
        ))
    })?;
    let stop_file = PathBuf::from(&options.stop_file);
    if stop_file.exists() {
        return Err(AppError::usage("relay --stop-file must not already exist"));
    }
    let socket_a = bind_face(&manifest.relay.a.bind, "relay A")?;
    let socket_b = bind_face(&manifest.relay.b.bind, "relay B")?;
    socket_a.set_nonblocking(true).map_err(|error| {
        AppError::evidence(format!("set relay A socket nonblocking: {error}"))
    })?;
    socket_b.set_nonblocking(true).map_err(|error| {
        AppError::evidence(format!("set relay B socket nonblocking: {error}"))
    })?;
    let address_a = socket_a
        .local_addr()
        .map_err(|error| AppError::evidence(format!("relay A local address: {error}")))?;
    let address_b = socket_b
        .local_addr()
        .map_err(|error| AppError::evidence(format!("relay B local address: {error}")))?;
    super::manifest::write_new_json(
        &artifact.join("ready.json"),
        &json!({
            "identity":RUNNER_IDENTITY,
            "a":{"bind":address_a.to_string()},
            "b":{"bind":address_b.to_string()},
            "profile":manifest.relay.faults,
            "case":manifest.case
        }),
    )?;
    let log = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(artifact.join("relay.jsonl"))
        .map_err(|error| AppError::evidence(format!("create relay log: {error}")))?;
    let seed_a = manifest.relay.faults.seed ^ Face::A.seed_tag();
    let seed_b = manifest.relay.faults.seed ^ Face::B.seed_tag();
    let mut state = RelayState {
        manifest,
        sockets: [socket_a, socket_b],
        peers: [None, None],
        bootstrap_pending: [Vec::new(), Vec::new()],
        scheduled: BinaryHeap::new(),
        rng: [Rng(seed_a.max(1)), Rng(seed_b.max(1))],
        ordinal: [0, 0],
        order: 0,
        started: Instant::now(),
        log: BufWriter::with_capacity(256 * 1024, log),
        counters: RelayCounters::default(),
    };
    let started = state.started;
    let mut stop_requested_at: Option<Instant> = None;
    let mut failure: Option<String> = None;
    let mut buffer = [0u8; 65535];
    let result = (|| -> AppResult<()> {
        loop {
            let now = Instant::now();
            if stop_requested_at.is_none() && stop_file.exists() {
                stop_requested_at = Some(now);
                state.log(json!({
                    "event":"stop-requested",
                    "elapsedUs":elapsed_micros(started,now)
                }))?;
            }
            let mut datagrams = Vec::<(Face, Vec<u8>, SocketAddr)>::new();
            for face in [Face::A, Face::B] {
                let socket = &state.sockets[face_index(face)];
                for _ in 0..256 {
                    match socket.recv_from(&mut buffer) {
                        Ok((length, source)) => {
                            datagrams.push((face, buffer[..length].to_vec(), source));
                        }
                        Err(error) if error.kind() == std::io::ErrorKind::WouldBlock => break,
                        Err(error) => {
                            return Err(AppError::evidence(format!(
                                "relay {} receive failed: {error}",
                                face.label()
                            )));
                        }
                    }
                }
            }
            for (face, raw, source) in datagrams {
                state.learn_peer_and_flush(face, source, now)?;
                state.receive(face, raw, now)?;
            }
            state.flush_due(now)?;
            if let Some(stop_at) = stop_requested_at {
                let drain_deadline = stop_at + Duration::from_millis(500);
                if state.scheduled.is_empty() || now >= drain_deadline {
                    break;
                }
            }
            if now.duration_since(started) > Duration::from_secs(7200) {
                return Err(AppError::timeout("relay exceeded the 7200 second run bound"));
            }
            std::thread::sleep(Duration::from_millis(1));
        }
        state.log.flush().map_err(|error| {
            AppError::evidence(format!("flush relay log: {error}"))
        })?;
        Ok(())
    })();
    if let Err(error) = result {
        failure = Some(error.to_string());
    }
    if let Err(error) = state.log.flush() {
        if failure.is_none() {
            failure = Some(format!("final relay log flush: {error}"));
        }
    }
    let counters = &state.counters;
    let stopped = json!({
        "identity":RUNNER_IDENTITY,
        "status":if failure.is_none(){"STOPPED"}else{"FAIL"},
        "failure":failure,
        "stopRequested":stop_requested_at.is_some(),
        "runtimeMs":started.elapsed().as_millis(),
        "received":{"a":counters.received[0],"b":counters.received[1]},
        "forwarded":{"aToB":counters.forwarded[0],"bToA":counters.forwarded[1]},
        "dropped":{"aToB":counters.dropped[0],"bToA":counters.dropped[1]},
        "duplicates":{"aToB":counters.duplicates[0],"bToA":counters.duplicates[1]},
        "rejected":counters.rejected,
        "bootstrapCached":counters.bootstrap_cached,
        "sendErrors":counters.send_errors
    });
    super::manifest::write_new_json(&artifact.join("stopped.json"), &stopped)?;
    if let Some(message) = failure {
        Err(AppError::evidence(format!("relay failed: {message}")))
    } else {
        Ok(())
    }
}

struct Options {
    manifest: String,
    artifact_dir: String,
    stop_file: String,
}

fn parse_options(arguments: &[String]) -> AppResult<Options> {
    let mut manifest = None;
    let mut artifact_dir = None;
    let mut stop_file = None;
    let mut index = 0;
    while index < arguments.len() {
        let key = &arguments[index];
        let value = arguments
            .get(index + 1)
            .ok_or_else(|| AppError::usage(format!("relay option {key} requires a value")))?;
        match key.as_str() {
            "--manifest" => {
                if manifest.replace(value.clone()).is_some() {
                    return Err(AppError::usage("duplicate relay --manifest"));
                }
            }
            "--artifact-dir" => {
                if artifact_dir.replace(value.clone()).is_some() {
                    return Err(AppError::usage("duplicate relay --artifact-dir"));
                }
            }
            "--stop-file" => {
                if stop_file.replace(value.clone()).is_some() {
                    return Err(AppError::usage("duplicate relay --stop-file"));
                }
            }
            _ => return Err(AppError::usage(format!("unknown relay option {key}"))),
        }
        index += 2;
    }
    Ok(Options {
        manifest: manifest.ok_or_else(|| AppError::usage("relay requires --manifest"))?,
        artifact_dir: artifact_dir
            .ok_or_else(|| AppError::usage("relay requires --artifact-dir"))?,
        stop_file: stop_file.ok_or_else(|| AppError::usage("relay requires --stop-file"))?,
    })
}

fn bind_face(text: &str, label: &str) -> AppResult<UdpSocket> {
    let address = parse_socket_addr(text, label)?;
    UdpSocket::bind(address)
        .map_err(|error| AppError::evidence(format!("{label} bind {address}: {error}")))
}

fn face_index(face: Face) -> usize {
    match face {
        Face::A => 0,
        Face::B => 1,
    }
}

fn elapsed_micros(start: Instant, now: Instant) -> u64 {
    now.saturating_duration_since(start).as_micros() as u64
}

fn parse_prefix(raw: &[u8]) -> Result<PacketPrefix, &'static str> {
    if raw.len() < MIN_DATAGRAM || raw.len() > MAX_DATAGRAM {
        return Err("length");
    }
    if &raw[..4] != b"ICT8" {
        return Err("magic");
    }
    let version = u16::from_le_bytes([raw[4], raw[5]]);
    if version != 2 {
        return Err("wire-version");
    }
    if u16::from_le_bytes([raw[8], raw[9]]) != 64 {
        return Err("header-size");
    }
    let kind = u16::from_le_bytes([raw[6], raw[7]]);
    if !(1..=4).contains(&kind) {
        return Err("kind");
    }
    let role = u16::from_le_bytes([raw[10], raw[11]]);
    if role != 1 && role != 2 {
        return Err("role");
    }
    let payload_size = u32::from_le_bytes([raw[48], raw[49], raw[50], raw[51]]);
    if payload_size as usize != raw.len().saturating_sub(64) {
        return Err("payload-length");
    }
    let sender = u64::from_le_bytes(raw[32..40].try_into().map_err(|_| "sender")?);
    let sequence = u64::from_le_bytes(raw[40..48].try_into().map_err(|_| "sequence")?);
    if sender == 0 || sequence == 0 || sequence == u64::MAX {
        return Err("identity-sequence");
    }
    let bootstrap = kind == 1 || (kind == 3 && raw.len() >= 135 && raw[134] == 1);
    Ok(PacketPrefix {
        kind,
        role,
        sender,
        sequence,
        payload_size,
        authoritative_bootstrap: bootstrap,
    })
}
