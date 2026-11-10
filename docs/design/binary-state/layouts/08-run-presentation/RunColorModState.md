# RunColorModState

由唯一布局源生成。布局编号 235；布局版本 58；原生大小 0x18；未知 0B；完整：是。

原生字节归属：

| 0x0 f32 r【状态】当前最终ColorModifier分量；不是Host渐变目标或速度（4B；已核） | 0x4 f32 g【状态】当前最终ColorModifier分量；不是Host渐变目标或速度（4B；已核） | 0x8 f32 b【状态】当前最终ColorModifier分量；不是Host渐变目标或速度（4B；已核） | 0xC f32 a【状态】当前最终ColorModifier分量；不是Host渐变目标或速度（4B；已核） | 0x10 f32 brightness【状态】当前最终ColorModifier分量；不是Host渐变目标或速度（4B；已核） | 0x14 f32 contrast【状态】当前最终ColorModifier分量；不是Host渐变目标或速度（4B；已核） |
|---|---|---|---|---|---|

状态二进制布局（总计 24—24B，不含第 9 步记录头）：

块 0：当前屏幕颜色结果（world，24—24B）：

| +0x0 f32 r：当前最终ColorModifier分量；不是Host渐变目标或速度 | +0x4 f32 g：当前最终ColorModifier分量；不是Host渐变目标或速度 | +0x8 f32 b：当前最终ColorModifier分量；不是Host渐变目标或速度 | +0xC f32 a：当前最终ColorModifier分量；不是Host渐变目标或速度 | +0x10 f32 brightness：当前最终ColorModifier分量；不是Host渐变目标或速度 | +0x14 f32 contrast：当前最终ColorModifier分量；不是Host渐变目标或速度 |
|---|---|---|---|---|---|

证据：

- `r`：ColorModState.zhl；6F7A80读取Game+66920..66937
- `g`：ColorModState.zhl；6F7A80读取Game+66920..66937
- `b`：ColorModState.zhl；6F7A80读取Game+66920..66937
- `a`：ColorModState.zhl；6F7A80读取Game+66920..66937
- `brightness`：ColorModState.zhl；6F7A80读取Game+66920..66937
- `contrast`：ColorModState.zhl；6F7A80读取Game+66920..66937
