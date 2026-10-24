> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this bulk remaining frontier v30 candidate

日期：2026-09-05  
角色：`MAIN / EVIDENCE-LEDGER INTEGRATION`  
状态：`CANDIDATE_AWAITING_INDEPENDENT_AUDIT`

## 1. 候选结论

本 create-new v30 候选只把已经作者分离接受的 copy/move 负向审计合入 v29 机器前沿。它没有新增正向闭合，也没有改变任何剩余行的分类：

| 类别 | 正向接受 | 精确全集 | remaining | 独立负向审计覆盖 |
| --- | ---: | ---: | ---: | ---: |
| copy/move | 319 | 405 | 86 | 86/86 |
| memset | 225 | 242 | 17 | 17/17 |
| 合计 | 544 | 647 | 103 | 103/103 |

全部 103 行继续为 `UNKNOWN`。负向审计覆盖仅证明每个剩余候选都已经失败安全地拒绝正向提升；它不构成 backing extent、writer/type fixed point、同代 identity 或 no-overlap 的正向证明。`managerThisPromotionAllowed`、`wholeImageAliasPromotionAllowed`、`overallAccepted` 以及全部权限仍为 false。

## 2. 绑定与精确分区

候选直接绑定 v29 和六份已接受的 copy 负向审计：

| 输入 | SHA-256 |
| --- | --- |
| v29 frontier | `58F2E4728A5DCB67198FE54C09536E818FD0530A371FF0359C1760AA27B5B1D2` |
| EntityList exact2 audit | `0682BA829B3D1EE2E59731A40CEA8BF56B9E47DF6BEAD2461DE164CE905FC5F7` |
| vectorGrow exact23 audit | `C16054194FE4BC2E27E0EF856CB761EE7B64CF910EAAB650B15F9A70DD7AAA2F` |
| local-hard exact5 audit | `95148047034CA738BB426BCD46F68D19B052EA3A67360551E22729E1815A5102` |
| vectorShift exact25 audit | `4F8C5F804E613606D1C386483858217AD55FE8346A90AF3264FDCAF2728516DC` |
| string exact14 audit | `812B0F9F21128829EA85B1C055BE13E750000C838B4BEA44DC71D0BC5634C92E` |
| callerHeap exact17 audit | `6132BEB9B9083FA63980B534A0FA76D9A3EFA446132F63FF143119E34F9AA7DD` |

主线 builder 从这些输入独立重算 `callerHeap 17 + localHard (EntityList 2 + exact5) 7 + vectorGrow 23 + vectorShift 25 + string 14 = 86`。五个机器分组互斥，合集与 v29 `copyMove.remainingRvas` 精确相等；memset 的 `17/17` 覆盖逐值继承 v29。

## 3. 生成与复核

```powershell
python .\out\cs-t0\build-manager-this-bulk-remaining-frontier-v30.py
python .\out\cs-t0\build-manager-this-bulk-remaining-frontier-v30.py --verify-existing
python .\out\cs-t0\build-manager-this-bulk-remaining-frontier-v30.py --verify-existing
```

create-new 与连续两次 verify 均通过并保持相同 JSON SHA。builder 自身的 17 个负向 mutation 全部被拒绝，覆盖 binding、schema/supersedes、两个正向 ledger、remaining/UNKNOWN、负向覆盖、promotion 与权限漂移。

## 4. 候选产物

| 产物 | SHA-256 |
| --- | --- |
| `out/cs-t0/build-manager-this-bulk-remaining-frontier-v30.py` | `79A9430F4BF9D6C03A7AF6972065AF3A901105D4565D408BDB2676C4ED35E215` |
| `out/cs-t0/native-join-manager-this-bulk-remaining-frontier-v30.json` | `12EC336104A9C049EE15E83E634977F2C8CAE3E11AD4AD3E9711DEBCC3D4F814` |

本文件生成后由外部计算 handoff SHA，避免自引用。

## 5. 独立验收边界

候选作者没有使用或 import TEST-role validator 作为实现代码。正式接受必须由此前只编写 validator、未参与本候选编写的测试角色，对以上 candidate SHA 执行参数化验收并生成 create-new audit。验收前 v30 只能称为 candidate，不能更新正式 frontier 状态。

本次仅处理磁盘证据账本，没有继续 save-record 或其它逆向，没有使用 IDA、x32dbg、游戏、进程内存、注入或 runtime mutation，也没有修改生产代码、测试、v29、plan/progress、matrix 或解锁字段。
