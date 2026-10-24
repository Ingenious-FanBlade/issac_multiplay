exec(open(r"C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\match_lib.py").read())
import ida_hexrays
out = {}
# A. return address after the Poll (vtable +0x8C) call inside common device update 0x5EC900
f = va(0x5EC900)
for ea in func_items(f):
    l = idc.generate_disasm_line(ea, 0)
    if l.startswith("call") and ("8Ch" in l or "eax" in l):
        out.setdefault("poll_calls", []).append((hx(rva(ea)), l, hx(rva(idc.next_head(ea, ea + 16))), idc.generate_disasm_line(idc.next_head(ea, ea + 16), 0)))
for ea in func_items(f)[:40]:
    out.setdefault("common_update_head", []).append((hx(rva(ea)), idc.generate_disasm_line(ea, 0)))
# B. call sites to 0x5EC900
out["common_update_callers"] = [(hx(rva(x.frm)), hx(rva(func_start(x.frm)))) for x in idautils.XrefsTo(f, 0) if is_call(x.frm)]
# C. operator new
for name in ("??2@YAPAXI@Z", "??2@YAPAXI@Z_0"):
    ea = idc.get_name_ea_simple(name)
    if ea != idaapi.BADADDR: out["operator_new"] = (name, hx(rva(ea)))
# v17 allocator fingerprint vs operator new
fp = V17["funcs"].get("0x60F4C0")
if fp and "operator_new" in out: out["alloc_sim"] = round(sim(fp["mnem"], va(int(out["operator_new"][1], 16))), 3)
out["v17_alloc_pattern"] = fp["pattern"][:80] if fp else None
out["opnew_bytes"] = (ida_bytes.get_bytes(va(int(out["operator_new"][1], 16)), 24) or b"").hex(" ") if "operator_new" in out else None
# F. Initialize: globals referenced
ini = va(0x5EC6A0)
refs = []
for ea in func_items(ini):
    for x in idautils.DataRefsFrom(ea):
        if ida_segment.getseg(x) and ida_segment.get_segm_name(ida_segment.getseg(x)) in (".data", ".rdata"):
            refs.append((hx(rva(ea)), hx(rva(x)), idc.generate_disasm_line(ea, 0)))
out["initialize_refs"] = refs
try:
    out["initialize_dec"] = str(ida_hexrays.decompile(ini)).splitlines()[:45]
except Exception as e:
    out["initialize_dec"] = str(e)
# G. keyboard state: refs from keyboard poll (slot35 0x633DD0) to data
refs = []
for ea in func_items(va(0x633DD0)):
    for x in idautils.DataRefsFrom(ea):
        refs.append((hx(rva(ea)), hx(rva(x)), idc.generate_disasm_line(ea, 0)))
out["kbpoll_refs"] = refs[:30]
json.dump(out, open(SP + r"\v12_input.json", "w"), indent=1)
for k, v in out.items():
    if k == "initialize_dec": print(k); print("\n".join(v) if isinstance(v, list) else v)
    else: print(k, v)
