# tx-01-97：操作8静态分析与主智能体逐项审阅

日期：2026-10-20（Asia/Shanghai）。本轮只读审阅生产Session/Clock/Pipeline/Participant、操作3–7历史场景及新的故障矩阵。未构建、未运行场景、未访问游戏。按计划启动独立只读静态审阅子智能体，主智能体回到源码逐项核对。

## 1. 关键决定

| 审阅点 | 决定 |
| --- | --- |
| A/B conflict策略此前在Native wrapper中，离线pipeline场景无法证明A关闭session/B仅退休。 | 将已采纳策略下沉到`AuthorityTickPipeline::AcceptInput`（typed+bytes）：A`SequenceConflict`使session`Failed/RuntimeFailure`；B仍由Participant Retire。Native wrapper保留错误发布。 |
| source sequence u64 max旧测试只能靠海量循环。 | `Participant::Initialize`增加可选`source_sequence_start=1`，仅用于受控初态；默认Native不变，0/MAX仍为wire Invalid。MAX-1可生成一次合法source command，下一次MakeCommand退休不回绕。 |
| queue/history极值不能只靠替身。 | 新场景直接调用生产`MakeCommand`/`AcceptInput`/`Begin`/`End`：257次尝试验证256 pending后Full；4100次成功处理验证4096 history淘汰后seq1为Stale、新seq仍Accepted。 |
| far-future/expired/edge/duplicate需在共享AcceptInput路径。 | 新场景通过生产Pipeline typed/bytes入口覆盖64/65/UINT64_MAX target、过期、edge conflict、duplicate及无效0/MAX sequence。 |
| return/publication错误不得提交completed或发布success。 | 新场景覆盖abnormal/wrong/missing return、post-before-body、publication failure和repeat publish；检查completed、session cause和receipt published。 |
| collector状态不能伪装完整。 | 同一Domain连续产生Disabled/Gap/Backpressure/Unavailable，并用odd Complete验证转Gap/reason2/capture0。 |
| timeout只诊断。 | 121个完成tick后断言B TimedOut、silent_ticks120、timeout_entries1，同时session Active且B未退休。 |
| 修改输入布局/Hook/IPC是禁止的。 | 无codec/Event/Shared/Hook布局修改；`Initialize`只在末尾增加默认参数，既有Native调用仍取1。 |
| 历史覆盖不重写成新结果。 | 文档保留操作3–7出处，新矩阵只补当前生产链和缺口；历史artifact不改写。 |

## 2. 风险与边界

- queue-full尺寸不通过缩短容量来测；使用真实256容量并允许257次尝试。
- history 4100循环只输出一条汇总，不产生逐命令事件；中途Accepted失败会使汇总ok=false并保留实际最后计数。
- conflict A关闭session后不执行任何world动作；B退休不影响A和tick。
- source测试起点MAX-1是离线构造，不改变生产默认1；不把source序号当wire身份或canonical input。
- collector `Disabled`表示没有collector配置，不等于完整采集；odd-only不获得完整world资格。
- 真实游戏/Hook/采样内存/事件环/网络均为NOT_RUN；离线结果不证明世界动作。
- 未设gate、未计算/校验哈希、未自动重跑或覆盖历史。

独立子智能体的最终问题清单将在最终构建前并入；若发现生产语义或场景预期错误，先保留失败证据，再做实质修正并使用新目录验收。

## 3. 独立静态审阅发现与处理

独立只读静态审阅子智能体返回的问题及主智能体处理：

| 发现 | 处理 |
| --- | --- |
| acceptance传3参数而scenario曾要求4参数，程序exit2且不生成events。 | 采纳。场景入口改为仅input/events两参数，不保留无用batch流。首轮失败artifact保留。 |
| 场景Snapshot没有`simulation`字段，而testdata有6处引用。 | 采纳。改用现有`current`字段做resolve断言，不新增重复诊断字段。 |
| A-conflict在Pipeline中只调`session.Fail`，不Abort，phase/clock不一致；Native wrapper在session已Failed后捕获before，可能丢失Active→Failed transition事件。 | 采纳。Pipeline A conflict在session fail后执行`Abort(Cancelled)`，关闭scope并使phase Failed；Native新增`FailFrom(error,cause,authority_before)`，在Submit前捕获pre-failure status，保留transition发布。A conflict矩阵改断言phase Failed。 |
| source初始0/MAX拒绝路径未单独覆盖。 | 采纳。新增source-start-zero-invalid和source-start-max-invalid场景，断言Participant直接retired、无生成命令。 |
| queue-full没有证明已入队256条仍能完成随后tick，也未区分next_sequence和last_accepted。 | 采纳。fill后断言next_sequence=258/last_accepted=256，再Begin/Finish tick1，断言depth0、processed source256。 |
| client timeout只有双方静默，缺B-only+A持续新鲜输入、坏包不保活、恢复。 | 采纳。新增`timeout-a-live`驱动路径：A每tick新鲜输入，B静默121 tick；随后非法seq0不刷新，合法B输入恢复并断言timeout_recoveries/valid_arrival_count。 |
| duplicate/late缺少过期后accepted高水位和late edge不重放。 | 采纳。过期后立即status断言last_accepted仍1；加入edge accepted/processed tick与count字段，确认只有一次处理。 |
| phase/return/publication缺repeat-post、wrong-loop post、wrong-tick Unavailable、simulation后missing return/post。 | 采纳。新增对应5个场景；wrong-tick断言post=Unavailable(4)/reason18/capture0。 |
| identity/encoding缺version/length/edge-flag malformed及terminal生产入口。 | 采纳。新增bad-version/bad-length/bad-edge；新增session Retiring后direct Participant输入返回Retired。 |
| collector mismatched sample仅继承历史。 | 采纳为本轮新场景wrong-tick-sample；Disabled/Gap/Backpressure/Unavailable/odd仍连续执行。 |

审阅未发现新增source sequence参数、24/64字节codec、Event/Shared布局或默认Native source sequence被改变。最终仅以`build-05`和`fault-matrix-06`的184/184 PASS作为操作8结果；旧PASS/FAIL均保留。
