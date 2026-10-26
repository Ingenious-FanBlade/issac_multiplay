# -*- coding: utf-8 -*-
"""只读导出 REPENTOGON 目标映像中 ST4 的有限函数集合。"""
import idaapi
import ida_funcs
import ida_hexrays
import ida_lines
import ida_name
import ida_nalt
import idautils
import idc
import json
import os

# 这里的地址按 RVA 录入，再按当前 IDB imagebase 换成 VA；脚本不改 IDB。
ST4NEW_TARGETS = [
    ("Manager_ExecuteStartGame", 0x532A20),
    ("Manager_Update", 0x52E3E0),
    ("Menu_Game_Update", 0x559590),
    ("MenuManager_Update_pages", 0x561CB0),
    ("Manager_cleanup_current_state", 0x532970),
    ("Manager_ExecuteGoToMenu", 0x533060),
    ("Manager_LoadGameState", 0x5332F0),
    ("Manager_LoadGameState_menu_caller", 0x562E20),
    ("Manager_LoadGameState_persistent_wrapper", 0x5331C0),
    ("Manager_SaveGameState", 0x532160),
    ("Game_RestoreState", 0x2F43A0),
    ("Manager_start_game_helper", 0x2F1BF0),
    ("Game_RestoreState_other_caller_0x301AA0", 0x301AA0),
    ("Game_RestoreState_other_caller_0x4EA500", 0x4EA500),
    ("Game_SaveState", 0x2F51C0),
    ("PlayerManager_SaveGameState", 0x5940E0),
    ("Game_Exit", 0x2F6270),
    ("Level_RestoreGameState", 0x3415E0),
    ("PlayerManager_RestoreGameState", 0x593A90),
    ("PlayerManager_SyncPlayersFromGameState", 0x593780),
    ("PlayerManager_RestoreGameState_PostLevelInit", 0x593FA0),
    ("PlayerManager_SpawnCoPlayer2", 0x590250),
    ("PlayerManager_AutoReassignControllers", 0x5964A0),
    ("PlayerManager_Clear", 0x58F7C0),
    ("Player_RestoreGameState", 0x3A5660),
    ("Player_SetControllerId", 0x39E1B0),
]
ST4NEW_BASE = idaapi.get_imagebase()

def st4new_function(rva, label):
    """读取单个函数的反编译、逐条指令和直接引用，不创建或重命名对象。"""
    va = ST4NEW_BASE + rva
    fn = ida_funcs.get_func(va)
    item = {"label": label, "rva": hex(rva), "va": hex(va)}
    if not fn:
        item["error"] = "function_not_found_at_rva"
        return item
    item.update({
        "ida_name": ida_name.get_name(fn.start_ea),
        "func_start_va": hex(fn.start_ea),
        "func_start_rva": hex(fn.start_ea - ST4NEW_BASE),
        "func_end_va": hex(fn.end_ea),
        "func_end_rva": hex(fn.end_ea - ST4NEW_BASE),
    })
    try:
        item["decompile"] = str(ida_hexrays.decompile(fn))
    except Exception as exc:
        item["decompile_error"] = str(exc)
    asm = []
    direct_calls = []
    for ea in idautils.FuncItems(fn.start_ea):
        raw = idc.generate_disasm_line(ea, 0) or ""
        line = ida_lines.tag_remove(raw)
        asm.append({"va": hex(ea), "rva": hex(ea - ST4NEW_BASE), "text": line})
        if idc.print_insn_mnem(ea).lower() in ("call", "callq"):
            for target in idautils.CodeRefsFrom(ea, 0):
                target_fn = ida_funcs.get_func(target)
                direct_calls.append({
                    "site_va": hex(ea),
                    "site_rva": hex(ea - ST4NEW_BASE),
                    "target_va": hex(target),
                    "target_rva": hex(target - ST4NEW_BASE),
                    "target_name": ida_name.get_name(target_fn.start_ea) if target_fn else ida_name.get_name(target),
                    "target_is_function": bool(target_fn),
                })
    item["asm"] = asm
    item["direct_call_sites"] = direct_calls
    item["xrefs_to_function"] = [{
        "from_va": hex(x.frm),
        "from_rva": hex(x.frm - ST4NEW_BASE),
        "type": int(x.type),
        "from_name": ida_name.get_name(x.frm),
    } for x in idautils.XrefsTo(fn.start_ea, 0)]
    return item

# 保留有限目标清单，避免把这些直接调用伪装成完整的程序闭包。
ST4NEW_RESULT = {
    "kind": "read_only_static_export",
    "task": "tx-04-07 ST4",
    "input_path": ida_nalt.get_input_file_path(),
    "module": ida_nalt.get_root_filename(),
    "imagebase": hex(ST4NEW_BASE),
    "address_note": "rva is image-relative; va = imagebase + rva",
    "target_count": len(ST4NEW_TARGETS),
    "functions": [st4new_function(rva, label) for label, rva in ST4NEW_TARGETS],
}
ST4NEW_OUT = r"F:\projects_isaac\issac_multiplay\artifacts\repentogon\tx-04-07-static\st4-agent-current\ida-evidence.json"
with open(ST4NEW_OUT, "w", encoding="utf-8") as st4new_file:
    json.dump(ST4NEW_RESULT, st4new_file, ensure_ascii=False, indent=2)
print(json.dumps({"output": ST4NEW_OUT, "imagebase": ST4NEW_RESULT["imagebase"], "function_count": len(ST4NEW_RESULT["functions"]), "missing": [f["label"] for f in ST4NEW_RESULT["functions"] if f.get("error")]}, ensure_ascii=False))
