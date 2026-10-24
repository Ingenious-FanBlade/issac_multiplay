# tx-03-02：T0-09-B 统一验收中断 05

日期：2026-10-20（Asia/Shanghai）。事实记录；原始 artifact 保留，不自动重跑。

## 实际调用

```powershell
python cs-t0/accept-t09-b.py `
  --artifact-dir artifacts/cs-t0/tx-03-02-e2e-05 `
  --authorization "user requested T0-09-B work in this chat; offline two-process foundation only"
```

本次运行已越过 manifest/参数/import 阶段，实际启动了 relay、两个 endpoint controller 和两个
独立 endpoint driver。

## 中断原因

执行工具的调用窗口在 120 秒到期，未等待离线 F0 完整结束；`accept-t09-b.py` 的 240 秒 / 
orchestrator 180 秒内部边界尚未触发。Python/工具树随后不再存活。

## 中断时的实际事实

- build exit 0。
- `run-start.json`、host/vm Wire/UDP/control/driver 日志已生成。
- A driver 最后记录到 tick 7412；B 输入 sequence 7308、target 7427。
- T9C1 heartbeat 已转发给 B，B 输入由 `controllerHeartbeat + lead_ticks` 驱动。
- Wire 无 overwrite 记录；未见 driver/controller error。
- 没有形成完整 F0、正常 stop、STOP/RETIRE、relay stopped summary、analyzer 或最终
  acceptance result；因此不能判 PASS。
- 未启动游戏、未注入、未操作 Original Online/Isaac Sync。

artifact：`artifacts/cs-t0/tx-03-02-e2e-05/`，staging work 保留。

## 后续

后续执行必须使用新的 create-new artifact 根，并在不受 120 秒前台调用上限影响的受控后台
job 中等待同一统一入口结束；不把本次部分运行拼接为后续 evidence，也不自动重跑本目录。
