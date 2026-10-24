> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 MT-R05 filtered-vector v1 交接

日期：2026-09-06  
角色：`REVERSE / MT-R05`  
状态：`FAIL_SAFE_UNKNOWN_0P1U`

## 1. 结论

`MT-R05 {0x0033570B}` 保持 `UNKNOWN`。当前 PE 中不存在支配 selection/erase
路径的 nonempty guard；不得补造该前提。

真实 filtered owner 已定位为 `sub_734F30` 栈上的三指针
`std::vector<int>`：

```text
[ebp-0x7C] = begin
[ebp-0x78] = end
[ebp-0x74] = capacity
```

`sub_428590` 通过 `sub_40CF00` 分配并替换该 vector 的 heap backing。过滤循环
中的 direct append 与 grow append 都受谓词支配，因而零次 append 是保留路径。
循环结束后 `0x0073553F` 把真实 begin 缓存到 `[ebp-0x6C]`。

在空 filtered range 上：

1. `begin=end=capacity`，元素 count 为零；
2. `0x007356DF/0x007356E1` 检测零值并跳过 `div`，但不退出；
3. `0x007356EB` 仍令 `selected=begin`，即 `selected=begin=end`；
4. `0x00735706` 计算 `end-(selected+4)`，32-bit raw 长度为
   `0xFFFFFFFC`；
5. `0x0073570B` 是冻结的 `memmove` callsite。

因此真实 owner/backing 证明没有消除空范围反例：空 vector 没有可选的四字节
元素，也没有覆盖该 raw 长度的 backing。首缺口保持
`filteredEnd-filteredBegin>=4` 未证明。本交接不声称该路径在实机中完成调用，
也不扩展为漏洞或利用结论。

## 2. 证书结果

逐行 certificate 保持失败安全：

```text
realFilteredOwnerProven = true
filteredBackingExtentProven = true
filteredBeginEndOrdered = true
emptyRangeReachable = true
emptyRangeLengthBytes = 0xFFFFFFFC
emptyRangeCountermodelResolvedByOwnerBackingProof = false
dominatingNonemptyGuardProven = false
synthesizedNonemptyGuard = false
selectedIteratorClosed = true
fullyAdjudicatedNoOverlap = false
disposition = UNKNOWN
promoted = false
```

机器证据绑定冻结 PE、v31 及其独立 audit、Wave B 独立 audit、v33、Wave C
public envelope、callerHeap v29 独立 audit 和 copy-range v7。proof atoms 覆盖
callsite、真实 vector owner/grow helper、空范围路径、selected iterator 和 raw
length arithmetic；七个 direct callers 亦绑定到当前 PE。

`producerClosure.writerRvas` 是 public universe 中
`WRITER/ALIAS/LIFECYCLE` 的去重并集，`callerRvas` 是七个 `CALLER` 的去重
并集。owner lifetime/all-writer fixed point 仍保持 open，不以局部识别代替完整
正向闭包。

accepted 为 `0/1`，所以 `frontierCompositionEligible=false`，不得创建空的
positive frontier successor。

## 3. 精简验证

只执行一次 create-new、一次 verify-existing。6 个核心 mutation 覆盖 binding、
target、exact scope、call bytes、伪造 nonempty guard 和非法 positive promotion；
全部被拒绝。

```powershell
python .\out\cs-t0\build-manager-this-mt-r05-filtered-vector-v1.py
python .\out\cs-t0\build-manager-this-mt-r05-filtered-vector-v1.py --verify-existing
```

两次均报告：

```text
sha256=1FB31C6A04355C0FA03C3F8D69043DFD58EC16B85705E76E0EEFDB66D463F26C
accepted=0 unknown=1 mutations=6/6
```

## 4. 产物

| 对象 | SHA-256 |
| --- | --- |
| `out/cs-t0/build-manager-this-mt-r05-filtered-vector-v1.py` | `D2C2338CB249CD00B884E01371D48470A29FB20BC2056A3D2D34FCA549B601AC` |
| `out/cs-t0/native-join-manager-this-mt-r05-filtered-vector-v1.json` | `1FB31C6A04355C0FA03C3F8D69043DFD58EC16B85705E76E0EEFDB66D463F26C` |

handoff 自身不写入自引用 hash。

## 5. 权限与审计边界

本轮只读取冻结磁盘 PE 和 IDA 静态视图；未启动游戏、未打开或读取目标进程、
未附加调试器、未修改 IDB、生产代码、历史证据、状态、索引或 frontier。本角色
不执行候选审计；后续 TEST 必须仅从 public envelope、candidate JSON、冻结 PE
及公开绑定独立重算，且不得读取或 import 本 builder。
