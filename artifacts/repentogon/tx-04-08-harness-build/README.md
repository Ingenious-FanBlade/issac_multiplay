# tx-04-08 Lua / unified harness 纯编译

日期：2026-10-27，Asia/Singapore。实现说明见[Lua 与统一 E2E](../../../docs/design/tx-04-08/tx-04-08-harness.md)。本目录只保存构建日志，不含 E2E、replay/compare 或游戏运行结果。

```powershell
cargo build --manifest-path tools/harness/Cargo.toml --release --locked --offline --target i686-pc-windows-msvc --target-dir build/harness-tx08-e2e
cmake --build build/repentogon-tx-04-08-world-commands --config Release --target zhlIsaacCS -- /m:4 /v:minimal
```

首次 Rust 纯编译发现 BufRead 导入、共用方法可见性和 report 借用错误，已修正；后续编译链接成功。首次 DLL 构建被沙箱拒绝读取 Windows SDK 缓存目录；允许纯构建访问后成功。`rust-build.log/dll-build.log` 保存最终源码的纯编译结果，两项退出码均为 0；Rust 最终构建 16.88 s，DLL 收尾构建 6.63 s。它们不是运行验收。Rust 产物来自 `build/harness-tx08-e2e/i686-pc-windows-msvc/release/harness.exe`，已同步至 `out/repentogon/harness.exe`；DLL 产物为 `out/repentogon/zhlIsaacCS.dll`。

没有启动游戏、部署测试 mod、运行 Lua、测试、离线生产 CLI、额外语法校验或哈希操作。默认场景及缺口见实现说明；不能把构建成功解释为 tx-04-08 功能通过。
