"""Additional read-only binding and Grid initialization evidence for tx-01-67."""

import json
from pathlib import Path
import runpy

import ida_bytes
import ida_nalt


HERE = Path(__file__).resolve().parent
helpers = runpy.run_path(str(HERE / "export_tx_01_67_review.py"), run_name="tx_01_67_helpers")
function = helpers["function"]
region = helpers["region"]
address = helpers["address"]
OUT = HERE.parents[3] / "artifacts" / "cs-t0" / "tx-01-67-static" / "binding-evidence.json"

if OUT.exists():
    raise RuntimeError("Refusing to overwrite " + str(OUT))

vtables = []
for label, va in [("Pickup", 0xB67F24), ("Familiar", 0xB6491C), ("Effect", 0xB63BDC)]:
    vtables.append({
        "class": label,
        "vtable_va": address(va),
        "rva": address(va - ida_nalt.get_imagebase()),
        "first_three_slots_raw": ida_bytes.get_bytes(va, 12).hex(" "),
        "init_slot_plus_4": address(ida_bytes.get_dword(va + 4)),
    })

evidence = {
    "task": "tx-01-67 binding and Grid initialization supplement",
    "imagebase": address(ida_nalt.get_imagebase()),
    "input_path": ida_nalt.get_input_file_path(),
    "functions": [function(ea) for ea in [0x866960, 0x709C40, 0x709DB0, 0x70B2E0]],
    "regions": [
        region("Player EAX zero and damage cooldown store", 0x755B50, 0x755C69),
        region("Entity start-frame source and adjacent unknown field", 0x6A9480, 0x6A94D0),
    ],
    "vtables": vtables,
    "notes": [
        "The three derived entity functions are Init at virtual slot +4, not C++ constructors.",
        "Grid base ctor writes State +0xC=0; later Init or restore may overwrite it.",
        "Grid Init sets byte +0x1C and stores current Game frame at +0x28.",
        "Only static IDB data was read; original evidence files were preserved.",
    ],
}
OUT.parent.mkdir(parents=True, exist_ok=True)
with OUT.open("x", encoding="utf-8", newline="\n") as stream:
    json.dump(evidence, stream, ensure_ascii=False, indent=2)
    stream.write("\n")
print("Created binding supplement:", OUT)
print("Functions:", len(evidence["functions"]), "regions:", len(evidence["regions"]), "vtables:", len(vtables))
