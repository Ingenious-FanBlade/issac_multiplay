> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 Virtual Device 自有 ABI 补充静态确认 v4（IDA-MCP）

日期：2026-10-12
推进：`tx-00-09`（整体推进；R01–R07 为一个内容编号集合，只在 §10 做一次整体静态文档审阅）
状态：已完成 R01–R07 与 §10 唯一一次整体静态文档审阅（七项检查全部通过，第 5 项带明确缺口标注；`T0-05-05` 本身未完成）
证据：[静态确认证据 v4](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v4.md)
依据：[下一步工作设计 v3](virtual-device-owned-abi-static-confirmation-next-work-design-v3.md)（本文记为 `tx-00-08`）

先读[现行项目规范](../../../../../project-conventions.md)，用户会话指示优先。本工作不设 gate、不校验任何哈希，不以旧 decision、解锁字段、矩阵完成率或作者身份作为推进前置条件；静态分析、整理和一次整体审阅由同一智能体完成。

**本轮实际工具**：IDA Pro MCP（`ida-pro-mcp` 1.0.0，streamable-http `127.0.0.1:13337`，66 个工具，Hex-Rays 可用）只读调用 `decompile` / `disasm` / `xrefs_to` / `get_bytes` / `get_int` / `stack_frame` / `basic_blocks` / `callees` / `py_eval` / `py_exec_file`（纯读）。未 patch、未 rename、未 set_type、未 define_func、未保存 IDB；未运行游戏、调试器、测试、构建、harness、probe、smoke、canary 或任何 verifier。
**input / IDB**：`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe` / 同名 `.i64`；imagebase `0x400000`；PE32 i386（Machine `0x14C`）、5 节、`SizeOfImage 0x8FE000`、TimeDateStamp `0x69E6E3A7`。地址一律写 IDB VA（= RVA + `0x400000`），不填运行时 VA。

---

## 1. 执行范围与目标

把 v3 纠正为事实、条件、设计建议和未决项彼此一致的 v4：

1. **纠正** Q1 分配器栈偏移、Q2 容量条件不闭合、Q3 计数算法不完整、Q4 EH 状态映射未完成、Q5 资源 lifetime 仍带条件、Q6 状态与审阅矛盾、Q7 字段/边界笔误、Q8 把静态完成与实际实现混用。
2. **保留** v3 已支持的注册方向、实际复制宽度、整数回绕、缓冲失效和并发反例；不因「七项有章节」推出「全部 ABI 已验证完整」。
3. 对当前有关的 caller 和资源给出具体范围；无法隔离的 receiver、计算地址及外部模块如实保留 `UNKNOWN`，继续不依赖这些结果的工作。
4. 不恢复全映像 native alias universe、独立 factory/final-release-thread、Batch 2 retry 或未来模块闭包流程。

本文不采纳任何候选契约修订；`DESIGN_ONLY` 建议集中在 §9，不覆盖[契约 v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)。

---

## 2. Q1–Q8 逐条修正表

| Q | v3 原位置与原结论 | 本轮直接依据（IDB VA + 原始字节） | v4 处理 | 影响 |
| --- | --- | --- | --- | --- |
| **Q1** 分配器栈偏移 | 证据 v3 §K02-1 第 77 行：「ABI：`mode=CL`、`a2=EDX`、64-bit size 在栈 `[ebp+0x10]`（低）/`[ebp+0x14]`（高）」 | `0xA64910: 8B 5D 0C` = `mov ebx,[ebp+0xC]`（hi）；`0xA64917: 8B 7D 08` = `mov edi,[ebp+8]`（lo）。`stack_frame` 列 `__saved_registers@0x8`、`__return_address@0xC`、`a3@0x10`(8B)，而返回地址物理上在 `[ebp+4]` ⇒ 该函数 **frame 偏移 = 实际 EBP 位移 + 8** | **修正**为 `[ebp+8]`（低）/`[ebp+0xC]`（高）；写明 IDA frame 偏移与实际 EBP 位移的差别 | R01；O-ALLOC、O-CTOR；F1 |
| **Q2** 容量不闭合 | 主报告 §5.1、§10 D1；证据 K03-2 的统一条件是「`n>=32` + 32 个已初始化 16B 元素 + 两份 32-float 缓冲」 | `common sub_A1F280` 在 `0xA1F2A9..B5` 与 `0xA1F2E6..F2` **各自**按当时的 `n_bind=(end-begin)>>4` 现算 `4n` 做 `memcpy`/`memset`；`n_bind=33` 时请求 132 字节，两份 32-float 缓冲仅 128 字节 | **修正**：固定方案必须是 `n_bind==32` 且「两份缓冲各覆盖至少 `n_bind`」；一般方案必须按实际 `n_bind` 覆盖存储与初始化。`n>=32`＋仅 32 项**不充分** | R02；O-BUF、O-ABI-QUERY、O-JOIN、O-ACTION；F4–F6；D1 |
| **Q3** 计数算法不完整 | 主报告 §8.2；证据 K06-3/4：候选 A1 未定义 bounded CAS 失败、隐含 weak、32-bit shadow 溢出、PINNED 后操作与返回值 | 见 §7（R05 六槽基线、control block 布局、候选操作表与有限交错） | **重写**为逐槽操作层，并明确「候选尚不完整」的具体条款 | R05；O-CB、O-RC-INLINE、O-THREAD；F5；D2、D6 |
| **Q4** EH 未完成状态映射 | 主报告 §7、§11；证据 K05：4 张 FuncInfo 已索引，但 state 与指令、旧/新 buffer、部分构造、锁及回调副作用未全部对应 | 见 §5（R03：FuncInfo 原始表、state→指令→资源→锁→出口→发布表） | **逐 state 展开**，未解析处按地址标注 `UNKNOWN` | R03；O-INSERT-ABI、O-REMOVE、O-NOTIFY；F6、F7；D4 |
| **Q5** 资源 lifetime 仍带条件 | 主报告 §6、§11；证据 K04：Init/cleanup/slot5 的间接 receiver 及部分派生/funclet 路径未展开；`NEVER` 不自动维持缓冲/vector/vptr 有效 | 见 §6（R04：caller/receiver 表、资源生命周期表、槽状态表、直接旁路） | **展开**到当前可隔离范围，其余列具体 `UNKNOWN`；`NEVER` 与「缓冲/vector/vptr 有效」明确分开 | R04；O-CTOR、O-BUF、O-THREAD；F4、F5；L2、L3；D3 |
| **Q6** 状态与审阅矛盾 | 证据 v3 第 4 行仍写「状态：进行中（K01/K02 已记录；K03–K07 待补）」，主报告 §12 第 7 项却判「主报告与证据 v3 状态一致 … 通过」 | 直接读证据 v3 第 4 行与主报告 §12.7 原文 | **保留历史**，在 §10 明确：v3 §12.7 的「通过」不支持「完整结论」，该条本身是错误判定；v4 主报告与证据 v4 两处状态一致 | R07；文档一致性 |
| **Q7** 字段与边界笔误 | 主报告 §3.1：「`+0xB00/+0xC4`（状态缓冲）」；§5.1 UI 行：「`n≥action`」 | **Base 布局是 `[0,0x370)`**：Base deleting dtor `0xA1E1CF: 68 70 03 00 00` + `0xA1E1D5: E8` ⇒ `operator delete(this, 0x370)`（R04 复核：Net `0x3F8`、Gamepad `0x4F0`，本映像最大的设备对象也只有 `0x4F0`）；`+0xB00` 落在任何已观察设备对象之外。`sub_A243C0` 的实际比较是 `index < ((end-begin)>>4)`（`0xA243CA..D4`），`>=` 时只日志后继续读 | **修正**为 `+0xC0`/`+0xC4` 与 `0<=action<n`；并把 **device 每 action 4 个 primitive** 与 **record 每 action 2 个 primitive** 分列（record 无额外 key 字段） | R02、R07；O-BUF、O-ABI-QUERY；F4 |
| **Q8** 静态完成与实际实现混用 | 契约 v2 的「实现状态」、生产工程与离线原型被当作同一件事 | 生产工程 `IsaacCsT0.Native.vcxproj` 的 `ClCompile`/`ClInclude` 列表**不含** `src\virtual_device_lifecycle.cpp` / `include\isaac_cs_t0\virtual_device_lifecycle.hpp`；只有 `IsaacCsT0.VirtualDeviceLifecycleTests.vcxproj` 第 51–55 行包含 `tests\virtual_device_lifecycle_main.cpp` + `src\virtual_device_lifecycle.cpp` + 头文件。原型 `FinalRelease` 要求 port 报告 `objectDeleted && implicitWeakReleased && controlBlockFreed`，否则 `PortContractViolation`，成功后置 `LifecycleState::Destroyed` | **区分四类**：静态证据收尾 / 可执行契约 / 真实自有设备生产实现 / 最小 Join 与退休恢复接续；原型只在独立测试工程内、不实现 `NEVER`，不能作为真实设备接入依据 | R06；全部 O-*、F*、L*；§8.4 |
| **Q9（本轮自纠，v3 原结论保留）** 通知调用点 | 证据 v3 §K01-1 写「manager vtable `+0x78` handler `sub_A211D0`」 | 本报告一度按 vtable 起点 `0xB827F0` 对齐，误得 `+0x78 = 0xA20FB0`。改用 RTTI 复核后：`0xB827F0` 处的 dword 是 **COL 指针 `0xBB033C`**（TypeDescriptor `.?AVManagerBase@Input@KAGE@@`），**manager vtable 起点是 `0xB827F4`**（`sub_A1FC00` 的 xref `0xB82804` = `+0x10` = slot4）。按 `0xB827F4`：slot27=insert `0xA20DF0`、slot28=remove `0xA20EB0`、**slot30 = `0xA211D0`**。另在 `0xA20E85`/`0xA20E92` 发现第二条通知路径：全局回调指针 `n79_0 @ 0xC7E814` 被按值 pair 调用 | **撤回上一条错误对齐**；v3 原结论成立，不修正它。新增：`sub_A211D0`(slot30) 与 `sub_A212C0`(slot31) 内部分派到的回调目标、`n79_0` 的赋值点列 `UNKNOWN` | R02、R03；O-NOTIFY；F6 |

---

## 3. R01：分配器 ABI、栈位移与 Init 失败

**原始字节与栈图**见证据 v4 §R01-1..§R01-3。结论：

- `sub_A648B0` 的 ABI 是 `__fastcall(char op /*CL*/, void* ptr /*EDX*/, unsigned __int64 size /*[ebp+8]=lo, [ebp+0xC]=hi*/)`，裸 `retn`。
- 三个 wrapper 分清：`sub_A0F4C0`（size 在 `[ebp+8]`）、`sub_A0F4E0`（size 在 **ECX**）、Init 在 `0xA1E23F`/`0xA1E265` 的**直接**调用。三者都以 `op=0`、`ptr=0`、`hi=0` 进入；wrapper 自己 `add esp,8` 清栈；**Init 自己 `retn 4`**（`0xA1E2AE: C2 04 00`）。
- 四个概念分列（证据 v4 表）：**请求大小**、**malloc 实参（只有低 32 位）**、**4 字节 header `[ptr]`**、**payload 可用范围**。它们只在 `hi==0 且 lo<=0xFFFFFFFB` 时一致。
  - `lo=0xFFFFFFFF ⇒ malloc(3)`、header 写 3、payload 可用 0 字节；`lo∈{0xFFFFFFFC..0xFFFFFFFF}` 时 `mov [eax],edi`（`0xA64954`）写 4 字节**超出请求长度**；`eax+4` 不能提供请求的 payload。不推断现场一定崩溃、也不推断 allocator 实际块大小。
  - `hi!=0 ⇒ size 槽被整块清零 ⇒ malloc(4)`、header=4、payload 可用 0 字节。这不是「4GiB 失败路径」。
  - **可静态判定的记账缺陷**：分配侧 64 位统计累加 `(edi,ebx)`（含 `lo+4` 的进位，`add`/`adc`），free 侧只减 4 字节 header（`0xA648F3` + `0xA64900`）⇒ `lo>=0xFFFFFFFC` 的分配让统计**永久多出 2^32**。
- `0xA6491D: 76 0E`（`jbe`）在 32 位下对 `edi` 恒真，所以 `hi==0` 永不进入 `0xA6491F` 清零分支。
- **OOM 出口是「诊断/回调 + 返回 NULL」，不是抛出**：`0xA64941` push `0x7FCB9DD6` → `sub_A23200` 遍历 `0xC37974` 的 handler 链并逐个回调（`0xA23213..1A`），随后 `xor eax,eax; retn`。`0x7FCB9DD6` 是**通用 OOM 诊断码**（同码至少出现在 `0x730C8D`、`0x828EB4`）。链本身由静态初始化 `sub_406740` 用**同一个** `sub_A0F4C0` 分配节点建立，静态映像中链头为 0。只得出「诊断/回调副作用」这一已支持结论，不推断 handler 一定终止进程。
- **Init 整数与失败**：`n=(end-begin)>>4`（`sar`，有符号）→ `mul edi(4)` → `seto cl` → `neg` → `or` 饱和；溢出时 allocator 得到 `lo=0xFFFFFFFF`（⇒ `malloc(3)`），但 `memset` 用**重新计算且未饱和**的 `4n`。函数内共 **4 次独立长度快照**（两次分配、两次 memset），无锁。
  - 分配失败时返回值被写成 0，函数**没有空指针检查**且恒返回 `AL=1` ⇒ `n>0` 时函数内 `memset(NULL,0,4n)` 崩溃，调用方的返回后检查无法挽救。
  - `n==0` 时两次 `malloc(4)` **各自**可能失败；零长度 `memset` 的现场行为与 handler 非局部退出保留条件。
  - Init **先调用 slot5**（`0xA1E21C: call [eax+0x14]`）再算长度，因此 Init 长度与后续 `common` 长度之间天然存在一次写入窗口。
- 因分配器是原生 `operator new` 语义，「自有分配或初始化」若改用别的分配方式，会同时改变契约 v2 「原生 base ctor 与原生 Init 写入」的前提：本轮只列候选，不采纳。

## 4. R02：统一逻辑 n、实际存储与初始化

**五个量必须先分开**（证据 v4 §R02-1）：`n_bind`（binding 元素数）、`cap_bind`（容量）、`n_init_bind`（真实构造并初始化的元素数）、`n_prev`/`n_cur`（两块 float 缓冲的 float 数）、每次访问现算的下标上界。

- **唯一安全条件**（所有消费者共用）：
  ```text
  0 <= action < n_bind                        （UI、slot19、注册的边界比较）
  32 个 16B binding 元素真实存在且已初始化       （注册 helper / 同名写回固定访问 action 0..31）
  n_prev >= n_bind  且  n_cur >= n_bind        （common 的 memcpy/memset、query 12/13/14、slot21 的 cur/prev[action]）
  ```
  固定方案取 `n_bind == 32` 且 `cap_bind >= 32`，发布后禁止未协调的长度/容量变化；一般方案取 `n_bind >= 32` 且**实际存储与初始化按 `n_bind` 覆盖**。**不得**再把「`n>=32` + 仅 32 项元素/缓冲」写成充分条件（`n_bind=33` 反例）。
- **消费者逐条**（原始字节见证据 v4 §R02-3）：注册 helper 32 次越界**读**；同名写回 32 次越界**写**（`0x835EB7: 89 0C 30`，内存破坏）；`common` 两次独立长度快照；`slot21` 的循环有边界检查，但末尾 `cur[action]=prev[action]` **无条件**执行（`arg>=n_bind` 时越界）；UI `sub_A243C0` 在 `index>=n_bind` 时**只日志后继续读 16 字节**。`sub_A1ED50`(slot19) 用 `action<n_bind` 保护对 `+0xC0` 的写 ⇒ **入口 `action<n` 不证明内部缓冲有效**，条件成立依赖 `n_prev==n_bind`。
- **`common` 的真实到达路径是 manager update 的直接调用，槽覆写无法拦截**（证据 v4 §R02-6）：`InputManager` vtable `0xB827F4`（COL `0xB827F0`）的 slot4 = `sub_A1FC00` 先锁住 manager 自己的 Mutex（`+0x14` 处的 Mutex 对象），再从 `[this+8]` 遍历到 `[this+0xC]`（8 字节 `{object, control}` pair vector），对每个元素 `ecx = pair.object; call sub_A1F280`（`0xA1FC97: E8 E4 F5 FF FF`，**直接调用**）。因此只要自有设备被 insert 进 manager 的 pair vector，manager 每次 update 都会按**当时的 `n_bind`** 对 `+0xC0/+0xC4` 执行 `4*n_bind` 字节的 `memcpy`/`memset`。§R02-4 的容量条件因此是**注册进 vector 的前提**，不是可选优化。
- **primitive 数分列**：device binding 元素 16B = **4 个 dword**；record 每 action = **2 个 dword**（`+0x04..0x103` = 32×8B，无额外 key 字段）。同名写回只覆盖 dword0/1，dword2/3 保留。
- 通知调用点：`insert sub_A20DF0` 调 `[manager vtable+0x78]`（`0xA20E5C: 8B 70 78`）；manager vtable 起点是 `0xB827F4`，故 `+0x78` = slot30 = `0xA211D0`（v3 原结论成立）。另发现第二条通知路径：insert 末尾经全局回调指针 `n79_0 @ 0xC7E814` 用按值 pair 调用（`0xA20E85: A1 14 E8 C7 00` + `0xA20E92: FF D0`）。证据 v4 §R02-7/§R02-12。
- **writer 集合**（当前可隔离范围）：binding 元素 dword0/1 ← 同名写回；三指针 ← Base ctor（置空）、slot5（`reserve`+`memmove` 全局默认向量）、`sub_A1E760`（清 0 不释放）、Base dtor 的 `sub_4B3950`（析构释放）；`+0xC0/+0xC4` ← Init（分配）、cleanup（释放+清零）、Base ctor（置 0）；单元素 ← slot19、slot21、common 的 `memset`。默认模板来自全局 `0xC799E8/0xC799EC`，其运行期内容本轮未观察 ⇒ `n_bind` 实值 `UNKNOWN`。
- 笔误已按 §2 Q7 修正（`+0xC0/+0xC4`、`0<=action<n`）。

## 5. R03：EH state 与局部事务

主题证据：[证据 v4 · R03（EH 与局部事务）](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v4-r03-eh.md)（含 5 张 FuncInfo 的 raw 0x24 字节、`_UnwindMapEntry`/`_TryBlockMapEntry`/`_HandlerType` 原文、5 个 funclet/chunk 的归属、逐 state 证据表、契约四类可恢复范围结论（分册 §5）、关键地址索引与覆盖对照（分册 §6））。

### 5.1 state 索引方式与四条主事务的 FuncInfo（本轮直接读取）

- **本映像的 unwind map 直接按 raw state 索引**（`pUnwindMap[state]`，条目数 = `maxState`，`state∈0..maxState-1`，`-1`=无状态）。**不**是常见的「state-1 索引」：证据是 `sub_A21800`（`0xBB33D0`，maxState=1，U[0]=`__std_terminate`）在释放前写 `state=0`（`0xA21830: C7 45 FC 00 00 00 00`）、释放后写 `state=-1`（`0xA2185B: C7 45 FC FF FF FF FF`），`sub_A21570`（`0xBB33FC`）同形；grow 的 U[1] action `0xB11780` 读的 `[var_38,var_34)` 只在 raw state=1 的尾插复制区间被写（`0xA216E1`/`0xA21702`）。**按 state-1 读会让这些 terminate 永不可达**。
- 四张主 FuncInfo：`0xBD21A8`(insert, maxState=2, 无 try)、`0xBD21DC`(remove, maxState=3, 无 try)、`0xBD22F4`(grow, maxState=3, **nTryBlocks=1**，tryLow=0/tryHigh=1/catchHigh=2，`_HandlerType@0xBD2344`：adjectives=`0x40`、pType=0、handler=`0xA217CE`)、`0xBD2354`(migration `sub_A21880`, maxState=1)。四张表 `nIPMapEntries=0`、`pIPToStateMap=0`、`pESTypeList=0`、`EHFlags=0x1`。
- **insert 的 unwind map 里没有 manager unlock action**：U[0]=`0xB11650`（销毁按值 pair）、U[1]=`__std_terminate`；锁在 raw state=0 期间 `0xA20E27` 获取、`0xA20E52` 释放；grow 的 catch-all 只清理新 buffer 并 `_CxxThrowException(0,0)` 重抛。因此「已加锁、未到 `0xA20E52`」期间的 C++ unwind **锁残留**。
- 5 个 funclet/chunk：`0xB11650`(insert U[0])、`0xB11680`(remove U[0]，**与 insert 不同**) 、`0xB11780`(grow U[1])、`0xB117B0`(migration U[0])、`0xB116F0`(notification 三函数共用 U[0])。

### 5.2 资源/锁/发布/副作用（逐 state 表的结论）

| 事务 | 发布点 | 锁 | 异常时的清理 | 不回滚的副作用 |
| --- | --- | --- | --- | --- |
| insert append 分支 | `0xA20E3D: 83 47 0C 08`（end+=8） | `0xA20E27` 获取、`0xA20E52` 释放 | U[0] 只销毁按值 arg pair | `sub_902D60` 返回 false 时**不检查** → `{0,0}` 元素仍被 end+=8 发布 |
| insert grow 分支 | `sub_A21630` 尾插 → `0xA2179E` 三指针发布 | 同上（锁内） | 尾插：U[1] 销毁已复制前缀 → catch-all 销毁新元素并 free 新 buffer → 重抛 → insert U[0] | `sub_902D60` false → `{0,0}` 写进新 buffer 并最终发布（原元素 pair/strong 静默丢失） |
| insert notification | vector 元素已发布 | **已解锁** | `sub_A211D0` 遍历 `Manager+0x2C` list；迭代在抛出的 callback 处**停止** | record append / 同名写回（`0x95B290→sub_835C90`）无回滚；更早 callback 的副作用保留 |
| remove | 先通知、后 erase、再 `end-=8`（`0xA20F6B`） | **全程不取 manager `+0x14` 锁** | erase-shift 阶段（state=-1）**无 unwind action**；release 阶段 U[0]=`__std_terminate` | `[vtable+0x7C]`=`sub_A212C0`、`[vtable+0x80]`=`sub_A213B0` 在 keyed lookup **之前无条件**触发（key 不在 vector 时也触发）；list `+0x44` 的静态注册点未找到 |
| erase `sub_A21570` | 循环推进；最后元素的 pair 由**调用方**释放 | 无锁 | copy/swap 阶段无清理 ⇒ temp strong 泄漏、found 槽已被 next 副本覆盖、`end` 未减 | 半 erase，vector 可能重复/错位 |
| grow 旧元素销毁 | 三指针发布在旧元素销毁之后 | 锁内 | `sub_A21800` 的 U[0]=`__std_terminate` | **「未发布 = 旧 vector 必然未变」不成立**：terminate 时旧三指针仍指向 old buffer，而部分旧元素已释放 |
| length/alloc 异常 | — | 锁内 | 不经 catch-all；只由 insert U[0] 销毁 arg | `_Xlength_error`/`bad_array_new_length` 在 raw state=-1 抛出；`sub_40CF00` 在 size<0x1000 时返回 0 ⇒ grow 在 `NULL+offset` 写 ⇒ AV；size>=0x1000 ⇒ `_invalid_parameter_noinfo_noreturn`。**AV 是否被 `catch(...)` 覆盖：无依据**（`_CxxFrameHandler3` 在外部 CRT，import thunk `0xAF05C7`） |

### 5.3 契约 v2「insert 异常 → RETIRING → remove」的可恢复范围（分册 §5 的结论，压缩）

> 契约 v2 第 89 行把「insert 异常」统一映射为「进入 RETIRING，再走 remove 路径」。R03 的结论是**必须按异常发生的阶段拆分**，并把 terminate/noreturn/AV 单独归类；分册 §5.1–§5.3 给出完整依据与六条修订建议。以下为压缩版。

三个阶段：**阶段 A = 锁内**（`0xA20E27` 加锁 → `0xA20E52` 解锁，含 append 复制与 grow）；**阶段 B = 锁外 notification**（`0xA20E52` 之后，`0xA20E6A` 起）；**终止路径**（`__std_terminate`、`_invalid_parameter_noinfo_noreturn`、未证明被 `catch(...)` 覆盖的 AV）。

| 类别 | 阶段 A（锁内异常） | 阶段 B（锁外 notification 异常） | 终止/noreturn/AV |
| --- | --- | --- | --- |
| **锁残留** | **有**：`0xBD21A8` U[0] 只销毁按值 pair、U[1]=`__std_terminate`，无 unlock action。按调用形态推导 Manager+`0x14` 是递归 `KAGE::Mutex`（同线程可再进入、其他线程会阻塞）；若实现不同则降级 `UNKNOWN`。**不得**声称自动 unlock | **无**：unlock 已在 `0xA20E52` 完成 | 已终止，锁状态无意义；**不进入 RETIRING** |
| **成员身份未知** | C++ unwind 下 grow 未发布三指针、old vector 未改，keyed lookup 应返回不存在。**但普通失败例外**：append 的 `TryAddStrong=false` 无异常、`end+=8` 仍执行、vector 出现 `{0,0}`；此时「lookup 未命中」**不能**证明「vector 无该设备痕迹」 | 成员**已发布**（`element.first=key`、`element.second` 持 strong），insert 异常不回滚 vector ⇒ 此时「RETIRING→remove」才有实际清理对象 | 无成员状态可恢复；不得 remove/重试 |
| **半迁移** | grow 非尾插：`sub_A21880` 自身 U[0] 销毁已构造 dest 范围，grow catch-all 再销毁前缀+新元素并 free 新 buffer，old vector 保持旧三指针 ⇒ **不会留下已发布的半迁移 manager vector**。erase-shift 的 copy/swap 阶段（state=-1）抛异常会留下部分搬移/重复元素与未回滚的 temp pair ⇒ **半 erase，不可重试** | 无半迁移；但回调内部的 record/name/binding 可能部分完成（如 name 已分配而 record 未 append） | 旧元素销毁中途异常走 `sub_A21800` U[0]=`__std_terminate`：旧三指针仍指向 old buffer、部分旧元素已释放而 buffer 未 free ⇒ **「未发布 = 旧 vector 必然未变」不成立**；进程终止 |
| **进程终止** | 不进入 RETIRING（noreturn / 未证明被 catch 的 AV / unwind 清理中触发的 terminate） | 若 callback 的析构/unwind action 触发 terminate（`sub_A211D0` U[1]、`0x95B290` U[1] 等）同样终止，不能继续 remove | 终止即终态；记录 fail-closed；不强制卸载/重试 |

六条修订建议（分册 §5.3）：(1) 阶段 A 的 C++ unwind 只能靠 keyed lookup 复核，未确认「本对象且唯一」时**不要**用 remove 当清理（remove 会先无条件发通知，可能误删同 key 的其他/重复元素），保留 root、记录锁残留、进入 fail-closed；(2) 阶段 A 的 `TryAddStrong=false` 已是正常返回，不得盲目 remove；(3) 阶段 B 可以进入 RETIRING 并调 remove（同线程，remove 不取 manager 锁），但 remove 自身有半 erase 风险，且回调副作用不在其清理范围；(4) 半迁移/半 erase 不得重试、不得强删元素、不得靠重新 insert 覆盖；(5) 终止/AV/noreturn 不进入 RETIRING、不 remove、不重试、不强制卸载；(6) 契约应新增「insert 锁内异常 = 锁残留 UNKNOWN」与「notification 副作用（name heap、record append、同名 binding 写回）在 manager pair vector 之外，需从可恢复范围中扣除」两条。

补充（与 R02/R04 的交叉）：record append / 同名写回 / name 堆分配都发生在 notification 回调内，**不属于** manager pair vector 的清理范围；`sub_835C90` 的 U[0]（`0xB02340`）只销毁 lookup pair，name 堆不回收、record append 成功后 persist。**不得**把「remove 成功」当作「record/binding 副作用已回滚」。

## 6. R04：基类资源与破坏性调用的 receiver

主题证据：[证据 v4 · R04（资源与 receiver）](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v4-r04-lifetime.md)。本轮做了 `.text` 全段 `call [reg+disp]`（`disp∈{4,8,0x14}` 与 `{0x4C,0x50,0x54,0x78,0x88}`）的有界普查：1,372 命中 / 405 函数 + 25 命中。

### 6.1 vtable / 对象大小 / 直接 caller（本轮直接事实）

- 5 张表的 RTTI 身份见 §8.2；4 张设备表逐槽 38 槽与 v3 种子一致。Keyboard 表起点 `0xBA1678`，其 slot5 是自身实现 `0xA6C480`（模板全局 `0xC799F8/0xC799FC`）。
- **对象分配大小**：Base/Keyboard `0x370`、Net `0x3F8`、Gamepad `0x4F0`。设备对象**没有内嵌 CS/Mutex**（`sub_A2C980` 只清 0x280 字节键状态表）。
- **Base ctor `0xA1E050`**：4 个直呼点全是派生类构造（Net×2、Keyboard 工厂 `sub_A21980`、Gamepad `sub_A21D40`）；**不调用 Init、不分配绑定向量/双缓冲、不触碰 control block**。
- **Init `0xA1E210`**：2 个直呼点（Gamepad slot1 `0xA22420`、Keyboard slot1 `0xA6C42B`）；不检查 `+8` bit1、不清 bit1、不析构旧双缓冲 ⇒ **重复 Init 会泄漏旧缓冲并可能改变 `n_bind`**。
- **cleanup `0xA1E2C0`**：8 个 code caller 全在派生 dtor/EH 路径；Gamepad 与 Keyboard 的 dtor **各调 cleanup 两次**；cleanup 只释放 `+0xC0/+0xC4`，**不析构 binding vector、不改 `+8`**。
- **slot5 `0xA1E430` 与 Base deleting dtor `0xA1E1B0` 的全映像 code xref 都是 0**（各只有 vtable 数据引用）；`common sub_A1F280` 的全映像 code xref 只有 1 条 = manager update `0xA1FC97`。
- **Init 体内虚调 slot5**（`0xA1E21A: 8B 06` / `0xA1E21C: FF 50 14`）⇒ 若先写自有 vptr 再调原生 Init，`n_bind` 取决于**自有 slot5**；若自有 slot5 是 no-op 且未预填 `+0x10/+0x14`，则 `n=0`。这是必须写进契约的创建顺序约束。

### 6.2 资源生命周期（分册 §2 的结论）

| 资源 | Base ctor | Init | common | cleanup | slot5 | vector dtor `sub_4B3950` | Base dtor |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 对象内存 | 不分配/不释放 | 不触碰 | 不触碰 | 不释放 | 不触碰 | 不释放 | `arg&1` 时 `free(this,0x370)` |
| control block | 不触碰 | 不触碰 | 不触碰 | 不触碰 | 不触碰 | 不触碰 | 不触碰 |
| vptr | 写 `0xB82758` | 不写 | 不写 | 不写 | 不写 | 不写 | 写回 `0xB82758` |
| binding 三指针 | 清零 | 只读算 `n` | 只读算 `4n` 作 memcpy/memset 长度 | **不析构、不改** | reserve+memmove 覆盖、改 end | `begin` 非空则释放 + 三指针清零 | 调 cleanup 后 `sub_4B3950` |
| `+0xC0/+0xC4` | 清零 | 各 `malloc(4n)`+`memset` | `memcpy(+0xC4←+0xC0,4n)` + `memset(+0xC0,4n)` | 非空则 `free` 并置 0（两块独立） | 不触碰 | 不触碰 | 经 cleanup |
| snapshot masks `+0x20..+0x3C` | 8 dword 清零 | 不触碰 | 前移并清 current | 不触碰 | 不触碰 | 不触碰 | 不触碰 |
| 原生 CS/Mutex | **设备对象无内嵌 CS/Mutex** | 无 | 无 | 无 | 无 | 无 | 无（control 的 CS 在 control 自身） |

### 6.3 直接旁路（不能靠槽覆写拦截）

1. **manager update `sub_A1FC00` 对 vector 内每个 device 直呼 `common`**（`0xA1FC95/0xA1FC97`）——每帧按当时 `n_bind` 写 `+0xC0/+0xC4`。
2. 注册路径（`sub_835C90`/`sub_835BA0`）直读写 `device+0x10` binding vector。
3. UI `sub_A243C0` 直读裸 binding vector（`this` 是 vector 指针本身，不是 device）。
4. Init 体内的 slot5 虚调（`0xA1E21C`）。

### 6.4 状态与中性化（R04 的关键限制）

> 校正：R04 分册 §3.1 称「`common` 不调用 slot30(+0x78)」与原始字节不符。`0xA1F307: 8B 40 78` / `0xA1F30A: FF D0` / `0xA1F30C: 84 C0` / `0xA1F30E: 75 06` 证明 `common` **先用 slot30 作布尔门控**；分册提到的 `+8` bit3 门控（`0xA1F53B: F6 46 08 08`）与 `call [eax+0x84]`（slot33，`0xA1F711: FF 90 84 00 00 00`）是同一函数内**后续**阶段的门控。

- 原生**没有** RETIRING/UNREGISTERED/ZOMBIE 的字段；`+8` 的 bit1 是唯一观测到的“槽有效性门”（ctor 尾部 `bts ax,1` 置位），且扫描范围内**无清除点**。
- 因此「字段中性 ⇒ 继承槽中性」**不成立**：门控 bit1 的槽（7/8/10/11/12/13/14/15/24）在退役后仍会执行并读 `+0xC0/+0xC4`；不门控的槽（16–23、30、34 与 `common`）本来就无退役概念，其中 slot19/20/21/34/`common` 还会**写**自有状态。
- 「清 bit1 以中性化」是自有新增行为，需契约授权，且只覆盖上述 9 个槽，不覆盖 16–23、30、34 与 `common`。
- 结论：**L3 不能由「对象不释放」或「字段中性」推出**；必须逐槽覆写或保证缓冲有效并接受 native 推进语义。

## 7. R05：完整计数同步候选

主题证据：[证据 v4 · R05（计数同步）](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v4-r05-counting.md)。

### 7.1 原生六槽基线（本轮原始字节）

- **control block vtable 身份由 RTTI 确认**：`0xB78014` 起 6 槽，`[-1]` = `0xB78010` → `0xBAEF64` → `.?AV?$ReferenceCount@VInputDeviceBase@Input@KAGE@@@@`；typed 变体 `0xB82918`(Keyboard)/`0xB82934`(Gamepad) 六槽内容逐项相同。**六个槽函数只有 vtable 数据 xref，无 code xref**。
- **六个原生槽都不含原子指令**（六段字节内 `F0`/`0F C1`/`0F B0`/`0F B1` 零命中、无 `xchg`）；串行化完全靠 `ctrl+8` 的内嵌 `KAGE::Mutex`（vtable `0xB81C0C`；lock=`0xA157F0`，unlock=`0xA159A0`）。该 Mutex **非可重入**（`lock` 在 `[CS+0x18]` 上 `Sleep(1000)` 自旋），且 `KAGE::Mutex::ctor` 在 CS 分配失败时仍置 bit0 并令 `[mutex+8]=0`。
- 逐槽（`this` = ECX；`w4=word[+4]`、`w6=word[+6]`）：

| 槽 | 地址 | 次序摘要 | 返回 |
| --- | --- | --- | --- |
| 0 dtor | `0x40CB50` | 写 vptr、`DeleteCriticalSection([+0x10])` + `operator delete(0x1C)`、`and byte[+0xC],0xFE`、`[+8]=0xBA04B4`、flag&1 时 `operator delete(this,0x18)` | `EAX=this`；`retn 4` |
| 1 TryAddStrong | `0x40C6B0` | lock → 读 `w4` → unlock →（`w4!=0`）再经 `ctrl->vptr+8`（AddStrong 重新 lock/inc/unlock） | `AL`=0/1 |
| 2 AddStrong | `0x40C690` | lock → `inc word[+4]` → 尾跳 unlock | EAX = unlock 的返回（无 bool 语义） |
| 3 ReleaseStrong | `0x4358B0` | lock → `w4==0` ⇒ unlock/`AL=1`；否则 `--w4`；非 0 ⇒ unlock/`AL=1`；变 0 ⇒ **unlock** → `obj=[+0x14]`，`obj->vptr[0](../../../07-virtual-device/design-revision/obj,1)`（**删除型析构**）→ `[+0x14]=0` → **尾调 `ctrl->vptr+0x14`**（ReleaseWeak） | `AL`=1 或尾调结果 |
| 4 AddWeak | `0x40C610` | lock → `inc word[+6]` → 尾跳 unlock | 同槽 2 |
| 5 ReleaseWeak | `0x40C5C0` | lock → `w6==0` ⇒ unlock/`AL=1`；否则 `--w6`；非 0 ⇒ unlock/`AL=1`；变 0 ⇒ unlock → `ctrl->vptr[0](../../../07-virtual-device/design-revision/ctrl,1)`（**释放 control**） | 最后 weak ⇒ `AL=0` |

- 原生缺陷（可构造）：`TryAddStrong` 的「读 `w4`」与「调 `vptr+8` 加一」之间**解锁**，与最后 Release 竞争 ⇒ 可在对象/control 已析构后把 `S` 复活为 1 而 `[+0x14]` 仍为 0（check-then-act）。
- 返回值消费（已隔离范围）：`sub_902D60` pair copy **消费 `AL`** —— Try `AL==0` ⇒ 输出 `{0,0}` 静默返回；Release `AL!=0` ⇒ 若全局 `n79_0 @ 0xC7E814` 非空则 `n79_0(out_pair)`。`AddStrong`/`AddWeak` 的 EAX 在已隔离族中无消费者。范围外 caller 未闭合。

### 7.2 候选 A1（`DESIGN_ONLY`）与它是否完整

候选数据模型：原生 `[0,0x18)` 不变；扩展区放 `shadow_strong`(u32)、`shadow_weak`(u32)、`flags/pinned`、`self_lock`、`state`(`zombie`/`implicit_weak_done`/`weak_zero`/`retire_seq`)、固定 4 项环形诊断。不变量 `mirror = (min(W,0xFFFF)<<16) | min(S,0xFFFF)`，在释放锁前用**单次 32 位 store** 发布。7 行操作的完整次序与返回分支见分册 §3.2；要点：

- `TryAddStrong`：pinned / `S==0` / `S==0xFFFFFFFF` ⇒ `AL=0`（不假成功）；否则锁内 `S+=1`，`S>0xFFFF` 置 overflow、镜像饱和。
- `AddStrong`/`AddWeak`：0 基 ⇒ no-op + `zero_add` + PINNED（**不复活**）；`0xFFFFFFFF` ⇒ 不加；正常 32 位增长、镜像 u16 饱和。
- `ReleaseStrong`：`S==0` ⇒ underflow + `AL=1`（不下溢、不动 weak）；否则 `--S`；变 0 ⇒ 清 `[+0x14]=0`、置 `zombie`（恰一次）、**锁内合并**隐含 weak（`implicit_weak_done` 守卫，恰一次）、返回 weak 结果。
- `ReleaseWeak`：`W==0` ⇒ underflow + `AL=1`；变 0 ⇒ `weak_zero`、`AL=0`，**不调用 ctrl dtor**。
- `PINNED` 粘滞，不冻结计数（除 0 底与 u32 顶）。
- **选择锁内合并 helper 而非「解锁后调用 weak 槽」**：后者会出现 `S=0,O=0,W=1` 窗口（交错 I8），导致另一线程误判「最后 weak」、伪 PINNED 与 `AL` 语义偏差；合并变体把三处状态放进同一临界区，也避免递归获取同一非重入锁。

**完整性判定（分册 §3.3、§6.1）**：

- 主候选用**无界 CAS 自旋** ⇒ 不存在「bounded CAS 达到上限」分支，因此不丢弃 Release、不假装 Try 成功、不需要无锁覆盖 shadow/mirror。但「持有者被长期挂起时其他槽自旋等待」与契约「不得阻塞等待主线程」之间的关系需要契约解释（活性解释缺口）。
- **有界变体（bounded CAS）没有满足契约的失败处置**：达到上限后，`ReleaseStrong`/`ReleaseWeak` 的每条可选路（丢弃 / 无锁减 shadow / 继续等待 / 终止）都违反契约 §5 或 R05 的失败约束 ⇒ 该变体**候选算法尚不完整**，需要契约修订或接受无界自旋。

### 7.3 三个必须分开的结论

1. **打包 store 的对齐依据不足**：IDB 只能证明 `ctrl+4 = malloc()+8`（三条安装点都先 `sub_A0F4C0(0x18)`）；`malloc` 的 8 字节对齐是 CRT 外部契约 ⇒ `UNKNOWN`。候选必须 `lock` 前缀 store，或运行时检查 `(ctrl+4)&3==0` 并 fail-closed。
2. **单次打包 store 不给两个独立 u16 load 同一快照**：native reader 先 `movzx word[+4]` 再 `movzx word[+6]`，writer 在中间 store，可组合出从未原子存在的 `{strong=old, weak=new}`（交错 I9）。该 store 也不能推出它与 shadow/object 的整体一致（三者在不同存储位置）。
3. **A0 反例只否定 A0**：I1 只否定「shadow 原子 + 无验证普通 mirror store」；不能作「所有纯原子算法不可能」的全称结论。A2（自有有序/版本化发布）是未证明的反例候选。

有限交错 I1–I9（含双 Release、Try+最后 Release、镜像饱和回落、shadow u32 上限、PINNED 后操作、隐含 weak 交错）见分册 §4，全部手工推导，未运行程序或模型。

## 8. R06：16 项 O-*、F1–F8、L1–L4 与实现边界

### 8.1 原 16 项 O-*（逐项分别报告，不整项无条件标完成）

| 义务 | 本轮处理 | 状态 |
| --- | --- | --- |
| O-ALLOC | §3（R01）：`sub_A648B0` 的 ABI/栈位移/header/payload/64 位统计/OOM 出口；`sub_A0F4C0`、`sub_A0F4E0` wrapper 与 Init 直调分列；`sub_A0F500` free wrapper | **本轮重读**；`IDA_STATIC_CONFIRMED`（原始字节）。对齐与 malloc(0) 现场行为仍 `UNKNOWN` |
| O-CTOR | §3（R01）+ §6（R04）：Base ctor `sub_A1E050` 的字段写入与「**不调用 Init**」；Init `sub_A1E210` 的整数/失败；cleanup `sub_A1E2C0` 与 Base dtor `sub_A1E1B0` 的直接 caller | **本轮重读** ctors/dtors 部分；Base ctor 的 key 计数器语义（`+0xC = dword_C7362C++`）本轮只读出现场值 |
| O-CB | §7（R05）+ 证据 §R02-10：control block 的字段布局（`+4/+6` 计数、`+8` Mutex、`+0xC` 标志、`+0x10` CS 指针、`+0x14` object）与 6 槽原始语义 | **本轮重读**；`IDA_STATIC_CONFIRMED` |
| O-RC-INLINE | §7（R05）的有限范围；不恢复全程序 points-to 闭包 | **部分**；新旁路按接收者分析，其余 `UNKNOWN` |
| O-INSERT-ABI | §5（R03）+ 证据 §R02-6/§R02-7：pair vector `{+8,+0xC,+0x10}`、manager 互斥量 `+0x14`、临界区只覆盖拷贝/增长、通知在 unlock 之后、通知调用点是 slot30 `0xA211D0`（vtable 起点 `0xB827F4`）。FuncInfo `0xBD21A8`（maxState=2，U[0]=`0xB11650` 销毁按值 pair、U[1]=`__std_terminate`，**无 unlock action**）；`sub_902D60` 返回 false **不检查** ⇒ `end+=8` 发布 `{0,0}` | **本轮重读** |
| O-REMOVE | §5（R03）：remove **全程不取 manager `+0x14` 锁**，且在 keyed lookup **之前无条件**调 `[vtable+0x7C]=sub_A212C0`(list `+0x34`) 与 `[vtable+0x80]=sub_A213B0`(list `+0x44`)；erase `sub_A21570`（FuncInfo `0xBB33FC`）的 copy/swap 阶段无 unwind action；最后元素由调用方在 release 阶段（U[0]=`__std_terminate`）释放 | **本轮重读** |
| O-NOTIFY | §5（R03）+ 证据 §R02-7：insert 的通知是 `[vtable+0x78]=sub_A211D0`（list `Manager+0x2C`），迭代在抛出的 callback 处停止；静态注册点：list `+0x2C` ← `0x952C36`（callback `0x95B290`）、list `+0x34` ← `0x906E70`（callback `0x913760`）；list `+0x3C`(`0xC57B54`)/`+0x44`(`0xC57B5C`) 无静态注册点 ⇒ 目标 `UNKNOWN` | **本轮重读**；`n79_0 @ 0xC7E814` 目标 `UNKNOWN` |
| O-ABI-0C | §6（R04）：**base vtable `0xB82758` 的 slot3 是 `_purecall`**（`0xAF05D3`）；具体设备各自实现（`0x901470` 委托到 `[ecx+0x370]->vtable[1]`、`0xA224A0`、`0xA6C440`）⇒ 自有实现必须覆写为稳定非空名称 | **本轮新增直接依据** |
| O-ABI-78 | §6（R04）：**base slot30 也是 `_purecall`**；具体设备实现为 `0x709C30`（`mov al,1; retn`）、`0xA22940`、`0xA6C650`；`common` 把它当作**布尔门控**（原始字节 `0xA1F307: 8B 40 78` / `0xA1F30A: FF D0` / `0xA1F30C: 84 C0` / `0xA1F30E: 75 06`；AL==0 时 `common` 直接返回）；R04 分册 §3.1 曾称「common 不调用 slot30」，已按原始字节更正。契约把 `+0x78` 命名为「polll 锁存」与原生门控语义不同，见 §9 D5 | **本轮新增直接依据** |
| O-ABI-QUERY | §4 + §6：slot12 `sub_A1E8D0` = pressed（`action>=n_bind` → 0）、slot13 `sub_A1E970` = triggered（同时读 prev/cur）、slot14 `sub_A1EA20` = value（读 prev，返回 ST(0)）；三者入口都有 `(this[8]&2)` 状态位检查；越界返回 0/0.0 | **本轮重读** |
| O-BUF | §4：buffer 读者 = slot7/8/12/13/14/19/21 + Init/cleanup/ctor + `common`；`n_bind` 惯用式在 `.text` 全段命中 13 处 / 9 个函数（证据 §R02-9） | **本轮重读**并做全段扫描 |
| O-RTTI | 未在本轮重读，沿用 v2 §9 范围。**本轮新增 1 处直接依据**：`sub_835C90` 在 `0x835CF8` 调 `__RTDynamicCast(device,0,InputDeviceBase TD,NetInputDevice TD,0)`，非零则 `0x835D02: jnz` 跳过整条注册映射 | 继承 + 1 处新依据 |
| O-KEY | 未在本轮重读，沿用 v2 §7（计数器非原子、lookup 首匹配）。**本轮关联**：同名写回覆盖 binding dword0/1（§4）；key→pair 查找 `sub_A20390(&off_C57B18,&pair,key)` 是 manager vtable `0xB827F0` 的 slot11 | 继承 + 关联 |
| O-JOIN | §4：slot21 `sub_A1EE10` 的 action 参数、4 primitive 循环与**无条件** `cur[action]=prev[action]`；slot21 覆写只能覆盖这一条路径。Join 触发/选择链本轮未重读 | **部分** |
| O-ACTION | 未在本轮重读，沿用 v2 与 T0-04。**本轮新增 1 处直接依据**：`sub_A1E360`（slot4）是 action id→名称的 switch，`0xABAB..0xABAF`（跳转表 `0xA1E410` 5 项）、`0xABB0`、`0xABB1`、`0xABB2`、`0xABCD`、`0xCDCD`、`0xCDCE`、`0xFFFFFFFF` 有专名，其余落默认串 | 继承 + 1 处新依据 |
| O-THREAD | 证据 §R02-6 + §7（R05）：manager update（vtable `0xB827F4` slot4 = `sub_A1FC00`）在 manager 互斥量内遍历 pair vector，对每个设备**直接调用** `common`；`common` 内的 `4*n_bind` 访问因此发生在 manager 锁内 | **本轮新增直接依据** |

### 8.2 16 项以外本轮新增的直接事实

| 事实 | 依据 |
| --- | --- |
| 对象总大小 `0x370`（`operator delete(this,0x370)`） | `0xA1E1CF: 68 70 03 00 00` + `0xA1E1D5: E8` |
| control block 总大小 `0x18`，`CriticalSection` 另行分配 `0x1C` | `0x40CB95: 6A 18`、`0x40CB72: 6A 1C` |
| 4 张同类设备 vtable：`0xB82758`（Base，slot3/slot30 = `_purecall`）、`0xB78030`、`0xB82C80`、`0xBA1678`（Keyboard）；另有 manager vtable `0xB827F4` | `probe/list_funcs.py` 逐槽读取 + RTTI 复核 |
| `InputDeviceBase` 的 slot7/slot8 会读 `this+0x108` 起、stride 40 的内嵌表，计数在 `this+0x368` | `sub_A1E4B0`/`sub_A1E590` 反编译 |
| `common` 在 `0xA1F316` 起还会按门控结果用 slot31(`0xA1F720`)/slot32(`0x709C30`) 采样并把结果写进 `+0x30` 位域与 `+0x40..+0x4C` | `sub_A1F280` 反编译 |
| 5 张相关表的 RTTI 身份：manager=`0xB827F4`(`.?AVManagerBase@Input@KAGE@@`)、Base=`0xB82758`(`.?AVInputDeviceBase@Input@KAGE@@`)、Net=`0xB78030`(`.?AVNetInputDevice@IsaacRepentancePlus@@`)、Gamepad=`0xB82C80`(`.?AVInputDeviceGamepad@Input@KAGE@@`)、Keyboard=`0xBA1678`(`.?AVInputDeviceKeyboard@Input@KAGE@@`)；control 6 槽表 `0xB78014` = `.?AV?$ReferenceCount@VInputDeviceBase@Input@KAGE@@@@` | `probe/col.py` 读 `_RTTICompleteObjectLocator.pTypeDescriptor`（证据 §R02-12） |
| **原生 Init 自身会虚调 slot5**：`0xA1E21A: 8B 06; 0xA1E21C: FF 50 14`（`ecx=this`，无栈参），然后才用 `+0x10/+0x14` 算 `n` 并分配双缓冲 ⇒ 若已写入自有 vptr，原生 Init 分派到自有 slot5，`n_bind` 由自有 slot5 决定 | `sub_A1E210` 原始字节（R04 复核） |
| 原生 InputManager 是全局对象 `0xC57B18`（vptr `0xB827F4`，33 槽 0..32）；设备表 `+8/+0xC` 元素 8B `{device,control}`；`+0x14` 是内嵌原生 Mutex 包装（vtable `0xBA04B4`，lock=`+0xC`、unlock=`+0x10`） | R04 `r04_vtables`/`r04_manager`；`0xA20E22..0xA20E52` |

### 8.3 F1–F8 条件表（每项附 ABI、线程、失败后的资源与可恢复范围）

| 项 | 直接事实 / ABI | 成立条件 | 失败后的资源与可恢复范围 | 线程 |
| --- | --- | --- | --- | --- |
| F1 原始分配与 handler | `sub_A0F4C0`（size 在 `[ebp+8]`）与 `sub_A0F4E0`（size 在 ECX）都调 `sub_A648B0(op=0,ptr=0,hi=0,lo=size32)`；`malloc` 只收低 32 位；失败 → `push 0x7FCB9DD6; call sub_A23200`（遍历 `0xC37974` 链回调）后**返回 NULL** | `size<=0xFFFFFFFB` 时 `malloc` 参数 = `size+4`；`size∈{0xFFFFFFFC..0xFFFFFFFF}` 时回绕；`hi!=0` 时清零后 `malloc(4)` | 返回 NULL 即「分配失败」；handler 链的具体副作用未展开（链由 `sub_406740` 用同一 allocator 建立）；无回滚对象 | 任意（无锁，仅 64 位统计累加） |
| F2 ctor / key | 对象分配大小按类不同：Base/Keyboard `0x370`、Net `0x3F8`、Gamepad `0x4F0`（ctor 分配点与对应 dtor 的 `free(this,N)` 一致）；Base ctor 自身**不分配对象**。`sub_A1E050` 写 vptr=`0xB82758`、`+8`(uint16 标志)=0 后 `|=2`、`+0xC = dword_C7362C++`、三指针 `+0x10/+0x14/+0x18`=0、`+0x20..+0x3C`=0、`+0xC0/+0xC4`=0、`+0xC8`=-1、`+0xCC`=4、`+0xD0/+0xD4`=两个 float 常量、`+0xD8..+0xE4`=0、`sub_A2C980(this+0xE8)`、`memset(this+0x40,0,0x80)`、`memset(this+0xE8,0,0x284)`；**不调用 Init** | 只做写入，函数内无分配、无失败出口 | 若此后放弃创建：按 `NEVER`，`RAW_ALLOCATED -> EMPTY` 之外的路径不释放；key 计数器已自增且非原子（`mov edx,[0xC7362C]; lea eax,[edx+1]; mov [0xC7362C],eax`），不可回退 | 由调用者（派生 ctor）决定 |
| F3 自有 vptr | Base 表 `0xB82758` 共 **38 槽**（slot0..37）已逐项读出；**slot3 与 slot30 是 `_purecall`（`0xAF05D3`）**；具体设备覆写这两槽 | ctor 内部先写 `0xB82758`，自有 vptr 必须在原生 ctor **之后**覆盖；覆盖前任何虚调用都会落到 Base 表 | 覆盖失败＝虚调用进入 `_purecall` → 进程终止；无内存回滚问题 | 任意 |
| F4 binding / 双缓冲 | Init `sub_A1E210` **本身会先虚调 slot5**（`0xA1E21A: 8B 06; 0xA1E21C: FF 50 14`），再用 `+0x10/+0x14` 算 `n`：`n=(end-begin)>>4`（`sar`）→ `4n` 经 `mul/seto/neg/or` 饱和 → 两次 `sub_A648B0` → 写 `+0xC0`/`+0xC4` → 两次**重新计算**的 `4n` `memset` | 需先有已初始化的 binding vector（slot5 写入）；`n_bind>=1` 时有分配 | 分配返回 NULL 时**无空指针检查**且恒返回 `AL=1`；`n>0` 时函数内 `memset(NULL,0,4n)`；`NEVER` 下不释放已成功的缓冲 | 由 coordinator；对象已在 vector 内时 native `common` 也会改这些字段 |
| F5 control / CS / root | control block 大小 `0x18`；`sub_40CC10` 写 vptr=`0xB1A6FC`、`+4`=一次 32 位 store `0x10001`（strong=1, weak=1）、`+8`=Mutex vptr `0xB81C0C`、`+0xC`=0、`+0x10`=0、`sub_A15770(mutex)`；`+0x14`（object）由调用方设 | control 分配失败时**绝不**构造 `{object, null}` pair | control 未构造则不进入 `CONTROL_READY`；已构造的 root strong=1 见 §7 的槽语义 | 任意（槽内自持互斥量） |
| F6 按值 pair 注册与通知 | `sub_A20DF0`：锁 manager `+0x14` 的 Mutex → 若 `end!=cap` 则 `sub_902D60` 拷 8 字节 pair 并 `end+=8`，否则 `sub_A21630` 增长 → **解锁** → `call [vtable+0x78]`=`sub_A20FB0` → 若 `[ebp+0xC]` 的虚槽返回真且 `n79_0 @ 0xC7E814` 非空则 `n79_0(&pair)` | pair vector = `{+8,+0xC,+0x10}`，元素 8 字节；通知在临界区**之外** | 通知内的 record/binding 修改**不回滚**（`sub_835C90` 的写回不可撤销）；增长失败的 unwind 见 §5（R03） | manager 锁内只做拷贝/增长；通知在锁外 |
| F7 成员身份 / 同 key 唯一 / lookup 解析 | key→pair 查找是 manager vtable `0xB827F0` 的 slot11 = `sub_A20390`；`sub_835C90` 用 `__RTDynamicCast(device,0,InputDeviceBase TD,NetInputDevice TD,0)`，非零则整条映射跳过 | 「同 key 唯一」「解析到本对象」本轮**未重读**，沿用 v2 §7 的限定结论 | 复核失败按契约进入退休流程；本轮不新增可恢复范围 | 继承 v2 |
| F8 assignment / Join / generation 的独立窗口 | 本轮**未重读**，沿用 v2 / 05-03 的独立窗口结论 | 同上 | 同上 | 继承 |

### 8.4 L1–L4（按当前已隔离路径作有条件结论）

- **L1（分派）**：已隔离路径内成立，但有 **4 条已发现的直接旁路**（见 §6.3）：(1) manager update 对 vector 内每个 device **直呼** `common`（`0xA1FC97`）；(2) 注册路径直读写 `device+0x10` binding vector；(3) UI `sub_A243C0` 直读裸 binding vector；(4) **Init 体内虚调 slot5**（`0xA1E21C`）。支持 L1 的 xref 事实：Base slot0 `sub_A1E1B0` 全映像 **0 条 code xref**；slot5 `sub_A1E430` 0 条 code xref；slot12/13/14 各只有 vtable 数据引用；控制 block 六槽 `0x40CB50/0x4358B0/0x40C5C0` 经 MCP `xrefs_to` 复核为 20/11/20 条**全 data、0 code**。**L1 不能扩展到 `common`。**
- **L2（常驻与资源有效）**：`NEVER` 只保证自有对象/control 内存不被 coordinator 释放。对象 free 点已全部枚举：Base dtor `sub_A1E1B0`（`free(this,0x370)`，0 code xref）、Net dtor `0x9017C0`（`0x3F8`）、Gamepad dtor `0xA220F0`（`0x4F0`）、Keyboard dtor `0xA6C390`（`0x370`）、Gamepad ctor 的 EH funclet `0xB11850`（`0x4F0`）；它们只经各自类 vtable slot0 或该 ctor 的 EH 到达。**破坏基类资源有效性的原生路径**（本轮定位）：`cleanup sub_A1E2C0` 释放 `+0xC0/+0xC4` 并置 0（8 个 code caller 全在派生 dtor/EH 路径，Gamepad/Keyboard 各调两次）；`sub_A1E760` 把三指针与 `+4/+8/+0xC` 一起清 0（不释放）；Base dtor 写回 Base vptr 并析构 binding vector；`slot5 sub_A1E430` 覆盖 binding vector；`slot1 sub_A1E210` 可重复 Init 泄漏旧缓冲并改变 `n_bind`。L2 成立需附加：**自有对象只用自有 vtable、不从原生派生 ctor/dtor 构造/销毁、且无原生路径调用 Init/cleanup/Base dtor/slot5/`sub_A1E760`**。残余风险（本轮未隔离）：外部模块/未知别名直接 free 或直接 call 上述 dtor；把 vptr 写回 `0xB82758/0xB78030/0xB82C80/0xBA1678`。
- **L3（迟到访问无害）**：**不能**由「对象不释放」或「字段中性」推出。原生没有 RETIRING/UNREGISTERED/ZOMBIE 字段；`+8` bit1 是唯一观测到的“槽有效性门”（ctor 尾部 `bts ax,1` 置位，扫描范围内无清除点）。门控 bit1 的槽 7/8/10/11/12/13/14/15/24 在退役后仍会执行并读 `+0xC0/+0xC4`；不门控 bit1 的槽 16–23、30、34 与 `common` 本来就无退役概念，其中 slot19/20/21/34/`common` 还会**写**自有状态。「清 bit1 以中性化」是自有新增行为、需契约授权，且只覆盖上述 9 槽。L3 因此要写成「**逐槽覆写**（§9 D3 的候选集合）＋`n_prev/n_cur>=n_bind`＋binding vector 未被清空」，并在走路线 A 原生注册时接受 **native `common` 每帧直接推进语义**（`memcpy(+0xC4←+0xC0,4n)`、`memset(+0xC0,0,4n)`、`+0xC8=-1`、`+0xCC=4`）。
- **L4（代码有效）**：模块常驻 ⇒ vptr、COL 与槽代码在进程生命周期内有效；本轮无新依据，沿用 v2 §7。

### 8.5 原型、生产接入与 T0-05-05 剩余功能（本轮直接读取源码与工程文件，只读，未构建）

**原型文件**：`cs-t0/native/include/isaac_cs_t0/virtual_device_lifecycle.hpp`（310 行）、`cs-t0/native/src/virtual_device_lifecycle.cpp`（609 行）、harness `cs-t0/native/tests/virtual_device_lifecycle_main.cpp`。

| 原型事实（直接读源码） | 现行契约 v2 / `NEVER` | 差异 | 复用 / 需重写 |
| --- | --- | --- | --- |
| `LifecycleState = {Empty, ObjectAllocated, ControlReady, Registered, Retiring, Unregistered, **Destroyed**}` | §3 要求 `EMPTY/RAW_ALLOCATED/BASE_CONSTRUCTED/INITIALIZED/CONTROL_READY/REGISTERED/RETIRING/UNREGISTERED/ZOMBIE`，**不存在 `DESTROYED` 终态** | **直接冲突** | 需重写：终态改为常驻 retired（ZOMBIE）；`Destroyed` 名字与语义都要换 |
| `FinalRelease` = `ReleaseClass(FactoryLocalStrong, expectFinal=true)`；要求 `TotalStrong()==0 && objectDeleted && implicitWeakReleased && controlBlockFreed`，否则 `PortContractViolation`；成功后 `object_=nullptr; control_=nullptr; ++objectDeleteCount_; ++controlFreeCount_` | §3/§7 L2：进入 `BASE_CONSTRUCTED` 后对象与 control block **永不释放** | **直接冲突** | **必须重写**：`StrongReleaseResult` 的 `objectDeleted`/`controlBlockFreed` 语义、`FinalRelease` 的成功判据、以及「释放」命名 |
| `ReleaseClass(..., expectFinal=false)` 在 `result.objectDeleted \|\| result.implicitWeakReleased \|\| result.controlBlockFreed` 时报 `PortContractViolation` | 同上的 NEVER 版本 | 冲突 | 需重写为「释放 root 后对象仍常驻，只清 coordinator 引用」 |
| `RollBackRegistration` 结尾无条件 `(void)FinalRelease(port, token)`；`ManagerRemove` 未完成时置 `Retiring` 并提前返回 | §6 `CONTROL_READY -> REGISTERED` 失败处置 + `NEVER` | 冲突 | 需重写：回滚成功后对象常驻，进入 `UNREGISTERED`/`ZOMBIE`，不得释放 |
| `CleanupRawObject/CleanupRawControl(port, constructed)` → `DestroyRawObject/DestroyRawControlBlock` | §3「唯一允许的内存释放：`RAW_ALLOCATED -> EMPTY` 回滚」 | **一致** | 可复用（限「原生构造从未执行」） |
| 6 类 strong ref（`kStrongRefClassCount=6`）、`kMaximumSimultaneousStrong=6`、`kMaximumExternalWeak=0` | v2 已用本契约替代 v1 的「六类 strong 上限」条款 | 部分继承 | 结构可复用；上限语义需按 `NEVER` 重新论证 |
| 无 `+0xC0/+0xC4`、无 binding vector、无 `+0x10` 长度、无 38 槽表、无 COL/RTTI、无 slot3/slot30、无 `PINNED`、无 uint16 saturating 语义、无 shadow/mirror | §2/§4/§5 要求项 | **未实现** | 原型对这些**无任何覆盖**；不得当作真实设备接入依据 |
| 只被 `IsaacCsT0.VirtualDeviceLifecycleTests.vcxproj`（第 51–55 行）编译 | 生产工程 `IsaacCsT0.Native.vcxproj` 必须包含真实实现 | **未接入** | 生产接入需新增；生产工程的 `ClCompile`/`ClInclude` 现无该文件 |

**T0-05-05 剩余工作按功能归为四类**（不构成四次推进、不是工期或百分比估算）：

1. **静态证据收尾**（本轮主覆盖）：R01–R05 的直接事实与明确 `UNKNOWN`。
2. **可执行契约**（本轮提供候选）：§9 的差异草案；契约 v2 是否正式修订按实际任务处理。
3. **真实自有设备生产实现**（本轮不执行、不宣称完成）：自有 38 槽表与 6 槽 control 表、`+0xC0/+0xC4` 与 binding vector 的实际布局、`PINNED`、按 §R05 结论选定的计数同步、覆写槽实现，并接入生产 DLL。
4. **最小 Join/selected-device 与退休恢复接续**（本轮不执行）：05-05 的最小 Join/confirm 防重复不能省略；完整 A/B scheduler 隔离、1,000 tick soak 与 Replay 按 05-06/05-07 的实际目标处理。

## 9. 契约差异草案（DESIGN_ONLY，不覆盖契约 v2）

编号沿用 v3 §10 的 D1–D6 以便对照（D4 恢复为 v3 的「失败处置」），另加 D7–D9。全部为**候选**，本轮不采纳。

| # | 契约 v2 原条款 | 候选条款/差异 | 事实/推导依据 | 行为变化 | 未决/未实现 |
| --- | --- | --- | --- | --- | --- |
| D1 | §4「越界 action 的行为必须与 Base 实现对越界的处理一致」；未写明容量前提 | 增加注册前提：**32 个 16B binding 元素真实存在且已初始化**，且 `n_bind==32`、`n_prev>=n_bind`、`n_cur>=n_bind`；发布后禁止未协调的长度/容量变化。若采用一般 `n`，则实际存储与初始化必须按 `n_bind` 覆盖 | 证据 v4 §R02-3/4/6：`common` 由 manager update 直接调用并现算 `4*n_bind`；`n_bind=33` 反例 | 容量条件从「`n>=32`」收紧为「缓冲覆盖 `n_bind`」 | 默认 binding 运行期实值；是否拦截 notification 属额外设计 |
| D2 | §5「所有槽可在任意线程被调用，不得阻塞等待主线程，不得执行回调、析构或内存释放」+「自有槽不依赖原生 embedded mutex」 | 保留前半句。后半句须按 R05 的选择二选一：**A1 自有短锁**（建议取无界 CAS 自旋；bounded 变体已证不完整）或 **B 使用原生 embedded Mutex**（放弃「不依赖」，且该 Mutex 非可重入、会在 `[CS+0x18]` 上 `Sleep(1000)` 自旋）。两种都必须给出线性化点与镜像观察者范围；A1 还需在契约 §2 增列扩展区字段（`self_lock`、`state`、诊断环） | §7（R05）分册 §3.3/§3.6 | 从「不依赖原生 mutex」改为「依赖 A1（无界）或 B 之一」；`AddStrong` 的 0 基输入必须明文禁止复活 | 是否存在原生 inline reader/writer；「自旋等待」是否等价于「阻塞等待主线程」需契约解释 |
| D3 | §4 六槽覆写集合 `+0x00/+0x0C/+0x78/+0x30/+0x34/+0x38` | 维持六槽，但补三条：(a) `common` 对每个已注册设备的 `4*n_bind` 访问**不经虚分派**，覆写集合无法覆盖它；(b) **原生 Init 自身会虚调 slot5**（`0xA1E21A: 8B 06; 0xA1E21C: FF 50 14`，`ecx=this`，无栈参），因此 `n_bind` 由自有表里的 slot5 决定 —— 若 slot5 继承 Base 实现 `sub_A1E430`，`n_bind` 就等于全局默认 binding 向量的运行期长度（设计时未知）；若要固定 `n=32`，必须在 slot5 给出受控实现或在使用前复核；(c) 需评估受控覆写 **9 个槽**：`+0x04/+0x08/+0x14/+0x1C/+0x20/+0x4C/+0x50/+0x54/+0x88`（即 slot1/2/5/7/8/19/20/21/34；拒绝二次 Init、不释放自有缓冲、限制 slot5 重拷贝、RETIRING 后不写快照/masks），或明确接受「无原生路径调用它们」的假设 | 证据 v4 §R02-6、§8.2 的 Init 虚调事实、§R04 的 caller 表 | 覆写集合可能从 6 槽扩展到 9 槽 | vtable 间接 caller 未隔离；全局默认 binding 运行期长度 |
| D4 | §6 失败处置（v3 原 D4） | 明确四类：**(a) Init 分配失败/回绕在函数内不可恢复** —— 分配返回 0 被直接写入 `+0xC0/+0xC4`，函数体内无空指针检查且恒返回 `AL=1`；**(b) allocator OOM 出口是「诊断/回调 + 返回 NULL」而非抛出**（`push 0x7FCB9DD6; call sub_A23200` 后 `xor eax,eax; retn`）；**(c) insert/grow 在锁内的 C++ 异常会锁残留** —— `0xBD21A8` 两个 unwind entry 都不指向 unlock，`0xA217CE` catch-all 也不 unlock，且该 `KAGE::Mutex` 是递归的；**(d) `sub_40CF00` 在 size<0x1000 时返回 0 导致 AV、size>=0x1000 走 `_invalid_parameter_noinfo_noreturn`，AV 是否被 `catch(...)` 覆盖无依据**。并明确 `TryAddStrong` 返回 false 不被检查（`end+=8` 发布 `{0,0}`）与 erase-shift 半 erase 不可重试 | R01 §3、R03 §5.2/§5.3 | 失败处置从「笼统不可恢复」细化为四类可判定/不可判定 | 现场 handler 行为、AV 分类、`_CxxFrameHandler3` 运行时行为 |
| D5 | §4 `+0x78` 名为 poll，做 current/previous 锁存，返回非 0；退休输出中性 | 原生 `common` 把 `[vtable+0x78]` 当**布尔门控**用（`call [eax+0x78]; test al,al; jz 返回`），base 该槽是 `_purecall`，具体设备返回常量 1 或真值。契约的「锁存 poll」与原生「门控」是两套语义，必须择一并写清返回值约定：若取门控读法，则退休期返回 0 会让原生 `common` 提前返回（内存上更保守），与「退休必须非 0」相冲突 | 证据 v4 §R02-6、§R04（slot30 = `0x709C30: B0 01 C3`） | `+0x78` 的语义与返回约定需要重写 | 退休后 poll 的实际调用窗口；`+0x78` 同时是 native `common` 的门控与自有 poll 时如何自洽 |
| D6 | §5「返回值、`+0x04/+0x06` 镜像值、`+0x14` 清零时机均相同」+ 计数所有槽可在任意线程 | 保留；但必须写明：(a) 打包 store 只对**单个 32 位 load** 原子，native 的两次独立 u16 load 仍可混读（交错 I9）；(b) 打包 store 的 4 字节对齐不能只靠 malloc 承诺，需 `lock` 前缀或运行时检查；(c) 最后 strong 的 `+0x14` 清零与隐含 weak 在候选里被合并进同一临界区，外部可见窗口比原生「dtor 后、weak 前」更短 | §7（R05）分册 §3.4/§4 I9/§5 | 一致性从「所有槽」收窄为「取同一锁的调用者 + 单个 32 位 load」；`+0x14` 的外部可见顺序需明文 | 是否存在原生 inline reader/writer；契约是否要求「两次 u16 load 同一快照」 |
| D7 | §3 明确「不存在 `DESTROYED` 终态」 | 与离线原型直接冲突：原型终态就是 `LifecycleState::Destroyed`，且 `FinalRelease` 要求 port 报告 `objectDeleted && implicitWeakReleased && controlBlockFreed`。原型须按 `NEVER` 重写后才能复用；它目前**只编入独立测试工程**，不在生产 DLL | 证据 v4 §8.5（生产/测试 `.vcxproj` 的 `ClCompile`/`ClInclude` 列表） | 原型不能作为真实设备接入依据 | 无 |
| D8 | §4「其余 32 槽继承 Base 实现，前提是 O-BUF 判定该槽对虚拟设备安全」 | 本轮把 O-BUF 的具体范围收窄为：**slot7/8/12/13/14/19/21 直接读 `+0xC0/+0xC4`**，且 `common`（非虚分派）按 `4*n_bind` 访问两块缓冲。因此「该槽安全」的判据必须写成「缓冲覆盖 `n_bind`」而不是「该槽不访问硬件」 | 证据 v4 §R02-2/3/6/9 | 继承槽的安全判据可操作化 | 范围外 slot 未逐一判定 |
| D9 | §5 control block `[0,0x18)` 与原生行为一致（本轮由原 D4 顺延编号） | 本轮原始字节确认布局（`+4/+6` 计数、`+8` Mutex、`+0xC` 标志、`+0x10` CS 指针、`+0x14` object）。**补充**：原生 `ReleaseStrong` 在 strong 归零时会 `push 1; call object->vtable[0]`（即对对象调**删除型析构**）并清 `+0x14`，然后尾调 `ctrl->vtable[5]`；自有实现必须**不**调用对象析构 | 证据 v4 §R02-10、§R05（`0x4358DB..0x4358F8`） | 自有 `+0x0C` 的行为与原生在「谁析构对象」上不同 | 无 |

## 10. 唯一一次整体静态文档审阅

按[工作设计 v3](virtual-device-owned-abi-static-confirmation-next-work-design-v3.md) §7 执行**一次**人工静态审阅：读取本 v4、[证据 v4](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v4.md) 及其三个主题分册、本设计与[契约 v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)、以及[07-virtual-device README](../README.md)。未运行程序、构建、游戏或校验脚本。

### 10.1 七项检查

| # | 检查项 | 结果 | 依据/缺口 |
| --- | --- | --- | --- |
| 1 | **ABI**：栈位移忠于原始字节；wrapper/Init/下层 allocator 分开；整数公式、header、payload 与失败出口不混用 | **通过** | Q1 已修正为 `[ebp+8]/[ebp+0xC]`（`0xA64910: 8B 5D 0C` / `0xA64917: 8B 7D 08`），并写明 IDA frame 偏移 = EBP 位移 + 8；`sub_A0F4C0`/`sub_A0F4E0`/Init 直调三者分列（清栈归属分别记录，Init 为 `ret 4`）；§3 的四个概念表把「请求大小/malloc 实参/4B header/payload 可用范围」分行；OOM 出口明确为「诊断/回调 + 返回 NULL」。`malloc(0)` 现场行为与 handler 目标保留 `UNKNOWN` |
| 2 | **容量**：固定 n=32 或一般 n 的实际存储/初始化要求清楚；所有消费者共用同一条件；n=33 反例已处理；UI 边界准确 | **通过** | §4 的五量定义 + 唯一安全条件；`common` 由 manager update 直接调用这一事实使「缓冲覆盖 `n_bind`」成为注册前提；`n_bind=33`（132B vs 128B）反例保留；UI 边界由 `sub_A243C0` 的 `index < ((end-begin)>>4)` 修正为 `0<=action<n_bind`；`+0xB00` 笔误已由「Base 布局 `[0,0x370)`、最大设备对象 `0x4F0`」证伪并改为 `+0xC0/+0xC4`。默认 binding 的运行期 count 仍 `UNKNOWN`（已标注为设计前置） |
| 3 | **EH**：相关 state 与具体指令、资源/锁/发布/回调副作用对应；无法解析处准确标注；恢复建议不越过证据 | **通过** | §5 给出 raw state 索引方式的两条独立证据、5 张 FuncInfo 的 raw 0x24 字节与 unwind/try/handler 原文、5 个 funclet/chunk 归属、逐 state 的资源/锁/发布/副作用表；§5.3 的四类可恢复范围每条标注依据。`_CxxFrameHandler3` 在外部 CRT、`n79_0` 与 list `+0x3C`/`+0x44` 的 callback 目标、AV 是否被 `catch(...)` 覆盖均标 `UNKNOWN`，未据此给恢复建议 |
| 4 | **lifetime**：常驻与有效基类资源分开；直接/虚调用 receiver 有范围；增补覆写及旁路说明与 L2/L3 相符 | **通过**（范围外 receiver 标 `UNKNOWN`） | §6 的 caller/receiver 表、资源生命周期表、槽行为表与 4 条直接旁路；L2 明确「`NEVER` 只保证自有对象/control 内存不释放」，并列出破坏基类资源有效性的原生路径与全部对象 free 点；L3 明确「字段中性不足」与「清 bit1 只覆盖 9 个槽」。46 个末定类的间接 receiver 已在分册 §6 列具体地址并标 `UNKNOWN` |
| 5 | **计数**：六槽返回、获取失败、隐含 weak、饱和/溢出/PINNED、可见窗口和同步参与者有具体答案**或明确缺口**；不作无依据的原子算法全称判断 | **通过（含明确缺口）** | §7 给出六槽逐槽基线（含 `AL` 返回、清栈、Mutex 使用与「无原子指令」的字节扫描）、control block 布局、A1 候选的 7 行操作与线性化、I1–I9 有限交错。**「bounded CAS 获取失败」明确记为候选算法尚不完整**（四约束不可同时满足）；打包 store 的对齐依据标 `UNKNOWN` 并给出 `lock` 前缀/运行时检查两条出路；A0 反例只用于否定 A0，未推广为「所有纯原子算法不可能」 |
| 6 | **整合**：Q1–Q8、16 项 O-*、F1–F8、L1–L4 与契约差异可定位；原型及生产接入状态准确；未把静态报告视为 T0-05-05 已完成 | **通过** | §2 的 Q 表含 Q1–Q9（Q9 为本轮自纠）；§8.1 逐项列出 16 项 O-*（区分「本轮重读」与「继承 v2/未重读」）；§8.3 为 F1–F8 条件表；§8.4 为 L1–L4；§9 为 D1–D9 契约差异草案（`DESIGN_ONLY`）；§8.5 明确原型只在测试工程内编译、`FinalRelease` 与 `NEVER` 直接冲突，并给出 T0-05-05 剩余功能四分类与「本轮不宣称阶段完成」 |
| 7 | **文档**：主报告/证据开头和末尾状态、导航与引用一致；保留历史和失败；不含 gate、哈希校验、角色分离或多个验收安排 | **通过** | 本报告与证据 v4 的状态行、§0 工具段、主题分册索引互相一致；五处相对链接已逐一验证可解析；v1–v3 报告与证据、失败记录、旧 gate/哈希字段原文均未改写；本报告与证据均声明「未运行构建/测试/harness/verifier」，不设 gate、不校验哈希、无角色分离、无多次验收 |

### 10.2 四项分别报告

1. **事实修正是否完成**：Q1–Q8 全部有落点，另有 Q9（本轮自纠）。其中 Q1（栈位移）、Q2（容量条件）、Q6（状态矛盾）、Q7（字段/边界笔误）、Q8（生产接入）为**事实性修正**；Q3（计数）与 Q4（EH）、Q5（lifetime）为**补齐与条件化**，不是把「未完成」改写成「已确认」。
2. **局部静态分析覆盖范围**：R01 覆盖 `sub_A648B0`/两个 wrapper/free wrapper/`sub_A23200` 链/Init 的完整指令与调用点；R02 覆盖 `0xA1E000..0xA26000` 的缓冲读写点、`.text` 全段的 `n_bind` 惯用式（13 处/9 函数）、注册与 UI 路径；R03 覆盖 5 张 FuncInfo + 5 个 funclet + insert/remove/grow/erase/migration/notification/record-append；R04 覆盖 4 张设备表 38 槽 + control 6 槽 + `.text` 全段间接调用普查（1,372 + 25 命中）；R05 覆盖六槽逐字节、control 布局、有限交错。**未覆盖**：全程序 points-to、未知/未来模块、动态生成代码、运行时数据值（默认 binding、`n79_0`、COL 实例）。
3. **候选算法/契约是否完整**：契约差异草案 D1–D9 已形成但**未采纳、未实现**；计数候选 A1 的**主候选（无界自旋）**操作层完整，**bounded 变体明确不完整**；A2/B 只作比较分析；slot 覆写集合的扩展（D3）与原型重写（D7）是待决设计，不是已完成工作。
4. **剩余功能及无法确认的证据**：剩余功能按 §8.5 四类（静态证据收尾 / 可执行契约 / 真实生产实现 / 最小 Join 与退休恢复接续）；无法确认的证据已在证据 v4 §0 与各分册 §4/§6 逐条给出地址与原因（`malloc` 对齐、`n79_0` 目标、list `+0x3C`/`+0x44` 回调、`_CxxFrameHandler3`、46 个末定类 receiver、全局模板运行期值、`+0xC8/+0xCC` 语义、协调器状态到原生字段无映射、UI 向量指针来源、Keyboard slot30 返回语义）。

### 10.3 已知矛盾与未通过理由的记录

本审阅**未发现**本次交付内部存在未标注的矛盾；但如实记录以下三处**已发现并已按原始字节处理**的矛盾/自纠，保留历史、不静默改写：

1. **Q6（v3 内部矛盾）**：证据 v3 第 4 行「进行中（K03–K07 待补）」与主报告 §12.7「主报告与证据 v3 状态一致 … 通过」互相矛盾。v4 §2 Q6 明确旧「通过」不支持完整结论；v3 原文未改写。
2. **Q9（本轮自纠）**：本报告一度按 manager vtable 起点 `0xB827F0` 对齐，误得 `+0x78 = 0xA20FB0`；改用 `_RTTICompleteObjectLocator.pTypeDescriptor` 复核后确认起点是 `0xB827F4`，`+0x78` = slot30 = `0xA211D0`，**v3 原结论成立**。错误过程与撤回理由记在 §2 Q9 与证据 §R02-7/§R02-12。
3. **R04 分册 §3.1 的一处误读**：该分册称「`common` 不调用 slot30(+0x78)」，与原始字节 `0xA1F307: 8B 40 78` / `0xA1F30A: FF D0` / `0xA1F30C: 84 C0` / `0xA1F30E: 75 06` 不符；已在本报告 §6.4 与 R04 分册抬头按原始字节更正（分册提到的 `+8` bit3 门控与 `call [eax+0x84]` 是同一函数内后续阶段的门控，不否定前一处）。

**审阅结论**：本次静态文档交付的七项检查全部通过（第 5 项带明确缺口标注）。**这不表示 T0-05-05 已完成**：契约修订未被采纳、自有设备生产实现与最小 Join/退休恢复接续未执行，计数候选的有界变体仍不完整。审阅失败后不自动修补重跑；后续有实质修改时按新的完整目标推进。
