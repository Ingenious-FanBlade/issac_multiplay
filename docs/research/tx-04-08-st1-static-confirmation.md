# tx-04-08 ST1：world_record 入口与 ABI 静态核验

日期：2026-10-26，Asia/Shanghai。范围来自[迁移方案第 4.3 节](../design/tx-04-08/tx-04-08-implementation-plan.md#43-后续静态工作-st1st8)。本次用户授权启动 ST1，并要求一个 **GPT-6 Luna / max** 子智能体负责逆向、主智能体审阅。初始设计会话的“仅设计”授权说明保留为历史语境。

本文登记 19 项观察语义的目标入口、原始指令、ABI、所属对象和 ZHL 对应；不将字段、来源、分类闭包、Hook 链次序或运行效果扩展为已确认。ST2–ST8、工程实现和 tx-04-08 的整体 E2E 尚未完成。本次没有运行测试、构建、生成器、游戏或离线工具，没有计算/校验哈希，没有修改 IDB、安装 Hook 或部署 DLL。

## 1. 目标与证据口径

主智能体直接读取当前 IDB 的输入路径、imagebase 和 VA `0xB2EEA0` 的版本字符串：输入文件名为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，版本为 `Binding of Isaac: Repentance+ v1.9.7.12.J273`，imagebase 为 `0x400000`。目标是 x86。`RVA = VA - 0x400000`；以下 VA 不直接作为实际进程地址。

SDK 源码来自本地 `F:/projects_isaac/REPENTOGON-1.1.3/`。`STATIC_CONFIRMED` 表示当前 IDB 的指令/调用关系，`CURRENT_SOURCE` 表示该 SDK 或当前仓库源码；两者都不是本次运行观察。子智能体材料见[逆向笔记](tx-04-08-st1-agent-notes.md)，最终结论以本文的主审限定为准。

主审原始材料：

- [19 项地址交叉读取](../../artifacts/repentogon/tx-04-08-static/st1-review/main-entry-crosscheck.json)：17 项完整官方特征逐项搜索与入口 32 字节、主块正常 `retn`，另列 Publish/Replace。
- [创建/发布/构造反编译](../../artifacts/repentogon/tx-04-08-static/st1-review/main-owner-decompile.json)。
- [GridFactory/Spawn/析构反编译](../../artifacts/repentogon/tx-04-08-static/st1-review/main-risk-decompile.json)。
- [Replace 独立定位与原始调用点、Grid 寄存器指令](../../artifacts/repentogon/tx-04-08-static/st1-review/main-replace-and-grid.json)。
- [Factory/Init/ManagerRestore/GridFree 与原始片段](../../artifacts/repentogon/tx-04-08-static/st1-review/main-lifecycle-details.json)。
- [官方 SDK/生成后 ArgData/Hook 摘录](../../artifacts/repentogon/tx-04-08-static/st1-review/sdk-source-excerpts.json)。
- [IDB 目标上下文](../../artifacts/repentogon/tx-04-08-static/st1-review/main-idb-context.json)、[实际虚表入口](../../artifacts/repentogon/tx-04-08-static/st1-review/main-vtable-entries.json)与 [Entity 删除包装汇编](../../artifacts/repentogon/tx-04-08-static/st1-review/main-entity-free-wrapper.json)。

反编译保留 IDA 的原始类型推断，包括不准确的 `userpurge`、整数/指针和返回类型。ABI 结论按原始参数读取、调用点、清栈与官方元数据交叉解释，不能直接复制伪代码签名。

## 2. 十九项入口登记

下表的入口字节均截至完整指令，仅用于描述所观察入口；不是建议的补丁长度。正常返回字节和更长入口见原始材料。17 项完整官方特征各唯一命中对应入口；Publish 与 Replace 通过调用关系和对象操作定位，缺官方函数定义。

| 语义 | 目标函数 | VA | RVA | 入口原始指令字节 |
| --- | --- | --- | --- | --- |
| Factory | `EntityFactory::Create` | `0x6EC540` | `0x2EC540` | `55 8B EC 51 8B 45 08` |
| Init | `Entity::Init` | `0x6A4A20` | `0x2A4A20` | `55 8B EC 83 EC 30` |
| Publish | EntityList 发布 owner，自有定义候选 | `0x413D00` | `0x013D00` | `55 8B EC 53 8B 5D 08` |
| Remove | `Entity::Remove` | `0x6A8000` | `0x2A8000` | `51 56 8B F1 57` |
| ClearReferences | `Entity::ClearReferences` | `0x6A8060` | `0x2A8060` | `55 8B EC 51 53` |
| Destroy | `Entity::destructor` | `0x6A3EF0` | `0x2A3EF0` | `55 8B EC 6A FF` |
| PlayerCtor | `Entity_Player::constructor` | `0x74DC70` | `0x34DC70` | `53 8B DC 83 EC 08` |
| PlayerCreate | `PlayerManager::SpawnCoPlayer2` | `0x990250` | `0x590250` | `55 8B EC 6A FF` |
| ManagerRestore | `PlayerManager::RestoreGameState` | `0x993A90` | `0x593A90` | `55 8B EC 6A FF` |
| Replace | `PlayerManager::ReplacePlayer` 语义，自有定义候选 | `0x995E10` | `0x595E10` | `53 8B DC 83 EC 08` |
| PlayerRestore | `Entity_Player::RestoreGameState` | `0x7A5660` | `0x3A5660` | `53 8B DC 83 EC 08` |
| NpcMorph | `Entity_NPC::Morph` | `0x6BD0C0` | `0x2BD0C0` | `55 8B EC 83 E4 F8` |
| PickupMorph | `Entity_Pickup::Morph` | `0x6DFF30` | `0x2DFF30` | `55 8B EC 6A FF` |
| GridFactory | `Isaac::CreateGridEntity` | `0x706100` | `0x306100` | `55 8B EC 6A FF` |
| GridInit | `GridEntity::Init` | `0x705070` | `0x305070` | `55 8B EC 8B 45 08` |
| GridSpawn | `Room::SpawnGridEntity` | `0x7E21D0` | `0x3E21D0` | `55 8B EC 53 56` |
| Door | `Room::MakeDoor` | `0x7E4DE0` | `0x3E4DE0` | `55 8B EC 6A FF` |
| GridFree | `Room::RemoveGridEntityImmediate` | `0x813BD0` | `0x413BD0` | `55 8B EC 8B 55 08` |
| Spawn | `Game::Spawn` | `0x6FA210` | `0x2FA210` | `55 8B EC 83 E4 F8` |

## 3. ABI、ZHL 与完成含义

以下 `S1…Sn` 指**进入原生入口时** `[ESP+4]…[ESP+4n]` 的 32 位 word。指针/引用在 x86 都占一 word；bool 参数也占一栈 word，但观察其有效低字节，不擅自将其它位规范化。除 GridFactory 外，登记的对象方法均以 ECX 接收 `this`，由被调用方清理所列栈参数。`void` 是官方观察接口契约，不把残余 EAX 当成可供业务使用的结果。

| 语义 | this / 显式参数与返回 | 正常清栈 | 官方 `.zhl` / owner 限定 |
| --- | --- | --- | --- |
| Factory | ECX=EntityFactory；S1=u32 type，S2=bool force；EAX=Entity*/null | `ret 8` | `EntityFactory.zhl:2`；分配或池取得完成，不等于 Init/Spawn 完成 |
| Init | ECX=Entity；S1–4=u32 type/variant/subtype/seed；官方 void | `ret 0x10` | `Entity.zhl:259`，虚表槽 1（`+4`）；基类 Init 正常返回不代表派生 Init 或外层 owner 已返回 |
| Publish | ECX=EntityList；S1=Entity*；EAX=尾部 helper 的结果，业务意义 UNKNOWN | `ret 4` | 缺定义；容器发布，不从返回值取得 Entity 或推断完整出生 |
| Remove | ECX=Entity；无显式参数；void | `ret` | `Entity.zhl:278`，槽 10（`+0x28`）；逻辑移除请求，不能等同析构 |
| ClearReferences | ECX=Entity；无显式参数；void | `ret` | `Entity.zhl:291`，槽 17（`+0x44`）；引用清理与 revision 观察 |
| Destroy | ECX=Entity；无显式参数；官方 void | `ret` | `Entity.zhl:5`；基类物理析构体，未承诺已执行 operator delete |
| PlayerCtor | ECX=Player；无显式参数；官方 void；原生成功路径 EAX=this | `ret` | `EntityPlayer.zhl:9`；构造 owner，不能把进入或异常当完成，也不把构造当完整初始化 |
| PlayerCreate | ECX=PlayerManager；S1=i32 playerType；EAX=Player*/null | `ret 4` | `PlayerManager.zhl:5`；真实创建 owner，包含构造/Init/列表发布，不能再创建一个 Player |
| ManagerRestore | ECX=PlayerManager；S1=GameState*；void | `ret 4` | `PlayerManager.zhl:57`；多个 Player/保留关系的聚合 owner，manager 本身不是 Entity |
| Replace | ECX=PlayerManager；S1=旧 Player*，S2=新 Player*；AL=0/1 | `ret 8` | 缺定义；交换/替换 owner，两侧对象分开记录；失败不是成功替换 |
| PlayerRestore | ECX=Player；S1=GameStatePlayer*；void | `ret 4` | `EntityPlayer.zhl:602`；状态恢复，不能补造构造/出生历史 |
| NpcMorph | ECX=NPC；S1–4=i32 type/variant/subtype/champion；void | `ret 0x10` | `EntityNPC.zhl:2`；Morph owner，原生与 REPENTOGON 链正常完成须区分 |
| PickupMorph | ECX=Pickup；S1–3=i32 type/variant/subtype，S4–6=bool KeepPrice/KeepSeed/IgnoreModifiers；void | `ret 0x18` | `EntityPickup.zhl:2`；布尔只读低字节，不套 NPC 的四参数模板 |
| GridFactory | **无 this**；ECX=i32 type，EDX=u32 **GridIndex**；EAX=Grid*/null；无原生栈参 | `ret` | 优先 `Isaac.zhl:4` 的 `Isaac::CreateGridEntity`；工厂与 Init/Room 发布分开 |
| GridInit | ECX=Grid；S1=u32 Seed；void | `ret 4` | `GridEntity.zhl:5`；普通方法，不冒充虚表派生初始化入口 |
| GridSpawn | ECX=Room；S1=i32 index，S2–4=u32 type/variant/seed，S5=i32 VarData；AL=bool | `ret 0x14` | `Room.zhl:32`；Room owner 返回成功后仍需确认实际槽及前后对象 |
| Door | ECX=Room；S1=i32 door slot；void | `ret 4` | `Room.zhl:92`；门 owner，无 Entity* 返回契约，需观察实际 door/Grid 别名 |
| GridFree | ECX=Room；S1=i32 index，S2=i32 PathTrail，S3=bool KeepDecoration；void | `ret 0x0C` | `Room.zhl:38`；移除 owner 含空槽/保留 decoration 分支，调用不等于已释放 |
| Spawn | ECX=Game；S1–2=u32 type/variant，S3–4=const Vector& 指针，S5=Entity* spawner，S6–8=u32 subtype/seed/unk；EAX=Entity*/null | `ret 0x20` | `Game.zhl:29`；完整八参 owner，Publish 之后仍有后处理；末参原生读取低位不允许删去第八 word |

虚表槽号只描述这些已定位目标的官方基类虚表位置，不证明七类派生虚表全部共享同一实现。派生覆盖、池复用和分类闭包留给 ST5/ST6；不得把单个基类 Hook 计作完整析构/Init 覆盖。

主审实际读取 Entity 虚表 VA `0xB18224`，槽 0/1/10/17 分别为 `0x6A3EC0/0x6A4A20/0x6A8000/0x6A8060`。Player 的 Init/ClearReferences 分别为 `0x74F6A0/0x751B90`，NPC 的 Remove/ClearReferences 为 `0x6B4E30/0x6B4E60`，Pickup 对应为 `0x6EA840/0x6EA860`；这些已经直接证明不能概括为全部同址共享。派生目标是否最终调用基类，要另追调用关系。

## 4. 主审确认的关键区别

### 4.1 GridFactory 的同址定义与 Hook 侧 ABI

`0x706126` 的 `8B DA` 保存入口 EDX 到 EBX，`0x706128` 的 `8B F9` 保存 ECX 到 EDI；`0x706636` 的 `89 7E 04` 写 type，`0x706639` 的 `89 5E 24` 写 Grid index。两条正常路径均为不带立即数的 `ret`，没有两个栈 word。

`GridEntity.zhl:2` 的全局 `CreateGridEntity(GridEntityType type, unsigned int seed)` 与 `Isaac.zhl:4` 使用同一完整特征；参数名 seed 不符合该指令用途。生成后的 `libzhl/IsaacRepentance.cpp:33038–33043` 为 `Isaac::CreateGridEntity` 明确登记 ECX/EDX、flags=2，其 Hook 侧为普通静态函数参数；这与 `GridEntityPlus.cpp:42` 的 `__cdecl` Hook 声明相容，**不能把 Hook 侧 cdecl 当原生传参**。

相同源码 `IsaacRepentance.cpp:42227–42232` 的全局定义却登记 `NoRegister/ECX`、flags=0。它与已核对原生 ABI 不符，本次不选它作为 world 的绑定依据。优先复用 `Isaac::CreateGridEntity` 的真实 ArgData 及既有 ZHL 入口链；不要为同址全局别名另建入口补丁。实际同址多定义的注册/链行为仍属 ST8 的未完成项。

### 4.2 Replace 的真实调用约定

主审从 Player 的 `+0x267C` 互相赋值定位 `0x995E10`。该函数 `push ebx; mov ebx,esp` 后，`0x995E4B` 读 `[EBX+8]`（原 S1，旧 Player），`0x995E4E` 读 `[EBX+0xC]`（原 S2，新 Player）；`0x995E51/0x995E5E` 互写两侧 `+0x267C`。它既有 roster 交换分支，也有查找旧 Player 后替换列表项的分支。

调用点 `0x7A41F8–0x7A420A` 先 push 新 Player（ESI）、再 push 旧 Player（EDI），ECX 取 Game 内的 PlayerManager，再 call。`0x995F33/0x996497` 写 `AL=1`，`0x995F55` 写 `AL=0`，`0x995F74` 是 `C2 08 00`。因此可表达为 `bool __thiscall(PlayerManager*, Player* old, Player* replacement)`；Hex-Rays 展示的额外 EBP 输入是对齐/局部框架分析伪象，不增设 EBP 参数。

### 4.3 Publish、析构与 Restore 不混用

PlayerCreate 的 `0x9904A0` 和 Spawn 的 `0x6FA3B3` 均调用 `0x413D00`，ECX 是实际 EntityList 容器，栈实参才是 Entity。Publish 最后调用 `0x416D10(type,variant,subtype)` 后返回其 EAX；当前无需使用该值，语义保持 UNKNOWN，不能将其声明为返回 Entity*。

Spawn 的 Publish 之后仍有属性继承、引用及其它后处理；创建完成必须绑定实际外层 owner 的正常返回。PlayerCreate 还有特殊参数分支，不应把函数每次返回都概括为普通 roster 发布；更完整的嵌套/就绪规则留给 ST6。

Destroy 对应 `Entity::destructor 0x6A3EF0`，不是官方虚表槽 0 的 `Entity::Free(flags)` 删除包装。基类 dtor 重置虚表、清引用并拆成员，不自行证明内存释放。GridFree 则是 Room 的三参数方法，在 `0x813BFF` 清槽后 `0x813C0B` 调 Grid 虚表槽 0（flags=1），与 Entity 析构体完全不同。

`Entity::Free` 实际入口为 `0x6A3EC0`：`0x6A3EC6` 先调用无参 dtor，`0x6A3ECB` 测试 flags 的 bit 0，条件成立才进入释放路径；`0x6A3EE3` 为 `ret 4`。这与本次 Destroy 的无显式参数、普通 `ret` 有直接区别。

ManagerRestore 是 `0x993A90`，不是当前 Hook `restore` 的 `Game::RestoreState`（RVA `0x2F43A0`）；其内部可构造/恢复多个 Player。`PlayerManager::RestoreGameState_PostLevelInit` 和 Player 的同名后阶段也是另一个边界，不能由当前一个入口外推全部恢复完成。

### 4.4 原型与反编译的实参不能直接迁移

旧 `world_record.cpp` 的原始 RVA/汇编 wrapper 只用作语义索引；本表全部来自目标 IDB 的新读取。NPC Morph 四参、Pickup Morph 六参与 Game Spawn 八参必须分开表达，不能沿用观察帧里未定义的其它参数 word。

`Entity::Init` 的伪代码还展示 EDI/ESI 输入；原始入口明确从四个栈 word 写入 type/variant/subtype/seed，保存 ESI/EDI 属于 callee-save，内部虚调用未证明额外业务参数。本次不据该伪签名扩张 Init 的四参数 ABI；其它未解析虚调用的行为不在 ST1 内补造。

## 5. 与现有 Hook 清单的接线约束

`CURRENT_SOURCE`：当前 `mod/binding/hooks.json` 的 41 行中没有上述 19 个入口的同 RVA wrapper。PlayerCreate 已被 `join_create` 的 CALL 点（RVA `0x591FE6`）转入 RVA `0x590250`，这是既有创建路径的共享，**不是已存在 PlayerCreate 入口 wrapper**。新增观察不能再创建 B，也不能另装同址入口补丁。ManagerRestore、Remove 不得与已有 Game Restore、Entity Kill 按名字近似合并。

阶段通知应加入现有 Manager/Game/Room/tail/Player/Restore wrapper；这是方案要求的共享 wrapper，不意味着 ST1 的 19 项已经注册。REPENTOGON 自己已有 Factory、Entity Remove/析构、Player 构造、Morph、GridFactory/GridInit/GridSpawn 和 ManagerRestore 等相关 Hook；其中 Morph/GridSpawn 可因 callback 返回而短路或改参。Hook 观察层返回、下一层返回和真正原生 body 到达必须分开。具体 priority/链闭包留给 ST2/ST8。

17 项优先复用官方定义；Publish/Replace 缺定义，需要目标地址、正确成员类型及原始返回/透传约定的自有 FunctionDefinition。当前 `gen_binding.py` 按逗号数登记参数，`binding/definitions.cpp` 给自有参数一律 `NoRegister,1`；它不能表达 GridFactory 的 ECX/EDX。不能仅填 `fastcall` 字符串就认为 ABI 已被支持。

Publish 的自有桥应保存并原样返回原生 EAX raw32，语义仍记 UNKNOWN；不要因当前观察逻辑不用结果而改成丢弃返回值。PlayerCtor 则应按已有官方 void Hook 观察 `this`，不能在 C++ 中从 `super()` 取得 Entity*。原生成功路径 EAX=this 与官方 void 包装必须分列：`HookSystem.cpp:502–542` 对 void 保存/恢复入口 EAX，并不把它解释为业务返回值。全部调用点与现有链的寄存器保持闭包留给 ST8，不能仅凭“官方有定义”声称原生逐寄存器行为已验收。

当前注册表 `pending[64]` 超限静默忽略，方案必须修正这一实现风险；41+19=60 只是按本表逐项新增的候选算术，不是实际注册/安装数。统一清单、world 故障隔离、原调用 exactly-once、异常中立与常驻逻辑启停仍按[现行规范](../refactor/project-conventions.md)和迁移方案实施，本次未改这些代码。

## 6. 主智能体审阅结论

**ST1 的 19 项入口/ABI 登记与主审已完成**，UNKNOWN 按上述限定保留。这是 tx-04-08 进行中的静态资料工作，不是对子项另做测试验收，也不代表整个 tx-04-08 完成。

主智能体逐项回读子智能体的 19 行笔记，并独立取得完整官方特征匹配、原始入口/清栈、Replace 调用点、基类/派生虚表和 SDK ArgData。审阅没有只接受代理摘要：主智能体补出 Replace 的目标和两参数/AL 契约，纠正了 GridFactory 查询少一个 `51` 所造成的误命中，并要求将 GridSpawn 的 index/VarData 有符号类型、bool 仅 AL 的返回意义及 PlayerCreate 成功路径限定写清。代理笔记保留查询错误与修订说明。

最终接受的是本文中的具体入口、实参位置、原始返回/清栈和有来源的 owner 限定；没有接受完整派生覆盖、完整恢复/出生判定或实际 Hook 链已验证的扩张结论。当前工程与原始历史失败记录保持原样。

## 7. 保留事项与继续位置

入口登记不解决：派生虚表覆盖与物理/use 代次、Morph/Restore 的最终 owner 就绪、正常采集尾部/Hook 短路顺序、全部字段/来源与 Grid 447/448 项差异、共享链注册/容量、线程和退出收口。它们分别归入 ST2–ST8。GridFree 局部指令允许 index `0x1BF`，这只是该方法的实际边界观察，不能单独决定完整采集数组上限。

Publish 返回意义、官方同址全局 GridFactory 元数据的运行影响、完整派生覆盖和所有运行时行为继续明确为 UNKNOWN。本次静态资料不增加运行 PASS，不消耗或替代功能完成后的整体 E2E，也不恢复任何旧 gate 或哈希流程。
