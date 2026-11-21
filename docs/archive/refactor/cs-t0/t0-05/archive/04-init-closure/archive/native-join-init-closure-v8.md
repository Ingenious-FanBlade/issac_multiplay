> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 Init closure v8：累计 manager receiver 章节

`cs-t0/coop-native-join-init-closure-v8.json`（SHA-256 `32EC5EC7FFEA7D749FE07403CFAD24CC0D610F30FC0404D564863B641F2D71F5`）在 Init v7 的累计证据上新增 `managerReceiverIdentity` 章节，并只读保留此前 publication、fresh B、manager guard、persistence 和 cache/P0 证据。

新增章节把两个不同强度的结论分开：

- `managerReceiverFormulaStatus=STATIC_CONFIRMED`，表示 ordinary 调用处四条连续指令可以推出本次 receiver 的局部定义；
- `managerThisIdentityStatus=UNKNOWN`，表示尚不能推出 caller 入口、manager 保存值和 ordinary 重读值属于同一 generation 的存活 GameRoot/PlayerManager。

章节固定唯一 caller `0x002FADA7`、入口身份链、39/7 个先行调用、三个 GameRoot slot 直接 writer、身份失效条件和 `frontierManagerThisIdentity`。四个未决事实仍是 `CALLER_PRECEDING_CALLEE_PRESERVATION`、`GAMEROOT_SLOT_ALIAS_WRITER_CLOSURE`、`SAME_LIVE_GAMEROOT_GENERATION_ABA` 与 `LIFECYCLE_AND_EXCEPTION_PRESERVATION`。

Init v8 不声明入口前提已经在 publication 前建立，也不声明完整同步传播闭合。publication、fresh B、P0、matching familiar、完整 manager 返回、自然更新窗口和 reader/ABA 都保持原状态；全部 runtime、active、live 与下游解锁权限继续为 false。
