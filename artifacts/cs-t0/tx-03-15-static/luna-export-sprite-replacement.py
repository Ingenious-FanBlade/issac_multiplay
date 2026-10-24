# Read-only IDA Pro MCP exporter for tx-03-15 Sprite replacement/reload paths.
# Execute with mcp__ida_mcp__py_exec_file. Writes a create-new JSON file and
# only reads the active IDB; it does not rename/type/patch/save or run target code.
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
DEFAULT_OUTPUT = r"F:\projects_isaac\issac_multiplay\artifacts\cs-t0\tx-03-15-static\luna-sprite-replacement.json"
OUTPUT = Path(os.environ.get("TX0315_LUNA_OUTPUT") or globals().get("TX0315_LUNA_OUTPUT") or DEFAULT_OUTPUT)
FUNCTIONS = [0x866960, 0x40BF30, 0x40BFF0, 0x40C000, 0x408640, 0x408590, 0x40C3B0]
FOCUS_SITES = [0x86731A, 0x86731F, 0x867324, 0x86732F, 0x867334, 0x867339]


def hx(value):
    return hex(value) if value is not None else None


def instruction(ea):
    size = idc.get_item_size(ea) or 1
    raw = ida_bytes.get_bytes(ea, size) or b""
    return {
        "va": hx(ea),
        "rva": hx(ea - BASE),
        "bytes": raw.hex(" "),
        "size": size,
        "assembly": ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or ""),
        "code_refs_from": [hx(x) for x in idautils.CodeRefsFrom(ea, False)],
        "data_refs_from": [hx(x) for x in idautils.DataRefsFrom(ea)],
    }


def function_record(address):
    fn = ida_funcs.get_func(address)
    row = {"requested_va": hx(address), "requested_rva": hx(address - BASE), "name_at_request": idc.get_name(address)}
    if not fn:
        row["function_found"] = False
        return row
    items = [ea for ea in idautils.FuncItems(fn.start_ea) if ida_bytes.is_code(ida_bytes.get_flags(ea))]
    row.update({
        "function_found": True,
        "va": hx(fn.start_ea),
        "rva": hx(fn.start_ea - BASE),
        "end_va": hx(fn.end_ea),
        "size": hx(fn.end_ea - fn.start_ea),
        "name": ida_funcs.get_func_name(fn.start_ea),
        "chunks": [[hx(a), hx(b)] for a, b in idautils.Chunks(fn.start_ea)],
        "instructions": [instruction(ea) for ea in items],
        "xrefs_to_entry": [
            {"from_va": hx(x.frm), "from_rva": hx(x.frm - BASE), "type": x.type,
             "is_code": bool(x.iscode), "source_function": ida_funcs.get_func_name(x.frm)}
            for x in idautils.XrefsTo(fn.start_ea)
        ],
    })
    try:
        cfunc = ida_hexrays.decompile(fn.start_ea)
        row["pseudocode"] = "\n".join(ida_lines.tag_remove(line.line) for line in cfunc.get_pseudocode()) if cfunc else None
    except Exception as exc:
        row["pseudocode_error"] = repr(exc)
    return row


def focus_record(address, before=8, after=7):
    fn = ida_funcs.get_func(address)
    row = {"va": hx(address), "rva": hx(address - BASE), "name": idc.get_name(address)}
    if not fn:
        row["function_found"] = False
        return row
    items = [ea for ea in idautils.FuncItems(fn.start_ea) if ida_bytes.is_code(ida_bytes.get_flags(ea))]
    try:
        index = items.index(address)
    except ValueError:
        row["instruction_found"] = False
        return row
    lo = max(0, index - before)
    hi = min(len(items), index + after + 1)
    row.update({
        "instruction_found": True,
        "function_va": hx(fn.start_ea),
        "function_rva": hx(fn.start_ea - BASE),
        "context": [instruction(ea) for ea in items[lo:hi]],
    })
    return row


payload = {
    "tx": "tx-03-15",
    "author": "GPT-6 Luna",
    "evidence": "STATIC_IDB_READ_ONLY",
    "input_path": ida_nalt.get_input_file_path(),
    "idb_path": idc.get_idb_path(),
    "imagebase": hx(BASE),
    "rva_rule": "RVA = VA - imagebase; object offsets and table indices are not RVAs",
    "functions": [function_record(ea) for ea in FUNCTIONS],
    "registration_focus_sites": [focus_record(ea) for ea in FOCUS_SITES],
    "limitations": [
        "Static IDB evidence only; no runtime calls or behavior observed",
        "No hash or fingerprint checks performed",
        "IDA xrefs do not include dynamically constructed targets",
        "Decompiler types are not by themselves a complete ABI contract",
    ],
}

if OUTPUT.exists():
    raise RuntimeError("Refusing to overwrite existing output: %s" % OUTPUT)
OUTPUT.parent.mkdir(parents=True, exist_ok=True)
with OUTPUT.open("x", encoding="utf-8") as stream:
    json.dump(payload, stream, ensure_ascii=False, indent=2)
print(json.dumps({"output": str(OUTPUT), "functions": len(payload["functions"]),
                  "registration_sites": len(payload["registration_focus_sites"]), "imagebase": hx(BASE)}))
