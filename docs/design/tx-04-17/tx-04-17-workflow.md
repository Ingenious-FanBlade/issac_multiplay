# tx-04-17 客户端玩法隔离与资源展示运行时工作流程

日期：2026-10-31，Asia/Singapore。状态：**ST1—ST8指定范围的静态研究与主审完成；ST2／ST3及ST5—ST7已完成来回互核**。成果见[接口目录](tx-04-17-public-interface-catalog.md)、[场景根与生命周期](tx-04-17-client-scene-lifecycle.md)、[更新隔离](tx-04-17-update-isolation.md)、[展示边界](tx-04-17-render-presentation-boundary.md)、[资源与完整姿态](tx-04-17-resource-pose-runtime.md)、[线程／context／在途绘制](tx-04-17-thread-context-lifetime.md)、[Lua与借用寿命](tx-04-17-lua-borrow-lifetime.md)、[访客持久副作用与退出隔离](tx-04-17-persistence-exit-isolation.md)。Client世界壳及隔离运行时仍未实现，具体构造／展示／可控释放与持久隔离缺口和运行UNKNOWN保留，未构建或测试。

本tx为真实游戏Client建立不执行权威玩法的运行环境，并交付资源准备、姿态应用、正常展示与退休的公共运行时。**与协议无关的静态核验可以先于tx-04-15／16进行；角色接线和完整功能验收仍依赖15／16。** 静态结果应及时反馈15的资源、姿态、引用和错误模型，避免协议定稿后才发现表达缺口。

执行规范见[现行项目规范](../../refactor/project-conventions.md)，完整范围见[09—34路线图](../state-synchronization-roadmap.md)，技术模型见[状态同步设计](../state-synchronization.md)。本文是tx-04-17的专项工作入口，不改变其他tx的范围与编号。

## 1 完整目标与责任范围

目标环境为官方REPENTOGON 1.1.3与Repentance+ v1.9.7.12.J273。Host继续作为唯一Authority，Client只上行输入意图并展示Authority裁决的结果；Off保持原生行为。C/S不进入或复用原版Online、Isaac Sync及其可变状态。

本tx须交付：

1. Client最小运行环境及角色条件下的启停控制，明确Game、Room、相机、HUD、渲染和Lua所需的实际有效对象。
2. 普通、odd、FSM、loading、render及其外层处理的玩法隔离，覆盖AI、碰撞与伤害裁决、物品使用、拾取、掉落、玩法随机和玩法Lua。
3. 资源加载、缓存、动态换图、main／overlay及逐层姿态、渲染context与Lua寿命的公共适配。
4. 候选资源准备、受控发布、部分失败、引用失效、停止和正常退休；失败后不能放开本地玩法或发布半套候选世界。
5. Client服务器世界与访客原版run、统计、成就、解锁和保存的隔离，保留按产品语义允许的本地设置行为。

| 相邻任务 | 与17的责任关系 |
| --- | --- |
| 09—14 | 提供输入、完整观测与实际缺口；先行静态研究不提前改造其采集，也不宣称观测已经成为复制协议 |
| 15 | 负责复制模型、稳定身份、资源描述、状态与事件及协议合同；17反馈展示实际所需的数据和失败语义 |
| 16 | 负责Host／Client／Off、真实连接、输入上行、主线程消息入口及连接生命周期；17在该基础上接入隔离和资源运行时 |
| 18—20 | 负责完整Room／Grid、非Player Entity及Player的具体应用；17提供公共边界、资源与寿命合同，不提前宣称这些域已经应用完成 |
| 21—22 | 负责实时Baseline闭环、Delta、Applied ACK与每Peer恢复；17提供准备／提交／失败结果，不因资源已加载就声称整个世界已应用 |
| 23—29 | 扩展整局场景、完整HUD音画、声明Mod与全内容差异；新增适配必须沿用17的隔离约束，发现公共边界缺陷须修正，不能在17留下已知必需路径后统称交给29 |
| 31 | 负责房主合法保存／Continue及各端进度归属的完整功能；17先防止访客本地发生服务器世界的持久副作用 |

一个屏幕Sprite可以辅助检查资源路径，不能代替本tx的完整玩法边界。反之，本tx也不以18—20全部世界应用先完成为研究前提；统一E2E使用足以覆盖本tx实际路径的受控场景与资源。

## 2 当前基础与依赖安排

现有[Hook清单](../../../mod/binding/hooks.json)已经登记J273的Manager／Game／Room／odd、Player、生命周期及部分持久入口；[主线程与owner管理](../../../mod/src/hooks/context.cpp)可复用。当前[Session](../../../mod/src/core/session.h)只有None／Solo／Host，[网络Worker](../../../mod/src/net/worker.h)仍明确为DLL内Authority。这些基础不代表真实游戏Client或Replica隔离已实现。

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

## 4 先行静态核验清单

ST1—ST8是内部工作索引，不是单独推进编号或分项验收。每项结果直接更新相应条目及必要设计说明，不要求源码快照、日志包或独立artifact。每条结论标明公开文档语义、当前源码事实、目标二进制静态事实、静态推导或UNKNOWN；实际线程、像素、调用次数及运行可达性不得由静态资料伪造。

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

## 5 完整实现工作

在编写对应实现前，先列出其可能失败的方式和所需处理，再完成全部接线。以下工作使用ST结果和15／16的实际交付；建议职责名不是新增已存在API。

| 工作 | 必须形成的行为 | 主要失败方式与处理 |
| --- | --- | --- |
| I1 角色与最小场景 | Client进入准备／展示／退休状态，Host保留Authority，Off原样透传 | 角色与session不匹配、初始化失败或残留场景：关闭发布，保持隔离并按已解释顺序退休，不启动本地权威run |
| I2 全路径玩法分派 | 在统一Hook清单内接入Client条件，覆盖ST3实际路径 | 漏入口、重入、异常或上下文不明：停止受影响逻辑；观察wrapper仍保持original exactly-once，主动隔离的跳过行为单独明确 |
| I3 展示和回调归属 | 保留必要相机／HUD／render，隔离玩法Lua及动画事件消费者 | 展示路径仍有世界副作用或未知回调：改用明确适配或停止该路径，不能默认透传后声称安全 |
| I4 资源管理 | 以描述和代次管理资源缓存、未发布候选与已发布实例 | 缺失／不兼容资源、准备失败、预算耗尽：明确失败原因，不发布部分候选，不无限积压 |
| I5 姿态与图层 | 通过公开接口及必要专用适配应用main／overlay、层状态和外观 | 无效层／动画、姿态缺失、资源版本变化：拒绝不完整目标并撤销失效借用，不用本地Update猜出权威姿态 |
| I6 线程与Lua寿命 | 原生／Lua／图形操作只在已解释阶段执行，后台只持有自有值 | 错误线程、context不符、userdata失效、关闭中回调：取消候选、停止发布并按有效owner清理 |
| I7 持久隔离 | 在准备前建立访客原有进度／Continue归属；隔离服务器run、统计、解锁、成就、挑战、完成标记及Mod保存，保留具名本地设置 | 在官方整链副作用之前阻止错误来源的请求；下层write=false、readonly或dirty不代替caller处理。失败／断连／stop保持必要隔离至安全退休，Host合法保存不因Client规则被全局关闭 |
| I8 发布与退休协调 | prepare／commit／cancel／retire与15／16接线，错误向调用者明确返回 | 旧epoch／引用迟到、部分应用、stop在途：不确认半套状态、不恢复旧对象；先停止输入／发布和回调，再安全退休后回透传 |

所有Hook经唯一清单注册，启动安装后常驻至进程结束，会话仅逻辑启停；Off原样透传，不恢复目标字节或强卸载DLL。新增资源的正常退休遵循各自owner，不改变既有自有virtual device的NEVER回收策略。

实现时明确对象、资源、候选、队列和在途内存预算；游戏线程不等待网络或磁盘。正式运行不依赖调试录制、逐tick日志或测试夹具；核心函数、功能和数据结构使用中文注释。不编写实现后的单元测试。

## 6 全目标完成后的统一整体验收

统一入口在实现时接入现有E2E编排，以真实游戏Host／Client角色验证17新增行为。Rust只作编排或辅助工具，不能替代真实Client。场景参数、持续时间、预算、必要启动与用户操作在实施时按本tx范围明确，不机械沿用旧tx的250／32样本数或重跑全部历史矩阵。

| 场景 | 必须观察的结果 |
| --- | --- |
| E1 Host Client Off与连接等待 | Host正常权威模拟；Client无可展示世界时明确等待；Off保持原生行为；角色切换不遗留可被本地模拟接管的场景 |
| E2 普通 odd和特殊FSM | 实际触发相关路径；Client无本地AI、碰撞伤害、使用、拾取、掉落或玩法随机裁决；不能仅凭输入为零或包数判定 |
| E3 loading 暂停和恢复 | 必要资源、相机、HUD和展示仍按阶段工作；加载旁路不恢复玩法，旧输入边沿不补发 |
| E4 动画事件与Lua | 受控展示持续更新，实际包含动画事件和回调路径；不产生第二份实体、奖励或玩法副作用 |
| E5 资源和姿态 | 正常加载、main／overlay、独立层、外观、动态换图及资源重建显示正确，不以单个屏幕Sprite覆盖全部边界 |
| E6 失败候选与预算 | 缺资源、无效描述、部分准备失败及队列／预算耗尽明确处理；不发布或确认半套目标，不无限持有失败候选 |
| E7 失效借用与旧代次 | 资源／场景重建后重新获取借用；旧epoch、迟到姿态和无效引用不能复活旧对象或污染当前展示 |
| E8 线程 context与退休 | 记录实际阶段和必要运行条件；加载、换图、GC、stop与在途绘制不越过owner／context寿命；错误条件下停止相关操作 |
| E9 访客进度与保存 | 覆盖准备、加载、render／HUD、失败／断连／stop、返回菜单及正常进程退出的整个窗口；联合观察内存进度、平台通知、run写入／删除、备份、官方进度JSON／附加文件／PGD版本同步与Mod保存。访客原有Continue／进度归属不被服务器世界改变，允许的本地设置来源和字段明确；不以dirty=0、文件未变或write=false单独判定 |
| E10 stop 故障与正常退出 | 准备中／展示中／失败后均能退休并回透传；返回菜单、窗口正常退出及Lua／context清理无旧回调或资源使用 |

测试专用夹具只服务统一E2E，不进入发布模组；场景动作按已有授权执行。每次游戏启动前分别提示用户需要配合的操作及何时停止手动操作，需要手动配合时先确认已准备好再启动。不从本文推导新的存档、成就、持久写入或其他实机操作授权。

全目标实现完成后才进入上述统一验收。此前不运行smoke、probe、harness、自测、隔离试验或离线校验器，不把分项测试改名成静态核验。验收发现问题在同一tx修复并继续必要测试，覆盖修复影响和未完成场景；不因一次失败默认停下等新指令，也不为续测更换tx。

tx和E2E默认不留artifact，正常临时材料用后清理。仅用户叫停、缺必要配合／授权或无法自行解决的阻塞而需要中断恢复时，保留必要诊断与进度并说明位置和用途。保留实际失败与修复事实，最终报告覆盖、未执行、未完成和运行边界，不以文件清理或目录存在判断功能通过。

## 7 执行顺序与工作量

1. **先行静态阶段**：按第4节开展资料分析和必要J273核验，逐项续记结论与运行未知；不要求15／16先完成，不进行分项测试或分项验收。
2. **模型与角色衔接**：将资源、姿态、引用、错误和寿命需求反馈15，将最小场景、主线程阶段、启停和失败隔离需求反馈16；根据它们实际交付修订接线。
3. **完整实现阶段**：完成I1—I8及统一E2E编排，处理已知必需缺口和失败方式；不提前运行功能测试。
4. **统一整体验收**：通过一个入口覆盖E1—E10，连续定位、修复和续测直至本tx完整目标完成；确有必要中断时按现行规范说明继续条件。
5. **结果与后续接入**：在本文或实际实现说明中记录完整行为、失败修复及未决项，向18—20／21—22／27／31提供公共合同。研究完成、资源加载成功和单场景成功分别保留其实际范围，不能替代整个17完成。

当前工程量估算沿用会话评估，按熟悉项目的单人有效工程量计：

| 部分 | 估算 |
| --- | --- |
| 既有研究复用 公开接口与源码核对 必要静态逆向 | 3—6人日，可先行 |
| 玩法隔离 资源与生命周期完整实现 | 5—8人日，随15／16接线 |
| 统一E2E与修复续测 | 2—4人日 |
| 合计 | 10—18人日 |

估算包含正常研究、实现与修复，不包含15／16工作或设备／人工等待；ST2／ST3发现新的必需引擎适配时据实调整。先行研究将风险提前暴露，不表示总工程量自动减少，也不是工期上限或完成承诺。
