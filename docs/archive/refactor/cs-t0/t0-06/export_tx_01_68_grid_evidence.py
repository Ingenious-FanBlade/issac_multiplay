"""Read-only Grid.Desc binding supplement for tx-01-68; creates a new file."""

import json
from pathlib import Path
import runpy

import ida_nalt


HERE = Path(__file__).resolve().parent
helpers = runpy.run_path(str(HERE / "export_tx_01_67_review.py"), run_name="tx_01_68_grid_helpers")
OUT = HERE.parents[3] / "artifacts" / "cs-t0" / "tx-01-68-static" / "grid-evidence.json"

if OUT.exists():
    raise RuntimeError("Refusing to overwrite " + str(OUT))
evidence = {
    "task": "tx-01-68 direct Grid.Desc binding supplement",
    "input_path": ida_nalt.get_input_file_path(),
    "imagebase": helpers["address"](ida_nalt.get_imagebase()),
    "functions": [helpers["function"](ea) for ea in [0x88ADA0, 0x8C07B0]],
    "notes": [
        "Grid.Desc registration captures offset +4, then copies two 16-byte blocks from that subobject into a Lua-owned value.",
        "The sampler must copy native bytes directly and must not invoke this allocating Lua getter.",
        "Grid.GetRNG +0x2C evidence and full root class context are separately preserved in primary-evidence.json.",
        "Static reads only; no target code execution or IDB mutation; no previous evidence overwritten.",
    ],
}
OUT.parent.mkdir(parents=True, exist_ok=True)
with OUT.open("x", encoding="utf-8", newline="\n") as stream:
    json.dump(evidence, stream, ensure_ascii=False, indent=2)
    stream.write("\n")
print("Created Grid supplement:", OUT)
print("Functions:", len(evidence["functions"]))
