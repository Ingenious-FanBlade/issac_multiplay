# PlayerCostumeMap

由唯一布局源生成。布局编号 151；布局版本 90；原生大小 0x10；未知 3B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

原生字节归属：

| 0x0 i32 index【换算】非负真实costume下标；负值只消费符号（4B；已核） | 0x4 i32 layer_id【换算】index非负时读取；负时不读残留并编码0（4B；已核） | 0x8 i32 priority【权威】层竞争生产输入，最终map已同步（4B；已核） | 0xC bool body【状态】备份绘制选择身体／头部比例，负index也保留（1B；已核） | 0xD bytes[3] unknown_d_f【未知】尾部尚未解释；不以对象大小或初始化零值判填充（3B；未知） |
|---|---|---|---|---|

状态二进制布局（总计 7—7B，不含第 9 步记录头）：

块 0：展示值（world，7—7B）：

| +0x0 u16 index：非负真实costume下标；负值只消费符号 | +0x2 i32 layer_id：index非负时读取；负时不读残留并编码0 | +0x6 u8 body：备份绘制选择身体／头部比例，负index也保留 |
|---|---|---|

证据：

- `index`：5-player-p6-p8.md
- `layer_id`：5-player-p6-p8.md
- `priority`：5-player-p6-p8.md
- `body`：5-player-p6-p8.md
- `unknown_d_f`：5-player-p6-p8.md
