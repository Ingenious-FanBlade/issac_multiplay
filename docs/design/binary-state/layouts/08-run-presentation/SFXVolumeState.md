# SFXVolumeState

由唯一布局源生成。布局编号 437；布局版本 75；原生大小 0x4；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

D8窄值视图，非Manager完整对象。根定位由具名音频owner负责，offset0对应Manager+2A170。处理器只登记合同，实际采集与安装待第8/10/13步接线。

原生字节归属：

| 0x0 f32 volume_modifier【换算】RE-10当前SFX全局乘数；允许负的有限值，包括-10；只拒NaN／无穷（4B；已核） |
|---|

状态二进制布局（总计 4—4B，不含第 9 步记录头）：

块 0：D8当前SFX乘数（world，4—4B）：

| +0x0 f32 volume_modifier：RE-10当前SFX全局乘数；允许负的有限值，包括-10；只拒NaN／无穷 |
|---|

证据：

- `volume_modifier`：docs/design/binary-state/client-display-delegation.md 第4.6、8.4节（Q8／D1—D8）；RE-10/audio-and-schedule.md；908340；Manager+2A170
