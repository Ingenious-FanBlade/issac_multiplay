"""tx-03-19 Luna read-only IDA export; invoke with py_eval/runpy.run_path.

runpy.run_path(init_globals=...) supports TX0319_OUTPUT, TX0319_FUNCTIONS,
TX0319_RANGES, TX0319_XREFS, and TX0319_SYMBOL_TERMS. Output is create-new.
This exports current IDB metadata, exact function instructions/bytes, selected
data bytes and xrefs. It does not execute the target, mutate the IDB, compare
disk identity, or calculate/check hashes. No virtual table is inferred here.
"""
import json
from pathlib import Path
import idaapi
import ida_nalt
import ida_funcs
import ida_hexrays
import ida_bytes
import ida_lines
import idautils
import idc

base = idaapi.get_imagebase()
output = Path(globals().get(
    'TX0319_OUTPUT',
    str(Path(__file__).with_name('luna-render-window-guard.json'))))
addresses = globals().get('TX0319_FUNCTIONS', [
    0x931050, 0x9aa040, 0x9555c0, 0x8fb120,
    0xa18e90, 0xa19df0, 0xa6a420, 0xa6bbb0, 0xa6b570,
    0xa7fa50, 0xa7fce0, 0xa80010,
])
ranges_requested = globals().get('TX0319_RANGES', [
    (0x9312ec, 0x9312f6),
    (0x9aaae3, 0x9aaae8),
    (0x8fc32c, 0x8fc331),
    (0x9560c6, 0x9560d5),
    (0xa8fb7a, 0xa8fb85),
])
xref_addresses = globals().get('TX0319_XREFS', [
    0xc798e0, 0xc7999c, 0xc74b80,
    0xc75d50, 0xc75d54, 0xc75d58,
    0xc13510, 0xc135e0,
])
symbol_terms = globals().get('TX0319_SYMBOL_TERMS', [
    'wglGetCurrentDC', 'wglGetCurrentContext', 'wglMakeCurrent',
    'epoxy_wglGetCurrentDC', 'epoxy_wglGetCurrentContext',
    'epoxy_wglMakeCurrent',
])

def insn(ea):
    raw = ida_bytes.get_bytes(ea, idc.get_item_size(ea))
    return {
        'va': hex(ea),
        'rva': hex(ea - base),
        'bytes': raw.hex(' ') if raw else None,
        'asm': ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or ''),
        'code_refs': [hex(x) for x in idautils.CodeRefsFrom(ea, False)],
    }

functions = []
for address in addresses:
    fn = ida_funcs.get_func(address)
    item = {'requested_va': hex(address),
            'requested_rva': hex(address - base),
            'name': ida_funcs.get_func_name(address)}
    if fn:
        item.update({
            'va': hex(fn.start_ea),
            'rva': hex(fn.start_ea - base),
            'end_va': hex(fn.end_ea),
            'size_bytes': fn.end_ea - fn.start_ea,
            'type': idc.get_type(fn.start_ea),
            'chunks': [[hex(a), hex(b)] for a, b in idautils.Chunks(fn.start_ea)],
            'instructions': [
                insn(ea) for ea in idautils.FuncItems(fn.start_ea)
                if ida_bytes.is_code(ida_bytes.get_flags(ea))
            ],
            'xrefs_to': [{
                'va': hex(x.frm), 'type': x.type, 'iscode': bool(x.iscode),
                'function': ida_funcs.get_func_name(x.frm),
            } for x in idautils.XrefsTo(fn.start_ea)],
        })
        try:
            item['pseudocode'] = str(ida_hexrays.decompile(fn.start_ea))
        except Exception as exc:
            item['decompile_error'] = str(exc)
    else:
        item['error'] = 'No IDA function at requested address'
    functions.append(item)

ranges = [{
    'start_va': hex(a), 'start_rva': hex(a - base),
    'end_va': hex(b), 'instructions': [
        insn(ea) for ea in idautils.Heads(a, b)
        if ida_bytes.is_code(ida_bytes.get_flags(ea))
    ],
} for a, b in ranges_requested]

xref_records = [{
    'va': hex(ea), 'rva': hex(ea - base), 'name': idc.get_name(ea),
    'static_bytes_16': (ida_bytes.get_bytes(ea, 16) or b'').hex(' '),
    'xrefs_to': [{
        'va': hex(x.frm), 'type': x.type, 'iscode': bool(x.iscode),
        'asm': ida_lines.tag_remove(idc.generate_disasm_line(x.frm, 0) or ''),
        'function': ida_funcs.get_func_name(x.frm),
    } for x in idautils.XrefsTo(ea)],
} for ea in xref_addresses]

symbols = []
if symbol_terms:
    for ea, name in idautils.Names():
        if any(term.lower() in name.lower() for term in symbol_terms):
            symbols.append({
                'va': hex(ea), 'rva': hex(ea - base), 'name': name,
                'bytes_16': (ida_bytes.get_bytes(ea, 16) or b'').hex(' '),
                'xrefs_to': [{
                    'va': hex(x.frm), 'type': x.type,
                    'function': ida_funcs.get_func_name(x.frm),
                    'asm': ida_lines.tag_remove(idc.generate_disasm_line(x.frm, 0) or ''),
                } for x in idautils.XrefsTo(ea)],
            })

payload = {
    'tx': 'tx-03-19',
    'evidence': 'STATIC_IDB_READ_ONLY',
    'idb_path': idc.get_idb_path(),
    'input_path': ida_nalt.get_input_file_path(),
    'imagebase': hex(base),
    'functions': functions,
    'ranges': ranges,
    'xref_records': xref_records,
    'symbol_terms': symbol_terms,
    'symbols': symbols,
    'tables': [],
    'counts': {
        'function_seed_count': len(addresses),
        'unique_resolved_function_count': len({item['va'] for item in functions if item.get('va')}),
        'unresolved_function_seed_count': sum(1 for item in functions if not item.get('va')),
        'table_count': 0,
    },
    'limitations': [
        'No runtime thread ID, TLS value, OS current HGLRC/HDC, or API outcome observed',
        'IDB metadata is not a disk identity assertion',
        'Decompiler prototypes do not establish a complete runtime ABI',
        'Static xrefs do not enumerate all dynamic call paths',
        'No virtual table or vtable slot count is asserted',
    ],
}
with output.open('x', encoding='utf-8') as stream:
    json.dump(payload, stream, ensure_ascii=False, indent=2)
summary = {
    'tx': payload['tx'], 'output': str(output), 'idb_path': payload['idb_path'],
    'input_path': payload['input_path'], 'imagebase': payload['imagebase'],
    'function_seed_count': len(functions),
    'unique_resolved_function_count': payload['counts']['unique_resolved_function_count'],
    'unresolved_function_seed_count': payload['counts']['unresolved_function_seed_count'],
    'range_count': len(ranges),
    'xref_record_count': len(xref_records), 'symbol_count': len(symbols),
    'table_count': len(payload['tables']),
}
