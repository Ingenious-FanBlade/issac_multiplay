"""只读解析本轮 DLL 的 PDB，不附加或暂停游戏。"""
import ctypes as C
from ctypes import wintypes as W
import sys
d=C.WinDLL('dbghelp',use_last_error=True)
k=C.WinDLL('kernel32',use_last_error=True)
k.GetCurrentProcess.restype=W.HANDLE
p=k.GetCurrentProcess()
d.SymInitializeW.argtypes=[W.HANDLE,W.LPCWSTR,W.BOOL]
d.SymSetOptions(0x10|0x2)
assert d.SymInitializeW(p,None,False)
d.SymLoadModuleExW.argtypes=[W.HANDLE,W.HANDLE,W.LPCWSTR,W.LPCWSTR,C.c_ulonglong,W.DWORD,C.c_void_p,W.DWORD]
d.SymLoadModuleExW.restype=C.c_ulonglong
base=d.SymLoadModuleExW(p,None,str(__import__('pathlib').Path('out/repentogon/e2e/zhlIsaacCS.dll').resolve()),None,0x10000000,0,None,0)
class Info(C.Structure):
    _fields_=[('SizeOfStruct',W.ULONG),('TypeIndex',W.ULONG),('Reserved',C.c_ulonglong*2),('Index',W.ULONG),('Size',W.ULONG),('ModBase',C.c_ulonglong),('Flags',W.ULONG),('Value',C.c_ulonglong),('Address',C.c_ulonglong),('Register',W.ULONG),('Scope',W.ULONG),('Tag',W.ULONG),('NameLen',W.ULONG),('MaxNameLen',W.ULONG),('Name',C.c_char*1)]
d.SymFromAddr.argtypes=[W.HANDLE,C.c_ulonglong,C.POINTER(C.c_ulonglong),C.POINTER(Info)]
for rva in sys.argv[1:]:
    buf=C.create_string_buffer(C.sizeof(Info)+2048); info=C.cast(buf,C.POINTER(Info));info.contents.SizeOfStruct=C.sizeof(Info);info.contents.MaxNameLen=2048
    offset=C.c_ulonglong()
    if d.SymFromAddr(p,base+int(rva,16),C.byref(offset),info):
        print(rva,C.string_at(C.addressof(buf)+Info.Name.offset).decode(),hex(offset.value))
    else: print(rva,'error',C.get_last_error())
