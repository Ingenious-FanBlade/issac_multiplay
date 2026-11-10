# ColorMod

由唯一布局源生成。布局编号 2；布局版本 57；原生大小 0x2C；未知 0B；完整：是。

Color 映射到 ColorMod。REP Lua 取色接口逐数组读 _tint、_colorize、_offset；39 个具名常量及 EmberFade 是全局静态颜色，均不映射为实例字段。

原生字节归属：

| 0x0 f32 tint_r【状态】tint RGBA 的 R 分量（4B；已核） | 0x4 f32 tint_g【状态】tint RGBA 的 G 分量（4B；已核） | 0x8 f32 tint_b【状态】tint RGBA 的 B 分量（4B；已核） | 0xC f32 tint_a【状态】tint RGBA 的 A 分量（4B；已核） | 0x10 f32 colorize_r【状态】colorize RGBA 的 R 分量（4B；已核） | 0x14 f32 colorize_g【状态】colorize RGBA 的 G 分量（4B；已核） | 0x18 f32 colorize_b【状态】colorize RGBA 的 B 分量（4B；已核） | 0x1C f32 colorize_a【状态】colorize RGBA 的 A 分量（4B；已核） | 0x20 f32 offset_r【状态】offset RGB 的 R 分量（4B；已核） | 0x24 f32 offset_g【状态】offset RGB 的 G 分量（4B；已核） | 0x28 f32 offset_b【状态】offset RGB 的 B 分量（4B；已核） |
|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 44—44B，不含第 9 步记录头）：

块 0：颜色变换（body，44—44B）：

| +0x0 f32 tint_r：tint RGBA 的 R 分量 | +0x4 f32 tint_g：tint RGBA 的 G 分量 | +0x8 f32 tint_b：tint RGBA 的 B 分量 | +0xC f32 tint_a：tint RGBA 的 A 分量 | +0x10 f32 colorize_r：colorize RGBA 的 R 分量 | +0x14 f32 colorize_g：colorize RGBA 的 G 分量 | +0x18 f32 colorize_b：colorize RGBA 的 B 分量 | +0x1C f32 colorize_a：colorize RGBA 的 A 分量 | +0x20 f32 offset_r：offset RGB 的 R 分量 | +0x24 f32 offset_g：offset RGB 的 G 分量 | +0x28 f32 offset_b：offset RGB 的 B 分量 |
|---|---|---|---|---|---|---|---|---|---|---|

证据：

- `tint_r`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `tint_g`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `tint_b`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `tint_a`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `colorize_r`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `colorize_g`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `colorize_b`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `colorize_a`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `offset_r`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `offset_g`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `offset_b`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
