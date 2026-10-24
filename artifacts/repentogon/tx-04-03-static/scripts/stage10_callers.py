exec(open(r"C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\match_lib.py").read())
F = {"PostUpdateCaller": 0x446C90, "PostGameStart": 0x44B660, "NewRoomCaller": 0x44B930, "Game::Exit": 0x2F6270,
     "Game::Update": 0x2F6D30, "Manager::Update": 0x52E3E0, "Manager::Render": 0x52EC40, "ExecuteStartGame": 0x532A20,
     "Room::Update": 0x3F7F60, "EntityList::Update": 0x13F00, "console_pred": 0x287020, "RoomLifecycleFSM": 0x509100,
     "RoomTransition::Update": 0x426430, "InputManagerUpdate": 0x5EDCC0, "DeviceCommonUpdate": 0x5EC900,
     "OddHelper": 0x2F9340, "Game::RestoreState": 0x2F43A0, "JoinPass": 0x591360, "SpawnCoPlayer2": 0x590250,
     "CoopBabiesOnly": 0x5910A0, "Move": 0x372540, "Bomb": 0x37A210, "ControlActiveItem": 0x389E50, "CardPill": 0x39BDD0,
     "Shoot": 0x373990, "Drop": 0x39C300, "Clock": 0x5F9690, "Game::ProcessInput": 0x2F66E0}
res = {}
for k, r in F.items():
    cs = [(hx(rva(func_start(x.frm))) if func_start(x.frm) else None, hx(rva(x.frm))) for x in idautils.XrefsTo(va(r), 0) if is_call(x.frm)]
    res[k] = {"rva": hx(r), "n_callers": len(cs), "callers": [(c[0], c[1], ANCH_BY_RVA.get(int(c[0], 16), [""])[0][:40] if c[0] else "") for c in cs][:12]}
json.dump(res, open(SP + r"\v12_callers.json", "w"), indent=1)
for k, v in res.items(): print(k, v["rva"], v["n_callers"], [(c[0], c[2][:28]) for c in v["callers"]][:8])
