# Run inside the v1.9.7.17.J460 IDB. Exports fingerprints of every first-round prototype point.
exec(open(r"C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\fp_common.py").read())

LOOP_SITES = [0x531231,0x5312EC,0x555231,0x2FBBA5,0x559696,0x5AB84A,0x33F5B9,0x5312F1,0x554F7D,0x61FC97,
              0x554D5E,0x559B4D,0x559B82,0x2FADFA,0x2FAE09,0x2FBA4D,0x403E70,0x559B21,0x2F580B,0x2F61FF,
              0x2F72C9,0x2F7721,0x2F7A10,0x2FB4F0,0x52FC77,0x2FB51C,0x2FE37C]
COOP_SITES = [0x385BC2,0x385BC9,0x385BD2,0x385BD9,0x385BE0,0x385BE7,0x2FADA7,0x5BB415,0x555241,0x5BAE7E,0x5BB081,0x5BB288]
EXTRA_FUNCS = {"Pressed":0x61E8D0,"Triggered":0x61E970,"Scalar":0x61EA20,"KeyboardPoll":0x66C650,"GamepadPoll":0x622940,
               "PlayerUpdate":0x382AF0,"Allocator":0x60F4C0,"DeviceCtor":0x61E050,"ControlCtor":0xCC10,"DeviceInit":0x61E210,
               "Register":0x620DF0,"Remove":0x620EB0,"JoinExit":0x5BA000}
CODE_ADDRS = {"PhysicalExitReturn":0x61F30C,"PlayerOnlyReturn":0x2FD60C,"OddHelperReturn":0x555246,"CoopNoCreate":0x5BB561}
VTABLES = {"Keyboard":0x7A1678,"Gamepad":0x782C80,"Player":0x76BDD0,"InputManagerDerived":0x782950,"DeviceBaseTemplate":0x782758}
GLOBALS = {"GameRootSlot":0x871678,"OnlineManagerSlot":0x87169C,"LuaSlot":0x871690,"InputManagerObject":0x857B18,
           "DeviceVectorBegin":0x8799E8,"DeviceVectorEnd":0x8799EC,"KeyboardState":0x878C10}
IATS = {"PeekMessage":0x718714,"TranslateMessage":0x718650,"DispatchMessage":0x7185B8}

out = {"image": {"timestamp_note": "v1.9.7.17.J460 IDB"}, "sites": {}, "funcs": {}, "code": {}, "vtables": {}, "globals": {}, "iat": {}}
need_funcs = set()

def site_fp(site_rva):
    ea = va(site_rva)
    fs = func_start(ea)
    tgt = call_target(ea)
    calls = callees_ordered(fs) if fs else []
    idx_all = [i for i, (s, t) in enumerate(calls) if s == site_rva]
    same = [s for s, t in calls if tgt is not None and t == rva(tgt)]
    need_funcs.add(fs)
    if tgt: need_funcs.add(tgt)
    return {"site": hx(site_rva), "func": hx(rva(fs)) if fs else None, "off": hx(ea - fs) if fs else None,
            "target": hx(rva(tgt)) if tgt else None, "call_index": idx_all[0] if idx_all else None,
            "n_calls": len(calls), "same_target_index": same.index(site_rva) if site_rva in same else None,
            "same_target_count": len(same), "bytes": pattern_str(masked_bytes(ea, 5)),
            "context": pattern_str(masked_bytes(ea - 24, 40)), "insn": idc.generate_disasm_line(ea, 0)}

for s in LOOP_SITES: out["sites"]["loop:" + hx(s)] = site_fp(s)
for s in COOP_SITES: out["sites"]["coop:" + hx(s)] = site_fp(s)
for k, r in EXTRA_FUNCS.items(): need_funcs.add(va(r)); out["funcs"]["name:" + k] = hx(r)

for k, r in CODE_ADDRS.items():
    ea = va(r); fs = func_start(ea)
    prev = idc.prev_head(ea)
    need_funcs.add(fs)
    out["code"][k] = {"rva": hx(r), "func": hx(rva(fs)) if fs else None, "off": hx(ea - fs) if fs else None,
                      "prev_insn": idc.generate_disasm_line(prev, 0), "prev_call_target": hx(rva(call_target(prev))) if call_target(prev) else None,
                      "insn": idc.generate_disasm_line(ea, 0), "context": pattern_str(masked_bytes(ea - 16, 32)),
                      "xrefs_to": [hx(rva(x.frm)) for x in idautils.XrefsTo(ea, 0)][:10]}

for k, r in VTABLES.items():
    vt = va(r)
    slots = [hx(rva(ida_bytes.get_dword(vt + 4 * i))) for i in range(40)]
    out["vtables"][k] = {"rva": hx(r), "rtti": rtti_of_vtable(vt), "slots": slots}

def data_uses(ea, limit=12):
    res = []
    for x in list(idautils.XrefsTo(ea, 0))[:limit]:
        fs = func_start(x.frm)
        if fs: need_funcs.add(fs)
        res.append({"insn_rva": hx(rva(x.frm)), "func": hx(rva(fs)) if fs else None,
                    "off": hx(x.frm - fs) if fs else None, "insn": idc.generate_disasm_line(x.frm, 0)})
    return res

for k, r in GLOBALS.items():
    out["globals"][k] = {"rva": hx(r), "uses": data_uses(va(r)), "n_xrefs": len(list(idautils.XrefsTo(va(r), 0)))}
for k, r in IATS.items():
    out["iat"][k] = {"rva": hx(r), "name": ida_name.get_name(va(r))}

for fs in sorted(x for x in need_funcs if x):
    out["funcs"][hx(rva(fs))] = func_fp(fs)

json.dump(out, open(SP + r"\v17_fingerprints.json", "w"), indent=1)
print("sites", len(out["sites"]), "funcs", len(out["funcs"]))
