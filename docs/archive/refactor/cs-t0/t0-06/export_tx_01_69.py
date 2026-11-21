"""Read current IDA data for tx-01-69 offset reconciliation; create-new only."""

import json
from pathlib import Path
import runpy

import ida_bytes
import ida_nalt
import idc


HERE = Path(__file__).resolve().parent
helpers = runpy.run_path(str(HERE / "export_tx_01_67_review.py"), run_name="tx_01_69_helpers")
OUT = HERE.parents[3] / "artifacts" / "cs-t0" / "tx-01-69-static" / "ida-evidence.json"
BASE = ida_nalt.get_imagebase()
hx = helpers["address"]

FUNCTIONS = [
    0x866960,
    0x417260, 0x417270, 0x417280, 0x456370,
    0x6A8180, 0x435CD0,
    0x4173E0, 0x417400, 0x417420, 0x417430,
    0x417480, 0x44BF90, 0x417470,
    0x6A93A0, 0x418500, 0x6ACB00, 0x6F0090,
    0x4177C0, 0x676150, 0x417830, 0x447740,
    0x88AF40, 0x8C08F0, 0x8C0950,
    0x4E4830, 0x67EFC0, 0x709DB0, 0x709C40,
    0x4253B0, 0x417820,
]

VTABLES = [
    ("Entity", 0xB6730C), ("Player", 0xB6BDD0),
    ("NPC", 0xB67468), ("Tear", 0xB64EAC),
    ("Projectile", 0xB64990), ("Pickup", 0xB67F24),
    ("Familiar", 0xB6491C), ("Effect", 0xB63BDC),
]

if OUT.exists():
    raise RuntimeError("Refusing to overwrite " + str(OUT))

vtables = []
for label, va in VTABLES:
    slots = []
    for offset in [0, 4, 8, 0xC, 0x20, 0x24, 0x28, 0x44]:
        target = ida_bytes.get_dword(va + offset)
        slots.append({"offset": hx(offset), "slot_va": hx(va + offset), "target_va": hx(target), "target_rva": hx(target - BASE), "name": idc.get_func_name(target)})
    vtables.append({"class": label, "va": hx(va), "rva": hx(va - BASE), "raw_first_0x48_bytes": ida_bytes.get_bytes(va, 0x48).hex(" "), "slots": slots})

evidence = {
    "task": "tx-01-69 offset reconciliation",
    "input_path": ida_nalt.get_input_file_path(),
    "imagebase": hx(BASE),
    "banner_va": "0xB7E1B4",
    "banner": (ida_bytes.get_strlit_contents(0xB7E1B4, -1, idc.STRTYPE_C) or b"").decode("utf-8", "replace"),
    "functions": [helpers["function"](ea) for ea in FUNCTIONS],
    "vtables": vtables,
    "notes": [
        "Current IDB read only: no target code, build, tests, hashing or IDB changes.",
        "Full root registration captures class grouping; accessors establish raw widths and offsets.",
        "Prior tx-01-63 through tx-01-68 evidence is referenced for already-closed fields rather than overwritten or re-accepted.",
        "Vtable values are static data, never invoked; they are not global fixed targets for all derived classes.",
        "Shared formatting helper is loaded under a non-main run name, so its historical export does not execute.",
    ],
}
OUT.parent.mkdir(parents=True, exist_ok=True)
with OUT.open("x", encoding="utf-8", newline="\n") as stream:
    json.dump(evidence, stream, ensure_ascii=False, indent=2)
    stream.write("\n")
print("Created new evidence:", OUT)
print("Functions:", len(evidence["functions"]), "vtables:", len(vtables))
