"""Read-only IDA evidence export for tx-01-68 seed and owner fields.

Run inside the already-open IDA MCP Python context. This script reads the
current IDB and creates one new JSON evidence file. It does not modify the
IDB, target process, or runtime code.
"""

import json
import os

import ida_bytes
import ida_funcs
import ida_hexrays
import ida_ida
import ida_lines
import ida_name
import ida_nalt
import ida_ua
import idautils
import idc


OUT = r"D:\projects\isaac_hook_reserach\artifacts\cs-t0\tx-01-68-static\ida-evidence.json"
IMAGEBASE = ida_nalt.get_imagebase()


def hx(value):
    return "0x%X" % int(value)


def raw_instruction(ea):
    size = idc.get_item_size(ea)
    if not size or size <= 0:
        insn = ida_ua.insn_t()
        size = ida_ua.decode_insn(insn, ea)
    if not size or size <= 0:
        return {"va": hx(ea), "rva": hx(ea - IMAGEBASE), "bytes": "", "instruction": ""}
    data = ida_bytes.get_bytes(ea, size) or b""
    return {
        "va": hx(ea),
        "rva": hx(ea - IMAGEBASE),
        "bytes": " ".join("%02X" % b for b in bytearray(data)),
        "instruction": ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or ""),
    }


def function_record(ea):
    f = ida_funcs.get_func(ea)
    if not f:
        return {"requested_va": hx(ea), "error": "no function at address"}
    start = f.start_ea
    end = f.end_ea
    try:
        pseudocode = ida_lines.tag_remove(str(ida_hexrays.decompile(start)))
    except Exception as exc:
        pseudocode = None
        decompile_error = str(exc)
    else:
        decompile_error = None
    instructions = []
    for item in idautils.FuncItems(start):
        if not ida_bytes.is_code(ida_bytes.get_full_flags(item)):
            continue
        insn = ida_ua.insn_t()
        if ida_ua.decode_insn(insn, item) > 0:
            instructions.append(raw_instruction(item))
    callers = []
    for xref in idautils.XrefsTo(start, 0):
        caller_func = ida_funcs.get_func(xref.frm)
        callers.append({
            "va": hx(xref.frm),
            "rva": hx(xref.frm - IMAGEBASE),
            "function": ida_name.get_name(caller_func.start_ea) if caller_func else "",
        })
    record = {
        "name": ida_name.get_name(start),
        "start_va": hx(start),
        "start_rva": hx(start - IMAGEBASE),
        "end_va_exclusive": hx(end),
        "size": hx(end - start),
        "pseudocode": pseudocode,
        "raw_code_items": instructions,
        "code_xrefs_to_entry": callers,
    }
    if decompile_error:
        record["decompile_error"] = decompile_error
    return record


def point_context(ea, before=7, after=5):
    f = ida_funcs.get_func(ea)
    lo = f.start_ea if f else max(IMAGEBASE, ea - 0x40)
    hi = f.end_ea if f else ea + 0x40
    items = [ea]
    cur = ea
    for _ in range(before):
        cur = idc.prev_head(cur, lo)
        if cur == idc.BADADDR:
            break
        items.append(cur)
    cur = ea
    for _ in range(after):
        cur = idc.next_head(cur, hi)
        if cur == idc.BADADDR or cur >= hi:
            break
        items.append(cur)
    result = []
    for item in sorted(set(items)):
        if ida_bytes.is_code(ida_bytes.get_full_flags(item)):
            insn = ida_ua.insn_t()
            if ida_ua.decode_insn(insn, item) > 0:
                result.append(raw_instruction(item))
    return result


function_addresses = [
    0x866960,  # Entity root Lua binding registration
    0x505B70, 0x5B1590, 0x4174B0, 0x4174A0, 0x40C3A0,  # Entity seed/RNG accessors
    0x42A260, 0x431560, 0x4B7080,  # Parent/Child/Target getters
    0x6A92A0, 0x6A92E0, 0x6A9320, 0x6A9360,  # tracked-reference setters
    0x6A8990, 0x4CF210, 0x4CDCF0,  # GetLastParent and ref bookkeeping helpers
    0x6A8290, 0x6A93A0, 0x6ACB00, 0x6ACB60,  # Entity ctor/Init/Remove/ClearReferences
    0x6FE410,  # Spawn writes SpawnerEntity before virtual Init
    0x755470, 0x6B8B20, 0x6660A0, 0x652B90, 0x6E02F0, 0x60DE70, 0x5CDB20,
    0x4361F0, 0x62E020,  # Familiar.Player getter/setter
    0x709C40, 0x709DB0, 0x4176F0,  # Grid ctor/Init and direct State cross-check
    0x88A270, 0x88A3E0, 0x8C0680, 0x8C0620, 0x8C06E0, 0x8C0740,
    0x8765A0,  # Grid field helpers and RNG method registration helper
]

entity_init_functions = [
    ("Player", 0x755470),
    ("NPC", 0x6B8B20),
    ("Tear", 0x6660A0),
    ("Projectile", 0x652B90),
    ("Pickup", 0x6E02F0),
    ("Familiar", 0x60DE70),
    ("Effect", 0x5CDB20),
]

binding_sites = [
    ("Entity.Parent", 0x868EBE),
    ("Entity.Child", 0x868ED4),
    ("Entity.Target", 0x868EEA),
    ("Entity.SpawnerEntity", 0x868F00),
    ("Entity.GetLastParent", 0x868F11),
    ("Entity.InitSeed", 0x868F66),
    ("Entity.DropSeed", 0x868F77),
    ("Entity.GetDropRNG", 0x868F88),
    ("RNG.GetSeed", 0x867769),
    ("Familiar.Player", 0x86A82B),
    ("GridEntityDesc.SpawnSeed", 0x86B5AE),
    ("GridEntityDesc.Initialized", 0x86B5CE),
    ("GridEntityDesc.VariableSeed", 0x86B5DE),
    ("GridEntityDesc.State offset cross-check", 0x86B595),
]

instruction_anchors = [
    ("Entity ctor: tracked-reference zero fields", 0x6A8752),
    ("Entity ctor: embedded RNG default words", 0x6A87C5),
    ("Entity Init: InitSeed write", 0x6A93C7),
    ("Entity Init: Drop RNG seed/state writes", 0x6A93CD),
    ("Entity Init: Parent zero", 0x6A9753),
    ("Entity Init: Child zero", 0x6A975D),
    ("Entity Init: Target zero", 0x6A9767),
    ("Spawn: address of SpawnerEntity slot", 0x6FE5A3),
    ("Spawn: write SpawnerEntity slot", 0x6FE5C6),
    ("Spawn: virtual Init call", 0x6FE610),
    ("Familiar Init: read SpawnerEntity", 0x60DEEE),
    ("Familiar Init: write Player field", 0x60DF0B),
    ("Familiar.Player setter: raw store", 0x62E09B),
    ("Entity ClearReferences: Parent", 0x6ACB69),
    ("Entity ClearReferences: Child", 0x6ACB8B),
    ("Entity ClearReferences: Target", 0x6ACBB3),
    ("Entity ClearReferences: SpawnerEntity", 0x6ACBDB),
    ("Grid ctor: SpawnSeed default", 0x709C7C),
    ("Grid ctor: Initialized false", 0x709C8A),
    ("Grid ctor: VariableSeed default", 0x709C8E),
    ("Grid Init: SpawnSeed argument store", 0x709DB9),
    ("Grid Init: derived virtual Init", 0x709DC5),
    ("Grid Init: Initialized true after virtual call", 0x709DD0),
    ("Grid direct State getter confirms GridDesc +4 mapping", 0x4176F0),
]

evidence = {
    "task": "tx-01-68-seed-owner",
    "collection_mode": "read-only current-IDB extraction",
    "binary": {
        "idb_path": ida_nalt.get_input_file_path() + ".i64",
        "input_path": ida_nalt.get_input_file_path(),
        "imagebase": hx(IMAGEBASE),
        "processor": ida_ida.inf_get_procname(),
        "build_banner_va": "0xB7E1B4",
        "build_banner": (idc.get_strlit_contents(0xB7E1B4, -1, idc.STRTYPE_C) or b"").decode("latin1").strip(),
    },
    "functions": [function_record(ea) for ea in function_addresses],
    "entity_derived_init_calls": [],
    "binding_contexts": [],
    "instruction_contexts": [],
    "scope_notes": [
        "Function raw_code_items contain VA, RVA, bytes, and disassembly for decoded code items only; embedded data is omitted.",
        "The seven listed Entity-derived Init functions are included to preserve the current direct calls into Entity::Init; they do not imply complete reset, Morph, Restore, or reuse coverage.",
        "GridEntityDesc seed access is captured as object-relative offsets using the current Grid/Desc +4 mapping; a matching Grid.State registration/getter cross-check is included.",
        "Reference values are stored in four-byte object slots. The evidence does not follow pointer targets or establish cross-tick/process identity.",
        "No game execution, target memory read, injection, debugger operation, build, test, hash, or IDB mutation is performed.",
    ],
}

for category, ea in entity_init_functions:
    f = ida_funcs.get_func(ea)
    calls = []
    if f:
        for item in idautils.FuncItems(f.start_ea):
            if not ida_bytes.is_code(ida_bytes.get_full_flags(item)):
                continue
            if idc.print_insn_mnem(item).lower() == "call" and idc.get_operand_value(item, 0) == 0x6A93A0:
                calls.append({"call": raw_instruction(item), "context": point_context(item, 4, 4)})
    evidence["entity_derived_init_calls"].append({"category": category, "init_va": hx(ea), "base_init_va": "0x6A93A0", "calls": calls})

for label, ea in binding_sites:
    evidence["binding_contexts"].append({"label": label, "anchor_va": hx(ea), "context": point_context(ea, 7, 4)})

for label, ea in instruction_anchors:
    evidence["instruction_contexts"].append({"label": label, "anchor_va": hx(ea), "context": point_context(ea, 5, 8)})

if os.path.exists(OUT):
    raise RuntimeError("Refusing to overwrite existing evidence file: " + OUT)
os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "x", encoding="utf-8", newline="\n") as stream:
    json.dump(evidence, stream, ensure_ascii=False, indent=2)
    stream.write("\n")
print("Wrote new evidence file:", OUT)
print("Functions:", len(evidence["functions"]), "binding contexts:", len(evidence["binding_contexts"]), "instruction contexts:", len(evidence["instruction_contexts"]))
