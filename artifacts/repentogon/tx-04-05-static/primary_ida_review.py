"""主智能体独立读取 IDB 的重点证据；只读静态导出，不修改 IDB 或运行目标。"""
from pathlib import Path
import ida_bytes
import ida_funcs
import ida_hexrays
import ida_nalt
import ida_lines
import idautils
import idc

OUT = Path(r"F:\projects_isaac\issac_multiplay\artifacts\repentogon\tx-04-05-static")
BASE = ida_nalt.get_imagebase()
lines = [f"IMAGEBASE={BASE:#x}", f"INPUT={ida_nalt.get_input_file_path()}"]


def section(title, start, end):
    """保留关键指令的地址和原字节，避免把伪代码类型推断当成实证。"""
    lines.append(f"\n[{title}] RVA {start:#x}..{end:#x}")
    for ea in idautils.Heads(BASE + start, BASE + end):
        raw = ida_bytes.get_bytes(ea, idc.get_item_size(ea)) or b""
        asm = ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or "")
        lines.append(f"RVA {ea-BASE:#010x} VA {ea:#010x} {raw.hex(' ')}  {asm}")


def function(title, rva):
    """独立导出小函数的伪代码及完整汇编；无反编译结果时明确保留事实。"""
    ea = BASE + rva
    fn = ida_funcs.get_func(ea)
    lines.append(f"\n[{title} pseudocode]")
    try:
        code = ida_hexrays.decompile(ea)
        lines.append(str(code) if code else "Hex-Rays returned None; assembly is the evidence.")
    except Exception as exc:
        lines.append(f"Hex-Rays unavailable: {exc}")
    if fn:
        section(title, fn.start_ea - BASE, fn.end_ea - BASE)


section("T1 special updates", 0x2F93D0, 0x2F9412)
section("T1 second special update", 0x2F9473, 0x2F94C0)
section("T1 ordinary update", 0x2F94E7, 0x2F9589)
function("T2 lookup by device id", 0x5EEC40)
for cls, table in [("keyboard", 0x73B490), ("gamepad", 0x73B3F0)]:
    for slot in (5, 6, 7, 8, 9, 10, 35):
        target = ida_bytes.get_dword(BASE + table + slot * 4)
        lines.append(f"VTABLE {cls} slot={slot} RVA={target-BASE:#x}")
        if slot in (5, 6, 7):
            function(f"T3 {cls} raw query {slot}", target - BASE)
function("T3 gamepad mapped button state", 0x5ED190)
function("T3 device common update", 0x5EC900)
function("T5 pause/loading predicate", 0x2F92A0)
function("T5 PauseScreen predicate", 0x58ED50)
function("T5 auxiliary state predicate", 0x42CB90)
section("T5 Game console branch", 0x2F6D30, 0x2F6D90)
section("T5 Game focus branch", 0x2F7690, 0x2F7710)
section("T5 Odd focus branch", 0x2F94F6, 0x2F951E)
section("T5 focus field writer A", 0x52BA80, 0x52BAE0)
section("T5 focus field writer B", 0x581AA0, 0x581B00)
section("T5 focus field writer C", 0x58D3C0, 0x58D470)
section("T5 focus callback registration", 0x5821E0, 0x58222A)
function("T5 focus callback dispatch", 0x5E4910)
function("T5 focus flag setter", 0x581AC0)
section("T5 window callback installation", 0x5E4C70, 0x5E4CA0)
section("T5 WM_ACTIVATE dispatch entry", 0x629010, 0x6290B8)
section("T5 WM_ACTIVATE focus true false", 0x629108, 0x62919E)
section("T5 pause open writes state", 0x58EE11, 0x58EE3D)
section("T5 PauseScreen branch", 0x2F7730, 0x2F77B0)
section("T5 ordinary Room dispatch", 0x2F7850, 0x2F7950)
section("T6 new-room callback neighborhood", 0x339650, 0x3397A0)
section("T6 conditional pre-callback reposition", 0x3395A1, 0x3395BB)
section("T6 reposition per-player call", 0x592C50, 0x592C98)
section("T6 player position writes", 0x39ED00, 0x39ED80)
section("T6 relative player position adjustment", 0x592E60, 0x592EFE)
section("T6 caller after callback owner", 0x33A737, 0x33A770)
section("T6 nested Room after callback", 0x2FA160, 0x2FA187)
section("T6 ChangeRoom load call before later positioning", 0x4256D0, 0x425738)
section("T6 ChangeRoom mode11 PlayerManager list", 0x425A40, 0x425A90)
section("T6 ChangeRoom mode11 later position writes", 0x425B4E, 0x425B8B)
section("T6 ChangeRoom mode20 PlayerManager list", 0x425DA5, 0x425DF8)
section("T6 ChangeRoom mode20 later position writes", 0x425E7F, 0x425E9F)
section("T6 Lua StartRoomTransition registration", 0x47ACF1, 0x47AD31)
function("T6 Lua StartRoomTransition bridge", 0x49EEB0)
function("T6 Game StartRoomTransition native", 0x2F9710)
function("T6 RoomTransition argument setter", 0x423990)
section("T6 ChangeRoom animation selector", 0x4256D5, 0x425738)
section("T6 ChangeRoom selected player", 0x425891, 0x4258B1)
section("T6 ChangeRoom animation3 player helper", 0x425F96, 0x42601F)
section("T6 ChangeRoom animation20 familiar list", 0x425EE1, 0x425F3F)
section("T6 ChangeRoom animation3 familiar list", 0x4260FF, 0x426158)
section("T6 player coordinate helper direct stores", 0x3C3E60, 0x3C3E9E)
function("T6 secondary room positioning caller", 0x3415E0)
section("T7 shooting byte write", 0x379490, 0x3794E0)
section("T7 direction write", 0x3728C0, 0x372970)
function("T8 movement input projection", 0x371D70)
function("T8 Manager scalar wrapper", 0x52F7B0)
function("T8 InputManager scalar", 0x5EE2E0)
function("T8 Manager pressed wrapper", 0x52F610)
function("T8 Manager triggered wrapper", 0x52F6D0)
section("T8 movement Triggered action table use", 0x373850, 0x373893)
section("T8 shooting Triggered action table use", 0x375FD3, 0x37602C)
for table in (0x741020, 0x741120):
    values = [ida_bytes.get_dword(BASE + table + i * 4) for i in range(4)]
    lines.append(f"T8 action table RVA={table:#x} uint32[4]={values}")
lines.append("\n[T6 independent xrefs]")
for target in (0x3385F0, 0x44B930):
    for ref in idautils.XrefsTo(BASE + target):
        owner = ida_funcs.get_func(ref.frm)
        owner_rva = hex(owner.start_ea - BASE) if owner else "UNKNOWN"
        lines.append(f"target={target:#x} callsite={ref.frm-BASE:#x} owner={owner_rva}")
(OUT / "primary-ida-evidence.txt").write_text("\n".join(lines), encoding="utf-8")
print(f"Primary static evidence saved: {len(lines)} lines.")
