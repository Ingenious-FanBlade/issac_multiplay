# tx-04-04 实现方案：开发包、工程骨架、Hook 框架、命令桥与 harness

> 2026-10-24 tx-04-04 实现修订：IDA 原始指令确认 Lua 捕获字段为 `Game+0x680D8`（即 `Console+0x104`），类型为 MSVC `std::string*`；此前记为 `0x68158` 的资料与实现有误。`Console::Print` 捕获时直接追加后返回，不写控制台历史，继续使用 Print。静态记录见 `artifacts/repentogon/tx-04-04-revision-static/`。

日期：2026-10-24。状态：审阅修订已实现并编译；第一次和 D16 允许的第二次 E2E 均在注入启动阶段超时失败。第二次新增诊断捕获 Use The Launcher! 错误框，未再重跑。根因静态确认为 REPENTOGON 的启动器环境变量检查；按 D17 补上标记后执行第三次 E2E（第 7 节第 4 步、第 8 节）。运行时能力尚未验证，见[实现与结果](tx-04-04-results.md)。依据：

- [REPENTOGON 适配重构设计](repentogon-refactor.md)（下称“设计”）；
- [v1.9.7.12 点位核对](../hooks/repentogon-binding.md) 及其 [JSON 清单](../hooks/repentogon-binding.json)（下称“binding”）；
- [现行项目规范](../refactor/project-conventions.md)。

本文属于 tx-04-04。按规范第 2 节，这次推进只在全部实现完成后执行一次 E2E（第 8 节），编写和实现期间不运行任何测试。文中标注 `ABI 待实现时确认` 的地方，在实现过程中用 IDA 静态确认，不靠试跑。

**审阅修订（2026-10-24）。** 唯一一次 E2E 失败后，按[实现与结果](tx-04-04-results.md)中“静态审阅”一节修订了下列各处，每处都标注“审阅修订”并保留原写法：
- 第 3 节：注释要求；
- 4.4、4.5：能力组与状态的关系、`log_level`；
- 4.6：退出事件；
- 4.7：输出方式；
- 第 5 节：异常语义；
- 第 6 节：界面轮询、测试 mod 的主菜单阶段；
- 第 7 节：存档对比范围、部署记录、注入路径与失败诊断、浮层关闭后的状态确认；
- 第 8 节：E7、失败情况与 artifact；
- 第 9 节：风险。

修订只依据代码、日志、文件时间戳和目录列表等静态材料，没有运行任何测试。

修订内容沿用 tx-04-04 编号实施（用户决定，设计 D16）。实施完成后执行 tx-04-04 的第二次 E2E，同样只执行一次；artifact 放在新目录 `artifacts/repentogon/tx-04-04-e2e-02/`。第一次 E2E 的 FAIL 和 artifact 原样保留。

## 1. 目标、范围与边界

**目标。** 在 REPENTOGON v1.1.3 + Repentance+ v1.9.7.12.J273 上做到以下几件事：

- `zhlIsaacCS.dll` 被 REPENTOGON 正常加载，完成能力组判定，注册全部首轮 Hook（只透传和计数）；
- 打通 `cs` 命令桥：控制台、Lua `Isaac.ExecuteCommand`、测试控制口三种来源；
- 交付 `isaac-cs-ui` 界面 mod；
- Rust harness 能自行注入启动游戏、使用第二个存档、收集 artifact。

**预期结果。** 一次 E2E 执行通过第 8 节的 E1–E7。

**本轮包含：**
1. 构建 REPENTOGON 1.1.3 开发包；
2. `mod/` 工程与构建脚本；
3. bootstrap；
4. binding 表与自有定义；
5. Hook 框架：`CS_HOOK` 注册表、Hook 清单、上下文栈、延后安装的指针补丁、计数器；
6. ops：配置、日志、trace、命令处理器、控制台 Hook、测试控制口；
7. 界面 mod 与测试 Lua mod；
8. harness。

**本轮不包含：**
- 会话真实逻辑：`Warmup/Active`、tick 判定、A 走统一队列（tx-04-05）；
- 网络、虚拟设备 B、Join 调用点补丁（tx-04-06）；
- world_record（tx-04-07）；
- Replica。

本轮 `cs host/stop` 只切换占位状态 `Idle↔Hosting`，用来验证命令队列，不涉及网络。

**本轮记录的用户决定**（同步写入设计第 1 节）：

| 编号 | 决定 |
| --- | --- |
| D11 | 本地 REPENTOGON 仓库已更新到最新；开发包取 tag `1.1.3`，与已安装版本一致 |
| D12 | 启动器自动更新已关闭（`repentogon_launcher.ini` 中 `Update = 0`） |
| D13 | E2E 由 harness 自行注入启动，不经过官方启动器 |
| D14 | E2E 使用第二个存档 |
| D15 | 原 tx-04-04 拆为 tx-04-04（本文）和 tx-04-05（循环、tick 判定与 A 的输入），之后的编号依次顺延 |

## 2. REPENTOGON 开发包

- **位置。** `F:\projects_isaac\REPENTOGON-1.1.3`。从本地仓库 `D:\projects\REPENTOGON` clone，检出 tag `1.1.3`，子模块从 GitHub 拉取（2026-10-24 已完成）。CMake 已按官方 CI 的方式配置到 `build\`（`-G "Visual Studio 17 2022" -A Win32`，C++17，`/MD`），尚未构建。
- **构建。** `cmake --build build --config Release --target libzhl`。这会先构建 `zhlparser`，并生成 `include\IsaacRepentance.h`、`libzhl\IsaacRepentance.cpp` 和 `libzhl\hooks.json`。不需要构建 `zhlREPENTOGON`。
- **我们用到的产物：**
  - `libzhl.lib`（导入库，路径以实际构建输出为准）；
  - 头文件：`include\`（`HookSystem.h`、`libzhl.h`、`IsaacRepentance.h`），`libzhl\`（`HookSystem_private.h`、`SigScan.h`、`ASMPatcher.hpp`），`libs\rapidjson\include`。
- **运行时。** 只使用游戏目录里已安装的 `Repentogon\libzhl.dll`（v1.1.3）。我们自己构建出的 `libzhl.dll` 不部署。
- **版本记录。** DLL 启动时把 `zhlREPENTOGON.dll` 导出的 `__REPENTOGON_VERSION`、exe 的 PE timestamp 和模块基址写进 trace。这些只用于记录，不作为启用条件；不计算或校验任何哈希。若以后 REPENTOGON 升级，需要重新构建开发包，并重新核对 binding。

## 3. 工程与构建

```text
mod/
  CMakeLists.txt              VS2022 v143、Win32、/MD、/EHs；REPENTOGON_ROOT 默认 F:/projects_isaac/REPENTOGON-1.1.3
  tools/gen_binding.py        由 binding JSON 和自有定义表生成 C++ 片段
  src/bootstrap/              dllmain.cpp、mod_init.cpp
  src/binding/                binding_v1_9_7_12.inc（生成）、definitions.cpp、offsets.h
  src/hooks/                  cs_hook.h、registry.cpp、manifest.cpp、context.cpp、pointer_patch.cpp、hooks_*.cpp
  src/ops/                    config.cpp、log.cpp、trace.cpp、command.cpp、console_bridge.cpp、control_port.cpp、status.cpp
  src/core/                   本轮只放会话占位状态；tx-04-05 再从原型移植
ui/isaac-cs-ui/               metadata.xml、main.lua
tests/e2e-mods/isaac-cs-test/ metadata.xml、main.lua（只在 E2E 期间部署）
tools/harness/                Rust crate，目标 i686-pc-windows-msvc
scripts/build.ps1             只构建，不带任何测试步骤
```

- **语言标准。** 我们的代码用 C++20（原型的 core 用到 `std::span`）。如果 `IsaacRepentance.h` 在 C++20 下编译有问题，凡是包含它的编译单元改用 C++17，core 保持 C++20 并且不包含 REPENTOGON 头文件。
- **构建输出。** 放在 `out\repentogon\`：`zhlIsaacCS.dll`、`.pdb`、`harness.exe`。
- **部署**由 harness 执行，并把每一步记进 artifact：
  - `zhlIsaacCS.dll` → `<游戏目录>\Repentogon\`；
  - 配置 → `<游戏目录>\Repentogon\isaac-cs\config.json`；
  - 界面 mod → `<游戏目录>\mods\isaac-cs-ui\`（REPENTOGON 会把 mod 目录重定向回游戏根目录的 `mods\`，见 `repentogon/Patches/ASMPatches.cpp:138`）；
  - 测试 mod → `<游戏目录>\mods\isaac-cs-test\`。
- **不带测试。** `scripts/build.ps1` 只运行 `cmake --build` 和 `cargo build --target i686-pc-windows-msvc`。原型的 `build.ps1` 默认运行 `cargo test`，按规范这算测试，新脚本不沿用。
- **注释（审阅修订）。** 按 AGENTS.md（2026-10-24 版），每个功能模块、核心函数和数据结构都要写中文注释；一行只写一条语句，不压缩成多条语句的长行。tx-04-04 首版代码注释偏少且是英文（见结果文档“静态审阅”），下一轮一并改写。

## 4. 接口约定

### 4.1 `CS_HOOK` 注册表

- **宏。** `CS_HOOK_METHOD(group, def_name, Class, Name, priority, type)` 和 `CS_HOOK_STATIC(...)` 生成的 wrapper 和 `super` 与 REPENTOGON 的 `_DEFINE_METHOD_HOOK1` 相同（`include/HookSystem.h:45-56`）：`super` 是一个裸函数，`jmp internalSuper`。区别在于不创建静态 `FunctionHook` 对象，而是在静态初始化时往本 DLL 的待注册表里加一条记录：`{group, def_name, typeid(auto (Class::*) type), hook, &internalSuper, priority}`。静态初始化阶段不调用 REPENTOGON 的任何接口。
- **注册。** ModInit 对判定通过的能力组逐条调用 `FunctionHook::Add`。`enabled=false` 时注册数为 0。
- **REPENTOGON 已有定义。** `def_name` 必须写成和 REPENTOGON 一致的 `"Class::Name"`，类型取自 `IsaacRepentance.h` 的声明，两者完全一致。
- **自有定义。** `def_name` 加 `IsaacCS::` 前缀，类型借用 `IsaacRepentance.h` 里的类（例如 `Room`、`Console`）或我们自己声明的空结构体。在 `FunctionHook::Add` 之前，先用地址构造函数创建 `FunctionDefinition`，internalName 传空串。

### 4.2 binding 表与定位

- **代码生成。** `tools/gen_binding.py` 读取 `docs/hooks/repentogon-binding.json` 和 4.3 的自有定义表，生成 `binding_v1_9_7_12.inc`，内容包括 RVA、期望指令、所属能力组、priority 和字段偏移常量。
- **自有定义的定位。** 采用“模块基址 + RVA，再核对期望指令”，这是对设计 4.4 的细化：exe 版本已经锁定，这样定位确定、简单。期望指令是函数入口前 16–32 字节，其中 rel32 和受重定位影响的字节写成通配。这些字节在实现时用 IDA 从 v1.9.7.12 IDB 导出，存进 JSON。不一致时，所属能力组判为失败。这只是检查指令形状，不是哈希。
- **REPENTOGON 定义的交叉核对。** 用 `FunctionDefinition::Find(name, type)->GetAddress()` 取出地址，和 binding 中的 RVA 比较（RVA = 地址 - 模块基址）。不一致记为该能力组失败。
- **字段偏移。** 集中写在 `offsets.h`，名称沿用 REPENTOGON `.zhl` 的字段名（binding 第 7 节）。

### 4.3 本轮的自有定义

| 名称 | RVA | 约定 | 参数 / 返回 | 依据 |
| --- | --- | --- | --- | --- |
| `IsaacCS::PostUpdateCaller` | `0x446C90` | cdecl | () → int | 调用点没有 push、不用 ECX；`retn` |
| `IsaacCS::NewRoomCaller` | `0x44B930` | cdecl | () → int | 同上 |
| `IsaacCS::ConsoleGate` | `0x287020` | thiscall | (Console*) → bool | `lea ecx,[edi+67FD4h]`；`test al,al` |
| `IsaacCS::RoomUpdate` | `0x3F7F60` | thiscall | (Room*) → void | `mov ecx,[edi+18300h]`；`retn` |
| `IsaacCS::EntityListUpdate` | `0x13F00` | thiscall | (EntityList*, 1 个栈参数) → ? | `lea ecx,[edi+1218h]`；`retn 4`；参数含义与返回值待实现时确认 |
| `IsaacCS::InputManagerUpdate` | `0x5EDCC0` | thiscall | (InputManager*) → int | 已反编译 |
| `IsaacCS::DeviceCommonUpdate` | `0x5EC900` | thiscall | (Device*) → bool | 已反编译；栈对齐型序言，MologieDetours 搬移前几条指令时需要注意（待实现时确认） |
| `IsaacCS::RoomLifecycleFSM` | `0x509100` | thiscall | (Game+0x1BA78 处的对象) → ? | `lea ecx,[edi+1BA78h]`；`retn`；返回值待实现时确认 |
| `IsaacCS::DoorNestedOwner` | `0x2FA0F0` | thiscall | (?, 2 个栈参数) | `retn 8`；调用方只有 `RoomTransition::ChangeRoom` 和 `Console::RunCommand`（解决 Q15）；参数含义待实现时确认 |
| `IsaacCS::OddHelper` | `0x2F9340` | cdecl | () → int | 不用 ECX；`retn` |
| `IsaacCS::JoinPass` | `0x591360` | thiscall | (PlayerManager*) → ? | `lea ecx,[edi+1BAA8h]`；`retn` |
| `IsaacCS::ConsumerMove/Bomb/CardPill/Shoot/Drop` | `0x372540/0x37A210/0x39BDD0/0x373990/0x39C300` | thiscall | (Entity_Player*) → void | `mov ecx,edi`，无 push；`retn` |

ArgData 和 flags（THISCALL、VOID）的写法，照抄生成的 `IsaacRepentance.cpp` 中同类定义（thiscall、无参或单个栈参数）。ArgData 只计显式参数，不计隐式 ECX。地址定义必须保留有效 outfunc 存储：`FunctionDefinition::Load()` 即使地址已存在也会写入它。

实现时补齐的 `STATIC_CONFIRMED`：`EntityListUpdate` 为 thiscall(bool partial) → int（`retn 4`，partial 控制部分实体更新）；`RoomLifecycleFSM` 返回 EAX 指针值，以 uintptr_t 透传；`DoorNestedOwner` 的 this 为 Game，两个 int 栈参数、返回 int；`JoinPass` 返回 unsigned int。设备公共更新首 6 字节为 `push ebx; mov ebx,esp; sub esp,8`，是无相对寻址的完整指令，可以由 MologieDetours 拷贝到 trampoline。原始入口与类型记录在 `mod/binding/v1_9_7_12-static.json`。

tag 1.1.3 的 `FunctionHook::Add` 实际为 private，实现改用公开的 `FunctionHook` 构造函数在 ModInit 内注册，不在静态初始化时调用 REPENTOGON。SDK 生成头在 C++20 下产生 C4596，包含 SDK 的两个编译单元按第 3 节改为 C++17。SDK / 项目使用 MSVC 19.44.35217、v143、Win32、/MD；未部署自建 libzhl.dll。

### 4.4 能力组

沿用设计 4.3，本轮的具体处理如下：

| 组 | 本轮内容 | 本轮结果 |
| --- | --- | --- |
| `loop` | binding 里所有 loop 入口 Hook（REPENTOGON 定义 + 自有定义） | 注册 |
| `input` | InputManager 更新、设备公共更新，以及 Keyboard/Gamepad 第 8/9/10/35 槽的指针补丁 | 注册，并延后安装指针补丁 |
| `player` | `Entity_Player::Update` 和 6 个 consumer | 注册 |
| `join` | 只含 Join pass 的入口 Hook；调用点补丁在 tx-04-06 | 注册 |
| `control` | `Console::RunCommand` | 注册，这一项会实际处理 `cs` 命令 |
| `device` | 自有虚拟设备的原生依赖 | 只解析和记录，不注册 |

组内全有或全无；判定结果写入 trace，`cs status` 中也能看到。

任一组失败时，占位状态都是 `Disabled`，不接受 `cs host`。这与设计 4.3 一致：device 组失败就没有 B，也就没有会话。审阅 A6 核对后不需要改代码（审阅修订）。

### 4.5 配置（`Repentogon\isaac-cs\config.json`）

```json
{ "schema": 1, "enabled": true, "log_level": "info", "trace_dir": "isaac-cs/traces",
  "control_port": { "enabled": false, "port": 27091 }, "host": { "default_port": 27090 } }
```

文件缺失、解析失败或 `schema` 不认识时，进入 `Disabled`：不注册任何 Hook，只写一条 boot 记录。解析用开发包里的 rapidjson。

`log_level` 本轮只写进 `config` 记录，不改变 trace 内容（审阅修订：首版读进来后没有任何用途，这里明确为只记录；如需分级，留到 tx-04-05 再定义）。

### 4.6 trace（JSONL）

- **格式。** 每行一个对象，公共字段为 `t`（QPC 微秒）、`seq`、`ev`。
- **写入方式。** 主线程只写有界环形缓冲，后台线程落盘。缓冲满时计数并丢弃，不阻塞。
- **本轮的事件：**

| `ev` | 内容 |
| --- | --- |
| `boot` | DLL 版本、exe 的 PE timestamp、模块基址、`__REPENTOGON_VERSION`、ModInit 所在线程 id |
| `config` | 配置内容，或解析失败的原因 |
| `capability` | 组名、是否通过、失败的点位与原因 |
| `hook_registered` | 已注册的 Hook |
| `first_call` | 每个 Hook 第一次被调用 |
| `pointer_patch` | 类、槽号、原值、期望值、新值、是否成功 |
| `counters` | 每 60 次 `Manager::Update` 一次快照：各 Hook 的调用数、非主线程透传数 |
| `command` | 来源、文本、结果，以及当时的 `g_Game` 值（Q14） |
| `state` | 占位会话状态的迁移 |
| `run_exit` | 每次 `Game::Exit` 之后：占位状态回到 `Idle`，清空还没执行的变更（审阅修订：首版实现把这一条记为 `shutdown`） |
| `shutdown` | 进程退出（审阅修订）。`DLL_PROCESS_DETACH` 且 `lpReserved` 不为空时，先把环形缓冲里剩下的事件同步写出，再记这一条。这时系统已经结束了其他线程，它们可能停在 CRT 的文件锁里，所以 trace 文件改用 Win32 句柄和 `WriteFile` 写入，不用 CRT 的 `FILE*`。这一步只写文件，不创建线程，不调用 ZHL；写不出来只是少一条记录，不算失败 |

### 4.7 `cs` 命令协议

- **请求。** 单行：`cs <verb> [args]`。
- **响应。** 若干行 `key=value`，第一行是 `ok=1`，或者 `ok=0` 后跟 `error=<code>`。
- **本轮的 verb：**
  - `version`：DLL 版本、协议版本、REPENTOGON 版本；
  - `status`：`status_version=1`，以及能力组、占位状态、Manager 状态、`g_game` 是否为空、计数摘要；
  - `host <port>`、`stop`：只经命令队列切换占位状态；
  - `echo <token>`：原样返回 token，供 E2E 用；
  - `counters`：完整的 Hook 计数。
- **执行时机。** 处理器只校验并入队，命令在下一次 `Manager::Update` 入口执行（设计 7.2 第 1 步）。`status` 和 `counters` 读的是快照。
- **输出方式（审阅修订）。**
  - `out` 为空时，用 `Console::Print` 输出整段响应。Lua 调用就是这种情况，输出靠 `g_Game+0x680D8` 捕获。
  - `out` 不为空时只写入 `out`，不再 Print。这与 REPENTOGON 自己处理命令的写法一致（`repentogon/Patches/ConsoleHooks.cpp:306-309`）。
  - 原写法是“`out` 不为空时同时写入 `out`”，会让自行显示 `out` 的调用方看到两份输出。
- **捕获时是否写入控制台历史（审阅修订；现已静态确认不写入）。** 本次 IDA 已确认：该字段为 `std::string*`，Print 捕获后直接返回，不进入历史分支，继续使用 Print。以下两种处理是原审阅提出的分支，现已选择不写历史的原生捕获路径。
  - 如果仍会写入，Lua 来源的 `cs` 响应改为直接追加到捕获字符串，不调用 Print，避免界面轮询刷屏；
  - 如果确认不了，保持用 Print，靠第 6 节降低轮询频率。
- **Hook 位置。** `Console::RunCommand` 的 priority 设为 -10000，排在 REPENTOGON 的 7 个 Hook 之外。只有 `cs` 开头的命令由我们处理，并且不调用 `super()`。
- **来源判定**（只用于 trace）：
  - 调用时 `g_Game+0x680D8` 不为空，判为 Lua；
  - 来自测试控制口的，判为 port；
  - 其余判为控制台。

### 4.8 测试控制口

- **何时存在。** 只在 `control_port.enabled=true` 时启用，监听 `127.0.0.1`，同时只接受一个连接。
- **协议。** 按行收发，命令文本和 4.7 相同；响应以单独一行 `.` 结束。
- **线程。** 用一个工作线程（tx-04-06 时扩展为网络线程）收命令，放进主线程命令队列。主线程在 `Manager::Update` 入口执行，结果经有界完成队列交回工作线程。主菜单期间 `Manager::Update` 也在运行，所以菜单阶段同样能响应。

## 5. 本轮的 Hook 行为

- **全部 ZHL Hook：**
  - `Armed` 恒为假：每次只调用一次 `super()`，原样返回，不读写任何游戏对象；
  - 计数加一；不在主线程上时，额外计入“非主线程透传”；
  - 外层 owner 照常压栈、弹栈，维护上下文栈（本轮不用它判定 tick，只用它在 trace 中给计数分类）。
- **异常语义（审阅修订）。**
  - wrapper 不声明 `noexcept`，对原函数抛出的异常保持中立：异常照常穿过 wrapper 交给上层，不会被我们变成 `std::terminate`。
  - 只有我们自己的代码保证不抛出：计数、`FrameBegin/FrameEnd`、命令处理和 trace，要么标 `noexcept`，要么在内部 `try/catch`。
  - 上下文栈用 RAII 弹栈。异常穿过时照常弹栈，并记一条 `context_error`（`exception`）。
  - 依据：REPENTOGON 自己会在游戏调用链中抛 C++ 异常。例如存档的成就数不符时，它会 `throw std::runtime_error`（`repentogon/SaveSyncing/SaveSyncing.cpp:204`）。原 R8 要求“Hook 函数一律 `noexcept`”，会把这类异常变成进程直接终止。
- **priority：** `Manager::Update` 为 -10000，`Console::RunCommand` 为 -10000，其余为 0，全部写进 Hook 清单。
- **指针补丁：**
  - 在第一次主线程 `Manager::Update` 时安装。先核对槽里的原值是否等于 binding 中的目标（第 8/9/10 槽为 `0x5ECD80/0x5ECDE0/0x5ECE50`，第 35 槽为 `0x633DD0/0x6331B0`），再写入。
  - wrapper 只调用原函数并计数。原值不一致时，`input` 组记为失败，不写入。
  - 常驻，不恢复。
- **主线程。** 记录 ModInit 所在的线程 id。LoadMods 由 `IsaacMain` 调用，所以 ModInit 就在主线程上执行。
- **本轮不安装：**
  - Join 的 3 处 mode predicate 和创建点调用点补丁（tx-04-06）；
  - 自有虚拟设备（tx-04-06）；
  - 主循环时钟和 3 个 IAT（已决定去掉）。

## 6. 界面 mod 与测试 Lua mod

- **`isaac-cs-ui`**（设计 9.2）：
  - 内容：“Isaac CS”菜单和窗口，显示版本、能力组、占位状态和计数摘要；“开始主机”“停止”两个按钮（本轮只切换占位状态）；状态变化时弹通知；`Disabled` 时只显示原因。
  - 窗口默认可见。用户按控制台键（默认 `` ` ``）打开 REPENTOGON 浮层就能看到，E2E 不需要点鼠标。
  - 窗口的 Render 回调第一次执行时，以及状态变化时，用 `Isaac.DebugString` 往 `log.txt` 写 `[isaac-cs-ui]` 标记（审阅修订：原为“每次 `cs status` 成功时”，按首版的轮询频率每秒会写好几次）。
  - **轮询频率（审阅修订）。** Render 回调每 60 帧（约 1 秒）才查询一次 `cs version`、`cs status`；首版为每 15 帧一次。REPENTOGON 只在浮层打开或窗口被固定时渲染窗口（`repentogon/ImGuiFeatures/CustomImGui.h:571`），所以浮层关闭时不会轮询。
- **`isaac-cs-test`**（只在 E2E 期间部署，D9）：
  - 只注册渲染回调（审阅修订）。run 内用 `MC_POST_RENDER` 调用一次 `Isaac.ExecuteCommand("cs echo lua-run")`；主菜单用 REPENTOGON 的 `MC_MAIN_MENU_RENDER`（1023，`repentogon/resources/scripts/enums_ex.lua:25`）调用一次 `("cs echo lua-menu")`。返回值写进 `log.txt`，前缀 `[isaac-cs-test]`。原写法是“只注册 `MC_POST_RENDER`，在主菜单和 run 内各调用一次”，但这个回调在主菜单不一定触发。
  - 脚本加载时另调用一次 `lua-menu`，记为 `phase=load`，只作补充观察。首版只在脚本加载时调用 `lua-menu`，那时还没到主菜单，看不到 Q14 关心的状态。
  - **run 内的判断（D18）。** 用 `Game():GetNumPlayers()` 判断是否已有玩家，放在 `pcall` 里。原写法 `Isaac.GetNumPlayers` 不存在（REPENTOGON 脚本和 EID 都用 `Game():GetNumPlayers()`），`pcall` 每帧失败，所以第七次 E2E 之前从未记录 `lua-run`。
  - **界面窗口（D18）。** REPENTOGON 在第一帧 ImGui 时按 `<存档目录>\Repentogon\imgui.ini` 恢复自定义窗口的可见性（`repentogon/ImGuiFeatures/ImGui.cpp:176-198`），会覆盖界面 mod 加载时的 `SetVisible(true)`。第六次 E2E 把 `Visible=0` 存进了这个文件，第七次窗口因此没有显示。测试 mod 在每次主菜单渲染时调用 `ImGui.SetVisible("isaac-cs-panel", true)`，并记一次 `panel-visible`。界面 mod 本身继续尊重用户保存的可见性。
  - 不注册任何 gameplay 回调。
- **当前 mod 状态。** 游戏根目录 `mods\` 里只有一个启用的 mod：“REPENTOGON Download Helper”（`repentogonreleasesbackup_3643104060`）。它没有 Lua，不属于 gameplay mod。harness 只记录启用列表，不修改用户自己 mod 的启用状态。

## 7. Harness（`tools/harness`，i686）

入口：`harness e2e tx-04-04 --game "<游戏目录>" --out <artifact 目录>`，一次执行。

1. **记录环境。** 游戏目录；`Repentogon\isaac-ng.exe` 的 PE timestamp（期望 `0x68154D8D`）；已安装的 REPENTOGON 版本；启动器配置里的 `Update` 值；`steam.exe` 是否在运行；有没有残留的 `isaac-ng.exe` 进程；启用的 mod 列表；Steam 本地云存储目录（审阅修订，见第 2 步）；REPENTOGON 的 `SkipIntro` 选项值（D18，只读；不为 1 时不注入）。
   - 下面几种情况属于运行前提不满足，记下原因后结束本次执行：exe 版本不对（继续跑会注入错误版本）、Steam 没有运行、已有游戏进程在运行。这是功能上的保护，不是流程 gate。
2. **存档（D14，审阅修订）。**
   - **原写法与问题。** 原方案只复制存档目录（`Documents\My Games\Binding of Isaac Repentance+\`），并认为第二个存档对应 `persistentgamedata2.dat`、`gamestate2.dat`。审阅确认这个假设不适用于 REPENTOGON：开着 Steam Cloud 时，第二个存档写在 Steam 云存储里，原方案的对比看不到。
   - **第二个存档的实际文件**（静态事实，`repentogon/SaveSyncing/SaveSyncing.cpp:52-71`）：

     | 模式 | REPENTOGON 存档 | 同步的原版存档 | 游戏状态 |
     | --- | --- | --- | --- |
     | Steam Cloud | `rgon_steam_persistentgamedata2.dat` | `rep+persistentgamedata2.dat` | `rep+gamestate2.dat` |
     | 本地 | `<存档目录>\Repentogon\rgon_persistentgamedata2.dat` | `<存档目录>\persistentgamedata2.dat` | `<存档目录>\gamestate2.dat` |

     REPENTOGON 还会写同步状态文件：云模式是 `rgon_savesyncstatus.json`，本地模式是 REPENTOGON 数据目录下的 `savesyncstatus.json`（同一文件 238-243 行）。
   - **本机是云模式**（直接观察，2026-10-24）：
     - Steam 本地云存储目录是 `D:\steam\userdata\387111160\250900\remote\`，里面有上表云模式的文件；
     - 存档目录下没有 `Repentogon\rgon_persistentgamedata*.dat`。
     - harness 不写死这个路径：从 `HKCU\Software\Valve\Steam` 的 `SteamPath` 出发找 `userdata\*\250900\remote\`，找到几个就全部记录、全部快照。
   - **快照。** 运行前、两次启动之间、结束后，各把存档目录和 Steam 云存储目录复制一份，放在 `save_before/`、`save_mid/`、`save_after/`，每份都含 `documents/` 和 `steam_remote/`。只复制，不写入、不恢复；变化按每次启动分别列出。
   - **允许的变化：**
     - 上表中第二个存档的文件，以及同步状态文件；
     - `save_backups\` 中新增的文件（游戏和 REPENTOGON 生成的每日备份，例如 `20261024.rgon_steam_persistentgamedata2.dat`），以及第二个存档已有备份的变化；
     - `log.txt`、`cmd_history.txt` 和选项类文件（`*.ini`、`inputconfigs*`）；
     - REPENTOGON 第二个存档的游戏状态附属文件（D18）：存档目录 `Repentogon\` 下各子目录中的 `remote_gamestate2.dat/.json`、`ex_remote_gamestate2.json`（第七次 E2E 直接观察到 `EntitySaveStateManagement`、`ItemPoolManager`、`VirtualRoomSetManager` 三处）。
   - **第一、三个存档的判定。**
     - 直接观察：官方启动器那次运行（00:40:59 启动，00:41:37 退出）期间，Steam 云存储目录里三个存档的 `rgon_steam_`、`rep+` 文件和 `rgon_savesyncstatus.json`，修改时间都落在运行期间。这说明 REPENTOGON 本身也可能写其他槽位，归因为 `UNKNOWN`。
     - 因此按字节判定：只是修改时间变了、字节相同，只记录；字节不同记为 F12，E7 失败，报告注明是哪一次启动，以及可能来自 REPENTOGON 的存档同步。
     - **基线对比（D18）。** 第七次 E2E 中，第一、三个存档的云端文件和当日备份在启用与关闭配置两次启动里都有字节变化，而关闭配置那次本 DLL 不注册任何 Hook。因此：关闭配置那次启动中的第一、三槽变化记为 REPENTOGON 基线；启用启动中的同一文件如果在关闭配置启动里也变化，同样放行；只在启用启动中变化的仍记为 F12。关闭配置启动没有执行时不做任何放行。每条变化都标注 `baseline` 说明。
   - 不自动恢复存档，以免和 Steam Cloud 冲突；备份留给用户自行决定。
   - E2E 只做“开新局、短暂停留、退回菜单”，不做任何推进进度的操作。
3. **部署。** 按第 3 节部署，记录每个文件的路径和来源，包括每一次写入 `config.json`（审阅修订：首版的部署清单没有这一项）。
4. **注入启动（D13，复现 REPENTOGON 启动器的流程）：**
   - **路径（审阅修订）。** exe 路径、工作目录和 `zhlLoader.dll` 路径一律用普通 Win32 绝对路径，不用 `\\?\` 前缀。
     - 首版对 `--game` 调用了 `canonicalize()`，这三处都变成了 `\\?\F:\...`；官方启动器的 `launcher.log` 记录的是普通路径。
     - 路径不是 ASCII 或超过 `MAX_PATH` 时不注入，记录原因后结束。
   - 以挂起状态 `CreateProcessW` 启动 `Repentogon\isaac-ng.exe`，工作目录设为 `Repentogon\`（那里已有 `steam_appid.txt`）。命令行传带引号的 exe 路径（`"<exe>"`），不带其他参数（审阅修订：首版传的是 NULL，含空格的路径没有引号）。`launcher.log` 没有记录官方用的命令行，用带引号的 exe 路径是设计决定。
   - **启动器标记（D17）。**
     - 静态事实：REPENTOGON 的 `Lua5.3.3r.dll` 在 `DllMain` 中检查环境变量 `LAUNCHED_BY_RGONLAUNCHER`，变量缺失或为空时弹出 “Use The Launcher!” 并 `ExitProcess(0)`（`libs/lua/lua.c:28-41`）。exe 在进程初始化阶段就加载这个 DLL，早于 `Launch`。
     - 官方启动器的 `REPENTOGONLauncherApp.dll` 中，`SetEnvironmentVariableA` 和这个变量名就在创建进程的日志字符串旁边，推断它在创建进程前设置该变量；它设的值未知。
     - harness 用显式环境块传给子进程：复制自身环境，加上 `LAUNCHED_BY_RGONLAUNCHER=1`，标志加 `CREATE_UNICODE_ENVIRONMENT`。harness 自己的环境不改。
     - 这道检查是为了防止 exe 在没有 REPENTOGON 的情况下运行而损坏存档。harness 只在 `Launch` 返回 0 后才 `ResumeThread`，失败时直接终止挂起的子进程，这层保护仍然成立。
     - 启动器可能还会附加 `--luaheapsize=` 等参数（字符串可见，是否用于 REPENTOGON 模式未知）；本轮不附加。
   - 在目标进程里分配内存，写入 `zhlLoader.dll` 路径和一小段 x86 stub：`LoadLibraryA` → `GetProcAddress("Launch")` → `Launch(0)` → 返回结果；
   - harness 本身是 32 位，kernel32 地址可以直接用自己进程里的；
   - 注入前记录 `Repentogon\repentogon.log` 的大小和修改时间（审阅修订）。`Launch` 第一步就以 `"w"` 方式打开这个文件，libzhl 加载时也会往里追加，所以从它能看出线程停在哪一段；
   - 用 `CreateRemoteThread` 执行 stub，等待最多 60 秒，以线程退出码作为 `Launch` 的返回值（0 为成功）；
   - **失败时的诊断（审阅修订）。** 超时或 `Launch` 返回非 0 时，在终止进程之前依次做：
     - 再记录一次 `repentogon.log` 的大小和修改时间，并复制这个文件；
     - 截一张整屏截图，因为错误对话框不一定属于游戏进程；
     - 枚举这个进程的全部顶层窗口，包括 `#32770` 对话框，记录类名和标题；
     - 记录进程是否已经退出，以及退出码。

     启动失败时同样收集第 6 步的日志（首版在这一步失败时直接返回，什么都没收集）；
   - 最后 `ResumeThread` 恢复主线程。
5. **驱动游戏。** 按原型 `e2e_physical_input.py` 的方式，用扫描码 `SendInput`，每次发送前确认焦点在游戏窗口。每一步都用 `cs status` 轮询，并设超时。菜单的按键序列（选第二个存档、开新局、退回菜单）在实现时确认，写进 harness 的场景配置。
   - **浮层关闭后的确认（审阅修订）。**
     - 静态事实：REPENTOGON 的窗口过程中，控制台键只负责打开浮层；浮层打开时按 ESC 会关闭浮层，并且这条按键消息不再交给游戏（`repentogon/ImGuiFeatures/ImGui.cpp:219-257`）；ENTER 只在控制台输入为空时才关闭浮层（`ConsoleMega.h:162-164`）。
     - 游戏是否还会从别的途径读到这次 ESC，仍是 `UNKNOWN`。
     - 所以每次关闭浮层后，先截图，再重新读取 `cs status`。后面的菜单操作一律以新读到的 `menu_id`、`run_active` 为准，不沿用打开浮层之前读到的状态（首版沿用了旧状态）。
   - **第一次启动（启用配置，开启测试控制口）：**
     - a. 到达主菜单后，记录 `cs status`，包括 `g_game` 是否为空（Q14）；
     - b. 测试 mod 在主菜单调用 `lua-menu`；
     - c. 按控制台键，输入 `cs echo console-menu` 并回车；
     - d. 浮层打开时截图（窗口应可见），再按 ESC 关闭；
     - e. 进入第二个存档，开新局；
     - f. 在 run 内停留约 10 秒，采集计数；通过控制口发 `cs echo port-run`、`cs host 27090`、`cs status`、`cs stop`；测试 mod 调用 `lua-run`；用控制台输入 `cs echo console-run`；
     - g. 退回主菜单，确认 `Game::Exit` 计数增加；
     - h. 发 `WM_CLOSE` 关闭窗口，等待进程退出，检查有没有新的崩溃转储。
   - **第二次启动（关闭配置）：** 启动并到达主菜单，确认 trace 中只有 boot 记录、注册数为 0、指针补丁数为 0；截图后关闭。
6. **收集。** 我们的 trace、`Repentogon\repentogon.log`、存档目录里的 `log.txt`、截图、存档对比、harness 自己的步骤日志；输出 `report.json` 和 `report.md`。
7. **清理。** 删除测试 mod；把 `config.json` 改回 `enabled=false`；DLL 和界面 mod 保留。不计算任何哈希。

## 8. E2E 验收（本次推进唯一的一次测试）

一次执行。失败只记录，不自动重跑，也不在实现后补单元测试。

D17（用户决定）：第二次 E2E 失败后，按第 7 节第 4 步补上启动器标记，再执行一次，作为 tx-04-04 的第三次 E2E；artifact 放在 `artifacts/repentogon/tx-04-04-e2e-03/`，同样只执行一次、不自动重跑。

D16（用户决定）：第一次 E2E 失败后，按修订后的方案修复，完成后再执行一次，作为 tx-04-04 的第二次 E2E。这一次同样只执行一次、不自动重跑，判定仍按下表。

| 场景 | 期望结果与判定依据 |
| --- | --- |
| E1 加载 | ModInit 返回 0；boot、config 记录正常；loop、input、player、join、control 五个组通过，device 组解析成功并有记录；游戏到达主菜单，没有弹框也没有提前退出 |
| E2 Hook | 注册数等于 Hook 清单条数；8 个指针补丁的原值和期望一致并已写入；run 内停留后，“必须非零”集合的计数全部大于 0；非主线程透传数为 0；REPENTOGON 的日志初始化正常，浮层能打开 |
| E3 命令桥 | `console-menu/console-run`、`lua-run`、`port-run` 都在 trace 中出现，并返回正确输出（Lua 的返回值写在 `log.txt`，port 的结果在响应里）；如实记录 Q14：主菜单时 `g_Game` 是否为空，以及 `lua-menu` 的结果 |
| E4 命令队列 | `cs host` 发出后，在下一次 `Manager::Update` 时状态变为 `Hosting`；`cs stop` 后回到 `Idle`；`status` 快照与之一致 |
| E5 界面 | `log.txt` 里有 `[isaac-cs-ui]` 的窗口渲染标记；有截图 |
| E6 关闭配置 | 第二次启动时注册数和指针补丁数都为 0，游戏正常到达主菜单 |
| E7 退出与副作用 | 退回菜单时 `Game::Exit` 计数增加；两次启动都正常退出，没有新的崩溃转储；存档目录和 Steam 云存储目录的变化不超出第 7 节第 2 步允许的范围，第一、三个存档的文件字节不变（审阅修订：原为“存档变化只限于第二个存档和日志、选项类文件”，只看了存档目录） |

**“必须非零”的计数**（新开一局并停留就一定会走到的点）：

- `Manager::Update`、`Manager::Render`、`Game::Update`、控制台判断、普通 `Room::Update`、`EntityList::Update`；
- MC_POST_UPDATE 调用函数、MC_POST_NEW_ROOM 调用函数、`LuaEngine::PostGameStart`、`Manager::ExecuteStartGame`；
- InputManager 更新、设备公共更新、Keyboard 的 Poll 槽（第 35 槽）；
- `Entity_Player::Update`、移动、射击、丢弃 consumer；
- Join pass、`Console::RunCommand`。

以下点位允许为 0，只记录：门切房（`RoomTransition::Update`、`0x2FA0F0`）、生命周期 FSM 内的 Room::Update、`Game::RestoreState`、炸弹/主动道具/卡牌药丸 consumer、奇数帧 helper、Gamepad 的槽、查询槽。

**预先列出的失败情况：**

| 编号 | 情况 |
| --- | --- |
| F1 | 注入失败：`Launch` 返回非 0，或等待超时。按第 7 节第 4 步保留 `repentogon.log` 的前后状态、整屏截图和窗口列表 |
| F2 | `ZHL::Init` 弹框并 `ExitProcess`：出现错误窗口，或进程过早退出 |
| F3 | ModInit 内发生异常，进入 `Disabled` |
| F4 | 某个能力组判定失败：期望指令不符，或 REPENTOGON 地址交叉核对不一致 |
| F5 | 指针补丁的原值和期望不一致 |
| F6 | “必须非零”集合里有计数为 0 |
| F7 | 出现非主线程调用 |
| F8 | 某个来源的命令丢失，或输出不对 |
| F9 | Q14 的观察结果：主菜单时 `g_Game` 为空，导致 `lua-menu` 返回空。这一项只是观察，用来决定设计 9.3 采用哪种处理，不算本轮失败 |
| F10 | 界面窗口没有渲染 |
| F11 | 崩溃或卡死：看门狗超时 |
| F12 | 存档目录或 Steam 云存储目录出现允许范围之外的变化，或第一、三个存档的文件字节改变（审阅修订；原为“存档出现第二个存档之外的变化”） |
| F13 | 启用和关闭两次启动的 `repentogon.log` 对比，REPENTOGON 自身出现了新的错误 |

**artifact 清单：**
- 环境记录（含 Steam 云存储目录）、部署清单（含每次写入 `config.json`）、两次启动各自的 trace；
- `repentogon.log`、`log.txt`、截图；注入失败时，另有 `repentogon.log` 的前后状态、整屏截图和窗口列表；
- `save_before/`、`save_mid/`、`save_after/`（各含 `documents/` 和 `steam_remote/`），以及按每次启动分开的变化列表；
- harness 步骤日志、`report.json`、`report.md`；`report.md` 只列实际生成的文件（审阅修订：首版列出了不存在的 `enabled/`、`disabled/`）。

## 9. 风险与未决事项

| 编号 | 事项 | 处理 |
| --- | --- | --- |
| Q14 | 主菜单时 `g_Game` 是否为空 | E3 中观察，按结果在设计 9.3 的两种处理中选一种。静态旁证（审阅修订）：REPENTOGON 的窗口过程每处理一条消息都会读 `g_Game->GetConsole()->_state`（`ImGui.cpp:306`、`331`；`GetConsole()` 是取成员地址）。官方启动的游戏能在菜单里正常运行，所以推断至少在这个窗口过程装上之后，`g_Game` 不为空。脚本加载时可能更早，那时的结果单独记为 `phase=load` |
| Q15 | 门切房内层的所在函数 `0x2FA0F0` | 已解决：调用方只有 `RoomTransition::ChangeRoom` 和 `Console::RunCommand`，上下文按调用方区分 |
| R1 | `IsaacRepentance.h` 在 C++20 下能否编译 | 不行就让相关编译单元退回 C++17（第 3 节） |
| R2 | 菜单按键序列（选第二个存档、开新局、退回菜单） | 实现时确认，写进 harness 配置 |
| R3 | REPENTOGON 的 ImGui 控制台能否接收 `SendInput` 输入的文字 | 不行时，E3 的控制台来源记为失败，另外两个来源照常判定。浮层的按键处理见第 7 节第 5 步（审阅修订） |
| R4 | 要求 Steam 在线、Steam Cloud 会同步第二个存档 | 记入报告；不自动恢复存档 |
| R5 | 设备公共更新 `0x5EC900` 的栈对齐序言 | 实现时用 IDA 确认 MologieDetours 能搬移它的前几条指令；不行则 `input` 组失败，并记录原因 |
| R6 | 自有定义里还有几处 ABI 细节未定（4.3 中标注的） | 实现时用 IDA 静态确认 |
| R7 | REPENTOGON 版本漂移 | 自动更新已关闭（D12）；trace 记录版本 |
| R8 | Hook 里抛出异常，或在 Hook 内崩溃 | 审阅修订：wrapper 对原函数抛出的异常保持中立，只有我们自己的代码保证不抛出（第 5 节）；计数路径不分配内存。原写法“Hook 函数一律 `noexcept`”会把游戏调用链中的异常变成进程终止 |
| R9 | 首版注入超时，根因 `UNKNOWN` | 首要嫌疑是 `\\?\` 路径和空命令行（静态推断）。按第 7 节第 4 步改用普通路径并补充诊断；如果仍然超时，靠诊断材料继续定位，不自动重跑。后续：第二次 E2E 证明路径不是原因；诊断截到 “Use The Launcher!”，根因静态确认为启动器环境变量检查，按 D17 处理（第 7 节第 4 步） |
| R10 | REPENTOGON 的存档同步也会写其他槽位 | 按字节判定，并按每次启动分开记录（第 7 节第 2 步）。第一、三个存档的字节改变时 E7 失败，并注明可能的来源；归因等看过诊断材料再定。D18 起按关闭配置启动做基线对比 |
| R12 | 开场动画需要人工跳过，否则启动等待超时 | D18：用户自行开启 REPENTOGON 的 `[VanillaTweaks] SkipIntro`（`repentogon/Patches/CutsceneSkip.cpp:13-26`）。harness 在环境检查中只读这一项，未开启则在注入前结束，不改用户设置 |
| R11 | 进程退出时同步写 trace | 系统已结束其他线程，CRT 的文件锁可能没人释放，所以改用 `WriteFile`；写不出来只少一条 `shutdown` 记录（4.6） |

## 第三次超时后的实现修订（2026-10-24）

本次修订和证据边界见[结果中的分析与修订](tx-04-04-results.md)。启动前与轮询期间确认前台焦点；按 startup_timeout_seconds 等待真实菜单，只对启动中的只读 status 超时继续查询，不重发变更或传输失败的请求。控制口请求在入队时设置五秒截止，主线程不执行已过期请求；所有游戏操作仍在主线程，变更保留原有下一 Manager 边界语义。

harness 新增 control.jsonl 的请求/响应耗时，以及关闭进程前的失败现场；DLL 新增请求的入队/执行/超时、前三帧 begin/end、first_return/completed。关闭配置启动用本次 log.txt 的 MC_MAIN_MENU_RENDER 夹具确认菜单，取代固定 12 秒等待。不开新 Hook、不改游戏动画/选项或时钟。

用户本会话明确同意执行一次第四轮 E2E，目录为 artifacts/repentogon/tx-04-04-e2e-04/。修复在此之前只编译，没有进行其他测试；第四次同样失败不重跑。

### 第四至第六次执行记录

第四、五次等待菜单超时，用户分别说明未跳过动画、未及时操作；第六次在用户确认准备好后启动，已跳动画并进入第二槽 run，整体仍 FAIL。退菜单固定序列落入 Options；另有第一、三槽 REPENTOGON 存档字节变化和未列入允许清单的第二槽扩展文件。原始 FAIL 不改写，不自动重跑；具体记录和 Q14 新观察见结果文档。之后测试先说明配合事项，需手动跳动画时先确认用户已经准备好。

### 第六次后按状态返回菜单

用户要求修复退菜单导航后再测试。目标 IDB 确认 PauseScreen 位于 Game+0x2363C，Show 将 state 设为 1、selection 设为 1；选择 0=Options、1=继续、2=退出，0..2 循环。固定 DOWN 两次会从继续移到 Options。新增主线程只读暂停状态快照，harness 等待 state=1 后最多移动一格，观察 selection=2 才按 ENTER，再检查 run_active=0、已知本地菜单与 Game::Exit 计数。未知状态或未到预期立即失败，不重发输入。

原始静态依据见 artifacts/repentogon/tx-04-04-pause-static/README.md。DLL/harness 已纯构建成功，第七次 E2E 等用户确认准备好后执行一次，保存到新的 tx-04-04-e2e-07 目录；没有修改其余判定或存档允许范围。

第七次已在用户确认准备好后执行完毕：暂停 selection 1→2，确认后 run_active=0/menu_id=3，Game::Exit 计数 0→1；启用/关闭两次进程均正常退出。整体仍 FAIL（E3 Lua run 缺失、E5 UI render 标记缺失、E7 存档变化），未自动重跑；关闭配置启动也观察到第一、三槽字节变化，具体来源仍未归因。完整记录见结果文档第七次 E2E。
