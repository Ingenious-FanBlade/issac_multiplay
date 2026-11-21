# tx-04-19：非 Player Entity 逐类别应用计划

日期：2026-11-03。状态：**C01—C05的类别值、资源、引用与正常生命周期路径已实现**，见[I10—I14实现与具名拒绝边界](tx-04-19-class-application.md)。C01／C02的未明私有容器、拥有的actor及复合draw继续明确拒绝并交I25／20；全类别draw调度归I25。C06—C13及统一E2E待实施。下文是完整类别目标，不将代码／观测存在写成已运行通过的全类别清单。执行、公共任务和统一验收见[总实施计划](tx-04-19-implementation-plan.md)，原生前置见[静态研究清单](tx-04-19-native-research-plan.md)。

I04 后续静态结论见[全类别采用合同](entity-application-contracts.md)、[J273必要逆向与主审](entity-native-static.md)及[功能目录研究索引](../../../../mod/binding/catalog/entity-native-contracts.json)。字段宽度、池大小、专用资源／owner、Timeout文档冲突和失败前缀以具名证据为准；下面的类别任务仍须实际实现并在I34统一验收，不能把静态结果改成native支持。

公共I05—I09的阶段执行器、字段表与Player/依赖接口见[公共应用实现](tx-04-19-common-application.md)。C01—C13须注册完整`ClientEntityClassAdapter`；未注册类别在写前拒绝，不能将公共执行器存在当作该类别已经实现。

## 1. 全类别共同要求

每类任务均包含：正式完整来源 → 发送字段 → 纯值解析 → 构造与官方扩展 → 最终公共／派生状态 → 引用及辅助资源 → 原生绘制 → 改变／替换／移除 → 失败退休。下文的字段主题是实现导航；具体全量范围以 10 的 `entity-catalog.json`、实际 schema、I01 应用目录及新增必要原生表现字段为准，不能把示例字段当上限。

每类在功能目录登记明确的适用 Type／Variant／SubType／派生接口，状态与形态改变策略、原生 owner、必须有的关系和资源、已核定调用路径及完整性要求。普通变体共用实现；具有不同布局、初始化、draw、资源或退休语义的变体建立专用分支。当前 29 的完整组合验收不能成为漏掉本 tx 必需类别逻辑的理由。

所有初始化、删除、资源重建和 draw 的副作用都按 Client 用途核定。禁止调用 AI／碰撞／Damage／Pickup／UseItem／开奖来得到“相同结果”，禁止仅用主 Sprite 替代实际类别。类别任务可以按依赖交错实施，只有全部目标完成后运行同一整体 E2E。

## 2. 逐类别任务

### C01：EntityNPC（I11，R03；E02）

**范围与输入：** Entity 公共字段，NPC 状态／状态帧／计时、血量／状态效果、目标／路径的公开状态、冠军／Boss／阶段／部件关系、配置与动态外观；PathFinder 等关联值分别判断是本地展示所需还是仅保留纯值的 Authority 状态。分类来自真实派生类型，不以“Type≥10”包办 Text／Dummy／Effect。

**实现步骤：**

1. 按配置和实际派生合同取得原生 NPC use，明确 Init 中依赖的 Room／Player／部件。安装 Source 的最终外观、冠军与状态，不让 Client 自行选择冠军、Boss 状态或部件。
2. 逐项安装公共和 NPC 状态，部件均进入共同 Key 表；父子／目标／部件链在合法阶段绑定。PathFinder 不运行寻路或 AI 来恢复 Source。
3. 核定 NPC 组合 draw、附属 Sprite、阴影和状态着色所需缓存；如缺来源，由 I03 补只读最终值，不按 HP 重新运行伤害流程。
4. type／variant／subtype、配置或特殊形态变化由 plan 明确原位或替换；默认不把 NPC Morph 全链放行，已知 bestiary 写入纳入隔离。

**失败与完成判据：** 配置／必需部件缺失、初始化附带未对应目标的实体、非法 champion 状态或未核定资源 owner 均给具体错误。普通、冠军、多部件及死亡中仍存在的 NPC 有可应用合同和真实后态；无 AI、掉落、清房／开门裁决。特殊 NPC 尚未适配分支逐项登记，不用整体“支持 NPC”掩盖。

### C02：EntityDelirium（I11，R03；E03）

**范围与输入：** 在 C01 之上解析 `special` 中全部 EntityDelirium 成员，保留当前变身形态、适用专用状态与 Source 的 shape revision。它是专用接口，不是额外复制一份逻辑 Entity。

**实现步骤：** 梳理 `ToDelirium` 的适用条件和官方扩展寿命；明确特殊状态的安装顺序及变身时普通 NPC 配置与专用状态的关系。采用已解释的原位变化或本地实例替换，逻辑 Key 保持，重建必要资源／部件，撤销旧借用。变身结果来自目标，不调用随机变形逻辑。

**失败与完成判据：** 当前类与 special 数据不匹配、变身只换图但未换必要布局、旧部件泄漏等必须拒绝或退休重建。专用字段、资源及引用均有来源和适配；实际变形通过 I34 观察，不能只以普通 NPC 场景代替。

### C03：EntityTear（I12，R04；E04／E18）

**范围与输入：** TearFlags／伤害／尺寸／高度／下落速度与加速度／旋转／轨迹／附着／目标、专用变量、命中集合及相关表现；主 Sprite 和 `GetDeadEyeSprite`、`GetTearEffectSprite`、`GetTearHaloSprite` 三份独立辅助 ANM2。

**实现步骤：** 创建已核定的 Tear 实例，安装当前高度／尺寸／旋转及必要 scale 缓存，再安装各资源和姿态。bitset128 保持完整宽度；命中列表若含本地 Index，先解释身份语义，不把 Source 整数当 Client 原生实体索引。附着／目标通过共同引用服务处理。派生 Render 中的临时位置／颜色变化限当前 draw 借用，恢复顺序明确。

**失败与完成判据：** 不调用 FireSplitTear、重算碰撞、Update 或随机 flags 选择。清空辅助 Sprite／命中列表及 Morph 后资源有显式处理。高度、旗标、附着变化和三种辅助资源均能表达，Client 不生成第二份分裂 Tear；仅声明已解释分支的实际覆盖。

### C04：EntityProjectile（I13，R04；E04）

**范围与输入：** projectile flags、伤害／碰撞、目标、速度／高度／下落／加速、方向／转弯／变形相关状态和派生外观。共用 Tear 的值工具，但不把两类结构、flags 或 Init 当作同一合同。

**实现步骤：** 核定专用工厂／Init 与可安全安装的派生字段；由 Source 的轨迹后态和资源选择准备原生展示，必要缓存有正式来源或纯表现推导。变化与删除按自己的 owner 和引用处理，不执行落地、分裂或命中回调。

**失败与完成判据：** 高度／加速／旋转与适用性检查明确，轨迹参数不被错误地当本地模拟指令。代表不同高度与 flags 的完整目标可真实绘制、清空与退休，且没有新 projectile／伤害结果产生。

### C05：EntityBomb（I14，R04；E05）

**范围与输入：** 引爆计时与当前状态、flags、伤害／范围、尺寸／闪烁、附着／所有者／专用变量以及 costume layer Sprite 集合。目录中具有真实玩法调用语义的 Getter 仍不能主动轮询。

**实现步骤：** 建立无爆炸副作用的原生实例，按适用分支安装已观测计时、外观及独立 costume owner。明确 `GetBombFlags` 的历史随机副作用与当前所需值的区别：从已有真实调用记录／可纯读的已核定字段取得必要结果，不靠 false 参数假定只读。倒计时、碰撞和爆炸不在 Client 自行推进。

**失败与完成判据：** owner 失效、costume 数组不完整、flags 无有效来源时不得补默认值。Source 爆炸后以新 T 的 Bomb 删除、Grid 改变和其他 Entity 结果体现，Client 不另执行爆炸／掉落；所有辅助资源按实际 owner 退休。

### C06：EntityLaser／HomingLaser（I15，R05；E06）

**范围与输入：** 类型、起止位置／角度／长度／半径／宽度／缩放、超时／收缩、parent／follow／链式关系、伤害／flags、命中信息，以及直线、圆形、sample／弯曲激光所需最终几何。HomingLaser 为关联内部结构，不是另一 Entity 网络身份。

**实现步骤：**

1. 对公开 `GetSamples`／`GetNonOptimizedSamples` 的当前结果逐项确定实际 draw 消费关系、容器 owner 和更新／清空规则，分清原始和优化采样。
2. 若公开点集足够安装原生几何，写入已核定容器；若仍需私有曲线／顶点／宽度／截断状态，I03 补 Source 当前结果的窄复制及版本。不能只复制不透明指针，也不能以 `RecalculateSamplesNextUpdate` 或 `CalculateSamples` 补跑逻辑。
3. 核定 parent 何时必须可用、派生 Init 的资源／声音／链式 spawn；先满足构造依赖，后绑可后置链。直线、圆形与 sample 分支分别装配和检查。
4. 为采样数组／关联 owner 制定独立点数、字节、候选和在途寿命预算，换形或清空先撤旧借用，保留必要原生 beam draw。

**失败与完成判据：** 不完整点集、非有限几何、版本不符、数组 owner 未明不能退化成直线。`SampleLaser` 沿用 10 已核定的布尔来源；激光宽度／端点／弯曲形状与链式关系正确，删除无旧几何残影、无命中或分裂副作用。

### C07：EntityKnife（I16，R06；E07）

**范围与输入：** owner／parent、挂载位置、角度／距离／旋转、伸出／投掷／返回／蓄力阶段、伤害及专用资源状态；凡 draw 读取 Player 的成员都登记为明确前置。

**实现步骤：** 核定构造期是否必须有实际 Player 或 Weapon 关联及其精确类型；用共同 Key 查当前 owner，不固定首玩家。安装 Source 的当前武器位置／阶段和资源，再准备派生 draw 所需挂载缓存。若普通 Isaac 前置不足，补 19 必需的窄接口；不能为绕过 20 而伪造装备或调用 FireKnife 重演玩法。

**失败与完成判据：** 缺失／已退休 owner、未经支持的装备依赖、返回阶段资源缺失分别报告。必要 Player 几何依赖可从正式值供应，且不宣称完整 Player 装备支持。挂载／伸出／返回变化无本地 charge、碰撞或伤害推进。

### C08：EntityPickup（I17，R07；E08）

**范围与输入：** 物品标识、价格／商店／选项组、循环内容、等待／触碰／开启／收集状态、隐藏表现、相关容器与专用变量；主 Sprite、价格 ANM2 及适用的辅助资源分别管理。

**实现步骤：** 按确定的 Type／Variant／SubType 构造，阻止 Init／Morph 重抽道具池、生成奖励或更新图鉴。Source 已确定的价格、循环物品等直接安装；根据具体 draw 消费补缺缓存，而不是调用购买或触碰路径。价格 Sprite 的 Load／Reset 和 Morph 同 this 资源重建与主 Sprite 分开处理。规范 pickup 关系／选项组，客户端不自行删除同组选项。

**失败与完成判据：** 不主动调用 LootList，即使参数声称恢复 RNG；真实抽取结果只由 Authority 记录。空循环集合、Null 引用、主／价格资源替换和删除均正确。宝箱／道具／资源 Pickup 的适用合同明确，无支付、拾取、奖励或本地重新开奖。

### C09：EntityFamiliar（I18，R06；E09）

**范围与输入：** 所属 Player、跟随／环绕／队形／链式／附着关系、距离／角度／计时／攻击状态、专用变量、形态和资源。所属参与者与实体身体分开，不能以 participant ordinal 替代 Entity Key。

**实现步骤：** 核定 Player owner、Familiar 专用链表及构造必要上下文。安装当前 orbit／follow 结果和 Source 位置，不调用跟随 AI、EvaluateItems 或生成 Familiar 的装备路径；队形／链式资源必要缓存独立安装。owner／形态变化按实际关系清理与重新绑定，不能临时挂到另一个玩家。

**失败与完成判据：** owner 或链依赖缺失保持等待／拒绝，删除不得连带移除目标中仍存在的同组 Familiar。各代表关系正确，后台无 Player 指针，Client 不开火／攻击；四人／多身体完整来源的后续实机覆盖归 20，引用结构本次不写死人数。

### C10：EntityEffect（I19／I24，R08；E10／E14）

**范围与输入：** 类型／子型、时间／寿命／状态／范围／计时、parent／target／owner、公共与专用变量、flags、地面／持续／透明／光照／粒子等表现。不能以“Effect”分类推断无玩法副作用。

**实现步骤：** 在 Effect57 基础上按实际 Init／draw／辅助 owner 分支建立适配族，覆盖实际使用的资源和结构；状态可同代码处理的分支共用，存在特殊容器／引用者单列。子对象若本身有 Entity Key 按目标构建，若属于原生内嵌资源按 owner 管理，避免两份生成。Effect 到期只随 Source 状态改变，不自行执行超时销毁／生成。

**Effect136 专项：** 复用 18 的 lifted 描述。`GetGridEntityDesc` 在官方 wrapper 中仅对 variant136 返回 `_varData` 起始的描述视图；需要 R08 确认完整大小／布局与 Init／draw／退休。目标保存完整描述及可空语义，不给此描述分配第二个活动 Grid 槽。Source 的 Unresolved 原 Grid 不补为虚构 Key，落地结果由后续完整 T 决定。

**失败与完成判据：** 不支持的专用 effect 分支必须具体列出；本 tx 类别需要的分支未做不能靠保留 Effect57 限制宣布完成。持续地面、透明／光照、子对象以及 Effect136 有正式来源和真实绘制／清理路径；不重复生成火焰、碎片、奖励或旧 Grid。

### C11：EntitySlot（I20，R07；E11／E12）

**范围与输入：** 主状态、DonationValue、PrizeType、PrizeCollectible、奖品 Sprite、Timeout、Touch、TriggerTimer、ShellGameAnimationIndex 及全部公共／专用适用成员。按现有 `domains/entity/slot_variants.lua` 的 1—18 表逐项确认，不把所有数值当统一含义。

| 变体 | 解释对象 | 必须明确的差异 |
| --- | --- | --- |
| 1 | SlotMachine | PrizeType 类别与无奖区间，主／奖励动画 |
| 2 | BloodDonation | 支付后状态与献血结果，不在 Client 扣心或改统计 |
| 3 | FortuneTelling | 牌／奖励／文字结果与表现，不能重抽 |
| 4 | Beggar | 硬币 DonationValue、Timeout 累积语义及清零 |
| 5 | DemonBeggar | 心量投入与计时状态，不能套硬币字段解释 |
| 6 | ShellGame | PickupVariant 奖品、Prizes 图层及选择动画索引 |
| 7 | KeyMaster | 钥匙交易状态和奖品关系 |
| 8 | DonationMachine | 捐款／卡币／损坏表现；持久总量绝不由 Client 写回 |
| 9 | BombBum | 奖励类别与爆炸相关 TriggerTimer，不能触发第二次爆炸结果 |
| 10 | RestockMachine | 重抽次数／爆炸标记与显示；不重新随机商店物品 |
| 11 | GreedDonation | 贪婪捐款状态及持久隔离 |
| 12 | DressingTable | 换装后显示及关联资源；不重跑 Player 装备／服装裁决 |
| 13 | BatteryBum | DonationValue 的既定随机增量结果与归零 |
| 14 | HomeClosetPlayer | 所用角色／表现与真实 Player 依赖，不创建第二个参与者 |
| 15 | HellGame | PickupVariant 与 PrizeCollectible 的联合解释 |
| 16 | CraneGame | 道具奖品、奖品 Sprite；Wiki记复合Timeout位，但本版本Getter/已读native访问仅16位，高位存储尚未定位，不按文档补造packed字段 |
| 17 | Confessional | 当前交互／奖励／表现结果，不裁决心量或随机效果 |
| 18 | RottenBeggar | DonationValue 的随机后态、奖励及关联物表现 |

**实现步骤：** 校对上述解释与 J273 wrapper／SDK，建立变体适用字段表和安全最终值写入；官方公开 setter 优先但有具体副作用疑问时核链。构造、主／奖励 ANM、状态改变、清空和破坏后的删除均纳入。奖品 Entity 通过独立 Key 与 Slot 状态关联，不在 Client `CreateDropsFromExplosion` 或 `RandomCoinJamAnim`。

**失败与完成判据：** 变体未知、奖品与资源冲突、捐款标量含义不明不能补猜值。18 种都具备适用状态合同，并各有真实来源与 Client 后态；Source 交互受实际持久授权约束，Client 永不捐款或写成就。尚无实际来源的行保持未完成，不用共享主 Sprite 代测。

### C12：NativeText（I21，R09；E13）

**范围与输入：** 10 的 J273 补充已核定 Text 类和 Type9001。I03 已补当前计数／寿命、当前use真实writer见证下至多256字节零结尾文本、KColor20和独立render phase来源；Init不初始化文本。既有读取依据不证明可安全构造／写入／draw，未匹配的phase和未知writer保持明确缺失。

**实现步骤：** 复用已定位 ctor／Init／Render 线索，补可调用 owner、字节串编码／终止和长度约束、字体／颜色／布局、当前 phase 的来源和安装。Text 可能不是通常的 ANM 绘制，必须使用真实类的表现路径。文本空串、变化、到期后移除分别处理，不让 Client 自己计时决定存在性。

**失败与完成判据：** 未终止／Truncated 文本、未知私有颜色和字体资源不能用猜测默认值弥补。完整文本状态与实际画面相符，确认退役不会进入普通 NPC／Effect 清理；专用颜色若仍无来源则该类别未完成。

### C13：NativeDummy 与明确未知类别（I22，R01／R09；E13／E22）

**范围与输入：** 10 已定位 Entity_Dummy 的 RTTI／ctor／Update／Render，正式补充输出 `NativeDummy`，不含猜测的派生字段。15 当前能力字符串的 `NativeDarkness` 不能直接当作同物别名。

**实现步骤：** 核对分类与能力名称、真实工厂／构造用途、成员归属、当前公共字段所能满足的 Render 和清理前置。若只有共同字段确已足够，则保留独立类型并复用公共适配；若发现专用状态，补正式来源和目录。测试来源需真实受支持的构造／场景，不能把 Effect／NPC 改 Type 伪造 Dummy。

I04已核定Dummy池预构造不等于通用Factory有对应选择分支：`Create(964,true)`默认进入NPC池，不能采用。I22须按专题取得真实Dummy owner；真实caller尚未解释时保持该具体缺口，不能改vptr绕过。

**失败与完成判据：** 实际未知类报告具体分类证据和缺失项，不能泛化为 Dummy。没有有效来源／未闭合 owner 时明确未完成，不把“暂未观察到”写成已支持。完成的 Dummy 应能独立建立／改变／退休且不会串入错误 vtable 或派生布局。

## 3. 类别工作落点与交付形式

建议在现有 `mod/src/game/client_objects.*` 保留协调与身份入口，把各类公共值解析、状态适配、原生生命周期和绘制职责分离；是否按每类或类别族拆文件由实际共享关系决定。`client_scene.*` 只协调依赖与唯一提交，不直接拥有全部派生字段。来源仍在 Entity 域，资源仍共用 17 的资源服务，Grid 仍由 18 的 owner 管理。

每类写回应用目录：已实现字段、正式源版本、调用／布局依据、构造前置、引用／资源 owner、失败前缀、具体覆盖和未决项。静态确认、代码完成、原生后态通过、画面通过分别记录；所有实际运行结论统一从 I34 写入，不为 C 编号另开测试或保存 artifact。
