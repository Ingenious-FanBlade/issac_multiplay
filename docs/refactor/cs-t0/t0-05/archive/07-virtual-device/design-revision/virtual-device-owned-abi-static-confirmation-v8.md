> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# 自有 Virtual Device ABI 正常功能静态确认 v8

日期：2026-10-12。目标：接续 tx-01-12，完成工作设计 v7 要求的正常功能静态确认。原 v8 子智能体启动异常后，由替代执行者继续同一目标；未发现可接续的 v8 草稿或实际审阅记录。README 中的原异常记载保留，重启不增加审阅次数。执行依据为[现行项目规范](../../../../../project-conventions.md)和[完整工作设计 v7](virtual-device-owned-abi-static-confirmation-next-work-design-v7.md)。

**结论分开记录：**本报告纠正了 v7 的事实错误，列明正常链的静态依据和明确缺口；这不等于正常功能已在生产代码或游戏中接通。原生 Join 创建并发布 B 仍是已接受的 DESIGN_ONLY 规划前提。当前六槽 C/S gameplay adapter 没有 Join 事件，也没有把所有 native cross-family 查询接到输入映射。生产实现与运行值仍未完成确认。

## 证据等级、目标与边界

- IDA-DIRECT：本轮从当前只读 IDB 直接读取的指令、反编译、数据或交叉引用。
- SOURCE-DIRECT：本轮直接阅读的 C/S 源码；不代表 IDA/native binary 事实。
- STATIC-INHERITED：沿用 v1–v7 或 T0-04 handoff、且未在本轮重读的事实。
- CONTRACT：契约 v2 的规范性要求；不代表 binary 事实或生产实现。
- STATIC-DERIVED：由具名指令与可见条件推导出的正常分支行为。
- DESIGN_ONLY：待接口/生命周期设计决定的候选，不是原生事实或现有实现。
- UNKNOWN：只有在具体静态边、运行时值或语义尚不能决定时使用，并写出缺失点。

IDA 当前目标为 F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe，IDB 为同目录 isaac-ng.exe.i64，imagebase 0x400000，PE32 i386；本轮健康探测显示 IDA 自动分析与 Hex-Rays 可用。地址统一用 IDA VA，表中另列 RVA 时以 VA−0x400000 计算。本轮只读使用 health、bytes、反编译、反汇编及 xref；没有 patch、rename、改类型、定义函数或保存 IDB。没有启动/检查游戏进程、读进程内存、附加调试器、输入、注入、Hook 或 IPC。

正常分析条件为成功分配、原生对象和缓冲有效、顺序操作、Offline/local-co-op roster 为空、virtual key 唯一、创建 B 前主 Player vector 只有 A。保留既定的自有 38 槽 device、六槽 control、S2 快照、NEVER、构造前固定的 DLL vtable/模块并每 session 至多一次创建；原对象/control 和模块保持进程常驻。C/S 与原版/Isaac Sync 保持隔离。此范围不展开 OOM、EH/unwind、锁/worker 并发、原子镜像或计数溢出、ABA、全映像 alias、持久化和异常恢复，不重开 05-06 的完整隔离证明。

## v7 勘误与证据来源

| 事项 | v8 结论 | 依据归属 |
| --- | --- | --- |
| Join action 编号 | 9BB2FE 的立即数 0x15 是十进制 action 21 导航；取消 action 15 在 9BB3C2 的立即数 0x0F。确认 action 14 在 9BB393；action 20 导航在 9BB2B0；Join action 19 的旧证据继续有效。 | 21、15、14 的当前指令为 IDA-DIRECT；19、20 的主体路径为 STATIC-INHERITED v7/W09 并有限复读。 |
| Scalar 多层 ABI | device +0x38 返回 x87 ST(0)，ret 4；A20AB0 消费 ST(0)，写调用者 float 输出指针，并以 AL 返回“非零”bool；9562D0 从输出指针读 float 到 XMM0，ret 16。AL 不是 scalar。 | 当前 A1EA20、A20AB0、9562D0 指令/反编译为 IDA-DIRECT。 |
| 固定 action 表 | BAB240 的 4 个表项固定为 0、1、2、3；BAB370 固定为 4、6、5、7。不能把它们描述为动态 action 或跨 family。 | 当前函数指令和表数据为 IDA-DIRECT。 |
| Mutex 与通知 receiver | A20E4C 经 Manager+0x14 的 embedded Mutex 解锁；A20E55 后才以 Manager this 调 Manager slot30/+0x78 通知。两个 receiver 身份不同。 | 当前 bytes/反汇编为 IDA-DIRECT；v7 的 insert 通知身份为 STATIC-INHERITED。 |
| 六类 gameplay 查询 | 每类列明具体函数、动作常量/分支、key、pressed/triggered/scalar 通道和消费返回值；另列跨 family 与 action11 held 需求。 | 本轮 consumer 函数/调用点为 IDA-DIRECT；T0-04 对 query 总体语义和部分 runtime witness 为 STATIC-INHERITED。 |

有效构造/binding/key 链沿用[报告 v7 R01–R02](virtual-device-owned-abi-static-confirmation-v7.md)，不重复整个旧研究：正常 _main 分支的 sub_4307C0 构造 32 项 generic 与 32 项 keyboard 默认 binding template；C799E8/EC/F0 是 generic vector，C799F8/FC/C79A00 是 keyboard vector；C799F4 是独立 update auxiliary count，初始化常量 4，不能混作 binding 长度。A1E430 由 generic template 向对象 +0x10/+0x14/+0x18 复制，A1E210 按对象 vector n 分配并清零两份 4*n 的 C0/C4 buffer。A1E050 的 ctor 以立即数 B82758 写 Base vptr，以 C7362C counter 分配 device key 并写 +0x0C。以上为 STATIC-INHERITED v7；本轮未宣称观察到当前进程是否走该 _main 分支、对象当前 key/vptr 或最终 template 内容。

## S01–S06 执行映射

| 设计项 | v8 完成位置 | 主要状态 |
| --- | --- | --- |
| S01 action 编号、receiver 与三层返回 ABI | 勘误表、P02、证据 E01–E02 | 纠正action15/21；区分device ST(0)、A20AB0输出指针/AL、9562D0 XMM0；Mutex/Manager receiver有直接与继承边界。 |
| S02 固定方向表与六类 query callsite | P05、证据 E03 | BAB240/BAB370明确固定值；六类consumer列明地址、action、key和返回消费。 |
| S03 cross-family 与 pressed/triggered/held | P05、证据 E03/E05 | 列明实际family交叉、当前C/S mismatch gate与held_flags缺口。 |
| S04 Pause/retirement、913760 context/cache | P06、证据 E04 | 区分poll自动移除、正key中性暂停、remove/解绑/退出；记录已知cache边与具体未闭合owner关系。 |
| S05 publication/poll/Join/gameplay阶段 | 阶段章节、证据 E01/E05 | 只确认native内部顺序与C/S Authority scope，列明跨边界未知相位。 |
| S06 六条轨迹、Join接入点、contract差异 | DESIGN_ONLY章节、六条手工轨迹、P01–P06表 | 给出9BAC40/6FADAC候选边界、JoinCommand/epoch和contract v2差异；未表示已实现。 |

## P01：注册、原生枚举与 update

**状态：正常路径的关键 native 分支和容器关系已静态确认；自有设备生产注册未实现，实际成员值及 listener 归属未闭合。** 主 active vector 在 Manager+0x08/+0x0C 保存 8-byte device/control pair；Join 经 A20240 从该 vector 枚举，keyed query 也在此按设备 +0x0C key 查找。Manager+0x20 的 auxiliary table 是另一容器，只受 Base slot7/A1FC00 条件路径管理，不是普通 Join 的前置条件。三种对象不可合并：主 active vector、update auxiliary table、PlayerManager 四项 selector。

本轮直接读取的主更新序列为 sub_954CD0：954F7D call A1FC00；954F84 紧接 call 95B310；state=2 分支在 9551BD call 6FA540，后者在 6FADA7 call 9BAC40，返回点为 6FADAC。A1FC00 对当前 active pair 调 A1F280；A1F280 先 C4←C0、清 C0，再调用 device slot +0x78 poll。A1F280 返回 AL=0 时，A1FC00 走 7C/80 manager callbacks、pair erase/release 的正常分支；因此自有 poll 返回 0 会自动把设备从 native active membership 移除。暂停输入必须保持登记，不能把 neutral 与 poll=0 混为一谈。

插入、名称通知和正常 update listener 的历史有效链继续引用 v7。通知运行时 listener 的具体集合仍未知。另有 913760 callback 的 owner/container 关系见 P06；它不能反向证明主 vector、fallback list 或 selector 是同一对象。

## P02：自有类型、名称与 binding

**状态：所需 normal native template 的静态构造链可复用；自有 ABI 实现和实际对象初始化尚未接入。** Base +0x0C/slot3 是名称路径；自有 38 槽表和六槽 control 是已接受设计，须在原生 Base ctor 前固定 vptr/模块，并实现有效静态名称。通用记录每 action 读取两个 primitive，而对象 binding vector 每 action 有四个 primitive；默认 generic 模板在标准 _main 初始化成功分支为 32 项，结构上满足 32-action 读取范围。此结论不证明真实 B 已调用 Init、运行值已可读或每个 primitive 内容适合自有设备。C799F4=4 仅为另一张辅助表 count。

Base query ABI 分层如下：

| device vtable 槽 | native 实现 | 传参/结果 | 消费语义 |
| --- | --- | --- | --- |
| +0x30 / slot12 | A1E8D0 | ECX=device，stack action，AL bool，ret 4 | 当前 pressed；逻辑查询走 C0[action] 或 primitive +0x24。 |
| +0x34 / slot13 | A1E970 | ECX=device，stack action，AL bool，ret 4 | triggered；比较 C0/C4 或 primitive +0x28。 |
| +0x38 / slot14 | A1EA20 | ECX=device，stack action，float 在 x87 ST(0)，ret 4 | scalar=max(C0[action], 四 primitive 的 +0x2C 值)。 |
| +0x78 / slot30 | A1F280 的虚调用点 | ECX=device，AL bool | poll 返回 0 触发 P01 所述 native 自动移除分支。 |

native slot21/+0x54 是独立 raw flush：A1EE10 把 C0[action] 写到 C4[action]。Join 调用 slot21 不会推进 C/S private previous；也不能把 vtable slot21 与十进制 action21 混淆。

## P03：普通 Offline Join 与 selector

**状态：native 普通分支和字段更新链已静态确认；自有 Join command、正常生产接入和运行时可达性仍未实现/观察。** 设计前提要求 Offline roster 为空、key 未占用、selector 有可复用项或空位、Player 数和 allowance 允许创建、普通 mode 分支成立。当前 roster、count、flags 和 selector 实项均为运行值 UNKNOWN。创建并发布 B 是已接受的 DESIGN_ONLY 原生 Join 规划，不在本轮重新证明完整 Player Init。

9BAC40 在普通路径中对 Main active vector 设备按选中 key 查 Join action19（触发 selector）；action20/21 导航 selector；action14 confirm；action15 cancel。action15 成功将 selector index 置 -1，不是 Player unbind/exit。confirm 选定 key 后调用 9B9CD0 创建 Player，再调用 SetControllerId(key,1)。action19 和 action14 可以在同一次 9BAC40 pass 内按顺序执行，因此候选输入允许 select+confirm 同 pass；持续 true 或重复 pass 是否重放事件必须由自有 epoch/pass-completion 决定。

原生 Join 入口的具体插入候选为 9BAC40 entry 的 JoinPassBegin；已知唯一被本轮确认的该 native caller 为 6FA540 的 6FADA7 call，返回后 6FADAC 可作为 JoinPassComplete 边界。这里是 DESIGN_ONLY observer 点，尚无生产 Hook/API，不能把静态 callsite 当已安装接入。

## P04：device key、B assignment 与 ordinal

**状态：key 链和原生 assignment 写点已确认；当前 B identity、count 和退出语义未观察/未接通。** 同一非负 32-bit device key 来自 device+0x0C，selector entry+0、Manager keyed lookup 和 Player+0x1618。SetControllerId 的 this 是 Player，栈参是 key 与 propagate flag；它写 Player+0x1618，正常尾部 ret 8，Join caller不检查 AL。STATIC-INHERITED v6/W09：propagate非零还会在有关联对象/parent条件下更新关联 controller key，不能概括为只写一个字段。9B9CD0 在 append 前以旧主 Player vector count 写 Player+0x161C；仅当创建前 count=1 时，新 B ordinal=1，这个前提不是 binary 强制。key、selector、Player key、ordinal 和 HUD/roster 是不同对象状态。

正 key lookup miss 只会得到 no-device 查询语义；key=-1 走全 active device 或 fallback list 路径，不能作为 B 的中性输入手段。设备保留登记、B 正 key 和自有 query 中性的暂停候选可保留 Player→device identity；但不能将该候选描述为已经实现。

## P05：S2 快照、六类 gameplay consumer 与 Join 差异

**状态：native ABI 和六类 consumer 查询已定位；当前 C/S schema 只实现同 family gameplay 映射，held 与 Join 语义存在明确缺口。** 六类 native consumer 的逐点表、key 和结果使用见[证据 v8 E03](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v8.md#e03-六类-gameplay-consumer-逐点查询)。consumer 调用带 B 的 Player+0x1618 key；按该正 key keyed lookup 到 Manager active vector，再回调自有 device。标量最终由 9562D0 返回 XMM0；pressed/triggered 最终由 helper AL 返回。

Native pressed(+0x30)表示当前非零/按住，triggered(+0x34)表示当前相对 previous 的触发沿；标量(+0x38)给方向幅值。源码的 InputStateDraftV1 同时有 held_flags 与 edge_flags，但 ActionMagnitude 对 action8–11 读取 edge_flags，忽略 held_flags；ActionTriggered 对 action0–7 做 previous=0/current≠0，对 action8–11 使用当前 edge bit。drop consumer 明确连续调用 action11 pressed 并以 0x78 threshold 走持续时间分支，所以只供一帧 edge 的输入无法表达该 held 需求。

真实跨 family 查询：Movement 查询 Drop action11；Shooting 路径查询 Movement actions0–3，也查询 Drop action11；Item consumer 条件下查询 PillCard action10；PillCard consumer 条件下查询 Item action9。Item owner slot0默认取action9；在 Player+0x13C0∈{19,20} 且 sub_6F0040 gate 与 var3D/var3F 均满足时改取10。var3F先取 n298+0x2A3D4==0；fallback list非空时再看 90AC70(Player) 是否返回entry，以及 [entry+0x3D0]==0。Pill/Card默认为10；Player+0x13C0∈{19,20}、sub_753BF0(Player)及关联候选 v6 条件同时成立时改取9；v6使用同一候选列表/90AC70关联项检查，entry的 +0x3D0 可读常量比较为0。固定方向表自身不是 cross-family：BAB240=0,1,2,3，BAB370=4,6,5,7。本轮 SOURCE-DIRECT 读取 C/S SyntheticAdapter::Evaluate/QueryEligibility：consumer 不匹配 ConsumerForAction 的调用路由至 TerminalPassthrough；因此现有 synthetic mapping 不能替代原生的上述查询关系。原 native vtable 可以按 logical action 返回，但自有输入模型/consumer gating 需要扩展。

当前[contract v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)的六个覆写槽为 deleting dtor、name、pressed、triggered、scalar、poll；S2 query 只读、poll 推 current/previous。contract 没有 native Join 的 action14/15/19/20/21、JoinCommand/epoch、pass begin/complete、admission close 或事件 mask。此处为 CONTRACT 要求。[InputStateDraftV1](../../../../../../../cs-t0/native/include/isaac_cs_t0/input_core.hpp#L53)仅定义 gameplay 状态。[active_apply.cpp](../../../../../../../cs-t0/native/src/active_apply.cpp#L19)定义同-family映射，[loop_runtime.cpp](../../../../../../../cs-t0/native/src/loop_runtime.cpp#L1248)显示StandardRoom hook范围；三者为本轮 SOURCE-DIRECT。native slot21 flush 不推进 private previous，不能当成 Join 或 gameplay query 的事件消费。

## P06：暂停、正常退休、cache 与退出

**状态：暂停与 native 自动移除/显式移除的区别已确认；B 的正常退出入口和 callback owner→cache 容器边仍未闭合。** Pause：保留 B 的正 key 和 native active pair，发布中性 gameplay snapshot；自有 +0x78 poll 必须返回非零以保持登记，+0x30/+0x34 返回 neutral、+0x38 返回 0。Poll=0 会自动移除。Retirement：关闭 Join admission/事件，决定 B 是退出还是显式 unbind，进入 RETIRING 中性输出，走 manager normal remove 并确认 key lookup/member miss；按 NEVER 保留已构造对象/control 与模块。取消 action15只清 selector index，不是 B unbind。SetControllerId(B,-1,1) 会写 B+0x1618并可能按 propagate 规则影响关联对象，但不证明 B 的期望游戏语义或 Player exit；若 B 仍有 active consumers，key=-1 可能进入 fallback/all-device查询，不能作为 neutral。移除 pair 本身不写 Player+0x1618；正 key miss 不自动等于已退休。

913760/cache 证据定位了进一步关系，但只闭合到 container 边界：906D60 在 906E70–906E7B 将 913760 与 owner 参数注册到 A1D610；实际调用点为 906E7B。唯一 caller 952410 于 952DBC 把 n298+0x4B3D8 传给 906D60。913760 以 disconnected device 的 +0x0C key 搜索通知 context vector；在另一分支读取 vector object+0x378 内 pair 的 +0x0C key，匹配时由 902F10(pair,0) 清关联。A22190 的构造路径将长度 0x178 数据复制到 object+0x378；90AEB0、90AF30、90AC70 是该字段的正常读/写/按 Player key 比较路径，Item/PillCard 条件也调用 90AC70。

具体缺边：静态材料没有证明 952410 传入的 n298+0x4B3D8 就是 90AC70 所枚举的输入列表，也没有证明它是 Player selector 或 fallback list。需要继续定位该 context 的完整构造/调用关系，才能将 913760 cache 清理闭合到 B、selector 或 Join。实际 listener 集合、runtime pair/key 以及 native notification 是否命中 B 均未观察。

## Publication、poll、Join 与 gameplay 阶段

已证明的 native 次序限于同一 sub_954CD0 更新路径：manager poll A1FC00 → 设备 reselect/update 95B310 → state2 Join dispatch 6FA540→9BAC40；A1FC00 内每个正常成员依次 A1F280 poll。自有契约 v2 规定 coordinator 生命周期转换位于 AUTHORITY_MAIN_LIFECYCLE，即 scheduler 提交 authority tick 后、下一次 manager update 前。C/S 源码另行证明 StandardRoomHook 在 g_standard_room(self) 前调用 Authority(true)，原函数正常返回后 Authority(false)；Authority(true) 发布/锁存该 Authority scope 的 gameplay snapshot。

跨边界相位没有闭合：当前 C/S StandardRoom hook/Authority 调用链没有静态证据显示它发生在 sub_954CD0 的 A1FC00 之前；也没有证明六个 native gameplay consumer 相对 Join pass 的顺序。因此只保留 native manager 内部 poll→Join 的顺序与 C/S scope 内 pre→original→post 的顺序；不得把它们拼成已证明的全局 publication→poll→Join→gameplay 顺序。生产方案需要一个明确的主线程 coordinator boundary，并使 publication epoch、native poll epoch、Join pass epoch 与 Authority tick 的映射可审计。

## DESIGN_ONLY 接口候选及与 contract v2 差异

保持六覆写槽、自有 38 槽 device、六槽 control、S2、NEVER、构造前固定 DLL 表与常驻。建议只加自有 coordinator API/私有快照元数据，不把 Join 挤进 gameplay action schema：JoinCommand 携带 session、generation、epoch、sequence、target_authority_tick 与 action mask（14/15/19/20/21）；在 9BAC40 entry latch 当前命令；同 pass 内 query 只读，select+confirm 可同时为真；在 6FADAC return 做 JoinPassComplete 并关闭该 pass event，防止后续 pass 重放。CloseJoinAdmission 禁止新 Join 命令。若一 authority publication 触发同 epoch 多次 poll，候选方案应使相同 publication epoch 的 current/previous latch 幂等；新 epoch 才推进 previous/current。此项需要修改/细化 contract v2 当前“poll 将旧 current 移至 previous”的语义。它不证明多个 consumer 的游戏副作用 exactly-once。

退休需要区分 Pause 与 RETIRING：已注册 Pause 保留正 key/active membership，poll 非零而 gameplay query neutral；RETIRING 关闭 Join admission 并中性 query，poll 同样不能无意返回 0 而提前触发自动移除，随后由 coordinator 显式 remove、检查 key/member miss，保持对象/control/DLL 常驻。现 contract v2 的 poll return 留给 O-ABI-78 确定，且没有 Pause 状态；建议为 paused-retained 与 retiring-before-remove 写明 poll 非零规则，并把真实 B exit 或 unbind 语义与 manager remove 分开。该候选不替代正常 B-exit 入口的静态缺口，也不改变原版/Isaac Sync 隔离。

相对 contract v2 的缺项：Join command/action mask 与独立 epoch；Join pass begin/complete；admission close/cancel；gameplay snapshot publication epoch 与重复 poll 语义；跨 family mapping；action8–11 held/edge 表达；Pause 与 Retirement poll 返回语义；B 正常退出/解绑之后的 removal 顺序。以上均是 DESIGN_ONLY/需要后续生产实现，不修改当前 contract 文件。

## 六条手工正常轨迹

以下均为手工顺序推导，不是测试、模拟或运行证据；每条从可见条件出发，列明 publication/epoch、native query、selector/key 和下一 pass。

1. **同一 pass select+confirm。** 起态：A 在 roster，B 已按规划以唯一正 key K 登记，selector K 项可用，Offline roster 空。coordinator 在新 Join epoch E 发布 action19+14 mask；下一 native update poll 返回非零，后续 state2 到 9BAC40。它先以 K 查询19开启/复用 selector，再查询14并按选中 index 调 9B9CD0、SetControllerId(K,1)。6FADAC 完成点关闭 E；下一 pass 不再见 E。单 pass 可达属于直接 native CFG；E/mask 和关闭动作仍是 DESIGN_ONLY。
2. **同 pass 重复只读 query。** 起态同上，JoinPassBegin latch epoch E。对同一 key K 的 action19/14 query 任意重复调用仍读取相同 immutable mask，不由 query 改写/消费；native slot21 raw flush只做 C4[action]←C0[action]。本 pass 完成才关闭 E；下一 pass读中性 Join 输出。契约 S2 只读是设计事实，自有 Join event 尚未实现。
3. **同一 Authority snapshot 跨两个 Join pass。** 起态：gameplay publication G 已经由一次 poll latch；其后两个 Join pass 读取期间不产生新的 Authority publication。第一 Join pass读取自己的 E1 action19/14 并完成关闭；第二 pass仍见同一 gameplay G，但没有 E1，须由 E2 或中性 Join 输出决定。pass completion 而非“查询过一次”防止 confirm 在第二 pass 重放。当前 native 路径没有证明这两个 pass 一定发生；这是候选对重复 pass 的静态场景。
4. **同一 publication epoch 多次 poll。** 起态：G1 已发布，current=G0、previous=G−1。若同一 G1 被两次 poll latch，现 contract 的逐次旋转会使第二次成为 previous=G1/current=G1，0→1 triggered edge随即消失。候选增加 publication_epoch 并使同 epoch poll 幂等，直到新的 G2 才推进 previous/current；query仍只读。native 是否每 tick多次 poll和期望的 edge 语义 UNKNOWN，候选需设计确认，不作实现/实测结论。
5. **cancel 与 admission close。** 起态：selector 项 key K 正值、index 已导航，B 仍有正 key K；发布新 E5 cancel action15。9BAC40 在 query真时只把该 selector index 置 −1；不改 B+0x1618、不退出 Player、不移除 Manager pair。6FADAC 关闭 E5，coordinator 关闭 Join admission；后续 gameplay publication按 Neutral恢复。设备若仍需供 B 使用，继续登记且 poll非零。
6. **正常 retirement/remove。** 起态：B 已分配且绑定 K、device在 active vector，可能有 selector/cache 关系。先 CloseJoinAdmission并停止 Join event；选择有独立依据的 B exit，或明确 SetControllerId(B,−1,1) 只作 unbind并考虑关联key传播及key=-1查询副作用；保持自有 query neutral并暂令 poll非零，避免 native auto-remove早于 coordinator 顺序。随后调用 manager normal remove，检查 key K 不再解析到该 device且 active pair已删除；保留 object/control与DLL常驻。selector取消、913760 context cache 清理以及真实 B exit 当前无完整静态闭环，任何检查失败时功能状态均未完成。

## 六条系统功能状态和剩余工作

| 系统链 | 静态已知/可复用 | 明确未完成 |
| --- | --- | --- |
| P01 register/enumerate | 32-entry default、Base ctor/key、Manager active pair、Join 枚举和 poll=0 auto-remove 分支。 | 自有对象的真实 construction/Init/insert/member check 未实现；listener 集合、当前 pair 与 callback owner关系未知。 |
| P02 type/name/binding | v7 默认 generic/keyboard 32-entry 模板及 4*n C0/C4 分配事实有效；六覆写槽满足 ABI 轮廓。 | 自有 38-slot 表、COL、name槽、运行时 Init 成功与 primitive 值未接入/观察。 |
| P03 Offline Join | 19/20/21/14/15 分支、普通 selector、confirm→Player create/setter；同 pass confirm路径可行。 | C/S 无 Join event；roster、mode、selector和Player count运行值未知；B发布仍是 DESIGN_ONLY 前提。 |
| P04 key/B/ordinal | key→selector→active lookup→Player+0x1618 与 count→+0x161C写点明确。 | B真实身份/count未观察；unbound B的正常玩法与exit语义、coordinator调用点未定。 |
| P05 gameplay snapshot | 六 consumer 查询地址、actions、key、返回消费及跨family/held需求明确；scalar/boolean ABI分层确认。 | contract/C/S未表达Join、cross-family和真正held；Authority与native poll/Join相位映射未知，生产query未接通。 |
| P06 normal shutdown | Pause需中性 query + poll非零；poll零会自动remove；显式remove不解绑Player；NEVER保持对象/module。+0x378 reader/writer有定位。 | B正常exit、selector cleanup、913760 owner与Player/fallback/selector列表映射、remove后runtime确认未闭合。 |

准确性结论：本轮直接更正了 action15/21、三层scalar ABI、固定方向 action 表和 Mutex/Manager receiver；六个 gameplay consumer 逐项映射并标出 native cross-family/held。保留每项直接/继承证据边界。正常功能完整性结论：P01–P06 静态状态不等于生产实现；P01真实 construction/registration、P02自有ABI、P03 Join events、P04 B接续、P05扩展输入/poll epoch、P06 B exit/unbind/cache闭合均未完成，运行值亦未观察。

## tx-01-12 唯一整体静态文档审阅

在报告、证据、README及引用的设计、contract和源码读完并编辑完成后，依据工作设计v7 §6人工整体审阅一次。总体结果：**未通过（证据来源标注）**。

| 检查项 | 实际结果 |
| --- | --- |
| ABI、action事实主体、consumer查询与返回消费 | 通过：21/15、三层scalar返回、固定方向表、六类consumer与cross-family/held结论一致。 |
| Mutex/Manager receiver与cache、poll=0、停用边界 | 通过：分别给出本轮直接读与继承身份、auto-remove证据和B-exit/cache缺边。 |
| publication/poll/Join/gameplay顺序、候选与六轨迹 | 通过：native内部顺序与C/S scope分开，跨边界未知和DESIGN_ONLY接入明确。 |
| S01–S06、P01–P06、范围与导航 | 通过：六个分析项和六条系统功能状态分别列明；未宣称生产或运行完整。 |
| 本轮直接/继承证据来源标注 | 未通过：证据E02将9BB05D action19标作v7/W09继承并“bytes复读”，将9BB2B0 action20标作IDA-DIRECT有限复读；本轮实际IDA直接复读记录覆盖9BB2FE action21、9BB393 action14、9BB3C2 action15，19/20只沿用v7/W09。两行因此夸大了本轮直接证据。该问题是来源标注错误；action值仍沿用既有证据。 |

没有修正这两行或重跑审阅。tx-01-12文档准确性总体未通过；正常功能完整性仍按P01–P06表报告为未实现或未闭合。没有运行测试、构建、模拟、harness、probe、smoke、canary、verifier 或链接校验脚本。原v1–v7文件、tx-01-09审阅PASS、tx-01-10错误/不完整结论及本轮原实例异常均保留。
