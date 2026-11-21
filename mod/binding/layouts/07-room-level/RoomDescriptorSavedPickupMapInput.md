# RoomDescriptorSavedPickupMapInput

由唯一布局源生成。布局编号 271；布局版本 90；原生大小 0x78；未知 70B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

仅type5已构造有效保存元素；未知70B，通用EntitySaveState仍74B；不是生产176变更或完整保存格式。 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 i32 type【派生】仅type5的场景输入视图，非新原生类；同步决定：Host选分支，原type不发（4B；已核） | 0x4 i32[2] variant_subtype【权威】实际图标类别和帧选择输入；未消费subtype分支不读其原值；同步决定：Host聚合kind/mask和max frame，原variant/subtype不发（8B；已核） | 0xC bytes[8] storage1_2【未知】原保存体未解释；同步决定：不读不发，UNKNOWN保持（8B；未知） | 0x14 u32 map_exclusion_storage3【权威】type5地图排除谓词，仅检查是否非零；不宣称所有保存变体都具有同一语义；同步决定：Host过滤，谓词原值不发（4B；已核） | 0x18 bytes[8] storage4_5【未知】原保存体未解释；同步决定：不读不发（8B；未知） | 0x20 i16 grid_spawn_index【权威】原生保存/ESSM身份，非地图输入；同步决定：留Host（2B；已核） | 0x22 bytes[2] bool_storage【未知】通用保存体未解释；同步决定：不读不发（2B；未知） | 0x24 bytes[4] storage6【未知】通用保存体未解释；同步决定：不读不发（4B；未知） | 0x28 f32 target_position.x【权威】仅与原点(0,0)比较，按原生短路读实际需要分量；同步决定：Host过滤，坐标不发：X 分量（4B；已核） | 0x2C f32 target_position.y【权威】仅与原点(0,0)比较，按原生短路读实际需要分量；同步决定：Host过滤，坐标不发：Y 分量（4B；已核） | 0x30 bytes[4] storage7【未知】通用保存体未解释/ESSM复用；同步决定：不读不发（4B；未知） | 0x34 u32[2] init_drop_seeds【权威】恢复生产的seed；同步决定：留Host（8B；已核） | 0x3C i32[2] spawner_type_variant【权威】恢复生产来源；同步决定：留Host（8B；已核） | 0x44 bytes[12] unknown44【未知】通用保存体未知；同步决定：不读不发（12B；未知） | 0x50 bytes[8] float_storage【未知】通用保存体未知；同步决定：不读不发（8B；未知） | 0x58 bytes[4] unknown58【未知】通用保存体未知；同步决定：不读不发（4B；未知） | 0x5C bytes[4] storage8【未知】通用保存体未知；同步决定：不读不发（4B；未知） | 0x60 bytes[1] byte_storage【未知】通用保存体未知；同步决定：不读不发（1B；未知） | 0x61 bytes[11] unknown61【未知】通用保存体未知；同步决定：不读不发（11B；未知） | 0x6C strong pair nested_save_holder【本地】递归保存体与控制块本地生命周期；同步决定：不发地址/控制块，不作为实体引用（8B；SDK声明） | 0x74 bytes[4] unknown74【未知】通用保存体未知；同步决定：不读不发（4B；未知） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `type`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取
- `variant_subtype`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取
- `storage1_2`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取
- `map_exclusion_storage3`：9645AB；源码未知业务名不影响本consumer的明确比较
- `storage4_5`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取
- `grid_spawn_index`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取
- `bool_storage`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取
- `storage6`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取
- `target_position.x`：9645B5/9645C7；9635B0；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `target_position.y`：9645B5/9645C7；9635B0；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `storage7`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取
- `init_drop_seeds`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取
- `spawner_type_variant`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取
- `unknown44`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取
- `float_storage`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取
- `unknown58`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取
- `storage8`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取
- `byte_storage`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取
- `unknown61`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取
- `nested_save_holder`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取
- `unknown74`：EntitySaveState.zhl；布局176既有账本；9635B0/9645AB/9645B5/9645C7地图读取

逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| mod/binding/catalog/stream-catalog.json:1 | EntitiesSaveState.GetB1 | " " / "boolean" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；原stream行1归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:2 | EntitiesSaveState.GetB2 | " " / "boolean" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；原stream行2归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:3 | EntitiesSaveState.GetDropSeed | " " / "int" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；原stream行3归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:4 | EntitiesSaveState.GetF1 | " " / "float" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；原stream行4归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:5 | EntitiesSaveState.GetF2 | " " / "float" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；原stream行5归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:6 | EntitiesSaveState.GetGridSpawnIdx | " " / "int" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；原stream行6归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:7 | EntitiesSaveState.GetI1 | " " / "int" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；原stream行7归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:8 | EntitiesSaveState.GetI2 | " " / "int" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；原stream行8归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:9 | EntitiesSaveState.GetI3 | " " / "int" | derived → @catalog_9；L2.icon_mask/selected_frames; raw saved payload HostOnly | Host只读相关地图type／variant／subtype／价格排除／位置谓词，OR/max后只发最终图标；其它保存用途留Host；原stream行9归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:10 | EntitiesSaveState.GetI4 | " " / "int" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；原stream行10归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:11 | EntitiesSaveState.GetI5 | " " / "int" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；原stream行11归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:12 | EntitiesSaveState.GetI6 | " " / "int" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；原stream行12归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:13 | EntitiesSaveState.GetI7 | " " / "int" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；原stream行13归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:14 | EntitiesSaveState.GetI8 | " " / "int" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；原stream行14归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:15 | EntitiesSaveState.GetInitSeed | " " / "int" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；原stream行15归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:16 | EntitiesSaveState.GetPos | " " / "Vector" | derived → @catalog_16；L2.icon_mask/selected_frames; raw saved payload HostOnly | Host只读相关地图type／variant／subtype／价格排除／位置谓词，OR/max后只发最终图标；其它保存用途留Host；原stream行16归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:17 | EntitiesSaveState.GetSpawnerType | " " / "EntityType" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；原stream行17归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:18 | EntitiesSaveState.GetSpawnerVariant | " " / "int" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；原stream行18归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:19 | EntitiesSaveState.GetSubType | " " / "int" | derived → @catalog_19；L2.icon_mask/selected_frames; raw saved payload HostOnly | Host只读相关地图type／variant／subtype／价格排除／位置谓词，OR/max后只发最终图标；其它保存用途留Host；原stream行19归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:20 | EntitiesSaveState.GetType | " " / "EntityType" | derived → @catalog_20；L2.icon_mask/selected_frames; raw saved payload HostOnly | Host只读相关地图type／variant／subtype／价格排除／位置谓词，OR/max后只发最终图标；其它保存用途留Host；原stream行20归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:21 | EntitiesSaveState.GetU1 | " " / "int" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 离房恢复payload留Host；实际恢复后的Entity／显示结果独立同步，不复制保存槽位或重放恢复；原stream行21归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:22 | EntitiesSaveState.GetVariant | " " / "int" | derived → @catalog_22；L2.icon_mask/selected_frames; raw saved payload HostOnly | Host只读相关地图type／variant／subtype／价格排除／位置谓词，OR/max后只发最终图标；其它保存用途留Host；原stream行22归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:23 | EntitiesSaveState.SetB1 | " boolean B1 " / "void" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；原stream行23归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:24 | EntitiesSaveState.SetB2 | " boolean B2 " / "void" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；原stream行24归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:25 | EntitiesSaveState.SetF1 | " float Num " / "void" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；原stream行25归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:26 | EntitiesSaveState.SetF2 | " float Num " / "void" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；原stream行26归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:27 | EntitiesSaveState.SetI1 | " int Num " / "void" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；原stream行27归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:28 | EntitiesSaveState.SetI2 | " int Num " / "void" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；原stream行28归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:29 | EntitiesSaveState.SetI3 | " int Num " / "void" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；原stream行29归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:30 | EntitiesSaveState.SetI4 | " int Num " / "void" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；原stream行30归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:31 | EntitiesSaveState.SetI5 | " int Num " / "void" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；原stream行31归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:32 | EntitiesSaveState.SetI6 | " int Num " / "void" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；原stream行32归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:33 | EntitiesSaveState.SetI7 | " int Num " / "void" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；原stream行33归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:34 | EntitiesSaveState.SetI8 | " int Num " / "void" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；原stream行34归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:35 | EntitiesSaveState.SetPos | " Vector Pos " / "void" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；原stream行35归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:36 | EntitiesSaveState.SetSubType | " int SubType " / "void" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；原stream行36归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:37 | EntitiesSaveState.SetType | " EntityType Type " / "void" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；原stream行37归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:38 | EntitiesSaveState.SetU1 | " int Num " / "void" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；原stream行38归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:39 | EntitiesSaveState.SetVariant | " int Variant " / "void" | authority → ；@HostSavedPayload -> @CurrentEntity/Presentation | 保存体修改命令只在Host；不网络回放，不为读取调用Setter或临时写回原值；原stream行39归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:40 | EntitiesSaveStateVector.Clear | " " / "void" | authority → ；@HostSavedContainer | 先ESSM清理batch再vector.clear，有生命周期副作用，禁止为采集／Client安装执行；原stream行40归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:41 | EntitiesSaveStateVector.Get | " int Index " / "EntitiesSaveState" | derived → @catalog_41；L2.icon_mask/selected_frames | 正式来源仅扫描有效元素中地图必要投影并聚合，不调用Lua、不发送Get结果对象或保存数组；Get为0-based；原stream行41归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:42 | EntitiesSaveStateVector.GetByType | " EntityType Type, int Variant = 0, int SubType = 0 " / "EntitiesSaveState" | derived → @catalog_42；L2.icon_mask/selected_frames | 正式来源仅扫描有效元素中地图必要投影并聚合，不调用Lua、不发送Get结果对象或保存数组；Get为0-based；原stream行42归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:43 | EntitiesSaveStateVector.__len | " " / "int" | authority → ；@HostSavedContainer | 容器头／长度／保存项成员关系属于Host恢复，不独立下行；读取不得产生状态或跨窗口借用；原stream行43归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:622 | GridEntitiesSaveStateVector.Get | " int Index " / "GridEntityDesc" | derived → @catalog_622；L2.icon_mask/selected_frames | 正式来源仅扫描有效元素中地图必要投影并聚合，不调用Lua、不发送Get结果对象或保存数组；Get为0-based；原stream行622归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:623 | GridEntitiesSaveStateVector.GetByType | " GridEntityType Type " / "GridEntityDesc" | derived → @catalog_623；L2.icon_mask/selected_frames | 正式来源仅扫描有效元素中地图必要投影并聚合，不调用Lua、不发送Get结果对象或保存数组；Get为0-based；原stream行623归属ReplicatedState原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
| mod/binding/catalog/stream-catalog.json:624 | GridEntitiesSaveStateVector.__len | " " / "int" | authority → ；@HostSavedContainer | 容器头／长度／保存项成员关系属于Host恢复，不独立下行；读取不得产生状态或跨窗口借用；原stream行624归属AuthorityOnly原样保留；第7步当前处置，具体缓存／adapter尚待接线。 |
