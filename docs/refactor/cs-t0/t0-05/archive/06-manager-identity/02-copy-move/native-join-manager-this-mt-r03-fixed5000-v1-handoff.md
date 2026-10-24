> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 MT-R03 fixed-5000 v1 交接

日期：2026-09-06  
角色：`REVERSE / MT-R03`  
状态：`FAIL_SAFE_UNKNOWN_0P2U`

## 1. 结论

`MT-R03 {0x0001513A,0x000155A4}` 未取得正向闭合，两行均保持
`UNKNOWN`：

- `0x0001513A` 的目标 backing 固定为 `5000*84=420000` bytes；
- `0x000155A4` 的目标 backing 固定为 `5000*108=540000` bytes；
- 两个函数在 `emittedCount+1 >= 5000` 时只调用
  `"too many frames after interpolation\n"` 的日志路径，返回后继续正常插值和
  `emittedCount` 增长；
- 随后分别按 `84*emittedCount`、`108*emittedCount` 调用 `memcpy`，没有
  dominating return、clamp 或 hard failure。

因此现有控制流不能证明 parser/interpolation 的输入元素数始终不超过 5000。
最小保留反例为 `emittedCount=5001`：两行分别请求 `420084>420000` 与
`540108>540000` bytes。这里仅保留证明缺口，不外推为运行时可达性或漏洞结论。

## 2. 当前 PE 与证据边界

candidate 严格绑定：

- frozen PE SHA-256
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`；
- ImageBase `0x00400000`、PE32/I386、SizeOfImage `0x008FE000`；
- v31 frontier、callerHeap v29 独立审计、copy-range v7、v33 分解文档和 Wave B
  public envelope。

磁盘 PE proof atoms 绑定了两个 `memcpy` callsite、两个 `5000` 检查/日志 basic
block、两个计数增长/回边 basic block，以及三个直接 caller。v29 的完整 direct
caller/forwarder census 被保留；但 parser path、interpolation path、allocator owner
和 count-writer fixed point 没有被伪装为闭合。

逐行任务字段均满足失败安全形式：

```text
fullyAdjudicatedNoOverlap = false
disposition = UNKNOWN
promoted = false
hardBoundaryProven = false
nonBlockingLimitLogRejectedAsBoundary = true
```

正向 accepted 为 `0`，所以 `frontierCompositionEligible=false`，本任务不创建
positive frontier successor，也不改变既有 accepted/remaining 计数。

## 3. 精简验证

按用户要求只执行一次 create-new、一次 verify-existing，并保留 6 个核心
mutation：binding hash、target identity、exact scope、call bytes、非法 hard-bound
提升和非法 row promotion。结果均为 PASS，`6/6` mutation 被拒绝。

```powershell
python .\out\cs-t0\build-manager-this-mt-r03-fixed5000-v1.py
python .\out\cs-t0\build-manager-this-mt-r03-fixed5000-v1.py --verify-existing
```

两次输出均报告：

```text
sha256=2F4FA49768D7EEDC100CDB3C6D757374905F8E3D505B05CD0695C38FA0092B3B
accepted=0 unknown=2 mutations=6/6
```

## 4. 产物

| 对象 | SHA-256 |
| --- | --- |
| `out/cs-t0/build-manager-this-mt-r03-fixed5000-v1.py` | `51CD3D8F294BB392564FBF2266010EB3E6421306DE6E7329C817CBAE7881AECC` |
| `out/cs-t0/native-join-manager-this-mt-r03-fixed5000-v1.json` | `2F4FA49768D7EEDC100CDB3C6D757374905F8E3D505B05CD0695C38FA0092B3B` |

handoff 自身不写入自引用 hash。

## 5. 权限与后续接口

本任务未启动游戏、未打开或读取目标进程、未附加调试器、未修改 IDB、生产代码、
历史证据、状态、索引或 frontier。后续独立 TEST 只能读取 public envelope、candidate
JSON 和公开冻结输入，不得 import 本 builder。若要提升两行，必须从当前 PE 独立证明
全部 parser/interpolation 路径共同满足 hard `emittedCount<=5000`，而不是重新解释
现有日志分支。
