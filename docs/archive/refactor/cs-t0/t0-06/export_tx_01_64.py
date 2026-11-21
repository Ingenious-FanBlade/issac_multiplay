"""Export tx-01-64 static IDA evidence; no IDB or game-process mutation.

Run through IDA MCP py_exec_file. For another export, load this module and
call export(new_absolute_directory). Existing evidence is never overwritten.
The tx-01-63 script supplies only its static instruction formatting helper.
"""

import json
import runpy
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
    0x86FB20: "Native GetRoomEntities: updateEL source",
    0x418500: "EntityList publication and updateEL conditions",
    0x4186C0: "EntityList update/removal windows",
    0x419E70: "Spatial/collision processing and derived effectEL",
    0x6F0090: "EntityFactory type selection",
    0x6F2A10: "Pool construction: offsets, strides, classes",
    0x6F3060: "Pool destruction: ownership and strides",
    0x89D610: "Lua ToPlayer RTTI conversion",
    0x89D390: "Lua ToNPC RTTI conversion",
    0x89CC10: "Lua ToTear RTTI conversion",
    0x89CAD0: "Lua ToProjectile RTTI conversion",
    0x89D250: "Lua ToPickup RTTI conversion",
    0x89D110: "Lua ToFamiliar RTTI conversion",
    0x89D4D0: "Lua ToEffect RTTI conversion",
    0x87F8B0: "ToNPC registration",
    0x87FC70: "ToTear registration",
    0x8707B0: "Native FindByType Lua wrapper",
    0x41AF60: "QueryType: ordinary, cache and wisp branches",
    0x8704F0: "Native FindInRadius Lua wrapper",
    0x41A790: "QueryRadius: partitions, filters and scratch state",
    0x4253B0: "GetPlayerType: current offset",
    0x6DAE10: "GetSubPlayer: separate relation",
    0x417820: "GetOtherTwin: current offset",
    0x9B9CD0: "Coop Player creation, roster and ordinal",
    0x9BCAA0: "PlayerManager restore: backup and four Esau slots",
    0x9BEE80: "ReplacePlayer: temporary peer, backup and roster relations",
    0x7AC7E0: "Esau stored-player creation and replacement caller",
    0x60DE70: "Familiar Init: owner pointer",
    0x436060: "GetGridEntity: separate array and bounds",
    0x436020: "GetDoor: separate pointer view",
    0x436040: "Get door-array base",
    0x4176F0: "GetGridWidth",
    0x40C2E0: "GetGridHeight",
    0x4566A0: "GetGridSize: width times height",
    0x7EEA90: "MakeDoor: grid and door-slot aliases",
    0x70B2E0: "Grid factory: separate classes/type namespace",
    0x7E9BA0: "Room destructor: 448 grid slots",
    0x709DB0: "Grid initialization fields",
    0x6A93A0: "Entity Init: basic classification offsets",
}

RANGES = [
    (0x868FBB, 0x869033),
    (0x869997, 0x8699A8),
    (0x86A0A4, 0x86A0B5),
    (0x86A30D, 0x86A31E),
    (0x86C30C, 0x86C31D),
    (0x86C482, 0x86C4B4),
    (0x86DF6C, 0x86DF8E),
    (0x7ACA6C, 0x7ACAD0),
]


def rtti_catalog():
    catalog = []
    for ea, name in idautils.Names():
        if not (name.startswith("??_7Entity") or name.startswith("??_7GridEntity")):
            continue
        if "@IsaacRepentancePlus@@6B@" not in name:
            continue
        col = ida_bytes.get_dword(ea - 4)
        td = ida_bytes.get_dword(col + 12)
        chd = ida_bytes.get_dword(col + 16)
        base_count = ida_bytes.get_dword(chd + 8)
        base_array = ida_bytes.get_dword(chd + 12)
        bases = []
        for index in range(min(base_count, 32)):
            descriptor = ida_bytes.get_dword(base_array + index * 4)
            base_td = ida_bytes.get_dword(descriptor)
            raw_name = ida_bytes.get_strlit_contents(base_td + 8, -1, ida_nalt.STRTYPE_C)
            bases.append({"typeDescriptorVa": hex(base_td),
                          "name": raw_name.decode("utf-8", errors="replace") if raw_name else None})
        raw_name = ida_bytes.get_strlit_contents(td + 8, -1, ida_nalt.STRTYPE_C)
        catalog.append({
            "name": name, "va": hex(ea), "rva": hex(ea - idaapi.get_imagebase()),
            "colVa": hex(col), "typeDescriptorVa": hex(td),
            "typeName": raw_name.decode("utf-8", errors="replace") if raw_name else None,
            "hierarchyVa": hex(chd), "baseCount": base_count,
            "basesTruncated": base_count > 32, "bases": bases,
            "slots": {str(i): hex(ida_bytes.get_dword(ea + i * 4)) for i in (0, 1, 2, 3)},
        })
    return catalog


def export(output_directory):
    destination = Path(output_directory)
    destination.mkdir(parents=True, exist_ok=False)
    helper_file = Path(__file__).with_name("export_tx_01_63.py")
    helpers = runpy.run_path(str(helper_file), run_name="tx_01_64_static_helpers")
    instructions = helpers["instructions"]
    input_path = Path(ida_nalt.get_input_file_path())
    with input_path.open("rb") as handle:
        header = handle.read(4096)
    pe = struct.unpack_from("<I", header, 0x3C)[0]
    result = {
        "tx": "tx-01-64", "evidence": "STATIC_CONFIRMED",
        "scope": "T0-06 operation 2 static coverage and offset corrections; no runtime capture",
        "imageBase": hex(idaapi.get_imagebase()), "inputName": input_path.name,
        "machine": hex(struct.unpack_from("<H", header, pe + 4)[0]),
        "timestamp": hex(struct.unpack_from("<I", header, pe + 8)[0]),
        "sizeOfImage": hex(struct.unpack_from("<I", header, pe + 0x50)[0]),
        "referenceSession": "01a149e2-a48c-7cc2-883e-fd04d14e964d",
        "versionStrings": [], "functions": {}, "ranges": [],
        "limitations": [
            "All observations are static; no game process or debugger was used.",
            "Pseudocode types are not a callable ABI; no Lua/query/RTTI function was invoked in game.",
            "Xrefs are capped at 64; RTTI bases at 32; truncation is recorded.",
            "No hashes were calculated or checked.",
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
    result["rttiCatalog"] = rtti_catalog()
    for begin, end in RANGES:
        result["ranges"].append({"startVa": hex(begin), "endVaExclusive": hex(end),
                                 "instructions": instructions(begin, end)})
    output = destination / "ida-evidence.json"
    with output.open("x", encoding="utf-8", newline="\n") as handle:
        json.dump(result, handle, ensure_ascii=False, indent=2)
        handle.write("\n")
    print(json.dumps({"output": str(output), "functionCount": len(FUNCTIONS),
                      "rangeCount": len(RANGES), "rttiCount": len(result["rttiCatalog"]),
                      "status": "EXPORTED_STATIC_EVIDENCE"}))


if __name__ == "__main__":
    repository = Path(__file__).resolve().parents[4]
    export(repository / "artifacts" / "cs-t0" / "tx-01-64-static")
