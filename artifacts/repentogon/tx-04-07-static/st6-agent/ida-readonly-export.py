"""只读导出 tx-04-07 ST6 的 IDA 指令与交叉引用证据。"""

import json
import os

import ida_bytes
import ida_funcs
import ida_nalt
import idautils
import idc


ST6_OUTPUT = r"F:\projects_isaac\issac_multiplay\artifacts\repentogon\tx-04-07-static\st6-agent\ida-evidence.json"
ST6_BASE = ida_nalt.get_imagebase()


def st6_addr(ea):
    """把 IDA 地址同时记为 VA 与相对本次 imagebase 的 RVA。"""
    return {"va": f"0x{ea:X}", "rva": f"0x{ea - ST6_BASE:X}"}


def st6_function(ea):
    """读取函数边界与名称，不给 IDB 写入任何内容。"""
    fn = ida_funcs.get_func(ea)
    if not fn:
        return {"at": st6_addr(ea), "found": False}
    return {
        "at": st6_addr(ea),
        "start": st6_addr(fn.start_ea),
        "end": st6_addr(fn.end_ea),
        "name": idc.get_name(fn.start_ea),
        "size": fn.end_ea - fn.start_ea,
    }


def st6_instructions(start, end):
    """导出指定边界内的反汇编行，供人工复核寄存器与跳转。"""
    rows = []
    for ea in idautils.Heads(start, end):
        if not idc.is_code(idc.get_full_flags(ea)):
            continue
        rows.append({"at": st6_addr(ea), "text": idc.generate_disasm_line(ea, 0) or ""})
    return rows


def st6_field_references(ea, marker):
    """筛出函数中直接提及目标字段的指令；有限筛选不表示完整调用闭包。"""
    fn = ida_funcs.get_func(ea)
    if not fn:
        return []
    rows = []
    for head in idautils.Heads(fn.start_ea, fn.end_ea):
        if not idc.is_code(idc.get_full_flags(head)):
            continue
        text = idc.generate_disasm_line(head, 0) or ""
        if marker.lower() in text.lower():
            rows.append({"at": st6_addr(head), "text": text})
    return rows


def st6_xrefs_to(ea):
    """读取指向目标的 xref 地址与类型。"""
    return [
        {"from": st6_addr(xref.frm), "type": xref.type}
        for xref in idautils.XrefsTo(ea, 0)
    ]


def st6_dwords(start, count):
    """读取固定长度的 DWORD 表，不解释或改写表项。"""
    return [
        {"at": st6_addr(start + 4 * i), "value": ida_bytes.get_dword(start + 4 * i)}
        for i in range(count)
    ]


st6_functions = {
    "role_table_builder_PlayerManager_init_special_baby_selection": 0x994CD0,
    "join_pass_role_table_consumer": 0x991360,
    "player_init": 0x74F6A0,
    "init_pre_level_init_stats": 0x7B2C90,
    "init_post_level_init_stats": 0x7B3980,
    "room_transition_update_Door_owner": 0x826430,
    "player_update_vtable_target": 0x77BD40,
    "shoot_consumer": 0x773990,
}

st6_ranges = {
    "role_table_vector_initialization": (0x994E70, 0x994EC8),
    "role_table_stock_candidates_and_unlock_branch": (0x994EB0, 0x994F78),
    "role_table_modded_character_loop": (0x994F95, 0x995048),
    "role_table_builder_caller": (0x98F730, 0x98F780),
    "join_pass_role_array_read_and_spawn": (0x991E40, 0x992010),
    "player_init_skin_field_role7_collision": (0x750EAC, 0x750F24),
    "player_init_role18_branch": (0x751A40, 0x751AB0),
    "player_init_pre_call": (0x7518A0, 0x7518D0),
    "init_pre_role29_auxiliary_branch": (0x7B2EDF, 0x7B3156),
    "door_transition_direct_player_update_calls": (0x826540, 0x8265D5),
    "player_update_to_shoot_consumer_call": (0x77DDF0, 0x77DE40),
}

st6_vtable_address_point = 0xB1BDB4
st6_evidence = {
    "scope": "tx-04-07 ST6 read-only static evidence",
    "input_file": idc.get_input_file_path(),
    "imagebase": f"0x{ST6_BASE:X}",
    "functions": {name: st6_function(ea) for name, ea in st6_functions.items()},
    "instruction_ranges": {
        name: {
            "start": st6_addr(start),
            "end": st6_addr(end),
            "instructions": st6_instructions(start, end),
        }
        for name, (start, end) in st6_ranges.items()
    },
    "player_type_field_references": {
        "player_init_0x13BC": st6_field_references(0x74F6A0, "+13BCh"),
        "init_pre_0x13BC": st6_field_references(0x7B2C90, "+13BCh"),
        "init_post_0x13BC": st6_field_references(0x7B3980, "+13BCh"),
    },
    "role_candidate_table_dword_B2F8F0": st6_dwords(0xB2F8F0, 34),
    "player_vftable_address_point": {
        "address_point": st6_addr(st6_vtable_address_point),
        "entries": st6_dwords(st6_vtable_address_point, 20),
    },
    "cross_references_to": {
        name: st6_xrefs_to(ea)
        for name, ea in {
            "role_table_builder": 0x994CD0,
            "init_pre": 0x7B2C90,
            "init_post": 0x7B3980,
            "player_update": 0x77BD40,
            "shoot_consumer": 0x773990,
        }.items()
    },
    "limitations": [
        "此导出只覆盖列出的有限函数范围和字段引用，不声称任意角色或任意实体的调用闭包完整。",
        "本脚本只使用 IDA 只读 API，不调用改名、注释、类型、字节或分析状态写入 API。",
    ],
}

os.makedirs(os.path.dirname(ST6_OUTPUT), exist_ok=True)
with open(ST6_OUTPUT, "w", encoding="utf-8") as st6_file:
    json.dump(st6_evidence, st6_file, ensure_ascii=False, indent=2)
print(ST6_OUTPUT)
