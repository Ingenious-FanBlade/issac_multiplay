> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 下一步静态确认工作设计 v5

日期：2026-10-12  
设计交付推进：`tx-01-05`  
静态执行推进：`tx-01-06`（一个完整目标，P01–P06 是功能内容编号）  
状态：`DESIGN_ONLY / 功能范围设计；静态确认 v6 待执行`  
执行者：用户指定 **GPT-6-Luna、max** 子智能体。

先读[现行项目规范](../../../../../project-conventions.md)。用户本次要求按正常功能路径安排静态核验，不考虑安全问题；本文据此限定分析范围。现行隔离、统一输入、线程和已采纳生命周期约定继续作为设计背景，不因范围缩小而宣称它们已被验证。

## 1. 完整目标与预期结果

回答：在既定 Offline/local-co-op 场景中，一个自有 InputDeviceBase 派生对象以自有 control block 注册后，原生代码怎样枚举它、接受其 Join/confirm 输入、把它的 key 绑定给 B，并从自有快照获得 B 的输入；正常停用怎样撤销这些关联。

功能主线为：正常创建与注册 → 原生枚举/选择 → Join → key/Player assignment → 输入消费 → 正常停用。必须区分 InputManager active pair vector、update 的辅助 pair 表、PlayerManager 的四项 Join selector；它们是否互相依赖由实际数据流判断，不能因都被简称为 selected 就视为同一容器。

本轮复用既有有效证据，并复读直接决定这条主线的原始指令。特别是[静态确认 v2 §10/W09](virtual-device-owned-abi-static-confirmation-v2.md)和[W09 证据草稿](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w09-draft.md)已有 Join 链、action 立即数及 assignment 分析；不能把“v5 未重读”解释为“此前从未分析”。要核对旧结果与当前 S2 私有快照方案是否兼容，纠正受后续事实影响的部分。

预期交付是功能范围的静态确认 v6、证据 v6、准确的目录入口，以及：

- 六条路径的函数/调用点、对象关系、正常分支条件与必需字段/槽。
- 普通路线 A 的最小功能调用顺序和自有 ABI 覆写建议；事实与设计建议分别标注。
- 已知可复用项、实际补读项、剩余具体功能缺口和只能运行期观察的值。
- 一次整体静态文档审阅的真实结果。文档交付完成、功能链明确、实现完成和实机成立分别报告。

`tx-01-05` 只交付本文和目录入口，完成后一次人工整体静态文档审阅。`tx-01-06` 执行全部 P01–P06 并整合报告，在全部分析和编辑后一次人工整体静态文档审阅。二者是不同交付目标，不按 P 编号拆分验收，不运行过程测试。

## 2. 正常路径条件与范围

本轮在以下分析条件下推导功能：分配成功，已构造对象/缓冲有效，所分析操作按正常顺序执行；相关 key 在所选场景中可用于唯一关联；Offline 分支的 online roster 为空。条件必须列在报告中，不改标为本轮运行时观察，也不外推为所有模式成立。

继承[原生 Join 用户规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md)：既定场景下原生 Join 可以创建并发布 B。它仍是 `USER_ACCEPTED_PLANNING_ASSUMPTION / DESIGN_ONLY`。本轮追虚拟设备到该调用入口及其 key、ordinal 关联，不重开完整 Player Init、持久化或 manager-this 全程序证明。

### 2.1 本轮需要分析的功能条件

- 正常构造/Init/注册的 ABI、字段和顺序；control 正常 Try/copy/Release 返回值及调用方的消费。
- binding 的实际正常使用方式、必要元素及内容来源。固定读取 32 项的注册路径需要 32 项真实元素；只预留容量不等于元素存在。分析其功能用途，不重做越界/OOM 反例批次。
- poll/common/update 的正常返回分支、选中条件、字段更新与通知；它们是否能看到 S2 私有快照。
- 路线 A 实际使用的 query/flush、名称与 RTTI 分支、Join action、key、selector、assignment 和 gameplay consumer。
- 正常停止输入、移除、selector/assignment 解除、B 中性或解绑的功能动作。

### 2.2 本轮不展开的内容

OOM/Init 失败策略、EH/unwind/terminate、锁残留与死锁恢复、worker 并发、镜像原子发布与内存序、计数溢出/下溢全证明、全映像 alias/points-to、未来模块、ABA、持久化排除与异常退休不属于本轮目标。已知问题保留引用和范围标签，不改判为解决，不把它们加入 P01–P06 的完成条件。

不实现生产设备、覆写表、coordinator 或 scheduler，不修改契约、生产源码、测试、verifier、机器决策或权限。完整双 Endpoint 输入隔离、网络/队列/去重及 exactly-once 总证明属于 05-06；本轮只明确最小 Join 和设备快照消费所需接口行为。

已采纳的自有对象 38 槽、自有 control 6 槽、S2 action 层覆写、`NEVER`、DLL 常驻、每 session 至多一次创建、路线 A 为主继续作为设计背景。NetInputDevice、Online Manager、FrameStore 不进入 C/S 正向实现；相关静态比较只用于识别分支。

## 3. 输入资料、旧结论与定位种子

### 3.1 资料入口

| 资料 | 本轮用途 |
| --- | --- |
| [项目规范](../../../../../project-conventions.md)、[架构规划](../../../../../isaac-cs-architecture-plan.md)、[T0 任务书](../../../../isaac-cs-t0-task-plan.md)、[cs-t0 入口](../../../../../../../cs-t0/README.md) | 当前工作约定及功能边界；历史流程不执行 |
| [契约 v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)、[设计修订 v1](../../../07-virtual-device/design-revision/virtual-device-owned-abi-design-revision-v1.md) | S2、只读 query、快照、38/6 槽、路线 A/B 的已采纳设计 |
| [静态确认 v2](virtual-device-owned-abi-static-confirmation-v2.md)、[W09 证据草稿](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v2-w09-draft.md) | 已有 Join/selector、action、key/assignment、正常解绑及 gameplay 映射 |
| [静态确认 v5](virtual-device-owned-abi-static-confirmation-v5.md)、[证据 v5](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v5.md) | receiver、表身份、normal common/poll、C0/C4 等修正；不能依旧审阅通过文字推定完整 |
| [静态确认 v4](virtual-device-owned-abi-static-confirmation-v4.md)、[证据 v4](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v4.md) | 正常构造/Init/注册/binding 的原始细节；引用时同步应用已确认修正 |
| [T0-04 handoff](../../../../t0-04/isaac-cs-t0-04-reverse-handoff.md)、[input_core.hpp](../../../../../../../cs-t0/native/include/isaac_cs_t0/input_core.hpp)、[active_apply.cpp](../../../../../../../cs-t0/native/src/active_apply.cpp) | 现有 canonical/gameplay 映射、自然 consumer 与查询语义；源码只读 |
| [原生 Join 规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md)、[目录入口](../README.md) | 用户已接受的创建/发布前提及历史/当前状态 |

### 3.2 本会话与旧证据的种子

下表是复读入口，不是 `tx-01-06` 新证据。地址为旧 IDB VA，资料 imagebase 为 `0x400000`；执行者应记录自己的实际读取位置、原始指令与 RVA。

| 主题 | 种子 | 需要核对的关系 |
| --- | --- | --- |
| 构造/Init/binding | `A1E050`、`A1E210`、`A1E430`；设备 `+0x0C/+0x10/+0x14/+0x18/+0xC0/+0xC4` | ctor 不调 Init；Init 虚调 slot5，按 binding 数分配；normal 注册仍消费原生字段 |
| manager 正常路径 | 全局 `C57B18`；Base 表 `B827F4`、派生表 `B82950`；`A21CB0→A1FC00→A1F280`；insert `A20DF0` | 派生/基类阶段、active vector `+8/+0xC/+0x10`、按值 pair、正常通知顺序 |
| update 非零分支 | `A1FDD3–A1FEA9`；slot7/+0x1C 于 `A1FDE4`；辅助表指针 `manager+0x20`、计数 `C799F4`；`A1FE86` 置设备 bit0；正常解锁 `A1FEC2` 后的 `manager+0x3C` 通知 | 这张辅助表的消费者，以及它与 Join active-vector 枚举的真实依赖 |
| Base 活动判断 | slot7 `A1E4B0`；`A1EED0` 再虚调设备 slot31/+0x7C；原生 masks/C0/内嵌表 | 本会话复读见其读取原生状态/缓冲和采样槽，不能直接等同自有 action query；是否影响路线 A 是待验证问题 |
| 注册记录/名称 | insert notify slot30 `A211D0`；已知注册点 `952C36→95B290`；映射 `835C90`；名称 slot3/+0x0C；设备记录 helper `A1E640/A1E790/A1E760` | binding/name/RTTI 的正常消费；`+0x94` 是 slot37，`+0x64` 是 slot25；helper 两个已知 receiver 是栈临时输出 |
| Join 帧入口 | `954CD0` 中 `954F7D` manager update、`954F84→95B310`、`9551BD→6FA540`；`6FADA7→9BAC40` | 正常帧内顺序、state/Offline/roster 条件；不把帧直接当 Authority tick |
| Join 设备枚举 | `A20240` 按下标复制 pair；Manager slot29/+0x74 `A20FB0`；slot10/+0x28 `A20390`；`A209E0/A20940` | active-vector 枚举与 keyed dispatch；device receiver 的 triggered/pressed 调用 |
| Join 动作/创建 | `9BAC40`；旧 W09：19 加入、14 确认、15 取消、20/21 导航；`9BB0B8` call device+0x54(19)；`9BB415→9B9CD0`、`9BB424→7A6450` | 立即数、slot/receiver、selector 修改、普通创建分支；不把 slot21 flush 当 Join 查询 |
| key/ordinal | `9BAD2C` 读 device+0x0C；`9BAAA0` selector；`7A6598` 写 Player+0x1618；Player+0x161C 来自主 vector 旧长度 | key→selector→Player→lookup；ordinal 1 的正常条件 |
| 正常解绑 | remove `A20EB0`；通知 `A212C0/A213B0`；`95B310` 中 `95B87F` 解绑与 `95BCA0` 重绑 | 移除是否同步撤销辅助表/selector/Player key；virtual 类型是否进入原生断连分支 |

### 3.3 对已有状态与错误的处理

上一子智能体交付回报确认：`tx-01-03` 的唯一文档审阅实际未通过，原因包含 README 当前标题重复及报告预填通过与实际结果矛盾。v5 正文仍保留通过文字。本轮不重跑其审阅，不覆盖 v5；新报告和当前入口须如实记录这个差异。

继承 v5 时应用已知勘误：`+0x94` 为 slot37，`B827F0/B8294C` 是 COL 指针存储位置，实际 COL 对象为 `BB033C/BB0424`。v5 将 update 非零分支简化为继续迭代的表述不能用于本轮功能推导。EH、Mutex OOM、计数安全等其他缺口只引用范围外记录，不展开补证。

此前会话把 Join 链概括为全部未补齐，忽略了 v2/W09 已有分析；本设计纠正此范围判断。slot7 可能挡住 S2 设备选择仅是候选推断：若 Join 直接枚举 active vector、无需 update 辅助表，则应明确排除其作为 Join 必需条件，不能预先要求新增 slot7 覆写。

## 4. 执行内容

### P01：正常注册、原生枚举与选择的关系

1. 用原始指令复读正常构造/Init/控制块绑定/insert 的接口，记录 this、按值 pair、栈参数、清栈、正常返回和字段发布顺序。已确认布局与分配成功路径可继承；只补与后续消费相关的差异。
2. 正常 pair copy 的 Try/Release 与通知返回值要按调用者消费解释，不把它们统一解释为一般成功/失败；不展开溢出和并发候选。
3. 从 insert notify、派生 update/common 的非零路径追到辅助表写入、设备 bit0、本地 pair 集合及解锁后通知。明确这些表的已知消费者。
4. 复读 Base slot7/A1E4B0、A1EED0 与相关采样槽；列出 S2 自有快照与原生 masks/C0/轴值之间实际有没有连接。
5. 与 P03 的 A20240、A20FB0、9BAC40 对照：Join 取 active vector、辅助表还是两者？哪些字段/槽确实使路线 A 进入候选？区分仅影响设备活动标识、其他 UI 与直接影响 Join 的条件。

交付：三个容器的地址/布局/producer/consumer 表；正常注册→枚举/选择的调用链；slot7 对路线 A 的确切作用。若现有六槽 S2 方案无需 slot7 即可 Join，应给直接路径依据；若需要扩写，给具体消费者与候选行为，保持 DESIGN_ONLY。

### P02：自有类型、名称与 binding 的功能兼容

1. 沿真实 insert/update 通知到 95B290/835C90 等已定位 consumer，核对其对名称、key、类型与 binding 记录的使用。只展开与本设备正常功能有关的分支。
2. 验证相同偏移的 receiver 类别。A1E760 的临时输出记录、设备 binding、record binding 及 manager 容器分别列出，避免重复旧归属错误。
3. 确认当前 38 槽表在这条功能路径实际调用哪些槽，尤其名称、slot25/37 的输出及分类、Init/slot5、query/flush。未知参数用原始压栈与返回消费推导，不按伪代码函数名确定签名。
4. 确认正常注册需要哪些 binding 内容、元素数量及模板来源；区分 device 每 action 四个 primitive 与 record 每 action 两个 primitive，区分 action 索引与 primitive 标识。私有快照绕过查询实现，不代表注册记录不再读取原生字段。
5. 对路线 A、注册和相邻正常重选的相关 RTTI 分支逐点判断：自有 Base 派生类型进入哪个分支？转 Gamepad/Keyboard/Net 为 null 时怎样继续？不扫描所有无关 cast。

交付：实际被消费的槽/字段/记录最小清单、类型分支表、正常 binding 初始化要求。运行期模板内容未观察就列为具体数据条件，不能把镜像初值当运行值。

### P03：复核普通 Offline Join、confirm 与 selector 路径

1. 复读旧 W09 的 954CD0→6FA540→9BAC40→A20FB0→A209E0/A20940 链，核对 Manager 表身份、slot29/+0x74、keyed/全设备枚举、device receiver 和 AL 消费。按关键调用点附原始字节。
2. 复核 Join action 19、confirm 14、cancel 15、导航20/21及各自 query 槽。possessor 的19+8+10仅作分支对照，不扩大本轮最小普通 B 路线。
3. 列普通路线的实际条件：帧状态、roster、Player/selector key 占用、selector 位置、角色列表及会改走其他模式的相关字段。对 706940、9BD3B0、LABEL_69 只分析其是否影响所选普通分支；能通过明确正常条件排除的部分说明理由。
4. 复核 9BB0B8 的 device+0x54(19) 语义。按 v5 正确方向 C4←C0 分析其对原生 previous 的作用，以及它是否能够影响自有私有 previous/Join edge。不继承旧 current←previous 命名。
5. 从普通 confirm 接到创建入口9B9CD0及assignment调用7A6450，沿 selector 状态列出最小 Join 动作序列。旧 W09称同一 pass 可以 select后confirm，必须复核，不预设要两次自然 pass。
6. 记录重复 query、重复 Join pass、action19/14持续为真时的正常结果；这是重复创建的功能语义。不执行脚本/模拟，不完成05-06总去重证明。

交付：普通 Offline Join CFG、action/slot/ABI/receiver 表、条件清单、selector 字段变化与最小调用序列；继承的原生创建假设与新静态事实分开。

### P04：设备 key 到 B 与输入查询的完整关联

1. 从 device+0x0C 追到 selector item+0、普通创建的参数和SetControllerId写点，记录key在各处的宽度、符号/特殊值用法。
2. 复核 SetControllerId 的正常 ABI、retn8、propagate参数、Player+0x1618写入、相关正常传播。旧证据指出返回AL未归一化、caller忽略返回，不能新设计成AL成功判据。
3. 复核 Player+0x161C 的来源与主vector append顺序。明确“创建前恰有A一个Player”怎样使新B成为ordinal1；native不强制ordinal1的事实保留。
4. 从已定位的B gameplay consumer反向追key→Manager keyed lookup→device，不展开27个无关SetControllerId caller或全部Player reader。
5. 把设备成员、selector、B controller key、ordinal及HUD分别记录；若其正常更新阶段不同则列出阶段，不能因同key直接视为同一对象或同时发布。

交付：`virtual device → key → selector → B.controllerId → lookup → device query` 数据流及正常顺序；列出实现如何辨认当前B的字段，pointer/key只作临时关联。

### P05：自有快照到实际输入消费者

1. 复用T0-04与旧W09的gameplay action表，核对movement0–3、shooting4–7和现有bomb/item/pill-card/drop consumer与canonical映射；只补当前自有device实际接入点。
2. 记录pressed/triggered/value的正常语义与ABI（AL、x87 ST(0)、action栈参、ret4），Float/Q15转换及实际阈值/方向条件。旧A5F230/A5F370无action栈参的wrapper不能作为device query caller。
3. 追原生consumer是否经过三个自有action槽，或还读C0/C4、raw masks、axis、binding或其他继承槽。结合common先C4←C0、清C0再poll的顺序，画出私有快照与原生状态的正常时序。
4. 把Join action19/14与gameplay发布分开：何时让原生Join观察到边沿、何时关闭Join输出；slot21原生flush是否更新私有previous；正常取消/完成后怎样回到gameplay输入。
5. 契约规定query只读，不能直接把“每次query消费一个事件”当作已采纳实现。若最小Join需要一次性边沿、pass级锁存或额外状态转换，给出具体正常查询轨迹及候选接口，标DESIGN_ONLY/契约差异。普通同一快照被多次读取与跨Join pass重复spawn分别处理。
6. 定位快照publication/poll/query可用的正常阶段；没有Authority tick对应依据时记录精确接口缺口，不把渲染帧等同tick，不重做T0-03/04整体或双Endpoint语义。

交付：action→snapshot字段→native consumer/slot表；common/poll/query/Join的时序图或表；最小功能覆写集合与设计差异。普通输入及Join接口仍未确定处给具体地址/原因。

### P06：正常中性、移除与解绑

1. 在正常完成/停止条件下追关闭Join/gameplay publication、中性query、cancel selector的动作和调用顺序；poll返回值与自动移除的正常结果按既有事实补相关消费。
2. 从remove A20EB0的正常通知/查找/erase路径追active vector及已确认的辅助表、selector、assignment记录更新。本轮不分析锁、异常或失败恢复。
3. 追95B310相关正常断连/重选分支对自有RTTI的处理；判断设备移除是否会使B正常解绑，不能把“active vector中不存在”当作“Player key已清除”。
4. 若协调器需要额外cancel或SetControllerId(-1,propagate)动作，列出正常ABI和字段结果，并与路线A的自然解绑区分；不要求原生物理删除B才能正常停用。
5. 结合NEVER定义正常停用后的功能状态：自有query中性、设备成员撤出、Join输出关闭、B解绑或退出、模块与有效对象状态保留。进程退出、迟到别名及异常退休安全不纳入本轮。

交付：正常停用顺序、各容器/字段的前后状态和必要coordinator动作；只能确认部分时逐项标注，不把中性输出等同完整解绑。

## 5. 整合与报告要求

执行顺序建议为旧W09索引→P01/P03容器与入口关系→P02正常兼容→P04身份→P05消费→P06停用；允许沿实际依赖调整。P01与P03共同确定slot7作用，不先把它写成必需覆写。执行者完成所有必要编辑后再整体审阅一次。

报告必须包含：

| 内容 | 必须给出的结果 |
| --- | --- |
| 已有分析复用 | 哪些来自v2/W09、v4/v5、T0-04；哪些本轮直接复读；旧结论的实际修正 |
| 六条功能路径 | 正常条件、函数/原始调用点、字段/槽、返回消费、具体输出和未完成处 |
| 容器区别 | InputManager active vector、update辅助表、PlayerManager selector各自用途与相互依赖 |
| 最小路线A | 正常调用顺序、Join/confirm/cancel/action映射、key/ordinal关系、需要实现的自有槽/状态 |
| 设计差异 | 现有六槽S2是否足够；slot7/slot21等是否真的需要变化；query只读与Join边沿的接口建议 |
| O/F导航 | 关联到O-CTOR/O-CB/O-INSERT-ABI/O-NOTIFY/O-KEY/O-JOIN/O-ACTION/O-RTTI/O-ABI-QUERY/O-BUF/O-REMOVE及F1–F8；范围外项写本轮未核验，不重建安全完成率 |
| 真实范围 | 剩余静态问题、未观察运行值、设计候选、未实现功能分别列出 |
| 历史事实 | v5实际审阅未通过、预填通过矛盾和已有有效证据同时保留；不重写旧报告 |

一条结论的模板：`P编号/问题 → 来源与新旧证据标签 → VA/RVA/函数/原始字节 → receiver与参数 → 分支条件/字段读写/返回消费 → 正常功能含义 → 适用条件/剩余未知 → 自有实现建议（如有）`。

没有静态来源的runtime callback目标、默认模板内容、当前roster/player count/vptr/thread和实际游戏结果保留UNKNOWN。没有现成静态writer时给具体读取/注册点及可分析边界；不要无限追全映像或以常量猜动态目标。六条路径仍有缺口时可以准确交付报告，但不得写成六条功能均已验证完整。

## 6. 工具、文件与执行交接

执行者先读`C:\Users\114514\.codex\skills\isaac-cs-refactor\SKILL.md`，按实际ALL_TOOLS发现IDA MCP并读取schema。旧input为`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`、IDB同名`.i64`、imagebase`0x400000`；记录当前实际路径/元数据，不读取、计算或校验任何哈希。

允许资料检索、只读decompile/disasm/xrefs/字节/整数/元数据、纯读IDA Python、静态手工数据流和文档编辑。不得patch/rename/set_type/define_func/保存IDB；不得执行候选、模拟器、测试、校验脚本、harness、probe、smoke、canary、构建、旧verifier。不得启动或检查游戏进程、附加调试器、读取进程内存、输入、注入、Hook、IPC或运行期修改。

新交付文件：

- 同目录`virtual-device-owned-abi-static-confirmation-v6.md`，标题和摘要明确“正常功能路径”；取得实际分析后创建，不预先填充通过。
- `out/cs-t0/virtual-device-owned-abi-static-evidence-v6.md`，必要时增加v6功能主题分册并链接真实文件；保留工具读取范围和原始证据。
- 更新[07-virtual-device README](../README.md)当前入口与实际状态，保留本设计及历史结果。

设计v1–v4、确认v1–v5、旧证据与失败记录不覆盖。生产源码、测试、contract、decision/权限文件不编辑。工作区已有其他修改和删除，不恢复、整理或覆盖它们。

主智能体交付本文并完成设计文档审阅后，启动一个`model=gpt-6-luna`、`reasoning_effort=max`、新上下文子智能体执行`tx-01-06`。此为用户指定分工；执行者不再创建子智能体。主智能体交出IDA只读使用权，不并发操作IDA或编辑执行者的新报告。此前IDA只有只读观察，没有patch、保存、调试器附加或断点变更。

工具不可用时如实记录缺失，继续资料分析；没有新二进制读出就标继承，不能伪造直接确认。指定模型启动失败如实报告实际错误，不默换模型。

## 7. tx-01-06 唯一一次整体静态文档审阅

统一入口：全部分析和文档编辑完成后，人工一次阅读v6主报告、证据/实际分册、本文、相关契约条款及README，检查下表；不运行程序、游戏、构建或校验脚本。

| 检查 | 判断内容 |
| --- | --- |
| 正常功能覆盖 | P01–P06逐项有实际结果或具体未完成位置；未把安全范围外事项重新加入目标 |
| 事实与ABI | receiver、槽号、offset、action、清栈、返回消费、C0/C4、key/ordinal和三种容器一致 |
| 旧W09复用 | 已有Join事实被合理复读/继承，未重复假设未知；派生/基类表和后续勘误应用准确 |
| S2兼容 | 私有快照到选择/Join/gameplay的路径、必要覆写、flush与query只读差异真实记录 |
| 范围与设计 | 规划假设、静态事实、候选、运行值与未实现分别标注；未宣称安全或生产已完成 |
| 状态与导航 | 主报告、证据、README和最终回报采用同一真实结果；历史失败和修正原因保留 |

逐项记录实际结果，再分别回答：文档交付是否准确、正常功能链明确到什么程度、实现还需哪些接口/设计、剩余静态或运行值缺口有哪些。UNKNOWN记录准确仅表示范围真实，不自动说明对应功能成立。

**必须在审阅后回填真实结果，包括失败结果。** 主报告状态行、审阅节、证据状态、README和子智能体最终回报须一致；这项事实记录不是修补失败项或再次验收。不得预填PASS；不得因“失败后不自动修补”而保留已知虚假的通过文字。发现失败后不自动修复或重跑，记录失败项与未完成工作。

## 8. tx-01-05 设计交付记录

本次新增本文并更新目录入口，整理旧W09与本会话只读观察为执行种子；没有新增本次执行证据，没有修改旧报告、契约或生产代码，没有运行程序、构建、测试或实机操作。全部编辑后只做一次人工整体静态文档审阅，实际结果在本节回填。子智能体执行结果另由tx-01-06报告。

设计整体静态文档审阅：**通过（设计交付范围）**。全部编辑完成后人工一次通读本文及目录当前入口，核对六条功能路径、正常条件与排除范围、旧W09复用、槽号/三种容器的区别、资料引用、tx编号、指定模型与交付责任，确认相互一致。旧v5实际审阅失败在新设计和当前入口均如实记录，历史原文保留。此结果只说明本设计可供执行，不表示静态确认v6已完成、正常功能已实现或已得到实机结果。未运行程序、构建、测试或校验脚本，未进行第二次审阅。
