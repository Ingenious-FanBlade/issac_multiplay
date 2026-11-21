> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 admission v6：manager receiver 静态准入检查

admission v6 为 `ORDINARY_SPAWN_RECEIVER_FROM_GAMEROOT_SLOT` 增加独立的 contract、证据 checker、合成身份模型和公开磁盘入口。正式 contract 是 `cs-t0/coop-native-join-admission-contract-v6.json`，SHA-256 为 `1022B812A6F98CB1D409E5B7E5FA9E8077BDA97C58746A98BE5DB0BEE343F7F4`。

实现位于 `cs-t0/admission_v6/`：

- `admission_contract.py` 固定 v6 schema、Init v8 引用、receiver formula 与 manager-this 两层状态，并阻止局部结论提升为完整 manager context；
- `receiver_identity_evidence.py` 从五个函数清单交叉核对 2,337 条指令、225 个调用、842 个写、7 个出口、四条 ordinary 指令及其字节、唯一 caller xref、39/7 个先行调用、三个直接 writer 和四个未决前沿；
- `receiver_identity_model.py` 对有限合成 trace 检查 ECX 定义、slot replacement、generation、调用破坏和 spawn receiver。模型通过不等于原生身份或 ABA 闭合；
- `manager_context_evidence.py` 与 `publication_alias_evidence.py` 只读继承 v5/v4 语义并适配累计 Init v8；
- `inspect_admission.py` 固定 contract SHA，校验严格 JSON、磁盘绑定、目标 PE、anchors/chunks 和各版本章节。

公开入口是 `cs-t0/inspect-coop-native-join-admission-v6.ps1` 与 `cs-t0/verify-coop-native-join-admission-v6.ps1`，参数仍为 `-GameExe`、`-RepositoryRoot` 和可选 create-new `-Output`。非法输入退出 2 且不生成报告；合法 inspector 报告 `BLOCKED_BY_EVIDENCE` 并退出 1。

verifier 的决策顺序是：局部 receiver 公式未闭合时 `FAILED_MANAGER_RECEIVER_FORMULA_PROOF`；公式闭合而同代身份未建立时 `FAILED_MANAGER_THIS_IDENTITY_PREMISE`；manager-this 闭合而其余 manager frontier 未完成时 `BLOCKED_REMAINING_MANAGER_CONTEXT_PREMISES`。本轮正式预期是第二种，整体仍非零。

正式 inspector 产物 `out/cs-t0/native-join-manager-receiver-v6-inspection-v1.json`（SHA-256 `5D2FFBDA91BC1EBA5014DECB4DB17B71D86C19408ECF0B6E94BF29A51ED7096E`）给出 `managerGuardFormulaStatus=STATIC_CONFIRMED`、`managerReceiverFormulaStatus=STATIC_CONFIRMED`、`managerThisIdentityStatus=UNKNOWN`、`managerContextStatus=UNKNOWN`；publication、fresh B 和完整同步证明均为 `UNKNOWN`。

v6 不修改 v1-v5、旧七缺口 preflight、生产 reader 或二进制。plan v10、join progress v4 和三个当前索引保持不变；不发布 plan v11/progress v5。
