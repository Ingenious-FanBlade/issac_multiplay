> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R02/R03/R05 static follow-up v1 handoff

日期：2026-09-06  
状态：`0 STATIC_CONFIRMED_NO_OVERLAP / 5 UNKNOWN`

## 1. 结论

本轮只继续 `MT-R02`、`MT-R03`、`MT-R05` 的 exact5 静态闭包。两名 REVERSE
角色读取冻结 PE/IDA；MAIN 只把已经返回的逐行边界结论序列化为 exact5 candidate；
独立 TEST 在候选出现前封印验收规则，并在候选出现后逐行验收。没有行满足四项全称
义务，因此不得创建空的 positive frontier successor。

| 任务 | RVA | 结果 | 当前首缺口 |
|---|---|---|---|
| `MT-R02` | `0x00008A62` | `UNKNOWN` | `backingExtent`：按 `newCount` 分配、按 `oldCount` 复制，无 `oldCount<=newCount` 硬关系。 |
| `MT-R02` | `0x00008B2A` | `UNKNOWN` | 第二组 descriptor pair 保留同一独立缺口。 |
| `MT-R03` | `0x0001513A` | `UNKNOWN` | `5000` 比较只记录后回流；`5001*84` 超过固定 backing。 |
| `MT-R03` | `0x000155A4` | `UNKNOWN` | 平行路径同样回流；`5001*108` 超过固定 backing。 |
| `MT-R05` | `0x0033570B` | `UNKNOWN` | `auxiliaryRangeCount>0 && filteredCount==0` 未被支配 guard 排除。 |

## 2. 静态补证

### MT-R02

`sub_408970` 的两路 destination 以 `4*newCount` 分配，复制却仅受
`oldCount>0` 支配并使用 `4*oldCount`；没有 clamp 或大小比较。静态复核确认至少
两条 `a3=0` 调用链进入该 direct path：

- `0x0043ABE6 -> sub_40A5D0 -> 0x0040A678 -> sub_408970`；
- `0x004E54C5 -> sub_40AAF0 -> 0x0040AB98 -> sub_408970`。

descriptor `+0x1C/+0x24` producer 没有形成可审计的单调 fixed point，故精确保留
`oldCount=2,newCount=1` 的首缺口；这不是已证明发生的运行时越界，也不能晋升为
no-overlap。

### MT-R03

三个 producer 入口 `0x00410DDC`、`0x00411DE6`、`0x0041300C` 收敛到两条
fixed-5000 构造路径。关键比较 `0x004102F2`、`0x004110E2`、`0x004125C0`
达到 5000 后只记录诊断并继续迭代，没有 return、break 或 clamp。目标 memcpy
仍为 `0x0001513A` 与 `0x000155A4`。因此 5001 是未被静态控制流消除的最小
countermodel；两行的 backing extent 不能正向闭合。

### MT-R05

真实 filtered vector 可以零次 append，而目标 block 只要求独立 auxiliary range
非空。没有发现支配 memmove 的 filtered-nonempty guard。当前最小开放合取保持
`auxiliaryRangeCount>0 && filteredCount==0`，对应 raw length `0xFFFFFFFC`。

若以后只验证该反例的存在性，动态探针必须捕获一次 `sub_734F30` 调用，并同时满足
auxiliary range 非空、所有候选均被过滤、filtered count 为零。当前没有已证能稳定
产生该内部状态的普通用户场景；单次动态样本即使命中，也只能加强负向结论，不能提供
全称 no-overlap 证明。因此本轮没有启动游戏或请求用户操作。

## 3. 作者分离验收

TEST 在候选出现前封印 exact5 contract：按 `backingExtent -> countAndNoWrap ->
sourceLifetimeAndIdentity -> targetIntervals` 顺序逐行验收，正向行必须对
`ALL_REACHABLE_EXECUTIONS` 关闭全部四项。

原 sealed validator 的 candidate-mode mutation harness 暴露两条全 UNKNOWN 候选上的
空操作，以及一处复用末行 `exact` 变量的实现级 scoping bug。两次失败均发生在
create-new 写入前，没有生成 audit。append-only v2 wrapper 只修 harness 与逐行执行，
不修改 sealed spec、preparation、candidate 或 admission contract。最终结果：

- create-new：PASS；
- 恰好一次 verify-existing：PASS、字节一致；
- 六个决策性 mutation：全部拒绝；
- wrong-candidate-hash：写入前拒绝，probe 前后不存在；
- verdict：`0 positive / 5 UNKNOWN`。

## 4. 产物

| 产物 | SHA-256 |
|---|---|
| exact5 sealed spec | `B8EA5449E90F398D2B2514D9176810E4B91986783AE3E0482ECF4B07F79D6A93` |
| sealed preparation | `36231A55B099685A06F00D4F7FE2F315EFCDB11CFCF5266FD0EFE65558593F4C` |
| integrated REVERSE candidate | `47B1E320B329C5571D2C7F6CD35D48EBDF89D69E5EE6B5BC7C659E509689F694` |
| post-candidate TEST wrapper v2 | `9F65707DC077323D28C742151EDEB79950F4CF40D80041F91E6EB6442EA142F9` |
| exact5 independent audit v2 | `C17A2589256FEF6152F5878FE0FADDE7219C68D8B5ED5F852CE9704A85E32ECE` |

详细测试过程见 [sealed preparation](native-join-manager-this-mt-r02-r03-r05-exact5-sealed-preparation-v1-handoff.md)
和 [independent audit v2](native-join-manager-this-mt-r02-r03-r05-exact5-sealed-independent-audit-v2-handoff.md)。

## 5. 权限边界

本轮没有启动游戏、打开进程、附加调试器、执行运行时 mutation 或修改 IDB；没有
运行 Cargo、全量 T0 build 或无关回归。frontier、production、权限、矩阵和解锁字段
均未改变。
