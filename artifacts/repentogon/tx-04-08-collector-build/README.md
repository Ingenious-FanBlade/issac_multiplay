# tx-04-08 collector / bridge 纯编译记录

> 后续状态：2026-10-27，SDK 编译阻断已按用户要求修复，完整 DLL 编译与链接成功，见[后续构建记录](../tx-04-08-sdk-build-fix/README.md)。本目录保留此前的失败与局部纯编译事实，原日志不覆盖。

日期：2026-10-27（Asia/Singapore）。这是代码生成和编译材料，不是测试或整体 E2E 报告。

- 可重复入口：[compile.ps1](compile.ps1)。运行于仓库根：`& artifacts/repentogon/tx-04-08-collector-build/compile.ps1`。
- 最终编译日志：[compile.log](compile.log)。首版及修订前的成功编译保留在 `compile-initial.log`、`compile-before-run-scope-fix.log`，不改写旧输出。
- CMake：Visual Studio 17 2022、Win32、v143、Release。world/core 静态库使用 C++20；绑定与生成 wrapper 保留项目原有逐文件 C++17 参数。
- 生成结果：`rows=79/96 (legacy=41 world=38) fields=78 sources=15 classes=10`，distinct RVA 59。
- DLL 编译范围：mod_init、definitions、world_layout、registry、tick、consumers、lifecycle、remote_life、world_bridge、config、command、hooks_generated。使用复制的生成工程及 `ClCompile` 目标，不链接 DLL，不部署产物。
- SDK 生成头/rapidjson 的既有警告保留在日志，未通过修改 SDK 掩盖。

此前普通 MSBuild 在沙箱中查询 `AppData/Local/Microsoft SDKs` 被拒绝（MSB4184）；获工具批准读取 SDK 安装目录后，world/core 纯编译成功。整 DLL 构建随后仍在 SDK `ASMPatcher.hpp:302` 的 `friend static` 上报 C2216，影响既有 odd/host 单元。单次 `SelectedFiles` 属性尝试未限制实际编译范围，仍遇到同一既有错误；此后改为脚本中显式复制/筛选生成工程，只执行纯编译。首次筛选脚本由 vswhere 选择了本机另一版本 MSBuild 且编译成功；最终脚本固定使用 CMake 所选 Visual Studio 安装下的 MSBuild。

脚本不执行 ctest、cargo test、游戏、harness、replay 或其他测试。没有安装/卸载 Hook，没有运行期输入、注入或存档操作。本记录不声称 DLL 已整体链接，更不声称 2,000 次或任何运行功能通过。
