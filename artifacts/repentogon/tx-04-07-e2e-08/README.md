# tx-04-07 中继与 V12 修复后的完整入口 E2E

2026-10-26，Asia/Shanghai。用户在 e2e-07 启动前中止后再次要求继续，并确认本次键鼠接管准备。补齐此前 D20 的 Download Helper 临时禁用，执行一次完整入口、一次游戏启动；无自动重跑、重新归约或存档回滚。

**整体 FAIL：S0–S19 COMPLETE，S20 FAILED，S21 NOT_RUN。** S20 等待 menu_game_selection=1 超时；失败截图中 Continue 灰色，停在 New Run(0)。游戏正常 WM_CLOSE 退出码 0，无新 dump，cleanup_error/collection_error/evaluation_error 均为空。

## 复现与原始记录

```powershell
out/repentogon/harness.exe e2e tx-04-07 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out <new-artifact-directory>
```

须按现行规范另行确认每次启动的接管时机，并临时禁用/恢复 Download Helper。`operator-run.ps1` 是本次单次执行脚本，`operator-preparation.jsonl` 记录准备和恢复。临时禁用窗口为 Asia/Shanghai 13:20:39.539–13:23:20.779；原本启用，结束后恢复启用。

原始 `report.json` / `report.md`、`control.jsonl`、`steps.jsonl`、四轮 relay/remote、`enabled/trace.jsonl`、Lua 观察、源码/场景与存档窗口均保留。不要直接复用本目录或自动重复执行一次性脚本。

## 中继收尾实际结果

第 2 轮 S16 已完成，继续进入第三、四轮；原 e2e-06 的 relay receive 10054 中断未重现。主机 stop seq=51128：PeerBye、stopped=1、rx=669、tx=683、terminal_rx=2，说明有界终止收包实际收到了两份迟到包；没有再次提交输入或回复 Bye。relay 第 0/1 方向分别为 669/683，成功转发数与两端实际收包一致，严格对账通过。

前三轮 network-ledger 均 PASS；第 4 轮在 S20 超时后进入失败清理，没有计划内 S21 终态/退休检查点，主机至远端方向有 1 包缺口，所以 V4 整项仍 FAIL。没有放宽严格对账。第二轮 udp_notices=[0,0]，本次没有实际覆盖新的 10054/10052 通知分类分支，不能外推该分支已实机通过。

## V12：确有跨房提前发射，但松键边界仍有观测偏移

新的 `laser-evidence.json` 中：同角色普通松键正对照 PASS，出现 1 条归属 B 的激光；预置按住 182 tick / 3.017258 秒，跨房后重新 held 193 tick / 3.200348 秒，均满足时长要求。由此可确认本次原生角色可以发射，Lua 的新激光观测链工作，旧“所有激光事件均缺失”的情况未重现。

真实连续性仍失败：重绑后的 tick 432、433、434 都是 input_sequence=0、held=0、aim=0、shooting_suppressed=0（trace seq 47562/47574/47589）。远端已预置下一代次源不等于新命令及时到达。这三个正常 tick 按既有规则消费中性，没有偷偷保持旧攻击。

原报告将两条激光都列为 before_release，after_release 为空，V12=FAIL。需区分下面两条：

- 第一条 Lua loop=4491，明显早于计划松键；相邻 native trace seq=47557/47558 显示原生 Shooting 与 Lua status 查询发生在实际 loop=4492、tick=432，正好是缺输入首 tick。这支持“重绑后中性消费造成提前发射”，不支持加载阻断时 original 被误调用（loop=4491 原调用为 0）。
- 第二条 Lua loop=4687；实际松键是 tick=628、loop=4688、input_sequence=630。native trace seq=50514/50515 在该 Shooting 内记录 Lua status 查询。它与真正松键相邻，不能直接把它当成另一次提前发射，或断言松键后没有发射。

静态回读发现 Lua `currentLoop()` 虽每次都执行 `cs status`，但 ops 返回的是 `status_snapshot`，只在 `FrameEnd` 更新；模拟中的 Lua callback 因此读到上一轮 loop。该跨来源时间偏移尚未修复，本次没有重归约原报告。V12 仍有独立的提前激光与三个输入空档证据，不能因第二条存在归类问题而改判 PASS。

## S20 根因：A 已死亡，无法 Continue

第一条 A 已死观测在 Lua loop=4993（第三轮长断网阶段），hearts=0、dead=true；S18.end、S19.begin 前后均已死亡。S19 退出回调明确 should_save=false，snapshot 中 A 仍 dead=true、hearts=0。退出后 Continue 灰色；S20 选项始终为 New Run(0)，不是仅凭超时猜测按键失效。没有可用于本次 Continue 的活跃 run；具体伤害来源未取证，不作猜测。

用户随后指出下次开局应输入 `debug 10`。已另外修改下一次 harness 准备并纯构建，见 [debug10 记录](../tx-04-07-debug10-preparation/README.md)。**本次测试未执行 debug 10**，测试后没有重新启动；本目录 sources 是实际受测版本，后续改动不冒充本轮结果。

## 其它结果与未决项

- PASS：V1、V7、V8、V9、V11、V15。
- FAIL：V2、V3、V4、V5、V6、V12、V13、V14、V16。
- NOT_OBSERVED：V10。S7 clean/fault 推进率比例约 1.000160、非等待输入不变量和 S8 局部检查通过；S17 没有覆盖所需 Hello retry/重复握手证据。S18 实际主机/远端超时年龄为 3000/3004 ms。
- V10 的 Bye 丢弃计数还有归约错误：代码筛选 packet.kind=4，协议 Bye 实际为 5；relay-3 原始记录有 3 份 direction=1、kind=5 的 outage 丢弃，但报告显示 0。本次记录错误，不改写或重新归约结果。
- control 中无 focus_lost/focus_restore 样本；不宣称焦点恢复分支已实测。

shutdown seq=58625 的 trace 丢失/截断/队列丢弃/写入/唤醒/native_error 均为 0。16 项存档变化均 allowed=true；未回滚、未翻只读位、未计算哈希。测试 mod 已移除，C/S 与 control_port 已禁用，无残留游戏进程，Download Helper 已恢复。用户已获知可恢复键鼠。
