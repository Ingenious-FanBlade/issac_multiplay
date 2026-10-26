"""tx-04-07 ST5–ST7 主审静态资料导出；只读 IDB，不调用目标程序。"""
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
    """保留完整函数、字节和引用；函数标签是人工解释，不写回数据库。"""
    out = Path(r"F:\projects_isaac\issac_multiplay\artifacts\repentogon\tx-04-07-static\st5-st7-review")
    out.mkdir(parents=True, exist_ok=True)
    base = ida_nalt.get_imagebase()
    targets = {
        "PlayerManager_CoopBabiesOnly": 0x9910A0,
        "PlayerManager_JoinPass": 0x991360,
        "PlayerManager_BuildRoleTable": 0x994CD0,
        "PlayerManager_SpawnCoPlayer2": 0x990250,
        "Level_ChangeRoom": 0x739DE0,
        "Level_Init": 0x73EB10,
        "Level_SetNextStage": 0x7409A0,
        "Game_SaveState": 0x6F51C0,
        "Game_RestoreState": 0x6F43A0,
        "Game_ResetState": 0x6F08D0,
        "Game_ExecuteStartPreparation": 0x6F0AF0,
        "Game_Constructor_OfficialSignature": 0x6ED4B0,
        "Game_InitializeState": 0x6F0770,
        "Game_Start": 0x6F16D0,
        "Game_RestartLike": 0x703460,
        "Manager_ExecuteStartGame": 0x932A20,
        "Player_Init": 0x74F6A0,
        "Player_InitPreLevelInitStats": 0x7B2C90,
        "Player_InitPostLevelInitStats": 0x7B3980,
        "Player_Update": 0x77BD40,
        "Player_Shooting": 0x773990,
        "RoomTransition_Update": 0x826430,
        "Game_DoorNestedOwner": 0x6FA0F0,
        "Player_RevivePlayerGhost": 0x7CFC60,
        "Player_MorphToCoopGhost": 0x7CFFA0,
        "Manager_SaveGameState": 0x932160,
        "Manager_DeleteGameState": 0x9324A0,
        "Manager_LoadGameState": 0x9332F0,
        "GameState_Delete": 0x99E7B0,
        "GameState_DeleteRerun": 0x99E870,
        "GameState_write": 0x99F7A0,
        "StageTransition_Update": 0x909100,
        "PGD_TryUnlock": 0x903930,
        "PGD_IncreaseEventCounter": 0x903A50,
        "PGD_SaveLocally": 0x903460,
        "PGD_SaveToSteamCloud": 0x902F00,
        "PGD_BackupCandidate": 0x9032A0,
        "PGD_BackupFileCandidate": 0x903380,
    }
    meta = {"kind": "read_only_static_export", "task": "tx-04-07 ST5-ST7 主审",
            "idb": idaapi.get_path(idaapi.PATH_TYPE_IDB),
            "input": ida_nalt.get_input_file_path(), "imagebase": hex(base),
            "functions": []}
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
    # Player vtable 的 Update 实值用于连接门过渡中的虚调用；不改任何槽位。
    vtable = {"va": "0xb1bdb4", "rva": "0x71bdb4", "slot": 3,
              "slot_offset": "0xc", "target_va": hex(ida_bytes.get_dword(0xB1BDB4 + 0xC))}
    (out / "player-vtable.json").write_text(json.dumps(vtable, indent=2), encoding="utf-8")
    # 原生候选角色表是只读 int32 数组，表中出现 7 不等于已通过解锁筛选。
    roles = {"va": "0xb2f8f0", "rva": "0x72f8f0",
             "kind": "first_32_int32_values_only_not_array_boundary_claim",
             "values": [ida_bytes.get_dword(0xB2F8F0 + i * 4) for i in range(32)]}
    (out / "stock-role-candidates.json").write_text(json.dumps(roles, indent=2), encoding="utf-8")
    print(json.dumps({"directory": str(out), "functions": len(targets)}, ensure_ascii=False))


export_review()
