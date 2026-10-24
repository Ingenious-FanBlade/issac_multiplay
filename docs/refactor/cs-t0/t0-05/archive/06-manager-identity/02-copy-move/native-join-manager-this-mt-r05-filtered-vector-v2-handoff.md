> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 MT-R05 filtered-vector v2 续接交接

日期：2026-09-06  
角色：`REVERSE / MT-R05`；独立 `TEST` 前置检查  
状态：`FAIL_SAFE_UNKNOWN_0P1U`

## 1. 结论

`MT-R05 {0x0033570B}` 本轮没有新增正向证据，继续保持 `UNKNOWN`。没有创建
新的 successor，也没有修改 manager-this、frontier、status、index 或生产代码。

静态复核确认：

- filtered vector owner 仍是 `sub_734F30` 的三指针局部对象：
  `[ebp-0x7C] = begin`、`[ebp-0x78] = end`、`[ebp-0x74] = capacity`；
- `0x0073536E` 附近将该 tuple 初始化为空；直接 append 与 grow append 均受过滤
  谓词支配，没有支配 `filteredCount > 0` 的 guard；
- auxiliary 非空只由 `0x00735604` 一侧的检查要求，不能推出 filtered vector 非空；
- 空 filtered 路径在 `0x007356DF` 跳过除法，随后 `selected = begin`，并在
  `0x00735703–0x0073570B` 计算/执行 raw length；
- `sub_428590` 的 backing 分配/替换关系已知，但 capacity/lifetime 仍不能排除
  零元素；最小未决合取仍为
  `auxiliaryRangeCount > 0 && filteredCount == 0`。

因此不能把静态伪代码升级为 no-overlap，也不能补造 nonempty 前提。

## 2. 算术与证据标签

在 `filteredBegin == filteredEnd` 时，目标长度表达式仍为
`end - (selected + 4)`，32-bit raw 结果为 `0xFFFFFFFC`。该结论是
`STATIC_CONFIRMED` 的局部算术和控制流结果；辅助范围与过滤结果的可行性合取仍为
`UNKNOWN`，不声称实机已命中该调用。

冻结目标构建：

`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`

## 3. 现有候选产物

本轮沿用已有 v2 候选，不覆盖或重建：

| 对象 | SHA-256 |
|---|---|
| `out/cs-t0/build-manager-this-mt-r05-filtered-vector-v2.py` | `A25F78724A68D7951CDAA0216A07FB12F48077CD7E3386A5F07E9C8F6958BE8E` |
| `out/cs-t0/native-join-manager-this-mt-r05-filtered-vector-v2.json` | `509D89D51F3983FB98FB2FE0A50A7B5157658972AAC035F94BC288C78D6898EB` |

候选范围为单行 `0x0033570B`，`exactCount=1`，结论为 `UNKNOWN`，callsite bytes
为 `E8 AD B1 3B 00`，`runtimeUsed=false`。

## 4. 压缩测试边界

独立 TEST 角色检查到候选绑定的冻结 PE
`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`
在当前环境不存在。因此不能诚实执行依赖完整 PE 的 current-PE 重算、
create-new/verify-existing 或机器码篡改验收；该部分状态为 `BLOCKED`，不是 PASS。

为避免不必要测试，本轮不运行 Cargo、全量构建、游戏、进程、调试器、动态场景或
历史宽套件。主智能体仅做了非验收性质的公开 JSON smoke：

```text
baseline=true
fakeVerdictRejected=true
promotionRejected=true
scopeDriftRejected=true
```

该 smoke 只检查公开字段与内存副本，不能替代独立 TEST 的 current-PE 验收。

## 5. 下一步

只有在冻结目标 PE 可用后，才由独立 TEST 角色执行一次 create-new、一次
verify-existing 和三项决策性篡改（proof/callsite binding、伪造 positive、
继承行 promotion）；若仍为 `UNKNOWN`，保留本 countermodel，不创建空的 positive
frontier successor。当前不需要动态实验。
