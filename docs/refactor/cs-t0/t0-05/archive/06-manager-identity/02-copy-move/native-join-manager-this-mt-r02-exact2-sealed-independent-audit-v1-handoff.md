> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R02 exact2 sealed independent TEST audit v1 handoff

日期：2026-09-06  
角色：`TEST`  
状态：`PASS_SEALED_MT_R02_EXACT2_ALL_UNKNOWN`

## 结论

独立 TEST 在预封印 exact2 contract 下接受 REVERSE candidate 的物理文件，但两行均保持
`UNKNOWN`，没有任何行可进入 positive frontier：

| RVA | verdict | first open obligation | retained gap |
| --- | --- | --- | --- |
| `0x00008A62` | `UNKNOWN` | `backingExtent` | `R02_OLD_COUNT_LE_NEW_COUNT_AND_BOTH_BACKINGS_OPEN` |
| `0x00008B2A` | `UNKNOWN` | `backingExtent` | `R02_SECOND_OLD_COUNT_LE_NEW_COUNT_AND_BOTH_BACKINGS_OPEN` |

两行都逐字保留冻结 countermodel：

```text
oldCount=2,newCount=1 allocates 4 destination bytes and copies 8 bytes
```

候选的四项义务按 `backingExtent -> countAndNoWrap ->
sourceLifetimeAndIdentity -> targetIntervals` 精确排序，且两行第一项都是第一处
`OPEN`。因此最终分类为 `0 positive / 2 UNKNOWN`，
`frontierCompositionEligible=false`；本审计不授权 frontier、manager-this、whole-image
或 runtime 变更。

## 物理绑定

| 文件 | SHA-256 |
| --- | --- |
| `out/cs-t0/native-join-manager-this-mt-r02-exact2-sealed-acceptance-spec-v1.json` | `7CAEAC7ADE8E53255AAC70E03BC58C8835563F966B706D83BC4CE76B6D822169` |
| `out/cs-t0/prepare-manager-this-mt-r02-exact2-sealed-v1.py` | `E430E0B1851189C534E234821CA901BE8387E76AB86684E72DA323B4FEF51E94` |
| `out/cs-t0/native-join-manager-this-mt-r02-exact2-sealed-preparation-v1.json` | `10E6AACDE120034CFCE108EA35F7BDC6C6CC013F46D1595CA17AD7FA80DD3B03` |
| `out/cs-t0/native-join-manager-this-mt-r02-animation-descriptor-bounded-successor-v3.json` | `D441CE7910F448153140E9F1F75B4EA692276246829ABE244ACF566241C4F46C` |
| `out/cs-t0/audit-manager-this-mt-r02-exact2-post-candidate-v1.py` | `5911034CD2AB055C05534A9460D142F991500E89340191B14544E12B66F628BE` |
| `out/cs-t0/native-join-manager-this-mt-r02-exact2-sealed-independent-audit-v1.json` | `9677D086F43A65F4177BA21619DD189481AA6CC10E6108FA1696827260CAAB4B` |

冻结 PE SHA-256 重新计算为
`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。
TEST 逐个重算 candidate 的八个 proof atom bytes/digest，两个 exact call 均从 PE
解析到 `memcpy 0x00AF05DF`；candidate 的每个公开 binding 也均通过物理 hash 核对。

MAIN 首次转交的 provisional candidate hash 为 `7CEA...A6C`，随后在任何审计命令运行前
明确更正为上述 `D441...F46C`。旧物理版本只被读取，没有产生 audit、probe 或其它输出；
正式审计只以稳定的 `D441...F46C` 启动。

## 独立裁决

`0x00008A62` 的 destination allocation 仍为 `4*newCount`，transfer 为
`4*oldCount`；现有 caller/callee 证据没有形成覆盖所有可达 animation descriptor
transition 的 `oldCount<=newCount` 硬不变量。旧 count 乘法 no-wrap、asset/callback
producer fixed point、source lifetime 和最终区间仍开放。

`0x00008B2A` 保留相同的 count/extent 缺口；此外 asymmetric adoption 和 address-taken
callback producer/reentry fixed point 仍未闭合。两行的 source/destination 与
manager-this slot 非别名都不能在更早义务未闭合时单独提升。

封印的 positive 分支也由 audit tool 保持：只有四项义务全部在
`ALL_REACHABLE_EXECUTIONS` 下 `CLOSED`、baseline countermodel 为 `DISCHARGED`、八类
reachable universe 全部 `CLOSED` 且 `unresolved=[]`，并满足完整 backing/count/lifetime/
interval 结构后才可接受。当前 candidate 不满足这些正向条件，也没有自报正向行。

## 压缩验证

wrong-hash probe 只执行一次：传入 64 个零作为 candidate hash，返回
`candidate requested hash drift`、exit `1`；probe output 在调用前后都不存在，证明
fail-before-write。

正式 create-new：

```text
python out/cs-t0/audit-manager-this-mt-r02-exact2-post-candidate-v1.py --candidate out/cs-t0/native-join-manager-this-mt-r02-animation-descriptor-bounded-successor-v3.json --candidate-sha256 D441CE7910F448153140E9F1F75B4EA692276246829ABE244ACF566241C4F46C --output out/cs-t0/native-join-manager-this-mt-r02-exact2-sealed-independent-audit-v1.json
```

结果：`PASS`，exact `2`、accepted `0`、unknown `2`、mutation rejected `4/4`，audit
SHA-256 为 `9677D086F43A65F4177BA21619DD189481AA6CC10E6108FA1696827260CAAB4B`。

随后只执行恰好一次相同命令加 `--verify-existing`；结果 `PASS` 且字节完全一致。
四个决策性候选变异均 fail closed：

1. exact-row omission；
2. proof-atom PE / sealed-binding drift；
3. quantifier / first-gap drift；
4. permission enabled。

没有读取或 import REVERSE builder，没有修改 candidate、sealed spec/preparation、prior
evidence、frontier、README/status、生产代码或 IDB。没有运行 Cargo、全量 build、历史
回归、游戏、进程、runtime 或调试器测试。

## 最终边界

本 candidate 的 `PASS_SEALED_MT_R02_EXACT2_ALL_UNKNOWN` 只表示其失败安全的逐行
`UNKNOWN` 结果符合预封印契约。两行必须继续留在 remaining；不得据此生成 positive
frontier successor 或宣称 MT-R02 静态闭合。
