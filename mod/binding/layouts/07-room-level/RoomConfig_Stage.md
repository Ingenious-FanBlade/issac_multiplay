# RoomConfig_Stage

由唯一布局源生成。布局编号 264；布局版本 94；原生大小 0xC4；未知 6B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 u32 id【权威】库身份；已选RoomConfigVisual.stage_id独立提供（4B；SDK声明） | 0x4 std::string displayName【权威】实际已选显示文字归Presentation；不整库复制（24B；SDK声明） | 0x1C std::string playerSpot【权威】实际选用图像/Sprite归转场资源；不按stage重新选（24B；SDK声明） | 0x34 std::string bossSpot【权威】实际boss intro资源由Presentation提供（24B；SDK声明） | 0x4C std::string suffix【权威】所选实际Sprite/Image/具名显示上下文承接；不传生产库（24B；SDK声明） | 0x64 std::string rooms[0].filepath【权威】Normal/Greed候选库不下发；GetRoomSet可能加载：Host候选库来源，不下行（24B；SDK声明） | 0x7C RoomConfig_Room* rooms[0].configs【权威】Normal/Greed候选库不下发；GetRoomSet可能加载：Host库/分配，不复制地址或全数组（4B；SDK声明） | 0x80 u32 rooms[0].count【权威】Normal/Greed候选库不下发；GetRoomSet可能加载：Host库计数，不成为Client活动描述数量（4B；SDK声明） | 0x84 std::map header rooms[0].variantSetMap【本地】Normal/Greed候选库不下发；GetRoomSet可能加载：原生索引管理；不传树/地址（8B；SDK声明） | 0x8C bool rooms[0].loaded【本地】Normal/Greed候选库不下发；GetRoomSet可能加载：Host库加载资格；Client资源ready独立（1B；SDK声明） | 0x8D bytes[3] rooms[0].unknown_29_2B【未知】Normal/Greed候选库不下发；GetRoomSet可能加载：保持UNKNOWN，不读不发（3B；未知） | 0x90 std::string rooms[1].filepath【权威】Normal/Greed候选库不下发；GetRoomSet可能加载：Host候选库来源，不下行（24B；SDK声明） | 0xA8 RoomConfig_Room* rooms[1].configs【权威】Normal/Greed候选库不下发；GetRoomSet可能加载：Host库/分配，不复制地址或全数组（4B；SDK声明） | 0xAC u32 rooms[1].count【权威】Normal/Greed候选库不下发；GetRoomSet可能加载：Host库计数，不成为Client活动描述数量（4B；SDK声明） | 0xB0 std::map header rooms[1].variantSetMap【本地】Normal/Greed候选库不下发；GetRoomSet可能加载：原生索引管理；不传树/地址（8B；SDK声明） | 0xB8 bool rooms[1].loaded【本地】Normal/Greed候选库不下发；GetRoomSet可能加载：Host库加载资格；Client资源ready独立（1B；SDK声明） | 0xB9 bytes[3] rooms[1].unknown_29_2B【未知】Normal/Greed候选库不下发；GetRoomSet可能加载：保持UNKNOWN，不读不发（3B；未知） | 0xBC i32 musicId【权威】生产选择；已选音乐意图由音频owner承接（4B；SDK声明） | 0xC0 i32 backdrop【权威】默认背景选择；实际纹理/分支由Room显示结果承接（4B；SDK声明） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `id`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage
- `displayName`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage
- `playerSpot`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage
- `bossSpot`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage
- `suffix`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage
- `rooms[0].filepath`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
- `rooms[0].configs`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
- `rooms[0].count`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
- `rooms[0].variantSetMap`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
- `rooms[0].loaded`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
- `rooms[0].unknown_29_2B`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
- `rooms[1].filepath`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
- `rooms[1].configs`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
- `rooms[1].count`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
- `rooms[1].variantSetMap`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
- `rooms[1].loaded`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
- `rooms[1].unknown_29_2B`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage；嵌套字段：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSet.zhl；LuaRoomConfigStage.cpp::GetRoomSet
- `musicId`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage
- `backdrop`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigStage.zhl；LuaInterfaces/Room/LuaRoomConfigStage.cpp；Wiki RoomConfigStage

逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| mod/binding/catalog/stream-catalog.json:3073 | RoomConfigStage.GetBackdrop | " " / "BackdropType" | authority → ；Host配置／生成上下文 | 原生配置定位、生成选择或原始seed／历史留Host；当前世界已选配置、实际背景／音频／图像由正式唯一owner承接，不执行该查询补源或在Client重演；原始行号及签名保持。 |
| mod/binding/catalog/stream-catalog.json:3074 | RoomConfigStage.GetBossSpot | " " / "string" | derived → @catalog_3074；第7步RoomDescriptor/RoomConfig资源核对（本轮未完成） | 本次已选文字／头像／资源由Presentation承接，不传整份Stage库或按stage重新选择；配置声明／职责已经第7步核定，实际Presentation来源与adapter归第8/10/13步；不保证原库Getter任意调用等价。 |
| mod/binding/catalog/stream-catalog.json:3075 | RoomConfigStage.GetDisplayName | " " / "string" | derived → @catalog_3075；第7步RoomDescriptor/RoomConfig资源核对（本轮未完成） | 本次已选文字／头像／资源由Presentation承接，不传整份Stage库或按stage重新选择；配置声明／职责已经第7步核定，实际Presentation来源与adapter归第8/10/13步；不保证原库Getter任意调用等价。 |
| mod/binding/catalog/stream-catalog.json:3076 | RoomConfigStage.GetID | " " / "int" | authority → ；Host配置／生成上下文 | 原生配置定位、生成选择或原始seed／历史留Host；当前世界已选配置、实际背景／音频／图像由正式唯一owner承接，不执行该查询补源或在Client重演；原始行号及签名保持。 |
| mod/binding/catalog/stream-catalog.json:3077 | RoomConfigStage.GetMusic | " " / "Music" | authority → ；Host配置／生成上下文 | 原生配置定位、生成选择或原始seed／历史留Host；当前世界已选配置、实际背景／音频／图像由正式唯一owner承接，不执行该查询补源或在Client重演；原始行号及签名保持。 |
| mod/binding/catalog/stream-catalog.json:3078 | RoomConfigStage.GetPlayerSpot | " " / "string" | derived → @catalog_3078；第7步RoomDescriptor/RoomConfig资源核对（本轮未完成） | 本次已选文字／头像／资源由Presentation承接，不传整份Stage库或按stage重新选择；配置声明／职责已经第7步核定，实际Presentation来源与adapter归第8/10/13步；不保证原库Getter任意调用等价。 |
| mod/binding/catalog/stream-catalog.json:3079 | RoomConfigStage.GetRoomSet | " int Mode " / "RoomConfigSet" | authority → ；Host command | 命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行；原stream行3079归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:3080 | RoomConfigStage.GetSuffix | " " / "string" | derived → @catalog_3080；第7步RoomDescriptor/RoomConfig资源核对（本轮未完成） | 本次已选文字／头像／资源由Presentation承接，不传整份Stage库或按stage重新选择；配置声明／职责已经第7步核定，实际Presentation来源与adapter归第8/10/13步；不保证原库Getter任意调用等价。 |
| mod/binding/catalog/stream-catalog.json:3081 | RoomConfigStage.GetXMLName | " " / "string" | derived → @catalog_3081；第7步RoomDescriptor/RoomConfig资源核对（本轮未完成） | 本次已选文字／头像／资源由Presentation承接，不传整份Stage库或按stage重新选择；配置声明／职责已经第7步核定，实际Presentation来源与adapter归第8/10/13步；不保证原库Getter任意调用等价。 |
| mod/binding/catalog/stream-catalog.json:3082 | RoomConfigStage.IsLoaded | " int mode = 0 " / "boolean" | not_applicable → ；Host库加载／Client独立资源ready | 本次已选文字／头像／资源由Presentation承接，不传整份Stage库或按stage重新选择；配置声明／职责已经第7步核定，实际Presentation来源与adapter归第8/10/13步；不保证原库Getter任意调用等价。 |
| mod/binding/catalog/stream-catalog.json:3083 | RoomConfigStage.SetBackdrop | " BackdropType Backdrop " / "void" | authority → ；Host command | 命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行；原stream行3083归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:3084 | RoomConfigStage.SetBossSpot | " string PngFilename " / "void" | authority → ；Host command | 命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行；原stream行3084归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:3085 | RoomConfigStage.SetDisplayName | " string Name " / "void" | authority → ；Host command | 命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行；原stream行3085归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:3086 | RoomConfigStage.SetMusic | " Music Music " / "void" | authority → ；Host command | 命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行；原stream行3086归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:3087 | RoomConfigStage.SetPlayerSpot | " string PngFilename " / "void" | authority → ；Host command | 命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行；原stream行3087归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:3088 | RoomConfigStage.SetSuffix | " string Suffix " / "void" | authority → ；Host command | 命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行；原stream行3088归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:3089 | RoomConfigStage.SetXMLName | " string name " / "void" | authority → ；Host command | 命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行；原stream行3089归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:3090 | RoomConfigStage.Unload | " " / "void" | authority → ；Host command | 命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行；原stream行3090归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
