# 10：Host 采集器

状态：设计稿，未实施。上级：[总计划](README.md)。

## 目标

在主线程的合法窗口内，按生成的复制计划把全部成员的状态复制到预分配的自有缓冲区，然后非阻塞地交给后台。主线程每帧 p95 ≤2 ms、p99 ≤4 ms。

## 输入事实

### 现有 Collector（`mod/src/world/collector.h`）

- **定位**：tx-04-08 建立的主线程只读采集器。所有入口零等待，运行期不分配内存。
- **读取方式**：通过 `MemoryReader.copy` 进行有界原始内存复制，捕获不可读地址，不调用游戏接口。
- **其他已有能力**：
  - 维护 `IdentityTable`；
  - 按 RTTI 分类；
  - 枚举实体表（`Room+0x1218` 起的 8 个列表）、roster、Grid 槽和门槽；
  - 记录逐身体证言 `PlayerPostWitness`。

### 现有 CaptureWorker 交接（`stream_capture_worker.h`、`stream-catalog.json` capture_profiles）

- **交接槽位**：一个构建中、一个最新的 ordinary 槽；冷源单独固定；8 个最新身体槽；256 个有序控制。
- **撤销机制**：用代次和水位撤销失效的交接。
- **后台约束**：后台线程不借用 Lua 或原生对象。

### 现有成本口径

stage11—18（[tx-04-21 采集性能方案](../tx-04-21/tx-04-21-capture-performance-plan.md)），其中 stage12 是主线程采集。

### 合法窗口与读取规则

ordinary 窗口、PlayerPost 窗口，以及规范第 11 节的条件读取规则（例如 `removedByFactory`）。

第 3 步的生成表把读取资格写在 `Field.read_requirement` 和 `CopyRange.read_requirement` 上（`entity-current-window`、`current-entity-byte-0x177`、`entity-current-window-and-confirmed-timer-clock`、`entity-current-window-and-confirmed-color-clock`）。本步必须为每种资格提供真实见证，不能从布局声明推导授权。条件计时由字段 owner 给出已核的实际递减条件；活动但时钟关系未知时报缺口，不能用 mode=0 冒充停走。EntityPlus 只经 `ReadEntityExtension` 读取已存在的来源（[第 3 步实现](implementation-3.md)）。Delirium 变身内的 Init 之后，`current-entity-byte-0x177` 的资格仍由 `world_delirium_owner` 见证恢复（第 4 步 4.2.6）。

## 设计

### 10.1 位置与时机

- **采集位置**：新采集器是 Collector 的一个采集阶段，在已核的 ordinary 窗口和 PlayerPost 窗口内执行，不新增 Hook，也不新增窗口。
- **按会话启停**：会话外逻辑停用，Hook 保持常驻。

### 10.2 布局识别

- **建立映射**：会话开始时，在后台按 `rtti` 名找到 J273 的类型描述符，建立“虚表地址 → 布局编号”映射表。
- **主线程查表**：主线程读取对象的虚表指针后查表。查不到时报具名缺口（`unknown_vtable`），该对象不采集，不能按 `_type` 猜测布局。
- **选臂**：如果类型声明了变体臂（第 1 步 `variants`），查到虚表之后还要读选择字段，按变体集合选定布局编号，例如 Effect 读 `Entity._variant`（第 4 步 4.1.2），NPC 读 `Entity._type`（第 4 步 4.2）。
  - 未列出的取值使用基础臂。
  - 选择字段读取失败时报具名缺口，该对象不采集，不能退回基础臂。
  - Delirium 的 AI 会临时把 `_type/_variant` 换成 Boss，返回前恢复。采集窗口不能落在 `0x49F3D0` 内，沿用 owner 未退尽不提交采集的规则。

### 10.3 复制计划的执行

第 4 步子类计数的 `entity-current-window-and-confirmed-subclass-counter-clock` 需要字段 owner 给出实际增／减／停走分支；未知方向或活动时钟关系报 Gap。Familiar／Knife的E9保留正确派生Plus来源，但版本39起均只读取公共nullable覆盖，可复用`ReadEntityExtension`，不读取Knife hitboxSource。容器投影既检查元素数，也检查生成的 `Field.max_size` 与全局池预算；所有投影来自同一个窗口和同一组元素，不截断。

版本33起HeadFrameDelay归Host，不再复制或核定同步时钟。版本39又将KnifePlus.hitboxSource留Host，删除正式ReadKnifeExtension及其身份预约；公共两optional仍经现有链前后复读。新增WeaponUmbilicalWhip的+44真实主引用才需要当前窗口的Identity／Pending预约，后台分配成员序号，不向后台交地址或提前要求新实体已经编号。计数组按生成的 `CountConstraint`／`ValidContainerCountGroups` 检查，蝗虫两组合计最多 16384，投影重复 count 不重复计数。

第 4 步新增 Tear／Pickup／Laser 同样按 RTTI 查布局，不按 `_type` 猜派生 Plus。按版本29后的实际线上引用采集Tear StickTarget、Pickup公开三槽与Laser BounceLaser，使用身份／Pending，后台转成员序号；23条内部Host关系不恢复采集，不得在主线程提前要求全部新实体已有网络序号。HomingLaser 的 `laser_self` 是本地自回指，不跟随或发送；版本37只在同一窗口读取optimized／unoptimized两个投影，各自count上限16384、独立点数，递归校验浮点及实际累计字节预算；不读Homing六标量、indices中间缓存、Bezier输入或系数。版本35起Pickup循环数组及count留Host，不再遍历或为其count建同步Gap；FlipSaveState继续不采集其玩法对象。历史字段与计时发现见[本轮实现](implementation-4-tear-pickup-laser.md)，Tear两个计时的现行排除见下方版本34合同。

Slot／Projectile／Effect 的布局和公开回映射见[后续三类实现](implementation-4-slot-projectile-effect.md)。Slot 的奖励 ANM2 与主 Sprite 独立读取；Projectile 的 DepthOffset 只读取继承 E4 一次。Effect `shared-owner` 字段的偏移指向 holder，必须读前后 holder／具体 RTTI／生命周期见证，全部投影来自同一个 payload 和窗口。缺失输出规范化最短全零，未知类型或读失败报 Gap，不能按缺失处理；严禁把生成的 payload 偏移直接加到根 Entity。版本38已由606190的明确new(0x40)及完整复制核定Trail节点：按deque逻辑顺序遍历独立节点指针，不能当连续数组；不再要求旧confirmed-trail-record-layout读取资格。Ghost只采集0—4有效Sprite前缀，ANM2 Offset／Scale不上线，原生i32 count收窄前检查，三个投影同一窗口／前缀。普通变体的 `0x420—0x43F` 未核，FX2 预留不复制。Timeout 每窗从原值重算到期帧，停走或非线性变化会使锚点变化并重新发送，不依赖所有变体都恰好递减一次的假设。

生成器为每个布局产出一份操作序列（`state_capture.inc`）。主线程按顺序执行：

| 操作 | 动作 |
| --- | --- |
| `Copy(src, len, dst)` | 合并区间，一次 memcpy 进缓冲区 |
| `Convert(fn, dst)` | 调用具名换算函数，例如 Effect Timeout 的到期帧 = 全局帧 + Timeout |
| `Ref(src, dst)` | 读指针，查 `IdentityTable` 得到本地身份句柄，写入缓冲区；后台再转成成员序号 |
| `Resource(src, kind, dst)` | 读资源指针或路径，查资源表；遇到新资源先登记待发送 |
| `Container(src, elem_layout, cap, dst)` | 读 begin／end 或 count，校验后按元素布局逐项执行（嵌套计划） |
| `Sub(offset, layout)` | 对内嵌对象（ANM2、LayerState 数组等）递归执行其计划 |

- **一个对象一层保护**：每个对象的计划在一个 SEH 保护块内执行（一个对象一次，不是一个字段一次）。发生访问异常时，该对象的整条记录作废并报具名缺口，不写入半条记录。
- **地址来源**：对象地址只来自已核的枚举来源，以及 Collector 已校验的成员关系，不跟随任意指针。
- **条件读取**：带条件的字段（规范第 11 节）由计划中的条件操作控制，条件不满足时写入“不可读”标记。

版本30的33个Entity公共排除范围见[职责复核](entity-common-synchronization-review.md)，版本31再从E2移出valid，见[生命周期再复核](entity-lifecycle-synchronization-review.md)。上述范围不执行公共状态复制；removedByFactory及valid仍可在合法Host身份／生命周期观测中读取。Player结构源所需的独立激活资格继续由第5步接管，不能连同公共valid字段一起静默删除。版本32再按[动画／资源／颜色职责复核](animation-resource-synchronization-review.md)移出ColorParams容器及颜色锚点；事件位不采集，ANM2.loaded只参与可空文件资源选择，不作为独立状态，LayerState.loaded不采集。最终颜色、独立selector、is_playing／playback_speed、实际Image与路径继续采集；子类及Player真实展示计时保持各自合同。Host资源转换失败报Gap，不编码为空资源。

版本33另按[Familiar参数／充能／绳索复核](familiar-synchronization-review.md)排除Familiar11个轨道／方向／计数范围，不为HeadFrameDelay取时钟。80只从稳定Weapon来源复现已核纯字段公式，编码最终0—100整数amount，不调用getter或序列化Weapon。240从具体UmbilicalBabyData按逻辑deque顺序采XYZ和Sprite；不复制map／环偏移／代理／节点模拟字节，非空payload节点数0、未知动态类型、非法map或来源变化报Gap。四payload投影count一致且同窗口，不能把读取失败编码为缺失。

版本34按[Knife／Bomb／Tear参数展示复核](knife-bomb-tear-synchronization-review.md)排除Knife10／Bomb11／Tear21个参数范围；生成的复制、换算和线上校验不再读取它们。Bomb ExplosionCountdown与Tear WaitFrames／StickTimer不再建立锚点或时钟见证，原Host分支未知不因此阻止展示采集。仍采集Knife拖影输入、Bomb最终主／五份costume及开关、Tear效果位／DeadEye与三份辅助Sprite；公共最终Position／PositionOffset和Sprite姿态不能由已排除输入重算或省略。

版本35按[Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)撤销Pickup选择组／触碰／自动价格／商店标识等输入、Timeout／Wait锚点和未来循环列表，以及Slot八项交互／状态／奖励输入；不读取、遍历或为它们取得同步时钟。Pickup仍读Price和DropDelay：前者供WaterClip，后者正值隐藏Render，不能只凭Visible或最终Sprite省略。保留主／辅助Sprite、真实引用与DropDelay活动条件见证；Slot奖品动画／播放状态属于ANM2，不能误解为被排除的机器状态。

版本36按[Projectile运动与展示参数复核](projectile-synchronization-review.md)不再复制Projectile的FallingSpeed／FallingAccel／HomingStrength／CurvingStrength／Acceleration／WiggleFrameOffset／Scale／ProjectileFlags；WiggleFrameOffset不新增时钟／锚点要求。Height有直接Render消费，继续读有限f32，并与公共最终PositionOffset、位置／Velocity、尺寸和Sprite同窗。私有+440及+460／+464展示来源仍待第4步补齐，UNKNOWN不读、不伪造零或称完整；公共EntityFlags与Game帧展示来源保持。

### 10.4 缓冲区

- **预分配**：缓冲区和身份句柄表在会话开始时一次分配，分别归入现有的 512／128／128 MiB 三池。
- **双缓冲**：主线程写一份，后台读另一份。
- **超出容量**：报具名缺口，并把本次采集标为“不完整”交给后台。后台不发布不完整的 world 帧，改发下一次完整采集；容量按高水位在会话之间调整。
- **主线程不分配内存**：采集过程中主线程不分配内存，不加锁，不等待。

### 10.5 频率分层

- **body**：每个合格身体在 PlayerPost 窗口内执行 Player 的 body 块计划。
  - 只复制 P0 以及 E1／E4／E5 中属于 Player 的部分，写入对应的身体槽。
  - 没有 PlayerPost 资格的身体，由 ordinary 窗口补采（沿用现有的 ordinary body-joint 语义）。
- **普通**：ordinary 窗口按约 30 Hz 采集全部普通成员和房间、楼层的动态块。
- **低频轮转**：描述表 L1／L2、Mod 域、Run／Presentation 中的大块内容分片轮转读取。
  - 每个窗口只读一片，一整轮的周期由第 11 步按预算定稿。
  - 房间切换、楼层切换等已知事件触发立即全量重读。
  - 不依赖“通知完整”来跳过读取。

### 10.6 成本估计与计量

- **估计**：
  - 一个典型房间约 300 个实体。每个实体有效复制约 0.5—2 KB，图层和容器在外。
  - 合计约 0.6 MB memcpy 加约 1—3 万次指针跳转。
  - 估计主线程成本在 1 ms 量级。这是按 isaac-lan 实测（约 1.3—2.6 ms／视图，含 Lua）推算的目标，不是实测值。
- **计量**：沿用 stage12 口径，并按域细分计时，方便第 15 步定位。

## 改动范围

- Collector 新增采集阶段，以及复制计划解释器（读取生成的计划表，不手写逐字段代码）。
- 虚表映射表和资源表登记。
- CaptureWorker 交接的载荷改为二进制缓冲区，槽位和撤销语义不变。
- 现有 `CopyLuaSource`、`lua_entity.cpp` 的正式采集入口在第 14 步移除。

## 失败方式

- **跟随野指针**：跟随未经校验的指针读取，触发访问异常，或读到已释放的对象。
- **选错变体臂**：选择字段读错，成员套上错误布局；或者选择字段读取失败时没有报缺口，而是退回了基础臂。
- **复制半条记录**：异常发生在对象中途，前面的块已写入缓冲区却没有作废。
- **窗口时序错误**：在窗口外执行，或者 body 采集早于该身体的 PlayerPost 更新。
- **身份时序错误**：引用指向本帧刚加入、还未登记身份的成员，句柄查找失败。句柄必须按“待登记”处理，不能写 Null。
- **轮转漏变化**：低频轮转周期太长，变化长时间不被发现；或者已知事件没有触发重读。
- **超额不报**：缓冲区超额后静默截断。
- **主线程阻塞**：主线程等待后台释放缓冲区。缓冲区不足时，必须丢弃本次采集并报缺口。

## 完成内容

- **实现与编译**：采集器与计划解释器实现完成，纯编译通过。
- **代码约束**：主线程路径没有 Lua 调用、没有 JSON、没有内存分配，所有核心函数和数据结构带中文注释。
- **验证安排**：性能和正确性的验证全部在第 15 步完成。

## 未决问题

- 低频轮转的分片大小和一整轮的周期。
- 身体槽数量 8 是否继续足够；Esau、Tainted Lazarus 等多身体情况下要核对上限。

版本37按[Laser／Homing采样、运动与计时复核](laser-homing-synchronization-review.md)不采集Laser十九项更新／运动／计时输入及Homing中间数据；Timeout／RotationDelay不再要求同步时钟见证或制造Gap，原生分支UNKNOWN保持。当前Angle／Length／完整TearFlags／sample_mode_raw／MaxDistance／GridHit／公开EndPoint／+4F0与两组最终点列同窗读取，公共最终位置／偏移／尺寸与Sprite各自保持。两组容器不要求点数相等，但均须来自同一目标版本和稳定生命周期。

**版本38 Effect采集**：按[Effect各臂与payload复核](effect-synchronization-review.md)不采集Height／FallingSpeed／FallingAcceleration／MinRadius／MaxRadius／DamageSource、四个TearFlags变体的完整128位或136描述八成员；不为这些Host输入建立同步Gap、原值或时钟见证。State／Timeout／LifeSpan／IsFollowing／ParentOffset／Rotation／Scale及独立payload继续同窗采集。MrMe新增四项Host私有范围不读，仍读取其最终ANM2；Trail最终64B节点不本地推算；Ghost三投影count一致、每个count≤4且实际总字节在预算内，读取失败／错RTTI不能伪装缺失或截短前缀。

**版本39 Player装备／Plus采集**：按[完整复核](player-equipment-plus-synchronization-review.md)四主动槽只读六值，五Weapon槽按真实vtable／type读取四个展示值及15号主引用；不得用0x44前缀当实际动态大小。TemporaryEffects采集disabled及完整原顺序vector，配置保留type/id，count／cooldown不由库存／costume猜测；mask／evaluate不采集。合法Player装备窗口与前后来源、实际字节和16384预算保持；不得触发Lua getter／回调、PlayerHUD Update或任何重算。EntityPlus两nullable保持ReadEntityExtension的engaged先读／未设payload不读，不能把缺holder当Null。Knife hitboxSource不读取、预约、校验或制造Gap，其他已排除Plus项不恢复。Familiar新增D76原bool与主体同窗读，不由Weapon／is_follower推导。
**版本40 Effect后续五项采集合同**：见[证据与落地](effect-next-five-evidence.md)。按生成的11个布局选臂，102为54／126／167、141为113；137—140分别处理传播、Timer、生成请求和30／153计数的原生归属，不采集这些Host输入或函数地址，FX2始终预留，残余UNKNOWN不补零。每窗以同窗F／T重算Timeout锚点，101双减、141刷新及负值／停走均允许锚点变化；Rotation／Scale归FX0（16B），FX1为LifeSpan／following／ParentOffset（13B）。Trail只在正常Update及后置裁剪完成的完整尾窗读取逻辑0—127个完整64B节点，源数量先按原生宽度检查，最大节点块8130B；超限报Gap，不截断／抽样，不读取中途的128。所有Effect先检查holder资格：非附属臂必须空，附属臂非空时RTTI必须与variant匹配；未知／不匹配／读取失败不能伪装缺失或继续发布。此资格检查不等于采集或传输holder地址；仍须前后核生命周期、主线程及实际字节预算。具名接线仍由本步实施，未运行测试。

**版本42 NPC现行合同**：NPC142／Delirium143的BossType／Variant及AI值不采集为线上字段，17／18均空预留，不因它们改变安排shape。N0保留五个原生绘制值及u8受控UI模式；F1C仅在主线程供具名转换使用，匹配来源的具体动态谓词/只读公式未核定则Gap，不调用未知虚函数、Lua getter或默认true。使用entity-current-window-and-confirmed-npc-control-ui-source资格，设备ID不进入自有线上缓冲或后台。OverlayEffect144排除Scale读取，其他姿态／资源与三deque保持；仍在Delirium AI owner退尽后选臂。见[完整复核](npc-synchronization-review.md)。
