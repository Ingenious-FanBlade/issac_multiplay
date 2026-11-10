# 第7步 Level：原生字节单行账本

2026-11-10；[职责与证据](room-descriptor-synchronization-review.md)；[机器源](level-byte-ledger.json)。

**静态设计表，未进入生产生成链，无新layout号、构建或测试。** SDK／外部未知继续保留。

| 类型／视图 | 原生范围 | 直属UNKNOWN | 边界 |
|---|---|---|---|
| LevelGamePrefix | 0x18910 | 22B | game_prefix_view |
| RoomDescriptor | 0xB8 | 0B | native/field view |
| LevelRoomOffsetEntry | 0x4 | 0B | native/field view |
| RNG | 0x10 | 0B | native/field view |
| LevelTemporaryCurseField | 0x4 | 0B | native/field view |
| LevelASM | 0x4 | 0B | native/field view |
| LevelCustomStageNameABI | 0x18 | 0B | abi_model |

## LevelGamePrefix：`0x18910`字节

REPENTOGON libzhl/functions/Game.zhl；J273 6ED4B0/73EB10/7415E0/741E00

这是Game的Level连续视图，不是改写SDK sizeof(Level)。外部Room、RoomConfig和保存元素未在本轮完整展开；-19不采集。

| +0x0 `u32` `stage`【状态】当前楼层阶段（4B；已核）；L0.stage；证据：REPENTOGON libzhl/functions/Game.zhl；J273 6ED4B0/73EB10/7415E0/741E00；803E20/804500/97C300 | +0x4 `u32` `stage_type`【状态】当前全局楼层变体；挑战44逐房主题另取实际资源（4B；已核）；L0.stage_type；证据：REPENTOGON libzhl/functions/Game.zhl；J273 6ED4B0/73EB10/7415E0/741E00；749600/740290 | +0x8 `u8` `boss_challenge`【权威】挑战房生成／可开启性输入（1B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：844780；SDK Game._bossChallenge | +0x9 `u8` `devil_room_disabled`【权威】恶魔房生成禁止（1B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：Game.zhl；7415E0 byte+9；7DF710 | +0xA `bytes` `unknown_A_B`【未知】未获得语义或填充证明；清零不构成填充证据（2B；未知）；不读不发；保留UNKNOWN；证据：REPENTOGON libzhl/functions/Game.zhl；J273 6ED4B0/73EB10/7415E0/741E00 | +0xC `u32` `raw_curses`【换算】未合并temporary／强制／阻止的原始诅咒（4B；已核）；只发L0.effective_curses；不发原值；证据：742450；6F55C0/6F5750；966300/966680 | +0x10 `u8` `heart_picked`【权威】本层拾心玩法记录（1B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：74D680返回this+10；431660置1；7415E0 | +0x11 `u8` `can_see_everything`【权威】地图可见性生产输入（1B；已核）；不发原值；最终DisplayFlags／地图结果由L1/L2承接；证据：844790返回this+11；5B3460写；原版Wiki Level | +0x12 `bytes` `unknown_12_13`【未知】未获得语义或填充证明；清零不构成填充证据（2B；未知）；不读不发；保留UNKNOWN；证据：REPENTOGON libzhl/functions/Game.zhl；J273 6ED4B0/73EB10/7415E0/741E00 | +0x14 `RoomDescriptor[507]` `grid_rooms`【容器】普通物理描述存储；不能等同507个活动房间（93288B；已核）；L1按活动身份投影；资源／地图结果另块；禁止整块复制；证据：Game.zhl；6ED4B0构造527×B8；7332F0；7415E0 | +0x16C7C `RoomDescriptor[18]` `off_grid_minus1_through_minus18`【容器】离图源槽-1至-18；ListIndex可为负（3312B；已核）；L1实际活动／被引用描述，源槽有符号，不能用ListIndex寻址；证据：RoomDescriptor.zhl；732E90；73AD10；GridRooms Wiki | +0x1796C `excluded RoomDescriptor` `off_grid_minus19_deathmatch`【本地】原版Deathmatch存储槽525；C/S隔离（184B；已核）；不采不发不安装，不复用原版Online对象；证据：GridRooms Wiki -19；现行规范；tx-04-18-foundation.md | +0x17A24 `RoomDescriptor` `off_grid_minus20_lil_portal`【容器】Lil Portal槽526；reset(false)保留其特殊寿命（184B；已核）；L1实际所需投影；不触发reset／新建房间；证据：732E90 i=526例外；GridRooms Wiki | +0x17ADC `i32[3][169]` `room_offset`【容器】每维169个物理描述源槽；包含官方旧Level两个错误别名的位置（2028B；已核）；L3固定507×u16，FFFF为空；不按ListIndex重建；证据：606D10 imul 2A4/add 17ADC；7332F0；7415E0拷贝7EC | +0x182C8 `i32*` `current_room_offset`【派生】当前维度映射的本地别名（4B；已核）；不发地址；Client建立指向自身数组的别名；证据：606D1A；739F02/739FD8；9635EC | +0x182CC `u32` `ordinary_room_count`【派生】有序普通房间成员数量（4B；已核）；结构成员集合承接，不重复发裸计数；证据：Game.zhl；7332F0；73AF40；7415E0；9635B0 | +0x182D0 `i32` `starting_room_index`【权威】楼层生成与返回起点（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：Game.zhl；73EB10置84；7415E0 | +0x182D4 `RNG` `generation_rng`【权威】楼层生成游标／shift（16B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：Game.zhl；73EB10；73AF60；Random核对 | +0x182E4 `RNG` `devil_angel_room_rng`【权威】恶魔／天使房生成RNG，独立于generation_rng（16B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：74D6F0返回+182E4；743970消费；random-synchronization-review.md | +0x182F4 `f32` `angel_room_chance_modifier`【权威】AddAngelRoomChance累积量；不是最终StatHUD概率（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：7DF730返回；659800累加；7EF590消费 | +0x182F8 `opaque local holder[8]` `native_interop_holder`【本地】本地互操作holder，具体C++类名未核（8B；已核）；不传两个字／地址；不猜Lua或通信ABI；证据：6ED4B0初始化；732C50/6EFB09经vtable+0C与本地callback退休 | +0x18300 `Room*` `current_room`【引用】当前world的原生Room owner（4B；已核）；结构引用，不传地址；完整Room布局属于本步另一任务；证据：Game.zhl；732D20；本地Room owner | +0x18304 `i32` `current_entry_index`【状态】进入房间的象限索引，允许特殊负房间（4B；已核）；L0.current_entry_index；证据：Game.zhl；73AF00；73AD10；Level Wiki | +0x18308 `i32` `previous_room_index`【权威】前一房间的跨房控制状态（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：739DE0；7415E0/741E00；744060 | +0x1830C `i32` `current_dimension`【换算】当前维度0..2；不是查询默认-1（4B；已核）；L0.dimension为u8；实际维表别名本地派生；证据：Game.zhl；739DE0；606D10；803E20/804500 | +0x18310 `i32` `previous_dimension`【权威】前一房间维度（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：739DE0；7415E0；73AF20 | +0x18314 `i32` `last_boss_room_list_index`【换算】Boss选择身份；Greed HUD比较当前描述ListIndex（4B；已核）；只发L0.current_is_last_boss真实匹配，所有模式保留；不能在非Greed清零；证据：97D0EA；97C300；733D25；本轮补核803313/815B80/748900普通TrapDoor Light | +0x18318 `i32` `leave_door`【权威】离开门／过房FSM输入（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：739DE0；74B530；Level Wiki LeaveDoor | +0x1831C `i32` `enter_door`【权威】进入门／过房与原生创建控制（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：739DE0；815430/7EE350/79EA20；Level Wiki EnterDoor | +0x18320 `Vector` `dungeon_return_position`【权威】地牢返回坐标（8B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：57B270；844760；744060；7415E0 | +0x18328 `i32` `dungeon_return_room_index`【权威】地牢返回房间（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：57B2A0；744060；7415E0 | +0x1832C `u32` `dungeon_placement_seed`【权威】房间放置／随机生成输入（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：606D00；73EB10；LuaLevel.cpp GetRoomPlacementSeed | +0x18330 `i32` `extra_boss_list_index`【权威】Void等Boss选择保存（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：733D2B与lastBoss同赋值；6494AD消费 | +0x18334 `i32` `greed_wave`【状态】当前Greed波次；不是波次seed数组（4B；已核）；L0.greed_wave；非Greed规范0且不读残留；证据：97D371格式化%d/%d；74D700 setter；Game.zhl | +0x18338 `u32[12]` `greed_wave_seeds`【权威】十二个波次seed（48B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：73DEB8起循环12次生成；7415E0/741E00复制三组16B | +0x18368 `i32` `greed_wave_progress`【权威】波次生产／清理进度；不作为当前wave显示（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：7E5D60/7F7420/7FBDD0；7415E0对wave取较小值 | +0x1836C `i32` `greed_wave_spawn_control`【权威】Greed波次生成控制缓存（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：80BF98比较；80C18C写；7415E0 | +0x18370 `i32` `greed_treasure_room_index`【换算】Greed宝物房图标分支输入（4B；已核）；只发每描述L1.greed_treasure_icon；不适用规范false；证据：96437B比较GridIndex，选择IconTreasureRoomGreed | +0x18374 `i32` `greed_undamaged_wave_streak`【权威】真实连续无红心伤害波次；官方简化Level错误读18204（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：7455D7清零；80BE88读取；80BEAB递增；LuaLevel.cpp:52-60 | +0x18378 `i32` `portal0_return_room`【权威】第一组portal返回房间（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：744250三字stride；739F53；7415E0 | +0x1837C `i32` `portal0_return_argument`【权威】第一组portal返回参数；不替代实际Entity状态（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：744287；433779读取 | +0x18380 `u8` `portal0_return_flag`【权威】第一组portal返回条件字节（1B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：744291；7415E0保存复制不证明其后三字节填充 | +0x18381 `bytes` `unknown_18381_18383`【未知】未获得语义或填充证明；清零不构成填充证据（3B；未知）；不读不发；保留UNKNOWN；证据：REPENTOGON libzhl/functions/Game.zhl；J273 6ED4B0/73EB10/7415E0/741E00 | +0x18384 `i32` `portal1_return_room`【权威】第二组portal返回房间（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：744250 stride0C；739F49；7415E0 | +0x18388 `i32` `portal1_return_argument`【权威】第二组portal返回参数（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：744250 n2=1；7415E0 | +0x1838C `u8` `portal1_return_flag`【权威】第二组portal返回条件字节（1B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：744250 n2=1 byte；7415E0整字复制不补语义 | +0x1838D `bytes` `unknown_1838D_1838F`【未知】未获得语义或填充证明；清零不构成填充证据（3B；未知）；不读不发；保留UNKNOWN；证据：REPENTOGON libzhl/functions/Game.zhl；J273 6ED4B0/73EB10/7415E0/741E00 | +0x18390 `i32` `item_random_attempt_count`【权威】随机阈值的本层累加输入，未强加具体物品名（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：6FD5F0读取并加24h比较随机100；6FD701递增 | +0x18394 `Vector` `mama_mega_world_origin`【权威】Mama Mega传播的世界起点（8B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：745A30按格子/离图位置计算后写18394/18398；745AC7置flag | +0x1839C `u32` `level_state_flags`【权威】真实24位楼层玩法标志；1822C是错误SDK别名（4B；已核）；不发全flags；DisplayFlags／地图／门／音画承接结果；证据：4F3CA0；4242A0；745AC7；LevelStateFlag Wiki | +0x183A0 `u8` `initializing_level`【权威】Level::Init中为1、结束置0的生命周期标记（1B；已核）；不发；采集见初始化中必须报Gap；证据：73EB10置1/0；749633与745641分支 | +0x183A1 `bytes` `unknown_183A1_183A3`【未知】未获得语义或填充证明；清零不构成填充证据（3B；未知）；不读不发；保留UNKNOWN；证据：REPENTOGON libzhl/functions/Game.zhl；J273 6ED4B0/73EB10/7415E0/741E00 | +0x183A4 `std::vector<EntitySaveState>` `myosotis_pickups`【权威】下层搬运的拾取保存队列（12B；已核）；不发头/元素；Host创建的真实Pickup由Entity域承接；证据：Game.zhl；LuaLevel.cpp GetMyosotisPickups；73EB10清队列 | +0x183B0 `f32[169]` `challenge44_theme_field0`【权威】挑战44主题插值图第一场（676B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；实际Room主题资源与名称仍必需；证据：749790生成；749600查表；740290选名 | +0x18654 `f32[169]` `challenge44_theme_field1`【权威】挑战44主题插值图第二场（676B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；实际Room主题资源与名称仍必需；证据：749790生成；749600查表；740290选名 | +0x188F8 `u8` `floor_spawn_latch`【权威】本层一次性玩法生成锁存；具体功能名未定（1B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：611AF1检查；611C8E置1；732DE5清零 | +0x188F9 `bytes` `unknown_188F9_188FB`【未知】未获得语义或填充证明；清零不构成填充证据（3B；未知）；不读不发；保留UNKNOWN；证据：REPENTOGON libzhl/functions/Game.zhl；J273 6ED4B0/73EB10/7415E0/741E00 | +0x188FC `i32` `death_certificate_return_door`【权威】Death Certificate返回门（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：74B59F保存18318；739F15读取；7415E0 | +0x18900 `i32` `death_certificate_return_room`【权威】Death Certificate返回当前房间（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：74B565保存18304；739ED0；6F7874 | +0x18904 `i8` `death_certificate_return_dimension`【权威】Death Certificate返回当前维度（1B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：74B582；739EE8 movsx byte；6F7866 | +0x18905 `bytes` `unknown_18905_18907`【未知】未获得语义或填充证明；清零不构成填充证据（3B；未知）；不读不发；保留UNKNOWN；证据：REPENTOGON libzhl/functions/Game.zhl；J273 6ED4B0/73EB10/7415E0/741E00 | +0x18908 `i32` `death_certificate_previous_room`【权威】Death Certificate保存的先前房间（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：74B571保存18308；739EDC | +0x1890C `i8` `death_certificate_previous_dimension`【权威】Death Certificate保存的先前维度；超出SDK sizeof(Level)但仍在Game前缀内（1B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：6ED4B0；74B591保存18310；739F08读取 | +0x1890D `bytes` `unknown_1890D_1890F`【未知】未获语义或填充证明（3B；未知）；不读不发；保留UNKNOWN；证据：REPENTOGON libzhl/functions/Game.zhl；J273 6ED4B0/73EB10/7415E0/741E00 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

## RoomDescriptor：`0xB8`字节

RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；本轮补证809640/80A2E0/80ACA0/80B5A0/801BE0/803E20/ASMHud.cpp

184B直属范围已登记，原94..AB的24B改为商店类别/补货次数，直属未归属字节为0；GreedSubType业务名仍依SDK，Flags高位语义和外部配置/保存体未知不因此消失。

| +0x0 `i32` `grid_index`【状态】描述身份字段；物理source-slot与原始ListIndex分开（4B；已核）；L1.grid_index:i32；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；73AD10；Wiki RoomDescriptor | +0x4 `i32` `safe_grid_index`【状态】描述身份字段；物理source-slot与原始ListIndex分开（4B；已核）；L1.safe_grid_index:i32；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；73AD10；Wiki RoomDescriptor | +0x8 `i32` `list_index`【换算】描述身份字段；物理source-slot与原始ListIndex分开（4B；已核）；原值不发；物理source-slot独立；L0.current_is_last_boss在所有模式保留真实匹配；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；73AD10；Wiki RoomDescriptor；815B80；801BE0在803313调用，748900普通路线可true而Greed恒false | +0xC `i32` `dimension`【换算】描述身份字段；物理source-slot与原始ListIndex分开（4B；已核）；L1.dimension:u8，仅0..2；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；73AD10；Wiki RoomDescriptor | +0x10 `RoomConfig_Room*` `data`【资源】独立可空的已选配置；不发送原生指针（4B；已核）；L1.data:u16配置引用，FFFF空；配置资源表待正式注册；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；原版Wiki OverrideData；tx-04-18-foundation.md | +0x14 `RoomConfig_Room*` `override_data`【资源】独立可空的已选配置；不发送原生指针（4B；已核）；L1.override_data:u16配置引用，FFFF空；配置资源表待正式注册；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；原版Wiki OverrideData；tx-04-18-foundation.md | +0x18 `u32` `allowed_doors`【换算】当前允许门槽mask；与Doors[]和实际门存在性分开（4B；已核）；L1.allowed_doors:u8低8位；证据：RoomDescriptor.zhl；LuaRoomDescriptor.cpp修复AllowedDoors；9635B0 Void框线 | +0x1C `i32[8]` `doors`【状态】八个门槽的结构目标；允许无实门的有效目标（32B；已核）；L1.doors:8×i32，保留-100/-101等特殊值；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；REPENTOGON Wiki Doors | +0x3C `u32` `display_flags`【状态】最终地图可见性／阴影／图标标志（4B；已核）；L1.display_flags:u32；不执行可见性生产器；证据：9638BC；96430F；9644EC；Level Wiki UpdateVisibility | +0x40 `i32` `visited_count`【换算】累计访问次数；显示只判断非零（4B；已核）；L1.visited:u8(value!=0)，原累计留Host；证据：96447E/9644E6；RoomDescriptor Wiki VisitedCount | +0x44 `u32` `flags`【换算】Flags混合玩法输入和实际显示条件；高位业务语义未穷尽（4B；已核）；L1.display_bits:u8，原bit0/10/11/18/5→线上bit0/1/2/3/4，其余零；bit2/9的最终lighting/pass、bit6实际音乐及其它结果由具名owner承接；不发全flags；证据：964037；963EF5；964392；97C300；grid-pressure-door-evidence.md Door Light Flags&40000；801BE0 Pit读取20h；804500特殊坑重绘；803E20献祭bit2/pitch-black；8134C0 | +0x48 `u16 storage (SDK i16)` `total_money_spent`【权威】花费累积，原生按无符号40阈值判断；不是Client支付命令（2B；已核）；留Host；不重演支付/解锁，实际价格由Pickup承接；证据：SDK int16_t；7CF3EF..7CF3FC Game→Room→descriptor加16位；7CF400 unsigned cmp 28h；7CF415→903930解锁 | +0x4A `i16` `clear_count`【权威】累计清理计数；当前clear另由bit投影（2B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；7E5DF2/7FBDF4的16位递增及后续movzx | +0x4C `i32` `greed_subtype`【权威】Greed生成／特殊房间内容输入；私有语义依SDK（4B；SDK声明）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；733010/6F4F20确证本4B保存搬移；具体业务名仍SDK声明 | +0x50 `i16` `poop_count`【权威】保存／生成计数（2B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0 | +0x52 `i16` `tainted_keeper_coin_spawns`【权威】重进房防刷硬币计数（2B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；REPENTOGON Wiki GetTaintedKeeperCoinSpawns | +0x54 `i32` `pits_count`【权威】生成坑洞计数；实际Grid/水面另承接（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0 | +0x58 `u32` `decoration_seed`【权威】背景／装饰生成输入；challenge32地图选择输入（4B；已核）；原值留Host；实际背景由Room资源承接，challenge32已选地图标记另提供；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；9635B0 challenge32；Wiki DecorationSeed | +0x5C `u32` `spawn_seed`【换算】实体出生／Error饰品效果选择输入（4B；已核）；原seed及完整Error效果ID留Host；D1只发u8布尔error_is_modeling_clay供最终颜色adapter；效果>0在目标版恒真；实际Image仍由Presentation承接；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；Wiki SpawnSeed；官方GetErrorTrinketEffect说明；IsaacRepentance_static.cpp:588；NUM_TRINKETS=190；ASMHud.cpp:62/207/223 | +0x60 `u32` `award_seed`【权威】清理奖励和商店生成输入（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；Wiki AwardSeed | +0x64 `u32` `boss_death_seed`【权威】Boss死亡生成输入（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0 | +0x68 `std::vector<GridEntityDesc>` `saved_grid_entities`【容器】离房Grid保存；固定初始448×20h分配（12B；已核）；保存头/完整元素不发；有效type18只设置L2.icon_mask.bit14，固定帧0不发；证据：6EBB10分配3823h/对齐32B；964B6D..964BA2扫描type18 | +0x74 `std::vector<EntitySaveState>` `saved_entities`【容器】离房实体保存；78h元素有地图消费者（12B；已核）；完整保存体/类型/variant/subtype/顺序不发；Host纯聚合L2图标mask及必要选中帧，缺席1B/存在3..12B；证据：6EBB10按30 DWORD步长reserve；9635B0读type/variant/subtype/+14/+28/+2C | +0x80 `std::set<int>` `restricted_grid_indexes`【权威】房间生成限制；不是本轮地图消费者（8B；已核）；不读不发元素/树头；不要求为同步实现树遍历；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；官方LuaRoomDescriptor.cpp | +0x88 `std::vector<EntitySaveState>` `saved_effects`【权威】离房效果保存，当前Effect独立投影（12B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；6EBB10第二个78h元素vector | +0x94 `i16[8]` `shop_item_generation_kinds`【权威】八个ShopItemID槽的生成类别，不是最终Pickup subtype；类别完整业务名未强行命名（16B；已核）；全部留Host；只下发实际Pickup/当前Price/价格Sprite；不在Client补货或定价；证据：809640/809F4B初始化写；80A2E0按word+2*slot读取后选variant/池；80A8EF/80B5DC价格分支 | +0xA4 `u8[8]` `shop_restock_counts`【权威】八槽饱和补货次数；ctor不初始化本8B，商店初始化才清零；当前价格由次数参与产生（8B；已核）；全部留Host；只下发实际Pickup/当前Price/价格Sprite；不在Client补货或定价；证据：809689清8B；80AD15..80AD22在FF前递增；80B6FD movzx并三角增价/封顶99 | +0xAC `i16` `shop_item_index`【权威】商店生成槽位（2B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；Wiki ShopItemIdx | +0xAE `i16` `shop_discount_index`【权威】商店折扣生成槽位（2B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；Wiki ShopItemDiscountIdx | +0xB0 `i32` `delirium_distance`【权威】Void生成／Boss选择距离（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；Wiki DeliriumDistance | +0xB4 `i32` `group`【权威】传送随机选择的分组（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；REPENTOGON Wiki GetGroup |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

## LevelRoomOffsetEntry：`0x4`字节

Game+17ADC，三个169项视图

官方Level._greedwavesclearedwithoutredheartdamage=+18204实际是dimension2/grid120；_levelStateFlag=+1822C实际是dimension2/grid130。

| +0x0 `i32` `physical_descriptor_slot`【换算】-1或0..526物理源槽；不是自由ListIndex（4B；已核）；L3 u16；FFFF空；槽525 Deathmatch不得引用；证据：606D10；7332F0；7415E0 |
| --- |

## RNG：`0x10`字节

RNG.zhl；Random静态核对

仅Level生成实例的职责；不覆盖两个纯绘制实例的特殊合同。

| +0x0 `u32` `seed`【权威】当前随机游标（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RNG.zhl；random-synchronization-review.md | +0x4 `u32` `shift1`【权威】第一右移（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RNG.zhl；random-synchronization-review.md | +0x8 `u32` `shift2`【权威】左移（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RNG.zhl；random-synchronization-review.md | +0xC `u32` `shift3`【权威】最后右移（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RNG.zhl；random-synchronization-review.md |
| --- | --- | --- | --- |

## LevelTemporaryCurseField：`0x4`字节

Game+25AC8的4B字段视图，不是新增原生对象



| +0x0 `u32` `temporary_curses`【换算】Game+25AC8独立运行时诅咒输入（4B；已核）；仅参与Host有效诅咒投影，不重复发送此原值；证据：742450；6F55C0/6F5750；run-synchronization-review.md |
| --- |

## LevelASM：`0x4`字节

官方Level.h：int ForceSpecialQuest；Win32四字节

没有核定官方DLL全局RVA；不按Game偏移采集。

| +0x0 `i32` `force_special_quest`【权威】官方对象外生成参数，level init前重置（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：repentogon/LuaInterfaces/Level.h与LuaLevel.cpp；ASMLevel.cpp |
| --- |

## LevelCustomStageNameABI：`0x18`字节

官方LuaLevel.cpp静态std::string CustomStageName

24B是MSVC32模型，实际官方DLL地址和ABI未独立核定；不能据此读取。

| +0x0 `MSVC string storage` `sso_or_data`【本地】小字符串或本地堆指针；仅声明ABI模型（16B；ABI模型）；不读模型偏移；实际已选文字由Presentation承接；证据：LuaLevel.cpp:43/146；MSVC32 string模型 | +0x10 `u32` `size`【本地】模型中的逻辑长度（4B；ABI模型）；不发string对象；证据：同上 | +0x14 `u32` `capacity`【本地】模型中的容量／SSO选择（4B；ABI模型）；不发地址／容量；证据：同上 |
| --- | --- | --- |

## 设计线表

不包括记录头／结构身份／配置资源及其它显示上下文；原生地址／保存头不上线。

### L0_LevelContext：22

| +0x0 `u32` `stage`：当前阶段（4B） | +0x4 `u32` `stage_type`：当前全局楼层变体（4B） | +0x8 `u32` `effective_curses`：Host最终组合诅咒（4B） | +0xC `i32` `current_entry_index`：入口象限／特殊负房间（4B） | +0x10 `u8` `dimension`：已安装维度0..2（1B） | +0x11 `i32` `greed_wave`：Greed当前波次；非Greed规范0（4B） | +0x15 `u8` `current_is_last_boss`：当前ListIndex匹配lastBoss；所有模式真实值，供Greed HUD及普通路线TrapDoor Light（1B） |
| --- | --- | --- | --- | --- | --- | --- |

### L1_RoomDescriptorDisplay：53

| +0x0 `i32` `grid_index`：描述原始GridIndex（4B） | +0x4 `i32` `safe_grid_index`：描述原始SafeGridIndex（4B） | +0x8 `u8` `dimension`：描述维度0..2（1B） | +0x9 `u16` `data`：基础配置引用，FFFF为空（2B） | +0xB `u16` `override_data`：替换配置独立引用，FFFF为空（2B） | +0xD `u8` `allowed_doors`：低八门槽mask（1B） | +0xE `i32[8]` `doors`：八个结构目标，含特殊路由值（32B） | +0x2E `u32` `display_flags`：最终地图显示标志（4B） | +0x32 `u8` `visited`：VisitedCount!=0（1B） | +0x33 `u8` `display_bits`：clear/red_room/devil_treasure/blue_redirect/has_water五位，其余零（1B） | +0x34 `u8` `greed_treasure_icon`：Greed特殊宝物房图标最终开关，不适用为false（1B） |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

### L3_LevelOccupancy：1014

| +0x0 `u16[3][169]` `physical_descriptor_slots`：源槽0..526或FFFF；不允许525/未提供/不活动描述（1014B） |
| --- |

### D1_CurrentDescriptorVisual：1

| +0x0 `u8 bool` `error_is_modeling_clay`：仅当前必要颜色owner的Error→ModelingClay条件；当前world一份、随值变化更新；原效果ID不发、不伪造数值Getter；已传完整最终颜色时不重复发送（1B） |
| --- |

### L2_FinalSavedMapIcons：present=0:1B; present=1:3+popcount(icon_mask & 0x2B9B)B，3..12B

| +0x0 `u8` `present`：未显示/不适用为0；来源失败为Gap（1B） | +0x1 `u16 if present` `icon_mask`：16类图标完整聚合，含bit14楼梯/bit15 Portal；不含房型图标（2B） | +0x3 `u8[] if present` `selected_frames`：仅存在的动态类别，按bit升序；固定帧0不发（popcount(mask & 0x2B9B)B） |
| --- | --- | --- |

| mask bit | 图标类别 | 最大帧 | 帧是否发送 |
|---|---|---|---|
| 0 | heart | 9 | 是，仅该bit存在时 |
| 1 | coin | 3 | 是，仅该bit存在时 |
| 2 | golden_bomb | 0 | 否，固定0 |
| 3 | key | 2 | 是，仅该bit存在时 |
| 4 | bomb | 1 | 是，仅该bit存在时 |
| 5 | card | 0 | 否，固定0 |
| 6 | collectible | 0 | 否，固定0 |
| 7 | pill | 1 | 是，仅该bit存在时 |
| 8 | battery | 1 | 是，仅该bit存在时 |
| 9 | chest | 9 | 是，仅该bit存在时 |
| 10 | trinket | 0 | 否，固定0 |
| 11 | slot_machine | 8 | 是，仅该bit存在时 |
| 12 | rune | 0 | 否，固定0 |
| 13 | beggar | 5 | 是，仅该bit存在时 |
| 14 | stairs | 0 | 否，固定0 |
| 15 | portal | 0 | 否，固定0 |

Host纯聚合规则：

- `predicate`：type5 intStorage3!=0或targetPosition==(0,0)排除；按原生短路读取；无关保存字段不读作状态
- `heart`：subtype2→0,3→5,4→9,6→6,7→8,8→3,10→4,11→7,12→2,default→1；同类取max
- `coin`：subtype2/6→1,3→2,7→3,default→0；同类取max
- `key`：subtype2→1,4→2,default→0；同类取max
- `bomb`：subtype4→1,其它0；同类取max
- `pill`：(subtype&0x7FF)==14→1,其它0；同类取max
- `battery`：subtype4→1,其它0；同类取max
- `card_rune`：variant300：实际Host固定CardConfig有效且type==2→rune，否则card；只读正常配置，不抽卡
- `chest`：6EAA70认可50/60/360/51/53/52/54/390/55/56/57/58，另认可69；51→1,52→0,53→7,56→6,57→9,60→8,69→(subtype==2)+2,360→5,其它认可值→4；同类取max
- `fixed_pickup_categories`：variant42→golden_bomb，100→collectible，350→trinket，均固定帧0；其它未采用variant不产生图标
- `slot`：type6 variant14排除；2→machine3,3→machine4,4/6→beggar0,5/15→beggar2,7→beggar3,8/11→machine1,9→beggar4,10→machine6,12→machine0,13→beggar5,16→machine5,17→machine7,18→beggar1,19→machine8,default→machine2；同类取max
- `portal`：type1000/variant161/subtype980→bit15，重复幂等
- `stairs`：SavedGrid有效元素任一type18→bit14，重复幂等
- `count_order`：保存图标个数由popcount(mask)给出；原房型图标另计；保留绘制上限筛选前的聚合，不复制Host本地显示前缀

Level根未知22B；RoomDescriptor原24B已核为Host两张商店表、直属0B；保存体通用74/type5视图70/Grid3B及SDK／外部未知仍保留。L1五位flags同53B，Boss匹配所有模式，D1当前必要颜色谓词1B。
