> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0 PowerShell / JSON 清理记录 v1

日期：2026-09-08  
范围：`cs-t0/**/*.ps1`、`cs-t0/**/*.json`

清理前共有 32 个 PowerShell 文件和 132 个 JSON 文件。判定规则是：当前入口或
构建仍调用、被任何源码/文档按路径引用、位于 artifact hash/predecessor/test
baseline 链，或承担 template/testdata/ledger 角色的文件均保留。

删除项：

- `cs-t0/freeze-coop-native-join-predecessor.ps1`：仓库内无任何引用；它只负责
  create-new 生成已经存在的 `coop-native-join-predecessor-v1.json` 和历史 snapshot，
  现有逻辑会因目标已存在而永久拒绝执行。删除脚本不删除已冻结 manifest/snapshot。

JSON 清点没有发现可安全删除项。132 个 JSON 均仍有路径引用或证据职责。特别是
`tests/join_admission_v6/frozen-v5-baseline-v1.json` 与 `v2.json` 虽为 0 字节，仍被
`coop-native-join-admission-verification-v6.json` 以 SHA-256 绑定，删除会破坏历史
可验证性，因此保留。

`out/cs-t0` 与 `artifacts/cs-t0` 不属于本次源树清理范围；其中的 create-new PASS/
FAIL 证据未删除或覆盖。

