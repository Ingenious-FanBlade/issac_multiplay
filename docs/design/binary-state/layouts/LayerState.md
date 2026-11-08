# LayerState

由唯一布局源生成。布局编号 6；布局版本 20；原生大小 0xA0；未知 4B；完整：否。

LayerState.HasCustomShader 与 HasCustomChampionShader 由 REPENTOGON Anm2Extras.cpp 的 ANM2* / LayerState* 旁表提供，非 J273 +0x31、非 ANM2 +0x108/+0x10C；需第8步旁表布局和资源路径接线。GetSpritesheet 返回实际 Image 对象，+0x98 的资源身份仍需核对名字唯一性/程序图像；+0x08 路径不能替代它，因为 SetSpriteSheet 仅更换 smart pointer、过滤/环绕和 loaded 状态，不改路径字符串。

原生字节归属：

| 0x0 LayerData* layer_data【派生】由所属 ANM2 文件及数组序号定位的资源定义（4B；推断） | 0x4 ANM2* animation_owner【本地】所属 ANM2 回指，不作为网络状态（4B；SDK声明） | 0x8 std::string spritesheet_path【资源】当前 spritesheet 路径字符串，转换为图像资源号（24B；SDK声明） | 0x20 u32 min_filter_mode【状态】缩小过滤模式（4B；SDK声明） | 0x24 u32 mag_filter_mode【状态】放大过滤模式（4B；SDK声明） | 0x28 u32 wrap_s_mode【状态】S 方向环绕模式（4B；SDK声明） | 0x2C u32 wrap_t_mode【状态】T 方向环绕模式（4B；SDK声明） | 0x30 bool sprite_sheet_loaded【状态】原生图层加载标志；保守按状态保留，客户端须先完成相应资源准备（1B；SDK声明） | 0x31 u8 unknown_31【未知】J273 LoadGraphics 将该字节清零；具体语义未明，绝非已证填充（1B；未知） | 0x32 bool flip_x【状态】水平翻转（1B；SDK声明） | 0x33 bool flip_y【状态】垂直翻转（1B；SDK声明） | 0x34 f32 size.x【状态】图层尺寸：X 分量（4B；SDK声明） | 0x38 f32 size.y【状态】图层尺寸：Y 分量（4B；SDK声明） | 0x3C f32 rotation【状态】图层旋转角度（4B；SDK声明） | 0x40 f32 position.x【状态】图层位置：X 分量（4B；SDK声明） | 0x44 f32 position.y【状态】图层位置：Y 分量（4B；SDK声明） | 0x48 f32 color.tint_r【状态】图层 ColorMod：tint RGBA 的 R 分量（4B；SDK声明） | 0x4C f32 color.tint_g【状态】图层 ColorMod：tint RGBA 的 G 分量（4B；SDK声明） | 0x50 f32 color.tint_b【状态】图层 ColorMod：tint RGBA 的 B 分量（4B；SDK声明） | 0x54 f32 color.tint_a【状态】图层 ColorMod：tint RGBA 的 A 分量（4B；SDK声明） | 0x58 f32 color.colorize_r【状态】图层 ColorMod：colorize RGBA 的 R 分量（4B；SDK声明） | 0x5C f32 color.colorize_g【状态】图层 ColorMod：colorize RGBA 的 G 分量（4B；SDK声明） | 0x60 f32 color.colorize_b【状态】图层 ColorMod：colorize RGBA 的 B 分量（4B；SDK声明） | 0x64 f32 color.colorize_a【状态】图层 ColorMod：colorize RGBA 的 A 分量（4B；SDK声明） | 0x68 f32 color.offset_r【状态】图层 ColorMod：offset RGB 的 R 分量（4B；SDK声明） | 0x6C f32 color.offset_g【状态】图层 ColorMod：offset RGB 的 G 分量（4B；SDK声明） | 0x70 f32 color.offset_b【状态】图层 ColorMod：offset RGB 的 B 分量（4B；SDK声明） | 0x74 bool visible【状态】图层显示开关（1B；SDK声明） | 0x75 bytes[3] unknown_75_77【未知】SDK 布局间未说明的三个字节；不认作填充（3B；未知） | 0x78 u32 blend_mode.equation【状态】图层混合模式的五 DWORD 值：混合方程（4B；已核） | 0x7C u32 blend_mode.rgb_source_factor【状态】图层混合模式的五 DWORD 值：RGB 源因子（4B；已核） | 0x80 u32 blend_mode.rgb_destination_factor【状态】图层混合模式的五 DWORD 值：RGB 目标因子（4B；已核） | 0x84 u32 blend_mode.alpha_source_factor【状态】图层混合模式的五 DWORD 值：Alpha 源因子（4B；已核） | 0x88 u32 blend_mode.alpha_destination_factor【状态】图层混合模式的五 DWORD 值：Alpha 目标因子（4B；已核） | 0x8C u32 render_flags【状态】图层渲染位标志（4B；SDK声明） | 0x90 f32 crop_offset.x【状态】图层裁剪偏移：X 分量（4B；SDK声明） | 0x94 f32 crop_offset.y【状态】图层裁剪偏移：Y 分量（4B；SDK声明） | 0x98 KAGE_SmartPointer_ImageBase image_selection【资源】实际 ImageBase 选择；Host 由 ImageBase._name 形成资源身份，客户端重建引用；稳定唯一性仍待第8步核（8B；推断） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 120—120B，不含第 9 步记录头）：

块 0：贴图资源（body，4—4B）：

| +0x0 u16 spritesheet_path：当前 spritesheet 路径字符串，转换为图像资源号 | +0x2 u16 image_selection：实际 ImageBase 选择；Host 由 ImageBase._name 形成资源身份，客户端重建引用；稳定唯一性仍待第8步核 |
|---|---|

块 1：图层外观（body，116—116B）：

| +0x0 u32 min_filter_mode：缩小过滤模式 | +0x4 u32 mag_filter_mode：放大过滤模式 | +0x8 u32 wrap_s_mode：S 方向环绕模式 | +0xC u32 wrap_t_mode：T 方向环绕模式 | +0x10 u8 sprite_sheet_loaded：原生图层加载标志；保守按状态保留，客户端须先完成相应资源准备 | +0x11 u8 flip_x：水平翻转 | +0x12 u8 flip_y：垂直翻转 | +0x13 Vector size：图层尺寸 | +0x1B f32 rotation：图层旋转角度 | +0x1F Vector position：图层位置 | +0x27 ColorMod color：图层 ColorMod | +0x53 u8 visible：图层显示开关 | +0x54 BlendMode blend_mode：图层混合模式的五 DWORD 值 | +0x68 u32 render_flags：图层渲染位标志 | +0x6C Vector crop_offset：图层裁剪偏移 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

证据：

- `layer_data`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `animation_owner`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `spritesheet_path`：LayerState.zhl +0x08 std::string；LuaSprite.cpp GetSpritesheetPath 直接返回 GetSpritesheetPath().c_str()
- `min_filter_mode`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `mag_filter_mode`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `wrap_s_mode`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `wrap_t_mode`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `sprite_sheet_loaded`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `unknown_31`：J273 VA 0x407DA0 LoadGraphics 内 VA 0x407F7A 写 LayerState+0x31=0；imagebase 0x400000
- `flip_x`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `flip_y`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `size.x`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `size.y`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `rotation`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `position.x`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `position.y`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `color.tint_r`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.tint_g`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.tint_b`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.tint_a`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.colorize_r`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.colorize_g`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.colorize_b`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.colorize_a`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.offset_r`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.offset_g`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `color.offset_b`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；嵌套字段：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读
- `visible`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `unknown_75_77`：LayerState.zhl 在 visible +0x74 后以 BlendMode +0x78 对齐；没有不变证据
- `blend_mode.equation`：J273 VA 0x9E17B0（RVA 0x5E17B0，imagebase 0x400000）BlendMode::GetMode；官方 BlendMode.zhl；嵌套字段：J273 VA 0x9E17B0（RVA 0x5E17B0，imagebase 0x400000）BlendMode::GetMode；官方 BlendMode.zhl；Global.zhl:3340-3362，LuaBlendMode.cpp 校验合法枚举
- `blend_mode.rgb_source_factor`：J273 VA 0x9E17B0（RVA 0x5E17B0，imagebase 0x400000）BlendMode::GetMode；官方 BlendMode.zhl；嵌套字段：J273 VA 0x9E17B0（RVA 0x5E17B0，imagebase 0x400000）BlendMode::GetMode；官方 BlendMode.zhl；Global.zhl:3340-3362，LuaBlendMode.cpp 校验合法枚举
- `blend_mode.rgb_destination_factor`：J273 VA 0x9E17B0（RVA 0x5E17B0，imagebase 0x400000）BlendMode::GetMode；官方 BlendMode.zhl；嵌套字段：J273 VA 0x9E17B0（RVA 0x5E17B0，imagebase 0x400000）BlendMode::GetMode；官方 BlendMode.zhl；Global.zhl:3340-3362，LuaBlendMode.cpp 校验合法枚举
- `blend_mode.alpha_source_factor`：J273 VA 0x9E17B0（RVA 0x5E17B0，imagebase 0x400000）BlendMode::GetMode；官方 BlendMode.zhl；嵌套字段：J273 VA 0x9E17B0（RVA 0x5E17B0，imagebase 0x400000）BlendMode::GetMode；官方 BlendMode.zhl；Global.zhl:3340-3362，LuaBlendMode.cpp 校验合法枚举
- `blend_mode.alpha_destination_factor`：J273 VA 0x9E17B0（RVA 0x5E17B0，imagebase 0x400000）BlendMode::GetMode；官方 BlendMode.zhl；嵌套字段：J273 VA 0x9E17B0（RVA 0x5E17B0，imagebase 0x400000）BlendMode::GetMode；官方 BlendMode.zhl；Global.zhl:3340-3362，LuaBlendMode.cpp 校验合法枚举
- `render_flags`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `crop_offset.x`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `crop_offset.y`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；嵌套字段：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl
- `image_selection`：SmartPointer.zhl：+0 image、+4 refcount control block；KAGE_Graphics_Image.zhl：ImageBase::_name 在 +0x3C；LayerState::SetSpriteSheet 会替换整个 smart pointer
