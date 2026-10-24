> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-context v5 独立验收结果

本轮只验收 manager guard 的局部静态公式和 manager-context 入口前提。测试角色未运行或枚举游戏进程，未注入、未构建生产代码，也未把逆向提交的字段存在、锚点数量或哈希一致解释为原生闭合。

## 正式结论

公开 verifier 从冻结输入重新执行全部检查，生成 `cs-t0/coop-native-join-admission-verification-v5.json`。正式状态为 `FAILED_MANAGER_CONTEXT_PREMISE_PROOF`，退出码为 1；`localChecksPassed=true`、`allRequiredChecksExecuted=true`、`overallAccepted=false`。

独立磁盘解码确认 bounded manager guard 公式为 `STATIC_CONFIRMED`。manager-context 入口时的 owner 身份、roster canonical zero、device snapshot 生命周期、callback 稳定性、generation/ABA、异常和提前退出前提仍有明确 frontier，因此 `managerContextStatus=UNKNOWN`。publication identity、fresh-B preservation 和完整同步证明也都保持 `UNKNOWN`。局部 guard 结论没有被提升为 manager 前提或整体 admission PASS。

## 执行结果

| 检查面 | 结果 |
|---|---|
| v5 contract 篡改反例 | 39/39 通过 |
| 纯离线 manager-context 模型反例 | 55/55 通过；只覆盖 synthetic model |
| submitted evidence 结构与交叉引用反例 | 73/73 通过；不构成原生证明 |
| 公开 inspector 黑盒 | 6 组、80 个断言通过；合法输入固定 `BLOCKED_BY_EVIDENCE`/exit 1 |
| v6→v7 create-new 合并保持 | 68 个 v6 函数完整保留；22 个 manager 函数合入，21 个新增、1 个重叠完全相同 |
| 磁盘重建 | 四份证据共 220 个文档函数行、136 个唯一入口；53,112 条指令、5,620 个 call、21,154 个 writer、484 个 exit 的声明集合与目标 PE 一致 |
| manager 局部语义 | 22 个函数、68 个关键字节序列、10 个 query、2 个 spawn、17 个 predicate、19 个 definition chain 完成独立核对 |
| 显式 SHA DAG | 5 个解析节点、114 条边、94 个 bound leaf；0 自绑定、0 环；历史 opaque binding 未递归解释 |
| 历史/生产回归 | v4 完整公开报告逐字段相等；旧生产 preflight 保持七缺口、`BLOCKED_BY_EVIDENCE`/exit 1 |

语义检查项被记录为 `COMPLETED_WITH_UNRESOLVED`，没有计作 PASS。独立解码仍记录 465 个 indirect-call target 未决、23 个 no-return 假设未决、24 个 indirect-successor 未决，以及 trap、return-continuation、fallthrough、orphan 和未分类 effect 等边界。这些缺口与九个 manager proof frontier 一起阻止 manager-context 闭合。

## 完整性与版本保护

正式报告绑定 473 个输入，运行前后逐 SHA 不变。冻结 v4 正式输入仍为 426 个；历史计数保持 174 个既有源码/测试、164 个 snapshot 文件、149 个原始 baseline 文件和 3 个生产二进制。三个可变索引保持冻结值，当前入口仍为 plan v10 / progress v4；本轮未发布新 plan、progress 或活动授权。

开发期失败与重试均保留。`native-join-manager-context-v5-development-failure-v1.json` 记录 duplicate-frontier 误收，`native-join-manager-context-v5-inspector-integration-failure-v1.json` 记录公开入口集成失败，`native-join-admission-v5-verification-development-v2/v3/v4.json` 分别保留历史保护边界修正和最终开发复核。它们不替代正式验证。

## 封存输入与入口

| 文件 | SHA-256 |
|---|---|
| `cs-t0/coop-native-join-admission-contract-v5.json` | `82FAEA8941BA4EC687CB571B9704CB393358216972361EC69C74ED981C78A69E` |
| `cs-t0/coop-native-join-init-closure-v7.json` | `F03155CF0845C58B9F09E05900258166E0A9745E9C6B8CB674230C46C58C7065` |
| `cs-t0/coop-native-join-manager-context-v1.json` | `2D0CB3C4DEE18AED73793AB63B72524D6EF043C2E0AD00611D06B22C78994B06` |
| `out/cs-t0/native-join-manager-context-v5-inspection-v2.json` | `7ABD4792A65F00B53BC727E90566D9FBAE9F1DC7B85858B298F04E5DA375238F` |
| `cs-t0/coop-native-join-admission-verification-v5.json` | `33C8095F37F567FC69DAA1B2F76EBEB81CFFA080664F2EF47772AE504E966C44` |
| `cs-t0/tests/join_admission_v5/verify_join_admission_v5.py` | `E67B585A6DE80337D5E49D0A45AD6391516C681FCCF2C73DF6CCFF7269124FCB` |
| `cs-t0/tests/join_admission_v5/support_v5.py` | `1BCCBE150AE02FDB94D90D0BEB92B66B7FD153C2537167938ACB589101FA5306` |
| `cs-t0/verify-coop-native-join-admission-v5.ps1` | `E76189CFE1199292742412074D1310EACD088C742C746B25685D607EC44777BD` |
| `cs-t0/inspect-coop-native-join-admission-v5.ps1` | `457760B9FB95C823D9E5AC1BD412A594C12B87EA44E9B92262A09B5C08D598AC` |

正式命令使用公开入口、磁盘目标和固定 decoder：

```powershell
pwsh -NoProfile -NonInteractive -File cs-t0/verify-coop-native-join-admission-v5.ps1 `
  -GameExe 'F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe' `
  -RepositoryRoot 'D:\projects\isaac_hook_reserach' `
  -Objdump 'D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe'
```

所有 process inspection、injection、mapping、arm、claim、active join、live authorization、runtime mutation 和后续工作包解锁权限均保持 false。
