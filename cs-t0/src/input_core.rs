//! Pure-memory T0-04 canonical input and loopback scheduling primitives.
//!
//! This module deliberately has no Win32, game-process, hook, or network dependency.  The wire
//! layouts are experimental identities and must not be confused with the existing ISCS protocol.

use std::collections::{BTreeMap, HashMap, VecDeque};
use std::error::Error;
use std::fmt;

pub const INPUT_STATE_VERSION: u16 = 1;
pub const INPUT_STATE_SIZE: usize = 24;
pub const INPUT_COMMAND_SIZE: usize = 64;
pub const INPUT_DEAD_ZONE_Q15: i16 = 4096;
pub const INPUT_LEAD_TICKS: u64 = 2;
pub const INPUT_FUTURE_WINDOW: u64 = 64;
pub const INPUT_TRANSPORT_CAPACITY: usize = 256;
pub const INPUT_QUEUE_CAPACITY: usize = 256;
pub const INPUT_DRAIN_BUDGET: usize = 64;
pub const INPUT_DEDUPE_HISTORY: usize = 4096;
pub const GAMEPLAY_ACTION_COUNT: usize = 12;
pub const MAX_LOGICAL_ACTION_COUNT: usize = 32;
pub const PRIMITIVES_PER_ACTION: usize = 4;
pub const UNBOUND_PRIMITIVE: u32 = u32::MAX;

pub const HELD_SHOOT: u32 = 1 << 0;
pub const EDGE_BOMB: u8 = 1 << 0;
pub const EDGE_ITEM: u8 = 1 << 1;
pub const EDGE_CARD_OR_PILL: u8 = 1 << 2;
pub const EDGE_DROP: u8 = 1 << 3;
pub const EDGE_KNOWN_MASK: u8 = EDGE_BOMB | EDGE_ITEM | EDGE_CARD_OR_PILL | EDGE_DROP;

const Q15_MAX: i16 = i16::MAX;
const EDGE_BITS: [u8; 4] = [EDGE_BOMB, EDGE_ITEM, EDGE_CARD_OR_PILL, EDGE_DROP];

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum PrimitiveActionClassification {
    NoMatch,
    UniqueGameplay(u32),
    AmbiguousOrOutsideGameplay,
    InvalidLayout,
}

/// Classifies a per-device primitive without ever promoting an action outside `0..11`.
///
/// The fixed game stores four primitive bindings per logical action. A primitive is safe to emit
/// as a gameplay action only when exactly one logical action owns it and that action is in the T0
/// gameplay range. Aliases with UI/session actions are deliberately redacted.
pub fn classify_primitive_action(
    primitive: u32,
    bindings: &[[u32; PRIMITIVES_PER_ACTION]],
) -> PrimitiveActionClassification {
    if bindings.len() > MAX_LOGICAL_ACTION_COUNT {
        return PrimitiveActionClassification::InvalidLayout;
    }
    if primitive == UNBOUND_PRIMITIVE {
        return PrimitiveActionClassification::NoMatch;
    }
    let mut matched_action = None;
    for (action, record) in bindings.iter().enumerate() {
        if !record.contains(&primitive) {
            continue;
        }
        if matched_action.replace(action).is_some() {
            return PrimitiveActionClassification::AmbiguousOrOutsideGameplay;
        }
    }
    match matched_action {
        None => PrimitiveActionClassification::NoMatch,
        Some(action) if action < GAMEPLAY_ACTION_COUNT => {
            PrimitiveActionClassification::UniqueGameplay(action as u32)
        }
        Some(_) => PrimitiveActionClassification::AmbiguousOrOutsideGameplay,
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum InputCoreError {
    WrongLength { expected: usize, actual: usize },
    UnsupportedVersion(u16),
    InvalidDeclaredLength(u16),
    InvalidBoolean(u8),
    UnknownHeldBits(u32),
    UnknownEdgeBits(u32),
    ReservedNotZero { offset: usize },
    AxisIsNegativeFullScale { axis: &'static str },
    AxisPairInsideDeadZone { pair: &'static str },
    AxisPairOutsideUnitCircle { pair: &'static str },
    ShootAimConflict,
    NonFiniteAxis { axis: &'static str },
    AxisOutsideUnitRange { axis: &'static str },
    ZeroSession,
    ZeroClient,
    ZeroSequence,
    SequenceExhausted,
    TickOverflow,
    SelfTestFailure(&'static str),
}

impl fmt::Display for InputCoreError {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::WrongLength { expected, actual } => {
                write!(
                    formatter,
                    "wrong input length: expected {expected}, got {actual}"
                )
            }
            Self::UnsupportedVersion(version) => {
                write!(formatter, "unsupported input state version {version}")
            }
            Self::InvalidDeclaredLength(length) => {
                write!(formatter, "invalid declared input state length {length}")
            }
            Self::InvalidBoolean(value) => write!(formatter, "invalid shootHeld value {value}"),
            Self::UnknownHeldBits(bits) => {
                write!(formatter, "unknown held flag bits 0x{bits:08X}")
            }
            Self::UnknownEdgeBits(bits) => write!(formatter, "unknown edge flag bits 0x{bits:08X}"),
            Self::ReservedNotZero { offset } => {
                write!(
                    formatter,
                    "reserved input byte at offset {offset} is not zero"
                )
            }
            Self::AxisIsNegativeFullScale { axis } => {
                write!(formatter, "axis {axis} uses forbidden Q15 value -32768")
            }
            Self::AxisPairInsideDeadZone { pair } => {
                write!(
                    formatter,
                    "nonzero axis pair {pair} is inside the canonical dead zone"
                )
            }
            Self::AxisPairOutsideUnitCircle { pair } => {
                write!(
                    formatter,
                    "axis pair {pair} is outside the canonical unit circle"
                )
            }
            Self::ShootAimConflict => {
                formatter.write_str("shootHeld and canonical aim direction disagree")
            }
            Self::NonFiniteAxis { axis } => write!(formatter, "axis {axis} is not finite"),
            Self::AxisOutsideUnitRange { axis } => {
                write!(formatter, "axis {axis} is outside [-1, 1]")
            }
            Self::ZeroSession => formatter.write_str("session identity is all zero"),
            Self::ZeroClient => formatter.write_str("client identity is zero"),
            Self::ZeroSequence => formatter.write_str("input sequence is zero"),
            Self::SequenceExhausted => formatter.write_str("input sequence is exhausted"),
            Self::TickOverflow => formatter.write_str("input target tick overflow"),
            Self::SelfTestFailure(reason) => {
                write!(formatter, "input codec self-test failed: {reason}")
            }
        }
    }
}

impl Error for InputCoreError {}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct InputCoreManifest {
    pub schema_version: u16,
    pub state_version: u16,
    pub state_size: usize,
    pub command_size: usize,
    pub dead_zone_q15: i16,
    pub lead_ticks: u64,
    pub future_window: u64,
    pub transport_capacity: usize,
    pub queue_capacity: usize,
    pub drain_budget: usize,
    pub dedupe_history: usize,
    pub known_held_mask: u32,
    pub known_edge_mask: u8,
}

pub const INPUT_CORE_MANIFEST: InputCoreManifest = InputCoreManifest {
    schema_version: 1,
    state_version: INPUT_STATE_VERSION,
    state_size: INPUT_STATE_SIZE,
    command_size: INPUT_COMMAND_SIZE,
    dead_zone_q15: INPUT_DEAD_ZONE_Q15,
    lead_ticks: INPUT_LEAD_TICKS,
    future_window: INPUT_FUTURE_WINDOW,
    transport_capacity: INPUT_TRANSPORT_CAPACITY,
    queue_capacity: INPUT_QUEUE_CAPACITY,
    drain_budget: INPUT_DRAIN_BUDGET,
    dedupe_history: INPUT_DEDUPE_HISTORY,
    known_held_mask: HELD_SHOOT,
    known_edge_mask: EDGE_KNOWN_MASK,
};

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct RawInputSample {
    pub move_x: f32,
    pub move_y: f32,
    pub aim_x: f32,
    pub aim_y: f32,
    pub shoot_held: bool,
    /// Rising edges, not physical button levels.
    pub edge_flags: u8,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct InputStateDraftV1 {
    shoot_held: bool,
    edge_flags: u8,
    move_x: i16,
    move_y: i16,
    aim_x: i16,
    aim_y: i16,
}

impl InputStateDraftV1 {
    pub const fn neutral() -> Self {
        Self {
            shoot_held: false,
            edge_flags: 0,
            move_x: 0,
            move_y: 0,
            aim_x: 0,
            aim_y: 0,
        }
    }

    pub fn from_q15(
        shoot_held: bool,
        edge_flags: u8,
        move_x: i16,
        move_y: i16,
        aim_x: i16,
        aim_y: i16,
    ) -> Result<Self, InputCoreError> {
        let state = Self {
            shoot_held,
            edge_flags,
            move_x,
            move_y,
            aim_x,
            aim_y,
        };
        state.validate()?;
        Ok(state)
    }

    pub fn canonicalize(sample: RawInputSample) -> Result<Self, InputCoreError> {
        validate_edge_flags(sample.edge_flags)?;
        let (move_x, move_y) = canonicalize_pair(sample.move_x, sample.move_y, "move")?;
        let (aim_x, aim_y) = canonicalize_pair(sample.aim_x, sample.aim_y, "aim")?;
        Self::from_q15(
            sample.shoot_held,
            sample.edge_flags,
            move_x,
            move_y,
            aim_x,
            aim_y,
        )
    }

    pub fn decode(bytes: &[u8]) -> Result<Self, InputCoreError> {
        if bytes.len() != INPUT_STATE_SIZE {
            return Err(InputCoreError::WrongLength {
                expected: INPUT_STATE_SIZE,
                actual: bytes.len(),
            });
        }
        let version = u16::from_le_bytes([bytes[0], bytes[1]]);
        if version != INPUT_STATE_VERSION {
            return Err(InputCoreError::UnsupportedVersion(version));
        }
        let declared_length = u16::from_le_bytes([bytes[2], bytes[3]]);
        if usize::from(declared_length) != INPUT_STATE_SIZE {
            return Err(InputCoreError::InvalidDeclaredLength(declared_length));
        }
        let held_flags = read_u32(bytes, 4);
        let unknown_held = held_flags & !HELD_SHOOT;
        if unknown_held != 0 {
            return Err(InputCoreError::UnknownHeldBits(unknown_held));
        }
        let edge_flags = read_u32(bytes, 8);
        let unknown_edges = edge_flags & !u32::from(EDGE_KNOWN_MASK);
        if unknown_edges != 0 {
            return Err(InputCoreError::UnknownEdgeBits(unknown_edges));
        }
        if read_u32(bytes, 20) != 0 {
            return Err(InputCoreError::ReservedNotZero { offset: 20 });
        }
        Self::from_q15(
            held_flags & HELD_SHOOT != 0,
            edge_flags as u8,
            read_i16(bytes, 12),
            read_i16(bytes, 14),
            read_i16(bytes, 16),
            read_i16(bytes, 18),
        )
    }

    pub fn encode(self) -> Result<[u8; INPUT_STATE_SIZE], InputCoreError> {
        self.validate()?;
        let mut bytes = [0_u8; INPUT_STATE_SIZE];
        bytes[0..2].copy_from_slice(&INPUT_STATE_VERSION.to_le_bytes());
        bytes[2..4].copy_from_slice(&(INPUT_STATE_SIZE as u16).to_le_bytes());
        bytes[4..8].copy_from_slice(&u32::from(self.shoot_held).to_le_bytes());
        bytes[8..12].copy_from_slice(&u32::from(self.edge_flags).to_le_bytes());
        bytes[12..14].copy_from_slice(&self.move_x.to_le_bytes());
        bytes[14..16].copy_from_slice(&self.move_y.to_le_bytes());
        bytes[16..18].copy_from_slice(&self.aim_x.to_le_bytes());
        bytes[18..20].copy_from_slice(&self.aim_y.to_le_bytes());
        Ok(bytes)
    }

    pub fn validate(self) -> Result<(), InputCoreError> {
        validate_edge_flags(self.edge_flags)?;
        validate_pair(self.move_x, self.move_y, "move")?;
        validate_pair(self.aim_x, self.aim_y, "aim")?;
        let has_aim = self.aim_x != 0 || self.aim_y != 0;
        if self.shoot_held != has_aim {
            return Err(InputCoreError::ShootAimConflict);
        }
        Ok(())
    }

    pub const fn shoot_held(self) -> bool {
        self.shoot_held
    }

    pub const fn edge_flags(self) -> u8 {
        self.edge_flags
    }

    pub const fn move_axes(self) -> (i16, i16) {
        (self.move_x, self.move_y)
    }

    pub const fn aim_axes(self) -> (i16, i16) {
        (self.aim_x, self.aim_y)
    }

    fn with_edges(self, edge_flags: u8) -> Self {
        Self { edge_flags, ..self }
    }
}

fn read_i16(bytes: &[u8], offset: usize) -> i16 {
    i16::from_le_bytes([bytes[offset], bytes[offset + 1]])
}

fn read_u32(bytes: &[u8], offset: usize) -> u32 {
    u32::from_le_bytes(
        bytes[offset..offset + 4]
            .try_into()
            .expect("bounded wire offset"),
    )
}

fn validate_edge_flags(flags: u8) -> Result<(), InputCoreError> {
    let unknown = flags & !EDGE_KNOWN_MASK;
    if unknown != 0 {
        Err(InputCoreError::UnknownEdgeBits(u32::from(unknown)))
    } else {
        Ok(())
    }
}

fn validate_pair(x: i16, y: i16, pair: &'static str) -> Result<(), InputCoreError> {
    if x == i16::MIN {
        return Err(InputCoreError::AxisIsNegativeFullScale {
            axis: pair_axis(pair, true),
        });
    }
    if y == i16::MIN {
        return Err(InputCoreError::AxisIsNegativeFullScale {
            axis: pair_axis(pair, false),
        });
    }
    let squared = pair_squared(x, y);
    if squared > i64::from(Q15_MAX).pow(2) {
        return Err(InputCoreError::AxisPairOutsideUnitCircle { pair });
    }
    Ok(())
}

fn pair_axis(pair: &'static str, x: bool) -> &'static str {
    match (pair, x) {
        ("move", true) => "move_x",
        ("move", false) => "move_y",
        ("aim", true) => "aim_x",
        ("aim", false) => "aim_y",
        (_, true) => "x",
        (_, false) => "y",
    }
}

fn pair_squared(x: i16, y: i16) -> i64 {
    let x = i64::from(x);
    let y = i64::from(y);
    x * x + y * y
}

fn canonicalize_pair(x: f32, y: f32, pair: &'static str) -> Result<(i16, i16), InputCoreError> {
    validate_raw_axis(x, pair_axis(pair, true))?;
    validate_raw_axis(y, pair_axis(pair, false))?;
    let qx = quantize_raw_axis(x);
    let qy = quantize_raw_axis(y);
    radial_dead_zone_and_unit_circle(qx, qy)
}

fn quantize_raw_axis(value: f32) -> i64 {
    (value.clamp(-1.0, 1.0) * f32::from(Q15_MAX)).trunc() as i64
}

fn radial_dead_zone_and_unit_circle(qx: i64, qy: i64) -> Result<(i16, i16), InputCoreError> {
    let squared = qx * qx + qy * qy;
    let radius = integer_sqrt_ceil(squared as u64) as i64;
    let dead_zone = i64::from(INPUT_DEAD_ZONE_Q15);
    if radius <= dead_zone {
        return Ok((0, 0));
    }

    // The radial dead zone is removed and the remaining integer radius is linearly remapped to
    // full Q15 range. Signed integer division is intentionally truncation toward zero.
    let capped_radius = radius.min(i64::from(Q15_MAX));
    let output_magnitude =
        (capped_radius - dead_zone) * i64::from(Q15_MAX) / (i64::from(Q15_MAX) - dead_zone);
    let out_x = qx * output_magnitude / radius;
    let out_y = qy * output_magnitude / radius;
    let out_x = i16::try_from(out_x)
        .map_err(|_| InputCoreError::AxisPairOutsideUnitCircle { pair: "canonical" })?;
    let out_y = i16::try_from(out_y)
        .map_err(|_| InputCoreError::AxisPairOutsideUnitCircle { pair: "canonical" })?;
    Ok((out_x, out_y))
}

fn integer_sqrt_floor(value: u64) -> u64 {
    if value < 2 {
        return value;
    }
    let mut low = 1_u64;
    let mut high = value.min(u64::from(u32::MAX));
    let mut answer = 1_u64;
    while low <= high {
        let middle = low + (high - low) / 2;
        if middle <= value / middle {
            answer = middle;
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    }
    answer
}

fn integer_sqrt_ceil(value: u64) -> u64 {
    let floor = integer_sqrt_floor(value);
    floor + u64::from(floor * floor != value)
}

fn validate_raw_axis(value: f32, axis: &'static str) -> Result<(), InputCoreError> {
    if !value.is_finite() {
        return Err(InputCoreError::NonFiniteAxis { axis });
    }
    Ok(())
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub struct InputIdentity {
    pub session: [u8; 16],
    pub client_id: u64,
    pub sequence: u64,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct InputCmdDraftV1 {
    session: [u8; 16],
    client_id: u64,
    sequence: u64,
    target_tick: u64,
    state: InputStateDraftV1,
}

impl InputCmdDraftV1 {
    pub fn new(
        session: [u8; 16],
        client_id: u64,
        sequence: u64,
        target_tick: u64,
        state: InputStateDraftV1,
    ) -> Result<Self, InputCoreError> {
        if session == [0; 16] {
            return Err(InputCoreError::ZeroSession);
        }
        if client_id == 0 {
            return Err(InputCoreError::ZeroClient);
        }
        if sequence == 0 {
            return Err(InputCoreError::ZeroSequence);
        }
        if sequence == u64::MAX {
            return Err(InputCoreError::SequenceExhausted);
        }
        state.validate()?;
        Ok(Self {
            session,
            client_id,
            sequence,
            target_tick,
            state,
        })
    }

    pub fn decode(bytes: &[u8]) -> Result<Self, InputCoreError> {
        if bytes.len() != INPUT_COMMAND_SIZE {
            return Err(InputCoreError::WrongLength {
                expected: INPUT_COMMAND_SIZE,
                actual: bytes.len(),
            });
        }
        let mut session = [0_u8; 16];
        session.copy_from_slice(&bytes[0..16]);
        Self::new(
            session,
            read_u64(bytes, 16),
            read_u64(bytes, 24),
            read_u64(bytes, 32),
            InputStateDraftV1::decode(&bytes[40..64])?,
        )
    }

    pub fn encode(self) -> Result<[u8; INPUT_COMMAND_SIZE], InputCoreError> {
        let state = self.state.encode()?;
        Self::new(
            self.session,
            self.client_id,
            self.sequence,
            self.target_tick,
            self.state,
        )?;
        let mut bytes = [0_u8; INPUT_COMMAND_SIZE];
        bytes[0..16].copy_from_slice(&self.session);
        bytes[16..24].copy_from_slice(&self.client_id.to_le_bytes());
        bytes[24..32].copy_from_slice(&self.sequence.to_le_bytes());
        bytes[32..40].copy_from_slice(&self.target_tick.to_le_bytes());
        bytes[40..64].copy_from_slice(&state);
        Ok(bytes)
    }

    pub const fn identity(self) -> InputIdentity {
        InputIdentity {
            session: self.session,
            client_id: self.client_id,
            sequence: self.sequence,
        }
    }

    pub const fn session(self) -> [u8; 16] {
        self.session
    }

    pub const fn client_id(self) -> u64 {
        self.client_id
    }

    pub const fn sequence(self) -> u64 {
        self.sequence
    }

    pub const fn target_tick(self) -> u64 {
        self.target_tick
    }

    pub const fn state(self) -> InputStateDraftV1 {
        self.state
    }
}

fn read_u64(bytes: &[u8], offset: usize) -> u64 {
    u64::from_le_bytes(
        bytes[offset..offset + 8]
            .try_into()
            .expect("bounded wire offset"),
    )
}

/// Small runtime self-test for CLI inspection paths.  Full mutation/queue coverage remains in the
/// module test suite and the checked-in JSON fixture.
pub fn run_codec_self_test() -> Result<(), InputCoreError> {
    let neutral = InputStateDraftV1::neutral();
    let neutral_bytes = neutral.encode()?;
    let expected_neutral = [
        1, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    ];
    if neutral_bytes != expected_neutral {
        return Err(InputCoreError::SelfTestFailure(
            "neutral golden bytes changed",
        ));
    }
    if InputStateDraftV1::decode(&neutral_bytes)? != neutral {
        return Err(InputCoreError::SelfTestFailure("state round-trip changed"));
    }
    let command = InputCmdDraftV1::new([0xA5; 16], 1, 1, 2, neutral)?;
    if InputCmdDraftV1::decode(&command.encode()?)? != command {
        return Err(InputCoreError::SelfTestFailure(
            "command round-trip changed",
        ));
    }
    let mut reserved_mutation = neutral_bytes;
    reserved_mutation[20] = 1;
    if InputStateDraftV1::decode(&reserved_mutation).is_ok() {
        return Err(InputCoreError::SelfTestFailure(
            "reserved mutation was accepted",
        ));
    }
    Ok(())
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum TransportSendOutcome {
    Enqueued,
    Full,
}

#[derive(Debug, Default)]
pub struct LoopbackTransport {
    messages: VecDeque<[u8; INPUT_COMMAND_SIZE]>,
}

impl LoopbackTransport {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn send(
        &mut self,
        command: InputCmdDraftV1,
    ) -> Result<TransportSendOutcome, InputCoreError> {
        self.send_bytes(&command.encode()?)
    }

    pub fn send_bytes(&mut self, bytes: &[u8]) -> Result<TransportSendOutcome, InputCoreError> {
        let command = InputCmdDraftV1::decode(bytes)?;
        if self.messages.len() == INPUT_TRANSPORT_CAPACITY {
            return Ok(TransportSendOutcome::Full);
        }
        self.messages.push_back(command.encode()?);
        Ok(TransportSendOutcome::Enqueued)
    }

    pub fn drain(&mut self) -> Vec<InputCmdDraftV1> {
        let count = self.messages.len().min(INPUT_DRAIN_BUDGET);
        (0..count)
            .map(|_| {
                let bytes = self
                    .messages
                    .pop_front()
                    .expect("count is bounded by queue length");
                InputCmdDraftV1::decode(&bytes).expect("transport stores validated canonical bytes")
            })
            .collect()
    }

    pub fn len(&self) -> usize {
        self.messages.len()
    }

    pub fn is_empty(&self) -> bool {
        self.messages.is_empty()
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum InputRejectReason {
    SessionMismatch,
    ClientRetired,
    StaleSequence,
    StaleTarget,
    Expired,
    TooFar,
    QueueFull,
    EdgeCollision,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum InputAcceptOutcome {
    Accepted { merged_same_target: bool },
    Duplicate,
    ConflictRetired,
    Rejected(InputRejectReason),
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct InputDecision {
    pub identity: InputIdentity,
    pub target_tick: u64,
    pub outcome: InputAcceptOutcome,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct ResolvedInput {
    pub client_id: u64,
    pub target_tick: u64,
    /// Highest sequence supplies continuous state.
    pub sequence: u64,
    pub state: InputStateDraftV1,
    /// Source sequence for bomb, item, card/pill, and drop, respectively.
    pub edge_sequences: [Option<u64>; 4],
}

#[derive(Clone, Copy, Debug)]
struct StagedInput {
    sequence: u64,
    state: InputStateDraftV1,
    edge_sequences: [Option<u64>; 4],
}

impl StagedInput {
    fn from_command(command: InputCmdDraftV1) -> Self {
        let mut edge_sequences = [None; 4];
        for (index, bit) in EDGE_BITS.into_iter().enumerate() {
            if command.state.edge_flags & bit != 0 {
                edge_sequences[index] = Some(command.sequence);
            }
        }
        Self {
            sequence: command.sequence,
            state: command.state,
            edge_sequences,
        }
    }

    fn merge(self, command: InputCmdDraftV1) -> Self {
        debug_assert_eq!(self.state.edge_flags & command.state.edge_flags, 0);
        let combined_edges = self.state.edge_flags | command.state.edge_flags;
        let mut edge_sequences = self.edge_sequences;
        for (index, bit) in EDGE_BITS.into_iter().enumerate() {
            if command.state.edge_flags & bit != 0 {
                edge_sequences[index] = Some(command.sequence);
            }
        }
        Self {
            sequence: command.sequence,
            state: command.state.with_edges(combined_edges),
            edge_sequences,
        }
    }
}

#[derive(Debug)]
pub struct InputQueue {
    session: [u8; 16],
    staged: BTreeMap<(u64, u64), StagedInput>,
    highest_sequence: HashMap<u64, u64>,
    highest_target_tick: HashMap<u64, u64>,
    last_resolve_call: Option<u64>,
    retired_clients: HashMap<u64, InputIdentity>,
    dedupe: HashMap<InputIdentity, [u8; INPUT_COMMAND_SIZE]>,
    dedupe_order: VecDeque<InputIdentity>,
}

impl InputQueue {
    pub fn new(session: [u8; 16]) -> Result<Self, InputCoreError> {
        if session == [0; 16] {
            return Err(InputCoreError::ZeroSession);
        }
        Ok(Self {
            session,
            staged: BTreeMap::new(),
            highest_sequence: HashMap::new(),
            highest_target_tick: HashMap::new(),
            last_resolve_call: None,
            retired_clients: HashMap::new(),
            dedupe: HashMap::new(),
            dedupe_order: VecDeque::new(),
        })
    }

    pub fn accept(&mut self, current_tick: u64, command: InputCmdDraftV1) -> InputDecision {
        let identity = command.identity();
        let decision = |outcome| InputDecision {
            identity,
            target_tick: command.target_tick,
            outcome,
        };
        if command.session != self.session {
            return decision(InputAcceptOutcome::Rejected(
                InputRejectReason::SessionMismatch,
            ));
        }
        if self.retired_clients.contains_key(&command.client_id) {
            return decision(InputAcceptOutcome::Rejected(
                InputRejectReason::ClientRetired,
            ));
        }
        let canonical = command
            .encode()
            .expect("InputCmdDraftV1 is validated at construction");
        if let Some(previous) = self.dedupe.get(&identity) {
            if previous == &canonical {
                return decision(InputAcceptOutcome::Duplicate);
            }
            self.retire_conflicting_client(identity);
            return decision(InputAcceptOutcome::ConflictRetired);
        }
        if self
            .highest_sequence
            .get(&command.client_id)
            .is_some_and(|highest| command.sequence <= *highest)
        {
            return decision(InputAcceptOutcome::Rejected(
                InputRejectReason::StaleSequence,
            ));
        }
        if command.target_tick < current_tick {
            return decision(InputAcceptOutcome::Rejected(InputRejectReason::Expired));
        }
        let last_future_tick = current_tick.saturating_add(INPUT_FUTURE_WINDOW);
        if command.target_tick > last_future_tick {
            return decision(InputAcceptOutcome::Rejected(InputRejectReason::TooFar));
        }
        if self
            .last_resolve_call
            .is_some_and(|tick| command.target_tick <= tick)
            || self
                .highest_target_tick
                .get(&command.client_id)
                .is_some_and(|highest| command.target_tick < *highest)
        {
            return decision(InputAcceptOutcome::Rejected(InputRejectReason::StaleTarget));
        }

        let key = (command.client_id, command.target_tick);
        if let Some(staged) = self.staged.get(&key) {
            if staged.state.edge_flags & command.state.edge_flags != 0 {
                return decision(InputAcceptOutcome::Rejected(
                    InputRejectReason::EdgeCollision,
                ));
            }
        } else if self.staged.len() == INPUT_QUEUE_CAPACITY {
            return decision(InputAcceptOutcome::Rejected(InputRejectReason::QueueFull));
        }

        let merged_same_target = if let Some(staged) = self.staged.get(&key).copied() {
            self.staged.insert(key, staged.merge(command));
            true
        } else {
            self.staged.insert(key, StagedInput::from_command(command));
            false
        };
        self.highest_sequence
            .insert(command.client_id, command.sequence);
        self.highest_target_tick
            .entry(command.client_id)
            .and_modify(|highest| *highest = (*highest).max(command.target_tick))
            .or_insert(command.target_tick);
        self.remember(identity, canonical);
        decision(InputAcceptOutcome::Accepted { merged_same_target })
    }

    pub fn drain_transport(
        &mut self,
        current_tick: u64,
        transport: &mut LoopbackTransport,
    ) -> Vec<InputDecision> {
        transport
            .drain()
            .into_iter()
            .map(|command| self.accept(current_tick, command))
            .collect()
    }

    /// Returns at most one resolved input per client for `current_tick`, in client-id order.
    /// Staged inputs older than the current tick are discarded, never replayed late.
    pub fn resolve_tick(&mut self, current_tick: u64) -> Vec<ResolvedInput> {
        self.last_resolve_call = Some(
            self.last_resolve_call
                .map_or(current_tick, |previous| previous.max(current_tick)),
        );
        self.staged
            .retain(|(_, target_tick), _| *target_tick >= current_tick);
        let keys: Vec<_> = self
            .staged
            .keys()
            .filter(|(_, target_tick)| *target_tick == current_tick)
            .copied()
            .collect();
        keys.into_iter()
            .filter_map(|(client_id, target_tick)| {
                let staged = self.staged.remove(&(client_id, target_tick))?;
                if self.retired_clients.contains_key(&client_id) {
                    return None;
                }
                Some(ResolvedInput {
                    client_id,
                    target_tick,
                    sequence: staged.sequence,
                    state: staged.state,
                    edge_sequences: staged.edge_sequences,
                })
            })
            .collect()
    }

    pub fn staged_len(&self) -> usize {
        self.staged.len()
    }

    pub fn dedupe_len(&self) -> usize {
        self.dedupe.len()
    }

    pub fn is_client_retired(&self, client_id: u64) -> bool {
        self.retired_clients.contains_key(&client_id)
    }

    fn retire_conflicting_client(&mut self, identity: InputIdentity) {
        self.retired_clients.insert(identity.client_id, identity);
        self.staged
            .retain(|(client_id, _), _| *client_id != identity.client_id);
    }

    fn remember(&mut self, identity: InputIdentity, bytes: [u8; INPUT_COMMAND_SIZE]) {
        if self.dedupe.len() == INPUT_DEDUPE_HISTORY {
            let oldest = self
                .dedupe_order
                .pop_front()
                .expect("nonzero capacity has an oldest identity");
            self.dedupe.remove(&oldest);
        }
        self.dedupe.insert(identity, bytes);
        self.dedupe_order.push_back(identity);
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct PlannedEdge {
    pub target_tick: u64,
    pub edge_flags: u8,
}

#[derive(Clone, Debug, Default)]
pub struct EdgePlanner {
    previous_levels: u8,
    last_scheduled_tick: [Option<u64>; 4],
}

impl EdgePlanner {
    pub fn new() -> Self {
        Self::default()
    }

    /// Converts physical edge-button levels to rising edges and groups them by target tick.
    pub fn plan(
        &mut self,
        estimated_server_tick: u64,
        held_edge_levels: u8,
    ) -> Result<Vec<PlannedEdge>, InputCoreError> {
        validate_edge_flags(held_edge_levels)?;
        let base_tick = estimated_server_tick
            .checked_add(INPUT_LEAD_TICKS)
            .ok_or(InputCoreError::TickOverflow)?;
        let rising = held_edge_levels & !self.previous_levels;
        let mut targets = BTreeMap::<u64, u8>::new();
        let mut next_last = self.last_scheduled_tick;
        for (index, bit) in EDGE_BITS.into_iter().enumerate() {
            if rising & bit == 0 {
                continue;
            }
            let ordered_tick = match next_last[index] {
                Some(last) => {
                    base_tick.max(last.checked_add(1).ok_or(InputCoreError::TickOverflow)?)
                }
                None => base_tick,
            };
            next_last[index] = Some(ordered_tick);
            *targets.entry(ordered_tick).or_default() |= bit;
        }
        self.previous_levels = held_edge_levels;
        self.last_scheduled_tick = next_last;
        Ok(targets
            .into_iter()
            .map(|(target_tick, edge_flags)| PlannedEdge {
                target_tick,
                edge_flags,
            })
            .collect())
    }

    /// Observes current levels without producing an edge (pause/focus/loading resynchronization).
    pub fn synchronize_levels(&mut self, held_edge_levels: u8) -> Result<(), InputCoreError> {
        validate_edge_flags(held_edge_levels)?;
        self.previous_levels = held_edge_levels;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use serde::Deserialize;

    use super::*;

    const SESSION: [u8; 16] = [0xA5; 16];

    fn state(edges: u8, move_x: i16, aim_x: i16) -> InputStateDraftV1 {
        InputStateDraftV1::from_q15(aim_x != 0, edges, move_x, 0, aim_x, 0).unwrap()
    }

    fn command(client: u64, sequence: u64, tick: u64, state: InputStateDraftV1) -> InputCmdDraftV1 {
        InputCmdDraftV1::new(SESSION, client, sequence, tick, state).unwrap()
    }

    #[test]
    fn primitive_classifier_redacts_ui_aliases_and_invalid_layouts() {
        let mut bindings = [[UNBOUND_PRIMITIVE; PRIMITIVES_PER_ACTION]; MAX_LOGICAL_ACTION_COUNT];
        bindings[0][0] = 0x41;
        bindings[4][0] = 0x107;
        bindings[9][0] = 0x20;
        bindings[14][0] = 0x20;
        bindings[20][0] = 0x107;
        bindings[12][0] = 0x1B;

        assert_eq!(
            classify_primitive_action(0x41, &bindings),
            PrimitiveActionClassification::UniqueGameplay(0)
        );
        assert_eq!(
            classify_primitive_action(0x107, &bindings),
            PrimitiveActionClassification::AmbiguousOrOutsideGameplay
        );
        assert_eq!(
            classify_primitive_action(0x20, &bindings),
            PrimitiveActionClassification::AmbiguousOrOutsideGameplay
        );
        assert_eq!(
            classify_primitive_action(0x1B, &bindings),
            PrimitiveActionClassification::AmbiguousOrOutsideGameplay
        );
        assert_eq!(
            classify_primitive_action(0xDEAD, &bindings),
            PrimitiveActionClassification::NoMatch
        );
        assert_eq!(
            classify_primitive_action(UNBOUND_PRIMITIVE, &bindings),
            PrimitiveActionClassification::NoMatch
        );

        let too_many = [[UNBOUND_PRIMITIVE; PRIMITIVES_PER_ACTION]; MAX_LOGICAL_ACTION_COUNT + 1];
        assert_eq!(
            classify_primitive_action(0x41, &too_many),
            PrimitiveActionClassification::InvalidLayout
        );
    }

    #[test]
    fn primitive_classifier_counts_duplicate_slots_as_one_action() {
        let bindings = [[0x41, 0x41, UNBOUND_PRIMITIVE, UNBOUND_PRIMITIVE]];
        assert_eq!(
            classify_primitive_action(0x41, &bindings),
            PrimitiveActionClassification::UniqueGameplay(0)
        );
    }

    fn hex_decode(value: &str) -> Vec<u8> {
        assert_eq!(value.len() % 2, 0);
        value
            .as_bytes()
            .chunks_exact(2)
            .map(|pair| {
                let pair = std::str::from_utf8(pair).unwrap();
                u8::from_str_radix(pair, 16).unwrap()
            })
            .collect()
    }

    #[derive(Deserialize)]
    #[serde(rename_all = "camelCase")]
    struct Fixture {
        schema_version: u32,
        state_size: usize,
        command_size: usize,
        dead_zone_q15: i16,
        golden: Vec<GoldenFixture>,
        mutations: Vec<MutationFixture>,
    }

    #[derive(Deserialize)]
    struct GoldenFixture {
        name: String,
        hex: String,
    }

    #[derive(Deserialize)]
    struct MutationFixture {
        name: String,
        hex: String,
        error: String,
    }

    #[test]
    fn fixture_golden_vectors_round_trip_and_mutations_fail_closed() {
        let fixture: Fixture =
            serde_json::from_str(include_str!("../testdata/t0-04-input-vectors.json")).unwrap();
        assert_eq!(fixture.schema_version, 1);
        assert_eq!(fixture.state_size, INPUT_STATE_SIZE);
        assert_eq!(fixture.command_size, INPUT_COMMAND_SIZE);
        assert_eq!(fixture.dead_zone_q15, INPUT_DEAD_ZONE_Q15);
        assert!(fixture.golden.len() >= 2);
        for vector in fixture.golden {
            let bytes = hex_decode(&vector.hex);
            let decoded = InputStateDraftV1::decode(&bytes)
                .unwrap_or_else(|error| panic!("golden {} failed: {error}", vector.name));
            assert_eq!(
                decoded.encode().unwrap().as_slice(),
                bytes,
                "{}",
                vector.name
            );
        }
        for vector in fixture.mutations {
            let bytes = hex_decode(&vector.hex);
            let error = InputStateDraftV1::decode(&bytes)
                .expect_err(&format!("mutation {} was accepted", vector.name));
            assert!(
                format!("{error:?}").starts_with(&vector.error),
                "{}: expected {}, got {error:?}",
                vector.name,
                vector.error
            );
        }
    }

    #[test]
    fn state_layout_is_exact_little_endian_and_reserved_zero() {
        let state = InputStateDraftV1::from_q15(true, EDGE_KNOWN_MASK, 4097, 0, 32767, 0).unwrap();
        let bytes = state.encode().unwrap();
        assert_eq!(&bytes[0..12], &[1, 0, 24, 0, 1, 0, 0, 0, 15, 0, 0, 0]);
        assert_eq!(&bytes[12..20], &[1, 16, 0, 0, 255, 127, 0, 0]);
        assert_eq!(&bytes[20..24], &[0; 4]);
        assert_eq!(InputStateDraftV1::decode(&bytes).unwrap(), state);

        let diagonal = InputStateDraftV1::canonicalize(RawInputSample {
            move_x: -1.0,
            move_y: 1.0,
            aim_x: 1.0,
            aim_y: -1.0,
            shoot_held: true,
            edge_flags: 0,
        })
        .unwrap();
        assert_eq!(
            diagonal.encode().unwrap(),
            [
                0x01, 0x00, 0x18, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xA5,
                0x81, 0x5A, 0x81, 0x5A, 0x7F, 0xA5, 0x00, 0x00, 0x00, 0x00,
            ]
        );
    }

    #[test]
    fn raw_canonicalization_normalizes_and_rejects_nonfinite_or_conflicts() {
        let keyboard = InputStateDraftV1::canonicalize(RawInputSample {
            move_x: 1.0,
            move_y: 1.0,
            aim_x: 1.0,
            aim_y: 0.0,
            shoot_held: true,
            edge_flags: 0,
        })
        .unwrap();
        let controller = InputStateDraftV1::canonicalize(RawInputSample {
            move_x: std::f32::consts::FRAC_1_SQRT_2,
            move_y: std::f32::consts::FRAC_1_SQRT_2,
            aim_x: 1.0,
            aim_y: 0.0,
            shoot_held: true,
            edge_flags: 0,
        })
        .unwrap();
        assert_ne!(keyboard, controller);
        assert!(pair_squared(keyboard.move_x, keyboard.move_y) <= i64::from(Q15_MAX).pow(2));
        assert!(pair_squared(controller.move_x, controller.move_y) <= i64::from(Q15_MAX).pow(2));
        assert_eq!(keyboard.move_axes(), (23169, 23169));
        assert_eq!(quantize_raw_axis(0.5), 16383);
        assert_eq!(quantize_raw_axis(-0.5), -16383);

        let clamped = InputStateDraftV1::canonicalize(RawInputSample {
            move_x: 2.0,
            move_y: 0.0,
            aim_x: 0.0,
            aim_y: 0.0,
            shoot_held: false,
            edge_flags: 0,
        })
        .unwrap();
        let full_scale = InputStateDraftV1::canonicalize(RawInputSample {
            move_x: 1.0,
            move_y: 0.0,
            aim_x: 0.0,
            aim_y: 0.0,
            shoot_held: false,
            edge_flags: 0,
        })
        .unwrap();
        assert_eq!(clamped, full_scale);

        let deadzone = InputStateDraftV1::canonicalize(RawInputSample {
            move_x: 4096.0 / 32767.0,
            move_y: 0.0,
            aim_x: 0.0,
            aim_y: 0.0,
            shoot_held: false,
            edge_flags: 0,
        })
        .unwrap();
        assert_eq!(deadzone, InputStateDraftV1::neutral());
        assert!(
            InputStateDraftV1::canonicalize(RawInputSample {
                move_x: f32::NAN,
                move_y: 0.0,
                aim_x: 0.0,
                aim_y: 0.0,
                shoot_held: false,
                edge_flags: 0,
            })
            .is_err()
        );
        assert!(InputStateDraftV1::from_q15(false, 0, 0, 0, 32767, 0).is_err());
        assert!(InputStateDraftV1::from_q15(true, 0, 0, 0, 0, 0).is_err());
    }

    #[test]
    fn command_layout_is_exact_and_strict() {
        let command = command(0x0102_0304_0506_0708, 9, 10, state(EDGE_BOMB, 4097, 32767));
        let bytes = command.encode().unwrap();
        assert_eq!(bytes.len(), 64);
        assert_eq!(&bytes[0..16], &SESSION);
        assert_eq!(&bytes[16..24], &0x0102_0304_0506_0708_u64.to_le_bytes());
        assert_eq!(&bytes[24..32], &9_u64.to_le_bytes());
        assert_eq!(&bytes[32..40], &10_u64.to_le_bytes());
        assert_eq!(InputCmdDraftV1::decode(&bytes).unwrap(), command);
        assert!(InputCmdDraftV1::decode(&bytes[..63]).is_err());
        assert!(InputCmdDraftV1::new([0; 16], 1, 1, 0, InputStateDraftV1::neutral()).is_err());
        assert!(InputCmdDraftV1::new(SESSION, 0, 1, 0, InputStateDraftV1::neutral()).is_err());
        assert!(InputCmdDraftV1::new(SESSION, 1, 0, 0, InputStateDraftV1::neutral()).is_err());
        assert!(
            InputCmdDraftV1::new(SESSION, 1, u64::MAX, 0, InputStateDraftV1::neutral()).is_err()
        );
    }

    #[test]
    fn transport_is_bounded_validated_and_drains_only_budget() {
        let mut transport = LoopbackTransport::new();
        for sequence in 1..=INPUT_TRANSPORT_CAPACITY as u64 {
            assert_eq!(
                transport
                    .send(command(1, sequence, sequence, InputStateDraftV1::neutral()))
                    .unwrap(),
                TransportSendOutcome::Enqueued
            );
        }
        assert_eq!(
            transport
                .send(command(1, 999, 999, InputStateDraftV1::neutral()))
                .unwrap(),
            TransportSendOutcome::Full
        );
        assert_eq!(transport.drain().len(), INPUT_DRAIN_BUDGET);
        assert_eq!(
            transport.len(),
            INPUT_TRANSPORT_CAPACITY - INPUT_DRAIN_BUDGET
        );
        assert!(transport.send_bytes(&[0; 63]).is_err());
        assert_eq!(
            transport.len(),
            INPUT_TRANSPORT_CAPACITY - INPUT_DRAIN_BUDGET
        );
    }

    #[test]
    fn queue_duplicate_conflict_and_sequence_fail_closed() {
        let mut queue = InputQueue::new(SESSION).unwrap();
        let original = command(7, 1, 12, state(EDGE_BOMB, 4097, 0));
        assert_eq!(
            queue.accept(10, original).outcome,
            InputAcceptOutcome::Accepted {
                merged_same_target: false
            }
        );
        assert_eq!(
            queue.accept(10, original).outcome,
            InputAcceptOutcome::Duplicate
        );

        let conflict = command(7, 1, 12, state(EDGE_ITEM, 4097, 0));
        assert_eq!(
            queue.accept(10, conflict).outcome,
            InputAcceptOutcome::ConflictRetired
        );
        assert!(queue.is_client_retired(7));
        assert_eq!(queue.staged_len(), 0);
        assert!(queue.resolve_tick(12).is_empty());
        assert_eq!(
            queue
                .accept(10, command(7, 2, 12, InputStateDraftV1::neutral()))
                .outcome,
            InputAcceptOutcome::Rejected(InputRejectReason::ClientRetired)
        );

        let mut other = InputQueue::new(SESSION).unwrap();
        assert!(matches!(
            other
                .accept(10, command(1, 2, 12, InputStateDraftV1::neutral()))
                .outcome,
            InputAcceptOutcome::Accepted { .. }
        ));
        assert_eq!(
            other
                .accept(10, command(1, 1, 13, InputStateDraftV1::neutral()))
                .outcome,
            InputAcceptOutcome::Rejected(InputRejectReason::StaleSequence)
        );
    }

    #[test]
    fn queue_rejects_session_expired_future_full_and_edge_collision() {
        let mut queue = InputQueue::new(SESSION).unwrap();
        let wrong_session =
            InputCmdDraftV1::new([0x5A; 16], 1, 1, 10, InputStateDraftV1::neutral()).unwrap();
        assert_eq!(
            queue.accept(10, wrong_session).outcome,
            InputAcceptOutcome::Rejected(InputRejectReason::SessionMismatch)
        );
        assert_eq!(
            queue
                .accept(10, command(1, 1, 9, InputStateDraftV1::neutral()))
                .outcome,
            InputAcceptOutcome::Rejected(InputRejectReason::Expired)
        );
        assert_eq!(
            queue
                .accept(10, command(1, 1, 75, InputStateDraftV1::neutral()))
                .outcome,
            InputAcceptOutcome::Rejected(InputRejectReason::TooFar)
        );

        assert!(matches!(
            queue
                .accept(10, command(1, 1, 12, state(EDGE_BOMB, 0, 0)))
                .outcome,
            InputAcceptOutcome::Accepted { .. }
        ));
        assert_eq!(
            queue
                .accept(10, command(1, 2, 12, state(EDGE_BOMB, 4097, 0)))
                .outcome,
            InputAcceptOutcome::Rejected(InputRejectReason::EdgeCollision)
        );
        let resolved = queue.resolve_tick(12);
        assert_eq!(resolved.len(), 1);
        assert_eq!(resolved[0].sequence, 1);
        assert_eq!(resolved[0].state.move_axes(), (0, 0));

        let mut full = InputQueue::new(SESSION).unwrap();
        for client in 1..=INPUT_QUEUE_CAPACITY as u64 {
            assert!(matches!(
                full.accept(0, command(client, 1, 1, InputStateDraftV1::neutral()))
                    .outcome,
                InputAcceptOutcome::Accepted { .. }
            ));
        }
        assert_eq!(
            full.accept(0, command(999, 1, 1, InputStateDraftV1::neutral()))
                .outcome,
            InputAcceptOutcome::Rejected(InputRejectReason::QueueFull)
        );
        assert_eq!(full.staged_len(), INPUT_QUEUE_CAPACITY);
    }

    #[test]
    fn same_target_merges_distinct_edges_and_uses_highest_continuous_sequence_once() {
        let mut queue = InputQueue::new(SESSION).unwrap();
        assert!(matches!(
            queue
                .accept(20, command(3, 10, 22, state(EDGE_BOMB, 4097, 0)))
                .outcome,
            InputAcceptOutcome::Accepted {
                merged_same_target: false
            }
        ));
        assert!(matches!(
            queue
                .accept(20, command(3, 11, 22, state(EDGE_ITEM, 8192, 0)))
                .outcome,
            InputAcceptOutcome::Accepted {
                merged_same_target: true
            }
        ));
        let resolved = queue.resolve_tick(22);
        assert_eq!(resolved.len(), 1);
        assert_eq!(resolved[0].sequence, 11);
        assert_eq!(resolved[0].state.move_axes(), (8192, 0));
        assert_eq!(resolved[0].state.edge_flags(), EDGE_BOMB | EDGE_ITEM);
        assert_eq!(resolved[0].edge_sequences, [Some(10), Some(11), None, None]);
        assert!(queue.resolve_tick(22).is_empty());
    }

    #[test]
    fn queued_expired_inputs_are_discarded_never_applied_late() {
        let mut queue = InputQueue::new(SESSION).unwrap();
        assert!(matches!(
            queue
                .accept(10, command(1, 1, 11, state(EDGE_BOMB, 0, 0)))
                .outcome,
            InputAcceptOutcome::Accepted { .. }
        ));
        assert!(queue.resolve_tick(12).is_empty());
        assert_eq!(queue.staged_len(), 0);
        assert_eq!(
            queue
                .accept(12, command(1, 1, 11, state(EDGE_BOMB, 0, 0)))
                .outcome,
            InputAcceptOutcome::Duplicate
        );
        assert_eq!(
            queue
                .accept(12, command(1, 2, 12, InputStateDraftV1::neutral()))
                .outcome,
            InputAcceptOutcome::Rejected(InputRejectReason::StaleTarget)
        );
        assert!(queue.resolve_tick(12).is_empty());
    }

    #[test]
    fn target_ticks_cannot_regress_even_when_sequence_increases() {
        let mut queue = InputQueue::new(SESSION).unwrap();
        assert!(matches!(
            queue
                .accept(10, command(1, 1, 14, InputStateDraftV1::neutral()))
                .outcome,
            InputAcceptOutcome::Accepted { .. }
        ));
        assert_eq!(
            queue
                .accept(10, command(1, 2, 13, InputStateDraftV1::neutral()))
                .outcome,
            InputAcceptOutcome::Rejected(InputRejectReason::StaleTarget)
        );
        assert_eq!(queue.staged_len(), 1);
    }

    #[test]
    fn drain_transport_is_limited_to_sixty_four_per_tick() {
        let mut transport = LoopbackTransport::new();
        let mut queue = InputQueue::new(SESSION).unwrap();
        for sequence in 1..=100 {
            transport
                .send(command(sequence, 1, 2, InputStateDraftV1::neutral()))
                .unwrap();
        }
        let first = queue.drain_transport(0, &mut transport);
        assert_eq!(first.len(), INPUT_DRAIN_BUDGET);
        assert_eq!(transport.len(), 100 - INPUT_DRAIN_BUDGET);
        assert_eq!(queue.staged_len(), INPUT_DRAIN_BUDGET);
    }

    #[test]
    fn dedupe_history_is_bounded() {
        let mut queue = InputQueue::new(SESSION).unwrap();
        for sequence in 1..=INPUT_DEDUPE_HISTORY as u64 + 10 {
            let client = sequence;
            let command = command(client, 1, sequence, InputStateDraftV1::neutral());
            assert!(matches!(
                queue.accept(sequence, command).outcome,
                InputAcceptOutcome::Accepted { .. }
            ));
            queue.resolve_tick(sequence);
        }
        assert_eq!(queue.dedupe_len(), INPUT_DEDUPE_HISTORY);
    }

    #[test]
    fn edge_planner_emits_rising_edges_once_and_orders_rapid_second_press() {
        let mut planner = EdgePlanner::new();
        assert_eq!(
            planner.plan(100, EDGE_BOMB).unwrap(),
            vec![PlannedEdge {
                target_tick: 102,
                edge_flags: EDGE_BOMB
            }]
        );
        assert!(planner.plan(100, EDGE_BOMB).unwrap().is_empty());
        assert!(planner.plan(100, 0).unwrap().is_empty());
        assert_eq!(
            planner.plan(100, EDGE_BOMB).unwrap(),
            vec![PlannedEdge {
                target_tick: 103,
                edge_flags: EDGE_BOMB
            }]
        );
        assert!(planner.plan(u64::MAX, 0).is_err());
    }

    #[test]
    fn edge_planner_groups_distinct_edges_and_pause_sync_does_not_create_stale_edge() {
        let mut planner = EdgePlanner::new();
        assert_eq!(
            planner.plan(5, EDGE_BOMB | EDGE_ITEM).unwrap(),
            vec![PlannedEdge {
                target_tick: 7,
                edge_flags: EDGE_BOMB | EDGE_ITEM
            }]
        );
        planner.synchronize_levels(EDGE_BOMB).unwrap();
        assert!(planner.plan(10, EDGE_BOMB).unwrap().is_empty());
        assert!(planner.plan(10, 0).unwrap().is_empty());
        assert_eq!(planner.plan(10, EDGE_BOMB).unwrap()[0].target_tick, 12);
        assert!(planner.synchronize_levels(0x80).is_err());
    }
}
