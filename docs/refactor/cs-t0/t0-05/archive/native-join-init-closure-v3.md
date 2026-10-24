> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-04 普通 Player 初始化来源补证 v3

- 日期：2026-09-01；create-new，只读静态逆向交接。
- 整体仍为 **BLOCKED_BY_EVIDENCE**。新增的是具体来源写入、读取和局部路径；没有完整 Init、全部 desired-familiar 或全窗口无污染证明。
- [机器证据](../../../../../cs-t0/coop-native-join-init-closure-v3.json) SHA-256：`17E9EA8DB6EC4F59FE19B89FAD587916A83B88BFB54783FB766501C11BC9B9BF`。
- 30 个新增窗口、34 个 reader 布局条目是内容清单，**不是语义闭合或验收通过数**。独立验收交给另一个实例；本文作者没有测试验收自己的结论。
- 未启动、枚举、打开、附着或控制游戏进程；未修改 IDB、PE、旧证据、生产代码或 reader/runtime。未触发输入、Hook、注入、arm、claim 或实机授权。

## 1. 目标与继承边界

当前只读 IDA 目标仍为 x86，preferred base `0x00400000`、image size `0x008FE000`；canonical SHA-256 `C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30`，approved-unpacked/IDA SHA-256 `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`，equivalence SHA-256 `8B3FB097ED32B18DE6A78F535045ACD525185569ACA029A6370494511851F138`。本文地址均为 RVA；本机目标路径不进入机器证据。

新 JSON 直接绑定 plan v8、progress v2、Init JSON/report v2、技术报告 v1，以及 persistence-window JSON/report v1；不与同时编制的 window v2 相互绑定。旧 v1/v2 及其独立失败记录不改写。

候选仍为普通显式 seed、新起始房、无 mods/相关外部回调与物理 gameplay/menu 输入、Keyboard P0、唯一未分配 XInput B、普通 Isaac。此前冻结的配置、存档解锁/直接计数、seed-effect、callback、owner 和生命周期限制全部继承，不因新补证减少。B 尚未构造时没有可供准入读取的 B 对象；必须从 constructor/Init 推导，不能用创建后的空值读回补事前证明。

已证 readonly setter 守卫继续复用。本文不重复展开所有 achievement threshold，也不把 readonly 扩展为直接统计写、GameStateSave、room flush 或任意 Steam 调用的通用保护。

## 2. 本轮确认的来源链

### 容器、数组与 active slots

`353DA0` 构造函数建立：

- effects owner `Player+1508` 的 begin/end/capacity 为 `+150C/+1510/+1514`，均为零；`+1518/+1519` 两个 byte 为零，`+151C` 为 Player 回指。
- raw collectible `+16C8/+16CC/+16D0`、pill history `+16D4/+16D8/+16DC`、smelted `+1748/+174C/+1750` 三指针为零。
- 四个 stride `0x20` active slot 从 `+1580` 起清零；Init 的 `3563BC/3563FB/35642E/356464` 再次将各 slot 的 ID 所在前 16 字节写零。
- extra collectible map 的 `+1F80` 是分配的 24-byte sentinel，`+1F84` 是零 size；三个链接回指自身，sentinel 的 word `+C=0101`。它不是 vector，没有可猜测的 capacity 指针。
- extra collectible/trinket byte vector 的头分别为 `+1FD4/+1FD8/+1FDC` 和 `+1FE0/+1FE4/+1FE8`；原始指令先把 EDI 调整到 `Player+1FC8`，不能将后续小位移误当 Player 直接偏移。

Init `3560C8..35622B` 根据配置指针表数量 resize raw、smelted、pill history，并逐项清除整个 used range。smelted 的 stride 是 4，但每项由两个 **signed i16** 组成，getter 的贡献是 ordinary + 2×golden；不是一个普通 unsigned count。Init `3575EC..357660` 对两个 extra byte vector 同样清除整个 used range。

`3310E0`、`3DCCD0` 和 smelted grow `3DE9F0` 的第三指针参与容量比较/写回，提供容量字段的原始依据。配置源计数读取为：

| 目标 | onlineRoot begin/end | 单项大小 |
| --- | --- | ---: |
| raw 与 extra collectible | +2A404/+2A408 | 4-byte config pointer |
| smelted 与 extra trinket | +2A410/+2A414 | 4-byte config pointer |
| pill history | +2A434/+2A438 | 4-byte config pointer |

这些是成功返回、合法有界容器上的局部归零事实。**原生 resize 后通常是有分配且元素全零，不是三个指针都为零。** 源配置表自己的完整容量/生命周期、分配失败处理和后续所有 callee 的保持尚未闭合；原生分配器整数上限也不是可执行 reader 的安全读取预算。

### 空 effects 的具体短路

Init `355B3E → 5306C0(1)` 使用 `Player+1508`。该 helper 在 begin=end 时于 `5306F7` 跳过循环 body，不执行 effect 移除、memmove 或相关后继 helper。`530980 → 530A70` 的 item count 和 `530AE0` 的 pill-effect count 在相同空来源下返回零。

这是“输入仍为空时该 helper 不进入副作用 body”的局部证明。构造零写与每次调用之间全部 callee 都保持空，仍需单独证明；没有将空值来源提升为整条 Init 无回调。

### 新增的 pending trinket 来源

`3CB935` 读取 `Player+17C4` 的 pending item-config pointer。指针非空、record type=2、ID 匹配时，temporary trinket `+2EF8` 为零或匹配均允许贡献一；`3CB958` 的 **byte** `+17CC bit2` 再贡献一。

因此 slots、smelted、byte-vector 和 temporary ID 都空，仍不足以证明完整 trinket-empty。Init `3562F9` 明确写 `+17C4=0`，为 fresh B 提供了该来源的原生零起点；这是可继续组合的正向事实，不是候选必然失败。对 P0 必须另行要求 bounded reader 读到 null 并证明窗口保持。采用 null guard 时无需解引用 config，也不应强行要求未使用的 flags 为零。

JSON 的 insufficientPredicateWitnesses 只给出该条件分支的数值赋值，**不声称正常候选已经可达这些赋值或发生污染**。

### effective inventory 之外的 desired-familiar 来源

Init `356040/356042` 先 xor EAX，再以一个 DWORD 零写 `Player+1382`，同时清除两个 signed words：

| 字段 | cache 读取 | 直接 desired 消费 |
| --- | --- | --- |
| Player+1384，i16 | 36DD95 | 36DDA5 → familiar variant121 |
| Player+1382，i16 | 36ECE2 | 36ECF3 → familiar variant103 |

相邻 `+1380` 由 RNG 计算另行初始化，不能把周围整个结构视为全零。即使 effective inventory 已空，也需要这两个 carry 的来源与保持证明。

此外，`36DE35` 取 `Player+16D4`，`36DE3D` 调无 bounds check 的 `02C810` 读取 pillHistory[10]，然后与 Count(item10)、pill-effect107 和 clamp helper 组合，流入 `36DE9B` 的 familiar33 desired。只读取 scale 使用的 pillHistory32/33 会漏掉这个来源。本轮冻结 owner/index/数据流，不宣称全部 clamp/desired 算式已经闭合。

`371200` 的原始完整函数进一步支持限域角色 grant：空 roster 跳过 online identity map，PlayerType0 不命中特殊角色分支而返回 false。这个结论只覆盖角色 grant helper，不能代替其余 implicit flags、special seed、related Player、map 或 pending-record 来源。

## 3. familiar-empty 不能伪装成 Player 自有 vector 为空

完整 `363370` 显示，reconcile 从 `GameRoot+18300` 子对象的 `+1218` 通过 `01AF60` 查询房间实体 Type3/variant，再按下列字段筛选：

- Familiar `+DC8=0`、`+D76=0`、`+172!=0`，均为 byte；
- 请求 subtype 非负时，与 Familiar `+30` 对应；
- Familiar `+410` 或非空 `+414` 的 Player owner 匹配，均为 pointer32。

desired 会被钳制到非负，但 **desired=0 时已有匹配 familiar 仍在 `363442` 调 virtual-remove**。只有 desired=0 且查询结果中没有匹配实体，才能排除 remove/spawn/virtual-init。

新 reader 条目把 underlying room population storage、完整有界枚举、容量和生命周期明确留为 UNKNOWN。不能靠读取一个未证存在的 Player-owned vector 来声明无熟悉物，也不能用普通 Isaac 的外观替代这个证明。

## 4. 实际推进范围与仍未闭合的义务

本轮没有发现一条已经证明由完整正常候选可达的污染反例。已确认的是上述新来源与条件分支；覆盖不足和反例分开记录。

保留四项具体 UNKNOWN：

1. **完整中间源保持**：constructor/Init/tail/cache 中的健康、配置、共享资源、reconcile 等 callee 是否保持所有相关零值，尚未形成完整有序摘要；零写位置不等于稍后 getter 必得零。
2. **全部 desired 与现有实体集合**：本轮只覆盖具体新增 carry/pill/pending 来源，没有完成 cache 全部 reconcile 参数及其他实体处理的组合证明，不再因数量庞大而无限扩大本轮范围。
3. **已存在 P0**：P0 的输入源、共享资源更新遍历全部 main players、相关生命/coin-heart 路径和房间实体拓扑，不能用 fresh B 的构造事实代替。
4. **生命周期与持久化组合**：本轮局部结果不能证明完整 Init 不写 timer/request/fade/pending/readonly 或直接 persistent；须与独立 natural-window 证据组合。已知 readonly-dominated setter 继续复用，不为此重新研究所有 health threshold。

readerLayoutAdditions 只冻结字段宽度、容器布局和已知来源，**不实现进程 reader**，不创建运行授权，也不提前关闭 generation/ABA。完整契约应保持字段读取失败、边界不足、未知 owner 与采样间替换均 fail closed；相邻相同快照不能排除 ABA，更不能排除此前已经 flush。

## 5. 独立交接

每个新增 INSTRUCTION_WINDOW 采用旧结构：RVA、完整 expectedBytes、逐条 size/mnemonic、direct REL8/REL32 edges、decodeStart/End 与 function-entry decode context。提取使用只读 decode_insn/get_canon_mnem；单字节 CC 明确记为 int3，不依赖可能为空的 IDA 文本助记符。旧锚点通过绑定复用，不复制后改判。

独立测试应重新核对磁盘目标等价、原始字节、边界、callee 和上述关键字段/分支，再审查局部结论与 UNKNOWN 是否相符。即使这些新增静态事实独立通过，也不意味着 sourcePropagation 全闭合、55 槽 runtime 已实现、05-04 PASS 或任何 live/unlock 资格。

