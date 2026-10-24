> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Copy/Move 证据族

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

本目录按 destination backing/extent 与 owner 生命周期家族整合 copy/move 记录。`MT-R01`–`MT-R10` follow-up 后，v33 当前正向接受 `323/405`，exact82 remaining 均保留独立负向审计覆盖并继续为 `UNKNOWN`。

## 家族索引

| 家族 | 记录 |
| --- | --- |
| range/backing 基线 | [range bounds v7](native-join-manager-this-copy-range-bounds-v7-handoff.md)、[backing six v8](native-join-manager-this-copy-backing-six-v8-handoff.md)、[backing four v9](native-join-manager-this-copy-backing-four-v9-handoff.md)、[critical two v10](native-join-manager-this-copy-backing-critical-two-v10-handoff.md)、[hard eleven v10](native-join-manager-this-copy-hard-eleven-v10-handoff.md) |
| caller/heap | [v5 累计入口](native-join-manager-this-copy-move-caller-heap-v5-handoff.md)、[residual exact17 v29](native-join-manager-this-copy-move-caller-heap-residual-exact-seventeen-v29-handoff.md)、[独立审计](native-join-manager-this-copy-move-caller-heap-residual-exact-seventeen-v29-independent-audit-v1-handoff.md)；v3/v4 正文已清理 |
| local stack/hard | [local-stack audit v4](native-join-manager-this-copy-move-local-stack-v4-independent-audit-v1.md)、[local-hard v5](native-join-manager-this-copy-move-local-hard-v5-handoff.md)、[exact5 v28](native-join-manager-this-copy-move-local-hard-exact-five-v28-handoff.md)、[独立审计](native-join-manager-this-copy-move-local-hard-exact-five-v28-independent-audit-v1-handoff.md) |
| vector grow | [v6](native-join-manager-this-copy-move-vector-grow-v6-handoff.md)、[exact23 v27](native-join-manager-this-copy-move-vector-grow-exact23-v27-handoff.md)、[独立审计](native-join-manager-this-copy-move-vector-grow-exact23-v27-audit-v1-handoff.md) |
| vector shift | [v7](native-join-manager-this-copy-move-vector-shift-v7-handoff.md)、[exact25 v28](native-join-manager-this-copy-move-vector-shift-exact25-v28-handoff.md)、[独立审计](native-join-manager-this-copy-move-vector-shift-exact25-v28-independent-audit-v1-handoff.md) |
| string | [exact14 v29](native-join-manager-this-copy-move-string-exact14-v29-handoff.md)、[独立审计](native-join-manager-this-copy-move-string-exact14-v29-independent-audit-v1-handoff.md) |
| EntityList arena | [exact2 v25](native-join-manager-this-copy-entity-list-arena-two-v25-handoff.md)、[独立审计](native-join-manager-this-copy-entity-list-arena-two-v25-independent-audit-v1-handoff.md) |
| save record | [v21](native-join-manager-this-copy-save-record-one-v21-handoff.md)、[v22](native-join-manager-this-copy-save-record-one-v22-handoff.md)、[seed census v23](native-join-manager-this-copy-save-record-one-seeds-v23-handoff.md)、[successor-first v24](native-join-manager-this-copy-save-record-one-successor-first-v24-handoff.md) |
| `MT-R01`–`MT-R10` 正向恢复 | [当前策略 v41](../00-overview-and-gates/native-join-manager-this-mt-r02-animation-descriptor-checkpoint-v41.md)、[R02 bounded successor](native-join-manager-this-mt-r02-animation-descriptor-bounded-successor-v3-handoff.md)、[R02 exact2 audit](native-join-manager-this-mt-r02-exact2-sealed-independent-audit-v1-handoff.md)、[R03 waiver v40](../00-overview-and-gates/native-join-manager-this-mt-r03-user-assumption-policy-checkpoint-v40.md)、[R05 waiver v39](../00-overview-and-gates/native-join-manager-this-mt-r05-user-assumption-policy-checkpoint-v39.md)、[v33 frontier audit](native-join-manager-this-bulk-remaining-frontier-v33-independent-audit-v1-handoff.md)；v34–v38、exact16 与 Wave A–E audit 保留历史链 |

exact82 的严格机器分区仍为 `callerHeap 13`、`localHard 7`、`vectorGrow 23`、`vectorShift 25`、`string 14`。前十项累计静态接受 `0x001CBEB5`、`0x00689F63`、`0x0061F2C2`、`0x0068E3DB`；MT-R03 exact2 与 MT-R05 exact1 的严格证据仍为 UNKNOWN，但已按用户风险接受取得 policy-only PASS 并退出人工逆向队列。MT-R02 exact2 经 v41 有界复核仍为严格 `UNKNOWN`、未获 waiver，故人工队列保持 exact79 copy/move、exact17 memset。其余行的 `audit PASS` 只表示相应 contract 被接受，必须区分静态正向与 policy waiver。
