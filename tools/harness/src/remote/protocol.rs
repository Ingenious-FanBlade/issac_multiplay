//! ICSN v2 的独立 Rust 线格式；只处理稳定值，不读取游戏或 C++ 结构。
use crate::Result;
use serde_json::{Value, json};
pub const BUILD: &[u8] = b"isaac.cs.tx-04-09";
pub const MAX_PACKET: usize = 1200;
pub type Id = [u8; 16];
const LIMIT: u64 = u64::MAX;

// 所有读取都先由报文/负载精确长度约束，整数显式小端。
fn u16_at(b: &[u8], n: usize) -> u16 {
    u16::from_le_bytes(b[n..n + 2].try_into().unwrap())
}
fn u32_at(b: &[u8], n: usize) -> u32 {
    u32::from_le_bytes(b[n..n + 4].try_into().unwrap())
}
fn u64_at(b: &[u8], n: usize) -> u64 {
    u64::from_le_bytes(b[n..n + 8].try_into().unwrap())
}
fn put(b: &mut [u8], n: usize, v: u64, width: usize) {
    b[n..n + width].copy_from_slice(&v.to_le_bytes()[..width]);
}
fn sequence(v: u64) -> bool {
    v > 0 && v < LIMIT
}
fn nonzero(v: &Id) -> bool {
    v.iter().any(|b| *b != 0)
}
fn need(ok: bool, label: &str) -> Result<()> {
    if ok { Ok(()) } else { Err(label.into()) }
}

// CRC 只检查网络损坏，不用于项目文件、构建或状态校验。
fn crc(b: &[u8]) -> u32 {
    let mut value = !0u32;
    for (i, byte) in b.iter().enumerate() {
        value ^= if (56..60).contains(&i) {
            0
        } else {
            *byte as u32
        };
        for _ in 0..8 {
            value = (value >> 1) ^ if value & 1 != 0 { 0xedb88320 } else { 0 };
        }
    }
    !value
}

// V2 的 24 字节状态内容；轴为有符号量，边沿不隐含重复按住。
#[derive(Clone, Copy, Default, Debug, PartialEq, Eq)]
pub struct State {
    pub held: u32,
    pub edge: u32,
    pub movement: [i16; 2],
    pub aim: [i16; 2],
}
impl State {
    fn valid(&self) -> bool {
        let vector = |v: [i16; 2]| {
            v.iter().all(|a| *a != i16::MIN)
                && v.iter().map(|a| i64::from(*a).pow(2)).sum::<i64>() <= 32767i64.pow(2)
        };
        self.held & !15 == 0
            && self.edge & !15 == 0
            && vector(self.movement)
            && vector(self.aim)
            && ((self.held & 1 != 0) == (self.aim != [0, 0]))
    }
    pub fn value(&self) -> Value {
        json!({"held":self.held,"edge":self.edge,"move":self.movement,"aim":self.aim})
    }
}

// 命令只持 session 内递增序号、目标 tick 与 canonical 状态。
#[derive(Clone, Debug)]
pub struct Command {
    pub sequence: u64,
    pub target: u64,
    pub state: State,
}
// Hello 与 Ack 共用协商块；仅用于识别，不提供认证。
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Agreement {
    pub build: [u8; 32],
    pub nonce: Id,
    pub peer: Id,
    pub role: u16,
}
impl Agreement {
    pub fn hello(nonce: Id) -> Self {
        let mut build = [0; 32];
        build[..BUILD.len()].copy_from_slice(BUILD);
        Self {
            build,
            nonce,
            peer: [0; 16],
            role: 2,
        }
    }
    fn valid(&self, agreement: bool) -> bool {
        let end = self.build.iter().position(|b| *b == 0).unwrap_or(32);
        end > 0
            && end < 32
            && self.build[..end].iter().all(|b| (33..=126).contains(b))
            && self.build[end..].iter().all(|b| *b == 0)
            && nonzero(&self.nonce)
            && if agreement {
                self.role == 1 && nonzero(&self.peer) && self.nonce != self.peer
            } else {
                self.role == 2 && !nonzero(&self.peer)
            }
    }
    fn encode(&self) -> Vec<u8> {
        let mut b = vec![0; 120];
        b[..32].copy_from_slice(&self.build);
        b[32..48].copy_from_slice(&self.nonce);
        b[48..64].copy_from_slice(&self.peer);
        put(&mut b, 64, 2, 2);
        put(&mut b, 66, 2, 2);
        put(&mut b, 68, self.role.into(), 2);
        put(&mut b, 72, 13, 8);
        put(&mut b, 80, 1200, 4);
        put(&mut b, 88, 1, 8);
        put(&mut b, 96, 2, 8);
        put(&mut b, 104, 3, 8);
        put(&mut b, 112, 4, 8);
        b
    }
    fn decode(b: &[u8]) -> Result<Self> {
        need(b.len() == 120, "agreement_length")?;
        need(u16_at(b, 70) == 0 && u32_at(b, 84) == 0, "reserved")?;
        need(
            u16_at(b, 64) == 2
                && u16_at(b, 66) == 2
                && u64_at(b, 72) == 13
                && u32_at(b, 80) == 1200
                && u64_at(b, 88) == 1
                && u64_at(b, 96) == 2 && u64_at(b, 104) == 3 && u64_at(b, 112) == 4,
            "incompatible",
        )?;
        Ok(Self {
            build: b[..32].try_into().unwrap(),
            nonce: b[32..48].try_into().unwrap(),
            peer: b[48..64].try_into().unwrap(),
            role: u16_at(b, 68),
        })
    }
}

// 主机进度快照；高水位与本次实际来源序号分别保存。
#[derive(Clone, Debug)]
pub struct Ack {
    pub client: u64,
    pub roster: u32,
    pub tick: u64,
    pub accepted: u64,
    pub processed: u64,
    pub processed_tick: u64,
    pub source: u64,
    pub generation: u64,
    pub rejected: u64,
    pub mask: u32,
    pub state: u16,
    pub purpose: u16,
    pub hello: u64,
    pub agreement: Agreement,
}
impl Ack {
    // 首包必须是零进度 Warmup；后续进度允许 processed=0 的中性模拟。
    fn valid(&self) -> bool {
        (2..=4).contains(&self.client) && self.roster & 1 != 0 && self.roster & !15 == 0
            && self.agreement.valid(true)
            && self.tick < LIMIT
            && self.accepted < LIMIT
            && self.processed <= self.accepted
            && self.source <= self.processed
            && self.processed_tick <= self.tick
            && (2..=6).contains(&self.state)
            && self.mask & !0x7ff8 == 0
            && self.rejected >= self.mask.count_ones().into()
            && ((self.rejected == 0) == (self.mask == 0))
            && (self.processed_tick == 0 || self.generation > 0)
            && (self.processed == 0 || (self.processed_tick > 0 && self.generation > 0))
            && if self.purpose == 1 {
                sequence(self.hello)
                    && self.state == 2
                    && self.tick == 0
                    && self.accepted == 0
                    && self.processed == 0
                    && self.processed_tick == 0
                    && self.generation == 0
                    && self.rejected == 0
                    && self.mask == 0
            } else {
                self.purpose == 2 && self.hello == 0
            }
    }
    pub fn advances(&self, next: &Self) -> bool {
        next.tick >= self.tick
            && next.accepted >= self.accepted
            && next.processed >= self.processed
            && next.processed_tick >= self.processed_tick
            && next.generation >= self.generation
            && next.rejected >= self.rejected
            && next.state >= self.state
            && next.mask & self.mask == self.mask
    }
    pub fn value(&self) -> Value {
        json!({"client":self.client,"roster":self.roster,"tick":self.tick,"accepted":self.accepted,"processed":self.processed,
        "processed_tick":self.processed_tick,"source":self.source,"generation":self.generation,
        "rejected":self.rejected,"mask":self.mask,"state":self.state,"purpose":self.purpose})
    }
}
// 四类独立 payload；编码前后均检查精确长度和 reserved。
#[derive(Clone, Debug)]
pub enum Body {
    Hello(Agreement),
    Input {
        last_ack: u64,
        commands: Vec<Command>,
    },
    Ack(Ack),
    Bye {
        reason: u16,
        last_ack: u64,
    },
}
// 网络身份不含本地 Player/device 指针或临时绑定 generation。
#[derive(Clone, Debug)]
pub struct Message {
    pub sender: u64,
    pub session: Id,
    pub role: u16,
    pub sequence: u64,
    pub body: Body,
}
impl Message {
    pub fn kind(&self) -> u16 {
        match self.body {
            Body::Hello(_) => 1,
            Body::Input { .. } => 2,
            Body::Ack(_) => 3,
            Body::Bye { .. } => 5,
        }
    }
    pub fn value(&self) -> Value {
        let content = match &self.body {
            Body::Hello(_) => json!({"build":String::from_utf8_lossy(BUILD)}),
            Body::Input { last_ack, commands } => {
                json!({"last_ack":last_ack,"commands":commands.iter().map(|c|
                json!({"sequence":c.sequence,"target":c.target,"state":c.state.value()})).collect::<Vec<_>>()})
            }
            Body::Ack(a) => a.value(),
            Body::Bye { reason, last_ack } => json!({"reason":reason,"last_ack":last_ack}),
        };
        json!({"sender":self.sender,"kind":self.kind(),"role":self.role,"sequence":self.sequence,"session":hex(&self.session),"content":content})
    }
}
pub fn hex(b: &[u8]) -> String {
    b.iter().map(|b| format!("{b:02x}")).collect()
}
pub fn unhex(s: &str) -> Result<Vec<u8>> {
    need(s.len() % 2 == 0 && s.is_ascii(), "hex")?;
    (0..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16).map_err(|_| "hex".into()))
        .collect()
}

// 输入冗余按 sequence 递增，状态和身份逐条检查；成功前不提交任何 Endpoint 状态。
pub fn encode(m: &Message) -> Result<Vec<u8>> {
    let mut payload = match &m.body {
        Body::Hello(h) => {
            need(h.valid(false), "hello")?;
            h.encode()
        }
        Body::Input { last_ack, commands } => {
            need(
                (1..=4).contains(&commands.len()) && *last_ack < LIMIT,
                "input_count",
            )?;
            let mut p = vec![0; 16 + 64 * commands.len()];
            put(&mut p, 0, commands.len() as u64, 2);
            put(&mut p, 8, *last_ack, 8);
            let mut previous = 0;
            for (i, c) in commands.iter().enumerate() {
                need(
                    sequence(c.sequence)
                        && c.sequence > previous
                        && sequence(c.target)
                        && c.state.valid(),
                    "input_state",
                )?;
                previous = c.sequence;
                let n = 16 + i * 64;
                p[n..n + 16].copy_from_slice(&m.session);
                put(&mut p, n + 16, m.sender, 8);
                put(&mut p, n + 24, c.sequence, 8);
                put(&mut p, n + 32, c.target, 8);
                put(&mut p, n + 40, 2, 2);
                put(&mut p, n + 42, 24, 2);
                put(&mut p, n + 44, c.state.held.into(), 4);
                put(&mut p, n + 48, c.state.edge.into(), 4);
                for (j, v) in c
                    .state
                    .movement
                    .iter()
                    .chain(c.state.aim.iter())
                    .enumerate()
                {
                    put(&mut p, n + 52 + j * 2, *v as u16 as u64, 2);
                }
            }
            p
        }
        Body::Bye { reason, last_ack } => {
            need((1..=7).contains(reason) && *last_ack < LIMIT, "bye")?;
            let mut p = vec![0; 16];
            put(&mut p, 0, (*reason).into(), 2);
            put(&mut p, 8, *last_ack, 8);
            p
        }
        Body::Ack(a) => {
            need(a.valid(), "ack_payload")?;
            let mut p = vec![0; 88];
            for (i, v) in [
                a.tick,
                a.client,
                a.accepted,
                a.processed,
                a.processed_tick,
                a.source,
                a.generation,
                a.rejected,
            ]
            .iter()
            .enumerate()
            {
                put(&mut p, i * 8, *v, 8);
            }
            put(&mut p, 64, a.mask.into(), 4);
            put(&mut p, 68, a.state.into(), 2);
            put(&mut p, 70, a.purpose.into(), 2);
            put(&mut p, 72, a.hello, 8);
            put(&mut p, 80, a.roster.into(), 4);
            p.extend(a.agreement.encode());
            p
        }
    };
    need(
        (m.role == 1 && m.sender == 1 || m.role == 2 && (2..=4).contains(&m.sender)) && sequence(m.sequence),
        "sender",
    )?;
    need(
        match m.body {
            Body::Hello(_) | Body::Input { .. } => m.role == 2,
            Body::Ack(_) => m.role == 1,
            Body::Bye { .. } => true,
        },
        "role",
    )?;
    need(
        if m.kind() == 1 {
            !nonzero(&m.session)
        } else {
            nonzero(&m.session)
        },
        "session",
    )?;
    let mut b = vec![0; 64];
    b[..4].copy_from_slice(b"ICSN");
    put(&mut b, 4, 2, 2);
    put(&mut b, 6, m.kind().into(), 2);
    put(&mut b, 8, 64, 2);
    put(&mut b, 10, m.role.into(), 2);
    b[16..32].copy_from_slice(&m.session);
    put(&mut b, 32, m.sender, 8);
    put(&mut b, 40, m.sequence, 8);
    put(&mut b, 48, payload.len() as u64, 4);
    b.append(&mut payload);
    let crc = crc(&b);
    put(&mut b, 56, crc.into(), 4);
    Ok(b)
}
pub fn decode(b: &[u8]) -> Result<Message> {
    need((64..=MAX_PACKET).contains(&b.len()), "length")?;
    need(&b[..4] == b"ICSN", "magic")?;
    need(u16_at(b, 4) == 2, "version")?;
    need(
        u16_at(b, 8) == 64 && b.len() == 64 + u32_at(b, 48) as usize,
        "header",
    )?;
    need(u32_at(b, 12) == 0, "flags")?;
    need(u32_at(b, 52) == 0 && u32_at(b, 60) == 0, "reserved")?;
    need(u32_at(b, 56) == crc(b), "crc")?;
    let role = u16_at(b, 10);
    let sender = u64_at(b, 32);
    let sequence = u64_at(b, 40);
    let session: Id = b[16..32].try_into().unwrap();
    need(
        (role == 1 && sender == 1) || (role == 2 && (2..=4).contains(&sender)),
        "identity",
    )?;
    need(self::sequence(sequence), "sequence")?;
    let p = &b[64..];
    let body = match u16_at(b, 6) {
        1 => {
            need(role == 2 && !nonzero(&session), "hello_identity")?;
            let h = Agreement::decode(p)?;
            need(h.valid(false), "hello")?;
            Body::Hello(h)
        }
        2 => {
            need(
                role == 2 && nonzero(&session) && p.len() >= 16,
                "input_length_or_role",
            )?;
            let count = u16_at(p, 0) as usize;
            need(
                (1..=4).contains(&count) && p.len() == 16 + 64 * count,
                "input_count",
            )?;
            need(u16_at(p, 2) == 0 && u32_at(p, 4) == 0, "reserved")?;
            let last_ack = u64_at(p, 8);
            need(last_ack < LIMIT, "last_ack")?;
            let mut commands = vec![];
            let mut previous = 0;
            for i in 0..count {
                let q = &p[16 + i * 64..16 + (i + 1) * 64];
                need(q[..16] == session && u64_at(q, 16) == sender, "command_identity")?;
                let sequence = u64_at(q, 24);
                let target = u64_at(q, 32);
                let state = State {
                    held: u32_at(q, 44),
                    edge: u32_at(q, 48),
                    movement: [u16_at(q, 52) as i16, u16_at(q, 54) as i16],
                    aim: [u16_at(q, 56) as i16, u16_at(q, 58) as i16],
                };
                need(
                    self::sequence(sequence) && sequence > previous && self::sequence(target),
                    "command_sequence",
                )?;
                need(
                    u16_at(q, 40) == 2
                        && u16_at(q, 42) == 24
                        && u32_at(q, 60) == 0
                        && state.valid(),
                    "input_state",
                )?;
                commands.push(Command {
                    sequence,
                    target,
                    state,
                });
                previous = sequence;
            }
            Body::Input { last_ack, commands }
        }
        3 => {
            need(
                role == 1 && nonzero(&session) && p.len() == 208,
                "ack_length_or_role",
            )?;
            need(u32_at(p, 84) == 0, "ack_reserved")?;
            let a = Ack {
                client: u64_at(p, 8),
                roster: u32_at(p, 80),
                tick: u64_at(p, 0),
                accepted: u64_at(p, 16),
                processed: u64_at(p, 24),
                processed_tick: u64_at(p, 32),
                source: u64_at(p, 40),
                generation: u64_at(p, 48),
                rejected: u64_at(p, 56),
                mask: u32_at(p, 64),
                state: u16_at(p, 68),
                purpose: u16_at(p, 70),
                hello: u64_at(p, 72),
                agreement: Agreement::decode(&p[88..])?,
            };
            need(a.valid(), "ack_payload")?;
            Body::Ack(a)
        }
        5 => {
            need(nonzero(&session) && p.len() == 16, "bye_length")?;
            let reason = u16_at(p, 0);
            let last_ack = u64_at(p, 8);
            need(
                (1..=7).contains(&reason)
                    && last_ack < LIMIT
                    && u16_at(p, 2) == 0
                    && u32_at(p, 4) == 0,
                "bye_payload",
            )?;
            Body::Bye { reason, last_ack }
        }
        _ => return Err("kind".into()),
    };
    Ok(Message {
        sender,
        session,
        role,
        sequence,
        body,
    })
}
