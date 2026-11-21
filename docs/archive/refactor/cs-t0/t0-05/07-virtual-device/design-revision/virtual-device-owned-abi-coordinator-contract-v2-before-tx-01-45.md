> tx-01-45 前的契约原文快照；仅保留历史条款，不作为现行 writer/root 要求。

# CS-T0-05-05 Virtual Device 自有 ABI 协调器契约 v2

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

日期：2026-10-10  
证据等级：`DESIGN_ONLY`  
实现状态：`DEFINED_NOT_IMPLEMENTED`  
设计依据：[设计修订稿 v1](virtual-device-owned-abi-design-revision-v1.md)（用户已采纳 D1–D6）

## 1. 定位

- 本契约定义自有 ABI 路线下虚拟设备必须满足的行为。它不是游戏二进制中的事实，不贡献任何 `STATIC_CONFIRMED`。
- [契约 v1](../virtual-device-lifecycle-coordinator-contract-v1.md) 原文保留不改。v1 中依赖“原生最终释放线程受约束”和“同步析构”的条款（`LC_FINAL_RELEASE`、`EXTERNAL_WEAK=0`、六类 strong 上限等），在本路线下由本契约替代；其余条款中与本契约不冲突的部分，作为设计约束继续有效。
- 本契约与设计修订稿 §3.2 的回滚列冲突时，以本契约为准（D2=`NEVER` 使“构造后释放”不再允许）。
- OriginalOnline `NetInputDevice` 与 Online Manager 仍不得作为任何组件。

## 2. 组件与内存布局

| 组件 | 布局要求 |
| --- | --- |
| 对象 | `[0, 0x370)` 为原生 InputDeviceBase 布局，只由原生 base ctor 与原生 Init 写入；`[0x370, 0x370+ext)` 为私有扩展。分配大小与对齐由 O-ALLOC 确定 |
| 对象 vptr | 指向自有 38 槽表。表内容为冻结的 Base vtable 逐项复制，仅覆写第 4 节列出的 6 槽 |
| 对象 COL | 位于自有模块 `vptr[-1]`；TypeDescriptor 名称 `.?AVVirtualInputDevice@IsaacCsT0@@`；层次为 `[自身, InputDeviceBase]` |
| control block | `[0, 0x18)` 与原生布局一致，由原生 refcount base ctor 初始化；`[0x18, 0x18+ext)` 为私有扩展（32 位影子 strong/weak、`pinned` 标志、诊断） |
| control vptr | 指向自有 6 槽表 |
| 模块 | 在第一次调用原生 base ctor 之前，以 `GET_MODULE_HANDLE_EX_FLAG_PIN` 固定；固定失败则不创建设备 |

## 3. 状态机

```text
EMPTY
  -> RAW_ALLOCATED       （只分配，未调用任何原生构造）
  -> BASE_CONSTRUCTED    （原生 base ctor 已执行，自有 vptr 已写入）
  -> INITIALIZED         （原生 Init 已执行，双缓冲已分配）
  -> CONTROL_READY       （control block 已构造并绑定，root strong 已持有）
  -> REGISTERED          （已在 InputManager vector 中，且 F7 唯一性复核通过）
  -> RETIRING            （admission 已关闭，输出中性）
  -> UNREGISTERED        （已经原生 remove，vector 中确认不存在）
  -> ZOMBIE              （root strong 已释放；终态）
```

- `PINNED` 是与状态正交的粘滞标志：一旦置位不再清除。它只影响诊断与 fail-closed 信号；在 `NEVER` 策略下，内存行为与未置位时相同（都不回收）。
- 不存在 `DESTROYED` 终态。在 `NEVER` 策略下，进入 `BASE_CONSTRUCTED` 之后，对象与 control block 在进程生命周期内都不释放。
- 唯一允许的内存释放：`RAW_ALLOCATED -> EMPTY` 回滚（原生构造从未执行，`this` 不可能已逃逸）。
- 每个 session 至多发起一次创建；任何失败都不自动重试（研究计划 §2.4、§15）。因此泄漏上界为每 session 一个对象加一个 control block。

## 4. 对象 vtable 覆写

| 槽 | 自有行为 | 线程 |
| --- | --- | --- |
| `+0x00` deleting dtor | 不调用 base dtor，不释放内存。置 `unexpectedDestroy` 诊断（coordinator 从不主动调用此槽），对象进入中性输出；返回 `this` | 任意 |
| `+0x0C` | 按 O-ABI-0C 确定的签名返回自有静态常量；O-ABI-0C 闭合前不得实现 | 任意 |
| `+0x78` poll | 把当前已发布快照锁存为 `current`，原 `current` 移为 `previous`；按 O-ABI-78 确定的语义返回。不读硬件、不阻塞、不分配、不调用原生函数 | 由 O-THREAD 确定；实现与线程无关 |
| `+0x30` | 返回 `current` 中 action 的按下状态；`RETIRING` 及之后返回 false | 任意 |
| `+0x34` | 返回由 `previous/current` 计算的边沿，语义按 O-ABI-QUERY 确定；`RETIRING` 及之后返回 false | 任意 |
| `+0x38` | 返回 `current` 中 action 的数值（x87 `ST(0)`，`ret 4`）；`RETIRING` 及之后返回 0.0 | 任意 |

- 其余 32 槽继承 Base 实现，前提是 O-BUF 判定该槽对虚拟设备安全；判定不安全的槽改为覆写并返回中性值，此时须同步修订本表。
- 越界 action（`action >= n`）的行为必须与 Base 实现对越界的处理一致，具体由 O-ABI-QUERY 确定；在确定之前一律返回中性值。
- 覆写槽读取的快照是不可变对象，以原子指针发布；查询路径不得修改任何共享状态。poll 是唯一写锁存的槽，锁存字段只由 poll 写入。

## 5. control block 槽语义

对调用者可观察的语义与原生实现一致：返回值、`+0x04/+0x06` 的镜像值、`+0x14` 的清零时机均相同。差异只有下表列出的四点，且方向一律是“延长寿命”或“fail-closed”。

| 槽 | 原生行为（REVERSE v1） | 自有行为 |
| --- | --- | --- |
| `+0x00` deleting dtor | 销毁 critical section；flag bit0 时释放 `0x18` | 不释放；置诊断 |
| `+0x04` TryAddStrong | 加锁；strong≠0 时转 `+0x08` | 原子比较交换影子 strong：为 0 则失败，否则加一；同步镜像 |
| `+0x08` AddStrong | `++uint16 strong` | 原子加一影子 strong；超过 `0xFFFF` 则置 `PINNED`，镜像饱和为 `0xFFFF` |
| `+0x0C` ReleaseStrong | `--strong`；变零时调 `object->vtable[0](1)`、清 `+0x14`、转 `+0x14` weak release | 原子减一影子 strong；在 0 上再减则置 `PINNED` 且不修改计数；变零时**不**调用对象析构，清 `+0x14`，标记 `ZOMBIE` 候选，然后转自有 `+0x14` |
| `+0x10` AddWeak | `++uint16 weak` | 同 `+0x08` 规则作用于 weak |
| `+0x14` ReleaseWeak | `--weak`；变零时调 `ctrl->vtable[0](1)` | 原子减一影子 weak；下溢规则同上；变零时不释放 control block |

- 所有槽可在任意线程被调用，不得阻塞等待主线程，不得执行回调、析构或内存释放。
- 返回值的精确语义（bool 含义、`AL` 以外寄存器）以 O-CB 确定的原生实现为准，自有实现须逐槽对齐。
- 原生 embedded mutex（`+0x08..+0x10`）保持由原生 ctor 初始化后的状态，供可能存在的内联加锁路径使用；自有槽不依赖它。是否存在这样的路径由 O-RC-INLINE 判定。

## 6. 转换、线程与失败处置

所有生命周期转换都由 coordinator 在 `AUTHORITY_MAIN_LIFECYCLE`（主线程，scheduler 提交 authority tick 之后、下一次 manager update 之前）执行，并由 thread token 强制。

| 转换 | 动作 | 失败处置 |
| --- | --- | --- |
| `EMPTY -> RAW_ALLOCATED` | 固定模块；调用原生分配函数（O-ALLOC） | 分配失败：回到 `EMPTY`，记录失败 |
| `RAW_ALLOCATED -> BASE_CONSTRUCTED` | 调用原生 base ctor（O-CTOR）；写自有 vptr；初始化扩展区 | ctor 失败（若 O-CTOR 证明存在失败出口）：不释放，标记泄漏，session 内终止创建 |
| `BASE_CONSTRUCTED -> INITIALIZED` | 按原生创建顺序执行 Init（O-CTOR） | 不释放；session 内终止创建 |
| `INITIALIZED -> CONTROL_READY` | 分配 control block；调用原生 refcount ctor；写自有 ctrl vptr；`ctrl+0x14 = object`；影子 strong=1、weak=1（root） | control 分配返回 null：绝不构造 `{object, null}` pair；对象泄漏，session 内终止创建 |
| `CONTROL_READY -> REGISTERED` | 以按值 pair 调原生 insert（O-INSERT-ABI）；随后用 keyed lookup 复核 key 唯一且解析到本对象（O-KEY） | insert 异常或无法确认成员身份：进入 `RETIRING`，再走 remove 路径；复核失败同样处理 |
| `REGISTERED -> RETIRING` | 关闭 admission；generation 加一；所有覆写查询输出中性；B 解绑或退出按 05-03 语法处理 | 不可失败（只写自有状态） |
| `RETIRING -> UNREGISTERED` | 调原生 remove（O-REMOVE、O-NOTIFY）；确认 vector 中不存在 | 无法确认时停在 `RETIRING`，保留 root，记录 fail-closed |
| `UNREGISTERED -> ZOMBIE` | 经自有 `+0x0C` 释放 root | 不可失败；残余原生引用只影响影子计数 |

- 进程退出：manager stop 在任意线程释放 vector strong，由自有槽处理；退出阶段不要求到达 `ZOMBIE`。
- 意外事件（`unexpectedDestroy`、`PINNED`、被他人 remove、F7 复核失败）一律使设备立即进入中性输出，并上报 fail-closed；不自动恢复。

## 7. 安全性质

实现与证据必须共同支持以下四条：

- **L1（分派）**：对本对象 control block 的计数修改与释放，全部经其 vptr 进入自有槽。依据为已确认的 Release 两槽零直接调用；O-RC-INLINE 补全其余槽与内联副本。
- **L2（不释放）**：进入 `BASE_CONSTRUCTED` 后，对象与 control block 永不释放。因此任何原生路径（含未知别名、任意线程、进程退出）都不会访问已释放内存。
- **L3（迟到访问无害）**：`RETIRING` 及之后，所有覆写槽返回中性值；继承槽操作的基类状态始终有效（base cleanup 永不执行）。
- **L4（代码有效）**：模块常驻，vptr、COL 与槽代码在进程生命周期内始终有效。

## 8. 恢复口径（D5）

恢复完成的定义改为同时满足：

1. 所有 Hook 目标字节逐字节恢复；
2. 虚拟设备处于 `UNREGISTERED` 或 `ZOMBIE`，且 InputManager vector 中不存在；
3. B 已解绑或退出；
4. 自有 DLL 常驻，不卸载。

## 9. 不在本契约范围内

- device key 的取值选择、assignment 时机与 join 路线细节：由 O-KEY、O-JOIN 的结果另行固定。
- B 的 action 快照如何由 scheduler 生成，以及 exactly-once：属于 05-06。
- 任何运行时权限：本契约不授权进程访问、注入、IPC、Hook 或 canary。
