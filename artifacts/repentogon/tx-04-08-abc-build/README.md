# tx-04-08 ABC 纯编译链接记录

日期：2026-10-27，Asia/Singapore。源码交付及范围见[ABC说明](../../../docs/design/tx-04-08/tx-04-08-abc-implementation.md)。这里只有构建日志，没有测试或游戏运行结果。

执行命令：

```powershell
cargo build --manifest-path tools/harness/Cargo.toml --release --locked --offline --target i686-pc-windows-msvc --target-dir build/harness-tx08-e2e
cmake --build build/repentogon-tx-04-08-world-commands --config Release --target zhlIsaacCS -- /m:4 /v:minimal
```

首轮 Rust 编译成功（rust-build.log，18.23秒）。首轮 DLL 调用在进入编译前被沙箱拒绝读取 `C:/Users/114514/AppData/Local/Microsoft SDKs`，MSBuild报MSB4184；这是构建环境访问失败，没有运行产物。随后允许纯构建访问SDK缓存，完整DLL编译链接成功（dll-build.log）。该日志包含外部SDK/RapidJSON/ASMPatcher头的既有警告；未修改第三方头。

之后静态阅读补齐通知线程真实退出、故障清理label归属、严格test_injection解码、作用域关联和episode匹配，再做必要增量纯编译：`rust-build-final.log`（17.29秒）与 `dll-build-final.log`，退出码均为0。DLL输出 `out/repentogon/zhlIsaacCS.dll`；Rust输出 `build/harness-tx08-e2e/i686-pc-windows-msvc/release/harness.exe` 并复制到 `out/repentogon/harness.exe`。生成清单复制至同一out目录；未部署到游戏。

最后静态复读修正了短命episode：Spawn返回后的Remove/Clear/Destroy可能是新的根call，须由同一身份接回对应body，不能只找Spawn的parent子树；另保存逐身份Publish正常返回次数。该Rust源码的增量编译记录为 `rust-build-episodes.log`，退出码0。Lua补全创建前/创建后/移除后三态与seed/varData后态核对，未运行Lua语法或功能校验。

没有执行生成产物、Lua解释器、测试、replay/compare、游戏或哈希操作。生成器只随生产构建更新编译所需表，不运行单独验证入口。编译成功不代表故障场景已经触发、参数实际可用或tx-04-08整体验收通过。特殊Player Replace完整资格仍UNKNOWN，D–G和唯一整体E2E继续待完成。
