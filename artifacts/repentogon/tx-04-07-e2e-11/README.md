# tx-04-07 第十一次 E2E（用户正式验收通过）

> **当前验收结论：PASS / USER_ACCEPTED（D31，2026-10-26）**。用户明确指示“正式视为此e2e测试全部通过”，本轮整体验收正式通过，tx-04-07 按用户接受完成；不再安排补测。见[验收记录](acceptance.md)和[结构化记录](acceptance.json)。原 report.json/report.md 与日志不改写、不重归约，下文保留决定前的实际运行及自动判定事实。

2026-10-26，Asia/Shanghai。用户要求继续测试；先完成[移动净空选择与等待余量修正](../tx-04-07-movement-clearance-fix/README.md)、Rust纯构建，再收到本次键鼠接管确认后执行一次完整入口、一次游戏启动。沿用开局 debug10、六 tick Shooting 恢复窗口和实时 witness。没有自动重跑或旧报告重归约。

**V12 本次 PASS。整体 FAIL：S0–S16 COMPLETE，S17 FAILED，S18–S21 NOT_RUN。** 最终错误仍为 no_observed_free_direction，发生于第三轮 S17 的移动准备，不能把 V12 通过外推为整个 tx-04-07 完成。

## V12 的实际通过证据

原始 [laser-evidence.json](laser-evidence.json) 与 report.json 保存以下证据：

- 普通松键正对照 PASS；跨房前实际 held 189 tick / 3.132948 秒，跨房后实际 held 197 tick / 3.266893 秒，均达到配置要求180 tick与至少两秒。
- before_release=[]，after_release 只有1条归属 B 的新激光，unknown_attribution=0；所有相关激光带实时 witness_version=1。
- 实际计划松键为 native tick=648、loop=4808、input_sequence=650、held/aim=0（trace seq=51042）。激光 laser_id=7:2 的 witness 同为 frame=4808、simulation_tick=648、session_generation=2、b_generation=2，attribution=B。旧帧尾快照把松键激光归到前一轮的问题未在此样本重现。
- 重绑时 tick449/450（loop4609/4610，trace seq48012/48020）仍真实为 sequence=0、held/aim=0。每个 tick 的 shooting_suppressed=1、shooting_startup_deferred=1，并有相应实际分派 original=0 的恢复窗口证据；未复用旧 held，Authority继续推进。
- fresh_held_continuity=false 如实保留中性事实；verified_startup_pauses=2，continuity_gaps=[]，effective_shooting_continuity=true。它按用户明确采纳的六 tick 恢复规则判定，不把中性伪写成 held，也不删除原始空档。armed_new_source=true。

这次通过覆盖了重绑恢复暂停、首条新命令后恢复射击、计划松键发射与实时激光时序。不能由此宣称六 tick 到期分支、A 的实际恢复暂停分支或所有武器/网络延迟都已覆盖。

## 移动选择与 S17 阻断

S9/S11/S12 的移动编排本次均完成，所选方向实际净空均80、held段18 tick；新 movement_paths 记录保留各方向净空。但本次没有选用不足80的短通道，不能将它当成短通道分支已实测通过，或补称 e2e-10 的原房间已复测。

第三轮 S17 最后双玩家观察 loop=4878：B present=true、dead=false、animation_finished=true、player_index=1、Entity Index=62，位置约(848.6172,405.0739)，room index71/stage2/type2；四方向 clear_distance 都为0，movement_options=[]。路径预览的20像素边界/原生格子碰撞条件未满足，helper停止，没有发试探移动或绕过碰撞；这不证明世界中绝对不能移动，具体几何/生成点原因仍待核对。

## 网络与自动结果

前两轮严格 network-ledger 均 PASS，S13 Stop与S16 PeerBye收尾完成。第三轮双方向包数差均0，但未到达计划内超时/退休检查点，因此该轮与V4整项仍FAIL；第四轮未启动。没有放宽网络计数或把失败清理当成计划内退休。

| 状态 | 用例 |
| --- | --- |
| PASS | V1、V7、V8、V9、V11、V12、V15 |
| FAIL | V3、V4、V5、V6、V10、V14、V16 |
| NOT_RUN | V2、V13 |

## 收尾与复现

游戏正常 WM_CLOSE 退出码0，无新 dump；cleanup/collection/evaluation_error 均为空。shutdown seq=51812 的 trace 丢失/截断/队列丢弃/写入/唤醒/native_error 均0。16项存档变化 allowed=true，不回滚、不翻只读位、不计算哈希。control无focus_lost/focus_restore样本。

测试 mod 已移除、C/S enabled/control_port.enabled=false，无残留游戏进程；Download Helper已恢复。临时禁用窗口为Asia/Shanghai 14:13:35.469–14:15:42.561，见operator-preparation.jsonl。用户已获知可恢复键鼠。本次测试后没有功能修改、再启动或重跑。

原报告、源码/场景、control/steps、三轮网络、Lua/trace、存档窗口与截图均保留。operator-run.ps1是本次一次性外层脚本。按同一完整入口复现须新目录、另行确认每次启动接管，并临时禁用/恢复Download Helper：

```powershell
out/repentogon/harness.exe e2e tx-04-07 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out <new-artifact-directory>
```

见[V12修复](../../../docs/design/tx-04-07-v12-recovery-fix.md)与[历次结果](../../../docs/design/tx-04-07-results.md)。
