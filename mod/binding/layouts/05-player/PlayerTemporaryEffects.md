# PlayerTemporaryEffects

由唯一布局源生成。布局编号 150；布局版本 94；原生大小 0x18；未知 2B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

原生字节归属：

| 0x0 u32 cache_mask【权威】待属性重算mask，不是vptr（4B；已核） | 0x4 vector<TemporaryEffect> entries【容器】完整有界有序身份／count；仅704 cooldown（12B；已核） | 0x10 bool evaluate【权威】Host重算请求（1B；已核） | 0x11 bool disabled【状态】效果查询禁用，独立于列表空值（1B；已核） | 0x12 bytes[2] unknown_12_13【未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判（2B；未知） | 0x14 Player* owner【本地】本地承载Player回指（4B；已核） |
|---|---|---|---|---|---|

状态二进制布局（总计 3—163843B，不含第 9 步记录头）：

块 0：展示值（world，3—163843B）：

| +0x0 u8 disabled：效果查询禁用，独立于列表空值 | +0x1 u16 count + count×PlayerTemporaryEffect（上限16384） entries：完整有界有序身份／count；仅704 cooldown |
|---|---|

证据：

- `cache_mask`：5-player-p6-p8.md
- `entries`：5-player-p6-p8.md
- `evaluate`：5-player-p6-p8.md
- `disabled`：5-player-p6-p8.md
- `unknown_12_13`：5-player-p6-p8.md
- `owner`：5-player-p6-p8.md
