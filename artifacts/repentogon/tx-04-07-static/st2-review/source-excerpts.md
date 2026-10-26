# ST2 主审官方源码摘录

2026-10-26。只读摘录，未构建或运行。

## libzhl/functions/EntityPlayer.zhl

120: 
121: "558bec6aff68????????64a1????????5081ecdc010000":
122: __thiscall void Entity_Player::Render(Vector *offset);
123: 
124: "8b81????????83c0fc83f824":
125: __thiscall int Entity_Player::GetHealthType();
126: 

510: __thiscall void Entity_Player::RerollAllCollectibles(RNG * rng, bool includeActives);
511: 
512: "558bec83e4f883ec10568bf157837e??00":
513: __thiscall void Entity_Player::ResetPlayer();
514: 
515: "558bec83e4f883ec145356578bf9c74424??00001000":
516: __thiscall void Entity_Player::RevivePlayerGhost();
517: 

## repentogon/LuaInterfaces/Entities/LuaEntityPlayer.cpp

2355: LUA_FUNCTION(Lua_PlayerReviveCoopGhost) {
2356: 	Entity_Player* player = lua::GetLuabridgeUserdata<Entity_Player*>(L, 1, lua::Metatables::ENTITY_PLAYER, "EntityPlayer");
2357: 	bool res = false;
2358: 	
2359: 	//ZHL::Log("IsPlayerGhost: %s\n", player->_isCoopGhost ? "TRUE" : "FALSE");
2360: 	if (player->_isCoopGhost) {
2361: 		//ZHL::Log("attempting revive");
2362: 		player->RevivePlayerGhost();
2363: 		res = true;
2364: 	}
2365: 	
2366: 	lua_pushboolean(L, res);
2367: 	return 1;
2368: }

3748: 		{ "RerollAllCollectibles", Lua_PlayerRerollAllCollectibles },
3749: 		{ "ResetPlayer", Lua_PlayerResetPlayer },
3750: 		{ "ReviveCoopGhost", Lua_PlayerReviveCoopGhost },
3751: 		{ "SalvageCollectible", Lua_PlayerSalvageCollectible },
3752: 		{ "SetControllerIndex", Lua_PlayerSetControllerIndex },

## repentogon/Patches/CustomCache.cpp

290: 			TriggerCustomCache(player, cache, immediate);
291: 		}
292: 	}
293: }
294: 
295: void TriggerNullCustomCache(Entity_Player* player, const int id, const bool immediate) {
296: 	TriggerCustomCache(player, ItemConfigEx::GetNullItemEx(id), immediate);
297: }
298: 
299: void TriggerCollectibleCustomCache(Entity_Player* player, const int id, const bool immediate) {
300: 	TriggerCustomCache(player, ItemConfigEx::GetCollectibleEx(id), immediate);
301: }
302: 
303: void TriggerTrinketCustomCache(Entity_Player* player, const int id, const bool immediate) {
304: 	TriggerCustomCache(player, ItemConfigEx::GetTrinketEx(id), immediate);
305: }
306: 
307: void TriggerItemCustomCache(Entity_Player* player, ItemConfig_Item* item, const bool immediate) {
308: 	if (item->type == 0) {  // Null
309: 		TriggerNullCustomCache(player, item->id, immediate);
310: 	} else if (item->type == 2) {  // Trinket
311: 		TriggerTrinketCustomCache(player, item->id, immediate);
312: 	} else {  // Collectible (Passive/Active/Familiar)
313: 		TriggerCollectibleCustomCache(player, item->id, immediate);
314: 	}
315: }
316: 
317: // Re-evaluate custom caches that appeared in XML data if EvaluateItems is run for CacheFlag.CACHE_ALL.
318: HOOK_METHOD_PRIORITY(Entity_Player, EvaluateItems, -1, () -> void) {
319: 	EntityPlayerPlus* playerPlus = GetEntityPlayerPlus(this);
320: 	if (playerPlus) {
321: 		if ((this->_cacheFlags & CACHE_ALL) == CACHE_ALL) {  // If visual studio tries to tell you that this equality check is not needed do NOT listen
322: 			for (const std::string& customcache : ItemConfigEx::GetKnownCustomCaches()) {
323: 				playerPlus->customCacheTags.insert(customcache);
324: 			}
325: 		}
326: 		if (!playerPlus->customCacheTags.empty()) {
327: 			// Make a copy of the customCacheTags and clear before iterating,
328: 			// to avoid any funny business with inserts/removals from the set.
329: 			const std::set<std::string> customcaches = playerPlus->customCacheTags;
330: 			playerPlus->customCacheTags.clear();
331: 			for (const std::string& customcache : customcaches) {
332: 				RunEvaluateCustomCacheCallback(this, customcache);
333: 			}
334: 		}
335: 		playerPlus->customCacheRequiresEvaluateItemsCall = false;
336: 	}
337: 
338: 	super();
339: }

## repentogon/LuaInterfaces/CustomCallbacks.cpp

266: //SFX_PRE/POST_PLAY (id: 1030/1031)
267: void ProcessPostSFXPlay(int ID, float Volume, int FrameDelay, bool Loop, float Pitch, float Pan)
268: {
269: 	const int callbackid = 1031;
270: 	if (CallbackState.test(callbackid - 1000)) {
271: 		lua_State* L = g_LuaEngine->_state;
272: 		lua::LuaStackProtector protector(L);
273: 
274: 		lua_rawgeti(L, LUA_REGISTRYINDEX, g_LuaEngine->runCallbackRegistry->key);
275: 
276: 		lua::LuaCaller(L).push(callbackid)
277: 			.push(ID)
278: 			.push(ID)
279: 			.push(Volume)
280: 			.push(FrameDelay)
281: 			.push(Loop)
282: 			.push(Pitch)
283: 			.push(Pan)
284: 			.call(1);
285: 	}
286: }
287: 
288: HOOK_METHOD(SFXManager, Play, (int ID, float Volume, int FrameDelay, bool Loop, float Pitch, float Pan) -> void) {
289: 	const int callbackid = 1030;
290: 	if (!CallbackState.test(callbackid - 1000)) {
291: 		super(ID, Volume, FrameDelay, Loop, Pitch, Pan);
292: 		ProcessPostSFXPlay(ID, Volume, FrameDelay, Loop, Pitch, Pan);
293: 		return;
294: 	}
295: 		lua_State* L = g_LuaEngine->_state;
296: 		lua::LuaStackProtector protector(L);
297: 
298: 		lua_rawgeti(L, LUA_REGISTRYINDEX, g_LuaEngine->runCallbackRegistry->key);
299: 
300: 		lua::LuaResults result = lua::LuaCaller(L).push(callbackid)
301: 			.push(ID)
302: 			.push(ID)
303: 			.push(Volume)
304: 			.push(FrameDelay)
305: 			.push(Loop)
306: 			.push(Pitch)
307: 			.push(Pan)
308: 			.call(1);
309: 	if (!result) {
310: 		if (lua_istable(L, -1)) {
311: 			if (lua_rawlen(L, -1) == 6) {
312: 				ID = lua::callbacks::ToInteger(L, 1);
313: 				Volume = (float) lua::callbacks::ToNumber(L, 2);
314: 				FrameDelay = lua::callbacks::ToInteger(L, 3);
315: 				Loop = lua::callbacks::ToBoolean(L, 4);
316: 				Pitch = (float) lua::callbacks::ToNumber(L, 5);
317: 				Pan = (float) lua::callbacks::ToNumber(L, 6);
318: 				super(ID, Volume, FrameDelay, Loop, Pitch, Pan);
319: 				ProcessPostSFXPlay(ID, Volume, FrameDelay, Loop, Pitch, Pan);
320: 				return;
321: 			}
322: 		}
323: 		else if (lua_isboolean(L, -1)) {
324: 			if (!lua_toboolean(L, -1)) { // this particular if statement can't be combined with the one above it, or it fails to register false properly
325: 				return;
326: 			}
327: 		}
328: 	}
329: 	super(ID, Volume, FrameDelay, Loop, Pitch, Pan);
330: 	ProcessPostSFXPlay(ID, Volume, FrameDelay, Loop, Pitch, Pan);

1574: HOOK_METHOD(Entity_Player, GetHealthType, () -> int) {
1575: 	int vanillaHealthType = super();
1576: 	int defaultHealthType = vanillaHealthType;
1577: 
1578: 	// Check for HealthType specified in players.xml
1579: 	if (const int xmlHealthType = XMLStuff.PlayerData->GetNumberAttributeById(this->GetPlayerType(), "healthtype"); xmlHealthType >= 0 && xmlHealthType <= 4) {
1580: 		defaultHealthType = xmlHealthType;
1581: 	}
1582: 
1583: 	EntityPlayerPlus* playerPlus = GetEntityPlayerPlus(this);
1584: 
1585: 	if (!playerPlus || playerPlus->disableHealthTypeModification) {
1586: 		return defaultHealthType;
1587: 	} else if (playerPlus->evaluatingHealthType) {
1588: 		// Prevent recursion.
1589: 		return playerPlus->previousHealthType > -1 ? playerPlus->previousHealthType : defaultHealthType;
1590: 	}
1591: 
1592: 	playerPlus->evaluatingHealthType = true;
1593: 
1594: 	int previousHealthType = vanillaHealthType;
1595: 
1596: 	if (playerPlus->previousHealthType > -1) {
1597: 		previousHealthType = playerPlus->previousHealthType;
1598: 	}
1599: 
1600: 	int healthType = defaultHealthType;
1601: 
1602: 	if (playerPlus && playerPlus->customCacheResults.find("healthtype") != playerPlus->customCacheResults.end()) {
1603: 		const int cacheHealthType = (int)playerPlus->customCacheResults["healthtype"];
1604: 		if (cacheHealthType >= 0 && cacheHealthType <= 4) {
1605: 			healthType = cacheHealthType;
1606: 		}
1607: 	}
1608: 
1609: 	// MC_PLAYER_GET_HEALTH_TYPE
1610: 	const int callbackid = 1067;
1611: 	if (CallbackState.test(callbackid - 1000)) {
1612: 		lua_State* L = g_LuaEngine->_state;
1613: 		lua::LuaStackProtector protector(L);
1614: 
1615: 		lua_rawgeti(L, LUA_REGISTRYINDEX, g_LuaEngine->runCallbackRegistry->key);
1616: 
1617: 		lua::LuaResults result = lua::LuaCaller(L).push(callbackid)
1618: 			.push(this->GetPlayerType())
1619: 			.push(this, lua::Metatables::ENTITY_PLAYER)
1620: 			.push(previousHealthType)
1621: 			.push(defaultHealthType)
1622: 			.call(1);
1623: 
1624: 		if (!result && lua_isinteger(L, -1)) {
1625: 			const int luaHealthType = (int)lua_tointeger(L, -1);
1626: 			if (luaHealthType >= 0 && luaHealthType <= 4) {
1627: 				healthType = luaHealthType;
1628: 			}
1629: 		}
1630: 	}
1631: 
1632: 	// If the player's health type changes dynamically, convert their health as necessary to avoid bugs from health they should not be able to have.
1633: 	if (healthType != previousHealthType) {
1634: 		// Update this right away to mitigate weirdness from any recursive calls.
1635: 		playerPlus->previousHealthType = healthType;
1636: 
1637: 		bool convertHeartContainers = true;
1638: 
1639: 		// MC_PLAYER_HEALTH_TYPE_CHANGE
1640: 		const int callbackid = 1128;
1641: 		if (CallbackState.test(callbackid - 1000)) {
1642: 			lua_State* L = g_LuaEngine->_state;
1643: 			lua::LuaStackProtector protector(L);
1644: 
1645: 			lua_rawgeti(L, LUA_REGISTRYINDEX, g_LuaEngine->runCallbackRegistry->key);
1646: 
1647: 			lua::LuaCaller(L).push(callbackid)
1648: 				.push(this->GetPlayerType())
1649: 				.push(this, lua::Metatables::ENTITY_PLAYER)
1650: 				.push(healthType)
1651: 				.push(previousHealthType)
1652: 				.push(defaultHealthType)
1653: 				.call(1);
1654: 		}
1655: 
1656: 		if (healthType == 1) {  // Soul
1657: 			this->_redHearts = 0;
1658: 			this->_rottenHearts = 0;

751: HOOK_METHOD(Entity_Player, AddEternalHearts, (int amount) -> void) {	//eternal
752: 	if (!CallbackState.test(1009 - 1000) && !CallbackState.test(1010 - 1000)) {
753: 		super(amount);
754: 	}
755: 	else {
756: 		std::optional<int> heartcount = PreAddHeartsCallbacks(this, amount, 1 << 4, std::nullopt);
757: 		amount = heartcount.value_or(amount);
758: 		super(amount);
759: 	}
760: 	if (CallbackState.test(1010 - 1000)) {
761: 		PostAddHeartsCallbacks(this, amount, 1 << 4, std::nullopt);
762: 	}
763: }
764: 
765: HOOK_METHOD(Entity_Player, AddGoldenHearts, (int amount, bool retBool) -> void) {	//golden
766: 	if (!CallbackState.test(1009 - 1000)) {
767: 		super(amount, retBool);
768: 	}
769: 	else {
770: 		std::optional<int> heartcount = PreAddHeartsCallbacks(this, amount, 1 << 5, std::nullopt);
771: 		amount = heartcount.value_or(amount);
772: 		super(amount, retBool);
773: 	}
774: 	if (CallbackState.test(1010 - 1000)) {
775: 		PostAddHeartsCallbacks(this, amount, 1 << 5, std::nullopt);
776: 	}
777: }
778: 
779: HOOK_METHOD(Entity_Player, AddBoneHearts, (int amount) -> void) {	//bone
780: 	if (!CallbackState.test(1009 - 1000)) {
781: 		super(amount);
782: 	}
783: 	else {
784: 		std::optional<int> heartcount = PreAddHeartsCallbacks(this, amount, 1 << 6, std::nullopt);
785: 		amount = heartcount.value_or(amount);
786: 		super(amount);
787: 	}
788: 	if (CallbackState.test(1010 - 1000)) {
789: 		PostAddHeartsCallbacks(this, amount, 1 << 6, std::nullopt);

4874: 
4875: //MC_PRE/POST_PLAYER_REVIVE (1482)
4876: HOOK_METHOD(Entity_Player, Revive, () -> void) {
4877: 	const int precallbackid = 1481;
4878: 	if (CallbackState.test(precallbackid - 1000)) {
4879: 		lua_State* L = g_LuaEngine->_state;
4880: 		lua::LuaStackProtector protector(L);
4881: 
4882: 		lua_rawgeti(L, LUA_REGISTRYINDEX, g_LuaEngine->runCallbackRegistry->key);
4883: 
4884: 		Entity_Player* ent = (Entity_Player*)this;
4885: 
4886: 		lua::LuaResults lua_result = lua::LuaCaller(L).push(precallbackid)
4887: 			.push(this->GetPlayerType())
4888: 			.push(ent, lua::Metatables::ENTITY_PLAYER)
4889: 			.call(1);
4890: 
4891: 		if (!lua_result && lua_isboolean(L, -1) && lua_toboolean(L, -1) == false) {
4892: 			return;
4893: 		}
4894: 	}
4895: 
4896: 	super();
4897: 
4898: 	if (this->GetPlayerType() == PLAYER_THEFORGOTTEN) {
4899: 		// This function can leave the forgotten with half a soul heart, despite
4900: 		// it normally being impossible for the forgotten to have soul hearts.
4901: 		// (For vanilla revives the health is changed after Revive is called.)
4902: 		// This results in the half soul heart being impossible to remove, as
4903: 		// AddSoulHearts always redirects to The Soul.
4904: 		// Fix the forgotten's health here to revive them with an empty bone heart.
4905: 		this->_soulHearts = 0;

5482: inline bool IsCoopBaby(Entity_Player* player) {
5483: 	return player->_variant == 1 || player->_isCoopGhost;
5484: }
5485: 
5486: //PRE/POST_PLAYER_ADD_COSTUME (1281/1283)
5487: HOOK_METHOD(Entity_Player, AddCostume, (ItemConfig_Item* item, bool itemStateOnly) -> void) {

## repentogon/Patches/Anm2Extras.cpp

85: HOOK_METHOD(ANM2, destructor, ()->void) {
86: 	anm2_extras.erase(this);
87: 	super();
88: }
89: HOOK_METHOD(ANM2, Reset, ()->void) {
90: 	anm2_extras.erase(this);
91: 	super();
92: }
93: 
94: Anm2Extras* GetAnm2Extras(ANM2* anm2) {
