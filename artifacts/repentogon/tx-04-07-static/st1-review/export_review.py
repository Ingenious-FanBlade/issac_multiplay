"""tx-04-07 ST1 主审原始静态导出；仅读取 IDB，不执行目标或修改数据库。"""
import json
from pathlib import Path
import idaapi
import ida_bytes
import ida_funcs
import ida_hexrays
import ida_lines
import ida_nalt
import idautils
import idc


def export_st1_review():
    """保存函数反编译、逐条指令和调用引用，供人工复核分支与证据边界。"""
    out = Path(r"F:\projects_isaac\issac_multiplay\artifacts\repentogon\tx-04-07-static\st1-review")
    out.mkdir(parents=True, exist_ok=True)
    base = ida_nalt.get_imagebase()
    targets = {
        "Entity_Kill": 0x6A7ED0,
        "PlayerManager_Update": 0x992650,
        "Player_TriggerDeath": 0x7992A0,
        "Player_MorphToCoopGhost": 0x7CFFA0,
        "Player_Revive": 0x79A400,
        "PlayerManager_IsCoopPlay": 0x996990,
        "PlayerManager_CountNonGhost": 0x996A00,
        "Player_TakeDamage": 0x76B900,
        "Player_Update": 0x77BD40,
        "PlayerHUD_Update": 0x832C10,
    }
    meta = {"idb": idaapi.get_path(idaapi.PATH_TYPE_IDB),
            "input": ida_nalt.get_input_file_path(), "imagebase": hex(base),
            "kind": "read_only_static_export", "functions": []}
    for label, ea in targets.items():
        fn = ida_funcs.get_func(ea)
        entry = {"label": label, "va": hex(ea), "rva": hex(ea-base)}
        if fn is None:
            entry["error"] = "function_not_defined"
        else:
            entry["end_va"] = hex(fn.end_ea)
            try:
                entry["decompile"] = str(ida_hexrays.decompile(ea))
            except Exception as err:
                entry["decompile_error"] = str(err)
            entry["instructions"] = [
                {"va": hex(ins), "rva": hex(ins-base),
                 "bytes": (ida_bytes.get_bytes(ins, idc.get_item_size(ins)) or b"").hex(" "),
                 "text": ida_lines.tag_remove(idc.generate_disasm_line(ins, 0) or "")}
                for ins in idautils.FuncItems(ea)]
            entry["xrefs_to"] = [
                {"from_va": hex(x.frm), "from_rva": hex(x.frm-base), "type": x.type}
                for x in idautils.XrefsTo(ea, 0)]
        (out / (label + ".json")).write_text(json.dumps(entry, ensure_ascii=False, indent=2), encoding="utf-8")
        meta["functions"].append({k: v for k, v in entry.items() if k not in ("decompile", "instructions", "xrefs_to")})
    (out / "metadata.json").write_text(json.dumps(meta, ensure_ascii=False, indent=2), encoding="utf-8")
    return {"directory": str(out), "function_count": len(targets)}


print(json.dumps(export_st1_review(), ensure_ascii=False))
