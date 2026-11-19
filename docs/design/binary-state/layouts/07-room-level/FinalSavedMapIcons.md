# FinalSavedMapIcons

由唯一布局源生成。布局编号 298；布局版本 87；原生大小 0x0；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—12B，不含第 9 步记录头）：

块 0：FinalSavedMapIcons当前结果（world，1—12B）：

| +0x0 u8（C/S自有元数据，无原生偏移） present：不显示／不适用为0；读取不足为Gap，不能折空 | +0x1 SavedMapIconPayload（见该类型状态表）（伴随源，无根对象偏移）（仅finalsavedmapicons_present_present(present)成立时存在） payload：Host按9635B0已核OR/max规则聚合完整可显示保存事实；不发数量／顺序／seed |
|---|---|

证据：



伴随来源（不属于上述原生对象字节）：

- `payload` → `SavedMapIconPayload`：Host按9635B0已核OR/max规则聚合完整可显示保存事实；不发数量／顺序／seed；第7步正常生产结果／具名owner；对应原生账本及room-display-resource-design.md（已核）
