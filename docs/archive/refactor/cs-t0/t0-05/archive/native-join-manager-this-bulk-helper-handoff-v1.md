> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 manager-this direct bulk-helper destination handoff v1

## 结论

本证据包对 exact PE32 `isaac-ng.exe` whole-image 中三个 direct bulk helper thunk 的 **1022/1022** 个直接调用点建立了可复现索引，并对 arg0 destination 做只读静态 backward-slice：

- `memcpy`：272；
- `memmove`：306；
- `memset`：444；
- containing functions：541；
- 已完整证明 destination 写区间不覆盖 GameRoot slot：51；
- 尚未完整裁决：971；
- 观察到明确 slot overlap：0。

因此，**whole-image 1022 个调用点不能给出全称正向结论**；状态保持 `UNKNOWN`。这里的 971 个 UNKNOWN 也不能反过来冒充当前 manager wall-clock window 的 writer：它们只是 whole-image 索引项，必须与独立逆向得到的 exact reachable-function set 取交集。

四个 fixed worker root 的函数本体内共有 10 个 direct bulk callsites；这 10 个已全部闭合：

- mod install/update worker：9 个 `memcpy`，9/9 `STATIC_CONFIRMED_NO_SLOT_OVERLAP`；
- network command worker：1 个 `memmove`，1/1 `STATIC_CONFIRMED_NO_SLOT_OVERLAP`；
- input polling worker 与 theoraplayer worker：函数本体内均为 0 个 direct bulk callsite。

这只裁决了 **root-local direct subset**。对 main local causal cone、worker 的 transitive direct callees、以及已解析 indirect targets，仍须执行 machine JSON 中的 `functionRva` join；本证据包本身不提升 manager-this 总命题。

## 目标与边界

- work package：`CS-T0-05-04`；
- preferred image base：`0x00400000`；
- image end exclusive：`0x00CFE000`；
- GameRoot slot VA：`0x00C71678`；
- GameRoot slot RVA：`0x00871678`；
- slot width：4 bytes，目标区间为 `[0x00C71678, 0x00C7167C)`；
- observed SHA-256：`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`；
- canonical SHA-256：`C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30`。

本轮严格遵守 `isaac-cs-refactor` 逆向边界：没有启动游戏、没有附加调试器、没有打开目标进程、没有修改 IDB、没有修改生产 reader、没有修改 v1-v6 或 v7 tooling，也没有修改状态/计划文档。IDA/Hex-Rays 仅用于读出 exact callsites、ctree destination/length、局部定义链、调用者参数与 raw instruction fallback。

## 机器证据

主 machine artifact：

- `out/cs-t0/native-join-manager-this-bulk-helper-destinations-v1.json`
- SHA-256：`348F587C9625277E35C0808E54662D0F85E3B2254329D95877228A8B6F8DE36C`

可复现 analyzer：

- `out/cs-t0/analyze-manager-this-bulk-helper-destinations-v1.py`
- SHA-256：`0454D0F266F344EE548A3D95F238AB482E49C34D9DA1A588475DCD170C3BEF11`

机器 JSON 对每条调用点至少保留：

- `containingFunctionRva`、`callsiteRva`、call bytes 与 helper；
- decompiler destination、length expression、局部 destination definitions；
- raw fallback backward-slice（当 exact ctree EA 不可用时）；
- address domain、domain evidence、span-bound basis；
- exact static range（若可得）及与 slot 的交集判断；
- `fullyAdjudicatedNoOverlap` 与 `explicitSlotOverlapObserved`；
- root-local 10 条的逐点 allocator/capacity inequality；
- 按 `functionRva` 分组的 `reachabilityJoin.index`。

全量 inventory 自校验：

| 项 | 值 |
| --- | ---: |
| direct callsites | 1022 |
| unique callsite RVAs | 1022 |
| containing functions | 541 |
| Hex-Rays exact-EA rows | 1013 |
| raw fallbacks | 9 |
| join-indexed callsites | 1022 |
| inventory SHA-256 | `7666F9C33515995A76AB853BAC8C67BE86929EC26EE6600132A2370EC8F022C7` |

Hex-Rays 另产生 1 个不由同 EA direct thunk xref 支持的 helper expression（`VA 0x0040C978`）；它被记录在 `decompilerExtraNonDirectHelpers`，没有混入 1022 direct-call inventory。

## 判定模型

对每个 helper 使用 forward half-open write range：

```text
W(callsite) = [destination, destination + length)
S(slot)     = [0x00C71678, 0x00C7167C)
```

仅当能证明 `W ∩ S = ∅` 才将该调用点标为 `fullyAdjudicatedNoOverlap=true`。实现显式覆盖：

- destination 直接或间接形成 runtime image base + split constant；
- destination 从 slot 内部起始造成的 partial overlap；
- destination 从 slot 前方开始、较宽 length 横跨 slot 的 wide overlap；
- exact static destination + constant/CFG-bounded length；
- fresh live heap allocation、thread stack/local aggregate 与 loaded image VAD 的互斥地址域；
- heap/stack destination 的完整 allocation/object span bound。

Win32 地址域互斥只用于有效 native invocation：live process-heap block 与 live thread stack mapping 均不等于 loaded main-image VAD。**仅有起点域仍不足够**；除非能进一步证明整个 helper span 留在该 allocation/object 内，否则仍保持 UNKNOWN。这个约束防止把 heap/stack 起点错误地当作任意宽越界写的排除证据。

一个独立的单调规则用于 exact static start：三种 helper 都写向更高地址的 forward range，因此若 exact destination start 已满足 `start >= slot_end`，无需知道 upper extent 也能排除对更低 slot 的 partial/wide overlap。本规则闭合了 6 个此前 length-open 的 static callsites。

## root-local 10 条完整证明

### Mod install/update worker

containing function RVA：`0x004FA390`。9 个调用点均落在 string 的 non-SSO allocation 或 growth 分支；destination 是 `sub_40CF00` 返回的 live process-heap range，且 copy length 被 allocation capacity 支配。

| callsite RVA | helper | destination/copy | 闭合不等式 |
| --- | --- | --- | --- |
| `0x004FA4EE` | memcpy | `pFrom_2[0]`, `length + 1` | `length + 1 <= (length \| 0xF) + 1` |
| `0x004FA59C` | memcpy | `lpPathName[0]`, `length + 1` | `length + 1 <= (length \| 0xF) + 1` |
| `0x004FA74C` | memcpy | fresh growth buffer, `oldSize` | `oldSize <= requestedSize < requestedSize + 1 <= newCapacity + 1` |
| `0x004FA79F` | memcpy | alternate fresh growth buffer, `oldSize` | 同上 |
| `0x004FA87F` | memcpy | path-string growth buffer, `oldSize` | 同上 |
| `0x004FA8D2` | memcpy | alternate path-string growth buffer, `oldSize` | 同上 |
| `0x004FADFE` | memcpy | uninstall non-SSO buffer, `length + 1` | `length + 1 <= (length \| 0xF) + 1` |
| `0x004FAF3B` | memcpy | uninstall growth buffer, `oldSize` | `oldSize <= requestedSize < requestedSize + 1 <= newCapacity + 1` |
| `0x004FAF88` | memcpy | alternate uninstall growth buffer, `oldSize` | 同上 |

`sub_40CF00`（RVA `0x0000CF00`）的只读反编译/反汇编表明它是 native heap allocator wrapper；大块分配的对齐路径最终仍返回新 live heap block。上述 9 条都同时满足：

1. callsite 被相应 non-SSO/growth control-flow branch 支配；
2. destination 来自该 branch 的 allocation result；
3. allocation bytes 不小于 helper length；
4. 完整 write range 留在 live heap allocation；
5. live heap allocation 与 loaded main-image VAD 不相交，故也不能覆盖 slot。

### Network command worker

containing function RVA：`0x0069E950`；唯一调用点为 `memmove` at RVA `0x0069EA1F`。

局部链为：

```text
Size = sourceEnd - sourceBegin
requestedElements = Size >> 3
if requestedElements > currentElementCapacity:
    sub_7099A0(requestedElements)  // v16 vector growth
memmove(v16.begin, sourceBegin, Size)
```

`sourceBegin/sourceEnd` 是同一个 8-byte-element vector 的 begin/end，因此 `Size` 为非负且 `Size % 8 == 0`。`sub_7099A0`（RVA `0x003099A0`）建立的 byte capacity 为 `8 * elementCapacity`，并经 `sub_40CF00` 分配/更新 vector triplet。于是：

```text
Size == 0
OR
Size <= 8 * establishedElementCapacity
```

`Size == 0` 时 write range 为空；`Size > 0` 时 destination 是现有或刚增长的 live heap vector，且完整 memmove span 在 capacity 内。两支都不能覆盖 image slot。

机器 JSON 的 `wallClockRootLocalSubset` 对 10 条逐条保存 domain、branch/allocator evidence、allocation expression、copy expression 和 inequality；汇总为 10 closed / 0 unknown / 0 overlap。

## 其余 whole-image 分类

| address domain / bound | 数量 |
| --- | ---: |
| `EMPTY_WRITE_OR_HEAP:BOUND_CLOSED` | 1 |
| `HEAP:BOUND_CLOSED` | 13 |
| `STACK:BOUND_CLOSED` | 8 |
| `STATIC_IMAGE_GLOBAL:BOUND_CLOSED` | 29 |
| `HEAP:BOUND_OPEN` | 197 |
| `STACK:BOUND_OPEN` | 181 |
| `HEAP+UNKNOWN:BOUND_OPEN` | 94 |
| `STACK+UNKNOWN:BOUND_OPEN` | 21 |
| `HEAP+STACK:BOUND_OPEN` | 2 |
| `HEAP+STACK+UNKNOWN:BOUND_OPEN` | 5 |
| `UNKNOWN:BOUND_OPEN` | 471 |

闭合 static range 中两个值得人工复核的邻近点：

- `dword_C716A0` 的 exact start 为 `0x00C716A0`，在 slot end 之后 `0x24` bytes（在 slot start 之后 `0x28` bytes）；所有 helper 都向高地址写，因此不能回写 `[0x00C71678,0x00C7167C)`。其中 RVA `0x0050CCAA` 与 `0x0050CE3C` 还有常量 length `0xB74` 的完整区间证据。
- logger 两条调用点 RVA `0x006E7EBB`、`0x006E7F9A` 的 destination 为 `0x00C61D88 + counter`；CFG 给出 `counter < 0x8000` 且 `length <= 0x7FFF-counter`，最大 end exclusive 为 `0x00C69D87`，低于 slot。

没有任何已分析行观察到 explicit slot start、partial overlap 或 wide overlap。对 971 个 UNKNOWN，`0 observed overlap` 不是 absence proof。

## UNKNOWN 的最小反模型

以下反模型仍与未闭合静态信息相容，因此不得把 whole-image 状态提升为正向：

1. parameter/no-definition destination：caller 以 `moduleBase + 0x00870000 + 0x1678` 的 split constants 构造 arg0，本地 callee 没有 closed points-to set；
2. pointer-load destination：object/container field 已保存 slot alias，本地只看到 field load；
3. heap/stack start、span open：destination 起点属于互斥域，但未闭合 offset/length 关系，模型允许 write span 离开 allocation/object；
4. unresolved indirect target：若 wall-clock reachable cone 尚未闭合 indirect dispatch，则其内部 helper callsite 不能被当前 direct-function join 静默排除。

## Reachability join 接口

逆向主线应使用 exact wall-clock reachable set，而不是 whole-image 函数全集：

1. 建立 main local causal cone、四个 fixed worker loop、以及 resolved indirect targets 的 exact reachable function RVA set；
2. 用 `reachabilityJoin.index[].functionRva` 与该集合取交集；
3. 对交集中的每个 callsite 检查 `fullyAdjudicatedNoOverlap`；
4. 若交集中出现 false，则只保留这些精确 callsites 作为剩余反模型，不把未取交集的 971 个 whole-image UNKNOWN 计作当前窗口 writer；
5. 仅当 reachable bulk frontier 全部为 true 且 indirect-target closure 独立完成，才能对 bulk-helper writer 子前沿给出正向结论。

当前 machine decision 明确区分：

- `wholeImageBulkHelperDestinationSpanClosure = UNKNOWN`；
- `directFixedWorkerRootLocalBulkSubset = STATIC_CONFIRMED_NO_SLOT_OVERLAP`；
- `exactWallClockReachableBulkFrontier = REQUIRES_REACHABILITY_AND_RESOLVED_INDIRECT_TARGET_JOIN`；
- `managerThisPromotionDecidedByThisEvidenceAlone = false`。

## 复现与审计

在已打开 exact target IDB 的 IDA Python 环境中只读执行 analyzer。脚本包含以下 fail-closed invariants：

- callsite count 必须为 1022；
- helper counts 必须严格为 272/306/444；
- fixed-root local inventory 必须严格为 10；
- 这 10 条 callsite VA 必须与显式 proof table 完全相等；
- root-local 10 条必须全部 closed 且无 overlap，否则拒绝写出结果。

独立 JSON 解析校验结果：1022 个唯一 callsite RVA、541 个唯一 containing function RVA、1022 个 join-indexed callsites、51 closed、971 unknown、0 overlap、root-local 10/10 closed。
