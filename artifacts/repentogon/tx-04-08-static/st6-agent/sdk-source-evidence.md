# ST6 原始静态证据索引

任务：tx-04-08 ST6。证据性质：目标 IDB 静态反编译/指令，或 SDK/工程当前源码；不含运行时观察。目标为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，imagebase `0x400000`，J273。此索引没有哈希、构建、运行或执行任何目标函数。

## IDA 原始导出

本目录下 `ida-functions.json` 保存 15 个目标函数的 IDA 反编译伪代码和完整指令列表（含 VA/RVA）；`ida-registration-contexts.json` 保存 `Game:Spawn` 与 `Isaac.Spawn` Lua 注册附近指令；`ida-lua-spawn-wrappers.json` 保存两个 Lua Spawn wrapper 和相关 helper；`ida-restore-replace-callers.json` 保存 Restore、rerun、Replace owner/caller 上下文。记录中均保留 target address 与 instruction address，可按函数地址回 IDB 复核。

关键目标函数（VA）：

| 函数 | 地址 | 导出键 |
| --- | --- | --- |
| Game::Spawn | `0x6FA210` | `game_spawn` |
| EntityFactory::Create | `0x6EC540` | `entity_factory_create` |
| Entity::Init | `0x6A4A20` | `entity_init` |
| EntityList::Publish | `0x413D00` | `entity_list_publish` |
| PlayerManager::SpawnCoPlayer2 | `0x990250` | `player_create` |
| PlayerManager::RestoreGameState | `0x993A90` | `player_manager_restore` |
| PlayerManager::ReplacePlayer | `0x995E10` | `player_manager_replace` |
| Entity_Player::RestoreGameState | `0x7A5660` | `player_restore` |
| Entity_NPC::Morph | `0x6BD0C0` | `npc_morph` |
| Entity_Pickup::Morph | `0x6DFF30` | `pickup_morph` |
| GridEntity factory | `0x706100` | `grid_factory` |
| GridEntity::Init | `0x705070` | `grid_init` |
| Room::SpawnGridEntity | `0x7E21D0` | `grid_spawn` |
| Room::MakeDoor | `0x7E4DE0` | `make_door` |
| Room::RemoveGridEntityImmediate | `0x813BD0` | `grid_remove` |

Restore/Replace 调用上下文另见 `ida-restore-replace-callers.json`：Game::RestoreState `0x6F43A0`、Rerun owner `0x6F3650`、另一个 PlayerManager::ReplacePlayer caller `0x7A3A40` 与 `0x5B4A60`。不把这些 owner 的入口地址误作 Lua API。

早期导出偏差说明：曾生成本目录下的 ida-core-owner-functions.json，但其中 pseudocode 因把 IDA simpleline_t 直接转字符串而成为占位/非有效内容。该错误文件已删除，不应引用。最终应复核的是 ida-functions.json：重新按 ida_lines.tag_remove(line.line) 取伪代码，并保留每个目标函数的原始指令清单；其余所列 IDA 文件分别保存 registration、Lua wrapper、Restore/Replace caller 上下文。没有恢复无效占位文件。

## SDK / 当前源码定位

以下源文件仅用于核对官方 SDK/当前源码提供的签名、Lua 路由和 hook 行为，不能替代目标二进制事实或运行时观察。

- `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Game.zhl:29`：`Game::Spawn(type, variant, position, velocity, spawner, subtype, seed, unk)`。
- `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerManager.zhl:5,57,60`：`SpawnCoPlayer2(playerType)`、`RestoreGameState(GameState*)`、`RestoreGameState_PostLevelInit(GameState*)`。
- `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntityNPC.zhl:2`：NPC Morph 四个参数；`EntityPickup.zhl:2`：Pickup Morph 六个参数。
- `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Room.zhl:32-38,92`：Grid spawn、descriptor spawn、immediate remove 与 MakeDoor 签名。
- `F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/Room/LuaRoom.cpp:33-63,533-534`：`Room:SpawnGridEntity` 与 `Room:RemoveGridEntityImmediate` 注册及参数解析。整数 overload 的 Lua 参数为 `(GridIndex, Type, Variant=0, Seed=GridIndex+frameCount+1, VarData=0)`；该 wrapper 检查 `0..447` 并返回 bool。Remove wrapper 解析 `(GridIndex, PathTrail, KeepDecoration)`，没有返回值。
- `F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/LuaPlayerManager.cpp:162-170,397` 与 `docs/docs/PlayerManager.md:117-118`：Lua `PlayerManager:SpawnCoPlayer2(playerType)` 确实可达并返回 Player userdata 或 nil。
- 目标 IDB Morph Lua 绑定原始材料：父审阅目录 artifacts/repentogon/tx-04-08-static/st6-st8-review/main-morph-registration.json、main-morph-adapters.json、main-morph-closures.json。NPC：注册 0x85180E/0x851813 → adapter 0x86D190 → closure 0x897480，L2..L5 整数，native 四参，Lua 返回 0 值。Pickup：注册 0x850FB3/0x850FB8 → adapter 0x86B040 → closure 0x896C30，L2..L4 整数、L5..L7 boolean，native 六参，Lua 返回 0 值。
- 目标 IDB seed helper 原始材料：父审阅目录 artifacts/repentogon/tx-04-08-static/st6-st8-review/main-spawn-seed-helper.json 保存 0x6EB4B0 的伪代码/指令，通常路径读写全局 dword_BE8720 状态数组与 n625 索引，另有 trace list 与 CaptureStackBackTrace；byte_BE72FF 非零路径调用 rand。该静态分支说明不替代运行时分支观察。
- `F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/CustomCallbacks.cpp:1192-1241,1266-1318`：NPC/Pickup Morph 的 Hook 链；False 可早退，table 返回可改参再调用 native，native 后才触发 post callback。
- `F:/projects_isaac/issac_multiplay/mod/src/game/join.cpp:60-112,114-182,197-245`：当前 C/S Host Join owner 限定在已有 A/有效 session 与原生 Join `ProcessInput` 的 Manager owner 上，控制 Join role，并在 native 返回后验证实际 roster 与 B，再 bind ordinal 1。
- `F:/projects_isaac/issac_multiplay/tests/e2e-mods/isaac-cs-test/README.md:34-42` 与 `main.lua:249,514-...`：现有夹具动作仅 `skip, room, kill_b, revive_b, stage, room`；当前没有 Spawn/Morph/Grid/Replace Lua action。
- `F:/projects_isaac/issac_multiplay/docs/design/tx-04-08/tx-04-08-e2e-plan.md:19-31,37-52`：S4/S5 的新生命周期动作仍是设计范围，要求 ST1–ST7 核准实际 API/参数/副作用；未知项需保留未覆盖，静态 reachability 不预先授权实机动作。

## 部分源码原文摘录

`LuaRoom.cpp`（行号为仓库当前行）：

```cpp
33 LUA_FUNCTION(Lua_SpawnGridEntity) {
36 int GridIndex = (int)luaL_checkinteger(L, 2);
37 if (GridIndex < 0 || GridIndex > 447) { ... }
46 int Type = (int)luaL_checkinteger(L, 3);
47 int Variant = (int)luaL_optinteger(L, 4, 0);
48 unsigned int Seed = (unsigned int)luaL_optinteger(L, 5, GridIndex + g_Game->_frameCount + 1);
49 int VarData = (int)luaL_optinteger(L, 6, 0);
50 ret = room->SpawnGridEntity(GridIndex, Type, Variant, Seed, VarData);
52 lua_pushboolean(L, ret);
56 LUA_FUNCTION(Lua_RemoveGridEntityImmediate) {
58 int GridIndex = (int)luaL_checkinteger(L, 2);
59 int PathTrail = (int)luaL_checkinteger(L, 3);
60 bool KeepDecoration = lua_toboolean(L, 4);
61 room->RemoveGridEntityImmediate(GridIndex, PathTrail, KeepDecoration);
```

`CustomCallbacks.cpp` 原文的关键分支：NPC Morph 1192 起、Pickup Morph 1266 起。`tablesize==4/3`（NPC）或 `6/3`（Pickup）分支先保存旧 type/variant/subtype，再 `super(...)` 后调用 `Post*Morph(...)`；Lua Boolean false 分支在 1230-1232 / 1306-1309 直接返回。一般分支亦是 native original 后才调用 post callback。Post callback 参数传递的是缓存的旧 type/variant/subtype。

`isaac-cs-test/README.md` 的当前动作表逐行列出：skip 30 个 `MC_PRE_UPDATE`；room 取已开启未锁普通相邻门并调用 `Game:StartRoomTransition`；kill_b 对已确认正常 B 调一次 `B:Kill()`；revive_b 只对已确认 ghost 调一次 `B:ReviveCoopGhost()`；stage 调 `Game():StartStageTransition(false, 0, nil)`。对应 Lua 调用见 `main.lua:115,621,656,687`。调用返回与房间/阶段/重绑完成是不同观察。
