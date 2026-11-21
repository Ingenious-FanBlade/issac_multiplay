# tx-04-12 特殊房间补全覆盖清单

状态：清单已实现并通过同tx统一E2E，当前结果与边界见[补充实现及验收](../../../docs/archive/design/observation-expansion/tx-04-12-supplement.md)。历史5/5仅证明旧场景。

来自两套本地Wiki、目标SDK及原版STB静态布局（包括独立greed资源集）。按房间类型、原生离图槽位、公开结构子类、全部房形、维度和路线分别列项；每项必须有Complete的正式Room/Level样本，不能仅有夹具请求或debug类型计数。

隔离排除：RoomType30/槽-19属于原版Online，禁止进入/初始化；0/数量常量/NO_ROOM_IDX不是真实房间；-100/-101仅为门路由哨兵，禁止按房号进入。目标SDK实际有20个离图槽（总527），本地Wiki的MAX_ROOMS=525为旧值。

206项是按不同风险维度计数的场景，不是206种RoomType。29个类型项只测试各类型；12个形状项统一使用ROOM_DEFAULT，没有做29×12的组合。槽位、公开子类、原生入口、路线各有独立判据，同一类型在这些项中可以重复。
29种单机适用类型含当前未自然使用的传送器入口/出口25/26，使用其原版Mausoleum布局进行受控构建，不宣称自然触发。离图槽的受控配置构建、原生物品/入口流程以及调试槽分别标明。Genesis、天使/恶魔、夹层必须另外走真实原生入口。所有离图槽逐项退出/回访；原生slot -1的天使/恶魔和slot -3多配置重建不能被合并成同一构建版本。

| 必测ID | 分类/方式 | 实际要求 | 来源布局 |
| --- | --- | --- | --- |
| `type_ROOM_DEFAULT` | room_type / controlled-config | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 1.0.0 test room |
| `type_ROOM_SHOP` | room_type / controlled-config | {"expected_type": 2, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 2.0.0 Shop L1 |
| `type_ROOM_ERROR` | room_type / controlled-config | {"expected_type": 3, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 3.0.0  |
| `type_ROOM_TREASURE` | room_type / controlled-config | {"expected_type": 4, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 4.0.0  |
| `type_ROOM_BOSS` | room_type / controlled-config | {"expected_type": 5, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -3} | 0 / 5.1010.1 Monstro |
| `type_ROOM_MINIBOSS` | room_type / controlled-config | {"expected_type": 6, "expected_subtype": 2, "expected_shape": 1, "expected_grid": -3} | 0 / 6.2000.2 Wrath |
| `type_ROOM_SECRET` | room_type / controlled-config | {"expected_type": 7, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 7.0.0  |
| `type_ROOM_SUPERSECRET` | room_type / controlled-config | {"expected_type": 8, "expected_subtype": 10, "expected_shape": 1, "expected_grid": -3} | 0 / 8.0.10  |
| `type_ROOM_ARCADE` | room_type / controlled-config | {"expected_type": 9, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 9.0.0  |
| `type_ROOM_CURSE` | room_type / controlled-config | {"expected_type": 10, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 10.0.0  |
| `type_ROOM_CHALLENGE` | room_type / controlled-config | {"expected_type": 11, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 11.0.0  |
| `type_ROOM_LIBRARY` | room_type / controlled-config | {"expected_type": 12, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 12.0.0  |
| `type_ROOM_SACRIFICE` | room_type / controlled-config | {"expected_type": 13, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 13.0.0  |
| `type_ROOM_DEVIL` | room_type / controlled-config | {"expected_type": 14, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 14.0.0 Devil (Shop) |
| `type_ROOM_ANGEL` | room_type / controlled-config | {"expected_type": 15, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 15.0.0 Angel |
| `type_ROOM_DUNGEON` | room_type / controlled-config | {"expected_type": 16, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 16.0.0 ItemDungeon |
| `type_ROOM_BOSSRUSH` | room_type / controlled-config | {"expected_type": 17, "expected_subtype": 0, "expected_shape": 8, "expected_grid": -3} | 0 / 17.0.0 Placeholder Boss Rush |
| `type_ROOM_ISAACS` | room_type / controlled-config | {"expected_type": 18, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 18.0.0 Isaac's Room |
| `type_ROOM_BARREN` | room_type / controlled-config | {"expected_type": 19, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 19.0.0 The Barren Room |
| `type_ROOM_CHEST` | room_type / controlled-config | {"expected_type": 20, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 20.0.0 The Chest Room (Placeholder) |
| `type_ROOM_DICE` | room_type / controlled-config | {"expected_type": 21, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 21.0.0 The Dice Room (Placeholder) |
| `type_ROOM_BLACK_MARKET` | room_type / controlled-config | {"expected_type": 22, "expected_subtype": 0, "expected_shape": 6, "expected_grid": -3} | 0 / 22.0.0 The Black Market (Racist Placeholder) (copy) |
| `type_ROOM_PLANETARIUM` | room_type / controlled-config | {"expected_type": 24, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 24.0.0 Planetarium |
| `type_ROOM_TELEPORTER` | room_type / controlled-config | {"expected_type": 25, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 31 / 25.0.0 Teleporter Entrance |
| `type_ROOM_TELEPORTER_EXIT` | room_type / controlled-config | {"expected_type": 26, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 31 / 26.0.0 Teleporter Exit |
| `type_ROOM_SECRET_EXIT` | room_type / controlled-config | {"expected_type": 27, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -3} | 0 / 27.1.1 Secret Exit (downpour) |
| `type_ROOM_BLUE` | room_type / controlled-config | {"expected_type": 28, "expected_subtype": 0, "expected_shape": 4, "expected_grid": -3} | 0 / 28.0.0 Blue Room |
| `type_ROOM_ULTRASECRET` | room_type / controlled-config | {"expected_type": 29, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 29.0.0  |
| `slot_1` | off_grid_slot / controlled-slot | {"expected_type": 14, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -1} | 0 / 14.0.0 Devil (Shop) |
| `slot_2` | off_grid_slot / controlled-slot | {"expected_type": 3, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -2} | 0 / 3.0.0  |
| `slot_3` | off_grid_slot / controlled-slot | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 1.0.0 test room |
| `slot_4` | off_grid_slot / controlled-slot | {"expected_type": 16, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -4} | 0 / 16.0.0 ItemDungeon |
| `slot_5` | off_grid_slot / controlled-slot | {"expected_type": 17, "expected_subtype": 0, "expected_shape": 8, "expected_grid": -5} | 0 / 17.0.0 Placeholder Boss Rush |
| `slot_6` | off_grid_slot / controlled-slot | {"expected_type": 22, "expected_subtype": 0, "expected_shape": 6, "expected_grid": -6} | 0 / 22.0.0 The Black Market (Racist Placeholder) (copy) |
| `slot_7` | off_grid_slot / controlled-slot | {"expected_type": 5, "expected_subtype": 55, "expected_shape": 4, "expected_grid": -7} | 0 / 5.5000.55 Mega Satan |
| `slot_8` | off_grid_slot / controlled-slot | {"expected_type": 5, "expected_subtype": 63, "expected_shape": 8, "expected_grid": -8} | 13 / 5.0.63 Boss Room |
| `slot_9` | off_grid_slot / controlled-slot | {"expected_type": 5, "expected_subtype": 70, "expected_shape": 8, "expected_grid": -9} | 0 / 5.3414.70 Delirium |
| `slot_10` | off_grid_slot / controlled-slot | {"expected_type": 27, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -10} | 0 / 27.1.1 Secret Exit (downpour) |
| `slot_11` | off_grid_slot / controlled-slot | {"expected_type": 16, "expected_subtype": 1, "expected_shape": 4, "expected_grid": -11} | 0 / 16.1000.1 Gideon's Grave |
| `slot_12` | off_grid_slot / controlled-slot | {"expected_type": 18, "expected_subtype": 99, "expected_shape": 1, "expected_grid": -12} | 0 / 18.1000.99 Genesis Room |
| `slot_13` | off_grid_slot / controlled-slot | {"expected_type": 2, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -13} | 0 / 2.0.0 Shop L1 |
| `slot_14` | off_grid_slot / controlled-slot | {"expected_type": 16, "expected_subtype": 2, "expected_shape": 1, "expected_grid": -14} | 0 / 16.1010.2 Rotgut Maggot |
| `slot_15` | off_grid_slot / controlled-slot | {"expected_type": 16, "expected_subtype": 3, "expected_shape": 1, "expected_grid": -15} | 0 / 16.1020.3 Rotgut Heart |
| `slot_16` | off_grid_slot / controlled-slot | {"expected_type": 28, "expected_subtype": 0, "expected_shape": 4, "expected_grid": -16} | 0 / 28.0.0 Blue Room |
| `slot_17` | off_grid_slot / controlled-slot | {"expected_type": 5, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -17} | 25 / 5.0.0 Monstro |
| `slot_18` | off_grid_slot / controlled-slot | {"expected_type": 15, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -18} | 0 / 15.40.1 Angel Shop |
| `slot_20` | off_grid_slot / controlled-slot | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -20} | 0 / 1.0.0 test room |
| `slot_devil_native` | native_entry / devil | {"expected_type": 14, "expected_room": -1} | 0 / 14.0.0 Devil (Shop) |
| `slot_angel_native` | native_entry / angel | {"expected_type": 15, "expected_room": -1} | 0 / 15.0.0 Angel |
| `genesis_native` | native_entry / genesis | {"expected_type": 18, "expected_subtype": 99, "expected_shape": 1, "expected_room": -12} | 0 / 18.1000.99 Genesis Room |
| `crawlspace_native` | native_entry / crawlspace | {"expected_type": 16, "expected_subtype": 0, "expected_shape": 1, "expected_room": -4} | 0 / 16.0.0 ItemDungeon |
| `shape_1` | room_shape / controlled-config | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 1 / 1.0.0  |
| `shape_2` | room_shape / controlled-config | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 2, "expected_grid": -3} | 1 / 1.569.0 New Room |
| `shape_3` | room_shape / controlled-config | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 3, "expected_grid": -3} | 1 / 1.578.0 New Room |
| `shape_4` | room_shape / controlled-config | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 4, "expected_grid": -3} | 1 / 1.345.0 New Room |
| `shape_5` | room_shape / controlled-config | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 5, "expected_grid": -3} | 1 / 1.700.0 New Room |
| `shape_6` | room_shape / controlled-config | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 6, "expected_grid": -3} | 1 / 1.375.0 New Room |
| `shape_7` | room_shape / controlled-config | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 7, "expected_grid": -3} | 1 / 1.712.0 New Room |
| `shape_8` | room_shape / controlled-config | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 8, "expected_grid": -3} | 1 / 1.405.0 New Room |
| `shape_9` | room_shape / controlled-config | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 9, "expected_grid": -3} | 1 / 1.736.0 New Room |
| `shape_10` | room_shape / controlled-config | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 10, "expected_grid": -3} | 1 / 1.737.0 New Room (copy) |
| `shape_11` | room_shape / controlled-config | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 11, "expected_grid": -3} | 1 / 1.738.0 New Room (copy) |
| `shape_12` | room_shape / controlled-config | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 12, "expected_grid": -3} | 1 / 1.739.0 New Room (copy) |
| `subtype_2_0` | public_subtype / controlled-config | {"expected_type": 2, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 2.0.0 Shop L1 |
| `subtype_2_1` | public_subtype / controlled-config | {"expected_type": 2, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -3} | 0 / 2.1.1 Shop L2 |
| `subtype_2_2` | public_subtype / controlled-config | {"expected_type": 2, "expected_subtype": 2, "expected_shape": 1, "expected_grid": -3} | 0 / 2.2.2 Shop L3 |
| `subtype_2_3` | public_subtype / controlled-config | {"expected_type": 2, "expected_subtype": 3, "expected_shape": 1, "expected_grid": -3} | 0 / 2.3.3 Shop L4 |
| `subtype_2_4` | public_subtype / controlled-config | {"expected_type": 2, "expected_subtype": 4, "expected_shape": 1, "expected_grid": -3} | 0 / 2.4.4 Shop L5 |
| `subtype_2_10` | public_subtype / controlled-config | {"expected_type": 2, "expected_subtype": 10, "expected_shape": 1, "expected_grid": -3} | 0 / 2.5.10 Rare Shop (good) |
| `subtype_2_11` | public_subtype / controlled-config | {"expected_type": 2, "expected_subtype": 11, "expected_shape": 1, "expected_grid": -3} | 0 / 2.6.11 Rare Shop (bad) |
| `subtype_2_100` | public_subtype / controlled-config | {"expected_type": 2, "expected_subtype": 100, "expected_shape": 1, "expected_grid": -3} | 0 / 2.7.100 Keeper Shop L1 |
| `subtype_2_101` | public_subtype / controlled-config | {"expected_type": 2, "expected_subtype": 101, "expected_shape": 1, "expected_grid": -3} | 0 / 2.8.101 Keeper Shop L2 |
| `subtype_2_102` | public_subtype / controlled-config | {"expected_type": 2, "expected_subtype": 102, "expected_shape": 1, "expected_grid": -3} | 0 / 2.9.102 Keeper Shop L3 |
| `subtype_2_103` | public_subtype / controlled-config | {"expected_type": 2, "expected_subtype": 103, "expected_shape": 1, "expected_grid": -3} | 0 / 2.10.103 Keeper Shop L4 |
| `subtype_2_104` | public_subtype / controlled-config | {"expected_type": 2, "expected_subtype": 104, "expected_shape": 1, "expected_grid": -3} | 0 / 2.11.104 Keeper Shop L5 |
| `subtype_2_110` | public_subtype / controlled-config | {"expected_type": 2, "expected_subtype": 110, "expected_shape": 1, "expected_grid": -3} | 0 / 2.12.110 Keeper Shop (good) |
| `subtype_2_111` | public_subtype / controlled-config | {"expected_type": 2, "expected_subtype": 111, "expected_shape": 1, "expected_grid": -3} | 0 / 2.13.111 Keeper Shop (bad) |
| `subtype_4_0` | public_subtype / controlled-config | {"expected_type": 4, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 4.0.0  |
| `subtype_4_1` | public_subtype / controlled-config | {"expected_type": 4, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -3} | 0 / 4.36.1 double t |
| `subtype_4_2` | public_subtype / controlled-config | {"expected_type": 4, "expected_subtype": 2, "expected_shape": 1, "expected_grid": -3} | 0 / 4.47.2 reroll |
| `subtype_4_3` | public_subtype / controlled-config | {"expected_type": 4, "expected_subtype": 3, "expected_shape": 1, "expected_grid": -3} | 0 / 4.52.3 double t reroll |
| `subtype_4_34` | public_subtype / controlled-config | {"expected_type": 4, "expected_subtype": 34, "expected_shape": 1, "expected_grid": -3} | 27 / 4.10000.34 Knife Piece Room |
| `subtype_6_0` | public_subtype / controlled-config | {"expected_type": 6, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 6.2030.0 Sloth |
| `subtype_6_1` | public_subtype / controlled-config | {"expected_type": 6, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -3} | 0 / 6.2020.1 Lust |
| `subtype_6_2` | public_subtype / controlled-config | {"expected_type": 6, "expected_subtype": 2, "expected_shape": 1, "expected_grid": -3} | 0 / 6.2000.2 Wrath |
| `subtype_6_3` | public_subtype / controlled-config | {"expected_type": 6, "expected_subtype": 3, "expected_shape": 1, "expected_grid": -3} | 0 / 6.2010.3 Gluttony |
| `subtype_6_4` | public_subtype / controlled-config | {"expected_type": 6, "expected_subtype": 4, "expected_shape": 1, "expected_grid": -3} | 0 / 6.2040.4 Greed |
| `subtype_6_5` | public_subtype / controlled-config | {"expected_type": 6, "expected_subtype": 5, "expected_shape": 1, "expected_grid": -3} | 0 / 6.2050.5 Envy |
| `subtype_6_6` | public_subtype / controlled-config | {"expected_type": 6, "expected_subtype": 6, "expected_shape": 1, "expected_grid": -3} | 0 / 6.2060.6 Pride |
| `subtype_6_7` | public_subtype / controlled-config | {"expected_type": 6, "expected_subtype": 7, "expected_shape": 1, "expected_grid": -3} | 0 / 6.2130.7 Super Sloth |
| `subtype_6_8` | public_subtype / controlled-config | {"expected_type": 6, "expected_subtype": 8, "expected_shape": 1, "expected_grid": -3} | 0 / 6.2120.8 Super Lust |
| `subtype_6_9` | public_subtype / controlled-config | {"expected_type": 6, "expected_subtype": 9, "expected_shape": 1, "expected_grid": -3} | 0 / 6.2100.9 Super Wrath |
| `subtype_6_10` | public_subtype / controlled-config | {"expected_type": 6, "expected_subtype": 10, "expected_shape": 1, "expected_grid": -3} | 0 / 6.2110.10 Super Gluttony |
| `subtype_6_11` | public_subtype / controlled-config | {"expected_type": 6, "expected_subtype": 11, "expected_shape": 1, "expected_grid": -3} | 0 / 6.2140.11 Super Greed |
| `subtype_6_12` | public_subtype / controlled-config | {"expected_type": 6, "expected_subtype": 12, "expected_shape": 1, "expected_grid": -3} | 0 / 6.2150.12 Super Envy |
| `subtype_6_13` | public_subtype / controlled-config | {"expected_type": 6, "expected_subtype": 13, "expected_shape": 1, "expected_grid": -3} | 0 / 6.2160.13 Super Pride |
| `subtype_6_14` | public_subtype / controlled-config | {"expected_type": 6, "expected_subtype": 14, "expected_shape": 1, "expected_grid": -3} | 0 / 6.2260.14 Ultra Pride |
| `subtype_6_15` | public_subtype / controlled-config | {"expected_type": 6, "expected_subtype": 15, "expected_shape": 1, "expected_grid": -3} | 0 / 6.2300.15 Krampus |
| `subtype_9_0` | public_subtype / controlled-config | {"expected_type": 9, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 9.0.0  |
| `subtype_9_1` | public_subtype / controlled-config | {"expected_type": 9, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -3} | 0 / 9.41.1 Cain Arcade |
| `subtype_10_0` | public_subtype / controlled-config | {"expected_type": 10, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 10.0.0  |
| `subtype_10_1` | public_subtype / controlled-config | {"expected_type": 10, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -3} | 0 / 10.31.1 (voodoo head) |
| `subtype_11_0` | public_subtype / controlled-config | {"expected_type": 11, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 11.0.0  |
| `subtype_11_1` | public_subtype / controlled-config | {"expected_type": 11, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -3} | 0 / 11.16.1  |
| `subtype_11_10` | public_subtype / controlled-config | {"expected_type": 11, "expected_subtype": 10, "expected_shape": 1, "expected_grid": -3} | 0 / 11.100.10 default wave (easy) |
| `subtype_11_11` | public_subtype / controlled-config | {"expected_type": 11, "expected_subtype": 11, "expected_shape": 1, "expected_grid": -3} | 0 / 11.104.11 default boss wave (easy) |
| `subtype_11_12` | public_subtype / controlled-config | {"expected_type": 11, "expected_subtype": 12, "expected_shape": 4, "expected_grid": -3} | 0 / 11.200.12 default gideon wave (easy) |
| `subtype_12_0` | public_subtype / controlled-config | {"expected_type": 12, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 12.0.0  |
| `subtype_12_1` | public_subtype / controlled-config | {"expected_type": 12, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -3} | 0 / 12.2.1  |
| `subtype_12_2` | public_subtype / controlled-config | {"expected_type": 12, "expected_subtype": 2, "expected_shape": 1, "expected_grid": -3} | 0 / 12.9.2  |
| `subtype_12_3` | public_subtype / controlled-config | {"expected_type": 12, "expected_subtype": 3, "expected_shape": 1, "expected_grid": -3} | 0 / 12.4.3  |
| `subtype_12_4` | public_subtype / controlled-config | {"expected_type": 12, "expected_subtype": 4, "expected_shape": 1, "expected_grid": -3} | 0 / 12.5.4  |
| `subtype_14_0` | public_subtype / controlled-config | {"expected_type": 14, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 14.0.0 Devil (Shop) |
| `subtype_14_1` | public_subtype / controlled-config | {"expected_type": 14, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -3} | 0 / 14.25.1 Devil Room (6 Room) |
| `subtype_15_0` | public_subtype / controlled-config | {"expected_type": 15, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 15.0.0 Angel |
| `subtype_15_1` | public_subtype / controlled-config | {"expected_type": 15, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -3} | 0 / 15.40.1 Angel Shop |
| `subtype_16_0` | public_subtype / controlled-config | {"expected_type": 16, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 16.0.0 ItemDungeon |
| `subtype_16_1` | public_subtype / controlled-config | {"expected_type": 16, "expected_subtype": 1, "expected_shape": 4, "expected_grid": -3} | 0 / 16.1000.1 Gideon's Grave |
| `subtype_16_2` | public_subtype / controlled-config | {"expected_type": 16, "expected_subtype": 2, "expected_shape": 1, "expected_grid": -3} | 0 / 16.1010.2 Rotgut Maggot |
| `subtype_16_3` | public_subtype / existing-slot | {"expected_type": 16, "expected_subtype": 3, "expected_shape": 1, "expected_grid": -15} | 0 / 16.1020.3 Rotgut Heart |
| `subtype_16_4` | public_subtype / controlled-config | {"expected_type": 16, "expected_subtype": 4, "expected_shape": 1, "expected_grid": -3} | 35 / 16.666.4 Beast Room |
| `subtype_18_0` | public_subtype / controlled-config | {"expected_type": 18, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 0 / 18.0.0 Isaac's Room |
| `subtype_18_99` | public_subtype / controlled-config | {"expected_type": 18, "expected_subtype": 99, "expected_shape": 1, "expected_grid": -3} | 0 / 18.1000.99 Genesis Room |
| `subtype_27_1` | public_subtype / controlled-config | {"expected_type": 27, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -3} | 0 / 27.1.1 Secret Exit (downpour) |
| `subtype_27_2` | public_subtype / controlled-config | {"expected_type": 27, "expected_subtype": 2, "expected_shape": 1, "expected_grid": -3} | 0 / 27.7.2 Secret Exit (mines) |
| `subtype_27_3` | public_subtype / controlled-config | {"expected_type": 27, "expected_subtype": 3, "expected_shape": 1, "expected_grid": -3} | 0 / 27.14.3 Secret Exit (mausoleum) |
| `default_special_7_1` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -3} | 7 / 1.10000.1 Fool Room |
| `default_special_27_1` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -3} | 27 / 1.5000.1 White Fire Room |
| `default_special_27_34` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 34, "expected_shape": 1, "expected_grid": -3} | 27 / 1.10000.34 Mirror Room |
| `default_special_29_1` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -3} | 29 / 1.5000.1 Button Room |
| `default_special_29_10` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 10, "expected_shape": 4, "expected_grid": -3} | 29 / 1.10000.10 Secret Entrance |
| `default_special_29_11` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 11, "expected_shape": 1, "expected_grid": -3} | 29 / 1.10009.11 Mineshaft Lobby |
| `default_special_29_20` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 20, "expected_shape": 1, "expected_grid": -3} | 29 / 1.10018.20 Knife Piece Room |
| `default_special_29_30` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 30, "expected_shape": 8, "expected_grid": -3} | 29 / 1.10024.30 Mineshaft Room (big straight) |
| `default_special_29_31` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 31, "expected_shape": 8, "expected_grid": -3} | 29 / 1.10025.31 Mineshaft Room (big straight) |
| `default_special_35_0` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 35 / 1.1.0 Isaac's Bedroom |
| `default_special_35_1` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 1, "expected_shape": 5, "expected_grid": -3} | 35 / 1.2.1 Hallway |
| `default_special_35_2` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 2, "expected_shape": 1, "expected_grid": -3} | 35 / 1.3.2 Mom's Bedroom |
| `default_special_35_3` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 3, "expected_shape": 4, "expected_grid": -3} | 35 / 1.4.3 Living Room |
| `default_special_35_10` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 10, "expected_shape": 2, "expected_grid": -3} | 35 / 1.5.10 Closet R |
| `default_special_35_11` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 11, "expected_shape": 2, "expected_grid": -3} | 35 / 1.6.11 Closet L |
| `default_special_35_33` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 33, "expected_shape": 2, "expected_grid": -3} | 35 / 1.100.33 Death Certificate |
| `default_special_35_34` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 34, "expected_shape": 1, "expected_grid": -3} | 35 / 1.101.34 Death Certificate |
| `default_special_36_0` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 36 / 1.250.0 New Room |
| `default_special_36_1` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 1, "expected_shape": 1, "expected_grid": -3} | 36 / 1.189.1 New Room |
| `default_special_36_4` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 4, "expected_shape": 1, "expected_grid": -3} | 36 / 1.94.4 New Room |
| `default_special_36_7` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 7, "expected_shape": 1, "expected_grid": -3} | 36 / 1.0.7 New Room |
| `default_special_36_27` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 27, "expected_shape": 1, "expected_grid": -3} | 36 / 1.433.27 New Room |
| `default_special_36_29` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 29, "expected_shape": 1, "expected_grid": -3} | 36 / 1.355.29 New Room |
| `default_special_36_31` | special_default_layout / controlled-config | {"expected_type": 1, "expected_subtype": 31, "expected_shape": 1, "expected_grid": -3} | 36 / 1.276.31 New Room |
| `route_downpour` | route / stage | {"expected_dimension": 0} |  / ..  |
| `route_dross` | route / stage | {"expected_dimension": 0} |  / ..  |
| `route_mines` | route / stage | {"expected_dimension": 0} |  / ..  |
| `route_ashpit` | route / stage | {"expected_dimension": 0} |  / ..  |
| `route_mausoleum` | route / stage | {"expected_dimension": 0} |  / ..  |
| `route_gehenna` | route / stage | {"expected_dimension": 0} |  / ..  |
| `route_corpse` | route / stage | {"expected_dimension": 0} |  / ..  |
| `route_blue_womb` | route / stage | {"expected_dimension": 0} |  / ..  |
| `route_sheol` | route / stage | {"expected_dimension": 0} |  / ..  |
| `route_cathedral` | route / stage | {"expected_dimension": 0} |  / ..  |
| `route_dark_room` | route / stage | {"expected_dimension": 0} |  / ..  |
| `route_chest` | route / stage | {"expected_dimension": 0} |  / ..  |
| `route_void` | route / stage | {"expected_dimension": 0} |  / ..  |
| `route_home` | route / stage | {"expected_dimension": 0} |  / ..  |
| `mirror_entrance` | route / route-room | {"expected_dimension": 0} |  / ..  |
| `mirror_inside` | dimension / dimension | {"expected_dimension": 1} |  / ..  |
| `mineshaft_entrance` | route / route-room | {"expected_dimension": 0} |  / ..  |
| `mineshaft_inside` | dimension / dimension | {"expected_dimension": 1} |  / ..  |
| `death_certificate` | dimension / death-certificate | {"expected_dimension": 2} |  / ..  |
| `red_room` | structure_change / red-room | {} |  / ..  |
| `ascent` | route / ascent | {"expected_ascent": true} |  / ..  |
| `greed` | progress / greed | {"expected_greed": true} |  / ..  |
| `greed_layout_1` | greed_room / controlled-config | {"expected_type": 1, "expected_subtype": 0, "expected_shape": 4, "expected_grid": -3, "expected_greed": true} | 1 / 1.1000.0 Basement |
| `greed_layout_2` | greed_room / controlled-config | {"expected_type": 2, "expected_subtype": 0, "expected_shape": 6, "expected_grid": -3, "expected_greed": true} | 0 / 2.0.0  |
| `greed_layout_4` | greed_room / controlled-config | {"expected_type": 4, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3, "expected_greed": true} | 0 / 4.0.0  |
| `greed_layout_5` | greed_room / controlled-config | {"expected_type": 5, "expected_subtype": 1, "expected_shape": 4, "expected_grid": -3, "expected_greed": true} | 1 / 5.0.1 New Room |
| `greed_layout_8` | greed_room / controlled-config | {"expected_type": 8, "expected_subtype": 10, "expected_shape": 1, "expected_grid": -3, "expected_greed": true} | 0 / 8.0.10  |
| `greed_layout_10` | greed_room / controlled-config | {"expected_type": 10, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3, "expected_greed": true} | 0 / 10.0.0  |
| `greed_layout_11` | greed_room / controlled-config | {"expected_type": 11, "expected_subtype": 0, "expected_shape": 4, "expected_grid": -3, "expected_greed": true} | 1 / 11.0.0 New Room (copy) |
| `greed_layout_14` | greed_room / controlled-config | {"expected_type": 14, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3, "expected_greed": true} | 0 / 14.0.0 Devil (Shop) |
| `greed_layout_15` | greed_room / controlled-config | {"expected_type": 15, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3, "expected_greed": true} | 0 / 15.0.0 Angel |
| `greed_layout_18` | greed_room / controlled-config | {"expected_type": 18, "expected_subtype": 99, "expected_shape": 1, "expected_grid": -3, "expected_greed": true} | 0 / 18.1000.99 Genesis Room |
| `greed_layout_23` | greed_room / controlled-config | {"expected_type": 23, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3, "expected_greed": true} | 0 / 23.0.0  |
| `greed_layout_28` | greed_room / controlled-config | {"expected_type": 28, "expected_subtype": 0, "expected_shape": 4, "expected_grid": -3, "expected_greed": true} | 0 / 28.0.0 Blue Room |
| `greed_ultra_layout` | greed_room / controlled-config | {"expected_type": 5, "expected_subtype": 62, "expected_shape": 4, "expected_grid": -3, "expected_greed": true} | 25 / 5.1000.62 Ultra Greed |
| `greed_generated_1` | greed_room / greed-room | {"expected_type": 1, "expected_greed": true} |  / ..  |
| `greed_generated_2` | greed_room / greed-room | {"expected_type": 2, "expected_greed": true} |  / ..  |
| `greed_generated_4` | greed_room / greed-room | {"expected_type": 4, "expected_greed": true} |  / ..  |
| `greed_generated_8` | greed_room / greed-room | {"expected_type": 8, "expected_greed": true} |  / ..  |
| `greed_generated_10` | greed_room / greed-room | {"expected_type": 10, "expected_greed": true} |  / ..  |
| `greed_generated_23` | greed_room / greed-room | {"expected_type": 23, "expected_greed": true} |  / ..  |
| `greed_native_angel` | greed_room / angel | {"expected_room": -1, "expected_type": 15, "expected_greed": true} |  / ..  |
| `greed_native_devil` | greed_room / devil | {"expected_room": -1, "expected_type": 14, "expected_greed": true} |  / ..  |
| `greed_arena` | greed_room / greed-room | {"expected_type": 1, "expected_shape": 4, "expected_greed": true} |  / ..  |
| `greed_wave` | progress / greed-wave | {"expected_greed": true, "expected_wave": true} |  / ..  |
| `type_ROOM_GREED_EXIT` | room_type / controlled-config | {"expected_type": 23, "expected_subtype": 0, "expected_shape": 1, "expected_grid": -3} | 18 / 23.0.0  |
| `greed_floor_2` | greed_floor / greed-stage | {"expected_stage": 2, "expected_greed": true} |  / ..  |
| `greed_floor_3` | greed_floor / greed-stage | {"expected_stage": 3, "expected_greed": true} |  / ..  |
| `greed_floor_4` | greed_floor / greed-stage | {"expected_stage": 4, "expected_greed": true} |  / ..  |
| `greed_floor_5` | greed_floor / greed-stage | {"expected_stage": 5, "expected_greed": true} |  / ..  |
| `greed_floor_6` | greed_floor / greed-stage | {"expected_stage": 6, "expected_greed": true} |  / ..  |
| `greed_floor_7` | greed_floor / greed-stage | {"expected_stage": 7, "expected_greed": true} |  / ..  |
| `greed_ultra_native` | greed_room / greed-room | {"expected_type": 5, "expected_subtype": 62, "expected_greed": true} |  / ..  |
| `greedier` | progress / greedier | {"expected_greed": true, "expected_difficulty": 3} |  / ..  |
| `greedier_arena` | greed_room / greed-room | {"expected_type": 1, "expected_shape": 4, "expected_greed": true} |  / ..  |
| `greedier_wave` | progress / greed-wave | {"expected_greed": true, "expected_wave": true, "expected_difficulty": 3} |  / ..  |

所有原进度/环境/查询/停录/暂停/退出/隔离/部署恢复用例一起保留，统一入口连续执行。公开RoomSubType的商店等级/稀有/店主、宝物选择/付费/刀片、小Boss、街机/诅咒、挑战/波次/Gideon、图书馆、恶魔/天使变体、Gideon/Rotgut/Beast夹层、Genesis卧室及三种支线出口分别列项。其余公开默认房子类由路线中的实际描述另列，不能以一次Stage命令替代房间内样本。

## 用例计数（不同维度可重复覆盖同一RoomType）

| 维度 | 用例数 |
| --- | --- |
| room_type | 29 |
| off_grid_slot | 19 |
| native_entry | 4 |
| room_shape | 12 |
| public_subtype | 63 |
| special_default_layout | 24 |
| route | 17 |
| dimension | 3 |
| structure_change | 1 |
| progress | 4 |
| greed_room | 24 |
| greed_floor | 6 |

合计206项；退出/回访是各项下的额外阶段，另行记录。
