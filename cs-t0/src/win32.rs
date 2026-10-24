use std::ffi::c_void;
use std::mem::size_of;
use std::path::{Path, PathBuf};
use std::thread;
use std::time::{Duration, Instant};

use windows::Win32::Foundation::{
    CloseHandle, FILETIME, HANDLE, HMODULE, HWND, LPARAM, WAIT_OBJECT_0, WAIT_TIMEOUT,
};
use windows::Win32::System::Diagnostics::Debug::{CheckRemoteDebuggerPresent, ReadProcessMemory};
use windows::Win32::System::Diagnostics::ToolHelp::{
    CreateToolhelp32Snapshot, PROCESSENTRY32W, Process32FirstW, Process32NextW, TH32CS_SNAPPROCESS,
};
use windows::Win32::System::Performance::{QueryPerformanceCounter, QueryPerformanceFrequency};
use windows::Win32::System::ProcessStatus::{
    K32EnumProcessModulesEx, K32GetModuleBaseNameW, K32GetModuleInformation, LIST_MODULES_32BIT,
    MODULEINFO,
};
use windows::Win32::System::SystemInformation::IMAGE_FILE_MACHINE_I386;
use windows::Win32::System::Threading::{
    GetCurrentThreadId, GetExitCodeProcess, GetProcessTimes, IsWow64Process2, OpenProcess,
    PROCESS_ACCESS_RIGHTS, PROCESS_QUERY_INFORMATION, PROCESS_QUERY_LIMITED_INFORMATION,
    PROCESS_SYNCHRONIZE, PROCESS_VM_READ, QueryFullProcessImageNameW, WaitForSingleObject,
};
use windows::Win32::UI::WindowsAndMessaging::{
    EnumWindows, GA_ROOT, GetAncestor, GetWindowThreadProcessId, IsWindowVisible,
};
use windows::core::{BOOL, PWSTR};

use crate::error::{AppError, AppResult};

pub const OBSERVER_ACCESS_BITS: u32 = PROCESS_QUERY_INFORMATION.0
    | PROCESS_QUERY_LIMITED_INFORMATION.0
    | PROCESS_VM_READ.0
    | PROCESS_SYNCHRONIZE.0;

const TARGET_EXE: &str = "isaac-ng.exe";
const BLOCKED_PROCESSES: &[&str] = &[
    "issaconlinesync.exe",
    "x32dbg.exe",
    "x64dbg.exe",
    "cheatengine.exe",
    "cheatengine-x86_64.exe",
];

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct ModuleInfo {
    pub name: String,
    pub base: u64,
    pub size: u32,
}

pub struct ObservedProcess {
    handle: HANDLE,
    pub pid: u32,
    pub creation_time_100ns: u64,
    pub image_path: PathBuf,
    pub modules: Vec<ModuleInfo>,
    pub main_module: ModuleInfo,
}

impl Drop for ObservedProcess {
    fn drop(&mut self) {
        // SAFETY: `handle` is returned by OpenProcess and is owned by this value.
        let _ = unsafe { CloseHandle(self.handle) };
    }
}

impl ObservedProcess {
    pub fn open_exact(pid: u32, expected_path: &Path) -> AppResult<Self> {
        let access = PROCESS_ACCESS_RIGHTS(OBSERVER_ACCESS_BITS);
        // SAFETY: the PID comes from a Toolhelp snapshot and the handle is closed in Drop.
        let handle = unsafe { OpenProcess(access, false, pid) }
            .map_err(|error| AppError::evidence(format!("OpenProcess({pid}) failed: {error}")))?;

        let result = (|| {
            let image_path = query_image_path(handle)?;
            if normalize_path(&image_path) != normalize_path(expected_path) {
                return Err(AppError::evidence(format!(
                    "PID {pid} image path does not match the inspected game executable"
                )));
            }
            ensure_x86(handle)?;
            let creation_time_100ns = process_creation_time(handle)?;
            let modules = enumerate_modules_with_retry(handle, pid)?;
            let main_module = modules
                .iter()
                .find(|module| module.name.eq_ignore_ascii_case(TARGET_EXE))
                .cloned()
                .ok_or_else(|| AppError::evidence("target process has no isaac-ng.exe module"))?;
            Ok(Self {
                handle,
                pid,
                creation_time_100ns,
                image_path,
                modules,
                main_module,
            })
        })();
        if result.is_err() {
            // SAFETY: construction failed, so ownership was not transferred to Self.
            let _ = unsafe { CloseHandle(handle) };
        }
        result
    }

    pub fn read_memory(&self, address: u64, size: usize) -> AppResult<Vec<u8>> {
        let address = usize::try_from(address)
            .map_err(|_| AppError::evidence("remote address does not fit this observer"))?;
        let mut bytes = vec![0_u8; size];
        let mut read = 0_usize;
        // SAFETY: the destination covers `size` bytes; this call only reads the remote process.
        unsafe {
            ReadProcessMemory(
                self.handle,
                address as *const c_void,
                bytes.as_mut_ptr().cast::<c_void>(),
                size,
                Some(&mut read),
            )
        }
        .map_err(|error| {
            AppError::evidence(format!(
                "ReadProcessMemory(0x{address:08X}, {size}) failed: {error}"
            ))
        })?;
        if read != size {
            return Err(AppError::evidence(format!(
                "short remote read at 0x{address:08X}: expected {size}, got {read}"
            )));
        }
        Ok(bytes)
    }

    pub fn debugger_present(&self) -> AppResult<bool> {
        let mut present = BOOL(0);
        // SAFETY: the output points to a valid BOOL and the process handle remains live.
        unsafe { CheckRemoteDebuggerPresent(self.handle, &mut present) }
            .map_err(|error| AppError::evidence(format!("debugger query failed: {error}")))?;
        Ok(present.as_bool())
    }

    pub fn owner_module(&self, address: u64) -> Option<&ModuleInfo> {
        self.modules.iter().find(|module| {
            address >= module.base && address < module.base.saturating_add(u64::from(module.size))
        })
    }

    pub fn forbidden_modules(&self) -> Vec<String> {
        self.modules
            .iter()
            .filter(|module| is_forbidden_module(&module.name))
            .map(|module| module.name.clone())
            .collect()
    }

    pub fn wait(&self, milliseconds: u32) -> AppResult<bool> {
        // SAFETY: the handle was opened with SYNCHRONIZE and remains valid.
        let result = unsafe { WaitForSingleObject(self.handle, milliseconds) };
        if result == WAIT_OBJECT_0 {
            Ok(true)
        } else if result == WAIT_TIMEOUT {
            Ok(false)
        } else {
            Err(AppError::evidence(format!(
                "WaitForSingleObject returned 0x{:08X}",
                result.0
            )))
        }
    }

    pub fn exit_code(&self) -> AppResult<u32> {
        let mut code = 0_u32;
        // SAFETY: `code` is writable and the process handle remains valid after signaling.
        unsafe { GetExitCodeProcess(self.handle, &mut code) }.map_err(|error| {
            AppError::evidence(format!("process exit code query failed: {error}"))
        })?;
        Ok(code)
    }
}

pub fn find_target_pids() -> AppResult<Vec<u32>> {
    Ok(enumerate_processes()?
        .into_iter()
        .filter(|(_, name)| name.eq_ignore_ascii_case(TARGET_EXE))
        .map(|(pid, _)| pid)
        .collect())
}

pub fn unique_target_image_path() -> AppResult<PathBuf> {
    let pids = find_target_pids()?;
    if pids.len() != 1 {
        return Err(AppError::evidence(format!(
            "exactly one isaac-ng.exe is required; observed {pids:?}"
        )));
    }
    // SAFETY: the PID comes from a Toolhelp snapshot; the handle is closed below.
    let handle = unsafe {
        OpenProcess(
            PROCESS_ACCESS_RIGHTS(PROCESS_QUERY_LIMITED_INFORMATION.0),
            false,
            pids[0],
        )
    }
    .map_err(|error| AppError::evidence(format!("OpenProcess({}) failed: {error}", pids[0])))?;
    let result = query_image_path(handle);
    // SAFETY: the handle is owned by this function.
    let _ = unsafe { CloseHandle(handle) };
    result
}

pub fn running_blockers() -> AppResult<Vec<String>> {
    let mut blockers = enumerate_processes()?
        .into_iter()
        .filter(|(_, name)| {
            BLOCKED_PROCESSES
                .iter()
                .any(|item| name.eq_ignore_ascii_case(item))
        })
        .map(|(_, name)| name)
        .collect::<Vec<_>>();
    blockers.sort_by_key(|name| name.to_ascii_lowercase());
    blockers.dedup_by(|left, right| left.eq_ignore_ascii_case(right));
    Ok(blockers)
}

pub fn qpc_now() -> AppResult<i64> {
    let mut value = 0_i64;
    // SAFETY: `value` is a valid output location.
    unsafe { QueryPerformanceCounter(&mut value) }
        .map_err(|error| AppError::evidence(format!("QueryPerformanceCounter failed: {error}")))?;
    Ok(value)
}

pub fn qpc_frequency() -> AppResult<i64> {
    let mut value = 0_i64;
    // SAFETY: `value` is a valid output location.
    unsafe { QueryPerformanceFrequency(&mut value) }.map_err(|error| {
        AppError::evidence(format!("QueryPerformanceFrequency failed: {error}"))
    })?;
    if value <= 0 {
        return Err(AppError::evidence(
            "QueryPerformanceFrequency returned a non-positive value",
        ));
    }
    Ok(value)
}

pub fn current_thread_id() -> u32 {
    // SAFETY: GetCurrentThreadId has no preconditions.
    unsafe { GetCurrentThreadId() }
}

pub fn unique_visible_root_window_thread(pid: u32) -> AppResult<u32> {
    if pid == 0 {
        return Err(AppError::usage(
            "window thread lookup requires a nonzero PID",
        ));
    }
    struct Context {
        pid: u32,
        threads: Vec<u32>,
    }
    unsafe extern "system" fn visit(hwnd: HWND, lparam: LPARAM) -> BOOL {
        let context = unsafe { &mut *(lparam.0 as *mut Context) };
        let mut window_pid = 0_u32;
        let thread_id = unsafe { GetWindowThreadProcessId(hwnd, Some(&mut window_pid)) };
        if window_pid == context.pid
            && thread_id != 0
            && unsafe { IsWindowVisible(hwnd).as_bool() }
            && unsafe { GetAncestor(hwnd, GA_ROOT) } == hwnd
        {
            context.threads.push(thread_id);
        }
        BOOL(1)
    }

    let mut context = Context {
        pid,
        threads: Vec::new(),
    };
    unsafe {
        EnumWindows(
            Some(visit),
            LPARAM((&raw mut context).cast::<c_void>() as isize),
        )
    }
    .map_err(|error| AppError::evidence(format!("EnumWindows failed: {error}")))?;
    context.threads.sort_unstable();
    context.threads.dedup();
    match context.threads.as_slice() {
        [thread_id] => Ok(*thread_id),
        [] => Err(AppError::evidence(
            "target has no unique visible root game window for menu attestation",
        )),
        _ => Err(AppError::evidence(
            "target has multiple visible root window threads; menu attestation is ambiguous",
        )),
    }
}

fn enumerate_processes() -> AppResult<Vec<(u32, String)>> {
    // SAFETY: no process-specific input is required for a process snapshot.
    let snapshot = unsafe { CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) }
        .map_err(|error| AppError::evidence(format!("process snapshot failed: {error}")))?;
    let result = (|| {
        let mut entry = PROCESSENTRY32W {
            dwSize: u32::try_from(size_of::<PROCESSENTRY32W>())
                .map_err(|_| AppError::evidence("PROCESSENTRY32W size overflow"))?,
            ..Default::default()
        };
        let mut processes = Vec::new();
        // SAFETY: `entry.dwSize` is initialized and entry remains writable.
        if unsafe { Process32FirstW(snapshot, &mut entry) }.is_err() {
            return Ok(processes);
        }
        loop {
            processes.push((entry.th32ProcessID, utf16_z(&entry.szExeFile)));
            // SAFETY: same initialized entry and snapshot as above.
            if unsafe { Process32NextW(snapshot, &mut entry) }.is_err() {
                break;
            }
        }
        Ok(processes)
    })();
    // SAFETY: snapshot is owned by this function.
    let _ = unsafe { CloseHandle(snapshot) };
    result
}

fn enumerate_modules_with_retry(handle: HANDLE, pid: u32) -> AppResult<Vec<ModuleInfo>> {
    let deadline = Instant::now() + Duration::from_secs(15);
    loop {
        let last_error = match enumerate_modules(handle) {
            Ok(modules) if !modules.is_empty() => return Ok(modules),
            Ok(_) => "module list was empty".to_owned(),
            Err(error) => error.message,
        };
        // SAFETY: handle is valid and opened with SYNCHRONIZE.
        if unsafe { WaitForSingleObject(handle, 0) } == WAIT_OBJECT_0 {
            return Err(AppError::evidence(format!(
                "PID {pid} exited before module enumeration completed"
            )));
        }
        if Instant::now() >= deadline {
            return Err(AppError::evidence(format!(
                "module enumeration for PID {pid} did not become readable: {}",
                last_error
            )));
        }
        thread::sleep(Duration::from_millis(100));
    }
}

fn enumerate_modules(handle: HANDLE) -> AppResult<Vec<ModuleInfo>> {
    let mut handles = vec![HMODULE::default(); 128];
    loop {
        let byte_capacity = handles
            .len()
            .checked_mul(size_of::<HMODULE>())
            .and_then(|size| u32::try_from(size).ok())
            .ok_or_else(|| AppError::evidence("module handle buffer size overflow"))?;
        let mut needed = 0_u32;
        // SAFETY: the handle buffer is writable for byte_capacity and needed is a valid output.
        let succeeded = unsafe {
            K32EnumProcessModulesEx(
                handle,
                handles.as_mut_ptr(),
                byte_capacity,
                &mut needed,
                LIST_MODULES_32BIT.0,
            )
        };
        if !succeeded.as_bool() {
            return Err(AppError::evidence(format!(
                "K32EnumProcessModulesEx failed: {}",
                windows::core::Error::from_win32()
            )));
        }
        if needed > byte_capacity {
            let needed = usize::try_from(needed)
                .ok()
                .and_then(|size| size.checked_add(size_of::<HMODULE>() - 1))
                .map(|size| size / size_of::<HMODULE>())
                .ok_or_else(|| AppError::evidence("module handle count overflow"))?;
            handles.resize(needed, HMODULE::default());
            continue;
        }
        let count = usize::try_from(needed)
            .map_err(|_| AppError::evidence("module handle byte count overflow"))?
            / size_of::<HMODULE>();
        handles.truncate(count);
        break;
    }

    let mut modules = Vec::with_capacity(handles.len());
    for module in handles {
        let mut name = [0_u16; 1024];
        // SAFETY: name is writable and module came from K32EnumProcessModulesEx.
        let name_length = unsafe { K32GetModuleBaseNameW(handle, Some(module), &mut name) };
        if name_length == 0 {
            return Err(AppError::evidence(format!(
                "K32GetModuleBaseNameW failed: {}",
                windows::core::Error::from_win32()
            )));
        }
        let mut information = MODULEINFO::default();
        // SAFETY: information is writable and module came from the same process handle.
        let succeeded = unsafe {
            K32GetModuleInformation(
                handle,
                module,
                &mut information,
                u32::try_from(size_of::<MODULEINFO>())
                    .map_err(|_| AppError::evidence("MODULEINFO size overflow"))?,
            )
        };
        if !succeeded.as_bool() {
            return Err(AppError::evidence(format!(
                "K32GetModuleInformation failed: {}",
                windows::core::Error::from_win32()
            )));
        }
        modules.push(ModuleInfo {
            name: String::from_utf16_lossy(&name[..usize::try_from(name_length).unwrap()]),
            base: information.lpBaseOfDll as usize as u64,
            size: information.SizeOfImage,
        });
    }
    modules.sort_by_key(|module| module.base);
    Ok(modules)
}

fn query_image_path(handle: HANDLE) -> AppResult<PathBuf> {
    let mut buffer = vec![0_u16; 32_768];
    let mut size = u32::try_from(buffer.len())
        .map_err(|_| AppError::evidence("process image path buffer too large"))?;
    // SAFETY: buffer is writable for `size` UTF-16 code units.
    unsafe {
        QueryFullProcessImageNameW(
            handle,
            Default::default(),
            PWSTR(buffer.as_mut_ptr()),
            &mut size,
        )
    }
    .map_err(|error| AppError::evidence(format!("process image path query failed: {error}")))?;
    let used =
        usize::try_from(size).map_err(|_| AppError::evidence("process path size overflow"))?;
    Ok(PathBuf::from(String::from_utf16_lossy(&buffer[..used])))
}

fn ensure_x86(handle: HANDLE) -> AppResult<()> {
    let mut process_machine = Default::default();
    let mut native_machine = Default::default();
    // SAFETY: both output pointers are valid.
    unsafe { IsWow64Process2(handle, &mut process_machine, Some(&mut native_machine)) }.map_err(
        |error| AppError::evidence(format!("process architecture query failed: {error}")),
    )?;
    if process_machine != IMAGE_FILE_MACHINE_I386 {
        return Err(AppError::evidence(format!(
            "target process is not x86 (process machine=0x{:04X})",
            process_machine.0
        )));
    }
    Ok(())
}

fn process_creation_time(handle: HANDLE) -> AppResult<u64> {
    let mut creation = FILETIME::default();
    let mut exit = FILETIME::default();
    let mut kernel = FILETIME::default();
    let mut user = FILETIME::default();
    // SAFETY: all FILETIME outputs are valid and writable.
    unsafe { GetProcessTimes(handle, &mut creation, &mut exit, &mut kernel, &mut user) }
        .map_err(|error| AppError::evidence(format!("process time query failed: {error}")))?;
    Ok((u64::from(creation.dwHighDateTime) << 32) | u64::from(creation.dwLowDateTime))
}

fn normalize_path(path: &Path) -> String {
    path.to_string_lossy()
        .trim_start_matches(r"\\?\")
        .replace('/', "\\")
        .to_ascii_lowercase()
}

fn utf16_z(buffer: &[u16]) -> String {
    let end = buffer
        .iter()
        .position(|value| *value == 0)
        .unwrap_or(buffer.len());
    String::from_utf16_lossy(&buffer[..end])
}

fn is_forbidden_module(name: &str) -> bool {
    let lower = name.to_ascii_lowercase();
    lower.contains("isaacsync")
        || lower.contains("isaac_cs_t0_native")
        || lower.contains("isaaccst0.native")
        || lower.contains("x32dbg")
        || lower.contains("x64dbg")
        || lower.contains("vehdebug")
        || lower.contains("tractor_beam_native_hook")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn observer_access_is_read_only_and_synchronizable() {
        assert_eq!(
            OBSERVER_ACCESS_BITS,
            PROCESS_QUERY_INFORMATION.0
                | PROCESS_QUERY_LIMITED_INFORMATION.0
                | PROCESS_VM_READ.0
                | PROCESS_SYNCHRONIZE.0
        );
        assert_eq!(
            OBSERVER_ACCESS_BITS & 0x002A,
            0,
            "no create/write/operation rights"
        );
    }

    #[test]
    fn path_normalization_does_not_expose_or_transform_components() {
        assert_eq!(
            normalize_path(Path::new(r"\\?\F:\Steam\ISAAC-NG.EXE")),
            r"f:\steam\isaac-ng.exe"
        );
    }

    #[test]
    fn active_preflight_denylist_includes_tractor_beam_hook() {
        assert!(is_forbidden_module("tractor_beam_native_hook.dll"));
        assert!(is_forbidden_module("TRACTOR_BEAM_NATIVE_HOOK.DLL"));
        assert!(!is_forbidden_module("USER32.dll"));
    }
}
