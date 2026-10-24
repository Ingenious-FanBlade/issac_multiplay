# CS-T0-05-04 原生 Join 创建与发布用户规划假设 v1

日期：2026-09-06  
状态：`USER_ACCEPTED_PLANNING_ASSUMPTION / STRICT_EVIDENCE_UNCHANGED`

## 1. 用户决策

用户要求现阶段先假定：在当前固定 Win32/x86 目标和既定 Offline/local-co-op
场景中，Authority 可以通过游戏原生 local co-op Join 路径，可靠地创建并发布第二玩家
`B`。

后续规划允许使用以下抽象后置条件：

1. 原生 Join 返回一个非空、与 `A` 不同且可作为普通 local-co-op Player 使用的
   fresh `B`；
2. `B` 被发布到主 Player vector 的冻结 ordinal `1`；
3. `B` 的 assignment、HUD/index 与 `Isaac.GetPlayer(1)` 在同一 run generation 中
   指向同一个已发布 Player；
4. 在同一固定目标和相同前置场景下，该创建与发布能力可被后续设计视为可靠可用，
   除非目标构建变化、出现反例或用户重新打开本假设。

该假设使后续文档可以把工作重心转向 `CS-T0-05-05` 的 virtual-device 必要性决策，
不再把“原生路径是否存在并能发布 B”作为规划层的首要未知项。

## 2. 证据标签与保留结论

本记录是用户接受的规划假设，证据等级为 `DESIGN_ONLY`；它不是
`STATIC_CONFIRMED`、`RUNTIME_OBSERVED`、正式 gate `PASS` 或实机 source。

以下既有结论全部保持不变：

- `CS-T0-05-04` 严格机器状态仍为 `BLOCKED_BY_EVIDENCE`；
- plan v10、join progress v4、矩阵第 11 项及其全部历史 hash 不改写；
- manager-this 的 v33 strict frontier、MT-R02 等剩余 `UNKNOWN` 不改判；
- Init/P0 状态保持、自然更新窗口、reader、generation/ABA、manager context、
  publication 前置证明和恢复证明仍按原证据链保留；
- 历史失败、反例、sealed candidate、独立 audit 和权限字段均不得由本假设覆盖。

## 3. 本假设不包含的声明

“可靠地创建并发布 B”只作为后续设计的功能前提，不额外证明：

- Join 前 `ModeEvidence=Offline` 的生产 reader 与主动准入已经实现；
- save、achievement、unlock、统计、Cloud/Steam 或 run 外持久污染已被排除；
- 所有 Hook、publication、drain、retire 和目标字节恢复已经通过实机验收；
- manager-this 同代身份、全程序无别名/无越界、reader 完整性或原生 ABA 已闭合；
- B gameplay input、virtual device、双 Endpoint 输入隔离或 1,000 tick soak 已通过。

## 4. 权限与下游边界

本轮只修改文档。没有启动或打开游戏进程，没有使用 x32dbg/IDA 动态能力，没有注入、
Hook、runtime mutation、arm/token/claim、active source 或权限提升。

本假设允许开始 `CS-T0-05-05` 的只读设计和决策准备，但不构成其正式前置 gate：

- `activeExperimentAllowed=false`；
- `runtimeMutationAllowed=false`；
- `csT00505Unlocked=false`；
- `t005Unlocked=false`；
- T0-06 保持关闭。

任何实际 active Join、virtual-device canary 或游戏输入实验仍须满足各自的当前证据门、
独立验收和新的单次授权，不能复用本规划假设充当运行时证据。
