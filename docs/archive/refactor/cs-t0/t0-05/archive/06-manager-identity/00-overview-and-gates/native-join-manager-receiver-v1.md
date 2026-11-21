> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager receiver v1：局部定义与身份前提拆分

本证据新增子证明 `ORDINARY_SPAWN_RECEIVER_FROM_GAMEROOT_SLOT`。边界只覆盖 ordinary spawn 调用处 receiver 的局部定义，以及理解该定义所需的 caller、入口保存值和 GameRoot slot 直接 writer 清单；不进入 `0x005B9CD0`、Init、publication 或 consumer 链。

正式 sidecar 是 `cs-t0/coop-native-join-manager-receiver-v1.json`，SHA-256 为 `E4CFFC6C973244D112C28E690EAC46A83E4277A3F10CBDC216389E2C8A752CAE`。它绑定 raw v1（SHA-256 `1044A2C849A4FE158FF91033B7A3A0E7B0EFB2C48412434056F415B50FB74993`）与 reverse draft v1（SHA-256 `D9738B5DB7CD5CA7D9BA0BFE511FA8FCF80603D07E22C533C23588C812623A1D`）。冻结清单包含 5 个函数、2,337 条指令、225 个调用、842 个显式写和 7 个出口。

## 已闭合的局部公式

ordinary 路径的四条连续指令是：

- `0x005BB408: mov ecx, [0x00C71678]`，从 GameRoot slot RVA `0x00871678` 读取 4 字节指针；
- `0x005BB40E: push edx`，压入已选择的 player type，不改写 `ECX`；
- `0x005BB40F: lea ecx, [ecx+0x1BAA8]`，形成嵌入的 PlayerManager receiver；
- `0x005BB415: call 0x005B9CD0`，以该 `ECX` 发起 ordinary spawn。

四条指令之间没有 call，也没有 receiver 写入。因此 `managerReceiverFormulaStatus=STATIC_CONFIRMED`：`0x005BB415` 的 receiver 等于 `0x005BB408` 本次读取的 GameRoot 指针加 `0x1BAA8`。这个结论允许 slot 中的值非零，也没有把调用结果或未来 fresh B 读回当作入口证据。

目标 PE 中 `0x005BAC40` 的唯一静态 caller xref 是 `0x002FADA7`。caller 在 `0x002FA556` 较早读取 GameRoot 到 `EDI`，在 `0x002FADA1` 形成 `EDI+0x1BAA8`，随后调用 manager。manager 在 `0x005BAC68` 保存入口 receiver；possessor 在 `0x005BAFF8` 使用保存值，而 ordinary 在 `0x005BB408` 重新读取全局 slot。该关系已经冻结，但没有升级成跨调用的同一对象证明。

GameRoot slot 的已识别直接 writer 全集是 `0x00552847`、`0x005597D0` 和 `0x005AB8A9`，宽度均为 4 字节。冻结范围内没有直接宽写或明确 address-of 形成；间接 writer、派生别名、异常恢复和外部生命周期仍未闭合。

## 保持为 UNKNOWN 的身份前提

caller 从 `0x002FA556` 到 `0x002FADA1` 之间有 39 个调用，其中 7 个是目标未知的间接调用；首个未决点为 `0x002FA55E`。静态最终值相等、写后恢复或保存寄存器的常规 ABI 约定不能代替这些 callee 的保持证明。

`managerThisIdentityStatus=UNKNOWN`，精确缺口为：先行 callee 保持、GameRoot slot 别名 writer 闭合、同一存活 GameRoot/PlayerManager 的 generation 与 ABA 排除，以及生命周期与异常恢复保持。owner replacement、read failure、drift、callback reentry 或 A→B→A 都必须让未来 adapter generation 失效；本轮没有实现对应 reader。

因此 `premisesGloballyEstablished=false`，`entryEstablishedBeforePublication=false`。本证据只关闭 ordinary receiver 的局部公式，不关闭 manager-context 九个 frontier 中的任何整项。
