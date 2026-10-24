> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Native Join 主动路径

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

本目录整合 ordinary native join 的静态路径、preflight 实现和独立测试边界。

## 当前结论

用户现已在规划轨接受“原生 local-co-op Join 可以可靠创建并发布第二玩家 B”的功能假设；后续只读设计可据此转向 virtual-device 必要性决策。该假设不改写严格证据：完整 Init/自然更新期间的零持久污染、manager context、同代身份、reader/ABA 与恢复仍未闭合，active runtime、55 槽事务、IPC/arm/claim 和实机 source 均未开始，`CS-T0-05-04` 仍为 `BLOCKED_BY_EVIDENCE`。

## 证据链

| 记录 | 用途 |
| --- | --- |
| [主动路径逆向交接 v1](native-join-active-reverse-v1.md) | 冻结 UI/selector/ordinary join 静态路线及原始阻断 |
| [有界补核纪要 v1](native-join-active-followup-v1.md) | 收窄“无独立 prejoin 依据”的旧表述；不关闭持久化阻断 |
| [实施记录 v1](native-join-active-implementation-v1.md) | 只读 `coop-join-preflight`、历史快照和 fail-closed 行为 |
| [独立测试计划 v1](native-join-active-test-plan-v1.md) | active 资格、零权限和未来实机验收边界 |
| [独立测试结果 v1](native-join-active-test-results-v1.md) | 阻断门、继承回归及失败保留 |
| [用户规划假设 v1](../../02-active-join/native-join-active-user-planning-assumption-v1.md) | 允许后续规划假定原生 Join 可可靠创建/发布 B；不构成证据 PASS 或运行时授权 |

阅读当前状态时按“reverse -> followup -> implementation/test result”合并理解；前两份之间不是互相否定，而是从宽泛阻断收窄到精确剩余阻断。
