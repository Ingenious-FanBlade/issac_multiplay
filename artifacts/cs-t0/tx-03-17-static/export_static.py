"""tx-03-17 read-only IDA export, invoked with py_exec_file (shared namespace).

runpy.run_path(init_globals=...) supports TX0317_OUTPUT, TX0317_FUNCTIONS,
TX0317_RANGES, TX0317_TABLES, TX0317_STRING_TERMS. Always create-new output.
TX0317_XREFS adds raw static data bytes and current IDA xrefs for selected globals.
TX0317_SYMBOL_TERMS selects a name inventory with xrefs. Table entries assume
pointer words: classify mixed ID/handler tables separately and do not use an ID
word's mechanical negative subtraction as an RVA.
No target execution, IDB mutation, identity comparison or hashes.
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
output = Path(globals().get('TX0317_OUTPUT', str(Path(__file__).with_name('primary-owner-initial.json'))))
addresses = globals().get('TX0317_FUNCTIONS', [0x8bb720,0x4074c0,0x40bd50,0x40db90,0x4081d0,0x408310,0x407ae0])
ranges_requested = globals().get('TX0317_RANGES', [])
tables_requested = globals().get('TX0317_TABLES', [])
string_terms = globals().get('TX0317_STRING_TERMS', [])
xref_addresses = globals().get('TX0317_XREFS', [])
symbol_terms = globals().get('TX0317_SYMBOL_TERMS', [])

def insn(ea):
    raw = ida_bytes.get_bytes(ea, idc.get_item_size(ea))
    return {'va':hex(ea), 'rva':hex(ea-base), 'bytes':raw.hex(' ') if raw else None,
            'asm':ida_lines.tag_remove(idc.generate_disasm_line(ea,0) or ''),
            'code_refs':[hex(x) for x in idautils.CodeRefsFrom(ea,False)]}

functions=[]
for address in addresses:
    fn=ida_funcs.get_func(address)
    item={'requested_va':hex(address),'name':ida_funcs.get_func_name(address)}
    if fn:
        item.update({'va':hex(fn.start_ea),'rva':hex(fn.start_ea-base),'end_va':hex(fn.end_ea),
            'type':idc.get_type(fn.start_ea),
            'chunks':[[hex(a),hex(b)] for a,b in idautils.Chunks(fn.start_ea)],
            'instructions':[insn(ea) for ea in idautils.FuncItems(fn.start_ea) if ida_bytes.is_code(ida_bytes.get_flags(ea))],
            'xrefs_to':[{'va':hex(x.frm),'type':x.type,'iscode':bool(x.iscode),'function':ida_funcs.get_func_name(x.frm)} for x in idautils.XrefsTo(fn.start_ea)]})
        try: item['pseudocode']=str(ida_hexrays.decompile(fn.start_ea))
        except Exception as exc: item['decompile_error']=str(exc)
    else: item['error']='No IDA function at requested address'
    functions.append(item)

strings=[]
if string_terms:
    for value in idautils.Strings():
        line=str(value)
        if any(term.lower() in line.lower() for term in string_terms):
            strings.append({'va':hex(value.ea),'text':line,
                'xrefs':[{'va':hex(x.frm),'function':ida_funcs.get_func_name(x.frm)} for x in idautils.XrefsTo(value.ea)]})
tables=[{'va':hex(address),'rva':hex(address-base),'name':idc.get_name(address),
    'entries':[{'offset':hex(i*4),'bytes':ida_bytes.get_bytes(address+i*4,4).hex(' '),
        'target_va':hex(ida_bytes.get_dword(address+i*4)),
        'target_rva':hex(ida_bytes.get_dword(address+i*4)-base),
        'name':idc.get_name(ida_bytes.get_dword(address+i*4))} for i in range(count)]} for address,count in tables_requested]
ranges=[{'start_va':hex(a),'end_va':hex(b),
    'instructions':[insn(ea) for ea in idautils.Heads(a,b) if ida_bytes.is_code(ida_bytes.get_flags(ea))]} for a,b in ranges_requested]
xref_records=[{'va':hex(ea),'rva':hex(ea-base),'name':idc.get_name(ea),
    'static_bytes_16':(ida_bytes.get_bytes(ea,16) or b'').hex(' '),
    'xrefs_to':[{'va':hex(x.frm),'type':x.type,'iscode':bool(x.iscode),
        'asm':ida_lines.tag_remove(idc.generate_disasm_line(x.frm,0) or ''),
        'function':ida_funcs.get_func_name(x.frm)} for x in idautils.XrefsTo(ea)]} for ea in xref_addresses]
symbols=[]
if symbol_terms:
    for ea,name in idautils.Names():
        if any(term.lower() in name.lower() for term in symbol_terms):
            symbols.append({'va':hex(ea),'rva':hex(ea-base),'name':name,
                'xrefs_to':[{'va':hex(x.frm),'type':x.type,'function':ida_funcs.get_func_name(x.frm),'asm':ida_lines.tag_remove(idc.generate_disasm_line(x.frm,0) or '')} for x in idautils.XrefsTo(ea)]})
payload={'tx':'tx-03-17','evidence':'STATIC_IDB_READ_ONLY','idb_path':idc.get_idb_path(),
    'input_path':ida_nalt.get_input_file_path(),'imagebase':hex(base),
    'functions':functions,'tables':tables,'ranges':ranges,'string_terms':string_terms,'strings':strings,'xref_records':xref_records,'symbol_terms':symbol_terms,'symbols':symbols,
    'limitations':['No runtime observations','IDB and disk file identity not asserted',
        'Decompiler types are not a complete ABI','IDA xrefs do not enumerate all dynamic targets']}
with output.open('x',encoding='utf-8') as stream:
    json.dump(payload,stream,ensure_ascii=False,indent=2)
print(json.dumps({'output':str(output),'functions':len(functions),'strings':len(strings),'tables':len(tables)}))
