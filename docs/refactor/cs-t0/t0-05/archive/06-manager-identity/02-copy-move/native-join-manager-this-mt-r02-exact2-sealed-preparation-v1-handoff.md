> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R02 exact2 sealed TEST preparation v1 handoff

日期：2026-09-06  
角色：`TEST`  
状态：`SEALED / READY_FOR_MT_R02_EXACT2_REVERSE_CANDIDATE`

## 结论

独立 TEST 已在读取本轮 REVERSE candidate/builder 之前，针对 MT-R02 的精确两行封印
append-only 验收契约：

- `0x00008A62`；
- `0x00008B2A`。

封印从冻结 PE、v33/v35 公开任务接口、R02 历史交接、v33 machine frontier 和
exact5 sealed spec/preparation/audit v2 独立重算。两行仍位于 v33 copy/move remaining，
冻结 call bytes 均解析到 `memcpy 0x00AF05DF`；exact5 v2 的基线 verdict 均为
`UNKNOWN`，首缺口仍是 `backingExtent`。

本轮没有读取、import、编写或猜测新的 REVERSE candidate/builder。没有修改 README、
status、index、frontier、prior evidence、生产代码或 IDB；没有启动游戏、打开进程、
附加调试器或执行 runtime 采样。

## 封印产物

| 文件 | SHA-256 |
| --- | --- |
| `out/cs-t0/native-join-manager-this-mt-r02-exact2-sealed-acceptance-spec-v1.json` | `7CAEAC7ADE8E53255AAC70E03BC58C8835563F966B706D83BC4CE76B6D822169` |
| `out/cs-t0/prepare-manager-this-mt-r02-exact2-sealed-v1.py` | `E430E0B1851189C534E234821CA901BE8387E76AB86684E72DA323B4FEF51E94` |
| `out/cs-t0/native-join-manager-this-mt-r02-exact2-sealed-preparation-v1.json` | `10E6AACDE120034CFCE108EA35F7BDC6C6CC013F46D1595CA17AD7FA80DD3B03` |

后续 candidate 必须逐字绑定 sealed spec 路径及物理 SHA-256。candidate 到达后只允许
新建 append-only independent audit；不得修改本 spec、preparation 或其准入语义。

## 逐行准入规则

两行分别按以下固定顺序裁决，不允许用批次共同结论替代逐行证书：

1. `backingExtent`；
2. `countAndNoWrap`；
3. `sourceLifetimeAndIdentity`；
4. `targetIntervals`。

正向 `STATIC_CONFIRMED_NO_OVERLAP` 只有同时满足以下条件才可接受：

- 四项义务全部在 `ALL_REACHABLE_EXECUTIONS` 下为 `CLOSED`，且每项引用冻结 PE
  proof atom；
- source/destination extent 均覆盖 transfer，长度非负且不回绕，source owner、identity、
  lifetime 和 transfer 期间稳定性闭合；
- source/destination 的 start/end-exclusive 区间均给出，并分别与 manager-this slot
  `[0x00C71678,0x00C7167C)` 不相交；两行都是 `memcpy`，所以 source/destination 还必须
  显式为 `DISJOINT`；
- `countermodelResolution.baselineGapId` 精确对应本行冻结 gap，状态为 `DISCHARGED`，
  statement 与 evidence 非空；
- `universeResolution` 为 `CLOSED / ALL_REACHABLE_EXECUTIONS`，`unresolved=[]`，并完整覆盖
  `direct`、`indirect`、`addressTaken`、`tailTransfer`、`splitEh`、`stackSpillReload`、
  `returnLiveout`、`callbackReentry`；
- `fullyAdjudicatedNoOverlap=true`、`promoted=true`、row `unresolved=[]`，不存在
  `firstUnclosedBoundary`。

`UNKNOWN` 必须保留按上述顺序遇到的第一项 `OPEN` 义务，提供 uppercase gap ID、具体
statement、countermodel、`retained=true` 和非空 unresolved。若第一项仍是冻结
`backingExtent`，gap ID 与 countermodel 必须逐字等于基线；不能用泛化的“universe open”
覆盖更早的 backing gap。

## 冻结首缺口

| RVA | first open | gap ID | countermodel |
| --- | --- | --- | --- |
| `0x00008A62` | `backingExtent` | `R02_OLD_COUNT_LE_NEW_COUNT_AND_BOTH_BACKINGS_OPEN` | `oldCount=2,newCount=1 allocates 4 destination bytes and copies 8 bytes` |
| `0x00008B2A` | `backingExtent` | `R02_SECOND_OLD_COUNT_LE_NEW_COUNT_AND_BOTH_BACKINGS_OPEN` | `oldCount=2,newCount=1 allocates 4 destination bytes and copies 8 bytes` |

任一行保留 `UNKNOWN` 都不得 promotion；另一行即使闭合，也只能按自身证书独立接受。
全部权限字段必须为 false，本封印不授权 frontier、manager-this、whole-image 或 runtime
状态变更。

## 压缩验证

只执行了封印 preparation 的一次 create-new：

```text
python out/cs-t0/prepare-manager-this-mt-r02-exact2-sealed-v1.py --spec out/cs-t0/native-join-manager-this-mt-r02-exact2-sealed-acceptance-spec-v1.json --spec-sha256 7CAEAC7ADE8E53255AAC70E03BC58C8835563F966B706D83BC4CE76B6D822169 --output out/cs-t0/native-join-manager-this-mt-r02-exact2-sealed-preparation-v1.json
```

结果：`PASS`，`candidateRead=false`、exact `2`、mutation rejected `4/4`，preparation
SHA-256 为 `10E6AACDE120034CFCE108EA35F7BDC6C6CC013F46D1595CA17AD7FA80DD3B03`。

随后只执行恰好一次相同命令加 `--verify-existing`；结果 `PASS` 且字节完全一致。
四个决策性变异均 fail closed：

1. exact-row omission；
2. target PE / binding drift；
3. quantifier / first-gap policy weakened；
4. permission enabled。

没有执行额外 wrong-hash probe、candidate mutation、Cargo、全量 build、历史回归、游戏
或调试器测试。

## candidate 交接边界

MAIN 可把本 spec 路径与 SHA-256 交给 REVERSE。待 MAIN 再提供 candidate 的公开路径和
物理 SHA-256 后，TEST 才能读取 candidate，并依据本封印逐行生成 create-new independent
audit；仍不读取或 import REVERSE builder。candidate 交付前，TEST 在此等待。
