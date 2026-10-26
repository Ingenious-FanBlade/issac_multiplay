# tx-04-07 ST1 官方源码静态摘录

2026-10-25。来源：F:/projects_isaac/REPENTOGON-1.1.3；仅读取，未构建或运行。每行保留原文件行号。

## libzhl/functions/Entity.zhl
270: 		
271:         "558bec83e4f883ec3c5356578bf9837f??01":
272:         bool TakeDamage(float Damage, unsigned long long DamageFlags, EntityRef *Source, int DamageCountdown);
273: 		
274:         "558bec83e4f8568bf15780be????????00":
275:         void Kill(EntityRef* Source);
276: 		
277:         "51568bf15780be????????00c686????????007507":
278:         void Remove(); 

## libzhl/functions/EntityPlayer.zhl
495: __thiscall bool Entity_Player::IsHeadless();
496: 
497: "80b9????????0074??8b81????????83f81d74??83f82675??8b81????????85c074??8b90????????83faff74??3b91????????7d??3bc174":
498: __thiscall bool Entity_Player::IsHologram();
499: 
500: "558bec6aff68????????64a1????????5083ec24535657a1????????33c5508d45??64a3????????8bf98b87":
501: __thiscall void Entity_Player::MorphToCoopGhost();
502: 

## include/IsaacRepentance.h
849:     SPAWN_GRAVITY = 10000,
850: };
851: 
852: enum ePlayerType {
853:     PLAYER_POSSESSOR = -1,
854:     PLAYER_ISAAC = 0,
855:     PLAYER_MAGDALENE = 1,
856:     PLAYER_CAIN = 2,

## repentogon/LuaInterfaces/CustomCallbacks.cpp
6498: EntityRef* lastSource = nullptr;
6499: 
6500: HOOK_METHOD(Entity, Kill, (EntityRef* source) -> void) {
6501: 	lastSource = source;
6502: 	super(source);
6503: 	lastSource = nullptr;
6504: }
6505: 
6506: //POST_ENTITY_KILL reimplementation (id: 68)
6507: HOOK_STATIC(LuaEngine, PostEntityKill, (Entity* ent) -> void, __stdcall) {
6508: 	const int callbackid = 68;
6509: 	if (VanillaCallbackState.test(callbackid)) {
6510: 		lua_State* L = g_LuaEngine->_state;
6511: 		lua::LuaStackProtector protector(L);
6512: 
6513: 		lua_rawgeti(L, LUA_REGISTRYINDEX, g_LuaEngine->runCallbackRegistry->key);
6514: 
6515: 		lua::LuaResults result = lua::LuaCaller(L).push(callbackid)
6516: 			.push(ent->_type)
6517: 			.push(ent, lua::Metatables::ENTITY)
6518: 			.push(lastSource, lua::Metatables::ENTITY_REF)
6519: 			.call(1);
6520: 	}
6521: }
6522: 
6523: //MC_POST_PLAYER_DROP_TRINKET (id : 1144)
6524: HOOK_METHOD(Entity_Player, DropTrinket, (Vector* DropPos, bool ReplaceTick) -> Entity_Pickup*) {

## repentogon/Patches/ExtraLives.cpp
29: 
30: // TriggerDeath calls (that aren't `checkOnly`) are done in PlayerManager::Update, where it iterates over the players to look for dead ones no longer playing an animation.
31: // We may call TriggerDeath earlier than normal on the non-main twin, in order to allow mods to save the main twin with the other twin's revives.
32: // Since the game is going to iterate to the non-main twin still, we mark the twin's player index here to prevent callbacks from getting excecuted twice.
33: static std::unordered_set<int> s_EarlyTriggeredTwinDeaths;
34: 
35: HOOK_METHOD(PlayerManager, Update, () -> void) {
36:     super();
37:     s_EarlyTriggeredTwinDeaths.clear();
38: }

## repentogon/Patches/ExtraLives.cpp
97: // MC_PRE_TRIGGER_PLAYER_DEATH (id: 1050)
98: HOOK_METHOD(Entity_Player, TriggerDeath, (bool checkOnly) -> bool) {
99: 	if (checkOnly) {
100:         if (super(checkOnly) || GetCustomReviveCount(this, /*includeHidden=*/true) > 0) {
101:             return true;
102:         }
103:         if (Entity_Player* twin = this->GetTwinPlayer()) {
104:             return GetCustomReviveCount(twin, /*includeHidden=*/true) > 0;
105:         }
106:         return false;
107: 	}
108: 	this->_damageCooldown = 1;
109: 	if (RunTriggerDeathCallback(this, 1050)) {
110: 		return true;
111: 	}
112: 	if (!this->_exists) {
113: 		return false;
114: 	}
115: 	return super(checkOnly);
116: }
117: 
118: // MC_TRIGGER_PLAYER_DEATH_POST_CHECK_REVIVES (id: 1051)
119: void __stdcall RunPostTriggerPlayerDeathCheckRevivesCallback(Entity_Player* player) {
120: 	RunTriggerDeathCallback(player, 1051);
121:     if (player->_exists && player->_dead) {
122: 		if (Entity_Player* twin = player->GetTwinPlayer(); twin && twin->_exists) {
123:             if (!twin->_dead) {
124:                 player->Revive();
125:                 PostReviveChecks(player);
126:             } else if (player->_playerIndex < twin->_playerIndex) {
127:                 // If the main twin is about to die, trigger death on the other twin early to fish for modded revives.
128:                 // Otherwise by the time this happens naturally it may be too late for mods to prevent the run from ending!
129:                 twin->TriggerDeath(false);
130: 				if (!player->_dead) {
131: 					PostReviveChecks(player);
132: 				}
133:                 s_EarlyTriggeredTwinDeaths.insert(twin->_playerIndex);
134:             }
135:         }
136:     }

## repentogon/Patches/ItemSpoofSystem.cpp
346: HOOK_METHOD_PRIORITY(Entity_Player, TriggerDeath, 9999, (bool checkOnly) -> bool) {
347:     bool res = super(checkOnly);
348:     if (!checkOnly) {
349:         if (EntityPlayerPlus* playerPlus = GetEntityPlayerPlus(this)) {
350:             playerPlus->itemSpoofs.CheckPlayerType(*this, false);
351:         }
352:     }
353:     return res;
354: }
355: HOOK_METHOD_PRIORITY(Entity_Player, RestoreGameState, -9999, (GameStatePlayer* saveState) -> void) {

## repentogon/Patches/AchievementsStuff.cpp
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
258: 
259: 

## repentogon/Patches/AchievementsStuff.h
35: 	simplifiedeventsenum["UNKNOWN_EVENT_8"] = 8;
36: 	simplifiedeventsenum["ARCADES_ENTERED"] = 9; //Also includes teleports with goto.
37: 	simplifiedeventsenum["DEATHS"] = 10; //does not increase when revived
38: 	simplifiedeventsenum["ISAAC_KILLS"] = 11; //How often was the final boss of cathedral killed. (Isaac)(Does only count instances where the kill is combined with run "progression".aka in final room and spawning a trapdoor / chest / trophy)
39: 	simplifiedeventsenum["SHOPKEEPER_KILLED"] = 12;
