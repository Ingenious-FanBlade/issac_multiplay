# HomingLaserBezier

由唯一布局源生成。布局编号 23；布局版本 20；原生大小 0x18；未知 0B；完整：是。

原生字节归属：

| 0x0 f32[6] coefficients【状态】CalculateSamples按24字节步长读取六个浮点槽；具体控制点成员名未知（24B；已核） |
|---|

状态二进制布局（总计 24—24B，不含第 9 步记录头）：

块 0：Bezier六浮点原值（world，24—24B）：

| +0x0 f32[6] coefficients：CalculateSamples按24字节步长读取六个浮点槽；具体控制点成员名未知 |
|---|

证据：

- `coefficients`：docs/design/binary-state/tear-pickup-laser-evidence.md
