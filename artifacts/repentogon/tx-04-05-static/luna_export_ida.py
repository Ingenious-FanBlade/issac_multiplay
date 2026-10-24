# -*- coding: utf-8 -*-
"""只读导出 tx-04-05 静态证据；不修改 IDB 的名称、类型、字节或注释。"""

from pathlib import Path
import json
import os

import ida_bytes
import ida_funcs
import ida_hexrays
import ida_lines
import ida_nalt
import idautils
import idc


# 文档使用 RVA；IDA Python 使用 VA，统一由当前 IDB imagebase 换算。
IMAGEBASE = ida_nalt.get_imagebase()
OUT_DIR = Path(os.environ.get("TX0405_STATIC_OUT", str(Path(__file__).resolve().parent / "reexport")))

# 只选取本次 T1–T8 需要复核的函数；超大函数仍保留伪代码，汇编有上限避免误写巨型文件。
FUNCTIONS = [
    ("t1_odd_helper", 0x2F9340),
    ("t1_entity_player_update", 0x37BD40),
    ("t2_input_manager_update", 0x5EDCC0),
    ("t2_find_device_by_controller_id", 0x5EEC40),
    ("t3_device_common_update", 0x5EC900),
    ("t3_device_pressed", 0x5ECD80),
    ("t3_device_triggered", 0x5ECDE0),
    ("t3_device_scalar", 0x5ECE50),
    ("t3_keyboard_raw_pressed", 0x633CA0),
    ("t3_keyboard_raw_triggered", 0x633CD0),
    ("t3_gamepad_raw_pressed", 0x5ECB80),
    ("t3_gamepad_raw_triggered", 0x5ECC40),
    ("t3_gamepad_action_map", 0x5ED190),
    ("t4_input_manager_pressed", 0x5EE140),
    ("t4_input_manager_triggered", 0x5EE200),
    ("t4_manager_bool_helper", 0x5EE2C0),
    ("t8_input_manager_scalar", 0x5EE2E0),
    ("t8_manager_pressed_wrapper", 0x52F610),
    ("t8_manager_triggered_wrapper", 0x52F6D0),
    ("t8_manager_scalar_wrapper", 0x52F7B0),
    ("t5_game_update", 0x2F6D30),
    ("t5_odd_pause_focus_predicate", 0x2F92A0),
    ("t5_pause_screen_predicate", 0x58ED50),
    ("t5_focus_pause_screen_transition", 0x58ED80),
    ("t5_focus_option_load", 0x4FE470),
    ("t5_focus_flag_setter", 0x581AC0),
    ("t5_focus_callback_registrar", 0x581AE0),
    ("t5_focus_callback_registration_block", 0x5821E0),
    ("t6_room_load_large", 0x3385F0),
    ("t6_level_change_room", 0x339DE0),
    ("t6_lifecycle_owner", 0x33EB10),
    ("t6_other_room_load_owner", 0x3415E0),
    ("t6_post_new_room_callback", 0x44B930),
    ("t6_post_load_followup", 0x593030),
    ("t7_player_field_stores", 0x3794B0),
    ("t8_movement_scalar_helper", 0x371D70),
    ("t8_movement_consumer", 0x372540),
    ("t8_shooting_consumer", 0x373990),
    ("t8_bomb_consumer", 0x37A210),
    ("t8_active_consumer", 0x389E50),
    ("t8_card_pill_consumer", 0x39BDD0),
    ("t8_drop_consumer", 0x39C300),
]

# 聚焦导出对判断顺序关键的指令窗口，窗口坐标同样是 RVA。
RANGES = [
    ("t1_odd_update_calls", 0x2F93D0, 0x2F9589),
    ("t5_game_focus_branch", 0x2F76C0, 0x2F7770),
    ("t5_odd_focus_branch", 0x2F94F0, 0x2F9520),
    ("t5_pause_state_gate", 0x58ED50, 0x58EE70),
    ("t6_room_callback_order", 0x339690, 0x339774),
    ("t7_player_field_stores", 0x379490, 0x3794E0),
    ("t8_manager_scalar_call_path", 0x52F7B0, 0x52F870),
]


def clean(text):
    """去掉 IDA 颜色标签，只保留可读文本。"""
    return ida_lines.tag_remove(text or "")


def disasm_function(ea, limit=12000):
    """导出真实函数范围的指令和原字节，并明确记录截断。"""
    function = ida_funcs.get_func(ea)
    if function is None:
        return ["[NO FUNCTION BOUNDARY]"]
    lines = []
    for count, head in enumerate(idautils.FuncItems(function.start_ea)):
        if count >= limit:
            lines.append("[TRUNCATED: instruction limit %d]" % limit)
            break
        size = idc.get_item_size(head)
        raw = ida_bytes.get_bytes(head, size) or b""
        byte_text = " ".join("%02X" % byte for byte in raw)
        lines.append("RVA 0x%08X VA 0x%08X %s  %s" %
                     (head - IMAGEBASE, head, byte_text, clean(idc.generate_disasm_line(head, 0))))
    return lines


def decompile_function(ea):
    """原样保留反编译失败事实，不用空伪代码填补未知。"""
    try:
        pseudocode = ida_hexrays.decompile(ea)
    except Exception as error:
        return "[HEX-RAYS ERROR] %s" % error
    if pseudocode is None:
        return "[HEX-RAYS NONE] 无伪代码结果；该结论只能引用汇编。"
    return "\n".join(clean(line.line) for line in pseudocode.get_pseudocode())


def disasm_range(start_rva, end_rva):
    """按 RVA 窗口读取已有指令，不创建代码或修改函数边界。"""
    start = IMAGEBASE + start_rva
    end = IMAGEBASE + end_rva
    lines = []
    ea = start
    while ea < end:
        if ida_bytes.is_head(ida_bytes.get_flags(ea)):
            size = idc.get_item_size(ea)
            raw = ida_bytes.get_bytes(ea, size) or b""
            byte_text = " ".join("%02X" % byte for byte in raw)
            lines.append("RVA 0x%08X VA 0x%08X %s  %s" %
                         (ea - IMAGEBASE, ea, byte_text, clean(idc.generate_disasm_line(ea, 0))))
        next_ea = ida_bytes.next_head(ea, end)
        if next_ea <= ea:
            break
        ea = next_ea
    return lines


OUT_DIR.mkdir(parents=True, exist_ok=True)
manifest = {
    "input": ida_nalt.get_input_file_path(),
    "imagebase": "0x%08X" % IMAGEBASE,
    "addresses_in_manifest": "RVA; exported IDA addresses are VA = imagebase + RVA",
    "idb_mutated": False,
    "exports": [],
}

for name, rva in FUNCTIONS:
    ea = IMAGEBASE + rva
    function = ida_funcs.get_func(ea)
    start_rva = function.start_ea - IMAGEBASE if function else None
    end_rva = function.end_ea - IMAGEBASE if function else None
    path = OUT_DIR / (name + ".txt")
    content = [
        "NAME=%s" % name,
        "REQUESTED_RVA=0x%08X VA=0x%08X" % (rva, ea),
        "FUNCTION_RVA=%s..%s" % (("0x%08X" % start_rva) if start_rva is not None else "UNKNOWN",
                                  ("0x%08X" % end_rva) if end_rva is not None else "UNKNOWN"),
        "INPUT=%s" % ida_nalt.get_input_file_path(),
        "IMAGEBASE=0x%08X" % IMAGEBASE,
        "[DECOMPILE]",
        decompile_function(ea),
        "[DISASSEMBLY]",
        "\n".join(disasm_function(ea)),
    ]
    path.write_text("\n".join(content) + "\n", encoding="utf-8")
    manifest["exports"].append({"name": name, "rva": "0x%08X" % rva,
                                "function_start_rva": ("0x%08X" % start_rva) if start_rva is not None else None,
                                "function_end_rva": ("0x%08X" % end_rva) if end_rva is not None else None,
                                "path": str(path)})

for name, start_rva, end_rva in RANGES:
    path = OUT_DIR / (name + ".txt")
    lines = ["RVA_RANGE=0x%08X..0x%08X" % (start_rva, end_rva),
             "IMAGEBASE=0x%08X" % IMAGEBASE,
             "INPUT=%s" % ida_nalt.get_input_file_path(), "[DISASSEMBLY]"]
    lines.extend(disasm_range(start_rva, end_rva))
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    manifest["exports"].append({"name": name, "rva_range": ["0x%08X" % start_rva, "0x%08X" % end_rva],
                                "path": str(path)})

(OUT_DIR / "luna-export-index.json").write_text(
    json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print("只读静态证据已导出到: %s" % OUT_DIR)
