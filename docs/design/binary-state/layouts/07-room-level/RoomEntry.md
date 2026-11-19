# RoomEntry

由唯一布局源生成。布局编号 267；布局版本 87；原生大小 0xC；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 u32 type【权威】生成候选type，不是活动Entity结构身份（4B；SDK声明） | 0x4 u16 variant【权威】生成候选，不发（2B；SDK声明） | 0x6 u16 subtype【权威】生成候选，不发（2B；SDK声明） | 0x8 f32 weight【权威】Host抽选，不发（4B；SDK声明） |
|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `type`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomEntry.zhl；Wiki RoomConfig_Entry
- `variant`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomEntry.zhl；Wiki RoomConfig_Entry
- `subtype`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomEntry.zhl；Wiki RoomConfig_Entry
- `weight`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomEntry.zhl；Wiki RoomConfig_Entry

逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| docs/design/tx-04-21/stream-catalog.json:3046 | RoomConfigEntry.Subtype | null / "int" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| docs/design/tx-04-21/stream-catalog.json:3047 | RoomConfigEntry.Type | null / "EntityType" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| docs/design/tx-04-21/stream-catalog.json:3048 | RoomConfigEntry.Variant | null / "int" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| docs/design/tx-04-21/stream-catalog.json:3049 | RoomConfigEntry.Weight | null / "float" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
