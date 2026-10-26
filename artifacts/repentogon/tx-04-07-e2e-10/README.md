# tx-04-07 V12 恢复与实时采证修复后的 E2E

2026-10-26，Asia/Shanghai。用户要求查明并修复 V12，再重跑测试；本次另外明确采纳最多六个 Authority tick 的 Shooting 恢复窗口。完成[修复与统一纯构建](../tx-04-07-v12-recovery-fix/README.md)后，收到本次键鼠接管准备确认，执行一次完整入口、一次游戏启动，没有自动重跑或重新归约。

**整体 FAIL：S0–S8 COMPLETE，S9 FAILED，S10–S21 NOT_RUN；V12 NOT_RUN。** 最终错误为 `no_observed_free_direction`，尚未进入第二轮角色7或 S15，不能宣称本次已验证跨房蓄力修复。

## S9 的实际阻断

夹具 room 动作已从 stage1/type2/index84 正常到达 index97，target_matched=true，没有 fixture_error。最后一条双玩家观察在实时 loop=2833：B present=true、dead=false、animation_finished=true，位置约 (317.3926,158.5162)，但 movement_options=[]；A 同位置附近有左右两个选项。

`direction_tx07` 读取最后一条 observation，发现 B 没有可选方向后立即返回错误。此后没有发送移动脚本、补按 F8、传送或改变碰撞判定；没有证据证明“无方向”是否会随原生更新自然消失。失败收尾移除 B 后的仅 A 观察不能替代错误当时的双玩家记录。

## 已覆盖的修复片段与限制

- 523 条本次 Lua 记录均带 witness_version=1，loop 与 witness.frame 一致；cs witness 新接口已实际部署。没有到达角色7激光场景，因此不能宣称松键激光的新前后归类已实测通过。
- native shooting_dispatch 共记录7次 startup_deferred=1，均为 B，original=0、sequence=0、source_seen=0、limit=6。新局代次1暂停 tick1/2/3，后续恢复代次2/3/4分别暂停 tick136/463/473；换房代次5暂停 tick1827。Authority 继续推进，没有复用旧 held。
- 这些样本证明恢复暂停分支已实际执行；本次未证明六 tick 到期分支、A 实际暂停分支或完整 V12 世界行为，不能外推为完整通过。
- 开局 debug 10 步骤已执行，B 普通射击与投弹局部用例通过；本次没有死亡/复活、Continue 或计划内 S21 退出覆盖。

自动结果：PASS 为 V1/V7/V8/V15；FAIL 为 V3/V4/V5/V6/V9/V14/V16；NOT_RUN 为 V2/V10/V11/V12/V13。原 report.json/report.md 与空/部分证据文件不修改、不重归约，历史通过不并入本次结果。

## 收尾与复现

游戏正常 WM_CLOSE 退出码0，无新 dump；cleanup_error、collection_error、evaluation_error 为空。shutdown seq=32116 的 trace 丢失/截断/队列丢弃/写入/唤醒/native_error 均0。16项存档变化 allowed=true，未回滚、未翻只读位、未计算哈希。测试 mod 已移除、C/S/control_port 禁用、无残留游戏进程。

Download Helper 已恢复，临时禁用窗口为 Asia/Shanghai 13:58:50.069–13:59:52.872，见 operator-preparation.jsonl。用户已获知可恢复键鼠。本次测试后未修改功能代码、重新启动或重跑测试。

原报告、control/steps、trace、Lua、源码/场景与文件窗口均保留。operator-run.ps1 是本次单次外层编排。复现入口需要新目录、每次启动单独确认接管，并临时禁用/恢复 Download Helper：

```powershell
out/repentogon/harness.exe e2e tx-04-07 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out <new-artifact-directory>
```

后续仍需处理 S9 无可观察移动方向的编排阻断，再安排完整验收，不能只换产物编号自动重复。见[修复说明](../../../docs/design/tx-04-07-v12-recovery-fix.md)与[历次结果](../../../docs/design/tx-04-07-results.md)。
