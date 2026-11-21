> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 successor frontier v30 独立验收器 v1 交接

日期：2026-09-05  
角色：`INDEPENDENT_TESTER / COMPOSE-FRONTIER-SUCCESSOR`  
状态：`PASS_VALIDATOR_READY / NO_CANDIDATE_AUDITED`

## 1. 结论与边界

本 TEST 角色完成了 successor-frontier v30 的参数化独立验收器，但没有编写、合成、修改或验收任何 successor candidate。验收器只允许 v30 把已由作者分离 tester 接受的负向审计合入机器前沿：

- copy 精确分区独立重算为 `callerHeap 17 + localHard 7 + vectorGrow 23 + vectorShift 25 + string 14 = 86`；
- 五组两两互斥，全集与 v29 `copyMove.remainingRvas` 精确相等；
- callerHeap 审计前的已接受 copy 负向覆盖为 `69/86`；
- callerHeap exact17 审计封印后，证据层 copy 负向覆盖为 `86/86`；
- memset 剩余负向覆盖保持 `17/17`；
- copy 正向计数必须保持 `319/405`，memset 保持 `225/242`，精确 remaining 保持 `103`；
- 103 行全部继续 `UNKNOWN`，manager-this、whole-image、overall acceptance 与所有权限保持 false。

`86/86` 只表示每个 remaining copy 行都有独立失败安全审计，不是 backing extent、writer/type fixed point、同代 identity 或 no-overlap 的正向证明。

## 2. 冻结输入

| 输入 | SHA-256 |
| --- | --- |
| v29 frontier JSON | `58F2E4728A5DCB67198FE54C09536E818FD0530A371FF0359C1760AA27B5B1D2` |
| EntityList exact2 independent audit | `0682BA829B3D1EE2E59731A40CEA8BF56B9E47DF6BEAD2461DE164CE905FC5F7` |
| vectorGrow exact23 independent audit | `C16054194FE4BC2E27E0EF856CB761EE7B64CF910EAAB650B15F9A70DD7AAA2F` |
| local-hard exact5 independent audit | `95148047034CA738BB426BCD46F68D19B052EA3A67360551E22729E1815A5102` |
| vectorShift exact25 independent audit | `4F8C5F804E613606D1C386483858217AD55FE8346A90AF3264FDCAF2728516DC` |
| string exact14 independent audit | `812B0F9F21128829EA85B1C055BE13E750000C838B4BEA44DC71D0BC5634C92E` |
| callerHeap exact17 independent audit | `6132BEB9B9083FA63980B534A0FA76D9A3EFA446132F63FF143119E34F9AA7DD` |

验收器重算每份固定 audit 的 schema、create-new 身份、精确 UNKNOWN 集、零正向 delta、冻结 ledger 与全 false 权限。callerHeap audit 还必须证明 `INDEPENDENT_TESTER`、未 import candidate analyzer、未把 candidate decision 当真值、17 行全 UNKNOWN、candidate provenance 三件套 hash 精确一致。路径名或结构伪装成 audit 不能通过。

## 3. candidate CLI 与 schema

正式 candidate 尚不存在。逆向/合成角色后续只能创建新的 v30 JSON，不能覆盖 v29。文件必须位于 `out/cs-t0`，名称为 `native-join-manager-this-bulk-remaining-frontier-v30*.json`，并满足：

- `schemaVersion = cs-t0.manager-this.bulk-remaining-frontier.v30`；
- 顶层字段集合与 v29 相同，不接受额外提升声明；
- `identity` 明确包含 `exact103` 与 `negative`；
- `supersedes = { schemaVersion: cs-t0.manager-this.bulk-remaining-frontier.v29, remainingTotal: 103 }`；
- direct bindings 至少包含 v29、EntityList、vectorShift、string、callerHeap 的精确 audit path/hash；vectorGrow 与 local-hard audit 可作为可选 direct binding，因为 v29 已传递绑定两者；除此以外不接受任何 candidate、analyzer 或未知 artifact 冒充 audit；
- `acceptedDelta`、`copyMove`、`memset`、`frontier`、`finalDecision` 与 v29 逐值相等；
- `independentAuditCoverage` 只允许把 copy negative count/RVA/complete 更新为精确 `86/86`，其 `acceptedPositiveCount=4` 与 memset `17/17` 保持不变；
- 所有 permission/promotion 字段均为 false。

candidate 作者交接 SHA 后，独立测试命令为：

```powershell
python .\out\cs-t0\audit-manager-this-bulk-remaining-frontier-successor-v30-independent-v1.py `
  --candidate .\out\cs-t0\native-join-manager-this-bulk-remaining-frontier-v30.json `
  --candidate-sha256 <candidate-sha256> `
  --caller-heap-audit .\out\cs-t0\native-join-manager-this-copy-move-caller-heap-residual-exact-seventeen-v29-independent-audit-v1.json `
  --caller-heap-audit-sha256 6132BEB9B9083FA63980B534A0FA76D9A3EFA446132F63FF143119E34F9AA7DD `
  --output .\out\cs-t0\native-join-manager-this-bulk-remaining-frontier-v30-independent-audit-v1.json
```

成功时只能生成 create-new tester audit，状态为 `PASS_SUCCESSOR_FRONTIER_NEGATIVE_COMPOSITION_ONLY_86_OF_86`；它仍明确记录 `acceptedPositiveClosureCount=0`、`all103RowsRemainUnknown=true` 和所有 promotion false。相同命令加 `--verify-existing` 才能复核已有 tester audit，默认模式拒绝覆盖。

## 4. self-test 与 mutation

封印 self-test 的 create-new 命令：

```powershell
python .\out\cs-t0\audit-manager-this-bulk-remaining-frontier-successor-v30-independent-v1.py `
  --self-test `
  --caller-heap-audit .\out\cs-t0\native-join-manager-this-copy-move-caller-heap-residual-exact-seventeen-v29-independent-audit-v1.json `
  --caller-heap-audit-sha256 6132BEB9B9083FA63980B534A0FA76D9A3EFA446132F63FF143119E34F9AA7DD `
  --output .\out\cs-t0\native-join-manager-this-bulk-remaining-frontier-successor-v30-independent-validator-v1-self-test-v2.json
```

随后以同参数加 `--verify-existing` 连续执行两次，三次均得到相同 JSON SHA-256。self-test 使用内存契约模型作为唯一正向基准，不把该模型写成 candidate；39 个独立负向 mutation 全部拒绝，覆盖：

- 分组缺失、重复、跨组重叠、越界行和 remaining 集漂移；
- 缺失/重复/未知 binding、candidate 冒充 audit、v29/caller/fixed-audit hash 漂移；
- schema 回退、错误 supersedes、覆盖 v29；
- copy/memset/remaining/accepted-positive 非法变化；
- UNKNOWN、manager-this、whole-image、overall acceptance 或权限提升；
- callerHeap 测试作者隔离、17 行、零正向、candidate provenance 和权限篡改。

公开 CLI 额外负向检查均退出 1 且不创建输出：错误 callerHeap hash；self-test 混入 candidate；把 v29 本身作为 candidate；对旧 v1 self-test 执行 v2 verify；对已存在 v2 self-test 再执行默认 create-new。

## 5. 产物

| 产物 | SHA-256 |
| --- | --- |
| `out/cs-t0/audit-manager-this-bulk-remaining-frontier-successor-v30-independent-v1.py` | `F915DE6F32817DB57EC334B22F9ABF1FF71850260FB08452CEA39FA1BC54EFA7` |
| `out/cs-t0/native-join-manager-this-bulk-remaining-frontier-successor-v30-independent-validator-v1-self-test-v2.json` | `462FA1DB23BE881AC012706CCD36ADD70AB768014E48F55992623D897C3770A5` |

旧 `...validator-v1-self-test.json` 是 pause checkpoint v31 已登记的未封印准备文件，保留其原始 `69/86 / conditional 86/86` 视图，不覆盖、不改判；v2 以封印 callerHeap audit 为新输入，是当前 tester readiness 产物。

handoff 自身 SHA-256 在文件生成后外部计算并随交接消息报告，避免自引用改变文件摘要。

## 6. v30 / v31 版本隔离

本 v30 validator 只接受“负向审计覆盖合成”，不接受 save-record stage3 或任何其它正向候选。若 save-record 或其它 remaining 行出现正向候选，必须先由与其作者分离的测试角色独立验收，再进入最早 v31 的新 frontier；不能修改、放宽或借用本 v30 validator 来改变 `319/405`、`225/242` 或 `103`。

本轮未调用逆向工具、IDA、x32dbg 或游戏；没有打开目标进程、读取运行时内存、注入、修改 IDB、生产代码、正式 status/plan/progress/verification/index、v29 或任何 candidate。当前唯一结论仍为 `NO_CANDIDATE_AUDITED`。
