//! Explicitly opted-in acceptance transport; independent of all production IPC.
#![cfg(windows)]
use crate::error::{AppError,AppResult};
use serde_json::json;
use std::collections::BTreeSet;
use std::fs::{File,OpenOptions};
use std::io::{BufWriter,Write};
use std::mem::{offset_of,size_of};
use std::path::{Path,PathBuf};
use std::ptr::{NonNull,read_volatile,write_volatile};
use std::sync::atomic::{AtomicI32,AtomicU64,Ordering,fence};
use windows::core::PCWSTR;
use windows::Win32::Foundation::{CloseHandle,ERROR_ALREADY_EXISTS,GetLastError,HANDLE,INVALID_HANDLE_VALUE};
use windows::Win32::System::Memory::{CreateFileMappingW,MapViewOfFile,UnmapViewOfFile,
    MEMORY_MAPPED_VIEW_ADDRESS,FILE_MAP_ALL_ACCESS,PAGE_READWRITE};

const MAGIC:u64=0x3145324557534349;
const CAPACITY:usize=65536;
#[repr(C,align(8))]
struct Header {
    magic:u64,version:u32,size:u32,pid:u32,controller:u32,nonce:[u8;32],
    command:AtomicI32,ack:AtomicI32,error:AtomicI32,closed:AtomicI32,
    produced:AtomicU64,consumed:AtomicU64,overflow:AtomicU64,frequency:u64,reserved:[u8;24],
}
#[derive(Clone,Copy)]
#[repr(C,align(8))]
struct Event {
    published:u64,outer:u64,tick:u64,attempt:u64,subject:u64,use_generation:u64,qpc:u64,
    kind:u32,tag:u32,thread:u32,count:u32,words:[u32;14],
}
const SIZE:usize=size_of::<Header>()+CAPACITY*size_of::<Event>();
const _:()=assert!(size_of::<Header>()==128 && offset_of!(Header,produced)==72 && size_of::<Event>()==128);
pub struct Probe {
    handle:HANDLE,view:NonNull<u8>,directory:PathBuf,log:BufWriter<File>,next:u64,issued:BTreeSet<i32>,
    pub name:String,
}
fn error(text:impl ToString)->AppError {AppError::evidence(text.to_string())}
impl Probe {
    pub fn create(world_name:&str,pid:u32,nonce:[u8;32],directory:&Path)->AppResult<Self> {
        let name=format!("{world_name}.E2E");
        let wide:Vec<u16>=name.encode_utf16().chain(Some(0)).collect();
        let handle=unsafe {CreateFileMappingW(INVALID_HANDLE_VALUE,None,PAGE_READWRITE,0,SIZE as u32,PCWSTR(wide.as_ptr()))}.map_err(error)?;
        if unsafe {GetLastError()}==ERROR_ALREADY_EXISTS {let _=unsafe {CloseHandle(handle)};return Err(error("E2E mapping already exists"));}
        let raw=unsafe {MapViewOfFile(handle,FILE_MAP_ALL_ACCESS,0,0,SIZE)};
        let Some(view)=NonNull::new(raw.Value.cast::<u8>()) else {let _=unsafe {CloseHandle(handle)};return Err(error("Map E2E probe failed"));};
        let ptr=view.as_ptr().cast::<Header>();
        unsafe {
            write_volatile(&raw mut (*ptr).magic,MAGIC);write_volatile(&raw mut (*ptr).version,1);
            write_volatile(&raw mut (*ptr).size,SIZE as u32);write_volatile(&raw mut (*ptr).pid,pid);
            write_volatile(&raw mut (*ptr).controller,std::process::id());write_volatile(&raw mut (*ptr).nonce,nonce);
        }
        std::fs::create_dir(directory).map_err(error)?;
        let file=OpenOptions::new().create_new(true).write(true).open(directory.join("trace.jsonl")).map_err(error)?;
        let manifest=json!({"version":1,"purpose":"USER_AUTHORIZED_E2E_ONLY","mapping":name,
            "pid":pid,"controller_pid":std::process::id(),"nonce":hex::encode(nonce),"record_bytes":128,
            "header_bytes":128,"capacity":CAPACITY,"compiled_native_required":"WorldE2E=true",
            "production_ipc_changed":false,"faults":"OBSERVER_OWNED_BRANCH_INJECTION_NOT_GAME_MEMORY_CORRUPTION"});
        std::fs::write(directory.join("manifest.json"),serde_json::to_vec_pretty(&manifest).map_err(error)?).map_err(error)?;
        Ok(Self {handle,view,directory:directory.to_owned(),log:BufWriter::with_capacity(1024*1024,file),next:0,issued:BTreeSet::new(),name})
    }
    fn header(&self)->&Header {unsafe {&*self.view.as_ptr().cast::<Header>()}}
    pub fn drain(&mut self)->AppResult<()> {
        let end=self.header().produced.load(Ordering::Acquire);
        if end<self.next || end-self.next>CAPACITY as u64 {return Err(error("E2E trace sequence/capacity lost"));}
        while self.next<end {
            let ptr=unsafe {self.view.as_ptr().add(128).cast::<Event>().add(self.next as usize%CAPACITY)};
            let event=unsafe {read_volatile(ptr)};
            fence(Ordering::Acquire);
            if event.published!=self.next+1 || event.count>14 {return Err(error("E2E trace publication invalid"));}
            serde_json::to_writer(&mut self.log,&json!({"sequence":event.published.to_string(),
                "outer":event.outer.to_string(),"tick":event.tick.to_string(),"attempt":event.attempt.to_string(),
                "subject":event.subject.to_string(),"use_generation":event.use_generation.to_string(),"qpc":event.qpc.to_string(),
                "kind":event.kind,"tag":event.tag,"thread":event.thread,"words":&event.words[..event.count as usize]})).map_err(error)?;
            self.log.write_all(b"\n").map_err(error)?;self.next+=1;
        }
        self.header().consumed.store(self.next,Ordering::Release);
        Ok(())
    }
    pub fn command(&mut self,name:&str)->AppResult<()> {
        let mode=match name {"object-budget"=>1,"membership-budget"=>2,"relation-budget"=>3,"field-read"=>4,
            "source-changed"=>5,"event-overflow"=>6,"wrong-thread"=>7,"reference-ambiguity"=>8,"root-changed"=>9,
            _=>return Err(error("Unknown E2E observer fault"))};
        if self.header().closed.load(Ordering::Acquire)!=0 || self.header().command.load(Ordering::Acquire)!=0 || !self.issued.insert(mode) {
            return Err(error("E2E fault pending, repeated, or observer closed; no automatic retry"));
        }
        self.header().ack.store(0,Ordering::Release);self.header().command.store(mode,Ordering::Release);Ok(())
    }
    pub fn finish(&mut self)->AppResult<()> {
        self.drain()?;self.log.flush().map_err(error)?;
        let header=self.header();
        let value=json!({"version":1,"closed":header.closed.load(Ordering::Acquire)!=0,
            "error":header.error.load(Ordering::Acquire),"overflow":header.overflow.load(Ordering::Acquire).to_string(),
            "produced":header.produced.load(Ordering::Acquire).to_string(),"consumed":self.next.to_string(),
            "pending_command":header.command.load(Ordering::Acquire),"last_ack":header.ack.load(Ordering::Acquire),
            "qpc_frequency":unsafe {read_volatile(&raw const header.frequency)}.to_string(),"issued_faults":self.issued});
        let mut file=OpenOptions::new().create_new(true).write(true).open(self.directory.join("result.json")).map_err(error)?;
        serde_json::to_writer_pretty(&mut file,&value).map_err(error)?;file.write_all(b"\n").map_err(error)
    }
}
impl Drop for Probe {
    fn drop(&mut self) {let _=self.log.flush();unsafe {let _=UnmapViewOfFile(MEMORY_MAPPED_VIEW_ADDRESS{Value:self.view.as_ptr().cast()});let _=CloseHandle(self.handle);}}
}
