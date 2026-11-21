# tx-03-04 Native Hook 静态逆向记录

日期：2026-10-21（Asia/Shanghai）。状态：固定点位与已列 ABI 的静态核对完成；未完成的动态模块/语义范围单列为 UNKNOWN。

## 范围与限制

本记录核对 `cs-t0/native` 所有安装家族与直接原生调用依赖，重点为 `target_image.hpp`、`hooks.cpp/.hpp`、`loop_runtime.cpp`、`input_runtime.cpp`、`boundary_runtime.cpp`、`raw_apply_runtime.cpp`、`active_targets.cpp`、`runtime.cpp`、`offline_attestation_runtime.cpp`、`coop_passive_runtime.cpp`、`coop_active_runtime.cpp`、`world_record.cpp` 及 `owned_virtual_device.cpp`。仅阅读源、静态解析 PE 与 IDA 静态反汇编；未运行构建、测试、probe、smoke、脚本验收或运行时实验，未注入/操控游戏或调试器，也未修改 IDA 数据库。不读取或校验哈希。

## IDB 与目标 PE 对应性

**直接观察：** IDA MCP 当前数据库为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`，输入文件为同目录 `isaac-ng.exe`，模块 `isaac-ng.exe`，IDA imagebase `0x00400000`。文件时间 `2026-10-21 13:31:49.978814`，文件大小 `9,176,064` 字节。PE32/i386，`TimeDateStamp=0x69E6E3A7`，首选 ImageBase `0x00400000`，`SizeOfImage=0x008FE000`，入口 RVA `0x006EFC46`，共 5 节。

节表：`.text` RVA `0x1000`, virtual `0x716134`, raw `0x400/0x716200`; `.rdata` RVA `0x718000`, virtual `0xDF948`, raw `0x716600/0xDFA00`; `.data` RVA `0x7F8000`, virtual `0xA4AA4`, raw `0x7F6000/0x69E00`; `.rsrc` RVA `0x89D000`, virtual `0x3B20`, raw `0x85FE00/0x3C00`; `.reloc` RVA `0x8A1000`, virtual `0x5C93C`, raw `0x863A00/0x5CA00`。这些字段与 `native/include/isaac_cs_t0/target_image.hpp` 的 ApprovedUnpacked 布局一致。历史只读 live 记录载明加载基址 `0x00DB0000`，与首选基址不同；加载基址不用于 RVA 比较。IDB 输入路径与 PE 路径一致，且时间、PE 头和节表吻合；据此判断为同一文件来源（未以哈希证明）。

此前出现的 `ImageBase=0x1000`、`SizeOfImage=0x718000` 是读取 PE32 OptionalHeader 错偏移导致的初始误读，已撤回；应使用 ImageBase +28、EntryPoint +16、SizeOfImage +56。

## 已完成静态核对

| 家族 | 点位/目标 | 静态结果 |
| --- | --- | --- |
| 主循环 `loop_runtime.cpp` | 27 个 `E8 rel32` call（`kCallRvas/kTargetRvas/kCallBytes`） | 27/27 起点为指令边界；长度均为 5；源字节、IDA 指令与 rel32 实际目标一致。相邻 `0x5312EC` 与 `0x5312F1` 正好相接。 |
| 主循环 IAT | PeekMessageW `0x718714`、TranslateMessage `0x718650`、DispatchMessageW `0x7185B8` | 文件导入目录 RVA `0x7F3088`，USER32 的 OFT 非零（`0x7F3860`），FirstThunk `0x718594`；三槽均处在对应导入表。故当前文件不支持 “OFT=0 导致名称回退失败” 的解释。IDA 对 IAT 地址显示 `FF FF FF FF` 与磁盘 FirstThunk 名称 RVA不同；这属于 IDA 导入项呈现/自动导入状态，不能仅凭此判为运行时槽内容错误。 |
| 输入 vtable | Keyboard `0x7A1678` / COL `0x7B191C`；Gamepad `0x782C80` / COL `0x7B0460` | 两个 `vtable[-1]` 均指向声明 COL；查询槽目标均为 `0x61E8D0/0x61E970/0x61EA20`；poll 槽目标分别 `0x66C650`、`0x622940`。 |
| active targets | 6 个 consumer call、7 个 pointer slot | 6 个 call 均是长度 5 的 `E8 rel32`，字节和目标一致；6 个 query pointer 与两组 query vtable 目标一致；PlayerUpdate pointer `0x76BDDC -> 0x382AF0`。 |
| coop active | 12 个 relative call（active consumer 6 + Join/Create 2 + PlayerOnly 1 + JoinMode 3） | 12/12 均为完整 5-byte call，字节及目标与源码常量一致。 |
| 可选 world observer | 19 个 EntryPatch | 19/19 声明字节与 IDA 入口一致；入口各解码出的完整指令长度之和等于 patch 覆盖长度。长度为 `[7,6,7,5,5,5,6,5,5,6,6,6,5,5,6,5,5,6,6]`，没有截断指令。 |

可复核的全地址摘要见 `artifacts/cs-t0/tx-03-04-static/hook-observations.json`。具体原始 call 常量分别位于 `loop_runtime.cpp:45-108`、`active_targets.cpp:10-39`；world 站点/expected bytes 位于 `world_record.cpp` 的安装准备函数附近（`Entries()` 上游，安装入口约 1840 行）。

## ABI 与 wrapper 观察

### Clock

**直接观察：** IDA RVA `0x6266A0`（VA `0xA266A0`）正常分支在 `0xA26705` 执行 `movaps xmm0,xmm1` 后 `retn`；失败分支在 `0xA266BE` 执行 `xorps xmm0,xmm0` 后 `retn`。主循环 `main` RVA `0x531231` 的 call 下一条 `movsd [ebp+...],xmm0`，随后以 `subsd` 计算时间差，仅消费 XMM0 的低 64 位。源码 `ClockFn=__m128d(__cdecl*)()` 与 `LoopClockHook` 的 `__m128d __cdecl` 返回均走 XMM0；其低双精度值契合原调用者。**推断/限制：** 原函数反汇编标记为 `__m128`，但调用点只使用低 64 位；没有证据要求上 64 位具有语义。wrapper 将原始向量结果原样返回。

### WorldEntry0..18

**直接观察：** 每个 wrapper 先 `pushfd; pushad`，保存 XMM0–7/FPU 状态到对齐区，构造含原 return address 与原始参数槽的 shadow stack，再调用 trampoline 一次；结束后再次保存原函数结果寄存器/标志/FPU 状态，调用观测函数后恢复并 `ret N`。`kArgumentWords` 为 `[2,4,1,0,0,0,0,1,1,2,1,4,6,0,1,5,1,3,8]`，对应 `ret N` `[8,16,4,0,0,0,0,4,4,8,4,16,24,0,4,20,4,12,32]`。各 `WorldEnter` 读取栈参数从原栈 return-address 后开始，wrapper 的清栈字节数为参数总字节数。Pushad 视图使用 `[EDI, ESI, EBP, savedESP, EBX, EDX, ECX, EAX, EFLAGS, return]`；this 指针来自原 ECX；返回观察取 EAX。Hook13 特别使用 EDX 作为 userpurge 输入，适配源码寄存器提取。**结果：** 参数词数、转发槽数和 `ret N` 一一对应；未发现不匹配。

**有限 UNKNOWN：** world18 的原函数 RVA 0x2FE410 有直接观察到的 ret 0x20，且和 wrapper 八个栈词的清栈一致；其 call site RVA 0x28B3D 是 spawn 调用。现有 caller 范围没有把该处完整八个 outgoing word 的语义逐项映射清楚，因此参数的高层语义仍 UNKNOWN；这不构成已观察到的点位/清栈不匹配。

### Join/Create

**直接观察：** 原 Join predicate RVA 0x5BA980 读取全局状态，true 分支执行 mov al,1; retn，false 分支 xor al,al; retn；三个 caller 0x5BAE7E、0x5BB081、0x5BB288 都紧接 test al,al。因而 wrapper 的 bool 输出确实在 AL/EAX，原谓词不接收栈参数；在这条短函数内除 EAX/AL 与条件码外没有其他被写寄存器。源 wrapper 保存 flags、通用寄存器和 XMM0–7，调用选择逻辑后只替换保存的 EAX，再恢复其余状态，满足 caller 实际使用方式。

Join caller 上下文由原始反汇编范围直接支持：前两处使用 EDI manager、ESI key，第三处 selector 为 EBX、key 为 ECX，且 EBX 在该路径由 EDI+0x14 得到；这些值与三个 naked wrapper 的寄存器读取一致。相应完整原始地址/指令见 root-ida-join-context.json。

**Create 直接观察：** call site RVA 0x5BB415 的五字节指令为 E8 B6 E8 FF FF，目标 RVA 0x5B9CD0。caller 先 push EDX（来自 selector 项数组），再令 ECX=Game+0x1BAA8，指向内嵌的 manager。callee 以 ECX 为 this，取一个栈参数，并以 retn 4 返回；与 CreateHook 的 thiscall 转发、单栈参数及允许路径 tail-jump 相符。拒绝路径从调用点栈中移除 return address 与一个四字节参数，然后跳到 RVA 0x5BB561；原生续段在 [EBX] 写 -1、推进 selector 项并回到循环。callee ret 行和上下文见 root-ida-raw.json 与 root-ida-join-context.json。

### Input pointer patches 与 owned virtual device

**Input 直接观察：** input_runtime 的 11 个 pointer patch 按 3 个 manager vtable 槽（+0x40/+0x44/+0x48）、keyboard/gamepad 各 3 个 logical query 槽（+0x30/+0x34/+0x38）及两处 setter 槽（+0x4C）布置。IDA 的 manager 按键/触发目标 RVA 0x620710/0x6207B0 都从 ECX 取 manager、读取三个栈参数并以 retn 0Ch 清栈；manager scalar RVA 0x620850 同样 retn 0Ch，返回前 fld [var_4]，结果在 x87 ST(0)。logical pressed/triggered RVA 0x61E8D0/0x61E970 使用 ECX this、一个栈 action 参数并 retn 4；logical scalar RVA 0x61EA20 正常 fld [var_4]、默认 fldz 后 retn 4。logical setter RVA 0x61ED50 由 ECX 接 this、两个栈参数 action/float，写入设备状态并 retn 8。源码的 __thiscall prototypes 与清栈/浮点返回相符。

Input vtable/COL 实际指针值、两个 logical query vtable、manager 三方法及 setter 的 IDA 原始范围收录于 root-ida-input-extra.json；静态点位摘要见 hook-observations.json。raw_apply_runtime 的 12 个 pointer patch 复用这些 11 个输入槽，并增加 PlayerUpdate 槽 RVA 0x76BDDC -> 函数 RVA 0x382AF0。

**Owned virtual device 直接观察：** 源码复制 38 槽基类 device vtable 并覆盖 slot 0/3/12/13/14/30；COL 来自 RVA 0x782754，vtable 来自 0x782758。Pressed/Triggered/Scalar 的 fastcall bridge 形参为 ECX=this、忽略 EDX、action 留在栈上，与原 logical query 的 thiscall 一栈参 ABI 对齐；Scalar 原函数在正常路径 fld 局部 float、默认路径 fldz，故 C++ float bridge 的 ST(0) 返回约定匹配。Poll 无显式参数；基础 query/poll 的清栈方式分别为 ret 4/ret。

原生构造与注册依赖的调用约定也已核对：allocator RVA 0x60F4C0 为单栈参数 cdecl、ret（调用者清栈）；device ctor 0x61E050 与 control ctor 0xCC10 是 ECX this、无显式栈参数、ret；Init RVA 0x61E210 使用 ECX this 加一个栈参数并 retn 4；Register/Remove pair RVA 0x620DF0/0x620EB0 使用 ECX manager 加 object/control 两个栈参数并 retn 8。源码 Allocate/Ctor/Init/PairOperation typedef 与这些原生栈约定相符。Control table 的源码回调中，slot 1–5 使用 ECX this 加忽略的 EDX dummy；slot 0 ControlDestroy 另有一个栈上 uint32 参数。此事务没有展开 native callers 对自定义 control vtable 各槽的逐项语义，故该自有表的调用端契约保持 UNKNOWN，不把源码签名当成 native caller 的直接证明。各函数入口、ret 与浮点返回实际行见 root-ida-abi.json、root-ida-input-extra.json。

### Hook 安装家族覆盖清单

下表为源中声明的安装数量，跨家族重复的相同 site/slot 只在反汇编证据中去重；已列静态点位均未发现 RVA、原指令/字节、rel32 目标或覆盖指令边界漂移。

| 家族 | 源声明的点位数量 | 结果 |
| --- | --- | --- |
| runtime.cpp | 10 relative、5 固定 pointer/IAT，另 3 个动态 Steam vtable 槽 | 固定点位已由共享/新增 IDA 原始行覆盖；外部动态槽 UNKNOWN。 |
| loop_runtime.cpp | 27 relative、5 固定 pointer/IAT | 27/27 E8、边界、字节、目标吻合；IAT3 的 PE 导入目录/OFT/FirstThunk 已核；剩余固定指针见静态摘要。 |
| input_runtime.cpp | 11 pointer | 11 槽目标与 manager/device vtable 槽布局、COL 及原生 ABI 一致。 |
| boundary_runtime.cpp | 12 relative、4 IAT | 12 个数组项已核；site 0x28B3D 调用 target 0x2FE410，是 spawn call site，不是 target RVA。Lua IAT callsite 位于原始导出。 |
| raw_apply_runtime.cpp | 6 relative、12 pointer | relative 与 active/loop 已核点重叠；pointer 集覆盖输入 11 槽与 PlayerUpdate，源码新增 expected_original 绑定核过。 |
| active_targets.cpp / active_runtime.cpp | 6 relative、7 pointer | 六个 call 与七个槽目标均与 IDA 一致。 |
| coop_active_runtime.cpp | 12 relative、7 pointer，另可带 world 19 EntryPatch | Join/Create caller ABI 已核；共用 active 6、Join/Create/PlayerOnly/JoinMode call 目标均一致。 |
| coop_passive_runtime.cpp | 8 additional relative、3 device relative，复用 active pointers | 新增 call 原始指令/目标见 root-ida-raw.json；device 目标 ABI 与 owned-device bridge 已核。 |
| offline_attestation_runtime.cpp | 4 IAT pointer，另 3 动态 Steam vtable 槽 | IAT 调用/导入名静态核对；Steam 外部动态目标 UNKNOWN。 |
| embedded world observer | 最多 17 relative、12 pointer、19 个 EntryPatch | 19/19 entry expected bytes 与完整指令覆盖长度匹配；调用/指针项并入各 family 去重覆盖。 |
| owned_virtual_device.cpp | 38 device vtable slots 中覆写 6 个，control vtable 6 个入口；若干原生调用依赖 | 构造、Init、pair Register/Remove、query、setter、Poll ABI 已核。它们不是宿主地址补丁，未混入 Hook 点数量。 |

静态范围仅能确认当前 IDB 指向的主模块及主 PE 中可见的代码/导入。运行时由外部 Steam 模块解析出的 3 个 vtable 槽地址、实际目标与目标函数 ABI 没有本次静态 IDB 证据，列为 **UNKNOWN**。

### 新脱壳 PE 准入源审阅

直接 PE 观察为 SizeOfImage 0x8FE000、五节；target_image.hpp 的 ApprovedUnpacked profile 与该布局相符，Canonical 则使用独立六节/0x92C000 元数据。此前 runtime/input/boundary/raw_apply 四处硬编码 0x92C000 会拒绝当前 ApprovedUnpacked；现源均改为先调用 target_image::ApprovedImageSize/IsApproved，并在 input/boundary/raw_apply 缓存已批准 PE 的真实 SizeOfImage，用它界定 ReturnRva/RVA 范围。静态阅读确认拒绝仍由原有 exact PE 元数据与节表批准控制，没有放宽到实验 profile；不改变 Hook RVA 或 expected bytes。此为源代码审阅结论，未执行编译或验收入口。

PointerPatch 新增 expected_original 的使用也已静态审阅：Prepare 在挂起线程前检查槽当前指针；Apply 在事务线程挂起期间同时要求槽仍等于捕获 original 且与 frozen expected_original 一致。input 的 11 槽和 raw_apply 的 12 槽均把目标 RVA 对应的原生函数作为 expected_original；restore 仍按捕获 original 回滚。

### tx-03-03 失败根因与剩余 UNKNOWN

`artifacts/cs-t0/tx-03-03-live-05/host/native-init-events-readonly.json` 只记录 CoopActiveInitialize 返回泛化 Error::Hook，没有 StartEmbeddedObserver 的阶段/索引/站点失败事件。PE 元数据记录通过，且静态核对没有发现本次固定 call/entry 地址漂移，也不支持本 PE 的 USER32 OFT=0 解释；因此 exact-site reject 不是已证明事实。失败阶段/站点仍为 **UNKNOWN**，可能位于 Preparation、Mapping、Image、Conflict 或其他前置环节。主线程增加的阶段/目标诊断及保留 StartEmbeddedObserver 原错误值已静态阅读，但没有运行。

剩余静态 UNKNOWN 为 world18 caller 的八个 outgoing word 高层语义映射、自有 control vtable 各槽的 native caller 语义，以及动态 Steam vtable 的实际模块目标/ABI（属于本次主模块 IDB 范围外）。除这些边界外，本次纳入的固定 Hook site、目标、expected bytes、指令边界/覆盖长度和已列调用约定没有剩余静态 UNKNOWN。所有已取得证据索引如下：

- hook-observations.json：旧 27-call、固定 IAT/vtable 与 world19 摘要。
- root-ida-raw.json：新增/历史 relative call 的原始 IDA 指令与 world19 callee 的 ret 行。
- root-ida-abi.json：共享目标函数入口、ret 原始行及类型提示。
- root-ida-join-context.json：Join/Create 原生调用点上下文、selector 和拒绝续段。
- root-ida-input-extra.json：输入 vtable/COL 指针、输入 ABI 和 owned Remove/Scalar 返回上下文。

本记录只反映离线静态阅读与主线程源 diff 审阅；没有运行任何测试、构建、probe、smoke、统一验收入口或运行时实验，未改 native 源与 IDA 数据库。
