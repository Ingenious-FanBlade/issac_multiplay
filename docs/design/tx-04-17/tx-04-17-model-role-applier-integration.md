# tx-04-17 R6：模型、角色与原生应用器接线归并

日期：2026-11-01，Asia/Singapore。目标为官方 REPENTOGON 1.1.3／Repentance+ v1.9.7.12.J273。同属 tx-04-17，本轮对照 15复制合同（已删除，原文 `39ddcfd6^:docs/design/tx-04-15.md`）、16真实Client（已删除，原文 `39ddcfd6^:docs/design/tx-04-16.md`）和现有源码，归并 [R1初始化](tx-04-17-client-scene-lifecycle.md#r1-results)、[R2对象生命周期](tx-04-17-native-object-lifecycle.md)、[R3维护绘制](tx-04-17-native-maintenance-rendering.md)、[R4资源寿命](tx-04-17-native-resource-lifetime.md)、[R5失败退休](tx-04-17-failure-retirement.md)。只更新设计文档，没有修改代码、绑定、Hook清单、游戏文件或IDB，没有构建、测试、游戏操作或artifact包。本轮没有新增需要逆向解释的native事实，不另开逆向子任务。

`SOURCE`表示当前工程直接事实；`DESIGN`表示待实现的接线合同和接口职责，名称不代表已有类／函数；`OPEN`表示尚未落实的具体原生或表达缺口。R1—R5的静态证据、主审纠错和UNKNOWN保持，不因本次归并改为运行通过。17提供公共机制及实际代表操作，18—20补完整类别应用，21—22接实时状态传输与ACK／恢复，23以后扩展整局；原生世界、代表操作和完整可玩同步分别报告。

## 1 当前源码可以复用什么

路径相对于仓库根；这里只阅读，不执行生成器、合同工具或验收脚本。

| 已有入口 | `SOURCE`：实际行为与限制 | R6接线取舍 |
| --- | --- | --- |
| `mod/src/replica/model.h`、`model.cpp:85,222—325` | Scope有session／connection／run／floor／dimension／room／scene；key为`(run,space,id)`，session取自Scope。CaptureRows核对11域普通同窗口，Registry投影身份，Difference／Reconstruct处理component最终值，Applied要求调用者传入native_committed=true | 复用纯值模型和身份／值状态语义；不能把传true的普通bool当原生提交证明。由主线程协调器产生真实提交结果后才允许ACK适配器调用Applied |
| `replica/codec.cpp:143—163`、`codec.h`的ModelWindow | Stage验证并重建目标，把Current换成最新接纳DTO，保留两份baseline和有限已见编码；它没有原生对象、应用进度、回滚或CommitApplied接口 | Current是目标T，不是原生A。跨帧应用保留有owner的不可变目标；不能持有Current内部Value指针跨下一次Stage，也不能把Stage false当原生无需恢复 |
| `codec.cpp:164—179`的EventWindow／ControlWindow | Event Accept在Ready时即记入played；Control Accept在接纳时即递增next并保存历史。两者均不执行原生效果／操作，也不提供执行后的事务提交接口 | 区分接纳与效果完成；采用有界待执行记录和明确执行结果，不能用这两个返回值直接宣称已播放／已退休／已应用 |
| `net/live.h`、`live.cpp:70—74,92—115,169—241` | 16使用64KiB／4096节点／24层／64片的小控制预算；跨线程Event只有Welcome／Progress／InputAck／Stopped／Failed。等待Scope使用`{session,new_connection,1,1,1,1,1}`，不是正式run；客户端固定校验完整Scope，世界消息到达会报live_unexpected_world_message | 保留连接／物理输入／控制收束。不得直接给现有Event塞原生指针或世界大对象，不把占位代次用于世界；世界接收／Scope转换／预算扩展是新增接线，当前尚无 |
| `game/client.cpp:14—80,95—211` | 独立ClientRuntime保存连接、ordinal、输入代次／时序和边沿FIFO；不创建Host SessionRuntime／TickPipeline。状态只有Off、Connecting、AwaitingWorld、Retiring、Failed | 在此独立运行时接入场景协调职责，不为了共享枚举启动本地Authority时钟。场景状态与网络连接状态分开，新增名称不是当前枚举已存在 |
| `gen_binding.py:532—537`、`ops/command.cpp` | FrameScope在ClientManagerBoundary之前；ClientBusy期间先建立自有context，完整Manager原链被短路。原生Manager在ExecuteStartGame之前就被接管；物理泵另由ClientPollInput保留 | 复用唯一主线程泵，扩展准备／应用／维护／退休，不放开整个Manager。停止请求和旧排队启动命令在恢复原链前处理 |
| `game/world_bridge.h`、`world/common/contracts.h` | 正式观测和录制分离，主线程复制为自有值；主体epoch、采集scope、phase与wire Scope有不同含义 | Authority的正式consumer按15取得同窗口值；不从录制文件给产品同步供数，也不将多个latest域无条件拼成完整世界。Client应用不创建第二个Authority采集／模拟流程 |
| `game/client.cpp:171—178,259—263`、`hooks/exit_patch.cpp` | 当前退休仅worker／两轮中性／MenuReady，退出决定仅等待worker；退出patch安装仍与WorldObservationArmed／World能力关联 | 按R5增加原生owner／资源／持久结果，进程关闭接线独立于调试录制。保留16既有成功与暂缓记录，不把它们扩成世界退休已通过 |
| `mod/binding/hooks.json`与生成wrapper | 唯一清单有owner、优先级、适用入口；现有观测包装仍调用原链。清单存在不证明全部Client分派／官方Hook层次已经安装正确 | 17按用途扩展唯一协调owner，Hook启动安装后常驻，Host／Off原样透传；不通过卸Hook或临时换字节启停Client |

## 2 五种状态所有权与身份边界

以下为`DESIGN`。同一目标只在自有值中重建，在同一个原生Game世界应用，不构造并行的第二套运行世界。

| 数据 | owner／寿命 | 不能混用 |
| --- | --- | --- |
| 不可变baseline B及解码窗口 | 每连接／正式世界Scope的纯值接收服务；有界保留，缺B时请求完整恢复 | 不在原生世界上patch Delta，不把即将淘汰的B内部Value借给跨帧计划 |
| 完整目标T | 从Baseline或Reconstruct(B,D)得到的自有不可变DTO，持有到本次计划提交／取消并退尽使用 | 不把T的版本称已应用；较新T到达不修改正在执行计划的allocator／字段 |
| 最后完整应用A | 主线程应用协调器持有纯值及提交结果，仅正常原生提交后更新 | 不与ModelWindow::Current别名；部分写入后A仍是历史记录，但其“当前原生可按A差分”资格失效 |
| 原生实例／成员／资源表 | 主线程，按本地scene、pool-use／instance、容器、resource、Lua及context代次管理 | 同key不保证同实例，同地址不复活旧use；wire scene不替代本地resource／context generation |
| 计划／在途／退休记录 | 唯一主线程执行owner；保存实际前缀、作用域、依赖与释放责任；纯值部分可后台准备 | 不把网络线程、DTO析构、普通bool或一条ACK当资源owner；不把已移除对象的待释放记录继续当存活目标 |

session／connection是路由归属，run与key身份空间属于服务器逻辑世界；floor／dimension／room／scene限定状态适用范围。Participant、控制组和EntityPlayer分别映射：ordinal冻结于连接，身体共用Entity的网络key，身体更换不挪参与者身份。本地Player*、设备key／指针、控制器index、观测local/use均不进入wire身份。

观测epoch不是wire scene，source capture scope不是消息Scope；15的Registry只负责从已登记来源投影稳定key，不是Client本地实例注册表。新run换Registry，跨房存活对象仍按真实逻辑key解析；仅退出Room membership不等同服务器死亡。所有引用都在使用点确认其状态与当前实例代次，不能把KnownHistorical、Retired或有target字段的Unresolved当可解引用的存活对象。

`SOURCE`的Registry::BindSource还冻结source_epoch／source_run；同逻辑run中更换正式观测来源epoch并不是现成支持的重新绑定。不能随录制重启或Room变化就丢Registry并给全部对象换网络key。正式consumer保持独立寿命，确需更换源时由Authority身份／恢复方案说明迁移或重新建立世界的语义；现有15接口没有自动保持旧逻辑ID的迁移方法，此缺口交21—26在实际采用时落实。

## 3 从B→T到A→T的应用计划

### 3.1 目标接入与完整性

1. 接收服务先完成报文／分片／预算／连接及Scope校验，15 Stage生成完整T；部分包、缺B、同版本冲突、不可publish的诊断模型不进入原生计划。Stage或ValidateModel不证明每类资源、引用和写回语义已适配。
2. 目标接入对照实际native_applier能力、每域ownership／coverage／membership_complete、必需字段与值状态、引用依赖、资源和模式支持。AuthorityOnly随机内部状态不写Client RNG；ClientLocal只接受明确本地来源；未知必需ReplicatedState不能静默略过后报告完整提交。
3. 为计划冻结目标、适用Scope、版本、逻辑key集合、依赖和预算。A有效且当前实例表一致时按A→T比较；无A或A资格失效时走受控准备／全目标重建，不直接在未知部分后态上继续diff。

当前模型11域中Random为AuthorityOnly占位，publishable不代表11域全都要写游戏对象；同样，字段目录的ReplicatedState归属不自动赋予setter或native写权限。具体归一化和逐类handler由18—20及后续对应域解释，17先实现它所用代表路径的同一正式接口。

### 3.2 三类操作与字段语义

| 规划操作 | 输入和必须保留的语义 | 执行与提交 |
| --- | --- | --- |
| create | T明确增加逻辑对象，或声明的新scope需实体化既有逻辑对象；从服务器目标取类别／seed／初值／资源／关系 | R1／R2对应工厂或专用Player／Grid owner受控构造，原生成员可能早于本方提交可见；其间停相关draw。连带对象先分类／关联，不重复create或先生成再清理 |
| remove | 完整成员集合明确缺席或显式删除；成员退出、对象死亡和组件缺席分开 | 先停止新使用，按R2／R5处理关系、成员和owner退休，逻辑摘除与物理释放分别记录。不重复调用Remove期待幂等，不以Kill／奖励替代 |
| change | 同key的最终字段、关系、资源、姿态、形态／membership变化 | 原位变更或必要时换本地实例，保留key并递增相应本地代次；Morph／Load／容器变化先撤销旧借用，不伪造wire remove＋create |

15的Delta `replace/remove`是component编码，不是逐对象RPC。对于一个重建后的T，若A中HP=7、B／T中HP=10，即使Delta未单独列HP仍需应用10；不能直接把Delta replace项当原生操作清单。当前component整体replace也不等于给所有字段调一遍setter：按A→T语义及本地失效点规划必要操作，重复目标不重播奖励、声音、物品效果或创建。

字段缺席、显式Null／Empty、Unknown／Unavailable／NotCaptured／ReadFailed／Nonfinite／Truncated和Deleted保持不同。显式空关系可清除，缺失必需字段是错误或表达缺口，不自动保留旧值／补零；无效引用状态不降成Null。部分枚举缺席不能产生删除。即使DTO coverage声称Complete，adapter仍核对本类型实际所需成员／字段，不把schema完整当游戏语义完整。

### 3.3 依赖、循环引用和原生提交

计划先以纯值建立依赖图，区分逻辑key、原生对象存在、资源就绪、容器可用、关系可写和显示缓存的依赖。对可安全分两阶段的循环引用，先构造受控壳和映射，再在整个引用组就绪后统一修复；没有合法空关系／分步Init合同的环不靠临时野指针解开，而是明确UnsupportedDependency并停该目标。暂缺依赖有界等待，缺失或超预算进入恢复／失败，不填造对象。

顺序遵守R1—R5实际owner：Grid占槽冲突可能先退休旧槽，Player清理需要有效当前Room，Room／Camera变化会撤销相关借用；不硬排“所有create先于所有remove”。已提交映射和本次待发布映射分开管理，原生早发布成员不获得本方draw／提交资格；不额外复制完整运行世界来实现事务。

一次native计划可跨多个允许的主线程窗口推进，但仅在已解释的完整子步骤之间让出，不在任意原生call栈中暂停。进入破坏性步骤前登记已尝试，正常返回后才登记已完成；同一scope不得重入执行第二批。跨帧准备可保留仍完全有效的旧展示，一旦开始影响旧owner的写入／析构便停受影响展示；未核定依赖隔离时停整个该scene，而不是绘制混合版本。

提交要求：当前连接／Scope／attempt未被取消，所需原生对象／成员／关系、资源姿态和R3必要维护均完成，官方结构登记／玩法与持久隔离完整，显示资格与目标一致。满足后主线程一次性发布提交结果并令A=T；尚有独立物理退休时记录其有界责任，只有已无可访问旧引用且保留安全才可与逻辑提交分开。资源loaded、sort完成、DTO Stage和Render返回都不能独立产生成功结果，亦不把提交称GPU idle或像素已验收。

## 4 版本、取消与恢复

| 情况 | 协调行为（`DESIGN`） |
| --- | --- |
| 同版本重复且A／实例／资源资格均有效 | DTO窗口按已有规则识别；应用协调器返回已有提交结果，不再执行原生操作或事件；21—22可重发对应真实ACK |
| 旧版本或提交历史已淘汰 | 不把旧T重新写入世界；仅重发仍留存且适用的真实结果或报告当前进度，不能为回答旧包补造提交记录。窗口内冲突按15拒绝，有限历史不宣传成永久保存所有版本 |
| Stage返回false，但尚未原生应用或资格失效 | 不丢失当前完整T；按本地attempt状态继续或受控重建。不能以重复DTO为由永远跳过失败恢复 |
| 较新T到达，旧计划未写世界 | 取消旧候选并退尽其资源后可合并到新T；保留B接纳和同版本冲突记录，预算不能因合并绕过 |
| 较新T到达，旧计划已写部分原生状态 | 当前计划先停在完整子步骤并按R5处理实际前缀；可完成旧T后再处理新T，或停显／清理后从新完整T重建。不能直接把旧计划后半替换为新字段；取消不发送旧T成功ACK |
| 旧Scope／旧attempt结果迟到 | 仅按其真实owner完成必要退休，不应用、不发布到新场景，不让“资源准备成功”复活旧key／旧事件 |
| 缺B／缺控制前序／缺引用依赖 | 保留有界纯值等待或请求新Baseline／控制恢复；不运行本地模拟补状态，不让Authority等所有Peer |
| 部分写入／异常／不可恢复owner | A的当前原生差分资格撤销，保留历史A与实际前缀；停显，R5安全清理后从完整T重建。清理不明时维持Client隔离并报具体剩余owner，不循环重跑helper或假称回滚 |
| 重连／新run／切scene | 经明确转换创建新的Scope窗口及本地scene attempt，旧窗口不能用来接纳新scene。持久对象按服务器key重新关联而不是复活旧指针；重连换connection并清输入旧代次 |

ModelWindow的两份baseline、Current副本、seen编码、正在应用T、历史A、收包候选／排队目标和退休资源分别计费；源码注释的“两份／64MiB”不作为整体内存上限证明。网络到主线程每次只转交有界自有值／不可变blob及其owner，不借RapidJSON旧allocator。限制总字节、对象／节点、计划步骤、候选与退休数量；预算到限停止新准备并显式恢复／失败，不无限排队、强放仍在途资源或确认已丢弃目标。

## 5 角色、连接与场景分开接线

角色继续为Host／Client／Off；原SessionMode::None／Solo／Host与独立ClientRuntime的现状保持，17不为了统一名称把Client塞进Host TickPipeline。连接可为协商／存活／断开，场景职责则是等待、准备、应用、展示、恢复、退休或失败；以下均为待实现语义，不冒充已有enum。

| 场景职责 | 入口／允许工作 | 退出与限制 |
| --- | --- | --- |
| 等待／连接 | 仅稳定菜单入口，保留16唯一设备物理泵、UI和网络协商；没有世界时明确等待 | 不用等待Scope的1值伪造服务器run，未声明applier能力不展示一个同seed本地局 |
| 准备 | 正式Scope／完整目标就绪后启用R1受控原生编排，持久与玩法隔离先于首次写入 | 不放行完整Start／Restore／Room Init；资源／Player／Room前置缺失报错，不载访客Continue |
| 应用 | 主线程推进本次T计划、关系与资源／缓存维护；物理消息泵仍可服务窗口／控制器 | 不推进本地Authority tick，不在Room／Player consumer直接消费物理输入，不以原生gameplay补字段 |
| 展示 | R3已提交版本的原生pass／draw与独立展示时间；按目标提供Camera／HUD／姿态 | odd／FSM／loading不恢复整条模拟；焦点／UI／暂停只改变输入资格与允许展示，不给玩法执行例外 |
| 恢复 | 关闭受影响draw和新应用，等待／请求完整目标，在R5有效owner内退休／重建 | 连接活着不表示世界健康；重建不创建第二套Game，不复用旧edge或本地随机结果 |
| 退休／失败 | 输入及publication关闭，按R5处理在途／owner／资源与持久截止点 | worker结束、MenuReady、run位false或两轮松键均不足以Off。安全菜单后才交还；进程退出不要求先回菜单或转Off |

输入继续沿16的canonical、edge FIFO、服务器进度时序和Host统一scheduler，不因本地可绘就获得输入资格。应用／重绑／恢复期间按明确控制组绑定、服务器input_enabled／generation及本地可操作状态判定；不让本地InputTiming::Target估计充当世界／事件的Authority时钟。相反方向后按优先、旧代次中性、真实松键与UI屏蔽等既有语义不变；新增场景状态必须接入采样资格，不能只把现有`AwaitingWorld`判断改名后继续全阶段发边沿。

## 6 主线程和网络的具体交接位置

| 位置／职责 | 应接的行为及数据 | 失败与owner约束 |
| --- | --- | --- |
| Authority正式值consumer→15 | 从同窗口自有ObservationEnvelope构建模型／Registry，发布不可变状态给每Peer发送服务；不依赖Recorder／日志 | 不在网络线程补调Getter；同epoch／窗口／tick不满足时不冒充完整模型，不把latest独立scope拼接 |
| 16网络worker→目标接入 | 保留唯一socket／连接owner和输入优先服务；新增世界纯值接收记录，明确kind、完整Scope、revision及所占预算 | 现有SPSC Event无世界payload；新增容器／队列必须拥有数据，不携带Player／Lua／GL引用。反压仅影响该Peer，慢Peer不阻塞Host |
| FrameScope→ClientManagerBoundary | 端口／本地停止与退出优先撤销新工作，再消费有界通知、更新输入、推进准备／应用／退休完整子步骤 | 当前FrameScope早于Client边界，旧start命令、外层重入和foreign thread必须在原生写之前处理。`BeginManager`／Authority collector不能为Client恢复本地tick |
| 独立render／loading owner | 在R3实际采用pass前只读取已发布的一致场景与资源代次，准备阶段不绘半成品；按R4检查context | Manager Update被短路不意味着Manager Render也安全；未知官方Lua／gameplay消费者不透传，原链观察与主动跳过分别记录 |
| 结果→21／22 | 提供不可变真实提交／失败结果及scope／attempt，不直接由worker猜native_committed | 15 Applied只是编码辅助；主线程结果经scope与连接复核后才生成Applied ACK，排队失败可重试ACK而不重跑原生apply |
| Stop／Shutdown／常驻清单 | 按R5在有效主线程处理在途，保护覆盖Shutdown直接写、Game／Manager／Lua／renderer截止点；接线独立于录制 | Host／Off透传，Client按用途分派；停止／失败不卸Hook，不在DllMain等待，不因网络对象销毁撤销持久归属策略 |

### 6.1 等待Scope到正式世界Scope

`SOURCE`：live HostPeer分配等待Scope，客户端Receive要求后续消息与它完整相等；ModelWindow／EventWindow／ControlWindow也分别固定完整Scope。故“握手成功后把run／room字段覆盖掉”不是现有可用操作。

`DESIGN`：连接身份`(session,connection)`在同次连接内固定；世界Scope只能由Authority明确准备请求和完整目标建立，主线程记录待转换／当前／已退休Scope。利用既有Control `scene_prepare/scene_retire`的operation表达语义，转换描述须包含新的完整Scope、目标revision、资源／模式要求与适用旧Scope；具体parameters版本与可靠交付由21—23落实，当前16还不接纳这些operation，不宣称零改动即可传世界。新参数不得偷偷加到Baseline／Delta的严格顶层schema或改变既有key语义。

只为已核对且有界的待转换Scope接纳候选；当前提交Scope仍独立存在，转换计划完成后才发布新scene并退休旧window。旧Scope下有效的跨房逻辑对象可以保留key，但旧window、输入generation和事件水位不能凭地址或数值相同复活。scene_prepare本身不等于原生准备成功；取消／断连可在转换中进入R5，不能为了让新包过校验去暂时关闭整个Scope检查。17代表性目标入口可直接提供明确定义的正式Scope，不能因此冒称21—23的网络转换已实现。

`SOURCE`的`net::live::Intent`只有输入值／generation／sequence／target，worker发送时使用其当前Scope；切世界后不能沿用这一假设让旧队列命令借新Scope发送。`DESIGN`：待发输入明确携带采样提交时的完整Scope和input generation，worker只发送与当前已接纳路由一致的记录；切换前撤销旧输入资格、边沿和重发历史，主线程／worker分别确认新Scope及新的服务器进度后再恢复输入。InputAck和Progress也按所属Scope／generation验证，不能只在主线程换ModelWindow而保留网络worker旧scope。此项为纯值队列／路由接线扩展，不增加Input wire字段。

现有`net::live::Control(...)`辅助总是写`required_state=0`；未来scene操作若确有版本依赖，发送端必须提供正确参数／构造方式并由接收协调器处理依赖，不能因15存在ControlWindow类就声称16已有这条链。准备／退休请求的可靠序号、转换取消及独立于world的连接结束须一起定义。

16的64KiB小重组预算与15的32MiB模型上限不是同一配置。21—22应落实协商后的世界接收／总预算及控制／输入保留容量；不能把所有连接在Hello前就放大到世界预算，也不能继续用64KiB然后把大世界误记为不支持。ICRF片头没有kind／lane，不能假设先按消息kind选择大型重组器；重组前授权、transfer接纳和全包解码后的每kind预算需要明确设计，本轮只登记缺口，不修改头格式或现有Limits。

## 7 公共应用器和域adapter的接口合同

以下为`DESIGN`接口职责，不是新增代码声明或已经导出的API。主线程协调器统一拥有attempt、身份表、提交与退休；各域handler不能自己发ACK、切角色或独立创建网络线程。

| 职责 | 输入／输出 | 完成／失败含义 |
| --- | --- | --- |
| 接纳目标 | 不可变T、正式Scope／revision、能力及字节预算 → 接纳／重复／缺依赖／冲突结果和目标owner | 纯值成功，不触世界；不升级成Applied |
| 准备场景 | R1场景描述、T中必需Room／Player／资源参数、本地owner状态 → 准备进度／有效承载或失败前缀 | 区分尚未写、部分准备、有效空承载、可应用；前置UNKNOWN不猜调用 |
| 规划对象操作 | 有资格的A或明确重建模式、T、当前实例／资源表 → 有依赖、有预算的create／remove／change计划 | 计划是纯值语义加主线程解析，不包含跨线程可解引用native指针；不重做15的wire Delta |
| 推进域步骤 | 单次attempt、域／key、字段／关系／资源目标、用途和当前main phase → 未写等待／正常步骤／实际失败前缀 | 不可重入，必须报告原生成员提前发布、连带对象、需退休资源及是否破坏旧展示 |
| 提交目标 | 已完成计划、当前Scope／generation与各域结果 → 一份native提交结果及新A | 只有全部必需状态／资源／引用／维护就绪才成功；不把部分域完成冒充整个T |
| 维护／绘制 | 已提交A及本地显示时间、有效owner／context → 正常维护／draw或显示失效 | 不改变A的权威值，不推进Authority；显示失败需撤销相应资格并报告，不能回写本地变化为服务器状态 |
| 退休／恢复 | 停止原因、实际前缀、在途与资源表 → 逻辑退休进度／物理待释放／安全菜单／安全进程关闭或不可恢复 | R5决定可否Off；不能返回一个bool掩盖残留对象或假回滚 |

提交结果至少关联完整Scope、目标revision、local attempt／scene generation、实际提交的域／对象范围、是否完整native commit及当时的引用／资源资格；失败结果至少关联阶段、域／key或资源、原因、是否发生原生写入、旧展示是否仍有效、待退休owner及可执行的恢复方向。字符串错误来自固定分类和有界detail，日志失败不改变结果。

17代表性路径可报告明确范围的本地成功，但若T中仍有必需域／字段未应用，完整native_committed必须为false；该范围结果不能传给Applied(...,true)确认整个T。代表性目标应明确实际内容、能力和必需范围，所有采用路径都走正式机制；不能删掉不支持值、篡改coverage或把仅resource ready记成完整世界Applied来使17通过。完整实时全域T的确认仍由18—22实际完成后成立，不因此降低最终原版全内容目标。

拟采用的本地结果分类包括：WaitingDependency、DuplicateCommitted、StaleOrCancelled、UnsupportedRequirement、ResourceFailure、BudgetExhausted、ContextOrThreadFault、PartialNativeFailure、NativeCommitted、Retiring和UnrecoverableOwner；它们是语义清单，不伪造当前已有枚举。对外Error仍用15的`code/recoverable/detail`顶层，何种错误可请求Baseline、退scene或必须结束连接由21—22适配；recoverable只能在有可执行恢复路径时为真，不因为“理论上以后可修”而置真。

| 域／后续任务 | 使用17公共机制的责任 |
| --- | --- |
| 18 Room／Grid | 场景结构、descriptor／几何／slot／door、Grid原生owner与全部实际资源／状态；反馈R1所用最小可调用结构缺口 |
| 19 Entity | 每类别工厂／Init／派生draw／资源／引用／Morph与连带对象；池use／逻辑key不混同，不把effect一类成功外推所有实体 |
| 20 Player／控制组／多身体 | 首Player／roster／房间成员／HUD关联、身体替换、特殊资源与存活范围；多个身体共用参与者控制意图，不再新建Client scheduler |
| 21—22 传输／提交闭环 | 真正的状态发送、baseline引用保留、分片可靠恢复、Scope路由、Applied ACK和每Peer恢复；17提供真实结果，收包或Stage不替代 |
| 23—26 整局世界 | 跨房持久对象／membership、楼层探索／离房保存、规则与随机结果、换层重构；沿17依赖与失败责任扩展，不由Client重算玩法RNG |
| 27—29 HUD／Mod／完整内容 | 完整组合视觉、音画事件、声明Mod和原版全部必要内容，逐路径继承借用、唯一触发和持久隔离；特殊绘制不能丢弃后宣称覆盖 |
| 31 保存／Continue | Authority合法保存与访客原档归属；Client场景不写成访客run，后续合法Off／Host不被全局禁用 |

## 8 ACK、独立Component和事件的执行边界

| 结果／消息 | 真实含义及推进者 |
| --- | --- |
| ReceiveAck／重组完成 | 片段或完整纯值已到达；不改变A、事件水位或原生scene |
| ModelWindow Stage | 最新可用DTO T；可以先于资源／原生apply完成，不能据它发AppliedAck |
| InputAck | Host统一输入处理进度，不证明下行世界或某个Player在Client显示完整 |
| NativeCommitted结果 | 主线程实际完成本attempt的目标；21—22在当前连接／Scope仍匹配时编码AppliedAck，ACK重传不重复apply |
| AppliedAck.event_watermark | baseline抑制的历史事件和实际完成／明确过期等终态的消费边界；不得用收到的最大event_id、最新tick或排队成功替代 |
| 物理退休完成 | 旧owner／资源最后使用结束；与逻辑提交分开，有残留必须报告并限额，不能用它反向伪造前一目标已提交 |

独立Component保留自己的source_scope／phase／coverage／required_state，不补普通tick，不覆盖完整A或用于全成员删除。具名表现handler可在当前已提交版本及依赖成立时消费；未知handler／缺依赖显式等待或拒绝，不把一次Render／Lifecycle观察当全世界目标。要改变玩法存在性／最终状态仍须来自正式模型或明确可靠状态语义，不能把允许丢失的音画事件当对象create依据。

`SOURCE`的EventWindow::Accept(Ready)已经写played，而ControlWindow::Accept(true)已经推进接纳序号。本轮选择在其上层明确区分“已接纳／待执行”和“已完成效果／原生操作”的有界记录；事件执行需要预留与完成两个阶段，当前EventWindow没有这样的原生事务接口，后续17／27须补本地协调或适配，不能先调用Accept轮询Ready、再因context不符丢掉效果却宣称已播放。执行前确认资源／context和版本，保留同ID冲突校验；不确定是否已产生效果的异常禁止盲目重播，以实际前缀进入失败／退休。

具体本地接口语义为：只读检查返回依赖／到期／重复／冲突判断且不消费ID；主线程执行前预留`(Scope,event_id,参数原值,attempt)`，同ID不能重入；正常完成才标完成并推进可确认前缀，明确过期按终态处理，已尝试但效果不确定则标失败并禁止自动重播。等待资源或版本的记录保持未执行。实现可以在现有纯值窗口外维护这份协调记录或扩展其本地API，但不能把当前具有副作用的Accept伪称只读检查，也不需要给wire加native指针、回调或任意执行代码。

Baseline的event watermark在对应目标原生提交后才安装，用于抑制历史重播；Stage时不提前覆盖仍在显示的A的事件窗口。之后只有已声明事件序列中没有未决缺口的处理前缀才能推进确认水位，乱序已完成项另有界记录；不假设event_id数值必然逐一连续，也不把最大已见ID当累计确认。21—22须提供能判定前缀的交付／恢复语义；当前EventWindow自身没有补齐它。事件的server_tick／expires_tick使用当前有效的服务器时间依据，显示帧数和输入预计target_tick不代替Authority时间；缺时间／状态依赖保持等待或按显式过期规则处理。

可靠控制接纳与scene_prepare／retire等实际完成分开；控制按其序号／依赖留记录，不能重复原生操作。用户本地stop、网络关闭／失败与进程关闭立即撤销新工作，不被永远无法满足的世界依赖阻塞；非法／乱序报文仍按协议拒绝，不因退休而执行未经接纳的payload。21—22负责可靠重传及状态结果关联，本轮不新增完成确认opcode或通用函数RPC。

<a id="r6-open-items"></a>

## 9 17实现范围、统一验收接线与具体未决项

17代表性目标可以由测试专用入口提供已定义Scope和来自15的正式不可变模型；它必须经过同一目标接入、A→T计划、原生adapter、提交／失败与退休接口，不允许测试直接改对象代替公共applier。该入口不进入发布模组，不能用它宣称21—22真实实时下行已经完成，也不能借“传输在后续”略过17的真实Client角色与原生世界。

现有[主流程E1—E12](tx-04-17-workflow.md#7-全目标完成后的统一整体验收)保持一个全目标验收入口：E1／E10覆盖角色与R5收口，E2—E4覆盖R1／R3主动分派，E5／E7／E8覆盖R4资源与借用，E6覆盖实际前缀／预算，E9覆盖全过程持久归属，E11覆盖create／remove／change和实例换代，E12覆盖B→T／A→T、字段回到基线、缺失／清空、依赖／取消／提交结果。代表对象与采用路径列明，未采用类别保持18—20未完成；Host／Off与16已通过且不受影响项不重复测试。

| 具体缺口 | 必须落实的位置／影响 |
| --- | --- |
| R1完整结构／首Player可调用合同 | 17准备及所用18／20代表adapter；有效Room／几何／descriptor、构造所需实际输入仍需补，不能用本文状态名代替native前置 |
| R2／R3逐类Init／draw及官方Hook层次 | 17选定代表路径先落实，18—20扩到全类别；必要内部维护与gameplay／持久Lua分派不能只靠Client bool |
| R4／R5取消与安全菜单编排 | 全采用pass取消、LuaImage退休、Camera／Room失效窗口及官方Exit后处理桥接仍UNKNOWN；17必须解释实际采用前缀，不等到31或29兜底 |
| 模型到handler的语义映射 | 15提供通用值和resource_pose而非已归一化的全部native字段；缺少精确相位、具体资源／容器版本、特殊关系或必要姿态时反馈15／18—20，列来源、用途、缺失语义与版本影响，不写未知native key补值 |
| 网络／Scope／预算与事件完成记录 | 16现有worker／队列／等待Scope和小预算不支持世界；21—23补正式参数及路由，17／27补事件预留／完成与真实结果。本轮不改wire；未来若新增顶层必需字段／能力语义，须更新相应schema／协商版本，不能让旧端静默接受 |
| 原生提交与ACK接线 | 17落实不可变提交结果、A有效性和attempt；21—22消费后发ACK并恢复每Peer。仅调用Applied(...,true)不是实现证明 |
| 运行事实 | 安装／线程／context／像素／原生调用次数、正常与失败退出、全过程持久归属留完整实现后的统一E2E，本轮不运行任何试验或合同工具 |

R6归并不取消上述具体工作，不将“接口已写在文档”计作实现完成。后续按I1—I8实施，进入对应代码前先明确失败方式；全目标完成后才进入统一E2E，在同一tx连续修复与必要续测。所有新功能、核心函数和数据结构使用中文注释；生产采集／应用／输入不依赖录制、逐tick日志或测试夹具。

## 10 本轮静态审阅与交付

主智能体直接阅读15模型／Registry／codec／窗口、16网络／Client输入与生命周期、主线程context／FrameScope及统一清单来源，交叉核对R1—R5最终设计。审阅中特别分开Stage接纳与native提交、连接等待Scope与正式世界、事件／控制接纳与实际完成、原生逻辑提交与物理退休，以及17范围结果与完整世界ACK；未将纯值库的bool、接口名字或已有E2E结果外推为新功能已实现。

本轮已同步15／16交接说明、17主流程／接口目录、R1—R5相关入口、状态同步设计／路线图与docs／hooks入口。完成统一人工文档审阅，只核对引用、职责、源码口径、修订范围和历史状态保留；没有运行格式／链接校验器、生成器、合同工具、构建或任何测试，不保存额外artifact。R1—R6指定范围的文档结果已形成，具体OPEN／UNKNOWN仍按第9节和前项原表处理，tx-04-17运行功能及整体验收未完成。
