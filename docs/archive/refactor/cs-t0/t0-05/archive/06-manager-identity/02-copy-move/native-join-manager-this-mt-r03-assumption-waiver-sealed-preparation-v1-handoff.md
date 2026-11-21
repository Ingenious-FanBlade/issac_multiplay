> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R03 assumption-waiver 候选前封印交接 v1

日期：2026-09-06  
角色：独立 TEST  
状态：`READY_FOR_UNSEEN_MT_R03_ASSUMPTION_WAIVER_CANDIDATE`

## 1. 封印结论

本轮为 `MT-R03` exact2 `0x0001513A / 0x000155A4` 建立候选前封印。
TEST 未读取、import 或编写任何未来 waiver candidate/builder。

两行严格证据均保持 `UNKNOWN / backingExtent OPEN`：5000 比较只记录后继续，
保留反例分别为 `5001*84=420084 > 420000` 与
`5001*108=540108 > 540000`。用户长期游玩未观察到类似崩溃只登记为
`USER_ACCEPTED_ASSUMPTION / USER_DECISION_NOT_EVIDENCE`，不得改称
`STATIC_CONFIRMED_NO_OVERLAP` 或 runtime 证明。

v33 strict frontier 不改，strict copy/move `82`、memset `17`、总计 `99`。
在已独立接受 MT-R05 exact1 waiver 的 policy 视图上，本候选若以后通过独立 TEST，
人工逆向队列才从 98 降至 96：copy/move `79`、memset `17`。waiver 不授予
frontier composition、manager-this、whole-image、runtime、active、T0-05 或 T0-06 权限。

## 2. Create-new 产物与 SHA-256

| 产物 | SHA-256 |
| --- | --- |
| `out/cs-t0/native-join-manager-this-mt-r03-assumption-waiver-sealed-acceptance-spec-v1.json` | `2167D5368C43C52EF909AC16AD92C043E2BE77BBBD3F366BD647C5A991C19616` |
| `out/cs-t0/audit-manager-this-mt-r03-assumption-waiver-sealed-v1.py` | `B578468968F001D956A34654E96F1140B8A11CE33AF0D30716C42B1D89B2BEC9` |
| `out/cs-t0/native-join-manager-this-mt-r03-assumption-waiver-sealed-preparation-v1.json` | `C4E47E079CAE5D40788F892C9E2AC8CED097BAF2384E2E3A9BF774E60CF8F52A` |

封印绑定 v37、exact5 independent audit v2、v33 strict frontier、v39 MT-R05
policy checkpoint 与 MT-R05 waiver independent audit，并从冻结 PE 独立核对两处 call
bytes 和 `memcpy` target。

## 3. 极简验证

- preparation create-new 一次：退出 0；
- verify-existing 一次：退出 0且字节一致；
- 三类 mutation 全拒绝：`target-or-exact2-scope-drift`、
  `assumption-mislabeled-as-static-or-runtime-proof`、
  `cumulative-queue-or-permission-escalation`；
- wrong-spec-hash 一次：退出 1，`spec requested hash drift`；探针输出前后不存在。

未运行 Cargo、build、历史宽回归、游戏、进程访问、调试器或 IDB 操作。

create-new 命令：

```powershell
python out\cs-t0\audit-manager-this-mt-r03-assumption-waiver-sealed-v1.py `
  --spec out\cs-t0\native-join-manager-this-mt-r03-assumption-waiver-sealed-acceptance-spec-v1.json `
  --spec-sha256 2167D5368C43C52EF909AC16AD92C043E2BE77BBBD3F366BD647C5A991C19616 `
  --prepare `
  --output out\cs-t0\native-join-manager-this-mt-r03-assumption-waiver-sealed-preparation-v1.json
```

verify-existing 使用同一命令仅追加 `--verify-existing`。未来 candidate 必须使用
`cs-t0.manager-this.mt-r03.assumption-waiver.candidate.v1`，精确绑定本 spec 并保持所有
严格证据与权限字段不提升。
