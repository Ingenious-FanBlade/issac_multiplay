# AfterImageFrame

由唯一布局源生成。布局编号 156；布局版本 60；原生大小 0xC；未知 0B；完整：是。

原生字节归属：

| 0x0 i32 duration【状态】原生Render直接转alpha，不由Client递减（4B；已核） | 0x4 f32 position[0].x【状态】每份实际残影位置：X 分量（4B；已核） | 0x8 f32 position[0].y【状态】每份实际残影位置：Y 分量（4B；已核） |
|---|---|---|

状态二进制布局（总计 12—12B，不含第 9 步记录头）：

块 0：展示值（world，12—12B）：

| +0x0 i32 duration：原生Render直接转alpha，不由Client递减 | +0x4 Vector position：每份实际残影位置 |
|---|---|

证据：

- `duration`：5-player-p6-p8.md
- `position[0].x`：5-player-p6-p8.md；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `position[0].y`：5-player-p6-p8.md；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
