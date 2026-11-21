> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 正常功能路径静态确认 v7

日期：2026-10-12  
推进：`tx-01-09`，R01–R06 合并为一个目标  
状态：**tx-01-09 唯一人工整体静态文档审阅通过（文档准确性范围）**
证据：[只读静态证据 v7](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v7.md)  
执行设计：[v6](virtual-device-owned-abi-static-confirmation-next-work-design-v6.md)

本报告修正 v6 的 receiver、vptr/key 和 binding 来源，并逐项追踪六类原生 Player consumer。报告只确认本轮限定正常条件下的静态路径与当前 C/S 源码能力；不表示自有虚拟设备已实现，也不声称运行时 Join 成功。

## 条件、边界与资料级别

静态推导采用：分配成功、对象与缓冲有效、操作按正常顺序执行、既定 Offline roster 为空、虚拟 key 唯一，且创建 B 前 Player vector 仅有 A。以上是研究条件，不是本轮运行观察。接受的原生 Join 创建/发布 B 前提仍见[规划假设](../../../02-active-join/native-join-active-user-planning-assumption-v1.md)，状态为 `DESIGN_ONLY`。

输入为只读 IDA MCP 的 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe` 与同名 `.i64`，IDB imagebase `0x00400000`，PE32 i386。本文 VA 均为该 IDB VA；RVA=VA−`0x400000`。本轮没有认证哈希或运行时基址。证据状态分为本轮 IDA 静态读取、继承静态/动态资料、当前源码、手工路径推导、设计候选、UNKNOWN。

不展开 OOM/EH、锁/worker 并发、原子镜像与溢出、ABA、全映像 alias、持久化或异常退休；不重开 T0-03/04、完整 Player Init/manager-this 或 05-06 双端隔离证明。设计背景仍是自有 38 槽 device 表、六槽 control、S2、`NEVER`、DLL 常驻、每 session 至多创建一次及 C/S 与原版/Isaac Sync 隔离。

## Q1–Q6 修订索引

| 项 | v6 问题 | v7 纠正及影响 |
| --- | --- | --- |
| Q1 / R01 | 把 A20DF0 插入末尾 `+0x78` 通知写成 device vptr 回调 | `A20E55: 8B 07` 从 EDI=Manager 读 Manager vptr；`A20E5C: 8B 70 78` 取 Manager slot30；`A20E68: 8B CF` 令 ECX=Manager。它是插入通知 receiver，不是 device。 |
| Q2 / R01 | 把 C7362C 说成 Base vptr 来源 | `A1E07C: 8B 15 2C 36 C7 00` 读 key counter；`A1E084: C7 07 58 27 B8 00` 写 Base vptr B82758；`A1E08E: 89 57 0C` 写对象 key。 |
| Q3 / R02 | 混淆 C799F4 与默认 binding template，停止于静态初值 | 默认 vector 是 C799E8/EC/F0，正常 `_main` 初始化 `sub_4307C0` 创建 32 项 generic 与 32 项 keyboard template；C799F4 是独立 update auxiliary table count，正常初始化为 4。运行时当前值仍 UNKNOWN。 |
| Q4 / R03 | 仅列六个 consumer 地址集合 | 新增逐行 action、owner callsite、函数 ABI、Player key、Manager lookup、查询槽和返回值消费；同时记录交叉 action 与动态 action 边界。 |
| Q5 / R06 | v6 主报告到 out 的相对链接少一级父目录 | 本报告到 out 使用六级父目录；README 使用五级；证据回 docs 使用 `../../docs`。审阅时人工核对位置和文件元数据。 |
| Q6 / R04–R05 | 把 `SetControllerId(-1)` 接近等同 B 中性，Join epoch 仍是未落地文字 | 按 key 正负、fallback list 与 active vector 路径分别推导；给出保留正 key、设备留在 vector、self query 中性的优先候选，并把 Join/pass 接口留作 DESIGN_ONLY。 |

## R01：Manager receiver、Base vptr 与 key

`sub_A20DF0` 的 EDI 是 InputManager。插入 pair 已准备并写入 Main active vector 后，`0xA20E55` 的 `mov eax,[edi]` 读取 Manager 首 dword；`0xA20E5C` 的 `mov esi,[eax+78h]` 选 Manager 表 byte offset `+0x78`；`0xA20E68` 的 `mov ecx,edi` 设置调用 receiver；`0xA20E6A` `call esi` 发出 slot30 通知。前面的 unlock 在 `0xA20E52`，pair 临时对象按值传入。故此通知不能用来推断 device 的 `+0x78` poll 被调用。

| VA / RVA | 原始字节 | 来源和值 |
| --- | --- | --- |
| `A20E55` / `620E55` | `8B 07` | EDI=Manager；EAX=Manager vptr。 |
| `A20E5C` / `620E5C` | `8B 70 78` | ESI=`[Manager vptr+0x78]`，Manager slot30。 |
| `A20E68` / `620E68` | `8B CF` | ECX=Manager。 |
| `A20E6A` / `620E6A` | `FF D6` | call Manager slot30 target。 |
| `A1E07C` / `61E07C` | `8B 15 2C 36 C7 00` | EDX=`[C7362C]`，key counter。 |
| `A1E084` / `61E084` | `C7 07 58 27 B8 00` | `[EDI]=B82758`，Base vptr。 |
| `A1E08E` / `61E08E` | `89 57 0C` | `[EDI+0x0C]=EDX`，当前 device key。 |
| `A1E091`, `A1E09B` / `61E091`, `61E09B` | `8D 42 01`, `A3 2C 36 C7 00` | counter+1 并写回；不展开并发。 |

同一 device 表的 byte offsets：`+0x94=slot37`、`+0x64=slot25`。ManagerBase vtable 从 B827F4 开始，B827F0 是 COL pointer cell（实际 COL BB033C）；派生 Manager vtable 从 B82950 开始，B8294C 是 COL pointer cell（实际 COL BB0424）。这些是本轮相关槽身份的核对，不是完整 RTTI 闭包。

## R02：默认 binding 初始化链

三个全局指针分别是 generic default binding vector begin/end/cap `C799E8/C799EC/C799F0`。`C799F4` 属于另一张 update auxiliary table；不要将 auxiliary count 当成 template 长度。`sub_4307C0` 由 `_main` 在 `0x93114C` 调用，是默认模板的直接初始化者。

| 阶段 | 本轮静态结果 | 正常分支与边界 |
| --- | --- | --- |
| Generic template | `sub_4307C0` 以 `sub_A24420(temp,0x20,...)` 建 32 个 16-byte 项，4 个 primitive dword/项，初值 `0xFFFFFFFF`。随后循环读取 C33950/C33954 的 41 个 primitive-action pair，重映射 primitive 17→7、18→5，只写每 action 首 primitive；最终 memmove 到 C799E8，更新 C799EC。 | 只在正常 `_main` 初始化、有效源表和成功分配条件下成立；若 `byte_C78AAD!=0` 则走 `SetEngineParameters must be called before ...` 分支。当前运行走哪个分支及当前数据值 UNKNOWN。 |
| Keyboard template | 同一初始化器以 `sub_A24420(...,0x20,...)` 建另一张 32 项 template，循环读 C33B10/C33B14 33 个 pair，填充后复制到 C799F8/C799FC/C79A00。`sub_835820` 对 Keyboard name 选该表，其他通用设备选 C799E8。 | 两表分开，不把其中一表内容外推为另一表运行值。 |
| Auxiliary table | `sub_4307C0` 在 `0x430CD8` 将 count 4 写到 C799F4；写点前 `n4=4`。A1FC00/A200A0/A20620/A213B0/A21980 使用其辅助表。 | 4 不是 binding vector count；runtime 当前值 UNKNOWN。 |
| Base slot5 copy | `A1E430` 读取 C799E8/C799EC，将 template 复制到对象 `+0x10/+0x14/+0x18`；对象元素数 `(end-begin)>>4`，元素 stride16。 | `A1E430` 是 Base vtable `+0x14`/slot5；原字节引用见证据 v7。 |
| Init | `A1E210` 调对象 vptr+0x14/slot5，按对象 n 为 C0 与 C4 各分配 `4*n` 并清零；此路径被 A22190、A6C3E0 调用。 | 在正常 Main 初始化规则下 n=32；运行期对象当前长度、是否经历其他合法修改 UNKNOWN。 |
| Record consumer | `835C90` 固定遍历 32 actions；每 record action stride8 保存两个 primitive，而 device binding action stride16 有四 primitive。 | 正常 generic registration 需要 32 项；已知初始化器能供应 32 项。record 复制内容仅取前两项，不等于完整四 primitive row。 |

因此，静态初始化规则不再是“binding 长度来源完全 UNKNOWN”：main 的普通初始化分支把固定 32 项模板写入默认 vector。此规则不证明目标运行时当前指针值、每个 primitive 的最终内容、分支已执行或自有设备已成功 Init。`C799F4=4` 仅适用于 auxiliary table。

## R03：六类原生 gameplay consumer

地址按 IDB base `0x400000` 展示。owner callsite→target 来自 T0-04 handoff §7，本轮 IDA 确认各 target 的查询链。所有核心查询用 Player `+0x1618` 的 controller key。`A20FB0` 是 Manager slot29（vtable byte offset `+0x74`）；正 key 下经 Manager slot10 `A20390` 在 Main active vector 按 device `+0x0C` 查找，找到才调用 query callback。key=-1 与辅助 list 分支见 R04。

| Consumer / RVA / VA | Action、owner ABI、key 到设备路径 | Device query、返回如何消费 | 对自有 snapshot 的边界 |
| --- | --- | --- | --- |
| Movement `385BC2→379B40`; `785BC2→779B40`; `thiscall(Player*)` | 主要标量方向 action0–3，经 `779360→9562D0`；key=`Player+0x1618`；slot29→slot10/keyed active pair。 | callback `A20AB0` 调 device `vptr+0x38(action)`，float 在 ST(0)，AL 是“值非零”；consumer 以四向 scalar 组合移动。 | action0–3 映射 C/S `move_x/y` 四方向。函数另有 action11 triggered gate 与 `xmmword_BAB240` 派生的四个动态 action query；这些未归入 Movement 当前 0–3 schema，且 cross-family 会拒绝当前 snapshot override。动态 action 值 UNKNOWN。 |
| Bomb `385BC9→380F40`; `785BC9→780F40`; `thiscall(Player*)` | action8；key=`Player+0x1618`；同一正 key lookup。 | `9561E0`→callback `A209E0`→device `vptr+0x34(action)`；AL bool 驱动 bomb branch。 | 对应 C/S edge flag bit0/action8。完整 action helper 有 early override bypass；当前 C/S snapshot 不证明自有 vtable 接入。 |
| Item slot `385BD2→391A40`; `785BD2→791A40`; `thiscall(Player*,int slot)` | 普通 `n9=9`，条件 `(v178&&v10)||n2_5` 时改 action10；另有 action11 gate。key=`Player+0x1618`，owner 传 slot（callsite push 0）。 | action9/10 经 `956110` pressed `+0x30` 和/或 `9561E0` triggered `+0x34`；callback `A20940`/`A209E0` 返回 AL，分别用于使用物品判定。 | action9 对应 item edge bit1；条件 action10 映射 pill/card family，当前 Item scope 与 action10 分类不符，C/S adapter terminal-passthrough。action11 gate 也 cross-family。原生条件/Player mode 决定 action，不能将整函数概括为仅 action9。 |
| Pill/card `385BD9→3A3D30`; `785BD9→7A3D30`; `thiscall(Player*)` | 默认 action10，`v4&&v6` 条件改 action9；另有 action11 gate。key=`Player+0x1618`。 | action10/9 经 `9561E0` triggered `+0x34`、callback `A209E0`，AL 驱动 pill/card action。 | action10 对应 edge bit2；条件 action9 被 C/S adapter 归类为 Item，与 PillCard scope 不符；action11 同样跨 family。 |
| Shooting `385BE0→37B020`; `785BE0→77B020`; `thiscall(Player*)` | 主要 action4–7（方向 aim scalar/pressed 查询，按分支）；另有 alternate movement action0–3、action11 及动态 action list query。key=`Player+0x1618`。 | 逻辑查询经 `956110`/+0x30（bool AL）和 Movement scalar 路径 `9562D0`；结果用于 aim/shoot intent。动态 triggered 查询仍有 `9561E0`/+0x34。 | action4–7 映射 `aim_x/y`。同一 Shooting scope 内 action0–3、11 不属当前 Shooting family；动态索引集合/边界未完全解码，不能宣称 snapshot coverage 完整。 |
| Drop `385BE7→3A42B0`; `785BE7→7A42B0`; `thiscall(Player*)` | action11；key=`Player+0x1618`；同一正 key lookup。 | 两处均可见：`9561E0` triggered `+0x34` 及 `956110` pressed `+0x30`；结果 AL 进入 drop / hold-timer 路径。 | 对应 edge bit3/action11。当前 C/S 输入只有一个 edge flag，不能单凭 schema 推出 native pressed/triggered 的持续时间语义完全相同。 |

`956110` 的 raw 起始序列在 `0x956115` 读 action、`0x95615A` 读 key、`0x956177` 载 device vtable+`0x30`、`0x95617A` 间接调用。`9561E0` 对应 `+0x34`，`9562D0` 对应 `+0x38`；callback `A20940/A209E0/A20AB0` 接受 device receiver 与 action。函数先可能进入 `sub_864C30/864D80` override 分支；实际运行触发与否 UNKNOWN。正 key 无 pair 时 Manager callback 不运行并返回 false/zero；不会因此自动退到 -1 的枚举语义。

当前 C/S 实现参考 [`input_core.hpp`](../../../../../../../../cs-t0/native/include/isaac_cs_t0/input_core.hpp) 与 [`active_apply.cpp`](../../../../../../../../cs-t0/native/src/active_apply.cpp)：`InputStateDraftV1` 有四个 Q15 signed axes（move x/y、aim x/y）、shoot-held flag 和四个 edge flags；`ConsumerForAction` 将 0–3 Movement、4–7 Shooting、8 Bomb、9 Item、10 PillCard、11 Drop 分类。轴 action 从轴分量取方向 magnitude，0–7 edge 是 previous=0/current≠0；8–11 使用当前 edge bit。`Evaluate` 的自然 consumer/action family 不匹配会终止 snapshot override并报告 unsafe，而不是跨 family 提供 snapshot。active runtime 当前挂六类 target/callsite，但没有自有 device vtable 实现；所以上表描述 native 路径与源码 adapter 当前覆盖，不能写成“自有设备已对六类完整生效”。

## R04：解绑、移除与保持 B 中性的结论

Player `SetControllerId` 为 `sub_7A6450(this=Player,key,propagate)`，正常 return `retn 8`；`0x7A6598: 89 9F 18 16 00 00` 写 `Player+0x1618`。propagate 非零时正常更新 `+0x1D78/+0x1E6C/+0x1E68` 关联 child；并在 parent 条件满足时沿 Player vector 查 `[child+0x3BC]==this` 继续传播。该函数不是成功 bool，不移除 manager device，不清除 selector。没有看到该 setter 写 ordinal `+0x161C`。

| 正常状态 | 已读解析路径 | B 的输入结果与限制 |
| --- | --- | --- |
| 保持 B 正 key，设备仍在 Main active vector | `A20FB0(key)`→`A20390` 查 `device+0x0C==key`→仅该 device callback。 | 让自有 `+0x30/+0x34` 全部 false、`+0x38` 为 0，可在保留唯一 B key 与设备注册的情况下使 B 中性；这依赖自有查询实际按契约返回 neutral。A 不会因该正 key lookup 被查询。 |
| 保留正 key，但 device 已正常移除 | 正 key lookup miss，`A20FB0` 不调用 query callback，不切换到 key=-1 枚举。 | 该 helper 查询为无设备的 false/0，属于失去目标设备，不等于正常 unbind/exit；Player 保存的 key 仍可能是旧 key。本轮没有证明所有 helper 外的缓存均同步清除。 |
| `SetControllerId(B,-1,1)` | 三个 action helper 在 Manager 路径前检查辅助/fallback list。list 非空且分支允许时直接遍历该 list；list 空时由 Manager slot29 调 `A20FB0(-1)`，枚举 Main active vector 全设备。 | 不能判定 B 必然串到 A，也不能保证 B 中性：list 可能包含 A；空 list 分支的全设备枚举也可能访问 A。该行为依赖运行时 list membership 与各设备 query。setter -1 本身不是中性保证。 |

`sub_A20EB0` 正常 remove 在 Manager receiver 上，先调 Manager slots `+0x7C/+0x80`，再按 Main vector 中 pair 的首字段 device pointer 找并 erase、release pair/control 与通知；直接体没有 Player key `+0x1618` 写点。`sub_A213B0` 执行 auxiliary-table/device flag/listener 清理；直接体也没有 Player key 写点。`sub_913760` 会检查断连 device key，移除对应 NetInputDevice vector 项，或对传入容器中 `object+0x378` pair 做匹配和清理。注册点在 `sub_906D60`，通过 `sub_A1D610(&dword_C57B4C,sub_913760,owner)` 注册；该 owner/容器是否关联 Player 输入 list、selector 或其他缓存仍 UNKNOWN，不能据此宣称不存在间接影响。

Selector 是另一个状态：普通 action15 在 selector key/查询仍有效时取消并把 selector index 置 -1；selector 取消不等于 Player unbind 或退出。优先的正常停用候选是保留 B 的正 key 和 active pair、停止新 Join admission、通过自有查询使 B 的 gameplay query neutral。若必须移除设备，正 key miss 会让查询无设备；随后 Player 自然退出/unbind 的入口与 callback 造成的缓存关系仍需明确。不要先把 B key 改 -1 再假定只会命中 B。

## R05：Join event、poll 和只读 query 的 DESIGN_ONLY 候选

原生普通 Join 直接枚举 Main active vector；slot7/update auxiliary table 不是普通 Join 的前置条件。继承并复核的 native 行为：action19 开启/复用普通 selector，action20/21 导航，action14 confirm，action15 cancel；普通 confirm 创建 Player 并把 device key 传给 `SetControllerId(key,1)`。原生 slot21 `+0x54` 是 `C4←C0` flush，不推进自有 current/previous。源证据、有限 CFG 字节、selector key 与 ABI 见[证据 v7 §5](../../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v7.md#r05-join-poll-and-interface-candidate)。这些只是原生静态路径与已接受创建规划假设的组合，不是新运行事实。

当前 C/S authority 在 `StandardRoomHook` 调原生 standard-room 前调用 `Authority(true)`，返回后 `Authority(false)`；active runtime pre 阶段 resolve/publish command 并 `BeginAuthority`，post 在原始调用正常返回时 `EndAuthority(true)`。这给出 C/S Authority snapshot 作用范围，却没有证明它与 native InputManager update / `9BAC40` Join pass 同一 phase。原生 Main update 路径在 `0x954F7D` 调 `A1FC00`，紧接 `0x954F84` 调 `95B310`；其 state2 经 `6FA540→9BAC40`。当前源码没有 Join command schema 或 `JoinPassComplete` observer。

建议的最小 `DESIGN_ONLY` 协议（不改 contract）：

| 接口/状态 | 候选语义 |
| --- | --- |
| `JoinCommand` | 独立于 `InputStateDraftV1`；含 session、generation/epoch、sequence、目标 Authority tick、原生动作 `19/14/15/20/21`，以及需要时的 selector intent。只接受已发布且目标 tick 有效的命令。 |
| Join pass 进入 | 在已证实的 `9BAC40` native pass 边界读取单个有效 Join epoch；当前没有该 observer，需要由 coordinator 设计接入。不要拿渲染帧或墙钟替代 Authority tick。 |
| `+0x78` poll | 只推进 gameplay snapshot current/previous；poll 才是快照锁存点。Join event 不被 gameplay poll/query 次数消费。原生 native poll 与 C/S Authority 顺序仍待接入设计。 |
| `+0x30/+0x34/+0x38` | 只读同一 immutable gameplay snapshot。`+0x34` 对 Join 动作读取 Join pass 当前 event；同一 pass 内重复 read 结果一致，不由 query 自身清 edge。 |
| `JoinPassComplete(epoch)` | 在 `9BAC40` 返回后由 coordinator 明确确认；清该 pass event，禁止下一 pass 重播 action14/19。当前无此完成通知，需增加边界观察。 |
| `CancelJoin` / `CloseJoinAdmission` | 先在 selector 与设备 key 可解析时用独立 action15 event 取消 selector；完成该 pass 后关闭 Join admission 并令 14/15/19/20/21 中性。不要把 selector cancellation 当 Player unbind。 |
| gameplay 恢复 | Join 完成/关闭后，在下一有效 Authority tick 发布 B 的 gameplay state；保留正 key/self device，并让无输入查询返回 neutral。 |

手工正常轨迹：tick T 提交 epoch E 的 action19；native pass 开始后 B 的 +0x34 对 action19 在本 pass 每次都读 true；`9BAC40` 建立 selector，pass return 后确认 E 完成并清 event。tick T+1 提交 action14 epoch E+1；下一个 pass 对该 selector key 读到 confirm，原生创建 B 并 setter 绑定正 key；pass 完成清 event。若取消，则下一个有效 pass 只发 action15，native selector index 复位，随后完成该 pass，再关闭 Join admission；在 selector/key lookup 有效时完成取消。此后保持 B 正 key和自有 device，gameplay snapshot 切到 neutral，之后才接受新 gameplay state。

`9BAC40` 的 action14 query 位于 selector slots 循环里，key 对应 slot 的触发后新建 Player 并 `SetControllerId`；action15 是该循环的另一 query。普通 action19 有多个分支 query 点。唯一 key 前提能限制 B 的 selector slot，但动态设备/其他 slot 状态和每种 mode 的完整 query 次数仍有边界；接口仍应以 pass event 稳定、pass 完成清除。确切 Native pass boundary instrumentation 需要另行设计/实现。当前 contract v2 定义六槽和 gameplay poll/query 只读，没有 `JoinCommand`、epoch、pass completion、Join admission 或 cancel API；本轮不自动改契约，也不要求覆写 slot7。现有六槽仍可表达查询与 poll，但必须补充外部 Join 输入来源和完成边界设计。

## 剩余待办与状态区分

| 类别 | 真实状态 |
| --- | --- |
| 可复读的静态事实 | Q1/Q2 receiver 与构造字段来源；正常 `_main` binding template 固定构造长度 32、对象 n 与 Init 使用链；六个主要 consumer 的 key/query 与返回消费；正 key miss、key=-1 fallback/all-active 分支；remove 直接体 key 不写。 |
| 静态尚未展开 | Movement/shooting 的动态 action list 最终数值与逐项语义；`sub_913760` callback owner 对具体 Player/input-list/selector 缓存的动态关联；全部 mode 下 native Join 查询次数及 Native pass observer 的最佳接入点。 |
| 当前运行值 UNKNOWN | main 初始化分支是否已执行、当前模板数据、device binding/key、active vector、fallback list、roster、Player count、selector、thread 和 native poll/Join phase。 |
| 待采纳的设计 | `JoinCommand`/epoch、native pass begin/complete 边界、Join admission/cancel、gameplay neutral/resume 顺序；是否让 action14 分独立 tick。 |
| 尚未实现 | 自有 38 槽 device、六槽 control、创建与注册、Join 命令、snapshot query、pass completion、B 接续及中性停用。 |

该状态不将“UNKNOWN 已记录”当成功能完整，也不把尚未实现的源码接入伪装为静态二进制错误。端到端自有设备 Join 与 C/S gameplay 仍未完成。

## tx-01-09 唯一整体静态文档审阅

全部分析与编辑完成后，按执行设计 v6 §6 人工通读 v7 报告、证据、v6 执行设计、契约 v2 相关槽语义、当前 C/S 输入/adapter 源码及 README。唯一一次审阅：**通过（文档准确性与本轮范围内的静态结论组织）**。没有运行程序、游戏、构建、测试、verifier 或链接校验脚本；没有进行第二次审阅。

| 检查项 | 实际结果 |
| --- | --- |
| Q1/Q2 receiver 与字段纠正 | 通过：Manager slot30 通知 receiver、Base vptr、key counter/key store 在主报告与证据一致。 |
| Binding 来源 | 通过：模板 vector、对象 n、C799F4 auxiliary count分开；`sub_4307C0` 正常初始化分支与 32 项 template 来源有直接证据。当前 runtime values 仍 UNKNOWN。 |
| 六类 consumer | 通过（本轮主要路径）：六行均有 RVA/VA owner、ABI、action、Player key、Manager/device query、返回消费和当前 snapshot 对应；dynamic side actions 与 C/S family mismatch 明确保留。不是所有函数内动态 action 都已闭合。 |
| 正常停用 | 通过（已读路径）：正 key 保留设备、正 key 移除、`SetControllerId(-1)` 三态区分；未将全设备查询泛化成 B 必然串线，也未把 setter/remove 判为中性保证。listener owner/context与外围缓存关系仍 UNKNOWN。 |
| Join/poll/query | 通过（候选边界）：pass event、query只读、poll current/previous 与 Authority 阶段分开；JoinCommand/pass completion 未采纳、未实现，原生 pass observer 仍是具体待接入项。 |
| 文档、链接、范围与状态 | 通过：以文件位置和元数据人工核对相对路径；VA/RVA相互一致；旧失败记录保留；没有 gate/hash 认证或生产完工声明。 |

事实修订准确性：本轮列出的新静态事实与直接字节、callee 和 receiver 一致。六条主要正常 gameplay query 链已逐项说明；额外动态 action list 尚未完全解码，因此不宣称所有 consumer 内 action 均覆盖。仍需采纳/实现 JoinCommand、native Join pass begin/complete observer、B 中性 snapshot 与正式 virtual device 接入。仍 UNKNOWN 的具体项包括初始化分支是否在当前运行执行、当前 template 内容/key/vector/list/selector/roster/thread/poll phase，以及 listener owner 与 Player 缓存的关系。审阅通过表示文档和静态结论在本轮范围内一致，不表示端到端功能完成。

