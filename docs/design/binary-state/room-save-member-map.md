# 第7步 RoomSave：原始成员逐行回映射

2026-11-10；全部98条原始`source_schema=isaac.cs.room_save`记录。职责／API差异／生命周期与证据见[核对说明](room-save-exploration-synchronization-review.md)，完整来源与实际API类型见[机器表](room-save-member-map.json)。

source_ordinal为stream-catalog的0-based全局行号；原continuity行号单列。参数按原JSON显示，保留null／空字符串／空格，重载不合并；旧归属保留用于说明修订。未新增线字段、生产owner或运行接线，版本仍56。

| 全局行／原行／field | 原成员与参数 | 原类型／旧归属 | 当前决定／去向 | 理由 |
|---|---|---|---|---|
| 0 / 0 / 1 | EntitiesSaveState.GetB1 params=" " | boolean / ReplicatedState | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复 |
| 1 / 1 / 2 | EntitiesSaveState.GetB2 params=" " | boolean / ReplicatedState | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复 |
| 2 / 2 / 3 | EntitiesSaveState.GetDropSeed params=" " | int / ReplicatedState | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复 |
| 3 / 3 / 4 | EntitiesSaveState.GetF1 params=" " | float / ReplicatedState | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复 |
| 4 / 4 / 5 | EntitiesSaveState.GetF2 params=" " | float / ReplicatedState | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复 |
| 5 / 5 / 6 | EntitiesSaveState.GetGridSpawnIdx params=" " | int / ReplicatedState | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；ESSM Getter在hijacked时读旁表原值，raw+20/+30是marker／内部ID；不等价Getter，不作为C/S身份 |
| 6 / 6 / 7 | EntitiesSaveState.GetI1 params=" " | int / ReplicatedState | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复 |
| 7 / 7 / 8 | EntitiesSaveState.GetI2 params=" " | int / ReplicatedState | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；Wiki明确type5的主动充能／价格／ShopItemId，J273 7E51D0 type5写臂补证；不外推其它type，不改通用UNKNOWN74B |
| 8 / 8 / 9 | EntitiesSaveState.GetI3 params=" " | int / ReplicatedState | converted → L2.icon_mask/selected_frames; raw saved payload HostOnly | Host只读相关地图type／variant／subtype／价格排除／位置谓词，OR/max后只发最终图标；其它保存用途留Host；Wiki明确type5的主动充能／价格／ShopItemId，J273 7E51D0 type5写臂补证；不外推其它type，不改通用UNKNOWN74B |
| 9 / 9 / 10 | EntitiesSaveState.GetI4 params=" " | int / ReplicatedState | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；Wiki明确type5的主动充能／价格／ShopItemId，J273 7E51D0 type5写臂补证；不外推其它type，不改通用UNKNOWN74B |
| 10 / 10 / 11 | EntitiesSaveState.GetI5 params=" " | int / ReplicatedState | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复 |
| 11 / 11 / 12 | EntitiesSaveState.GetI6 params=" " | int / ReplicatedState | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复 |
| 12 / 12 / 13 | EntitiesSaveState.GetI7 params=" " | int / ReplicatedState | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；ESSM Getter在hijacked时读旁表原值，raw+20/+30是marker／内部ID；不等价Getter，不作为C/S身份 |
| 13 / 13 / 14 | EntitiesSaveState.GetI8 params=" " | int / ReplicatedState | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复 |
| 14 / 14 / 15 | EntitiesSaveState.GetInitSeed params=" " | int / ReplicatedState | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复 |
| 15 / 15 / 16 | EntitiesSaveState.GetPos params=" " | Vector / ReplicatedState | converted → L2.icon_mask/selected_frames; raw saved payload HostOnly | Host只读相关地图type／variant／subtype／价格排除／位置谓词，OR/max后只发最终图标；其它保存用途留Host |
| 16 / 16 / 17 | EntitiesSaveState.GetSpawnerType params=" " | EntityType / ReplicatedState | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复 |
| 17 / 17 / 18 | EntitiesSaveState.GetSpawnerVariant params=" " | int / ReplicatedState | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复 |
| 18 / 18 / 19 | EntitiesSaveState.GetSubType params=" " | int / ReplicatedState | converted → L2.icon_mask/selected_frames; raw saved payload HostOnly | Host只读相关地图type／variant／subtype／价格排除／位置谓词，OR/max后只发最终图标；其它保存用途留Host |
| 19 / 19 / 20 | EntitiesSaveState.GetType params=" " | EntityType / ReplicatedState | converted → L2.icon_mask/selected_frames; raw saved payload HostOnly | Host只读相关地图type／variant／subtype／价格排除／位置谓词，OR/max后只发最终图标；其它保存用途留Host |
| 20 / 20 / 21 | EntitiesSaveState.GetU1 params=" " | int / ReplicatedState | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；实际API：type==5: boolean; otherwise signed int8 (Lua integer) |
| 21 / 21 / 22 | EntitiesSaveState.GetVariant params=" " | int / ReplicatedState | converted → L2.icon_mask/selected_frames; raw saved payload HostOnly | Host只读相关地图type／variant／subtype／价格排除／位置谓词，OR/max后只发最终图标；其它保存用途留Host |
| 22 / 22 / 23 | EntitiesSaveState.SetB1 params=" boolean B1 " | void / AuthorityOnly | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值 |
| 23 / 23 / 24 | EntitiesSaveState.SetB2 params=" boolean B2 " | void / AuthorityOnly | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值 |
| 24 / 24 / 25 | EntitiesSaveState.SetF1 params=" float Num " | void / AuthorityOnly | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值 |
| 25 / 25 / 26 | EntitiesSaveState.SetF2 params=" float Num " | void / AuthorityOnly | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值 |
| 26 / 26 / 27 | EntitiesSaveState.SetI1 params=" int Num " | void / AuthorityOnly | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值 |
| 27 / 27 / 28 | EntitiesSaveState.SetI2 params=" int Num " | void / AuthorityOnly | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；Wiki明确type5的主动充能／价格／ShopItemId，J273 7E51D0 type5写臂补证；不外推其它type，不改通用UNKNOWN74B |
| 28 / 28 / 29 | EntitiesSaveState.SetI3 params=" int Num " | void / AuthorityOnly | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；Wiki明确type5的主动充能／价格／ShopItemId，J273 7E51D0 type5写臂补证；不外推其它type，不改通用UNKNOWN74B |
| 29 / 29 / 30 | EntitiesSaveState.SetI4 params=" int Num " | void / AuthorityOnly | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；Wiki明确type5的主动充能／价格／ShopItemId，J273 7E51D0 type5写臂补证；不外推其它type，不改通用UNKNOWN74B |
| 30 / 30 / 31 | EntitiesSaveState.SetI5 params=" int Num " | void / AuthorityOnly | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值 |
| 31 / 31 / 32 | EntitiesSaveState.SetI6 params=" int Num " | void / AuthorityOnly | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值 |
| 32 / 32 / 33 | EntitiesSaveState.SetI7 params=" int Num " | void / AuthorityOnly | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；ESSM Getter在hijacked时读旁表原值，raw+20/+30是marker／内部ID；不等价Getter，不作为C/S身份 |
| 33 / 33 / 34 | EntitiesSaveState.SetI8 params=" int Num " | void / AuthorityOnly | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值 |
| 34 / 34 / 35 | EntitiesSaveState.SetPos params=" Vector Pos " | void / AuthorityOnly | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值 |
| 35 / 35 / 36 | EntitiesSaveState.SetSubType params=" int SubType " | void / AuthorityOnly | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值 |
| 36 / 36 / 37 | EntitiesSaveState.SetType params=" EntityType Type " | void / AuthorityOnly | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值 |
| 37 / 37 / 38 | EntitiesSaveState.SetU1 params=" int Num " | void / AuthorityOnly | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；实际API：type==5: boolean; otherwise signed int8 (Lua integer) |
| 38 / 38 / 39 | EntitiesSaveState.SetVariant params=" int Variant " | void / AuthorityOnly | authority → @HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值 |
| 39 / 39 / 40 | EntitiesSaveStateVector.Clear params=" " | void / AuthorityOnly | authority → @HostSavedContainer | 先ESSM清理batch再vector.clear，有生命周期副作用，禁止为采集／Client安装执行 |
| 40 / 40 / 41 | EntitiesSaveStateVector.Get params=" int Index " | EntitiesSaveState / ReplicatedState | converted → L2.icon_mask/selected_frames | 正式来源仅扫描有效元素中地图必要投影并聚合，不调用Lua、不发送Get结果对象或保存数组；Get为0-based |
| 41 / 41 / 42 | EntitiesSaveStateVector.GetByType params=" EntityType Type, int Variant = 0, int SubType = 0 " | EntitiesSaveState / ReplicatedState | converted → L2.icon_mask/selected_frames | 正式来源仅扫描有效元素中地图必要投影并聚合，不调用Lua、不发送Get结果对象或保存数组；Get为0-based；实际API：Lua table (1-based matched result array), not single saved object |
| 42 / 42 / 43 | EntitiesSaveStateVector.__len params=" " | int / AuthorityOnly | authority → @HostSavedContainer | 容器头／长度／保存项成员关系属于Host恢复，不独立下行；读取不得产生状态或跨窗口借用；Getter本身只读；留Host是职责决定，不表示调用它会修改状态；Grid Clear不改变len |
| 621 / 621 / 622 | GridEntitiesSaveStateVector.Get params=" int Index " | GridEntityDesc / ReplicatedState | converted → L2.icon_mask/selected_frames | 正式来源仅扫描有效元素中地图必要投影并聚合，不调用Lua、不发送Get结果对象或保存数组；Get为0-based |
| 622 / 622 / 623 | GridEntitiesSaveStateVector.GetByType params=" GridEntityType Type " | GridEntityDesc / ReplicatedState | converted → L2.icon_mask/selected_frames | 正式来源仅扫描有效元素中地图必要投影并聚合，不调用Lua、不发送Get结果对象或保存数组；Get为0-based；实际API：Lua table (1-based matched result array), not single saved object |
| 623 / 623 / 624 | GridEntitiesSaveStateVector.__len params=" " | int / AuthorityOnly | authority → @HostSavedContainer | 容器头／长度／保存项成员关系属于Host恢复，不独立下行；读取不得产生状态或跨窗口借用；Getter本身只读；留Host是职责决定，不表示调用它会修改状态；Grid Clear不改变len |
| 624 / 624 / 625 | GridEntityDesc.Initialized params=null | boolean / ReplicatedState | authority → @HostSavedGrid -> @CurrentGrid | 这是保存容器中的字段，不沿用活动Grid描述的线负载；回访后的实际Grid由第6步承接 |
| 625 / 625 / 626 | GridEntityDesc.SpawnCount params=null | int / ReplicatedState | authority → @HostSavedGrid -> @CurrentGrid | 这是保存容器中的字段，不沿用活动Grid描述的线负载；回访后的实际Grid由第6步承接 |
| 626 / 626 / 627 | GridEntityDesc.SpawnSeed params=null | int / ReplicatedState | authority → @HostSavedGrid -> @CurrentGrid | 这是保存容器中的字段，不沿用活动Grid描述的线负载；回访后的实际Grid由第6步承接 |
| 627 / 627 / 628 | GridEntityDesc.State params=null | int / ReplicatedState | authority → @HostSavedGrid -> @CurrentGrid | 这是保存容器中的字段，不沿用活动Grid描述的线负载；回访后的实际Grid由第6步承接 |
| 628 / 628 / 629 | GridEntityDesc.Type params=null | GridEntityType / ReplicatedState | converted → L2.icon_mask.bit14 | 保存槽任一Type18只生成楼梯存在位，不发完整20h保存体 |
| 629 / 629 / 630 | GridEntityDesc.VarData params=null | int / ReplicatedState | authority → @HostSavedGrid -> @CurrentGrid | 这是保存容器中的字段，不沿用活动Grid描述的线负载；回访后的实际Grid由第6步承接 |
| 630 / 630 / 631 | GridEntityDesc.VariableSeed params=null | int / ReplicatedState | authority → @HostSavedGrid -> @CurrentGrid | 这是保存容器中的字段，不沿用活动Grid描述的线负载；回访后的实际Grid由第6步承接 |
| 631 / 631 / 632 | GridEntityDesc.Variant params=null | int / ReplicatedState | authority → @HostSavedGrid -> @CurrentGrid | 这是保存容器中的字段，不沿用活动Grid描述的线负载；回访后的实际Grid由第6步承接 |
| 763 / 763 / 764 | RoomDescriptor.AddRestrictedGridIndex params=" int GridIndex " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 764 / 764 / 765 | RoomDescriptor.AllowedDoors params=null | DoorSet / ReplicatedState | converted → L1.allowed_doors | 官方修复为有效低八位mask；Void地图框线消费；不同于实际门存在性 |
| 765 / 765 / 766 | RoomDescriptor.AwardSeed params=null | int / ReplicatedState | authority → @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 766 / 766 / 767 | RoomDescriptor.BossDeathSeed params=null | int / ReplicatedState | authority → @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 767 / 767 / 768 | RoomDescriptor.ChallengeDone params=null | boolean / ReplicatedState | authority → @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 768 / 768 / 769 | RoomDescriptor.Clear params=null | boolean / ReplicatedState | converted → L1.display_bits.bit0 | 当前clear位为地图/Greed HUD消费；不因此生成清理奖励 |
| 769 / 769 / 770 | RoomDescriptor.ClearCount params=null | int / ReplicatedState | authority → @HostRoomHistory | 累计计数不发送；当前clear由display_bits承接 |
| 770 / 770 / 771 | RoomDescriptor.CreateGroup params=" string groupName " | int / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 771 / 771 / 772 | RoomDescriptor.Data params=null | RoomConfigRoom / ReplicatedState | resource → L1.data + @RoomConfigurationResource | 已选基础配置，独立nullable u16资源；实际配置表待注册 |
| 772 / 772 / 773 | RoomDescriptor.DecorationSeed params=null | int / ReplicatedState | authority → @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 773 / 773 / 774 | RoomDescriptor.DeliriumDistance params=null | int / ReplicatedState | authority → @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 774 / 774 / 775 | RoomDescriptor.DisplayFlags params=null | int / ReplicatedState | field → L1.display_flags | 最终地图显示值；不执行原生可见性生产器 |
| 775 / 775 / 776 | RoomDescriptor.Doors params=null | int[] / ReplicatedState | field → L1.doors | 八个i32结构目标，保留特殊路由；目标存在不等于实际门存在 |
| 776 / 776 / 777 | RoomDescriptor.Flags params=null | RoomDescriptor / ReplicatedState | converted → L1.display_bits | 五位原生0/10/11/18/5→线上0..4；sacrifice/pitch-black最终lighting及音乐/背景另承接；不回放全flags |
| 777 / 777 / 778 | RoomDescriptor.GetDecoSaveState params=" " | EntitiesSaveStateVector / ReplicatedState | authority → @HostRoomSave + @Entity.CurrentEffects | 离房效果保存不复制，当前显示Effect由Entity域承接；无已核地图消费者 |
| 778 / 778 / 779 | RoomDescriptor.GetDimension params=" " | Dimension / ReplicatedState | converted → L1.dimension | 原生i32核0..2后发送u8 |
| 779 / 779 / 780 | RoomDescriptor.GetEntitiesSaveState params=" " | EntitiesSaveStateVector / ReplicatedState | delegated → L2.icon_mask/selected_frames | Host过滤后按原生OR/max聚合；不发保存数量/顺序/type/variant/subtype；存在时仅3..12B，保留Slot default和全部选中帧 |
| 780 / 780 / 781 | RoomDescriptor.GetErrorTrinketEffect params=" " | TrinketType / ReplicatedState | converted → D1.error_is_modeling_clay + @Presentation.ActualImage/ColorContext; numeric result HostOnly | 官方效果范围恒为1..189（零seed为75），>0无需发；实际Image已另传，仅需1B是否等于ModelingClay供颜色adapter；不替代数值Getter或推进RNG |
| 781 / 781 / 782 | RoomDescriptor.GetGridEntitiesSaveState params=" " | GridEntitiesSaveStateVector / ReplicatedState | delegated → L2.icon_mask.bit14 | 20h有效元素任一type18只设置楼梯bit；固定帧0不发，不复制保存体/头/seed |
| 782 / 782 / 783 | RoomDescriptor.GetGroup params="" | int / ReplicatedState | authority → @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 783 / 783 / 784 | RoomDescriptor.GetGroupByName params=" string groupName " | int? / NotReplicated | local → @OfficialLocalGroupRegistry | 官方进程内分组名称注册表；分组用于Host传送抽选，不复制注册表或私有ID |
| 784 / 784 / 785 | RoomDescriptor.GetNeighboringRooms params=" " | table / ReplicatedState | derived → @DescriptorTopology | 从同world已安装配置形状、三维占用和源槽关系作具名纯结构查询；不是重跑放房/连门 |
| 785 / 785 / 786 | RoomDescriptor.GetRestrictedGridIndexes params=" " | int[] / ReplicatedState | authority → @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 786 / 786 / 787 | RoomDescriptor.GetTaintedKeeperCoinSpawns params=" " | int / ReplicatedState | authority → @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 787 / 787 / 788 | RoomDescriptor.GetValidNeighborPlacementLocations params=" RoomConfigRoom RoomConfig, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false " | int[] / NotReplicated | authority → @HostRoomPlacement | 玩法放房候选查询，不是Client拓扑所需字段；不运行放房判断补源 |
| 788 / 788 / 789 | RoomDescriptor.GetValidNeighborPlacementLocations params=" RoomShape RoomShape = 1, DoorMask DoorMask = -1, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false " | int[] / NotReplicated | authority → @HostRoomPlacement | 玩法放房候选查询，不是Client拓扑所需字段；不运行放房判断补源 |
| 789 / 789 / 790 | RoomDescriptor.GridIndex params=null | int / ReplicatedState | field → L1.grid_index | 描述的原始GridIndex，允许L型缺口，不能充当入口象限 |
| 790 / 790 / 791 | RoomDescriptor.HasWater params=null | boolean / ReplicatedState | converted → L1.display_bits.bit4 + @Room.FinalWater | 801BE0的Pit及804500 Backdrop32重绘直接检查原bit5；不能只用waterAmount/水色/WaterClip代替 |
| 791 / 791 / 792 | RoomDescriptor.InitSeeds params=" RNG RNG " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 792 / 792 / 793 | RoomDescriptor.ListIndex params=null | int / ReplicatedState | converted → @PhysicalSourceSlot + L0.current_is_last_boss | 不以可变ListIndex作存储下标；物理源槽独立，L0匹配bool供所有模式的HUD/TrapDoor Light |
| 793 / 793 / 794 | RoomDescriptor.NoReward params=null | boolean / ReplicatedState | authority → @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 794 / 794 / 795 | RoomDescriptor.OverrideData params=null | RoomConfigRoom / ReplicatedState | resource → L1.override_data + @RoomConfigurationResource | 与基础配置独立；不合并成单个Type/Variant或随机重选 |
| 795 / 795 / 796 | RoomDescriptor.PitsCount params=null | int / ReplicatedState | authority → @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 796 / 796 / 797 | RoomDescriptor.PoopCount params=null | int / ReplicatedState | authority → @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 797 / 797 / 798 | RoomDescriptor.PressurePlatesTriggered params=null | boolean / ReplicatedState | authority → @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 798 / 798 / 799 | RoomDescriptor.SacrificeDone params=null | boolean / ReplicatedState | delegated → @Room.FinalLighting/Pass | 原bit2在803E20影响献祭房最终shadow fill；不发原位，Room实际lighting必须承接，不归为无显示历史 |
| 799 / 799 / 800 | RoomDescriptor.SafeGridIndex params=null | int / ReplicatedState | field → L1.safe_grid_index | 原始安全格子索引；不能代替完整占用映射 |
| 800 / 800 / 801 | RoomDescriptor.SetGroup params=" int group " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 801 / 801 / 802 | RoomDescriptor.SetTaintedKeeperCoinSpawns params=" int Num " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 802 / 802 / 803 | RoomDescriptor.ShopItemDiscountIdx params=null | int / ReplicatedState | authority → @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 803 / 803 / 804 | RoomDescriptor.ShopItemIdx params=null | int / ReplicatedState | authority → @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 804 / 804 / 805 | RoomDescriptor.SpawnSeed params=null | int / ReplicatedState | converted → D1.error_is_modeling_clay; raw seed/effect ID HostOnly | 出生生产seed及完整效果ID留Host；从合法当前seed按官方纯整数式求是否ModelingClay，D1仅1B；实际图像另承接，不在离房描述重复 |
| 805 / 805 / 806 | RoomDescriptor.SurpriseMiniboss params=null | boolean / ReplicatedState | authority → @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 806 / 806 / 807 | RoomDescriptor.VisitedCount params=null | int / ReplicatedState | converted → L1.visited | 地图只检查非零，累计历史留Host |

补充项不计入上述原始行数：

| 补充范围 | 决定与原因 |
|---|---|
| GridEntitiesSaveStateVector.Clear | authority：源码已注册、Wiki／原98行未列；逐槽Type=0，vector长度保持；仅Host执行 |
| Room.SaveState/RestoreState/save_entity/restore_entity | authority：保存／恢复与生产命令；回调仅阶段，不证明新修订；禁止Client恢复或读取时补调用 |
| EntityPlayer.GetMovingBoxContents + Level.GetMyosotisPickups + recursive Flip/minecart saves | authority：不额外复制保存域；Host实际恢复后的Entity／Grid／身体关系承接 |
| ESSM marker/id + EntitySaveStateManager/EntitySaveStateEx | local：官方保存owner／可回收ID／递归所有权本地；业务当前结果另由既有域承接，Mod数据表迁出当前范围 |
| callbacks 1300..1308 | lifecycle：失效／真实阶段，不是网络保存RPC或公开备份容器；PRE_LOAD在super前，PRE_SAVE可能slot=-1 |

直接归属计数：authority=68；converted=19；resource=2；field=4；delegated=3；local=1；derived=1。同一结果有多个API入口时复用唯一owner，不按行数增加负载。
