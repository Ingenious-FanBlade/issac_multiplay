# 13：Client 原生应用

**第6步描述接口已实现，原生接线仍待本步**：见[影子描述实现](grid-description-implementation.md)。从第12步完整影子记录调用`ClientDescribeGrid`，通过语义角色取得同源数字字段及真实nested_layout投影，不能把旧JSON `LegacyClientGridDescription`强转为正式描述。本步再把正式描述接入原生执行器／门绑定、资源owner、容器与绘制；当前执行器仍显式消费legacy命令，Plus／Lock等既有运行缺口保持。不得因描述生成成功就回AppliedAck。

**2026-11-10 Presentation安装合同（待实施）**：依[核对结论](presentation-synchronization-review.md)按C/S真实身体/显示组与目标版本安装8内嵌及动态HUD、最终心格/实际图像/有序列表、StatHUD三值，不恢复previous/threshold/dirty。97E4A0/97EB40/834930含Online选择，使用具名C/S分派；禁止965DB0，挑战32无最终标记不得随机补绘，SDK误标4ECA30绝不调用。地图展开/holdTime/视口及音量/静音归各端本地；屏幕/Prompt/转场/死亡仅执行已核最终pass，不执行提交/退出/健康/解锁。one-shot去重，持续voice支持baseline/停止/clear且重复state不Play。未知来源/图像失败不可填默认后ACK，正式接线/第15步验收仍待完成。

**版本56 Run应用合同（owner待实施）**：[Run核对](run-synchronization-review.md)的Run身份／时间、当前颜色／黑暗／震动／闪电／眩晕、分数与窄显示投影只进入具名显示owner。种子7位与ascent bit48在已声明绘制／只读显示范围借用并恢复；不替换原生种子树、整组flags、Host模式对象或本地进度，不调用SetStartSeed／SetStateFlag／Ambush／每日Init／挑战完成。97C300→966680内9BAC90分数重算须隔离；HUD禁用成就图标取Host真实批次，不能查Client资格补值。TimeCounter不代替GameFrame，target／lerp颜色不再安装，两个黑暗浮点有直接消费者均保留。非Run的Pause／UI／转场／shader／当前标题等按核对第4节逐项接线，缺同world来源不ACK。

**版本54 Grid水面应用（待实施）**：按[七族与官方旁表合同](grid-plus-water-source-evidence.md)安装每实例／use最终u32。现有六wrapper漏掉Lock，必须经统一清单给J273 VA70CCE0补第七常驻覆盖点，原链一次后写out.bitFlags；所有七族out+4 float由原链置0。未补Lock不能ACK其结果，不能改Host collision／虚表或官方共享旁表补缺。本地+3C先中性初始化，六族与Lock都消费C/S自有结果，绝不重演Set／Reset／Init。候选工厂会清同index官方slot，故最终结果不能存在官方表中；停用／退休只撤C/S结果与借用，Hook常驻且非C/S原样透传。

**版本53 PressurePlate／Door应用（待实施）**：按[逐类合同](grid-pressure-door-evidence.md)安装227／228；TimerPlate与extra各用真实内嵌ANM2／201，不用值副本、variant或本地loaded字节冒充资格。Door主226把1B入口值安装成本地alpha0/1，实际颜色借用同world完整墙色并恢复；extra自身颜色保留，Light独立不随主入口跳过。+28C先本地中性初始化，仅设置/清除bit0x20，不写Host整word；target索引以同world目标映射解析，不能启动过门FSM。禁止安装previous／四动画名／NextGreedAnimation／RNG或调用PlayAnimation／SetRoomTypes／Reward／Direction setter补画面；旧door_animation_mode误用字节退出，+154为Host dword计数。释放先撤全部门别名并drain绘制，再正常析构双ANM2／四string；缺目标/资源/墙色/计时来源不ACK。

**版本51 八类Grid应用合同（待实施）**：按[逐类职责](grid-eight-subclasses-evidence.md)以真实工厂构造206—213，不调用Init／PostInit／Update／Hurt／Destroy补状态。五类202主Sprite借用同world Room.floorColor并恢复，Wall非16房间必须保留201颜色。TrapDoor计数、Fire计时／样式及ember速度不写Host原值；Fire线上16B粒子重建到本地24B元素，速度中性且绝不模拟，vector存储遵守本地owner／Free前detach和drain。第三Sprite按204／203安装，主帧与两组选择器由实际age/maxLife和主资源frameCount逐粒子重建，is_playing／playbackSpeed／overlay保留；SetFrame产生的帧／选择器／事件临时修改须精确恢复，不能派发Host动画事件。三份Sprite各自消费glitch最终选择；state>=4空列表不代表重演熄灭或删除Grid。未知／资源／owner失败不ACK，实际writer／draw恢复尚未实现。

**版本50 Grid公共应用修订（待实施）**：按[复核](grid-common-synchronization-review.md)先由本地构造owner把+3C初始化为中性0，再将完整u32 water_clip_flags安装到per-instance owner，六个wrapper保留original各一次并覆盖最终结果，不复制Host碰撞或永久污染按index共享的REP旁表。Sprite201／图层200只按实际资源／Image与显式动画安装，不按Host默认名／待加载路径调用Play或LoadGraphics。glitch_draw_bits须由同world Dataminer pass中的具名RenderFrame/source-quad adapter消费，阻止原生／REP再按Client地址随机并精确恢复临时状态；不可用Sprite Rotation/FlipX替代纹理source-quad变换。实际ABI／资源／pass／adapter未闭合时保留Gap，禁止用默认值ACK；此段取代版本48的collision写入与完整通用Sprite复用要求。 冠军参数只写+D4—E3的四float，不覆盖+E4—FF七值，不据旧完整ColorMod回读要求恢复28B；普通颜色完整11值保持。

**版本48 Grid公共应用合同（2026-11-10，运行接线待实施）**：按[实际消费者](grid-common-evidence.md)安装Desc的四项展示输入、index／collision及主Sprite投影；旧`Common/CommonMatches`的八项Desc／spawn_frame写入与原值比较应在本步／第14步退出。来源Initialized不上线，工厂构造／资源准备／发布均由Client本地owner见证，不为查询相等重演Init／PostInit／Update或恢复奖励／玩法_rng。Position由同房间width推导，屏幕位置按Client本地视口换算；Light临时随机使用同world Host帧＋SpawnSeed。Plus水面nullable旁表、Room颜色／压力板文字及其它上下文未接入前如实报缺口，不能用0冒充Null或宣称仅主Sprite足够。子类仍按真实布局后续接入，199仅公共前缀。

**版本47布局来源（2026-11-09）**：[第5步正式源／生成](implementation-5-player.md)已完成，EntityPlayer=197，participant／control-group=195／196，Player主动槽=179，条件效果=180，动态Weapon=181—194；物理0—24及条件／固定槽／预算元数据以唯一源和生成表为准。以下版本46／草稿／待生成表述保留原时点；本步骤的运行接线仍未实施，不能因布局已生成就提前接收、采集、安装或宣称验收通过。

**第5步统一映射安装补充（2026-11-09，未实现）**：[逐行回映射](player-member-map.md)是裁剪归属，不是完整公共API兼容承诺。Client仅在批准绘制上下文消费具名最终决定，不能在一般玩法路径放行有Online／进度／RNG分支的getter；缺少具体缓存时报具名Gap。原版通信+268C分支保持关闭，不写Host通信选择／计数或把两ANM并入P0；正常本地资源仍按自身owner析构。+2E94队列不安装，效果只用P8当前列表。Plus使用官方成员和正常owner，Host缓存／重入／恢复状态不写入，不依据推导A8表强行修正内存。

**第5步P6—P8安装合同（2026-11-09，未实现）**：[候选／结构／退休次序](5-player-p6-p8.md)要求先准备全体Player／C/S组／资源，再绑定真实twin／backup／held及roster／列表／独立激活资格，共同提交统一P6拓扑。普通热块不得写valid；非room独立激活只由本方正常初始化、独占且不属于可复用工厂池的Player结构owner安装。costume只写保留元数据及最终map，效果只写配置／count和704 cooldown，其余冷却本地中性；P8维护正常残影deque与0／1当前记录list及1C50 ANM2。sub／replaced／703四槽、原始History／索引／forms和完整回放队列不安装／不做原值漂移比较；其实际HUD图像列表／分数由第8步owner安装，并隔离Render内9BAC90重算。CPU drain后撤raw／EntityPtr借用及ANM强引用，不沿互惠关系级联Free，不调用AddCostume／效果Update／Flip／Use703或HUD Recompute补状态。版本仍46，尚无新writer或测试。

**第5步P3—P5待实现合同（2026-11-09）**：按[投影／owner细则](5-player-p3-p5.md)直接安装当前库存／槽／材料，queued仅饰品配置／golden，P4仅六值且空槽清展示负载；不执行获取／使用／充能／到期操作。Plus以本地固定展示组persistent承载有效innate，temporary list与缓存都为空，保留真实官方虚表／容器owner，禁用公开mutator／CheckPlayerType／计时更新；不伪造只有cachedTotals非空的内部状态。P5按真实factory类型构造，五恒零类不写被裁充能，十类只更新公式mask位，Whip真引用保留，其余边保持正常本地owner并按动态类完整撤边后窄Free。Host槽别名按独立本地槽承载，不复用一个指针重复Free；禁用ClearReferences／DestroyWeapon级联Remove／停音。P3特殊查询含Online／RNG等依赖仍须由第8步具体显示路径闭合，缺失不能放行整个getter或旧Lua补值。版本仍46，无新writer或测试。

**第5步P0—P2待实现合同（2026-11-09）**：[必要值及安装依赖](5-player-p0-p2.md)。只安装P0真实闪隐／六比例／headAnim／ItemState／SuplexState／五充能及Sprite投影，P1 CanFly和P2的77B原生存储；不为裁剪字段调用EvaluateItems、Shooting、Player／HUD Update或UpdateHearts。属性栏五值缓存与普通／动态逐心缓存由第8步具体owner安装，EternalHearts／BoneHeartsMask原值不作漂移相等要求。P6有序拓扑版本／资源未匹配、P7角色与held／主备关系未就绪时不装P0也不ACK。758850是Render消费者，不能删五计数；ControllerIndex保持-1，C/S表现选择不使用Host设备键或原版Online getter补值。当前版本46无Player新writer接线。

版本45按[展示来源及残余边界](projectile-effect-residual-evidence.md)安装Projectile新1B开关和独立8B冻结锚点；同版本公共FLAG_FREEZE决定原生分支，不能按最新Position重建锚点或执行冻结随机逻辑。Height临时改写公共PositionOffset.y仍须保存并精确恢复，保留同world Game展示帧；430／444／468不装或漂移比较。Effect按148／149新臂，只安装既有实际展示字段／payload，不执行ChainLightning生成、伤害或Timer callback，不把167的434、119通用尾部及三段空白当可写状态。原五项决定保持；正式owner接线尚待本步。

版本44 Text／Dummy安装按[证据及未决](text-dummy-evidence.md)：文本经有界暂存验证后只安装有效前缀＋NUL，空串／缩短不得复活旧尾巴；TX0安装最终RGBA，固定Text字体包装保留本地正常KColor标签初值，不写／比较Host标签。counter仅146安装，按同world Manager／pause与原生32位运算计算缩放；不推进Update／Interpolate、不装duration或按其退休。字体使用本地Font6，视口／渲染offset由展示owner提供。Dummy仅真实预构造空槽owner取得，继承公共状态／Plus，标签由Type/Variant及本地Font2派生；visible=false导致公共屏幕cache未刷新的历史依赖必须闭合具名来源，未闭合报Gap，不能把当前Screen或旧本地cache当权威值。draw借用状态须精确恢复，不引入原版Online／Isaac Sync。这里约束后续接线，不代表本轮已实现运行应用。

状态：设计稿，未实施。上级：[总计划](README.md)。

## 目标

主线程把应用批次写入原生对象，分为两条路径：

- **结构路径**：成员加入、形态变化、离开，走场景结构事务。
- **热路径**：变化块按生成的分派表写入。

写入完成后回复 `AppliedAck`，并检测、校正 Client 本地状态与影子状态之间的漂移。动态安装的主线程耗时 p95 ≤2 ms、p99 ≤4 ms。

## 输入事实

### 现有原生写入层（保留）

- **场景结构**：`ClientNativeWorld{Prepare,Refresh,Finalize,CheckCommit,Retire}`（`client_native_world.h:68-81`）。
- **Grid**：`ClientNativeGrids{Execute,BindDoors,Retire}`（`client_grid_native.h`）。
- **Entity 与 Player**：
  - Entity 公共字段的原生写入（`client_entity_common_native.cpp`）；
  - Entity 子类的写入（`client_entity_adapters*.cpp`）；
  - Player 的最终量、资源、角色和备份写入（tx-04-20 I11—I27）。

### 描述结构

以下描述结构有一部分字段要靠 JSON 目标填充：

- `ClientObjectDescription`（`client_objects_native.h:42-66`）
- `ClientGridDescription`
- `ClientPlayerDescription`
- `ClientNativeSceneDescription`

### 字段写入现状

公共字段按字符串成员名分派（`client_entity_common_native.cpp:43-50`）；`GetWaterClipFlags` 经 Lua 写入。

## 设计

### 13.1 每个字段的写入方式

布局源为每个状态字段声明一种写入方式，生成器产出分派表：

| 写入方式 | 含义 |
| --- | --- |
| `direct` | 按原生偏移把状态字节原样写回，即复制计划的逆操作。相邻的 `direct` 字段合并成一次 memcpy |
| `writer:<名称>` | 调用具名的原生写入函数。例如 Sprite 加载、`SetAnimation`、容器重建、引用解析、Effect Timeout 由到期帧还原（`effect_timeout`，第 4 步 4.1.5），以及现有的已核写入路径 |
| `construct` | 只在构造时使用，热路径变化时触发形态事务。例如 type、variant |
| `derive` | 不写入，由 Client 原生逻辑重建，需要第 3—8 步给出证据 |

- **`direct` 需要证据**：一个字段只有在证实“直接写内存不会破坏不变量”之后，才能用 `direct`。证据可以是 J273 中对该字段的写入点只有普通赋值，或者现有原生写入正是直接写入。没有证据的字段一律用 `writer`。
- **引用和资源**：引用字段按成员序号查 Client 本地实例表，得到原生地址后写入。目标还没构造时，该块延后到目标构造之后再写，按依赖排序。资源字段按资源序号找到本地路径，交给现有的资源 owner 加载。
- **子类 EntityPtr 按展示职责保留**：版本29将[24项内部关系](internal-entity-relations-review.md)中的23项归Host权威，不生成身份解析或安装；Knife K3保留为空块。Familiar `+DAC` 仅variant240使用布局130恢复真实引用，其他变体保持本地空关系；当前最低展示需求是存在性，不得写伪非空地址。公共关系、Tear.StickTarget、Pickup公开三关系和Laser.BounceLaser继续经SetReference维护；版本39起KnifePlus.hitboxSource留Host，旧本地边仍须正常撤销。目标先构造，退休先撤边，再退独立实例；23槽如因旧实例复用已有本地绑定，也须正常撤边，不直接memcpy零或重演级联Remove。
- **Bomb 懒重建请求**：`+463` 虽归 Host 权威、不采集，Client 仍必须在最终主 Sprite／五份 costume 安装完毕后清为 false，并保持旧执行器的读回检查（`client_entity_adapters_native.cpp` 的 Bomb 分支）。不能因“权威不采集”而省略本地清除，或用 Flags setter 再次置 pending；第 14 步退役旧路径时保留这个行为。
- **版本 28 权威排除**：Knife K4／命中父链选择、四类命中历史、Projectile Damage／Change 三参数、Laser 掉落概率／伤害倍率共 17 个范围不生成安装字段，不再要求本地原值与 Host 相等。旧执行器及检查在第 13／14 步按新职责退役；保留空块号不意味着必须发送或写回空负载。Client 自有容器的正常构造、析构和必要关系撤边仍由本地 owner 管理，见[职责复核](synchronization-field-review.md)。
- **Familiar 间接展示（版本33）**：`61E360→62C0C0/647310` 的FireCooldown／State／Coins继续保留。[Familiar参数／充能／绳索复核](familiar-synchronization-review.md)排除11个轨道／计数／方向范围；80布局131用`incubus_charge_bar_amount`传最终i32百分比，具名writer仅安装本地表现缓存，不写DB0、不创建Weapon。专用展示adapter保留原充能helper的动画／可见性尾部及DB8借用恢复；该adapter仍未接线，不能把旧Player Weapon安装器或置空视为等价。
- **Familiar240 绳索（版本33）**：布局130保留真实DAC引用，新增F7—F10四投影：133号payload的Sprite外观／动画／图层及132号XYZ节点几何。节点为原生0x78、按deque逻辑顺序输出12B，未知模拟字节不上线；所有投影与资源同目标版本齐备后，通过游戏兼容构造及`430E40`共享owner一次安装，不写ECC／ED0裸指针，不调用AI`42D6F0`或Effect的`606310`。非空payload节点数0会导致原生下溢，必须拒绝；DAC先安装目标引用、退休先撤边，再正常退休payload，不重演级联Remove。候选构造／失败清理及实际接线仍待本步。Room Light使用Laser未优化采样点的合同不变。

**版本31公共字段安装合同**：版本30排除的33个范围保持不解码、不安装、不比较；版本31再移除公共valid及`entity_valid_presentation_owner`。普通对象由本地构造／发布／池／退休owner维护valid；Player由P7完整结构事务接管活动／备用资格，不能用热块改池占用，也不能把所有已构造身体置true或把valid当作room_member。旧Player结构源的独立资格在新路径接管前保留。exists／dead继续按最终状态安装，不以成员离表、HP或动画派生，不执行Kill／Remove。原生半帧历史及PauseTimer仍不上线，网络插值如实施仅用本地快照。其余有真实展示用途的碰撞类别、尺寸和SpeedMultiplier保留。具体依据见[公共复核](entity-common-synchronization-review.md)和[生命周期再复核](entity-lifecycle-synchronization-review.md)。

**版本32动画／资源／颜色安装合同**：见[动画／资源／颜色职责复核](animation-resource-synchronization-review.md)。两组动画事件位只在本地构造／切资源／切动画时清理，不恢复Host历史、不漂移比较；最终动画选择、两组selector、anim_frame、is_playing与playback_speed继续安装，官方绘制插值不因此被停用。ANM2 loaded由file_name可空资源选择驱动本地owner：Null安装已构造空资源，非Null等待全部依赖成功后发布／ACK，不直接写true；加载失败不能当Null。LayerState loaded交本地owner，分别安装路径元数据与实际Image，Host待重载但仍使用旧Image时不得仅因新路径抢先LoadGraphics。ColorParams队列不重建／推进／比较，E7为空；Entity.Color、主Sprite最终color、champion_color及图层color仍按完整11分量安装。上述新资源handler尚待第8／10／13步实现，旧路径不能被视为已接管。

**版本33 Familiar安装边界**：11个Host参数不安装／锚定／漂移比较；FireCooldown、State、Coins、IsFollower及原有最终姿态／资源仍按真实展示依赖安装。80的表现缓存随成员generation退休，240候选与资源完整后才替换，失败不ACK；native尾部／节点未知区不复制。详见[Familiar参数／充能／绳索复核](familiar-synchronization-review.md)，新布局不等于执行器已经接通。

**版本34 Knife／Bomb／Tear安装边界**：[Knife／Bomb／Tear参数展示复核](knife-bomb-tear-synchronization-review.md)的42个Host范围不安装、还原或漂移比较；保留Knife效果位、RotationOffset、当前距离／速度及飞行状态，Bomb五份costume／绘制开关与pending=false，Tear效果位／DeadEye及三份辅助Sprite。Height、Scale、Rotation等输入的最终公共绘制偏移／Sprite变换单独安装，不调用GetRenderHeight、ResetSpriteScale、Flags／Scale setter或LoadCostumes求值；不放行原生Interpolate重演运动。新接线仍待本步，不能把生成表视为运行时替代完成。

**版本35 Pickup／Slot安装边界**：[Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)中的Host输入不安装或漂移比较，Pickup只保留Price／DropDelay及原有主／两份辅助Sprite和三引用，Slot保留最终主／奖品Sprite。奖品ANM2的active_animation／is_playing是实际绘制条件，不是Slot.State或PrizeType；price ANM2的loaded由本地资源owner维护。Pickup固定循环数组／count由本地构造与复用owner初始化为空，不恢复Host未来列表或读取池残留。禁止自动价格、MakeShopItem、ReloadGraphics／SetForceBlind、循环Morph、SetPrizeCollectible及购买／捐款／开奖等玩法。新接线仍待本步，保持候选失败／资源依赖和正常退休合同。

**版本36 Projectile安装边界**：[Projectile运动与展示参数复核](projectile-synchronization-review.md)的八项Host运动／效果输入不安装或漂移比较。保留Height：659460在私有+440开关下临时从公共PositionOffset.y减去它，draw scope须保存并精确恢复实际改写的+350，不能只依赖浮点加回。公共EntityFlags、最终位置／Velocity／偏移、尺寸、Sprite及DepthOffset单份映射保持；Game+25A70展示时钟继续供应原Render的局部随机相位。私有+440及+460／+464仍UNKNOWN、来源／安装待补齐，不能补零或称已支持；不运行Update／Interpolate、碰撞／Deflect或+470回调重建轨迹。

### 13.2 结构路径

- **加入**：用类型化视图填写描述结构，取代 JSON 字段，交给现有的场景结构事务构造。
- **形态变化**：退休旧实例，按新形态构造新实例。
- **离开**：按现有的退休流程处理。
- **事务边界**：沿用 `ClientScene` 的结构事务和提交边界，不在事务中途写热块。

### 13.3 热路径

- **写入顺序**：按（成员, 块）调用分派表；每个块内按字段顺序执行。
- **出错处理**：某个块写入失败（例如 writer 返回失败），该成员标为“需要重建”，等下一批次重试；批次里的其他成员照常写入。
- **整批成功才确认**：只有整个批次成功，才回复 `AppliedAck(world_version, body_version, member_epoch, resource_epoch)`。部分失败时回复上一个完整版本，让 Host 继续携带失败的块。

### 13.4 漂移校正

Client 的原生对象可能偏离影子状态。Client 不运行原生 Update，但 Render 和维护 helper 仍会写部分字段，例如维护姿态时推进动画，或者 base draw 写屏幕坐标缓存。Host 只在块变化时发送，如果块没有变化，Client 的偏离就不会被纠正。

- **检测方式**：Client 用与 Host 相同的复制计划，读取本地原生对象，再与影子状态逐块 memcmp。不一致的块加入下一批次重写。
- **执行频率**：轮转执行，每帧只检查一部分成员，总耗时计入安装预算。
- **前提（已定）**：Client 不运行原生 Update。这是 tx-04-17 的隔离设计：现有 Client 拦截 Game、Room、EntityList、Player、PostUpdate 等入口（`client.cpp:28,142-146`），本地不会按玩法推进任何字段。
- **待核问题**：可能造成偏离的只剩 Client 仍在运行的原生路径，即 Render 和维护 helper。例如 base draw `0x6AC4E0` 会写 Entity `+0x1A4/+0x1A8`（tx-04-17 R3）。本步逐类核对这些路径会写哪些字段，据此决定哪些块需要每帧校正，哪些只需要轮转检查。

### 13.5 退役的部分

- `client_entity_fields.inc` 中按字符串匹配的分派，改为生成的索引分派表。
- 描述结构中的 `std::vector<ClientGridField> fields` 等字符串字段。
- `GetWaterClipFlags` 改为原生写入；无法原生写入时，在第 3 步给出证据并归为派生或换算。
- Player 记录上的 `application_draw` 及其 JSON 校验（`client_entity_class_model.cpp:118-124`），改读影子状态中的绘制输入块 D0（13.6）。
- draw 期间逐实体重新锚定 `+0x328`（`client_entity_draw_native.cpp:128,185,195`），以及每次展示前重新锚定 `Room+0x11F0`（`client_room_native.cpp:132-135`）。它们由 13.6 的展示时钟取代。

### 13.6 展示时钟与帧锚点

- **定义**：展示时钟是 Client 最近已应用 world 帧头中的 game_frame（第 9 步 9.3）。
  - 它只在装好新的 world 帧后前进。Host 在静止场景中也发送空帧（第 11 步 11.3），所以时钟会照常推进。
  - Client 自己的 `Game+0x25A70` 不代表 Host 时间，不能作为换算基准。
- **展示期间**：沿用现有借用方式（`client_presentation_native.cpp:826-829`、`client_entity_draw_native.cpp:238-241`），进入展示时换入下列 Host 值，结束后恢复 Client 原值（同文件 136-145 行）：
  - `Game._frameCount`（`+0x25A70`）← 展示时钟；
  - `Manager._framecount`（`+0x4A8B8`）、Pickup 绘制 RNG（VA `0xBD0CA4`）、Familiar 绘制 RNG（VA `0xBD0674`）← 影子状态中的绘制输入块 D0（第 8 步 8.2.1）。D0 与展示时钟来自同一个 world 帧。
- **帧锚点字段**：`Entity+0x328`、`Room+0x11F0` 的基准按原值写回，不再逐次重新锚定。展示期间，FrameCount 和 `Room:GetFrameCount()` 由原生 getter 用"时钟 − 基准"自然得出。
- **Effect Timeout**：每装好一个新的 world 帧，按新的时钟把全部 Effect 的 Timeout 重写为"到期帧 − 时钟"（`effect_timeout`）。
- **第 4 步子类计数**：Familiar FireCooldown的明确方向锚点先经 `ValidEntityCounter` 校验，再用同一展示时钟调用 `DecodeEntityCounter`；版本33起HeadFrameDelay留Host，版本34起Bomb ExplosionCountdown亦留Host，不再安装、换算或漂移比较。只还原已核分支的值，不运行射击／爆炸等玩法；未知活动条件由 Host 报缺口。共享 payload／Sprite vector 的分块投影计数须一致，并经 `ValidContainerCountGroups` 核对合计，蝗虫两组合计最多 16384。真实编码长度不得超过 `Field.max_size`，全部依赖齐备后交具名 owner 一次安装。
- **Tear／Pickup／Laser**：版本34起Tear WaitFrames／StickTimer留Host，版本35起Pickup Timeout／Wait亦留Host。版本37起Laser Timeout／RotationDelay亦留Host；此组三类只对Pickup DropDelay按真实条件编码随world展示时钟还原，不套用通用无夹限计数。Tear 三份、Pickup 两份辅助 ANM2 分别交资源 owner；HomingLaser只对optimized／unoptimized两组最终vector由各自owner构造候选，完整同目标版本依赖就绪后安装，独立count不要求相等，空集合清除旧点；保留本地self，indices中间缓存与Bezier保留正常构造的本地空vector及正常析构，不运行CalculateSamples。Pickup未来循环道具自版本35起不安装，本地owner维护安全空列表／count；不得重建Flip保存玩法对象。版本29只对Tear.StickTarget、Pickup公开三关系及Laser.BounceLaser解析成员身份，经SetReference安装、退休前撤边；已归Host权威的23条内部关系不安装，其本地空槽和旧绑定清理遵循13.1，防止原生Remove误删独立对象。见[本轮实现](implementation-4-tear-pickup-laser.md)。
- **Slot／Projectile／Effect**：版本35起Slot只安装主／独立奖励ANM2，八项原生i32／i16／u16／u32玩法参数不上线，其真实宽度与未知仍保留，不运行SetPrizeCollectible／开奖；Projectile自版本36起只保留子类Height，DepthOffset复用E4；八项运动／规则输入及此前Damage／Change参数不上线，不运行半帧模拟。Effect 三个附属臂先用 `DecodeSharedOwnerProjection` 核 present、尾块依赖及缺失值最短全零；存在时再递归校验 payload。全部同目标版本投影齐备后创建具体候选，经 `0x606310` 替换共享 owner，不逐投影换 owner。FX1 IsFollowing／ParentOffset依赖E8 Parent；版本38起136描述不在Effect或Grid中安装，持物关系／BodySprite／最终主Sprite仍须齐备。见[实现边界](implementation-4-slot-projectile-effect.md)。
- **第3步保留的计时**：版本30排除Entity公共倒计时、PauseTimer和毒／火伤害相位，不生成还原调用。版本32再排除ColorParams，不生成Lifespan还原、元素重建或颜色时钟见证；淡出／覆盖直接安装Host最终Sprite ColorMod。子类／Player的真正展示计时继续各自合同。EntityPlus optional由原生owner赋值（`DecodeFlyingOverride`／`DecodeWaterOverride`），不memcpy。
- **对照与校正**：E2E 对照（第 15 步）和漂移校正读取这些字段时，用同一个时钟换算。

## 改动范围

- **新增**：分派表、引用依赖排序、漂移检测。
- **修改**：`ClientScene`、`ClientObjects`、`ClientGrids`、`ClientPlayers` 的输入，改用应用批次和类型化视图。
- **保留**：原生写入函数、结构事务和退休流程。

## 失败方式

- **直接写入破坏不变量**：`direct` 写入绕过了原生 setter 维护的不变量，例如缓存、索引或派生值。
- **引用目标未构造**：引用指向的目标还没构造就写入，写成 Null 或悬空地址。
- **部分失败仍确认**：部分失败后仍然回复了新版本，Host 停止携带失败的块。
- **校正形成振荡**：漂移校正与本地自然推进的字段来回改写。这类字段应改为每帧覆盖，或者归为派生。
- **漂移检查超预算**：检查本身超出主线程预算。
- **结构事务中写热块**：在结构事务中途写热块，对象处于半构造状态。
- **展示时钟错位**：
  - 展示期间没有换入展示时钟，或者用 Client 自己的 `Game+0x25A70` 换算，导致 FrameCount、`Room:GetFrameCount()` 和 Timeout 错位；
  - 装好新帧后没有重写 Timeout。
- **绘制输入未借用或未恢复**：展示期间没有换入 D0，Pickup、Familiar 用 Client 本地的 RNG 和 Manager 帧绘制；或者展示结束后没有恢复，Client 的全局值被常驻改写。

## 完成内容

- **实现与编译**：分派表、结构路径、热路径、漂移校正实现完成，纯编译通过。
- **代码约束**：旧的字符串分派已删除；所有新函数和数据结构带中文注释。

## 未决问题

- 哪些字段可以用 `direct`，需要第 3—8 步逐字段给出证据。
- Client 仍在运行的原生路径（Render、维护 helper）会改写哪些状态字段。这决定哪些块需要漂移校正。

**版本37 Laser安装边界**：[Laser／Homing采样、运动与计时复核](laser-homing-synchronization-review.md)排除十九项Laser输入、六项Homing标量及中间缓存／系数；不写回、不作漂移比较、不恢复两个Laser计时锚点。保留直接展示字段、两组最终点列、BounceLaser及公共最终姿态／Sprite，不能重算追踪／旋转／收缩／采样。旧实例中间容器若有旧分配须经owner正常释放，不memcpy零头；不得将其owner与保留点列共用，也不通过ClearLaserSamples／Plus重采样请求破坏最终点列。不运行Update／Interpolate／接触／碰撞或级联Remove。

**版本38 Effect安装**：按[Effect各臂与payload复核](effect-synchronization-review.md)六项Host参数、四变体TearFlags和136描述没有writer／原值读回／漂移比较；State、Timeout到期帧及其余真实展示输入保留。136只用最终主Sprite和Player持物展示依赖，不创建active Grid或执行举持／落地。MrMe只建表现ANM2，四项私有状态留本地正常初值且不让玩法消费。Trail用独立64B节点重建逻辑deque、保留完整历史几何与参数，不重算累计距离或衰减。Ghost按三投影一致前缀安装全部资源／姿态，末尾无效槽经正常ANM2 owner清理，原生i32 count由共同count重建；其Offset／Scale由已核Render纯展示公式在draw scope派生，保存并精确恢复，既不传Host原值也不修改通用ANM2合同。保留全部本地构造／资源／共享owner生命周期；失败不部分替换，不调用Update／Interpolate、Spawn／传播、loot选择或Trail维护。

**版本39装备／Plus安装**：按[完整复核](player-equipment-plus-synchronization-review.md)四主动槽安装六值、五Weapon槽依真实工厂／动态类安装四值及15号主引用，P4／P5／P8与角色／库存／资源同目标版本；不写或比较来源池／定时充能冷却、五项Weapon输入、Knife hitboxSource。Whip EntityPtr目标先构造再绑定，Familiar同时满足Player／D74／D76／619／type32依赖；D76从Host最终bool安装，不执行跟随逻辑。TemporaryEffects完整配置／count／cooldown和disabled同版本提交，cache mask／evaluate不上线，Player回指由本地P8 owner维护，不调用Add/Remove/ClearEffects／EvaluateItems。两个进度条保留真实动态getter、动画和精确draw恢复；HUD图像／flash等由第8步owner准备，禁止用完整HUD Update刷新。Plus只在同代正确动态类的现有本地holder/data上标准optional赋值，保留Null／false／零；缺失失败不得分配fallback、清整个派生Plus、部分发布或ACK。其他已排除Plus项不恢复。旧Weapon替换须正常撤所有自身EntityPtr、owner=null后窄Free，不用会Remove攻击实体的ClearReferences／DestroyWeapon。
**版本40 Effect后续五项安装合同**：见[证据与落地](effect-next-five-evidence.md)。按11布局及其生成表安装；FX0的Rotation／Scale随State／Timeout同目标版本提交，FX1仍须等待E8 Parent，不能继续按旧8／21B字段偏移写入。每个已应用world帧对全部Effect以该帧F还原T=A−F，不预测、不夹限负数、不调用Update或SetTimeout补递减；101双减／141刷新反映为Host发送的新锚点。Trail正常owner只重建逻辑0—127个独立64B节点，保持全序、零alpha边界节点和全部参数；不运行追加／裁剪／衰减，空payload不同于缺失。原生ExtraData只允许既有108／166／191三种具体类型，全部投影齐备后经606310一次替换，错variant／未知类型不造fallback。137—141已排除输入不安装、不回调、不Spawn／传播；未知字节及无效holder不会变成原生写入目标。实际接线仍由本步完成，未运行测试。

**版本43 NPC现行合同**：C/S受控UI只以同版本0/2旁表结果消费：第一段6D025D入口，0走6D02C5、2走6D0285；第二段6D0314入口，0走6D03AC、2走6D0340。必须绕过整段Manager+4AFB8读取及原版IsIdxLocalPlayer，不能只替换6D027C/6D0337调用。403物品独立HP条路径、Sprite后续就绪/alpha及可见检查保持；不写F1C，不注册/重绑输入，不全局Hook8E7550。NPC另在6CF09D读取NetplayManager作Spawner为Player时的远端淡出筛选，C/S采用无原版远端筛选语义，具名draw adapter须绕开该整段至6CF0EB，不为它新增网络字段。该点列入本步既有隔离接线，未提前安装Hook。Boss选择不重建、OverlayEffect Scale派生及其他复合owner合同保持。见[来源闭合](npc-control-ui-evidence.md)。


**版本46 Delirium安装合同**：见[companion消费与绘制依赖](delirium-residual-evidence.md)。两个Host u64打包值、Boss选择和BE8均无writer／读回／漂移比较，不能扩大官方u32／u8接口或恢复旧字符串字段写入。410的Child与4A13B0生成的companion按真实独立成员及现行公共关系／资源展示，不根据BE8创建第二个NPC或连带Remove；本地Init空槽保持为空，异常非空交合法owner，不能裸清登记。稳定公共shape下的资源／姿态改变仍更新实际块，不强制退休重建根NPC。143根UNKNOWN为590B，BD4—BD7不写；17／18空预留、710B下界和其他NPC合同保持。实际安装接线仍属本步。
