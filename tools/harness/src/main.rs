//! tx-04-04 覆盖 E1–E7 / 两次启动，tx-04-05 覆盖 V1–V10 / 单次启动。
//! 失败保留产物，不自动重试；夹具和按键由各次运行管理。
//! 构建脚本不执行本程序，存档只快照、不恢复。
mod relay;
mod remote;
mod saves;
mod tx05;
mod tx06;
mod tx07;
mod tx08;
mod tx09;
mod win32;
mod world;
mod observation;
mod tx10;
use serde_json::{Value, json};
use std::{
    collections::BTreeMap,
    env, fs,
    io::{BufRead, BufReader, Write},
    net::{SocketAddr, TcpStream},
    path::{Path, PathBuf},
    thread,
    time::{Duration, Instant},
};
type Result<T> = std::result::Result<T, String>;

fn write_json(path: &Path, value: &Value) -> Result<()> {
    fs::write(
        path,
        serde_json::to_vec_pretty(value).map_err(|e| e.to_string())?,
    )
    .map_err(|e| e.to_string())
}
fn read(path: &Path) -> Result<String> {
    fs::read_to_string(path).map_err(|e| format!("{}: {e}", path.display()))
}
fn mkdir(path: &Path) -> Result<()> {
    fs::create_dir_all(path).map_err(|e| e.to_string())
}
// 备份只复制普通文件和目录，拒绝符号链接。
fn copy_tree(source: &Path, dest: &Path) -> Result<()> {
    mkdir(dest)?;
    for item in fs::read_dir(source).map_err(|e| e.to_string())? {
        let item = item.map_err(|e| e.to_string())?;
        let kind = item.file_type().map_err(|e| e.to_string())?;
        if kind.is_symlink() {
            return Err(format!("refusing symlink {}", item.path().display()));
        }
        let dst = dest.join(item.file_name());
        if kind.is_dir() {
            copy_tree(&item.path(), &dst)?;
        } else {
            fs::copy(item.path(), dst).map_err(|e| e.to_string())?;
        }
    }
    Ok(())
}
// 读取快照字节用于直接比较，不计算哈希。
fn files(root: &Path) -> Result<BTreeMap<String, Vec<u8>>> {
    fn walk(root: &Path, p: &Path, out: &mut BTreeMap<String, Vec<u8>>) -> Result<()> {
        for entry in fs::read_dir(p).map_err(|e| e.to_string())? {
            let e = entry.map_err(|e| e.to_string())?;
            let t = e.file_type().map_err(|e| e.to_string())?;
            if t.is_symlink() {
                return Err("symlink in snapshot".into());
            }
            if t.is_dir() {
                walk(root, &e.path(), out)?;
            } else {
                out.insert(
                    e.path()
                        .strip_prefix(root)
                        .unwrap()
                        .to_string_lossy()
                        .replace('\\', "/"),
                    fs::read(e.path()).map_err(|e| e.to_string())?,
                );
            }
        }
        Ok(())
    }
    let mut result = BTreeMap::new();
    walk(root, root, &mut result)?;
    Ok(result)
}
// 版本保护只读取 PE 时间戳，不校验文件摘要。
// 读取 REPENTOGON options.ini 中某一节的键值；文件或键不存在时返回 None。
fn repentogon_option(path: &Path, section: &str, key: &str) -> Option<String> {
    let text = fs::read_to_string(path).ok()?;
    let mut current = String::new();
    for line in text.lines() {
        let line = line.trim();
        if let Some(name) = line.strip_prefix('[').and_then(|l| l.strip_suffix(']')) {
            current = name.trim().to_string();
        } else if current.eq_ignore_ascii_case(section) {
            if let Some((k, v)) = line.split_once('=') {
                if k.trim().eq_ignore_ascii_case(key) {
                    return Some(v.trim().to_string());
                }
            }
        }
    }
    None
}
fn pe_timestamp(path: &Path) -> Result<u32> {
    let bytes = fs::read(path).map_err(|e| e.to_string())?;
    let word = |n: usize| -> Result<u32> {
        let b = bytes.get(n..n + 4).ok_or("truncated PE")?;
        Ok(u32::from_le_bytes(b.try_into().unwrap()))
    };
    if bytes.get(..2) != Some(b"MZ") {
        return Err("missing MZ header".into());
    }
    let pe = word(0x3C)? as usize;
    if bytes.get(pe..pe + 4) != Some(b"PE\0\0") {
        return Err("missing PE header".into());
    }
    word(pe + 8)
}
// 单连接控制口只发送命令，状态通过主线程发布的快照读取。
struct Control {
    stream: BufReader<TcpStream>,
    transcript: fs::File,
    clock: Instant,
    sequence: u64,
    // 仅显式启用的 tx07 会维护本次游戏窗口；其它入口沿用原行为。
    focus_window: Option<usize>,
    focus_restored_at: Option<Instant>,
}
impl Control {
    fn connect(port: u16, game: &win32::Game, seconds: u64, transcript: &Path) -> Result<Self> {
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
    fn request(&mut self, text: &str) -> Result<BTreeMap<String, String>> {
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
                if count > 65536 {
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
    fn command(&mut self, text: &str) -> Result<BTreeMap<String, String>> {
        let response = self.request(text)?;
        if response.get("ok").map(String::as_str) != Some("1") {
            return Err(format!("{text}: {response:?}"));
        }
        Ok(response)
    }
    // 只在启动阶段容忍已完整接收的 status 超时；任何变更或传输错误均不重发。
    fn wait_startup(
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
    fn wait(&mut self, key: &str, value: &str, seconds: u64) -> Result<BTreeMap<String, String>> {
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
    fn wait_pause(&mut self, selection: Option<&str>) -> Result<BTreeMap<String, String>> {
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
// 一次 E2E 的来源、部署所有权、两次启动结果和清理状态。
struct Run {
    repo: PathBuf,
    game: PathBuf,
    out: PathBuf,
    save: PathBuf,
    scenario: Value,
    report: Value,
    steps: fs::File,
    deployed: bool,
    fixture_owned: bool,
    saves: Option<saves::SaveRoots>,
    deployment: Vec<Value>,
}
impl Run {
    fn step(&mut self, name: &str, detail: Value) -> Result<()> {
        let item = json!({"step":name,"detail":detail});
        writeln!(self.steps, "{item}").map_err(|e| e.to_string())?;
        self.steps.flush().map_err(|e| e.to_string())?;
        println!("{name}");
        Ok(())
    }
    fn case(&mut self, id: &str, pass: bool, detail: Value) {
        self.report["cases"][id] = json!({"pass":pass,"detail":detail});
    }
    // 每次配置写入都记入部署清单，包括第二次启动和最终关闭配置。
    fn config(&mut self, enabled: bool) -> Result<()> {
        let p = self.game.join("Repentogon/isaac-cs");
        mkdir(&p)?;
        let mut content = json!({"schema":1,"enabled":enabled,"log_level":"info","trace_dir":"isaac-cs/traces",
            "control_port":{"enabled":enabled,"port":self.scenario["control_port"]},"host":{"default_port":self.scenario["host_port"].as_u64().unwrap_or(27090),
                "bind_address":"127.0.0.1","peer_timeout_ms":self.scenario["peer_timeout_ms"].as_u64().unwrap_or(3000)}});
        if self.report["tx"] == "tx-04-08" || self.report["tx"] == "tx-04-09" || self.report["tx"] == "tx-04-10" {
            content["world_record"] = json!({"enabled":enabled,
                "output_dir":self.out.join("runtime-world"),"close_timeout_ms":self.scenario["close_timeout_ms"],
                "e2e_token":self.report["token"]});
        }
        // E2E显式启用明细；仍使用生产的拓扑复用路径，不强制逐consumer重读全图。
        let production = self.report["tx"] == "tx-04-09" && self.scenario["production"] == true;
        content["diagnostics"] = json!({"enabled":enabled && !production,"repeat_topology_reads":false});
        if production { content["world_record"]["enabled"] = json!(false); }
        write_json(&p.join("config.json"), &content)?;
        self.deployment.push(json!({"operation":"write_config","destination":p.join("config.json"),"content":content}));
        write_json(&self.out.join("deployment.json"), &json!(self.deployment))?;
        self.step(
            "write_config",
            json!({"enabled":enabled,"control_port_enabled":enabled}),
        )
    }
    fn deploy(&mut self) -> Result<()> {
        let fixture = self.game.join("mods/isaac-cs-test");
        if fixture.exists() {
            return Err("existing test mod must be preserved; deployment refused".into());
        }
        let backup_root=if self.report["tx"]=="tx-04-08" {
            self.out.join(format!("deployment-before-p{}",self.report["deploy_process"].as_u64().unwrap_or(1)))
        }else{self.out.join("deployment_before")};
        mkdir(&backup_root)?;
        let mut targets = vec![
            ("out/repentogon/zhlIsaacCS.dll", "Repentogon/zhlIsaacCS.dll"),
            ("ui/isaac-cs-ui", "mods/isaac-cs-ui"),
            ("tests/e2e-mods/isaac-cs-test", "mods/isaac-cs-test"),
        ];
        if self.report["tx"]=="tx-04-10" { targets.push(("mod/observation/isaac-cs-observation","mods/isaac-cs-observation")); }
        let existing = self.game.join("Repentogon/isaac-cs/config.json");
        if existing.exists() {
            fs::copy(&existing, backup_root.join("config.json"))
                .map_err(|e| e.to_string())?;
        }
        for (src, dst) in targets {
            // tx08 始终部署本次 artifact 已冻结的源码/二进制，人工等待期间不改用后来变化的工作区。
            let s = if self.report["tx"]=="tx-04-08" {
                if dst.ends_with("zhlIsaacCS.dll"){self.out.join("binaries/zhlIsaacCS.dll")}
                else{self.out.join("sources").join(src)}
            }else{self.repo.join(src)};
            let d = self.game.join(dst);
            if d.exists() {
                let backup = backup_root.join(d.file_name().unwrap());
                if d.is_dir() {
                    copy_tree(&d, &backup)?;
                } else {
                    fs::copy(&d, &backup).map_err(|e| e.to_string())?;
                }
            }
            if dst.ends_with("isaac-cs-test") {
                self.fixture_owned = true;
            }
            if s.is_dir() {
                copy_tree(&s, &d)?;
            } else {
                fs::copy(&s, &d).map_err(|e| e.to_string())?;
            }
            self.deployment
                .push(json!({"operation":"copy","source":s,"destination":d}));
        }
        self.deployed = true;
        if matches!(
            self.report["tx"].as_str(),
            Some("tx-04-05" | "tx-04-06" | "tx-04-07" | "tx-04-08" | "tx-04-09" | "tx-04-10")
        ) {
            let config = if self.report["tx"] == "tx-04-10" {
                format!("return {{tx=\"tx-04-10\",token=\"{}\"}}\n",self.report["token"].as_str().ok_or("token")?)
            } else if self.report["tx"] == "tx-04-09" {
                format!("return {{tx=\"tx-04-09\",token=\"{}\",seed=\"S227 7MNF\"}}\n", self.report["token"].as_str().ok_or("token")?)
            } else if self.report["tx"] == "tx-04-08" {
                self.fixture_config_tx08(self.report["deploy_process"].as_u64().unwrap_or(1))?
            } else if self.report["tx"] == "tx-04-07" {
                self.fixture_config_tx07()?
            } else {
                format!(
                    "-- 本次临时 E2E 配置，不写回源码。\nreturn {{ tx = \"{}\", token = \"{}\", host_port = {} }}\n",
                    self.report["tx"].as_str().ok_or("missing tx")?,
                    self.report["token"]
                        .as_str()
                        .ok_or("missing fixture token")?,
                    self.scenario["host_port"].as_u64().unwrap_or(27090)
                )
            };
            fs::write(
                self.game.join("mods/isaac-cs-test/harness_config.lua"),
                &config,
            )
            .map_err(|e| e.to_string())?;
            self.deployment
                .push(json!({"operation":"fixture_config","content":config}));
        }
        self.config(true)?;
        self.step("deployment", json!(self.deployment))
    }
    fn environment(&mut self) -> Result<()> {
        self.saves = Some(saves::SaveRoots::discover(self.save.clone())?);
        let roots = self.saves.as_ref().unwrap().describe();
        let names = win32::processes()?;
        let steam = names.iter().any(|n| n.eq_ignore_ascii_case("steam.exe"));
        let running = names.iter().any(|n| n.eq_ignore_ascii_case("isaac-ng.exe"));
        let timestamp = pe_timestamp(&self.game.join("Repentogon/isaac-ng.exe"))?;
        let home = PathBuf::from(env::var("USERPROFILE").map_err(|e| e.to_string())?);
        let launcher =
            read(&home.join("Documents/My Games/repentogon_launcher.ini")).unwrap_or_default();
        let version = read(&self.game.join("Repentogon/rgon_changelog.txt")).unwrap_or_default();
        // D18：只读检查 REPENTOGON 的跳过开场动画选项，由用户自行开启，harness 不写入。
        let skip_intro = repentogon_option(
            &self.save.join("Repentogon/options.ini"),
            "VanillaTweaks",
            "SkipIntro",
        );
        let mut mods = vec![];
        for entry in fs::read_dir(self.game.join("mods")).map_err(|e| e.to_string())? {
            let e = entry.map_err(|e| e.to_string())?;
            if e.path().is_dir() {
                mods.push(json!({"directory":e.file_name().to_string_lossy(),"enabled":!e.path().join("disable.it").exists()}));
            }
        }
        self.report["environment"] = json!({
            "game":self.game,
            "pe_timestamp":format!("0x{timestamp:08X}"),
            "steam_running":steam,
            "game_running":running,
            "launcher":launcher,
            "installed_version_excerpt":version.chars().take(500).collect::<String>(),
            "mods":mods,
            "architecture":"i686",
            "save_slot":2,
            "save_roots":roots,
            "repentogon_skip_intro":skip_intro
        });
        self.step("environment", self.report["environment"].clone())?;
        if timestamp != 0x68154D8D {
            return Err("wrong game version; no injection".into());
        }
        if !steam {
            return Err("Steam is not running; no injection".into());
        }
        if running {
            return Err("a game process already exists; no injection".into());
        }
        // 开场动画需要人工跳过时启动会超时；未开启则在注入前结束，不产生副作用。
        if skip_intro.as_deref() != Some("1") {
            return Err(
                "REPENTOGON SkipIntro is not enabled ([VanillaTweaks] SkipIntro=1); no injection"
                    .into(),
            );
        }
        Ok(())
    }
    fn keys(&mut self, w: usize, keys: &[String]) -> Result<()> {
        let delay = self.scenario["key_settle_ms"].as_u64().unwrap_or(1400);
        for k in keys {
            self.step("physical_key", json!(k))?;
            win32::key(w, k)?;
            thread::sleep(Duration::from_millis(delay));
        }
        Ok(())
    }
    fn sequence(&mut self, w: usize, name: &str) -> Result<()> {
        let keys = self.scenario[name]
            .as_array()
            .ok_or("missing scenario keys")?
            .iter()
            .map(|v| v.as_str().unwrap_or("").to_string())
            .collect::<Vec<_>>();
        self.keys(w, &keys)
    }
    // 原生暂停菜单为 0=Options、1=继续、2=退出；按实测快照导航，不重发失败输入。
    fn return_to_menu(&mut self, w: usize, control: &mut Control) -> Result<()> {
        let initial = control.command("cs status")?;
        self.step("return_menu_initial", json!(initial))?;
        if initial.get("run_active").map(String::as_str) != Some("1") {
            return Err(format!("return menu requires active run: {initial:?}"));
        }
        match initial.get("pause_state").map(String::as_str) {
            Some("0" | "2") => self.keys(w, &["ESC".into()])?,
            Some("1") => (),
            _ => return Err(format!("unknown pause state: {initial:?}")),
        }
        let opened = control.wait_pause(None)?;
        self.step("return_menu_pause", json!(opened))?;
        match opened.get("pause_selection").map(String::as_str) {
            Some("0") => self.keys(w, &["UP".into()])?,
            Some("1") => self.keys(w, &["DOWN".into()])?,
            Some("2") => (),
            _ => return Err(format!("unknown pause selection: {opened:?}")),
        }
        let selected = control.wait_pause(Some("2"))?;
        self.step("return_menu_exit_selected", json!(selected))?;
        win32::screenshot(w, &self.out.join("pause-exit-selected.bmp"))?;
        self.keys(w, &["ENTER".into()])?;
        let returned = control.wait("run_active", "0", 20)?;
        self.step("return_menu_run_closed", json!(returned))?;
        if !matches!(
            returned.get("menu_id").map(String::as_str),
            Some("1" | "2" | "3" | "5")
        ) {
            return Err(format!(
                "run closed without a known local menu: {returned:?}"
            ));
        }
        win32::screenshot(w, &self.out.join("returned-menu.bmp"))?;
        Ok(())
    }
    fn console(
        &mut self,
        w: usize,
        token: &str,
        control: &mut Control,
    ) -> Result<BTreeMap<String, String>> {
        win32::key(w, "CONSOLE")?;
        thread::sleep(Duration::from_millis(500));
        win32::text(w, &format!("cs echo {token}"))?;
        win32::key(w, "ENTER")?;
        thread::sleep(Duration::from_millis(500));
        win32::screenshot(w, &self.out.join(format!("{token}.bmp")))?;
        self.step(
            "console",
            json!({"token":token,"screenshot":format!("{token}.bmp")}),
        )?;
        win32::key(w, "ESC")?;
        thread::sleep(Duration::from_millis(500));
        win32::screenshot(w, &self.out.join(format!("{token}-closed.bmp")))?;
        // 关闭浮层后重新读取状态，菜单导航不能沿用打开浮层前的值。
        let status = control.command("cs status")?;
        self.step("overlay_closed", json!({"token":token,"status":status}))?;
        Ok(status)
    }
    fn collect(&self, pid: u32, label: &str) -> Result<()> {
        let dest = self.out.join(label);
        mkdir(&dest)?;
        for (s, d) in [
            (
                self.game.join("Repentogon/repentogon.log"),
                dest.join("repentogon.log"),
            ),
            (self.save.join("log.txt"), dest.join("log.txt")),
            (
                self.game
                    .join(format!("Repentogon/isaac-cs/traces/boot-{pid}.jsonl")),
                dest.join("trace.jsonl"),
            ),
        ] {
            if s.exists() {
                fs::copy(s, d).map_err(|e| e.to_string())?;
            }
        }
        let launch = read(&self.out.join(format!("{label}-launch/launch.json")))
            .ok()
            .and_then(|s| serde_json::from_str::<Value>(&s).ok());
        write_json(
            &dest.join("collection.json"),
            &json!({"pid":pid,"launch":launch,
            "note":"Logs may predate this launch; compare launch before/after metadata."}),
        )
    }
    fn trace(&self, label: &str) -> Vec<Value> {
        read(&self.out.join(label).join("trace.jsonl"))
            .unwrap_or_default()
            .lines()
            .filter_map(|l| serde_json::from_str(l).ok())
            .collect()
    }
    // 启动失败也收集日志，不能让 ? 跳过注入阶段的诊断。
    fn launch(&mut self, label: &str) -> Result<win32::Game> {
        let diagnostic = self.out.join(format!("{label}-launch"));
        let result = win32::Game::launch(&self.game.join("Repentogon"), &diagnostic);
        if result.is_err() {
            let launch: Value = read(&diagnostic.join("launch.json"))
                .ok()
                .and_then(|s| serde_json::from_str(&s).ok())
                .unwrap_or(Value::Null);
            if let Err(e) = self.collect(launch["pid"].as_u64().unwrap_or(0) as u32, label) {
                self.report["collection_error"] = json!(e);
            }
            self.step(
                "launch_failed",
                json!({"label":label,"error":result.as_ref().err(),"diagnostic":launch}),
            )?;
        }
        result
    }
    // 场景失败后先保存不抢焦点的桌面与窗口状态，再进行关闭和日志收集。
    fn capture_failure(&mut self, game: &win32::Game, label: &str, error: &str) {
        let path = self.out.join(format!("{label}-failure"));
        let collected = (|| -> Result<()> {
            mkdir(&path)?;
            let windows = win32::windows(game.pid());
            let screenshot = win32::desktop_screenshot(&path.join("desktop.bmp"));
            write_json(
                &path.join("state.json"),
                &json!({
                    "failure":error,"pid":game.pid(),"exit_code":game.exited(),"windows":windows,
                    "screenshot_error":screenshot.err()
                }),
            )
        })();
        if let Err(error) = collected {
            self.report["failure_diagnostic_error"] = json!(error);
        }
    }
    // 每份快照冻结两个来源树，同时记录路径与源文件修改时间。
    fn snapshot(&mut self, label: &str) -> Result<()> {
        self.saves
            .as_ref()
            .ok_or("save roots not discovered")?
            .snapshot(&self.out.join(label))?;
        self.step(label, json!({"slot":2,"backup":label}))
    }
    fn enabled(&mut self) -> Result<()> {
        self.step(
            "launch_enabled",
            json!({"loader":"zhlLoader.dll","Launch":0}),
        )?;
        let game = self.launch("enabled")?;
        let pid = game.pid();
        // 场景失败也先清理自有子进程，再收集本次日志。
        let result = self.enabled_scene(&game);
        if result.is_err() {
            self.capture_failure(&game, "enabled", result.as_ref().err().unwrap());
            let closed = game.close();
            self.report["failure_close"] =
                json!({"exit_code":closed.as_ref().ok(),"error":closed.as_ref().err()});
        }
        drop(game);
        thread::sleep(Duration::from_millis(300));
        self.collect(pid, "enabled")?;
        result
    }
    fn enabled_scene(&mut self, game: &win32::Game) -> Result<()> {
        let startup_seconds = self.scenario["startup_timeout_seconds"]
            .as_u64()
            .unwrap_or(90);
        let w = game.wait_window(startup_seconds)?;
        // PauseOnFocusLost=1 的环境必须在任何控制口等待之前激活游戏。
        let before_focus = win32::windows(game.pid());
        win32::focus(w)?;
        self.step(
            "startup_focus",
            json!({"before":before_focus,"after":win32::windows(game.pid())}),
        )?;
        let mut control = Control::connect(
            self.scenario["control_port"].as_u64().unwrap_or(27091) as u16,
            game,
            startup_seconds,
            &self.out.join("control.jsonl"),
        )?;
        // 自然等待开场动画和真实菜单；不盲发 ENTER，不放宽后续场景的超时判定。
        let mut initial = control.wait_startup(game, w, startup_seconds)?;
        self.step("menu_status", json!(initial))?;
        self.report["q14"] = json!({"g_game":initial.get("g_game"),"lua_menu":"observe enabled/log.txt; empty is observation, not failure"});
        initial = self.console(w, "console-menu", &mut control)?;
        if initial.get("menu_id").map(String::as_str) == Some("1") {
            self.sequence(w, "to_file_select")?;
        }
        control.wait("menu_id", "2", 15)?;
        // SelectedSave 从零开始，第二槽必须观测到 1 才能确认。
        for _ in 0..3 {
            let s = control.command("cs status")?;
            let slot = s
                .get("selected_save")
                .ok_or("missing selected_save")?
                .parse::<i32>()
                .map_err(|e| e.to_string())?;
            if slot == 1 {
                break;
            }
            self.keys(w, &[if slot < 1 { "RIGHT" } else { "LEFT" }.into()])?;
        }
        let selected = control.wait("selected_save", "1", 5)?;
        self.step("selected_second_save", json!(selected))?;
        win32::screenshot(w, &self.out.join("save-slot-2.bmp"))?;
        self.keys(w, &["ENTER".into()])?;
        control.wait("menu_id", "3", 15)?;
        // Menu_Game 选择 0 才进入角色菜单；明确观测后确认，避免进入 Online。
        for _ in 0..6 {
            let s = control.command("cs status")?;
            if s.get("menu_game_selection").map(String::as_str) == Some("0") {
                break;
            }
            self.keys(w, &["UP".into()])?;
        }
        control.wait("menu_game_selection", "0", 5)?;
        self.sequence(w, "open_character_menu")?;
        control.wait("menu_id", "5", 15)?;
        self.sequence(w, "start_run")?;
        let running = control.wait("run_active", "1", 20)?;
        self.step("run_status", json!(running))?;
        thread::sleep(Duration::from_secs(
            self.scenario["run_seconds"].as_u64().unwrap_or(10),
        ));
        let after_overlay = self.console(w, "console-run", &mut control)?;
        if after_overlay.get("run_active").map(String::as_str) != Some("1") {
            return Err(format!(
                "run changed after overlay close: {after_overlay:?}"
            ));
        }
        let echo = control.command("cs echo port-run")?;
        self.step("port_echo", json!(echo))?;
        if echo.get("echo").map(String::as_str) != Some("port-run") {
            return Err("port echo mismatch".into());
        }
        let host = control.command("cs host 27090")?;
        let hosting = control.wait("state", "Hosting", 5)?;
        let stop = control.command("cs stop")?;
        let idle = control.wait("state", "Idle", 5)?;
        let host_frame = host.get("frame").and_then(|x| x.parse::<u64>().ok());
        let active_frame = hosting.get("frame").and_then(|x| x.parse::<u64>().ok());
        self.case(
            "E4",
            matches!((host_frame,active_frame),(Some(a),Some(b)) if b>a),
            json!({"host":host,"hosting":hosting,"stop":stop,"idle":idle}),
        );
        let counters = control.command("cs counters")?;
        write_json(&self.out.join("counters.json"), &json!(counters))?;
        let manifest: Value =
            serde_json::from_str(&read(&self.repo.join("out/repentogon/hook-manifest.json"))?)
                .map_err(|e| e.to_string())?;
        let required = manifest["hooks"]
            .as_array()
            .unwrap()
            .iter()
            .filter(|h| h["required_nonzero"] == true && h["id"] != "game_exit")
            .map(|h| h["id"].as_str().unwrap())
            .chain(["keyboard_35"])
            .collect::<Vec<_>>();
        let missing = required
            .into_iter()
            .filter(|id| {
                counters
                    .get(&format!("hook.{id}"))
                    .and_then(|v| v.parse::<u64>().ok())
                    .unwrap_or(0)
                    == 0
            })
            .collect::<Vec<_>>();
        let foreign = counters
            .iter()
            .any(|(k, v)| k.ends_with(".foreign") && v != "0");
        let count = manifest["hooks"].as_array().unwrap().len().to_string();
        let status = control.command("cs status")?;
        let caps = ["loop", "input", "player", "join", "control", "device"]
            .iter()
            .all(|g| status.get(&format!("capability.{g}")).map(String::as_str) == Some("1"));
        self.case(
            "E1",
            caps,
            json!({"status":status,"pid":game.pid(),"Launch":0}),
        );
        let hook_ok = missing.is_empty()
            && !foreign
            && status.get("registered") == Some(&count)
            && status.get("pointers").map(String::as_str) == Some("8")
            && status.get("trace_dropped").map(String::as_str) == Some("0")
            && status.get("context_faults").map(String::as_str) == Some("0");
        self.case(
            "E2",
            hook_ok,
            json!({
                "required_missing":missing,"foreign":foreign,
                "expected_registered":count,"status":status
            }),
        );
        let before = counters
            .get("hook.game_exit")
            .and_then(|x| x.parse::<u64>().ok())
            .unwrap_or(0);
        self.return_to_menu(w, &mut control)?;
        let after = control
            .command("cs counters")?
            .get("hook.game_exit")
            .and_then(|x| x.parse::<u64>().ok())
            .unwrap_or(0);
        self.report["exit_hook"] = json!({"before":before,"after":after,"increased":after>before});
        drop(control);
        let code = game.close()?;
        self.report["enabled_exit"] = json!(code);
        if code != 0 {
            return Err(format!("enabled process exit code {code}"));
        }
        Ok(())
    }
    fn disabled(&mut self) -> Result<()> {
        self.config(false)?;
        self.step("launch_disabled", json!({"enabled":false}))?;
        let log_path = self.save.join("log.txt");
        let previous_log = win32::file_state(&log_path);
        let game = self.launch("disabled")?;
        let pid = game.pid();
        let result = (|| -> Result<u32> {
            let seconds = self.scenario["startup_timeout_seconds"]
                .as_u64()
                .unwrap_or(90);
            let w = game.wait_window(seconds)?;
            let deadline = Instant::now() + Duration::from_secs(seconds);
            // 关闭配置没有命令桥，由本次启动的菜单渲染夹具确认已越过开场动画。
            // 必须先看到 log.txt 被本次启动改写，不能复用启用启动的旧标记。
            loop {
                if let Some(code) = game.exited() {
                    return Err(format!("disabled game exited before menu: {code}"));
                }
                win32::focus(w)?;
                let current = win32::file_state(&log_path);
                let log = read(&log_path).unwrap_or_default();
                if current != previous_log && log.contains("[isaac-cs-test] lua-menu phase=menu ") {
                    self.step(
                        "disabled_menu",
                        json!({"source":"MC_MAIN_MENU_RENDER","log":current}),
                    )?;
                    break;
                }
                if Instant::now() >= deadline {
                    return Err("disabled startup menu timeout".into());
                }
                thread::sleep(Duration::from_millis(150));
            }
            win32::screenshot(w, &self.out.join("disabled-menu.bmp"))?;
            game.close()
        })();
        if let Err(error) = &result {
            self.capture_failure(&game, "disabled", error);
            let closed = game.close();
            self.report["failure_close"] =
                json!({"exit_code":closed.as_ref().ok(),"error":closed.as_ref().err()});
        }
        drop(game);
        thread::sleep(Duration::from_millis(300));
        self.collect(pid, "disabled")?;
        let code = result?;
        self.report["disabled_exit"] = json!(code);
        let events = self.trace("disabled");
        self.case(
            "E6",
            code == 0
                && events.len() == 1
                && events[0]["ev"] == "boot"
                && events[0]["detail"]
                    .as_str()
                    .unwrap_or("")
                    .contains("enabled=0 registered=0 pointers=0"),
            json!({"events":events,"exit":code,"menu_observation":"disabled-menu.bmp"}),
        );
        Ok(())
    }
    fn evaluate_logs(&mut self) {
        let events = self.trace("enabled");
        let log = read(&self.out.join("enabled/log.txt")).unwrap_or_default();
        let details = events
            .iter()
            .filter(|e| e["ev"] == "command")
            .filter_map(|e| e["detail"].as_str())
            .collect::<Vec<_>>();
        let bridge = [
            ("console", "console-menu"),
            ("console", "console-run"),
            ("lua", "lua-run"),
            ("port", "port-run"),
        ]
        .iter()
        .all(|(source, token)| {
            details.iter().any(|d| {
                d.contains(&format!("source={source}"))
                    && d.contains(&format!("text=cs echo {token}"))
                    && d.contains(&format!("echo={token}"))
            })
        });
        self.case(
            "E3",
            bridge && log.contains("[isaac-cs-test] lua-run") && log.contains("echo=lua-run"),
            json!({
                "bridge":bridge,
                "q14_menu_log":log.lines().filter(|s|s.contains("lua-menu")).collect::<Vec<_>>()
            }),
        );
        self.case(
            "E5",
            log.contains("[isaac-cs-ui] window-render")
                && self.out.join("console-menu.bmp").exists(),
            json!({
                "render_marker":log.contains("[isaac-cs-ui] window-render"),
                "screenshot":"console-menu.bmp"
            }),
        );
        // 状态轮询可能跳帧，仍须由 trace 判定变更恰好在下一 Manager 边界执行。
        let mut queue_timing = true;
        for verb in ["host", "stop"] {
            let queued = &self.report["cases"]["E4"]["detail"][verb];
            let id = queued["queued"].as_str().unwrap_or("");
            let frame = queued["frame"].as_str().and_then(|s| s.parse::<u64>().ok());
            queue_timing &= frame.is_some_and(|f| {
                events.iter().any(|e| {
                    e["ev"] == "state"
                        && e["detail"].as_str().is_some_and(|s| {
                            s.starts_with(&format!("id={id} "))
                                && s.contains(&format!("frame={} ", f + 1))
                        })
                })
            });
        }
        self.report["cases"]["E4"]["detail"]["exact_next_frame"] = json!(queue_timing);
        if !queue_timing {
            self.report["cases"]["E4"]["pass"] = json!(false);
        }
        let enabled = read(&self.out.join("enabled/repentogon.log")).unwrap_or_default();
        let disabled = read(&self.out.join("disabled/repentogon.log")).unwrap_or_default();
        let errors = |text: &str| {
            text.lines()
                .filter(|l| l.contains("[ERROR]") || l.contains("[FATAL]"))
                .map(str::to_string)
                .collect::<Vec<_>>()
        };
        let new_errors = errors(&enabled)
            .into_iter()
            .filter(|e| !errors(&disabled).contains(e))
            .collect::<Vec<_>>();
        self.report["repentogon_errors"] = json!({"new_in_enabled":new_errors,"enabled":errors(&enabled),"disabled":errors(&disabled)});
    }
    fn cleanup(&mut self) -> Result<()> {
        let mut errors = vec![];
        if self.fixture_owned {
            // 只删除本次创建且父目录确实为游戏 mods 的夹具目录。
            let cleanup = (|| -> Result<()> {
                let path = self.game.join("mods/isaac-cs-test");
                if !path.exists() {
                    return Ok(());
                }
                let target = path.canonicalize().map_err(|e| e.to_string())?;
                let mods = self
                    .game
                    .join("mods")
                    .canonicalize()
                    .map_err(|e| e.to_string())?;
                if target.parent() != Some(mods.as_path()) {
                    return Err("fixture cleanup escaped mods directory".into());
                }
                fs::remove_dir_all(&target).map_err(|e| e.to_string())
            })();
            if let Err(e) = cleanup {
                errors.push(e);
            }
        }
        if self.deployed {
            if let Err(e) = self.config(false) {
                errors.push(e);
            }
        }
        if errors.is_empty() {
            Ok(())
        } else {
            Err(errors.join("; "))
        }
    }
    // 保存本次实际构建源码和清单，让 artifact 可复查、可按同一入口复现。
    fn capture_sources(&self) -> Result<()> {
        for folder in ["mod", "tools/harness", "ui", "tests/e2e-mods", "scripts"] {
            copy_tree(
                &self.repo.join(folder),
                &self.out.join("sources").join(folder),
            )?;
        }
        for file in [
            "hook-manifest.json",
            "hook-manifest.md",
            "build-revision.log",
        ] {
            let source = self.repo.join("out/repentogon").join(file);
            if source.exists() {
                fs::copy(&source, self.out.join(file)).map_err(|e| e.to_string())?;
            }
        }
        Ok(())
    }
    // 记录游戏目录内的崩溃转储元数据，不读取或修改用户存档。
    fn crash_files(&self) -> Result<Value> {
        fn walk(root: &Path, path: &Path, found: &mut BTreeMap<String, Value>) -> Result<()> {
            for entry in fs::read_dir(path).map_err(|e| e.to_string())? {
                let entry = entry.map_err(|e| e.to_string())?;
                let kind = entry.file_type().map_err(|e| e.to_string())?;
                if kind.is_symlink() {
                    continue;
                }
                if kind.is_dir() {
                    walk(root, &entry.path(), found)?;
                } else if entry
                    .path()
                    .extension()
                    .is_some_and(|e| e.eq_ignore_ascii_case("dmp"))
                {
                    found.insert(
                        entry
                            .path()
                            .strip_prefix(root)
                            .unwrap()
                            .to_string_lossy()
                            .into_owned(),
                        win32::file_state(&entry.path()),
                    );
                }
            }
            Ok(())
        }
        let mut found = BTreeMap::new();
        walk(&self.game, &self.game, &mut found)?;
        // 游戏实际把转储写在存档根的 crash_dumps，不在游戏目录（tx-04-06 第一次 E2E 漏记）。
        let dumps = self.save.join("crash_dumps");
        if dumps.is_dir() {
            let mut documents = BTreeMap::new();
            walk(&dumps, &dumps, &mut documents)?;
            for (name, state) in documents {
                found.insert(format!("documents/crash_dumps/{name}"), state);
            }
        }
        Ok(json!(found))
    }
    // 所有场景在一个入口中顺序执行，不重试失败启动或失败验收。
    fn execute(&mut self) -> Result<()> {
        if self.report["tx"] == "tx-04-09" || self.report["tx"] == "tx-04-10" { return self.execute_tx09(); }
        if self.report["tx"] == "tx-04-08" { return self.execute_tx08(); }
        if self.report["tx"] == "tx-04-07" {
            return self.execute_tx07();
        }
        if self.report["tx"] == "tx-04-06" {
            return self.execute_tx06();
        }
        if self.report["tx"] == "tx-04-05" {
            return self.execute_tx05();
        }
        self.environment()?;
        self.capture_sources()?;
        self.report["crash_before"] = self.crash_files()?;
        self.snapshot("save_before")?;
        self.deploy()?;
        let enabled = self.enabled();
        // 即使首轮场景失败，也保留这次启动结束时的快照。
        let snapshot = self.snapshot("save_mid");
        if let Err(e) = &snapshot {
            self.report["save_mid_error"] = json!(e);
        }
        enabled?;
        snapshot?;
        self.disabled()?;
        self.evaluate_logs();
        Ok(())
    }
    // 清理先于报告分析，失败也关闭测试口并移除自己部署的夹具。
    fn finish(&mut self, result: Result<()>) -> Result<()> {
        if self.report["tx"] == "tx-04-09" || self.report["tx"] == "tx-04-10" { return self.finish_tx09(result); }
        if self.report["tx"] == "tx-04-08" { return self.finish_tx08(result); }
        if self.report["tx"] == "tx-04-07" {
            return self.finish_tx07(result);
        }
        if self.report["tx"] == "tx-04-06" {
            return self.finish_tx06(result);
        }
        if self.report["tx"] == "tx-04-05" {
            return self.finish_tx05(result);
        }
        self.report["failure"] = result.err().map(Value::String).unwrap_or(Value::Null);
        if let Err(e) = self.cleanup() {
            self.report["cleanup_error"] = json!(e);
        }
        match self.crash_files() {
            Ok(after) => {
                let before = &self.report["crash_before"];
                let changed = after
                    .as_object()
                    .unwrap()
                    .iter()
                    .filter(|(name, state)| before[*name] != **state)
                    .map(|(name, state)| json!({"path":name,"state":state}))
                    .collect::<Vec<_>>();
                self.report["crash_changes"] = json!(changed);
            }
            Err(e) => self.report["crash_scan_error"] = json!(e),
        }
        if self.out.join("save_before").exists() {
            if let Err(e) = self.snapshot("save_after") {
                self.report["save_backup_error"] = json!(e);
            } else {
                let comparisons = if self.out.join("save_mid/snapshot.json").exists() {
                    vec![
                        ("enabled", "save_before", "save_mid"),
                        ("disabled_or_cleanup", "save_mid", "save_after"),
                    ]
                } else {
                    vec![("failed_attempt", "save_before", "save_after")]
                };
                let mut intervals = vec![];
                for (label, before, after) in comparisons {
                    match saves::changes(&self.out.join(before), &self.out.join(after)) {
                        Ok(changes) => intervals.push(
                            json!({"launch":label,"before":before,"after":after,"changes":changes}),
                        ),
                        Err(e) => self.report["save_compare_error"] = json!(e),
                    }
                }
                // D18：先按关闭配置启动的基线放行第一、三槽的同源变化，再统一判定。
                saves::apply_slot_baseline(&mut intervals, !self.report["disabled_exit"].is_null());
                let changed = intervals
                    .iter()
                    .flat_map(|i| i["changes"].as_array().unwrap())
                    .collect::<Vec<_>>();
                let allowed = changed.iter().all(|c| c["allowed"] == true);
                let dumps = changed
                    .iter()
                    .filter(|c| {
                        c["bytes_changed"] == true
                            && c["path"]
                                .as_str()
                                .unwrap_or("")
                                .to_lowercase()
                                .contains("crash")
                    })
                    .collect::<Vec<_>>();
                let exit_ok = self.report["enabled_exit"] == 0
                    && self.report["disabled_exit"] == 0
                    && self.report["exit_hook"]["increased"] == true;
                let logs_ok = self.report["repentogon_errors"]["new_in_enabled"]
                    .as_array()
                    .is_some_and(Vec::is_empty);
                let dumps_ok = dumps.is_empty()
                    && self.report["crash_scan_error"].is_null()
                    && self.report["crash_changes"]
                        .as_array()
                        .is_some_and(Vec::is_empty);
                self.case("E7",allowed && dumps_ok && exit_ok && logs_ok && self.report["save_compare_error"].is_null(),
                    json!({"intervals":intervals,"crash_changes":dumps,"normal_exits":exit_ok,"repentogon_log_ok":logs_ok,
                        "first_third_slots":"D18 baseline: allowed only if the same file also changed in the disabled launch"}));
                write_json(&self.out.join("save-changes.json"), &json!(intervals))?;
            }
        }
        let pass = self.report["failure"].is_null()
            && self.report["cleanup_error"].is_null()
            && self.report["save_backup_error"].is_null()
            && ["E1", "E2", "E3", "E4", "E5", "E6", "E7"]
                .iter()
                .all(|id| self.report["cases"][id]["pass"] == true);
        self.report["pass"] = json!(pass);
        self.report["automatic_retry"] = json!(false);
        write_json(&self.out.join("report.json"), &self.report)?;
        let mut md = format!(
            "# tx-04-04 E2E\n\nResult: **{}**\n\n",
            if pass { "PASS" } else { "FAIL" }
        );
        for id in ["E1", "E2", "E3", "E4", "E5", "E6", "E7"] {
            let c = &self.report["cases"][id];
            md += &format!(
                "- {id}: {}\n",
                if c.is_null() {
                    "NOT_RUN"
                } else if c["pass"] == true {
                    "PASS"
                } else {
                    "FAIL"
                }
            );
        }
        md += &format!(
            "\nFailure: {}\n\nArtifacts actually generated:\n\n",
            self.report["failure"]
        );
        let mut actual = fs::read_dir(&self.out)
            .map_err(|e| e.to_string())?
            .filter_map(|e| e.ok())
            .map(|e| e.file_name().to_string_lossy().to_string())
            .collect::<Vec<_>>();
        actual.sort();
        for name in actual {
            md += &format!("- `{name}`\n");
        }
        md += "\nNo automatic retry. No save restoration.\n";
        fs::write(self.out.join("report.md"), md).map_err(|e| e.to_string())?;
        if pass {
            Ok(())
        } else {
            Err(format!(
                "E2E failed; see {}",
                self.out.join("report.json").display()
            ))
        }
    }
}
fn main() {
    if let Err(e) = entry() {
        eprintln!("{e}");
        std::process::exit(1);
    }
}
fn entry() -> Result<()> {
    let args = env::args().skip(1).collect::<Vec<_>>();
    if args.first().map(String::as_str)==Some("observation-read") { return observation::run_cli(&args); }
    if matches!(args.first().map(String::as_str), Some("world-replay" | "world-compare")) {
        return world::run_cli(&args);
    }
    if args.first().map(String::as_str) == Some("remote") {
        return remote::run_cli(&args);
    }
    if args.get(0).map(String::as_str) != Some("e2e")
        || !matches!(
            args.get(1).map(String::as_str),
            Some("tx-04-04" | "tx-04-05" | "tx-04-06" | "tx-04-07" | "tx-04-08" | "tx-04-09" | "tx-04-10")
        )
    {
        return Err("usage: harness e2e <tx-04-04|tx-04-05|tx-04-06|tx-04-07|tx-04-08> --game <game-root> --artifact-dir <new-directory> [--scenario <json>]".into());
    }
    let option = |key: &str| {
        args.iter()
            .position(|a| a == key)
            .and_then(|i| args.get(i + 1))
            .cloned()
    };
    if args[1] == "tx-04-08" {
        let mut seen=std::collections::BTreeSet::new();
        let mut at=2;
        while at<args.len() {
            let key=args[at].as_str();
            let canonical=if key=="--out" {"--artifact-dir"}else{key};
            if !["--game","--artifact-dir","--scenario"].contains(&canonical) || !seen.insert(canonical)
                || at+1>=args.len() || args[at+1].starts_with("--") {return Err("tx08 requires unique known key/value options".into());}
            at+=2;
        }
    }
    let game = win32::ordinary_path(&PathBuf::from(option("--game").ok_or("--game required")?))?;
    let out = PathBuf::from(option("--artifact-dir").or_else(|| option("--out")).ok_or("--artifact-dir required")?);
    if out.exists() {
        return Err("artifact directory already exists; no automatic rerun".into());
    }
    if args[1] == "tx-04-08" {
        if let Some(parent)=out.parent().filter(|p|!p.as_os_str().is_empty()){mkdir(parent)?;}
        fs::create_dir(&out).map_err(|e|format!("new artifact directory: {e}"))?;
    } else { mkdir(&out)?; }
    let out = out.canonicalize().map_err(|e| e.to_string())?;
    let repo = PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .parent()
        .unwrap()
        .parent()
        .unwrap()
        .to_path_buf();
    let scenario_path = option("--scenario")
        .map(PathBuf::from)
        .unwrap_or_else(|| repo.join(format!("tools/harness/scenarios/{}.json", args[1])));
    let mut scenario: Value =
        serde_json::from_str(&read(&scenario_path)?).map_err(|e| e.to_string())?;
    if args[1] == "tx-04-09" {
        let profile=option("--profile").unwrap_or_else(||"diagnostic".into());
        if !["diagnostic","production"].contains(&profile.as_str()) { return Err("unknown tx09 profile".into()); }
        scenario["production"] = json!(profile=="production");
    }
    write_json(&out.join("scenario.json"), &scenario)?;
    let save = PathBuf::from(env::var("USERPROFILE").map_err(|e| e.to_string())?)
        .join("Documents/My Games/Binding of Isaac Repentance+");
    let mut run = Run {
        repo,
        game,
        out: out.clone(),
        save,
        scenario,
        report: json!({"tx":args[1],"cases":{},"cleanup_error":null,
            "token":format!("{}-{}-{}",args[1],std::process::id(), std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).map_err(|e|e.to_string())?.as_millis())}),
        steps: fs::File::create(out.join("steps.jsonl")).map_err(|e| e.to_string())?,
        deployed: false,
        fixture_owned: false,
        saves: None,
        deployment: vec![],
    };
    // 意外 Rust panic 也转为失败报告，确保走清理路径。
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| run.execute()))
        .unwrap_or_else(|_| Err("harness panic".into()));
    let result=run.finish(result);
    let remove_temporary=result.is_ok()&&run.report["remove_temporary_output"]==true;
    drop(run); // Windows必须先关闭steps/transcript再清理临时目录。
    if remove_temporary {
        let resolved=out.canonicalize().map_err(|e|e.to_string())?;
        let intended=PathBuf::from(env!("CARGO_MANIFEST_DIR")).parent().unwrap().parent().unwrap().join("out").canonicalize().map_err(|e|e.to_string())?;
        if resolved==intended || !resolved.starts_with(&intended){return Err("temporary_cleanup_path_escape".into());}
        fs::remove_dir_all(resolved).map_err(|e|e.to_string())?;
    }
    result
}
