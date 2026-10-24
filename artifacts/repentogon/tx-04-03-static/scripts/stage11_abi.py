exec(open(r"C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\match_lib.py").read())
import ida_typeinf, ida_nalt
F = {"PostUpdateCaller": 0x446C90, "NewRoomCaller": 0x44B930, "ConsolePred": 0x287020, "RoomUpdate": 0x3F7F60,
     "EntityListUpdate": 0x13F00, "InputManagerUpdate": 0x5EDCC0, "DeviceCommonUpdate": 0x5EC900, "RoomLifecycleFSM": 0x509100,
     "DoorNestedOwner": 0x2FA0F0, "OddHelper": 0x2F9340, "JoinPass": 0x591360, "Move": 0x372540, "Bomb": 0x37A210,
     "CardPill": 0x39BDD0, "Shoot": 0x373990, "Drop": 0x39C300}
res = {}
for k, r in F.items():
    f = ida_funcs.get_func(va(r))
    rets = sorted(set(idc.generate_disasm_line(ea, 0) for ea in func_items(va(r)) if idc.print_insn_mnem(ea) in ("retn", "ret")))
    # does the function read ECX before writing it?
    ecx_this = None
    for ea in func_items(va(r))[:30]:
        l = idc.generate_disasm_line(ea, 0).split(";")[0]
        ops = l.split(None, 1)[1] if " " in l else ""
        if "ecx" in ops:
            dst = ops.split(",")[0]
            ecx_this = not (dst.strip() == "ecx" and idc.print_insn_mnem(ea) in ("mov", "lea", "xor", "pop"))
            break
    tif = ida_typeinf.tinfo_t()
    t = idc.get_type(va(r)) or (idc.guess_type(va(r)))
    res[k] = {"rva": hx(r), "size": f.end_ea - f.start_ea, "rets": rets, "reads_ecx_first": ecx_this, "ida_type": t,
              "callers": len([x for x in idautils.XrefsTo(va(r), 0) if is_call(x.frm)])}
dn = [(hx(rva(func_start(x.frm))), ANCH_BY_RVA.get(rva(func_start(x.frm)), [""])[0][:40]) for x in idautils.XrefsTo(va(0x2FA0F0), 0) if is_call(x.frm)]
res["DoorNestedOwner"]["caller_funcs"] = dn
json.dump(res, open(SP + r"\v12_abi.json", "w"), indent=1)
for k, v in res.items(): print(k, v)
