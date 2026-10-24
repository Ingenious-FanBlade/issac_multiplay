"""Additional primary static evidence for tx-03-10; no runtime or IDB writes.

Run through IDA MCP py_exec_file. Reuse the read-only exporter and write a
separate create-new artifact. Set TX0310_FOLLOWUP_OUTPUT in this execution
namespace to an explicit new file when repeating this extraction.
"""

import importlib.util
from pathlib import Path

root_path = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location(
    "tx0310_primary_export", root_path / "export_primary_review.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
module.FUNCTIONS = (
    0x8318A0, 0x408E00, 0x40A1B0, 0x40A5D0,
    0x419B50, 0x6FADC0, 0x9555C0, 0x954CD0,
)
module.RANGES = (
    (0x8318A0, 0x831960),
    (0x83199B, 0x831B17),
    (0x6FB500, 0x6FB530),
    (0x6FBA38, 0x6FBA85),
    (0x40A1B0, 0x40A216),
    (0x40A5D0, 0x40A690),
)
module.export(globals().get(
    "TX0310_FOLLOWUP_OUTPUT", root_path / "primary-followup-evidence.json"))
