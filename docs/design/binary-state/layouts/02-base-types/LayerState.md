# LayerState

由唯一布局源生成。布局编号 6；布局版本 75；原生大小 0xA0；未知 4B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

LayerState.HasCustomShader 与 HasCustomChampionShader 由 REPENTOGON Anm2Extras.cpp 的 ANM2* / LayerState* 旁表提供，非 J273 +0x31、非 ANM2 +0x108/+0x10C；需第8步旁表布局和资源路径接线。GetSpritesheet 返回实际 Image 对象，+0x98 的资源身份仍需核对名字唯一性/程序图像；+0x08 路径不能替代它，因为 SetSpriteSheet 仅更换 smart pointer、过滤/环绕和 loaded 状态，不改路径字符串。 版本32：sprite_sheet_loaded不上线；J273 ReplaceSpritesheet VA0x407CD0清+30但保留Image，Render经GetSpriteSheet读取实际Image而非该标志。安装实际image_selection与路径元数据分离，不能因路径变更抢先LoadGraphics；本地owner负责loaded，等待／失败不得伪装为空资源。

原生字节归属：

| 0x0 LayerData* layer_data【派生】由所属 ANM2 文件及数组序号定位的资源定义（4B；推断） | 0x4 ANM2* animation_owner【本地】所属 ANM2 回指，不作为网络状态（4B；SDK声明） | 0x8 std::string spritesheet_path【资源】spritesheet路径元数据资源号；与实际Image选择独立，路径变更不授权Client抢先重载（24B；SDK声明） | 0x20 u32 min_filter_mode【状态】缩小过滤模式（4B；SDK声明） | 0x24 u32 mag_filter_mode【状态】放大过滤模式（4B；SDK声明） | 0x28 u32 wrap_s_mode【状态】S 方向环绕模式（4B；SDK声明） | 0x2C u32 wrap_t_mode【状态】T 方向环绕模式（4B；SDK声明） | 0x30 bool sprite_sheet_loaded【本地】本地图层资源加载／重载维护标志；ReplaceSpritesheet可置false同时保留旧Image，不跨进程复制，也不按Host原值覆盖Client加载结果（1B；SDK声明） | 0x31 u8 unknown_31【未知】J273 LoadGraphics 将该字节清零；具体语义未明，绝非已证填充（1B；未知） | 0x32 bool flip_x【状态】水平翻转（1B；SDK声明） | 0x33 bool flip_y【状态】垂直翻转（1B；SDK声明） | 0x34 f32 size.x【状态】图层尺寸：X 分量（4B；SDK声明） | 0x38 f32 size.y【状态】图层尺寸：Y 分量（4B；SDK声明） | 0x3C f32 rotation【状态】图层旋转角度（4B；SDK声明） | 0x40 f32 position.x【状态】图层位置：X 分量（4B；SDK声明） | 0x44 f32 position.y【状态】图层位置：Y 分量（4B；SDK声明） | 0x48 f32 color.tint_r【状态】图层 ColorMod：tint RGBA 的 R 分量（4B；SDK声明） | 0x4C f32 color.tint_g【状态】图层 ColorMod：tint RGBA 的 G 分量（4B；SDK声明） | 0x50 f32 color.tint_b【状态】图层 ColorMod：tint RGBA 的 B 分量（4B；SDK声明） | 0x54 f32 color.tint_a【状态】图层 ColorMod：tint RGBA 的 A 分量（4B；SDK声明） | 0x58 f32 color.colorize_r【状态】图层 ColorMod：colorize RGBA 的 R 分量（4B；SDK声明） | 0x5C f32 color.colorize_g【状态】图层 ColorMod：colorize RGBA 的 G 分量（4B；SDK声明） | 0x60 f32 color.colorize_b【状态】图层 ColorMod：colorize RGBA 的 B 分量（4B；SDK声明） | 0x64 f32 color.colorize_a【状态】图层 ColorMod：colorize RGBA 的 A 分量（4B；SDK声明） | 0x68 f32 color.offset_r【状态】图层 ColorMod：offset RGB 的 R 分量（4B；SDK声明） | 0x6C f32 color.offset_g【状态】图层 ColorMod：offset RGB 的 G 分量（4B；SDK声明） | 0x70 f32 color.offset_b【状态】图层 ColorMod：offset RGB 的 B 分量（4B；SDK声明） | 0x74 bool visible【状态】图层显示开关（1B；SDK声明） | 0x75 bytes[3] unknown_75_77【未知】SDK 布局间未说明的三个字节；不认作填充（3B；未知） | 0x78 u32 blend_mode.equation【状态】图层混合模式的五 DWORD 值：混合方程（4B；已核） | 0x7C u32 blend_mode.rgb_source_factor【状态】图层混合模式的五 DWORD 值：RGB 源因子（4B；已核） | 0x80 u32 blend_mode.rgb_destination_factor【状态】图层混合模式的五 DWORD 值：RGB 目标因子（4B；已核） | 0x84 u32 blend_mode.alpha_source_factor【状态】图层混合模式的五 DWORD 值：Alpha 源因子（4B；已核） | 0x88 u32 blend_mode.alpha_destination_factor【状态】图层混合模式的五 DWORD 值：Alpha 目标因子（4B；已核） | 0x8C u32 render_flags【状态】图层渲染位标志（4B；SDK声明） | 0x90 f32 crop_offset.x【状态】图层裁剪偏移：X 分量（4B；SDK声明） | 0x94 f32 crop_offset.y【状态】图层裁剪偏移：Y 分量（4B；SDK声明） | 0x98 KAGE_SmartPointer_ImageBase image_selection【资源】当前实际ImageBase选择，Null使用0xFFFF；Host由ImageBase._name形成资源身份，Client重建引用；重名／程序图像身份仍待第8步核（8B；推断） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 119—119B，不含第 9 步记录头）：

块 0：贴图资源（body，4—4B）：

| +0x0 u16 spritesheet_path：spritesheet路径元数据资源号；与实际Image选择独立，路径变更不授权Client抢先重载 | +0x2 u16 image_selection：当前实际ImageBase选择，Null使用0xFFFF；Host由ImageBase._name形成资源身份，Client重建引用；重名／程序图像身份仍待第8步核 |
|---|---|

块 1：图层外观（body，115—115B）：

| +0x0 u32 min_filter_mode：缩小过滤模式 | +0x4 u32 mag_filter_mode：放大过滤模式 | +0x8 u32 wrap_s_mode：S 方向环绕模式 | +0xC u32 wrap_t_mode：T 方向环绕模式 | +0x10 u8 flip_x：水平翻转 | +0x11 u8 flip_y：垂直翻转 | +0x12 Vector size：图层尺寸 | +0x1A f32 rotation：图层旋转角度 | +0x1E Vector position：图层位置 | +0x26 ColorMod color：图层 ColorMod | +0x52 u8 visible：图层显示开关 | +0x53 BlendMode blend_mode：图层混合模式的五 DWORD 值 | +0x67 u32 render_flags：图层渲染位标志 | +0x6B Vector crop_offset：图层裁剪偏移 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

证据：

- `layer_data`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `animation_owner`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `spritesheet_path`：LayerState.zhl +0x08 std::string；LuaSprite.cpp GetSpritesheetPath 直接返回 GetSpritesheetPath().c_str()
- `min_filter_mode`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `mag_filter_mode`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `wrap_s_mode`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `wrap_t_mode`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics
- `sprite_sheet_loaded`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md
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
