//! x86 启动器、物理输入与截图的最小 Win32 绑定。
//! 只管理本次创建的子进程，诊断包括错误对话框和启动日志。
#![allow(non_snake_case)]
use serde_json::{Value, json};
use std::{
    ffi::c_void,
    fs,
    mem::size_of,
    path::Path,
    path::PathBuf,
    ptr::null_mut,
    thread,
    time::UNIX_EPOCH,
    time::{Duration, Instant},
};
type Handle = *mut c_void;
type Bool = i32;
#[repr(C)]
#[derive(Default)]
struct StartupInfo {
    cb: u32,
    reserved: Handle,
    desktop: Handle,
    title: Handle,
    x: u32,
    y: u32,
    xsize: u32,
    ysize: u32,
    xc: u32,
    yc: u32,
    fill: u32,
    flags: u32,
    show: u16,
    pad: u16,
    reserved2: Handle,
    input: Handle,
    output: Handle,
    error: Handle,
}
#[repr(C)]
#[derive(Default)]
struct ProcessInfo {
    process: Handle,
    thread: Handle,
    pid: u32,
    tid: u32,
}
#[repr(C)]
struct ProcessEntry {
    size: u32,
    usage: u32,
    pid: u32,
    heap: usize,
    module: u32,
    threads: u32,
    parent: u32,
    priority: i32,
    flags: u32,
    exe: [u16; 260],
}
#[repr(C)]
#[derive(Default)]
struct Rect {
    left: i32,
    top: i32,
    right: i32,
    bottom: i32,
}
#[repr(C)]
#[derive(Clone, Copy)]
struct Keyboard {
    vk: u16,
    scan: u16,
    flags: u32,
    time: u32,
    extra: usize,
}
#[repr(C)]
#[derive(Clone, Copy)]
union InputData {
    key: Keyboard,
    padding: [u32; 6],
}
#[repr(C)]
struct Input {
    kind: u32,
    data: InputData,
}
#[repr(C)]
#[derive(Default)]
struct BitmapInfo {
    size: u32,
    width: i32,
    height: i32,
    planes: u16,
    bits: u16,
    compression: u32,
    image_size: u32,
    xppm: i32,
    yppm: i32,
    colors: u32,
    important: u32,
}

#[link(name = "kernel32")]
unsafe extern "system" {
    fn CreateProcessW(
        app: *const u16,
        cmd: *mut u16,
        pa: Handle,
        ta: Handle,
        inherit: Bool,
        flags: u32,
        env: Handle,
        cwd: *const u16,
        start: *mut StartupInfo,
        info: *mut ProcessInfo,
    ) -> Bool;
    fn CloseHandle(h: Handle) -> Bool;
    fn GetModuleHandleW(name: *const u16) -> Handle;
    fn GetProcAddress(module: Handle, name: *const u8) -> Handle;
    fn VirtualAllocEx(
        process: Handle,
        address: Handle,
        size: usize,
        kind: u32,
        protect: u32,
    ) -> Handle;
    fn VirtualFreeEx(process: Handle, address: Handle, size: usize, kind: u32) -> Bool;
    fn WriteProcessMemory(
        process: Handle,
        address: Handle,
        buffer: *const c_void,
        size: usize,
        written: *mut usize,
    ) -> Bool;
    fn CreateRemoteThread(
        process: Handle,
        attr: Handle,
        stack: usize,
        start: Handle,
        param: Handle,
        flags: u32,
        id: *mut u32,
    ) -> Handle;
    fn FlushInstructionCache(process: Handle, address: Handle, size: usize) -> Bool;
    fn WaitForSingleObject(handle: Handle, ms: u32) -> u32;
    fn GetExitCodeThread(handle: Handle, code: *mut u32) -> Bool;
    fn GetExitCodeProcess(handle: Handle, code: *mut u32) -> Bool;
    fn ResumeThread(handle: Handle) -> u32;
    fn TerminateProcess(handle: Handle, exit: u32) -> Bool;
    fn CreateToolhelp32Snapshot(flags: u32, pid: u32) -> Handle;
    fn Process32FirstW(snapshot: Handle, entry: *mut ProcessEntry) -> Bool;
    fn Process32NextW(snapshot: Handle, entry: *mut ProcessEntry) -> Bool;
}
#[link(name = "user32")]
unsafe extern "system" {
    fn EnumWindows(callback: unsafe extern "system" fn(Handle, isize) -> Bool, data: isize)
    -> Bool;
    fn GetWindowThreadProcessId(window: Handle, pid: *mut u32) -> u32;
    fn IsWindowVisible(window: Handle) -> Bool;
    fn IsIconic(window: Handle) -> Bool;
    fn GetClassNameW(window: Handle, text: *mut u16, count: i32) -> i32;
    fn GetWindowTextW(window: Handle, text: *mut u16, count: i32) -> i32;
    fn GetSystemMetrics(index: i32) -> i32;
    fn GetForegroundWindow() -> Handle;
    fn SetForegroundWindow(window: Handle) -> Bool;
    fn ShowWindow(window: Handle, cmd: i32) -> Bool;
    fn SendInput(count: u32, input: *const Input, size: i32) -> u32;
    fn MapVirtualKeyW(code: u32, kind: u32) -> u32;
    fn VkKeyScanW(c: u16) -> i16;
    fn PostMessageW(window: Handle, msg: u32, wparam: usize, lparam: isize) -> Bool;
    fn GetClientRect(window: Handle, rect: *mut Rect) -> Bool;
    fn GetDC(window: Handle) -> Handle;
    fn ReleaseDC(window: Handle, dc: Handle) -> i32;
}
#[link(name = "gdi32")]
unsafe extern "system" {
    fn CreateCompatibleDC(dc: Handle) -> Handle;
    fn CreateCompatibleBitmap(dc: Handle, w: i32, h: i32) -> Handle;
    fn SelectObject(dc: Handle, obj: Handle) -> Handle;
    fn BitBlt(
        dst: Handle,
        x: i32,
        y: i32,
        w: i32,
        h: i32,
        src: Handle,
        sx: i32,
        sy: i32,
        rop: u32,
    ) -> Bool;
    fn GetDIBits(
        dc: Handle,
        bmp: Handle,
        start: u32,
        lines: u32,
        bits: Handle,
        info: *mut BitmapInfo,
        usage: u32,
    ) -> i32;
    fn DeleteObject(obj: Handle) -> Bool;
    fn DeleteDC(dc: Handle) -> Bool;
}
#[link(name = "advapi32")]
unsafe extern "system" {
    fn RegGetValueW(
        key: Handle,
        subkey: *const u16,
        value: *const u16,
        flags: u32,
        kind: *mut u32,
        data: Handle,
        size: *mut u32,
    ) -> i32;
}

// canonicalize 用于解析路径后立即去掉扩展前缀，避免把它传给旧加载器。
pub fn ordinary_path(path: &Path) -> Result<PathBuf, String> {
    let absolute = path
        .canonicalize()
        .map_err(|e| format!("{}: {e}", path.display()))?;
    let text = absolute.to_str().ok_or("path is not Unicode")?;
    if let Some(tail) = text.strip_prefix(r"\\?\UNC\") {
        Ok(PathBuf::from(format!(r"\\{tail}")))
    } else {
        Ok(PathBuf::from(text.strip_prefix(r"\\?\").unwrap_or(text)))
    }
}

// 从当前用户的 Steam 配置发现云存储根目录，不写死安装盘或账户。
pub fn steam_path() -> Result<PathBuf, String> {
    unsafe {
        let mut data = [0u16; 32768];
        let mut bytes = (data.len() * 2) as u32;
        let status = RegGetValueW(
            0x80000001usize as Handle,
            wide(r"Software\Valve\Steam").as_ptr(),
            wide("SteamPath").as_ptr(),
            6,
            null_mut(),
            data.as_mut_ptr() as _,
            &mut bytes,
        );
        if status != 0 {
            return Err(format!("SteamPath registry read failed: {status}"));
        }
        let n = data
            .iter()
            .position(|&c| c == 0)
            .ok_or("unterminated SteamPath")?;
        ordinary_path(Path::new(
            &String::from_utf16(&data[..n]).map_err(|e| e.to_string())?,
        ))
    }
}

// 日志即使没有更新也要保留其元数据，不能把旧文件当成本轮证据。
pub fn file_state(path: &Path) -> Value {
    match fs::metadata(path) {
        Ok(m) => json!({"exists":true,"size":m.len(),"modified_ns":m.modified().ok()
            .and_then(|t|t.duration_since(UNIX_EPOCH).ok()).map(|d|d.as_nanos().to_string())}),
        Err(e) => json!({"exists":false,"error":e.to_string()}),
    }
}

// 枚举目标进程的全部顶层窗口，包含不可见窗口和 #32770 错误框。
pub fn windows(pid: u32) -> Vec<Value> {
    unsafe {
        struct Search {
            pid: u32,
            windows: Vec<Value>,
        }
        unsafe extern "system" fn collect(h: Handle, data: isize) -> Bool {
            unsafe {
                let search = &mut *(data as *mut Search);
                let mut owner = 0;
                GetWindowThreadProcessId(h, &mut owner);
                if owner == search.pid {
                    let mut class = [0u16; 256];
                    let mut title = [0u16; 2048];
                    let cn =
                        GetClassNameW(h, class.as_mut_ptr(), class.len() as i32).max(0) as usize;
                    let tn =
                        GetWindowTextW(h, title.as_mut_ptr(), title.len() as i32).max(0) as usize;
                    search.windows.push(json!({
                        "handle":h as usize,
                        "class":String::from_utf16_lossy(&class[..cn]),
                        "title":String::from_utf16_lossy(&title[..tn]),
                        "visible":IsWindowVisible(h)!=0,
                        "minimized":IsIconic(h)!=0,
                        "foreground":GetForegroundWindow()==h
                    }));
                }
                1
            }
        }
        let mut search = Search {
            pid,
            windows: vec![],
        };
        EnumWindows(collect, &mut search as *mut _ as isize);
        search.windows
    }
}

fn wide(s: &str) -> Vec<u16> {
    s.encode_utf16().chain(Some(0)).collect()
}
fn error(action: &str) -> String {
    format!("{action}: {}", std::io::Error::last_os_error())
}
pub fn processes() -> Result<Vec<String>, String> {
    unsafe {
        let h = CreateToolhelp32Snapshot(2, 0);
        if h as isize == -1 {
            return Err(error("process snapshot"));
        }
        let mut e: ProcessEntry = std::mem::zeroed();
        e.size = size_of::<ProcessEntry>() as u32;
        let mut names = vec![];
        let mut more = Process32FirstW(h, &mut e);
        while more != 0 {
            let n = e.exe.iter().position(|&v| v == 0).unwrap_or(260);
            names.push(String::from_utf16_lossy(&e.exe[..n]));
            more = Process32NextW(h, &mut e);
        }
        CloseHandle(h);
        Ok(names)
    }
}
// REPENTOGON 的 Lua5.3.3r.dll 在 DllMain 中检查此变量（libs/lua/lua.c:28-41），
// 缺失或为空时弹出 “Use The Launcher!” 并退出；官方启动器在创建进程前设置它。
const LAUNCHER_MARKER: &str = "LAUNCHED_BY_RGONLAUNCHER";
const LAUNCHER_MARKER_VALUE: &str = "1";

// 构造子进程的 Unicode 环境块：复制 harness 当前环境，覆盖启动器标记。
// 按名称不区分大小写排序，每项以 NUL 结尾，整体再以一个 NUL 结束。
fn launcher_environment() -> Vec<u16> {
    let mut variables: Vec<(String, String)> = std::env::vars_os()
        .map(|(k, v)| {
            (
                k.to_string_lossy().into_owned(),
                v.to_string_lossy().into_owned(),
            )
        })
        .filter(|(k, _)| !k.eq_ignore_ascii_case(LAUNCHER_MARKER))
        .collect();
    variables.push((LAUNCHER_MARKER.into(), LAUNCHER_MARKER_VALUE.into()));
    variables.sort_by_key(|(k, _)| k.to_uppercase());
    let mut block = Vec::new();
    for (key, value) in variables {
        block.extend(format!("{key}={value}").encode_utf16());
        block.push(0);
    }
    block.push(0);
    block
}

pub struct Game {
    info: ProcessInfo,
}
impl Game {
    pub fn pid(&self) -> u32 {
        self.info.pid
    }
    // 只启动挂起的自有子进程，失败诊断完成后才允许 Drop 终止它。
    pub fn launch(directory: &Path, diagnostic: &Path) -> Result<Self, String> {
        unsafe {
            fs::create_dir_all(diagnostic).map_err(|e| e.to_string())?;
            let directory = ordinary_path(directory)?;
            let exe = directory.join("isaac-ng.exe");
            let dll = directory.join("zhlLoader.dll");
            for path in [&directory, &exe, &dll] {
                let text = path.to_str().ok_or("launcher path is not Unicode")?;
                if !text.is_ascii() || text.len() >= 260 || text.contains('"') {
                    return Err(format!(
                        "launcher requires ASCII paths below MAX_PATH: {}",
                        path.display()
                    ));
                }
            }
            let log = directory.join("repentogon.log");
            let mut report = json!({"directory":directory,"exe":exe,"dll":dll,
            "command_line":format!("\"{}\"",exe.display()),
            "environment_added":{LAUNCHER_MARKER:LAUNCHER_MARKER_VALUE},"before":file_state(&log)});
            if log.exists() {
                report["before_copy"] = json!(
                    fs::copy(&log, diagnostic.join("repentogon-before.log"))
                        .map(|_| "copied".to_string())
                        .unwrap_or_else(|e| e.to_string())
                );
            }
            let mut created: Option<Self> = None;
            let result = (|| -> Result<(), String> {
                if size_of::<usize>() != 4 {
                    return Err("harness must be i686".into());
                }
                let mut start = StartupInfo::default();
                start.cb = size_of::<StartupInfo>() as u32;
                let mut info = ProcessInfo::default();
                let mut command = wide(&format!("\"{}\"", exe.display()));
                // D17：显式环境块带上启动器标记，harness 自身环境保持不变。
                let mut environment = launcher_environment();
                // CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT
                if CreateProcessW(
                    wide(&exe.to_string_lossy()).as_ptr(),
                    command.as_mut_ptr(),
                    null_mut(),
                    null_mut(),
                    0,
                    0x4 | 0x400,
                    environment.as_mut_ptr() as Handle,
                    wide(&directory.to_string_lossy()).as_ptr(),
                    &mut start,
                    &mut info,
                ) == 0
                {
                    return Err(error("CreateProcessW"));
                }
                created = Some(Self { info });
                let game = created.as_mut().unwrap();
                report["pid"] = json!(game.pid());
                game.inject(&directory)?;
                if ResumeThread(game.info.thread) == u32::MAX {
                    return Err(error("ResumeThread"));
                }
                Ok(())
            })();
            report["after"] = file_state(&log);
            report["log_changed"] = json!(report["before"] != report["after"]);
            report["failure"] = json!(result.as_ref().err());
            if let Some(game) = created.as_ref() {
                report["exit_code"] = json!(game.exited());
                report["windows"] = json!(windows(game.pid()));
            }
            if result.is_err() {
                if log.exists() {
                    report["after_copy"] = json!(
                        fs::copy(&log, diagnostic.join("repentogon-after.log"))
                            .map(|_| "copied".to_string())
                            .unwrap_or_else(|e| e.to_string())
                    );
                }
                report["desktop_screenshot"] = json!(
                    desktop_screenshot(&diagnostic.join("desktop.bmp"))
                        .map(|_| "desktop.bmp".to_string())
                        .unwrap_or_else(|e| e.to_string())
                );
            }
            fs::write(
                diagnostic.join("launch.json"),
                serde_json::to_vec_pretty(&report).map_err(|e| e.to_string())?,
            )
            .map_err(|e| e.to_string())?;
            result?;
            Ok(created.unwrap())
        }
    }
    unsafe fn inject(&mut self, directory: &Path) -> Result<(), String> {
        unsafe {
            let dll = directory
                .join("zhlLoader.dll")
                .to_string_lossy()
                .as_bytes()
                .to_vec();
            if !dll.is_ascii() {
                return Err("LoadLibraryA launcher path must be ASCII".into());
            }
            let remote = VirtualAllocEx(self.info.process, null_mut(), 4096, 0x3000, 0x40);
            if remote.is_null() {
                return Err(error("VirtualAllocEx"));
            }
            let base = remote as u32;
            let kernel = GetModuleHandleW(wide("kernel32.dll").as_ptr());
            let load = GetProcAddress(kernel, c"LoadLibraryA".as_ptr() as *const u8) as u32;
            let get = GetProcAddress(kernel, c"GetProcAddress".as_ptr() as *const u8) as u32;
            let mut code = vec![];
            code.push(0x68);
            code.extend((base + 256).to_le_bytes()); // 压入 DLL 路径
            code.push(0xB8);
            code.extend(load.to_le_bytes());
            code.extend([0xFF, 0xD0, 0x85, 0xC0, 0x74, 0]);
            let first = code.len() - 1;
            code.push(0x68);
            code.extend((base + 1024).to_le_bytes());
            code.push(0x50);
            code.push(0xB8);
            code.extend(get.to_le_bytes());
            code.extend([0xFF, 0xD0, 0x85, 0xC0, 0x74, 0]);
            let second = code.len() - 1;
            // Launch 使用 cdecl；线程入口使用 stdcall(LPVOID)，以 ret 4 返回。
            code.extend([0x6A, 0, 0xFF, 0xD0, 0x83, 0xC4, 4, 0xC2, 4, 0]);
            let fail = code.len();
            code.extend([0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xC2, 4, 0]);
            code[first] = (fail - first - 1) as u8;
            code[second] = (fail - second - 1) as u8;
            let mut block = vec![0u8; 2048];
            block[..code.len()].copy_from_slice(&code);
            if dll.len() >= 768 {
                VirtualFreeEx(self.info.process, remote, 0, 0x8000);
                return Err("loader path too long".into());
            }
            block[256..256 + dll.len()].copy_from_slice(&dll);
            block[1024..1031].copy_from_slice(b"Launch\0");
            let mut written = 0;
            if WriteProcessMemory(
                self.info.process,
                remote,
                block.as_ptr() as _,
                block.len(),
                &mut written,
            ) == 0
                || written != block.len()
            {
                return Err(error("WriteProcessMemory"));
            }
            FlushInstructionCache(self.info.process, remote, block.len());
            let thread = CreateRemoteThread(
                self.info.process,
                null_mut(),
                0,
                remote,
                null_mut(),
                0,
                null_mut(),
            );
            if thread.is_null() {
                return Err(error("CreateRemoteThread"));
            }
            let wait = WaitForSingleObject(thread, 60000);
            let mut code = 1;
            let wait_error = if wait == u32::MAX {
                Some(error("WaitForSingleObject"))
            } else {
                None
            };
            let got_code = wait == 0 && GetExitCodeThread(thread, &mut code) != 0;
            CloseHandle(thread);
            // 超时不释放仍可能执行的远程内存，先保留诊断，再由自有子进程退出回收。
            if let Some(e) = wait_error {
                return Err(e);
            }
            if wait == 258 {
                return Err("Launch timeout (60 seconds)".into());
            }
            if wait != 0 {
                return Err(format!("unexpected launch wait status {wait}"));
            }
            if !got_code {
                return Err(error("GetExitCodeThread"));
            }
            VirtualFreeEx(self.info.process, remote, 0, 0x8000);
            if code != 0 {
                return Err(format!("Launch returned 0x{code:08X}"));
            }
            Ok(())
        }
    }
    pub fn exited(&self) -> Option<u32> {
        unsafe {
            if WaitForSingleObject(self.info.process, 0) == 0 {
                let mut code = 1;
                GetExitCodeProcess(self.info.process, &mut code);
                Some(code)
            } else {
                None
            }
        }
    }
    pub fn window(&self) -> Result<usize, String> {
        unsafe {
            struct Search {
                pid: u32,
                windows: Vec<usize>,
            }
            unsafe extern "system" fn collect(h: Handle, data: isize) -> Bool {
                unsafe {
                    let search = &mut *(data as *mut Search);
                    let mut pid = 0;
                    GetWindowThreadProcessId(h, &mut pid);
                    if pid == search.pid && IsWindowVisible(h) != 0 {
                        let mut class = [0u16; 100];
                        let n = GetClassNameW(h, class.as_mut_ptr(), 100);
                        if String::from_utf16_lossy(&class[..n as usize]) != "#32770" {
                            search.windows.push(h as usize);
                        }
                    }
                    1
                }
            }
            let mut s = Search {
                pid: self.pid(),
                windows: vec![],
            };
            EnumWindows(collect, &mut s as *mut _ as isize);
            if s.windows.len() == 1 {
                Ok(s.windows[0])
            } else {
                Err(format!(
                    "expected one visible game window, got {}",
                    s.windows.len()
                ))
            }
        }
    }
    pub fn wait_window(&self, seconds: u64) -> Result<usize, String> {
        let until = Instant::now() + Duration::from_secs(seconds);
        while Instant::now() < until {
            if let Some(code) = self.exited() {
                return Err(format!("game exited early: {code}"));
            }
            if let Ok(window) = self.window() {
                return Ok(window);
            }
            thread::sleep(Duration::from_millis(100));
        }
        Err("window timeout".into())
    }
    pub fn close(&self) -> Result<u32, String> {
        if self.exited().is_none() {
            if let Ok(w) = self.window() {
                unsafe {
                    PostMessageW(w as _, 0x10, 0, 0);
                }
            }
        }
        let until = Instant::now() + Duration::from_secs(15);
        while Instant::now() < until {
            if let Some(code) = self.exited() {
                return Ok(code);
            }
            thread::sleep(Duration::from_millis(100));
        }
        Err("game did not exit after WM_CLOSE".into())
    }
}
impl Drop for Game {
    fn drop(&mut self) {
        unsafe {
            // 只清理本次创建的子进程；失败保留报告，不自动重试。
            if self.exited().is_none() {
                TerminateProcess(self.info.process, 1);
                WaitForSingleObject(self.info.process, 5000);
            }
            CloseHandle(self.info.thread);
            CloseHandle(self.info.process);
        }
    }
}
// 启动等待也必须先获取焦点，不能等到发出第一条菜单按键时才激活游戏。
pub fn focus(window: usize) -> Result<(), String> {
    unsafe {
        ShowWindow(window as _, 9);
        for _ in 0..50 {
            SetForegroundWindow(window as _);
            if GetForegroundWindow() == window as _ {
                return Ok(());
            }
            thread::sleep(Duration::from_millis(10));
        }
        Err("game focus could not be acquired".into())
    }
}
fn scan(window: usize, code: u16, up: bool, extended: bool) -> Result<(), String> {
    unsafe {
        focus(window)?;
        let event = Input {
            kind: 1,
            data: InputData {
                key: Keyboard {
                    vk: 0,
                    scan: code,
                    flags: 8 | if up { 2 } else { 0 } | if extended { 1 } else { 0 },
                    time: 0,
                    extra: 0,
                },
            },
        };
        if SendInput(1, &event, size_of::<Input>() as i32) != 1 {
            return Err(error("SendInput"));
        }
        Ok(())
    }
}
pub fn key(window: usize, name: &str) -> Result<(), String> {
    let (code, extended) = match name {
        "ENTER" => (0x1C, false),
        "ESC" => (1, false),
        "LEFT" => (0x4B, true),
        "RIGHT" => (0x4D, true),
        "UP" => (0x48, true),
        "DOWN" => (0x50, true),
        "CONSOLE" => (0x29, false),
        _ => return Err(format!("unknown key {name}")),
    };
    scan(window, code, false, extended)?;
    thread::sleep(Duration::from_millis(100));
    scan(window, code, true, extended)
}
pub fn text(window: usize, text: &str) -> Result<(), String> {
    for c in text.encode_utf16() {
        unsafe {
            let vk = VkKeyScanW(c);
            if vk < 0 {
                return Err("unmappable console character".into());
            }
            if vk as u16 & 0xFF00 != 0 {
                return Err("console text requires unsupported modifiers".into());
            }
            let code = MapVirtualKeyW(vk as u32 & 255, 0) as u16;
            scan(window, code, false, false)?;
            thread::sleep(Duration::from_millis(15));
            scan(window, code, true, false)?;
        }
    }
    Ok(())
}
// 场景截图先确认游戏焦点；失败诊断则直接截取整个虚拟桌面。
pub fn screenshot(window: usize, path: &Path) -> Result<(), String> {
    focus(window)?;
    let mut rect = Rect::default();
    unsafe {
        if GetClientRect(window as _, &mut rect) == 0 {
            return Err(error("GetClientRect"));
        }
    }
    capture(window, path, 0, 0, rect.right, rect.bottom)
}
pub fn desktop_screenshot(path: &Path) -> Result<(), String> {
    unsafe {
        capture(
            0,
            path,
            GetSystemMetrics(76),
            GetSystemMetrics(77),
            GetSystemMetrics(78),
            GetSystemMetrics(79),
        )
    }
}
fn capture(window: usize, path: &Path, x: i32, y: i32, w: i32, h: i32) -> Result<(), String> {
    unsafe {
        if w <= 0 || h <= 0 || w > 8192 || h > 8192 {
            return Err("invalid screenshot dimensions".into());
        }
        let src = GetDC(window as _);
        let dc = CreateCompatibleDC(src);
        let bmp = CreateCompatibleBitmap(src, w, h);
        let old = SelectObject(dc, bmp);
        let copied = BitBlt(dc, 0, 0, w, h, src, x, y, 0x40CC0020);
        SelectObject(dc, old);
        let mut info = BitmapInfo {
            size: 40,
            width: w,
            height: -h,
            planes: 1,
            bits: 32,
            ..Default::default()
        };
        let mut pixels = vec![0u8; (w * h * 4) as usize];
        let lines = GetDIBits(dc, bmp, 0, h as u32, pixels.as_mut_ptr() as _, &mut info, 0);
        DeleteObject(bmp);
        DeleteDC(dc);
        ReleaseDC(window as _, src);
        if copied == 0 || lines != h {
            return Err(error("screenshot"));
        }
        let mut file = vec![];
        file.extend(b"BM");
        file.extend((54 + pixels.len() as u32).to_le_bytes());
        file.extend([0; 4]);
        file.extend(54u32.to_le_bytes());
        let header = std::slice::from_raw_parts(&info as *const _ as *const u8, 40);
        file.extend(header);
        file.extend(pixels);
        std::fs::write(path, file).map_err(|e| e.to_string())
    }
}
