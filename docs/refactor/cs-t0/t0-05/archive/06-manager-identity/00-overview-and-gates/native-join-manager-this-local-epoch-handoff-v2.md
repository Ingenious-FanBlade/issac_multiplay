> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this 同代身份：同步 local epoch 静态闭包交接 v2

记录日期：2026-09-01

状态：`LOCAL_CAUSAL_CONE_STATIC_CONFIRMED / ALL_THREAD_WALL_CLOCK_ALIAS_UNKNOWN`

本交接是 `native-join-manager-this-reader-handoff-v1.md` 的 create-new 第二阶段增量，只回答本次 main-thread 同步 `caller -> manager -> ordinary spawn` causal cone。它不修改既有 v1-v6 封印产物、生产代码或测试，不启动／附加游戏，也不把结论外推为全进程、全线程、动态实机或最终 v7 验收。

机器可核对的地址、原字节与 target 表位于 `out/cs-t0/native-join-manager-this-local-epoch-allowlist-v1.json`。

## 1. 目标构建与结论边界

- IDA input：`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`。
- Win32/x86 image base：`0x00400000`；下文地址均为 RVA。
- observed approved-unpacked SHA-256：`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`。
- canonical SHA-256：`C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30`；只通过既有 equivalence 绑定。
- GameRoot slot：RVA `0x00871678`，4B；PlayerManager offset `0x1BAA8`。

本轮把四个 frontier 拆成两个不同量词：

1. `LOCAL_CAUSAL_CONE`：只量化本次 caller 的同线程同步可达调用、已绑定 vcall/callback 目标及其异常出口；本轮可闭合。
2. `ALL_THREAD_WALL_CLOCK`：量化同一时间窗内任意其他线程、已有但未发现的 computed alias writer；本轮不主张闭合。

因此后文出现的 `STATIC_CONFIRMED` 都必须带上 `LOCAL_CAUSAL_CONE` 作用域，不能改写成“全进程不存在 alias writer”。

## 2. InputManager、device 与 reference-control 的完整绑定

### 2.1 InputManager 入口

生产 reader 要求 runtime vptr 为 derived vtable RVA `0x00782950`。该表的本次路径相关槽为：

| 槽 | entry RVA / 原字节 | target RVA | 调用点 |
| --- | --- | --- | --- |
| `+0x14` | `0x00782964` / `00 1D A2 00` | `0x00621D00` | caller `0x002FA55E`, `FF 50 14` |
| `+0x44` | `0x00782994` / `B0 07 A2 00` | `0x006207B0` | caller `0x002FA56B`, `FF D6`；wrapper 内再进 `+0x74` |
| `+0x74` | `0x007829C4` / `B0 0F A2 00` | `0x00620FB0` | caller 4 点、manager 10 点，均 `FF D0` |
| `+0x28` | `0x00782978` / `90 03 A2 00` | `0x00620390` | dispatcher `0x0062110C`, `FF 50 28` |

base/purecall vptr 仍表示生命周期错误并失效；本轮不放宽 reader 的 derived-vptr 双读条件。

### 2.2 device vtable

对 callback `0x006209E0` 的 device `+0x34`，全镜像对 target `0x0061E970` 的 data xref 只落在三张 device vtable：

- `InputDeviceBase` RVA `0x00782758`，entry `0x0078278C`，字节 `70 E9 A1 00`；
- `InputDeviceKeyboard` RVA `0x007A1678`，entry `0x007A16AC`，字节 `70 E9 A1 00`；
- `InputDeviceGamepad` RVA `0x00782C80`，entry `0x00782CB4`，字节 `70 E9 A1 00`。

所以 `0x00620A17`（`FF D2`）的 concrete target universe 是单元素 `{0x0061E970}`。held callback `0x00620940` 的 `+0x30` 同理全部落到 `0x0061E8D0`。

manager 内此前未闭合的 `0x005BB0B8`（`FF 50 54`）也不再是动态洞：上述三张表的 `+0x54` 都是 `0x0061EE10`。其进一步 `+0x50` 与 `+0x8C/+0x90/+0x94` 目标已在 sidecar 分类型列出；最终只访问 device 自身输入状态、键盘数组或 gamepad 状态，不引用或写入 GameRoot slot。

### 2.3 reference-control vtable

InputManager vector 的 shared device/control pair 只由以下构造来源形成：

| 来源 | vtable 写入 RVA | control vtable RVA |
| --- | --- | --- |
| generic `InputDeviceBase` 包装 | `0x00505FBC` | `0x00778014` |
| keyboard 初始化 | `0x00621AB5` | `0x00782918` |
| gamepad connect | `0x00621EC3` | `0x00782934` |

三张表的 `+0x04` 都是 RVA `0x0000C6B0`，`+0x0C` 都是 RVA `0x000358B0`：

- retain entry bytes：`B0 C6 40 00`；
- release entry bytes：`B0 58 43 00`。

`0x0000C6B0` 在 control 内嵌 mutex 下检查 live strong count，成功时通过同表 `+0x08 -> 0x0000C690` 增加引用。`0x000358B0` 减引用；只有降到零才调用 payload device destructor 与 control 自毁。

在 `0x00620FB0` 的 all-device 分支，InputManager vector mutex 在 `0x00621007` 获取，成功 retain 后才把 pair 交给 callback，随后释放临时引用。vector 自身仍持有一份 strong reference且其结构锁尚未释放，因此该临时 release 不可能把 vector-owned device 降到零。specific-controller miss 由 `0x00502CB0` 构造 null payload control；它的 last release 也没有 device destructor target。由此 device destructor/reentrant destruction 分支对本次查询不可达。

## 3. global release callback 与诊断 logger

global release callback slot RVA `0x0087E814` 的磁盘值／字节为 `0 / 00 00 00 00`。IDA rendered listing 对该槽共有 101 个 code xref，逐项均为 `mov eax,[slot]` 读取；写入为 0 个。没有代码把该槽地址传出。故在 target 未注入／未外部篡改前提下：

```text
GLOBAL_RELEASE_CALLBACK_TARGET = null and stable
```

`0x00621083`、`0x0062116F`、`0x006211B0`、`0x00620A41`、`0x006209A1` 以及 manager 的同类 call 均被前置 null test 剪掉，不存在未知回调 target。

`0x0061E970` 与 `0x0061EE10` 的 action-range 诊断可调用 `0x006112C0`。其唯一可配置 logger object 的非零 writer 是 RVA `0x0065F478`（`89 35 D8 9B C7 00`），对象 vtable 固定为 RVA `0x007A5184`；logger `+0x1C` entry `0x007A51A0`（`D0 3F A8 00`）目标 RVA `0x00683FD0`，只执行 `fwrite/fflush`。因此诊断边也没有 GameRoot writer 或 caller/manager 重入。

## 4. transitive effect 与重入闭包

本次动态边的 concrete targets 包括：InputManager `0x00621D00/0x006207B0/0x00620FB0/0x00620390`，mutex `0x006157F0/0x006159A0`，pair retain/release `0x0000C6B0/0x000358B0`，pair helper `0x00502CB0/0x00502D60`，held/triggered callbacks `0x00620940/0x006209E0`，device action targets与 logger `0x00683FD0`。逐函数反编译／反汇编结论：

- 无 target 对 RVA `0x00871678` 执行 store，也没有形成 `&slot` 后传给任意 callee；
- 无 target 调用三个 direct writer function RVA `0x00552410/0x00559720/0x005AB700`；
- 无 target 调用 caller RVA `0x002FA540` 或 manager RVA `0x005BAC40`；
- caller、manager 与三个 writer function entry 各只有一个 direct code xref，且 entry 没有 data xref，因而不在上述 vtable/callback universe 中；
- Win32 critical-section、sleep、C runtime file/formatting 调用没有 game callback 参数；在 no-injection 前提下不增加 native reentry target。

这关闭的是 concrete local call graph，不是对任意损坏函数指针的防御性证明。runtime vptr/target 不在 allowlist 时必须 fail closed。

## 5. canonical-zero roster 对 player `+0x30` 的不可达证明

caller 在 RVA `0x002FA5CE` 执行：

```text
8B 0E                         mov ecx,[esi]
8B 46 04                      mov eax,[esi+4]
3B C8                         cmp ecx,eax
0F 84 61 01 00 00             jz  RVA 0x002FA73C
```

player vtable `+0x30` call 序列位于 RVA `0x002FA630`，原字节 `6A 12 8B 40 30 FF D0`，只在不相等的 fallthrough roster loop 内。canonical-zero 强制取 `0x002FA73C` successor；该 successor 到 manager call 的 CFG 不回到 `0x002FA5F0..0x002FA736` loop。故：

```text
caller player +0x30 = UNREACHABLE_UNDER_CANONICAL_ZERO_ROSTER
```

这不把 player vtable target 声明为全局 resolved；一旦三指针 roster 不再 canonical-zero，reader 应直接失效。

## 6. SEH、normal-return dominance 与 fail-closed

MSVC FuncInfo 的 `nTryBlocks` 字段均为 0：

| 函数 | handler RVA | FuncInfo RVA | maxState | nTryBlocks | 含义 |
| --- | --- | --- | ---: | ---: | --- |
| manager `0x005BAC40` | `0x0070E93D` | `0x007CF804` | 5 | 0 | cleanup unwind，无 native catch/resume |
| spawn `0x005B9CD0` | `0x0070E889` | `0x007CF748` | 2 | 0 | cleanup unwind，无 native catch/resume |
| triggered callback `0x006209E0` | `0x007115CD` | `0x007D210C` | 2 | 0 | cleanup unwind，无 native catch/resume |
| dispatcher `0x00620FB0` | `0x007116C5` | `0x007D2218` | 6 | 0 | cleanup unwind，无 native catch/resume |

caller `0x002FA540` 没有本地 SEH frame。故这些函数中的 C++ exception 只能执行 cleanup 后向外 unwind，不能被本函数吞掉再伪装成对应 normal return。

当前 production reader 的门序为：spawn 只到 `SpawnValidated` 且不发布；随后必须观察 manager normal exit，再观察 caller 三个 normal return 之一，才进入 `Eligible`。任一 unwind、SEH resume hook、writer、reentry、stop 或乱序均永久失效。因此：

- exception 在 spawn/manager/callback/dispatcher 内传播时，至少缺一个 normal-exit observation，不能发布；
- exception 被 caller 之外更高层恢复也不改变这一点；
- manager/dispatcher cleanup 释放 mutex 只恢复 native resource，不会让 reader epoch 复活；
- 只在 return instruction 打补丁而没有 wrapper-wide unwind observer仍不完整；当前契约必须继续要求 SEH/nonlocal exit 调 `Invalidate`，且任何未闭合 wrapper normal-exit 状态不能跨 epoch 重用。

所以 `LIFECYCLE_AND_EXCEPTION_PRESERVATION` 对“fail-closed eligibility”可静态闭合；它不是“异常后 native 游戏状态一定可继续运行”的证明。

## 7. 对 production reader 的 alias / ABA 判断

### 7.1 同步 local causal cone

在 exact target allowlist、canonical-zero、no injection、三个 writer pre-original 单调失效与三层 normal-exit gate 同时成立时：

1. 本地 causal cone 内没有第四个 slot writer；
2. 三个 direct writer 无论写入值是否最终相等都会先推进／退休 epoch；
3. callback/release/destructor/reentry target 已闭合；
4. nonlocal exit 不可能满足 normal-exit gate。

因此端点相等不再承担 ABA 证明，reader 的 monotonic invalidation 承担该义务：

```text
LOCAL_CAUSAL_CONE_ALL_WRITER_PRESERVATION = STATIC_CONFIRMED
LOCAL_CAUSAL_CONE_ABA_EXCLUSION = STATIC_CONFIRMED
```

对这个一次性、失效后不复活的 reader，单调 `generation + permanently_disarmed` 等价于独立 `gameRootGeneration` 的本 epoch 安全语义。T0-04 owner generation 可作为父 lifecycle epoch，但若移除三个 GameRoot writer 的 pre-original invalidation，owner generation 单独仍不足。

### 7.2 不能外推的最小反例

normal-exit gate、exact local target 与 no injection 本身不能证明 `ALL_THREAD_WALL_CLOCK`。最小未消除反例是：另一个合法 native thread 早已持有一个通过计算得到的 `p = &GameRootSlot`，在两个 reader sample 之间执行 `*p = B; *p = A`，不调用三个 direct writer，也不调用 `Invalidate`。所有本地 callback 都正常返回、三个 normal-exit 全部被观察，reader 仍会误把 A 当作同一代。

本轮没有发现这样的本地边，也没有证据称它实际存在；但消除它需要全进程 computed-alias writer universe 或能覆盖任意 slot 页写入的独立机制，超出本次收缩任务。因此不能把上述 local 结论写成 `ALL_THREAD_WALL_CLOCK_ABA=STATIC_CONFIRMED`。

## 8. 四个 frontier 的最终分层判定

| frontier | local causal cone | 全进程／全线程 |
| --- | --- | --- |
| `CALLER_PRECEDING_CALLEE_PRESERVATION` | `STATIC_CONFIRMED_UNDER_ALLOWLIST` | 不主张 |
| `GAMEROOT_SLOT_ALIAS_WRITER_CLOSURE` | `STATIC_CONFIRMED_UNDER_ALLOWLIST_AND_NO_INJECTION` | `UNKNOWN`，computed alias 量词未闭合 |
| `SAME_LIVE_GAMEROOT_GENERATION_ABA` | `STATIC_CONFIRMED_WITH_MONOTONIC_PRE_WRITER_INVALIDATION` | `UNKNOWN`，保留上一节反例 |
| `LIFECYCLE_AND_EXCEPTION_PRESERVATION` | `STATIC_CONFIRMED_FOR_FAIL_CLOSED_NORMAL_EXIT_GATE` | native 异常后可继续运行不在本证明内 |

推荐主线 schema 同时保存两个字段，避免量词坍缩：

```text
managerThisIdentity = {
  localCausalConeStatus: STATIC_CONFIRMED,
  allThreadWallClockAliasStatus: UNKNOWN,
  exactAllowlistSha256,
  targetSha256,
  canonicalZeroRequired: true,
  derivedInputManagerVptrRequired: true,
  directWriterPreInvalidationRequired: true,
  spawnManagerCallerNormalExitGateRequired: true,
  permanentDisarmOnAnyUnwindOrReentry: true
}
```

是否允许 T0-05-04 总字段由 `UNKNOWN` 提升，取决于该字段的公开量词：若明确是本交接定义的同步 local causal cone，可进入后续独立验收；若声称整个 wall-clock 窗口的真实 GameRoot 同代身份，必须保持 `UNKNOWN`。
