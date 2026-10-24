> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this 同代身份补证暂停检查点 v30

日期：2026-09-02  
状态：`PAUSED / BLOCKED_BY_EVIDENCE`  
范围：仅冻结当前磁盘静态研究与独立审计进度；不授予主动能力，不替代正式 verification、plan 或 progress。

## 1. 暂停结论

本轮 `manager-this` 同代身份完整证明尚未闭合。已静态确认的是普通 receiver 的局部取值关系：

```text
receiver@0x005BB415 = DWORD[GameRoot slot RVA 0x00871678]@0x005BB408 + 0x1BAA8
```

manager guard、ordinary receiver 局部公式与已冻结 manager tuple 证据不能单独推出 `receiver` 与当前 manager `this` 属于同一代实例。正式结论仍为 `FAILED_MANAGER_THIS_IDENTITY_PREMISE / BLOCKED_BY_EVIDENCE`，不得改写为 `STATIC_CONFIRMED`、PASS 或任何 runtime/live 解锁。

当前仍需同时闭合：callee 前后的 manager 身份保持、GameRoot slot 全写者与别名、同一 live generation/ABA 排除、lifecycle/exception 窗口。负向独立审计只确认某些候选行尚不能被接受，不构成这些命题的正向证明。

## 2. 已接受基线

| 基线 | 已冻结值 |
| --- | --- |
| corrected whole-image component v2 | `D1648905B0F2279CAC6A60B98DBE035F57340542EA9A08E02C8D762D33410981` |
| component analyzer | `A5B09328D4B86B7B0F6B772980260FFA4D33722C06CB9C3BD9FCBC71231EACB6` |
| corrected `.text` 映射范围 | RVA `[0x1000, 0x718000)` |
| accepted manager tuple global v20 | helpers `36/36`、sources `282/282`、liveouts `88`、writers `7+2`、growth `3` |
| manager tuple global v20 JSON | `E1D42FABAF1651B01BCC1C7005A2958C0510B26F29BE361A2FE8F71AA44D58C9` |
| exact row `0x005B91EF` | `17792FD0138A8CDADC1D9950C2C018B9C238F88DD20E3E8C85AD8FE8602E3E0F` |

当前机器合成前沿是 [bulk remaining frontier v29](../01-bulk-and-whole-image/native-join-manager-this-bulk-remaining-frontier-v29-handoff.md)：builder `3C866F34231A53FD2607AB4C37A12FF3712EAE6113370B2772D9140A2025F564`，JSON `58F2E4728A5DCB67198FE54C09536E818FD0530A371FF0359C1760AA27B5B1D2`，handoff `C012A5D3C423C1BF9622994793A6B9D816D63EBB975BAC471CB270E6602460A6`。create 与两次 verify 均通过，mutation `12/12`；它冻结的正向计数如下：

| 类别 | 已接受 | 精确全集 | 未闭合 |
| --- | ---: | ---: | ---: |
| copy/move | 319 | 405 | 86 |
| memset | 225 | 242 | 17 |
| 合计 | 544 | 647 | 103 |

v29 之后没有新增正向闭合行，因此以上计数是暂停时的唯一有效计数。v26 之后最后接受的四个非 vector memset 行仍是 `0x0001D1D9`、`0x0024CF7A`、`0x0029FA85`、`0x003DE449`。

## 3. 剩余行与独立审计覆盖

| 剩余组 | 行数 | 暂停时状态 | 是否已独立审计 |
| --- | ---: | --- | --- |
| memset 精确剩余 | 17 | 全部保持 UNKNOWN | 17/17；负向 |
| copy vectorGrow | 23 | 全部保持 UNKNOWN | 23/23；负向 |
| copy local-hard（不含 EntityList） | 5 | 全部保持 UNKNOWN | 5/5；负向 |
| copy EntityList arena | 2 | 全部保持 UNKNOWN | 2/2；负向 |
| copy vectorShift | 25 | 全部保持 UNKNOWN | 25/25；负向 |
| copy string | 14 | 全部保持 UNKNOWN | 14/14；负向 |
| copy callerHeap residual | 17 | candidate 为 0V/17U；尚未验收 | 0/17；独立审计被暂停 |

因此，暂停时可接受的独立负向覆盖为：

- memset 剩余：`17/17`；
- copy 剩余：`69/86`；
- 合计精确剩余：`86/103`。

这里的 `69/86` 是 v29 之后把 EntityList `2`、vectorShift `25`、string `14` 的已完成独立审计计入后的证据视图；v29 JSON 本身仍只绑定 `28/86`，尚未合成这些后续审计。恢复工作时必须 create-new successor frontier，不能回写 v29。

### 3.1 已完成的关键独立审计

| 组 | 独立审计产物 | 结果 |
| --- | --- | --- |
| EntityList arena exact2 | [handoff](../02-copy-move/native-join-manager-this-copy-entity-list-arena-two-v25-independent-audit-v1-handoff.md)；audit JSON `0682BA829B3D1EE2E59731A40CEA8BF56B9E47DF6BEAD2461DE164CE905FC5F7` | `0V/2U`，mutation `26/26`；首缺口为 source-count hard bound 未闭合 |
| vectorGrow exact23 | [handoff](../02-copy-move/native-join-manager-this-copy-move-vector-grow-exact23-v27-audit-v1-handoff.md)；audit JSON `C16054194FE4BC2E27E0EF856CB761EE7B64CF910EAAB650B15F9A70DD7AAA2F` | `0V/23U`，mutation `110/110` |
| local-hard exact5 | [handoff](../02-copy-move/native-join-manager-this-copy-move-local-hard-exact-five-v28-independent-audit-v1-handoff.md)；audit JSON `95148047034CA738BB426BCD46F68D19B052EA3A67360551E22729E1815A5102` | `0V/5U`，mutation `12/12` |
| vectorShift exact25 | [handoff](../02-copy-move/native-join-manager-this-copy-move-vector-shift-exact25-v28-independent-audit-v1-handoff.md)；audit JSON `4F8C5F804E613606D1C386483858217AD55FE8346A90AF3264FDCAF2728516DC` | `0V/25U`，mutation `15/15` |
| string exact14 | [handoff](../02-copy-move/native-join-manager-this-copy-move-string-exact14-v29-independent-audit-v1-handoff.md)；audit JSON `812B0F9F21128829EA85B1C055BE13E750000C838B4BEA44DC71D0BC5634C92E` | `0V/14U`，mutation `15/15` |
| allocator metadata exact1 | [handoff](../03-fill-and-allocation/native-join-manager-this-fill-allocator-metadata-one-v14-independent-audit-handoff.md)；audit JSON `45AEF9007B2988066278CA1CD2BB7A0ACE5A0A6FB2AC41F4D8BCC1608DD3BD6E` | UNKNOWN；alternate backing producer 未闭合 |

memset 非 vector exact8 的独立审计接受 `4V/4U`：四个正向行已进入 v29，`0x000148AA`、`0x005CF2B3`、`0x0062C316`、`0x006EC167` 继续 UNKNOWN；其中 `0x0062C316` 的首缺口为 `FORMAT_CALL_0x009A382B_BACKING_EXTENT_OPEN`。audit JSON 为 `F01AD1505160C661E147CBF6D8A6AA77531DD2398913C07948C435DC2F7B51EB`。

## 4. 暂停时未验收工作

### 4.1 callerHeap residual exact17

候选作者产物已 create 并双重 verify，结果为 `0V/17U`、mutation `85/85`：analyzer `76E0A41D823DE2B8BDF0C566962F7A5641F36938062540CA6C855CDF3B932F69`，JSON `BFE51D1DAEE85058F8904EA1327FECD8A4A1AA8A9EE81CEFD0C01A73CB637DAA`，handoff `1C9271EBC2E9647E3DBAE4EAA67BD6C6F1FB63AC4FAE70E61998422FBE18F8D3`。作者分离的独立审计在执行中被暂停，未产生可接受结论；这 17 行不能计入独立覆盖。

### 4.2 save-record exact1 `0x001CBEB5`

stage2 v24 仍为 UNKNOWN：analyzer `F95032B28291351F3B703619F5D420384CC3DE483FE716A17E826FC53B472EE9`，JSON `17B8B3F661FA01327B1DAD0762A513FA58926E7C62C4CE0184D97802E69D2A9F`，handoff `EF618F96CCA6CCECB6977FB958C0E4DE0F28BD048C143DC51E435A9ADDA0DAC3`。167 个 seed 中 165 个已解析，剩余两个折叠为来自 `0x007B3BE0` 的同一 unresolved stack liveout，首个未闭合调用为 `0x007B3C61 -> 0x005B39D0`。stage3 接续逆向在执行中被暂停，没有可接受的新产物。

### 4.3 重复测试任务

第二份 vectorShift 独立审计在执行中被暂停。现有作者分离的 root 审计已经完整接受，因此该重复任务不是当前计数的缺口；若恢复，只能作为额外复核，不能覆盖既有产物。

## 5. 角色、安全与权限边界

- 逆向候选与独立测试保持作者分离；候选输出必须经另一角色验收后才能进入 accepted ledger。
- 本检查点只使用磁盘静态/PE 证据。没有授权或执行游戏进程打开、注入、live capture、x32dbg/动态调试或主动输入。
- 一次 headless IDA 临时副本分析此前超时并已终止；临时副本已清理，原 IDB 未修改。该结果按 `TIMEOUT_NO_PASS` 处理，不计任何正向或负向证明。
- 本次暂停落盘只增加/追加 Markdown 文档，不改生产代码、JSON、历史 evidence、verification、plan、progress、权限或解锁字段。

## 6. 恢复时的精确任务切片

1. `TEST-CALLERHEAP-17`：由未参与候选编写的测试角色，从冻结 candidate 输入重新完成 exact17 独立审计；任何未完整结束的旧运行均不得续记为 PASS。
2. `REVERSE-SAVE-RECORD-STAGE3`：从 `0x007B3C61 -> 0x005B39D0` 继续闭合唯一 stack liveout；若无法正向闭合，保留最小首缺口与 UNKNOWN。
3. `COMPOSE-FRONTIER-SUCCESSOR`：create-new v30/v31，绑定 EntityList、vectorShift、string 和 callerHeap 的已接受审计，独立核对分组互斥及 `17+7+23+25+14=86`。这一步只同步证据账本，不会把负向覆盖转成正向行。
4. `REVERSE-REMAINING-103`：逐行获得正向 backing extent/identity/generation/lifecycle 证明；只有实际正向闭合才允许改变 `319/405`、`225/242` 或 `103`。
5. `WHOLE-IMAGE-AND-FORMAL-V7`：在全部必要行正向闭合后，才可构建 corrected whole-image raw v2 无重叠证据及 manager-this v7 formal artifacts，并交给独立测试角色验收。

在上述步骤实际完成前，继续沿用 [plan v10](../../../../../../../cs-t0/coop-plan-v10.json) 与 [join progress v4](../../../../../../../cs-t0/coop-native-join-progress-v4.json) 的 `BLOCKED_BY_EVIDENCE`。本检查点不是任务完成声明。
