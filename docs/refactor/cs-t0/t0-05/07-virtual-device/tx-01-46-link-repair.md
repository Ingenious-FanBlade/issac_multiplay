# tx-01-46：固定 v143 工具链并重新整体验收

日期：2026-10-16。用户在 tx-01-45 链接失败后明确要求“解决链接问题，然后重新开始测试”。本次包含实际构建入口修复，并沿用既有全范围隔离场景授权；不覆盖或重写 tx-01-45 的失败记录。

## 实现前失败判据与产物

本次统一入口只执行一次构建和真实游戏 E2E，运行前不另行编译或试链接。链接失败、工具选择不明确、旧 PDB/对象混入、helper 或控制器失败均记 FAIL；没有实际到达的游戏阶段记 NOT_RUN。游戏用例顺序及失败判据沿用[实现前定义的 E2E 场景](tx-01-45-e2e.md)，未观察场景不记 PASS，失败不自动重跑。

产物使用新目录 `artifacts/cs-t0/tx-01-46-live-01`，在构建前保存 invocation.json、scenario-input.json 和 toolchain.json；构建输出保存在 build.log，本次对象、PDB、DLL/helper 放在同目录 build/ 下。运行阶段继续由原有 coop-active 捕获 Authority/query/lifecycle/恢复等事件及逐项结果。capture 中 tx-01-45 表示功能实现来源，本次入口及构建修复编号为 tx-01-46。不计算哈希。

## 直接读取与修复

- 旧入口的 `vswhere -latest` 现在解析到 `D:\Microsoft Visual Studio\VS2026\MSBuild\Current\Bin\MSBuild.exe`，但 Native 工程指定 v143。
- 上一轮构建的 tlog/lastbuildstate 指向 `D:\VisualStudio2022\VC\Tools\MSVC\14.44.35207`，Hostx86 / Win32，链接带 `/DEBUG:FULL` 与 `/LTCG`，复用 out/ 下已有 PDB/对象。
- 静态文件元数据显示 VS 2022 的 cl/link/PDB 组件版本为 19.44/14.44.35217.0；VS 2026 的为 19.51/14.51.36260.0。各自目录内文件版本一致；未证明上一轮究竟加载了哪一份 PDB DLL，不能将跨安装选择直接写成已证实的唯一根因。

修复统一入口：只选择带 C++ x86/x64 组件的 VS 2022；从该安装读取 v143 默认版本，明确使用它的 amd64 MSBuild、Hostx64→Win32 编译/链接工具。该进程的 PATH 首部使用同一版本工具目录；MSBuild 关闭节点复用。Native/helper 使用本次新目录中的独立 OutDir/IntDir，避免旧对象或 PDB 进入本次构建。保留符号、优化与 Win32 目标，不修改已安装 VS 文件，不停止其他构建进程。

Microsoft 说明 PDB manager 组件应匹配版本，且 `PreferredToolArchitecture=x64` 选择宿主工具、不改变输出目标：[PDB manager mismatch](https://learn.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/fatal-error-c1902?view=msvc-170)、[MSBuild C++ 工具属性](https://learn.microsoft.com/en-us/cpp/build/reference/msbuild-visual-cpp-overview?view=msvc-170)。这些文档支持修复选择，不能替代本次链接结果。

## 验收结果

**链接修复 PASS，整体 E2E FAIL。**

一次统一入口构建中，Rust release、Native DLL、helper 全部成功；Native 的 lastbuildstate 为 v143 / Native64Bit / 14.44.35207 / Win32。未再出现 LNK1101。保留 [build.log](../../../../../artifacts/cs-t0/tx-01-46-live-01/build.log) 与 [toolchain.json](../../../../../artifacts/cs-t0/tx-01-46-live-01/toolchain.json)。这证明本轮构建修复有效，不证明旧错误的唯一根因。

初次运行阶段在受限 shell 中未枚举到唯一游戏，入口退出 1；保留 `capture/result.json`。同一时段桌面 MCP 和提权只读 Get-Process 均确认唯一 PID 47276、路径符合，受限 shell 的 Get-Process 没有结果。用户随后明确回复“继续实机验收，沿用现有操作授权”。依此使用本轮已构建产物在桌面权限下继续一次实机窗口，未重新构建；产物单独保存在 `capture-desktop/`，没有覆盖第一次启动失败。

桌面实机窗口的 helper 报告 `stage=REMOTE_INITIALIZE native_error=3221225477`（`0xC0000005`），控制器退出 1。这里是远程初始化返回的异常值，不能误称 helper 本机崩溃。正常 resident-stop 清理尝试一次，记录 error=31；之后桌面工具确认游戏已不再运行。没有重启、再次注入、强制卸载或发送物理按键。

Windows Application Error 事件 1000（2026-10-16 22:14:44）确认：故障应用 isaac-ng.exe / PID 0xB8AC（47276），故障模块为本轮 `IsaacCsT0.Native.dll`，异常 `0xC0000005`，模块偏移 `0x372D`，Report ID `29d55a5c-81d3-4698-af45-4312c5d50a18`。离线读取已生成 DLL 的反汇编，偏移 0x372D 为 `rep movs dword ptr es:[edi],dword ptr [esi]`，对应 `coop_active_runtime.cpp` 的 Publish 事件负载复制（120 字节）。没有取得当时寄存器、访问地址或 dump，具体无效源/目标及根因仍 UNKNOWN；未在本轮继续修改运行时代码或试跑。

| 内容 | 结果 |
| --- | --- |
| Rust、Native、helper 构建 | PASS |
| 本轮消除 LNK1101 并生成独立 DLL/helper/PDB | PASS |
| 桌面远程初始化 | FAIL，0xC0000005 |
| loop 原生初始化/Hook 安装事件 | 已观察，loop-events.jsonl sequence 1/2；不能替代完整恢复证明 |
| Virtual Device 注册、Join、A/B 身份及输入、lifecycle/退休 | NOT_OBSERVED，events.jsonl 为空 |
| 47 槽恢复、回调排空、pair 缺席及 DLL 常驻 | NOT_OBSERVED；restoration 记录 nativeRestored=false、pinned=false、targets=[] |
| 整体 | FAIL；T0-05 闭环仍未验收完成 |

完整 [实机逐项结果](../../../../../artifacts/cs-t0/tx-01-46-live-01/capture-desktop/result.json)、[helper 日志](../../../../../artifacts/cs-t0/tx-01-46-live-01/capture-desktop/helper.log)、[恢复记录](../../../../../artifacts/cs-t0/tx-01-46-live-01/capture-desktop/restoration.json) 及 [loop 事件](../../../../../artifacts/cs-t0/tx-01-46-live-01/capture-desktop/loop-events.jsonl) 均已保存。根 result.json 更新为实机结果；先前沙箱失败原文仍在 capture/result.json。
