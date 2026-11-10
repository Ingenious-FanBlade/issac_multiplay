# 第5步：Player 统一原生字节账本

**后续实施（2026-11-09）**：正式Player源与生成已迁入版本47，见[实现说明](implementation-5-player.md)。本文件保留设计时点和证据；构建不读取设计JSON。Plus未知与ABI模型限制保持，实际采集／安装及最终显示缓存仍待后续步骤，没有提前测试。

日期：2026-11-09。完整数据见 [player-byte-ledger.json](player-byte-ledger.json)。这是P0—P8设计后的统一归属，不是正式布局源；当前布局版本仍46，未生成／编译／运行测试。

EntityPlayer 从0到2E9F按字段及间隙展开，继承Entity和内嵌对象递归复用下表。未知仍是未知；保存体中没有命名的大片字节没有因“不发送”而改判成已知Host字段。EntityPlayerPlus的A8大小及后半偏移为官方声明顺序＋既有Win32 Release STL布局的静态模型，明确保留证据等级，不称官方DLL ABI实测。

## 范围与计数

| 根类型 | 原生范围 | 按递归叶子统计的字节归属 |
|---|---|---|
| EntityPlayer | 0—0x2E9F | 本地 2151B；未知 3707B；权威 1565B；换算 85B；状态 2624B；资源 492B；容器 772B；派生 488B；引用 52B |
| EntityPlayerPlus | 0—0xA7 | 本地 16B；换算 7B；未知 9B；容器 80B；权威 56B |

以上是原生对象表示的字节数，不是网络负载。外部costume、效果、History、回放节点及保存容器元素按实际数量另计，不能塞入一个固定“Player总UNKNOWN”掩盖变长未知。嵌套在Host／本地域的ANM2不因通用ANM2字段标状态就自动上线。

## 本轮补充判定

- Player+268C是原版通信气泡／表情／打字提示承载：74E930构造，42CB40实际绘制，6F66E0与8EC3B0经原版设备／通信输入驱动。C/S不发送其激活位、原版通信选择、帧计数或消息／资源名表，不把其两份ANM2加进P0；Client原生该分支保持关闭，C/S若需通信UI须由独立已声明界面域提供，本轮不引入该功能。这里不是GenericPrompt，不能按大小或邻近字符串猜类名。
- Player+2E94的12B vector由7A4EE4—7A4F5B遍历ID并移除效果后清尾，是Host待移除队列，不能作为当前效果列表发送。
- 保存体20C0—267B按GameStatePlayer的实际SDK字段与UNKNOWN间隙展开；SavedHistory只采用容器已知部分，不能借live History的owner证据解释保存体尾字。
- PlayerPlus.camoOverride的正面消费者在UpdateEffect补丁，最终ANM／flags／颜色已有owner，不单独发送；customCache调度、健康类型回调重入标志、设备恢复索引与restoreTwinID留Host／本地。真正的有效健康显示结果归第8步具体adapter。
- EntityPlus中optional<u32>内部D—F的3B尾隙单独列UNKNOWN；旧版46复合8B字段的外部未知统计不代替细化账本。没有因此修改当前生成文件。

- 移动箱当前与保存体的vector真正元素是EntitySaveState（78h），不是FriendBall用EntityDesc（1Ch）；已逐字节登记，泛称storage的字段仍列UNKNOWN，不以已知类型冒充已知用途。

## 原生单行表

每格均包含范围／位宽／类别／用途／去向和证据等级；结构格引用同名类型，根字节完整覆盖不表示所有语义已知。STL头只按本地对象表示记账，禁止复制地址、allocator、capacity或桶结构。为保持原生布局表为单行，长类型横向展示；完整范围同时保存在JSON。

### Vector

原生大小 0x8。J273 SDK 所列 Vector 为两个连续 f32；角度公开成员可由两分量派生。

| 0x0—0x3 f32 x【状态；已核】X 分量 → 继承／嵌套 Vector 的当前已核投影 | 0x4—0x7 f32 y【状态；已核】Y 分量 → 继承／嵌套 Vector 的当前已核投影 |
|---|---|

证据：J273 VA0x9E5BC0/RVA0x5E5BC0 Lerp；0x9E5BE6/0x9E5BEB读两f32、0x9E5BEF写第二分量（imagebase 0x400000）；Vector2.zhl

### ColorMod

原生大小 0x2C。Color 映射到 ColorMod。REP Lua 取色接口逐数组读 _tint、_colorize、_offset；39 个具名常量及 EmberFade 是全局静态颜色，均不映射为实例字段。

| 0x0—0x3 f32 tint_r【状态；已核】tint RGBA 的 R 分量 → 继承／嵌套 ColorMod 的当前已核投影 | 0x4—0x7 f32 tint_g【状态；已核】tint RGBA 的 G 分量 → 继承／嵌套 ColorMod 的当前已核投影 | 0x8—0xB f32 tint_b【状态；已核】tint RGBA 的 B 分量 → 继承／嵌套 ColorMod 的当前已核投影 | 0xC—0xF f32 tint_a【状态；已核】tint RGBA 的 A 分量 → 继承／嵌套 ColorMod 的当前已核投影 | 0x10—0x13 f32 colorize_r【状态；已核】colorize RGBA 的 R 分量 → 继承／嵌套 ColorMod 的当前已核投影 | 0x14—0x17 f32 colorize_g【状态；已核】colorize RGBA 的 G 分量 → 继承／嵌套 ColorMod 的当前已核投影 | 0x18—0x1B f32 colorize_b【状态；已核】colorize RGBA 的 B 分量 → 继承／嵌套 ColorMod 的当前已核投影 | 0x1C—0x1F f32 colorize_a【状态；已核】colorize RGBA 的 A 分量 → 继承／嵌套 ColorMod 的当前已核投影 | 0x20—0x23 f32 offset_r【状态；已核】offset RGB 的 R 分量 → 继承／嵌套 ColorMod 的当前已核投影 | 0x24—0x27 f32 offset_g【状态；已核】offset RGB 的 G 分量 → 继承／嵌套 ColorMod 的当前已核投影 | 0x28—0x2B f32 offset_b【状态；已核】offset RGB 的 B 分量 → 继承／嵌套 ColorMod 的当前已核投影 |
|---|---|---|---|---|---|---|---|---|---|---|

证据：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ColorMod.zhl；REPENTOGON\LuaInterfaces\LuaColor.cpp:8-80、132-169；J273 ctor VA0x402760/RVA0x2760初始化11字；VA0x67E940/RVA0x27E940按tint[0..3]、colorize[4..7]、offset[8..10]处理float；主审已复读

### KColor

原生大小 0x14。KColor+0x10已由J273颜色空间转换核为u32标记，不是padding或无语义opaque32；0=RGB、1=sRGB、2=HSL已直接观察，未将三标签当作穷举合法值集合。四个f32分量按颜色空间解释，第四分量为Alpha。 当前原生安装仅接受已核标签0/1/2；其他标签应报未支持缺口并补证，不据此宣称它们在全程序中非法。

| 0x0—0x3 f32 red【状态；已核】红色分量 → 继承／嵌套 KColor 的当前已核投影 | 0x4—0x7 f32 green【状态；已核】绿色分量 → 继承／嵌套 KColor 的当前已核投影 | 0x8—0xB f32 blue【状态；已核】蓝色分量 → 继承／嵌套 KColor 的当前已核投影 | 0xC—0xF f32 alpha【状态；已核】透明度分量 → 继承／嵌套 KColor 的当前已核投影 | 0x10—0x13 u32 color_space【状态；已核】颜色空间标记：已核0=RGB、1=sRGB、2=HSL；不将当前观察标签冒充穷举集合 → 继承／嵌套 KColor 的当前已核投影 |
|---|---|---|---|---|

证据：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\KColor.zhl；docs/design/tx-04-19/entity-application.json 中 KColor RGBA+opaque32；J273 VA0x9E39F0转换前三个f32并复制+0xC Alpha，+0x10颜色空间另行处理；J273 VA0x9E39F0：+0x9E3A01检查输入space=1，+0x9E3A37写输出0；VA0x9E37B0检查RGB0，VA0x9E3BA0检查HSL2/输出0；imagebase 0x400000；主审已复读；VA0x9E3DF0成功转换分支在VA0x9E3EB0写结果标签1；分派表VA0xAB1E00有三组三项；其他标签未作穷举排除

### BlendMode

原生大小 0x14。SDK 声明与 J273 GetMode 写入均确认连续五个 DWORD 的成员顺序；合法值来自官方 Global.zhl 的混合枚举。

| 0x0—0x3 u32 equation【状态；已核】混合方程 → 继承／嵌套 BlendMode 的当前已核投影 | 0x4—0x7 u32 rgb_source_factor【状态；已核】RGB 源因子 → 继承／嵌套 BlendMode 的当前已核投影 | 0x8—0xB u32 rgb_destination_factor【状态；已核】RGB 目标因子 → 继承／嵌套 BlendMode 的当前已核投影 | 0xC—0xF u32 alpha_source_factor【状态；已核】Alpha 源因子 → 继承／嵌套 BlendMode 的当前已核投影 | 0x10—0x13 u32 alpha_destination_factor【状态；已核】Alpha 目标因子 → 继承／嵌套 BlendMode 的当前已核投影 |
|---|---|---|---|---|

证据：J273 VA 0x9E17B0（RVA 0x5E17B0，imagebase 0x400000）BlendMode::GetMode；官方 BlendMode.zhl；Global.zhl:3340-3362，LuaBlendMode.cpp 校验合法枚举

### AnimationState

原生大小 0x20。两个帧数组是独立选择器，不是 _animFrame 的派生值。SetLayerFrame 与 NullFrame 路径分别校验并更新数组；长度来自当前 AnimationData 的 layer_count/null_layer_count，不能使用 ANM2 总层数。 版本32：两组事件位归Host，块3显式预留；is_playing与anim_frame保留，官方InterpolV2.cpp绘制插值读取两者，不以Client停用Update为由删除。

| 0x0—0x3 ANM2* animation_owner【本地；已核】回指所属 ANM2；由上层对象关系确定 → 不上线；沿当前已核归属 | 0x4—0x7 AnimationData* active_animation【资源；已核】当前动画资源项；序号由 ANM2 文件表解析，空指针使用资源空值标记 → 继承／嵌套 AnimationState 的当前已核投影 | 0x8—0xB int* layer_frames【容器；已核】各图层 AnimationFrame 内部序号选择器，长度取当前 AnimationData._layerCount → 继承／嵌套 AnimationState 的当前已核投影 | 0xC—0xF int* null_layer_frames【容器；已核】各空层 AnimationFrame 内部序号选择器，长度取当前 AnimationData._nullLayerCount → 继承／嵌套 AnimationState 的当前已核投影 | 0x10—0x13 f32 anim_frame【状态；已核】动画当前帧值 → 继承／嵌套 AnimationState 的当前已核投影 | 0x14—0x14 bool is_playing【状态；SDK声明】播放标志兼绘制插值输入；直接安装最终值，不授权Client推进动画 → 继承／嵌套 AnimationState 的当前已核投影 | 0x15—0x17 bytes[3] unknown_15_17【未知；未知】SDK 字段间未解释的三个字节；不认作填充 → 不上线；不读取 | 0x18—0x1B i32 currently_triggered_events【权威；已核】Host动画事件消费位；Client安装最终姿态，不推进或重放事件，构造／换动画后本地清零 → 不上线；沿当前已核归属 | 0x1C—0x1F i32 previously_triggered_events【权威；已核】Host动画事件历史位；不表示可靠事件队列，不进入Client状态或漂移比较 → 不上线；沿当前已核归属 |
|---|---|---|---|---|---|---|---|---|

证据：J273 VA 0x404390（RVA 0x4390）SetLayerFrame；VA 0x4040B0（RVA 0x40B0）AdvanceNullLayerPosition；官方 AnimationState.zhl；SetLayerFrame VA0x40439E取state+0 owner，VA0x4043A0访问owner+0x80层数；J273 VA 0x404390（RVA 0x4390）SetLayerFrame；VA 0x4040B0（RVA 0x40B0）AdvanceNullLayerPosition；官方 AnimationState.zhl；J273 VA 0x404390 SetLayerFrame 使用 state+0x08 并按当前动画层映射写选择器；AnimationState.zhl 注释；数组不可从 _animFrame 重建；J273 VA 0x4040B0 AdvanceNullLayerPosition 读写 state+0x0C；计数从当前 AnimationData+0x24 读取；数组不可从 _animFrame 重建；J273 AdvancePosition VA0x4041A0/RVA0x41A0：VA0x4041B2以movss读+0x10、VA0x404239写floor(frame)+1；主审已复读；J273 VA 0x404390（RVA 0x4390）SetLayerFrame；VA 0x4040B0（RVA 0x40B0）AdvanceNullLayerPosition；官方 AnimationState.zhl；REPENTOGON/Patches/InterpolV2.cpp:34读取_isPlaying；版本32复核见docs/design/binary-state/animation-resource-synchronization-review.md；AnimationState.zhl 在 bool +0x14 后对齐至 i32 +0x18；未证明这些字节永不变化；J273 AnimationState::IsEventTriggered 读取 this+0x18（imagebase 0x400000）；官方 AnimationState.zhl；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md；J273 AnimationState::WasEventTriggered 读取 this+0x1C（imagebase 0x400000）；官方 AnimationState.zhl；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md

### LayerState

原生大小 0xA0。LayerState.HasCustomShader 与 HasCustomChampionShader 由 REPENTOGON Anm2Extras.cpp 的 ANM2* / LayerState* 旁表提供，非 J273 +0x31、非 ANM2 +0x108/+0x10C；需第8步旁表布局和资源路径接线。GetSpritesheet 返回实际 Image 对象，+0x98 的资源身份仍需核对名字唯一性/程序图像；+0x08 路径不能替代它，因为 SetSpriteSheet 仅更换 smart pointer、过滤/环绕和 loaded 状态，不改路径字符串。 版本32：sprite_sheet_loaded不上线；J273 ReplaceSpritesheet VA0x407CD0清+30但保留Image，Render经GetSpriteSheet读取实际Image而非该标志。安装实际image_selection与路径元数据分离，不能因路径变更抢先LoadGraphics；本地owner负责loaded，等待／失败不得伪装为空资源。

| 0x0—0x3 LayerData* layer_data【派生；推断】由所属 ANM2 文件及数组序号定位的资源定义 → 不上线；沿当前已核归属 | 0x4—0x7 ANM2* animation_owner【本地；SDK声明】所属 ANM2 回指，不作为网络状态 → 不上线；沿当前已核归属 | 0x8—0x1F std::string spritesheet_path【资源；SDK声明】spritesheet路径元数据资源号；与实际Image选择独立，路径变更不授权Client抢先重载 → 继承／嵌套 LayerState 的当前已核投影 | 0x20—0x23 u32 min_filter_mode【状态；SDK声明】缩小过滤模式 → 继承／嵌套 LayerState 的当前已核投影 | 0x24—0x27 u32 mag_filter_mode【状态；SDK声明】放大过滤模式 → 继承／嵌套 LayerState 的当前已核投影 | 0x28—0x2B u32 wrap_s_mode【状态；SDK声明】S 方向环绕模式 → 继承／嵌套 LayerState 的当前已核投影 | 0x2C—0x2F u32 wrap_t_mode【状态；SDK声明】T 方向环绕模式 → 继承／嵌套 LayerState 的当前已核投影 | 0x30—0x30 bool sprite_sheet_loaded【本地；SDK声明】本地图层资源加载／重载维护标志；ReplaceSpritesheet可置false同时保留旧Image，不跨进程复制，也不按Host原值覆盖Client加载结果 → 不上线；沿当前已核归属 | 0x31—0x31 u8 unknown_31【未知；未知】J273 LoadGraphics 将该字节清零；具体语义未明，绝非已证填充 → 不上线；不读取 | 0x32—0x32 bool flip_x【状态；SDK声明】水平翻转 → 继承／嵌套 LayerState 的当前已核投影 | 0x33—0x33 bool flip_y【状态；SDK声明】垂直翻转 → 继承／嵌套 LayerState 的当前已核投影 | 0x34—0x3B Vector size【状态；SDK声明】图层尺寸 → 继承／嵌套 LayerState 的当前已核投影；展开Vector×1 | 0x3C—0x3F f32 rotation【状态；SDK声明】图层旋转角度 → 继承／嵌套 LayerState 的当前已核投影 | 0x40—0x47 Vector position【状态；SDK声明】图层位置 → 继承／嵌套 LayerState 的当前已核投影；展开Vector×1 | 0x48—0x73 ColorMod color【状态；SDK声明】图层 ColorMod → 继承／嵌套 LayerState 的当前已核投影；展开ColorMod×1 | 0x74—0x74 bool visible【状态；SDK声明】图层显示开关 → 继承／嵌套 LayerState 的当前已核投影 | 0x75—0x77 bytes[3] unknown_75_77【未知；未知】SDK 布局间未说明的三个字节；不认作填充 → 不上线；不读取 | 0x78—0x8B BlendMode blend_mode【状态；已核】图层混合模式的五 DWORD 值 → 继承／嵌套 LayerState 的当前已核投影；展开BlendMode×1 | 0x8C—0x8F u32 render_flags【状态；SDK声明】图层渲染位标志 → 继承／嵌套 LayerState 的当前已核投影 | 0x90—0x97 Vector crop_offset【状态；SDK声明】图层裁剪偏移 → 继承／嵌套 LayerState 的当前已核投影；展开Vector×1 | 0x98—0x9F KAGE_SmartPointer_ImageBase image_selection【资源；推断】当前实际ImageBase选择，Null使用0xFFFF；Host由ImageBase._name形成资源身份，Client重建引用；重名／程序图像身份仍待第8步核 → 继承／嵌套 LayerState 的当前已核投影 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

证据：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；LayerState.zhl +0x08 std::string；LuaSprite.cpp GetSpritesheetPath 直接返回 GetSpritesheetPath().c_str()；F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\LayerState.zhl；J273 VA 0x407DA0 LoadGraphics；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md；J273 VA 0x407DA0 LoadGraphics 内 VA 0x407F7A 写 LayerState+0x31=0；imagebase 0x400000；LayerState.zhl 在 visible +0x74 后以 BlendMode +0x78 对齐；没有不变证据；J273 VA 0x9E17B0（RVA 0x5E17B0，imagebase 0x400000）BlendMode::GetMode；官方 BlendMode.zhl；SmartPointer.zhl：+0 image、+4 refcount control block；KAGE_Graphics_Image.zhl：ImageBase::_name 在 +0x3C；LayerState::SetSpriteSheet 会替换整个 smart pointer

### ANM2

原生大小 0x114。版本32：loaded原始字节归本地，资源有无并入file_name的可空资源号；Host的未选择、Client的等待／失败与成功准备分开。is_playing／anim_frame／playback_speed因官方InterpolV2绘制消费者保留。Sprite.HasCustomShader 与 HasCustomChampionShader 来自 REPENTOGON Anm2Extras.cpp 外部旁表，不是 ANM2 +0x108/+0x10C；未声明 Sprite public_owner，待第8步接入 shader 旁表和实际 Image 资源路径。_animDefaultName 是可写 std::string：J273 Load 拷贝缓存串到 +0x18，SetAnimation 只改当前 AnimationData，不构成不可变证明。 资源路径／实际Image／shader选择仍同步，本地指针、refcount及加载结果不上线。默认动画名此轮未裁剪，仍保留已有有界串与证据边界；不声称它是不可变文件派生。

| 0x0—0x17 std::string file_name【资源；SDK声明】可空ANM2文件资源选择；Host loaded=false编码0xFFFF，true编码文件资源号；不以路径非空猜测存在性 → 继承／嵌套 ANM2 的当前已核投影 | 0x18—0x2F std::string default_animation_name【容器；已核】默认动画名字符串；load 阶段读取，保留有界字节串，不当作不可变文件派生量 → 继承／嵌套 ANM2 的当前已核投影 | 0x30—0x4F AnimationState main_state【状态；SDK声明】主动画状态与独立图层/空层帧选择器 → 继承／嵌套 ANM2 的当前已核投影；展开AnimationState×1 | 0x50—0x6F AnimationState overlay_state【状态；SDK声明】叠加动画状态与独立图层/空层帧选择器 → 继承／嵌套 ANM2 的当前已核投影；展开AnimationState×1 | 0x70—0x70 bool overlay_render_first【状态；SDK声明】叠加动画先绘制开关 → 继承／嵌套 ANM2 的当前已核投影 | 0x71—0x73 bytes[3] unknown_71_73【未知；未知】SDK 字段间未解释字节；不认作填充 → 不上线；不读取 | 0x74—0x77 AnimationData* animation_data【派生；已核】由 ANM2 文件动画表建立的资源数组首址 → 不上线；沿当前已核归属 | 0x78—0x7B i32 animation_count【派生；已核】由 ANM2 文件动画表确定的动画数量 → 不上线；沿当前已核归属 | 0x7C—0x7F LayerState* layer_states【容器；已核】按 layer_count 遍历图层实例状态 → 继承／嵌套 ANM2 的当前已核投影 | 0x80—0x83 u32 layer_count【派生；已核】由 ANM2 文件图层数组确定的图层数 → 不上线；沿当前已核归属 | 0x84—0x87 std::string* null_layer_names【派生；已核】由文件建立的空层名称表 → 不上线；沿当前已核归属 | 0x88—0x8B u32 null_layer_count【派生；已核】由文件建立的空层名称数量 → 不上线；沿当前已核归属 | 0x8C—0x8F std::string* event_names【派生；已核】由文件建立的事件名表 → 不上线；沿当前已核归属 | 0x90—0x93 u32 event_count【派生；已核】由文件建立的事件数量 → 不上线；沿当前已核归属 | 0x94—0x9B Vector offset【状态；SDK声明】绘制偏移 → 继承／嵌套 ANM2 的当前已核投影；展开Vector×1 | 0x9C—0xA3 Vector scale【状态；SDK声明】绘制缩放 → 继承／嵌套 ANM2 的当前已核投影；展开Vector×1 | 0xA4—0xA7 f32 rotation【状态；SDK声明】全局旋转 → 继承／嵌套 ANM2 的当前已核投影 | 0xA8—0xD3 ColorMod color【状态；SDK声明】全局颜色 → 继承／嵌套 ANM2 的当前已核投影；展开ColorMod×1 | 0xD4—0xFF ColorMod champion_color【状态；SDK声明】精英色调 → 继承／嵌套 ANM2 的当前已核投影；展开ColorMod×1 | 0x100—0x100 bool flip_x【状态；SDK声明】水平翻转 → 继承／嵌套 ANM2 的当前已核投影 | 0x101—0x101 bool flip_y【状态；SDK声明】垂直翻转 → 继承／嵌套 ANM2 的当前已核投影 | 0x102—0x103 bytes[2] unknown_102_103【未知；未知】SDK 字段间未解释字节；不认作填充 → 不上线；不读取 | 0x104—0x107 f32 playback_speed【状态；SDK声明】播放速度兼绘制插值的半帧比例输入；不在Client推进动画 → 继承／嵌套 ANM2 的当前已核投影 | 0x108—0x108 u8 unknown_108【未知；未知】初始化为零但语义未明；不作为 shader 状态 → 不上线；不读取 | 0x109—0x109 bool loaded【本地；已核】本地ANM2构造／加载／重置结果；Host只用它确定file_name的可空资源选择，Client由资源owner完成准备后维护，不直接传布尔字节 → 不上线；沿当前已核归属 | 0x10A—0x10B bytes[2] unknown_10A_10B【未知；未知】+0x109 与 shadowLayerIndex +0x10C 间未解释字节 → 不上线；不读取 | 0x10C—0x10F i32 shadow_layer_index【派生；已核】资源中名称为 shadow 的 LayerState 数组序号；不存在时为 -1 → 不上线；沿当前已核归属 | 0x110—0x113 u32 render_flags【状态；SDK声明】ANM2 渲染位标志 → 继承／嵌套 ANM2 的当前已核投影 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

证据：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ANM2.zhl；J273 VA 0x407AF0 Load、VA 0x405F50 SetAnimation；J273 Render VA0x4059E0读取+109；Reset VA0x403530清文件／资源及loaded，Load VA0x407B52置1；版本32见docs/design/binary-state/animation-resource-synchronization-review.md；J273 VA 0x407AF0 Load 调 VA 0x409680；VA 0x4099D4/0x4099E9 将 cache 字符串复制到 this+0x18；SetAnimation VA 0x405F50 只切换 AnimationData。REP ANM2.zhl；J273 VA 0x404390（RVA 0x4390）SetLayerFrame；VA 0x4040B0（RVA 0x40B0）AdvanceNullLayerPosition；官方 AnimationState.zhl；F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ANM2.zhl；J273 VA 0x407AF0 Load、VA 0x405F50 SetAnimation；ANM2.zhl 中 overlayRenderFirst +0x70 与 AnimationData* +0x74 间未命名；ANM2.zhl +0x7C；LayerState 实例步长 0xA0；J273 VA 0x407AF0 按该数组顺序处理图层；ANM2.zhl 在 flip bool +0x100/+0x101 与 playbackSpeed +0x104 之间留空；F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ANM2.zhl；J273 VA 0x407AF0 Load、VA 0x405F50 SetAnimation；REPENTOGON/Patches/InterpolV2.cpp按_playbackSpeed计算lerpappend；版本32见docs/design/binary-state/animation-resource-synchronization-review.md；J273 ANM2 ctor VA 0x403530 初始化周围字段；REP shader 扩展另在 Anm2Extras 旁表；J273 Load VA 0x407AF0 的 VA 0x407B52 写 +0x109=1；属于本地资源生命周期状态；J273 IsLoaded VA0x41B820/RVA0x1B820直接return this[0x109]，主审已复读；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md；ANM2.zhl 与 J273 Load 对该两字节未给出语义；J273 Load VA 0x407AF0：VA 0x407B59 初始化 -1；VA 0x407BC9 在层名等于 shadow 时写当前层序号

### RNG

原生大小 0x10。J273 SetSeed 写入全部四 DWORD；RandomInt/RandomFloat/Next 读取三个 shift 并更新 seed。此类型是 authority-only，不加入发送块。

| 0x0—0x3 u32 seed【权威；已核】伪随机状态 seed；仅权威端推进 → 不上线；沿当前已核归属 | 0x4—0x7 u32 shift_1【权威；已核】Xorshift 参数一；仅权威端使用 → 不上线；沿当前已核归属 | 0x8—0xB u32 shift_2【权威；已核】Xorshift 参数二；仅权威端使用 → 不上线；沿当前已核归属 | 0xC—0xF u32 shift_3【权威；已核】Xorshift 参数三；仅权威端使用 → 不上线；沿当前已核归属 |
|---|---|---|---|

证据：J273 VA 0x7DF4A0 SetSeed、0x7DF4E0 RandomInt、0x7DF540 RandomFloat、0x7DF5B0 Next（imagebase 0x400000）；官方 RNG.zhl

### BitSet128

原生大小 0x10。四个u32的顺序由官方构造函数参数ll/lh/hl/hh明确；J273 SetTearFlags按16B值参数原样复制，按128位保留。

| 0x0—0x3 u32 ll【状态；已核】低 64 位低 DWORD → 继承／嵌套 BitSet128 的当前已核投影 | 0x4—0x7 u32 lh【状态；已核】低 64 位高 DWORD → 继承／嵌套 BitSet128 的当前已核投影 | 0x8—0xB u32 hl【状态；已核】高 64 位低 DWORD → 继承／嵌套 BitSet128 的当前已核投影 | 0xC—0xF u32 hh【状态；已核】高 64 位高 DWORD → 继承／嵌套 BitSet128 的当前已核投影 |
|---|---|---|---|

证据：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\BitSet128.zhl；成员与四个 DWORD 的位运算定义；J273 SetTearFlags VA0x672BF0/RVA0x272BF0，VA0x673619 movups读取值参数16B、VA0x67361D movups写Tear+0x428；主审已复读

### EntityPtr

原生大小 0x4。EntityPtr::SetReference 会维护 Entity 内引用登记链；网络只解析稳定成员索引，不能直接复制指针。

| 0x0—0x3 Entity* entity【引用；已核】实体智能引用；只发送成员表内实体序号，不发送地址 → 继承／嵌套 EntityPtr 的当前已核投影 |
|---|

证据：J273 VA 0x858760 EntityPtr::SetReference（RVA 0x458760，imagebase 0x400000）；官方 EntityPtr.zhl

### ColorParams

原生大小 0x3C。版本32：整个ColorParams时间线留Host，布局11只保留原生字节归属与空块，不发送元素。J273 Entity::Update VA0x6AA10E—0x6AA22F计算覆盖／淡出，并在0x6AA221调用0x4029B0写主Sprite+0xA8最终颜色；Client不运行该Update。Duration分母与Lifespan剩余次数仍不同，负Lifespan仍递减这一原生事实不变；不再需要颜色锚点／读取时钟／容器writer。未知+3A/+3B仍为2B，ColorMod本身的11个分量继续完整同步。

| 0x0—0x3 i32 priority【权威；已核】颜色覆盖优先级；仅Host合成队列消费，Client接收最终颜色 → 不上线；沿当前已核归属 | 0x4—0x2F ColorMod color【权威；已核】覆盖输入ColorMod；仅Host合成，不能与最终Entity.Color／ANM2.color混同 → 不上线；沿当前已核归属 | 0x30—0x33 i32 duration【权威；已核】Duration／+30，淡出比例分母；原生颜色更新不递减，仅Host消费 → 不上线；沿当前已核归属 | 0x34—0x37 i32 lifespan【权威；已核】Lifespan／+34，每次颜色更新有符号减一，负值也递减、仅结果0删项；留Host，不再编码颜色锚点 → 不上线；沿当前已核归属 | 0x38—0x38 bool fadeout【权威；已核】是否按Lifespan／Duration淡出；仅Host颜色合成消费 → 不上线；沿当前已核归属 | 0x39—0x39 bool shared【权威；已核】颜色共享／传播标志；由Host处理关联实体的颜色结果，Client不重演传播 → 不上线；沿当前已核归属 | 0x3A—0x3B bytes[2] unknown_3A_3B【未知；未知】SDK 类型大小尾端尚未解释的两个字节 → 不上线；不读取 |
|---|---|---|---|---|---|---|

证据：官方 ColorParams.zhl；REPENTOGON\LuaInterfaces\LuaColorParams.cpp（Get/Set Duration、Lifespan、Fadeout、Shared）；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md；ColorParams.zhl +0x30；官方 LuaColorParams.cpp 的 GetDuration/SetDuration 直接读取写入 _duration1；第3步核对J273颜色更新循环，递减位置为+34，+30仅作比例分母；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md；ColorParams.zhl +0x34；官方 LuaColorParams.cpp 的 GetLifespan/SetLifespan 直接读取写入 _duration2；J273 Entity::Update VA0x6AA122读+34、VA0x6AA12A dec、VA0x6AA12E写，VA0x6AA13B检查0并删除；主审已复读；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md；ColorParams.zhl 声明到 +0x39；没有证据可把 +0x3A..+0x3B 认作填充

### ActiveItemDesc

原生大小 0x20。REP ZHL 的 ActiveItemDesc +0x04 _unk 在 J273 AddCollectible 写入路径中已核为 item_pool；SDK 的 PartialCharge int 声明错误，J273 Lua closure 按 float 读写 +0x18。 版本39：item_pool／timed_recharge_cooldown留Host；HUD实际消费Item／Charge／BatteryCharge／SubCharge／PartialCharge／VarData共24B，四槽同时保留。

| 0x0—0x3 i32 item_id【状态；已核】主动道具配置 ID；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。 → 继承／嵌套 ActiveItemDesc 的当前已核投影 | 0x4—0x7 i32 item_pool【权威；已核】Host获得／保存／掉落时的来源池，AddCollectible VA0x759864写入；当前Player/HUD展示不读取。 不采集／编码／安装或比较原值。 → 不上线；沿当前已核归属 | 0x8—0xB i32 charge【状态；已核】充能数；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。 → 继承／嵌套 ActiveItemDesc 的当前已核投影 | 0xC—0xF i32 battery_charge【状态；已核】电池充能数；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。 → 继承／嵌套 ActiveItemDesc 的当前已核投影 | 0x10—0x13 i32 sub_charge【状态；已核】子充能数；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。 → 继承／嵌套 ActiveItemDesc 的当前已核投影 | 0x14—0x17 i32 timed_recharge_cooldown【权威；已核】Host主动道具充能更新输入，VA0x789E50/0x78A35B等消费；HUD读取最终charge／partial等，不推进本地充能。 不采集／编码／安装或比较原值。 → 不上线；沿当前已核归属 | 0x18—0x1B f32 partial_charge【状态；已核】部分充能进度；公开类型为 float；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。 → 继承／嵌套 ActiveItemDesc 的当前已核投影 | 0x1C—0x1F i32 var_data【状态；已核】道具关联 VarData；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。 → 继承／嵌套 ActiveItemDesc 的当前已核投影 |
|---|---|---|---|---|---|---|---|

证据：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ActiveItemDesc.zhl；docs/design/tx-04-20/tx-04-20-state-installation.md:37-38；player-i39-source.md:71；docs/design/binary-state/player-equipment-plus-synchronization-review.md；docs/design/tx-04-20/tx-04-20-state-installation.md:38；J273 AddCollectible VA 0x758F00 于 VA 0x759864 写入最后一个 item pool 参数；docs/design/binary-state/player-equipment-plus-synchronization-review.md；player-i39-source.md:71；J273 VA 0x863B93 写 getter closure 的 offset 0x18、VA 0x863BB9 以 PartialCharge 注册；VA 0x863C4E 写 setter closure offset；VA 0x8A4510/0x8A4570 以 movss 读写（imagebase 0x400000）；docs/design/binary-state/player-equipment-plus-synchronization-review.md

### Weapon

原生大小 0x44。本轮Player设计采用实际0x44基础前缀；+8为真实公共攻击EntityPtr，不是GetMainEntity；UNKNOWN共16B（18—1F及3C—43）。动态factory大小逐类见下表。P5只保留具体充能公式需要的max_fire_delay/charge与Modifiers位，5/6/8/12/15恒零不传；类型及Whip15+44真实主引用独立保留。当前生成46仍有旧通用布局13/135，本账本不更改它们。

| 0x0—0x3 u32 vtable【本地；已核】原生 Weapon 虚表指针 → 不上线；沿当前已核归属 | 0x4—0x7 Entity* owner【权威；已核】Host玩法Weapon所属关系；当前充能条公式不读取owner。Client工厂使用本地承载身体，退休前owner=null；不发送Host引用。 → 不上线；沿当前已核归属 | 0x8—0xB EntityPtr common_attack_link【权威；已核】真实公共EntityPtr攻击链接，9C5050/9D4240/9D0990/9E0D50清理会Remove并解绑；不是通用展示MainEntity → Host；不传 | 0xC—0xF f32 fire_delay【权威；SDK声明】Host Weapon::Update VA0x9C5220使用owner速度倍率递减的射击冷却；当前展示不用，不作为Client时钟。 → 不上线；沿当前已核归属 | 0x10—0x13 f32 max_fire_delay【状态；SDK声明】最大射击冷却；Player758850／Familiar62C0C0经Weapon slot7→slot6纯字段公式消费；保留，不运行Update／Fire。 → P5仅十种充能类型；Modifiers还须按具体公式mask | 0x14—0x17 f32 charge【状态；SDK声明】武器充能；Player758850／Familiar62C0C0经Weapon slot7→slot6纯字段公式消费；保留，不运行Update／Fire。 → P5仅十种充能类型；Modifiers还须按具体公式mask | 0x18—0x1F bytes[8] unknown_18_1F【未知；未知】J273 ctor 初始化 +0x18=1.0f、+0x1C=0；字段语义未核 → 不上线；不读取 | 0x20—0x27 Vector direction【权威；SDK声明】Host Fire／瞄准输入；Player最终朝向、Sprite与攻击实体单独安装，当前装备展示不消费此Vector。 → 不上线；沿当前已核归属 | 0x28—0x2F f32[2] axis_parameters【权威；推断】IsAxisAligned VA0x9C5090同时读取+28/+2C及owner／道具；不是原生绘制输入，两个f32类型事实保留但不传输。 → 不上线；沿当前已核归属 | 0x30—0x33 i32 weapon_type【状态；SDK声明】HUD RenderActiveItem VA0x836093区分type12，Familiar60C190区分type15；由工厂动态类及初始化类型核对，保留。 → 继承／嵌套 Weapon 的当前已核投影 | 0x34—0x37 u32 modifiers【换算；SDK声明】武器修改标志；Player758850／Familiar62C0C0经Weapon slot7→slot6纯字段公式消费；保留，不运行Update／Fire。 → P5仅十种充能类型；Modifiers还须按具体公式mask | 0x38—0x3B i32 num_fired【权威；SDK声明】Host发射历史／公开查询值；当前Player/HUD/Familiar展示不消费。 → 不上线；沿当前已核归属 | 0x3C—0x3F u32 unknown_3C【未知；未知】SDK 未声明；J273 ctor 写零，语义未核 → 不上线；不读取 | 0x40—0x43 u32 unknown_40【未知；未知】J273 Weapon ctor 写入范围超过 SDK sizeof(Weapon)=0x40；+0x40 字节归属未核 → 不上线；不读取 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

证据：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582；J273 VA0x9C4ED9写Weapon vtable，主审已复读；F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Weapon.zhl；J273 VA 0x9C4EB0 ctor、0x9C5200 Init、0x9C5090 IsAxisAligned（imagebase 0x400000）；player-application.json:62582；docs/design/binary-state/player-equipment-plus-synchronization-review.md；5-player-p3-p5.md；J273 Weapon ctor VA0x9C4EB0：VA0x9C4EED/0x9C4F58写+0x18=1.0f，VA0x9C4F3A写+0x1C=0；语义未核，主审已复读；J273 Weapon::IsAxisAligned VA 0x9C5090 读取 this+0x28 与 +0x2C；具体含义未核；docs/design/binary-state/player-equipment-plus-synchronization-review.md；J273 Weapon ctor VA 0x9C4EB0 初始化 +0x3C；J273 Weapon ctor VA 0x9C4EB0 写 this+0x40；player-application.json:62582

### TemporaryEffect

原生大小 0x10。Player P8专用裁剪：每元素config/count必需，只有collectible704保留cooldown；末尾4B仍UNKNOWN。有序列表不按ID合并，非704冷却变化不触发P8。当前版本46通用布局14仍为旧三值投影，本账本不回写它。

| 0x0—0x3 ItemConfig_Item* item【资源；已核】配置资源身份，必须保留(type,id)及本地config.cooldown；90A7A0/90A900按类别与id查找，Player/HUD直接消费。 → P8.item | 0x4—0x7 i32 count【状态；已核】叠加数；HUD835CDF→90A900及Player769E90等查询读取，不由库存或costume推导。 → P8.count | 0x8—0xB i32 cooldown【状态；已核】条件显示值：758952和836E35对collectible704读取正cooldown，角色不限27；其他记录同偏移为Host生产冷却，不递减／不下行。 → P8仅collectible704；其他Host | 0xC—0xF u32 unknown_0C【未知；未知】SDK 对象大小 0x10 中未声明的末尾 DWORD；尚未证明可当填充 → UNKNOWN不上线 |
|---|---|---|---|

证据：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\TemporaryEffect.zhl；docs/design/tx-04-19/entity-application.json TemporaryEffect.*；docs/design/binary-state/player-equipment-plus-synchronization-review.md；5-player-p6-p8.md；F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\TemporaryEffect.zhl；docs/design/tx-04-19/entity-application.json TemporaryEffect.*；J273 AddEffect VA0x90A090/RVA0x50A090：VA0x90A0E3加count(+4)，VA0x90A0E9加cooldown(+8)，插入元素步长0x10；主审已复读；docs/design/binary-state/player-equipment-plus-synchronization-review.md；TemporaryEffect.zhl 只命名 +0x00 item、+0x04 count、+0x08 cooldown；J273 +0x0C 用途未核；J273 VA0x90A194把新记录尾字置0，VA0x90A0D0/0x90A0D3按16B复制传入记录；不能仅以常见零值判为padding，主审已复读

### EntityPlus

原生大小 0x10。本账本进一步展开optional内部表示；+D—F三字节不因MSVC输出padding就当原生语义已核。当前版本46生成表未随本次设计改变。

| 0x0—0x3 void* vptr【本地；已核】官方实际动态虚表 → 本地owner | 0x4—0x4 bool flying_value【换算；已核】仅engaged时读取payload；Null／false分开 → 继承E9.is_flying_override | 0x5—0x5 bool flying_engaged【换算；已核】optional<bool>判别字节 → 继承E9.is_flying_override | 0x6—0x7 bytes[2] unknown_6_7【未知；未知】类成员间隙尚无原生语义 → 不读取／不传 | 0x8—0xB u32 water_value【换算；已核】仅engaged时读取；有效零必须保留 → 继承E9.water_clip_override | 0xC—0xC bool water_engaged【换算；已核】optional<u32>判别字节 → 继承E9.water_clip_override | 0xD—0xF bytes[3] unknown_d_f【未知；未知】optional内部尾隙；既有8B复合字段不等于8B都具有值语义 → 不读取／不传 |
|---|---|---|---|---|---|---|

证据：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/EntityPlus.h；mod/src/game/client_player_sdk_abi.cpp；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}

### Entity

原生大小 0x410。第3步Entity公共目录；未知字节、SDK声明和跨域依赖明确保留。E0含形态元数据，E1/E4/E5在Player切换为body。正式采集与原生应用在第10/13步接线；此处不代表完整同步已运行。 版本30移除33个公共范围的线上职责（32个Host玩法范围、1个本地工厂标志）；保留valid/exists/dead及真实绘制参数，详见entity-common-synchronization-review.md。 版本31进一步撤销公共valid的wire/validator/handler，原生资格归本地owner；exists/dead保留，Player激活资格的结构信息由第5步P7另行接管，见entity-lifecycle-synchronization-review.md。

| 0x0—0x3 u32 vtable【本地；已核】原生虚表；不作为线上状态或地址 → 不上线；沿当前已核归属 | 0x4—0x13 bytes[16] unknown_4_13【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x14—0x17 f32 pending_damage【权威；已核】Host待结算伤害缓冲；Client不执行TakeDamage或伤害结算，最终HP和表现另按状态安装 → 不上线；沿当前已核归属 | 0x18—0x1F bytes[8] unknown_18_1f【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x20—0x23 u32 authority_order【换算；已核】Host index 转绘制同Z排序值；不覆盖Client本地索引或用作网络身份 → 继承／嵌套 Entity 的当前已核投影 | 0x24—0x27 u32 collision_index【本地；SDK声明】当前进程碰撞索引；不作为网络身份 → 不上线；沿当前已核归属 | 0x28—0x2B u32 type【状态；已核】实体Type；形态变化走结构事务 → 继承／嵌套 Entity 的当前已核投影 | 0x2C—0x2F u32 variant【状态；已核】实体Variant；形态变化走结构事务 → 继承／嵌套 Entity 的当前已核投影 | 0x30—0x33 u32 subtype【状态；已核】实体SubType；形态变化走结构事务 → 继承／嵌套 Entity 的当前已核投影 | 0x34—0x37 u32 spawner_type【状态；SDK声明】生成者Type；分类和结构描述 → 继承／嵌套 Entity 的当前已核投影 | 0x38—0x3B u32 spawner_variant【状态；SDK声明】生成者Variant；分类和结构描述 → 继承／嵌套 Entity 的当前已核投影 | 0x3C—0x47 bytes[12] unknown_3c_47【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x48—0x15B ANM2 sprite【状态；已核】Entity内嵌主Sprite，字节事实复用ANM2，分别投影外观/动画/图层 → 继承／嵌套 Entity 的当前已核投影；展开ANM2×1 | 0x15C—0x15F f32 shadow_size【状态；SDK声明】阴影尺寸 → 继承／嵌套 Entity 的当前已核投影 | 0x160—0x167 bytes[8] unknown_160_167【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x168—0x16F u64 flags【状态；已核】EntityFlags 64位位集 → 继承／嵌套 Entity 的当前已核投影 | 0x170—0x170 bool valid【本地；已核】本进程对象/列表/池有效资格，由Client构造、发布和退休owner维护，不进入公共E2；Player活动/备用身体的原生激活资格由第5步P7结构角色单独接管，不能一律置true，也不声称等于room_member。 → 本地owner；Player激活语义见P7 | 0x171—0x171 bool visible【状态；已核】公开可见开关 → 继承／嵌套 Entity 的当前已核投影 | 0x172—0x172 bool exists【状态；已核】公共Render VA0x6AC4ED及多类Render读取+172；成员在表不代表可绘制，保留当前展示存在状态。 → 继承／嵌套 Entity 的当前已核投影 | 0x173—0x173 bool dead【状态；已核】Player Render VA0x785F3B和Room Light VA0x7FF848读取+173；死亡展示与成员退休不同，保留。 → 继承／嵌套 Entity 的当前已核投影 | 0x174—0x174 bytes[1] unknown_174_174【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x175—0x175 u8 unknown_175【未知；未知】slot4在暂存位置时写1，Init写0；所有读取者及用途尚未闭合，不据两次赋值判定为可复制bool → 不上线；不读取 | 0x176—0x176 bytes[1] unknown_176_176【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x177—0x177 bool removed_by_factory【本地；已核】当前进程工厂删除/REP EntityManager记账字节；Client由本地构造/退休owner维护，不写入Host原值。规范第11节的只读资格仍适用于需要该字节的Host生命周期观测。 → 不上线；沿当前已核归属 | 0x178—0x183 bytes[12] unknown_178_183【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x184—0x187 i32 grid_collision_class【状态；已核】格子碰撞类别 → 继承／嵌套 Entity 的当前已核投影 | 0x188—0x18B i32 entity_collision_class【状态；SDK声明】公共PostRender VA0x6ACCA7/0x6ACCCD、Laser PostRender VA0x956FC9/0x956FFC按+188选择调试碰撞几何颜色，保留。 → 继承／嵌套 Entity 的当前已核投影 | 0x18C—0x18D bytes[2] unknown_18c_18d【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x18E—0x18F u16 entity_plus_holder_upper【本地；已核】EntityPlusHolder地址高16位；不可进入复制区间 → 不上线；沿当前已核归属 | 0x190—0x190 bool collides_with_grid【权威；SDK声明】Host本tick格子碰撞结果；用于玩法后续处理，Client不重演格子碰撞。 → 不上线；沿当前已核归属 | 0x191—0x191 bytes[1] unknown_191_191【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x192—0x193 u16 entity_plus_holder_lower【本地；已核】EntityPlusHolder地址低16位；不可进入复制区间 → 不上线；沿当前已核归属 | 0x194—0x1A3 bytes[16] unknown_194_1a3【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x1A4—0x1AB Vector screen_position_cache【本地；已核】当前Client屏幕/Camera坐标缓存；已核Render由世界坐标和屏幕参数计算，不跨进程复制 → 不上线；沿当前已核归属 | 0x1AC—0x1B7 std::vector<ColorParams> color_params【权威；SDK声明】Host有序颜色覆盖／淡出队列；Client不运行颜色合成，传Entity.Color与主Sprite最终ColorMod即可；容器头与元素均不传输 → 不上线；沿当前已核归属 | 0x1B8—0x1E3 ColorMod color【状态；已核】当前最终颜色 → 继承／嵌套 Entity 的当前已核投影；展开ColorMod×1 | 0x1E4—0x20F ColorMod splat_color【状态；已核】溅射颜色 → 继承／嵌套 Entity 的当前已核投影；展开ColorMod×1 | 0x210—0x213 i32 fire_damage_cooldown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x214—0x217 i32 damage_countdown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x218—0x21B i32 freeze_countdown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x21C—0x21F i32 poison_countdown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x220—0x223 i32 slowing_countdown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x224—0x227 i32 charmed_countdown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x228—0x22B i32 confusion_countdown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x22C—0x22F i32 midas_freeze_countdown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x230—0x233 i32 fear_countdown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x234—0x237 i32 burn_countdown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x238—0x23B i32 boss_status_effect_cooldown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x23C—0x23F i32 shrink_countdown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x240—0x277 bytes[56] unknown_240_277【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x278—0x27B f32 poison_damage【权威；SDK声明】Host毒伤害强度；最终HP和毒效果表现另行同步，Client不计算伤害。 → 不上线；沿当前已核归属 | 0x27C—0x27F f32 burn_damage【权威；SDK声明】Host灼烧伤害强度；最终HP和火效果表现另行同步，Client不计算伤害。 → 不上线；沿当前已核归属 | 0x280—0x2D3 bytes[84] unknown_280_2d3【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x2D4—0x2D7 f32 mass【权威；SDK声明】Host碰撞/物理模拟参数；Client不运行碰撞或原生Interpolate，展示使用最终姿态。SDK声明等级保留。 → 不上线；沿当前已核归属 | 0x2D8—0x2DF bytes[8] unknown_2d8_2df【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x2E0—0x2E3 i32 poison_damage_timer【权威；已核】Host毒/火伤害节奏递增相位（mod 20）；不是展示时钟输入。Client接收最终HP、颜色和效果成员，不执行毒/火伤害，不采集、不编码、不安装此相位。 → 不上线；沿当前已核归属 | 0x2E4—0x2E7 i32 burn_damage_timer【权威；已核】Host毒/火伤害节奏递增相位（mod 20）；不是展示时钟输入。Client接收最终HP、颜色和效果成员，不执行毒/火伤害，不采集、不编码、不安装此相位。 → 不上线；沿当前已核归属 | 0x2E8—0x2EB i32 bleeding_countdown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x2EC—0x2EF bytes[4] unknown_2ec_2ef【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x2F0—0x2F3 i32 magnetized_countdown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x2F4—0x2F7 i32 baited_countdown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x2F8—0x2FB i32 knockback_countdown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x2FC—0x2FF i32 weakness_countdown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x300—0x303 i32 ice_countdown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x304—0x307 i32 brimstone_mark_countdown【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x308—0x30F Vector knockback_direction【权威；SDK声明】Host击退/位置更新输入；展示安装最终位置、速度和姿态，不重演击退。 → 不上线；沿当前已核归属 | 0x310—0x313 bytes[4] unknown_310_313【未知；未知】附属owner/引用候选，生命周期及用途未闭合，不复制 → 不上线；不读取 | 0x314—0x31B Vector pause_held_position【权威；已核】Host位置更新的暂停保存坐标；+31C仍正时回写最终Position。Client不运行该路径，接收最终Position，不以当前坐标伪造Host保存历史。 → 不上线；沿当前已核归属 | 0x31C—0x31F i32 pause_timer【权威；已核】Host状态持续/伤害/击退/暂停玩法计时；Client不运行Entity Update或位置更新，不重演到期清标志/生成/伤害。展示读取已同步的最终flags、颜色、Sprite、位置和子类表现，不采集、不编码、不安装此计数。 → 不上线；沿当前已核归属 | 0x320—0x327 bytes[8] unknown_320_327【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x328—0x32B u32 frame_base【状态；已核】Host帧空间中的FrameCount基准；原值发送，FrameCount=world.game_frame-frame_base → 继承／嵌套 Entity 的当前已核投影 | 0x32C—0x32F i32 spawn_grid_index【状态；已核】出生格子序号，未绑定为-1；不是帧计数 → 继承／嵌套 Entity 的当前已核投影 | 0x330—0x333 EntityConfig_Entity* selected_config【资源；已核】原生实际选中的配置项；可为空，与REP按当前Type/Variant/SubType重新查询配置的API分别处理；发送稳定资源序号 → 继承／嵌套 Entity 的当前已核投影 | 0x334—0x33B Vector target_position【状态；已核】目标位置 → 继承／嵌套 Entity 的当前已核投影；展开Vector×1 | 0x33C—0x343 Vector position【状态；SDK声明】当前坐标 → 继承／嵌套 Entity 的当前已核投影；展开Vector×1 | 0x344—0x34B Vector staged_position【权威；已核】Host原生Interpolate槽保存的半帧前坐标；Client不调用该槽。只同步最终位置/速度；如需网络展示插值，以已应用快照构建本地历史，不复制Host这份历史。 → 不上线；沿当前已核归属 | 0x34C—0x353 Vector position_offset【状态；SDK声明】位置偏移 → 继承／嵌套 Entity 的当前已核投影；展开Vector×1 | 0x354—0x357 i32 render_z_offset【状态；已核】绘制Z偏移 → 继承／嵌套 Entity 的当前已核投影 | 0x358—0x35B f32 depth_offset【状态；已核】公开DepthOffset；不是SDK +0x2B0 → 继承／嵌套 Entity 的当前已核投影 | 0x35C—0x35F i32 sorting_layer【状态；已核】绘制排序层 → 继承／嵌套 Entity 的当前已核投影 | 0x360—0x367 Vector velocity【状态；SDK声明】当前速度 → 继承／嵌套 Entity 的当前已核投影；展开Vector×1 | 0x368—0x36B bytes[4] unknown_368_36b【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x36C—0x36F f32 friction【权威；已核】Host运动/Interpolate输入；Client不运行原生半帧运动，展示使用最终位置、速度和Sprite。 → 不上线；沿当前已核归属 | 0x370—0x373 f32 size【状态；SDK声明】公共/Laser PostRender VA0x6ACC90/0x956FB4及Room Light VA0x7FFD4C读取+370；保留展示/调试几何尺寸。 → 继承／嵌套 Entity 的当前已核投影 | 0x374—0x37B Vector size_multi【状态；已核】Familiar展示helper VA0x6473C5与公共PostRender VA0x6ACCF8读取+374；保留最终尺寸倍率。 → 继承／嵌套 Entity 的当前已核投影；展开Vector×1 | 0x37C—0x37F bytes[4] unknown_37c_37f【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x380—0x383 f32 hit_points【状态；已核】当前生命值 → 继承／嵌套 Entity 的当前已核投影 | 0x384—0x387 f32 max_hit_points【状态；已核】最大生命值 → 继承／嵌套 Entity 的当前已核投影 | 0x388—0x38B f32 collision_damage【权威；SDK声明】Host接触伤害裁决输入；Client不重演碰撞伤害，最终HP和效果成员照常同步。 → 不上线；沿当前已核归属 | 0x38C—0x39B bytes[16] unknown_38c_39b【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x39C—0x39F f32 speed_multiplier【状态；SDK声明】Effect Render variant172/subtype1 VA0x6039A0读取+39C参与展示方向旋转；保留，不能只按名称归运动玩法。 → 继承／嵌套 Entity 的当前已核投影 | 0x3A0—0x3AF bytes[16] unknown_3a0_3af【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x3B0—0x3B0 bool invincible【权威；SDK声明】Host伤害可受性裁决开关；不是Player独立伤害闪烁计时。Client同步最终死亡/HP/颜色/姿态，不执行伤害裁决。 → 不上线；沿当前已核归属 | 0x3B1—0x3B3 bytes[3] unknown_3b1_3b3【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x3B4—0x3B7 f32 shrink_status_1【状态；SDK声明】缩小插值状态1 → 继承／嵌套 Entity 的当前已核投影 | 0x3B8—0x3BB f32 shrink_status_2【状态；SDK声明】缩小插值状态2 → 继承／嵌套 Entity 的当前已核投影 | 0x3BC—0x3BF EntityPtr parent【引用；已核】实体引用 parent；经当前成员表与EntityPtr owner恢复 → 继承／嵌套 Entity 的当前已核投影 | 0x3C0—0x3C3 EntityPtr child【引用；已核】实体引用 child；经当前成员表与EntityPtr owner恢复 → 继承／嵌套 Entity 的当前已核投影 | 0x3C4—0x3C7 EntityPtr target【引用；已核】实体引用 target；经当前成员表与EntityPtr owner恢复 → 继承／嵌套 Entity 的当前已核投影 | 0x3C8—0x3CB EntityPtr spawner【引用；已核】实体引用 spawner；经当前成员表与EntityPtr owner恢复 → 继承／嵌套 Entity 的当前已核投影 | 0x3CC—0x3CF u32 incoming_reference_tree【本地；已核】反向EntityPtr观察者树的本地根节点地址，随本地引用owner重建 → 不上线；沿当前已核归属 | 0x3D0—0x3D3 u32 incoming_reference_control【未知；未知】反向引用集合相邻控制字；仅Ctor清零不足以证明归属/计数语义，保持未知，不复制 → 不上线；不读取 | 0x3D4—0x3D7 EntityPtr minecart【引用；SDK声明】实体引用 minecart；经当前成员表与EntityPtr owner恢复 → 继承／嵌套 Entity 的当前已核投影 | 0x3D8—0x3DB bytes[4] unknown_3d8_3db【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x3DC—0x3EB RNG drop_rng【权威；已核】掉落RNG种子与游标只属于Host；不发送、不安装到Client → 不上线；沿当前已核归属 | 0x3EC—0x3EF u32 init_seed【状态；已核】构造和表现种子；不是网络身份 → 继承／嵌套 Entity 的当前已核投影 | 0x3F0—0x407 bytes[24] unknown_3f0_407【未知；未知】SDK与现有J273资料尚未解释；不复制、不当作填充 → 不上线；不读取 | 0x408—0x40B LuaBridgeRef* lua_bridge_ref【本地；SDK声明】当前进程Lua桥引用，不发送 → 不上线；沿当前已核归属 | 0x40C—0x40F bytes[4] unknown_40c_40f【未知；未知】对象尾部未解释字节，不当作填充 → 不上线；不读取 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

证据：mod/binding/world_layout.json:classes.Entity；REPENTOGON-1.1.3/libzhl/functions/Entity.zhl；docs/design/binary-state/entity-common-evidence.md；J273 HasMortalDamage getter VA0x6A8810 movss xmm0,[ecx+14]，VA0x6A8815与+380 HP比较；Init VA0x6A4D92清零；主审已复读原指令；mod/binding/world_layout.json:fields.index；client_entity_model.cpp:authority_index；client_entity_draw_native.cpp；mod/binding/world_layout.json:fields.type；J273 VA0x6A4A35写+0x28；mod/binding/world_layout.json:fields.variant；J273 VA0x6A4A3B写+0x2C；mod/binding/world_layout.json:fields.subtype；J273 VA0x6A4A41写+0x30；J273 ctor VA0x6A398E lea ecx,[edi+48]，VA0x6A39BB调用ANM2 ctor VA0x402B00；主审已复读指令边界；嵌套语义见第2步ANM2；mod/binding/world_layout.json:fields.flags；Init VA0x6A4D7B/0x6A4D85；mod/binding/world_layout.json:fields.valid；2026-11-09 J273原始指令/伪代码复读，见entity-common-synchronization-review.md；版本31复核：ctor VA0x6A39D4清零，AddEntity VA0x413D0B检查/0x413D9C置1，Player RenderHead 0x783707消费角色资格；docs/design/binary-state/entity-lifecycle-synchronization-review.md；5-player-p6-p8.md；mod/binding/world_layout.json:fields.visible；mod/binding/world_layout.json:fields.exists；2026-11-09 J273原始指令/伪代码复读，见entity-common-synchronization-review.md；版本31复核独立性：docs/design/binary-state/entity-lifecycle-synchronization-review.md；旧Esau exists=true/valid=false反例及J273 Kill/Remove分离保留；mod/binding/world_layout.json:fields.dead；2026-11-09 J273原始指令/伪代码复读，见entity-common-synchronization-review.md；版本31复核独立性：docs/design/binary-state/entity-lifecycle-synchronization-review.md；旧Esau exists=true/valid=false反例及J273 Kill/Remove分离保留；J273 Entity slot4 VA0x6AC2A0；VA0x6AC320 mov byte [edi+175h],1；主审已复读；规范第11节 USER_ACCEPTED_CURRENT_ENTITY_BYTE；mod/binding/world_layout.json:removed_by_factory；Init VA0x6A4BD6跨字段清零不代替当前资格；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；官方Entity.zhl +184；J273 IsFlying VA0x6A7C90读+184并与0/1/2/3比较；不得将Init VA0x6A4BDF的+188=4写误当成+184；REPENTOGON-1.1.3/libzhl/functions/Entity.zhl；docs/design/binary-state/entity-common-evidence.md；2026-11-09 J273原始指令/伪代码复读，见entity-common-synchronization-review.md；官方 repentogon/Patches/EntityPlus.cpp:InitEntityPlusHolder/GetEntityPlusHolder；REPENTOGON-1.1.3/libzhl/functions/Entity.zhl；docs/design/binary-state/entity-common-evidence.md；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；J273 vtable槽5=VA0x6AC4E0；VA0x6AC646/0x6AC64A写+1A4/+1A8，VA0x6AC64F调用主Sprite Render；依赖position/position_offset和本地屏幕参数；主审已复读；REPENTOGON-1.1.3/libzhl/functions/Entity.zhl；docs/design/binary-state/entity-common-evidence.md；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md；mod/src/game/client_entity_fields.inc：tx-04-19 J273内置Lua注册和访问器逐项主审；REPENTOGON-1.1.3/libzhl/functions/Entity.zhl；docs/design/binary-state/entity-common-evidence.md；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA396 / dec 0x6AA3A0 / store 0x6AA3A1测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA3A7 / dec 0x6AA3B1 / store 0x6AA3B2测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA3B8 / store 0x6AA3CD测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA3EE测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA42C测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA476测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA4E3测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA646测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA52B测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA5B7测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA9DC / dec 0x6AA9E6 / store 0x6AA9E7测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA58F测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；J273 Entity::Update VA0x6A9CF0，毒/火分支对应flag0x40/0x1000，读取+2E0/+2E4后inc；entity-common-evidence.md，主审采用前核原始指令；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA67D测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA77E测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA7C2测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA926测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA8C7测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA95D测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl 命名；J273 Entity::Update VA0x6A9CF0：0x6AA9A7测试正值后递减；entity-common-evidence.md。锚点是条件换算合同，不声称与global frame无条件等速。；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；J273位置路径VA0x6C03B0；VA0x6C089C取+314、0x6C08A2写+33C，VA0x6C08A8取+318、0x6C08AE写+340；Init VA0x6A4E87初始化该Vector；主审已复读原始指令；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；SDK Entity.zhl +31C；J273位置路径VA0x6C03B0，在VA0x6C0887读、0x6C088D测试正、0x6C0891减一、0x6C0892写回；主审已复读；全部调用资格未闭合，需实际clock见证；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；J273 VA0x6A5290/RVA0x2A5290 FrameCount getter：Game+0x25A70-Entity+0x328；Init VA0x6A4B2B读全局帧、VA0x6A4B31写+328；登记VA0x84F385（主审见entity-common-evidence.md）；J273 Init VA0x6A4B46写-1；公开getter VA0x6EB980读取+0x32C（VA0x6EB986是ret）；详见entity-common-evidence.md及implementation-3.md主审；J273 GetBossID VA0x7DF5F0取Entity+330，非空VA0x7DF5FA取config+30；登记VA0x84F3E4/0x84F3E9；主审已复读；J273 vtable VA0xB18224槽4=0x6AC2A0；VA0x6AC2E8写+344、VA0x6AC2F1写+348；主审已复读原始指令；旧tx-03的6B0EB0不是当前点位；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；mod/src/game/client_entity_fields.inc：tx-04-19 J273内置Lua注册和访问器逐项主审；client_entity_fields.inc；J273公共Lua绑定主审见tx-04-19；mod/src/game/client_entity_fields.inc：tx-04-19 J273内置Lua注册和访问器逐项主审；REPENTOGON-1.1.3/libzhl/functions/Entity.zhl；docs/design/binary-state/entity-common-evidence.md；同步职责：docs/design/binary-state/entity-common-synchronization-review.md（2026-11-09静态复核；不改变原证据等级）；mod/src/game/client_entity_fields.inc：tx-04-19 J273内置Lua注册和访问器逐项主审；REPENTOGON-1.1.3/libzhl/functions/Entity.zhl；docs/design/binary-state/entity-common-evidence.md；2026-11-09 J273原始指令/伪代码复读，见entity-common-synchronization-review.md；mod/binding/world_layout.json；Entity.zhl；J273 EntityPtr::SetReference VA0x858760；J273 ctor VA0x6A3E32分配节点，VA0x6A3E50把新节点写至this+3CC；EntityPtr::SetReference VA0x858760及ClearReferences VA0x6A8060维护本地反向集合；J273 ctor VA0x6A3E2B写this+3D0=0；EntityPtr::SetReference维护反向集合；本地树计数；mod/binding/world_layout.json:drop_rng；第2步RNG布局；mod/binding/world_layout.json:init_seed；J273 Init VA0x6A4A47

### PlayerTemporaryEffects

原生大小 0x18。

| 0x0—0x3 u32 cache_mask【权威；已核】待属性重算mask，不是vptr → Host；不传 | 0x4—0xF vector<TemporaryEffect> entries【容器；已核】完整有界有序身份／count；仅704 cooldown → P8.effects | 0x10—0x10 bool evaluate【权威；已核】Host重算请求 → Host；不传 | 0x11—0x11 bool disabled【状态；已核】效果查询禁用，独立于列表空值 → P8.effects_disabled | 0x12—0x13 bytes[2] unknown_12_13【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x14—0x17 Player* owner【本地；已核】本地承载Player回指 → 本地owner |
|---|---|---|---|---|---|

证据：5-player-p6-p8.md

### PlayerCostumeMap

原生大小 0x10。

| 0x0—0x3 i32 index【换算；已核】非负真实costume下标；负值只消费符号 → P6.layer_choice.costume | 0x4—0x7 i32 layer_id【状态；已核】index非负时读取；负时不读残留并编码0 → P6.layer_choice.layer_id | 0x8—0xB i32 priority【权威；已核】层竞争生产输入，最终map已同步 → Host；不传 | 0xC—0xC bool body【状态；已核】备份绘制选择身体／头部比例，负index也保留 → P6.layer_choice.is_body_layer | 0xD—0xF bytes[3] unknown_d_f【未知；未知】尾部尚未解释；不以对象大小或初始化零值判填充 → 不上线；不采集／不安装 |
|---|---|---|---|---|

证据：5-player-p6-p8.md

### CostumeSpriteDesc

原生大小 0x130。

| 0x0—0x113 ANM2 sprite【嵌套；已核】当前Sprite，按ANM2递归归属 → P0；物理22—24；展开ANM2×1 | 0x114—0x117 ItemConfig* item【资源；已核】可空(type,id)，Room Light及Render直接消费 → P6.config | 0x118—0x11B i32 priority【权威；已核】排序／生成输入 → Host；不传 | 0x11C—0x11C bool item_animation【权威；已核】动画生产状态，最终ANM2承接 → Host；不传 | 0x11D—0x11D bool flying【状态；已核】飞行层选择 → P6.is_flying | 0x11E—0x11E bool overlay【状态；已核】29/38借画选择overlay → P6.has_overlay | 0x11F—0x11F bool skin_alt【权威；已核】肤色生产输入 → Host；不传 | 0x120—0x127 i32[2] skin_colors【权威；已核】默认／当前肤色生产缓存 → Host；不传 | 0x128—0x129 bool[2] color_and_state_only【权威；已核】overwriteColor／itemStateOnly生产／移除条件 → Host；不传 | 0x12A—0x12B bytes[2] unknown_12a_12b【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x12C—0x12F i32 costume_player_type【权威；已核】生成时角色缓存，不代替P7真实PlayerType → Host；不传 |
|---|---|---|---|---|---|---|---|---|---|---|

证据：5-player-p6-p8.md

### PocketItem

原生大小 0x8。

| 0x0—0x3 i32 id【状态；已核】按type解释，active是slot+1；0为空 → P3.pocket.id | 0x4—0x7 u32 type【状态；已核】空槽也保留真实type，不按主动槽的空策略清零 → P3.pocket.type |
|---|---|

证据：5-player-p3-p5.md

### QueueItemData

原生大小 0x10。

| 0x0—0x3 ItemConfig* item【换算；已核】仅真实queued饰品配置选择；非饰品折Null → P3.queued_trinket_config | 0x4—0x7 i32 charge【权威；已核】将来获得/使用的charge → Host；不传 | 0x8—0xB u32 flags【换算；已核】只投影bit1黄金饰品份额，其他位Host → P3.queued_trinket_golden | 0xC—0xF i32 var_data【权威；已核】未来获得/使用附加数据 → Host；不传 |
|---|---|---|---|

证据：5-player-p3-p5.md

### SmeltedTrinketDesc

原生大小 0x4。

| 0x0—0x1 i16 normal【状态；已核】普通熔炼份数，signed → P3.smelted.normal | 0x2—0x3 i16 golden【状态；已核】黄金熔炼份数，signed → P3.smelted.golden |
|---|---|

证据：5-player-p3-p5.md

### AfterImageFrame

原生大小 0xC。

| 0x0—0x3 i32 duration【状态；已核】原生Render直接转alpha，不由Client递减 → P8.afterimages.duration | 0x4—0xB Vector position【状态；已核】每份实际残影位置 → P8.afterimages.position；展开Vector×1 |
|---|---|

证据：5-player-p6-p8.md

### History

原生大小 0x14。

| 0x0—0xB vector<HistoryItem> items【权威；已核】原始获得史，时间／层／房／pool不作为显示输入发送 → Host；第8步最终图像列表 | 0xC—0xC bool mutation_disabled【权威；已核】Add／Remove禁止位，不是显示禁用开关 → Host；不传 | 0xD—0xF bytes[3] unknown_d_f【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x10—0x13 Player* owner【本地；已核】74E5E1将自身写入live History末字 → 本地owner |
|---|---|---|---|

证据：5-player-p6-p8.md

### HistoryItem

原生大小 0x1C。

| 0x0—0x3 i32 time【权威；已核】原始获得时点，不能替代HUD图像顺序 → Host；不传 | 0x4—0x4 bool trinket【权威；已核】原始历史类别 → Host；不传 | 0x5—0x7 bytes[3] unknown_5_7【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x8—0x1B i32[5] history_values【权威；已核】item_id／stage／stage_type／room_type／pool → Host；不传 |
|---|---|---|---|

证据：5-player-p6-p8.md

### BagOfCraftingOutput

原生大小 0x8。

| 0x0—0x3 i32 collectible【权威；已核】配方输出生产存储，实际结果图像在HUD缓存 → 第8步结果缓存；原值不发 | 0x4—0x7 i32 pool【权威；已核】配方来源池 → Host；不传 |
|---|---|

证据：5-player-p3-p5.md

### EntityDescHost

原生大小 0x1C。

| 0x0—0x17 i32/f32[6] spawn_descriptor【权威；SDK声明】type／variant／subtype／champion／health／maxHealth生成描述 → Host；不传 | 0x18—0x18 bool player_controlled【权威；SDK声明】未来生成对象的控制类别 → Host；不传 | 0x19—0x1B bytes[3] unknown_19_1b【未知；未知】尾部尚未解释；不以对象大小或初始化零值判填充 → 不上线；不采集／不安装 |
|---|---|---|

证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntityPlayer.zhl；F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntityDesc.zhl

### ConsumableData

原生大小 0x20。

| 0x0—0xB i32[3] coins_keys_bombs【权威；SDK声明】备份硬币／钥匙／炸弹，不是live P2 → Host恢复；不发 | 0xC—0xD bool[2] golden_flags【权威；SDK声明】备份黄金钥匙／炸弹 → Host恢复；不发 | 0xE—0xF bytes[2] unknown_e_f【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x10—0x1F i32[4] charges_and_special【权威；SDK声明】魂充能／giga／poop／血充能备份 → Host恢复；不发 |
|---|---|---|---|

证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/ConsumableData.zhl

### ImageStrongPair

原生大小 0x8。

| 0x0—0x3 Image* image【资源；已核】实际选中图像由第8步资源身份承接 → 资源身份，不发地址 | 0x4—0x7 Control* strong_control【本地；已核】本地引用计数／析构控制块 → 本地owner |
|---|---|

证据：5-player-p6-p8.md；mod/src/game/client_player_draw_native.cpp

### PlayerHUDStrongPair

原生大小 0x8。

| 0x0—0x3 PlayerHUD* hud【本地；已核】本地动态HUD承载，状态在第8步 → 第8步HUD逻辑引用，不发地址 | 0x4—0x7 Control* control【本地；已核】本地control vtable／引用计数 → 本地owner |
|---|---|

证据：5-player-p6-p8.md；mod/src/game/client_player_hud_native.cpp

### OptionalDrawSequence

原生大小 0x124。同一原生结构有两个实例；1B20是生产源，1C44只发送当前节点。末4B语义仍未知。

| 0x0—0x3 ListNode* head【本地；已核】真实sentinel → 本地0／1投影列表 | 0x4—0x7 u32 count【派生；已核】Client按当前可绘投影为0／1，不能照Host原count装空链 → P8派生本地列表 | 0x8—0xB ListNode* cursor【派生；已核】head或唯一节点，非地址传输 → P8派生本地cursor | 0xC—0x11F ANM2 sprite【嵌套；已核】仅当前可绘节点存在时同步此Sprite；录制生产序列实例整体留Host → 1C44实例P8；1B20实例Host；展开ANM2×1 | 0x120—0x123 bytes[4] unknown_120_123【未知；未知】尾部尚未解释；不以对象大小或初始化零值判填充 → 不上线；不采集／不安装 |
|---|---|---|---|---|

证据：5-player-p6-p8.md

### DrawRecordNode

原生大小 0x28。

| 0x0—0x7 Node*[2] links【本地；已核】next／prev仅维护本机链 → 本地owner | 0x8—0xF Vector position【状态；已核】当前可绘节点位置；未选节点不读负载 → P8.current_record.position；展开Vector×1 | 0x10—0x17 bytes[8] unknown_10_17【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x18—0x1F i32[2] main_animation_frame【换算；已核】animation负时整节点无绘制，不读其余负载 → P8当前animation／frame | 0x20—0x27 i32[2] second_animation_frame【换算；已核】第二animation负时规范-1／0，不读无效frame → P8当前第二动画／frame |
|---|---|---|---|---|

证据：5-player-p6-p8.md

### SavedHistory

原生大小 0x14。

| 0x0—0xB vector<HistoryItem> items【权威；SDK声明】SDK给定保存History容器；不借live History的owner证明解释其后8B → Host保存；不发 | 0xC—0x13 bytes[8] unknown_c_13【未知；未知】尾部尚未解释；不以对象大小或初始化零值判填充 → 不上线；不采集／不安装 |
|---|---|

证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl

### GameStatePlayer

原生大小 0x5BC。SDK仅命名少量字段；所有间隙逐段UNKNOWN，不把整个保存体称为已知Host字段。

| 0x0—0x3 i32 player_type【权威；SDK声明】恢复的角色类型 → Host恢复；不发 | 0x4—0x1D3 bytes[464] unknown_4_1d3【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1D4—0x1DF vector<EntitySaveState> moving_box【权威；SDK声明】保存的移动箱实体 → Host恢复；不发；外部元素EntitySaveState另计 | 0x1E0—0x33F bytes[352] unknown_1e0_33f【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x340—0x347 i32[2] conception_states【权威；SDK声明】ESSM可劫持的保存字段，不能当live Player缓存 → Host／官方保存adapter；不发 | 0x348—0x373 bytes[44] unknown_348_373【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x374—0x37F vector<FamiliarData> familiar_data【权威；SDK声明】保存熟悉者的数据，元素逐字节另列 → Host恢复；不发；外部元素FamiliarData另计 | 0x380—0x3AF bytes[48] unknown_380_3af【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x3B0—0x3C3 SavedHistory history【权威；SDK声明】保存历史，不借live History初始化／owner证据 → Host恢复；不发；展开SavedHistory×1 | 0x3C4—0x4C3 bytes[256] unknown_3c4_4c3【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x4C4—0x4CB BagOfCraftingOutput crafting_output【权威；SDK声明】保存的配方输出 → Host恢复；不发；展开BagOfCraftingOutput×1 | 0x4CC—0x517 bytes[76] unknown_4cc_517【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x518—0x51B i32 controller_index【权威；SDK声明】Host恢复设备索引；不写Client控制器 → Host；不传 | 0x51C—0x5BB bytes[160] unknown_51c_5bb【未知；未知】尾部尚未解释；不以对象大小或初始化零值判填充 → 不上线；不采集／不安装 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl；J273 74E90D/74E917，保存体ctor74E9A0

### FamiliarData

原生大小 0x34。

| 0x0—0x1F i32[8] saved_identifiers_counters【权威；SDK声明】seed／variant／subtype／coins／hearts／keys／state／roomClear，后二者由ESSM劫持 → Host保存；不发 | 0x20—0x27 bytes[8] unknown_20_27【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x28—0x2B f32 health【权威；SDK声明】保存健康 → Host保存；不发 | 0x2C—0x2F u32 drop_rng_seed【权威；SDK声明】保存掉落RNG种子 → Host保存；不发 | 0x30—0x33 bytes[4] unknown_30_33【未知；未知】尾部尚未解释；不以对象大小或初始化零值判填充 → 不上线；不采集／不安装 |
|---|---|---|---|---|

证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/GameStatePlayer.zhl

### NativeCommunicationDisplay

原生大小 0x808。0x808为下一个2E94字段界定的承载范围，非独立factory大小证据；0x7C4后的68B保持UNKNOWN。

| 0x0—0x0 bool active【本地；已核】原版通信气泡／typing激活位；C/S不复用该通信域 → C/S本地关闭，不发 | 0x1—0x3 bytes[3] unknown_1_3【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x4—0x7 i32 selection【本地；已核】通信选择值；21为点号打字提示 → C/S不接Host原版通信值 | 0x8—0xB u32 display_counter【本地；已核】Render对60取模选择点号个数；由原版通信Update推进 → 不上线，不在C/S推进 | 0xC—0xF i32 duration_control【本地；推断】通信输入写0／120的控制值；完整具体语义未外推 → 不上线 | 0x10—0x10 bool bubble_enabled【本地；已核】选择是否绘制第一个气泡Sprite → C/S本地关闭，不发 | 0x11—0x13 bytes[3] unknown_11_13【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x14—0x127 ANM2 bubble_sprite【本地；已核】原版通信气泡；不是P0身体Sprite → 不发；本地正常资源owner；展开ANM2×1 | 0x128—0x23B ANM2 icon_sprite【本地；已核】原版通信表情／骰子图；不是P0新增槽 → 不发；本地正常资源owner；展开ANM2×1 | 0x23C—0x7C3 std::string[59] names【本地；已核】Up/Happy/Heart/D6/D20等原版通信动画资源名表；由真实string构造维护 → 本地资源，不复制原进程头或消息 | 0x7C4—0x807 bytes[68] unknown_7c4_807【未知；未知】尾部尚未解释；不以对象大小或初始化零值判填充 → 不上线；不采集／不安装 |
|---|---|---|---|---|---|---|---|---|---|---|

证据：J273 74E920—74E930、78495D—7849D9、42BFB0/42CB40；J273 78495D/8EC6AB；J273 42CB40，8EC6A4—8EC6CC；J273 42CB40；J273 6F691E/8EC79B；J273 42BFB0/42CB40；J273 42C0E0/42CB40；J273 42BFB0，42C0E0

### ItemSpoof

原生大小 0x2C。

| 0x0—0x3 void* vptr【本地；已核】实际CollectibleSpoof／TrinketSpoof虚表，不传地址 → 保留本地官方虚表 | 0x4—0x23 unordered_set<i32> blocked【容器；已核】实际blocked逻辑ID集合；头／桶／迭代链本地 → P3.blocked集合投影 | 0x24—0x2B map<string,InnateItems> innate_groups【容器；已核】仅提取全部组persistent与temporary.cachedTotals的有效合计；组名与时限不上线 → P3.innate有效计数投影 |
|---|---|---|

证据：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}

### InnateItems

原生大小 0x4C。

| 0x0—0x1F unordered_map<i32,i32> persistent【容器；已核】每组持久计数参与有效合计；原分组管理不上线 → P3聚合有效计数 | 0x20—0x2B vector<TemporaryInnateItem> temporary_list【权威；已核】原始临时条目／顺序／到期timer留Host，不能当当前count源 → Host；不读取该列表补当前计数 | 0x2C—0x4B unordered_map<i32,i32> temporary_totals【容器；已核】官方查询实际使用的临时有效计数缓存 → P3聚合有效计数 |
|---|---|---|

证据：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}

### TemporaryInnateItem

原生大小 0xC。

| 0x0—0x3 i32 id【权威；已核】定时管理条目ID → Host；不传 | 0x4—0x7 i32 count【权威；已核】定时管理条目份数，正式投影使用cachedTotals → Host；不传 | 0x8—0xB i32 timer【权威；已核】到期管理timer，不使P3每tick变化 → Host；不传 |
|---|---|---|

证据：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h

### PlayerItemSpoofs

原生大小 0x5C。

| 0x0—0x2B ItemSpoof collectible【嵌套；已核】CollectibleSpoof实际对象 → P3 collectible projection；展开ItemSpoof×1 | 0x2C—0x57 ItemSpoof trinket【嵌套；已核】TrinketSpoof实际对象，保留黄金ID语义 → P3 trinket projection；展开ItemSpoof×1 | 0x58—0x5B i32 last_player_type【权威；已核】触发类型变更处理的缓存，不参与有效计数查询 → Host；Client按本地角色初始化，不运行CheckPlayerType |
|---|---|---|

证据：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}

### EntityPlayerPlus

原生大小 0xA8。0xA8及+6C后偏移由官方声明顺序与既有Win32/v143 Release STL类型大小静态推导；不是官方DLL PDB直接证据。独立空隙95—97、9F和公共前缀未知仍保留，不拿编译器padding输出替代原生语义。生成实际reader时须保持官方成员访问owner，不按本表猜写未核ABI。

| 0x0—0xF EntityPlus entity_plus【继承；已核】官方公共nullable前缀，保留Null／false／有效零区别 → 继承E9 nullable覆盖；展开EntityPlus×1 | 0x10—0x6B PlayerItemSpoofs item_spoofs【嵌套；已核】当前blocked＋有效innate来源，不复制容器对象表示 → P3有效投影；展开PlayerItemSpoofs×1 | 0x6C—0x73 set<string> custom_cache_tags【权威；推断】待重算标签集合，生产调度状态 → Host；不传 | 0x74—0x93 unordered_map<string,double> custom_cache_results【权威；推断】tearscap／statmultiplier等属性生产缓存；healthtype有效显示结果按Mod适配／缓存承接 → Host原表；第8步必要有效显示结果另传 | 0x94—0x94 bool requires_evaluate【权威；推断】请求EvaluateItems的工作标志 → Host；不传 | 0x95—0x97 bytes[3] unknown_95_97【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x98—0x9B i32 previous_health_type【权威；推断】回调重入／转型管理缓存，不能直接充当本帧有效HealthType → Host；第8步有效结果adapter | 0x9C—0x9C bool evaluating_health_type【权威；推断】防止递归的当前调用状态，不可远程重放 → Host／本地调用栈，不传 | 0x9D—0x9D bool disable_health_modification【权威；推断】临时禁止属性回调的调用上下文 → Host／本地调用栈，不传 | 0x9E—0x9E bool camo_override【权威；推断】PatchPlayerForceCamo只在UpdateEffect分支改变外观生产；Render消费已产生的Sprite／flags／颜色 → Host；不新增Player热状态 | 0x9F—0x9F bytes[1] unknown_9f_9f【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0xA0—0xA3 i32 previous_controller【本地；推断】原版设备撤销／重新分配缓存，不是C/S组身份 → 不上线；Client中性 | 0xA4—0xA7 i32 restore_twin_id【权威；推断】官方保存恢复配对标签，不是稳定网络Key或P7 twin → Host恢复；P7当前twin独立同步 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

证据：F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/EntityPlus.h；F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ItemSpoofSystem.h；out/repentogon/zhlIsaacCS.pdb: cs::game::player_sdk::{PlusHeader,Spoof,PlusItems,Innate,Timed}；CustomCache.cpp::EvaluateCustomCache；CustomCache.cpp::GetCustomCacheValue/TearsCapHook/StatMultiplierHook；CustomCallbacks.cpp::Entity_Player::GetHealthType；CustomCallbacks.cpp:1583—1732；EntityPlus.cpp::PlayerHasCamoOverride/PatchPlayerForceCamo；LuaEntityPlayer.cpp::Lua_PlayerHasCamoEffect；ASMPatches/ASMPlayerManager.cpp::UnassignController/AssignController；SaveStateManagement/EntitySaveStateEx.cpp；ASMPatches/ASMPlayerManager.cpp:196—214

### WeaponTears

原生大小 0x50。

| 0x0—0x43 Weapon base【继承；已核】已核动态Weapon共同前缀，按P5具体type裁充能输入 → P5分型；展开Weapon×1 | 0x44—0x4F bytes[12] tail_44【未知；未知】动态factory尾部尚未解释；不当填充 → Host／UNKNOWN；不传 |
|---|---|

证据：5-player-p3-p5.md

### WeaponBrimstone

原生大小 0x4C。

| 0x0—0x43 Weapon base【继承；已核】已核动态Weapon共同前缀，按P5具体type裁充能输入 → P5分型；展开Weapon×1 | 0x44—0x47 bytes[4] tail_44【未知；未知】动态factory尾部尚未解释；不当填充 → Host／UNKNOWN；不传 | 0x48—0x4B EntityPtr[1] tail_48【权威；已核】动态攻击链接，Host模拟；不作为展示边 → Host／UNKNOWN；不传 |
|---|---|---|

证据：5-player-p3-p5.md

### WeaponLaser

原生大小 0x50。

| 0x0—0x43 Weapon base【继承；已核】已核动态Weapon共同前缀，按P5具体type裁充能输入 → P5分型；展开Weapon×1 | 0x44—0x47 EntityPtr[1] tail_44【权威；已核】动态攻击链接，Host模拟；不作为展示边 → Host／UNKNOWN；不传 | 0x48—0x4F bytes[8] tail_48【未知；未知】动态factory尾部尚未解释；不当填充 → Host／UNKNOWN；不传 |
|---|---|---|

证据：5-player-p3-p5.md

### WeaponKnife

原生大小 0x54。

| 0x0—0x43 Weapon base【继承；已核】已核动态Weapon共同前缀，按P5具体type裁充能输入 → P5分型；展开Weapon×1 | 0x44—0x53 EntityPtr[4] tail_44【权威；已核】动态攻击链接，Host模拟；不作为展示边 → Host／UNKNOWN；不传 |
|---|---|

证据：5-player-p3-p5.md

### WeaponBombs

原生大小 0x48。

| 0x0—0x43 Weapon base【继承；已核】已核动态Weapon共同前缀，按P5具体type裁充能输入 → P5分型；展开Weapon×1 | 0x44—0x47 EntityPtr[1] tail_44【权威；已核】动态攻击链接，Host模拟；不作为展示边 → Host／UNKNOWN；不传 |
|---|---|

证据：5-player-p3-p5.md

### WeaponRockets

原生大小 0x48。

| 0x0—0x43 Weapon base【继承；已核】已核动态Weapon共同前缀，按P5具体type裁充能输入 → P5分型；展开Weapon×1 | 0x44—0x47 EntityPtr[1] tail_44【权威；已核】动态攻击链接，Host模拟；不作为展示边 → Host／UNKNOWN；不传 |
|---|---|

证据：5-player-p3-p5.md

### WeaponMonstrosLung

原生大小 0x44。

| 0x0—0x43 Weapon base【继承；已核】已核动态Weapon共同前缀，按P5具体type裁充能输入 → P5分型；展开Weapon×1 |
|---|

证据：5-player-p3-p5.md

### WeaponLudovico

原生大小 0x48。

| 0x0—0x43 Weapon base【继承；已核】已核动态Weapon共同前缀，按P5具体type裁充能输入 → P5分型；展开Weapon×1 | 0x44—0x47 EntityPtr[1] tail_44【权威；已核】动态攻击链接，Host模拟；不作为展示边 → Host／UNKNOWN；不传 |
|---|---|

证据：5-player-p3-p5.md

### WeaponTechX

原生大小 0x44。

| 0x0—0x43 Weapon base【继承；已核】已核动态Weapon共同前缀，按P5具体type裁充能输入 → P5分型；展开Weapon×1 |
|---|

证据：5-player-p3-p5.md

### WeaponBoneAxe

原生大小 0x60。

| 0x0—0x43 Weapon base【继承；已核】已核动态Weapon共同前缀，按P5具体type裁充能输入 → P5分型；展开Weapon×1 | 0x44—0x5B EntityPtr[6] tail_44【权威；已核】动态攻击链接，Host模拟；不作为展示边 → Host／UNKNOWN；不传 | 0x5C—0x5F bytes[4] tail_5c【未知；未知】动态factory尾部尚未解释；不当填充 → Host／UNKNOWN；不传 |
|---|---|---|

证据：5-player-p3-p5.md

### WeaponUrn

原生大小 0x54。

| 0x0—0x43 Weapon base【继承；已核】已核动态Weapon共同前缀，按P5具体type裁充能输入 → P5分型；展开Weapon×1 | 0x44—0x4B bytes[8] tail_44【未知；未知】动态factory尾部尚未解释；不当填充 → Host／UNKNOWN；不传 | 0x4C—0x53 EntityPtr[2] tail_4c【权威；已核】动态攻击链接，Host模拟；不作为展示边 → Host／UNKNOWN；不传 |
|---|---|---|

证据：5-player-p3-p5.md

### WeaponSpiritSword

原生大小 0x50。

| 0x0—0x43 Weapon base【继承；已核】已核动态Weapon共同前缀，按P5具体type裁充能输入 → P5分型；展开Weapon×1 | 0x44—0x47 EntityPtr[1] tail_44【权威；已核】动态攻击链接，Host模拟；不作为展示边 → Host／UNKNOWN；不传 | 0x48—0x4F bytes[8] tail_48【未知；未知】动态factory尾部尚未解释；不当填充 → Host／UNKNOWN；不传 |
|---|---|---|

证据：5-player-p3-p5.md

### WeaponFetus

原生大小 0x44。

| 0x0—0x43 Weapon base【继承；已核】已核动态Weapon共同前缀，按P5具体type裁充能输入 → P5分型；展开Weapon×1 |
|---|

证据：5-player-p3-p5.md

### WeaponWhip

原生大小 0x48。

| 0x0—0x43 Weapon base【继承；已核】已核动态Weapon共同前缀，按P5具体type裁充能输入 → P5分型；展开Weapon×1 | 0x44—0x47 EntityPtr[1] tail_44【引用；已核】Whip真实展示主引用 → P5.main_entity |
|---|---|

证据：5-player-p3-p5.md

### EntityPlayer

原生大小 0x2EA0。从0到2E9F全部范围分类；嵌套递归，容器头与外部元素分别记账；没有新增生成布局。

| 0x0—0x40F Entity entity【继承；已核】父类逐字节归属沿现行第3步；valid只作P7结构资格见证 → E0—E9；Player E1/E4/E5/E6走body；展开Entity×1 | 0x410—0x41B bytes[12] unknown_410_41b【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x41C—0x121F ANM2[13] inline_sprites【嵌套；已核】实际固定13份内嵌ANM2，按真实物理槽序，不能合并同文件Sprite → P0；物理19—21；展开ANM2×13 | 0x1220—0x122B vector_CostumeSpriteDesc _costumeSpriteDescs【容器；已核】_costumeSpriteDescs：P6目录／P0 Sprite投影；vector头本地 → P6目录／P0 Sprite投影；vector头本地；外部元素CostumeSpriteDesc另计 | 0x122C—0x131B PlayerCostumeMap[15] _playerCostumeMap【嵌套；已核】_playerCostumeMap：P6最终层映射 → P6最终层映射；展开PlayerCostumeMap×15 | 0x131C—0x131F bytes[4] unknown_131c_131f【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1320—0x1337 std_string _headAnim【容器；已核】_headAnim：P0.head_animation → P0.head_animation | 0x1338—0x133B int _blinkTime【权威；SDK声明】_blinkTime：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x133C—0x133F int _headDirectionTime【权威；SDK声明】_headDirectionTime：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1340—0x1343 int _maxHearts【状态；已核】_maxHearts：P2.max_hearts → P2.max_hearts | 0x1344—0x1347 int _redHearts【状态；已核】_redHearts：P2.red_hearts → P2.red_hearts | 0x1348—0x134B int _eternalHearts【权威；SDK声明】_eternalHearts：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x134C—0x134F int _soulHearts【状态；已核】_soulHearts：P2.soul_hearts → P2.soul_hearts | 0x1350—0x1353 int _blackHearts【状态；已核】_blackHearts：P2.black_hearts → P2.black_hearts | 0x1354—0x1357 i32 jar_hearts【状态；已核】当前资源及主动HUD显示输入 → P2.jar_hearts | 0x1358—0x135B i32 jar_flies【状态；已核】当前资源及主动HUD显示输入 → P2.jar_flies | 0x135C—0x135F int _numKeys【状态；已核】_numKeys：P2.keys → P2.keys | 0x1360—0x1360 bool _hasGoldenKey【状态；已核】_hasGoldenKey：P2.golden_key → P2.golden_key | 0x1361—0x1361 bool _hasGoldenBomb【状态；已核】_hasGoldenBomb：P2.golden_bomb → P2.golden_bomb | 0x1362—0x1363 bytes[2] unknown_1362_1363【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1364—0x1367 int _numBombs【状态；已核】_numBombs：P2.bombs → P2.bombs | 0x1368—0x136B int _numCoins【状态；已核】_numCoins：P2.coins → P2.coins | 0x136C—0x1373 bytes[8] unknown_136c_1373【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1374—0x1377 int _gnawedLeafTimer【权威；SDK声明】_gnawedLeafTimer：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1378—0x137B bytes[4] unknown_1378_137b【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x137C—0x137D short _bloodLustCounter【权威；SDK声明】_bloodLustCounter：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x137E—0x137F i16 extra_lives_adjustment【状态；已核】7B67A6 movsx的有符号额外生命求和项 → P2.extra_lives_adjustment | 0x1380—0x1387 bytes[8] unknown_1380_1387【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1388—0x138B int _metronomeCollectibleID【状态；已核】_metronomeCollectibleID：P3.current_effect_488_id → P3.current_effect_488_id | 0x138C—0x138F bytes[4] unknown_138c_138f【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1390—0x1393 i32 current_effect_191_id【状态；已核】当前191选择ID；不是获得历史 → P3.current_effect_191_id | 0x1394—0x1397 int _monkeyPawCounter【权威；SDK声明】_monkeyPawCounter：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1398—0x1398 bool full_sprite_a【状态；已核】7A2630读取的独立完整Sprite开关 → P0.full_sprite_a | 0x1399—0x1399 bytes[1] unknown_1399_1399【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x139A—0x139A bool full_sprite_b【状态；已核】7A2630读取的独立完整Sprite开关 → P0.full_sprite_b | 0x139B—0x13B7 bytes[29] unknown_139b_13b7【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x13B8—0x13BB int _damageCooldown【状态；已核】_damageCooldown：P0.damage_cooldown → P0.damage_cooldown | 0x13BC—0x13BF int _playerType【状态；已核】_playerType：P7.player_type；形态 → P7.player_type；形态 | 0x13C0—0x13D7 bytes[24] unknown_13c0_13d7【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x13D8—0x13EB Weapon*[5] _weapon【引用；已核】_weapon：P5五槽按实际动态Weapon；地址本地 → P5五槽按实际动态Weapon；地址本地 | 0x13EC—0x1407 bytes[28] unknown_13ec_1407【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1408—0x140B Entity_Effect* _markedTarget【权威；SDK声明】_markedTarget：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x140C—0x140F Entity* _heldEntity【引用；已核】_heldEntity：P7.held → P7.held | 0x1410—0x1453 bytes[68] unknown_1410_1453【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1454—0x1457 Entity_Laser* _megaBlastLaser【权威；SDK声明】_megaBlastLaser：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1458—0x145B int _rUaWizardTimer【权威；SDK声明】_rUaWizardTimer：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x145C—0x145F float _maxfiredelay【权威；SDK声明】_maxfiredelay：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1460—0x1463 float _shotspeed【权威；SDK声明】_shotspeed：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1464—0x146B bytes[8] unknown_1464_146b【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x146C—0x146F float _damage【权威；SDK声明】_damage：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1470—0x1473 float _tearheight【权威；SDK声明】_tearheight：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1474—0x1477 float _tearfallspeed【权威；SDK声明】_tearfallspeed：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1478—0x147B float _tearfallaccel【权威；SDK声明】_tearfallaccel：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x147C—0x147F float _tearrange【权威；SDK声明】_tearrange：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1480—0x148F BitSet128 tear_flags【权威；已核】Player攻击模板；Tear等最终实体按自身字段同步 → Host；不传；展开BitSet128×1 | 0x1490—0x14BB ColorMod tear_color【权威；已核】攻击颜色模板，不用其重放射击 → Host；最终实体颜色各自同步；展开ColorMod×1 | 0x14BC—0x14E7 ColorMod _laserColor【权威；SDK声明】_laserColor：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x14E8—0x14FF f32[6] render_scales【状态；已核】身体／body层／head层六个实际比例，29／38借画读取 → P0六比例 | 0x1500—0x1517 TemporaryEffects _temporaryeffects【嵌套；已核】_temporaryeffects：P8效果投影 → P8效果投影；展开PlayerTemporaryEffects×1 | 0x1518—0x151B int _speedModifier【权威；SDK声明】_speedModifier：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x151C—0x151F int _fireDelayModifier【权威；SDK声明】_fireDelayModifier：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1520—0x1523 int _damageModifier【权威；SDK声明】_damageModifier：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1524—0x1527 int _tearRangeModifier【权威；SDK声明】_tearRangeModifier：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1528—0x152B int _shotSpeedModifier【权威；SDK声明】_shotSpeedModifier：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x152C—0x152F int _luckModifier【权威；SDK声明】_luckModifier：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1530—0x1533 int _donateLuck【权威；SDK声明】_donateLuck：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1534—0x1537 int _potatoPeelerCounter【权威；SDK声明】_potatoPeelerCounter：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1538—0x153F bytes[8] unknown_1538_153f【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1540—0x1543 float _edenSpeed【权威；SDK声明】_edenSpeed：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1544—0x1547 float _edenFireDelay【权威；SDK声明】_edenFireDelay：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1548—0x154B float _edenDamage【权威；SDK声明】_edenDamage：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x154C—0x154F float _edenTearRange【权威；SDK声明】_edenTearRange：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1550—0x1553 float _edenShotSpeed【权威；SDK声明】_edenShotSpeed：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1554—0x1557 float _edenLuck【权威；SDK声明】_edenLuck：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1558—0x155B bytes[4] unknown_1558_155b【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x155C—0x155F float _movespeed【权威；SDK声明】_movespeed：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1560—0x1563 float _luck【权威；SDK声明】_luck：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1564—0x1564 bool _canFly【状态；已核】_canFly：P1.can_fly → P1.can_fly | 0x1565—0x1567 bytes[3] unknown_1565_1567【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1568—0x156B uint32_t _cacheFlags【权威；SDK声明】_cacheFlags：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x156C—0x156C int8_t _tearDisplacement【权威；SDK声明】_tearDisplacement：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x156D—0x156F bytes[3] unknown_156d_156f【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1570—0x1573 float _tearPoisonDamage【权威；SDK声明】_tearPoisonDamage：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1574—0x15F3 ActiveItemDesc[4] _activeDesc【嵌套；已核】_activeDesc：P4四槽六值 → P4四槽六值；展开ActiveItemDesc×4 | 0x15F4—0x15F7 i32 item_state【状态；已核】持物与充能Render当前状态 → P0.item_state | 0x15F8—0x15FB int _itemStateCooldown【权威；SDK声明】_itemStateCooldown：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x15FC—0x15FF uint32_t _vampireCharmKills【权威；SDK声明】_vampireCharmKills：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1600—0x1603 bytes[4] unknown_1600_1603【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1604—0x1607 int _bombPlaceDelay【权威；SDK声明】_bombPlaceDelay：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1608—0x160B int _forgottenSwapFormCooldown【权威；SDK声明】_forgottenSwapFormCooldown：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x160C—0x160F int _controllerIndex【本地；已核】_controllerIndex：C/S Client=-1；不接Host设备键 → C/S Client=-1；不接Host设备键 | 0x1610—0x1613 int _playerIndex【状态；已核】_playerIndex：P7.player_index → P7.player_index | 0x1614—0x1617 bytes[4] unknown_1614_1617【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1618—0x161B int _moveDirection【权威；SDK声明】_moveDirection：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x161C—0x161F int _headDirection【权威；SDK声明】_headDirection：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1620—0x16AF bytes[144] unknown_1620_16af【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x16B0—0x16B7 uint32_t[2] _trinketsID【状态；已核】_trinketsID：P3.trinkets → P3.trinkets | 0x16B8—0x16C3 std_vector_int _collectibleCounts【容器；已核】_collectibleCounts：P3.collectible_counts → P3.collectible_counts | 0x16C4—0x1737 bytes[116] unknown_16c4_1737【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1738—0x1743 vector_SmeltedTrinketDesc _smeltedTrinkets【容器；已核】_smeltedTrinkets：P3.smelted → P3.smelted | 0x1744—0x174F std_vector_int _voidedCollectibles【权威；SDK声明】_voidedCollectibles：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1750—0x178F bytes[64] unknown_1750_178f【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1790—0x17AF PocketItem[4] pocket_items【嵌套；已核】四口袋ID与原type，ACTIVE_ITEM表示slot+1 → P3.pocket；展开PocketItem×4 | 0x17B0—0x17B3 uint32_t _actionHoldDrop【权威；SDK声明】_actionHoldDrop：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x17B4—0x17C3 QueueItemData queued_item【嵌套；已核】只投影queued饰品配置与黄金位；空/非饰品不读无效负载 → P3.queued_trinket；其他字段Host；展开QueueItemData×1 | 0x17C4—0x17CB bytes[8] unknown_17c4_17cb【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x17CC—0x1807 int[15] _playerForms【权威；SDK声明】_playerForms：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1808—0x180F bytes[8] unknown_1808_180f【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1810—0x1810 bool _postLevelInitFinished【权威；SDK声明】_postLevelInitFinished：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1811—0x1811 bool _canShoot【权威；SDK声明】_canShoot：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1812—0x1813 bytes[2] unknown_1812_1813【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1814—0x183F ColorMod _footprintColor1【嵌套；SDK声明】_footprintColor1：Host足迹生产模板；已生成Effect独立同步 → Host足迹生产模板；已生成Effect独立同步；展开ColorMod×1 | 0x1840—0x186B ColorMod _footprintColor2【嵌套；SDK声明】_footprintColor2：Host足迹生产模板；已生成Effect独立同步 → Host足迹生产模板；已生成Effect独立同步；展开ColorMod×1 | 0x186C—0x1873 bytes[8] unknown_186c_1873【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1874—0x1877 int _ponyCharge【权威；SDK声明】_ponyCharge：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1878—0x187B int _planCKillCountdown【权威；SDK声明】_planCKillCountdown：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x187C—0x188F bytes[20] unknown_187c_188f【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1890—0x1893 int _epiphoraCharge【权威；SDK声明】_epiphoraCharge：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1894—0x189B bytes[8] unknown_1894_189b【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x189C—0x189F uint32_t _deadEyeCharges【权威；SDK声明】_deadEyeCharges：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x18A0—0x18A3 uint32_t _deadEyeMisses【权威；SDK声明】_deadEyeMisses：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x18A4—0x18A7 bytes[4] unknown_18a4_18a7【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x18A8—0x18AB uint32_t _mawOfTheVoidChargeTimer【状态；已核】_mawOfTheVoidChargeTimer：P0.maw_charge → P0.maw_charge | 0x18AC—0x18C7 bytes[28] unknown_18ac_18c7【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x18C8—0x18CF i32[2] temporary_trinket_inputs【权威；已核】GetTrinketMultiplier的19/24计数；当前有界显示查询未使用 → Host；不传 | 0x18D0—0x18D3 bytes[4] unknown_18d0_18d3【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x18D4—0x18D7 uint32_t _imExcitedSpeedupCountdown【权威；SDK声明】_imExcitedSpeedupCountdown：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x18D8—0x18DF bytes[8] unknown_18d8_18df【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x18E0—0x18E3 int _megaBlastDuration【权威；SDK声明】_megaBlastDuration：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x18E4—0x18E7 int _peeBurstCooldown【权威；SDK声明】_peeBurstCooldown：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x18E8—0x18EB int _maxPeeBurstCooldown【权威；SDK声明】_maxPeeBurstCooldown：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x18EC—0x18EF int _bladderCharge【权威；SDK声明】_bladderCharge：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x18F0—0x18F3 int _maxBladderCharge【权威；SDK声明】_maxBladderCharge：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x18F4—0x18F4 bool _isUrethraBlocked【权威；SDK声明】_isUrethraBlocked：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x18F5—0x18F7 bytes[3] unknown_18f5_18f7【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x18F8—0x18FB int _nextUrethraBlockFrame【权威；SDK声明】_nextUrethraBlockFrame：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x18FC—0x1917 EntityDesc _friendBallEnemy【嵌套；SDK声明】_friendBallEnemy：Host未来投掷／生成描述，不发 → Host未来投掷／生成描述，不发；展开EntityDescHost×1 | 0x1918—0x191B int _purityState【权威；SDK声明】_purityState：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x191C—0x191F int _immaculateConceptionState【权威；SDK声明】_immaculateConceptionState：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1920—0x1923 int _cambionConceptionState【权威；SDK声明】_cambionConceptionState：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1924—0x1927 uint32_t _conceptionFamiliarFlags【权威；SDK声明】_conceptionFamiliarFlags：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1928—0x192B float _d8DamageModifier【权威；SDK声明】_d8DamageModifier：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x192C—0x192F float _d8SpeedModifier【权威；SDK声明】_d8SpeedModifier：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1930—0x1933 float _d8RangeModifier【权威；SDK声明】_d8RangeModifier：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1934—0x1937 float _d8FireDelayModifier【权威；SDK声明】_d8FireDelayModifier：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1938—0x193B int _goldenHearts【状态；已核】_goldenHearts：P2.golden_hearts → P2.golden_hearts | 0x193C—0x1AD7 bytes[412] unknown_193c_1ad7【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1AD8—0x1ADB int _babySkin【权威；SDK声明】_babySkin：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1ADC—0x1B1F bytes[68] unknown_1adc_1b1f【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1B20—0x1C43 OptionalDrawSequence recording_sequence【权威；已核】记录／更新生产序列；与1C44当前绘制序列独立 → Host；不用它生成Client回放；展开OptionalDrawSequence×1 | 0x1C44—0x1D67 OptionalDrawSequence current_draw_sequence【嵌套；已核】当前有效节点与1C50 Sprite归P8，未选节点／cursor原值不传 → P8.current_draw_record；展开OptionalDrawSequence×1 | 0x1D68—0x1D73 vector_EntitySaveState _movingBoxContents【权威；SDK声明】_movingBoxContents：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行；外部元素EntitySaveState另计 | 0x1D74—0x1D77 int _boneHearts【状态；已核】_boneHearts：P2.bone_hearts → P2.bone_hearts | 0x1D78—0x1D7B int _boneHeartsMask【权威；SDK声明】_boneHeartsMask：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1D7C—0x1D7F int _hallowedGroundCountdown【权威；SDK声明】_hallowedGroundCountdown：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1D80—0x1D83 bytes[4] unknown_1d80_1d83【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1D84—0x1D87 Player* sub_storage【权威；已核】Host发现／模拟关系；分数与HUD使用最终缓存 → Host；不成为Client native边 | 0x1D88—0x1D8B bytes[4] unknown_1d88_1d8b【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1D8C—0x1D8F int _brokenHearts【状态；已核】_brokenHearts：P2.broken_hearts → P2.broken_hearts | 0x1D90—0x1D93 int _rottenHearts【状态；已核】_rottenHearts：P2.rotten_hearts → P2.rotten_hearts | 0x1D94—0x1D97 i32 soul_charge【状态；已核】当前资源及主动HUD显示输入 → P2.soul_charge | 0x1D98—0x1D9B i32 blood_charge【状态；已核】当前资源及主动HUD显示输入 → P2.blood_charge | 0x1D9C—0x1DA7 uint16_t[6] _candyHeartStatUps【权威；SDK声明】_candyHeartStatUps：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1DA8—0x1DB3 uint16_t[6] _soulLocketStatUps【权威；SDK声明】_soulLocketStatUps：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1DB4—0x1DBB bytes[8] unknown_1db4_1dbb【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1DBC—0x1DBF uint32_t _urnSouls【状态；已核】_urnSouls：P2.urn_souls → P2.urn_souls | 0x1DC0—0x1DC3 int _numGigaBombs【状态；已核】_numGigaBombs：P2.giga_bombs → P2.giga_bombs | 0x1DC4—0x1DD7 bytes[20] unknown_1dc4_1dd7【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1DD8—0x1DEB History _history【嵌套；已核】_history：原始获得史留Host；第8步最终图像缓存 → 原始获得史留Host；第8步最终图像缓存；展开History×1 | 0x1DEC—0x1E2B int[16] _inventoryHistoryIdx【权威；SDK声明】_inventoryHistoryIdx：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1E2C—0x1E37 bytes[12] unknown_1e2c_1e37【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1E38—0x1E4B deque_afterImageFrames _afterImageFrames【容器；已核】_afterImageFrames：P8.afterimages，外部AfterImageFrame → P8.afterimages，外部AfterImageFrame | 0x1E4C—0x1E4F EntityPtr _twinPlayer【引用；已核】_twinPlayer：P7.twin；注册节点本地 → P7.twin；注册节点本地 | 0x1E50—0x1E53 Entity_Player* _backupPlayer【引用；已核】_backupPlayer：P7.backup → P7.backup | 0x1E54—0x1E57 bytes[4] unknown_1e54_1e57【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1E58—0x1E5B int _redStewBonusDuration【权威；SDK声明】_redStewBonusDuration：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1E5C—0x1E6F bytes[20] unknown_1e5c_1e6f【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1E70—0x1E73 int _marsUnk1【未知；未知】_marsUnk1：不上线；SDK名含Unk，尚无具体字段语义 → 不上线；SDK名含Unk，尚无具体字段语义 | 0x1E74—0x1E77 int _marsCooldown【权威；SDK声明】_marsCooldown：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1E78—0x1E7B int _marsUnk2【未知；未知】_marsUnk2：不上线；SDK名含Unk，尚无具体字段语义 → 不上线；SDK名含Unk，尚无具体字段语义 | 0x1E7C—0x1E7C bool _marsUnkBool【未知；未知】_marsUnkBool：不上线；SDK名含Unk，尚无具体字段语义 → 不上线；SDK名含Unk，尚无具体字段语义 | 0x1E7D—0x1E93 bytes[23] unknown_1e7d_1e93【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1E94—0x1E97 float _rockBottomMoveSpeed【权威；SDK声明】_rockBottomMoveSpeed：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1E98—0x1E9B float _rockBottomMaxFireDelay【权威；SDK声明】_rockBottomMaxFireDelay：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1E9C—0x1E9F float _rockBottomDamage【权威；SDK声明】_rockBottomDamage：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1EA0—0x1EA3 float _rockBottomTearRange【权威；SDK声明】_rockBottomTearRange：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1EA4—0x1EA7 float _rockBottomShotSpeed【权威；SDK声明】_rockBottomShotSpeed：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1EA8—0x1EAB float _rockBottomLuck【权威；SDK声明】_rockBottomLuck：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1EAC—0x1EBF bytes[20] unknown_1eac_1ebf【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1EC0—0x1EC3 uint32_t _revelationChargeTimer【状态；已核】_revelationChargeTimer：P0.revelation_charge → P0.revelation_charge | 0x1EC4—0x1EC7 uint32_t _montezumaChargeTimer【状态；已核】_montezumaChargeTimer：P0.montezuma_charge → P0.montezuma_charge | 0x1EC8—0x1ECF bytes[8] unknown_1ec8_1ecf【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1ED0—0x1ED3 uint32_t _maggyHealthDrainCooldown【权威；SDK声明】_maggyHealthDrainCooldown：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1ED4—0x1ED7 int _maggySwingCooldown【权威；SDK声明】_maggySwingCooldown：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1ED8—0x1F07 bytes[48] unknown_1ed8_1f07【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1F08—0x1F0B int _suplexState【状态；已核】_suplexState：P0.suplex_state → P0.suplex_state | 0x1F0C—0x1F0F int _suplexAimCountdown【权威；SDK声明】_suplexAimCountdown：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1F10—0x1F17 Vector _suplexTargetPos【权威；SDK声明】_suplexTargetPos：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1F18—0x1F1F Vector _suplexLandPos【权威；SDK声明】_suplexLandPos：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1F20—0x1F27 char[8] _bagOfCraftingContent【状态；已核】_bagOfCraftingContent：P3.crafting[8] → P3.crafting[8] | 0x1F28—0x1F2F BagOfCraftingOutput _bagOfCraftingOutput【嵌套；已核】_bagOfCraftingOutput：Host配方生产输入；第8步最终图像／结果 → Host配方生产输入；第8步最终图像／结果；展开BagOfCraftingOutput×1 | 0x1F30—0x1F33 int _poopMana【状态；已核】_poopMana：P2.poop_mana → P2.poop_mana | 0x1F34—0x1F39 char[6] _poopSpellQueue【状态；已核】_poopSpellQueue：P3.poop[6] → P3.poop[6] | 0x1F3A—0x1F53 bytes[26] unknown_1f3a_1f53【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1F54—0x1F57 i32 crafting_use_charge【状态；已核】758850的710进度条输入 → P0.crafting_use_charge | 0x1F58—0x1F5B int _eveSumptoriumCharge【状态；已核】_eveSumptoriumCharge：P0.eve_charge → P0.eve_charge | 0x1F5C—0x1F63 std_map_int_int _itemWispsList【容器；已核】_itemWispsList：P3.item_wisps → P3.item_wisps | 0x1F64—0x1F93 bytes[48] unknown_1f64_1f93【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1F94—0x1F97 int _modelingClayEffect【状态；已核】_modelingClayEffect：P3.modeling_clay → P3.modeling_clay | 0x1F98—0x1F9B int _keepersSackBonus【权威；SDK声明】_keepersSackBonus：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x1F9C—0x1FAF bytes[20] unknown_1f9c_1faf【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1FB0—0x1FBB vector<u8> extra_collectible_counts【容器；已核】按ID索引的当前附加查询输入；trinket仅消费零／非零 → P3.extra_collectible_counts | 0x1FBC—0x1FC7 vector<u8> extra_trinket_counts【换算；已核】按ID索引的当前附加查询输入；trinket仅消费零／非零 → P3.extra_trinket_counts | 0x1FC8—0x1FF7 bytes[48] unknown_1fc8_1ff7【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x1FF8—0x1FFF ImageStrongPair multiplayer_shadow_image【嵌套；已核】本地Image strong持有；实际图片资源选择交Presentation，不能只发路径猜实际图像 → 第8步图片资源owner；展开ImageStrongPair×1 | 0x2000—0x2007 ImageStrongPair coop_menu_image【嵌套；已核】本地Image strong持有；实际图片资源选择交Presentation，不能只发路径猜实际图像 → 第8步图片资源owner；展开ImageStrongPair×1 | 0x2008—0x200B bytes[4] unknown_2008_200b【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x200C—0x200F u32 special_item_mask【权威；已核】特殊查询305/257/231/182/108输入；已核显示参数未进入这些分支 → Host；不传 | 0x2010—0x2010 u8 inventory_suppressed【换算；已核】当前库存屏蔽的零／非零含义，独立于CoopGhost → P3.inventory_suppressed | 0x2011—0x2013 bytes[3] unknown_2011_2013【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x2014—0x2033 ConsumableData _consumablesBackup【嵌套；SDK声明】_consumablesBackup：Host资源恢复备份，不发 → Host资源恢复备份，不发；展开ConsumableData×1 | 0x2034—0x2063 bytes[48] unknown_2034_2063【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x2064—0x2064 u8 wild_card_item_type【权威；SDK声明】Wild Card未来Use类型，非当前口袋类型 → Host；不传 | 0x2065—0x2067 bytes[3] unknown_2065_2067【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x2068—0x206B int _wildCardItem【权威；SDK声明】_wildCardItem：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x206C—0x206F bytes[4] unknown_206c_206f【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x2070—0x2077 i32[2] samson_and_ibs_charge【权威；已核】Samson／IBS更新生产输入；非758850五计数 → Host；最终Sprite／攻击实体承接 | 0x2078—0x207C bytes[5] unknown_2078_207c【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x207D—0x207D bool _isCoopGhost【状态；已核】_isCoopGhost：P2.coop_ghost → P2.coop_ghost | 0x207E—0x20A7 bytes[42] unknown_207e_20a7【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x20A8—0x20AF PlayerHUDStrongPair dynamic_hud_pair【嵌套；已核】实际PlayerHUD与control的本地smart pair，HUD状态交第8步 → 第8步动态HUD owner；展开PlayerHUDStrongPair×1 | 0x20B0—0x20BB bytes[12] unknown_20b0_20bb【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x20BC—0x20BC bool _hasUnlistedRestoreState【权威；SDK声明】_hasUnlistedRestoreState：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host模拟／生产／恢复；不下行 | 0x20BD—0x20BF bytes[3] unknown_20bd_20bf【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x20C0—0x267B GameStatePlayer _unlistedRestoreState【嵌套；已核】_unlistedRestoreState：Host恢复快照，不发；内部未知仍列出 → Host恢复快照，不发；内部未知仍列出；展开GameStatePlayer×1 | 0x267C—0x267F Entity_Player* _replacedPlayer【权威；已核】_replacedPlayer：已知SDK用途为玩法／生产状态，最终实体／ANM／显示缓存另有owner；不能因有公开getter自动上线 → Host替换恢复边；不上线 | 0x2680—0x268B bytes[12] unknown_2680_268b【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x268C—0x2E93 NativeCommunicationDisplay vanilla_communication_display【本地；推断】原版通信气泡／表情／typing展示对象；C/S不复用原版通信状态 → 不下行；C/S本地关闭该原生分支；展开NativeCommunicationDisplay×1 | 0x2E94—0x2E9F vector<i32> deferred_effect_removals【权威；已核】7A4CC0遍历ID并RemoveEffect后清尾；Host延迟移除队列 → Host；不传 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntityPlayer.zhl；5-player-p0-p2.md；5-player-p3-p5.md；5-player-p6-p8.md；mod/binding/state/entity.json；J273 ctor 74DC70；J273 ctor 74E454，77BD40/79EA20；docs/design/tx-04-20/player-i26-i30-native.md；mod/src/game/client_player_draw_native.cpp；mod/src/game/client_player_hud_native.cpp；J273 74E920/74E930 ctor，78495D—7849D9 Render；J273 6F6874/6F6917、8EC6AB/8EC794 communication input；J273 7A4EE4—7A4F5B，74F320—74F326

### EntitySaveStateStrongPair

原生大小 0x8。仅8B强引用pair；控制块和递归实体保存体按真实owner持有，不能展开成固定网络图。

| 0x0—0x3 EntitySaveState* save_state【本地；已核】本地可递归保存体所有权，不是C/S实体StateRef → Host本地保存owner | 0x4—0x7 KAGE_ReferenceCount* counter【本地；已核】本地控制块／引用计数，禁止下行 → 本地owner |
|---|---|

证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl；F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/SmartPointer.zhl

### EntitySaveState

原生大小 0x78。移动箱两处vector的真正元素。int/float/boolStorage虽有类型仍为UNKNOWN，不能误用更小的FriendBall EntityDesc替代。保存／离房容器的外部元素不计入根2EA0固定字节总数。

| 0x0—0xB i32[3] type_variant_subtype【权威；已核】保存对象身份；恢复时由Host使用 → Host；不下行 | 0xC—0x1F u32/i32[5] storage_1_5【未知；未知】SDK仅叫intStorage，类型／偏移已声明，具体用途未核 → UNKNOWN不上线 | 0x20—0x21 i16 grid_spawn_idx【权威；已核】保存生成位置索引，ESSM可复用 → Host保存adapter | 0x22—0x23 bool[2] bool_storage【未知；未知】SDK只声明boolStorage1/2，尚无用途 → UNKNOWN不上线 | 0x24—0x27 u32 storage_6【未知；未知】SDK仅intStorage6，不能当已知普通Host字段 → UNKNOWN不上线 | 0x28—0x2F Vector target_position【权威；已核】保存目标位置；不用于Client重放恢复 → Host；不下行 | 0x30—0x33 u32 storage_7【未知；未知】SDK声明ESSM可复用的intStorage7，未核具体用途 → UNKNOWN不上线 | 0x34—0x3B u32[2] init_drop_seeds【权威；已核】保存RNG种子 → Host；不下行 | 0x3C—0x43 i32[2] spawner_type_variant【权威；已核】Host恢复生产来源描述 → Host；不下行 | 0x44—0x4F bytes[12] unknown_44_4f【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x50—0x57 f32[2] float_storage【未知；未知】只有类型与偏移，具体用途未核 → UNKNOWN不上线 | 0x58—0x5B bytes[4] unknown_58_5b【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x5C—0x5F u32 storage_8【未知；未知】SDK intStorage8语义未核 → UNKNOWN不上线 | 0x60—0x60 i8 byte_storage【未知；未知】SDK byteStorage语义未核 → UNKNOWN不上线 | 0x61—0x6B bytes[11] unknown_61_6b【未知；未知】尚无字段语义或填充证明；初始化清零／相邻字段不足以改判 → 不上线；不采集／不安装 | 0x6C—0x73 EntitySaveStateStrongPair nested_save【本地；SDK声明】保存体可递归强引用；不映射线上关系 → Host本地保存owner；展开EntitySaveStateStrongPair×1 | 0x74—0x77 bytes[4] unknown_74_77【未知；未知】尾部尚未解释；不以对象大小或初始化零值判填充 → 不上线；不采集／不安装 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

证据：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl

## 完成边界

本账本完成范围登记、字节归属及UNKNOWN显式登记；没有消灭未知、没有把ABI推导提升为运行观察。公开成员逐行回映射另见 [player-member-map.md](player-member-map.md)。正式player.json、整数版本递增、生成链及底层支持仍按后续实现推进。P0—P8线布局以三份设计为依据，本台账没有增加新状态类型或线值；所有保留字段必须有合法读取资格，未知／不适用不填零，首次来源不明不借其他身体证言。
