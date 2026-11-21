> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v2

日期：2026-10-12  
文档编写推进：`tx-00-05`  
状态：`DESIGN_ONLY / 待执行补充静态确认`  
依据：对[静态确认报告 v2](virtual-device-owned-abi-static-confirmation-v2.md)的 `tx-00-04` 会话静态核查。

先读[现行项目规范](../../../../../project-conventions.md)。用户会话指示优先。本工作不设 gate、不校验哈希，不把旧 decision、解锁字段、作者身份或矩阵完成率作为推进前置条件；同一智能体可完成分析、整理和最终审阅，不要求子智能体。

## 1. 整体目标、范围与执行约定

修正 v2 中注册回调读写方向、分配溢出和 binding 容量的事实或约束错误，补充 cleanup 生命周期、异常 unwind 与影子计数/镜像一致性分析，形成可按明确条件使用的静态确认 v3。保留 v2 的已支持结论、原始记录和历史审阅结果，在 v3 逐条说明修正依据和仍未解决的问题。

建议后续补充静态确认使用一个整体推进 `tx-00-06`。执行者开始时记录实际推进编号和完整目标。本文 K01–K07 只是内容编号，不是多个推进或多个验收入口；整个目标完成后只做一次整体静态文档审阅，分析、逆向与修订期间不运行测试。

本文是供执行使用的设计，本次编写不执行补充二进制分析。后续范围仅包含资料阅读、只读 IDB/磁盘 PE 分析、静态控制流/数据流推导与结果文档编辑。工具使用方式沿用[工作设计 v1 §4](virtual-device-owned-abi-static-confirmation-next-work-design-v1.md)，以实际提供的工具 schema 为准。

只读提取指令、字节、xref、EH 元数据属于分析；不得运行游戏、附加调试器、读取进程内存、输入、注入、Hook、IPC、故障注入、构建、harness、probe、smoke、canary、旧 builder/verifier 或其他测试/校验脚本，不修改或保存 IDB。工具不足时如实记录，继续不依赖该工具的工作。

延续现行设计：自有对象 vtable 与自有 control block、S2 action 覆写、`NEVER`、构造前固定 DLL、恢复后模块常驻、每 session 至多一次创建、路线 A 为主且 B 为备选。C/S 与原版 Online/Isaac Sync 的隔离、统一输入语义、generation 与实际操作授权边界继续适用。NetInputDevice 只作静态对照，不作为 C/S 组件。

不恢复 native alias universe 全映像闭包、独立 factory、最终释放线程或 Batch 2 旧流程；不要求静态分析穷尽未来模块与任意裸指针别名。分析结果应明确覆盖范围与最小未决问题。

## 2. 本次核查问题与资料入口

### 2.1 `tx-00-04` 核查摘要

以下是上一轮会话核查的摘要与定位种子，不是本次编写新增的二进制证据。后续执行须直接读取有关指令，给出 v3 的可追溯依据。

| 问题 | v2 位置 | 核查发现及影响 | 工作项 |
| --- | --- | --- | --- |
| 注册回调方向/字段错误 | 主报告 §4.5；W06/W07 草稿的 callback 副作用 | 同名分支将游戏侧记录写回设备 binding；新条目经 `sub_835BA0` 读取设备。binding 位于对象 `+0x10` vector，`+0xC0` 是 float 状态缓冲，不能混用 | K01 |
| 分配溢出结论错误 | §3.1、§3.3；主证据 W02 | `malloc` 只接收 `size+4` 的低 32 位；`0xFFFFFFFF+4` 回绕为 3。`mul` 的 OF 不是有符号乘法溢出；“不会分配小缓冲”不能成立 | K02 |
| 注册/Join 容量条件不统一 | §4.5、§10.6、§11.4 | 注册访问 32 个 action，Join flush 访问 action19；只保证 `n>19` 或覆写 slot21 不能保护注册路径。空 vector 建议同样需要重新说明 | K03 |
| `NEVER` 与缓冲 lifetime 推理不足 | §5.1、§5.4、§11.2 | 对象/control 不释放，不自动保证 cleanup、重复 Init 或直接 Base dtor 不破坏基类状态。“不主动调用”不是对原生调用的约束 | K04 |
| 异常状态未完整展开 | §8.3、§8.7、§12；W06/W07 草稿 U-5/U-6 | insert/grow 的 EH 状态、部分构造清理与锁恢复仍需局部静态分析 | K05 |
| shadow/mirror 一致性未完整分析 | §6.5；工作设计 v1 W05 第 5 项 | 即使没有原生 inline reader，自有并发写也可能把旧 mirror 覆盖回去；不能仅称其“短暂过时” | K06 |
| 摘要、笔误与审阅结论不一致 | §4.1 第 173 行；证据索引开头；§13 | purecall 应为 slot3/30，slot0 是 deleting dtor；索引状态仍写“W04–W10 待补”；旧“全部通过”不能替代本次实际审阅 | K07 |

### 2.2 资料

| 入口 | 用途 |
| --- | --- |
| [现行项目规范](../../../../../project-conventions.md) | 唯一整体验收、事实分类、`NEVER`、线程与恢复要求 |
| [静态确认 v2](virtual-device-owned-abi-static-confirmation-v2.md) | 主报告、F1–F8、覆写建议、16 项 O-* 状态和原审阅结论 |
| [v2 主证据索引](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2.md) | W01–W05 指令与扫描记录、补充内容的实际位置 |
| [v2 W06/W07 证据](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w0607-draft.md) | 通知、pair、EH、锁与线程资料 |
| [v2 W08 证据](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w08-draft.md) | COL/RTTI 的已有范围与未决消费 |
| [v2 W09 证据](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w09-draft.md) | Join action、selector、slot21 与身份关系 |
| [工作设计 v1](virtual-device-owned-abi-static-confirmation-next-work-design-v1.md) | 原 16 项 O-*、证据模板和既有工作边界；本轮具体执行安排以本文为准 |
| [设计修订稿 v1](../../../07-virtual-device/design-revision/virtual-device-owned-abi-design-revision-v1.md)、[协调器契约 v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md) | 当前布局、六槽基线、状态机、L1–L4 与功能约定 |

旧记录的哈希、测试命令、独立角色和准入条款只作历史资料。后续不运行其验证流程，不改写旧失败或旧审阅结果。v3 将“原报告曾判通过”与“本轮纠正结果”并列记录。

## 3. 分析对象、地址与证据规则

执行时只读记录实际 input/IDB 路径、imagebase、PE Machine/section/SizeOfImage，以及关键函数和数据位置；不计算或核对哈希。`IDB VA = IDB imagebase + RVA`；沿用资料的 base=`0x00400000` 只是种子，执行时读取实际值。没有 live moduleBase，不填运行时 VA。

| 定位种子 | IDB VA（资料 base=`0x400000`） | RVA |
| --- | --- | --- |
| 注册列表消费 / 新条目读取 helper | `0x835C90 / 0x835BA0` | `0x435C90 / 0x435BA0` |
| 同名分支写回指令 | `0x835EB7` | `0x435EB7` |
| allocator / 下层分配 | `0xA0F4C0 / 0xA648B0` | `0x60F4C0 / 0x6648B0` |
| 加 4 / 调 malloc | `0xA6492D / 0xA64934` | `0x66492D / 0x664934` |
| Base ctor / Init / slot5 | `0xA1E050 / 0xA1E210 / 0xA1E430` | `0x61E050 / 0x61E210 / 0x61E430` |
| cleanup / Base deleting dtor | `0xA1E2C0 / 0xA1E1B0` | `0x61E2C0 / 0x61E1B0` |
| common / Join flush slot21 | `0xA1F280 / 0xA1EE10` | `0x61F280 / 0x61EE10` |
| insert / grow / erase | `0xA20DF0 / 0xA21630 / 0xA21570` | `0x620DF0 / 0x621630 / 0x621570` |
| 已有 EH 元数据 / funclet 种子 | `0xBD21A8 / 0xB11650 / 0xA217CE` | `0x7D21A8 / 0x711650 / 0x6217CE` |
| refcount ctor / ReleaseStrong / ReleaseWeak | `0x40CC10 / 0x4358B0 / 0x40C5C0` | `0xCC10 / 0x358B0 / 0xC5C0` |
| Base vtable | `0xB82758` | `0x782758` |

每条结论分开写直接事实、控制流推导、设计建议与未决问题；使用 `IDA_STATIC_CONFIRMED`、`STATIC_CONFIRMED`、`STATIC_DERIVED`、`DESIGN_ONLY`、`UNKNOWN`。继承 v2 的内容标“引用 v2，未在本轮重读”，不能整体标成本轮直接确认。静态上仍可展开但本轮未展开的分支，与静态无法观察的运行期值分开列出。

对关键 ABI、整数宽度、读写方向、EH action 和锁恢复，保存原始指令/字节与数据解释；伪代码是辅助。扫描计数附范围、筛选条件、命中与分类，不能用“表有 16 行”“xref 为零”代替真实分析。

## 4. 内容安排与输出

建议按 K01/K02 → K03/K04 → K05/K06 → K07 整理；先后仅表达资料依赖。遇到未决项时写明原因，继续不依赖它的分析。

| 工作项 | 对应 O-* | 必须形成的结果 |
| --- | --- | --- |
| K01 注册回调读写方向 | O-NOTIFY、O-BUF、O-INSERT-ABI | 分支数据流、布局、binding writer 与副作用表 |
| K02 分配整数语义 | O-ALLOC、O-CTOR、O-CB | 位宽/回绕公式、Init 可达条件、实际失败 CFG |
| K03 容量与创建流程 | O-BUF、O-ABI-QUERY、O-JOIN、O-ACTION | 统一容量条件、初始化布局、F4–F6 与 slot21 建议 |
| K04 基类资源 lifetime | O-CTOR、O-BUF、O-REMOVE、O-THREAD | 原生 cleanup/Init/dtor 路径、退休约束和 L2/L3 修订 |
| K05 异常 unwind | O-INSERT-ABI、O-REMOVE、O-NOTIFY、O-CTOR | EH state/资源清理/锁恢复表、不可恢复出口 |
| K06 shadow/mirror 并发 | O-CB、O-RC-INLINE、O-THREAD | 线性化点、并发反例、同步建议与槽返回语义 |
| K07 统一修订与交付 | 原全部 16 项 O-*；L1–L4 | v3 主报告、证据、契约差异与唯一整体审阅 |

### K01：注册列表的新条目与同名条目数据流

范围：`sub_A20DF0 → sub_A211D0 → sub_95B290 → sub_835C90`，并展开 `sub_835BA0` 与两条记录分支直接涉及的分配/append 路径。

执行步骤：

1. 从 `key → lookup pair → object` 恢复指针来源；标清 device 的 binding vector `+0x10/+0x14/+0x18` 与状态缓冲 `+0xC0/+0xC4`。不得再把 `+0xC0` 写成 binding vector。
2. 新名称/空列表分支：追踪 `sub_835BA0` 的每条 load/store，确认从设备读取哪两个 key、写入记录哪个位置、遍历上限及查不到设备时的行为。
3. 同名分支：追踪 `0x835E58..0x835EDD`，确认 `0x835EB7` 的源值、目标地址、index/stride；分别说明日志、继续写入和范围不足的后果。
4. 区分访问跨度、实际复制字节数与 vector 的逻辑元素数。资料显示“32 action、每 action 16B、每次仅两个 dword”，不能概括为连续复制 0x200B；以原始循环复核。
5. 列每个分支对设备 binding、游戏侧列表、名称存储、临时 pair/control 的修改和 lifetime；说明同名恢复会否覆盖自有 binding，以及每 action 其余 key 项是否保留。
6. 不仅统计字段固定偏移；对 `this+0x10`、vector begin、adjusted pointer 与 helper 参数追踪读写。区分 binding 元素 writer、vector begin/end/cap writer 和状态缓冲 writer。

输出分支表：`触发条件 → source → destination → index/stride/实际宽度 → bound → 修改状态 → 失败出口`，并更新 O-NOTIFY/O-BUF 的相关结论。Name 唯一性不得当作跳过 32 项读取的理由：新名称 helper 本身也需分析。

### K02：分配器位宽、回绕与 Init 失败 CFG

必须用反汇编解释 `sub_A648B0` 的 64-bit 输入、CL/EDX 寄存器参数、`add/adc`、传给 Win32 `malloc` 的单个 32-bit size、4B header 写入和 `ptr+4` 返回；把内存统计的 64-bit 值与实际 malloc 请求区分开。

1. 分别记录 high dword 非零时的截断、低 dword 加 4 时的回绕、malloc 返回零/非零、handler 不返回或抛出，以及释放 `ptr-4` 的契约。
2. 逐位推导以下静态边界，不运行脚本/实验：低位 `0xFFFFFFFB`、`0xFFFFFFFC`、`0xFFFFFFFD`、`0xFFFFFFFE`、`0xFFFFFFFF`，以及 high dword 非零。对 malloc(0) 的返回行为只使用可读的当前实现依据，无法确认则保留 `UNKNOWN`。
3. 特别保留 `size=0xFFFFFFFF → 低 32 位 malloc 参数为 3` 的控制流反例。分析小请求成功时的 header 与返回 payload 是否满足请求，不能继续写“约 4GiB 必然失败”或“不会得到小缓冲”。
4. Init 的 `mul` 是无符号乘；CF/OF 的含义以高半部是否非零说明。随后 `seto/neg/or` 如何构造 size、memset 如何重新计算长度，逐条记录。
5. 推导正常对象的 `begin/end/stride` 是否能使乘法溢出分支可达；畸形 vector 与正常已初始化对象分别说明。发现原语边界缺陷不等于正常 n 必然触发该缺陷。
6. 重写 n=0、n>0、两次分配失败、header size 回绕、长度不一致、slot5 grow、CS 分配、异常与重复 Init 的失败表。n=0 的 malloc(4) 也可能失败，不能把“长度为零”写成“分配一定成功”。
7. 原生 malloc 失败会调用 Memory Manager handler，不能不经分析就称 F1 “无副作用”；区分自有世界未发布、原生诊断/回调副作用和未知 handler 行为。

输出精确整数公式、正常布局前提、失败 CFG 与 F1/F4/F5 影响。推荐受检查的自有尺寸计算、分配或 Init 替代方案时，须标为设计建议并说明与当前契约的差异，不直接实现或自动采纳。

### K03：统一 binding 容量与 F4–F6/Join 条件

以 K01 的真实访问集合为基础，统一注册、Join、query 与 common 的长度要求。

1. 对每个消费者列逻辑 n、vector capacity、实际初始化元素数、cur/prev 分配元素数、最大 index 和读/写方向；禁止把“reserve 了空间”当成元素已初始化。
2. 复核新条目与同名条目注册都遍历 action `0..31`；在保留该原生注册通知路径的设计中，明确 F6 之前的 binding 容量与初始化条件。
3. 复核 Join `+0x54(19)` 的 cur/prev 访问，以及 action14/15/20/21 的查询域。slot21 覆写只能处理该槽路径，不能保护注册 helper 的直接 binding 访问。
4. 保留静态反例：n=20 满足 Join action19 的局部条件，却不满足注册 32 项访问；n=0 且只覆写 slot21 仍不能保护注册。均只作推导，不执行用例。
5. 在基类 vector 采用 n≥32 时，说明 32 个合法的 16B binding 元素、相同 n 的两份 float 缓冲、其余字段与 raw/UI/action 查询的中性条件。仅把 end 改为 begin+0x200 不构成有效初始化。
6. 若希望采用小 n/空 vector，必须具体说明如何处理原生注册读写；仅新增对象 query/slot21 覆写、名称避重或“协调器不调用”均不能替代这项说明。改变 notification 路径属于额外设计，不在本轮安装 Hook。
7. 追踪同名写回与后续 UI binding 编辑的作用，说明它们是否改变长度、元素、状态缓冲或原生 fallback 的输入；同步修订 writer 表和资源不变量。

输出一个统一条件表供 F4/F5/F6、Join 与覆写集合共同引用，不在多个摘要中保留不同容量要求。优先评估完整初始化的 32 项 binding 方案；若其实现与 native Init 约定冲突，提出契约修订草案并保留未决项。

### K04：cleanup、重复 Init、dtor 与有效基类状态

对象不释放、指针仍存在、缓冲仍有效和迟到访问无害是不同命题，分别给出证据。

1. 对 slot1/2/5、Base deleting dtor、vector dtor、common、slot7/8/19/21/34 建立资源表：哪些字段/缓冲/记录区会被写、重分配、清零或释放。
2. 在已知 InputDevice 构造/析构、manager、通知、UI/配置和 Join 路径中查直接 call、tail call 与相关 vtable 间接调用；对每个 caller 判断 receiver 能否是自有对象、需要哪些 RTTI/状态条件。
3. 未隔离到间接 receiver 的调用写为 `UNKNOWN`，不能用“没有 code xref”“自有代码不导出”“协调器不主动调用”说明原生不可达。
4. 用有限静态反例说明：cleanup 释放 C0/C4 后对象仍常驻；随后 common 按非零 n 使用缓冲仍可能失效。重复 Init 的旧缓冲、vector 变长与 Base dtor 写回原生 vptr同样分析。
5. 比较覆写 slot1/2/5、只允许受控创建调用、中性覆写 slot7/8，以及限制具体 native caller 的方案。对象 slot0 覆写不能拦截直接 Base dtor；覆写 slot2 也不能拦截直接 cleanup，必须各自说明剩余旁路。
6. 对无效状态下的相关查询和 common 访问说明可行处置；原生路径无安全失败出口时，只记录不可恢复性，不能在文档中虚构继续写世界或盲目恢复的行为。

输出生命周期调用表、资源不变量和 L2/L3 的有条件结论。未解决原生释放路径时，不得写“NEVER 所以不会出现 use-after-free”。修改覆写集合/调用机制只作为契约建议。

### K05：insert/grow 的 EH 状态与锁恢复

范围限定为 insert、grow、erase、pair copy/release、notification 及其与当前对象有关的清理 action。以 `FuncInfo 0xBD21A8`、`0xB11650`、`0xA217CE` 为定位种子，实际确认所属函数和状态。

1. 读取 x86 `__CxxFrameHandler3` 相关 FuncInfo、unwind map、try/handler map与构造状态变量；反汇编所有与该局部事务有关的 funclet。不能由第一个 action 代表整张 unwind map。
2. 将每个 state 转换对应到：manager lock 持有、root/传入 pair、vector 旧/新 buffer、部分新 range、temporary pair、notification 的执行阶段。
3. 分析 `_Xlength_error`、`bad_array_new_length`、Memory Manager handler 抛出、TryAddStrong 失败、allocator null、noreturn 与 callback 异常。区别正常返回失败、C++ 异常、访问异常和进程终止；不假定 AV 会落入 C++ unwind。
4. 对每条相关退出路径记录 unwind 后哪些 pair 被释放、buffer 是否仍有效、vector 是否已发布、manager mutex 是否 unlock、coordinator 能否合法继续或 remove。
5. callback 已执行后的列表/binding 副作用不一定被 vector unwind 撤销；将 K01 的状态修改纳入异常事务分析。
6. 若某 funclet、handler 或 AV 处理规则不可解析，明确列其地址和缺少的静态依据；不得把“未展开”改写为只能实机观察。

输出表：`state/位置 → 资源与锁 → 触发出口 → unwind action/顺序 → 发布与副作用状态 → 可恢复范围/UNKNOWN`。只有覆盖到具体状态的依据才支持“会 unlock”或“不会 unlock”；不给异常后自动重试建议，不进行 fault injection。

### K06：影子计数、uint16 镜像与 object 指针的一致性

这项包含原生事实和自有算法设计，分列标注。当前没有已实现的自有 control 算法，不能把设计论证写成二进制观察或离线测试结果。

1. 沿用或重读六槽原生返回分支、embedded Mutex/CS、`+4/+6/+0x14` 更新顺序，确认 TryAddStrong、AddStrong/AddWeak、最后释放、零上 release 与 pinned 的可观察语义。
2. 为候选自有算法给出操作次序与线性化点：shadow 原子更新、mirror 发布、object 清零、退休信号、内部 weak release。仅写“CAS + 同步镜像”不足以分析多个槽交错。
3. 保留有限交错反例：T1 shadow 增至 2 后暂停；T2 增至 3 并写 mirror=3；T1 恢复后写 mirror=2。没有后续操作时旧镜像可能一直保留，不能仅称“短暂过时”。此为手工静态推导，不运行线程程序或模型检查器。
4. 分别分析 Add/Release、两个 Release、TryAddStrong 与最后 Release、strong/weak 各自更新、零计数、`0xFFFF` 饱和、32-bit shadow 自身溢出，以及 pinned 后允许的操作。
5. 区分“自有槽之间串行化”和“可能的原生 inline reader/writer 与自有槽同步”。保留原生 CS 指针不等于发生同步，原子 mirror store 也不自动保证它与 shadow 的对应。
6. 比较短临界区、版本化/有序发布或其他具体方案，说明返回值、饱和规则、锁顺序和与 manager 锁的关系；不能阻塞等待主线程，不在 control 槽执行回调、析构或释放，不反向获取 manager 锁。
7. 若建议改用原生 embedded mutex 同步或改变镜像/清零契约，写明与协调器契约 v2“自有槽不依赖它”的差异；不要自动标为已采纳。

交付至少一个带完整操作次序的候选算法及有限并发推导，或明确说明尚无法给出满足当前约定的算法。内存不回收不能作为计数、mirror、object 指针或输入状态一致性的证明。

### K07：报告、契约与审阅口径收敛

1. 逐条对应 v2 的错误/未决项：原位置、原结论、v3 事实、修正原因、受影响的 O-*、F1–F8、L1–L4 与引用。对 v2 状态表按真实范围重新表述，不把局部确认标成整项无条件完成。
2. 修正 purecall 槽编号，并明确区分 slot0 生命周期、slot3 稳定名称、slot30 存活信号和 query 中性；退休不意味着所有返回值均为零。
3. 整合 K01–K06 的约束，使 ABI/字段表、容量表、writer 表、覆写集合、F1–F8、EH 失败行为和 L1–L4 不互相矛盾。
4. 完整保留原 16 项 O-* 索引：受影响项引用新证据，其余项明确引用 v2 的原范围与残余，不要求机械重复原有全部扫描。
5. 新证据索引的开头状态、主题文件状态和主报告结论一致；已整合的草稿标明其适用范围。v2 的“进行中/完成”冲突作为历史纠错记录，不删除原文件。
6. 保存 v2 §13 的历史“通过”，v3 明确注明 `tx-00-04` 核查发现哪些错误，以及本轮纠正范围。不能直接复制旧“全部通过”。
7. 全部分析、整理与必要的文档修订完成后，执行 §7 一次整体静态审阅；若发现未处理的问题，如实记录结果，不自动再次验收。

## 5. 交付与证据模板

后续执行建议新增：

- 同目录 `virtual-device-owned-abi-static-confirmation-v3.md`：实际目标/工具、v2 修正表、K01–K07 结论、原 16 项 O-* 的引用范围、统一容量与资源/异常/计数约束、契约差异、残余及整体审阅结果。
- `out/cs-t0/virtual-device-owned-abi-static-evidence-v3.md`：原始指令/字节、数据流、整数公式、EH state 表、有限并发推导与主题索引。必要时按主题拆分，但主报告须引用实际存在的文件与章节。
- v3 主报告内的协调器契约修订草案：逐条给出旧条款、建议条款、依据、行为变化与未决项。若后续实际修订独立契约，保留契约 v2，不把候选建议冒充当前已采纳行为。
- 07-virtual-device 导航更新：指出新结果与仍未解决的问题，保留历史入口。

本文编写只产出工作设计 v2 与导航更新，不创建空的确认 v3、测试计划、认证 manifest 或 verifier。

每条关键结论记录：

```text
问题 ID：Kxx / O-xxx / 对应 v2 章节
来源：实际工具、input/IDB 路径、函数/数据/原报告
位置：RVA、IDB VA、imagebase、读取范围和筛选条件
原始依据：指令/字节/元数据；必要时附伪代码
语义：真实 source/destination、整数宽度、参数、返回、资源/锁
标签：直接事实 / STATIC_DERIVED / DESIGN_ONLY / UNKNOWN
条件：对象、状态、长度、capacity、线程来源、EH 状态
修正影响：字段/ABI/writer/容量/F1–F8/L1–L4/契约条款
剩余问题：尚未读取的静态分支与运行期依赖分别列明
```

并发交错、尺寸边界与 n=20/n=0 反例只能作为手工静态推导，不运行程序证实；无法唯一解释的类型、异常分类、调用者 receiver 或锁行为保留 `UNKNOWN`。

## 6. 仍需保留的问题及静态边界

| 类型 | 本轮处理 |
| --- | --- |
| 可继续静态展开 | 注册新/同名分支、size 回绕、cleanup caller、EH map/funclet、相关 writer、shadow/mirror 候选交错；给出实际读取或尚未展开的精确位置 |
| 运行期未观察 | 实际默认 binding 值、实际 malloc/handler 行为、worker/main 实际交错、现场 UI/日志/持久化、现场 COL/typeid；不因未实机而抹掉静态分支事实 |
| 设计尚未采纳/实现 | 自建 F4、增补覆写、计数同步方式、发布关闭与迟到调用处置；附建议与契约差异 |
| 原范围外的残余 | 未解析的计算地址访问、未知模块、完整 points-to、其他 Net/teardown caller；保留已有范围，不恢复全程序闭包要求 |

key 非原子/F7 竞争、注册与 assignment 非原子、ordinal 1、generation、重复 poll 与 Join/confirm exactly-once 继续引用 v2 的限定结论。若 K01–K06 的新事实改变这些结论，沿关联路径补充分析；不提前安排游戏或代码测试。

允许 v3 保留带依据的未决项，但要区分“当前问题已有精确回答”和“只记录仍未完成”。存在影响正确性的已知矛盾时，不能通过填齐条目宣称整个静态确认完整。

## 7. 唯一一次整体静态文档审阅

统一入口是一次人工静态审阅：读取 v3 主报告、v3 证据、实际修改的契约/导航与本文，一次给出整个后续推进结果，不运行游戏、程序或校验脚本。

在所有分析与修订完成后，整体检查：

1. 注册新/同名两条分支的 source/destination、`+0x10` binding 与 `+0xC0/+0xC4` 状态缓冲清楚；stride、元素数与实际复制宽度一致。
2. allocator 与 Init 公式忠于指令，malloc 32-bit 请求、加 4 回绕、无符号 `mul` 和正常 vector 的可达条件均已说明；没有“溢出一定失败”的错误外推。
3. F4–F6、注册、Join、query/common 使用同一容量/初始化表；slot21 覆写不能被当作注册 32 项访问的替代方案。
4. 对象/control 常驻与有效基类资源分开证明；cleanup/Init/dtor 的原生直接和虚调用范围明确，不以“自有不主动调用”证明原生不调用。
5. EH 对应实际 state/funclet/资源/锁；未展开部分明确列出。计数算法列操作次序、线性化点与 mirror 过时反例，没有把不释放当作一致性证明。
6. 原 16 项 O-* 的事实范围、F1–F8、L1–L4、覆写建议和契约差异一致；设计建议、继承证据和本轮新事实标签正确。
7. 主报告与索引状态一致，历史 v1/v2 和失败记录保留，引用可定位；没有 gate、哈希校验、作者分离或多次验收安排。

结果分别报告：本次工作实际完成范围、纠正了哪些错误、哪些静态问题仍未完成、哪些仅待运行期观察；不生成 promotion/解锁/实现资格字段，不从“本次整理完成”外推为“全部 ABI 无条件验证完整”。审阅失败时记录失败与未完成项，不自动重跑；后续实质修改按新的完整目标继续。

## 8. 本次文档编写交付

`tx-00-05` 只编写本文并更新目录导航；未执行 K01–K07 补充确认，不产生新的二进制/运行时观察。编写全部完成后进行一次整体静态文档审阅，核对核查问题覆盖、执行步骤、引用、交付文件与现行规范；结果在本次交付说明中报告。
