# HistoryHUD

由唯一布局源生成。布局编号 322；布局版本 90；原生大小 0x2C；未知 11B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

原生字节归属：

| 0x0 Entity_Player* players[0].player【引用】两个原生显示列；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：逐身体HistoryHUDPlayer和ControlGroupHudSelection；保持负visual_id及原顺序：这列最终列表的实际身体（4B；已核） | 0x4 bytes[4] players[0].unknown_4_7【未知】两个原生显示列；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：逐身体HistoryHUDPlayer和ControlGroupHudSelection；保持负visual_id及原顺序：未取得字段用途或填充证明（4B；未知） | 0x8 vector<HistoryHUDItem> players[0].items【容器】两个原生显示列；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：逐身体HistoryHUDPlayer和ControlGroupHudSelection；保持负visual_id及原顺序：完整有序图像列表（12B；已核） | 0x14 Entity_Player* players[1].player【引用】两个原生显示列；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：逐身体HistoryHUDPlayer和ControlGroupHudSelection；保持负visual_id及原顺序：这列最终列表的实际身体（4B；已核） | 0x18 bytes[4] players[1].unknown_4_7【未知】两个原生显示列；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：逐身体HistoryHUDPlayer和ControlGroupHudSelection；保持负visual_id及原顺序：未取得字段用途或填充证明（4B；未知） | 0x1C vector<HistoryHUDItem> players[1].items【容器】两个原生显示列；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：逐身体HistoryHUDPlayer和ControlGroupHudSelection；保持负visual_id及原顺序：完整有序图像列表（12B；已核） | 0x28 bool initialized【本地】97AE90首次RecomputeAll维护标志（1B；已核） | 0x29 bytes[3] unknown_29_2B【未知】未取得字段用途或填充证明（3B；未知） |
|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `players[0].player`：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl；嵌套字段：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl
- `players[0].unknown_4_7`：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl；嵌套字段：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl
- `players[0].items`：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl；嵌套字段：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl
- `players[1].player`：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl；嵌套字段：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl
- `players[1].unknown_4_7`：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl；嵌套字段：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl
- `players[1].items`：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl；嵌套字段：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl
- `initialized`：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl
- `unknown_29_2B`：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl

逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| mod/binding/catalog/stream-catalog.json:3377 | HistoryHUD.GetCollectibleOffsets | " int PlayerIndex, CollectibleType[] IDs = nil, boolean IncludeNotVisible = false " / "Vector[]" | not_applicable → ；最终展示列表投影＋本地位置/裁切，不承诺旧Getter全语义 | 旧带原始ID/饰品种类过滤的Getter依赖History原字段；不为兼容API恢复获得史，也不把最终visual_id冒充原始ID |
| mod/binding/catalog/stream-catalog.json:3378 | HistoryHUD.GetCollectibles | " int PlayerIndex, CollectibleType[] IDs = nil, boolean IncludeNotVisible = false " / "HistoryHUDItems[]" | not_applicable → ；最终展示列表投影＋本地位置/裁切，不承诺旧Getter全语义 | 旧带原始ID/饰品种类过滤的Getter依赖History原字段；不为兼容API恢复获得史，也不把最终visual_id冒充原始ID |
| mod/binding/catalog/stream-catalog.json:3379 | HistoryHUD.GetItems | " int PlayerIndex, boolean IncludeNotVisible = false " / "HistoryHUDItems[]" | not_applicable → ；最终展示列表投影＋本地位置/裁切，不承诺旧Getter全语义 | 旧带原始ID/饰品种类过滤的Getter依赖History原字段；不为兼容API恢复获得史，也不把最终visual_id冒充原始ID |
| mod/binding/catalog/stream-catalog.json:3380 | HistoryHUD.GetPlayer | " int PlayerIndex " / "EntityPlayer" | derived → @catalog_result_3380；HistoryHUD.players[index0..1].player | 实际两个原生显示列的StateRef；不能按参与者序号推断 |
| mod/binding/catalog/stream-catalog.json:3381 | HistoryHUD.GetPosition | " " / "Vector" | derived → @catalog_result_3381；本地History布局 | 官方GetPosition按本地屏幕/HUDOffset/ExtraHUDStyle计算，不发Host屏幕坐标 |
| mod/binding/catalog/stream-catalog.json:3382 | HistoryHUD.GetTrinketOffsets | " int PlayerIndex, TrinketType[] IDs = nil, boolean IncludeNotVisible = false " / "Vector[]" | not_applicable → ；最终展示列表投影＋本地位置/裁切，不承诺旧Getter全语义 | 旧带原始ID/饰品种类过滤的Getter依赖History原字段；不为兼容API恢复获得史，也不把最终visual_id冒充原始ID |
| mod/binding/catalog/stream-catalog.json:3383 | HistoryHUD.GetTrinkets | " int PlayerIndex, TrinketType[] IDs = nil, boolean IncludeNotVisible = false " / "HistoryHUDItems[]" | not_applicable → ；最终展示列表投影＋本地位置/裁切，不承诺旧Getter全语义 | 旧带原始ID/饰品种类过滤的Getter依赖History原字段；不为兼容API恢复获得史，也不把最终visual_id冒充原始ID |
