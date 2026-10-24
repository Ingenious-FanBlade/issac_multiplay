"""Read current IDA instructions for the primary review of tx-01-67.

This creates a separate evidence file. It does not run target code, change the
IDB, calculate hashes, or overwrite the subagent's original evidence.
"""

import json
from pathlib import Path

import ida_bytes
import ida_funcs
import ida_hexrays
import ida_lines
import ida_nalt
import idautils
import idc


OUT = Path(r"D:\projects\isaac_hook_reserach\artifacts\cs-t0\tx-01-67-static\primary-evidence.json")
BASE = ida_nalt.get_imagebase()


def address(value):
    return "0x%X" % value


def instruction(ea):
    size = idc.get_item_size(ea)
    return {
        "va": address(ea),
        "rva": address(ea - BASE),
        "bytes": (ida_bytes.get_bytes(ea, size) or b"").hex(" "),
        "text": ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or ""),
    }


def function(ea):
    owner = ida_funcs.get_func(ea)
    if owner is None:
        return {"requested_va": address(ea), "error": "missing function"}
    result = {
        "start_va": address(owner.start_ea),
        "name": idc.get_func_name(owner.start_ea),
        "instructions": [],
    }
    try:
        result["pseudocode"] = str(ida_hexrays.decompile(owner.start_ea))
    except Exception as exc:
        result["decompile_error"] = str(exc)
    for item in idautils.FuncItems(owner.start_ea):
        if ida_bytes.is_code(ida_bytes.get_full_flags(item)):
            result["instructions"].append(instruction(item))
    return result


def region(label, start, stop):
    result = {"label": label, "start_va": address(start), "end_va_exclusive": address(stop), "instructions": []}
    for item in idautils.Heads(start, stop):
        if ida_bytes.is_code(ida_bytes.get_full_flags(item)):
            result["instructions"].append(instruction(item))
    return result


FUNCTIONS = [
    0x6A9C10,  # FrameCount computes Game frame minus Entity start frame.
    0x5B1940, 0x85E440, 0x60D080, 0x4176F0,
    0x85BFD0, 0x85BFF0,  # Lua signed / unsigned integer presentation.
    0x8BEDB0, 0x8BEF40, 0x8BF210, 0x8BF5B0, 0x8BF920, 0x8C05C0,
    0x884EA0, 0x8854A0, 0x885F80, 0x8877E0, 0x887AD0, 0x88A100,
]

REGIONS = [
    ("Entity Init start-frame baseline", 0x6A9490, 0x6A94C5),
    ("Player Init damage-cooldown baseline", 0x755C40, 0x755C80),
    ("NPC Init State and StateFrame", 0x6B8D20, 0x6B8D60),
    ("NPC Init projectile timers", 0x6B8E20, 0x6B8E55),
    ("Pickup initial Timeout and Wait", 0x6E0340, 0x6E0375),
    ("Pickup initial State", 0x6E03B0, 0x6E03E0),
    ("Familiar initial FireCooldown", 0x60DFB0, 0x60DFE0),
    ("Familiar initial State", 0x60E290, 0x60E2C5),
    ("Effect initial State Timeout LifeSpan", 0x5CDB90, 0x5CDBE0),
]


if __name__ == "__main__":
    if OUT.exists():
        raise RuntimeError("Refusing to overwrite " + str(OUT))
    evidence = {
        "task": "tx-01-67 primary static review evidence",
        "input_path": ida_nalt.get_input_file_path(),
        "imagebase": address(BASE),
        "banner_va": "0xB7E1B4",
        "banner": (ida_bytes.get_strlit_contents(0xB7E1B4, -1, idc.STRTYPE_C) or b"").decode("utf-8", "replace"),
        "functions": [function(ea) for ea in FUNCTIONS],
        "regions": [region(label, start, stop) for label, start, stop in REGIONS],
        "limits": [
            "Current-IDB static reads only; no target execution or runtime observation.",
            "Initialization anchors are limited observations, not exhaustive reset or writer coverage.",
            "Raw 32-bit storage is separate from Lua signed/unsigned presentation and gameplay meaning.",
        ],
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(evidence, stream, ensure_ascii=False, indent=2)
        stream.write("\n")
    print("Created separate primary evidence:", OUT)
    print("Functions:", len(evidence["functions"]), "regions:", len(evidence["regions"]))
