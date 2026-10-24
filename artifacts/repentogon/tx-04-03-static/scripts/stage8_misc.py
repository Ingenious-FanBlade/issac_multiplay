exec(open(r"C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\match_lib.py").read())
import ida_hexrays
def dec(ea, n=40):
    try: return "\n".join(str(ida_hexrays.decompile(ea)).splitlines()[:n])
    except Exception as e: return "ERR " + str(e)
def callers_of(r):
    return sorted(set((hx(rva(func_start(x.frm))) if func_start(x.frm) else None, hx(rva(x.frm))) for x in idautils.XrefsTo(va(r), 0)))
print("IsPaused 0x2F92A0 callers:", callers_of(0x2F92A0))
print("pred 0x287020 callers:", callers_of(0x287020))
print(dec(va(0x287020), 30))
print("==== Game::Update head"); print(dec(va(0x2F6D30), 22))
# Q10: ExecuteCommand Lua binding
sd = ida_segment.get_segm_by_name(".rdata")
buf = ida_bytes.get_bytes(sd.start_ea, sd.end_ea - sd.start_ea)
for mm in re.finditer(rb"ExecuteCommand\x00", buf):
    s = sd.start_ea + mm.start()
    print("string ExecuteCommand at", hx(rva(s)), "xrefs", [(hx(rva(x.frm)), hx(rva(func_start(x.frm))) if func_start(x.frm) else None) for x in idautils.XrefsTo(s, 0)])
    for x in idautils.XrefsTo(s, 0):
        # look for a code pointer pushed near the string reference (lua_pushcclosure / luabridge addFunction pattern)
        p = x.frm
        for i in range(8):
            p = idc.next_head(p, p + 32)
            print("   ", hx(rva(p)), idc.generate_disasm_line(p, 0))
print("RunCommand 0x288100 callers:", callers_of(0x288100))
