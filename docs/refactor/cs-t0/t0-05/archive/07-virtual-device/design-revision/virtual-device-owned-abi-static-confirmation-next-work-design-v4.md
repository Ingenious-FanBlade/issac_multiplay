> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v4

日期：2026-10-12  
文档编写推进：`tx-01-02`  
执行推进：`tx-01-03`（一个完整目标，S01–S07 仅为内容编号）  
状态：`DESIGN_ONLY / 待执行补充静态确认 v5`  
依据：本会话 `tx-01-01` 对[静态确认 v4](virtual-device-owned-abi-static-confirmation-v4.md)、[证据 v4](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v4.md)及三个主题分册的核查。

先读[现行项目规范](../../../../../project-conventions.md)。用户会话指示优先；本设计不设 gate，不读取、计算或校验哈希，不以旧 decision、解锁字段、完成率或作者身份作为推进前置条件。用户明确要求由 **GPT-6-Luna、max** 子智能体执行本轮静态核验；这次分工不恢复独立角色、作者分离或强制子智能体制度。

## 1. 完整目标、范围与预期结果

纠正 v4 的 receiver 归属、poll 返回值的上层副作用、计数候选矛盾、manager 表身份与槽号、Mutex 重入推断及 current/previous 命名，沿实际关联路径补读证据，形成可以追溯且相互一致的静态确认 v5。保留已有支持的 allocator 栈偏移、n=33 容量反例、原型未接入生产等结论；受新事实影响时补读，不机械重做全部历史分析。

`tx-01-02` 的完整目标是交付本文与目录入口，结束时只做一次整体静态文档审阅。`tx-01-03` 的完整目标是执行全部 S01–S07、形成 v5 主报告与证据、更新导航，全部完成后只做第 7 节一次整体静态文档审阅。两者分别是设计交付与实际证据修订，不按内容编号拆分验收，也不以修改编号重复同一验收。

本轮允许资料阅读、只读 IDB/磁盘 PE 分析、静态控制流与数据流分析、手工有限交错推导及文档编辑。纯读 IDA Python 可用于读取已有字节、指令、xref、元数据和反编译结果；不得执行候选算法、线程程序、尺寸实验、模型检查器或通过/失败校验脚本。不得启动游戏、附加调试器、读取进程内存、输入、注入、Hook、IPC、构建、harness、probe、smoke、canary、旧 verifier，或 patch/rename/set_type/define_func/保存 IDB。

已采纳功能约定继续适用：自有 38 槽对象表、自有 6 槽 control 表、S2 action 覆写、`NEVER`、原生构造前固定 DLL、恢复后模块常驻、每 session 至多一次创建、路线 A 原生 Join 为主。C/S 与原版 Online/Isaac Sync 隔离；NetInputDevice 仅作静态对照。本文及 v5 中的算法、覆写与失败处置建议均为 `DESIGN_ONLY`，不自动修改或采纳[协调器契约 v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)。

不恢复全映像 native alias universe、Batch 2 retry、未来模块闭包、独立 factory/final-release-thread 工作。当前相关路径尚可静态展开的问题给出实际分析或具体未完成位置；运行期未观察值、范围外问题、候选不完整及生产未实现分别报告，不合并成“已验证”。

## 2. 核查问题与资料入口

### 2.1 会话核查的定位种子

以下地址、字节和结论来自上一轮 `tx-01-01` 会话的只读 IDA 观察及文档阅读。本文编写未新增二进制观察；执行者须复读原始指令，不把会话摘要改标为 `tx-01-03` 的直接证据。地址为 IDB VA，资料 imagebase=`0x400000`。

| 问题 | v4 位置 | 会话发现与核验要求 | 内容 |
| --- | --- | --- | --- |
| V4-Q1 receiver 误归属 | 主报告 §4 writer、§8.4 L2；主证据 R02-2/R02-5 | `sub_A1E760` 两个已知调用前为 `0xA1E728: 8D 4C 24 08`、`0xA1E889: 8D 4C 24 10`，ECX 指向栈上临时结果结构。不能因该结构也写 `+0x10/+0x14/+0x18` 就列为设备 binding writer | S01 |
| V4-Q2 poll 返回值的副作用遗漏 | 主报告 §6.4、§8.4 L3、§9 D5；R04 的 common 表 | `0xA1F2C2` memcpy、`0xA1F2FB` memset 在 `0xA1F30A` 调 slot30 前；返回 0 后 manager 经 `0xA1FC9C` 分支进入通知、`0xA1FD8E` erase、release、`0xA1FDCA` end-=8。不能仅表述为提前返回更保守 | S03 |
| V4-Q3 计数候选矛盾 | R05 §3.2、I5、I7；主报告 §10.2.3 | 操作表规定 PINNED 时 Try 失败，I7 却成功加一；I5 的 `0x10001 - 4` 应为 `0xFFFD`；I9 的示例需说明是否能由候选合法操作到达。重写并核对全部受影响交错 | S05 |
| V4-Q4 表身份与槽号未统一 | 主报告 §4/§8.2、O-KEY、F6/F7；README | F6 残留 `0xA20FB0`；Base 表的 slot30 为 `0xA211D0`，FindByKey 为 slot10/+0x28。`0x40233B: C7 05 18 7B C5 00 50 29 B8 00` 将全局 vptr 写成 `0xB82950`，其 slot4=`0xA21CB0`，再调 Base update `0xA1FC00` | S02 |
| V4-Q5 Mutex 重入被过度确认 | R03 I2/U15/§5.2；主报告 §5.3、D4 | manager Mutex 初始化尚未隔离，却借 EnterCriticalSection 推为可递归。`sub_A157F0` 在 Enter 后检查 `[CS+0x18]`，占用时 Sleep 循环；须追 manager 的实际初始化与分派，不从 API 名称推包装层可重入 | S04 |
| V4-Q6 cur/prev 命名相反 | 主证据 R02-1/R02-3；主报告 O-ABI-QUERY；R04 资源表 | 主证据将 C0/C4 标成 prev/cur，R04 标成 cur/prev。按复制、清零、poll、query 的完整顺序建立定义，统一所有表，容量成立条件不因换名而改变 | S06 |
| V4-Q7 审阅结论不支持交付事实 | 主报告状态、§10.1、§10.3；证据与导航 | “七项全部通过”“无未标注矛盾”“无界候选操作层完整”与以上内容不符。保留 v4 原结论，在 v5 明确修正依据和真实范围 | S07 |

### 2.2 必读资料

| 资料 | 用途 |
| --- | --- |
| [现行项目规范](../../../../../project-conventions.md)、[架构规划](../../../../../isaac-cs-architecture-plan.md)、[T0 任务书](../../../../isaac-cs-t0-task-plan.md)、[cs-t0 入口](../../../../../../../cs-t0/README.md) | 现行工作约定、隔离与功能边界 |
| [主报告 v4](virtual-device-owned-abi-static-confirmation-v4.md)、[主证据 v4](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v4.md) | 本轮纠错对象、16 项 O-*、F1–F8、L1–L4、D1–D9 |
| [R03 EH](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v4-r03-eh.md)、[R04 lifetime](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v4-r04-lifetime.md)、[R05 counting](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v4-r05-counting.md) | 细节、内部矛盾与未完成项 |
| [工作设计 v3](virtual-device-owned-abi-static-confirmation-next-work-design-v3.md) | 上轮实际范围与尚未满足的交付要求；错误推断不直接继承 |
| [契约 v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)、[设计修订 v1](../../../07-virtual-device/design-revision/virtual-device-owned-abi-design-revision-v1.md) | 已采纳行为；区分契约原文与后续候选草案 |
| [07-virtual-device README](../README.md)、[生命周期原型](../../../../../../../cs-t0/native/src/virtual_device_lifecycle.cpp)、[头文件](../../../../../../../cs-t0/native/include/isaac_cs_t0/virtual_device_lifecycle.hpp)、[生产工程](../../../../../../../cs-t0/native/IsaacCsT0.Native.vcxproj) | 导航、原型与生产状态；只读，不构建 |

## 3. 工具、证据与交接规则

执行者先读取项目 `isaac-cs-refactor` 技能，发现当前 IDA MCP 工具并读取实际 schema。本会话上一轮连接到 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`，input 为同名 exe，imagebase=`0x400000`；这只作定位资料。执行时记录实际路径、imagebase、PE32/i386 元数据与读取范围，不取得或校验哈希。工具不可用时报告具体缺失并继续资料分析，不宣称取得了新 IDA 证据。

只读调用 decompile/disasm/xrefs、字节/整数/元数据读取和纯读 py_eval。若工具自动返回历史哈希字段，不据此比较、认证或推进；不得调用会计算或校验哈希的分析入口。原始指令优先于伪代码变量名、IDA frame offset 和历史注释。

证据分为 `IDA_STATIC_CONFIRMED`、`STATIC_DERIVED`、`INHERITED_NOT_REREAD`、`DESIGN_ONLY`、`UNKNOWN`。每个结论记录来源、函数/指令、原始字节、IDB VA/RVA、receiver、访问宽度、条件和覆盖范围。没有运行时 moduleBase，不填 live VA；静态初始化写 vptr 不等于现场 vptr 已被观察。

主智能体完成设计后把 IDA 只读分析交给该子智能体，期间不并发操作 IDA。此前未附加调试器、未设置断点、未 patch 或保存 IDB；不检查或改变游戏进程状态。子智能体执行 S01–S07 与文档交付，不继续创建其他子智能体。工作区已有大量其他修改，避免覆盖与本任务无关的内容。

## 4. 执行内容

### S01：按实际 receiver 重建 binding writer 与 L2

1. 复读 `sub_A1E640`、`sub_A1E790`、`sub_A1E760` 的调用前后原始指令。记录 ECX 来源、局部结构跨度、后续传给 slot25/+0x64 的输出参数及使用的字段；分别标注设备 receiver 和临时输出 receiver。
2. 对该 helper 的当前 code xref 作有界 receiver 分析。两个已知调用不能支持“设备三指针被清空”；若找到其他调用，逐条判断，不将一个反例扩展为全程序不存在设备调用。
3. 重建实际设备 binding begin/end/cap writer 表：Base ctor、slot5 的 reserve/memmove、Base dtor/vector dtor及已知注册路径。删除新报告中无 receiver 依据的归属，保留真正的 Init/cleanup/common 风险。
4. 检查同一偏移在临时结构、device、control、manager、裸 vector 中是否再次被混用。重写 L2 条件，说明哪些销毁/覆写路径确实作用于自有设备，哪些尚无法隔离。

交付：`调用点 → ECX/参数来源 → 对象类别 → 访问 → 后续消费 → 对 writer/L2 的影响` 表；所有旧错误在修正表中保留位置与理由。

### S02：区分 ManagerBase、Manager 与初始化阶段

1. 复读 `0xB827F4`、`0xB82950` 的表项、vptr[-1]、COL、TypeDescriptor 和相关初始化/析构写入点。`0xB827F0` 为 Base COL 所在位置，不能作为表项 0。
2. 追 `sub_402280` 对 `0xC57B18` 的写入；按当前可定位初始化与析构阶段列出表身份。复读派生 slot4 `sub_A21CB0` 的前置调用与到 `sub_A1FC00` 的路径，不把 Base update 单独等同完整派生 update，也不推导墙钟/渲染帧就是 Authority tick。
3. 对两张表分别核对 slot4/update、slot9/GetAt、slot10/FindByKey、slot11、slot27/insert、slot28/remove、slot30/31/32 通知，给出 `slot index、byte offset、表项 VA、target`。记录哪些 target 相同、哪些不同。
4. 修正 F6 的通知 target、O-KEY/F7 的槽号，并检查主报告、主题分册与导航当前摘要的同类对齐错误。后续报告引用实际类/阶段，不写“manager vtable”而省略身份。

交付：两表对照、初始化阶段表、派生 update→Base update→common 调用链；现场 vptr 与运行线程仍分列为未观察值。

### S03：完整追踪 poll 返回 0/1 的上层消费

1. 按指令顺序展开 common 的 masks 前移/清零、`memcpy(C4←C0)`、`memset(C0)`、slot30 调用及后续采样。说明哪些访问无论 poll 返回什么都先发生，哪些只在非零分支发生。
2. 从 `0xA1FC97` 的返回值消费追两条分支。返回 0 的路径至少覆盖 manager slot31/32 通知、pair 拷贝/TryAddStrong、`sub_A21570` erase、最后 pair release 与 vector end 更新，核对是否均在 manager 锁内及是否调用 keyed remove。
3. 结合现有 EH，按这条实际路径补读必要的 `sub_A1FC00` FuncInfo/state/funclet；记录通知中断、半 erase、release 异常、锁清理与 publication 的实际范围。不只引用原生 remove 的 EH 来代表 update 内自动移除。
4. 比较 RETIRING 时 poll 返回 0/非零两个候选：对缓冲、自动通知/移除、B assignment、generation、root、coordinator 状态、重复显式 remove 的影响分别说明。没有证据时保留 UNKNOWN，不把 poll=0 写成纯内存保护或完整恢复方案。
5. 逐字核对契约 v2 §4 对返回值的原文。v4 所称“退休必须非 0”须追准确来源；若来自旧报告/工作设计而非契约，不得在契约差异表伪称原条款。

交付：common 操作顺序、返回值上层 CFG、自动移除的资源/锁/EH 表、两个退休候选的行为差异；不自动采纳候选。

### S04：manager Mutex 的实际初始化与重入条件

1. 沿 S02 的真实 manager 初始化路径定位 `manager+0x14` 包装对象的 vptr/CS 分配、初始化、Init/cleanup，以及 lock/unlock 虚分派。通过寄存器/adjusted this 追踪，不只查固定绝对地址的立即数写入模式。
2. 若 target 是 `sub_A157F0/sub_A159A0`，复读参数 `-1` 与已相关的 timed 分支、Enter/TryEnter、`[CS+0x18]` 占用标志、Sleep 循环和 unlock 的清标志次序。区分底层 Windows CS 可递归与 KAGE 包装层是否可重入。
3. 把 manager Mutex、control embedded Mutex、自有短锁分为三个同步域；同槽偏移或同 API 名称不证明实现/锁对象相同。
4. 重新解释 insert 锁内异常后的 keyed lookup、再次 insert/update、显式 remove 与自动移除是否可能再次获取残留锁。无 unwind unlock 的事实与“同线程可再进入”的推断分别列出；不能依靠未证明的重入性提出恢复步骤。

交付：初始化/分派证据、三同步域表、重入手工轨迹、锁残留后的可继续范围。无法定位实际 target 时具体标 UNKNOWN，并撤回确定的递归判断。

### S05：重写计数候选及有限交错的一致语义

1. 原生六槽基线按实际来源继承；受修改影响的返回值和 caller（尤其 manager 自动移除中的 Try/Release）补读。原生事实、候选操作和候选可执行性分别报告。
2. 统一 PINNED 后 Try/Add/Release 的规则。依据现行契约与 fail-closed 约束说明所选候选及差异；操作总表、I5/I7、契约草案与摘要采用同一规则，不能让一个表“Try 恒失败”、另一个“Try 成功”。
3. 手工复算 I1–I9 的每步前置状态、S/W、镜像、object、返回和 flags。纠正 I5 算术；I9 给出能由所分析候选合法操作到达的写入轨迹，区分“一般 packed store 不保证双 load 快照”与“当前候选的具体反例”。
4. 最后 strong 的每次 mirror store、object 清零、隐含 weak 减一和退休信号逐步列出。锁内合并不等于锁外 reader 看不到中间状态；线性化与观察者保证分别说明，不用“整体切换”遮盖两次 publish。
5. dtor/unexpectedDestroy、PINNED/flags、诊断环、state/retire_seq 与 coordinator 读取采用何种宽度、原子操作、所有权、内存顺序，均须明确。若 dtor 不取短锁，说明与其他槽并发时怎样避免覆盖粘滞位或丢信号；候选不足时标明未定义分支。
6. 对镜像发布明确实际可用的 x86 指令/原子原语和对齐条件，不用未指定的“lock 前缀 store”代表任意 MOV。分清机器指令推导、C++ 原子/非原子访问规则及外部 CRT 对齐承诺；原生 inline observer 范围不扩大成全程序闭包。
7. 保留 bounded 变体的明确缺口；无界自旋仍需讨论持有者挂起与重入、是否满足“不得阻塞等待主线程”。未定义分支或观察者协议不完整时，不因取消获取上限就宣称算法已验证完整。

交付：统一六槽/边界操作表、重写的 I1–I9（需要时增加有限轨迹）、同步参与者/可见窗口表、准确契约差异。可以得出候选仍不完整，但须说明具体分支与所需设计决定。

### S06：统一 current/previous、容量与查询语义

1. 从 common 的“前移→清 current→poll/采样”及 slot12/13/14、slot19/21 的原始访问确认 C0/C4 的时序角色。先用字段地址描述事实，再定义 current/previous，避免名字替代证据。
2. 统一 `n_cur/n_prev`、buffer writer、slot21 拷贝方向、pressed/triggered/value 描述及候选 poll 锁存次序。query value 读哪个地址、对应哪个时刻单列；不把私有 S2 快照与原生双缓冲当同一存储。
3. 保留固定 n=32／一般 n>=32 的实际构造、初始化和两缓冲覆盖条件；确认纠正名称后条件仍引用正确字段。保留 n=0/20/32/33 与实际消费者反例，说明 Init slot5 与发布后长度变化窗口。
4. 检查继承槽、中性覆写、common 的直接访问，以及 coordinator 自有状态与原生状态位的关系。把 S01 已排除的临时结构 writer 从此处的设备不变量中同步排除。

交付：地址/时序/名称对照、逐消费者访问表、唯一容量与资源不变量，以及候选的原生缓冲/私有快照边界。

### S07：整合、修正历史判断与导航

1. 建立 V4-Q1–Q7 修正表，包含旧位置/原文、直接证据、纠正或保留、影响的 O/F/L/D 与未决。v4 主报告、主证据和分册保留原文；在 v5 明确指出旧“全部通过”与“无未标注矛盾”不支持当前完整性结论。
2. 原 16 项 O-*、F1–F8、L1–L4 和 D1–D9 全部可定位。继承项引用准确文件/章节，受影响项改用 S01–S06 证据；当前尚未分析的静态位置、运行值、候选决策和未实现功能分开。
3. 对未决项说明实际影响。例如剩余 receiver、O-RC-INLINE、默认模板、callback、COL/RTTI、Join/assignment 与 worker 并发，不因有 UNKNOWN 就停止不依赖它的工作，也不把它消去以宣称全部 ABI 完成。
4. 源码/工程只读辨认原型与生产状态；不运行旧测试。真实设备生产实现、最小 Join 与退休接续仍按实际状态记录，本轮不做实现，不宣称 T0-05-05 完成。
5. 更新当前导航到 v5 结果，修正当前摘要的槽偏移、D 编号、状态及不存在的章节引用；v1–v4 和失败记录保留。所有分析、编辑完成后只执行第 7 节一次整体静态审阅，不再安排主智能体第二次独立验收。

## 5. 交付文件与记录模板

执行者在分析取得实际内容后新增：

- 同目录 `virtual-device-owned-abi-static-confirmation-v5.md`：实际工具/范围、V4-Q1–Q7、S01–S07、O/F/L 状态、契约差异、剩余工作与唯一整体审阅结果。
- `out/cs-t0/virtual-device-owned-abi-static-evidence-v5.md`：直接指令/字节、receiver、两张表与初始化、common/poll/update CFG、Mutex、cur/prev 及有限交错。必要时新增 v5 主题分册，并在主索引中指向实际章节。
- [07-virtual-device README](../README.md) 的当前入口与状态更新。

不要提前创建空报告或空证据，不覆盖 v1–v4，不改生产源码、测试、verifier、机器 decision 或权限文件。可以把纯读工具输出保存在新的 v5 证据目录，注明其生成方式，不能把文件存在视为结论正确。

```text
问题/义务：V4-Qx / Sxx / O-* / F* / L* / D*
来源：本轮工具观察 / 继承文件与章节 / 手工推导 / 候选
位置：input/IDB、imagebase、IDB VA、RVA、原始字节、读取范围
receiver：ECX 与栈参数来源、adjusted this、对象类别、后续消费
事实：访问宽度、调用次序、返回值、资源/锁/发布/异常行为
推导：具体前置条件、有限轨迹、可恢复范围及反例
设计：候选操作、观察者保证、与契约原文的真实差异
未决：未完成静态位置 / 未观察运行值 / 未采纳 / 未实现
结论：纠正 / 保留 / 条件成立 / 候选不完整 / UNKNOWN
```

## 6. 子智能体执行安排

用户已明确要求一个 GPT-6-Luna、max 子智能体。主智能体保存本文和导航后，使用 `model=gpt-6-luna`、`reasoning_effort=max`、新上下文派发 `tx-01-03`；完整范围、现行规范、定位种子与证据路径以本文交接。子智能体取得实际静态依据、编写 v5、更新导航并执行唯一整体验收；不要只复述上一轮结论。

该安排是用户指定的执行者，不是独立审计或后续实现的准入条件。主智能体不并发控制 IDA，不修改子智能体负责的新报告；执行结果以真实输出报告。若指定模型无法启动，如实说明实际工具错误，不默换模型。

## 7. tx-01-03 唯一一次整体静态文档审阅

统一入口是全部分析和编辑完成后的 **一次人工静态文档审阅**：读取 v5 主报告、主证据/实际主题分册、本文、契约 v2 与当前导航；不运行游戏、构建、程序、候选模拟或校验脚本。

| 检查 | 必须判断的内容 |
| --- | --- |
| receiver/资源 | 临时结构与设备/control/manager 分开；真实 writer 与 L2 条件一致 |
| manager/ABI | Base/派生表、初始化阶段、槽号/偏移/target 一致；实际 update 链可定位 |
| poll/EH/恢复 | 双缓冲操作先后准确；返回 0/非零的通知、erase、release、锁和异常副作用完整到已分析范围 |
| Mutex | 实际同步域与初始化有依据；重入性不由底层 API 名称代替；锁残留恢复建议有条件 |
| 计数候选 | PINNED、I1–I9 算术/可达性、mirror/object/weak 顺序、诊断与观察者协议一致；未定义分支仍写未完成 |
| cur/prev/容量 | 字段地址、时序名称、query、slot21、快照与容量条件统一 |
| 整合/状态 | O/F/L/D、修正表、导航与结论一致；历史保留；不把候选或静态分析完成等同生产/T0-05-05 完成 |

分别报告：事实修正完成情况、实际静态覆盖范围、候选算法/契约完整性、未确认证据与剩余功能。**文档明确记下某个缺口，只能说明缺口记录准确，不能让该算法/ABI 获得“验证完整”结论。** 当前交付仍有事实错误、内部矛盾或未定义分支时，给出未通过项及理由，不写“全部通过（含矛盾）”。

有依据的 UNKNOWN 与已知错误分开评价；不要求证明范围外全程序或未来模块才能交付。验收失败如实记录，禁止自动修补后重跑。后续实质修改再说明新的完整目标，不能只换编号重复验收。

## 8. tx-01-02 设计交付记录

本次只交付本文及目录入口，未新增二进制观察、未修改 v4 原报告/证据、未采纳契约修订、未运行游戏、构建或测试程序。本文以会话核查为执行种子，执行结果须由 `tx-01-03` 的实际证据给出。文档和导航全部编辑完成后做一次整体静态文档审阅，检查现行规则、范围、引用、编号、问题覆盖与交接一致性；结果在本次交付说明中报告。
