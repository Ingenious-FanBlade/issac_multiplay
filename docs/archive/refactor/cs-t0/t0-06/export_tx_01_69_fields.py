"""Read-only additional field-width and initialization evidence for tx-01-69."""

import json
from pathlib import Path
import runpy

import ida_nalt


HERE = Path(__file__).resolve().parent
helpers = runpy.run_path(str(HERE / "export_tx_01_67_review.py"), run_name="tx_01_69_field_helpers")
OUT = HERE.parents[3] / "artifacts" / "cs-t0" / "tx-01-69-static" / "field-evidence.json"
if OUT.exists():
    raise RuntimeError("Refusing to overwrite " + str(OUT))

evidence = {
    "task": "tx-01-69 field supplement",
    "input_path": ida_nalt.get_input_file_path(),
    "imagebase": helpers["address"](ida_nalt.get_imagebase()),
    "functions": [helpers["function"](ea) for ea in [0x4073C0, 0x43EEC0, 0x8C0620]],
    "regions": [helpers["region"](label, start, stop) for label, start, stop in [
        ("Tear Init four raw flag dwords", 0x6661D8, 0x66620A),
        ("Projectile Init two raw flag dwords", 0x652BE0, 0x652C1F),
        ("Entity Init two raw flag dwords", 0x6A96EB, 0x6A9710),
        ("Spawn room ordinal to Entity Index before publication", 0x6FE613, 0x6FE63B),
    ]],
    "notes": [
        "Only static IDB reads; all previous evidence retained.",
        "Initialization stores are narrow anchors, not final values after every derived Init, callback, Morph or restore.",
        "No code, tests, build, hashing or game actions executed.",
    ],
}
OUT.parent.mkdir(parents=True, exist_ok=True)
with OUT.open("x", encoding="utf-8", newline="\n") as stream:
    json.dump(evidence, stream, ensure_ascii=False, indent=2)
    stream.write("\n")
print("Created new field supplement:", OUT)
print("Functions:", len(evidence["functions"]), "regions:", len(evidence["regions"]))
