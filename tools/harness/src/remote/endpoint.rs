//! 一次性 Remote Endpoint；握手缓存与进度缓存分离，旧包不续命。
use super::protocol::{self, Ack, Agreement, Body, Command, Id, Message};
use crate::Result;
use std::time::{Duration, Instant};

pub enum Reception {
    Delivered(Message),
    Duplicate,
    Stale,
    RetiredIgnored,
}
// 每轮创建新对象，First Ack 缓存常驻至本轮结束，不复用退休身份。
pub struct Endpoint {
    pub session: Id,
    pub ordinal: u32,
    pub ack: Option<Ack>,
    pub last_ack: u64,
    pub established: bool,
    pub retired: bool,
    pub establishments: u64,
    hello: Agreement,
    hello_bytes: Vec<u8>,
    first_ack: Vec<u8>,
    latest: Vec<u8>,
    bye: Vec<u8>,
    high: u64,
    next: u64,
    last_receive: Instant,
    started: Instant,
    timeout: Duration,
}
impl Endpoint {
    pub fn new(nonce: Id, timeout: Duration, ordinal: u32) -> Result<Self> {
        if !(1..=3).contains(&ordinal) { return Err("remote_ordinal".into()); }
        let hello = Agreement::hello(nonce);
        let bytes = protocol::encode(&Message {
            sender: u64::from(ordinal) + 1,
            session: [0; 16],
            role: 2,
            sequence: 1,
            body: Body::Hello(hello.clone()),
        })?;
        Ok(Self {
            ordinal,
            session: [0; 16],
            ack: None,
            last_ack: 0,
            established: false,
            retired: false,
            establishments: 0,
            hello,
            hello_bytes: bytes,
            first_ack: vec![],
            latest: vec![],
            bye: vec![],
            high: 0,
            next: 2,
            last_receive: Instant::now(),
            started: Instant::now(),
            timeout,
        })
    }
    pub fn hello(&self) -> &[u8] {
        &self.hello_bytes
    }
    pub fn timed_out(&self) -> bool {
        !self.retired
            && if self.established {
                self.last_receive.elapsed() >= self.timeout
            } else {
                self.started.elapsed() >= self.timeout
            }
    }
    pub fn receive(&mut self, b: &[u8]) -> Result<Reception> {
        // First Ack 晚于进度 Ack 到达仍为原字节副本；不回退 last_ack 或 generation。
        if b == self.first_ack || b == self.latest || b == self.bye {
            self.last_receive = Instant::now();
            return Ok(Reception::Duplicate);
        }
        if self.retired {
            return Ok(Reception::RetiredIgnored);
        }
        let m = protocol::decode(b)?;
        if m.role != 1 || m.sender != 1 {
            return Err("role".into());
        }
        if let Body::Ack(a) = &m.body {
            if a.client != u64::from(self.ordinal) + 1 { return Err("ack_client".into()); }
        }
        match &m.body {
            Body::Ack(a) if a.purpose == 1 => {
                if self.established
                    || m.sequence <= self.high
                    || a.hello != 1
                    || a.agreement.build != self.hello.build
                    || a.agreement.peer != self.hello.nonce
                {
                    return Err("handshake".into());
                }
                self.session = m.session;
                self.established = true;
                self.establishments += 1;
                self.first_ack = b.to_vec();
            }
            Body::Ack(a) => {
                if !self.established || m.session != self.session {
                    return Err("identity_or_phase".into());
                }
                let old = self.ack.as_ref().ok_or("missing_first_ack")?;
                if a.agreement != old.agreement {
                    return Err("agreement_changed".into());
                }
                if m.sequence <= self.high {
                    return Ok(Reception::Stale);
                }
                if !old.advances(a) {
                    return Err("ack_regressed".into());
                }
            }
            Body::Bye { last_ack, .. } => {
                if !self.established || m.session != self.session || *last_ack != 0 {
                    return Err("bye_identity".into());
                }
                if m.sequence <= self.high {
                    return Ok(Reception::Stale);
                }
                self.retired = true;
                self.bye = b.to_vec();
            }
            _ => return Err("remote_receive_kind".into()),
        }
        self.high = m.sequence;
        self.last_receive = Instant::now();
        if let Body::Ack(a) = &m.body {
            self.ack = Some(a.clone());
            self.last_ack = m.sequence;
            self.latest = b.to_vec();
        }
        Ok(Reception::Delivered(m))
    }
    pub fn input(&mut self, commands: Vec<Command>) -> Result<Vec<u8>> {
        self.input_referencing(self.last_ack, commands)
    }
    // 仅测试客户端使用：新包序号、合法编码，引用本会话已收到的旧 Ack。
    pub fn input_referencing(&mut self, last_ack: u64, commands: Vec<Command>) -> Result<Vec<u8>> {
        self.prepare(Body::Input { last_ack, commands })
    }
    pub fn timeout_ms(&self) -> u128 {
        self.timeout.as_millis()
    }
    pub fn receive_age_ms(&self) -> u128 {
        self.last_receive.elapsed().as_millis()
    }
    pub fn bye(&mut self, reason: u16) -> Result<Vec<u8>> {
        self.prepare(Body::Bye {
            reason,
            last_ack: self.last_ack,
        })
    }
    // 只有完整 send 成功才递增包序号；重发 Hello/Bye 不重新编码或提交。
    fn prepare(&self, body: Body) -> Result<Vec<u8>> {
        if !self.established || self.retired {
            return Err("send_phase".into());
        }
        protocol::encode(&Message {
            sender: u64::from(self.ordinal) + 1,
            session: self.session,
            role: 2,
            sequence: self.next,
            body,
        })
    }
    pub fn sent(&mut self, bye: bool) -> Result<()> {
        self.next = self
            .next
            .checked_add(1)
            .filter(|n| *n < u64::MAX)
            .ok_or("sequence_exhausted")?;
        if bye {
            self.retired = true;
        }
        Ok(())
    }
}
