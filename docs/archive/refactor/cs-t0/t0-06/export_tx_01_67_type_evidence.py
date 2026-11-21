"""Read-only IDA export for the two Lua integer-push helpers used by tx-01-67.

Run inside the already-open IDA MCP Python context. The script creates a new,
small supplemental JSON file and refuses to overwrite an existing file.
"""

import json
import os

import ida_bytes
import ida_funcs
import ida_hexrays
import ida_ida
import ida_lines
import ida_nalt
import ida_ua
import idautils
import idc


OUT = r"D:\projects\isaac_hook_reserach\artifacts\cs-t0\tx-01-67-static\supplemental-evidence.json"
IMAGEBASE = ida_nalt.get_imagebase()


def hx(value):
    return "0x%X" % int(value)


def raw_instruction(ea):
    size = idc.get_item_size(ea)
    if not size or size <= 0:
        insn = ida_ua.insn_t()
        size = ida_ua.decode_insn(insn, ea)
    if not size or size <= 0:
        return {"va": hx(ea), "rva": hx(ea - IMAGEBASE), "bytes": "", "text": ""}
    data = ida_bytes.get_bytes(ea, size) or b""
    return {
        "va": hx(ea),
        "rva": hx(ea - IMAGEBASE),
        "bytes": " ".join("%02X" % b for b in bytearray(data)),
        "text": ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or ""),
    }


def function_record(ea):
    f = ida_funcs.get_func(ea)
    if not f:
        return {"requested_va": hx(ea), "error": "no function at address"}
    start = f.start_ea
    end = f.end_ea
    try:
        pseudocode = ida_lines.tag_remove(str(ida_hexrays.decompile(start)))
    except Exception as exc:
        pseudocode = None
        decompile_error = str(exc)
    else:
        decompile_error = None
    instructions = []
    for item in idautils.FuncItems(start):
        if not ida_bytes.is_code(ida_bytes.get_full_flags(item)):
            continue
        insn = ida_ua.insn_t()
        if ida_ua.decode_insn(insn, item) > 0:
            instructions.append(raw_instruction(item))
    record = {
        "name": idc.get_func_name(start),
        "start_va": hx(start),
        "start_rva": hx(start - IMAGEBASE),
        "end_va_exclusive": hx(end),
        "size": hx(end - start),
        "pseudocode": pseudocode,
        "raw_instructions": instructions,
    }
    if decompile_error:
        record["decompile_error"] = decompile_error
    return record


evidence = {
    "task": "tx-01-67-state-timer",
    "collection_mode": "read-only current-IDB extraction",
    "binary": {
        "idb_path": r"F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64",
        "input_path": ida_nalt.get_input_file_path(),
        "imagebase": hx(IMAGEBASE),
        "processor": ida_ida.inf_get_procname(),
        "build_banner": "Binding of Isaac: Repentance+ v1.9.7.17.J460",
    },
    "functions": [function_record(0x85BFD0), function_record(0x85BFF0)],
    "purpose": "Disambiguate the Lua integer bridge signed-extension behavior used by registered raw32 field accessors.",
    "scope_notes": [
        "The two helper bodies are exported in full pseudocode and raw code instructions.",
        "This file supplements ida-evidence.json; it does not replace or overwrite that initial snapshot.",
        "No IDB names, comments, bytes, types, structures, or other database state are modified.",
    ],
}

if os.path.exists(OUT):
    raise RuntimeError("Refusing to overwrite existing evidence file: " + OUT)
os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "x", encoding="utf-8", newline="\n") as stream:
    json.dump(evidence, stream, ensure_ascii=False, indent=2)
    stream.write("\n")
print("Wrote new evidence file:", OUT)
print("Functions:", len(evidence["functions"]))
