# tx-04-06 3.2 core/ 纯构建记录

日期：2026-10-25。仅编译与链接，不执行测试、游戏、UDP worker 或独立程序，不部署到游戏目录；实现和接线约定见[core 记录](../../../docs/design/tx-04-06-core.md)。

- [build-01.log](build-01.log)：默认沙箱内 MSBuild 读取 `C:/Users/114514/AppData/Local/Microsoft SDKs` 被拒，返回 MSB4184，尚未编译源文件。
- [build-02.log](build-02.log)：允许工具链访问后，同一现有 CMake 工程以 Win32 / v143 / Release 成功构建 core、net 与 DLL。core 无 warning；net 有原有的三条 SPSC 缓存行对齐填充 C4324 提示，没有编译或链接错误。

构建入口（现有 CMake 工程；无测试）：

```powershell
cmake --build build/repentogon --config Release --target zhlIsaacCS -- /m:4 /v:minimal
```

产物为 `build/repentogon/Release/isaac_cs_core.lib` 与 `out/repentogon/zhlIsaacCS.dll`。Host 尚未被主线程适配层调用；编译成功不表示 Host 联机已可用。tx-04-06 的唯一整体 E2E 尚未执行。
