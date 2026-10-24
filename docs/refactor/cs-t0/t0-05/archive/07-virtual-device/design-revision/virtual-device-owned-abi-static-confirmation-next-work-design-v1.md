> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v1

日期：2026-10-12  
文档编写推进：`tx-00-02`  
状态：`DESIGN_ONLY / 待执行静态确认`  
适用对象：当前 Win32/x86 `isaac-ng.exe` 的自有 Virtual Device 路线。

先读[现行项目规范](../../../../../project-conventions.md)。用户会话指示优先；本文不设 gate，不校验任何哈希，不使用旧 decision、权限/解锁字段或矩阵完成率作为推进前置条件。逆向、整理与最终审阅可由同一智能体完成，不要求子智能体或作者分离。

## 1. 整体目标与执行范围

将[静态确认 v1](virtual-device-owned-abi-static-confirmation-v1.md)补充为可供实现使用的、有明确适用条件的原生接口说明，回答创建、查询、计数、注册、移除和退休路径中的具体问题。重点纠正 Init 失败处理、F7 唯一性判断、查询语义与继承槽安全性方面的不足，并覆盖[设计修订稿 v1](../../../07-virtual-device/design-revision/virtual-device-owned-abi-design-revision-v1.md)的全部 16 项 O-* 义务。

建议后续静态确认使用一个整体推进 `tx-00-03`；执行者开始时记录实际推进编号、所用资料与完整目标。本文的 W01–W10 只是分析内容编号，不能拆成多次测试验收。整个静态目标完成后，只做一次整体静态文档审阅；分析、逆向和修订期间不运行测试。

本次授权目标是编写工作设计，尚未执行本文中的二进制确认。后续执行范围为资料阅读、只读 IDB/磁盘 PE 分析和结果文档编辑。本文不安排游戏启动、附加调试器、进程内存读取、输入、注入、Hook、IPC、故障注入、构建、harness、probe、smoke、canary 或验证脚本。

预期结果包含三类内容：

1. 直接读取指令/数据得到的原生事实，以及这些事实适用的函数、对象和控制流范围。
2. 基于原生事实推导的实现约束、静态反例和协调器契约修订建议。
3. 静态分析仍不能回答的问题及其影响，尤其是运行期值、实际并发时序和实机行为。

允许最终仍存在 `UNKNOWN`。完成本文的工作，表示所有问题已被分析并如实交代，不表示自有设备已实现、已实机验证或整个 ABI 已获无条件证明。

## 2. 已采纳的设计与本轮不变的功能边界

- 自有对象 38 槽 vtable、自有 control block 6 槽表；采用 S2 action 层覆写。
- `NEVER`：进入 `BASE_CONSTRUCTED` 后，对象、control block 和有效基类状态在进程生命周期内不释放；只有未执行原生构造的 raw allocation 可以回滚释放。
- 首次调用原生 base ctor 前固定 DLL，恢复后模块常驻；每 session 至多一次创建，失败不自动重试。
- 原生 Join 路线 A 为主，直接重绑路线 B 为备选。[协调器契约 v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)继续提供功能约定；本轮提出的修订必须说明依据，不悄悄改变已采纳设计。
- C/S 与 OriginalOnline、Isaac Sync 独立；NetInputDevice 可用作静态比较对象，不能复用为 C/S 的组件或创建入口。
- A/B 使用同一 canonical、sequence、target tick、queue、dedupe、resolve 和 apply；pointer、key、handle 不成为网络身份。
- 本轮不恢复 native alias universe 全映像别名闭包、原生独立 factory 或最终释放线程旧流程，不重跑 Batch 2。

## 3. 资料入口与证据使用方式

### 3.1 必读资料

| 资料 | 本轮用途 |
| --- | --- |
| [现行规范](../../../../../project-conventions.md) | 推进、唯一整体验收、`NEVER`、线程、恢复和实际授权边界 |
| [自有 ABI 静态确认 v1](virtual-device-owned-abi-static-confirmation-v1.md) | 函数/数据定位种子、原报告结论与 S-1–S-11 未决项 |
| [设计修订稿 v1](../../../07-virtual-device/design-revision/virtual-device-owned-abi-design-revision-v1.md) | F1–F8、L1–L4、16 项 O-*；冲突流程按现行规范处理 |
| [协调器契约 v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md) | 内存布局、槽行为、状态转换、迟到访问与恢复约定 |
| [静态逆向交接 v1](../virtual-device-static-reverse-v1.md) | assignment、selected key、manager、poll/query 与地址换算 |
| [联合逆向 v2](../virtual-device-ownership-lifecycle-joint-reverse-v2.md) | 按值 pair、通知、copy/reset/erase/grow/stop 已有证据 |
| [MT-R06 双缓冲交接 v1](../../06-manager-identity/02-copy-move/native-join-manager-this-mt-r06-double-buffer-v1-handoff.md) | 指针与长度关系、历史反例 |
| [MT-R06/R07/R08 successor v3](../../06-manager-identity/02-copy-move/native-join-manager-this-mt-r06-r07-r08-successor-v3-handoff.md) | 修正后的扫描集合：75 个 vtable 方法、18 处方法内访问；另列构造与 common update |
| [T0-04 输入交接](../../../../t0-04/isaac-cs-t0-04-reverse-handoff.md) | gameplay action、canonical、自然 consumer、Authority phase 与生命周期语义 |
| [原生 Join 用户规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md) | 可靠创建/发布 B 的已接受设计前提；不扩大为设备选择与持久副作用已证明 |
| [原生 Join 主动路径资料入口](../../02-active-join/native-join-active-reverse-v1.md) | 追踪 join 触发、设备选择、key 与 assignment；按入口导航到原文 |
| [canonical 输入类型](../../../../../../../cs-t0/native/include/isaac_cs_t0/input_core.hpp)与[输入实现](../../../../../../../cs-t0/native/src/input_core.cpp) | 24B 输入布局、axes、held/edge、action 分类的源码语义 |

旧记录中的哈希、测试命令和角色条款只作历史信息，不执行。旧 `STATIC_CONFIRMED_NO_OVERLAP` 结论必须保留其对应调用、对象族和条件，不能外推为全部迟到访问安全。

### 3.2 区分已有资料与本轮新事实

v1 的表、计数和字节锚点是定位种子。本轮须将涉及最终结论的关键返回分支、释放调用、字段访问和调用者使用方式直接读取出来；仅从 v1 抄录的内容标明“继承原报告，未在本轮重读”。

证据使用 `IDA_STATIC_CONFIRMED`（实际使用 IDA 直接读取时）、`STATIC_CONFIRMED`（其他只读静态工具直接读取时）、`STATIC_DERIVED`、`DESIGN_ONLY`、`UNKNOWN`。已有运行时记录保留 `RUNTIME_OBSERVED` 及其原场景，本轮不新增该等级。

数值计数必须附实际查询范围、筛选条件和命中列表的位置。例如“198 个候选”不能替代命中项分类；“code xref 为零”不能替代对间接调用与计算地址的说明。此处不要求全程序别名闭包或任何认证链。

## 4. 工具与地址规则

### 4.1 只读工具选择

先发现当前实际提供的 IDA Pro MCP 工具，读取其 schema。可按项目技能使用以下发现入口：

```javascript
const matches = ALL_TOOLS.filter(t =>
  /ida[-_]pro[-_]mcp/i.test(t.name) || /\bIDA Pro\b/i.test(t.description)
);
text(matches);
```

使用已发现的只读反编译、反汇编、xref、原始字节及数据读取能力。不得猜测工具名、端口或连接状态。无 IDA 入口时，可以使用已有反汇编资料或可用的磁盘 PE 静态工具；工具不足导致无法重读的内容如实标注，不宣称已确认。

只读提取用于查看指令和数据，属于分析；禁止调用包含哈希检查、PASS/FAIL 判定、自测、mutation 或 replay 的旧 builder/verifier。不得修改 IDB 的字节、名称、类型、函数定义或保存数据库。为显示某种伪代码而改类型也不在本次范围内。

### 4.2 地址与目标记录

- 分开记录磁盘路径、IDB 路径、PE header ImageBase 和 IDB imagebase；用 PE 版本信息、machine、section 布局、SizeOfImage 和实际函数/数据位置说明分析对象，不计算哈希。
- 对当前资料采用 `IDB VA = IDB imagebase + RVA`。现有 IDB imagebase 是 `0x00400000`；执行时实际读取，不能把历史值冒充本次观察。
- 修正 v1 §0 的措辞：`0x0061E050` 是 RVA，`0x00A1E050` 是对应 IDB VA。不能把前者称为“已带 imagebase 的绝对地址”后再次加 base。
- 核对不同来源是否使用同一地址空间。旧稿中未标地址种类的 `0xA112C0` 先作为待定位种子，确认所属模块和用途后再记录，不机械加减 base。
- 本轮没有 live moduleBase，不填写运行时 VA。后续实机地址另按 `moduleBase + RVA` 换算。

以下仅为已有报告中的定位种子，不能据此填写“本轮直接确认”：

| 对象/函数 | RVA | IDB VA（base=`0x00400000`） |
| --- | --- | --- |
| Base ctor / Init / cleanup | `0x0061E050 / 0x0061E210 / 0x0061E2C0` | `0x00A1E050 / 0x00A1E210 / 0x00A1E2C0` |
| Base deleting dtor | `0x0061E1B0` | `0x00A1E1B0` |
| allocator / 下层分配函数 | `0x0060F4C0 / 0x006648B0` | `0x00A0F4C0 / 0x00A648B0` |
| refcount ctor / deleting dtor | `0x0000CC10 / 0x0000CB50` | `0x0040CC10 / 0x0040CB50` |
| TryAddStrong / AddStrong / ReleaseStrong | `0x0000C6B0 / 0x0000C690 / 0x000358B0` | `0x0040C6B0 / 0x0040C690 / 0x004358B0` |
| AddWeak / ReleaseWeak | `0x0000C610 / 0x0000C5C0` | `0x0040C610 / 0x0040C5C0` |
| Base query 12/13/14 | `0x0061E8D0 / 0x0061E970 / 0x0061EA20` | `0x00A1E8D0 / 0x00A1E970 / 0x00A1EA20` |
| per-device common / manager update | `0x0061F280 / 0x0061FC00` | `0x00A1F280 / 0x00A1FC00` |
| manager update 已知 callsite | `0x00554F7D` | `0x00954F7D` |
| keyed lookup / insert / remove | `0x00620390 / 0x00620DF0 / 0x00620EB0` | `0x00A20390 / 0x00A20DF0 / 0x00A20EB0` |
| vector erase / grow | `0x00621570 / 0x00621630` | `0x00A21570 / 0x00A21630` |
| Manager singleton / key counter | `0x00857B18 / 0x0087362C` | `0x00C57B18 / 0x00C7362C` |
| Base vtable / control vtable | `0x00782758 / 0x00778014` | `0x00B82758 / 0x00B78014` |
| SetControllerId | `0x003A6450` | `0x007A6450` |

## 5. 工作项与建议顺序

W01 → W02/W03/W04 → W05/W06/W07 → W08/W09 → W10。箭头表示资料依赖，不是阶段验收；遇到某项无法回答时记录原因，继续不依赖该结论的分析。

| 工作项 | 对应原义务 | 主要输出 |
| --- | --- | --- |
| W01 地址与既有结论整理 | 全部项的定位基础 | 地址表、已有/新增证据区分、v1 纠错记录 |
| W02 分配、构造与 Init 失败 | O-ALLOC、O-CTOR、O-CB | 失败 CFG、字段初始化表、资源与异常处置建议 |
| W03 六个对象覆写槽与 query | O-ABI-0C、O-ABI-78、O-ABI-QUERY | 精确 ABI、返回分支、参数域与边沿公式 |
| W04 继承槽和双缓冲 | O-BUF、O-CTOR | 32 槽逐项判断、长度/容量/生命周期依据 |
| W05 control block 与旁路 | O-CB、O-RC-INLINE | 六槽 ABI、镜像与影子计数约束、有限旁路分析 |
| W06 key 与唯一性 | O-KEY | key writer/consumer 表、F7 修订建议与竞争反例 |
| W07 注册、移除、通知和线程 | O-INSERT-ABI、O-REMOVE、O-NOTIFY、O-THREAD | pair 所有权、锁/线程图、异常及状态窗口 |
| W08 RTTI/COL | O-RTTI | 数据布局、cast/typeid 消费、方案适用条件 |
| W09 Join、action 与身份 | O-JOIN、O-ACTION | Join 设备选择链、action 映射表、绑定关系 |
| W10 统一结论与契约建议 | 全部 16 项；L1–L4 | 新确认报告、契约差异、残余问题、唯一整体验收 |

### W01：地址与 v1 结论整理

1. 按 §4.2 记录实际分析对象与地址空间，逐项确认关键函数和表的定位。
2. 将 v1 结论拆为“直接代码/数据”“静态推导”“设计建议”“运行期未决”四类；保留原文，修正结果写入新报告。
3. 明确以下待纠正表述：Init 返回后检查不能覆盖函数内部 null 解引用；一次 lookup 不能证明唯一；对象内偏移合法不能证明指向的缓冲仍有效；slot33 的 `(int,float)` 与 `(float,float)` 不一致；“ctor 无失败返回”不能外推为其 callees 不抛异常或不会终止。

### W02：分配、构造与 Init 失败

分析范围：allocator 两层、Base ctor/Init/cleanup/dtor、`sub_A2C980`、默认 binding 拷贝/扩容、refcount ctor 及 Mutex/CS 初始化；用 Keyboard/Gamepad 创建路径比较顺序。只展开与分配、字段有效性和失败出口有关的 callees，不无限扩展到全程序。

执行步骤：

1. 读取分配函数每条正常/失败/异常出口，确认 null、throw、终止、new-handler/重试路径及调用者清理。不能仅从调用者存在 null 分支判断 allocator 一定返回 null。
2. 说明分配对齐、size 溢出与扩展区对齐需求；定位只供 raw allocation 回滚使用的配对释放函数。对无法从当前二进制确认的库保证保留 `UNKNOWN`。
3. 展开 ctor 的关键 callee，区分“无失败返回值”和“不会失败”；列出构造中的 Mutex/CS、vector、子对象资源及部分构造情况。
4. 对 Init 分别记录 `n=0`、`n>0`、第一次/第二次分配失败、binding grow 失败、重复 Init、异常退出的控制流与指针状态。追踪 `4*n` 的宽度、溢出、长度来源和两次分配时长度是否一致。
5. 对每个失败点给出“发生在发布之前还是之后”“是否还能返回协调器”“哪些资源有效”“允许做什么”的说明。

必须交付：

- 失败点表：`函数/指令 → 条件 → 实际出口或后续解引用 → 指针/资源状态 → 对 F1–F5 的影响`。
- Init 调用前需要满足的具体条件。若原生路径可直接崩溃，明确说明返回后检查或通常的内存预检无法保证该次分配成功；不把 SEH 捕获访问异常自动当作安全恢复。
- 可选实现方案的静态依据与设计差异。改变 Init/slot5、增加覆写或自行构造缓冲只能作为建议，必须解释与原生布局、默认 binding 和 `NEVER` 的关系，不能在报告中冒充已采纳实现。
- 基类字段表，包含未初始化的 `+0x04`、gap/padding、初始化 callee、副作用和后续 reader。不再写“全字段已初始化”。

### W03：六个对象覆写槽与 query 的精确语义

范围：slot0/3/12/13/14/30；相关调用点、元素函数指针及 fallback。slot31/32/33 的 ABI 同时作为继承分析输入。

对每槽记录 `ECX=this`、stack 参数的偏移/宽度/顺序、callee/caller 清栈、`ret N`、返回载体（AL/EAX/x87 ST0）、out 参数与异常路径。以调用者如何使用返回值和参数解释其类型；伪代码类型只作辅助。

具体工作：

1. slot3：核对三类原生实现与 insert 通知消费者，确定 C 字符串生命周期、非空要求及是否还有缓存/比较用途。
2. slot30：核对 poll true/false 后的 common/update 路径。分别说明“输入中性”与“设备仍存活”的语义；退休输出不能简单让 poll 返回 0，从而意外触发原生 erase。确定重复 poll、缺少新 snapshot 和暂停状态对锁存设计的影响。
3. query 三槽：逐分支恢复 action 参数域、unsigned/signed 比较、flags 前提、`action < n`、`action >= n`、`UINT32_MAX`、fallback 和元素函数调用规则。不得假定所有越界都返回零。
4. 将 pressed/triggered/value 写成从原始分支得出的公式或伪代码，说明比较的是零、阈值、bit 还是其他状态；边沿方向、连续 held、松开与重复查询语义逐项说明。不得仅凭 current/previous 缓冲命名推导 rising edge。
5. 核对 slot33 参数从两个调用点的构造到返回；若 Base 空实现无法唯一确定参数类型，写明两个 32-bit stack 项已确认、类型仍未知，保留读取 float bit-pattern 等可能性。

必须交付：六个覆写槽 ABI 表、query 完整分支表、边沿公式与静态控制流依据、slot31/32/33 参数结论。对于同一 authority tick 的 repeated poll 是否重推 previous，记录设计问题，不能由 query ABI 推导 exactly-once 已完成。

### W04：32 个继承槽与双缓冲有效性

以 Base 38 槽逐项分类，不把所有四类设备的专有字段混入自有对象。对 32 个当前继承槽记录：参数/返回的已知范围、直接及间接调用条件、read/write 字段、指针长度/容量前提、分配/释放、锁、重入、运行期全局、退休后的行为。

具体工作：

1. 读取 MT-R06 successor v3 的 18 处方法内访问列表，并单列 ctor 两处写和 common update 三处读；区分“命中指令数”与“函数数”。旧 builder 不运行。
2. 将 `+0x10/+0x14/+0x18` binding vector 的长度/容量与 `+0xC0/+0xC4` 实际分配大小对应起来，追踪 post-Init 的 binding 增删/替换与缓冲重分配关系。
3. 判断后续 memcpy/memset 重新计算的 `n` 是否仍受分配时容量约束；审查长度顺序、算术回绕、source/destination 重叠和有效 lifetime。已有局部 NO_OVERLAP 不替代这些其他条件。
4. 特别追踪 cleanup、重复 Init、Base dtor 的直接调用与虚调用。覆写 slot0 不会自动拦截直接调用；继承 slot2 并“不主动调用”不足以证明缓冲不释放。
5. 核对 slot34 的调用者、`+0xE8/+0x368` 边界、active 记录数量与 slot7/deferred list 的联系；区分“协调器不调用”与“游戏不会调用”。
6. 核对默认 binding 两个全局族和 `qword_C7B640` 的静态初始化、已知 writer/地址传播；不能从磁盘初值零推出运行期恒零。

每槽输出以下一种有条件判断：`可继承（列条件与依据）`、`建议中性覆写（列原因与正确 ABI）`、`调用受限（列实际可保证的限制）`、`UNKNOWN（列最小缺口）`。未找到调用点只能写“当前范围未发现”，不能据此标为不可达。

若发现 cleanup 或其他槽可能破坏 `NEVER` 的有效基类状态，给出增加覆写或限制调用的契约建议；同时说明直接调用旁路是否仍存在。不得继续使用“其余 32 槽无条件转发安全”的摘要。

### W05：control block 六槽、计数旁路与一致性

1. 重读六槽每条返回路径，明确零引用、递减后非零、最后 strong/weak、freeFlag 的返回载体和 stack 清理。若 AddStrong/AddWeak 的 EAX/AL 只是 Mutex unlock 尾调结果，说明调用者是否消费，不猜测为固定 bool。
2. 记录 `+4/+6/+0x10/+0x14` 的读写时机与原生锁范围，核对 TryAddStrong 的解锁/转 AddStrong 窗口。
3. 在已知 pair copy/reset/erase/grow/range release/stop 与相关对象族中查找 raw 计数访问、直接 free、mutex inline 路径；补充 32-bit 组合写入、复制覆盖和计算地址的有限追踪。列查询范围、命中和 unresolved 路径，不声称完成全程序 points-to。
4. 区分 reset 为 null 对象建立新原生 control 与为自有非空对象建立第二 control。已有两个 zero-reset caller 不能外推为所有 reset 使用方式相同。
5. 分析 32-bit 原子影子计数、uint16 饱和镜像与 `ctrl+0x14` 的可观察顺序。并发 shadow 更新后再分别写 mirror，可能出现旧 mirror 覆盖新 mirror；要求说明一致性方案或保留设计缺口。
6. 保留有效原生 Mutex/CS 只能满足布局与可加锁条件，不能自动证明原生 inline 访问与自有无锁写入同步；分别说明功能影响。

交付六槽 ABI/状态表、有限旁路证据、root/vector/临时 pair 的计数关系，以及原生与自有语义差异。`NEVER` 降低释放风险，不等于引用获取、指针清零或输入语义已安全。

### W06：key 分配与 F7 唯一性

1. 列 key counter 的已知 reader/writer、调用线程来源、锁、回绕、保留值，以及 Keyboard 固定 key=0 的影响。
2. 追踪 `Player+0x1618`、manager lookup 和相关 UI/controller 列表消费者对 signedness、特殊值与取值范围的假设。
3. 把 F7 拆成三个问题：设备确实在 vector 中；同 key 的成员只有一个；lookup 解析到自有对象。后两者不能合并。
4. 保留静态反例：vector 为 `[self(K), other(K)]` 时，返回首个匹配的 lookup 可返回 self，但 key 仍重复。该反例是控制流推导，不运行实验。
5. 判断计数器 writer 与成员扫描是否受同一同步约束。主线程执行创建、或拿到 manager 锁扫描，都不能在未证明 worker writer 遵守该约束时保证永久无碰撞。

交付 writer/consumer 表、F7 的完整成员判定需求、创建/注册期间和注册后的竞争窗口。若没有可证明的同步方式，列出可选设计及代价，不以“多查一次”“稍后再查”充当消除竞争的方案。

### W07：insert/remove/notification、异常与线程

范围：insert、grow、remove、erase、三个 notification、两类已知 callback、manager update、Gamepad worker、Net/teardown 已知调用链和 assignment。只分析这些对象相关的锁和生命周期边界，不要求枚举整个游戏所有线程。

1. 对按值 pair 标明传参、root 保留、vector acquire、临时 copy 和释放责任；记录通知是同步还是异步、发生在锁内还是锁外。
2. 逐出口分析 vector grow/copy 的分配失败、异常清理、部分发布和通知失败；不能假定每个错误都允许再次调用 remove。
3. 对 remove 写明 object==0、成员、非成员、重复 remove、multiple match 时的行为，确认返回值是否能证明移除；成员判定仍需考虑并发读取安全。
4. 标记 callback 使用或更新了哪些游戏侧 controller/Player 状态，以及可达的 UI、暂停、重分配、Lua 或持久化调用。静态可达不等于实际发生，未知 callee 如实保留。
5. 分别画出 worker create/insert/remove 与 main update/create/remove 的访问序列，确认哪些操作共享锁。对“不持锁 remove + 持锁 update”的问题给出实际同步依据或未决结论，不能以自有 thread token 推断 worker 已序列化。
6. 从 callsite RVA `0x00554F7D` 向主循环追踪 manager update 的阶段、可达线程入口和与 Authority 提交位置的相对顺序；并阅读其他已知刷新调用链。固定生命周期插入位置必须有调用图依据。
7. 对 assignment 分析 `propagate` 的真实 ABI、PlayerManager/其他 Player 的副作用、lookup 的关系；保留已注册未绑定、已移除仍持旧 key、意外原生 erase 等状态。

交付 pair 所有权/异常表、锁与线程来源图、通知副作用表、可用主线程边界及其限制。只从静态调用图推导线程来源，不填写运行时 thread id、频率或竞争是否实际发生。

### W08：RTTI/COL 与 typeid 消费

1. 重读对象与 control 表的 `[-1]`、x86 COL/TD/CH/BCD；记录 signature、offset、cdOffset、绝对指针、基类排列与访问属性。不要把 x64 `pSelf` 加入 x86 布局。
2. 对 Gamepad/Net/Keyboard/Base 的 TD 引用、已知动态 cast 和独立 typeid/type_info 比较追踪其消费者；“没有 TD code xref”不排除按字符串或 vptr 比较身份。
3. 对每个 cast 记录 source/target、null-check 后的实际分支。处理 null 不代表非 Net/非 Gamepad 分支对虚拟设备没有副作用。
4. 比较 Base COL 直指与自建 `[自身, InputDeviceBase]` COL；当前协调器契约选择自建 COL，Base COL 只能作为带差异说明的备选，不能静默替换。
5. 明确对象 COL、control COL、静态字符串和槽代码的存储与模块常驻要求，以及未读取到的消费路径。

交付结构字段表、cast/typeid 消费表和有条件的方案结论。现场 cast/typeid 行为保留为运行期未观察，不把“结构静态合法”写成“所有 RTTI 消费均已验证”。

### W09：原生 Join 设备选择、action 映射与身份

接受“原生 Join 可靠创建/发布 B”的用户规划前提，不重复整个 05-04 manager-this 研究；本项专门回答“自有 action 能否触发该入口，并把自有 key 绑定给 B”。

1. 从 Join 的真实输入查询向设备选择和最终 SetControllerId 追踪，记录 action ID、使用的槽、参数、返回分支、选取顺序、key 来源和冲突设备处理。
2. 识别是否依赖 Gamepad/Keyboard RTTI、平台 handle、特殊字段或物理枚举。若路线 A 存在硬件依赖，列出精确位置，不自动转为路线 B。
3. 对路线 B 单独记录直接调用 SetControllerId 的 ABI、propagate、跨 Player 影响及临时绑定失效，不把 live 原生 reassign 样本外推为任意直接调用安全。
4. 以 T0-04 和现有输入源码为起点，建立 action 表：movement `0..3`、shooting `4..7`、bomb `8`、item `9`、pill/card `10`、drop `11` 是已有资料入口；逐项区分本轮重读和继承结论。
5. 对每项写明 canonical axes/held/edge 到 pressed/triggered/value 的对应、量化/阈值、自然 consumer 和副作用。把 gameplay、UI、Join 窗口、未分类 action 分开；Join ID 必须从代码确认，不能猜为 gameplay action。
6. 确定 binding 数 n、canonical 支持的 action 集和特殊 fallback 的关系；原生 action 越界与自有 snapshot 越界分别说明。
7. 记录 `(session generation, ordinal 1) → virtual key → device → B` 的临时关联，以及 room、death/respawn、reassignment、menu teardown 后的失效规则；不把 pointer/key 写入 canonical 身份。

交付 Join 选择链、路线 A/B 条件、完整 action 映射与未分类项。scheduler exactly-once 的实现与实机结果属于后续输入整合；本项指出多次 poll/query 对该语义的要求，不声称已实现。

### W10：统一结论与契约修订建议

1. 为全部 16 项 O-* 填写：问题、读取范围、直接事实、推导、设计差异、未决项与实现影响。不按章节存在或表格行数判断完整性。
2. 对 L1–L4 逐条写适用范围与最小反例：分派旁路、有效缓冲被 cleanup、mirror/指针状态失配、模块/元数据生命周期等。
3. 给出建议覆写集合与每槽理由。保留现有六槽为基线；需要新增覆写时说明事实和契约差异，未分析的槽不能默认安全。
4. 给出 F1–F8 修订建议，明确真正可处理的失败与无法返回的失败；注册成员、key 唯一和绑定分别记录。
5. 将 slot0 生命周期输出、slot3 名称、slot30 存活状态、query 中性和 control 返回值分别说明，避免把“退休后所有槽中性”写成所有返回值都是 0。
6. 记录主线程静态边界与 worker 同步未决项，不将设计位置冒充已经存在的完整运行时同步机制。
7. 完成全部结果整理与必要的文档修订后，再执行 §8 唯一一次整体静态审阅。

## 6. 证据记录模板与交付文件

建议后续执行完成时新增以下两个文件；本次只创建工作设计，不预先填写确认结果：

- 同目录 `virtual-device-owned-abi-static-confirmation-v2.md`：执行范围、实际目标、16 项结论、ABI/字段/ownership/action 表、契约建议、残余风险和整体审阅结果。
- `out/cs-t0/virtual-device-owned-abi-static-evidence-v2.md`：只读提取的关键指令/数据、xref 命中和局部 CFG，以可读索引供主报告引用。内容较大时可拆成相关主题文件，在主报告列实际路径。

执行时记录查询名称、参数/地址范围、实际工具和证据片段；不记录任何 hash，不创建认证 manifest，不编写 verifier。历史 v1 和失败记录保留；修正以 v2 明确对应旧章节，不覆盖原观察。

每条关键结论按以下模板记录：

```text
问题 ID：Wxx / O-xxx / 对应 v1 的 S-x（如有）
实际来源：工具、IDB/磁盘路径、函数/数据位置
地址：RVA、IDB VA、使用的 imagebase
读取范围：函数、chunk、调用点、data/xref 查询与筛选条件
直接依据：原始指令/字节/数据；必要时附伪代码
调用者如何使用：参数、返回值、pointer/计数/锁的后续行为
结论标签：STATIC_CONFIRMED / STATIC_DERIVED / DESIGN_ONLY / UNKNOWN
适用条件：对象族、状态、线程来源、长度、锁或其他前提
实现影响：保持/建议改动/仍无法给出安全实现的具体原因
残余问题：未分析边、运行期依赖及所需观察，不伪装为本轮事实
```

计数扫描另附命中分类表；ABI 表中的原生事实和自有建议分列。静态反例只是从控制流构造的分析说明，不运行实验来证实反例。

## 7. 原 S-1–S-11 与运行期未决项的承接

| v1 项 | 本轮处理 | 静态结果的边界 |
| --- | --- | --- |
| S-1、S-2、S-10：raw 计数/inline lock/free | W05 有限对象族与编码补查，附实际查询和命中 | 未知计算地址或外部代码仍如实保留；不要求全程序别名闭包 |
| S-3：worker/main vector 并发 | W06/W07 恢复访问与锁关系，说明可选同步设计 | 无法由静态图确认某次运行实际交错或同步方案已生效 |
| S-4：Net/teardown 线程 | W07 追踪已知入口与调用链 | 无法归属的路径仍未知，不要求任意间接调用线程穷尽 |
| S-5：默认 binding 运行期值 | W02/W04 查 initializer、writer、长度与容量 | 静态初值不能证明实际 Run 中的 n 与内容 |
| S-6：全局零 pair 可写路径 | W04 查已知 writer、地址传播与消费 | 未发现写入不等于运行期恒零 |
| S-7：Init 分配失败 | W02 提供 CFG 与真实失败处置范围 | 不进行 fault injection，不声称返回后检查可覆盖内部崩溃 |
| S-8、S-9：自定义 COL/typeid | W08 结构与有限消费分析 | 现场 cast/typeid 行为仍未观察 |
| S-11：未初始化字段 | W02/W04 记录字段与 reader/callee | 无 reader 的查询结论受实际查询范围限制 |

此外单列：key 回绕/碰撞、注册与 assignment 窗口、Join 的设备选择、运行期 callback 注册、副作用、重复 poll 与 authority tick 的对应、迟到 cleanup/Init、mirror 并发一致性。每项都写明会影响哪条实现约束。

若后续任务扩大为运行时实现与整体验收，再按那个完整目标、实际授权和唯一验收约定安排；本文不创建逐项实机测试，也不指定先跑 canary 或预验证。

## 8. 唯一一次整体静态文档审阅

统一入口为一次人工静态审阅：读取 v2 主报告、其证据索引与本轮实际改动的契约/导航文档，一次给出整个推进结果。无需运行程序或校验脚本。

在全部分析与结果修订完成后，整体检查：

1. 全部 16 项 O-* 均有具体回答或带原因的未决说明；每个结论与实际读取范围相符。
2. 原始指令、调用者使用和 ABI 表一致；RVA/IDB VA 分列；未定类型与未读分支如实标注。
3. Init 崩溃路径、F7 lookup 反例、query 参数域/边沿、cleanup/重复 Init、双缓冲容量、worker 同步均得到明确处理。
4. vtable 覆写建议、F1–F8、L1–L4、`NEVER`、模块常驻与协调器契约建议没有互相矛盾。
5. 静态事实、推导、设计与运行期观察没有混用；不出现“列齐 38 槽便完整”“null-check 便无副作用”“主线程便无竞争”等无依据外推。
6. 保存历史事实与失败记录，引用能定位到实际文件；没有恢复 gate、哈希校验、角色分离或多次测试流程。

结果分别说明“本次静态工作是否按范围完成”与“哪些 ABI/实现问题仍未解决”，不生成 promotion、解锁或实现资格字段。审阅发现问题时记录失败与未完成项，不自动重复验收；后续有实质修订时，按新完整目标继续。

## 9. 本文编写结果

`tx-00-02` 的交付为本工作设计及 07-virtual-device 导航更新。本文没有产生新的二进制、游戏或运行时观察；前次核查中的缺项用于安排分析，不因此升级为新的 `STATIC_CONFIRMED`。

本次编写全部完成后，以一次整体静态文档审阅检查现行规范一致性、16 项义务覆盖、引用与修订范围；结果在本次交付说明中报告。
