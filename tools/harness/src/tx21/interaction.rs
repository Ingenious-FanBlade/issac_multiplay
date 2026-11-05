//! 游戏内面板与备用窗口共用的有界文件信箱；不改变验收步骤或人工结论规则。
use crate::Result;
use serde_json::{json,Value};
use std::{fs,path::{Path,PathBuf},sync::{Mutex,OnceLock,atomic::{AtomicU64,Ordering}},thread,time::Duration};

static DIRECTORY:OnceLock<PathBuf>=OnceLock::new();
static SESSION:OnceLock<String>=OnceLock::new();
static CONTEXT:Mutex<String>=Mutex::new(String::new());
static NEXT:AtomicU64=AtomicU64::new(1);

// 只有统一脚本显式传入本机信箱时启用；原终端入口继续使用原stdin。
pub(super) fn initialize(config:&Value)->Result<()>{
 if let Some(path)=config["ui_directory"].as_str(){
  let path=PathBuf::from(path);if !path.is_absolute()||!path.is_dir(){return Err("GUI信箱必须为已存在的本机绝对目录".into())}
  DIRECTORY.set(path).map_err(|_|"GUI信箱重复初始化".to_owned())?;
  let session=config["ui_session"].as_str().ok_or("GUI会话缺失")?;
  if session.len()!=32||!session.bytes().all(|b|b.is_ascii_hexdigit()){return Err("GUI会话格式".into())}
  SESSION.set(session.to_owned()).map_err(|_|"GUI会话重复初始化".to_owned())?;
 }Ok(())
}
fn publish(path:&Path,value:&Value)->Result<()>{
 let temporary=path.with_extension("new");
 let bytes=serde_json::to_vec(value).map_err(|e|e.to_string())?;
 if bytes.len()>131072{return Err("GUI状态超过有界信箱容量".into())}
 fs::write(&temporary,bytes).map_err(|e|e.to_string())?;
 // Windows rename不覆盖已有文件；读者遇到替换间隙保留上一份完整状态。
 if path.exists(){fs::remove_file(path).map_err(|e|e.to_string())?;}
 fs::rename(temporary,path).map_err(|e|e.to_string())
}
pub(super) fn context(text:&str)->Result<()>{
 *CONTEXT.lock().map_err(|e|e.to_string())?=text.to_owned();
 if let Some(directory)=DIRECTORY.get(){publish(&directory.join("state.json"),&json!({"context":text,"pending":false}))?;}Ok(())
}
pub(super) fn prompt(kind:&str,instruction:&str)->Result<Option<String>>{
 let Some(directory)=DIRECTORY.get()else{return Ok(None)};
 if instruction.len()>65536{return Err("GUI提示超过有界信箱容量".into())}
 let id=format!("{}-{}-{}",SESSION.get().ok_or("GUI会话缺失")?,std::process::id(),NEXT.fetch_add(1,Ordering::Relaxed));
 if !matches!(kind,"ready"|"done"|"observed"|"reviewed"|"retry"){return Err("未声明的GUI提示类型".into())}
 publish(&directory.join("state.json"),&json!({"context":*CONTEXT.lock().map_err(|e|e.to_string())?,"pending":true,"id":id,"kind":kind,"instruction":instruction}))?;
 let reply=directory.join("responses").join(format!("{id}.json"));
 let mut partial=0u32;loop{
  if reply.exists(){
   // 首次写入者用create_new占位；完整文件关闭前允许继续等待，不能把半包当回答。
   let bytes=fs::read(&reply).map_err(|e|e.to_string())?;
   if bytes.len()>16384{return Err("GUI回答超限".into())}
   if let Ok(value)=serde_json::from_slice::<Value>(&bytes){
    if value["id"]!=id{return Err("GUI回答不属于当前提示".into())}
    let answer=value["answer"].as_str().ok_or("GUI回答缺失")?.trim().to_owned();
    fs::remove_file(&reply).map_err(|e|e.to_string())?;
    let context=CONTEXT.lock().map_err(|e|e.to_string())?.clone();context_status(directory,&context)?;
    return Ok(Some(answer))
   }
   partial+=1;if partial>=30{return Err("GUI回答文件未完整写入；保留同tx恢复信息".into())}
  }
  thread::sleep(Duration::from_millis(100));
 }
}
fn context_status(directory:&Path,text:&str)->Result<()>{publish(&directory.join("state.json"),&json!({"context":text,"pending":false}))}

