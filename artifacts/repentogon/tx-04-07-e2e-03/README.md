# tx-04-07 退休修复后的唯一整体 E2E

2026-10-25，America/Los_Angeles。用户要求修复 e2e-02 的 S13 崩溃后继续测试。完成原生 RemoveCoPlayer 的 this 修复、退休身份复核和统一纯构建 build-02，收到本次准备确认后仅启动一次游戏、运行一次统一入口，无自动重跑或重新归约。

结果 **FAIL**：S0–S14 COMPLETE，S15–S21 NOT_RUN，failure=b_animation_timeout。S13 已正常 Stop 退休，round1.retired 的 K=Removed、players=1、peer stopped、Idle、cleanup_failed=0，第一轮网络账本 PASS；上次崩溃未重现。第二轮 Active 后，Lua 夹具因实体 Index=17 与 ordinal=1 不同而误标 B 不存在，动画字段缺失，15 秒后超时。这不是“动画实际未结束”的证据。

失败收尾又正常移除第二轮角色 7 的 B；游戏退出码 0，无新 dump，shutdown trace 错误/丢失/截断均为 0。V1/V7/V11/V15 PASS；完整 V4/V14 等四轮项仍未通过，旧局部失败保留。

复现入口（会再次接管输入并操作已授权第二档，须另行安排启动）：

```powershell
out/repentogon/harness.exe e2e tx-04-07 --game 'F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth' --out <new-artifact-directory>
```

- `report.json` / `report.md`：原始自动结果，不回写 PASS。
- `enabled/trace.jsonl` / `control.jsonl` / `steps.jsonl`：退休、status 与场景事件；原生移除正常返回 seq=39853、53550。
- `lua-observations.json`：B 的 resolved_ordinal=17 / present=false；保留错误观测而非补造动画结果。
- `network-ledger.json` / `relay-*.jsonl` / `remote-*.jsonl`：第一轮完整对账与第二轮提前收尾。
- `sources/` / `scenario.json`：本次源与配置快照。
- `file-*.json*` / `save-changes.json` / `persistent-evidence.json`：文件监听、窗口与首尾字节变化；存档内容仅留本地，不回滚、不计算哈希。
- `preparation.json`：确认准备与 Download Helper 恢复时间。

测试 mod 已移除，C/S enabled/control_port.enabled=false，Download Helper 恢复，无残留游戏进程。完整解释见[结果](../../../docs/design/tx-04-07-results.md)，改动与静态依据见[修复说明](../../../docs/design/tx-04-07-retirement-fix.md)，最终纯构建见 [build-02.log](../tx-04-07-retirement-fix/build-02.log)。
