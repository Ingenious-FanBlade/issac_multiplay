exec(open(r"C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\fp_common.py").read())
import re, struct
rd = ida_segment.get_segm_by_name(".rdata"); dt = ida_segment.get_segm_by_name(".data")
RD = ida_bytes.get_bytes(rd.start_ea, rd.end_ea - rd.start_ea)
DT = ida_bytes.get_bytes(dt.start_ea, dt.end_ea - dt.start_ea)
def find_all(buf, needle):
    return [m.start() for m in re.finditer(re.escape(needle), buf)]
out = {}
for nm in [".?AVDeviceKeyboard@Input@KAGE@@", ".?AVDeviceGamepad@Input@KAGE@@", ".?AVNetInputDevice@IsaacRepentancePlus@@", ".?AVInputDeviceBase@Input@KAGE@@",
           ".?AVEntity_Player@IsaacRepentancePlus@@", ".?AVManager@Input@KAGE@@"]:
    hits = []
    for seg, buf in ((dt, DT), (rd, RD)):
        for off in find_all(buf, nm.encode() + b"\x00"):
            td = seg.start_ea + off - 8
            for coff in find_all(RD, struct.pack("<I", td)):
                col = rd.start_ea + coff - 12
                if ida_bytes.get_dword(col) != 0: continue
                for voff in find_all(RD, struct.pack("<I", col)):
                    vt = rd.start_ea + voff + 4
                    hits.append(dict(td=hx(rva(td)), col=hx(rva(col)), col_offset=ida_bytes.get_dword(col + 4), vtable=hx(rva(vt)),
                                     slots=[hx(rva(ida_bytes.get_dword(vt + 4 * i))) for i in range(40)]))
    out[nm] = hits
json.dump(out, open(SP + r"\v12_vtables_all.json", "w"), indent=1)
for k, v in out.items():
    print(k, [(h["vtable"], h["col"], h["col_offset"], h["slots"][3], h["slots"][12:15], h["slots"][30]) for h in v])
