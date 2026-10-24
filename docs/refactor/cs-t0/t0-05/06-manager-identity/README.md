# Manager identity：身份风险与历史研究摘要

更新时间：2026-10-16；整理：tx-01-44。执行约定以[现行项目规范](../../../project-conventions.md)为准。

## 当前用途

本主题保留 caller 入口 manager、ordinary spawn 重读 GameRoot/PlayerManager、alias/bulk writer 和 generation/ABA 的资料。局部公式成立不证明所有对象属于同一存活 generation；指针数值不变也不足以排除替换与 ABA。

当前自有 ABI 路线和 v18 的限定正常路径不要求先清零全映像 bulk/alias 账本。这里的严格计数是历史事实，后续按实际依赖使用；归档不把 UNKNOWN 改判为已证明。

| 历史结论 | 保留状态 |
| --- | --- |
| manager guard / ordinary receiver 局部公式 | STATIC_CONFIRMED |
| manager-this 同代身份 | UNKNOWN |
| v33 copy/move | 323/405 正向接受，82 remaining |
| v33 memset | 225/242 正向接受，17 remaining |
| strict remaining | 99；负向审计覆盖不等于正向证明 |
| MT-R03 / R05 | exact3 用户接受风险的 policy-only waiver，strict verdict 仍 UNKNOWN |
| 人工策略队列 | 96；MT-R02 exact2 仍 UNKNOWN，未获 waiver |

## 按问题追溯

- [最后的 MT-R02 检查点 v41](../archive/06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r02-animation-descriptor-checkpoint-v41.md)：animation descriptor count/backing 的具体未知。
- [MT-R03 用户假设](../archive/06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r03-user-assumption-policy-checkpoint-v40.md)与[MT-R05 用户假设](../archive/06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r05-user-assumption-policy-checkpoint-v39.md)：policy 与严格证据的区别。
- [v33 frontier](../archive/06-manager-identity/01-bulk-and-whole-image/native-join-manager-this-bulk-remaining-frontier-v33-handoff.md)：647 行账本的历史接受集合。
- [Reader / generation / context 索引](../archive/06-manager-identity/00-overview-and-gates/README.md)。
- [Copy/Move](../archive/06-manager-identity/02-copy-move/README.md)、[Fill/Allocation](../archive/06-manager-identity/03-fill-and-allocation/README.md)、[Manager tuple](../archive/06-manager-identity/04-manager-tuple/README.md)、[辅助审计](../archive/06-manager-identity/05-supporting-audits/README.md)。

旧 proof breakdown、暂停检查点、预封与 formal v7 发布安排完整归档，不再作为当前任务清单。原始越界反例、未决生命周期与失败仍可用于实现说明；不要因为取消流程而声称这些事实已闭合。
