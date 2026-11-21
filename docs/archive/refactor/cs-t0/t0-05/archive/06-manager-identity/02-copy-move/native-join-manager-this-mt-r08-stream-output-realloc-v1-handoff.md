> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 MT-R08 stream/output/realloc v1 交接

日期：2026-09-06  
角色：`REVERSE / MT-R08`  
状态：`PARTIAL_POSITIVE_1P2U`

## 1. 逐行结论

| RVA | 结论 | 关键闭包／首缺口 |
| --- | --- | --- |
| `0x00661577` | `UNKNOWN` | 第二 static ring 的 `0x10000` extent 已闭合，但三个 `sub_91AD50` caller 都从 stream 取得 32-bit `Size` 后直接调用；copy 后的 overflow 报告和 power-of-two 日志都不是硬边界。最小反例为 `Size=0x10001`。 |
| `0x00689F63` | `STATIC_CONFIRMED_NO_OVERLAP` | `sub_A89DE0` 的 direct caller 精确为 `0x00918C19/0x0091AD36`。二者分别在 `0x00918BC5/0x0091ACDB` 以同一 `Sizea` 调用 `sub_A0F4E0`，随后把返回的 fresh allocation 与同一 `Sizea` 传入。公开 allocator 链 `sub_A0F4E0 -> sub_A648B0 -> malloc(Sizea+4)` 返回 allocation `+4`，故 destination extent 覆盖复制长度，且与仍存活的旧 source 及 image slot 分离。 |
| `0x0068E3DB` | `UNKNOWN` | 本地 reset、shift、malloc/realloc、pointer/capacity/used/consumed 写入已绑定，但 nested parent constructor 及 `+0x00B0/+0x00B4/+0x00B8/+0x00BC` 的外部/address-taken writer fixed point 未闭。最小反例是 incoming `capacity>=0` tuple 不满足完整 pointer/backing 与 `consumed<=oldUsed<=capacity` 不变量，却仍能到达正的 shifted length。 |

本轮精确结果为 `1 VERIFIED / 2 UNKNOWN`。只允许把
`0x00689F63` 送交独立 TEST 审计；另外两行不因同任务中的局部正向结果而提升。

## 2. 正向行证书

`0x00689F63` 的冻结 PE 证书满足：

```text
directCallerCountExpected = 2
directCallerCountObserved = 2
callerOutputPointerUniverseClosed = true
callerOutputCapacityUniverseClosed = true
actualExtentCoversSizeaForAllCallers = true
sizeaNoWrap = true
sourceDestinationDisjoint = true
destinationDisjointFromGameRootSlot = true
sourceDisjointFromGameRootSlot = true
fullyAdjudicatedNoOverlap = true
disposition = STATIC_CONFIRMED_NO_OVERLAP
unresolved = []
promoted = true
```

proof atoms 分别绑定 callsite、两个 caller 的 allocation-to-call slices、
`sub_A0F4E0/sub_A648B0` capacity/owner-lifetime 路径。row-local named universes
`outputPointerCallers`、`outputCapacityProducers` 与 `callersAndForwarders` 均为
非空、closed、无 unresolved；所有六个规定 countermodel 均已消解。

## 3. UNKNOWN 行的失败安全边界

`0x00661577` 不把日志当控制流边界。机器证据仍记录第二 static ring 的真实
地址与 extent、stream size producer，以及含该 helper 的 caller/forwarder census；
但 `streamSizeWriters.closed=false`，因此 `Size=0x10001` 保留。

`0x0068E3DB` 不以 `sub_A8E1B0` 内的局部 realloc 算术替代全 writer closure。
机器证据分别列出 nested constructor、`+176` pointer、`+180` capacity、
`+184` used、`+188` consumed 以及 caller universe；五个 tuple writer universe
均失败安全保持 open。

## 4. 精简验证

只执行一次 create-new、一次 verify-existing。8 个 author mutation 覆盖 binding、
目标 PE、exact row 顺序、call bytes、非法提升 UNKNOWN、破坏正向行证书、遗漏
universe 与权限扩张；全部被拒绝。

```powershell
python .\out\cs-t0\build-manager-this-mt-r08-stream-output-realloc-v1.py
python .\out\cs-t0\build-manager-this-mt-r08-stream-output-realloc-v1.py --verify-existing
```

两次均报告：

```text
sha256=90932901F2723C64CE5348D3C964BC54A2CB494111CDA66379FBCA34802C31ED
accepted=1 unknown=2 mutations=8/8
```

## 5. 产物

| 对象 | SHA-256 |
| --- | --- |
| `out/cs-t0/build-manager-this-mt-r08-stream-output-realloc-v1.py` | `5AF6C3053543BE78CF69FD86F656C1780DBD25E753A5AEC09184258F04491CC1` |
| `out/cs-t0/native-join-manager-this-mt-r08-stream-output-realloc-v1.json` | `90932901F2723C64CE5348D3C964BC54A2CB494111CDA66379FBCA34802C31ED` |

handoff 自身不写入自引用 hash。

## 6. 权限与审计边界

本轮只读取冻结磁盘 PE 与 IDA 静态视图；未启动游戏、未打开或读取目标进程、
未附加调试器、未修改 IDB、生产代码、历史证据、状态、索引或 frontier。本角色
不执行候选审计；后续 TEST 必须从 Wave E public envelope、candidate JSON、冻结
PE 及公开绑定独立重算，且不得读取或 import 本 builder。
