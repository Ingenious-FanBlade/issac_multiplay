# tx-01-98：T0-07操作9实现与静态审阅

日期：2026-10-20（Asia/Shanghai）。范围：操作9的真实实机 B 源停发场景、统一执行器和分析器。按[现行规范](../../project-conventions.md)，实现和静态分析阶段不运行游戏、不提前跑测试；最终只以统一入口执行一次有效实机验收。历史失败 artifact 原样保留。

## 1. 生产路径复核

审阅并直接对照以下现行实现：

- `cs-t0/native/src/coop_active_runtime.cpp`：`Authority`、`ScriptInput`、`PublishTickBoundary`、`TopExit`、`PublishClientProgress`、`RetireB` 和 `IsaacCsT0CoopActiveInitialize`。
- `cs-t0/native/src/coop_input.cpp`：`Participant::Begin/End/AcceptInput/Progress/FillAvailability`、`NoteValidArrival`、`CurrentMissingReason`。
- `cs-t0/native/src/authority_tick_pipeline.cpp`：candidate tick、双 Participant Begin、Native normal return、post-state、receipt 和 publication 顺序。
- `cs-t0/native/src/input_core.cpp`：`NextInputTarget`、`NextStartupTarget`、`ScriptDrainAllowed`。
- `cs-t0/native/include/isaac_cs_t0/coop_active_ipc.hpp`、`coop_input.hpp`：EventKind/ABI、Shared v5、ScriptStep 和 availability/receipt 字段。
- `cs-t0/native/include/isaac_cs_t0/target_image.hpp`：批准 canonical/unpacked PE 身份。

已确认的关键事实：

1. `Submit` 事件的实际序号字段是 `event.command`；Event 结构没有独立的 `sequence` 成员。
2. 在 `input_lead_ticks=2` 的正常脚本发布中，`ScriptInput` 在 clock Begin 之前读取 `Completed()`，普通命令的 `target - Submit.tick` 为 2；首个 startup 命令目标为 `completed+1`，因此恰好有 1 行 delta=1 的 startup 例外。
3. 源停发长度 N 对应 effective 缺失 N tick，effective 窗口相对源事件窗口后移 2 tick。最后一个有效缺失 tick 已可能收到恢复 target 的未来命令，故该 tick 的 availability 原因为 `FutureOnly`；更早 tick 为 `NoTargetCommand`。1-tick 窗口只有 `FutureOnly`。
4. `AuthorityTickReceipt`：`query=receipt.kind`、`detail=post.status`、`action=post.reason`、`boolean=current_binding`。
5. committed `AuthorityTickBoundary` 的 `before` 为 `[completed.lo, completed.hi, current.lo, current.hi]`；提交时 `current=0`，`completed=event.tick`。
6. `ClientInputProgress` query7 的 `before` 为 `[processed_generation.lo, hi, processed_count.lo, hi]`，edge 处理数在 `before[2..3]`。
7. `ScriptComplete.detail` 是完成时的脚本索引（本场景 200）；正常退休产生唯一 ordinal 1 `Retired.boolean=1`。
8. world-record 的 `capture` 记录即使 `complete=false`，仍可包含只读 Player `position.x/y` 的 `VALUE` 字段；这种情况只能作为明确 partial 证据，不能改写为完整世界。

## 2. 实现前失败方式与对策

| 失败方式 | 对策/检查 |
| --- | --- |
| 误用非批准 PE，Native 在 Active 前返回 `Error::Image` | 统一入口先按 `target_image.hpp` 结构校验 on-disk PE；失败不创建测试 artifact，也不注入 |
| A 物理动作晚于 B 脚本首个 gap | 可选 controller start delay，让授权外部动作先开始；事件仍从真实注入后 tick0 起分析 |
| A 持续方向动作使 A 穿门/死亡 | 有界 W/D/S/A 中心方形动作；报告要求窗口内无 pause/loading/room/death/rebind/teardown lifecycle，并检查 A 各停发段非零 resolve |
| 把 future command 当成当前有效输入 | 分别检查 `NoTargetCommand`/`FutureOnly`、`effective_available=false`、`neutral_applied=true`、B Resolve 零状态 |
| 停发时清队列/重绑/复用 sequence | 检查 source 与 target 内部缺口仅 1/5/30/120；source/effective 按 lead 对齐；恢复 sequence = 中断前 +1，generation 不变 |
| 旧 edge 重放 | 5 个新 edge accepted、retirement query7 processed_count=5；所有 gap Resolve edge=0 |
| timeout 错误退休 B/结束 session | 120-tick 窗口实际 TimedOut、timeout recovery，B 保持绑定，session 不 Failed；正常退休只在最终 target 之后 |
| 只观察 B 缺输入而忽略 A | A 每段有实际非零 Resolve，world capture 中 A Player 位置改变；B 物理位置只作独立记录，不把“位置未动”当输入中性证明 |
| 把 partial/gap world 说成完整世界 | world capture 的 complete/partial/reason 全量记录；218 个 partial `NOT_READY`、0 complete；Player 字段只作 partial 证据 |
| 停止流程未恢复或自动重试 | 最终 target effective 且额外 commit 后才发送一次 stop；检查 70 槽、CAPTURE_CLOSED、consumer 关闭、controller exit0 和进程仍存活；`automaticRetry=false` |

## 3. 实现审阅结果

新增/修改文件：

- `cs-t0/testdata/tx-01-98-source-stop-plan.json`：9 段相对 target 场景、完整失败方式、lead/gap/FutureOnly/edge/timeout 规定。
- `cs-t0/testdata/tx-01-98-source-stop-script.json`：200 个 step、四个内部缺口 1/5/30/120、5 个新 drop edge。
- `cs-t0/testdata/tx-01-98-input-config.json`：显式 2/64/0/64/258 配置。
- `cs-t0/build-authority-source-stop.ps1`：纯编译 Rust Controller、Native 和 Helper；不运行测试/游戏/哈希。
- `cs-t0/run-authority-source-stop-e2e.py`：批准 PE 预检、可选 start delay、一次 controller、final target effective 后额外 commit 再 stop、产物/源码/环境保存。
- `cs-t0/report-authority-source-stop.py`：只读事件/世界/restoration 分析，24 个统一检查；partial world 明确记录；不访问游戏。
- `cs-t0/accept-authority-source-stop.ps1`：统一入口。
- `docs/archive/refactor/cs-t0/t0-07/tx-01-98-source-stop-e2e.md`：执行前范围与失败方式。

静态审阅期间发现并修正的具体问题：

1. 分析器原先把 Event 的 accepted command 序号写成不存在的 `sequence` 字段；改为 `command`，并把 startup 例外从“容忍两个偏差”收紧为恰好 1 个 lead-1 行。
2. 原先把 edge processed_count 读到 `before[1]`；改为 `before[2..3]` 的 32+32 组合。
3. 原 orchestrator 在任意 kind35 tick 达到 final target 时就可能停止；改为记录 final target **effective** availability，并要求一个额外 committed boundary 后才 stop。
4. 原报告把 A receipt 只计数；改为检查 receipt 的 tick/query/detail/action/boolean 与对应 commit/boundary 一致。
5. 原报告要求所有 missing tick 都是 `NoTargetCommand`；静态核对 lead 行为后改为每段尾部允许且要求一个 `FutureOnly`，并验证 1-tick 窗口形状。
6. 原报告把 world record 限制为 `complete=true`；实机 partial `NOT_READY` 暴露后改为记录 complete/partial/reason，仅使用可用 Player 字段作为 partial 证据，不宣称完整世界。
7. 原场景使用未受限 A/D，实机中 A 穿门并死亡；改为 bounded W/D/S/A 方形动作并增加 `stable_run_and_room`。

## 4. 历史失败与最终验收

- 第一次实机 invocation：**FAIL，注入前**。错误 GameExe 的 D-drive PE header 不匹配；Native 返回 `Error::Image=2`；0 captures/0 restoration。保留在 `tx-01-98-source-stop-e2e-01`。
- 第二次：**FAIL 20/22**。正确目标，但 A 动作晚到，前三段无 A 物理证据；world 全部 partial 尚未被分析器明确消费。保留在 `tx-01-98-source-stop-e2e-02`。
- 第三次：**FAIL 21/23**。A 启动已提前，但 A/D 动作穿门进入 room 1.1100；A 在 tick185 Death，120-tick 段无绑定 A 动作。保留在 `tx-01-98-source-stop-e2e-03`。
- 最终有效 invocation：**PASS 24/24**，exit0。见[结果](tx-01-98-results.md)和 `artifacts/cs-t0/tx-01-98-source-stop-e2e-04/report/result.json`。

## 5. 结论边界

本操作结论仅覆盖一个批准 unpacked 目标、一次 New Run、一个开始房间、native B、当前配置和标准/odd Authority 路径。**不是真实网络断线或 wire ACK 测试；没有 complete world sample；不外推到所有角色/武器/敌人/死亡/换房/loading/Restore/Replace。** 用户于2026-10-20基于操作1–8已有记录与本操作9的24/24实机结果确认 **T0-07整体PASS**；该PASS保留上述事实边界。本轮未计算/校验哈希，未进入原版 Online/Isaac Sync，未新增 gate/unlock。
