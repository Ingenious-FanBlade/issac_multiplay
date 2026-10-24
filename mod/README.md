# REPENTOGON mod (tx-04-04)

2026-10-24 tx-04-05：`src/core/` 的五个原型模块已移植到 `cs::core`，独立 C++20 静态库与现有 DLL 已纯构建成功。`PlaceholderSession` 已移到 `ops/command.cpp`；真实 core 尚未接入 Hook/solo 命令，当前 DLL 仍执行原有透传行为。接线顺序、生命周期和剩余范围见[core 移植记录](../docs/design/tx-04-05-core-port.md)。没有运行测试或游戏，tx-04-05 的唯一整体 E2E 尚未执行。以下为 tx-04-04 历史记录。

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
