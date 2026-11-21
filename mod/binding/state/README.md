# 原生二进制布局源

**2026-11-18 平铺布局，包129**。[实施记录](../../../docs/archive/design/binary-state/10-flat-record-encoding.md#91-包129布局源与生成器)。首批域源在原生 range、合成字段或伴随字段上声明 `sequence`。实体和 Grid 的根声明 `flat: true`，派生类和变体继承该标记。Player 控制组也声明平铺根标记。

`write_groups` 以字段名或内嵌叶路径登记联写依赖，具名情况附 `handler`。`conversions` 可以用 `{semantics, fields}` 声明跨字段输入，`validators` 可以用 `fields` 登记参与联写的其他字段。生成器据此生成 `state_flat.inc`。版本 84 的现有 codec 兼容定义在 [legacy-block-layout.json](legacy-block-layout.json)，未迁移域继续使用块。下方记录保留原版本时点。

**2026-11-13 当前版本74**：版本73已加入438—453的第二批状态／事件；74将MusicRequestState.mode限定为0—5，5表示Queue自然接续，不能当外部静音Play。音乐布尔和六位layer约束同源生成；来源、主线程事件范围／worker接管及未接通的正式运行链见[Host实施记录](../../../docs/archive/design/binary-state/implementation-10-host-display-capture.md)。第15步仍唯一整体验收，未运行测试。

**2026-11-12 版本72：按需收缩与D1—D8入源**：`demand.json` 是独立需求输入，不加入 `version.json.domains`。生成器先保留完整域源参照，再在内存中剥离未选wire；发送计划、正文编解码和校验器只包含需求字段，空块保留编号、空投影省略。原生字节账本、公开成员回映射与所有类型文档仍全量生成。`Layout.complete` 改为需求覆盖，不表示原生未知已清零或运行已完成。覆盖入口见[生成需求表](../layouts/demand-coverage.md)。Release／Win32产品DLL纯编译与链接通过，没有运行测试。

需求清单有两表：`all` 的每项为 `{"type":"类型名","reason":"消费方或依据"}`，保留该展开类型（含继承）的既有wire；`fields` 的每项为 `{"type":"类型名","fields":{"投影前字段名":"消费方或依据"}}`，只保留列出的量。变体使用生成后的完整类型名单独登记。类型或字段错名、无wire字段、重复类型或A/B重叠、缺嵌套／动态槽候选、缺条件选择器均在构建时报告。未选择的新类型不会自动上线；不能通过专用旧表恢复其正文。

D1—D8已登记Q8 Game／Room当前值、HUD可见性、两处raw诅咒、SpawnSeed、种子效果有界ID列表、Q11九个转场值、Q12 paused／终结分支和SFX当前乘数（新布局437）。Game相机偏移、effective_curses、Error谓词与旧最终显示结果不在发送清单中。Room显示子结构默认照旧，但明确退役的Backdrop binding／FX scene等结果companion不保留。`presentation_envelope`、`resource_protocol`、`hud_local`专用表暂用完整参照生成，供后续迁移前编译；原生采集／安装处理器、Q12分支计算、Seeds原生树和音频dirty实际接线仍待第8／10／13步。

**2026-11-12 RE-5／RE-6 静态补注（仍为版本71）**：[已知 Render 写集](../../../docs/archive/design/binary-state/client-display-delegation/RE-5/write-set.md)补入 Entity 屏幕坐标、Room LightGradient scale／坑纹理 dirty、Game resize／Render 反馈和 PlayerHUD 包围框的本地职责，并明确 Tear 暂存的是主 Sprite ColorMod。只改已有范围的语义／证据文字，不改变类别、块、wire、处理器或互通接受合同；没有生成、构建或测试。版本72的按需收缩及旧结果／相机偏移退役仍待实施，不把这些文字补注当成运行路径已迁移。

生成布局文档按总计划步骤存放在 `mod/binding/layouts/02-base-types/` 至 `08-run-presentation/`；总索引与各步骤索引由生成器同时维护。目录号是计划步骤号，不是布局编号或版本号；变体随其所属域，共享类型不重复生成。第7步已登记的显示／资源结构仍随第7步归档。布局源中引用的 `layouts/EntitySaveState.md`、`layouts/GridEntityDesc.md` 旧证据路径保留跳转页。

## 版本56：第8步Run归属

`run.json`新增230—238：RunSeeds／RunAmbush／RunChallengeParam／RunDailyChallenge／RunScoreSheet／RunColorModState／RunGameStateFlags／GameRun／RunSeedEffectNode。证据见`docs/archive/design/binary-state/run-synchronization-review.md`。GameRun只是Run视图，跨域委派与原生孔不冒充已核或本地；191条目录保留原行序、签名与旧归属说明。只传有实际显示用途的当前值、7项种子显示选择、ascent bit48和最终分数，RNG／波次控制／回溯／生成历史留Host。新handler尚待第10／13步；第15步前无测试。

## 版本54：GridEntityPlus外部水面来源

`grid.json`新增229号12B本地ABI表（vptr4、payload4、engaged1、已核对齐3），无发送块；199的4B `water_clip_flags`仍为最终结果。官方1.1.3 DLL表RVA62E650、448项、stride12，来源／调用ABI／Factory与Init重置及共享生命周期见`docs/archive/design/binary-state/grid-plus-water-source-evidence.md`。六族应用nullable覆盖，Lock忽略它并按Host collision合成；现有六C/S wrapper不足，第13步须补Lock第七点。无复制optional、读未设置payload、写官方旁表或回放Set／Reset；各Grid根大小和UNKNOWN不变。实际运行绑定／reader／writer未接线，全119项逐行回映射未完成，第15步前无测试。

## 版本53：PressurePlate／Door

`grid-classes.json`追加224非随机Light描述、225Door主颜色、226Door主Sprite、227PressurePlate、228Door，保留此前202—223。证据见`docs/archive/design/binary-state/grid-pressure-door-evidence.md`。两类G0用12B描述（不发无消费者的临时Light种子），公共198/其它类不变；Pressure主202/Timer201，Door主226/extra201。Door颜色只传1B alpha正值入口，其余从同world墙色取得；+28C只传bit0x20布尔，target类型/i32索引/ExtraVisible保留，previous/动画名/玩法控制值及RNG留Host。Door2EC为string capacity，27F未知；根31/32B，最小212/223B，Release状态库纯编译通过，无测试。17派生均已建表，完整目录与正式owner仍待后续。

## 版本52：第6步指定七类

`grid-classes.json`新增214—216专用视图、217—223实际Lock／Pit／Spikes／Statue／TNT／Poop／Rock。证据见`docs/archive/design/binary-state/grid-seven-subclasses-evidence.md`。TNT的`_frameCount+154`等计时、Poop40B EntityRef、Statue尾部Entity*均归Host；Statue自身无Sprite负载但原生UNKNOWN仍保留。Rock只在可绘碎块态采RubbleAnim资源和有序Vector，主姿态／layer0位置以父state和层序号控制存在性，覆写残留不读不发，其余独立姿态与原色保留。条件／具名纯值嵌套不再降为无条件memcpy。

七根UNKNOWN分别20／20／17／17／17／19／17B，完整标志仍否。Release／Win32状态库纯编译通过；具名reader／writer／绘制与生命周期owner尚未实际接线，Rock多pass overlay最终来源仍须明确。未运行测试，第15步唯一统一验收。PressurePlate、Door及全119项回映射仍待完成；以下段落保留历史时点。

## 版本51：第6步指定八类

`grid-classes.json`登记202—213：地板色主Sprite视图、Fire粒子主动画／模板／元素，以及Decoration／Gravity／Stairs／Teleporter／Wall／Web／TrapDoor／Fire。字节证据与实际同步决定见`docs/archive/design/binary-state/grid-eight-subclasses-evidence.md`。

`native_view_of`保留原类型全部原生字节边界、名称和UNKNOWN；真实派生类型可用`nested_views`将父类的直接等大嵌套字段选择为该明确视图，块号及父子原生布局不改。这是生成时完整字节模型，不允许覆写父类范围或把指针改成copy。五类202不发必覆写的44B颜色，Wall／Teleporter／Fire仍保留；Fire203不发逐粒子SetFrame必覆写的主帧／两组选择器，仍发is_playing和真实主资源，overlay不裁。后续reader/writer必须按nested_layout递归，不能硬编码公共布局号。

TrapDoor计数、Fire计时／样式／粒子速度归Host；粒子原生24B、线16B，普通完整窗口最多5条，state>=4投影空，age/maxLife继续供原生绘制。七类根未知17B、Fire45B，最小98／142／372B；不是整个第6步或运行接线完成。Release状态库纯编译通过，无测试，第15步统一验收。

版本50按`docs/archive/design/binary-state/grid-common-synchronization-review.md`收紧`grid.json`：Grid199的+3C碰撞原值归Host，G0改用4B synthetic最终水面值；GridLayerState200不发路径、GridSprite201不发默认名且增加1B synthetic故障纹理选择。G1固定94B，公共最小状态142B，根UNKNOWN17B不变；合成值没有原生偏移，不能落入memcpy或直接写游戏对象。通用ANM2／其它Sprite原布局保持各自合同。所有具名来源／安装／绘制handler仍待正式接线；此前版本48段落保留原时点。 冠军参数只传+D4前四float16B，+E4—FF七值留Host；其它ColorMod不裁剪。

版本48新增`grid.json`：Desc198的四项展示输入为16B，公共Grid199为`base_prefix`，G0—G3最小状态171B。340B公共前缀递归保留17B UNKNOWN（Desc3B＋ANM2 14B）；SpawnSeed有Light展示消费者，对象_rng及来源Init／玩法元数据不上线。子类、Plus水面旁表、119项全目录与实际窗口／安装owner尚未接入；本轮`public_owners=[]`不宣称全域覆盖。证据见`docs/archive/design/binary-state/grid-common-evidence.md`。这不改变前轮Player等布局内容，重新生成的旧表仅更新整数布局版本。

`version.json` 指定整数布局版本、域文件和公开成员核对目录。`schema.json` 描述域文件结构；`gen_state_layout.py` 在普通构建中执行结构与跨类型语义检查，同时生成代码表和 `mod/binding/layouts/`。不运行游戏或测试，不计算哈希。

布局源任何改动都必须手工递增版本。版本 1 是首次创建，尚未用于网络握手；第 9 步接入握手后，两端直接比较整数版本。版本是 u16 正整数，与帧头 `layout_version` 同宽。

- `ranges` 按原生偏移排列；子类只写父类大小之后的范围，生成时展开父类，完整覆盖 `[0,native_size)`。未知区必须具名，不能自动补为填充。标量和 `bytes[N]` 类型标签的宽度必须等于 `size`。
- 独立RTTI但无尾部成员的等大派生类（Dummy）允许与父类相同大小；此时子类`ranges`必须为空，不能覆盖父类字节归属。
- `storage=c-string`是最多256B的内联零终止字节串，不是MSVC string头或复杂内联数组。u16线上上限必须为原生容量减一，必须声明真实写入资格；线上不含NUL，Host不读取终止后的尾部，Client暂存时验证无内嵌NUL并补一个终止符。
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

版本27续研第4步原未决前四项：Knife尾部六个f32组成K4，具体分裂／碰撞用途已核，+518更名为碰撞方向角；Slot进度对象指针与Projectile回调指针归本地；Effect传播／生成参数的专用用途和Host归属已核。六段4B空白按全映像候选及间接成员边界核对，尚未把填充假设写成已核事实。依据和剩余问题见 `docs/archive/design/binary-state/implementation-4-first-four.md`。

版本28按 `docs/archive/design/binary-state/synchronization-field-review.md` 修订同步职责：Knife K4六值／命中父链选择、四类命中历史、Projectile伤害／三项延迟变更参数、Laser掉落概率／伤害倍率共17个范围改归权威，不再有wire或线上validator，相关公开成员回映射改为authority；清空的块显式reserved，后续块号不变。Familiar充能／状态／硬币等真实展示输入保留，Weapon充能比例的跨域展示缺口明确记录。其余候选不因主Render没有直接位移命中就批量归权威；原生范围、证据等级、UNKNOWN及正常Client owner生命周期不变。

版本29按 `docs/archive/design/binary-state/internal-entity-relations-review.md` 核对24项内部关系：Knife4／Tear6／Pickup1／Laser12共23槽权威；Familiar+DAC只在新增variant240／布局130传真实引用，其余Familiar臂留Host。Knife K3保留空块号，公开关系和必要本地撤边保留。240绳索payload仍待补齐；HomingLaser unoptimized已证实由Room Light消费，当时保留四容器，版本37续研后只保留两组最终点列。上述版本28段落为前次修订范围，不是当前版本号。

版本30按 `docs/archive/design/binary-state/entity-common-synchronization-review.md` 复核Entity公共部分：33个范围撤销wire、专用读取资格和线上validator，28项公开成员改为authority。32个Host玩法范围包括20个公共计时器／毒火伤害相位／两组历史位置及伤害物理输入；removedByFactory归本地工厂生命周期。valid／exists／dead、碰撞类别、尺寸及有Effect展示用途的速度倍率继续保留。E3仍有两项缩小状态，块号不变；每个Entity最小负载减少223B，未知和证据等级不变。旧运行路径在第13／14步统一切换。

版本31按 `docs/archive/design/binary-state/entity-lifecycle-synchronization-review.md` 撤销公共valid的wire／读取资格／validator及具名handler，原生字节归本地构造／发布／池／退休owner；Entity最小状态由411B降为410B。Player活动／备用身体的独立激活资格仍由第5步P7结构接管，不把valid猜成room_member，不在E2重复发送；新结构编码／writer尚未实现。exists／dead继续同步，已有Exists=true但valid=false且非列表身体的历史反例，以及Kill／Remove与退休分离的静态证据；UNKNOWN不变。

版本32按 `docs/archive/design/binary-state/animation-resource-synchronization-review.md` 排除AnimationState两组事件位、ANM2／LayerState loaded原字节及Entity ColorParams整个队列。AnimationState块3、ColorParams块0、Entity E7显式reserved；原生语义／UNKNOWN保留。file_name改为可空资源选择，声明`anm2_selected_file_to_resource/from_resource`，保留资源有无与本地准备失败的区别，实际执行器仍归第8／10／13步。Layer路径与实际Image独立，Host待重载但保留旧图时不得按路径抢先加载。is_playing／anim_frame／playback_speed因官方InterpolV2真实绘制消费者保留；最终ColorMod仍完整11个f32。撤销颜色锚点／线上validator／读取时钟及七项颜色公开成员的状态回映射；Entity最小状态410B→391B。没有构建或测试，生成表仅作当前布局输出。

版本33按 `docs/archive/design/binary-state/familiar-synchronization-review.md` 将Familiar11个轨道／计数／方向范围共48B归Host，撤销wire／读取资格／validator及公开成员状态回映射；FireCooldown／State／Coins／IsFollower等展示依赖保持。新增80布局131，只按16个已核原版Weapon动态公式输出i32最终充能amount（0—100），Client具名writer仅存本地表现缓存，不写DB0或创建玩法Weapon。240布局130追加133号UmbilicalBabyData四投影，节点132按逻辑deque顺序只发XYZ（12B／节点，原生0x78）；Sprite完整三投影保留。`storage=array`表示专用reader归并后的顺序输出，不表示原生deque连续。非空payload节点0、超限、非有限值、错类型、来源变化及各投影count不一致均不能发布。根对象UNKNOWN不变，payload含58B未知、外部每节点另108B未知，继续如实保留。Familiar最小613B，80为617B，240缺失payload为623B。来源／布局已补齐，具名reader／展示adapter／本地owner仍待第10／13步；没有构建或测试。

版本34按 `docs/archive/design/binary-state/knife-bomb-tear-synchronization-review.md` 排除Knife10／Bomb11／Tear21个攻击、运动与计时参数范围，原线负载合计减少181B。撤销wire／读取资格／validator／公开成员状态回映射及Bomb ExplosionCountdown、Tear WaitFrames／StickTimer三个计时锚点；原生类型、证据等级、UNKNOWN和Host观测职责保持。Bomb B0及Tear T0／T2显式reserved，块号不变；保留Knife拖影输入、Bomb五份costume／绘制开关及pending清除、Tear效果位／DeadEye与三份辅助Sprite。最终公共PositionOffset／Sprite变换独立同步，不在Client执行原生Update／Interpolate、ResetSpriteScale、GetRenderHeight重算或LoadCostumes。最小状态Knife422B／Bomb1127B／Tear854B，未知497B／441B／478B不变；运行时待第10／13／14步，未构建或测试。

版本35按 `docs/archive/design/binary-state/pickup-slot-synchronization-review.md` 将Pickup12／Slot8个原生范围归Host，撤销19项原线上字段／容器（CycleCount原已由容器派生，不重复扣除4B）。选择组／触碰／自动价格／商店标识、机器状态／捐款／触发／奖品输入及未来循环队列不采集或安装；Pickup Timeout／Wait两个锚点及读取资格撤销。Pickup Price因WaterClip、DropDelay因Render正值隐藏继续保留；所有主／辅助Sprite与真实引用保持。Pickup P3、Slot S0／S1显式reserved；Slot+460／+470是奖品动画指针／is_playing，继续由ANM2投影供应，不误作机器状态或loaded／visible。Client本地循环初始化为空，不运行Morph、价格重算、购买／捐款／开奖或SetPrizeCollectible。最小Pickup703B／Slot538B，未知448B／363B不变；未构建或测试，运行接线仍待第10／13／14步。

版本36按 `docs/archive/design/binary-state/projectile-synchronization-review.md` 将Projectile的FallingSpeed／FallingAccel、HomingStrength／CurvingStrength／Acceleration、WiggleFrameOffset及Scale／ProjectileFlags八项共36B归Host，撤销wire／读取资格／validator／状态回映射，PR1显式reserved。Height因659460临时绘制偏移直接消费而保留；公共EntityFlags、最终Position／Velocity／PositionOffset／尺寸／Sprite、DepthOffset单份映射及Game展示帧保持。draw scope须精确恢复+350，不依赖浮点减加逆运算；不运行Update／Interpolate／碰撞／+470回调。私有+440开关与+460／+464位置的Render事实已写入原UNKNOWN语义，生产／生命周期／布局仍未闭合，375B未知不变，不据此称画面完整。最小Projectile395B；未构建或测试，具名接线和第15步验收仍待实施。

版本37按 `docs/archive/design/binary-state/laser-homing-synchronization-review.md` 保留optimized／unoptimized及直接展示标量，十九项Laser更新输入、六项Homing标量、indices中间缓存和Bezier／系数留Host，撤销两个Laser计时锚点。indices原生类型改为8B双f32点对，旧i32／4B步长记录错误。Homing子块0／3／4与Laser L2／L4／L7／L8显式预留，父只投影子块1／2；生成器要求全部实际发送子块各出现一次，空预留子块可省略、不改编号，兼容既存空投影。Bezier不再声明线上pure_value。两组独立count≤16384、有限点、同目标版本安装、空集合清除、正常本地self／vector owner及析构保持；不调用采样／更新／半帧／接触模拟。最小Laser542→439B，Homing40→4B，Bezier24→0B；421B未知不变。未构建或测试，运行时仍待第10／13／14步接线与第15步统一验收。

版本38按 `docs/archive/design/binary-state/effect-synchronization-review.md` 修订Effect六臂及payload：六项公共参数、TearFlags完整128位及136描述八成员归Host，撤销wire／读取资格／validator／公开状态回映射；FX2全部reserved且布局号不变。保留真实展示参数与Timeout锚点。MrMe原16B未知已核四项Host内部状态；Trail节点由606190明确分配／复制0x40，撤销base_prefix限制。Ghost新增134专用ANM2视图，仅Offset／Scale归派生，通用ANM2不变；布局28改为inline_count=4但只发有效前缀，三投影u16 count一致且≤4，原i32 count派生。原生目录始终展开四槽，56B未知保持。公共Effect444→420B，Ghost空前缀格式下界1037→427B；未构建或测试，运行时待第10／13／14步。

版本39按 `docs/archive/design/binary-state/player-equipment-plus-synchronization-review.md` 审查Player／HUD／装备及Plus：ActiveItemDesc两项留Host、六值24B；Weapon五项留Host、四值16B，不再同步重复chargebar／MaxCharge，原20B未知和base_prefix保持。135号Whip实际0x48另发+44真实主引用；136号TemporaryEffects保留disabled与完整列表，cache mask／evaluate留Host、Player回指本地，外层2B未知与每项4B未知另列。TemporaryEffect三值10B保持；KnifePlus hitboxSource留Host、块1reserved，正式专用身份reader删除，公共两nullable9B保持。Familiar原D76字节有Render／Shadow正面消费，新增1B状态、根未知905B；其他已排除项及未知不恢复。本轮只静态修订和生成，未构建或测试，Player／HUD／reader／writer仍待第5／8／10／13／14步接线。

版本40按 `docs/archive/design/binary-state/effect-next-five-evidence.md` 完成Effect后续五项静态处理：137—141为传播／Timer／生成／计数／113具名原生臂，102保留54／126／167，全部FX2预留；未知字节不抹除。Rotation／Scale移FX0后为16B，FX1为13B；只需整数版本兼容检查，帧格式不改。Trail正常Update尾部裁到count<128，计划上限127，节点块最大8130B，0／1节点合法，超限不截断；其他容器上限不变。Timeout已核101可双减、141刷新，仍每窗重算到期帧。原生ExtraData RTTI只有既有三种具体类型，非附属非空holder或错RTTI须报Gap，不造fallback。普通Release／Win32产品编译与链接成功，没有运行测试；实际采集／安装／旧链退役仍待第10／13／14步。

版本41新增EntityNPC142／Delirium143及N0—N6／DL0，三组真实deque使用20B头、4／8B元素，schema与生成器显式识别storage=deque。ReadDequeInto在合法主线程窗口无分配归并到预分配区，引用暂存立即转当前身份；双辅助ANM2三投影保持。六项绘制标量、真实EntityRef／复合引用及两组点列进入布局，Delirium AI计数与其他已核Host输入留Host，块18空预留。NPC本类223B／根586B与Delirium根597B未知及Pathfinder SDK声明边界保持；公开44个唯一成员显式回映射。普通Release产品编译／链接通过，没有运行测试，实际接线仍待第10／13／14步。见docs/archive/design/binary-state/implementation-4-npc.md与npc-evidence.md。

版本42按docs/archive/design/binary-state/npc-synchronization-review.md修正：Delirium两Boss选择值归Host并撤销shape，17／18空预留；ControllerId只换算0/1/2受控UI表现，不发送或安装本地设备键，原公开raw查询归Host；新增144号NpcOverlaySprite仅排除由主Sprite null-frame绘制前覆写的Scale。N0为25B，N4最小238B，两NPC最小均710B；未知586／597B与Pathfinder SDK声明保持。来源谓词未核定必须Gap，正式reader/adapter仍待第10/13步；本轮不运行测试。

版本43闭合NPC受控UI来源：Manager+4AFB8是原版NetplayManager，8E7550为IsIdxLocalPlayer，版本42“本地设备可用性”解释错误。C/S仅用F1C哨兵输出0/2，已实现state/npc_presentation.h/.cpp的ReadNpcControlledUi／DecodeNpcControlledUi；去掉特殊来源未决资格，枚举拒绝1，不读取原版Online或调用getter。线上大小与UNKNOWN不变；正式窗口/绘制接线仍归第10/13步，未运行测试。见docs/archive/design/binary-state/npc-control-ui-evidence.md。

版本44接入entity-text-dummy.json：Text145／variant2缩放146／Dummy147；Text只发有效原始字节前缀与RGBA，counter仅146发送，duration／此固定字体不消费的颜色空间标签留Host，通用KColor20B不变。新增c-string容量／资格声明与ReadCStringInto／StageCStringInto；等大派生仅允许空ranges。Text339B／Dummy335B未知及Dummy历史屏幕cache依赖／自然来源缺口保持。实际窗口／writer／draw接线仍归第10／13步，第15步前不测试。见docs/archive/design/binary-state/text-dummy-evidence.md与implementation-4-text-dummy.md。

版本45按docs/archive/design/binary-state/projectile-effect-residual-evidence.md收窄剩余：Projectile+440原始u8开关及+460冻结Vector共9B状态，PR0保持4B、PR2新增9B，最小404B；+430轨迹参数／+444动画缓存／+468父速度历史留Host，根未知350B。Effect148独立167（430伤害快照归Host，434及438后8B未知），149处理19/71/194内部flags，不扩大公开API资格；119的430—437仅在6FC690回调下核为DamageFlags，通用未知保持。三段41C/44C/4BC和其余空白不升级填充。此前五项不重做，正式接线／唯一验收仍归第10/13/15步。

当前版本46按docs/archive/design/binary-state/delirium-residual-evidence.md处理Delirium原未决：BD8—BDF和BE0—BE7改为两个Host u64打包存储；官方AttackID／Cycle仍u32、Angle仍u8，SDK编译断言固定其宽度。BDD三字节是已核五u8拼装高位，BE4四字节是实际双字SHRD读取的一部分，不再独立未知；BD4四字节仍UNKNOWN，根未知597→590B。非412臂不读取这36B，不能误称所有非412 NPC都无用途；池复用保留值有静态可达路径但未实测。4A13B0创建的companion是独立成员，BE8继续归Host，不预约／安装／级联Remove；最小710B及17／18空预留不变。正式接线及唯一验收仍归第10／13／14／15步，未运行测试。

## 版本47：正式Player源

`player.json`登记150—197共48个布局；Player197继承Entity，P0—P8与Sprite三投影共25块。`docs/archive/design/binary-state/implementation-5-player.md`说明职责、预算、生成与后续接线；设计ledger／member-map不作为生成输入。Player根UNKNOWN3707B、Plus9B保持。

- `native_type/native_count`只展开原生内嵌字节，包括不上线的保存体；父范围的Host／本地用途约束子字段，未知仍保留，不伪造线字段。
- `fixed_count`必须等于真实inline_count及limit，线上不附u16 count；普通复杂容器仍按各投影count及max_bytes限制。
- `storage=aggregate`必须声明stride=0：没有一对一原生元素，只能由具名官方成员owner归并成已声明纯值结果；不得把结果pair宽度当原map节点步长。
- `kind=choice`用于已声明slots-mask动态槽，生成允许的view_layout集合；每个view一个值块。类型、空槽、Modifiers mask及Whip真引用不能按指针地址猜测。
- `wire.when`依赖同块先行无条件定长字段及具名谓词；条件不成立时不读取负载，也没有该段线上字节。谓词语义在conversions声明，后续reader／decoder注册实现。
- `synthetic_fields`是无原生偏移的C/S自有值；`extent=protocol`只允许native_size=0、无父类及无ranges，不分配游戏对象。`block_sequences`只调整本类继承块的序列。
- `record_limit/body_limit`生成总预算元数据；Player为32MiB／64KiB，body超限改ordinary，不能截断投影。
- `catalog_rows`以目录＋一基ordinal保留全部1265行及原签名（含null／空串），`catalog_aliases`保留1244条Entity目录逐行链接，`supplementary_rows`保留14 native／77旧宏／6结构。原目录保持原事实，不把回映射当可执行getter字典。

生成表中的新handler、条件、mask及预算只是正式接线元数据，不能代替第8—14步owner实现。通用12／13／14／135／136继续保留原合同，不冒充Player专用179／180／181—194。第15步前没有测试。

## 版本57：第7步生产源与生成

`room.json`／`level.json`新增239—309共71个布局，四域原stream目录597行全部登记并生成。实现与当前边界见`docs/archive/design/binary-state/implementation-7-room-level.md`；设计ledger／member-map不是构建输入。Room13个world块，L0=22B、L1=53B、L2=1—12B、L3=1014B、D1=1B、配置15B。Room直属UNKNOWN28B、静态内嵌递归613B，Level22B，指针后动态payload及外部owner未知另计。

生成器支持零步长的自有复杂结果列表、带正式target的合成资源引用及显式u8／默认u16计数；`Field.count_bytes`为0／1／2，不改变既有固定槽格式。旧stream目录只在生成器内存中适配原返回类型／owner／domain，按四个source_schema范围登记，不扩大其他owner的覆盖声明。新资源空间为`room-config-visual`和`shader-table`。

Resource Context的头与typed固定值已登记，完整payload／blob／recipe调度、具体GPU枚举／leaf ABI和实际缓存／reader／writer仍归第9—13步；不能把表里的handler名称当成已绑定运行实现。Release／Win32状态库纯编译通过，第15步前不运行测试。

## 第7步影子描述视图（布局版本保持57）

正式生成器同时产出`state_room_view.inc`：Room／Level／Descriptor／配置的52个语义角色映射、必需块、真实Grid／Door布局及L2动态帧bit／上限均取同一生产源。消费者是`client_room_description.h/.cpp`，只接完整已递归校验的影子记录，持有自有不可变负载；不执行Lua／SDK／网络调度或原生安装。接口和构建情况见`docs/archive/design/binary-state/room-level-description-implementation.md`。没有改线字段或清除UNKNOWN；第15步前不运行测试。

## 版本58：Presentation与Random正式源

`presentation.json`登记74个布局，`random.json`登记24个Host／本地事实视图，共310—407。共享RNG8补17条目录行，其他基础类型不重复；141条Presentation与54条Random目录按原行完整生成。地图复用300—303，D0为356，逐用途随机alpha为357／358；各控制组选择364及其他最终结果见`docs/archive/design/binary-state/implementation-8-presentation.md`。

`presentation_contract`跟随结果布局声明owner／subject、正常producer／adapter、world/body序列、窄依赖／共同提交、有效／失效条件、贡献身份与来源Gap。生成28条`PresentationContracts()`及数字`PresentationDependencies()`；这些是接线声明，不是已安装Hook。资源空间新增`audio-table`，完整资源payload、批次／贡献／事件外层继续在第9步登记。

可见心格353与Stat行354使用条件payload，未知／隐藏不读旧值；body组整体64KiB及world32MiB等既有总预算不变。Random无发送块，原生UNKNOWN／弱证据／模型边界保留。无测试、注入或部署，运行接线与唯一验收分别在第9—14／15步。
