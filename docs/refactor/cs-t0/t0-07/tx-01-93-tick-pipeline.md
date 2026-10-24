# tx-01-93：T0-07 操作5，Active tick执行顺序

2026-10-20（Asia/Shanghai）。接续用户上一轮确认的T0-07操作4，完成操作5：有界drain及统一准入、双方effective快照、一次原生模拟、post-state复制或明确gap、tick提交、状态与本地处理回执发布。现行规范禁止计算/校验任何哈希，本轮使用已有值副本与可读状态，不新增hash。真实网络ACK协议留在T0-08。

## 实现前失败方式

1. 原生body尚未正常返回，或post-state尚未复制，就推进completed或发布成功回执。
2. 仅将孤立phase状态机写对，Native仍先commit后capture；延迟clock后current泄漏到tail/FSM消费者。
3. A物理输入在drain外准入，B脚本无界；startup双发/重复/故障输入漏计预算；TopExit控制输入绕过新阶段。
4. A/B使用不同validator/queue；输入在Resolve前改变世界，或只Resolve一方并等待另一方有包。
5. Warmup/暂停/loading/非法线程或来源仍消费script、生成sequence或进入drain；输入预算耗尽后继续无限处理。
6. duplicate或非法输入污染effective状态；same-sequence冲突未保持A失败/B退休策略；被拒绝的命令被当作processed。
7. 双玩家快照不在同一candidate；Read写状态、重复查询重放edge；缺B阻断A或计时。
8. previous未在正常原生返回时提交，或异常返回/旧代仍确认edge；模拟区间结束后的Shooting仍运行。
9. 标准Room返回就冒称完整世界，跳过owner/tail/outer配对；odd-only拿到完整世界资格。
10. sampler关闭/缺样/读取失败/背压阻断实际已正常完成的模拟计数；或者把未知/旧样本冒称完整。
11. copied batch提前state=READY被consumer读取；提交失败/取消时遗留WRITING槽，或者复用上一tick的staged值。
12. NativeReturned至TopExit间lifecycle/session变化抹掉已经观察到的正常模拟；反向以已退休session制造新模拟。
13. 返回/outer重复、缺失或错配造成二次commit/二次publication；publication失败回滚tick或自动重试。
14. 回执把processed高水位当作连续ACK，丢失实际source_sequence、generation和采样gap；声称已实现网络或世界动作效果。
15. 新接口破坏24/64字节输入、Event/Shared布局、Hook事务/恢复，进入原版Online/Isaac Sync或哈希校验路径。

## 工作安排

先登记本清单并编写完整离线生命周期场景、驱动与统一验收入口，再修改生产实现。按用户要求调用GPT-6 Luna / max只读分析，并由主智能体复读审阅。实现过程中不运行测试；最终纯编译Native与场景程序后，只执行一次统一离线验收，保存命令、阶段/准入/回执与实际源码和二进制。没有实机/网络操作授权推导，不运行游戏。

设计保持原生wrapper original exactly-once。共同pipeline协调已有Session/Clock/Participant；先验证drain入口，Resolve时才保留candidate。原生正常返回立即End(true)，收起消费者可见性；post-state在现有Outer尾部复制，之后TopExit提交tick再发布副本/回执。采样不足显式gap，不作为模拟是否发生的判据。

drain上限按现有脚本源最坏界取 `2 + 4 * kDrainBudgetPerTick`（258次准入尝试）：A最多2份启动命令，B最多64个script step、每步保守按4份计。参数化留给操作7；此预算不宣称真实网络接收能力。generation与来源仍由主线程保留，A的poll只记录新鲜样本，统一在Authority pre发布。

## 生产接入与阶段

`AuthorityTickPipeline`直接协调Native同一组AuthoritySession、AuthorityTickClock和两个Participant，没有另建scheduler替身。正常顺序为：

| 阶段 | 实际行为 |
| --- | --- |
| BeginDrain | 校验session/阻断/线程/来源/loop/上界；尚不保留clock.Current，不提前消耗输入序号。 |
| 有界准入 | A新鲜样本、B最多64步脚本、启动/重复/受控故障都经同一AcceptInput；尝试计预算，非法及重复也计。实际源最坏A2+B130+控制2=134，低于258。 |
| BeginSimulation | clock保留completed+1，两名未退休Participant依次Begin相同candidate，供原生动作消费者只读；不要求双方有输入。 |
| 原生body | 原wrapper保持original一次；有效消费者只认SimulationTick，phase外返回中性，Shooting沿用暂停消费约束。 |
| NativeReturned | CheckComplete仅核对原生返回；正常返回立即对双方End(true)，关闭输入可见性，world::Commit只登记该candidate的原生返回依据。completed仍不变。 |
| Outer post-state | 原owner/Room/tail/outer配对后复制。authority批次仍为WRITING，未向consumer发布；odd只产生gap，读取失败/预算/背压保留实际状态。 |
| FinishPostState | TopExit核对同loop，旧/错配sample降为Unavailable，确认已经正常返回的模拟并增加completed；后续lifecycle/session变化不抹掉该事实。 |
| publication | completed对controller可见后，释放匹配的world批次，再发布原AuthorityCommit与两个本地处理回执。失败保留已发生tick与可能发布的前缀，关闭后续输入，不回滚或重试。 |

完成时的真实TickBoundary保留于receipt，未用全零结构替代返回地址/线程/owner。processed代际与当前binding代际分开：生命周期发生在正常返回后时，历史处理仍归属原代，binding有效性可为false。processed序号是高水位，实际source_sequence另列，不是连续ACK。

物理poll不准入，Authority pre每个正常候选统一发布A；同loop/exact pair/新鲜源与startup双发规则保持。TopExit的受控conflict-b命令改为下一次drain执行，并保留设置时generation，重绑后丢弃。没有新增网络线程写入路径。

`world::PublishPostState(false)`释放未提交或错配的WRITING槽；未消费的pending槽使Drained返回false，不能报告已经完成恢复。正常TopExit先提交再释放。collector自身既有读取error只决定gap；仅本次通知/事件发布失败影响pipeline publication。缺失outer或异常退出仍按缺返回/恢复不确定处理，未宣称完整EH闭包。

## 本地回执记录

Event/Shared结构和输入codec大小保持，EventKind新增34=`AuthorityTickReceipt`。每个成功提交tick至多输出A/B各一条，仍在独立C/S日志，不是网络ACK。event.tick为已提交tick，ordinal为冻结A/B，command为client ID，target为processed高水位，generation为处理时的代际（尚无处理则0）；query为Authority kind，detail为PostStateStatus，action为world reason，boolean表示当前binding仍有效且session Active。

before四个u32依次为accepted序号低/高、processed tick低/高；after为实际processed source序号低/高、capture序号低/高。原AuthorityCommit的before新增drained、预算、attempt低/高，after为post-state状态、原因、capture低/高。PostStateStatus为0 Disabled、1 Complete、2 Gap、3 Backpressure、4 Unavailable。无collector、odd、失败或背压均不会被冒称完整世界。

## 验收边界

离线驱动链接实际pipeline/Session/Clock/Participant/codec，用自有地址标记绑定，并把原生body/collector结果作为显式环境输入。36个实现前场景贯通准入、双Resolve、只读查询、正常/异常返回、post-state、commit与本地publication；120次连续循环保留逐次结果。它不执行Windows游戏/Hook/真实采样内存/事件环/通知，不把模拟的body_calls当作实机original调用证据。真实Native接入由源码审阅与纯编译覆盖，运行行为尚未验证；最终唯一离线结果另记。
