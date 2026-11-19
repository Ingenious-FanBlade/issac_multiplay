# HomingLaserBezier

由唯一布局源生成。布局编号 23；布局版本 87；原生大小 0x18；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

原生字节归属：

| 0x0 f32[6] coefficients【权威】CalculateSamples按24B步长使用六f32系数槽；具体控制点成员名未知。Host采样输入，无展示消费者，不传输（24B；已核） |
|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

块 0：Bezier六浮点原值（Host输入，不发送，保留空块号）（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

证据：

- `coefficients`：docs/design/binary-state/tear-pickup-laser-evidence.md；docs/design/binary-state/laser-homing-synchronization-review.md
