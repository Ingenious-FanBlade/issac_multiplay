# tx-04-06 3.1 net/ 纯构建记录

日期：2026-10-25。只构建，不运行测试、游戏、UDP worker 或独立协议程序；没有部署到游戏目录。实现和接口见[记录](../../../docs/design/tx-04-06-net.md)。

- CMake 生成成功：Visual Studio 17 2022、Win32、v143，Windows SDK 10.0.26100.0，SDK 根 `F:/projects_isaac/REPENTOGON-1.1.3`。
- 第一次沙箱内 MSBuild 在 `Microsoft.Cpp.WindowsSDK.props` 读取 `C:/Users/114514/AppData/Local/Microsoft SDKs` 时被拒，尚未编译源文件；工具原始返回为 MSB4184。之后同一纯构建使用已批准的沙箱外工具链访问。
- [build-01-failed.log](build-01-failed.log)：首次实际编译失败，clang-format 对 include 排序后将 bcrypt.h 放在 Windows 头前，出现类型缺失。已显式固定 WinSock → Windows → BCrypt 头顺序，并用格式化禁用区保护该组。
- [build-02.log](build-02.log)：修正头顺序后，net 库编译和 DLL 链接成功。三条 C4324 是 SPSC 的 64 字节缓存行对齐所产生的预期填充提示。
- [build-03.log](build-03.log)：初版 net 增加不可恢复 sendto 错误结束处理及失败状态发布后，纯构建成功，0 error、3 条上述对齐填充 warning。
- [build-04-network-rules.log](build-04-network-rules.log)：按后补网络规则增加 Stale 接收结果、独立 duplicate/stale 计数、Bye 原字节三次发送和首份 Bye 后的有界副本计数。首次成功发送后只 CommitSent 一次，发送失败保留真实终止原因，另发布 last_send_error/bye_send_attempts/bye_sent_packets。Win32 Release 纯编译和 DLL 链接成功，0 error、3 条原有对齐填充 warning；当前构建结论以此日志为准。没有运行协议程序、网络 worker 或任何测试，未部署到游戏。

产物路径：`build/repentogon/Release/isaac_cs_net.lib` 与 `out/repentogon/zhlIsaacCS.dll`。当前没有主线程调用 Worker，链接器可以裁去未引用的网络对象；此构建不表示游戏中已运行网络。

构建入口（纯编译，无测试）：

```powershell
cmake -S mod -B build/repentogon -G 'Visual Studio 17 2022' -A Win32 -T v143 '-DREPENTOGON_ROOT=F:/projects_isaac/REPENTOGON-1.1.3'
cmake --build build/repentogon --config Release --target zhlIsaacCS -- /m:4 /v:minimal
```

tx-04-06 整体实现和唯一一次 E2E 尚未完成；本目录不是测试验收产物，不包含哈希或 gate。
