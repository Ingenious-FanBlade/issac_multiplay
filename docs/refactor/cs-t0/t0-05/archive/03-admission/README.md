> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Native Join Admission 累计准入规格

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

本目录按版本配对保存 admission producer 记录与 independent test。v1–v6 是累积收窄链，不是六套并列的当前契约。

## 当前合并结论

- v5 已局部确认 manager guard 条件公式；完整 manager context 仍为 `UNKNOWN`。
- v6 已确认 ordinary spawn receiver 的局部公式；manager-this 同代身份仍为 `UNKNOWN`，正式 verifier 结果为预期的 `FAILED_MANAGER_THIS_IDENTITY_PREMISE`、退出 1。
- publication identity、fresh-B source preservation、完整同步传播、自然更新窗口、生产 runtime 和 live 权限均未因此提升。

## 版本索引

| 版本 | 本轮新增边界 | Producer | Independent test |
| --- | --- | --- | --- |
| v1 | 静态 admission 规格、reader/generation 规则与全局缺口清单 | [implementation v1](native-join-admission-implementation-v1.md) | [test results v1](native-join-admission-test-results-v1.md) |
| v2 | Init/P0 同步读取与完整 manager return 边界 | [implementation v2](native-join-admission-implementation-v2.md) | [v2 test results](native-join-admission-v2-test-results-v1.md) |
| v3 | fresh-B reaching-definition/source preservation | [implementation v3](native-join-admission-implementation-v3.md) | [v3 test results](native-join-admission-v3-test-results-v1.md) |
| v4 | publication identity 到 Init dispatch 的局部子证明 | [implementation v4](native-join-admission-implementation-v4.md) | [v4 test results](native-join-admission-v4-test-results-v1.md) |
| v5 | manager-context guard 公式 | [implementation v5](native-join-admission-implementation-v5.md) | [v5 test results](native-join-admission-v5-test-results-v1.md) |
| v6 | GameRoot slot 到 ordinary spawn receiver 的局部公式 | [implementation v6](native-join-admission-implementation-v6.md) | [v6 test results](native-join-admission-v6-test-results-v1.md) |

通常只需从 v6 配对记录开始，再按其中引用回看 v1–v5。旧版本的 `UNKNOWN`、失败报告和权限字段不得用新版本的局部 PASS 回写改判。
