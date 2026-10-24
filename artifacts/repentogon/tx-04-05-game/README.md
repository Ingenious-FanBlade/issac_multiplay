# tx-04-05 game 适配构建记录

日期：2026-10-24。只配置、编译和链接，没有运行测试、游戏或 DLL，没有部署。实现范围见[记录](../../../docs/design/tx-04-05-game-adapter.md)。

- `configure.log`：CMake 配置成功。
- `build-01.log`：全部新增适配、shim 和生成 wrapper 编译链接成功；现有依赖 warning 保留。
- `build-02.log`：静态复查补充设备失败和退休中 A 替换处理后的增量构建成功，无 warning。
- `build-03.log`：统一激活前后采样新鲜度的 Manager 轮次后构建成功；SDK/生成 wrapper 的 warning 保留，game 源文件没有 warning。
- `build-04.log`：收紧采样作用域与原查询失败处理后的最终构建成功，无 warning。四份日志均为纯编译/链接，测试执行次数为零。
- `hook-manifest.json/md`、`hooks_generated.cpp`、`permanent_patches.inc`：最终构建生成的可读材料，描述 28 个入口、8 个槽与一个五字节 Odd 补丁，不表示它们已在实机安装。

可重复的纯构建入口（这里没有调用测试入口）：

```powershell
cmake -S mod -B build/repentogon -G 'Visual Studio 17 2022' -A Win32 -T v143
cmake --build build/repentogon --config Release --target zhlIsaacCS -- /m:4 /v:minimal
```

产物为 `out/repentogon/zhlIsaacCS.dll`。tx-04-05 唯一整体 E2E 尚未执行，不能从编译成功推断运行时通过。
