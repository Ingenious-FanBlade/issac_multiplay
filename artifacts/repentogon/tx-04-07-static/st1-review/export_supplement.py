"""ST1 主审补充静态导出：死亡计数、HUD、对象字段及官方函数线索。"""
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


def export_st1_supplement():
    """只读采集补充函数，不运行目标、不设断点、不修改 IDB。"""
    out = Path(r"F:\projects_isaac\issac_multiplay\artifacts\repentogon\tx-04-07-static\st1-review")
    base = ida_nalt.get_imagebase()
    targets = {
        "Lua_EntityKill": 0x844230,
        "PlayerManager_DeathHandler": 0x993120,
        "Persistent_IncreaseEventCounter": 0x903A50,
        "PlayerHUD_RenderHearts": 0x834930,
        "Player_LoadGhostSprite": 0x7D03C0,
        "Manager_DeathRunState": 0x9324A0,
        "Manager_DeathRunBookkeeping": 0x933C10,
        "Player_TryPreventDeath": 0x7C0DC0,
        "Entity_KillHelper": 0x6AFA80,
        "LuaEngine_PostEntityKill": 0x84CB30,
        "TemporaryEffects_RemoveNullEffect": 0x90A3F0,
        "Player_EvaluateItems": 0x75D4B0,
    }
    for label, ea in targets.items():
        entry = {"label": label, "va": hex(ea), "rva": hex(ea-base),
                 "decompile": str(ida_hexrays.decompile(ea)),
                 "instructions": [
                     {"va": hex(ins), "rva": hex(ins-base),
                      "bytes": (ida_bytes.get_bytes(ins, idc.get_item_size(ins)) or b"").hex(" "),
                      "text": ida_lines.tag_remove(idc.generate_disasm_line(ins, 0) or "")}
                     for ins in idautils.FuncItems(ea)],
                 "xrefs_to": [{"from_va": hex(x.frm), "type": x.type}
                              for x in idautils.XrefsTo(ea, 0)]}
        (out / (label + ".json")).write_text(json.dumps(entry, ensure_ascii=False, indent=2), encoding="utf-8")
    details = {
        "version_strings": [{"va": hex(s.ea), "text": str(s)} for s in idautils.Strings()
                            if "1.9.7.12" in str(s) or "J273" in str(s)],
        "player_vtable": {"va": "0xb1bdb4", "kill_slot9": hex(ida_bytes.get_dword(0xB1BDB4+9*4))},
        "lua_kill_registration": [{"va": hex(ea), "text": ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or "")}
                                  for ea in idautils.Heads(0x84F6B9, 0x84F6EB)],
        "death_event_direct_xref_candidates": [],
    }
    # 仅作为直接调用线索：向前十二条找 push 10，不声称覆盖间接或计算参数。
    for ref in idautils.XrefsTo(0x903A50, 0):
        seq = []
        cursor = ref.frm
        for _ in range(12):
            cursor = idc.prev_head(cursor)
            seq.append(cursor)
        if any(idc.print_insn_mnem(ea) == "push" and idc.get_operand_type(ea, 0) == 5
               and idc.get_operand_value(ea, 0) == 10 for ea in seq):
            details["death_event_direct_xref_candidates"].append({
                "call_va": hex(ref.frm),
                "context": [{"va": hex(ea), "text": ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or "")}
                            for ea in list(reversed(seq)) + [ref.frm]]})
    (out / "supplement-details.json").write_text(json.dumps(details, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({"directory": str(out), "supplement_function_count": len(targets)}, ensure_ascii=False))


export_st1_supplement()
