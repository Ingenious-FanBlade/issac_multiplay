"""tx-04-07 ST2 主审：只读导出当前 IDB，不执行目标、不修改或保存数据库。"""


def st2_review_export():
    """保存复活、动画、输入 consumer、条件附属 Player 与直接调用方的原始材料。"""
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

    out = Path(r"F:\projects_isaac\issac_multiplay\artifacts\repentogon\tx-04-07-static\st2-review")
    out.mkdir(parents=True, exist_ok=True)
    base = ida_nalt.get_imagebase()
    targets = {
        "Player_RevivePlayerGhost": 0x7CFC60,
        "Player_PlayExtraAnimation": 0x7A3340,
        "Player_IsExtraAnimationFinished": 0x7A2630,
        "Player_Update": 0x77BD40,
        "Player_Movement": 0x772540,
        "Player_MovementAux": 0x771D70,
        "Player_Shooting": 0x773990,
        "Player_RefreshGraphics_candidate": 0x7B5CF0,
        "Player_RefreshCostume_candidate": 0x7560C0,
        "Player_EvaluateItems": 0x75D4B0,
        "Player_InitTwin": 0x7C8370,
        "Player_GetHealthType": 0x7C12C0,
        "Player_AddEternalHearts": 0x752D70,
        "Player_AddBoneHearts": 0x7C0B30,
        "PlayerManager_ReviveAllCoopGhosts": 0x996AE0,
        "PlayerManager_ReviveCoopPlayer": 0x996B20,
        "PlayerManager_CountMainGhosts": 0x996A70,
        "Player_RestoreGameState": 0x7A5660,
        "Player_Revive": 0x79A400,
        "Player_MorphToCoopGhost": 0x7CFFA0,
    }
    version = idc.get_strlit_contents(0xB2EEA0)
    meta = {"task": "tx-04-07 ST2 主审", "date": "2026-10-26",
            "kind": "read_only_static_export", "idb": idaapi.get_path(idaapi.PATH_TYPE_IDB),
            "input": ida_nalt.get_input_file_path(), "imagebase": hex(base),
            "version_string": version.decode("utf-8", errors="replace") if version else None,
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
            entry["instructions"] = []
            entry["direct_calls"] = []
            entry["indirect_calls"] = []
            for ins in idautils.FuncItems(fn.start_ea):
                asm = ida_lines.tag_remove(idc.generate_disasm_line(ins, 0) or "")
                entry["instructions"].append({"va": hex(ins), "rva": hex(ins - base),
                    "bytes": (ida_bytes.get_bytes(ins, idc.get_item_size(ins)) or b"").hex(" "),
                    "text": asm})
                if idc.print_insn_mnem(ins) == "call":
                    if idc.get_operand_type(ins, 0) in (idc.o_near, idc.o_far):
                        entry["direct_calls"].append({"site_rva": hex(ins - base),
                            "target_rva": hex(idc.get_operand_value(ins, 0) - base)})
                    else:
                        entry["indirect_calls"].append({"site_rva": hex(ins - base), "text": asm})
            entry["xrefs_to"] = [{"from_va": hex(x.frm), "from_rva": hex(x.frm - base),
                "type": x.type, "iscode": bool(x.iscode)} for x in idautils.XrefsTo(ea, 0)]
        (out / (label + ".json")).write_text(json.dumps(entry, ensure_ascii=False, indent=2), encoding="utf-8")
        meta["functions"].append({k: v for k, v in entry.items()
            if k not in ("decompile", "instructions", "direct_calls", "indirect_calls", "xrefs_to")})
    (out / "metadata.json").write_text(json.dumps(meta, ensure_ascii=False, indent=2), encoding="utf-8")
    # 单独保留调用方条件与注册片段，避免把大函数的局部读取冒充全闭包。
    ranges = {
        "lua_extra_animation_registration": (0x850160, 0x8501C3),
        "boss_clear_revive": (0x7F1680, 0x7F1713),
        "greed_clear_revive": (0x7FC040, 0x7FC0FE),
        "bed_revive": (0x6E88A3, 0x6E8998),
        "slot_revive": (0x65EDAC, 0x65EEF1),
        "new_stage_eternal_heart": (0x79D8CB, 0x79D8E9),
        "restore_ghost_copy": (0x7A680C, 0x7A6842),
    }
    details = {"ranges": []}
    for label, (start, end) in ranges.items():
        details["ranges"].append({"label": label, "start_va": hex(start), "end_va": hex(end),
            "instructions": [{"va": hex(ea), "rva": hex(ea - base),
                "bytes": (ida_bytes.get_bytes(ea, idc.get_item_size(ea)) or b"").hex(" "),
                "text": ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or "")}
                for ea in idautils.Heads(start, end)]})
    # 第 47 项开始已是 RTTI COL；只记录实值，不将邻接数据当成函数槽。
    details["inputmanager_vtable_tail"] = [{"slot": slot, "va": hex(0xB32E44 + slot * 4),
        "value": hex(ida_bytes.get_dword(0xB32E44 + slot * 4))}
        for slot in range(44, 50)]
    details["revive_entry_bytes"] = (ida_bytes.get_bytes(0x7CFC60, 20) or b"").hex(" ")
    (out / "details.json").write_text(json.dumps(details, ensure_ascii=False, indent=2), encoding="utf-8")
    # 官方 Hook 源码与原生 IDB 分开存放；保留原文件行号，供离线复阅。
    source_root = Path(r"F:\projects_isaac\REPENTOGON-1.1.3")
    source_ranges = {
        "libzhl/functions/EntityPlayer.zhl": [(120, 126), (510, 517)],
        "repentogon/LuaInterfaces/Entities/LuaEntityPlayer.cpp": [(2355, 2368), (3748, 3752)],
        "repentogon/Patches/CustomCache.cpp": [(290, 339)],
        "repentogon/LuaInterfaces/CustomCallbacks.cpp": [(266, 330), (1574, 1658),
            (751, 789), (4874, 4905), (5482, 5487)],
        "repentogon/Patches/Anm2Extras.cpp": [(85, 94)],
    }
    source_text = ["# ST2 主审官方源码摘录", "", "2026-10-26。只读摘录，未构建或运行。", ""]
    for relative, spans in source_ranges.items():
        lines = (source_root / relative).read_text(encoding="utf-8-sig").splitlines()
        source_text.extend(["## " + relative, ""])
        for first, last in spans:
            source_text.extend(f"{line}: {lines[line - 1]}" for line in range(first, min(last, len(lines)) + 1))
            source_text.append("")
    (out / "source-excerpts.md").write_text("\n".join(source_text), encoding="utf-8")
    print(json.dumps({"directory": str(out), "functions": len(targets)}, ensure_ascii=False))


st2_review_export()
