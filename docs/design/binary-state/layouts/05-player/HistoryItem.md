# HistoryItem

由唯一布局源生成。布局编号 158；布局版本 60；原生大小 0x1C；未知 3B；完整：否。

原生字节归属：

| 0x0 i32 time【权威】原始获得时点，不能替代HUD图像顺序（4B；已核） | 0x4 bool trinket【权威】原始历史类别（1B；已核） | 0x5 bytes[3] unknown_5_7【未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判（3B；未知） | 0x8 i32[5] history_values【权威】item_id／stage／stage_type／room_type／pool（20B；已核） |
|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `time`：5-player-p6-p8.md
- `trinket`：5-player-p6-p8.md
- `unknown_5_7`：5-player-p6-p8.md
- `history_values`：5-player-p6-p8.md
