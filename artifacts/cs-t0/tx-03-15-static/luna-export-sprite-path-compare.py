# Read-only IDA export for tx-03-15: expand the path equality helper call.
# Execute with mcp__ida_mcp__py_exec_file; this only reads IDB structures.
import json
import os
from pathlib import Path

import idaapi
import ida_bytes
import ida_funcs
import ida_hexrays
import ida_lines
import ida_nalt
import idautils
import idc

BASE = idaapi.get_imagebase()
DEFAULT_OUTPUT = r"F:\projects_isaac\issac_multiplay\artifacts\cs-t0\tx-03-15-static\luna-sprite-path-compare.json"
OUTPUT = Path(os.environ.get("TX0315_LUNA_OUTPUT") or globals().get("TX0315_LUNA_OUTPUT") or DEFAULT_OUTPUT)


def hx(value):
    return hex(value) if value is not None else None


def insn(ea):
    size = idc.get_item_size(ea) or 1
    raw = ida_bytes.get_bytes(ea, size) or b""
    return {"va": hx(ea), "rva": hx(ea - BASE), "bytes": raw.hex(" "), "size": size,
            "assembly": ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or ""),
            "code_refs_from": [hx(x) for x in idautils.CodeRefsFrom(ea, False)],
            "data_refs_from": [hx(x) for x in idautils.DataRefsFrom(ea)]}


def fn_record(ea):
    fn = ida_funcs.get_func(ea)
    row = {"requested_va": hx(ea), "requested_rva": hx(ea - BASE)}
    if not fn:
        row["function_found"] = False
        return row
    items = [a for a in idautils.FuncItems(fn.start_ea) if ida_bytes.is_code(ida_bytes.get_flags(a))]
    row.update({"function_found": True, "va": hx(fn.start_ea), "rva": hx(fn.start_ea - BASE),
                "end_va": hx(fn.end_ea), "size": hx(fn.end_ea - fn.start_ea),
                "name": ida_funcs.get_func_name(fn.start_ea),
                "instructions": [insn(a) for a in items],
                "xrefs_to_entry": [{"from_va": hx(x.frm), "from_rva": hx(x.frm - BASE),
                                    "type": x.type, "is_code": bool(x.iscode),
                                    "source_function": ida_funcs.get_func_name(x.frm)}
                                   for x in idautils.XrefsTo(fn.start_ea)]})
    try:
        cfunc = ida_hexrays.decompile(fn.start_ea)
        row["pseudocode"] = "\n".join(ida_lines.tag_remove(line.line) for line in cfunc.get_pseudocode()) if cfunc else None
    except Exception as exc:
        row["pseudocode_error"] = repr(exc)
    return row


payload = {"tx": "tx-03-15", "author": "GPT-6 Luna", "evidence": "STATIC_IDB_READ_ONLY",
           "input_path": ida_nalt.get_input_file_path(), "idb_path": idc.get_idb_path(),
           "imagebase": hx(BASE), "rva_rule": "RVA = VA - imagebase",
           "functions": [fn_record(0x40CAE0)]}
if OUTPUT.exists():
    raise RuntimeError("Refusing to overwrite existing output: %s" % OUTPUT)
OUTPUT.parent.mkdir(parents=True, exist_ok=True)
with OUTPUT.open("x", encoding="utf-8") as stream:
    json.dump(payload, stream, ensure_ascii=False, indent=2)
print(json.dumps({"output": str(OUTPUT), "functions": len(payload["functions"]), "imagebase": hx(BASE)}))
