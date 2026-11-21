# 4：Entity 子类

**2026-11-12 显示委托修订**：

- 各子类按需求清单重标，只保留可见字段。
- Effect 各变体（E32）收窄到可见状态。
- 原先按展示时钟还原的子类计数（Familiar FireCooldown、Pickup DropDelay、Effect Timeout 等），改为在安装点按 `Game+0x25A70` 还原。
- 见[修改方案](client-display-delegation.md)第 3、5 节。

本次只修订文档，未改代码，未运行测试。

**2026-11-09 当前版本46：Delirium未决事项。** [静态证据与处理](delirium-residual-evidence.md)将BD8—BDF、BE0—BE7核为Host的两个8B打包存储，原BDD三字节及BE4四字节不再独立未知；BD4—BD7的4B仍无填充证明。根未知597→590B，普通NPC仍586B，两臂最小线状态仍710B、17／18空预留。池槽残留已有静态可达路径，未作实机观察；补核4A13B0的独立companion创建与BE8登记，当前展示依赖闭合为独立成员／实际线上关系，BE8继续留Host。正式接线仍归第10／13步，第15步统一验收；下文旧版本记录保留各自时点。

**2026-11-09 版本45：Projectile展示来源与Effect残余。** [J273静态证据与逐臂结论](projectile-effect-residual-evidence.md)闭合Projectile+440开关、+460/+464冻结绘制锚点的构造／Init／生产链，新增9B独立PR2，PR0保留4B Height、最小状态404B、根未知350B。Effect新增148号167臂与149号19/71/194内部flags臂，167的+430核为Host生成伤害快照；+434及后8B仍未知。119尾部确认一个callback的8B DamageFlags特例，通用臂仍保持未知。41C／44C／4BC没有语义或填充证明；此前五项不重做。普通Release／Win32产品纯编译与链接通过，未运行测试；正式接线仍按第10／13步。

**2026-11-09 版本44：Text／Dummy布局与字段职责。** [静态证据及同步决定](text-dummy-evidence.md)与[实施记录](implementation-4-text-dummy.md)接入145号Text、146号variant2缩放臂、147号Dummy。Text只发有效字节前缀／最终RGBA，counter仅缩放臂发送；duration和此固定字体不消费的KColor标签留Host，通用KColor保持20B。Dummy无专有尾部，继承公共状态。新增c-string读取／暂存及等大派生支持；Text339B／Dummy335B未知、Dummy不可见分支的历史屏幕缓存依赖与自然来源缺口保留。普通Release／Win32产品纯编译与链接通过，未运行测试，正式窗口／绘制接线仍在第10／13步。

**2026-11-09 版本43：NPC受控UI来源未决已闭合。** [静态证据与纠错](npc-control-ui-evidence.md)核定8E7550为原版NetplayManager::IsIdxLocalPlayer，撤销上轮“本地设备可用性”的误解。C/S只以NPC+F1C的-1哨兵生成0/2受控表现，不读取原版联机对象；新增ReadNpcControlledUi／DecodeNpcControlledUi，拒绝旧模式1。布局长度、根未知586／597B及其他未决保持，正式窗口／绘制接线仍按第10／13步；第15步之前不运行测试。

**2026-11-09 版本42：NPC同步职责修正。** [逐段复核](npc-synchronization-review.md)取消Delirium BossType／Variant的8B传输与无根据的shape换代；ControllerId本地设备键改为1B受控UI表现，禁止跨进程raw写入；OverlayEffect新增144号场景视图，8B Scale由当前主Sprite null-frame派生。五项原生绘制标量、真实引用、三deque与必要双Sprite保持。NPC／Delirium最小负载均710B，未知586／597B及Pathfinder SDK声明不变；实际控制UI来源／adapter仍待第10／13步，不运行测试。

**2026-11-09 版本41：Entity_NPC。** [实现说明](implementation-4-npc.md)与[静态证据](npc-evidence.md)将NPC142／Delirium143、双辅助Sprite、三组deque及44个唯一公开成员回映射接入生成链。保留真实绘制参数与8B变身shape，AI计数留Host；新增无分配deque读取接口。NPC本类223B未知、根586／597B未知与Pathfinder声明边界保留。普通Release纯编译／链接通过，没有运行测试；实际采集／安装仍待第10／13步。

**2026-11-09 版本40：Effect后续五项未决。** [证据与处理](effect-next-five-evidence.md)补入137—141号VarData臂，原101—106不重排；Rotation／Scale移FX0、Trail上限127。Timeout存在双减和刷新，不再要求全部静止Effect锚点不重发；三种原生ExtraData之外未发现新具体类型。已知归属落源，未知与后续运行接线保留，本批没有运行测试。

**2026-11-09 版本39：Player装备与Plus展示依赖。** [完整消费者与安装复核](player-equipment-plus-synchronization-review.md)将ActiveItemDesc来源池／定时充能冷却、Weapon五项非展示输入及KnifePlus.hitboxSource留Host；保留主动槽六值、Weapon充能三值与类型、TemporaryEffect三值及Plus公共nullable覆盖。新增135号UmbilicalWhip真实主引用、136号TemporaryEffects外层disabled／列表，并补Familiar+D76绘制抑制字节。原已排除Plus项不恢复，其他UNKNOWN保持。源、生成表、正式Plus读取接口和第5／8／10—15步已修订，未构建或测试，运行时仍待接线。

**2026-11-09 版本38（Effect合同继续适用）：Effect各臂与附属payload。** [Effect同步职责复核](effect-synchronization-review.md)撤销六项公共更新输入、TearFlags全128位与136描述八成员；State／Timeout／LifeSpan／IsFollowing／ParentOffset／Rotation／Scale及三种表现payload保留。Ghost只发0—4有效Sprite前缀，Offset／Scale派生、i32 count由投影计数重建；MrMe原16B未知核为Host内部状态，Trail找到64B分配与完整复制。六臂编号不变，FX2均预留；公共Effect最小444→420B。源、生成表和第6／10／12／13／14／15步合同已修订，未构建或测试，运行时仍待接线。

**2026-11-09 版本37（历史范围）：Laser／Homing采样、运动与计时。** [Laser／Homing采样、运动与计时复核](laser-homing-synchronization-review.md)保留optimized／unoptimized两组最终点列与当前角度、长度、完整效果位、原始采样模式、MaxDistance／GridHit、公开EndPoint和+4F0展示分支输入；19项Laser更新输入、六项Homing标量、indices中间点缓存及Bezier／系数留Host。indices原生类型纠正为8B双f32点对，旧i32步长记录错误。L2／L4／L7／L8及Homing子块0／3／4预留，父投影仅1／2；撤销两个Laser计时锚点。最小Laser542→439B，未知421B不变。源、生成器和第10／12／13／14／15步合同已修订，未构建或测试；运行时仍待接线。

状态：部分实施。Familiar／Knife／Bomb、Tear／Pickup／Laser 和 Slot／Projectile／Effect 的布局、公开成员回映射和专用表现已接入生成链，未知字节与计时分支缺口保留，见[前三类实现](implementation-4-familiar-knife-bomb.md)、[中间三类实现](implementation-4-tear-pickup-laser.md)和[本轮三类实现](implementation-4-slot-projectile-effect.md)；NPC／Delirium布局迁移见[本轮实现](implementation-4-npc.md)，Text／Dummy布局与字段职责见[版本44实现](implementation-4-text-dummy.md)，全部已知缺口仍按各节推进。上级：[总计划](README.md)。

## 目标

**版本36修订（历史范围）**：[Projectile运动与展示参数复核](projectile-synchronization-review.md)排除Projectile八项追踪／曲线／加速／下落／摆动／Scale／规则位输入，共36B，PR1预留；保留真实Render Height、公共最终位置／偏移／尺寸和Sprite，最小状态395B。ProjectileFlags与公共EntityFlags分开，后者保持。私有+440开关与+460／+464位置虽有Render消费者，其来源／布局仍UNKNOWN，375B未知不变，不以只剩Height就宣称类型或画面完整。运行接线仍待第10／13步，未构建或测试；当前以版本37源及生成表为准。

**版本35修订**：[Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)将20个原生范围归Host，撤销19项原线上字段／容器及Pickup Timeout／Wait两个锚点；Pickup P3、Slot S0／S1预留，编号不前移。Pickup Price／DropDelay因WaterClip／Render消费者保留，两份辅助Sprite与公开三引用保持；Slot只需最终主／奖品Sprite，奖品动画指针与is_playing继续由ANM2投影供应。Client不定价、交互、循环Morph、开奖、支付或运行SetPrizeCollectible。最小Pickup703B／Slot538B，未知448B／363B不变；本轮未构建或测试。下文旧实施记录保留原时点，当前传输以版本37源及生成表为准。

**版本34修订**：[Knife／Bomb／Tear参数展示复核](knife-bomb-tear-synchronization-review.md)排除Knife10／Bomb11／Tear21个参数范围，共181B原线状态；三个计时锚点撤销，Bomb B0及Tear T0／T2保留空块号。保留Knife拖影和飞行输入、Bomb最终costume／绘制开关与pending清除、Tear效果位／DeadEye及全部辅助Sprite。最终位置／公共绘制偏移和Sprite姿态独立同步，不运行原生Interpolate、爆炸、击退或资源懒重建。原生事实、证据等级和UNKNOWN不变；下文实施小节按原时点保留，当前传输以生成表为准。未构建或测试。

**版本33修订**：[Familiar参数／充能／绳索复核](familiar-synchronization-review.md)排除Familiar11个参数范围（48B）；保留已核FireCooldown／State／Coins／IsFollower展示依赖。80新增布局131的最终充能i32，Host按已核只读公式换算，Client不创建Weapon。240布局130保留真实DAC引用，补齐132号节点XYZ与133号typed payload的Sprite／逻辑节点四投影；共享owner安装使用Familiar的430E40，不混用Effect的606310。布局、证据和计划已补入，运行时采集／展示／owner仍待第10／13步；新增未知和第15步验收照实保留。

**历史修订（版本29）**：[24项内部关系展示复核](internal-entity-relations-review.md)将Knife4／Tear6／Pickup1／Laser12共23槽归权威；Familiar +DAC基础臂亦归权威，只在新增variant240布局130保留引用。同步撤销线上字段／回映射依赖，Knife K3显式预留。Familiar240的UmbilicalBabyData绳索payload已确认有绘制消费者但尚未完整建模；不能因关系已保留就声称该变体完整。Room Light对Laser未优化采样点的依赖已核；版本37续研后只保留optimized／unoptimized两组最终点列。

**版本28修订（历史范围）**：[同步职责复核](synchronization-field-review.md)取代下文版本16—27中对Knife K4、四类命中历史和另外七个玩法范围的传输约定；共17个范围归权威。原生偏移、用途、证据等级和未知区保持。Familiar充能／状态／硬币／跟随标记有间接展示消费者；variant80 Weapon充能结果仍未接线。当前以版本37源及生成表为准，历史实施记录保留原时点。

完成十个 Entity 子类在公共部分之后（0x410 起）的字节归属和状态块，并覆盖现有的专用形态：Text、Dummy、Delirium、Effect136 搬起的格子、拥有型表现等。

## 输入事实

各子类的**字节归属范围**以 J273 `EntityFactory` 池步长为准（构造 `0x6EEE60`，见 [Entity 池静态研究](../tx-04-19/entity-i26-i30-native.md)第 11—24 行与 [Entity 原生静态研究](../tx-04-19/entity-native-static.md)第 13—26 行）；仅池步长本身不等于已证明原生 C++ `sizeof`；本轮Knife／Slot／Projectile／Effect另经删除析构size参数互证，见4.6。SDK 声明大小只作对照；子类字节数按池步长减 0x410 计算。下表已核字段统计是初始 `entity-application.json` 的 `class_native` 口径，当前剩余工作量见 4.6：

| 类型 | J273 池步长 | SDK 声明 | 子类字节数 | SDK 字段数 | 已核字段数 | 已核字节数 | 已核偏移范围 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `Entity_NPC` | 0xF60 | 0xF60 | 2896 | 25 | 19 | 113 | 0x410—0xF1C |
| `Entity_Familiar` | 0xF30 | 0xF30 | 2848 | 11 | 19 | 111 | 0x5F8—0xEDC |
| `Entity_Knife` | 0x520 | 0xE78 | 272 | 9 | 15 | 57 | 0x410—0x4EA |
| `Entity_Bomb` | 0x9D8 | 0x9D8 | 1480 | 10 | 11（另有 NativeBomb 1） | 51 | 0x410—0x462 |
| `Entity_Tear` | 0x848 | 0x848 | 1080 | 22 | 21 | 103 | 0x410—0x7F4 |
| `Entity_Pickup` | 0x6F0 | 0x6F0 | 736 | 12 | 12 | 39 | 0x524—0x59C |
| `Entity_Laser` | 0x568 | 0x568 | 344 | 16 | 30 | 113 | 0x410—0x560 |
| `Entity_Slot` | 0x550 | 0x548 | 320 | 10 | 8 | 26 | 0x410—0x540 |
| `Entity_Effect` | 0x4C0（见 4.1） | 0x490 | 176 | 4 | 13（另有 NativeEffect 1） | 69 | 0x410—0x47C |
| `Entity_Projectile` | 0x478 | 0x470 | 104 | 0 | 14 | 64 | 0x358—0x45C |

- **池步长与 SDK 不一致的四类**：Knife、Slot、Effect、Projectile。对象范围和未知区一律按池步长划定，不按 SDK 声明。
  - **Knife**：SDK 声明 0xE78，池步长只有 0x520（`entity-native-static.md:67`）。SDK 中偏移超过 0x520 的字段在 J273 不成立，本步逐项排除；子类区按 272 字节核对，而不是 2664 字节。
  - **Slot、Projectile**：池步长比 SDK 各多 8 字节。初轮把多出的 0x548—0x54F、0x470—0x477 归未知；续研已分别分离前4B为进度对象指针和回调函数指针，后4B仍未知。
  - **大小与填充分开判断**：初轮只有池步长上界；续研由删除析构size参数核定这四类的实际sizeof与步长相同。Effect构造只写到+0x4B8不意味着sizeof止于0x4BC，也不能证明后4B为填充。只有字节归属得到证明后才改变未知分类。
- **其余专用池**：Text 池步长 0x530，Dummy 0x410（同上研究），按设计中的“布局编号”按虚表区分。
- **Delirium**：`EntityDelirium` 的 8 项公开字段位于 `Entity_NPC` 的 0xBC8—0xBE3，AI 实际使用到 0xBEB，见 4.2。
- **NativeText**：有 2 项，位于 0x410—0x414。
- **Projectile 的公共别名（已闭合）**：`DepthOffset +0x358` 与 Entity 公共字段共用同一对原生访问器；生成布局只在 E4 保存一次，不增加子类字段。2026-11-09 再次复读两组注册和读写指令，见 4.6。
- **现有专用内容**（`client_entity_adapters.h`）：
  - Laser 两组最终几何容器（optimized／unoptimized采样点）；indices中间缓存与Bezier输入自版本37起留Host；
  - Pickup 循环道具；
  - NPC 复合绘制路径（两份 deque）；
  - Effect136 搬起的格子描述；
  - 拥有型表现 MrMe、SpriteTrail、PickupGhost、IsaacsBody、AbyssLocust；
  - Text 的原始字节与 KColor；
  - 各类辅助 Sprite。
- **几个本地指针**：
  - NPC +0xF40 拥有的玩法对象指针，Client 侧要求为空（`client_entity_adapters_native.cpp:46`）。
  - `Familiar.Player` 是裸借用，不是 EntityPtr 布局（同文件 48 行）。
  - Laser +0x4E8，Pickup +0x590／0x594／0x598 是具名引用（同文件 49—52 行）。

## 设计

- **布局编号**：每个子类一个布局编号。Host 按虚表（RTTI）确定布局编号，不只看 `_type`，因此 Text、Dummy 这类特殊虚表能区分开。
- **子类状态块**：按变化频率拆分，例如 Laser 的几何容器单独成块，Tear 的效果位／DeadEye与辅助Sprite投影分开；版本34起不传Tear运动输入和两个玩法计时。
- **附加块**：辅助Sprite、拥有型表现和Laser几何等实际展示内容各自成块，元素布局按第2步处理；版本35起Pickup未来循环队列归Host，不再作为Client容器。
- **专用形态**：Effect136 等沿用现有具名 owner 的语义，只更换数据来源。Delirium 不是原生子类，做成 NPC 的变体臂，见 4.2。
- **核对重点**：子类区间中未知字节数很大，NPC 有 2896 字节，现只核了 113 字节。必须先用 J273 反编译划出构造函数、Init 和 Update 写入的范围，再逐段归类；不能把整段未知区当成填充。

### 4.1 EntityEffect 各臂与附属payload（版本45，2026-11-09）

现行残余归属以[版本45证据](projectile-effect-residual-evidence.md)为准，既有五项依据为[Effect后续五项证据](effect-next-five-evidence.md)、[版本38职责复核](effect-synchronization-review.md)、唯一源 `mod/binding/state/entity-effect.json` 和[完整生成表](../../../../mod/binding/layouts/README.md)。本节取代旧69B单块及2026-11-08草稿的传输决定；原始逆向事实、失败和旧编译结果保留在[初轮证据](slot-projectile-effect-evidence.md)与[实施记录](implementation-4-slot-projectile-effect.md)。版本40纯编译状态见新增证据，未运行测试，采集／安装仍待接线。

#### 4.1.1 输入事实与展示边界

- Effect原生大小0x4C0；Init `5CD070`、Update `5D35D0`、Interpolate `5D1960`、Render `600E60`。池步长与删除析构相互印证大小，不能据此把未知字节归为填充。
- `+420—+43F` 是随variant复用的区域。四种TearFlags与136完整32B GridEntityDesc是已核原生含义；它们当前都没有Client原生展示消费者，归Host。137—141号臂补入传播、119定时器、120生成、30／153计数及113伤害／尺寸基准；这些已核用途不上线。未核变体和各臂空白保持未知，不复制opaque数据；119的`+420`是本进程函数地址。
- State／Timeout／LifeSpan／IsFollowing／ParentOffset／Rotation／Scale有真实Render／Light消费者，继续同步。Height／FallingSpeed／FallingAcceleration／MinRadius／MaxRadius／DamageSource只供Host更新；传播、生成请求和私有vector仍归Host。最终公共位置／PositionOffset、Sprite／图层和独立成员各自同步，Client不调用Update／Interpolate恢复。
- `+4B4/+4B8`为payload／控制块共享holder，只有108／166／191的已核表现payload传输。两字地址本身不发送，经具体RTTI核定后读取外部对象，Client用`606310`正常替换，不能把payload偏移加到根Effect。

#### 4.1.2 十三个布局臂

先按虚表确定Effect，再读公共variant选臂；成员生命期内布局固定，variant变化走形态事务。原布局编号不因线内容缩小而合并或重排。

| 布局／variant | 复用区 | 附属payload与发送块 |
| --- | --- | --- |
| 101 基础，其余未核variant | 32B未知 | 实机 variant 21 可出现对象指针为空而控制块仍存活的 holder。对象指针非空且类型未知仍报 Gap，FX2 预留 |
| 102 TearFlags，54／126 | 前16B眼泪效果位归Host，后16B未知；167已分到148 | FX2预留，不再发BitSet128 |
| 103 GridDesc，136 | 八成员29B归Host，三个未知字节保持 | FX2预留；主Sprite／持物关系展示，Effect和Grid域均不发描述 |
| 104 MrMe，108 | 同基础臂 | FX3—FX5：present＋独立ANM2三投影 |
| 105 SpriteTrail，166 | 同基础臂 | FX3—FX4：present＋有序完整节点 |
| 106 PickupGhost，191 | 同基础臂 | FX3—FX5：present＋0—4个有效Sprite的三投影 |
| 137 Propagation，32／44／45／46／53／204 | 完整32B后继传播输入归Host | FX2预留 |
| 138 Timer，119 | callback本地，interval／remaining归Host，余20B未知 | FX2预留，Client不执行回调 |
| 139 SpawnRequest，120 | Type／Variant归Host，余24B未知 | FX2预留，Client不Spawn |
| 140 DustCountdown，30／153 | +424 u32移除计数归Host，余28B未知 | FX2预留 |
| 141 TearFlags113，113 | flags及+430／+434伤害／尺寸基准归Host，后8B未知 | FX2预留，保留两项TearFlags公开回映射 |
| 148 ChainLightning，167 | flags及+430生成伤害快照归Host；+434常数槽及后8B仍未知 | FX2预留，不套113公式 |
| 149 InternalTearFlags，19／71／194 | 内部flags16B归Host，后16B未知；不开放公开TearFlags API | FX2预留 |

136由Host creator交换真实Grid ANM2至Effect主Sprite。Client保留Player HeldEntity、GetItemState、GetSuplexState、BodySprite／空层姿态及暂时置可见的绘制借用，不由描述重新构造active Grid，也不重演投掷／落地；落地结果由Host作为真实成员发布。

#### 4.1.3 原生字节归属与payload

完整原生单行表见[基础臂](../../../../mod/binding/layouts/04-entity-classes/EntityEffect.md)、[TearFlags](../../../../mod/binding/layouts/04-entity-classes/EntityEffect_TearFlags.md)、[GridDesc](../../../../mod/binding/layouts/04-entity-classes/EntityEffect_GridDesc.md)及三个附属臂；不得以本节摘要替代完整0—sizeof归属。

| MrMeData25：0x0 vptr【本地】 | 0x4 i32目标处理分支【权威】 | 0x8 ANM2独立Sprite【逐字段】 | 0x11C i32暂存碰撞类别【权威】 | 0x120 i32格子序号【权威】 | 0x124 i32动作计数【权威】 |
|---|---|---|---|---|---|

| SpriteTrailData27：0x0 vptr【本地】 | 0x4 deque头20B【容器】按逻辑顺序遍历独立节点指针；不发送proxy／map／容量／逻辑offset |
|---|---|

| EffectTrailNode26：0x0 Vector历史位置【状态】 | 0x8 ColorMod完整44B【状态】 | 0x34 f32宽度／距离筛选输入【状态】 | 0x38 u32累计绘制参数位模式【状态】不是Rotation | 0x3C f32 alpha【状态】 |
|---|---|---|---|---|

节点producer在`5DA3C6`调用`606190`，后者明确分配0x40并完整复制64B，旧“只知前缀／stride待核”已解决。当前点列不能等价推回累计参数、衰减和已退休历史，全部64B保留。`storage=array`指reader归并输出，原生deque不是连续数组。

| PickupGhostData28：0x0 vptr【本地】 | 0x4 ANM2[4]共0x450B【逐槽归属，线上只发有效前缀】 | 0x454 i32 count【派生】由三投影一致的u16数量重建 |
|---|---|---|

Ghost采用[专用ANM2视图134](../../../../mod/binding/layouts/04-entity-classes/EffectPickupGhostSprite.md)：Offset与Scale共16B由`602F4E—602F68`在每次绘制前完全覆写，归派生；其他资源、动画、独立选择器、图层和颜色保持。通用ANM2布局7与MrMe不变。全部四槽仍展开原生归属，未使用槽不发送但每槽14B未知继续保留。MrMe仅其外部16B有新证据，未知30→14B；根对象未知不变。

#### 4.1.4 状态块

E0—E9保持公共块号；所有Effect块属于world。下面是实际格式，不再采用旧ANM2定长动画常数。

| FX0块10：+0 i32 State | +4 i32 expire_frame：Timeout到期帧 | +8 f32 Rotation | +12 f32 Scale；共16B |
|---|---|---|---|

| FX1块11：+0 i32 LifeSpan | +4 u8 IsFollowing | +5 Vector ParentOffset；共13B |
|---|---|---|

FX2块12在全部十三个Effect布局均为空预留，不作为构造必收负载。FX3块13首字节为u8 present，仅首投影带此标记；后续投影不重复存在标记。MrMe尾接ANM2外观／动画／图层，变长部分依真实资源及选择器；Trail FX4为u16 count＋count×64B。Ghost每投影独立u16 count＋对应有效Sprite内容，三计数必须相同且≤4，不另发原生i32 count。Trail的count上限127，节点块最大8130B；正常Update尾部裁剪完成后才可采集。

present=0时各投影为其最短全零编码，Ghost三个count均0。所有附属投影、成员关系与资源在同目标版本齐备后由同一owner一次提交，不逐块换owner。最小公共Effect444→420B，TearFlags460→420B，GridDesc473→420B；其他下界见复核报告，不代表实测带宽。

#### 4.1.5 Timeout到期帧（保留用户已采纳转换）

Render真实消费Timeout，继续每个采集窗从原值重算`A=F+T`，Client以已应用world帧的展示时钟还原`T=A−F`，均按模2^32。只在原生恰好递减的分支锚点不变；负值、冻结、重置和跳变可能使其持续更新。版本40核到101局部递减后仍可进入公共正值递减、141可刷新常数，不将这些路径误作每帧仅减一次。旧“所有Effect无条件逐帧递减／静止必不重发”不是J273契约。Client不在两个已应用帧间自行预测，不运行Update，并在新world帧应用后重写`+45C`；同一时钟用于读回比较。

#### 4.1.6 写入、校验与生命周期

- 只安装FX0／FX1保留字段及真实payload。IsFollowing／ParentOffset依赖同版本公共Parent，MrMe另依赖Target及主Sprite CarryPos；不得因裁剪Host字段删掉绘制依赖。
- 实际线上f32与Vector／ColorMod必须有限；Trail `+38`保持u32位模式。following／present仅0或1；Ghost原生i32 count编码收窄前拒绝负数／>4，三投影计数一致；Trail count≤127，不能截断／抽样或在原生Update追加后、裁剪前采集。真实字节预算保持，不截断。
- 共享payload前后holder／RTTI／生命周期稳定，缺失、读取失败、未知类型分别处理，不能把失败当空。Ghost缩短／归零／再增长经本地ANM2 owner清理旧槽，所有候选失败不得部分替换。
- vptr、控制块、deque头与本地资源由正常owner构造／替换／析构；不memcpy活容器，不放行原生Update／Interpolate、Trail追加／衰减、Ghost选loot、传播／Spawn或格子落地。Ghost派生Offset／Scale与其他draw临时写入须精确恢复。

#### 4.1.7 公开成员回映射

| 公开成员 | 现行归属 |
| --- | --- |
| m_Height／FallingSpeed／FallingAcceleration／MinRadius／MaxRadius／DamageSource | authority：Host更新输入，不编码／安装／比较原值 |
| State／Timeout／Rotation／Scale | FX0；Timeout换算，旋转／缩放按动态字段处理 |
| LifeSpan／IsFollowing／ParentOffset | FX1实际展示输入 |
| GetTearFlags／NativeEffect.TearFlags | 四个适用variant归authority，其余不适用；不恢复FX2 |
| GetGridEntityDesc | 136归authority，其余不适用；不移回Grid域 |
| IsPlayerCreep | 由公共variant派生 |
| setters、FollowParent、CreateLight、CreateLootPreview | 操作，不作为状态重演 |

#### 4.1.8 典型帧

粒子只更新最终公共位置／偏移／Sprite，不重发三个Host下落参数；State／Timeout有实际变化时FX0按规则发送。Trail最终节点增长／缩短重发FX4完整逻辑序列，不能仅发count。Ghost只对当前有效前缀发送各投影，count改变时三个投影同目标版本提交；不把未使用槽的变化当作网络变化。

#### 4.1.9 配套步骤与唯一验收

第6步撤销136重复描述；第10步只采集真实展示输入与最终payload；第12步不等待FX2空块，校验实际count／present／资源；第13步维护同版本依赖、具体owner与draw借用；第14步退役已排除原值、旧writer／validator／漂移比较及lifted_descriptors同步。第15步沿用E09／E25／E32、E13／E17、E29、E28、E34与E27覆盖最终画面、payload前缀与生命周期、拒绝路径、模拟隔离、Timeout及未知，未提前执行。

### 4.2 NPC 的 Delirium 变体臂（设计稿，2026-11-08）

本节取代“Delirium 沿用现有具名 owner 语义”的笼统说法。事实来自 J273 静态分析，没有运行游戏，标明“推断”的未实测。表格都是草稿，实施后以 `layouts/` 的输出为准。

#### 4.2.1 为什么是变体臂，不是子类

- **原生没有这个类**：J273 RTTI 的 `Entity_*` 共 13 个（本步开头表中的 10 个，加 Player、Text、Dummy），没有 Delirium。
- **同虚表、同池**：Type 412 进 NPC 默认池（[entity-native-static](../tx-04-19/entity-native-static.md) 第 23、28 行），虚表就是 `Entity_NPC`，按虚表分不出来。
- **`EntityDelirium` 只是 Lua 接口**：REPENTOGON 的元表以 `ENTITY_NPC` 为 `__parent`，底下仍是 `Entity_NPC*`（`LuaDelirium.cpp:260-262`）；8 个字段就声明在 `Entity_NPC` 里（`EntityNPC.zhl:137-144`）。

稳定type412用143号专用归属臂；其余NPC用142号Host区域。版本46补核：非412的Delirium companion也可能由AI helper消费打包值，不能把“同步不读取普通臂”解释成“原生从不使用”；见[新证据](delirium-residual-evidence.md)。变体选择仍用合法窗口恢复后的稳定type，不用临时Boss类型或池槽旧值。

#### 4.2.2 J273 事实

以下保留初轮静态记录；版本46已纠正BDD／BE4的字节归属及“除Init外只有49F3D0访问BE8”的遗漏，池残留升级为静态可达路径但未实测。现行结论见[Delirium未决处理](delirium-residual-evidence.md)和下面4.2.3—4.2.8。

对 `.text` 扫描了 `+0xBC8—0xBEB` 的全部 disp32 直接寻址。经基址计算的访问看不到；`0x6ED4B0`、`0x903730`、`0x9041E0`、`0x925C00`、`0x98C690`、`0x98C7E0`、`0x98CCC0`、`0x98DDA0` 的命中上下文不像 NPC，未逐个定类。

- **写入者**：Delirium AI `0x49F3D0`、`0x4A1A30`、`0x4A2720`；NPC ctor `0x6B4520`、Init `0x6B4A50`（`Entity_NPC` 虚表 +4）；`0x670200` 在判定 Type 412 后写红色模式位。
- **其他读者**：
  - `0x44EF70`、`0x45A450`、`0x45B130`、`0x474720`、`0x485F10`、`0x4AC220`、`0x4BF700` 这 7 个函数只读 `+BC8`，判断是否为 0 或与 `_type` 比较，用来区分当前是否由 Delirium 代跑。
  - `0x6C9510`（虚表 +0x5C）对 Type 412 用 BossType／BossVariant 代替自身类型。
  - NPC Render `0x6CEC90` 不读这一段。
- **池槽残留（推断）**：Init 只清 `+BC8`、`+BCC`、`+BD0—BD3`、`+BE8`。ctor 只在工厂建池时运行（唯一引用在 `0x6EF009`），且只写 `+BE0`、`+BE4`；两者都没有范围清零。所以普通 NPC 的 `+BD8—BE7` 在池槽复用后可能留着上一只 Delirium 的值。
- **变身不改 `_type`**：`0x49F3D0` 保存 `+0x28／+0x2C`，临时换成 BossType／BossVariant 运行，返回前恢复。tx-04-19 实测也是 BossType 20、Type 仍为 412（[验收记录](../tx-04-19/tx-04-19-acceptance.md)第 27 行）。
- **BossType 有两条改写路径**：变身分支经虚 Init 重配；state 8 分支中，如果 Boss AI 改了 `_type`，AI 直接把新值写回 `+BC8／+BCC`，只重载主 Sprite（`0x49EC80`），不经过 Init。
- **打包字段**：
  - `+BD8` 的四个字节分别是弹幕编号、剩余次数、继续概率、相位计数（`0x4A1A30`），`+BDC` 低字节是 Angle。REPENTOGON 的 `AttackID` 是这四个字节合成的 u32，只有低字节是编号。
  - `+BE0` 被当作 64 位读取，但 `+BE4` 的全部 disp32 写入点都写 0，有效内容在低 32 位。
- **`+BE8` 是 EntityPtr 槽位**：形态 410 时由 Child（`+0x3C0`）写入，序列与 `EntityPtr::SetReference 0x858760` 相同（`0x4C8EE0` 摘旧、`0x4C79D0` 挂新）。除 Init 外只有 `0x49F3D0` 访问，公开接口也没有。
- **变化频率**：state 8 下，AI 每个 tick 都改 `+BD0`（递减）、`+BD8`（相位计数加 1，形态 45、84:10 除外）和 `+BE0`。

#### 4.2.3 变体臂（版本46）

142号EntityNPC与143号EntityNPC_Delirium均已入源，仍先按RTTI确定NPC，再以继承的type字段选臂。type412臂用于保留BC8—BEB的真实字节归属和4B UNKNOWN，不代表它多出Client状态。完整表见[142](../../../../mod/binding/layouts/04-entity-classes/EntityNPC.md)、[143](../../../../mod/binding/layouts/04-entity-classes/EntityNPC_Delirium.md)。

| 布局 | 选择字段type | BC8—BEB归属 | 17／18块 |
| --- | --- | --- | --- |
| 142 | 其他 | 36B Host区域；可含companion AI值或普通池槽残留，不读取／上线 | 两块均空预留 |
| 143 | 412 | BossType／Variant、两个8B打包值及其余AI／关系共32B归Host，4B未知 | 两块均空预留 |

#### 4.2.4 原生归属（版本46）

| +BC8 u32 BossType【权威】 | +BCC u32 BossVariant【权威】 | +BD0 u16 TransformationTimer【权威】 | +BD2 u8 RemainingAttacks【权威】 | +BD3 u8 StateD【权威】 | +BD4 4B【未知】 | +BD8 u64弹幕打包存储【权威】 | +BE0 u64 Cycle打包存储【权威】 | +BE8 EntityPtr companion【权威】 |
|---|---|---|---|---|---|---|---|---|

版本46将BDD的3B核为五个u8拼装的8B弹幕存储高位，将BE4的4B核为原生Cycle双字存储上半字，后者以SHRD参与bits25—34的读取；两者不是源级填充。仅BD4—BD7的4B保持UNKNOWN，详见[指令与边界](delirium-residual-evidence.md)。官方AttackID／Cycle仍是u32、Angle仍是u8；原生打包存储不扩大公开接口。旧草稿13B DL1已在版本41归Host；版本42进一步撤销8B DL0及两个shape标记，当前继续不传输或重演AI，最终资源独立供应。

#### 4.2.5 NPC共有块

| 块号 | 当前内容 | 最小字节 |
| --- | --- | ---: |
| 10 N0 | State、ProjectileDelay、Vector V1、I2、ChampionColorIdx及u8受控UI模式 | 25 |
| 11 N1 | u16 EntityRef＋u16 count＋有序复合引用成员序号 | 4 |
| 12／13 N2／N3 | 各自u16 count＋有序Vector次／主路径 | 各2 |
| 14／15／16 N4／N5／N6 | OverlayEffect场景Sprite144与通用副ANM2的外观／动画／图层 | 238／44／4 |
| 17／18 | 原DL0／DL1空预留，不接收旧负载 | 0 |

| N0：+0 i32 State | +4 i32 ProjectileDelay | +8 Vector V1 | +16 i32 I2 | +20 i32 ChampionColorIdx | +24 u8 controlled_ui_mode；共25B |
|---|---|---|---|---|---|

三deque各≤16384，完整逻辑序列与owner合同不变。ControllerId不作为raw i32传输：版本43只允许0=无控制者、2=受控，模式1无效。来源已闭合为C/S窗口只读F1C的-1哨兵，不读取原版NetplayManager；ReadNpcControlledUi／DecodeNpcControlledUi已落地，Client只存表现旁表，正式窗口与整段绘制条件绕行归第10／13步。+CD8只有Scale由同版本主Sprite null-frame重建，其他资源／姿态及+DEC保持；详见[144号表](../../../../mod/binding/layouts/04-entity-classes/NpcOverlaySprite.md)。普通NPC与Delirium最小均710B，含公共391B，不含帧头与实际变长负载。版本46根未知586／590B，NPC本类223B和其他嵌套未知保持。

#### 4.2.6 资源变化与生命周期

- Delirium的BossType／Variant不属于Client shape；变身仅更新实际改变的资源、姿态、公共最终状态与真实引用，不退休重建同一NPC。
- 只有公共真实shape或其他已证实原生结构字段变化才走第11／13步结构事务；不能把Host AI选择器重新加入shape集合。
- `world_delirium_owner`仍在Init后恢复当前字节读取资格，不能无条件推进shape。采集仍须在49F3D0 owner退尽后，不能用临时Boss type选错臂。
- 835固定前两引用与真实NPC点列、906／920子体Sprite、921次路径、951:3邻点和965:10跨Player依赖保持，缺当前依赖则写前失败。Player+C18绝不按NPC解释。

#### 4.2.7 写入与校验

两臂17／18均预留，Boss选择／AI打包值无writer、读回或漂移比较。N0五个真实原生绘制值按可信字段宽度安装，受控UI模式自版本43只能0/2并存入同key/generation自有表现值，不能直接写本地设备键。OverlayEffect Scale只在已核draw窗口派生并恢复；不得将该规则扩展到其他Sprite。BE8继续不安装，独立companion按成员表和最终状态展示／退休；本地空槽保持为空，异常非空由合法owner处理，不能裸清或触发级联Remove。五个Host私有关系同样只按本地owner正常撤登记。

#### 4.2.8 公开成员回映射

Delirium十个唯一公开ReplicatedState全部归Host查询；BossType／Variant、条件计数及两个Cycle派生查询的原始语义/公式仍保留在版本41事实中，但不伪造Client可还原值。NPC GetControllerId同样归Host raw查询，u8受控UI模式不可反解设备ID。ParentNPC／ChildNPC／FlyingOverride仍复用公共状态，其余必要公开值保留真实字段映射。总44个唯一成员均有显式归属。

#### 4.2.9 同步修改的文档（2026-11-08 已完成）

- **第 1 步**：`variants` 增加 NPC 的例子。
- **第 3 步**：3.1 注明子类可以追加形态字段。
- **第 10 步**：读取规则增加 `world_delirium_owner` 的资格见证；10.2 增加 NPC 选臂和采集时机。
- **第 11 步**：11.2 的 shape 判定改为按布局的 `shape` 字段；增加对应失败方式。
- **第 14 步**：14.3 注明该 Hook 保留及其职责。

以上为2026-11-08文档设计时点；2026-11-09已在`entity-classes.json`落实142／143，整数版本41，见[本轮实现](implementation-4-npc.md)。旧“布局源没有改动”的状态已结束，原设计事实与未核字节保持。

### 4.3 Familiar、Knife、Bomb 本轮实现（2026-11-08）

布局源为 `mod/binding/state/entity-classes.json`，本节记录版本16当时的实际实现范围；后续版本33已排除HeadFrameDelay等11个参数，并补入80／240的展示负载，当前传输以本页顶部及生成表为准。原生发现、遗漏修复和编译事实保留，不把未知字节归属和第15步验收提前写成完成。生成表分别见 [Familiar](../../../../mod/binding/layouts/04-entity-classes/EntityFamiliar.md)、[Knife](../../../../mod/binding/layouts/04-entity-classes/EntityKnife.md)、[Bomb](../../../../mod/binding/layouts/04-entity-classes/EntityBomb.md)，证据与主审见[静态证据](familiar-knife-bomb-evidence.md)。

- 三类继承 Entity 的 E0—E9，子类块从 10 起。Familiar／Knife／Bomb 编号分别为 120／121／122；Familiar 的 48／231 变体臂为 123／124，分别传送 IsaacsBodyData 和 AbyssLocustData，不在 Client 创建其他未知玩法 owner。
- Familiar `+0xDB8` 的 ANM2 与主 Sprite 独立；Bomb `+0x464` 的五份 ANM2 按 0x114 步长分别投影。外观、动画、图层分别成块；共享 payload 和 Sprite vector 的元素也按子块投影，避免动画变化重发整组资源。
- Familiar.Player `+0x410` 按裸借用映射；KnifePlus.hitboxSource 按 EntityPtr 映射。派生 Plus 沿用同一份 E9 伴随来源，不重复发送基类字段。
- Familiar FireCooldown、Bomb `+0x410` 保留明确方向的 mode/value 锚点声明。尚未核定活动条件必须报 Gap，条目保持“推断”。HeadFrameDelay 的线性计时语义没有证据，改为直接保存公开 i32 原值，不要求计时方向。Bomb `+0x414` 完整用途未知，继续不复制。
- 恢复旧路径已同步的 Knife `+0x4F4/+0x4F8/+0x4FC/+0x500` 和 Familiar `+0xDAC` 五个 EntityPtr 槽；以身份／待登记句柄采集，后台转成员序号，Client 经引用 owner 恢复。Knife `+0x4E7` 降为“推断”的原始 u8，不强加未证明的 bool 0/1 约束。
- 蝗虫两组 Sprite 的 `count_groups` 合计上限为 16384，不是每组各自独立允许 16384；同组各投影 count 必须一致，数字检查表与生成文档来自同一源。
- 原生范围按 J273 池步长 F30／520／9D8；Knife SDK 的 E78 仅作差异记录。完整未知列表和各嵌套对象的未知均由生成表保留，本轮不声明类型字节完整。

详细交付、编译与后续第 10／13／14 步接线责任见[实现说明](implementation-4-familiar-knife-bomb.md)。

### 4.4 Tear、Pickup、Laser 本轮实现（2026-11-08）

本节保留版本20当时的实现事实；版本34／35／37已分别排除攻击／运动输入、Pickup交互输入／未来队列及Laser采样中间数据与计时。现行传输以本页顶部、版本37源及生成表为准。原布局版本20的唯一源为 `mod/binding/state/entity-classes.json`。布局编号 125／126／127，继承 E0—E9，子类块从 10 开始。完整生成表见 [Tear](../../../../mod/binding/layouts/04-entity-classes/EntityTear.md)、[Pickup](../../../../mod/binding/layouts/04-entity-classes/EntityPickup.md)、[Laser](../../../../mod/binding/layouts/04-entity-classes/EntityLaser.md)，交付与后续接线见[本轮实现](implementation-4-tear-pickup-laser.md)，静态逆向及主审见[证据](tear-pickup-laser-evidence.md)。

- Tear 三份辅助 ANM2 的外观／动画／图层独立投影，七个引用槽和命中索引分别保留，不借带副作用的 Height／Scale／TearFlags setter 恢复最终状态。
- Pickup 保留两份 ANM2、四个引用槽、八项有界循环道具；FlipSaveState 为 Host 玩法保存对象，Client 不重建。价格与图形最终值分别安装，不调用 SetPrice／InitFlipState 触发玩法。
- Laser 保留原始有符号采样模式、十三个引用槽，以及单独成表的 HomingLaser；其 self 回指不发送。四份几何容器独立建模，Bezier 保留六个 f32 槽，不运行 CalculateSamples。
- 三个 Plus 的派生音效、重抽和重新采样请求归 Host，公共覆盖只经一份 E9。条件计时不凭字段名判定；未知活动分支明确报 Gap。
- 原生范围为 0x848／0x6F0／0x568，未知与 SDK 声明保持实际等级。本轮只生成布局与纯编译，不宣称类型字节完整、正式采集已接通或第 15 步验收完成。

### 4.5 Slot、Projectile、Effect 本轮实现（2026-11-09）

布局版本 23。Slot／Projectile 为 128／129；Effect 保留 101／102／103 并增加 104／105／106 三个附属臂，payload／节点为 25—28。见[实现与边界](implementation-4-slot-projectile-effect.md)、[必要逆向与主审](slot-projectile-effect-evidence.md)，完整字节和二进制单行表由生成器输出。

- Slot 八个数值与独立奖励 ANM2 分块；Timeout 保留 i16，复杂状态机计数不猜全局线性方向。奖励编号不替代最终 Sprite，不在 Client 重演购买／派奖。
- Projectile 的 DepthOffset 直接回映射到继承的 E4 `depth_offset +0x358`，仅发送一次。两项 flags 各 u64，ChangeTimeout 保存公开变更参数，不误建递减倒计时。
- Effect 普通 VarData 未核定则不复制；TearFlags／GridDesc 按变体分开。三种具体 payload 不发送虚表、共享控制块或 deque header，经单一原生 owner 安装全部同版本投影。
- `state/owned_payload.*` 有界处理 u8 present、尾块存在性依赖和缺失值最短全零规范；未知动态类型或读取失败不得当作不存在。
- SDK 大小与池步长分别记录；公开目录已回映射，根对象和 payload 未知仍在各自生成表中，不能据此宣称字节完整或第 15 步通过。

### 4.6 前四项未决的工作量与拆分（2026-11-09）

本轮范围按原未决清单的前四个顶层条目固定，第三条虽已写明 DepthOffset 核定，仍计作其中一条。必要逆向由一个 GPT-6 Luna（max）子智能体执行，主智能体审阅；[静态证据及主审](step4-first-four-evidence.md)记录对象尾部与 Effect 的结论。此处拆分的是第 4 步内部研究次序，不新增 tx、测试或推进前置条件。

**NPC／Familiar 工作量估算**以当前布局和既有 owner 证据为依据，不再把全部子类字节减去旧公开字段字节当作未知量：

| 对象 | 已有结构依据 | 尚需处理的实际范围与工作量 |
| --- | --- | --- |
| Familiar | 子类共 2848B；`+0x5FC` 寻路对象 1872B 已按 SDK 声明归权威；`+0xDB8` ANM2 276B 可复用基础布局 | 子类自身未知 **557B／9 段**，其中 `+0x418—0x5F7` 占 480B，其余八段合计 77B。根表 906B 未知另含公共 Entity 335B、内嵌 ANM2 14B，不能重复算作 Familiar 专有研究。寻路的 SDK 证据升级、条件计时和 payload 完整性另计，不能由未知数较小宣称完成。 |
| NPC | 子类共 2896B；SDK 寻路 `+0x414—0xB63` 1872B；既有双 ANM2 `+0xCD8/+0xDEC` 共 552B；三组 deque `+0xBF0/+0xC04/+0xC18` 头共 60B | 以上结构以外剩 **412B** 的混合区域，含已核公开值、Delirium 臂、ColorMod、引用／容器及未知，不是 412B 全部未知。版本41已迁入生成源：NPC本类未知223B／15段，根586B另含公共335B及双ANM2共28B；Delirium根597B另加臂内11B。寻路声明等级与外部deque元素仍单列，不能由根未知数宣称完整。deque 的外部元素与具名 owner 工作不包含在 60B 头大小中。 |

采用以下次序；规模是相对核对量，不承诺固定工时：

1. **共享寻路边界（中）**：一起核 NPC `+0x414` 与 Familiar `+0x5FC` 的构造／重置／调用链，区分 Host AI 数据与实际展示依赖；不因名称一口气把所有空白归权威。
2. **NPC 已有表现结构（中至大）**：迁入双 ANM2、三组 deque、已核公开字段和 Delirium 变体臂；按实际消费者补 owner／引用／资源合同。复用[既有复合绘制证据](../tx-04-19/entity-i20-i25-native.md)，保留 Player 与 NPC 的布局隔离。
3. **Familiar 480B 历史区（中）**：核 `+0x418—0x5F7` 的元素、索引／长度规则、写入者与消费者。60 组 8B 清零只是构造事实，不能直接声明为 Vector 数组或填充。
4. **Familiar 八段零散区（中）**：优先 `+0xD90—0xDAB` 容器候选、`+0xED8` owner 邻接区和 `+0xF28/+0xF2C` 树控制关系，再处理其余小段；已有字节原样保留至语义闭合。
5. **NPC 剩余混合区与变体消费者（大）**：按 `[0xB64,0xBF0)`、`[0xC2C,0xCD8)`、`[0xF00,0xF60)` 收敛；每段区分共用值、特定形态、引用、权威和未知。该项通常比简单移植 ANM2 耗时，不能用只扫构造函数替代。
6. **零散尾部与证据等级收尾（中，依实际发现调整）**：合并复核未证明的 padding、SDK 声明和跨函数活动条件；静态证据不足的字节继续列未知，第 15 步统一观察，不提前执行未知区探测。

**DepthOffset 复核**：Entity 注册 `VA 0x84F33F/0x84F344/0x84F349` 与 Projectile 注册 `0x8514CF/0x8514D4/0x8514D9` 均绑定 `0x431380/0x4313A0`。setter 在 `0x431388` 写 `float [ecx+0x358]`，getter 在 `0x4313A0` 读同一位置。`entity-classes.json` 的公开成员回映射已经指向父类 `depth_offset`，故本项只清理过期未决，不再添加线字段。SDK `_depthOffset +0x2B0` 仍不是此公开字段。

**对象大小与尾部**：工厂向 MSVC EH 数组构造器传入本类 ctor／dtor、数量与单元素大小；Knife／Slot／Projectile／Effect 分别为 `0x520／0x550／0x478／0x4C0`。续研又在四类 vtable slot0 的删除析构包装器中核到同样的显式 size 参数：`VA 0x939C00／0x659A54／0x64F487／0x5CCFA2`，均先压大小、再压对象地址、调用释放函数后回收8B参数。两条编译器路径共同核定 J273 的 `sizeof`，Effect不是`0x4BC`。Hex-Rays伪码隐藏了第二个参数，结论以原始指令为准。尾部填充与否另判，大小本身不能证明字段语义。Slot `+0x548` 已确定为进度对象借用指针，Projectile `+0x470` 已确定为回调函数指针，分别按本地归属；它们都不是填充。末四字节的全映像候选已按实际对象复核，支持尾部填充假设；尚不能把源级填充与未使用保留成员区分，继续保留证据边界。

**Effect 指定区间**：`+0x440` 模式、`+0x444` 来源关系和 `+0x448` 角度用于 `0x5D3280 → 0x633330` 的玩法传播；来源槽会登记到目标的反向引用集合。`+0x47C—0x4A7` 在 variant 203 的 `0x5D3480` 中组成生成请求：类型、变体、位置、速度、spawner、subtype、seed、第八参数和出生格子序号。两个 Vector 代替旧“16B 容器头”描述。布局按这些实际边界逐段命名，归权威且具体用途证据升级“已核”：这些玩法输入只由Host消费，Client不复制其地址或重演传播／生成；原版全部关系生命周期没有被证明，也不是本次新增的Client操作。`+0x41C`、`+0x44C`、`+0x4BC` 的四字节段仍未知，限定范围内没有直接位移命中不等于填充证据。

布局版本 **27**。Knife `+0x504—0x51B` 的六个f32保留独立K4（24B），具体用途为父刀距离、相对角度、局部偏移X／Y、本刀分裂触发距离和碰撞方向角，证据升级“已核”。`+0x518`由HandleCollision消费，前轮称绘制字段有误，已更名`knife_tail_collision_angle`；不在Client执行碰撞。末四字节仍按现有证据等级保留。Slot／Projectile／Effect本轮更正原生归属，不增加运行中的采集、安装或回调执行；第10／13步必须遵循本地地址不传输、Host玩法不重演的约束。完整改动和普通编译结果见[本轮落地说明](implementation-4-first-four.md)。

### 4.7 Text与Dummy（版本44，2026-11-09）

唯一源`state/entity-text-dummy.json`、[逐段证据](text-dummy-evidence.md)及[生成表](../../../../mod/binding/layouts/README.md)为现行合同。Text按RTTI选145，再以variant2选146；Dummy只按真实RTTI选147，不能用type964冒充。Text TX0为RGBA16B，TX1为u16 count加0—255原始字节，TX2仅146有counter4B，基础臂显式预留。duration不上线；KColor末字已核颜色空间，固定Text draw不消费，通用20B布局不变。读串须当前use/revision真实writer，Init／复用／退休撤销资格；空串、缩短、非UTF8及未终止分别处理。

Dummy无尾部字节／专用字符串，标签由公共Type/Variant派生，使用本地Font2；正常可见分支使用公共Screen公式。不可见时公共Render不刷新cache但标签继续读cache，此历史依赖不能假称无条件派生，须在第13步闭合具名来源，缺失报Gap；本轮没有猜测8B跨视口负载。Text尾字52C—52F保持UNKNOWN；两类继承的公共未知不变，不能据已生成布局宣称类型完整或运行验收通过。

## 改动范围

- 布局源 `state/entity-classes.json`、`state/entity-effect.json` 中的子类条目和专用块；其中 Effect 按 4.1 分为 6 个变体臂布局，NPC 按 4.2 增加 Delirium 变体臂。
- Effect 的 Timeout 换算函数和 `effect_timeout` writer。
- Client 类别写入由生成表分派，取代 `ClientEntityClassState.fields` 的字符串匹配。专用容器继续沿用现有写入函数，例如 `ClientEntityCollections`。

## 失败方式

- **按 type 选布局**：只看 `_type`，把特殊虚表对象套上错误布局。
- **未知区处理错误**：大段未知区被当成填充跳过，导致关键状态漏同步（例如 NPC 的 AI 状态）。
- **借用当成 EntityPtr**：把裸借用当成 EntityPtr 读取，地址解析错误。
- **容器读取越界**：几何容器读取时没核 begin、end，或者超过上限。
- **复活拥有型对象**：在 Client 上重建了不应存在的玩法 actor，例如 NPC +0xF40。
- **按 SDK 大小划范围**：Effect 按 0x490 划定对象范围，漏掉 0x490—0x4BF 的私有容器和 ExtraData；Slot、Projectile 漏掉尾部 8 字节；Knife 按 0xE78 读取，越过对象进入池中下一个槽。
- **Effect 选错变体臂**：113仍按旧102选臂，或把未核变体的VarData当TearFlags／传播输入；136描述自版本38起归Host，不能恢复旧16B或32B同步负载。
- **Effect 直接复制 ExtraData**：复制 owner 指针或 memcpy payload，没有经游戏分配器和 `0x606310` 重建。
- **Effect 跟随关系先写**：IsFollowing 为 1 的 FX1 先于 Parent（E8）写入，而 Parent 目标还没有构造。
- **Effect 到期帧基准不一致**：两端换算 Timeout 用的帧基准不同，例如 Client 误用自己的 `Game+0x25A70`；或者装好新的 world 帧后，没有按新的 game_frame 重写 Timeout。
- **Effect 轨迹失控**：轨迹节点数没有上限或含非有限值；整条轨迹每帧重发，超出带宽。
- **Effect 格子描述重复**：举起的格子描述在 Effect 和 Grid 两处各发一份，而且内容不一致。
- **Delirium 资源变化漏发**：稳定公共shape下没有比较最终资源／姿态块，误以为必须先发BossType或结构换代才刷新画面。
- **Delirium 选错臂**：采集窗口落在 `0x49F3D0` 内，`type` 是临时的 Boss 类型，对象被套上基础臂。
- **发送池槽残留**：把普通 NPC 的 `+BD8—BE7` 当状态发送，内容是上一只 Delirium 留下的值。
- **Delirium 无根据换代**：仅Host Boss选择／AI重配变化时，`world_delirium_owner`或旧Boss shape逻辑仍退休重建同一NPC；版本42只按真实公共结构字段决定换代。

## 完成标准（本步尚未全部满足）

- **字节归属与表格**：十个子类和专用形态的字节归属全部完成，带证据，两张单行表由生成器产出。
- **回映射**：子类全部 `ReplicatedState` 成员都有回映射。

## 未决问题

- **Projectile版本45后剩余**：+440与+460/+464的指定展示来源已闭合；本类仍有434—437、441—443、44C—44F、474—477共15B未知（加公共335B）。见[残余证据](projectile-effect-residual-evidence.md)。正式采集／安装／draw恢复接线归第10／13步，不能据静态闭合宣称运行已覆盖。

NPC受控UI的来源问题已在版本43[闭合并纠正](npc-control-ui-evidence.md)，不再要求逆向原版Online slot5或读取其记录。Text／Dummy布局及职责已按版本44实施；下列其他UNKNOWN与各域后续接线不因此成为已完成。

- **原前四项的剩余部分**（工作量／次序、DepthOffset、对象范围和已核字段结论见 4.6）：
  - NPC／Familiar 按 4.6 的次序实施剩余归属；工作量估算已经完成，不等于未知字节已清零。
  - Knife六个f32的用途已闭合；`+0x51C—0x51F`、Slot `+0x54C—0x54F`、Projectile `+0x474—0x477` 尚无填充证明。Slot `+0x548` 与 Projectile `+0x470` 已从未知中分离为本地指针。
  - Effect `+0x41C—0x41F`、`+0x44C—0x44F`、`+0x4BC—0x4BF` 未知；传播／生成参数的专用用途及Host归属已闭合，未新增Client引用写入或玩法执行，因而不把原版关系全生命周期穷尽继续列作该项同步依赖。`+0x484—0x493` 已证明是两个 Vector，不再作为待核容器。
- **Effect剩余事项**：[本轮五项处理结果](effect-next-five-evidence.md)已落源；版本45已核19/71/194内部flags、167伤害快照及119一个callback闭包；其余未核variant、167的434及438后8B、各臂残余UNKNOWN保持，第10／13步接通具名reader／writer，第15步观察实际分支／频率与未知间接访问边界。已核的Timeout非线性结论、FX0分块、Trail64B／127上限和三种具体ExtraData不再列作原泛化问题重做。
- Delirium：
  - 版本46[已处理原四项](delirium-residual-evidence.md)：BDD／BE4共7B核为两个打包存储的一部分；AI值已入源且不上线；补齐BE8的创建／消费／绘制依赖后继续归Host，不再保留泛化的“410可能需要引用”前置条件。
  - **仍未知的4B**：`+BD4—BD7`符合对齐假设，缺少源级填充或保留成员的正面证明；保持UNKNOWN，不能宣称Delirium字节完整。第15步E27保留此范围。
  - **只剩实机事实待观察**：普通NPC池复用保留BD8—BE7已有静态可达路径，但未实测；第15步按同槽新use记录，不提前运行测试，也不把它当作继续实现的前置条件。
  - 第10／13／14步接线依现行合同推进：稳定type412选143、owner退尽后采集、真实成员及最终表现安装、删除旧AI writer和无条件shape；本轮不越步宣称运行链完成。
