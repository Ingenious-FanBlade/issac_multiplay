> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# 持久化安全与停止窗口

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

本目录把 readonly、dirty、save-enabled、文件/Cloud/Steam 路径和自然更新窗口的静态审查合并为一个阅读入口。

## 当前合并结论

正常菜单路径能够原生建立 readonly，并可排除已定位的部分写入口；但 readonly、最终 dirty=0 或文件哈希未变都不能推出“窗口内从未发生持久化污染”。完整 Init、每次自然 GameUpdate、停止/drain/恢复/terminal 期间的不变量保持尚未归纳闭合，因此 native join active 仍不可准入。

## 阅读顺序

1. [完整停止窗口与自然更新传播 v2](native-join-persistence-window-v2.md)：当前窗口定义、局部新增事实和未决传播。
2. [持久化保护与停止窗口 v1](native-join-persistence-window-v1.md)：readonly 正向路径、save/dirty/Steam 边界和初始 writer 清单。
3. [限域静态条件审查实施 v1](native-join-static-closure-implementation-v1.md)：只读接口与历史隔离。
4. [独立测试结果 v1](native-join-static-closure-test-results-v1.md)：对条件含义、反例和未闭合边界的独立验收。

v2 收窄并补充 v1，但不把 v1 的阶段性失败或历史证据覆盖掉。
