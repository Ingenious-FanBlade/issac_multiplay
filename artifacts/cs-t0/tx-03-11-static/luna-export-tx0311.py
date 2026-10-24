# IDA MCP py_exec_file read-only IDB exporter for tx-03-11 render-boundary evidence.
# Use py_exec_file so functions/imports share one execution namespace.
# Writes one create-new evidence JSON beside this script; no target execution or IDB edits.
import idaapi
import ida_nalt
import ida_loader
import ida_funcs
import ida_bytes
import ida_hexrays
import ida_lines
import idautils
import idc
import json
import re
from pathlib import Path

IMAGEBASE = idaapi.get_imagebase()
FUNCTIONS = [
    0x6FBC10, 0x80EA80, 0x80F200, 0x832850, 0x419B50,
    0x6B2000, 0x6B10F0, 0x863680, 0x872980,
]
IMPORTANT_SITES = [
    0x6FC094, 0x6FC0A1, 0x6FC0A6, 0x80EAE6, 0x80EAEC, 0x80EAED,
    0x80EAF3, 0x419C61, 0x419C63, 0x419C64, 0x419C67,
    0x80FD69, 0x80FE61, 0x81001E, 0x8100B5, 0x810496, 0x8104F7,
    0x8109AF, 0x810ACE, 0x810CB8, 0x810D0D, 0x811D90, 0x8121DE,
    0x812625, 0x832E50, 0x832E65, 0x832EDF, 0x8636DB, 0x8729CB,
]
XREF_TARGETS = [0x80EA80, 0x80F200, 0x832850, 0x6B2000, 0x6B10F0, 0x863680, 0x872980]
ENTITY_VTABLE = 0xB6730C

def _hex(value):
    return hex(value) if value is not None else None

def _function_record(ea):
    f = ida_funcs.get_func(ea)
    if not f:
        return {"va": _hex(ea), "rva": _hex(ea - IMAGEBASE), "name": idc.get_name(ea), "function": False}
    size = f.end_ea - f.start_ea
    b = ida_bytes.get_bytes(f.start_ea, min(size, 0x40)) or b""
    return {
        "va": _hex(f.start_ea),
        "rva": _hex(f.start_ea - IMAGEBASE),
        "name": idc.get_name(f.start_ea),
        "size": _hex(size),
        "entry_bytes_first_0x40_max": b.hex(" "),
    }

def _instruction(ea):
    size = idc.get_item_size(ea) or 1
    raw = ida_bytes.get_bytes(ea, size) or b""
    return {
        "va": _hex(ea),
        "rva": _hex(ea - IMAGEBASE),
        "bytes": raw.hex(" "),
        "mnemonic": idc.print_insn_mnem(ea),
        "operand0": idc.print_operand(ea, 0) or "",
        "assembly": idc.generate_disasm_line(ea, 0) or "",
    }

def _function_items(start):
    f = ida_funcs.get_func(start)
    if not f:
        return []
    return [ea for ea in idautils.FuncItems(f.start_ea) if idc.is_code(idc.get_full_flags(ea))]

def _near(ea, before=5, after=3):
    f = ida_funcs.get_func(ea)
    if not f:
        return []
    items = _function_items(f.start_ea)
    try:
        i = items.index(ea)
    except ValueError:
        return []
    lo = max(0, i - before)
    hi = min(len(items), i + after + 1)
    return [_instruction(x) for x in items[lo:hi]]

def _calls(start):
    rows = []
    for ea in _function_items(start):
        if (idc.print_insn_mnem(ea) or "").lower() != "call":
            continue
        op = (idc.print_operand(ea, 0) or "").strip()
        indirect = ("[" in op) or op.lower() in ("eax", "ebx", "ecx", "edx", "esi", "edi")
        target = None if indirect else idc.get_operand_value(ea, 0)
        refs = [_hex(x) for x in idautils.CodeRefsFrom(ea, False)]
        name = idc.get_name(target) if target else ""
        rows.append({
            "va": _hex(ea),
            "rva": _hex(ea - IMAGEBASE),
            "bytes": (ida_bytes.get_bytes(ea, idc.get_item_size(ea) or 1) or b"").hex(" "),
            "operand0": op,
            "indirect": indirect,
            "direct_target_va": _hex(target) if target else None,
            "direct_target_rva": _hex(target - IMAGEBASE) if target else None,
            "direct_target_name": name,
            "code_refs": refs,
            "lua_named_target": ("lua" in name.lower() or "pcall" in name.lower()) if not indirect else False,
            "context": _near(ea, 5, 3) if indirect else [],
        })
    return rows

def _xrefs_to(ea):
    rows = []
    for xr in idautils.XrefsTo(ea):
        fr = xr.frm
        f = ida_funcs.get_func(fr)
        rows.append({
            "from_va": _hex(fr),
            "from_rva": _hex(fr - IMAGEBASE),
            "type": int(xr.type),
            "from_function_va": _hex(f.start_ea) if f else None,
            "from_function_name": idc.get_name(f.start_ea) if f else None,
        })
    return rows

def _vtable_words(start, count):
    rows = []
    for i in range(count):
        ea = start + i * 4
        value = ida_bytes.get_dword(ea)
        rows.append({
            "entry_va": _hex(ea),
            "entry_rva": _hex(ea - IMAGEBASE),
            "byte_offset_from_vtable": _hex(i * 4),
            "value_va": _hex(value),
            "value_rva": _hex(value - IMAGEBASE),
            "value_name": idc.get_name(value),
        })
    raw = ida_bytes.get_bytes(start, count * 4) or b""
    return {"vtable_va": _hex(start), "vtable_rva": _hex(start - IMAGEBASE),
            "symbol": idc.get_name(start), "raw_bytes": raw.hex(" "), "entries": rows}

def _direct_store_excerpt(start):
    c = ida_hexrays.decompile(start)
    lines = [ida_lines.tag_remove(sl.line) for sl in c.get_pseudocode()] if c else []
    assignment = re.compile(r"(?<![<>=!])=(?!=)")
    rows = []
    for line in lines:
        stripped = line.strip()
        if not assignment.search(stripped) or stripped.startswith(("if ", "if(", "for ", "while ", "switch ", "return ")):
            continue
        lhs = stripped.split("=", 1)[0].strip()
        if ("*(" in lhs or "[" in lhs or "->" in lhs or
                re.search(r"\b(?:xmmword|dword|byte|word|qword)_[A-Za-z0-9]+", lhs)):
            rows.append(stripped)
    return {
        "pseudocode_line_count": len(lines),
        "candidate_store_count": len(rows),
        "candidate_store_lines": rows,
        "caveat": "Filtered Hex-Rays assignment-shaped lvalues; variable names and pointer ownership require contextual interpretation.",
    }

def export_tx0311():
    direct_lua = {}
    calls = {}
    for ea in FUNCTIONS:
        calls[_hex(ea)] = _calls(ea)
        direct_lua[_hex(ea)] = [
            row for row in calls[_hex(ea)]
            if row["lua_named_target"]
        ]
    return {
        "scope": "Read-only static IDB export for tx-03-11; no target execution, no debugger, no IDB mutation, create-new evidence file only, no hashes.",
        "database": {
            "input_file": ida_nalt.get_input_file_path(),
            "idb_path": ida_loader.get_path(ida_loader.PATH_TYPE_IDB),
            "imagebase": _hex(IMAGEBASE),
            "ida_version": getattr(idaapi, "IDA_SDK_VERSION", None),
        },
        "functions": [_function_record(ea) for ea in FUNCTIONS],
        "important_site_contexts": {_hex(ea): _near(ea, 5, 3) for ea in IMPORTANT_SITES},
        "entity_base_vtable": _vtable_words(ENTITY_VTABLE, 12),
        "direct_store_excerpts": {_hex(ea): _direct_store_excerpt(ea) for ea in [0x80EA80, 0x80F200, 0x832850, 0x6B2000]},
        "xrefs_to": {_hex(ea): _xrefs_to(ea) for ea in XREF_TARGETS},
        "call_inventory": calls,
        "direct_lua_named_targets": direct_lua,
    }

output_path = Path(globals().get('TX0311_LUNA_OUTPUT', str(Path(__file__).with_name('luna-render-export.json'))))
with output_path.open('x', encoding='utf-8') as evidence_file:
    json.dump(export_tx0311(), evidence_file, ensure_ascii=False, indent=2)
print(json.dumps({'output': str(output_path), 'functions': len(FUNCTIONS)}, ensure_ascii=False))

