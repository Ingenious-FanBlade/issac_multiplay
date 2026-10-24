"""tx-03-13 read-only IDA export for the ImageBase reference control path.

Run with IDA MCP py_exec_file. Optional TX0313_OUTPUT selects a new output path.
This script reads the active IDB and creates one JSON evidence file only.
It does not modify the IDB, target process, or source files.
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

BASE = idaapi.get_imagebase()
OUT = Path(globals().get('TX0313_OUTPUT', str(Path(__file__).with_name('luna-control-core.json'))))
FUNCTIONS = globals().get('TX0313_FUNCTIONS', [
    0x407A60, 0x408640,
    0x40C3B0, 0x40C550, 0x40C5C0, 0x40C610,
    0x40C630, 0x40C690, 0x40C6B0, 0x40C7F0, 0x40CC10, 0x40CB50,
    0xA156E0, 0xA15770, 0xA157C0, 0xA157F0, 0xA159A0,
    0xA0F4C0, 0xAEF15C,
])
TABLES = globals().get('TX0313_TABLES', [
    {'name': 'ReferenceCount<KAGE::Graphics::ImageBase>::vftable', 'va': 0xB1A6E0, 'count': 6},
    {'name': 'ReferenceCountBase::vftable', 'va': 0xB1A6FC, 'count': 6},
    {'name': 'KAGE::Mutex::vftable', 'va': 0xB81C0C, 'count': 5},
])

def fmt_insn(ea):
    size = idc.get_item_size(ea)
    raw = ida_bytes.get_bytes(ea, size) if size else None
    return {
        'va': hex(ea), 'rva': hex(ea - BASE),
        'bytes': raw.hex(' ') if raw else None,
        'asm': ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or ''),
        'code_refs': [hex(target) for target in idautils.CodeRefsFrom(ea, False)],
    }

functions = []
for requested in FUNCTIONS:
    fn = ida_funcs.get_func(requested)
    item = {'requested_va': hex(requested), 'name': ida_funcs.get_func_name(requested)}
    if fn:
        item.update({
            'va': hex(fn.start_ea), 'rva': hex(fn.start_ea - BASE),
            'end_va': hex(fn.end_ea), 'type': idc.get_type(fn.start_ea),
            'chunks': [[hex(a), hex(b)] for a, b in idautils.Chunks(fn.start_ea)],
            'instructions': [fmt_insn(ea) for ea in idautils.FuncItems(fn.start_ea)
                             if ida_bytes.is_code(ida_bytes.get_flags(ea))],
            'xrefs_to': [{'va': hex(x.frm), 'type': x.type, 'iscode': bool(x.iscode),
                          'function': ida_funcs.get_func_name(x.frm)}
                         for x in idautils.XrefsTo(fn.start_ea)],
        })
        try:
            item['pseudocode'] = str(ida_hexrays.decompile(fn.start_ea))
        except Exception as exc:
            item['decompile_error'] = str(exc)
    else:
        item['error'] = 'No IDA function at requested address'
    functions.append(item)

tables = []
for table in TABLES:
    address = table['va']
    count = table['count']
    raw = ida_bytes.get_bytes(address, count * 4)
    entries = []
    for index in range(count):
        target = ida_bytes.get_dword(address + index * 4)
        entries.append({
            'offset': hex(index * 4), 'entry_va': hex(address + index * 4),
            'bytes': ida_bytes.get_bytes(address + index * 4, 4).hex(' '),
            'target_va': hex(target), 'target_rva': hex(target - BASE) if target >= BASE else None,
            'target_name': idc.get_name(target),
        })
    tables.append({
        'name': table['name'], 'va': hex(address), 'rva': hex(address - BASE),
        'raw_bytes': raw.hex(' ') if raw else None, 'entries': entries,
    })

payload = {
    'tx': 'tx-03-13', 'evidence': 'STATIC_IDB_READ_ONLY',
    'idb_path': idc.get_idb_path(), 'input_path': ida_nalt.get_input_file_path(),
    'imagebase': hex(BASE), 'functions': functions, 'tables': tables,
    'limitations': [
        'No runtime observations or target execution',
        'IDB and disk input identity not asserted; no hashes used',
        'Dynamic ImageBase vtable+0x34 target and C7163C callback target remain runtime-dependent',
        'Decompiler prototypes are not treated as complete ABI evidence',
        'IDA xrefs do not enumerate all dynamic call targets',
    ],
}
with OUT.open('x', encoding='utf-8') as stream:
    json.dump(payload, stream, ensure_ascii=False, indent=2)
print(json.dumps({'output': str(OUT), 'functions': len(functions),
                  'tables': len(tables), 'imagebase': hex(BASE)}))
