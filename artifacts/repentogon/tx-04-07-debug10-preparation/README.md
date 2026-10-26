# tx-04-07 下一次开局 debug 10 准备

> 后续 e2e-09 已执行一次：控制台截图确认 `debug 10` 返回 Enabled debug flag；S19 A 存活且 should_save=true，S20 Continue 完成。整体仍因 S21 暂停等待超时而 FAIL，见[实际结果](../tx-04-07-e2e-09/README.md)。下方“未测试”保留编码时点，未自动重跑。

> 后续 e2e-09 已执行一次：控制台截图确认 `debug 10` 返回 Enabled debug flag；S19 A 存活且 should_save=true，S20 Continue 完成。整体仍因 S21 暂停等待超时而 FAIL，见[实际结果](../tx-04-07-e2e-09/README.md)。下方“未测试”保留编码时点，未自动重跑。

2026-10-26，Asia/Shanghai。e2e-08 结束后，用户指出所有角色死亡导致 S20 超时，建议下次开局在控制台输入 `debug 10`。本次按该指示只修改下一次 harness 编排；未部署、启动游戏或运行任何新测试。

`startup_debug_tx07` 在首轮新局完成 S3 初始启动观察后，通过原生控制台提交一次 `debug 10`，保存 startup-debug10-console.bmp、startup_debug_command 步骤、前后检查点及报告准备字段。关闭控制台后，使用现有 stable 等待真实恢复；不跳过阻断、不重复发命令、不在 Continue 时再次切换。不直接改生命、debug flag 或存档字段；报告 native_flag_verified=false，不能把发键当作原生 flag 值已验证。

Rust i686 Release 纯构建成功，见 build-01.log；harness.exe 已更新到工程 out 目录，未部署到游戏。DLL 没有改动。不新增单元测试、不执行局部试跑或重复本轮 E2E。

changes.patch 相对 e2e-08 实际受测源码，只含这一项新增编排。原 e2e-08 sources 和失败报告保持原样。下一次完整 E2E 应核对控制台执行记录、A 存活、S19 should_save 与 S20 Continue，并继续如实记录 V12 与其它未完成项。debug 10 对显式 B Kill/Revive 场景的实际影响也须在整体入口观察，不能提前记为通过。
