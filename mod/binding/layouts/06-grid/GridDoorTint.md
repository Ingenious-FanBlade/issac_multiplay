# GridDoorTint

由唯一布局源生成。布局编号 225；布局版本 94；原生大小 0x2C；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

版本53：原生ColorMod44B全归属，十个分量40B派生，alpha4B转入口布尔1B；普通ColorMod不改。COMISS非有限行为不能由伪代码外推，来源非有限报Gap；墙色完整11float由第7步承接。

原生字节归属：

| 0x0 f32 tint_r【派生】入口通过后必被同world Room.wallColor完整覆盖；旧值不读不发，draw后恢复本地状态（4B；已核） | 0x4 f32 tint_g【派生】入口通过后必被同world Room.wallColor完整覆盖；旧值不读不发，draw后恢复本地状态（4B；已核） | 0x8 f32 tint_b【派生】入口通过后必被同world Room.wallColor完整覆盖；旧值不读不发，draw后恢复本地状态（4B；已核） | 0xC f32 tint_a【换算】有限原alpha的alpha>0入口判断转1B布尔；随后实际颜色全取Room.wallColor，不发原alpha幅值（4B；已核） | 0x10 f32 colorize_r【派生】入口通过后必被同world Room.wallColor完整覆盖；旧值不读不发，draw后恢复本地状态（4B；已核） | 0x14 f32 colorize_g【派生】入口通过后必被同world Room.wallColor完整覆盖；旧值不读不发，draw后恢复本地状态（4B；已核） | 0x18 f32 colorize_b【派生】入口通过后必被同world Room.wallColor完整覆盖；旧值不读不发，draw后恢复本地状态（4B；已核） | 0x1C f32 colorize_a【派生】入口通过后必被同world Room.wallColor完整覆盖；旧值不读不发，draw后恢复本地状态（4B；已核） | 0x20 f32 offset_r【派生】入口通过后必被同world Room.wallColor完整覆盖；旧值不读不发，draw后恢复本地状态（4B；已核） | 0x24 f32 offset_g【派生】入口通过后必被同world Room.wallColor完整覆盖；旧值不读不发，draw后恢复本地状态（4B；已核） | 0x28 f32 offset_b【派生】入口通过后必被同world Room.wallColor完整覆盖；旧值不读不发，draw后恢复本地状态（4B；已核） |
|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 1—1B，不含第 9 步记录头）：

块 0：Door主体与附属绘制入口（body，1—1B）：

| +0x0 u8 tint_a：有限原alpha的alpha>0入口判断转1B布尔；随后实际颜色全取Room.wallColor，不发原alpha幅值 |
|---|

证据：

- `tint_r`：docs/archive/design/binary-state/grid-pressure-door-evidence.md；VA707FA1/707FA8、70800E-708019
- `tint_g`：docs/archive/design/binary-state/grid-pressure-door-evidence.md；VA707FA1/707FA8、70800E-708019
- `tint_b`：docs/archive/design/binary-state/grid-pressure-door-evidence.md；VA707FA1/707FA8、70800E-708019
- `tint_a`：docs/archive/design/binary-state/grid-pressure-door-evidence.md；VA707FA1/707FA8、70800E-708019
- `colorize_r`：docs/archive/design/binary-state/grid-pressure-door-evidence.md；VA707FA1/707FA8、70800E-708019
- `colorize_g`：docs/archive/design/binary-state/grid-pressure-door-evidence.md；VA707FA1/707FA8、70800E-708019
- `colorize_b`：docs/archive/design/binary-state/grid-pressure-door-evidence.md；VA707FA1/707FA8、70800E-708019
- `colorize_a`：docs/archive/design/binary-state/grid-pressure-door-evidence.md；VA707FA1/707FA8、70800E-708019
- `offset_r`：docs/archive/design/binary-state/grid-pressure-door-evidence.md；VA707FA1/707FA8、70800E-708019
- `offset_g`：docs/archive/design/binary-state/grid-pressure-door-evidence.md；VA707FA1/707FA8、70800E-708019
- `offset_b`：docs/archive/design/binary-state/grid-pressure-door-evidence.md；VA707FA1/707FA8、70800E-708019
