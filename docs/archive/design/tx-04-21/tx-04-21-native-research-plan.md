# tx-04-21：静态研究与必要逆向计划

日期：2026-11-05。状态：**R01—R09采用合同已归并，R02必要J273逆向经主审；本轮I01—I05未构建或运行测试**。结果见[采用合同](tx-04-21-native-contracts.md)与[基础实现](tx-04-21-foundation.md)。以下保留初版研究问题和后续实际采用责任；具体UNKNOWN没有升级为可执行资格。对应[主实施计划](tx-04-21-implementation-plan.md)。

## 1. 研究方法和采用要求

先查原版 Wiki `F:/projects_isaac/IsaacDocs`、REPENTOGON Wiki `F:/projects_isaac/REPENTOGON-1.1.3/docs/docs`，再查官方 wrapper／SDK、现有目标版本研究和本项目源码。明确的公开 Getter 不要求全部重新逆向；纯协议、预算、zstd、队列、时序算法不需要游戏逆向。仅对缺少解释的私有读写、实际调用阶段、资源寿命和副作用补 J273 静态反编译／指令／调用关系。

旧 J460 点位仅作检索入口，采用前核对目标 J273 的入口、原始指令、ABI 与布局，不校验哈希。静态确认不等于运行观察；UNKNOWN 必须保留，不能用后续 E2E“试试看”代替未知写入合同。

采用的每条 native 合同登记：对象／字段／用途、公开或 native 入口、VA／RVA 与基址口径、类型宽度／返回／栈／寄存器、主线程／context、可读依据、写入依赖、关联字段、可能副作用、失败前缀、正常退休、来源与不确定性、对应 I／E。结果写进功能设计／发送字段目录，不为每个 R 建 artifact 包。没有强制子智能体、独立作者或单独试验；所有运行验证归 I39。

## 2. 逐项研究任务

### R01：实时字段、完整快照和实际消费者清单

**类型：必做源码／目录分析；只有新发现的私有消费者缺口才补逆向。对应 I01、I03、I05—I07、E02／E05／E09／E18。**

已有基础：15 的归属目录、18 Room／Grid target、19 Entity target、20 Player source／target v2，以及 `world/common/contracts.*` 的来源／阶段。20 的发送投影并不是现成的紧凑实时 schema。

具体工作：

1. 从 `ClientScene`、Room／Grid／Entity／Player 安装器、draw 和只读 HUD 视图逐个列出实际消费字段，追到正式来源；包含每个 costume／Weapon／子 ANM 的动态输入。
2. 对每项给出 Baseline／Context／Structure／Resource／WorldDynamic／PlayerFast／AuthorityOnly／后续表现归属，明确更新单位、Null／Empty／NotApplicable 和参数语义。
3. 列出同一个值的规范 owner，特别是 Player Entity 公共字段与 Player 域关系，禁止两路不同版本反复覆盖。
4. 给每个低频值列出失效来源与其覆盖范围；没有可靠失效信息时选有界读取，不假设值静态。
5. 交付逐成员目录及缺口表；缺少当次消费者必需值仍归 21 修复，不靠缩小支持 profile 放行。

### R02：ordinary／odd 后逐身体 PlayerPost 来源

**类型：必做现有研究／源码归并；若现有合同不能解释全部采用分支，必须定点补逆向。对应 I08、I18、I23，E03／E04／E08／E18。**

已有基础：`game/tick.cpp::{BeginOddPlayer,EndManager}`、`WorldPostState/WorldPublishPostState`、09 控制组、20 实际 ordinary／odd consumer 缺席记录；公共枚举有 PlayerPost，但 Entity／Player 的 SupportedPhases 当前未开放。

具体核对：

1. 普通更新、odd helper、主／副／非 roster 身体各在哪个正常返回边界完成；回调顺序、outer token、actual PlayerUpdate 与 authority commit 的关系。
2. 已经完成更新的每个身体如何取得同 use／shape／binding 的只读资格，异常、死亡撤绑、Init／Restore／Replace、pause／menu 如何撤销；不能用首身体成功外推全部身体。
3. 位置／姿态等能否在当前边界读取；不需要全部 Player 图的窄字段是否有公开语义或现有核定布局。不得为补帧调用 Update／EvaluateItems／射击函数。
4. PlayerPost 若在 Authority 提交之前取得，只作为候选；外层异常／提交取消必须撤销。若在提交后读取，必须说明对象／generation 仍稳定的窗口。
5. 将结果整理为“身体类别→实际更新阶段→可读字段→提交／撤销条件”表；原生未执行的身体记录实际原因，不能报告它新更新了 60 次。

需要逆向时仅追对应 J273 normal／odd 调用链、所采用 body 分支和写入字段；不重新研究原版 Online，不要求运行探针。阶段不足则先补合同／实现，不能把旧普通样本贴 PlayerPost 标签。

### R03：动态字段、结构变化与资源不变性的边界

**类型：必做 source／applier／draw 对照；新增私有读取或变化关系不明确时补逆向。对应 I09、I23、I24，E05／E07／E09／E22。**

已有基础：20 的最终量／装备／角色／draw 研究，19 的 Entity 资源／姿态合同，17 的资源 owner、context 和 Lua 借用。

具体核对：

1. 哪些是每步改变的精确动画 selector／frame／overlay／layer transform，哪些是资源定义；不能把层实例变换作为静态 ANM 数据缓存。
2. Weapon charge／delay、Player 闪烁／角色计数／特殊 draw、Laser 曲线／端点、NPC 阶段／部件、Grid 动画等每步实际消费者需要哪些 backing 值。
3. 库存／costume／Weapon type／shape／held／twin／parent 变化怎样使动态安装布局失效；跨 Player 与 Entity 的同窗关系哪些必须联合提交。
4. 对已有 resources unchanged 路径核对确切相等条件和原生实例有效性；使用字段值、显式版本和 owner，不引入哈希或泛型子树去重。
5. 输出每类“可以快速改值／必须结构事务／必须完整重建”的分类以及具体失效条件。现有字段若被完整 target 解析器读取，不能只从 wire 删除后假设原生仍正常。

### R04：同一原生世界的高频安装与绘制安全

**类型：必做现有 writer／owner 静态审阅；采用新写入顺序、布局或缓存时必须必要逆向。对应 I23、I24，E03／E05／E09／E15／E21。**

已有基础：`ClientScene::AdmitTarget/Pump/Result`、各 `*_state_native.cpp`、Player／Entity draw、资源 `Retain/DrainRetained`。当前每轮完整子步骤与完整 DOM 解析不能直接证明每帧可更新。

具体核对：

1. 将纯值 schema 检查／计划与原生最终值安装分开；预解析结果仅含自有值和稳定身份，不把主线程解析出的指针交后台。
2. 每组快速写入的最小完整步骤、字段联动、renderer／Lua 消费时点；两步中间发生 render 时如何防止半套画面。
3. 已验证的小步骤能否在一次 Manager 窗口内按时间／步骤预算批量完成；不得在原生调用栈中任意暂停，也不得异步 worker 写游戏对象。
4. 动态更新不应每帧调用资源加载／构造／析构；确有资源变化走 R03 的结构合同。无资源变化时原有 borrow／instance／context 资格仍需检查。
5. 写前失败保留有效 A；写后异常失去 A 的展示资格，按实际前缀退休；只完成一个子范围不得升级整场景 Applied ACK。

本项不以“直接 memcpy 更快”代替布局和生命周期理解，不放开 Manager／Game／Player gameplay 原链，不新增预测或插值。

### R05：实时在途取消、失败恢复和访客退休

**类型：必做既有生命周期合同归并；只有新发现的原生截止点／清理副作用补逆向。对应 I21、I24、I26、I28、I36，E14—E16／E19／E22。**

核对 17 的 scene／resource／Lua／持久退休，18—20 实际失败前缀和 20 的一次完整 T 恢复。新增网络队列／压缩任务／候选／ACK owner 不得延长原生对象借用。

需要解释：用户 stop、Host stop、Peer timeout、程序关闭分别怎样撤输入和新候选；主线程退出前哪些资源必须退尽；网络线程停止不等于原生世界退出；准备／部分应用／显示中取消如何复用实际前缀；同连接恢复保持哪些逻辑身份、哪些本地实例必须换代；失败 owner 无法安全释放时怎样保持隔离并报告，不能猜测析构或盲目交回 Off。

访客准备至退出覆盖保存、统计、成就、解锁、REPENTOGON 附加保存与 Mod 写入。不能只比较最终文件；新调用如果没有旧合同覆盖，须在使用前补读源码或必要逆向。

### R06：低频 Context 的失效来源与正式采集成本

**类型：必做来源／官方回调／已核 owner 分析；无法确认具体可变字段 writer 时定点逆向。对应 I05—I07、I09、E02／E09／E18／E21。**

具体工作：逐一列 floor descriptors、room_save、房间几何、探索、库存、资源和 HUD 必需值的变更入口；区分“不能在本 tx 跨房”与“当前房间内仍可能变化”。回调缺失、来源 loss、mutation、generation、Lua state／资源 reload 立即撤缓存资格；完整采集缺口不可只改成 NotScheduled。

核对 Service 的 interval 是否被实际来源执行、所有消费者是否触发重复采集、正式观测是否仍经完整 Lua JSON 返回热路径。确定 typed 快速只读采集与完整观测目录的共用语义，避免每 tick 扫全域再丢弃 99% 字段。背景线程只能整理已经复制的值，不能把扫描游戏对象搬到后台。

不可证明低频缓存有效的字段应进入受预算约束的刷新／动态采集；若同窗完整性无法维持就显式 gap，不伪造混合快照。采用 dirty 通知不意味着取消运行中的必要稳定性检查。

### R07：输入时序、低频名单和高频进度分离

**类型：必做本项目纯源码分析，通常不需要 native 逆向。对应 I18、I21，E03／E08／E12／E14。**

核对 `InputTiming::Observe/Target`、`ClientManagerBoundary` 的 progress／last_tick、`HostPeer::Progress`、worker 最新 Ack 合并和 Input history。设计 TickProgress 与 InputAck 如何保留服务器采样／发送与 RTT 回显，消除 roster 20 Hz 对新输入的限制；startup 尚无 processed 时不能伪造 InputAck 来启动时钟。

保留 lead=2、age=0、上限、先前已采纳的启动／重绑例外和 exactly-once；逐身体原生消费不足由 R02 解释，不能调大旧输入保留时间掩盖。若来源已具有效时序合同，不重复逆向原生键盘消息泵。

### R08：原版交互的持续可见结果与隔离

**类型：必做场景／consumer 静态确认；仅为选定 E2E 基本场景的未解释原生路径补逆向。对应 I23、I24、I32、I36，E05—E07／E19。**

为普通眼泪／NPC 受击死亡／拾取消失及资源变化／炸弹与 Grid 损坏／一种已有 Laser 或 Knife／换装建立“Host 裁决→正式来源→完整范围→原生安装→画面”表。自然生成／短命实体跨普通采样可能消失，须区分不持久表现与最终世界状态；不能要求 Client 重演发射来补出遗漏。

本次已有 adapter 必需的表现字段或阶段缺失须补齐；27 的独立音效、文本、全 HUD 和 29 的全部组合仍保留其责任。不能将本次基础战斗出现的实际错误全部归到后续任务。无具体需求不展开全武器／全 Boss 逆向。

### R09：网络、压缩、队列与确认状态机

**类型：必做纯工程／源码设计，不需要游戏逆向。对应 I04、I11—I17、I20、I25、I27，E10—E17／E21。**

初版以旧v1 ICRF 32字节头、ModelWindow和ControlWindow为阅读入口；本轮已新增ICSR v2逻辑40字节头、ICRF v2 64字节头、严格范围schema与Live小消息适配。继续按采用合同核对 Reassembler、旧ModelWindow双基线／seen、控制序号与Host／Client socket owner，不能把旧头口径用于当前v2。明确新版路由在分配前的检查、可靠消息取消、字节／节点／解压／历史预算、每 Peer 与全局公平、ACK 丢失、最后静止后态以及错误收口。

zstd 的 C API、窗口参数、错误值、依赖许可证／发布方式在实际引入时查所采用版本的官方头文件和说明，不凭本文假装已有依赖或选定库版本。scope／schema 使用显式版本，不新增哈希校验。本轮不下载依赖、构建或运行 codec 验证。

## 3. 输出与未知的处理

每项 R 的结果写明“复用已有核定事实”“本次源码确认”“本次必要逆向”“静态推导”“UNKNOWN”，并列采用函数／字段与 I／E。若确认无需新增 native 入口，直接说明依据，不为完成研究编号虚构新逆向。

R02、R03、R04 是本次最可能需要追加 J273 定点逆向的部分；R05、R06、R08 按具体缺口补证；R01 以目录为主，R07／R09 原则上是纯工程分析。全部运行现象和正确性验证仍统一归 I39，不提前启动游戏、probe、harness、构建测试或孤立测试。
