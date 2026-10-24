# tx-04-05 夹具与 harness 编写记录

2026-10-24。完成 Lua F8/F9/只读日志、Rust 单启动 S0–S11、V1–V10 归约及必要的只读诊断，见[实现记录](../../../docs/design/tx-04-05-test-harness.md)。没有部署、启动游戏或运行 harness，没有执行测试；唯一整体 E2E 尚未执行。

本目录只保存纯编译输出与交付说明，不能作为 E2E 验收结果。

- `rust-build.log`：Rust i686 MSVC Release 纯编译链接成功，没有 warning。
- `rust-build-02.log`：补充到房目标不符时立即停止后，Rust 增量纯编译成功，无 warning。
- `rust-build-03.log`：收紧缺少 tick 指标时的判定后最终纯编译成功，无 warning；交付的 harness.exe 对应该构建。
- `rust-build-04.log`：第一次 E2E 失败后按 D20 修改启动 focus 与 V10 第一、三档基线规则，增量纯编译成功，无 warning；`out/repentogon/harness.exe` 已更新为该构建。没有运行 harness。
- `rust-build-05.log`：第二次 E2E 后按 D21 增加 S0 的 game_frame 等待、S9 改发 F8，增量纯编译成功，无 warning；`out/repentogon/harness.exe` 已更新为该构建。没有运行 harness。
- `rust-build-06.log`：第三次 E2E 后按 D22 让 S10 沿 S9.move 反向移动，增量纯编译成功，无 warning；`out/repentogon/harness.exe` 已更新为该构建。没有运行 harness。
- `native-build.log`：首次纯构建被沙箱拒绝访问本机 `AppData/Local/Microsoft SDKs`，MSBuild 在源码编译前失败。保留原始记录。
- `native-build-02.log`：按相同命令放宽工具链读取后，Win32 DLL 编译链接成功，没有 warning。没有部署或运行 DLL。

纯构建命令（均不运行测试）：

```powershell
cargo build --manifest-path tools/harness/Cargo.toml --release --locked --offline --target i686-pc-windows-msvc --target-dir build/harness
cmake --build build/repentogon --config Release --target zhlIsaacCS -- /m:4 /v:minimal
```

编译产物为 `out/repentogon/zhlIsaacCS.dll` 与 `out/repentogon/harness.exe`。Lua 代码没有运行；只能在未来一次整体 E2E 中确认实际回调、键位、阻断、切房和游戏效果。
