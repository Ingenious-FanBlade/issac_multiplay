# tx-04-19 I15—I19：Laser、Knife、Pickup、Familiar、Effect 应用

日期：2026-11-03，Asia/Singapore。本轮完成这五类的正式来源、纯值解析、类别注册、已采用原生最终值／资源／引用与正常退休路径，并更新产品目录和 CMake 接线。按本次会话要求直接提交；没有运行构建、分项测试、E2E、游戏或部署，不将静态实现说成运行通过。I20—I35继续按原计划，统一运行验收归I34。

## 类别实现

| 项目 | 本轮实现 |
| --- | --- |
| I15 | Laser公共专用量、完整128位flags、端点／角度／宽度／收缩／链数、内部关系；`sample_mode_raw`按int8保留，包括-1，与公开bool分别核对。`application_geometry v1`复制HomingLaser两份点集、int容器、每元素6个f32的Bezier容器及标量。四份vector各自拥有本地分配，不复制Source容器头，不调用CalculateSamples；self backpointer由真实ctor保留。 |
| I16 | Knife角度／偏移／距离／速度／蓄力／飞行／挥舞／spin及flags；Parent构造前置按Key取得真实实例，构造拓扑拒绝全Knife父环。官方KnifePlus hitboxSource经受保护leaf接口绑定、检查、解除。共享DeadEye ANM从正式来源采集并由进程级原owner管理，单Knife不析构它；Knife原Init保留全局资源Ensure，临时全局RNG使用已有窄隔离。 |
| I17 | Pickup价格／商店／选项组／Charge／Wait／DropDelay／State／Touched／blind／VarData；完整最多8项cycle按全量覆盖，空集合清旧。主、价格、alternate pedestal三个实际ANM分别安装。ghost、左右MegaChest及内部关系按Key维护EntityPtr；删除先解除出边，避免连带删除仍活的独立Key。跳过含抽池／生成／Flip玩法的派生Init，经官方base Init与Plus后安装最终量。 |
| I18 | Familiar公共专用量、轨道／跟随最终几何、颜色和按Key的Player裸借用。保留Factory ctor建立的Pathfinder／容器；不跑AI、EvaluateItems、Player重编队setter或生成Weapon。补variant48 IsaacsBody的5份ANM和4个selector、variant231 AbyssLocust的两份ANM vector，均由具体native ExtraData shared owner持有并核对。正常退休先撤独立引用、释放本方typed ExtraData，再走base Remove/Clear，避免原派生清理修改Player计数／位置／装备和Grid。 |
| I19 | Effect公共专用量及生命周期后态；TearFlags只对54／113／126／167按128位解释，136的完整Grid描述仍由I24安装。补108 MrMe的ANM、166 SpriteTrail完整64B记录序列、191 PickupGhost四份ANM和count；使用具体动态类型、游戏分配器和原生shared-owner替换／析构。由Source提供结果，不执行Effect生成／到期／玩法Init。 |

主要落点：`client_entity_class_fields.inc`、`client_entity_class_model.cpp`、`client_entity_adapters_native.cpp`；容器在`client_entity_collections_native.cpp`，typed ExtraData在`client_entity_owned_native.cpp`，沿同一ClientObjects执行器与实例表执行，没有第二套Scene。

## 正式来源与后态

Source声明新增必需合同`entity-class-application-v2`；旧consumer不能将新结构误认为已经应用。公开成员和版本化`application_geometry`／`application_owned`／`application_runtime`分开，未知来源和不完整数组写前失败。固定关系列由同一`client_entity_links.inc`生成名字，Null仍显式序列化；公共和内部引用均走现有稳定身份／依赖计划。

五种typed ExtraData分别是MrMe、SpriteTrail、PickupGhost、IsaacsBody、AbyssLocust。Source只借用当前同use、正常完成创建owner的对象，按具体vtable核定实际类型；协议不携带vptr、Source地址、容器头或引用计数。主／各辅助／各vector成员ANM逐个提供完整resource-pose；文件名相同不合并实例。新原生普通表也计入同一64M节点预算，几何／trail／辅助Sprite各有明确容量；不静默截断成可提交目标。

SpriteTrail的`+4/+8/+C/+10/+14`由主审结合Render与析构`606270`核定为带proxy的deque：map、容量、逻辑起点和数量，不采用子报告中“hash”的初始语义猜测。每个64B记录以16个u32位模式传输并检查其浮点消费，避免将不透明本地地址或容器头发给Client；本地重基为offset=0，保留完整逻辑顺序。替换通过真正shared owner析构旧deque；大块分配遵守游戏容器32字节对齐／前置基址约定，小块及外层统一调用J273游戏new/free。Abyss的原生ANM vector也使用同一堆及原生ANM ctor/dtor，不用SDK默认浅拷贝ANM。

字段、容器、资源和引用都沿现有check步骤回读当前native存储，不能只比较DTO。首次写入、资源替换及原生异常继续由公共执行器保留前缀并撤展示资格；完整异常收口和在途借用仍归I28。没有设置完整Entity域提交位，也没有发送网络ACK。

## 纯值保留与后续集成

命中数字／历史、配置／Pathfinder／Weapon查询及玩法能力判断保留Authority事实，不安装为本地身份、AI或装备。Pickup FlipSaveState只服务Authority的玩法恢复；Client安装其当前主／底座表现，不调用InitFlipState重抽保存状态。Knife HomingPath的构建仅用于更新；本轮安装当前距离／旋转／位置，不重演轨迹生成。

本轮主Render已核定实际消费的私有payload是上述五族；其他不被这些消费路径读取的私有玩法owner显式标为`AuthorityOnly`，只保留存在性事实，不创建玩法owner。实际需要五族payload的变体不能使用该标记代替；遇到未知RTTI明确Unavailable。Effect普通VarData、outer私有更新容器，以及Abyss的游戏专用表／关联不让本地Update消费，也不复制Source指针。I25新增具体消费者时必须重新核对这些去向，不能把此决定推广到未知draw或任意第三方Mod。

Familiar68／78／243需要Player复合绘制，仍写前返回`entity_familiar_player_composite_draw_pending_I25`；既有NPC私有draw拒绝清单保持原事实。这些目标不从全成员表中过滤，不冒称全形态／全类别画面完成。I25负责完整原生分层draw、shadow／light／water／mirror和临时随机／fade隔离，尤其Abyss Render中的`647310`不能被当作纯读；I24负责Effect136与Grid双向交接；I29负责唯一Scene提交；完整Player能力继续归20。

## 必要逆向与主审

按用户要求，由一个GPT-6 Luna max子智能体完成[本轮静态研究](entity-i15-i19-native.md)，主智能体先读本地两份Wiki和官方源码，再独立复读J273注册／Getter、构造、Render、Remove及实际vtable Clear槽。主审补证Knife Rotation `4D13C0/5B3350`是float`+438`，IsFlying `606C10`是byte`+464`；明确Laser `Angle` setter耦合而getter别名`+41C`；纠正Effect179清理读的是Target`+3C4`，以及Laser／Effect具有自己的Clear override。先前UNKNOWN保留研究时的事实，采用范围以上述实际代码和本节补证为准。

本轮没有测试结果；后续在I34对整个tx统一验收并连续修复／续测。不创建tx artifact包。
