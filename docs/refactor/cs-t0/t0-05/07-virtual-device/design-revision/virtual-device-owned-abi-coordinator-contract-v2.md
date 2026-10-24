# CS-T0-05-05 Virtual Device 自有 ABI 协调器契约 v2

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

日期：2026-10-10；tx-01-45 修订：2026-10-16  
证据等级：`DESIGN_ONLY`  
实现状态：`PRODUCTION_SOURCE_ADDED_ACCEPTANCE_FAILED`（tx-01-45，Rust 构建成功；Native 链接 LNK1101 / MSPDB140.DLL 版本不正确；实机 NOT_RUN。见[验收记录](../tx-01-45-results.md)）  
后续状态：tx-01-46 已链接成功；授权继续实机后远程初始化 0xC0000005、游戏退出，整体仍 FAIL，见[本轮结果](../tx-01-46-link-repair.md)。  
历史状态：tx-01-47 修复 Publish 的参数传递，实机初始化与正常 shutdown 成功、47 槽恢复；该轮整体 E2E NOT_OBSERVED，绑定/注册/Join 未确认，见[记录](../tx-01-47-event-publication-repair.md)。  
历史状态：tx-01-48 修复拓扑对 Lua engine 的误拒绝，一次实机确认 A/B 绑定、单次设备注册、原生 Join 与身份匹配、B 冲突退休后 A 继续、菜单 teardown/47 槽恢复与 DLL 常驻；完整 E2E 仍为 NOT_OBSERVED（35 PASS、7 未观察项），见[当前结果](../tx-01-48-binding-join-repair.md)。  
历史修复：tx-01-50 已补齐原生奇数帧普通 Player 模拟窗口；最终 E2E 观察到 B 死亡/复活、新输入恢复及旧 generation 拒绝，48 槽正常恢复、DLL 常驻。专项整体 FAIL（19 PASS、1 FAIL），换房及相邻方向切换仍有丢蓄力，未自动重跑。见[来源、修复和结果](../tx-01-50-input-window-repair.md)。  
当前修复：tx-01-52 后按相反射击方向优先与释放回退一次实机36项专项 PASS，用户确认蓄力正常；新增 B raw 脚本移动量重复死区另记补充 FAIL。tx-01-53 修复该新增分支并一次实机8/8 PASS，101个 raw/4个 canonical步骤保留移动 pair，48 Hook恢复/DLL常驻。跨房自动发射及51未接纳 Join 原因仍未解决，见[52结果](../tx-01-52-opposite-shooting-priority.md)与[53结果](../tx-01-53-script-movement-preservation.md)。  
设计依据：[设计修订稿 v1](virtual-device-owned-abi-design-revision-v1.md)（用户已采纳 D1–D6）

## 1. 定位

- tx-01-45 已经用户采纳 Authority writer、常驻 root、缺输入立即中性及正常顺序路径范围。修订前原文见[历史快照](virtual-device-owned-abi-coordinator-contract-v2-before-tx-01-45.md)。下面功能条款按本次采纳同步，不把源码落盘等同于验收通过。

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
                         （UNREGISTERED 为正常退休终态，root strong 常驻）
```

- `PINNED` 是与状态正交的粘滞标志：一旦置位不再清除。它只影响诊断与 fail-closed 信号；在 `NEVER` 策略下，内存行为与未置位时相同（都不回收）。
- 不存在 `DESTROYED` 终态。在 `NEVER` 策略下，进入 `BASE_CONSTRUCTED` 之后，对象与 control block 在进程生命周期内都不释放。
- 旧 ZOMBIE/root-release 路径保留在历史快照，本路线不主动执行；control 的 object 字段、隐含 weak 和 native Mutex/CS 随 root 保持有效。
- 唯一允许的内存释放：`RAW_ALLOCATED -> EMPTY` 回滚（原生构造从未执行，`this` 不可能已逃逸）。
- 每个 session 至多发起一次创建；任何失败都不自动重试（研究计划 §2.4、§15）。因此泄漏上界为每 session 一个对象加一个 control block。

## 4. 对象 vtable 覆写

| 槽 | 自有行为 | 线程 |
| --- | --- | --- |
| `+0x00` deleting dtor | 不调用 base dtor，不释放内存。置 `unexpectedDestroy` 诊断（coordinator 从不主动调用此槽），对象进入中性输出；返回 `this` | 任意 |
| `+0x0C` | 按 O-ABI-0C 确定的签名返回自有静态常量；O-ABI-0C 闭合前不得实现 | 任意 |
| `+0x78` poll | 不推进快照；返回 native active=true，避免在退休协调器前触发 poll=false 自动移除。不读硬件、不阻塞、不分配、不调用原生函数 | 无快照写入 |
| `+0x30` | 返回 `current` 中 action 的按下状态；`RETIRING` 及之后返回 false | 任意 |
| `+0x34` | 返回由 `previous/current` 计算的边沿，语义按 O-ABI-QUERY 确定；`RETIRING` 及之后返回 false | 任意 |
| `+0x38` | 返回 `current` 中 action 的数值（x87 `ST(0)`，`ret 4`）；`RETIRING` 及之后返回 0.0 | 任意 |

- 其余 32 槽继承 Base 实现，前提是 O-BUF 判定该槽对虚拟设备安全；判定不安全的槽改为覆写并返回中性值，此时须同步修订本表。
- 越界 action（`action >= n`）的行为必须与 Base 实现对越界的处理一致，具体由 O-ABI-QUERY 确定；在确定之前一律返回中性值。
- Authority-pre 对 A/B 各自 Resolve 至多一次并发布 current。StandardRoom original 正常返回后提交 previous；原生奇数帧 955241→6FD3F0 的正常 Main Player 路径，在原生暂停/状态检查完成、首次 Player::Update 前开启一次 Authority，helper 正常返回后提交 previous。两者共用 scheduler；暂停/FSM 特殊 Player 调用不推进。只有对应正常模拟返回的 Authority-post 可提交仍属当前 generation 的有效 current/neutral。TopUpdateExit 不写 previous；generation transition 只失效 tag/可见性，poll 不推进快照，查询不消费命令；正常主线程范围外中性。
- CurrentEligible 约束所有受控输出；PreviousComparable 只用于 action 0–7 的 level-derived Triggered。8–11 显式 edge 只读当前 target 的已 Resolve bit，不因 previous 无效而被整体抑制。新绑定首个有效命令前 current 无效且输出中性，不凭空建立 previous。
- tx-01-54 跨房修复已一次专项18/18 PASS，用户确认持续按住攻击跨房不误发：无 Authority tick、pause/loading/lifecycle 阻断或停用期间，C/S 主动分派不调用原生 Shooting consumer，以免中性查询向武器状态机制造 release；原生 Player 更新及观测 wrapper 保持原调用。有效无阻断 tick 仍执行 Shooting，即使 current/命令缺失也消费中性，不隐式保持旧攻击。A/B 共用规则，事件25记录实际执行/暂停；不改武器字段、输入 codec 或 Hook 槽数。本轮覆盖普通门切换与暂停，不外推 loading/换层或所有武器。详见[本轮结果](../tx-01-54-room-shooting-suspension.md)。
- tx-01-51 用户采纳：每 generation 重新唯一绑定后的首个新鲜命令可经 queue/Resolve 指向 completed+1；另将相同连续量按正常 lead=2 排到 completed+2，使用新 sequence 且 edge=0，随后恢复普通 lead=2。A/B 共用此规划，未收到新输入不启动。物理源须当前 native poll loop/exact pair 匹配；物理按键历史不因 gameplay generation 清零，已发布 edge 不重复，旧 frame 的 rising 不搬入新绑定。该例外不放宽 CurrentEligible，也不复制旧 current 或旧 pending。
- tx-01-52 用户确认并采纳：射击源在 canonical 化前按每条轴解析后按方向，释放胜出键回退仍持有的相反键；未知同次采样顺序保留仍按住的旧胜出方向，无历史时右／下优先。源历史与 Authority current/previous 分离，A/B 各自保存、使用同一转换；真实物理 device/control/K 变化才重置 A 的源历史，room/pause generation 不清零它。B 原始方向脚本只在当前 generation 的新鲜按期输入上推进历史；旧/过期/非法输入不推进。最终只将已解析的 aim/held 放入 24/64 字节 V2，正常队列/快照/查询规则不变。
- tx-01-53 原始射击 mask 只替换射击源；脚本其他已验证 canonical 字段（尤其 move_x/move_y）原样保留，不重建为 raw 后重复死区。A 真实物理 raw 移动仍正常 canonical 化一次。

## 5. control block 槽语义

正常顺序路径对齐原生调用者消费的 AL 返回及 `+0x04/+0x06` 计数镜像；永久 root 使 strong/隐含 weak 不归零，`+0x14` 不清零。完整并发镜像算法未作为本次交付；意外并发尝试只记录终止性 fault、保持内存并中性，不声称计数闭包或异常恢复已成立。

| 槽 | 原生行为（REVERSE v1） | 自有行为 |
| --- | --- | --- |
| `+0x00` deleting dtor | 销毁 critical section；flag bit0 时释放 `0x18` | 不释放；置诊断 |
| `+0x04` TryAddStrong | 加锁；strong≠0 时转 `+0x08` | 原子比较交换影子 strong：为 0 则失败，否则加一；同步镜像 |
| `+0x08` AddStrong | `++uint16 strong` | 原子加一影子 strong；超过 `0xFFFF` 则置 `PINNED`，镜像饱和为 `0xFFFF` |
| `+0x0C` ReleaseStrong | `--strong`；变零时析构并释放隐含 weak | 正常只递减 native 持有的 strong；root floor=1，尝试越过 floor 时置 fault 并中性，不清 object、不析构 |
| `+0x10` AddWeak | `++uint16 weak` | 同 `+0x08` 规则作用于 weak |
| `+0x14` ReleaseWeak | `--weak`；变零时调 `ctrl->vtable[0](1)` | 正常递减外部 weak；保留 root 对应隐含 weak floor=1，越界则 fault、中性，不释放 |

- 槽不得阻塞等待主线程、执行原生回调、析构或释放。本次计数操作用无等待的单次 owner CAS、32-bit shadow 和打包镜像；意外并发不自旋，不把它当作已安全处理的支持场景。完整任意线程并发语义留待后续实际任务。
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
| `CONTROL_READY -> REGISTERED` | 以按值 pair 调原生 insert（O-INSERT-ABI）；随后复核 key 唯一且 exact pair 解析到本对象（O-KEY） | 正常返回但复核失败则中性；仅在 exact membership 明确时走 remove。原生 OOM/EH/半发布不属于本次可恢复范围，不据旧条款盲目继续 remove |
| `REGISTERED -> RETIRING` | 关闭 admission；generation 加一；所有覆写查询输出中性；B 解绑或退出按 05-03 语法处理 | 不可失败（只写自有状态） |
| `RETIRING -> UNREGISTERED` | 调原生 remove（O-REMOVE、O-NOTIFY）；确认 vector 中不存在 | 无法确认时停在 `RETIRING`，保留 root，记录 fail-closed |
| `UNREGISTERED` 终态 | 保持 root；native vector 责任由原生消费恰一次，协调器只记账 | 不二次 ReleaseClass，不 FinalRelease，不释放 root |

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
2. 虚拟设备处于 `UNREGISTERED`，且 InputManager vector 中不存在，root 仍持有；
3. B 已解绑或退出；
4. 自有 DLL 常驻，不卸载。

## 9. 不在本契约范围内

- device key 的取值选择、assignment 时机与 join 路线细节：由 O-KEY、O-JOIN 的结果另行固定。
- tx-01-45 纳入最小 A/B V2 scheduler/脚本输入接线；完整 05-06 网络与任意 consumer 副作用证明仍按实际任务处理。
- 任何运行时权限：本契约不授权进程访问、注入、IPC、Hook 或 canary。
