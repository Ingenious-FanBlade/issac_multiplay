# 第7步 RoomSave、Exploration：目录与同步职责核对

2026-11-10；官方 REPENTOGON 1.1.3／Repentance+ v1.9.7.12.J273／Win32。完成本次要求的 **RoomSave 98行、Exploration 99行目录核对**，包括重载、原参数空白／null、旧归属、来源及逐行同步去向。机器表与可读表分别为 [RoomSave JSON](room-save-member-map.json)／[逐行表](room-save-member-map.md)、[Exploration JSON](exploration-member-map.json)／[逐行表](exploration-member-map.md)。这些是设计目录，不是生产 schema、Getter兼容实现或运行验收；布局版本仍56。本轮不构建、不运行游戏或测试。

## 1. 资料顺序与本次范围

先读本地原版 Wiki 的 `RoomDescriptor.md`、`GridEntityDesc.md`、`Level.md`、`EntityPlayer.md`，再读官方 REPENTOGON Wiki 的 `EntitiesSaveState.md`、`EntitiesSaveStateVector.md`、`GridEntitiesSaveStateVector.md`、`RoomDescriptor.md`、`Minimap.md`、`EntitySaveStateManager.md`、`enums/{RoomDisplayFlags,MinimapState,ModCallbacks}.md`。路径根分别为 `F:/projects_isaac/IsaacDocs/docs/` 和 `F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/`。

语义／副作用有具体疑问后读官方 `repentogon/LuaInterfaces/{LuaEntitySaveState.cpp,Room/LuaRoomDescriptor.cpp,HUD/LuaMinimap.cpp,CustomCallbacks.cpp}`、`SaveStateManagement/{EntitySaveStateManagement.cpp,EntitySaveStateManagement.h,EntitySaveStateEx.h}`、`Patches/GameStateManagement.cpp` 及 `libzhl/functions/{Room,RoomDescriptor,EntitySaveState,Minimap}.zhl`。最后在当前J273静态IDB核对 Room Save／Restore 和地图徽标消费，不附着游戏，不修改或保存IDB。本文地址均为VA；RVA=VA−400000h。

98行包含：EntitiesSaveState 39、EntitiesSaveStateVector 4、GridEntitiesSaveStateVector 3、GridEntityDesc 8、RoomDescriptor 44。99行包含 Level 87、Minimap 12。RoomSave 中的 RoomDescriptor 与 Exploration 中的 Level 是旧 continuity 目录的重复语义入口：分别复用[描述核对](room-descriptor-synchronization-review.md)和[Level核对](level-synchronization-review.md)的现行决定，**保留原始行，但不重复建线字段或读取owner**。它们不是新增98／99种状态。

本轮不要求解开所有保存payload的私有语义，也不把目录完成写作完整字节语义已知。已登记的 EntitySaveState 78h／通用UNKNOWN 74B、type5地图输入视图UNKNOWN 70B、GridEntityDesc 20h／UNKNOWN 3B、Minimap 628h及其嵌套未知维持原证据等级，见[Player保存体](layouts/05-player/EntitySaveState.md)、[描述字节账本](room-descriptor-byte-ledger.md)、[Grid描述](layouts/06-grid/GridEntityDesc.md)、[Presentation账本](presentation-byte-ledger.md)。公开I/F/B/U名字和SDK字段类型不能自动升级为已知业务字段。

## 2. 必须同步、由结果承接与本地保留

| 范围 | 必须保留的同步结果 | 原值／操作的归属 |
|---|---|---|
| 探索／房间成员 | 同world／floor／dimension的描述身份、成员和拓扑、双配置；L1完整DisplayFlags、visited非零、clear及已核五位显示投影；L0有效诅咒 | 不同步全部访问次数、清房累计、全Flags、生成／奖励seed；不能从可见推成已访问，也不能用本地揭图／进度重算Host知识 |
| 离房Entity、Grid | L2保存图标最终16类mask及必要动态最大帧，含type18楼梯和1000/161/980 Portal；当前活动世界仍由Entity／Grid域完整承接 | SavedEntities／SavedGridEntities整vector、计数、排列、保存I/F/B/U、位置／随机／递归保存关系均不作为独立下行状态；Client不负责回访时恢复它们 |
| 装饰与跨房携带 | 回访／Myosotis／Moving Box／Flip／矿车等在Host实际恢复／创建后的Entity、Grid、资源、身体关联与显示结果 | SavedEffects、Myosotis队列、Moving Box两份vector、Flip递归保存体、Ascent／Hourglass／Continue备份只在Host；不让Client再生成一遍，不把后台保存条目当活动实体 |
| 地图揭示 | 最终DisplayFlags、有效诅咒和实际图标／world显示限制；描述／地图结果同批提交 | map／blue-map／compass／full-map LevelStateFlag及canSeeEverything是生产输入，不新增四个揭图bool，不重放ShowMap／Apply*／UpdateVisibility；实际徽标另见第5节 |
| 小地图外观 | 两类图标Sprite的实际资源／必要可变图层外观、两份config独立border资源，最终震动Vector，challenge32已选锚点／对应描述／窗口上下文，实际有序道具／诅咒徽标 | Host Sprite地址／Image控制块／FBO、随机流和shakeDuration不发；不能只同步Sprite最后一帧便声称各徽标完整 |
| 查看操作／布局 | 使用同world事实在本地绘制，world强制隐藏／资格独立施加 | Minimap state、holdTime、按住／松开历史、displayed size、窗口／分辨率、展开alpha与排版缓存归各端本地；Host按Map不会强制访客展开 |
| 保存API及持久数据 | 必要生命周期变化触发源失效／新批次；只提交正常生产完成后的当前状态 | SaveState／RestoreState／Clear／Setter／RoomSwap及备份命令不作为网络RPC；不写访客存档、统计、成就、解锁，不借用原版Online序列化 |

“不发送完整保存体”不等于忽略玩法结果。Host回访后必须重新提供当前真实对象及其状态；Client退休实际已结束生命周期的旧use，并等待对应结构／资源／状态共同提交。连续存活的Player／Familiar不能仅因换房默认换身份；销毁后恢复的对象也不能只凭保存seed或ordinal续接旧use，具体连续性由真实owner核定。初次连接或重连安装当前world与地图事实，不依赖重放全部过去的保存／恢复事件。这里不包含host migration或Client自行模拟未来离房世界。

## 3. 保存API的实际语义与目录差异

官方 `LuaRoomDescriptor.cpp:171—215` 直接把三个Getter接到 descriptor 的 `+74 SavedEntities`、`+88 SavedEffects`、`+68 SavedGridEntities`。它们返回借用容器包装，不复制历史，也不刷新保存；当前活动房间的保存容器可以为空或陈旧。`GetRestrictedGridIndexes`遍历set并建Lua表，`AddRestrictedGridIndex`修改生成限制；本轮下行不需要这棵树，不能用vector方式遍历。

官方 `LuaEntitySaveState.cpp` 逐项核出以下差异；原始签名仍保留在机器表的原字段里，实际语义单列，不能悄悄改写历史目录：

| 接口 | 实际行为 | 对同步的影响 |
|---|---|---|
| EntitiesSaveState.GetI2／GetI3／GetI4 | Wiki明确Pickup保存的主动充能／价格／ShopItemId；J273 7E51D0的type5分支分别写+10／+14／+18 | 只证明type5特定臂，不证明其它type同义。充能／价格等当前显示由真实Pickup／HUD承接，+14还参与保存图标筛选，最终进L2；不发这些保存原值 |
| GetU1／SetU1 | type==5时为boolean，其余为有符号int8；Wiki和旧目录统一写int | 旧声明不是无损读取证明。当前不为下行读取U1，也不把true强制变成整数而掩盖类型分支；观测对照修复须按实际type保留区别 |
| EntitiesSaveStateVector.GetByType | 返回Lua表，1-based结果数组；variant／subtype的−1是通配。Get(Index)访问原vector，索引0-based | Wiki返回类型及旧目录是单个保存体，不能据此遗漏其它匹配项；原始索引／筛选结果表均不是C/S实体身份 |
| GridEntitiesSaveStateVector.GetByType | 同样返回Lua表，1-based结果；Get(Index)原容器0-based | 不能拿第一个匹配项代表完整保存集合；正式L2源只需相关type投影 |
| EntitiesSaveStateVector.Clear | 先ESSM::EntitySaveState_ClearBatch，再vector.clear() | 有管理／生命周期副作用，不是获取空快照的办法，不在Client重放 |
| GridEntitiesSaveStateVector.Clear | 源码已注册，Wiki及这98行未收录；逐元素把Type写0，**不清vector，不改变其长度** | 作为补充接口登记，不加入原98行计数。它的“空”是内容谓词，不是len==0；相关地图楼梯结果失效 |
| GetGridSpawnIdx／GetI7 | 调用ESSM的unhijacked访问器，详见第4节 | 禁止把+20／+30原始内存机械当成公开Getter值或网络身份 |

EntitiesSaveState不是完整活动Entity。它没有原生活动对象的完整Sprite、速度、AI、展示缓存和C/S use／shape语义；公开容器及回调没有逐对象saved-from／restored-from token。旧观测以不可变集合修订＋ordinal记录保存项是观测约定，不能据seed、位置、ordinal或ESSM ID把回访对象接回旧网络身份。本轮目录不声明已解决任何这类连续性。

## 4. 官方保存管理与生命周期边界

`EntitySaveStateManagement.cpp:644—647,755—772`显示ESSM把EntitySaveState+20的i16作为marker，把+30的u32作为内部ID；正常hijack marker为5247h，写出／读入／清理还有其它marker。原来的gridSpawnIdx／I7保存在`s_systemData.hijackedStates[id]`，公开Getter在hijacked状态返回旁表中的原值。ID可回收，序列化又有serializeId／realId映射；这不是可直接跨会话、跨进程复用的C/S身份。旁表物理ABI与任意状态的读资格没有在本轮核定；不调用unhijack／RegisterSaveState／GetEntityData来“读取”。

官方正常 `Room::save_entity` wrapper（2338行起）在super成功后维护ESSM、Flip及矿车保存；`restore_entity`（2398行起）在super前unhijack、返回后重新hijack并恢复扩展状态。即使第三方Mod不在范围内，官方保存owner仍实际存在，不能把未打补丁的J273原始字段直接等同官方运行时语义。`EntitySaveStateEx.h`中的Player twin／innate与Pickup override保存也留Host，其**当前**身体关系／有效效果／Pickup结果由既有域承接；不复制virtual对象、unique_ptr或JSON保存文件。`EntitySaveStateManager.Get*Data`会创建Mod数据表，TryGet版本也只属于迁出的Mod观测，不是本轮正式同步入口。

静态J273地址由上述SDK签名定位：`Room::SaveState=7E5D60`、`RestoreState=7E6250`、`save_entity=7E51D0`、`restore_entity=7E5730`。7E5D60有分支早退／跳过保存，且会改清房标志／计数、调用统计等生产路径；不是无副作用getter。7E6250创建Entity、逐条恢复、析构并清掉SavedEntities有效区间，另处理门／Grid，不能在Client作为“安装保存列表”调用。7E51D0按具体type选择存储臂，Pickup还分配循环内容及递归Flip保存；通用storage字段不能套一个含义。

| 公开事件 | 可确认的阶段 | 本轮使用边界 |
|---|---|---|
| 1304 POST_ROOM_SAVE_STATE | `CustomCallbacks.cpp:5307`在super返回后调用 | 可作为失效通知；原函数可能没有更新集合，回调本身不证明新保存修订，更不证明当前活动世界已完整提交 |
| 1305 PRE_ROOM_RESTORE_STATE | super前调用，wrapper临时Begin／EndIgnoreModifiers | 是恢复前阶段，不能当恢复后稳定状态读取；Client不回放回调或原函数 |
| 1306 POST_SWAP_ROOMS | 两描述交换后 | 描述source-slot、配置／拓扑／探索／L2对应关系共同失效；不能只按旧GridIndex沿用 |
| 1307／1308 Ascent保存／恢复 | stage＋descriptor＋key，官方额外ASM恢复点见ASMCallbacks.cpp | 备份留Host；只承接实际恢复后的新楼层／描述／对象结果，不把key当跨run身份 |
| 1302／1300、1303／1301 Hourglass | 源码分别在SaveState／RestoreState的super前／后 | Wiki把PRE_LOAD说明写成after，但源码5271行起明确在super前；slot不是公开备份内容。PRE_SAVE还可能收到−1，不能把任何此回调都算有效Hourglass备份 |

正式采集仍在已理解的主线程正常窗口，后台只持有自有值。生命周期回调只提供阶段与失效，不获得提前测试资格，也不证明官方Hook链的最终运行顺序。本轮不新增Hook；后续需要的正常生产点缓存必须纳入统一Hook清单。

## 5. Exploration不另建一套地图真值

DisplayFlags含可见／锁影／图标位，Wiki也明确未访问邻房能显示。`VisitedCount!=0`和clear投影与它独立；原始全DisplayFlags仍保留u32，因为原生既查具体位也查整体非零。地图／罗盘／蓝图等生产标志不能替代它，也不能从L2存在推断访问。世界诅咒采用L0已核的有效组合，不用+C原始值或Client本地种子重新组合。Hourglass回退、Curse of the Maze交换、红房、维度／楼层／配置替换都可能使旧探索修订失效，更新不要求单调增长。

保存地图图标完全沿用L2：缺席仅1B；存在为`u8 present + u16 mask + 按bit升序的必要u8动态帧`，3—12B，动态位mask为2B9Bh；同类OR／max、不依赖保存顺序和重复数。取得完整聚合后在本地按原固定优先顺序和配置显示上限排版，不只发送Host当前可见前缀。只读实际需要且已初始化的78h／20h元素范围；DisplayFlags为0的未显示描述可为present=0，不为完整观测额外扫描隐藏保存体。源读取失败／配置缺失／预算不足是Gap，不能伪装成mask0、空vector或缺席。

**地图道具／诅咒徽标与L2是两种结果。** J273 `Minimap::RenderIcons=966300`（SDK签名核定）先判断PlayerManager对333／21／246／54／376的实际有效持有结果：333存在时放`icons:5`并替代21／246／54三项；否则按21→0、246→1、54→2排列；376再追加4。然后按有效诅咒bit0—8顺序追加`curses:0—8`，整列最多7项。当前函数还逐项SetFrame并绘制，因此 **同步最后留在Sprite中的动画／帧不能代表完整徽标列**。已有Level核对已要求最终列表，本轮将其明确为具名交接，避免只保留四个揭图flag或漏掉这一消费。

采用一个尚未注册的 `MapBadgeList` 设计投影，由Presentation唯一持有：

| u8 count（0..7，1B） | i8 signed_icon_id[count]（0..7B） |
|---|---|

非负ID只允许0／1／2／4／5，对应`icons`动画；负ID−1..−9对应`curses`帧`~id`。它是最终有序结果，1—8B，不增加五个物品查询bool或九个诅咒bool。与L2不同，这列在原消费者内已按7项封顶；保留原优先级和顺序，不排序。位置／间隔由本地布局派生，Sprite文件／可变外观仍由资源owner承接，world显示资格独立施加。最终列表来源必须复用正常Host已产生结果或已核同world有效效果投影，不能额外执行PlayerManager查询／Render／随机补采。本轮只有静态算法与线布局草稿，实际生产点缓存、资源版本及writer未实现；正式注册前不接收此记录。

Minimap另12行的决定：GetShakeOffset同步最终Vector；GetIconsSprite／GetItemIconsSprite只承接实际资源与所需可变外观；GetState／GetHoldTime保留本地，GetDisplayedSize本地派生，GetShakeDuration留Host生产器。SetState／SetHoldTime是本地UI操作；SetShakeDuration／SetShakeOffset是Host显示结果生产操作；Refresh是缓存重建命令，均不网络回放。官方Update=965DB0涉及原版Online输入、随机震动和其它UI变化，Refresh=965D60→9635B0涉及FBO和challenge32随机选择，不能整函数放行。border Sprite虽没有在12行出现，仍由两份MinimapConfig资源承接；hudPickup Sprite“seems unused”只保留现有未证明边界。

## 6. 后续接线与完成口径

第9步注册L2、MapBadgeList、地图显示限制／资源结果的正式有界线布局，不扩充通用保存体网络类型；第10步落实同world来源、扫描预算和正常生产缓存；第12／13步验证身份、成员／资源／地图修订、mask和帧／有序徽标，并用自有地图数据及具名安全绘制adapter安装。不能把1—12B图标结果写成78h保存体、把缺口当空、放行Restore／随机Refresh／UpdateVisibility或借用原版Online。第14步只退休正式下行中的旧Lua／JSON room_save／exploration链，历史完整观测与事实按原职责保留。

第15步统一验收需要覆盖：地图可见但未访问、地图／罗盘／蓝图／全图／诅咒实际显示与独立本地展开；Pickup筛选条件与各动态图标、楼梯／Portal、相同聚合下重复及顺序变化不重发；徽标333覆盖三项／376追加／诅咒顺序／7项上限及两端本地布局；当前保存体陈旧／实际回访／Myosotis／Moving Box／Flip／矿车；Maze交换／红房／镜像维度／Ascent／Hourglass／合法Continue后的失效与重建；读失败／超预算不ACK且不改Client持久数据。测试仅在第15步，本轮未执行。

本次目录核对完成：197条原始行均有决定，额外API差异、原生输入及生命周期另列补充，不改变旧目录计数或生产版本。完整保存体私有语义、官方旁表物理ABI、RoomConfig资源、地图批次／缓存和正式reader／writer仍有既存工作；本次不把整个第7步或可玩同步写成完成。
