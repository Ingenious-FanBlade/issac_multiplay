exec(open(r"C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\match_lib.py").read())
t0 = time.time()
m = load_state()
OFFS = {"Game": [0x2658A, 0x1BAA8, 0x1BAAC, 0x26548], "Manager": [0x4B3D8, 0x4B3DC, 0x4B3E4, 0x4B130, 0x2A3C0],
        "Player": [0x161C, 0x1618, 0x20A9, 0x1340, 0x1344, 0x134C, 0x1D88, 0x360, 0x1624, 0x162C, 0x1630, 0x1634, 0x1638,
                   0x1378, 0x1580, 0x1588, 0x17A0, 0x17C0, 0x13BC]}
_am = {}
def amap(f17):
    if f17 not in _am: _am[f17] = align_map(f17, va(m[f17]["v12"]))
    return _am[f17]
def displs(ea):
    insn = ida_ua.insn_t(); ida_ua.decode_insn(insn, ea); r = []
    for op in insn.ops:
        if op.type == ida_ua.o_void: break
        if op.type == ida_ua.o_displ: r.append(op.addr)
    return r
res = {}
for grp, offs in OFFS.items():
    for off in offs:
        votes = {}; samples = []
        for key, fp in V17["funcs"].items():
            if not key.startswith("0x"): continue
            f17 = int(key, 16)
            if f17 not in m or hx(off) not in fp["disp"]: continue
            if time.time() - t0 > 80: break
            for insn17 in fp["disp"][hx(off)]:
                o = int(insn17, 16) - f17
                if o not in fp["offs"]: continue
                ea12 = amap(f17).get(fp["offs"].index(o))
                if ea12 is None: continue
                for d in displs(ea12):
                    if d >= 0x10:
                        votes[hx(d)] = votes.get(hx(d), 0) + 1
                        if len(samples) < 3: samples.append((insn17, hx(rva(ea12)), idc.generate_disasm_line(ea12, 0)))
        res[grp + ":" + hx(off)] = {"votes": votes, "samples": samples}
json.dump(res, open(SP + r"\v12_fields.json", "w"), indent=1)
for k, v in res.items(): print(k, v["votes"], v["samples"][:1])
print("secs", round(time.time() - t0, 1))
