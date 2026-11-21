# FXPose

由唯一布局源生成。布局编号 244；布局版本 90；原生大小 0x14；未知 2B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 f32 position.x【状态】实际有序纹理偏移；同步决定：发送两个f32：X 分量（4B；已核） | 0x4 f32 position.y【状态】实际有序纹理偏移；同步决定：发送两个f32：Y 分量（4B；已核） | 0x8 f32 velocity.x【权威】位置推进／Host颜色采样查询输入；当前draw不读这8B；同步决定：不发；Client不运行颜色采样查询补生产，最终Entity颜色由Entity域承接：X 分量（4B；推断） | 0xC f32 velocity.y【权威】位置推进／Host颜色采样查询输入；当前draw不读这8B；同步决定：不发；Client不运行颜色采样查询补生产，最终Entity颜色由Entity域承接：Y 分量（4B；推断） | 0x10 u8[2] flip_x_y【状态】实际flip条件；同步决定：两个规范bool或两位mask；不复制尾部未知2B（2B；已核） | 0x12 bytes[2] unknown_12_13【未知】声明边界内未解释尾部，不能称填充；同步决定：不读不发；已读消费者不提供该范围的必要显示值，语义UNKNOWN保留，不称填充（2B；未知） |
|---|---|---|---|---|---|

状态二进制布局（总计 10—10B，不含第 9 步记录头）：

块 0：FXPose当前结果（world，10—10B）：

| +0x0 Vector position：实际有序纹理偏移；同步决定：发送两个f32 | +0x8 u8[2] flip_x_y：实际flip条件；同步决定：两个规范bool或两位mask；不复制尾部未知2B |
|---|---|

证据：

- `position.x`：96C620/96DB20/96CE20；原生0x14步长；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `position.y`：96C620/96DB20/96CE20；原生0x14步长；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `velocity.x`：96BF40位置生产；96E3C0采样pass4图片时读FXPose+8/+C；96C620/96DB20/96CE20实际draw；原账本标签：已读消费者；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `velocity.y`：96BF40位置生产；96E3C0采样pass4图片时读FXPose+8/+C；96C620/96DB20/96CE20实际draw；原账本标签：已读消费者；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `flip_x_y`：96C620/96DB20/96CE20；原生0x14步长
- `unknown_12_13`：96C620/96DB20/96CE20；原生0x14步长；本轮补读：9703B0按20B构造只写16B＋2flip；96C620/96DB20/96CE20不读+12..13，仍非padding证明
