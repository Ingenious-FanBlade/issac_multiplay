# tx-04-07 S13 退休崩溃实质修复

2026-10-25，America/Los_Angeles。`read_dump.py` 只读解析 e2e-02 已保留的 minidump；`dump-analysis.json` 保存实际异常、寄存器与全局对象关系，缺失内存保留 null。未启动游戏或进行阶段测试。

转储指向 RemoveCoPlayer RVA 0x590CE5，EBX=this 等于全局 Manager；该 ABI 实际需要当前 Game 内嵌的 PlayerManager。具体静态依据、修复范围和唯一 E2E 安排见[修复说明](../../../docs/design/tx-04-07-retirement-fix.md)。原始转储与两次失败报告不覆盖、不删除。

`native-remove-window.json` 为当前 v1.9.7.12 IDB 对故障附近指令的只读导出；不修改 IDB 或进程。`build-01.log` / `build-02.log` 是两次纯构建成功日志，均未运行测试或部署；最后补齐不可读/缺席分支后的最终构建为 build-02。E2E 使用单独的 `tx-04-07-e2e-03` 目录。

本轮唯一 E2E 已结束：S13 正常退休、无新增崩溃；整体仍因 Lua 夹具的实体 Index/玩家 ordinal 混淆导致动画字段缺失而 FAIL。没有自动重跑或测试后功能修改，见[本轮产物](../tx-04-07-e2e-03/README.md)。
