> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 Init v5 fresh B reaching-definition evidence

本证据只记录普通确认路径上 fresh B 的原始指令、定义到达与别名边界，供独立磁盘重建与公式审查。正式组件是 `cs-t0/coop-native-join-init-closure-v5.json`，SHA-256 为 `E86E086D75C3AC103042816D70D8E02751FFBB53D7EEC3640348AF11A8C1C308`。它不实现 reader/runtime，不覆盖 P0、Room matching 结果、出生 Effect、完整 manager remainder、自然更新窗口或 generation/ABA，也不把结构校验当作原生语义证明。

## 结论状态

`FRESH_B_SOURCE_PRESERVATION` 仍为 `UNKNOWN`。这不是候选前提下已证可达污染反例；目前存在的是精确的证明缺口。合法非零来源被保留，尤其是 temporary flag 的构造值 1、`Player+0x1380` 的 RNG 值以及 config/special-seed 派生值。本稿从未使用“fresh B 整体为零”前提。

正式 v5 登记 17 个来源、190 个实际消费者和 70 个局部 definition writer。独立审查从目标盘重建并匹配 47,694 条指令、5,276 个 call、19,679 个 write 与 366 个 exit，复核 publication、Effects 空分支、14 个 callback、17 个 tail external source 和全部新增 writer；组合后仍保留 604 个 unresolved。该结果只批准为合法 `UNKNOWN` 包，不是 fresh B 原生闭合。

## 分配、构造与首次 publication

`0x005B9E2E` 申请 `0x2F00` 字节，`0x005B9E46` 调 `0x00353DA0` 构造 Player。正常路径上的具体对象指针继续流入主玩家向量，但 allocator 的 null、地址复用、与仍存活对象不别名条件尚未冻结；构造和 Init 内的嵌套容器/Weapon 分配也各自保留失败与别名义务。

首次 publication 发生在 Init 之前，且有两个互斥的真实提交点：

- 非满路径由 `0x005B9EA6 cmp eax,[edi+8]` / `0x005B9EA9 jz` 选择，`0x005B9EAB` 先写元素指针，`0x005B9EAD` 再提交 vector end。
- 扩容路径从 `0x005B9EBA` 调 `0x0002C920`；新缓冲区插入、复制后进入 `0x0001D150`，`0x0001D193` 提交新 vector end。`0x0001D18E` 的 begin 与 `0x0001D193` 的 end 之间没有 call，但该路径的 allocator/异常语义仍未闭合。

两条路径均在 `0x005B9F1F` 的 Player vtable+4 Init dispatch 前使 B 可由 `playerManager.mainVector` 枚举。此后仅凭“当前 callee 没有显式 B 参数”不能排除写入 B；全局玩家遍历、related-player 遍历、HUD/helper 与 callback 都可能重新取得该指针。

## 已冻结的局部定义

定义清单按 17 个 v2 source channel 建立稳定 `definitionId`。代表性局部事实如下：

- effects：构造在 `0x00354036/40/4A/54/5E` 建立空 header/flags，并在 `0x00354067` 保存回指 B。空 header 只证明 effects clear 的短路输入；不证明之后所有调用保持为空。
- raw/pill/smelted：构造分别建立三个 canonical-zero vector header；Init resize 后在 `0x00356102`、`0x00356217`、`0x003561D4` 对每个已用 dword 清零。已分配全零范围不等于 null header，也不覆盖 capacity 外字节。
- active slots：constructor 以 ECX=4、stride `0x20` 执行 `0x00354080/89/90/97/9E/A5/AC/B3` 八个 dword writer heads；Init 又在 `0x003563BC/E3`、`0x003563FB/0x00356416`、`0x0035642E/4C`、`0x00356464/6F` 向四个槽的前后 16 字节分别写零。writer heads 已完整列出，但四轮循环保持和后续别名仍为 `UNKNOWN`。
- byte vectors：constructor 在 `0x003547E7/EE/F5` 与 `0x003547FC/0x00354803/0A` 建立两个 canonical-empty header；Init 的 `0x00357614` 与 `0x00357652` 清每个已用 byte。动态 allocation 使用独立 ownerId，不能用 freshB inline range 代替。
- temporaryIds：`0x00354950` 合法写 1，`0x00354957/61` 写两个零 ID。
- trinket/pending：`0x0035614A/54` 清两个 slot；`0x003562F9` 清 pending config pointer。相邻 `+0x17CC` flags 没有被该 dword 写覆盖。
- related players：构造清 `+0x1E68/+0x1E6C`，Init 在 `0x003574A1` 再清 `+0x1E6C`；tail 的 `0x003BBF4F/68` 是路径相关覆盖，不能继续沿用 constructor definition。
- familiar carry：`0x00356042` 一次 dword 写清 `+0x1382/+0x1384` 两个 signed word；`0x00355E95/9F` 清两个 native counters，`0x00356139` 清 `+0x180C..+0x181B`。相邻 `+0x1380` 不是零。
- implicit grants：`0x003576A6` 清 `+0x2028`，`0x003576B0` 清 `+0x202C`，`0x00355B67` 清 secondary suppression `+0x20A9`。`0x0036EBD0` 的公式同时依赖 `+0x202C/+0x20A9/+0x180C`，因此 sourceIds 是 `implicitGrants` 与 `familiarCarry`，不是单一 counter 来源。
- extra map：`0x00354736/3C` 清 head/count，`0x00354743` 申请 24 字节 sentinel，`0x0035474B/4D/50/53` 初始化动态 node，`0x00354759` 提交 head；allocator/null/alias 均继续为 `UNKNOWN`。
- configStartingItems 与 specialSeedEffects 是 preexisting/global 条件来源，没有伪造 B 内部 definition writer；其有效值允许非零。

所有局部 definition 的 `completenessStatus` 仍为 `UNKNOWN`。每个动态 owner、未知 alias write、合法覆盖和后续 callback 都保留在 writer/callee/frontier 中；没有用“最终又恢复相同值”恢复旧 reaching definition。

## 190 个消费者

consumer universe 与 cache sidecar 逐项交叉：

- 151 个 `RECONCILE_ARGUMENT`，每条保留原 `desiredExpression`、callsite、宽度、符号和 receiver。普通条目 ownerId 为 `currentPlayer`；六个 pair-table 条目为 `mainPlayerIteration`。其中 147 条保守映射到全部 17 channel，三条是 effects/familiarCarry 单源，`0x0036EBD0` 是 implicitGrants/familiarCarry 双源；这些映射本身不构成 reaching-definition 证明。
- 8 个 `DIRECT_FORMULA` 实际消费点，覆盖 `DIRECT519`、birth/role/effect、secondary recursion 三类公式。它们分别记录 desired/query、remove、spawn、bucket、recursive cache、Effect spawn/tail 与 related-player recursion，而不是用三个聚合占位替代八个程序点。
- 14 个 `CALLBACK_EXPOSURE`：Init 末尾 callback 9 一处，以及 cache 的 callback gate 十三处。
- 17 个 `TAIL_EXTERNAL_SOURCE`：type-0 config/appearance 与 special-seed/achievement 的实际调用点；`0x003BC6CD` 同时映射两个外部来源但只作为一个实际消费者计数。

所有 190 条的 `reachingDefinitionIds` 为空且 disposition 为 `UNKNOWN`，因为尚不能为任一条声称唯一、字节完备且无中间覆盖的 definition 集。该空集合和逐项 unresolved 是有意的失败表达，不是遗漏。

## callback 与外部来源 frontier

Init 在 `0x00357959` 调 `0x00464050`。该 callee 从静态 registry owner 链取得 bitset，并在 `0x0046409F` 测 `u32(bitset+0) & 0x200`；置位时把 fresh B 传给 callback id 9。admission v2 没有建立该 owner/bitset 身份、0x200 mask 清零或 publication 后的保持。

cache sidecar 另枚举 13 次 `0x00463E90`，门条件为 `u32(bitset+0) & 0x100`。它同样未由 v2 全局建立。即使入口声明“无 mods”，也不能替代实际 owner/mask 读取和所有 callback body 的副作用摘要。

`0x003BBBD0` 的 type-0 路径仍会进入 appearance lookup/lazy load、资源/初始状态 helper，以及 special-seed/achievement 分支。新增覆盖包含 `0x00294DE0`、`0x003BEBB0` 和相关 helper，但 config record 身份、loaded/lazy 两条路径、有限无环资源图与 transitive writes 尚未组合。未来 B 的读回不能充当前置证据。

## 精确 proof frontiers

1. `DYNAMIC-VECTOR-RESIZE-AND-ZERO-LOOPS`：constructor 的 canonical-empty header 与 Init 的 zero-loop writer heads 已证；resize bounds、allocator/null/alias 和每次循环迭代未闭合。
2. `ACTIVE-SLOT-CONSTRUCTOR-FOUR-ITERATION-INVARIANT`：八个 constructor writer heads 与八个 Init XMM writer heads 已证；四轮循环组合和后续 alias 保持未闭合。
3. `FRESH-B-PUBLISHED-BEFORE-INIT-GLOBAL-ALIAS`：两条 branch-specific publication commit 已证；publication 后第一个可遍历 `playerManager` 的 Init/callee 及所有 global writer 未闭合。
4. `INIT-CALLBACK9-MASK-OWNER`：callback 9 callsite 和 0x200 gate 已证；registry owner/bitset 身份与 taken body 未闭合。
5. `CACHE-CALLBACK13-MASK-OWNER`：13 个 cache callback callsite 和 0x100 gate 已证；每次 owner/bitset 读取及 side effect 未闭合。
6. `TYPE0-CONFIG-LAZY-RESOURCE`：17 个 tail external-source 消费点已证；`0x003BBC81` lookup/lazy load 与后续 config/seed helper 的 transitive effect 未闭合。
7. `FRESH-B-ALLOCATION-WEAPON-FACTORY`：Player allocation size 与选定局部写已证；allocator/factory 的 null、disjoint-live-object、异常和嵌套分配契约未闭合。allocator 没有提供 payload-zero 保证，因此 `allocationProof.zeroedRanges=[]`、`identityBeforePublication=false`。
8. `FRESH-B-CONSUMER-REACHING-DEFINITIONS`：190 个 consumer 与 70 个局部 definition writer 的结构映射已证；逐 consumer 的唯一 accepted definition 集和 byte-complete no-intervening-write 组合未闭合。

因此 `premisesGloballyEstablished=false`、`entryEstablishedBeforePublication=false`、`freshBSourcePreservationComplete=false`。其它三个同步 subproof 继续继承 v4 的 `UNKNOWN`，不随本证据的局部进展升级。

## 权限、发布与索引

本轮没有进程检查、注入、实机操作、runtime reader、IPC、arm/claim 或 active/live 权限；`runtimeObserved=false`、`activeJoinEligible=false`，矩阵第 11 项仍为 `NOT_STARTED`。plan v10、join progress v4 及三个当前索引均未修改；plan v11 / progress v5 未发布，05-05、T0-05 整体与 T0-06 继续关闭。

主要原始依据是 `out/cs-t0/native-join-init-v5-targeted-inventory-raw-v1.json`（SHA-256 `A9363BD7C0A190B0D522DA808FEDFDD3FBED9549C93EFA21C54CA0BDDF571575`）、`out/cs-t0/native-join-init-v5-targeted-pseudocode-raw-v1.json`（SHA-256 `AAA7A3A6A638B77CE61C094583CAEA18BF3C241C41410402C1E51C6B8F44B56C`）、封存的 cache/P0 sidecar（SHA-256 `9B646AC8A7EBB4EB4CEABB2F7E081945B9C840F48A49A81315823DB9929BC2DC`）与 Init v4（SHA-256 `9EC4241EDC902E9C51482DA3B2EF508CE232007D392D84470D6C920476B5CFEA`）。draft-v1 至 draft-v9 的失败/重试产物全部保留；正式包由 create-new builder-v10 生成，未覆盖任何旧版本。
