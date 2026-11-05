//! Rust 远端只持 UDP、脚本与值快照；网络任务不接触游戏、Lua 或控制口。
mod endpoint;
pub mod protocol;
use crate::Result;
use endpoint::{Endpoint, Reception};
use protocol::{Body, Command, State};
use serde_json::{Value, json};
use std::{
    collections::VecDeque,
    fs::{File, OpenOptions},
    io::Write,
    net::{SocketAddr, UdpSocket},
    path::Path,
    sync::{
        Arc, Mutex,
        mpsc::{self, Receiver, SyncSender},
    },
    thread::{self, JoinHandle},
    time::{Duration, Instant},
};

// 单个脚本步骤相对安装时最新 Ack.completed；状态持续至下一步骤，edge 仅采集一次。
#[derive(Clone)]
pub struct Step {
    pub after: u64,
    pub state: State,
}
enum Request {
    Script(String, Vec<Step>, Option<u64>),
    ArmNextGeneration(String, Vec<Step>, u64, Duration),
    OldGeneration(u64, u64),
    ExpectTimeout,
    Silence,
    Bye,
    Shutdown,
}
// 线程间只读快照；不把 Ack/generation 解释为已接触任何游戏对象。
#[derive(Clone, Default)]
pub struct Snapshot {
    pub ack: Option<protocol::Ack>,
    pub tx: u64,
    pub rx: u64,
    pub duplicates: u64,
    pub stale: u64,
    pub establishments: u64,
    pub bye: Option<u16>,
    pub error: Option<String>,
    pub finished: bool,
    pub script_label: String,
    pub script_base: u64,
    pub script_generation: u64,
    pub last_ack: u64,
    pub terminal_cause: Option<String>,
    pub expected_timeout: bool,
}
// 主线程使用有界消息切换脚本；Drop 确保错误路径也关闭并回收远端线程。
pub struct Remote {
    tx: SyncSender<Request>,
    state: Arc<Mutex<Snapshot>>,
    thread: Option<JoinHandle<()>>,
}
impl Remote {
    pub fn start(address: SocketAddr, path: &Path, timeout_ms: u64, lead: u64) -> Result<Self> {
        Self::start_locked(address, path, timeout_ms, lead, |_| Ok(()))
    }
    // socket 已绑定、尚未发送 Hello 时先登记 relay 远端地址，避免“首包抢占”或丢首包。
    pub fn start_locked(
        address: SocketAddr,
        path: &Path,
        timeout_ms: u64,
        lead: u64,
        ready: impl FnOnce(SocketAddr) -> Result<()>,
    ) -> Result<Self> {
        Self::start_participant(address, path, timeout_ms, lead, 1, ready)
    }
    // 三个独立远端显式选择冻结ordinal；不从ControllerIndex或身体数推断。
    pub fn start_participant(
        address: SocketAddr, path: &Path, timeout_ms: u64, lead: u64, ordinal: u32,
        ready: impl FnOnce(SocketAddr) -> Result<()>,
    ) -> Result<Self> {
        if !(1..=3).contains(&ordinal) { return Err("remote_ordinal".into()); }
        if !(1..=64).contains(&lead) || !(500..=60000).contains(&timeout_ms) || !address.is_ipv4() {
            return Err("invalid_remote_config".into());
        }
        let bind = if address.ip().is_loopback() {
            "127.0.0.1:0"
        } else {
            "0.0.0.0:0"
        };
        let socket = UdpSocket::bind(bind).map_err(|e| e.to_string())?;
        socket.connect(address).map_err(|e| e.to_string())?;
        socket.set_nonblocking(true).map_err(|e| e.to_string())?;
        ready(socket.local_addr().map_err(|e| e.to_string())?)?;
        let log = OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(path)
            .map_err(|e| e.to_string())?;
        let mut nonce = [0; 16];
        #[link(name = "bcrypt")]
        unsafe extern "system" {
            fn BCryptGenRandom(
                algorithm: *mut std::ffi::c_void,
                buffer: *mut u8,
                length: u32,
                flags: u32,
            ) -> i32;
        }
        if unsafe { BCryptGenRandom(std::ptr::null_mut(), nonce.as_mut_ptr(), 16, 2) } != 0 {
            return Err("remote_random_failed".into());
        }
        let endpoint = Endpoint::new(nonce, Duration::from_millis(timeout_ms), ordinal)?;
        let (tx, rx) = mpsc::sync_channel(32);
        let state = Arc::new(Mutex::new(Snapshot::default()));
        let shared = state.clone();
        let thread=thread::Builder::new().name("icsn-remote".into()).spawn(move||{
            let mut worker=Worker {socket,endpoint,log,clock:Instant::now(),state:Snapshot::default(),lead};
            let result=worker.run(rx,&shared);
            if let Err(e)=result {worker.state.error=Some(e);}
            worker.state.finished=true;
            if let Err(e)=worker.record(json!({"event":"summary","tx":worker.state.tx,"rx":worker.state.rx,
                "duplicate":worker.state.duplicates,"stale":worker.state.stale,"establishments":worker.state.establishments,
                "bye":worker.state.bye,"error":worker.state.error,"terminal_cause":worker.state.terminal_cause,
                "expected_timeout":worker.state.expected_timeout})) {worker.state.error=Some(e);}
            *shared.lock().unwrap_or_else(|p|p.into_inner())=worker.state;
        }).map_err(|e|e.to_string())?;
        Ok(Self {
            tx,
            state,
            thread: Some(thread),
        })
    }
    pub fn snapshot(&self) -> Snapshot {
        self.state.lock().unwrap_or_else(|p| p.into_inner()).clone()
    }
    pub fn script(&self, label: &str, steps: Vec<Step>) -> Result<()> {
        self.script_with_parity(label,steps,None)
    }
    // 仅E2E已知稳定普通窗口使用：单个边沿选择有该组原生consumer的目标tick，协议/生产Resolve不变。
    pub fn script_on_tick_parity(&self,label:&str,steps:Vec<Step>,parity:u64)->Result<()> {
        if parity>1{return Err("invalid_target_tick_parity".into())}
        self.script_with_parity(label,steps,Some(parity))
    }
    fn script_with_parity(&self,label:&str,steps:Vec<Step>,parity:Option<u64>)->Result<()> {
        if steps.is_empty()
            || steps.len() > 128
            || steps[0].after != 0
            || steps.windows(2).any(|s| s[0].after >= s[1].after)
        {
            return Err("script_steps_must_increase_from_zero".into());
        }
        self.tx
            .try_send(Request::Script(label.into(), steps, parity))
            .map_err(|e| e.to_string())
    }
    pub fn silence(&self) -> Result<()> {
        self.tx
            .try_send(Request::Silence)
            .map_err(|e| e.to_string())
    }
    // 下一代次源是一次性脚本，不能携带旧命令、target 或边沿。
    pub fn arm_next_generation(&self, label: &str, steps: Vec<Step>, ttl_ms: u64) -> Result<()> {
        if steps.is_empty()
            || steps.len() > 128
            || steps[0].after != 0
            || steps.windows(2).any(|s| s[0].after >= s[1].after)
            || steps.iter().any(|s| s.state.edge != 0)
            || !(100..=60000).contains(&ttl_ms)
        {
            return Err("invalid_arm_next_generation".into());
        }
        let generation = self
            .snapshot()
            .ack
            .as_ref()
            .map(|a| a.generation)
            .unwrap_or(0);
        self.tx
            .try_send(Request::ArmNextGeneration(
                label.into(),
                steps,
                generation,
                Duration::from_millis(ttl_ms),
            ))
            .map_err(|e| e.to_string())
    }
    pub fn old_generation_sample(&self, last_ack: u64, generation: u64) -> Result<()> {
        self.tx
            .try_send(Request::OldGeneration(last_ack, generation))
            .map_err(|e| e.to_string())
    }
    pub fn expect_timeout(&self) -> Result<()> {
        self.tx
            .try_send(Request::ExpectTimeout)
            .map_err(|e| e.to_string())
    }
    pub fn bye(&self) -> Result<()> {
        self.tx.try_send(Request::Bye).map_err(|e| e.to_string())
    }
}
impl Drop for Remote {
    fn drop(&mut self) {
        let _ = self.tx.send(Request::Shutdown);
        if let Some(t) = self.thread.take() {
            let _ = t.join();
        }
    }
}

// 单 owner 工作线程的完整资源；离开线程作用域即关闭 socket 与日志。
struct Worker {
    socket: UdpSocket,
    endpoint: Endpoint,
    log: File,
    clock: Instant,
    state: Snapshot,
    lead: u64,
}
impl Worker {
    fn record(&mut self, mut v: Value) -> Result<()> {
        v["elapsed_us"] = json!(self.clock.elapsed().as_micros());
        writeln!(self.log, "{v}")
            .and_then(|_| self.log.flush())
            .map_err(|e| e.to_string())
    }
    fn send(&mut self, b: &[u8], classification: &str) -> Result<bool> {
        match self.socket.send(b) {
            Ok(n) if n == b.len() => {
                self.state.tx += 1;
                self.record(json!({"event":"send","classification":classification,
                "bytes":protocol::hex(b),"packet":protocol::decode(b).ok().map(|m|m.value())}))?;
                Ok(true)
            }
            result => {
                self.record(json!({"event":"send_error","classification":classification,"bytes":protocol::hex(b),
                "error":format!("{result:?}")}))?;
                Ok(false)
            }
        }
    }
    fn end(&mut self) -> Result<()> {
        if self.endpoint.established && !self.endpoint.retired {
            let b = self.endpoint.bye(1)?;
            let mut committed = false;
            for i in 0..3 {
                if self.send(&b, if i == 0 { "bye_first" } else { "bye_retry" })? && !committed {
                    self.endpoint.sent(true)?;
                    committed = true;
                }
            }
        }
        Ok(())
    }
    // 接收预算固定 64；墙钟只管理握手、存活与中性心跳，输入目标只来自 Ack tick。
    fn run(&mut self, rx: Receiver<Request>, shared: &Arc<Mutex<Snapshot>>) -> Result<()> {
        let hello = self.endpoint.hello().to_vec();
        if !self.send(&hello, "hello_first")? {
            return Err("hello_send_failed".into());
        }
        let mut last_hello = Instant::now();
        let mut last_send = Instant::now();
        let mut last_progress = Instant::now();
        let mut previous_tick = None;
        let mut generation = 0;
        let mut sequence = 0u64;
        let mut last_sample = None;
        let mut history: VecDeque<Command> = VecDeque::new();
        let mut script: VecDeque<Step> = VecDeque::new();
        let mut script_base = 0;
        let mut script_parity=None;
        let mut desired = State::default();
        let mut silent = false;
        let mut armed: Option<(String, Vec<Step>, u64, Instant)> = None;
        let mut bootstrap = true;
        let mut received_stamps: VecDeque<(u64, u64)> = VecDeque::new();
        let mut ending: Option<Instant> = None;
        loop {
            for _ in 0..32 {
                match rx.try_recv() {
                    Ok(Request::Shutdown) => {
                        self.end()?;
                        return Ok(());
                    }
                    Ok(Request::Bye) => {
                        self.end()?;
                        self.state.terminal_cause = Some("local_bye".into());
                        ending = Some(Instant::now() + Duration::from_millis(100));
                    }
                    Ok(Request::ExpectTimeout) => {
                        self.state.expected_timeout = true;
                        self.record(json!({"event":"expect_timeout","scene":"S18"}))?;
                    }
                    Ok(Request::ArmNextGeneration(label, steps, old, ttl)) => {
                        if armed.is_some() {
                            return Err("next_generation_already_armed".into());
                        }
                        self.record(json!({"event":"arm_generation","label":label,"old_generation":old,"ttl_ms":ttl.as_millis(),"session":protocol::hex(&self.endpoint.session)}))?;
                        armed = Some((label, steps, old, Instant::now() + ttl));
                    }
                    Ok(Request::OldGeneration(last_ack, old)) => {
                        let ack = self.endpoint.ack.clone().ok_or("old_sample_before_ack")?;
                        if !received_stamps.contains(&(last_ack, old))
                            || old >= ack.generation
                            || ack.state != 3
                        {
                            return Err("old_sample_not_known_prior_generation".into());
                        }
                        sequence = sequence
                            .checked_add(1)
                            .filter(|n| *n < u64::MAX)
                            .ok_or("command_sequence_exhausted")?;
                        let command = Command {
                            sequence,
                            target: ack.tick.checked_add(self.lead).ok_or("target_overflow")?,
                            state: State {
                                movement: [32767, 0],
                                ..State::default()
                            },
                        };
                        let bytes = self
                            .endpoint
                            .input_referencing(last_ack, vec![command.clone()])?;
                        self.record(json!({"event":"old_generation_sample","last_ack":last_ack,"old_generation":old,
                            "current_generation":ack.generation,"command_sequence":sequence,"target":command.target,"bytes":protocol::hex(&bytes)}))?;
                        if self.send(&bytes, "old_generation_input")? {
                            self.endpoint.sent(false)?;
                        }
                    }
                    Ok(Request::Silence) => {
                        silent = true;
                        self.record(json!({"event":"silence"}))?;
                    }
                    Ok(Request::Script(label, steps, parity)) => {
                        let ack = self
                            .endpoint
                            .ack
                            .as_ref()
                            .ok_or("script_before_handshake")?;
                        if ack.state != 3 || ack.generation == 0 {
                            return Err("script_before_active_generation".into());
                        }
                        script_base = ack.tick;
                        script_parity=parity;
                        script = steps.into();
                        desired = State::default();
                        self.state.script_label = label.clone();
                        self.state.script_base = script_base;
                        self.state.script_generation = generation;
                        self.record(json!({"event":"script","label":label,"base":script_base,"generation":generation,"target_tick_parity":parity}))?;
                    }
                    Err(mpsc::TryRecvError::Empty) => break,
                    Err(mpsc::TryRecvError::Disconnected) => {
                        self.end()?;
                        return Ok(());
                    }
                }
            }
            for _ in 0..64 {
                let mut bytes = [0u8; 1201];
                match self.socket.recv(&mut bytes) {
                    Ok(n) => {
                        self.state.rx += 1;
                        let result = self.endpoint.receive(&bytes[..n]);
                        let classification = match &result {
                            Ok(Reception::Duplicate) => {
                                self.state.duplicates += 1;
                                "duplicate"
                            }
                            Ok(Reception::Stale) => {
                                self.state.stale += 1;
                                "stale"
                            }
                            Ok(Reception::RetiredIgnored) => "retired_ignored",
                            Ok(Reception::Delivered(_)) => "delivered",
                            Err(_) => "error",
                        };
                        self.record(json!({"event":"receive","classification":classification,"bytes":protocol::hex(&bytes[..n]),
                            "packet":protocol::decode(&bytes[..n]).ok().map(|m|m.value()),"error":result.as_ref().err()}))?;
                        if let Reception::Delivered(m) = result? {
                            if let Body::Ack(a) = &m.body {
                                received_stamps.push_back((m.sequence, a.generation));
                                while received_stamps.len() > 4096 {
                                    received_stamps.pop_front();
                                }
                            }
                            if let Body::Bye { reason, .. } = m.body {
                                self.state.bye = Some(reason);
                                self.state.terminal_cause = Some("peer_bye".into());
                                ending.get_or_insert(Instant::now() + Duration::from_millis(100));
                            }
                        }
                    }
                    Err(e) if e.kind() == std::io::ErrorKind::WouldBlock => break,
                    Err(e) => return Err(format!("remote_receive: {e}")),
                }
            }
            self.state.ack = self.endpoint.ack.clone();
            self.state.establishments = self.endpoint.establishments;
            self.state.last_ack = self.endpoint.last_ack;
            *shared.lock().unwrap_or_else(|p| p.into_inner()) = self.state.clone();
            if let Some(deadline) = ending {
                if Instant::now() >= deadline {
                    return Ok(());
                }
                thread::sleep(Duration::from_millis(2));
                continue;
            }
            if self.endpoint.timed_out() {
                self.state.terminal_cause = Some(
                    if self.endpoint.established {
                        "peer_timeout"
                    } else {
                        "handshake_timeout"
                    }
                    .into(),
                );
                self.record(json!({"event":"timeout","established":self.endpoint.established,"expected":self.state.expected_timeout,
                    "timeout_ms":self.endpoint.timeout_ms(),"last_receive_elapsed_ms":self.endpoint.receive_age_ms()}))?;
                return Err(if self.endpoint.established {
                    "peer_timeout"
                } else {
                    "handshake_timeout"
                }
                .into());
            }
            if !self.endpoint.established {
                if last_hello.elapsed() >= Duration::from_millis(250) {
                    self.send(&hello, "hello_retry")?;
                    last_hello = Instant::now();
                }
            } else if let Some(ack) = self.endpoint.ack.clone() {
                if ack.generation > generation {
                    generation = ack.generation;
                    history.clear();
                    script.clear();
                    script_parity=None;
                    desired = State::default();
                    last_sample = None;
                    // 新代次 Ack 即使 completed 未变也代表新的可用采样边界；
                    // 不能沿用加载前的进度计时，把刚消费的预置源当作停帧旧状态清掉。
                    previous_tick = None;
                    last_progress = Instant::now();
                    self.state.script_label.clear();
                    self.state.script_base = 0;
                    self.state.script_generation = 0;
                    bootstrap = true;
                    self.record(json!({"event":"generation","generation":generation,"tick":ack.tick,"sequence_preserved":sequence}))?;
                }
                if armed.as_ref().is_some_and(|a| Instant::now() >= a.3) {
                    let (label, _, old, _) = armed.take().unwrap();
                    self.record(json!({"event":"arm_expired","label":label,"old_generation":old}))?;
                    return Err("arm_generation_timeout".into());
                }
                if ack.state == 3 && armed.as_ref().is_some_and(|a| generation > a.2) {
                    let (label, steps, old, _) = armed.take().unwrap();
                    script_base = ack.tick;
                    script_parity=None;
                    script = steps.into();
                    desired = State::default();
                    last_sample = None;
                    self.state.script_label = label.clone();
                    self.state.script_base = script_base;
                    self.state.script_generation = generation;
                    self.record(json!({"event":"arm_consumed","label":label,"old_generation":old,"generation":generation,
                        "base":script_base,"ack_sequence":self.endpoint.last_ack,"history_cleared":true}))?;
                }
                if previous_tick != Some(ack.tick) {
                    previous_tick = Some(ack.tick);
                    last_progress = Instant::now();
                }
                let fresh = last_sample != Some((generation, ack.tick));
                let heartbeat = last_send.elapsed() >= Duration::from_millis(250);
                if !silent && (fresh || heartbeat) {
                    let active = generation > 0
                        && ack.state == 3
                        && last_progress.elapsed() < Duration::from_millis(100);
                    // generation 改变后脚本已清空；阻断心跳不携带旧样本或冗余边沿。
                    if !active {
                        history.clear();
                        desired = State::default();
                        script.clear();
                        script_parity=None;
                    }
                    let mut edge = 0;
                    let candidate_target=ack.tick.checked_add(if bootstrap&&active&&fresh{1}else{self.lead}).ok_or("target_overflow")?;
                    if active && fresh {
                        while script
                            .front()
                            .is_some_and(|s| ack.tick.saturating_sub(script_base) >= s.after)
                        {
                            // 等下一次新鲜Ack的正确目标窗口；未发出的边沿没有被Resolve，不重发旧命令。
                            if script.front().is_some_and(|s|s.state.edge!=0)&&script_parity.is_some_and(|p|candidate_target%2!=p){break;}
                            desired = script.pop_front().unwrap().state;
                            edge |= desired.edge;
                            desired.edge = 0;
                        }
                    }
                    let mut state = if active { desired } else { State::default() };
                    state.edge = edge;
                    let startup = bootstrap && active && fresh;
                    sequence = sequence
                        .checked_add(1)
                        .filter(|n| *n < u64::MAX)
                        .ok_or("command_sequence_exhausted")?;
                    let command = Command {
                        sequence,
                        target: ack
                            .tick
                            .checked_add(if startup { 1 } else { self.lead })
                            .ok_or("target_overflow")?,
                        state,
                    };
                    history.push_back(command.clone());
                    let mut sampled = vec![command];
                    if startup && self.lead != 1 {
                        sequence = sequence
                            .checked_add(1)
                            .filter(|n| *n < u64::MAX)
                            .ok_or("command_sequence_exhausted")?;
                        state.edge = 0;
                        let continuation = Command {
                            sequence,
                            target: ack.tick.checked_add(self.lead).ok_or("target_overflow")?,
                            state,
                        };
                        history.push_back(continuation.clone());
                        sampled.push(continuation);
                    }
                    while history.len() > 4 {
                        history.pop_front();
                    }
                    let packet = self.endpoint.input(history.iter().cloned().collect())?;
                    self.record(json!({"event":"sample","sample_id":sequence,"generation":generation,"ack_sequence":self.endpoint.last_ack,
                        "ack_tick":ack.tick,"startup":startup,"active":active,"label":self.state.script_label,
                        "commands":sampled.iter().map(|c|json!({"sequence":c.sequence,"target":c.target,"state":c.state.value()})).collect::<Vec<_>>()}))?;
                    if self.send(&packet, "input")? {
                        self.endpoint.sent(false)?;
                        last_send = Instant::now();
                        last_sample = Some((generation, ack.tick));
                        if startup {
                            bootstrap = false;
                        }
                    }
                }
            }
            thread::sleep(Duration::from_millis(2));
        }
    }
}

// 独立脚本入口供人工网络联调复现；不启动游戏或触碰控制口。
pub fn run_cli(args: &[String]) -> Result<()> {
    let option = |key: &str| {
        args.iter()
            .position(|s| s == key)
            .and_then(|i| args.get(i + 1))
    };
    let address = option("--host")
        .ok_or("--host ip:port required")?
        .parse()
        .map_err(|e| format!("{e}"))?;
    let script_path = option("--script").ok_or("--script json required")?;
    let output = std::path::PathBuf::from(option("--out").ok_or("--out new-directory required")?);
    if output.exists() {
        return Err("remote artifact directory already exists".into());
    }
    let script: Value =
        serde_json::from_str(&crate::read(Path::new(script_path))?).map_err(|e| e.to_string())?;
    let vector = |v: &Value| -> Result<[i16; 2]> {
        let a = v
            .as_array()
            .filter(|a| a.len() == 2)
            .ok_or("script_axis_pair")?;
        let axis = |v: &Value| {
            v.as_i64()
                .and_then(|v| i16::try_from(v).ok())
                .ok_or_else(|| "script_axis_range".to_string())
        };
        Ok([axis(&a[0])?, axis(&a[1])?])
    };
    let steps = script["steps"]
        .as_array()
        .ok_or("script_steps")?
        .iter()
        .map(|s| {
            let aim = vector(&s["aim"])?;
            let movement = vector(&s["move"])?;
            let edge = u32::try_from(s["edge"].as_u64().ok_or("script_edge")?)
                .map_err(|_| "script_edge_range")?;
            Ok(Step {
                after: s["after"].as_u64().ok_or("script_after")?,
                state: State {
                    held: if aim == [0, 0] { 0 } else { 1 },
                    edge,
                    movement,
                    aim,
                },
            })
        })
        .collect::<Result<Vec<_>>>()?;
    let duration = script["duration_ticks"]
        .as_u64()
        .filter(|n| *n > 0 && *n <= 36000)
        .ok_or("duration_ticks_range")?;
    let finish = script["finish"].as_str().ok_or("script_finish")?;
    if !matches!(finish, "bye" | "silence") {
        return Err("finish must be bye or silence".into());
    }
    std::fs::create_dir_all(&output).map_err(|e| e.to_string())?;
    crate::write_json(&output.join("script.json"), &script)?;
    let ordinal = option("--ordinal").map(|s| s.parse::<u32>()).transpose().map_err(|e| e.to_string())?.unwrap_or(1);
    let remote = Remote::start_participant(address, &output.join("packets.jsonl"), 3000, 2, ordinal, |_| Ok(()))?;
    let result = (|| -> Result<()> {
        let deadline = Instant::now() + Duration::from_secs(600);
        let mut started = false;
        let mut ending = false;
        loop {
            let s = remote.snapshot();
            if let Some(e) = s.error {
                return Err(e);
            }
            if s.finished {
                return if ending {
                    Ok(())
                } else {
                    Err("peer_ended_before_script".into())
                };
            }
            if !started
                && s.ack
                    .as_ref()
                    .is_some_and(|a| a.state == 3 && a.generation > 0)
            {
                remote.script("cli", steps.clone())?;
                started = true;
            }
            if started
                && s.script_label == "cli"
                && s.ack
                    .as_ref()
                    .is_some_and(|a| a.generation != s.script_generation)
            {
                return Err("script_generation_invalidated".into());
            }
            if started
                && !ending
                && s.script_label == "cli"
                && s.ack
                    .as_ref()
                    .is_some_and(|a| a.tick >= s.script_base + duration)
            {
                if finish == "bye" {
                    remote.bye()?;
                } else {
                    remote.silence()?;
                }
                ending = true;
            }
            if Instant::now() >= deadline {
                return Err("script_wall_clock_timeout".into());
            }
            thread::sleep(Duration::from_millis(10));
        }
    })();
    let snapshot = remote.snapshot();
    drop(remote);
    crate::write_json(
        &output.join("report.json"),
        &json!({"pass":result.is_ok(),"failure":result.as_ref().err(),
        "tx":snapshot.tx,"rx":snapshot.rx,"duplicate":snapshot.duplicates,"stale":snapshot.stale,
        "bye":snapshot.bye,"automatic_retry":false}),
    )?;
    result
}
