# RunGameStateFlags

由唯一布局源生成。布局编号 236；布局版本 90；原生大小 0x8；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

Game+25AC0。仅1B显示投影，不用SetStateFlag、不恢复原版Online状态。

原生字节归属：

| 0x0 u32 low_flags【权威】bit0..31生成／交易／奖励／历史标志；不因GetStateFlag公开而整体发送（4B；已核） | 0x4 u32 ascent_display【换算】只提取全u64 bit48即本字bit16为1B显示布尔；其余bit32..47/49..63仍Host，不原样写回（4B；已核） |
|---|---|

状态二进制布局（总计 1—1B，不含第 9 步记录头）：

块 0：Ascent显示分支（world，1—1B）：

| +0x0 u8 ascent_display：只提取全u64 bit48即本字bit16为1B显示布尔；其余bit32..47/49..63仍Host，不原样写回 |
|---|

证据：

- `low_flags`：原版GameStateFlag.md；6F51C0保存53位
- `ascent_display`：909E60／90A040／967DB0／971D80检查Game+25AC4&10000h
