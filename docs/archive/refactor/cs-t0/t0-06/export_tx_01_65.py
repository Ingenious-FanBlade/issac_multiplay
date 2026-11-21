"""Read-only IDA export for tx-01-65 HP layout research.

Run through IDA MCP py_exec_file. The exporter reads the open IDB and input
file metadata, and creates one new evidence directory. It does not edit the
IDB, access a game process, run tests, or calculate/check hashes.
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
    0x866960: "Entity and EntityPlayer Lua property/method registration",
    0x6A8290: "Entity base constructor",
    0x6A93A0: "Entity base Init",
    0x6A98B0: "Entity config loading path used by virtual slot 0x64",
    0x6BB790: "NPC config loading override used by virtual slot 0x64",
    0x417360: "Entity GetMaxHitPoints raw member getter",
    0x417370: "Entity GetHitPoints raw member getter",
    0x435D40: "Entity SetMaxHitPoints raw member setter",
    0x435D20: "Entity SetHitPoints raw member setter",
    0x6A9C30: "Entity HasFullHealth",
    0x6A9C50: "Entity AddHealth",
    0x6ACE60: "Entity TakeDamage base implementation",
    0x6AC920: "Entity Kill base implementation",
    0x755470: "Player Init",
    0x753DA0: "Player constructor",
    0x7729D0: "Player TakeDamage override",
    0x6B8B20: "NPC Init",
    0x6660A0: "Tear Init",
    0x652B90: "Projectile Init",
    0x6E02F0: "Pickup Init",
    0x60DE70: "Familiar Init",
    0x5CDB20: "Effect Init",
    0x709DB0: "GridEntity Init wrapper",
    0x428B10: "Player GetHearts direct getter",
    0x42A310: "Player GetMaxHearts direct getter",
    0x6DAD70: "Player GetEternalHearts direct getter",
    0x5B1740: "Player GetSoulHearts direct getter",
    0x5B1750: "Player GetBlackHearts direct getter",
    0x5B1700: "Player GetBoneHearts direct getter",
    0x5B1770: "Player GetGoldenHearts direct getter",
    0x65CF20: "Player GetBrokenHearts direct getter",
    0x5B1710: "Player GetRottenHearts direct getter",
    0x7CB060: "Player GetEffectiveMaxHearts computed helper",
    0x7CAFE0: "Player health type from PlayerType; this function does not route to another player",
    0x7588A0: "Player AddMaxHearts",
    0x758A70: "Player AddHearts",
    0x758C20: "Player AddEternalHearts",
    0x758D00: "Player AddSoulHearts",
    0x758F90: "Player AddBlackHearts",
    0x7CA840: "Player AddBoneHearts",
    0x7D2A40: "Player AddBrokenHearts",
    0x7D2FC0: "Player AddRottenHearts",
}

VTABLES = {
    "Entity": 0xB6730C,
    "Player": 0xB6BDD0,
    "NPC": 0xB67468,
    "Tear": 0xB64EAC,
    "Projectile": 0xB64990,
    "Pickup": 0xB67F24,
    "Familiar": 0xB6491C,
    "Effect": 0xB63BDC,
    "GridEntity": 0xB686E8,
}

VTABLE_SLOTS = (0, 1, 8, 9, 10, 25)

RANGES = [
    (0x6A9448, 0x6A94C4),   # Entity Init: virtual config call, current := max
    (0x755B05, 0x755B98),   # Player Init: base Init then base HP defaults
    (0x6B8D00, 0x6B8D24),   # NPC Init calls Entity Init
    (0x6660C8, 0x6661E8),   # Tear Init calls Entity Init; max HP set to zero
    (0x652B98, 0x652C98),   # Projectile Init calls Entity Init; max HP set to zero
    (0x5CDB60, 0x5CDB94),   # Effect Init calls Entity Init; max HP set to zero
    (0x60DEDC, 0x60DEF0),   # Familiar Init calls Entity Init
    (0x60EDF0, 0x60EE10),   # Familiar Init Variant 67 branch: 100/100
    (0x60F288, 0x60F2A0),   # Familiar Init Variant 100 branch: 9999/9999
    (0x60FD9C, 0x60FE10),   # Familiar Init later health scaling/writes
    (0x6BBC10, 0x6BBC52),   # NPC config branch scales MaxHitPoints
    (0x6A9C50, 0x6A9C84),   # Entity AddHealth reads max/current and writes current
    (0x6AD23C, 0x6AD27C),   # Entity TakeDamage max-health guard
    (0x758990, 0x758A20),   # Player AddMaxHearts direct heart fields
    (0x758B3C, 0x758B70),   # Player AddHearts direct heart fields
    (0x758C84, 0x758CF0),   # Player AddEternalHearts direct heart field
    (0x758E84, 0x758F7C),   # Player AddSoulHearts direct heart field
    (0x7CA8E8, 0x7CA9C8),   # Player AddBoneHearts direct field
    (0x7730E0, 0x7730F8),   # Player TakeDamage health-field compare
    (0x7771A0, 0x777280),   # Player TakeDamage eternal/soul/black/bone writes
    (0x7776B8, 0x777744),   # Player TakeDamage red-heart writes
]


def instructions(start, end):
    result = []
    for ea in idautils.Heads(start, end):
        if not ida_bytes.is_code(ida_bytes.get_full_flags(ea)):
            continue
        size = ida_bytes.get_item_size(ea)
        raw = ida_bytes.get_bytes(ea, size)
        result.append({
            "va": hex(ea),
            "rva": hex(ea - idaapi.get_imagebase()),
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
        "tx": "tx-01-65",
        "evidence": "STATIC_CONFIRMED",
        "scope": "T0-06 operation 3 HP subset; static IDA research only",
        "source": {
            "tool": "IDA Pro MCP py_exec_file",
            "inputPath": str(input_path),
            "idbPath": "F:\\SteamLibrary\\steamapps\\common\\The Binding of Isaac Rebirth\\isaac-ng.exe.i64 (task-specified)",
            "imageBase": hex(idaapi.get_imagebase()),
            "machine": hex(struct.unpack_from("<H", header, pe + 4)[0]),
            "timestamp": hex(struct.unpack_from("<I", header, pe + 8)[0]),
            "sizeOfImage": hex(struct.unpack_from("<I", header, pe + 0x50)[0]),
            "versionStrings": [],
        },
        "functions": {},
        "ranges": [],
        "vtables": [],
        "limitations": [
            "All function pseudocode and instructions are static observations from the open IDB.",
            "IDA pseudocode types and parameter names are not a callable ABI contract.",
            "No game process, debugger, hook, memory write, build, or test was used.",
            "No hashes were calculated or checked.",
            "No IDB names, comments, types, bytes, or structures were modified.",
        ],
    }

    for value in idautils.Strings():
        text = str(value)
        if "1.9.7." in text or text in (
            "MaxHitPoints", "HitPoints", "GetHearts", "GetMaxHearts",
            "GetEternalHearts", "GetSoulHearts", "GetBlackHearts",
            "GetBoneHearts", "GetGoldenHearts", "GetBrokenHearts",
            "GetRottenHearts", "AddMaxHearts", "AddHearts", "AddSoulHearts",
        ):
            result["source"]["versionStrings"].append({"va": hex(value.ea), "text": text})

    for ea, meaning in FUNCTIONS.items():
        function = ida_funcs.get_func(ea)
        entry = {
            "meaning": meaning,
            "va": hex(ea),
            "rva": hex(ea - idaapi.get_imagebase()),
        }
        if function is None:
            entry["error"] = "No function in IDB"
        else:
            try:
                pseudocode = ida_hexrays.decompile(ea)
                entry["pseudocode"] = ida_lines.tag_remove(str(pseudocode)) if pseudocode else None
            except Exception as error:
                entry["decompileError"] = str(error)
            entry["size"] = hex(function.end_ea - function.start_ea)
            entry["instructions"] = []
            for begin, end in idautils.Chunks(function.start_ea):
                entry["instructions"].extend(instructions(begin, end))
            refs = list(idautils.XrefsTo(ea, 0))
            entry["xrefsTruncated"] = len(refs) > 128
            entry["xrefs"] = [{"va": hex(ref.frm), "type": ref.type} for ref in refs[:128]]
        result["functions"][hex(ea)] = entry

    for begin, end in RANGES:
        result["ranges"].append({
            "startVa": hex(begin),
            "endVaExclusive": hex(end),
            "instructions": instructions(begin, end),
        })

    for name, base in VTABLES.items():
        slots = {}
        indices = range(0, 12) if name == "GridEntity" else VTABLE_SLOTS
        for index in indices:
            cell = base + index * 4
            target = ida_bytes.get_dword(cell)
            raw = ida_bytes.get_bytes(cell, 4)
            slots[str(index)] = {
                "cellVa": hex(cell),
                "cellRva": hex(cell - idaapi.get_imagebase()),
                "bytes": raw.hex(" ") if raw is not None else None,
                "targetVa": hex(target),
                "targetRva": hex(target - idaapi.get_imagebase()),
            }
        result["vtables"].append({"name": name, "va": hex(base), "slots": slots})

    output = destination / "ida-evidence.json"
    with output.open("x", encoding="utf-8", newline="\n") as handle:
        json.dump(result, handle, ensure_ascii=False, indent=2)
        handle.write("\n")

    print(json.dumps({
        "output": str(output),
        "functionCount": len(result["functions"]),
        "rangeCount": len(result["ranges"]),
        "vtableCount": len(result["vtables"]),
        "status": "EXPORTED_STATIC_EVIDENCE",
    }))


if __name__ == "__main__":
    repository = Path(__file__).resolve().parents[4]
    export(repository / "artifacts" / "cs-t0" / "tx-01-65-static")
