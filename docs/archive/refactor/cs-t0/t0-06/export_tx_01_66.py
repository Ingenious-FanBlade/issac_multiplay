"""Read-only IDA evidence export for tx-01-66.

Run inside the already-open IDA MCP Python context. This script only reads the
current IDB and writes a create-new JSON evidence file; it does not patch or
rename IDB items and performs no hash/check/gate operation.
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


OUT = r"D:\projects\isaac_hook_reserach\artifacts\cs-t0\tx-01-66-static\ida-evidence.json"
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
        callers.append({"va": hx(xref.frm), "rva": hx(xref.frm - IMAGEBASE),
                        "function": ida_name.get_name(ida_funcs.get_func(xref.frm).start_ea)
                        if ida_funcs.get_func(xref.frm) else ""})
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


def point_context(ea, before=5, after=8):
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


function_addresses = [
    0x931050,  # _main: one outer update followed by render
    0x9AB6D0,  # top update wrapper
    0x954CD0,  # standard/even vs odd/player-only driver
    0x6FADC0,  # standard simulation owner and post-update tail
    0x802980,  # Room::Update
    0x4186C0,  # EntityList::Update / staged remove commit
    0x419E70,  # spatial/collision pass
    0x419750,  # post-list pass
    0x9BB5D0,
    0x4257B0,
    0x8607A0,  # MC_POST_UPDATE tail
    0x6FD3F0,  # odd player-only helper
    0x6FD350,  # odd helper guard
    0x6B0EB0,  # Entity base vtable slot 4 (+0x10), position stage
    0x866960,  # Lua property registration, includes Position/Velocity
    0x417290,  # Position getter
    0x4172D0,  # Velocity getter
    0x4288F0,  # Position setter
    0x428910,  # Velocity setter
    0x4172F0,  # PositionOffset setter (corrected in primary review)
    0x417310,  # PositionOffset getter (corrected in primary review)
    0x6DAD60,  # Player VelocityBeforeUpdate getter
    0x709DF0,  # Grid position getter
    0x812F00,  # Room GetGridPosition
    0x86FB20,  # GetRoomEntities (table creation)
    0x41AF60,  # QueryType (mutating scratch/cache/statistics)
    0x41A790,  # QueryRadius (mutating scratch/cache/statistics)
    0x9555C0,  # render owner called after TopExit
    0x6FBC10,  # gameplay render/state presentation branch
    0x6FA0C0,  # run teardown target
    0x959720,  # alternate/loading envelope owner
    0x864EE0,  # alternate envelope tail
    0x865180,  # new-room hook target
    0x92F1C0,  # room-lifecycle FSM owner function
    0x6FE2F0,  # additional selected function; not the 6FB51C hook target
    0x8318A0,  # actual ordinary room-change owner called at 6FB51C
]

points = {
    "main_top_update_then_render": 0x9312EC,
    "main_render_call": 0x9312F1,
    "top_wrapper_inner_driver": 0x9AB6F2,
    "driver_frame_branch": 0x9551D6,
    "driver_position_stage_restore_begin": 0x955202,
    "driver_position_stage_restore_end": 0x95520E,
    "driver_standard_simulation_call": 0x955231,
    "driver_odd_branch_test": 0x955238,
    "driver_odd_player_helper_call": 0x955241,
    "standard_room_update_call": 0x6FBA4D,
    "standard_after_room_update": 0x6FBA52,
    "standard_secondary_update": 0x6FBA5D,
    "standard_post_update_callback": 0x6FBBA5,
    "room_update_entity_list_call": 0x803E70,
    "entity_list_spatial_call": 0x4196AC,
    "odd_player_update_call": 0x6FD609,
    "odd_player_slot4_position_stage_call": 0x6FD62D,
    "room_lifecycle_owner_hook_site": 0x6FB4F0,
    "room_change_owner_hook_site": 0x6FB51C,
    "room_lifecycle_nested_sim_site": 0x92FC77,
    "room_change_nested_sim_site": 0x6FE37C,
}

vtable_addresses = {
    "Entity": 0xB6730C,
    "Player": 0xB6BDD0,
    "NPC": 0xB67468,
    "Tear": 0xB64EAC,
    "Projectile": 0xB64990,
    "Pickup": 0xB67F24,
    "Familiar": 0xB6491C,
    "Effect": 0xB63BDC,
}
vtable_records = {}
for kind, base in vtable_addresses.items():
    entries = []
    for slot in range(12):
        target = ida_bytes.get_dword(base + slot * 4)
        entries.append({"slot": slot, "byte_offset": hx(slot * 4),
                        "target_va": hx(target), "target_rva": hx(target - IMAGEBASE),
                        "target_name": ida_name.get_name(target)})
    vtable_records[kind] = {"va": hx(base), "rva": hx(base - IMAGEBASE),
                            "entries": entries}

evidence = {
    "task": "tx-01-66-readonly-sampling",
    "collection_mode": "read-only IDA database extraction",
    "binary": {
        "idb_path": r"F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64",
        "input_path": ida_nalt.get_input_file_path(),
        "imagebase": hx(IMAGEBASE),
        "processor": ida_ida.inf_get_procname(),
        "build_banner": "Binding of Isaac: Repentance+ v1.9.7.17.J460",
        "build_note": "An older v1.9.7.16 string also exists in the image; this evidence uses the current IDB banner above.",
    },
    "functions": [function_record(ea) for ea in function_addresses],
    "instruction_contexts": {name: point_context(ea) for name, ea in points.items()},
    "entity_vtables": vtable_records,
    "prior_static_report_refs": [
        "docs/archive/refactor/cs-t0/t0-06/tx-01-63-entity-lifecycle.md",
        "docs/archive/refactor/cs-t0/t0-06/tx-01-64-category-coverage.md",
        "docs/archive/refactor/cs-t0/t0-06/tx-01-65-hp-analysis.md",
    ],
    "scope_note": "No IDB patch, runtime inspection, hash computation, gate, test, build, or game operation is performed by this exporter.",
}

if os.path.exists(OUT):
    raise RuntimeError("Refusing to overwrite existing evidence file: " + OUT)
os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "x", encoding="utf-8", newline="\n") as stream:
    json.dump(evidence, stream, ensure_ascii=False, indent=2)
    stream.write("\n")
print("Wrote new evidence file:", OUT)
print("Functions:", len(evidence["functions"]), "contexts:", len(evidence["instruction_contexts"]))
