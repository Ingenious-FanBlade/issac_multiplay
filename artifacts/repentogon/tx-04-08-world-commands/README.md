# tx-04-08：cs world 命令纯编译记录

日期：2026-10-27，Asia/Singapore。实现说明见[命令与后台准备](../../../docs/design/tx-04-08/tx-04-08-world-commands.md)。

复现入口：仓库根执行 `& artifacts/repentogon/tx-04-08-world-commands/build.ps1`。CMake / Visual Studio 17 2022 / Win32 / v143 / Release，仅构建 `zhlIsaacCS` 及其 core/net/world 依赖，不运行测试、启动游戏或部署到游戏目录。

- [configure-sandbox.log](configure-sandbox.log)：首次配置时 MSBuild 读取用户目录的 Microsoft SDK 元数据被沙箱拒绝，未开始代码编译；此原始记录保留。
- [build-initial.log](build-initial.log)：允许读取 SDK 元数据后，在新的 `build/repentogon-tx-04-08-world-commands` 构建树完成全部单元和 DLL 链接，退出码 0。
- [build.log](build.log)：静态复核补齐后台实际线程退出、异常收口和状态字段后，对最终源码执行纯编译链接成功，退出码 0。

产物为 `out/repentogon/zhlIsaacCS.dll` 和对应 PDB。SDK/rapidjson 警告与网络队列对齐警告原样保留；不以纯编译结果推断实机行为或 tx-04-08 整体通过。测试、游戏、部署均 NOT_RUN。
