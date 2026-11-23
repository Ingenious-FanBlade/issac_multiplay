# RoomSpawn

由唯一布局源生成。布局编号 266；布局版本 94；原生大小 0x10；未知 3B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 u16 X【权威】Host放置候选；实际Entity/Grid位置另发（2B；SDK声明） | 0x2 u16 Y【权威】Host放置候选（2B；SDK声明） | 0x4 RoomEntry* Entries【权威】候选数组/地址不发（4B；SDK声明） | 0x8 u8 CountEntries【权威】候选数不发；不能按旧Lua int宽读（1B；SDK声明） | 0x9 bytes[3] unknown_09_0B【未知】保持UNKNOWN，不读不发（3B；未知） | 0xC f32 SumWeights【权威】Host抽选权重；不调用PickEntry（4B；SDK声明） |
|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `X`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSpawn.zhl；Wiki RoomConfig_Spawn
- `Y`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSpawn.zhl；Wiki RoomConfig_Spawn
- `Entries`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSpawn.zhl；Wiki RoomConfig_Spawn
- `CountEntries`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSpawn.zhl；Wiki RoomConfig_Spawn
- `unknown_09_0B`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSpawn.zhl；Wiki RoomConfig_Spawn
- `SumWeights`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomSpawn.zhl；Wiki RoomConfig_Spawn

逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| mod/binding/catalog/stream-catalog.json:3067 | RoomConfigSpawn.Entries | null / "RoomConfigEntries" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| mod/binding/catalog/stream-catalog.json:3068 | RoomConfigSpawn.EntryCount | null / "int" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| mod/binding/catalog/stream-catalog.json:3069 | RoomConfigSpawn.PickEntry | " float r " / "RoomConfig Entry" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| mod/binding/catalog/stream-catalog.json:3070 | RoomConfigSpawn.SumWeights | null / "float" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| mod/binding/catalog/stream-catalog.json:3071 | RoomConfigSpawn.X | null / "int" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| mod/binding/catalog/stream-catalog.json:3072 | RoomConfigSpawn.Y | null / "int" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
