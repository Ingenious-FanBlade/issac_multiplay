# tx-04-18：全部Grid逐类实现任务

日期：2026-11-02。状态：**计划，G01—G27均待实现／统一验收。** 本文是[实现计划I17](tx-04-18-implementation-plan.md#i17-grid)的组成部分；编号对应 `GridEntityType` 数值，不另建tx，不分项提前测试。

类型以本地[原版枚举](F:/projects_isaac/IsaacDocs/docs/enums/GridEntityType.md)及11实际来源为依据；公开字段见[Grid逐成员目录](../observation-expansion/grid-catalog.md)。原版Wiki的自然用途说明、11实际受控工厂结果和18将来应用结果分别记录。本文没有把未研究的派生Init／Render分支认定为无副作用。

字段类型和版本细节同时对照[PressurePlate](F:/projects_isaac/IsaacDocs/docs/GridEntityPressurePlate.md)、[Poop](F:/projects_isaac/IsaacDocs/docs/GridEntityPoop.md)和[REPENTOGON Door修订](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/GridEntityDoor.md)：TimerPlate是可空Sprite，UnderPlayer是布尔值，ReducedSpawnRate有版本移除说明，Door Previous字段有特定更新时点。实现时仍以目标版本官方绑定及正式来源实际状态落实。

## 1. 每一类都必须实现的共同内容

以下C01—C08适用于全部G项，各项下文只补其专有要求。不能以“本类无专用公开字段”免除共同内容。

| 项目 | 每个类别必须交付 |
| --- | --- |
| C01 类型与目标解析 | 真实Type／Variant／Desc、稳定Key、所属Room、格索引、合法派生转换；未知组合显式失败，不能降级成其他类别 |
| C02 创建与本地有效性 | 实际原生分配／工厂、必要Init／PostInit及官方扩展维护，写前／写后失败前缀，构造owner与Room接管时点 |
| C03 状态安装 | 公共State／VarData／Position／CollisionClass与本类字段、Desc八项；区分源Initialized与本地构造状态 |
| C04 资源和显示 | 主／额外Sprite、ANM／图层／相位／颜色／偏移、render位置、水裁切及实际派生draw；不通过玩法Update推进动画 |
| C05 关系与缓存 | 格子／门／邻居／桥接／依赖引用、碰撞／路径及特有缓存；同格替换不会串用旧对象或GridEntityPlus |
| C06 change与replace | 同Key改变、必要时同Key换实例、同格新Key、回到基线值、资源变更；明确每种操作实际改什么 |
| C07 移除与恢复 | 非玩法删除、别名／借用／资源退场、部分失败和整个Room退休；不用Destroy／掉落路径代替remove |
| C08 统一验收 | 真正Source状态→正式模型→真实Client应用；实际类型／字段／Desc／引用及画面对照，重复目标无额外玩法副作用 |

通用水裁切按服务器有效值应用；来源没有暴露“默认／override”区别时，不宣称重建了未观测的覆盖历史。源与目标应明确所比较的是有效值。Grid对象构造／Init会重置官方按index管理的副表，安装顺序与退休时点由I15处理。

`GRID_NULL=0`表示无Grid对象，不设置G00构造器。显式删除和完整集合的空格可以产生空槽；非法类型／未知记录不得借GRID_NULL静默省略。

## 2. 全部27类任务

### G01：`GRID_DECORATION = 1`

- **实现：** 装饰本体、Desc、位置、Sprite／图层、颜色、裁切和绘制层；处理装饰与主格子成员的实际关系、保留装饰和明确删除。接入 `ToDecoration` 的真实转换结果。
- **注意：** `KeepDecoration`必须服从目标，不能所有remove都保留或都删除；装饰不应被当作另一份岩石／碰撞主体。Init／Render是否使用随机或环境条件按采用路径核定。
- **完成：** 装饰变化／换图／移除后无残影，原主格对象换代时保留或删除行为与目标一致，重复应用不叠加装饰。

### G02：`GRID_ROCK = 2`

- **实现：** 普通岩石状态、`Anim`、`FrameCnt`、`RubbleAnim`、BigRockFrame相关结果、邻居连接、上层绘制和破坏后最终形态；使用真实 `ToRock` 与公共合同。
- **注意：** `UpdateNeighbors/UpdateCollision/UpdateAnimFrame/RenderTop`分别核定，不靠Grid.Update整链修复。不得调用SpawnDrops、RegisterRockDestroyed或TrySpawnLadder来重建目标。
- **完成：** 单块／相邻／大岩组合的完整和破坏状态正确，顶层遮挡与碰撞一致，客户端不掉落或累加破坏统计。

### G03：`GRID_ROCKB = 3`

- **实现：** 方块实际变体、资源、State／Desc、碰撞和邻接，复用岩石公共部分但保留本类型的状态／显示分支。
- **注意：** 不依据“石头外观”假定与G02具有相同可破坏性、帧选择或碰撞变化；按服务器最终结果应用。
- **完成：** 方块不会被降级为普通岩石；存在、状态变化、同格替换和非玩法删除均正确。

### G04：`GRID_ROCKT = 4`

- **实现：** 标记岩石的Type／Variant、标记外观、当前／破坏状态及Desc，关联背景所需资源和相邻帧。
- **注意：** 标记／奖励选择由Authority决定；本地不能重新挑选标记岩石，也不能破坏一次来生成正确画面。
- **完成：** 来源指定岩石与标记准确对应，破坏后无二次掉落、额外解锁或统计写入。

### G05：`GRID_ROCK_BOMB = 5`

- **实现：** 爆炸岩石的最终状态、姿态、碰撞和消失／残留；爆炸产生的持续对象通过Entity接口依赖表达。
- **注意：** 不调用Hurt／Destroy模拟引爆；初始化及移除中潜在的爆炸／连锁路径须隔离。瞬时爆炸表现与Grid存在性分开。
- **完成：** 改成已破坏／已移除状态不会造成客户端伤害、连锁破坏或新炸弹，最终格子与资源正确。

### G06：`GRID_ROCK_ALT = 6`

- **实现：** 与场景有关的壶／蘑菇／骷髅等来源最终Type、Variant、State、资源、碎屑和必要背景依赖；保留不同外观／状态分支。
- **注意：** 不用Client自己的Stage／Backdrop猜外观；不能由Destroy重抽药丸、卡牌、敌人或毒效。若构造依赖真实背景，先解析其资源与几何版本。
- **完成：** 代表性的不同环境资源分支均可应用，破坏和换图不重复抽取或生成对象。

### G07：`GRID_PIT = 7`

- **实现：** 坑本体、相邻边缘、桥接后的最终状态、`HasLadder`、碰撞／路径和水裁切；桥接父对象按实际来源记录依赖。支持有桥／无桥、有梯／无梯及邻格改变。
- **注意：** `MakeBridge`、`SetLadder`、`UpdateCollision`是否可作为受控维护逐项核定，不重放过去的桥接行为。父对象缺失不能随便借用同格岩石；L形缺口不能当相邻坑。
- **完成：** 桥／梯／坑边、水面与查询一致，引用失效和同格替换后无旧桥或旧阻挡。

### G08：`GRID_SPIKES = 8`

- **实现：** 尖刺Type／Variant／State、`Timeout`、实际动画／显示层与碰撞最终值；接入 `ToSpikes`。
- **注意：** 客户端不因站位调用伤害，不自行递减Timeout或判定缩回；原生Render若依赖附加字段，必须有来源或明确派生合同。
- **完成：** 展示和阶段跟随服务器目标，重复应用／静置显示不会自主改变尖刺或玩家生命。

### G09：`GRID_SPIKES_ONOFF = 9`

- **实现：** 开关尖刺的开启／关闭／过渡目标、Timeout、姿态及碰撞，明确与G08的不同状态分支。
- **注意：** 本地帧数不能驱动开关周期；相同目标多次提交不能重置／推进服务器计时。不能只复制Sprite而留下相反碰撞状态。
- **完成：** 开→关→开及中间姿态一致，无自主循环或伤害；回到先前值真实生效。

### G10：`GRID_SPIDERWEB = 10`

- **实现：** 蜘蛛网的存在性、Desc／State、资源、地面层、裁切及相应格子查询结果，保留 `ToWeb` 类型信息。
- **注意：** 减速／接触属于Authority，不通过Client玩家更新来使网生效；网消失时同步撤销相关缓存。
- **完成：** 蜘蛛网不因显示产生本地移动修正；删除／替换后无残图或旧格子结果。

### G11：`GRID_LOCK = 11`

- **实现：** 钥匙方块的锁定／解锁／移除最终状态、资源及碰撞；接入 `ToLock`，声明需要的玩家引用或纯展示信息。
- **注意：** 不调用 `TryUnlock(Player, TryPayToPlay)`扣钥匙／硬币；显示解锁不执行解锁裁决，缺Player也不创建占位玩家。
- **完成：** 状态与通行结构一致，Client任何重复应用不消费资源或产生第二次解锁事件。

### G12：`GRID_TNT = 12`

- **实现：** TNT状态、`FrameCnt`、姿态、损伤／引爆后最终形态、Desc和格子移除；区别实体Bomb与GridTNT。
- **注意：** 不重放Hurt／Destroy／爆炸。11已发现部分官方回调userdata元表差异，来源沿用其安全借用，不为18修改官方全局元表；实际应用不依赖回调重演。
- **完成：** 各必要阶段及删除正确，Client无额外伤害／连锁／爆炸对象，类型与回调身份不混同。

### G13：`GRID_FIREPLACE = 13`

- **实现：** 原生Grid Fire实例、实际可用资源／姿态／State／Desc、裁切及退休，保留 `ToFire` 结果；与Entity火堆分开。
- **注意：** Wiki标记该类型自然用途为“not used”，11受控工厂实际生成过且曾有资源动画警告；不能因此删掉类型，也不能据生成成功声称其资源完整。须明确可用原生表示，处理实际缺图／缺动画，禁止伪装成Entity火堆。
- **完成：** 工厂支持的该类型能真实应用和安全显示／退休；资源问题若未解决保持该类未完成，受控覆盖不宣称自然房间覆盖。

### G14：`GRID_POOP = 14`

- **实现：** 便便状态／损伤、字符串 `StateAnimation`、整数 `ReviveTimer`、布尔 `UnderPlayer`及专用外观／碰撞。目录中的 `ReducedSpawnRate`按目标实际可用性单独登记；原版Wiki标记其在Repentance移除，不能直接作为必有可写字段。
- **注意：** 不调用RespawnRedPoop、ReduceSpawnRate或破坏操作重演历史；客户端不自行倒计时复生或抽取奖励。`UnderPlayer`不是Player指针。历史字段在目标确不可用时保留明确Unavailable／版本归属，不伪造setter或缺省值；若官方目标绑定确实提供，则按真实来源补合同。
- **完成：** 完整／受损／破坏／服务器复生目标均正确；本地无自主复生、掉落或错误身体关系。

### G15：`GRID_WALL = 15`

- **实现：** 墙Grid存在性、资源、位置、碰撞、邻接和前后遮挡；接入 `ToWall`，区分Room边界几何和实际墙Grid。
- **注意：** 背景墙与Grid墙不能重复创建／绘制；特殊房形和无墙场景按最终结构处理，不能用普通矩形补满。
- **完成：** 普通／L形／相关专用房间的墙体和遮挡准确，墙移除或替换后不保留旧碰撞／阴影。

### G16：`GRID_DOOR = 16`

- **实现：** 门本体 `Busted`、Direction、Slot、CurrentRoomType／TargetRoomType／TargetRoomIndex、PreviousState／PreviousVariant、各开关／锁定动画名、Sprite offset、ExtraVisible／ExtraSprite，以及锁／开／破开查询对应状态。I18负责Room侧全部别名和路由。
- **注意：** REPENTOGON已把PreviousState／PreviousVariant修正为整数；它们仅在初始化／播放动画等时点更新，不能每次强置为当前State／Variant。Open／Close／PlayAnimation等须理解后才作为安装手段，不调用TryUnlock／TryBlowOpen／SpawnDust重复玩法或表现。
- **完成：** 主／额外Sprite及所有门状态准确；同槽替换、目标变化、明确空引用和删除后无悬空门，不会自主过门。

### G17：`GRID_TRAPDOOR = 17`

- **实现：** 活板门的关闭／打开／目标所需阶段、Desc、资源及地面层，保留 `ToTrapDoor` 与目的相关描述。
- **注意：** 入口显示不允许Client执行下层生成、玩家吸入、结局或存档；相关流程由Authority与后续26／28处理。不能通过进入活板门初始化目标画面。
- **完成：** 入口可正确出现／变化／消失，但Client显示／接触不引发独立换层或持久操作。

### G18：`GRID_STAIRS = 18`

- **实现：** 梯子／楼梯入口Grid本体、State／Desc、资源／位置、裁切及已提供入口关系；保留 `ToStairs`。
- **注意：** 与G07坑上的HasLadder是不同语义；夹层／黑市进入和目标Room构建不能由该Grid本地Update触发。
- **完成：** 梯子入口与坑梯各自正确存在，变化／删除无串用；无本地传送或第二个房间。

### G19：`GRID_GRAVITY = 19`

- **实现：** Gravity类型真实构造／状态／Desc／资源及必要格子结果，接入 `ToGravity` 并查明采用分支的专有展示前置。
- **注意：** 公开派生接口无详细字段说明不等于内部没有状态。不能猜它与装饰／坑相同，也不能为显示执行引力、位移或其他玩法更新。
- **完成：** 该类型可安全创建、改变、移除和绘制，实际必要隐藏字段有明确来源；未解释分支保持具体缺口。

### G20：`GRID_PRESSURE_PLATE = 20`

- **实现：** 压力板State／姿态、字符串 `NextGreedAnimation`、可空Sprite `TimerPlate`及当前格子／门／房间进度关联；覆盖普通和Greed所需表现分支。TimerPlate按I21资源owner管理，复制其真实姿态，不构造第二个Grid。
- **注意：** `GreedModeRNG`是Authority玩法流，不复制为Client可运行抽奖器；`Reward`、刷波次、扣血启动不能作为状态安装。Wiki明确提示不存在的NextGreedAnimation动画名可导致崩溃，须在任何原生写入前核对目标资源中的动画。TimerPlate的nil和读取失败不能合并。
- **完成：** 未踩／已触发及计时Sprite状态一致，非法动画写前拒绝，重复应用无二次奖励／刷怪／扣血，资源失效安全。

### G21：`GRID_STATUE = 21`

- **实现：** 雕像的实际Type／Variant、State、Desc、Sprite、碰撞、阴影／专用绘制，保留 `ToStatue`，按采用变体记录所需环境／Entity依赖。
- **注意：** 不能通过射击、伤害、机关或生成守卫重演雕像状态；有专用动画事件的资源须保持状态驱动而非本地消费事件。
- **完成：** 雕像当前形态和变化准确，无本地攻击、生成物或持久副作用；必要引用均可退休。

### G22：`GRID_ROCK_SS = 22`

- **实现：** 超级标记岩石的实际标记、专用状态／姿态、Desc和邻居／碰撞；明确与G04不同的分支及所需破坏中间状态。
- **注意：** 不以奖励数量或需要几次伤害作为客户端推进规则；只使用Authority最终状态，不重抽额外奖励。
- **完成：** 特有外观／状态正确，重复目标不会产生额外掉落／破坏次数，基类共享实现保留本类型身份。

### G23：`GRID_TELEPORTER = 23`

- **实现：** 传送器Grid本体、状态、资源／动画、位置、裁切与目的关系的已提供表示，接入 `ToTeleporter`。
- **注意：** 不由本地位置碰撞选择目的地，不调用传送或重新抽取房间；目标Room／维度Key与门路由值分别处理。
- **完成：** 出现／启动外观／关闭／移除准确，Client不自主改变当前Room或消费传送事件。

### G24：`GRID_PILLAR = 24`

- **实现：** 柱子的真实类型、可用cast结果、构造／状态／Desc／资源和碰撞／遮挡；根据实际目标类型接入相应公共adapter并保留专用分支。
- **注意：** 公开接口没有 `ToPillar` 不能作为跳过理由；也不能仅凭名称断言它是 `GridEntityRock` 或Wall。具体原生类、Init／Render／退休按I04核定。
- **完成：** 柱子可实际创建、变化、替换和删除，资源层次正确，类型与副作用处理有明确实现依据。

### G25：`GRID_ROCK_SPIKED = 25`

- **实现：** 带刺岩石的刺外观、岩石／碎屑状态、专用碰撞／遮挡和Desc；保留与普通岩石不同的资源及危险表现。
- **注意：** 可复用岩石公共缓存但不能丢失尖刺外观；本地接触伤害禁止，不调用伤害流程验证危险性。
- **完成：** 完整／改变／破坏后的最终状态与Source一致，Client不独立扣血，换成其他岩石后无旧刺资源。

### G26：`GRID_ROCK_ALT2 = 26`

- **实现：** 标记骷髅的Type／Variant、标记／资源、State／Desc、破坏最终形态及背景依赖。
- **注意：** Wiki描述其固定掉落愚者卡，这仍是Authority裁决；Client不能因为类型固定就自行创建卡牌。不能与G06按外观合并掉类型身份。
- **完成：** 标记、资源和破坏状态正确，卡牌等结果仅来自19承载的权威Entity，无本地生成。

### G27：`GRID_ROCK_GOLD = 27`

- **实现：** 金岩的实际状态、金色专用资源／效果、Desc、邻接／碰撞及破坏残留；明确哪些光效属于Grid绘制、哪些为独立Entity。
- **注意：** 不通过破坏／生成金币／传播效果来形成目标画面；类型专用资源不能只靠普通岩石染色假定等价。
- **完成：** 金岩存在、变化、移除和同格替换均正确，无额外金币／效果实例；跨域效果依赖按真实能力报告。

## 3. 17个公开派生接口的核对责任

| 接口 | 主要任务 | 除公共内容外必须处理 |
| --- | --- | --- |
| `ToDecoration` | G01 | 装饰保留／移除与绘制层 |
| `ToDoor` | G16、I18 | 主／额外Sprite、Previous字段、门槽／别名／目标 |
| `ToFire` | G13 | 工厂支持与实际资源可用性，不外推自然使用 |
| `ToGravity` | G19 | 采用分支专有展示前置和隔离 |
| `ToLock` | G11 | 锁最终状态，不执行TryUnlock |
| `ToPit` | G07 | HasLadder、桥接、邻居边缘、碰撞 |
| `ToPoop` | G14 | ReviveTimer／StateAnimation／布尔UnderPlayer；ReducedSpawnRate按目标可用性处理 |
| `ToPressurePlate` | G20 | NextGreedAnimation合法动画名／可空Sprite TimerPlate；GreedModeRNG保留房主归属 |
| `ToRock` | G02及实际返回该接口的岩石类型 | Anim／FrameCnt／RubbleAnim／BigRockFrame、上层／邻居缓存 |
| `ToSpikes` | G08、G09 | Timeout与最终开关状态 |
| `ToStairs` | G18 | 与坑梯分开，不触发房间流程 |
| `ToStatue` | G21 | 真实变体资源／状态，无本地攻击／生成 |
| `ToTNT` | G12 | FrameCnt及阶段，不重放爆炸 |
| `ToTeleporter` | G23 | 目标关系与传送裁决分离 |
| `ToTrapDoor` | G17 | 入口表现与换层／结局分离 |
| `ToWall` | G15 | 与背景边界墙区分、遮挡／碰撞 |
| `ToWeb` | G10 | 地面层及格子状态，无本地减速模拟 |

上表是接口任务归属，不是把每个枚举硬编码成某个C++类的证明。Pillar及任何多接口／继承结果以11实际观测和I04目标版本事实决定；全部cast结果、Null、Unsupported与读取失败保持原语义。

## 4. 每类完成时应更新的记录

每项G任务在实际实施时补充：正式解析／adapter位置，采用构造／change／remove入口及依据，支持的专用分支，必需字段／资源／引用，失败退休策略，所属整体E2E场景及实际结果。记录进入本目录或代码内的功能注释，不生成逐项artifact，不要求独立人员／分项测试。

发生以下任一种情况不得标该类完成：只在目录列出类型；只在Source生成过；Client用另一类占位；只显示单独Sprite而缺少必要原生对象／成员；字段通过但原生画面错误；遇到未知值直接丢弃；删除仍残留门／邻居指针；初始化／显示／退休仍有已知二次玩法副作用。
