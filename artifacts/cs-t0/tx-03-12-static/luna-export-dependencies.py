# Read-only IDA Pro MCP exporter for tx-03-12's direct render dependencies.
# Execute with mcp__ida_mcp__py_exec_file. Writes a create-new JSON file beside
# this script; it does not change or save the IDB and does not execute the game.
import idaapi
import ida_loader
import ida_funcs
import ida_bytes
import ida_hexrays
import ida_lines
import idautils
import idc
import json
from pathlib import Path

IMAGEBASE = idaapi.get_imagebase()
FUNCTIONS = [
    0x6A8120,             # Entity +0x34 shared sort helper
    0x860BA0, 0x872B10,  # NPC conditional Lua path
    0x862B80,             # Tear conditional Lua path
    0x862670, 0x873740,  # Projectile conditional Lua path
    0x4E9B10, 0x59BED0,  # NPC connection / cord geometry
    0x41E720, 0x41E960,  # geometry append and render/finalize path
]
CALL_SITES = [
    0x6D4006, 0x6793E7, 0x65CEDB,
    0x860C09, 0x862CA5, 0x872C02, 0x873832,
    0x4EA1C3, 0x4EA2E7, 0x4EA335, 0x59C35A, 0x59C495,
    0x41F5DB,
]
MAX_INSTRUCTIONS_PER_FUNCTION = 2200


def _hex(value):
    return hex(value) if value is not None else None


def _insn(ea):
    size = idc.get_item_size(ea) or 1
    raw = ida_bytes.get_bytes(ea, size) or b""
    return {
        "va": _hex(ea),
        "rva": _hex(ea - IMAGEBASE),
        "bytes": raw.hex(" "),
        "mnemonic": idc.print_insn_mnem(ea),
        "operand0": idc.print_operand(ea, 0) or "",
        "assembly": idc.generate_disasm_line(ea, 0) or "",
    }


def _items(start):
    func = ida_funcs.get_func(start)
    if not func:
        return None, []
    rows = [ea for ea in idautils.FuncItems(func.start_ea) if idc.is_code(idc.get_full_flags(ea))]
    return func, rows


def _near(ea, before=6, after=4):
    func = ida_funcs.get_func(ea)
    if not func:
        return []
    rows = [x for x in idautils.FuncItems(func.start_ea) if idc.is_code(idc.get_full_flags(x))]
    try:
        i = rows.index(ea)
    except ValueError:
        return []
    return [_insn(x) for x in rows[max(0, i-before):min(len(rows), i+after+1)]]


def _calls(rows):
    out = []
    for ea in rows:
        if (idc.print_insn_mnem(ea) or "").lower() != "call":
            continue
        op = (idc.print_operand(ea, 0) or "").strip()
        indirect = ("[" in op) or op.lower() in ("eax", "ebx", "ecx", "edx", "esi", "edi")
        target = None if indirect else idc.get_operand_value(ea, 0)
        out.append({
            "call_instruction": _insn(ea),
            "indirect": indirect,
            "direct_target_va": _hex(target) if target else None,
            "direct_target_rva": _hex(target - IMAGEBASE) if target else None,
            "direct_target_name": idc.get_name(target) if target else None,
            "code_refs_from_call": [_hex(x) for x in idautils.CodeRefsFrom(ea, False)],
            "indirect_context": _near(ea, 6, 4) if indirect else [],
        })
    return out


def _function(ea):
    func, rows = _items(ea)
    if not func:
        return {"requested_va": _hex(ea), "function_found": False, "name": idc.get_name(ea)}
    try:
        cfunc = ida_hexrays.decompile(func.start_ea)
        pseudo = "\n".join(ida_lines.tag_remove(line.line) for line in cfunc.get_pseudocode()) if cfunc else None
        pseudo_error = None
    except Exception as exc:
        pseudo = None
        pseudo_error = repr(exc)
    shown = rows[:MAX_INSTRUCTIONS_PER_FUNCTION]
    return {
        "requested_va": _hex(ea),
        "va": _hex(func.start_ea),
        "rva": _hex(func.start_ea - IMAGEBASE),
        "end_va": _hex(func.end_ea),
        "size": _hex(func.end_ea - func.start_ea),
        "name": idc.get_name(func.start_ea),
        "pseudocode": pseudo,
        "pseudocode_error": pseudo_error,
        "instruction_count": len(rows),
        "instruction_data_truncated": len(rows) > len(shown),
        "instructions": [_insn(x) for x in shown],
        "direct_calls": _calls(rows),
    }


data = {
    "scope": "tx-03-12 read-only static export of selected direct render callees and call-site bytes; no target process, debugger, IDB mutation/save, or hashes.",
    "database_metadata": {
        "idb_path": ida_loader.get_path(ida_loader.PATH_TYPE_IDB),
        "imagebase": _hex(IMAGEBASE),
    },
    "address_note": "RVA = VA - imagebase. Object offsets are not RVAs. Hex-Rays usercall signatures are not treated as ABI without register/stack evidence.",
    "functions": [_function(ea) for ea in FUNCTIONS],
    "call_site_contexts": {_hex(ea): _near(ea, 7, 5) for ea in CALL_SITES},
}

output_path = Path(globals().get("TX0312_LUNA_OUTPUT", str(Path(__file__).with_name("luna-derived-dependencies.json"))))
with output_path.open("x", encoding="utf-8") as output_file:
    json.dump(data, output_file, ensure_ascii=False, indent=2)
print(json.dumps({"output": str(output_path), "functions": len(FUNCTIONS), "call_sites": len(CALL_SITES)}, ensure_ascii=False))
