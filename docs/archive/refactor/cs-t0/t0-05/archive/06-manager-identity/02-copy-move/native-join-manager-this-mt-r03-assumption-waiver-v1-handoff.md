> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R03 exact2 assumption-waiver candidate v1 交接

日期：2026-09-06  
角色：`POLICY_CANDIDATE_AUTHOR`  
状态：`CANDIDATE_PENDING_INDEPENDENT_TEST`

## 结果

已 create-new 生成 `MT-R03 {0x0001513A, 0x000155A4}` policy-only waiver
candidate，并绑定预封印 spec SHA-256
`2167D5368C43C52EF909AC16AD92C043E2BE77BBBD3F366BD647C5A991C19616`。

两行 strict evidence 均保持 `UNKNOWN / backingExtent OPEN`：

- `0x0001513A` 保留 `R03_EMITTED_COUNT_LE_5000_OPEN_84` 与
  `5001*84=420084 > 420000`；
- `0x000155A4` 保留 `R03_EMITTED_COUNT_LE_5000_OPEN_108` 与
  `5001*108=540108 > 540000`。

当前 policy effect 仅为 `PASS_USER_ACCEPTED_ASSUMPTION_WAIVER_ONLY`。累计 prior R05
与 current R03 共 waived exact3：
`{0x0033570B, 0x0001513A, 0x000155A4}`。人工逆向队列从 copy/move `81`、
memset `17`、总计 `98` 变为 copy/move `79`、memset `17`、总计 `96`。

v33 strict frontier 未修改，仍为 copy/move `82`、memset `17`、remaining `99`。
候选没有把两行改标为静态或运行时证明；frontier composition、manager-this、
whole-image、runtime、active、T0-05、T0-06 与 overall acceptance 全部为 false。

## 产物与作者自检

| 文件 | SHA-256 |
|---|---|
| `out/cs-t0/build-manager-this-mt-r03-assumption-waiver-v1.py` | `587C4CC654BF9F6FBA9CDDA79EE89100B97B9DE5468DB4826D48E42B0DF97D06` |
| `out/cs-t0/native-join-manager-this-mt-r03-assumption-waiver-v1.json` | `A3A659A77B0167E71E63D6D216CBDE15E77103194C490A1273E8914EEA2EF865` |

只运行一次 create-new、一次 verify-existing；两次均退出 `0`、候选 SHA 一致，三项
核心 mutation 全部拒绝。未运行独立 TEST auditor、Cargo、build、宽回归、游戏、进程、
IDA 或调试器。下一步仅由作者分离的独立 TEST 按已封印 contract 审计候选。
