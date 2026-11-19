import ctypes as c, ctypes.wintypes as w, struct, sys, time, subprocess, json
k=c.WinDLL('kernel32',use_last_error=True)
class ER(c.Structure):
    _fields_=[('code',w.DWORD),('flags',w.DWORD),('record',c.c_void_p),('address',c.c_void_p),('count',w.DWORD),('info',c.c_size_t*15)]
class EX(c.Structure):
    _fields_=[('record',ER),('first',w.DWORD)]
class U(c.Union):
    _fields_=[('exception',EX),('raw',c.c_byte*160)]
class EV(c.Structure):
    _fields_=[('code',w.DWORD),('pid',w.DWORD),('tid',w.DWORD),('u',U)]
k.OpenProcess.restype=c.c_void_p;k.OpenThread.restype=c.c_void_p
k.ReadProcessMemory.argtypes=[c.c_void_p,c.c_void_p,c.c_void_p,c.c_size_t,c.c_void_p]
k.Wow64GetThreadContext.argtypes=[c.c_void_p,c.c_void_p]
k.CloseHandle.argtypes=[c.c_void_p]
k.DebugActiveProcess.argtypes=[w.DWORD]
pid=int(sys.argv[1]); assert k.DebugActiveProcess(pid),c.get_last_error()
k.DebugSetProcessKillOnExit(False)
hp=k.OpenProcess(0x410,False,pid)
print('attached',pid,flush=True)
try:
    deadline=time.monotonic()+80
    while time.monotonic()<deadline:
        ev=EV()
        if not k.WaitForDebugEvent(c.byref(ev),1000):continue
        status=0x10002
        if ev.code==1:
            er=ev.u.exception.record
            status=0x80010001 if er.code!=0x80000003 else 0x10002
            if er.code in (0xc0000005,0xc00000fd,0xc0000409,0xc0000374):
                print('FAULT',hex(er.code),hex(er.address or 0),'first',ev.u.exception.first,'info',list(er.info)[:er.count],flush=True)
                ht=k.OpenThread(0x48,False,ev.tid);ctx=c.create_string_buffer(716);struct.pack_into('<I',ctx,0,0x1003f)
                if k.Wow64GetThreadContext(ht,ctx):
                    regs=struct.unpack_from('<11I',ctx,156);print('REG edi esi ebx edx ecx eax ebp eip cs flags esp',*[hex(v) for v in regs],flush=True)
                    for label,addr in [('stack',regs[-1]),('ebp',regs[6])]:
                        buf=c.create_string_buffer(512);read=c.c_size_t()
                        k.ReadProcessMemory(hp,addr,buf,512,c.byref(read));print(label,hex(addr),buf.raw[:read.value].hex(),flush=True)
                k.CloseHandle(ht)
                k.ContinueDebugEvent(ev.pid,ev.tid,status)
                break
        if ev.code==5:
            print('EXIT',struct.unpack_from('<I',ev.u.raw)[0],flush=True)
            k.ContinueDebugEvent(ev.pid,ev.tid,status);break
        k.ContinueDebugEvent(ev.pid,ev.tid,status)
finally:
    k.DebugActiveProcessStop(pid);k.CloseHandle(hp)
