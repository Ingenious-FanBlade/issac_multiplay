# HistoryHUDItem

由唯一布局源生成。布局编号 320；布局版本 60；原生大小 0x28；未知 0B；完整：是。

 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

记录编码预算≤4194304B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 0x0 i32 visual_id【状态】实际显示ID，含633、0x88000位和负ID（4B；已核） | 0x4 Image* image.image【权威】实际Image与本地引用计数control：实际选中图像由第8步资源身份承接（4B；已核） | 0x8 Control* image.strong_control【本地】实际Image与本地引用计数control：本地引用计数／析构控制块（4B；已核） | 0xC History_HistoryItem history_origin【权威】原始获得史：时间、来源池、关卡等（28B；已核） |
|---|---|---|---|

状态二进制布局（总计 6—6B，不含第 9 步记录头）：

块 0：HistoryHUDItem具名当前结果（world，6—6B）：

| +0x0 i32 visual_id：实际显示ID，含633、0x88000位和负ID | +0x4 u16 image：实际Image与本地引用计数control |
|---|---|

证据：

- `visual_id`：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl
- `image.image`：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl；嵌套字段：mod/src/game/client/player/client_player_draw_native.cpp
- `image.strong_control`：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl；嵌套字段：mod/src/game/client/player/client_player_draw_native.cpp
- `history_origin`：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl

逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| docs/design/tx-04-21/stream-catalog.json:3384 | HistoryHUDItem.GetHistoryItem | " " / "HistoryItem" | authority → ；HistoryHUDItem.history_origin（不上线） | 公开Getter读原始History_HistoryItem；Render只需visual_id@0和Image@4，不能把最终visual_id冒充原API原始itemID/trinket/time或完整HistoryItem |
| docs/design/tx-04-21/stream-catalog.json:3385 | HistoryHUDItem.GetItemID | " " / "int" | authority → ；HistoryHUDItem.history_origin（不上线） | 公开Getter读原始History_HistoryItem；Render只需visual_id@0和Image@4，不能把最终visual_id冒充原API原始itemID/trinket/time或完整HistoryItem |
| docs/design/tx-04-21/stream-catalog.json:3386 | HistoryHUDItem.GetRenderOffset | " " / "Vector" | derived → @catalog_result_3386；实际列表index＋本地History布局 | 官方LuaHistoryHUD.cpp按本地columns/iconSize/visibleCount计算，不传Host屏幕坐标 |
| docs/design/tx-04-21/stream-catalog.json:3387 | HistoryHUDItem.GetTime | " " / "int" | authority → ；HistoryHUDItem.history_origin（不上线） | 公开Getter读原始History_HistoryItem；Render只需visual_id@0和Image@4，不能把最终visual_id冒充原API原始itemID/trinket/time或完整HistoryItem |
| docs/design/tx-04-21/stream-catalog.json:3388 | HistoryHUDItem.IsTrinket | " " / "boolean" | authority → ；HistoryHUDItem.history_origin（不上线） | 公开Getter读原始History_HistoryItem；Render只需visual_id@0和Image@4，不能把最终visual_id冒充原API原始itemID/trinket/time或完整HistoryItem |
| docs/design/tx-04-21/stream-catalog.json:3389 | HistoryHUDItem.IsVisible | " " / "boolean" | derived → @catalog_result_3389；实际列表index＋本地History布局 | 官方LuaHistoryHUD.cpp按本地columns/iconSize/visibleCount计算，不传Host屏幕坐标 |
