# tx-04-07 Lua 夹具与证据窗口实质修复

2026-10-26，Asia/Shanghai。范围与原始依据见[修复说明](../../../docs/design/tx-04-07-fixture-fix.md)：PlayerManager ordinal 使用 GetPlayerIndex，事件 loop 不按 Game frame 缓存，A/B 投弹分别采样资源消耗窗口。旧结果和未知项保留；分析/修改期间未运行测试。

本次纯构建日志放在当前目录；唯一完整 E2E 使用新目录 `tx-04-07-e2e-04`，须在本次实机准备确认后启动，失败不自动重跑。

`build-01.log`：统一 Win32 DLL 与 i686 Rust harness 纯构建成功，Rust 无 warning，未运行测试或部署。

`changes.patch`：相对于 e2e-03 保存的三份源文件的可读差异，不包含哈希。

本轮唯一 E2E 已执行：S4 移动、skip 局部检查和 V8 投弹通过；S7 失焦后进入暂停，S8.recovered 脚本超时，整体 FAIL。无新 dump，环境已清理，无自动重跑或测试后功能修改。见[本轮产物](../tx-04-07-e2e-04/README.md)。
