# tx-04-07 中继与 V12 修复产物

2026-10-26，Asia/Shanghai。本目录是本次代码修复与纯构建记录，不是新的 E2E 结果。未部署、启动游戏或运行任何测试；e2e-06 原报告与原始日志保持原样。

- `build-01.log`：受限构建无法访问本机 Windows SDK 目录，未进入代码编译。
- `build-02.log`：获得构建访问权限后，Win32 DLL 与 i686 Rust harness 编译链接成功。C++ 的 SPSC 对齐填充提示保留，Rust 无 warning。
- `changes.patch`：本次生产代码与 harness 相对 e2e-06 的 sources 快照差异，不包含更早工作区改动，不涉及哈希。

中继记录 10054/10052 为 UDP 通知，主机逻辑退休后进行 100 ms 有界真实收包。V12 延长按住时长、增加同角色松键对照，按实际消费命令划分窗口，并输出独立 laser-evidence.json。真实缺输入仍消费中性；跨房射击和严格网络对账均未在本次修复后实机验证。

详细事实、修改和限制见 [说明](../../../docs/design/tx-04-07-relay-v12-fix.md)。唯一整体入口保持 `out/repentogon/harness.exe e2e tx-04-07 --game <game-root> --out <new-artifact-directory>`；这里只记录用法，没有执行。
