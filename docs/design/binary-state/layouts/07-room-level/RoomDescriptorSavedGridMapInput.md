# RoomDescriptorSavedGridMapInput

由唯一布局源生成。布局编号 272；布局版本 84；原生大小 0x20；未知 3B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

只提取地图楼梯结果；不恢复第6步当前Grid字段发送决定。 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 i32 type【权威】只判断是否GRID_STAIRS=18；同步决定：有效type18设置L2.icon_mask.bit14；不另发楼梯字段／常量帧（4B；已核） | 0x4 i32/u32[5] other_generation_fields【权威】variant/state/spawnCount/spawnSeed/varData；在此保存地图视图不消费；同步决定：留Host；不复制保存体（20B；已核） | 0x18 u8 initialized【权威】保存初始化输入，地图不使用；同步决定：留Host（1B；已核） | 0x19 bytes[3] unknown19【未知】GridEntityDesc既有3B未知；同步决定：不读不发；不因外层Host而消除（3B；未知） | 0x1C u32 variable_seed【权威】保存生成seed，地图不使用；同步决定：留Host（4B；已核） |
|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `type`：964B6D..964BA2
- `other_generation_fields`：GridEntityDesc.zhl；既有198布局；6EBB10
- `initialized`：GridEntityDesc.zhl
- `unknown19`：Grid公共账本
- `variable_seed`：GridEntityDesc.zhl
