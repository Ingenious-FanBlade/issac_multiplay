"""tx-04-07 ST3/ST4 主审静态导出；只读 IDB，不执行目标、不改分析状态。"""
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


def export_review():
    """完整保留函数汇编和反编译；标签仅为说明，不写入 IDB。"""
    out = Path(r"F:\projects_isaac\issac_multiplay\artifacts\repentogon\tx-04-07-static\st3-st4-review")
    out.mkdir(parents=True, exist_ok=True)
    base = ida_nalt.get_imagebase()
    targets = {
        "Game_StartStageTransition": 0x6F9B30,
        "StageTransition_Update": 0x909100,
        "StageTransition_Reset": 0x908F30,
        "Game_Update": 0x6F6D30,
        "Game_OddHelper": 0x6F9340,
        "Game_ProcessInput": 0x6F66E0,
        "Game_IsPaused": 0x6F92A0,
        "Manager_Update": 0x92E3E0,
        "Manager_ExecuteStartGame": 0x932A20,
        "Menu_Game_Update": 0x959590,
        "Game_RestoreState": 0x6F43A0,
        "Game_SaveState": 0x6F51C0,
        "Game_GetPlayer": 0x4131B0,
        "PlayerManager_SaveGameState": 0x9940E0,
        "PlayerManager_RestoreGameState": 0x993A90,
        "PlayerManager_SyncPlayersFromGameState": 0x993780,
        "PlayerManager_RestoreGameState_PostLevelInit": 0x993FA0,
        "PlayerManager_AutoReassignControllers": 0x9964A0,
        "Level_RestoreGameState": 0x7415E0,
        "RoomLoad": 0x7385F0,
        "Player_TriggerNewStage": 0x79CBE0,
        "PlayerManager_TriggerNewStageAll": 0x9929B0,
        "Player_TriggerRoomExit": 0x79E3E0,
        "PlayerManager_NewRoomTempEffects": 0x993030,
        "Player_SetControllerId": 0x79E1B0,
        "Player_Update": 0x77BD40,
        "Player_RestoreGameState": 0x7A5660,
        "Level_SetNextStage": 0x7409A0,
        "Level_Init": 0x73EB10,
        "PlayerManager_SpawnCoPlayer2": 0x990250,
        "PlayerManager_FirstCollectibleOwner": 0x9950B0,
    }
    meta = {"kind": "read_only_static_export", "task": "tx-04-07 ST3-ST4 主审",
            "idb": idaapi.get_path(idaapi.PATH_TYPE_IDB),
            "input": ida_nalt.get_input_file_path(), "imagebase": hex(base), "functions": []}
    # 只读取版本字符串，不计算或比较任何文件摘要。
    version = idc.get_strlit_contents(0xB2EEA0)
    meta["version_string_va"] = "0xb2eea0"
    meta["version_string"] = version.decode("utf-8", errors="replace") if version else None
    for label, ea in targets.items():
        fn = ida_funcs.get_func(ea)
        entry = {"label": label, "va": hex(ea), "rva": hex(ea - base)}
        if fn is None:
            entry["error"] = "function_not_defined"
        else:
            entry["func_start_va"] = hex(fn.start_ea)
            entry["func_end_va"] = hex(fn.end_ea)
            try:
                entry["decompile"] = str(ida_hexrays.decompile(ea))
            except Exception as err:
                entry["decompile_error"] = str(err)
            entry["instructions"] = [
                {"va": hex(ins), "rva": hex(ins - base),
                 "bytes": (ida_bytes.get_bytes(ins, idc.get_item_size(ins)) or b"").hex(" "),
                 "text": ida_lines.tag_remove(idc.generate_disasm_line(ins, 0) or "")}
                for ins in idautils.FuncItems(fn.start_ea)]
            entry["xrefs_to"] = [{"from_va": hex(x.frm), "from_rva": hex(x.frm - base), "type": x.type}
                                 for x in idautils.XrefsTo(ea, 0)]
        (out / (label + ".json")).write_text(json.dumps(entry, ensure_ascii=False, indent=2), encoding="utf-8")
        meta["functions"].append({k: v for k, v in entry.items() if k not in ("decompile", "instructions", "xrefs_to")})
    (out / "metadata.json").write_text(json.dumps(meta, ensure_ascii=False, indent=2), encoding="utf-8")
    # 将转场中的虚调用接到 Player 原生入口；只是读取槽值，不修改它。
    vtable = {"va": "0xb1bdb4", "slot_offset": "0xc",
              "target_va": hex(ida_bytes.get_dword(0xB1BDB4 + 0xC))}
    (out / "player-vtable.json").write_text(json.dumps(vtable, indent=2), encoding="utf-8")
    print(json.dumps({"directory": str(out), "functions": len(targets)}, ensure_ascii=False))


export_review()
