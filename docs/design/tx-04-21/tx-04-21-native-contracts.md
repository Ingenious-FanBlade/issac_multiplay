# tx-04-21 I02：实时状态采用合同与定点静态研究

日期：2026-11-05。目标版本：Repentance+ v1.9.7.12.J273，官方 REPENTOGON 1.1.3。状态：**R01—R09 静态合同归并完成；R02 的 J273 odd 入口已定点复核；本文件不表示 I03—I28 已实现，也不表示任何运行验收已通过。**

本文件是 I02 对 R01—R09 的采用结果，配合[研究计划](tx-04-21-native-research-plan.md)、[实施计划](tx-04-21-implementation-plan.md)和[网络合同](tx-04-21-network-contract.md)。地址口径：IDA 目标映像 imagebase 为 0x400000；下文 VA 与 RVA 同列，RVA=VA−0x400000。只读静态研究，不执行哈希，不修改 IDA 名称、注释、类型或字节；未运行构建、测试、游戏、probe 或 harness。

## 1. 证据标签与本轮边界

| 标签 | 含义 |
| --- | --- |
| **复用事实** | tx-04-17／18／19／20 已记录并可沿引用复核的 J273、官方源码或隔离合同；不因本文件重述而升级成运行保证。 |
| **本次源码确认** | 本次只读复核当前仓库源码及其可见调用／数据流。 |
| **本次定点逆向** | 本次只读 IDA 的 J273 函数、调用点、指令和寄存器／栈使用。 |
| **静态推导** | 从以上事实给未来 I 项采用的约束；属于设计合同，不是现成实现。 |
| **UNKNOWN** | 精确指出尚未闭合的对象、字段、caller、失败阶段或行为及其阻塞范围；不得以运行测试代替写前合同。 |

本轮优先读本地原版 Wiki 与 REPENTOGON Wiki，再读 ZHL／官方 wrapper、项目既有研究及生产源码，最后只对 PlayerPost / odd 分支缺口读 IDA。原版 Wiki 的 MC_POST_PLAYER_UPDATE 页面记录回调每秒 60 次；Game:GetFrameCount 页面记录游戏帧每秒 30 次。这两条公开事实都不证明某个指定 Player body 每秒进入原生 Player::Update 60 次。目标资料分别见 F:/projects_isaac/IsaacDocs/docs/enums/ModCallbacks.md:691–702、F:/projects_isaac/IsaacDocs/docs/Game.md:250–259；REP 的 MC_PRE_PLAYER_UPDATE 合同见 F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/enums/ModCallbacks.md:2721–2728。

未来 I08/I23 所采用的逐身体快照／快速安装仍是待实现合同。当前 PlayerPost 只是观测 phase 名称；当前世界采集不读取完整的逐身体高频值，当前 Client 也没有 PlayerFast 写入器。**不要**把现有 native-reached / activity witness、processed input ACK 或奇数帧 tick 说成“此身体已经完成 60 Hz 快速同步”。

## 2. R01：发送字段与真实消费者的采用范围

**对应：I01、I03、I05–I07；E02、E05、E09、E18。证据：复用事实 + 本次源码确认 + 静态推导。**

### 2.1 归属合同

| 状态范围 | 可采用来源及消费者 | 更新单位／资格 | 所有权与空值规则 |
| --- | --- | --- | --- |
| Baseline | 18–20 的完整 Room/Grid/Entity/Player 目标及冷端创建依赖；Client 实际安装链为 ClientScene。 | 新 scope 首次准备或同连接完整恢复；必须包含应用所需的全部 Context、Structure、Resource 和完整成员。 | 只是各范围的重建组合，不得因拆频率而删掉现有 draw / HUD / native adapter 需要的成员。缺失、Null、Empty、NotApplicable、Unavailable、Unsupported 必须各自保留。 |
| Context | Floor/Level、descriptor、room_save、探索、run 等低频但可能变更的上下文。 | 仅在具名 dirty／generation／source loss 后撤销，或在下一次有界完整 ordinary 窗口重新验证；没有写者闭包的字段不得仅凭“通常不变”无限续期。 | 以来源版本引用，不从未声明的“当前对象”继承字段；不同 run／room／scope 不拼接。 |
| Structure / Resource | Entity/Grid/Room/Player 的 Key/use/shape/class、完整成员、控制关系、装备、Weapon 类型、costume 结构、资源文件与实例前置值。 | 创建、删除、Morph、shape/角色/类型改变、roster/slot/parent/use 变化、资源 reload 或来源版本改变时走完整结构／资源事务。 | 稳定 Key 与 use/shape 是身份条件；同名文件不等同同一原生资源实例。只有完整成员范围中的确实缺席可退休对象。 |
| WorldDynamic | Room/Grid/Entity 的当房最终动态状态和完整成员关系。 | 只由完整 ordinary Game→Room→EntityList／标准尾部及 Manager 外层正常提交产生；设计目标 30 Hz，但只计新的合格 ordinary 样本。odd gap 不补普通帧。 | Player 公共 Entity 动态字段在线路上归 PlayerFast；WorldDynamic 排除 Player 记录。原生安装继续复用 Entity 公共字段 owner，Player 专属 backing/关系由 Player owner 负责，不允许两路快照覆盖同一字段。 |
| PlayerFast | 只覆盖 I01 明确映射到真实 Player draw / 状态消费者、并满足 R02 逐身体资格的有限动态组。候选包括 P02 的位置／速度／偏移／颜色／flags 等公开最终值，P05 当前属性／方向／动作值，P11 主／overlay／layer pose，P12 同结构 costume layer pose，P13 当前 Weapon 动态标量；具体字段须逐成员连到来源与 writer。 | 目标上限 60 Hz；每个身体以新的、实际正常返回来源发布一次。实际仅随普通更新的身体可约 30 Hz 或更低，不能重复旧值填成 60 Hz。 | 每个字段保留真实 body Key/use/shape、binding revision、关系版本和 source sequence；不重复 Player Entity 公共字段，不把 participant、control group、body、backup slot 互换。 |
| AuthorityOnly / 后续表现 | 权威操作、随机结果、玩法／回调调用、短命声音／文本／独立表现仍归 Authority 或 tx-04-27。 | 由 Authority 事件或已声明的独立表现 scope 驱动。 | 不从画面状态推导事件，不在 Client 补 Fire／Update／EvaluateItems，也不因状态 Applied ACK 推进未播放事件水位。 |

### 2.2 目录与消费者定位

**复用事实：** Player 真实状态族和消费边界以 docs/design/tx-04-20/tx-04-20-player-application.md 的 P01–P20 为目录，不以协议目前实际投影的字段数量代替消费者清单。特别是 P02 公共空间／最终状态 (lines 29–35)、P05 属性与当前动作 (53–59)、P11 ANM 主/overlay/layer pose (101–107)、P12 costume 描述与层次 (109–115)、P13 Weapon 槽及主实体 (117–123)、P15 关系清理 (133–139)、P19 生命周期 (165–171)。全部 Player 原生目标目录引用 docs/design/tx-04-20/player-native-static.md:21–28、docs/design/tx-04-20/tx-04-20-equipment-installation.md:9–17。Room/Grid、Entity、HUD 的消费者责任分别沿 tx-04-18、tx-04-19、tx-04-27 的目录走，不把本次 R01 误当它们的全部研究。

**本次源码确认：** ClientScene 当前按主线程 Manager boundary 执行；ClientScene::Pump 在 Manager owner、健康 context 和非阻断窗口工作，并在正常准备／应用路径保留原生与 Graphics 借用；当前每轮最多一个对象命令，场景最终在同一次完整 target 检查后提交 (mod/src/game/client_scene.cpp:517–659)。Player parse/preflight 位于原生写入前 (mod/src/game/client_players.cpp:81–95)。

**静态推导：** I01 应从以上 P01–P20 和 18/19 目录逐成员回链到当前 getter、world source、应用器、draw/HUD consumer、结构／资源 owner，而不是复制本文件的范围表后宣布完整。每个同一值只指定一个 writer；每项记录合法空值、参数／单位／精度、更新阶段、依赖版本、失效源、失败状态和 I/E。低频缓存若来源 writer 或失效入口未闭合，具体成员只能走有界刷新或留下该成员 gap，不能外推整个 Context 静态。

**本轮 I01 回填：** 逐签名来源、18—20 逐成员应用器链接、投影复核与原生结构补源已进入 [stream-catalog.json](stream-catalog.json)，由 gen_stream_catalog.py 生成并嵌入产品。目录保守保留当前消费者；没有完整 dirty 覆盖的值每个合格普通窗口刷新。**UNKNOWN（精确范围）：** I08/I23 新增窄读取与快写器尚未注册，不能将目录里候选分层当成已具备逐身体采集／应用资格；未闭合的具体字段仍需在实际采用前补证。

## 3. R02：ordinary / odd 逐身体 PlayerPost 来源

**对应：I08、I18、I23；E03、E04、E08、E18。证据：复用事实 + 本次源码确认 + 本次定点逆向。**

### 3.1 J273 ordinary 与 odd 调用边界

| 路径 | 目标版本事实 | 采用边界 |
| --- | --- | --- |
| ordinary Manager → Game → Room → EntityList | **复用事实：** Manager case 2 的 ordinary 分支调用 Game::Update；Game::Update 进入 Room::Update 和 EntityList::Update；Room EntityList 调用点 VA 0x7F93A7 / RVA 0x3F93A7，push 0；动态 Entity vtable slot +0x0C 进入每个真实成员的 Update。完整 ordinary world 来源还需 Room、EntityList、标准 PostUpdate 尾部、Manager 正常返回及 Clock commit，见 docs/research/tx-04-08-st2-static-confirmation.md:81–87、docs/design/tx-04-17/tx-04-17-update-isolation.md:92–104。 | 对实际进入 Player derived Update 并正常返回的每个同身份 body 记录独立候选。ordinary 的一份完整 WorldDynamic 仍只由其普通外层提交生成；不能从某 Player 返回直接发布整房范围。 |
| odd helper | **本次定点逆向：** J273 Game odd helper VA 0x6F9340 / RVA 0x2F9340，函数 IDA 显示 char(void)、无显式参数；它从 Game singleton VA 0xBE49F4 取上下文。Manager caller VA 0x92E921 调用它。helper 在正常 Player 循环前有 pause／transition 等早退，因此“进入 helper”不是 Authority tick 见证。PlayerManager 来自 Game+0x18300；指针数组基址为 manager+0x125C、元素计数为 manager+0x1264，按 4-byte pointer 项迭代（主审纠正：+0x1264 是计数，不是 STL end 指针），body 的 [p+0x28] 必须为 1 才进入普通循环。 | 只接受实际循环访问且满足类型条件的具体 p，不从“在 Player topology/backup 中”推导已更新。每个 p 独立匹配现有 Key/use/shape/binding generation。 |
| odd ordinary Player call | **本次定点逆向：** 普通 odd Player 虚调用 VA 0x6F9559 / RVA 0x2F9559，bytes FF 52 0C；ECX=p，调用 Player vtable +0x0C，显式栈参数为 0。它后续 VA 0x6F9560 / RVA 0x2F9560 调 sub_6A6A40 / RVA 0x2A6A40，ECX=p，push 0，正常返回点 VA 0x6F9565。前后临时状态含 [p+0x168] 的 0x4000 bit。函数仍要正常返回。 | 0x2F9559 是普通 odd Player::Update 候选标记点，不是 odd helper 的整体提交点。被 Update 改动且之后碰撞 helper 还能改动的字段不可在 0x2F9559 处盲目冻结；要在字段 writer 闭包内确定读取点，并等 odd helper、随后 outer Manager 正常提交后才对外发布。 |
| odd 特殊 Player 调用 | **本次定点逆向 + 复用事实：** VA 0x6F93F9 / RVA 0x2F93F9 和 VA 0x6F949C / RVA 0x2F949C 是 Game+0x1B83C 与 Game+0x1BA78 特殊状态分支里的 Player vtable +0x0C，不是普通 odd simulation。docs/hooks/repentogon-binding.json:21 也将三个点分开并只承认 0x2F9559。 | 两个特殊 call 均不取得普通 PlayerFast 资格，不推进 Authority 模拟 tick。不能因 receiver 是 Player 就给它们贴 PlayerPost。 |
| helper 额外对象向量 | **本次定点逆向：** helper 后段遍历 Game+0x1BB78..+0x1BB7C 的另一 4-byte pointer vector，在 VA 0x6F96CD / RVA 0x2F96CD 调对象 vtable +0x0C。没有本体类型检查。复用 docs/research/tx-03-10-replica-update-presentation-boundary.md:63 和 tx-03-11-luna-render-findings.md:68 已判集合对象类别／所有权未闭合。 | 不把该向量命名成 Player roster，也不作为 PlayerPost 来源。 |

**副作用：** sub_6A6A40 不是姿态 helper。复用 docs/design/tx-04-17/tx-04-17-update-isolation.md:84：它被 EntityList 与 odd helper 调用；函数体可见 Player/Grid collision、HP 更新和 Grid factory/槽位写入路径。主审对本次 odd callsite 后续指令及该 helper 再独立复读：碰撞 helper 能写 Entity Position `[p+0x33C/+0x340]`、Velocity `[p+0x360/+0x364]` 等并触发 Grid 响应；helper 的原生返回之后 odd loop 还在 VA 0x6F9565–0x6F9575 清 Entity flags 低 DWORD `[p+0x168]` 的 `0x4000` 临时位。故不得在 0x6F9559 的 PlayerUpdate 返回、也不得在碰撞 helper 返回的瞬间复制所有公共位置／速度／flags。只有字段 writer 闭包确认后才能选读点；广义公共动态组最早等该清位代码与 outer Manager/Clock 正常收束后作为 owned candidate 取值并提交。Client 不因收集而再次调用碰撞 helper。

### 3.2 ABI 与 PlayerPost 资格合同

| 项 | 证据与合同 |
| --- | --- |
| 公开/SDK vtable 含义 | REP 官方 ZHL F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntityPlayer.zhl:667–678 声明 Player 的 Update override；Entity.zhl:254–264 的虚槽序列为 Free +0、Init +4、PreUpdate +8、Update +0x0C。它给出虚槽语义，不单独证明某个调用点会运行。 |
| odd native ABI | 0x2F9559 的真实指令使用 ECX receiver、无显式参数，保存原生 vtable 分派 exactly-once。官方 wrapper 的 Player::Update 是 `__thiscall` 虚方法。target-specific 调用后续 collision helper 使用 ECX=p 与一个 32-bit stack literal 0；函数返回值不作为 PlayerFast 成功码。 |
| 官方 callback short-circuit | REP 官方源码 F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/CustomCallbacks.cpp:521–534：MC_PRE_PLAYER_UPDATE 为 true 时跳过 Player derived super，改调 Entity::Original_Update。故只有确证已到 Player derived Update 的 native marker 才证明该 body 真走所选 derived 路径；仅外层 PlayerUpdate wrapper 回调进入／退出不够。 |
| 每身体授权 | **静态推导，I08 采用：** marker 与 body 指针完全对应；marker 前保存的 actual session、稳定 Entity Key/use/shape、world scope、binding revision 与关系版本，在 candidate commit 前仍相等；该 body 的 derived Update 确已进入、没有 callback skip，且正常返回；当前未 pause/menu/console/focus-block/lifecycle/transition、无对象 mutation/source loss/consumer detach。身体资格不从同组主身体、其它副身体、backup、roster slot、same tick 或相同地址继承。 |
| 可读字段 | 单次 PlayerUpdate normal return 只证明所选 update code 正常退出，不证明 Player 的每个公开／私有字段、Weapon、costume、ANM、HUD 或派生 draw cache 都已完整写好。每项必须有字段级来源与 consumer 闭包；只有 20 已核定最终状态的确定成员可以进入 I01 候选。不能调用 Update、EvaluateItems、Fire、动画推进或碰撞 helper 补齐。 |
| 正常提交与取消 | **静态推导：** PlayerUpdate return 先生成有界 owned candidate，不发布或提升 World revision。ordinary candidate 必须在其完整普通 tail 与 Manager／Clock 正常 commit 后，且同 binding/use/shape 仍有效时才发布；odd candidate 至少等普通 odd helper 正常返回、其后的副作用收束且外层 Manager／Clock 正常 commit。外层异常、Clock 拒绝、scope/generation/关系变化、source mutation/loss、detach、stop 或候选超预算均撤销候选。Failed prefix 要带实际 failure reason；未 Native commit 不发 Applied ACK。 |
| 实际被跳过的 body | 记录 `Skipped` 的真实原因：callback short-circuit / type 条件不满足 / 没在相应 vector / special branch / pause 或外层早退 / binding invalid / exception / source gap。不得对它发布旧 pose 作为新样本。 |

### 3.3 当前实现与确切未知

**本次源码确认：** mod/src/game/consumers.cpp:237–333 的 PlayerScope 只在 controlled ordinal 的现有模拟范围内记录 Player anchor；PlayerNativeReached 提供到达事实，正常析构时对同一 binding/use 记录 world update witness。mod/src/game/world_bridge.cpp:426–441 把该 witness 映射到当前 tracked group body；mod/src/world/collector.cpp:466–493 只在 normal/reached/tick/outer/anchor 一致时标 current witness，不复制 pose。mod/src/game/tick.cpp:502–515 仅在 odd normal marker 与当前 Authority controlled pair 对上时开始唯一 OddPlayer tick；mod/src/game/tick.cpp:356–389 仅在 Manager 正常返回时走 post capture、Clock finish 与 publish。odd helper 的世界采集目前显式得到 ODD_PLAYER_ONLY gap，见 docs/research/tx-04-08-st2-static-confirmation.md:81–87、mod/src/world/collector.cpp:793–842。

**本次源码确认：** Phase::PlayerPost 枚举已在 mod/src/world/common/contracts.h:11，但 Player 和 Entity 的 SupportedPhases 不包含 bit 1 (contracts.h:13–15)；Service 验证会拒绝未开放 phase，service.cpp:5–12、63–71。当前正式采集没有完整 PlayerPost consumer。I08 必须同时新增适当 source/phase consumer、body identity/versioned candidate、正常 outer commit 与失败撤销；不能只将 enum 开放。

**UNKNOWN（阻塞范围）：** R02 的 IDA 证据确认所列 Player call 与后续 helper，但没有证明所有 ordinary EntityList 的实际 body 集合如何覆盖 Player topology 的每一类 main/sub/非 roster/backup，也没有字段级确认 collision helper、Room 后处理、Mod callback 对全部 PlayerFast 值的写入集合。I08 对这些未映射成员和未到达身体必须输出明确 gap；I01 给字段级 reader/writer 目录后才能扩字段。它们不阻塞采用“每个实际正常更新 body 单独见证、无继承”的合同，也不要求额外 native 调用点猜测。

## 4. R03：快速姿态、Weapon、costume、层状态与资源不变性

**对应：I09、I23、I24；E05、E07、E09、E22。证据：复用事实 + 本次源码确认 + 静态推导；无新构造器逆向。**

### 4.1 字段组采用

| 字段组 | 可作为快速动态候选的部分 | 结构／资源事务触发条件 | 证据／限制 |
| --- | --- | --- | --- |
| Player public Entity 最终状态 | P02 已来源核定、Client 实际消费者需要且不要求玩法模拟的具体最终数值；公有 Entity draw 与 Player draw 所见同一字段只由 Entity owner 更新。 | Key/use/shape/class、Room membership、parent/twin/held/link、EntityList/roster 归属变更；碰撞或插值临时值不能当最终逻辑位置。 | docs/design/tx-04-20/tx-04-20-player-application.md:29–35；tx-04-17-native-maintenance-rendering.md:116–120。Player::Interpolate 和 slot +0x10 会写临时位置／进入碰撞，不被用作客户端姿态补帧。 |
| 当前最终量与动作 | P05 当前方向、属性、公开动作／charge/cooldown 等已闭合最终 backing 值；不含需 EvaluateItems/Cache 更新或事件重放的玩法值。 | 角色、库存／饰品、TemporaryEffects、variant、Weapon type、角色计数、结构化动作关系变化；改动需完整 Player 状态事务。 | P05 lines 53–59；tx-04-20-equipment-installation.md:9–13 明确效果、History、装备数据采用“直接装最终值，不跑玩法链”。具体字段仍逐成员由 I01 确认。 |
| 主／overlay／layer pose | 同一已装载原生 ANM 实例的精确主动画／帧相位、overlay、播放状态、shader 与每层 selector/null-frame、变换／可见性等当前姿态。Player 当前 13 份 ANM 作为实际成员逐一保留，不按同文件折叠。 | 动画/层数量、ANM资源或 texture replacement、selector 槽形状、角色/Item resource owner、引用的 layer/ANM 类型改变；改配置必须重构 candidate。 | REP ZHL / docs/design/tx-04-20/player-native-static.md:21–28 记录 Player size 0x2EA0、Player+0x41C 起 13 个内嵌 ANM、stride 0x114；equipment-installation.md:11 有 pose 子状态清单。构造只证明成员存在，不证明已 Load。 |
| costume 描述与 layer map | 结构完全相同的 ordered CostumeSpriteDesc / 15 项 layer map 上的真实 layer pose 更新。 | Costume 数量/顺序/身份、配置/filename、priority/skin/flying/overlay/item-animation metadata、15 项 map 变化或 costume resource generation 改变。 | Player+0x1220 desc vector、+0x122C 15 项 map，size/layout见 player-native-static.md:21–28。client_player_equipment_native.cpp:50–63、100–116 同结构才更新当前姿态；不同结构先完整构造 candidate 再交换，失败保留实际已写前缀。 |
| 五个 Weapon slot | 只更新已存在同动态 type/slot generation 的当前公开最终动态值：charge、delay、方向、modifiers、num fired、两份 chargebar 输入，并读取真实 getter后态。 | slot 是否存在、dynamic type/shape、modifiers backing layout、owner/main Entity/item-animation EntityPtr、关联实体或资源变化。 | Player+0x13D8 是 5 个指针槽，player-native-static.md:21–28；equipment-installation.md:13、Weapon factory/引用合同:32–38。类型 replacement 不能写旧类裸偏移。 |

### 4.2 Resource unchanged 的实际判据

**复用事实：** mod/src/game/client_resources_native.cpp:69–80、81–135、168–176、177–218、220–255 逐层安装 ANM 主/overlay、phase、selectors、shader、图像替换和动态 layer transform；未调用 ANM2::Update。主审区分两条实际分支：`ClientResourcePoseMatches` 比较完整资源和当前姿态，只用于完全相同值跳过安装及安装后回读；它不是一般的资源不变性判据。`ReusableGraphics`（144–159行）才是无资源重建的现有分支，核对已加载、path、layer count、动画名、实际selector数组/几何、Image/control pair等，并且当前还要求目标frame等于原生当前phase的floor。通过后 `SetPose(...,false)` 安装姿态，不重新Load；帧/动画或selector变化不满足时仍进入完整候选路径。I23不能把当前有限复用当作所有动画跨帧都已支持，新增放宽前必须补齐所用字段合同。图像文件名相同本身不足以证明实例或借用相同。

**本次源码确认：** mod/src/replica/player_plan.cpp:80–118 将实体形状／类型／身份关系、Player resource application、GetBodySprite、costume/descriptors/map、Weapon、head animation、special images 与 record sprite 的变动纳入资源 lease invalidation / full Resources action。mod/src/game/client_player_equipment_native.cpp:50–63、100–116 对 costume 同结构同元数据同文件名才复用原 vector、否则先建全 candidate 后交换 owner/map，并保留实际旧 owner／失败前缀。异常加载不能继续显示“半套”姿态。

**静态推导：** I09/I23 的快路径以“已存在原生实例 + 所有结构/资源 version 不变”为前提，只安装当前姿态和有限 backing state；动态 transform 不能缓存成资源静态定义。新资源、换装、Weapon type、身体形态、role、成员／引用／控制关系变化走 R03 已有完整结构／资源提交。必须保留所有现有 consumer 要求的值，即使目标线协议以后紧凑编码，也不能先从合成 Player target 中省略后再假定原生仍可绘制。

**UNKNOWN（阻塞范围）：** ANM/Weapon/Costume 全部成员的每一种类型 writer 集、每个额外 hidden draw/cache 字段、角色专用视觉是否依赖 Authority gameplay cache，不能从结构大小推导。I01 将真实 consumer 和 writer 对齐后才选字段；无对应 reader/writer 的成员不得新增快写器。本轮不重复逆向已按 P11–P13 闭合的 factory/dtor。

## 5. R04：一次 Manager 窗口完成完整动态组

**对应：I23、I24；E03、E05、E09、E15、E21。证据：复用隔离事实 + 本次源码确认 + 静态推导。**

1. **owner/thread/context：** 只在主线程 Client Manager boundary 处理完整目标；当前入口位于 FrameScope 之后、完整 Manager original 之前，ClientManagerBoundary 返回跳过整条 Manager 链，不推进 Authority tick (mod/src/game/client.cpp:474–480)。ClientScene::Pump 还要求 TopOwner=Manager、context healthy、非阻断并确认 GraphicsWindowValid；持有 NativeApply 与 Graphics borrow，不能由网络 worker 持有 Player/ANM/GL/Lua 对象或在原生调用栈中暂停 (client_scene.cpp:517–585)。
2. **纯值与原生步骤分开：** 先在拥有值的模型上检查 Key/use/shape/route/body/structure/resource revisions、所有字段状态、budget 和全部依赖；确定这个 body 的整个快更新组闭合后才进入 native writer。跨线程仅传自有 DTO 与稳定身份，不传对象指针。现有完整场景每 Manager pump 最多一个对象命令 (client_scene.cpp:591–600)，I23 需要为实时组提供在单个 Manager 窗口内有界完成的原子逻辑步骤；不能把多轮完整 Scene 的“逐轮一个步骤”误用成逐帧 pose 更新。
3. **完整组：** 一个 PlayerFast candidate 的最终公共 Entity 动态值（唯一 Entity owner）、声明支持的 Player 动态值、所需 13 个 ANM 当前 pose、同结构 costume layer pose、同类型 Weapon 当前 dynamic values 及它们需要的一致关系构成同版本依赖组。具体成员由 I01 目录定，不默认每个 candidate 必须包含 Player 的全部 20 状态族；凡同一 consumer 需要一起读取的值必须同 Manager 写窗完成。
4. **边界内一次完成：** preflight 必须一次验证整个组的所有成员、原生实例、resource borrow 与 generation。完整 Native writes 在一次 Manager boundary、任何后续 Render consumer 之前结束；不做渐进式同屏更新、不跨帧暴露成员子集、不借用原生 Update/ANM2::Update 生成丢失姿态。预算不足时整个尚未开始的组保留旧完整显示或报告明确 stale/gap，不写部分新组；允许在剩余 budget 内合并多个完整 body group，但不得切开任何一组。
5. **失败前缀：** 第一次 native write 之前失败时保留上一完整 A。第一次 write 后失败时不能假称回滚成功：记录精确 body/key/field/writer phase 和实际 owner prefix；立即撤销受影响组的 DisplayReady/applied-valid，停止其后续 draw／借用，不发 applied ACK；等真实 CPU/GPU/Lua borrow 与 queue 消费退尽后按 R05 owner 退休，再从完整 T 重建。未知析构 owner 时留住资源与隔离，不盲目 free。
6. **不变性条件：** 快写器只写值；无 Load、构造、析构、换图、EvaluateItems、Update、Fire、碰撞、EntityList 更新、callback 或持久副作用。任一结构／资源依赖变化必须撤掉快速候选并进入完整结构事务。当前 Player writer 仍是 Scene/Structure/Finish 多步骤，不是高频快更新的实现 (mod/src/game/client_players.cpp:81–112)。

## 6. R05：取消、失败与资源退休

**对应：I21、I24、I26、I28、I36；E14–E16、E19、E22。证据：复用事实 + 本次源码确认 + 静态推导。**

**复用事实：** docs/design/tx-04-17/tx-04-17-failure-retirement.md:11–15、56–87 的状态机按是否改过 native 世界、实际对象 owner 和已进入的失败前缀收束：先禁新输入／新目标／受影响 draw，网络线程 stop 只请求而不代替主线程资源退役；无法证明清理安全时留 owner、关闭隔离出口并报告，不能伪造 Off。访客期间保存、统计、成就、解锁、REP 附加保存及 Mod 写入隔离沿 tx17 ST8 与 tx20 player application 既有合同保留。

**本次源码确认：** ClientScene::Pump 退休分步先停 presentation/new borrow，清对象关系，再 Grid，再 native world；DestructiveReady 与 drain 通过前不会析构；对每步保留 owner、grid/native prefix 和失败阶段 (mod/src/game/client_scene.cpp:521–556)。ClientManagerBoundary 仅在 worker Finished、两次物理中性、MenuReady 且 scene.CanReturnToMenu 后解除逻辑 Client 隔离 (mod/src/game/client.cpp:522–530)。这些是已有常规路径，不代表任意损坏指针、强制终止、SEH/access violation 能自动恢复。

**未来实时传输采用合同：**

- 用户 stop、Host stop、Peer timeout、scope/route/generation 更换、应用失败时，先停止发送新 edge/input 与新 range target；清除待发送／未安装 PlayerFast 候选。worker 结束不触发 native world Off。
- write 前候选可撤销；一旦任何 native write 已执行，应用状态不可通过 DTO A 回滚声明恢复。对受影响显示范围关借用、保留实际 owner prefix，随后从完整的新 T／Baseline 重建最多一次；非受影响范围只有依赖不共享且仍完整时才可继续展示。
- 旧 route 的延迟 Input/ACK/range/fragment 不得标成新 scope。连接恢复可保留连接身份，但世界 route、binding generation、body use/shape 与输入 edge 历史必须按新范围重验；不继承旧 applied ACK。
- 先等待实际 draw/pass/CPU/GPU/Lua borrowed refs、固定队列和资源 pins 的 consumer 生命周期完成；按 Player → Weapon/Entity/Costume 的正式 owner 撤反向引用后释放。Player out-links 在 non-Player object release 前解除；Image pin 不代表已入队 draw 仍可释放。
- 保持已有部分安装／析构 owner，不重复 Remove/ClearReferences/Free；未知 native EH 是否已清的前缀不重复猜析构。旧 complete A 可作历史一致性依据，不能在 native 变更后继续充当有效展示。

**UNKNOWN（阻塞范围）：** tx17 明确保留任意 native 异常／进程关闭中断／损坏 owner 的不可恢复前缀；I24/I28 必须针对新快写入的每个新写后故障边界补充 owner 日志与正确退休步骤。本轮没有发现可安全用于 21 的新通用 rollback/free helper，故不引入。

## 7. R06：正式采集、低频失效与共享值

**对应：I05–I07、I09；E02、E09、E18、E21。证据：复用现行 Service 合同 + 本次源码确认 + 静态推导。**

**本次源码确认：** Service 单一主线程协调器、consumer 槽为 4 (mod/src/world/common/service.h:7–39)。既有 Latest=1、Record=2，Live=3 已在当前 live_consumer.h:8 声明；Service::AttachConsumer 对重复 id、无效 callback、publish 边界、scope pending、槽满和 OOM 有具名拒绝 (service.cpp:35–45)。MergePlan 合并相同 domain 的 phase，并取较短 interval 与不小于 consumer 要求的各域预算 (service.cpp:20–33)；Due 以 capture index 对 OrdinaryPost interval 作实际筛选 (service.cpp:63–71)。LiveConsumer 只在回调借用 envelope，并将自身的单份普通值转成共享 immutable LiveBatch 给多个 peer；变更、loss、gap 和不完整 domain 会撤资格 (live_consumer.h:21–29、live_consumer.cpp:8–46)。

**采用合同：**

- 新正式 live consumer 绑定 Authority session lifecycle，不依赖 Recorder／diagnostics 打开；Host 的多个 Peer 从同一已提交 const batch 读取，不按 Peer 重新扫 Lua/world。Client/Off 不启动 Authority 采集。
- ordinary capture 候选要经过同一 outer/tick/owner/mutation/coverage 和正常 commit。loss、mutation、run/room/epoch/plan revision、Lua state/resource reload 立即撤低频 cache，先标 gap 再从全量有效来源重建；单纯 odd-only 不撤销已提交 ordinary sample，也不补完整 WorldDynamic。
- interval 只节制真正被 plan 覆盖的 source 工作；若当前 writer 仍先遍历大图、再由 consumer 丢字段，不能声称其获得高频性能收益。热 PlayerFast 使用独立 typed value/source 和逐身体资格，不在每 tick 做完整 Lua JSON 或完整 World scan。
- Source 回调内只借 const 纯值至返回；网络/压缩 worker 使用自有 bounded batch，绝不持游戏地址。consumer detach 前撤所有 pending/latest target 与 owner token，Service 变更计划时旧值作废。

**未知范围：** 当前 Service interval 可验证调度门槛，但 I07 的固定字段 reader/source 热路径及 I29 的成本尚待实现；不能由 plan interval 推断 writer 已避免全域扫描。若某个低频字段没有明确失效入口，该具体成员使用有界普通刷新，不永久 cache。

## 8. R07：roster、tick progress 与输入 ACK 分离

**对应：I18、I21；E03、E08、E12、E14。证据：本次源码确认 + 网络合同设计目标；不需要 native 逆向。纯工程只依据 net/live.*、client.cpp 和 replica codec/model。**

**本次源码确认：** `client.cpp::InputTiming::Observe/Target` (mod/src/game/client.cpp:27–58) 使用 server tick 与同一端 monotonic time 的真实样本建速率；输入目标拒绝过期 progress、超长 RTT/publication age、异常 rate，并调用既有 `NextInputTarget`。ClientManagerBoundary 收 Progress 时更新 `c.progress`、roster 和 timing (client.cpp:503–510)，输入机会仍由 `c.progress.server_tick != c.last_tick` 和目标递增触发 (552–563)。Progress generation 更换先中性化并清 timing/last_tick (506–509)。发送成功才消费边沿；queue 失败不增加序号、不提前出队 (552–563)。InputAck 只在真实 processed 进度到达且 generation 对上时更新，超出已发送序号报 `client_ack_unsent` (511–516)。

**本次源码确认：** 当前 `HostPeer::Progress` (mod/src/net/live.cpp:184–193) 先发 Roster，只有 processed、processed_tick、generation 都非零时才另发真实 InputAck；零进度合法，不造假 ACK。Roster 体包含 `server_tick`、accepted/processed/processed_tick、generation、session、roster/input eligibility 和发送/echo 时刻 (live.cpp:36–63, 184–193)。当前 live 收端仅将 Roster 映为 Progress event、将 InputAck 映为处理高水位 (live.cpp:251–277)。stream codec 已定义 TickProgress schema (mod/src/replica/stream_codec.cpp:47–52)，但 `live::Pack/Receive` 还没有接入 TickProgress；它目前不是 Host→Client 的独立高频进度消息。

**采用合同：**

- roster 20 Hz 保留完整资格／generation／名单和 session state；资格、暂停、停止变化即时发。它可以合并成最新 roster，不能占用可靠 control 的序号缺口。
- 新 TickProgress 只由每次有效 Authority commit 产生，携带真实 tick、tick kind、generation、input_enabled、服务器同钟 sample/send、客户端 echo 值。只在有新有效 tick 时推进；250 ms heartbeat 只能证明连接存活，不制造 tick。
- InputAck 只报告真实被 Authority scheduler 处理的 nonzero sequence 与 processed_tick。首次 processed 仍为 0 时不伪造 ACK 启动输入时钟；TickProgress 独立给输入时序估算锚点。相同确认可重发但不计新处理。
- 客户端把 roster 与输入时序状态分离：高频 TickProgress 更新 `InputTiming` 和新 canonical 输入目标；roster 只改变资格／generation。保留已定义的 lead=2、age=0、future 上限、实际输入速率估计、启动／重绑中性例外、边沿 exactly-once；不按跨机单调钟直接相减。
- 输入约一条新 canonical 命令 / 每有效服务器步 / 每控制组，暂停、focus/UI block、binding mismatch 中性；缺输入仍中性，不以重复旧 held 或加长 age 掩盖 20 Hz 驱动缺口。InputAck 不等同任何 body 的 native-update/activity witness。

**状态差异：** roster 20 Hz 与 TickProgress／InputAck 正常服务约 60 Hz 均是 tx21 设计目标 (network-contract.md:20–30)，不是当前 live 已达频率；当前源码证实是 roster 同时携带 tick 并驱动 last_tick，因此 I18 必须先增加并接通 TickProgress 才能满足合同。该差异不会因 MC_POST_PLAYER_UPDATE Wiki 页写 60/sec 而改变。

## 9. R08：基础战斗与原版交互

**对应：I23、I24、I32、I36；E05–E07、E19。证据：复用 18–20 消费／隔离合同 + 本次静态归并；运行仍待 I39。**

| 结果链 | 本轮采用边界 | 复核责任 |
| --- | --- | --- |
| Player 射击／眼泪与 NPC 受击死亡 | Host 原版输入／Entity update／伤害／死亡为唯一裁决；Source 只发完整最终 Entity identity/shape、空间／生命与必要 draw pose；Client Entity target 应用最终值，不调用 Player Weapon Fire 或 NPC/Entity Update，不本地再碰撞伤害。 | I01 按 P02/P05/P13 连齐具体 getter→Entity/player target→draw reader；E05 比较真实 native 后态，确保不是 DTO 自比 DTO。 |
| 拾取、计数归零与掉落消失 | Host 裁决拾取、消耗和删除；Player 最终计数属于其唯一 Player owner，Pickup/NPC/Familiar 成员变化属于 Entity owner。完整成员后态中的缺席才可物理退休，最终零值、空槽、Null 与不适用不得混用。 | I07/I23/I24 的跨 Player–Entity 同窗完整组；E06 覆盖最后变化后的静止最终值和删除。 |
| 炸弹与 Grid／门受损 | Host 负责爆炸、碰撞和 Grid state transition；发送完整 Grid/door/Entity final state，Client 仅安装核定终值及 draw pose。R02 确认的 collision helper 私有副作用仍不进入客户端快速应用。 | E05 的炸弹／Grid 同窗口后态及姿态核对；既有 tx-04-18 Grid owner 不重复逆向。 |
| 已支持 Laser 或 Knife | 只沿已有 REP/SDK 具体 dynamic Weapon type、相关主实体与 Draw consumer 使用确定已有适配；发射／伤害结果仍 Host-authoritative。 | P13 的五槽／W01–W15 类型表与 tx20 已有有限场景优先；E05/E07 必须声明所选类型与实际源，不用一类静态结果外推其他 Weapon/Boss/Mod。 |
| 角色／costume／Weapon 变化 | Host 发全结构版本与资源依赖；Client 先构建和预检整套 candidate，在同 Manager 原生 owner 窗口提交；失败不发 ACK，按资源 owner 退休。 | I09/I23/I24 与 E07；Client 不执行 Add/RemoveCollectible、Add/RemoveCostume、EvaluateItems 或构造攻击 entity 的玩法 helper。 |

本轮没有新增 Gameplay private writer 逆向：I02 复用 tx-04-17 isolate 与 tx-04-18/19/20 已核 consumer/adapters，缺少新增路径则不扫描全部武器／Boss。**UNKNOWN** 只保留所选 E05 之前必须从已支持 adapter 确认的具体 Projectile/Laser/Knife 类型与表现成员；由 E05 输入场景清单和 I01 consumer 链闭合，不扩展成所有内容或事件系统研究。结果没有任何 E2E 通过声明。

## 10. R09：当前网络源码状态与待接入的可靠性边界

**对应：I04、I11–I17、I20、I25、I27；E10–E17、E21。证据：本次纯源码确认 + 网络合同设计目标；不需要游戏逆向。只依据 net/live.*、client.cpp 和 replica codec/model。**

### 10.1 已在源码中的范围

- **当前 wire 实现：** mod/src/replica/stream_codec.cpp:76–84 已实现 ICSR version 2 / 40-byte logical header 与 ICRF version 2 / 固定 64-byte fragment header。整数 little-endian，reserved/flags 需为零；每数据报上限 1200 byte、分片净荷最多 1136 byte；fragment 需匹配 connection/route/transfer/wire length/decoded length/index/count/kind/lane/codec，并验证分片几何。该事实是当前代码，不是仅计划。
- **当前严格 stream schema：** stream_codec.cpp:29–64 验证 kind、direction、handshake/preparing/active/retiring 合法消息、精确对象成员集、scope、range 依赖和 input canonical 值；InputAck sequence 必须等于 input_sequence，Roster 校验 processed≤accepted、processed_tick≤server_tick、roster mask 及身份。TickProgress 有独立 schema，但尚未接入 live adaptors。Capabilities 明确压缩当前只发布 none，zstd 未协商 (stream_codec.cpp:65–74)，非 none codec 会被 Reassembler 以 `stream_codec_not_negotiated` 拒绝 (85–92)。
- **Live adapter 当前范围：** mod/src/net/live.h:8–15 声明 v2 真实游戏 socket 与旧 v1 离线 codec 分离；live.cpp:79–105 将受支持旧 model 消息映射为 stream messages。Host 只接 Hello/Ready/Disconnect/Input，并检查 nonce、固定 Participant、当前 Scope、输入身份；其他控制／world 消息拒绝 (live.cpp:121–180)。Client 目前接 Welcome、Roster、InputAck、Stop/Disconnect；意外 world kind fail `live_unexpected_world_message` (live.cpp:250–277)。`Pack` 对当前未接入的 Range/Baseline/Applied 等 fail `live_v2_world_not_connected` (live.cpp:80–105)。故当前 stream codec 定义了 schema，不能误写为 Host/Client 的完整世界传输已经实现。
- **当前实际 message:** HostPeer::Progress 发 roster，并仅在确有非零 processed 发 InputAck (live.cpp:184–193)；model::InputProgress 对 participant≤3、generation/sequence/tick 非零有前置检查 (mod/src/replica/model.cpp:335–338)。model::Applied 只在 committed 且 snapshot 非零时可建消息 (model.cpp:331–333)，但 live pack 当前没有 AppliedAck route，不能声称 socket 已把 native commit 确认送达。
- **当前 live reassembly limits:** live.cpp:73–77 的 Limits 是 bytes=65536、nodes=4096、depth=24、string_bytes=8192、fragments=64、candidates=2、timeout_ms=1000。stream Reassembler 对 candidate count、冲突片、重复同片和逾时具名处理 (stream_codec.cpp:85–92)。主审补入 GameCapabilities：真实 socket 当前广告小消息的 64KiB/64片/2候选/1000ms 上限，AgreedLimits 明确拒绝不满足固定 bytes/datagram/fragments 的对端，并把 candidates/timeout 收紧到双方最小值后配置重组器、撤销旧候选。32MiB 只是通用 schema 保护上限；I14/I21 的正式世界授权未接入，不能声称当前 live 能收大基线。
- **当前纯模型/codec:** replica/model.cpp:91–120 的 ValidateModel 区分 publishable、完整 coverage、native_applied=false、每 component 的 scope/revision 和引用；model.cpp:299–329 的 Difference/Reconstruct 只允许同 scope、完整 publishable base/target。replica/codec.cpp 有旧 model 消息结构严格检查；真实游戏 live 不与旧 ICSN/v1 消息混正文，见 live.h 与 live.cpp adaptor。

### 10.2 I11–I17 必须采用的传输／ACK合同

以下是当前 network-contract.md 的设计目标，不可用本节开头的已实现 framing 代替尚未完成的世界 transfer / ACK 状态机。

| 责任 | 采用合同 | 当前缺口 / 失败动作 |
| --- | --- | --- |
| 路由与准备 | connection、route epoch、transfer、message sequence、world scope、Player body sequence 各自作用域独立；Handshake 小预算；只有已协商且正式 route 接受后才分配 world candidate。scope/route 换代清旧重组、待输入、候选。 | 当前 live 固定 route=1，world 不接收；未有 route prepare/accept 与候选隔离。旧 route／cross Peer 数据不可借当前 scope 标签。 |
| 完整范围收取 | Range/Context/Structure/Resource/WorldDynamic/PlayerFast 必须拥有 declared schema、complete membership、精确依赖 revisions。收到 fragment ≠ 完整逻辑消息，完整消息 ≠ 依赖 ready，依赖 ready ≠ native applied。 | 当前 Reassembler 只在全部部分可拼齐后输出消息，不发 ReceiveAck；world Range 未接 live。完整范围缺片、冲突或超预算必须丢候选并具名失败，零 native write。 |
| Compression / memory | 压缩算法与窗口须协商；encoded/decoded 单独上限；先检查长度、节点、depth/string/budget，再限窗解压；每 Peer candidate、总内存、历史、重试与全局 fair share 均有界。只按实际压缩率、payload/线上包成本做预算。 | 当前只有 codec none；zstd 是后续 I12/包装交付，不能写成依赖已存在。已知 codec!=none 必须拒绝。预算声明/实现不符时拒绝握手或缩小协商，不静默扩大实际对象池。 |
| Reliable control / cancel | ScenePrepare/Prepared、ContextReady、AppliedAck、Resync、Cancel、Stop/Disconnect 等有严格状态机、幂等完成记录与独立可靠 control sequence；合并 roster/TickProgress 不占可靠序号，取消/stop 不等待自己所依赖的未完成世界数据。 | 当前 live 只支握手/ready/roster/input/disconnect，其他 v2 operation fail closed。可靠控制、取消候选、重传和 stop 仍未接通。 |
| ACK 语义 | ReceiveAck 只声明已收分片窗口；ContextReady 只声明精确依赖准备好；AppliedAck 只能由 Client main-thread真实完整 native commit / retained range 形成。Player 子帧 ACK 不提升整房 revision；失败/取消/静止最终值仍需对账。 | model::Applied 的纯模型构造要求 committed=true，但不是 native commit 证据本身；未来由 I25 的 ClientScene commit/result 来调用。丢 ACK 只补确认，不重复 apply/Fire/构造。 |
| 完成收敛 | sender 保存 last unconfirmed complete target；重复包/重复 ACK不刷新有效进度；最后静止值与删除按定时重传到达最终 ACK 或明确触发有界完整恢复；慢 Peer 不拖住其它 Peer。 | 当前 Live 没有 receive/applied ACK，也没有 transfer/window/公平预算；不能把网络心跳、Roster 或 decode 成功当最终收敛。 |

`tx-04-21-network-contract.md:68–85、91–109、122–150` 保留 v2 路由、64-byte 头、重传与预算目标；源码实现已确认 header 与 strict codec 部分，范围传输、zstd、ACK、可靠控制和 fair scheduling 仍为 I11–I17 后续工作。研究计划 tx-04-21-native-research-plan.md:111–117 对“现有 ICRF 32-byte”有版本滞后：本次读取的当前 stream_codec 已是 ICRF version 2 / 64-byte framing；后续维护该研究计划时应纠正“现行代码”描述，不能把旧 v1 头套在 live v2 上。

## 11. 按 R 项的完成判定与待实现关系

| R | 本次证据结果 | 采用的未来代码合同 / 主要引用 | I / E |
| --- | --- | --- | --- |
| R01 | 复用 18–20 consumer catalogs；本次将字段owner和实时范围归类；逐成员全 consumer 链留给 I01。 | 一值一 writer；完整目标不丢现有 reader 必需字段；low-frequency cache 有具名失效。 | I01、I03、I05–I07 / E02、E05、E09、E18 |
| R02 | 复用 ordinary complete outer commit 规则；本次源码核 witness/Collector 现状；IDA 定点确认 odd 0x2F9340、0x2F9559 及副作用／特殊分支。 | 每实际正常返回 body 独立 candidate；ordinary/odd 分时间线；外层 Clock commit 后才发布；未更新明确 gap。 | I08、I18、I23 / E03、E04、E08、E18 |
| R03 | 复用 20 已核 Player layout、装备／resource field owner；本次确认 exact resource reuse invalidation。 | 当前动态 pose 与 structure/resource definition 分开；不变资源要求全 pose 与实例条件相等；结构变化完整事务。 | I09、I23、I24 / E05、E07、E09、E22 |
| R04 | 复用 Manager-only Client write window、borrow/context 与无回滚失败原则；本次核当前 Scene 仅每轮一步且没有 PlayerFast writer。 | preflight 后同一 Manager 窗口完成一个完整关联写组；写后失败撤显示、留 prefix、不 ACK。 | I23、I24 / E03、E05、E09、E15、E21 |
| R05 | 复用 17/20 持久隔离、真实前缀、owner退休；本次核 ClientScene 顺序。 | Stop 先关新输入／候选／显示；主线程借用退尽后按对象真实 owner 清理；未知前缀留隔离。 | I21、I24、I26、I28、I36 / E14–E16、E19、E22 |
| R06 | 复用 Service merged plan、capture commit、odd gap 和 loss invalidation；当前正式 Live consumer ID=3 已在源码声明。 | Recorder 关闭亦可取正式共享 immutable batch；失效立即撤 cache；热身体 source 单独 typed/有界。 | I05–I07、I09 / E02、E09、E18、E21 |
| R07 | 本次纯源码核 timing / target / edge / roster / InputAck；TickProgress schema有定义但 Live 未接。 | roster 20Hz，真实 TickProgress / InputAck 独立高频；不造 tick/ACK，保留 lead/age/启动中性/edge exactly-once。 | I18、I21 / E03、E08、E12、E14 |
| R08 | 复用 18–20 Authority-only 与 native consumer/apply合同，无新增Gameplay逆向必要。 | Host-only 裁决；Entity/Grid/Player 最终状态按 owner 安装；E05/E06/E07 给实际场景与 consumer 证明。 | I23、I24、I32、I36 / E05–E07、E19 |
| R09 | 本次纯源码确认 ICSR/ICRF v2 headers、strict schemas、live adaptor 子集和 limit mismatch风险。 | Range transfer、zstd budget、可靠控制、Receive/Context/Applied 三类ACK 和最终静止收敛继续由 I11–I17/I25 落地。 | I04、I11–I17、I20、I25、I27 / E10–E17、E21 |

## 12. 主审应优先复读的证据点

1. IDA 目标是 J273，imagebase 0x400000。重点复读 RVA 0x2F9340 函数入口和 caller VA 0x92E921；普通 Player vcall VA 0x6F9559/RVA 0x2F9559 的 `FF 52 0C`、后续 `push 0; call 0x6A6A40`、Position/Velocity helper writes、VA 0x6F9565–0x6F9575 清 Entity+0x168 bit 0x4000；特殊点 VA 0x6F93F9/0x6F949C；额外不明向量 VA 0x6F96CD。不要把 0x2F9559 当 helper/clock commit 或称全 body 60 Hz。
2. 对照官方 MC_PRE_PLAYER_UPDATE 源码 custom callback 521–534：true 跳过 Player derived super 只调用 base Entity original；PlayerNativeReached 必须代表真实 derived call 已进入。
3. 对照 Collector::PlayerReturned 466–493 目前只有 update witness、Collector::PreparePostState 793–842 的 odd-only gap，以及 EndManager 356–389 的外层提交顺序。未来 I08 的 PlayerFast candidate 还需同身份资格、字段级读权限、outer commit/cancel，当前皆未接通。
4. 对照 Player 13 inline ANM 布局及 Costume/Weapon 真实 owner：Player+0x41C stride 0x114 count 13、costume vector +0x1220、15-layer map +0x122C、five Weapon pointers +0x13D8；优先核现有 20 `ClientResourcePoseMatches` 与 candidate-swap 逻辑，避免用文件名或 `PlayerUpdate returned` 推导资源安全。
5. 对照 stream_codec 40/64-byte headers 与 live `Limits` 64KiB/64 fragments/1s；当前 live adaptor未接 Range、TickProgress、ReceiveAck、AppliedAck 和 zstd。设计文档的 32MiB network target 不能写成现存 live 预算。

I01/I03—I05 本轮实现及界限见 [基础实现](tx-04-21-foundation.md)。I06—I09、I11—I18、I21、I23—I28 与最终 I39 仍待推进。运行是否达到 30/60 Hz、是否完整覆盖每个 body、带宽／时延／CPU、画面一致性和访客隔离都需按 tx-04-21 E2E 计划在统一验收中证明；本文件不作这些事实声明。

## 13. 主审结论

主智能体独立复读了 VA 0x6F9340、0x6F9559 起的指令和 0x6A6A40。采用逐身体真实返回见证、后置碰撞与临时 flag 清理之后的候选、外层正常提交的合同；拒绝把特殊暂停分支或未知额外向量当作普通 PlayerPost。纠正 manager+0x1264 为元素计数，纠正 Player 公共动态在线路上的 owner 为 PlayerFast。其余资源／失败合同按既有具名 owner 复用；本轮没有新增 native writer，I08/I23 的具体快路径必须遵守本文限制。此为静态主审，未运行程序验收。
