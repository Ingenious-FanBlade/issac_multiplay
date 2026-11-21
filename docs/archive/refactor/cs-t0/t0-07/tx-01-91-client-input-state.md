# tx-01-91：T0-07 操作 3，逐 client 输入状态

2026-10-20（Asia/Shanghai）。完整目标：在当前生产 Participant 中补齐可读的序号/未来窗口、接收/接受/Resolve/正常处理进度、最近连续状态、逐动作边沿来源及分类拒绝计数，并保留 Native 的有界诊断输出。只完成操作3；不改变输入lead=2、启动例外、缺输入立即中性、A/B身份、会话/时钟、协议或操作4–9。

## 实现前失败清单

1. 把客户端target tick当接收时刻，或把成功入队/Resolve当成原生正常返回后的处理确认。
2. duplicate、stale、late、far-future、wrong session/client/generation或非法编码推进accepted/processed高水位，或重放旧边沿。
3. Native编码失败绕过Participant导致漏计，或重复统计同一次提交；拒绝分类遗漏终态/暖机/队列满/冲突。
4. 接收记录被误作超时/ACK策略，改变尚未实施的后续操作；原始到达顺序与accepted高水位混在一个字段。
5. 同tick多条输入的edge按OR合并后，全部错误归属给最高sequence；accepted、resolved、processed边沿未区分。
6. 重复查询修改边沿账本或计数；重复End、失败返回或generation变化仍确认边沿，或正常缺输入重复确认旧edge。
7. 新绑定、失绑、死亡/房间变化、Retire或Session终态仍暴露旧held/edge为有效last state；读取统计触发重绑或写世界。
8. Invalidate清除会话序号/去重历史/拒绝计数，使旧packet复活；history淘汰后旧sequence重新入队。
9. last processed sequence被未来入队污染，或把最高已确认sequence误称为所有更小序号都处理过。
10. future window下/上界加法溢出；正在Resolve但current不可见的tick仍可迟到入队；窗口展示与validator行为不一致。
11. neutral/unbound Resolve导致scope不可见，再次Begin覆盖未结束范围；正常模拟已发生却因计数新增字段卡住时钟。
12. accepted edge target达到u64上限时MakeCommand加一回绕；诊断计数溢出后静默归零。
13. 退休清掉全部历史，无法复查已处理/拒绝事实；历史ledger没有generation标记，混作当前可执行动作。
14. 每次输入/查询输出大量新增事件，超过主线程或既有事件环预算；更改Shared/Event/InputCmd布局破坏IPC/codec。
15. 使用离线替身状态证明生产路径通过；实现后补写单元测试，或验收失败后自动重跑并覆盖历史。

## 工作与验收安排

GPT-6 Luna / max子智能体只读分析现有状态、入口和定义，主智能体直接复读源码/历史快照并审阅结论。先写此失败清单和离线生命周期场景，再实现；分析与实现中零测试。准备完成后纯编译Native与离线程序，仅执行一次统一离线验收，贯通生产Session/Clock/codec/Participant，保存可复查、可重复的输入/事件/源码/二进制/日志。

本次不查询或操作游戏，不假称新的实机Hook/输入/原生动作结果。边沿processed只表示同代有效输入范围在模拟正常返回后确认，不能证明炸弹/道具在世界中一定成功触发。旧结果和失败均保持。

## 实现与字段契约

`Participant::Progress(completed_tick)` 返回固定大小的值快照，包括冻结身份、本地 next_sequence、接受高水位及其target、可归属 received 的 tick/sequence/target/generation/decision、resolved和processed进度、有效性与未来时间窗。processed高水位不是连续ACK；同范围的source_sequence与贡献数量另列。缺输入立即中性，未获得第一条有效输入的fresh binding不虚构previous/processed。

last_state只存最近一次成功处理的连续量，edge固定清零。有效性要求当前Active、仍绑定且代际相同；失绑、退休或异常范围会使它不可用，getter返回中性状态，历史processed tick/来源仍保留。它不用于延长持有旧输入。scope_pending独立于可见性；target必须大于completed和last_resolved，且与completed差值不超过64。future边界使用饱和加法，窗口为空时first=0且valid=false。

四项edge账本按Bomb/Item/Pill/Drop保存accepted、resolved、processed的序号/tick/generation及处理次数。Begin合并edge但保留每种真实来源；只有正常End且资格仍有效时确认，重复查询/End不增加次数。失绑清临时来源、保留历史。

每次Submit或编码失败恰好一次记账：submit_count覆盖送到该Participant的尝试；received_count只覆盖session/client可归属的尝试。accepted和duplicate单列，WrongSession至SessionInactive的12种拒绝分类计数；进入原验证顺序得到的首个原因即本次类别。失绑丢弃的队列、已接受后过期的队列另列。计数饱和后保留粘滞标志，不改变输入或模拟资格。

## 有界 Native 诊断

只在B退休首次进入及A最终退休后调用 `PublishClientProgress`；每个client最多28行，不在Submit/Read增加新事件。新增EventKind33，Event/Shared/codec大小和布局不改。所有64位对以low32/high32顺序放入before/after。query含义如下：

| query | command / target | before | after |
| --- | --- | --- | --- |
| 0 | accepted sequence / processed高水位 | received tick、processed tick | resolved tick、accepted target |
| 1 | next source sequence / received sequence | received target、received generation | future first、future last |
| 2 | submit_count / received_count | accepted、duplicate计数 | rejected、accepted后过期计数 |
| 3 | 失绑丢弃计数 / processed source sequence | last state tick、last state generation | resolved sequence低/高32位、resolved contributors、processed contributors |
| 4 | 对应拒绝计数 / 0 | 清零 | 清零；action为Decision 3–14 |
| 5/6/7 | edge accepted/resolved/processed sequence / 对应tick | generation、processed_count（仅7） | 清零；action为edge 0–3 |

所有行detail位0–6依次为has_received/current_valid/last_state_valid/future_window_valid/scope_pending/retired/counters_saturated；query0的state为有效last state，query1的decision为最近可归属接收决定。退休后last state中性，历史时刻和来源仍可读。发布失败停止额外输出，调用方继续既有退休清理。

## 唯一离线验收范围

实现前已写22个场景：生成/接收/接受/处理分离、同tick多edge来源、缺输入中性、全部拒绝类型、冲突退休、未来输入不冒充已处理、失绑/重绑/旧代/history保留、在途失效、异常返回、终态状态屏蔽、不可见范围迟到/重入、队列满、4100次生命周期跨history淘汰、上界窗口/edge加一溢出，以及乱序target的processed高水位。

`accepted-then-expired-fault`直接调用生产Participant API跳过此前tick的Resolve，用于观察过期丢弃分支；它显式标为离线故障注入，不声称Native允许漏掉一名client的正常Resolve。其余时钟相关场景经生产Clock精确配对。输入标记是自有对象地址，不解引用游戏对象。极值场景通过离线Clock构造初值实现，Native初值仍为0。

准备期间仅编辑与静态阅读，尚未运行验收。最终结果单独记录，不回写为实现前已测试。
