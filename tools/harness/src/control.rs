use crate::{Result, win32};
use serde_json::{Value, json};
use std::{
    collections::BTreeMap,
    fs,
    io::{BufRead, BufReader, Write},
    net::{SocketAddr, TcpStream},
    path::Path,
    thread,
    time::{Duration, Instant},
};

// 单连接控制口只发送命令，状态通过主线程发布的快照读取。
pub(crate) struct Control {
    stream: BufReader<TcpStream>,
    transcript: fs::File,
    clock: Instant,
    sequence: u64,
    // 仅显式启用的 tx07 会维护本次游戏窗口；其它入口沿用原行为。
    pub(crate) focus_window: Option<usize>,
    pub(crate) focus_restored_at: Option<Instant>,
}
impl Control {
    pub(crate) fn connect(
        port: u16,
        game: &win32::Game,
        seconds: u64,
        transcript: &Path,
    ) -> Result<Self> {
        let end = Instant::now() + Duration::from_secs(seconds);
        while Instant::now() < end {
            if let Some(code) = game.exited() {
                return Err(format!("early process exit {code}"));
            }
            if let Ok(s) = TcpStream::connect_timeout(
                &SocketAddr::from(([127, 0, 0, 1], port)),
                Duration::from_millis(250),
            ) {
                s.set_read_timeout(Some(Duration::from_secs(6)))
                    .map_err(|e| e.to_string())?;
                s.set_write_timeout(Some(Duration::from_secs(6)))
                    .map_err(|e| e.to_string())?;
                return Ok(Self {
                    stream: BufReader::new(s),
                    transcript: fs::OpenOptions::new()
                        .write(true)
                        .create_new(true)
                        .open(transcript)
                        .map_err(|e| e.to_string())?,
                    clock: Instant::now(),
                    sequence: 0,
                    focus_window: None,
                    focus_restored_at: None,
                });
            }
            thread::sleep(Duration::from_millis(100));
        }
        Err("control port timeout".into())
    }
    // 每条请求在发送前和收到完整响应后都记录；超时现场可对应到 DLL 的请求序号。
    fn record(&mut self, event: Value) -> Result<()> {
        writeln!(self.transcript, "{event}").map_err(|e| e.to_string())?;
        self.transcript.flush().map_err(|e| e.to_string())
    }
    pub(crate) fn request(&mut self, text: &str) -> Result<BTreeMap<String, String>> {
        if let Some(window) = self.focus_window {
            let before = win32::focus_state(window);
            if before["active"] != true {
                self.record(json!({"event":"focus_lost","elapsed_ms":self.clock.elapsed().as_millis(),"state":before}))?;
                let result = win32::focus(window);
                self.record(
                    json!({"event":"focus_restore","elapsed_ms":self.clock.elapsed().as_millis(),
                    "state":win32::focus_state(window),"error":result.as_ref().err()}),
                )?;
                result?;
                self.focus_restored_at = Some(Instant::now());
            }
        }
        self.sequence += 1;
        let id = self.sequence;
        let started = Instant::now();
        self.record(json!({"event":"send","id":id,"text":text,"elapsed_ms":self.clock.elapsed().as_millis()}))?;
        let result = (|| -> Result<BTreeMap<String, String>> {
            // harness只随E2E发布，status也可能等完整来源尾部；匹配测试DLL的有界60秒，生产口不变。
            self.stream
                .get_mut()
                .set_read_timeout(Some(Duration::from_secs(65)))
                .map_err(|e| e.to_string())?;
            writeln!(self.stream.get_mut(), "{text}").map_err(|e| e.to_string())?;
            let mut response = BTreeMap::new();
            let mut count = 0;
            loop {
                let mut line = String::new();
                if self
                    .stream
                    .read_line(&mut line)
                    .map_err(|e| e.to_string())?
                    == 0
                {
                    return Err("control disconnected".into());
                }
                count += line.len();
                // 仅自有E2E快照大页允许较大回复，其它控制命令保持原有界限。
                let limit = if text.starts_with("cs stream_test ") && text.contains(" snapshot ") {
                    600000
                } else {
                    65536
                };
                if count > limit {
                    return Err("oversize response".into());
                }
                let line = line.trim_end();
                if line == "." {
                    break;
                }
                if let Some((key, value)) = line.split_once('=') {
                    response.insert(key.into(), value.into());
                }
            }
            Ok(response)
        })();
        self.record(json!({
            "event":"response","id":id,"text":text,
            "elapsed_ms":self.clock.elapsed().as_millis(),
            "request_ms":started.elapsed().as_millis(),
            "response":result.as_ref().ok(),"transport_error":result.as_ref().err()
        }))?;
        result
    }
    pub(crate) fn command(&mut self, text: &str) -> Result<BTreeMap<String, String>> {
        let response = self.request(text)?;
        if response.get("ok").map(String::as_str) != Some("1") {
            return Err(format!("{text}: {response:?}"));
        }
        Ok(response)
    }
    // 只在启动阶段容忍已完整接收的 status 超时；任何变更或传输错误均不重发。
    pub(crate) fn wait_startup(
        &mut self,
        game: &win32::Game,
        window: usize,
        seconds: u64,
    ) -> Result<BTreeMap<String, String>> {
        let deadline = Instant::now() + Duration::from_secs(seconds);
        let mut last = BTreeMap::new();
        while Instant::now() < deadline {
            if let Some(code) = game.exited() {
                return Err(format!("game exited during startup: {code}"));
            }
            // 启动加载时主线程可能超过 5 s 不处理窗口消息，系统将窗口判为未响应后无法设为前台。
            // 启动等待只尽力激活并记录；发键和截图前的 focus 仍必须成功（D20）。
            if let Err(error) = win32::focus(window) {
                self.record(json!({"event":"startup_focus_pending","error":error,
                    "elapsed_ms":self.clock.elapsed().as_millis()}))?;
            }
            // 启动加载可能尚未响应窗口激活；只读状态不要求焦点，实际发键仍由win32::focus确认。
            // 保存/恢复正常请求的焦点要求，错误路径也不能把后续场景变成无焦点操作。
            let focus_window = self.focus_window.take();
            let startup_response = self.request("cs status");
            self.focus_window = focus_window;
            last = startup_response?;
            match last.get("ok").map(String::as_str) {
                Some("1") => {
                    let menu = last.get("menu_id").map(String::as_str);
                    if matches!(menu, Some("1" | "2")) {
                        return Ok(last);
                    }
                }
                Some("0")
                    if last.get("error").map(String::as_str) == Some("main_thread_timeout") =>
                {
                    self.record(
                        json!({"event":"startup_pending","reason":"main_thread_timeout",
                        "elapsed_ms":self.clock.elapsed().as_millis()}),
                    )?;
                }
                _ => return Err(format!("startup status rejected: {last:?}")),
            }
            thread::sleep(Duration::from_millis(150));
        }
        Err(format!("startup menu timeout after {seconds}s: {last:?}"))
    }
    pub(crate) fn wait(
        &mut self,
        key: &str,
        value: &str,
        seconds: u64,
    ) -> Result<BTreeMap<String, String>> {
        let until = Instant::now() + Duration::from_secs(seconds);
        let mut last = BTreeMap::new();
        while Instant::now() < until {
            last = self.command("cs status")?;
            if last.get(key).map(String::as_str) == Some(value) {
                return Ok(last);
            }
            thread::sleep(Duration::from_millis(150));
        }
        Err(format!("status timeout {key}={value}: {last:?}"))
    }
    // 只有仍在 run、暂停主菜单及预期光标同时成立，才允许下一步物理输入。
    pub(crate) fn wait_pause(
        &mut self,
        selection: Option<&str>,
    ) -> Result<BTreeMap<String, String>> {
        let deadline = Instant::now() + Duration::from_secs(5);
        let mut last = BTreeMap::new();
        while Instant::now() < deadline {
            last = self.command("cs status")?;
            if last.get("run_active").map(String::as_str) != Some("1") {
                return Err(format!("run changed during pause navigation: {last:?}"));
            }
            if last.get("pause_state").map(String::as_str) == Some("1")
                && selection
                    .is_none_or(|s| last.get("pause_selection").map(String::as_str) == Some(s))
            {
                return Ok(last);
            }
            thread::sleep(Duration::from_millis(150));
        }
        Err(format!(
            "pause navigation timeout selection={selection:?}: {last:?}"
        ))
    }
}
