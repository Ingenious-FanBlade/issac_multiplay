# SmeltedTrinketDesc

由唯一布局源生成。布局编号 155；布局版本 89；原生大小 0x4；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

原生字节归属：

| 0x0 i16 normal【状态】普通熔炼份数，signed（2B；已核） | 0x2 i16 golden【状态】黄金熔炼份数，signed（2B；已核） |
|---|---|

状态二进制布局（总计 4—4B，不含第 9 步记录头）：

块 0：展示值（world，4—4B）：

| +0x0 i16 normal：普通熔炼份数，signed | +0x2 i16 golden：黄金熔炼份数，signed |
|---|---|

证据：

- `normal`：5-player-p3-p5.md
- `golden`：5-player-p3-p5.md
