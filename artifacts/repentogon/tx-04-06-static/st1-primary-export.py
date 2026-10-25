"""主智能体通过 IDA MCP 只读导出 ST1 的原始表、指令与伪码，不运行目标。"""

import json
from pathlib import Path

import ida_bytes
import ida_funcs
import ida_hexrays
import ida_lines
import ida_name
import ida_nalt
import idaapi
import idautils


def export_function(rva):
    """以原始指令为准保存函数；保留反编译异常及 IDA 的类型推断。"""
    va = base + rva
    function = ida_funcs.get_func(va)
    if function is None:
        return f"\n[FUNCTION RVA {rva:#x}] NO_FUNCTION\n"
    lines = [f"\n[FUNCTION RVA {rva:#x} VA {va:#x} {ida_name.get_name(va)}]"]
    try:
        lines.extend(["[PSEUDOCODE]", str(ida_hexrays.decompile(va))])
    except Exception as error:
        lines.append(f"DECOMPILE_ERROR {error!r}")
    lines.append("[DISASSEMBLY]")
    for ea in idautils.FuncItems(function.start_ea):
        size = ida_bytes.get_item_size(ea)
        raw = ida_bytes.get_bytes(ea, size)
        listing = ida_lines.tag_remove(ida_lines.generate_disasm_line(ea, 0) or "")
        lines.append(f"RVA {ea-base:#010x} VA {ea:#010x} {(raw or b'').hex(' '):<30} {listing}")
    lines.append("[XREFS_TO_ENTRY]")
    for xref in idautils.XrefsTo(va):
        lines.append(f"RVA {xref.frm-base:#x} type {xref.type}")
    return "\n".join(lines) + "\n"


# 只记录可读路径、基址和表内容；不读取、计算或校验哈希。
base = idaapi.get_imagebase()
out = Path(r"F:\projects_isaac\issac_multiplay\artifacts\repentogon\tx-04-06-static")
out.mkdir(parents=True, exist_ok=True)
metadata = {"imagebase": hex(base), "input_path": ida_nalt.get_input_file_path(),
            "idb_path": idaapi.get_path(idaapi.PATH_TYPE_IDB)}
tables = {}
targets = set()
for label, rva in (("base", 0x732F04), ("keyboard", 0x73B490), ("gamepad", 0x73B3F0)):
    rows = []
    for slot in range(38):
        va = ida_bytes.get_dword(base + rva + slot * 4)
        rows.append({"slot": slot, "offset": hex(slot*4), "target_rva": hex(va-base),
                     "name": ida_name.get_name(va)})
        if label == "base" or slot in (0, 1, 2, 3, 4, 35, 36):
            targets.add(va-base)
    tables[label] = {"rva": hex(rva), "rows": rows}
(out / "st1-primary-tables.json").write_text(
    json.dumps({"metadata": metadata, "tables": tables}, ensure_ascii=False, indent=2), encoding="utf-8")

# 主审独立读取公共更新、查询、移除与 Join；不建立新的函数或修改 IDB。
targets.update((0x5EC570, 0x5EC900, 0x5EDCC0, 0x5EE140, 0x5EE200, 0x5EE2E0,
                0x5EEB40, 0x5EEBB0, 0x5EECA0, 0x591360, 0x5EDAA0,
                0x4B8C70, 0x4B9AD0, 0x561CB0, 0x52F610, 0x52F6D0, 0x52F7B0))
# 补齐 InputManager 分派包装层，避免将其锁对象的相同偏移误判为设备槽。
targets.update(ea-base for ea in idautils.Functions(base+0x5EDBD0, base+0x5EEB40))
parts = [json.dumps(metadata, ensure_ascii=False, indent=2)]
parts.extend(export_function(rva) for rva in sorted(targets))
(out / "st1-primary-ida-evidence.txt").write_text("\n".join(parts), encoding="utf-8")
# 槽 18 的基类实现从该全局复制两个 DWORD；记录实际数据而不以错误伪码猜类型。
(out / "st1-primary-vector-default.json").write_text(json.dumps({
    "rva": "0x7eb900", "bytes": ida_bytes.get_bytes(base+0x7EB900, 8).hex(" ")
}, indent=2), encoding="utf-8")
print(json.dumps({"output": str(out), "function_count": len(targets)}, ensure_ascii=False))
