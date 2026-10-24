> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Manager-this MT-R05 用户假设策略检查点 v39

日期：2026-09-06  
状态：`PASS_USER_ACCEPTED_ASSUMPTION_WAIVER_ONLY / STRICT_EVIDENCE_UNKNOWN / MANUAL_QUEUE_98`

## 1. 用户决策与当前结论

用户基于该版本游戏长期游玩中未观察到相关崩溃，明确接受 MT-R05 的剩余风险，并要求先假设 `0x0033570B` 的 `memmove` 不会发生 32 位长度下溢。该 exact1 因此在人工逆向策略轨确认为：

`PASS_USER_ACCEPTED_ASSUMPTION_WAIVER_ONLY`

MT-R05 已从后续人工逆向队列移除，不再继续追踪 `n298+0x2A434/+0x2A438` 的 producer/lifetime fixed point，除非用户重新打开该假设或目标构建发生变化。

## 2. 严格证据与策略通过分离

本决策是用户风险接受，不是新增静态或运行时证据：

| 轨道 | MT-R05 状态 | 剩余计数 |
| --- | --- | --- |
| 严格证据 | `UNKNOWN`；`countAndNoWrap=OPEN`；`STATIC_CONFIRMED_NO_OVERLAP=false` | copy/move `82`，memset `17`，总计 `99` |
| 人工逆向策略 | `PASS_USER_ACCEPTED_ASSUMPTION_WAIVER_ONLY`；从队列移除 exact1 | copy/move `81`，memset `17`，总计 `98` |

保留的静态 countermodel 仍为：

`auxiliaryRangeCount>0 && filteredCount==0 yields raw transfer length 0xFFFFFFFC`

因此 [v33 strict frontier](../01-bulk-and-whole-image/native-join-manager-this-bulk-remaining-frontier-v33-handoff.md) 不修改，`0x0033570B` 仍保留在 strict `callerHeap`、`remainingRvas` 与 accepted-negative coverage 中。该 waiver 不得作为 zero frontier、whole-image raw v2、MT-W01 或 formal v7 的静态正向输入。

## 3. 候选前封印与作者分离验收

独立 TEST 在候选发布前封印 assumption-waiver contract：

- [候选前封印交接](../02-copy-move/native-join-manager-this-mt-r05-assumption-waiver-sealed-preparation-v1-handoff.md)；
- sealed spec SHA-256：`4977DBEE6D2DDAC78E5B25EF9BEE87B9D0D27DCE3DB8BF49FD595FED76D387AE`；
- preparation SHA-256：`8181ED97110240B9D671039062971AE29BDD1D8101D34297824A82631EE6E357`。

策略候选与独立审计：

- [waiver candidate 交接](../02-copy-move/native-join-manager-this-mt-r05-assumption-waiver-v1-handoff.md)；
- candidate SHA-256：`04B713B932028873756C860057D138B2F1E63EFF481B812CF0B369479EBD629A`；
- [独立审计交接](../02-copy-move/native-join-manager-this-mt-r05-assumption-waiver-sealed-independent-audit-v1-handoff.md)；
- independent audit SHA-256：`92FC838C9063514508D79BCE65174D7A57ACDB4BBEA01BF6E28048506E295C5A`。

独立 verdict 为 `PASS_USER_ACCEPTED_ASSUMPTION_WAIVER_ONLY`。验收同时确认静态 verdict 继续为 `UNKNOWN`，没有 `STATIC_CONFIRMED_NO_OVERLAP`、`RUNTIME_OBSERVED` 或 frontier composition promotion。

## 4. 压缩测试与权限边界

候选作者仅执行一次 create-new、一次 verify-existing 与三项核心 mutation。独立 TEST 的 preparation 和 candidate audit 各只执行一次 create-new、一次 verify-existing、三类必要 mutation 与一次 wrong-hash fail-before-write。

没有运行 Cargo、全量 T0 build、历史宽回归、重复 self-test、游戏、进程读取、x32dbg、注入或 IDB 修改。manager-this、whole-image、runtime、active、05-05、T0-05 总门和 T0-06 权限全部保持关闭。

