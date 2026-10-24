> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 admission v3：fresh B 到达定义与别名闭合边界

本版承接 plan v10 / join progress v4，只处理普通确认路径中 fresh B 从 `image+0x5B9CD0` 的分配、`image+0x353DA0` constructor、`image+0x355470` Init、assignment/HUD 到首次 tail/cache 消费的来源定义与别名传播。P0、Room matching、出生 Effect、完整 manager 返回、自然更新窗口、reader/ABA、55 槽 runtime、IPC、arm/claim、active/live 和实机观察均不在本轮闭合范围。v1/v2 证据、脚本、测试、失败产物和三个当前索引保持原样。

## 证据结果

正式 Init v5 为 `cs-t0/coop-native-join-init-closure-v5.json`，SHA-256 为 `E86E086D75C3AC103042816D70D8E02751FFBB53C466ADBAD8F47FDEFD53AB99`。它登记 admission v2 的全部 17 类来源、70 个定义写入及处置、190 个消费者：151 个 reconcile 参数、8 个直接公式、14 个 callback 暴露和 17 个 tail 外部来源。

证据不预设 fresh B 全零。每个来源分别记录 owner、字节切片、合法零或非零形态、定义写入、可接受定义、消费者反向索引和未决条件。`implicitGrants` 还包含 `B+0x20A9` 的 byte 切片及 `0x00355B67` 写入；`0x0036EBD0` 的公式同时映射 `implicitGrants` 和 `familiarCarry`。active slots 的四轮 constructor 写和 Init 的八个 16-byte 写、extra map/byte vector 的寄存器别名写、宽写覆盖及动态 resize/zero-loop 均作为独立定义或精确前沿保留。

两条玩家向量 publication 分支分别位于 `0x005B9EAD` 和 `0x0001D193`，并且都发生在 Init 调用之前。因此 Init、callback、tail 或 cache callee 可通过全局玩家容器重新发现 B；“调用参数没有 B”不能排除别名写。磁盘复核只确认这两条指令是 dword writer，所写对象身份、互斥路径与跨调用保持仍为 UNKNOWN。

本轮保留八个证明前沿：

- 动态 vector resize 与逐元素 zero-loop 的边界、寿命和第二轮保持；
- active-slot constructor 四轮循环的不变量；
- publication-before-Init 后经全局玩家容器发生的别名写；
- Init callback 9 的 owner、mask 和副作用；
- 13 个 cache callback 的 owner、mask 和副作用；
- type-0 config、loaded/lazy resource 与特殊 seed 外部来源；
- fresh-B allocator、Weapon factory 的 non-null、对象身份、别名及异常路径；
- 190 个消费者处逐字节、无中间未分类写的 reaching-definition 组合。

因此 `FRESH_B_SOURCE_PRESERVATION=UNKNOWN`，`premisesGloballyEstablished=false`，`entryEstablishedBeforePublication=false`。另外三个同步子证明也保持 UNKNOWN，`synchronousPropagationComplete=false`。没有候选场景污染反例被确立；当前结论是证明不足，不能提升成事前安全结论。

## 契约与入口

admission contract v3 位于 `cs-t0/coop-native-join-admission-contract-v3.json`，SHA-256 为 `1502776C6E4C2F72674291FE1AF58F48746ECB53C466ADBAD8F47FDEFD53AB99`。它单独暴露 `freshBProof.status`，固定 17 类来源和 Init v5 的 source、consumer、publication、alias、callee、allocation 引用，并禁止局部 fresh-B 结果提升完整同步证明。

实现位于独立的 `cs-t0/admission_v3/`，公开入口为：

- `cs-t0/inspect-coop-native-join-admission-v3.ps1`
- `cs-t0/verify-coop-native-join-admission-v3.ps1`

两个入口继续使用 `-GameExe`、`-RepositoryRoot` 和可选 create-new `-Output`。inspector 检查严格 JSON、目标 PE、绑定 SHA、原始函数字节、fresh-B 结构关系和契约一致性；合法输入仍报告整体 `BLOCKED_BY_EVIDENCE` 并退出 1，非法输入拒绝且不生成报告。

verifier 的模型、结构反例、原始磁盘重建和公开入口测试位于独立的 `cs-t0/tests/join_admission_v3/`。合成模型按字节和定义身份追踪 ALLOC、WRITE、CALL、PUBLISH、CONSUME，覆盖恢复零、中间消费、宽写、循环第二轮、未知 callee、allocator null/alias、越界和 publication 调序等反例。模型通过只证明校验逻辑能拒绝这些反例，不证明原生程序闭合。

独立磁盘验收重新构造冻结的 92 个函数，并对 v3 的 62 个声明行再解码，其中 17 个是新增函数、45 个是重复交叉核对；总计 47,694 条指令、5,276 个调用、19,679 个写入和 366 个出口。声明集合与磁盘字节一致，但仍保留 796 项未分类控制/副作用和 604 项保守语义未决。Effects 空分支仍调用 `0x530980`，不能据空容器裁掉 helper。

## 权限与保存

Init v5、contract v3、实现、测试、日志、失败和重试产物均 create-new 或位于独立版本目录。依赖方向为冻结 v4/v2 与原始提取 → Init v5 → contract v3 → 固定 contract SHA 的 inspector → 独立验收报告；后续报告不被其祖先绑定，以保持 SHA 依赖无环。

本版不发布 plan v11 / join progress v5，也不修改 plan v10、join progress v4 或三个当前入口索引。矩阵第 11 项保持 `NOT_STARTED`；05-05、T0-05 整体、T0-06 及所有 runtime、active、live 权限继续关闭。
