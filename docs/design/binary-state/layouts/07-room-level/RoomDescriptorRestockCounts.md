# RoomDescriptorRestockCounts

由唯一布局源生成。布局编号 274；布局版本 84；原生大小 0x8；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

内嵌于descriptor+A4，商店Init前不由ctor获得读取资格。 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 u8 slot0_restock_count【权威】0..255饱和补货次数；同步决定：不发；最终Price承接（1B；已核） | 0x1 u8 slot1_restock_count【权威】0..255饱和补货次数；同步决定：不发；最终Price承接（1B；已核） | 0x2 u8 slot2_restock_count【权威】0..255饱和补货次数；同步决定：不发；最终Price承接（1B；已核） | 0x3 u8 slot3_restock_count【权威】0..255饱和补货次数；同步决定：不发；最终Price承接（1B；已核） | 0x4 u8 slot4_restock_count【权威】0..255饱和补货次数；同步决定：不发；最终Price承接（1B；已核） | 0x5 u8 slot5_restock_count【权威】0..255饱和补货次数；同步决定：不发；最终Price承接（1B；已核） | 0x6 u8 slot6_restock_count【权威】0..255饱和补货次数；同步决定：不发；最终Price承接（1B；已核） | 0x7 u8 slot7_restock_count【权威】0..255饱和补货次数；同步决定：不发；最终Price承接（1B；已核） |
|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `slot0_restock_count`：80AD15..22饱和增；80B6FD movzx
- `slot1_restock_count`：80AD15..22饱和增；80B6FD movzx
- `slot2_restock_count`：80AD15..22饱和增；80B6FD movzx
- `slot3_restock_count`：80AD15..22饱和增；80B6FD movzx
- `slot4_restock_count`：80AD15..22饱和增；80B6FD movzx
- `slot5_restock_count`：80AD15..22饱和增；80B6FD movzx
- `slot6_restock_count`：80AD15..22饱和增；80B6FD movzx
- `slot7_restock_count`：80AD15..22饱和增；80B6FD movzx
