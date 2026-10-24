# Read-only IDAPython export for tx-01-71. This script does not alter the IDB.
import os
import json
import idaapi
import ida_bytes
import ida_funcs
import ida_hexrays
import ida_lines
import ida_name
import idautils
import idc

OUT = r'D:\projects\isaac_hook_reserach\artifacts\cs-t0\tx-01-71-static\agent-native-evidence.json'
BASE = idaapi.get_imagebase()
TARGETS = [
    0x6EEF60, 0x6EEF20, 0x916770, 0x904E70, 0x9032D0, 0x903670,
    0x8FE3A0, 0x8FF600, 0x8FD900, 0x8FFC70, 0x90D950,
    0x90D9B0, 0x90DF40, 0x90CA40, 0x9015B0, 0x901B70,
    0x9091A0, 0x90B070, 0x90ABF0, 0x90A8A0,
    0x6C0F30, 0x6E30A0, 0x9B9CD0, 0x9BEE80,
    0x959720, 0x959D00, 0x954CD0, 0x959670,
]
DATA_TARGETS = [0xC79864, 0xC7AC68, 0xC7169C, 0xC716A0]

def clean(s):
    if s is None:
        return None
    return ida_lines.tag_remove(str(s))

def get_insn(ea):
    size = idc.get_item_size(ea)
    raw = ida_bytes.get_bytes(ea, size) if size and size > 0 else None
    return {
        'va': hex(ea),
        'rva': hex(ea - BASE),
        'size': size,
        'bytes': raw.hex(' ') if raw else None,
        'disasm': clean(idc.generate_disasm_line(ea, 0)),
    }

def collect_func(addr):
    f = ida_funcs.get_func(addr)
    if not f:
        return {'requested_va': hex(addr), 'error': 'no function at requested address'}
    start, end = f.start_ea, f.end_ea
    code = []
    calls = []
    refs = []
    for ea in idautils.Heads(start, end):
        flags = ida_bytes.get_full_flags(ea)
        if not ida_bytes.is_code(flags):
            continue
        ins = get_insn(ea)
        code.append(ins)
        mnem = (idc.print_insn_mnem(ea) or '').lower()
        if mnem.startswith('call'):
            for dst in idautils.CodeRefsFrom(ea, False):
                df = ida_funcs.get_func(dst)
                calls.append({'site': hex(ea), 'target': hex(dst), 'name': ida_name.get_name(dst),
                              'target_func': hex(df.start_ea) if df else None})
        for dst in idautils.DataRefsFrom(ea):
            text = idc.get_strlit_contents(dst, -1, idc.STRTYPE_C)
            if text:
                try:
                    value = text.decode('utf-8', 'replace')
                except Exception:
                    value = repr(text)
                refs.append({'site': hex(ea), 'target': hex(dst), 'string': value})
    callers = []
    seen_callers = set()
    for xr in idautils.CodeRefsTo(start, False):
        cf = ida_funcs.get_func(xr)
        if cf:
            key = (cf.start_ea, xr)
            if key not in seen_callers:
                seen_callers.add(key)
                callers.append({'site': hex(xr), 'function': hex(cf.start_ea), 'name': ida_name.get_name(cf.start_ea)})
    try:
        cfunc = ida_hexrays.decompile(start)
        pseudocode = clean(str(cfunc)) if cfunc else None
        proto = clean(str(cfunc.type)) if cfunc else None
    except Exception as ex:
        pseudocode = None
        proto = None
        decompile_error = repr(ex)
    else:
        decompile_error = None
    return {
        'requested_va': hex(addr), 'name': ida_name.get_name(start),
        'start_va': hex(start), 'end_va': hex(end), 'start_rva': hex(start - BASE),
        'size': end - start, 'prototype': proto,
        'pseudocode': pseudocode, 'decompile_error': decompile_error,
        'instructions': code, 'call_sites': calls, 'string_refs': refs,
        'caller_sites': callers,
    }

result = {
    'evidence_type': 'static_ida_read_only_export',
    'module': ida_nalt.get_root_filename() if 'ida_nalt' in globals() else idc.get_root_filename(),
    'input_path': idc.get_input_file_path(),
    'imagebase': hex(BASE),
    'functions': [collect_func(x) for x in TARGETS],
    'data_xrefs': {},
}
for addr in DATA_TARGETS:
    items = []
    for xr in idautils.XrefsTo(addr, 0):
        items.append({'site_va': hex(xr.frm), 'site_rva': hex(xr.frm - BASE),
                      'type': int(xr.type), 'function': hex(ida_funcs.get_func(xr.frm).start_ea) if ida_funcs.get_func(xr.frm) else None,
                      'instruction': get_insn(xr.frm) if ida_bytes.is_code(ida_bytes.get_full_flags(xr.frm)) else None})
    result['data_xrefs'][hex(addr)] = items
if os.path.exists(OUT):
    raise RuntimeError('refusing to overwrite existing evidence file: ' + OUT)
with open(OUT, 'x', encoding='utf-8') as stream:
    json.dump(result, stream, ensure_ascii=False, indent=2)
print('wrote read-only static evidence: %s; functions=%d; imagebase=%s' % (OUT, len(result['functions']), hex(BASE)))
