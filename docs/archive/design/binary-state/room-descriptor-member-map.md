# 第7步 RoomDescriptor：44条公开成员回映射

2026-11-10；[专门核对](room-descriptor-synchronization-review.md)；[机器源](room-descriptor-member-map.json)。

原始ordinal零基，原ownership／参数／重载／field／wire_name在JSON保留；不改写原目录，不表示运行Getter兼容。

决定数量：authority=24，converted=9，delegated=3，derived=1，field=4，local=1，resource=2。

| stream ordinal | 应用目录ordinal | ID | 原参数 | 旧归属 | 本轮决定 | 目标 | 理由 |
|---|---|---|---|---|---|---|---|
| 3090 | 470 | RoomDescriptor.AddRestrictedGridIndex | " int GridIndex " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 3091 | 471 | RoomDescriptor.AllowedDoors | null | ReplicatedState | converted | L1.allowed_doors | 官方修复为有效低八位mask；Void地图框线消费；不同于实际门存在性 |
| 3092 | 472 | RoomDescriptor.AwardSeed | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3093 | 473 | RoomDescriptor.BossDeathSeed | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3094 | 474 | RoomDescriptor.ChallengeDone | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3095 | 475 | RoomDescriptor.Clear | null | ReplicatedState | converted | L1.display_bits.bit0 | 当前clear位为地图/Greed HUD消费；不因此生成清理奖励 |
| 3096 | 476 | RoomDescriptor.ClearCount | null | ReplicatedState | authority | @HostRoomHistory | 累计计数不发送；当前clear由display_bits承接 |
| 3097 | 477 | RoomDescriptor.CreateGroup | " string groupName " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 3098 | 478 | RoomDescriptor.Data | null | ReplicatedState | resource | L1.data + @RoomConfigurationResource | 已选基础配置，独立nullable u16资源；实际配置表待注册 |
| 3099 | 479 | RoomDescriptor.DecorationSeed | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3100 | 480 | RoomDescriptor.DeliriumDistance | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3101 | 481 | RoomDescriptor.DisplayFlags | null | ReplicatedState | field | L1.display_flags | 最终地图显示值；不执行原生可见性生产器 |
| 3102 | 482 | RoomDescriptor.Doors | null | ReplicatedState | field | L1.doors | 八个i32结构目标，保留特殊路由；目标存在不等于实际门存在 |
| 3103 | 483 | RoomDescriptor.Flags | null | ReplicatedState | converted | L1.display_bits | 五位原生0/10/11/18/5→线上0..4；sacrifice/pitch-black最终lighting及音乐/背景另承接；不回放全flags |
| 3104 | 484 | RoomDescriptor.GetDecoSaveState | " " | ReplicatedState | authority | @HostRoomSave + @Entity.CurrentEffects | 离房效果保存不复制，当前显示Effect由Entity域承接；无已核地图消费者 |
| 3105 | 485 | RoomDescriptor.GetDimension | " " | ReplicatedState | converted | L1.dimension | 原生i32核0..2后发送u8 |
| 3106 | 486 | RoomDescriptor.GetEntitiesSaveState | " " | ReplicatedState | delegated | L2.icon_mask/selected_frames | Host过滤后按原生OR/max聚合；不发保存数量/顺序/type/variant/subtype；存在时仅3..12B，保留Slot default和全部选中帧 |
| 3107 | 487 | RoomDescriptor.GetErrorTrinketEffect | " " | ReplicatedState | converted | D1.error_is_modeling_clay + @Presentation.ActualImage/ColorContext; numeric result HostOnly | 官方效果范围恒为1..189（零seed为75），>0无需发；实际Image已另传，仅需1B是否等于ModelingClay供颜色adapter；不替代数值Getter或推进RNG |
| 3108 | 488 | RoomDescriptor.GetGridEntitiesSaveState | " " | ReplicatedState | delegated | L2.icon_mask.bit14 | 20h有效元素任一type18只设置楼梯bit；固定帧0不发，不复制保存体/头/seed |
| 3109 | 489 | RoomDescriptor.GetGroup | "" | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3110 | 490 | RoomDescriptor.GetGroupByName | " string groupName " | NotReplicated | local | @OfficialLocalGroupRegistry | 官方进程内分组名称注册表；分组用于Host传送抽选，不复制注册表或私有ID |
| 3111 | 491 | RoomDescriptor.GetNeighboringRooms | " " | ReplicatedState | derived | @DescriptorTopology | 从同world已安装配置形状、三维占用和源槽关系作具名纯结构查询；不是重跑放房/连门 |
| 3112 | 492 | RoomDescriptor.GetRestrictedGridIndexes | " " | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3113 | 493 | RoomDescriptor.GetTaintedKeeperCoinSpawns | " " | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3114 | 494 | RoomDescriptor.GetValidNeighborPlacementLocations | " RoomConfigRoom RoomConfig, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false " | NotReplicated | authority | @HostRoomPlacement | 玩法放房候选查询，不是Client拓扑所需字段；不运行放房判断补源 |
| 3115 | 495 | RoomDescriptor.GetValidNeighborPlacementLocations | " RoomShape RoomShape = 1, DoorMask DoorMask = -1, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false " | NotReplicated | authority | @HostRoomPlacement | 玩法放房候选查询，不是Client拓扑所需字段；不运行放房判断补源 |
| 3116 | 496 | RoomDescriptor.GridIndex | null | ReplicatedState | field | L1.grid_index | 描述的原始GridIndex，允许L型缺口，不能充当入口象限 |
| 3117 | 497 | RoomDescriptor.HasWater | null | ReplicatedState | converted | L1.display_bits.bit4 + @Room.FinalWater | 801BE0的Pit及804500 Backdrop32重绘直接检查原bit5；不能只用waterAmount/水色/WaterClip代替 |
| 3118 | 498 | RoomDescriptor.InitSeeds | " RNG RNG " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 3119 | 499 | RoomDescriptor.ListIndex | null | ReplicatedState | converted | @PhysicalSourceSlot + L0.current_is_last_boss | 不以可变ListIndex作存储下标；物理源槽独立，L0匹配bool供所有模式的HUD/TrapDoor Light |
| 3120 | 500 | RoomDescriptor.NoReward | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3121 | 501 | RoomDescriptor.OverrideData | null | ReplicatedState | resource | L1.override_data + @RoomConfigurationResource | 与基础配置独立；不合并成单个Type/Variant或随机重选 |
| 3122 | 502 | RoomDescriptor.PitsCount | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3123 | 503 | RoomDescriptor.PoopCount | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3124 | 504 | RoomDescriptor.PressurePlatesTriggered | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3125 | 505 | RoomDescriptor.SacrificeDone | null | ReplicatedState | delegated | @Room.FinalLighting/Pass | 原bit2在803E20影响献祭房最终shadow fill；不发原位，Room实际lighting必须承接，不归为无显示历史 |
| 3126 | 506 | RoomDescriptor.SafeGridIndex | null | ReplicatedState | field | L1.safe_grid_index | 原始安全格子索引；不能代替完整占用映射 |
| 3127 | 507 | RoomDescriptor.SetGroup | " int group " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 3128 | 508 | RoomDescriptor.SetTaintedKeeperCoinSpawns | " int Num " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 3129 | 509 | RoomDescriptor.ShopItemDiscountIdx | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3130 | 510 | RoomDescriptor.ShopItemIdx | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3131 | 511 | RoomDescriptor.SpawnSeed | null | ReplicatedState | converted | D1.error_is_modeling_clay; raw seed/effect ID HostOnly | 出生生产seed及完整效果ID留Host；从合法当前seed按官方纯整数式求是否ModelingClay，D1仅1B；实际图像另承接，不在离房描述重复 |
| 3132 | 512 | RoomDescriptor.SurpriseMiniboss | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3133 | 513 | RoomDescriptor.VisitedCount | null | ReplicatedState | converted | L1.visited | 地图只检查非零，累计历史留Host |

正式来源、资源和显示adapter待接线；当前生产版本56不接收本轮草稿。官方Greed Getter源码18204与真实18374的冲突仍见Level核对，不要求对照相等。
