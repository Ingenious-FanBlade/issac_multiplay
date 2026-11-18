use super::{Result, TestInstance, control::Control, win32};
use serde_json::{Value, json};
use std::{
    collections::BTreeMap,
    fs,
    net::{TcpListener, UdpSocket},
    path::{Component, Path, PathBuf},
    thread,
    time::{Duration, Instant},
};

const HOST_PORT: u16 = 27090;
const HOST_CONTROL: u16 = 27091;
const CLIENT_CONTROL: u16 = 27092;
const PROJECT_MODS: [&str; 3] = ["isaac-cs-ui", "isaac-cs-observation", "isaac-cs-test"];

// 网络验收可通过仅在本机监听的代理连接 Host，正常 P2 仍直连游戏端口。
pub(super) fn client_port() -> Result<u16> {
    match std::env::var("ISAAC_CS_P2_CLIENT_PORT") {
        Ok(value) => value.parse::<u16>().ok().filter(|port| *port != 0)
            .ok_or_else(|| "invalid P2 client loopback port".to_owned()),
        Err(std::env::VarError::NotPresent) => Ok(HOST_PORT),
        Err(error) => Err(error.to_string()),
    }
}

// 部署只写两份已确认的测试副本；拒绝重解析点，避免进入真实安装目录。
fn local_path(root: &Path, relative: &Path) -> Result<PathBuf> {
    let mut path = root.to_owned();
    for part in relative.components() {
        let Component::Normal(name) = part else {
            return Err(format!("invalid deployment path: {}", relative.display()));
        };
        path.push(name);
        match fs::symlink_metadata(&path) {
            Ok(metadata) => {
                use std::os::windows::fs::MetadataExt;
                if metadata.file_attributes() & 0x400 != 0 {
                    return Err(format!("redirected deployment path: {}", path.display()));
                }
            }
            Err(error) if error.kind() == std::io::ErrorKind::NotFound => {}
            Err(error) => return Err(format!("{}: {error}", path.display())),
        }
    }
    Ok(path)
}

pub(super) fn deploy(instances: &[&TestInstance; 2], package: &Path) -> Result<()> {
    let fault_token = std::env::var("ISAAC_CS_P2_E2E_TOKEN").ok();
    if fault_token.as_ref().is_some_and(|token| {
        token.is_empty() || token.len() > 96 ||
            !token.bytes().all(|byte| byte.is_ascii_alphanumeric() || b"_.-".contains(&byte))
    }) {
        return Err("invalid P2 E2E token".into());
    }
    let host_fixture = match std::env::var("ISAAC_CS_P2_HOST_FIXTURE").as_deref() {
        Ok("step15") => Some(fault_token.as_ref().ok_or("P2 Host fixture requires an E2E token")?),
        Err(std::env::VarError::NotPresent) => None,
        _ => return Err("invalid P2 Host fixture".into()),
    };
    // 在分配控制口前确认本地端口空闲，避免命令连到其他会话。
    let _host =
        TcpListener::bind(("127.0.0.1", HOST_CONTROL)).map_err(|e| format!("Host control: {e}"))?;
    let _client = TcpListener::bind(("127.0.0.1", CLIENT_CONTROL))
        .map_err(|e| format!("Client control: {e}"))?;
    let _udp = UdpSocket::bind(("127.0.0.1", HOST_PORT)).map_err(|e| format!("Host UDP: {e}"))?;

    let package = win32::ordinary_path(package)?;
    let read = |path: &Path| fs::read(path).map_err(|e| format!("{}: {e}", path.display()));
    let manifest: Value = serde_json::from_slice(&read(&package.join("deployment.json"))?)
        .map_err(|e| e.to_string())?;
    if manifest["format"] != "isaac.cs.deployment"
        || manifest["version"] != 1
        || manifest["build"]["flavor"] != "e2e"
        || manifest["build"]["state_contract"] != "binary-state-v1"
    {
        return Err("a binary-state E2E deployment package is required".into());
    }
    let files = manifest["files"]
        .as_array()
        .ok_or("missing deployment files")?;
    let mut sources = Vec::new();
    for file in files {
        let relative = Path::new(file["relative"].as_str().ok_or("missing package path")?);
        if relative.starts_with("e2e-tools") {
            continue;
        }
        let source = local_path(&package, relative)?;
        if !source.is_file() {
            return Err(format!("missing package file: {}", source.display()));
        }
        sources.push((relative, source));
    }
    for required in [
        "Repentogon/zhlIsaacCS.dll",
        "Repentogon/isaac-cs/config.json",
    ] {
        let source = package.join(required);
        if !sources.iter().any(|(_, path)| path == &source) {
            return Err(format!("missing deployment entry: {required}"));
        }
    }

    for (instance, port) in instances.iter().zip([HOST_CONTROL, CLIENT_CONTROL]) {
        for (relative, source) in &sources {
            let destination = local_path(&instance.root, relative)?;
            fs::create_dir_all(destination.parent().ok_or("missing deployment directory")?)
                .map_err(|e| e.to_string())?;
            fs::copy(source, &destination)
                .map_err(|e| format!("{}: {e}", destination.display()))?;
        }
        // 隔离副本中保留第三方文件，通过原生禁用标记避免玩法或其他联机模组介入。
        let sync = local_path(&instance.root, Path::new("Repentogon/zhlIsaacSync.dll"))?;
        if sync.is_file() {
            let disabled = sync.with_extension("dll.p2-disabled");
            if disabled.exists() {
                return Err(format!("both Isaac Sync copies exist: {}", sync.display()));
            }
            fs::rename(&sync, &disabled).map_err(|e| format!("{}: {e}", sync.display()))?;
        }
        let mods = local_path(&instance.root, Path::new("mods"))?;
        for entry in fs::read_dir(&mods).map_err(|e| e.to_string())? {
            let entry = entry.map_err(|e| e.to_string())?;
            let relative = Path::new("mods").join(entry.file_name()).join("disable.it");
            let marker = local_path(&instance.root, &relative)?;
            if !entry.file_type().map_err(|e| e.to_string())?.is_dir() {
                continue;
            }
            if PROJECT_MODS
                .iter()
                .any(|name| entry.file_name() == std::ffi::OsStr::new(name))
            {
                if marker.is_file() {
                    fs::remove_file(&marker).map_err(|e| e.to_string())?;
                }
            } else if !marker.exists() {
                fs::write(&marker, "Disabled for binary state P2\n").map_err(|e| e.to_string())?;
            }
        }
        let config = local_path(&instance.root, Path::new("Repentogon/isaac-cs/config.json"))?;
        // 使用二进制运行时的默认流量配置，旧示例 stream 额度不覆盖当前默认值。
        let mut settings = json!({
            "schema": 1, "enabled": true,
            "control_port": {"enabled": true, "port": port},
            "host": {"default_port": HOST_PORT, "bind_address": "127.0.0.1", "peer_timeout_ms": 30000}
        });
        // 仅为显式启用的 E2E 故障入口配置令牌，常规双端启动沿用默认关闭状态。
        if let Some(token) = &fault_token {
            settings["world_record"] = json!({"enabled": false, "e2e_token": token});
        }
        fs::write(
            &config,
            serde_json::to_vec_pretty(&settings).map_err(|e| e.to_string())?,
        )
        .map_err(|e| format!("{}: {e}", config.display()))?;
        // 复用现有 Host 实机负载，F8 才开始；Client 始终拒绝注册玩法夹具。
        if let Some(token) = host_fixture {
            let path = local_path(&instance.root, Path::new("mods/isaac-cs-test/harness_config.lua"))?;
            // 标准压力与超载各自从干净房间开始，避免依赖前序 F8 操作留下的实体。
            let load = std::env::var("ISAAC_CS_P2_LOAD").unwrap_or_default();
            let (role, actions) = if port == HOST_CONTROL {
                ("Host", match load.as_str() {
                    "ordinary" => r#"["ordinary","stop-supply"]"#,
                    "dual" => r#"["dual","stop-supply"]"#,
                    "ordinary-steady" => r#"["ordinary-steady","stop-supply"]"#,
                    "dual-steady" => r#"["dual-steady","stop-supply"]"#,
                    // 同一压力会话追加真实装备与身体增减，核跨流结构仍走联合事务。
                    "dual-steady-changes" => r#"["dual-steady","weapon","ordinary-steady","dual-steady","stop-supply","zero-empty","last-static"]"#,
                    "tears-100" => r#"["tears-100","stop-supply","zero-empty","last-static"]"#,
                    "tears-300" => r#"["tears-300","mutate-static-tear","stop-supply","zero-empty"]"#,
                    "enemies-100" => r#"["enemies-100","stop-supply","zero-empty","last-static"]"#,
                    "" | "overload" => r#"["overload","stop-supply","zero-empty","last-static","dual","mixed","stop-supply","zero-empty","ordinary","stop-supply"]"#,
                    _ => return Err(format!("unknown step15 load: {load}")),
                })
            } else {
                ("Client", "[]")
            };
            fs::write(path, format!("return {{tx='tx-04-21', node_role='{role}', network_regression=true, participants=2, token='{token}', actions_json=[=[{actions}]=]}}\n"))
                .map_err(|e| e.to_string())?;
        }
        println!("{} E2E installed, control port {port}", instance.role);
    }
    Ok(())
}

type Status = BTreeMap<String, String>;

fn field<'a>(status: &'a Status, key: &str) -> &'a str {
    status.get(key).map_or("", String::as_str)
}

// 只用当前主线程状态推进，不用固定延时猜测菜单或网络阶段。
fn wait_status(
    control: &mut Control,
    stage: &str,
    seconds: u64,
    accept: impl Fn(&Status) -> bool,
) -> Result<Status> {
    let deadline = Instant::now() + Duration::from_secs(seconds);
    loop {
        let status = control.command("cs status")?;
        if accept(&status) {
            return Ok(status);
        }
        let error = field(&status, "last_error");
        let client_error = field(&status, "client.error");
        if !error.is_empty()
            || !client_error.is_empty()
            || matches!(field(&status, "client_state"), "Failed" | "Retiring")
            || Instant::now() >= deadline
        {
            return Err(format!(
                "{stage}: state={} menu={} run={} applied={} error={} client_error={} native={}",
                field(&status, "client_state"), field(&status, "menu_id"),
                field(&status, "run_active"), field(&status, "client_world_applied"),
                error, client_error, field(&status, "client.native_reason")
            ));
        }
        thread::sleep(Duration::from_millis(150));
    }
}

fn wait_host_source(control: &mut Control) -> Result<()> {
    let deadline = Instant::now() + Duration::from_secs(30);
    loop {
        let source = control.request("cs stream source 0")?;
        if field(&source, "stream.source.ready") == "1" {
            return Ok(());
        }
        let observation = control.command("cs observe status")?;
        if field(&observation, "observation_state") == "Faulted" || Instant::now() >= deadline {
            return Err(format!("Host capture startup: {source:?}; {observation:?}"));
        }
        thread::sleep(Duration::from_millis(150));
    }
}

fn press(window: usize, key: &str) -> Result<()> {
    win32::key(window, key)?;
    // 原生菜单切换及同键释放需有时间被输入泵消费，之后仍按状态确认。
    thread::sleep(Duration::from_millis(900));
    Ok(())
}

// 沿用历史 E2E 的第二存档和原生 New Run 路径，所有按键只发给 Host。
fn start_host(control: &mut Control, window: usize) -> Result<()> {
    if field(&control.command("cs status")?, "menu_id") == "1" {
        press(window, "ENTER")?;
    }
    control.wait("menu_id", "2", 15)?;
    for _ in 0..3 {
        let status = control.command("cs status")?;
        match field(&status, "selected_save") {
            "0" => press(window, "RIGHT")?,
            "1" => break,
            "2" => press(window, "LEFT")?,
            other => return Err(format!("unexpected save selector: {other}")),
        }
    }
    control.wait("selected_save", "1", 5)?;
    press(window, "ENTER")?;
    control.wait("menu_id", "3", 15)?;
    for _ in 0..6 {
        if field(&control.command("cs status")?, "menu_game_selection") == "0" {
            break;
        }
        press(window, "UP")?;
    }
    control.wait("menu_game_selection", "0", 5)?;
    press(window, "ENTER")?;
    control.wait("menu_id", "5", 15)?;
    // 压力夹具只运行固定种子局，由原生种子输入关闭成就；不修改持久进度。
    if std::env::var("ISAAC_CS_P2_HOST_FIXTURE").as_deref() == Ok("step15") {
        press(window, "TAB")?;
        win32::text(window, "s2277mnf")?;
        press(window, "ENTER")?;
    }
    press(window, "ENTER")?;
    control.wait("run_active", "1", 30)?;
    Ok(())
}

pub(super) fn enter_world(
    host: &win32::Game,
    client: &win32::Game,
    host_window: usize,
    diagnostic: &Path,
    connect_port: u16,
) -> Result<()> {
    let mut host_control = Control::connect(
        HOST_CONTROL,
        host,
        60,
        &diagnostic.join("host-control.jsonl"),
    )?;
    let mut client_control = Control::connect(
        CLIENT_CONTROL,
        client,
        60,
        &diagnostic.join("client1-control.jsonl"),
    )?;
    for (role, control) in [
        ("Host", &mut host_control),
        ("Client1", &mut client_control),
    ] {
        wait_status(control, role, 60, |s| {
            matches!(field(s, "menu_id"), "1" | "2") && field(s, "run_active") == "0"
        })?;
    }
    // 仅调试续测使用：等两端菜单完全就绪后再附着，避开启动 TLS；默认 P2 不等待。
    // 暂停期间尚未建立会话，也没有世界安装超时。输入换行后继续同一轮开局。
    if std::env::var_os("ISAAC_CS_P2_DEBUG_PAUSE").is_some() {
        println!("Debugger pause at menus: host PID {}, client1 PID {}; press Enter to continue", host.pid(), client.pid());
        let mut line = String::new();
        if std::io::stdin().read_line(&mut line).map_err(|e| e.to_string())? == 0 {
            return Err("debugger pause input closed".into());
        }
    }
    host_control.command(&format!("cs host {HOST_PORT} 0"))?;
    wait_status(&mut host_control, "Host listening", 15, |s| {
        field(s, "role") == "Host"
    })?;
    client_control.command(&format!("cs client 127.0.0.1 {connect_port} 0"))?;
    let awaiting = wait_status(&mut client_control, "Client1 AwaitingWorld", 30, |s| {
        field(s, "client_state") == "AwaitingWorld"
    })?;
    if field(&awaiting, "client_world_applied") != "0"
        || field(&awaiting, "run_active") != "0"
        || !matches!(field(&awaiting, "menu_id"), "1" | "2")
    {
        return Err(format!("Client1 did not wait at the menu: {awaiting:?}"));
    }
    // 必须先接上采集器，再让原生开局产生 Grid 与 Player 生命周期见证。
    wait_host_source(&mut host_control)?;
    println!("Client1 AwaitingWorld; starting Host on save slot 2");
    start_host(&mut host_control, host_window)?;
    let applied = wait_status(&mut client_control, "Client1 world install", 20, |s| {
        field(s, "client_world_applied") == "1"
    }).map_err(|failure| {
        let source = host_control.request("cs stream source 0");
        let observation = host_control.request("cs observe status");
        let world = host_control.request("cs world status");
        // 首帧失败同时保留编码、主线程耗时和客机候选状态，避免只看到菜单等待。
        let timing = host_control.request("cs stream main 0");
        let client_source = client_control.request("cs stream client 0");
        let _ = host_control.request("cs stream 1 0");
        format!("{failure}; Host source: {source:?}; observation: {observation:?}; world: {world:?}; timing: {timing:?}; client: {client_source:?}")
    })?;
    println!(
        "Client1 world installed, revision {}",
        field(&applied, "client.applied_revision")
    );
    Ok(())
}
