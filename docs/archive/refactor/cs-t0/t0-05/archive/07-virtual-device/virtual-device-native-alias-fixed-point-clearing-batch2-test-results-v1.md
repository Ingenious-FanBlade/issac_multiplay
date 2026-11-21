> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 Native Alias 固定点清零 Batch 2 独立 TEST 结果 v1

## 1. 结论

- 状态：`FAIL_GROUP_1 / FAIL_STOPPED`。
- 验收依据：[独立 TEST 预封 v1](virtual-device-native-alias-fixed-point-clearing-batch2-test-plan-v1.md)，SHA-256 `793527BCC2864A0CFEB3E1BBD7723EFE3B5345EE56D39BEA625A64C74D585F1F`。
- Group 1 formal oracle 恰好运行一次并失败；按预封 fail-stop 规则，Group 2、Group 3、Group 4 均为 `NOT_RUN`，没有重试、没有运行 public CLI、没有运行负向批次。
- 本结果不接受 Batch 2 v4/v7 候选，不证明 `46 + 3` 清除可由当前 TEST oracle 独立重放，也不提升 Alias Universe、virtual-device owner、实现资格、权限或下游解锁。
- 本轮未启动或访问游戏进程，未使用 x32dbg、调试器、进程探针、注入、IPC、Hook、输入或动态测试。

## 2. 当次候选与工具绑定

当次正式 Group 1 开始前，候选和测试工具的绑定身份如下；失败后未修改这些已绑定文件：

| 产物 | SHA-256 |
| --- | --- |
| `cs-t0/native-alias-universe-v4.json` | `400D94538D5309818836066EFD2F8646F10A863EA192B4F1F3240D013A1C30F9` |
| `cs-t0/coop-virtual-device-ownership-lifecycle-decision-v7.json` | `7C6AC1FDA75B0C2111AFCAD0D4FA29C9F2AAD0920B1A1EEA75D081E6D5145057` |
| `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v7/run_native_alias_batch2_oracle_v1.py` | `3D40B84CABA60EF9530B1FB3D3AF53FCACCAE8FE0C939C233ACF27FD00B1A694` |
| `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v7/verify_virtual_device_ownership_lifecycle_decision_v7.py` | `D16B0D69A711D2B71A5C598EE27DB6EAB66FDE4CCC814B8F4C87BE8C62D5380C` |
| `cs-t0/verify-coop-virtual-device-ownership-lifecycle-decision-v7.ps1` | `9EB587144743DCEFD7061C406452319FA9B6DA6E342FA25BEE0E046DD82EDF54` |
| `cs-t0/tests/virtual_device_ownership_lifecycle_decision_v7/test_virtual_device_ownership_lifecycle_decision_v7.py` | `2FA72779E996E6B7D4FF0B117DBC8947867F30C2A89AC129E8997362B6F05615` |
| `virtual-device-native-alias-fixed-point-clearing-batch2-test-plan-v1.md` | `793527BCC2864A0CFEB3E1BBD7723EFE3B5345EE56D39BEA625A64C74D585F1F` |
| `virtual-device-native-alias-fixed-point-clearing-batch2-reverse-v1.md` | `FF77AE2F25C02CFB389F9B18E3741ADBF4F72BC01920D8F554E25943E031A6AF` |

固定输入仍为：

- Win32/x86 `isaac-ng.exe` SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`；
- GNU `objdump.exe` SHA-256 `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`。

## 3. Group 1 — 独立 fixed-PE/objdump oracle

- 执行次数：`1`。
- UTC 起止：当次调用日志未单独提供 UTC 时间；本报告不补造时间戳。
- 唯一命令：

```powershell
python -B cs-t0/tests/virtual_device_ownership_lifecycle_decision_v7/run_native_alias_batch2_oracle_v1.py --game-exe 'F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe' --objdump 'D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe' --repository-root 'D:\projects\isaac_hook_reserach'
```

- 退出码：`2`。
- oracle `verificationStatus`：`REJECTED`。
- oracle `outputWritten`：`false`。
- 精确错误：`register mechanical source/function census mismatch`。
- 结果：`FAIL`。失败发生在独立机械 source/function census 断言，后续 register resolved、switch、集合差分及 v7 状态验收不能计为 formal PASS。
- 持久化输出：无；oracle 明确报告 `outputWritten=false`。

### 只读诊断状态

- 当前已定位到 oracle 的计数口径与 REVERSE 交接第 3.1 节存在实现差异：交接要求“没有 sink 的 source 不进入 census”，而当次 oracle 在识别 source 时即递增 `source_count`，并在函数出现任一 source 时即递增 `functions_with_source`，未先要求该 source 实际产生父 residual sink。
- 独立只读差分得到：当前 oracle 计数为 `1,077 sources / 551 functions`；应用“source 必须产生至少一个匹配父 residual sink”后为封印的 `1,075 / 549`。两个 source-only 项为：
  - function RVA `0x006539F0`，source RVA `0x00653A20`，register `EBX`，IAT slot RVA `0x007187DC`，function SHA-256 `EBA06C7F53D3EB1085CA54298200865F8D01553545EA30BBDFC92473540B18A3`；
  - function RVA `0x00688170`，source RVA `0x00688180`，register `EDI`，IAT slot RVA `0x00718884`，function SHA-256 `9BD6FD4EAB1E0629B8D0092ABBA7C01DF554E73A670171F6284940099FE14219`。
- 最小修复原则是：仅当从该 source 向同寄存器首个语法写之前实际发现至少一个与 v3 residual 父行对应的 `call *%same_reg` sink 时，才递增 `source_count` 并把 containing function 计入 `functions_with_source`；没有 sink 的 source 不计数。该修复不改变 2,333 个 sink projection 或 REVERSE verdict，但必须进入新的 create-new oracle/候选/绑定/TEST 身份，不能回写当次已绑定 oracle。
- 因 v4/v7 已哈希绑定当次 oracle，失败后不得就地修补 oracle、覆盖候选或重跑同一四组链。任何修复必须 create-new 新身份、新绑定和新的独立 TEST 封印/结果链。

## 4. Fail-stop 后续组

| Group | 状态 | 原因 |
| --- | --- | --- |
| Group 2 — v7 public CLI create-new | `NOT_RUN` | Group 1 失败，按预封立即停止 |
| Group 3 — v7 verify-existing / immutable snapshot | `NOT_RUN` | Group 2 未运行，且 Group 1 已失败 |
| Group 4 — two-method parameterized negative batch | `NOT_RUN` | Group 1 失败，禁止越过 fail-stop |

计划中的 Group 2 输出 `out/cs-t0/virtual-device-native-alias-fixed-point-clearing-batch2-verification-20260909-v7.json` 保持不存在；没有临时输出、替代报告或 partial success artifact。

## 5. 保守状态

- Batch 2 当前测试结论是 `FAIL_GROUP_1`，不是 alias promotion。
- `receiverDispatchFixedPointClosed=false`、`pairAliasFixedPointClosed=false`、`dynamicAliasUniverseClosed=false`。
- drainability/native owner 保持 `UNKNOWN`，`VIRTUAL_DEVICE_OBJECT_OWNER` 不提升。
- `formalDecision=null`、`implementationEligibility=false`、`workPackageStatus=BLOCKED_BY_EVIDENCE`。
- 全部权限以及 `csT00506Unlocked`、`t005Unlocked`、`t006Unlocked` 保持 `false`。
- factory/rollback、reference-count、final-release/thread 未在本轮开展。

明确未运行：v4 lifecycle harness、Cargo、完整 T0 build、Native Debug/Release/LTCG 矩阵、历史宽回归、重复 self-test、public positive CLI、负向测试、游戏、x32dbg、进程探针或任何动态测试。
