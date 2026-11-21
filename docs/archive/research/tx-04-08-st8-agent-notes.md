# TX-04-08 ST8 静态逆向笔记

日期：2026-10-26  
范围：REPENTOGON Hook 定义共享与优先级、当前 Hook 清单与容量、32 位地址空间预算、退出收口窗口、构建入口。  
本笔记只作静态阅读；没有运行游戏、测试、构建、生成器、格式/语法校验、哈希或性能探针，也没有写入 IDB。可复查证据索引见 [`artifacts/repentogon/tx-04-08-static/st8-agent/st8-source-evidence.md`](../../../artifacts/repentogon/tx-04-08-static/st8-agent/st8-source-evidence.md)。

## 结论与证据等级

- **CURRENT_SOURCE**：`mod/binding/hooks.json` 有 41 行 `hooks` 条目，其中 `player_update` 与 `player_native` 都指向 RVA `0x37BD40`，所以是 40 个 distinct RVA。另有 12 个 `byte_patches`、8 个 `pointer_hooks`、2 个 `native_dependencies`；三者不是额外的 Hook 行。
- **CURRENT_SOURCE**：本工程静态初始化先把 Hook 记录放入容量 64 的本地表；ModInit 按能力组创建 REPENTOGON `FunctionHook` 对象；加载器等所有 `zhl*.dll` 初始化后调用一次 `ZHL::Init()`，再由 HookSystem 按优先级安装。`registered=true` 只发生在 Hook 对象构造之后，不能作为目标函数已打补丁或已运行的证据。
- **CURRENT_SOURCE**：REPENTOGON SDK 为同一 GridFactory 字节特征生成两个不同 `FunctionDefinition`：`Isaac::CreateGridEntity(int,unsigned)` 的显式参数来自 ECX/EDX；全局 `CreateGridEntity(GridEntityType,unsigned)` 的参数来自栈/ECX。二者的特征串相同，不能仅凭签名把它们当成两份独立、ABI 等价的 Hook 入口。设计应使用已验证的 `Isaac::CreateGridEntity` 定义并避免重复拥有同一地址。
- **DESIGN**：新录制设计的 256 MiB、64 MiB、64 项、两个 1 MiB I/O 缓冲和 128 MiB 解码行是方案上限，不是当前实现分配量。老 `cs-t0` `Shared` 的 136,316,688 bytes 只可用于参照。按主审提供的参照式 `旧 Shared + 64 MiB queue + 两个 1 MiB I/O 缓冲` 为 205,522,704 bytes，距 256 MiB 尚有 62,912,752 bytes；该差额不含新 identity/radix、线程 scratch、管理结构、新字段或对齐开销，因此不能称为新模型总量。
- **STATIC_CONFIRMED**（IDA 静态证据）：J273 的 `Isaac::Shutdown` 位于 VA `0x982C90` / RVA `0x582C90`，由 main 的 `0x90AC60` 在地址 `0x90AFCE` 调用。调用发生在 Manager Update/Render 主循环结束之后；Shutdown 后执行清理并返回，没有后续 Manager 主循环。当前 Hook 清单没有 Shutdown。
- **UNKNOWN**：运行中有多少 Hook 通过定义校验、实际被 ZHL 安装、能够命中或覆盖目标；新世界布局的真实分配与峰值；本机连续可用 x86 地址空间；退出时是否能在已知窗口内完成后台 drain/flush。不能从静态清单、`CanHook` 或 LAA 位推断这些运行事实。

## 1. GridFactory 定义、共享地址与优先级链

REPENTOGON 生成源 `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/IsaacRepentance.cpp` 中：

| SDK 定义 | C++ 原型 / 参数描述 | 特征与 Hook 能力 |
| --- | --- | --- |
| `Isaac::CreateGridEntity`（33039–33043） | `GridEntity * (*)(int,unsigned int)`；`Type` 在 ECX，`Index` 在 EDX | 与下一行使用相同 signature pattern；`canHook=true` |
| `CreateGridEntity`（42228–42232） | `GridEntity * (*)(GridEntityType,unsigned int)`；`type` 在栈，`seed` 在 ECX | 同一 signature pattern；`canHook=true` |

REPENTOGON 的 `Definition::Find` 根据 name 与 `type_info` 生成的 unique name 查询注册表；`Definition::Add` 将定义加入列表并按 unique name 放入映射，没有按目标地址合并定义。`HookSystem::Install` 也按 Hook 所保存的名称查找定义。因而“同地址”不会自动让两份定义变成同一个 ABI；如果对两个别名各注册一份 Hook，地址 detour 机制可能形成包装链，但每层的参数恢复和 `super` 调用都必须匹配各自定义。当前静态证据不支持把它们当成两个安全、互换的 Hook owner。

计划已指出 GridFactory 是官方 `__fastcall` 入口，应优先复用正确的 REPENTOGON 定义。工程的自有定义路径 `mod/src/binding/definitions.cpp:79–91` 给每个参数构造 `{NoRegister{},1}`，适合普通栈参数模型；它不能表达上述 ECX/EDX 参数位置。故 GridFactory 应绑定到已经核对的 `Isaac::CreateGridEntity` SDK 定义，不应为同一入口再造一个 generic 自有定义，也不应把两个别名都纳入 Hook 清单。

HookSystem 的静态安装链为：

1. REPENTOGON loader 遍历并加载 `zhl*.dll`，调用各 DLL 的 `ModInit` / `InitRepentogon`，全部完成后再调用 `ZHL::Init()`（`loader/loader.cpp:130–190`）。
2. REPENTOGON Hook 列表是 `std::multimap<int, FunctionHook_private*, std::greater<int>>`，初始化按该顺序调用 `Install()`（`HookSystem.cpp:397–425`）。SDK 注释说明低数值先进入，高数值最后、紧邻原函数（`repentogon/dllmain.cpp:69–76`）。源码的降序安装次序与逐层 detour 一致：高优先级层先装，低优先级层随后包装它；调用时低优先级 wrapper 先进入，再经 `super` 进入高优先级 wrapper 与原函数。相同 priority 的跨 DLL 次序不由此源码保证。
3. Detour 保存其安装时目标入口的原始字节，建立 trampoline，再把入口改成跳转（`libzhl/mologie_detours.h:394–431`）。后续同地址 detour 会看到先前的跳转并将其接入新的 trampoline；这解释了共享地址可形成链的机制，不证明任意 ABI/优先级组合都正确。
4. `FunctionHook_private::Install` 查找具体定义；若定义不存在就失败；若 `CanHook()==false` 则直接返回成功而不安装（`HookSystem.cpp:558–575`）。所以即便将来从 ZHL 接口得到成功返回，也必须区分“未报失败”与“该入口确实被 detour”。

## 2. 当前清单数量与容量

**CURRENT_SOURCE**：`mod/binding/hooks.json` 的 `hooks` 数组现有 41 行。按行统计为 41 个 Hook candidate；`player_update` 和 `player_native` 两行共用 `0x37BD40`，因此 40 个 distinct RVA。两行优先级分别为 `-10000` 和 `10000`，是同一目标的双层 hook candidate，不能把 distinct RVA 数误当成 row 数。

同一文件另外列有 12 个 byte patch、8 个 pointer hook 名称和 2 个 native dependency pattern。它们的部署/安装路径不同：pointer slots 是 `pointer_patch.cpp` 的固定 `slots[8]`；native dependencies 是 binding 准备阶段的前置特征，不占 `Hook` pending 行。不要把它们加进 41 行的 Hook 计数，也不要把旧文档里的“70 槽”当成本方案的容量事实。

`mod/src/hooks/registry.cpp:5–15` 的 `pending[64]` 是单独的固定容量；`AddPending` 仅在 `used < 64` 时写入，超量没有错误返回或诊断。以当前清单逐行生成一份 Hook 记录为源码路径，41 行在名义上距 64 上限还差 23 行；这只是源清单对容量的静态比较。`cs_hook.h:28–35` 的静态构造只把记录送入该本地表。`RegisterHooks()` 再按能力组筛选，构造 `FunctionHook`，随后写 `h.registered=true` 并记录 `hook_registered`（`registry.cpp:17–27`）。这个布尔值的含义是已向 ZHL 注册对象，不代表目标已实际安装或被调用。实际安装还要经过 SDK 的后续 `ZHL::Init` / `Install` 链。

规划中 19 个新增观察语义若机械地各占一行，则估算 `41+19=60`，只剩 4 个 pending 槽；若 Shutdown 另增一行则 61、只剩 3 个。此数只是候选容量演算，不能据此定最终容量：每个语义可能拆成多行，Shutdown 也未列入当前 41 行；还须统计生命周期入口、各别名、所需 wrapper 以及任何额外 Hook。实际注册数也会受能力组和 binding 校验影响，安装数更未知。

## 3. 32 位预算核算与数据类型边界

下列为 **DESIGN** 上限和算术，不是新 Recorder 的实测分配。

| 设计项 | 逐项换算 | 解释 |
| --- | ---: | --- |
| 每批对象字段格 | `16,384 × 52 = 851,968` 格 | 每个对象最多 52 字段；这是格数，不是字节数 |
| 三个 raw batch 槽 | `851,968 × 3 = 2,555,904` 格 | 每槽仍有对象/成员/控制信息；新布局各结构的实际 `sizeof` 未知 |
| raw copied queue | `64 MiB = 67,108,864 bytes`，最多 64 项 | 两条件同时满足；**已包括** reader reservation、队列内、writer in-flight 和 events |
| JSONL I/O buffers | `2 × 1 MiB = 2,097,152 bytes` | captures 与 events 各一份 |
| world 自有动态总预算 | `256 MiB = 268,435,456 bytes` | 方案说明包含 batch、queue、identity/radix、control、I/O、管理开销 |
| 单条解码行 | `128 MiB = 134,217,728 bytes` | **DESIGN 离线**：`world-replay` / `compare` 离线工具的单行解码上限，不是 DLL 预留内存或 256 MiB world 预算的一部分 |

扣除队列和两个 I/O buffer 后，名义剩余为 `268,435,456 - 67,108,864 - 2,097,152 = 199,229,440 bytes`（190 MiB），尚需容纳 batches、identity/radix、control、管理与其他世界自有动态存储。不能把这 190 MiB 全部视为可分配的 batch 空间。

老原型 `cs-t0/native/include/isaac_cs_t0/world_record.hpp` 的现有 `static_assert` 给出 `sizeof(Field)=48`、`sizeof(Object)=2592`、`sizeof(Batch)=43,516,368`、`sizeof(Shared)=136,316,688`（该文件 29、64–66 行）；本次只阅读了断言源码，没有运行断言。按主审提供的静态预算参照作算术：

```text
136,316,688  old Shared (完整 3 批及其控制/事件区)
 67,108,864  新方案 64 MiB copied queue 上限
  2,097,152  新方案两个 1 MiB I/O buffers
-----------
205,522,704  对照合计
268,435,456  新方案 256 MiB 上限
-205,522,704
 62,912,752  算术余量
```

这个对照把旧 `Shared` 与新方案其他额度拼在一起，仅作预算压力参照；它**不是**新布局估算，也遗漏了新 identity/radix、线程 scratch、管理开销、新字段以及对齐等，所以 62,912,752 bytes 不是已证明的安全余量。旧工程外部进程的额外 256 MiB queue 已明确禁止复制。

工程 CMake 明确要求 MSVC Win32 且 `CMAKE_SIZEOF_VOID_P=4`（`mod/CMakeLists.txt:3–5`）；因此该 DLL 的指针和 `size_t` 是 4 字节模型。当前 `Ring` 使用 `std::atomic<size_t>` 作为序号与位置（`runtime.h:69–117`），Hook 优先级字段是 `int`（`runtime.h:119–134`），ZHL `ArgData::_size` 类型为 `size_t` 且以 4 字节栈对齐单位表示参数长度（`HookSystem_private.h:52–63`）。这些是当前源码类型，不代表新 recorder 所有新计数器/容器均已实现。老记录原型显式使用 `uint64_t` 标识与序号；新文件格式也规定 u64 的文本表示，但没有可计算的新 C++ 布局。

IDA 只读输入 PE 摘要见主审工件 `artifacts/repentogon/tx-04-08-static/st6-st8-review/main-pe-context.json`：Machine `0x14c`、PE32 Magic `0x10b`、Characteristics `0x122`、LAA 为 true、ImageBase `0x400000`、SizeOfImage `0x85f000`。这描述的是该 unpacked 输入映像头，不证明部署文件相同，也不表示进程有连续可用的任意地址范围。**不能**据此宣称 2 GiB 固定上限或 LAA 后一定可用 4 GiB；进程现有映射、地址碎片和大块分配成败仍是 **UNKNOWN**。

当前 trace 还有一个独立的 4096 项固定 ring，源注释约 14 MiB 常驻（`mod/src/ops/trace.cpp:8–18`）。它是现有 C/S DLL 的额外进程压力，不等于新方案声明的 world 自有 256 MiB，也没有静态证据证明已把这项纳入该方案总预算。

## 4. Shutdown 收口窗口

**STATIC_CONFIRMED**（IDA 静态证据，主审原始 JSON：`artifacts/repentogon/tx-04-08-static/st6-st8-review/main-shutdown.json` 与 `main-shutdown-caller.json`）：`Isaac::Shutdown` 位于 VA `0x982C90` / RVA `0x582C90`。main 在 `0x90AC60` 执行 Manager Update/Render 循环，循环后在 `0x90AFCE` 调用 Shutdown，然后做后续清理（包括 `sub_A2AD80`、`CoUninitialize`）并返回。Shutdown body 包含 `Game::Exit(1)`、Game/Manager 析构/释放及 root 清零。Shutdown 不在现有 `hooks.json` 中。

这与清单里的 `game_exit`（`Game::Exit`，RVA `0x2F6270`）不是同一个入口语义：它被当前代码用于 run teardown，`RunExit()` 停 session、清 pending run mutations 并关闭 run persistence window（`mod/src/ops/command.cpp:359–368`）。REPENTOGON 的 ImGui `Isaac::Shutdown` wrapper 在进入 `super` 前只设置 `shutdownInitiated`（`ImGui.cpp:777–780`）；该处不是 `Game::Exit` wrapper。这些事实不能证明 `Game::Exit` 是进程退出通知。

方案三种时间承诺必须分开解释：

- **主线程等待 0**（DESIGN）：主线程每轮只作有界推进，不等后台。
- **正常关闭后台 10 秒**（DESIGN）：可以用于此前显式 world stop 的后台收口，但还需有仍存活的进程/线程与状态轮询/完成路径。
- **正常进程退出先 drain**（DESIGN 要求）：IDA 看到的 main 在 Manager 循环退出后马上进入 Shutdown，之后没有下一次 Manager 更新。当前没有已证实的 Shutdown 后 10 秒窗口，也没有现有 Shutdown Hook；因此不能声称终端 Shutdown 通知会自动提供十秒 drain。

与 E2E S11“最后一次正常 world close、游戏退出”兼容的保守解释是：正常路径在主循环还活着时显式 stop，并等待后台在 10 秒设计期限内到 Closed；终端 Shutdown 只关闭新发布并做非阻塞收束，若到进程退出仍未 drain，则保留 incomplete。若要求所有直接退出路径都保证 10 秒 drain，必须另行明确主循环结束前的关闭窗口/策略；现有静态证据无法确认方案已解决这一点。`DllMain` 也不能承载 join、文件写入或释放仍被后台引用的上下文；现有 trace 的 `FinishTraceOnProcessExit` 只在 process detach 时不等待 worker 并补写已有 trace（`trace.cpp:214–232`），不是 world recorder drain 的证明。

## 5. 构建入口与测试边界

**CURRENT_SOURCE**：`scripts/build.ps1`（本次未执行）可选构建 SDK，然后执行 CMake configure/build、Cargo `build`、复制产物（1–17 行）。脚本没有 `ctest`、E2E harness 命令或游戏启动命令。因此它是构建入口，不是测试入口；仅从源代码静态判断，执行它不会运行测试。

要同时说明的是，CMake build 有一个自定义生成步骤：`mod/CMakeLists.txt:23–26` 在生成依赖触发时运行 `mod/tools/gen_binding.py` 产出生成代码与 manifest。因此“构建入口不跑测试”成立，但“构建入口不运行生成器”不成立。本次没有执行 build 或 generator。

## 6. 主审应复核的关键 UNKNOWN

1. 不以 `registered`、`CanHook=true` 或 41 个 source rows 声称 Hook 已安装；实际 ZHL 返回、目标字节与运行命中仍须未来授权的静态/实机证据补足。
2. GridFactory 同址双定义应只选 ABI 已核对的 SDK 定义，并确认最终 Hook 名称/type 查找得到的正是它；不要让自有 `NoRegister` 定义或第二别名再拥有同一 target。
3. 最终 Hook 容量必须从落地的逐项观察语义和实际要注册的入口重新计数。`41+19=60` 只是估算；若另加 Shutdown 至少再加 1，且仍遗漏细分入口可能性。
4. 新 world C++ 对象与临时缓冲布局尚未给出，不能用旧 `Shared` 的 static_assert 代替新 `sizeof` 总量；DLL 新实现的分配总量/峰值、分配失败与 x86 碎片仍未知。128 MiB 是离线 `world-replay` / `compare` 解码行的设计上限，不是 DLL 预留额度；离线工具实际峰值另属未知。
5. 显式 stop 的正常 E2E drain 与 main 循环后 terminal Shutdown 的关闭路径应分开验收；是否新增一个退出前窗口属于设计决策，不能由静态 Shutdown 自行推定。
