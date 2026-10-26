# tx-04-07 V12 恢复窗口与 witness 修复产物

> 后续 e2e-11 已覆盖 V12 并 PASS：两个恢复暂停tick，提前B激光0、真实松键后B激光1，实时witness与松键tick一致；整体仍因S17移动净空停止而FAIL。见[实际证据](../tx-04-07-e2e-11/README.md)，下方保留e2e-10原时点。

> 后续 e2e-11 已覆盖 V12 并 PASS：两个恢复暂停tick，提前B激光0、真实松键后B激光1，实时witness与松键tick一致；整体仍因S17移动净空停止而FAIL。见[实际证据](../tx-04-07-e2e-11/README.md)，下方保留e2e-10原时点。

> e2e-10 已执行一次，S9 因 no_observed_free_direction 提前停止，V12 NOT_RUN。仅新 witness/恢复暂停片段得到实际观察，不能宣称跨房修复通过。见[本次结果](../tx-04-07-e2e-10/README.md)。未自动重跑。

> e2e-10 已执行一次，S9 因 no_observed_free_direction 提前停止，V12 NOT_RUN。仅新 witness/恢复暂停片段得到实际观察，不能宣称跨房修复通过。见[本次结果](../tx-04-07-e2e-10/README.md)。未自动重跑。

2026-10-26，Asia/Shanghai。用户要求修复 V12，并明确采纳最多六个 Authority tick 的 Shooting 恢复窗口；规则已同步到项目规范第9节。A/B 同规则，Authority 不等待输入，查询仍中性，首条新 current 命令或到期结束窗口，不复用旧 held、不写蓄力。

build-01.log：Win32 DLL 与 i686 Rust harness 统一纯编译链接成功，未运行任何局部/单元测试。changes.patch：相对 e2e-09 实际受测 sources 的六个实现/夹具/归约文件差异，无哈希。

新增 cs witness 读取实时主线程 frame/tick/generation；tx07 Lua 不再使用帧尾 status_snapshot 定位激光。V12 分别报告原始 held 连续性、原始空档、逐调用证明的恢复暂停和有效射击连续性，不删除中性事实。

纯构建后经用户本次接管准备确认，执行一次完整 E2E，见 [e2e-10](../tx-04-07-e2e-10/README.md)。原始结果保留，失败不自动重跑。详细原因与实现边界见 [修复说明](../../../docs/design/tx-04-07-v12-recovery-fix.md)。
