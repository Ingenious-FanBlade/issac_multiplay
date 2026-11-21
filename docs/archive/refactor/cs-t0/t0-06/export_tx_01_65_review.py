"""Primary-agent static reread and supplemental HP evidence; never a game test.

Run using IDA MCP py_exec_file. Original subagent evidence is preserved.
An existing output file is never overwritten. No IDB or process is mutated.
"""

import json
import runpy
from pathlib import Path

import ida_bytes
import ida_funcs
import ida_hexrays
import ida_lines
import ida_nalt
import idaapi
import idautils


FUNCTIONS = {
    0x417360: "MaxHitPoints getter: f32 +384",
    0x417370: "HitPoints getter: f32 +380",
    0x435D20: "HitPoints direct setter",
    0x435D40: "MaxHitPoints direct setter",
    0x428B10: "GetHearts: raw +1344",
    0x42A310: "GetMaxHearts: raw +1340",
    0x6DAD70: "GetEternalHearts: raw +1348",
    0x5B1740: "GetSoulHearts: raw +134C",
    0x5B1750: "GetBlackHearts: raw +1350",
    0x5B1700: "GetBoneHearts: raw +1D88",
    0x5B1770: "GetGoldenHearts: raw +194C",
    0x65CF20: "GetBrokenHearts: raw +1DA0",
    0x5B1710: "GetRottenHearts: raw +1DA4",
    0x7CAFE0: "Get health type from PlayerType; no player pointer redirection",
    0x7CB060: "EffectiveMaxHearts: same object, MaxHearts plus optional 2*BoneHearts",
    0x6AE820: "Entity Update: queued damage is applied to HP, then possible virtual Kill",
    0x6B7DB0: "Append 56-byte damage record to Entity+8 vector",
}

RANGES = [
    (0x868C32, 0x868C5C, "Named MaxHitPoints/HitPoints property registration"),
    (0x8698CB, 0x86992D, "Player named raw-heart getter registration"),
    (0x86A01C, 0x86A04D, "Bone and effective-max getter registration"),
    (0x6A9448, 0x6A94C4, "Config load before HP := MaxHP"),
    (0x6A9980, 0x6A999C, "Config-derived MaxHP write"),
    (0x755B05, 0x755BF9, "Player base Init then HP/MaxHP=1 and hearts reset"),
    (0x60EDF0, 0x60EE10, "Familiar HP/MaxHP = 100"),
    (0x60F288, 0x60F2A0, "Familiar HP/MaxHP = 9999"),
    (0x6AD24D, 0x6AD2CD, "Base damage guard, callback, queue append, pending sum"),
    (0x6AF54C, 0x6AF77B, "Entity Update damage queue read and HP subtraction"),
    (0x6AF89B, 0x6AF8E1, "Queued-death virtual Kill and queue/pending reset"),
    (0x758B3C, 0x758B70, "AddHearts direct red-heart clamp/write"),
    (0x7590D0, 0x75916A, "AddBlackHearts bitmap updates then AddSoulHearts"),
    (0x7776B8, 0x777744, "Player damage red-heart writes"),
]


def export(output_file):
    helpers = runpy.run_path(str(Path(__file__).with_name("export_tx_01_65.py")),
                             run_name="tx_01_65_review_helpers")
    instructions = helpers["instructions"]
    result = {
        "tx": "tx-01-65",
        "kind": "PRIMARY_STATIC_REREAD_AND_SUPPLEMENT",
        "scope": "HP subset only; analysis evidence, not a runtime test or acceptance run",
        "source": {"inputPath": ida_nalt.get_input_file_path(),
                   "imageBase": hex(idaapi.get_imagebase())},
        "originalEvidence": "ida-evidence.json (preserved)",
        "corrections": [
            "Original annotation calling 0x7CAFE0 a related-player routing helper is incorrect; it only switches on PlayerType.",
            "Hex-Rays rax/HIDWORD rendering in 0x7CB060 is not a real 64-bit return/pointer route: EDX is saved ECX; only EAX health type is returned.",
            "Base TakeDamage queues damage; actual HP subtraction is in Entity Update. A successful damage return is not a post-damage HP sampling boundary.",
            "Familiar constant 0x461C3C00 decodes to 9999.0, correcting the primary agent's early oral 10000 estimate.",
        ],
        "userAcceptedAssumption": "Each Entity-derived category uses common HP fields as life; this semantic assumption is not a STATIC_CONFIRMED result. GridEntity has an independent layout.",
        "functions": {}, "ranges": [],
        "limitations": ["Static IDB read only; no calls into the game.",
                        "No hashes, tests, builds, debugger, hook or IDB mutation.",
                        "No exhaustive writer, special-character or all-grid-health analysis."],
    }
    for ea, meaning in FUNCTIONS.items():
        func = ida_funcs.get_func(ea)
        entry = {"meaning": meaning, "va": hex(ea),
                 "rva": hex(ea - idaapi.get_imagebase())}
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
                                 "meaning": meaning,
                                 "instructions": instructions(begin, end)})
    result["entityUpdateVtableCell"] = {
        "va": "0xb67318", "targetVa": hex(ida_bytes.get_dword(0xB67318)),
        "bytes": ida_bytes.get_bytes(0xB67318, 4).hex(" ")}
    with Path(output_file).open("x", encoding="utf-8", newline="\n") as handle:
        json.dump(result, handle, ensure_ascii=False, indent=2)
        handle.write("\n")
    print(json.dumps({"output": str(output_file), "functions": len(FUNCTIONS),
                      "ranges": len(RANGES), "status": "EXPORTED_STATIC_EVIDENCE"}))


if __name__ == "__main__":
    repository = Path(__file__).resolve().parents[4]
    export(repository / "artifacts" / "cs-t0" / "tx-01-65-static" / "primary-evidence.json")
