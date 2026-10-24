> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 native join Init 静态证据 v4

状态：`BLOCKED_BY_EVIDENCE`。本轮扩展同步范围并提交全量根函数库存及有限局部证明，**没有闭合整个 Init/P0 同步证明**；没有运行游戏、读取进程、修改 IDB、生产代码或二进制。本文件不替代独立验收。

JSON：`cs-t0/coop-native-join-init-closure-v4.json`，SHA-256 `9EC4241EDC902E9C51482DA3B2EF508CE232007D392D84470D6C920476B5CFEA`。

## 边界与覆盖

从已封印 Init v3、plan v10 / progress v4 接续。8个根函数是 `5BAC40, 5B9CD0, 353DA0, 355470, 3BBBD0, 3A6450, 5A8620, 3BC740`；核心提交还含37个实际追到的支持函数，总45函数、14666条指令、1452个调用、6224个显式/隐式写记录和116个出口。cache/P0补充证据单向绑定，独立列出自己的完整库存；151条reconcile只用于交叉核对，不代表全部副作用入口。

普通确认的顺序是 manager `5BB415`创建 → constructor → virtual Init → `3577D5`后处理 → `5BB424`assignment → `5BB4FD`HUD → `5BB504`tail/cache → `5BB545`出生Effect → B字段写入和selector退休 →剩余循环及 `5BB599`secondary updater →完整manager返回。先前“到cache或tail返回”不足以覆盖这个范围。JSON中的有序段按真实嵌套调用记录，不把RVA排序当执行顺序。

首次publication之前到确认入口的自然更新保持、本轮之后的完整pass与停止/drain/恢复/terminal窗口另属window。本轮入口假设未被升级为已在publication前建立的事实。

## 新增的有限事实

- base constructor/Init都写parent `Player+3BC=0`。Init的 `+171` dword写 `0x101` 得出byte `+172=1,+173=0`，不是整块全零。assignment还读 `+1D98/+1E68/+1E6C/+3BC`，可能扫描主玩家与旧controller关联的Room NPC；HUD同样访问parent/twin和资源。空secondary vector不能代替全拓扑证明。
- native Weapon_Tears链解析到factory `5F9260`、base ctor `5F3180`、null旧引用分支 `5F3320`、init `5F34D0` 与reset `60D850`。在精确vtable和独立成功分配前提下，reset仅写Weapon `+44/+48/+4C`，init保存Player指针本身而不解引用。这解决了一个有限调用摘要；全局factory身份保持、分配失败和异常返回仍不具备完整证明。
- birth Effect确为普通确认后必须覆盖的spawn。`2FE410`先调用callback24，再选池slot，再虚调用Effect Init。因此先验无familiar不能排除此路径。callback24读registry→bitset第0个u32的 `0x01000000`；Effect共同尾部callback54读第1个u32的 `0x00400000`。无mods的口头声明或其他回调位不足以证明这两个位清零及owner保持。
- 固定Type1000/variant15/subtype0的具体Init路径已解析：subtype0跳过替代动画，`Effect+354=8000`，后续大switch因variant减21的无符号范围检查被跳过。但此前extra-data/引用控制块释放、旧vector销毁、base Init和资源预加载，以及之后共同尾部仍然执行。
- `GameRoot+26614`原生比较是**有符号i32**：`JL`与`SETNL`。signed<2可排除两处同步分派；若契约采用u32<2，只能称作更强的所选场景条件，不能冒充原生无符号比较。

## 必须保留的明确未决路径

1. v3的8项事实仍只在各自程序点成立。从这些写入到实际getter的每个callee、别名和bulk写尚未完成有序保持证明；P0初始状态必须独立建立，不能借用B归零。
2. base appearance lookup `2A98B0→294FB0→29BA90`可能lazy preload；需实际record身份和loaded标记，或有限无环linked-resource证明。该资源记录与普通Isaac起始道具config不是同一个断言。
3. effect池实际owner来自GameRoot `+26580`，1024槽、stride `4C0`，index在owner `+130000`。有1024槽不推出有空槽；free flag0也不推出旧refcontrol/vector为空。满池复用会调用旧slot remove `1D2420`/reset `20ABB0`；旧variant179/193的分支不能用“即将写入variant15”排除。
4. Effect Init一开始的 `20C800`可能释放旧extra-data控制块并触发release hook；`E47C0`释放旧vector。只有direct xref找不到全局writer，不等于hook指针为空或生命周期固定。Room登记、后置资源callee及剩余manager更新仍需结合真实owner写集合。
5. cache/P0补充材料盘点所有实际desired、独立Spawn与已有玩家/实体移除。Room Query `01AF60`即使零匹配仍重置/写scratch结果与计数，并累加profiling全局；这不是整条纯读，必须证明这些owner与persistent/save/request不别名。每个源仍须在消费时成立，尤其P0 conversion的signed健康条件、父关系、accumulator及递归twin。新增局部输入切片不关闭旧11项aggregate UNKNOWN reader。
6. 原始库存保留INT3可能恢复、导入noreturn调用可能返回、SEH `mov continuation; ret`以及tail中不属于IDA block的一条可解码指令。它们逐项列在edgeDispositions/unclassifiedInstructionRvas中；没有伪造block、删字节或把IDA边当独立语义证明。

JSON中的每个callee都有owner/reads/writes/aliases/returnRelations，全部call/exit逐条disposition；CFG强连通循环登记UNKNOWN不变量。未解析的精确callee调用仍在库存中，并具有所需源/别名关系，不以“已抽取足够多函数”终止为成功。

## 反例与诚实性

未发现已经满足全部候选条件、可证明到达的真实污染反例。保留的反例是“较弱前提不能推出结论”：callback24未包含在旧mask、free slot仍有旧引用、未来variant不能约束旧slot、secondary空但parent图非空，以及有符号比较误读。它们不被描述为实机观测或完整候选可达反例。

原始 `root-inventory-raw-v1`因把108条SSE movss误列为string隐式写而失败，连同纠正记录保留；正式引用修正后的v2。堆栈/SEH/allocator计数写没有被直接标记为游戏持久写。readonly保护证据继续复用，但不推断任意直接写均受保护。

## 状态

四个同步subproof均为UNKNOWN，`synchronousPropagationComplete=false`，整体仍`BLOCKED_BY_EVIDENCE`。独立验收必须区分原始指令集合一致与语义未闭合；本产物自己不计PASS。完整自然窗口、生产reader/generation/ABA、55槽runtime、IPC/arm/claim均未实现；matrix11仍NOT_STARTED，active/live、05-05、T0-05整体和T0-06继续关闭。
