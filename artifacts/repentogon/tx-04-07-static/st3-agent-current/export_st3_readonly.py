# -*- coding: utf-8 -*-
"""只读导出 ST3 静态证据；不修改 IDB、分析状态或目标字节。"""
import os as st3new_os
import json as st3new_json
import idaapi as st3new_idaapi
import ida_funcs as st3new_ida_funcs
import ida_hexrays as st3new_hexrays
import ida_bytes as st3new_ida_bytes
import ida_lines as st3new_ida_lines
import idautils as st3new_idautils
import idc as st3new_idc
import ida_nalt as st3new_ida_nalt

st3new_base = int(st3new_idaapi.get_imagebase())
st3new_output = r"F:\projects_isaac\issac_multiplay\artifacts\repentogon\tx-04-07-static\st3-agent-current\st3-static-export.json"
st3new_targets = [
    (0x6F9B30, "Game::StartStageTransition", True, True),
    (0x6F66E0, "Game::ProcessInput", True, True),
    (0x92E3E0, "Manager::Update", True, True),
    (0x6F6D30, "Game::Update", True, True),
    (0x909100, "StageTransition::Update", True, True),
    (0x73EB10, "Level::Init", True, True),
    (0x7385F0, "Level::RoomLoad", True, True),
    (0x9929B0, "PlayerManager::TriggerNewStageAll", True, True),
    (0x79CBE0, "Player::TriggerNewStage", True, False),
    (0x77BD40, "Player::Update", True, False),
    (0x79E3E0, "Player::TriggerRoomExit", True, False),
    (0x4131B0, "Game::GetPlayer", True, True),
    (0x9910A0, "PlayerManager::CoopBabiesOnly", True, True),
    (0x991360, "JoinPass", False, False),
    (0x7F7F60, "Room::Update", True, False),
    (0x84B930, "LuaEngine_PostNewRoom_candidate", False, False),
    (0x84BA10, "LuaEngine_PostNewLevel_candidate", False, False),
    (0x932160, "Manager::SaveGameState", False, False),
    (0x984110, "ItemOverlay::Update", False, False),
    (0x79E1B0, "Player::SetControllerIndex", True, False),
]
st3new_focus = {
    0x6F9B30: [(0x6F9B44, 0x6F9BEE), (0x6F9D30, 0x6F9D9A), (0x6F9E70, 0x6F9EAE), (0x6F9F20, 0x6F9F50)],
    0x6F66E0: [(0x6F6CD0, 0x6F6D28)],
    0x77BD40: [(0x77BF03, 0x77C025)],
    0x92E3E0: [(0x92E885, 0x92E926)],
    0x6F6D30: [(0x6F73E0, 0x6F7415)],
    0x909100: [(0x9098B0, 0x909B50), (0x909D50, 0x909E15)],
    0x73EB10: [(0x73FA20, 0x73FC55)],
    0x7385F0: [(0x739700, 0x739725)],
    0x9929B0: [(0x9929B0, 0x992A75)],
    0x9910A0: [(0x9915A0, 0x9915C0), (0x991740, 0x991760), (0x991E58, 0x991E78)],
    0x991360: [(0x9915A0, 0x9915C0), (0x991740, 0x991760), (0x991E58, 0x991E78)],
}

def st3new_hex(value):
    return "0x%X" % int(value)

def st3new_item(ea):
    st3new_size = int(st3new_idc.get_item_size(ea) or 0)
    st3new_raw = st3new_ida_bytes.get_bytes(ea, st3new_size) if st3new_size else b""
    st3new_refs = []
    st3new_mnem = st3new_idc.print_insn_mnem(ea)
    if st3new_mnem.lower().startswith("call"):
        for st3new_ref in st3new_idautils.CodeRefsFrom(ea, False):
            st3new_refs.append({"va": st3new_hex(st3new_ref), "rva": st3new_hex(st3new_ref - st3new_base), "name": st3new_idc.get_name(st3new_ref)})
    return {
        "va": st3new_hex(ea), "rva": st3new_hex(ea - st3new_base), "size": st3new_size,
        "bytes": st3new_raw.hex(" ").upper() if st3new_raw else "",
        "instruction": st3new_ida_lines.tag_remove(st3new_idc.generate_disasm_line(ea, 0) or ""),
        "direct_call_refs": st3new_refs,
    }

def st3new_function(address, label, include_pseudo, include_asm):
    st3new_func = st3new_ida_funcs.get_func(address)
    if not st3new_func:
        return {"label": label, "requested_va": st3new_hex(address), "error": "IDA 未识别该函数入口"}
    st3new_start = int(st3new_func.start_ea)
    st3new_end = int(st3new_func.end_ea)
    st3new_data = {
        "label": label,
        "ida_name": st3new_idc.get_func_name(st3new_start),
        "va_start": st3new_hex(st3new_start), "rva_start": st3new_hex(st3new_start - st3new_base),
        "va_end_exclusive": st3new_hex(st3new_end), "rva_end_exclusive": st3new_hex(st3new_end - st3new_base),
        "size_bytes": st3new_end - st3new_start,
        "type": st3new_idc.get_type(st3new_start) or "",
        "xrefs_to_code": [], "direct_calls": [], "focus_windows": [],
    }
    for st3new_xref in st3new_idautils.CodeRefsTo(st3new_start, False):
        st3new_xref_func = st3new_ida_funcs.get_func(st3new_xref)
        st3new_data["xrefs_to_code"].append({
            "site_va": st3new_hex(st3new_xref), "site_rva": st3new_hex(st3new_xref - st3new_base),
            "caller_va": st3new_hex(st3new_xref_func.start_ea) if st3new_xref_func else None,
            "caller_name": st3new_idc.get_func_name(st3new_xref_func.start_ea) if st3new_xref_func else None,
        })
    st3new_all_items = []
    for st3new_ea in st3new_idautils.FuncItems(st3new_start):
        if st3new_ea >= st3new_end:
            continue
        st3new_row = st3new_item(st3new_ea)
        st3new_all_items.append(st3new_row)
        st3new_data["direct_calls"].extend([dict(st3new_row["direct_call_refs"][st3new_i], site_va=st3new_row["va"], site_rva=st3new_row["rva"]) for st3new_i in range(len(st3new_row["direct_call_refs"]))])
    if include_asm:
        st3new_data["instructions"] = st3new_all_items
    for st3new_lo, st3new_hi in st3new_focus.get(st3new_start, []):
        st3new_data["focus_windows"].append({"va_start": st3new_hex(st3new_lo), "va_end_exclusive": st3new_hex(st3new_hi), "instructions": [st3new_row for st3new_row in st3new_all_items if st3new_lo <= int(st3new_row["va"], 16) < st3new_hi]})
    if include_pseudo:
        try:
            st3new_cfunc = st3new_hexrays.decompile(st3new_start)
            st3new_data["pseudocode"] = "\n".join(st3new_ida_lines.tag_remove(st3new_line.line) for st3new_line in st3new_cfunc.get_pseudocode()) if st3new_cfunc else None
        except Exception as st3new_exc:
            st3new_data["pseudocode_error"] = repr(st3new_exc)
    return st3new_data

st3new_report = {
    "evidence_kind": "只读 IDA/Hex-Rays 导出；不更改 IDB",
    "target_version": "v1.9.7.12",
    "module": st3new_os.path.basename(st3new_ida_nalt.get_input_file_path()),
    "input_path": st3new_ida_nalt.get_input_file_path(),
    "imagebase": st3new_hex(st3new_base),
    "rva_rule": "RVA = VA - imagebase",
    "functions": [st3new_function(*st3new_target) for st3new_target in st3new_targets],
    "data": {
        "player_vtable_va": st3new_hex(0xB1BDB4),
        "player_vtable_slot_update_va": st3new_hex(0xB1BDB4 + 0xC),
        "player_vtable_slot_update_value_va": st3new_hex(st3new_ida_bytes.get_dword(0xB1BDB4 + 0xC)),
        "player_vtable_slot_update_value_rva": st3new_hex(st3new_ida_bytes.get_dword(0xB1BDB4 + 0xC) - st3new_base),
    },
}
with open(st3new_output, "w", encoding="utf-8") as st3new_file:
    st3new_json.dump(st3new_report, st3new_file, ensure_ascii=False, indent=2)
print("只读导出完成: " + st3new_output)
print("函数数=%d, imagebase=%s" % (len(st3new_report["functions"]), st3new_report["imagebase"]))


