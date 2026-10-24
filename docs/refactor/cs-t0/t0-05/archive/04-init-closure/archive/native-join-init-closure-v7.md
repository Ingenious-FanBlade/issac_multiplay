> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 Init v7：累计 manager-context 证据

`cs-t0/coop-native-join-init-closure-v7.json` 是 Init v6 的累计扩展，SHA-256 为 `F03155CF0845C58B9F09E05900258166E0A9745E9C6B8CB674230C46C58C7065`。它保留 v6 的 publication 身份、fresh-B 来源/消费者、同步子证明、目标身份和 deny-only 权限，再绑定 manager-context sidecar v1。

v7 合并后有 89 个唯一函数记录；新增 sidecar 自身完整覆盖 22 个 manager/input 函数。新增子证明 `FRESH_B_MANAGER_CONTEXT_TO_ORDINARY_SPAWN` 的边界止于 `0x005BAFFA` 和 `0x005BB415` 两个 spawn 调用，不进入 `0x005B9CD0` 的分配路径。

累计状态为：

- `managerGuardFormulaStatus=STATIC_CONFIRMED`；
- `managerContextStatus=UNKNOWN`；
- `publicationIdentityStatus=UNKNOWN`；
- `freshBStatus=UNKNOWN`；
- `synchronousPropagationComplete=false`；
- `premisesGloballyEstablished=false`、`entryEstablishedBeforePublication=false`；
- reader、runtime、active/live 和下游授权全部为 false。

这个版本只关闭 PE 局部条件公式。它不把 callback 结果、selector 前态、device/shared-control 身份或 generation/ABA 当成已读取事实，也不借 manager 公式升级 v4 publication 子证明。
