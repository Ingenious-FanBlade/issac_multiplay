use std::fs::{self, File, OpenOptions};
use std::io::{BufReader, BufWriter, Write};
use std::net::{SocketAddr, TcpListener, TcpStream};
use std::os::windows::process::CommandExt;
use std::path::{Path, PathBuf};
use std::process::{Child, Command, Stdio};
use std::sync::mpsc;
use std::thread;
use std::time::{Duration, Instant};

use serde_json::json;
use windows::Win32::System::Threading::CREATE_NO_WINDOW;

use crate::error::{AppError, AppResult};
use crate::t09::control::{
    self, ArmedBody, ControlType, Encoder, Envelope, ErrorBody, HeartbeatBody, ReadyBody, Receiver,
    ReceiveSide, Role, RunStartBody, StopCompleteBody, StopRequestBody,
};
use crate::t09::manifest::Manifest;
use crate::t09::RUNNER_IDENTITY;

const CONNECT_TIMEOUT: Duration = Duration::from_secs(30);
const TOTAL_TIMEOUT: Duration = Duration::from_secs(180);

struct Options {
    manifest: String,
    artifact_dir: String,
    scenario: Option<String>,
    game_exe: Option<PathBuf>,
    native_dll: Option<PathBuf>,
    helper: Option<PathBuf>,
    vm_share: Option<PathBuf>,
    vm_guest_share: Option<String>,
    await_b_seconds: u64,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum PeerState {
    Connected,
    Ready,
    Armed,
    Stopping,
    Retired,
}

struct Peer {
    id: u64,
    role: Option<Role>,
    state: PeerState,
    writer: TcpStream,
    encoder: Option<Encoder>,
    receiver: Receiver,
    ready: Option<ReadyBody>,
    armed: Option<ArmedBody>,
    stop_complete: Option<StopCompleteBody>,
    last_heartbeat: u64,
}

enum PeerEvent {
    Message(u64, Result<Envelope, String>),
}

struct ControlLog(BufWriter<File>);

impl ControlLog {
    fn line(&mut self, value: &serde_json::Value) -> AppResult<()> {
        serde_json::to_writer(&mut self.0, value)
            .map_err(|error| AppError::evidence(format!("serialize control log: {error}")))?;
        self.0
            .write_all(b"\n")
            .map_err(|error| AppError::evidence(format!("write control log: {error}")))
    }

    fn envelope(&mut self, direction: &str, envelope: &Envelope) -> AppResult<()> {
        self.line(&json!({
            "direction":direction,
            "magic":envelope.magic,
            "version":envelope.version,
            "runId":envelope.run_id,
            "role":envelope.role,
            "seq":envelope.seq,
            "ack":envelope.ack,
            "type":control::control_type_name(envelope.kind),
            "body":envelope.body
        }))
    }

    fn flush(&mut self) -> AppResult<()> {
        self.0
            .flush()
            .map_err(|error| AppError::evidence(format!("flush control log: {error}")))
    }
}

pub fn run_f0(arguments: &[String]) -> AppResult<()> {
    run_f0_impl(arguments, false)
}

pub fn run_live_f0(arguments: &[String]) -> AppResult<()> {
    run_f0_impl(arguments, true)
}

fn run_f0_impl(arguments: &[String], live: bool) -> AppResult<()> {
    let options = parse_options(arguments)?;
    let manifest = Manifest::load(Path::new(&options.manifest))?;
    manifest.validate_f0()?;
    if live {
        if !manifest.user_preconditions.real_game_authorized {
            return Err(AppError::usage(
                "live-f0 requires user_preconditions.real_game_authorized=true",
            ));
        }
        if options.scenario.is_some()
            || options.native_dll.is_none()
            || options.helper.is_none()
            || options.vm_share.is_none()
        {
            return Err(AppError::usage(
                "live-f0 requires --native-dll --helper --vm-share and no --scenario",
            ));
        }
    } else if manifest.user_preconditions.real_game_authorized {
        return Err(AppError::usage(
            "T0-09-B offline runner refuses real_game_authorized=true; use live-f0 only after explicit authorization",
        ));
    }
    let artifact = PathBuf::from(&options.artifact_dir);
    fs::create_dir(&artifact).map_err(|error| {
        AppError::evidence(format!(
            "orchestrator artifact directory must be new ({}): {error}",
            artifact.display()
        ))
    })?;
    let control_dir = artifact.join("control");
    fs::create_dir(&control_dir)
        .map_err(|error| AppError::evidence(format!("create control directory: {error}")))?;
    super::manifest::write_new_json(
        &artifact.join("manifest.json"),
        &serde_json::to_value(&manifest)
            .map_err(|error| AppError::evidence(format!("serialize manifest copy: {error}")))?,
    )?;
    let scenario = options.scenario.as_ref().map(PathBuf::from);
    if !live {
        let scenario = scenario.as_ref().ok_or_else(|| {
            AppError::usage("offline f0 requires --scenario")
        })?;
        if !scenario.is_file() {
            return Err(AppError::usage(format!(
                "offline scenario executable does not exist: {}",
                scenario.display()
            )));
        }
    }
    let current_exe = std::env::current_exe()
        .map_err(|error| AppError::evidence(format!("current executable: {error}")))?;
    let relay_stop = control_dir.join("relay.stop");
    let relay_dir = artifact.join("relay");
    let relay_stdout = new_log(&control_dir.join("relay.stdout.log"))?;
    let relay_stderr = new_log(&control_dir.join("relay.stderr.log"))?;
    let mut relay = Command::new(&current_exe)
        .arg("relay")
        .arg("--manifest")
        .arg(&options.manifest)
        .arg("--artifact-dir")
        .arg(&relay_dir)
        .arg("--stop-file")
        .arg(&relay_stop)
        .stdin(Stdio::null())
        .stdout(Stdio::from(relay_stdout))
        .stderr(Stdio::from(relay_stderr))
        .creation_flags(CREATE_NO_WINDOW.0)
        .spawn()
        .map_err(|error| AppError::evidence(format!("spawn relay: {error}")))?;
    let relay_ready = wait_for_relay_ready(&relay_dir.join("ready.json"), &mut relay)?;
    let relay_a = parse_ready_address(&relay_ready, "a")?;
    let relay_b = parse_ready_address(&relay_ready, "b")?;
    let listener = TcpListener::bind(
        manifest
            .control
            .bind
            .parse::<SocketAddr>()
            .map_err(|error| AppError::usage(format!("control.bind: {error}")))?,
    )
    .map_err(|error| AppError::evidence(format!("bind T9C1 control listener: {error}")))?;
    listener
        .set_nonblocking(true)
        .map_err(|error| AppError::evidence(format!("set T9C1 listener nonblocking: {error}")))?;
    let control_address = listener
        .local_addr()
        .map_err(|error| AppError::evidence(format!("T9C1 local address: {error}")))?;
    super::manifest::write_new_json(
        &control_dir.join("ready.json"),
        &json!({
            "identity":RUNNER_IDENTITY,
            "control":control_address.to_string(),
            "relay":{"a":relay_a.to_string(),"b":relay_b.to_string()},
            "runId":manifest.run_id,
            "automaticRetry":false
        }),
    )?;
    let mut control_log = ControlLog(BufWriter::with_capacity(
        256 * 1024,
        new_log(&control_dir.join("control.jsonl"))?,
    ));
    let mut children = Vec::<(Role, Child)>::new();
    let mut vm_launch: Option<VmLaunch> = None;
    if live {
        let launch = prepare_vm_launch(
            &current_exe,
            &manifest,
            Path::new(&options.manifest),
            options.native_dll.as_ref().expect("validated native dll"),
            options.helper.as_ref().expect("validated helper"),
            options.vm_share.as_ref().expect("validated vm share"),
            options.vm_guest_share.as_deref(),
            control_address,
            relay_b,
        )?;
        println!("T09_LIVE_VM_SCRIPT={}", launch.script_host.display());
        println!("T09_LIVE_VM_GUEST_SCRIPT={}", launch.script_guest);
        println!("T09_LIVE_VM_ARTIFACT={}", launch.artifact_host.display());
        super::manifest::write_new_json(
            &control_dir.join("vm-launch.json"),
            &json!({
                "scriptHost":launch.script_host,
                "scriptGuest":launch.script_guest,
                "artifactHost":launch.artifact_host,
                "automaticRetry":false
            }),
        )?;
        vm_launch = Some(launch);
        children.push((
            Role::A,
            spawn_live_endpoint(
                &current_exe,
                Role::A,
                &options.manifest,
                &manifest,
                &artifact,
                control_address,
                relay_a,
                options.game_exe.as_ref(),
                options.native_dll.as_ref().expect("validated native dll"),
                options.helper.as_ref().expect("validated helper"),
                &control_dir,
            )?,
        ));
    } else {
        children.push((
            Role::A,
            spawn_endpoint(
                &current_exe,
                Role::A,
                &options.manifest,
                &manifest,
                &artifact,
                control_address,
                relay_a,
                scenario.as_ref().expect("validated scenario"),
                &control_dir,
            )?,
        ));
        children.push((
            Role::B,
            spawn_endpoint(
                &current_exe,
                Role::B,
                &options.manifest,
                &manifest,
                &artifact,
                control_address,
                relay_b,
                scenario.as_ref().expect("validated scenario"),
                &control_dir,
            )?,
        ));
    }
    let (event_sender, event_receiver) = mpsc::channel();
    let mut peers = Vec::<Peer>::new();
    let mut next_peer_id = 0u64;
    let connect_timeout = if live {
        Duration::from_secs(options.await_b_seconds.clamp(30, 900))
    } else {
        CONNECT_TIMEOUT
    };
    let connect_deadline = Instant::now() + connect_timeout;
    while peers.len() < 2 {
        match listener.accept() {
            Ok((stream, _address)) => {
                stream
                    .set_nonblocking(false)
                    .map_err(|error| AppError::evidence(format!("T9C1 accepted stream blocking mode: {error}")))?;
                stream
                    .set_nodelay(true)
                    .map_err(|error| AppError::evidence(format!("T9C1 nodelay: {error}")))?;
                let reader_stream = stream
                    .try_clone()
                    .map_err(|error| AppError::evidence(format!("clone T9C1 stream: {error}")))?;
                let peer_id = next_peer_id;
                next_peer_id += 1;
                let sender = event_sender.clone();
                thread::spawn(move || {
                    let mut reader = BufReader::new(reader_stream);
                    loop {
                        match control::read_envelope(&mut reader) {
                            Ok(Some(envelope)) => {
                                if sender.send(PeerEvent::Message(peer_id, Ok(envelope))).is_err() {
                                    break;
                                }
                            }
                            Ok(None) => {
                                let _ = sender.send(PeerEvent::Message(
                                    peer_id,
                                    Err("T9C1 endpoint closed the control connection".to_owned()),
                                ));
                                break;
                            }
                            Err(error) => {
                                let _ = sender.send(PeerEvent::Message(
                                    peer_id,
                                    Err(error.to_string()),
                                ));
                                break;
                            }
                        }
                    }
                });
                peers.push(Peer {
                    id: peer_id,
                    role: None,
                    state: PeerState::Connected,
                    writer: stream,
                    encoder: None,
                    receiver: Receiver::new(),
                    ready: None,
                    armed: None,
                    stop_complete: None,
                    last_heartbeat: 0,
                });
            }
            Err(error) if error.kind() == std::io::ErrorKind::WouldBlock => {
                if Instant::now() > connect_deadline {
                    stop_relay_once(&mut relay, &relay_stop);
                    return Err(AppError::timeout(format!(
                        "both endpoint controllers did not connect within {} seconds",
                        connect_timeout.as_secs()
                    )));
                }
                for (role, child) in children.iter_mut() {
                    if let Some(status) = child.try_wait().map_err(|error| {
                        AppError::evidence(format!("query endpoint during connect: {error}"))
                    })? {
                        stop_relay_once(&mut relay, &relay_stop);
                        return Err(AppError::evidence(format!(
                            "endpoint {role} exited before both control connections: {status}"
                        )));
                    }
                }
                thread::sleep(Duration::from_millis(5));
            }
            Err(error) => {
                stop_relay_once(&mut relay, &relay_stop);
                return Err(AppError::evidence(format!("accept T9C1 connection: {error}")));
            }
        }
    }
    let started = Instant::now();
    let total_timeout = if live {
        Duration::from_secs(1800)
    } else {
        TOTAL_TIMEOUT
    };
    let mut run_start_sent = false;
    let mut stop_sent = false;
    let mut target_reached = false;
    let mut target_reached_at: Option<Instant> = None;
    let mut first_failure: Option<String> = None;
    let mut all_retired = false;
    let mut child_exit_grace: [Option<Instant>; 2] = [None, None];
    let operation = (|| -> AppResult<()> {
        loop {
            let now = Instant::now();
            let mut heartbeat_to_b: Option<HeartbeatBody> = None;
            if now.duration_since(started) > total_timeout {
                return Err(AppError::timeout(format!(
                    "T0-09-B F0 orchestration exceeded {} seconds",
                    total_timeout.as_secs()
                )));
            }
            while let Ok(event) = event_receiver.try_recv() {
                let PeerEvent::Message(peer_id, message) = event;
                let peer_index = peers
                    .iter()
                    .position(|peer| peer.id == peer_id)
                    .ok_or_else(|| AppError::evidence("unknown T9C1 peer id"))?;
                let peer_state = peers[peer_index].state;
                let envelope = match message {
                    Ok(envelope) => envelope,
                    Err(error) if peer_state == PeerState::Retired => {
                        // A retired endpoint may close/reset its TCP stream after
                        // its final RETIRE_COMPLETE. That close must not turn the
                        // already completed retirement into a failure.
                        control_log.line(&json!({
                            "direction":"closed",
                            "peerId":peer_id,
                            "ignoredAfterRetire":true,
                            "error":error
                        }))?;
                        continue;
                    }
                    Err(error) => return Err(AppError::evidence(error)),
                };
                let duplicate_role = envelope.kind == ControlType::Ready
                    && peers
                        .iter()
                        .any(|other| other.id != peer_id && other.role == Some(envelope.role));
                if duplicate_role {
                    return Err(AppError::evidence(format!(
                        "T9C1 role {} connected more than once",
                        envelope.role
                    )));
                }
                let peer = &mut peers[peer_index];
                if peer.state == PeerState::Connected {
                    if envelope.kind != ControlType::Ready {
                        return Err(AppError::evidence(
                            "first T9C1 endpoint message must be READY",
                        ));
                    }
                    let role = envelope.role;
                    peer.role = Some(role);
                    peer.encoder = Some(Encoder::new(role, manifest.run_id.clone())?);
                }
                let role = peer
                    .role
                    .ok_or_else(|| AppError::evidence("T9C1 peer role is unresolved"))?;
                let our_last_sent = peer
                    .encoder
                    .as_ref()
                    .map_or(0, |encoder| encoder.next_sequence().saturating_sub(1));
                peer.receiver.accept(
                    &envelope,
                    &manifest.run_id,
                    role,
                    our_last_sent,
                    ReceiveSide::Orchestrator,
                )?;
                if let Some(encoder) = peer.encoder.as_mut() {
                    encoder.acknowledge(envelope.seq);
                }
                control_log.envelope("receive", &envelope)?;
                match envelope.kind {
                    ControlType::Ready => {
                        if peer.state != PeerState::Connected {
                            return Err(AppError::evidence("duplicate READY"));
                        }
                        let body: ReadyBody = control::parse_body(&envelope)?;
                        if body.script_id != manifest.script.id {
                            return Err(AppError::evidence(
                                "READY script id differs from manifest",
                            ));
                        }
                        peer.ready = Some(body);
                        peer.state = PeerState::Ready;
                    }
                    ControlType::Armed => {
                        if peer.state != PeerState::Ready {
                            return Err(AppError::evidence("ARMED before READY"));
                        }
                        let body: ArmedBody = control::parse_body(&envelope)?;
                        peer.armed = Some(body);
                        peer.state = PeerState::Armed;
                    }
                    ControlType::TickHeartbeat => {
                        if role != Role::A || peer.state != PeerState::Armed {
                            return Err(AppError::evidence(
                                "TICK_HEARTBEAT outside the A/ARMED state",
                            ));
                        }
                        let body: HeartbeatBody = control::parse_body(&envelope)?;
                        if body.authority_tick <= peer.last_heartbeat {
                            return Err(AppError::evidence(
                                "TICK_HEARTBEAT authority tick did not advance",
                            ));
                        }
                        if body.target_reached && body.authority_tick < manifest.script.last_tick {
                            return Err(AppError::evidence(
                                "target_reached heartbeat precedes manifest last_tick",
                            ));
                        }
                        peer.last_heartbeat = body.authority_tick;
                        if body.target_reached {
                            if !target_reached {
                                target_reached_at = Some(Instant::now());
                            }
                            target_reached = true;
                        }
                        heartbeat_to_b = Some(body);
                    }
                    ControlType::StopComplete => {
                        if peer.state != PeerState::Armed && peer.state != PeerState::Stopping {
                            return Err(AppError::evidence(
                                "STOP_COMPLETE outside ARMED/STOPPING",
                            ));
                        }
                        let body: StopCompleteBody = control::parse_body(&envelope)?;
                        peer.stop_complete = Some(body);
                        peer.state = PeerState::Stopping;
                    }
                    ControlType::RetireComplete => {
                        if peer.state != PeerState::Stopping {
                            return Err(AppError::evidence(
                                "RETIRE_COMPLETE before STOP_COMPLETE",
                            ));
                        }
                        peer.state = PeerState::Retired;
                    }
                    ControlType::Error => {
                        let body: ErrorBody = control::parse_body(&envelope)?;
                        return Err(AppError::evidence(format!(
                            "endpoint {} error {}: {}",
                            role, body.code, body.message
                        )));
                    }
                    _ => return Err(AppError::evidence("unexpected T9C1 message at orchestrator")),
                }
            }
            if let Some(body) = heartbeat_to_b {
                if run_start_sent {
                    let b_peer = peers
                        .iter_mut()
                        .find(|peer| peer.role == Some(Role::B))
                        .ok_or_else(|| AppError::evidence("B peer missing for heartbeat forward"))?;
                    send_to_peer(
                        b_peer,
                        ControlType::TickHeartbeat,
                        &body,
                        &mut control_log,
                    )?;
                }
            }
            if !run_start_sent
                && peers
                    .iter()
                    .all(|peer| peer.state == PeerState::Armed && peer.role.is_some())
            {
                for peer in &mut peers {
                    let body = RunStartBody {
                        run_start_id: format!("{}-start", manifest.run_id),
                        tick0: manifest.script.first_tick,
                        script_id: manifest.script.id.clone(),
                        fault_profile: manifest.relay.faults.clone(),
                        last_tick: manifest.script.last_tick,
                    };
                    send_to_peer(
                        peer,
                        ControlType::RunStart,
                        &body,
                        &mut control_log,
                    )?;
                }
                super::manifest::write_new_json(
                    &control_dir.join("run-start.json"),
                    &json!({
                        "runId":manifest.run_id,
                        "scriptId":manifest.script.id,
                        "tick0":manifest.script.first_tick,
                        "lastTick":manifest.script.last_tick,
                        "faultProfile":manifest.relay.faults,
                        "automaticRetry":false
                    }),
                )?;
                run_start_sent = true;
            }
            let target_settled = target_reached_at
                .map(|instant| instant.elapsed() >= Duration::from_millis(400))
                .unwrap_or(false);
            if run_start_sent && target_reached && target_settled && !stop_sent {
                for peer in &mut peers {
                    if peer.state == PeerState::Armed || peer.state == PeerState::Stopping {
                        let body = StopRequestBody {
                            last_tick: manifest.script.last_tick,
                            reason: "target_reached".to_owned(),
                        };
                        send_to_peer(
                            peer,
                            ControlType::StopRequest,
                            &body,
                            &mut control_log,
                        )?;
                    }
                }
                super::manifest::write_new_json(
                    &control_dir.join("stop.json"),
                    &json!({
                        "runId":manifest.run_id,
                        "lastTick":manifest.script.last_tick,
                        "reason":"target_reached",
                        "automaticRetry":false
                    }),
                )?;
                stop_sent = true;
            }
            if peers
                .iter()
                .all(|peer| peer.state == PeerState::Retired && peer.role.is_some())
            {
                all_retired = true;
                break;
            }
            for (role, child) in children.iter_mut() {
                if let Some(status) = child
                    .try_wait()
                    .map_err(|error| AppError::evidence(format!("query endpoint process: {error}")))?
                {
                    let role = *role;
                    let role_retired = peers.iter().any(|peer| {
                        peer.role == Some(role) && peer.state == PeerState::Retired
                    });
                    if role_retired {
                        continue;
                    }
                    if !status.success() {
                        return Err(AppError::evidence(format!(
                            "endpoint {role} controller exited unsuccessfully before retirement: {status}"
                        )));
                    }
                    // Exit code 0 is expected after the controller has flushed its
                    // final STOP/RETIRE lines. Reader delivery may still be in the
                    // channel; allow a bounded grace window for that drain.
                    let index = if role == Role::A { 0 } else { 1 };
                    let started = child_exit_grace[index].get_or_insert_with(Instant::now);
                    if started.elapsed() > Duration::from_secs(5) {
                        return Err(AppError::evidence(format!(
                            "endpoint {role} exited 0 but its retirement messages did not arrive within 5 seconds"
                        )));
                    }
                }
            }
            control_log.flush()?;
            thread::sleep(Duration::from_millis(5));
        }
        Ok(())
    })();
    if let Err(error) = operation {
        first_failure = Some(error.to_string());
        if !stop_sent {
            let _ = stop_peers_best_effort(&mut peers, &mut control_log, manifest.script.last_tick);
            stop_sent = true;
        }
    }
    let mut controller_exit = Vec::<serde_json::Value>::new();
    let cleanup_deadline = Instant::now() + Duration::from_secs(40);
    for (role, child) in children.iter_mut() {
        let role = role.as_str();
        let mut status = None;
        while Instant::now() < cleanup_deadline {
            match child.try_wait() {
                Ok(Some(value)) => {
                    status = Some(value);
                    break;
                }
                Ok(None) => thread::sleep(Duration::from_millis(10)),
                Err(error) => {
                    if first_failure.is_none() {
                        first_failure = Some(format!("{role} endpoint wait failed: {error}"));
                    }
                    break;
                }
            }
        }
        if status.is_none() {
            let _ = child.kill();
            let _ = child.wait();
            if first_failure.is_none() {
                first_failure = Some(format!(
                    "endpoint {role} exceeded stop deadline; owned controller terminated"
                ));
            }
        }
        controller_exit.push(json!({
            "role":role,
            "status":status.map(|value|value.code()),
            "forcedTermination":status.is_none()
        }));
    }
    let _ = control_log.flush();
    let relay_stop_written = write_stop_file(&relay_stop);
    if let Err(error) = relay_stop_written {
        if first_failure.is_none() {
            first_failure = Some(format!("write relay stop file: {error}"));
        }
    }
    let relay_exited = wait_process(&mut relay, Duration::from_secs(10));
    if !relay_exited {
        let _ = relay.kill();
        let _ = relay.wait();
        if first_failure.is_none() {
            first_failure = Some("relay exceeded stop deadline; owned relay terminated".to_owned());
        }
    }
    let mut vm_result: Option<serde_json::Value> = None;
    if live {
        if let Some(launch) = vm_launch.take() {
            let result_path = launch.artifact_host.join("result.json");
            let deadline = Instant::now() + Duration::from_secs(20);
            while !result_path.is_file() && Instant::now() < deadline {
                thread::sleep(Duration::from_millis(50));
            }
            if result_path.is_file() {
                match copy_tree_new(&launch.artifact_host, &artifact.join("vm")) {
                    Ok(()) => {
                        vm_result = fs::read(artifact.join("vm").join("result.json"))
                            .ok()
                            .and_then(|bytes| serde_json::from_slice(&bytes).ok());
                    }
                    Err(error) => {
                        if first_failure.is_none() {
                            first_failure = Some(format!("copy VM artifact: {error}"));
                        }
                    }
                }
            } else if first_failure.is_none() {
                first_failure = Some(
                    "VM controller did not write result.json to the shared artifact directory"
                        .to_owned(),
                );
            }
        }
    }
    let roles_retired = peers
        .iter()
        .filter(|peer| peer.state == PeerState::Retired)
        .count()
        == 2;
    let vm_controller_ok = !live
        || vm_result
            .as_ref()
            .is_some_and(|value| value.get("status") == Some(&json!("PASS")));
    let status = if first_failure.is_none()
        && all_retired
        && roles_retired
        && run_start_sent
        && stop_sent
        && vm_controller_ok
    {
        "PASS"
    } else {
        "FAIL"
    };
    let report = json!({
        "identity":RUNNER_IDENTITY,
        "tx":"tx-03-02",
        "scope":if live {"live-host-vm-two-game-F0-transport-p1"} else {"offline-two-process-F0-transport-p1"},
        "status":status,
        "runId":manifest.run_id,
        "case":manifest.case,
        "automaticRetry":false,
        "realGame":if live {"ATTEMPTED"} else {"NOT_RUN"},
        "liveHostVmP1":if live {Some(status == "PASS")} else {None},
        "runStartSent":run_start_sent,
        "stopSent":stop_sent,
        "targetReached":target_reached,
        "allRetired":all_retired,
        "rolesRetired":roles_retired,
        "control":control_address.to_string(),
        "relay":{"a":relay_a.to_string(),"b":relay_b.to_string(),"exited":relay_exited},
        "endpointProcesses":controller_exit,
        "peerStates":peers.iter().map(|peer|json!({
            "role":peer.role,
            "state":format!("{:?}",peer.state),
            "lastHeartbeat":peer.last_heartbeat,
            "ready":peer.ready,
            "armed":peer.armed,
            "stopComplete":peer.stop_complete
        })).collect::<Vec<_>>(),
        "vmController":vm_result,
        "error":first_failure
    });
    super::manifest::write_new_json(&artifact.join("orchestrator-result.json"), &report)?;
    if status == "PASS" {
        Ok(())
    } else {
        Err(AppError::evidence(format!(
            "offline F0 orchestration failed: {}",
            first_failure.unwrap_or_else(|| "unretired endpoint or barrier failure".to_owned())
        )))
    }
}

struct VmLaunch {
    script_host: PathBuf,
    script_guest: String,
    artifact_host: PathBuf,
}

#[allow(clippy::too_many_arguments)]
fn prepare_vm_launch(
    current_exe: &Path,
    manifest: &Manifest,
    manifest_input: &Path,
    native_dll: &Path,
    helper: &Path,
    vm_share: &Path,
    vm_guest_share: Option<&str>,
    control: SocketAddr,
    relay_b: SocketAddr,
) -> AppResult<VmLaunch> {
    let run_dir = vm_share.join(&manifest.run_id);
    fs::create_dir(&run_dir).map_err(|error| {
        AppError::evidence(format!(
            "VM launch directory must be new ({}): {error}",
            run_dir.display()
        ))
    })?;
    copy_new_file(current_exe, &run_dir.join("IsaacCsT0T09.exe"))?;
    copy_new_file(
        native_dll,
        &run_dir.join(
            native_dll
                .file_name()
                .ok_or_else(|| AppError::usage("native DLL path has no file name"))?,
        ),
    )?;
    copy_new_file(
        helper,
        &run_dir.join(
            helper
                .file_name()
                .ok_or_else(|| AppError::usage("helper path has no file name"))?,
        ),
    )?;
    copy_new_file(manifest_input, &run_dir.join("manifest.json"))?;
    fs::create_dir(run_dir.join("artifact"))
        .map_err(|error| AppError::evidence(format!("create VM artifact parent: {error}")))?;
    let script_host = run_dir.join("start-vm-endpoint.ps1");
    let script = format!(
        r#"$ErrorActionPreference='Stop'
$Root=Split-Path -Parent $MyInvocation.MyCommand.Path
New-Item -ItemType Directory -Force -Path (Join-Path $Root 'artifact') | Out-Null
& (Join-Path $Root 'IsaacCsT0T09.exe') endpoint `
  --role B `
  --manifest (Join-Path $Root 'manifest.json') `
  --artifact-dir (Join-Path $Root 'artifact\vm') `
  --control '{}' `
  --relay '{}' `
  --native-dll (Join-Path $Root '{}') `
  --helper (Join-Path $Root '{}')
exit $LASTEXITCODE
"#,
        control,
        relay_b,
        native_dll.file_name().unwrap().to_string_lossy(),
        helper.file_name().unwrap().to_string_lossy()
    );
    let mut script_file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(&script_host)
        .map_err(|error| AppError::evidence(format!("create VM launch script: {error}")))?;
    script_file
        .write_all(script.as_bytes())
        .map_err(|error| AppError::evidence(format!("write VM launch script: {error}")))?;
    let guest_root = match vm_guest_share {
        Some(value) => format!("{value}\\{}", manifest.run_id),
        None => format!("{}\\{}", vm_share.display(), manifest.run_id),
    };
    let script_guest = format!("{guest_root}\\start-vm-endpoint.ps1");
    let artifact_host = run_dir.join("artifact").join("vm");
    let instructions = format!(
        "T0-09-B live F0 VM launch
run_id={}
Host orchestration dir: {}
VM shared run dir: {}
VM script: {}
VM artifact returns to host: {}
Keep the same RDP session connected for the whole run. Start exactly one isaac-ng.exe in the VM at the confirmed baseline, then run the script once.
",
        manifest.run_id,
        run_dir.display(),
        guest_root,
        script_guest,
        artifact_host.display()
    );
    let mut instructions_file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(run_dir.join("VM-START-INSTRUCTIONS.txt"))
        .map_err(|error| AppError::evidence(format!("create VM instructions: {error}")))?;
    instructions_file
        .write_all(instructions.as_bytes())
        .map_err(|error| AppError::evidence(format!("write VM instructions: {error}")))?;
    Ok(VmLaunch {
        script_host,
        script_guest,
        artifact_host,
    })
}

fn copy_new_file(source: &Path, destination: &Path) -> AppResult<()> {
    if destination.exists() {
        return Err(AppError::evidence(format!(
            "VM launch destination already exists: {}",
            destination.display()
        )));
    }
    fs::copy(source, destination)
        .map_err(|error| AppError::evidence(format!("copy {}: {error}", source.display())))?;
    Ok(())
}

fn copy_tree_new(source: &Path, destination: &Path) -> AppResult<()> {
    fs::create_dir(destination).map_err(|error| {
        AppError::evidence(format!(
            "VM artifact destination must be new ({}): {error}",
            destination.display()
        ))
    })?;
    for entry in fs::read_dir(source)
        .map_err(|error| AppError::evidence(format!("read VM artifact directory: {error}")))?
    {
        let entry = entry.map_err(|error| AppError::evidence(format!("VM artifact entry: {error}")))?;
        let from = entry.path();
        let to = destination.join(entry.file_name());
        if from.is_dir() {
            copy_tree_new(&from, &to)?;
        } else {
            copy_new_file(&from, &to)?;
        }
    }
    Ok(())
}

#[allow(clippy::too_many_arguments)]
fn spawn_live_endpoint(
    current_exe: &Path,
    role: Role,
    manifest_path: &str,
    manifest: &Manifest,
    artifact: &Path,
    control: SocketAddr,
    relay: SocketAddr,
    game_exe: Option<&PathBuf>,
    native_dll: &Path,
    helper: &Path,
    control_dir: &Path,
) -> AppResult<Child> {
    let role_dir = super::manifest::artifact_subdir(artifact, manifest, role.as_str())?;
    let stdout = new_log(&control_dir.join(format!("endpoint-{}-stdout.log", role.as_str())))?;
    let stderr = new_log(&control_dir.join(format!("endpoint-{}-stderr.log", role.as_str())))?;
    let mut command = Command::new(current_exe);
    command
        .arg("endpoint")
        .arg("--role")
        .arg(role.as_str())
        .arg("--manifest")
        .arg(manifest_path)
        .arg("--artifact-dir")
        .arg(role_dir)
        .arg("--control")
        .arg(control.to_string())
        .arg("--relay")
        .arg(relay.to_string())
        .arg("--native-dll")
        .arg(native_dll)
        .arg("--helper")
        .arg(helper);
    if let Some(game_exe) = game_exe {
        command.arg("--game-exe").arg(game_exe);
    }
    command
        .stdin(Stdio::null())
        .stdout(Stdio::from(stdout))
        .stderr(Stdio::from(stderr))
        .creation_flags(CREATE_NO_WINDOW.0)
        .spawn()
        .map_err(|error| AppError::evidence(format!("spawn live endpoint {role}: {error}")))
}

#[allow(clippy::too_many_arguments)]
fn spawn_endpoint(
    current_exe: &Path,
    role: Role,
    manifest_path: &str,
    manifest: &Manifest,
    artifact: &Path,
    control: SocketAddr,
    relay: SocketAddr,
    scenario: &Path,
    control_dir: &Path,
) -> AppResult<Child> {
    let role_dir = super::manifest::artifact_subdir(artifact, manifest, role.as_str())?;
    let stdout = new_log(&control_dir.join(format!("endpoint-{}-stdout.log", role.as_str())))?;
    let stderr = new_log(&control_dir.join(format!("endpoint-{}-stderr.log", role.as_str())))?;
    Command::new(current_exe)
        .arg("endpoint")
        .arg("--role")
        .arg(role.as_str())
        .arg("--manifest")
        .arg(manifest_path)
        .arg("--artifact-dir")
        .arg(role_dir)
        .arg("--control")
        .arg(control.to_string())
        .arg("--relay")
        .arg(relay.to_string())
        .arg("--scenario")
        .arg(scenario)
        .stdin(Stdio::null())
        .stdout(Stdio::from(stdout))
        .stderr(Stdio::from(stderr))
        .creation_flags(CREATE_NO_WINDOW.0)
        .spawn()
        .map_err(|error| AppError::evidence(format!("spawn endpoint {role}: {error}")))
}

fn send_to_peer<T: serde::Serialize>(
    peer: &mut Peer,
    kind: ControlType,
    body: &T,
    control_log: &mut ControlLog,
) -> AppResult<()> {
    let encoder = peer
        .encoder
        .as_mut()
        .ok_or_else(|| AppError::evidence("cannot send before T9C1 role is known"))?;
    let encoded = encoder.encode(kind, body)?;
    let envelope: Envelope = serde_json::from_slice(
        encoded
            .strip_suffix(b"\n")
            .ok_or_else(|| AppError::evidence("encoded T9C1 line lacks newline"))?,
    )
    .map_err(|error| AppError::evidence(format!("decode outgoing T9C1: {error}")))?;
    control::write_encoded(&mut peer.writer, &encoded)?;
    control_log.envelope("send", &envelope)
}

fn stop_peers_best_effort(
    peers: &mut [Peer],
    control_log: &mut ControlLog,
    last_tick: u64,
) -> AppResult<()> {
    let body = StopRequestBody {
        last_tick,
        reason: "controller_error".to_owned(),
    };
    for peer in peers {
        if peer.encoder.is_some()
            && matches!(peer.state, PeerState::Ready | PeerState::Armed | PeerState::Stopping)
        {
            if let Err(error) = send_to_peer(peer, ControlType::StopRequest, &body, control_log) {
                eprintln!("best-effort STOP_REQUEST for {:?}: {error}", peer.role);
            }
        }
    }
    Ok(())
}

fn stop_relay_once(relay: &mut Child, stop_file: &Path) {
    let _ = write_stop_file(stop_file);
    if !wait_process(relay, Duration::from_secs(5)) {
        let _ = relay.kill();
        let _ = relay.wait();
    }
}

fn wait_for_relay_ready(path: &Path, relay: &mut Child) -> AppResult<serde_json::Value> {
    let deadline = Instant::now() + Duration::from_secs(15);
    while Instant::now() < deadline {
        if path.is_file() {
            let bytes = fs::read(path)
                .map_err(|error| AppError::evidence(format!("read relay ready: {error}")))?;
            return serde_json::from_slice(&bytes)
                .map_err(|error| AppError::evidence(format!("parse relay ready: {error}")));
        }
        if let Some(status) = relay
            .try_wait()
            .map_err(|error| AppError::evidence(format!("query relay startup: {error}")))?
        {
            return Err(AppError::evidence(format!(
                "relay exited before ready.json: {status}"
            )));
        }
        thread::sleep(Duration::from_millis(10));
    }
    Err(AppError::timeout("relay ready.json was not created within 15 seconds"))
}

fn parse_ready_address(value: &serde_json::Value, face: &str) -> AppResult<SocketAddr> {
    let text = value
        .get(face)
        .and_then(|face| face.get("bind"))
        .and_then(serde_json::Value::as_str)
        .ok_or_else(|| AppError::evidence(format!("relay ready.json lacks {face}.bind")))?;
    text.parse::<SocketAddr>()
        .map_err(|error| AppError::evidence(format!("relay {face}.bind '{text}': {error}")))
}

fn write_stop_file(path: &Path) -> AppResult<()> {
    let mut file = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .map_err(|error| AppError::evidence(format!("create relay stop file: {error}")))?;
    file.write_all(b"stop\n")
        .map_err(|error| AppError::evidence(format!("write relay stop file: {error}")))
}

fn wait_process(child: &mut Child, timeout: Duration) -> bool {
    let deadline = Instant::now() + timeout;
    while Instant::now() < deadline {
        match child.try_wait() {
            Ok(Some(_)) => return true,
            Ok(None) => thread::sleep(Duration::from_millis(10)),
            Err(_) => return false,
        }
    }
    false
}

fn new_log(path: &Path) -> AppResult<File> {
    OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .map_err(|error| AppError::evidence(format!("create {}: {error}", path.display())))
}

fn parse_options(arguments: &[String]) -> AppResult<Options> {
    let mut manifest = None;
    let mut artifact_dir = None;
    let mut scenario = None;
    let mut game_exe = None;
    let mut native_dll = None;
    let mut helper = None;
    let mut vm_share = None;
    let mut vm_guest_share = None;
    let mut await_b_seconds = 180u64;
    let mut index = 0;
    while index < arguments.len() {
        let key = &arguments[index];
        let value = arguments
            .get(index + 1)
            .ok_or_else(|| AppError::usage(format!("f0 option {key} requires a value")))?;
        match key.as_str() {
            "--manifest" => manifest = Some(value.clone()),
            "--artifact-dir" => artifact_dir = Some(value.clone()),
            "--scenario" => scenario = Some(value.clone()),
            "--game-exe" => game_exe = Some(PathBuf::from(value)),
            "--native-dll" => native_dll = Some(PathBuf::from(value)),
            "--helper" => helper = Some(PathBuf::from(value)),
            "--vm-share" => vm_share = Some(PathBuf::from(value)),
            "--vm-guest-share" => vm_guest_share = Some(value.clone()),
            "--await-b-seconds" => {
                await_b_seconds = value.parse::<u64>().map_err(|error| {
                    AppError::usage(format!("--await-b-seconds '{value}': {error}"))
                })?;
            }
            _ => return Err(AppError::usage(format!("unknown f0 option {key}"))),
        }
        index += 2;
    }
    Ok(Options {
        manifest: manifest.ok_or_else(|| AppError::usage("f0 requires --manifest"))?,
        artifact_dir: artifact_dir.ok_or_else(|| AppError::usage("f0 requires --artifact-dir"))?,
        scenario,
        game_exe,
        native_dll,
        helper,
        vm_share,
        vm_guest_share,
        await_b_seconds,
    })
}
