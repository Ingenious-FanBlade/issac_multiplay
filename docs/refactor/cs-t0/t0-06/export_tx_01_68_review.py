"""Read-only current-IDB evidence for the primary review of tx-01-68.

The shared helper file has a guarded main entry: loading it with a different
run name reads definitions only and does not rerun tx-01-67's export.
"""

import json
from pathlib import Path
import runpy

import ida_bytes
import ida_nalt
import idc


HERE = Path(__file__).resolve().parent
helpers = runpy.run_path(str(HERE / "export_tx_01_67_review.py"), run_name="tx_01_68_helpers")
function = helpers["function"]
region = helpers["region"]
address = helpers["address"]
OUT = HERE.parents[3] / "artifacts" / "cs-t0" / "tx-01-68-static" / "primary-evidence.json"

FUNCTIONS = [
    0x505B70, 0x5B1590, 0x4174B0, 0x40C3A0,
    0x42A260, 0x431560, 0x4B7080, 0x4174A0,
    0x431430, 0x431440, 0x4361F0,
    0x6A92A0, 0x6A92E0, 0x6A9320, 0x6A9360, 0x62E020,
    0x6A8990,  # Computed LastParent candidate; do not call from sampler.
    0x4CF210, 0x4CDCF0, 0x6ACB60,
    0x709C40, 0x709DB0, 0x85DDA0,
    0x88A270, 0x8C0680, 0x8C06E0,
    0x866960,  # Full class/property grouping and registration offsets.
]

REGIONS = [
    ("Entity Init seed input and RNG initialization", 0x6A93A0, 0x6A9430),
    ("Entity constructor relation slots", 0x6A8740, 0x6A8780),
    ("Entity Init direct relation clears", 0x6A9730, 0x6A9780),
    ("Spawn source metadata reference and virtual Init", 0x6FE560, 0x6FE61A),
    ("Familiar Init owner source context", 0x60DEB0, 0x60DF40),
]


if __name__ == "__main__":
    if OUT.exists():
        raise RuntimeError("Refusing to overwrite " + str(OUT))
    evidence = {
        "task": "tx-01-68 primary static evidence",
        "input_path": ida_nalt.get_input_file_path(),
        "imagebase": address(ida_nalt.get_imagebase()),
        "banner_va": "0xB7E1B4",
        "banner": (ida_bytes.get_strlit_contents(0xB7E1B4, -1, idc.STRTYPE_C) or b"").decode("utf-8", "replace"),
        "functions": [function(ea) for ea in FUNCTIONS],
        "regions": [region(label, start, stop) for label, start, stop in REGIONS],
        "notes": [
            "Only current-IDB data read; no target code, runtime tests or IDB mutation.",
            "Candidate addresses are re-read as evidence, not assumed from historical names.",
            "This artifact preserves the subagent original; more evidence must use a new file.",
            "Reference values are observer-local data, not persistent or cross-process identity.",
        ],
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(evidence, stream, ensure_ascii=False, indent=2)
        stream.write("\n")
    print("Created primary evidence:", OUT)
    print("Functions:", len(evidence["functions"]), "regions:", len(evidence["regions"]))
