# tx-04-08 Hook manifest

Generated from `mod/binding/hooks.json` and `mod/binding/world_layout.json`. 安装后常驻；session 只控制逻辑启停。

待注册容量：79 / 96（既有 41 + world 38，distinct RVA 59）。

## 既有入口

| id | group | definition | RVA | priority | required nonzero |
|---|---|---|---|---|---|
| manager_update | loop | Manager::Update | 0x52e3e0 | -10000 | True |
| game_update | loop | Game::Update | 0x2f6d30 | 0 | True |
| post_update | loop | IsaacCS::PostUpdateCaller | 0x446c90 | 0 | True |
| game_exit | loop | Game::Exit | 0x2f6270 | 0 | True |
| new_room | loop | IsaacCS::NewRoomCaller | 0x44b930 | 0 | True |
| manager_render | loop | Manager::Render | 0x52ec40 | 0 | True |
| start_game | loop | Manager::ExecuteStartGame | 0x532a20 | 0 | True |
| post_start | loop | LuaEngine::PostGameStart | 0x44b660 | 0 | True |
| console_check | loop | IsaacCS::ConsoleGate | 0x287020 | 0 | True |
| room_update | loop | IsaacCS::RoomUpdate | 0x3f7f60 | 0 | True |
| entity_list | loop | IsaacCS::EntityListUpdate | 0x13f00 | 0 | True |
| restore | loop | Game::RestoreState | 0x2f43a0 | 0 | False |
| room_lifecycle | loop | IsaacCS::RoomLifecycleFSM | 0x509100 | 0 | False |
| door_update | loop | RoomTransition::Update | 0x426430 | 0 | False |
| door_nested | loop | IsaacCS::DoorNestedOwner | 0x2fa0f0 | 0 | False |
| odd_helper | loop | IsaacCS::OddHelper | 0x2f9340 | 0 | False |
| input_update | input | IsaacCS::InputManagerUpdate | 0x5edcc0 | 0 | True |
| device_update | input | IsaacCS::DeviceCommonUpdate | 0x5ec900 | 0 | True |
| player_update | player | Entity_Player::Update | 0x37bd40 | -10000 | True |
| player_native | player | Entity_Player::Update | 0x37bd40 | 10000 | True |
| move | player | IsaacCS::ConsumerMove | 0x372540 | 0 | True |
| bomb | player | IsaacCS::ConsumerBomb | 0x37a210 | 0 | False |
| active_item | player | Entity_Player::ControlActiveItem | 0x389e50 | 0 | False |
| card_pill | player | IsaacCS::ConsumerCardPill | 0x39bdd0 | 0 | False |
| shoot | player | IsaacCS::ConsumerShoot | 0x373990 | 0 | True |
| drop | player | IsaacCS::ConsumerDrop | 0x39c300 | 0 | True |
| join_pass | join | IsaacCS::JoinPass | 0x591360 | 0 | True |
| console_command | control | Console::RunCommand | 0x288100 | -10000 | True |
| pgd_unlock | loop | PersistentGameData::TryUnlock | 0x503930 | -10000 | False |
| pgd_counter | loop | PersistentGameData::IncreaseEventCounter | 0x503a50 | -10000 | False |
| pgd_local | loop | PersistentGameData::SaveLocally | 0x503460 | -10000 | False |
| pgd_cloud | loop | PersistentGameData::SaveToSteamCloud | 0x502f00 | -10000 | False |
| button_maps | loop | IsaacCS::SaveButtonMaps | 0x42ac90 | 0 | False |
| entity_kill | observe | IsaacCS::EntityKill | 0x2a7ed0 | -10000 | True |
| revive_ghost | observe | IsaacCS::ReviveGhost | 0x3cfc60 | -10000 | True |
| stage_transition | observe | IsaacCS::StartStageTransition | 0x2f9b30 | -10000 | True |
| load_game_state | observe | IsaacCS::LoadGameState | 0x5332f0 | -10000 | False |
| gamestate_write | observe | IsaacCS::write | 0x59f7a0 | -10000 | False |
| gamestate_read | observe | IsaacCS::read | 0x5a2240 | -10000 | False |
| gamestate_delete | observe | IsaacCS::Delete | 0x59e7b0 | -10000 | False |
| gamestate_delete_rerun | observe | IsaacCS::DeleteRerun | 0x59e870 | -10000 | False |

## world 观察入口

| observation | id | definition | RVA | priority | ABI | 完成含义 |
|---|---|---|---|---|---|---|
| Factory | world_factory | EntityFactory::Create | 0x2ec540 | -10000 | type:stack:1, force:stack:1 | Factory 返回非空只登记池取得/新 use；不证明物理构造、Init、Publish 或外层 owner 完成。 |
| Init | world_entity_init | Entity::Init | 0x2a4a20 | -10000 | type:stack:1, variant:stack:1, subtype:stack:1, initSeed:stack:1 | 基 Entity::Init 正常返回只证明基类这一层；七类派生 Init 与该次创建 owner 未闭合，重复 Init 不增加出生。 |
| Publish | world_publish | IsaacCS::PublishEntity | 0x13d00 | -10000 | entity:stack:1 | 容器发布观察；尾部 helper 的返回值语义 UNKNOWN，自有桥原样保存并透传 raw32，不当作 Entity*。 |
| Remove | world_entity_remove | Entity::Remove | 0x2a8000 | -10000 | (none) | 逻辑移除请求；不等于析构、清引用或成员退出，也不等于物理释放。 |
| ClearReferences | world_clear_references | Entity::ClearReferences | 0x2a8060 | -10000 | (none) | 引用清理观察；与 revision 更新分开记录，不因此断定所有别名已退出。 |
| Destroy | world_entity_destroy | Entity::destructor | 0x2a3ef0 | -10000 | (none) | 基类物理析构体，不承诺 operator delete；派生析构、slot 0 删除包装与分配器释放分列。 |
| PlayerCtor | world_player_ctor | Entity_Player::constructor | 0x34dc70 | -10000 | (none) | 构造 owner；进入或异常都不是完成，构造也不等于完整初始化。 |
| PlayerCreate | world_player_create | PlayerManager::SpawnCoPlayer2 | 0x590250 | -10000 | playerType:stack:1 | 普通 Player 创建 owner，含构造/Init/列表发布；不得再创建 B，也不把每次返回概括为普通 roster 发布。 |
| ManagerRestore | world_manager_restore | PlayerManager::RestoreGameState | 0x593a90 | -10000 | state:stack:1 | PlayerManager 的聚合恢复 owner，可恢复多名 Player 与保留关系；不是 Game::RestoreState，保留对象不自动成为 active roster。 |
| Replace | world_replace | IsaacCS::ReplacePlayer | 0x595e10 | -10000 | old_player:stack:1, replacement:stack:1 | 两侧 Player 分开记录；失败不是成功替换，旧对象退出 roster 不等于物理销毁。 |
| PlayerRestore | world_player_restore | Entity_Player::RestoreGameState | 0x3a5660 | -10000 | saveState:stack:1 | 单名 Player 的状态恢复；不补造构造/出生历史，也不恢复旧 witness。 |
| NpcMorph | world_npc_morph | Entity_NPC::Morph | 0x2bd0c0 | -10000 | EntityType:stack:1, Variant:stack:1, SubType:stack:1, Championid:stack:1 | Morph owner；相同参数可早退，回调可短路或改参；保留出生/use，更新分类与 revision，不建立新物理身份。 |
| PickupMorph | world_pickup_morph | Entity_Pickup::Morph | 0x2dff30 | -10000 | EntityType:stack:1, Variant:stack:1, SubType:stack:1, KeepPrice:stack:1, KeepSeed:stack:1, IgnoreModifiers:stack:1 | 内层 Init 返回时 Morph 未完成；本对象 RNG 可能被推进，保存/恢复 seed/Index/位置等，仍要等外层 owner 返回。 |
| GridFactory | world_grid_factory | Isaac::CreateGridEntity | 0x306100 | -10000 | Type:ecx, Index:edx | 工厂与 Init/Room 发布分开；既无此参数也无栈参，ECX/EDX 由官方 ArgData 表达。 |
| GridInit | world_grid_init | GridEntity::Init | 0x305070 | -10000 | Seed:stack:1 | 普通方法而非虚表派生入口；+3C 清零之后仍有虚调用 0x705085，返回后的最终值不由该 store 定论。 |
| GridSpawn | world_grid_spawn | Room::SpawnGridEntity | 0x3e21d0 | -10000 | GridIndex:stack:1, Type:stack:1, Variant:stack:1, Seed:stack:1, VarData:stack:1 | bool 返回；新指针在初始化完成前写入槽，失败不凭请求推断替换，返回后仍需确认实际槽。 |
| Door | world_make_door | Room::MakeDoor | 0x3e4de0 | -10000 | slot:stack:1 | 门 owner，无 Entity* 返回契约；同一指针可同时带 Grid/door membership，只算一个对象/use。 |
| GridFree | world_grid_free | Room::RemoveGridEntityImmediate | 0x413bd0 | -10000 | GridIndex:stack:1, PathTrail:stack:1, KeepDecoration:stack:1 | 移除 owner；越界、空槽或 type=1 且 KeepDecoration 可无事返回；请求不等于已释放，Grid 槽退出不证明 door 别名消失。 |
| Spawn | world_spawn | Game::Spawn | 0x2fa210 | -10000 | type:stack:1, variant:stack:1, position:stack:1, velocity:stack:1, spawner:stack:1, subtype:stack:1, seed:stack:1, unk:stack:1 | 完整八参 owner；Publish 之后仍有属性继承/引用/回调等后处理，只在外层实际返回后核对返回对象/use。 |

每项 world 观察另由同清单派生 `_native` 内层（priority=10000），相同定义/ABI、原调用一次；原生到达与正常返回均需同 call 配对。

## 布局

字段 78 项、来源 15 项、类别 10 项（schema: Effect, Familiar, Grid, NPC, Pickup, Player, Projectile, Tear）。

Pointer hooks: Keyboard/Gamepad slots 8, 9, 10, 35. Permanent.
OddNormal: 0x2F9559, expected FF 52 0C 6A 00, resume 0x2F955E. Permanent.
Host/K: join_mode_1 @ 0x5915ae, expected E8 ED FA FF FF, resume 0x5915b3. Permanent.
Host/K: join_mode_2 @ 0x59174c, expected E8 4F F9 FF FF, resume 0x591751. Permanent.
Host/K: join_mode_3 @ 0x591e67, expected E8 34 F2 FF FF, resume 0x591e6c. Permanent.
Host/K: join_create @ 0x591fe6, expected E8 65 E2 FF FF, resume 0x591feb. Permanent.
Host/K: k_register_notify @ 0x5eeb7c, expected 8B 77 14 8B 5B 08, resume 0x5eeb82. Permanent.
Host/K: k_remove_notify @ 0x5eebc3, expected 8B 73 20 3B 73 24, resume 0x5eebc9. Permanent.
Host/K: k_poll_notify @ 0x5edd2d, expected 8B 73 20 8B 41 08, resume 0x5edd33. Permanent.
Host/K: k_physical_disconnect @ 0x5ec37c, expected 8B 4D 08 8B 87 F8 03 00 00, resume 0x5ec385. Permanent.
Host/K: k_controller_list @ 0x4af970, expected 85 C0 0F 84 D5 00 00 00, resume 0x4af978. Permanent.
Host/K: k_keyconfig_draw @ 0x4b8d3d, expected 83 B8 8C 04 00 00 00, resume 0x4b8d44. Permanent.
Host/K: k_pause_disconnect @ 0x534768, expected 83 B8 8C 04 00 00 00, resume 0x53476f. Permanent.
