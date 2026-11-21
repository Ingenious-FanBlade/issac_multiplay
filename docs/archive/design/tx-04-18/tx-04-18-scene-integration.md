# tx-04-18 I09—I25：Room/Grid 场景应用接线

2026-11-02，Asia/Singapore。I09—I18 复用已有领域 adapter；I19—I25 本轮完成实现与静态主审，完整纯构建成功。I26—I30 尚待推进，I29 统一实机验收没有执行，不报告像素、实机生命周期或完整联机通过。

## 范围与实现前失败模型

沿用[实施计划第7节](tx-04-18-implementation-plan.md)及[Grid owner失败模型](tx-04-18-grid-owner.md)。本轮按同一个 tx 完成引用、最终碰撞/路径与邻接结果、全部 Grid 原生 pass、独立记录边界、删除与失败退休及唯一场景提交。不另设 gate、哈希校验、分项测试或 tx artifact。

主要失败处理为：必需引用缺失、同版本冲突、无法支持的 Entity、旧 scope、非法成员在写前拒绝；原生分配/资源/字段/别名/缓存已写后失败撤销 A 和展示资格；借用或 CPU raw 队列未结束不释放；原生异常前缀不确定时保留 owner 和 Client 隔离，不盲重试 Free 或把旧 A 当回滚。短命对象只按最终完整成员决定存在性，不重放 Destroy/Hurt/Reward。

## I09—I25 对应关系

| 任务 | 实际实现与边界 |
| --- | --- |
| I09—I12 | 既有 `ClientRoomStateApply`、配置/背景、环境、轨道/L结构继续通过同一 `ClientNativeWorldPrepare/Finalize` 安装。新增正式 path 与 Grid 灯光来源，未引入清房、奖励、波次或 Room.Update。既有完成范围见[Room说明](tx-04-18-room-application.md)。 |
| I13—I18 | 既有 Camera/地图、27 type/17 cast、Grid工厂/字段/资源及Door别名 owner 接入本次场景。真实原生 Statue slot3 为 null，纠正早先“需要Room专用可见pass”的推测；按原版空绘制行为保留主体。 |
| I19 | `RoomGridTarget::lifted` 复制 Effect136 owner 和完整可空 Desc；Source 的 `source_grid` 继续 Unresolved。`ClientGrids::PrepareTarget` 先登记全体 Key 后检查 Grid/Entity/descriptor 依赖，`ResolveReferences` 对照实际 Entity owner，允许合法 Grid 环。Door descriptor 关系已由既有绑定安装；Pit.MakeBridge 没有持久 parent 指针，不创建虚构关联。`VisitLifted` 向19交付描述值，Effect136 的真实构造/专用应用仍由19负责；当前17代表 Entity adapter 在写前拒绝该未支持类别，不生成占位或第二份活动 Grid。 |
| I20 | `RoomApplication` 按当前 extent 复制 +76c 原始 path，`ClientRoomStateApply` 安装，`ClientNativeWorldCheckCommit` 最终核对。碰撞查询消费同一 Grid.CollisionClass 与 path，没有另一份 collision 数组。Rock/Pit/Wall 邻接结果来自 Source 最终 Variant/Anim/Sprite/层/桥接状态；不运行会改变邻居、Variant或随机的 UpdateNeighbors/PostInit。Path尾部448..455不读、不赋予语义。移除不使用 PathTrail/KeepDecoration 的玩法移除接口；保留装饰只来自完整目标的实际成员，目标 path 覆盖旧残留。 |
| I21 | `ClientGridDraw` 依次提供灯光、Decoration、Under、纹理坑、Door、Main、Dungeon与柱顶行深度 pass；轨道/坑轨道由 Room owner 排程，水面沿用原生独立 FBO。Door在z10000实体之后、shading/shadow合成之前；Main在合成之后；柱顶与实体按 `24030+row*4000` 交错。只在水面 Main pass过滤 bit2。Backdrop32/descriptor flags0x20 先由81BB30生产真实纹理坑surface，再经81C000和Pit重绘消费。 |
| I21资源 | 主/额外/TimerPlate/Fire资源沿用原生内嵌 ANM。整个draw栈为 Grid与Room两只灯光 Sprite深拷贝main/overlay选择器，并保护具名ANM与Layer显示标量；游戏临时索引由游戏allocator分配/释放，最后归还原索引。覆盖水面内部和碎屑Render的强制选动画，不调用 Sprite.Update/Load。没有在每个Grid pass中Present或重置主画面深度。 |
| I22 | `ClientGridEvents` 保留独立 scope/capture_kind/source_scope/required_state/value，限64项、单项64KiB、总1MiB；冲突副本/旧scope/背压明确失败，已完成历史淘汰后拒绝旧序号。完整目标没有的短命主体不重建。`ClientAdmitGridIndependent/ClientVisitGridIndependent/ClientCompleteGridIndependent` 向27交接，实际消费者完成才可确认该条；普通状态提交不确认独立效果，不推进网络事件水位。required_state=0没有版本依赖，保留为待绑定历史，不擅自套用当前 A；通用瞬时音画实现归27。 |
| I23 | 场景停止draw/借用、退尽CPU使用，退休展示owner与Entity映射，再由Grid owner撤所有Room/Door别名、清自有Ember/Rubble header、调用唯一派生Free；最后由原生Room owner析构。已发布与未发布实例均由表追踪，Room不再二次释放Grid。不确定异常/VM/context丢失继续隔离；可确定失败前缀按同一owner退休后从完整T重建。 |
| I24 | 新构造/应用/绘制仍在既有Manager/Render用途分派与访客持久保护内；保留官方工厂/Plus及实际WaterClip wrapper原链。五个灯光只读查询统一登记常驻Hook，仅Client GridLight栈按Source值返回，Host/Off与其它用途各走原链。没有新Hook owner、字节恢复、在线/Isaac Sync访问、跨线程对象或保存入口。 |
| I25 | 删除旧 `native_grid_integration_pending_I19_I25` 拒绝。重建也先预检全部纯值，再按Room结构→真实Entity→Grid→引用→背景收尾→展示缓存→Grid/Room最终检查→Scene提交推进。同一 attempt/scope/generation 下完成才更新 A。结果含revision、域/Key、Grid写前缀/owner数、首次失败阶段/原因、旧展示资格和恢复方向；退休进度不抹去首次失败。`committed_domains` 使用 `1<<DomainId`，这里只确认Room/Grid范围，不表示完整11域，不发送Applied ACK。 |

## 正式来源与能力

Room application_source=4、Grid application_source=3、Room/Grid application_runtime=2，能力 `room-grid-target-v4`。Envelope/schema v2、15 codec v1与RoomPresentation v2不变。旧应用来源显式拒绝，不补默认值。新增来源与调试录制无关，正常 consumer 同样得到 path、灯光五查询和Door +28c flags。

五项灯光输入依次为 `Seeds.HasSeedEffect(79)`、官方SDK `Game::GetStageID(false)`、J273完整路线条件748900、当前Boss房815B80、显示阶段编号745B10。后三项没有对应的已核定公开Getter，先查本地wiki/源码后由原生只读桥采最终结果。尤其748900不是 SDK `Level::IsAltPath`（745AF0）。其用途绑定分别检查 receiver、seed id与同步draw栈，避免使用Client代表Player或基础Stage重新裁决。新增5条Hook后统一清单为144/160。

`native_apply=true` 表示 Room/Grid 原生应用接线已经存在；`full_world_commit=false`、`runtime_acceptance=pending-I29` 明示当前边界。Entity19、Player20、探索24、规则25、通用HUD/音画27仍有自己的完整能力；本轮不解除17代表Entity/Player profile，也不把完整来源中的不支持对象悄悄删除后宣称应用成功。

## Luna 研究与主审

按用户要求启动一个 GPT-6 Luna / max 子智能体，先本地原版/REPENTOGON wiki，再源码/SDK，最后只读IDA。报告见[集成原生研究](tx-04-18-integration-native-research.md)。主智能体另取伪码/原始指令直接复读707F90、7146E0、70DE30、7149E0、801BE0、804500、81BB30、70DFC0、81C000、7E08E0及五个查询；并核A86DE5的operator new→malloc与A86BC9的j_j_free。

主审纠正Door排序、把BackdropId32误称RoomType、81C000把callee保存ESI误当入参、Poop特殊Under条件实际为State==1000、748900误配IsAltPath和Statue专用可见pass推测。修正落实到代码/报告；静态依据不替代I29像素或实际生命周期结果。

## 构建与验收状态

`pwsh -NoProfile -File scripts/build.ps1` 完整纯构建最终成功：Win32 Release `zhlIsaacCS.dll`、core/world/replica/net与i686 Rust harness完成编译链接，正式Lua/目录完成产品构建接线。生成的fixture/contract程序没有执行。

首次沙箱内MSBuild停在Checking File Globs，主动中断；获准在沙箱外编译。新增岩石资源检查曾将SDK `GetAnimationByName` 的optional索引误作指针而编译失败，改用 `GetAnimationData` 后完整构建通过。既有SDK/RapidJSON警告保留，未修改SDK。没有部署、游戏启动、probe、分项/E2E或单元测试，不留tx artifact。

后续I26补预算/稳定缓存和必要诊断；I27完成发布工程边界的统一审阅；I28/I29覆盖E01—E18、G01—G27及本文失败模型，按每次启动提示和准备确认连续执行整体验收；I30记录实际结果和交接。当前不存在这些实机通过结论。
