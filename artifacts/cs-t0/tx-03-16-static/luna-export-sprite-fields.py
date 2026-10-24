# Read-only IDA exporter for tx-03-16 Sprite draw/transform field flow.
# This script only reads the active IDB and writes a new JSON artifact.
import json
import os
from pathlib import Path

import ida_bytes
import ida_funcs
import ida_hexrays
import ida_lines
import ida_nalt
import idaapi
import idautils
import idc


BASE = idaapi.get_imagebase()
DEFAULT_OUTPUT = Path(r"F:\projects_isaac\issac_multiplay\artifacts\cs-t0\tx-03-16-static\luna-draw-fields.json")
# Set this global before executing the file, or set the same-named environment
# variable, to write a fresh export elsewhere. The selected path remains create-new.
TX0316_LUNA_OUTPUT = globals().get("TX0316_LUNA_OUTPUT") or os.environ.get("TX0316_LUNA_OUTPUT")
OUTPUT = Path(TX0316_LUNA_OUTPUT) if TX0316_LUNA_OUTPUT else DEFAULT_OUTPUT
FUNCTIONS = [
    # Requested draw/dispatch functions.
    0x409120, 0x4098A0, 0x409CB0, 0x40A0D0, 0x40A140,
    # Direct dispatch and transform helpers needed to interpret their dataflow.
    0x40A030, 0x40A080, 0x4071C0, 0x4072E0, 0x40C6F0,
    0xA10760, 0xA10950, 0xA10D00, 0xA0F550,
    0x408590, 0x40C440, 0x40C550, 0x682260, 0x683060, 0xA112C0,
    # Native Sprite property accessors used to map the values read by draw.
    0x456340, 0x49CB40, 0x420170, 0x417200, 0x43EE80, 0x417210,
    0x4073D0, 0x407410, 0x435C60, 0x407420, 0x505B50, 0x407430,
]
XREF_TARGETS = [0x409120, 0x40A030, 0x40A080, 0x40A0D0, 0x40A140]
MAX_XREFS_PER_TARGET = 256


def hx(value):
    return hex(value) if value is not None else None


def instruction(ea):
    size = idc.get_item_size(ea) or 1
    raw = ida_bytes.get_bytes(ea, size) or b""
    return {
        "va": hx(ea),
        "rva": hx(ea - BASE),
        "bytes": raw.hex(" "),
        "size": size,
        "assembly": ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or ""),
        "code_refs_from": [hx(x) for x in idautils.CodeRefsFrom(ea, False)],
        "data_refs_from": [hx(x) for x in idautils.DataRefsFrom(ea)],
    }


def function_record(address):
    fn = ida_funcs.get_func(address)
    row = {
        "requested_va": hx(address),
        "requested_rva": hx(address - BASE),
        "name_at_request": idc.get_name(address),
    }
    if not fn:
        row["function_found"] = False
        row["failure"] = "IDA has no function at requested address"
        return row

    chunks = list(idautils.Chunks(fn.start_ea))
    items = [ea for ea in idautils.FuncItems(fn.start_ea) if ida_bytes.is_code(ida_bytes.get_flags(ea))]
    row.update({
        "function_found": True,
        "va": hx(fn.start_ea),
        "rva": hx(fn.start_ea - BASE),
        "end_va": hx(fn.end_ea),
        "end_rva": hx(fn.end_ea - BASE),
        "size": hx(fn.end_ea - fn.start_ea),
        "name": ida_funcs.get_func_name(fn.start_ea),
        "chunks": [{"start_va": hx(start), "start_rva": hx(start - BASE),
                    "end_va": hx(end), "end_rva": hx(end - BASE)}
                   for start, end in chunks],
        "raw_chunk_bytes": [],
        "instructions": [instruction(ea) for ea in items],
        "pseudocode": None,
    })
    for start, end in chunks:
        try:
            raw = ida_bytes.get_bytes(start, end - start)
            row["raw_chunk_bytes"].append({
                "start_va": hx(start), "start_rva": hx(start - BASE),
                "end_va": hx(end), "end_rva": hx(end - BASE),
                "bytes": raw.hex(" ") if raw is not None else None,
                "failure": None if raw is not None else "IDA returned no bytes for chunk",
            })
        except Exception as exc:
            row["raw_chunk_bytes"].append({
                "start_va": hx(start), "start_rva": hx(start - BASE),
                "end_va": hx(end), "end_rva": hx(end - BASE),
                "bytes": None, "failure": repr(exc),
            })
    try:
        cfunc = ida_hexrays.decompile(fn.start_ea)
        row["pseudocode"] = (
            "\n".join(ida_lines.tag_remove(line.line) for line in cfunc.get_pseudocode())
            if cfunc else None
        )
        if cfunc is None:
            row["pseudocode_failure"] = "Hex-Rays returned no cfunc"
    except Exception as exc:
        row["pseudocode_failure"] = repr(exc)
    return row


def xrefs_to(address):
    refs = list(idautils.XrefsTo(address))
    capped = refs[:MAX_XREFS_PER_TARGET]
    return {
        "target_va": hx(address),
        "target_rva": hx(address - BASE),
        "xref_count": len(refs),
        "truncated": len(refs) > len(capped),
        "xrefs": [{
            "from_va": hx(x.frm),
            "from_rva": hx(x.frm - BASE),
            "type": x.type,
            "is_code": bool(x.iscode),
            "source_function": ida_funcs.get_func_name(x.frm),
        } for x in capped],
    }


payload = {
    "tx": "tx-03-16",
    "subtask": "Luna Sprite draw fields",
    "evidence": "STATIC_IDB_READ_ONLY",
    "input_path": ida_nalt.get_input_file_path(),
    "idb_path": idc.get_idb_path(),
    "imagebase": hx(BASE),
    "rva_rule": "RVA = VA - imagebase; object offsets, frame indices, track indices, and layer ordinals are not RVAs",
    "functions": [function_record(ea) for ea in FUNCTIONS],
    "entry_xrefs": [xrefs_to(ea) for ea in XREF_TARGETS],
    "limitations": [
        "Static IDB evidence only; no runtime draw or pixel behavior observed",
        "No hashes, gates, or runtime helpers were used",
        "Indirect virtual draw targets and their implementations are not resolved here",
        "Decompiler types are not a complete ABI contract; raw instructions are retained",
        "Native constructor defaults and caller-specific rendering parameters remain unknown unless stated in the report",
    ],
}

if OUTPUT.exists():
    raise RuntimeError("Refusing to overwrite existing output: %s" % OUTPUT)
OUTPUT.parent.mkdir(parents=True, exist_ok=True)
with OUTPUT.open("x", encoding="utf-8") as stream:
    json.dump(payload, stream, ensure_ascii=False, indent=2)
print(json.dumps({
    "output": str(OUTPUT),
    "functions": len(payload["functions"]),
    "xref_targets": len(payload["entry_xrefs"]),
    "imagebase": hx(BASE),
}))
