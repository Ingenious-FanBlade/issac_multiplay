# REPENTOGON mod (tx-04-04)

目标为官方 REPENTOGON 1.1.3 + Repentance+ v1.9.7.12.J273。实现范围与原始失败情况见 [实现方案](../docs/design/tx-04-04-implementation-plan.md)。

本轮代码已编译；唯一 E2E 在注入启动阶段等待 Launch 返回超过 60 秒而失败，运行时能力尚未验证，未重跑。详见[结果](../docs/design/tx-04-04-results.md)。下列 E2E 命令用于说明入口，不表示应重复本轮验收。

```powershell
pwsh -NoProfile -File scripts/build.ps1
# 仅在实现全部结束后，执行该次推进唯一一次 E2E：
out/repentogon/harness.exe e2e tx-04-04 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out artifacts/repentogon/tx-04-04-e2e-01
```

构建脚本不运行测试。依赖开发包默认在 `F:/projects_isaac/REPENTOGON-1.1.3`；`-RepentogonRoot` 可指定位置，`-BuildSdk` 可构建开发包。运行时始终使用游戏安装的 libzhl.dll，构建出的 SDK DLL 不部署。

唯一 Hook 源清单为 `binding/hooks.json`；生成器读取原点位 JSON 与 `binding/v1_9_7_12-static.json`，输出 C++ 和 JSON/Markdown Hook 清单。原型重复调用点合并为 27 个入口 Hook，另有 8 个设备槽补丁。所有 gameplay Hook 只透传和计数。入口经 libzhl 共用调用链，设备槽在首次主线程 Manager::Update 中整体核对后安装，进程退出前常驻。自有定义的 ArgData 只包含显式参数，不含隐式 ECX；outfunc 保留到进程退出，兼容 REPENTOGON 晚于本 mod 初始化的情况。

配置样例 `config.example.json` 默认关闭。缺失、无效或关闭配置不注册 Hook，只输出 boot。打开后 `cs version/status/counters/echo/host/stop` 经唯一处理器处理；`host/stop` 仅是 Idle/Hosting 占位状态，没有网络或输入接管。变更在下一次最外层 Manager::Update 入口应用，端口仅监听 127.0.0.1；实际监听失败会记录 bootstrap_error。未知的 `cs` 子命令不交给原生处理器。

主线程仅将固定大小 trace 事件写入有界 ring，后台线程写 JSONL。计数与状态为上一帧末尾快照；frame 是 Manager 观测编号，**不是 Authority tick**。`MenuStatus` 为 harness 只读观察当前菜单与存档选择；测试不进入原版 Online/Isaac Sync。菜单 SelectedSave=1 对应第二档，New Run 的 Menu_Game.SelectedElement=0；这两项已用 IDA 调用/分支静态核对。

Harness 的两次启动属于同一个 E2E：启用配置与关闭配置。它自行启动挂起的游戏子进程、调用 zhlLoader.Launch(0)、确认焦点后发扫描码，显式确认第二档，短暂停留后退回菜单。失败不自动重跑。只结束自己创建的子进程；退出时移除自己部署的测试 mod、写回关闭配置，保留 DLL、界面 mod、存档前后备份和报告，不自动恢复存档。已有测试 mod 会导致部署拒绝，已有 DLL/界面/配置会先备份。

SDK 生成头在 C++20 下不兼容，因此 SDK-facing 的 definitions.cpp 和生成 wrapper 使用 C++17，其余源使用 C++20、/MD、v143/Win32。SDK 构建使用 MSVC 19.44.35217。

本轮未实现 tick、A 输入、网络、虚拟设备、Join 调用点补丁、world_record 或 Replica。`core/session.h` 是明确的占位状态。
