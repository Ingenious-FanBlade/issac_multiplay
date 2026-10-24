# tx-04-04 实现与 E2E 结果

> 最新状态（2026-10-24）：按 D18 修正后的第八次 E2E **PASS**，E1–E7 全部通过，见文末“第八次 E2E”。下面各次的失败记录原样保留。
>
> 最新状态（2026-10-24）：暂停菜单状态导航已修订并编译；用户确认准备好后的第七次 E2E 已正确选中 EXIT GAME、返回菜单 3，Game::Exit 计数 0→1，两次启动正常退出。整体仍 FAIL：E3 缺少 Lua run 回应、E5 缺少 UI 渲染标记、E7 存在允许范围外的存档字节变化（关闭配置启动也出现第一、三槽变化）。见文末“第七次 E2E”；所有原始记录保留。

> 第二次结果摘要（2026-10-24）：已按审阅要求修订并编译；D16 允许的第二次 E2E 仍为 **FAIL**。新增诊断捕获 `Use The Launcher!` 错误框，见文末“修订与第二次 E2E”。以下第一次 E2E 和原静态审阅记录保留。
>
> 第三次 E2E（D17）走通了注入启动、ModInit 和第一帧，但在到达主菜单之前的状态轮询中返回 `main_thread_timeout`，仍为 **FAIL**，见文末“第三次 E2E”。

日期：2026-10-24。结果：**实现与编译完成；E2E FAIL（注入启动超时），运行时能力尚未验证**。E2E 之后补充了一次静态审阅，见文末“静态审阅”。

范围依照[实现方案](tx-04-04-implementation-plan.md)；未实施 tx-04-05 的 tick/A 输入，也未实施网络、B、Join 字节补丁或 world_record。

## 已写入的代码

- `mod/`：Win32 DLL `zhlIsaacCS.dll`，ModInit 永远返回 0，缺失/关闭/错误配置不注册 Hook。
- `mod/binding/hooks.json`：统一注册清单。生成器同时输出 C++ wrapper、JSON/Markdown 清单；合并原型重复调用点后共 27 个入口 Hook，另有 8 个设备槽补丁。入口 Hook 使用 ZHL 共享链，设备槽在首次主线程 Manager::Update 延后安装；gameplay 路径只透传、计数和维护上下文。
- binding：REPENTOGON 定义按类型和 RVA 交叉核对，自有定义按 RVA 与带 relocation/rel32 通配的入口指令定位。20 个自有入口/设备依赖的静态数据保留在 `mod/binding/v1_9_7_12-static.json`；未新增或校验哈希。
- ops/core：配置、后台 JSONL、有界 ring、主线程命令队列、上一帧状态快照、`cs version/status/counters/echo/host/stop`、单连接 loopback TCP。host/stop 仅切换占位 Idle/Hosting，无真实网络或输入接管。
- `ui/isaac-cs-ui/`：REPENTOGON ImGui 界面，无 gameplay 回调。`tests/e2e-mods/isaac-cs-test/`：临时 Lua 命令夹具，只注册 MC_POST_RENDER，另在脚本加载时记录菜单命令返回值。
- `tools/harness/`：Rust i686 单入口，环境记录、存档前后备份、部署、挂起启动/加载 zhlLoader/Launch(0)、焦点检查和扫描码、日志与截图收集、两次启动的 E1–E7 报告、失败清理。菜单选择依据只读状态，不进入原版 Online。
- `scripts/build.ps1`：只构建，不含 cargo test、游戏试跑或验收步骤。

这些是源码实现事实，不表示相应运行时行为已通过。

## 静态核对与编译事实

读取了目标 v1.9.7.12 IDB，确认待定 ABI 与 MologieDetours 的指令搬移实现。EntityListUpdate 的 bool 栈参数控制部分更新、EAX 返回与 `retn 4`；RoomLifecycleFSM 的 EAX 指针值用 uintptr_t 透传；DoorNestedOwner 是 Game thiscall(int,int) → int、`retn 8`；JoinPass 返回 unsigned int。设备公共更新首 6 字节 `53 8B DC 83 EC 08` 是完整、无相对寻址的指令。

Menu_Save 调用 `select_save_slot` 的分支在 RVA `0x5AEFD8–0x5AF00B`：SelectedSave=0/1/2 分别 push 1/2/3，因此第二档选择值为 1。Menu_Game::Update（`0x559590`）中 SelectedElement=0 进入 CHARACTER（菜单 5）；导航会环绕，所以 harness 在确认值 0 后才按确认键。以上为 `STATIC_CONFIRMED`。

开发包 `F:/projects_isaac/REPENTOGON-1.1.3` 的 tag 1.1.3 已构建 libzhl，生成 SDK 头与导入库。SDK 与本项目均使用 VS2022 v143/Win32、MSVC 19.44.35217、/MD。项目 DLL 与 Rust i686 harness 链接成功，产物在 `out/repentogon/`。自建 libzhl.dll 未部署。

实现过程中处理了两项 SDK 差异：FunctionHook::Add 为 private，改为 ModInit 中调用公开构造函数；生成头在 C++20 下出现 C4596，两个 SDK-facing 编译单元按方案使用 C++17，其余保持 C++20。地址定义的 outfunc 采用常驻存储，以适应 REPENTOGON 稍后再次调用 Definition::Init。上述为源码/编译事实。

## 第一次 E2E

本节原标题为“唯一一次 E2E”，写作时只执行过这一次；之后按 D16、D17 又执行了第二、三次，内容保持原样。

执行命令：

```powershell
out/repentogon/harness.exe e2e tx-04-04 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out artifacts/repentogon/tx-04-04-e2e-01
```

前提记录：目标 PE timestamp 为 `0x68154D8D`、已安装 REPENTOGON 为 1.1.3、启动器 Update=0、Steam 正在运行、无已有游戏进程；用户 mod 的启用状态仅记录。完成整个存档目录备份和 DLL/界面/临时测试 mod/配置部署后，进入挂起子进程的注入启动阶段。

**实际失败：`Launch timeout (60 seconds)`。** 远程注入线程在 60 秒内未返回。harness 没有 ResumeThread 到游戏主循环，没有发送菜单/游戏输入，未产生项目 boot trace、截图或新的项目运行日志。无法由这一结果区分 LoadLibraryA、GetProcAddress、Launch 内部或对话框等待等具体停留位置；根因仍为 `UNKNOWN`。不能把它记为 DLL ModInit、Hook 安装、Q14 或界面的运行时结论。

| 项目 | 原始报告 | 解释 |
|---|---|---|
| E1–E6 | NOT_RUN | 启动阶段失败，未完成相应场景；第二次关闭配置启动未执行 |
| E7 | FAIL | 正常退出、Game::Exit 与两次运行日志对比等要求未完成；存档比较本身无变化 |
| Q14 | UNKNOWN | 未到达主菜单/命令桥，未观察 g_Game 或 Lua 返回值 |

只执行了这一次 E2E，没有自动重跑或在失败后修补代码再试。保留本轮实现与构建产物；下一次实质修改应针对注入启动的诊断与修复单独说明范围，不能将本轮失败改写成成功。

## 清理与 artifact

harness 按失败路径终止自己创建的挂起子进程。随后已从磁盘观察：临时 `mods/isaac-cs-test` 已移除；`Repentogon/isaac-cs/config.json` 已写回 enabled=false、control_port.enabled=false；DLL 与纯界面 mod 按方案保留。未自动恢复存档。

`artifacts/repentogon/tx-04-04-e2e-01/` 保留：

- `report.json`、`report.md`：原始失败报告；
- `steps.jsonl`、`scenario.json`、`deployment.json`：环境、操作步骤和部署来源；
- `save_before/`、`save_after/`、`save-changes.json`：整个存档目录前后副本，直接字节对比变化列表为 `[]`；
- `sources/`、`build-all.log`、`hook-manifest.json`、`hook-manifest.md`：可复查源码与构建/清单记录。

原始 report.md 中的通用 artifact 说明列出了日志/截图目录，但本轮因启动失败没有生成 `enabled/`、`disabled/`、boot trace 或截图；应以实际文件与本说明为准，不将旧运行日志拷入冒充本轮证据。

## 静态审阅（E2E 之后补充）

本节在 E2E 结束后由另一个会话补充。只读了代码、日志和文件时间戳，没有运行程序、构建或任何测试。上面的 FAIL 结论不变，本节也不算重跑。

### 结论

代码结构基本符合[实现方案](tx-04-04-implementation-plan.md)，静态核对下来大部分正确。但本轮目标是一次 E2E 通过 E1–E7，实际为 FAIL，**tx-04-04 尚未完成**。

**流程。** 以下几点符合现行规范：
- 只执行了一次 E2E，未补单元测试，未自动重跑；
- 构建脚本不含测试；
- 没有设 gate，没有计算或校验哈希；
- 存档未自动恢复，临时 mod 已清理。

**不符合之处：注释不足。** AGENTS.md 要求“编写代码时一定要写注释”，目前做不到：
- `mod/src/ops/config.cpp`、`mod/src/hooks/context.cpp` 没有注释；
- `mod/src/ops/control_port.cpp`、`mod/src/hooks/pointer_patch.cpp` 各只有一处；
- `mod/tools/export_ida_bindings.py` 除文件说明外没有注释；
- 多处一行写多条语句，`tools/harness/src/main.rs` 约有 20 行超过 160 字符。

### 注入超时的进一步定位

- **直接观察：**
  - `<游戏目录>\Repentogon\repentogon.log` 的修改时间是 2026-10-24 00:41:37，即官方启动器那次运行留下的，02:33 的 E2E 没有改动它。
  - 在 `F:\projects_isaac`、游戏目录和 `Documents\My Games` 下，都没有 02:00 之后新建的 `repentogon.log` 或 `zhl.log`。
- **静态事实：**
  - zhlLoader `Launch` 的第一步是 `fopen("repentogon.log","w")`（REPENTOGON `loader/loader.cpp:256`）。
  - zhlLoader 静态导入 libzhl：`loader.cpp:190` 直接调用了 `ZHL::Init`。
  - libzhl 加载时，各个 `FunctionDefinition` 的静态构造会经 `ZHL::Log` 以追加方式写同一个文件（`libzhl/HookSystem.cpp:246`、`libzhl/Log.cpp:18-30`）。
- **静态推断：**
  - 远程线程没有从 `Launch` 返回，日志也没有任何写入，所以线程最可能停在 libzhl 静态初始化之前，即进程初始化或 `LoadLibraryA` 加载依赖的阶段，而没有进入 `Launch`。
  - `zhlIsaacCS.dll` 要等主线程恢复后才由 LoadMods 加载，所以这次失败不涉及本 DLL。
- **与官方启动器的差异：** 官方启动器的 `launcher.log` 记录的是普通路径（`lpCurrentDirectory = F:\SteamLibrary\...\Repentogon`），远程线程 5 秒内返回 0。harness 的做法不同：
  - `tools/harness/src/main.rs:315` 对 `--game` 调用 `canonicalize()`，得到 `\\?\F:\...` 形式的路径。这个路径同时用作 exe 路径、工作目录和 `LoadLibraryA` 的路径（`tools/harness/src/win32.rs:94`、`win32.rs:103`）。
  - `CreateProcessW` 的命令行为 NULL，而 exe 路径含空格又没有引号。
- **诊断缺口：**
  - 启动失败时，`main.rs:156` 的 `?` 直接返回，不收集 `repentogon.log`；
  - 查找窗口时排除了 `#32770` 对话框（`win32.rs:144`），如果弹出错误框，harness 记录不到。
- **根因仍为 `UNKNOWN`。** 上面的差异是首要嫌疑，未经运行验证。

### 存档对比范围有误

- **静态事实。** REPENTOGON 的 `repentogon/SaveSyncing/SaveSyncing.cpp:52-71` 规定了槽 N 的存档文件：

  | 模式 | 存档数据 | 游戏状态 |
  | --- | --- | --- |
  | 开启 Steam Cloud | 云端 `rgon_steam_persistentgamedataN.dat` | `rep+gamestateN.dat` |
  | 不开云 | `<存档目录>/Repentogon/rgon_persistentgamedataN.dat` | `gamestateN.dat` |

- **直接观察。** `save_before/` 中没有 `Repentogon/rgon_persistentgamedata*.dat`；`save_before/save_backups/` 中有 `20261024.rgon_steam_persistentgamedata{1,2,3}.dat`。据此推断本机是云存档模式（静态推断）。
- **影响：**
  - harness 只对 `Documents\My Games\Binding of Isaac Repentance+\` 做前后比较（`main.rs:323`），看不到第二个存档实际写入的云端文件；
  - E7 中“存档变化只限于第二个存档”无法据此判定；
  - 本轮 `save-changes.json` 为 `[]`，只说明这个目录没有变化。
- **来源与修订：**
  - 实现方案第 7 节第 2 步假设“第二个存档对应 `persistentgamedata2.dat`、`gamestate2.dat`”，这个假设有误。
  - 需要修订：另外只读地快照 Steam `userdata\<id>\250900\remote\`，具体路径在实现前核对。

### 其他发现（不是本次失败的原因）

| 编号 | 位置 | 问题 | 建议 |
| --- | --- | --- | --- |
| A1 | `mod/src/hooks/cs_hook.h:10,17` | 透传 wrapper 全部声明为 `noexcept`，原函数抛出的 C++ 异常经过时会 `std::terminate`。`CallScope` 里对 `uncaught_exceptions` 的检查因此不会生效。游戏是否让异常穿过这些函数为 `UNKNOWN` | wrapper 保持异常中立，只保护自有代码；同步修订方案 R8 |
| A2 | `ui/isaac-cs-ui/main.lua:41-44` | Render 回调每 15 帧发一次 `cs version` 和 `cs status`，每次都经 `Console::Print`。如果 Lua 捕获输出时 Print 仍写入控制台历史（待确认），浮层打开时控制台会被状态文本刷屏 | 降低频率，或改用不经控制台显示的取数方式 |
| A3 | `mod/tools/gen_binding.py:43-44` | `out` 不为空时既 Print 又写入 `out`。REPENTOGON 自己的写法是二选一（`repentogon/Patches/ConsoleHooks.cpp:306-309`），自行显示 `out` 的调用方会看到两份输出 | 改为二选一，同步修订方案 4.7 |
| A4 | `tools/harness/src/main.rs:143`、`:167-176` | 关浮层的 ESC 可能传给游戏：菜单里会回退，run 内会暂停。之后的固定按键序列依据的是开控制台之前的状态 | 关闭浮层后重新读取 `cs status` |
| A5 | `mod/src/ops/command.cpp:130-135` | 每次 `Game::Exit` 都写 `shutdown`，进程真正退出时反而不 flush trace | 区分 run 退出和进程退出 |
| A6 | `mod/src/ops/command.cpp:23-26` | device 组失败也会使状态变为 `Disabled`，导致无法 host | 按设计 4.3 确认 device 组是否参与状态判定 |
| A7 | 其他 | `log_level` 读进来但没有使用；部署清单没有记录写入 `config.json` 这一步；`report.md` 列出了不存在的 `enabled/`、`disabled/` | 下次修改时一并处理 |

### 静态核对一致的部分

- **类型与参数。** 27 个 Hook 的类型和 libzhl 生成的定义一致，包括 static 的 `void (*)(bool)` 和 `void (*)(unsigned int)`。ArgData 只计显式参数，flags 与 `HookSystem_private.h` 的定义一致。
- **RVA。** 27 个 RVA 与 binding JSON 的 v12 目标一致。没有原型映射的 `0x288100`（`Console::RunCommand`）和 `0x28DC60`（`Console::Print`），与 tx-04-03 的 `repentogon_anchor_rva.json` 一致。
- **注册与加载顺序：**
  - 在 ModInit 中按能力组过滤后再注册，避免定义缺失时 `ZHL::Init` 弹框并 `ExitProcess`；
  - 自有定义的 outfunc 存储常驻；
  - 本 DLL 先于 zhlREPENTOGON.dll 加载，实现里在第一帧重新读取版本号。
- **指针补丁。** 在第一帧先核对全部 8 个原值，再整体写入；两张虚表在同一个内存页。
- **命令队列。** 环形队列是有界 MPMC 实现；`host/stop` 在下一帧生效，E4 按 trace 判定恰好下一帧。
- **关闭配置。** 关闭时不注册任何 Hook，E6 的判定与 boot 记录格式一致。

### 建议的下一轮范围

单独作为一次推进：
1. 修正注入：改用普通路径和加引号的命令行；超时时记录 `repentogon.log` 的状态，以及该进程的全部窗口（包括对话框）；
2. 修订实现方案第 7 节和 harness 的存档对比范围；
3. 视情况处理 A1–A4。

全部完成后只执行一次 E2E。

### 方案修订对应

[实现方案](tx-04-04-implementation-plan.md)已按本节修订，修订处都标注了“审阅修订”。修订只依据静态材料，没有运行任何测试。

| 审阅项 | 方案位置 | 处理 |
| --- | --- | --- |
| 注入超时 | 第 7 节第 4 步、F1、R9 | 改用普通路径和带引号的命令行；失败时保留 `repentogon.log` 的前后状态、整屏截图和窗口列表，启动失败时也收集日志 |
| 存档对比范围 | 第 7 节第 1、2 步，E7，F12，R10 | 增加 Steam 云存储目录快照，以及两次启动之间的快照；第一、三个存档按字节判定 |
| 注释 | 第 3 节 | 按 2026-10-24 版 AGENTS.md 改为中文注释，一行一条语句 |
| A1 | 第 5 节、R8 | wrapper 对异常保持中立。补充依据：REPENTOGON 自己会在游戏调用链中抛异常（`SaveSyncing.cpp:204`） |
| A2 | 4.7、第 6 节 | 轮询改为每 60 帧一次，标记只在状态变化时写；Print 在捕获时是否写入历史，待实现时用 IDA 确认 |
| A3 | 4.7 | `out` 为空时 Print，不为空时只写 `out` |
| A4 | 第 7 节第 5 步、R3 | 补充静态事实：关闭浮层的 ESC 不会交给游戏的窗口过程。仍在关闭浮层后截图，并重新读取状态 |
| A5 | 4.6、R11 | 分出 `run_exit`；进程退出时用 `WriteFile` 补写 `shutdown` |
| A6 | 4.4 | 已核对：与设计 4.3 一致（device 组失败即没有会话），不改 |
| A7 | 4.5，第 7 节第 3 步，artifact 清单 | `log_level` 明确为只记录；部署清单记录 `config.json` 的写入；`report.md` 只列实际生成的文件 |
| Q14 观察时机 | 第 6 节、Q14 | 测试 mod 在主菜单用 `MC_MAIN_MENU_RENDER` 调用 `lua-menu`；补充静态旁证，倾向主菜单时 `g_Game` 不为空，仍以 E2E 观察为准 |

修复按修订后的方案实施，编号沿用 tx-04-04（用户决定，设计 D16）。修复完成后执行 tx-04-04 的第二次 E2E，artifact 放在新目录 `artifacts/repentogon/tx-04-04-e2e-02/`。本文上面关于第一次 E2E 的记录原样保留。

## 修订与第二次 E2E（2026-10-24，D16）

本次沿用 tx-04-04，完整目标为落实上面的审阅修订，并执行一次第二轮 E2E。全部代码修订完成前没有运行游戏实验、smoke、probe、单元测试或其他验收；未新增单元测试、gate 或哈希校验。第二次 E2E 结束后只读取既有产物和源码、补充说明，没有再改功能代码、构建或重跑。

### 已完成的修订

| 范围 | 最终行为 |
| --- | --- |
| 启动路径 | `--game` 解析后去除扩展路径前缀；exe、cwd、loader 均使用普通绝对路径；命令行为带引号的 exe；非 ASCII 或达到 MAX_PATH 时拒绝注入 |
| 启动失败诊断 | 启动前后记录 repentogon.log 的大小和修改时间；失败时在终止自有子进程前复制日志、截取虚拟桌面、枚举全部顶层窗口（含不可见窗口和 #32770），记录退出状态；启动失败也走日志收集 |
| 存档范围 | 新增 `tools/harness/src/saves.rs`；从 HKCU SteamPath 发现全部 userdata/*/250900/remote；Documents 与全部云存储均保留 before/mid/after 快照、源文件时间与大小，按两次启动分别直接比较字节；第一、三槽字节改变失败，单纯时间变化只记录；每日备份按修订方案处理 |
| A1 | 入口和设备槽 wrapper 去除 noexcept，保持原生异常中立；自有计数路径不抛出；CallScope 异常时弹栈并记 context_error；新增 FrameScope，在 Manager 异常展开时归还深度且不发布未完成帧快照；控制口工作线程捕获自有异常 |
| A2 | 界面首次渲染和每 60 次渲染查询，状态标记仅在变化时写入；经 IDA 确认 Print 捕获分支不写历史，保留 Print 路径 |
| A3 | `out` 非空只追加 out；为空才调用 Print，不重复输出 |
| A4 | 每次 ESC 关闭浮层后截图并重新查询状态；选档使用新的 menu_id，run 内继续前重新确认 run_active |
| A5 | Game::Exit 记录 run_exit、清空排队变更；正常进程终止时用 Win32 WriteFile 尽力补写剩余队列与 shutdown，不等待线程、不使用 CRT 文件锁或 ZHL |
| A6 | 保留任一能力组（含 device）失败即 Disabled 的设计 |
| A7 | log_level 明确为只记录；每次 config.json 写入进入 deployment.json；report.md 仅列实际存在的产物 |
| Q14 夹具 | MC_MAIN_MENU_RENDER 首次渲染观察 lua-menu；MC_POST_RENDER 首次 run 观察 lua-run；脚本加载阶段另记 phase=load，不代替菜单观察 |
| 注释和产物 | 当前功能模块、核心函数和结构补齐中文注释；C++/Rust 展开压缩语句；artifact 自动复制本次源码、构建日志和 Hook 清单；保留崩溃转储变化记录 |

### IDA 静态复核补充

用户通知服务恢复后，通过已配置的 ida-mcp HTTP 端点发现工具，确认当前 IDB 为 v1.9.7.12、image base `0x400000`。原始反编译/反汇编及说明位于 [静态记录](../../artifacts/repentogon/tx-04-04-revision-static/README.md)。

1. 旧资料与首版代码中的 `Game+0x68158` **有误**。RVA `0x4567F0`（VA `0x8567F0`）的 ExecuteCommand 在 `0x856865` 将返回字符串指针写入 `Game+0x680D8`，在 `0x856870` 清零；类型为 MSVC `std::string*`，即 `Console+0x104`。本次修正 `offsets.h` 和当前方案、设计、binding 说明；上面旧审阅文字保留原样。
2. Print RVA `0x28DC60` 在 VA `0x68DC8D` 读取 `Console+0x104`；非空时追加字符串后，`0x68DCB0` 直接跳到返回处，绕过历史分支。因此无需直接写入捕获字符串。
3. 这些属于 `STATIC_CONFIRMED`；第二次 E2E 未到达 Lua 场景，不能标为运行时通过。

### 构建

纯构建命令为 `pwsh -NoProfile -File scripts/build.ps1`。初次被沙箱阻止访问 MSBuild 的 `C:/Users/114514/AppData/Local/Microsoft SDKs`，未进入编译；获得构建所需访问后构建成功。C++ Win32 DLL 与 Rust i686 harness 均链接成功，SDK 与 naked super 的未使用参数等编译警告保留于 [构建日志](../../artifacts/repentogon/tx-04-04-e2e-02/build-revision.log)。未运行带测试的构建入口，未部署自建 libzhl.dll。

### 唯一一次第二轮 E2E

```powershell
out/repentogon/harness.exe e2e tx-04-04 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out artifacts/repentogon/tx-04-04-e2e-02
```

结果：**FAIL，`Launch timeout (60 seconds)`。** 未自动重跑，第一次 artifact 没有改动。

本次直接观察到：

- 创建的子进程 PID 为 33272；记录中的 exe、cwd 和 DLL 都是普通路径，命令行已加引号。`\\?\` 路径和空命令行这两个首版差异已消除，但不足以解决启动失败。
- [launch.json](../../artifacts/repentogon/tx-04-04-e2e-02/enabled-launch/launch.json) 捕获到可见的 `#32770` 对话框，标题为 **Use The Launcher!**。[整屏截图](../../artifacts/repentogon/tx-04-04-e2e-02/enabled-launch/desktop.bmp) 的正文要求使用 REPENTOGONLauncher，提示直接启动可能破坏存档，并提示已使用启动器时检查版本。
- 等待超时时进程尚未退出（exit_code=null）。repentogon.log 前后均为 772921 字节、相同修改时间，log_changed=false；复制出的日志明确标注为可能早于本次启动，不能冒充本轮运行日志。
- 未恢复游戏主线程到主循环，未发送任何菜单或 gameplay 输入，未产生本轮 boot trace。E1–E6 为 NOT_RUN，E7 为 FAIL；Q14 仍无运行时结论。
- 实际记录的云存储是 `D:/steam/userdata/387111160/250900/remote/`。before→mid、mid→after 的 Documents 与 Steam 文件变化列表均为 `[]`，没有观察到崩溃转储变化。
- 临时 `mods/isaac-cs-test` 已从磁盘移除；最终配置为 enabled=false、control_port.enabled=false。DLL 和界面 mod 保留；未恢复或写入存档。

**失败定位与限制。** 已将无诊断超时缩小到可见的“要求启动器”错误框；截图没有给出模块、调用栈或具体启动器检查条件。SDK loader 的 Launch 第一条语句会重建日志，而本次日志未变化，支持“仍停在 Launch 之前”的静态推断。不能由此断言具体是哪一个初始化分支，也不能认为项目 DLL、Hook、命令桥或界面已经通过。后续需要针对官方启动器的启动约定进一步静态核对并作实质修复；本次不再执行第三轮验收。

本轮 [原始报告](../../artifacts/repentogon/tx-04-04-e2e-02/report.md)、[完整报告](../../artifacts/repentogon/tx-04-04-e2e-02/report.json)、[存档变化](../../artifacts/repentogon/tx-04-04-e2e-02/save-changes.json) 与 sources、deployment、三份存档快照均已保留。第二次启动没有执行，`disabled_or_cleanup` 区间仅覆盖首轮结束后的清理窗口；不能把它当作关闭配置启动的证据。

## 第三次 E2E（2026-10-24，D17）

### 修改与构建

- **修改范围。** 按 D17 修改了 harness 的启动方式，见[实现方案](tx-04-04-implementation-plan.md)第 7 节第 4 步。这次传给子进程的环境项记录在 `launch.json` 的 `environment_added` 中。DLL、界面 mod 和测试 mod 的源码与第二次相同。
- **DLL 重新编译。** `docs/hooks/repentogon-binding.json` 是生成器的输入，补了 `0x680D8` 的更正字段后，DLL 随之重新编译。binding JSON 只影响 Hook 清单里的原型对应，生成的 wrapper 内容没有变化。
- **构建。** 只运行了 `scripts/build.ps1`，C++ DLL 与 Rust harness 都链接成功，日志见 [build-revision.log](../../artifacts/repentogon/tx-04-04-e2e-03/build-revision.log)。

### 执行与结果

```powershell
out/repentogon/harness.exe e2e tx-04-04 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out artifacts/repentogon/tx-04-04-e2e-03
```

结果：**FAIL，`cs status: main_thread_timeout`。** 只执行了这一次，没有自动重跑；前两次的 artifact 没有改动。

| 项目 | 结果 | 说明 |
| --- | --- | --- |
| E1–E6 | NOT_RUN | 场景在等待主菜单的状态轮询中中断；关闭配置的第二次启动没有执行 |
| E7 | FAIL | 正常退出、`Game::Exit` 计数等要求没有完成；存档没有超出允许范围的变化 |
| Q14 | 部分观察 | 见下文“Q14 的运行时观察” |

### 直接观察

依据是本轮 trace `boot-33420.jsonl`（56 条事件）、`launch.json`、`steps.jsonl`、游戏 `log.txt` 和存档快照：

- **注入启动：**
  - `Launch` 返回 0，主线程恢复；
  - 这次启动重写了 `repentogon.log`（772921 → 165715 字节）。
- **ModInit：**
  - boot、config 记录正常；
  - 六个能力组全部 `ok=1`，27 个 Hook 全部注册；
  - 测试控制口在 `127.0.0.1:27091` 启动；
  - boot 记录的 REPENTOGON 版本为 `unknown`（本 DLL 先于 zhlREPENTOGON.dll 加载），第一帧的 `runtime_version` 为 `1.1.3`。
- **Lua 命令桥（脚本加载阶段）：**
  - 测试 mod 的 `cs echo lua-menu`，来源判定为 `lua`，返回 `ok=1`、`echo=lua-menu`；
  - 当时 `g_game` 不为空（`5012DFC0`），`frame=0`；
  - `log.txt` 中有对应记录：`[isaac-cs-test] lua-menu phase=load success=true response=ok=1|echo=lua-menu`。
- **第一帧：**
  - `Manager::Update` 首次调用；
  - 8 个设备槽的原值都和期望一致，已全部写入；
  - 控制口的第一条 `cs status` 在这一帧执行，返回 `ok=1`。读到的是 ModInit 时发布的快照（`frame=0`、`g_game=0`）。
- **之后：**
  - 依次出现 `start_game`、`input_update`、`device_update`、`keyboard_35`、`keyboard_9`、`manager_render` 的首次调用；
  - trace 中没有 `counters` 事件（每 60 帧一次），也没有第二条来自控制口的命令记录；
  - 最后一条事件是进程退出时补写的 `shutdown`。
- **harness：**
  - 第二条 `cs status` 在 5 秒内没有被主线程执行，控制口返回 `main_thread_timeout`；
  - harness 按失败路径关闭自己创建的游戏进程；
  - 场景没有走到 `menu_status` 一步，没有发送任何菜单或游戏按键，也没有打开浮层或进入 run。
- **游戏日志：** `log.txt` 显示两个项目 mod 已加载，随后是开场动画 `playing cutscene 1 (Intro)`，最后是 `Isaac is shutting down...` 和 `Isaac has shut down successfully`。
- **存档：**
  - 启用启动期间（before→mid），以下文件只有修改时间变化、字节相同：`options.ini`、`Repentogon/options.ini`、`Repentogon/vanilla_options.ini`，以及云端的 `rgon_savesyncstatus.json`；
  - `log.txt` 字节有变化，属于允许范围；
  - mid→after 没有变化；
  - 没有崩溃转储变化，第一、三个存档的字节都没有变化。
- **清理：** 临时 `mods/isaac-cs-test` 已移除；`Repentogon/isaac-cs/config.json` 已写回 `enabled=false`、`control_port.enabled=false`；DLL 和界面 mod 保留；没有恢复或写入存档。

### Q14 的运行时观察

- ModInit 时 `g_Game` 为空：第一帧回复的是 ModInit 时的快照，`g_game=0`。
- mod 脚本加载时（第一帧 `Manager::Update` 之前），`g_Game` 已经不为空，`Isaac.ExecuteCommand` 到 `cs` 处理器的路径可用。
- 主菜单渲染阶段（`MC_MAIN_MENU_RENDER`）的观察没有发生，所以“主菜单时 `g_Game` 是否为空”仍没有直接观察。

### 未决

- 主线程为什么没有在 5 秒内处理第二条请求，目前没有依据判断，记为 `UNKNOWN`。
- 这一次不能看作 Hook 计数、命令队列、界面或关闭配置启动已经通过。
- 是否执行第四次 E2E 由用户决定。

本轮的[原始报告](../../artifacts/repentogon/tx-04-04-e2e-03/report.md)、[完整报告](../../artifacts/repentogon/tx-04-04-e2e-03/report.json)、[存档变化](../../artifacts/repentogon/tx-04-04-e2e-03/save-changes.json)、`enabled/`（trace、`repentogon.log`、`log.txt`）、`enabled-launch/`、sources 与三份存档快照均已保留。

## 第三次超时的分析与修订（2026-10-24）

用户要求了解第三次结果、分析并尝试修复。本次继续处理 tx-04-04 的启动与命令桥，没有增加 tick、网络或输入接管。全部分析与修改期间未运行游戏、probe、单元测试或其他验收。

### 证据与判断

- 第三次的 Launch=0、Lua 加载阶段 echo 成功、27 个注册与 8 个槽安装，已经排除前两次“启动器标记缺失”的阻塞；不能把第三次继续归因为注入失败。
- 控制口所有命令仍只在 Manager::Update 入口执行。harness 在获取窗口后直接连接和轮询，第一次调用 focus 在后面的输入/截图路径，启动等待没有焦点保证；第三次快照的原生选项是 PauseOnFocusLost=1、SkipIntro=0。这是明确的时序缺口，失焦是否为第三次根因仍是 STATIC_INFERENCE，不能标为运行时确认。
- 原 harness 只要收到一次 main_thread_timeout 就立刻结束；后面的 100 次、每次 150 ms 的轮询没有机会继续，而且循环耗尽后也没有检查真正到达菜单。启动加载/动画中暂时没有下一主循环边界会被当作整个功能失败。
- 第三次没有请求入队时间、执行等待、实际响应耗时和关闭前窗口状态。trace 里第一条端口 command 的 t=6690283909，shutdown 的 t=6690570634，相差约 0.287 秒；因此不能仅由错误名和源码中的 5000 ms 推导本轮实际等待了五秒。计时差异的原因仍为 UNKNOWN，本次同时记录 harness 单调时间、DLL QPC 事件时间和系统毫秒。
- IDA 确认当前数据库为 v1.9.7.12、image base=0x400000；IsaacMain 在 RVA 0x50AF08、0x50AF0D 依次调用 Update 和 Render。原生代码没有提供“只要 Intro 播放就一定停止 Update”的证据。本次不把 cutscene 直接判作根因，也不改原生动画、选项或游戏时钟。

### 候选修复

1. harness 在连接/查询前获取并确认游戏焦点，启动轮询期间持续检查；步骤中记录窗口是否前台和是否最小化。不改用户的 PauseOnFocusLost 或 SkipIntro 配置。
2. 使用场景中的 startup_timeout_seconds（当前 90 秒）等待真实菜单 id=1/2；只对启动阶段、已经完整接收的 status/main_thread_timeout 继续查询。传输错误、其它错误和后续场景仍立即失败，host/stop 不自动重发。不盲发 ENTER 跳过未知画面。
3. 关闭配置启动不再固定睡 12 秒就宣称到达菜单：必须看到本次重写的 log.txt 中出现 MC_MAIN_MENU_RENDER 夹具的 phase=menu 标记，再截图和判定 E6。
4. PortRequest 携带入队时间和截止时间；主线程在执行前丢弃已过期的请求，避免积压请求在恢复后迟到执行。变更仍在原来的下一次 Manager 边界执行，网络线程不访问游戏对象。截止检查不构成“超时响应和执行之间完全无竞争”的新承诺。
5. 新增 control.jsonl 记录每条请求、完整响应、传输错误与真实耗时；DLL 新增 control_request/control_execute/control_reply/control_expired、前三帧的 begin/end、每个 Hook 的 first_return 和 completed 计数。
6. 场景失败时先保存 enabled-failure/ 或 disabled-failure/ 的整屏截图、全部窗口（包括前台/最小化状态）、退出状态与错误，再关闭自有进程，不让清理抹掉现场。

### 构建与验收安排

纯构建中第一次 C++ DLL 编译成功，Rust 诊断字段直接序列化 Result 导致 E0277；修正为显式 exit_code/error 字段后，DLL 与 i686 harness 均构建成功。两次构建日志保存在 [分析产物目录](../../artifacts/repentogon/tx-04-04-timeout-static/README.md)。未写单元测试，未部署自建 SDK DLL，未校验哈希。

用户在本会话明确同意“执行一次第四轮 E2E”。只执行一次，产物使用新的 `artifacts/repentogon/tx-04-04-e2e-04/`，不自动重跑；前三次记录和产物保留。

## 第四次 E2E（2026-10-24，本会话明确授权）

执行前已完成上节候选修复及纯构建，用户明确同意“执行一次第四轮 E2E”。唯一一次执行使用 `artifacts/repentogon/tx-04-04-e2e-04/`，修订后的功能代码在本次执行后没有再修改。

结果：**FAIL，startup menu timeout after 90s**。原始 [report.json](../../artifacts/repentogon/tx-04-04-e2e-04/report.json)、[report.md](../../artifacts/repentogon/tx-04-04-e2e-04/report.md) 保留。

直接观察：

- Launch=0；PID=28232；六组能力通过，27 个注册、8 个槽正常安装。
- [control.jsonl](../../artifacts/repentogon/tx-04-04-e2e-04/control.jsonl) 有 510 条完整响应，全部 ok=1，无 main_thread_timeout 或传输错误。第一条等待 4297 ms，第二条 17 ms；第二条返回 frame=8，已越过第三次的具体失败位置。
- 最后返回 frame=5102、g_game=1、pointers=8、registered=27；queue_dropped、trace_dropped、foreign、context_faults 都为 0。first_return、前三帧 begin/end 与 counters 均有记录。
- menu_id 和 selected_save 一直是 -1。90 秒后才判定失败，未发送菜单/gameplay 按键，也未进入 run。
- [关闭前现场](../../artifacts/repentogon/tx-04-04-e2e-04/enabled-failure/state.json) 的 GLFW 窗口是前台、未最小化；[整屏截图](../../artifacts/repentogon/tx-04-04-e2e-04/enabled-failure/desktop.bmp) 直接显示仍处于 Intro 动画。不能把这次菜单未出现解释为主线程完全卡死。
- failure_close.exit_code=0，游戏日志正常关闭；E1–E6 仍是 NOT_RUN、E7 为 FAIL，因为没有执行 run、Game::Exit 和关闭配置启动等场景。不能把启动观察提升为全轮通过。
- [文件变化](../../artifacts/repentogon/tx-04-04-e2e-04/save-changes.json) 全部在允许范围内。log.txt、options.ini 与 Repentogon/vanilla_options.ini 字节变化；选项变化包括窗口位置；另有选项、imgui 与云同步状态文件仅修改时间变化。各槽存档字节没有变化，无崩溃转储变化，mid→after 无变化。
- 临时夹具已从磁盘移除，最终配置 enabled=false、control_port.enabled=false，未恢复存档。

结论：本轮没有复现第三次的状态请求超时，但不能由一次运行证明其根因或普遍修复。当前失败是开场动画仍未跳过而达到菜单等待上限；过期丢弃、启动阶段暂时超时重询、关闭配置菜单确认等新分支尚未在本轮实际覆盖。

### 用户说明与后续授权

第四次结束后，用户明确说明“是我忘记跳过动画了”，要求“重新开始测试”，并要求“下次测试前先告诉我我需要做什么”。本次重测属于用户明确授权，不是 harness 自动重跑，也不是另换编号增加自动验收次数。

已在启动第五次之前告知：本轮启用/关闭配置各启动一次；两次都请用户跳过动画，到标题页或选档页后停止手动操作，保持游戏在前台，由 harness 接手。该提醒要求同步到现行规范第 8 节与 AGENTS.md。第五次使用新目录 `artifacts/repentogon/tx-04-04-e2e-05/`，不改功能代码、不重新构建；结果另记。

## 第五次 E2E（2026-10-24，用户要求重新开始）

在用户说明第四次忘记跳过动画并明确要求重测后，使用已编译的相同代码执行一次 `artifacts/repentogon/tx-04-04-e2e-05/`。启动前已说明两次都需要跳动画，第一次 startup_focus 后再次提醒；没有修改功能代码或重新构建。

结果仍是 **FAIL，startup menu timeout after 90s**，见 [report.json](../../artifacts/repentogon/tx-04-04-e2e-05/report.json)。PID=35116；514 条状态响应全部 ok=1，无 main_thread_timeout；末次 frame=5128、menu_id=-1、g_game=1、registered=27、pointers=8，drops/foreign/context_faults 均为 0。[失败现场](../../artifacts/repentogon/tx-04-04-e2e-05/enabled-failure/desktop.bmp) 仍显示 Intro 动画，窗口为前台，未进入菜单或 run，关闭配置启动未执行；failure_close.exit_code=0。

前后只有 log.txt 字节变化，选项/imgui/云同步状态仅修改时间变化，各槽存档字节不变，无崩溃转储；mid→after 无变化。临时夹具已从磁盘移除，最终配置已关闭，未恢复存档。原始 FAIL 与文件都保留。

第五次后，用户明确回复“没来得及操作”。因此没有把本轮记为按键不生效或 DLL 卡死，也没有自动再次启动；已改为先等待用户确认准备好，再进行下一次明确授权的实机运行。每次启动仍分别提醒用户跳过动画，看到标题/选档页后停止操作。

## 第六次 E2E（2026-10-24，用户确认准备好后重测）

本次先等待用户明确回复“已准备好，重新开始一次”，再启动 `artifacts/repentogon/tx-04-04-e2e-06/`。启动前说明两次启动都需跳过动画，第一次窗口出现后再次提示。与第四、五次使用同一已编译版本，没有修改功能代码或重新构建。第二次启动未发生，所以没有发出不存在的第二次动画提醒。

### 结果与已观察行为

整体 **FAIL**，具体错误为 `status timeout run_active=0`。原始 [report.md](../../artifacts/repentogon/tx-04-04-e2e-06/report.md)、[report.json](../../artifacts/repentogon/tx-04-04-e2e-06/report.json) 原样保留。

| 项目 | 原始报告 | 实际依据与限制 |
| --- | --- | --- |
| E1 加载 | PASS | Launch=0、六能力组通过，已到主菜单后进入 run |
| E2 Hook | PASS | 27 个注册、8 个设备槽、必须非零集合齐全，foreign/context_faults/queue_dropped/trace_dropped 均为 0 |
| E3 命令桥 | NOT_RUN | 最终日志判定在退菜单失败后没有执行；已有 console-menu、console-run、port-run 的正确 trace，也有 Lua 的 load/menu 回应，但没有 lua-run 回应记录，不能标为全项通过 |
| E4 命令队列 | PASS | 原始报告先由状态轮询判定。事后只读 trace 进一步确认：host 在 frame=1603 入队、1604 变为 Hosting；stop 在 1615 入队、1616 变为 Idle。没有重跑 |
| E5 界面 | NOT_RUN | 最终判定未执行；log.txt 已有 window-render/state=Idle 标记并有浮层截图，只作为已有观察 |
| E6 关闭配置启动 | NOT_RUN | 第一轮退菜单失败，第二次启动没有执行 |
| E7 退出与副作用 | FAIL | 未正常退回菜单，未完成第二次启动/日志对比，且有允许范围外的存档字节变化 |

本次已经成功跳过动画。harness 明确观察 selected_save=1 后选择第二槽，观察菜单 3 的 New Run=0 后进入角色菜单及 run；没有进入原版 Online。此前第三次的 main_thread_timeout 在第四、五、六次都没有复现；这支持候选修复后的可用性，但不能据此单独确认第三次根因。

**Q14 的直接观察已补齐。** 在主菜单阶段，status 的 g_game=1，`[isaac-cs-test] lua-menu phase=menu success=true response=ok=1|echo=lua-menu` 已写入 log.txt。这是 MC_MAIN_MENU_RENDER 的实际回调，不再只是脚本加载时观察。主菜单命令桥可用，但 lua-run 夹具结果仍缺失。

### 失败位置

退菜单的固定序列是 ESC、DOWN、DOWN、ENTER。等待 run_active=0 超时；[关闭前截图](../../artifacts/repentogon/tx-04-04-e2e-06/enabled-failure/desktop.bmp) 显示处于 **Options** 界面，非主菜单，窗口为前台且未最小化，最后 status 的 run_active=1、frame=3188。说明本次固定按键序列没有选中退出 run；不能再归因为用户未跳动画或主线程停止响应。下一次应静态核对暂停菜单选择与返回层级，按实际状态导航，不能直接重复这条序列。

失败清理的 WM_CLOSE 返回 exit_code=0，Game::Exit/run_exit 在关闭期间出现；这不等于“退回主菜单时已通过 Game::Exit 计数检查”。没有自动重跑。

### 存档变化

[save-changes.json](../../artifacts/repentogon/tx-04-04-e2e-06/save-changes.json) 保留 before→mid、mid→after 两个区间。后一个没有变化，前一个包含：

- 允许范围内的第二槽原版/REPENTOGON 存档、gamestate2、第二槽每日备份，以及日志/控制台历史/选项/ImGui 文件变化；
- **不在允许范围内**的云端 `rgon_steam_persistentgamedata1.dat`、`rgon_steam_persistentgamedata3.dat`，以及两个已有的第一、三槽每日备份字节变化；
- **不在现行允许清单内**的第二槽 REPENTOGON 扩展状态文件：EntitySaveStateManagement 的 ex_remote_gamestate2.json、remote_gamestate2.dat，ItemPoolManager/VirtualRoomSetManager 的 remote_gamestate2.json。

第一、三槽变化的具体内容和来源尚未归因，不能仅因使用第二槽就断言其他槽未变；也不能把扩展文件自动补进允许清单来改写本轮结果。前后全部备份已保留，不自动恢复，以免与 Steam Cloud 冲突。没有崩溃转储变化。

临时 `mods/isaac-cs-test` 已从磁盘移除，配置已写回 enabled=false、control_port.enabled=false。DLL 和纯界面 mod 保留。后续实机测试前继续先告知用户配合事项，并在需要手动跳动画时先确认用户已经准备好。

## 第六次退菜单失败后的修订（2026-10-24）

用户要求先修复进入 Options 的返回菜单按键，再开始下一次测试。对目标 IDB 的静态复核确认：PauseScreen::Show 默认 state=1、selection=1；选择编号 0=Options、1=继续、2=退出，向下在 0..2 循环。因此固定 ESC、DOWN、DOWN、ENTER 从默认 1 落到 0，正好解释第六次截图。原始反编译与点位说明见 [暂停菜单静态依据](../../artifacts/repentogon/tx-04-04-pause-static/README.md)。

已用状态导航替换固定序列：主线程复制 pause_state/pause_selection；harness 根据 state=0/1/2 决定打开暂停或离开 Options，在 state=1 读取光标，从 1 按一次 DOWN、从 0 按一次 UP、从 2 直接继续；观察 selection=2 后才确认。未知状态、光标未达到预期直接失败，不重复输入。增加每步状态、退出选项截图和返回菜单截图，保留 run_active=0 与 Game::Exit 计数检查。没有修改 E3/E7 判定或存档允许清单。

本修订的 DLL 与 Rust harness 已纯构建成功，日志为 out/repentogon/build-revision.log；随后用户明确确认“已准备好，开始测试”，执行第七次整体 E2E，见下节。分析和修改期间未进行其他测试。旧一至六次结果均保持原样。

## 第七次 E2E（2026-10-24，暂停菜单状态导航修订后）

用户要求先修复退菜单进入 Options 的问题，再开始下一次测试。修订纯构建成功后，先说明两次启动均需跳过动画，到标题或选档页停止手动操作；用户确认准备好后才开始。两次启动分别提醒。本轮仅执行一次统一入口，未重跑。

### 结果

整体 **FAIL**，但本次指定的退菜单问题已在实机修复。原始 [report.md](../../artifacts/repentogon/tx-04-04-e2e-07/report.md)、[report.json](../../artifacts/repentogon/tx-04-04-e2e-07/report.json)、[steps.jsonl](../../artifacts/repentogon/tx-04-04-e2e-07/steps.jsonl) 保留。场景本身走完，failure=null；整体失败来自最终 E3/E5/E7 判定，不能把 failure=null 解读为整体通过。

| 项目 | 结果 | 依据 |
| --- | --- | --- |
| E1 加载 | PASS | 启用进程 PID 28340，Launch=0，六能力组通过并进入第二槽 run |
| E2 Hook | PASS | 27 个注册、8 个设备槽，必须非零集合齐全；foreign/context_faults/queue_dropped/trace_dropped 均为 0 |
| E3 命令桥 | FAIL | 控制台菜单/run、端口 run、Lua load/menu 均有正确响应；缺少 lua-run 的日志及 trace，不能标为全项通过 |
| E4 命令队列 | PASS | host 在 1514 入队、1515 执行；stop 在 1526 入队、1527 执行，exact_next_frame=true |
| E5 界面 | FAIL | 控制台截图存在，UI 脚本加载；本轮没有 window-render 标记，未证明面板实际渲染。不能沿用第六次的标记 |
| E6 关闭配置启动 | PASS | PID 34208，实际 MC_MAIN_MENU_RENDER 标记与 disabled-menu.bmp，只有 enabled=0/registered=0/pointers=0 的 boot 事件，正常退出 |
| E7 退出与副作用 | FAIL | 两次 exit_code=0，退菜单时 Game::Exit 增长，零新增 REPENTOGON ERROR/FATAL、零崩溃转储变化；但存在允许范围外存档字节变化 |

### 退菜单直接证据

原始 steps.jsonl 中的路径是：

1. frame=1539，run_active=1、pause_state=0、pause_selection=1；
2. ESC 后 frame=1630，pause_state=1、selection=1；
3. 一次 DOWN 后 frame=1721，pause_state=1、selection=2；[退出选项截图](../../artifacts/repentogon/tx-04-04-e2e-07/pause-exit-selected.bmp) 清楚显示箭头位于 EXIT GAME；
4. ENTER 后 frame=1802，run_active=0、menu_id=3；[返回截图](../../artifacts/repentogon/tx-04-04-e2e-07/returned-menu.bmp) 显示主菜单，光标为 CONTINUE。

Game::Exit 在 frame=1747 的正常退菜单过程中产生 run_exit，计数从 0 增至 1，早于关闭进程；此次不依赖失败清理的 WM_CLOSE 伪装退菜单成功。关闭后再启动禁用配置阶段，两次进程均返回 0。新的只读暂停字段由主线程复制，没有增加游戏对象写入或网络线程游戏访问。

### 未解决项与存档变化

E3 的 lua-run 缺失与上一轮一致。当前测试夹具在 MC_POST_RENDER 中调用 `pcall(Isaac.GetNumPlayers)`，失败直接返回而无诊断；SDK 自带 Lua 脚本使用 `Game():GetNumPlayers()`。这是后续应静态核对的夹具 API 路径，本轮没有在验收后修改代码或补跑。E5 只表明缺少 UI Render 回调标记；harness 的控制台截图步骤没有确保 Session panel 真正渲染，不能据此直接判定 DLL/UI 崩溃。后续应核对浮层显示状态和操作覆盖。

[save-changes.json](../../artifacts/repentogon/tx-04-04-e2e-07/save-changes.json) 分开保留两个区间，所有 before/mid/after 原文件均备份：

- 启用启动 before→mid：第一、三槽的 Steam `rgon_steam_persistentgamedata{1,3}.dat` 及对应当日日备份字节变化；第二槽四个 REPENTOGON 扩展文件也变化且不在现行允许清单内。第二槽普通存档、日志/历史/ImGui 等允许文件有变化。
- 关闭配置启动 mid→after：第一、三槽的同类 Steam 存档及日备份**再次**字节变化；第二槽 REPENTOGON 存档与日备份变化，日志改写。关闭配置进程没有注册本项目 Hook/设备槽，这说明本轮第一、三槽变化也出现在未启用 C/S 的启动区间；具体来源、字段与是否只是格式重写仍 UNKNOWN，不能据此豁免 E7。

没有扩大存档允许清单，没有自动恢复存档。cleanup_error=null；临时测试 mod 已移除，最终配置 enabled=false、control_port.enabled=false，DLL 与纯界面 mod 保留。后续测试仍先等待用户准备好，失败不自动重跑。

## 第七次 E2E 的 E3/E5 失败分析与 D18 修正

本节只读了第七次 E2E 的 artifact、截图和 REPENTOGON 源码，没有运行程序。

**两个测试 mod 都已加载（直接观察）。** `enabled/log.txt` 第 77–80 行加载了 `isaac-cs-test` 和 `isaac-cs-ui`，第 105–107 行两个 `main.lua` 都已执行且没有 Lua 报错；测试 mod 在加载和主菜单阶段都记录了 `lua-menu`；控制台截图的菜单栏中有界面 mod 创建的 “Isaac CS” 菜单。

**`lua-run` 缺失（静态推断）。** 测试 mod 在 `MC_POST_RENDER` 中用 `pcall(Isaac.GetNumPlayers)` 判断是否已有玩家。`GetNumPlayers` 是 `Game` 对象的方法：REPENTOGON 脚本 `onlinestub.lua:27` 写作 `Game():GetNumPlayers()`，EID 也用 `game:GetNumPlayers()`；REPENTOGON 的源码和脚本中都没有 `Isaac.GetNumPlayers`。`pcall` 每帧失败，回调提前返回，所以从首版起 `lua-run` 都不会记录（第六次同样没有）。

**界面渲染标记缺失（静态事实＋直接观察）：**
- REPENTOGON 把自定义窗口可见性存进 `<存档目录>\Repentogon\imgui.ini`（`ImGui.cpp:165-173`），第一帧 ImGui 时用 `window->SetVisible(settings->Visible)` 恢复（`ImGui.cpp:176-198`），这一步晚于 mod 脚本，覆盖了界面 mod 的 `SetVisible(true)`。
- 第六次运行前 `imgui.ini` 中没有 Isaac CS 窗口，第六次有 `window-render` 标记；第六次启用启动结束后（`save_mid`）已存为 `Visible=0`；第七次以此开始，窗口未显示，trace 中也没有界面发出的 Lua `cs version/status`，说明 Render 回调一次都没执行。
- 第六次为什么存成 0，目前 `UNKNOWN`。

**E7（直接观察）。** 不允许的变化有两类：第二槽的 REPENTOGON 附属文件（`Repentogon/EntitySaveStateManagement/remote_gamestate2.dat`、`ex_remote_gamestate2.json`，`ItemPoolManager/remote_gamestate2.json`，`VirtualRoomSetManager/remote_gamestate2.json`）不在允许清单；第一、三槽的云端文件及当日备份在两次启动中都有字节变化。

**D18 修正（用户决定）：**

| 项 | 修改 |
| --- | --- |
| 测试 mod 的 run 判断 | 改用 `Game():GetNumPlayers()`，仍在 `pcall` 内 |
| 界面窗口 | 测试 mod 在每次主菜单渲染时 `ImGui.SetVisible("isaac-cs-panel", true)`，记一次 `panel-visible`；界面 mod 本身不改，继续尊重用户保存的可见性 |
| 第二槽附属文件 | `Repentogon\` 子目录下的 `remote_gamestate2.*`、`ex_remote_gamestate2.json` 列入允许范围 |
| 第一、三槽 | 基线对比：关闭配置启动中的变化记为 REPENTOGON 基线；启用启动中同一文件也在关闭配置启动中变化则放行；只在启用启动中变化的仍判 F12；每条标注 `baseline` |
| 开场动画 | 用户自行开启 REPENTOGON `[VanillaTweaks] SkipIntro`；harness 只读检查，不为 1 时在注入前结束 |

修改见实现方案第 6 节、第 7 节第 1、2 步与 R10、R12。修正后再执行一次 E2E，不自动重跑。

## 第八次 E2E（2026-10-24，D18）

### 执行

- **执行前：**
  - 只读确认 `[VanillaTweaks] SkipIntro = 1`（用户自行开启）；
  - 没有游戏进程在运行；
  - 构建只运行了 `scripts/build.ps1`。
- **命令：**

  ```powershell
  out/repentogon/harness.exe e2e tx-04-04 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out artifacts/repentogon/tx-04-04-e2e-08
  ```

- **结果：PASS，E1–E7 全部通过。** 只执行了一次；`failure=null`，`cleanup_error=null`。见[原始报告](../../artifacts/repentogon/tx-04-04-e2e-08/report.md)和[完整报告](../../artifacts/repentogon/tx-04-04-e2e-08/report.json)。

### 各项依据（直接观察）

| 项 | 依据 |
| --- | --- |
| E1 加载 | 六个能力组全部为 1 |
| E2 Hook | `registered=27`、`pointers=8`；“必须非零”的计数全部大于 0；非主线程透传 0，`trace_dropped=0`，`context_faults=0` |
| E3 命令桥 | trace 中四个来源都有对应命令和正确输出：`console-menu`、`console-run`（console），`lua-run`（lua），`port-run`（port）。`log.txt` 第 461 行：`[isaac-cs-test] lua-run phase=run success=true response=ok=1\|echo=lua-run` |
| E4 命令队列 | `cs host` 入队后，trace 中的状态迁移恰好发生在下一帧（`exact_next_frame=true`），状态变为 `Hosting`；`cs stop` 后回到 `Idle` |
| E5 界面 | `log.txt` 第 380 行 `panel-visible phase=menu success=true`，第 382 行 `[isaac-cs-ui] window-render`，第 383 行 `status state=Idle`；trace 中有界面经 Lua 发出的 `cs version`、`cs status` 各 4 次；截图 `console-menu.bmp` |
| E6 关闭配置 | 第二次启动的 trace 只有一条 boot：`enabled=0 registered=0 pointers=0 reason=config_disabled`；退出码 0 |
| E7 退出与副作用 | 两次启动退出码都是 0；`Game::Exit` 计数 0→1；没有崩溃转储变化；REPENTOGON 日志没有新增错误。存档变化见下 |

**存档变化：**
- **启用启动：** 第二个存档的云端文件（`rgon_steam_persistentgamedata2.dat`、`rep+persistentgamedata2.dat`、`rep+gamestate2.dat`）、四个 REPENTOGON 附属文件和当日备份发生变化，都在允许范围内。
- **第一、三个存档：** 云端文件和当日备份在两次启动中都有字节变化，按 D18 记为 REPENTOGON 基线。每条都有标注：启用启动中的标为 `also changed in disabled launch`，关闭配置启动中的标为 `disabled launch; no hooks registered`。

**Q14（运行时观察，本环境）：** 主菜单时 `cs status` 的 `g_game=1`；`MC_MAIN_MENU_RENDER` 中 `lua-menu phase=menu` 返回 `ok=1`。也就是说，主菜单时 `g_Game` 不为空，Lua 到命令桥的路径在主菜单可用。

**计数快照**（`counters.json`，在退回菜单之前采集）：

| 点位 | 次数 |
| --- | --- |
| `manager_update`、`input_update`、`device_update`、`keyboard_35`、`start_game` | 1425 |
| `game_update`、`odd_helper` | 397 |
| `room_update`、`entity_list` | 258 |
| `post_update` | 257 |
| `player_update` 和六个 consumer | 各 605 |
| `join_pass` | 695 |
| `console_command` | 13 |
| `new_room`、`post_start` | 各 1 |
| `door_update`、`door_nested`、`room_lifecycle`、`restore`、Gamepad 各槽 | 0（方案允许为 0） |

### 观察与限制

- **`start_game` 的计数与 `manager_update` 相同（1425）。** 说明 `Manager::ExecuteStartGame` 在这个版本里每帧都会被调用。它属于“必须非零”集合，本轮判定没有问题；但它作为外层 owner 的语义，要在 tx-04-05 做 tick 判定前复核。
- **样本范围。** 这是本机、本环境下的一次样本。本轮的 Hook 都只透传和计数，不涉及 tick、输入接管或网络（属于 tx-04-05 及以后）。
- **持久设置。** `SkipIntro=1` 是用户自己开启的 REPENTOGON 设置，会持续生效。
- **清理。** 临时测试 mod 已移除；配置已写回 `enabled=false`、`control_port.enabled=false`；DLL 与界面 mod 保留；没有恢复或写入存档。

### 结论

在 D16–D18 的用户决定下，tx-04-04 的验收目标由第八次 E2E 达成。第一到第七次的失败记录和 artifact 原样保留。
