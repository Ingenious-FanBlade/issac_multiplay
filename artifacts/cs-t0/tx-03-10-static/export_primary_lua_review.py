"""Read the post-update/post-render Lua chain; no runtime or IDB writes."""

import importlib.util
from pathlib import Path

root_path = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location(
    "tx0310_primary_lua_export", root_path / "export_primary_review.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
module.FUNCTIONS = (0x8607A0, 0x863680, 0x872980)
module.RANGES = (
    (0x8607A0, 0x860801),
    (0x863680, 0x8636E1),
    (0x872980, 0x872A20),
)
module.export(globals().get(
    "TX0310_LUA_OUTPUT", root_path / "primary-lua-evidence.json"))
