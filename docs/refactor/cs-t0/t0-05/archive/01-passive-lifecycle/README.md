> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Passive 生命周期与适用性

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

本目录整合 `CS-T0-05-03` 的设备重绑和原生退出适用性记录。

## 当前结论

05-03 已以限域适用性门正式结束：五份适用 source 为 `PASS`，不可达的 native-exit 场景按冻结目标和 ordinary Variant=0 构造路径记为限域 `N/A`。旧六源门、live-07 `BLOCKED_BY_EVIDENCE` 和 5/6 历史进度继续保留，但不再作为当前 gate。

## 阅读顺序

1. [适用性 gate 迁移 v1](native-exit-applicability-gate-v1.md)：当前机器门、28/28 自动检查和 15/15 独立终审的权威入口。
2. [device-rebind v2 取证计划](device-rebind-v2-evidence-plan.md)：device live-11 与 menu live-12 所依据的因果语法和 shutdown generation 修复边界。
3. 迁移前的适用性修订已由 gate v1 吸收；旧文件名只保留迁移页，清理记录见[过时文档迁移表](../00-overview/obsolete-document-map.md)。

这些文档不授予新的 passive/live 授权，也不把适用性 PASS 扩张成 active join 或 T0-05 总 PASS。
