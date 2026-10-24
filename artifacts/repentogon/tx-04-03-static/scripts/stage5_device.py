exec(open(r"C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\match_lib.py").read())
VT = json.load(open(SP + r"\v12_vtables_all.json"))
def an(r): return ANCH_BY_RVA.get(r, [""])[0][:60]
def fsize(r):
    f = ida_funcs.get_func(va(r)); return (f.end_ea - f.start_ea) if f and f.start_ea == va(r) else None
def retn(r):
    f = ida_funcs.get_func(va(r))
    if not f: return None
    last = idc.prev_head(f.end_ea)
    return idc.generate_disasm_line(last, 0)
out = {}
base = VT[".?AVInputDeviceBase@Input@KAGE@@"][0]["slots"]
kb = VT[".?AVDeviceKeyboard@Input@KAGE@@"][0]["slots"]
gp = VT[".?AVDeviceGamepad@Input@KAGE@@"][0]["slots"]
rows = []
for i in range(39):
    b, k, g = int(base[i], 16), int(kb[i], 16), int(gp[i], 16)
    rows.append((i, hx(b), fsize(b), retn(b), an(b), hx(k), an(k), hx(g), an(g)))
for r in rows: print(r)
# constructors: code that stores the base vtable address
vt = va(0x732F04)
ctors = sorted(set(hx(rva(func_start(x.frm))) for x in idautils.XrefsTo(vt, 0) if func_start(x.frm)))
print("base-vtable writers", ctors, [an(int(c, 16)) for c in ctors])
for name, r in (("kb", 0x73B490), ("gp", 0x73B3F0)):
    w = sorted(set(hx(rva(func_start(x.frm))) for x in idautils.XrefsTo(va(r), 0) if func_start(x.frm)))
    print(name, "vtable writers", w, [an(int(c, 16)) for c in w])
