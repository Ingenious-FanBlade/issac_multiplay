# CS-T0-05-05 Virtual Device 自有 ABI 设计修订稿 v1

> 2026-10-12 执行约定更新：先读[现行项目规范](../../../../project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

日期：2026-10-10  
状态：`ADOPTED_BY_USER / DESIGN_ONLY`（采纳记录见第 9 节）  
适用目标：Win32/x86 `isaac-ng.exe`，SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`

## 0. 文档定位与边界

- 本文是独立存放的设计修订稿，不替换、不改写 `07-virtual-device/` 下任何既有记录、[decision v6](../../../../../../../cs-t0/coop-virtual-device-ownership-lifecycle-decision-v6.json)、[coordinator contract v1](../virtual-device-lifecycle-coordinator-contract-v1.md)、离线原型 v4 或权限字段。
- 证据等级为 `DESIGN_ONLY`。本文不新增任何 `STATIC_CONFIRMED`，不提升任何 decision 行，`formalDecision=null`、`workPackageStatus=BLOCKED_BY_EVIDENCE` 与全部 18 项权限保持不变。
- 本文被用户采纳前，05-05 的正式入口仍为 decision v6；采纳后也必须按 create-new 规则另行发布 contract v2、新 decision schema、TEST 预封与 REVERSE 交接（见第 7 节）。
- 第 2.3、2.4 节与附录 A 含作者在撰写本稿时对磁盘 PE 的只读观察（vtable 原始指针、少量 objdump 反汇编）。这些观察未经独立 TEST，不计证据；采纳后须由 REVERSE 角色重新出具 hash-bound 交接。
- 撰写本稿没有启动或访问游戏进程，没有使用 x32dbg、IDB 写入、注入、IPC 或 Hook。

## 1. 修订动机

### 1.1 现行证明路线的收敛问题

现行 05-05 路线要求证明“游戏中所有可能持有 virtual device `{object, control}` pair 的别名，都在 manager remove 之前被释放或 drain，并且最终释放发生在允许的线程”。这要求闭合 native alias universe：

| 版本 | receiver frontier | pair frontier | 说明 |
| --- | ---: | ---: | --- |
| v5（alias universe v2） | 38,985 | 37 | 保守裁决 |
| v6（Batch 1，当前正式） | 20,744 | 34 | 一次清除 IAT `FF 15/FF 25`、callback slot、service escape |
| Batch 2 候选（Group 1 失败） | 20,695 | 34 | 只增加 46 个 `_wassert` 调用和 3 个 switch |

Batch 1 的大幅下降来自可机械裁决的 IAT 调用。剩余 16,323 个 residual indirect call 主要是普通 C++ 虚调用；逐行证明它们不会保留 pair，实质是对整个游戏做全程序 points-to 分析，按现有批次粒度不可收敛。

另外，`NATIVE-FACTORY-CLOSURE` 的目标是“找到非 OriginalOnline 的可复用原生 factory”，但 [joint REVERSE v2](../virtual-device-ownership-lifecycle-joint-reverse-v2.md) 已确认 PE 中不存在这样的 factory。按现有定义，该项不可能闭合。

### 1.2 关键已知事实

以下事实已由既有 REVERSE 交接确认：

1. 最后一次 strong release（control slot `+0x0C`，RVA `0x000358B0`）内联执行 `object->vtable[0](1)`，即通过**对象自己的 vtable** 调用 deleting destructor（[REVERSE v1](../virtual-device-ownership-lifecycle-reverse-v1.md)，字节 `8B 4E 14 ... 6A 01 FF 10`）。
2. 最后一次 weak release（slot `+0x14`，RVA `0x0000C5C0`）通过 `ctrl->vtable[0](1)` 释放 control block。
3. `ReleaseStrong` 与 `ReleaseWeak` 的全部 xref 都是 vtable 数据引用，没有直接 code caller；实际释放全部经 control block vtable 间接分派（[joint REVERSE v2](../virtual-device-ownership-lifecycle-joint-reverse-v2.md) §4.3）。

### 1.3 核心思路

只要对象 vtable 与 control block vtable 都由我们提供，无论游戏中哪段代码、在哪个线程持有并释放别名，最终释放都只会进入我们的代码。证明问题因此从

> 全映像所有别名都在 remove 前释放，且最终释放线程受约束

改为

> 对已退休对象的迟到访问无害；对象与 control block 的内存只在主线程安全点、计数确定归零后回收，或永不回收。

后者是关于自有代码的局部性质，加上有限个原生接口的 ABI 与失败语义，不再依赖全映像别名闭包。

## 2. 已知事实基线

### 2.1 输入链（[static reverse v1](../virtual-device-static-reverse-v1.md) SC-01～SC-06）

- InputManager 单例 RVA `0x857B18`；active vector 位于 `+0x08/+0x0C/+0x10`，元素为 8 字节 `{InputDevice*, ReferenceCount*}`，mutex 位于 `+0x14`。
- 插入 RVA `0x620DF0`（mutex 下 push，必要时 grow RVA `0x621630`，解锁后调 manager vtable `+0x78` notification）。
- 移除 RVA `0x620EB0`；已知 caller：`0x506020`（Net）、`0x50A8A0`、`0x621F40`（Gamepad disconnect，经 RTTI 匹配）。
- keyed lookup RVA `0x620390`：加锁寻找首个 `device+0x0C == key` 的设备。
- `Player::SetControllerId` RVA `0x3A6450`，写 `Player+0x1618`。
- 每帧 manager update RVA `0x61FC00` → per-device common RVA `0x61F280` → 设备 vtable `+0x78` poll → logical `+0x30/+0x34`（bool，`AL`）与 `+0x38`（float，x87 `ST(0)`）。
- InputDeviceBase ctor RVA `0x61E050`：写 base vtable，并令 `device+0x0C = global[VA 0x00C7362C]` 后递增该全局。

### 2.2 shared pair 与 control block ABI（REVERSE v1）

| Offset | 含义 |
| --- | --- |
| `+0x00` | control vptr |
| `+0x04` | `uint16 strong` |
| `+0x06` | `uint16 weak` |
| `+0x08` | embedded mutex vptr |
| `+0x0C` | mutex state |
| `+0x10` | `CRITICAL_SECTION*` |
| `+0x14` | managed `InputDeviceBase*` |

六个 slot：`+0x00` deleting dtor（`0x0000CB50`）、`+0x04` TryAddStrong（`0x0000C6B0`）、`+0x08` AddStrong（`0x0000C690`）、`+0x0C` ReleaseStrong（`0x000358B0`）、`+0x10` AddWeak（`0x0000C610`）、`+0x14` ReleaseWeak（`0x0000C5C0`）。refcount base ctor RVA `0x0000CC10` 写初值 `strong=1/weak=1`。原生 wrapper RVA `0x505F60` 的顺序为：分配 `0x18` → 调 `0xCC10` → 写 vptr `0x778014` → `ctrl+0x14 = object` → 交给 insert。

原生分配函数：wrapper（RVA `0x505F9A`）与 add-player（RVA `0x2F6636`）的 `E8` 均指向 RVA `0x0060F4C0`；该函数以单个 size 参数转调 RVA `0x006648B0`（设计期观察）。它是否可能返回 null 仍未知，对应 REVERSE v1 的 `{object, null-control}` 反例。

### 2.3 InputDevice 38 槽 vtable 对照（设计期观察）

四张 vtable（MT-R06 交接列出）：Base RVA `0x782758`、Gamepad `0x782C80`、Keyboard `0x7A1678`、Net `0x778030`，各 38 槽。完整对照见附录 A，要点如下：

- **Base 是抽象类**：`+0x0C` 与 `+0x78` 指向 RVA `0x6F05D3`，即 `jmp [IAT VCRUNTIME140!_purecall]`。任何可实例化的派生类都必须实现这两个槽。
- 相对 Base，Gamepad 覆写 11 槽、Keyboard 覆写 14 槽、Net 覆写 18 槽。
- `+0x30/+0x34/+0x38` 三个 logical query 在 Base、Gamepad、Keyboard 中是同一组 Base 实现（RVA `0x61E8D0/0x61E970/0x61EA20`），只有 Net 覆写。三者都先检查 `action < (this[+0x14]-this[+0x10])>>4`，再读取状态缓冲：`+0x30` 与 `+0x38` 读 `[+0xC0]`（当前帧），`+0x34` 同时读 `[+0xC0]` 与 `[+0xC4]`（上一帧）。由此推测 `+0x34` 是边沿（triggered）查询，但尚未证明。
- per-device common `0x61F280` 每帧依次执行：把 `+0x20/+0x24`、`+0x30/+0x34` 复制到 `+0x28/+0x2C`、`+0x38/+0x3C`；`memcpy([+0xC4], [+0xC0], n*4)`；清零当前字段，并 `memset([+0xC0], 0, n*4)`；调用 poll `+0x78`，再根据 `AL` 分支。
- `+0x0C` 的三种实现：Keyboard 返回常量 VA `0x00B6D044`，Gamepad 返回 `[this+0x378]`，Net 转发给 `+0x370` 子对象的 vtable `+0x04`。推测这是名称或描述类 getter，未证明。
- Net 的 poll `+0x78` 是 `mov al,1; ret`，无条件返回 true。

### 2.4 RTTI（设计期观察）

COL RVA：Base `0x7B02F0`、Gamepad `0x7B0460`、Keyboard `0x7B191C`、Net `0x7AEF78`。REVERSE v1 记录 Gamepad disconnect `0x621F40` 通过 RTTI 与设备身份匹配后才调用 remove，因此至少有一处原生路径依赖设备的动态类型。

## 3. 修订后的设计

### 3.1 组件

| 组件 | 职责 |
| --- | --- |
| `VirtualInputDevice` | 自有对象。前 `0x370` 字节为原生 InputDeviceBase 布局，由原生 base ctor 初始化；之后为私有扩展（retire 标志、generation、snapshot 指针等） |
| 自有对象 vtable | 38 槽。默认逐槽复制已验证字节的 Base vtable，只覆写 3.4 节列出的槽 |
| 自有 COL / TypeDescriptor | 让 `dynamic_cast<InputDeviceBase*>` 成立，`dynamic_cast` 到 Gamepad、Keyboard、Net 均失败 |
| `VirtualControlBlock` | 前 `0x18` 字节与原生布局一致，vptr 指向自有 6 槽表；之后为私有扩展（32 位影子计数、pinned 标志、回收队列链接） |
| `VirtualDeviceCoordinator` | 复用离线原型 v4 的状态机与 lease 机制，按 3.8 节修订释放语义 |
| `InputSnapshotPort` | 主线程 scheduler 按 authority tick 发布 B 的不可变 action 快照；设备只读 |

### 3.2 创建事务（只在 authority 主线程执行）

| 步骤 | 动作 | 失败回滚 |
| --- | --- | --- |
| F1 | 调用原生分配函数 RVA `0x60F4C0` 分配对象（`0x370 + ext`） | 返回 null 则结束，无副作用 |
| F2 | 调用 base ctor RVA `0x61E050` 初始化基类部分（取得 device key） | 原生释放对象内存；key 计数器已递增，记录为不可复用 |
| F3 | 写入自有对象 vptr，并初始化私有扩展 | 调用 base cleanup 后释放 |
| F4 | 按原生 Keyboard/Gamepad 的创建顺序执行 Init（Base `+0x04`，RVA `0x61E210`），分配 `+0xC0/+0xC4` 双缓冲（是否需要显式调用见 O-CTOR） | 调用 base cleanup（`+0x08`，RVA `0x61E2C0`）后释放 |
| F5 | 分配 control block（`0x18 + ext`）→ 调 `0xCC10` → 写自有 ctrl vptr → `ctrl+0x14 = object`；这一个 strong 即 coordinator root | control 分配失败时绝不构造 `{object, null}` pair，直接回滚 F4–F1 |
| F6 | 以按值 pair 调原生 insert `0x620DF0`；随后 vector 持有一个 strong，并触发 `+0x78` notification | 插入异常或 notification 失败时，经原生 remove 撤出并进入退休流程 |
| F7 | 用 keyed lookup 复核：本 key 唯一，且解析到本对象 | 不唯一则立即退休并 fail-closed |
| F8 | 状态置为 `REGISTERED`，进入 assignment（3.5 节） | 见 3.8 节 |

与原生 wrapper 的区别：null control 分支被显式拒绝，所有失败点都有自有回滚，而不依赖原生 ctor 的 SEH 行为。

### 3.3 自有 control block 语义

原则：对调用方可观察的语义（返回值、`+0x04/+0x06/+0x14` 字段变化）与原生实现一致；只在三点上偏离原生行为，且偏离方向一律是“延长生命周期”或“fail-closed”。

1. **计数**：真实计数保存在扩展区的 32 位原子影子计数中。`+0x04/+0x06` 仍按原生语义维护 uint16 镜像（饱和到 `0xFFFF`），供可能存在的内联读取者使用。
2. **溢出或下溢**：影子计数越过 `0xFFFF`、或在 0 上再次 release 时，置 `PINNED`。之后对象与 control block 永不回收，coordinator 收到信号后让设备退休、B 输入归中性，并记录诊断。不发生回绕，也不发生二次释放。
3. **最后一次 strong release**：与原生一样清空 `+0x14`，并返回相同的 bool，但**不同步析构对象**：只把对象标记为 `ZOMBIE`，按回收策略入队。
4. **最后一次 weak release**：不同步释放 control block，只入队。
5. **锁与线程**：所有 slot 可在任意线程调用（原生 worker 线程会插入、移除设备，grow 时复制 pair）。实现无阻塞或只用短临界区，绝不在 slot 内执行析构、释放或回调。
6. embedded mutex（`+0x08/+0x0C/+0x10`）仍由 `0xCC10` 初始化，以兼容可能存在的内联加锁；回收时调用原生 ctrl 析构逻辑 `0xCB50`（flag=0）清理 critical section，再自行释放内存。

### 3.4 自有对象 vtable

推荐方案 S2（action 层覆写）。启动时从磁盘镜像对应的内存中读取 Base vtable 的 38 个指针，先与冻结期望值逐字节核对（不一致则 fail-closed，不创建设备），再复制并覆写以下 6 槽：

| 槽 | Base 原值 | 自有实现 |
| --- | --- | --- |
| `+0x00` deleting dtor | `0x61E1B0` | 不同步释放。标记 `ZOMBIE`、执行与回收策略一致的入队，并返回 `this`；若在 coordinator 意料之外被调用，视为 fail-closed 信号 |
| `+0x0C`（Base 为 pure） | `_purecall` | 语义确定后（O-ABI-0C）返回自有静态值，例如名称字符串 |
| `+0x78` poll（Base 为 pure） | `_purecall` | 只把当前 authority tick 的快照指针锁存到私有扩展（current/previous 两份），返回值语义见 O-ABI-78。不读硬件，不分配内存 |
| `+0x30` | `0x61E8D0` | 由锁存快照返回 action 当前状态；退休后返回 false |
| `+0x34` | `0x61E970` | 由 current/previous 快照计算边沿；退休后返回 false |
| `+0x38` | `0x61EA20` | 由锁存快照返回 float 值（x87 `ST(0)`，`ret 4`）；退休后返回 0.0 |

其余 32 槽继承 Base 实现。基类部分由原生 ctor 与 Init 正常初始化，因此这些实现操作的是合法的基类字段；per-device common `0x61F280` 对 `+0xC0/+0xC4` 的 memcpy/memset 也作用在 Init 分配的合法缓冲上。若 O-BUF 发现某个继承槽依赖“物理设备才有的状态”，则该槽改为覆写，返回中性值。

选择 Base 而不是 Gamepad 或 Keyboard 作为父型：Gamepad 专有槽（`+0x7C..+0x90`，其中 `+0x84` 在 Base 中是 `ret 8` 空实现，疑似震动类接口）与 Gamepad 的 RTTI 匹配路径（如 disconnect `0x621F40`）都不会把虚拟设备当作硬件处理。

备选方案 S1（raw 层）：只实现 poll，向 `+0xC0` 填入原始状态，由 Base 的 binding 元素函数完成 action 求值。此方案依赖全局模板向量 `0xC799E8/0xC799F8` 的 binding 语义，暂不推荐。

UI 与 gameplay 分区：查询只对 gameplay action 返回 B 的 canonical 状态；纯 UI action 恒为中性，3.5 节的 join 窗口除外。action ID 分区表见 O-ACTION。

### 3.5 Assignment 与 Join

- **路线 A（推荐）**：在有界的 join 窗口内，由虚拟设备报告 join action，让游戏原生 local co-op join 路径创建 B 并分配“触发 join 的设备”的 key。这与 05-04 用户规划假设（原生 Join 可靠创建并发布 B）一致，也让 assignment 完全走原生路径。前置义务：O-JOIN。
- **路线 B（备选）**：B 由其他途径加入后，调用 `SetControllerId(B, virtualKey, propagate)` 改绑到虚拟设备，形式上对应 05-03 live-11 观察到的原生 reassign（RVA `0x55BCA0`）。前置义务：`propagate=1` 的副作用，以及在原生 disconnect 流程之外直接调用是否安全。

### 3.6 RTTI / COL

在自有模块内构造 x86 形式的 COL（signature 0，绝对指针）：TypeDescriptor 名称为 `.?AVVirtualInputDevice@IsaacCsT0@@`，其 `type_info` vptr 取自现有 InputDevice TypeDescriptor 的 `+0`；ClassHierarchyDescriptor 的 base class array 为 `[自身, InputDeviceBase]`，后者的 BaseClassDescriptor 引用 Base COL `0x7B02F0` 中的现有描述。效果是：

- `dynamic_cast<InputDeviceBase*>` 成功；
- `dynamic_cast` 到 Gamepad、Keyboard、Net 返回 null。原生代码本来就要区分 Keyboard 与 Gamepad，预期已有 null 处理，但仍需 O-RTTI 逐点核实。

### 3.7 线程与阶段

| 操作 | 允许线程 | 约束 |
| --- | --- | --- |
| 创建、插入、assignment、退休、移除、回收 | authority 主线程 | 固定在 scheduler 提交 authority tick 之后、下一次 manager update 之前的阶段；由 coordinator thread token 强制 |
| poll / query | 任意线程 | 只读不可变快照（原子指针交换或 seqlock）；不阻塞、不分配、不调用原生函数 |
| control block slots | 任意线程 | 见 3.3 节；不析构、不释放 |

内存安全不再依赖“原生在哪个线程调用”。剩余需要确定的是 manager update 的线程与时序（O-THREAD），用于定义快照发布与 authority tick 的 exactly-once 关系；这属于 05-06 的语义问题。

### 3.8 退休、移除、回收与恢复

1. `BeginRetire`：关闭 admission，generation 递增，所有覆写查询立即返回中性值。
2. B 解绑或退出：按 05-03 的生命周期语法处理。
3. 在主线程调用原生 remove `0x620EB0`，触发 `+0x7C` callback（RVA `0x513760`，disconnect bookkeeping）。
4. 释放 coordinator root strong。
5. 若仍有原生别名持有引用，对象保持 `ZOMBIE`：内存有效，查询返回中性。
6. 回收策略（决策点 D2）：
   - `NEVER`（首个 canary 推荐）：对象与 control block 永不回收。泄漏有上界，每个 session 约一个 `0x370+ext` 对象加一个 control block。
   - `DEFERRED_MAIN_THREAD`：影子 strong 与 weak 均为 0，并再经过一个完整主循环宽限期后，在主线程回收。先调用 base cleanup，再调用原生 ctrl 析构逻辑（flag=0），最后用与 `0x60F4C0` 配对的释放函数释放内存。
7. **模块常驻**：首个虚拟设备构造前，以 `GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN)` 固定自有 DLL，使 vtable、COL 与 slot 代码在进程生命周期内始终有效。
8. **恢复语义修订**：Hook 恢复后游戏字节逐字节恢复；虚拟设备已退休、不在 vector 中，因而经 keyed lookup 不可达；但 DLL 代码常驻，不卸载。这改变了既有“完全恢复”的口径，需用户确认（决策点 D5）。
9. 进程退出时，manager stop（经 `_main` 或 atexit）释放 vector 中的 strong，由自有 slot 在任意线程安全处理；退出阶段不要求回收。

### 3.9 内存安全论证结构

- **L1（分派）**：对虚拟设备 control block 的计数修改与释放，全部经由其 vptr 分派到自有 slot。依据为 1.2 节事实 3，并由 O-RC-INLINE 补全 acquire 槽与内联副本。
- **L2（只延长）**：自有 slot 从不同步释放；内存只在主线程、在影子计数确定归零并经过宽限期后回收，或永不回收。原生语义下安全的任何执行，在本设计下对象的寿命只会更长，因此仍然安全。
- **L3（迟到访问无害）**：退休后，覆写槽返回中性值；继承槽操作的基类状态在回收前始终有效（base cleanup 推迟到回收时）。
- **L4（代码有效）**：模块常驻，因此任意时刻的 vtable 指针都指向有效代码。
- **残余风险**：计数归零后仍存在未计数的裸指针别名。原生对象同样暴露于此风险（原生会在归零时立即析构），本设计至少不劣于原生；选择 `NEVER` 策略则完全消除该风险。

## 4. 对 decision v6 中 10 项 UNKNOWN 的影响

| 项 | 现行证明义务 | 修订后义务 | 原生侧剩余工作（均为有限集合） |
| --- | --- | --- | --- |
| `OBJECT_OWNER` | 全映像别名闭包；证明 callback/reader 不留存 pair | coordinator 唯一语义 owner，加上 L1–L4 | O-RC-INLINE |
| `CREATION_DESTRUCTION` | 找到非联机原生 factory（已确认不存在） | 自有 factory，按 3.2 节逐步回滚 | O-ALLOC、O-CTOR、O-CB、O-INSERT-ABI |
| `REFERENCE_COUNTING` | 所有 caller、uint16 边界、最终释放线程 | 自有 slot、32 位影子计数、饱和后 pinned | O-RC-INLINE |
| `CALLING_THREAD` | 原生所有线程来源 | 自有生命周期操作只在主线程；读路径与线程无关 | O-THREAD（只涉及语义） |
| `VECTOR_INSERT_REMOVE_OWNERSHIP` | 移除与 drain 协议 | 只由我们调用 remove；意外移除也只产生 zombie | O-REMOVE、O-INSERT-ABI |
| `TEARDOWN_DRAIN_RESTORE` | reader/callback drain 全称证明 | zombie 加模块常驻；drain 只保障语义（退休后输出中性） | O-NOTIFY；D5 恢复口径 |
| `KEY_ALLOCATION` | key 并发、回绕、复用规则 | 基本不变；增加 F7 唯一性复核 | O-KEY |
| `VTABLE_COL_SLOT_ABI` | 38 槽完整签名、语义与全部调用点 | 只需 6 个覆写槽的 ABI，加上 32 个继承槽对本对象的安全性；COL 自建 | O-ABI-0C、O-ABI-78、O-ABI-QUERY、O-BUF、O-RTTI |
| `ASSIGNMENT_INTEGRATION` | 原子事务关系 | 路线 A 或 B | O-JOIN、O-ACTION |
| `GENERATION_LIFECYCLE` | adapter 与 join generation 的关系 | 设备 generation 等于 session；`(ownerGeneration, ordinal 1) → key` 映射 | 主要是设计工作，衔接 05-03 语法 |

## 5. 新的证明义务清单

按现行项目规范，由同一智能体完成静态分析、实现与最终验收即可，不要求 REVERSE/TEST 分离或预封。每项限定在可枚举的对象集合内，未理解项保持 `UNKNOWN`；整个推进完成后仅做一次整体验收，不为每项另设 gate 或测试。

| ID | 内容 | 有限集合与方法 |
| --- | --- | --- |
| O-ALLOC | `0x60F4C0` 与 `0x6648B0` 的 null/throw 语义、对齐，以及配对的释放函数 | 两个函数的局部 CFG |
| O-CTOR | base ctor `0x61E050` 的参数与副作用（key 计数器、成员分配、SEH）；Init `0x61E210` 的调用时机（对照 Keyboard `0x621A41`、Gamepad `0x621DF2` 的创建路径）；cleanup `0x61E2C0` 与 base dtor `0x61E1B0` 在 flag=0 时的行为 | 5 个函数加 2 条创建路径 |
| O-CB | `0xCC10` 的 ABI；`0xCB50` 在 flag=0 时的行为；`+0x08..+0x10` mutex 布局 | 2 个函数 |
| O-RC-INLINE | 6 个 ctrl slot 实现是否有直接 code caller（Release 两槽已确认为零）；全映像搜索绕过 vptr 的 `strong/weak` 内联修改，以及对 control block 的直接 `operator delete` | xref 计数加字节模式普查，命中项逐一裁决 |
| O-INSERT-ABI | insert `0x620DF0` 的参数传递（按值 pair 的布局与清理责任）、grow 异常路径、notification 调用时序 | 3 个函数 |
| O-REMOVE | remove 的 3 个已知 caller（`0x506020`、`0x50A8A0`、`0x621F40`）能否命中非 Gamepad、非 Net 的设备 | 3 条路径 |
| O-NOTIFY | insert callback `0x55B290 → 0x435C90(key)` 与 remove callback `0x513760` 的游戏行为（是否弹出“控制器断开”暂停、join 提示或自动分配） | 2 个已注册目标（joint v2 已证明列表中只有这 2 个） |
| O-ABI-0C | `+0x0C` 的签名、返回值语义与使用者 | 3 种实现加调用点 |
| O-ABI-78 | poll 返回值的含义，以及 `0x61F280` 在 true/false 两个分支后的行为 | 1 个函数 |
| O-ABI-QUERY | `+0x30/+0x34/+0x38` 的 action 参数域、pressed/triggered/value 语义，越界时的 fallback（`0xA112C0` 与元素函数指针） | 3 个函数 |
| O-BUF | MT-R06 普查中 18 处 `+0xC0/+0xC4` 命中所在的 vtable 函数：判断每个继承槽对虚拟设备是否安全 | 18 处命中 |
| O-RTTI | 对四个 InputDevice TypeDescriptor 的 `__RTDynamicCast` / `typeid` 使用点，以及每处的 null 处理 | TD xref 普查 |
| O-KEY | `0xC7362C` 计数器的并发保护；`Player+0x1618` 消费者是否对 key 取值范围有假设 | 1 个全局及其 xref，加 `+0x1618` 消费者 |
| O-JOIN | 原生 local co-op join 如何选取触发设备并取得 key；join 触发是否经 `+0x30/+0x34` 查询 | 原生 join 路径（05-01 已有静态图） |
| O-THREAD | manager update（callsite `0x554F7D`）所在线程与主循环阶段 | 1 条调用链 |
| O-ACTION | canonical 24B 状态到 action ID 的映射；UI 与 gameplay 的分区 | T0-04 映射加 action 表 |

## 6. 对现有工作的处置建议

- **Native alias universe**：采纳后转为非阻断的背景证据；v6 正式结果与 Batch 2 失败记录原样保留，暂停后续 Batch；Batch 2 retry 降为可选。
- **`NATIVE-FACTORY-CLOSURE`**：由 O-ALLOC、O-CTOR、O-CB、O-INSERT-ABI 替代。
- **`NATIVE-RELEASE-THREAD-CLOSURE`**：由 O-RC-INLINE 与 L2 替代。
- **离线原型 v4**：复用七态状态机与 lease 机制。需要扩展 `ZOMBIE`、`PINNED` 子状态、延迟回收、影子计数与模块常驻，作为 create-new 的离线 v5。
- **研究计划 §10 的兼容性**：§10 要求证明 owner、分配与释放、调用线程、vtable/COL/slot/ABI 等。本修订仍然要证明这些项，只是证明对象变为“自有实现加有限原生接口”。§10 的停止条件“需要伪造未理解的 vtable/COL”针对的是猜测未知 ABI；本设计在 O-ABI-*、O-BUF、O-RTTI 闭合之前不构造任何 vtable 或 COL，因此不违反该条件。是否接受此解释由用户裁决（决策点 D1）。
- **不受影响的部分**：05-04 严格状态，以及“runtime canary 需 05-04 正式门通过”（[next-step v1](../virtual-device-native-evidence-closure-next-step-v1.md) §5）。本稿不处理这两项，它们是独立的决策。

## 7. 采纳流程（create-new）

1. 第 8 节 D1–D6 已由用户采纳，按第 9 节记录执行。
2. 每次推进最小为 `tx-0x-0x`，明确整体目标；不创建新 gate、decision 认证链或哈希绑定要求。
3. 同一智能体可阅读并分析 O-*，完成契约与实现，记录直接观察、推导和 `UNKNOWN`；探讨与修改期间不运行测试。
4. 若该次目标包含离线 harness，实现覆盖 zombie、pinned、影子计数饱和、迟到调用与模块常驻等所需行为，构造后按 `NEVER` 不回收。
5. 完成整个目标后只通过一个统一入口执行一次整体验收，不执行预封、四组测试、verify-existing、额外 mutation 或重复验收。

实机、调试器、注入与 runtime 按用户当前授权范围执行，不以旧证据门或哈希检查作为前置。历史失败与未决事项如实保留。

## 8. 待用户决策

| ID | 决策 | 推荐 |
| --- | --- | --- |
| D1 | 采纳“自有对象 vtable 加自有 control block”路线，以第 5 节清单替代 alias universe 全映像闭包 | 采纳 |
| D2 | 回收策略：`NEVER` 或 `DEFERRED_MAIN_THREAD` | 首个 canary 用 `NEVER`，之后再评估 |
| D3 | 覆写层级：S2（action 层）或 S1（raw 层） | S2 |
| D4 | Assignment：路线 A（虚拟设备触发原生 join）或路线 B（直接 `SetControllerId`） | A，B 作为备选 |
| D5 | 恢复口径：首个虚拟设备构造后 DLL 永久常驻 | 接受 |
| D6 | Batch 2 retry 是否继续 | 暂停，作为可选项 |

## 附录 A：InputDevice 38 槽对照（设计期只读观察）

读取方法：读取磁盘 PE（SHA-256 已核对），每个 vtable 项减去 image base `0x400000` 得到 RVA；`same` 表示与 Base 同一目标。`[-1]` 为 COL：Base `0x7B02F0`、Gamepad `0x7B0460`、Keyboard `0x7B191C`、Net `0x7AEF78`。

| 槽 | 偏移 | Base | Gamepad | Keyboard | Net | 备注 |
| ---: | --- | --- | --- | --- | --- | --- |
| 0 | `+0x00` | `0061E1B0` | `006220F0` | `0066C390` | `005017C0` | deleting dtor |
| 1 | `+0x04` | `0061E210` | `00622190` | `0066C3E0` | same | Base 为 Init（MT-R06） |
| 2 | `+0x08` | `0061E2C0` | `00622430` | same | same | Base 为 cleanup（MT-R06） |
| 3 | `+0x0C` | `006F05D3` | `006224A0` | `0066C440` | `00501470` | Base 为 `_purecall` |
| 4 | `+0x10` | `0061E360` | `006224B0` | `0066C450` | same | |
| 5 | `+0x14` | `0061E430` | same | `0066C480` | same | 模板 tuple 填充（MT-R06） |
| 6 | `+0x18` | `0061E490` | same | same | same | |
| 7 | `+0x1C` | `0061E4B0` | same | `0066C4E0` | `00501560` | |
| 8 | `+0x20` | `0061E590` | same | same | `00501580` | |
| 9 | `+0x24` | `0061E610` | same | `0066C510` | `0002CB80` | Net：`xor al,al; ret 4` |
| 10 | `+0x28` | `0061E640` | same | `0066C540` | `0002CB80` | 同上 |
| 11 | `+0x2C` | `0061E790` | same | `0066C580` | `005015A0` | |
| 12 | `+0x30` | `0061E8D0` | same | same | `00501480` | logical bool，读 `+0xC0` |
| 13 | `+0x34` | `0061E970` | same | same | `005014C0` | logical bool，读 `+0xC0/+0xC4` |
| 14 | `+0x38` | `0061EA20` | same | same | `005026A0` | logical float，读 `+0xC0` |
| 15 | `+0x3C` | `0061EAD0` | same | `0066C5C0` | same | |
| 16 | `+0x40` | `0061EBE0` | same | same | `0002BC90` | Net：`ret 8` |
| 17 | `+0x44` | `0061EC20` | same | same | `0002BC90` | 同上 |
| 18 | `+0x48` | `0061ED10` | same | same | `00501500` | |
| 19 | `+0x4C` | `0061ED50` | same | same | `00502850` | |
| 20 | `+0x50` | `0061ED80` | same | `0066C5F0` | `0002CDF0` | |
| 21 | `+0x54` | `0061EE10` | same | same | `00501530` | |
| 22 | `+0x58` | `0061EE90` | same | same | same | |
| 23 | `+0x5C` | `0061EEB0` | same | same | same | |
| 24 | `+0x60` | `0061EFA0` | same | same | same | |
| 25 | `+0x64` | `0061F0E0` | same | same | same | |
| 26 | `+0x68` | `0061F160` | same | same | same | |
| 27 | `+0x6C` | `0061F1F0` | same | same | same | |
| 28 | `+0x70` | `0002C160` | same | `0066C610` | same | Base：`xor eax,eax; ret 4` |
| 29 | `+0x74` | `0002C160` | same | `0066C630` | same | 同上 |
| 30 | `+0x78` | `006F05D3` | `00622940` | `0066C650` | `00309C30` | poll；Base 为 `_purecall`，Net 为 `mov al,1; ret` |
| 31 | `+0x7C` | `0061F720` | `00622B80` | same | same | |
| 32 | `+0x80` | `00309C30` | `001CCC00` | same | same | Base：返回 true；Gamepad：`xor al,al; ret` |
| 33 | `+0x84` | `0002BC90` | `00622C80` | same | `00502930` | Base：`ret 8` 空实现 |
| 34 | `+0x88` | `0061F740` | same | same | same | |
| 35 | `+0x8C` | `0061F8F0` | `00622D20` | same | same | |
| 36 | `+0x90` | `0061F910` | `00622D80` | same | same | |
| 37 | `+0x94` | `0061F930` | same | same | same | |

覆写计数：Gamepad 11 槽（0–4、30–33、35、36），Keyboard 14 槽（0、1、3、4、5、7、9、10、11、15、20、28、29、30），Net 18 槽（0、3、7–14、16–21、30、33）。

## 9. 采纳记录

- 2026-10-10，用户确认第 8 节 D1–D6 全部按推荐执行：
  - D1：采纳“自有对象 vtable 加自有 control block”路线；
  - D2：回收策略为 `NEVER`；
  - D3：S2（action 层覆写）；
  - D4：路线 A 为主，路线 B 为备选；
  - D5：接受首个虚拟设备构造后 DLL 永久常驻的恢复口径；
  - D6：暂停 Batch 2 retry，作为可选项。
- 用户要求不在 `07-virtual-device/README.md` 或其他既有索引中加入指向本目录的条目；本目录自成入口。
- 用户要求只完成必要测试，不运行全量测试。
- 2026-10-12，用户进一步修订项目约定：永不设定 gate、不校验哈希；每次推进最小为 `tx-0x-0x`，仅一次最终整体验收，探讨过程不测试；取消逆向与测试必须独立的要求。旧采纳流程和历史解锁字段不再控制后续推进。
- 采纳本身不改变任何证据标签：decision v6 仍为正式入口，`formalDecision=null`、`workPackageStatus=BLOCKED_BY_EVIDENCE`，全部权限与解锁字段不变。后续工作见[协调器契约 v2](virtual-device-owned-abi-coordinator-contract-v2.md)。
