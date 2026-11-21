"""Primary static reread for tx-01-66; no runtime or IDB mutation.

Run with IDA MCP py_exec_file. Keeps the original subagent export untouched.
An existing output file is never overwritten. CFG reachability is static
control-flow analysis of normal IDB edges, not a game experiment.
"""

import json
from pathlib import Path

import ida_bytes
import ida_funcs
import ida_gdl
import ida_hexrays
import ida_lines
import ida_nalt
import idaapi
import idautils
import idc


FUNCTIONS = {
    0x417290: "Position copy getter, Entity +33C/+340",
    0x4172D0: "Velocity copy getter, Entity +360/+364",
    0x4288F0: "Position setter",
    0x428910: "Velocity setter",
    0x417310: "PositionOffset address getter, +34C",
    0x4172F0: "PositionOffset two-component setter",
    0x6DAD60: "VelocityBeforeUpdate address getter, inline Player+1660",
    0x709DF0: "Grid position computed from stored Grid index",
    0x812F00: "Grid position computed from Room width and supplied index",
    0x9AB6D0: "Outer update wrapper: exactly one direct 954CD0 call",
    0x6B0EB0: "Position slot4: flag2 skip path, otherwise staging and movement",
}

RANGES = [
    (0x9312E7, 0x9312F6, "Main call update at 9312EC then render at 9312F1"),
    (0x9551CF, 0x955249, "Driver parity, staged restore, standard versus odd call"),
    (0x6FADC0, 0x6FAE13, "Standard entry and pause/predicate early-return paths"),
    (0x6FBA38, 0x6FBA85, "Normal Room call and post-room calls"),
    (0x6FBB91, 0x6FBBAD, "Standard post-update callback and return"),
    (0x803E62, 0x803E82, "Room passes literal zero to EntityList::Update"),
    (0x41969C, 0x4196B8, "EntityList spatial pass then render ordering pass"),
    (0x6FD5E0, 0x6FD647, "Odd helper Player Update and slot4 calls"),
    (0x92FC6B, 0x92FC81, "FSM Room callsite 92FC77, return 92FC7C"),
    (0x6FE376, 0x6FE388, "RoomChange Room callsite 6FE37C, return 6FE381"),
    (0x86894D, 0x868979, "Entity Position/Velocity property registration"),
]


def instructions(begin, end):
    rows = []
    for ea in idautils.Heads(begin, end):
        if not ida_bytes.is_code(ida_bytes.get_full_flags(ea)):
            continue
        size = ida_bytes.get_item_size(ea)
        data = ida_bytes.get_bytes(ea, size)
        rows.append({"va": hex(ea), "rva": hex(ea - idaapi.get_imagebase()),
                     "bytes": data.hex(" ") if data else None,
                     "asm": ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or "")})
    return rows


def normal_room_cfg():
    flow = ida_gdl.FlowChart(ida_funcs.get_func(0x802980))
    blocks = {block.start_ea: block for block in flow}
    call_block = None
    return_blocks = []
    graph = []
    for block in flow:
        if block.start_ea <= 0x803E70 < block.end_ea:
            call_block = block.start_ea
        last = idc.prev_head(block.end_ea, block.start_ea)
        if idc.print_insn_mnem(last).startswith("ret"):
            return_blocks.append(block.start_ea)
        graph.append({"startVa": hex(block.start_ea), "endVaExclusive": hex(block.end_ea),
                      "successors": [hex(succ.start_ea) for succ in block.succs()]})
    seen = set()
    pending = [0x802980]
    while pending:
        current = pending.pop()
        if current in seen or current == call_block:
            continue
        seen.add(current)
        pending.extend(succ.start_ea for succ in blocks[current].succs())
    return {"functionVa": "0x802980", "callVa": "0x803e70",
            "callBlockVa": hex(call_block) if call_block is not None else None,
            "normalReturnBlocks": [hex(value) for value in return_blocks],
            "returnsReachableWhenCallBlockExcluded": [hex(value) for value in return_blocks if value in seen],
            "blocks": graph,
            "limitations": "Normal statically recovered CFG only; does not prove arbitrary indirect reentry, exceptional exits or worker behavior."}


def export(output_file):
    banner = ida_bytes.get_strlit_contents(0xB7E1B4, -1, ida_nalt.STRTYPE_C)
    result = {
        "tx": "tx-01-66", "kind": "PRIMARY_STATIC_REREAD_AND_SUPPLEMENT",
        "source": {"inputPath": ida_nalt.get_input_file_path(),
                   "imageBase": hex(idaapi.get_imagebase()),
                   "bannerVa": "0xb7e1b4", "banner": banner.decode("utf-8") if banner else None},
        "originalEvidence": "ida-evidence.json (preserved)",
        "corrections": [
            "FSM 92FC77 and RoomChange 6FE37C are callsites; return addresses are 92FC7C and 6FE381 respectively.",
            "PositionOffset getter is 417310; setter is 4172F0. Original exporter comments had them reversed.",
            "Player VelocityBeforeUpdate getter returns the address of the inline vector at +1660; it does not load a pointer member.",
            "Slot4 has a flag-2 path that clears the bit and returns without staging or movement. Staging/movement is conditional.",
            "The literal argument of Room's 803E70 call is zero. Normal recovered Room return paths cannot bypass that call block.",
            "Full-room sampling is a proposed observer phase after native update but before the existing mutating TopExit callback; no capture implementation is delivered.",
        ],
        "functions": {}, "ranges": [], "roomNormalControlFlow": normal_room_cfg(),
        "floatConstants": [],
        "limitations": ["Static IDB reading only; no game, test, build, debugger or IDB mutation.",
                        "No hashes are computed or checked.",
                        "No exhaustive Position/Velocity writer or lifecycle-state closure is claimed."]}
    for ea, meaning in FUNCTIONS.items():
        func = ida_funcs.get_func(ea)
        entry = {"va": hex(ea), "rva": hex(ea - idaapi.get_imagebase()), "meaning": meaning}
        if func is None:
            entry["error"] = "No function in IDB"
        else:
            try:
                entry["pseudocode"] = ida_lines.tag_remove(str(ida_hexrays.decompile(ea)))
            except Exception as error:
                entry["decompileError"] = str(error)
            entry["instructions"] = []
            for begin, end in idautils.Chunks(func.start_ea):
                entry["instructions"].extend(instructions(begin, end))
        result["functions"][hex(ea)] = entry
    for begin, end, meaning in RANGES:
        result["ranges"].append({"startVa": hex(begin), "endVaExclusive": hex(end),
                                 "meaning": meaning, "instructions": instructions(begin, end)})
    import struct
    for ea in (0xBAA904, 0xBAAA00, 0xBAA2D0):
        data = ida_bytes.get_bytes(ea, 4)
        result["floatConstants"].append({"va": hex(ea), "bytes": data.hex(" "),
                                         "f32": struct.unpack("<f", data)[0]})
    with Path(output_file).open("x", encoding="utf-8", newline="\n") as handle:
        json.dump(result, handle, ensure_ascii=False, indent=2)
        handle.write("\n")
    print(json.dumps({"output": str(output_file), "functionCount": len(FUNCTIONS),
                      "rangeCount": len(RANGES), "status": "EXPORTED_STATIC_EVIDENCE"}))


if __name__ == "__main__":
    repository = Path(__file__).resolve().parents[4]
    export(repository / "artifacts" / "cs-t0" / "tx-01-66-static" / "primary-evidence.json")
