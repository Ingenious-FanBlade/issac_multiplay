# RoomSet

由唯一布局源生成。布局编号 265；布局版本 57；原生大小 0x2C；未知 3B；完整：否。

 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 std::string filepath【权威】Host候选库来源，不下行（24B；SDK声明） | 0x18 RoomConfig_Room* configs【权威】Host库/分配，不复制地址或全数组（4B；SDK声明） | 0x1C u32 count【权威】Host库计数，不成为Client活动描述数量（4B；SDK声明） | 0x20 std::map header variantSetMap【本地】原生索引管理；不传树/地址（8B；SDK声明） | 0x28 bool loaded【本地】Host库加载资格；Client资源ready独立（1B；SDK声明） | 0x29 bytes[3] unknown_29_2B【未知】保持UNKNOWN，不读不发（3B；未知） |
|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `filepath`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
- `configs`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
- `count`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
- `variantSetMap`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
- `loaded`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
- `unknown_29_2B`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
