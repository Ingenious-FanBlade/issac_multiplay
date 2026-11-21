> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 successor frontier v30 独立审计 v1 交接

日期：2026-09-05  
角色：`INDEPENDENT_TESTER / COMPOSE-FRONTIER-SUCCESSOR`  
状态：`PASS_SUCCESSOR_FRONTIER_NEGATIVE_COMPOSITION_ONLY_86_OF_86`

## 1. 独立结论

TEST 角色使用已封存 validator 对 REVERSE/合成角色创建的 v30 candidate 执行了 create-new 独立审计，并连续两次 `--verify-existing`；三次均通过且审计 JSON 摘要一致。

- copy remaining 的负向审计分区独立重算为 `callerHeap 17 + localHard 7 + vectorGrow 23 + vectorShift 25 + string 14 = 86`；五组两两互斥，全集与 v29 的精确 86 行一致；
- callerHeap 审计前已接受负向覆盖为 `69/86`，合入已封存的 callerHeap exact17 独立审计后为 `86/86`；
- `86/86` **仅是负向审计覆盖**，不是任何行的同代 identity 正向闭合；本审计接受的正向闭合数为 `0`；
- copy 正向计数锁死为 `319/405`，memset 正向计数锁死为 `225/242`，remaining 总数为 `103`；
- 103 行全部继续 `UNKNOWN`；`managerThisPromotionAllowed=false`、`wholeImagePromotionAllowed=false`、`overallAccepted=false`，全部权限与 promotion 均为 false。

因此，本审计只允许 v30 合成已由作者分离 tester 接受的负向审计，不允许借 `86/86` 改写 T0-05-04 的正向证明状态。

## 2. 作者隔离与冻结输入

本 TEST 角色未编写、修改或导入 candidate/builder，也未修改 candidate handoff。审计时重新计算并核对以下摘要：

| 输入 | SHA-256 |
| --- | --- |
| v30 candidate | `12EC336104A9C049EE15E83E634977F2C8CAE3E11AD4AD3E9711DEBCC3D4F814` |
| v30 builder | `79A9430F4BF9D6C03A7AF6972065AF3A901105D4565D408BDB2676C4ED35E215` |
| v30 candidate handoff | `07D96144AD7664F0FA462EBD5D665F6478AB406FACC65611E58D40CC4859E1CC` |
| 独立 validator | `F915DE6F32817DB57EC334B22F9ABF1FF71850260FB08452CEA39FA1BC54EFA7` |
| validator self-test v2 | `462FA1DB23BE881AC012706CCD36ADD70AB768014E48F55992623D897C3770A5` |
| callerHeap exact17 independent audit | `6132BEB9B9083FA63980B534A0FA76D9A3EFA446132F63FF143119E34F9AA7DD` |
| v29 frontier | `58F2E4728A5DCB67198FE54C09536E818FD0530A371FF0359C1760AA27B5B1D2` |

candidate 直接绑定 v29 frontier 以及 EntityList、vectorGrow、local-hard、vectorShift、string、callerHeap 六份冻结独立审计；validator 逐项核对 path/hash、schema、精确 UNKNOWN 行集、零正向 delta 和 false 权限。candidate 不能冒充 audit，v29 也没有被覆盖。

## 3. CLI 与结果

create-new：

```powershell
python .\out\cs-t0\audit-manager-this-bulk-remaining-frontier-successor-v30-independent-v1.py `
  --candidate .\out\cs-t0\native-join-manager-this-bulk-remaining-frontier-v30.json `
  --candidate-sha256 12EC336104A9C049EE15E83E634977F2C8CAE3E11AD4AD3E9711DEBCC3D4F814 `
  --caller-heap-audit .\out\cs-t0\native-join-manager-this-copy-move-caller-heap-residual-exact-seventeen-v29-independent-audit-v1.json `
  --caller-heap-audit-sha256 6132BEB9B9083FA63980B534A0FA76D9A3EFA446132F63FF143119E34F9AA7DD `
  --output .\out\cs-t0\native-join-manager-this-bulk-remaining-frontier-v30-independent-audit-v1.json
```

结果：`action=created`，`copyNegativeCoverage=86/86`，`copyClosed=319`，`memsetClosed=225`，`remainingTotal=103`，`mutationsRejected=39`。随后使用完全相同参数附加 `--verify-existing` 连续执行两次，均得到 `action=verified-existing` 和相同摘要。

审计输出 schema 为 `cs-t0.manager-this.bulk-remaining-frontier-v30.independent-audit.v1`；decision 必须为 `PASS_SUCCESSOR_FRONTIER_NEGATIVE_COMPOSITION_ONLY_86_OF_86`，并同时保留 `acceptedPositiveClosureCount=0`、`all103RowsRemainUnknown=true` 及所有 promotion false。

## 4. 负例与 fail-closed

每次正式 create/verify 都重跑 39 个内存 mutation，39/39 全部被拒绝，覆盖：缺失/重复/跨组/越界行、remaining 集漂移、candidate 冒充 audit、binding 缺失/重复/未知、hash 漂移、schema/supersedes/v29 覆盖错误、正向计数变化、UNKNOWN 提升、manager-this/whole-image/overall acceptance 与权限提升，以及 callerHeap 作者隔离和 provenance 篡改。

另行运行的公开 CLI 负例均按预期退出 1：

- 错误 v30 candidate hash：`candidate hash drift`，未创建输出；
- 错误 callerHeap audit hash：`callerHeap audit hash drift`，未创建输出；
- 对正式审计输出再次执行默认 create-new：`create-new output already exists`，既有输出未被覆盖。

## 5. 产物与版本隔离

| 产物 | SHA-256 |
| --- | --- |
| `out/cs-t0/native-join-manager-this-bulk-remaining-frontier-v30-independent-audit-v1.json` | `E9331E974ACD5B6DA0F32A3607A89678B9CA3619A768915CCD23C7EC092695D6` |
| `out/cs-t0/audit-manager-this-bulk-remaining-frontier-successor-v30-independent-v1.py` | `F915DE6F32817DB57EC334B22F9ABF1FF71850260FB08452CEA39FA1BC54EFA7` |

若 save-record stage3 或其它 remaining 行出现正向候选，必须由与作者分离的另一独立审计进入最早 v31；不得回写或放宽本 v30 validator/audit，也不得提前改变 `319/405`、`225/242` 或 103 行 UNKNOWN 状态。

本轮未调用逆向工具、IDA、x32dbg 或游戏，未打开/读取目标进程，未修改 IDB、生产代码、测试代码、既有证据、正式 status/plan/progress/verification/index，也未修改 root 的 candidate、builder 或 candidate handoff。handoff 自身 SHA-256 在文件生成后外部计算并随交接消息报告，避免自引用改变摘要。
