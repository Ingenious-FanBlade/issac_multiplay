# tx-04-07 第 3.4 节纯构建记录

日期：2026-10-26。代码范围与接口见[编写记录](../../../docs/design/tx-04-07-harness.md)。所有日志来自纯编译链接，未运行 harness、游戏或任何测试。

- `build-01.log`：CMake 配置完成；MSBuild 读取用户目录中的 Microsoft SDKs 时被沙箱拒绝，编译未开始。保留原始失败。
- `build-02.log`：同一 `scripts/build.ps1` 在允许读取 SDK 的环境完成 Win32/v143/Release DLL 与 i686 Rust harness 编译链接。DLL 的 SDK/RapidJSON、生成 wrapper 未使用参数及队列对齐警告保留；Rust 有一项变量命名警告，随后已修正。
- `build-03.log`：包含新代次进度计时修正、启动代次覆盖、切房与加载归约的统一纯构建成功，Rust 无 warning。
- `build-04.log`：最终统一纯构建成功，补齐 relay 错误收尾摘要及原始错误传递；Rust 无 warning。仅代码编译，不消耗或替代唯一整体 E2E。

构建入口为 `scripts/build.ps1`。产物在 `out/repentogon/`，包括 `zhlIsaacCS.dll`、`harness.exe` 与生成的 Hook manifest；没有部署至游戏目录。没有校验哈希、修改存档、启动网络端点或执行阶段测试。
