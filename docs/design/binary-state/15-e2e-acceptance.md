# 15：E2E 适配与统一整体验收

**第6步影子描述代码（仅本步执行验证）**：按[已实现接口与失败方式](grid-description-implementation.md)覆盖17类／27种type的完整记录生成、错key/use/shape/layout、缺块／未定义块／预算、合法旧变化版本、Statue三空块缺席和224无Seed。检查描述／投影复制共享自有字节及租约、借用撤销后内容不变、不同角色真实nested_layout和ANM2条件投影仍受第12步递归校验；完整u32水面、Door负索引／两个布尔、Fire16B粒子与Rock有序重复偏移保持。旧JSON legacy描述不得隐式进入正式安装。第10／12／13步运行接线完成后再验证实际游戏行为及AppliedAck，本轮仅纯编译未执行测试。

**2026-11-10 Presentation待验收范围**：按[具体清单](presentation-synchronization-review.md#8-后续正式布局与唯一验收)覆盖四参与者/多身体/动态HUD、24心与特殊健康/HolyMantle/低血、StatHUD0—8行/两列、11消息槽/fortune/程序图、History顺序/负ID/shader、各端独立地图展开/视口/挑战32/黑框、屏幕/转场/死亡/暂停只读结果，以及crossfade/jingle/各端静音/loop中途加入与停止/one-shot去重。确认不运行HUD/Map/Overlay/概率/分数生产器，不触及Online或访客进度；未知来源/图像失败不伪装为空后ACK。本次未提前执行测试。

**版本55 Grid回映射消费口径（仅本步执行）**：按[正式119行](layouts/grid-catalog-members.md)选择实际布局及处置；Lua对照以目录路径＋行号保留第2/8、61/63、105/110行的属性／方法区别，不按ID去重。field／derived只比较对应展示投影，不恢复74行Host原值相等；ReducedSpawnRate第92行不适用，不探未知字节冒充该属性。SpawnSeed的198／224、GetSprite的实际nested_layout、Rock条件主姿态／layer0及无符号GetBigRockFrame须分别覆盖；Render／RenderTop行不授权重放原生副作用。正式reader／writer、七族水面、资源／pass来源和借用恢复仍须实际接通并在本步验收，生成／纯编译不算测试通过。本轮未执行测试。

**版本54 GridEntityPlus（仅本步执行）**：按[来源及失败方式](grid-plus-water-source-evidence.md)覆盖七族、Null／覆盖0／高位完整u32、Reset恢复各自默认；特别覆盖Lock设置override无效、同index门／装饰／Lock的不同最终值，以及同index其它对象Factory／Init导致共享重置、Init内重新Set。Remove／换房不假设官方自动清，复用／停用／退休撤本地借用；缺模块／错ABI／越界／非0或1 engaged／读失败／错use不ACK。七个C/S原链各一次，Lock须有新增覆盖点，out+4保持零；验证仅最终值与画面，不比较optional／填充或制造其线上负例。旧JSON的<=255限制实际迁移前不得记高位通过。本轮没有执行测试。

**版本53 PressurePlate／Door（仅本步执行）**：按[失败方式](grid-pressure-door-evidence.md)覆盖普通／Greed板、真实TimerPlate有无资源与非variant2已加载、计时0／负值／分钟边界／Host帧闪烁与震屏；门alpha零／负／正及幅值不影响入口、墙色与extra独立颜色／ExtraVisible、target type27／15及负特殊索引／目标Flags40000、Light bit0x20／维度／layer3/5／独立glitch、方向最终双旋转、门别名／跨房／释放与资源等待不ACK。previous／动画名／其余flags／Seed变化而最终投影不变不应单独重发；不恢复被裁原值相等或为其制造线上负载。拒绝非法布尔／截断／非有限／错use与未知target依赖；根31／32B UNKNOWN包含Door27F，不能以阴性结果判填充。正式owner未接线不记通过，本轮没有执行这些测试。

**版本51 八类Grid覆盖（仅在本步执行）**：按[完整失败方式及静态边界](grid-eight-subclasses-evidence.md)覆盖八类真实RTTI／Factory／重建、Decoration隐藏variant1000、五类地板色变化与draw恢复、Wall在16／非16房间、TrapDoor计数不发但最终开闭同步；Fire双附属Sprite、主模板不同残留帧／独立overlay／is_playing插值、state>=4空投影及重新可见、有序0／5粒子与删除、24B原生／16B线元素、跨房／退休／Free所有权及三Sprite各自Dataminer选择。拒绝count>5／容量或长度不符、非有限位置、maxLife=0或>29、age<0或>=maxLife、错版本／use／资源；状态<4的负数仍按原生有符号判断处理。只比较保留的实际展示状态，不恢复计时／样式／速度／必覆写颜色／旧模板帧相等要求，不为已删除字段制造线上负例。E27保留七类根17B、Fire45B UNKNOWN；draw／资源／容器owner尚未接线不能记通过。本轮没有运行这些测试。

**版本50 Grid验收修订（仅在本步执行）**：按[复核](grid-common-synchronization-review.md)比较最终水面值与实际画面，不比较Host CollisionClass、默认动画名、图层待加载路径或旧来源元数据。原Grid／E09／E25／E28／E29覆盖七原生水面族、nullable未设／有值0／高位u32与同index门/装饰，Client+3C本地中性初始化、wrapper原链各一次、无碰撞玩法重演；覆盖路径先改但仍画旧Image、显式动画／selector与资源失败不ACK；覆盖Dataminer两端不同ANM2地址、0—3四种最终纹理选择、pass开关／暂停／use换代／角色切换及draw精确恢复，不能按Client地址生成后记通过。E13／E17拒绝glitch=4/255、旧版本／截断／坏资源／index／角色绑定，合法u32水面值不沿用旧<=255限制。E27根17B与各外部图层4B未知保持。两种合成字段实际来源／writer／绘制adapter缺席时保持未通过，不恢复已删除负载；本轮未运行任何测试。 冠军着色验证前四实际GPU参数，拒绝16B截断／非有限值；后28B不上线不比较，普通颜色全部11值继续覆盖。

**版本48 Grid公共验收口径（仅在本步执行）**：按[公共归属与未知区](grid-common-evidence.md)，原Grid场景及E09／E25只比较保留状态与最终画面，不再要求Host SpawnCount／VarData／Initialized／VariableSeed／SpawnFrame或对象_rng在Client原值相等。E27保留对象+1D—1F与ANM2递归14B未知，共17B；不得将未观察变化说成填充。E13／E17在真实子类布局接入后覆盖index447合法／448及65535非法、身份／门别名冲突、来源失效与实际资源／长度错误；不能用抽象公共前缀199代测派生类。E28／E29核Client无Init／PostInit／Update／奖励／玩法RNG重演，本地owner安全退休及临时颜色／Image／visible精确恢复；Light验证SpawnSeed＋同world Host帧，覆盖Pause／帧跳变，不能以本地时钟代替。Plus水面覆盖及Room颜色／压力板文字等实际依赖须闭合才可记相应用例通过。本轮未执行测试，不新增验收编号。

**版本47布局来源（2026-11-09）**：[第5步正式源／生成](implementation-5-player.md)已完成，EntityPlayer=197，participant／control-group=195／196，Player主动槽=179，条件效果=180，动态Weapon=181—194；物理0—24及条件／固定槽／预算元数据以唯一源和生成表为准。以下版本46／草稿／待生成表述保留原时点；本步骤的运行接线仍未实施，不能因布局已生成就提前接收、采集、安装或宣称验收通过。

**第5步P6—P8设计覆盖（2026-11-09，仅本步执行）**：[完整失败方式](5-player-p6-p8.md)要求覆盖飞行／overlay／Room Light配置身份、15 map及negative map的body标记、明确无ANM与资源等待、原位重排／跨backup／held use变化和统一拓扑body拒绝；四参与者／64身体组与角色主备／sub／Flip／703变换后实际成员，独立valid不由room／Exists推导，不把Host sub／replaced／四槽嫁接Client。效果须覆盖重复first-match／disabled／704角色环与非27主动HUD冷却，其他cooldown变化不重发；HistoryHUD负ID／633／88000与真实库存图像列表、原始History变化而最终缓存不变、Render内分数重算隔离及最终分数来源时点。残影按全部元素／顺序／duration／位置比较；可选序列只装当前节点与实际ANM，D=0／second=-1空负载、未选节点不重发、0／1本地list及CPU借用／异常退休均覆盖。P3—P5旧段的“703备份依赖”以本次裁剪取代：仍核VarData，明确不要求四槽下行。非法截断／条件长度／引用／bool／预算／早ACK及原三端性能保持；本轮没有构建或执行测试，版本仍46，第15步是唯一统一验收。

**第5步P3—P5设计覆盖（2026-11-09，仅本步执行）**：[字段与失败方式](5-player-p3-p5.md)要求覆盖完整dense库存／熔炼黄金位／wisp／附加计数、blocked∩innate、legacy／多group相同有效计数、timer推进而投影不变及到期有效计数变化；检查本地投影容器合法且无计时／回调。四Pocket含后三槽、空type与ActiveSlot+1、queued饰品golden、crafting八位置／poop六位置与实际HUD最终结果必须覆盖，不能用counts代替显示列表。P4覆盖四槽、occupied三段、空槽旧负载不参与、partial真实浮点／VarData／703备份依赖。P5覆盖1—15真实factory大小、10／11同类不同type、五恒零类不读／不比较充能、十类mask位影响／无关位不重发、first-positive与first-nonempty顺序、Whip实际引用及独立退休；不由槽指针相同就制造双Free或用ClearReferences停音／删实体。非法截断／count／ID／mask／错view／非有限／资源未就绪、四参与者多身体／恢复与三端性能按现有整体验收覆盖。P3跨域计算查询最终显示来源的未决须实际闭合后才记该项覆盖，三组有界阴性输入不新增原值比较，不能通过省略画面宣称通过。本轮无Player新实现，未执行这些测试，版本仍46。

**第5步P0—P2设计覆盖（2026-11-09，仅本步执行）**：按[定稿与失败方式](5-player-p0-p2.md)覆盖原生13B8闪隐与BlinkLock区分、五种Render充能／真实Weapon与704效果依赖、29／38六比例／headAnim、主Sprite E6同body、costume同count原位重排／拓扑换代及资源未就绪。健康覆盖834930普通缓存及83B160特殊分支、永恒心overlay／骨心排列、BlackHearts掩码、负额外生命修正、Jar／Urn／魂血／poop资源及真实多身体backing；不比较不上线的EternalHearts／BoneHeartsMask原值或Player属性生产历史，仍比较最终属性栏五值缓存／心缓存。拒绝P1／P2混入body、P0截断／超长／非有限值、错误P6版本和不一致投影count；在四人／多身体／三端扇出下验收变化块大小、安装与总带宽，不能用全量下界声称性能通过。当前没有Player新布局或reader／writer，本轮未执行这些测试，布局版本仍46。

**版本46 Delirium覆盖（仅本步执行）**：按[原未决处理](delirium-residual-evidence.md)，E09／E25覆盖经Init及不经Init的变身、资源／几何变化、410 Child和其他实际companion独立成员，稳定公共shape下不得额外重建根NPC；E28／E29检查BE8没有Client写入／玩法消费／重复创建／级联Remove，正常引用撤边和独立成员退休继续覆盖。两个Host打包值及官方u32／u8查询不作跨端原值／漂移比较，也不为它们新增线上非法负载；17／18仍为空预留。E27的Delirium专有范围由11B收窄到BD4—BD7的4B，根590B，其他既有未知保持；BDD／BE4已经归入打包存储，不重复当未知。池残留只记录自然同槽前后不同use的实际原值；静态可达不算已观察，不通过强写或重复Init制造样本。无样本如实记录，阴性结果不证明padding。第4步本轮没有执行这些测试。

**版本45 Projectile／Effect残余覆盖（仅本步执行）**：按[逐项证据](projectile-effect-residual-evidence.md)，E09／E25／E28比较Projectile Height＋440零／非零开关＋独立冻结锚点，覆盖锚点不等于当前Position、默认锚点、冻结结束／Remove、非零PositionOffset、Shadow／水面及draw精确恢复；不为测试触发Client冻结随机或原生Update／Interpolate。E13／E17拒绝PR0／PR2截断及非法浮点，u8=2等非零原值应合法，勿误当bool。Effect覆盖148的167、149的19／71／194、原102的54／126及119已知／其他callback，不开放不适用公开API、不比较不上线的伤害元数据、内部flags或callback地址。E27观察Projectile本类434四字节／441三字节／44C四字节／474四字节及公共335B；Effect按新残余表保留167的434及438后8B、119按callback复用区、其他未核variant、三段41C／44C／4BC和其他空白，不用阴性结果伪造填充。此前五项既有覆盖保持，不新增独立验收编号；本轮未执行测试。

**版本44 Text／Dummy覆盖口径（仅在本步执行）**：按[字段决定](text-dummy-evidence.md)，E09／E25覆盖145基础臂、146的variant2、147真实Dummy RTTI，Text最终RGBA／字节与variant2缩放、Dummy正常可见标签与公共Sprite；不比较Text duration／颜色空间标签，也不为基础臂发counter。E13／E17拒绝count256／65535、长度不符、内嵌NUL／未终止、失效窗口、来源变化、旧版本／错臂及RGBA非有限值；非UTF8、空串和255B有效前缀合法。E28／E29覆盖writer缺席、slot复用／Init／退休撤销、长→短→空串、同world Manager／pause切换、字体未加载、无Update／Interpolate／到期Remove及draw恢复；跨视口与Dummy visible切换／Baseline进入不可见实例的历史屏幕cache依赖必须实际闭合，缺来源如实记Gap，不能按当前位置重画后记通过。E27包括Text52C—52F和两类公共335B未知。Dummy自然来源未核事实保持，不用Spawn964生成的NPC代测或宣称自然可达；合法测试池桥仍仅限E2E构建。第4步本轮未执行上述测试。

状态：设计稿，未实施。上级：[总计划](README.md)。

## 目标

本步做三件事：

- 把 E2E 工具改为验证二进制状态路径；
- 加入两类新的检查：未知区探测和 Lua 对照；
- 修订 tx-04-21 的 E01—E26，作为整个重构**唯一的**统一整体验收。

本步之前的 0—14 步不运行任何测试。本步开始后连续执行，遇到问题就地修复并续测，不另开编号。

## 输入事实

- **原验收清单**：[tx-04-21 E2E 方案第 4 节](../tx-04-21/tx-04-21-e2e-plan.md)，E01—E26 两人全套和四人扩展，以及第 3.1 节的指标。
- **原清单的进度**：tx-04-21 第 73 轮 E02 未通过，E03—E26 未执行（[进度](../tx-04-21/tx-04-21-e2e-progress.md)）。
- **E2E 构建**：E2E 工具只进入 `ISAACCS_BUILD_E2E_TOOLS` 构建（`mod/CMakeLists.txt:94-98,151-157`）。

## 设计

### 15.1 新增的 E2E 能力（只在 E2E 构建中）

| 能力 | 内容 |
| --- | --- |
| Lua 对照 | Host 侧用观测 Lua 读取公开成员，按回映射表（`state_members.inc`）与同一次采集的二进制状态逐成员比较；Client 侧在原生写入完成后读取同样的成员，与 Host 来源比较。“派生”成员只在 Client 侧比较。`Entity.GetLastParent`／`GetLastChild` 在首边为空时，J273 静态 getter 返回自身而 Wiki 写 nil，本项以实际结果为准并记录差异（第 3 步）。 |
| 未知区探测 | 对每个布局的未知区间，附加一份诊断复制计划，在覆盖的场景中逐采集记录是否变化。输出每个区间的结论：变化、未变化或未覆盖 |
| 帧统计 | 每个布局、每个块的字节数和变化频率，Baseline 大小按布局拆分，Host 与 Client 的分段耗时 |
| 故障注入 | 对状态帧、成员表、资源表注入以下错误：截断、错 record_bytes、错 block_mask、错 epoch、版本回退、非法值。另可临时屏蔽 D0，构造 record_count=0 的空帧（E33） |

处理规则：

- **未知区发生变化**：说明该区间含有状态。在本步内对它归类、补布局，然后继续测试。
- **未覆盖的区间**：如实记录，不能宣称“完整”。

### 15.2 E01—E26 修订

原条目的场景和指标保留，判定依据改为新格式。主要修订如下：

| 条目 | 修订 |
| --- | --- |
| E01 | 握手比较布局版本；版本不符时拒绝，并报具名原因 |
| E02 | Baseline 改为“成员表 + 资源表 + base=0 状态帧”；体积 ≤1 MiB、时长 ≤10 s；按布局拆分体积 |
| E03 | 指标不变；另报主线程采集按域的耗时、每帧字节数、变化块比例 |
| E05、E06 | 增加“变了又变回”和“最后一次变化后静止”的收敛检查 |
| E09 | 逐字段比较改为：Host 二进制状态 ↔ Lua 对照 ↔ Client 原生；按回映射表覆盖全部 `ReplicatedState` 成员 |
| E10、E11 | 改为验证：丢帧后由下一帧自愈；ACK 丢失后已确认版本不回退；Resync 回到 Baseline |
| E13 | 增加错误 member_epoch、resource_epoch，未登记的成员序号，以及布局编号不符 |
| E15 | 改为：块写入失败后，确认停在上一完整版本，Host 继续携带失败的块 |
| E17 | 增加 15.1 中的全部格式故障注入 |
| E25 | 改为 18—20 的应用回归，加上回映射覆盖报告 |
| 原补充矩阵中的旧内容（63／64／65 项分块、graph_ref、参数矩阵） | 改为容器上限边界、Mod 值编码的共享与循环、查询派生检查 |

### 15.3 新增条目

| 条目 | 内容 |
| --- | --- |
| E27 | 未知区探测：所有布局的未知区间都有结论；变化的区间已归类并重测。包括 J273 池步长或工厂分配大于 SDK 声明的尾部字节：Effect、Knife、Slot、Projectile（第 4 步），Door、TrapDoor、Fire、Poop（第 6 步） |
| E28 | 漂移校正：Client 不运行原生 Update，漂移只来自仍在运行的 Render 和维护 helper（第 13 步 13.4），例如 base draw 写 Entity `+0x1A4/+0x1A8`。这些路径改写的字段被检测并纠正，没有振荡，耗时计入安装预算 |
| E29 | 成员序号回收：大量生成与移除后，序号在所有 Peer 确认之后才复用，旧帧不会写到新成员上 |
| E30 | 资源表：同路径重载、贴图替换、新资源首次出现都正确，序号不复用 |
| E31 | Mod 域：具名 adapter 的二进制编码在循环、共享、超预算、adapter 出错时的行为 |
| E32 | Effect 十一个布局（第 4 步 4.1，版本40）：Host 选臂与 Client 布局一致，覆盖以下场景：<br>- variant136举起的格子：最终主Sprite、State、Player持物关系／BodySprite完整，Effect与Grid均不发描述；<br>- 102的54／126／167及141的113分别选臂，最终效果与派生成员完整，FX2留空且Client不重演规则；<br>- MrMe（108）、SpriteTrail（166）、PickupGhost（191）的创建、轨迹增长／缩短与移除，Ghost 0—4前缀及缩短／再增长；<br>- 137传播、138Timer、139生成、140计数只传最终展示，不执行Host请求／回调；其他未核variant走基础臂；同一Effect的variant变化走形态变化 |
| E33 | 展示时钟与空帧（第 11 步 11.3、第 13 步 13.6）：静止场景中 Host 每次采集仍发出帧，Client 展示时钟随帧头 game_frame 前进。另在 E2E 构建中临时屏蔽 D0，构造 record_count=0 的空帧，确认它被接受、确认并推进时钟 |
| E34 | 帧锚点（第 3、4、7 步）：静止和移动场景中，Client 的 Entity FrameCount（`+0x328`）、`Room:GetFrameCount()`（`+0x11F0`）、Effect Timeout 按展示时钟换算后与 Host 一致；仅在已核的线性计时区间要求锚点不变；Effect负值／停走／101双减／141刷新等按真实A变化重发并同帧还原，静止位置不保证锚点不变 |
| E35 | 绘制输入（第 8 步 8.2.1）：D0 与帧头 game_frame 来自同一窗口；展示期间 Client 的 Manager 帧和两份绘制 RNG 等于 Host 值，暂停时 subframe 为 0；展示结束后恢复 Client 原值；Pickup、Familiar 的绘制效果与 Host 一致 |

### 15.4 执行方式

**版本36 Projectile口径**：按[Projectile运动与展示参数复核](projectile-synchronization-review.md)，八项Host运动／规则输入不上线、不做原值／漂移比较。E09／E25覆盖追踪、左右曲线、加速／减速、下落速度／加速度、摆动相位、Scale选图及ProjectileFlags组合产生的最终轨迹／大小／Sprite／独立效果；Height覆盖+440两分支与公共偏移非零、Shadow／水面／排序及特殊绘制路径。E28／E29确认没有本地Update／Interpolate／碰撞／+470回调，+350及Sprite借用精确恢复；Game帧展示借用仍供应原Render局部随机相位。E13／E17继续检查Height有限值及真实资源／引用，不再构造八项被撤销字段的非法负载；PR1预留不作为构造必收。E34不新增摆动／其他运动锚点；E27保留全部375B未知，私有+440及+460／+464来源仍未闭合，不能跳过场景后称画面完整。只安排第15步，未提前执行。

**版本35 Pickup／Slot口径**：按[Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)，20个原生范围留Host、19项原线上字段／容器撤销，不比较其原值或未来列表。E09／E25覆盖选择组拾取后的成员退出、触碰／充能、自动价格／补货重抽、盲目图层、循环切换后的最终成员与Sprite；保留Price／价格图形和水面反射、DropDelay正值隐藏／归零显示。Slot覆盖1—18变体的支付／捐款／触发／开奖后最终Sprite及6／15奖品Sprite的动画与播放条件。E28／E29确认无本地定价／循环Morph／支付／开奖／资源生产setter／持久副作用，检查循环本地安全空初始化、资源借用和正常退休。E34撤销Pickup Timeout／Wait，仅保留DropDelay与其他真正线上计时；Slot八个标量也不建立锚点。E13／E17不构造已撤销字段／循环数组的非法负载，继续检查真实资源／选择器／引用与DropDelay编码；预留块不作为构造必收负载，E27未知不变。只安排第15步，未提前执行。

**版本34 Knife／Bomb／Tear口径**：按[Knife／Bomb／Tear参数展示复核](knife-bomb-tear-synchronization-review.md)，42个Host参数不做原值相等／漂移比较或线上字段拒绝用例。E09／E25覆盖Knife投掷／挥砍／旋转／路径／分裂及拖影，Bomb下落／火箭／爆炸／不同范围伤害Flags和Scale对应的最终效果，Tear等待／粘附到期、下落／追踪／父跟随及不同高度缩放旋转的最终位置／Sprite；保留Tear flags／DeadEye的Room Light、排序／水面与所有辅助Sprite。E28／E29确认未放行Update／Interpolate、爆炸／击退／LoadCostumes／ResetSpriteScale／高度重算，Bomb pending清除及owner退休保持。E34不再比较Bomb ExplosionCountdown、Tear WaitFrames／StickTimer锚点；其余真正线上计时保持原合同。E13／E17继续校验真实线上浮点／引用／资源／容器，E27未知范围不变。只安排第15步，不提前运行。

**版本33 Familiar口径**：按[Familiar参数／充能／绳索复核](familiar-synchronization-review.md)，11个Host参数不做跨端原值比较或线上拒绝负载。E09／E25检查轨道最终运动、Hearts／Keys／清房奖励后的最终画面和方向／射击动画；80覆盖原版Weapon充能族、modifier、0／1／中间／100、空Weapon、出生6帧／隐藏限制和四动画阶段；240覆盖payload空↔非空、DAC空↔非空、逻辑环顺序、全段／后半段、节点Z／反射及Sprite资源变化。E13／E17拒绝amount越界、非有限XYZ、非空payload节点0／超限、非法map／错类型与投影count不一致。E28／E29覆盖DB8借用恢复、候选失败／释放、真实引用撤边及无玩法重演；E27增加payload尾44B、嵌套ANM未知与每节点108B未知观察。布局与来源已补入，运行时handler和owner仍未接线，不能标通过。

**版本32动画／资源／颜色口径**：按[动画／资源／颜色职责复核](animation-resource-synchronization-review.md)，不比较Host事件位、两个loaded原始字节或ColorParams队列／锚点，也不为这些已排除字段构造线上拒绝用例。E09／E25覆盖主／叠加／逐层／空层独立姿态、播放／停止与不同速度的官方绘制插值，以及颜色覆盖优先级、淡出、正／负Lifespan与shared传播后的最终Entity／Sprite／图层／costume颜色；完整ColorMod的11分量继续对照。E28确认事件本地清理、无事件重放／本地颜色合成及官方插值临时改写恢复。E30覆盖Null↔非Null文件资源、Client等待／加载失败不得ACK、路径已变而仍画旧Image、随后实际Image替换及Null Image；本地准备结果不由Host布尔覆盖。E34不再覆盖ColorParams时钟，其他展示计时保留。新资源handler与未核Player／Grid／HUD owner未接通不能标通过。

**版本31生命周期口径**：按[再复核](entity-lifecycle-synchronization-review.md)，公共valid不做Host↔Client逐字节相等对照；E09／E25／E28／E29覆盖本地发布／池资格、角色29／38互惠backup复合绘制与递归终止、Player结构激活资格、非房间但exists=true的备用存储以及尚未退休的死亡展示。exists／dead继续按回映射比较；不能通过Kill／Remove或同步Host池字节取得期望值，也不能默删旧Player结构资格源。P7编码与writer未接通不能标通过。

**版本30公共字段口径（valid部分由版本31修订）**：按[Entity公共复核](entity-common-synchronization-review.md)，33个已排除范围不比较Host↔Client原值、不为不上线字段构造解码拒绝用例。E09／E25检查毒／火／冻结等最终flags、颜色／图标、HP／死亡、暂停及最终位置；覆盖valid参与的角色29／38替身复合绘制、exists／dead差异、碰撞调试几何和Effect172旋转／光照。E28检查本地工厂／池生命周期及完整绘制借用恢复，不能通过同步Host工厂字节修正本地owner。E34不再比较公共20个计时器和毒／火相位，版本32又移出ColorParams，继续覆盖子类及Player实际保留的展示计时。插值如实施只使用本地已应用快照历史，不放行原生Interpolate。未知字节与既有失败记录不变。

**版本29内部关系口径**：[24项逐槽复核](internal-entity-relations-review.md)中23条Host边不再比较原值，也不构造其未知引用负载作拒绝测试；E09／E25继续比较分裂／终端等独立成员最终画面。Familiar +DAC只在240布局130检查，payload补齐后覆盖空↔非空造成的绳索分段；当前不能标通过。E29检查真正保留引用和被排除槽的本地空状态、旧绑定正常撤边以及不得级联误删。Light必须覆盖Laser未优化采样点。下文“全部独立引用”均指当前实际线上引用，不能恢复旧23条玩法边。

**版本28回映射口径**：按[字段职责复核](synchronization-field-review.md)，17个已改权威范围不再要求Host↔Client原值相等；四类命中历史连元素也不比较。E09／E25仍覆盖分裂、掉落／伤害、弹幕变更后的最终成员与画面，并确认Client未重演玩法。Familiar的FireCooldown／State／Coins等实际展示依赖继续覆盖，variant80最终充能表现尚待接线，不能记为已通过。未闭合候选仍按现有线上归属比较，不提前裁剪。

第4步原未决前四项的补充沿用既有条目：版本28起Knife K4六个f32只归Host权威，E09／E25比较分裂后的最终成员、位置／姿态，不再比较K4原值；`+0x518`是碰撞方向角，Client不得为验证其效果执行原生碰撞。E13／E17只对实际线上字段拒绝非法浮点，不再要求拒绝不上线的K4负载。E27保留Knife末四字节、Slot末四字节、Projectile末四字节和Effect`+0x41C/+0x44C/+0x4BC`的未知区观察，并复核六段空白的填充／保留字节假设；Effect传播／生成用途已静态核定，验收确认Client未重演这些Host玩法。Slot进度指针、Projectile回调指针不得按状态写入或执行；Effect不得在Client重演Spawn／传播。这里只安排第15步覆盖，未提前运行。

第 4 步 Tear／Pickup／Laser 的覆盖并入上述既有条目，不另开验收编号：E09／E25 比较三份 Tear 辅助 Sprite、两份 Pickup Sprite、全部当前线上独立引用、循环切换后的最终形态／Sprite、Laser原始有符号采样模式、真实展示标量和optimized／unoptimized两组最终点列；版本35起不比较Host未来循环列表；E13／E17 覆盖 count 越界、非有限几何、未知引用和分配失败；E29 覆盖关系替换、撤边及级联 Remove 不得误删独立存活对象；E34仅覆盖此组三类中仍上线的Pickup DropDelay条件计时，Pickup Timeout／Wait自版本35、Laser Timeout／RotationDelay自版本37起不比较；其他真实线上未知时钟仍须报Gap；版本34撤销Tear两个计时的锚点比较；E27 包括这三类的全部未知区。容器内容必须完整提交，不能只比较 count 或跳过空集合清除。

Slot／Projectile／Effect 同样归入既有条目：E09／E25覆盖Slot 1—18变体、奖励Sprite最终外观／动画／图层及真实播放条件，以及Projectile的Height／公共DepthOffset单份映射与最终轨迹／Sprite；版本36起不比较ProjectileFlags等八项Host输入；Slot八项Host参数自版本35起不比较，其原生16位等宽度事实仍保留；E32 覆盖 Effect 六臂、三种具体 owner 的存在→缺失→再创建和同目标版本跨块变化；E13／E17 拒绝非法 present、缺失值非零、尾投影缺少存在性、未知动态类型、非有限轨迹与超预算。E29 覆盖旧 owner 正常退休、失败候选不得部分替换；E34 比较 Effect Timeout 的负值、停走、跳变和到期帧重算，E27 包括三个根对象和外部 payload 的全部未知区。基础臂未核 VarData 不得按零或不透明内存伪装成完整状态。

- **人数顺序**：先两人（P2）跑全套，再四人（P4）扩展，与原方案一致。
- **启动前沟通**：每次实机启动前，告知用户需要配合的操作和何时停止手动操作；需要手动配合时，先确认用户已准备好。
- **连续执行**：遇到失败先修复，再在本步内续测必要的条目，不因失败默认停下。
- **材料保留**：临时材料用完就清理；只在中途停下、需要续测时保留必要材料。
- **不放宽**：不放宽指标，不扩大池或预算上限来让测试通过。

## 改动范围

- E2E 工具：对照、探测、统计、故障注入。
- E2E 方案文档：在本目录下另起 `15-e2e-plan.md`，保存修订后的完整条目与指标。
- 进度记录：在本目录下另起 `15-e2e-progress.md`。

## 失败方式

- **对照工具接入同步路径**：Lua 对照被正式同步路径调用，或者影响主线程的计时结果。
- **探测成本未扣除**：未知区探测的诊断复制计入正式性能指标，使结果失真。探测和计时要分开运行，或者分别报告。
- **对照对错帧**：Host 采集和 Lua 对照不在同一窗口、同一次采集，误判不一致。
- **只看平均值**：只报告平均 FPS 或平均耗时，掩盖 p95、p99 或最大值超标。
- **混用旧材料**：tx-04-21 的暂停材料与新格式的结果混用。
- **锚点字段对照口径错误**：Client 侧对照在展示借用之外读取 FrameCount、`Room:GetFrameCount()` 或 Timeout，用的是 Client 自己的帧计数，误判不一致（第 13 步 13.6）。
- **空帧无法出现**：D0 几乎每帧变化，正常运行中很少出现空帧；不专门构造就宣称空帧路径已验证。

## 完成内容

- **全部通过**：E01—E35 的两人全套和四人扩展全部通过，每项都有首因、修复和续验的记录。
- **字节归属**：全部布局的未知区都有结论；“完整”只对未知字节为 0，或未知区已确认不变的类型宣称。
- **回映射**：覆盖报告中，每个 `ReplicatedState` 成员都已比较，或已说明为什么不比较。
- **性能**：总计划第 1 节的性能指标全部达标。

## 未决问题

- E2E 对照与正式采集在同一窗口内的计时隔离方式。
- `out/tx-04-21-p2-20261106-01` 恢复材料的处理（第 0 步，用户决定）。

**版本37 Laser口径**：按[Laser／Homing采样、运动与计时复核](laser-homing-synchronization-review.md)，E09／E25覆盖追踪、曲线、环形、旋转、收缩、父体跟随和接触派生造成的最终Angle／Length／点列／Sprite／独立成员；兼顾sample_mode_raw=-1／0／1、完整效果位、公开端点、MaxDistance／GridHit及+4F0展示分支，Room Light实际使用unoptimized。E13／E17拒绝真实上线点数越界、非有限几何／标量、来源变化、未知引用、分配失败及预算溢出；不构造撤销输入的非法负载。E29覆盖两组不同点数、空↔非空／增长／缩短、同目标版本完整安装、失败不部分替换、正常退休及本地indices／Bezier空owner；E28确认Client不做采样／Update／Interpolate／接触模拟，draw借用精确恢复。E34撤销两个Laser锚点／原值比较，其他实际展示时钟保持；E27保留421B未知。预留L2／L4／L7／L8不作为完整构造必收负载。本轮只修订验收安排，未执行。

**版本38 Effect验收口径**：按[Effect各臂与payload复核](effect-synchronization-review.md)E09／E25／E32比较最终画面、Scale在variant121的Light、172旋转、160连接几何、196跟随显示、136持物及落地后的真实成员、三种payload全部实际状态；不比较六项Host参数、TearFlags／136描述、MrMe四项Host私有状态、Ghost派生Offset／Scale或独立i32 count原值。Ghost覆盖0—4前缀、三投影count一致、同资源不同实例、缩短／归零／再增长与present切换；E13／E17拒绝源i32负数／>4收窄、线上计数不一致、错RTTI／present／资源／非有限值及预算溢出，不构造已撤销字段负载。E29核正常清理四槽ANM2、独立Trail节点及共享owner和失败不部分替换；E28核draw临时派生精确恢复，Client无Trail追加／衰减、loot选择、传播／Spawn、举起／投掷／落地或Update／Interpolate。E34仍比较真实展示Timeout的负值、停走、跳变与锚点重算；E27保留根对象和各ANM2的UNKNOWN，不把Ghost未发送槽中的未知消除。MrMe16B与Trail0x40已有静态正面证据，原未决事实见历史报告，不再把旧producer缺口当现行同步Gap。这里只安排第15步，未提前执行。

**版本39装备／Plus验收口径**：按[完整复核](player-equipment-plus-synchronization-review.md)E09／E25覆盖四主动槽／空槽的Charge／Battery／Sub／Partial／VarData及特殊HUD，15种Weapon及10／11同vtable、两个真实进度条、Whip主实体＋Familiar D74／D76展示切换、Berserk704环、Null109／112／trinket69效果及disabled且列表非空的查询差异。E29覆盖五槽类型替换／清空、Whip主引用正常撤边、效果列表清空／重建及窄Free，不级联删除独立成员；E13／E17拒绝真正在网的坏资源／引用／count／非有限值／nullable与预算溢出。E28检查无Fire／Update／HUD Update、效果到期／增减、重算／回调玩法补值、重抽／重采样／初始化音效，以及进度条／HUD临时姿态精确恢复。Plus覆盖未设→false／有效零→true／非零→再未设，Scene实际水面／反射须消费；其他已排除项不恢复。E34核最终效果cooldown与世界展示帧，不为已撤销冷却新增锚点；E27保留Weapon20B、TemporaryEffect4B、外层2B及各Plus未知。只比较保留字段与最终画面，不比较八个撤销范围；这些测试仅在第15步执行。
**版本40 Effect后续五项验收口径**：见[证据与落地](effect-next-five-evidence.md)。E09／E25／E32覆盖11布局、113正确落141、六个传播variant与Timer／生成／计数的最终展示，客户端不读取或重演Host请求／函数回调；原有三payload与未核variant的UNKNOWN保持。E32覆盖Trail present缺失、存在且0／1／126／127节点、增长越过原生128裁剪点后仅发布127，以及缩短／再创建；不为测试触及生产数据而新增持久副作用。E13／E17拒绝线上count=128／65535、长度不符、旧版本／错臂、非法浮点／资源／实际预算；原生损坏／中途采集的128或更大数量应报Gap，不能截断通过。E34分别覆盖负值、零、线性递减、101局部＋公共双减、141刷新、暂停及跳帧／回绕，比较同已应用world帧精确还原；仅线性区间断言锚点不重发，不把静止位置当计时条件。E28／E29核未知或错variant非空holder拒绝、前后变化报缺口、正常owner清理、无Spawn／传播／回调／Trail维护，失败不部分替换；E27继续保留167后16B及各臂剩余未知。本批没有运行上述测试，第15步仍为唯一统一整体验收。


其中Type964是NPC Render的静态数值分支；实机场景必须先有真实NPC RTTI及合法producer，不能把同数值的NativeDummy强套NPC布局。未取得来源时如实记未覆盖，不造V1或假对象满足用例。

**版本43 NPC现行合同**：受控UI覆盖F1C=-1→0及其他合法来源值→2、C/S本机/远端参与者相同展示语义、来源变化/窗口失效/读取失败报Gap、线上1/其他非法值拒绝、Client设备变化不改变已应用表现。用同一次整体验收确认reader不读Manager/NetplayManager、不调用IsIdxLocalPlayer或slot5；Client两个受控UI整段和Spawner远端淡出整段均不借原版Online筛选。保留403独立HP路径及所有原有可见/资源条件；Delirium资源变化不强制换代、OverlayEffect Scale恢复、五标量/完整点列/引用/UNKNOWN范围保持。未提前执行。见[来源闭合](npc-control-ui-evidence.md)。
