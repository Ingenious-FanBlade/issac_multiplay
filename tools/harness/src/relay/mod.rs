//! 单轮双 socket UDP relay；只处理报文字节与故障决定，不访问游戏或控制口。
use crate::{Result, remote::protocol};
use serde_json::{Value, json};
use std::{
    fs::{File, OpenOptions},
    io::Write,
    net::{SocketAddr, UdpSocket},
    path::Path,
    sync::mpsc::{self, Receiver, SyncSender},
    thread::{self, JoinHandle},
    time::{Duration, Instant},
};

// 固定档位和独立方向 PRNG；切换只影响新到报文，已排队副本保留原决定。
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Profile {
    Clean,
    Moderate,
    OutageShort,
    HandshakeLoss,
    OutageLong,
}
impl Profile {
    fn name(self) -> &'static str {
        match self {
            Self::Clean => "clean",
            Self::Moderate => "moderate",
            Self::OutageShort => "outage_short",
            Self::HandshakeLoss => "handshake_loss",
            Self::OutageLong => "outage_long",
        }
    }
}
#[derive(Clone, Default)]
struct Counts {
    received: u64,
    policy_drop: u64,
    copies_scheduled: u64,
    send_ok: u64,
    send_error: u64,
    shutdown_drop: u64,
    duplicate: u64,
    reorder: u64,
}
impl Counts {
    fn value(&self, pending: usize) -> Value {
        json!({"received":self.received,"policy_drop":self.policy_drop,
        "copies_scheduled":self.copies_scheduled,"send_ok":self.send_ok,"send_error":self.send_error,
        "pending":pending,"shutdown_drop":self.shutdown_drop,"duplicate":self.duplicate,"reorder":self.reorder})
    }
}
// occurrence 是每次真实接收；同序号 Hello/Bye 重发与其副本分别记账。
struct Pending {
    due: Instant,
    bytes: Vec<u8>,
    direction: usize,
    occurrence: u64,
    copy: u64,
    epoch: u64,
}
enum Request {
    LockRemote(SocketAddr, mpsc::Sender<Result<()>>),
    Switch(Profile, mpsc::Sender<Result<Value>>),
    Drain(mpsc::Sender<Result<Value>>),
    Finish,
}
pub struct Relay {
    tx: SyncSender<Request>,
    worker: Option<JoinHandle<Result<Value>>>,
    pub address: SocketAddr,
}
impl Relay {
    pub fn start(
        host: SocketAddr,
        path: &Path,
        seed: u64,
        capacity: usize,
        initial: Profile,
    ) -> Result<Self> {
        if !host.ip().is_loopback()
            || !host.is_ipv4()
            || seed == 0
            || !(64..=65536).contains(&capacity)
        {
            return Err("invalid_relay_config".into());
        }
        let remote = UdpSocket::bind("127.0.0.1:0").map_err(|e| e.to_string())?;
        let authority = UdpSocket::bind("127.0.0.1:0").map_err(|e| e.to_string())?;
        authority.connect(host).map_err(|e| e.to_string())?;
        remote.set_nonblocking(true).map_err(|e| e.to_string())?;
        authority.set_nonblocking(true).map_err(|e| e.to_string())?;
        let address = remote.local_addr().map_err(|e| e.to_string())?;
        let log = OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(path)
            .map_err(|e| e.to_string())?;
        let (tx, rx) = mpsc::sync_channel(32);
        let worker=thread::Builder::new().name("icsn-relay".into()).spawn(move|| {
            let mut w=Worker { remote,authority,peer:None,host,log,clock:Instant::now(),profile:initial,
                epoch:1, control:0, random:[seed,seed^0x9e3779b97f4a7c15], counts:Default::default(),
                arrivals:[0;2],pending:vec![],capacity,foreign:0,short_until:None,udp_notices:[0;2] };
            if w.random[1]==0 {w.random[1]=0xd1b54a32d192ed03;}
            w.record(json!({"event":"start","seed":seed,"prng":"xorshift64-v1","direction_seeds":w.random,
                "host":host,"remote_socket":address,"host_socket":w.authority.local_addr().map_err(|e|e.to_string())?,
                "capacity":capacity,"profile":initial.name(),"profile_epoch":1}))?;
            w.run(rx)
        }).map_err(|e|e.to_string())?;
        Ok(Self {
            tx,
            worker: Some(worker),
            address,
        })
    }
    // 返回值来自 relay 消费消息的实际时刻，不用控制线程发消息时间代替。
    pub fn lock_remote(&self, address: SocketAddr) -> Result<()> {
        let (tx, rx) = mpsc::channel();
        self.tx
            .try_send(Request::LockRemote(address, tx))
            .map_err(|e| e.to_string())?;
        rx.recv_timeout(Duration::from_secs(3))
            .map_err(|e| e.to_string())?
    }
    pub fn switch(&self, profile: Profile) -> Result<Value> {
        let (tx, rx) = mpsc::channel();
        self.tx
            .try_send(Request::Switch(profile, tx))
            .map_err(|e| e.to_string())?;
        rx.recv_timeout(Duration::from_secs(3))
            .map_err(|e| e.to_string())?
    }
    pub fn drain(&self) -> Result<Value> {
        let (tx, rx) = mpsc::channel();
        self.tx
            .try_send(Request::Drain(tx))
            .map_err(|e| e.to_string())?;
        rx.recv_timeout(Duration::from_secs(12))
            .map_err(|e| e.to_string())?
    }
    pub fn finish(mut self) -> Result<Value> {
        // 已退出的线程仍需 join 取得原始错误；不让控制通道断开覆盖网络失败。
        let _ = self.tx.send(Request::Finish);
        self.worker
            .take()
            .ok_or("relay_already_joined")?
            .join()
            .map_err(|_| "relay_panic".to_string())?
    }
}
impl Drop for Relay {
    fn drop(&mut self) {
        if let Some(w) = self.worker.take() {
            let _ = self.tx.try_send(Request::Finish);
            let _ = w.join();
        }
    }
}
struct Worker {
    remote: UdpSocket,
    authority: UdpSocket,
    peer: Option<SocketAddr>,
    host: SocketAddr,
    log: File,
    clock: Instant,
    profile: Profile,
    epoch: u64,
    control: u64,
    random: [u64; 2],
    counts: [Counts; 2],
    arrivals: [u64; 2],
    pending: Vec<Pending>,
    capacity: usize,
    foreign: u64,
    short_until: Option<Instant>,
    // Winsock UDP 异步通知单独计数，不伪装成已收到报文或合法 Bye。
    udp_notices: [u64; 2],
}
impl Worker {
    fn record(&mut self, mut v: Value) -> Result<()> {
        v["elapsed_us"] = json!(self.clock.elapsed().as_micros());
        writeln!(self.log, "{v}").map_err(|e| e.to_string())
    }
    fn random(&mut self, d: usize) -> u64 {
        let mut x = self.random[d];
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        self.random[d] = x;
        x
    }
    fn summary(&self) -> Value {
        json!({"directions":[self.counts[0].value(self.pending.iter().filter(|p|p.direction==0).count()),
        self.counts[1].value(self.pending.iter().filter(|p|p.direction==1).count())],"foreign":self.foreign,"peer":self.peer,"host":self.host,
        "udp_notices":self.udp_notices})
    }
    fn arrive(&mut self, d: usize, b: Vec<u8>) -> Result<()> {
        self.arrivals[d] += 1;
        let occurrence = self.arrivals[d];
        self.counts[d].received += 1;
        // 每次决定固定消耗四个随机数，失败或丢弃也不重新抽签。
        let draws = [
            self.random(d),
            self.random(d),
            self.random(d),
            self.random(d),
        ];
        let outage = self.profile == Profile::OutageLong
            || self.short_until.is_some_and(|t| Instant::now() < t);
        let loss = match self.profile {
            Profile::Moderate => 50000,
            Profile::HandshakeLoss => 300000,
            _ => 0,
        };
        let drop = outage || draws[0] % 1000000 < loss;
        let duplicate = !drop && self.profile == Profile::Moderate && draws[1] % 1000000 < 20000;
        let reorder = !drop && self.profile == Profile::Moderate && draws[2] % 1000000 < 20000;
        let delay = if self.profile == Profile::Moderate {
            10 + draws[3] % 21 + if reorder { 60 } else { 0 }
        } else {
            0
        };
        let copies = if drop {
            0
        } else {
            if duplicate { 2 } else { 1 }
        };
        self.record(
            json!({"event":"receive","direction":d,"occurrence_id":occurrence,"length":b.len(),
            "bytes":protocol::hex(&b),"packet":protocol::decode(&b).ok().map(|m|m.value()),
            "profile":self.profile.name(),"profile_epoch":self.epoch,"draws":draws,
            "decision":if outage {"outage"}else if drop {"drop"}else{"forward"},
            "duplicate":duplicate,"reorder":reorder,"copies":copies,"delay_ms":delay}),
        )?;
        if drop {
            self.counts[d].policy_drop += 1;
            return Ok(());
        }
        if self.pending.len() + copies > self.capacity {
            return Err("relay_queue_overflow".into());
        }
        self.counts[d].duplicate += u64::from(duplicate);
        self.counts[d].reorder += u64::from(reorder);
        for copy in 0..copies {
            let due = Instant::now() + Duration::from_millis(delay + copy as u64);
            self.record(json!({"event":"schedule","direction":d,"occurrence_id":occurrence,"copy_id":copy,
                "profile_epoch":self.epoch,"planned_us":due.duration_since(self.clock).as_micros()}))?;
            self.pending.push(Pending {
                due,
                bytes: b.clone(),
                direction: d,
                occurrence,
                copy: copy as u64,
                epoch: self.epoch,
            });
            self.counts[d].copies_scheduled += 1;
        }
        Ok(())
    }
    fn pump(&mut self) -> Result<()> {
        for d in 0..2 {
            for _ in 0..64 {
                let mut b = [0; 65536];
                let received = if d == 0 {
                    self.remote.recv_from(&mut b)
                } else {
                    self.authority.recv(&mut b).map(|n| (n, self.host))
                };
                match received {
                    Ok((n, from)) => {
                        if d == 0 {
                            if self.peer != Some(from) {
                                self.foreign += 1;
                                self.record(json!({"event":"foreign","address":from,"length":n}))?;
                                continue;
                            }
                        }
                        self.arrive(d, b[..n].to_vec())?;
                    }
                    Err(e) if e.kind() == std::io::ErrorKind::WouldBlock => break,
                    // 与原生 Worker 一致：UDP 的连接/网络重置通知不等于合法终止包。
                    // 对端退休后的迟到副本也可能触发；保存原错误，交由协议/超时判定。
                    Err(e) if matches!(e.raw_os_error(), Some(10054 | 10052)) => {
                        self.udp_notices[d] += 1;
                        self.record(json!({"event":"udp_notice","direction":d,
                            "native_error":e.raw_os_error(),"error":e.to_string(),
                            "count":self.udp_notices[d]}))?;
                        break; // 每轮处理有界，不能在错误就绪的 socket 上忙等。
                    }
                    Err(e) => return Err(format!("relay_receive:{e}")),
                }
            }
        }
        self.pending.sort_by_key(|p| p.due);
        while self
            .pending
            .first()
            .is_some_and(|p| p.due <= Instant::now())
        {
            let p = self.pending.remove(0);
            let result = if p.direction == 0 {
                self.authority.send(&p.bytes)
            } else {
                self.peer
                    .ok_or_else(|| std::io::Error::other("remote_not_locked"))
                    .and_then(|a| self.remote.send_to(&p.bytes, a))
            };
            let ok = result.as_ref().is_ok_and(|n| *n == p.bytes.len());
            if ok {
                self.counts[p.direction].send_ok += 1;
            } else {
                self.counts[p.direction].send_error += 1;
            }
            self.record(json!({"event":"send","direction":p.direction,"occurrence_id":p.occurrence,"copy_id":p.copy,
                "profile_epoch":p.epoch,"planned_us":p.due.duration_since(self.clock).as_micros(),"ok":ok,
                "error":if ok {None}else{Some(format!("{result:?}"))}}))?;
        }
        Ok(())
    }
    fn run(&mut self, rx: Receiver<Request>) -> Result<Value> {
        let mut finish = None;
        let mut drain: Option<(Instant, mpsc::Sender<Result<Value>>)> = None;
        loop {
            for _ in 0..32 {
                match rx.try_recv() {
                    Ok(Request::LockRemote(address, reply)) => {
                        let result = if self.peer.is_some()
                            || !address.ip().is_loopback()
                            || !address.is_ipv4()
                        {
                            Err("relay_remote_lock_invalid".into())
                        } else {
                            self.peer = Some(address);
                            self.record(json!({"event":"remote_locked","address":address}))
                        };
                        let _ = reply.send(result);
                    }
                    Ok(Request::Switch(profile, reply)) => {
                        self.profile = profile;
                        self.epoch += 1;
                        self.control += 1;
                        self.short_until = if profile == Profile::OutageShort {
                            Some(Instant::now() + Duration::from_millis(1000))
                        } else {
                            None
                        };
                        let v = json!({"event":"profile","profile":profile.name(),"profile_epoch":self.epoch,
                        "control_sequence":self.control,"effective_us":self.clock.elapsed().as_micros(),"summary":self.summary()});
                        self.record(v.clone())?;
                        self.log.flush().map_err(|e| e.to_string())?;
                        let _ = reply.send(Ok(v));
                    }
                    Ok(Request::Drain(reply)) => {
                        if drain.is_some() {
                            let _ = reply.send(Err("relay_drain_busy".into()));
                        } else {
                            drain = Some((Instant::now() + Duration::from_secs(11), reply));
                        }
                    }
                    Ok(Request::Finish) => {
                        finish.get_or_insert(Instant::now() + Duration::from_secs(11));
                    }
                    Err(mpsc::TryRecvError::Empty) => break,
                    Err(mpsc::TryRecvError::Disconnected) => {
                        finish.get_or_insert(Instant::now() + Duration::from_secs(11));
                        break;
                    }
                }
            }
            if let Err(error) = self.pump() {
                self.record(json!({"event":"error","error":error}))?;
                for packet in std::mem::take(&mut self.pending) {
                    self.counts[packet.direction].shutdown_drop += 1;
                    self.record(json!({"event":"shutdown_drop","direction":packet.direction,
                        "occurrence_id":packet.occurrence,"copy_id":packet.copy,"reason":"worker_error"}))?;
                }
                self.record(json!({"event":"summary","error":error,"summary":self.summary()}))?;
                self.log.flush().map_err(|e| e.to_string())?;
                return Err(error);
            }
            if drain
                .as_ref()
                .is_some_and(|(end, _)| self.pending.is_empty() || Instant::now() >= *end)
            {
                let (_, reply) = drain.take().unwrap();
                let result = if self.pending.is_empty() {
                    Ok(self.summary())
                } else {
                    Err("relay_drain_timeout".into())
                };
                let _ = reply.send(result);
            }
            if finish.is_some_and(|end| Instant::now() >= end)
                || (finish.is_some() && self.pending.is_empty())
            {
                // 两端停止后仍保留 100 ms 接收窗口，避免三份 Bye 尾包遗失在本地 socket。
                if finish.is_some_and(|end| {
                    end.saturating_duration_since(Instant::now()) > Duration::from_millis(10900)
                }) {
                    thread::sleep(Duration::from_millis(2));
                    continue;
                }
                for p in std::mem::take(&mut self.pending) {
                    self.counts[p.direction].shutdown_drop += 1;
                    self.record(json!({"event":"shutdown_drop","direction":p.direction,"occurrence_id":p.occurrence,"copy_id":p.copy}))?;
                }
                let v = self.summary();
                self.record(json!({"event":"summary","summary":v}))?;
                self.log.flush().map_err(|e| e.to_string())?;
                return Ok(v);
            }
            thread::sleep(Duration::from_millis(2));
        }
    }
}
