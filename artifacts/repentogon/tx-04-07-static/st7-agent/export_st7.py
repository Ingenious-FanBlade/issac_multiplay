"""tx-04-07 ST7 静态证据导出；只读 IDB，不调用目标程序、不写回 IDB。"""
import json
from pathlib import Path

import idaapi as st7_idaapi
import ida_bytes as st7_ida_bytes
import ida_funcs as st7_ida_funcs
import ida_lines as st7_ida_lines
import ida_nalt as st7_ida_nalt
import idautils as st7_idautils
import idc as st7_idc


def st7_instruction(ea):
    """导出单条原始指令的地址、字节与反汇编文本，不改变数据库。"""
    st7_size = st7_idc.get_item_size(ea)
    st7_raw = st7_ida_bytes.get_bytes(ea, st7_size) or b""
    st7_text = st7_ida_lines.tag_remove(st7_idc.generate_disasm_line(ea, 0) or "")
    return {
        "va": hex(ea),
        "rva": hex(ea - st7_base),
        "bytes": st7_raw.hex(" "),
        "asm": st7_text,
    }


def st7_export_window(label, start, end, purpose):
    """按人工选择的地址窗导出指令，用于核对调用点及其邻接分支。"""
    st7_func = st7_ida_funcs.get_func(start)
    st7_row = {
        "label": label,
        "purpose_zh": purpose,
        "window_va": [hex(start), hex(end)],
        "window_rva": [hex(start - st7_base), hex(end - st7_base)],
        "function_va": hex(st7_func.start_ea) if st7_func else None,
        "function_rva": hex(st7_func.start_ea - st7_base) if st7_func else None,
        "instructions": [],
    }
    if st7_func:
        st7_row["instructions"] = [
            st7_instruction(st7_ea)
            for st7_ea in st7_idautils.FuncItems(st7_func.start_ea)
            if start <= st7_ea < end
        ]
    return st7_row


def st7_export_xrefs(label, target):
    """列出目标入口已定义的直接交叉引用，图外调用仍须另行判断。"""
    return {
        "label": label,
        "target_va": hex(target),
        "target_rva": hex(target - st7_base),
        "xrefs_to": [
            {
                "from_va": hex(st7_xref.frm),
                "from_rva": hex(st7_xref.frm - st7_base),
                "type": st7_xref.type,
                "from_function": st7_idc.get_func_name(st7_xref.frm),
                "from_function_va": hex(st7_ida_funcs.get_func(st7_xref.frm).start_ea)
                if st7_ida_funcs.get_func(st7_xref.frm)
                else None,
            }
            for st7_xref in st7_idautils.XrefsTo(target, 0)
        ],
    }


st7_base = st7_ida_nalt.get_imagebase()
st7_out = Path(
    r"F:\projects_isaac\issac_multiplay\artifacts\repentogon\tx-04-07-static\st7-agent"
)
st7_out.mkdir(parents=True, exist_ok=True)

# 指令窗覆盖 StageTransition 的条件解锁与原生 save 调用、Continue 分支、
# Manager 保存条件与 GameState/PGD 调用点；地址均以 IDA VA 表示。
st7_windows = [
    st7_export_window(
        "StageTransition_Update_unlock",
        0x9099DF,
        0x909A55,
        "换层更新中按转场标志与累计次数条件调用成就入口",
    ),
    st7_export_window(
        "StageTransition_Update_save",
        0x909B0D,
        0x909B55,
        "换层更新中调用 Manager_SaveGameState 的邻接指令",
    ),
    st7_export_window(
        "Manager_SaveGameState_guards",
        0x932160,
        0x9321F8,
        "原生保存入口的运行状态、队列、槽位与忙碌条件检查",
    ),
    st7_export_window(
        "Manager_SaveGameState_writes",
        0x932320,
        0x9324A0,
        "两处 GameState::write 与末尾 PGD 云/本地保存分支",
    ),
    st7_export_window(
        "PGD_SaveBackup",
        0x9032A0,
        0x903372,
        "PGD 备份路径生成及备份文件写入 helper 调用",
    ),
    st7_export_window(
        "PGD_SaveBackup_writer",
        0x903380,
        0x90345B,
        "备份文件打开、PGD 序列化、flush/close 的指令证据",
    ),
    st7_export_window(
        "Manager_ExecuteStartGame_continue",
        0x932DD9,
        0x932EF1,
        "Continue/恢复路径；核查是否直接调用 Manager_SaveGameState",
    ),
    st7_export_window(
        "Manager_ExecuteStartGame_new_run_tail",
        0x932FE4,
        0x933020,
        "相邻新 run 分支用于区分 Continue 与 Game::Start",
    ),
    st7_export_window(
        "PlayerManager_death_counter",
        0x993120,
        0x993220,
        "终局死亡 helper 的分支、统计写入与 PGD counter 调用",
    ),
    st7_export_window(
        "Game_Exit_save",
        0x6F635E,
        0x6F64A0,
        "退出时 dirty/fileLoaded 条件、先清 dirty 与保存分支",
    ),
    st7_export_window(
        "PGD_TryUnlock",
        0x903930,
        0x9039C0,
        "PGD 解锁入口的 readonly、索引、已有状态与写入条件",
    ),
    st7_export_window(
        "PGD_IncreaseEventCounter",
        0x903A50,
        0x903B10,
        "PGD 计数入口的 readonly、计数更新、dirty 与阈值解锁调用",
    ),
]

st7_targets = [
    ("Manager_SaveGameState", 0x932160),
    ("GameState_write", 0x99F7A0),
    ("GameState_Delete", 0x99E7B0),
    ("GameState_DeleteRerun", 0x99E870),
    ("Manager_LoadGameState", 0x9332F0),
    ("PGD_TryUnlock", 0x903930),
    ("PGD_IncreaseEventCounter", 0x903A50),
    ("PGD_SaveBackup", 0x9032A0),
    ("PGD_SaveBackup_writer", 0x903380),
]

st7_document = {
    "kind": "read_only_static_export",
    "task": "tx-04-07 ST7",
    "method_zh": "IDA MCP 只读读取 v1.9.7.12 指令与 xref；有限地址窗不代表间接调用闭包",
    "idb": st7_idaapi.get_path(st7_idaapi.PATH_TYPE_IDB),
    "input": st7_ida_nalt.get_input_file_path(),
    "imagebase": hex(st7_base),
    "windows": st7_windows,
    "xref_targets": [st7_export_xrefs(st7_label, st7_ea) for st7_label, st7_ea in st7_targets],
}

st7_file = st7_out / "ida-st7-target-windows.json"
st7_file.write_text(json.dumps(st7_document, ensure_ascii=False, indent=2), encoding="utf-8")
print(json.dumps({"directory": str(st7_out), "file": str(st7_file), "windows": len(st7_windows)}, ensure_ascii=False))
