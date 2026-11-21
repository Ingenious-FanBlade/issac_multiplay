# 第8步 Presentation：原生字节核对账本

**2026-11-10修订5**：[同步职责复核](presentation-source-synchronization-review.md)逐来源区分状态／事件／资源、Host生产与本地隔离。最终贡献替代Nightmare路线／场景帧等重复输入；最终voice gain替代layer mask／混音树／ducking，全局pitch不重复。个人后端失败不广播world Stop。仍50类型／视图，所有原生范围与UNKNOWN数量保持，旧决定留JSON历史；生产版本57，未构建或测试。

**2026-11-10修订4**：[音频来源与未决](presentation-audio-sources.md)补核sample、持续voice、两组音乐／层与jingle；纠正MusicStream+0C为后端volume、pitch在+14、+2C非游标。新增MusicLayerDefinition，共50类型／视图；直属UNKNOWN分别MusicStream24→4B、MusicEntry72→2B、SoundSample16→4B、SoundEffect30→2B、SoundEffects4→0B。旧范围／错误留JSON历史；其余孔及逻辑包络／异步结束／seek／视频音轨未知保持。生产版本57，未接线、构建或测试。

**2026-11-10修订3**：[资源／屏幕／转场来源](presentation-resource-screen-sources.md)补齐正常Host来源合同；新增BossOverlay及定义、Nightmare队列及32B条目，49类型／视图。Nightmare直属UNKNOWN61→37B；新增BossOverlay直属0B、每定义3B未知；Pause外部只拆出密码显示1B。旧范围／判断保存在JSON修订历史。Image+20本地模式与实际shader资源分开。生产版本仍57，未构建或测试。

**2026-11-10修订2**：[第7步显示／资源补证](room-display-resource-design.md)将Minimap+614..61B核为独立Lost问号ImageStrongPair，直属UNKNOWN23→15B；原8B未知记录保存在JSON的display_resource_revision，其余字节／未知不变。生产版本56，无测试。

2026-11-10。源：[presentation-byte-ledger.json](presentation-byte-ledger.json)。职责见[presentation-synchronization-review.md](presentation-synchronization-review.md)，新增来源见[presentation-resource-screen-sources.md](presentation-resource-screen-sources.md)。

**设计核对表，未进入正式布局生成链。** 声明范围有表不表示语义完整或运行接线完成。外层Host／本地不因嵌套通用ANM2归属恢复传输；直属UNKNOWN不含嵌套或指针后对象。

## 类型范围

| 类型 | 声明大小 | 直属UNKNOWN | 边界 |
|---|---|---|---|
| Vector | 0x8 | 0B（不含嵌套） | 已登记整个声明范围 |
| ColorMod | 0x2C | 0B（不含嵌套） | 已登记整个声明范围 |
| KColor | 0x14 | 0B（不含嵌套） | 已登记整个声明范围 |
| AnimationState | 0x20 | 3B（不含嵌套） | 已登记整个声明范围 |
| LayerState | 0xA0 | 4B（不含嵌套） | 已登记整个声明范围 |
| ANM2 | 0x114 | 8B（不含嵌套） | 已登记整个声明范围 |
| ImageStrongPair | 0x8 | 0B（不含嵌套） | 已登记整个声明范围 |
| PlayerHUDHeart | 0x10 | 3B（不含嵌套） | 已登记整个声明范围 |
| PlayerHUDActiveItem | 0x1C | 0B（不含嵌套） | 已登记整个声明范围 |
| PlayerHUDTrinket | 0x18 | 0B（不含嵌套） | 已登记整个声明范围 |
| PlayerHUDPocket | 0x11C | 0B（不含嵌套） | 已登记整个声明范围 |
| InventoryVisual | 0xC | 0B（不含嵌套） | 已登记整个声明范围 |
| PlayerHUD | 0x6E0 | 22B（不含嵌套） | 已登记整个声明范围 |
| StatHUDValue | 0x14 | 0B（不含嵌套） | 已登记整个声明范围 |
| StatHUDPlayer | 0xCC | 0B（不含嵌套） | 已登记整个声明范围 |
| StatHUD | 0x2B0 | 3B（不含嵌套） | 已登记整个声明范围 |
| HUDMessage | 0x1F4 | 153B（不含嵌套） | 已登记整个声明范围 |
| HistoryHUDItem | 0x28 | 0B（不含嵌套） | 已登记整个声明范围 |
| HistoryHUDPlayer | 0x14 | 4B（不含嵌套） | 已登记整个声明范围 |
| HistoryHUD | 0x2C | 3B（不含嵌套） | 已登记整个声明范围 |
| HUD | 0x5C90 | 169B（不含嵌套） | 已登记整个声明范围 |
| MinimapConfig | 0x160 | 12B（不含嵌套） | 已登记整个声明范围 |
| Minimap | 0x628 | 15B（不含嵌套） | 已登记整个声明范围 |
| ColorModState | 0x18 | 0B（不含嵌套） | 已登记整个声明范围 |
| GenericPrompt | 0x134 | 4B（不含嵌套） | 已登记整个声明范围 |
| ItemOverlayDefinition | 0x58 | 4B（不含嵌套） | 已登记整个声明范围 |
| ItemOverlay | 0x1260 | 0B（不含嵌套） | 已登记整个声明范围 |
| MusicStream | 0x34 | 4B（不含嵌套） | 已登记整个声明范围 |
| MusicStreamSet | 0x184 | 3B（不含嵌套） | 已登记整个声明范围 |
| MusicEntry | 0x60 | 2B（不含嵌套） | 已登记整个声明范围 |
| MusicLayerDefinition | 0x34 | 0B（不含嵌套） | 已登记整个声明范围 |
| Music | 0x368 | 1B（不含嵌套） | 已登记整个声明范围 |
| SoundSample | 0x38 | 4B（不含嵌套） | 已登记整个声明范围 |
| SoundEffect | 0x1B8 | 2B（不含嵌套） | 已登记整个声明范围 |
| SoundEffects | 0x18 | 0B（不含嵌套） | 已登记整个声明范围 |
| Font | 0x4C | 14B（不含嵌套） | 已登记整个声明范围 |
| FontSettings | 0x1C | 8B（不含嵌套） | 已登记整个声明范围 |
| ImageBase | 0x80 | 53B（不含嵌套） | 仅前缀 |
| RoomTransition | 0x23C | 228B（不含嵌套） | 已登记整个声明范围 |
| StageTransition | 0x20 | 16B（不含嵌套） | 已登记整个声明范围 |
| NightmareScene | 0x5EC | 37B（不含嵌套） | 已登记整个声明范围 |
| GameOver | 0x130 | 20B（不含嵌套） | 已登记整个声明范围 |
| PauseScreen | 0xDF4 | 1212B（不含嵌套） | 仅前缀 |
| GameScreenEffects | 0x24 | 6B（不含嵌套） | 已登记整个声明范围 |
| GameFadeView | 0x34 | 7B（不含嵌套） | 已登记整个声明范围 |
| GameColorView | 0x90 | 47B（不含嵌套） | 已登记整个声明范围 |
| NightmareSnapshotQueue | 0x14 | 0B（不含嵌套） | 已登记整个声明范围 |
| NightmareSnapshot | 0x20 | 0B（不含嵌套） | 已登记整个声明范围 |
| BossOverlayDefinition | 0x38 | 3B（不含嵌套） | 已登记整个声明范围 |
| BossOverlay | 0x234 | 0B（不含嵌套） | 已登记整个声明范围 |

## Vector

继承当前生产源的证据等级与UNKNOWN；本核对不扩大通用Sprite公开API或线格式。

依据：mod/binding/state/base.json::Vector

| 0x0—0x3 f32 x【状态；已核】X 分量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x4—0x7 f32 y【状态；已核】Y 分量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 |
|---|---|

## ColorMod

继承当前生产源的证据等级与UNKNOWN；本核对不扩大通用Sprite公开API或线格式。

依据：mod/binding/state/base.json::ColorMod

| 0x0—0x3 f32 tint_r【状态；已核】tint RGBA 的 R 分量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x4—0x7 f32 tint_g【状态；已核】tint RGBA 的 G 分量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x8—0xB f32 tint_b【状态；已核】tint RGBA 的 B 分量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0xC—0xF f32 tint_a【状态；已核】tint RGBA 的 A 分量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x10—0x13 f32 colorize_r【状态；已核】colorize RGBA 的 R 分量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x14—0x17 f32 colorize_g【状态；已核】colorize RGBA 的 G 分量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x18—0x1B f32 colorize_b【状态；已核】colorize RGBA 的 B 分量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x1C—0x1F f32 colorize_a【状态；已核】colorize RGBA 的 A 分量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x20—0x23 f32 offset_r【状态；已核】offset RGB 的 R 分量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x24—0x27 f32 offset_g【状态；已核】offset RGB 的 G 分量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x28—0x2B f32 offset_b【状态；已核】offset RGB 的 B 分量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 |
|---|---|---|---|---|---|---|---|---|---|---|

## KColor

继承当前生产源的证据等级与UNKNOWN；本核对不扩大通用Sprite公开API或线格式。

依据：mod/binding/state/base.json::KColor

| 0x0—0x3 f32 red【状态；已核】红色分量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x4—0x7 f32 green【状态；已核】绿色分量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x8—0xB f32 blue【状态；已核】蓝色分量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0xC—0xF f32 alpha【状态；已核】透明度分量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x10—0x13 u32 color_space【状态；已核】颜色空间标记：已核0=RGB、1=sRGB、2=HSL；不将当前观察标签冒充穷举集合 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 |
|---|---|---|---|---|

## AnimationState

继承当前生产源的证据等级与UNKNOWN；本核对不扩大通用Sprite公开API或线格式。

依据：mod/binding/state/base.json::AnimationState

| 0x0—0x3 ANM2* animation_owner【本地；已核】回指所属 ANM2；由上层对象关系确定 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x4—0x7 AnimationData* active_animation【资源；已核】当前动画资源项；序号由 ANM2 文件表解析，空指针使用资源空值标记 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x8—0xB int* layer_frames【容器；已核】各图层 AnimationFrame 内部序号选择器，长度取当前 AnimationData._layerCount → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0xC—0xF int* null_layer_frames【容器；已核】各空层 AnimationFrame 内部序号选择器，长度取当前 AnimationData._nullLayerCount → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x10—0x13 f32 anim_frame【状态；已核】动画当前帧值 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x14—0x14 bool is_playing【状态；SDK声明】播放标志兼绘制插值输入；直接安装最终值，不授权Client推进动画 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x15—0x17 bytes[3] unknown_15_17【未知；未知】SDK 字段间未解释的三个字节；不认作填充 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x18—0x1B i32 currently_triggered_events【权威；已核】Host动画事件消费位；Client安装最终姿态，不推进或重放事件，构造／换动画后本地清零 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x1C—0x1F i32 previously_triggered_events【权威；已核】Host动画事件历史位；不表示可靠事件队列，不进入Client状态或漂移比较 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 |
|---|---|---|---|---|---|---|---|---|

## LayerState

继承当前生产源的证据等级与UNKNOWN；本核对不扩大通用Sprite公开API或线格式。

依据：mod/binding/state/base.json::LayerState

| 0x0—0x3 LayerData* layer_data【派生；推断】由所属 ANM2 文件及数组序号定位的资源定义 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x4—0x7 ANM2* animation_owner【本地；SDK声明】所属 ANM2 回指，不作为网络状态 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x8—0x1F std::string spritesheet_path【资源；SDK声明】spritesheet路径元数据资源号；与实际Image选择独立，路径变更不授权Client抢先重载 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x20—0x23 u32 min_filter_mode【状态；SDK声明】缩小过滤模式 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x24—0x27 u32 mag_filter_mode【状态；SDK声明】放大过滤模式 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x28—0x2B u32 wrap_s_mode【状态；SDK声明】S 方向环绕模式 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x2C—0x2F u32 wrap_t_mode【状态；SDK声明】T 方向环绕模式 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x30—0x30 bool sprite_sheet_loaded【本地；SDK声明】本地图层资源加载／重载维护标志；ReplaceSpritesheet可置false同时保留旧Image，不跨进程复制，也不按Host原值覆盖Client加载结果 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x31—0x31 u8 unknown_31【未知；未知】J273 LoadGraphics 将该字节清零；具体语义未明，绝非已证填充 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x32—0x32 bool flip_x【状态；SDK声明】水平翻转 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x33—0x33 bool flip_y【状态；SDK声明】垂直翻转 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x34—0x3B Vector size【状态；SDK声明】图层尺寸 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x3C—0x3F f32 rotation【状态；SDK声明】图层旋转角度 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x40—0x47 Vector position【状态；SDK声明】图层位置 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x48—0x73 ColorMod color【状态；SDK声明】图层 ColorMod → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x74—0x74 bool visible【状态；SDK声明】图层显示开关 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x75—0x77 bytes[3] unknown_75_77【未知；未知】SDK 布局间未说明的三个字节；不认作填充 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x78—0x8B BlendMode blend_mode【状态；已核】图层混合模式的五 DWORD 值 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x8C—0x8F u32 render_flags【状态；SDK声明】图层渲染位标志 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x90—0x97 Vector crop_offset【状态；SDK声明】图层裁剪偏移 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x98—0x9F KAGE_SmartPointer_ImageBase image_selection【资源；推断】当前实际ImageBase选择，Null使用0xFFFF；Host由ImageBase._name形成资源身份，Client重建引用；重名／程序图像身份仍待第8步核 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## ANM2

继承当前生产源的证据等级与UNKNOWN；本核对不扩大通用Sprite公开API或线格式。

依据：mod/binding/state/base.json::ANM2

| 0x0—0x17 std::string file_name【资源；SDK声明】可空ANM2文件资源选择；Host loaded=false编码0xFFFF，true编码文件资源号；不以路径非空猜测存在性 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x18—0x2F std::string default_animation_name【容器；已核】默认动画名字符串；load 阶段读取，保留有界字节串，不当作不可变文件派生量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x30—0x4F AnimationState main_state【状态；SDK声明】主动画状态与独立图层/空层帧选择器 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x50—0x6F AnimationState overlay_state【状态；SDK声明】叠加动画状态与独立图层/空层帧选择器 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x70—0x70 bool overlay_render_first【状态；SDK声明】叠加动画先绘制开关 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x71—0x73 bytes[3] unknown_71_73【未知；未知】SDK 字段间未解释字节；不认作填充 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x74—0x77 AnimationData* animation_data【派生；已核】由 ANM2 文件动画表建立的资源数组首址 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x78—0x7B i32 animation_count【派生；已核】由 ANM2 文件动画表确定的动画数量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x7C—0x7F LayerState* layer_states【容器；已核】按 layer_count 遍历图层实例状态 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x80—0x83 u32 layer_count【派生；已核】由 ANM2 文件图层数组确定的图层数 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x84—0x87 std::string* null_layer_names【派生；已核】由文件建立的空层名称表 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x88—0x8B u32 null_layer_count【派生；已核】由文件建立的空层名称数量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x8C—0x8F std::string* event_names【派生；已核】由文件建立的事件名表 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x90—0x93 u32 event_count【派生；已核】由文件建立的事件数量 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x94—0x9B Vector offset【状态；SDK声明】绘制偏移 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x9C—0xA3 Vector scale【状态；SDK声明】绘制缩放 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0xA4—0xA7 f32 rotation【状态；SDK声明】全局旋转 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0xA8—0xD3 ColorMod color【状态；SDK声明】全局颜色 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0xD4—0xFF ColorMod champion_color【状态；SDK声明】精英色调 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x100—0x100 bool flip_x【状态；SDK声明】水平翻转 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x101—0x101 bool flip_y【状态；SDK声明】垂直翻转 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x102—0x103 bytes[2] unknown_102_103【未知；未知】SDK 字段间未解释字节；不认作填充 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x104—0x107 f32 playback_speed【状态；SDK声明】播放速度兼绘制插值的半帧比例输入；不在Client推进动画 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x108—0x108 u8 unknown_108【未知；未知】初始化为零但语义未明；不作为 shader 状态 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x109—0x109 bool loaded【本地；已核】本地ANM2构造／加载／重置结果；Host只用它确定file_name的可空资源选择，Client由资源owner完成准备后维护，不直接传布尔字节 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x10A—0x10B bytes[2] unknown_10A_10B【未知；未知】+0x109 与 shadowLayerIndex +0x10C 间未解释字节 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x10C—0x10F i32 shadow_layer_index【派生；已核】资源中名称为 shadow 的 LayerState 数组序号；不存在时为 -1 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 | 0x110—0x113 u32 render_flags【状态；SDK声明】ANM2 渲染位标志 → 仅复用字节事实；是否发送取决于外层已核Presentation用途 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## ImageStrongPair

依据：J273 smart-pointer helpers

| 0x0—0x3 ImageBase* image【资源；已核】当前实际选中的图像 → 资源号／具名程序图像结果；不传地址 | 0x4—0x7 Control* control【本地；已核】引用计数、析构及所有权 → 不发送；本地正常owner |
|---|---|

## PlayerHUDHeart

834930先检查visible；隐藏心格只传存在性，不读其余可能残留的名字。淡出色的412AC0是sin，不是RNG；按同world相位派生。

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUDHeart.zhl; LuaPlayerHUDHeart.cpp; J273 834930/832800

| 0x0—0x0 bool visible【状态；已核】心格可见 → 同步最终显示布尔 | 0x1—0x1 bool eternal【状态；已核】永恒心叠加 → 同步最终显示布尔 | 0x2—0x2 bool golden【状态；已核】金心叠加 → 同步最终显示布尔 | 0x3—0x3 bool fading【状态；已核】淡出心 → 同步最终显示布尔 | 0x4—0x4 i8 flash_type【状态；已核】最终心格闪烁类别 → 仅visible时同步有符号原值，不在Client执行UpdateHearts | 0x5—0x7 bytes[3] unknown_5_7【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x8—0xB char* animation【资源；已核】最终主动画名指针 → 仅visible时读有界名称／所属资源动画ID；本地持有字符串 | 0xC—0xF char* overlay_animation【资源；已核】最终叠加动画名指针 → 仅visible且eternal分支需要时读；不传指针 |
|---|---|---|---|---|---|---|---|

## PlayerHUDActiveItem

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUDActiveItem.zhl; J273 832C10/835B40

| 0x0—0x3 i32 visual_id【状态；已核】已选择的主动道具显示身份，含负生成ID → 同步；不据此GiveItem | 0x4—0x7 i32 book_visual_id【权威；推断】第二图像选择的更新比较缓存 → 不发送；最终book_image承接 | 0x8—0xF ImageStrongPair item_image【资源；已核】实际Image与本地引用计数control → 发送实际图像资源身份，空与准备失败分开；本地重建strong持有 | 0x10—0x17 ImageStrongPair book_image【资源；已核】实际Image与本地引用计数control → 发送实际图像资源身份，空与准备失败分开；本地重建strong持有 | 0x18—0x1B i32 charge_flash【状态；已核】充能闪烁倒计时，RenderActiveItem读取 → 同步当前显示值／合法帧锚点；不自行充能 |
|---|---|---|---|---|

## PlayerHUDTrinket

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUDTrinket.zhl; J273 832C10/837720

| 0x0—0x3 i32 visual_id【状态；已核】实际饰品显示身份 → 同步有符号值／golden语义，不重抽取 | 0x4—0x7 i32 tint_override_source【换算；已核】SDK名slot；837720在非golden分支实际判断此DWORD是否非零并选择额外colorize → 仅同步非零判断的1B最终tint_override；其整数值作为更新比较历史留Host | 0x8—0xF ImageStrongPair image【资源；已核】实际Image与本地引用计数control → 发送实际图像资源身份，空与准备失败分开；本地重建strong持有 | 0x10—0x17 Vector draw_center【状态；已核】Update扫描实际图片非透明范围形成的绘制中心 → 同步最终两值；不可只凭ID猜图像中心 |
|---|---|---|---|

## PlayerHUDPocket

依据：J273 832C10四槽实际284步长、+230/+234/+238；8378C0

| 0x0—0x3 i32 cached_type【权威；已核】用于检测何时切换资源的类型 → 不传更新比较值；实际PocketItem属于Player P3 | 0x4—0x7 i32 cached_visual【权威；已核】已识别药丸／卡片资源选择比较值 → 不传；发送实际Sprite／显示决定 | 0x8—0x11B ANM2 sprite【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update |
|---|---|---|

## InventoryVisual

依据：J273 832C10/839370/839A80；PlayerHUD+6A4 vector stride12

| 0x0—0x3 i32 visual_id【状态；已核】有序显示身份，含负生成ID → 同步；不得按库存count重排 | 0x4—0xB ImageStrongPair image【资源；已核】实际Image与本地引用计数control → 发送实际图像资源身份，空与准备失败分开；本地重建strong持有 |
|---|---|

## PlayerHUD

内嵌八份之外，Player+20A8动态HUD必须按同一类型单独建owner；空owner与缺失来源分开。

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PlayerHUD.zhl; J273 832C10/834930/835B40/837720/8378C0/839370/839A80/97EB40

| 0x0—0x3 Entity_Player* player【引用；已核】这份HUD实际身体owner → 发送StateRef并绑定subject/use/shape；不发送设备键 | 0x4—0x7 HUD* hud【本地；已核】回指本地HUD根 → 本地绑定，不发送 | 0x8—0x9 i16 index【换算；已核】原生HUD槽位 → 发送C/S显示槽／参与者身体关系；不复制原版联机索引语义 | 0xA—0xB i16 low_health_flash【状态；已核】低生命时产生的闪隐强度 → 同步最终显示值 | 0xC—0xC i8 red_flash【状态；已核】心色闪烁剩余显示相位 → 同步当前值；不恢复旧事件 | 0xD—0xD i8 soul_flash【状态；已核】心色闪烁剩余显示相位 → 同步当前值；不恢复旧事件 | 0xE—0xE i8 gold_flash【状态；已核】心色闪烁剩余显示相位 → 同步当前值；不恢复旧事件 | 0xF—0xF bytes[1] unknown_F_F【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x10—0x18F PlayerHUDHeart[24] hearts【容器；已核】固定24个最终心格 → 逐槽投影，不运行UpdateHearts | 0x190—0x1FF PlayerHUDActiveItem[4] active【容器；已核】固定四主动显示槽 → 逐槽资源／显示缓存，P4六值不重复 | 0x200—0x22F PlayerHUDTrinket[2] trinkets【容器；已核】两饰品显示槽 → 逐槽投影 | 0x230—0x69F PlayerHUDPocket[4] pockets【容器；已核】四份独立Pocket Sprite及更新比较值 → 发最终Sprite，比较值留Host | 0x6A0—0x6A0 bool holy_mantle_visible【状态；已核】Update求effect313，RenderHearts消费的最终护盾显示资格 → 同步最终布尔；不在Client查询效果313补算 | 0x6A1—0x6A3 bytes[3] unknown_6A1_6A3【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x6A4—0x6AF vector<InventoryVisual> inventory_visuals【容器；已核】Tainted Isaac／Crafting最终有序显示列表 → 发逻辑条目，不发begin/end/capacity | 0x6B0—0x6B0 bool inventory_dirty【权威；已核】请求重建库存图像列表 → 不传；不开放整个832C10 | 0x6B1—0x6B1 bool crafting_dirty【权威；已核】请求重建合成结果图像 → 不传；最终crafting_image承接 | 0x6B2—0x6B3 bytes[2] unknown_6B2_6B3【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x6B4—0x6BB ImageStrongPair crafting_image【资源；已核】实际Image与本地引用计数control → 发送实际图像资源身份，空与准备失败分开；本地重建strong持有 | 0x6BC—0x6BF f32 drop_display_scale【状态；已核】Drop输入已生产的HUD淡化/缩放，97EB40消费 → 同步最终值，不查询Client设备 | 0x6C0—0x6CF f32[4] crafting_bounds【派生；已核】839A80逐次min/max累计的绘制包围框scratch → 在相同绘制分派／本地视口重建；不下发上帧残值 | 0x6D0—0x6DF bytes[16] unknown_6D0_6DF【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## StatHUDValue

依据：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁

| 0x0—0x3 f32 value【状态；已核】当前显示值 → 同步实际可绘行 | 0x4—0x7 f32 delta【状态；已核】差值文字 → 同步实际可绘行 | 0x8—0xB f32 previous【权威；已核】更新时计算差值的历史值 → 不发；83C9B0不读此成员 | 0xC—0xF f32 threshold【权威；已核】更新时是否触发提示的阈值 → 不发；83C9B0不读此成员 | 0x10—0x13 i32 countdown【状态；已核】差值文字alpha／位移的当前相位 → 同步；不调用RecomputeStats |
|---|---|---|---|---|

## StatHUDPlayer

依据：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁

| 0x0—0x3 Entity_Player* player【引用；已核】实际显示身体 → StateRef，最多两列，不等同参与者上限 | 0x4—0xCB StatHUDValue[10] values【容器；已核】十个物理槽位 → 线上只选0..8；9无当前已核原版/官方绘制消费者 |
|---|---|

## StatHUD

官方Patch启用row8；其显示资格含Host进度406与本地显示偏好，须拆为Host最终资格+本地偏好，不能读取Client进度代替。

依据：J273 83CFD0/83D210生产；83C9B0绘制；REPENTOGON ASMRender.cpp星象房补丁

| 0x0—0x113 ANM2 sprite【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update | 0x114—0x2AB StatHUDPlayer[2] players【容器；已核】两列显示缓存 → 实际owner及0..8当前值、差值、计时；列位置本地排版 | 0x2AC—0x2AC bool combined_devil_angel【状态；已核】是否合并恶魔／天使概率的显示方式 → 同步最终布尔，不在Client查询物品498 | 0x2AD—0x2AF bytes[3] unknown_2AD_2AF【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 |
|---|---|---|---|

## HUDMessage

尾部未核用途仍UNKNOWN。普通样式可经具名完整recipe重建，现有通用字符串+路径不是完整recipe证明；不得把UNKNOWN尾部静默当不影响显示。

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/HUD_Message.zhl; J273 831620/8317A0/8318D0/831AD0/97E590

| 0x0—0x3 wchar_t* text【权威；已核】主文字栅格生产输入；内容含本地化／特殊首码 → 不传地址；最终text_image同步；需要文本语义的声明UI可另发有界内容 | 0x4—0x7 wchar_t* subtext【权威；已核】副文字栅格生产输入 → 同主文字；不以本地翻译覆盖Host结果 | 0x8—0x8 bool showing【状态；已核】横幅是否显示 → 同步当前值，中途加入不重放Show | 0x9—0x9 bool auto_hide【权威；已核】TextIn/TextStay/TextOut生产控制 → 不发；最终动画状态承接 | 0xA—0xA bool curse_display【权威；已核】选择字体／层及文字栅格样式的生产控制 → 最终图像／Sprite承接，不重复驱动Show | 0xB—0xB bytes[1] unknown_B_B【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0xC—0x17 f32[3] draw_transform【派生；已核】8318D0入口必覆写position/scale → 由该次布局与调用参数重建；不发旧scratch | 0x18—0x1F ImageStrongPair text_image【资源；已核】实际Image与本地引用计数control → 发送实际图像资源身份，空与准备失败分开；本地重建strong持有 | 0x20—0x133 ANM2 streak【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update | 0x134—0x147 KColor color1【权威；推断】文字图像生产/外观输入；Render text_image使用固定白色 → 只传最终图像；不得照搬二十字节当当前Render tint | 0x148—0x15B KColor color2【权威；推断】文字图像生产/外观输入 → 最终图像承接；官方8318D0不直接读 | 0x15C—0x1F3 bytes[152] unknown_15C_1F3【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 |
|---|---|---|---|---|---|---|---|---|---|---|---|

## HistoryHUDItem

依据：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl

| 0x0—0x3 i32 visual_id【状态；已核】实际显示ID，含633、0x88000位和负ID → 同步真实有序值；决定shader/程序图像路径 | 0x4—0xB ImageStrongPair image【资源；已核】实际Image与本地引用计数control → 发送实际图像资源身份，空与准备失败分开；本地重建strong持有 | 0xC—0x27 History_HistoryItem history_origin【权威；已核】原始获得史：时间、来源池、关卡等 → 原生Render不消费；不传整条历史，不恢复Player原始History |
|---|---|---|

## HistoryHUDPlayer

依据：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl

| 0x0—0x3 Entity_Player* player【引用；已核】这列最终列表的实际身体 → StateRef；不从P1/P2编号猜 | 0x4—0x7 bytes[4] unknown_4_7【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x8—0x13 vector<HistoryHUDItem> items【容器；已核】完整有序图像列表 → 发逻辑条目；屏幕上可见前缀由Client视口裁切 |
|---|---|---|

## HistoryHUD

依据：J273 82FA10；REP LuaHistoryHUD.cpp；HistoryHUD*.zhl

| 0x0—0x27 HistoryHUDPlayer[2] players【容器；已核】两个原生显示列 → 逐列owner和顺序；不简化成四参与者各一列 | 0x28—0x28 bool initialized【本地；已核】97AE90首次RecomputeAll维护标志 → 本地owner；不借它放行Recompute | 0x29—0x2B bytes[3] unknown_29_2B【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 |
|---|---|---|

## HUD

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/HUD.zhl; J273 979E10/97ACF0/97AE90/97C300/97E590/97F7A0/97FD50

| 0x0—0x36FF PlayerHUD[8] players【容器；已核】八个物理HUD槽 → 每槽明确存在性及C/S身体owner；动态HUD另计 | 0x3700—0x3797 bytes[152] unknown_3700_3797【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x3798—0x38AB ANM2 charge_bar【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update | 0x38AC—0x39BF ANM2 secondary_charge_bar【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update | 0x39C0—0x3AD3 ANM2 hearts【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update | 0x3AD4—0x3BE7 ANM2 pickup【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update | 0x3BE8—0x3CFB ANM2 cards_pills【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update | 0x3CFC—0x3EEF HUDMessage message_main【容器；已核】楼层／curse横幅 → 当前消息结果；不重放命令 | 0x3EF0—0x4AA7 HUDMessage[6] message_stack【容器；已核】六份堆叠横幅 → 逐槽当前结果 | 0x4AA8—0x5277 HUDMessage[4] message_players【容器；已核】四份参与者附近小横幅 → 逐槽结果，颜色若97E590必覆写则由C/S参与者色派生 | 0x5278—0x538B ANM2 fortune【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update | 0x538C—0x5393 ImageStrongPair fortune_image【资源；已核】实际Image与本地引用计数control → 发送实际图像资源身份，空与准备失败分开；本地重建strong持有 | 0x5394—0x54A7 ANM2 stats【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update | 0x54A8—0x54AF bytes[8] unknown_54A8_54AF【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x54B0—0x54B3 f32 boss_fill【状态；已核】Boss条填充，负值隐藏 → 同步有符号有限原值，不能强夹到0..1丢失隐藏 | 0x54B4—0x54BB i32[2] boss_flash【状态；已核】Boss条两组闪色相位 → 同步显示值；不在Client处理伤害事件 | 0x54BC—0x54BC bool boss_placement【派生；推断】97F7A0场景分支写位置选择；特殊83分支有保留值路径 → 需实际最终位置结果，不能将所有路径称必覆写；不直接复制raw | 0x54BD—0x54BF bytes[3] unknown_54BD_54BF【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x54C0—0x54C7 ImageStrongPair boss_bar_normal【资源；已核】实际Image与本地引用计数control → 发送实际图像资源身份，空与准备失败分开；本地重建strong持有 | 0x54C8—0x54CF ImageStrongPair boss_bar_alternate【资源；已核】实际Image与本地引用计数control → 发送实际图像资源身份，空与准备失败分开；本地重建strong持有 | 0x54D0—0x54D7 ImageStrongPair boss_bar_special【资源；已核】实际Image与本地引用计数control → 发送实际图像资源身份，空与准备失败分开；本地重建strong持有 | 0x54D8—0x54DB i16[2] boss_counts【状态；已核】Boss83的已完成/总数文字 → 同步当前两值 | 0x54DC—0x54E3 ImageStrongPair boss_counter_image【资源；已核】实际Image与本地引用计数control → 发送实际图像资源身份，空与准备失败分开；本地重建strong持有 | 0x54E4—0x54E4 bool visible【状态；已核】Host明确隐藏HUD的world显示资格 → 同步world资格；Client本地UI隐藏偏好可再收紧 | 0x54E5—0x54E7 bytes[3] unknown_54E5_54E7【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x54E8—0x54EB i32 refresh_type【本地；已核】原生刷新调度 → 不传；本地安装owner管理 | 0x54EC—0x54EC bool refresh_dirty【本地；已核】97ACF0清除的刷新标记 → 不传 | 0x54ED—0x54EF bytes[3] unknown_54ED_54EF【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x54F0—0x54F3 i32 refresh_countdown【本地；已核】显示缓存重建倒计时 → 不传；安装最终缓存 | 0x54F4—0x5607 ANM2 coop_menu【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 原版Join/Online面板不发送；C/S使用自己的参与者面板，若复用资源只本地加载 | 0x5608—0x571B ANM2 inventory【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update | 0x571C—0x582F ANM2 crafting【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update | 0x5830—0x5943 ANM2 poop_spells【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update | 0x5944—0x59B3 f32[28] layout_scratch【派生；已核】97FD50按本地HUDOffset重写的布局量 → 同调用及本地视口重建，不传Host坐标 | 0x59B4—0x5C63 StatHUD stat_hud【容器；已核】实际属性栏 → 最终显示投影，禁止Update/Recompute | 0x5C64—0x5C8F HistoryHUD history_hud【容器；已核】历史图像栏 → 实际显示列表及owner |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## MinimapConfig

官方MinimapConfig::Render签名命中4ECA30，实际为NPC更新且创建实体；此SDK标签不构成绘制调用授权。

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/MinimapConfig.zhl；J273 9635B0/966680；MinimapPatches.cpp

| 0x0—0x113 ANM2 border_sprite【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 保留实际资源／可变图层外观；最终边框选择按具名pass派生，不能遗漏黑框 | 0x114—0x11B ImageStrongPair surface【资源；已核】实际Image与本地引用计数control → 不发送FBO或像素；从同world描述/探索/保存图标及最终标记经安全绘制缓存路径重建 | 0x11C—0x123 Vector anchor【派生；SDK声明】本地地图锚点 → 本地视口/显示布局；声明Mod修改作为该adapter资源/布局结果 | 0x124—0x12B Vector padding【派生；SDK声明】边框留白 → 同上 | 0x12C—0x12F f32 offset_x【派生；SDK声明】位置偏移 → 本地布局或地图缓存由已声明输入重建；不按Host视口盲写 | 0x130—0x133 f32 border_scale【派生；SDK声明】边框缩放 → 本地布局或地图缓存由已声明输入重建；不按Host视口盲写 | 0x134—0x137 i32 width【派生；SDK声明】地图格宽 → 本地布局或地图缓存由已声明输入重建；不按Host视口盲写 | 0x138—0x13B i32 height【派生；SDK声明】地图格高 → 本地布局或地图缓存由已声明输入重建；不按Host视口盲写 | 0x13C—0x13F i32 icon_count【派生；SDK声明】图标数 → 本地布局或地图缓存由已声明输入重建；不按Host视口盲写 | 0x140—0x147 bytes[8] unknown_140_147【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x148—0x157 f32[4] bounds【派生；已核】地图缓存有效范围 → 安全构建路径产生；不是Player状态自动更新 | 0x158—0x15B f32 alpha【本地；已核】965DB0按本地展开状态推进透明度 → 独立纯UI相位；不放行原965DB0 | 0x15C—0x15F bytes[4] unknown_15C_15F【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

## Minimap

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Minimap.zhl; Wiki Minimap.md; J273 965DB0/965D60/9635B0/963540/966680

| 0x0—0x3 i32 state【本地；已核】本地查看/展开状态 → 不同步Host按键；world强制隐藏由独立最终资格控制 | 0x4—0x4 bool hold_mode【本地；已核】本地map按住/释放转换历史 → 不发送，不读取原版Online输入 | 0x5—0x7 bytes[3] unknown_5_7【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x8—0xB u32 hold_time【本地；已核】Map动作长按时间 → 不发送；C/S本地UI采样 | 0xC—0xF f32 unknown_float【未知；未知】SDK仅给_unkFloat，没有语义证据 → 不发送；保留UNKNOWN | 0x10—0x17 bytes[8] unknown_10_17【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x18—0x2D7 MinimapConfig[2] configs【容器；已核】小/大地图两份缓存 → 分开最终资源、world事实和本地视口 | 0x2D8—0x3EB ANM2 item_icons【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update | 0x3EC—0x4FF ANM2 pickup_sprite【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → SDK称seems unused不是证明；保留字节事实，具体无消费者投影不发，不宣称可安全删除资源 | 0x500—0x613 ANM2 icons【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update | 0x614—0x61B ImageStrongPair question_image【资源；已核】Curse of the Lost独立问号图像；默认gfx/ui/minimap_questionmark.png，不是iconsSprite某帧 → 实际可空图像资源u16引用归Presentation；地址/control本地；不传地图FBO或按固定文件掩盖实际内容变化 | 0x61C—0x61F i32 shake_duration【权威；已核】965DB0用随机生成震动的控制倒计时 → 不发送；不调用其随机生产器 | 0x620—0x627 Vector shake_offset【状态；已核】Render直接使用的最终震动偏移 → 同步同world两值，不从Client RNG重算 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

## ColorModState

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/ColorModState.zhl；J273 6F7F90/6F6DE8/7021A0

| 0x0—0x3 f32 r【状态；已核】屏幕最终色调分量 → 仅Game+66920 current的实例投影；target/lerp实例整体留Host | 0x4—0x7 f32 g【状态；已核】屏幕最终色调分量 → 仅Game+66920 current的实例投影；target/lerp实例整体留Host | 0x8—0xB f32 b【状态；已核】屏幕最终色调分量 → 仅Game+66920 current的实例投影；target/lerp实例整体留Host | 0xC—0xF f32 strength【状态；已核】屏幕最终色调分量 → 仅Game+66920 current的实例投影；target/lerp实例整体留Host | 0x10—0x13 f32 brightness【状态；已核】屏幕最终色调分量 → 仅Game+66920 current的实例投影；target/lerp实例整体留Host | 0x14—0x17 f32 contrast【状态；已核】屏幕最终色调分量 → 仅Game+66920 current的实例投影；target/lerp实例整体留Host |
|---|---|---|---|---|---|

## GenericPrompt

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GenericPrompt.zhl；J273 82C5A0/82C440/82CBB0/82CB90

| 0x0—0x113 ANM2 paper【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 共享world Prompt同步最终纸张资源/姿态；本地菜单Prompt整体本地 | 0x114—0x127 wchar_t*[5] text【容器；已核】五行实际文字：前二标题、后三说明 → 共享Prompt发送有界UTF16内容/最终图像；本地Prompt不上线，地址不传 | 0x128—0x12B i32 current_selection【换算；已核】未提交的Yes/No选中项 → 本地交互Prompt留本地；共享Prompt传Host最终显示选择，不能执行提交 | 0x12C—0x12F i32 submitted_selection【权威；已核】0未提交、1Yes、2No；可触发run决策 → 不作为状态应用写Client命令/回放输入 | 0x130—0x133 bytes[4] unknown_130_133【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 |
|---|---|---|---|---|

## ItemOverlayDefinition

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/ItemOverlay.zhl；J273 984660/984110/9848B0

| 0x0—0x3 bytes[4] unknown_0_3【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x4—0x1B std::string file【本地；已核】本地匹配配置中的Sprite资源路径 → 以资源目录匹配，不按帧发送配置 | 0x1C—0x33 std::string animation【本地；已核】配置动画名 → 同上 | 0x34—0x4B std::string image【本地；已核】配置图层替换路径 → 同上，当前实际Image仍属于Sprite | 0x4C—0x57 vector<LayerColor> layer_colors【本地；已核】配置按名称的图层颜色覆盖 → 本地配置；最终Sprite状态承接 |
|---|---|---|---|---|

## ItemOverlay

984110不只是动画：含SleepFillHP、解锁与转场分支。Client不得运行它来补字节。

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/ItemOverlay.zhl；J273 984660/984110/9848B0

| 0x0—0x3 u32 state【换算；已核】0inactive/1waiting/2active → 只传最终可见性(state>=2)；不在Client推进等待/完成FSM | 0x4—0x7 i32 overlay_id【状态；已核】已选giantbook类型，45有专门绘制分支 → 同步当前身份，不回放Show | 0x8—0x11B ANM2 sprite【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update | 0x11C—0x1143 ItemOverlayDefinition[47] definitions【本地；已核】47份0x58配置表，9848B0索引 → 不发送整表；本地配置匹配且最终资源已备好 | 0x1144—0x1147 i32 delay【权威；已核】WAITING更新倒计时 → 不传；最终可见性承接，旧GetDelay不保证原值相等 | 0x1148—0x114B Entity_Player* player【引用；已核】MegaMush绘制锚点身体 → StateRef；不复制原生引用链 | 0x114C—0x125F ANM2 mega_mush_player【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update |
|---|---|---|---|---|---|---|

## MusicStream

修订4撤销旧+0C=pitch及+2C播放游标推断；直属UNKNOWN24→4B，后端对象ABI另计。

依据：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md

| 0x0—0x3 SoundSourceBase* audio_source【本地；已核】本地音频source强引用的对象指针 → 不发送；本地adapter管理 | 0x4—0x7 ReferenceCount* audio_control【本地；已核】强引用pair的本地control；9F76E0/9F7CA0建立 → 不发送；不当作播放身份 | 0x8—0x8 bool loop【换算；已核】当前source循环请求 → 对应具名voice的有效循环状态 | 0x9—0x9 bool pending_play【本地；已核】Play提交后置1，正常后端playing查询可清零；并非已出声 → 不发raw；记录实际提交及结果 | 0xA—0xA bool paused【换算；已核】正常Pause/Resume维护的source暂停标志 → 不复制后端paused镜像；仅承接有明确world用途的voice期望暂停 | 0xB—0xB bytes[1] unknown_B_B【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 | 0xC—0xF f32 backend_volume【本地；已核】后端音量缓存；旧pitch误名已撤销 → 不直接发送、不反除Options | 0x10—0x13 f32 pan【换算；已核】当前pan请求/缓存；非mono有效分支另核 → 只承接实际生效的voice参数 | 0x14—0x17 f32 pitch【换算；已核】真实source pitch；7D5C40写+14并调用vtable+58 → 当前有效pitch随voice承接 | 0x18—0x1B f32 config_gain【权威；已核】音乐/分层配置mul，参与最终混音 → 留Host；最终voice gain已经承接mul，不重复发或再乘一次 | 0x1C—0x1F f32 layer_gain【权威；已核】当前声部权重，不是后端最终volume → 留Host；最终voice gain已经承接层权重，不再发第二套混音输入 | 0x20—0x23 f32 target_layer_gain【权威；已核】声部权重目标；由layer mask等生产 → 不把目标当当前gain | 0x24—0x27 f32 layer_fade_rate【权威；已核】当前声部权重平滑速率 → 正常生产结果承接 | 0x28—0x28 bool active【本地；已核】流活动维护标志；不等价于后端已出声 → 后端活动维护缓存不上线；world逻辑存续/退休须由独立用途来源建立 | 0x29—0x2B bytes[3] unknown_29_2B【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 | 0x2C—0x2F u32 update_count【本地；已核】7D7B90正常Update计数；>10才查询后端活动状态 → 不上线；不能作为秒/采样游标 | 0x30—0x33 MusicStreamSet* parent_set【本地；已核】最终gain读取父组+174；无父时不乘组gain → 不发地址；自有voice组关系承接 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## MusicStreamSet

依据：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md

| 0x0—0x16B MusicStream[7] streams【本地；已核】主流+六layer，各0x34 → 只保留有逻辑用途的并存voice最终资源/参数；不发送固定14个原生空槽 | 0x16C—0x16F u32 layers【权威；已核】六个layer开关，7D8A40实际读取 → 留Host；已发每voice最终gain时无需另发六位mask | 0x170—0x173 u32 mix_mode【权威；已核】layer影响主流的最终混音分支 → 留Host；最终voice gain承接模式效果，没有另行网络消费者 | 0x174—0x177 f32 volume【权威；已核】流组当前gain；多个正常writer已乘Options，不能反除恢复全部world意图 → Options前最终voice gain唯一承接；原生组current和混音树不下行，完整writer仍未闭合 | 0x178—0x17B f32 target_volume【权威；已核】流组目标音量 → 不传，最终逻辑增益承接 | 0x17C—0x17F f32 fade_rate【权威；已核】流组淡入淡出生产速率 → 不传原生生产器值 | 0x180—0x180 bool playing【本地；已核】组播放维护状态 → 本地后端维护，逻辑期望另外承载 | 0x181—0x183 bytes[3] unknown_181_183【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 |
|---|---|---|---|---|---|---|---|

## MusicEntry

修订4据7D60A0解析/实际存储及7D5CC0消费者补证；直属UNKNOWN72→2B。长解析函数bad-SP告警不作异常ABI闭合。

依据：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md

| 0x0—0x3 u32 config_id【本地；已核】XML曲目ID，解析写入 → 实际资源/voice承接；不复制配置对象 | 0x4—0x1B std::string name【本地；已核】XML曲目名称 → 资源目录匹配 | 0x1C—0x33 std::string intro【资源；已核】已解析intro文件；存在时先播并排入main → 实际资源依赖；不发string对象 | 0x34—0x4B std::string main【资源；已核】已解析主文件路径；+44是该string长度成员 → 实际文件资源；不发string对象 | 0x4C—0x4C bool loop【资源；已核】主文件loop配置 → 当前voice有效循环语义 | 0x4D—0x4D u8 layer_mode【权威；已核】分层混音模式；模式2可抑制主声部 → 最终实际声部gain承接 | 0x4E—0x4F bytes[2] unknown_4E_4F【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 | 0x50—0x53 f32 gain【权威；已核】XML mul；复制到主流+18 → 留Host；实际voice最终gain承接mul，不作为Client重复混音输入 | 0x54—0x57 f32 layer_fade_speed【权威；已核】XML layerfadespeed；复制到流+24 → 实际当前权重承接 | 0x58—0x5B MusicLayerDefinition* layer_defs【本地；已核】已分配34h步长层定义数组；含new[]计数头 → 只在合法配置owner存活期间读取；不发地址 | 0x5C—0x5F u32 layer_count【本地；已核】配置层数；消费端只处理6物理layer槽 → 不把任意数量当网络允许集 |
|---|---|---|---|---|---|---|---|---|---|---|

## MusicLayerDefinition

音乐分层资源定义，不是MusicStream；两个24B字符串及mul，直属未知0B。

依据：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md

| 0x0—0x17 std::string intro【资源；已核】分层intro路径 → 已选实际资源引用，不发原生string | 0x18—0x2F std::string main【资源；已核】分层main路径 → 已选实际资源引用，不发原生string | 0x30—0x33 f32 gain【权威；已核】分层mul，复制到对应MusicStream+18 → 留Host；实际voice最终gain已含该配置mul |
|---|---|---|

## Music

依据：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md

| 0x0—0x3 u32 current_stream【本地；已核】两组crossfade缓冲选择 → 不发后端槽号；发稳定逻辑voice身份 | 0x4—0x30B MusicStreamSet[2] stream_sets【容器；已核】两组0x184，crossfade时两者都可能发声 → 所有并存逻辑voice的最终参数；原生两组buffer及空槽不是网络结构 | 0x30C—0x30F u32 current_id【资源；已核】当前曲目资源身份 → 只作为Host绑定实际曲目文件的来源；最终voice资源已有时不另发currentId Getter | 0x310—0x313 u32 queued_id【权威；已核】原生待播放控制 → 需要续播时记录Host实际选择/事件；不授权Client按游戏条件选曲 | 0x314—0x317 f32 pitch【权威；已核】当前有效pitch → 留Host作生产输入；每voice最终pitch已承接时不另发全局pitch | 0x318—0x31B f32 target_pitch【权威；已核】pitch滑动目标 → 不传；同步最终pitch/具名控制事件 | 0x31C—0x327 vector<MusicEntry> entries【本地；已核】曲目配置/后端资源池 → 不传容器地址/配置实例；资源目录匹配 | 0x328—0x328 bool enabled【权威；已核】游戏请求与用户MusicEnabled需分开 → 不发混合enabled字段；明确world影响由voice最终gain/pause/存续承接，个人Mute独立 | 0x329—0x329 bytes[1] unknown_329_329【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x32A—0x32B u16 jingle_id【本地；已核】官方Hook在super后写的jingle ID；原生拒绝启动时也可能更新 → 本地官方诊断ID，不作独立网络状态；实际有效jingle资源/实例另承接 | 0x32C—0x35F MusicStream jingle_stream【本地；已核】独立jingle音频流 → 语义voice状态/开始事件；本地构造 | 0x360—0x363 f32 background_ducking【权威；已核】7D7DA0产生的背景音乐ducking当前量；非jingle流自身gain → 留Host；最终voice gain已承接ducking，不重复发送 | 0x364—0x367 i32 jingle_countdown【权威；已核】jingle保持倒计时；公开ID读它，<=0不证明淡出已停止 → 留Host倒计时，不发剩余值或当游标；逻辑事件时点/必要段落另核 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

## SoundSample

修订4直属UNKNOWN16→4B；资源加载/声音实例身份分开，sample+28为pan，+34为配置weight。

依据：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md

| 0x0—0x17 std::string path【资源；已核】已选sample文件元数据；实际加载解析路径另绑定 → 实际文件资源身份；不发string或在Client随机 | 0x18—0x1B SoundSourceBase* audio_source【本地；已核】本地source强引用对象指针 → 本地adapter拥有 | 0x1C—0x1F ReferenceCount* audio_control【本地；已核】9F76E0/9F7CA0建立的本地强引用control → 不上线 | 0x20—0x20 bool loop【换算；已核】当前source循环请求；StopLooping还查后端有效loop → 只承接明确world用途voice的loop请求；不复制后端查询/缓存 | 0x21—0x21 bool pending_play【本地；已核】Play提交后置1；正常playing查询可清零 → 不作网络身份或声学确认 | 0x22—0x22 bool paused【本地；已核】与通用source wrapper相同的暂停缓存 → 本地/具名world暂停分开 | 0x23—0x23 bytes[1] unknown_23_23【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 | 0x24—0x27 f32 backend_volume【本地；已核】乘Host Options后后端音量；不同正常路径clamp不同 → 不直接发、不反除；Options前逻辑量及规则另记 | 0x28—0x2B f32 pan【换算；已核】9F7970及Room sound460内联路径写入；非mono可能不生效 → 有world用途时保留pan语义与资源规则；不传播后端失败/请求缓存为跨端事实 | 0x2C—0x2F f32 pitch【换算；已核】当前source pitch → voice参数/Start实际结果 | 0x30—0x30 bool load_on_play【本地；已核】1走Play内正常加载，0可预载；旧streaming名称撤销 → 加载策略及后端状态本地 | 0x31—0x33 bytes[3] unknown_31_33【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 | 0x34—0x37 i32 config_weight【权威；已核】907AA6将XML weight的atoi结果写此；907C60选样本未读本字 → 不重新按配置抽取，实际已选sample承接 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

## SoundEffect

修订4直属UNKNOWN30→2B；+19C是真实配置ID，不撤销+0是最近Play帧的纠错。

依据：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md

| 0x0—0x3 i32 last_play_frame【本地；已核】907C60写ManagerFrame/2，SDK误名_id → 不作为SoundEffect ID同步 | 0x4—0x7 i32 next_allowed_frame【权威；已核】重复播放节流截止帧 → 不发；FrameDelay不是声音启动延迟 | 0x8—0x18F SoundSample[7] samples【本地；已核】七个物理sample槽，实际数量由+190限定 → 不发原生数组；选中资源由事件/持续voice结果承接 | 0x190—0x193 u32 sample_count【本地；已核】可选sample数 → 资源配置本地匹配，不驱动Client随机 | 0x194—0x197 f32 logical_volume【权威；已核】最近Play或AdjustVolume写入的sound级逻辑volume；不能表示不同slot历史 → 按实际命中voice缓存逻辑gain；不能当完整SFX快照 | 0x198—0x198 bool loaded【本地；已核】资源实际加载结果 → 本地owner维护 | 0x199—0x199 bool preload【本地；已核】预加载策略 → 本地资源策略 | 0x19A—0x19B bytes[2] unknown_19A_19B【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 | 0x19C—0x19F i32 config_id【本地；已核】真实配置ID；90783F解析写+19C，SDK+0误名 → 声音调用/资源身份匹配，不发本地对象 | 0x1A0—0x1B7 std::string name【本地；已核】XML sound名称，构造/解析/析构均有对应操作 → 资源目录匹配，不发原生string |
|---|---|---|---|---|---|---|---|---|---|

## SoundEffects

SFXManager.zhl的size0是空壳声明；Manager+2A160..2A177实际SoundEffects18B，下一字段OptionsConfig+2A178。 修订4另核+14为modifier恢复步长；直属UNKNOWN4→0B。

依据：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md

| 0x0—0xB vector<SoundEffect> sounds【本地；已核】实际SFXManager底层类型 → 本地资源池，不发地址/所有后端状态 | 0xC—0xF i32 load_count【本地；已核】加载计数 → 不传 | 0x10—0x13 f32 volume_modifier【权威；已核】Play/Adjust/ClearVolumeModifier消费的乘数；全部world/本地writer分类未闭合 → 最终world voice gain唯一承接；不再发乘数/平滑器，world/个人writer来源仍待分离 | 0x14—0x17 f32 modifier_recovery_step【权威；已核】Manager92EB03读取；未暂停时令+10向1恢复的步长，92A19E构造初值0.005 → 原生步长/平滑器留Host；正常恢复后的Options前乘数及voice实际gain承接 |
|---|---|---|---|

## Font

Font不是唯一全局单例：Manager内嵌九份，Lua还可构造独立实例；不能跨owner套用字体。

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680

| 0x0—0x0 bool loaded【本地；已核】本地字体准备结果 → 不发送原值；资源owner失败不能伪装为空 | 0x1—0x3 bytes[3] unknown_1_3【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x4—0x5 i16 size【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0x6—0x6 u8 info_flags【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0x7—0x7 u8 charset【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0x8—0x9 u16 stretch【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0xA—0xA u8 aa【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0xB—0xB u8 padding_up【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0xC—0xC u8 padding_right【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0xD—0xD u8 padding_down【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0xE—0xE u8 padding_left【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0xF—0xF u8 spacing_x【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0x10—0x10 u8 spacing_y【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0x11—0x11 u8 outline【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0x12—0x13 bytes[2] unknown_12_13【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x14—0x15 u16 line_height【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0x16—0x17 u16 baseline【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0x18—0x19 u16 scale_w【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0x1A—0x1B u16 scale_h【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0x1C—0x1D u16 pages【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0x1E—0x1E u8 common_flags【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0x1F—0x1F u8 alpha_channel【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0x20—0x20 u8 red_channel【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0x21—0x21 u8 green_channel【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0x22—0x22 u8 blue_channel【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0x23—0x23 bytes[1] unknown_23_23【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x24—0x27 u32 char_count【派生；SDK声明】匹配.fnt资源中的字体度量/定义 → 本地资源重建，不发每帧度量 | 0x28—0x2B FontCharBlock* chars【本地；已核】字体字符资源数组 → 不发地址；由资源加载 | 0x2C—0x37 vector<FontKerningPair> kerning【本地；已核】字体字距资源 → 不发头 | 0x38—0x3F bytes[8] unknown_38_3F【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x40—0x47 map<char,int> char_map【本地；已核】字符索引查找表 → 本地重建 | 0x48—0x4B i32 missing_char【本地；SDK声明】这个Font实例的缺字替换选择 → 本地字体策略；声明Mod有特殊修改时其recipe需包含 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## FontSettings

SDK两个unk保留UNKNOWN；不把有DrawString API理解成自动捕获所有绘图。

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/FontSettings.zhl

| 0x0—0x3 i32 alignment【本地；SDK声明】DrawString调用排版参数 → 本地布局；共享文字recipe需要时具名携带 | 0x4—0x5 i16 max_chars【本地；SDK声明】字符上限 → 同上 | 0x6—0x7 u16 max_width【本地；SDK声明】换行宽度 → 同上 | 0x8—0xB i32 wrap【本地；SDK声明】换行模式 → 同上 | 0xC—0xF f32 line_height【本地；SDK声明】行距 → 同上 | 0x10—0x13 i32 missing_char【本地；SDK声明】缺字覆盖 → 同上 | 0x14—0x1B bytes[8] unknown_14_1B【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 |
|---|---|---|---|---|---|---|

## ImageBase

未声明ImagePNG/PIC/PCX/GL后端完整大小；不据ImageBase的80h前缀读取派生私有像素。

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl；ImagePatches.cpp；LuaRender.cpp:Renderer_CreateImage

| 0x0—0x3 void** vtable【本地；SDK声明】原生Image类虚表 → 不传 | 0x4—0xF bytes[12] unknown_4_F【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x10—0x17 u64 flags【换算；SDK声明】文件图/程序图等资源种类 → 具名资源类型，原始混合flags不整传 | 0x18—0x1F bytes[8] unknown_18_1F【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x20—0x23 u32 shader【本地；已核】Image自动选择/抑制shader的本地模式；3用于抑制，不等于ShaderType枚举 → 不直接把模式原数当资源ID；实际固定pass/GraphicsManager.shader映射AACAB0内置表 | 0x24—0x27 u32 min_filter【状态；SDK声明】实际图像采样参数 → 所在Layer已承载时不重复；独立Image需具名资源owner | 0x28—0x2B u32 mag_filter【状态；SDK声明】实际图像采样参数 → 所在Layer已承载时不重复；独立Image需具名资源owner | 0x2C—0x2F u32 wrap_s【状态；SDK声明】实际图像采样参数 → 所在Layer已承载时不重复；独立Image需具名资源owner | 0x30—0x33 u32 wrap_t【状态；SDK声明】实际图像采样参数 → 所在Layer已承载时不重复；独立Image需具名资源owner | 0x34—0x37 u8* vertex_format【本地；SDK声明】本地顶点格式数组 → 资源定义重建，不传地址 | 0x38—0x38 u8 attribute_count【派生；SDK声明】顶点格式属性数 → 本地shader定义 | 0x39—0x39 bytes[1] unknown_39_39【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x3A—0x3B u16 vertex_stride【派生；SDK声明】顶点宽度 → 本地shader定义 | 0x3C—0x3F char* name【资源；SDK声明】可能重复的图像名称 → 不能单凭名字唯一标识程序图像/动态内容 | 0x40—0x4B vector<RenderBatch*> batches【本地；SDK声明】绘制批次 → 本地renderer，不传 | 0x4C—0x4F RenderBatch* last_batch【本地；SDK声明】批次缓存 → 不传 | 0x50—0x5B vector<RenderBatch*> transparent_batches【本地；SDK声明】透明批次复用池 → 不传 | 0x5C—0x5F RenderBatch* last_transparent_batch【本地；SDK声明】最近透明批次 → 不传 | 0x60—0x7F bytes[32] unknown_60_7F【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## RoomTransition

 修订3补正常surface内容来源与逐层额外肖像临时变换采集点；原生孔保持。

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomTransition.zhl；J273 8273C0

| 0x0—0x3 i32 mode【权威；已核】Render只处理2/3等阶段 → 留Host作最终贡献生产；网络只保留最终图像/位移/alpha/遮罩和顺序，不额外发原生selector/进度 | 0x4—0xB u32[2] progress_inputs【权威；推断】过渡计数/总时长生产输入 → 留Host作最终贡献生产；网络只保留最终图像/位移/alpha/遮罩和顺序，不额外发原生selector/进度 | 0xC—0xF f32 scroll_progress【权威；已核】滑动绘制当前比例 → 留Host作最终贡献生产；网络只保留最终图像/位移/alpha/遮罩和顺序，不额外发原生selector/进度 | 0x10—0x13 i32 direction【权威；已核】转场可见滑动方向 → 留Host作最终贡献生产；网络只保留最终图像/位移/alpha/遮罩和顺序，不额外发原生selector/进度 | 0x14—0x1B i32[2] destination【权威；SDK声明】目标roomIndex/dimension → 结构事务唯一接管；显示只引用已有结构身份，不发目标房间命令 | 0x1C—0x1F i32 transition_style【权威；已核】Render遮罩样式选择 → 留Host作最终贡献生产；网络只保留最终图像/位移/alpha/遮罩和顺序，不额外发原生selector/进度 | 0x20—0x27 bytes[8] unknown_20_27【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x28—0x2F ImageStrongPair previous_surface【资源；已核】实际Image与本地引用计数control → 正常Host已完成的实际surface内容或完整已核recipe；分别绑定结构/资源版本；见presentation-resource-screen-sources.md，不只靠名字或重新Render旧房 | 0x30—0x37 ImageStrongPair next_surface【资源；已核】实际Image与本地引用计数control → 正常Host已完成的实际surface内容或完整已核recipe；分别绑定结构/资源版本；见presentation-resource-screen-sources.md，不只靠名字或重新Render旧房 | 0x38—0x14B ANM2 versus【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update | 0x14C—0x223 bytes[216] unknown_14C_223【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x224—0x22B map<int,ANM2> extra_portraits【容器；已核】按层ID的额外肖像Sprite → 只发实际被采用的逐layer主/叠加最终贡献；原map/未经变换Sprite及变换生产输入不上线 | 0x22C—0x233 Vector scroll_offset【权威；已核】转场绘制偏移 → 留Host作最终贡献生产；网络只保留最终图像/位移/alpha/遮罩和顺序，不额外发原生selector/进度 | 0x234—0x237 bytes[4] unknown_234_237【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x238—0x23B i32 boss_intro【权威；已核】boss intro实际分支选择 → 留Host作最终贡献生产；网络只保留最终图像/位移/alpha/遮罩和顺序，不额外发原生selector/进度 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## StageTransition

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/StageTransition.zhl；J273 909E60

| 0x0—0x3 u32 mode【权威；已核】2淡入/3淡出等分支 → 本路线只发最终遮罩颜色/alpha/存在性；原mode/计时/render_kind留Host，不按SDK bool重放 | 0x4—0xB u32[2] progress_inputs【权威；已核】Render unsigned current/duration → 本路线只发最终遮罩颜色/alpha/存在性；原mode/计时/render_kind留Host，不按SDK bool重放 | 0xC—0xF bytes[4] unknown_C_F【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x10—0x13 i32 render_kind【权威；已核】909E60按DWORD==5决定白色；SDK只声明_sameStage bool → 本路线只发最终遮罩颜色/alpha/存在性；原mode/计时/render_kind留Host，不按SDK bool重放 | 0x14—0x1F bytes[12] unknown_14_1F【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 |
|---|---|---|---|---|

## NightmareScene

8FAC20含实体/配置绘制与未核payload；不能直接放行整函数或仅发四Sprite宣称完整。412A90/412AC0为cos/sin；+4B1属于+3A8 Sprite.loaded，不是独立外层字段。 修订3核定+360配置索引和+4C0队列头，直属未知61→37B；正常消费后的有序贡献取代未名payload交接，原始未知保留于修订历史。

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/NightmareScene.zhl；LuaNightmareScene.cpp；J273 8FAC20

| 0x0—0x3 i32 active【换算；已核】实际绘制根开关 → 只承接当前完整贡献批次的有效性/存在语义，不发原生流程状态或启动Show | 0x4—0x7 u32 display_frame【权威；已核】进度条/图标闪烁读本场景帧 → 留Host作有序最终贡献的生产输入；不与最终Connector/Levels/slot/Sprite/颜色结果重复下发 | 0x8—0x1F bytes[24] unknown_8_1F【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x20—0x133 ANM2 background【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 只承接该Sprite每次实际主/叠加有序贡献的资源/姿态；保留必要ANM2插值字段，不复制完整生产owner | 0x134—0x247 ANM2 bubble【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 只承接该Sprite每次实际主/叠加有序贡献的资源/姿态；保留必要ANM2插值字段，不复制完整生产owner | 0x248—0x35B ANM2 progress【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 只承接该Sprite每次实际主/叠加有序贡献的资源/姿态；保留必要ANM2插值字段，不复制完整生产owner | 0x35C—0x35F f32 progress_alpha【权威；已核】进度背景当前透明量 → 留Host作有序最终贡献的生产输入；不与最终Connector/Levels/slot/Sprite/颜色结果重复下发 | 0x360—0x363 i32 selected_config_index【权威；已核】Host选中Nightmare配置字符串条目的索引 → 留Host；同步实际可见Sprite/图像贡献 | 0x364—0x367 i32 progress_stage【权威；已核】实际进度阶段选择 → 留Host作有序最终贡献的生产输入；不与最终Connector/Levels/slot/Sprite/颜色结果重复下发 | 0x368—0x368 bool progress_flag【权威；已核】进度路线绘制分支 → 留Host作有序最终贡献的生产输入；不与最终Connector/Levels/slot/Sprite/颜色结果重复下发 | 0x369—0x36B bytes[3] unknown_369_36B【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x36C—0x3A3 i32[14] progress_map【权威；已核】公开14项最终路线图 → 留Host作有序最终贡献的生产输入；不与最终Connector/Levels/slot/Sprite/颜色结果重复下发 | 0x3A4—0x3A7 bytes[4] unknown_3A4_3A7【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x3A8—0x4BB ANM2 extra_bubble_sprite【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 只承接该Sprite每次实际主/叠加有序贡献的资源/姿态；保留必要ANM2插值字段，不复制完整生产owner | 0x4BC—0x4BC bool bubble_visible【权威；已核】bubble绘制开关 → 留Host作有序最终贡献的生产输入；不与最终Connector/Levels/slot/Sprite/颜色结果重复下发 | 0x4BD—0x4BF bytes[3] unknown_4BD_4BF【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x4C0—0x4D3 NightmareSnapshotQueue snapshot_queue【容器；已核】正常Show维护的Host条件显示队列；20B头及32B元素见独立表 → 不发原生队列/历史；8FAF1A—8FB178等正常消费后记录有序最终显示贡献 | 0x4D4—0x5E7 ANM2 player_extra_portrait【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 只承接该Sprite每次实际主/叠加有序贡献的资源/姿态；保留必要ANM2插值字段，不复制完整生产owner | 0x5E8—0x5E8 bool dogma【权威；已核】Dogma nightmare显示类型 → 留Host作有序最终贡献的生产输入；不与最终Connector/Levels/slot/Sprite/颜色结果重复下发 | 0x5E9—0x5EB bytes[3] unknown_5E9_5EB【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## GameOver

970C10重选本地/Online Player并遍历死亡物品史；Client用Host具名最终死亡显示列表与C/S subject，不能复制原版选择路径。

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameOver.zhl；J273 970C10

| 0x0—0x3 i32 active【换算；已核】GameOver实际显示开关 → 共享最终死亡界面存在性；不能触发本地Game::End | 0x4—0x7 bytes[4] unknown_4_7【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x8—0x11B ANM2 sprite【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 同步实际资源／独立姿态；绘制前必覆写的量按具体pass派生，不调用Update | 0x11C—0x12B bytes[16] unknown_11C_12B【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x12C—0x12F EntityConfig* mod_entity_config【资源；SDK声明】击杀者mod配置引用 → 稳定配置资源身份；Mod未声明则具体Gap，不传指针 |
|---|---|---|---|---|

## PauseScreen

这里只能称SDK声明前缀：98DDA0还读取+E04，超出DF4。Game的2363C..25443区域余字节独立保留UNKNOWN，实际Pause承载大小未闭合。

依据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/PauseScreen.zhl/MenuOptions.zhl/CompletionWidget.zhl；J273 98DDA0

| 0x0—0x3 i32 state【本地；SDK声明】本地菜单状态 → 不复制Host选择/恢复/退出命令；world pause在D0 | 0x4—0x7 i32 page【本地；已核】98DDA0按DWORD选择子页；SDK原bool猜测不适用 → 本地 | 0x8—0xB i32 selection【本地；SDK声明】菜单选项 → 不发送 | 0xC—0x2B bytes[32] unknown_C_2B【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x2C—0x13F ANM2 main【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 本地菜单资源与本地布局；展示Host run内容走具名只读投影 | 0x140—0x253 ANM2 mini【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 本地菜单资源与本地布局；展示Host run内容走具名只读投影 | 0x254—0x367 ANM2 stats【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 本地菜单资源与本地布局；展示Host run内容走具名只读投影 | 0x368—0x47B ANM2 my_stuff【容器；已核】内嵌Sprite，递归见ANM2；原生指针／事件位／loaded不传 → 本地菜单资源与本地布局；展示Host run内容走具名只读投影 | 0x47C—0x5AF GenericPrompt prompt【本地；已核】暂停内的本地确认框 → 整体本地，不复用共享world Prompt | 0x5B0—0x837 Menu_Options options【本地；SDK声明】音量/显示/输入/菜单设置 → 不发送或覆写Host偏好 | 0x838—0x83F bytes[8] unknown_838_83F【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x840—0x95B CompletionWidget completion【本地；SDK声明】本地进度贴纸承载 → 不接收Host成就/解锁/存档；如展示Host贴纸只用独立图像结果 | 0x95C—0x95F i32 display_player【换算；已核】98DDA0显示主体索引 → C/S subject由独立run显示投影绑定；不照搬原版index | 0x960—0xDF3 bytes[1172] unknown_960_DF3【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## GameScreenEffects

独立连续视图，真实基址Game+25A98；不是新原生类。剩余UNKNOWN不能由此缩小为零。

依据：Game相对偏移；J273 6F7A80/6FF500/701450；详见职责说明

| 0x0—0x7 ImageStrongPair previous_image【资源；已核】实际Image与本地引用计数control → 该视图从Game+25A98开始；发送具名程序图像内容/可证明的场景recipe，不能只传名称或FBO | 0x8—0xF i32[2] bloom_timers【权威；已核】Game+25AA0/AA4 bloom倒计时/时长生产输入 → 发送正常Host pass已经形成的最终有效性/强度，不重放bloom生产 | 0x10—0x13 f32 bloom_strength【换算；已核】Game+25AA8 bloom设置强度 → 最终pass有效强度；不与已经换算的结果重复发送 | 0x14—0x14 bool resize_request【本地；已核】Game+25AAC窗口resize请求 → 不传；Client拥有自己的窗口 | 0x15—0x17 bytes[3] unknown_15_17【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x18—0x1F i32[2] hallucination_inputs【权威；已核】Game+25AB0/AB4剩余/总时间 → 最终pass的资源/进度/四边形结果承接；ShowHallucination有玩法副作用 | 0x20—0x20 u8 render_feedback【本地；已核】Game+25AB8 Host Render反馈标记 → 不常驻安装或回写；Client只维护自己的绘制反馈 | 0x21—0x23 bytes[3] unknown_21_23【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口 |
|---|---|---|---|---|---|---|---|

## GameFadeView

6F7A80在25B10与25B28等处作分支；必须记录最终有序pass，不能只复制这两组数据后调用完整Game::Render。

依据：Game+25B04..25B37；J273 6F7A80尾部、6F1640与6F6D30

| 0x0—0xF f32[4] primary_rgba【状态；已核】第一整屏覆盖最终RGBA（Game+25B04） → 同步实际最终颜色，不运行退出/存档FSM | 0x10—0x13 bytes[4] unknown_10_13【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x14—0x14 bool render_branch【换算；已核】Game+25B18绘制后的分派选择 → 具名最终pass选择，不放行原流程副作用 | 0x15—0x17 bytes[3] unknown_15_17【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x18—0x27 f32[4] secondary_rgba【状态；已核】第二整屏覆盖最终RGBA（Game+25B1C） → 同步实际被选择的pass；不发送未选路径的旧颜色 | 0x28—0x2B f32 secondary_progress【权威；推断】Game+25B2C由6F1640清零的生产进度 → 不发；最终RGBA承接 | 0x2C—0x2F f32 secondary_speed【权威；已核】Game+25B30由6F1640写入的渐变速率 → 不发；最终RGBA承接 | 0x30—0x33 u32 fadeout_target【权威；已核】Game+25B34退出/转场目标，6F1640校验0..11 → 绝不作为Client命令安装/执行 |
|---|---|---|---|---|---|---|---|

## GameColorView

视图基址Game+6691C；66968..66993原用途未知44B照实保留，未归调试器或填充。

依据：Game+6691C..669AB；J273 6F7F90/6F6DE8/7021A0/804639/803E20

| 0x0—0x0 bool lerp_enabled【权威；已核】颜色渐变生产开关 → 不传 | 0x1—0x3 bytes[3] unknown_1_3【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x4—0x1B ColorModState current【状态；已核】Game+66920最终屏幕色调 → 同步六值 | 0x1C—0x33 ColorModState target【权威；已核】Game+66938渐变目标 → 整份留Host，不因嵌套通用类型标状态而恢复发送 | 0x34—0x4B ColorModState lerp【权威；已核】Game+66950渐变步长 → 整份留Host | 0x4C—0x77 bytes[44] unknown_4C_77【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 | 0x78—0x7F DebugRenderer debug_renderer【本地；SDK声明】Game+66994调试绘制owner → 不传，不开启debug玩法/绘图 | 0x80—0x83 f32 lightning【状态；已核】Game+6699C当前闪电强度 → 同步；NPC/Room/Light均有实际读取 | 0x84—0x87 f32 dizzy【状态；已核】Game+669A0当前眩晕强度 → 同步当前最终值 | 0x88—0x8B f32 dizzy_target【权威；已核】Game+669A4目标 → 不传 | 0x8C—0x8F f32 dizzy_speed【权威；已核】Game+669A8生产速度 → 不传 |
|---|---|---|---|---|---|---|---|---|---|---|

## NightmareSnapshotQueue

Host生产容器；一块32B仅一元素；未知外部对象完整ABI不由此核定。

依据：J273 Show8FBE00及Render8FAC20，目标版实布局，不由通用deque ABI外推

| 0x0—0x3 pointer proxy【本地；已核】容器proxy，Show构造/交换并维护回指 → 不传 | 0x4—0x7 pointer map【本地；已核】本进程32B单元素块指针表 → 不传地址 | 0x8—0xB u32 map_size【权威；已核】Host块指针表容量 → 不传容器容量 | 0xC—0xF u32 offset【权威；已核】Host队首偏移 → 不传队列生产历史 | 0x10—0x13 u32 size【权威；已核】Host队列元素数 → 只消费正常已采用结果；不发送历史集合 |
|---|---|---|---|---|

## NightmareSnapshot

只说明该32B记录的存储和用途，不宣称所指实体/配置对象内部完整。

依据：J273 8FC916—8FC943分配/复制；8FAF1A—8FB178四slot消费

| 0x0—0x17 std::string config_text【权威；已核】正常Show保存的配置字符串 → 不重放生产；最终已选资源/显示结果承接 | 0x18—0x1B pointer source0【本地；已核】第一个已选显示来源对象/配置；kind由实际Item/Boss slot决定 → 不发/不跨线程持有；正常消费后复制显示结果 | 0x1C—0x1F pointer source1【本地；已核】第二个已选显示来源对象/配置；含实体专用Sprite路径 → 不发/不跨线程持有；正常消费后复制显示结果 |
|---|---|---|

## BossOverlayDefinition

五份静态生产定义不下行；每份3B未知不称padding。

依据：J273 41C2F0构造/41C3D0解析/41CD30选择，Game ctor五份步长38h

| 0x0—0x3 i32 id【权威；已核】bossoverlays.xml entry id → 生产配置留Host | 0x4—0x1B std::string anm2_path【权威；已核】配置选择的ANM2路径 → 只传当前实际Sprite资源，不发五份定义 | 0x1C—0x33 std::string animation_name【权威；已核】配置选择的动画名 → 只传当前实际姿态 | 0x34—0x34 bool persist【权威；已核】配置persist；Update据此决定结束 → 不运行Update生产 | 0x35—0x37 bytes[3] unknown_35_37【未知；未知】未取得字段用途或填充证明 → 不读取、不发送；保留具名缺口，不宣称语义完整 |
|---|---|---|---|---|

## BossOverlay

静态识别的原生连续对象；并非SDK GenericPopup。直属0B未知，五份定义共15B未知、ANM2未知另计。

依据：Game+1D294..1D4C7；6ED4B0构造；6F0F76加载bossoverlays.xml；41CC80绘制

| 0x0—0x3 i32 active【换算；已核】当前Boss覆盖画面是否存在 → 当前有效贡献的存在性；已有贡献批次表达时不再重复active位 | 0x4—0x7 u32 selected_entry【权威；已核】当前0..4配置选择；entry3的Update有进度/切房副作用 → 当前实际Sprite承接；选择命令不上线 | 0x8—0x11B ANM2 sprite【容器；已核】当前实际Sprite/资源；Render重设视口比例 → 当前资源/姿态；比例按Client视口除480/270派生 | 0x11C—0x233 BossOverlayDefinition[5] definitions【容器；已核】从bossoverlays.xml解析的五份Host生产定义 → 不发送生产定义；嵌套未知保持 |
|---|---|---|---|

## 外部范围与跨域唯一归属

| owner | 范围 | 归属 | 理由 |
|---|---|---|---|
| Game | [0x1D294,0x1D4C8) | 已识别BossOverlay，见独立字节表 | 6F0F76加载bossoverlays.xml；直属未知0B，嵌套定义15B及ANM2未知另计 |
| Game | [0x24430,0x24440) | UNKNOWN | PauseScreen SDK前缀外；本轮未核这16B的完整用途 |
| Game | [0x24440,0x24441) | 本地/原版Online排除 | PauseScreen+E04；98DF32/98DF69和98E23D/98E2B9控制密码显示，98D1AD切换；不读取原版Online补C/S显示 |
| Game | [0x24441,0x25444) | UNKNOWN | 其余Pause后缀及后续owner边界不能由已核密码1B外推，继续保留 |
| Game | [0x25A84,0x25A8C) | 已有Run/Presentation唯一字段 | 最终screenShakeOffset8B；原生Render/压力板文字消费；不重复传输 |
| Game | [0x25A90,0x25A98) | 已有Run/Room显示字段 | darkness两f32都有Render消费者，不因名称target裁掉 |
| Game | [0x25F58,0x25F60) | 换算 | retrovision剩余/总时相；Room804500最终强度/seed78选择由具名pass结果承接，不在Client计时 |
| Game | [0x669EC,0x669F0) | 换算 | 6F7A80最终白色整屏覆盖alpha=clamp(value/90,0,1)；Update的转场/计时留Host |
| Manager | [0x4A8B8,0x4A8BC) | 已有D0 | manager_frame一次全局承载，主线程绘制借用与恢复 |
| Player | [0x1FF8,0x2008) | 资源 | 两份实际Image strong；由具名Player绘制资源用途承接，地址与control本地 |
| Player | [0x20A8,0x20B0) | 本地pair＋Presentation内容 | 动态PlayerHUD strong owner；内容按PlayerHUD表，不当作不存在 |
