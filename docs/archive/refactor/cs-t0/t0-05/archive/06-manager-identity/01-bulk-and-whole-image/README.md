> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Bulk 与 Whole-image 闭包

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

本目录保存 direct bulk helper、copy/move、memset、可执行区间扫描和 v26–v33 remaining frontier。

## 当前合并结论

- [v33 frontier](native-join-manager-this-bulk-remaining-frontier-v33-handoff.md)及其[独立审计](../02-copy-move/native-join-manager-this-bulk-remaining-frontier-v33-independent-audit-v1-handoff.md)是当前机器前沿：copy accepted `323/405`、remaining `82`，memset accepted `225/242`、remaining `17`，总 remaining `99`。v33 只接纳 exact16 独立审计通过的 `0x0061F2C2` 与 `0x0068E3DB`；R04 两行保持未移除。
- [successor validator 交接](native-join-manager-this-bulk-remaining-frontier-successor-v30-independent-validator-v1-handoff.md)只证明合成与篡改拒绝逻辑已封印。
- [whole-image v5](native-join-manager-this-whole-image-alias-handoff-v5.md)修正 executable-range 扫描边界并保留 indirect/split component；它不是尚未生成的 promotable raw whole-image v2。

## 版本与专题

| 系列 | 当前入口 | 历史/组成记录 |
| --- | --- | --- |
| remaining frontier | [v30](native-join-manager-this-bulk-remaining-frontier-v30-handoff.md) + [v30 audit](native-join-manager-this-bulk-remaining-frontier-v30-independent-audit-v1-handoff.md) + [v31](native-join-manager-this-bulk-remaining-frontier-v31-handoff.md) + [v32](native-join-manager-this-bulk-remaining-frontier-v32-handoff.md) + [v33](native-join-manager-this-bulk-remaining-frontier-v33-handoff.md) | v26 仅保留冻结兼容原文；v31/v32/v33 是连续的 append-only positive successor |
| whole-image/executable range | [v5](native-join-manager-this-whole-image-alias-handoff-v5.md) | v3/v4 正文已由 v5 吸收并清理 |
| direct bulk | [copy/move phase 2](native-join-manager-this-bulk-copy-move-handoff-v2.md)、[helper](native-join-manager-this-bulk-helper-handoff-v1.md) | 保留各 helper destination 与 caller 分类 |
| memset | [closure v4](native-join-manager-this-bulk-memset-handoff-v4.md) + [range audit v5](native-join-manager-this-bulk-memset-range-audit-v5-handoff.md) | v1/v3 正文已由 v4 吸收并清理 |

任何 whole-image 提升都必须等待 `647/647 fullyAdjudicatedNoOverlap` 的正向 zero frontier；负向 coverage 字段不能替代该条件。
