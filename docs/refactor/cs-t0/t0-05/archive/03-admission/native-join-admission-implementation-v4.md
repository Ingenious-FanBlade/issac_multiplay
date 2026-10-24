> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 admission v4：publication 身份准入子证明

admission v4 为 `FRESH_B_PUBLICATION_IDENTITY_TO_INIT_DISPATCH` 新增独立的磁盘检查、合成身份模型和公开入口。它的静态边界止于 `image+0x005B9F1F`，不会进入 Init body，也不会把局部 publication 结果升级为完整 fresh-B、同步传播、reader 或 runtime 结论。

正式 contract 是 `cs-t0/coop-native-join-admission-contract-v4.json`，SHA-256 为 `91787E4DFB406F5A27C3B03DBEC2BBA648793AA53DE2188DDBEF441866BEB180`。它绑定 Init v6（SHA-256 `281137C970DC39CE2DF325E817076E325FE37C02D98F1FC6B3A9EB0E57367847`），单独暴露 `publicationIdentityProof.status`，并保持：

- `publicationIdentityProof.status=UNKNOWN`
- `freshBProof.status=UNKNOWN`
- `freshBProof.nativePublicationClosure=false`
- `freshBProof.nativeAliasClosure=false`
- `syncProof.synchronousPropagationComplete=false`
- 全部 runtime、arm/claim、active/live 与下游权限为 false

## 独立实现目录

版本化实现位于 `cs-t0/admission_v4/`：

- `admission_contract.py` 严格验证 contract v4 的字段、引用、局部状态与父证明不提升规则。
- `publication_identity_model.py` 追踪合成路径中的 ALLOC、CALL、ALIAS、WRITE、PUBLISH、CONSUME 和 END_PATH。模型通过只表示反例校验逻辑有效，`nativeProof` 永远为 false。
- `publication_alias_evidence.py` 验证正式 Init v6 的 object/allocation/path/publication/transfer/escape/dispatch/failure/callee/frontier 结构和交叉引用。它明确返回 `independentInstructionReconstructionPerformed=false` 与 `nativeSemanticProofVerified=false`。
- `disk_support.py` 只读复用版本无关的 PE、路径、SHA 与解码基础能力。
- `inspect_admission.py` 固定 contract SHA，校验全部绑定、目标 PE、anchors/chunks 和 publication 章节；合法输入仍只能生成 blocked 报告。

v4 没有修改 v1-v3 的模块、入口、测试或正式证据。旧生产 `coop-join-preflight` 的七项缺口和退出行为保持原样。

## 结构约束

证据 checker 固定两个 path 与 publication point，要求六步身份链按实际执行事件顺序出现：allocator return、constructor receiver、constructor return、local save、vector store、vector commit。每条路径还必须有唯一的 vtable-load 与 Init-receiver transfer，publication 必须先于 dispatch。UNKNOWN 记录必须给出非空未决原因，STATIC 记录不得保留未决原因。

分配章节固定 8 个必要 callsite；callee 章节至少覆盖 9 个必要根，正式包实际登记 14 个 summary。dispatch 固定到 `0x005B9F1F`、vtable read `0x005B9EC2`、slot read `0x005B9ECA`、offset `+4` 和 target `0x00355470`。这些结构约束防止漏路径、调序、错误对象 publication、错误 receiver、宽写覆盖、stale alias 和未知 callee 通过字段自述冒充闭合。

## 公开入口和退出语义

公开入口为：

- `cs-t0/inspect-coop-native-join-admission-v4.ps1`
- `cs-t0/verify-coop-native-join-admission-v4.ps1`

两者沿用 `-GameExe`、`-RepositoryRoot` 和可选 create-new `-Output`。合法 inspector 报告 `BLOCKED_BY_EVIDENCE` 并退出 1；非法输入退出 2 且不得生成输出。verifier 在 publication 身份未闭合时报告 `FAILED_PUBLICATION_IDENTITY_PROOF` 并退出 1；若未来该局部子证明闭合但其余 fresh-B 证明仍未完成，则报告 `BLOCKED_REMAINING_FRESH_B_PROOF` 并退出 1。只有全部要求的独立检查实际执行且全部父证明闭合，才允许退出 0。

第一次 inspector 集成运行发现 inherited v5 chunk 使用带单空格的十六进制字节，而新 parser 只接受无空格形式，因此合法输入被错误拒绝。失败已保留在 `out/cs-t0/native-join-admission-v4-inspector-integration-failure-v1.json`；parser 修正为只接受两位十六进制 byte 与可选单个普通空格，任意空白或畸形输入仍拒绝。成功重试保存在 `out/cs-t0/native-join-admission-v4-inspection-v1.json`，结果为 `BLOCKED_BY_EVIDENCE`、退出 1。

## 边界与发布状态

本版没有实现 reader、generation runtime、55 槽、IPC、arm/claim，也没有读取进程或进行实机操作。plan v10、join progress v4 和三个当前索引未修改；plan v11/progress v5 未发布。矩阵第 11 项保持 `NOT_STARTED`，05-05、T0-05 整体、T0-06 及所有 active/live 权限继续关闭。
