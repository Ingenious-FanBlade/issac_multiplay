# tx-04-05 core 构建记录

日期：2026-10-24。仅配置和编译/链接，不运行测试或游戏；实现范围和接线约定见[移植记录](../../../docs/design/tx-04-05-core-port.md)。

- `configure.log`：CMake 配置成功。
- `build-sandbox-denied.log`：第一次构建在 MSBuild 读取 `C:/Users/114514/AppData/Local/Microsoft SDKs` 时被沙箱拒绝，尚未编译源码。保留原始失败。
- `build.log`：允许读取已安装工具链后构建成功；五个 core 源文件编译、静态库生成、DLL 链接完成。warning 来自 SDK 头文件与现有生成 wrapper，core 没有 warning。

可重复的纯构建入口（不代表需要重复执行）：

```powershell
cmake -S mod -B build/repentogon -G 'Visual Studio 17 2022' -A Win32 -T v143
cmake --build build/repentogon --config Release --target zhlIsaacCS -- /m:4 /v:minimal
```

产物为 `build/repentogon/Release/isaac_cs_core.lib` 与 `out/repentogon/zhlIsaacCS.dll`。没有部署；DLL 的运行时接线仍是 tx-04-04。这里不是 E2E 结果，不能据构建成功声称输入、tick 或单人会话实机通过。
