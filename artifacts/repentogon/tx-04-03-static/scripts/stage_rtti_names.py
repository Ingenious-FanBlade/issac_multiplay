exec(open(r"C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\fp_common.py").read())
import re
res = set()
for segname in (".data", ".rdata"):
    sg = ida_segment.get_segm_by_name(segname)
    buf = ida_bytes.get_bytes(sg.start_ea, sg.end_ea - sg.start_ea)
    for m in re.finditer(rb"\.\?AV[^\x00]{0,80}(Input|Keyboard|Gamepad|Device|Controller)[^\x00]{0,60}\x00", buf):
        res.add((segname, hx(rva(sg.start_ea + m.start())), m.group(0)[:-1].decode("latin1")))
for r in sorted(res, key=lambda x: x[2]): print(r)
