# HistoryItem

由唯一布局源生成。布局编号 158；布局版本 90；原生大小 0x1C；未知 3B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

客机原生 HistoryHUD 由显示委托在本机重算，条目按原值复制；3字节未知填充不上线，客机构造时清零。

原生字节归属：

| 0x0 i32 time【状态】原始获得时点，不能替代HUD图像顺序（4B；已核） | 0x4 bool trinket【状态】原始历史类别（1B；已核） | 0x5 bytes[3] unknown_5_7【未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判（3B；未知） | 0x8 i32[5] history_values【状态】item_id／stage／stage_type／room_type／pool（20B；已核） |
|---|---|---|---|

状态二进制布局（总计 25—25B，不含第 9 步记录头）：

块 0：展示值（world，25—25B）：

| +0x0 i32 time：原始获得时点，不能替代HUD图像顺序 | +0x4 u8 trinket：原始历史类别 | +0x5 i32[5] history_values：item_id／stage／stage_type／room_type／pool |
|---|---|---|

证据：

- `time`：5-player-p6-p8.md；J273 82FF40／8304B0（HistoryHUD按History条目+4类别、+8道具ID重算图像）；client-display-delegation RE-1 update-order 2.2
- `trinket`：5-player-p6-p8.md；J273 82FF40／8304B0（HistoryHUD按History条目+4类别、+8道具ID重算图像）；client-display-delegation RE-1 update-order 2.2
- `unknown_5_7`：5-player-p6-p8.md
- `history_values`：5-player-p6-p8.md；J273 82FF40／8304B0（HistoryHUD按History条目+4类别、+8道具ID重算图像）；client-display-delegation RE-1 update-order 2.2
