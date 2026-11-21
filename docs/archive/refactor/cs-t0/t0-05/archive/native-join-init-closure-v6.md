> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 Init v6：fresh B publication 身份到 Init dispatch

本证据新增独立子证明 `FRESH_B_PUBLICATION_IDENTITY_TO_INIT_DISPATCH`，边界从普通确认调用中的 fresh Player 分配返回开始，止于 `image+0x005B9F1F` 的间接 Init dispatch。它不进入 `0x00355470` Init body，也不处理 190 个消费者、17 类来源的到达定义、P0、自然更新窗口、reader/ABA 或 runtime/live。

正式证据是 `cs-t0/coop-native-join-init-closure-v6.json`，SHA-256 为 `281137C970DC39CE2DF325E817076E325FE37C02D98F1FC6B3A9EB0E57367847`。它以 Init v5 为累计基底，保留 v5 的目标身份、来源/消费者章节、全部 deny-only 权限和历史绑定，再合并 publication 身份章节及 68 个唯一函数记录。`publicationIdentityStatus=UNKNOWN`、`freshBStatus=UNKNOWN`、`synchronousPropagationComplete=false`。

## 独立磁盘依据

统一 schema 的原始证据 `out/cs-t0/native-join-publication-identity-raw-v2.json`，SHA-256 为 `194AEA35B5004BE0D75FAE0522E1F2293CFD1A5956644F3383FBB74DA3F30097`。修正后的章节草稿 `out/cs-t0/native-join-publication-identity-draft-v5.json`，SHA-256 为 `9FDDEAA6C720D5F3F01771D4DF7C9BD7D1514BFCC5D1FF727CA3C7BA2A3278CB`。

独立角色从目标 PE 重建 17/17 个 publication 函数，逐集合匹配 2,095 条指令、115 个调用、1,102 个写入和 23 个出口；36 个未知控制/副作用项原样保留。两处 `int3`（`0x005B9DB5`、`0x005B9EEA`）作为 trap 登记。malloc、free、invalid-parameter 与长度失败的 IAT 调用按间接调用处理，import directory 另行绑定；没有把 IAT slot 伪装成 direct callee。

独立复核还从盘确认：

- 普通调用点 `0x005BB415` 与另一调用点 `0x005BAFFA` 都真实调用 `0x005B9CD0`；后者能否由候选入口前提排除仍未证。
- 外层 `0x005B9E2E`、constructor `0x005B9E46`、两条 vector publication 和 `0x005B9F1F` dispatch 的固定 opcode 与调用目标一致。
- 非扩容提交点是 `0x005B9EAD`；扩容路径经 `0x0002C920` 与 `0x0001D150`，end 提交点是 `0x0001D193`。
- 两条 ordered identity chain 都包含 constructor return 事件 `0x0035493A`、vtable read `0x005B9EC2`、slot `+4` read `0x005B9ECA`、receiver transfer `0x005B9F0D` 和 dispatch `0x005B9F1F`。
- Entity_Player vtable 的 `+4` 槽静态指向 `0x00355470`。

这些磁盘事实证明了局部指令、CFG、调用、写入、出口、import 与 vtable 布局，不证明 allocator 的外部 ABI、对象区域不重叠、全局别名首曝光、异常不可恢复或跨 helper 的身份保持。

## 两条 publication 路径

非扩容路径在 `0x005B9EA6/0x005B9EA9` 判断 end 不等于 capacity，先由 `0x005B9EAB` 写入 B 指针，再由 `0x005B9EAD` 提交 end。扩容路径从 `0x005B9EBA` 进入 `0x0002C920`，在新缓冲区写入指针，经复制/释放与 `0x0001D150` 提交 begin、end、capacity。两个提交点都在 vtable read 与 Init dispatch 前。

章节分别登记 15 个 object definition、8 个 allocation region、2 条路径、2 个 publication point、12 个 identity transfer、10 个 early-escape site、7 个 failure disposition 和 14 个 callee identity summary。所有 `UNKNOWN` 记录都有非空的精确未决原因；五个 closure 标志全部为 false。

## 未闭合前沿

当前保留八个精确前沿：

1. `frontierManagerContext`：候选前提尚不能排除 `0x005BAFFA` possessor 调用上下文。
2. `frontierOuterAllocator`：imported allocator 的 non-null、fresh/disjoint、失败回调和地址复用语义未冻结。
3. `frontierConstructorNested`：constructor 的传递分配、异常、unwind 与指针逃逸图未闭合。
4. `frontierPrepublicationEscape`：constructor callee 在首次 vector publication 前可能保留 B 别名。
5. `frontierReallocBuffer`：动态长度、memmove、旧缓冲区 free 和跨函数 live range 未闭合。
6. `frontierFirstExposure`：inactive/new-buffer store 早于 header commit，缺少完整全局别名和线程可见性模型。
7. `frontierTrapFailure`：trap 恢复以及 imported no-return/throw continuation 未排除。
8. `frontierDispatchReceiver`：vtable slot 局部事实成立，但 allocation、constructor 与两条 publication 链仍是条件性的。

目前没有发现满足候选前提的已证污染反例；结论是证据不足，不能关闭 publication 身份子证明。`GLOBAL_ALIAS_EXPOSURE`、190 个消费者、17 类来源和总 `FRESH_B_SOURCE_PRESERVATION` 继续为 `UNKNOWN`。

## 失败历史与权限

raw-v1/draft-v1 至 draft-v3 保留了旧的非统一函数 schema；draft-v4 缺少 ordered path 中的 `0x0035493A` constructor-return 事件，也作为失败历史保留。producer harness 因冻结基线变化在正式比较前拒绝的记录同样保留；最终独立验收没有使用该失败运行制造 PASS。

本轮没有进程读取、实机操作、注入、生产 reader、IPC、arm/claim 或 active/live 授权。plan v10、join progress v4 和三个当前索引保持不变；矩阵第 11 项仍为 `NOT_STARTED`，05-05、T0-05 整体和 T0-06 继续关闭。
