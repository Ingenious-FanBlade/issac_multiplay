# tx-04-07 第七次 E2E 入口：启动前中止

2026-10-26，Asia/Shanghai。用户要求继续测试，并确认本次停止键鼠操作。执行一次 `harness.exe e2e tx-04-07`，退出码 1；**未部署、未启动游戏，S0–S21 / V1–V16 全部 NOT_RUN**。没有自动重跑，没有更改功能代码或 mod 启用状态。

入口：

```powershell
out/repentogon/harness.exe e2e tx-04-07 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out artifacts/repentogon/tx-04-07-e2e-07
```

`steps.jsonl` 仅有 S0、environment、S0_keyboard。失败发生在 `input_environment_tx05`：只允许启用 isaac-cs-ui，而 `repentogonreleasesbackup_3643104060` 处于启用状态，因此在 capture_sources、crash_before、存档快照、部署和 launch 之前返回错误。

该目录的 metadata.xml 名称为 **REPENTOGON Download Helper**、directory 为 repentogonreleasesbackup、id 为 3643104060，描述为备用下载方式。原 metadata 只读复制为 `download-helper-metadata.xml`。这是前几轮按 D20 临时禁用并在结束后恢复的同一下载辅助 mod；本轮操作遗漏该准备步骤，不是本轮已经观察到第三方 gameplay 干扰。没有为绕过检查扩大允许列表。

## 原报告的已知错误

report.json / report.md 保持原始内容。其 `failure_cause.kind=game_crash` 和 `crash_changes` 中的 12 个 dump **不能作为本轮游戏崩溃证据**：入口在采集 crash_before 之前失败，但 finish_tx05 仍将结束时的全部 dump 与缺失基线比较，把历史文件误报为变化。列出的文件名均为本次之前的日期。报告的 launches 为空，environment.game_running=false，启动后也未发现游戏进程；本轮没有启动游戏。

证据 JSON 是失败收尾生成的空归约材料；laser-evidence.json 存在不表示执行过 S15。中继收尾修复与 V12 时长/对照/证据边界修复 **仍未接受实机验证**。

用户已获知键鼠接管没有发生，可以恢复操作。本轮没有部署测试夹具、写游戏配置、修改存档、创建 disable.it 或启动游戏；无本轮环境修改需要恢复。后续应先完成已授权的 Download Helper 临时禁用与恢复安排，并修复缺 crash_before 时的崩溃误分类，再安排完整验收；遵守现行规范，不自动重跑。

见 [本次修复范围](../../../docs/design/tx-04-07-relay-v12-fix.md)和[历次结果](../../../docs/design/tx-04-07-results.md)。
