exec(open(r"C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\match_lib.py").read())
m = load_state()
def fname(ea):
    r = rva(func_start(ea)) if func_start(ea) else None
    return "%s%s" % (hx(r), ("(" + ANCH_BY_RVA[r][0][:40] + ")") if r in ANCH_BY_RVA else "")
out = {}
# 1. NewRoom callback (v17 0x465180 -> v12 0x44B930): callers
out["newroom_callers"] = [(fname(x.frm), hx(rva(x.frm))) for x in idautils.XrefsTo(va(0x44B930), 0) if is_call(x.frm)]
# v17 caller 0x33E0A0 fingerprint vs candidate callers
fp = V17["funcs"]["0x33E0A0"]
out["newroom_owner_sims"] = [(fname(x.frm), round(sim(fp["mnem"], func_start(x.frm)), 3)) for x in idautils.XrefsTo(va(0x44B930), 0) if is_call(x.frm)]
# 2. InputManager update candidate 0x5EDCC0 vs v17 0x61FC00
fp = V17["funcs"]["0x61FC00"]; out["im_update_sim"] = round(sim(fp["mnem"], va(0x5EDCC0)), 3)
out["im_update_callers_v12"] = [(fname(x.frm), hx(rva(x.frm))) for x in idautils.XrefsTo(va(0x5EDCC0), 0) if is_call(x.frm)]
m[0x61FC00] = {"v12": 0x5EDCC0, "how": "site:0x554F7D", "sim": out["im_update_sim"]}
# 3. Room::Update callers in v12 vs v17 0x2FE2F0
fp = V17["funcs"]["0x2FE2F0"]
out["roomupdate_callers_v12"] = sorted(set(fname(x.frm) for x in idautils.XrefsTo(va(0x3F7F60), 0) if is_call(x.frm)))
best = []
for x in idautils.XrefsTo(va(0x3F7F60), 0):
    if is_call(x.frm): best.append((round(sim(fp["mnem"], func_start(x.frm)), 3), fname(x.frm), hx(rva(x.frm))))
out["door_nested_owner_candidates"] = sorted(best, reverse=True)[:4]
# 4. Standard body anchor: v17 0x308630 called from Game::Update; list Game::Update callees v12 near the predicate call
fp = V17["funcs"]["0x308630"]
cands = []
for site, t in callees_ordered(va(0x2F6D30))[:40]:
    if ida_funcs.get_func(va(t)) and func_start(va(t)) == va(t):
        cands.append((round(sim(fp["mnem"], va(t)), 3), hx(site), hx(t), ANCH_BY_RVA.get(t, [""])[0][:40]))
out["standard_anchor_candidates"] = sorted(cands, reverse=True)[:5]
out["v17_gameupdate_first_calls"] = V17["funcs"]["0x2FADC0"]["callees"][:12]
out["v12_gameupdate_first_calls"] = [(hx(a), hx(b), ANCH_BY_RVA.get(b, [""])[0][:30]) for a, b in callees_ordered(va(0x2F6D30))[:12]]
save_state(m)
json.dump(out, open(SP + r"\v12_manual1.json", "w"), indent=1)
for k, v in out.items(): print(k, v)
