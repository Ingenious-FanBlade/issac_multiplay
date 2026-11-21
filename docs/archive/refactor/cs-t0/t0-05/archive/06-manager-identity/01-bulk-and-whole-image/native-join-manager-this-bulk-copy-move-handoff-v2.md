> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 manager-this reachable bulk copy/move Phase-2 handoff v2

## 结论

本阶段按任务切割只裁决 exact wall-clock reachable 的 `memcpy` 与 `memmove`，明确不处理由另一子任务负责的 242 个 `memset`。

Reachability join 已严格闭合其集合身份：

- reachable functions：5646 个唯一 RVA；
- reachable `memcpy`：159；
- reachable `memmove`：246；
- 本子前沿合计：405 个唯一 callsite RVA，分布于 208 个 containing functions；
- 405/405 均按 callsite RVA、helper、owner function RVA 与 call bytes 同时匹配 whole-image v1；
- v1 已闭合：13；
- Phase-2 新闭合：6；
- 累计闭合：19；
- 剩余 UNKNOWN：386；
- 观察到明确 slot overlap：0。

因此本阶段结论是：

```text
reachableCopyMoveFrontier = UNKNOWN
positiveThresholdSatisfied = false
```

不能给出 `STATIC_CONFIRMED_NO_SLOT_OVERLAP`。405/405 且 0 overlap 的正向门槛尚未满足；`0 observed overlap` 不能替代 386 条未闭合 destination/span 的 absence proof。

## 任务切割与边界

本 artifact 只包含：

```text
reachable memcpy 159 + reachable memmove 246 = 405
```

以下内容不由本 artifact 裁决：

- reachable `memset` 242 条；
- manager-this 的其他 direct store、indirect target 或 writer cone；
- manager-this 总命题的最终提升。

本轮严格遵守 `isaac-cs-refactor` 边界：未启动游戏、未附加调试器、未打开目标进程、未修改 IDB、未修改 production/v7、未修改 whole-image v1、未修改状态或计划文档。

## 绑定输入

Whole-image destination v1：

- `out/cs-t0/native-join-manager-this-bulk-helper-destinations-v1.json`
- SHA-256：`348F587C9625277E35C0808E54662D0F85E3B2254329D95877228A8B6F8DE36C`

Reverse reachability component：

- `out/cs-t0/native-join-manager-this-indirect-split-component-v1.json`
- 当前 SHA-256：`EA6560B15FE6F60D694474D05042206662EB07E4C244C8A04B7097C8261532A5`

任务下发时提供的旧 SHA `2C39B2DCC871FABACDC3A128E1C3B066D2827A6803AEE44C4215656AEA8058DD` 已被 reverse component 的 create-new 内容更新取代；647 callsite 数组未变化。Phase-2 builder fail-closed 绑定当前 SHA，避免在 join 中静默接受 TOCTOU 变化。

## 输出

Machine JSON：

- `out/cs-t0/native-join-manager-this-bulk-copy-move-v2.json`
- SHA-256：`D5D531935BE0D108C50AB5177B8FFF18CD223C083A3E0356BEB7DDE7DDA4C263`

Create-new offline builder：

- `out/cs-t0/build-native-join-manager-this-bulk-copy-move-v2.py`
- SHA-256：`F8BA37FD604C2AF50BD0A04F56C7F8C4F549C904BF40FD1831DED71596E26152`

Machine JSON 对 405 条逐点保留：

- `containingFunctionRva`、`callsiteRva`、helper 与双方 call bytes；
- destination、copy-length expression；
- v1 address domains、domain evidence、span bound；
- decompiler destination metadata、局部 definitions、raw fallback；
- direct caller propagation attempt（若 destination 是 parameter）；
- Phase-2 status、proof source；
- UNKNOWN 的精确 reason class、reason 与 minimum countermodel。

同时提供：

- `remainingUnknownIndexByReason`：386 个 RVA 的无损分组索引；
- `joinIndexByContainingFunctionRva`：供后续模板/调用者批处理的 208-function join index；
- `callerPropagationAudit`：parameter destination 的 caller 反传统计。

## 精确 join 校验

Builder 的 fail-closed 条件包括：

1. whole-image v1 SHA 必须严格匹配；
2. reverse component SHA 必须严格匹配；
3. reachable function RVA 必须为 5646 个且全部唯一；
4. reverse helper intersection 必须为 647 个唯一 RVA；
5. reverse helper counts 必须为 `memcpy 159 / memmove 246 / memset 242`；
6. copy/move split 必须严格为 405；
7. 每条必须在 whole-image 1022 index 中命中相同 helper、owner RVA 与 bytes；
8. 每个 owner RVA 必须属于 5646 reachable function set；
9. Phase-2 新 proof key 必须全部属于这 405 条；
10. 当前结果必须为 `13 reused + 6 new = 19 closed / 386 unknown / 0 overlap`。

任何条件不满足，builder 都拒绝发布 JSON。

## Phase-2 新闭合的 6 条

| containing function RVA | callsite RVA | helper | proof |
| --- | --- | --- | --- |
| `0x00023290` | `0x000232CE` | memmove | exact length `0`，write range 为 `[dest,dest)` |
| `0x0035B540` | `0x0035BB19` | memmove | `_white: _DWORD[4]` 为 16-byte stack local，length 6 |
| `0x0035B540` | `0x0035BB64` | memmove | 同一 16-byte stack local，length 6 |
| `0x0035B540` | `0x0035BB85` | memmove | 同一 16-byte stack local，length 5 |
| `0x004318A0` | `0x0043270E` | memmove | `&v93` at `EBP-0x28`，5 bytes 至 `EBP-0x24`，仍在显式 frame 内；下一 named local `v94` at `EBP-0x23` |
| `0x005A3EB0` | `0x005A5AA6` | memmove | `IN_LOBBY: const void *[4]` 在 PE32 为 16-byte stack local，length 7 |

这些证明没有使用“destination 看起来像 stack 就安全”的宽泛规则。每个非空 range 都绑定 exact local/frame extent 与 exact length；完整 write range 留在当前线程 stack mapping，和 loaded main-image VAD 严格不相交。

## 复用的 13 条 v1 证明

v1 中已有 13 条 reachable copy/move callsites 为 `fullyAdjudicatedNoOverlap=true`：

- mod worker root-local 9 个 `memcpy` 的 string heap allocation/capacity 证明；
- network worker root-local 1 个 `memmove` 的 zero-length-or-vector-capacity 证明；
- 2 个 exact static start-after-slot `memcpy`；
- 1 个 copy-length-equals-fresh-allocation-request `memcpy`。

Phase-2 不复制或放宽这些证明规则，而是在每条 machine row 的 `phase2Proof` 中绑定 v1 SHA、address domain 与 span bound。

## 386 条精确剩余反模型

| reason class | 数量 | 未闭合义务 |
| --- | ---: | --- |
| `HEAP_ALLOCATION_CAPACITY_RELATION_OPEN` | 129 | heap 起点可见，但 allocation/capacity 尚未证明支配 `destinationOffset + length` 的所有来路 |
| `HEAP_OR_UNKNOWN_PHI_CAPACITY_OPEN` | 55 | phi/definitions 混合 heap 与未知来源，points-to 与完整 span 均未闭合 |
| `STACK_LOCAL_OR_AGGREGATE_SPAN_OPEN` | 28 | stack/local 起点可见，但缺少 exact object/frame bound |
| `STACK_OR_UNKNOWN_PHI_SPAN_OPEN` | 16 | phi 同时含 stack 与未知来源 |
| `CALLER_PARAMETER_DOMAIN_OR_SPAN_OPEN` | 18 | destination 由 caller 提供，caller points-to 或完整 span 尚未全闭合 |
| `POINTER_OR_CONTAINER_LOAD_UNRESOLVED` | 10 | arg0 来自 field/container/pointer load，loaded value 仍可模型化为 slot alias |
| `RAW_REGISTER_OR_STACK_SLICE_OPEN` | 3 | exact ctree EA 不可得，raw arg0 register/stack slice 仍为 pointer-derived |
| `LOCAL_PROVENANCE_OR_DEFINITION_OPEN` | 127 | 无 closed local points-to set，runtime image base + split constants 仍可满足 |

所有 386 个精确 callsite RVA 均在 machine JSON 的 `remainingUnknownIndexByReason` 中；每条 row 还包含 owner RVA、helper、destination、length、definitions 与 caller attempt，因此后续工作不需要重新抽样或把 whole-image 其他函数混入。

最小反模型保持为：

```text
arg0 或未闭合 phi/field/caller source
= runtime main-image-base + split constants
使 destination 落在 slot 或 slot 前方；
同时未闭合 length/capacity 允许 [dest,dest+length)
与 [slot,slot+4) 相交。
```

只要该模型对任一 reachable callsite 未被排除，就不能提升 copy/move 子前沿。

## Caller 反传结果

18 条 parameter-destination callsites 均保留并复核了 v1 的 direct-caller propagation attempt：

- propagation attempted：18/18；
- direct caller sites 合计：182；
- destination-start domain 在所有 direct callers 上闭合：1；
- 完整 destination span 经 caller propagation 闭合：0。

即使那 1 条 start-domain 已闭合，caller 证据仍没有给出 destination offset + helper length 的完整 object/allocation bound，因此继续保持 UNKNOWN。其余 caller 分支包含 caller-analysis-set 外部、caller decompile 不可用或 caller argument domain 未闭合，不允许抽样正向。

## 后续可继续成组闭合的优先级

若继续 copy/move Phase-3，机器索引支持以下安全顺序：

1. 对 129 条 `HEAP_ALLOCATION_CAPACITY_RELATION_OPEN` 按共享 allocator/container 模板分组，逐模板证明 `offset + length <= capacityBytes`；
2. 对 55 条 heap/unknown phi 先消除每个 unresolved definition，再复用同一 capacity proof；
3. 对 44 条 stack 或 stack/unknown，使用 exact lvar/frame offset 与最大 length 计算，不使用单纯地址域断言；
4. 对 18 条 parameter destination 仅沿 5646 reachable set 内的 exact callers 递归反传，并同时传递 object bound；
5. pointer/container-load 与 raw-slice 集合最后处理，保留 split-constant 与 partial/wide-overlap 反模型直至真实 points-to set 闭合。

在这 386 条全部转为 true 之前，本阶段的准确处置只能是 `RETAIN_EXACT_REMAINING_COPY_MOVE_COUNTERMODELS`。
