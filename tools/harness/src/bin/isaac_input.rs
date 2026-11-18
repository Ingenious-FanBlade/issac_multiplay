//! 第15步持续输入续验：仅操作Client1隔离副本，使用真实键盘和既有C/S输入链路。
#[allow(dead_code)]
#[path = "../win32.rs"]
mod win32;

use serde_json::Value;
use std::{collections::BTreeMap, env, fs, io::{BufRead, BufReader, Write},
    net::{SocketAddr, TcpStream}, thread, time::{Duration, Instant}};
type Result<T> = std::result::Result<T, String>;
const CLIENT_EXE: &str = r"F:\projects_isaac\Isaac-for-test\client1\The Binding of Isaac Rebirth\Repentogon\isaac-ng.exe";

// 只读控制口先核E2E令牌及当前显示状态；不接入生产副本，也不改写原生位置。
fn query(command: &str) -> Result<BTreeMap<String, String>> {
    let mut stream = TcpStream::connect_timeout(&SocketAddr::from(([127,0,0,1],27092)), Duration::from_secs(5)).map_err(|e| e.to_string())?;
    stream.set_read_timeout(Some(Duration::from_secs(5))).map_err(|e| e.to_string())?;
    stream.set_write_timeout(Some(Duration::from_secs(5))).map_err(|e| e.to_string())?;
    writeln!(stream,"{command}").map_err(|e| e.to_string())?;
    let mut reader=BufReader::new(stream);let mut values=BTreeMap::new();let mut size=0;
    loop {
        let mut line=String::new();let n=reader.read_line(&mut line).map_err(|e| e.to_string())?;
        if n==0 { return Err("input control disconnected".into()); }
        size+=n;if size>65536 { return Err("input control response too large".into()); }
        let line=line.trim_end();if line=="." { return Ok(values); }
        if let Some((key,value))=line.split_once('=') { values.insert(key.into(),value.into()); }
    }
}
fn run() -> Result<()> {
    let args: Vec<_>=env::args().collect();
    if args.len()!=4 { return Err("usage: isaac-input CLIENT_LAUNCH_JSON E2E_TOKEN SECONDS".into()); }
    let seconds=args[3].parse::<u64>().map_err(|e| e.to_string())?;
    if !(1..=240).contains(&seconds)||args[2].is_empty()||!args[2].bytes().all(|b| b.is_ascii_alphanumeric()||b==b'-'||b==b'_') {
        return Err("invalid bounded input duration or token".into());
    }
    let launch: Value=serde_json::from_slice(&fs::read(&args[1]).map_err(|e| e.to_string())?).map_err(|e| e.to_string())?;
    if !launch["exe"].as_str().is_some_and(|p| p.eq_ignore_ascii_case(CLIENT_EXE)) { return Err("input requires Client1 isolated executable".into()); }
    let pid=u32::try_from(launch["pid"].as_u64().ok_or("missing input target pid")?).map_err(|e| e.to_string())?;
    if !win32::process_image(pid)?.to_string_lossy().eq_ignore_ascii_case(CLIENT_EXE) { return Err("input target process changed".into()); }
    let auth=query(&format!("cs client_test {} fault-status",args[2]))?;
    let state=query("cs status")?;
    if auth.get("ok").map(String::as_str)!=Some("1")||state.get("client.scene_state").map(String::as_str)!=Some("3")||
        state.get("client.error").map(String::as_str)!=Some("") { return Err("input requires active E2E client scene".into()); }
    let windows: Vec<_>=win32::windows(pid).into_iter().filter(|w| w["visible"]==true&&w["class"]!="#32770"&&
        w["title"].as_str().is_some_and(|t| t.contains("The Binding of Isaac"))).collect();
    if windows.len()!=1 { return Err("input requires one visible game window".into()); }
    let window=windows[0]["handle"].as_u64().ok_or("missing input window")? as usize;
    win32::focus(window)?;win32::set_auto_focus(false);
    let _awake=win32::keep_display_awake()?;
    let start=Instant::now();let deadline=start+Duration::from_secs(seconds);let mut phase=0;
    println!("input started: Client1 PID {pid}, {seconds}s");
    // 800ms四方向闭环持续移动并向上射击；失焦立即退出，HeldKeys在任何返回／panic时释放。
    while Instant::now()<deadline {
        let direction=["D","S","A","W"][phase%4];let mut keys=win32::HeldKeys::new(window);
        keys.press(direction)?;keys.press("UP")?;
        let until=(Instant::now()+Duration::from_millis(800)).min(deadline);
        while Instant::now()<until {
            if win32::focus_state(window)["active"]!=true { return Err("input stopped after focus loss".into()); }
            thread::sleep(Duration::from_millis(20));
        }
        keys.release("UP")?;keys.release(direction)?;phase+=1;
    }
    println!("input released: {} phases, {}ms",phase,start.elapsed().as_millis());Ok(())
}
fn main() { if let Err(error)=run() { eprintln!("{error}");std::process::exit(1); } }
