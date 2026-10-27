//! 持久文件证据：原始目录通知与只读窗口快照；通知不等同于写入成功或调用者归属。
use crate::{Result, Run, write_json};
use serde_json::{Value, json};
use std::{
    ffi::c_void,
    fs::{self, File, OpenOptions},
    io::Write,
    path::PathBuf,
    sync::{
        Arc, Mutex,
        atomic::{AtomicBool, Ordering},
        mpsc,
    },
    thread::{self, JoinHandle},
    time::{Duration, Instant},
};
type Handle = *mut c_void;
// OVERLAPPED 的 offset union 在目录通知中置零；usize 保持 i686/x64 的原生布局。
#[repr(C)]
struct Overlapped {
    internal: usize,
    internal_high: usize,
    offset: u32,
    offset_high: u32,
    event: Handle,
}
#[link(name = "kernel32")]
unsafe extern "system" {
    fn CreateFileW(
        name: *const u16,
        access: u32,
        share: u32,
        security: Handle,
        disposition: u32,
        flags: u32,
        template: Handle,
    ) -> Handle;
    fn CreateEventW(attributes: Handle, manual: i32, initial: i32, name: *const u16) -> Handle;
    fn ReadDirectoryChangesW(
        directory: Handle,
        buffer: *mut c_void,
        length: u32,
        subtree: i32,
        filter: u32,
        returned: *mut u32,
        overlapped: *mut Overlapped,
        completion: Handle,
    ) -> i32;
    fn GetOverlappedResult(
        file: Handle,
        overlapped: *mut Overlapped,
        bytes: *mut u32,
        wait: i32,
    ) -> i32;
    fn WaitForSingleObject(handle: Handle, ms: u32) -> u32;
    fn CancelIoEx(file: Handle, overlapped: *mut Overlapped) -> i32;
    fn CloseHandle(handle: Handle) -> i32;
}
// 一根一个监听线程，64 KiB 通知缓冲、有界事件总量；退出取消并等待在途 IO 后才释放缓冲。
pub struct Monitor {
    stop: Arc<AtomicBool>,
    workers: Vec<JoinHandle<Result<()>>>,
}
impl Monitor {
    pub fn start(roots: Vec<PathBuf>, path: PathBuf) -> Result<Self> {
        let log = Arc::new(Mutex::new(
            OpenOptions::new()
                .write(true)
                .create_new(true)
                .open(path)
                .map_err(|e| e.to_string())?,
        ));
        let stop = Arc::new(AtomicBool::new(false));
        let clock = Instant::now();
        let mut result = Self {
            stop: stop.clone(),
            workers: vec![],
        };
        for root in roots {
            let (ready_tx, ready_rx) = mpsc::channel();
            let log = log.clone();
            let stop = stop.clone();
            result.workers.push(thread::Builder::new().name("save-notifications".into()).spawn(move||{
                let answer=watch(root,&stop,&log,clock,ready_tx);
                if let Err(error)=&answer {let _=record(&log,json!({"event":"error","error":error,"elapsed_us":clock.elapsed().as_micros()}));}
                answer
            }).map_err(|e|e.to_string())?);
            ready_rx
                .recv_timeout(Duration::from_secs(5))
                .map_err(|e| e.to_string())??;
        }
        Ok(result)
    }
    pub fn finish(mut self) -> Result<()> {
        self.stop.store(true, Ordering::Release);
        let mut errors = vec![];
        for worker in self.workers.drain(..) {
            match worker.join() {
                Ok(Ok(())) => (),
                Ok(Err(e)) => errors.push(e),
                Err(_) => errors.push("file_monitor_panic".into()),
            }
        }
        if errors.is_empty() {
            Ok(())
        } else {
            Err(errors.join("; "))
        }
    }
}
impl Drop for Monitor {
    fn drop(&mut self) {
        self.stop.store(true, Ordering::Release);
        for w in self.workers.drain(..) {
            let _ = w.join();
        }
    }
}
fn record(log: &Mutex<File>, value: Value) -> Result<()> {
    let mut file = log.lock().map_err(|_| "file_log_poisoned")?;
    writeln!(file, "{value}").map_err(|e| e.to_string())?;
    file.flush().map_err(|e| e.to_string())
}
fn watch(
    root: PathBuf,
    stop: &AtomicBool,
    log: &Mutex<File>,
    clock: Instant,
    ready: mpsc::Sender<Result<()>>,
) -> Result<()> {
    let name = root
        .to_string_lossy()
        .encode_utf16()
        .chain(Some(0))
        .collect::<Vec<_>>();
    let directory = unsafe {
        CreateFileW(
            name.as_ptr(),
            1,
            7,
            std::ptr::null_mut(),
            3,
            0x42000000,
            std::ptr::null_mut(),
        )
    };
    if directory as isize == -1 {
        let error = format!(
            "monitor_open {}: {}",
            root.display(),
            std::io::Error::last_os_error()
        );
        let _ = ready.send(Err(error.clone()));
        return Err(error);
    }
    let event = unsafe { CreateEventW(std::ptr::null_mut(), 0, 0, std::ptr::null()) };
    if event.is_null() {
        unsafe { CloseHandle(directory) };
        return Err("monitor_event_failed".into());
    }
    let result = (|| -> Result<()> {
        record(
            log,
            json!({"event":"start","root":root,"elapsed_us":clock.elapsed().as_micros()}),
        )?;
        let mut count = 0u64;
        let mut ready = Some(ready);
        loop {
            let mut buffer = vec![0u32; 16384];
            let mut overlapped = Overlapped {
                internal: 0,
                internal_high: 0,
                offset: 0,
                offset_high: 0,
                event,
            };
            let queued = unsafe {
                ReadDirectoryChangesW(
                    directory,
                    buffer.as_mut_ptr().cast(),
                    65536,
                    1,
                    0x1f,
                    std::ptr::null_mut(),
                    &mut overlapped,
                    std::ptr::null_mut(),
                )
            };
            if queued == 0 {
                return Err(format!("monitor_read: {}", std::io::Error::last_os_error()));
            }
            if let Some(ready) = ready.take() {
                let _ = ready.send(Ok(()));
            }
            // 任何退出路径都先取消 IO 并等待完成，避免 kernel 持有已释放的缓冲。
            let mut stopping = false;
            loop {
                let wait = unsafe { WaitForSingleObject(event, 100) };
                if wait == 0 {
                    break;
                }
                if wait != 258 || stop.load(Ordering::Acquire) {
                    unsafe { CancelIoEx(directory, &mut overlapped) };
                    stopping = true;
                    break;
                }
            }
            let mut length = 0;
            let ok = unsafe { GetOverlappedResult(directory, &mut overlapped, &mut length, 1) };
            if stopping {
                return record(
                    log,
                    json!({"event":"stop","root":root,"events":count,"elapsed_us":clock.elapsed().as_micros()}),
                );
            }
            if ok == 0 {
                return Err(format!(
                    "monitor_result: {}",
                    std::io::Error::last_os_error()
                ));
            }
            if length == 0 {
                return Err(format!("monitor_overflow:{}", root.display()));
            }
            let bytes = unsafe {
                std::slice::from_raw_parts(buffer.as_ptr().cast::<u8>(), length as usize)
            };
            let mut offset = 0;
            loop {
                let word = |n: usize| -> Result<u32> {
                    let b = bytes.get(n..n + 4).ok_or("monitor_truncated_record")?;
                    Ok(u32::from_le_bytes(b.try_into().unwrap()))
                };
                let next = word(offset)? as usize;
                let action = word(offset + 4)?;
                let size = word(offset + 8)? as usize;
                if size % 2 != 0 {
                    return Err("monitor_invalid_name".into());
                }
                let name = bytes
                    .get(offset + 12..offset + 12 + size)
                    .ok_or("monitor_truncated_name")?
                    .chunks_exact(2)
                    .map(|b| u16::from_le_bytes([b[0], b[1]]))
                    .collect::<Vec<_>>();
                let name = String::from_utf16(&name).map_err(|e| e.to_string())?;
                count += 1;
                if count > 200000 {
                    return Err("monitor_event_limit".into());
                }
                record(
                    log,
                    json!({"event":"notification","root":root,"relative":name,"action":action,
                    "elapsed_us":clock.elapsed().as_micros(),"attribution":"UNKNOWN"}),
                )?;
                if next == 0 {
                    break;
                }
                if next < 12 || offset + next >= bytes.len() {
                    return Err("monitor_invalid_next".into());
                }
                offset += next;
            }
        }
    })();
    unsafe {
        CloseHandle(event);
        CloseHandle(directory);
    }
    result
}
impl Run {
    // 与运行首尾采用相同冻结根集合；读取期间元数据变化单列，不拿快照当原子文件事务。
    pub(crate) fn files_tx07(&mut self, label: &str) -> Result<()> {
        let start = std::time::SystemTime::now();
        let root = self.out.join("file-windows").join(label);
        let saves = self.saves.as_ref().ok_or("save_roots_not_discovered")?;
        // 首尾读取源元数据：读取失败或读取期间变化均保留，不能宣称快照原子性。
        let mut sources = vec![("documents".to_string(), saves.documents.clone())];
        sources.extend(
            saves
                .steam_remote
                .iter()
                .map(|(account, path)| (format!("steam_remote/{account}"), path.clone())),
        );
        fn inventory(
            path: &std::path::Path,
            prefix: &str,
            out: &mut std::collections::BTreeMap<String, Value>,
        ) -> Result<()> {
            for entry in fs::read_dir(path).map_err(|e| e.to_string())? {
                let entry = entry.map_err(|e| e.to_string())?;
                let kind = entry.file_type().map_err(|e| e.to_string())?;
                if kind.is_symlink() {
                    return Err("window_symlink".into());
                }
                let name = format!("{prefix}/{}", entry.file_name().to_string_lossy());
                if kind.is_dir() {
                    inventory(&entry.path(), &name, out)?;
                } else {
                    out.insert(name, crate::win32::file_state(&entry.path()));
                }
            }
            Ok(())
        }
        let mut before = std::collections::BTreeMap::new();
        let mut errors = vec![];
        for (prefix, path) in &sources {
            if let Err(e) = inventory(path, prefix, &mut before) {
                errors.push(format!("before:{prefix}:{e}"));
            }
        }
        if let Err(e) = saves.snapshot(&root.join("snapshot")) {
            errors.push(format!("copy:{e}"));
        }
        let mut after = std::collections::BTreeMap::new();
        for (prefix, path) in &sources {
            if let Err(e) = inventory(path, prefix, &mut after) {
                errors.push(format!("after:{prefix}:{e}"));
            }
        }
        let result = if errors.is_empty() {
            Ok(())
        } else {
            Err(errors.join("; "))
        };
        let unstable = before
            .keys()
            .chain(after.keys())
            .collect::<std::collections::BTreeSet<_>>()
            .into_iter()
            .filter(|name| before.get(*name) != after.get(*name))
            .cloned()
            .collect::<Vec<_>>();
        let evidence = json!({"label":label,"started_unix_ms":start.duration_since(std::time::UNIX_EPOCH).map_err(|e|e.to_string())?.as_millis(),
            "ended_unix_ms":std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).map_err(|e|e.to_string())?.as_millis(),
            "slot":2,"snapshot":root.join("snapshot"),"error":result.as_ref().err(),"atomic":false,"operation_result":"UNKNOWN",
            "source_before":before,"source_after":after,"possibly_writing":unstable});
        fs::create_dir_all(&root).map_err(|e| e.to_string())?;
        write_json(&root.join("window.json"), &evidence)?;
        self.report["file_windows"][label] = evidence;
        result
    }
}
