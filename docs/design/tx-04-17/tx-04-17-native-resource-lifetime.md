# tx-04-17 R4：原生内嵌资源、借用与在途寿命

日期：2026-11-01，Asia/Singapore。目标为官方 REPENTOGON 1.1.3 与 Repentance+ v1.9.7.12.J273。同属 tx-04-17，承接 [R2 对象生命周期](tx-04-17-native-object-lifecycle.md)与 [R3 维护／绘制](tx-04-17-native-maintenance-rendering.md)。本轮先读两套本地 Wiki、官方源码与现有工程，只对源码未解释的实际生命周期补只读静态逆向；必要逆向由用户指定的一个 GPT-6 Luna／max 子智能体执行，主智能体复读审阅。没有修改运行代码、绑定、游戏文件或 IDB，没有构建、游戏操作、测试或 artifact 包。

`PUBLIC_DOC`、`SOURCE`、`J273_STATIC`、`REUSED_J273`、`DERIVED`、`DESIGN`、`UNKNOWN`分别表示公开语义、当前本地源码、本轮目标静态事实、复用前轮静态事实、推导、设计选择及未闭合内容。静态研究不证明实际 Hook 安装、context、像素或异常恢复。地址默认 VA，`RVA = VA - 0x400000`。

## 1 本轮确定的边界

**后续R6归并（2026-11-01）**：[模型／角色／应用器合同](tx-04-17-model-role-applier-integration.md)补不可变目标／allocator、原生实例／资源表与计划／退休记录的owner和总预算，最新T不与原生A混同。Scope或新目标取消不立即释放仍被在途计划使用的资源，原生提交／待释放状态分别报告；正式Scope、input generation、资源／Lua／context代次不互相代替。资源表达／相位／特殊关系缺口反馈15及18—20，未升级成全部可写回。

**后续R5回补（2026-11-01）**：[失败退休与菜单交还](tx-04-17-failure-retirement.md)已按部分构造／apply／待释放remove／换资源／正常关闭列收口。原生Room重建本身先free旧owner后分配新对象，不能用于已失效Camera／Room的通用取消；资源清理保持有效context与真实owner，normal CPU reset和完整取消仍分开。所选native资源服务避免为本方退休表新增LuaImage wrapper，用类型化pins管理；既有LuaImage的实际GC窗口不因此变成已解决。全部采用pass取消、异常与完整菜单调用仍有UNKNOWN，下文保留R4当时事实和主审记录。

**服务器逻辑 key、原生实例、实例内部容器、ANM 资源、Image、Lua state 与 graphics context 的寿命分别管理。** Client 使用原生世界，不为资源换代另建完整 Game，也不把内嵌 ANM2 浅拷贝或裸内存 swap 当作资源提交。`change`可以保留 key 和原生实例，却使服装元素、Layer／Animation 借用及图像关联全部失效；`remove`逻辑完成也不等于允许物理释放。

`DESIGN`：原生内嵌 Sprite 由其实际 Entity／Player／Grid／Room 子系统 owner 析构。本方可以在受控应用阶段经具名接口改资源与姿态，但不能为清理借用调用它的 Reset／destructor／Lua GC，不能释放其外层存储。自有独立 Sprite 才沿用 ST5—ST7 的显式 Reset 与 Lua／native owner 策略。原生 owner 内部正常调用 Reset／destructor 与 adapter 越权调用是不同权限。

旧资源保留分成两种责任：Image 强引用保留图像；推迟原生实例／容器／ANM 资源变化保留实际仍会解引用的对象。两者不能互相替代。**本地1.1.3的 deferred Entity／Animation context 分支关闭，不列为当前活跃owner**；当前仍须处理实际在途栈、原生Image队列及采用pass的其他依赖，不能仅靠 Image pin 推定一切原生对象均可立即销毁。

## 2 公开接口与源码结果

公开语义优先使用本地 `IsaacDocs/docs/{Entity,GridEntity,Sprite}.md` 与 REPENTOGON `docs/docs/{Sprite,LayerState,AnimationData,AnimationLayer,CostumeSpriteDesc,EntityPlayer}.md`、`renderer/Image.md`。在线 [Sprite](https://repentogon.com/Sprite.html)、[LayerState](https://repentogon.com/LayerState.html)、[CostumeSpriteDesc](https://repentogon.com/CostumeSpriteDesc.html) 和 [Image](https://repentogon.com/renderer/Image.html)用于核对公开入口；网站可能滚动更新，不覆盖本地 1.1.3／J273 的版本依据。

下表源码相对于 `F:/projects_isaac/REPENTOGON-1.1.3/`；文件／函数定位用于阅读，不要求源码快照或哈希。

| 对象／入口 | `SOURCE`：实际 owner 与包装 | 采用合同及失效点（`DESIGN`／`DERIVED`） |
| --- | --- | --- |
| Entity／Grid 的 `GetSprite` | 原版公开接口返回现有 Sprite；`Entity.zhl`的 `_sprite +0x48`、`GridEntity.zhl`的 `_sprite +0x40`是内嵌 ANM2 声明。声明本身不是所有派生析构闭包 | 跟随当前池 use／原生实例；对象重用、退休或对应资源重建后重取。只保 registry wrapper 不能延寿 |
| Player body／held／blood Sprite | `LuaInterfaces/Entities/LuaEntityPlayer.cpp:1522—1532,3407—3410`用 `UserdataPtr::push`包装现有成员；`EntityPlayer.zhl`声明 body `+0x41C`、blood `+0xEE4`、held `+0xFF8` | 公开 GetBodySprite 所说的 temporary copy 是原生临时成员的用途，不是 Lua 新建独立 Sprite；Player 活着仍须区分各成员资源 revision，不把它们压成一个 Sprite |
| Player 服装描述／Sprite | `LuaCostumeSpriteDesc.cpp:5—25`遍历 `player->_costumeSpriteDescs`，将 **vector 元素地址**放进指针 userdata，再以 UserdataPtr 暴露该元素内嵌 Sprite；`CostumeSpriteDesc.zhl`声明其 Sprite 在元素 `+0` | `DERIVED`：vector 重分配、删除或重建会使元素／Sprite 借用失效，即使 Player 地址不变。按服装容器 revision 重新枚举；Lua table 只保存 wrappers，不是服装值快照 |
| RoomTransition／portrait Sprite | `LuaInterfaces/Room/LuaRoomTransition.cpp:14—18,48—80`借出 versus Sprite 或 map 内 ANM2；源码说明 StartBossIntro 重建 portrait map | 不是 Room 的通用自有 Sprite；以 transition owner、portrait 容器和资源 revision 管理。不得为了重取资源运行有副作用的 StartBossIntro |
| Layer／Animation／BlendMode | `LuaSprite.cpp:114—155,197—218,623—625`、`LuaAnimationData.cpp`、`LuaAnimationLayer.cpp`、`LuaBlendMode.h`包装内部裸地址，无父 owner 强根或代次校验 | LayerState 随层数组；AnimationData／Layer／Frame 随当前 ANM/cache；BlendMode 随 LayerState。Load／Reload／Reset／重绑前先撤销本方借用；原始 wrapper 无法主动变成安全空值 |
| Sprite／Layer 的 Image | `LuaSprite.cpp:62—83,468—490`构造 `LuaImage`并复制 smart pointer；`LayerState.zhl:15—27`的 SetSpriteSheet 赋强引用并同步 loaded/filter/wrap 字段，GetSpriteSheet 注明 lazy sync | Image 可活过来源 layer，但不保活 layer、ANM/cache 或 context。getter 可能同步 sampler，仍限已解释主线程／图形阶段；不能拿 Get 当无副作用健康检查 |
| 自有 native Image pin | `SmartPointer.zhl:10—82`提供值构造、copy 加 strong、move 清源、析构 DecrRef；并非 raw `ImageBase*`引用 | 本方退休表选用类型化 smart-pointer 值，取得／释放均在资源 owner 阶段。正常 C++ 作用域／容器销毁负责释放一次，不手工 DecrRef 后再让同一非空值析构；不伪造 control block、不 memcpy、不在 worker 析构 |
| Lua Image | `LuaRender.h:12—14`内含 smart pointer；`LuaRender.cpp:228—230,359—372`注册 `__gc`调用 DecrRef，公开方法无 Close／Release | native pin 解决本方额外强持有的可控释放，**没有**解决既有 LuaImage 的 GC 时点。unref 不等于 GC，不手调 __gc，不全局 collectgarbage 来强迫其他 finalizer |
| 官方 Entity／Grid 扩展 | `EntityPlus.cpp:71—111`在 ctor 建 holder、基 Init 换 data、dtor 删 holder；Remove 清 data 的 hook 只是注释代码。`GridEntityPlus.cpp:12—51`按 448 个 grid index 管理扩展并在 Create／Init 重置 | EntityPlus 不一定随逻辑 Remove 消失，Init 可能先使扩展数据借用失效；Grid 同 index 新对象不得继承旧扩展。保留所需官方结构维护，不手删其 holder／全局表，不把扩展字段当网络身份 |
| shader association／program | `Anm2Extras.cpp:77—92`按 ANM2* 和 layer ID 存关联，Reset／dtor 前 erase；`ShaderLoader.cpp:201—248,251—291`共享 map 持 shader，reload 可 HardReset；RenderBatch 声明有 raw shader* | extras erase 不销毁所有共享 program；普通 Load／Reload 不凭名字保证清 extras。按目标显式清／重放本对象关联，不清全局 shader cache。共享 shader reload 与在途 batch 不能并发交错；地址相同不证明 program revision 相同 |

`EntityPtr.zhl`称其 SetReference 会跟踪并在目标 remove 时清空引用；这也不是持有原生实例的强引用接口，不能用 EntityPtr、Lua registry 或 `_valid`代替“不允许池重用／析构”的实际 owner 控制。

### 2.1 J273 正常原生析构的主审事实

本轮子智能体定位、主智能体独立回读原始指令；当前 IDB 为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`、imagebase `0x400000`。只读，没有给 IDB 改类型／改名／写注释或保存。以下只证明正常调用前缀与次序，不声称所有分配、异常和派生 owner 均已解释。

| owner／入口（VA；RVA） | 主智能体回读的精确调用点 | 寿命结论 |
| --- | --- | --- |
| Entity dtor `0x6A3EF0`；`0x2A3EF0` | `0x6A3FE0 lea ecx,[edi+0x48]`，`0x6A3FE3 call 0x403790`；随后 `0x6A3FEB call 0x417EB0(this+8)` | 内嵌基 Sprite 在对象析构内直接销毁；没有由 Lua wrapper 延迟此调用 |
| Entity deleting wrapper `0x6A3EC0`；`0x2A3EC0` | `0x6A3EC6`先调上述 dtor，再检查 flags bit0；置位时 `0x6A3ED7`释放 `0x410`外层对象 | 析构、释放外层存储与 R2 的逻辑 Remove 分开；不能将此 wrapper 当所有池对象统一退休接口 |
| Player dtor `0x74F260`；`0x34F260` | `0x74F61E→0x7D2DD0(this+0x1220)`清服装vector；`0x74F629/634/63F`分别析构 `+0x110C/+0xFF8/+0xEE4`；`0x74F657`按 `7×0x114`析构 `+0x758`数组；`0x74F662/66D/678`分别析构 `+0x644/+0x530/+0x41C`；`0x74F67F`才进入基 Entity dtor。更早还有 `+0x1C50/+0x1B2C`在 `0x74F532/548`析构 | body／held／blood、服装及其它辅助 ANM 分开存在；即使基Sprite尚未析构，部分派生资源已消失，不允许 dtor 中重入绘制 |
| 服装vector销毁 `0x7D2DD0`；`0x3D2DD0` | 从 begin/end逐项 `0x7D2DE4→0x403790`，元素步长 `0x130`；之后 `0x7D2E2C`释放buffer | 与源码 `CostumeSpriteDesc`元素 `+0`内嵌 ANM 一致；服装 Lua table／Sprite wrapper 不保护 vector 元素 |
| Grid base deleting wrapper `0x705020`；`0x305020` | `0x70502F→0x403790(this+0x40)`，之后按 flags bit0 在 `0x705040`free；另非删除入口 `0x705050`在 `0x70505A`析构同一内嵌 Sprite | 内嵌 Sprite 的析构权限在 Grid owner；不是通用 Entity Factory 或 Lua GC |
| Door deleting wrapper `0x706730`；`0x306730` | 四个string清理后，`0x706768→0x403790(this+0x168)`析构额外 Sprite，`0x706776`析构基 `+0x40`，`0x706787`才按flags free | Door 是同时拥有额外／基 Sprite 的代表，不能以 GetSprite 一项覆盖完整资源；不外推所有 Grid 派生类 |
| Room dtor `0x7E0190`；`0x3E0190` | 从 `Room+0x24`扫 `0x1C0`即448槽，非空在 `0x7E01CA`调首虚槽并传1；后 `0x7E0233/253/2E4/315/320`分别析构 `+0x7770/+0x762C/+0x7100/+0x6F54/+0x6E40`，`0x7E036E`以 `0x403790`回调析构 `+0x18E0`起 `2×0x114`数组 | Grid先退休而Room仍有自己的资源；Room destructor不是单个清表，也没有因此获得所有 raw queue／光照集合安全取消保证 |

`DERIVED`：这些 owner 的正常析构会直接进入已知 ANM Reset／Image 引用清理链，所以 CPU 使用退尽和有效 context 必须在进入它们之前处理，不能先物理 Free 再靠本方借用表标旧代次无效补救。未找到统一“原生实例强持有”API；本方必须通过生命周期编排延迟真正 owner 的破坏性步骤，不能给对象随意增加计数。

### 2.2 原位 Morph 及内嵌 Sprite 到图像清理的连接

`J273_STATIC`，主审回读：NPC Morph `0x6BD0C0`（RVA `0x2BD0C0`）以 `ESI=this`在 `0x6BD114/117/11A`改 type／variant／subtype，随后 `0x6BD1D8`调 vtable `+0x64`；本目标 NPC vtable `0xB187D0+0x64`为 `0x6B7730`配置加载。配置有效路径 `0x6B77A4→0x6B6EF0`，在 `0x6B6FA7`把ECX设为同对象 `+0x48`，`0x6B6FAB→ANM2::Load 0x407AF0(path,false)`；另一特殊路径 `0x6B7084`同成员Load(path,true)，末段 `0x6B7691→LoadGraphics 0x407DA0`。这解释了**原生实例和ANM地址不换，内部资源仍会重建**；不把所有NPC Morph都说成先Reset／基Init或完整析构重生，也不抹去R2已记录的PGD副作用。

Pickup Morph `0x6DFF30`（RVA `0x2DFF30`）在 `0x6E01B9/1CF`用同一 `EDI=this`调 vtable `+4`的派生Init，之后 `0x6E0300→0x6DF890`。其Init `0x6DD7C0`中的 `0x6DDDA4/0x6DDEFF`对本体 `+0x48`调ANM2 Load；价格资源helper在 `0x6DF932`以 `ECX=this+0x410`调用Load，在无该图形路径的 `0x6DFE23`对同成员调用Reset `0x403530`。`EntityPickup.zhl`将 `+0x410`命名 `_priceANM2`。因而只重取主Sprite不能覆盖Morph后价格资源；同key／同this不保护旧层／动画借用。更多辅助ANM及整个Init副作用仍按实际adapter补齐，不放行整链。

从原生析构到图像释放的连接也已主审：ANM dtor `0x403790`在 `0x4037B6→Reset 0x403530`；Reset在 `0x4035CB`以 `0xA0`步长和 `0x4030E0`回调销毁 `ANM+0x7C`数组，之后清该字段。对照 `ANM2.zhl`的 `_layerState +0x7C`与 `LayerState.zhl`的 `0xA0`布局，这是 **LayerState数组，不是AnimationFrame数组**。元素清理在 `0x403106`取 `this+0x98`的Image smart pair，`0x40311F`调counter `+0x0C`，条件成立且callback非空时 `0x40312F`调用 `0x9EFE60`。至此连接了实际内嵌ANM的正常层释放与ST6 cache／queue callback，不能只凭ANM destructor名字推断已排尽所有引用。

`DESIGN`：Image pin只保证本体存活，**不保证原有draw记录被保留或提交**。层引用减少后仍可能进入上述计数条件下的raw queue／cache移除；所以不能把“先pin图、立刻析构旧ANM、以后再画旧队列”当完整保画面方案。采用正常消费后变化的次序，取消路径另按实际owner处理。

## 3 当前在途队列与未启用的扩展分支

### 3.1 当前队列与条件扩展

`REUSED_J273`：ST6 已核原生 ImageBase Render `0x9E2170`、raw Image 入普通队列 `0x9E2C8C`、batch `0x9E2EC0`、提交 `0x9EFA70`、普通队列 reset `0x9EFA10`。透明队列为 `(RenderBatch*, ImageBase*)`；批次、Image 及 shader 的 raw 使用不能由 Lua wrapper 自动覆盖。

**`SOURCE`，关闭分支，不能列作当前活跃依赖**：`LuaRender.cpp:30`声明 `static constexpr bool EnableCustomRendering = false`；`:3139—3141`的 Allocate hook 在super后直接返回，不生成 context 副本；`Patches/ASMPatches.cpp:272`注释了 `LuaRender::PatchglDrawElements()`，该函数内部`:3340—3343`也按同一常量限制安装。因此下面只记录条件风险，不要求当前1.1.3为这个未启用功能新增清理步骤，也不能以其未完成取消阻止当前原生路径研究。

该分支若将来启用：`LuaRender.h:166—180,208—271`的 Context variant 含各 Entity*、GridEntity_Rock*、AnimationState*、AnimationLayer*；ScopedContext 只 push／pop 当前队列。Allocate中的后续代码把队列复制到 `ElementsRenderContext`／`VerticesRenderContext`，以buffer地址为key，当前Render栈pop不清副本。当前仍存在的ScopedContext栈本身按正常RAII退场，与未执行的buffer副本代码分开。

同一未启用分支的 `LuaPreDrawElements`（`:2687—2822`）在 callback 1136 启用且 currentRenderTarget 非零时把context array交给Lua；visitor（`:2247—2330`）解引用AnimationState／Layer，正常尾部draw后清array。如果未来启用，须另解释这些延后借用及正常消费／取消，不能只关闭1136或保活Image；本轮不启用它，也不把未安装分支说成运行命中。实际安装环境与静态源码一致性仍留统一E2E。

`DESIGN`：本方的“在途”登记覆盖实际原生提交、本方 Lua／draw 活跃栈及采用的 shadow／light／HUD pass；不为已关闭的context副本分支增加虚假reader。旧实例和动画/cache容器保留到实际会解引用它们的CPU使用段退场；随后才能重建资源／容器或交owner回收。只有图像raw队列待提交、已无实例／动画使用时，Image pins才足以保留那部分图像依赖；全部采用pass的raw batch／image owner仍需核定，不能只观察一个队列归零便外推全部退尽。

### 3.2 正常消费与取消分别处理

首选把 change／remove／Morph／资源写入安排在已采用 pass 的上一个提交／消费阶段结束后、下一个 pass 开始前；使用同一主线程也要防止 Lua／loading 的嵌套 render。已经有旧代次 draw 入队时，先停止其新 admission，保留旧 owner 与资源，让仍合法的正常 draw owner 消费并清理，再执行破坏性操作。正常消费仍服从 R3 的 Lua／玩法分派，不能为了排空而放行未知整条 Game Render。

若不能继续正常消费（取消、context 丢失、部分应用、异常），必须由实际队列／batch owner明确取消关联记录，且停止其后续消费者；本轮没有证据把 `0x9EFA10`、Image cache callback、Entity Remove 或禁用回调认作这样的统一取消接口。无法解释取消的分支保持隔离、停新发布并保留必要对象，交 R5 补实际收口；不能销毁 owner 后指望下一帧清理、按墙钟延迟若干帧或在当前 callback 栈内等待自己退出。若未来启用custom-render contexts，才把其额外取消责任纳入采用范围。

### 3.3 最后 Image 引用与 context

`REUSED_J273`：DecrRef `0x4081A0`调用 counter `+0x0C→0x408500`；strong 1→0 在 `0x408536`调 image vtable `+0x60(arg=1)`，子类析构经 `0xA4C360`于 `0xA4C369/0xA4C375`调用 glDeleteTextures。另一个可选 `0x9EFE60` callback 在 strong==1 条件下到 `0x9EF7C0`做 cache／raw-container 移除；不能把它与最后析构或所有旧 revision 退尽合并。

`DESIGN`：释放本方 pins 前，已有采用路径的 raw batch／image 与实际资源使用全部退休，ImageManager／renderer 仍有效，线程、phase、资源及 renderer generation 正确，OS current HDC／HGLRC 与已知 owner 匹配。释放可能进入 GL，引用计数有锁不允许 worker 执行。CPU 引用退场与 GPU 完成分别记录；本轮不要求用 glFinish／SwapBuffers 猜成一项，也没有证明 GPU idle。context 错误时不强制切换、不执行最后释放，停资源操作并交 R5 处理关闭截止点。

## 4 失效表与重新获取

以下是本方句柄合同，尚未实现。句柄记录 scene、native instance／pool-use、成员容器、Sprite resource、Lua state 与 renderer/context 的适用代次，稳定 key 只负责重新解析对象。使用点先确认 owner／代次，再从当前对象重取；不调用旧 raw wrapper 来“检查它还活着”。

| 变化 | 立即撤销的本方使用资格 | 可保留什么／重新获取条件 |
| --- | --- | --- |
| Remove 请求／membership 退出 | 被终止 scope 的新 draw／新借用；存活的跨房 Player 按自身 owner 判断 | 物理实例／引用保留至实际退场；仅 membership 改变不伪造死亡 |
| 池 use／本地实例替换、析构 | 旧实例及其所有内嵌 Sprite／Layer／Animation／扩展句柄 | 同 key 解析新 instance generation 后重取；同地址不复活旧 wrapper |
| 原位 Morph／基 Init | 形态／扩展／资源相关句柄，具体变更可早于整个 Morph 返回 | 原生 outer owner 成功、资源及最终形态核定后重取；无变化或外围短路也按实际结果决定，不猜回滚 |
| 服装 vector 变化／portrait map 重建 | 相关容器元素及其 Sprite 的全部借用 | 外层 Player／transition 仍存活不够，需按新容器 revision 重新枚举 |
| Load／Reload／Reset／ANM cache 重绑 | 全部 ANM 内部 Layer／Animation／Frame／BlendMode、旧姿态发布资格 | Sprite 外层可能同址；按新资源重新核对层映射、main／overlay、shader 与姿态；失败亦不回用旧借用 |
| ReplaceSpritesheet／SetSpriteSheet／LoadGraphics | 被改层的图像关联及本方资源句柄；未知影响范围时撤销该 Sprite 整组缓存 | 某层数组地址可能不变，不必声称物理已析构；旧 Image pin 可延寿图像，不能使旧关联仍代表目标 |
| 纯姿态 change | 旧姿态发布／当前帧选择 cache，按所用 setter 失效 | 不虚称每次 SetFrame 都重建 ANM；重取当前 frame selector，避免动画事件消费 |
| Lua close／reset、renderer 重建 | 对应 state 的 refs／userdata，或所有依赖旧 context 的资源使用资格 | Lua state 与 context 分别换代；没有跨 state 解引用 registry ref／复用 GL handle 的捷径 |

## 5 候选、资源变化和退休顺序

`DESIGN`，对原生内嵌资源补充 ST5—ST7 的自有 Sprite 候选合同：

1. 网络／DTO 阶段仅保存有界纯值资源描述和姿态，检查目标、层映射、动画、预算与取消状态；worker 不保存任何原生／Lua／GL 指针。资源准备可用受控独立候选，但它只是准备材料，不是替代原生世界的第二套显示对象。
2. 在主线程资源窗口取得目标实例／容器／资源代次，准备必要资源和本方类型化 Image pins。取得 pins 的 getter 也遵守 context 检查；外来 Image 是否共享、实际成本和强引用来源均记录，路径相同不代表不同资源或唯一 owner。
3. 关闭受影响旧代次的新 draw／callback admission，在当前在途栈返回后的边界完成采用pass的旧CPU消费或已解释取消。原位Load／Morph／服装重建之前撤销本方旧借用，并退尽实际依赖旧ANM／容器的使用；不能先破坏再补保活。当前不要求消费未启用的custom-render context副本。
4. 经 R2 对应 owner 对内嵌 Sprite 采用具名 Load／换图／逐层绑定／姿态／shader 操作，重取内部借用；不裸交换 ANM2，不把自有 Sprite Copy 当作完整 Entity／资源原子转移接口。实例替换则增加本地 instance generation，分别处理旧 owner 退休与新 owner 资源。
5. 对完整 main／overlay／逐层资源、姿态和必要显示缓存核对后才恢复本批目标 draw 资格。公开 LoadGraphics void、loaded marker、ReplaceSpritesheet bool、非空 userdata 和 Render 返回均不代表完整成功；原生写入前缀失败时停显受影响目标，不确认 Applied，不假称旧画面可回滚。尚未改动的旧完整展示才可继续保留。
6. 旧实例由真正 owner 在依赖已退尽时摘除／析构；自有 Image pins 在 CPU raw 使用退尽且 context 有效时销毁，旧自有 Sprite 才按自身 owner Reset。原生清理间接释放 Image 也受同一窗口约束，不把它留到任意 worker／容器析构。
7. 本方 registry refs 按 state 撤销；原生 Sprite wrapper 仅撤销本方使用，不销毁 pointee。既有 LuaImage 还可能由 Lua 持 strong；必须说明 finalizer 可控窗口或其真实 renderer／ImageManager owner 保留策略，不能用 native pins 已释放冒充 Lua 引用全清。

预算同时限制候选数、待退休 owner／Image 关联数、可知的资源字节和待处理目标；静态资料未提供可信全量显存成本时明确 unknown，不报精确预算。达到上限即停止准备／发布新 revision，保留最后仍完整有效的展示或等待界面，并报告具体原因；不无限积压，不驱逐仍在途的旧资源，不丢弃协议必需状态后继续 ACK。取消只关闭新使用，已经执行的原生前缀仍须按实际 owner 退休。

## 6 关闭截止点及前后项反馈

沿用 ST6／ST7 已核顺序：Game／Manager／ANM-cache 的清理早于 Lua close；ImageManager／renderer／window context 的最终清理在其后。不能等 late Lua finalizer 才处理依赖已经消失的 ANM owner，亦不能以 early unload 回调替代实际主线程／context／在途检查或 ST8 持久整链保护。原生 Entity dtor 可能直接释放最后图像引用，故资源退休必须先于关闭对应依赖；R5须把这些截止点纳入准备失败、断连、stop、菜单交还与 Shutdown，而不是只接普通下一帧。

| 接收项 | 本轮反馈 |
| --- | --- |
| R1 | Player 的 body／held／blood／costume、Room／transition 和 HUD 资源有不同 owner；初始化应建立实际准备与清理位置，不把一个 Sprite ready 当完整场景 ready |
| R2 | 物理Free、池reuse、Init／Morph和服装变化前撤销借用、退尽实际使用；Image pin只保图像。当前custom-render buffer context副本分支关闭，不增加不存在的引用前置 |
| R3 | draw admission覆盖全部采用pass；当前Render活跃栈与Image raw提交寿命分别处理。正常消费与取消分开，不恢复gameplay／Lua来排空；若将来启用custom rendering再补buffer context责任 |
| R5 | 补全采用路径的正常消费不可达／context丢失／部分构造与取消、LuaImage迟发GC、owner关闭截止点、共享shader reload和异常重入；不为关闭的扩展功能增加任务 |
| R6、15／16 | 资源 revision、容器／实例代次、应用前缀、待退休／预算／取消结果是本地运行时责任；正式协议仍为纯值。Resource ready、draw ready、目标 Applied 与物理退休分别报告，断连不自动 Off |
| 18—20／27 | 按实际类别补完整派生 Sprite／服装／曲线／shadow／light／HUD 资源，不以本轮代表路径推成全类别已适配 |

## 7 未决问题

| 问题 | 已缩小的范围与后续责任 |
| --- | --- |
| 实际 owner／Hook／context | 静态事实不能证明当前加载的 official hooks、主线程、OS current、loading／退出路径或 GL API 成功；完整实现后的统一 E2E 观察，不在本轮预跑 |
| 取消与异常 | 当前实际raw batch／image、活跃栈与直接原生销毁路径的取消／error／longjmp／SEH／重入没有完整覆盖。R5解释具体接口及caller后态；custom-render context副本分支已关闭，其风险仅在未来启用时适用 |
| LuaImage 最后 strong | 本方 native pin 可按值控制释放；公开 LuaImage 仍没有显式 release，unref／普通 Lua return 不保证 finalizer 立即发生。不得扩大为任意 Lua Mod 借用的可撤销保证 |
| 全部类别及在途 pass | Player 服装及 Room／Grid／派生 Entity 的所有辅助 ANM、shadow batch、light 集合、HUD／transition 缓存的取消／重用仍需按实际采用路径补齐；不做全引擎逆向或将未知隐藏成通用成功 |
| 共享 shader／纹理 | extras erase、Image refcount 与共享 shader program 生命周期不同；reload／context 失效、全部 raw owner 和 GPU 状态仍需实际采用路径解释。没有建立全 renderer 销毁协议 |
| 未类型化辅助成员 | Player／Room析构中另有smart-reference形态成员及Pickup Init的辅助ANM加载路径未定完整字段语义；本轮不靠offset或伪码局部名补造全部owner。17实际代表路径及18—20／27采用到时补具体依赖 |
| 失败恢复／预算 | pins 与候选的 OOM、引用计数异常、原生部分写入、不能继续正常消费时的有界保留及安全菜单交还交 R5；本轮不声称恢复已实现 |

## 8 主智能体审阅与本轮收束

主智能体独立回读Entity／Player／Room、Grid base／Door的上述原始指令、服装vector helper、NPC配置至Load／LoadGraphics、Pickup Init／price Load／Reset及ANM层析构至Image counter／cache callback；另外直接阅读官方Lua包装、smart pointer、扩展holder、shader与custom-render开关／安装源码。接受的是这些有限正常路径和表内设计，不接受“所有对象／所有pass／所有失败已闭合”的外推。

主审纠正并据此落文：子智能体初稿将Entity末尾helper写为`0x6A3FF0`，实际call在`0x6A3FEB`；Player blood偏移是`+0xEE4`，不是`+0xEF0`，基dtor call在`0x74F67F`，不是之后的`0x74F684`。子智能体把`0xA0`元素称frame record，结合ZHL和raw应为LayerState。主智能体初步阅读custom-render的后半段时将deferred context视为当前依赖，补读`:30`的false常量、Allocate早退和注释patch入口后已纠正为关闭分支；不因保留源码就给当前路线新增虚假reader或取消任务。上述是静态审阅纠错，未发生实机失败或测试。

完成本轮统一人工文档审阅，检查引用、事实／推导／设计口径、前后项责任和失效／退休顺序；没有运行格式／链接检查器、构建、单元测试、E2E、游戏或图形实验，没有保存artifact包。历史ST5—ST7事实／失败／UNKNOWN保留，当前补充以上文为准。

本轮交付静态合同和明确未决问题，不将其记为原生 Client 世界、全类别资源适配或 tx-04-17 功能完成。整体验收仍在整个目标实现后统一进行。
