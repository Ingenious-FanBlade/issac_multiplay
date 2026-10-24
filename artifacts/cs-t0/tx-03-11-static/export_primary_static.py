"""Read-only IDA evidence export. Run via IDA MCP py_exec_file.

To select another output/target set, use runpy.run_path with init_globals:
TX0311_OUTPUT, TX0311_FUNCTIONS, TX0311_RANGES, TX0311_TABLES. Output is create-new.
No target execution, database edits, file identity comparison, or hashing.
"""
import json
from pathlib import Path
import idaapi
import ida_nalt
import ida_funcs
import ida_hexrays
import ida_bytes
import ida_lines
import idautils
import idc

base = idaapi.get_imagebase()
target_functions = globals().get("TX0311_FUNCTIONS", [
    0x408E00, 0x408C90, 0x408B90, 0x408C10,
    0x40A5D0, 0x408830, 0x408970, 0x40A1B0,
])
target_ranges = globals().get("TX0311_RANGES", [])
target_tables = globals().get("TX0311_TABLES", [])
output = Path(globals().get("TX0311_OUTPUT", str(Path(__file__).with_name("primary-animation.json"))))

def instruction(ea):
    size = idc.get_item_size(ea)
    raw = ida_bytes.get_bytes(ea, size)
    return {"va": hex(ea), "rva": hex(ea - base),
            "bytes": raw.hex(" ") if raw else None,
            "asm": ida_lines.tag_remove(idc.generate_disasm_line(ea, 0) or ""),
            "code_refs": [hex(x) for x in idautils.CodeRefsFrom(ea, False)]}

records = []
for address in target_functions:
    fn = ida_funcs.get_func(address)
    row = {"requested_va": hex(address), "name": ida_funcs.get_func_name(address)}
    if fn:
        row.update({"va": hex(fn.start_ea), "rva": hex(fn.start_ea-base),
                    "end_va": hex(fn.end_ea), "type": idc.get_type(fn.start_ea),
                    "chunks": [[hex(a), hex(b)] for a, b in idautils.Chunks(fn.start_ea)],
                    "instructions": [instruction(ea) for ea in idautils.FuncItems(fn.start_ea)
                                     if ida_bytes.is_code(ida_bytes.get_flags(ea))],
                    "xrefs_to": [{"va": hex(x.frm), "type": x.type,
                                   "iscode": bool(x.iscode),
                                   "function": ida_funcs.get_func_name(x.frm)}
                                  for x in idautils.XrefsTo(fn.start_ea)]})
        try:
            row["pseudocode"] = str(ida_hexrays.decompile(fn.start_ea))
        except Exception as exc:
            row["decompile_error"] = str(exc)
    else:
        row["error"] = "No IDA function at requested address"
    records.append(row)

ranges = [{"start_va": hex(start), "end_va": hex(end),
           "instructions": [instruction(ea) for ea in idautils.Heads(start, end)
                            if ida_bytes.is_code(ida_bytes.get_flags(ea))]}
          for start, end in target_ranges]
tables = [{"va": hex(address), "rva": hex(address-base),
           "entries": [{"offset": hex(i*4),
                        "bytes": ida_bytes.get_bytes(address+i*4, 4).hex(" "),
                        "target_va": hex(ida_bytes.get_dword(address+i*4)),
                        "target_rva": hex(ida_bytes.get_dword(address+i*4)-base),
                        "target_name": idc.get_name(ida_bytes.get_dword(address+i*4))}
                       for i in range(count)]} for address, count in target_tables]
payload = {"tx": "tx-03-11", "evidence": "STATIC_IDB_READ_ONLY",
           "idb_path": idc.get_idb_path(), "input_path": ida_nalt.get_input_file_path(),
           "imagebase": hex(base), "functions": records, "ranges": ranges, "tables": tables,
           "limitations": ["No runtime observations", "No IDB-to-disk identity assertion",
                           "Decompiler types are not a complete ABI contract",
                           "Xrefs cover current IDA analysis, not all dynamic callers"]}
with output.open("x", encoding="utf-8") as stream:
    json.dump(payload, stream, ensure_ascii=False, indent=2)
print(json.dumps({"output": str(output), "functions": len(records), "ranges": len(ranges)}))
