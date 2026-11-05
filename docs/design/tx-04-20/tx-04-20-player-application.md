# tx-04-20：玩家状态、角色与武器应用清单

日期：2026-11-05。当前I01—I40完成；I39核心114逻辑场景、18全49与5完整Player环境、19全65及负向／故障／退出通过，I40目录与交接已更新，实际结果见[统一验收记录](tx-04-20-e2e.md)。下列P／C／W表保留完整需求及本地Wiki语义；实现与有限运行覆盖分别报告，枚举注册不能代替后态／画面验证。配合[主计划](tx-04-20-implementation-plan.md)、[逐签名目录](player-application.json)及[静态研究](tx-04-20-native-research-plan.md)使用。

## 1. 字段目录的建立方法

本文件列完整工作族；逐成员签名和来源沿用 [Entity 目录](../observation-expansion/entity-catalog.md)、[continuity 目录](../observation-expansion/continuity-catalog.md)，不在此复制数百行可能失步的 Getter。I01 必须将其中所有 Player 相关行及新增 native 来源映射到下列 P 族，形成机器可读应用目录；不能只处理这里举出的字段。

目录记录 `member/signature/parameters/source_scope/read_basis/role/ownership/disposition/native_consumer/resource_owner/retirement/task/research/e2e/status`，参数查询有独立范围、默认值和空结果语义。公开 setter 不自动等于安全最终值安装。按角色不适用、来源未捕获、必需值缺失和合法 Null 分开。

成员归属的基本规则：

- Entity 记录是本体唯一真值；player 记录引用同窗口本体，负责关系、连续性与 History。
- 参与者、控制组、身体、原生 roster 索引、武器／服装子对象是不同身份空间。
- 原生消费者需要的字段必须有真实安装；只作协议知识或后续 HUD 数据的字段可保留纯值，但要写明消费者，不用“保留”掩盖缺实现。
- 操作／随机／攻击参数生成接口保留 Authority 的事实或实际事件，Client 不重放；只有明确定义的表现事件可交 27。
- 未支持字段若为当前目标必需，拒绝该目标并说明原因；不得缩小 profile 后宣布整个 Player 完成。

## 2. P01—P20 全状态族

### P01：身份、控制组、roster 和备份

**任务：I02、I07—I10、I23—I26；研究：R01—R03、R10／R11；覆盖：E02—E08。**

处理 subject/use/shape、participant／group、control_session／binding_revision、body_slot／body_role、active／can_consume、in_public_roster、main／other twin、sub、parent／child、替换与 Esau Jr 槽。control_relation 只描述实际输入锚点；membership 可描述合格非活动身体，二者不能互换。

应用时先建立所有权图，再决定原生列表成员；循环关系有界解析。活动、可见、存活和可输入不是同一标志。备份槽保存精确槽号与当前身体引用，不能按参与者排序生成；关系缺失不构成删除证据。

### P02：Entity 公共最终状态和空间表现

**任务：I10／I11／I28；研究：R04／R13；覆盖：E09／E20／E21。**

覆盖继承 Entity 的位置／速度／偏移／缩放、颜色与 flags、碰撞／大小／质量、FrameCount、深度、渲染偏移、存在性和全部公共引用。保留 Source 身份字段的诊断意义，不将 Source Index、指针或容器位置直接安装为本地身份。

Player 不是 19 的普通 NPC adapter；公共字段的物理布局可以复用已核事实，但 owner、生命周期和可读资格须按 Player 核定。任何插值／临时 draw 工作值与最终逻辑状态分开，不能在 Client 运行运动／碰撞更新。

### P03：心量、生命形态和受击表现

**任务：I12／I28；研究：R04；覆盖：E09／E21。**

包括 red／max／soul／black／eternal／bone／golden／rotten／broken hearts、黑心掩码／排列、心量上限与角色规则、额外生命当前结果、dead／ghost／revive 后态、受击闪烁和无敌表现需要的最终计数。逐项说明半心单位、bitmask 和容器来源。

Lost、Keeper、Forgotten／Soul、Bethany 等不能用普通 Isaac 的数值不变量一刀切。写心量不是 AddHearts／TakeDamage；表现计数由来源供给，不让本地 tick 自行推演到死亡或复活。

### P04：普通资源和专用充能／计数

**任务：I13／I27；研究：R04／R12；覆盖：E09／E16。**

金币、炸弹、钥匙及金色／无限等实际标志；魂充能／血充能、角色专用条／计数和正式来源中的其他资源。区分每身体值、明确共享 owner 和整局事实，不通过同值推断共享。

装入最终值时不得支付、捐款、生成掉落、更新成就。角色变更／归零／不适用后，旧成员应按合同清除或退休，不能留旧角色计数供 draw 误用。

### P05：最终属性、方向与动作当前量

**任务：I11／I20／I29；研究：R04／R08／R13；覆盖：E09／E14／E17。**

Damage、MoveSpeed、MaxFireDelay、ShotSpeed、TearRange、Luck 及现有目录全部属性；头／身体／射击方向、动作阶段、蓄力／冷却、飞行／可见性、必要武器及动画状态。记录哪些是最终值、哪些是只读计算结果、哪些实际 consumer 仍要求原生 backing storage。

缓存 flags 与最终属性分开；不置一个“需要重算”位再跑 EvaluateItems。当前动作展示不由本地键盘决定，也不重复触发边沿动画／射击。

### P06：道具、饰品、固有与熔炼库存

**任务：I14／I17／I27；研究：R05／R12；覆盖：E10／E16。**

完整道具计数与实际库存次序、饰品槽／金饰品／熔炼条目、固有 collectible／trinket 分组、阻止／禁用／可见性与效果乘数。当前完整有限集合与参数查询结果分别记录，不能仅传 `HasCollectible` 默认参数或当前一件物品。

库存、临时效果、History 和 costume 不互相替代。Source 最终非 Player 成员决定 Familiar／wisp 等存在性，Client 修改库存不能再次生成它们。

### P07：主动槽和完整 ActiveItemDesc

**任务：I15／I20／I26；研究：R05／R08／R11；覆盖：E08／E11。**

每个正式槽包含 Item、Charge、BatteryCharge、PartialCharge、SubCharge、TimedRechargeCooldown、VarData；另保留实际消费者需要的最大／最小可用充能、slot kind 和有效性。字段单位／范围及 VarData 的角色特定含义由源码核定。

槽更换和空槽是完整后态；不调用放电或使用道具。Esau Jr 的槽关联不是通用整数复制的例外漏洞，应解析为明确备份槽关系并安全绑定。

### P08：口袋、卡药和排队取得物品

**任务：I15／I18／I31；研究：R05／R07／R14；覆盖：E11／E13／E19。**

全部卡／药／PocketItem 槽、口袋主动、物品类型与 ID、队列中的 QueueItemData 及其展示依赖。区分槽空、槽不适用、未捕获，以及 queued item 与已入库存。

举起道具动画和队列消耗结果由 Source 提供；Client 不自动推进队列或触发物品回调。已知药丸效果／鉴定归整局知识，20 负责玩家当前槽和所需数据接口，25／27 继续其世界与界面语义。

### P09：TemporaryEffects 和变身

**任务：I16／I19／I27；研究：R06／R07／R12；覆盖：E12／E16。**

collectible／trinket／null 三类临时效果的有序或计数集合、有效时长、来源标识和变身状态／计数。完整列表不存在公开安全读法时先查实际容器，不能无限枚举或通过 Add／Remove 反推。

临时效果移除必须清旧资源／显示依赖；最终属性与 costume 来自各自规范值，不在 Client 重做道具触发链。真实 RNG 结果属于 Authority，不复制出一份会继续运行的玩法 RNG。

### P10：History 与跨身体共享语义

**任务：I17／I31；研究：R03／R06；覆盖：E05—E08／E12／E19。**

HistoryItem 的 ID、trinket 标志、获得顺序／时间、stage／stage type／room type／pool 等公开记录及完整性；共享资源关系单独引用具体字段／槽 owner。历史空洞与空历史不能混用。

跨房保留同一逻辑身体的历史，run／use 改变不能误继承；不为恢复历史而重新授予物品。为 HUD27 暴露纯值顺序视图，是否需要同步原生 History 容器取决于实际 native consumer，不能默认执行保存／获得链。

### P11：主／身体／头／手／辅助资源

**任务：I18／I21／I29；研究：R07／R09／R13；覆盖：E13／E18／E20。**

枚举实际 ANM 成员与共享层、主／overlay 精确姿态、层状态、null-frame、shader／champion shader、颜色／缩放、替换图和辅助动作资源。身体、头和手并不预设各有独立 Sprite；必须按真实布局登记，避免凭概念创建不存在的成员。

相同文件名不等于同一实例；GetBodySprite 与主 Sprite 的不同当前姿态必须保留。换图／角色重配后重取借用，原生资源分配器与 renderer context 合同沿用 17—19。

### P12：costume 描述、容器与层次

**任务：I19／I29；研究：R07／R13；覆盖：E13／E16／E20。**

每项 CostumeSpriteDesc 的身份／配置、优先级／顺序、对应身体部位、实际资源、姿态和可见性。覆盖物品、null costume、角色原有外观、飞行／头身覆盖、卸装和动态重载所需状态。

不采用“先 AddCollectible 再删掉副作用”的方案。必须知道容器复制／移动／销毁的 owner、图像在途引用和 Lua wrapper 失效；真实空容器应清除所有旧项，不能只覆盖前 N 项。

### P13：Weapon 与关联攻击实体

**任务：I20／I29；研究：R08／R13；覆盖：E14／E15。**

所有实际武器槽及 W01—W15：类型、修饰、方向、charge／max charge、delay／max delay、num fired、owner、main entity、实际 item animation 与必要私有表现数据。Weapon.GetMainEntity 合法 Null 与解析失败分开。

构造／退休可连带写 Entity 或 Player，必须登记；实体本体继续归 19。类型变化使旧武器 slot generation 失效，不保留一个裸指针当稳定身份。只读 Getter 未暴露的必要 draw 缓存不能由 Client 发射一次补出。

### P14：举持、Suplex、矿车及复合 draw

**任务：I21／I24／I27／I29；研究：R09／R10／R12／R13；覆盖：E06／E18。**

精确 held Key／shape、ItemState／SuplexState、Body null-frame、持物姿态、minecart 关系和 Source 偏移；真实投掷前后成员与 Player 代画关系分别表达。19 的 136 岩石及描述变化是已有窄事实，不外推全部持物。

父级代画的 Entity 不强制 Visible=true；绘制中临时改值保持到外层消费完毕并恢复。由 NPC／Familiar 间接调用 Player draw 的路径也受当前 Player owner 和 pass 合同约束。

### P15：全部具名跨对象关系

**任务：I10／I20／I21／I28；研究：R03／R08／R09／R15；覆盖：E15／E18／E21／E22。**

本体 parent／child／target／spawner／last、twin／sub、持有／跟随／Familiar／Weapon／矿车、替换／备份，以及必要官方扩展关系。每条关系登记目标类别、可空性、作用域、构造前置还是可后置、正反向更新及清理责任。

允许的循环先分配必要结构再绑定，不允许用首个同类对象解开循环。删除／换形／owner 退休后引用必须清除或重新解析；已过期目标即使地址复用也不能继续借用。

### P16：角色专用结果

**任务：I22—I27；研究：R10—R12；覆盖：E03—E08／E16。**

按下文 C 表逐族展开，涵盖库存顺序、crafting／poop 队列、魂／血充能、狂暴／变身、hologram、clot／wisp／locust／fetus／Dark Esau 关联及必要角色资源。普通共有字段不能代替这些专用输入。

角色原生过程仍由 Authority 完成；Client 安装最终结果。每个族都需来源、安装、draw、清理及合法切换合同，未实现的族明确未完成，不能用“后续内容组合”掩盖。

### P17：意图、控制资格和输入换代

**任务：I30；研究：R03／R14；覆盖：E17／E24。**

同组一个 canonical 源，多个身体按既有 Authority 语义读取；Client 不按身体复制 Submit。控制组／连接／binding revision 与身体 shape、资源版本分别失效，非活动备份不可消费。

本地设备数或 ControllerIndex 不是服务器的参与者归属。不能调用原版 IsLocalPlayer／Online 路径解决 C/S 归属；用正式 C/S 关系。重绑恢复窗口沿现行规则，不通过保持旧 held 掩盖缺输入。

### P18：HUD 与面向表现的只读数据

**任务：I31；研究：R14；覆盖：E19。**

为每参与者／身体提供心量布局、资源、充能、槽位、角色专用条、History 和共享关系；区分当前身体与备份的显示选择。完整 HUD27 自己负责布局、动画、事件水位和音效。

若 Player 原生 draw／生命周期直接依赖 PlayerHUD，20 需创建安全必需结构；不能以“HUD 留后续”为理由保留悬空引用。只读视图只来自已提交 A，不获取游戏对象跨线程使用。

### P19：存在性、换形、退休和续用

**任务：I06／I28／I33；研究：R02／R15；覆盖：E21／E23／E25。**

Exists、dead、ghost、in_roster、active、binding valid、retired 独立建模。普通切房不自动摧毁持久身体；当前 tx 支持新 scope 的安全重建／交接，不承诺已完成 23 的网络场景事务。

同 Key 变形和同址新 Key 需要不同处理；旧 borrow／costume／Weapon／Lua／draw cache 一起撤销。正常清理和未知异常分别说明，不通过永久保留所有 Player 来回避可核定的正常退休。

### P20：Authority-only 调用、独立事件和不适用项

**任务：I01／I03／I05／I32／I34；研究：R01／R16；覆盖：E22／E24／E27／E30。**

对目录中的操作／actual-call／external-reference／derived-query 逐行说明：是 Authority 实际事件、纯值知识、随机域结果、独立表现交接，还是明确不适用于 Client。GetData 不为采集创建状态表，随机 Getter 不轮询，IsLocalPlayer 保持原版 Online 隔离。

短命 Entity／动作在两次快照间发生不必重建为常驻对象；独立表现交 27 且保留自身 scope／顺序。只完成 Player 普通状态不确认独立事件已播放，不把未处理的协议知识写成原生已安装。

## 3. C01—C17 全角色／身体族

枚举入口：[本地 PlayerType](F:/projects_isaac/IsaacDocs/docs/enums/PlayerType.md)。本 tx 的原版角色支持集合为 0—40；枚举中的 `NUM_PLAYER_TYPES=41` 是原版计数，不是可按该枚举名构造的角色，也不是动态角色配置的总上限。已加载的模组角色可以使用 41 及更高 ID，其合法性与映射须由实际配置及声明适配确定，不能只凭数值范围放行。目标是官方 REPENTOGON 1.1.3／J273，若本地文档与目标接口有差异由 R12 明确，不把新版 Wiki 数字直接写内存。

2026-11-04 职责明确：下表覆盖原版角色及其基本应用；模组角色的身份、自定义状态、Lua 行为、专用表现和生命周期统一由 tx-04-27 的声明 Mod 适配处理，来源复用 tx-04-14。本 tx 保持现有窄适配接口，不提前实现通用 Mod 角色框架，也不因这项分工推迟下表原版族的必要状态／资源／关联。动态配置依据见 REPENTOGON 的 [EntityConfig::GetPlayer](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntityConfig.zhl) 与 [XMLDataHolder::AssignId](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/XMLData.cpp)。

| 族 | 类型／关系 | 本 tx 具体实现要求 | 任务／研究／E2E |
| --- | --- | --- | --- |
| C01 | 0 Isaac、1 Magdalene、2 Cain、3 Judas、4 Blue Baby、5 Eve、6 Samson、7 Azazel、8 Lazarus、9 Eden、11 Lazarus2、12 Black Judas | 普通外观族逐类型核定心量／资源／最终形态、飞行、默认身体资源和武器差异；Eden 已选结果不能重新随机，复活形态不重新发装备 | I22；R04／R07／R12；E03／E09／E13 |
| C02 | 10 Lost、14 Keeper、31 Tainted Lost、33 Tainted Keeper | 非普通心量和货币／护盾等实际显示结果，不套普通 max-heart 条件；受击／消失／飞行按最终值 | I12／I22／I27；R04／R12；E03／E09／E16 |
| C03 | 13 Lilith、32 Tainted Lilith | 本体／眼部外观、Familiar／胎儿的具名 owner；胎儿／鞭不是第二个独立玩家；19 Entity 与 W14／W15 衔接 | I20／I22／I27；R08／R12；E03／E15／E16 |
| C04 | 15 Apollyon、34 Tainted Apollyon | 已吸收结果、当前主动与 locust 关系／资源；不在 Client Void／Abyss、不重做物品消耗 | I15／I20／I22／I27；R05／R12；E03／E11／E16 |
| C05 | 18 Bethany、36 Tainted Bethany | 魂／血充能、主动槽、wisp 集合及关系、角色条数据；Familiar 唯一构造 owner | I13／I15／I22／I27；R04／R12；E03／E09／E16 |
| C06 | 19 Jacob、20 Esau | 一组两活动身体；不同库存／心量／姿态，四组八体共存；主副关系和局部退出 | I23；R03／R10；E04／E05 |
| C07 | 16 Forgotten、17 Soul | 普通两形态、主／sub、同对象交换事实、不同心量和骨棒；原位还是替换按 Client 合同选择 | I24；R10；E06 |
| C08 | 35 Tainted Forgotten、40 Tainted Soul | 携带／抛掷／落地结果，非 roster 身体、灵魂／骸骨的活动和可见性；不是36 | I21／I24；R09／R10；E06／E18 |
| C09 | 29 Tainted Lazarus、38 Tainted Lazarus2；Birthright hologram | 两套装备与资源、Flip、hologram 实际活动／输入限制、递归退休 | I25；R11；E07 |
| C10 | Esau Jr 703 引发的活动／备份／恢复关系，非新的 PlayerType | 全局四槽及真实主动槽关联；每身体状态、资源及来源依据；退出不承诺回收 Authority 四槽 | I26；R11；E08 |
| C11 | 21 Tainted Isaac | 有序受限库存、当前可见物品／装备结果；增删替换不重新触发物品链 | I14／I27；R05／R12；E10／E16 |
| C12 | 22 Tainted Magdalene、24 Tainted Judas、28 Tainted Azazel | 各自实际专用动作／资源／临时状态和身体表现；只按 Source 安装特殊攻击／短期能力结果，不在 Client 裁决 | I13／I16／I20／I27；R12／R13；E16／E20 |
| C13 | 23 Tainted Cain | crafting bag 有序内容／槽、当前结果及展示输入，不调用配方／随机掉落生成补数据 | I14／I15／I27；R05／R12；E11／E16 |
| C14 | 25 Tainted Blue Baby | poop spell 队列／当前槽／状态／资源，合法空和重排；实际生成对象归19 | I15／I27；R05／R12；E11／E16 |
| C15 | 26 Tainted Eve | 当前心量／特殊状态、clot 集合及具名所属与资源，不重做消耗心量／生成 clot | I12／I27；R12；E09／E16 |
| C16 | 27 Tainted Samson、30 Tainted Eden | 狂暴／骨武器与变身结果；重掷后装备／资源／属性一起替换，不在 Client 重掷 | I16／I20／I27／I28；R08／R12；E14／E16／E21 |
| C17 | 37 Tainted Jacob、39 Tainted Jacob2 | 当前身体形态、Dark Esau 具名非 Player 关联及最终表现；不让关联 NPC 占新参与者名额 | I10／I27／I28；R12；E16／E21 |

另行分类 `PLAYER_POSSESSOR=-1`、Baby／特殊原生内部身体：先查是否会作为上述正式目标的必要中间／关联对象。如果确为必需，I08／I27 补对应合法结构和生命周期；若属于原版 Online 专用或不在当前来源的独立玩法，不得用它取得原版联机状态，也不伪装普通 Player。未知值明确不支持，不能按 `type<=40` 放行所有负数。

最低角色变化覆盖包括：全类型基本后态、四组八体、普通／双体混合、两套 Forgotten、Flip 与 Birthright、Esau Jr 两次真实切换、角色装备变更和当前死亡／幽灵／复活状态。后续29负责组合爆炸，不免除任何此表基本族的实现。

## 4. W01—W15 全武器类型

枚举入口：[本地 WeaponType](F:/projects_isaac/IsaacDocs/docs/enums/WeaponType.md)；公开值入口：[REP Weapon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)、[REP EntityPlayer](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)。16 是计数 sentinel。下列是应核定的消费者／场景，不预先假定每种都有非空 MainEntity 或同一私有布局。

| 族／值 | 武器 | 具体状态／依赖任务和代表变化 |
| --- | --- | --- |
| W01／1 | Tears | 普通方向／delay／修饰，普通和多发结果；现存 Tear 归19，不补 FireTear |
| W02／2 | Brimstone | charge／max、身体／头蓄力姿态、活动 Laser 的 owner／main entity；松键后最终状态 |
| W03／3 | Laser（Technology） | 当前 Laser 关系、方向及身体资源；不自行生成光束 |
| W04／4 | Knife | 武器 owner、Knife 主实体、挂载／伸出／返回；不能用19的NPC Parent夹具代替真实Player依赖 |
| W05／5 | Bombs（Dr. Fetus） | 发射姿态／delay、当前 bomb 结果；Client 不引爆或重建炸弹 |
| W06／6 | Rockets（Epic Fetus） | 瞄准表现／关联对象和最终阶段；不运行本地目标选择或轰炸 |
| W07／7 | Monstro's Lungs | 蓄力／方向／释放后态及实际派生资源，不用普通 Tears 名称掩盖私有表现 |
| W08／8 | Ludovico Technique | 持续 Tear 的稳定关系／存在性、owner 变化和清除，不重新发射补实体 |
| W09／9 | Tech X | charge、圆环 Laser／几何 owner 与身体姿态；同一 Entity19 几何合同 |
| W10／10 | Bone | Forgotten／相关狂暴形态的骨棒与挥击／投掷最终表现、所需辅助资源 |
| W11／11 | Notched Axe | 手持资源／item animation／当前挥动与格子关系，不在 Client 破坏 Grid |
| W12／12 | Urn of Souls | 当前充能／释放表现与相关攻击 Entity／资源；不额外消耗魂 |
| W13／13 | Spirit Sword | 刀身／挥击／蓄力／方向及 owner，核清真实类型和私有 draw，不预设就是普通 Knife |
| W14／14 | Fetus（C Section） | 当前胎儿攻击／owner 与本体资源，实际 Entity 分类按来源，不预设 Child 是 Player |
| W15／15 | Umbilical Whip | Tainted Lilith 胎儿／鞭／身体连接、最终几何和资源，清理跨对象依赖 |

每种 W 由 I20／R08 负责，I29／R13 接 draw、I33／R15 接退休；统一 E14／E15 覆盖。每种至少有实际适用的有效目标、变化／空槽或退休后态以及 Owner／MainEntity 的真实核对；具备充能／多阶段语义的类型补相应阶段。`GetWeapon` 参数范围和多槽关系先核定，不只用示例 `GetWeapon(1)` 当成完整武器集合。

## 5. 应用目录的完成口径

I01 的目录至少有“已编目、正式来源、发送保留、原生处置、静态依据、已实现、运行后态、画面覆盖、未决原因”这些独立信息。只有实际实现的状态族及实际 E2E 场景能填完成；一个枚举值命中注册表不算该族通过。

对角色／武器组合的缺口，明确是基本 adapter 缺失、必要私有来源缺失、owner／退休未知，还是29负责的额外组合覆盖。前面三种仍属本 tx 工作。未知 ABI／布局／释放不猜测，当前目标失败不截断；形成具名后续任务不能自动将本 tx 改判完成。

## 6. I40实现边界与后续接口

完整目标使用`player-source-v2`／`player-target-v2`，Entity本体与Player关系／History来自同一普通采样窗口。完整源、角色／装备／资源依赖和实际原生检查全部满足，才在同一Scene attempt提交Room、Grid、Entity、Player位（当前位集142）；旧单Isaac代表profile保持自己的范围，不能置完整Entity／Player位。此谓词不是网络Applied ACK，21／22负责真正下行、版本恢复与提交闭环。

字段目录分别说明具名原生存储、提交后只读captured query和Authority-only操作；捕获查询保留原参数／状态／图根，不在Client重新运行任意Lua查询，也不把全部目录行冒称已独立验证所有参数。非活动来源允许本次已观察的完整创建owner2、具名子体／完整Init4、完整状态owner5；旧exact-pair PlayerUpdate读取依据3保持其会话限制，不扩展成任意备用体资格。Lazarus Birthright双活动twin与结构休眠backup分开核定，Exists本身不等同活动资格。

四参与者、每组64身体、总256身体、4全局备份、4096资源、65536层、32MiB目标及256MiB纯值展开是拒绝上限，不是已运行的最大规模；本轮实际压力为15身体，含8活动／4全局备份及其他引用休眠体。Scene候选retained容量1056按单身体1024 costume和主／内嵌资源安装窗口配置，仍须等CPU／GPU／context借用安全退尽。完整成本和实际运行结果见[统一验收记录](tx-04-20-e2e.md)。

| 后续tx | 接口与边界 |
| --- | --- |
| 21／22 | 同window目标、精确域位、具名拒绝原因、安全退休后完整T恢复一次；Authority普通／odd consumer实际缺席仍需分别处理，不能用processed ACK当实际消费 |
| 23—26 | 稳定Key／use／shape／实例与成员资格分离；死亡仍存在、ghost、复活、换形、backup和组退出不能复活旧输入 |
| 27 | 已提交participant／body／slot视图、原查询语义与真实动态HUD依赖；完整HUD布局、全局过场、独立音画及声明Mod角色适配仍属27 |
| 28／29 | 原版0—40基本最终量／身体族和15武器动态工厂最终态；自然开火／全阶段、全部道具和角色组合、965:10专用乘员pass不从有限矩阵推导 |
| 32 | 逐阶段CPU、IPC墙钟与整个进程内存分列；Source ordinary之后的原生插值／更新与截图时序分开，Client不以Update追帧；不承诺实时10Mbps或完整四端网络性能 |
