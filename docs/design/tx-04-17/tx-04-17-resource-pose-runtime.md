# tx-04-17 ST5：资源准备、动态换图与完整姿态

研究日期：2026-10-31，Asia/Singapore。状态：**ST5指定静态研究、与ST6／ST7来回互核及主智能体审阅完成，具体UNKNOWN保留**。本项由一个GPT-6 Luna／max子智能体推进；纠错与接受范围见[工作流程主审记录](tx-04-17-workflow.md)。本报告属于 tx-04-17 的先行静态阶段，不代表 Client 资源运行时或整个 tx 已完成。

## 1 范围与证据标记

目标版本为官方 REPENTOGON 1.1.3 与 Repentance+ v1.9.7.12.J273。优先采用公开 `Sprite`、`LayerState`、`AnimationData`、`AnimationLayer`、`AnimationFrame` API，并用 REPENTOGON 1.1.3 绑定源码区分文档声明和当前源码行为。J273 地址以下均为 VA；imagebase 为 `0x400000`，RVA = VA − `0x400000`。

| 标记 | 含义 |
| --- | --- |
| `PUBLIC_DOC` | `F:/projects_isaac/IsaacDocs` 与 `F:/projects_isaac/REPENTOGON-1.1.3/docs/docs` 中的公开 API 语义；不证明 native ABI、实际 context 或像素结果。 |
| `CURRENT_SOURCE` | 本地 REPENTOGON 1.1.3 Lua 绑定／辅助源码；不是运行时观察。 |
| `J273_STATIC` | 本次在共享 J273 IDA 数据库内只读定位、反编译或反汇编的静态事实；不证明运行时命中。 |
| `REUSED_J273` | ST2／ST4／ST6／ST7 已核定并经消息反馈的本版本事实，本报告在互核后引用。 |
| `J460_INDEX` | 03-13—20 的 v1.9.7.17.J460 研究，仅作缺口索引；地址、偏移、布局和后态不移植到 J273。 |
| `DESIGN`／`UNKNOWN` | 提议的候选管理约定／当前证据未闭合的问题。 |

只读 `server_health` 确认当前 IDA 输入与 ST2 相同：`isaac-ng-v1.9.7.12.exe.unpacked.exe`，对应 IDB `isaac-ng-v1.9.7.12.exe.unpacked.exe.i64`，imagebase `0x400000`。没有切换数据库、编辑 IDB、改名、写注释、保存 IDB、运行游戏、构建或测试。只编辑本文件；没有改共享入口或生产代码。

## 2 结论摘要

**资源准备不是单一的“loaded”布尔值。** 至少要分开记录 ANM2 描述已装入、动画／层结构符合目标、每个需用图像已关联、加载调用处于已确认的主线程／renderer context、目标姿态完整、候选可绘制、实际画面可见。构造器第二返回值、`IsLoaded()`、`ReplaceSpritesheet()` 的 bool、`LoadGraphics()` 返回以及 `GetSpritesheet()` 得到非 nil 都不能合并为 GPU 资源可用或像素正确。

公开 API 足以表达本任务需要的实例创建、main／overlay 选择、整数时间帧、逐层姿态、颜色／变换／flags、按层换图与加载图像调用。公开 API **不提供事务式 Load／Reload、整套图像成功结果、任意分数帧 phase 或每个 native selector 的原样快照**。J273 静态代码还显示 Load 会先处理旧 filename/cache 引用后重绑定，Replace 只改目标层路径及该层加载标记，Reload 会重绑并重新走图像加载；这些都没有回滚结果。未发布候选和代次隔离因此必须成为产品管理方式。

ST4 已裁决原生 `MC_POST_RENDER`／派生 render 与动画事件消费者不是 Client 的通用安全边界。ST5 不通过本地 `Update()` 推进 Authority 姿态，不让 Client 的本地动画事件驱动 gameplay consumer，不以一个 Sprite 代替 Player 组合、附属对象、阴影、曲线或特殊 renderer pass。[ST4 展示边界](tx-04-17-render-presentation-boundary.md)列出的适配责任仍由 18—20／27 承接。

## 3 构造、加载、换图与重载的实际后态

### 3.1 调用顺序与公开结果

| 操作 | 公开／绑定语义 | J273 静态后态与使用边界 |
| --- | --- | --- |
| `Sprite(anm2Path, loadGraphics)` | `PUBLIC_DOC` RGON Wiki 声明返回 `Sprite, boolean`；`CURRENT_SOURCE` `main_ex.lua:2107–2120` 在传入路径时执行 `Load`，第二返回值实际为 `GetLayerCount()>0`，无路径时为 false。 | 构造对象和有层的 ANM2 结构与图像／GPU／像素成功是不同状态。不要把这个 bool 当“全部图像已加载”。 |
| `Load(anm2Path, loadGraphics)` | `PUBLIC_DOC` 返回 void；路径相对 `resources`，第二参数控制是否随后调用 `LoadGraphics()`。 | `ANM2::Load` VA `0x407AF0` / RVA `0x7AF0`：已分析路径先处理旧 filename 引用（调用 VA `0x407B00` → `0x409BB0`），再由 `0x407B23` → `0x409680` 绑定资源，`0x407B29` → `0x409A40` 增加引用，按参数选择图像加载，尾部 VA `0x407B52` 写 `_loaded` 标记。无业务返回值；不能由该标记推出层／PNG／GL／像素成功。ST7 另核到 cache miss 会写 warning 且保留的旧缓存借用不是新资源的成功回退；旧 LayerState／AnimationData userdata 不能因名字、指针相同而继续借用。没有旧目标事务式回滚保证。 |
| `IsLoaded()` | `PUBLIC_DOC` 返回 boolean。 | 它表示 Sprite 的 loaded 标记，不是所有 LayerState、Image wrapper、GL texture 或画面验证的总结果。无可供网络线程使用的 readback 成功码。 |
| `ReplaceSpritesheet(layerId, pngFilename)` | `IsaacDocs` 对 Repentance+ 声明返回 boolean：目标层有效且新路径不同才为 true，同路径会 false；要求随后 `LoadGraphics()`。RGON Wiki 的修改签名列 `void`，但 `LuaSprite.cpp:26–44` 当前绑定实际返回 bool。`ReplaceSpritesheet(..., loadGraphics)` 的扩展参数默认 false。 | `ANM2::ReplaceSpritesheet` VA `0x407CD0` / RVA `0x7CD0`：越界或同路径返回 false；不同路径复制该层的路径并把该层 `_spriteSheetLoaded` 标为 false 后返回 true。不会在这一步完成图像加载，也不处理其他引用同一 PNG 的层。False 可能只是同路径 no-op，不应单独作为资源错误；先以 `GetLayer(id)` 和路径元数据核对目标。替换后、LoadGraphics 前不得发布或开始使用候选。 |
| `LoadGraphics()` | `PUBLIC_DOC` 返回 void；官方样例在 Load 或 Replace 后调用。当前 `Lua_SpriteReplaceSpritesheet` 仅在 `successful && loadGraphics` 时自动调用它，忽略其结果且仍返回 Replace 的 bool。 | `ANM2::LoadGraphics` VA `0x407DA0` / RVA `0x7DA0` 按层处理未标记已加载的 sheet，并将每层状态写回；没有 aggregate success。IDA中的局部 `v24` loaded witness 在后续图像 pair 的最终成功之前就会预写，因此只表示过程中的状态，不证明 Image pair 最后建立成功。缺 PNG／资源解析失败可形成部分层后态。不能把调用返回、某一层成功、路径存在或 Image userdata 非 nil当成所有层已备妥。 |
| `Reload()` | `PUBLIC_DOC` 返回 void；没有公开事务、撤回或恢复成功码。 | `ANM2::Reload` VA `0x407C40` / RVA `0x7C40` 在有已存 filename 时重新绑定／增加引用并调用 `LoadGraphics()`；ST7 静态复核确认它会重建 LayerState，且没有先像 Load 那样减旧引用的步骤。不要把 Reload 用作每帧刷新或候选回滚；无论路径、ANM 缓存命中结果如何，都将关联资源修订和全部内部借用视为需重新检查／获取。 |
| `SetSpritesheet`／`LayerState:SetSpritesheet(image)` | `PUBLIC_DOC` LayerState API 记为 void；当前 `LuaSprite.cpp:46–60,468–474` 直接赋 Image 关联，Sprite 版本额外返回 `layer != nullptr`。 | 返回值只说明 LayerState 是否存在，不表示图片正确、已上传、context 合法或渲染成功。LayerState setter 会将 Image smart reference 赋给该层，并按 image 是否为空写 layer loaded marker；这不说明 GPU/pixel 正确。该方式改变实例图像关联，不等于路径替换或 `LoadGraphics()`；仅在明确准备中的候选使用。 |

`LayerState::GetSpritesheet()` 不只是无副作用查询：`LayerState.zhl` 注释称其会惰性同步 filter/wrap；绑定构造 `Image` userdata 并复制 image smart reference。它不能在线程／GL context 不明时作为健康检查。ST6静态互核要求图形资源读取、加载与变换同时处于主线程和 OS current guard 通过的窗口；当前尚无已证明的菜单／Manager render 通用窗口。图像引用可以延长 Image 自身关联，但不延长它来自的 LayerState、Sprite、ANM cache 或 renderer/context 的寿命（ST7互核）。

**最后 strong reference 的正常静态释放路径（J273_STATIC，ST6互核）**：REPENTOGON `SmartPointer.zhl` 的 `KAGE_SmartPointer_ImageBase::DecrRef` 对应 `sub_4081A0`，在 `0x4081D1` 调 reference counter vtable `+0x0C`（`DecrementReference`，`sub_408500`）；strong count 从 1 减到 0 时，`0x408536` 调 Image vtable `+0x60` 的子类 destructor（参数 1），继而进入对应 Image 子类的 `sub_A4C360`，于 `0xA4C369/0xA4C375` 调两次 `epoxy_glDeleteTextures`。这给出正常引用释放的静态调用链，但不证明指定候选实际走到该分支、删除 API 成功、当时 OS context 有效或 GPU 已 idle。独立的 `0x9EFE60→0x9EF7C0` 是 DecrRef 里的可选 ImageManager/cache/CPU queue remove callback（counter decrement 返回 true 且 callback 非空，callback 内另有 strong-count==1 条件）；它不是 Image destructor 或最终 GL delete。

### 3.2 失败处理

- `Load`、`LoadGraphics`、`Reload` 和姿态帧 setter 没有可据以确认原子提交的业务返回。任何准备步骤出错、返回缺项或上下文未确认时，都让候选保持私有并报告具体失败阶段；不能恢复旧字符串或用“旧画面应该还在”作事务保证。
- Replace 的 false 需区分预期同路径 no-op 与无效 layer；若调用者无法用描述信息和 `GetLayer(id)` 分辨，就拒绝目标而不继续后续帧写。图像加载缺失／部分失败时不能把候选当完整状态发布；当前公开接口没有汇总层成功的确认字段。
- 不因候选失败立即在错误 owner／context 调 `Reset`、析构或手动释放 native userdata。未发布对象按 ST6／ST7确认的本地 owner 与 context 退休；Lua GC、Sprite Reset、Image refcount 和 GL 最终释放不是同一个完成信号。
- 使用 `Copy()` 不能替代自有候选：当前绑定是 `construct_from_copy`，公开语义不承诺字段全复制或事务切换。候选与已发布 Sprite 不浅拷贝 native 内存、不共用内部 userdata。

## 4 主动画、overlay 与完整逐层姿态

### 4.1 选择器与时间域

- main 与 overlay 是独立 `AnimationState`。使用 `SetAnimation(name, true)` 和 `SetOverlayAnimation(name, true)` 建立明确目标；返回 false 时目标不完整，不发布。按名称的 `SetFrame(name, frame)` 没有业务返回值，不能代替动画存在性检查。Overlay 需分别表示不存在、动画名和渲染先后；`RemoveOverlay()`、`SetOverlayRenderPriority()` 只改变 overlay 语义，不应用一个空名字去代替 absent。
- `SetFrame(int)`、`SetFrame(name, int)`、`SetOverlayFrame(name, int)`、RGON `SetOverlayFrame(int)` 的参数都是动画**时间帧**整数，`SetLayerFrame(layerID, frame)`／`SetOverlayLayerFrame(layerID, frame)` 同样接收时间帧，不接收 layer 的 key-frame 数组下标。RGON 的 overlay 单整数重载按 Wiki 是“当前动画设帧但不停止动画”。`GetFrame()`／`GetOverlayFrame()` 也只给整数。要应用静止 Authority 姿态，可先显式选动画、停用相应播放，再设帧；不能把 `Update()` 当成世界 tick。
- `AnimationData:GetAllLayers()` 是 bottom-to-top 绘制顺序数组，不按 layer ID 编号；`AnimationData:GetLayer(id)`、`AnimationLayer:GetLayerID()` 才用于 layer 身份。`AnimationLayer:GetFrame(frame)` 查询时间区间中的烘焙帧记录，`AnimationFrame:GetEndFrame()` 是下一记录起点。协议和本地数组不得混淆 animation frame、layer ID、order、selected-key index。
- `AnimationFrame` 的公开只读字段含 start/end、pos、scale、pivot、crop、rotation、color、width/height、visible、interpolated。`GetCurrentAnimationData()`／overlay data、`GetLayerFrameData()`／overlay layer frame data 可读取缓存记录，但不把只读记录当可写姿态或资源 owner。

### 4.2 颜色、几何、可见性与 flags

Sprite 与每个 LayerState 都支持分别设置／清除／查询 custom shader 与 custom champion shader。每个范围都要在资源描述里区分两种shader的期望路径和显式未设置；setter `void` 也可能通过 Lua error 失败，不能把抛错路径视为成功。`HasCustomShader(path)`／`HasCustomChampionShader(path)` 只能核对已指定路径或是否存在任意 shader，没有返回当前 shader path 的列举接口，因此路径由候选描述提供，不从旧 Sprite 猜回。`Has...` 为 true 也只证明绑定保存了指定shader关联，不证明 GPU pipeline、输入与像素正确。

`CURRENT_SOURCE`：`Anm2Extras.cpp` 将 default／layer shader extras 保存在以 `ANM2*` 为键的 map；其内部 `layer_extras` 是 `unordered_map<int, Anm2LayerExtras>`，以 `layer->GetLayerID()` 取值，不以 LayerState* 为键（78–92, 105–136, 173–185）。官方 Reset 与 destructor hook 在 `super()` 前 `erase(this)`（85–91）。本次未见普通 `Load`／`Reload` 主动清该 ANM2 owner map；换图／Load／Reload 后绝不依赖旧 default 或旧 layer-ID shader entry仍代表相同资源语义，候选按当前资源描述分别重放 shader 配置。若 setter 报 Lua error，记录精确失败阶段／path／执行前缀，候选不发布；不能默认 wrapper 对 partially-mutated extras 有回滚保证。

Root Sprite 姿态至少独立表达 `Color`（按原语义保留，不压扁成四个 RGBA 字节）、FlipX/FlipY、Offset、Scale、Rotation、OverlayRenderPriority 和完整 `AnimRenderFlags`。`PlaybackSpeed` 是本地播放控制；不作为客户端权威时钟。`Stop`／`Continue` 和 overlay 版本须分别作用于 main 与 overlay。

每个 AnimationLayer 的实例 LayerState 还需独立保留：spritesheet 关联／路径、Visible、Size、Rotation、Pos、Color、FlipX/Y、CropOffset、RenderFlags、WrapS/T 和完整 `BlendMode`（RGB／Alpha source、destination factors及Equation）。`GetBlendMode()` 返回指向 LayerState 内部对象的 userdata；`LuaBlendMode.h::NewPointer` 不取得 LayerState 所有权，必须随资源代次撤销。当前 `LuaBlendMode.cpp` 对 factor/equation 做 enum 范围检查；快照层先保留字段原语义，再按目标 enum 校验，不能把 BlendMode 省略成“只读”或持久保留旧 getter。姿态输入先验证 finite、范围、资源 layer ID、动画存在性与层映射，再在 owner 线程应用。当前 getter 多返回复制 Vector／Color；LayerState setters 直接改字段且无 finite/range 拒绝、无业务成功码。不得为躲避特殊渲染副作用清零未知 flags；完整特殊几何／额外 Sprite 继续交由 18—20／27 适配。

### 4.3 整数帧与相位缺口

`AnimationState.zhl` 的 J273 布局把当前动画时间存为 `_animFrame` float（+0x10），而公开 `GetFrame`／`SetFrame` 接口为 int。J273 main 上有三个需分开的 setter 入口：VA `0x406010` / RVA `0x6010` 对应 ZHL 声明的 named `std_string*` 版本；VA `0x406060` / RVA `0x6060` 是另一 named `const char*` 入口；VA `0x4060A0` / RVA `0x60A0` 是无动画名的 int 版本。两个 named 入口均先检查动画名，再进入位置更新 helper VA `0x4042A0` / RVA `0x42A0`，之后清除 main state 的 playing 标记；无名 int 入口直接设位置，不清该标记。此处按 ZHL 与调用点区分参数形式，不用 Hex-Rays 伪参数名推定 ABI。`0x4042A0` 把 frame 存为浮点值，并将大于动画长度的上界夹到 `nextafterf(length, 0)`，清理并重算当前事件位，再按层更新选择的烘焙记录。负帧在该 helper 中没有对称的下界夹取。Adapter 应在调用 setter 前拒绝负帧与超出目标动画长度的输入；不能依赖 API void 后态判断成功。

逐层 setter 的当前路径使用 layer ID 映射到层顺序，再从 key index 0 前进至时间帧对应记录；main native 入口 VA `0x406160` / RVA `0x6160`，内层 `AnimationState::SetLayerFrame` VA `0x404390` / RVA `0x4390`，helper VA `0x404030` / RVA `0x4030`。VA `0x4043E0` 的 `id out of bounds` 是诊断调用；无效 descriptor mapping 时函数随后仍执行对 `layerFrames[mapping]` 写 0 并调用推进 helper，日志不是拒绝或安全返回。适配必须先用当前 AnimationData 的公开 `GetLayer(id)` 确认层存在、且目标帧在对应轨道有效，再调用 setter；不能把诊断当作保护。`GetLayerFrameData` 的绑定会依 selected-key 取记录并 clamp selected-key；因此 readback 非 nil 不能证明原始 selector 本身是有效精确值。J460 中关于 selected `-1` 的观察只作索引，本轮没有证据将其提升为 J273 保证。

**反馈缺口（15 与 18—20）**：整数时间帧不能表示任意分数 phase；J273 结构确有浮点 `_animFrame`，但公开 Sprite getter／setter不提供该精度。本次尚无经目标动画样本或 15 状态定义证明的“每个实体都必须同步 fraction”结论；也不能把此未知变成缩小完整表现内容的理由。15 应先将 main、overlay、各层 pose 的语义区分为整数时间帧与可选 phase；若正式状态/实际资源需要精确插值 phase，提供稳定有界的分数表示和来源 tick，并由 18—20 的呈现 adapter 使用隔离副本或明确支持路径，不能改动全局模拟帧或写未知 native key vector。对于是否存在必须表达的无 selected key／隐藏 selector，15／18—20应以 J273 实际有效资源补证；若需求成立，应单独表达“无有效帧／隐藏”，不能用 getter 的 clamp值替代。

## 5 候选准备、检查、发布与退休合同

以下是 `DESIGN`，用于把资源行为反馈给 15／16；不是已实现协议或运行时 PASS。

| 阶段 | 合同 |
| --- | --- |
| Prepare | 接收有界纯值描述和目标资源修订，进入已由 ST6 确认的游戏主线程／OS-current guard 后创建全新私有 Sprite。构造、Load、替换所有必要图层路径、调用 LoadGraphics、选择 main／overlay、设置姿态及按描述设置 root 与各层 custom/champion shader 都只作用于候选。Shader path 来自描述且未设置态显式清除；Lua error 作为失败处理，保留失败操作前缀，不发布候选。网络线程只排队不可变值，不持有 Lua userdata、LayerState、Image、Player 或 GL handle。 |
| Inspect | 校验 ANM2 已有非空且目标动画存在、overlay 有无／顺序合法、每个 required layer ID 映射存在、动画长度与时间帧边界合法、shader descriptor 与当前candidate的显式期望一致、BlendMode enum有效、图像关联／LayerState读值可访问、所有姿态值有限。`HasCustomShader(path)`／`HasCustomChampionShader(path)` 只核候选保存指定路径关联；没有shader path getter，不从旧实例反推。`GetSpritesheet()` 可能做惰性 filter/wrap 同步，只允许在 ST6确认的主线程且 OS current guard 通过的 graphics owner/context 内调用；即使非 nil，也只记为 image association evidence。shader/image readback都不能证明 GPU／像素正确。 |
| Commit | 候选始终私有，直到完整目标描述和可读检查通过，并到达主线程边界：当 registry 关联不再被自有 callback/draw 正在访问时，一次性将完整候选替换为新的 resource revision。**旧 Sprite／Layer／Image 引用不能在 registry swap 时释放**，但不必等 raw queues 清空才进行逻辑 swap；旧代次需留在待退休表。ST6在 renderer Present 候选 VA `0x9E4170` 中核到一个局部调用顺序：`apply_frame_images` `0x9EFA70`、frameImages reset `0x9EFA10`（逐项清 `_frameImages` flags bit 4，并在 VA `0x9EFA47` 将 `_frameImages.end` 复位到 begin），之后才做renderer/vcall与条件 `SwapBuffers` `0x9E41C8`；ImageManager cache/queue removal callback 路径 `0x9EFE60`／`0x9EFE90` → `0x9EF7C0` 在 `0x9EF844/0x9EF84F` 从 frameImages memmove 并令 end 减 4，另清 tracked bit-8 vector 及透明 pair；它有 strong-count 条件，不是 Image destructor 或 GL delete。该局部序列不等同 `Manager::Render`（其地址为 `0x92EC40`），也不能泛化为所有 render、loading bypass、stop 或退出路径。它只证明已见普通队列／透明 pair 在这些 reset/remove 点被清理；其他 batch/cache raw owner 是否也不再持有旧 ImageBase* 仍须单独闭合，不能由队列末端归零推定。正常 strong reference 从 1→0 到 Image 子类 `+0x60` destructor、`0xA4C360` 与两次 `glDeleteTextures` 的静态链已见，但不证明本次候选实际命中、API成功、OS current有效或GPU idle；`apply_frame_images` 返回或 `SwapBuffers` 仍不是GPU完成证明。不得逐层发布半套资源或用 `IsLoaded`／ctor bool／replace bool／resource ACK 冒充 World Applied。旧画面能否继续显示取决于 15 的正式恢复语义；失败目标不覆盖已发布版本。 |
| Replace | 动态换图建立新 resource revision 的未发布候选，显式标出需要改变的 layer ID／路径。Replace true 只说明路径有变化，false 需按同路径 no-op 或无效 layer 分类；每次图像调用结束后再检查可观察状态。已发布 Sprite 不在绘制中原地 Replace，也不假设旧 Image／LayerState／AnimationData userdata 仍有效。新候选发布后，旧引用仍等已证明覆盖相关 batch/cache raw pointer 的 owner 清理后才允许退休；目前只确认指定 CPU queues 的 reset/remove 点，其他 owner 与 GPU 最后使用待核。 |
| Cancel / Retire | 先停止对旧 revision 的新姿态与 draw 发布，等待本地 callback／自有 draw 停止，再等 ST6确认的 renderer queue reset/remove owner 清除已见 frameImages／透明 pair 项；其他 batch/cache raw ImageBase* 是否都不再待用仍需单独证实。之后按 ST7确认的 Lua state 与 resource owner 顺序撤销 borrowed userdata/Image关联，最后由正确 owner/context退休资源。旧对象移出注册表、`apply_frame_images` 返回或 `SwapBuffers` 都不等于所有GPU batch／raw queue／Lua GC的最后一份引用已释放。若相关 owner 未闭合，不能声称安全退休；应保持有界代次引用并停止继续触发可能使旧引用失效的资源动作，直到安全清理点明确。Context／Lua代次关闭后绝不重取或继续访问旧借用。

候选的本地身份至少隔离 `session/scene epoch`、`resource revision`、本地 Sprite instance generation、Lua state generation 和 renderer/context generation。它们是本地有效性范围，不要求作为 native pointer、userdata 或硬编码 ABI 发到网络。任何 `Load`、`Reload`、spritesheet 关联变化、ANM cache rebuild、Lua close 或 renderer/context 重建都让相关借用重新获取；文件名相同、native 地址相同、上一代 `GetLayer()` 返回非 nil 不能恢复代次有效性。

保留待清理的旧 revision 必须同时受数量和总字节预算约束。renderer queue cleanup 尚未确认且预算耗尽时，停止接收新的动态资源 revision，继续保持隔离；不能无限留存，也不能为腾空间猜测性释放仍可能被 raw queue 引用的 ImageBase。

## 6 给 15／16 与展示适配的字段反馈

### 反馈给 15：语义和错误

- 资源目标携带明确的资源修订／描述键；局部 resource-ready、姿态设置完成与服务器世界状态 Applied 是不同结果。可描述的结果至少区分 `candidate constructed`、`ANM parsed/required animations found`、`per-layer image association unknown/available/missing`、`pose valid`、`published`、`cancelled`、`superseded`、`retired`。不能声称 public API 提供了 GPU/pixel aggregate success。
- 资源失败说明目标 revision、失败层／动画、已经执行到的阶段、失败原因和是否曾发布；同 revision 重复与冲突区分。不要只报一个 bool，因为相同路径 Replace 返回 false 是已知 no-op。
- 资源描述需保存 Sprite default 与逐层独立的 custom/champion shader 两种可选 path（或显式 absent/clear）；`HasCustomShader`没有列举 path 的 getter，所以不能从现有实例重建完整shader目标。shader setter 的Lua error、path、失败前缀与shader association检查结果纳入候选错误，Has成功不是像素/World Applied。
- 姿态快照需明确 main／overlay 有无、名字、整数时间帧、overlay 顺序、每层 `layerID`、layer timeline frame、Visible、Color/transform/crop/render flags。`GetAllLayers()` 的顺序不是 layer ID；不能传 native selected-key index 充当公开 `SetLayerFrame` frame。
- 若 15 或真实 J273 资源证明需要小数相位，增补有界 phase（连同参考帧／来源 tick）；不要默认全局模拟 tick 是纯视觉时钟。若需要表达无有效层帧，使用有无帧／隐藏的显式语义，而非 clamp 后 readback。

### 反馈给 16 与 18—20

- 16 的资源服务必须在已核定主线程和 graphics context 的窗口执行；加载旁路、实际 context current、batch 最后引用和 context 销毁前释放位置仍由 ST6 实证。Main/Manager render callback 的名字或 TLS 值不构成 context 已有效的证据。
- 18—20 从 authority 纯值应用本地姿态；不得执行玩法 Update、动画事件 consumer、Entity Render Lua 或复制单 Sprite 以外的未闭合原生 draw 树。表现专用插值/phase 不能写回共享 AnimationFrame、Global game frame、旧 key-vector 或 Authority fields。
- 失败候选保持 Client 隔离，退出/换代先退 publisher 和 draw，再清理资源。不得因资源准备失败切 Off 后让残留原生世界继续更新。

## 7 证据、未知与核对记录

### 主要本地来源

- `PUBLIC_DOC`：`F:/projects_isaac/IsaacDocs/docs/Sprite.md:128,170–200,269–271,326–345,362–399`；`F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md:7–23,38–45,80–139,148–166`；`.../LayerState.md:9–16,23–172`（shader methods见87–120）；`.../AnimationData.md:13–33`；`.../AnimationLayer.md:7–22`；`.../AnimationFrame.md:13–65`。
- `CURRENT_SOURCE`：`repentogon/resources/scripts/main_ex.lua:2107–2120`；`repentogon/LuaInterfaces/LuaSprite.cpp:26–84,114–155,173–218,273–447,468–490,508–690`；`LuaRender.cpp:228–231`（`Image` userdata GC 对其 image smart reference 执行 `DecrRef`）；`LuaBlendMode.h:26–34`（borrowed pointer wrapper）与 `LuaBlendMode.cpp:14–24,41–92,203–305`（factor／equation属性和边界检查）；`LuaAnimationData.cpp:5–146`；`LuaAnimationLayer.cpp:5–49`；`LuaAnimationFrame.cpp` 对只读字段包装；`Anm2Extras.cpp:78–92,105–136,173–185,205–234`（ANM2 owner map、内部layerID extras map、Reset/destructor清理）；`SmartPointer.zhl`（Image strong ref）。REPENTOGON `Sprite` 文档对 Replace 的 void 与源码当前实际 bool 返回不一致；shader path无列举getter；绑定源码只说明目标源码行为，未通过运行观察确认安装行为。
- `J273_STATIC`：`libzhl/functions/ANM2.zhl`、`AnimationState.zhl`、`LayerState.zhl` 的签名与 struct 声明用于定位；只读目标核对 `ANM2::Load` `0x407AF0`、`LoadGraphics` `0x407DA0`、`ReplaceSpritesheet` `0x407CD0`、`Reload` `0x407C40`、main `SetFrame` named `0x406010` / `0x406060`、int `0x4060A0`、`SetLayerFrame` `0x406160`、`AnimationState::SetLayerFrame` `0x404390`、时间／事件 helper `0x4042A0` 与 layer 时间映射 `0x404030`。没有对它们作重命名、注释或数据库保存。
- `REUSED_J273`：ST2 复核 Game/Room/Cam 与菜单渲染边界；ST4 裁定游戏/Entity renderer、动画事件 consumer 与 HUD 不作为无副作用透传；ST6核 `ImageBase::Render` VA `0x9E2170` → `0x9E2460`、VA `0x9E2C8C` 将 `ImageBase*` 放入 ImageManager `_frameImages` raw vector、batch获取 VA `0x9E2EC0`、`apply_frame_images` VA `0x9EFA70`逐帧读取queue并提交`glDrawElements`（VA `0x9EFD23`）。透明pair结尾在`0x9EFDEB`更新；renderer Present候选 VA `0x9E4170`在该局部路径依序 apply→普通队列 reset `0x9EFA10`（逐项清 `_frameImages` bit 4；`0x9EFA47` end=begin）→renderer/vcall→条件SwapBuffers `0x9E41C8`；`Manager::Render`另为VA `0x92EC40`。ImageManager cache/queue removal callback `0x9EFE60`／`0x9EFE90`→`0x9EF7C0`，在`0x9EF844/0x9EF84F`从frameImages memmove/end-=4，并同步清tracked bit-8 vector及透明pair；它有 strong-count 条件但不是Image析构。正常 strong count 1→0 在 `0x408536` 调 subtype vtable `+0x60`，子类到 `0xA4C360` 并在 `0xA4C369/0xA4C375` 执行 `glDeleteTextures`；`0x9EFE60` queue/cache callback 与此析构链独立。以上只闭合所见CPU队列项与正常静态释放链；不证明其他batch/cache raw owner、实际OS current、API成功或GPU idle；局部队列顺序也不证明所有render／stop／退出都经过此点。ST7复核Lua借用与Load/Reload owner/revision变化。J460的03-13—20只作“replace/reload、phase与selector有效性需核对”的问题索引。

### 明确 UNKNOWN

1. 实际安装 J273/RGON 在正常 render、菜单、加载旁路、退出时的 thread/context/current 与 `LoadGraphics`／`Reload` 可调用窗口；读源码和静态 caller 不替代 ST6 与后续 E2E 观察。J273普通CPU队列 reset/remove 点已静态闭合为 `0x9EFA10` 与 `0x9EF844/0x9EF84F`，正常 strong-reference释放到子类 GL delete 的静态链也已定位；但具体正常stop/异常退出是否都经过相关清理、其他batch/cache raw owner是否都撤销同一ImageBase*、当前OS context、GL API结果及GPU idle仍 UNKNOWN。
2. 当前正式 15 状态是否必须表示任意 `AnimationState._animFrame` 分数相位；目前确认存在 float native 状态和公开整数 API缺口，但实际必需字段由 15／18—20基于产品姿态要求与 J273 资源补证。
3. 当前指定 J273 资源是否含有必须表达的未选帧／隐藏 key selector，还是显式 LayerState/AnimationFrame visibility 足以表达目标画面；不能把 J460 `-1` 结论移植。
4. 缺失／坏 ANM2、部分 PNG 失败、resolver cache miss 时各路径的最终 Image/GPU 与旧缓存引用是否安全；无返回码的 Load/Reload/LoadGraphics不能作为恢复合同。
5. Lua close、resource owner、renderer batch 最终 release 的实机先后与实际 DecrRef 最终分支命中时点；静态正常 strong-reference释放链已见，但由 ST6／ST7补足运行条件，不由资源 getter、`Reset()`、析构请求或 Sprite 被移出表推定。
6. custom/champion shader 的请求路径、显式有无和失败字段已由 ST5 纳入候选；仍未知的是实际安装下 shader program／renderer pass 的有效期、完整原版 shader依赖集合及其与角色组合、附件、光照／shadow、曲线、beam的关系。ST4 已将原生 render 树限制交给 18—20／27逐类适配，本报告不把它们合并成一个 Sprite。

### 互核修订

- 首版曾将 Replace 的“当前 RGON wiki 签名 void”按文档直述；回读 `LuaSprite.cpp` 后补正为**文档写 void、当前源码返回 bool**，并将其 bool 限定为 native path replacement。原版 Wiki Repentance+ 的 bool 和 RGON optional `LoadGraphics` 也分别记录，不混为单一版本事实。
- 主审回读指出 `LayerState:GetBlendMode()` 的返回 `BlendMode` userdata 有可写 RGB／Alpha factor 和 Equation 属性，并会做 enum 边界校验；已修正首版“仅有 getter”的错误，保留完整 BlendMode 语义，同时按 pointer wrapper 标记其依附 LayerState 代次。
- 主审要求将公开 custom/champion shader 放进资源与姿态字段，而不只放 UNKNOWN；已补 Sprite／逐层两个范围的路径、absent/clear、Has查询、Lua error与失败前缀。当前无path列举getter；`Anm2Extras.cpp` 的外层map以`ANM2*`为键，内层以layer ID为键，Reset/destructor hook清除ANM2 owner map，普通Load/Reload未见清理，候选须从描述重放shader目标，不依赖旧layer地址或同名状态。
- 主审按当前 `ANM2.zhl` 与调用点独立确认 main `SetFrame` 有 `std_string*` named、`const char*` named、无动画名 int 三个入口；已将 `0x406010`、`0x406060`、`0x4060A0` 分列，不以 Hex-Rays 伪参数名推 ABI，且记录两个 named 路径会清 playing 标记、无名路径不清。
- 主审核对 `AnimationState::SetLayerFrame` 的实际指令后，补入 VA `0x4043E0` 诊断后仍写 selector 并调用推进 helper 的事实；不把错误日志当越界拒绝。输入先通过 AnimationData 的有效 layer／轨道元数据验证。
- 依据 ST7 的只读互核，将 LayerState、AnimationData、AnimationLayer、AnimationFrame 记为不保活父对象的 borrowed userdata；Image wrapper 的 smart pointer只延长 Image 关联；同名动画／相同路径／指针相等均不跨 resource revision 证明借用有效。
- 依据 ST7 反馈，区分“borrowed对象物理上仍驻留”与“资源revision语义仍有效”：Replace改路径后旧LayerState pointer可能仍在，Load cache miss也可留下旧layer数组；这不是目标revision或旧画面成功回退。设计仍在每次关联／Load／Reload变化时撤销旧借用并重取，但不声称每次调用都已物理free旧对象。
- 依据主审独立回读及 ST7 补充，收紧 Load/Reload 失败合同：Load 先处理旧 filename 引用再绑定新资源，Reload 不先减旧引用且会重建 layer；两者均无旧候选回滚成功码。失败处理因此不规定盲调 Reset/dtor或复用旧画面，改为候选保持私有并按 ST6／ST7已核 owner/context退休。
- 依据主审提醒将 `GetSpritesheet()` 从普通 metadata 检查移出；`LayerState.zhl` 说明 getter 会惰性同步 filter/wrap，必须服从 ST6 的 graphics owner/context窗口，Image 非 nil仍仅为引用关联证据。
- 依据 ST6互核更新动态换图退休顺序：初稿误记普通队列clear owner尚未定位；后续在 J273找到 `0x9E4170→0x9EFA70→0x9EFA10` 的CPU submit/reset顺序、清 bit 4 与 `0x9EFE60/0x9EFE90→0x9EF7C0` remove路径后修订。该证据只闭合所见 CPU queue 项，不推导其他 batch/cache raw owner 都已清理；logical registry swap与旧Image强引用退休分开，队列清理闭合不等于GPU idle或最终GL删除已完成。
- 依据主审回读，纠正对 VA `0x9E4170` 的归属表述：它只记为 renderer Present 候选路径，`Manager::Render` 为 VA `0x92EC40`；文中 `0x9EFA70→0x9EFA10→0x9E41C8` 只描述该局部静态路径，不代表所有渲染、stop 或退出分支。
- 依据 ST6／ST7最终互核与主审纠正，将 `0x9EFE60/0x9EFE90→0x9EF7C0` 明确限定为带 strong-count 条件的 ImageManager/cache/CPU queue remove 子路径，不再称为最后引用、Image destructor 或 GL delete；补入 `0x9EFA10`逐项清 bit 4、`0x9EFA47`复位end、image remove memmove/end减4及 tracked bit-8／透明pair清理。另补入正常 strong-reference 从 1→0 调 subtype `+0x60` destructor 并到 `0xA4C360`／两次 `glDeleteTextures` 的静态路径；实际命中、OS current、API结果、GPU idle和异常路径覆盖仍 UNKNOWN。
- 依据主审对 `LoadGraphics` VA `0x407DA0` 的回读，补充局部 `v24` loaded witness 在最终 image pair 成功之前已预写；不把中间 witness 当最终配对或图像成功证据。
- ST7终稿互读确认 borrowed Sprite 对象表已区分本方 `UserdataValue<ANM2>` 与 Entity／Room `UserdataPtr`；正常 finalizer 再次进入 destructor/Reset 的静态路径与异常行为、Lua close／ANM cache／完整 ImageManager/context teardown分开记录。已向ST7指出并确认修正两处“last-ref path”误称。

本轮仅资料阅读、只读 IDA 静态分析、人工回读及本文件编辑。没有运行测试、构建、游戏或离线校验脚本，没有保留 tx artifact。真实资源命中、动态换图、完整像素、GPU 同步与正常退休仍留整体 E2E；静态研究不记为运行通过。
