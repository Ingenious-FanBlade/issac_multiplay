# tx-04-08：cs world 命令与后台准备

> 后续ABC续作增加仅本地控制口可用的 `cs world_test <token> <generation> <mode> <value>`；配置默认关闭，严格限定当前录制，不加入玩家控制台/Lua/UDP操作。模式、线程、失败语义和未覆盖项见[ABC说明](tx-04-08-abc-implementation.md)。下方四项通用命令及早先交付记录保留。

日期：2026-10-27，Asia/Singapore。按本次用户要求完成 `cs world` 的代码编写，属于同一 tx-04-08。依据[迁移设计第 3.2、7、8 节](tx-04-08-implementation-plan.md)、[collector 接线契约](tx-04-08-collector-bridge.md)和[现行规范](../../refactor/project-conventions.md)。本项只做源码修改、静态阅读与纯编译链接，未启动游戏、部署或运行测试；不将命令编写完成等同于完整 tx-04-08 验收。

## 命令

四项命令共用 Lua、REPENTOGON 控制台和既有本地测试控制口的 `Command/Execute` 入口。控制口线程只传送有界字符串；真正变更进入现有 64 项命令队列，在后续最外层 Manager 边界执行。没有增加 UDP/远端输入协议命令。

| 命令 | 当前行为 |
| --- | --- |
| `cs world start <label>` | 检查配置、world/loop/player 能力与双层观察注册值；busy 时拒绝。成功 admission 返回 `queued/request`、`recording`、单调 `generation` 与 `world_state=Preparing`。后台完成准备后才接收上下文，下一最外层 Manager 才激活，不能把排队成功视为已录制。 |
| `cs world stop` | 排队后返回请求和 Closing/Closed 意图；Preparing 时允许取消，Recording 时停止新观察并等待在途作用域归还。重复调用幂等；实际 Closed 由后续状态确认。 |
| `cs world status` | 返回短值快照；已入队、尚未执行的 start 显示 `Preparing/phase=Queued`。有后台上下文时包括首因/阶段、请求/录制/generation、目录、attempt/complete/gap/背压、callback、checkpoint 生产/写入计数、reader/writer、drain/flush/result 和关闭超时。最后 Closed 快照保留到下一次 start。 |
| `cs world checkpoint <label>` | 仅在实际 Recording 时接受，再于命令边界尝试发布。返回排队请求不是落盘确认；按 request 关联 trace 和 diagnostics 中的 checkpoint，结合关闭结果确认交付。停止或故障先发生时可拒绝执行，trace 保留 `accepted=0`。 |

label 长度为 1–64 个 ASCII 字符，仅允许字母、数字、`._-`；拒绝 `.`、`..`、尾部点和 Windows 的 CON/PRN/AUX/NUL/COM1–9/LPT1–9 设备别名（含扩展名）。命令不接收目录路径，固定根来自启动配置。writer 与命令共用 `ValidWorldLabel`，最终目录和文件仍用 create-new；同 label 的既有产物不会被覆盖。配置的输出根不允许控制字符，后台按 UTF-8 转换为 Win32 宽路径。

## 准备、停止与所有权

新增 `mod/src/ops/world_commands.*`。主线程只复制版本、注册 mask、配置、编号和 `WorldMemoryReader` 的元数据。准备 worker 构造稳定 `WorldRecording`，依次准备 storage、identity、manifest/writer 和堆上 collector，通过 release/acquire Ready 发布；背景线程不调用游戏、Lua 或 memory.copy。

Manager 边界接收 Ready 后调用现有 `AttachPreparedWorld`；现有桥保持下一最外层入口才 Activate。stop 不停止 Host、不移除 B/K、不退休 C/S session，也不恢复 Hook。run/Restore 清理队列时只清会话命令，world 的 start/stop/checkpoint 保持原有顺序；录制可以跨 Host 和 run 边界保留真实退休记录。

collector 的 outer、生命周期、callback 和 pending 全部归还后才交出 producer。worker 等 reader 结束生产、writer 自然排空，再在后台取得两个线程的实际退出。不能提前停 writer，否则空队列可能早于 reader 结束而被误认为收口。主线程确认 Closed 后撤销桥和命令层引用，worker 最后释放大块内存及共享 ledger。主线程没有线程 join、文件 I/O 或大块初始化/释放。

`close_timeout_ms` 用于后台收口及 stop 后的 producer 等待。超时保留标记和 `THREAD_FAILED/close_timeout` 自有故障，不强杀线程、不卸载 DLL、不提前释放上下文；仍在运行时维持 Closing/busy。实际 Closed 是生命周期事实，错误首因和 `CAPTURE_ERROR/CAPTURE_UNCONFIRMED` 不被 Closed 覆盖。准备取消或失败不自动重启，新 start 必须等当前上下文真实收口。

## checkpoint 产物与文件结果

新增 `CheckpointRecord` 与独立 **128 项** SPSC 控制环；实际分配记入同一 256 MiB ledger，计划预算和 manifest 也登记其容量。该环、紧凑值队列和 writer 共用既有通知/背压/排空协议，满或通知失败只使 world 故障，不静默丢弃标记。

标记包含 label、request、独立 sequence、recording generation、run/room/outer/mutation，以及主线程已有的 attempt/capture/event_watermark。collector 不额外读取游戏对象，不清身份、生命周期或连续计数。writer 在 `diagnostics.jsonl` 输出 `kind=checkpoint`；所有新增 u64 字段为十进制字符串。文件交错不能当全序，必须按显式 watermark 关联 captures 与 lifecycle-events。

`result.json` 增加 checkpoints_produced/checkpoints_written/checkpoints_not_written。writer 新增原子 `result_written`，只有 result 自身写入并显式 flush/关闭成功才置真；结果文件失败会保留 FileError，不再忽略返回值后报告成功。Preparing 内的可抛分配异常会被 world 自有准备路径接住；writer 线程异常也进入原有自有失败路径。

## 编译与剩余范围

纯编译入口与日志见[本项构建记录](../../../artifacts/repentogon/tx-04-08-world-commands/README.md)。没有新增单元测试、运行 replay、probe、局部 E2E 或游戏。运行时的线程、文件失败、重复命令、超时、实际内存与时延仍待 tx-04-08 的唯一统一整体 E2E。

本项没有实现离线工具或 E2E 夹具，也不将工作区同时出现的 harness 编辑纳入本项验收。直接退出进程的完整协调仍是 ST8 已登记的缺口；当前可明确安排的健康路径是显式 stop、确认真实 Closed 后退出。`InitUnknown` 字段保持 UNCONFIRMED，2,000 次完整普通采集和两进程对照均未完成，历史失败与限定事实保持原样。
