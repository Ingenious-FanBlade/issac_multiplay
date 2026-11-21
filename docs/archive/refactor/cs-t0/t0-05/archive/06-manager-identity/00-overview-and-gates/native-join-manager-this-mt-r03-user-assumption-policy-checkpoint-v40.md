> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Manager-this MT-R03 用户假设策略检查点 v40

日期：2026-09-06  
状态：`PASS_USER_ACCEPTED_ASSUMPTION_WAIVER_ONLY / STRICT_EVIDENCE_UNKNOWN_EXACT2 / MANUAL_QUEUE_96`

## 1. 用户决策与当前结论

用户要求与 MT-R05 相同处理：基于该版本长期游玩中未观察到类似崩溃，接受 MT-R03 两个 copy callsite 的剩余风险，并先假设 intended use 不会产生超过固定 5000-element backing 的复制。以下 exact2 在人工逆向策略轨确认为：

`PASS_USER_ACCEPTED_ASSUMPTION_WAIVER_ONLY`

- `0x0001513A`，元素宽度 84；
- `0x000155A4`，元素宽度 108。

MT-R03 已从后续人工逆向队列移除；除非用户重新打开该假设或目标构建变化，不再继续 parser/interpolation 全路径的 5000 上界逆向。

## 2. 严格证据保持不变

本决策是用户风险接受，不是新增静态或运行时证据：

| RVA | strict verdict | first-open / retained countermodel |
| --- | --- | --- |
| `0x0001513A` | `UNKNOWN` | `backingExtent / R03_EMITTED_COUNT_LE_5000_OPEN_84`；`5001*84=420084 > 420000` |
| `0x000155A4` | `UNKNOWN` | `backingExtent / R03_EMITTED_COUNT_LE_5000_OPEN_108`；`5001*108=540108 > 540000` |

5000 比较仍只记录条件后继续，不是复制前的硬控制流上界。两行继续保留在 v33 strict `callerHeap`、`remainingRvas` 与 accepted-negative coverage 中；v33 strict frontier 仍为 copy/move `323/405`、memset `225/242`、总计 `548/647`、remaining `99`。

## 3. 累计人工逆向策略

| 项目 | v39 后 | 本轮移除 | v40 后 |
| --- | ---: | ---: | ---: |
| copy/move 人工队列 | 81 | MT-R03 exact2 | 79 |
| memset 人工队列 | 17 | 0 | 17 |
| 总人工队列 | 98 | 2 | 96 |

累计 assumption waiver 为两个任务、三个 RVA：MT-R05 `0x0033570B` 与本轮 MT-R03 两行。它们只从人工逆向调度中移除；strict whole-image、zero frontier、MT-W01 与 formal v7 仍不得把 waiver 当作 `STATIC_CONFIRMED_NO_OVERLAP`。

## 4. 候选前封印与作者分离验收

独立 TEST 在候选发布前封印 exact2 contract：

- [候选前封印交接](../02-copy-move/native-join-manager-this-mt-r03-assumption-waiver-sealed-preparation-v1-handoff.md)；
- sealed spec SHA-256：`2167D5368C43C52EF909AC16AD92C043E2BE77BBBD3F366BD647C5A991C19616`；
- preparation SHA-256：`C4E47E079CAE5D40788F892C9E2AC8CED097BAF2384E2E3A9BF774E60CF8F52A`。

策略候选与独立审计：

- [waiver candidate 交接](../02-copy-move/native-join-manager-this-mt-r03-assumption-waiver-v1-handoff.md)；
- candidate SHA-256：`A3A659A77B0167E71E63D6D216CBDE15E77103194C490A1273E8914EEA2EF865`；
- [独立审计交接](../02-copy-move/native-join-manager-this-mt-r03-assumption-waiver-sealed-independent-audit-v1-handoff.md)；
- independent audit SHA-256：`758F92C60890DE425E19932231019A7270148CD7FA01D29DD2BB92C472EA63D2`。

独立 verdict 为 `PASS_USER_ACCEPTED_ASSUMPTION_WAIVER_ONLY`。验收确认 exact2 strict verdict 继续为 `UNKNOWN`，没有静态/运行时 promotion，也没有修改 v33 frontier。

## 5. 压缩测试与权限边界

候选作者仅执行一次 create-new、一次 verify-existing 与三项核心 mutation。独立 TEST 的 preparation 和 candidate audit 各只执行一次 create-new、一次 verify-existing、三类必要 mutation与一次 wrong-hash fail-before-write。

没有运行 Cargo、全量 T0 build、历史宽回归、重复 self-test、游戏、进程读取、x32dbg、注入或 IDB 修改。manager-this、whole-image、runtime、active、05-05、T0-05 总门和 T0-06 权限全部保持关闭。

