# EffectTrailNode

由唯一布局源生成。布局编号 26；布局版本 23；原生大小 0x40；未知 0B；完整：否。

**范围仅为已核前缀，不是动态对象分配大小；具体对象仍需完整布局。**

0x40仅为当前消费者已访问到的记录前缀；实际producer分配大小和记录步长尚未闭合，不能宣称完整动态对象。+0x38只保留32位绘制参数原值，不命名为Rotation。真实读取/重建须先核定记录布局，不能据现有适配器的64B注释直接安装。

原生字节归属：

| 0x0 f32 trail_position.x【状态】轨迹节点位置：X 分量（4B；推断） | 0x4 f32 trail_position.y【状态】轨迹节点位置：Y 分量（4B；推断） | 0x8 f32 trail_color.tint_r【状态】轨迹节点颜色：tint RGBA 的 R 分量（4B；已核） | 0xC f32 trail_color.tint_g【状态】轨迹节点颜色：tint RGBA 的 G 分量（4B；已核） | 0x10 f32 trail_color.tint_b【状态】轨迹节点颜色：tint RGBA 的 B 分量（4B；已核） | 0x14 f32 trail_color.tint_a【状态】轨迹节点颜色：tint RGBA 的 A 分量（4B；已核） | 0x18 f32 trail_color.colorize_r【状态】轨迹节点颜色：colorize RGBA 的 R 分量（4B；已核） | 0x1C f32 trail_color.colorize_g【状态】轨迹节点颜色：colorize RGBA 的 G 分量（4B；已核） | 0x20 f32 trail_color.colorize_b【状态】轨迹节点颜色：colorize RGBA 的 B 分量（4B；已核） | 0x24 f32 trail_color.colorize_a【状态】轨迹节点颜色：colorize RGBA 的 A 分量（4B；已核） | 0x28 f32 trail_color.offset_r【状态】轨迹节点颜色：offset RGB 的 R 分量（4B；已核） | 0x2C f32 trail_color.offset_g【状态】轨迹节点颜色：offset RGB 的 G 分量（4B；已核） | 0x30 f32 trail_color.offset_b【状态】轨迹节点颜色：offset RGB 的 B 分量（4B；已核） | 0x34 f32 trail_scale【状态】节点缩放原值（4B；已核） | 0x38 u32 trail_render_word【状态】经XMM3透传到绘制记录的32位模式；尚未证明为Rotation，不施加浮点有限值约束（4B；推断） | 0x3C f32 trail_alpha【状态】节点透明度原值（4B；已核） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 64—64B，不含第 9 步记录头）：

块 0：轨迹节点（world，64—64B）：

| +0x0 Vector trail_position：轨迹节点位置 | +0x8 ColorMod trail_color：轨迹节点颜色 | +0x34 f32 trail_scale：节点缩放原值 | +0x38 u32 trail_render_word：经XMM3透传到绘制记录的32位模式；尚未证明为Rotation，不施加浮点有限值约束 | +0x3C f32 trail_alpha：节点透明度原值 |
|---|---|---|---|---|

证据：

- `trail_position.x`：docs/design/binary-state/slot-projectile-effect-evidence.md；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `trail_position.y`：docs/design/binary-state/slot-projectile-effect-evidence.md；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `trail_color.tint_r`：docs/design/binary-state/slot-projectile-effect-evidence.md；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `trail_color.tint_g`：docs/design/binary-state/slot-projectile-effect-evidence.md；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `trail_color.tint_b`：docs/design/binary-state/slot-projectile-effect-evidence.md；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `trail_color.tint_a`：docs/design/binary-state/slot-projectile-effect-evidence.md；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `trail_color.colorize_r`：docs/design/binary-state/slot-projectile-effect-evidence.md；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `trail_color.colorize_g`：docs/design/binary-state/slot-projectile-effect-evidence.md；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `trail_color.colorize_b`：docs/design/binary-state/slot-projectile-effect-evidence.md；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `trail_color.colorize_a`：docs/design/binary-state/slot-projectile-effect-evidence.md；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `trail_color.offset_r`：docs/design/binary-state/slot-projectile-effect-evidence.md；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `trail_color.offset_g`：docs/design/binary-state/slot-projectile-effect-evidence.md；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `trail_color.offset_b`：docs/design/binary-state/slot-projectile-effect-evidence.md；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `trail_scale`：docs/design/binary-state/slot-projectile-effect-evidence.md
- `trail_render_word`：docs/design/binary-state/slot-projectile-effect-evidence.md
- `trail_alpha`：docs/design/binary-state/slot-projectile-effect-evidence.md
