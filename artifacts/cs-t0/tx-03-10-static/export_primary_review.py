"""tx-03-10 primary static reread. Run with IDA MCP py_exec_file.

Reads the current IDB only. No game execution, IDB mutation, tests or hashes.
Writes a new evidence file beside this script; refuses to overwrite it.
"""

import json
from pathlib import Path

import ida_bytes
import ida_funcs
import ida_hexrays
import ida_lines
import ida_nalt
import idaapi
import idautils
import idc


FUNCTIONS = (
    0x6B0EB0, 0x6FD3F0, 0x6AB440, 0x6FBC10,
    0x80EA80, 0x80F200, 0x9A3EB0, 0x863680,
)

RANGES = (
    (0x9312E7, 0x9312F6),
    (0x9551CF, 0x955249),
    (0x6FD480, 0x6FD4E6),
    (0x6FD521, 0x6FD595),
    (0x6FD5E6, 0x6FD639),
    (0x6FD68A, 0x6FD6DF),
    (0x6FD744, 0x6FD7B5),
    (0x6B0EB0, 0x6B1000),
    (0x6AB7B0, 0x6AB901),
    (0x6FBC5A, 0x6FBC80),
    (0x6FC070, 0x6FC0AB),
    (0x6FC381, 0x6FC3E0),
)


def instructions(begin, end, image_base):
    rows = []
    for ea in idautils.Heads(begin, end):
        if not ida_bytes.is_code(ida_bytes.get_full_flags(ea)):
            continue
        raw = ida_bytes.get_bytes(ea, ida_bytes.get_item_size(ea))
        rows.append({
            "va": hex(ea), "rva": hex(ea - image_base),
            "bytes": raw.hex(" ") if raw else None,
            "asm": ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or ""),
        })
    return rows


def export(output_file=None):
    image_base = idaapi.get_imagebase()
    banner = ida_bytes.get_strlit_contents(0xB7E1B4, -1, ida_nalt.STRTYPE_C)
    result = {
        "tx": "tx-03-10", "kind": "PRIMARY_STATIC_REREAD",
        "input_path": ida_nalt.get_input_file_path(),
        "idb_path": idc.get_idb_path(), "image_base": hex(image_base),
        "banner": banner.decode("utf-8", errors="replace") if banner else None,
        "functions": {}, "ranges": [],
        "limitations": [
            "Static recovered code only; no runtime observation or safety proof.",
            "No IDB mutation, game, debugger control, build, tests or hashes.",
            "Indirect calls and unexpanded callees are not proved side-effect-free.",
        ],
    }
    for ea in FUNCTIONS:
        entry = {"va": hex(ea), "rva": hex(ea - image_base)}
        func = ida_funcs.get_func(ea)
        if func is None:
            entry["error"] = "No recovered function"
        else:
            entry["start_va"] = hex(func.start_ea)
            entry["end_va_exclusive"] = hex(func.end_ea)
            entry["name"] = idc.get_func_name(ea)
            try:
                entry["pseudocode"] = ida_lines.tag_remove(str(ida_hexrays.decompile(ea)))
            except Exception as error:
                entry["decompile_error"] = str(error)
        result["functions"][hex(ea)] = entry
    for begin, end in RANGES:
        result["ranges"].append({
            "begin_va": hex(begin), "end_va_exclusive": hex(end),
            "instructions": instructions(begin, end, image_base),
        })
    output = Path(output_file) if output_file is not None else Path(__file__).resolve().with_name("primary-evidence.json")
    with output.open("x", encoding="utf-8", newline="\n") as handle:
        json.dump(result, handle, ensure_ascii=False, indent=2)
        handle.write("\n")
    print(json.dumps({"output": str(output), "function_count": len(FUNCTIONS),
                      "range_count": len(RANGES), "status": "STATIC_EXPORTED"}))


if __name__ == "__main__":
    export()
