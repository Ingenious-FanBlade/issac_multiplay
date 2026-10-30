> **tx-04-10 当前交付**：[实现、成员目录与整体验收结果](../docs/design/observation-expansion/tx-04-10-implementation.md)。构建及统一 E2E 5/5通过。安装 DLL 时同时部署 `mod/observation/isaac-cs-observation` 到游戏 `mods/isaac-cs-observation`；此正式模块独立于 UI 和测试夹具。`cs observe start|status|stop` 管正式纯值消费，`cs world start <label>|status|stop` 管可选录制；录制关闭／失败不停止仍有需求的采集。新记录写入各录制的 `observation-v2/`，离线入口 `harness observation-read --capture <目录>`。完整 Grid 待 tx-04-11；下方旧 tx 记录按历史理解。

# tx-04-09 四参与者与多身体

四参与者、多身体控制组与ICSN v2已通过统一功能整体验收；性能开销、原生限制、失败修复与操作入口见[实现说明](../docs/design/observation-expansion/four-participant-implementation.md)。下方历史记录保持原结果。

生产默认关闭世界录制和详细诊断；`diagnostics.enabled`与`world_record.enabled`须显式配置才启用，逐consumer重复全图核对还须单独开启`diagnostics.repeat_topology_reads`。稳定窗口使用安全拓扑复用；E2E支持Diagnostic／Production配置并已续测通过，四参与者／七身体生产短窗口约60 FPS。Rust harness与`tests/e2e-mods`不属于正式模组内容。

# REPENTOGON mod

2026-10-27 tx-04-08 layout/Hook 清单与生成器（工程顺序中的清单项）：唯一清单 `binding/hooks.json` 升级为 schema 2——41 条既有行保持不变，新增 19 项 `world_observations`（逐参数 `stack:N`／寄存器 ABI、完成含义、SDK `flags/argdata` 交叉校验）；新增 `binding/world_layout.json` 登记 78 项字段偏移与证据、15 个来源、10 个 RTTI 类与目标版本常量。`tools/gen_binding.py` 生成 wrapper、`hook_capacity.h`、`world_layout.inc`、`hook-manifest.json/md` 与 `world-layout.json`，生成期拒绝 ABI/容量/字段/几何关系不一致；待注册 **60/64**（既有 41 + world 19），注册期超限也会显式报告而不是静默丢弃。`src/binding/world_layout.*` 提供只读访问与编译期一致性校验，`src/world/observe.*` 提供不进入会话失败路径的 world 观察策略；world 组当前**不安装**（`WorldObservationArmed()` 固定 false），待主线程桥接入配置后启用。**只做了一次临时目录生成与纯编译（`/W4` 无项目警告），没有运行测试、没有启动游戏**；DLL 整体链接仍被既有 `libzhl/ASMPatcher.hpp:302` 的 `friend static` 与当前 C++20 组合挡住，并已确认 `host_patch.cpp` 不能整体降级到 C++17，留给“必要纯编译链接”项。见[实现说明](../docs/design/tx-04-08/tx-04-08-layout-hook-manifest.md)。下方保留历史状态。

2026-10-27 tx-04-08 world 核心（第一项工程改动）：新增独立静态库 `isaac_cs_world`（`src/world/model|identity|storage|writer`，C++20、不依赖 SDK/游戏头），实现纯值模型与 78 项 profile、33 位 radix 本地身份、三槽所有权状态机、64 MiB/64 项双上限值队列、gap 控制环、`isaac.cs.world-record` v1 的 manifest/captures/lifecycle-events/diagnostics/result 编码与 reader/writer 两个后台线程的有界收口。**只做了一次纯编译（`/W4` 无警告），没有运行测试、没有启动游戏**；DLL 整体链接被既有 `libzhl/ASMPatcher.hpp:302` 的 `friend static` 与当前 MSVC C++20 组合挡住（既有 `hooks/odd_patch.cpp`、`host_patch.cpp` 受影响，非本次改动引入），留给“必要纯编译链接”项。主线程桥、命令、离线工具、harness 与整体 E2E 仍未实现；见[实现说明](../docs/design/tx-04-08/tx-04-08-world-core.md)与[设计](../docs/design/tx-04-08/tx-04-08-implementation-plan.md)。下方保留历史状态。

tx-04-07 已完成（用户正式接受，D31，2026-10-26）：e2e-11 整体 E2E 验收正式通过。原自动报告及失败/未执行事实不改写，不再补测。见[验收记录](../artifacts/repentogon/tx-04-07-e2e-11/acceptance.md)。下方保留历史状态。

tx-04-07 e2e-11：V12实际PASS，恢复暂停与实时激光松键归属均有证据；整体仍因S17移动净空选择停止而FAIL。S0–S16完成、环境恢复，无自动重跑。见[结果](../artifacts/repentogon/tx-04-07-e2e-11/README.md)与[V12修复](../docs/design/tx-04-07-v12-recovery-fix.md)。

tx-04-07 e2e-09：debug 10 控制台启用，S20 Continue 已走通，S0–S20 完成；S21 暂停等待超时，整体仍 FAIL。前三轮严格网络对账通过，V12 输入空档/早期激光与部分采证问题仍在。游戏正常退出、环境恢复，无自动重跑，见[记录](../artifacts/repentogon/tx-04-07-e2e-09/README.md)。

tx-04-07 e2e-08（2026-10-26）：单启动已走到 S20；第二轮收尾及前三轮严格对账通过，V12 仍有重绑空档和提前发射；S20 因 A 死亡、无法 Continue 而失败。游戏正常退出、环境恢复，见[结果](../artifacts/repentogon/tx-04-07-e2e-08/README.md)。用户要求下次开局 debug 10，已修改并纯构建 harness，尚未实测；下方保留历史结果。

tx-04-07 修复编写（2026-10-26）：native Worker 逻辑退休后增加 100 ms 有界收包与 terminal_rx 诊断，relay 单列 UDP 重置通知；V12 采证已修正。统一纯构建成功，没有部署或新 E2E，详见[说明与限制](../docs/design/tx-04-07-relay-v12-fix.md)。

tx-04-07 最新e2e-06（2026-10-26）：S6等待回归修复，S0–S15已执行，V9/V11通过；第二轮Bye后UDP中继报10054，整体仍FAIL。V12缺激光证据，第三/四轮未执行。游戏正常退出、无新dump、环境恢复，无自动重跑。见[结果](../docs/design/tx-04-07-results.md)与[修复说明](../docs/design/tx-04-07-pause-wait-fix.md)。下方保留原时点。

tx-04-07 最新 E2E（2026-10-26）：e2e-05 **FAIL**，S6 退出预期暂停时新增 harness 检查过早终止；DLL/Lua 未修改，本轮没有新增崩溃，环境已清理。焦点恢复未获得实机样本，完整场景仍未完成。见[结果](../docs/design/tx-04-07-results.md)与[修改/回归说明](../docs/design/tx-04-07-focus-fix.md)。下方保留历史时点。

tx-04-07 退休修复后第三次 E2E（2026-10-25）：**FAIL**。RemoveCoPlayer 已改传 Game 内嵌 PlayerManager，S13 Stop 及失败收尾的第二轮 B 均正常退休，无新增崩溃。S0–S14 完成，随后因 Lua 夹具把 Entity Index 当作 ordinal 导致 b_animation_timeout；未自动重跑，环境已清理。见[修复](../docs/design/tx-04-07-retirement-fix.md)与[结果](../docs/design/tx-04-07-results.md)。下方保留各次原始记录。

tx-04-07 实质修复后第二次 E2E（2026-10-25）：**FAIL**。启动 trace 丢失未重现，S0–S12 完成、V11 死亡/复活 PASS；S13 `cs stop` 的退休/原生 B 移除流程崩溃（0xC0000005），根因尚未确定，未自动重跑。测试 mod/C/S 配置已清理，Download Helper 恢复。见[修复](../docs/design/tx-04-07-trace-fix.md)与[结果](../docs/design/tx-04-07-results.md)。下方首次失败记录保持原样。

tx-04-07 唯一整体 E2E 已执行（2026-10-25，America/Los_Angeles），结果 **FAIL**：启动 trace 丢失 63 条导致首轮开局 trace_event_lost 退休，尚未创建 K/Join B，未重跑。游戏已正常退出、测试 mod/C/S 配置已清理，Download Helper 恢复。见[结果](../docs/design/tx-04-07-results.md)；下方未测试说明保留编码时点。

2026-10-26 tx-04-07 3.4 harness：relay、远端换代/超时、四轮 S0–S21、V1–V16 与文件窗口已编写；网络层配对补只读终态发布时间。3.1–3.4 的 Win32 DLL 与 i686 Rust harness 已统一纯编译链接，见[编写记录](../docs/design/tx-04-07-harness.md)与[构建日志](../artifacts/repentogon/tx-04-07-harness/README.md)。未部署、启动游戏或运行测试；下方未构建描述保留各阶段时点。

2026-10-26 tx-04-07 3.2 状态与 trace：已补齐退休状态/Join 结果、完整分类查询覆盖与 `cs query_checkpoint <label>` 场景累计快照、生命归属、丢弃/Ack、加载/持久窗口与原返回值，增加日志分片和显式截断/响应超长记录。见[接线记录](../docs/design/tx-04-07-state-trace.md)。**未构建、部署或测试**；统一整体验收留到 tx-04-07 全部实现完成后。

2026-10-26 tx-04-07 3.1 `game/`：B 生命状态/当前归属（新增 `game/remote_life.h/.cpp`）、加载优先的生命周期判断、一次 Restore 决策（首次菜单 Host 保留 vs 已有 run 退休）、按原因的远端丢弃、完整键查询重复读取、`join_state=None`/`join_last`、跨 loop 持久窗口与 Kill/Revive/Stage/GameState 只读入口已编写。统一清单增至 41 入口（新增 8 个 `observe` 组只读入口），设备槽 8 个、字节补丁 12 处不变；ByeReason/SessionCause 末尾追加 `PeerLost`，协商 build 标识改为 `isaac.cs.tx-04-07`（Rust 侧同步接受合法值与 build 字符串）。**尚未构建、编译、部署或运行游戏**；按规范，tx-04-07 的纯构建与唯一整体 E2E 留到全部实现完成后执行。以下保留各阶段原始记录。

2026-10-25 tx-04-06 3.4/3.5：Host/stop 已接入真实会话，包含 IPv4/timeout 配置、完整诊断、主机 UI、Rust codec/Endpoint/脚本远端与单启动 E2E 场景，V2 方向缺陷已修正。统一清单为 33 入口（含 5 个只读 ST8 观测）、8 个设备槽、12 个字节补丁。Win32 DLL 与 i686 Rust harness 纯编译链接成功，见[编写/使用记录](../docs/design/tx-04-06-ops-harness.md)、[原始构建日志](../artifacts/repentogon/tx-04-06-ops/README.md)。未部署或运行游戏/测试；下方占位与旧计数说明均为阶段历史。

2026-10-25 tx-04-06 game：第 3.3 节 K/Join/peer、A/B 拓扑/consumer/lifecycle、主线程 Host API 和必要的 11 处 Join/K 常驻补丁已编写并纯编译、链接成功，见[实现与接线记录](../docs/design/tx-04-06-game.md)、[构建日志](../artifacts/repentogon/tx-04-06-game/README.md)。入口仍为 28 个、设备槽 8 个，字节补丁连同原有 Odd 共 12 处。ops 的 `cs host/stop` 仍为占位，配置/UI/Rust 远端与 harness 待接入；没有部署、启动游戏、运行网络或测试，整体 E2E 留待全目标完成。以下保留各阶段原始记录。

2026-10-25 tx-04-06 core：第 3.2 节已编写并纯编译、链接成功。`SessionMode::Host` 使用两个参与者，首次激活要求 A/B 双绑定；新增 PeerBye/PeerTimeout/PeerProtocol 有序退休原因，A/B 复用同一 admission/queue/Resolve，序号冲突统一中止整个会话。见[实现与接线约定](../docs/design/tx-04-06-core.md)、[构建记录](../artifacts/repentogon/tx-04-06-core/README.md)。尚未接入 game/peer/K/Join 或 Host 命令，没有部署或执行测试；tx-04-06 全目标与唯一整体 E2E 仍待完成。下方保留各阶段原始记录。

2026-10-25 tx-04-06 网络规则修订：Authority 已同步 stale 丢弃、独立重复/迟到计数、Bye 原字节三次发送和接收端有界副本计数；发送失败不覆盖原结束原因。[同步说明](../docs/design/tx-04-06-net.md)、[build-04](../artifacts/repentogon/tx-04-06-net/build-04-network-rules.log)。Win32 Release 纯编译链接成功，仍未接入主线程 Host 会话或运行测试；Rust 远端规则留待第 13.2 节实现。

2026-10-25 tx-04-06：第 3.1 节 `src/net/` 四个模块已编写并加入独立 `isaac_cs_net` 静态库，Win32 Release 纯编译与 DLL 链接成功。包括 ICSN v1 codec/Endpoint、Winsock UDP worker、有界 SPSC、心跳/超时/Bye 与统计；[线格式和主线程接口](../docs/design/tx-04-06-net.md)、[构建记录](../artifacts/repentogon/tx-04-06-net/README.md)。尚未接入 `cs host` 或 game/peer，没有部署、运行游戏或执行测试，tx-04-06 整体 E2E 尚未执行。以下阶段记录保留其当时语境。

2026-10-24 tx-04-05 后续：剩余 Lua 夹具和 Rust 单启动 S0–S11 / V1–V10 harness 已编写，并补充 tick 提交、Odd 配对、射击分派及炸弹同 tick 查询的只读诊断，见[实现记录](../docs/design/tx-04-05-test-harness.md)。没有部署或执行测试，唯一整体 E2E 尚未执行。以下阶段记录保留其当时语境。

2026-10-24 tx-04-05 当前：`src/game/` 七个主线程适配模块及必要 Hook/Odd shim、`cs solo start/stop`、状态与 trace 已编写并纯编译链接成功，见[game 实现记录](../docs/design/tx-04-05-game-adapter.md)。构建清单为 28 个入口、8 个设备槽、1 个 Odd 普通调用点补丁；它们尚未在游戏进程中安装或验收。本次没有部署、启动游戏或运行测试；测试夹具和 harness 场景待完成，唯一整体 E2E 尚未执行。下段保留 core 阶段的原始状态。

2026-10-24 tx-04-05 core 阶段记录：`src/core/` 的五个原型模块已移植到 `cs::core`，独立 C++20 静态库与现有 DLL 已纯构建成功。`PlaceholderSession` 已移到 `ops/command.cpp`；真实 core 尚未接入 Hook/solo 命令，当前 DLL 仍执行原有透传行为。接线顺序、生命周期和剩余范围见[core 移植记录](../docs/design/tx-04-05-core-port.md)。没有运行测试或游戏，tx-04-05 的唯一整体 E2E 尚未执行。以下为 tx-04-04 历史记录。

目标为官方 REPENTOGON 1.1.3 + Repentance+ v1.9.7.12.J273。实现范围与原始失败情况见 [实现方案](../docs/design/tx-04-04-implementation-plan.md)。

首次 E2E 在注入启动阶段等待 Launch 返回超过 60 秒而失败，原始结果保留。按静态审阅修订的代码已编译；第二轮唯一一次 E2E 的结论见[结果](../docs/design/tx-04-04-results.md)。下列命令仅说明入口，不表示应重复验收。

```powershell
pwsh -NoProfile -File scripts/build.ps1
# 仅在实现全部结束后，执行该次推进唯一一次 E2E：
out/repentogon/harness.exe e2e tx-04-04 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out artifacts/repentogon/tx-04-04-e2e-02
```

构建脚本不运行测试。依赖开发包默认在 `F:/projects_isaac/REPENTOGON-1.1.3`；`-RepentogonRoot` 可指定位置，`-BuildSdk` 可构建开发包。运行时始终使用游戏安装的 libzhl.dll，构建出的 SDK DLL 不部署。

唯一 Hook 源清单为 `binding/hooks.json`；生成器读取原点位 JSON 与 `binding/v1_9_7_12-static.json`，输出 C++ 和 JSON/Markdown Hook 清单。原型重复调用点合并为 27 个入口 Hook，另有 8 个设备槽补丁。所有 gameplay Hook 只透传和计数。入口经 libzhl 共用调用链，设备槽在首次主线程 Manager::Update 中整体核对后安装，进程退出前常驻。自有定义的 ArgData 只包含显式参数，不含隐式 ECX；outfunc 保留到进程退出，兼容 REPENTOGON 晚于本 mod 初始化的情况。

配置样例 `config.example.json` 默认关闭。缺失、无效或关闭配置不注册 Hook，只输出 boot。打开后 `cs version/status/counters/echo/host/stop` 经唯一处理器处理；`host/stop` 仅是 Idle/Hosting 占位状态，没有网络或输入接管。变更在下一次最外层 Manager::Update 入口应用，端口仅监听 127.0.0.1；实际监听失败会记录 bootstrap_error。未知的 `cs` 子命令不交给原生处理器。

主线程仅将固定大小 trace 事件写入有界 ring，后台线程用 Win32 句柄写 JSONL。Game::Exit 记录 run_exit；正常进程终止时只用 WriteFile 尽力补写剩余事件和 shutdown，不等待后台线程或使用 CRT 文件锁。计数与状态为上一帧末尾快照；frame 是 Manager 观测编号，**不是 Authority tick**。`MenuStatus` 为 harness 只读观察当前菜单与存档选择；测试不进入原版 Online/Isaac Sync。菜单 SelectedSave=1 对应第二档，New Run 的 Menu_Game.SelectedElement=0；这两项已用 IDA 调用/分支静态核对。

Hook wrapper 不声明 noexcept，原生异常照常传播；调用上下文和 Manager 帧深度通过 RAII 收尾。Lua 捕获指针经 IDA 重新确认为 Game+0x680D8（旧资料的 0x68158 有误）；Print 捕获分支直接追加字符串并返回，不写控制台历史。out 非空时只追加 out。界面每 60 帧查询一次，仅状态变化时记录标记。

Harness 的两次启动属于同一个 E2E：启用配置与关闭配置。它自行启动挂起的游戏子进程、调用 zhlLoader.Launch(0)、确认焦点后发扫描码，显式确认第二档，短暂停留后退回菜单。失败不自动重跑。只结束自己创建的子进程；退出时移除自己部署的测试 mod、写回关闭配置，保留 DLL、界面 mod、存档前／中／后快照和报告，不自动恢复存档。已有测试 mod 会导致部署拒绝，已有 DLL/界面/配置会先备份。

启动路径使用普通 Win32 绝对路径，命令行为带引号的 exe；非 ASCII 或达到 MAX_PATH 时拒绝注入。失败时在清理子进程前保存日志前后状态、全部顶层窗口（包括错误框）和整屏截图，随后仍收集日志并注明可能为旧文件。存档来源覆盖 Documents 和注册表 SteamPath 下全部 userdata/*/250900/remote，按两次启动分别逐字节对比，第一、三槽字节变化判定失败，不自动恢复。

SDK 生成头在 C++20 下不兼容，因此 SDK-facing 的 definitions.cpp 和生成 wrapper 使用 C++17，其余源使用 C++20、/MD、v143/Win32。SDK 构建使用 MSVC 19.44.35217。

tx-04-04 当时未实现 tick、A 输入、网络、虚拟设备、Join 调用点补丁、world_record 或 Replica；当时的 `core/session.h` 是占位状态，现已由上方 tx-04-05 core 实现替换。

第三次 E2E 已走通启动与首帧，失败在第二条状态请求 main_thread_timeout。本次候选修订在启动等待前确认焦点，按场景截止等待真实菜单，仅对启动阶段的只读 status 超时继续查询；变更不重发。主线程丢弃执行前已过期的端口请求。control.jsonl、DLL 请求时间/首次返回记录和关闭进程前的窗口截图用于继续定位，具体根因尚未由运行时证据确认。第四次 E2E 由用户本会话明确授权，结果见结果文档；入口采用新的 tx-04-04-e2e-04 目录。

第六次已经进入第二槽 run，但固定返回序列进入 Options。静态复核后新增主线程复制的 pause_state/pause_selection：暂停 state=1，选择 0=Options、1=继续、2=退出。harness 依据状态打开暂停或离开 Options，最多移动一格到 2，观测确认后才发送 ENTER；保存导航状态及退出选项/返回截图。修订已编译，第七次 E2E 需先确认用户已准备好，两次启动分别提示跳过动画，到标题/选档页停止手动操作。

第七次已在用户确认后执行一次，退菜单及 Game::Exit 计数增长已通过，两次进程均正常退出；整体仍 FAIL，E3/E5/E7 细节见结果文档。最终配置关闭、临时测试夹具清理，不自动恢复存档或重跑。

- tx-04-11：正式Grid全量观测、全部17派生接口、保存／资源／真实变化事件与v2 reader，见[实现及统一E2E结果](../docs/design/observation-expansion/tx-04-11-implementation.md)。默认关闭时不枚举世界；采集与录制独立。

- tx-04-13：探索、全楼层离房保存容器、控制关系/非活动备份和玩家跨房历史，见[实现及统一E2E 5/5结果](../docs/design/observation-expansion/tx-04-13-implementation.md)。正式采集与录制分离；E2E复用不受影响的已完成项目，仅续测修复影响与剩余场景。
