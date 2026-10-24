> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R05 assumption-waiver candidate v1 交接

日期：2026-09-06  
角色：`POLICY_CANDIDATE_AUTHOR`  
状态：`CANDIDATE_PENDING_INDEPENDENT_TEST`

## 结果

已 create-new 生成 exact1 `MT-R05 / 0x0033570B` assumption policy overlay
candidate，并精确绑定候选前封印 spec SHA-256
`4977DBEE6D2DDAC78E5B25EF9BEE87B9D0D27DCE3DB8BF49FD595FED76D387AE`。

双轨状态保持为：

- strict evidence：`UNKNOWN`，`countAndNoWrap=OPEN`，countermodel
  `auxiliaryRangeCount>0 && filteredCount==0 yields raw transfer length 0xFFFFFFFC`；
- policy effect：`PASS_USER_ACCEPTED_ASSUMPTION_WAIVER_ONLY`；
- strict v33 machine frontier：仍为 remaining `99`，`0x0033570B` 仍在其中，未修改；
- manual reverse queue：仅移除 MT-R05，从 `99` 变为 `98`；copy/move 人工队列从
  `82` 变为 `81`，memset 保持 `17`。

候选没有使用 `STATIC_CONFIRMED_NO_OVERLAP`、`RUNTIME_OBSERVED` 或
`RUNTIME_CONFIRMED` 作为 MT-R05 的证据 verdict。manager-this、whole-image、runtime、
active、T0-05、T0-06 与 overall acceptance 全部为 false。

## 产物

| 文件 | SHA-256 |
|---|---|
| `out/cs-t0/build-manager-this-mt-r05-assumption-waiver-v1.py` | `6A9550A6B11A023A8B137B8622C9F866F96898C2296970FA2EC829542493C81B` |
| `out/cs-t0/native-join-manager-this-mt-r05-assumption-waiver-v1.json` | `04B713B932028873756C860057D138B2F1E63EFF481B812CF0B369479EBD629A` |

## 压缩作者自检

```text
python out\cs-t0\build-manager-this-mt-r05-assumption-waiver-v1.py
python out\cs-t0\build-manager-this-mt-r05-assumption-waiver-v1.py --verify-existing
```

两次均退出 `0`、候选 SHA 一致；三项核心 mutation 全部拒绝。未运行独立 TEST
auditor、Cargo、build、宽回归、游戏、进程、IDA 或调试器。下一步仅由作者分离的独立
TEST 按已封印 contract 审计该公开 candidate。
