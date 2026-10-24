> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 Native Alias 固定点清零 Batch 1 独立 TEST 结果 v1

## 1. 结论

- 状态：`PASS`。
- 执行日期：2026-09-09（UTC）。
- 验收依据：[独立 TEST 预封 v1](virtual-device-native-alias-fixed-point-clearing-batch1-test-plan-v1.md)，SHA-256 `06A1E0CFEAA8156EF1B23FFAF4183CBBAA46D17B9858D96DE192817859656637`。
- 四组按预封顺序各运行一次；无失败、无重试、无修订候选、无宽回归或动态测试。
- 本次通过只证明 Batch 1 的 IAT 分区、固定映像 callback 闭世界、Manager service 两分支和保守阻断决策可独立重放。它不表示完整 Alias Universe 已闭合，不提升 virtual-device owner，也不授权任何运行时工作。

## 2. 只读候选审阅

正式执行前只读核对以下候选和绑定；未修改这些文件：

| 产物 | SHA-256 |
| --- | --- |
| `cs-t0/native-alias-universe-v3.json` | `CD2A6BADFACAECD1FC1A147199CCC24B433F679A34FEF8C223A893E5A3621737` |
| `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v6.json` | `65C9EE330F4FBCD51CC2D88EDB32283A0E61FA09254012D8F5E9DB21044BAA50` |
| `cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v6.ps1` | `906721F6B211AF234226DD467646506C5833EAF67107CCD3A17D20CB7F8062A8` |
| `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v6/verify_virtual_device_ownership_lifecycle_decision_v6.py` | `72FA25DFCC8226FF5870B03C7712F885F4BA63BCE2B33BFC44205BA4C72A6D91` |
| `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v6/run_native_alias_batch1_oracle_v1.py` | `3FD16E92034D7FD182B197CED2D1E88CAF4EC3DB0F14E98389740786996D4B1B` |
| `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v6/test_virtual_device_ownership_lifecycle_decision_v6.py` | `B9A7FDD57FDD40090E2CC3637512416316BED070EE57C3EE185FC04769353A1E` |
| `virtual-device-native-alias-fixed-point-clearing-batch1-reverse-v1.md` | `C1CBBF6E8CDB380212BACF64DA28ED79D8F0E2888DB6C2C6271A47DF433B59E8` |

v6 有 45 个 artifact bindings；新增的 v5 parent、preseal、REVERSE、v3、公开入口、verifier、oracle 和负向批次绑定均与磁盘字节一致。负向批次在运行前静态确认恰有两个 `test_` methods。

## 3. 压缩四组执行结果

### Group 1 — 固定 PE 与独立集合 oracle

- UTC 开始：`2026-09-09T10:01:34.319Z`。
- UTC 结束：`2026-09-09T10:02:09.261Z`。
- 唯一命令：

```powershell
python -B cs-t0/tests/virtual_device_ownership_lifecycle_decision_v6/run_native_alias_batch1_oracle_v1.py --game-exe 'F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe' --objdump 'D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe' --repository-root .
```

- 退出码：`0`；结果：`PASS`。
- 持久化输出：无，oracle 明确报告 `outputWritten=false`，因此没有输出文件 SHA；oracle 源 SHA-256 为 `3FD16E92034D7FD182B197CED2D1E88CAF4EC3DB0F14E98389740786996D4B1B`。
- 固定目标和独立 `objdump` 哈希通过。IAT 严格按 `[0x00718000,0x00718A28)` 解析，`18,107` 个 `FF 15` 与 `133` 个 `FF 25` exclusions 逐行闭合；28 个 import modules、622 个非零 import entries，1 个 `_beginthreadex` Manager context transfer 被解析为已闭合 transfer，`IMPORT_ALIAS_ESCAPE=0`。
- callback slot `0x0087E814` 为 101 reads、0 writes、0 address escapes、0 loader mutations；3 个 unresolved calls 及 3 个重叠 escapes 被精确清除。
- Manager service 的 derived/base 两分支均闭合，`serviceClosedBranchCount=2`，旧 `managerArgumentEscapeFrontier` 从 1 降为 0。
- receiver frontier 由 `474 + 270 + 16,323 + 3,677 + 0 + 0 = 20,744` 得出；pair frontier 由 33 liveouts + 1 exception edge 得出 `34`。首个 receiver/universe 边为 `RESIDUAL-INDIRECT-CALL-00004`，RVA `0x00006A75`，raw `FF D7`。

### Group 2 — v6 公开 CLI create-new

- UTC 开始：`2026-09-09T10:02:32.731Z`；调用前已确认目标不存在。
- UTC 结束：`2026-09-09T10:03:13.624Z`。
- 唯一命令：

```powershell
pwsh -File cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v6.ps1 -GameExe 'F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe' -Objdump 'D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe' -RepositoryRoot . -Output out/cs-t0/virtual-device-native-alias-fixed-point-clearing-batch1-verification-20260909-v6.json
```

- 退出码：`0`；结果：`PASS`；7 个公开 checks 全部 `PASS`。
- create-new 输出：`out/cs-t0/virtual-device-native-alias-fixed-point-clearing-batch1-verification-20260909-v6.json`，5,731 bytes，SHA-256 `301D91F8812A82B5F64052C3DAD27872E270F26199015123B4CCC6B19F68915D`。
- 输出绑定 decision SHA-256 `65C9EE330F4FBCD51CC2D88EDB32283A0E61FA09254012D8F5E9DB21044BAA50`、manifest SHA-256 `CD2A6BADFACAECD1FC1A147199CCC24B433F679A34FEF8C223A893E5A3621737`，并重复得到 receiver `20,744`、pair `34`、首边 `0x00006A75`。

### Group 3 — v6 公开 CLI verify-existing 与不可变性

- UTC 开始：`2026-09-09T10:04:11.838Z`。
- UTC 结束：`2026-09-09T10:05:21.223Z`。
- 唯一公开 CLI 调用：

```powershell
pwsh -File cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v6.ps1 -GameExe 'F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe' -Objdump 'D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe' -RepositoryRoot .
```

- 退出码：`0`；结果：`PASS`；`outputWritten=false`，没有新输出文件 SHA。
- 调用前后对 72 个相关文件逐字节 SHA-256 快照，覆盖 v1-v6 决策/manifest、v5/v6 测试源、07 历史和已有 virtual-device 输出。聚合 SHA-256 前后均为 `4FB4FC806B72024F293D0232A240B58EC6FB88811491A7CDFD18B33719FCC7B1`，`snapshotBytesUnchanged=true`。
- `git status --porcelain=v1` 前后相同；Git 对用户级 ignore 文件报告访问警告，但两次警告一致且不影响工作树比较或命令退出码。Group 2 输出 SHA-256 仍为 `301D91F8812A82B5F64052C3DAD27872E270F26199015123B4CCC6B19F68915D`。

### Group 4 — 单一参数化负向批次

- UTC 开始：`2026-09-09T10:05:43.609Z`。
- UTC 结束：`2026-09-09T10:06:14.271Z`。
- 唯一命令：

```powershell
python -B cs-t0/tests/virtual_device_ownership_lifecycle_decision_v6/test_virtual_device_ownership_lifecycle_decision_v6.py
```

- 退出码：`0`；结果：`PASS`；输出为 `Ran 2 tests in 12.295s`、`OK`。
- 恰好两个 methods：`test_semantic_mutations` 与 `test_write_boundaries`。全部参数化 subcases 共用一个临时 fixture；没有拆分为额外 methods 或重复 positive CLI。
- 持久化输出：无，因此没有输出文件 SHA；负向测试源 SHA-256 为 `B9A7FDD57FDD40090E2CC3637512416316BED070EE57C3EE185FC04769353A1E`。

## 4. 最终保守边界

- `receiverDispatchFixedPointClosed=false`；剩余 receiver frontier `20,744`。
- `pairAliasFixedPointClosed=false`；剩余 pair frontier `34`。
- `dynamicAliasUniverseClosed=false`。
- `allAliasesBoundedAndReleasedBeforeManagerRemove=false`。
- `nativeOwnerHeldThroughDrain=false`。
- `VIRTUAL_DEVICE_OBJECT_OWNER` 保持 `UNKNOWN`，无 promotion。
- `formalDecision=null`、`implementationEligibility=false`、`workPackageStatus=BLOCKED_BY_EVIDENCE`。
- 全部 18 项权限，以及 `csT00506Unlocked`、`t005Unlocked`、`t006Unlocked` 均保持 `false`。
- factory/rollback、reference-count、final-release/thread 状态逐字继承 v5；本批未开展这些后续工作。

明确未运行：v4 lifecycle harness、Cargo 全量、完整 T0 build、Native Debug/Release/LTCG 矩阵、历史宽回归、重复 self-test、游戏、x32dbg、进程探针或任何动态测试。
