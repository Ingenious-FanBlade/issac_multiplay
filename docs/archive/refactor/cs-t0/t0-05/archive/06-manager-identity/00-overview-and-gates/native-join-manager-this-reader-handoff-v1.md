> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this 同代身份：production reader 最小静态交接 v1

记录日期：2026-09-01

状态：`SPECIFIED_WITH_STATIC_ANCHORS / NATIVE_ALIAS_AND_ABA_UNKNOWN`

本交接只冻结 manager-this 身份所需的最小 Hook／采样契约。它不修改既有 v1-v6 证据、plan v10、join progress v4、生产代码或测试，也不授予 runtime、active、live、05-05、T0-05 总体或 T0-06 权限。

## 1. 目标与证据边界

- IDA input：`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`。
- IDA module：Win32/x86 `isaac-ng.exe`，preferred image base `0x00400000`，image size `0x008FE000`。
- 本次实际静态目标 SHA-256：`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`（approved-unpacked observed target）。
- canonical target SHA-256：`C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30`；二者只通过既有 equivalence document 绑定。本交接中的字节直接来自 observed target，不能脱离该绑定外推。
- 下文地址均为 RVA；IDA VA 等于 `0x00400000 + RVA`。GameRoot slot 为 RVA `0x00871678`（VA `0x00C71678`），宽度 4 字节；嵌入 PlayerManager offset 为 `0x1BAA8`。

既有 `ORDINARY_SPAWN_RECEIVER_FROM_GAMEROOT_SLOT` 局部公式保持成立。本轮新增的是 direct-writer 同步 epoch 分类、InputManager 动态 vtable 分层，以及可实现的 generation/异常/重入失效契约；不是完整 ABA 证明。

## 2. 最小指令锚点与 ABI

| 角色 | RVA | 原字节 | 指令／ABI | 静态结论 |
| --- | --- | --- | --- | --- |
| caller 入口 | `0x002FA540` | `55 8B EC A1 18 7B C5 00 83 EC 34 B9 18 7B C5 00 53 56` | `sub_6FA540`；IDA `char()`；无栈参数，普通 `ret`，保存 `EBP/EBX/ESI/EDI` | 入口没有函数本地 SEH frame |
| caller GameRoot 采样 | `0x002FA556` | `8B 3D 78 16 C7 00` | `mov edi,[0x00C71678]` | caller 用本次采样的 GameRoot 保存在 `EDI` |
| caller receiver 定义 | `0x002FADA1` | `8D 8F A8 BA 01 00` | `lea ecx,[edi+0x1BAA8]` | manager receiver 来自 caller 入口保存值 |
| caller→manager | `0x002FADA7` | `E8 94 FE 2B 00` | direct `call RVA 0x005BAC40`；`thiscall`，`ECX=PlayerManager*`，无栈参数 | manager entry 的全 PE 唯一 direct code xref |
| manager 入口 | `0x005BAC40` | `55 8B EC 6A FF 68 3D E9 B0 00 64 A1 00 00 00 00 50 83 EC 2C` | `unsigned int __thiscall(PlayerManager*)`；SEH handler RVA `0x0070E93D` | 必须在 wrapper 入口冻结 `ECX` 与 generation tuple |
| manager 保存 this | `0x005BAC68` | `8B F9 89 7D E4` | `mov edi,ecx; mov [ebp-0x1C],edi` | possessor 路径使用保存值；ordinary 路径稍后重新读 slot |
| ordinary reload | `0x005BB408` | `8B 0D 78 16 C7 00` | `mov ecx,[0x00C71678]` | 新一次 GameRoot 采样，不能靠地址相等推出同一代 |
| ordinary 参数保持 | `0x005BB40E` | `52` | `push edx` | 不改 `ECX` |
| ordinary receiver | `0x005BB40F` | `8D 89 A8 BA 01 00` | `lea ecx,[ecx+0x1BAA8]` | receiver 来自 ordinary reload |
| ordinary spawn | `0x005BB415` | `E8 B6 E8 FF FF` | direct `call RVA 0x005B9CD0`；`_DWORD* __thiscall(PlayerManager*, playerType)` | stack 上一个 4 字节 player type 参数；正常返回落在 RVA `0x005BB41A` |
| spawn 入口 | `0x005B9CD0` | `55 8B EC 6A FF 68 89 E8 B0 00 64 A1 00 00 00 00 50 83 EC 08` | IDA `_DWORD* __thiscall(PlayerManager*, _DWORD*)`；实际 ordinary 参数由上项 `push edx` 提供；SEH handler RVA `0x0070E889` | 反编译类型名不作为公开 ABI；x86 事实是 `ECX` receiver＋一个 4B 栈参数 |

caller 的三个普通返回是 RVA `0x002FA87F`、`0x002FACB4`、`0x002FADB2`，原字节均为 `C3`。manager 只有一个普通返回 RVA `0x005BB5C3`，原字节 `C3`。caller 本身没有 SEH，任一先行 callee 的非局部退出都可以绕过三个普通返回，因此只在返回点清 token 不完整。

## 3. GameRoot slot 三个直接 writer

独立磁盘 operand scan 的既有结论保持：全镜像只发现以下三个直接 4B overlap writer；没有直接宽写，也没有明确 exact-address formation。这个结果是 `DIRECT_WRITER_UNIVERSE=STATIC_CONFIRMED`，不是计算别名／间接 writer 闭合。

| writer 指令 RVA | 原字节 | 所在函数／ABI | 值来源与 lifecycle 含义 | 唯一 direct caller |
| --- | --- | --- | --- | --- |
| `0x00552847` | `A3 78 16 C7 00` | `sub_952410` RVA `0x00552410`；无显式参数、普通 `ret`，有 SEH handler RVA `0x0070BF11` | `EAX` 是 `alloc(0x68E88)` 后 `sub_6F1020` 构造结果；`INITIAL_PUBLISH_OR_REPLACE` | `sub_9AA040` RVA `0x005AA040` 的 callsite RVA `0x005AAAB5`，字节 `E8 56 79 FA FF` |
| `0x005597D0` | `A3 78 16 C7 00` | `void __thiscall sub_959720(n298*)` RVA `0x00559720`，`ECX=n298`，有 SEH handler RVA `0x0070C36D` | 条件性 `alloc(0x68E88)`＋`sub_6F1020` 后把 `EAX` 发布到 slot；`ROOT_RECREATE_OR_REPLACE` | `sub_954CD0` RVA `0x00554CD0` 的 callsite RVA `0x00554D5E`，字节 `E8 BD 49 00 00` |
| `0x005AB8A9` | `C7 05 78 16 C7 00 00 00 00 00` | `sub_9AB700` RVA `0x005AB700`；无显式参数、普通 `ret`，有 SEH handler RVA `0x006F12F0` | 先执行 `sub_6F3060(root)` 和 free，再把 slot 置零；`ROOT_RETIRE` | `_main` RVA `0x00531050` 的 callsite RVA `0x005313DE`，字节 `E8 1D A3 07 00` |

同步 direct-writer epoch 的额外静态事实：

- caller `sub_6FA540` 由 `sub_954CD0` 的 RVA `0x005551BD` 调用，原字节 `E8 7E 53 DA FF`；同一 outer 函数中的 replacement writer callsite `0x00554D5E` 位于其前。
- `sub_954CD0` 的 134-block CFG 中，从 caller call block RVA `0x005551BD` 的后继出发不可回到 writer call block RVA `0x00554D5C..0x00554D73`。
- startup writer 和 shutdown writer 各只有上述一个 direct code caller；三个 writer 都不在 `sub_6FA540` 或 `sub_9BAC40` 的 direct instruction/call inventory 中。

因此可以登记 `DIRECT_WRITER_LOCAL_CALL_EPOCH_PRESERVATION=STATIC_CONFIRMED`：没有显式 CFG 边在本次 caller→manager→ordinary-spawn 同步 direct call epoch 内执行三条已知直接 store。它不能升级为 `ALL_WRITER_LOCAL_CALL_EPOCH_PRESERVATION`，因为计算别名、未知 callback/reentry 和异常恢复仍未闭合。

## 4. 先行间接调用的最小收窄与重入点

caller 先前记录的 7 个间接调用可静态收窄为 6 个 InputManager 调用和 1 个 roster 元素虚调用：

- RVA `0x002FA55E`：`[InputManager.vptr+0x14]`。磁盘初始/base vtable RVA `0x007827F4` 的该槽是 `_purecall`；静态初始化函数 RVA `0x00002280` 在 RVA `0x0000233B` 把对象 vptr 写成 derived `KAGE::Input::Manager` vtable RVA `0x00782950`，该槽目标是 RVA `0x00621D00`。析构链 RVA `0x0061F984` 又把 vptr 改回 base。reader 必须要求 runtime vptr 为 derived `0x00782950`；base/purecall 表示生命周期错误并失效，不能把磁盘初值当运行时目标。
- RVA `0x002FA56B`：derived/base 两张 vtable 的 `+0x44` 都指向 RVA `0x006207B0`；该 wrapper 又调用 `+0x74`。
- RVA `0x002FA91B`、`0x002FA997`、`0x002FA9BF`、`0x002FAD85`：derived/base 的 `+0x74` 都指向 RVA `0x00620FB0`。
- RVA `0x002FA635`：roster 元素 vtable `+0x30`，目标宇宙仍未绑定；在 prejoin 三指针 canonical-zero 前提下，caller 的 RVA `0x002FA5CE..0x002FA5D5` 直接跳过包含该 call 的 roster loop，因此它只可登记为 `UNREACHABLE_UNDER_CANONICAL_ZERO_ROSTER`，不能登记为全局 resolved。

InputManager 查询仍包含显式重入边界：RVA `0x00621093`、`0x00621130` 调用传入 callback；当前 caller 传入 RVA `0x006209E0`。该 callback 在 RVA `0x00620A17` 调 device vtable `+0x34`，并可在 RVA `0x00620A41` 调全局 release callback slot RVA `0x0087E814`。`sub_A20FB0` 自身还有 control/device vtable 与同一 release callback 调用，并带 SEH handler RVA `0x007116C5`。这些回调可能重入或非局部退出；静态端点相等不能关闭它们。

## 5. production reader 的最小 generation 契约

不得用 caller 入口值、manager entry 值和 ordinary reload 值最终相等排除 A→B→A。最小实现需要 adapter-owned 单调身份，建议把旧 T0-04 owner generation 与新 GameRoot generation 组合，而不是复用一个指针：

```text
ManagerEpochIdentity = {
  t004OwnerGeneration,
  gameRootGeneration,
  callEpoch,
  mainThreadId,
  gameRootPointer,
  playerManagerPointer = gameRootPointer + 0x1BAA8
}
```

建议的 create-new schema 字段：

```text
managerThisReaderContract = {
  schemaVersion,
  targetSha256,
  imageBase,
  slotRva,
  embeddedOffset,
  hookAnchors[],
  writerAnchors[],
  invalidationEvents[],
  samplePoints[],
  normalExitRvas[],
  exceptionHandlers[],
  requiredT004OwnerGeneration,
  gameRootGenerationOwnedByAdapter: true,
  directWriterUniverseComplete,
  aliasWriterUniverseComplete,
  nativeAbaExclusionProven,
  readerImplemented,
  failureMode
}
```

每个线程的 `callEpoch` 至少执行：

1. caller 入口 wrapper：在调用 original exactly-once 前双读 slot，冻结 `{t004OwnerGeneration, gameRootGeneration, root, thread}`；非主线程、空值、读取失败、双读漂移或已有未闭合 epoch 立即失能。
2. manager entry wrapper：读取原始 `ECX`，要求 `ECX-root==0x1BAA8`，两个 generation 未变；记录 manager entry。wrapper 不改变参数和返回值。
3. ordinary reload／spawn wrapper：采用 RVA `0x005BB408` 实际装入的 root，要求其与 epoch root 一致且 `gameRootGeneration` 未变；RVA `0x005BB415` original call exactly-once。相等只是必要条件，generation 不变才承担 ABA 排除。
4. 三个 writer 必须在执行原始 store 前推进 `gameRootGeneration` 并使所有 active call epoch 永久 ineligible；无论新旧指针是否相等都推进。原始 store 仍恰好执行一次。事件发布、计数溢出、嵌套 writer 或恢复不确定均进入 `Unsafe`。
5. manager 普通 RVA `0x005BB5C3` 和 caller 三个普通 return 只负责正常关闭；任一 SEH unwind、callback 非局部退出、stale TLS epoch 或 wrapper 未见匹配 normal-exit 都退休 generation，禁止自动重试。

必须使 generation 失效的事件集合：

- 三个 slot writer 的 pre-store：`INITIAL_PUBLISH_OR_REPLACE`、`ROOT_RECREATE_OR_REPLACE`、`ROOT_RETIRE`；
- T0-04 已有 manager change、simulation thread change、`lua_close`、run teardown、menu retire、room/owner lifecycle generic clear；
- InputManager vptr 从 derived 漂移、device/control/release callback 重入、重复进入 caller/manager、writer 嵌套；
- caller、manager、spawn、InputManager query 的 SEH unwind 或其他 nonlocal exit；
- 任一 slot／owner／vector／device 双读漂移、读取失败、generation overflow、事件 ring publish failure、restore/drain/terminal 不确定。

## 6. 与 T0-04 ownerGeneration 的绑定判断

T0-04 `ownerGeneration` 可以继承其单调、tombstone、队列／planner／snapshot 清理、fail-closed 与 room/menu/lua 生命周期语义，并作为 `ManagerEpochIdentity` 的父 epoch。它不能单独证明 manager-this 同代身份：现有契约没有证明在三个 GameRoot writer 的 pre-store 推进该 generation，也没有关闭 slot 的计算别名 writer，更不能捕获两个采样点之间 A→B→A。

可接受的实现有两种：

1. 保留 T0-04 `ownerGeneration`，新增独立 `gameRootGeneration`，身份使用二元 tuple；推荐。
2. 扩展同一 generation 的事件域，使三个 writer 与全部补齐的 alias/lifecycle writer 都在 pre-store 推进；这必须新建 schema/catalog/验证，不能声称旧 T0-04 generation 已经具备该语义。

因此当前字段必须保持：

```text
directWriterUniverseComplete = true
aliasWriterUniverseComplete = false
readerImplemented = false
nativeAbaExclusionProven = false
managerThisIdentityStatus = UNKNOWN
```

## 7. 四个 frontier 的本轮判定

| frontier | 本轮判定 | 精确剩余条件 |
| --- | --- | --- |
| `CALLER_PRECEDING_CALLEE_PRESERVATION` | `PARTIAL_STATIC_CLOSURE` | 6/7 间接调用已收窄，roster call 在 canonical-zero 下可剪枝；InputManager control/device/global-release callback 的重入、nonlocal exit 和全部 transitive effect 未闭合 |
| `GAMEROOT_SLOT_ALIAS_WRITER_CLOSURE` | `DIRECT_ONLY_STATIC_CONFIRMED` | 三个 direct writer 与同步 CFG 顺序已闭合；计算地址、间接 store、外部 callback/lifecycle alias universe 仍 `UNKNOWN` |
| `SAME_LIVE_GAMEROOT_GENERATION_ABA` | `SPECIFIED_NOT_IMPLEMENTED` | 必须实现 adapter-owned `gameRootGeneration` 并绑定全部 writer；禁止端点相等证明 |
| `LIFECYCLE_AND_EXCEPTION_PRESERVATION` | `PARTIAL_STATIC_CLOSURE` | 三 writer lifecycle 含义、caller/manager/spawn SEH/normal-exit边界已登记；真实 unwind、callback reentry、publish failure、restore/drain/terminal 仍需 production reader 和独立测试 |

本交接可作为 production reader 的 create-new 输入，但不能把 `managerThisIdentityStatus` 从 `UNKNOWN` 提升为 `STATIC_CONFIRMED`，也不能发布 plan v11/progress v5。
