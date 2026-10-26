# tx-04-07 夹具修复后的唯一整体 E2E

2026-10-26，Asia/Shanghai。用户要求修复 e2e-03 的身份误判并继续测试；本次完成 PlayerIndex/事件 loop/投弹资源窗口的实质修复与纯构建，经本次准备确认后仅运行一次完整入口、一次游戏启动，无自动重跑或重新归约。

结果 **FAIL**：S0–S7 COMPLETE、S8 FAILED、S9–S21 NOT_RUN，failure=script_timeout:S8.recovered。S7 故障段游戏失焦（loop=2216、focus=0），随后进入暂停，Authority tick 停在1298，S8 恢复脚本无法完成。失焦来源 UNKNOWN；失败现场游戏窗口 foreground=false。游戏正常退出、无新 dump、trace 错误与丢失为0。

本轮身份修复在 Entity Index=2 / player_index=1 的真实 B 上生效；S4 右移/上移及释放检查通过，skip 真实 loop 跨越60且跳过30次，V8 的 A/B 单次投弹与库存分别减一通过。第二轮未到达，不能声称阿萨泻勒动画/跨房场景通过。

复现入口（再次启动会接管输入并使用已授权第二档，须另行安排）：

```powershell
out/repentogon/harness.exe e2e tx-04-07 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out <new-artifact-directory>
```

- `report.json` / `report.md`：原始自动结果，V1/V7/V8/V15 PASS，其余 FAIL/NOT_RUN。
- `enabled/trace.jsonl` / `control.jsonl` / `steps.jsonl`：失焦阻断、命令、资源窗口与退休记录；shutdown seq=29408，各 trace 错误为0。
- `lua-observations.json`：区分原实体 index、entity_index、player_index；S4 位移、S5 资源与 skip loop 原始观测。
- `enabled-failure/state.json`：失败时窗口不在前台；desktop.bmp 含屏幕内容，仅按现有规则本地保留。
- `relay-1.jsonl` / `remote-1.jsonl` / `network-ledger.json`：第一轮提前收尾与实际故障决定，未冒记四轮对账通过。
- `sources/` / `scenario.json`：本次部署源码和配置快照。
- `file-*.json*` / `save-changes.json` / `persistent-evidence.json`：文件监听和快照；16项变化按现行规则均allowed=true，不合并成完整持久验收。
- `preparation.json`：本次准备确认与 Download Helper 恢复时间。

测试 mod 已移除，C/S enabled/control_port.enabled=false，Download Helper 恢复，无残留游戏进程。未回滚存档、未计算哈希、未改只读位。见[完整结果](../../../docs/design/tx-04-07-results.md)、[修复说明](../../../docs/design/tx-04-07-fixture-fix.md)与[纯构建](../tx-04-07-fixture-fix/build-01.log)。
