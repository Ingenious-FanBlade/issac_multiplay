"""主智能体只读导出 ST2–ST9；保存原始指令与源码分析依据，不执行目标。"""

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
    """导出实际函数边界、伪码、原字节和交叉引用，保留请求点与起点差异。"""
    va = base + rva
    function = ida_funcs.get_func(va)
    if not function:
        return {"requested_rva": hex(rva), "error": "NO_FUNCTION"}
    result = {"requested_rva": hex(rva), "start_rva": hex(function.start_ea-base),
              "end_rva": hex(function.end_ea-base), "name": ida_name.get_name(function.start_ea)}
    try:
        result["pseudocode"] = str(ida_hexrays.decompile(function.start_ea))
    except Exception as error:
        result["decompile_error"] = repr(error)
    result["instructions"] = []
    for ea in idautils.FuncItems(function.start_ea):
        result["instructions"].append({
            "rva": hex(ea-base), "bytes": (ida_bytes.get_bytes(ea, ida_bytes.get_item_size(ea)) or b"").hex(" "),
            "text": ida_lines.tag_remove(ida_lines.generate_disasm_line(ea, 0) or "")})
    result["xrefs_to_entry"] = [{"from_rva": hex(x.frm-base), "type": x.type}
                               for x in idautils.XrefsTo(function.start_ea)]
    return result


# 本轮沿用已打开的目标 IDB，只核对路径/基址，不读或计算哈希。
base = ida_nalt.get_imagebase()
out = Path(r"F:\projects_isaac\issac_multiplay\artifacts\repentogon\tx-04-06-static")
targets = {
    0x5EC040, 0x5EC110, 0x5EC170, 0x5EC2E0, 0x5EC450, 0x5EDAA0,
    0x5EEB40, 0x5EEBB0, 0x5EECA0, 0x5EDCC0, 0x5EC570,
    0x591360, 0x5910A0, 0x590250, 0x590560, 0x592650,
    0x4B8C70, 0x4B9AD0, 0x561CB0, 0x4B84B0,
    0x2F6270, 0x532A20, 0x5328D0,
}
# 后续只根据本轮调用图发现的实际目标补充，不扫描无关的整个虚调用空间。
supplement = out / "st2-st9-primary-targets.json"
if supplement.exists():
    targets.update(int(value, 0) for value in json.loads(supplement.read_text(encoding="utf-8-sig")))
export_data = {"metadata": {"imagebase": hex(base), "input_path": ida_nalt.get_input_file_path(),
                       "idb_path": idaapi.get_path(idaapi.PATH_TYPE_IDB)},
          "functions": {hex(rva): export_function(rva) for rva in sorted(targets)}}
# 保存本轮实际用到的全局字段引用；引用数量不代表全映像别名闭包。
export_data["global_xrefs"] = {}
for rva in (0x788A14, 0x7CE438, 0x7CE44C, 0x7CE458, 0x7E65EC):
    export_data["global_xrefs"][hex(rva)] = [
        {"from_rva": hex(x.frm-base), "type": x.type,
         "text": ida_lines.tag_remove(ida_lines.generate_disasm_line(x.frm, 0) or "")}
        for x in idautils.XrefsTo(base+rva)]
# InputManager 正好 47 个函数槽；紧邻的下一项为另一张表的 RTTI COL。
export_data["input_manager_table"] = [
    {"slot": index, "rva": hex(ida_bytes.get_dword(base+0x732E44+4*index)-base)}
    for index in range(47)]
export_data["input_manager_after_table"] = hex(ida_bytes.get_dword(base+0x732E44+4*47)-base)
(out / "st2-st9-primary-ida.json").write_text(json.dumps(export_data, ensure_ascii=False, indent=2), encoding="utf-8")
result = None
print(json.dumps({"output": str(out / "st2-st9-primary-ida.json"), "functions": len(targets)}))
