# tx-04-07 开局 debug 10 后的完整 E2E

2026-10-26，Asia/Shanghai。用户要求继续测试，并确认本次键鼠接管。使用已纯构建的 debug10 harness，按 D20 临时禁用 Download Helper，执行一次完整入口、一次游戏启动；结束后恢复。不自动重跑、不重新归约、不回滚存档。

**整体 FAIL：S0–S20 COMPLETE，S21 FAILED。** 最终失败为 `pause navigation timeout selection=None`。S21 发出一次 ESC 后未观察到暂停，失败快照仍是 Active、run_active=1、players=2、pause_state=0、blocks=0；具体原因 UNKNOWN，没有补按 ESC 或临时跳过场景。

## 本次确认的新行为

- `startup-debug10-console.bmp` 显示原生控制台 `>debug 10` 与 **Enabled debug flag.**；提交一次后关闭控制台。report.startup_debug 的 native_flag_verified=false 表示没有直接读取原生 flag 值，不否定控制台执行记录。
- S19 的 run-exit 为 should_save=true；A dead=false、hearts=4、controller=0。上一轮退出时 A 已死、should_save=false 的情况未重现。
- S20_continue_selected 已选择 Continue。S20.end 为 loop=5575、tick=31、Active、players=2、blocks=0；恢复、重建 B 与移动编排已执行。V5 的四轮 factory_once_per_round=true，V13 的 continue_preserved、invariants_pass、save_load_topology 均为 true；这些局部证据不能替代 V13 尚未观察齐的自然查询重复覆盖。

## 中继与 V12

第二轮 S16 正常完成，主机 stop seq=51125：PeerBye、rx=673、tx=689、terminal_rx=2。前三轮严格 network-ledger 均 PASS。第四轮双方向 source/destination_difference 也都是 0，但没有计划内 RunTeardown/round4.retired 终态，因此该轮与 V4 整项仍 FAIL，不能把失败清理的 PeerBye 算作 S21 验收。

V12 仍 FAIL：普通松键对照 PASS；预置 held 185 tick / 3.067863 秒，跨房后 held 191 tick / 3.167168 秒，时长通过。重绑后 tick=439、loop=4572、seq=47613 实际 input_sequence=0、held/aim=0、shooting_suppressed=0，输入连续性仍被打断。原 laser-evidence 显示 before_release=2、after_release=0、unknown_attribution=0。

沿用 e2e-08 已发现的限制：Lua currentLoop 读取的是 FrameEnd 发布的 status_snapshot，模拟回调可能落后 native loop 一轮；松键附近的最后一条激光不能仅靠这两个计数判断为提前或未发射。跨房后的早期激光与真实输入空档仍是独立失败证据。未改写或重新归约原报告，未为迁就场景保持旧 held。

## 自动判定

| 状态 | 用例 |
| --- | --- |
| PASS | V1、V2、V5、V7、V8、V9、V11、V15 |
| FAIL | V3、V4、V6、V12、V14、V16 |
| NOT_OBSERVED | V10、V13 |

V6 的非中性输入匹配 Accepted 检查通过，但 startup_coverage=false。V10 的既有 Bye kind=4/实际为5的归约错误与所需握手样本问题未在本次修改。V13 加载/Continue 局部不变量通过，但自然重复查询覆盖未齐；不补造调用、不将未观察改判 PASS。

## 退出、恢复与复现

游戏正常 WM_CLOSE 退出码0，crash_changes=[]，cleanup_error/collection_error/evaluation_error 为空。shutdown seq=64074 的 trace 丢失/截断/队列丢弃/写入/唤醒/native_error 均0。16项存档变化 allowed=true；不回滚、不翻只读位、不计算哈希。

测试夹具已移除，C/S enabled/control_port.enabled=false，无残留游戏进程；Download Helper 原本启用，已恢复。准备窗口为 Asia/Shanghai 13:39:19.964–13:42:04.963，见 operator-preparation.jsonl。control 无 focus_lost/focus_restore 样本，不声称焦点恢复分支通过。用户已获知可恢复键鼠。

原报告、四轮网络日志、Lua、trace、源码、场景、存档窗口与截图保留。operator-run.ps1 是本次一次性外层脚本；本目录不可用于重复执行。复现需新目录、按规范确认每次启动接管，并采用相同临时禁用/恢复安排：

```powershell
out/repentogon/harness.exe e2e tx-04-07 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out <new-artifact-directory>
```

见[历次结果](../../../docs/design/tx-04-07-results.md)与[debug10 修改](../tx-04-07-debug10-preparation/README.md)。本次测试后未修改功能代码或再启动游戏。
