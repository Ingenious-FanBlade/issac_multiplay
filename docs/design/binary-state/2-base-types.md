# 2：基础值类型与嵌套规则

**2026-11-12 显示委托修订**：

- 基础类型保留，ANM2、LayerState、ColorMod 等显示所需类型继续使用。
- RNG 不再作为同步状态，只同步由它选出的结果。
- 见[修改方案](client-display-delegation.md)第 7 节。

本次只修订文档，未改代码，未运行测试。

**2026-11-09 当前版本39：Player装备与Plus展示依赖。** [完整消费者与安装复核](player-equipment-plus-synchronization-review.md)将ActiveItemDesc来源池／定时充能冷却、Weapon五项非展示输入及KnifePlus.hitboxSource留Host；保留主动槽六值、Weapon充能三值与类型、TemporaryEffect三值及Plus公共nullable覆盖。新增135号UmbilicalWhip真实主引用、136号TemporaryEffects外层disabled／列表，并补Familiar+D76绘制抑制字节。原已排除Plus项不恢复，其他UNKNOWN保持。源、生成表、正式Plus读取接口和第5／8／10—15步已修订，未构建或测试，运行时仍待接线。

状态：基础布局、共用代码和本轮静态核对已入源，普通 Release 纯编译／链接通过（2026-11-08）；仍有明确的 SDK 声明、未知区与跨域未决，见末节。上级：[总计划](README.md)。实现入口：[第 0—2 步实现说明](implementation-0-2.md)。下方手写表仍为实施前草稿，当前事实以布局源、生成表和证据文档为准。

**版本32合同（未改变项继续适用）**：[动画／资源／颜色职责复核](animation-resource-synchronization-review.md)取代下方草稿中的事件位、loaded与ColorParams传输要求。事件位留Host；ANM2.loaded由本地资源owner维护，资源有无并入file_name可空资源号；图层loaded为本地加载／重载维护，路径与实际Image独立发送。ColorParams队列不传，最终ColorMod保留全部11个f32。is_playing、anim_frame及playback_speed有官方绘制插值消费者，继续同步。生成表与唯一布局源为当前字段依据；下方初稿表保留原设计语境，不能恢复已排除字段。

## 目标

确定 Entity、Player、Grid、Room 等类型共用的基础值类型的字节归属和状态布局，并统一嵌套类型的处理规则。

## 输入事实

基础类型的原 SDK 声明来自 REPENTOGON 1.1.3 `libzhl/functions/*.zhl`。本表保留 SDK 大小口径，J273 核对、差异和未知区见[基础类型证据](base-types-evidence.md)及[生成布局](layouts/README.md)：

| 类型 | 大小 | SDK 字段 |
| --- | --- | --- |
| `Vector` | 0x8 | x、y 两个 f32 |
| `ColorMod` | 0x2C | `_tint[4]` 0x0、`_colorize[4]` 0x10、`_offset[3]` 0x20 |
| `KColor` | 0x14 | RGBA 四个 f32，另有 `_unk` i32 0x10 |
| `BlendMode` | 0x14 | SDK 实际声明五个 u32：equation、RGB source/destination、alpha source/destination；J273 连续五 DWORD 已核 |
| `AnimationState` | 0x20 | `_animation` 0x0、`_animData` 0x4、`_layerFrames` 0x8、`_nullLayerFrames` 0xC、`_animFrame` 0x10、`_isPlaying` 0x14、`_currentlyTriggeredEvents` 0x18、`_previouslyTriggeredEvents` 0x1C |
| `LayerState` | 0xA0 | 见下文原生表 |
| `ANM2` | 0x114 | 见下文原生表 |
| `RNG` | 0x10 | seed 加三个 shift |
| `BitSet128` | 0x10 | 128 位 |
| `EntityPtr` | 0x4 | `Entity* _ref` |
| `ColorParams` | 0x3C | `_priority` 0x0、`_color` 0x4、duration1 0x30、duration2 0x34、fadeout 0x38、shared 0x39；末尾 2B 待核 |
| `ActiveItemDesc` | 0x20 | SDK 声明 8 个字段 |
| `Weapon` | 0x40（SDK） | SDK 声明 8 个字段；J273 构造已写到 +0x40，已核基类前缀至少到 0x44，具体动态大小由第 5 步按工厂确认 |
| `TemporaryEffect` | 0x10 | SDK 声明 3 个字段 |

MSVC 32 位下，`std::string` 为 0x18 字节，`std::vector` 为 0xC 字节（begin、end、capacity 三个指针）。

版本41补入NPC已核的MSVC Release deque：头0x14B含proxy／map／capacity／logical offset／count，引用元素4B、点元素8B，分配块16B。`ReadDequeInto`归并有效元素到预分配区，不复制或传输容器头；前后重读头用于拒绝来源变化，元素只复制一次。线上仍为u16 count＋递归投影元素，Client按具名游戏allocator／EntityPtr owner重建。完整边界见[NPC证据](npc-evidence.md)，不把该ABI外推到其他元素大小。

## 设计

### 2.1 嵌套规则

嵌套类型**只提取决定状态的成员变量**，不整块复制类型。只有纯值类型例外，因为它的“整个类型”和“成员变量”是同一回事。

| 嵌套成员 | 处理 |
| --- | --- |
| 定长纯值（`Vector`、`ColorMod`、`KColor`） | 全部成员内联在父块中，复制区间可以与相邻状态字段合并 |
| 变长容器（`std::vector<T>`） | 不发送容器头。单独成块，内容为 `u16 count + count×T`，`T` 按本表递归处理。Host 读取前校验 begin ≤ end、按元素大小对齐、元素数不超过布局上限，否则报具名缺口，不截断 |
| 字符串（`std::string`） | 发送 `u16 字节数 + 原字节`，不附加结尾 0，按布局上限截断视为缺口。内容如果是资源路径，改用资源表序号 |
| 实体引用（`EntityPtr`、裸 `Entity*`） | 转成 `u16` 成员序号，`0xFFFF` 表示 Null。目标不在成员表中时报缺口，不发送地址 |
| 资源或配置指针 | 转成稳定标识，例如资源表序号、配置 ID、动画序号 |
| 指向文件内容的指针 | 归为派生，例如 `ANM2._animData`、`_nullLayerNames`、`_eventNames`。两端都从同一文件加载，只发送选择了哪一项 |
| 本地指针 | 不发送，例如回指父对象、贴图智能指针、Lua 引用 |
| 用途不定的原始字节（`BitSet128` 型 VarData） | 整段作为不透明字节复制，不解释类型，Client 原样写回 |

Client 写入变长容器时，不能把字节 memcpy 回原生容器，必须通过游戏的分配方式或已核的原生写入路径重建。

### 2.2 草稿表格

以下内容来自 SDK 声明，类别为初判，需在本步完成 J273 核对，并以生成器的输出为准。

**Vector 原生字节归属**

| 0x0 f32 x【状态】横坐标 | 0x4 f32 y【状态】纵坐标 |
|---|---|

**Vector 状态布局（8 字节）**

| +0x0 f32 x：横坐标 | +0x4 f32 y：纵坐标 |
|---|---|

**ColorMod 原生字节归属**

| 0x00 f32[4] tint【状态】RGBA 乘色 | 0x10 f32[4] colorize【状态】RGBA 着色强度 | 0x20 f32[3] offset【状态】RGB 加色 |
|---|---|---|

**ColorMod 状态布局（44 字节）**

| +0x00 f32[4] tint：RGBA 乘色 | +0x10 f32[4] colorize：RGBA 着色强度 | +0x20 f32[3] offset：RGB 加色 |
|---|---|---|

**AnimationState 原生字节归属**

| 0x00 ANM2* animation【本地】回指所属 ANM2 | 0x04 AnimationData* animData【资源】当前动画，转成动画序号 | 0x08 int* layerFrames【未知】逐层帧数组，是否由帧号派生待核 | 0x0C int* nullLayerFrames【未知】逐空层帧数组，是否派生待核 | 0x10 f32 animFrame【状态】当前帧 | 0x14 bool isPlaying【状态】是否播放 | 0x15—0x17 3B【未知】疑似填充 | 0x18 i32 currentlyTriggeredEvents【状态】本帧触发事件位 | 0x1C i32 previouslyTriggeredEvents【状态】上一帧触发事件位 |
|---|---|---|---|---|---|---|---|---|

**AnimationState 状态布局（15 字节，草稿）**

| +0x0 u16 animation：动画序号，0xFFFF 表示无 | +0x2 f32 frame：当前帧 | +0x6 u8 playing：是否播放，0／1 | +0x7 i32 events_now：本帧触发事件位 | +0xB i32 events_prev：上一帧触发事件位 |
|---|---|---|---|---|

**LayerState 原生字节归属**

| 0x00 LayerData* layerData【派生】图层定义，由文件和层序决定 | 0x04 ANM2* animation【本地】回指所属 ANM2 | 0x08 std::string spriteSheetPath【资源】贴图路径，转成资源表序号 | 0x20 u32 minFilterMode【状态】缩小过滤 | 0x24 u32 magFilterMode【状态】放大过滤 | 0x28 u32 wrapSMode【状态】S 向环绕 | 0x2C u32 wrapTMode【状态】T 向环绕 | 0x30 bool spriteSheetLoaded【状态】贴图已加载 | 0x31 bool unk【未知】SDK 未命名 | 0x32 bool flipX【状态】水平翻转 | 0x33 bool flipY【状态】垂直翻转 | 0x34 Vector size【状态】尺寸 | 0x3C f32 rotation【状态】旋转 | 0x40 Vector pos【状态】位置 | 0x48 ColorMod color【状态】图层颜色，见 ColorMod 表 | 0x74 bool visible【状态】可见 | 0x75—0x77 3B【未知】疑似填充 | 0x78 BlendMode blendMode【状态】混合方式，内部字段待核 | 0x8C u32 bitflags【状态】渲染标志 | 0x90 Vector cropOffset【状态】裁剪偏移 | 0x98 SmartPointer spriteSheet【本地】已加载贴图 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

**ANM2 原生字节归属**

| 0x000 std::string filename【资源】anm2 文件路径，转成资源表序号 | 0x018 std::string animDefaultName【派生】默认动画名，由文件决定，待核 | 0x030 AnimationState animState【状态】主动画，见 AnimationState 表 | 0x050 AnimationState overlayAnimState【状态】叠加动画 | 0x070 bool overlayRenderFirst【状态】叠加层先绘制 | 0x071—0x073 3B【未知】疑似填充 | 0x074 AnimationData* animData【派生】动画表，由文件决定 | 0x078 i32 animCount【派生】动画数 | 0x07C LayerState* layerState【容器】图层数组首址 | 0x080 u32 layerCount【容器】图层数 | 0x084 std::string* nullLayerNames【派生】空层名表 | 0x088 u32 numNullLayers【派生】空层数 | 0x08C std::string* eventNames【派生】事件名表 | 0x090 u32 numEvents【派生】事件数 | 0x094 Vector offset【状态】绘制偏移 | 0x09C Vector scale【状态】缩放 | 0x0A4 f32 rotation【状态】旋转 | 0x0A8 ColorMod color【状态】颜色 | 0x0D4 ColorMod championColor【状态】精英色 | 0x100 bool flipX【状态】水平翻转 | 0x101 bool flipY【状态】垂直翻转 | 0x102—0x103 2B【未知】 | 0x104 f32 playbackSpeed【状态】播放速度 | 0x108 1B【未知】 | 0x109 bool loaded【状态】已加载 | 0x10A—0x10F 6B【未知】 | 0x110 u32 bitflags【状态】渲染标志 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

ANM2 的状态拆成三部分：

- **定长外观**：文件序号、偏移、缩放、旋转、颜色、精英色、翻转、播放速度等。
- **动画**：主动画与叠加动画两份 AnimationState，草稿为 2×15 字节。
- **变长图层**：`u16 count + count×LayerState 状态`。

动画状态在播放时每帧变化，外观很少变化。把两者分开，动画推进时就不必重发外观。Entity 中这三部分分别对应第 3 步的 E4、E5、E6；附属 Sprite 按同样方式拆分（第 4 步 4.1）。每个 LayerState 状态的元素布局由本步定稿。

### 2.3 资源表

资源路径（anm2 文件、贴图、shader 等）不逐帧携带字符串。Host 维护资源表，把路径映射成 `u16` 序号，资源表随成员表走可靠通道发送（第 9 步）。同一路径重新加载、贴图替换（官方 `LuaSprite.cpp` 的 Image 替换直接调用 `LayerState::SetSpriteSheet`），会表现为图层状态或资源序号的变化，由 memcmp 发现，不依赖 Hook 覆盖率。

**主审补充**：官方 `SetSpriteSheet` 不改 `_spriteSheetPath`。实际 Image 选择与公开路径必须分别归属，不能仅比较路径来判断贴图未变。REPENTOGON 自定义 shader 位于官方扩展旁表，独立的原生扩展来源归第 8 步，不能塞进 J273 对象的未知字节或冒充已覆盖。

## 改动范围

- 布局源中写入基础类型条目。
- Host 共用的容器读取与校验辅助函数，以及 Client 的容器重建辅助函数。
- 资源表的数据结构。

## 失败方式

- **对齐判断错误**：把对齐字节当成状态，memcmp 误判；或者把状态当成填充，导致漏字段（例如 `0x31 unk`）。
- **派生判断错误**：认定由文件决定的值，实际在运行时可改（例如默认动画名），Client 因此漏写。
- **容器指针失效**：容器指针在复制过程中被游戏改写。只能在主线程合法窗口内读取，有效元素只复制一次；具名reader可在前后重读头拒绝来源变化，不能以重读拼接不同代次数据。
- **BlendMode 不明**：内部布局没核清，导致混合方式写错。
- **资源表不同步**：资源序号先于资源表到达 Client，或资源表重编号后，旧帧中的序号被误解为新路径。

## 完成内容

- **核对与入源**：14 种类型均已写入布局源，逐字节记录归属、证据和状态；J273 逆向由 GPT-6 Luna（max）进行，主智能体复读关键指令后合并。生成表由普通构建产出。没有将其余 SDK 声明或未知区自动提升为已核。
- **规则落地**：嵌套规则、容器读写接口、引用转换和资源表已实现，代码位置见实现说明。资源表的线格式在第 9 步定稿。

### 已核定的草稿纠错

- `AnimationState` 两组选择器可独立修改，不能由 `_animFrame` 派生；长度来自当前 `AnimationData+0x1C/+0x24`，不能使用 Sprite 总层数。初次实施状态长度为 `19 + 4×(当前动画层数 + 当前动画空层数)`，取代草稿的固定15B；版本32移出8B事件位后为 `11 + 4×(当前动画层数 + 当前动画空层数)`。
- `ANM2._animDefaultName` 在加载与拷贝中可赋值，本次保留为状态，不凭“未见 setter”判为派生。
- `BlendMode` 是五个连续 u32，分别对应混合方程及四个混合因子。
- `Vector` 两个 f32、`ColorMod` 的 4/4/3 分组和 `BitSet128` 的 16B ABI 均有 J273 直接证据。`KColor+0x10` 是 u32 颜色空间标记，已核 RGB/sRGB/HSL 标签 0/1/2，必须保留；不是填充或语义不明的尾字。
- `ActiveItemDesc+0x4` 是 `item_pool`；`PartialCharge+0x18` 实际为 f32，SDK 的 int 声明错误。
- `Weapon` 构造已写到 +0x40，基类前缀和具体动态对象大小必须分开。

## 未决问题

- **明确未知字节**：AnimationState `0x15—0x17`；LayerState `0x31、0x75—0x77`；ANM2 `0x45—0x47、0x65—0x67`（两份内嵌 AnimationState 的未知区），以及 `0x71—0x73、0x102—0x103、0x108、0x10A—0x10B`；ColorParams `0x3A—0x3B`；Weapon `0x8—0xB、0x18—0x1F、0x3C—0x43`；TemporaryEffect `0xC—0xF`。ANM2 合计 14B，按全部 14 个类型条目统计为 47B（类型间的嵌套实例分别计入各自类型）。其余仅 SDK 声明或推断的范围仍按原标签保留，不宣称全部已核。第 15 步未知区探测继续保留。
- **值域边界**：KColor 当前只将已直接核定的颜色空间标签 0/1/2 纳入原生安装校验；其他标签应报未支持缺口并补证，不声称这些标签在全程序均非法，也不据此宣称原版所有取值已验收。
- 具体动态 Weapon 布局归第 5 步；官方扩展 shader 旁表及原生资源访问归第 8 步，不改变第 17 节的正式原生采集要求。

版本39的装备三类以[Player装备与Plus复核](player-equipment-plus-synchronization-review.md)为准：ActiveItemDesc仅六值24B，Weapon基础仅四值16B，TemporaryEffect三值10B保持。新增135号真实UmbilicalWhip主引用及136号TemporaryEffects外层显示开关／有序列表；前缀、动态分配、外部元素与未知分别建模。旧表／原值writer不构成传输必要性，未构建或测试。
