> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 Virtual Device 自有 ABI 补充静态确认 v5

日期：2026-10-12  
整体推进：tx-01-03（一个完整目标；S01–S07 是内容编号）  
状态：静态分析与文档已完成；唯一一次整体静态文档审阅已完成并通过文档一致性检查。计数候选不完整，ABI/生产实现未完成。  
主证据：[virtual-device-owned-abi-static-evidence-v5.md](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v5.md)  
执行设计：[工作设计 v4](virtual-device-owned-abi-static-confirmation-next-work-design-v4.md) · [契约 v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)

本轮依项目规范执行只读静态工作。没有设 gate；没有读取、计算或校验哈希；没有运行测试、构建、游戏、候选模拟或校验脚本；没有附加调试器、读取游戏进程、输入、注入、Hook、IPC 或修改 IDB。仅新增本报告和证据，并更新 07-virtual-device README。v1–v4 及其失败/历史记录均未改写；没有改生产源码、测试、verifier、机器 decision 或权限。

## 1. 结论与范围

本轮取得了新的直接静态依据并纠正了 v4 的 receiver、manager 表槽号、poll 上层副作用、manager Mutex 重入结论、update EH unlock、current/previous 与 slot21 方向，以及有限计数交错。最重要的结果：

- sub_A1E760 两个直接调用的 ECX 都是栈上输出记录，不是 InputDeviceBase；它不是设备 binding 三指针的 writer。
- global manager 的静态初始化把 vptr 写为派生 Manager 表 0xB82950；派生 slot4 调自身前置后再进入 Base update sub_A1FC00。ManagerBase 表起点是 0xB827F4；0xB827F0 是 COL 地址。
- common 在 poll slot30 前已把 C0 复制到 C4、清状态字段并清 C0。slot30 返回0会让 manager update 通知、压缩 vector、release control 并 end-=8；这些动作发生在 manager lock 内。该路径不调 keyed remove。
- manager Mutex 包装层不是可重入锁。insert 的旧“无 unwind unlock”结论仍成立；但本轮发现 update sub_A1FC00 的 U[1] action 会实际解锁 state1。其他 unwind state 有 terminate action，不能推导为任意故障可恢复。
- C0=current、C4=previous；slot12/14 读 C0，slot13 对比 C0/C4；slot21 末尾是 C4[action]=C0[action]，且该末尾访问在 action<n 检查之外。
- R05 操作表 / I5 / I7 / I9 现采用同一 PINNED Try 规则；I5 算术改为 0xFFFD；I9 改为 A1 候选能到达的最后 strong / 隐含 weak 轨迹。A1 仍不完整：dtor 诊断与锁内槽并发、state/flags/诊断环和 coordinator 的原子访问与内存顺序未定义，镜像观察者保证未闭合，无界自旋活性要求未决；有界版本明确不完整。

直接读取的工具、PE 元数据、逐函数地址、原始字节、receiver 及手工推导见证据 v5 §0–§6。范围外 points-to、实时 vptr/线程、运行期 binding 模板、callback 目标、CRT 行为不因本轮静态读出而闭合。契约 v2 未修改，所有新候选仍为 DESIGN_ONLY。

## 2. V4-Q1–Q7 修正表

| 问题 | v4 原位置 | 本轮直接依据 / 修正 | 影响追踪 |
| --- | --- | --- | --- |
| Q1 A1E760 receiver | v4 §4 writer、§8.4 L2；证据 v4 R02-2/R02-5 | sub_A1E640 与 sub_A1E790 在 0xA1E728/0xA1E889 分别以 ESP+8/ESP+0x10 作为 ECX 调 helper；helper 输出后同一局部地址继续传 slot25。撤回其清设备三指针的结论 | O-CTOR、O-BUF、L2、D3/D8 |
| Q2 poll 返回0副作用 | v4 §6.4、§8.4 L3、§9 D5；证据 v4 R04 | 0xA1F2C2 memcpy(C4←C0)、0xA1F2FB memset(C0) 均先于 0xA1F30A slot30。Base update 在 0xA1FC9C 判0并执行 slot31/32 通知、erase、tail release、end-=8；不是纯提前返回 | O-ABI-78、O-THREAD、O-REMOVE、O-NOTIFY、L3、D5 |
| Q3 PINNED/I5/I7/I9 | v4 §7 / R05 §3.2、I5、I7、I9 | PINNED Try 统一为失败；I5 从 0x10001 减4得 0xFFFD；I9 用候选可达的 S=1/W=1 最后 Release 两次 publish，读者可合成 {1,0} | O-CB、O-RC-INLINE、D2、D6、D9 |
| Q4 Manager 身份和槽 | v4 §4、§8.1 O-KEY/F6/F7、README | 读取 Base/派生 COL/TD/表项；Base 起点 0xB827F4、派生起点 0xB82950；FindByKey 是 slot10/+0x28/sub_A20390；insert 通知 slot30/+0x78/sub_A211D0 | O-KEY、O-NOTIFY、F6/F7、O-THREAD、D4 |
| Q5 manager Mutex 重入 | v4 §5.3、D4；R03 I2/U15 | 读取 0xA71350→sub_A21980→Mutex init、sub_A157F0 busy/Sleep、A1FC00 EH action。wrapper 不可重入；insert unwind 仍无 unlock；update state1 unwind 有 unlock | O-INSERT-ABI、O-THREAD、F6、L3、D2/D4 |
| Q6 C0/C4 命名及 slot21 | 证据 v4 R02-1/R02-3；v4 O-ABI-QUERY/O-JOIN | common 的 C4←C0、C0 清零和后续写入定义 C0=current、C4=previous；slot21 为 C4←C0，末尾无条件访问 | O-ABI-QUERY、O-BUF、O-JOIN、D1/D3/D8 |
| Q7 v4 审阅过度结论 | v4 §10.1/§10.3 | 保留 v4 原文。本轮明确撤回“update 没有 unlock action”“全部 ABI 已完整”类过宽表述；候选和生产功能分别判未完成 | 所有 O/F/L/D 与 README 当前状态 |

## 3. S01：receiver、设备 writer 与 L2

直接调用点、参数、后续消费和字节见证据 v5 §1.1。本轮能限定的 sub_A1E760 直接 code xref 共两处，且两个 receiver 都来自调用者栈局部；因此 v4 writer 表中该函数不再列入设备 binding/vector writer。

设备对象实质 writer 见证据 v5 §1.2：Base ctor 初始化 begin/end/cap 与 C0/C4；slot5 sub_A1E430 从默认模板扩容/复制 binding vector；Init 在虚调 slot5 后按该时刻 n 分配并清零 C0/C4；cleanup 释放双缓冲；slot19 写 C0；slot21 尾部 C0→C4；common 先 C0→C4 再清 C0。slot5 的间接调用与发布后长度变化范围仍 UNKNOWN。

L2 结论：NEVER 仅约束自有对象/control 的协调器释放；它不维持基类缓冲、binding vector、vptr 或字段资源。A1E760 对设备字段的破坏风险已排除，但 cleanup/dtor 与未闭合间接 receiver 风险仍在。不能写成“常驻即资源有效”。

## 4. S02：manager 两表、初始化和槽号

证据 v5 §2 记录了完整 RTTI 身份、初始化及析构阶段。ManagerBase 表起点为 0xB827F4，Manager 派生表起点为 0xB82950；0xB827F0/0xB8294C 是各自 COL。全局对象静态初值 vptr 与构造代码写入分属映像初始状态和静态初始化阶段，不是运行时采样。

| 槽 | 偏移 | ManagerBase | Manager |
| ---: | ---: | --- | --- |
| 4 | +0x10 | sub_A1FC00 | sub_A21CB0 → 前置调用 → sub_A1FC00 |
| 9 | +0x24 | sub_A20240 | sub_A20240 |
| 10 | +0x28 | sub_A20390 / FindByKey | sub_A20390 / FindByKey |
| 11 | +0x2C | sub_A204E0 | sub_A204E0 |
| 27 | +0x6C | sub_A20DF0 / insert | sub_A20DF0 / insert |
| 28 | +0x70 | sub_A20EB0 / remove | sub_A20EB0 / remove |
| 30 | +0x78 | sub_A211D0 / notify | sub_A211D0 / notify |
| 31 | +0x7C | sub_A212C0 / notify | sub_A212C0 / notify |
| 32 | +0x80 | sub_A213B0 / notify | sub_A213B0 / notify |

由此修正 v4 F6 将 slot30 错记为 sub_A20FB0、O-KEY/F7 将 sub_A20390 错记 slot11 的导航和摘要。slot target 对应的通知集合注册目标、线程或现场调用仍是另一个问题。

## 5. S03–S04：poll/update、EH 与 Mutex

common 内部访问顺序、0/非零控制流和 update false 分支见证据 v5 §3.1–§3.2；A1FC00 FuncInfo/unwind map/funclet 见 §3.3；Mutex 初始化/重入/残留锁的地址和轨迹见 §4。

poll=0 的已确认结果为：本次 C4/C0 与状态字段写入已经发生；manager 在同一持锁 update 中通知 slot31、slot32，压缩 pair vector、调 slot3 release、end-=8，然后继续遍历。路径没有调 keyed remove。通知涉及的 callback list +0x34/+0x44 目标、n79_0、assignment/generation/coordinator 的受影响状态无直接映射，标 UNKNOWN。

poll=非零只证明 common 允许继续到 slot32 门控和采样路径，不等于设备仍具备有效资源，也不等于 RETIRING 应返回非零。契约 v2 §4 仅要求 slot+0x78 锁存已发布快照并按 O-ABI-78 返回；没有“RETIRING poll 必须非零”的明文条款。retirement 时选 0 或非零的候选行为及其与自动移除的关系仍待设计，不能伪称该差异已是契约原文。

EH 事实按 state 细分：update 的 state1 action 确实经 0xA64970 解锁；state2–5 的 action 为 __std_terminate。insert 的独立 FuncInfo 无 manager unlock action，继续保留为锁残留风险。对 C++ unwind 的局部清理不能扩大为 AV、进程终止或外部 CRT 行为保证；解锁也不回滚 callback、buffer 或 vector 副作用。

## 6. S05：计数候选和有限交错

原生六槽本轮直接复读与候选复算见证据 v5 §5。候选固定采用“PINNED 时 Try=0”的一套操作规则仅为了内部一致，契约 v2 未明文定义该分支，故仍是 DESIGN_ONLY。

I5 修正为 0x10001−4=0xFFFD。I9 用合法候选序列：从 S=W=1、M=0x00010001 开始，读者先读 strong=1；写者 ReleaseStrong 先发布 S=0/W=1 的 M=0x00010000，再隐含 weak 递减并发布 M=0；读者后读 weak=0，得到从未发布过的 {1,0}。该推导证明候选不给双 u16 load 快照，未证明目标程序的所有 native reader 都存在。

候选尚不完整的具体原因：dtor 不取 self_lock 却写 flags/诊断/中性状态；其与 sticky bit、诊断 ring、计数槽及 coordinator 读取的原子性/内存顺序/所有权未定义。镜像对齐承诺未知；有效的 LOCK RMW 与对齐单拷贝 MOV 是不同实现方案，不能以无效的“lock MOV”代替指令；C++ atomic/编译器协议尚未定义。镜像、object、shadow、state 的分地址发布没有整体观察保证。无界自旋在持锁者暂停时无活性上界，是否符合“不等待主线程”仍未决定；bounded 版本的 Release 获取失败处置明确不完整。

## 7. S06：buffer、query 与容量条件

证据 v5 §6 从地址时序定义 C0=current、C4=previous；slot12/14 读 C0、slot13 读取 C0/C4，slot19 写 C0，slot21 尾部把 C0[action]写到 C4[action]。v4 的 value/Join 方向结论均已撤回。

统一容量不变量：n_bind=(end−begin)>>4；每项 binding stride=16B；common 对 C0/C4 均读写 4*n_bind 字节。固定 n=32 方案须确保32项 binding 确实存在并已初始化、cap_bind≥32、两块 buffer 各覆盖至少32 float、发布后不扩大长度；一般 n 方案按实际 n 初始化元素并分配两块 buffer。n=33、32-float buffer 造成132B访问对128B容量的反例仍成立。slot21 在越界 action 仍无条件访问 C0/C4；Init 前 slot5 会决定当次分配 n，后续 indirect slot5/长度变化闭包 UNKNOWN。

## 8. S07：实现状态及 O/F/L/D 导航矩阵

### 8.1 16 项 O-* 逐项状态

| 义务 | v5 状态与定位 |
| --- | --- |
| O-ALLOC | INHERITED_NOT_REREAD：v4 §3 / 证据 v4 R01；allocator ABI/OOM 旧事实沿用，CRT 对齐、malloc(0)、handler 现场行为 UNKNOWN |
| O-CTOR | 部分本轮直接重读：证据 v5 §1.2；Base ctor 不调 Init、Init 虚调 slot5；派生 ctor/完整生产创建链沿用 v4 |
| O-CB | 本轮直接重读六槽：证据 v5 §5.1；候选扩展字段/同步协议 DESIGN_ONLY 且不完整 |
| O-RC-INLINE | 部分：候选观察范围与 I9 见证据 v5 §5.2；native inline 计数读写和 points-to 全闭包 UNKNOWN |
| O-INSERT-ABI | INHERITED_NOT_REREAD：v4 §5 / 证据 v4 R03；insert 锁残留事实保留。slot30 target 本轮按 Manager 表修正为 sub_A211D0 |
| O-REMOVE | INHERITED_NOT_REREAD：v4 §5 / 证据 v4 R03；本轮用 update 路径比较，自动移除不调 sub_A20EB0，显式 remove 通知先于 lookup |
| O-NOTIFY | 两表 target 本轮直接确认：证据 v5 §2；slot30/31/32 分别 A211D0/A212C0/A213B0；回调 list 目标沿用 R03，未知项仍 UNKNOWN |
| O-ABI-0C | INHERITED_NOT_REREAD：v4 §6/R04；本轮未重读所有具体设备表实现，保持 Base purecall 及具体覆写范围的旧结论 |
| O-ABI-78 | 本轮直接确认 common bool gate 与 poll=0 提前返回：证据 v5 §3.1；native 返回语义不等同 snapshot lock 成功 |
| O-ABI-QUERY | 本轮直接重读：证据 v5 §6.2；C0/C4 地址及 query 访问修正 v4 命名 |
| O-BUF | 部分本轮直接重读：证据 v5 §1、§3、§6；A1E760 排除，真实 capacity closure 与间接 slot5 caller UNKNOWN |
| O-RTTI | manager COL/TD 本轮直接读取：证据 v5 §2.1；device RTTI/downcast 全路径继承 v4 §8 |
| O-KEY | 本轮直接确认槽号：FindByKey=sub_A20390、slot10/+0x28；key 唯一性和动态成员身份 UNKNOWN |
| O-JOIN | 本轮直接重读 slot21 访问方向：C0→C4，证据 v5 §6.2；Join 选择/assignment chain UNKNOWN |
| O-ACTION | INHERITED_NOT_REREAD：v4 §8.1/§8.2；本轮不扩大 action 映射闭包 |
| O-THREAD | update 在 manager+0x14 锁内直接 common，false 分支自动移除：证据 v5 §3；具体 Authority/worker 线程和现场交错 UNKNOWN |

### 8.2 F1–F8

| 项 | v5 定位 | 状态 |
| --- | --- | --- |
| F1 allocator / handler | v4 §3、证据 R01 | 继承；本轮不重做 |
| F2 ctor / key | 证据 v5 §1.2 | Base ctor 新重读；派生构造及现场 key 实值未闭合 |
| F3 自有 vptr | v4 §6、证据 R04；本轮 manager 表见 §2 | 仅 ABI 需求；本轮未实现/未写 IDB |
| F4 binding / 双缓冲 | 证据 v5 §1.2、§6.3 | 当次 Init 尺寸有直接依据；后续长度变更/容量安全未闭合 |
| F5 control / CS / root | 证据 v5 §5.1；布局扩展沿用 R05 | native 布局已读；candidate root/NEVER 实现未生产接入 |
| F6 pair 注册 / 通知 | v4 R03 + 证据 v5 §2.2/§3.2 | target 修正；回调副作用和异常现场仍未知 |
| F7 成员身份 / lookup | 证据 v5 §2.2 | FindByKey=slot10/+0x28 已修正；唯一性未观察 |
| F8 assignment / Join / generation | 证据 v5 §6.2；其余沿用 v4 | slot21 方向直接重读；整体选择链未重读 |

### 8.3 L1–L4

| 项 | v5 状态 |
| --- | --- |
| L1 分派 | 有界结论：manager update 直接 call common，vtable 覆写不能截断它；A1E760 是临时输出 receiver。其余间接 receiver / inline closure UNKNOWN。 |
| L2 资源有效 | 修正 A1E760 误归属后仍有条件：NEVER 不阻止真实 Base cleanup/dtor、vector/buffer 变化或未知 receiver。 |
| L3 退休迟到访问 | poll=0 会触发 manager 自动通知/移除而不只是中性 query；poll 返回策略、重复显式 remove 幂等性和 coordinator/generation 联动未定义。 |
| L4 代码有效 | 生产模块常驻属于已采纳设计前提；本轮没有运行时模块、COL、vptr 或线程观察，也没有生产实现。 |

### 8.4 D1–D9（全部 DESIGN_ONLY，不自动修改契约）

| 差异 | v5 处理与当前未决 |
| --- | --- |
| D1 容量前提 | 保留按 n_bind 覆盖 binding/C0/C4 的注册前提；修正 slot21 方向和无条件尾访问；默认模板运行值、长度是否稳定未知 |
| D2 同步域 | manager Mutex wrapper 直接证据证明不可重入；A1 无界版本不是完整候选，bounded 版本不完整；是否允许等待/自旋待契约决定 |
| D3 覆写集合 | Base Init 会虚调 slot5，common 直接访问缓冲；v4 所提扩展槽仍只是待设计，不能因六槽列表已写即认为闭合 |
| D4 失败 / EH | insert 锁残留仍成立；update state1 EH action 解锁，state2–5有 terminate action；v4“update无解锁”撤回；非C++ fault与CRT未知 |
| D5 poll | native common 以 slot30 返回值作 bool gate，0会进入通知/自动移除；契约 v2 未要求 RETIRING poll 必须非零；两种策略的分歧尚待决定 |
| D6 计数发布 | I5/I7/I9 已修正；packed mirror 不提供两次 u16 load快照，地址对齐与C++原子协议未定；最后 strong 候选可能两次 publish |
| D7 原型 | 原型 FinalRelease/Destroyed 与 NEVER 不同；生产接入状态继承 v4 §8.5，未修改、未采纳 |
| D8 buffer-safe inheritance | 按 buffer 覆盖 n_bind 判定；slot14/slot21 与 C0/C4 的方向由 v5 §6.2更正 |
| D9 原生 control 行为 | native ReleaseStrong 会调用对象删除型析构、清 +0x14 再 ReleaseWeak；自有实现必须与 NEVER 区分。候选 dtor flags/诊断竞争仍未定义 |

## 9. 剩余 UNKNOWN、候选和实现状态

本轮后仍未确认的事实包括：

- 全局默认 binding 模板的运行期 count/内容，发布后 binding vector 是否变化，所有间接 slot5 callers；malloc(0)、CRT 对齐、OOM handler 实际副作用。
- 现场 vptr/COL、设备确切 RTTI、manager update 的实际线程/时机；callback list +0x34/+0x44、n79_0 的全部目标和副作用。
- native inline count reader/writer 全闭包；两次 u16 load 的具体 reader 是否落在目标设备路径；外部线程如何读取 object/shadow/state/diagnostics。
- Join/assignment/selected device、B generation、coordinator 状态到 manager notify/自动移除的映射；RETIRING poll 0 或非0的契约选择；显式重复 remove 的协调器防重机制。
- C++ atomic/asm、memory_order、诊断环并发、sticky flag ownership、state/retire_seq发布协议、持锁者长期挂起时的活性语义。

候选 A1 只有用于比较的 DESIGN_ONLY 操作与手工有限推导，仍不完整；bounded 变体明确不完整；没有实际自有 vtable/control table 实现。离线原型要求 object/control 被最终释放并进入 Destroyed，与当前 NEVER 方向冲突；生产工程没有接入该原型。本轮没有实现真实设备、最小 Join 或 retirement/recovery，T0-05-05 不得标为完成。

## 10. tx-01-03 唯一一次整体静态文档审阅

在本轮所有分析和文档编辑完成后，仅进行一次人工静态文档审阅：读取本报告、证据 v5、工作设计 v4、协调器契约 v2、07-virtual-device README。未运行任何测试、程序、构建、模拟或校验脚本；没有第二次独立验收。

| 检查 | 结果 | 判据 |
| --- | --- | --- |
| receiver / 资源 | 通过（范围有界） | A1E760 的栈 receiver 与 device/control/manager 区分；writer 与 L2 条件一致；间接 receiver 仍标 UNKNOWN |
| manager / ABI | 通过 | Base/派生表、COL/TD、初始化阶段、slot/offset/target、派生 update 链统一；现场 vptr/线程仍标 UNKNOWN |
| poll / EH / 恢复 | 通过 | common 先 copy/clear 后调 poll；返回0的 notifications/erase/release/end update 和锁域可追溯；A1FC00 state1 unlock 与 terminate states 分列；未承诺回滚 |
| Mutex | 通过 | 实际初始化/包装 busy flag/sleep/unlock 有依据；insert residual lock 与 update EH cleanup 分开；没有把包装锁推为可重入 |
| 计数候选 | 通过（文档一致，候选不完整） | PINNED/I5/I7/I9一致；I9轨迹可由候选操作达到；mirror、dtor/diagnostic、observer及活性未定义均明确留为未完成 |
| current / previous / 容量 | 通过 | C0/C4 时序与查询、slot21、n=0/20/32/33、两块buffer不变量一致；未知长度变化未消去 |
| O/F/L/D、导航与状态 | 通过 | 16 O、F1–F8、L1–L4、D1–D9在本报告有可定位状态；历史 v4未改；README链接本轮当前状态；没有将静态交付等同生产完成 |

审阅结果：文档的事实边界、内部编号和导航检查通过。该通过只表示本轮文档按规定边界相互一致；不代表所有 ABI 已验证完整、不代表候选算法完整，也不代表真实设备或 T0-05-05 已完成。没有未记录的本轮审阅失败项；v4 历史错误按 §2 保留并给出撤回依据。
