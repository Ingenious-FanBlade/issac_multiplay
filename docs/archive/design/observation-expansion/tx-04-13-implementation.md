# tx-04-13 探索、离房保存与玩家跨房观测

日期：2026-10-31，Asia/Singapore。状态：完整公开观测目标已实现，统一构建通过；同一tx连续修复、复用不受影响的已完成项目后，统一E2E **5/5 PASS，31/31场景**。不保留tx artifact。

目标沿用[任务书](exploration-player-observation.md)，范围是exploration、room_save、player三域；仍不实施状态同步或世界写回。[逐成员目录](../../../../mod/binding/catalog/continuity-catalog.md)及JSON来自两套本地Wiki，列出动作、参数查询、共享规范来源与新增采集范围。目录不是实际运行覆盖。

正式Lua模块位于 `mod/observation/isaac-cs-observation/domains/` 对应三域。三个普通scope与Room/Level/Entity/Grid在同一native复制窗口内完成，先Room建立分层位置，再Entity，再新三域；最终来源/root复核和Authority提交统一决定发布或取消。三个新域在原有160MiB扩展预留内分别使用1/8/1MiB payload预算；超限明确Partial，不等待磁盘，不影响Authority。新增pending槽为九个普通投影，独立事件余量十二槽。历史只保留有界纯值；保存集合比较保留编码字符串和自有ID，不持有userdata，不计算摘要。首次观察、历史空档、完整空集合和未访问明确区分。

exploration读取实际GetRooms和19个适用离图槽，引用同一Floor/Dimension/Descriptor及construction_revision；复制DisplayFlags、访问/清理次数、Flags、类型、当前地图/蓝图/指南针/全图效果与诅咒。逐描述变化连接前一个已提交scope，成员消失单列。Minimap状态、展开大小、hold/shake及图标Sprite资源是单列本地显示投影，不作为世界访问事实；后续presentation域须复用该来源，不另造探索真值。轮询不调用Refresh/Set等动作。

room_save完整遍历每个已初始化描述的Entity/Grid/装饰保存容器，以及Myosotis和每个合格Player的Moving Box。保存项逐字段按公开API复制；I/F/B/U字段保持原名/原类型语义，不推断私有用途。当前活动容器明确可能滞后；GeneratedUnvisited、InactivePublicSave、空集合、读取失败/截断分开。保存项使用space=8的独立身份；集合内容或构建变化产生新修订和新项身份，相同修订引用不可变副本。Grid使用`grid_columns_v2`：集合声明八个字段名/类型、namespace和共同未解析关系，每个槽保留SaveEntry local_id、ordinal及八列原值；整数用十进制字符串，空槽和未访问房间的实际存储不省略。`objects`统计容器，行值由262144 nodes/queries和8MiB独立约束；旧具名字段记录继续读取，不能补成新编码。公开容器只给值和ordinal，不能证明逐对象SavedFrom/RestoredFrom；因此保留Unresolved及原因，只建立实际观察scope的时序关系，不凭seed/索引强认同一实体。

真实 `MC_POST_ROOM_SAVE_STATE` / `MC_PRE_ROOM_RESTORE_STATE` 另复制当时容器，记录真正保存/恢复前阶段，不借旧Authority tick。Ascent保存/恢复公开回调保留stage/key和描述，Glowing Hourglass保存/加载回调保留slot；未公开的Hourglass内部备份容器标Unavailable，不假装读取完整引擎存档。

player身体身份复用Entity subject/use；心量、资源、库存、主动/卡药/饰品槽、TemporaryEffects、变身/服装等本体唯一来源为同窗口 `isaac.cs.entity`，新域只保留具体scope引用和字段覆盖语义，绝不重新调用Entity采集器。新鲜History按公开History/HistoryItem展开，与持有和临时效果分开；四个Esau Jr备份槽独立引用，不能按ordinal猜槽。`control_group_membership`复用tx-04-09已复核缓存拓扑及703关联元数据，给出非活动备份的实际participant/control group、role及active/can_consume；`control_relation`仍只表示实际输入锚点。实际body_slot随当前锚点输出；独立事件未取得拓扑角色时不补默认角色值。当前合格原生来源、公开roster、twin/main/sub、parent/child、角色和死亡状态组成关系图。关系变化和来源缺席记录前后scope；缺席/绑定失效不推断销毁。普通切房不批量退休Player，run/epoch变化撤销旧关系。

participant/control group沿用原有exact-pair锚点。在实际锚点刷新/撤销处发布 `isaac.cs.player.control`，记录绑定代次、主体和失效，补足30tick采样之间的关系变化。device/key只在进程内比较，规范值用control session和binding revision限定，不输出原生指针/handle。Player Init、真实死亡/复活、new-room临时效果、新楼层、物品/卡药等公开回调另记独立事件。PlayerManager非随机参数查询按显式请求复制，随机选择不进入白名单。

公共服务、最新值消费、manifest、reader、Lua资源部署和正式发布同接七域；旧v1仍是兼容投影，`world_complete=false`，8—11域仍Unsupported。Entity.GetHistory/MovingBox和Level/Descriptor保存容器的旧占位改为owner限定的同窗域引用。正式采集与可选录制保持独立，生产默认不录制；不新增Hook owner、不触及原版Online或Isaac Sync。

## 统一验收安排

入口：`scripts/accept-tx-04-13.ps1`。全目标实现后才统一构建和运行E2E，不编写单测，不提前probe/试跑，不计算哈希。已通过的tx-04-09—12人数/输入、206个特殊房间和Greed清单不重跑；只使用四参与者/多身体作为新增Player关系观测的前置，并检查本次改变的跨域引用和调度。

单次启动顺序覆盖地图揭示/隐藏及诅咒、物品实际拿走、Grid破坏、机器变化后的离开/回访、Moving Box保存/携带/释放、库存/充能/临时效果/History、四组Jacob/Esau及普通往返、Forgotten/T.Forgotten/Lazarus+Birthright/Flip、Esau Jr交换与备份恢复、死亡复活、Downpour镜像进入/退出、Myosotis及换层、显式只读查询、远端退出、暂停、停录继续正式采集、正常退休/菜单退出与部署/配置/保存范围恢复。夹具只在F8动作中修改世界，不随正式观察模组发布。

每项先等夹具实际结果，再取新的三个正式普通scope和其Entity规范scope；离线逐场景对照地图原值、保存集合计数、Player身份和资源/槽值，检查同Authority窗口、保存独立身份、跨房连续记录、History和备份、真实控制失效、字节计账及reader交付。未触发或不完整不能以动作请求代替通过。正常测试临时材料消费后清理，失败与修复在下节如实续记。

## 实际验收及连续修复

验收前未运行试验、smoke或单测。首次受限MSBuild停在启动阶段且未产出目标，终止该构建后改用正常构建进程；DLL编译通过，harness首次因不存在的`Remote.disconnect()`编译失败，改用既有`bye()`后通过。未改变网络协议或正式输入策略。

1. 第一次实机完成前15项，特殊形态段等待普通scope超时，整体FAIL。追踪到旧v1 source 10对非活动Player的私有关系保持NOT_READY，被统一逻辑误判为来源发生变化，反复取消所有域。修复为区分私有字段读取资格与来源稳定性：旧v1仍保留原NOT_READY及字段不完整，正式Entity/Player由官方PlayerManager、main/twin/sub及四备份槽闭合公开来源，尾部重新复核roots/关系边，不补造Init/出生/原生更新依据。
2. 第二次复用15项，仅续测后段，完成特殊形态、Flip、往返及死亡等8项后，复活API返回false，整体FAIL。夹具仅等IsDead，早于原生合作幽灵就绪；改为等IsCoopGhost。Esau Jr两次API请求当时只保留实际结果，直到最终归约才确认未产生备份，不能将当时请求视作交换通过。
3. 第三次复用23项，实际复活完成；Downpour原生长加载超过控制口单次等待，`main_thread_timeout`导致夹具提前退出，整体FAIL。游戏随后正常完成加载和退出。新增有上限的只读状态重试，玩法动作不重发；本专项测试客户端/Host超时改为30秒，不修改生产默认策略。
4. 第四次仍在长加载期间触发Windows未响应窗口，状态轮询抢焦点失败，未增加通过项，整体FAIL。修复为加载期只读轮询不抢焦点，真正发送按键仍严格确认焦点。
5. 第五次加载完成，实际楼层保存Grid超过16384条，旧逐项冗长编码触发对象/查询/字节预算，明确Partial，整体FAIL。改为上述八列紧凑编码、容器/行预算分离，不跳过任何槽；在镜像离开/回访中补充实际八列Getter逐值对照。
6. 第六次完成剩余7项和完整正常收尾，31个动作阶段均有正式样本；最终统一归约发现Esau备份为0，整体仍FAIL。两次UseActiveItem请求确实未触发实际换身，不能据动作返回判通过。保存/部署/Helper/配置恢复及无新增崩溃检查通过；暂停、可选录制隔离结果在后续复用。
7. 第七次仅补Esau Jr交换/恢复两项：F8只准备真实物品槽和充能，远端经既有canonical提交held/edge=2，再等根身体实际改变、备份存在及恢复原身体。备份所属控制组复用已核定拓扑中的703关联，不按ControllerIndex或ordinal猜槽；非活动备份不获得输入消费资格。其余29项和第六次暂停/停录通过项不再实机执行。最终五份正式来源分别校验session/epoch/scope，以来源命名空间聚合，用新的两项scope替换未触发的旧请求样本，**5/5 PASS**。

七次实机各自正常退出/恢复成功，保存范围检查均通过，无新增崩溃文件。上述失败保留FAIL及实际原因；最终通过不改写它们。正常测试与连续修复所需临时录制、快照、截图和trace最终全部清理，未保留artifact。

## 最终实际覆盖及边界

| 检查组 | 结果与实际范围 |
| --- | --- |
| 探索知识与地图历史 | PASS：揭示/隐藏原值、地图效果和诅咒、Minimap状态、访问/清理、维度0/1及换层身份；逐场景实际值对照、前scope变化和过渡事件 |
| 离房保存及独立身份 | PASS：Entity/Grid/装饰三容器，物品实际取走、Grid破坏和机器变化后的离开/回访、Moving Box携带/释放、Myosotis与换层；19,264个实际保存Grid槽的完整编码为2,022,906 bytes、154,136 nodes、154,134 queries，未省略空槽；新编码八列按实际保存Getter对照 |
| 玩家本体引用及关系历史 | PASS：四参与者、最多8身体、Player/Familiar跨房连续性和实际所有者、资源/装备/主动及口袋槽、库存/TemporaryEffects/History实际值；Forgotten/T.Forgotten/Lazarus+Birthright/Flip、真实Esau Jr备份/恢复及非活动归属、死亡/幽灵/复活、真实控制失效 |
| Reader/交付/同窗上下文 | PASS：31/31场景，124个指定scope（93个新三域普通scope均Complete，31个Entity规范scope逐Player字段对照）；五份来源独立session/epoch校验，不跨进程拼接身份。原始新域记录9275条Complete、1条Partial；唯一Partial为第三次旧保存编码超预算的原记录，保留且未选作通过样本 |
| 暂停、可选录制和正常退休 | PASS：暂停不推进三个普通scope；停录后正式采集继续且epoch不变；全部输入/观测停止、退菜单/进程退出、部署/Helper/配置和保存范围恢复通过 |

成员目录为827条成员/签名记录、6份相关枚举、80条回调来源行；目录数量不代表827个运行场景。保存/恢复公开回调共63条、三个域各44份原始普通记录及各42条过渡、Player实际事件203条、显式查询1条、控制关系8751条均进入reader并保留原完整性。

公开SaveState未提供逐对象来源令牌，SavedFrom/RestoredFrom保持Unresolved，scope时序关系与实体身份分开。Hourglass私有备份容器无公开Getter，记录slot回调与Unavailable；未将所有引擎私有存档宣称为可读取/可继续模拟。Ascent/Hourglass回调已实现，专项未单独执行其玩法链；不同维度专项实际为0/1，维度2的既有房间/楼层验证未重复。未逐一执行所有原版变体/种子/备份私有分支、任意第三方Mod、极限负载；不由有限E2E外推这些场景。旧Entity/原生v1的Partial、NotReady和未知slot变体等原记录保持，`world_complete=false`；8—11域仍待tx-04-14，未实现Replica或世界状态同步。
