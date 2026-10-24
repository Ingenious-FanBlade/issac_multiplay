use std::fs::{self, File, OpenOptions};
use std::io::{BufReader, BufWriter, Seek, SeekFrom, Write};
use std::net::{Shutdown, SocketAddr, TcpStream, UdpSocket};
use std::os::windows::process::CommandExt;
use std::path::{Path, PathBuf};
use std::process::{Child, Command, Stdio};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{mpsc, Arc, Mutex};
use std::thread;
use std::time::{Duration, Instant};

use serde::Serialize;
use serde_json::json;
use windows::Win32::System::Threading::CREATE_NO_WINDOW;

use crate::error::{AppError, AppResult};
use crate::t09::authority_game::{AuthorityFinish, AuthorityGameDriver};
use crate::win32::{self, ObservedProcess};
use crate::t09::control::{
    self, ArmedBody, ControlType, Encoder, Envelope, ErrorBody, ReadyBody, Receiver, ReceiveSide,
    RetireCompleteBody, Role, StopCompleteBody,
};
use crate::t09::manifest::Manifest;
use crate::t09::wire::{
    self, WireMapping, WireStatus, WriterKind, CONTROLLER_ARMED_SENT,
    CONTROLLER_DRIVER_SPAWNED, CONTROLLER_READY_SENT, CONTROLLER_STOP_RECEIVED,
    CONTROLLER_STOPPED, CONTROLLER_UDP_READY, DRIVER_ESTABLISHED, DRIVER_FLAG_TARGET_REACHED,
    DRIVER_OPEN, DRIVER_READY, DRIVER_STOPPED,
};
use crate::t09::RUNNER_IDENTITY;

struct Options {
    role: Role,
    manifest: String,
    artifact_dir: String,
    control: SocketAddr,
    relay: SocketAddr,
    scenario: Option<String>,
    game_exe: Option<PathBuf>,
    native_dll: Option<PathBuf>,
    helper: Option<PathBuf>,
}

struct GameDriver {
    process: ObservedProcess,
    native_dll: PathBuf,
    helper: PathBuf,
    stdout_path: PathBuf,
    stderr_path: PathBuf,
}

struct DriverEnd {
    success: bool,
    description: String,
}

struct DriverProcess {
    scenario: Option<Child>,
    game: Option<GameDriver>,
    authority: Option<AuthorityGameDriver>,
}

impl DriverProcess {
    fn pid(&self) -> u32 {
        self.scenario
            .as_ref()
            .map(Child::id)
            .or_else(|| self.game.as_ref().map(|game| game.process.pid))
            .or_else(|| self.authority.as_ref().map(AuthorityGameDriver::pid))
            .unwrap_or(0)
    }

    fn creation_time_100ns(&self) -> u64 {
        self.game
            .as_ref()
            .map(|game| game.process.creation_time_100ns)
            .or_else(|| self.authority.as_ref().map(AuthorityGameDriver::creation_time_100ns))
            .unwrap_or(0)
    }

    fn is_game(&self) -> bool {
        self.game.is_some() || self.authority.is_some()
    }

    fn is_remote_game(&self) -> bool {
        self.game.is_some()
    }

    fn is_authority_game(&self) -> bool {
        self.authority.is_some()
    }

    fn dll_resident(&self) -> AppResult<Option<bool>> {
        let Some(game) = self.game.as_ref() else {
            return Ok(None);
        };
        let expected = game
            .native_dll
            .file_name()
            .map(|name| name.to_string_lossy().to_ascii_lowercase())
            .unwrap_or_default();
        let fresh = ObservedProcess::open_exact(game.process.pid, &game.process.image_path)?;
        Ok(Some(
            fresh
                .modules
                .iter()
                .any(|module| module.name.to_ascii_lowercase() == expected),
        ))
    }

    fn try_end(&mut self) -> AppResult<Option<DriverEnd>> {
        if let Some(child) = self.scenario.as_mut() {
            return child
                .try_wait()
                .map(|value| {
                    value.map(|status| DriverEnd {
                        success: status.success(),
                        description: format!("exit code {}", status.code().unwrap_or(-1)),
                    })
                })
                .map_err(|error| AppError::evidence(format!("query driver process: {error}")));
        }
        if let Some(game) = self.game.as_ref() {
            if game.process.wait(0)? {
                return Ok(Some(DriverEnd {
                    success: false,
                    description: "game process exited".to_owned(),
                }));
            }
        }
        if let Some(authority) = self.authority.as_ref() {
            if !authority.game_alive()? {
                return Ok(Some(DriverEnd {
                    success: false,
                    description: "authority game process exited".to_owned(),
                }));
            }
        }
        Ok(None)
    }

    fn game_alive(&self) -> AppResult<bool> {
        if let Some(game) = self.game.as_ref() {
            return Ok(!game.process.wait(0)?);
        }
        if let Some(authority) = self.authority.as_ref() {
            return authority.game_alive();
        }
        Ok(false)
    }

    fn request_driver_stop(&mut self, wire: &WireMapping, tick: u64) -> AppResult<bool> {
        wire.request_stop(tick);
        if let Some(authority) = self.authority.as_mut() {
            return authority.request_stop();
        }
        Ok(true)
    }

    fn authority_stop_success(&self) -> bool {
        self.authority
            .as_ref()
            .map(AuthorityGameDriver::stop_success)
            .unwrap_or(true)
    }

    fn pump_auxiliary(&mut self) -> AppResult<()> {
        if let Some(authority) = self.authority.as_mut() {
            authority.pump_auxiliary()?;
        }
        Ok(())
    }

    fn finish_authority(&mut self) -> AppResult<Option<AuthorityFinish>> {
        self.authority
            .as_mut()
            .map(AuthorityGameDriver::finish)
            .transpose()
    }

    fn stop_game_helper(&mut self) -> AppResult<bool> {
        let Some(game) = self.game.as_mut() else {
            return Ok(self.authority_stop_success());
        };
        let output = Command::new(&game.helper)
            .args([
                "--stop-t09-remote",
                &game.process.pid.to_string(),
                &game.native_dll.to_string_lossy(),
            ])
            .output()
            .map_err(|error| AppError::evidence(format!("run T09 remote stop helper: {error}")))?;
        append_output(&game.stdout_path, &output.stdout)?;
        append_output(&game.stderr_path, &output.stderr)?;
        Ok(output.status.success())
    }

    fn terminate_owned_scenario(&mut self) {
        if let Some(child) = self.scenario.as_mut() {
            let _ = child.kill();
            let _ = child.wait();
        }
    }
}

#[derive(Clone, Copy, Debug, Default)]
struct LatestAck {
    server_tick: u64,
    accepted: u64,
    processed: u64,
    state_tick: u64,
}

#[derive(Default, Debug)]
struct BridgeStats {
    outbound: u64,
    inbound: u64,
    udp_sent: u64,
    udp_received: u64,
    rejected: u64,
    late_dropped: u64,
}

struct Logs {
    endpoint: BufWriter<File>,
    control: BufWriter<File>,
}

impl Logs {
    fn event(&mut self, value: serde_json::Value) -> AppResult<()> {
        serde_json::to_writer(&mut self.endpoint, &value)
            .map_err(|error| AppError::evidence(format!("serialize endpoint event: {error}")))?;
        self.endpoint
            .write_all(b"\n")
            .map_err(|error| AppError::evidence(format!("write endpoint event: {error}")))
    }

    fn control(&mut self, direction: &str, envelope: &Envelope) -> AppResult<()> {
        serde_json::to_writer(
            &mut self.control,
            &json!({
                "direction":direction,
                "magic":envelope.magic,
                "version":envelope.version,
                "runId":envelope.run_id,
                "role":envelope.role,
                "seq":envelope.seq,
                "ack":envelope.ack,
                "type":control::control_type_name(envelope.kind),
                "body":envelope.body
            }),
        )
        .map_err(|error| AppError::evidence(format!("serialize control log: {error}")))?;
        self.control
            .write_all(b"\n")
            .map_err(|error| AppError::evidence(format!("write control log: {error}")))
    }

    fn flush(&mut self) -> AppResult<()> {
        self.endpoint
            .flush()
            .map_err(|error| AppError::evidence(format!("flush endpoint log: {error}")))?;
        self.control
            .flush()
            .map_err(|error| AppError::evidence(format!("flush control log: {error}")))
    }
}

pub fn run(arguments: &[String]) -> AppResult<()> {
    let options = parse_options(arguments)?;
    let manifest = Manifest::load(Path::new(&options.manifest))?;
    manifest.validate_f0()?;
    if manifest.script.last_tick < manifest.script.first_tick {
        return Err(AppError::usage("invalid script tick range"));
    }
    let artifact = PathBuf::from(&options.artifact_dir);
    fs::create_dir(&artifact).map_err(|error| {
        AppError::evidence(format!(
            "endpoint artifact directory must be new ({}): {error}",
            artifact.display()
        ))
    })?;
    let mapping_name = manifest.wire_mapping_name(options.role)?;
    let mapping = WireMapping::create(&mapping_name, options.role, &manifest)?;
    let mut logs = Logs {
        endpoint: BufWriter::with_capacity(
            128 * 1024,
            new_log(&artifact.join("endpoint.jsonl"))?,
        ),
        control: BufWriter::with_capacity(
            128 * 1024,
            new_log(&artifact.join("control.jsonl"))?,
        ),
    };
    logs.event(json!({
        "event":"mapping-created","identity":RUNNER_IDENTITY,"role":options.role,
        "mapping":mapping_name,"runId":manifest.run_id,"size":wire::WIRE_MAPPING_SIZE,
        "automaticRetry":false
    }))?;
    super::manifest::write_new_json(
        &artifact.join("wire-status-initial.json"),
        &wire_status_json(options.role, &mapping.status()),
    )?;
    let live_mode = options.scenario.is_none();
    let latest_ack = Arc::new(Mutex::new(LatestAck::default()));
    let stop_worker = Arc::new(AtomicBool::new(false));
    let bridge = {
        let mapping = Arc::clone(&mapping);
        let latest_ack = Arc::clone(&latest_ack);
        let stop = Arc::clone(&stop_worker);
        let artifact = artifact.clone();
        let role = options.role;
        let relay = options.relay;
        let run_id = manifest.run_id.clone();
        thread::spawn(move || {
            run_bridge(
                mapping,
                role,
                relay,
                artifact,
                run_id,
                latest_ack,
                stop,
                if live_mode { 3600 } else { 260 },
            )
        })
    };
    // Give the bridge a chance to bind/log. Any immediate failure is collected
    // before driver startup, but no retry is attempted.
    thread::sleep(Duration::from_millis(20));
    if let Some(error) = bridge_failure_hint(&artifact) {
        return Err(AppError::evidence(error));
    }
    let mut driver = spawn_driver(&options, &manifest, &artifact)?;
    mapping.set_driver_pid(driver.pid());
    mapping.set_controller_state(CONTROLLER_DRIVER_SPAWNED);
    logs.event(json!({
        "event":"driver-spawned","role":options.role,"pid":driver.pid(),
        "mode":if driver.is_game(){"game-remote"}else{"scenario"},
        "scenario":options.scenario,"gameExe":options.game_exe.as_ref().map(|path|path.file_name()),
        "tickIntervalMs":manifest.script.tick_interval_ms,
        "leadTicks":manifest.script.lead_ticks
    }))?;

    let control_stream = TcpStream::connect(options.control).map_err(|error| {
        AppError::evidence(format!("connect T9C1 control {}: {error}", options.control))
    })?;
    control_stream
        .set_nodelay(true)
        .map_err(|error| AppError::evidence(format!("set T9C1 nodelay: {error}")))?;
    let reader_stream = control_stream
        .try_clone()
        .map_err(|error| AppError::evidence(format!("clone T9C1 stream: {error}")))?;
    let (incoming_sender, incoming_receiver) = mpsc::channel();
    let _reader = thread::spawn(move || {
        let mut reader = BufReader::new(reader_stream);
        loop {
            match control::read_envelope(&mut reader) {
                Ok(Some(envelope)) => {
                    if incoming_sender.send(Ok(envelope)).is_err() {
                        break;
                    }
                }
                Ok(None) => {
                    let _ = incoming_sender.send(Err("T9C1 peer closed the connection".to_owned()));
                    break;
                }
                Err(error) => {
                    let _ = incoming_sender.send(Err(error.to_string()));
                    break;
                }
            }
        }
    });
    let mut writer = BufWriter::new(control_stream);
    let mut encoder = Encoder::new(options.role, manifest.run_id.clone())?;
    let mut receiver = Receiver::new();
    let mut ready_sent = false;
    let mut armed_sent = false;
    let mut stop_requested = false;
    let mut stop_forwarded_to_driver = false;
    let mut stop_activation_deadline: Option<Instant> = None;
    let mut last_heartbeat = 0u64;
    let mut last_ack_tick = 0u64;
    let mut last_state_tick = 0u64;
    let mut last_target_reached = false;
    let mut last_control_heartbeat = 0u64;
    let mut first_error: Option<String> = None;
    let run_deadline = Instant::now()
        + Duration::from_secs(if options.scenario.is_none() { 3600 } else { 240 });

    let operation = (|| -> AppResult<()> {
        wait_for_driver_open(&mapping, &mut driver, Duration::from_secs(30))?;
        logs.event(json!({
            "event":"driver-open","role":options.role,
            "status":wire_status_json(options.role,&mapping.status())
        }))?;
        loop {
            if mapping.status().driver_error != 0 {
                return Err(AppError::evidence(format!(
                    "driver reported error 0x{:08X}",
                    mapping.status().driver_error
                )));
            }
            if let Some(end) = driver.try_end()? {
                if !stop_requested || !end.success {
                    return Err(AppError::evidence(format!(
                        "driver ended before a successful STOP_REQUEST: {}",
                        end.description
                    )));
                }
            }
            let status = mapping.status();
            while let Ok(message) = incoming_receiver.try_recv() {
                let envelope = message.map_err(AppError::evidence)?;
                receiver.accept(
                    &envelope,
                    &manifest.run_id,
                    options.role,
                    encoder.next_sequence().saturating_sub(1),
                    ReceiveSide::Endpoint,
                )?;
                encoder.acknowledge(envelope.seq);
                logs.control("receive", &envelope)?;
                match envelope.kind {
                    ControlType::RunStart => {
                        if !armed_sent || stop_requested {
                            return Err(AppError::evidence(
                                "RUN_START arrived before ARMED or after stop",
                            ));
                        }
                        let body: crate::t09::control::RunStartBody =
                            control::parse_body(&envelope)?;
                        if body.script_id != manifest.script.id
                            || body.last_tick != manifest.script.last_tick
                        {
                            return Err(AppError::evidence(
                                "RUN_START does not match the frozen manifest script",
                            ));
                        }
                        mapping.set_run_start(body.tick0);
                        logs.event(json!({
                            "event":"run-start","role":options.role,
                            "runStartId":body.run_start_id,"tick0":body.tick0,
                            "lastTick":body.last_tick,"scriptId":body.script_id,
                            "elapsedMs":mapping.status().produced
                        }))?;
                    }
                    ControlType::TickHeartbeat => {
                        let body: crate::t09::control::HeartbeatBody =
                            control::parse_body(&envelope)?;
                        if options.role != Role::B || body.authority_tick <= last_control_heartbeat {
                            return Err(AppError::evidence(
                                "TICK_HEARTBEAT must be a new B-side authority tick observation",
                            ));
                        }
                        last_control_heartbeat = body.authority_tick;
                        mapping.set_controller_heartbeat_tick(body.authority_tick);
                        logs.event(json!({
                            "event":"control-heartbeat","role":options.role,
                            "authorityTick":body.authority_tick,
                            "accepted":body.accepted,
                            "processed":body.processed,
                            "targetReached":body.target_reached
                        }))?;
                    }
                    ControlType::StopRequest => {
                        stop_requested = true;
                        mapping.set_controller_state(CONTROLLER_STOP_RECEIVED);
                        if options.role == Role::A {
                            if !driver.request_driver_stop(&mapping, status.heartbeat_tick)? {
                                return Err(AppError::evidence(
                                    "Authority driver did not stop cleanly",
                                ));
                            }
                            stop_forwarded_to_driver = true;
                        } else {
                            // ACK/STATE and STOP_REQUEST travel on independent
                            // paths. Give the B bridge a bounded drain window
                            // before stopping the driver.
                            stop_activation_deadline =
                                Some(Instant::now() + Duration::from_millis(500));
                        }
                        logs.event(json!({
                            "event":"stop-requested","role":options.role,
                            "heartbeatTick":status.heartbeat_tick,
                            "body":envelope.body
                        }))?;
                    }
                    ControlType::Error => {
                        let body: ErrorBody = control::parse_body(&envelope)?;
                        return Err(AppError::evidence(format!(
                            "T9C1 peer error {}: {}",
                            body.code, body.message
                        )));
                    }
                    _ => return Err(AppError::evidence("unexpected T9C1 message for endpoint")),
                }
            }
            if !ready_sent && status.driver_state & DRIVER_OPEN != 0 {
                let body = ReadyBody {
                    phase: "ready".to_owned(),
                    script_id: manifest.script.id.clone(),
                    endpoint_state: status.driver_state,
                    artifact_writable: true,
                    game_pid: status.driver_pid,
                    creation_time_100ns: driver.creation_time_100ns(),
                    native_state: if driver.is_game() {
                        "game-remote-endpoint".to_owned()
                    } else {
                        "offline-scenario".to_owned()
                    },
                    pair_ready: status.driver_state & DRIVER_READY != 0,
                    observer_ready: status.driver_state & DRIVER_READY != 0,
                };
                send_control(
                    &mut writer,
                    &mut encoder,
                    ControlType::Ready,
                    &body,
                    &mut logs,
                )?;
                ready_sent = true;
                mapping.set_controller_state(CONTROLLER_READY_SENT);
                logs.event(json!({"event":"ready-sent","role":options.role}))?;
            }
            if ready_sent
                && !armed_sent
                && status.driver_state & DRIVER_ESTABLISHED != 0
                && status.driver_state & DRIVER_READY != 0
            {
                let body = ArmedBody {
                    endpoint_established: true,
                    pair_ready: true,
                    observer_ready: true,
                };
                send_control(
                    &mut writer,
                    &mut encoder,
                    ControlType::Armed,
                    &body,
                    &mut logs,
                )?;
                armed_sent = true;
                mapping.set_controller_state(CONTROLLER_ARMED_SENT);
                logs.event(json!({"event":"armed-sent","role":options.role}))?;
            }
            if options.role == Role::A {
                let published = *latest_ack
                    .lock()
                    .map_err(|_| AppError::evidence("latest publication mutex poisoned"))?;
                if published.server_tick < last_ack_tick
                    || published.state_tick < last_state_tick
                {
                    return Err(AppError::evidence(
                        "Authority ACK/STATE publication tick regressed",
                    ));
                }
                last_ack_tick = published.server_tick;
                last_state_tick = published.state_tick;
                let heartbeat = mapping.status().heartbeat_tick;
                if heartbeat < last_heartbeat {
                    return Err(AppError::evidence("driver heartbeat tick regressed"));
                }
                let published_tick = heartbeat
                    .min(published.server_tick)
                    .min(published.state_tick);
                let target_reached = status.driver_flags & DRIVER_FLAG_TARGET_REACHED != 0
                    && published_tick >= manifest.script.last_tick;
                if published_tick > last_heartbeat
                    || (target_reached && !last_target_reached)
                {
                    last_heartbeat = published_tick;
                    last_target_reached = target_reached;
                    let body = crate::t09::control::HeartbeatBody {
                        authority_tick: published_tick,
                        accepted: published.accepted,
                        processed: published.processed,
                        target_reached,
                    };
                    send_control(
                        &mut writer,
                        &mut encoder,
                        ControlType::TickHeartbeat,
                        &body,
                        &mut logs,
                    )?;
                }
            }
            if stop_requested && !stop_forwarded_to_driver {
                let published = *latest_ack
                    .lock()
                    .map_err(|_| AppError::evidence("latest publication mutex poisoned"))?;
                let lane_target = if last_control_heartbeat > 0 {
                    last_control_heartbeat
                } else {
                    manifest.script.last_tick
                };
                let lanes_caught_up = published.server_tick >= lane_target
                    && published.state_tick >= lane_target;
                let expired = stop_activation_deadline
                    .map(|deadline| Instant::now() >= deadline)
                    .unwrap_or(true);
                if lanes_caught_up || expired {
                    if !driver.request_driver_stop(
                        &mapping,
                        mapping.status().heartbeat_tick,
                    )? {
                        return Err(AppError::evidence(
                            "Remote driver did not stop cleanly",
                        ));
                    }
                    stop_forwarded_to_driver = true;
                    let remaining_ms = stop_activation_deadline
                        .map(|deadline| {
                            deadline
                                .saturating_duration_since(Instant::now())
                                .as_millis()
                        })
                        .unwrap_or(0);
                    logs.event(json!({
                        "event":"stop-forwarded-to-driver","role":options.role,
                        "lanesCaughtUp":lanes_caught_up,
                        "deadlineRemainingMs":remaining_ms,
                        "latestAckTick":published.server_tick,
                        "latestStateTick":published.state_tick
                    }))?;
                }
            }
            if stop_requested && stop_forwarded_to_driver {
                let status = mapping.status();
                if status.driver_state & DRIVER_STOPPED != 0 {
                    break;
                }
                if let Some(end) = driver.try_end()? {
                    if end.success && mapping.status().driver_state & DRIVER_STOPPED != 0 {
                        break;
                    }
                    return Err(AppError::evidence(format!(
                        "driver stopped without the required STOPPED state: {}",
                        end.description
                    )));
                }
            }
            if Instant::now() > run_deadline {
                return Err(AppError::timeout("endpoint controller exceeded 240 seconds"));
            }
            driver.pump_auxiliary()?;
            logs.flush()?;
            thread::sleep(Duration::from_millis(5));
        }
        Ok(())
    })();

    if let Err(error) = operation {
        first_error = Some(error.to_string());
        mapping.set_controller_error(0xE100);
        if !stop_requested {
            // A failing driver still receives exactly one normal stop request.
            // This is cleanup, not a retry of the failed run.
            if !driver.request_driver_stop(&mapping, mapping.status().heartbeat_tick)? {
                first_error = Some(format!(
                    "{}; driver cleanup stop also failed",
                    first_error.unwrap_or_else(|| "endpoint failure".to_owned())
                ));
            }
            stop_requested = true;
        }
    }
    let mut helper_stop_success = true;
    if stop_requested {
        if driver.is_remote_game() || driver.is_authority_game() {
            // The Native worker owns the stopped state. Give it one bounded chance
            // to finish; the remote helper shutdown is called exactly once here.
            let deadline = Instant::now() + Duration::from_secs(30);
            while Instant::now() < deadline
                && mapping.status().driver_state & DRIVER_STOPPED == 0
            {
                if let Some(end) = driver.try_end()? {
                    if first_error.is_none() {
                        first_error = Some(format!(
                            "game process ended before driver stop: {}",
                            end.description
                        ));
                    }
                    break;
                }
                thread::sleep(Duration::from_millis(10));
            }
            if driver.is_remote_game() {
                helper_stop_success = driver.stop_game_helper()?;
            } else {
                helper_stop_success = driver.authority_stop_success();
            }
            if !helper_stop_success && first_error.is_none() {
                first_error = Some(
                    "T09 helper stop did not report a clean worker stop".to_owned(),
                );
            }
        } else {
            // Ensure the owned offline scenario has exited. The timeout is
            // recorded; only this owned non-game process may be terminated.
            let deadline = Instant::now() + Duration::from_secs(30);
            while Instant::now() < deadline {
                if driver.try_end()?.is_some() {
                    break;
                }
                thread::sleep(Duration::from_millis(10));
            }
            if driver.try_end()?.is_none() {
                driver.terminate_owned_scenario();
                if first_error.is_none() {
                    first_error = Some(
                        "offline driver exceeded stop deadline; owned scenario terminated"
                            .to_owned(),
                    );
                }
            }
        }
    }
    stop_worker.store(true, Ordering::Release);
    let bridge_result = bridge
        .join()
        .map_err(|_| AppError::evidence("endpoint bridge thread panicked"))?;
    if let Err(error) = bridge_result {
        if first_error.is_none() {
            first_error = Some(error.to_string());
        }
    }
    // Drain no further: the bridge performed the final quiet-window drain before
    // close. Capture status and close the mapping so old slots cannot be revived.
    let final_status = mapping.status();
    logs.event(json!({
        "event":"bridge-stopped","role":options.role,
        "status":wire_status_json(options.role,&final_status)
    }))?;
    mapping.close();
    mapping.set_controller_state(CONTROLLER_STOPPED);
    let ring_overwritten = final_status.overwritten[0] + final_status.overwritten[1];
    let game_process_alive = if driver.is_game() {
        driver.game_alive()?
    } else {
        false
    };
    let native_dll_resident = driver.dll_resident()?;
    let authority_finish = driver.finish_authority()?;
    if let Some(summary) = authority_finish.as_ref() {
        if !summary.clean() && first_error.is_none() {
            first_error = Some(format!(
                "Authority restoration incomplete: {}",
                serde_json::to_string(summary)
                    .unwrap_or_else(|_| "serialize failure".to_owned())
            ));
        }
    }
    let driver_exited = if driver.is_game() {
        helper_stop_success
            && final_status.driver_state & DRIVER_STOPPED != 0
            && game_process_alive
    } else {
        driver.try_end()?.is_some()
    };
    let endpoint_retired = final_status.driver_state & DRIVER_STOPPED != 0
        && final_status.overwritten[0] == 0
        && final_status.overwritten[1] == 0
        && final_status.controller_error == 0;
    if stop_requested && !endpoint_retired && first_error.is_none() {
        first_error = Some("driver did not reach the stopped state before retirement".to_owned());
    }
    if stop_requested && first_error.is_none() {
        let stop_body = StopCompleteBody {
            status: "stopped".to_owned(),
            endpoint_retired,
            driver_exited,
            ring_overwritten,
        };
        if let Err(error) = send_control(
            &mut writer,
            &mut encoder,
            ControlType::StopComplete,
            &stop_body,
            &mut logs,
        ) {
            first_error = Some(format!("send STOP_COMPLETE after local cleanup: {error}"));
        }
        if first_error.is_none() {
            let retire_body = RetireCompleteBody {
                status: "retired".to_owned(),
                artifact_closed: true,
                process_alive: if driver.is_game() { game_process_alive } else { true },
            };
            if let Err(error) = send_control(
                &mut writer,
                &mut encoder,
                ControlType::RetireComplete,
                &retire_body,
                &mut logs,
            ) {
                first_error = Some(format!("send RETIRE_COMPLETE after local cleanup: {error}"));
            }
        }
    }
    if let Some(error) = &first_error {
        let body = ErrorBody {
            code: "T09B_ENDPOINT_FAILURE".to_owned(),
            message: error.chars().take(4096).collect(),
            tick: final_status.heartbeat_tick,
        };
        let _ = send_control(
            &mut writer,
            &mut encoder,
            ControlType::Error,
            &body,
            &mut logs,
        );
    }
    if let Err(error) = logs.flush() {
        if first_error.is_none() {
            first_error = Some(format!("flush endpoint logs after local cleanup: {error}"));
        }
    }
    // Preserve the final STOP_COMPLETE/RETIRE_COMPLETE bytes on the TCP stream.
    // A process close without an explicit write shutdown can surface as a
    // Windows 10054 reset before the orchestrator drains the last two messages.
    if let Err(error) = writer.flush() {
        if first_error.is_none() {
            first_error = Some(format!("flush final T9C1 control: {error}"));
        }
    }
    if let Err(error) = writer.get_ref().shutdown(Shutdown::Write) {
        if first_error.is_none() {
            first_error = Some(format!("shutdown final T9C1 write side: {error}"));
        }
    }
    thread::sleep(Duration::from_millis(100));
    let report = json!({
        "identity":RUNNER_IDENTITY,
        "status":if first_error.is_none() && stop_requested {"PASS"} else {"FAIL"},
        "role":options.role,
        "runId":manifest.run_id,
        "mapping":mapping_name,
        "automaticRetry":false,
        "readySent":ready_sent,
        "armedSent":armed_sent,
        "stopRequested":stop_requested,
        "driverExited":driver_exited,
        "gameProcessAlive":if driver.is_game() {Some(game_process_alive)} else {None},
        "dllResident":native_dll_resident,
        "authorityRestoration":authority_finish,
        "endpointRetired":endpoint_retired,
        "ringOverwritten":ring_overwritten,
        "finalStatus":wire_status_json(options.role,&final_status),
        "error":first_error
    });
    super::manifest::write_new_json(&artifact.join("result.json"), &report)?;
    if report["status"] != "PASS" {
        Err(AppError::evidence(
            first_error.unwrap_or_else(|| "endpoint controller did not pass".to_owned()),
        ))
    } else {
        Ok(())
    }
}

fn run_bridge(
    mapping: Arc<WireMapping>,
    role: Role,
    relay: SocketAddr,
    artifact: PathBuf,
    run_id: String,
    latest_ack: Arc<Mutex<LatestAck>>,
    stop: Arc<AtomicBool>,
    max_runtime_seconds: u64,
) -> AppResult<BridgeStats> {
    let socket = UdpSocket::bind("0.0.0.0:0")
        .map_err(|error| AppError::evidence(format!("bind endpoint UDP socket: {error}")))?;
    socket
        .connect(relay)
        .map_err(|error| AppError::evidence(format!("connect relay UDP {relay}: {error}")))?;
    socket
        .set_nonblocking(true)
        .map_err(|error| AppError::evidence(format!("set endpoint UDP nonblocking: {error}")))?;
    let local = socket
        .local_addr()
        .map_err(|error| AppError::evidence(format!("endpoint UDP local address: {error}")))?;
    mapping.set_controller_state(CONTROLLER_UDP_READY);
    let mut wire_log = BufWriter::with_capacity(256 * 1024, new_log(&artifact.join("wire.jsonl"))?);
    let mut udp_log = BufWriter::with_capacity(256 * 1024, new_log(&artifact.join("udp.jsonl"))?);
    let mut stats = BridgeStats::default();
    let started = Instant::now();
    let mut stop_seen: Option<Instant> = None;
    let mut quiet_since = Instant::now();
    let mut buffer = [0u8; 65535];
    loop {
        if stop.load(Ordering::Acquire) && stop_seen.is_none() {
            stop_seen = Some(Instant::now());
        }
        let mut activity = false;
        for _ in 0..256 {
            let Some((packet, generation, tick)) = mapping.read_packet(
                role,
                WriterKind::Controller,
                WireMapping::outbound_direction(role),
            )? else {
                break;
            };
            activity = true;
            stats.outbound = stats.outbound.saturating_add(1);
            let prefix = packet_prefix(&packet);
            if let Some(prefix) = prefix.as_ref() {
                update_latest_publication(&latest_ack, prefix);
            }
            write_json_line(
                &mut wire_log,
                &json!({
                    "direction":"outbound","role":role,
                    "generation":generation,"tick":tick,"length":packet.len(),
                    "prefix":prefix,"elapsedUs":started.elapsed().as_micros(),
                    "raw":hex::encode(&packet)
                }),
            )?;
            let sent = socket.send(&packet);
            match sent {
                Ok(length) if length == packet.len() => {
                    stats.udp_sent = stats.udp_sent.saturating_add(1);
                    write_json_line(
                        &mut udp_log,
                        &json!({
                            "direction":"send","role":role,"length":length,
                            "prefix":prefix,"elapsedUs":started.elapsed().as_micros()
                        }),
                    )?;
                }
                Ok(length) => {
                    return Err(AppError::evidence(format!(
                        "endpoint UDP short send: expected {}, sent {length}",
                        packet.len()
                    )));
                }
                Err(error) => {
                    return Err(AppError::evidence(format!("endpoint UDP send: {error}")));
                }
            }
        }
        for _ in 0..256 {
            match socket.recv(&mut buffer) {
                Ok(length) => {
                    activity = true;
                    stats.udp_received = stats.udp_received.saturating_add(1);
                    if !(64..=1200).contains(&length) {
                        write_json_line(
                            &mut udp_log,
                            &json!({
                                "direction":"receive-rejected","role":role,
                                "reason":"length","length":length,
                                "elapsedUs":started.elapsed().as_micros()
                            }),
                        )?;
                        return Err(AppError::evidence(format!(
                            "endpoint received datagram length {length}; expected one 64..=1200 byte ICT8 packet"
                        )));
                    }
                    let packet = buffer[..length].to_vec();
                    let prefix = packet_prefix(&packet);
                    if let Some(prefix) = prefix.as_ref() {
                        update_latest_publication(&latest_ack, prefix);
                    }
                    write_json_line(
                        &mut udp_log,
                        &json!({
                            "direction":"receive","role":role,"length":length,
                            "prefix":prefix,"elapsedUs":started.elapsed().as_micros()
                        }),
                    )?;
                    if mapping.closed() != 0 {
                        stats.late_dropped = stats.late_dropped.saturating_add(1);
                        write_json_line(
                            &mut udp_log,
                            &json!({
                                "direction":"receive-after-close","role":role,
                                "length":length,"prefix":prefix,
                                "elapsedUs":started.elapsed().as_micros()
                            }),
                        )?;
                        continue;
                    }
                    if let Err(error) = mapping.write_packet(
                        role,
                        WriterKind::Controller,
                        WireMapping::inbound_direction(role),
                        &packet,
                        0,
                        0,
                    ) {
                        if mapping.closed() != 0 {
                            stats.late_dropped = stats.late_dropped.saturating_add(1);
                            write_json_line(
                                &mut udp_log,
                                &json!({
                                    "direction":"receive-after-close","role":role,
                                    "length":length,"prefix":prefix,
                                    "error":error.to_string(),
                                    "elapsedUs":started.elapsed().as_micros()
                                }),
                            )?;
                            continue;
                        }
                        return Err(error);
                    }
                    stats.inbound = stats.inbound.saturating_add(1);
                    write_json_line(
                        &mut wire_log,
                        &json!({
                            "direction":"inbound","role":role,
                            "generation":0,"tick":0,"length":packet.len(),
                            "prefix":prefix,"elapsedUs":started.elapsed().as_micros(),
                            "raw":hex::encode(&packet)
                        }),
                    )?;
                }
                Err(error) if error.kind() == std::io::ErrorKind::WouldBlock => break,
                Err(error) => {
                    return Err(AppError::evidence(format!("endpoint UDP receive: {error}")));
                }
            }
        }
        if activity {
            quiet_since = Instant::now();
        }
        if let Some(stop_at) = stop_seen {
            let quiet = quiet_since.elapsed() >= Duration::from_millis(25);
            let deadline = stop_at.elapsed() >= Duration::from_millis(500);
            if quiet || deadline {
                break;
            }
        }
        if started.elapsed() > Duration::from_secs(max_runtime_seconds) {
            return Err(AppError::timeout(format!(
                "endpoint bridge exceeded {max_runtime_seconds} seconds"
            )));
        }
        wire_log
            .flush()
            .map_err(|error| AppError::evidence(format!("flush wire log: {error}")))?;
        udp_log
            .flush()
            .map_err(|error| AppError::evidence(format!("flush UDP log: {error}")))?;
        thread::sleep(Duration::from_millis(1));
    }
    wire_log
        .flush()
        .map_err(|error| AppError::evidence(format!("final wire log flush: {error}")))?;
    udp_log
        .flush()
        .map_err(|error| AppError::evidence(format!("final UDP log flush: {error}")))?;
    super::manifest::write_new_json(
        &artifact.join("bridge.json"),
        &json!({
            "identity":RUNNER_IDENTITY,
            "role":role,
            "runId":run_id,
            "localUdp":local.to_string(),
            "relay":relay.to_string(),
            "outbound":stats.outbound,
            "inbound":stats.inbound,
            "udpSent":stats.udp_sent,
            "udpReceived":stats.udp_received,
            "rejected":stats.rejected,
            "lateDroppedAfterClose":stats.late_dropped,
            "wireOverwritten":mapping.status().overwritten,
            "closed":mapping.closed()
        }),
    )?;
    Ok(stats)
}

fn spawn_driver(
    options: &Options,
    manifest: &Manifest,
    artifact: &Path,
) -> AppResult<DriverProcess> {
    let stdout_path = artifact.join("driver.stdout.log");
    let stderr_path = artifact.join("driver.stderr.log");
    let stdout = new_log(&stdout_path)?;
    let stderr = new_log(&stderr_path)?;
    if let Some(scenario) = options.scenario.as_ref() {
        let events = artifact.join("driver-events.jsonl");
        if events.exists() {
            return Err(AppError::evidence("driver event file already exists"));
        }
        let identity = manifest.wire_identity()?;
        let mut command = Command::new(scenario);
        command
            .arg("--role")
            .arg(options.role.as_str())
            .arg("--mapping")
            .arg(manifest.wire_mapping_name(options.role)?)
            .arg("--events")
            .arg(&events)
            .arg("--build")
            .arg(&manifest.build_label)
            .arg("--session")
            .arg(hex::encode(identity.session))
            .arg("--run-nonce")
            .arg(hex::encode(identity.run_nonce))
            .arg("--authority-nonce")
            .arg(hex::encode(identity.authority_nonce))
            .arg("--remote-nonce")
            .arg(hex::encode(identity.remote_nonce))
            .arg("--client-a")
            .arg(manifest.wire.clients[0].to_string())
            .arg("--client-b")
            .arg(manifest.wire.clients[1].to_string())
            .arg("--tick-interval-ms")
            .arg(manifest.script.tick_interval_ms.to_string())
            .arg("--lead-ticks")
            .arg(manifest.script.lead_ticks.to_string())
            .stdin(Stdio::null())
            .stdout(Stdio::from(stdout))
            .stderr(Stdio::from(stderr))
            .creation_flags(CREATE_NO_WINDOW.0);
        let child = command
            .spawn()
            .map_err(|error| AppError::evidence(format!("spawn endpoint driver: {error}")))?;
        return Ok(DriverProcess {
            scenario: Some(child),
            game: None,
            authority: None,
        });
    }

    let game_exe = match options.game_exe.as_ref() {
        Some(path) => path.clone(),
        None => win32::unique_target_image_path()?,
    };
    let native_dll = options
        .native_dll
        .as_ref()
        .ok_or_else(|| AppError::usage("game mode requires --native-dll"))?;
    let helper = options
        .helper
        .as_ref()
        .ok_or_else(|| AppError::usage("game mode requires --helper"))?;
    drop(stdout);
    drop(stderr);
    let pids = win32::find_target_pids()?;
    if pids.len() != 1 {
        return Err(AppError::evidence(format!(
            "game mode requires exactly one running isaac-ng.exe; observed {pids:?}"
        )));
    }
    let process = ObservedProcess::open_exact(pids[0], &game_exe)?;
    if process.debugger_present()? || !process.forbidden_modules().is_empty() {
        return Err(AppError::evidence(
            "debugger/conflicting module present; no T09 injection performed",
        ));
    }
    if options.role == Role::A {
        let authority = AuthorityGameDriver::start(
            process,
            game_exe.clone(),
            native_dll.clone(),
            helper.clone(),
            manifest,
            artifact,
            &manifest.wire_mapping_name(Role::A)?,
        )?;
        return Ok(DriverProcess {
            scenario: None,
            game: None,
            authority: Some(authority),
        });
    }
    let mapping_name = manifest.wire_mapping_name(Role::B)?;
    let injection = Command::new(helper)
        .args([
            "--inject-t09-remote",
            &process.pid.to_string(),
            &native_dll.to_string_lossy(),
            &mapping_name,
        ])
        .output()
        .map_err(|error| AppError::evidence(format!("run T09 remote inject helper: {error}")))?;
    append_output(&stdout_path, &injection.stdout)?;
    append_output(&stderr_path, &injection.stderr)?;
    if !injection.status.success() {
        return Err(AppError::evidence(format!(
            "T09 remote injection helper failed: {}",
            injection.status
        )));
    }
    Ok(DriverProcess {
        scenario: None,
        game: Some(GameDriver {
            process,
            native_dll: native_dll.clone(),
            helper: helper.clone(),
            stdout_path,
            stderr_path,
        }),
        authority: None,
    })
}

fn wait_for_driver_open(
    mapping: &WireMapping,
    driver: &mut DriverProcess,
    timeout: Duration,
) -> AppResult<()> {
    let deadline = Instant::now() + timeout;
    while Instant::now() < deadline {
        let status = mapping.status();
        if status.driver_error != 0 {
            return Err(AppError::evidence(format!(
                "driver error 0x{:08X} while opening mapping",
                status.driver_error
            )));
        }
        if status.driver_state & DRIVER_OPEN != 0 {
            return Ok(());
        }
        if let Some(end) = driver.try_end()? {
            return Err(AppError::evidence(format!(
                "driver ended before opening Wire IPC: {}",
                end.description
            )));
        }
        thread::sleep(Duration::from_millis(5));
    }
    Err(AppError::timeout("driver did not open Wire IPC within 30 seconds"))
}

fn send_control<T: serde::Serialize>(
    writer: &mut impl Write,
    encoder: &mut Encoder,
    kind: ControlType,
    body: &T,
    logs: &mut Logs,
) -> AppResult<()> {
    let encoded = encoder.encode(kind, body)?;
    let envelope: Envelope = serde_json::from_slice(
        encoded
            .strip_suffix(b"\n")
            .ok_or_else(|| AppError::evidence("encoded control line lacks newline"))?,
    )
    .map_err(|error| AppError::evidence(format!("decode encoded control envelope: {error}")))?;
    control::write_encoded(writer, &encoded)?;
    logs.control("send", &envelope)
}

fn update_latest_publication(latest: &Arc<Mutex<LatestAck>>, prefix: &PacketPrefix) {
    if let Ok(mut value) = latest.lock() {
        if prefix.kind == 3 {
            value.server_tick = value.server_tick.max(prefix.server_tick);
            value.accepted = value.accepted.max(prefix.accepted);
            value.processed = value.processed.max(prefix.processed);
        } else if prefix.kind == 4 {
            value.state_tick = value.state_tick.max(prefix.state_tick);
        }
    }
}

fn wire_status_json(role: Role, status: &WireStatus) -> serde_json::Value {
    json!({
        "role":role,
        "runState":status.run_state,
        "driverState":status.driver_state,
        "driverError":status.driver_error,
        "controllerError":status.controller_error,
        "produced":status.produced,
        "consumed":status.consumed,
        "overwritten":status.overwritten,
        "heartbeatTick":status.heartbeat_tick,
        "runStartTick":status.run_start_tick,
        "stopTick":status.stop_tick,
        "localGeneration":status.local_generation,
        "targetTick":status.target_tick,
        "driverPid":status.driver_pid,
        "driverFlags":status.driver_flags,
        "closed":status.closed,
        "controllerState":status.controller_state,
        "controllerHeartbeatTick":status.controller_heartbeat_tick
    })
}

#[derive(Debug, Serialize)]
struct PacketPrefix {
    kind: u16,
    role: u16,
    sender: u64,
    sequence: u64,
    payload_size: u32,
    server_tick: u64,
    accepted: u64,
    processed: u64,
    state_tick: u64,
}

fn packet_prefix(packet: &[u8]) -> Option<PacketPrefix> {
    if packet.len() < 64 || &packet[..4] != b"ICT8" {
        return None;
    }
    if u16::from_le_bytes([packet[4], packet[5]]) != 2
        || u16::from_le_bytes([packet[8], packet[9]]) != 64
    {
        return None;
    }
    let kind = u16::from_le_bytes([packet[6], packet[7]]);
    let role = u16::from_le_bytes([packet[10], packet[11]]);
    let sender = u64::from_le_bytes(packet[32..40].try_into().ok()?);
    let sequence = u64::from_le_bytes(packet[40..48].try_into().ok()?);
    let payload_size = u32::from_le_bytes(packet[48..52].try_into().ok()?);
    if payload_size as usize != packet.len().saturating_sub(64) {
        return None;
    }
    let (server_tick, accepted, processed) = if kind == 3 && packet.len() >= 104 {
        (
            u64::from_le_bytes(packet[64..72].try_into().ok()?),
            u64::from_le_bytes(packet[80..88].try_into().ok()?),
            u64::from_le_bytes(packet[88..96].try_into().ok()?),
        )
    } else {
        (0, 0, 0)
    };
    let state_tick = if kind == 4 && packet.len() >= 104 {
        u64::from_le_bytes(packet[96..104].try_into().ok()?)
    } else {
        0
    };
    Some(PacketPrefix {
        kind,
        role,
        sender,
        sequence,
        payload_size,
        server_tick,
        accepted,
        processed,
        state_tick,
    })
}

fn new_log(path: &Path) -> AppResult<File> {
    OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(path)
        .map_err(|error| AppError::evidence(format!("create {}: {error}", path.display())))
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

fn write_json_line(writer: &mut impl Write, value: &serde_json::Value) -> AppResult<()> {
    serde_json::to_writer(&mut *writer, value)
        .map_err(|error| AppError::evidence(format!("serialize JSONL: {error}")))?;
    writer
        .write_all(b"\n")
        .map_err(|error| AppError::evidence(format!("write JSONL: {error}")))
}

fn bridge_failure_hint(_artifact: &Path) -> Option<String> {
    // The bridge reports through its JoinHandle. This helper intentionally does
    // not probe files or infer success; it exists to document that no early
    // "ready" file is used as a barrier.
    None
}

fn parse_options(arguments: &[String]) -> AppResult<Options> {
    let mut role = None;
    let mut manifest = None;
    let mut artifact_dir = None;
    let mut control = None;
    let mut relay = None;
    let mut scenario = None;
    let mut game_exe = None;
    let mut native_dll = None;
    let mut helper = None;
    let mut index = 0;
    while index < arguments.len() {
        let key = &arguments[index];
        let value = arguments
            .get(index + 1)
            .ok_or_else(|| AppError::usage(format!("endpoint option {key} requires a value")))?;
        match key.as_str() {
            "--role" => {
                role = Some(match value.as_str() {
                    "A" => Role::A,
                    "B" => Role::B,
                    _ => return Err(AppError::usage("endpoint --role must be A or B")),
                });
            }
            "--manifest" => manifest = Some(value.clone()),
            "--artifact-dir" => artifact_dir = Some(value.clone()),
            "--control" => {
                control = Some(value.parse::<SocketAddr>().map_err(|error| {
                    AppError::usage(format!("endpoint --control '{value}': {error}"))
                })?)
            }
            "--relay" => {
                relay = Some(value.parse::<SocketAddr>().map_err(|error| {
                    AppError::usage(format!("endpoint --relay '{value}': {error}"))
                })?)
            }
            "--scenario" => scenario = Some(value.clone()),
            "--game-exe" => game_exe = Some(PathBuf::from(value)),
            "--native-dll" => native_dll = Some(PathBuf::from(value)),
            "--helper" => helper = Some(PathBuf::from(value)),
            _ => return Err(AppError::usage(format!("unknown endpoint option {key}"))),
        }
        index += 2;
    }
    let scenario_mode = scenario.is_some();
    let game_mode = game_exe.is_some() || native_dll.is_some() || helper.is_some();
    if scenario_mode == game_mode {
        return Err(AppError::usage(
            "endpoint requires exactly one driver mode: --scenario OR --native-dll/--helper (--game-exe optional)",
        ));
    }
    if game_mode && (native_dll.is_none() || helper.is_none()) {
        return Err(AppError::usage(
            "game mode requires --native-dll and --helper; --game-exe may be auto-discovered",
        ));
    }
    Ok(Options {
        role: role.ok_or_else(|| AppError::usage("endpoint requires --role"))?,
        manifest: manifest.ok_or_else(|| AppError::usage("endpoint requires --manifest"))?,
        artifact_dir: artifact_dir
            .ok_or_else(|| AppError::usage("endpoint requires --artifact-dir"))?,
        control: control.ok_or_else(|| AppError::usage("endpoint requires --control"))?,
        relay: relay.ok_or_else(|| AppError::usage("endpoint requires --relay"))?,
        scenario,
        game_exe,
        native_dll,
        helper,
    })
}
