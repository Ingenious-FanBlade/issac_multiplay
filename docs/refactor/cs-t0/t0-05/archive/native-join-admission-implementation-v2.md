> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 admission v2：同步 Init/P0 证据读取与验证边界

本版承接 plan v10 / join progress v4，只处理普通确认分支 `image+0x5BAC40` 至完整 manager 返回的同步证明。完整自然更新窗口、跨 pass 的 generation/ABA、生产 reader、55 槽 runtime、IPC、arm/claim 和实机观察均不在本轮实现范围。旧 v1 脚本、身份判定、固定 UNKNOWN 规则、测试和全部历史证据保持原样。

## 证据与契约

Init v4 保留原始函数字节、指令、基本块、调用、写入、出口及未分类指令；cache/P0 补充证据单向绑定到 v4。八个根函数的清单只是入口集合，不能替代其可达 callee、间接目标、循环和异常出口的完整摘要。出生效果初始化、效果池旧资源清理、B 字段写入及 selector 退休也属于当前边界。familiar 的逐消费者映射必须分别列出 desired 来源和既存匹配实体行为，不能用历史“151 处”数字证明闭合。

准入契约 v2 引用 v4 的 `necessaryInputSlices`、`orderedPropagation`、`calleeSummaries` 和四个子证明；旧完整 reader 的 11 项 UNKNOWN 不因同步切片中确认了偏移就升级。InputManager 仍是静态对象；roster 的三指针 canonical-zero、容器预算、永久失能和 Unsafe 停止规则沿用旧规格。本版没有执行任何 native 内存读取。

以下三项只是本轮有限同步摘要的条件，不声称是所有安全候选场景的必要条件，也不声称在首次 publication 前已经满足：

| 切片 | owner 与读取范围 | 原生分支谓词 | 尚未建立的事实 |
| --- | --- | --- | --- |
| CALLBACK24_MASK_CLEAR | `[image+0x871690] → owner+0 → bitset+0`，4 字节 | 无符号位掩码 `0x01000000` 清零 | owner/bitset 的可读范围、身份及入口前保持 |
| CALLBACK54_MASK_CLEAR | 同一 owner 链，`bitset+4`，4 字节 | 无符号位掩码 `0x00400000` 清零 | 至少覆盖所读字节的有效存储及入口前保持 |
| SYNC_STAGE_DISPATCH_GATE | `[image+0x871678] → gameRoot+0x26614`，4 字节 | **有符号** `i32 < 2` | 该条件在既定场景的入口前来源和保持 |

第三项对应原始 `cmp/jl` 及 `cmp/setnl`，不能解读为无符号比较。本版将三项放在 `syncProof.inputSlices.conditionalPremises`，而非悄然追加已执行的全局 guard；全部 `runtimeValidity=UNKNOWN`、`establishedBeforePublication=false`。这些条件不能补齐当前剩余的 callee、别名、效果池残留资源或 P0/Room 来源。

callback 24/54 在测试 mask 前都读取 registry owner 的 `+0x10` 指针值，因此 owner 还必须覆盖 `[+0x10,+0x14)`。mask 清零分支不解引用该 context 指针，不能额外要求它非空或其 pointee 保持身份；callback 54 还在 mask 前读取实际 Effect 的 `+0x2C` 字段。这些提前读取也在同步切片内保留。

只复用已证明的 readonly setter 分支保护。直接 persistent 写、先写后归零、在中间消费者或保存已经观察到写入的情况，均不能被 readonly 或最终零值排除。fresh B 必须由 constructor 和逐调用传播推导；P0 必须由既存来源及同步写入单独推导，不能使用未来 B 读回替代入口证据。

## 新磁盘入口

`cs-t0/inspect-coop-native-join-admission-v2.ps1` 与 `cs-t0/verify-coop-native-join-admission-v2.ps1` 使用 `-GameExe`、`-RepositoryRoot`、可选 `-Output`；Output 必须 create-new。实现位于独立 `cs-t0/admission_v2/`，验收位于独立 `cs-t0/tests/join_admission_v2/`。允许只读复用的旧基础设施仅限无版本语义的 PE、路径及指令解码功能，不使用 v1 身份替换或修改 v1 判定。

inspector 检查严格 JSON、目标 PE、绑定哈希、原始函数字节、提交清单的结构一致性和契约引用。它**不**执行独立指令解码，也**不**判定原生 callee 摘要为真。合法输入一律报告整体 `BLOCKED_BY_EVIDENCE`，退出 1；非法或未封印输入拒绝，退出非零，不生成报告。输出明确区分结构检查、原生语义证明和运行实现。

verifier 必须实际执行独立解码、指令/CFG/调用/写入/间接目标核对、语义反例、公开入口防篡改、路径与 create-new 检查及历史完整性复核。离线模型反例直接进入模型校验逻辑，与公开入口的哈希拒绝测试分组计数。测试环境缺失、跳过、未执行和覆盖不足均不能计 PASS。

合成模型按操作顺序维护有范围的字节存储，支持别名、带前提的调用摘要、即时消费、条件裁剪和 familiar 匹配检查，用于证明**校验器能拒绝错误模型**。它不从二进制自动提取完整程序，也不能用测试 PASS 证明原生路径没有遗漏。特别是 native 调用/间接目标遗漏及异常边必须由独立原始指令核对发现。

新版 verifier 只有当前完整同步证明以及全部独立检查均通过才退出 0。工具检查或模型反例全部通过，但存在未决摘要、入口来源、callee、别名、间接目标、异常边或消费者关系时，仍为整体非零。合法 UNKNOWN 报告不是非法输入；二者应保持不同诊断。

## 保存与发布

证据、测试报告、失败和重试产物均 create-new，不覆盖先前结果。依赖方向为旧证据/原始提取 → cache/P0 补充 → Init v4 → admission v2 契约 → 固定 SHA inspector → 独立验收报告。实现说明由契约绑定，不绑定后续报告，避免 SHA 环。

若同步证明未完成，保存 v4/v2 和失败或未决报告供后续推进，plan v10 / join progress v4 仍是当前入口，不发布 plan v11 / join progress v5，也不更新当前入口索引。若之后完成，则须先独立验收、审计待发布工具清单/入口 SHA/scope/权限/无环依赖，再 create-new 发布及独立复审。无论本轮局部结果如何，矩阵第 11 项仍为 `NOT_STARTED`，active/live、05-05、T0-05 整体和 T0-06 均不解锁。
