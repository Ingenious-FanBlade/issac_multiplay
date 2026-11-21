# tx-04-08 ST6：创建、Morph、Restore/Replace 与 Grid owner

日期：2026-10-26。范围：J273 目标二进制的静态逆向、官方 REPENTOGON SDK/Hook 当前源码、现有夹具动作盘点。没有运行游戏、测试、构建、生成器、语法/格式检查；没有修改 IDB、存档或游戏状态，也没有校验哈希。

**本报告只给静态事实与夹具可达性，不授权任何实机动作。** 本次授权止于逆向和本研究文档；将来计划中的 fixture 调用不能据此直接执行。需要实机时，仍按每次启动前说明具体动作、副作用和用户配合要求办理。

标签含义：**STATIC_CONFIRMED** = 从 J273 IDB 直接读取的静态控制流/指令；**CURRENT_SOURCE** = REPENTOGON SDK 或工程当前源码；**DESIGN** = tx-04-08 目标/未来 fixture 约束；**UNKNOWN** = 没有足够静态证据，不补猜。

## Spawn、派生 Init、Publish 与完成边界

**STATIC_CONFIRMED**：Game::Spawn 0x6FA210 的顺序为预处理/callback helper 0x84AE90 → EntityFactory::Create 0x6EC540 → 对非空对象写 spawner/位置/速度等 → 调对象 vtable +4 的 Init（native 参数 type/variant/subtype/seed）→ 写 Room Entity index → EntityList::Publish 0x413D00 → 继续原生/Repentogon 后处理 → 返回对象指针。Factory 返回 null 时 Spawn 也返回 null。Factory 可从池中给出对象；“新地址/新物理分配”不是 Birth 的必要条件。

Publish 会按 Entity 标志和类别插入相应 EntityList 视图、设置发布状态，并可登记额外视图；其返回值是按 type/variant/subtype 路径产生的值，不是 Entity 指针。Publish 后 Spawn 仍有后处理，所以 Init 返回、进入 Publish、Publish 返回都不是这个 Spawn owner 的完成。对出生/ready 的设计证据边界应为：Factory 非空、派生 Init 已经发生、Publish 与 Spawn 后处理均完成、最外层 Spawn 正常返回。异常、Hook 短路和分配失败覆盖未由这里证明；采集的逻辑状态与调用身份/side effect 记录还应分别记录。

**CURRENT_SOURCE + STATIC_CONFIRMED**：目标 Lua 注册与 wrapper 支持 Game():Spawn(type, variant, position, velocity, spawner, subtype, seed)，wrapper 给 native Spawn 的隐藏第八参数 unk=0；这是优先的有显式 seed fixture 入口。Isaac.Spawn(type, variant, subtype, position, velocity, spawner) 使用 seed helper 0x6EB4B0 后再调用 Game Spawn。主审补充的 main-spawn-seed-helper.json 静态导出显示：通常路径读写全局 dword_BE8720 状态数组并递增 n625 索引，且会分配/维护调试追溯列表并调用 CaptureStackBackTrace；byte_BE72FF 非零的另一路径调用 rand。故通常 Isaac.Spawn 路径有全局 RNG 与分配/调试追溯副作用，但不将此推广到另一分支的状态变化，也不声称已覆盖 Spawn 内部全部 RNG。两者都不在当前测试 Lua mod 的动作表里。

**UNKNOWN**：能够安全创建哪种七类对象的确切 type/variant/subtype、可稳定使用的位置/速度、spawner 是否允许 nil 的全部 Lua 转换细节、native callback/helper 的实机覆盖和 Spawn 其他分支的 Hook 影响。不能以伪参数、随机选一格或任意 Entity 指针补上这些空缺。

## NPC / Pickup Morph

**STATIC_CONFIRMED**：NPC Morph 0x6BD0C0 是同一对象上的原生状态/类别变化，不创建新 Entity，不经过 EntityFactory/Publish。若当前 type/variant/subtype/champion 已匹配可早退；正常变化路径会写 type/variant/subtype 与可选 Champion，清理/重置 NPC 状态，通过虚函数更新尺寸/生命缩放，调整引用/字段，再正常返回。Morph 的自然分类是 revision/状态变化；不能记为新出生。

**STATIC_CONFIRMED**：Pickup Morph 0x6DFF30 也是同对象改变。native 参数为 (type, variant, subtype, KeepPrice, KeepSeed, IgnoreModifiers)。实现会按 flag 保留价格、seed 或 modifiers，更新/保存较多 pickup 字段并可能调用 Init/重接引用；特别是 KeepSeed=false 会读取并推进/改变对象本地 RNG 状态。它不能当作无副作用的类型字段写入。

**CURRENT_SOURCE**：REPENTOGON CustomCallbacks.cpp:1192-1241 对 NPC Morph 调用 pre callback 1212；Lua Boolean false 会直接从 Hook 返回而不进 native original，也不发 post callback。返回 3/4 项 table 会改 type/variant/subtype（4 项另改 champion），再调用 original 一次；native 后调用 PostNPCMorph。Pickup 1266-1318 对应 pre callback 1213，false 同样短路；3/6 项 table 可改 type/variant/subtype（6 项另覆盖三个 flag），再调用 original，随后 PostPickupMorph。两个 post callback 传的是调用前缓存的旧 type/variant/subtype，因此 Post 回调出现仅能证明该 Hook 路径调用过 original 并返回到 post，不应把参数误读为新字段。Lua pre callback 自身也可能改变对象；false 只证明该 wrapper 没有调用 native original，不代表整个 Hook 链零变更。仍须记录实际可观察的分类、字段、生命周期变化及污染。

**STATIC_CONFIRMED**：目标二进制直接注册了两个 Morph Lua 方法。NPC 注册上下文 0x85180E 推入 native 0x6BD0C0，0x851813 配名 Morph，经 adapter 0x86D190 安装 closure 0x897480；closure 从 L2/L3/L4/L5 读取整数并调 native 四参，返回 0 个 Lua 值。因此 NPC 直接调用形状为 npc:Morph(type, variant, subtype, championId)，没有 Lua 返回值。Pickup 注册上下文 0x850FB3/0x850FB8 配对 0x6DFF30 与 Morph，经 adapter 0x86B040 安装 closure 0x896C30；closure 从 L2/L3/L4 读取整数、L5/L6/L7 读取 boolean 并调 native 六参，同样返回 0 个 Lua 值。该证据是目标二进制注册与 wrapper 静态事实，不是运行时调用观察。

## Player 创建、Restore 与 Replace

**STATIC_CONFIRMED**：PlayerManager::SpawnCoPlayer2 0x990250 的普通有效 playerType 分支分配/构造 Player，追加 manager roster，按当前 Manager RNG 计算并推进 seed，再调 Player vtable Init、写 Room index、Publish，返回 Player 指针。特殊 playerType == -1 走另一分支：Init 参数也不同，且本次静态读取未见其走正常 Publish；其语义/用途 UNKNOWN。只看 ctor、Init 或 roster append 均不能将 SpawnCoPlayer2 标为已完成；普通分支以该 owner 正常返回为边界。

**CURRENT_SOURCE**：REPENTOGON Lua PlayerManager:SpawnCoPlayer2(playerType) 是公开 wrapper：参数为一个整数，调用 manager 方法并推 Player 或 nil。**DESIGN**：不要用该 Lua API 手动制造 C/S 的 B；它会新增 roster Player、推进 Manager RNG 与运行态，并绕开 Host/Join 绑定与 exact pair 生命周期。tx-04-08 S1 的 fixture 路线是 A 已存在后，经 C/S Host/远端握手让原生 Join 创建 B。当前实现 mod/src/game/join.cpp:60-112,114-182,197-245 只在 Host、有效 Warmup、唯一 A、正常 Manager/Join owner、单人 roster 等边界内介入真实 Join；它改当前自有 Join row 的 role 参数一次，返回后验证确为 2 人、B 唯一/alive、role 与 key 匹配，再 bind ordinal 1。默认 expected_b_player_type 为 Isaac type 0（fixture README.md:17），实际角色值来自 scenario/config。Join 如何在所有目标分支内部到达 SpawnCoPlayer2 的精确调用映射 UNKNOWN，不能把 C/S wrapper 内部“预计调用”伪装成直接函数调用证据。

**STATIC_CONFIRMED**：PlayerManager::RestoreGameState 0x993A90 是聚合 owner：遍历 GameState 中保存的 Player 条目，对每个相关项调用 Player::RestoreGameState 0x7A5660；另外处理 backup/保留关系，并对部分已保存关系条件分配与初始化额外 Player（含 Esau Jr. 四槽路线）。Manager Restore 没有直接 EntityList::Publish；不能一概声称 Restore 新建的所有 Player 都已 publish。PlayerRestore 子函数返回、某次新 Player Init 返回，均不代表整个 Restore 完成。

**STATIC_CONFIRMED**：Game::RestoreState 0x6F43A0 调 Room restore、PlayerManager restore、其他状态恢复与 PlayerManager RestoreGameState_PostLevelInit（SDK 声明对应 0x993FA0），然后才返回；Rerun/重开 owner 0x6F3650 也有 Manager restore 与 post-level-init。故完整恢复应按实际 native Continue/恢复外层 transaction 完成，而不是在 PlayerRestore/ManagerRestore 入口或中途确认。实际各 Continue 状态分支、异常/Hooks 介入、具体哪条菜单动作必经哪个 owner仍 UNKNOWN。

**DESIGN**：可靠的 Restore fixture 动作为真实菜单 Continue / native Game restore 路径，并观察 Game/Room/Manager 多层 owner 完成、存档窗口和 Player roster/字段恢复。它会读取并恢复持久化 run/player/room 状态，可能改变当前 run 与存档；本次未运行也未授权。不可向 Lua/native Restore 传任意伪造 GameState*。

**STATIC_CONFIRMED**：PlayerManager::ReplacePlayer 0x995E10 返回 bool。不要为 replacement+0x170 推断“登记/未登记”语义。原始分支是：0x995E57 比较 byte [replacement+0x170] 与 0，0x995E64 在相等时跳到 0x995F37。非零 fallthrough 路径交换 Manager roster 的两个索引及两个 Player 的 index，并调用 swap/Manager helpers、重置两者若干字段；零分支从 0x995F37 开始查找 old 于 Manager vector，要求找到后以 replacement 替换旧 vector 项、重接引用、Publish replacement、搬移多项旧 Player 字段/关系，可能调用旧对象 Remove/helper，并在 flag 条件下调用 PlayerRestore。失败条件与副作用按原始条件记录，不给 +0x170 贴未证实的高层语义。Replace 正常返回不证明旧对象已析构/释放，也不等于普通 Spawn。

**CURRENT_SOURCE / UNKNOWN**：在当前 REPENTOGON LuaInterface source 中未找到 PlayerManager::ReplacePlayer 的 Lua wrapper；“没有找到”不是 runtime 不可达证明。二进制存在两个直接 native caller：0x5B4A60 和 0x7A3A40；后者的上下文由实际游戏逻辑选出另一 Player 并补齐状态后再调用 Replace。哪个可控真实游戏动作能经过它、两分支各自安全的完整对象前置条件都 UNKNOWN。目前禁止 fixture 直接调用 ReplacePlayer(old*, replacement*) 或传任意对象指针。若将来在已验证自然动作中不可达，则保留 NOT_RUN/UNKNOWN，不临时制造替换指针。

## Grid、Door 替换与退休

**STATIC_CONFIRMED**：Room::SpawnGridEntity 0x7E21D0 限 index 0..0x1BF（0..447，448 槽）。检查 type/index 对应的格子容量/位置规则，GridFactory 0x706100 返回 null 时本次 false、不替换。Factory 成功后，对原槽非空 Grid 调 virtual slot 0、参数 1；然后写新 Grid 槽、variant +8、VarData +0x18，调 Grid Init 0x705070(seed)，最后才返回 true。它不走 EntityList Publish；新 Grid 完成边界是 SpawnGridEntity owner 在 Init 返回后返回 true。返回 false 不应记新 Grid。

**CURRENT_SOURCE**：Room:SpawnGridEntity(GridIndex, Type, Variant=0, Seed=GridIndex+Game.frameCount+1, VarData=0) 是 Lua 可达整数 overload，wrapper 检查 0..447 并返回 bool（LuaRoom.cpp:33-53）。候选 fixture 应显式传 seed，避免默认值随 frame 变化；但不会因此获得任意 type/格子可用性。确切 Type/Variant/VarData 与一个既安全又符合原生格子规则的空/替换格 UNKNOWN，必须先由具体场景核准。

**STATIC_CONFIRMED + CURRENT_SOURCE**：Room::RemoveGridEntityImmediate 0x813BD0 / Lua wrapper 参数是 (GridIndex, PathTrail, KeepDecoration)，无返回值。目标检查 index 范围；在实际移除分支会写 Room 上与路径相关的值、清 Grid 槽，并调 Grid virtual slot 0 参数 1；但当 Grid type 为 1 且 KeepDecoration 为 true 时保留对象。Remove 调用、槽退出和对象物理释放不同。PathTrail 在本目标中的高层语义及不同 type 下的完整副作用 UNKNOWN。当前无此 fixture action；未校准值之前不建议提交 remove 动作，更不把 Remove 请求等同于物理死亡。

**STATIC_CONFIRMED**：Room::MakeDoor 0x7E4DE0 接 door slot。若该 door slot 已有非空 Door 指针，函数立即早退，不刷新或重新初始化它。只有 door slot 为空时才从 door-slot→grid-index 表找 Grid 槽；若该 Grid 槽已经是 Door，则复用这一个 Door 并刷新字段，再写入 door 槽；若原 Grid 不是 Door，则分配/构造 Door、释放旧 Grid 并替换 Grid 槽，然后写 Door 字段。正常路线同时写 Grid 与 door 两个 Room 槽、执行虚函数/帧更新。Door 是 Grid domain 对象在 Grid 与 door 两来源的 alias，不可双重出生。**CURRENT_SOURCE**：SDK 有 native Room::MakeDoor(int slot) 声明，但 LuaRoom 注册表没有 MakeDoor wrapper。门 owner/真实生成时机和安全可控触发动作 UNKNOWN；fixture 应优先读取实际生成的门与映射，不直调此方法。

**DESIGN**：S4/S5 允许计划观察 Grid/door aliases、真实 Grid create/replace/remove；只在确切公开 Lua wrapper、有效实际 index、能说明替换目标对象副作用并有实机授权时执行。当前夹具没有 Spawn/Morph/Grid/Replace action，因此这些不是“已可在现有夹具重放”的动作。

## Fixture 动作与副作用表

“可达”指源码/API 静态可触发，绝不等同于操作授权；CURRENT 表示当前仓库现有夹具，DESIGN/CANDIDATE 表示后续实现/授权后可考虑，UNKNOWN 表示保留待核。

| 操作 | 状态与精确动作/参数 | 成功边界或证据 | 已知副作用与限制 |
| --- | --- | --- | --- |
| A→B Join | DESIGN + CURRENT_SOURCE。A 已存在；启动 Host 与远端握手，等原生 Join 自身触发；Host Join owner 只改自有 row 的 role 参数，默认角色期望为 PlayerType.PLAYER_ISAAC=0，实际按 scenario。不是 Lua SpawnCoPlayer2。 | native Join 正常完成后 roster 计数为 2；唯一 B alive、实际 role/key 匹配；owner 才绑定 ordinal 1。 | 创建并登记真实 B，更新 Manager roster/RNG、Room/EntityList 及后续绑定状态。绕过 Join 的 direct create 不合规。现有菜单 menu_host 可自动 Host 一次，但 S1 仍是设计中的新 E2E 行程。 |
| 普通 Entity Spawn | CURRENT_SOURCE + STATIC_CONFIRMED API 可达，现有夹具未实现。Lua 形状：Game():Spawn(type, variant, position, velocity, spawner, subtype, seed)，七项参数；native 隐藏 unk=0。实际 ID/Vector/seed 值 UNKNOWN。 | 外层 Game::Spawn 正常返回非空对象，且 Factory/派生 Init/Publish/后处理都走完；应记录返回指针/实体类别，不把 Publish 单独算成功。 | Pool/factory acquire、可能复用、Init、Room index、列表发布、native 后处理。值和对象类别未确认前不应执行。Isaac.Spawn 另有自动 seed 路径，seed/RNG 副作用 UNKNOWN。 |
| NPC Morph | STATIC_CONFIRMED 目标 Lua wrapper 可达，当前夹具无 action。精确签名 npc:Morph(type, variant, subtype, championId)，四个方法参数分别从 Lua L2..L5 读 integer；Lua 返回值 0 个。安全对象与具体 type/variant/subtype/championId 由场景决定，目前 UNKNOWN。 | 目标 native Morph body 正常返回并且外层实际 owner 正常完成；若 pre callback false/跳过 original，不记作 native Morph 完成或据请求确认 revision；仍须记录 callback/Hook 已实际改变的分类、字段、生命周期及污染。 | 同对象状态/类别/引用/HP 等变更，无 Entity 新出生；REPENTOGON Hook 可短路或改参。API 可达不代表对象/参数安全或获得实机授权。 |
| Pickup Morph | STATIC_CONFIRMED 目标 Lua wrapper 可达，当前夹具无 action。精确签名 pickup:Morph(type, variant, subtype, keepPrice, keepSeed, ignoreModifiers)，L2..L4 读 integer、L5..L7 读 boolean；Lua 返回值 0 个。具体值/安全对象 UNKNOWN。 | native body reached + 返回 + 外层 owner 完成；Post 传旧字段。 | 同对象重新初始化与更多字段/引用变化；keepSeed=false 会推进/改变对象 RNG。不能用 wrapper 正常返回代替 original 到达；API 可达不代表获实机授权。 |
| Grid create/replace | CURRENT_SOURCE Lua wrapper 可达，当前夹具无动作。room:SpawnGridEntity(index, type, variant, seed, varData)；显式 index=0..447 与 seed；type/variant/varData 和 cell 待场景确定。 | 返回 true 代表 SpawnGridEntity owner 已走到 GridInit 后正常返回；false 不登记新 Grid。需同时记录旧/新 Grid 槽。 | 可能释放旧 Grid 并换槽；型别/格子限制，Grid init 带 seed，Room 世界改变。API 可达仍未授权。 |
| Grid remove | CURRENT_SOURCE wrapper 可达，当前无动作。room:RemoveGridEntityImmediate(index, pathTrail, false) 可设 KeepDecoration=false 消除保留 decoration 分支；index 必须为已验证普通 Grid 槽 0..447；pathTrail 的选择值 UNKNOWN。 | void wrapper，不提供成功返回；应在已知槽/对象上观察 Room 槽变化及析构/释放各自证据。 | 清槽并可能 free/remove Grid；不能对 Door alias 猜测；pathTrail 语义/影响未完整确认。 |
| MakeDoor | CURRENT_SOURCE native 签名可见，无已确认 Lua wrapper；没有 fixture action。Native 入参为一个有效 door slot，具体 slot 和 map 需来自当前 Room 关系，不能手写猜测。 | 调用/owner正常返回并读两个 alias 槽核对。 | 可能替换并释放该 index 原 Grid、重写 Grid+door 两个槽；已有 Door 则复用。当前没有安全直达动作。 |
| Player Restore/Continue | DESIGN。通过新局/退菜单保存后真实菜单 Continue，不造 state 参数；具体控制入口按 E2E 启动说明。 | Game outer Restore 完成，包括 Room/PlayerManager restore、PostLevelInit 与外层返回；并完成相应 Continue owner。 | 读取并恢复存档/run/player/room，能改变持久数据和当前 run；本次无实机授权。 |
| Player Replace | UNKNOWN。没有可核准 Lua API/自然触发参数；不允许 direct ReplacePlayer(old*,replacement*)。 | 仅当未来确认自然 native owner 与两对象连续性/roster 前置后跟踪替换 owner 正常返回，否则标 NOT_RUN。 | roster 交换或替换、字段/引用搬移、Publish、Remove/Restore 辅助路径；旧对象释放状态未必完成。触发 action 未知。 |
| 现有 skip | CURRENT_SOURCE。配置 actions 内 "skip"；夹具在 MC_PRE_UPDATE 连续返回 true 30 次。 | action phase=arrived, skipped=30。 | 跳过 simulation update 窗口，不直接调用 World create/morph。 |
| 现有 room | CURRENT_SOURCE。按 slot 0..7 找已开启、未锁、普通相邻门；调用 Game:StartRoomTransition(target, direction, 0, nil, -1)。 | 房间身份改变后 arrived；Lua return 不等于切换成功。 | Room change/loading、room epoch/成员改变；不直接写门或 Player 状态。 |
| 现有 kill_b | CURRENT_SOURCE。仅 player_count==2、A 健康、B 是唯一完整普通 B 时调一次 B:Kill()，无参数。 | 观察 native dead/ghost，不补造未见的 dead；记录调用返回与后续状态。 | 改变 B life/ghost/Player 生命周期，可能进入死亡路线；当前静态授权没有允许实机执行此动作。 |
| 现有 revive_b | CURRENT_SOURCE。只对已确认 B ghost 调一次 B:ReviveCoopGhost()，无参数。 | 记录 return、健康状态、Appear 动画各自证据；true 不等同稳定 DLL rebind。 | 改变 B ghost/alive 状态并开始 revive/Appear 路径；当前静态授权没有允许实机执行此动作。 |
| 现有 stage | CURRENT_SOURCE。仅未暂停时调 Game():StartStageTransition(false, 0, nil)。 | 记录 call_return 后，观察 stage/room 身份、new_level/new_room callbacks。 | 换层/加载 Room，会改变 run/room 状态；调用返回不表示已完成。当前静态授权没有允许实机执行此动作。 |

说明：上表是本次静态列出的可达/候选操作与已有动作，不建议把所有操作无差别堆入单次 E2E。计划里 tx-04-08 S4/S5 的具体 scenario 仍需依据可用自然 owner、参数和副作用核定；只要 owner、API 或类型/索引不明，就将该动作保留 UNKNOWN/NOT_RUN。

## 收束与待核

- Spawn 的 outer owner 仍是核心 birth 完成边界；Publish 是 membership 变化，不是完整生成完成。
- NPC/Pickup Morph 是同对象变化，目标二进制 Lua 注册、精确 wrapper closure 与参数槽已 STATIC_CONFIRMED；自然触发场景、对象选择和安全参数仍 UNKNOWN。Hook false/table 处理已由 SDK 当前源码明确。
- B 必须沿已经存在的 C/S Host/Join 生命周期创建；不能通过公开但改变 roster/RNG 的 PlayerManager Lua 方法伪造 B。
- Restore 只能从实际 Continue/Game restore 外层跟踪；Replace 的自然触发源 UNKNOWN，不安全直调任意指针。
- Grid spawn/remove 的公开 wrapper 可达且参数签名准确，但合法的具体 Type/Variant/VarData/PathTrail/cell 及其副作用场景仍要定；MakeDoor 不是已确认 Lua fixture API。Grid/door aliases 必须按同一个物理 Grid 对象去重。
- Hook 运行时注册/优先级、native/Lua callbacks 实际命中、所有 Spawn/Morph/Restore/Replace 分支的线程/异常覆盖、持续性字段准确读数均 UNKNOWN，需由以后授权范围内的整体 E2E 或进一步静态研究补齐，不从本报告的静态 reachability 推出。
- 现有 E2E Lua mod 只包含 skip, room, kill_b, revive_b, stage, room；没有实现 ST6 的 Spawn/Morph/Grid/Player Replace/Restore 动作。

可复查材料：同目录 sdk-source-evidence.md；IDA 原始导出 ida-functions.json、ida-registration-contexts.json、ida-lua-spawn-wrappers.json、ida-restore-replace-callers.json。没有修改 public design、implementation、fixture 或公共状态文档。
