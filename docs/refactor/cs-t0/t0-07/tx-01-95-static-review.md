# tx-01-95：操作6静态分析与主智能体逐项审阅

日期：2026-10-20（Asia/Shanghai）。本轮只做只读源码分析、设计比较和实现审阅；没有构建、运行场景程序、访问游戏或调试器，也没有把分析结果记作验收。

## 1. 审阅范围

阅读了[现行项目规范](../../project-conventions.md)、[操作6–9计划](tx-01-94-remaining-operations-plan.md)第1、2、6节、[操作5实现与结果](tx-01-93-tick-pipeline.md)、`tx-01-93-results.md`，以及生产`Participant`、`AuthorityTickClock`、`AuthorityTickPipeline`、V2 codec、Native `Submit`/`Authority`/`TopExit`/`ClientInputProgress`和操作5离线驱动。另按计划启动独立只读静态审阅子智能体；子智能体仅阅读文件并返回风险，主智能体逐项回到源码复核。

## 2. 关键结论与处理

| 审阅问题 | 主智能体复读与决定 |
| --- | --- |
| `last_received_tick/sequence`包含所有可归属的duplicate、invalid、旧代、wrong session/client尝试，不能作为有效保活或timeout恢复依据。 | 采纳。新增独立的`valid_arrival_*`和`valid_arrival_count`，只在`Submit`完成全部验证、队列写入和返回`Accepted`后刷新。坏到不进入`Submit`的编码失败也不刷新。`last_received`保留原诊断语义。 |
| 包内`target_tick`不能代表到达时间，未来命令到达也不能说明当前tick有effective输入。 | 采纳。有效到达时间记为本次Draining的Authority候选tick（`clock.Completed()+1`）；effective availability只由`target_tick == Begin(tick)`的同代命令决定。分离`effective_available`、`missing_reason`和`source`。 |
| `max_input_age_ticks=0`与client静默超时是两件事，后者不能自动踢B或结束session。 | 采纳。固定`kMaximumInputAgeTicks=0`、`kClientSilenceTimeoutTicks=120`；阈值仅产生`Silent/TimedOut`诊断和迁移计数，session/binding/retirement逻辑不读取阈值。缺输入仍立即中性。 |
| 原有`Begin`在所有未退休情况下都可执行。若在未绑定状态开始generation基线，随后rebind但不再次`Invalidate`会把静默年龄错误地从旧unbound Begin算起。 | 独立静态审阅指出的关键问题；采纳并修正。`Begin`不再直接`StartGeneration`；只有`UpdateSourceSilence`在`BoundValid && SessionInputActive`时建立generation起点，`NoteValidArrival`也只会在已接受的合法绑定路径上建立/刷新起点。 |
| 首次有效绑定后的首个缺失tick没有previous，原`End`不会推进processed tick，也不伪造可比较last_state。 | 采纳并保持tx-01-91规则。验收预期改为`processed_tick=0`、last_state invalid；不为了显示“已经处理missing”破坏既有fresh-binding语义。availability仍记录本次missing scope。 |
| 失绑/rebind保留历史edge诊断。 | 采纳。`Invalidate`清current/previous/pending和当前代可用性，但不重置`progress_.edges`的processed历史；旧edge不得在新代触发，但其历史processing count仍为1。验收预期已按此修正。 |
| 干净pause/loading/menu/render不应增加输入年龄。 | 采纳。只在已确认Authority候选的`Participant::Begin`中更新静默计数；blocked/Inactive路径不调用Begin，`completed_tick`不前进。离线blocked场景验证静默tick不增长。 |
| 失绑、room/death/lifecycle和终态不能让旧current/pending/edge复活。 | 采纳。`Invalidate`递增generation、清pending/current/previous并重置当前代到达/静默/effective source起点；保留会话sequence、dedupe history、拒绝计数和累计有效到达/迁移计数。`Progress`对Unbound/Retired/SessionInactive做终态覆盖。 |
| 异常返回不能确认missing scope的中立processed状态或edge。 | 采纳。只有正常、同代、Active、已绑定`End`才持久化availability、previous/neutral state和edge processed。`NativeReturned(false)`走Abort→`End(false)`，已发生prior tick保持不变。 |
| 逐次Submit/Read追加事件不可接受。 | 采纳。Read不产生或修改任何诊断；每次成功提交tick每client至多新增一条EventKind35；EventKind33在退休时从原28行扩展为30行固定行数，新增query8/9。 |
| `Event/Shared`或24/64字节输入布局若被修改会破坏Native/Hook/IPC。 | 保持。Event仍128字节、Shared v3/v4偏移不变、输入command仍64字节/state 24字节。新增字段只在内部`ClientInputProgress`、`TickClientReceipt`和`ClientInputAvailability`；Event使用既有字段承载诊断。 |
| x86 LTCG聚合值参数风险。 | 采纳历史教训。availability填充使用输出引用，不新增按值聚合参数；`TickPostState`仍按既有const引用约定。 |
| future pending命令、精确target command和同一tick多个edge来源。 | 保留tx-01-91行为：effective current只在`target_tick==tick`时解析，future仅保留队列；missing reason扫描同代future，区分`FutureOnly`与`NoTargetCommand`；edge仍按原OR合并与source账本，不由诊断改写。 |
| 恢复只接受新合法同代输入，启动例外不重放旧队列/edge。 | 采纳。诊断不绕过`MakeCommand`、sequence、dedupe、target window或startup规则；recovery计数仅在`Accepted`路径更新，旧代/重复/非法不触发。 |
| timeout阈值需要可解释起点、跨generation和终态规则。 | 采纳并写入实现/文档：起于当前generation最近有效到达候选tick，无到达时起于该generation首个已绑定Begin；blocked不推进；Invalidate重置基线；Retired/Unbound/SessionInactive显式报告终态且不继续累计活动超时。 |

## 3. 实现落点

- `coop_input.hpp/.cpp`：`MissingInputReason`、`SourceActivity`、`ClientInputAvailability`、独立有效到达/静默/恢复状态、`FillAvailability`、`Progress`终态覆盖和`End`提交规则。
- `authority_tick_pipeline.hpp/.cpp`：成功tick的`TickClientReceipt`携带同一`ClientInputAvailability`副本。
- `coop_active_ipc.hpp`：新增EventKind35；尺寸/偏移不变。
- `coop_active_runtime.cpp`：每成功tick每client发布一条availability事件；退休progress增加query8/9；查询和提交路径不新增事件。
- `missing_input_main.cpp`、testdata、`accept-missing-input.py`：先于生产诊断实现的离线场景与唯一统一入口。

独立子智能体返回的初步风险已并入本表，主智能体随后逐行审阅receipt/Native event packing、End/Progress终态规则和离线预期；没有发现需要重新逆向原生偏移、修改Hook事务、扩大实机授权或引入网络ACK的需求。真实Native/wrapper/采样内存/事件环和游戏动作仍未运行。

## 4. 首次统一入口后的测试修正

首次统一入口 `tx-01-95-missing-input-01` 已实际执行并原样保留：10场景、146命令、122条批量记录，141/152检查通过、11项失败，退出码0、场景程序无异常；失败均来自离线场景/预期，而非生产状态不符：

- `Magnitude`的action轴方向约定（action0为左/上、action1为右/下）与测试读取动作不一致，导致应由正`move_x`覆盖的读取误取反向轴；
- 失绑tick没有visible processed范围，`last_processed_source_sequence`保留上一正常tick的`1`而不是伪造`0`，测试预期写错；
- 终态直接输入命令写成了`direct remote <ordinal> ...`，而驱动语法实际为`direct-bytes <ordinal> ...`；前者使流解析丢token，旧命令成为非法编码而不是预期的`Retired`。

修正仅限`testdata/tx-01-95-missing-input.json`的场景命令/预期和驱动中`istream`参数类型；生产输入/时钟/pipeline/Event布局逻辑未因这次失败改判。旧artifact、commands、events和failure结果继续保留，最终结果使用新的artifact目录记录。此次不是自动重跑：先定位并实质修正错误场景，再以新目录执行一次整体验收。
