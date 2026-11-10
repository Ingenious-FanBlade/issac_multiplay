# 第7步 RoomDescriptor：原生字节单行账本

2026-11-10；[职责与证据](room-descriptor-synchronization-review.md)；[机器源](room-descriptor-byte-ledger.json)。

**静态设计表，未进入生产生成链，无新layout号、构建或测试。** SDK／外部未知继续保留。

| 类型／视图 | 原生范围 | 直属UNKNOWN | 边界 |
|---|---|---|---|
| RoomDescriptor | 0xB8 | 0B | native/field view |
| RoomDescriptorSavedPickupMapInput | 0x78 | 70B | type5_consumer_view |
| RoomDescriptorSavedGridMapInput | 0x20 | 3B | consumer_view |
| RoomDescriptorShopKinds | 0x10 | 0B | native/field view |
| RoomDescriptorRestockCounts | 0x8 | 0B | native/field view |
| RoomDescriptorRestrictedSetHeader | 0x8 | 0B | native/field view |

## RoomDescriptor：`0xB8`字节

RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；本轮补证809640/80A2E0/80ACA0/80B5A0/801BE0/803E20/ASMHud.cpp

184B直属范围已登记，原94..AB的24B改为商店类别/补货次数，直属未归属字节为0；GreedSubType业务名仍依SDK，Flags高位语义和外部配置/保存体未知不因此消失。

| +0x0 `i32` `grid_index`【状态】描述身份字段；物理source-slot与原始ListIndex分开（4B；已核）；L1.grid_index:i32；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；73AD10；Wiki RoomDescriptor | +0x4 `i32` `safe_grid_index`【状态】描述身份字段；物理source-slot与原始ListIndex分开（4B；已核）；L1.safe_grid_index:i32；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；73AD10；Wiki RoomDescriptor | +0x8 `i32` `list_index`【换算】描述身份字段；物理source-slot与原始ListIndex分开（4B；已核）；原值不发；物理source-slot独立；L0.current_is_last_boss在所有模式保留真实匹配；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；73AD10；Wiki RoomDescriptor；815B80；801BE0在803313调用，748900普通路线可true而Greed恒false | +0xC `i32` `dimension`【换算】描述身份字段；物理source-slot与原始ListIndex分开（4B；已核）；L1.dimension:u8，仅0..2；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；73AD10；Wiki RoomDescriptor | +0x10 `RoomConfig_Room*` `data`【资源】独立可空的已选配置；不发送原生指针（4B；已核）；L1.data:u16配置引用，FFFF空；配置资源表待正式注册；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；原版Wiki OverrideData；tx-04-18-foundation.md | +0x14 `RoomConfig_Room*` `override_data`【资源】独立可空的已选配置；不发送原生指针（4B；已核）；L1.override_data:u16配置引用，FFFF空；配置资源表待正式注册；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；原版Wiki OverrideData；tx-04-18-foundation.md | +0x18 `u32` `allowed_doors`【换算】当前允许门槽mask；与Doors[]和实际门存在性分开（4B；已核）；L1.allowed_doors:u8低8位；证据：RoomDescriptor.zhl；LuaRoomDescriptor.cpp修复AllowedDoors；9635B0 Void框线 | +0x1C `i32[8]` `doors`【状态】八个门槽的结构目标；允许无实门的有效目标（32B；已核）；L1.doors:8×i32，保留-100/-101等特殊值；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；REPENTOGON Wiki Doors | +0x3C `u32` `display_flags`【状态】最终地图可见性／阴影／图标标志（4B；已核）；L1.display_flags:u32；不执行可见性生产器；证据：9638BC；96430F；9644EC；Level Wiki UpdateVisibility | +0x40 `i32` `visited_count`【换算】累计访问次数；显示只判断非零（4B；已核）；L1.visited:u8(value!=0)，原累计留Host；证据：96447E/9644E6；RoomDescriptor Wiki VisitedCount | +0x44 `u32` `flags`【换算】Flags混合玩法输入和实际显示条件；高位业务语义未穷尽（4B；已核）；L1.display_bits:u8，原bit0/10/11/18/5→线上bit0/1/2/3/4，其余零；bit2/9的最终lighting/pass、bit6实际音乐及其它结果由具名owner承接；不发全flags；证据：964037；963EF5；964392；97C300；grid-pressure-door-evidence.md Door Light Flags&40000；801BE0 Pit读取20h；804500特殊坑重绘；803E20献祭bit2/pitch-black；8134C0 | +0x48 `u16 storage (SDK i16)` `total_money_spent`【权威】花费累积，原生按无符号40阈值判断；不是Client支付命令（2B；已核）；留Host；不重演支付/解锁，实际价格由Pickup承接；证据：SDK int16_t；7CF3EF..7CF3FC Game→Room→descriptor加16位；7CF400 unsigned cmp 28h；7CF415→903930解锁 | +0x4A `i16` `clear_count`【权威】累计清理计数；当前clear另由bit投影（2B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；7E5DF2/7FBDF4的16位递增及后续movzx | +0x4C `i32` `greed_subtype`【权威】Greed生成／特殊房间内容输入；私有语义依SDK（4B；SDK声明）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；733010/6F4F20确证本4B保存搬移；具体业务名仍SDK声明 | +0x50 `i16` `poop_count`【权威】保存／生成计数（2B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0 | +0x52 `i16` `tainted_keeper_coin_spawns`【权威】重进房防刷硬币计数（2B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；REPENTOGON Wiki GetTaintedKeeperCoinSpawns | +0x54 `i32` `pits_count`【权威】生成坑洞计数；实际Grid/水面另承接（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0 | +0x58 `u32` `decoration_seed`【权威】背景／装饰生成输入；challenge32地图选择输入（4B；已核）；原值留Host；实际背景由Room资源承接，challenge32已选地图标记另提供；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；9635B0 challenge32；Wiki DecorationSeed | +0x5C `u32` `spawn_seed`【换算】实体出生／Error饰品效果选择输入（4B；已核）；原seed及完整Error效果ID留Host；D1只发u8布尔error_is_modeling_clay供最终颜色adapter；效果>0在目标版恒真；实际Image仍由Presentation承接；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；Wiki SpawnSeed；官方GetErrorTrinketEffect说明；IsaacRepentance_static.cpp:588；NUM_TRINKETS=190；ASMHud.cpp:62/207/223 | +0x60 `u32` `award_seed`【权威】清理奖励和商店生成输入（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；Wiki AwardSeed | +0x64 `u32` `boss_death_seed`【权威】Boss死亡生成输入（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0 | +0x68 `std::vector<GridEntityDesc>` `saved_grid_entities`【容器】离房Grid保存；固定初始448×20h分配（12B；已核）；保存头/完整元素不发；有效type18只设置L2.icon_mask.bit14，固定帧0不发；证据：6EBB10分配3823h/对齐32B；964B6D..964BA2扫描type18 | +0x74 `std::vector<EntitySaveState>` `saved_entities`【容器】离房实体保存；78h元素有地图消费者（12B；已核）；完整保存体/类型/variant/subtype/顺序不发；Host纯聚合L2图标mask及必要选中帧，缺席1B/存在3..12B；证据：6EBB10按30 DWORD步长reserve；9635B0读type/variant/subtype/+14/+28/+2C | +0x80 `std::set<int>` `restricted_grid_indexes`【权威】房间生成限制；不是本轮地图消费者（8B；已核）；不读不发元素/树头；不要求为同步实现树遍历；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；官方LuaRoomDescriptor.cpp | +0x88 `std::vector<EntitySaveState>` `saved_effects`【权威】离房效果保存，当前Effect独立投影（12B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；6EBB10第二个78h元素vector | +0x94 `i16[8]` `shop_item_generation_kinds`【权威】八个ShopItemID槽的生成类别，不是最终Pickup subtype；类别完整业务名未强行命名（16B；已核）；全部留Host；只下发实际Pickup/当前Price/价格Sprite；不在Client补货或定价；证据：809640/809F4B初始化写；80A2E0按word+2*slot读取后选variant/池；80A8EF/80B5DC价格分支 | +0xA4 `u8[8]` `shop_restock_counts`【权威】八槽饱和补货次数；ctor不初始化本8B，商店初始化才清零；当前价格由次数参与产生（8B；已核）；全部留Host；只下发实际Pickup/当前Price/价格Sprite；不在Client补货或定价；证据：809689清8B；80AD15..80AD22在FF前递增；80B6FD movzx并三角增价/封顶99 | +0xAC `i16` `shop_item_index`【权威】商店生成槽位（2B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；Wiki ShopItemIdx | +0xAE `i16` `shop_discount_index`【权威】商店折扣生成槽位（2B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；Wiki ShopItemDiscountIdx | +0xB0 `i32` `delirium_distance`【权威】Void生成／Boss选择距离（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；J273 ctor6EBB10/dtor6EBF40；公开字段原绑定852A6A..852B77；Minimap缓存9635B0；Wiki DeliriumDistance | +0xB4 `i32` `group`【权威】传送随机选择的分组（4B；已核）；留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果；证据：RoomDescriptor.zhl；REPENTOGON Wiki GetGroup |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

## RoomDescriptorSavedPickupMapInput：`0x78`字节

EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取

仅type5已构造有效保存元素；未知70B，通用EntitySaveState仍74B；不是生产176变更或完整保存格式。

| +0x0 `i32` `type`【派生】仅type5的场景输入视图，非新原生类（4B；已核）；Host选分支，原type不发；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 | +0x4 `i32[2]` `variant_subtype`【换算】实际图标类别和帧选择输入；未消费subtype分支不读其原值（8B；已核）；Host聚合kind/mask和max frame，原variant/subtype不发；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 | +0xC `bytes` `storage1_2`【未知】原保存体未解释（8B；未知）；不读不发，UNKNOWN保持；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 | +0x14 `u32` `map_exclusion_storage3`【换算】type5地图排除谓词，仅检查是否非零；不宣称所有保存变体都具有同一语义（4B；已核）；Host过滤，谓词原值不发；证据：9645AB；源码未知业务名不影响本consumer的明确比较 | +0x18 `bytes` `storage4_5`【未知】原保存体未解释（8B；未知）；不读不发；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 | +0x20 `i16` `grid_spawn_index`【权威】原生保存/ESSM身份，非地图输入（2B；已核）；留Host；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 | +0x22 `bytes` `bool_storage`【未知】通用保存体未解释（2B；未知）；不读不发；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 | +0x24 `bytes` `storage6`【未知】通用保存体未解释（4B；未知）；不读不发；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 | +0x28 `Vector` `target_position`【换算】仅与原点(0,0)比较，按原生短路读实际需要分量（8B；已核）；Host过滤，坐标不发；证据：9645B5/9645C7；9635B0 | +0x30 `bytes` `storage7`【未知】通用保存体未解释/ESSM复用（4B；未知）；不读不发；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 | +0x34 `u32[2]` `init_drop_seeds`【权威】恢复生产的seed（8B；已核）；留Host；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 | +0x3C `i32[2]` `spawner_type_variant`【权威】恢复生产来源（8B；已核）；留Host；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 | +0x44 `bytes` `unknown44`【未知】通用保存体未知（12B；未知）；不读不发；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 | +0x50 `bytes` `float_storage`【未知】通用保存体未知（8B；未知）；不读不发；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 | +0x58 `bytes` `unknown58`【未知】通用保存体未知（4B；未知）；不读不发；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 | +0x5C `bytes` `storage8`【未知】通用保存体未知（4B；未知）；不读不发；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 | +0x60 `bytes` `byte_storage`【未知】通用保存体未知（1B；未知）；不读不发；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 | +0x61 `bytes` `unknown61`【未知】通用保存体未知（11B；未知）；不读不发；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 | +0x6C `strong pair` `nested_save_holder`【本地】递归保存体与控制块本地生命周期（8B；SDK声明）；不发地址/控制块，不作为实体引用；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 | +0x74 `bytes` `unknown74`【未知】通用保存体未知（4B；未知）；不读不发；证据：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

## RoomDescriptorSavedGridMapInput：`0x20`字节

GridEntityDesc.zhl；6EBB10；964B6D..964BA2

只提取地图楼梯结果；不恢复第6步当前Grid字段发送决定。

| +0x0 `i32` `type`【换算】只判断是否GRID_STAIRS=18（4B；已核）；有效type18设置L2.icon_mask.bit14；不另发楼梯字段／常量帧；证据：964B6D..964BA2 | +0x4 `i32/u32[5]` `other_generation_fields`【权威】variant/state/spawnCount/spawnSeed/varData；在此保存地图视图不消费（20B；已核）；留Host；不复制保存体；证据：GridEntityDesc.zhl；既有198布局；6EBB10 | +0x18 `u8` `initialized`【权威】保存初始化输入，地图不使用（1B；已核）；留Host；证据：GridEntityDesc.zhl | +0x19 `bytes` `unknown19`【未知】GridEntityDesc既有3B未知（3B；未知）；不读不发；不因外层Host而消除；证据：Grid公共账本 | +0x1C `u32` `variable_seed`【权威】保存生成seed，地图不使用（4B；已核）；留Host；证据：GridEntityDesc.zhl |
| --- | --- | --- | --- | --- |

## RoomDescriptorShopKinds：`0x10`字节

809640/80A2E0/80A790/80B5A0

内嵌于descriptor+94；逻辑8项，不能根据非法index8的别名扩大为9项。

| +0x0 `i16` `slot0_generation_kind`【权威】货物类别，产生实际variant和池选择（2B；已核）；不发；实际Pickup承接；证据：809640填表；80A2E0消费 | +0x2 `i16` `slot1_generation_kind`【权威】货物类别，产生实际variant和池选择（2B；已核）；不发；实际Pickup承接；证据：809640填表；80A2E0消费 | +0x4 `i16` `slot2_generation_kind`【权威】货物类别，产生实际variant和池选择（2B；已核）；不发；实际Pickup承接；证据：809640填表；80A2E0消费 | +0x6 `i16` `slot3_generation_kind`【权威】货物类别，产生实际variant和池选择（2B；已核）；不发；实际Pickup承接；证据：809640填表；80A2E0消费 | +0x8 `i16` `slot4_generation_kind`【权威】货物类别，产生实际variant和池选择（2B；已核）；不发；实际Pickup承接；证据：809640填表；80A2E0消费 | +0xA `i16` `slot5_generation_kind`【权威】货物类别，产生实际variant和池选择（2B；已核）；不发；实际Pickup承接；证据：809640填表；80A2E0消费 | +0xC `i16` `slot6_generation_kind`【权威】货物类别，产生实际variant和池选择（2B；已核）；不发；实际Pickup承接；证据：809640填表；80A2E0消费 | +0xE `i16` `slot7_generation_kind`【权威】货物类别，产生实际variant和池选择（2B；已核）；不发；实际Pickup承接；证据：809640填表；80A2E0消费 |
| --- | --- | --- | --- | --- | --- | --- | --- |

## RoomDescriptorRestockCounts：`0x8`字节

809689/80AD15/80AD22/80B6FD

内嵌于descriptor+A4，商店Init前不由ctor获得读取资格。

| +0x0 `u8` `slot0_restock_count`【权威】0..255饱和补货次数（1B；已核）；不发；最终Price承接；证据：80AD15..22饱和增；80B6FD movzx | +0x1 `u8` `slot1_restock_count`【权威】0..255饱和补货次数（1B；已核）；不发；最终Price承接；证据：80AD15..22饱和增；80B6FD movzx | +0x2 `u8` `slot2_restock_count`【权威】0..255饱和补货次数（1B；已核）；不发；最终Price承接；证据：80AD15..22饱和增；80B6FD movzx | +0x3 `u8` `slot3_restock_count`【权威】0..255饱和补货次数（1B；已核）；不发；最终Price承接；证据：80AD15..22饱和增；80B6FD movzx | +0x4 `u8` `slot4_restock_count`【权威】0..255饱和补货次数（1B；已核）；不发；最终Price承接；证据：80AD15..22饱和增；80B6FD movzx | +0x5 `u8` `slot5_restock_count`【权威】0..255饱和补货次数（1B；已核）；不发；最终Price承接；证据：80AD15..22饱和增；80B6FD movzx | +0x6 `u8` `slot6_restock_count`【权威】0..255饱和补货次数（1B；已核）；不发；最终Price承接；证据：80AD15..22饱和增；80B6FD movzx | +0x7 `u8` `slot7_restock_count`【权威】0..255饱和补货次数（1B；已核）；不发；最终Price承接；证据：80AD15..22饱和增；80B6FD movzx |
| --- | --- | --- | --- | --- | --- | --- | --- |

## RoomDescriptorRestrictedSetHeader：`0x8`字节

RoomDescriptor.zhl；6EBB10/6EBF40/733010

真实8B头，生成限制set不进入本轮下行；不据此推定全部节点ABI已独立核定。

| +0x0 `native pointer` `head`【本地】红黑树head，ctor分配14h节点（4B；已核）；不发地址/树头；证据：6EBB10；6EBF40 | +0x4 `u32` `size`【权威】限制集合元素数（4B；已核）；不发数量/元素；证据：6EBB10；733010；LuaRoomDescriptor.cpp遍历set |
| --- | --- |

## 设计线表

不包括记录头／结构身份／配置资源及其它显示上下文；原生地址／保存头不上线。

### L1_RoomDescriptorDisplay：53

| +0x0 `i32` `grid_index`：描述原始GridIndex（4B） | +0x4 `i32` `safe_grid_index`：描述原始SafeGridIndex（4B） | +0x8 `u8` `dimension`：描述维度0..2（1B） | +0x9 `u16` `data`：基础配置引用，FFFF为空（2B） | +0xB `u16` `override_data`：替换配置独立引用，FFFF为空（2B） | +0xD `u8` `allowed_doors`：低八门槽mask（1B） | +0xE `i32[8]` `doors`：八个结构目标，含特殊路由值（32B） | +0x2E `u32` `display_flags`：最终地图显示标志（4B） | +0x32 `u8` `visited`：VisitedCount!=0（1B） | +0x33 `u8` `display_bits`：clear/red_room/devil_treasure/blue_redirect/has_water五位，其余零（1B） | +0x34 `u8` `greed_treasure_icon`：Greed特殊宝物房图标最终开关，不适用为false（1B） |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

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
