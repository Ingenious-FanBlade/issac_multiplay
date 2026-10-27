# tx-04-08：SDK C++17 适配与完整 DLL 构建修复

日期：2026-10-27，Asia/Singapore。按用户要求修复 `ASMPatcher.hpp:302` 的 `friend static` 构建错误，属于同一 tx-04-08 的必要纯编译链接工作。**完整 DLL 在新的构建目录中编译、链接成功；未运行测试、未启动游戏、未部署到游戏目录。**

## 原因与修改

SDK 1.1.3 的 `ASMPatcher.hpp` 声明 `template<typename T> friend static StackSpace Make(bool);`，在本机 MSVC 的 C++20 模式下报 C2216。两个补丁编译单元都包含此头；Host 又通过 `game/join.h → core/participant.h` 引入 `std::span` 和默认比较运算符，因此此前无法直接整体降到 C++17。

- 将 `JoinMode/JoinCreate` 的单份声明移至现有 C++17 `game/bridge.h`。函数只传 bool、整数、地址；实现继续在 C++20 `join.cpp` 中。
- `host_patch.cpp` 移除 `join.h`，只经 `bridge.h` 调用 Join 决策；`join.h` 本来已包含 `bridge.h`，原调用者仍能看到同一份声明。
- CMake 将 `host_patch.cpp/odd_patch.cpp` 与既有 binding/生成 wrapper 一起设为逐文件 C++17。其余 DLL、core、net、world 保持 C++20。
- 没有改 SDK 文件、编译器一致性开关、警告屏蔽、函数 ABI、汇编指令、补丁地址/清单、原生调用次数、Hook 生命周期或输入策略。

## 构建证据

入口：[build.ps1](build.ps1)，日志：[build.log](build.log)。仓库根执行：

```powershell
& artifacts/repentogon/tx-04-08-sdk-build-fix/build.ps1
```

本次首次创建 `build/repentogon-tx-04-08-sdk-fix`，使用标准 CMake 生成工程，没有筛选编译单元，没有复用此前 `world-bridge-compile.vcxproj` 的 obj/lib。

- Generator：Visual Studio 17 2022 / Win32 / v143 / Release。
- 编译器：MSVC 19.44.35217.0；Windows SDK 10.0.26100.0。
- 完整重建 core、net、world，运行正式绑定生成命令，编译全部 DLL 单元并链接。
- 生成清单：79/96 行（既有 41 + world 外层 19 + 内层 19），distinct RVA 59；78 字段、15 来源、10 RTTI 类。
- 最终退出码：0。链接输出 `F:/projects_isaac/issac_multiplay/out/repentogon/zhlIsaacCS.dll`，对应 PDB 位于同目录。
- SDK 生成头/rapidjson 警告、网络队列 C4324 对齐警告及 SDK `ASMPatch::ASMNode` 非虚析构 C5205 警告原样保留在日志。此次不声称修复这些既有警告或验证 SDK 的运行行为。

先前 C2216 失败记录和仅部分单元纯编译的记录仍保留在 [collector 构建材料](../tx-04-08-collector-build/README.md)，不将旧失败改写成成功。

## 范围限制

此结果确认当前源码和 SDK 的完整 DLL 编译/链接问题已解决。没有执行 harness、replay、单元测试或游戏 E2E，不代表原生补丁、world 采集或 2,000 次目标已通过实机验收。其余命令、离线工具、夹具与统一整体 E2E 按 tx-04-08 设计继续推进。
