"""tx-04-07 ST2 子智能体原始静态导出脚本。

只读 IDB：不 rename、不写注释、不改类型、不 patch、不保存数据库；只把反编译、逐条指令和
调用引用写入 JSON。通过 IDA MCP 的 py_eval 以 exec(open(path).read()) 执行；顶层只定义
带 st2_ 前缀的名称，避免与同时使用同一 IDA 的其他子智能体冲突。
脚本不是测试或校验入口，仅适用于 v1.9.7.12.J273、imagebase=0x400000 且已分析的 IDB。
"""


def st2_export():
    """导出 ST2 核心函数（完整）与大函数的关键指令片段（按区间），并写入一个 JSON 文件。"""
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

    out_dir = Path(r"F:\projects_isaac\issac_multiplay\artifacts\repentogon\tx-04-07-static\st2-agent")
    out_dir.mkdir(parents=True, exist_ok=True)
    base = ida_nalt.get_imagebase()

    def insn(ea):
        """单条指令：VA、RVA、原始字节、去标签汇编。"""
        size = idc.get_item_size(ea)
        return {"va": hex(ea), "rva": hex(ea - base),
                "bytes": (ida_bytes.get_bytes(ea, size) or b"").hex(" "),
                "text": ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or "")}

    def xrefs_to(ea):
        """到函数入口的全部引用（代码与数据），附所在函数名与 RVA。"""
        res = []
        for x in idautils.XrefsTo(ea, 0):
            fn = ida_funcs.get_func(x.frm)
            res.append({"from_va": hex(x.frm), "from_rva": hex(x.frm - base), "type": x.type,
                        "iscode": bool(x.iscode),
                        "from_func_va": hex(fn.start_ea) if fn else None,
                        "from_func_rva": hex(fn.start_ea - base) if fn else None})
        return res

    def callees(ea):
        """函数内直接 call 的目标（只列直接目标；间接调用单列文本）。"""
        direct, indirect = [], []
        for i in idautils.FuncItems(ea):
            if idc.print_insn_mnem(i) != "call":
                continue
            t = idc.get_operand_type(i, 0)
            if t in (idc.o_near, idc.o_far):
                tgt = idc.get_operand_value(i, 0)
                direct.append({"site_rva": hex(i - base), "target_va": hex(tgt), "target_rva": hex(tgt - base)})
            else:
                indirect.append({"site_rva": hex(i - base), "text": ida_lines.tag_remove(idc.generate_disasm_line(i, 0) or "")})
        return direct, indirect

    def full(label, ea, with_decompile=True):
        """完整导出一个函数。"""
        fn = ida_funcs.get_func(ea)
        entry = {"label": label, "kind": "full_function", "va": hex(ea), "rva": hex(ea - base)}
        if fn is None:
            entry["error"] = "function_not_defined"
            return entry
        entry["end_va"] = hex(fn.end_ea)
        entry["end_rva"] = hex(fn.end_ea - base)
        if with_decompile:
            try:
                entry["decompile"] = str(ida_hexrays.decompile(ea))
            except Exception as err:  # 反编译失败如实记录
                entry["decompile_error"] = str(err)
        entry["instructions"] = [insn(i) for i in idautils.FuncItems(ea)]
        d, ind = callees(ea)
        entry["direct_calls"] = d
        entry["indirect_calls"] = ind
        entry["xrefs_to"] = xrefs_to(ea)
        return entry

    def excerpt(label, start, end, note):
        """按 [start,end) 区间导出关键指令片段，用于不必整函数导出的大函数。"""
        start = idc.get_item_head(start)  # 区间起点对齐到指令头，避免从指令中间解码
        fn = ida_funcs.get_func(start)
        items, ea = [], start
        while ea < end and ea != idc.BADADDR:
            items.append(insn(ea))
            ea = idc.next_head(ea, end + 0x10)
        return {"label": label, "kind": "excerpt", "note": note,
                "func_va": hex(fn.start_ea) if fn else None,
                "func_rva": hex(fn.start_ea - base) if fn else None,
                "range_va": [hex(start), hex(end)], "range_rva": [hex(start - base), hex(end - base)],
                "instructions": items}

    fulls = [
        ("Player_RevivePlayerGhost", 0x7CFC60),
        ("Player_AddEternalHearts", 0x752D70),
        ("PlayerManager_ReviveAllCoopGhosts", 0x996AE0),
        ("PlayerManager_ReviveCoopPlayer", 0x996B20),
        ("PlayerManager_CountMainGhosts", 0x996A70),
        ("Player_PlayExtraAnimation", 0x7A3340),
        ("Player_IsExtraAnimationFinished", 0x7A2630),
        ("ReviveMenu_Update_candidate", 0x8CCCC0),
        ("Player_GetHealthType", 0x7C12C0),
        ("Room_IsCurrentRoomLastBoss_candidate", 0x815B80),
    ]
    excerpts = [
        ("Player_RestoreGameState_revive_site", 0x7A5660, 0x7A57A7,
         "入口、SetControllerIndex(state+0x518)、玻璃沙漏恢复条件下调用 RevivePlayerGhost"),
        ("Player_RestoreGameState_ghost_copy", 0x7A680C, 0x7A6842,
         "从 GameStatePlayer+0x574 直接复制到 Player+0x207D"),
        ("Player_RestoreGameState_PostLevelInit_ghost_set", 0x7A7BA8, 0x7A7BFC,
         "恢复后对 ghost 播放 GhostAppear 并置 +0x207D=1"),
        ("Player_Init_ghost_clear", 0x74FD45, 0x74FD7B, "Player::Init 写 +0x207D=0"),
        ("EvaluateItems_familiar_gate", 0x764B9B, 0x764BC4, "cache 0x200、Variant==0 与 helper 0x412D30 门控"),
        ("EvaluateItems_minion_players", 0x76816E, 0x768545,
         "Found Soul(饰品180)/道具667 附属 Player 数维护：RemoveCoPlayer/SpawnCoPlayerBaby/SpawnCoPlayer2/SetControllerIndex"),
        ("Player_TriggerNewStage_eternal", 0x79D8CB, 0x79D8E9, "饰品 55 multiplier>0 时 AddEternalHearts"),
        ("Room_SpawnClearAward_revive", 0x7F1680, 0x7F1713, "boss 房条件 + IsCoopPlay 后 revive all"),
        ("Room_TriggerClear_greed_revive", 0x7FC040, 0x7FC0FE, "Greed 模式波次/boss 条件 revive all；非 Greed 走 SpawnClearAward"),
        ("Pickup_HandleCollision_bed_revive", 0x6E88A3, 0x6E8998, "pickup variant 380 subtype 10 分支 revive all"),
        ("Pickup_HandleCollision_variant_chain", 0x6E848E, 0x6E88AE, "variant 比较链（定位 380）"),
        ("Slot_Update_revive", 0x65EDAC, 0x65EEF1, "Slot variant 19：ghost 数==1 时 revive all，否则打开 ReviveMenu"),
        ("Slot_Update_variant_switch", 0x65B400, 0x65B435, "Slot variant switch"),
        ("AddEternalHearts_caller_sites", 0x7C4AC3, 0x7C4AE8, "ChangePlayerType 内 AddEternalHearts(0)"),
        ("RemoveCoPlayer_AddEternalHearts_site", 0x990794, 0x9907B3, "RemoveCoPlayer case 29"),
        ("Lua_registration_extra_animation", 0x850160, 0x8501C3, "原生 Lua 注册：PlayExtraAnimation 等名称与函数配对"),
        ("Lua_registration_add_hearts", 0x8506C8, 0x8506E6, "原生 Lua 注册：AddEternalHearts 配对"),
        ("Player_Update_extra_anim_clear", 0x77D234, 0x77D255, "extra animation 动画完成后清 +0x1398"),
        ("Player_Update_consumer_block", 0x77DD9F, 0x77DE29, "六个 consumer 的调用块"),
        ("Movement_consumer_controls_gate", 0x7725E0, 0x772627, "移动：ControlsEnabled(+0x410) 与 ControlsCooldown(+0x414) 门控"),
        ("Shooting_consumer_extra_anim_gate", 0x773A0F, 0x773B50, "射击：extra animation 时仅 LiftItem 进入常规块，否则跳 0x373F62"),
        ("Morph_saved_fields_for_reference", 0x7D0285, 0x7D02DA, "Morph 写 1398/188/1568/207D/3B0 的对照"),
    ]
    data = {"idb": idaapi.get_path(idaapi.PATH_TYPE_IDB), "input": ida_nalt.get_input_file_path(),
            "imagebase": hex(base), "kind": "read_only_static_export_st2",
            "functions": [full(l, a) for l, a in fulls],
            "excerpts": [excerpt(l, s, e, n) for l, s, e, n in excerpts]}
    path = out_dir / "ida-st2-functions.json"
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")
    return {"file": str(path), "functions": len(fulls), "excerpts": len(excerpts)}


st2_export_result = st2_export()
print(st2_export_result)
