# Read-only IDA static exporter for tx-03-17 Sprite retirement ownership.
# It reads the active IDB only, does not execute the target, mutate/save the IDB,
# change bytes, or calculate hashes. The JSON output is create-new only.
import os
import json
from pathlib import Path

import idaapi
import ida_bytes
import ida_funcs
import ida_hexrays
import ida_lines
import ida_loader
import ida_nalt
import idautils
import idc


IMAGEBASE = idaapi.get_imagebase()
FUNCTIONS = [
    0x4074C0,  # Sprite payload constructor
    0x407F10,  # Sprite Reset
    0x408170,  # standalone Sprite destructor
    0x4087A0,  # animation state destructor
    0x407A60,  # Sprite layer destructor
    0x40BD50,  # Load
    0x40C000,  # LoadGraphics
    0x40E110,  # ANM cache reference increment
    0x40E2B0,  # ANM cache reference decrement
    0x40E470,  # zero-reference ANM cache sweep
    0x8A71B0,  # Sprite Lua metatable setup
    0x8BB720,  # Sprite Lua userdata constructor
    0x8C31F0,  # generic Sprite Lua __gc closure
    0x85C3E0,  # Lua userdata extraction/type helper
    0x8C4CE0,  # UserdataValue<ANM2> virtual destructor
]
XREF_TARGETS = [0x407F10, 0x408170, 0x407A60, 0x40E470, 0x8A71B0, 0x8BB720, 0x8C31F0, 0x8C4CE0]
CONTEXT_SITES = [0x867218, 0x867223, 0x86723D, 0x8672A1, 0x8A7258, 0x8A72AD, 0x8BB734, 0x8BB756, 0x8BB762, 0x8C3209, 0x8C4D16, 0x8C4D45, 0x407FAC, 0x408196, 0x4081AE, 0x40E26E, 0x40E42C, 0x40E4BB]
USERDATA_VTABLE = 0xB73228


def _hex(value):
    return hex(value) if value is not None else None


def _insn(ea):
    size = idc.get_item_size(ea) or 1
    raw = ida_bytes.get_bytes(ea, size) or b""
    return {
        "va": _hex(ea),
        "rva": _hex(ea - IMAGEBASE),
        "bytes": raw.hex(" "),
        "assembly": idc.generate_disasm_line(ea, 0) or "",
    }


def _function_items(start):
    func = ida_funcs.get_func(start)
    if not func:
        return None, []
    items = [ea for ea in idautils.FuncItems(func.start_ea) if idc.is_code(idc.get_full_flags(ea))]
    return func, items


def _near(ea, before=5, after=5):
    func = ida_funcs.get_func(ea)
    if not func:
        return []
    items = [x for x in idautils.FuncItems(func.start_ea) if idc.is_code(idc.get_full_flags(x))]
    try:
        index = items.index(ea)
    except ValueError:
        return []
    return [_insn(x) for x in items[max(0, index - before):min(len(items), index + after + 1)]]


def _direct_calls(items):
    rows = []
    for ea in items:
        if (idc.print_insn_mnem(ea) or "").lower() not in ("call", "jmp"):
            continue
        operand = (idc.print_operand(ea, 0) or "").strip()
        indirect = ("[" in operand) or operand.lower() in ("eax", "ebx", "ecx", "edx", "esi", "edi")
        target = None if indirect else idc.get_operand_value(ea, 0)
        rows.append({
            "instruction": _insn(ea),
            "indirect": indirect,
            "target_va": _hex(target) if target else None,
            "target_rva": _hex(target - IMAGEBASE) if target else None,
            "target_name": idc.get_name(target) if target else None,
        })
    return rows


def _function_record(ea):
    func, items = _function_items(ea)
    if not func:
        return {"requested_va": _hex(ea), "function_found": False, "name": idc.get_name(ea)}
    start, end = func.start_ea, func.end_ea
    raw = ida_bytes.get_bytes(start, end - start) or b""
    pseudocode = None
    pseudocode_error = None
    try:
        cfunc = ida_hexrays.decompile(start)
        if cfunc:
            pseudocode = "\n".join(ida_lines.tag_remove(line.line) for line in cfunc.get_pseudocode())
    except Exception as exc:
        pseudocode_error = repr(exc)
    return {
        "requested_va": _hex(ea),
        "name": idc.get_name(start),
        "va": _hex(start),
        "rva": _hex(start - IMAGEBASE),
        "end_va": _hex(end),
        "size": _hex(end - start),
        "type": idc.get_type(start),
        "raw_bytes": raw.hex(" "),
        "pseudocode": pseudocode,
        "pseudocode_error": pseudocode_error,
        "instructions": [_insn(x) for x in items],
        "direct_calls_and_jumps": _direct_calls(items),
    }


def _incoming_code_xrefs(ea):
    rows = []
    for xref in idautils.XrefsTo(ea, 0):
        if not idc.is_code(idc.get_full_flags(xref.frm)):
            continue
        source_func = ida_funcs.get_func(xref.frm)
        rows.append({
            "from_va": _hex(xref.frm),
            "from_rva": _hex(xref.frm - IMAGEBASE),
            "xref_type": xref.type,
            "source_function_va": _hex(source_func.start_ea) if source_func else None,
            "source_function_name": idc.get_name(source_func.start_ea) if source_func else None,
            "instruction": _insn(xref.frm),
        })
    return rows


def _vtable_record(ea):
    target = ida_bytes.get_dword(ea)
    return {
        "label": "luabridge::UserdataValue<ANM2>::vftable",
        "va": _hex(ea),
        "rva": _hex(ea - IMAGEBASE),
        "symbol": idc.get_name(ea),
        "slot0_bytes": (ida_bytes.get_bytes(ea, 4) or b"").hex(" "),
        "slot0_target_va": _hex(target),
        "slot0_target_rva": _hex(target - IMAGEBASE),
        "slot0_target_name": idc.get_name(target),
    }


def export_tx0317():
    return {
        "tx": "tx-03-17",
        "evidence": "STATIC_IDB_READ_ONLY",
        "scope": "Sprite Reset, standalone destructor, Lua userdata finalizer, ANM reference lifetime, and bounded normal cleanup path.",
        "database": {
            "idb_path": ida_loader.get_path(ida_loader.PATH_TYPE_IDB),
            "input_path": ida_nalt.get_input_file_path(),
            "imagebase": _hex(IMAGEBASE),
            "address_note": "RVA = VA - imagebase; userdata and object field offsets are relative to their respective object bases, not RVAs.",
        },
        "functions": [_function_record(ea) for ea in FUNCTIONS],
        "incoming_code_xrefs": {_hex(ea): _incoming_code_xrefs(ea) for ea in XREF_TARGETS},
        "context_sites": {_hex(ea): _near(ea) for ea in CONTEXT_SITES},
        "userdata_vtable": _vtable_record(USERDATA_VTABLE),
        "limitations": [
            "No target process, native helper, game, debugger, build, or experiment was run.",
            "IDB input path is recorded; disk-file identity and hashes are not asserted.",
            "IDA direct xrefs do not enumerate all dynamic or indirect callers.",
            "Decompiler types do not prove exception safety, worker/GPU lifetime, or complete thread safety.",
            "A normal Lua userdata finalizer does not prove behavior after a partially completed C++ constructor.",
        ],
    }


default_output = Path(r"F:\projects_isaac\issac_multiplay\artifacts\cs-t0\tx-03-17-static\luna-retirement-static.json")
configured_output = os.environ.get("TX0317_LUNA_OUTPUT") or globals().get("TX0317_LUNA_OUTPUT")
output_path = Path(configured_output) if configured_output else default_output
if output_path.exists() and output_path.is_dir():
    output_path = output_path / "luna-retirement-static.json"
expected_dir = default_output.parent.resolve()
resolved_output = output_path.resolve()
if resolved_output.parent != expected_dir or not resolved_output.name.startswith("luna-"):
    raise ValueError("TX0317_LUNA_OUTPUT must name a luna-* file directly inside artifacts/cs-t0/tx-03-17-static")
output_path.parent.mkdir(parents=True, exist_ok=True)
with output_path.open("x", encoding="utf-8") as output_file:
    json.dump(export_tx0317(), output_file, ensure_ascii=False, indent=2)
print(json.dumps({"output": str(output_path), "function_count": len(FUNCTIONS), "xref_target_count": len(XREF_TARGETS)}, ensure_ascii=False))
