# tx-01-95：T0-07 操作6，缺失输入、静默超时与恢复

日期：2026-10-20（Asia/Shanghai）。完整目标：在现有生产 `AuthoritySession`、`AuthorityTickClock`、`AuthorityTickPipeline`、`Participant`、V2输入codec和Native日志路径上，明确并实现操作6的缺失输入、源静默超时诊断与恢复语义。只完成操作6；参数注入、完整故障矩阵和真实Authority停发实验分别留给操作7–9。现行规范禁止建立gate、校验哈希、用历史解锁字段控制推进或在实现过程中试跑。因此本轮先写失败方式、场景和入口，再改生产代码；全部目标完成后只执行一次纯编译和一次统一离线验收。

## 1. 已确认的行为范围

1. **有效输入缺失**只表示本Authority tick没有可用的同代、同client、target恰好等于该tick的命令。已接受的未来命令不等于当前tick已有输入；当前tick的effective状态仍为中性。
2. `max_input_age_ticks`固定为 `0`。连续量、held立即中性，不沿用上一条命令的移动、射击或道具按住状态。`last_state`只作诊断，绝不用于维持旧输入。
3. edge只来自当前tick的精确target命令。缺输入不执行edge；duplicate、expired、late、错误session/client、旧代或恢复后的旧edge均不补执行。重复查询和重复`End`不增加edge处理次数。
4. A/B完全同一规则、各自独立。B失联、静默、无绑定、已退休或缺少当前命令都不是A/tick等待条件；中性只定义输入值，不声称B的物理位置、惯性或碰撞结果不变。
5. pause/loading/menu/lifecycle阻断不产生成功Authority tick，也不增加输入年龄。room、死亡/复活、reload/rebind使原代current、previous和pending失效；menu/run teardown不能由旧输入复活。
6. 恢复只接受新鲜、合法、同代、序号可接受的命令。普通恢复仍按当前输入语义和正常lead；启动例外保持已采纳的`completed+1`首份命令加正常lead命令规则。旧队列、旧edge和旧样本不重标为新代。
7. 输入有效期与client静默超时分开。`max_input_age_ticks=0`和`client_silence_timeout`都不自动踢出B、不结束session、不触发AI接管或网络断线；本阶段只记录静默/超时/恢复事实并保持中性。

## 2. 源静默、有效缺失和超时口径

- 固定 `kMaximumInputAgeTicks = 0`、`kClientSilenceTimeoutTicks = 120`，二者都是Authority模拟tick单位，不由墙钟、QPC、渲染帧或顶层loop推进。
- **有效新输入到达**：仅 `Participant::Submit` 最终返回 `Accepted` 时刷新。duplicate、invalid、wrong session/client/generation、stale、expired、too-far、edge conflict、full或终态拒绝都不刷新它。
- 到达tick取本次准入所在的Authority候选tick，即Draining时 `AuthorityTickClock::Completed()+1`；不是包内`target_tick`。因此同一drain内到达但target未来的命令只刷新“有新到达”，不提供当前effective输入。
- 源静默tick数从当前generation最近一次有效新输入到达tick开始计算；当前generation尚未收到有效输入时，从该generation第一次成功`Begin`的tick开始计算。跨generation重置这个起点，但累计接受计数和迁移计数保留。
- `source=Active`表示无静默间隔；`Silent`表示静默1–119个Authority tick；`TimedOut`表示静默达到120个Authority tick。超时只发诊断，不改变session/binding/retirement。
- 当前generation内首次`Accepted`若发生在静默/超时之后，分别增加`silence_recoveries`/`timeout_recoveries`并清除状态。坏包、重复包或旧代包不触发recovery。
- binding失效、rebind、retire或session终态后，source报告`Unbound`/`Retired`/`SessionInactive`，不继续累计活动超时；新generation重新建立静默起点。

## 3. 实现前失败方式

1. 把target tick或包到达虚构为当前tick已有输入，使未来命令消除当前中性。
2. 缺输入仍保持last held/continuous值，或把上一条edge在后续tick补触发。
3. duplicate、invalid、old-generation或wrong session/client刷新有效到达时间，从而错误保活并推迟timeout/recovery。
4. 将`last_received`（含所有可归属拒绝）当作有效输入进度或ACK；不区分received、accepted、resolved、processed和source_sequence。
5. 只在查询时计算或每次Read新增事件，造成不可限定日志量；或者查询本身改变静默/超时计数。
6. 在pause/loading/menu/render/顶层loop期间增加输入年龄或推进completed。
7. 用单方缺失阻断另一名玩家或Authority tick；或用B物理惯性冒充输入仍有效/已中性。
8. room/death/rebind后仍把旧generation current/previous/pending当作可恢复状态；旧edge被平移到新generation。
9. 恢复时重放旧队列/edge、复用旧sequence，或把旧样本重新标为新代；启动例外被错误用于普通持续绑定恢复。
10. timeout直接退休B、结束session、清空历史或自动重试；或把“达到阈值”与“实际网络断线”混为一谈。
11. 源静默计时使用墙钟/渲染帧，或被blocked/暂停区间推进；跨generation起点错误。
12. Native只发布最终receipt而不提供缺失原因、effective source/age、有效到达、静默/超时/恢复状态，操作9无法按tick解释窗口。
13. 新增诊断破坏Event/Shared布局、24/64字节输入布局、Hook事务/恢复、original exactly-once或C/S与原版Online/Isaac Sync隔离。
14. 离线替身复制生产判断而不是链接真实Session/Clock/Participant/pipeline；或者实现后补写测试、失败后自动重跑来掩盖过程。
15. 把离线body/collector/地址标记冒充真实Windows original、真实内存、线程或网络证据。

## 4. 生产状态与字段

`ClientInputAvailability`是固定大小的只读值快照，随`ClientInputProgress`和每个成功tick的`TickClientReceipt`发布：

- `observed_tick`、`effective_available`、`neutral_applied`、`missing_reason`；
- `source`（Active/Silent/TimedOut/Unbound/Retired/SessionInactive/Unknown）；
- `effective_source_tick`、`effective_source_sequence`、`effective_age_ticks`；
- 最近有效新输入到达tick/sequence/target/generation、累计`valid_arrival_count`、`has_valid_arrival`；
- `silent_ticks`、`silent_entries`、`silence_recoveries`、`timeout_entries`、`timeout_recoveries`。

`missing_reason`只在有效缺失时给出`NoTargetCommand`或`FutureOnly`；终态给出`Unbound`/`Retired`/`SessionInactive`；有effective输入为`None`。`neutral_applied`表示本scope没有effective命令且仍处于可处理binding，因此当前查询按零状态处理；fresh binding尚无previous时不因此伪造可比较last_state。

`Begin(tick)`只更新本scope的availability与静默诊断，不承诺processed。正常同代`End`在session仍Active、binding仍有效时提交availability；异常返回、错误线程/来源、scope失效或终态不确认中立值、edge或新的effective source。原processed高水位、实际source sequence和capture状态语义保持不变。

## 5. Native诊断

Event/Shared尺寸、24/64字节输入codec和既有EventKind编号不改：

- 新增EventKind 35 `ClientInputAvailability`，每个成功提交tick、每个client至多一条。query为source状态，decision为missing reason，detail位0–2为effective_available/neutral_applied/has_valid_arrival，before为最近有效到达tick与effective source tick，after为effective age与silent ticks，command为最近有效到达sequence，target为本tick实际processed source sequence。
- 现有EventKind 33 `ClientInputProgress`在退休时新增query 8（当前可用性快照）和query 9（阈值与迁移计数）；每client仍为固定行数，不在Submit/Read逐次追加。
- 新事件只写既有128字节Event字段；不保存原始地址、peer identity或输入保留字段，不改变Shared v3/v4偏移。

## 6. 唯一离线验收设计

场景和预期先写入 `cs-t0/testdata/tx-01-95-missing-input.json`；驱动 `cs-t0/native/scenarios/missing_input_main.cpp` 链接真实生产Session/Clock/Pipeline/Participant/V2 codec。覆盖：

1. fresh binding首tick无历史缺失：无伪造previous，查询零，记录NoTargetCommand；
2. held+edge后缺失：continuous/held归零、edge不重放、重复Read不改变计数、processed tick与实际source分开；
3. A/B独立：一方缺失不阻止另一方或tick；
4. 未来命令：当前有效缺失为FutureOnly，到target tick才effective，同时保留“源静默”独立状态；
5. 精确静默阈值与恢复：119/120/121边界，duplicate/invalid/old-generation不保活，首次合法新输入才recovery，session不自动结束；
6. pause/loading阻断：completed与silent ticks不前进；
7. 失绑、rebind和旧代迟到：旧edge不复活，只有新代合法输入恢复；
8. session Retiring与Participant Retired终态：直接生产Participant入口拒绝旧输入，source报告终态；
9. 异常返回：不确认缺失scope的中立processed值或edge，已发生prior tick保持；
10. 固定策略常量、receipt availability和Native source接入静态绑定。

统一入口 `cs-t0/accept-missing-input.py` 只启动一次离线场景程序，保存commands、events、batch、源码/二进制/日志与路径长度时间出处；不计算/校验哈希。真实游戏、Hook、采样内存、事件环、网络和世界动作效果均为NOT_RUN。若验收失败，保留失败结果和未完成项，不自动重跑。

## 7. 完成依据

- missing/target/effective/source-silent/timeout/recovery字段直接来自生产状态链，而不是分析脚本重算；
- 缺输入值确实为零、旧edge不确认、恢复实际source正确、B缺失不阻止A/tick；
- timeout只诊断、不自动踢B/session，坏到达不保活；
- 旧generation、失绑/rebind、pause/loading和终态行为符合本文；
- Native receipt/availability日志字段与生产`Participant`同源，Event/Shared和输入布局不变；
- 一个统一离线入口一次执行，结果、限制与复现记录完整。
