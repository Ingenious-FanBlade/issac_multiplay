"""Read-only IDA export for tx-01-63; no game access, IDB edits, or checks.

Run with IDA MCP py_exec_file. To export again, call export(new_directory)
from IDA Python; the destination must not already exist. No hashes are read
or calculated. This is evidence extraction, not an acceptance test.
"""

import json
import struct
from pathlib import Path

import ida_bytes
import ida_funcs
import ida_hexrays
import ida_lines
import ida_nalt
import idaapi
import idautils
import idc


FUNCTIONS = {
    0x428B20: "Game::Spawn wrapper",
    0x6FE410: "Game::Spawn implementation",
    0x6F0090: "EntityFactory::Create",
    0x6F2A10: "EntityFactory pool construction",
    0x418500: "EntityList publication",
    0x417630: "EL push_back",
    0x417670: "EL remove matching pointer",
    0x86FB20: "native Isaac.GetRoomEntities",
    0x6A8290: "Entity base constructor",
    0x6A93A0: "Entity base Init",
    0x6ACB00: "Entity base Remove",
    0x6ACB60: "Entity base ClearReferences",
    0x866060: "native remove callback dispatcher",
    0x4186C0: "EntityList::Update",
    0x418230: "EntityList::Reset",
    0x417E50: "EntityList destructor",
    0x6A8820: "Entity scalar deleting destructor",
    0x6A8850: "Entity base destructor",
    0x754D60: "Player scalar deleting destructor",
    0x754ED0: "Player destructor",
    0x757A40: "Player ClearReferences override",
    0x9B9CD0: "ordinary local-coop Player allocation and publication",
    0x6F3060: "Game destruction including EntityFactory pools",
    0x9AB700: "Game destruction caller",
    0x7E9BA0: "Room destructor",
    0x6B8910: "NPC pool element destructor",
    0x60D400: "Familiar pool element destructor",
    0x5CDA90: "Effect pool element destructor",
    0x6E0160: "Pickup pool element destructor",
    0x665ED0: "Tear pool element destructor",
    0x436060: "Room::GetGridEntity",
    0x7EBCA0: "Room::SpawnGridEntity",
    0x81E980: "Room::RemoveGridEntityImmediate",
    0x70B2E0: "GridEntity allocation factory",
    0x709DB0: "GridEntity Init wrapper",
    0x709CB0: "GridEntity scalar deleting destructor",
    0x70B950: "Rock scalar deleting destructor example",
    0xAEF15C: "delete wrapper",
    0xA0F500: "allocator free wrapper",
    0xA648B0: "allocation accounting and free",
}

RANGES = [
    (0x803E62, 0x803E85),
    (0x86DE3A, 0x86DE4B),
    (0x6FE4EA, 0x6FE50A),
    (0x6FE5D4, 0x6FE63B),
    (0x6F31AB, 0x6F3346),
]


def instructions(start, end):
    result = []
    for ea in idautils.Heads(start, end):
        if not ida_bytes.is_code(ida_bytes.get_full_flags(ea)):
            continue
        size = ida_bytes.get_item_size(ea)
        raw = ida_bytes.get_bytes(ea, size)
        result.append({
            "va": hex(ea), "rva": hex(ea - idaapi.get_imagebase()),
            "bytes": raw.hex(" ") if raw is not None else None,
            "asm": ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or ""),
        })
    return result


def export(output_directory):
    destination = Path(output_directory)
    destination.mkdir(parents=True, exist_ok=False)
    input_path = Path(ida_nalt.get_input_file_path())
    with input_path.open("rb") as handle:
        header = handle.read(4096)
    pe = struct.unpack_from("<I", header, 0x3C)[0]
    result = {
        "tx": "tx-01-63", "evidence": "STATIC_CONFIRMED",
        "scope": "T0-06 operation 1 only; no runtime observations",
        "imageBase": hex(idaapi.get_imagebase()),
        "inputName": input_path.name,
        "peMetadataSource": "input file header; descriptive only",
        "machine": hex(struct.unpack_from("<H", header, pe + 4)[0]),
        "timestamp": hex(struct.unpack_from("<I", header, pe + 8)[0]),
        "sizeOfImage": hex(struct.unpack_from("<I", header, pe + 0x50)[0]),
        "referenceSession": "01a149e2-a48c-7cc2-883e-fd04d14e964d",
        "versionStrings": [], "functions": {}, "ranges": [], "vtables": [],
        "limitations": [
            "IDA pseudocode types and parameter names are not an ABI contract.",
            "Static input file and IDB correspondence was not hash-checked.",
            "No game process, debugger, hook, input, build or runtime test used.",
            "Xrefs are capped at 64 per function; more is explicitly recorded.",
        ],
    }
    for value in idautils.Strings():
        if "1.9.7." in str(value):
            result["versionStrings"].append({"va": hex(value.ea), "text": str(value)})
    for ea, meaning in FUNCTIONS.items():
        function = ida_funcs.get_func(ea)
        entry = {"meaning": meaning, "va": hex(ea), "rva": hex(ea - idaapi.get_imagebase())}
        if function is None:
            entry["error"] = "No function in IDB"
        else:
            try:
                entry["pseudocode"] = ida_lines.tag_remove(str(ida_hexrays.decompile(ea)))
            except Exception as error:
                entry["decompileError"] = str(error)
            entry["instructions"] = []
            for begin, end in idautils.Chunks(function.start_ea):
                entry["instructions"].extend(instructions(begin, end))
            refs = list(idautils.XrefsTo(ea, 0))
            entry["xrefsTruncated"] = len(refs) > 64
            entry["xrefs"] = [{"va": hex(ref.frm), "type": ref.type} for ref in refs[:64]]
        result["functions"][hex(ea)] = entry
    for begin, end in RANGES:
        result["ranges"].append({"startVa": hex(begin), "endVaExclusive": hex(end),
                                 "instructions": instructions(begin, end)})
    for ea, name in idautils.Names():
        if name in ("??_7Entity@IsaacRepentancePlus@@6B@",
                    "??_7Entity_Player@IsaacRepentancePlus@@6B@",
                    "??_7GridEntity@IsaacRepentancePlus@@6B@"):
            slots = (0, 1, 10, 17) if "Grid" not in name else (0, 8)
            result["vtables"].append({"va": hex(ea), "name": name,
                                      "slots": {str(i): hex(ida_bytes.get_dword(ea + 4 * i)) for i in slots}})
    output = destination / "ida-evidence.json"
    with output.open("x", encoding="utf-8", newline="\n") as handle:
        json.dump(result, handle, ensure_ascii=False, indent=2)
        handle.write("\n")
    print(json.dumps({"output": str(output), "functionCount": len(result["functions"]),
                      "rangeCount": len(result["ranges"]), "status": "EXPORTED_STATIC_EVIDENCE"}))


if __name__ == "__main__":
    repository = Path(__file__).resolve().parents[4]
    export(repository / "artifacts" / "cs-t0" / "tx-01-63-static")
