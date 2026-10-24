"""Read-only IDA evidence export for tx-01-67 state/timer field survey.

Run inside the already-open IDA MCP Python context. This reads the current
IDB and creates one JSON evidence file; it does not modify the IDB or target.
"""

import json
import os

import ida_bytes
import ida_funcs
import ida_hexrays
import ida_ida
import ida_lines
import ida_name
import ida_nalt
import ida_ua
import idautils
import idc


OUT = r"D:\projects\isaac_hook_reserach\artifacts\cs-t0\tx-01-67-static\ida-evidence.json"
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
    callers = []
    for xref in idautils.XrefsTo(start, 0):
        caller_func = ida_funcs.get_func(xref.frm)
        callers.append({
            "va": hx(xref.frm),
            "rva": hx(xref.frm - IMAGEBASE),
            "function": ida_name.get_name(caller_func.start_ea) if caller_func else "",
        })
    record = {
        "name": ida_name.get_name(start),
        "start_va": hx(start),
        "start_rva": hx(start - IMAGEBASE),
        "end_va_exclusive": hx(end),
        "size": hx(end - start),
        "pseudocode": pseudocode,
        "raw_instructions": instructions,
        "code_xrefs_to_entry": callers,
    }
    if decompile_error:
        record["decompile_error"] = decompile_error
    return record


def point_context(ea, before=7, after=3):
    f = ida_funcs.get_func(ea)
    lo = f.start_ea if f else max(IMAGEBASE, ea - 0x40)
    hi = f.end_ea if f else ea + 0x40
    items = [ea]
    cur = ea
    for _ in range(before):
        cur = idc.prev_head(cur, lo)
        if cur == idc.BADADDR:
            break
        items.append(cur)
    cur = ea
    for _ in range(after):
        cur = idc.next_head(cur, hi)
        if cur == idc.BADADDR or cur >= hi:
            break
        items.append(cur)
    return [raw_instruction(a) for a in sorted(set(items))]


def registration_sites(owner_ea, helper_targets):
    f = ida_funcs.get_func(owner_ea)
    if not f:
        return {"error": "no owner function at " + hx(owner_ea)}
    sites = []
    for ea in idautils.FuncItems(f.start_ea):
        if not ida_bytes.is_code(ida_bytes.get_full_flags(ea)):
            continue
        if idc.print_insn_mnem(ea).lower() != "call":
            continue
        target = idc.get_operand_value(ea, 0)
        if target not in helper_targets:
            continue
        context = point_context(ea, before=10, after=2)
        sites.append({
            "call_va": hx(ea),
            "call_rva": hx(ea - IMAGEBASE),
            "helper_va": hx(target),
            "context": context,
        })
    return sites


function_addresses = [
    0x866960,  # Lua registration source: exact class/property grouping
    0x87DCE0,  # generic method registration for computed Entity.FrameCount
    0x8849B0, 0x884EA0,  # Pickup Timeout/Wait and State registration helpers
    0x8854A0,  # Effect integer field registration helper
    0x885F80,  # Familiar integer field registration helper
    0x8877E0, 0x887AD0,  # NPC integer field registration helpers
    0x88A100, 0x88A740,  # GridDesc raw field and GridEntity State property
    0x888ED0, 0x888630,  # Laser Timeout registration, retained to disambiguate class
    0x6A9C10,  # Entity.FrameCount implementation
    0x5B1940, 0x60D1F0, 0x85E440, 0x436210,  # Pickup raw getter/setter pairs
    0x60D080,  # Player.GetDamageCooldown raw getter
    0x8BEDB0, 0x8BEE10,  # Pickup.State raw getter/setter closures
    0x8BEF40, 0x8BEFA0,  # Effect raw getter/setter closures
    0x8BF210, 0x8BF270,  # Familiar raw getter/setter closures
    0x8BF5B0, 0x8BF610, 0x8BF920,  # NPC raw getter/setter closures
    0x8C05C0, 0x8C0620,  # GridDesc raw getter/setter closures
    0x4176F0, 0x665B70,  # GridEntity.State direct getter/setter
    0x6A8290, 0x6A93A0,  # Entity constructor / Init
    0x753DA0, 0x755470,  # Player constructor / Init
    0x6B8B20,  # NPC Init
    0x6E02F0,  # Pickup Init
    0x60DE70,  # Familiar Init
    0x5CDB20,  # EntityEffect Init
    0x709DB0,  # Grid Init
]

binding_helpers = [
    0x87DCE0, 0x8849B0, 0x884EA0, 0x8854A0, 0x885F80,
    0x8877E0, 0x887AD0, 0x88A100, 0x88A740, 0x888ED0, 0x888630,
]

evidence = {
    "task": "tx-01-67-state-timer",
    "collection_mode": "read-only current-IDB extraction",
    "binary": {
        "idb_path": r"F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64",
        "input_path": ida_nalt.get_input_file_path(),
        "imagebase": hx(IMAGEBASE),
        "processor": ida_ida.inf_get_procname(),
        "build_banner": "Binding of Isaac: Repentance+ v1.9.7.17.J460",
        "build_note": "An older v1.9.7.16 string also exists; this extraction follows the current IDB banner.",
    },
    "functions": [function_record(ea) for ea in function_addresses],
    "entity_lua_binding_sites": registration_sites(0x866960, set(binding_helpers)),
    "scope_notes": [
        "The registration root and helper/accessor bodies are included in full where decompilation succeeds.",
        "Raw instruction records contain VA, RVA, bytes, and disassembly only for code items.",
        "Constructor/Init functions are sampled as limited initialization anchors; their presence is not a claim that every writer or reset path is covered.",
        "No runtime state, target process memory, game input, injection, build, test, hash, or gate is inspected or executed.",
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
print("Functions:", len(evidence["functions"]), "binding sites:", len(evidence["entity_lua_binding_sites"]))
