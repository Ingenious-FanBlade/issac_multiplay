# tx-04-06 game/ 纯构建记录

日期：2026-10-25。范围为 game/ 与必要 Join/K 常驻补丁，见[实现记录](../../../docs/design/tx-04-06-game.md)。只编译/链接和静态阅读，没有部署、启动游戏、运行 UDP worker 或任何测试。

- [build-01.log](build-01.log)：Win32 / v143 / Release 首次纯编译、DLL 链接成功。有六条新 shim 未引用标签 C4102，以及 SDK/生成 wrapper 的既有 warning；未引用标签已删除。随后根据静态调用链/生命周期阅读修正了 Join owner，补充接收代次和锁顺序处理，当前代码不以这份旧构建为最终结果。
- [build-02.log](build-02.log)：最终代码纯编译、DLL 链接成功。game/ 与 host_patch 没有编译 warning；仅保留 net SPSC 的三条对齐填充 C4324，以及 SDK/生成 wrapper 的既有 warning。包括 Ack 回显代次的本地元数据、可用后才公布 B 输入 generation、两类设备锁顺序、主菜单 Warmup 与 Join 完整返回后绑定；没有执行任何运行时程序或测试。

入口（现有 CMake 工程；不含测试）：

```powershell
cmake --build build/repentogon --config Release --target zhlIsaacCS -- /m:4 /v:minimal
```

DLL 产物为 `out/repentogon/zhlIsaacCS.dll`。生成的 `build/repentogon/generated/hook-manifest.json` 包含 28 个入口、8 个设备槽和 12 处字节补丁。构建成功不证明补丁已在游戏安装或 Host 联机可用；ops Host/stop、UI 与 Rust 客户端仍待接入。tx-04-06 全目标及唯一整体 E2E 尚未完成。

本次生成清单同时归档为 [hook-manifest.json](hook-manifest.json) / [hook-manifest.md](hook-manifest.md)，并同步至 `out/repentogon/`，避免 DLL 与供后续 harness 使用的清单版本不同。这是本地构建产物整理，没有部署到游戏目录或运行验收。
