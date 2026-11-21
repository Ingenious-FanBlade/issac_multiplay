> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R02 Stage B sealed independent TEST audit v2 handoff

日期：2026-09-06  
角色：`TEST`  
状态：`PASS_SEALED_MT_R02_STAGE_B_ALL_UNKNOWN`

## 结论

REVERSE candidate 在预先封印的 Stage B v2 contract 下通过 admission，但这不是正向
promotion：

| RVA | verdict | first open obligation | retained gap |
| --- | --- | --- | --- |
| `0x00008A62` | `UNKNOWN` | `backingExtent` | `R02_OLD_COUNT_LE_NEW_COUNT_AND_BOTH_BACKINGS_OPEN` |
| `0x00008B2A` | `UNKNOWN` | `backingExtent` | `R02_SECOND_OLD_COUNT_LE_NEW_COUNT_AND_BOTH_BACKINGS_OPEN` |

两行都逐字保留 countermodel：

```text
oldCount=2,newCount=1 allocates 4 destination bytes and copies 8 bytes
```

最终分类为 `0 positive / 2 UNKNOWN`。`admissionPass=true` 只表示 candidate 对开放边、
UNKNOWN 和权限的报告符合 sealed contract；`frontierCompositionEligible=false`、
`frontierMayChange=false`，不得修改 frontier、提升 manager-this 或 whole-image。

## 物理绑定

| 文件 | SHA-256 |
| --- | --- |
| `out/cs-t0/native-join-manager-this-mt-r02-stage-b-sealed-acceptance-spec-v2.json` | `2F2ECB1C5A76990443C93E579DBCA4272C7B8200AA6C1B430F70AADCB3A879EA` |
| `out/cs-t0/native-join-manager-this-mt-r02-stage-b-sealed-preparation-v2.json` | `D33E603114743CEA529DA2F0EEB43BA8F7AC569008ADF6234A6CDBF0DEADB871` |
| `out/cs-t0/native-join-manager-this-mt-r02-frozen-vanilla-asset-manifest-v1.json` | `188A1B2A2F97196045E2E70EDDB378161200CABBB4BE6A5B233B6C20A94044A3` |
| `out/cs-t0/native-join-manager-this-mt-r02-asset-census-state4-graph-v1.json` | `99CC4F2F85711961DD4B89177E97E10DAA1624C4CC2DA9E68C60932979DDA2E7` |
| `out/cs-t0/native-join-manager-this-mt-r02-ida-static-census-v1.json` | `6A90599FD0F0655E448E597A32D96B9A88A231AB4C4B9513ACD8C0456C909990` |
| `out/cs-t0/collect-native-join-manager-this-mt-r02-ida-static-census-v1.py` | `DD0C7923B3C28B0714B8B0292EBBE8208CF54C2E831DD6EC553F1CB0C8BE1487` |
| `out/cs-t0/build-native-join-manager-this-mt-r02-asset-census-state4-graph-v1.py` | `98DC479DC810F37A960381A6BF66B5060E1CCB3547ABB9C52CAAFDCD517C328F` |
| `out/cs-t0/audit-manager-this-mt-r02-stage-b-post-candidate-v2.py` | `7E5861F34F305DFD790E2C9DEECB048F161D3616AB4F8EADE9114FDDE38257E8` |
| `out/cs-t0/native-join-manager-this-mt-r02-stage-b-sealed-independent-audit-v2.json` | `CD9A55343C66BA724E6AB25101CEF29095348E6D37A511ED5B71E35F6B034D8B` |

冻结 PE 物理 SHA-256 重新核对为
`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。
candidate builder 与 IDA collector 仅做物理 hash 绑定；TEST 没有读取其源码内容、import
或执行它们。

## 唯一 deterministic census

独立工具只完成一次 frozen asset traversal：同一单遍中读取并核对 manifest 绑定的
`2373` 个文件，然后解析 `2372` 个 ANM2 和 `1` 个 `entities2.xml`。重算结果：

- parse failure `0`，Animation `13261`；
- `LayerAnimations` 最大值 `57`，`NullAnimations` 最大值 `26`；
- focus animation `973`；
- pre-prune risk `500`，其中 `0x00008A62=253`、`0x00008B2A=247`；
- asset-local exact `old=2,new=1` risk `173`，focus-name exact risk `1`；
- entities2 `1342` 项，resolved `1340`、no-reference `2`、unresolved/invalid `0`。

独立重算的完整 asset rows、focus rows 和 risk core rows 与 manifest 逐项一致，并与
candidate observed census 一致。规范化 census digest 为
`299CF6E469F2FAAB19317D95EACB1BBF0DF87E885A1671921E977F7F78999775`。

唯一 census 完成后，首个 pre-output audit attempt 在静态 validator 阶段误把两条已有
proof-atom 支持的 finite-literal 精化当作 IDA 字段漂移：

- `0x0003ABE6`: `Idle` / `Idle2`；
- `0x000E54C5`: `HeadLaughUp` / `HeadLaughDown` / `HeadLaughRight`。

该次调用没有创建 audit 文件。validator 只对白名单中的这两条既有 proof-atom 精化修正
语义，然后以 no-census resume 完成 create-new audit；resume 进程 asset traversal 为 `0`，
全任务 deterministic census 总数仍严格为 `1`。没有执行 verify-existing 或第二次 census。

## exact graph coverage

独立核对结果：

- exact rows `0x00008A62/0x00008B2A`、target/manifest/spec hashes 全匹配；
- 17 个 proof atom 的 bytes/digest 均从冻结 PE 重算；
- selector original `1048/91`；仅 `PROVED_NONZERO flag!=0` 被剪除 `935/79`，
  `PROVED_ZERO` 与 UNKNOWN 全部保留，最终 retained `113/12`；
- `sub_408830/sub_408970/sub_4086E0` writer xref 覆盖为 `60/2/2`；
- sealed 的 13 个 required risk edge id 全部出现，无 omission/duplicate；
- parser field 关系和三个 writer 的局部语义保持有界闭合；selector owner/source、
  copyMove、reload、addressTaken、asset-max reachable join 和 `0x8B6A` adoption 的全称边
  继续明确 `OPEN`，没有伪报 fixed point。

因此四项义务仍按 `backingExtent -> countAndNoWrap -> sourceLifetimeAndIdentity ->
targetIntervals` 在第一项停止，两行 UNKNOWN 合法且 fail closed。

## 压缩负向验收与权限

三个决策性 mutation 全部被同一 audit validator 拒绝，且复用唯一 census 结果：

1. target / manifest binding hash drift；
2. required risk-edge omission；
3. selector / observed census count tamper。

没有运行 candidate builder、Cargo、全量 T0/build、历史回归、游戏、进程、runtime、IDA
mutation 或 x32dbg。没有修改 candidate、sealed spec/preparation、production、frontier、
README/status/index 或既有证据；唯一新增文件是 audit tool、audit JSON 与本 handoff。
