# PocketItem

由唯一布局源生成。布局编号 153；布局版本 58；原生大小 0x8；未知 0B；完整：是。

保留4槽顺序；空id仍保留原u32 type，非空只解释0/1/2；active的id是slot+1。

原生字节归属：

| 0x0 i32 id【状态】按type解释，active是slot+1；0为空（4B；已核） | 0x4 u32 type【状态】空槽也保留真实type，不按主动槽的空策略清零（4B；已核） |
|---|---|

状态二进制布局（总计 8—8B，不含第 9 步记录头）：

块 0：展示值（world，8—8B）：

| +0x0 i32 id：按type解释，active是slot+1；0为空 | +0x4 u32 type：空槽也保留真实type，不按主动槽的空策略清零 |
|---|---|

证据：

- `id`：5-player-p3-p5.md
- `type`：5-player-p3-p5.md
