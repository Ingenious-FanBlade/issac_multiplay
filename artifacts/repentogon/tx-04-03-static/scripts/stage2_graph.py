exec(open(r"C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\match_lib.py").read())
t0 = time.time()
old = load_state()
# Keep only trustworthy pattern matches; everything else is re-derived through the call graph.
m = {k: v for k, v in old.items() if v["how"].startswith("graph") or v["how"].startswith("seed") or v["sim"] >= 0.85}
FIXED = {  # v17 -> v12 seeds from REPENTOGON anchors / RTTI vtable slots (verified by similarity below)
    0x531050: 0x50AC60, 0x382AF0: 0x37BD40, 0x61E8D0: 0x5ECF50, 0x61E970: 0x5ECF90, 0x61EA20: 0x5ED070,
    0x61E210: 0x5EC6A0, 0x66C650: 0x633DD0, 0x622940: 0x6331B0,
}
HYP = {0x2FADC0: 0x2F6D30, 0x554CD0: 0x52E3E0, 0x5AB6D0: 0x52E3E0, 0x5555C0: 0x52EC40, 0x28BBA0: 0x2F92A0,
       0x4607A0: 0x446C90, 0x464EE0: 0x44B660, 0x465180: 0x44B930, 0x5AB700: 0x582C90, 0xCC10: 0x2670}
seedlog = {}
for f17, r12 in list(FIXED.items()) + list(HYP.items()):
    fp = V17["funcs"].get(hx(f17))
    if not fp or not ida_funcs.get_func(va(r12)): seedlog[hx(f17)] = "nofp/nofunc"; continue
    s = sim(fp["mnem"], va(r12))
    seedlog[hx(f17)] = (hx(r12), round(s, 3), len(fp["mnem"]), len(insns(va(r12))))
    if f17 in FIXED or s >= 0.6:
        m[f17] = {"v12": r12, "how": "seed", "sim": round(s, 4)}
print("seeds", seedlog)

def calls17(f17):
    fp = V17["funcs"][hx(f17)]
    return [(fp["offs"].index(int(a, 16) - f17), int(a, 16), int(b, 16)) for a, b in fp["callees"] if (int(a, 16) - f17) in fp["offs"]]

done = set()
changed = True
while changed and time.time() - t0 < 85:
    changed = False
    for f17 in list(m.keys()):
        if f17 in done or hx(f17) not in V17["funcs"]: continue
        if time.time() - t0 > 85: break
        done.add(f17)
        fs12 = va(m[f17]["v12"])
        mp = align_map(f17, fs12)
        for idx, site17, t17 in calls17(f17):
            ea12 = mp.get(idx)
            if ea12 is None: continue
            t12 = call_target(ea12)
            if t12 is None or func_start(t12) != t12: continue
            if hx(t17) in V17["funcs"] and t17 not in m:
                s = sim(V17["funcs"][hx(t17)]["mnem"], t12)
                if s >= 0.5:
                    m[t17] = {"v12": rva(t12), "how": "graph:" + hx(f17), "sim": round(s, 4)}; changed = True
save_state(m)
tot = [k for k in V17["funcs"] if k.startswith("0x")]
print("matched", len(m), "of", len(tot), "secs", round(time.time() - t0, 1))
print("unmatched", [k for k in tot if int(k, 16) not in m])
