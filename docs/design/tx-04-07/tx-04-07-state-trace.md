# tx-04-07 第 3.2 节状态与 trace 编写记录

日期：2026-10-26。状态：**代码已编写，尚未构建、部署或运行；不代表 V1–V16 已通过。** 本次在工作区已有第 3.1 节代码上完成状态与证据接线，仍属于同一次 tx-04-07 推进。遵循[现行规范](../refactor/project-conventions.md)，没有运行测试、校验脚本或实机操作；统一纯构建和唯一整体 E2E 留到整个 tx-04-07 实现完成后。

依据：[实现方案第 3.2 节](tx-04-07-implementation-plan.md#32-状态与-trace)、[工程细化第 8 节](tx-04-07-engineering-changes.md#8-状态验收证据与实施顺序)。网络线程继续只交换值，所有新增游戏诊断从主线程现有记录复制，不增加原生查询、不调用 Submit/Resolve，也不消费边沿。

## 状态字段

`cs status` 继续采用 `status_version=1` 的键值格式，新增字段为追加项。

| 字段 | 语义 |
| --- | --- |
| `session_generation`、原 `generation` | 本轮会话代次；Hosting 尚未初始化 core 时也使用本轮已分配代次。A/B 的代次仍分别用 `a_generation/b_generation` |
| `b_life` | 当前生命状态 Alive/Lost/Revived；Join 前、Solo 或成功退休后为 None。清理失败仍保留实际生命状态与 Retiring |
| `b_deaths/b_revives/b_rebinds` | 本轮累计；成功退休后保留诊断，新 Host 构造新记录。重绑不包含首次 Join 绑定 |
| `b_death_event`、`b_lost_loop/tick`、`b_revived_loop/tick` | 当前死亡窗口及实际观测点 |
| `b_bound`、`b_awaiting_rebind` | 是否可用绑定、是否等待复活重绑；成功退休后均为 0 |
| `b_ownership_confirmed/generation`、`b_pending_reason`、`b_conflict_reason` | 最近确认归属的代次与原因；归属记录不代替当前 `b_bound`，退休后不再声称有当前归属 |
| `join_state/join_last` | 成功退休后 None / 最近业务结果；失败不提前复位。Solo 不显示默认 Waiting |
| `peer_admitted_commands` | 原有 admission 尝试次数，不能当作 Accepted |
| `peer_accepted_commands/duplicate_commands/rejected_commands` | Participant 真实 admission 结果计数 |
| `peer_accepted_sequence/processed_sequence/advertised_generation` | 接受、正常返回处理与已交给 Ack 的代次分别展示 |
| `peer_discard.<reason>` | 按命令条数累计，包含冗余；原因保留 life_lost/loading_or_blocked/unbound/old_generation/old_window/sequence_floor/inactive |
| `query_reads/repeats/mismatches/overflow/nonfinite` | 实际读取、同键重复、不一致、桶不足和非有限值；没有重复不能视为验证通过 |
| `query_keys_observed/repeated` | 按 ordinal × consumer × action × QueryKind 统计，最多 432 个分类键，不能用 A 的覆盖替代 B |
| `query_mismatch_known`、`query_mismatch_*` | 首个不一致的 ordinal、consumer、kind、action、tick、参与者代次、epoch 及两个值 |
| `query_snapshots` | 已发布的覆盖快照编号，仅用于诊断关联 |
| `loading_*`、`loading.a/b.*` | episode 来源/owner/进入与结束 loop/completed/admission，以及两个参与者的队列、代次、submit、最后 Resolve tick 前后值 |
| `loading_end_known/end_reason` | 区分尚未结束、loading_clear 和退休中断；尚未结束时 end 数值不代表完成观察 |
| `persist_load_slot_known/current_call`、`persist_window.<id>.*` | 区分未知存档槽；活动窗口补 call_normal、会话代次、caller、loading episode、death event |
| `trace_truncated` | 超长格式化次数，同时计入原 `trace_dropped`，沿用现有证据丢失处理 |

持久入口/窗口计数仍是进程累计，B 生命与 peer/query 计数是本轮会话累计，不混作同一口径。

## trace 与场景接线

| 事件 | 新增或补齐的证据 |
| --- | --- |
| `life_ownership` | Join、Lost、Revived、重绑、再次 Lost、身份矛盾的实际健康/列表人数、当前归属、本地地址、房间、owner/token、completed 与 simulation_tick；明确会话/参与者代次 |
| `binding` | 保留旧 `generation` 为该参与者代次，同时追加 session_generation/participant_generation/epoch，失效与 B 重绑保留原始 reason 和分类 |
| `life event=retired` | 仅成功退休后发出 status=None，保留 last_status、计数和 Join 结果；失败不发成功退休事件 |
| `peer_discard` | 每次 drain 按原因最多七条；命令条数、累计、Ack 命中与旧代次条数，优先保留首条 Ack 已知样本的包/命令/last_ack 序号与代次。不把 unknown stamp 当成已证实旧代次 |
| `submit` | B 的原 admission 事件补 participant_generation 与 last_ack/ack_stamp_known/ack_generation |
| `ack event=publication` | 首次、代次变化时立即记录，其余每 30 次汇总。queued 只表示入 worker 队列，不冒充发送成功；成功发送 stamp 仍由 worker 建立 |
| `tick` | 原汇总增加 input_generation/input_epoch，保留 Authority-pre 输入实际来源；tick 中途失效后仍记录真实结束代次、生命与绑定状态 |
| `query_repeat` | 首个不一致输出完整键及有限值；epoch 改用 uint64，与格式串一致 |
| `query_coverage/query_summary` | 每个 ordinal/consumer 一行、每行 36 个 action/kind 键，值为 reads/repeats/mismatches；固定容量，零覆盖保留。Host 每份快照最多 12 行加一条摘要，无逐查询事件 |
| `loading/loading_owner` | 会话代次、episode、owner/token、正常/异常返回；窗口补 submit/Resolve/队列/代次前后快照，退休中断按真实原因收口 |
| `persistent/persist_window/persist_read` | 同步 call/parent 与跨 loop window ids、会话代次、原返回值及其是否存在。八个窗口的 id 列表缓冲覆盖最长 uint64；TryUnlock 字段不可读时不宣称已知变化 |

场景边界可通过同一命令入口调用：

```text
cs query_checkpoint S11.begin
cs query_checkpoint S11.end
```

标签为 1–64 个 ASCII 字母、数字、点、下划线或连字符。命令返回 `query_checkpoint/query_snapshot/session_generation`；trace 各行带相同 `scope=checkpoint/checkpoint/snapshot`。归约在同一 session 内对 begin/end 的累计键计数作差；读取为零或重复为零分别记 NOT_OBSERVED。检查点不清桶、不清计数，不影响成功/失败退休时的一次最终覆盖快照。后续 tx07 harness 尚须接入这些调用，不能把接口已编写当作场景已经执行。

## 日志容量与边界

trace 保留 512 槽、每事件 detail 3500 字节。查询覆盖分行后，最坏三项 uint64 的 36 个键仍有界；`complete=0` 明确表示某行没有完整输出。定期 `counters` 按换行分片，每片正文最多 2500 字节，使用 frame/part/last/total_bytes 重组。`command` 不再把整个 status/counters 响应重复塞进 trace，改记长度与结果，完整响应仍从控制口读取。

每条 JSONL 增加 `truncated` 标志。超长格式化同时累计 trace_truncated 与 trace_dropped；最终 shutdown 记录这两个计数。控制口响应超过固定 PortReply 容量时显式返回 response_too_large，不静默给出残缺 status。

以上为代码与接口记录，来自源码回读，**没有执行阶段验收或功能测试**。官方 Hook 实际次序、实机时序、文件操作结果、队列实际负荷及 V1–V16 结论仍待全目标完成后的统一工作；第 3.3/3.4 的 Lua、relay、远端与 tx07 harness 不在本次交付中。
