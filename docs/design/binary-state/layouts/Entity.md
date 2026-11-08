# Entity

由唯一布局源生成。布局编号 100；布局版本 8；原生大小 0x410；未知 1032B；完整：否。

第1步生成链种子，完整Entity字节归属与E0—E9由第3步补齐；此处不是完整Entity实现。

原生字节归属：

| 0x0 u32 vtable【本地】原生虚表，仅当前进程使用（4B；已核） | 0x4 bytes[40] pending_common_prefix【未知】本步骤未核定，保留具名字节缺口，待第3/4步研究（40B；未知） | 0x2C i32 variant【状态】形态选择字段；变化必须更新成员形态（4B；已核） | 0x30 bytes[992] pending_common_suffix【未知】本步骤未核定，保留具名字节缺口，待第3/4步研究（992B；未知） |
|---|---|---|---|

状态二进制布局（总计 4—4B，不含第 9 步记录头）：

块 0：E0（第3步补齐）（world，4—4B）：

| +0x0 i32 variant：形态选择字段；变化必须更新成员形态 |
|---|

块 1：E1（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 2：E2（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 3：E3（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 4：E4（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 5：E5（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 6：E6（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 7：E7（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 8：E8（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

块 9：E9（第3步补齐）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

证据：

- `vtable`：mod/binding/world_layout.json:classes.Entity；docs/research/tx-04-08-st5-static-confirmation.md
- `pending_common_prefix`：docs/design/binary-state/3-entity-common.md；4-entity-classes.md：尚未核定区
- `variant`：mod/binding/world_layout.json:fields.variant；J273 VA 0x6A4A3B 写 +0x2C（imagebase 0x400000）
- `pending_common_suffix`：docs/design/binary-state/3-entity-common.md；4-entity-classes.md：尚未核定区
