# ColorParams

由唯一布局源生成。布局编号 11；布局版本 8；原生大小 0x3C；未知 2B；完整：否。

构造函数将两个 duration 初始化为 -1。REP 官方 Lua getters/setters 明确区分 +0x30 Duration 与 +0x34 Lifespan，不能折叠为一个持续时间。

原生字节归属：

| 0x0 i32 priority【状态】颜色参数优先级（4B；已核） | 0x4 f32 color.tint_r【状态】颜色变换值：tint RGBA 的 R 分量（4B；已核） | 0x8 f32 color.tint_g【状态】颜色变换值：tint RGBA 的 G 分量（4B；已核） | 0xC f32 color.tint_b【状态】颜色变换值：tint RGBA 的 B 分量（4B；已核） | 0x10 f32 color.tint_a【状态】颜色变换值：tint RGBA 的 A 分量（4B；已核） | 0x14 f32 color.colorize_r【状态】颜色变换值：colorize RGBA 的 R 分量（4B；已核） | 0x18 f32 color.colorize_g【状态】颜色变换值：colorize RGBA 的 G 分量（4B；已核） | 0x1C f32 color.colorize_b【状态】颜色变换值：colorize RGBA 的 B 分量（4B；已核） | 0x20 f32 color.colorize_a【状态】颜色变换值：colorize RGBA 的 A 分量（4B；已核） | 0x24 f32 color.offset_r【状态】颜色变换值：offset RGB 的 R 分量（4B；已核） | 0x28 f32 color.offset_g【状态】颜色变换值：offset RGB 的 G 分量（4B；已核） | 0x2C f32 color.offset_b【状态】颜色变换值：offset RGB 的 B 分量（4B；已核） | 0x30 i32 duration【状态】duration1；公开 GetDuration/SetDuration（4B；已核） | 0x34 i32 lifespan【状态】duration2；公开 GetLifespan/SetLifespan（4B；已核） | 0x38 bool fadeout【状态】淡出开关（1B；已核） | 0x39 bool shared【状态】共享标志（1B；已核） | 0x3A bytes[2] unknown_3A_3B【未知】SDK 类型大小尾端尚未解释的两个字节（2B；未知） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 58—58B，不含第 9 步记录头）：

块 0：颜色参数（body，58—58B）：

| +0x0 i32 priority：颜色参数优先级 | +0x4 ColorMod color：颜色变换值 | +0x30 i32 duration：duration1；公开 GetDuration/SetDuration | +0x34 i32 lifespan：duration2；公开 GetLifespan/SetLifespan | +0x38 u8 fadeout：淡出开关 | +0x39 u8 shared：共享标志 |
|---|---|---|---|---|---|

证据：

- `priority`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）
- `color.tint_r`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.tint_g`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.tint_b`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.tint_a`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.colorize_r`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.colorize_g`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.colorize_b`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.colorize_a`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.offset_r`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.offset_g`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.offset_b`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `duration`：ColorParams.zhl +0x30；官方 LuaColorParams.cpp 的 GetDuration/SetDuration 直接读取写入 _duration1
- `lifespan`：ColorParams.zhl +0x34；官方 LuaColorParams.cpp 的 GetLifespan/SetLifespan 直接读取写入 _duration2
- `fadeout`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）
- `shared`：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）
- `unknown_3A_3B`：ColorParams.zhl 声明到 +0x39；没有证据可把 +0x3A..+0x3B 认作填充
