# 2：基础值类型与嵌套规则

状态：设计稿，未实施。上级：[总计划](README.md)。

## 目标

确定 Entity、Player、Grid、Room 等类型共用的基础值类型的字节归属和状态布局，并统一嵌套类型的处理规则。

## 输入事实

基础类型的 SDK 声明，均来自 REPENTOGON 1.1.3 `libzhl/functions/*.zhl`，未经 J273 反编译核对：

| 类型 | 大小 | SDK 字段 |
| --- | --- | --- |
| `Vector` | 0x8 | x、y 两个 f32 |
| `ColorMod` | 0x2C | `_tint[4]` 0x0、`_colorize[4]` 0x10、`_offset[3]` 0x20 |
| `KColor` | 0x14 | RGBA 四个 f32，另有 `_unk` i32 0x10 |
| `BlendMode` | 0x14 | SDK 未声明字段，Client 现从 Lua `LayerState.GetBlendMode` 取值（`client_resource_model.cpp:91`） |
| `AnimationState` | 0x20 | `_animation` 0x0、`_animData` 0x4、`_layerFrames` 0x8、`_nullLayerFrames` 0xC、`_animFrame` 0x10、`_isPlaying` 0x14、`_currentlyTriggeredEvents` 0x18、`_previouslyTriggeredEvents` 0x1C |
| `LayerState` | 0xA0 | 见下文原生表 |
| `ANM2` | 0x114 | 见下文原生表 |
| `RNG` | 0x10 | seed 加三个 shift |
| `BitSet128` | 0x10 | 128 位 |
| `EntityPtr` | 0x4 | `Entity* _ref` |
| `ColorParams` | 0x3C | `_priority` 0x0、`_color` ColorMod 0x4，其余未声明 |
| `ActiveItemDesc` | 0x20 | SDK 声明 8 个字段 |
| `Weapon` | 0x40 | SDK 声明 8 个字段 |
| `TemporaryEffect` | 0x10 | SDK 声明 3 个字段 |

MSVC 32 位下，`std::string` 为 0x18 字节，`std::vector` 为 0xC 字节（begin、end、capacity 三个指针）。

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

ANM2 的状态拆成两块：定长外观块（文件序号、两份动画状态、偏移、缩放、颜色等）和变长图层块（`u16 count + count×LayerState 状态`）。每个 LayerState 状态的元素布局由本步定稿。

### 2.3 资源表

资源路径（anm2 文件、贴图、shader 等）不逐帧携带字符串。Host 维护资源表，把路径映射成 `u16` 序号，资源表随成员表走可靠通道发送（第 9 步）。同一路径重新加载、贴图替换（官方 `LuaSprite.cpp` 的 Image 替换直接调用 `LayerState::SetSpriteSheet`），会表现为图层状态或资源序号的变化，由 memcmp 发现，不依赖 Hook 覆盖率。

## 改动范围

- 布局源中写入基础类型条目。
- Host 共用的容器读取与校验辅助函数，以及 Client 的容器重建辅助函数。
- 资源表的数据结构。

## 失败方式

- **对齐判断错误**：把对齐字节当成状态，memcmp 误判；或者把状态当成填充，导致漏字段（例如 `0x31 unk`）。
- **派生判断错误**：认定由文件决定的值，实际在运行时可改（例如默认动画名），Client 因此漏写。
- **容器指针失效**：容器指针在复制过程中被游戏改写。只能在主线程合法窗口内读取，并且只能读一次。
- **BlendMode 不明**：内部布局没核清，导致混合方式写错。
- **资源表不同步**：资源序号先于资源表到达 Client，或资源表重编号后，旧帧中的序号被误解为新路径。

## 完成内容

- **核对与入源**：上述类型全部完成 J273 核对，写入布局源，生成两张单行表。
- **规则落地**：嵌套规则由生成器强制执行，资源表格式在第 9 步定稿。

## 未决问题

- `AnimationState._layerFrames`、`_nullLayerFrames` 是否只由帧号派生。
- `ANM2._animDefaultName` 在运行时是否会被修改。
- `BlendMode` 的内部字段。
