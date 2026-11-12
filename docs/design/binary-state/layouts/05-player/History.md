# History

由唯一布局源生成。布局编号 157；布局版本 71；原生大小 0x14；未知 3B；完整：否。

原生字节归属：

| 0x0 vector<HistoryItem> items【权威】原始获得史，时间／层／房／pool不作为显示输入发送（12B；已核） | 0xC bool mutation_disabled【权威】Add／Remove禁止位，不是显示禁用开关（1B；已核） | 0xD bytes[3] unknown_d_f【未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判（3B；未知） | 0x10 Player* owner【本地】74E5E1将自身写入live History末字（4B；已核） |
|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `items`：5-player-p6-p8.md
- `mutation_disabled`：5-player-p6-p8.md
- `unknown_d_f`：5-player-p6-p8.md
- `owner`：5-player-p6-p8.md
