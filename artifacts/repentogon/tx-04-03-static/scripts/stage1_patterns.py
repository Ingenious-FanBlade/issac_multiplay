exec(open(r"C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\match_lib.py").read())
t0 = time.time()
m = load_state()
for key, fp in V17["funcs"].items():
    if not key.startswith("0x") or "pattern" not in fp: continue
    f17 = int(key, 16)
    if f17 in m: continue
    pat = fp["pattern"].split()
    for n in (64, 40, 24, 16):
        if len(pat) < n and n != 16: continue
        hits = [h for h in search(" ".join(pat[:n])) if func_start(h) == h]
        if len(hits) == 1:
            setm(m, f17, hits[0], "pattern%d" % n, sim(fp["mnem"], hits[0])); break
        if len(hits) > 1:
            best = max(hits, key=lambda h: sim(fp["mnem"], h))
            setm(m, f17, best, "pattern%d-multi%d" % (n, len(hits)), sim(fp["mnem"], best)); break
    if time.time() - t0 > 80: break
save_state(m)
tot = sum(1 for k in V17["funcs"] if k.startswith("0x"))
print("matched", len(m), "of", tot, "secs", round(time.time() - t0, 1))
