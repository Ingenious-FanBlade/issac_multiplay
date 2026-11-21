# QueueItemData

由唯一布局源生成。布局编号 154；布局版本 90；原生大小 0x10；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

原生字节归属：

| 0x0 ItemConfig* item【资源】仅真实queued饰品配置选择；非饰品折Null（4B；已核） | 0x4 i32 charge【权威】将来获得/使用的charge（4B；已核） | 0x8 u32 flags【换算】只投影bit1黄金饰品份额，其他位Host（4B；已核） | 0xC i32 var_data【权威】未来获得/使用附加数据（4B；已核） |
|---|---|---|---|

状态二进制布局（总计 3—3B，不含第 9 步记录头）：

块 0：展示值（world，3—3B）：

| +0x0 u16 item：仅真实queued饰品配置选择；非饰品折Null | +0x2 u8 flags：只投影bit1黄金饰品份额，其他位Host |
|---|---|

证据：

- `item`：5-player-p3-p5.md
- `charge`：5-player-p3-p5.md
- `flags`：5-player-p3-p5.md
- `var_data`：5-player-p3-p5.md
