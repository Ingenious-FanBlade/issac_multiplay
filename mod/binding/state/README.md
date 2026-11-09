# 原生二进制布局源

`version.json` 指定整数布局版本、域文件和公开成员核对目录。`schema.json` 描述域文件结构；`gen_state_layout.py` 在普通构建中执行结构与跨类型语义检查，同时生成代码表和 `docs/design/binary-state/layouts/`。不运行游戏或测试，不计算哈希。

布局源任何改动都必须手工递增版本。版本 1 是首次创建，尚未用于网络握手；第 9 步接入握手后，两端直接比较整数版本。版本是 u16 正整数，与帧头 `layout_version` 同宽。

- `ranges` 按原生偏移排列；子类只写父类大小之后的范围，生成时展开父类，完整覆盖 `[0,native_size)`。未知区必须具名，不能自动补为填充。标量和 `bytes[N]` 类型标签的宽度必须等于 `size`。
- 原生文档和 `NativeRanges` 还会展开内嵌类型，保留各子成员实际归属，父／子证据状态取较弱者；未知计数包括内嵌未知字节。容器指向的外部元素按其元素布局另行处理，不计作容器头自身的字节。
- `wire.kind=copy` 只允许定长标量、数组和逐成员核定的纯值类型。复杂嵌套类型用 `nested`，容器用 `container`；转换、引用和资源都必须声明双端具名处理器。
- 嵌套状态可用 `wire.projections=[0,1,2]` 拆到父类型不同块，父块分别引用 `字段名#0`、`字段名#1`、`字段名#2`；所有实际产生发送字节的子块必须各出现一次；空预留子块可省略，原编号保持；既存空投影合法且不产生负载。这样 Entity 的 Sprite 外观、动画、图层能独立变化，不重抄 ANM2 字节事实。
- 块从 0 连续编号，最多 32 块。父类块固定在前；预留空块必须写 `reserved=true`。每个产生状态的字段恰好属于一个块。
- 容器声明原生步长、元素类型、数量上限和存储形式。显式 `wire.inline_count` 声明固定内联数组的全部物理槽数，要求范围大小等于槽数×步长且线上上限不超过容量；原生目录展开全部槽位（含无效槽UNKNOWN），线状态只按当前有效前缀编码。独立数组计数可来自本类型的 `count_field`，或来自具名 owner 的 `count_from`。不能把整个复杂对象当字节数组绕过递归规则。
- `variants` 为形态选择字段的取值分配独立布局编号。各臂替换原生范围和指定块，生成器再次检查全覆盖、取值互斥及字段归属。
- `public_owners` 是本布局已接管的公开目录范围：其每个 `ReplicatedState` 成员必须有字段、派生、权威或不适用的明确回映射。没有接管的域不得据此宣称目录覆盖完成。
- 嵌套类型可声明有证据的 `contexts`（例如所属资源号、容器元素序号）。派生成员的 `fields` 用 `@上下文名` 引用，不能用本地指针充当已发送状态。
- `companions` 声明另有原生字节表的伴随对象（例如 EntityPlus），经具名 owner 解析，不占根对象偏移。`read_requires` 声明特殊读取资格，资格不同的复制区间不合并；资格由采集 owner 提供真实见证。
- `complete` 是生成值：存在未知字节、未核声明或不完整嵌套时为 false。它只描述原生布局事实；不表示同步、应用或 E2E 已完成。

`entity.json` 是第 3 步的 Entity 公共部分（E0—E9）和 EntityPlus 公共前缀。`entity-effect.json` 继承它，是第 1 步的继承/变体首批输入；Effect 的全部变体与附属 owner 属于第 4 步。未知字节照实保留并继续生成，不能把它们当成可发布的完整 Entity 或 Effect 支持。

`entity-classes.json` 已接入第 4 步的 Familiar／Knife／Bomb／Tear／Pickup／Laser 及其伴随对象、几何与已核专用表现。版本 20 增加后三类；它们的条件计时、私有未知和 SDK 声明照实保留。这是版本20的历史结构：当时HomingLaser四组容器分别投影，Pickup循环数组发送有效前缀和u16 count；现行分别按版本37／35撤销Host输入；三类新增引用不因旧 Lua／JSON 路径退役而丢失。

版本27续研第4步原未决前四项：Knife尾部六个f32组成K4，具体分裂／碰撞用途已核，+518更名为碰撞方向角；Slot进度对象指针与Projectile回调指针归本地；Effect传播／生成参数的专用用途和Host归属已核。六段4B空白按全映像候选及间接成员边界核对，尚未把填充假设写成已核事实。依据和剩余问题见 `docs/design/binary-state/implementation-4-first-four.md`。

版本28按 `docs/design/binary-state/synchronization-field-review.md` 修订同步职责：Knife K4六值／命中父链选择、四类命中历史、Projectile伤害／三项延迟变更参数、Laser掉落概率／伤害倍率共17个范围改归权威，不再有wire或线上validator，相关公开成员回映射改为authority；清空的块显式reserved，后续块号不变。Familiar充能／状态／硬币等真实展示输入保留，Weapon充能比例的跨域展示缺口明确记录。其余候选不因主Render没有直接位移命中就批量归权威；原生范围、证据等级、UNKNOWN及正常Client owner生命周期不变。

版本29按 `docs/design/binary-state/internal-entity-relations-review.md` 核对24项内部关系：Knife4／Tear6／Pickup1／Laser12共23槽权威；Familiar+DAC只在新增variant240／布局130传真实引用，其余Familiar臂留Host。Knife K3保留空块号，公开关系和必要本地撤边保留。240绳索payload仍待补齐；HomingLaser unoptimized已证实由Room Light消费，当时保留四容器，版本37续研后只保留两组最终点列。上述版本28段落为前次修订范围，不是当前版本号。

版本30按 `docs/design/binary-state/entity-common-synchronization-review.md` 复核Entity公共部分：33个范围撤销wire、专用读取资格和线上validator，28项公开成员改为authority。32个Host玩法范围包括20个公共计时器／毒火伤害相位／两组历史位置及伤害物理输入；removedByFactory归本地工厂生命周期。valid／exists／dead、碰撞类别、尺寸及有Effect展示用途的速度倍率继续保留。E3仍有两项缩小状态，块号不变；每个Entity最小负载减少223B，未知和证据等级不变。旧运行路径在第13／14步统一切换。

版本31按 `docs/design/binary-state/entity-lifecycle-synchronization-review.md` 撤销公共valid的wire／读取资格／validator及具名handler，原生字节归本地构造／发布／池／退休owner；Entity最小状态由411B降为410B。Player活动／备用身体的独立激活资格仍由第5步P7结构接管，不把valid猜成room_member，不在E2重复发送；新结构编码／writer尚未实现。exists／dead继续同步，已有Exists=true但valid=false且非列表身体的历史反例，以及Kill／Remove与退休分离的静态证据；UNKNOWN不变。

版本32按 `docs/design/binary-state/animation-resource-synchronization-review.md` 排除AnimationState两组事件位、ANM2／LayerState loaded原字节及Entity ColorParams整个队列。AnimationState块3、ColorParams块0、Entity E7显式reserved；原生语义／UNKNOWN保留。file_name改为可空资源选择，声明`anm2_selected_file_to_resource/from_resource`，保留资源有无与本地准备失败的区别，实际执行器仍归第8／10／13步。Layer路径与实际Image独立，Host待重载但保留旧图时不得按路径抢先加载。is_playing／anim_frame／playback_speed因官方InterpolV2真实绘制消费者保留；最终ColorMod仍完整11个f32。撤销颜色锚点／线上validator／读取时钟及七项颜色公开成员的状态回映射；Entity最小状态410B→391B。没有构建或测试，生成表仅作当前布局输出。

版本33按 `docs/design/binary-state/familiar-synchronization-review.md` 将Familiar11个轨道／计数／方向范围共48B归Host，撤销wire／读取资格／validator及公开成员状态回映射；FireCooldown／State／Coins／IsFollower等展示依赖保持。新增80布局131，只按16个已核原版Weapon动态公式输出i32最终充能amount（0—100），Client具名writer仅存本地表现缓存，不写DB0或创建玩法Weapon。240布局130追加133号UmbilicalBabyData四投影，节点132按逻辑deque顺序只发XYZ（12B／节点，原生0x78）；Sprite完整三投影保留。`storage=array`表示专用reader归并后的顺序输出，不表示原生deque连续。非空payload节点0、超限、非有限值、错类型、来源变化及各投影count不一致均不能发布。根对象UNKNOWN不变，payload含58B未知、外部每节点另108B未知，继续如实保留。Familiar最小613B，80为617B，240缺失payload为623B。来源／布局已补齐，具名reader／展示adapter／本地owner仍待第10／13步；没有构建或测试。

版本34按 `docs/design/binary-state/knife-bomb-tear-synchronization-review.md` 排除Knife10／Bomb11／Tear21个攻击、运动与计时参数范围，原线负载合计减少181B。撤销wire／读取资格／validator／公开成员状态回映射及Bomb ExplosionCountdown、Tear WaitFrames／StickTimer三个计时锚点；原生类型、证据等级、UNKNOWN和Host观测职责保持。Bomb B0及Tear T0／T2显式reserved，块号不变；保留Knife拖影输入、Bomb五份costume／绘制开关及pending清除、Tear效果位／DeadEye与三份辅助Sprite。最终公共PositionOffset／Sprite变换独立同步，不在Client执行原生Update／Interpolate、ResetSpriteScale、GetRenderHeight重算或LoadCostumes。最小状态Knife422B／Bomb1127B／Tear854B，未知497B／441B／478B不变；运行时待第10／13／14步，未构建或测试。

版本35按 `docs/design/binary-state/pickup-slot-synchronization-review.md` 将Pickup12／Slot8个原生范围归Host，撤销19项原线上字段／容器（CycleCount原已由容器派生，不重复扣除4B）。选择组／触碰／自动价格／商店标识、机器状态／捐款／触发／奖品输入及未来循环队列不采集或安装；Pickup Timeout／Wait两个锚点及读取资格撤销。Pickup Price因WaterClip、DropDelay因Render正值隐藏继续保留；所有主／辅助Sprite与真实引用保持。Pickup P3、Slot S0／S1显式reserved；Slot+460／+470是奖品动画指针／is_playing，继续由ANM2投影供应，不误作机器状态或loaded／visible。Client本地循环初始化为空，不运行Morph、价格重算、购买／捐款／开奖或SetPrizeCollectible。最小Pickup703B／Slot538B，未知448B／363B不变；未构建或测试，运行接线仍待第10／13／14步。

版本36按 `docs/design/binary-state/projectile-synchronization-review.md` 将Projectile的FallingSpeed／FallingAccel、HomingStrength／CurvingStrength／Acceleration、WiggleFrameOffset及Scale／ProjectileFlags八项共36B归Host，撤销wire／读取资格／validator／状态回映射，PR1显式reserved。Height因659460临时绘制偏移直接消费而保留；公共EntityFlags、最终Position／Velocity／PositionOffset／尺寸／Sprite、DepthOffset单份映射及Game展示帧保持。draw scope须精确恢复+350，不依赖浮点减加逆运算；不运行Update／Interpolate／碰撞／+470回调。私有+440开关与+460／+464位置的Render事实已写入原UNKNOWN语义，生产／生命周期／布局仍未闭合，375B未知不变，不据此称画面完整。最小Projectile395B；未构建或测试，具名接线和第15步验收仍待实施。

版本37按 `docs/design/binary-state/laser-homing-synchronization-review.md` 保留optimized／unoptimized及直接展示标量，十九项Laser更新输入、六项Homing标量、indices中间缓存和Bezier／系数留Host，撤销两个Laser计时锚点。indices原生类型改为8B双f32点对，旧i32／4B步长记录错误。Homing子块0／3／4与Laser L2／L4／L7／L8显式预留，父只投影子块1／2；生成器要求全部实际发送子块各出现一次，空预留子块可省略、不改编号，兼容既存空投影。Bezier不再声明线上pure_value。两组独立count≤16384、有限点、同目标版本安装、空集合清除、正常本地self／vector owner及析构保持；不调用采样／更新／半帧／接触模拟。最小Laser542→439B，Homing40→4B，Bezier24→0B；421B未知不变。未构建或测试，运行时仍待第10／13／14步接线与第15步统一验收。

版本38按 `docs/design/binary-state/effect-synchronization-review.md` 修订Effect六臂及payload：六项公共参数、TearFlags完整128位及136描述八成员归Host，撤销wire／读取资格／validator／公开状态回映射；FX2全部reserved且布局号不变。保留真实展示参数与Timeout锚点。MrMe原16B未知已核四项Host内部状态；Trail节点由606190明确分配／复制0x40，撤销base_prefix限制。Ghost新增134专用ANM2视图，仅Offset／Scale归派生，通用ANM2不变；布局28改为inline_count=4但只发有效前缀，三投影u16 count一致且≤4，原i32 count派生。原生目录始终展开四槽，56B未知保持。公共Effect444→420B，Ghost空前缀格式下界1037→427B；未构建或测试，运行时待第10／13／14步。

版本39按 `docs/design/binary-state/player-equipment-plus-synchronization-review.md` 审查Player／HUD／装备及Plus：ActiveItemDesc两项留Host、六值24B；Weapon五项留Host、四值16B，不再同步重复chargebar／MaxCharge，原20B未知和base_prefix保持。135号Whip实际0x48另发+44真实主引用；136号TemporaryEffects保留disabled与完整列表，cache mask／evaluate留Host、Player回指本地，外层2B未知与每项4B未知另列。TemporaryEffect三值10B保持；KnifePlus hitboxSource留Host、块1reserved，正式专用身份reader删除，公共两nullable9B保持。Familiar原D76字节有Render／Shadow正面消费，新增1B状态、根未知905B；其他已排除项及未知不恢复。本轮只静态修订和生成，未构建或测试，Player／HUD／reader／writer仍待第5／8／10／13／14步接线。

版本40按 `docs/design/binary-state/effect-next-five-evidence.md` 完成Effect后续五项静态处理：137—141为传播／Timer／生成／计数／113具名原生臂，102保留54／126／167，全部FX2预留；未知字节不抹除。Rotation／Scale移FX0后为16B，FX1为13B；只需整数版本兼容检查，帧格式不改。Trail正常Update尾部裁到count<128，计划上限127，节点块最大8130B，0／1节点合法，超限不截断；其他容器上限不变。Timeout已核101可双减、141刷新，仍每窗重算到期帧。原生ExtraData RTTI只有既有三种具体类型，非附属非空holder或错RTTI须报Gap，不造fallback。普通Release／Win32产品编译与链接成功，没有运行测试；实际采集／安装／旧链退役仍待第10／13／14步。

版本41新增EntityNPC142／Delirium143及N0—N6／DL0，三组真实deque使用20B头、4／8B元素，schema与生成器显式识别storage=deque。ReadDequeInto在合法主线程窗口无分配归并到预分配区，引用暂存立即转当前身份；双辅助ANM2三投影保持。六项绘制标量、真实EntityRef／复合引用及两组点列进入布局，Delirium AI计数与其他已核Host输入留Host，块18空预留。NPC本类223B／根586B与Delirium根597B未知及Pathfinder SDK声明边界保持；公开44个唯一成员显式回映射。普通Release产品编译／链接通过，没有运行测试，实际接线仍待第10／13／14步。见docs/design/binary-state/implementation-4-npc.md与npc-evidence.md。

版本42按docs/design/binary-state/npc-synchronization-review.md修正：Delirium两Boss选择值归Host并撤销shape，17／18空预留；ControllerId只换算0/1/2受控UI表现，不发送或安装本地设备键，原公开raw查询归Host；新增144号NpcOverlaySprite仅排除由主Sprite null-frame绘制前覆写的Scale。N0为25B，N4最小238B，两NPC最小均710B；未知586／597B与Pathfinder SDK声明保持。来源谓词未核定必须Gap，正式reader/adapter仍待第10/13步；本轮不运行测试。

当前版本43闭合NPC受控UI来源：Manager+4AFB8是原版NetplayManager，8E7550为IsIdxLocalPlayer，版本42“本地设备可用性”解释错误。C/S仅用F1C哨兵输出0/2，已实现state/npc_presentation.h/.cpp的ReadNpcControlledUi／DecodeNpcControlledUi；去掉特殊来源未决资格，枚举拒绝1，不读取原版Online或调用getter。线上大小与UNKNOWN不变；正式窗口/绘制接线仍归第10/13步，未运行测试。见docs/design/binary-state/npc-control-ui-evidence.md。
