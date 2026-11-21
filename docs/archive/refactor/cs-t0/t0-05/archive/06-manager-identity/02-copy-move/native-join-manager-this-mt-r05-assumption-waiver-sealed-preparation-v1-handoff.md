> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# MT-R05 assumption-waiver 候选前封印交接 v1

日期：2026-09-06  
角色：独立 TEST  
状态：`READY_FOR_UNSEEN_MT_R05_ASSUMPTION_WAIVER_CANDIDATE`

## 1. 封印边界

本轮只为 exact1 `MT-R05 / 0x0033570B` 建立用户 assumption waiver 的候选前
验收边界。TEST 未读取、import 或编写任何未来 waiver candidate builder，也未创建或
读取未来 waiver candidate。

用户接受的标签固定为 `USER_ACCEPTED_ASSUMPTION`。它与
`STATIC_CONFIRMED_NO_OVERLAP` 不等价，也不是 `RUNTIME_OBSERVED`：用户基于长期游玩
未观察到相关崩溃而承担残余风险，只允许将该行从人工逆向队列移除。

v38 的静态 verdict 逐字保留为 `UNKNOWN`，first-open 仍为
`countAndNoWrap / R05_AUX_NONEMPTY_FILTERED_EMPTY_CONJUNCTION_OPEN`，countermodel 仍为
`auxiliaryRangeCount>0 && filteredCount==0 yields raw transfer length 0xFFFFFFFC`。
v33 machine frontier 仍保留该行且 remaining 仍为 99。本 waiver 不修改机器 frontier，
不产生静态或运行时证明，不自动授予 manager-this、whole-image、runtime、active、
T0-05 或 T0-06 权限。

## 2. Create-new 产物

| 产物 | SHA-256 |
| --- | --- |
| `out/cs-t0/native-join-manager-this-mt-r05-assumption-waiver-sealed-acceptance-spec-v1.json` | `4977DBEE6D2DDAC78E5B25EF9BEE87B9D0D27DCE3DB8BF49FD595FED76D387AE` |
| `out/cs-t0/audit-manager-this-mt-r05-assumption-waiver-sealed-v1.py` | `A01E08CE153072E86F565B21DCDE23F2E3F574FFACF30EF839B500B40F2FCC9B` |
| `out/cs-t0/native-join-manager-this-mt-r05-assumption-waiver-sealed-preparation-v1.json` | `8181ED97110240B9D671039062971AE29BDD1D8101D34297824A82631EE6E357` |

Spec/validator 绑定冻结 PE、v38 checkpoint、v38 对应的独立静态 audit 与 v33
frontier。冻结 PE 的 SHA-256 为
`1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`；
validator 独立重算 PE32/I386、ImageBase、SizeOfImage、call bytes
`E8 AD B1 3B 00` 及 `memmove` target `0x00AF08BD`。

## 3. 压缩验证结果

只执行以下正式验证：

1. preparation create-new 一次：退出 0，SHA-256
   `8181ED97110240B9D671039062971AE29BDD1D8101D34297824A82631EE6E357`；
2. 同一 preparation verify-existing 一次：退出 0，字节一致；
3. 三个不同决策机制的最小 mutation 全部拒绝：
   `target-or-scope-drift`、`assumption-mislabeled-as-proof`、
   `waiver-effect-or-permission-escalation`；
4. wrong-spec-hash 一次：退出 1，`spec requested hash drift`；探针输出在调用前后
   均不存在，满足 fail-before-write。

create-new / verify-existing 命令：

```powershell
python out\cs-t0\audit-manager-this-mt-r05-assumption-waiver-sealed-v1.py `
  --spec out\cs-t0\native-join-manager-this-mt-r05-assumption-waiver-sealed-acceptance-spec-v1.json `
  --spec-sha256 4977DBEE6D2DDAC78E5B25EF9BEE87B9D0D27DCE3DB8BF49FD595FED76D387AE `
  --prepare `
  --output out\cs-t0\native-join-manager-this-mt-r05-assumption-waiver-sealed-preparation-v1.json

# 第二次仅追加：
--verify-existing
```

没有运行 Cargo、全量 build、历史宽回归、游戏、进程访问、调试器或 IDB 修改。

## 4. 后续候选门

未来 candidate 必须使用 schema
`cs-t0.manager-this.mt-r05.assumption-waiver.candidate.v1` 并精确绑定本 spec。候选只能请求
`USER_ACCEPTED_ASSUMPTION` 的人工队列 waiver，必须继续报告静态 `UNKNOWN`、零
`STATIC_CONFIRMED_NO_OVERLAP`、零 runtime observation、v33 frontier 未变和全部权限为
false。候选不得 self-promote；只有独立 TEST 用同一封印 validator 完成候选审计后，才能
发布限定状态 `PASS_USER_ACCEPTED_ASSUMPTION_WAIVER_ONLY`。
