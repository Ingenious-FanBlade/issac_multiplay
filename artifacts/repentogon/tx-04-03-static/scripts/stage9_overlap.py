exec(open(r"C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\match_lib.py").read())
t0 = time.time()
SIGS = json.load(open(SP + r"\repentogon_asm_sigs.json"))
def to_pat(sig):
    s = re.sub(r"[.!()]", "", sig)
    toks = [s[i:i + 2] for i in range(0, len(s) - 1, 2)]
    return " ".join("??" if "?" in t else t.upper() for t in toks), len(toks)
hits = []
for name, sig in SIGS:
    pat, n = to_pat(sig)
    if n < 4: continue
    for ea in search(pat, limit=4):
        hits.append((name, hx(rva(ea)), n))
    if time.time() - t0 > 85: break
# our byte-level patch points (v1.9.7.12 RVAs) and functions we entry-hook
SITES = {"mode1": 0x5915AE, "mode2": 0x59174C, "mode3": 0x591E67, "create": 0x591FE6, "deny_label": 0x5925E4}
ENTRY = {"Game::Update": 0x2F6D30, "Manager::Update": 0x52E3E0, "Manager::Render": 0x52EC40, "console_pred": 0x287020,
         "Room::Update": 0x3F7F60, "EntityList::Update": 0x13F00, "PostUpdateCaller": 0x446C90, "PostGameStart": 0x44B660,
         "NewRoomCaller": 0x44B930, "Game::Exit": 0x2F6270, "ExecuteStartGame": 0x532A20, "Game::RestoreState": 0x2F43A0,
         "RoomLifecycleFSM": 0x509100, "RoomTransition::Update": 0x426430, "InputManagerUpdate": 0x5EDCC0,
         "DeviceCommonUpdate": 0x5EC900, "OddHelper": 0x2F9340, "PlayerOriginalUpdate": 0x37BD40, "Move": 0x372540,
         "Bomb": 0x37A210, "ControlActiveItem": 0x389E50, "CardPill": 0x39BDD0, "Shoot": 0x373990, "Drop": 0x39C300,
         "JoinPass": 0x591360, "SpawnCoPlayer2": 0x590250, "CoopBabiesOnly": 0x5910A0, "Clock": 0x5F9690,
         "Console::RunCommand": 0x288100, "ExecuteCommandLua": 0x4567F0, "Game::ProcessInput": 0x2F66E0}
near = []
for name, h, n in hits:
    a = int(h, 16)
    for k, s in SITES.items():
        if a - 16 <= s <= a + n + 16: near.append(("SITE", k, hx(s), name, h, n))
    for k, f in ENTRY.items():
        fn = ida_funcs.get_func(va(f))
        if fn and fn.start_ea <= va(a) < fn.end_ea:
            near.append(("INFUNC", k, hx(f), name, h, n, "entry-overlap" if va(a) < va(f) + 16 else ""))
json.dump({"hits": hits, "near": near}, open(SP + r"\v12_overlap.json", "w"), indent=1)
print("sigs", len(SIGS), "hits", len(hits), "secs", round(time.time() - t0, 1))
for r in near: print(r)
