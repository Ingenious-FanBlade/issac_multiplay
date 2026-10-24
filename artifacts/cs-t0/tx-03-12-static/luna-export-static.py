# Read-only IDA Pro MCP exporter for tx-03-12 derived Entity render slots.
# Execute this file with mcp__ida_mcp__py_exec_file. It reads the active IDB and
# writes one create-new JSON file beside itself. It does not mutate/save the IDB
# or execute the target process.
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
VTABLES = {
    "Entity": 0xB6730C,
    "Entity_NPC": 0xB67468,
    "Entity_Tear": 0xB64EAC,
    "Entity_Projectile": 0xB64990,
}
SLOT_OFFSETS = [0x14, 0x18, 0x34]
DISPATCH_SITES = [0x4199F5, 0x419A2E, 0x419B11, 0x419C67]
MAX_INSTRUCTIONS_PER_FUNCTION = 2000


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


def _function_items(start):
    func = ida_funcs.get_func(start)
    if not func:
        return None, []
    items = [ea for ea in idautils.FuncItems(func.start_ea) if idc.is_code(idc.get_full_flags(ea))]
    return func, items


def _near(ea, before=6, after=4):
    func = ida_funcs.get_func(ea)
    if not func:
        return []
    items = [x for x in idautils.FuncItems(func.start_ea) if idc.is_code(idc.get_full_flags(x))]
    try:
        index = items.index(ea)
    except ValueError:
        return []
    lo = max(0, index - before)
    hi = min(len(items), index + after + 1)
    return [_insn(x) for x in items[lo:hi]]


def _vtable_record(label, start):
    entries = []
    for offset in SLOT_OFFSETS:
        ea = start + offset
        target = ida_bytes.get_dword(ea)
        entries.append({
            "object_vtable_byte_offset": _hex(offset),
            "entry_va": _hex(ea),
            "entry_rva": _hex(ea - IMAGEBASE),
            "entry_bytes": (ida_bytes.get_bytes(ea, 4) or b"").hex(" "),
            "target_va": _hex(target),
            "target_rva": _hex(target - IMAGEBASE),
            "target_name": idc.get_name(target),
        })
    return {
        "label": label,
        "vtable_va": _hex(start),
        "vtable_rva": _hex(start - IMAGEBASE),
        "symbol": idc.get_name(start),
        "raw_bytes_through_plus_0x34": (ida_bytes.get_bytes(start, 0x38) or b"").hex(" "),
        "selected_entries": entries,
    }


def _call_inventory(items):
    rows = []
    for ea in items:
        if (idc.print_insn_mnem(ea) or "").lower() != "call":
            continue
        operand = (idc.print_operand(ea, 0) or "").strip()
        indirect = ("[" in operand) or operand.lower() in ("eax", "ebx", "ecx", "edx", "esi", "edi")
        target = None if indirect else idc.get_operand_value(ea, 0)
        rows.append({
            "call_instruction": _insn(ea),
            "indirect": indirect,
            "direct_target_va": _hex(target) if target else None,
            "direct_target_rva": _hex(target - IMAGEBASE) if target else None,
            "direct_target_name": idc.get_name(target) if target else None,
            "code_refs_from_call": [_hex(x) for x in idautils.CodeRefsFrom(ea, False)],
            "context": _near(ea, 5, 3) if indirect else [],
        })
    return rows


def _function_record(ea):
    func, items = _function_items(ea)
    if not func:
        return {"requested_va": _hex(ea), "function_found": False, "name": idc.get_name(ea)}
    size = func.end_ea - func.start_ea
    pseudocode = None
    pseudocode_error = None
    try:
        cfunc = ida_hexrays.decompile(func.start_ea)
        if cfunc:
            pseudocode = "\n".join(ida_lines.tag_remove(line.line) for line in cfunc.get_pseudocode())
    except Exception as exc:
        pseudocode_error = repr(exc)
    shown_items = items[:MAX_INSTRUCTIONS_PER_FUNCTION]
    return {
        "requested_va": _hex(ea),
        "va": _hex(func.start_ea),
        "rva": _hex(func.start_ea - IMAGEBASE),
        "end_va": _hex(func.end_ea),
        "size": _hex(size),
        "name": idc.get_name(func.start_ea),
        "pseudocode": pseudocode,
        "pseudocode_error": pseudocode_error,
        "instruction_count": len(items),
        "instruction_data_truncated": len(items) > len(shown_items),
        "instructions": [_insn(x) for x in shown_items],
        "direct_calls": _call_inventory(items),
    }


def export_tx0312():
    vtables = {label: _vtable_record(label, ea) for label, ea in VTABLES.items()}
    targets = []
    for table in vtables.values():
        targets.extend(int(row["target_va"], 16) for row in table["selected_entries"])
    unique_targets = sorted(set(targets))
    return {
        "scope": "tx-03-12 read-only static export from the active IDB; no process execution, debugger, IDB mutation, IDB save, or hashes.",
        "database_metadata": {
            "idb_path": ida_loader.get_path(ida_loader.PATH_TYPE_IDB),
            "imagebase": _hex(IMAGEBASE),
        },
        "address_note": "RVA = VA - imagebase. Vtable offsets are byte offsets from the object's vptr; they are not RVAs.",
        "vtables": vtables,
        "dispatch_call_contexts": {_hex(ea): _near(ea, 7, 5) for ea in DISPATCH_SITES},
        "unique_selected_slot_targets": [_function_record(ea) for ea in unique_targets],
    }


output_path = Path(globals().get("TX0312_LUNA_OUTPUT", str(Path(__file__).with_name("luna-derived-render.json"))))
with output_path.open("x", encoding="utf-8") as output_file:
    json.dump(export_tx0312(), output_file, ensure_ascii=False, indent=2)
print(json.dumps({"output": str(output_path), "vtable_count": len(VTABLES), "dispatch_sites": len(DISPATCH_SITES)}, ensure_ascii=False))
