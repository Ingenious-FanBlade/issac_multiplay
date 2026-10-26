# tx-04-07 唯一整体 E2E：FAIL

执行日期：2026-10-25（America/Los_Angeles）。一次入口、一次游戏启动，没有自动重跑。

原始 `report.json` / `report.md`、trace、control、relay/remote、存档快照和现场记录保持不变。详细事实与自动归约的边界见[结果记录](../../../docs/design/tx-04-07-results.md)。

首个阻断为 `trace_event_lost`：启动菜单阶段已有 63 条 trace 丢失，首轮开局 loop=553 触发失败退休，尚未创建 K/Join B，Authority tick=0。S0–S2 已完成；S3 检查点未完成，后续场景未运行。

游戏在失败收尾中正常退出（0），没有新增转储。测试 mod 已移除，C/S 与控制口关闭，Download Helper 恢复；`preparation.json` 记录临时禁用与恢复。`console.log` 是本次唯一命令的控制台输出。

原始报告的 `trace_parse_error=trace sequence gap` 导致严格解析不采用事件集合，故 V1 NOT_OBSERVED、V4 FAIL，其余项 NOT_RUN。原始 trace 中仍保留失败/停止事件；不得将自动归约缺少 host 数据解释为网络没有停止，也不得把未运行项记为通过。
