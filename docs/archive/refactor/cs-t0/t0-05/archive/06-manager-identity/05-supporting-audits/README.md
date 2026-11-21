> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Manager Identity 辅助与组合审计

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

本目录放置跨越单一 copy/fill 家族、但为当前 frontier 提供独立约束的记录。

| 记录 | 作用 |
| --- | --- |
| [string extent v9](native-join-manager-this-string-extent-v9-handoff.md) | string/SSO extent 与 owner 生命周期的静态组件 |
| [Theora exact11 independent audit v1](native-join-manager-this-theora-plane-state-eleven-v24-independent-audit-v1-handoff.md) | Theora plane/state 行的作者分离审计 |
| [save v21 + PNG/zlib v20 independent audit v1](native-join-manager-this-save-v21-png-zlib-v20-independent-audit-v1-handoff.md) | 跨 copy/fill 两条 producer 记录的组合独立审计 |

组合审计只对其绑定的 exact rows 和冻结输入生效；不能横向推广到同家族其它行，也不能替代 v30/未来 positive frontier 的全集校验。
