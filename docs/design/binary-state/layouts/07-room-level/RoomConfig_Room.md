# RoomConfig_Room

由唯一布局源生成。布局编号 263；布局版本 60；原生大小 0x5C；未知 0B；完整：否。

 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 i32 StageId【状态】RoomConfigVisual.stage_id；Home正面显示依赖（4B；已核） | 0x4 i32 Mode【权威】Host定位元数据，不发；官方Lua修正原Wiki userdata（4B；SDK声明） | 0x8 i32 Type【状态】RoomConfigVisual.type；独立于StageId（4B；已核） | 0xC u32 Variant【权威】Host定位/生成；实际资源由结果承接（4B；SDK声明） | 0x10 i32 Subtype【状态】RoomConfigVisual.subtype；地图和subtype89 Light（4B；已核） | 0x14 std::string Name【权威】不复制string；实际显示文字归Presentation，配置调试名不发（24B；SDK声明） | 0x2C i32 Difficulty【权威】Host选房条件（4B；SDK声明） | 0x30 f32 InitialWeight【权威】Host选房初始权重（4B；SDK声明） | 0x34 f32 Weight【权威】Host可变权重；不影响15B显示投影时不发布（4B；SDK声明） | 0x38 u32 Doors【权威】生成候选门；实际八门与AllowedDoors由R3/L1承接（4B；SDK声明） | 0x3C bitset32 QuestDoors【权威】生成/任务门；实际门对象由Grid和R3承接（4B；SDK声明） | 0x40 RoomSpawn* Spawns【权威】不读取/发送生成候选图或地址（4B；SDK声明） | 0x44 u16 SpawnCount【权威】生成数组长度不下行（2B；SDK声明） | 0x46 i8 Width【状态】RoomConfigVisual.width；配置内域尺寸，非R0实际grid extent（1B；SDK声明） | 0x47 i8 Height【状态】RoomConfigVisual.height（1B；SDK声明） | 0x48 i32 Shape【换算】RoomConfigVisual.shape:u8，先检查1..12，不能截断；正式投影：先检查原i32在1..12再窄化，不按低字节截断（4B；已核） | 0x4C bitset32 Flags【权威】完整位语义未穷尽；实际Entity/Grid/背景结果承接（4B；SDK声明） | 0x50 RoomConfig_Room*[2] MergedData【权威】Host合并生成图；不递归下发（8B；SDK声明） | 0x58 u32 OriginalVariant【权威】Host定位元数据/生成，不进入显示投影（4B；SDK声明） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 15—15B，不含第 9 步记录头）：

块 0：不可变RoomConfigVisual（15B）（world，15—15B）：

| +0x0 i32 StageId：RoomConfigVisual.stage_id；Home正面显示依赖 | +0x4 i32 Type：RoomConfigVisual.type；独立于StageId | +0x8 i32 Subtype：RoomConfigVisual.subtype；地图和subtype89 Light | +0xC i8 Width：RoomConfigVisual.width；配置内域尺寸，非R0实际grid extent | +0xD i8 Height：RoomConfigVisual.height | +0xE u8 Shape：RoomConfigVisual.shape:u8，先检查1..12，不能截断；正式投影：先检查原i32在1..12再窄化，不按低字节截断 |
|---|---|---|---|---|---|

证据：

- `StageId`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `Mode`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `Type`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `Variant`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `Subtype`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `Name`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `Difficulty`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `InitialWeight`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `Weight`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `Doors`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `QuestDoors`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `Spawns`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `SpawnCount`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `Width`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `Height`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `Shape`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `Flags`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `MergedData`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0
- `OriginalVariant`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl；Wiki RoomConfig_Room；J273 80276C/9666E3/9635B0/816BB0

逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| docs/design/tx-04-21/stream-catalog.json:3050 | RoomConfigRoom.Difficulty | null / "int" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| docs/design/tx-04-21/stream-catalog.json:3051 | RoomConfigRoom.Doors | null / "int" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| docs/design/tx-04-21/stream-catalog.json:3053 | RoomConfigRoom.InitialWeight | null / "float" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| docs/design/tx-04-21/stream-catalog.json:3054 | RoomConfigRoom.Mode | null / "int" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| docs/design/tx-04-21/stream-catalog.json:3055 | RoomConfigRoom.Mode | null / "int" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| docs/design/tx-04-21/stream-catalog.json:3056 | RoomConfigRoom.Name | null / "string" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| docs/design/tx-04-21/stream-catalog.json:3057 | RoomConfigRoom.OriginalVariant | null / "int" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| docs/design/tx-04-21/stream-catalog.json:3059 | RoomConfigRoom.SpawnCount | null / "int" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| docs/design/tx-04-21/stream-catalog.json:3060 | RoomConfigRoom.Spawns | null / "RoomConfigSpawns" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| docs/design/tx-04-21/stream-catalog.json:3064 | RoomConfigRoom.Variant | null / "int" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| docs/design/tx-04-21/stream-catalog.json:3065 | RoomConfigRoom.Weight | null / "float" | authority → ；Host配置生成／定位 | 生成候选／权重／定位元数据留Host；当前显示只承接15B配置及实际Entity/Grid/Presentation结果，不遍历Spawn/Merged图或执行命令；原签名及旧归属保持。 |
| docs/design/tx-04-21/stream-catalog.json:3052 | RoomConfigRoom.Height | null / "int" | derived → Height；RoomConfig_Room.Height | 保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选；原stream行3052归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 正式字段只承接该API必要的当前显示投影；不保证被裁原数值Getter相等。 |
| docs/design/tx-04-21/stream-catalog.json:3058 | RoomConfigRoom.Shape | null / "RoomShape" | derived → Shape；RoomConfig_Room.Shape | 保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选；原stream行3058归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 正式字段只承接该API必要的当前显示投影；不保证被裁原数值Getter相等。 |
| docs/design/tx-04-21/stream-catalog.json:3061 | RoomConfigRoom.StageID | null / "int" | derived → StageId；RoomConfig_Room.StageId | 保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选；原stream行3061归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 正式字段只承接该API必要的当前显示投影；不保证被裁原数值Getter相等。 |
| docs/design/tx-04-21/stream-catalog.json:3062 | RoomConfigRoom.Subtype | null / "int" | derived → Subtype；RoomConfig_Room.Subtype | 保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选；原stream行3062归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 正式字段只承接该API必要的当前显示投影；不保证被裁原数值Getter相等。 |
| docs/design/tx-04-21/stream-catalog.json:3063 | RoomConfigRoom.Type | null / "RoomType" | derived → Type；RoomConfig_Room.Type | 保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选；原stream行3063归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 正式字段只承接该API必要的当前显示投影；不保证被裁原数值Getter相等。 |
| docs/design/tx-04-21/stream-catalog.json:3066 | RoomConfigRoom.Width | null / "int" | derived → Width；RoomConfig_Room.Width | 保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选；原stream行3066归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 正式字段只承接该API必要的当前显示投影；不保证被裁原数值Getter相等。 |
