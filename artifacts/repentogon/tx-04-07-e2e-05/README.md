# tx-04-07 焦点维护修改后的唯一整体 E2E

2026-10-26，Asia/Shanghai。完成焦点维护实质修改与纯构建，经用户本次准备确认后仅执行一次完整入口、一次游戏启动。结果 **FAIL**：S0–S5 COMPLETE，S6 FAILED，S7–S21 NOT_RUN，failure=unexpected_pause_in_running_scene。

停止在 S6 主动关闭计划内暂停后的稳定绑定等待：loop=1356 仍为pause=1，新 watch_running 立即报错，没有区分预期退出过渡，属于 harness 回归，不能据此认定游戏无法退出暂停。本轮 focus_lost/focus_restore 均0条，焦点恢复分支尚未得到实机样本。

S4 移动/释放、S5 单边沿/资源减一、skip 局部检查再次通过；V1/V7/V8/V15 PASS，其余按原报告 FAIL/NOT_RUN。游戏正常退出、无新 dump、trace错误为0，环境已清理，无自动重跑、重新归约或测试后功能修改。

复现入口（再次接管输入与使用授权第二档，须另行安排启动）：

```powershell
out/repentogon/harness.exe e2e tx-04-07 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out <new-artifact-directory>
```

原始 report.json/report.md、control.jsonl、steps.jsonl、enabled/trace.jsonl、Lua观测、源码/场景快照与文件窗口均保留。`preparation.json` 保存准备确认与 Download Helper 恢复时间。测试 mod 已移除，C/S enabled/control_port.enabled=false，无残留游戏进程；16项存档变化按现行规则均allowed=true，不回滚、不计算哈希。

详见[完整结果](../../../docs/design/tx-04-07-results.md)、[修改与回归说明](../../../docs/design/tx-04-07-focus-fix.md)、[纯构建](../tx-04-07-focus-fix/build-01.log)。
