exec(open(r"C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\match_lib.py").read())
res = {}
for k, v in V17["vtables"].items():
    name = v["rtti"]["name"]
    found = []
    nb = name.encode("latin1") + b"\x00"
    for segname in (".data", ".rdata"):
        sg = ida_segment.get_segm_by_name(segname)
        data = ida_bytes.get_bytes(sg.start_ea, sg.end_ea - sg.start_ea)
        for mm in re.finditer(re.escape(nb), data):
            td = sg.start_ea + mm.start() - 8
            for x in idautils.XrefsTo(td, 0):
                col = x.frm - 12
                if ida_bytes.get_dword(col) != 0: continue
                for y in idautils.XrefsTo(col, 0):
                    vt = y.frm + 4
                    found.append({"vtable": hx(rva(vt)), "col": hx(rva(col)), "td": hx(rva(td)), "col_offset": ida_bytes.get_dword(col + 4),
                                  "slots": [hx(rva(ida_bytes.get_dword(vt + 4 * i))) for i in range(40)]})
    res[k] = {"name": name, "v17_vtable": v["rva"], "v17_col": v["rtti"]["col"], "v12": found}
json.dump(res, open(SP + r"\v12_vtables.json", "w"), indent=1)
for k, r in res.items():
    print(k, r["name"], [(f["vtable"], f["col"], f["col_offset"]) for f in r["v12"]])
