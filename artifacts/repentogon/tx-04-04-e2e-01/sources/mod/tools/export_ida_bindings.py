"""Run in the v1.9.7.12 IDB to export static instruction patterns, never execute.

Use ida-mcp py_exec_file. Pass a repository path via ISAAC_CS_REPOSITORY when
importing this script in a different checkout; no hash or runtime probe is used.
"""
import json
import os
from pathlib import Path
import ida_bytes
import ida_fixup
import ida_nalt
import idautils
import idc

def export():
    root = Path(os.environ.get('ISAAC_CS_REPOSITORY', 'F:/projects_isaac/issac_multiplay'))
    base = ida_nalt.get_imagebase()
    rows = json.loads((root/'mod/binding/hooks.json').read_text())['hooks']
    rvas = [int(row[7],16) for row in rows if row[11]] + [0x5EC570,0x5EC6A0,0x5EEB40,0x5EEBB0]
    functions = []
    for rva in rvas:
        ea = base + rva
        data = ida_bytes.get_bytes(ea,32)
        mask = [True]*32
        for offset in range(32):
            fixup = ida_fixup.fixup_data_t()
            if ida_fixup.get_fixup(fixup, ea+offset):
                for i in range(offset,min(offset+4,32)): mask[i] = False
        for head in idautils.Heads(ea,ea+32):
            if ida_bytes.get_byte(head) in (0xE8,0xE9):
                for i in range(head-ea+1,min(head-ea+5,32)): mask[i] = False
        functions.append(dict(rva=hex(rva),pattern=' '.join(f'{b:02X}' if mask[i] else '??' for i,b in enumerate(data)),
                              type=idc.get_type(ea),entry=[idc.generate_disasm_line(h,0) for h in idautils.Heads(ea,ea+32)]))
    (root/'mod/binding/v1_9_7_12-static.json').write_text(json.dumps(dict(evidence='STATIC_CONFIRMED',base=hex(base),functions=functions),indent=2),encoding='utf-8')

export()
