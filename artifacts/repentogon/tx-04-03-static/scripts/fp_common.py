# Shared IDAPython helpers for tx-04-03 point relocation (static analysis only).
import json, idaapi, idautils, idc, ida_bytes, ida_funcs, ida_ua, ida_fixup, ida_nalt, ida_name, ida_segment, ida_xref

SP = r"C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad"
BASE = ida_nalt.get_imagebase()

def va(rva): return BASE + rva
def rva(ea): return ea - BASE
def hx(x): return None if x is None else "0x%X" % x

def func_start(ea):
    f = ida_funcs.get_func(ea)
    return f.start_ea if f else None

def func_items(fs):
    f = ida_funcs.get_func(fs)
    return list(idautils.FuncItems(fs)) if f else []

def call_target(ea):
    insn = ida_ua.insn_t()
    if ida_ua.decode_insn(insn, ea) == 0: return None
    if insn.itype in (idaapi.NN_call, idaapi.NN_callfi, idaapi.NN_callni):
        op = insn.ops[0]
        if op.type in (ida_ua.o_near, ida_ua.o_far): return op.addr
    return None

def is_call(ea):
    insn = ida_ua.insn_t()
    if ida_ua.decode_insn(insn, ea) == 0: return False
    return insn.itype in (idaapi.NN_call, idaapi.NN_callfi, idaapi.NN_callni)

def masked_bytes(ea, n):
    """Bytes of [ea, ea+n) with fixup-covered bytes and near branch rel32 wildcarded (None)."""
    out = []
    raw = ida_bytes.get_bytes(ea, n) or b""
    mask = [True] * len(raw)
    # fixups (absolute relocations)
    for i in range(len(raw)):
        fd = ida_fixup.fixup_data_t()
        if ida_fixup.get_fixup(fd, ea + i):
            for k in range(4):
                if i + k < len(mask): mask[i + k] = False
    # rel32 of near call/jmp/jcc
    p = ea
    while p < ea + n:
        insn = ida_ua.insn_t()
        l = ida_ua.decode_insn(insn, p)
        if l <= 0: break
        op = insn.ops[0]
        if op.type == ida_ua.o_near and l >= 5:
            for k in range(l - 4, l):
                if p - ea + k < len(mask): mask[p - ea + k] = False
        p += l
    for i, b in enumerate(raw):
        out.append(b if mask[i] else None)
    return out

def pattern_str(mb):
    return " ".join("??" if b is None else "%02X" % b for b in mb)

def mnem_seq(fs, limit=4000):
    seq = []
    for ea in func_items(fs)[:limit]:
        seq.append(idc.print_insn_mnem(ea))
    return seq

def disp_set(fs):
    """Displacements of memory operands [reg+disp] used in the function (field offsets)."""
    s = {}
    for ea in func_items(fs):
        insn = ida_ua.insn_t()
        if ida_ua.decode_insn(insn, ea) == 0: continue
        for op in insn.ops:
            if op.type == ida_ua.o_void: break
            if op.type == ida_ua.o_displ and 0x10 <= op.addr < 0x100000:
                s.setdefault(op.addr, []).append(rva(ea))
    return s

def strings_in(fs):
    res = []
    for ea in func_items(fs):
        for x in idautils.DataRefsFrom(ea):
            t = idc.get_str_type(x)
            if t is not None and t >= 0:
                s = idc.get_strlit_contents(x, -1, t)
                if s: res.append(s.decode("latin1")[:80])
    return res

def callees_ordered(fs):
    res = []
    for ea in func_items(fs):
        t = call_target(ea)
        if t is not None: res.append((rva(ea), rva(t)))
    return res

def callers(target):
    res = []
    for x in idautils.XrefsTo(target, 0):
        if is_call(x.frm):
            res.append((rva(func_start(x.frm)) if func_start(x.frm) else None, rva(x.frm)))
    return res

def rtti_of_vtable(vt):
    col = ida_bytes.get_dword(vt - 4)
    td = ida_bytes.get_dword(col + 12)
    name = idc.get_strlit_contents(td + 8, -1, 0)
    return {"col_slot": hx(rva(vt - 4)), "col": hx(rva(col)), "td": hx(rva(td)), "name": name.decode("latin1") if name else None}

def func_fp(fs, nbytes=64):
    f = ida_funcs.get_func(fs)
    if not f: return {"error": "nofunc"}
    return {
        "start": hx(rva(f.start_ea)), "size": f.end_ea - f.start_ea,
        "n_insn": len(func_items(fs)),
        "pattern": pattern_str(masked_bytes(f.start_ea, min(nbytes, f.end_ea - f.start_ea))),
        "callees": [(hx(a), hx(b)) for a, b in callees_ordered(fs)],
        "callers": [(hx(a), hx(b)) for a, b in callers(f.start_ea)],
        "strings": strings_in(fs)[:20],
        "mnem": mnem_seq(fs),
        "offs": [ea - f.start_ea for ea in func_items(fs)],
        "disp": {hx(k): [hx(x) for x in v[:6]] for k, v in disp_set(fs).items()},
    }
