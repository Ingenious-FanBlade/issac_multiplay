"""tx-03-14 Luna read-only export for renderer/window/context shutdown chain."""
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
    r"F:\projects_isaac\issac_multiplay\artifacts\cs-t0\tx-03-14-static\luna-window-close-chain.json",
))
if OUTPUT.exists():
    raise RuntimeError("Refusing to overwrite existing output: %s" % OUTPUT)
ADDRESSES = [0xA6A420, 0xA9DB50, 0xA81050, 0xA6AF90, 0xA5D4E0, 0xA7FA50, 0xA7FCB0, 0xA69C00, 0xA18E10]
GLOBAL_ADDRESSES = [0xC73680, 0xC73694, 0xC7370C, 0xC7163C]

def insn(ea):
    size = idc.get_item_size(ea)
    raw = ida_bytes.get_bytes(ea, size) if size else None
    return {"va": hex(ea), "rva": hex(ea - BASE), "bytes": raw.hex(" ") if raw else None,
            "asm": ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or ""),
            "code_refs": [hex(x) for x in idautils.CodeRefsFrom(ea, False)]}

functions = []
for address in ADDRESSES:
    fn = ida_funcs.get_func(address)
    item = {"requested_va": hex(address), "name": ida_funcs.get_func_name(address)}
    if fn:
        item.update({"va": hex(fn.start_ea), "rva": hex(fn.start_ea - BASE), "end_va": hex(fn.end_ea),
                     "size": hex(fn.end_ea - fn.start_ea),
                     "instructions": [insn(ea) for ea in idautils.FuncItems(fn.start_ea)
                                      if ida_bytes.is_code(ida_bytes.get_flags(ea))],
                     "xrefs_to": [{"va": hex(x.frm), "rva": hex(x.frm - BASE), "type": x.type,
                                   "iscode": bool(x.iscode), "function": ida_funcs.get_func_name(x.frm)}
                                  for x in idautils.XrefsTo(fn.start_ea)]})
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
    globals_out.append({"va": hex(ea), "rva": hex(ea - BASE), "name": idc.get_name(ea),
                        "static_bytes_16": raw.hex(" "),
                        "xrefs_to": [{"va": hex(x.frm), "rva": hex(x.frm - BASE), "type": x.type,
                                      "iscode": bool(x.iscode), "function": ida_funcs.get_func_name(x.frm),
                                      "asm": ida_lines.tag_remove(idc.generate_disasm_line(x.frm, 0) or "")}
                                     for x in idautils.XrefsTo(ea)]})

payload = {"tx": "tx-03-14", "author": "GPT-6 Luna", "evidence": "STATIC_IDB_READ_ONLY",
           "idb_path": idc.get_idb_path(), "input_path": ida_nalt.get_input_file_path(),
           "imagebase": hex(BASE), "functions": functions, "globals": globals_out,
           "limitations": ["No runtime/thread observations", "No EH/OOM/concurrency closure",
                           "Decompiler types are not a complete ABI", "IDA xrefs omit dynamic targets"]}
with OUTPUT.open("x", encoding="utf-8") as stream:
    json.dump(payload, stream, ensure_ascii=False, indent=2)
print(json.dumps({"output": str(OUTPUT), "functions": len(functions), "globals": len(globals_out)}))
