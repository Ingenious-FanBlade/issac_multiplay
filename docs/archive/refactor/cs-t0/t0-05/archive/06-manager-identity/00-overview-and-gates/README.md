> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Manager identity 总览、Reader 与研究资料

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

本目录把 manager context/receiver 的局部公式、production reader、generation/ABA 设计、暂停状态和未来 v7 独立验收串成一条当前阅读链。

## 当前入口

1. [manager receiver 当前进度](native-join-manager-receiver-progress-v1.md)：局部已确认公式与九个完整 context 前提。
2. [暂停检查点 v32](native-join-manager-this-pause-checkpoint-v32.md)：当前接受的 v30 负向机器前沿和暂停边界。
3. [完整证明任务分解 v33](native-join-manager-this-proof-completion-work-breakdown-v33.md)：exact103 的 29 个正向微任务、zero frontier、whole-image 与 formal v7 发布链。
4. [MT-R01–R10 完成检查点 v34](native-join-manager-this-mt-r01-r10-completion-checkpoint-v34.md)：前十项 exact20 的逐行裁决、v32 positive frontier 与 remaining exact101。
5. [MT-R01–R10 后续完成优先级 v35](native-join-manager-this-mt-r01-r10-followup-priority-v35.md)：剩余 18 行的 `P0`–`P3` 顺序、执行波次、停止条件、压缩测试与动态补证边界。
6. [MT-R01–R10 后续检查点 v36](native-join-manager-this-mt-r01-r10-followup-checkpoint-v36.md)：exact16 作者分离 follow-up、两条新增正向行、R04 测试豁免边界与 v33 remaining exact99。
7. [MT-R02/R03/R05 检查点 v37](native-join-manager-this-mt-r02-r03-r05-checkpoint-v37.md)：exact5 深入静态复核、0 新正向、5 UNKNOWN 与压缩独立验收。
8. [MT-R05 filtered-vector 检查点 v38](native-join-manager-this-mt-r05-filtered-vector-checkpoint-v38.md)：单行有界静态复核、0 positive / 1 UNKNOWN 与候选前封印的压缩独立验收。
9. [MT-R05 用户假设策略检查点 v39](native-join-manager-this-mt-r05-user-assumption-policy-checkpoint-v39.md)：policy-only assumption waiver PASS；strict remaining 99 保留，人工逆向队列降为 98。
10. [MT-R03 用户假设策略检查点 v40](native-join-manager-this-mt-r03-user-assumption-policy-checkpoint-v40.md)：exact2 policy-only assumption waiver PASS；strict remaining 99 保留，累计人工逆向队列降为 96。
11. [MT-R02 animation descriptor 检查点 v41](native-join-manager-this-mt-r02-animation-descriptor-checkpoint-v41.md)：exact2 有界静态复核、0 positive / 2 UNKNOWN 与 R02-only 压缩独立验收；strict remaining 99、人工队列 96 均不变。

## 组成记录

| 主题 | 记录 | 合并后的作用 |
| --- | --- | --- |
| manager context | [context v1](native-join-manager-context-v1.md)、[receiver v1](native-join-manager-receiver-v1.md) | guard 与 receiver 局部公式；完整 context/identity 仍未知 |
| reader | [静态交接 v1](native-join-manager-this-reader-handoff-v1.md)、[实现 v1](native-join-manager-this-reader-implementation-v1.md) | production component 与 fail-closed generation 契约；不等于 whole-image/ABA 闭合 |
| generation | [local epoch v2](native-join-manager-this-local-epoch-handoff-v2.md) | 仅同步 local causal cone 条件闭合；全线程 wall-clock alias 仍未知 |
| v7 验收 | [独立测试计划](native-join-manager-this-v7-independent-test-plan-v1.md)、[pre-seal 结果 v2](native-join-manager-this-v7-independent-test-results-v2.md) | 固定未来正式 v7 的 oracle；当前 raw/contract 未发布 |
| 历史暂停 | 已整合进 v32 与[过时文档迁移表](../../00-overview/obsolete-document-map.md) | v30 冻结原文只作旧路径兼容；v31 正文已清理 |

暂停检查点 v32 只封印旧的负向覆盖，proof breakdown v33 定义严格任务接口；v34–v38 保留静态裁决链，v39/v40 依次记录 R05/R03 用户假设策略通过，v41 是当前人工逆向策略入口。MT-R02 本轮仍为严格 `UNKNOWN` 且没有 waiver；policy waiver 不改 v33 strict frontier，也不授予运行时、whole-image 或 formal v7 promotion 权限。
