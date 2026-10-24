use std::fmt::{Display, Formatter};
use std::io::{self, BufRead, Write};

use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use serde_json::Value;

use crate::error::{AppError, AppResult};
use crate::t09::manifest::FaultProfile;

pub const CONTROL_MAGIC: &str = "T9C1";
pub const CONTROL_VERSION: u32 = 1;
pub const MAX_CONTROL_LINE: usize = 65_536;

#[derive(Clone, Copy, Debug, Deserialize, Eq, PartialEq, Serialize)]
pub enum Role {
    A,
    B,
}

impl Role {
    #[must_use]
    pub const fn as_str(self) -> &'static str {
        match self {
            Self::A => "A",
            Self::B => "B",
        }
    }

    #[must_use]
    pub const fn other(self) -> Self {
        match self {
            Self::A => Self::B,
            Self::B => Self::A,
        }
    }
}

impl Display for Role {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> std::fmt::Result {
        formatter.write_str(self.as_str())
    }
}

#[derive(Clone, Copy, Debug, Deserialize, Eq, PartialEq, Serialize)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
pub enum ControlType {
    Ready,
    Armed,
    RunStart,
    TickHeartbeat,
    ArmFaults,
    FaultTrigger,
    StopRequest,
    StopComplete,
    RetireComplete,
    Error,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct Envelope {
    pub magic: String,
    pub version: u32,
    pub run_id: String,
    pub role: Role,
    pub seq: u64,
    pub ack: u64,
    #[serde(rename = "type")]
    pub kind: ControlType,
    pub body: Value,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ReadyBody {
    pub phase: String,
    pub script_id: String,
    pub endpoint_state: u32,
    pub artifact_writable: bool,
    pub game_pid: u32,
    pub creation_time_100ns: u64,
    pub native_state: String,
    pub pair_ready: bool,
    pub observer_ready: bool,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ArmedBody {
    pub endpoint_established: bool,
    pub pair_ready: bool,
    pub observer_ready: bool,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RunStartBody {
    pub run_start_id: String,
    pub tick0: u64,
    pub script_id: String,
    pub fault_profile: FaultProfile,
    pub last_tick: u64,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct HeartbeatBody {
    pub authority_tick: u64,
    pub accepted: u64,
    pub processed: u64,
    pub target_reached: bool,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ArmFaultsBody {
    pub fault_profile: FaultProfile,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct FaultTriggerBody {
    pub outage_ms: u64,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StopRequestBody {
    pub last_tick: u64,
    pub reason: String,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct StopCompleteBody {
    pub status: String,
    pub endpoint_retired: bool,
    pub driver_exited: bool,
    pub ring_overwritten: u64,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct RetireCompleteBody {
    pub status: String,
    pub artifact_closed: bool,
    pub process_alive: bool,
}

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ErrorBody {
    pub code: String,
    pub message: String,
    pub tick: u64,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ReceiveSide {
    Orchestrator,
    Endpoint,
}

#[derive(Clone, Debug)]
pub struct Encoder {
    role: Role,
    run_id: String,
    next_sequence: u64,
    last_received: u64,
}

impl Encoder {
    pub fn new(role: Role, run_id: impl Into<String>) -> AppResult<Self> {
        let run_id = run_id.into();
        if run_id.is_empty() || run_id.len() > 128 {
            return Err(AppError::usage("control run_id must be 1..=128 bytes"));
        }
        Ok(Self {
            role,
            run_id,
            next_sequence: 1,
            last_received: 0,
        })
    }

    pub fn next_sequence(&self) -> u64 {
        self.next_sequence
    }

    pub fn acknowledge(&mut self, received_sequence: u64) {
        self.last_received = self.last_received.max(received_sequence);
    }

    pub fn encode<T: Serialize>(
        &mut self,
        kind: ControlType,
        body: &T,
    ) -> AppResult<Vec<u8>> {
        let body = serde_json::to_value(body)
            .map_err(|error| AppError::evidence(format!("serialize control body: {error}")))?;
        validate_body(kind, &body)?;
        let sequence = self.next_sequence;
        if sequence == 0 || sequence == u64::MAX {
            return Err(AppError::evidence("control sequence exhausted"));
        }
        let envelope = Envelope {
            magic: CONTROL_MAGIC.to_owned(),
            version: CONTROL_VERSION,
            run_id: self.run_id.clone(),
            role: self.role,
            seq: sequence,
            ack: self.last_received,
            kind,
            body,
        };
        self.next_sequence += 1;
        encode_line(&envelope)
    }
}

#[derive(Clone, Debug)]
pub struct Receiver {
    expected_sequence: u64,
    last_acknowledged: u64,
}

impl Receiver {
    #[must_use]
    pub const fn new() -> Self {
        Self {
            expected_sequence: 1,
            last_acknowledged: 0,
        }
    }

    pub fn expected_sequence(&self) -> u64 {
        self.expected_sequence
    }

    pub fn accept(
        &mut self,
        envelope: &Envelope,
        run_id: &str,
        peer_role: Role,
        our_last_sent: u64,
        side: ReceiveSide,
    ) -> AppResult<()> {
        validate_envelope(envelope, run_id, peer_role, side)?;
        if envelope.seq != self.expected_sequence {
            return Err(AppError::evidence(format!(
                "control {} expected seq {}, received {}",
                envelope.role, self.expected_sequence, envelope.seq
            )));
        }
        if envelope.ack > our_last_sent {
            return Err(AppError::evidence(format!(
                "control {} ack {} exceeds our sent seq {}",
                envelope.role, envelope.ack, our_last_sent
            )));
        }
        if envelope.ack < self.last_acknowledged {
            return Err(AppError::evidence(format!(
                "control {} ack regressed from {} to {}",
                envelope.role, self.last_acknowledged, envelope.ack
            )));
        }
        self.last_acknowledged = envelope.ack;
        self.expected_sequence = self
            .expected_sequence
            .checked_add(1)
            .ok_or_else(|| AppError::evidence("control expected sequence exhausted"))?;
        Ok(())
    }
}

pub fn parse_body<T: DeserializeOwned>(envelope: &Envelope) -> AppResult<T> {
    serde_json::from_value(envelope.body.clone())
        .map_err(|error| AppError::evidence(format!("control {} body: {error}", envelope.role)))
}

pub fn read_bounded_line<R: BufRead>(reader: &mut R) -> io::Result<Option<Vec<u8>>> {
    let mut line = Vec::new();
    loop {
        let buffer = reader.fill_buf()?;
        if buffer.is_empty() {
            if line.is_empty() {
                return Ok(None);
            }
            return Err(io::Error::new(
                io::ErrorKind::UnexpectedEof,
                "control line ended without newline",
            ));
        }
        if let Some(position) = buffer.iter().position(|byte| *byte == b'\n') {
            if line.len() + position > MAX_CONTROL_LINE {
                return Err(io::Error::new(
                    io::ErrorKind::InvalidData,
                    "control line exceeds 65536 bytes",
                ));
            }
            line.extend_from_slice(&buffer[..position]);
            reader.consume(position + 1);
            if line.last() == Some(&b'\r') {
                line.pop();
            }
            if line.is_empty() {
                return Err(io::Error::new(
                    io::ErrorKind::InvalidData,
                    "empty control line",
                ));
            }
            return Ok(Some(line));
        }
        if line.len() + buffer.len() > MAX_CONTROL_LINE {
            return Err(io::Error::new(
                io::ErrorKind::InvalidData,
                "control line exceeds 65536 bytes",
            ));
        }
        line.extend_from_slice(buffer);
        let consumed = buffer.len();
        reader.consume(consumed);
    }
}

pub fn read_envelope<R: BufRead>(reader: &mut R) -> AppResult<Option<Envelope>> {
    let Some(line) = read_bounded_line(reader).map_err(|error| {
        AppError::evidence(format!("read T9C1 control line: {error}"))
    })? else {
        return Ok(None);
    };
    let envelope = serde_json::from_slice(&line)
        .map_err(|error| AppError::evidence(format!("parse T9C1 JSONL: {error}")))?;
    Ok(Some(envelope))
}

pub fn write_encoded<W: Write>(writer: &mut W, encoded: &[u8]) -> AppResult<()> {
    writer
        .write_all(encoded)
        .and_then(|()| writer.flush())
        .map_err(|error| AppError::evidence(format!("write T9C1 control: {error}")))
}

fn encode_line(envelope: &Envelope) -> AppResult<Vec<u8>> {
    let mut text = serde_json::to_vec(envelope)
        .map_err(|error| AppError::evidence(format!("serialize T9C1: {error}")))?;
    if text.len() > MAX_CONTROL_LINE {
        return Err(AppError::evidence("encoded T9C1 line exceeds 65536 bytes"));
    }
    text.push(b'\n');
    Ok(text)
}

fn validate_envelope(
    envelope: &Envelope,
    run_id: &str,
    peer_role: Role,
    side: ReceiveSide,
) -> AppResult<()> {
    if envelope.magic != CONTROL_MAGIC || envelope.version != CONTROL_VERSION {
        return Err(AppError::evidence("T9C1 magic/version mismatch"));
    }
    if envelope.run_id != run_id {
        return Err(AppError::evidence(format!(
            "T9C1 run_id mismatch: expected {run_id}, received {}",
            envelope.run_id
        )));
    }
    if envelope.role != peer_role {
        return Err(AppError::evidence(format!(
            "T9C1 role mismatch: expected {peer_role}, received {}",
            envelope.role
        )));
    }
    if envelope.seq == 0 || envelope.seq == u64::MAX {
        return Err(AppError::evidence("T9C1 sequence must be 1..u64::MAX-1"));
    }
    let allowed = match side {
        ReceiveSide::Orchestrator => matches!(
            envelope.kind,
            ControlType::Ready
                | ControlType::Armed
                | ControlType::TickHeartbeat
                | ControlType::StopComplete
                | ControlType::RetireComplete
                | ControlType::Error
        ),
        ReceiveSide::Endpoint => matches!(
            envelope.kind,
            ControlType::RunStart
                | ControlType::TickHeartbeat
                | ControlType::ArmFaults
                | ControlType::FaultTrigger
                | ControlType::StopRequest
                | ControlType::Error
        ),
    };
    if !allowed {
        return Err(AppError::evidence(format!(
            "T9C1 {} is not allowed on this side",
            control_type_name(envelope.kind)
        )));
    }
    validate_body(envelope.kind, &envelope.body)
}

fn validate_body(kind: ControlType, body: &Value) -> AppResult<()> {
    match kind {
        ControlType::Ready => {
            let value: ReadyBody = parse_body_value(kind, body)?;
            if value.phase != "ready"
                || !valid_text(&value.script_id, 64)
                || !valid_text(&value.native_state, 32)
                || value.game_pid == 0
            {
                return Err(AppError::evidence("invalid READY body values"));
            }
        }
        ControlType::Armed => {
            let value: ArmedBody = parse_body_value(kind, body)?;
            if !value.endpoint_established || !value.pair_ready || !value.observer_ready {
                return Err(AppError::evidence(
                    "invalid ARMED body: all readiness flags must be true",
                ));
            }
        }
        ControlType::RunStart => {
            let value: RunStartBody = parse_body_value(kind, body)?;
            validate_run_start(&value)?;
        }
        ControlType::TickHeartbeat => {
            let value: HeartbeatBody = parse_body_value(kind, body)?;
            if value.accepted < value.processed {
                return Err(AppError::evidence(
                    "TICK_HEARTBEAT processed exceeds accepted",
                ));
            }
        }
        ControlType::ArmFaults => {
            let value: ArmFaultsBody = parse_body_value(kind, body)?;
            value.fault_profile.validate()?;
        }
        ControlType::FaultTrigger => {
            let value: FaultTriggerBody = parse_body_value(kind, body)?;
            if value.outage_ms == 0 || value.outage_ms > 60_000 {
                return Err(AppError::evidence(
                    "FAULT_TRIGGER outage_ms must be 1..=60000",
                ));
            }
        }
        ControlType::StopRequest => {
            let value: StopRequestBody = parse_body_value(kind, body)?;
            if !valid_text(&value.reason, 128) {
                return Err(AppError::evidence("invalid STOP_REQUEST reason"));
            }
        }
        ControlType::StopComplete => {
            let value: StopCompleteBody = parse_body_value(kind, body)?;
            if value.status != "stopped" || !value.endpoint_retired || !value.driver_exited {
                return Err(AppError::evidence(
                    "invalid STOP_COMPLETE body or readiness flags",
                ));
            }
        }
        ControlType::RetireComplete => {
            let value: RetireCompleteBody = parse_body_value(kind, body)?;
            if value.status != "retired" || !value.artifact_closed || !value.process_alive {
                return Err(AppError::evidence(
                    "invalid RETIRE_COMPLETE body or readiness flags",
                ));
            }
        }
        ControlType::Error => {
            let value: ErrorBody = parse_body_value(kind, body)?;
            if !valid_text(&value.code, 64) || value.message.is_empty() || value.message.len() > 4096 {
                return Err(AppError::evidence("invalid ERROR body"));
            }
        }
    }
    Ok(())
}

fn parse_body_value<T: DeserializeOwned>(kind: ControlType, body: &Value) -> AppResult<T> {
    serde_json::from_value(body.clone()).map_err(|error| {
        AppError::evidence(format!(
            "T9C1 {} body has unknown/missing/mistyped fields: {error}",
            control_type_name(kind)
        ))
    })
}

fn validate_run_start(value: &RunStartBody) -> AppResult<()> {
    if !valid_text(&value.run_start_id, 128) || !valid_text(&value.script_id, 64) {
        return Err(AppError::evidence("invalid RUN_START identities"));
    }
    if value.last_tick <= value.tick0 {
        return Err(AppError::evidence("RUN_START last_tick must exceed tick0"));
    }
    value.fault_profile.validate()
}

fn valid_text(value: &str, maximum: usize) -> bool {
    !value.is_empty()
        && value.len() <= maximum
        && value.bytes().all(|byte| (0x21..=0x7e).contains(&byte))
}

#[must_use]
pub const fn control_type_name(kind: ControlType) -> &'static str {
    match kind {
        ControlType::Ready => "READY",
        ControlType::Armed => "ARMED",
        ControlType::RunStart => "RUN_START",
        ControlType::TickHeartbeat => "TICK_HEARTBEAT",
        ControlType::ArmFaults => "ARM_FAULTS",
        ControlType::FaultTrigger => "FAULT_TRIGGER",
        ControlType::StopRequest => "STOP_REQUEST",
        ControlType::StopComplete => "STOP_COMPLETE",
        ControlType::RetireComplete => "RETIRE_COMPLETE",
        ControlType::Error => "ERROR",
    }
}
