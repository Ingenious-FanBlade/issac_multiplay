# PlayerInnateCount

由唯一布局源生成。布局编号 178；布局版本 56；原生大小 0x8；未知 0B；完整：是。

官方多组有效计数归并后的自有8B纯值元素；不是原map<string,InnateItems>节点的原生布局或步长。

原生字节归属：

| 0x0 i32 id【状态】完整ID（4B；已核） | 0x4 i32 count【状态】当前份数（4B；已核） |
|---|---|

状态二进制布局（总计 8—8B，不含第 9 步记录头）：

块 0：展示值（world，8—8B）：

| +0x0 i32 id：完整ID | +0x4 i32 count：当前份数 |
|---|---|

证据：

- `id`：ItemSpoofSystem.h；5-player-p3-p5.md
- `count`：ItemSpoofSystem.h；5-player-p3-p5.md
