#[allow(dead_code)]
#[path = "../control.rs"]
mod control;
mod p2;
#[allow(dead_code)]
#[path = "../win32.rs"]
mod win32;

use std::{env, fs, io, path::Path, path::PathBuf};
type Result<T> = std::result::Result<T, String>;

const HOST: &str = r"F:\projects_isaac\Isaac-for-test\host\The Binding of Isaac Rebirth";
const CLIENT: &str = r"F:\projects_isaac\Isaac-for-test\client1\The Binding of Isaac Rebirth";
const SAVE_SUBDIRECTORY: &str = "Documents/My Games/Binding of Isaac Repentance+";

// 一个测试端点持有独立的游戏目录、可执行文件和本地存档根。
struct TestInstance {
    role: &'static str,
    root: PathBuf,
    game: PathBuf,
    executable: PathBuf,
    profile: PathBuf,
}

impl TestInstance {
    fn discover(role: &'static str, path: &Path) -> Result<Self> {
        let root = win32::ordinary_path(path)?;
        if !root
            .to_string_lossy()
            .eq_ignore_ascii_case(&path.to_string_lossy())
        {
            return Err(format!(
                "{role}: test copy resolves outside {}",
                path.display()
            ));
        }
        let game = win32::ordinary_path(&root.join("Repentogon"))?;
        if game != root.join("Repentogon") {
            return Err(format!(
                "{role}: Repentogon directory resolves outside its test copy"
            ));
        }
        for name in [
            "isaac-ng.exe",
            "steam_api.dll",
            "zhlLoader.dll",
            "zhlREPENTOGON.dll",
        ] {
            let file = game.join(name);
            if !file.is_file() || win32::ordinary_path(&file)? != file {
                return Err(format!("{role}: missing or redirected {}", file.display()));
            }
        }
        Ok(Self {
            role,
            executable: game.join("isaac-ng.exe"),
            game,
            profile: root.join("isaac-dual-profile"),
            root,
        })
    }

    fn launch(&self, bootstrap: &Path, diagnostic: &Path) -> Result<win32::Game> {
        let profile = self
            .profile
            .to_str()
            .ok_or("test profile path is not Unicode")?;
        if !profile.is_ascii() || profile.len() >= 260 {
            return Err(format!(
                "{}: profile path must be ASCII and below MAX_PATH",
                self.role
            ));
        }
        let (drive, home) = profile.split_at(2);
        win32::Game::launch_with_bootstrap(
            &self.game,
            &diagnostic.join(self.role),
            Some(bootstrap),
            &[
                ("SteamAppId", "250900"),
                ("USERPROFILE", profile),
                ("HOMEDRIVE", drive),
                ("HOMEPATH", home),
            ],
        )
        .map_err(|error| format!("{}: {error}", self.role))
    }
}

// 首次启动生成独立选项。后续保留测试存档，只重申双开所需的选项值。
fn prepare_options(path: &Path, section: &str, settings: &[(&str, &str)]) -> Result<()> {
    let text = match fs::read_to_string(path) {
        Ok(text) => text,
        Err(error) if error.kind() == io::ErrorKind::NotFound => String::new(),
        Err(error) => return Err(format!("{}: {error}", path.display())),
    };
    let section = format!("[{section}]");
    let mut result = String::new();
    let mut in_options = false;
    let mut found = false;
    for line in text.trim_start_matches('\u{feff}').lines() {
        let trimmed = line.trim();
        if trimmed.starts_with('[') && trimmed.ends_with(']') {
            in_options = trimmed.eq_ignore_ascii_case(&section);
            found |= in_options;
            result.push_str(line);
            result.push('\n');
            if in_options {
                for &(key, value) in settings {
                    result.push_str(&format!("{key}={value}\n"));
                }
            }
        } else if !in_options
            || !trimmed.split_once('=').is_some_and(|(key, _)| {
                settings
                    .iter()
                    .any(|(name, _)| key.trim().eq_ignore_ascii_case(name))
            })
        {
            result.push_str(line);
            result.push('\n');
        }
    }
    if !found {
        result.push_str(&section);
        result.push('\n');
        for &(key, value) in settings {
            result.push_str(&format!("{key}={value}\n"));
        }
    }
    fs::write(path, result).map_err(|error| format!("{}: {error}", path.display()))
}

fn prepare_profile(profile: &Path) -> Result<()> {
    // 逐级建目录，避免现有 junction 或链接把测试写入导向正式存档。
    let mut directory = profile.to_owned();
    for part in std::iter::once("")
        .chain(SAVE_SUBDIRECTORY.split('/'))
        .chain(["Repentogon"])
    {
        if !part.is_empty() {
            directory.push(part);
        }
        match fs::create_dir(&directory) {
            Ok(()) => {}
            Err(error) if error.kind() == io::ErrorKind::AlreadyExists => {}
            Err(error) => return Err(format!("{}: {error}", directory.display())),
        }
        if !directory.is_dir() || win32::ordinary_path(&directory)? != directory {
            return Err(format!(
                "redirected profile directory: {}",
                directory.display()
            ));
        }
    }
    let repentogon = directory.join("options.ini");
    let vanilla = directory
        .parent()
        .ok_or("test save directory missing")?
        .join("options.ini");
    for options in [&vanilla, &repentogon] {
        if fs::symlink_metadata(options).is_ok() && win32::ordinary_path(options)? != *options {
            return Err(format!("redirected profile options: {}", options.display()));
        }
    }
    prepare_options(
        &vanilla,
        "Options",
        &[
            ("SteamCloud", "0"),
            ("PauseOnFocusLost", "0"),
            ("Fullscreen", "0"),
        ],
    )?;
    // 隔离测试副本直接进入菜单，启动器随后从真实菜单状态开始接入会话。
    prepare_options(&repentogon, "VanillaTweaks", &[("SkipIntro", "1")])
}

// 由父进程同时保有两端句柄，任一启动失败都只清理本次启动的实例。
fn run() -> Result<()> {
    let mut args = env::args_os().skip(1);
    let bootstrap = win32::ordinary_path(&PathBuf::from(
        args.next().ok_or("missing bootstrap DLL path")?,
    ))?;
    let diagnostic = PathBuf::from(args.next().ok_or("missing diagnostic directory")?);
    let package = args.next().map(PathBuf::from);
    let launch_only = match args.next().as_deref() {
        None => false,
        Some(flag) if flag == "--launch-only" && args.next().is_none() => true,
        _ => return Err("usage: isaac-dual.exe <bootstrap.dll> <diagnostic directory> [E2E package [--launch-only]]".into()),
    };
    let host = TestInstance::discover("host", Path::new(HOST))?;
    let client = TestInstance::discover("client1", Path::new(CLIENT))?;
    if host.executable == client.executable {
        return Err("host and client1 resolve to the same executable".into());
    }
    // 手动联机沿用部署和窗口交接，跳过自动连接端口和开局编排。
    let connect_port = if package.is_some() && !launch_only {
        Some(p2::client_port()?)
    } else {
        None
    };
    win32::steam_ready()?;
    win32::ensure_copies_idle(&[&host.executable, &client.executable])?;
    prepare_profile(&host.profile)?;
    prepare_profile(&client.profile)?;
    if let Some(package) = &package {
        p2::deploy(&[&host, &client], package)?;
    }
    let host_game = host.launch(&bootstrap, &diagnostic)?;
    let mut client_game = None;
    let ready = (|| {
        let client = client_game.insert(client.launch(&bootstrap, &diagnostic)?);
        let host_window = host_game
            .wait_window(60)
            .map_err(|error| format!("host: {error}"))?;
        client
            .wait_window(60)
            .map_err(|error| format!("client1: {error}"))?;
        if host_game.exited().is_some() || client.exited().is_some() {
            return Err("a test instance exited before both windows were ready".into());
        }
        if let Some(port) = connect_port {
            p2::enter_world(&host_game, client, host_window, &diagnostic, port)?;
        }
        Ok::<(), String>(())
    })();
    if ready.is_err() {
        // 能看到窗口时先走正常退出；未完成启动的子进程由 Game 析构回收。
        for game in client_game.iter().chain(std::iter::once(&host_game)) {
            if game.window().is_ok() {
                let _ = game.close();
            }
        }
        ready?;
    }
    let client_game = client_game.ok_or("client1 process missing after launch")?;
    println!(
        "host PID {} | client1 PID {}",
        host_game.pid(),
        client_game.pid()
    );
    host_game.detach();
    client_game.detach();
    if launch_only {
        println!("Both games started. Use Isaac CS > Session panel to connect manually.");
    }
    Ok(())
}

fn main() {
    if let Err(error) = run() {
        eprintln!("isaac-dual: {error}");
        std::process::exit(1);
    }
}
