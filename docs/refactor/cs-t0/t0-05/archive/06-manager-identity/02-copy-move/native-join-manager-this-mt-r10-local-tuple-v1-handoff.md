> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 MT-R10 local-tuple v1 交接

日期：2026-09-06  
角色：`REVERSE / MT-R10`  
状态：`FAIL_SAFE_UNKNOWN_0P1U`

## 1. 结论

`MT-R10 {0x002EE04F}` 保持 `UNKNOWN`。冻结 PE 中，目标调用点位于
`sub_6EDFA0`，调用字节为 `E8 8B 25 40 00`，目标是
`memcpy (VA 0x00AF05DF)`。本地路径从 `this+0x5C0` 读取 `n4`，在
`n4==0` 时提前退出；实际复制长度为 `4*n4-4`，目标是调用时
`ESP+0x18` 的 32-byte 栈对象。

当前证据完整登记了既有 census 中的 22 个 direct writer RVA 和 28 个 LEA
alias RVA，但尚未完成每个 direct writer、alias helper/address-taken/return effect
及传递复制的语义闭包。首个未闭合传递点仍是 `0x003B1400`：它把
`source+0x2094` 复制到 `this+0x5C0`，而该 source 字段的 producer fixed point
未闭合。因此局部 `n4>=1` 成立，所需全局上界 `n4<=9` 不成立为已证明事实。

保留的最小反例为 `n4=10`，此时长度 `4*10-4=36`，超过已证明的 32-byte
目标 extent。该反例只用于说明正向证明缺口；本交接不扩展为漏洞或利用分析。

## 2. 证书结果

逐行 certificate 保持失败安全：

```text
directWriterCountExpected = 22
directWriterCountObserved = 22
leaAliasCountExpected = 28
leaAliasCountObserved = 28
allDirectWritersClassified = false
allLeaAliasesClassified = false
transitiveCopyFixedPointClosed = false
source2094ToDestination5C0Proven = true
n4LowerBound = 1
n4UpperBound = 9
n4RangeProven = false
localDestinationExtentBytes = 32
fullyAdjudicatedNoOverlap = false
disposition = UNKNOWN
promoted = false
```

机器证据绑定冻结 x86 PE、v31 frontier 及其独立 audit、Wave C 独立 audit、
v33、Wave D public candidate envelope 和 local-hard exact5 v28 独立 audit；
同时以 v28 candidate、local-stack v4 及其 audit 作为 supplemental public evidence。
proof atoms 重新绑定 callsite、函数入口、`n4` 读取和零值分支、长度计算、
一个局部 clamp writer，以及 `source+0x2094 -> this+0x5C0` 传递点的当前 PE
字节。

`producerClosure.writerRvas` 是 public universe 中
`WRITER/ALIAS/LIFECYCLE` 的去重并集，共 50 个 RVA；`callerRvas` 是四个
`CALLER` 的去重并集。census 数量一致不能替代语义分类与 producer fixed point，
因此 `producerClosure.closed=false`。

accepted 为 `0/1`，所以 `frontierCompositionEligible=false`，不得创建空的
positive frontier successor。

## 3. 精简验证

只执行一次 create-new、一次 verify-existing。6 个 author mutation 覆盖 binding、
目标 PE、exact scope、call bytes、伪造 writer/alias/fixed-point 闭包和非法 positive
promotion；全部被拒绝。

```powershell
python .\out\cs-t0\build-manager-this-mt-r10-local-tuple-v1.py
python .\out\cs-t0\build-manager-this-mt-r10-local-tuple-v1.py --verify-existing
```

两次均报告：

```text
sha256=C47B3CCEB50BFC0DFA61F8CA1F8A447ACDD6C09016240720F6300782BE1A2B43
accepted=0 unknown=1 mutations=6/6 writers=22 aliases=28
```

## 4. 产物

| 对象 | SHA-256 |
| --- | --- |
| `out/cs-t0/build-manager-this-mt-r10-local-tuple-v1.py` | `5F4AB78D50790155CF716CD34DCAEB98370A9CCDAEAD67DFC04E915963312C0F` |
| `out/cs-t0/native-join-manager-this-mt-r10-local-tuple-v1.json` | `C47B3CCEB50BFC0DFA61F8CA1F8A447ACDD6C09016240720F6300782BE1A2B43` |

handoff 自身不写入自引用 hash。

## 5. 权限与审计边界

本轮只读取冻结磁盘 PE 和 IDA 静态视图；未启动游戏、未打开或读取目标进程、
未附加调试器、未修改 IDB、生产代码、历史证据、状态、索引或 frontier。本角色
不执行候选审计；后续 TEST 必须仅从 public envelope、candidate JSON、冻结 PE
及公开绑定独立重算，且不得读取或 import 本 builder。
