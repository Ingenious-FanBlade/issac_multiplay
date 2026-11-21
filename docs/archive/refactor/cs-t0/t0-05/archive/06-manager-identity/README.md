> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Manager-this 同代身份

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

本目录整合 `CS-T0-05-04` 当前最大的静态阻断：caller 入口 manager、ordinary spawn 处重新读取的 GameRoot/PlayerManager，以及整个生命周期内可触达的 alias/bulk writer 是否属于同一存活 generation。

## 当前结论

| 项目 | 状态 |
| --- | --- |
| manager guard 公式 | `STATIC_CONFIRMED` |
| ordinary spawn receiver 局部公式 | `STATIC_CONFIRMED` |
| manager-this 同代身份 | `UNKNOWN / FAILED_MANAGER_THIS_IDENTITY_PREMISE` |
| copy/move bulk | `323/405` 正向接受，`82` remaining |
| memset bulk | `225/242` 正向接受，`17` remaining |
| remaining 负向审计 | `99/99`，仅证明 fail-closed，全部仍 `UNKNOWN` |
| 人工逆向策略队列 | MT-R03/R05 assumption waiver PASS 后 `96` remaining；不改 strict frontier |
| whole-image raw v2 / formal v7 | 尚不存在 |
| 当前工作状态 | `MT-R02_BOUNDED_STATIC_UNKNOWN / MT-R03_R05_USER_ASSUMPTION_POLICY_PASS / STRICT_BLOCKED_EXACT_99 / MANUAL_QUEUE_96` |

## 子域导航

| 子目录 | 内容 |
| --- | --- |
| [00-overview-and-gates](00-overview-and-gates/README.md) | receiver/context、reader、local epoch、暂停检查点与 v7 门 |
| [01-bulk-and-whole-image](01-bulk-and-whole-image/README.md) | bulk 分类、executable-range/whole-image 与 v33 机器前沿 |
| [02-copy-move](02-copy-move/README.md) | copy/move 405 行及 remaining exact82 的家族记录 |
| [03-fill-and-allocation](03-fill-and-allocation/README.md) | memset/fill 242 行、allocation/container 家族与 remaining exact17 |
| [04-manager-tuple](04-manager-tuple/README.md) | manager tuple writer/helper/live-out/global seal |
| [05-supporting-audits](05-supporting-audits/README.md) | 跨家族 extent、codec 与组合独立审计 |

当前人工逆向策略从 [MT-R02 animation descriptor checkpoint v41](00-overview-and-gates/native-join-manager-this-mt-r02-animation-descriptor-checkpoint-v41.md) 进入；MT-R02 exact2 的 strict verdict 仍为 `UNKNOWN` 且没有 waiver，v33 strict frontier 与 v40 人工队列均不变。任务范围与静态证明接口仍以 [proof breakdown v33](00-overview-and-gates/native-join-manager-this-proof-completion-work-breakdown-v33.md) 为唯一依据；policy waiver 只改变人工队列，不覆盖 strict remaining 集。
