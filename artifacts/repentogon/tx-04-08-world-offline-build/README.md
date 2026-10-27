# tx-04-08：离线工具纯编译记录

日期：2026-10-27，Asia/Singapore。实现与使用见[离线 replay／compare](../../../docs/design/tx-04-08/tx-04-08-world-offline.md)。本目录只有代码编译记录，不含测试或 replay/compare 运行结果。

Rust 使用现有依赖缓存、`--offline --locked`、i686 MSVC Release，在独立 `build/harness-world-offline` 构建目录完成 harness 编译链接。命令：

```powershell
cargo build --manifest-path tools/harness/Cargo.toml --release --locked --offline --target i686-pc-windows-msvc --target-dir build/harness-world-offline
```

首次增加直接 serde/derive 依赖后，`--locked` 提示锁文件需更新；使用离线模式更新锁文件的依赖关系。随后依赖 `syn v3.0.5` 已缓存但尚未解包，沙箱拒绝创建 Cargo registry 源目录，报 `os error 5`；允许缓存写入后构建成功。没有使用网络下载，没有执行任何文件摘要校验命令。

- `rust-build.log`：首次成功 Release 构建，24.69 s，原有本轮 unused PHASE import 警告保留；后续已改用常量。
- `rust-build-final.log`：补齐 schema/checkpoint 和比较边界后的纯编译，14.71 s，退出 0。
- `rust-build-complete.log`：补齐收口计数、来源/身份覆盖后的纯编译，14.45 s，退出 0。
- `rust-build-ready.log`：最终源码纯编译；以该日志为当前 Rust 产物出处。
- `dll-build.log`：修复 `FlushFaultAt` 的重复 JSON 时间键后，在已有 `build/repentogon-tx-04-08-world-commands` 构建树增量编译 writer 并重链完整 DLL，退出 0。命令为 `cmake --build build/repentogon-tx-04-08-world-commands --config Release --target zhlIsaacCS -- /m:4 /v:minimal`，不是运行程序。

Rust 产物：`build/harness-world-offline/i686-pc-windows-msvc/release/harness.exe`，同步到 `out/repentogon/harness.exe`。DLL 产物：`out/repentogon/zhlIsaacCS.dll`。没有部署到游戏目录，没有游戏启动、离线工具试跑、单元测试或功能验收；唯一整体 E2E 仍未执行。
