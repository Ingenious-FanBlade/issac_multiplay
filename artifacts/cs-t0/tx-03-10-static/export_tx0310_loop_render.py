# IDA Python read-only exporter for tx-03-10 loop/render evidence.
# This script reads the current IDB and input PE headers only. It does not
# patch bytes, rename items, save the database, run the target, or calculate hashes.

import json
import os
import struct

import ida_bytes
import ida_funcs
import ida_hexrays
import ida_nalt
import idaapi
import idc

IMAGEBASE = idaapi.get_imagebase()
INPUT_PATH = ida_nalt.get_input_file_path()
OUT_PATH = os.path.abspath(globals().get(
    "TX0310_EXPORT_OUTPUT",
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "loop-render-export.json"),
))

CALL_SITES = [
    0x955231, 0x955241, 0x6FBA4D, 0x803E70, 0x6FB51C, 0x831941,
    0x6FBBA5, 0x9559C1, 0x6FC094, 0x6FC0A1, 0x6FC0A6, 0x6FC39F,
    0x6FC5B7, 0x6FC5C2, 0x6FC5CD, 0x6FC5D8, 0x6FC5E3, 0x6FC5EE,
    0x8319C9, 0x831A11, 0x831A31,
]
FUNCTIONS = [
    0x954CD0, 0x6FADC0, 0x6FD3F0, 0x6B0EB0, 0x6FBC10, 0x80EA80,
    0x80F200, 0x832850, 0x4186C0, 0x8318A0, 0x6AB440, 0x9A3EB0,
    0x9B7190, 0x408E00, 0x40A1B0, 0x40A5D0, 0x8607A0, 0x863680,
    0x872980, 0x802980,
]
BYTE_RANGES = [
    (0x955231, 16), (0x955241, 16), (0x6FBA4D, 16), (0x803E62, 24),
    (0x6FBBA5, 12), (0x9559C1, 12), (0x6FD609, 20), (0x6FD62D, 20),
    (0x6FD77D, 20), (0x6B0EF8, 96), (0x6FC0A1, 20), (0x6FC5B7, 64),
    (0x83199B, 28), (0x8319C5, 12), (0x831A0D, 12), (0x831A2D, 12),
    (0x408E00, 128), (0x80EAE6, 24), (0x872980, 160),
]

RETURN_ADDRESS_MAP = [
    {"return_rva": "0x4319CC", "return_va": "0x8319CC", "call_va": "0x8319C9"},
    {"return_rva": "0x431A14", "return_va": "0x831A14", "call_va": "0x831A11"},
    {"return_rva": "0x431A34", "return_va": "0x831A34", "call_va": "0x831A31"},
]

def read_pe_headers(path):
    try:
        with open(path, "rb") as stream:
            dos = stream.read(64)
            if len(dos) < 64 or dos[:2] != b"MZ":
                return {"error": "DOS header unavailable"}
            pe_offset = struct.unpack_from("<I", dos, 0x3C)[0]
            stream.seek(pe_offset)
            signature = stream.read(4)
            coff = stream.read(20)
            if signature != b"PE\0\0" or len(coff) != 20:
                return {"error": "PE header unavailable"}
            timestamp = struct.unpack_from("<I", coff, 4)[0]
            optional_size = struct.unpack_from("<H", coff, 16)[0]
            optional = stream.read(optional_size)
            if len(optional) < 60:
                return {"error": "Optional header incomplete"}
            return {
                "pe_offset": hex(pe_offset),
                "timestamp": hex(timestamp),
                "optional_magic": hex(struct.unpack_from("<H", optional, 0)[0]),
                "size_of_image": hex(struct.unpack_from("<I", optional, 56)[0]),
            }
    except Exception as exc:
        return {"error": str(exc)}

def instruction_record(ea):
    instruction_size = ida_bytes.get_item_size(ea)
    raw = ida_bytes.get_bytes(ea, instruction_size)
    return {
        "va": hex(ea),
        "rva": hex(ea - IMAGEBASE),
        "instruction_size": instruction_size,
        "mnemonic": idc.print_insn_mnem(ea),
        "operand0": idc.print_operand(ea, 0),
        "operand0_value": hex(idc.get_operand_value(ea, 0)),
        "bytes": raw.hex(" ") if raw else None,
        "disassembly": idc.generate_disasm_line(ea, 0),
    }

def function_record(ea):
    function = ida_funcs.get_func(ea)
    if function is None:
        return {"va": hex(ea), "error": "No function"}
    try:
        pseudocode = str(ida_hexrays.decompile(function.start_ea))
    except Exception as exc:
        pseudocode = None
        error = str(exc)
    else:
        error = None
    return {
        "va": hex(function.start_ea),
        "rva": hex(function.start_ea - IMAGEBASE),
        "end": hex(function.end_ea),
        "size": hex(function.end_ea - function.start_ea),
        "name": idc.get_func_name(function.start_ea),
        "pseudocode": pseudocode,
        "decompile_error": error,
    }

result = {
    "kind": "tx-03-10-static-loop-render-export",
    "scope": "Read-only IDB extraction; no game execution, patching, IDB rename/save, debugger control, or hash calculation.",
    "input_path": INPUT_PATH,
    "imagebase": hex(IMAGEBASE),
    "idb_imagebase_bytes": (ida_bytes.get_bytes(IMAGEBASE, 16) or b"").hex(" "),
    "pe_headers": read_pe_headers(INPUT_PATH),
    "pe_header_scope": "FILE_METADATA_ONLY; disk input-path headers do not establish byte identity with the mapped IDB.",
    "return_address_map": RETURN_ADDRESS_MAP,
    "call_sites": [instruction_record(ea) for ea in CALL_SITES],
    "byte_ranges": [
        {
            "va": hex(ea),
            "rva": hex(ea - IMAGEBASE),
            "size": size,
            "bytes": (ida_bytes.get_bytes(ea, size) or b"").hex(" "),
        }
        for ea, size in BYTE_RANGES
    ],
    "functions": [function_record(ea) for ea in FUNCTIONS],
}

os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
with open(OUT_PATH, "x", encoding="utf-8") as stream:
    json.dump(result, stream, ensure_ascii=False, indent=2)
print(OUT_PATH)

