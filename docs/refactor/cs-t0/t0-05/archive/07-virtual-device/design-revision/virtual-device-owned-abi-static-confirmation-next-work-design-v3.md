> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v3

日期：2026-10-12  
文档编写推进：`tx-00-08`  
状态：`DESIGN_ONLY / 待执行补充静态确认`  
依据：对[补充静态确认 v3](virtual-device-owned-abi-static-confirmation-v3.md)及[证据 v3](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v3.md)的上一轮会话核查（本设计记作 `tx-00-07`）。

先读[现行项目规范](../../../../../project-conventions.md)，用户会话指示优先。本工作不设 gate、不校验任何哈希，不以旧 decision、解锁字段、矩阵完成率或作者身份作为推进前置条件；同一智能体可完成静态分析、整理和一次整体审阅。

## 1. 整体目标与执行范围

纠正 v3 的分配器栈偏移和容量条件，补充当前局部事务的 EH、基类资源 lifetime 与计数同步论证，形成事实、条件、设计建议和未决项彼此一致的静态确认 v4。保留 v3 已支持的注册方向、实际复制宽度、整数回绕、缓冲失效和并发反例，不从“七项有章节”推导“全部 ABI 已验证完整”。

建议后续执行使用一个整体推进 `tx-00-09`，执行者开工时记录实际编号、完整目标与工具。R01–R07 是内容编号，整个目标完成后只做 §7 一次整体静态文档审阅，不按内容编号分别验收。

本次 `tx-00-08` 仅编写本文和更新目录导航，不执行 R01–R07，不产生新 IDB/PE 观察。后续允许的范围为资料阅读、只读 IDB/磁盘 PE 分析、静态控制流/数据流推导及结果文档编辑。不得启动游戏、附加调试器、读进程内存、输入、注入、Hook、IPC、故障注入、运行构建/harness/probe/smoke/canary/旧 verifier，或修改、保存 IDB；不得运行线程程序、模型检查器或尺寸实验来代替静态推导。

延续已采纳路线：自有对象 vtable、自有 control block、S2 action 覆写、`NEVER`、构造前固定 DLL、恢复后模块常驻、每 session 至多一次创建，路线 A 原生 Join 为主、路线 B 重绑为备选。C/S 与原版 Online/Isaac Sync 隔离，NetInputDevice 仅作静态对照。候选初始化方式、增补覆写和计数算法标为 `DESIGN_ONLY`，本设计不自动采纳契约修订。

不恢复全映像 native alias universe、独立 factory/final-release-thread、Batch 2 retry 或未来模块闭包流程。对当前有关的 caller 和资源给出具体范围；无法隔离的 receiver、计算地址及外部模块如实保留 `UNKNOWN`，继续不依赖这些结果的工作。

## 2. 核查问题与资料

### 2.1 上一轮会话核查的定位种子

下表引用 `tx-00-07` 会话结果；本文编写没有重新读取二进制。后续执行须取得直接依据，不能把本表改标为本轮 `IDA_STATIC_CONFIRMED`。

| 问题 | v3 位置 | 核查发现与预期修正 | 工作项 |
| --- | --- | --- | --- |
| Q1 分配器栈偏移 | 证据 K02-1 第 77 行 | 文档写 size 低/高在 `[ebp+0x10]/[ebp+0x14]`；会话原始字节 `0xA64917: 8B 7D 08`、`0xA64910: 8B 5D 0C` 指向 `[ebp+8]/[ebp+0xC]`。区分 IDA frame offset 与实际 EBP 位移 | R01 |
| Q2 容量不闭合 | 主报告 §5、§10 D1；证据 K03-2 | `n≥32` 配两份 32-float 缓冲不能覆盖 common 的 `4*n`。n=33 时需要 132B、现有条件只要求 128B。选择固定 n=32，或实际存储与初始化都覆盖 n | R02 |
| Q3 计数算法不完整 | 主报告 §8.2；证据 K06-3/4 | bounded CAS 失败、隐含 weak、32-bit shadow 溢出、PINNED 后操作和返回值未定义；32-bit store 不自动使两个 uint16 读取成为同一快照；A0 反例不能否定所有纯原子方案 | R05 |
| Q4 EH 未完成状态映射 | 主报告 §7、§11；证据 K05 | 4 张 FuncInfo 已索引，但相关 state 与指令、旧/新 buffer、部分构造、锁及回调副作用未全部对应 | R03 |
| Q5 资源 lifetime 仍有条件 | 主报告 §6、§11；证据 K04 | Init/cleanup/slot5 的间接 receiver 及部分派生/funclet 路径未展开；NEVER 不自动维持缓冲/vector/vptr 有效 | R04 |
| Q6 状态与审阅矛盾 | 证据第 4 行；主报告 §12 第 7 项 | 证据开头仍写 K03–K07 待补，主报告却判“状态一致”通过。保留历史审阅，在 v4 明确纠错 | R07 |
| Q7 字段和边界笔误 | 主报告 §3.1、§5.1 | 状态缓冲 `+0xB00/+0xC4` 应核对为 `+0xC0/+0xC4`；UI 条件 `n≥action` 应为 `action<n`。区分 record 两个 primitive 与设备每元素四个 primitive | R02/R07 |
| Q8 静态完成与实际实现混用 | 契约 v2 实现状态、生产工程与离线原型 | 原型要求最后释放时 object/control 被析构释放，且只编入独立测试工程。它没有实现当前 NEVER，也不能作为真实设备接入的依据 | R06 |

v3 中已支持的事实按原范围保留：新条目 device binding→record、同名 record→device binding、实际读写 0x100B、`0xFFFFFFFF→malloc(3)`、`hi!=0→malloc(4)`、cleanup 清零双缓冲、A0 的 mirror 永久过时反例、insert 的 unwind map 无 manager unlock action。新增结论影响这些事实时，沿实际关联路径补读，不机械重做全部扫描。

### 2.2 阅读入口

| 资料 | 用途 |
| --- | --- |
| [现行项目规范](../../../../../project-conventions.md) | 当前功能约定、静态范围与唯一整体验收 |
| [静态确认 v3](virtual-device-owned-abi-static-confirmation-v3.md)、[证据 v3](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v3.md) | 本轮纠错对象、EH/资源/算法种子及历史审阅 |
| [工作设计 v2](virtual-device-owned-abi-static-confirmation-next-work-design-v2.md) | K01–K07 原目标，尤其 EH state、完整计数算法及状态一致性要求 |
| [静态确认 v2](virtual-device-owned-abi-static-confirmation-v2.md)及其实际引用的主题证据 | 16 项 O-*、F1–F8、38 槽分类、RC/RTTI/Join/key/线程基线；已被 v3 修正的旧结论不再直接沿用 |
| [设计修订稿 v1](../../../07-virtual-device/design-revision/virtual-device-owned-abi-design-revision-v1.md)、[协调器契约 v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md) | 已采纳路线、布局、覆写、计数及 NEVER 行为 |
| [生命周期原型源码](../../../../../../../cs-t0/native/src/virtual_device_lifecycle.cpp)、[头文件](../../../../../../../cs-t0/native/include/isaac_cs_t0/virtual_device_lifecycle.hpp) | 只读辨认旧原型与现行契约的差异 |
| [生产工程](../../../../../../../cs-t0/native/IsaacCsT0.Native.vcxproj)、[原型测试工程](../../../../../../../cs-t0/native/IsaacCsT0.VirtualDeviceLifecycleTests.vcxproj) | 只读辨认实际编译接入范围，不运行构建或测试 |
| [T0-05 功能计划 §10/§11](../../00-overview/isaac-cs-t0-05-research-plan.md)、[Join 规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md) | 05-05 与 05-06 的功能边界；假设不充当运行期观察 |

契约 v2 的已采纳行为与 v3 §10 的 D1–D6 修订草案应按文件分别命名；相同编号不代表相同决策。旧测试命令、哈希、独立角色与解锁字段只作历史资料。

## 3. 工具、地址与证据规则

先发现本次实际提供的 IDA MCP 工具并读取 schema；工具发现方式见项目 `isaac-cs-refactor` 技能及[工作设计 v1](virtual-device-owned-abi-static-confirmation-next-work-design-v1.md)。只使用静态 decompile/disasm/xrefs、字节/整数/元数据读取和纯读 py_eval。没有工具时记录具体缺失，继续文档和源码分析，不自行假定已连接 IDA，不把历史指令变成本轮观察。

执行时只读记录 input/IDB 路径、imagebase、PE Machine/section/SizeOfImage 与相关读取范围，不读取、计算或核对哈希。资料基线为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe` / `isaac-ng.exe.i64`，imagebase `0x400000`；实际工具结果为准。`IDB VA = RVA + imagebase`；无 live moduleBase，不填写运行时 VA。

| 定位种子 | IDB VA（资料 base=`0x400000`） | RVA |
| --- | --- | --- |
| allocator / 下层 alloc/free | `0xA0F4C0 / 0xA648B0` | `0x60F4C0 / 0x6648B0` |
| high / low load | `0xA64910 / 0xA64917` | `0x664910 / 0x664917` |
| header store / payload +4 | `0xA64954 / 0xA64959` | `0x664954 / 0x664959` |
| Base ctor / Init / slot5 | `0xA1E050 / 0xA1E210 / 0xA1E430` | `0x61E050 / 0x61E210 / 0x61E430` |
| cleanup / Base deleting dtor | `0xA1E2C0 / 0xA1E1B0` | `0x61E2C0 / 0x61E1B0` |
| common / Join flush / UI binding copy | `0xA1F280 / 0xA1EE10 / 0xA243C0` | `0x61F280 / 0x61EE10 / 0x6243C0` |
| 注册 record / 新条目 helper / 同名 store | `0x835C90 / 0x835BA0 / 0x835EB7` | `0x435C90 / 0x435BA0 / 0x435EB7` |
| insert / remove / grow / erase | `0xA20DF0 / 0xA20EB0 / 0xA21630 / 0xA21570` | `0x620DF0 / 0x620EB0 / 0x621630 / 0x621570` |
| pair copy / 清理 / 迁移 | `0x902D60 / 0x4242B0 / 0xA21880` | `0x502D60 / 0x242B0 / 0x621880` |
| RC ctor / ReleaseStrong / ReleaseWeak | `0x40CC10 / 0x4358B0 / 0x40C5C0` | `0xCC10 / 0x358B0 / 0xC5C0` |
| FuncInfo：insert / remove / grow / 迁移 | `0xBD21A8 / 0xBD21DC / 0xBD22F4 / 0xBD2354` | `0x7D21A8 / 0x7D21DC / 0x7D22F4 / 0x7D2354` |
| funclet / grow catch-all | `0xB11650 / 0xB11780 / 0xB117B0 / 0xA217CE` | `0x711650 / 0x711780 / 0x7117B0 / 0x6217CE` |

原始字节/指令优先于反编译类型、变量名和 frame 显示。Funclet 可能是非连续 chunk；确认所属函数与地址范围，不能因缺少独立 function 对象就视为不存在。关键 stack operand 必须解释实际寻址位移。

证据分为 `IDA_STATIC_CONFIRMED` / `STATIC_CONFIRMED`、`STATIC_DERIVED`、`DESIGN_ONLY`、`UNKNOWN`。继承项写明来源和“本轮未重读”；每条扫描记录范围、筛选条件、receiver 来源、命中与未解析部分。候选算法只按设计推导报告，不写成二进制事实或测试结果。

## 4. 静态确认内容与执行步骤

| 工作项 | 主要义务 | 必须交付的内容 |
| --- | --- | --- |
| R01 分配器 ABI 与 Init 整数/失败 | O-ALLOC、O-CTOR、O-CB | 实际栈图、原始字节、整数和 header/payload 边界表 |
| R02 n、容量与访问范围 | O-BUF、O-ABI-QUERY、O-JOIN、O-ACTION | 单一容量/初始化表、固定与一般方案、writer 与越界反例 |
| R03 EH state 与局部事务 | O-INSERT-ABI、O-REMOVE、O-NOTIFY | state/指令/资源/锁/副作用/恢复表 |
| R04 lifetime 与破坏性槽 | O-CTOR、O-BUF、O-THREAD | 有界 receiver 分析、资源不变量、覆写建议与旁路 |
| R05 完整计数同步候选 | O-CB、O-RC-INLINE、O-THREAD | 六槽操作、返回、边界与交错推导，准确的契约差异 |
| R06 统一契约及实现缺口 | 原 16 项 O-*、F1–F8、L1–L4 | 条件状态表、设计差异及 T0-05-05 剩余功能 |
| R07 报告与状态收敛 | 全文一致性 | v4、证据 v4、导航及唯一整体审阅 |

### R01：分配器 ABI、栈位移与 Init 失败

1. 复读 `sub_A648B0` prologue、两个 load 的字节、调用点 push 顺序、返回和 caller 清栈。给出函数入口 ESP、建立 frame 后 EBP、CL/EDX、64-bit size 两个 dword 的栈图，核对 `[ebp+8]/[ebp+0xC]`；解释证据 v3 的 `+0x10/+0x14` 是否混用了 IDA frame offset。
2. 区分 `sub_A0F4C0`、`sub_A0F4E0` wrapper 与 Init 的直接调用，不用一个函数的参数表替代另一个。记录寄存器输入、实际栈字节数、返回寄存器、清栈归属；Init 的 `ret 4` 单独记录。
3. 保留并核对 size `(hi:lo)`、`add/adc`、32-bit malloc 参数、64-bit统计及 free `ptr-4`。手工解释 `lo=0xFFFFFFFB..0xFFFFFFFF`、`hi!=0`、malloc 返回零/非零与 handler 出口。
4. 把“请求大小”“malloc 实参”“4B header 写入”“payload 可用范围”分列。对 malloc(0/1/2/3) 非空的假设分支，分析 header store 已超出请求范围、`ptr+4` 不能提供所需 payload；不能只写 payload=0 而省略 header 风险，也不能推断现场一定崩溃或 allocator 实际块大小。
5. 复读 Init 的 unsigned `mul`、`seto/neg/or`、两次分配和重新计算的 memset 长度。正常合法 vector、畸形/损坏 vector和并发长度变化分开；不把整数反例外推为正常 32 元素对象必然触发。
6. n=0 的两次 malloc(4) 分别可能失败；`AL=1` 是到达返回处的行为，零长度 memset 的现场行为/handler 非局部退出未确认时须保留条件。n>0 的函数内空指针访问不能由返回后检查解决。
7. Memory Manager handler 沿当前可定位的 `sub_A23200` 分派和已知目标分析，无法解析的目标记具体来源。只得出已支持的诊断/回调副作用，不要求读取进程或补齐任意外部模块。

输出：ABI 栈图、原始字节表、尺寸/header/payload 表、Init 失败 CFG，以及 F1/F4/F5 的具体影响。受检查的自有分配或初始化只能作为候选方案，并列明它改变了契约 v2 哪些“原生 ctor/Init 写入”约定。

### R02：统一逻辑 n、实际存储与初始化

分别定义 `n`、binding capacity、已初始化 binding 数、current/previous 的实际分配和已初始化 float 数。对合法布局说明 begin/end/cap 次序、16B stride、差值与算术范围；未实际构造的元素不能由 reserve 或伪造 end 得到。

| 消费者 | 必须核对的访问与条件 |
| --- | --- |
| 注册新/同名分支 | action 0..31，每个 16B binding 的前两个 dword，要求这些元素真实存在且初始化；名称避重不能跳过新条目 helper |
| common | memcpy/memset 各按当前 `4*n` 字节，双缓冲均须覆盖当前 n；同时分析两次长度读取之间是否可能改变 |
| Base query / raw fallback | 逐槽区分 cur/prev 与 binding 访问，入口 `action<n` 不证明内部缓冲有效；自有 query 中性不替代继承 common 的要求 |
| Join slot21 | action19 的 binding 循环及无条件 cur/prev[19]；slot21 覆写只能保护此路径 |
| UI binding copy | `sub_A243C0` 为 `action>=n` 只日志后继续读取，安全条件是 `0<=action<n` |

1. 手工保留 n=0、20、32、33 反例；n=33＋双 32-float 缓冲会让 common 请求 132B。将整数安全与实际分配/初始化安全分别解释，不运行用例。
2. 优先评估固定方案：`n=32`，32 个已初始化 16B binding、两份至少 32-float 缓冲，发布后禁止未协调的长度/容量变化。该方案是建议，不是默认模板 count 已知或已采纳的声明。
3. 一般方案：`n>=32` 且 binding 实际存储与初始化覆盖 n，两份缓冲各覆盖至少 n，并由原生读取与 writer 共用明确的长度不变量。不能继续写“n≥32＋仅32个元素/缓冲”作为充分条件。
4. 沿 slot5、同名写回、common 的 binding 编辑及已知 UI/helper 参数追踪元素 writer、begin/end/cap writer和双缓冲 writer。区分 device 每 action 四个 primitive 与 record 每 action 两个 primitive；record 没有额外两个未使用 key 字段。
5. 默认模板 count/内容仍属未观察值时，说明固定方案怎样限制或替代 slot5、怎样分配双缓冲、怎样避免原生 Init 内部失败；只写可执行的候选与契约差异，不假定运行期模板为32。
6. 修正 `+0xB00` 和 `n≥action` 笔误，并把 F4–F6、消费者表、覆写建议、README 和契约草案全部引用到同一容量条件。保持 slot3 名称 lifetime 与同名写回的影响范围准确。

输出：唯一容量/初始化表、元素/指针/缓冲 writer 表、固定 n=32 与一般 n≥32 的条件比较、已支持的访问事实及未决 writer。

### R03：逐 state 展开 EH 与注册事务

范围包含 insert `sub_A20DF0`、remove `sub_A20EB0`、grow `sub_A21630`、erase `sub_A21570`、pair copy/release/迁移、实际 notification 与直接相关的 controller-list append。`sub_8367E0` 的 record list 增长与 `sub_A21630` 的 manager pair vector 增长必须分开分析。

1. 从 4 张 FuncInfo 复读 maxState、unwind/try/handler map及相关 funclet，记录所属函数、raw state 值、toState、action、try 范围。沿实际 state 写入位置映射控制流，不用元数据表代替资源状态表。
2. 覆盖 lock 前/后、无增长 append、分配前/后、新元素 copy、旧元素迁移、旧 range release、旧 buffer free、vector 三指针发布、unlock、notification、按值 pair 最后释放。原 root、传入按值 pair、临时 pair、manager vector strong 分列。
3. 对 grow 的尾部插入与非尾部插入分支，追踪部分构造 range 的 begin/end 更新、funclet 清理以及 catch-all `0xA217CE` 的顺序。解释发生异常时旧 buffer/元素是否仍有效，不能把“未发布新 vector”写成“旧 vector 必然未改变”。
4. 对 `_Xlength_error`、分配/handler C++ 异常、allocator null、TryAddStrong 失败、noreturn、Release/notification 异常分别解释。区分普通失败、C++ unwind、AV 和终止；无依据时不声称 AV 被 C++ catch 覆盖。
5. 检查显式 manager unlock 与 unwind action 的关系。仅在已映射的路径内说明锁残留、已解锁、terminate 或未知；外层处理未知时不推导进程必然可继续，也不虚构自动 unlock。
6. 对 notification 中名称分配、record append、同名 binding 写回与 temporary pair 释放记录部分副作用。确认哪些属于 manager vector 清理、哪些不回滚；未展开的回调目标列实际地址/分派来源。
7. 根据上述状态修订契约 v2 的“insert 异常→RETIRING→remove”：锁残留、成员身份未知、半迁移、进程终止分别给出可恢复范围。不得在未知锁/成员状态下建议盲目 remove、重试或强制卸载。

输出表：`函数/指令区间/state → 资源与锁 → 触发出口 → unwind/action 顺序 → 发布与副作用 → 可继续的范围/UNKNOWN`。无法读取的具体 state/action 明确列出，不以“待实机”隐藏静态缺口。

### R04：基类资源与破坏性调用的 receiver

1. 以 v3 K04 的直接 callers 为种子，展开当前相关 cleanup/Init/slot5/Base dtor、派生路径及 funclet。跟踪调用实参、adjusted this、原始对象来源、RTTI/状态条件和返回后的访问。
2. 对 manager、notification、UI/配置、Join 和 teardown 内的相关 slot1/2/5 调用给出 receiver 来源。查不到 code xref 只说明当前扫描结果；“不导出对象”“vptr/COL 不是派生类”不能自动排除直接 native call。
3. 建立资源表，至少覆盖对象/control、vptr/COL、binding begin/end/cap、cur/prev、名称、快照、原生 Mutex/CS。Init、cleanup、vector dtor 的效果分列；cleanup 释放双缓冲，不应误写它本身也析构 binding vector。
4. 对 slot0/1/2/5、slot7/8/19/21/34 和 common，分别说明构造期、正常、RETIRING、UNREGISTERED、ZOMBIE 的访问、写入、返回和有效性条件。记录字段中性是否足以让继承槽中性，以及原生直接调用是否绕过自有覆写。
5. 比较受控 slot1/2/5、slot7/8 中性覆写、slot21 覆写或有效缓冲条件。每个增补槽列偏移、this/栈参数、清栈、返回寄存器、直接旁路和契约变化，不只列槽编号。
6. L2/L3 按当前已隔离路径作有条件结论。无法隔离的间接 receiver 和未解析旁路列出具体来源与覆盖范围；不要求穷尽整个映像或未来模块，不把任意未知都变成后续推进条件。

输出：caller/receiver 表、资源生命周期表、候选覆写集合、直接调用旁路及 L2/L3 条件。NEVER、缓冲有效、计数一致、输入中性和代码常驻分别论证。

### R05：把计数同步候选写到逐槽操作层次

先以原生六槽及调用者为事实基线，核对 this、栈参数、清栈、AL/EAX 返回、strong/weak 与 `+0x14` 更新顺序、embedded Mutex/CS。返回值未被当前 caller 使用只能按具体范围报告；自有槽不能笼统返回“native-compatible value”。

当前契约 §5 的准确限制为“不得阻塞等待主线程；不得执行回调、析构或内存释放”，以及“自有槽不依赖原生 embedded mutex”。分析自有短锁时不得把前者替换成“绝对禁止任何锁”。采用原生 mutex、改变延迟/镜像/清零语义时列明真实差异。

候选必须形成以下操作表；不能只用 `new=f(s,mode)` 代替全部槽。

| 操作/边界 | 必须定义的语义 |
| --- | --- |
| control dtor | 诊断与中性信号；NEVER 不释放对象/control/CS |
| TryAddStrong | strong=0 的失败、非零增长、与最后 release 的竞争、准确返回 |
| AddStrong/AddWeak | 正常增长、uint16 镜像饱和、32-bit shadow 最大值、不允许回绕至0 |
| ReleaseStrong | 零上 release、非最后一次、最后一次；object 清零、退休信号和隐含 weak 恰好释放一次 |
| ReleaseWeak | 零上 release、最后 weak、不释放；与隐含 weak release 的交错 |
| PINNED | 粘滞标志、之后允许的 Add/Release/Try、计数与返回、不会错误复活 object |
| 获取同步失败 | bounded CAS 达到上限、持锁线程被挂起、重入；释放槽无法报错时如何保持必要计数语义 |

1. 给出完整操作次序、所有返回分支、共享字段的原子性/读写宽度、内存顺序、同步参与者及线性化说明。shadow、mirror、object、退休信号不在同一存储位置时，分别说明哪些观察者能看到过渡状态及其允许范围。
2. 保留 A0 的永久旧 mirror 反例。补充 Add/Release、双 Release、TryAddStrong/最后 Release、strong/weak 同时更新、镜像饱和后回落、shadow 溢出、PINNED 后操作和隐含 weak 的有限交错；只手工推导，不运行程序或模型。
3. A1 若采用自有短锁，给出获取失败的具体行为和依据。不能丢弃一次 Release、假装 Try 成功、无锁覆盖 shadow/mirror 或等待主线程来掩盖失败。未找到满足契约的处置时，明确写“候选算法尚不完整”。
4. 最后 strong 在锁内处理隐含 weak 时，说明采用内部 helper/合并更新还是释放锁后调用 weak 槽；排除同一非重入锁被递归获取，并说明后者的可见窗口。诊断/入队必须有界、不分配且符合槽内无回调的约束。
5. 32-bit packed mirror store 的对齐和写入不可分性须有当前布局依据，不仅依赖未经确认的 malloc 对齐。单次 packed store 不保证两个独立 uint16 load 为同一快照：可手工列读 strong、writer store、读 weak 的交错。不能从该 store 推导它与 shadow/object 的整体一致。
6. 区分通过自有槽且使用同一同步规则的 caller，与可能的原生 inline reader/writer。已有 O-RC-INLINE 有限范围可继承；发现新的旁路沿其 receiver 分析。不恢复全程序 points-to 闭包，也不把保留 CS 当作已经同步。
7. 比较 A1、自有有序/版本化发布和使用原生 embedded mutex 的 B，按算法实质选择分析；不预设其中任何方案已证明。A0 反例只能否定 A0，不得作“所有纯原子算法不可能”的全称结论。

交付至少一个有完整次序与边界语义的候选，或明确列出尚不能满足的具体契约条款、反例和所需修订。两种结果都须按真实范围报告；记录“未完成”不等于候选已获验证。

### R06：F1–F8、L1–L4、16 项义务和实现边界

1. 原 16 项 O-* 全部保留，受影响项引用 R01–R05，其余项标“继承 v2/v3 的具体章节，未在本轮重读”。各项分别报告已回答的问题、成立条件、未完成静态分析、未观察运行值和候选设计，避免整项无条件标为完成。
2. 重写 F1–F8 条件表：F1 原始分配与 handler；F2 ctor/key；F3 自有 vptr；F4 binding/双缓冲；F5 control/CS/root；F6 按值 pair 注册和通知；F7 成员身份、同 key 唯一、lookup 解析到本对象三问；F8 assignment/Join/generation 的独立窗口。每项附 ABI、线程、失败后的资源和可恢复范围。
3. 沿用 key 非原子、lookup 首匹配、worker/remove 并发及注册/assignment 非原子的已有事实。只在 R01–R05 改变结论时补读关联路径；列候选实现约束和具体未决，不从主线程创建推出无竞争。
4. L1 区分自有槽分派与已知 inline 旁路；L2 为常驻与资源有效；L3 为迟到访问中性及继承槽条件；L4 为模块/vtable/COL/代码有效。slot3 名称和 slot30 存活信号退休后不能统一返回0。
5. 只读确认原型的 `ReleaseClass/FinalRelease`、Destroyed 状态、构造后回滚释放及生产工程编译项。产出原型复用/需重写差异表，不修改源码，不运行历史16 cases或任何构建；历史通过仅按当时原型解释。
6. 在 v4 内形成契约差异草案：`契约 v2 原条款 → 候选条款 → 事实/推导 → 行为变化 → 未决/未实现`。重点包括 n、初始化写入权限、覆写集合、计数同步/镜像观察者、异常退休与 remove 的合法范围。不把 v3 草案编号误当作已采纳设计。

T0-05-05 剩余工作按功能归为四类：静态证据收尾、可执行契约、真实自有设备生产实现、最小 Join/selected-device 与退休恢复接续。本轮覆盖第一类并提供第二类候选；不执行后两类，不宣称阶段已完成。这些归类不是强制四次推进或工期/百分比估算。完整 A/B scheduler 隔离、1,000 tick soak 和 Replay 按05-06/05-07的实际目标处理；05-05 的最小 Join/confirm 防重复不能因此省略。

### R07：v4 报告、索引与导航

1. 建立逐条修正表：v3 原位置/原结论、v4 依据、修正或保留、影响的 R/O/F/L 项及残余；Q1–Q8 均有落点。保留 v3 §12 和证据开头的原状态，在新报告解释为何旧“通过”不支持完整结论。
2. 主报告、证据和导航引用同一容量方案、状态与覆写建议。正在分析和已经整理的状态写在真实位置；结束时主报告与索引的开头、末尾应一致，不沿用“K03–K07 待补”。
3. 检查字段 `+0x10/+0x14/+0x18` 与 `+0xC0/+0xC4`、record/device primitive 数、slot0/3/30 编号、UI `action<n`、栈位移与 RVA/VA 区分。访问宽度、步长和跨度分别描述，不把 logical 0x200B binding 当作连续复制宽度。
4. 对候选计数算法单独报告成立范围、不能支持的观察者和未定义分支；不能靠加上 `DESIGN_ONLY` 标签免除已知矛盾。静态尚可展开与仅待运行观察分列。
5. 不创建 promotion、解锁、认证 manifest 或 verifier，不改写原 v1–v3 报告、原证据和失败记录。全部分析与文档修订完成后执行 §7 一次整体审阅，失败如实记录，不自动重跑。

## 5. 后续交付与证据模板

建议执行者完成分析后新增：

- 同目录 `virtual-device-owned-abi-static-confirmation-v4.md`：实际工具/范围、Q1–Q8 修正、R01–R07 结果、容量/资源/EH/计数表、16 项 O-*、F1–F8、L1–L4、契约差异、剩余功能及整体审阅结果。
- `out/cs-t0/virtual-device-owned-abi-static-evidence-v4.md`：原始字节/指令、实际栈图、尺寸与访问推导、state/资源表、有限交错、准确继承索引和状态。内容较大可按主题拆分，主索引指向实际存在的章节。
- v4 主报告内的契约差异草案和原型差异表；新候选不覆盖现行契约 v2。是否后续正式修订契约按实际任务处理。
- 07-virtual-device README 的当前入口更新；历史报告、设计和失败入口保留。

本次设计编写不创建空 v4、空证据、测试计划或占位验收报告。

```text
问题：Qxx / Rxx / O-* / F* / L*
来源：实际工具、input/IDB、原报告或源码；继承/本轮重读分列
位置：imagebase、IDB VA、RVA、原始字节、实际栈位移、读取范围
直接事实：输入/输出、访问宽度、资源写入、EH 元数据/状态
推导：成立条件、控制流/资源/锁、手工交错与最小反例
设计：完整候选操作、返回/失败语义、与契约 v2 的准确差异
范围：caller/receiver、长度/初始化、线程/同步参与者、状态
未决：尚未展开的具体静态位置；未观察运行值；未采纳/未实现
状态：按上述证据分类报告；不从章节齐全或无 xref 推导全部完成
```

## 6. 残余问题的处理

| 类型 | 处理方式 |
| --- | --- |
| 当前范围内可静态展开 | 栈偏移、容量、EH state、资源 receiver、计数操作；记录直接依据或未展开的具体位置，不伪装成仅能实机观察 |
| 运行期未观察 | 默认 binding 实值、malloc/handler 现场行为、worker/main 实际交错、现场 RTTI/UI/日志及持久化；不安排本轮实机 |
| 候选未完整/未采纳/未实现 | 分别记录算法缺口、契约差异与实现缺口，不合并成“已确认” |
| 原范围外 | 完整 points-to、未来/未知模块、无当前关联的 Net/teardown 路径；保留已有边界，不恢复全映像闭包 |

允许带依据的未决项存在，必须区分“问题已得到精确的条件答案”与“工作尚未完成”。已知事实错误、算法未定义分支或相互矛盾的容量/资源条件不能通过填齐表格得到“验证完整”。工具缺失也只说明具体证据未取得。

## 7. 唯一一次整体静态文档审阅

统一入口为一次人工静态审阅：在整个后续推进的分析与文档编辑完成后，一次读取 v4、证据 v4、本设计、实际契约草案和导航，给出整体结果；不运行程序、构建、游戏或校验脚本。

1. ABI：栈位移忠于原始字节，wrapper/Init/下层 allocator 分开；整数公式、header、payload 与失败出口不混用。
2. 容量：固定 n=32 或一般 n 的实际存储/初始化要求清楚；所有消费者共用同一条件，n=33反例已处理，UI边界准确。
3. EH：相关 state 与具体指令、资源/锁/发布/回调副作用对应；无法解析处准确标注，恢复建议不越过证据支持范围。
4. lifetime：常驻与有效基类资源分开，直接/虚调用 receiver 有范围；增补覆写及旁路说明与 L2/L3 相符。
5. 计数：六槽返回、获取失败、隐含 weak、饱和/溢出/PINNED、可见窗口和同步参与者有具体答案或明确缺口；不作无依据的原子算法全称判断。
6. 整合：Q1–Q8、16 项 O-*、F1–F8、L1–L4 与契约差异可定位；原型及生产接入状态准确，未把静态报告视为 T0-05-05 已完成。
7. 文档：主报告/证据开头和末尾状态、导航与引用一致；保留历史和失败，不含 gate、哈希校验、角色分离或多个验收安排。

一次分别报告事实修正是否完成、局部静态分析覆盖范围、候选算法/契约是否完整、剩余功能及无法确认的证据。存在已知矛盾时记录未通过的理由；验收失败后不自动修补重跑。后续有实质修改时按新的完整目标推进，不仅更换编号重做同一验收。

## 8. 本次文档编写交付

`tx-00-08` 交付本文与目录导航更新。本文按 `tx-00-07` 会话核查编写，未执行新二进制分析、未改生产代码、未采纳候选契约、未运行构建、程序或游戏测试。全部编辑完成后采用一次整体静态文档审阅，结果在本次交付说明中报告。
