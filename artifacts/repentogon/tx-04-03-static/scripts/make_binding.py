# Builds docs/hooks/repentogon-binding.json from the curated tx-04-03 results (static analysis only).
import json
OUT = r"F:\projects_isaac\issac_multiplay\docs\hooks\repentogon-binding.json"

SC, RO, SRC, SI, UNK = "STATIC_CONFIRMED", "RUNTIME_OBSERVED_REPENTOGON_LOG", "SOURCE_REPENTOGON_ZHL", "STATIC_INFERENCE", "UNKNOWN"

def site(id_, fam, idx, s17, t17, s12, owner12, owner_name, t12, t12_name, mech, ctx, ev, notes="", rg=None):
    return {"id": id_, "group": id_.split(".")[0],
            "prototype": {"family": fam, "index": idx, "kind": "REL32_CALL", "site_v17": s17, "target_v17": t17},
            "v12": {"site": s12, "owner": owner12, "owner_name": owner_name, "target": t12, "target_name": t12_name},
            "mechanism": mech, "context": ctx, "repentogon": rg or {}, "evidence": ev, "notes": notes}

items = []
L = "loop_runtime.cpp"; C = "coop_active_runtime.cpp / active_targets.cpp"
items += [
 site("loop.00.clock", L, 0, "0x531231", "0x6266A0", "0x50AE44", "0x50AC60", "IsaacMain", "0x5F9690", None, "DROP",
      "-", [SC + ": aligned call in IsaacMain (owner sim 0.938, target sim 0.774)"],
      "Returns double in XMM0; ZHL flags (THISCALL/CLEANUP/VOID/LONGLONG) cannot express it. 7 callers in v1.9.7.12. Not hooked in the first round; tick comes from inner execution."),
 site("loop.01.top_update", L, 1, "0x5312EC", "0x5AB6D0", "0x50AF08", "0x50AC60", "IsaacMain", "0x52E3E0", "Manager::Update", "ZHL_REPENTOGON",
      "outer frame: top-level update (priority below -9999)", [SC + ": v1.9.7.12 IsaacMain calls Manager::Update directly; v1.9.7.17 0x5AB6D0 was a 40-byte wrapper around 0x554CD0 (=Manager::Update, sim 0.950)", RO + ": Manager::Update anchor"],
      "2 callers: IsaacMain, Game::NextVictoryLap.", {"definition": "Manager::Update (static, __stdcall, bool)", "hooks": 3, "priorities": [0, 0, -9999]}),
 site("loop.02.game_update", L, 2, "0x555231", "0x2FADC0", "0x52E911", "0x52E3E0", "Manager::Update", "0x2F6D30", "Game::Update", "ZHL_REPENTOGON",
      "outer frame: normal gameplay", [SC + ": aligned site; v1.9.7.17 0x2FADC0 vs Game::Update sim 0.911", RO + ": Game::Update anchor"],
      "1 caller. REPENTOGON ASM patches inside: Game_Update_MainUpdate (0x2F77A4, MC_PRE_UPDATE skip) and Game_Update_Return (0x2F7A39).",
      {"definition": "Game::Update", "hooks": 2, "asm_inside": ["Game_Update_MainUpdate@0x2F77A4", "Game_Update_Return@0x2F7A39"]}),
 site("loop.03.post_update", L, 3, "0x2FBBA5", "0x4607A0", "0x2F7A34", "0x2F6D30", "Game::Update", "0x446C90", "MC_POST_UPDATE caller (callback id 1)", "ZHL_OWN",
      "inner: Game::Update tail", [SC + ": aligned site; target reads g_LuaEngine mask bit 2 and pushes id 1 (decompiled)"],
      "1 caller in both versions, so an entry hook equals the old call-site hook."),
 site("loop.04.teardown_1", L, 4, "0x559696", "0x2FA0C0", "0x532996", "0x532970", "Manager::cleanup_current_state", "0x2F6270", "Game::Exit", "ZHL_REPENTOGON",
      "teardown", [SC + ": v1.9.7.17 0x2FA0C0 vs Game::Exit sim 0.992", RO + ": Game::Exit anchor"], "Game::Exit has exactly the 2 prototype callers.",
      {"definition": "Game::Exit (__stdcall, bool)", "hooks": 3}),
 site("loop.05.teardown_2", L, 5, "0x5AB84A", "0x2FA0C0", "0x582DD8", "0x582C90", "Isaac::Shutdown", "0x2F6270", "Game::Exit", "ZHL_REPENTOGON",
      "teardown", [SC + ": aligned site in Isaac::Shutdown (owner sim 0.766)"], "Same Game::Exit hook as loop.04."),
 site("loop.06.new_room", L, 6, "0x33F5B9", "0x465180", "0x339719", "0x3385F0", None, "0x44B930", "MC_POST_NEW_ROOM caller (callback id 19)", "ZHL_OWN",
      "inner: new room", [SC + ": only caller of 0x44B930 is 0x3385F0 (owner sim 0.82 to v1.9.7.17 0x33E0A0); id 19 / mask 0x80000 decompiled"], "1 caller."),
 site("loop.07.render", L, 7, "0x5312F1", "0x5555C0", "0x50AF0D", "0x50AC60", "IsaacMain", "0x52EC40", "Manager::Render", "ZHL_REPENTOGON",
      "render frame (never a tick)", [SC + ": aligned call position in IsaacMain", RO + ": Manager::Render anchor", "function similarity only 0.519 (render code changed)"],
      "2 callers: IsaacMain and ModManager::UpdateWorkshopMods (loading path). REPENTOGON ASM patches inside (cursor render, ImGui).",
      {"definition": "Manager::Render", "hooks": 2, "asm_inside": ["Manager_Render_PreCursorRender@0x52EFAC", "Manager_Render_OnCursorRender@0x52F03C", "ImGui inline@0x52F5CF"]}),
 site("loop.08.input_update", L, 8, "0x554F7D", "0x61FC00", "0x52E68B", "0x52E3E0", "Manager::Update", "0x5EDCC0", "InputManager full update", "ZHL_OWN",
      "inner: Manager::Update frame", [SC + ": aligned site; decompiled loop over InputManager devices (+4/+8)", "function similarity 0.392 (input subsystem changed)"],
      "4 callers in v1.9.7.12 (Manager::Update, 0x5346B0 x2, 0x62AC40); act only inside the Manager::Update frame."),
 site("loop.09.device_update", L, 9, "0x61FC97", "0x61F280", "0x5EDD22", "0x5EDCC0", "InputManager full update", "0x5EC900", "device common update", "ZHL_OWN",
      "inner: input update", [SC + ": only call site of 0x5EC900; it calls Poll via vtable +0x8C and checks the bool"], "1 caller."),
 site("loop.10.alt_owner", L, 10, "0x554D5E", "0x559720", "0x52E46E", "0x52E3E0", "Manager::Update", "0x532A20", "Manager::ExecuteStartGame", "ZHL_REPENTOGON",
      "outer frame: alternate/loading", [SC + ": aligned site (owner sim 0.825)", RO + ": anchor"], "1 caller; REPENTOGON does not hook it.",
      {"definition": "Manager::ExecuteStartGame", "hooks": 0}),
 site("loop.11.alt_room_update", L, 11, "0x559B4D", "0x402980", "0x532EAA", "0x532A20", "Manager::ExecuteStartGame", "0x3F7F60", "Room::Update", "ZHL_OWN",
      "inner: Room::Update under ExecuteStartGame", [SC + ": v1.9.7.17 0x402980 vs 0x3F7F60 sim 0.944"], "Room::Update has 10 callers; context decides."),
 site("loop.12.alt_tail", L, 12, "0x559B82", "0x464EE0", "0x532EDF", "0x532A20", "Manager::ExecuteStartGame", "0x44B660", "LuaEngine::PostGameStart", "ZHL_REPENTOGON",
      "inner: alternate tail/completion", [SC + ": aligned; callback id 15", RO + ": LuaEngine::PostGameStart anchor"],
      "The 6 callers are exactly loop.12 and loop.18-22.", {"definition": "LuaEngine::PostGameStart (__stdcall, unsigned)", "hooks": 1}),
 site("loop.13.console_pred", L, 13, "0x2FADFA", "0x28BBA0", "0x2F6D69", "0x2F6D30", "Game::Update", "0x287020", "console method on Game::_console (+0x67FD4)", "ZHL_OWN",
      "inner: Game::Update gate", [SC + ": aligned; `lea ecx,[edi+67FD4h]; call; test al,al; jnz <skip body>`", SRC + ": Game::_console @0x67FD4"],
      "NOT Game::IsPaused (sim 0.20). 3 callers: Game::Update, odd helper, Manager::Update. Game::IsPaused is not called by Game::Update in v1.9.7.12."),
 site("loop.14.standard_anchor", L, 14, "0x2FAE09", "0x308630", None, "0x2F6D30", "Game::Update", None, None, "NONE_IN_V12",
      "replace with Game::Update frame + console predicate result", [SC + ": call sequences compared; v1.9.7.12 goes predicate -> HUD::Update directly; the 0x308630 call exists only in v1.9.7.17"], ""),
 site("loop.15.std_room_update", L, 15, "0x2FBA4D", "0x402980", "0x2F78DF", "0x2F6D30", "Game::Update", "0x3F7F60", "Room::Update", "ZHL_OWN",
      "inner: Room::Update under Game::Update", [SC + ": aligned"], "Shares the Room::Update hook."),
 site("loop.16.entitylist_update", L, 16, "0x403E70", "0x186C0", "0x3F93A7", "0x3F7F60", "Room::Update", "0x13F00", "EntityList::Update", "ZHL_OWN",
      "inner: under Room::Update", [SC + ": v1.9.7.17 0x186C0 vs 0x13F00 sim 0.901"], "9 callers; REPENTOGON ASM patches inside (EntityList_Update_*).",
      {"asm_inside": ["EntityList_Update_PreCommitRemovedEntities@0x14DBE", "EntityList_Update_RemoveEntityMainEL@0x14F11", "EntityList_Update_RemoveEntityPersistentEL@0x14F93"]}),
 site("loop.17.alt_state_apply", L, 17, "0x559B21", "0x2F8140", "0x532E7E", "0x532A20", "Manager::ExecuteStartGame", "0x2F43A0", "Game::RestoreState", "ZHL_REPENTOGON",
      "inner: alternate", [SC + ": sim 0.886", RO + ": anchor"], "3 callers.", {"definition": "Game::RestoreState", "hooks": 4, "priorities": [-100, 0, 0, 0]}),
]
for i, (s17, owner17, s12, o12, oname) in enumerate([
        ("0x2F580B", "0x2F5320", "0x2F1BAE", "0x2F16D0", "Game::Start"),
        ("0x2F61FF", "0x2F5850", "0x2F26A0", "0x2F1BF0", "Game::NetStart"),
        ("0x2F72C9", "0x2F6DD0", "0x2F362C", "0x2F3140", "Game::StartDailyChallenge"),
        ("0x2F7721", "0x2F72F0", "0x2F3A70", "0x2F3650", "Game::StartFromRerunState"),
        ("0x2F7A10", "0x2F7750", "0x2F3D63", "0x2F3AA0", "Game::StartDebug")]):
    items.append(site("loop.%02d.alt_completion_%d" % (18 + i, i), L, 18 + i, s17, "0x464EE0", s12, o12, oname, "0x44B660", "LuaEngine::PostGameStart",
                      "ZHL_REPENTOGON", "inner: alternate completion", [SC + ": aligned; owner is REPENTOGON-named"], "Same PostGameStart hook as loop.12."))
items += [
 site("loop.23.room_fsm", L, 23, "0x2FB4F0", "0x52F1C0", "0x2F73F4", "0x2F6D30", "Game::Update", "0x509100", "room lifecycle FSM", "ZHL_OWN",
      "outer frame: room lifecycle FSM", [SC + ": sim 0.928"], "1 caller."),
 site("loop.24.room_fsm_room_update", L, 24, "0x52FC77", "0x402980", "0x509B26", "0x509100", "room lifecycle FSM", "0x3F7F60", "Room::Update", "ZHL_OWN",
      "inner: Room::Update under FSM", [SC + ": aligned"], ""),
 site("loop.25.door_fsm", L, 25, "0x2FB51C", "0x4318A0", "0x2F7420", "0x2F6D30", "Game::Update", "0x426430", "RoomTransition::Update", "ZHL_REPENTOGON",
      "outer frame: door room change", [SC + ": sim 0.879", RO + ": anchor"], "5 callers.", {"definition": "RoomTransition::Update", "hooks": 1}),
 site("loop.26.door_room_update", L, 26, "0x2FE37C", "0x402980", "0x2FA17C", "0x2FA0F0", None, "0x3F7F60", "Room::Update", "ZHL_OWN",
      "inner: Room::Update under 0x2FA0F0", [SC + ": v1.9.7.17 owner 0x2FE2F0 vs 0x2FA0F0 sim 0.971"], "The context owner 0x2FA0F0 also needs an own-definition frame hook; its callers are to be listed during implementation."),
]
# pointer patches
items += [
 {"id": "loop.poll", "group": "loop", "prototype": {"kind": "VTABLE_SLOT", "keyboard": "0x7A16F0 (slot 30, +0x78) -> 0x66C650", "gamepad": "0x782CF8 (slot 30) -> 0x622940"},
  "v12": {"keyboard_vtable": "0x73B490", "gamepad_vtable": "0x73B3F0", "slot": 35, "slot_offset": "0x8C", "keyboard_target": "0x633DD0", "gamepad_target": "0x6331B0",
          "return_after_poll_call": "0x5EC98C"},
  "mechanism": "POINTER", "context": "inner: device common update", "repentogon": {"hooks": 0},
  "evidence": [SC + ": common update calls [vtable+0x8C] and tests AL; base slot 35 is _purecall; keyboard slot 35 copies the 0x15C-byte state blocks"],
  "notes": "Slot moved from 30 to 35. Return address check: v1.9.7.17 0x61F30C -> v1.9.7.12 0x5EC98C."},
 {"id": "loop.iat", "group": "loop", "prototype": {"kind": "IAT_SLOT", "slots": ["PeekMessage", "TranslateMessage", "DispatchMessage"]},
  "v12": {}, "mechanism": "DROP", "context": "-", "repentogon": {}, "evidence": ["design tx-04-01 6.1"], "notes": "Message-pump diagnostics only."},
 {"id": "coop.queries", "group": "coop", "prototype": {"kind": "VTABLE_SLOT", "slots": "12/13/14 (+0x30/+0x34/+0x38) of Keyboard 0x7A1678 and Gamepad 0x782C80 -> 0x61E8D0/0x61E970/0x61EA20"},
  "v12": {"keyboard_vtable": "0x73B490", "gamepad_vtable": "0x73B3F0", "slots": [8, 9, 10], "slot_offsets": ["0x20", "0x24", "0x28"],
          "targets": {"IsActionPressed": "0x5ECD80", "IsActionTriggered": "0x5ECDE0", "GetActionValue": "0x5ECE50"}, "abi": "thiscall(action), retn 4; slot 10 returns float in x87 ST0 (fldz path)"},
  "mechanism": "POINTER", "context": "query resolution for A/B", "repentogon": {"hooks_above": "InputManager::IsActionPressed/IsActionTriggered (menu/ImGui blocking)"},
  "evidence": [SC + ": InputManager::IsActionPressed (0x5EE140) calls device +0x20, IsActionTriggered (0x5EE200) calls +0x24; slot bodies decompiled", RO + ": InputManager anchors"],
  "notes": "v1.9.7.12 slots 12-15 are setters (SetButton/SetState/SetValue). Keyboard and Gamepad share the base implementations."},
 {"id": "coop.player_update", "group": "coop", "prototype": {"kind": "VTABLE_SLOT", "site_v17": "0x76BDDC (+0x0C)", "target_v17": "0x382AF0"},
  "v12": {"player_vtable": "0x71BDB4", "slot": 3, "target": "0x37BD40", "target_name": "Entity_Player::Update / Original_Update"},
  "mechanism": "ZHL_REPENTOGON", "context": "outer frame: Player update",
  "repentogon": {"definition": "Entity_Player::Update (__vtable override)", "hooks": 1, "hook_source": "CustomCallbacks.cpp HOOK_PRE_UPDATE_CALLBACK(Entity_Player, ..., 1160)",
                 "asm_inside": ["Player_DeathSoundOverride@0x381C76"]},
  "evidence": [SC + ": RTTI .?AVEntity_Player@IsaacRepentancePlus@@ vtable slot 3; sim 0.63 (large function)", RO + ": anchor"],
  "notes": "MC_PRE_PLAYER_UPDATE returning true skips the subclass update (consumers do not run)."},
]
for i, (s17, t17, s12, t12, name, mech, extra) in enumerate([
        ("0x385BC2", "0x379B40", "0x37DDFF", "0x372540", "movement", "ZHL_OWN", ["ASMPlayer inline@0x37389B"]),
        ("0x385BC9", "0x380F40", "0x37DE06", "0x37A210", "bomb", "ZHL_OWN", ["MaxBombs_ControlBombs@0x37AC0A", "ASMCallbacks inline@0x37A52E", "@0x37ADA8"]),
        ("0x385BD2", "0x391A40", "0x37DE0F", "0x389E50", "active item (Entity_Player::ControlActiveItem)", "ZHL_REPENTOGON", []),
        ("0x385BD9", "0x3A3D30", "0x37DE16", "0x39BDD0", "card/pill", "ZHL_OWN", []),
        ("0x385BE0", "0x37B020", "0x37DE1D", "0x373990", "shooting", "ZHL_OWN", []),
        ("0x385BE7", "0x3A42B0", "0x37DE24", "0x39C300", "drop", "ZHL_OWN", ["ASMCallbacks inline@0x39C381"])]):
    items.append(site("coop.consumer_%d" % i, C, i, s17, t17, s12, "0x37BD40", "Entity_Player::Update", t12, name, mech, "inner: Player::Update frame",
                      [SC + ": aligned site in Player update; target agrees with function match"],
                      "ControlActiveItem has 3 callers (Player::Update + card/pill consumer x2); the others have 1." if i == 2 else "",
                      {"asm_inside": extra, "hooks": 1 if i == 2 else 0}))
items += [
 site("coop.join_pass", C, 6, "0x2FADA7", "0x5BAC40", "0x2F6D21", "0x2F66E0", "Game::ProcessInput", "0x591360", "Join pass (PlayerManager method; ECX = Game+0x1BAA8)", "ZHL_OWN",
      "outer frame: Join", [SC + ": aligned; caller does lea ecx,[edi+1BAA8h]"], "1 caller. Returns early when NetplayManager (g_Manager+0x4AFB8/+0x4AFBC) is non-empty."),
 site("coop.create", C, 7, "0x5BB415", "0x5B9CD0", "0x591FE6", "0x591360", "Join pass", "0x590250", "PlayerManager::SpawnCoPlayer2", "CALLSITE",
      "inside Join pass", [SC + ": aligned; ECX = g_Game+0x1BAA8 (PlayerManager), EBX = PlayerManager (+0x70 character table), EDI = row pointer", RO + ": SpawnCoPlayer2 anchor"],
      "Needs caller registers and the stack argument, and denial jumps into the Join loop, so the call-site patch stays. Deny target v1.9.7.17 0x5BB561 -> v1.9.7.12 0x5925E4: EDI unchanged since the loop head 0x591E53, stack depth -268 before the push, same as the native jumps from 0x591E6E/0x591FA4 (IDA spd). The selector register changed: v1.9.7.17 EBX -> v1.9.7.12 EDI.",
      {"definition": "PlayerManager::SpawnCoPlayer2", "hooks": 0}),
 site("coop.odd_helper", C, 8, "0x555241", "0x2FD3F0", "0x52E921", "0x52E3E0", "Manager::Update", "0x2F9340", "odd-frame Player-only helper", "ZHL_OWN",
      "outer frame: odd-only", [SC + ": sim 0.989"], "Returns: helper -> 0x52E926 (v1.9.7.17 0x555246); Player update inside the helper -> 0x2F955C (v1.9.7.17 0x2FD60C)."),
]
for i, (s17, s12, regs) in enumerate([("0x5BAE7E", "0x5915AE", "manager=EBX key=ESI"), ("0x5BB081", "0x59174C", "manager=EBX key=ESI"),
                                      ("0x5BB288", "0x591E67", "manager=EBX key=ECX ([edi-8]) row=EDI")]):
    items.append(site("coop.mode_%d" % i, C, 9 + i, s17, "0x5BA980", s12, "0x591360", "Join pass", "0x5910A0", "PlayerManager::CoopBabiesOnly", "CALLSITE",
                      "inside Join pass", [SC + ": aligned; target sim 1.000", RO + ": CoopBabiesOnly anchor (static, no args, bool)"],
                      "Needs caller registers (v1.9.7.12: %s). v1.9.7.17 used EDI=manager, ESI/ECX=key, EBX=row, so the register reads change." % regs,
                      {"definition": "PlayerManager::CoopBabiesOnly", "hooks": 0}))

device = {
 "base_vtable": {"v17": "0x782758 (COL slot 0x782754)", "v12": "0x732F04", "col": "0x745F4C", "slots": 38, "pure_slots_v12": [3, 35], "pure_slots_v17": [3, 30]},
 "keyboard_vtable": {"v17": "0x7A1678 (.?AVInputDeviceKeyboard@Input@KAGE@@)", "v12": "0x73B490 (.?AVDeviceKeyboard@Input@KAGE@@)", "col": "0x7470B4"},
 "gamepad_vtable": {"v17": "0x782C80 (.?AVInputDeviceGamepad@Input@KAGE@@)", "v12": "0x73B3F0 (.?AVDeviceGamepad@Input@KAGE@@)", "col": "0x74704C", "note": "v1.9.7.12 Gamepad has an extra slot 38 (0x633310)"},
 "input_manager_vtable": {"v17": "0x782950", "v12": "0x732E44 (.?AVManager@Input@KAGE@@)", "col": "0x745ECC"},
 "base_ctor": {"v17": "0x61E050", "v12": "0x5EC570", "abi": "thiscall, no args", "evidence": SC + ": writes InputDeviceBase vftable; called by keyboard/gamepad creation and NetInputDevice ctor"},
 "initialize": {"v17": "0x61E210", "v12": "0x5EC6A0 (InputDeviceBase::Initialize, base slot 1)", "abi": "thiscall(void*), returns bool, retn 4", "evidence": RO + " + " + SC},
 "register": {"v17": "0x620DF0 (manager, object, control) retn 8", "v12": "InputManager vtable slot 45 (+0xB4) = 0x5EEB40", "abi": "thiscall(device*)", "evidence": SC + ": keyboard creation does push esi; call [eax+0B4h] on g_InputManagerBase"},
 "remove": {"v17": "0x620EB0", "v12": "InputManager vtable slot 46 (+0xB8) = 0x5EEBB0", "abi": "thiscall(device*)", "evidence": SC + ": removes from vector, then calls device slot 2 and slot 0(1) (deleting destructor)"},
 "allocator": {"v17": "0x60F4C0 (custom wrapper)", "v12": "operator new 0x686DE5 is what the game uses for devices", "note": "The game frees devices only through device slot 0, which the owned device overrides, so the owned object may use any allocator."},
 "control_block": {"v17": "0xCC10 control ctor; 6-slot control table", "v12": "NONE: InputManager stores raw device pointers (+4/+8/+0xC vector)", "evidence": SC},
 "object_size": {"v17_base": "0x370", "v12_base": "0x3F8", "keyboard_alloc": "0x2F98", "gamepad_alloc": "0x568", "evidence": SI + ": base writes up to +0x3F0; gamepad's own fields start at +0x3F8"},
 "fields_v12": {"device_id": "+0x08 (from counter 0x7E65EC; v1.9.7.17 key at +0x0C)", "enabled": "+0x0D (byte)", "template_table": "+0x10 = 0x7888C0, +0x14 = 40 entries x 8 bytes (v1.9.7.17: vector 0x8799E8, 32 x 16)",
                "mapping_count": "+0x18", "mapping_table": "+0x1C", "state_arrays": "+0xC8/+0xCC (v1.9.7.17 +0xC0/+0xC4)", "deadzone": "+0xD4", "evidence": SC + " (decompiled ctor/Initialize/queries)"},
 "owned_device_overrides_v12": {"slots": [0, 3, 8, 9, 10, 35], "v17_slots": [0, 3, 12, 13, 14, 30],
                                "notes": "slot 0 = deleting destructor called by RemoveDevice and by the update-failure path; it must not free (NEVER). slot 35 returning false makes InputManager remove and delete the device."},
 "keyboard_state": {"v17": "0x878C10", "v12": "0x7E6B08", "evidence": SI + ": both Poll routines copy current->previous, then raw->current; the v1.9.7.17 current-state array matches the v1.9.7.12 copy role. The 0x15C-byte blocks are 0x7E6B08 (current), 0x7E6C68 (raw), 0x7E6DC8 (previous)."},
}
globals_ = {
 "g_Game": {"v17": "0x871678", "v12": "0x7E49F4", "evidence": [SC + " (5 aligned uses)", RO + " (log value)"]},
 "g_Manager": {"v17": "0x87169C (prototype called it 'online')", "v12": "0x7E4A1C", "evidence": [SC, RO]},
 "g_LuaEngine": {"v17": "0x871690", "v12": "0x7E4A0C", "evidence": [SC, RO]},
 "g_InputManagerBase": {"v17": "0x857B18", "v12": "0x7CE438", "evidence": [SC, RO]},
 "device_id_counter": {"v12": "0x7E65EC", "evidence": [SC]},
}
fields = {
 "Game": {"0x2658A": "0x25B02 (run-active byte; next to _isDebug 0x25B01)", "0x1BAA8": "0x1BAA8 _playerManager", "0x1BAAC": "0x1BAAC", "0x26548": "0x25AC0 _gameStateFlags", "_console": "0x67FD4", "ExecuteCommand output capture": "0x68158"},
 "Manager": {"0x4B3D8/0x4B3DC": "0x4AFB8/0x4AFBC _netplayManager vector", "0x4B3E4": "0x4AFC4", "0x4B130": "0x4AD10 _starting", "0x2A3C0": "0x2A1FB"},
 "Player": {"0x161C": "0x1610 _playerIndex", "0x1618": "0x160C _controllerIndex", "0x173": "0x173 _dead", "0x20A9": "0x207D _isCoopGhost",
            "0x28/0x2C/0x30": "0x28/0x2C/0x30 type/variant/subtype", "0x1340/0x1344/0x134C": "unchanged (_maxHearts/_redHearts/_soulHearts)", "0x1D88": "0x1D74 _boneHearts",
            "0x360": "0x360 _velocity", "0x1624": "0x1618 _moveDirection", "0x162C/0x1630/0x1634/0x1638": "0x1620/0x1624/0x1628/(0x162C inferred)", "0x1378": "0x1378",
            "0x1580": "0x1574 _activeDesc", "0x1588": "0x157C", "0x17A0": "0x1790", "0x17C0": "0x17B0 _actionHoldDrop", "0x13BC": "0x13B8 _damageCooldown"},
 "JoinPass(PlayerManager)": {"rows": "+0x0C key / +0x14 state, stride 0x14, 4 rows (unchanged)", "character_table": "+0x70/+0x74 (unchanged)"},
 "evidence": [SC + ": aligned instruction votes (v12_fields.json)", SRC + ": REPENTOGON EntityPlayer.zhl / Entity.zhl / Game.zhl / Manager.zhl names and offsets"],
}
q10 = {"execute_command_binding": "0x4567F0 (registered in LuaEngine::RegisterClasses 0x44D340 as Isaac.ExecuteCommand)",
       "path": "if g_Game == null -> returns an empty string WITHOUT calling RunCommand; else sets g_Game+0x68158 = &result, calls Console::RunCommand(in, nullptr, nullptr), clears it, trims the trailing newline",
       "flags": "The binding writes only the capture pointer. The console-input caller 0x282240 writes only stack locals. A `cs` command handled before super() never runs the RunCommand body.",
       "evidence": SC, "open": "Whether g_Game is null on the main menu is UNKNOWN (static analysis cannot decide)."}
overlap = {"method": "343 REPENTOGON signatures (ASM.zhl + inline SigScan in repentogon/**/*.cpp) scanned on v1.9.7.12; 338 hits",
           "callsite_patches": "no REPENTOGON patch within 16 bytes of 0x5915AE/0x59174C/0x591E67/0x591FE6/0x5925E4",
           "entry_hooks": "no REPENTOGON patch inside the first 16 bytes of any function we entry-hook", "data": "artifacts/repentogon/tx-04-03-static/data/v12_overlap.json"}

doc = {"tx": "tx-04-03", "date": "2026-10-24",
       "target": {"game": "Repentance+ v1.9.7.12.J273", "pe_timestamp": "0x68154D8D", "image": "Repentogon/isaac-ng.exe unpacked with Steamless 3.1.0.5 (SteamStub 3.1)",
                  "imagebase": "0x00400000", "size_of_image_unpacked": "0x85F000", "repentogon": "v1.1.3 (installed; rgon_changelog.txt); local source 1.1.2-50-g421c91d08",
                  "runtime_anchor_base": "0x00BA0000 (repentogon.log 2026-10-24 00:40)"},
       "prototype": {"game": "Repentance+ v1.9.7.17.J460", "pe_timestamp": "0x69E6E3A7"},
       "address_rule": "All addresses are RVAs (subtract 0x400000 from IDA VA). No hash is computed or verified.",
       "mechanisms": {"ZHL_REPENTOGON": "ZHL hook on an existing REPENTOGON definition", "ZHL_OWN": "ZHL hook on an own address-constructed definition",
                      "POINTER": "own vtable slot write (deferred install)", "CALLSITE": "own call-site byte patch (deferred install, expected bytes checked)",
                      "DROP": "not installed in the first round", "NONE_IN_V12": "no counterpart exists in v1.9.7.12"},
       "items": items, "device": device, "globals": globals_, "fields": fields, "q10_execute_command": q10, "repentogon_overlap": overlap}
json.dump(doc, open(OUT, "w", encoding="utf-8"), indent=1, ensure_ascii=False)
print(len(items), "items")
