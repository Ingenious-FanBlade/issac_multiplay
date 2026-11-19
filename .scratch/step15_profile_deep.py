"""临时诊断采样：短暂停住测试主线程读取寄存器及栈，立即恢复；不用于性能验收计量。"""
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
p=k.OpenProcess(0x10,False,pid);assert p
ctx=C.create_string_buffer(716);stack=C.create_string_buffer(512);count=collections.Counter();pcs=collections.Counter();callers=collections.Counter();chains=collections.Counter();n=0;deadline=time.monotonic()+seconds
try:
    while time.monotonic()<deadline:
        C.c_uint32.from_buffer(ctx).value=0x10001
        suspended=k.SuspendThread(h)!=0xffffffff
        try:
            if suspended and k.Wow64GetThreadContext(h,ctx):
                pc=C.c_uint32.from_buffer(ctx,184).value;sp=C.c_uint32.from_buffer(ctx,196).value;pcs[pc]+=1;n+=1
                owned=[]
                if base<=pc<base+extent:owned.append(pc-base)
                read=C.c_size_t()
                if k.ReadProcessMemory(p,sp,stack,len(stack),C.byref(read)):
                    owned += [x-base for x in struct.unpack('<128I',stack.raw) if base<=x<base+extent]
                if owned:
                    count[owned[0]]+=1
                    chains[tuple(hex(x) for x in owned[:8])]+=1
                    if 0x2c560<=owned[0]<0x2c680:callers[tuple(hex(x) for x in owned[1:4])]+=1
        finally:
            if suspended:k.ResumeThread(h)
        time.sleep(.002)
finally:
    k.CloseHandle(h);k.CloseHandle(p)
result={'pid':pid,'tid':tid,'samples':n,'first_dll_stack_candidates':[(hex(a),v) for a,v in count.most_common(35)],'pc':[(hex(a),v) for a,v in pcs.most_common(15)],'read_memory_callers':callers.most_common(20)}
result['chains']=chains.most_common(100)
print(json.dumps({k:v for k,v in result.items() if k!='chains'}));open(sys.argv[5],'w').write(json.dumps(result,indent=2))

