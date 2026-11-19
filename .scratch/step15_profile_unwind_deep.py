"""临时栈展开采样：按本机SDK结构使用StackWalk64，逐次finally恢复；不用于性能验收计量。"""
import ctypes as C
from ctypes import wintypes as W
import collections,json,struct,sys,time
k=C.WinDLL('kernel32',use_last_error=True)
class Entry(C.Structure):
    _fields_=[(x,W.DWORD) for x in ('size','usage','tid','pid','priority','delta','flags')]
for name,args,ret in (
    ('CreateToolhelp32Snapshot',[W.DWORD,W.DWORD],W.HANDLE),
    ('Thread32First',[W.HANDLE,C.POINTER(Entry)],W.BOOL),('Thread32Next',[W.HANDLE,C.POINTER(Entry)],W.BOOL),
    ('OpenThread',[W.DWORD,W.BOOL,W.DWORD],W.HANDLE),('OpenProcess',[W.DWORD,W.BOOL,W.DWORD],W.HANDLE),
    ('GetThreadTimes',[W.HANDLE,C.c_void_p,C.c_void_p,C.c_void_p,C.c_void_p],W.BOOL),
    ('SuspendThread',[W.HANDLE],W.DWORD),('ResumeThread',[W.HANDLE],W.DWORD),
    ('Wow64GetThreadContext',[W.HANDLE,C.c_void_p],W.BOOL),('ReadProcessMemory',[W.HANDLE,C.c_void_p,C.c_void_p,C.c_size_t,C.c_void_p],W.BOOL),
    ('CloseHandle',[W.HANDLE],W.BOOL)):
    f=getattr(k,name);f.argtypes=args;f.restype=ret
pid=int(sys.argv[1]);base=int(sys.argv[2],16);extent=int(sys.argv[3]);seconds=int(sys.argv[4])
s=k.CreateToolhelp32Snapshot(4,0);e=Entry();e.size=C.sizeof(e);threads=[]
ok=k.Thread32First(s,C.byref(e))
while ok:
    if e.pid==pid:
        h=k.OpenThread(0x0002|0x0008|0x0040,False,e.tid)
        if h:
            times=[C.c_ulonglong() for _ in range(4)]
            if k.GetThreadTimes(h,*[C.byref(t) for t in times]): threads.append((times[0].value,e.tid,h))
    ok=k.Thread32Next(s,C.byref(e))
k.CloseHandle(s)
threads.sort();_,tid,h=threads[0]
for _,_,other in threads[1:]:k.CloseHandle(other)

p=k.OpenProcess(0x410,False,pid);assert p
from pathlib import Path
d=C.WinDLL('dbghelp',use_last_error=True)
class Address(C.Structure):
    _fields_=[('Offset',C.c_ulonglong),('Segment',W.WORD),('Mode',C.c_int)]
class KdHelp(C.Structure):
    _fields_=[('Thread',C.c_ulonglong),*[(n,W.DWORD) for n in ('ThCallbackStack','ThCallbackBStore','NextCallback','FramePointer')],*[(n,C.c_ulonglong) for n in ('KiCallUserMode','KeUserCallbackDispatcher','SystemRangeStart','KiUserExceptionDispatcher','StackBase','StackLimit')],('BuildVersion',W.DWORD),('RetpolineStubFunctionTableSize',W.DWORD),('RetpolineStubFunctionTable',C.c_ulonglong),('RetpolineStubOffset',W.DWORD),('RetpolineStubSize',W.DWORD),('Reserved0',C.c_ulonglong*2)]
class Frame(C.Structure):
    _fields_=[*[(n,Address) for n in ('AddrPC','AddrReturn','AddrFrame','AddrStack','AddrBStore')],('FuncTableEntry',C.c_void_p),('Params',C.c_ulonglong*4),('Far',W.BOOL),('Virtual',W.BOOL),('Reserved',C.c_ulonglong*3),('KdHelp',KdHelp)]
class Info(C.Structure):
    _fields_=[('SizeOfStruct',W.ULONG),('TypeIndex',W.ULONG),('Reserved',C.c_ulonglong*2),('Index',W.ULONG),('Size',W.ULONG),('ModBase',C.c_ulonglong),('Flags',W.ULONG),('Value',C.c_ulonglong),('Address',C.c_ulonglong),('Register',W.ULONG),('Scope',W.ULONG),('Tag',W.ULONG),('NameLen',W.ULONG),('MaxNameLen',W.ULONG),('Name',C.c_char*1)]
for name,args,ret in (
 ('SymInitializeW',[W.HANDLE,W.LPCWSTR,W.BOOL],W.BOOL),
 ('SymCleanup',[W.HANDLE],W.BOOL),
 ('SymLoadModuleExW',[W.HANDLE,W.HANDLE,W.LPCWSTR,W.LPCWSTR,C.c_ulonglong,W.DWORD,C.c_void_p,W.DWORD],C.c_ulonglong),
 ('SymFromAddr',[W.HANDLE,C.c_ulonglong,C.POINTER(C.c_ulonglong),C.POINTER(Info)],W.BOOL),
 ('StackWalk64',[W.DWORD,W.HANDLE,W.HANDLE,C.POINTER(Frame),C.c_void_p,C.c_void_p,C.c_void_p,C.c_void_p,C.c_void_p],W.BOOL)):
    f=getattr(d,name);f.argtypes=args;f.restype=ret
d.SymSetOptions(0x10|0x2|0x200|0x2000|0x80000)
assert d.SymInitializeW(p,str(Path('out/repentogon/e2e').resolve()),True),C.get_last_error()
d.SymLoadModuleExW(p,None,str(Path('out/repentogon/e2e/zhlIsaacCS.dll').resolve()),None,base,extent,None,0)
ctx=C.create_string_buffer(716);chains=collections.Counter();pcs=collections.Counter();depths=collections.Counter();n=0;max_pause=0
end=time.monotonic()+seconds
try:
    while time.monotonic()<end:
        C.c_uint32.from_buffer(ctx).value=0x10007
        suspended=k.SuspendThread(h)!=0xffffffff
        start=time.perf_counter();chain=[]
        try:
            if suspended and k.Wow64GetThreadContext(h,ctx):
                frame=Frame()
                for name,offset in (('AddrPC',184),('AddrFrame',180),('AddrStack',196)):
                    address=getattr(frame,name);address.Offset=C.c_uint32.from_buffer(ctx,offset).value;address.Mode=3
                initial_pc=frame.AddrPC.Offset;pcs[initial_pc]+=1;n+=1;seen=set()
                for depth in range(48):
                    if not d.StackWalk64(0x14c,p,h,C.byref(frame),ctx,None,C.cast(d.SymFunctionTableAccess64,C.c_void_p),C.cast(d.SymGetModuleBase64,C.c_void_p),None):break
                    pair=(frame.AddrPC.Offset,frame.AddrStack.Offset)
                    if not pair[0] or pair in seen:break
                    seen.add(pair);chain.append(pair[0])
                    if time.perf_counter()-start>.025:break
                if not chain:chain=[initial_pc]
        finally:
            if suspended:k.ResumeThread(h)
        max_pause=max(max_pause,time.perf_counter()-start)
        depths[len(chain)]+=1;chains[tuple(chain)]+=1
        time.sleep(.01)
    names={}
    for address in {a for chain in chains for a in chain}:
        buf=C.create_string_buffer(C.sizeof(Info)+2048);info=C.cast(buf,C.POINTER(Info));info.contents.SizeOfStruct=C.sizeof(Info);info.contents.MaxNameLen=2048;offset=C.c_ulonglong()
        names[address]=C.string_at(C.addressof(buf)+Info.Name.offset).decode(errors='replace') if d.SymFromAddr(p,address,C.byref(offset),info) else hex(address)
    first=collections.Counter();owned=collections.Counter();symbol_chains=collections.Counter()
    capture_first=collections.Counter();client_first=collections.Counter();inclusive=collections.Counter()
    for chain,count in chains.items():
        project=[names[a] for a in chain if base<=a<base+extent]
        if project:
            first[project[0]]+=count;owned[tuple(dict.fromkeys(project))]+=count
            for name in set(project): inclusive[name]+=count
            if any("BinaryEntities::Prepare" in name for name in project): capture_first[project[0]]+=count
            if any("ClientManagerBoundary" in name for name in project): client_first[project[0]]+=count
        symbol_chains[tuple(dict.fromkeys(names[a] for a in chain))]+=count
    result={'pid':pid,'tid':tid,'samples':n,'max_pause_ms':max_pause*1000,'depths':dict(depths),'first':first.most_common(35),'capture_first':capture_first.most_common(),'client_first':client_first.most_common(),'project_inclusive':inclusive.most_common(),'project_chains':owned.most_common(50),'all_chains':symbol_chains.most_common(30)}
    Path(sys.argv[5]).write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps({k:v for k,v in result.items() if k not in ('project_chains','all_chains','project_inclusive','capture_first','client_first')},ensure_ascii=False))
finally:
    d.SymCleanup(p);k.CloseHandle(h);k.CloseHandle(p)
