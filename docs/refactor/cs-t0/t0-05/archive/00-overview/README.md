> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05 总览与计划

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

本目录保留计划与状态两个研究入口；执行要求以开头链接的现行项目规范为准，旧门禁、哈希与独立角色流程已取消。

## 权威职责

- [研究计划](isaac-cs-t0-05-research-plan.md)：定义目标、非目标、工作包依赖、证据门、场景矩阵、停止条件和最终出口。第 1–16 节是规范性入口；第 17–35 节是按发生时间追加的实施记录。
- [执行状态](isaac-cs-t0-05-status.md)：记录已执行机器产物、验证结果和当前阻断。文件头与正文已更新至 2026-09-08；05-05 只读必要性决策见第 22 节，ownership/lifecycle 证明审计见第 23 节，v4 离线实现与最终压缩验收见第 24 节，严格 manager-this 状态仍由第 20 节及其证据链定义。
- [过时文档迁移表](obsolete-document-map.md)：记录已由明确后继版本吸收、正文按授权直接清理为迁移指针的 12 份阶段稿。

## 去重后的当前结论

| 工作包 | 状态 | 结论 |
| --- | --- | --- |
| `CS-T0-05-00` | `PASS` | evidence bootstrap 与冻结基线建立 |
| `CS-T0-05-01` | `PASS` | local co-op 静态路径图闭合 |
| `CS-T0-05-02` | `PASS` | 三模式 C0 只读对照闭合 |
| `CS-T0-05-03` | `PASS_SCOPED_APPLICABILITY` | 五份适用 PASS source 与一项限域 N/A；旧 5/6 历史门保留 |
| `CS-T0-05-04` | `PLANNING_ASSUMED / STRICT_BLOCKED_BY_EVIDENCE` | 规划轨假定原生 Join 可可靠创建/发布 B；manager-this、Init/持久化、reader/ABA 与正式主动门未闭合 |
| `CS-T0-05-05` | `BLOCKED_BY_EVIDENCE / OFFLINE_LIFECYCLE_CONFIRMED` | 物理设备自由 B 需要 virtual device；owner/创建析构/refcount 三项离线代码义务已确认，但 native/formal 仍 UNKNOWN、10 项证据未闭合，正式 decision 为空 |
| `CS-T0-05-06..07` | `NOT_STARTED` | 05-06 未解锁，不因必要性结论提前开放实现或 runtime |

当前恢复工作时，应先读[执行状态](isaac-cs-t0-05-status.md)的第 22～24 节和 [virtual device 决策入口](../07-virtual-device/README.md)，并以第 20 节及 [manager identity 索引](../06-manager-identity/README.md)核对 05-04 严格证据。历史章节中的“当前”“下一步”只代表当时快照，不覆盖后续 create-new 结论。
