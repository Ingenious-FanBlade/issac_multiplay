# tx-04-20：静态确认与必要逆向任务

日期：2026-11-04。当前状态：**R01—R16已形成采用／否决合同和具体未闭合分支**，见[静态研究及必要逆向](player-native-static.md)、[机器索引](player-native-contracts.json)和[主智能体独立复读](tx-04-20-foundation.md)。研究不表示全部角色／Weapon／私有draw／清理闭包已成立，未闭合调用不得采用，具体工作随对应adapter继续。没有构建、测试或启动游戏。

I16—I20的R06／R07／R08补证、主审修正及本轮采用范围见[装备／资源安装](tx-04-20-equipment-installation.md)；尤其type11实际复用Bone类，ClearReferences／Destroy helper不能当作无副作用退休，私有玩法缓存与当前draw／HUD输入分别处置。

初版规划事实保留：当时只读取已有项目材料、源码和本地Wiki来编写下列任务，未登记新增静态结论或运行结果。与[主实施计划](tx-04-20-implementation-plan.md)、[状态／角色／武器清单](tx-04-20-player-application.md)共同使用。

## 1. 方法、资料和统一产出

先查公开 Wiki，再查官方 wrapper／SDK／ZHL、已完成的 J273 研究；仅对尚未解释的具体缺口补静态反编译／指令／调用关系。公开语义明确的 Getter 不要求全部重做 native 对照；新增原生写入、构造／释放、draw 私有字段不能仅凭 Getter 名称推断。现行版本是官方 REPENTOGON 1.1.3 与 v1.9.7.12.J273，原型 J460 点位只作索引，不校验任何哈希。

资料入口：

- [原版 EntityPlayer](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)、[TemporaryEffects](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)、[ActiveItemDesc](F:/projects_isaac/IsaacDocs/docs/PlayerTypes_ActiveItemDesc.md)、[QueueItemData](F:/projects_isaac/IsaacDocs/docs/QueueItemData.md)。
- [REP EntityPlayer](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)、[PlayerManager](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/PlayerManager.md)、[Weapon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)、[CostumeSpriteDesc](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/CostumeSpriteDesc.md)、[History](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/History.md)、[PocketItem](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/PocketItem.md)。
- 官方源码根 `F:/projects_isaac/REPENTOGON-1.1.3/`：`repentogon/LuaInterfaces/Entities/LuaEntityPlayer.cpp`、`repentogon/LuaInterfaces/LuaPlayerManager.cpp`、`LuaCostumeSpriteDesc.cpp` 及对应 `libzhl/functions/*.zhl`；具体声明以实际文件为准。
- [09 四参与者实现／静态事实](../observation-expansion/four-participant-implementation.md)、[13 玩家来源及实际失败](../observation-expansion/tx-04-13-implementation.md)、[17 工作流程与原生研究](../tx-04-17/tx-04-17-workflow.md)、[19 采用合同](../tx-04-19/entity-application-contracts.md)、[19 最终验收与 Player 交接](../tx-04-19/tx-04-19-acceptance.md)。

每条实际采用合同应写明：对象／用途、公开成员或 native 入口、VA／RVA 的口径与 image base（仅地址资料）、ABI／寄存器／栈／返回、调用前置、读写／资源／引用、副作用、初始化及失败前缀、正常清理、线程／context／Lua 约束、目标版本来源、直接事实／静态推导／UNKNOWN、对应 I／P／C／W／E。不确定宽度、allocator 或 owner 时不能凭伪代码猜写。

结果作为功能设计和字段目录保存，不为每个 R 建 artifact 包。静态期间不运行 smoke／probe／harness／游戏或校验程序；运行后态、画面与异常覆盖统一归 I39。无需独立作者／强制子智能体，历史研究的角色安排不延续为当前要求。

## 2. 全部静态任务索引

| ID | 研究主题 | 当前可复用基础 | 必须补齐的主要缺口 | 实现落点 |
| --- | --- | --- | --- | --- |
| R01 | 目录、读取资格、scope 与投影 | 10／13正式来源、19 Player payload | 非活动／备份来源、被省略参数、必要私有表现 | I01—I05 |
| R02 | Player 构造、Init、发布与列表 owner | 17单Isaac、09原生Join | 多角色／多身体受控构造、自动生成及失败前缀 | I06—I09 |
| R03 | 身体关系、共享、备份与借用 | 09 topology、13 membership | 非roster真实所有权、用途就绪与递归关系 | I07—I10／I17 |
| R04 | 公共状态、心量、资源、stats | 19普通Isaac公共值、Wiki | 全心量／特殊规则／必要动作缓存的最终安装 | I11—I13 |
| R05 | 库存、饰品、主动／口袋槽 | 10参数来源与公开容器 | 最终容器 owner／allocator、槽副作用、完整有限范围 | I14—I15 |
| R06 | 临时效果、变身与History | 10／13规范来源 | 无玩法重算的安装／派生视图、共享与清理 | I16—I17 |
| R07 | Player真实资源与costume | 17资源寿命、19 BodySprite | 全部真实成员、容器／层、借用失效、动态重载 | I18—I19 |
| R08 | Weapon全类型与Entity依赖 | 公开Weapon、19 Knife／Familiar | 工厂／槽／动态类型／私有draw／连带退休 | I10／I20 |
| R09 | 持物、Suplex与矿车 | 19 Effect136和Player代画 | 其他持物类型、状态／资源、投掷结果归属 | I21 |
| R10 | Jacob／Esau、Forgotten两套语义 | 09／13实际多身体和切换 | Client受控重建／原位安装／资源及清理 | I23—I24 |
| R11 | Tainted Lazarus／Esau Jr | 09／13备份、切换、递归失败 | hologram／inactive／backup构造、读取和退休 | I25—I26 |
| R12 | 全角色专用状态族 | PlayerType及各公开专用成员 | 私有必要结果、各类型Init／draw差异及默认配置副作用 | I22／I27 |
| R13 | Player全pass绘制与必要维护 | 17—19分层draw、buffer／context | 全角色／武器／costume的读集合和临时写／副作用 | I29 |
| R14 | 输入关系、HUD与本地索引 | 09统一输入、16Client角色 | 必需原生绑定、HUD安全前置、本地与远端语义 | I30—I31 |
| R15 | 替换、删除、失败退休、在途资源 | 09递归清理、17—19退休服务 | 多身体／Weapon／costume／备份逐前缀唯一清理 | I28／I33 |
| R16 | 玩法、RNG、Lua、持久整链 | 17持久隔离、19用途分派 | 全新增调用链及其绕过既有分派的writer | I32／I34／I37 |

## 3. 每项具体研究计划

### R01：完整来源、非活动读取资格和发送投影

**现有依据：** `domains/player/collect.lua` 用同窗口 entity scope 引用身体本体，另采 History 和四备份槽；19 已保留 Player payload。P1—P6 当前省略多项 Player 参数查询。现行规范允许的是已满足当前正常 PlayerUpdate 见证的身体，不是所有可达指针。

**具体工作：**

1. 列出 Player 及关联类的全部成员／参数，逐项连接 P 族和真实 consumer；查 `gen_entity_catalog.py`／`gen_continuity_catalog.py` 分类及正式 reader 参数范围，不把默认参数值当完整槽枚举。
2. 跟踪公开 roster、twin／sub、Esau Jr backup 的来源枚举和 Entity readiness。确认非活动对象各字段是否有构造／Restore／复制完成的合法当前依据，哪里只能读关系而不能读本体；为实际缺口选择窄只读 owner 见证。
3. 检查 GetWeapon、active／pocket、costume、库存、TemporaryEffects、History 查询当前投影去向；决定恢复哪些具名查询、哪些用等价完整紧凑结构替代。
4. 对 draw 必需但公开未暴露的当前结果，只追具名 consumer 的读集合、实际 writer 和来源窗口；独立 render 值不得冒标 ordinary。

**产出与边界：** 字段／参数／来源资格／投影变化表，新增正式 schema 责任及读取失效条件。不能用测试桥读取或夹具补值冒充产品来源，不能把备份全对象可读作为未经证实的设计前提。

### R02：构造、Init、PlayerManager 和 Room 发布

**现有依据：** `client_objects_native.cpp` 目前只在空 roster 上分配普通 Isaac，构造后入 roster 再 Init，并通过 PreStats 隔离防止部分玩法。09 的 Join 是 Authority 的完整原生流程，不能直接作为 Client 初始化器。

**具体工作：**

1. 对普通单体、双体、sub-player、inactive、backup 查 allocator→ctor→必要字段→manager ownership→Init／Pre→官方扩展→Room 成员发布的真实顺序；确定每个阶段谁持有对象。
2. 核对每类 Init 的角色配置、起始装备、随机属性、自动子身体／Familiar／Weapon／HUD 创建、回调及持久副作用。明确哪些必要结构可复用，哪些玩法必须阻止或用最终结构替代。
3. 查 PlayerIndex／ControllerIndex／roster vector／专用列表的本地约束、容量及索引重新分配；不使用 Source ordinal 替代本地索引。
4. 分析两阶段构造是否成立；若某 twin／Weapon 需要前置 owner，列精确先决。构造失败、Init 返回／异常和未发布对象的前缀同步交 R15。

**产出与边界：** 按角色构造族的调用／发布表、自动生成对象归属和隔离点、结构就绪见证；不预先承诺存在一个可直接调用的“CreateReplicaPlayer”函数。不能靠清掉原生游戏对象后再裸写 vptr 建身体。

### R03：roster／非 roster 关系、共享和用途借用

**现有依据：** 09 已区分 primary／twin／sub／backup／replaced／EsauJr／inactive；13 输出 membership 和 control_relation。Entity19 当前外部 Player 必须位于公开 roster 的检查无法直接覆盖所有关系对象。

**具体工作：** 确认每条关系是 EntityPtr、裸借用、拥有型指针还是 manager 槽；定位 main／other twin、sub、替换、备份的真实双向维护和失效边界。查身体可见、可输入、可被 Entity 构造／draw 借用的不同条件，不让非活动对象获得模拟资格。

建立共享字段的实际依据：组归属不代表装备／血量都共享，Lazarus／Forgotten 特定共享也不能推广到所有 twin。检查列表／关系遍历的环和重复，明确 structure-ready、relations-ready、draw-ready 的最小证明。

**产出与边界：** 逐关系类型／方向／用途／owner／清理表，以及 `ClientEntityOwners` 所需接口改动。非 roster 接受条件必须更精确，不能只是取消 roster 检查；来源短暂缺席仍不等于对象销毁。

### R04：全部公共状态、心量、资源和最终属性

**现有依据：** 普通 Isaac 已安装有限 stats／heart／coin/key/bomb 与公共 Entity 值；这不能证明完整 Player 状态和特殊心量合法性。

**具体工作：** 按 P02—P05 列字段、真实宽度／符号／单位、所属 owner、可用 setter 与其内部动作。重点查黑／骨／金／腐／破碎心表示、心量排列与角色上限、魂／血充能、无敌／闪烁／飞行、头身方向与动作计数。

分开最终 stats、cache invalidation 标志和真实表现缓存；追 draw 直接读取而现有 parser 未安装的值。公开 setters 若隐含 Add／damage／重算／Lua，查到具体分支并改选最终量安装；语义清楚的 leaf 不强制全量反编译。

**产出与边界：** 逐状态族的安装与后态比较合同、角色条件和必需新来源；不要全对象 memcpy、不要使用普通 Isaac 的心量不变量拒绝合法特殊角色。P05 中只作 Authority 知识的派生量需明确无需原生安装的理由。

### R05：库存、饰品、主动／口袋／卡药槽

**现有依据：** Wiki 提供计数／槽／ActiveItemDesc／PocketItem／QueueItemData；现有观测与发送投影的参数覆盖不等价。

**具体工作：**

1. 核库存结构、固有物品分组、熔炼／金饰品表示、顺序和共享关系；确定完整枚举范围及有限配置 ID 来源，避免无限扫描参数空间。
2. 查槽数量、kind、嵌入／动态对象、charge 字段和 VarData；主动、口袋主动、卡药和 queued item 是否共享底层存储需有依据。
3. 查 Add／Remove／SetActiveItem 等候选路径是否触发 stats、costume、Familiar、随机、历史、平台或 Lua；不因名称是 Set 就采用。
4. 确定本地容器 allocator／析构／缩容／空集合，以及安装后哪些 native consumer 需要真实容器、哪些可用纯值只读视图；数据不能只放 sidecar 却让 native draw 继续读旧值。

**产出与边界：** 容器／槽安装表和禁止重演调用清单，依赖 R07／R08／R16。若缺无副作用 native setter，记录窄结构适配和必要逆向，不回退为循环“获得物品”。

### R06：TemporaryEffects、变身、History 和共享状态

**现有依据：** TemporaryEffects 有不同类型的效果查询；History 已由13在独立 player 域采集，库存不能代替它。

**具体工作：** 核各效果列表／计数／持续信息的真实完整性、容器寿命及共享关系，区分 effect object 和 item config；判断变身当前 flag／计数哪些被 draw 使用，哪些只作知识。查 AddEffect／ClearEffects 是否重算或触发资源／Lua，选择最终容器／纯值消费者合同。

History 保留顺序和字段，查实际 Player／HUD consumer 是否需要原生容器；如果仅提供27视图，明确不安装原生的范围和原因。源端 History 创建／读取是否懒写要按现有来源合同复核，不额外创建状态。

**产出与边界：** 效果／变身／History 的来源、安装／只读处置、共享和退休表。不能以 HasEffect 的若干布尔值替代全部计数列表，也不能用 History 重放取得道具。

### R07：全部 Player 资源、costume、动态重载和借用

**现有依据：** 17 研究了内嵌 ANM／Image／Lua/context 寿命；19 已对普通 Isaac 安装独立 BodySprite。19 主审已纠正 `Player+0x44C` 是 BodySprite 内动画状态，不是独立 HeldSprite，不能复活旧错误推断。

**具体工作：**

1. 从主／Body／Head／手／costume 的公开入口和真实 Render 读集合，枚举每个实际成员、层／track／null selector、嵌入还是独立资源；区分 getter 返回引用／指针／值副本。
2. 核 CostumeSpriteDesc、容器及配置引用的创建／复制／移动／析构、排序／可见性、换角色／换装／换图的失效；核准共享 Image 引用而非复制 control block。
3. 核完整 main／overlay／逐层 phase、颜色／shader、override、动态资源及表现工作缓存是否已由现有 ResourcePose 覆盖；缺什么补什么，不默认重新发送整张静态动画表。
4. 明确资源部分替换失败、在途 CPU／GPU、Lua wrapper 失效和 native draw 暂借的恢复时机。

**产出与边界：** 真实成员／别名／资源 owner 清单、构造和清理顺序、新增正式来源及姿态字段。主 Sprite 可绘制不证明完整 Player 外观；对无法解释的内嵌资源不任意复制字节或调用 Reload 全链。

### R08：Weapon 的全类型生命周期与跨 Entity 依赖

**现有依据：** REP Weapon 公开 owner／main entity／charge／delay／direction／modifiers 等；19 已处理 Knife／Familiar 的部分关联和清理，但完整 Player Weapon 留给20。

**具体工作：**

1. 核 `GetWeapon` 的合法参数、真实槽集合和动态类型，W01—W15 是否共享基类／拥有私有子结构；创建／替换／销毁的 owner 和 allocator 分别定位。
2. 对每类追构造／Init／draw／retire 对 Player、Entity、Sprite／几何、item animation、充能和方向的读写；确认合法 Null main entity 和必需前置，不假设每类都有固定攻击 Entity。
3. 标记构造和清理会生成／移除的 Entity、写回 Player 的槽及官方扩展引用；同 T 的独立 Entity Key 必须由唯一 owner 构造。
4. 核私有表现字段是否能用公开最终值覆盖；缺失时补 Authority 真实只读来源。不能用 Fire／Shoot／Update 得到缓存，不用 fake weapon pointer 满足19检查。

**产出与边界：** W01—W15 分类型工厂／状态／资源／依赖／退休合同，结构就绪与最终就绪阶段，能力及来源差额。未理解某类型动态布局时该类型未完成，不回退为 Tears。

### R09：held Entity、Suplex、投掷结果和矿车

**现有依据：** [19 验收的持物研究](../tx-04-19/tx-04-19-acceptance.md) 已记录 Player Render、held.Visible=false 代画、Body null-frame、ItemState604 的136路径，以及 SuplexState 真实0—5状态。地址和偏移只复用该已有合同，不算本轮新增发现。

**具体工作：** 核 held 不同动态类型的允许分支、必要 Parent／owner、实际姿态／资源、null-frame selector 和层次；区分 Source TryHold／Throw 的玩法链与 Client 纯显示安装。投掷后的新 Tear／Effect 与旧 held 删除按目标身份表达，不沿旧指针追继承。

核 minecart 的 Player offset／sort 位模式、NPC／Familiar 复合 draw 借用和外层调用顺序；Body、held Sprite 和可见性的临时写恢复保持到真实 CPU 队列消费之后。必要时追实际 caller 补类型检查和生命窗口。

**产出与边界：** 136窄实现向完整 Player owner 的迁移表、其他持物族采用合同及真来源缺口。不能仅把所有 held 强制可见或直接调用 Throw／Suplex 完成状态变化。

### R10：Jacob／Esau 与 Forgotten 两套关系

**现有依据：** 09 已有四组八身体运行事实；普通 Forgotten 16／17 交换同两个对象的状态／类型，35／40 是 Tainted 组合。原生 input／Join 事实可复用，但 Client 受控安装尚未完成。

**具体工作：** 对 C06—C08 查所有必需身体的构造 owner、roster／sub 成员、共享／独立资源和最终 active／visible 状态；追原生切换时修改了哪些身体／资源／引用，选择直接最终量、窄重配还是同 Key 实例替换。

核骨棒、携带／落地、魂和骸骨心量、角色类型交换与借用失效，避免调用完整 SwapForgottenForm／投掷来重演。核一组退出对其他组的引用及清理影响，处理多个组同时存在的列表与索引。

**产出与边界：** 三族具体顺序、角色／身体状态表、构造环和退休责任；09 输入侧 max64 关系预算不当成无限原生创建能力。控制原生不允许的动作不应被补做。

### R11：Tainted Lazarus／Birthright 与 Esau Jr

**现有依据：** 09／13 已覆盖29／38、Birthright／Flip和真实Esau Jr两次交换；09 曾在Lazarus退休中遇到递归重复清理，且Esau Jr退出不证明全局四槽回收。13 最终以真正控制意图触发交换，而非 `UseActiveItem` 返回值认定成功。

**具体工作：**

1. 查 active／inactive／hologram 身体的真实构造、roster关系、共享与独立库存、资源、允许借用及字段可读依据；Flip 是互换状态还是更换实例应分分支记录。
2. 复用703主动槽一基备份索引的旧静态事实，确认本次目标布局和当前 reader 的来源；槽号不是ordinal，四个槽不是四个活动玩家。
3. 追备份完成／恢复／退休 owner，解释未更新备份字段的合法读取时点；需要新来源见证反馈R01，不擅自扩大readiness3。
4. 分开Authority保留的原生槽和Client镜像owner，确定新建、切换、引用解绑、部分失败及run teardown的唯一清理路径。

**产出与边界：** 两族的状态／资源／成员转换和正常退休表，备份来源与容量限制、仍UNKNOWN的异常分支。不能为“释放槽”盲清原生指针，也不能把未知永久保留当成已解决正常清理。

### R12：全部角色类型与专用结果

**现有依据：** 本地PlayerType列0—40和-1内部possessor；公开接口有多项角色专用容器／查询。20需要按C01—C17分族，不是只给EntityPlayer换一个type数字。

**具体工作：** 为全部类型建立构造／状态／资源／draw差异表；重点核Tainted Isaac库存次序、Cain crafting、Blue Baby poop队列、Eve clot、Samson狂暴、Lilith胎儿／鞭、Apollyon locust、Bethany魂血充能／wisp、Jacob／Dark Esau及普通特殊心量／复活形态。

查每族真实必要私有数据和消费路径；依赖对象准确归Player、Weapon或Entity19。Eden／变身／复活／重掷的源结果不可由Client重随机。内部Baby／possessor若是必要目标，补合法结构；原版Online独有路径保持隔离。

**产出与边界：** 全类型到应用族的映射、每族必要源和构造／draw／退休差额；一条未知必需分支不能用普通Isaac代替。组合内容交29，但本体基本族留在20完成。

### R13：所有 Player pass、缓存与外层 caller

**现有依据：** 18／19已处理FBO／depth、水面／镜像和跨实体shadow buffer；19曾实际因孤立RenderShadows缺少外层前置而AV，不能再次把虚调用正常签名当充分条件。

**具体工作：**

1. 从Player Render／Body／Head、手／costume／weapon、shadow／light／post和水面间接路径，建立pass→caller前置→真实字段／资源→临时写→队列消费表。
2. 查角色／特殊动作／死亡／ghost／闪烁／飞行、持物、矿车、Familiar／NPC调用Player表现的分支，避免直接／间接重复绘制或遗漏层。
3. 识别使用本地时钟／全局RNG／Update／动画事件／Lua callback的分支，决定供给Authority最终表现输入、允许已解释的纯表现维护或窄隔离；不通过全Player Update恢复外观。
4. 复核稳定缓存的Key／shape／instance／resource失效，以及临时覆盖直到CPU／GPU消费结束的实际边界。

**产出与边界：** 逐角色／武器族×pass采用表、缺失正式值／缓存、用途规则和清理需求。静态调用合同不证明像素正确，全部画面比较归I39。

### R14：输入／索引和HUD安全依赖

**现有依据：** 09已经统一canonical输入，Client角色与本地输入分离；19普通Player采用无设备控制的本地索引，完整四人／多身体方案仍需核定。

**具体工作：** 核所有Player原生draw／资源／HUD helper对PlayerIndex、ControllerIndex、主玩家／本地玩家的读取，明确哪些必须有合法本地映射、哪些保留Authority事实。不能调用原版Online IsLocalPlayer解决C/S归属。

核PlayerHUD或其他UI owner的构造／绑定／退出是否是安全Player承载的必需部分；列20最小结构责任与27完整表现责任。审阅16当前Client输入会话、绑定代次和状态发布，明确控制组换根／形态变化对输入的真实影响。

**产出与边界：** 本地索引与participant映射、HUD安全前置、只读数据接口及输入失效条件；不把身体数压到设备数，不让Client运行第二份input consumer，不把纯换图当新连接。

### R15：换形、删除、部分失败和在途退休

**现有依据：** 17—19有Scene retained／borrow与部分失败退休；09清理按条件递归twin／sub／备份，曾出现重复移除崩溃。正常完整Player清理不代表半构造安全。

**具体工作：**

1. 对raw、ctor-entered、已构造未入manager、已入roster、Init部分完成、已发布、已绑定关系、部分costume／weapon、draw在途列前缀。
2. 查PlayerManager、Room、twin/sub/backup、Weapon、EntityPlus、官方扩展、Lua userdata及History／costume资源的实际释放职责与跨对象写入；避免两个owner递归释放同一身体。
3. 明确remove、same-Key replace、资源重配和整场景重建的差别；所有反向关系、缓存／borrow在什么时点失效，哪些owner必须等CPU／GPU／Lua退尽。
4. 正常返回、分配失败、C++异常、SEH、VM/context失效分别说明能否恢复；未知保持隔离和真实owner记录，不盲目析构／重注入／交还Off。

**产出与边界：** 逐前缀→清理owner→后态→可恢复条件表、故障注入位置和I33实现顺序。不改自有virtual device NEVER策略；不得将所有Player都永久泄漏作为默认正常完成方案。

### R16：全新调用链的玩法、Lua、随机和持久副作用

**现有依据：** 17定义保护覆盖Client准备至安全退休及最后相关持久writer；19已隔离采用类别路径和Client全窗口SFX，但Player装备／角色／Weapon新增链不在既有证明范围内。

**具体工作：** 对每条采用构造、setter、重配、draw、retire查：spawn／remove、伤害／拾取／支付、属性重算、动画事件、全局／局部RNG、官方必要结构维护、gameplay Lua、音效／独立表现、PGD／成就／统计／解锁／GameState／Mod保存及正常退出writer。

每条路径决定允许必要结构、安装最终量、阻止重复裁决或明确不可采用；查普通Update之外的直接调用、嵌套purpose和错误线程。沿唯一Hook清单登记入口和用途，保持常驻和Host／Off透传；原生必要管理维护不能被一刀切屏蔽导致资源泄漏。

**产出与边界：** 采用／替代／跳过调用表、完整保护窗口、Hook与故障结果的明确责任。不能用最终dirty=0或文件未变替代全窗口理解；不能为验证隔离进入原版Online或Isaac Sync。

## 4. 工作顺序、复用与未决处理

R01—R03先确定目标和owner；R04—R09支撑状态／资源／武器，R10—R12补角色族，R13收束完整绘制，R14对接输入／HUD。R15／R16从选择第一条构造或setter时同步进行，不留到实现之后才追清理和持久写入。公开源码已经解释的内容只记静态确认，必要native缺口才逆向，不按16项机械要求16次反编译。

下列问题目前没有足够依据可在计划中承诺具体实现细节：

| 当前技术未知 | 负责项 | 需要形成的实际结论 |
| --- | --- | --- |
| 多角色Init能否分离必要子结构和起始玩法生成 | R02／R12／R16 | 具名可调用顺序与隔离点；必要时自有编排，不能放行整Init全链 |
| 非活动／备份是否具有完整合法字段来源 | R01／R03／R11 | 按字段和owner的当前读取依据，补正式见证或明确未完成 |
| 各装备／效果容器可安全直接安装的范围 | R05／R06 | allocator／布局／派生consumer及最终量映射 |
| Weapon类型／槽／私有结构与连带Entity所有权 | R08 | W01—W15独立合同，不能拿一个通用假结构覆盖 |
| 多身体／costume／Weapon的实际在途资源与清理闭合 | R07／R15 | 前缀、唯一释放owner、上下文和正常恢复条件 |
| Player draw需要哪些未发送的表现字段 | R01／R07／R13 | 精确消费集合和正式来源／投影，不依赖调试录制 |
| Player完整应用怎样升级Scene域位而不误报全世界 | R01／R14，I34 | 逐域／版本／attempt的明确完成谓词和后续ACK责任 |

这些问题先由静态研究解决，不要求用户猜测原生实现；若结论需要改变产品范围或实际操作授权，再提出具体可审阅的选择。研究后仍UNKNOWN的必需项保留为本tx未完成工作，不能靠拒绝全部相关原版目标完成本tx。
