# tx-04-12 房间与楼层实现及整体验收

> 2026-10-31补全完成：原5/5保留为旧范围历史结果；房间及Greed完整清单206项已完成，同tx统一E2E **7/7 PASS**。当前范围、连续修复与验收见[补全清单](tx-04-12-special-room-coverage.md)和[补充实现及统一验收](tx-04-12-supplement.md)。

日期：2026-10-30。历史阶段状态：房间身份/构建、进度、环境及楼层结构的完整公开观测目标已实现；构建通过，同一tx连续修复和收尾续测后，最终统一E2E **5/5 PASS**。

正式来源位于 `mod/observation/isaac-cs-observation/domains/room` 与 `domains/level`，公共值复制器承接有界配置容器和域引用扩展；与Entity/Grid共用唯一native复制窗口、RNG见证、来源/root末尾复核及Authority提交。Room先建立上下文，随后Level及Entity/Grid取得同批分层引用。记录器是可选消费端，输入不依赖采集/磁盘。

[逐成员目录](room-level-catalog.md)及JSON由两套本地Wiki转换，包含Room、RoomDescriptor、Level、RoomConfigRoom/Spawn/Entry/Stage、RoomConfig、RoomTransition、Backdrop、FXParams/FXLayers、ColorModifier、L房边界、RailManager、Camera与相关枚举/回调。所有动作保留目录但不轮询；随机选择/掉落/懒加载明确禁止执行，显式只读参数查询与有限枚举/448格/8门槽遍历分别表达。`RoomConfigStage.GetRoomSet`会加载，`Room.GetItemPool`即使传种子也会求值C++默认参数表达式`Isaac::genrand_int32()`，二者均不进入轮询或显式只读白名单。

Run与RoomInstance沿用native观察器实际代次；Floor使用已观察新楼层边界及Stage/StageType/DungeonPlacementSeed当前见证建立，首次遇见保留FIRST_OBSERVED含义。Dimension按实际0/1/2命名且属于Floor；Descriptor按Floor/Dimension/ListIndex建立有界主体，GridIndex变化撤销旧槽关联。列表位置、GridIndex、SafeGridIndex、当前房号分别保留。1024项表溢出返回Unresolved/不完整，不借地址填身份。普通回访保留Descriptor但使用新的RoomInstance，不宣称Player销毁。

实际pre-change/pre-new-room/pre-level-init/退出/交换通知立即撤销当前窗口和latest Room/Level，过渡记录将前位置放在payload.previous，新位置保持UnknownUntilOrdinaryPost。原生run/room撤销同样在帧首撤销上下文；不依赖Lua通知成功。稳定样本继承同批floor/dimension/descriptor/instance；最终窗口不稳定或未提交取消所有六个投影并撤销活动位置。独立事件/render/query不借旧Authority tick。

Room复制全部当前公开状态、实际配置/Override和Spawn/Entry、具名种子、进度/清房/访问/伏击/献祭/奖励/商店计数、颜色/水流/光照/阴影/雨/熔岩/FX/背景资源与布局/轨道。Entity/Grid只存主体和membership，不复制本体。Door形状允许、Descriptor.AllowedDoors、Doors形状目标、邻接候选和实际GridDoor引用分别表达。FXLayers只公开AddPoopFx，缺少只读层容器，明确Unavailable；活动效果仍引用Entity域，不能宣称捕获引擎私有存储。

Level遍历GetRooms的全部实际成员，以每个Descriptor.GetDimension归入维度集合；空维度注明在已遍历列表中为空。当前特殊调试描述不在常规列表时单列current_descriptor，不伪造列表成员。描述保存集/API由tx-04-13负责，概率/随机流统一目录由14负责；本轮只复制具名RNG当前seed/shift。配置资源与描述/活动实例分开，相同variant不作为Data/Override内容相同的依据，两者分别完整复制；配置lookup_key不冒充资源身份。每30个普通采集窗口完整重读，sample_revision为本次scope，cache_reused=false，不假称具备未证实的长期缓存失效能力。

Schema v2增加 `isaac.cs.room`、`isaac.cs.level`、`.level.transition`、`.room.event`、`.room.render`、`.room.query`、`.level.query`及`.level.event`；reader和录制manifest同步，整个世界仍world_complete=false。各新增普通域4MiB/262144节点/12层；pending从4增到6，扩展预留160MiB涵盖六份pending/四份latest/临时编码及层级表，总预算仍512MiB，超限/失败明确不完整，主线程不等待写盘。未启用消费者时不枚举。

统一入口：`scripts/accept-tx-04-12.ps1`。场景含实际进度/环境变化、清房事件、显式查询/RNG不推进、邻门切房/回访、1x1/2x2/L形、献祭/商店/挑战房、Downpour镜像、Mines、Death Certificate维度、Ascent标志、暂停、停录继续正式采集、停止与正常退出/部署恢复。判据检查真实状态、跨域同窗上下文、Descriptor不跨Floor/Dimension复用、过渡不挂旧tick及reader/delivery。未触发状态如实保留，不外推所有路线/种子/第三方Mod或性能。

## 实际验收记录

第一次实机在初始采集失败：配置Doors位掩码错误进入描述Doors数组分支，Room Failed、Level Partial。已按owner区分，退出/部署/配置恢复完成，继续本tx完整续测。进入统一整体验收前未执行程序测试，未编写实现后单元测试，不校验哈希、不设gate、不新增Hook owner。

第二次续测完整走完全部场景，48组Room/Level普通后态均Complete，实际维度0/1/2、四个Floor、房形1/2/8/9及特殊房类型1/2/5/7/11/13均出现；最终归约发现Room显式Vector查询参数未按类型编码（结果本身正确），该条Partial，整体FAIL。已按目录签名逐项复制参数，保留原失败并继续完整续测；环境恢复及存档范围检查通过。

第三次完整续测5/5 PASS；收尾发现sample_revision插入后字节计账未更新，已修复并新增原始JSON payload字节精确比较，manifest同时声明房间/楼层目录。第四次完整续测5/5 PASS，49组Room/Level普通后态均Complete。随后按实际接线收窄Room/Level阶段能力，配置Data/Override分别复制、不按variant省略内容，进入本tx最终完整续测。


第五次完整续测最终 **5/5 PASS**。全部Room/Level普通、过渡、生成事件、进度事件、render和显式query记录均Complete；新增精确字节计账、实际阶段能力与Data/Override分别复制均随完整场景续测。程序正常退出0，输入/观测均正常停止，部署/Helper/配置恢复及存档变化范围检查通过，无新增崩溃文件；所有本tx临时材料和已核定失败启动trace均已清理，不保留artifact。失败事实保留在上文，不以最终通过改写前两次失败。

## 最终实际覆盖及限制

| 范围 | 第五次实际结果 |
| --- | --- |
| 成员目录 | 400条成员/签名来源行、27个枚举文档、36条回调来源行；目录不是400个运行场景 |
| 普通后态 | Room 48条、Level 48条，全部Complete；最大26065节点，低于262144节点上限 |
| 身份及结构 | 四个Floor、Dimension 0/1/2、114个Descriptor主体、15个RoomInstance代次；描述主体不跨楼层/维度复用；回访同Descriptor新RoomInstance；同tick跨域上下文一致 |
| 房间构建 | 实际RoomShape 1/2/8/9、RoomType 1/2/5/7/11/13；实际Spawn/Entry配置、Override、形状与布局、八门槽及原GridDoor主体引用、描述启用与邻接候选/实际门分别保留 |
| 进度 | Clear false/true、真实MC_POST_ROOM_TRIGGER_CLEAR、NoReward、商店索引、献祭/挑战/机关相关描述原值、回访状态 |
| 环境 | 水量0.75及水流(1.25,-0.5)、FX ShadowAlpha=0.375、颜色/光照/背景资源和持续参数；状态变化由实际Getter结果见证 |
| 过渡与维度 | 45条Complete过渡；Downpour镜像进入/退出、Mines楼层、Death Certificate维度、Ascent标志实际出现；未在过渡挂旧tick/新旧位置混代 |
| 事件及查询 | Level生成/选择/放置请求165条、Room进度事件3条、独立render4条、Room/Level显式查询各1条；显式查询前后GenerationRNG seed不推进，Vector按声明类型复制 |
| 录制与计账 | 停录后文件不增长，正式Room/Level继续发布且epoch不变；每条新增域记录payload字节与原始JSON编码相符，采集loss_sequence=0、发布record_loss_sequence=0 |
| reader与退出 | 71421条v2记录，无未知schema，delivery_complete=true；trace_dropped/query_mismatches/cleanup_failed均0；正常退出及恢复通过 |

实际Greed模式波次回调、自然献祭奖励链、Mineshaft内部完整行走、所有特殊路线/房形/变体/种子、每条显式参数请求、任意第三方Mod和极端负载未逐一运行，不能由本次有限场景外推。相关公开字段/回调已实现，未触发场景仍为未观察。FXLayers内部持续集合没有只读公开接口，明确Unavailable；带随机/懒加载副作用的公开接口不由观测执行。非活动保存/探索历史与整局随机/HUD分别归tx-04-13/14；整个世界仍world_complete=false。

本轮同时录制的旧Entity/Grid独立事件保留1377条实际Partial及1815条未调度/odd等Gap，不删除这些事实、不把Room/Level的Complete外推为全世界Complete。完整观测不是Replica、世界写回或状态同步，未触及原版Online/Isaac Sync。
