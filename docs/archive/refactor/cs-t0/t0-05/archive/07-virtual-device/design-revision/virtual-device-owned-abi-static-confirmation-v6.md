> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# Virtual Device 自有 ABI 正常功能路径静态确认 v6

日期：2026-10-12  
工作项：tx-01-06  
当前结论：已完成限定范围的只读静态分析与文档交付；六条正常功能链中有明确缺口，不能宣称实现已完整。tx-01-06 唯一整体文档审阅未通过，原因与未完成项见末节；未修复或重跑。

## 结论摘要

本轮把原生注册、active vector 枚举、普通 Offline Join、B 的 key/ordinal 关联、私有快照查询以及正常停用分别核对。最重要的功能结论是：原生 Join 直接枚举 InputManager 主 active vector，不依赖 update 辅助表；Base slot7 只决定是否将设备放入辅助表并带起 bit0 等活跃标记。因而 slot7 不是 Join 的必需覆写。不过，这不表示六槽 S2 方案已经能驱动 Join：当前 S2 canonical 状态只有 gameplay action 0–11，而 adapter 对 action ≥12 返回原生路径；Join 使用 action 19，confirm 使用 action 14，必须补独立的 Join 命令状态及只读查询映射。

正常注册的名称兼容需要覆写 Base vtable +0x0C（slot 3），因为新增设备进入非 NetInputDevice 的通用记录路径时，通知消费者会调用该名称槽。通用记录消费者按 32 个 action 复制设备 binding，每个 action 从设备四 primitive 槽中取前两个值；因此正常兼容要求 native Init 后的 binding 长度至少为 32。当前运行时模板长度和内容未知，不能把 IDB 中 C799F4 的静态初值当运行值。+0x64 是 slot 25，+0x94 是 slot 37；本轮继续区分 vtable 槽偏移、slot 号和 COL 指针 cell。

普通 Join 的最小原生序列为：设备已在主 active vector 且具有非负唯一 key → Join action 19 触发 selector 创建或复用，并设 type=0、index=1、scale=1、autoconfirm=0 → confirm action 14 触发原生 Player 创建 → Player 主 vector append 前记录 ordinal → SetControllerId(key, propagate=1) 写入 Player+0x1618。创建前恰有 A 一个 Player 时，新 B 的 +0x161C ordinal 为 1；native 不强制这一条件。历史 W09 的同一 pass 可先选择再 confirm 由本轮有限复读保留。重复 action 19 会复用 selector，但持续的 action 14 可再次创建 Player，原生 Join 没有 generation/ordinal 去重。

私有 gameplay 快照可经自有 +0x30/+0x34/+0x38 查询槽提供给 T0-04 已定位的 gameplay consumers；当前 C/S 代码的 action 映射覆盖 0–11。原生 slot21（+0x54）只把 native C0[action] 写到 C4[action]，不会清除或推进私有 previous。query 只读与 Join 一次性边沿之间仍缺接口设计：推荐保持 query 只读，在 coordinator/scheduler 中提供单次 Join epoch 命令并由 slot +0x34 映射 action 19/14/15/20/21；这是 DESIGN_ONLY，当前 canonical packet 与 adapter 尚无这些事件。

正常停用时，先关闭 Join admission 并让私有 query 中性，再将仍保留的 B 显式解绑（或按游戏正常路径退出），然后走原生 remove、确认主 vector 不再按 key 找到设备。remove 会执行 manager +0x7C/+0x80 清理路径并从主 active vector 移除，但静态链没有写入 Player+0x1618 的动作，故“设备已移除”不能推出“B 已解绑”。按已采纳 NEVER 与模块常驻设计，base constructed 的对象与 control block 不释放，模块保持常驻。B 退出的具体正常入口、解绑后玩家仍留场的交互语义均为 UNKNOWN。

各链的证据、原始指令、继承与本轮复核边界见[只读证据 v6](../../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v6.md)。设计范围依据为[执行设计 v5](virtual-device-owned-abi-static-confirmation-next-work-design-v5.md)，行为契约仍为[协调器契约 v2](../../../07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)。

## 范围与证据标签

- IDA-STATIC：本轮从只读 IDA MCP 对目标 IDB 读取的指令、字节、整数、交叉引用或反编译控制流。
- STATIC-INHERITED：来自 v2/W09、v4/v5、T0-04 handoff 的已存在静态/动态研究结论；本轮明确标出是否复读，不伪装成本轮的新动态观察。
- CODE/DESIGN：C/S 源码或已采纳契约/设计中的语义，不是游戏二进制事实。
- STATIC-DERIVED：由有地址的直接指令有限推导出的正常分支结论。
- UNKNOWN：本轮材料不能决定的运行时值、间接回调目标或未定义接口。

目标为 F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe，IDB 为同目录同名 .i64，IDA imagebase 为 0x400000；本轮以 IDA VA 记地址，只有在列明时才给 RVA。没有读取运行进程或内存，没有附加调试器、下断点、patch 或保存 IDB。没有运行测试、模拟、构建、verifier 或游戏。

正常功能范围限定为注册→原生枚举/选择→普通 Offline Join→key/B assignment→自有快照 gameplay 消费→正常停用。OOM、EH、锁/worker 并发、原子计数/溢出、全映像 alias/points-to、持久化及异常恢复不展开，也不改判其历史状态。OriginalOnline NetInputDevice 与 Online Manager 仍隔离。

## P01：注册、原生枚举与辅助表

本轮确认的三个容器不同，不能互相替代：

| 容器 | 原生布局/producer | 已知 consumer | 对普通 Join 的关系 |
| --- | --- | --- | --- |
| InputManager 主 active vector | Manager+0x08/+0x0C，8-byte pair；A20DF0 按值复制设备/control pair 追加 | A20240 枚举；A20390 keyed 查找；A20FB0 全设备或按 key 查询；9BAC40 Join 从这里取设备 | 必需。设备进入该 vector 后，Join 可以直接按 pair 查询；不需辅助表成员 |
| InputManager update 辅助表 | Manager+0x20 指向按 C799F4 计数的 8-byte pair 槽；A1FC00 在 slot7 返回非零、长度大于零且有空槽等条件下写入并设置 device+8 bit0 | A200A0 计空槽；A20620 按 device key 查；A213B0 移除匹配项、回调并清 bit0 | 不是 Join 的枚举源。它影响设备活跃标识及相关 update/UI/consumer 路径 |
| PlayerManager selector | 4 项，每项 0x14 字节：key+0、type+4、index+8、scale+0xC、autoconfirm+0x10；9BAAA0 复用同 key 或找第一空项 | 9BAC40 的角色选择、confirm、cancel 路径 | 仅 Join 的选择 UI 状态；不是设备成员表或 update 辅助表 |

IDA-STATIC：954CD0 的 0x954F7D 调 A1FC00、0x954F84 调 95B310；同帧后续分支 0x9551BD 调 6FA540，6FA540 的 0x6FADA7 调 9BAC40。A20DF0 在持有 manager+0x14 同步对象时将按值 pair 加入主 vector，正常路径在 0xA20E52 解锁；随后从设备对象 vtable +0x78 通知。通知实现 A211D0 遍历 manager+0x2C 通知列表，调用已注册的 listener；具体运行时 listener 集合及所有回调 target 不作全映像闭合。

IDA-STATIC：A1F280 先复制 C0→C4，清零 C0 与若干状态字段，再调用 device vtable +0x78。该 poll 返回 AL=0 时 common update 早退；非零才继续调 +0x80/+0x7C。A1FC00 common 非零后，若 device+8 bit0 尚未置位，会调用 device+0x1C（Base slot7 / A1E4B0）。只有 slot7 非零且 C799F4 长度非零并找到空辅助槽时，A1FC00 才把 pair 放入 Manager+0x20、设置 device+8 bit0，并追加到本轮临时 pair 列表；解锁后才运行 Manager+0x3C 通知列表。callback 列表的运行时对象和目标仍 UNKNOWN。

IDA-STATIC：Base slot7 A1E4B0 根据 device+8 bit1、device+0x20/+0x24 与 +0x30/+0x34 状态、A1EED0 返回的轴值、C0 current float 与内部 primitive 记录判断活动；A1EED0 调用 Base slot31（+0x7C）并做 dead-zone 映射。该路径不调用逻辑 action 查询 +0x30/+0x34/+0x38，也不读取 S2 私有扩展区。它不能把 S2 快照接入 native action consumer。

IDA-STATIC 与 STATIC-INHERITED 交叉确认：9BAC40 以 A20240 取主 active vector；A20FB0 的 key=-1 分支也遍历主 vector，keyed 路径经 A20390 在主 vector 按 device+0x0C 找对象。因此普通 Join 直接依赖主 vector 和 +0x34 query；没有静态路径要求先加入 Manager+0x20。slot7 是否返回非零影响 auxiliary membership/bit0，不是路线 A Join 的前置覆写。

## P02：自有类型、名称与 binding

本轮 IDA 读取的 Base device vtable 起点为 0xB82758。+0x0C 是 slot 3，当前 Base 项指向纯虚桩 0xAF05D3；+0x30/+0x34/+0x38 分别是 slot 12/13/14，指向 A1E8D0/A1E970/A1EA20；+0x54 是 slot 21/A1EE10；+0x64 是 slot 25/A1EAD0；+0x78 是 slot 30/纯虚桩；+0x7C 是 slot 31/A1F720；+0x80 是 slot 32/709C30；+0x94 是 slot 37/A1F930。槽号=字节偏移/4，+0x94 是 slot 37，不是 slot 0x94。

IDA-STATIC：95B290 收到添加通知时，以 pair 中 device+0x0C key 调 835C90。835C90 先用 Manager keyed lookup 取 device，并尝试 InputDeviceBase→NetInputDevice RTTI cast。对自有 InputDeviceBase 派生设备，该 cast 预期为 null，随后进入通用名称记录路径；该路径通过 vtable slot 3 获取名称。新增自有类型因此必须实现 +0x0C 的有效静态名称。95B310 的相邻重选路径尝试 Gamepad RTTI；非 Gamepad 自有 Base 派生设备不会进入 Gamepad 特有的 +0x900/action14 更新分支。范围内没有依据表明它应冒充 Gamepad、Keyboard 或 NetInputDevice。

IDA-STATIC：835C90 的通用记录每项 0x104 字节，名称后有 0x100 字节 binding 区；新记录先以 0xFF 初始化 binding 区，然后循环 32 个 action，每 action 读设备 binding vector 的两个 4-byte primitive，并写到 record 中对应 8-byte/action 位置。设备 vector 每 action 16-byte、四 primitive；record 每 action 8-byte、只保留前两个 primitive。action index 与 primitive ID 不同。若 binding vector count 小于 32，代码会输出 ActionId out of range 但仍执行写入，因此正常功能前提是 Init 完成后长度至少 32。

IDA-STATIC：A1E050 ctor 初始化 Base vptr、key+0x0C、binding vector 三指针+0x10/+0x14/+0x18、C0/C4 等；A1E210 Init 用 vector 长度 n 分配两个 4*n 数组；A1E430 将 C799E8/C799EC 的默认模板复制到对象 vector。当前 IDB 中 C799F4 静态读取值为 0；xref 仅有 sub_4307C0 writer、A1FA20、A1FC00、A200A0、A20620、A213B0、A21980。静态初值不表示初始化后运行值，模板内容和 n≥32 是否成立 UNKNOWN。基础对象区按契约只能由原生 ctor/Init 写；若默认 Init 不能提供至少32项，当前契约下尚无已核实的正常初始化接口可补足。

STATIC-INHERITED 更正：sub_A1E760 的两处 receiver 是栈上临时输出记录，不是设备对象；本轮不沿用 v4 将它列为 binding writer 的旧归属。+0x64 为 slot25、+0x94 为 slot37；ManagerBase/Manager 的 vtable 起点与 COL pointer cell/object 也按证据 v6 §0.3 区分。它们不改变六个自有覆写槽的当前 contract，但 +0x0C 名称槽是注册兼容的明确需求。

## P03：普通 Offline Join、confirm 与 selector

普通链在本轮直接读取和有限复读后为：

1. Frame manager update：954CD0+0x954F7D → A1FC00；随后 0x954F84 → 95B310；state2 Join 的 0x9551BD → 6FA540 → 0x6FADA7 → 9BAC40。
2. 9BAC40 取主 active vector，按 pair 对每个 device 以 manager slot29/+0x74（A20FB0）检查 Join action 19；A209E0 的 receiver 是 device，虚调用 +0x34，栈上传 action，调用者消费 AL。
3. action 19 成功后用 9BAAA0(key) 取得或复用四项 selector 中同 key 的项；key 必须非负。普通路线设 type=0、index=1、scale=1、autoconfirm=0，并通过 device +0x54 调 action 19 flush native raw buffer。
4. 导航 action 20/21 通过同一 triggered query 更新 selector index；confirm 14 通过 +0x34；cancel 15 通过 +0x34 将 index 复位到 -1。selector 按普通分支完成后进入 9B9CD0 建 Player，再以 7A6450(Player,key,1) assignment。

普通路线的静态条件承接并复核自 W09：Offline roster 为空；待分配 key 不已被主/次 Player 集合占用；selector 同 key 可复用或至少有空项；Player count <64 且局部 allowance 满足；sub_9BA980 的特殊模式分支为 false；gameRoot+0x26584 与 +0x26630 满足普通分支要求的零条件；slot 上的普通角色/selector 状态可达。确切当前 roster、Player count、active selector 及这些字段运行值均 UNKNOWN。+0x26630>0 时 action19 只有声音/效果而不执行正常 spawn。特殊 9BD3B0、706940、possessor 的 action19+8+10 不作为本次最小路线。

有限复读证据确认 action19 后同一 9BAC40 调用仍可能继续到 selector confirm 检查，故原生逻辑不要求两个独立自然帧。该事实继承 W09 并由本轮相关 CFG/字节锚点复核；不是实机体验。若 command 在同一 manager update 可令 action19 与 action14 都触发，普通选择后可直接创建 B。action19 对同 key selector 是幂等复用；持续为真的 action14 可重复创建 Player。原生代码没有按 generation/ordinal 去重，05-06 去重语义不在本轮重做。

slot21 flush 的真实方向是 C4[action]←C0[action]。9BB0B8 的 vtable +0x54 调用点传 19；该函数 A1EE10 返回 old C0 并把它写入 C4。它只触及 Base native raw buffer，不改变 S2 私有 current/previous。查询 +0x34 与 flush +0x54 是不同接口，不能把 flush 当事件消费或私有 snapshot previous 更新。

## P04：device key 到 B 与 ordinal

device key 是 Base 对象+0x0C 的 32-bit 值；主 vector pair 保留 device/control 两指针；selector item+0 是同一 key；A20FB0/A20390 在 active vector 中按 device+0x0C 查找；普通创建将同 key 传给 SetControllerId。7A6450 原始指令在 0x7A6598 写 Player+0x1618。它接收 Player this、stack key 和 stack propagate flag，尾部为 retn 8。调用者忽略 AL；反编译声明为 void 与历史“AL 未归一化”的调用约定风险并不矛盾，不能以 AL 判成功。

9B9CD0 在 append 主 Player vector 前将旧 vector count 写入新 Player+0x161C。由此，仅当创建前主 Player count 恰为1（原有 A 一个）时，新 B ordinal=1；这是部署/调用条件，不是 native 强制。设备成员、selector、B controllerId、ordinal 和 HUD 是不同字段/对象且发布顺序不同，不能因为 key 数值相同就视为一个状态。

从 gameplay consumers 经 Player key→Manager keyed lookup→主 active vector→device action query 的链使用 T0-04 现有证据，消费者地址及 action 见 P05/证据 v6 §5。T0-04 的实机 witness 属于历史证据，不是本轮新动态观察。本轮未扩展到所有 SetControllerId callers 或 Player readers。

## P05：自有快照到 gameplay 与 Join 消费者

Native query ABI：device+0x30（slot12/A1E8D0）pressed 和 +0x34（slot13/A1E970）triggered 均通过 device this、stack action 参数返回 AL，清栈 ret4；+0x38（slot14/A1EA20）value 通过 x87 ST(0) 返回 float，stack action 参数，ret4。native 版本比较 C0[action]、C4[action] 与 threshold，并可继续查询 action 对应四 primitive；自有覆写如果只读私有 current/previous，不会更新或消费这些 Base 字段。

CODE/DESIGN：input_core.hpp 定义 gameplay action 数为12、最高逻辑 action 数为32、每 action 四 primitive；active_apply.cpp 把 move_x/y 正负映射 action0–3、aim_x/y 正负映射 action4–7、edge bits 映射 action8 bomb、9 item、10 card/pill、11 drop。查询 snapshot 只服务自然 gameplay consumer；action≥12 返回原生路径。T0-04 已有的 game consumers 是 movement、shooting、bomb、item、pill/card、drop；其 native key/consumer witness 只作为 STATIC-INHERITED 引用，不重复声称本轮运行观察。

关键差异：native Join 在 +0x34 查询 19（Join）、14（confirm）、15（cancel）、20/21（导航）；当前 canonical InputState/RawInputSample 只有 gameplay 八轴/四 edge，SyntheticAdapter::Evaluate 对 action≥12 透传。于是当前六个 vtable 覆写槽只是可承载查询 ABI，当前状态模型并未提供 Join 输入；普通 Join 不能靠 gameplay snapshot 自动成立。最小 DESIGN_ONLY 候选为 coordinator 提供一个独立 Join epoch command，含动作集合及身份/目标序号，在 poll 前发布；自有 +0x34 对 19/14/15/20/21 按本轮只读 edge map 返回 AL；完成、cancel 或 epoch 过期时关闭 Join output。action query 保持纯读，避免查询次数改写状态。这个接口要与 gameplay snapshot、authority tick 和 manager poll epoch 明确对齐；目前 scheduler→native poll 的正常阶段关系与 tick映射 UNKNOWN。

即使不谈 Join，current/previous 也有两个不同来源：native A1F280 每次 common update 先 C4←C0、清 C0 后才调自有 poll；私有 poll 若依契约将已发布快照锁入 current、旧 current 变 previous；native slot21只改 native C4，不写私有 previous。契约称 query 只读是正确约束，但需要额外的快照发布阶段避免跨 poll 重播 gameplay edges。action8–11 在当前 C/S ActionTriggered 实现中按有效 edge flag 返回真；它本身不提供 Join 14/19。

因此最小自有 ABI 仍保留六个覆写槽（dtor +0、名称 +0x0C、pressed +0x30、triggered +0x34、value +0x38、poll +0x78）；本轮没有证据要求新增 slot7 覆写。功能设计仍需增加名称实现、Join action source、明确的快照/poll epoch、Join output关闭和 B 退出/解绑接口。六槽结构本身不足以覆盖现有 C/S canonical model 与 native Join 两类事件。

## P06：正常停用与解绑

可提出且与既有 NEVER 契约一致的正常顺序：

1. coordinator 在确定的生命周期阶段关闭 Join admission 与该设备的 Join action 事件；后续自有查询返回中性值。
2. 若 B 保留为 Player，则显式调用 SetControllerId(B,-1,propagate=1) 解除 key 关联；或者由普通游戏路径让 B 退出。已核实 setter ABI 为 stack key/flag、retn 8、写 Player+0x1618；这一步不会释放 Player，也不改变已记录 ordinal。setter 是否是期望的玩家语义及退出路线要由上层设计确定。
3. 按值 pair 调 Manager remove A20EB0。正常路径会 dispatch Manager +0x7C/+0x80 清理，然后在主 active vector 中按 device pointer 擦除 pair、释放 pair 引用并通知；+0x80 的 A213B0 会搜索辅助表 pair、移除匹配项、通知并清 device+8 bit0。
4. 通过 active-vector keyed lookup/成员检查确认设备不再存在；不能只以一次 remove 调用视为确认。对象/control block 不释放，模块不卸载，满足已采纳 NEVER 语义。

STATIC-INHERITED + 本轮复读：95B310 对自有 Base 派生对象的 Gamepad cast 不命中；Manager remove 关联的通知或 RTTI 分支不含明确的 Player+0x1618 写点。移除 active pair 因此不等于 B 解绑。remove 的正常路径也不会清 Player selector key；selector UI 若仍活动需 coordinator 先 cancel/关闭，或另行明确正常清理。

准确未完成处：正常 B-exit 的具体入口未在本轮静态定位；SetControllerId(-1,1) 的 native assignment 可行，但解绑后无设备键的 B 在实际游戏中如何保留/退出不是此 setter 的单独语义；manager remove 的运行时通知回调集合未知；remove 后的 selector/roster实际更新要由更多正常控制流或运行值确认。故本节是必要 coordinator 顺序及已确认写点，不是正常停用已实机验证。

## 继承、勘误与 O/F 导航

- 本轮复用并有限复读静态确认 v2 §10 / W09：954CD0→6FA540→9BAC40→A20FB0/A20390→A209E0/A20940、action 19/14/15/20/21、selector 布局、普通分支、9B9CD0 与 7A6450、玩家+0x161C ordinal。没有把历史 W09 当本轮新动态证据。
- 复用 v4/v5 对 C0=current、C4=previous、slot21 的 C4←C0、A1E760 receiver=临时输出记录、ManagerBase 与 Manager 表身份的修正；本轮再次读了相关当前入口/关键原始指令。保留 v5 中唯一审阅实际未通过但正文预填通过的历史矛盾，不重审、不修改旧文件。
- 复用 T0-04 handoff 的 gameplay action/consumer 映射与 native witness；本轮用当前 C/S 源码核对 action0–11 语义，但不重开 T0-04 运行证明。
- O/F 导航：O-CTOR/O-CB/O-INSERT-ABI/O-NOTIFY 映射到 P01–P02 与证据 v6 §1–2；O-KEY/O-JOIN 映射到 P03–P04/证据 v6 §3–4；O-ACTION/O-ABI-QUERY/O-BUF 映射到 P05/证据 v6 §5；O-RTTI 映射到 P02/P06；O-REMOVE 映射到 P06/证据 v6 §6。F1–F8 的既有安全/生命周期判断不在本轮重新验收；只引用它们作为历史导航，不据此提升安全完成度。

## 未闭合问题与实现边界

| 类别 | 本轮留下的明确缺口 |
| --- | --- |
| 静态目标数据 | Init 后 native binding vector 实际长度/默认内容；C799F4 运行时值；通知 listener 实际集合；B 的正常退出及 Join output 的 coordinator 接入点 |
| 接口设计 | Join 14/15/19/20/21 的来源、单 poll/authority epoch、query 纯读但自然边沿仅出现一次、关闭 Join 输出、Player unbind/exit 与 remove 顺序 |
| 运行时值 | 当前对象 vptr/key、当前 roster/player count、selector 状态、当前 Manager pair 和辅助表内容、线程/时间关系、是否可实际创建/加入 B |
| 未实现 | 自有 38 槽表与 COL、名称返回、Join event source、native register/Join/B assignment/快照消费/停用完整生产接入、实时解绑确认 |
| 范围外 | OOM/EH、同步/worker并发、计数原子与溢出、全映像 alias/points-to、持久化和异常恢复。本轮不重判这些问题已解决 |

STATIC/CODE 事实可支持六条链的必要 ABI、原生分支、字段与顺序；它不能证明六条路径在现有生产代码中已接通。正常 gameplay query 与 native Join query 是两类动作输入，必须完成上表接口设计后才能谈端到端功能。

## tx-01-06 唯一一次整体静态文档审阅

审阅在报告、证据、README 和设计要求均完成后人工通读一次；没有运行程序、构建、测试、脚本或 verifier。唯一审阅结果：**未通过**。发现后只作本节、证据与 README 的结果回填，没有修复事实/覆盖缺口，也没有重跑审阅。

| 检查项 | 实际结果 |
| --- | --- |
| P01–P06 正常功能覆盖 | 通过（六链均有静态结果或具体 UNKNOWN；不表示功能完整） |
| receiver、offset、slot/action/ABI、C0/C4、key/ordinal、三种容器一致性 | 未通过：P01 与证据 v6 §1.2 将 A20DF0 的 +0x78 通知 receiver 写成 device vptr；读取指令显示 this=Manager，故应为 Manager vptr。未修复 |
| W09 与 T0-04 复用边界、勘误 | 通过（历史/本轮证据边界与 v5 失败状态保留） |
| S2 快照/Join接口、slot7 与 slot21差异 | 部分通过：缺口与 DESIGN_ONLY 接口已标注；P05 只列 T0-04 的 consumer 地址集合，未逐项对应六种 consumer/action，设计要求的 action→consumer 定位不完整 |
| 设计/静态/运行值/未实现边界 | 未通过流程要求：两次误用工具使历史哈希行意外出现在输出中；未计算/验证/使用，已记录并报告，但仍违反禁止读取要求 |
| README/报告/证据状态与历史记录一致 | 通过（回填后状态一致，保留 tx-01-03 历史失败） |

文档交付准确性：**未通过**。除上述 receiver 错误外，P02 证据在报告 C799F4 aux count 后紧接 binding template 长度 UNKNOWN，读者可能误以为该全局是 binding template 长度来源；本轮实际证据只把 C799F4 关联到 auxiliary table，binding n 来自对象 vector end-begin，故该证据边界表述不够清楚。P05 未把继承的六个 native consumer RVA 一一对应到 action/group。

六条正常功能链明确程度：注册/active vector 与 Join、key/ordinal、ABI、slot21 flush、停用必要步骤大体明确；P02 默认 binding 运行长度、P05 Join event/poll epoch、P06 B 的正常 exit/解绑语义仍未闭合。静态链不代表端到端功能完成。

最小实现仍需：提供 name slot；证实 native Init 后至少 32 项 binding；定义独立 Join 14/15/19/20/21 event source 和只读 +0x34 映射；把该 command epoch 与 authority tick/native poll 对齐；明确 Join 关闭、B exit 或 SetControllerId(-1,1)、remove 与成员复核顺序。

剩余 UNKNOWN：C799E8/C799EC 模板的实际运行内容与对象 vector 长度；实际 notification listener；当前 roster/player count/key/vptr/selector/table/thread；scheduler 与 native poll 相位；自然 B exit 路径及解绑后的实际玩家语义。

本审阅是唯一一次。上述结果记录不是修复或再次验收；保留已发现 receiver 错误、P05 缺少逐项 consumer 对应、P02 来源表达歧义及工具流程偏差。

本轮执行偏差：在两次误用旧 handoff 阅读及宽泛导航搜索时，工具输出意外含有历史哈希行。没有计算、比较、校验或使用哈希值，已向父智能体报告；该行为仍违反本工作“不得读取哈希”的约束，并在证据 v6 记为流程偏差。审阅后仅按设计回填真实结果，不修复失败项、不重跑或进行第二次审阅。
