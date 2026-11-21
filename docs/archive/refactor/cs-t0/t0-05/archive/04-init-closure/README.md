> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Init、Fresh-B 与 P0 累计闭包

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

本页是 ordinary Player 初始化、fresh-B publication/source preservation、manager guard/receiver，以及 cache/familiar/P0 支线的主题入口。版本化研究记录已集中移入 [`archive/`](archive/README.md)；上级主线文档、冻结机器产物及其绑定路径不变。

## 当前合并结论

`managerReceiverFormulaStatus=STATIC_CONFIRMED`，但 `managerThisIdentityStatus=UNKNOWN`。publication identity、fresh-B source preservation、desired familiar/P0 topology、完整 manager return、自然更新窗口和 reader/ABA 仍未整体闭合，因此 `synchronousPropagationComplete=false`，全部 runtime/active/live 权限保持 false。

## 当前阅读入口

- [Init closure v8](archive/native-join-init-closure-v8.md)：累计 manager receiver 章节，并保留全部未决前提。
- [Cache/Familiar/P0 v5](archive/native-join-init-v4-cache-p0-v5.md)：该静态支线的当前累计入口。
- [研究过程归档](archive/README.md)：v1–v8 与 Cache/Familiar/P0 v1、v4、v5 的完整索引。

## 归档边界

- 支线的局部 consumer/数值结论不能替代完整 Init/P0 状态保持。
- `docs/archive/refactor/cs-t0/t0-05/` 根目录中由冻结 JSON、脚本或历史审计绑定的兼容原文保持原位，不重签、不改写。
- 目录整理不新增逆向事实，也不改变 `BLOCKED_BY_EVIDENCE`、权限或解锁状态。
