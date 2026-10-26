"""tx-04-07 ST5 静态证据导出器：只读当前 IDB，不运行目标、不改写 IDB。"""


def st5_export():
    """导出 Join predicate、场景路径函数和 Game+0x25AC0 的完整静态访问清单。"""
    import json as st5_json
    from pathlib import Path as st5_Path
    import idaapi as st5_idaapi
    import ida_bytes as st5_ida_bytes
    import ida_funcs as st5_ida_funcs
    import ida_hexrays as st5_ida_hexrays
    import ida_lines as st5_ida_lines
    import ida_nalt as st5_ida_nalt
    import ida_segment as st5_ida_segment
    import idautils as st5_idautils
    import idc as st5_idc

    st5_out = st5_Path(r"F:\projects_isaac\issac_multiplay\artifacts\repentogon\tx-04-07-static\st5-agent")
    st5_out.mkdir(parents=True, exist_ok=True)
    st5_base = st5_ida_nalt.get_imagebase()

    def st5_instruction(st5_ea):
        """记录指令 VA/RVA、原始字节和 IDA 渲染的汇编，供人工复核。"""
        st5_size = st5_idc.get_item_size(st5_ea)
        return {"va": hex(st5_ea), "rva": hex(st5_ea - st5_base),
                "bytes": (st5_ida_bytes.get_bytes(st5_ea, st5_size) or b"").hex(" "),
                "asm": st5_ida_lines.tag_remove(st5_idc.generate_disasm_line(st5_ea, 0) or "")}

    def st5_xrefs_to(st5_ea):
        """列出到指定函数入口的全部直接 xref，并注明源函数边界。"""
        st5_rows = []
        for st5_xref in st5_idautils.XrefsTo(st5_ea, 0):
            st5_fn = st5_ida_funcs.get_func(st5_xref.frm)
            st5_rows.append({"from_va": hex(st5_xref.frm), "from_rva": hex(st5_xref.frm - st5_base),
                             "type": st5_xref.type, "iscode": bool(st5_xref.iscode),
                             "from_func_va": hex(st5_fn.start_ea) if st5_fn else None,
                             "from_func_rva": hex(st5_fn.start_ea - st5_base) if st5_fn else None,
                             "from_func_name": st5_idc.get_func_name(st5_fn.start_ea) if st5_fn else None})
        return st5_rows

    def st5_calls(st5_ea):
        """列出完整函数内的直接 call 目标；间接 call 保留原汇编，不推测目标。"""
        st5_direct, st5_indirect = [], []
        for st5_ins in st5_idautils.FuncItems(st5_ea):
            if st5_idc.print_insn_mnem(st5_ins).lower() != "call":
                continue
            st5_kind = st5_idc.get_operand_type(st5_ins, 0)
            if st5_kind in (st5_idc.o_near, st5_idc.o_far):
                st5_target = st5_idc.get_operand_value(st5_ins, 0)
                st5_direct.append({"site_va": hex(st5_ins), "site_rva": hex(st5_ins - st5_base),
                                   "target_va": hex(st5_target), "target_rva": hex(st5_target - st5_base),
                                   "target_name": st5_idc.get_func_name(st5_target)})
            else:
                st5_indirect.append(st5_instruction(st5_ins))
        return st5_direct, st5_indirect

    def st5_full_function(st5_label, st5_ea):
        """完整导出关键函数，保留反编译、每条指令、调用目标和函数 xref。"""
        st5_fn = st5_ida_funcs.get_func(st5_ea)
        st5_entry = {"label": st5_label, "va": hex(st5_ea), "rva": hex(st5_ea - st5_base)}
        if st5_fn is None:
            st5_entry["error"] = "function_not_defined"
            return st5_entry
        st5_entry.update({"end_va": hex(st5_fn.end_ea), "end_rva": hex(st5_fn.end_ea - st5_base),
                          "ida_name": st5_idc.get_func_name(st5_fn.start_ea)})
        try:
            st5_entry["decompile_raw"] = str(st5_ida_hexrays.decompile(st5_ea))
        except Exception as st5_err:
            st5_entry["decompile_error"] = str(st5_err)
        st5_entry["instructions"] = [st5_instruction(st5_ins) for st5_ins in st5_idautils.FuncItems(st5_ea)]
        st5_direct, st5_indirect = st5_calls(st5_ea)
        st5_entry["direct_calls"] = st5_direct
        st5_entry["indirect_calls"] = st5_indirect
        st5_entry["xrefs_to_function"] = st5_xrefs_to(st5_ea)
        return st5_entry

    # 这些函数覆盖三处 predicate、切房置位、阶段/Continue 入口及保存恢复路径。
    st5_targets = [
        ("PlayerManager_CoopBabiesOnly", 0x9910A0),
        ("PlayerManager_JoinPass", 0x991360),
        ("Level_ChangeRoom", 0x739DE0),
        ("Level_Init", 0x73EB10),
        ("Level_SetNextStage", 0x7409A0),
        ("Game_StartStageTransition", 0x6F9B30),
        ("Game_SaveState", 0x6F51C0),
        ("Game_RestoreState", 0x6F43A0),
        ("Game_Constructor", 0x6ED4B0),
        ("ExecuteStartGame_Prepare_6F0AF0", 0x6F0AF0),
        ("Game_InitializationSubroutine_6F0770", 0x6F0770),
        ("Game_ResetState", 0x6F08D0),
        ("Game_RKey_ResetHelper", 0x703460),
        ("CallbackObjectFactory", 0x6047F0),
        ("Game_Start", 0x6F16D0),
        ("Manager_ExecuteStartGame", 0x932A20),
        ("Manager_LoadGameState", 0x9332F0),
        ("PlayerManager_RestoreGameState", 0x993A90),
        ("PlayerManager_RestoreGameState_PostLevelInit", 0x993FA0),
    ]

    st5_metadata = {"kind": "read_only_static_export_st5", "idb": st5_idaapi.get_path(st5_idaapi.PATH_TYPE_IDB),
                    "input": st5_ida_nalt.get_input_file_path(), "imagebase": hex(st5_base),
                    "functions": []}
    for st5_label, st5_ea in st5_targets:
        st5_entry = st5_full_function(st5_label, st5_ea)
        st5_file = st5_out / (st5_label + ".json")
        st5_file.write_text(st5_json.dumps(st5_entry, ensure_ascii=False, indent=2), encoding="utf-8")
        st5_metadata["functions"].append({key: value for key, value in st5_entry.items()
                                          if key not in ("decompile_raw", "instructions", "direct_calls",
                                                         "indirect_calls", "xrefs_to_function")})

    # 遍历整个 .text，只保留含 0x25AC0 位移的指令；它们可能是字段访问、地址形成或拷贝，
    # 汇编清单本身不替代人工判断读写方向和运行条件。
    st5_text = st5_ida_segment.get_segm_by_name(".text")
    st5_accesses = []
    if st5_text:
        for st5_ea in st5_idautils.Heads(st5_text.start_ea, st5_text.end_ea):
            if not st5_idc.is_code(st5_ida_bytes.get_flags(st5_ea)):
                continue
            st5_row = st5_instruction(st5_ea)
            if "25AC0h" not in st5_row["asm"]:
                continue
            st5_fn = st5_ida_funcs.get_func(st5_ea)
            st5_row["function_va"] = hex(st5_fn.start_ea) if st5_fn else None
            st5_row["function_rva"] = hex(st5_fn.start_ea - st5_base) if st5_fn else None
            st5_row["function_name"] = st5_idc.get_func_name(st5_fn.start_ea) if st5_fn else None
            st5_accesses.append(st5_row)
    st5_field_data = {"kind": "read_only_static_field_access_inventory", "field_expression": "[base+0x25AC0]",
                      "imagebase": hex(st5_base), "segment": ".text" if st5_text else None,
                      "access_count": len(st5_accesses), "accesses": st5_accesses}
    (st5_out / "GameField_25AC0_all_accesses.json").write_text(
        st5_json.dumps(st5_field_data, ensure_ascii=False, indent=2), encoding="utf-8")

    # 记录 ResetState 的特殊回调登记点和 thunk。此处仅截取调度器局部原始指令，
    # 不把局部 xref 当成完整间接调用闭包，也不推断普通换层会执行该回调。
    st5_callback_ranges = [
        {"label": "switch_case_636_registration", "start": 0x5C3760, "end": 0x5C37A5},
        {"label": "resetstate_callback_thunk", "start": 0x5C7C90, "end": 0x5C7C95},
    ]
    st5_callback_context = {"kind": "read_only_resetstate_callback_context", "imagebase": hex(st5_base),
                            "ranges": [], "xref_targets": {}}
    for st5_range in st5_callback_ranges:
        st5_rows = []
        for st5_ea in st5_idautils.Heads(st5_range["start"], st5_range["end"]):
            if st5_idc.is_code(st5_ida_bytes.get_flags(st5_ea)):
                st5_rows.append(st5_instruction(st5_ea))
        st5_callback_context["ranges"].append({"label": st5_range["label"],
                                                "start_va": hex(st5_range["start"]),
                                                "start_rva": hex(st5_range["start"] - st5_base),
                                                "end_va_exclusive": hex(st5_range["end"]),
                                                "instructions": st5_rows})
    for st5_label, st5_ea in (("resetstate", 0x6F08D0), ("rkey_helper", 0x703460),
                               ("callback_thunk", 0x5C7C90)):
        st5_callback_context["xref_targets"][st5_label] = {
            "va": hex(st5_ea), "rva": hex(st5_ea - st5_base),
            "xrefs_to": st5_xrefs_to(st5_ea)}
    (st5_out / "Game_ResetState_callback_context.json").write_text(
        st5_json.dumps(st5_callback_context, ensure_ascii=False, indent=2), encoding="utf-8")

    (st5_out / "metadata.json").write_text(st5_json.dumps(st5_metadata, ensure_ascii=False, indent=2), encoding="utf-8")
    return {"directory": str(st5_out), "function_count": len(st5_targets),
            "field_access_count": len(st5_accesses), "callback_ranges": len(st5_callback_ranges),
            "imagebase": hex(st5_base)}


st5_export_result = st5_export()
print(st5_export_result)
