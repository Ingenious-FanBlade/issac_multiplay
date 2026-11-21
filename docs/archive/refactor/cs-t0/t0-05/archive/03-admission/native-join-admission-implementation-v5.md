> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 admission v5：manager-context 静态准入检查

admission v5 为 `FRESH_B_MANAGER_CONTEXT_TO_ORDINARY_SPAWN` 增加版本化、磁盘只读的 inspector、contract checker、证据 checker 和合成反例模型。正式 contract 是 `cs-t0/coop-native-join-admission-contract-v5.json`，SHA-256 为 `82FAEA8941BA4EC687CB571B9704CB393358216972361EC69C74ED981C78A69E`。

实现位于 `cs-t0/admission_v5/`：

- `admission_contract.py` 固定 v5 根 schema、九个 manager frontier、父证明不提升和全部 deny-only 权限；
- `manager_context_evidence.py` 交叉验证 17 个谓词、19 条定义使用链、22 个 callee summary、9 个入口来源、循环 CFG、原生 exit 全集、InputManager `+0x74` 绑定和 sidecar 计数；
- `manager_context_model.py` 对有限合成 trace 执行路径、调用、alias、写入、循环第二轮和异常恢复反例；模型通过不代表原生证明；
- `publication_alias_evidence.py` 只读继承 v4 publication 结构语义，并适配累计 Init v7；
- `inspect_admission.py` 固定 contract SHA，校验全部磁盘绑定、PE bytes、anchors/chunks、manager 与 publication 章节。

公开入口是 `cs-t0/inspect-coop-native-join-admission-v5.ps1` 和 `cs-t0/verify-coop-native-join-admission-v5.ps1`，沿用 `-GameExe`、`-RepositoryRoot` 与可选 create-new `-Output`。非法输入退出 2 且不生成报告；合法 inspector 只会报告 `BLOCKED_BY_EVIDENCE` 并退出 1。

verifier 的预期决策顺序为：guard 公式未闭合时 `FAILED_MANAGER_GUARD_FORMULA_PROOF`；guard 已闭合但入口前提未建立时 `FAILED_MANAGER_CONTEXT_PREMISE_PROOF`；manager context 闭合而 publication 仍未完成时 `BLOCKED_REMAINING_PUBLICATION_PROOF`。本轮正式预期是第二种，整体仍非零。

v5 没有修改 v1-v4 的证据、脚本或测试，也没有改旧生产 `coop-join-preflight`。plan v10、join progress v4 和三个当前索引保持原 SHA；不发布 plan v11/progress v5。
