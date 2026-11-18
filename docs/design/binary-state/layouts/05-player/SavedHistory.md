# SavedHistory

由唯一布局源生成。布局编号 166；布局版本 84；原生大小 0x14；未知 8B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

原生字节归属：

| 0x0 vector<HistoryItem> items【权威】SDK给定保存History容器；不借live History的owner证明解释其后8B（12B；SDK声明） | 0xC bytes[8] unknown_c_13【未知】尾部尚未解释；不以对象大小或初始化零值判填充（8B；未知） |
|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `items`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl
- `unknown_c_13`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl
