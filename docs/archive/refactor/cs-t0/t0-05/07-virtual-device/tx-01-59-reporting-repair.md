# tx-01-59：修复构建/CLI验收报告

日期：2026-10-17。前序 tx-01-58 的控制器、Native、Helper Release 构建成功，CLI与入口检查完成后已进入源码快照阶段；最终 `ConvertTo-Json -Depth 12` 遍历 PowerShell ErrorRecord 对象图导致内存不足，入口退出1，没有生成 result.json。整体FAIL，原入口与源码快照、build.log、二进制保留；补充失败记录位于 `artifacts/cs-t0/tx-01-58-build-01/acceptance-failure.json`。

本次实质修改仅限验收报告：命令输出与错误输出统一保存为字符串；每项结果即时写入 checks.jsonl；最终汇总只包含有界文本/值，并记录复用二进制的路径、长度、时间与来源。角色选择生产实现不再修改。

实现前列出失败方式：错误对象仍进入报告并递归展开；中途失败导致所有已完成检查丢失；复用二进制被误标为重新构建；新入口覆盖旧失败产物；最终JSON未落盘或不可解析但宣称成功。

本次仅一次统一入口执行报告链路验收，复用 tx-01-58 已构建的三个二进制，覆盖命令行输出（含负向stderr）、逐项JSONL、最终JSON可读取和源码快照。不会重新构建、注入或执行游戏输入；最后两项实机测试仍未开始。结果目录为新的 `artifacts/cs-t0/tx-01-59-report-01`，失败不自动重跑。

## 唯一整体验收结果

```powershell
pwsh -NoProfile -File .\cs-t0\accept-character-selection-build.ps1 -ArtifactDirectory .\artifacts\cs-t0\tx-01-59-report-01 -BuildArtifactDirectory .\artifacts\cs-t0\tx-01-58-build-01 -Transaction tx-01-59
```

入口执行一次、退出0，35项PASS，最终状态 `PASS_CLI_AND_REPORTING`。覆盖三个既有构建产物的记录、名称目录/help、10组名称/ID解释、5组非法角色拒绝、6组实际coop-active参数解析与早期拒绝、9个E2E入口语法及BCharacter参数。所有实际命令输出（包括stderr）均作为字符串保存；checks.jsonl与result.json已由同一入口读回，数量一致。未重新构建或操作游戏，`buildPerformed=false`、`realGame=NOT_RUN`。

结果：[result.json](../../../../../../artifacts/cs-t0/tx-01-59-report-01/result.json)，逐项原始输出：[checks.jsonl](../../../../../../artifacts/cs-t0/tx-01-59-report-01/checks.jsonl)，构建来源：[existing-artifacts.json](../../../../../../artifacts/cs-t0/tx-01-59-report-01/existing-artifacts.json)。源快照在同目录sources，tx-01-58失败原件保持。生产角色选择的唯一新构建仍在tx-01-58-build-01，真实Join/角色效果随后续实机场景验收。
