# ST5–ST7 主审源码摘录
2026-10-25；只读摘录，不代表运行时 Hook 排列或实际写入。

## F:\projects_isaac\issac_multiplay\mod\src\game\consumers.cpp

```text
15: } // namespace
16: ConsumerScope *CurrentConsumer() noexcept { return consumer; }
17: // 同一函数按冻结 ordinal 识别 A/B；旧关联只作中性拦截，不成为新的绑定依据。
18: int ControlledOrdinal(void *player) noexcept {
19:   auto *r = Runtime();
20:   if (!r || r->finished || !player)
21:     return -1;
22:   const unsigned count = r->mode == core::SessionMode::Host ? 2u : 1u;
23:   for (unsigned i = 0; i < count; ++i) {
24:     if (player == r->controlled_player[i] ||
25:         player == r->topology[i].pair.player)
26:       return int(i);
27:   }
28:   for (unsigned i = 0; i < count; ++i) {
29:     const auto observed = ReadTopology(i, i ? r->device.Object() : nullptr);
30:     if (observed.pair.player == player)
31:       return int(i);
32:   }
33:   return -1;
34: }
35: bool ControlledPlayer(void *player) noexcept {
36:   return ControlledOrdinal(player) >= 0;
37: }
38: ConsumerScope::ConsumerScope(void *value, ConsumerKind type) noexcept
39:     : player(value), kind(type), lease_(HostBusy()), main_(IsMain()),
40:       exceptions_(std::uncaught_exceptions()) {
41:   if (!main_)
42:     return;
43:   previous = consumer;
44:   consumer = this;
45:   if (!Armed())
46:     return;
47:   // 后端锁先于原生 consumer 内部 InputManager 的 vector 锁取得。
48:   if (!lease_.Ready())
49:     InputLeaseUnavailable();
50:   auto &r = *Runtime();
51:   RuntimeHealthy();
52:   RefreshLifecycle(r, false);
53:   const auto ordinal = ControlledOrdinal(player);
54:   if (ordinal < 0)
55:     return;
56:   const auto tick = r.pipeline.SimulationTick();
57:   if (tick && unsigned(kind) > 0 && unsigned(kind) <= 6)
58:     ++r.metrics[ordinal].consumers[unsigned(kind) - 1];
59:   if (kind == ConsumerKind::Shooting) {
60:     call_original_ = tick && !r.blocks && r.players[ordinal].BoundValid() &&
61:                      r.players[ordinal].Bound().player == player &&
62:                      ContextHealthy() && !InBlockedContext() && !r.failure;
63:     if (!call_original_) {
64:       ++r.suppressed;
65:       if (tick)
66:         ++r.metrics[ordinal].shooting_suppressed;
67:     }
68:     // 每次分派记录决定，harness 可核对无 tick 时没有原调用，不能用 wrapper
69:     // 计数替代。
70:     Trace("shooting_dispatch",
71:           "ordinal=%u original=%u tick=%llu blocks=%u loop=%llu",
72:           unsigned(ordinal), call_original_, tick, r.blocks, r.loop);
73:     r.last_dispatch_suppressed[ordinal] = !call_original_;
74:   }
75: }
76: ConsumerScope::~ConsumerScope() noexcept {
```

## F:\projects_isaac\issac_multiplay\mod\src\game\context.cpp

```text
24: } // namespace
25: Owner TopOwner() noexcept {
26:   return depth ? stack[depth - 1].owner : Owner::None;
27: }
28: uint64_t OwnerToken(Owner owner) noexcept {
29:   for (unsigned i = depth; i; --i)
30:     if (stack[i - 1].owner == owner)
31:       return stack[i - 1].token;
32:   return 0;
33: }
34: bool HasOwner(Owner owner) noexcept { return OwnerToken(owner) != 0; }
35: bool ContextHealthy() noexcept { return healthy; }
36: bool InBlockedContext() noexcept {
37:   return HasOwner(Owner::Start) || HasOwner(Owner::Restore) ||
38:          HasOwner(Owner::Lifecycle) || HasOwner(Owner::Door) ||
39:          HasOwner(Owner::NestedDoor) || HasOwner(Owner::Join);
40: }
```

## F:\projects_isaac\issac_multiplay\mod\src\game\persistence.cpp

```text
1: #include "game/bridge.h"
2: #include "game/context.h"
3: #include "game/topology.h"
4: #include "game/tick.h"
5: #include "runtime.h"
6: #include <cstring>
7: #include <exception>
8: 
9: namespace cs::game {
10: namespace {
11: thread_local const char *operation = "none";
12: // 仅读取 ST8 与官方结构均已确认的字段；无效下标不解引用。
13: bool Value(const char *entry, void *data, int index, int &value) noexcept {
14:   if (std::strcmp(entry, "pgd_unlock") == 0) {
15:     unsigned char unlocked = 0;
16:     if (index < 0 || index >= 641 || !ReadAt(data, 0x38 + index, unlocked)) return false;
17:     value = unlocked;
18:     return true;
19:   }
20:   if (std::strcmp(entry, "pgd_counter") == 0)
21:     return index >= 0 && index < 523 && ReadAt(data, 0x2BC + index * 4, value);
22:   value = 0;
23:   return true;
24: }
25: }
26: PersistenceOperation::PersistenceOperation(const char *name) noexcept : previous_(operation) { operation = name; }
27: PersistenceOperation::~PersistenceOperation() noexcept { operation = previous_; }
28: PersistenceScope::PersistenceScope(const char *entry, void *data, int index, int amount) noexcept
29:     : entry_(entry), operation_(operation), data_(data), index_(index), amount_(amount),
30:       exceptions_(std::uncaught_exceptions()) {
31:   if (!IsMain()) return;
32:   // Join 原生 Init/官方回调发生在 Join owner 内，也覆盖嵌套的 PGD 调用。
33:   if (std::strcmp(operation_, "none") == 0 && HostBusy() && HasOwner(Owner::Join)) operation_ = "join";
34:   if (std::strcmp(entry_, "button_maps") == 0) {
35:     known_ = true;
36:     Trace("persistent", "phase=enter entry=%s operation=%s known=1 loop=%llu", entry_, operation_, ManagerLoop());
37:     return;
38:   }
39:   known_ = ReadAt(data_, 0, dirty_) && ReadAt(data_, 1, readonly_) &&
40:            ReadAt(data_, 0xF8C, loaded_) && Value(entry_, data_, index_, before_);
41:   Trace("persistent", "phase=enter entry=%s operation=%s known=%u dirty=%u readonly=%u loaded=%u index=%d amount=%d value=%d loop=%llu",
42:         entry_, operation_, known_, dirty_, readonly_, loaded_, index_, amount_, before_, ManagerLoop());
43: }
44: PersistenceScope::~PersistenceScope() noexcept {
45:   if (!IsMain()) return;
46:   unsigned char dirty = 0;
47:   int after = 0;
48:   const bool maps = std::strcmp(entry_, "button_maps") == 0;
49:   const bool known = known_ && (maps || (ReadAt(data_, 0, dirty) && Value(entry_, data_, index_, after)));
50:   Trace("persistent", "phase=return entry=%s operation=%s known=%u normal=%u dirty_before=%u dirty_after=%u readonly=%u loaded=%u index=%d amount=%d before=%d after=%d loop=%llu",
51:         entry_, operation_, known, exceptions_ == std::uncaught_exceptions(), dirty_, dirty,
52:         readonly_, loaded_, index_, amount_, before_, after, ManagerLoop());
53: }
54: } // namespace cs::game
```

## F:\projects_isaac\issac_multiplay\mod\tools\gen_binding.py

```text
51:     else:
52:         # 此处只桥接主线程适配；原生异常由各层 RAII 观察后继续传播。
53:         if ident.startswith('pgd_') or ident == 'button_maps':
54:             persistence_args = ', achievementID' if ident == 'pgd_unlock' else ', eEventCounters, num' if ident == 'pgd_counter' else ''
55:             source.append(f'    cs::game::PersistenceScope persistence("{ident}", this{persistence_args});')
56:         if ident=='room_update':
57:             source.append('    cs::game::SimulationScope simulation(cs::game::SimulationKind::Room, this);')
58:         elif ident=='odd_helper':
59:             source.append('    cs::game::SimulationScope simulation(cs::game::SimulationKind::Odd);')
60:         elif ident=='player_update':
61:             source.append('    cs::game::PlayerScope player_scope(this);')
62:         elif ident=='player_native':
63:             source.append('    cs::game::PlayerNativeReached(this);')
64:         elif ident=='join_pass':
65:             source.append('    cs::game::JoinScope join_scope(this);')
66:         elif ident=='input_update':
67:             source += ['    cs::game::InputLease input_lease(cs::game::HostBusy());',
68:                        '    if (!input_lease.Ready()) cs::game::InputLeaseUnavailable();']
69:         elif ident=='game_exit':
70:             source.append('    if (scope.main) cs::game::RunTeardownBeginning();')
71:         elif ident=='device_update':
72:             source.append('    cs::game::DeviceScope device_scope(this);')
73:         elif ident=='restore':
74:             source += ['    if (scope.main) { cs::game::RestoreBeginning(); cs::RunExit(); }']
75:         consumers={'move':'Movement','bomb':'Bomb','active_item':'Item',
76:                    'card_pill':'Pill','shoot':'Shooting','drop':'Drop'}
77:         if ident in consumers:
78:             source.append(f'    cs::game::ConsumerScope consumer(this, cs::game::ConsumerKind::{consumers[ident]});')
79:             if ident=='shoot': source.append('    if (!consumer.CallOriginal()) return;')
80:         source.append(f'    {"auto result = " if ret!="void" else ""}super({call});')
81:         if ident=='device_update': source.append('    device_scope.Returned(result);')
```

## F:\projects_isaac\issac_multiplay\tools\harness\src\tx06\evidence.rs

```text
611:         let persistent = events
612:             .iter()
613:             .filter(|e| e.kind() == "persistent")
614:             .collect::<Vec<_>>();
615:         let writes = persistent
616:             .iter()
617:             .filter(|e| e.text("phase") == "return" && e.text("operation") != "none")
618:             .collect::<Vec<_>>();
619:         let no_added_writes = writes.iter().all(|e| {
620:             e.n("known") == Some(1)
621:                 && e.n("normal") == Some(1)
622:                 && !["pgd_local", "pgd_cloud", "button_maps"].contains(&e.text("entry"))
623:                 && e.n("before") == e.n("after")
624:                 && e.n("dirty_before") == e.n("dirty_after")
625:         });
626:         let observers = [
627:             "pgd_unlock",
628:             "pgd_counter",
629:             "pgd_local",
630:             "pgd_cloud",
631:             "button_maps",
632:         ]
633:         .iter()
634:         .all(|id| {
635:             events.iter().any(|e| {
636:                 e.kind() == "hook_registered"
637:                     && e.raw["detail"]
638:                         .as_str()
639:                         .is_some_and(|d| d.starts_with(&format!("{id} ")))
640:             })
641:         });
642:         let returns = persistent
643:             .iter()
644:             .filter(|e| e.text("phase") == "return")
645:             .count();
646:         let persistent_ok = observers
647:             && no_added_writes
648:             && !persistent.is_empty()
649:             && returns
650:                 == persistent
651:                     .iter()
652:                     .filter(|e| e.text("phase") == "enter")
653:                     .count()
654:             && persistent.iter().all(|e| e.n("known") == Some(1));
```

## F:\projects_isaac\REPENTOGON-1.1.3\include\IsaacRepentance.h

```text
854:     PLAYER_ISAAC = 0,
855:     PLAYER_MAGDALENE = 1,
856:     PLAYER_CAIN = 2,
857:     PLAYER_JUDAS = 3,
858:     PLAYER_BLUEBABY = 4,
859:     PLAYER_EVE = 5,
860:     PLAYER_SAMSON = 6,
861:     PLAYER_AZAZEL = 7,
862:     PLAYER_LAZARUS = 8,
863:     PLAYER_EDEN = 9,
864:     PLAYER_THELOST = 10,
```

```text
1700:         COLLECTIBLE_EVIL_CHARM = 632,
1701:         COLLECTIBLE_DOGMA = 633,
1702:         COLLECTIBLE_PURGATORY = 634,
1703:         COLLECTIBLE_STITCHES = 635,
1704:         COLLECTIBLE_R_KEY = 636,
1705:         COLLECTIBLE_KNOCKOUT_DROPS = 637,
```

```text
5983: 	int _pill; // 0xcc
5984: 	bool _canShoot; // 0xd0
5985: 	char pad0[0x3]; // 0xd1
5986: 	std_vector_int _collectibles; // 0xd4
5987: 	int _trinket; // 0xe0
5988: 	int _skinColor; // 0xe4
5989: 	int _achievement; // 0xe8
5990: 	int _brokenHeart; // 0xec
5991: 	unsigned int _pocketActiveID; // 0xf0
5992: 	std_string _birthrightDescription; // 0xf4
5993: 	std_string _bSkinParentName; // 0x10c
5994: 	int _moddedTaintedPlayerID; // 0x124
5995: 	bool _hidden; // 0x128
```

```text
8375: 	char pad22[0x8]; // 0x25af0
8376: 	EntityFactory * _entityFactory; // 0x25af8
8377: 	unsigned int _challenge; // 0x25afc
8378: 	bool _isRerun; // 0x25b00
8379: 	bool _isDebug; // 0x25b01
8380: 	char pad23[0x3e]; // 0x25b02
8381: 	Ambush _ambush; // 0x25b40
8382: 	char pad24[0x1c]; // 0x25b8c
8383: 	DailyChallenge _dailyChallenge; // 0x25ba8
8384: 	ScoreSheet _scoreSheet; // 0x25c5c
8385: 	char pad25[0x4]; // 0x25cec
```

## F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Game.zhl

```text
1: "538bdc83ec0883e4f883c404558b6b??896c24??8bec6aff68????????64a1????????505383ec105657a1????????33c5508d45??64a3????????8bf9897d??897d":
2: __thiscall void Game::constructor();
3: 
4: "558bec6aff68????????64a1????????5083ec40a1????????33c58945??535657508d45??64a3????????c745??00000000":
5: __thiscall void Game::StartDebug(int levelStage,int stageType,int difficulty,std_string* unk); 
6: 
7: "558bec6aff68????????64a1????????5081eca0000000a1????????33c58945??535657508d45??64a3????????8b1d":
8: __thiscall void Game::StartFromRerunState(GameState* state);
9: 
10: "558bec6aff68????????64a1????????5081ecac000000a1????????33c58945??535657508d45??64a3????????8bf9":
11: __thiscall void Game::RestoreState(GameState* state,bool startGame);
12: 
13: "558bec6aff68????????64a1????????5083ec78a1????????33c58945??535657508d45??64a3????????8bf1":
14: __thiscall void Game::SaveState(GameState* state);
15: 
16: "c781????????00000000c781????????00000000c781????????00000000c781????????00000000c781????????00000000c781????????00000000c781????????00000000c781????????00000000c781????????00000000c681????????00":
17: __thiscall void Game::ResetState();
```

## F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\EntityPlayer.zhl

```text
88: "558bec6aff68????????64a1????????5081ecec000000a1????????33c58945??535657508d45??64a3????????8bf9":
89: __thiscall void Entity_Player::InitPostLevelInitStats();
```

```text
296: "558bec6aff68????????64a1????????5083ec6ca1????????33c58945??5657508d45??64a3????????8bf9":
297: __thiscall void Entity_Player::InitPreLevelInitStats();
```

```text
747: 	int _playerType: 0x13bc;
```

```text
815: 	int _babySkin : 0x1ad8;
```

## F:\projects_isaac\REPENTOGON-1.1.3\repentogon\LuaInterfaces\CustomCallbacks.cpp

```text
521: // Still call Entity::Update() if we skip the subclass' update,
522: // since that aligns with how MC_PRE_NPC_UPDATE works.
523: #define HOOK_PRE_UPDATE_CALLBACK(_type, _metatable, _callbackid) \
524: HOOK_METHOD(_type, Update, () -> void) { \
525: 	if (!RunPreUpdateCallback(this, _metatable, _callbackid)) { \
526: 		super(); \
527: 	} else { \
528: 		((Entity*)this)->Original_Update(); \
529: 	} \
530: }
531: 
532: // PRE_X_UPDATE (1160 ~ 1168) (PRE_SLOT_UPDATE is 1169)
533: HOOK_PRE_UPDATE_CALLBACK(Entity_Player, lua::Metatables::ENTITY_PLAYER, 1160)
534: HOOK_PRE_UPDATE_CALLBACK(Entity_Tear, lua::Metatables::ENTITY_TEAR, 1161)
535: HOOK_PRE_UPDATE_CALLBACK(Entity_Familiar, lua::Metatables::ENTITY_FAMILIAR, 1162)
536: HOOK_PRE_UPDATE_CALLBACK(Entity_Bomb, lua::Metatables::ENTITY_BOMB, 1163)
```

## F:\projects_isaac\REPENTOGON-1.1.3\repentogon\Patches\ASMPatches\ASMPlayerManager.cpp

```text
20: // Patches into `PlayerManager::init_special_baby_selection` to hide modded characters from the co-op menu if they are still locked or hidden by an achievement.
21: bool __stdcall ShouldHideModdedCharacterInCoopWheel(EntityConfig_Player* conf) {
22: 	return conf->_hidden || (conf->_achievement > 0 && !g_Manager->GetPersistentGameData()->Unlocked(conf->_achievement)) || !IsCharacterUnlockedRgon(conf->_id) || IsCharacterHiddenByAchievementRgon(conf->_id);
23: }
24: void ASMPatchCoopWheelRespectModdedAchievements() {
25: 	SigScan scanner("80b8????????0075??8b47??895d");
26: 	scanner.Scan();
27: 	void* addr = scanner.GetAddress();
28: 
29: 	ZHL::Log("[REPENTOGON] Patching PlayerManager::init_special_baby_selection (to respect modded achievements) at %p\n", addr);
30: 
31: 	ASMPatch::SavedRegisters reg(ASMPatch::SavedRegisters::GP_REGISTERS_STACKLESS, true);
32: 	ASMPatch patch;
33: 	patch.PreserveRegisters(reg)
34: 		.Push(ASMPatch::Registers::EAX)  // EntityConfig_Player
35: 		.AddInternalCall(ShouldHideModdedCharacterInCoopWheel)
36: 		.AddBytes("\x84\xC0")  // test al, al
37: 		.RestoreRegisters(reg)
38: 		.AddRelativeJump((char*)addr + 0x7);
39: 	sASMPatcher.PatchAt(addr, &patch);
40: }
```

## F:\projects_isaac\REPENTOGON-1.1.3\repentogon\Patches\AchievementsStuff.cpp

```text
63: void SaveAchieveemntsToJson() {
64: 	if (!achset) { return; }
65: 	rapidjson::Document doc;
66: 	doc.SetObject();
67: 
68: 	//SavingAchievements
69: 	ArrayToJson(&doc, "Achievements", Achievements);
70: 
71: 	rapidjson::StringBuffer buffer;
72: 	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
73: 	doc.Accept(writer);
74: 
75: 	ofstream ofs(achivjsonpath);
76: 	ofs << buffer.GetString() << std::endl;
77: 	logViewer.AddLog("[REPENTOGON]", "Achievements saved to : % s \n", achivjsonpath.c_str());
78: }
```

```text
105: HOOK_METHOD(PersistentGameData, TryUnlock, (int achieveemntid) -> bool) {
106: 	//if (achieveemntid < 0) {
107: 		//AccomplishCondition(achieveemntid);
108: 		//return false;
109: 	//}
110: 	if ((achieveemntid >= 0) && (achieveemntid < 641)) {
111: 		if (forceunlock) {
112: 			bool auxreadonly = this->readonly;
113: 			this->readonly = false;
114: 			bool ret = super(achieveemntid);
115: 			this->readonly = auxreadonly;
116: 			forceunlock = false;
117: 			return ret;
118: 		}
119: 		return super(achieveemntid);
120: 	}
121: 	if (this->readonly && !forceunlock) {
122: 		return false;
123: 	}
124: 	forceunlock = false;
125: 	ANM2* AchievPop = g_Manager->GetAchievementOverlay()->GetANM2();
126: 	XMLAttributes modachiev = XMLStuff.AchievementData->GetNodeById(achieveemntid);
127: 	string achievid = modachiev["name"] + modachiev["sourceid"];
128: 	if (Achievements[achievid] <= 1) {
129: 		Achievements[achievid] = 2; //2 is for notified, 1 is accomplished, <1 is in progress
130: 		RunTrackersForAchievementCounter(achieveemntid);
131: 		SaveAchieveemntsToJson();
132: 		if (nextSkipAchiev != achieveemntid && ((modachiev.find("hidden") == modachiev.end()) || (modachiev["hidden"] == "false")) && g_Manager->GetOptions()->PopUpsEnabled()) { //it is prevented even without this check, but theres no point in doing the hackies if thats the case.
133: 			pendingachievs.push(achieveemntid);
134: 			if (dummyachiev < 0) {
135: 				dummyachiev = 2;
136: 				for (int x = 2; x < 641; x++) {
```

```text
248: HOOK_METHOD(PersistentGameData, IncreaseEventCounter, (int eEvent, int val) -> void) {
249: 	if (!this->readonly) {
250: 		int numplayers = g_Game->GetNumPlayers();
251: 		for (int i = 0; i < numplayers; i++) {
252: 			int playertype = g_Game->GetPlayer(i)->GetPlayerType();
253: 			RunTrackersForEventCounter(eEvent, playertype);
254: 		}
255: 	}
256: 	return super(eEvent,val);
257: }
```

## F:\projects_isaac\REPENTOGON-1.1.3\repentogon\Patches\GameStateManagement.cpp

```text
24: static GameStateSaveInfo get_info(GameState* gameState, GameStateIO* io, bool isRerun)
25: {
26: 	GameStateSaveInfo info;
27: 	if (!isRerun && get_game_state_slot(gameState) != GameStateSlot::SAVE_FILE) // This can occur when (presumably) the save state is being sent to online players joining mid-run
28: 	{
29: 		return info;
30: 	}
31: 
32: 	info.saveSlot = get_state_slot(gameState, io, isRerun);
33: 	if (!(1 <= info.saveSlot && info.saveSlot <= 3))
34: 	{
35: 		info.saveSlot = 0;
36: 		return info;
37: 	}
38: 
39: 	info.isRemote = is_remote_save(gameState);
40: 	info.fileName = get_state_file_name(gameState, info.saveSlot, isRerun);
41: 	if (!is_file_name_valid(info.fileName))
42: 	{
43: 		info.saveSlot = 0;
44: 		return info;
45: 	}
46: 
47: 	return info;
48: };
```

```text
181: {
182: 	const std::string& fileName = info.fileName;
183: 
184: 	if (!VirtualRoomSetManager::detail::WriteSave(gameState, info, isRerun))
185: 	{
186: 		return false;
187: 	}
188: 
189: 	ItemPoolManager::__SaveToDisk(fileName, isRerun);
190: 	return true;
191: }
192: 
193: // Return false to invalidate the game state
194: static inline bool read_save(GameState& gameState, const GameStateSaveInfo& info, bool isRerun)
195: {
196: 	const std::string& fileName = info.fileName;
197: 
198: 	if (!VirtualRoomSetManager::detail::ReadSave(gameState, info, isRerun))
199: 	{
200: 		return false;
201: 	}
202: 
203: 	ItemPoolManager::__LoadFromDisk(fileName, isRerun);
204: 	return true;
205: }
206: 
207: static inline void delete_save(const GameStateSaveInfo& info, bool isRerun)
208: {
209: 	const std::string& fileName = info.fileName;
210: 	uint32_t slot = info.saveSlot;
211: 
212: 	ItemPoolManager::__DeleteGameState(fileName);
213: 	VirtualRoomSetManager::detail::DeleteSave(info, isRerun);
214: 	ESSM::detail::SaveData::DeleteGameState(fileName);
215: }
216: 
217: #pragma region Hooks
218: 
219: HOOK_METHOD(Game, SaveState, (GameState* state) -> void)
220: {
```

```text
261: HOOK_METHOD(GameState, write, (GameStateIO** io) -> bool)
262: {
263: 	bool isRerun = false;
264: 	GameStateSaveInfo info = get_info(this, *io, isRerun);
265: 	if (info.IsUnknown())
266: 	{
267: 		ZHL::Log("[INFO] [GameStateManagement] skipping GameState write.\n");
268: 		return super(io);
269: 	}
270: 
271: 	ESSM::detail::SaveData::WriteState state = ESSM::detail::SaveData::WriteGameState();
272: 	VirtualRoomSetManager::detail::PreWriteGameState(*this, info);
273: 
274: 	bool success = super(io);
275: 
276: 	ESSM::detail::SaveData::RestoreWrittenStates(state);
277: 	VirtualRoomSetManager::detail::PostWriteGameState(*this, info);
278: 
279: 	if (!success)
280: 	{
281: 		return false;
282: 	}
283: 
284: 	const std::string& fileName = info.fileName;
285: 	ESSM::detail::SaveData::Serialize(fileName, state);
286: 	return write_save(*this, info, isRerun);
287: }
288: 
289: HOOK_METHOD(GameState, write_rerun, (GameStateIO** io) -> bool)
290: {
291: 	bool isRerun = true;
292: 	GameStateSaveInfo info = get_info(this, *io, isRerun);
293: 
294: 	ESSM::detail::SaveData::WriteState state = ESSM::detail::SaveData::WriteGameState();
295: 	VirtualRoomSetManager::detail::ClearDB(info);
296: 
297: 	bool success = super(io);
298: 
299: 	ESSM::detail::SaveData::RestoreWrittenStates(state);
300: 
301: 	if (!success)
302: 	{
303: 		return false;
304: 	}
305: 
306: 	if (info.IsUnknown())
307: 	{
308: 		ZHL::Log("[INFO] [GameStateManagement] skipping GameState write.\n");
309: 		return true;
310: 	}
311: 
312: 	ESSM::detail::SaveData::Serialize(info.fileName, state);
313: 	return write_save(*this, info, isRerun);
314: }
315: 
316: HOOK_METHOD(GameState, read, (GameStateIO** io, bool isLocalRun) -> bool)
317: {
318: 	bool isRerun = false;
319: 	GameStateSaveInfo info = get_info(this, *io, isRerun);
```

```text
390: HOOK_METHOD(GameState, Delete, () -> void)
391: {
392: 	bool isRerun = false;
393: 	GameStateSaveInfo info = get_info(this, this->_saveFile, false);
394: 
395: 	super();
396: 
397: 	if (info.IsUnknown())
398: 	{
399: 		ZHL::Log("[INFO] [GameStateManagement] Unknown game state, skipping delete.\n");
400: 		return;
401: 	}
402: 
403: 	delete_save(info, isRerun);
404: }
405: 
406: HOOK_METHOD(GameState, DeleteRerun, () -> void)
407: {
408: 	bool isRerun = true;
409: 	GameStateSaveInfo info = get_info(this, this->_saveFile, false);
410: 
411: 	super();
412: 
413: 	if (info.IsUnknown())
414: 	{
415: 		ZHL::Log("[INFO] [GameStateManagement] Unknown game state, skipping delete.\n");
416: 		return;
417: 	}
418: 
419: 	delete_save(info, isRerun);
420: }
421: #pragma endregion
```

## F:\projects_isaac\REPENTOGON-1.1.3\repentogon\SaveStateManagement\EntitySaveStateManagement.cpp

```text
1942:     static std::string get_save_data_path(const std::string& fileName)
1943:     {
1944:         return REPENTOGON::StringFormat("%s/EntitySaveStateManagement/%s.dat", REPENTOGON::GetRepentogonDataPath(), fileName.c_str());
1945:     }
```

```text
2193:     static void serialize(const std::string& filename, const _WriteState& writeState)
2194:     {
2195:         std::filesystem::path filePath = get_save_data_path(filename);
2196:         bool exists = REPENTOGON::EnsureParentPathExists(filePath);
2197:         if (!exists)
2198:         {
2199:             ZHL::Log(LOG_ERROR_HEADER "unable to create directory for save file at \"%s\"\n", filePath.string().c_str());
2200:             return;
2201:         }
2202: 
2203:         std::ofstream file = std::ofstream(filePath, std::ios::binary);
2204:         if (!file.is_open())
2205:         {
2206:             ZHL::Log(LOG_ERROR_HEADER "unable to open save file for writing at \"%s\"\n", filePath.string().c_str());
2207:             return;
2208:         }
2209: 
2210:         try
2211:         {
2212:             serialize_save(file, writeState);
2213:             file.flush();
2214: 
2215:             if (!file)
2216:             {
2217:                 throw std::runtime_error("flush failed");
2218:             }
2219:         }
2220:         catch(const std::runtime_error& e)
2221:         {
2222:             ZHL::Log(LOG_ERROR_HEADER "unable to serialize save file at \"%s\": %s\n", filePath.string().c_str(), e.what());
2223:             file.close();
2224:             remove_file(filePath);
2225:             return;
2226:         }
2227: 
2228:         ZHL::Log(LOG_INFO_HEADER "successfully saved %s to \"%s\"\n", filename.c_str(), filePath.string().c_str());
2229: 
2230:         uint32_t gameChecksum = g_Manager->_gamestate._checksum;
2231:         s_luaCallbacks.Serialize(writeState, filename, gameChecksum);
2232:     }
2233: 
2234:     void Serialize(const std::string& fileName, WriteState& writeState)
2235:     {
2236:         ScopedDebugContext context("Serialize");
2237: 
2238:         _WriteState& _writeState = writeState.m_writeState;
2239:         serialize(fileName, _writeState);
2240:         EntitySaveStateEx::WriteSaveFile(fileName, _writeState.writeEntityIdPairs);
2241: 
2242:         _writeState.FreeMemory();
```

```text
2307:     void DeleteGameState(const std::string& fileName)
2308:     {
2309:         EntitySaveStateEx::DeleteSaveFile(fileName);
2310: 
2311:         std::filesystem::path filePath = get_save_data_path(fileName);
2312:         std::error_code errorCode;
2313:         if (!std::filesystem::exists(filePath, errorCode))
2314:         {
2315:             return;
2316:         }
2317: 
2318:         std::filesystem::remove(filePath, errorCode);
2319:         if (errorCode)
2320:         {
2321:             ZHL::Log(LOG_ERROR_HEADER "unable to delete file at \"%s\"\n", filePath.string().c_str());
2322:         }
2323:         else
2324:         {
2325:             ZHL::Log(LOG_INFO_HEADER "deleted file at \"%s\"\n", filePath.string().c_str());
2326:         }
2327:     }
```

## F:\projects_isaac\REPENTOGON-1.1.3\repentogon\SaveStateManagement\EntitySaveStateEx.cpp

```text
200: static inline std::string GetSaveFilePath(const std::string& filename) {
201: 	return REPENTOGON::StringFormat("%s/EntitySaveStateManagement/ex_%s.json", REPENTOGON::GetRepentogonDataPath(), filename.c_str());
202: }
203: 
204: void WriteSaveFile(const std::string& filename, const std::vector<EntitySaveStateManagement::detail::SaveData::SerializeIdPair>& writeEntityIdPairs) {
205: 	std::filesystem::path filePath = GetSaveFilePath(filename);
206: 	ZHL::Log("[EntitySaveStateEx] [INFO] - Writing state to file: %s\n", filePath.string().c_str());
207: 
208: 	// Initialize directory, if needed
209: 	try {
210: 		std::filesystem::path directory = filePath.parent_path();
211: 		if (!directory.empty() && !std::filesystem::exists(directory)) {
212: 			std::filesystem::create_directories(directory);
213: 		}
214: 	} catch (const std::filesystem::filesystem_error& e) {
215: 		ZHL::Log("[EntitySaveStateEx] [ERROR] - Filesystem error attempting to create directories for state: %s\n", e.what());
216: 		return;
217: 	}
218: 
219: 	// Open file
220: 	std::ofstream file(filePath);
221: 	if (!file) {
222: 		ZHL::Log("[EntitySaveStateEx] [ERROR] - Failed to open file for writing.\n");
223: 		return;
224: 	}
225: 
226: 	// Initialize JSON document
227: 	rapidjson::Document doc;
228: 	doc.SetObject();
229: 	doc.AddMember(kVersion, JSON_FORMAT_VERSION, doc.GetAllocator());
230: 	doc.AddMember(kChecksum, g_Manager->_gamestate._checksum, doc.GetAllocator());
231: 
232: 	// Create EntitySaveStates
233: 	rapidjson::Value entityStates(rapidjson::kArrayType);
234: 	for (const auto& idPair : writeEntityIdPairs) {
235: 		if (EntitySaveStateEx* state = ENTITY_SAVE_STATES[idPair.realId].get()) {
236: 			rapidjson::Value serializedData(rapidjson::kObjectType);
237: 			state->Serialize(serializedData, doc.GetAllocator());
238: 			if (!serializedData.ObjectEmpty()) {
239: 				rapidjson::Value entry(rapidjson::kObjectType);
240: 				entry.AddMember(kID, idPair.serializeId, doc.GetAllocator());
241: 				entry.AddMember(kType, state->GetType(), doc.GetAllocator());
242: 				entry.AddMember(kData, serializedData, doc.GetAllocator());
243: 				entityStates.PushBack(entry, doc.GetAllocator());
244: 			}
245: 		}
246: 	}
247: 	doc.AddMember(kEntitySaveStates, entityStates, doc.GetAllocator());
248: 
249: 	// Write the file
250: 	rapidjson::StringBuffer buffer;
251: 	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
252: 	doc.Accept(writer);
253: 	file << buffer.GetString() << std::endl;
254: 
255: 	ZHL::Log("[EntitySaveStateEx] [INFO] - Successfully wrote state.\n");
```

```text
353: void DeleteSaveFile(const std::string& filename) {
354: 	try {
355: 		std::filesystem::path filePath = GetSaveFilePath(filename);
356: 		if (std::filesystem::exists(filePath)) {
357: 			ZHL::Log("[EntitySaveStateEx] [INFO] - Deleting state: %s\n", filePath.string().c_str());
358: 			std::filesystem::remove(filePath);
359: 			ZHL::Log("[EntitySaveStateEx] [INFO] - Successfully deleted state.\n");
360: 		}
361: 	} catch (const std::filesystem::filesystem_error& e) {
362: 		ZHL::Log("[EntitySaveStateEx] [ERROR] - Filesystem error attempting to delete save file: %s\n", e.what());
363: 	}
364: }
```

## F:\projects_isaac\REPENTOGON-1.1.3\repentogon\Patches\ItemPoolManager.cpp

```text
886: static inline std::string get_state_file_path(const std::string& fileName)
887: {
888: 	return std::string(REPENTOGON::GetRepentogonDataPath()).append("ItemPoolManager/").append(fileName).append(".json");
889: }
```

```text
1476: void ItemPoolManager::__SaveToDisk(const std::string& fileName, bool isRerun)
1477: {
1478: 	Error error;
1479: 	auto& instance = ItemPoolManager::Get();
1480: 
1481: 	std::filesystem::path filePath = get_state_file_path(fileName);
1482: 	auto file = open_game_state_file(filePath, error);
1483: 	if (error)
1484: 	{
1485: 		ZHL::Log("[ItemPoolManager] [ERROR] - " __FUNCTION__ " - unable to save %s: %s\n", fileName.c_str(), error.value().c_str());
1486: 		return;
1487: 	}
1488: 
1489: 	rapidjson::Document doc = instance.serialize_game_state(isRerun);
1490: 
1491: 	rapidjson::StringBuffer buffer;
1492: 	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
1493: 	doc.Accept(writer);
1494: 
1495: 	file.value() << buffer.GetString() << std::endl;
1496: 	ZHL::Log("[ItemPoolManager] [INFO] - " __FUNCTION__ " - successfully saved %s to \"%s\"\n", fileName.c_str(), filePath.string().c_str());
1497: }
1498: 
1499: void ItemPoolManager::__LoadFromDisk(const std::string& fileName, bool isRerun)
1500: {
```

```text
1520: void ItemPoolManager::__DeleteGameState(const std::string& fileName)
1521: {
1522: 	std::filesystem::path filePath = get_state_file_path(fileName);
1523: 
1524: 	std::error_code errorCode;
1525: 	if (!std::filesystem::exists(filePath, errorCode))
1526: 	{
1527: 		return;
1528: 	}
1529: 
1530: 	bool status = std::filesystem::remove(filePath, errorCode);
1531: 	if (!status || errorCode)
1532: 	{
1533: 		if (errorCode)
1534: 		{
1535: 			ZHL::Log("[ItemPoolManager] [ERROR] - " __FUNCTION__ " - unable to delete %s at \"%s\": %s\n", fileName.c_str(), filePath.string().c_str(), errorCode.message().c_str());
```

## F:\projects_isaac\REPENTOGON-1.1.3\repentogon\VirtualRoomConfig\VirtualRoomSetManager.cpp

```text
948: static std::string get_db_path(const std::string& fileName)
949: {
950: 	return REPENTOGON::StringFormat("%s/VirtualRoomSetManager/%s.json", REPENTOGON::GetRepentogonDataPath(), fileName.c_str());
951: }
952: 
953: static bool WriteDB(const std::string& filePath, uint32_t gameChecksum, RestoreRoomsDB& db)
954: {
955: 	// open file
956: 	std::filesystem::path path(filePath);
957: 	if (!REPENTOGON::EnsureParentPathExists(path))
958: 	{
959: 		ZHL::Log(LOG_ERROR_HEADER "Unable to create parent path for \"%s\"\n", filePath.c_str());
960: 		return false;
961: 	}
962: 
963: 	std::ofstream file(path);
964: 	if (!file)
965: 	{
966: 		ZHL::Log(LOG_ERROR_HEADER "Unable to open file for writing \"%s\"\n", filePath.c_str());
967: 		return false;
968: 	}
969: 	
970: 	// serialize
971: 	rapidjson::Document doc;
972: 	auto& save = doc.SetObject();
973: 	auto& allocator = doc.GetAllocator();
974: 
975: 	save.AddMember("Version", DB_VERSION, allocator);
976: 	save.AddMember("Checksum", gameChecksum, allocator);
977: 	save.AddMember("Rooms", rapidjson::Value(rapidjson::kArrayType), allocator);
978: 
979: 	// serialize rooms
980: 	rapidjson::Value& rooms = doc["Rooms"];
981: 	for (RoomConfig_Room* room : db.restoredRooms)
982: 	{
983: 		assert(room); // Each saved room should never be nullptr
984: 		rooms.PushBack(RoomConfigUtility::SerializeRoom(*room, allocator), allocator);
985: 	}
986: 
987: 	// write to file
988: 	rapidjson::StringBuffer buffer;
989: 	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
990: 	doc.Accept(writer);
991: 
992: 	file << buffer.GetString() << std::endl;
993: 
994: 	ZHL::Log(LOG_INFO_HEADER "Successfully saved DB at \"%s\"\n", filePath.c_str());
995: 	return true;
996: }
997: 
998: static bool ReadDB(const std::string& filePath, uint32_t gameChecksum, RestoreRoomsDB& db)
999: {
1000: 	db.init();
```

```text
1167: bool VirtualRoomSetManager::detail::WriteSave(const GameState& gameState, const GameStateSaveInfo& saveInfo, bool isRerun)
1168: {
1169: 	// rooms are not saved on Rerun
1170: 	if (isRerun)
1171: 	{
1172: 		return true;
1173: 	}
1174: 
1175: 	size_t gameChecksum = gameState._checksum;
1176: 	RestoreRoomsDB& db = get_db_for_slot(saveInfo.saveSlot, saveInfo.isRemote);
1177: 
1178: 	std::string filePath = get_db_path(saveInfo.fileName);
1179: 	bool successfulWrite = WriteDB(filePath, gameChecksum, db);
1180: 	bool failure = !successfulWrite &&
1181: 		db.restoredRooms.size() > 0; // at least one hijack
1182: 
1183: 	return !failure;
1184: }
```

```text
1220: void VirtualRoomSetManager::detail::DeleteSave(const GameStateSaveInfo& saveInfo, bool isRerun)
1221: {
1222: 	// reset db
1223: 	RestoreRoomsDB& db = get_db_for_slot(saveInfo.saveSlot, saveInfo.isRemote);
1224: 	db.init();
1225: 
1226: 	// rooms are not saved on Rerun
1227: 	if (isRerun)
1228: 	{
1229: 		return;
1230: 	}
1231: 
1232: 	// delete file
1233: 	std::filesystem::path filePath = get_db_path(saveInfo.fileName);
1234: 	std::error_code errorCode;
1235: 	if (!std::filesystem::exists(filePath, errorCode))
1236: 	{
1237: 		return;
1238: 	}
1239: 
1240: 	bool deleteStatus = std::filesystem::remove(filePath, errorCode);
1241: 	if (errorCode)
1242: 	{
1243: 		ZHL::Log(LOG_ERROR_HEADER "Unable to delete DB at \"%s\": %s\n", filePath.string().c_str(), errorCode.message().c_str());
1244: 	}
1245: 	else if (!deleteStatus)
1246: 	{
1247: 		ZHL::Log(LOG_ERROR_HEADER "Unable to delete DB at \"%s\"\n", filePath.string().c_str());
1248: 	}
1249: 	else
1250: 	{
1251: 		ZHL::Log(LOG_INFO_HEADER "Deleted DB at \"%s\"\n", filePath.string().c_str());
```
