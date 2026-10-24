> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R05 单行 v3 sealed independent TEST audit v1 交接

日期：2026-09-06  
角色：`INDEPENDENT TEST / POST-CANDIDATE AUDIT`  
状态：`PASS_SEALED_MT_R05_V3_SINGLE_ROW_CANDIDATE / 0 POSITIVE / 1 UNKNOWN`

## 结果

公开 REVERSE candidate
`out/cs-t0/native-join-manager-this-mt-r05-filtered-vector-v3.json` 的物理 SHA-256 为
`DEB2E1301025DA0AB46DFBA2E02017F0BA6A8F27C7EAEE2C7F4FD2C15B9291A6`，与 MAIN
交接一致。它与候选前封印 spec 完全兼容，并通过单行 sealed validator；TEST 没有读取或
import `build-manager-this-mt-r05-filtered-vector-v3.py`，也没有修改 candidate、spec、
validator、frontier 或历史证据。

逐行 verdict：

| 任务 | RVA | verdict | first open | promotion |
|---|---|---|---|---|
| `MT-R05` | `0x0033570B` | `UNKNOWN` | `countAndNoWrap / R05_AUX_NONEMPTY_FILTERED_EMPTY_CONJUNCTION_OPEN` | `false` |

候选保留了封印的精确 countermodel，`baselineCountermodelResolution.resolved=false`，
四义务中的既有 `backingExtent` 没有回退。独立 TEST 只接受该失败安全的 `UNKNOWN`
classification；不接受任何正向行。`frontierCompositionEligible=false`，manager-this、
whole-image 和 overall promotion 均为 false。

## 审计产物

| 文件 | SHA-256 |
|---|---|
| `out/cs-t0/native-join-manager-this-mt-r05-filtered-vector-v3-sealed-independent-audit-v1.json` | `37195EFEE870E371B8915264F3E1AB45906B54508BE08E985B92501B19E73765` |

## 压缩执行

Candidate create-new：

```powershell
python out/cs-t0/audit-manager-this-mt-r05-filtered-vector-v3-sealed-v1.py --spec out/cs-t0/native-join-manager-this-mt-r05-filtered-vector-v3-sealed-acceptance-spec-v1.json --spec-sha256 8872E11ED61E153C2B27A394764FC4959C4E1C6035ABE28D40AED69891BF7759 --candidate out/cs-t0/native-join-manager-this-mt-r05-filtered-vector-v3.json --candidate-sha256 DEB2E1301025DA0AB46DFBA2E02017F0BA6A8F27C7EAEE2C7F4FD2C15B9291A6 --output out/cs-t0/native-join-manager-this-mt-r05-filtered-vector-v3-sealed-independent-audit-v1.json
```

结果：exit 0，create-new SHA-256
`37195EFEE870E371B8915264F3E1AB45906B54508BE08E985B92501B19E73765`，
`acceptedCount=0`、`unknownCount=1`、四类 mutation 全拒绝。

Verify-existing 使用同一命令并追加 `--verify-existing`。结果：exit 0，与 create-new
字节一致；只执行一次。

Wrong-candidate-hash probe 把 `--candidate-sha256` 替换为 64 个零，并使用 create-new
probe 输出名。结果：exit 1，错误为 `candidate physical/requested hash drift`；probe 文件
执行前后均不存在，确认 fail-before-write。

封印的四类最小 candidate mutation 均被拒绝：

1. `target-or-scope-drift`；
2. `evidence-binding-missing`；
3. `countermodel-resolution-weakened`；
4. `illegal-promotion-enabled`。

未运行 Cargo、全量 build、历史宽回归、游戏、进程、x32dbg 或 IDB 操作。冻结 PE
身份、callsite、helper 与 candidate proof-atom 物理字节由 sealed validator 独立重算；
由于 candidate 只申请保留 `UNKNOWN`，本审计没有把 REVERSE 叙述提升为正向真值。
