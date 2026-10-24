exec(open(r"C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\match_lib.py").read())
t0 = time.time()
m = load_state()
_am = {}
def amap(f17):
    if f17 not in _am: _am[f17] = align_map(f17, va(m[f17]["v12"]))
    return _am[f17]
def map_off(f17, off):
    if f17 not in m or hx(f17) not in V17["funcs"]: return None
    fp = V17["funcs"][hx(f17)]
    if off not in fp["offs"]: return None
    return amap(f17).get(fp["offs"].index(off))

sites = {}
for key, s in V17["sites"].items():
    o17 = int(s["func"], 16); site17 = int(s["site"], 16); t17 = int(s["target"], 16)
    r = {"v17_owner": s["func"], "v17_site": s["site"], "v17_target": s["target"], "v17_insn": s["insn"],
         "owner_v12": hx(m[o17]["v12"]) if o17 in m else None, "owner_how": m[o17]["how"] if o17 in m else None,
         "target_v12_expected": hx(m[t17]["v12"]) if t17 in m else None}
    ea12 = map_off(o17, site17 - o17)
    how = "aligned"
    if ea12 is None or call_target(ea12) is None:
        # fallback: same ordinal among calls to the matched target
        if o17 in m and t17 in m:
            calls = [a for a, b in callees_ordered(va(m[o17]["v12"])) if b == m[t17]["v12"]]
            if s["same_target_count"] == len(calls) and s["same_target_index"] is not None:
                ea12 = va(calls[s["same_target_index"]]); how = "ordinal"
            else:
                r["ordinal_candidates"] = [hx(a) for a in calls]; ea12 = None
    if ea12 is not None:
        t12 = call_target(ea12)
        r.update({"site_v12": hx(rva(ea12)), "map_how": how, "target_v12": hx(rva(t12)) if t12 else None,
                  "bytes_v12": (ida_bytes.get_bytes(ea12, 5) or b"").hex(" ").upper(),
                  "insn_v12": idc.generate_disasm_line(ea12, 0),
                  "target_agrees": (t17 in m and t12 is not None and rva(t12) == m[t17]["v12"]),
                  "target_anchor": ANCH_BY_RVA.get(rva(t12), []) if t12 else []})
    sites[key] = r

code = {}
for k, c in V17["code"].items():
    f17 = int(c["func"], 16); off = int(c["off"], 16)
    ea12 = map_off(f17, off)
    p12 = idc.prev_head(ea12) if ea12 else None
    code[k] = {"v17": c["rva"], "v17_prev": c["prev_insn"], "v12": hx(rva(ea12)) if ea12 else None,
               "insn_v12": idc.generate_disasm_line(ea12, 0) if ea12 else None,
               "prev_v12": idc.generate_disasm_line(p12, 0) if p12 else None,
               "prev_call_target_v12": hx(rva(call_target(p12))) if p12 and call_target(p12) else None}

def opaddrs(ea):
    insn = ida_ua.insn_t(); ida_ua.decode_insn(insn, ea); res = []
    for op in insn.ops:
        if op.type == ida_ua.o_void: break
        if op.type == ida_ua.o_mem: res.append(op.addr)
        elif op.type == ida_ua.o_imm and op.value >= BASE: res.append(op.value)
        elif op.type == ida_ua.o_displ and op.addr >= BASE: res.append(op.addr)
    return [hx(rva(a)) for a in res]

globs = {}
for k, g in V17["globals"].items():
    votes = {}
    for u in g["uses"]:
        if not u["func"]: continue
        ea12 = map_off(int(u["func"], 16), int(u["off"], 16))
        if ea12:
            for a in opaddrs(ea12): votes.setdefault(a, []).append(hx(rva(ea12)))
    globs[k] = {"v17": g["rva"], "votes": votes, "n_uses17": len(g["uses"])}

json.dump({"sites": sites, "code": code, "globals": globs}, open(SP + r"\v12_sites.json", "w"), indent=1)
print("sites mapped", sum(1 for r in sites.values() if r.get("site_v12")), "/", len(sites), "secs", round(time.time() - t0, 1))
