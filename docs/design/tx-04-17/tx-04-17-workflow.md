# tx-04-17 原生客户端世界与状态应用公共运行时

## 2026-11-02 公共骨架及 I6／I7 实施状态

本次推进在 `mod/src/game/client_scene.h/.cpp` 增加独立 Client 场景 owner：完整等待／目标／已应用 Scope、T 与 A 分别持有、单调 attempt、实际执行前缀和失败结果、主线程用途借用及有界在途表。现有 16 的网络 worker 仍无世界 payload；T 接入复用 15 的模型校验并拥有副本，但原生 adapter 尚未实现，返回 `UnsupportedRequirement`，A 为空，`native_committed=false`，不得产生 Applied ACK。该骨架不改变 Host 的统一 scheduler 或 Client 的输入语义。

I6 已接 `ClientManagerBoundary` 的 Lua state 身份观察、`LuaEngine::destroy` 的关闭前换代、`Manager::Render` 的 renderer-window HDC/HGLRC 与 OS-current 比较，以及 `Isaac::Shutdown` 返回后的 context 使用撤销。`ClientRenderScope` 的 `RenderObservation` 借用跨过完整 `Manager::Render` 原调用并在正常／异常展开时释放，只登记在途，不授予 Client 原生绘制资格。借用票据由有界唯一 ID、scene／instance／resource／Lua／context 代次与当前 owner token 共同约束；外线程只原子通知主线程处理。当前没有 Client 原生场景和资源 adapter，因此没有图形、Lua 或对象借用被实际用于原生世界；具体对象／容器精细换代及 raw batch／Image/LuaImage 退休仍待 I2／I5／I8，不能把此处代次与 `luaL_unref`、Render 返回或 SwapBuffers 视为析构／GPU idle。

`ClientRenderScope` 在原生异常展开时先将场景标为失败，再释放其在途票据；该失败由下一次 Manager 边界关闭 Client 输入和网络。

I7 已在统一清单为 Client 设独立能力组，已核对的 `Game::Exit`、Start／Restore、PGD 解锁／计数／本地与云保存、GameState 读写与删除、菜单 Continue 读取均在 Client 分支先于原生与官方外层 Hook 主动拒绝；Host／Off 及明确的 button-map 本地设置仍走原调用。观察 `PersistenceScope` 与主动分派分开。进程退出桥现独立于 world 录制能力，并要求网络 worker 与场景无执行前缀／借用才放行。当前 Client 路径没有服务器原生写入；在此前提下，Shutdown 的直接 PGD 写与随后 Manager 析构同步仍属于访客原有本地生命周期，可以正常透传。I1/I3/I5 尚未启用原生世界，故 **不得** 将这些 wrapper 视为未来服务器世界的完整持久隔离：Shutdown `0x982D44` 直接写、REPENTOGON achievements／challenge／completion JSON、ESSM／vanilla Mod SaveData、rerun 其它入口及最后 writer 的全部调用链仍未静态闭合。任何 Client 原生写入必须先补齐所采用路径的这些阻断与 I8 退休，不能靠下层 `write=false`、readonly、dirty 终值或延迟 Lua 回调宣称安全。

上述 I7 分派还覆盖已核 Lua `PostGameStart` 与 `PostRender` 入口；它们不代表全部动态 gameplay／卸载 callback 已受控。

本段只报告代码接线与静态结论；尚未构建、运行生成器／校验器、测试或启动游戏。以下 2026-11-01 文本保持当时的设计研究事实，不能读作本次代码运行结果。

修订日期：2026-11-01，Asia/Singapore。状态：**R1—R5已形成当轮原生生命周期静态合同，R6本轮已对照15／16源码归并模型、角色、应用器与后续接口；R1—R6指定范围的文档结果均已形成。完整可调用初始化／菜单编排、逐类应用、全部采用pass取消及异常恢复仍有具体UNKNOWN，模型／网络／Scope／事件结果的新增接线尚未实现。原生Client世界、公共应用运行时及统一E2E尚未完成。** 本轮仅更新设计文档，未修改代码、构建或运行测试。

2026-10-31 的 **ST1—ST8 指定范围静态研究与主审完成**事实保持，成果见[接口目录](tx-04-17-public-interface-catalog.md)、[场景根与生命周期](tx-04-17-client-scene-lifecycle.md)、[更新隔离](tx-04-17-update-isolation.md)、[展示边界](tx-04-17-render-presentation-boundary.md)、[资源与完整姿态](tx-04-17-resource-pose-runtime.md)、[线程／context／在途绘制](tx-04-17-thread-context-lifetime.md)、[Lua与借用寿命](tx-04-17-lua-borrow-lifetime.md)、[访客持久副作用与退出隔离](tx-04-17-persistence-exit-isolation.md)。这些研究解释了原生副作用，但没有实现本轮选定的受控复用路径。第4节及各专题保留历史事实、主审纠错与 UNKNOWN；其中一概禁止复用 Start／Room setup、只允许自有呈现对象写入、另建独立场景等设计结论，按本轮第1、5—8节替代，不追溯改写静态证据或宣称运行通过。

本tx为真实游戏Client建立不执行权威玩法的原生世界运行环境，并交付 create／remove／change、资源、展示维护与安全退休的公共合同。复用15的纯值模型／协议与16的真实Client连接／输入接线；实现中发现具体表达缺口再反馈相邻任务，不重做已有协议，不把某个原生函数调用等同于网络命令。

执行规范见[现行项目规范](../../refactor/project-conventions.md)，完整范围见[09—34路线图](../state-synchronization-roadmap.md)，技术模型见[状态同步设计](../state-synchronization.md)。本文是tx-04-17的专项工作入口，不改变其他tx的范围与编号。

## 1 完整目标与责任范围

目标环境为官方REPENTOGON 1.1.3与Repentance+ v1.9.7.12.J273。Host继续作为唯一Authority，Client只上行输入意图并展示Authority裁决的结果；Off保持原生行为。C/S不进入或复用原版Online、Isaac Sync及其可变状态。

本tx须交付：

1. 复用正常启动已有的Manager／Game，在其原生世界中构建有效Room、Camera、实体容器、Player及HUD上下文；不为同步另建并行Game或另一套运行中的复制品场景。原生生命周期确需替换根对象时，仍属同一场景的受控退休／重建，必须更新本地代次和关联。
2. 保留create／remove／change的存在性、最终值和关系语义，提供原生身份映射、应用上下文、依赖处理、提交与失败恢复公共机制。
3. 分开管理原生构建／状态应用、持续展示维护和玩法模拟，覆盖普通、odd、FSM、loading、render及外层处理；AI、伤害、拾取、掉落、物品效果和随机结果仍由Authority裁决。
4. 优先复用原生资源与绘制，维护必要缓存、排序、相机、HUD和姿态；落实动态换图、context、内嵌Sprite、Lua借用与在途对象的寿命。
5. 完成准备、失败、重建、停止和正常退休；半套状态不发布、不确认、不被本地玩法接管。客户端服务器世界不得影响访客原版run、Continue、统计、成就、解锁和保存，允许的本地设置按来源与字段管理。

| 相邻任务 | 与17的责任关系 |
| --- | --- |
| 09—14 | 提供输入、完整观测与实际缺口；先行静态研究不提前改造其采集，也不宣称观测已经成为复制协议 |
| 15 | 负责复制模型、稳定身份、资源描述、状态与事件及协议合同；17反馈展示实际所需的数据和失败语义 |
| 16 | 复用真实连接、统一输入和独立主线程泵；稳定菜单仍是连接入口。17扩展原生场景状态与退休条件，不能沿用“Client全程无场景／必须MenuReady”判断整个生命周期 |
| 18—20 | 负责完整Room／Grid、非Player Entity及Player的逐类创建／移除／变化、字段、引用与特殊缓存；17提供公共机制和足以覆盖它的原生场景／代表性操作，不提前承担全部类别适配或宣称全域应用完成 |
| 21—22 | 负责实时Baseline闭环、Delta、Applied ACK与每Peer恢复；17提供准备／提交／失败结果，不因资源已加载就声称整个世界已应用 |
| 23—29 | 扩展整局场景、完整HUD音画、声明Mod与全内容差异；新增适配必须沿用17的隔离约束，发现公共边界缺陷须修正，不能在17留下已知必需路径后统称交给29 |
| 31 | 负责房主合法保存／Continue及各端进度归属的完整功能；17先防止访客本地发生服务器世界的持久副作用 |

一个屏幕Sprite只能服务等待UI或明确的辅助表现，不能代替原生世界。17的代表性场景应实际经过原生构造、create／remove／change、绘制和退休，并覆盖本tx采用的公共路径；18—20继续完成逐类应用，21—22负责完整实时同步闭环，不能把这些后续完成条件倒灌为17的研究前提，也不能将公共机制缺口推迟到全内容阶段。

### 1.1 原生世界与纯值模型

Client原生世界是Authority状态在本地的实体化结果。服务器决定对象存在性、玩法状态与关系；客户端负责原生对象构建、状态应用、必要维护、展示和退休。网络DTO、不可变基线、目标模型和显示插值值仍由本项目拥有，它们不是另一套运行中的游戏世界。资源候选和短暂待发布对象可以按原生生命周期存在，但不并行构造完整第二世界来假设原子换场。

原生指针只作为主线程、当前实例代次内的关联。网络key、逻辑对象、原生实例、房间membership、资源版本和Lua／context代次分别管理；地址复用不复活旧key。Room替换、跨房持久对象与Player多身体按各自owner处理，不要求所有对象与Room同生同灭。

### 1.2 create／remove／change合同

| 操作 | 保留的状态语义 | 原生执行与成功含义 |
| --- | --- | --- |
| create | 目标集合出现服务器逻辑对象；不要求客户端经历服务器出生历史 | 通过对应原生工厂／构造及必要Init建立有效对象，绑定稳定key，应用初值与引用；完整可用后才纳入已提交展示。重复目标不能再次生成 |
| remove | 明确终止对象存在或撤销声明的scope成员关系；部分枚举缺席不代表删除 | 按域语义先处理引用、显示及成员关系，再由原生owner摘除／退休；不以Kill、伤害、掉落或清房奖励代替移除。逻辑摘除与延迟释放分别记录，安全退休未完成不能交还Off |
| change | 同一逻辑对象的字段、关系、资源、姿态或形态变为指定最终值；对应旧文档Update | 优先原位应用并刷新必要缓存。Morph保持逻辑key、增加shape revision；若需替换本地实例，增加本地实例代次并重绑引用，对外仍为change，不伪造服务器remove＋create |

同一版本重复、旧目标和冲突分别处理，不重复生成、移除或播放事件。跨房membership退出不必然是对象死亡；remove请求、摘表、不可见和物理析构不可混为同一时刻。显式空值／清空与字段缺失保持区别，不以零值猜补未知状态。

原生工厂连带创建的对象必须分类：属于复制模型的实体要与对应服务器key准确关联或在生成入口受控处理，不能随后再次create；引擎内部的显示辅助对象随其owner管理，不擅自获得网络身份。跟班、弹幕及其他玩法实体不能仅为方便而归为显示辅助物。未知连带生成不能先运行后批量删除来冒充无副作用。

### 1.3 构建、维护与玩法分派

复用原生初始化链建立引擎要求的有效结构，允许已解释的临时对象、缓存与构造写入。Start／Room setup不再一概禁止，也不直接整体放行；R1必须选出具体复用范围、服务器参数注入位置、禁止路径与失败出口。不能加载访客Continue借作世界，不能凭同seed生成替代服务器状态，也不能先正常开一局再删除结果来冒充隔离。

| 用途 | 允许行为 | 必须限制 |
| --- | --- | --- |
| 构建／状态应用 | 原生构造、资源加载、最终值写入、关系建立、必要缓存重建、成员摘除 | 构建前已启用持久与玩法隔离；不触发第二次奖励、掉落、物品效果或自主模拟；临时内容不成为同步真相 |
| 展示维护 | 原生排序、显示坐标、受控相机／HUD／动画、绘制和去重音画事件 | 显示时钟不推进Authority tick；动画事件不驱动开火／掉落消费者；原生与网络表现不能双播；不通过整条玩法Update猜出缺失的显示状态 |
| 玩法模拟 | Host原有Authority路径 | Client不自行裁决AI、伤害／碰撞结果、拾取、物品、随机奖励、清房或关卡进度；输入仍上行到统一scheduler |

是否允许某次本地写入取决于归属和语义，不以“写了Game／Entity”或“函数名是Render”直接判定。必要的本地初始化／表现随机计算须解释其状态归属和后续消费者，不得替代服务器随机结果、推进后续本地玩法或触及访客进度；不将所有RNG调用都当作同一随机流。混合路径按具体调用点分派，不以Client标记或单个回调本身证明整链隔离。网络线程不持有原生对象，所有原生／Lua／图形操作仍在已解释主线程阶段进行。

### 1.4 目标重建与原生应用

```text
Baseline直接恢复T；Delta(B,T)从不可变B重建完整目标T
    → 对照最后成功应用状态A形成create／remove／change计划
    → 主线程原生应用、关系修复和必要缓存维护
    → 发布T的展示资格与原生提交结果 → 21／22生成Applied ACK
```

15当前Delta采用component最终值replace／remove编码；上述三类是对象应用语义，不新增wire opcode，不把component.remove误当原生实体Remove。若A的HP=7、T恢复为B的HP=10，即使Delta省略HP，应用计划仍须把原生HP恢复到10。A用于状态规划，不代替本地实例／引用有效性检查，也不把本地插值或临时显示写入采纳为权威值。

先在自有值中核对scope、目标版本、成员、字段与依赖，尽量准备资源；再在主线程执行有预算的原生计划。通常先建立对象映射、应用字段再修复引用与成员；Grid槽位冲突、Player替换等按真实依赖调整顺序。不得渲染部分应用的目标；跨帧准备期间仅允许等待UI或仍完整有效的已提交展示，不能运行玩法来填补缺口。循环引用、暂缺目标、取消、资源失败及场景换代有明确处理。

只有所需状态、引用、资源与展示维护完成且当前代次仍一致，才提交A=T及对应结果。收包、DTO Stage、原生逻辑提交、事件水位和物理回收分别报告；不能因资源加载或下层函数返回就发送Applied ACK，也不伪造GPU完成。事件按依赖与实际消费更新水位。

### 1.5 失败、恢复与角色寿命

生命周期为稳定菜单连接／等待 → 准备原生场景 → 应用目标 → 展示；后续目标进入应用阶段，失败进入恢复或退休。名称为设计职责，不表示当前枚举已实现。准备／应用／展示／恢复／退休／失败都保持Client角色，socket断开不自动变Off。

- 尚未改动原生世界时失败：取消候选，保留仍有效的已提交展示；场景代次已失效时不能继续显示旧房。
- 已部分改动时失败：停显受影响世界，不确认目标，不假设可以回滚至A；保持隔离，按已解释owner清理并用完整目标／新Baseline重建。同一原生场景重建不需要第二套完整世界。
- 不清楚恢复／回收边界时：停止相关新使用，明确错误和剩余对象，不重复执行已部分完成的计划或假称清理完成。网络与内存缓存有界，慢Peer不阻塞Authority。
- 停止／正常退出：关闭输入和新publication，清空旧边沿，停止新对象使用，在已知边界处理在途回调／绘制和原生退休，保留进度隔离至最后相关owner结束；安全交还菜单后才Off。不使用未经适配的Exit(false)取消，不阻塞回调等待自身退出。

统一输入、Hook常驻、Host／Off行为与C/S隔离不变。等待期复用16的既有输入语义；世界应用、恢复与退休期间的输入资格按明确scope／状态管理，不恢复旧边沿，不以本地模拟提供输入捷径。

## 2 当前基础与依赖安排

现有[Hook清单](../../../mod/binding/hooks.json)及[主线程与owner管理](../../../mod/src/hooks/context.cpp)可复用。15已交付[纯值模型／协议合同](../tx-04-15.md)，16已交付[真实Client连接与输入](../tx-04-16.md)的当前范围；16的双机结果与四端暂缓仍按原记录，不扩成17通过。[client.cpp](../../../mod/src/game/client.cpp)的独立ClientRuntime已有Off／Connecting／AwaitingWorld／Retiring／Failed，在Manager原链前处理消息与输入并短路原链；其退休依赖无世界资源和MenuReady。它与Authority的SessionMode分开，不能再仅凭SessionMode只有None／Solo／Host就声称没有Client。17需扩展原生场景、应用与退休接线；现有输入消息泵、网络队列和观察Hook不等于这些能力已完成。

03-10—20主要是J460上的静态研究。其行为分析、失败反例和候选设计可以复用；实际需要的RVA、ABI、对象偏移、槽位及调用关系须在J273上核对。已经由04-05及04-08在J273解释的部分优先复用，不从旧地址作机械平移，也不为所有公开Lua成员重复做native对照。

| 工作 | 能否先行 | 与15／16的实际关系 |
| --- | --- | --- |
| 本地Wiki、公开接口、官方源码阅读及旧研究归并 | 可以 | 不依赖最终wire或Client实现 |
| J273静态反编译、调用点／ABI及副作用核验 | 可以 | 先解释引擎事实，结果反馈15／16 |
| Client最小场景、资源候选、借用和退休合同设计 | 可以 | 可先使用职责和语义描述；具体身份、消息及生命周期接线随15／16落实 |
| 正式角色控制、输入与消息接线、资源发布实现 | 按实际依赖接入 | 需要15的模型和16的真实Client入口，不另起旁路或第二套协议 |
| 实际命中、线程／context、资源显示及副作用判断 | 留给17全目标统一E2E | 不在静态核验期间运行游戏、probe、试验、校验器或分项测试 |

初始请求只建立工作流程；2026-10-31后续用户启动ST1，随后要求继续ST2／ST3，各由一个子智能体推进并来回核对，必要逆向使用GPT-6 Luna／max，主智能体审阅。ST1通过文档和源码归并完成；ST2／ST3已分别由指定模型完成只读J273研究，并交换初步事实、交叉回读草稿和纠错，主智能体独立重读关键指令与源码。随后用户根据ST2／ST3启动ST4，明确每个ST各用一个GPT-6 Luna／max子智能体，由主智能体审阅后直接提交；该轮只分派ST4，未重新执行ST2／ST3。用户随后要求继续ST5／ST6／ST7，每项各开一个子智能体并来回核对，必要逆向指定GPT-6 Luna／max；本轮三项均由各自指定模型完成，交换初步事实、互读草稿、按主审及互核意见修订，主智能体另行回读源码与J273关键指令。未启动实现或实机操作。先行静态核验属于同一tx-04-17，不新增仅研究的tx，也不恢复gate、哈希、预封或项目层强制独立角色流程。

## 3 可复用研究和公开接口

| 主题 | 既有研究入口 | 优先使用的公开能力与补充范围 |
| --- | --- | --- |
| 更新与玩法边界 | [03-10分派边界](../../research/tx-03-10-replica-update-presentation-boundary.md)、[04-05目标版本核验](../../research/tx-04-05-static-confirmation.md)、[04-08 ST2](../../research/tx-04-08-st2-static-confirmation.md) | `MC_PRE_UPDATE`、各类`MC_PRE_*_UPDATE`及碰撞／伤害回调是候选控制点；核验其跳过范围与剩余操作 |
| 动画事件与派生绘制 | [03-11逻辑与展示分离](../../research/tx-03-11-logic-presentation-separation.md)、[03-12派生render](../../research/tx-03-12-derived-render-sprite-lifecycle.md) | pre／post render、受控Sprite绘制和姿态设置；只追拟保留路径中的玩法消费者和未知副作用 |
| 资源与动态换图 | [03-13图像资源](../../research/tx-03-13-sprite-image-resources.md)、[03-15换图与Reload](../../research/tx-03-15-sprite-replacement-reload.md) | `Sprite()`、`Load`、`LoadGraphics`、`ReplaceSpritesheet`、资源查询；失败后态、共享资源和具体特殊需求才补源码／逆向 |
| 姿态与候选发布 | [03-16展示字段](../../research/tx-03-16-independent-sprite-display-fields.md)、[03-17 owner寿命](../../research/tx-03-17-independent-sprite-owner-lifecycle.md)、[03-18适配合同](../../research/tx-03-18-single-sprite-adapter-contract.md)、[03-20资源姿态闭环](../../research/tx-03-20-sprite-resource-pose-closure.md) | `SetFrame`、`SetLayerFrame`、`SetOverlayLayerFrame`、`LayerState`；检查分数相位、逐层姿态和特殊绘制的具体表达缺口 |
| context与批次寿命 | [03-14 renderer寿命](../../research/tx-03-14-renderer-context-lifecycle.md)、[03-19绘制窗口](../../research/tx-03-19-sprite-render-entry-window.md) | 正常render回调与原生owner；核对J273加载旁路、在途绘制和退出，不重新展开整个GPU后端 |
| 生命周期与持久入口 | [04-08 ST6—ST8](../../research/tx-04-08-st6-st8-static-confirmation.md)、[现有持久观察](../../../mod/src/game/persistence.cpp) | 复用已解释的Start／Restore／Exit、解锁、计数和保存入口；观察能力不等于Client下已经阻断副作用 |

本地接口入口：[原版Sprite](F:/projects_isaac/IsaacDocs/docs/Sprite.md)、[原版回调](F:/projects_isaac/IsaacDocs/docs/enums/ModCallbacks.md)、[REPENTOGON Sprite](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)、[LayerState](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)、[AnimationFrame](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationFrame.md)、[REPENTOGON回调](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/enums/ModCallbacks.md)。

## 4 既有ST1—ST8静态研究与历史主审记录

以下保留2026-10-31各轮的任务、静态结果、当时源码状态与主审原记录；其中“当前／下一步／不能调用”等需结合原日期阅读，冲突设计由2026-11-01第1、5—8节替代。ST完成不等于本轮R任务完成；历史模型和子智能体分工不成为新的执行要求。事实引用可复用，未决项按新路线判断实际需要后补充，不重新执行整套ST。

ST1—ST8和R1—R6均为同一tx的内部索引，不是独立推进编号或分项验收。每条结论区分公开接口、源码、目标二进制静态事实、推导和UNKNOWN；实际线程、像素、调用次数及运行可达性不得由静态资料伪造，不要求源码快照、日志包或独立artifact。

### ST1 公开接口和既有事实归并

- **2026-10-31已完成资料归并和源码审阅**：[接口目录与具体缺口](tx-04-17-public-interface-catalog.md)登记参数、返回、阶段、owner、失败语义和15／16反馈；J273复用与J460索引分列。当前源码确认构造器第二返回值仅检查层数、换图bool不汇总图像加载、当前层帧查询有clamp和内部对象借用。公开接口优先；只有后续具体不足才补J273，不沿用旧raw key写回方案。未新增逆向、运行代码或测试；ST1完成不等于17功能完成。
- 资料：第3节全部研究、两套本地Wiki、当前Hook清单及与具体疑问有关的REPENTOGON源码。
- 核验：将17必需操作分别归为公开接口可直接满足、需查具体源码、需补J273 native核验；记录版本、参数、返回、适用阶段、对象owner和失败语义。Wiki语义明确的成员不强制再做二进制对照。
- 已知限制：官方[`handle_skipped_update`](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ASMPatches/ASMCallbacks.cpp)仍提交移除、排序并调用HUD后处理；[`HOOK_PRE_UPDATE_CALLBACK`](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/CustomCallbacks.cpp)在跳过1160—1168派生Update后仍调用`Entity::Original_Update()`；`MC_PRE_GAME_EXIT`不是保存否决回调。
- 需写出的结论：接口选型与具体缺口表，清楚区分可复用J273事实和仅可作索引的J460结论。不能把上述回调单独当作完整隔离方案。
- 运行时留项：当前安装环境实际启用的链、命中和行为由统一E2E确认。

### ST2 Client最小场景与启动环境

- **2026-10-31指定静态研究、ST3交叉核对与主审完成**：[场景根与生命周期](tx-04-17-client-scene-lifecycle.md)。正常引擎初始化已构造Game和基础Room，但Camera尚空；实际Camera构造位于房间setup链，依赖有效几何。额外Game构造、Start／Continue及Exit(false)均不能视为无副作用的场景准备／取消接口。菜单1023回调可作为自有Sprite等待UI候选，完整Replica世界壳的构造／发布／退休仍是明确实现缺口，不以菜单展示替代。
- 资料：现有Start／Restore／Game::Exit、Game／Room／HUD生命周期研究和公开构造／生命周期入口。
- 核验：列出窗口、相机、Room、实体列表、HUD、资源及Lua渲染所需根对象和初始化顺序；标明哪些初始化同时生成房间、推进玩法随机、执行玩法回调或触及原版run／进度。
- 补充逆向：只追公开接口／源码未解释且实际需要的初始化和退出路径，确定建立展示环境的具体方式；不能以客户端重开同seed游戏代替准备Replica场景。
- 需写出的结论：最小场景依赖、准备／取消／销毁顺序及不能调用的玩法路径，并反馈16连接等待到资源准备的角色切换条件。
- 运行时留项：无本地权威run时实际能否初始化、进入加载／暂停和正常退出；结论未知时不声称已存在安全场景壳。

### ST3 普通 odd FSM loading和外层更新隔离

- **2026-10-31指定静态研究、ST2交叉核对与主审完成**：[更新隔离](tx-04-17-update-isolation.md)。补齐Manager前置Start／菜单、条件早退、staged位置写回、ordinary／odd／RoomTransition FSM、外层相机共享RNG及Lua尾部；区分Room代码caller与Lua注册数据引用。候选是在已知Hook链外层按Client角色隔离Manager玩法链，另接主线程消息／输入／退休和展示适配；当前wrapper仍无条件super，未实现或证明全路径运行隔离。
- 资料：03-10、04-05、04-08 ST2及当前Manager／Game／Room／odd／生命周期Hook。
- 核验：在J273连接普通Game→Room→尾部、odd helper、特殊FSM、loading、外层位置恢复和后处理；记录重入、其他已知入口、早退及异常返回。
- 补充逆向：针对实际保留路径确认AI、碰撞、伤害、拾取、物品使用、掉落、玩法RNG及Lua的执行位置；判断拟跳过位置之后是否仍有玩法写入。不能仅停输入、普通Room或单个Player consumer。
- 需写出的结论：按真实路径列出保留／跳过／展示适配位置、调用链与必要ABI，以及仍未解释的可达副作用。已由J273研究解释的入口不重复全量逆向。
- 运行时留项：各分支实际发生、隔离期间无本地裁决，以及Host／Off行为不受影响。

### ST4 render HUD动画事件与Lua副作用

- **2026-10-31指定静态研究及主审完成**：[展示边界、事件归属与适配合同](tx-04-17-render-presentation-boundary.md)。由一个GPT-6 Luna／max子智能体推进，主智能体独立回读J273及源码、补证并审阅。已核对SetFrame置事件位、Shoot消费者经共享RNG到Type=9／Spawn、三类派生render及Game／HUD的Lua分派、HUD声音、插值共享写入与特殊shader时间依赖。采用Authority展示状态与表现事件驱动的具名适配；不原样放开Game／Room／HUD／派生render树，不以单Sprite缩小完整内容。实际隔离、世界适配和统一E2E尚未完成。
- 资料：03-11／12的事件消费者和派生render、REPENTOGON render回调、HUD及动画相关源码。
- 核验：对拟保留的render／HUD／相机／动画路径，区分本地展示缓存写入、权威世界写入和Lua分派；检查动画事件是否仍能触发Spawn、物品、掉落或玩法随机。
- 补充逆向：追未被公开语义或源码解释的必要helper和动态调用。保持“渲染需要Lua”与“允许执行任意玩法Lua”分开，声明的presentation逻辑须有明确入口与owner。
- 需写出的结论：允许执行的展示路径、须隔离的回调／消费者、事件触发归属，以及为后续18—20／27保留的适配接口；不把独立Sprite当作完整Player或所有派生render。
- 运行时留项：正常／暂停／加载时的持续展示，动画事件不产生第二份战斗或奖励，网络表现不与原生重复触发。

### ST5 资源准备 动态换图与完整姿态

- **2026-10-31静态研究、ST6／ST7互核及主审完成**：[资源准备与完整姿态合同](tx-04-17-resource-pose-runtime.md)。区分ANM、图像关联、姿态、发布及实际像素；核对J273 Load／Reload／换图失败后态、帧setter映射与事件位。补齐BlendMode、Sprite／逐层custom与champion shader、三个main SetFrame入口；分数phase／无有效帧需求保留具体缺口。候选始终私有至完整发布，逻辑切换与旧引用退休分开，不以布尔值或getter的clamp结果冒充完整成功。
- 资料：03-13、15—18、20，公开Sprite／LayerState／AnimationData／AnimationFrame及相关绑定源码。
- 核验：构造和加载结果、ANM及图像的成功语义、换图和Reload区别、main／overlay及独立层姿态、颜色／变换／flags；明确缺失资源、无效动画、层越界和准备部分失败的后态。
- 补充逆向：仅对具体不足补J273核验，例如公开整数帧无法表达的必要相位、特殊几何依赖或不清楚的setter副作用。不以旧原地写key向量方案要求所有资源重新走native内存适配。
- 需写出的结论：资源描述和姿态所需语义、公开接口调用顺序、候选准备／检查／发布／退休合同，以及反馈15的字段或错误缺口。不能把`IsLoaded`或构造成功合并成GPU／像素正确的保证。
- 运行时留项：真实资源命中、动态换图、main／overlay与逐层姿态、失败候选不发布及可见结果。

### ST6 主线程 context加载旁路与在途绘制

- **2026-10-31静态研究、ST5／ST7互核及主审完成**：[线程／context与引用寿命](tx-04-17-thread-context-lifetime.md)。核对Manager正常与启动加载双caller、J273 WGL创建／切换／删除和TLS无返回值检查，补齐raw image／batch、正常CPU队列reset／remove与最后strong到GL删除的分别路径。旧J460批次强引用和window布局不移植；OS current、实际安装、全部异常／在途owner和GPU完成仍未知。正常退出按Game／ANM cache、Lua、ImageManager／context分别退休。
- 资料：03-13／14／19，J273 Manager::Render及初始化／Shutdown链，现有主线程记录和owner栈。
- 核验：资源加载、图像引用变化、绘制提交及最后释放各自要求的线程和context；确认正常render、启动／加载旁路以及退出是否经过同一接入位置。
- 补充逆向：仅补当前版本中必要且未解释的入口、context切换、共享绘制批次和退出顺序。不因处于render回调或TLS有值，就推断任意时刻OS current context均有效。
- 需写出的结论：允许资源操作的实际阶段、运行时应检查的条件、在途引用的持有和撤销顺序、context销毁前的清理位置。后台只处理有界自有值，不操作游戏对象、Lua或GL资源。
- 运行时留项：实际线程／context、加载旁路命中、在途绘制与停止竞合；静态结论不替代运行观察。

### ST7 Lua与借用对象寿命

- **2026-10-31静态研究、ST5／ST6互核及主审完成**：[Lua、Sprite与借用寿命](tx-04-17-lua-borrow-lifetime.md)。区分自有UserdataValue与原生UserdataPtr、Layer／Animation／BlendMode裸借用及Image强引用；核对Reset／原生析构／GC／外层Lua存储和Shutdown顺序。registry只保对应Lua值，unref不立即GC；显式Reset只限本方自有Sprite。LuaImage缺可控释放公开接口、关闭／异常／重入和旧代次raw引用退尽仍是具体实现或运行留项。
- 资料：03-15／17的owner及GC研究、[`LuaSprite.cpp`](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/LuaSprite.cpp)、当前Lua桥与生命周期管理。
- 核验：自有Sprite、原生对象、Lua userdata、内部Layer／Animation借用、Lua registry引用的分别归属；哪些Load／Reload／资源替换／场景退休／lua_close操作会撤销借用。
- 补充逆向：只对公开接口和源码未解释的有效期、析构／外层存储释放及最后图像释放补必要点位，不把原生析构、userdata GC和内存释放当成同一个delete。
- 需写出的结论：持有关系、资源或代次变化后的重新获取规则、回调在途处理及释放次序。禁止把userdata或原生Player指针带到网络线程或跨进程；不浅拷贝Sprite内存。
- 运行时留项：重建、失效、GC及正常关闭的实际顺序，旧代次和旧借用不再被使用。

### ST8 访客持久副作用与正常退出

- **2026-10-31指定静态研究与主审完成**：[访客持久副作用与退出隔离](tx-04-17-persistence-exit-isolation.md)。必要逆向由一个GPT-6 Luna／max子智能体完成，主智能体独立回读关键J273指令和官方源码。确认Exit(false)先有条件保存PGD、后可条件删除run；Shutdown更早含直接counter[192]清零、IncreaseEventCounter(522,1)及本地保存，另有备份／云或本地保存分支，均不能靠early unload或readonly兜底。官方成就／挑战／完成标记、GameState附加文件及Lua Mod保存分别处理；允许设置按本地来源和字段定义。Client准备前至安全退休后保持必要隔离，不因失败／断连／stop提前Off；当前只有观测而无Client阻断实现，实际调用和进度归属仍待完整实现与统一E2E。
- 资料：已有J273的TryUnlock、IncreaseEventCounter、SaveLocally／SaveToSteamCloud、GameState读写／删除、Game::Exit及Shutdown研究和当前观察实现。
- 核验：从Client准备、加载、render／HUD到stop、返回菜单及进程退出的实际可达路径；区分内存进度修改、run保存、文件写入、本地设置和Lua Mod保存。
- 补充逆向：确认尚未解释且确实可达的进度／写入调用和返回合同；复用已有入口，不为整套存档系统无差别重做逆向。阻断文件写入不能代替阻断不该发生的内存进度修改。
- 需写出的结论：Client角色下应抑制的裁决／进度路径、允许的本地写入、正常退出和剩余资源退休次序；失败或stop后保持必要隔离，直到Client场景安全退休，避免提前Off后原模拟接管残留对象。
- 运行时留项：整个操作窗口的实际调用与状态变化、访客进度归属及正常清理；最终文件未变或dirty=0不能单独证明全过程无副作用。

### 静态结果的记录与推进顺序

当前**ST1—ST8指定静态研究及主审完成，ST5—ST7另经三方来回互核**。后续随15／16落实完整实现，沿用允许／隔离／适配责任及ST8持久归属合同。世界壳、可控LuaImage释放、实际context、在途退休与持久隔离尚未实现；已知不能安全复用的路径不能留待实机碰运气。这里是工作依赖，不另设分项验收；既有ST5／ST6／ST7分工与互核、本轮ST8一个GPT-6 Luna／max逆向子智能体及主审均来自用户明确指定。

每项执行后在对应条目续记：实际范围、可读源码文件／函数位置、必要的J273 RVA／ABI／字节或布局、结论类型、拟采用方式、具体UNKNOWN和反馈任务。不要求每项单独报告包；不计算或校验任何哈希，不将静态结论登记成运行PASS。静态核验过程中不启动游戏或调试实机，不安装Hook、不做注入或运行时补丁，不执行功能／语法／离线校验脚本。

对实际准备调用的未知ABI、错误线程、无效对象或不确定恢复路径，停止该处修改并补充具体研究；这属于功能边界，不能扩展为全映像闭包或所有字段核验的推进门槛。

### ST2／ST3交叉核对与主审记录（2026-10-31）

两项各由GPT-6 Luna／max子智能体负责；第一轮交换场景依赖、Manager阻断和取消／退休条件，第二轮互读研究文档并修正caller、早退与根对象归属。主智能体另行读取J273指令及官方源码，不仅依据子智能体摘要接受结论。地址以下均为VA，imagebase为`0x400000`。

- **场景与启动**：主审重读`Manager::Init 0x92BA20`、`Game ctor 0x6ED4B0`、Room重建`0x732D20`与`ExecuteStartGame 0x932A20`，区分基础构造、`0x6F0AF0`初始化和`Game::Start 0x6F16D0`。构造中的`0x6EE128／0x6EE137`调用共享RNG；Start的seed helper还直接写回状态，后续有Player／房间／保存及回调链。正常启动已有根不等于已存在可安全绘制的世界壳。
- **Camera地址更正**：ST2初稿将`0x91EC20`误作构造器；主审按`Camera.zhl`完整特征定位到`0x91C730`，再读`0x7ED7E5`调用及`0x7ED7F7`写Room Camera指针。子智能体已复核并修订；旧点不用于构造或ABI接线。两个原生Camera更新helper还有四处条件共享RNG调用，不接受“相机即纯展示”的推断。
- **更新与伤害地址更正**：主审重读Manager staged位置写、普通／odd碰撞、RoomTransition中的EntityList及Player虚调用、Lua尾部。ST3初稿的`0x6AE820`并非J273 Entity Update，已撤回；主审从Entity vtable定位`0x6A9CF0`，回读`0x6AAB7E`扣值写回与条件`0x6AAD8A`虚Kill调用。Room的11项xref须分成10项代码调用与1项注册数据引用；不能据总数宣称11个直接caller。`0x92E45A`早退在Start之前，后续出口不能与它混写。
- **退休与当前实现**：主审确认`Exit(false)`在ShouldSave判断前仍有条件PGD云／本地保存，并会清只读标志；不接受它作为无保存取消接口。当前Manager生成wrapper无条件super、Authority的FrameBegin／FrameEnd和录制退出协调均不能冒充Client消息泵或安全场景退休。

接受范围是以上静态事实、明确候选及15／16接线约束。独立Replica世界根、保留render／HUD／Lua子树、资源context与最终释放、实际Hook链／线程／运行可达性、Host／Off无影响及全过程无访客持久副作用仍按后续研究、实现和统一E2E回答。本轮仅资料阅读、只读逆向、文档编辑及人工回读，没有构建、游戏、测试、离线校验或artifact包；不将静态研究记作tx-04-17功能完成。

### ST4主审补证与采用限制（2026-10-31）

ST4由用户指定的一个GPT-6 Luna／max子智能体研究；主智能体独立读取当前J273指令及REPENTOGON源码，补证以下范围。地址均为VA，imagebase为`0x400000`，不是已安装Hook或运行观察。

- **Game绘制与Lua时序**：按`Game.zhl`、`HUD.zhl`及`LuaEngine.zhl`完整特征分别定位`Game::Render 0x6F7A80`、`HUD::Render 0x97C300`和`LuaEngine::PostRender 0x849DD0`。`0x6F7E02`调用PostRender，之后`0x6F8108`以`Game+0x1D9AC`为ECX调用HUD；不能把PostRender名字解释成整个Game/HUD绘制结束。`0x849E01`设置callback编号2，`0x849E17`检查标志`&4`，`0x849E2B → 0x8591D0 → 0x85921B lua_pcallk`是条件Lua分派。保留Game绘制不等于只保留图形操作。
- **帧事件与玩法消费者**：主审从ZHL定位`ANM2::SetFrame 0x406010`，确认`0x406040 → 0x4042A0 → 0x404130`设置目标帧并更新事件位；`0x404181／0x404190`分别OR当前／历史事件mask。另从`Shoot`字符串交叉引用重新定位消费者`0x555E10`，不是按J460地址平移：`0x555EF0`以`Entity+0x78`查询事件，true分支`0x5560BD → 0x6C74B0`的mode 0先在`0x6C7513`调用共享RNG，再于`0x6C755D`压入Type=9，`0x6C7564 → 0x424340`，wrapper在`0x42435D → Game::Spawn 0x6FA210`。这是有条件的静态路径，帧setter本身不直接Spawn；须隔离后续消费者，不能把本地事件位当网络表现事件。伪码wrapper的`0x424364`行尾地址不是实际callsite，记录使用已核指令地址。
- **派生绘制并非只剩外层回调**：按三类ZHL特征独立定位`NPC 0x6CEC90`、`Tear 0x675AC0`和`Projectile 0x659460`，分别在`0x6CFE4C → 0x8470C0`、`0x675DE7 → 0x849210`、`0x6597DB → 0x848CF0`进入条件Lua分派，对应编号28、41、45。Projectile的`0x659484／0x6597D1`对`Entity+0x350`先减后加；它不是只读调用，也不是任意旧浮点值的保存／恢复合同。Tear会临时改颜色并在正常路径恢复。不能以pre-render返回false作为整树隔离，不能由三类入口推导Player组合、阴影和所有特殊几何均可由单Sprite表示。
- **HUD状态与声音归属**：`HUD::Update 0x97ACF0`直接递减本地计数器并推进Sprite；SlideIn／SlideOut条件分支汇聚到`0x97AE75 → SFXManager::Play 0x907C60`。声音选择在`0x907DA7`写独立静态随机状态`0xBD0F40`，本事实不证明它是Authority玩法RNG。须按已裁决表现事件的身份、水位和去重决定是否播放，不能让原生HUD更新与网络事件重复出声。`HUD::PostUpdate 0x97AE90`仍进入逐PlayerHUD、布局和统计显示helper；不把整条链直接列为安全展示更新。
- **渲染写入不等于推进玩法随机**：Game绘制的`0x6F7D38`写`Game+0x25AB8`。其相邻分支读取Room描述seed并作混合计算，本次没有在该段看到seed回写，不能仅因出现RNG诊断就宣称它推进共享玩法随机。`0x8587A0`包含条件`lua_gc(L,3,0)`内存统计及图形绘制，不是PostRender回调，二者明确分列。
- **公开Sprite仍依赖展示环境**：`InterpolV2.cpp`在条件成立时临时修改AnimationFrame的颜色、scale、pos和rotation，原调用正常返回后恢复，并用全局字段标记当前Game／AnimationState绘制；未证明重入或异常恢复。`Anm2Extras.cpp::RenderFrameEx`还依赖light pass、图像／shader和blend状态；Dogma／Gold分支按flag读取Game帧或Manager帧。Client需要独立表现时钟和pass适配，不能推进全局游戏帧伪造Authority tick，也不能清除特殊flags来声称完整显示。资源/context和在途寿命分别由ST5—ST7继续解释。

主审接受范围为本段直接核对的静态事实，以及[ST4正文](tx-04-17-render-presentation-boundary.md)的允许／隔离矩阵、事件归属和后续适配责任；纠错重点是区分帧置位与消费者、callsite与伪码行尾地址、表现随机与玩法随机、普通返回恢复与异常保证。原生整树没有获得安全透传结论；正常／暂停／加载展示、Host／Off无影响、真实回调顺序和无重复副作用仍留完整实现后的统一E2E。本次仅人工回读文档和依据，不为ST4另行安排测试，不表示整个tx-04-17完成。

### ST5—ST7交叉核对与主审记录（2026-10-31）

三项各由一个GPT-6 Luna／max子智能体负责，只编辑各自专题文档；三方交换资源后态、借用／引用和context约束后互读草稿，并再次核对修订。主智能体独立读取公开绑定、官方辅助源码、当前工程线程／owner实现和同一J273 IDB；以下地址均为VA，imagebase为`0x400000`。接受的是具体静态事实和设计约束，不是运行时保证。

- **资源与姿态**：重读`main_ex.lua`、`LuaSprite.cpp`、`LuaAnimationData.cpp`、`LuaAnimationLayer.cpp`及`LuaBlendMode`，区分构造器层数bool、换路径bool、图像关联及像素。J273 `Load 0x407AF0`先减旧filename引用再bind／addref，`Reload 0x407C40`没有同样的先减步骤；bind miss只警告，不能宣称旧画面已回滚。`LoadGraphics 0x407DA0`的中间loaded witness与最终pair分开。主审纠正首版漏掉BlendMode可写属性，补齐Sprite／逐层custom与champion shader的路径／有无／错误前缀；官方extras按ANM2 owner及layer ID保存，Reset／destructor清副表不等于销毁全局shader。
- **帧setter与无效输入**：独立回读`0x406010`的`std_string*` named、`0x406060`的`const char*` named及`0x4060A0`的无名int入口；两种named清playing，无名不清。`0x4042A0`有浮点时间及上界处理，公开整数帧不自动表达任意phase。`0x4043E0`／`0x4061B7`的映射错误诊断之后仍写selector并推进，不把日志当安全拒绝；先按当前动画／layer身份验证，不把时间帧、绘制order和selected-key混用。分数phase与无有效帧的实际必要表达继续反馈15／18—20，不借公开接口缺口缩小完整内容。
- **加载与context**：复核`Manager::Render 0x92EC40`的主循环caller `0x90AF0D`和启动加载caller `0x8D9228`，不能只覆盖外层主循环callsite。J273 renderer window为`+0xE0`，窗口HWND／HDC／HGLRC分别为`+0x218/+0x234/+0x238`；`0x9E4B63`调用MakeCurrent后未经返回值判断就在`0x9E4B70`写TLS，TLS不等于OS current。`0xA29FFE`删除context后同样直接清字段。官方ImGui多视口还直接切换／恢复context；现有`IsMain()`和owner栈不提供OS检查。主审纠正源码路径与伪码callsite，保留实际句柄、安装链和异常恢复未知。
- **CPU队列与最后引用**：独立回读`0x9E2170→0x9E2460`、`0x9E2EC0`、`0x9EFA70`及对应ZHL，J273队列持raw image／batch指针，不能移植J460的batch强引用。补证普通queue clear `0x9EFA10`清bit4、`0x9EFA47`令end=begin；renderer Present `0x9E4170`的apply→reset→renderer→条件SwapBuffers局部序列与Manager::Render分列。`0x9EFE60→0x9EF7C0`是计数条件下cache／queue移除，不是图像析构。正常最后强引用另由`DecrRef 0x4081A0→counter 0x408500`的1→0分支，经`0x408536`调用image vslot `+0x60(1)`，PNG／PCX析构进入`0xA4C360`，在`0xA4C369/0xA4C375`请求GL纹理删除。已读六槽表与原始指令；CPU清队列、图像删除请求、OS成功和GPU完成分别记录，不外推全异常／全raw owner覆盖。
- **Lua与退休分工**：主审复核裸指针Layer／Animation／BlendMode wrapper和Image值关联／显式GC DecrRef，区分自有Sprite与原生内嵌Sprite的UserdataPtr借用。`0xB25964→0x8AAF70`的owned thunk对payload+8调`0x403790`，`0x8A9480`以flag=0调用它；原生析构先`Reset 0x403530`，不释放外层userdata。显式Reset只限本方自有Sprite，不手调析构或GC；registry unref不立即GC，LuaImage缺可控释放公开接口仍需实现。关闭admission后应在当前回调返回后的主线程边界退休，不能在回调栈内阻塞等自己归零。
- **关闭顺序及纠错**：`Isaac::Shutdown 0x982C90`内条件`0x982DD8 Exit(1)`之后，原始`0x982DDD`字节`B9 78 6F BE 00`对应官方early-unload源码目标签名；不是当前已安装证明或保存否决。Game／Manager在`0x982E1C/0x982E45`销毁，`0x982E67`先处理ANM cache及随后部分资源，`0x98300B`才在Shutdown内调LuaEngine destroy，`0x854FA0`执行lua_close。主审纠正把`0x98300B`归到main及把全部graphics写成lua_close前清理的初稿表述：main `0x90AFCE`调Shutdown，`0x90AFD3→0xA2AD80`随后在`0xA39A39`处理ImageManager cache，再删FBO／RBO、`0xA39A6B→0xA2F440→0xA29FF0`拆context／窗口。依赖Game／ANM的资源、Lua state与最终Image／context各有自己的截止条件。

三项合同统一为未发布候选准备、完整描述检查、主线程注册关联切换、有界保留旧引用、停止新使用后按owner退休；session／scene、资源修订、实例、Lua和context代次分别失效。实际失败候选不发布，资源准备不等于World Applied，失败或stop不提前解除Client隔离。ST8访客持久副作用、世界壳、可控释放及15／16接线仍未完成；实际Hook／context、正常与异常在途覆盖、GC时点、像素和GPU结果均留完整实现后的统一E2E。本轮仅资料与源码阅读、只读逆向、文档编辑和人工整体回读，无构建、测试、实机或artifact包。

### ST8主审记录与采用限制（2026-10-31）

用户本轮要求完成ST8，必要逆向指定一个GPT-6 Luna／max子智能体；该分工不扩大为项目层独立角色要求。上面的ST2—ST7收束段保留各轮当时进度，本段将当前静态状态更新为ST1—ST8完成。主智能体直接回读当前J273指令与REPENTOGON源码，采用[ST8正文](tx-04-17-persistence-exit-isolation.md)限定的事实和设计合同；以下地址为VA，imagebase为`0x400000`。

- **Exit与run删除**：重读`0x6F635E—0x6F63BE`、`0x6F647B`、`0x9324A0`及`0x99E7B0／0x99E870`。PGD dirty／fileLoaded条件后的备份与cloud／local保存先于ShouldSave；false分支在Game `+0x25BA8`为0且Manager `+0x4AFB8／+0x4AFBC`相等时，于`0x9324CD`调用Delete，随后改内存及dirty。不能将条件路径写成每次必发，亦不能将原始EAX或反编译usercall当官方void接口的业务返回。
- **Shutdown直接进度写入与纠错**：重读`0x982D10—0x982DDD`。条件实际为Game非空、`+0x25BA8>0`、byte `+0x25BB0=0`、DWORD `+0x25CB4=0`；代理初步消息中另外两项条件偏移计算错误，主审按原始cmp更正，代理复核接受。`0x982D44`直接清PGD `+0x5BC`（counter[192]），之后`0x982D4E`增加counter[522]、`0x982D5C`本地保存；counter[522]业务含义保留UNKNOWN。随后独立dirty／fileLoaded分支先清dirty、备份并保存，再按Manager state=2条件调用Exit(true)。readonly不能约束前述直接写，early unload更晚。
- **返回值与官方整链**：重读`0x932300／0x9323F0`后的指令，原生Manager保存调用者不检查write的bool便继续文件／PGD处理。源码`GameStateManagement.cpp`的write／write_rerun在原生前后改暂态，后续分别调用ESSM、VirtualRoom与ItemPool保存；Delete亦扩展删除附加文件。bool既不是全部文件原子成功，也不能作为下层阻断后caller已安全返回的证明。
- **Lua、官方进度与本地设置**：直接阅读`LuaPersistentGameData.cpp`、`AchievementsStuff.cpp`、`ChallengesStuff.cpp`、`CompletionTracker.cpp`、`EntitySaveStateManagement.cpp`、`onlinestub.lua`、`OptionsSyncing.cpp`、`InputConfigs.cpp`。确认forceunlock临时绕readonly、官方独立JSON及ESSM内部Mod文件入口；原版Online stub不覆盖C/S，不能复用其状态。Options Load可写文件，Save先super再同步INI，允许设置须定义本地来源与字段，不能只凭函数名、路径或扩展名放行。
- **关闭与当前工程边界**：重读`ASMCallbacks.cpp:1876—1918`及原始`0x982DDD`补丁位置，early `MC_PRE_MOD_UNLOAD`在保存之后、Game／Manager销毁之前；与ST6／ST7的ANM cache、Lua close、ImageManager／context分别退休合同一致。主审另补读`SaveSyncing.cpp:611—647`，确认Manager析构Hook在super前仍可按启用／启动成功条件同步三档RGON／原版PGD并写同步状态JSON，经自有SaveFile写入。不能在early unload后提前撤销持久归属管理，也不能把正常本地版本同步当作Client服务器进度写入。当前`gen_binding.py`／`persistence.cpp`仍记录并调用super；`exit_patch.cpp`／`WorldProcessExitDecision`服务world录制收口，不能算Client玩法／持久隔离或通用关闭协调实现。

静态研究只说明上述条件路径、返回限制和应采用的隔离责任，没有证明其在未来Client安装环境的实际命中。入口、准备／失败／stop／退出均须在进度修改及官方整链副作用之前处理；只读位、文件拦截、回滚dirty、退出回调和提前Off均不能替代。保留访客已有进度／Continue，允许本地设置有明确归属；15／16／31及18—20／27的反馈和未决项见正文。已完成本轮文档整体人工回读，未构建、运行游戏或测试，不留artifact包，也不宣称tx-04-17完整功能已完成。

## 5 补充研究与必要逆向的具体任务

本节为2026-11-01新增计划；用户随后依次启动R1—R6，已形成[R1范围选型、静态事实及未决项](tx-04-17-client-scene-lifecycle.md#r1-results)、[R2对象生命周期及R1回补](tx-04-17-native-object-lifecycle.md)、[R3维护／绘制分派](tx-04-17-native-maintenance-rendering.md)、[R4资源／借用／在途寿命](tx-04-17-native-resource-lifetime.md)、[R5失败／退休／持久整链](tx-04-17-failure-retirement.md)和[R6模型／角色／应用器归并](tx-04-17-model-role-applier-integration.md)。各项指定范围的静态文档结果已形成，具体UNKNOWN和未实现接线仍保留。按“本地原版Wiki／REPENTOGON Wiki → 官方源码／当前工程 → 仍未解释的具体J273路径”处理，不要求每项都使用IDA，不重做已解释的ST事实，不以全引擎逆向替代实际需求。静态资料能说明调用合同，真实命中、安装链、context、像素和异常路径仍由完整实现后的统一E2E观察。

默认顺序为 **R1原生初始化 → R2三类操作公共生命周期 → R3维护与绘制 → R4资源与借用寿命 → R5失败退休与持久整链 → R6模型／角色接线归并**。这描述数据与生命周期依赖，不新增项目准入流程。R1即须读取ST8并标出持久副作用的阻断位置，不能等R5才考虑进度保护；R2／R3遇到释放或退出疑问即时回看R4／R5，必要结论回补前项。无需重复已解释的入口，也不拆新tx或为每项单独测试。

| 顺序 | 任务与依赖 | 必须形成的可实现说明 | 状态／落点 |
| --- | --- | --- | --- |
| R1 | 原生场景初始化；复用ST2／ST3／ST8、16入口 | 选择具体初始化路径，逐段说明前置对象、参数来源、保留／替代／禁止调用、正常与部分失败出口、初始化期间进度隔离 | 本轮范围选型与主审完成；采用已有根上的自有编排＋具名原生helper，Room必要结构段／Player初建／异常退休仍未闭合；[事实与未决项](tx-04-17-client-scene-lifecycle.md#r1-results)，反馈R2／R3／R5 |
| R2 | create／remove／change公共生命周期；依赖R1的场景owner | 原生工厂／成员提交／Morph／摘表与回收次序、稳定key与实例代次、连带对象归属、重入和重复目标处理 | 本轮静态公共合同及R1回补见[R2结果](tx-04-17-native-object-lifecycle.md)；已分开池use／发布／owner／摘表／回收，逐类Init闭包及完整异常退休仍未闭合，未实现或运行验收 |
| R3 | 停玩法后的维护与原生绘制；依赖R1／R2的有效对象 | 保留的维护／draw路径及所需状态，隔离玩法消费者与Lua的具体位置，显示时钟和音画唯一触发归属 | 本轮静态合同／主审及前项回补见[R3结果](tx-04-17-native-maintenance-rendering.md)；排序、位置插值、相机、HUD及draw阶段分开，具体派生绘制／资源与异常仍有UNKNOWN，未实现或运行验收 |
| R4 | 原生内嵌资源、借用与在途寿命；依赖R2的实例变化和R3的使用点 | 内嵌Sprite／图层／Image／userdata的owner与失效表，资源替换及对象退休前后顺序、context与队列限制 | 本轮静态合同及主审见[R4结果](tx-04-17-native-resource-lifetime.md)；核定代表对象析构／Morph、容器与ANM借用及native Image pin，custom-render延后上下文为关闭分支；完整取消、LuaImage GC与异常退休仍有UNKNOWN，未实现或运行验收 |
| R5 | 部分失败、场景退休、菜单交还与持久整链；整合R1—R4 | 各准备阶段失败／断连／stop／退出的收口表，安全摘除与延迟释放、进度保护结束位置及无法恢复时的实际行为 | 本轮静态合同及主审见[R5结果](tx-04-17-failure-retirement.md)；核定ResetState、Room重建→Player／HUD清理、菜单请求与持久截止点，选择自有退休编排；完整菜单调用、取消／异常仍有具体UNKNOWN，未实现或运行验收 |
| R6 | 15／16接线、18—20接口与整体验收归并；使用R1—R5 | A→T应用计划、角色／场景转换、应用结果／错误、域职责和验收覆盖；列明剩余具体UNKNOWN，不伪造接口已实现 | 本轮源码归并／设计结果见[R6结果](tx-04-17-model-role-applier-integration.md)；明确B／T／A／原生计划、Scope及输入切换、接纳与效果提交、接口和任务责任；无新增逆向、代码或测试，具体前置和新增接线仍未实现 |

<a id="r1"></a>

### R1 原生世界初始化的可复用范围

**2026-11-01本轮结果**：先查公开接口与官方源码，再由用户指定的一个GPT-6 Luna／max子智能体补J273逆向，主智能体独立复读并纠正伪码点位／参数含义。选择自有准备编排，复用已有Manager／Game／容器、独立HUD资源准备、纯门格计算及满足几何前置后的Camera构造；不整体执行Start／Level Init／Room Init后清空。Room Init在几何写入前已清理对象和改池，首参虽在ZHL写int，实际调用传RoomConfig指针；Data／OverrideData共同决定几何。Camera替换先释放旧对象，不能假设失败原子回滚。完整说明、服务器参数归属、步骤／不变量、持久隔离、主审纠错与未决表见[场景生命周期R1](tx-04-17-client-scene-lifecycle.md#r1-results)，公开能力与官方Hook边界见[接口目录补充](tx-04-17-public-interface-catalog.md#r1-init-interfaces)。本轮不声称已形成完整可调用建壳器；下列原任务保留为后续补齐具体缺口的范围。

- **已知起点**：ST2已解释正常启动已有Manager／Game／基础Room，Camera须经有效Room setup建立；Start还会初始化随机／ItemPool、创建Player、生成楼层／房间、更新Room及进入保存／回调。复用这些事实，不把基础Room当成可绘世界。先读ST8的Start／Exit／Shutdown与官方保存链限制。
- **公开接口与源码任务**：查Game／Manager／Room／Camera／PlayerManager／HUD的构造、setup、布局和参数要求，以及REPENTOGON外围Hook；对照16稳定菜单、消息泵与主线程owner。区分必须由服务器提供的房间／角色／模式描述、引擎自身初始化值、仅用于显示的辅助状态和不允许重新生成的玩法内容。
- **必要逆向任务**：源码未解释处，沿ST2已记录的ExecuteStartGame、Game::Start、Room setup及Camera构造实际调用链，找出可以复用的连续段或具名helper；核定所选调用的ABI、前置字段、隐含全局依赖、容器发布位置、连带实体创建、早退与部分失败后态。只追选定路径与必须的直接依赖；不得把旧J460地址或未知helper伪码当作可调用接口。
- **设计结果**：比较“受控复用原生启动链”和“复用已解释的原生setup子路径”，选定具体方案及理由；列出每一步的对象owner、服务器输入、允许写入、Gameplay／Lua／持久阻断点、调用后不变量和取消方式。说明如何形成有效Room、Camera、实体列表、必要Player／HUD上下文，而非手填几个字段或只画菜单Sprite。
- **边界**：原生Start整体是否能复用仍为UNKNOWN；有随机调用本身不是否决理由，但需解释局部状态和消费者，不能自主决定服务器世界。失败时不能先继续跑原生玩法再收拾结果。无法说明必要构造依赖时保留具体缺口，不将其改名为“空世界已就绪”。

<a id="r2"></a>

### R2 create／remove／change的公共原生生命周期

**2026-11-01本轮结果**：先读公开接口与官方1.1.3源码；用户指定的一个GPT-6 Luna／max子智能体补必要J273逆向，主智能体独立复读关键指令与源码后取舍。公共合同见[R2设计与事实](tx-04-17-native-object-lifecycle.md)：create经对应owner受控构建／派生Init／成员维护后提交；remove区分逻辑停止、摘表、引用清理及物理回收；change保留逻辑key，必要时更换本地实例并重绑引用。工厂池耗尽选不覆盖存活对象的分支；Player不走通用Factory；完整Spawn、Morph及ClearReferences均有需解释的后处理／连带行为。NPC Morph原生体的PGD bestiary写入已定位并反馈持久隔离；R1的专用Player准备／Room成员依赖及未决项同步回补。没有修改代码、构建或运行测试，不将本轮合同视为逐类adapter或完整Client初始化已实现。下列任务保留为后续补齐具体缺口的范围。

- **前置与范围**：基于R1确认的容器与owner，选取足以覆盖17公共机制的代表性原生对象和场景；同时记录哪些机制为Entity共用、哪些由Grid／Player专用adapter接入。明确样本及选择理由，不以一个Effect外推所有类别。
- **公开接口与源码任务**：核对Spawn／Init／Remove／Morph、成员列表提交、移除通知与Lua wrapper，以及既有identity／lifecycle观察。列出构造后绑定key、重复Init／重复目标、引用待补、原位change、形态重建、摘表及延迟释放的次序。
- **必要逆向任务**：只对拟用原生工厂、Init、Morph、remove／CommitRemovedEntities路径中源码未解释的分支补查：何时加入列表，回调与连带生成在何时发生，死亡／掉落和单纯摘除是否分开，哪些引用或缓存会留下旧实例，正常返回后谁拥有回收责任。不得用Kill代替Remove，也不能跳过必要构造后直接复制内存或vptr。
- **设计结果**：形成按操作列出的“输入描述 → 原生调用／保留工作 → key与实例关联 → 引用处理 → 可显示提交 → 退休”表。说明Morph逻辑key不变、内部实例换代、Grid同格冲突、多身体引用的公共接口及失败返回；具体类型字段与全部角色差异交18—20。
- **连带对象与幂等**：区分服务器对象、必要原生辅助对象和禁止的自主生成。明确实例关联失败、create中途成功后重入／取消、remove已摘表但未释放时如何保持一致，不能重试整个计划造成二次生成。

<a id="r3"></a>

### R3 原生维护、绘制与玩法分派

**2026-11-01本轮结果**：优先查公开接口和官方源码，再由用户指定的一个GPT-6 Luna／max子智能体补必要J273静态逆向，主智能体独立复读取舍。按R2依赖完成生命周期／目标应用与必要移除，再排序／准备显示缓存，进入原生pass／draw；不强定所有create先于remove。`render_sort`有空表、共享scratch及渐进排序限制；Entity::Interpolate含位置／碰撞，Camera::DoUpdate进入含共享RNG的路径，均不作Client显示时钟。HUD资源／关联／缓存／绘制分开，PlayerHUD输入Lua、History即时1029、shader时间、动画事件、音效和在途资源分别归属；不清视觉flag或跑Gameplay Update补显示。完整输入／允许写入／频率／失败及Host／Off透传合同、R1／R2回补与具体UNKNOWN见[R3结果](tx-04-17-native-maintenance-rendering.md)。本轮只改设计文档，无代码、构建或测试；下列任务保留为所选实际路径的后续补齐范围。

- **复用事实**：ST3的ordinary／odd／FSM／loading和外层写入，ST4的Game／Entity／HUD绘制、动画事件与Lua入口已解释一部分。停止普通Room更新不等于停止全部玩法；原生Render含写入也不等于必然不可复用。
- **公开接口与源码任务**：从R1／R2的实际对象出发，列出render所读的姿态、空间字段、排序、引用、HUD布局、Camera和特效时间；区分权威最终值、本地显示缓存、必须执行的资源维护及玩法状态。读取官方排序／移除提交／HUD／插值与回调包装源码，不把名称当副作用分类。
- **必要逆向任务**：针对仍不明确的必要缓存写入者、draw helper和混合维护链，向前追到可以单独调用或适配的位置，明确原函数前后尚有何种写入；核定所选普通、odd、FSM、加载入口的角色分派与重入处理。只扩展本次保留路径，不重扫全部Entity变体；具体绘制差异移交18—20／27／29。
- **设计结果**：每条保留路径写明所需输入、允许的本地写入、调用阶段／频率、禁止消费者、异常后态和Host／Off透传方式；明确动画事件、HUD声音和网络表现唯一触发者，以及显示时间与服务器tick的关系。不通过整条Player／Entity／Game Update求出缺失缓存，不清除必要视觉flags来假装完整显示。

<a id="r4"></a>

### R4 原生内嵌资源与在途引用

**2026-11-01本轮结果**：公开接口与官方1.1.3源码优先，必要J273静态逆向由用户指定的一个GPT-6 Luna／max子智能体执行，主智能体复读审阅。[R4资源寿命合同](tx-04-17-native-resource-lifetime.md)分开原生实例、服装／portrait容器、内嵌ANM、Layer／Animation／BlendMode借用、Image strong、Lua与context代次；核定Entity／Player／Room／代表Grid析构和NPC／Pickup原位Morph资源重载。主审确认custom-render的延后Entity／Animation上下文为EnableCustomRendering=false且patch入口注释的关闭分支，只记条件风险；当前按实际调用栈、Image raw队列和采用pass处理在途，正常消费与取消分开。本方native Image pin可用类型化smart-pointer值控制释放，但不解决既有LuaImage的GC时点。原生owner负责内嵌资源析构，本方不对借用Sprite手动Reset／GC；完整失效表、候选／在途／最后释放顺序、预算、主审纠错及具体UNKNOWN见正文。本轮只改设计文档，无代码、构建或测试；下列任务保留为实际采用路径的后续补齐范围。

- **复用事实**：ST5—ST7的Load／Reload、姿态setter、raw renderer queue、context、Lua userdata／registry、GC和关闭顺序保持。新增范围是由原生Entity／Player／Room拥有的资源、R2替换实例和R3实际绘制，不重做整个图形后端。
- **公开接口与源码任务**：核对内嵌Sprite的借用、层／动画／Image／shader关联，原生destructor与官方扩展副表的清理；建立实例、资源版本、Lua与context分别失效的表。明确自有独立Sprite与借用Sprite的不同释放权限，不能对借用对象套用自有Reset／GC策略。
- **必要逆向任务**：源码未解释的所选实例销毁／Morph／换资源路径，追其对Sprite、Lua借用和render raw引用的影响；将已知队列清理点与实际对象退休owner连接起来。只补必需的可控Image释放、在途持有与取消缺口；线程TLS、CPU清队列和GPU完成保持不同含义。
- **设计结果**：列明资源候选准备、旧资源保留、实例切换、借用失效重取及最后释放时机；缺资源／无效姿态／context错误／停用发生于在途绘制时的处理和预算。不提前确认半套姿态，不以loaded或Lua值仍存在代替原生对象有效。

<a id="r5"></a>

### R5 失败清理、原生退休与访客持久隔离

**2026-11-01本轮结果**：先读公开接口和官方源码，再由用户指定的GPT-6 Luna／max子智能体补实际Exit清理、ResetState、Room重建、Player退休及菜单转换，主智能体独立复读纠错。[R5失败／退休合同](tx-04-17-failure-retirement.md)按准备前、部分构造／create／change、待释放remove、已展示、资源替换及正常关闭列处理表。ResetState原生只改计数／标志，官方ESSM前置仍需有效descriptor；正常Exit先重建Room，再清PlayerManager／HUD，不是任意部分失败回滚。菜单请求helper有state1后态但混入原版Online，不整体采用、不触及其状态；Client继续自有编排和具名helper方案。保护先于首次写入，覆盖Shutdown直接PGD及Manager最后相关writer；安全菜单后可Off，常驻归属策略仍保留，未知残留不交还。当前worker／两轮松键／MenuReady与录制依赖退出接线不足以证明世界退休。具体前置、失败前缀、预算、不可恢复行为、主审及UNKNOWN见正文。本轮只改文档，无代码、构建或测试；下列任务保留为所选实际路径的补齐范围。

- **前置**：R1起即按ST8安排进度保护；本项将选定的新路径与R2—R4全寿命汇总，不把保护延后到退出。原生场景存在不等于客户端拥有一个可保存为访客Continue的权威run。
- **公开接口与源码任务**：复用Exit、Shutdown、GameState write／delete、官方附加文件、PGD直接写、SaveSyncing及本地设置归属研究；沿实际采用的初始化、场景替换和释放链检查进入这些路径的条件。将访客原有Continue／进度、服务器场景和允许的本地设置分别登记，不读取／覆盖访客存档来构建服务器世界。
- **必要逆向任务**：只有新路径仍不明确时，补查部分构造后的清理入口、原生对象摘除到菜单状态转换、延迟回收／在途引用与Shutdown直接写入的先后关系。核定能否复用经分派的原生退出子路径、调用前置与正常返回后状态；Exit(false)、readonly、下层write=false和early unload均不提供现成安全取消保证。
- **设计结果**：按“初始化前／部分构造／部分create或change／逻辑remove后待释放／已展示／资源替换／正常关闭”列失败与退休表，写清取消、停显、原生清理、保持隔离、重建或明确停止的行为；给出回到菜单及解除Client身份的实际条件。无法安全恢复时如实记录，不承诺任意SEH、access violation或强制终止后都能恢复。
- **交接**：先停止新输入／对象使用与publication，再在有效owner阶段处理在途资源；保护覆盖官方最后相关写入者，Host合法保存和Off正常路径不被全局关闭。向31提供最终归属与尚需完整产品处理的Continue功能边界。

<a id="r6"></a>

### R6 模型、角色与后续应用器接线归并

**2026-11-01本轮结果**：[R6源码与接口归并](tx-04-17-model-role-applier-integration.md)已对照15的Capture／Difference／Reconstruct／ModelWindow／事件与控制窗口，以及16的ClientRuntime、固定Event／Intent队列、等待Scope、小控制预算、ClientManagerBoundary／输入泵和退出清单形成合同。明确Stage的Current是T而非A，A→T三类操作、依赖／循环关系、跨帧子步骤、部分写入后A资格失效、版本取消和有界恢复；原生提交结果与Applied ACK分别由17和21—22负责。等待Scope不当正式世界，Scope转换还需处理worker路由与旧待发输入，Event／Control接纳不当效果完成；保留现有wire和公开接口语义，具体parameters／能力／本地完成记录缺口逐项列明。连接、Client场景状态和持久归属寿命分开，18—20／21—29／31责任及E1—E12统一验收接线已归并。原生前置／完整菜单／全采用pass取消等UNKNOWN不因归并消失。本轮只更新设计文档，未改代码或运行测试；无需新增逆向。下列条目保留接口实现时的具体责任。

- **源码为主**：对照15的model／ModelWindow、16的ClientRuntime／ClientManagerBoundary／输入泵和当前Hook清单，将R1—R5结论汇成实现接口。此项本身不要求逆向；只有发现具体未解释的native依赖才回到相应R项补查。
- **模型合同**：明确Baseline／Delta先重建T，再按A→T规划三类操作；说明scope变化、跨域引用、待解析关系、预算、同版本冲突、目标取消、部分写入后A失效和恢复行为。保留现有replace／remove wire编码；需要新字段时列出具体用途与版本影响，不在本次文档修订中改协议代码。
- **角色合同**：连接入口保留稳定菜单要求，Client有原生场景后采用独立场景状态；退休不再只凭MenuReady、worker结束或两轮中性判断。复用物理消息泵与统一输入，无第二Authority时钟；原生场景逻辑与连接状态分开，旧scope输入／对象／事件不能复活。
- **接口结果**：逐项说明公共applier所需描述、调用线程／阶段、成功／失败状态、原生逻辑提交条件与21—22生成ACK的责任；18—20承担具体类的实现，23以后扩展完整切房／换层／生命周期。首轮代表性原生场景和所有采用路径必须列入17统一E2E，不能只验证菜单或包数。
- **交付位置**：研究结果续写对应专题的当前设计部分及本节状态，保留引用、纠错与UNKNOWN。无需另留artifact；原生调用、允许副作用、失败处理和剩余缺口达到可实施的具体程度，不以“后续适配”代替说明，也不将静态结果记为功能PASS。

## 6 完整实现工作

以下为后续实现计划，本次不修改代码。在编写对应实现前，按第1、5节列出的失败方式补足具体处理；复用ST事实和R任务结果以及15／16的实际交付。职责名不是已有API，原生调用范围尚需R任务落实。

| 工作 | 必须形成的行为 | 主要失败方式与处理 |
| --- | --- | --- |
| I1 角色与原生场景（R1／R5／R6） | 原生初始化、准备／应用／展示／恢复／退休状态接入16；有效Room、Camera和必要Player／HUD由原生体系承载，Host保留Authority，Off透传 | 角色／scope冲突、构造失败、残留场景：停显并保持隔离，不把服务器场景交给本地玩法或访客Continue |
| I2 三类操作公共机制（R2／R6） | 稳定key到原生实例的映射、create／remove／change、依赖与引用、Morph换实例、成员提交和幂等；接入代表性实际操作 | 重复目标、地址复用、连带生成、引用迟到、槽位冲突、部分写入：不重放已执行动作，不伪造删除或绑定，保持错误可恢复性边界 |
| I3 玩法分派（R1／R3） | 统一Hook清单按构建／应用／展示用途分派，覆盖普通／odd／FSM／加载及外层已采用路径 | 漏入口、重入、异常或未知上下文：停止相关工作；观察wrapper仍exactly-once，主动分派的跳过与受控原调用明确记录 |
| I4 原生展示维护（R3） | 服务器状态供应原生draw所需数据，保留必要排序／缓存／相机／HUD／姿态，分离玩法Lua和动画消费者 | 缺必要状态不调用完整玩法Update补算；派生绘制副作用、重复音效、未知回调按具体adapter处理，不以独立Sprite代替原生世界 |
| I5 资源与姿态（R4） | 自有资源及原生内嵌Sprite的描述／代次／候选管理，main／overlay／逐层、换图与资源重建 | 缺资源、无效姿态、错误借用、半套换图和预算耗尽：不发布候选，借用失效后重取，禁止错误Reset／析构 |
| I6 线程与Lua寿命 | 原生／Lua／图形操作只在已解释阶段执行，后台只持有自有值 | 错误线程、context不符、userdata失效、关闭中回调：取消候选、停止发布并按有效owner清理 |
| I7 持久隔离 | 在准备前建立访客原有进度／Continue归属；隔离服务器run、统计、解锁、成就、挑战、完成标记及Mod保存，保留具名本地设置 | 在官方整链副作用之前阻止错误来源的请求；下层write=false、readonly或dirty不代替caller处理。失败／断连／stop保持必要隔离至安全退休，Host合法保存不因Client规则被全局关闭 |
| I8 发布、恢复与退休协调（R5／R6） | T从模型完成到原生逻辑提交分阶段报告；失败停显、完整目标重建、安全菜单交还，并向21—22提供Applied结果 | 原生不具备通用回滚；部分应用后不保留虚假的A有效性，不确认半套目标、不复活旧对象；回收未安全时不切Off |

所有Hook经唯一清单注册，启动安装后常驻至进程结束，会话仅逻辑启停；Off原样透传，不恢复目标字节或强卸载DLL。新增资源的正常退休遵循各自owner，不改变既有自有virtual device的NEVER回收策略。

实现时明确对象、资源、候选、队列和在途内存预算；游戏线程不等待网络或磁盘。正式运行不依赖调试录制、逐tick日志或测试夹具；核心函数、功能和数据结构使用中文注释。不编写实现后的单元测试。

## 7 全目标完成后的统一整体验收

统一入口在实现时接入现有E2E编排，以真实游戏Host／Client角色验证17新增行为。Rust只作编排或辅助工具，不能替代真实Client。17可由测试夹具提供明确来源与scope的受控目标模型，不因此宣称21—22的完整实时下行已完成；create／remove／change须走正式公共applier并实际作用于原生场景。选定对象／路径与未覆盖类别明确列出，不能以代表性场景外推18—20全域完成。场景参数、持续时间、预算和用户操作按本tx范围明确，不机械沿用旧样本数或重跑全部历史矩阵。

| 场景 | 必须观察的结果 |
| --- | --- |
| E1 原生构建、Host Client Off与等待 | 从稳定菜单经受控原生初始化形成实际Room／Camera／实体与必要Player／HUD；显示由原生世界承载；未提交时明确等待，Host正常模拟、Off透传，不遗留可被本地玩法接管的场景 |
| E2 普通 odd和特殊FSM | 实际触发相关路径；Client无本地AI、碰撞伤害、使用、拾取、掉落或玩法随机裁决；不能仅凭输入为零或包数判定 |
| E3 loading 暂停和恢复 | 原生必要排序、缓存、资源、相机和HUD按阶段维护；加载旁路不恢复玩法，显示时钟不伪造Authority tick，旧输入边沿不补发 |
| E4 动画事件与Lua | 受控展示持续更新，实际包含动画事件和回调路径；不因本地事件再次生成服务器已裁决的实体／奖励或产生玩法副作用 |
| E5 资源和姿态 | 正常加载、main／overlay、独立层、外观、动态换图及资源重建显示正确，不以单个屏幕Sprite覆盖全部边界 |
| E6 失败候选、部分原生写入与预算 | 缺资源、无效描述、构造／应用部分失败及预算耗尽明确处理；写入前可保留有效旧展示，部分写入后停显并清理／完整重建，不假称回滚，不发布或确认半套目标 |
| E7 失效借用与旧代次 | 资源／场景重建后重新获取借用；旧epoch、迟到姿态和无效引用不能复活旧对象或污染当前展示 |
| E8 线程 context与退休 | 记录实际阶段和必要运行条件；加载、换图、GC、stop与在途绘制不越过owner／context寿命；错误条件下停止相关操作 |
| E9 访客进度与保存 | 覆盖准备、加载、render／HUD、失败／断连／stop、返回菜单及正常进程退出的整个窗口；联合观察内存进度、平台通知、run写入／删除、备份、官方进度JSON／附加文件／PGD版本同步与Mod保存。访客原有Continue／进度归属不被服务器世界改变，允许的本地设置来源和字段明确；不以dirty=0、文件未变或write=false单独判定 |
| E10 stop 故障与正常退出 | 准备中／展示中／可恢复失败后按已解释路径退休并回透传；清理状态不明时保持隔离并明确错误，不伪造Off。正常返回菜单、窗口退出及Lua／context清理无旧回调或资源使用 |
| E11 create／remove／change与身份 | 实际原生create、原位change、需要替换实例的change和remove；同版本重复不重复执行，Morph保留key、旧实例引用失效，连带对象不重复生成，remove不触发掉落／奖励，延迟释放期间无悬空使用；未采用类别的特殊差异仍属18—20 |
| E12 目标版本、依赖与提交 | B→T模型重建后按A→T应用，覆盖值回到基线、字段清空、成员删除、引用依赖、旧scope／旧目标、取消及恢复；只有实际原生逻辑提交才产生成功结果，失败不Applied，不以收包或DTO Stage代替 |

测试专用夹具只服务统一E2E，不进入发布模组；场景动作按已有授权执行。每次游戏启动前分别提示用户需要配合的操作及何时停止手动操作，需要手动配合时先确认已准备好再启动。不从本文推导新的存档、成就、持久写入或其他实机操作授权。

全目标实现完成后才进入上述统一验收。此前不运行smoke、probe、harness、自测、隔离试验或离线校验器，不把分项测试改名成静态核验。验收发现问题在同一tx修复并继续必要测试，覆盖修复影响和未完成场景；不因一次失败默认停下等新指令，也不为续测更换tx。

tx和E2E默认不留artifact，正常临时材料用后清理。仅用户叫停、缺必要配合／授权或无法自行解决的阻塞而需要中断恢复时，保留必要诊断与进度并说明位置和用途。保留实际失败与修复事实，最终报告覆盖、未执行、未完成和运行边界，不以文件清理或目录存在判断功能通过。

## 8 执行顺序与交付范围

1. **方案及R1—R6静态文档结果**：用户依次启动R1—R6，公开接口／源码、必要的有限逆向与主审形成初始化、对象生命周期、维护绘制、资源借用、失败退休及模型／角色／接口归并结果。R6本轮无需新增逆向。均未修改代码、构建或测试；通过统一人工文档审阅收束，不运行校验器或分项测试，不把具体UNKNOWN改作已解决。
2. **所选路径的具体补齐**：按[R6未决表](tx-04-17-model-role-applier-integration.md#r6-open-items)及R1—R5原表补实际代表路径的可调用Room／Player／派生Init、draw／取消／退休与菜单合同，落实Scope／输入／结果及事件的接线缺口。复用已解释事实，只回补真正采用的前置，不全引擎扫、不新增编号或分项试跑。
3. **完整实现**：后续进入实现范围后按研究结果完成I1—I8及统一E2E编排，先写清可能失败方式，再编写对应代码与中文注释。持久隔离在任何原生构建前启用；依实际调用依赖接线，不留下已知公共缺口等待29补救。
4. **统一整体验收**：全目标完成后一个入口覆盖E1—E12，发现问题在同一tx连续修复与必要续测；每次启动分别提示操作并在需要配合时确认用户准备好，确需中断才保留必要恢复材料。
5. **结果与后续接入**：记录完整行为、失败修复、实际覆盖和未决项；18—20接具体类，21—22接实时下行／ACK／恢复，23以后完成整局流程。研究完成、原生场景形成、代表性操作通过和完整可玩世界分别陈述，不能互相代替。

## 9 历史工作量估算

以下为2026-10-31旧独立展示路线的历史会话估算，按熟悉项目的单人有效工程量计；保留原值，不作为2026-11-01原生世界方案的工期或工作量承诺。新估算需依据R1—R6实际发现调整，不为文档修订虚构减少比例。

| 部分 | 估算 |
| --- | --- |
| 既有研究复用 公开接口与源码核对 必要静态逆向 | 3—6人日，可先行 |
| 玩法隔离 资源与生命周期完整实现 | 5—8人日，随15／16接线 |
| 统一E2E与修复续测 | 2—4人日 |
| 合计 | 10—18人日 |

估算包含正常研究、实现与修复，不包含15／16工作或设备／人工等待；ST2／ST3发现新的必需引擎适配时据实调整。先行研究将风险提前暴露，不表示总工程量自动减少，也不是工期上限或完成承诺。
