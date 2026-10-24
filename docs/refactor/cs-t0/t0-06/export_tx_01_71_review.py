"""Create-new current-IDB static evidence for tx-01-71 parent analysis.

No target execution, IDB mutation, hashing, build or tests.
"""
import json
from pathlib import Path
import runpy

import ida_bytes
import ida_funcs
import ida_nalt
import idautils
import idc

HERE = Path(__file__).resolve().parent
helpers = runpy.run_path(str(HERE / "export_tx_01_67_review.py"), run_name="tx_01_71_read_helpers")
OUT = HERE.parents[3] / "artifacts" / "cs-t0" / "tx-01-71-static" / "primary-evidence.json"

FUNCTIONS = [
    0x4186C0, 0x418230, 0x417E50, 0x41C7D0, 0x41C8E0,
    0x6EEF60, 0x6EF1F0, 0x683410, 0x683580,
    0x90D640, 0x90D950, 0x910980, 0x90B150,
    0x91B130, 0x91ACC0, 0xA89DE0, 0x902950,
    0x9015B0, 0x901B70, 0x8FE3A0, 0x904E70,
    0x6C0F30, 0x6E30A0, 0x7AE4B0, 0x7B0070,
    0x9B9CD0, 0x9BCAA0, 0x9BEE80, 0x753DA0,
    0x6F0090, 0x6A93A0, 0x418500, 0x6ACB00, 0x6ACB60, 0x6A8850,
    0x70B2E0, 0x709DB0, 0x7EBCA0, 0x7EEA90, 0x81E980,
]

ENTRY_CANDIDATES = [
    0x6F0090, 0x6A93A0, 0x418500, 0x6ACB00, 0x6ACB60, 0x6A8850,
    0x753DA0, 0x9B9CD0, 0x9BCAA0, 0x9BEE80, 0x7AE4B0,
    0x6C0F30, 0x6E30A0,
    0x70B2E0, 0x709DB0, 0x7EBCA0, 0x7EEA90, 0x81E980,
]

REGIONS = [
    ("EntityList online-condition and 32-slot detailed history", 0x418701, 0x418790),
    ("EntityList actual per-entity Update", 0x418920, 0x418980),
    ("EntityList field feeds and 56-byte entry", 0x418BE0, 0x418D65),
    ("EntityList final state checksums", 0x4196A0, 0x419745),
    ("Online serializer input domains", 0x90D720, 0x90D790),
    ("Serialized-online-state CRC and summary payload", 0x90D835, 0x90D90B),
    ("Manager frame field is compared, not written", 0x959984, 0x9599B7),
    ("NPC Morph classification and no-op", 0x6C0F30, 0x6C0FC0),
    ("Pickup Morph KeepSeed and InitSeed", 0x6E30DC, 0x6E3180),
]


def entry_candidate(ea):
    rows = []
    cursor = ea
    while cursor - ea < 5:
        if not ida_bytes.is_code(ida_bytes.get_full_flags(cursor)):
            return {"va": hex(ea), "error": "non-code item at candidate entry", "items": rows}
        rows.append(helpers["instruction"](cursor))
        cursor += idc.get_item_size(cursor)
    return {"va": hex(ea), "rva": hex(ea - ida_nalt.get_imagebase()),
            "span_bytes": cursor - ea, "items": rows,
            "note": "Instruction-aligned candidate only; existing RelativeCallPatch cannot install an entry detour."}


def callers(ea):
    rows = []
    truncated = False
    for ref in idautils.XrefsTo(ea, 0):
        if len(rows) >= 512:
            truncated = True
            break
        owner = ida_funcs.get_func(ref.frm)
        rows.append({"from_va": hex(ref.frm), "xref_type": int(ref.type),
                     "owner_va": hex(owner.start_ea) if owner else None,
                     "instruction": helpers["instruction"](ref.frm) if ref.iscode else None})
    return {"target_va": hex(ea), "rows": rows, "truncated": truncated}


if __name__ == "__main__":
    if OUT.exists():
        raise RuntimeError("Refusing to overwrite " + str(OUT))
    evidence = {
        "tx": "tx-01-71", "kind": "PRIMARY_STATIC_ANALYSIS_AND_REVIEW",
        "input_path": ida_nalt.get_input_file_path(),
        "imagebase": hex(ida_nalt.get_imagebase()),
        "banner": (ida_bytes.get_strlit_contents(0xB7E1B4, -1, idc.STRTYPE_C) or b"").decode("utf-8", "replace"),
        "functions": [helpers["function"](ea) for ea in FUNCTIONS],
        "regions": [helpers["region"](label, start, stop) for label, start, stop in REGIONS],
        "entry_candidates": [entry_candidate(ea) for ea in ENTRY_CANDIDATES],
        "entry_callers": [callers(ea) for ea in ENTRY_CANDIDATES],
        "limits": [
            "Static IDB reads only, no native function is executed.",
            "Copied entry bytes are not a patch installation or runtime relocation proof.",
            "Legacy evidence and the subagent's original snapshot remain unchanged.",
        ],
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(evidence, stream, ensure_ascii=False, indent=2)
        stream.write("\n")
    print("Created", OUT, "functions", len(evidence["functions"]), "regions", len(evidence["regions"]))
