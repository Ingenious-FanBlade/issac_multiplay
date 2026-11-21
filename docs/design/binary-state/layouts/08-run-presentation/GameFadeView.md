# GameFadeView

由唯一布局源生成。布局编号 347；布局版本 89；原生大小 0x34；未知 7B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

6F7A80在25B10与25B28等处作分支；必须记录最终有序pass，不能只复制这两组数据后调用完整Game::Render。 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

原生字节归属：

| 0x0 f32[4] primary_rgba【派生】第一整屏覆盖最终RGBA（Game+25B04）；具体下行职责见具名结果，不直接复制该范围。；由GameFadeViewResult固定最终结果承接，不与生产输入重复发送。（16B；已核） | 0x10 bytes[4] unknown_10_13【未知】未取得字段用途或填充证明（4B；未知） | 0x14 bool render_branch【派生】Game+25B18绘制后的分派选择；具体下行职责见具名结果，不直接复制该范围。；由GameFadeViewResult固定最终结果承接，不与生产输入重复发送。（1B；已核） | 0x15 bytes[3] unknown_15_17【未知】未取得字段用途或填充证明（3B；未知） | 0x18 f32[4] secondary_rgba【派生】第二整屏覆盖最终RGBA（Game+25B1C）；具体下行职责见具名结果，不直接复制该范围。；由GameFadeViewResult固定最终结果承接，不与生产输入重复发送。（16B；已核） | 0x28 f32 secondary_progress【权威】Game+25B2C由6F1640清零的生产进度（4B；推断） | 0x2C f32 secondary_speed【权威】Game+25B30由6F1640写入的渐变速率（4B；已核） | 0x30 u32 fadeout_target【权威】Game+25B34退出/转场目标，6F1640校验0..11（4B；已核） |
|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `primary_rgba`：Game+25B04..25B37；J273 6F7A80尾部、6F1640与6F6D30
- `unknown_10_13`：Game+25B04..25B37；J273 6F7A80尾部、6F1640与6F6D30
- `render_branch`：Game+25B04..25B37；J273 6F7A80尾部、6F1640与6F6D30
- `unknown_15_17`：Game+25B04..25B37；J273 6F7A80尾部、6F1640与6F6D30
- `secondary_rgba`：Game+25B04..25B37；J273 6F7A80尾部、6F1640与6F6D30
- `secondary_progress`：Game+25B04..25B37；J273 6F7A80尾部、6F1640与6F6D30
- `secondary_speed`：Game+25B04..25B37；J273 6F7A80尾部、6F1640与6F6D30
- `fadeout_target`：Game+25B04..25B37；J273 6F7A80尾部、6F1640与6F6D30
