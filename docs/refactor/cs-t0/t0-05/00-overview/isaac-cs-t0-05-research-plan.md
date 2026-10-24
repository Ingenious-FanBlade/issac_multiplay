# T0-05 当前研究目标与范围

更新时间：2026-10-16；整理：tx-01-44。

执行约定以[现行项目规范](../../../project-conventions.md)为准。本文提炼仍适用的功能目标；旧工作包编号只作内容索引。旧版完整计划和实施时间线见[归档计划](../archive/00-overview/isaac-cs-t0-05-research-plan.md)。

## 目标

在真实 isaac-ng.exe 的 Offline/local-co-op Listen Server Authority 中，以游戏原生 local co-op 语义承载固定两名玩家，使用统一 scheduler，把各自 canonical 输入应用到对应玩家；Authority 日常 tick 不等待全员输入。

| 参与者 | 固定身份 | 输入路径 |
| --- | --- | --- |
| A | ordinal 0，session 内 ClientId 映射冻结 | 真实物理输入经 loopback，进入统一 canonical/sequence/target tick/queue/dedupe/Resolve/apply |
| B | ordinal 1，session 内 ClientId 映射冻结 | scripted / FaultInjectionTransport 的意图经同一管线，交给自有虚拟设备 |

Player/device/control 指针、K 和平台 handle 只用于当前 generation 的临时关联，不进入 canonical 输入或网络身份。房主没有即时旁路；B 不直接写 Player、device cache 或世界。

## 已采纳设计与证据范围

- 使用自有对象 38 槽 Base vtable、六个覆写槽和自有六槽 control block；S2 action 层覆写，路线 A 原生 Join 为主，路线 B 直接重绑为备选。
- NEVER：BASE_CONSTRUCTED 后对象、control 和有效 base 状态在进程生命周期内不释放；base ctor 前固定 DLL，恢复后 DLL 常驻；每 session 至多一次设备创建，失败不自动重试。
- [契约 v2](../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)提供已采纳的具体功能约定；[v18](../07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-v18.md)提供限定正常路径的静态结论。writer 时点候选尚未采纳，不能覆盖契约。
- [原生 Join 创建/发布假设](../02-active-join/native-join-active-user-planning-assumption-v1.md)仅为 USER-ASSUMPTION / DESIGN_ONLY。它不替代持久化、实际绑定、实现和运行时观察。
- v18 的范围限定为有效对象和缓冲、主线程顺序操作、Offline/local-co-op、Online roster 为空、B 唯一正 K、创建 B 前 Main Player vector 只有 A。完整 Init、global manager-this、全 alias/observer、全部 Mod、worker/EH 及完整网络闭包不由该报告证明。

## 后续实现需解决的问题

1. 明确 S2 current/previous 的唯一写入时点，处理候选与契约 v2 §4 的差异，并保留采纳与实现状态。
2. 按已理解 ABI 接入自有 factory、vptr/control、K 注册与读取、JoinControl 和 B actual-create attempt owner。
3. 从 A-only V1 扩展为 participant 各自的 queue、sequence/history、planner、pending 与 Resolve；共享一次 Authority world apply，不等待另一参与者。
4. 区分 CurrentEligible 与只供 level-derived Triggered 使用的 PreviousComparable；显式 edge、held V2、重复查询与失效中性的职责按 v18 和其继承依据实现。
5. 接入 room、death/respawn、设备重绑、pause/loading/menu 和 B-only retirement；失效后不复活旧输入或旧对象关系。
6. 在实际实现范围内列明分配失败、并发、COL/RTTI、注册/assignment、持久化、Hook 恢复和现场未知问题，按该次完整目标安排最终验收，不另设证明门或提前测试。

以上是功能问题清单，不构成下一次工作的预授权，也不要求先清零历史 647 行、alias universe 或解锁字段。

## 完成边界

完成 T0-05 需有固定 A/B 的实际创建、发布和映射结果，双端输入隔离与 exactly-once、缺输入时 Authority 正常推进、生命周期失效/重绑/退休和 Hook 恢复的实际结果。当前尚未完成这些完整功能。

C/S 与原版 Online Lockstep、Isaac Sync 1.0.4 的 session、协议、IPC、日志及可变状态保持隔离，不复用 Online Manager、NetInputDevice、FrameStore 或原版全员 ready 条件；保留 ISCS protocol 5、ABI 5、七项 Lua API、channel 0 和 0xFD 的既有行为。

T0-05 不代表可玩 C/S 联机、Replica World、snapshot/baseline/delta apply、预测、rollback、late join、Dedicated Server 或全部 Mod 兼容已经实现。更晚阶段范围见[总体架构](../../../isaac-cs-architecture-plan.md)与[T0 任务书](../../isaac-cs-t0-task-plan.md)。

## 推进与验收

每次推进最小为 tx-0x-0x；说明完整目标、范围与预期结果。分析、修改期间零测试，整个目标完成后只通过一个统一入口执行一次整体验收；文档任务采用一次整体静态文档审阅。实际游戏输入、注入和运行时修改按用户实际授权执行，不从历史结果推导权限。
