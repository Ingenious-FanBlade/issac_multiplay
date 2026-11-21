> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 当前进度：manager receiver 局部闭合后的阻断状态

记录日期：2026-09-01

当前发布入口仍为 `plan v10 / join progress v4`。本记录只描述后续 create-new 研究进展，不发布 plan v11/progress v5，不修改三个当前索引，也不授予 runtime、active 或 live 权限。

## 当前结论

| 证明或实现项 | 当前状态 | 已建立的结论或阻断原因 |
| --- | --- | --- |
| manager guard 条件公式 | `STATIC_CONFIRMED` | ordinary/possessor 条件、事件号、selector 与 callback 调用顺序已由 v5 局部证明闭合 |
| ordinary spawn receiver 局部公式 | `STATIC_CONFIRMED` | `0x005BB408` 读取 GameRoot slot，`0x005BB40F` 加 `0x1BAA8`，`0x005BB415` 以该 `ECX` 调用 `0x005B9CD0` |
| manager-this 同代身份 | `UNKNOWN` | caller 先行 callee、slot 别名 writer、生命周期/异常和 generation/ABA 未闭合 |
| 完整 manager context | `UNKNOWN` | 九个入口及稳定性 frontier 均未整项关闭 |
| fresh B publication identity | `UNKNOWN` | allocator/ABI、扩容搬迁、存活期、失败和异常路径尚未形成完整原生身份证明 |
| fresh B source preservation | `UNKNOWN` | 全局别名、17 类来源、Init body 与全部消费者的到达定义未闭合 |
| 四项完整同步子证明 | `UNKNOWN` | familiar matching、P0 topology 和完整 manager return 仍未闭合 |
| 完整自然更新窗口 | `UNKNOWN` | 三个 pass、停止/drain/恢复/terminal 期间的保存与生命周期保持尚未证明 |
| prejoin reader 与 generation/ABA | 未实现 | 当前只有设计契约，没有生产 reader 或跨采样点身份实现 |
| 55 槽 runtime、IPC、arm/claim、实机 | `NOT_STARTED`/关闭 | 不在本轮实施边界内，矩阵第 11 项保持 `NOT_STARTED` |

正式 verification v6 的总状态是 `FAILED_MANAGER_THIS_IDENTITY_PREMISE`、退出 1。该状态表示局部 receiver 公式已通过独立静态验证，但完整同一对象前提仍缺证据；它不是生产准入通过。

## 本轮已完成

新增的 `ORDINARY_SPAWN_RECEIVER_FROM_GAMEROOT_SLOT` 子证明确认以下局部定义链：

1. `0x005BB408` 从 GameRoot slot RVA `0x00871678` 读取 4 字节指针到 `ECX`。
2. `0x005BB40E` 只压入 ordinary player type，不改写 `ECX`。
3. `0x005BB40F` 形成 `ECX + 0x1BAA8` 的嵌入 PlayerManager receiver。
4. `0x005BB415` 直接调用 `0x005B9CD0`；四条指令之间没有 call 或 receiver 写入。

独立磁盘重建还确认：

- `0x005BAC40` 的唯一直接 caller 是 `0x002FADA7`；
- caller 从 GameRoot 采样到 manager 调用之前有 39 个先行调用，其中 7 个为目标未知的间接调用，首个未决点是 `0x002FA55E`；
- GameRoot slot 的直接 4 字节 writer 是 `0x00552847`、`0x005597D0` 和 `0x005AB8A9`；
- 没有发现直接宽写或明确的 slot 地址形成；计算别名和间接 writer 没有因此被排除；
- possessor 使用保存的入口 receiver，ordinary 会重新读取 GameRoot slot，两条路径没有被合并成同一个身份结论。

正式验证冻结 523 个输入绑定。7 项 structural 检查通过，1 项 semantic 检查以 `COMPLETED_WITH_UNRESOLVED` 完成，7 项 integrity 检查通过；模型反例 32/32、提交证据反例 44/44、contract 反例 34/34、公开黑盒 6 组/89 断言通过。全部输入复哈希无漂移，显式依赖图为 6 个节点、133 条边、107 个绑定叶，0 自绑定、0 环。

## manager-this 的精确剩余前沿

`frontierManagerThisIdentity` 保留四个未决事实：

- `CALLER_PRECEDING_CALLEE_PRESERVATION`：39 个先行调用中的寄存器、owner 和生命周期影响尚未逐项闭合，7 个间接目标尤其不能按无副作用处理；
- `GAMEROOT_SLOT_ALIAS_WRITER_CLOSURE`：三处直接 writer 已冻结，但派生地址、间接写和外部生命周期写仍未形成完整 universe；
- `SAME_LIVE_GAMEROOT_GENERATION_ABA`：入口采样和 ordinary 重读尚无 adapter-owned generation，地址 A→B→A 不能当作同一对象；
- `LIFECYCLE_AND_EXCEPTION_PRESERVATION`：销毁、替换、异常恢复及回调重入能否到达 ordinary spawn 仍未闭合。

因此 `premisesGloballyEstablished=false`，`entryEstablishedBeforePublication=false`。最终地址相等、写后恢复、常规 ABI 经验或未来 fresh B 读回都不能关闭这些前沿。

## T0-05-04 其余剩余工作

完整 manager context 还需建立 roster 三指针 canonical-zero、device/shared-control pair 生命周期、possessor 排除条件、InputManager/device vtable 运行时身份、reference-control callback、selector/player-type 前态、device predicate state，以及异常和提前退出保持。

publication 身份证明还需闭合外层与嵌套 allocator 的 ABI、non-null/disjoint、constructor failure/no-return、vector 非扩容与扩容 publication、memmove/header 提交、stale alias、vtable slot 和 Init dispatch 身份。当前不得接受 malloc/memmove/free 或 trap/异常 ABI 的经验假设。

fresh B 状态传播还需覆盖 admission v2/v3 冻结的 17 类来源、全局 alias exposure、Init body、151 个 familiar reconcile、13 个 cache callback、直接 spawn/remove 以及累计约 190 个消费者。合法非零 config/effects/trinket 等来源必须保留；最终恢复为零不能消除中间污染。

另外三个同步子证明仍为 `ALL_DESIRED_AND_MATCHING_ENTITIES`、`P0_SOURCE_AND_TOPOLOGY` 和 `FULL_MANAGER_RETURN_COVERAGE`。同步证明之后还需单独完成选择 pass、确认 pass、下一完整 pass，以及停止、drain、恢复、terminal publication 期间的自然更新、readonly、pending/timer/fade 和保存链证明。

生产 reader、generation 失效、永久失能和 Unsafe 保持只完成了规格设计，没有实现或实机验证。owner/vector/device/shared-control 替换、双读漂移、读取失败、重复设备或无法排除 ABA 时，未来实现必须永久失能且不自动重试。

## 建议接续顺序

1. 在纯静态边界内逐项分类 caller 的 39 个先行调用，解析 7 个间接目标，并扩展 GameRoot slot alias/lifecycle writer universe。
2. 将所得静态前提与 adapter generation 采样点、owner replacement 和 ABA 失效规则对齐；若不实现 reader，只能形成条件证明，不能把 manager-this 提升为 `STATIC_CONFIRMED`。
3. 继续关闭其余八个 manager-context frontier，再回到 publication identity。
4. publication 闭合后，完成 fresh B 到达定义、familiar/P0/manager-return 三项同步子证明。
5. 最后证明完整自然更新窗口；生产 reader、runtime 和 live 验证必须作为后续独立阶段处理。

## 冻结产物

- manager receiver sidecar v1：`cs-t0/coop-native-join-manager-receiver-v1.json`，SHA-256 `E4CFFC6C973244D112C28E690EAC46A83E4277A3F10CBDC216389E2C8A752CAE`；
- Init closure v8：`cs-t0/coop-native-join-init-closure-v8.json`，SHA-256 `32EC5EC7FFEA7D749FE07403CFAD24CC0D610F30FC0404D564863B641F2D71F5`；
- admission contract v6：`cs-t0/coop-native-join-admission-contract-v6.json`，SHA-256 `1022B812A6F98CB1D409E5B7E5FA9E8077BDA97C58746A98BE5DB0BEE343F7F4`；
- independent verification v6：`cs-t0/coop-native-join-admission-verification-v6.json`，SHA-256 `E6E59215B4775931DE3DD8E04C765698AE872A412FF1107463FC726F9EA7B5DE`；
- independent test results v1：`docs/archive/refactor/cs-t0/t0-05/native-join-admission-v6-test-results-v1.md`，SHA-256 `A7879CE61DEA8EABDD74CF0DC6B5ED1D561D5611495A7AEB02491ED0F6A6FE84`；
- post-verification audit v1：`out/cs-t0/native-join-manager-receiver-v6-post-verification-audit-v1.json`，SHA-256 `1AA92981EAE9305D68ED7B7D0097907DC1AA35AFA8092B177E3105EA5A147B5F`。

本进度记录不修改生产代码或二进制，不读取或枚举游戏进程，不实现 reader/runtime/IPC/arm/claim，不发布 plan v11/progress v5。所有 active/live、05-05、T0-05 总体和 T0-06 权限继续关闭。
