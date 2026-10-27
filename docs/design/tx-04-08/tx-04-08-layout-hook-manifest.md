# tx-04-08：布局与 Hook 清单生成器（工程顺序中的清单项）

日期：2026-10-27，Asia/Singapore。状态：**清单与生成器已编写；生成器在临时构建目录运行一次，产物完成纯编译（Release Win32、MSVC 19.44、`/W4` 无项目警告）；未运行测试、未启动游戏、DLL 整体链接仍未完成**。范围见[迁移设计第 10 节](tx-04-08-implementation-plan.md#10-工程顺序与验收边界)所列实现顺序的“ST1–ST8 与 layout/Hook 清单”一项（先于它提交的是[world 核心](tx-04-08-world-core.md)）。

本文中 `CURRENT_SOURCE` 指本次新增/修改的源码，`STATIC_CONFIRMED` 指目标 IDB 的静态事实，`DESIGN` 指沿用迁移设计的目标行为，`UNKNOWN` 指尚未取得运行证据的部分。

## 1. 交付文件

| 文件 | 状态 | 内容 |
| --- | --- | --- |
| `mod/binding/hooks.json` | 修改 | 唯一 Hook 清单升级为 schema 2：保留 41 条既有位置式行，新增 `world_observations`（19 项 world 观察入口，逐参数 ABI、完成含义、证据、SDK 元数据）与 `schema`（版本、待注册容量、布局清单引用） |
| `mod/binding/world_layout.json` | 新增 | 目标版本只读布局清单：常量、descriptor 形状、78 项字段、15 个来源、10 个 RTTI 类（含基类图）、假设与未知 |
| `mod/tools/gen_binding.py` | 修改 | 唯一生成器：校验两份清单、生成全部产物（见第 5 节），校验失败以非零退出并逐项打印问题 |
| `mod/src/binding/world_layout.h/.cpp` | 新增 | 生成表的只读访问层：字段/来源/类别/观察入口、常量、manifest 形状、观察掩码、内部一致性校验 |
| `mod/src/world/observation.h` | 新增 | 19 项观察语义的编号（C++17 兼容），供生成 wrapper 与 world 实现共用 |
| `mod/src/world/observe.h/.cpp` | 新增 | world 观察专用策略层：计数、故障位、只读诊断快照；不进入会话失败路径 |
| `mod/src/world/model.h/.cpp` | 修改 | 78 项 profile 声明移到头文件成为 `constexpr` 表，供布局清单在编译期逐项比对 |
| `mod/src/hooks/cs_hook.h` | 修改 | 新增 `CS_WORLD_HOOK_METHOD/STATIC`：group=World、携带 Observation 编号与 `arg_spec`、不经过 `CallScope` |
| `mod/src/hooks/registry.cpp` | 修改 | 待注册表按生成容量声明；超限显式保留并报告；world 组按 `WorldObservationArmed()` 延后安装；新增 `hook_summary` |
| `mod/src/binding/definitions.cpp` | 修改 | 自有定义的 ArgData 由 `HookArg` 表构造（栈 word / GP / XMM）；容量不足停用对应能力组；world 组参与 binding 校验 |
| `mod/src/runtime.h`、`mod/src/bootstrap/mod_init.cpp` | 修改 | `Group::World`、`HookArg`、`Hook.arg_spec/observation/world`、`HookCapacity`、`WorldObservationArmed()` |
| `mod/CMakeLists.txt` | 修改 | 新增 `world/observe.cpp`、`binding/world_layout.cpp`；生成命令增加 `hook_capacity.h`、`world_layout.inc`、`world-layout.json`，并依赖 `binding/world_layout.json` |

`isaac_cs_world` 仍不依赖 SDK、游戏头或 `runtime.h`；`binding/world_layout.*` 属于 DLL 侧 binding 层（依赖 `runtime.h` 的 Hook 记录，用于观察掩码）。

## 2. Hook 清单 schema 2（CURRENT_SOURCE）

### 2.1 两种行式

- `hooks`：41 条既有位置式行，字段顺序仍是 `id/group/class/method/返回/参数/调用实参/RVA/owner/static/conv/own/priority/required`。它按“每个显式参数占一个栈 word”表达 ABI，只适用于这 41 个已核对入口；生成器不改写它们的 wrapper 语义。
- `world_observations`：19 个对象行，逐参数声明 `stack:N` 或寄存器名（`ecx`/`edx`/`xmmN`），并登记 `class/method/definition/own/static/conv/priority/return/return_abi/entry_bytes/completion/reach/evidence/required`。

两类行分开登记，`hook-manifest.json` 也保持 `hooks` 只含既有 41 条。原因是既有 E2E harness 把 `hooks` 当作“应当逐个出现一次 `hook_registered`”的清单（`tools/harness/src/tx07/evidence.rs`）：world 观察入口当前延后安装，若混进 `hooks` 会让既有对账口径失真。world 入口单独在 `world_observations` 中登记，另有 `native_body_reach` 等字段。

### 2.2 ABI 校验

生成器不再按逗号数猜参数：

1. 由 `static` 与 `conv` 推导 ZHL flags（`THISCALL=1`、`CLEANUP=2`、`VOID=4`），并与清单登记的 `sdk.flags` 逐项比较；官方定义的 `sdk.argdata` 必须与逐参数 ABI 推导出的列表完全一致。17 个官方定义全部通过；GridFactory 的 `ecx:0, edx:0` 与官方 `Isaac::CreateGridEntity` 元数据一致。
2. 自有定义（`Publish`、`Replace`）必须给出入口特征字节；生成器为其生成 `cs::HookArg` 表，`definitions.cpp` 用该表构造真实 `ArgData`，可表达寄存器参数。
3. 静态入口必须 `cdecl`、成员入口必须 `thiscall`；非 void 入口必须给 `return_abi`，void 入口不得给。
4. 同一 RVA 只允许 `hooks` 里已登记的成对层（`player_update -10000` / `player_native 10000`）；world 与既有入口、world 之间出现同址一律是生成错误。

### 2.3 容量

- `schema.pending_capacity = 64`（本 DLL 待注册表的固定容量），实际行数 **60 = 41 既有 + 19 world**，distinct RVA 59，headroom 4。
- 生成期：行数超容量即非零退出并列出全部行 id；`hook_capacity.h` 生成容量/行数常量，并以 `static_assert` 固定关系。
- 注册期：`AddPending` 在超限时保留前 8 条被拒记录并计数；`PrepareBindings` 停用对应能力组、写 `hook_capacity_error`/`hook_capacity`，不再静默忽略；`RegisterHooks` 每次输出 `hook_summary`（capacity/rows/used/rejected/installed/deferred）。

## 3. 十九项 world 观察入口（STATIC_CONFIRMED）

本轮主智能体在目标 IDB（`isaac-ng-v1.9.7.12.exe.unpacked.exe`，imagebase `0x400000`）上逐项重读 19 个入口的字节、10 个类别的 COL/CHD/BCD、Room/MakeDoor/Grid 等关键指令，与 [ST1](../../research/tx-04-08-st1-static-confirmation.md)、[ST3](../../research/tx-04-08-st3-agent-notes.md)、[ST4](../../research/tx-04-08-st4-agent-notes.md)、[ST5](../../research/tx-04-08-st5-agent-notes.md) 报告一致。登记为：

| 观察语义 | 定义 | RVA | 参数 ABI | 返回 |
| --- | --- | --- | --- | --- |
| Factory | `EntityFactory::Create` | `0x2EC540` | stack,stack | `Entity*` (EAX) |
| Init | `Entity::Init` | `0x2A4A20` | 4×stack | void |
| Publish | `IsaacCS::PublishEntity`（自有） | `0x013D00` | stack | `std::uint32_t` (EAX, raw32) |
| Remove | `Entity::Remove` | `0x2A8000` | — | void |
| ClearReferences | `Entity::ClearReferences` | `0x2A8060` | — | void |
| Destroy | `Entity::destructor` | `0x2A3EF0` | — | void |
| PlayerCtor | `Entity_Player::constructor` | `0x34DC70` | — | void |
| PlayerCreate | `PlayerManager::SpawnCoPlayer2` | `0x590250` | stack | `Entity_Player*` (EAX) |
| ManagerRestore | `PlayerManager::RestoreGameState` | `0x593A90` | stack | void |
| Replace | `IsaacCS::ReplacePlayer`（自有） | `0x595E10` | stack,stack | `bool` (AL) |
| PlayerRestore | `Entity_Player::RestoreGameState` | `0x3A5660` | stack | void |
| NpcMorph | `Entity_NPC::Morph` | `0x2BD0C0` | 4×stack | void |
| PickupMorph | `Entity_Pickup::Morph` | `0x2DFF30` | 6×stack | void |
| GridFactory | `Isaac::CreateGridEntity` | `0x306100` | **ECX, EDX** | `GridEntity*` (EAX) |
| GridInit | `GridEntity::Init` | `0x305070` | stack | void |
| GridSpawn | `Room::SpawnGridEntity` | `0x3E21D0` | 5×stack | `bool` (AL) |
| Door | `Room::MakeDoor` | `0x3E4DE0` | stack | void |
| GridFree | `Room::RemoveGridEntityImmediate` | `0x413BD0` | 3×stack | void |
| Spawn | `Game::Spawn` | `0x2FA210` | 8×stack | `Entity*` (EAX) |

Publish 与 Replace 缺官方 `.zhl` 定义，使用自有定义；两者都不与既有入口共用地址。GridFactory 只复用官方 `Isaac::CreateGridEntity` 的 ECX/EDX ArgData，不为同址全局别名另建 owner。

### 3.1 优先级与“原生 body 到达”

19 项统一使用 **priority `-10000`**（比 REPENTOGON 默认层更外层），理由与限定：

- 单层观察必须保证事件可见：若外层回调短路，world 仍记录“本层进入/返回”，不静默丢生命周期事件；
- 代价是“原生 body 是否到达”不能由本层证明。清单为每行登记 `reach` 文本，生成的 `hook-manifest.json` 把这些行标为 `native_body_reach = UNKNOWN`（自有定义为 `OWN_ENTRY`）；
- 若后续 ST8 的统一链决定要求内外双层（例如 Morph/GridSpawn 回调短路），应按语义增加内层行；当前 headroom 4 不足以给 19 项全部加内层，届时必须重新核算容量并显式报告。

## 4. world 观察策略层（CURRENT_SOURCE / DESIGN）

迁移设计 4.1/4.2 与失败方式 F4 要求 world 观察不得套用会话失败策略。`world/observe.h` 的作用域因此：

1. 递增该 Hook 记录自己的 `calls/foreign/completed`（指针由生成代码传入，本层不持有运行时类型）；
2. 非主线程进入时只计数并置 `WrongThread` 故障位，不读写身份表、不调用 `ForeignCall`；
3. 原生异常展开时只记录 `exceptional` 并置 `NativeException`，不伪造正常返回、不调用 `ContextFault`；
4. 保存最近一次主线程观察的只读诊断快照（对象地址、实参 word、返回 raw32），由主线程桥与状态命令读取；
5. 不分配内存、不抛异常、不加锁，游戏线程等待为零。

`ObservationScope` 记录的是**观察层**信息，不是 lifecycle 事件：`call/parent` 编号、事件 ring、epoch 与身份更新仍属主线程桥（下一项）。故障位只表示“world 观察资格受损”，不使 Host、B/K 或 Hook 安装失败；收口点由主线程桥决定。

## 5. 生成产物与生成期校验

一次生成写入 9 个文件：`hooks_generated.cpp`、`binding_v1_9_7_12.inc`、`permanent_patches.inc`、`host_patches.inc`、`hook_capacity.h`、`world_layout.inc`、`hook-manifest.json`、`hook-manifest.md`、`world-layout.json`（另加目录创建）。

生成期校验（失败即非零退出）：

- Hook：id 唯一、group/conv 合法、RVA 在映像内、world 行必须覆盖且只覆盖 19 项观察语义、definition 名必须与 own/class/method 一致、官方行的 `sdk.flags/argdata` 与 ABI 推导一致、自有行必须有特征字节且不得带 SDK 元数据、同址行只允许既有成对层、行数 ≤ 容量；
- 布局：schema/domain/profile 固定、常量键集合与顺序必须与 `world_layout.h` 的 `Constants` 完全一致、10 个 RTTI 类与 8 个 schema 类必须齐全、基类表的 PMD 只能是 `0/-1/0` 且 attributes 必须等于常量期望值、CHD 必须包含自身 TypeDescriptor、78 个字段 ID 升序唯一且宽度/编码/状态/基址合法、字段 `offset+width` 必须落在登记的对象尺寸内、15 个来源 ID 与名称必须与文件模型一致、固定数组 `max_index = count-1`、Grid 448 槽之后必须紧接 door 槽、door 之后再紧接 door→grid 映射表。

## 6. 布局清单（CURRENT_SOURCE）

78 项字段逐项登记 `base`（entity/grid/game/derived）、`owner`、`offset`、`width`、`encoding`、`status`、`sdk_member`、`evidence`。`status` 分层与 ST3 一致：`InitConfirmed`（位置与初始化写入均确认）、`InitDefault`（只见构造/Init 默认值）、`InitUnknown`（未找到写入，如 `removed_by_factory +0x177`）、`InitAssumed`（通用 HP/maxHP 假设）、`LuaBinding`（SDK 未命名但由原生 Lua 注册/getter 确认：SubPlayer、FireCooldown、Wait、Pickup State、ProjectileFlags）、`Derived`（frame_count、Grid 格点坐标）。

15 个来源逐项登记形状、基址与容量：八 EL（`Room+0x1238` 起 0x10 步进，16 字节 descriptor）、roster（`Game+0x1BAA8` 的 12 字节 vector）、Esau（`Game+0x1BB28`，4 槽）、保留关系（`+0x1D84/+0x1E4C/+0x1E50/+0x267C`，64 节点）、Grid（`Room+0x24`，**448 槽 / max index 447**）、Door（`Room+0x724`，8 槽 + `+0x744` 映射表）、两个 reserved。

10 个 RTTI 类登记 vtable/COL/TypeDescriptor RVA、对象尺寸、删除包装与 Init 入口、以及 CHD 基类图（七类与 Door 的 PMD 均为 `0/-1/0`、attributes `0x40`）。分类只比对这些 RVA 与继承关系，不调用 `dynamic_cast`/getter/Lua。

编译期校验（`binding/world_layout.cpp`）：

- 字段表与 `world/model.h` 的 `kProfileFieldTable` 逐项比较 ID、宽度、编码、名称；manifest 形状必须与完整表同序同值；
- 类别/来源/观察入口必须按固定顺序完整登记；
- 常量几何关系：`0x24 + 448*4 == 0x724`、`0x724 + 8*4 == 0x744`、Room 构造清零 `0x720` 字节覆盖两段槽表、八个 EL 从 `EntityList+0x20` 起、RTTI 结构尺寸 `0x14/0x10/0x18`、EL descriptor `0x10`、roster vector `0x0C`。

这些检查使“读错偏移/漏项/顺序漂移”在编译期失败，而不是等采集时才暴露。运行时 `Validate()` 复用同一组 `constexpr` 检查，供 world 准备阶段拒绝不成立的布局。

## 7. 构建与验证状态

- 生成器在临时目录运行一次：`rows=60/64 (legacy=41 world=19) fields=78 sources=15 classes=10`，产物齐全。
- 生成 wrapper 与上一次生成产物比较：既有 41 个入口的 244 行在去掉宏内新增的 `__pragma(warning(suppress:4100))` 行（及其注释）后逐行一致，wrapper 体语义未变；世界段为新增。
- 纯编译（Release Win32、`/W4`，均在临时目录，不属于测试）：
  - C++20：`world/model|identity|storage|writer|observe`、`binding/world_layout`、`hooks/registry|context|pointer_patch`、全部 `game/`、`ops/`、`bootstrap/mod_init` 无项目警告；
  - C++17：`binding/definitions.cpp` 与生成的 `hooks_generated.cpp` 编译通过。生成的 `super` 跳板只做一次 `jmp`、形参不在本体内引用，宏内用 `__pragma(warning(suppress:4100))` 精确抑制这一处；项目文件零警告，剩余警告只来自 SDK 生成头自身（既有）。
- **DLL 整体链接仍未完成**：既有 `libzhl/ASMPatcher.hpp:302` 的 `friend static` 在当前 MSVC 的 C++20 下仍是 `error C2216`（`hooks/odd_patch.cpp`、`host_patch.cpp`）。本轮另确认这两个编译单元**不能**简单整体降级到 C++17：`host_patch.cpp` 经 `core/participant.h` 需要 C++20（`std::span`、默认比较运算符）。因此该修复需要显式的构建/包含拆分，属于“必要纯编译链接”推进项；本轮没有改构建选项或 SDK。
- CMake 侧的改动（生成命令新增 3 个输出与 `binding/world_layout.json` 依赖、新增两个编译单元）只做静态复核：本次会话的文件写入被限制在工作区之外唯一可写的临时目录，而 MSBuild 拒绝在临时目录建立构建树（`MSB8029` 使构建立即失败），因此没能执行 CMake 端到端构建。默认构建目录下的 CMake 自定义命令、生成输入输出与 `world_layout.cpp`/`observe.cpp` 的编译参数按源码逐项核对过。
- 生成器两次运行输出逐字节一致（9 个产物），说明生成结果不含时间戳/顺序随机性。
- 按现行规范，本次没有运行测试、没有启动游戏；2,000 次稳定段与双进程比较仍为 `NOT_RUN`。

## 8. 配置与启用状态

- 新增能力组 `Group::World`：binding 不符时只关闭该组，并写 `capability.world=0` 与原因。
- **world 观察入口当前不安装**：`WorldObservationArmed()` 固定返回 `false`（主线程桥、录制命令与 `world_record.enabled` 尚未实现）。延后安装不等于未校验：`PrepareBindings` 仍对 19 项执行定义查找与地址比对，失败会写 `binding_error` 并停用该组。
- 桥接项接入后只需让 `WorldObservationArmed()` 读取配置；Hook 一经安装即常驻到进程结束，不做字节恢复。

## 9. 给后续推进项的接口约定

1. 主线程桥从 `cs::world::layout` 读取字段/来源/类别/常量与 `ObservationSpec`，不要再写死偏移；`ManifestFields/Sources/Classes` 已是 writer 需要的形状。
2. `ManifestInput.hook_manifest` 使用 `layout::HookManifestReference()`；`observations` 用 `layout::ObservationMask()` 填充（未安装的入口为 false）。
3. world 观察只在主线程写身份表；`observe` 的故障位由主线程在下一个收束点读取，并按迁移设计停用录制或记录明确 gap。
4. 若需要“原生 body 到达”证据，应新增内层行而不是改写现有行的 `reach` 文本；新增行必须重新核算容量并让生成器/注册器显式报告。
5. 布局清单新增字段/来源/类别时，只需改 `world_layout.json`；生成期与编译期校验会拒绝与 profile 或几何关系不一致的改动。

## 10. 保留的 UNKNOWN

- 19 项在当前 Hook 链上的实际安装、命中与原生 body 到达（未运行）。
- 同址多定义（GridFactory 全局别名）在真实链上的注册行为；本轮只保证不使用错误别名。
- `removed_by_factory` 初值、Player/Tear/Projectile 额外 State/Timeout、Grid `+0x3C` 虚调之后的最终值。
- 采样期各来源 descriptor 的实际稳定性与 448 槽的真实占用（未运行）。
- world 组启用后的 detour 开销、Hook 链相对顺序与异常传播（未运行）。
