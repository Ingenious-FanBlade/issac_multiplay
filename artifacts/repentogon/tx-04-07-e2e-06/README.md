# tx-04-07 暂停等待修复后的唯一整体 E2E

2026-10-26，Asia/Shanghai。用户要求修复e2e-05的S6回归，完成stable预期阻断退出等待修复与纯构建，经本次准备确认后仅执行一次完整入口、一次游戏启动。无自动重跑或重新归约。

整体 **FAIL**：S0–S15 COMPLETE，S16–S21 NOT_RUN；S16已经发起远端Bye、主机正常PeerBye退休，但relay.finish报`relay_receive ... os error 10054`，完成标记/检查点未到达，第三轮未启动。这是UDP中继接收错误，游戏正常退出、无新dump。

S6回归未重现，V9 PASS；V1/V7/V8/V11/V15同样PASS。S4/S9/S12移动、S7 clean/fault局部推进率检查通过；第二轮B Entity Index=46、player_index=1，身份与动画观测正常。V12仍NOT_OBSERVED：fresh_held_continuity=false，松键前后均无所需新激光记录，不能把S15编排结束当作射击功能通过。

复现入口（会接管输入并操作已授权第二档，须另行安排启动）：

```powershell
out/repentogon/harness.exe e2e tx-04-07 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out <new-artifact-directory>
```

原始report.json/report.md、control.jsonl、steps.jsonl、enabled/trace.jsonl、Lua观测、两轮relay/remote日志、源码/场景快照与文件窗口均保留。第二轮remote summary error=null/local_bye；主机net stop正常PeerBye，但relay-2 summary保留10054，不伪造round2.retired或三方对账通过。

shutdown seq=44101，trace各项错误/丢失/截断均为0；测试mod移除，C/S enabled/control_port.enabled=false，Download Helper恢复，无残留游戏进程。16项存档变化按现行规则均allowed=true；不回滚、不计算哈希、不翻只读位。测试后未改功能代码。

见[完整结果](../../../docs/design/tx-04-07-results.md)、[修复说明](../../../docs/design/tx-04-07-pause-wait-fix.md)与[纯构建](../tx-04-07-pause-wait-fix/build-01.log)。
