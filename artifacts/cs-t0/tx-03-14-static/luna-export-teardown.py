"""tx-03-14 Luna read-only teardown export. Execute with IDA py_exec_file.
Writes complete instruction records and pseudocode for a bounded set of lifecycle functions,
static globals/tables, xrefs, and imported names. No database mutation or target execution.
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
OUTPUT = Path(globals().get(
    "TX0314_LUNA_OUTPUT",
    r"F:\projects_isaac\issac_multiplay\artifacts\cs-t0\tx-03-14-static\luna-renderer-teardown.json",
))
if OUTPUT.exists():
    raise RuntimeError("Refusing to overwrite existing output: %s" % OUTPUT)
ADDRESSES = [
    0xA115F0, 0xA11620, 0xA11700, 0xA11730, 0xA11780,
    0xA13060, 0xA19130, 0xA71770, 0xA196B0, 0xA19DF0,
    0xA6A420, 0xA69F60,
]
GLOBAL_ADDRESSES = [0xC7163C, 0xC7367C, 0xC73748, 0xC73754, 0xC37980, 0xC37984, 0xC3798C, 0xC37990]
TABLE_RANGES = [(0xB818A0, 0xB818D0), (0xB825F0, 0xB82620)]

def insn(ea):
    size = idc.get_item_size(ea)
    raw = ida_bytes.get_bytes(ea, size) if size else None
    return {
        "va": hex(ea), "rva": hex(ea - BASE),
        "bytes": raw.hex(" ") if raw else None,
        "asm": ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or ""),
        "code_refs": [hex(x) for x in idautils.CodeRefsFrom(ea, False)],
    }

functions = []
for address in ADDRESSES:
    fn = ida_funcs.get_func(address)
    item = {"requested_va": hex(address), "name": ida_funcs.get_func_name(address)}
    if fn:
        item.update({
            "va": hex(fn.start_ea), "rva": hex(fn.start_ea - BASE), "end_va": hex(fn.end_ea),
            "size": hex(fn.end_ea - fn.start_ea),
            "chunks": [[hex(a), hex(b)] for a, b in idautils.Chunks(fn.start_ea)],
            "instructions": [insn(ea) for ea in idautils.FuncItems(fn.start_ea)
                             if ida_bytes.is_code(ida_bytes.get_flags(ea))],
            "xrefs_to": [{"va": hex(x.frm), "type": x.type, "iscode": bool(x.iscode),
                          "function": ida_funcs.get_func_name(x.frm)} for x in idautils.XrefsTo(fn.start_ea)],
        })
        try:
            item["pseudocode"] = str(ida_hexrays.decompile(fn.start_ea))
        except Exception as exc:
            item["decompile_error"] = str(exc)
    else:
        item["error"] = "No IDA function at requested address"
    functions.append(item)

globals_out = []
for ea in GLOBAL_ADDRESSES:
    raw = ida_bytes.get_bytes(ea, 16) or b""
    refs = [{"va": hex(x.frm), "rva": hex(x.frm - BASE), "type": x.type,
             "iscode": bool(x.iscode), "function": ida_funcs.get_func_name(x.frm),
             "asm": ida_lines.tag_remove(idc.generate_disasm_line(x.frm, 0) or "")}
            for x in idautils.XrefsTo(ea)]
    globals_out.append({"va": hex(ea), "rva": hex(ea - BASE), "name": idc.get_name(ea),
                        "static_bytes_16": raw.hex(" "), "xrefs_to": refs})

tables = []
for start, end in TABLE_RANGES:
    entries = []
    for ea in range(start, end, 4):
        value = ida_bytes.get_dword(ea)
        entries.append({"va": hex(ea), "rva": hex(ea - BASE),
                        "bytes": (ida_bytes.get_bytes(ea, 4) or b"").hex(" "),
                        "value": hex(value), "target_name": idc.get_name(value)})
    tables.append({"start_va": hex(start), "end_va": hex(end), "entries": entries})

imports = []
for i in range(ida_nalt.get_import_module_qty()):
    module = ida_nalt.get_import_module_name(i) or ""
    def import_cb(ea, name, ordinal, module_name=module):
        text = name or "ordinal_%s" % ordinal
        if any(term in text.lower() for term in ("glfw", "destroywindow", "terminate", "contextcurrent", "makecurrent")):
            imports.append({"va": hex(ea), "rva": hex(ea - BASE), "module": module_name, "name": text})
        return True
    ida_nalt.enum_import_names(i, import_cb)

payload = {
    "tx": "tx-03-14", "author": "GPT-6 Luna", "evidence": "STATIC_IDB_READ_ONLY",
    "idb_path": idc.get_idb_path(), "input_path": ida_nalt.get_input_file_path(),
    "imagebase": hex(BASE), "functions": functions, "globals": globals_out,
    "table_ranges": tables, "matching_imports": imports,
    "limitations": ["No runtime/thread observations", "No EH/OOM/concurrency closure",
                    "Decompiler types are not a complete ABI", "IDA xrefs omit dynamic targets"],
}
with OUTPUT.open("x", encoding="utf-8") as stream:
    json.dump(payload, stream, ensure_ascii=False, indent=2)
print(json.dumps({"output": str(OUTPUT), "functions": len(functions), "globals": len(globals_out),
                  "tables": len(tables), "matching_imports": len(imports)}))
