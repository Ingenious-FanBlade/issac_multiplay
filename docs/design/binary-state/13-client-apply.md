# 13：Client 原生应用

**2026-11-12 显示委托修订**：

- 13.4：漂移校正的前提改为新的 Client 边界。
- 13.5：补入 Client 自协调绘制的退役项。
- 13.6：由帧号方案B 取代展示时钟的借用。
- 13.8（新增）：显示边界、帧号写入、注入与事件播放。
- 见[修改方案](client-display-delegation.md)。

本次只修订文档，未改代码，未运行测试。

**2026-11-12 HUD代码迁移，版本70**：已新增纯值HUD派生、普通／特殊图标准备、图像中心、材料／法术绘制描述、六项基础value格式、多人本地排版和原子候选owner。真实资源后端、原生writer／绘制及整批安装尚未接通。 具体接口、事实及剩余接线见[实施说明](hud-local-implementation.md)。第15步唯一统一验收，未运行测试。

**2026-11-12 HUD本地化应用交接，仅文档**：[现行合同](hud-local-presentation.md)要求以纯显示adapter生成低血周期闪烁、P8护盾资格、普通主动／饰品图标及图像中心、P3材料／法术队列、Drop渐变、基础属性文字和多人排版。复用同目标状态／资源／显示时钟，Drop按实际消费驱动，Stat基础格式只用Host最终value；不调用完整HUD Update、EvaluateItems、概率／合成重算、设备／Online路径。特殊结果／触发相位／有序列表／概率继续安装，本地资源准备及原子提交成功后才ACK，退休等待实际借用退出。下方旧重复缓存安装要求按新用途迁移；当前schema／代码未改，本次无生成、构建或测试。

**2026-11-11 第12步第3项交接**：[影子／候选事务](12-client-shadow.md)已有准确安装base、不可变数字视图／结构意图、Begin／Complete／CompleteDrain及原始joint凭据。主线程实际整批完成后才由后台Complete提升根，预留结果／事件缓存提升也必须成功才返回ACK；取消后Success及部分写入保留实际busy到真实清理排空。第4／5项完整validator与缓存owner尚未实施，不能用默认成功代替；本步实际写入／资源物化／退休仍待接线。第12步本项产品纯编译／链接完成，无测试。

**2026-11-11 第12步第1项接口交接**：[接口与寿命合同](12-client-contract.md)确定ClientStateAcceptance由后台owner串行驱动，主线程消费不可变准备／应用批次并返回原票据的准备／完成／排空事实，不跨线程修改接受状态机。Begin后执行前仍核取消与本地代次；RequireRecovery／Retire清逻辑inflight不代表实际writer／draw借用已排空，实际busy及存储保持到完成／排空，不能启动重叠原生批次。整批真实成功后后台Finish并ACK；失效期间实际写入仍须正常清理。上述仅接口合同，二进制批次及本步原生接线仍未实现，无构建或测试。

**2026-11-11 第9步收尾交接，版本65**：[现行预算／期限](9-budget-capacity.md)覆盖原7.5Mbps与待核预算：模组9Mbps；8秒传输／10秒整目标、2秒真实无进展、同episode最多两次全量尝试。后台超时只撤成功资格，不能强杀writer或假定回滚；部分写入后的清理／重建及ACK规则保持13.3，真正原生接线仍待本步。

**2026-11-11 第9步第五项同步修订，版本64**：[版本、接受与恢复](9-acceptance-recovery.md)已落实纯值Begin／Finish凭据。原生整批成功后才推进安装版本、事件水位并ACK；可能部分写入的失败将受影响事务标为不可用，停止增量、旧ACK补发及污染对象的展示消费，发送reason5并完整重建。历史成功版本只作诊断，不能继续作本地差分起点；本步实际writer／清理／重建接线尚未实施。

**2026-11-11 第9步第二项迁移，版本61**：[正文合同与新布局](9-domain-body-codec.md)已登记422文字／423有序行／424本地历史颜色描述，替换文字图引用、移除Host反馈与切房surface，374收窄为独立Boss versus；305／306正文禁用、304拒绝kind5。字体／特殊首码映射与真实封存、本地缓存、完整资源接受及原生显示仍由10—13步落实；取消的Host图像不形成Wait，真正必要依赖不早ACK。公共头及带宽预算保持。
**2026-11-11 当前显示应用调整**：[本地生成合同](local-media-presentation.md)：Client按文字／必要样式生成自有缓存，完整目标就绪后直接切房，以本端历史画面合成hallucination，使用本地文件播放Host选定音频。不要求Host像素／历史采样相同，不等待取消的surface依赖；纯显示缓存leaf不等于放行玩法HUD Update／ShowHallucination／ChangeRoom。下文旧像素上传／精确反馈要求被替代，代码／schema尚未迁移。

**2026-11-11 第9步各域正文实现，版本60**：[正文实现与边界](9-domain-body-codec.md)、[同源数字计划](layouts/09-wire-format/body-codec-plan.md)。BodyView提供数字索引的根字段／块纯值视图，失效分支present=false，编码规范空主动槽和Null costume直接为零；decoder不执行原生写入。消费前仍须完成成员／资源／拓扑及投影闭包验证，并持有不可变缓冲；UNKNOWN／来源Gap及原生adapter尚未完成，不能按协议偏移写游戏地址。

**2026-11-11 第9步公共合同，版本59**：[正式结构](9-public-wire-contract.md)。消费版本59明确分段／组／贡献及voice身份，双分段共同提交并一次确认两序列；body依赖相等P6拓扑，GameFrame仍u32。缓存引用必须来源有效，Scope退休不能借旧组；Clear仅作用明确C/S world voice范围。只有原生完整提交成功才AppliedAck，writer／adapter运行接线仍未实施。

**第8步版本58交接**：[地图、各端HUD及具名结果](implementation-8-presentation.md)已登记。Minimap325复用完整L0—L3与300—303；各端依ControlGroupHudSelection364选择本组详细HUD，其他组公共HUD保留。357／358只借用已确认alpha，屏幕／转场消费完整有序贡献，音频消费Options前逻辑参数；所有缺结果规则和既有UNKNOWN保持，具体writer／资源物化仍须本步实现。

**2026-11-10 各端HUD与显示批次定稿交接**：[用户确定的方案B](presentation-result-batches.md)要求各端History／Stat等详细HUD默认跟随本端控制组，双角色按实际组内身体显示，其他组保留公共多人HUD。原生两槽不是全部Peer容量；不得继续以roster第0身体充当本端主体，也不修改PlayerIndex／设备／Online状态获取本地性。按明确owner、贡献身份及依赖共同提交，缺结果不安装／不ACK；CPU消费后恢复临时借用。现有逐身体动态HUD仅为窄实现，完整选择／布局／adapter仍待实施，版本57不变，无构建或测试。

**2026-11-10来源同步职责复核交接**：[当前结果路线](presentation-source-synchronization-review.md)以屏幕／转场最终贡献及每voice最终参数驱动专用adapter。Client不再消费Nightmare路线／Scene帧／Dogma生产体或音乐mask／ducking／混音树重复计算；保留实际ANM2插值和已核局部坐标／视口语义。设备失效、个人Mute、后端抢占和普通one-shot自然播完由本地owner处理，不改world意图；明确world Stop仍执行。必要逻辑段落／起点及可靠恢复未核，精确硬件游标不作同步条件；版本57，未构建或测试。

**2026-11-10 第8步音频来源交接**：[具名voice与非随机消费边界](presentation-audio-sources.md)提供已选文件／实际Start、参数更新与有范围停止、两音乐组／声部及jingle合同。Client使用专用音频adapter和自有本地source强引用，按本地music／SFX音量类别及对应混音规则执行；不调用随机SFX Play、环境音改号helper或整段Music Update。baseline不重播历史one-shot，重复状态不重启，旧voice控制不能命中新代次；未在本端开始的历史one-shot不因参数更新补播。具体后端构造／自然结束／seek、intro-main段相位及重连续接仍未闭合，不能将Update计数当游标或从头Play当原相位恢复；正式adapter未实施，版本57不变，无构建或测试。

**2026-11-10 第8步资源／屏幕来源交接**：[具体结果和安全消费边界](presentation-resource-screen-sources.md)提供内置shader与实际图像、各屏幕pass、切房surface／versus／Nightmare条件贡献及BossOverlay。应用使用有界固定显示adapter和已就绪内容；不运行完整Game／Nightmare Render、BossOverlay Update、转场／Fadeout命令或旧Lua shader tracker来补源。保留本地视口／图形选项及提交后恢复，普通Image的本地自动shader模式不等于ShaderType；原版Deathmatch与Online密码显示排除。正式布局／typed解码／图形物化／writer仍未实施，版本57不变，无构建或测试。

**2026-11-10 第7步正式描述已实现**：[Room／Level描述接口](room-level-description-implementation.md)提供固定值、独立双配置、L2、L3及完整显示投影。现有原生Prepare／Refresh显式使用LegacyClientNativeSceneDescription等旧输入，本步再实现新writer／owner／绘制消费；不提供从新描述补造seed、全配置、保存vector或背景RNG的兼容转换。

状态：设计稿，未实施。上级：[总计划](README.md)。

**2026-11-10 各域交接统一整理**：现行入口为13.7“各域原生应用交接清单”，按来源、特殊约束、版本依赖与待落实项登记全部相关域。本次静态审阅的布局源为57；布局入源、运行接线和第15步验收分别说明。本次只改本文档，不修改实现／布局源／生成表，不构建、不运行测试。

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
| `derive` | 由已核的具体纯显示公式／pass派生，需要第3—8步证据；不能因此放行混合Update、随机或玩法生产路径 |

- **`direct` 需要证据**：一个字段只有在证实“直接写内存不会破坏不变量”之后，才能用 `direct`。证据可以是 J273 中对该字段的写入点只有普通赋值，或者现有原生写入正是直接写入。没有证据的字段一律用 `writer`。
- **引用和资源**：引用字段按成员序号查 Client 本地实例表，得到原生地址后写入。目标还没构造时，该块延后到目标构造之后再写，按依赖排序。资源字段按序号查typed不可变声明：已有文件由本地资源owner加载，配置投影构造自有显示承载，文字／有限样式与合法纯值recipe交本地图像owner生成；当前不接收Host原始图像／音频。只等待现行合同实际必需的本地资源，不等待已取消的切房surface或Host历史图；成功准备并提交后再ACK，不能统一解释成路径或原生地址。
- **子类 EntityPtr 按展示职责保留**：版本29将[24项内部关系](internal-entity-relations-review.md)中的23项归Host权威，不生成身份解析或安装；Knife K3保留为空块。Familiar `+DAC` 仅variant240使用布局130恢复真实引用，其他变体保持本地空关系；当前最低展示需求是存在性，不得写伪非空地址。公共关系、Tear.StickTarget、Pickup公开三关系和Laser.BounceLaser继续经SetReference维护；版本39起KnifePlus.hitboxSource留Host，旧本地边仍须正常撤销。目标先构造，退休先撤边，再退独立实例；23槽如因旧实例复用已有本地绑定，也须正常撤边，不直接memcpy零或重演级联Remove。
- **Bomb 懒重建请求**：`+463` 虽归 Host 权威、不采集，Client 仍必须在最终主 Sprite／五份 costume 安装完毕后清为 false，并保持旧执行器的读回检查（`client_entity_adapters_native.cpp` 的 Bomb 分支）。不能因“权威不采集”而省略本地清除，或用 Flags setter 再次置 pending；第 14 步退役旧路径时保留这个行为。
- **版本 28 权威排除**：Knife K4／命中父链选择、四类命中历史、Projectile Damage／Change 三参数、Laser 掉落概率／伤害倍率共 17 个范围不生成安装字段，不再要求本地原值与 Host 相等。旧执行器及检查在第 13／14 步按新职责退役；保留空块号不意味着必须发送或写回空负载。Client 自有容器的正常构造、析构和必要关系撤边仍由本地 owner 管理，见[职责复核](synchronization-field-review.md)。
- **Familiar 间接展示（版本33）**：`61E360→62C0C0/647310` 的FireCooldown／State／Coins继续保留。[Familiar参数／充能／绳索复核](familiar-synchronization-review.md)排除11个轨道／计数／方向范围；80布局131用`incubus_charge_bar_amount`传最终i32百分比，具名writer仅安装本地表现缓存，不写DB0、不创建Weapon。专用展示adapter保留原充能helper的动画／可见性尾部及DB8借用恢复；该adapter仍未接线，不能把旧Player Weapon安装器或置空视为等价。
- **Familiar240 绳索（版本33）**：布局130保留真实DAC引用，新增F7—F10四投影：133号payload的Sprite外观／动画／图层及132号XYZ节点几何。节点为原生0x78、按deque逻辑顺序输出12B，未知模拟字节不上线；所有投影与资源同目标版本齐备后，通过游戏兼容构造及`430E40`共享owner一次安装，不写ECC／ED0裸指针，不调用AI`42D6F0`或Effect的`606310`。非空payload节点数0会导致原生下溢，必须拒绝；DAC先安装目标引用、退休先撤边，再正常退休payload，不重演级联Remove。候选构造／失败清理及实际接线仍待本步。Room Light使用Laser未优化采样点的合同不变。

### 13.2 结构路径

- **加入**：用类型化视图填写描述结构，取代 JSON 字段，交给现有的场景结构事务构造。
- **形态变化**：退休旧实例，按新形态构造新实例。
- **离开**：按现有的退休流程处理。
- **事务边界**：沿用 `ClientScene` 的结构事务和提交边界，不在事务中途写热块。

- **当前切房显示**：新scope的完整合法目标与必要本地资源就绪后直接切换，不运行原版ChangeRoom或等待前后surface动画；旧目标按其真实生命周期退休，不把取消的平滑显示当世界缺口。文字纯显示缓存与本端hallucination历史按[本地生成合同](local-media-presentation.md)管理，仍须保护图形状态、CPU队列及资源引用。

### 13.3 热路径

- **写入顺序**：按（成员, 块）调用分派表；每个块内按字段顺序执行。
- **出错处理**：writer失败时标记受影响成员／事务需要恢复，不把已经部分改变的原生状态继续当作历史A差分起点。依赖同一结构／显示事务的成员不能继续组成混合目标；独立工作只按既有事务合同处理。具体重建／候选清理归本步接线，不声称任意原生写入都可回滚。
- **整批成功才确认**：只有整个原子批次成功，才推进成功安装版本及world连续事件水位，并回复原始 `AppliedAck`；joint一次确认两项。排队、解码、准备成功均不提前确认。
- **提交前失败**：能证明尚未产生任何原生写入／结构副作用时，只淘汰候选或等待依赖，保留仍合法的旧目标；重新准备后仍须复核完整候选与生命周期。
- **部分应用失败**：一旦可能已经写入，或无法证明没有副作用，按失败批次及依赖闭包标记受影响事务不可用；停止其增量、旧ACK补发及污染对象的展示／音频消费。历史成功版本保留作诊断，不再作为本地差分源。发送 `Resync(reason=5)`，其installed字段仅表示历史成功版本；Host为所需序列生成target继续前进的base=0完整目标，涉及结构或双分段时共同恢复。
- **重建次序**：先停止受影响publication／consumer并排空在途callback／draw借用，再按正常owner撤引用、清理未发布候选及退休污染承载，准备新对象／资源／全部必需结果并完整提交。资源已准备不代表原生目标可复用；不得用Save／Restore／Update／Lua／随机生产器补世界，也不假定任意写入可回滚。只有完整重建成功才恢复消费及ACK。
- **完成凭据**：Begin必须紧邻真正写入前重查候选／生命周期，Finish只接受当前route及唯一serial；旧route回调不发布ACK，也不解除当前恢复。ACK丢失仅重发原已成功提交凭据，不能把部分写入失败当作ACK丢失。
- **按 F 换算值的重写开销（2026-11-12，RE-3 审阅后补充）**：帧头每前进一次，Effect Timeout、TimeCounter 等按 F 换算的值都要重写，实体块没变也一样（13.8 安装点第 2 项）。这部分开销随实体数线性增长，计入 Client 安装预算（p95 ≤2 ms）。实现时只遍历保留了换算字段的对象，不扫全部实体。

历史说明：此前“部分失败回复上一个完整版本，让Host继续携带失败块”的表述已由本次规则替代；保留历史成功事实，不再执行该恢复方式。

### 13.4 漂移校正

Client 的原生对象可能偏离影子状态。Client 不运行原生 Update，但 Render 和维护 helper 仍会写部分字段，例如维护姿态时推进动画，或者 base draw 写屏幕坐标缓存。Host 只在块变化时发送，如果块没有变化，Client 的偏离就不会被纠正。

- **检测方式**：按同一状态布局建立Client合法读回／规范投影，再与对应影子目标比较；直接字段、具名owner值、Synthetic缓存、条件派生和展示借用分别取有证据的读回来源，不能把Host采集计划机械地用作Client根偏移读取，或为读回执行Getter／生产器。不一致的实际应用块进入恢复／重写；派生／本地scratch及已裁字段不要求原值相等。
- **执行频率**：轮转执行，每帧只检查一部分成员，总耗时计入安装预算。
- **前提（已定）**：Client 不运行原生 Update。这是 tx-04-17 的隔离设计：现有 Client 拦截 Game、Room、EntityList、Player、PostUpdate 等入口（`client.cpp:28,142-146`），本地不会按玩法推进任何字段。
  - **2026-11-12 修订**：Client 仍不运行玩法 Update。但它会运行“显示推进”根中已核实的显示子更新，以及包装放行的原生 `Game::Render`，见 13.8 和[修改方案](client-display-delegation.md)第 4 节。
- **已知写集与其余差异**：Client 仍在运行的显示推进、Render 和维护 helper 可能造成偏离。2026-11-12 [RE-5／RE-6 收窄静态成果](client-display-delegation/RE-5/README.md)已完成：Entity `+0x1A4/+0x1A8`、LightGradient scale `Room+0x6EDC/+0x6EE0`、坑纹理 dirty `+0x11F6`、本机 resize 与 Render 反馈均归本地，不作影子相等／漂移校正。
  - Projectile `+0x350` 继续是同步 PositionOffset，浮点减加不等于精确恢复；恢复作用域为保留的 `client_entity_draw_projectile`：进入时保存 4 字节，正常返回且对象身份未变时写回（2026-11-12 用户决定，修改方案 9.1、Q10）；Tear 暂存的是主 Sprite 44B ColorMod `Entity+0xF0`，正常路径已精确恢复。旧 Tear `+420/+424` 实为十进制，即本地屏幕缓存 `+0x1A4/+0x1A8`，不是子类 Scale。按[写集与恢复合同](client-display-delegation/RE-5/write-set.md)区分作用域恢复与轮转校正。
  - **取代同日较早的全写集／随机静态闭合要求**：RE-6 已并入 RE-5，只将已知项定稿，其余由第 15 步 E28 实测差异后补查。只改本地显示缓存允许发生；改到仍同步的字段才进入校正或 Render 后置恢复。纯外观随机按 Q4，回调按 Q6，不逐个闭合 Render helper。
  - 恢复前复核对象／scope／generation 与成功目标身份；身份变化、异常或可能部分写入时按 13.3 恢复，不向退休对象强写旧值。Render 后置不推进版本或 ACK。布局源本次仅补证据／说明，版本 71 的旧结果布局和相机偏移仍待统一收缩，不能宣称退役已接线。

### 13.5 退役的部分

- `client_entity_fields.inc` 中按字符串匹配的分派，改为生成的索引分派表。
- 描述结构中的 `std::vector<ClientGridField> fields` 等字符串字段。
- `GetWaterClipFlags` 改由Entity／Grid各自已核owner消费typed结果；缺writer／adapter是待实施项，不能回退Lua或仅因尚未接线改称派生。
- Player 记录上的 `application_draw` 及其 JSON 校验（`client_entity_class_model.cpp:118-124`），改读影子状态中的绘制输入块 D0（13.6）。
- draw 期间逐实体重新锚定 `+0x328`（`client_entity_draw_native.cpp:128,185,195`），以及每次展示前重新锚定 `Room+0x11F0`（`client_room_native.cpp:132-135`）。它们由 13.6 的展示时钟取代。
- **2026-11-12 补入：Client 自协调绘制**。退役以下内容：
  - `ClientMenuRenderBoundary` 的场景绘制；
  - `RenderPublished`；
  - `DrawImpl` 及其 `FrameBorrow`、`FXDrawContext`、手工阴影／光照调用、FX `0x56db20` 和 `ClientRoomWaterPreDraw`；
  - `client_resource_shaders_native.cpp`。

  D0 取消后，上文“改读影子状态中的绘制输入块 D0”作废；Player 绘制直接读已安装的状态。

### 13.6 展示时钟与帧锚点

> **2026-11-12 修订（帧号方案B）**：本节的“展示时钟”现在就是 Client 的 `Game+0x25A70`。
>
> - 每装好一个 world 帧，就在同一安装点把它写成帧头的 game_frame，并保持到下一个 world 帧。
> - 下文“展示期间”的借用与恢复、`Manager._framecount` 借用和 D0 全部取消。
> - 锚点、Effect Timeout 和第 4 步子类计数的规则继续有效，其中“展示时钟”一律读作 `Game+0x25A70`。还原在安装点完成，不在 Render 中进行。
> - 详见 13.8 和[修改方案](client-display-delegation.md)第 3 节。

**RE-3 同日核对**：[计数目录](client-display-delegation/RE-3/display-counters.md)区分原始值与锚点。下方 Familiar FireCooldown／Pickup DropDelay 的旧锚点段保留历史说明；两项已按 2026-11-11 用户决定及当前字段源改为原始 i32，不外推。Effect Timeout 与 TimeCounter 仍按各自现行换算合同在同目标安装点还原。Game／Room 内联显示标量按现行 Q8 同步当前值，不在 Client 推进。

**RE-4 同日核对**：[风险与处理](client-display-delegation/RE-4/handling.md)区分F相位、周期请求、本地M批次、按帧缓存和独立计数。周期HUD刷新检测跨30帧边界并合并请求；显示批次不以F去重或按ΔF补跑；world安装后维护平均位置缓存和本地HUD初始化。以下13.8按已核Manager边界调度，未实施／未测试。

- **定义**：展示时钟是 Client 最近已应用 world 帧头中的 game_frame（第 9 步 9.3）。
  - 它只在装好新的 world 帧后前进。Host 在静止场景中也发送空帧（第 11 步 11.3），所以时钟会照常推进。
  - Client 自己的 `Game+0x25A70` 不代表 Host 时间，不能作为换算基准。（2026-11-12 起不再适用：方案B 把它对齐为 Host 帧号。）
- **展示期间**（2026-11-12 取消，原文保留）：沿用现有借用方式（`client_presentation_native.cpp:826-829`、`client_entity_draw_native.cpp:238-241`），进入展示时换入下列 Host 值，结束后恢复 Client 原值（同文件 136-145 行）：
  - `Game._frameCount`（`+0x25A70`）← 展示时钟；
  - `Manager._framecount`（`+0x4A8B8`）与paused来自5B D0；Pickup／Familiar使用具名随机结果批次，不写两份原生全局RNG（见第8步8.2.1及room-random-handoff.md）。上下文与结果绑定同一已完成world／presentation修订。
- **帧锚点字段**：`Entity+0x328`、`Room+0x11F0` 的基准按原值写回，不再逐次重新锚定。展示期间，FrameCount 和 `Room:GetFrameCount()` 由原生 getter 用"时钟 − 基准"自然得出。
- **Effect Timeout**：每装好一个新的 world 帧，按新的时钟把全部 Effect 的 Timeout 重写为"到期帧 − 时钟"（`effect_timeout`）。
- **第 4 步子类计数**：Familiar FireCooldown的明确方向锚点先经 `ValidEntityCounter` 校验，再用同一展示时钟调用 `DecodeEntityCounter`；版本33起HeadFrameDelay留Host，版本34起Bomb ExplosionCountdown亦留Host，不再安装、换算或漂移比较。只还原已核分支的值，不运行射击／爆炸等玩法；未知活动条件由 Host 报缺口。共享 payload／Sprite vector 的分块投影计数须一致，并经 `ValidContainerCountGroups` 核对合计，蝗虫两组合计最多 16384。真实编码长度不得超过 `Field.max_size`，全部依赖齐备后交具名 owner 一次安装。
- **Tear／Pickup／Laser**：版本34起Tear WaitFrames／StickTimer留Host，版本35起Pickup Timeout／Wait亦留Host。版本37起Laser Timeout／RotationDelay亦留Host；此组三类只对Pickup DropDelay按真实条件编码随world展示时钟还原，不套用通用无夹限计数。Tear 三份、Pickup 两份辅助 ANM2 分别交资源 owner；HomingLaser只对optimized／unoptimized两组最终vector由各自owner构造候选，完整同目标版本依赖就绪后安装，独立count不要求相等，空集合清除旧点；保留本地self，indices中间缓存与Bezier保留正常构造的本地空vector及正常析构，不运行CalculateSamples。Pickup未来循环道具自版本35起不安装，本地owner维护安全空列表／count；不得重建Flip保存玩法对象。版本29只对Tear.StickTarget、Pickup公开三关系及Laser.BounceLaser解析成员身份，经SetReference安装、退休前撤边；已归Host权威的23条内部关系不安装，其本地空槽和旧绑定清理遵循13.1，防止原生Remove误删独立对象。见[本轮实现](implementation-4-tear-pickup-laser.md)。
- **Slot／Projectile／Effect**：Slot只安装保留的主／奖品Sprite等当前显示结果，不运行开奖。Projectile除Height还包括版本45的原始u8开关／冻结Vector，DepthOffset只复用E4，不运行半帧模拟。Effect三个具体附属臂先核存在性与全部投影／资源，再经606310对应owner一次替换，不逐投影更换；FX1依赖Parent。136不在Effect／Grid安装保存描述，其持物关系／BodySprite／最终主Sprite保留。当前字段／分块见13.7与[版本45补证](projectile-effect-residual-evidence.md)，旧版36仅Height或FX0/1旧大小不作为当前合同。
- **第3步保留的计时**：版本30排除Entity公共倒计时、PauseTimer和毒／火伤害相位，不生成还原调用。版本32再排除ColorParams，不生成Lifespan还原、元素重建或颜色时钟见证；淡出／覆盖直接安装Host最终Sprite ColorMod。子类／Player的真正展示计时继续各自合同。EntityPlus optional由原生owner赋值（`DecodeFlyingOverride`／`DecodeWaterOverride`），不memcpy。
- **对照与校正**：E2E 对照（第 15 步）和漂移校正读取这些字段时，用同一个时钟换算。

### 13.7 各域原生应用交接清单

输入为[第12步完整候选](12-client-decode.md)及[第9步9.10](9-wire-format.md)的typed布局／依赖。版本47 Player、版本57 Room／Level等已入源，实际writer／显示adapter不因此视为完成。纯协议字段／Synthetic结果进入自有owner，不能凭伪偏移写游戏对象；所有正常Host生产流程与Client本地显示维护分别管理。

| 域与来源 | 本步特殊约束 | 身份／版本依赖 | 待落实项 |
|---|---|---|---|
| 公共结构／容器／引用：[第0—2步](implementation-0-2.md)、本页13.1—13.4 | 直接写入须有不变量证据；容器／资源／EntityPtr由正常owner构造候选、整体替换和失败清理，不memcpy活STL／holder。引用目标先构造，退休先撤边、drain后释放，不级联删独立成员 | 当前key/use/shape与完整结构事务；同序列／成员／资源及专用修订匹配，原生失败后不再信任历史A | 数字分派、构造／热写／退休／读回及恢复闭环；整目标成功后才AppliedAck，不虚构通用回滚 |
| Entity公共／Sprite／Plus：[公共生命周期](entity-lifecycle-synchronization-review.md)、[动画资源](animation-resource-synchronization-review.md) | exists/dead按最终值，valid归本地／P7 owner；保留最终11分量颜色、独立selector和绘制插值，事件位只本地清理，不播放Host历史。两nullable按正确动态holder标准赋值 | 资源／动画／引用及当前use就绪；Host路径改变但旧Image仍使用时不得抢先重载 | 资源与optional owner及展示借用恢复，ColorParams／公共已裁计时不装不作漂移比较 |
| Familiar：[结果／绳索](familiar-synchronization-review.md) | 80仅表现amount缓存，充能adapter保留原动画尾部，不写DB0／创建Weapon；240四投影完整后经430E40安装绳索，非空节点不能0，不用Effect606310或裸写ECC | DAC目标先绑定；XYZ、Sprite、主体／资源同目标，蝗虫计数组完整 | 具体typed候选／失败清理及显示adapter；节点未知不写，FireCooldown只按实际时钟还原 |
| Knife／Bomb／Tear：[三类职责](knife-bomb-tear-synchronization-review.md) | 安装Knife拖影／飞行、Bomb五costume并清pending且读回、Tear三Sprite／效果；不以Flags/Scale setter、LoadCostumes、ResetSpriteScale、Update／Interpolate补图 | 主／辅助Sprite、真实关系、公共最终姿态及资源同目标 | 实际writer／pass恢复与正常旧边清理，已裁攻击／伤害／计时不反向恢复 |
| Pickup／Slot：[职责复核](pickup-slot-synchronization-review.md) | 保留Price/DropDelay、价格／替代底座／奖品Sprite及真实引用；本地未来循环安全空状态由owner维护。Slot动画／playing不是State；不自动定价、TryFlip、Morph、捐款或开奖 | 同world计时、资源、三公开引用与随机结果用途匹配 | 具体计时／资源／临时Sprite恢复，不调用SetPrizeCollectible或Flip保存体补状态 |
| Laser／Homing：[最终几何](laser-homing-synchronization-review.md) | 两组vector各有兼容owner，空集合清旧点，允许不同count；self本地，中间indices／Bezier保持安全空容器，旧分配正常释放，不共用最终点列owner | 两组几何／真实标量、Bounce、资源及world同目标完整提交 | 几何替换、正常退休与Light消费unoptimized；不得采样、Update、碰撞或ClearLaserSamples破坏保留结果 |
| Projectile：[版本45](projectile-effect-residual-evidence.md) | 装Height、原始u8开关和独立冻结Vector，不从Position推锚点；draw临时改+350须精确保存／恢复，不能仅靠浮点加回 | 当前use／freeze flags、完整公共姿态／资源及world展示时钟 | 新PR2 writer／draw scope与读回，430／444／468及callback／运动生产器不装不执行 |
| NPC／Delirium：[0/2合同](npc-control-ui-evidence.md)、[Delirium](delirium-residual-evidence.md) | 受控UI使用C/S 0/2旁表并绕过整段NetplayManager筛选及Spawner远端淡出；保留403独立HP路径。Overlay Scale由主null-frame派生，双Sprite／三deque经owner；Boss选择不强制重建 | 稳定公共shape、真实EntityRef／null资源；companion为独立正常成员，不按BE8再创建／Remove | 三处具名UI隔离及完整复合绘制／候选恢复；不写F1C或全局Hook原版查询，Delirium未知不写 |
| Text／Dummy：[专用来源](text-dummy-evidence.md)、[文本实现](implementation-4-text-dummy.md) | Text安装RGBA／有效原始字节／特型counter，不执行Update推进duration；Dummy不造专有串，真实字体／池／Plus与正常析构保留 | 当前writer/use/revision、合法独立Render观测scope及同显示相位；历史屏幕cache非无条件派生 | 有界原生串writer、实际显示scope和Dummy历史cache缺口；不造假NPC或混拼独立观测 |
| Player P0—P2／快源：[正式Player](implementation-5-player.md)、[前三块](5-player-p0-p2.md) | 按实际P0／主Sprite三投影和19—24应用，不丢五项Render充能；CanFly／77B健康原值保留。ControllerIndex本地-1，不用Host设备键，不以UpdateHearts重建HUD缓存 | P0 topology=已安装P6，P7真实角色／held／主备与资源就绪；旧候选不按旧count安装 | typed Player描述与body-joint／窄依赖提交；不执行EvaluateItems／Shooting／HUD Update |
| Player P3—P5：[中三块](5-player-p3-p5.md)、[正式源](implementation-5-player.md) | 当前库存／槽和innate有效计数由正确官方owner承载；P4空槽展示值归零。P5按真实动态类构造，分型mask只写保留位；每槽独立本地对象，Whip先撤真实引用再窄Free | 同资源／真实类型和Player拓扑；静态Plus A8模型不授权偏移写，Host别名不等于Client重复所有权 | 合法容器不变量、动态装备替换／清理及显示公式；不执行mutator、到期更新、Fire或会级联Remove的DestroyWeapon |
| Player P6—P8／组：[后三块](5-player-p6-p8.md)、[正式源](implementation-5-player.md) | 全体Player／C/S组／资源候选准备后绑定twin/backup/held、roster／独立激活并提交P6；不拿热valid改池占用。costume最终map、704条件cooldown、残影deque与0/1当前记录list正常owner承载 | participant/group空间9/10与身体use、anchors／有序目录／资源同事务；四参与者不等于四身体 | 独占Player结构owner、跨身体借用与CPU drain后撤边／强引用；不恢复sub/replaced/703四槽、原History或完整回放队列 |
| Grid公共／Plus：[水面证据](grid-plus-water-source-evidence.md)、[描述实现](grid-description-implementation.md) | ClientDescribeGrid只接完整typed影子，不强转Legacy描述；water完整u32存per-instance。官方旁表不写，collision本地中性，六wrapper加Lock第七点按统一常驻清单消费结果、原链一次 | 当前实例/use、Room/index与正确资源／pass；工厂可能清官方同index槽，不能以该表保存C/S结果 | 正式描述接执行器／门绑定、Lock覆盖、glitch纹理source-quad adapter；不能用Flip/Rotation替代或让Client地址再次随机 |
| Grid17派生：[第6步](6-grid.md)、[Pressure／Door](grid-pressure-door-evidence.md) | 五类主色借地板、Door主alpha入口／墙色与extra独立；Fire16B结果重建本地24B元素且不模拟，模板主帧逐粒子派生／精确恢复；Rock只装合法碎块分支，Statue无自身Sprite | Room颜色／几何、Door目标／HasWater／Boss Light、计时、资源与nested_layout同world | 各真实工厂／条件Sprite／粒子／图层writer、目标映射与退休撤门别名；不Init／Update／Reward补显示 |
| Room／背景／FX：[版本57](implementation-7-room-level.md)、[显示设计](room-display-resource-design.md) | source-slot映射自有描述，真实几何／八门／rails／FX按owner；water用最终32B不二次插值／乘量，背景phase独立2*GameFrame，wave本地视口投影，ray重复展示不推进phase | 几何、门、背景用途／内容、world/presentation/pass及资源同目标；旧room图像不能贴新world | Room239及各结果adapter／图形恢复，禁止Camera／Backdrop／Room混合Update、82EE90、雨／装饰重复生成或Host函数表执行 |
| Level／Descriptor／保存探索：[版本57](implementation-7-room-level.md)、[两域](room-save-exploration-synchronization-review.md) | 先活动源槽／占用／双配置，再建立0..2维度本地别名；L1/L2是自有地图事实，不写保存vector。只借用有效curse／五位显示／Boss上下文，D1配真实Image／Player颜色 | source-slot、MapFocus成员号和ListIndex不混用；-19隔离、特殊路由／Lil Portal寿命及回溯失效按真实结构 | L0—L3／D1自有承载／门Light与生命周期，禁止Restore／Clear／UpdateVisibility／随机Refresh／TLS fallback补源 |
| Run／时钟：[Run职责](run-synchronization-review.md)、13.6 | TimeCounter按同world F减基准还原i32；Entity／Room帧基准原值，Effect每新world还原Timeout。current颜色、两黑暗、64B分数及窄投影只给显示owner，种子／flags借用后恢复 | GameFrame、D0与对应显示批次，不以Client墙钟或本地tick推进；当前Game与冻结结算副本独立 | 新handler／实际计时与分数／资格显示，跳过Render内Calculate，不写进度或全树／模式对象 |
| HUD／地图／UI：[Presentation](presentation-synchronization-review.md)、[地图设计](room-display-resource-design.md) | 实际内嵌／动态身体HUD、心格／Stat三值／History顺序／消息图像；自有地图合成L0—L3／两配置焦点／徽标／问号资源。展开／视口／音量本地，world显示限制独立 | 身体归组、Player／Run、地图／world／资源／显示修订匹配；每项徽标顺序绘制，非Sprite最后帧 | HUD／地图／屏幕／Prompt／过渡／死亡具名adapter及CPU队列／图形状态恢复；不运行混合Update／Refresh、提交／退出／健康或解锁生产器 |
| D0／随机结果：[逐用途合同](room-random-handoff.md) | 只借Manager相位／paused，不写两全局RNG；Pickup用实际alpha，Familiar本地中性ColorMod对称push/pop，CPU消费完再恢复，不依赖本地遍历分配样本 | subject/use/shape、world/presentation、资源及pass／递归／调用身份；额外本地可见分支无源时报Gap | 正式结果缓存与具名分支替代／完整恢复；缺结果不装零、不额外Render／抽取或回退seed重放 |
| 配置／图像／shader／音频：[资源设计](room-display-resource-design.md)、[同步职责复核](presentation-source-synchronization-review.md) | 15B配置仅显示承载，图像采用完整recipe或精确内容，自有owner物化并保护图形状态；声音按已选sample／所有必要并存逻辑voice的最终参数执行，不随机Play或重复消费混音树；重复state不播历史one-shot | 内容／依赖ready、资源epoch与目标批次一致；按资源音量类别应用本地Options及对应clamp，实例／world事件／用途匹配；个人设备失败不改world意图，普通自然完成本地收尾 | GPU／字体／shader及非随机音频adapter、world用途／退休隔离和必要逻辑段／续接能力仍待落实；资源表／描述生成不等于应用成功，Host绝对屏幕坐标不直接跨端复用 |

所有受控显示修改在正确主线程／图形／音频owner边界执行；临时字节、颜色栈、FBO与CPU待消费资源须完整恢复／安全退休，不能恢复已被原生SetAnimation释放的旧数组。漂移只对实际安装投影及同版本依赖比较，本地UI／scratch和Host生产输入不回填。缺source／ABI／资源／拓扑或应用失败不确认新target，后续性能与隔离统一留第15步。

### 13.8 显示边界、帧号写入、注入与事件播放（2026-11-12）

依据：用户 2026-11-12 的决定和[修改方案](client-display-delegation.md)。逆向任务 RE-1—RE-7 归入本步，RE-8 归入第 8 步。RE-10（2026-11-12 审阅新开，待做）归入本步，须在接线显示推进根之前完成。

**显示边界**：

- 每个 Client 入口判为放行、拦截或拆分，未分类的一律拦截，并记为 RejectedEntry。
- 继续用 `ClientEntryDecision`、`CountDispatch` 和 `kExpectedNestedMask`，新增入口都登记为具名入口。
- `Game::Update`（`0x2F6D30`）继续作为拦截守卫；Client活动时完整Manager原链也被跳过。自有“显示推进”根挂在Manager边界内、world安装之后，分本地M偶数的30Hz批次与独立相机阶段；HUD::PostUpdate内部已更新PlayerHUD，外层不重复调用。调用顺序／分支见[修改方案6.2](client-display-delegation.md#62-显示推进的白名单与调用顺序re-1)，计数来源与去重见RE-3／RE-4，不按到包次数或ΔF补跑。
- **RE-1／RE-3／RE-4 核定的阶段（2026-11-12）**：
  - Client 忙时 `ClientManagerBoundary` 整体跳过 Manager 原链，`Game::Update` 不会被调用，`client_game_update` 只作为拦截守卫。相机也在 Manager 尾部。
  - 显示推进根挂在 Manager 边界、world 安装之后。本机 `Manager+0x4A8B8` 由它推进（RE-3）。
  - **30Hz 批次**（本机 Manager 偶数帧）：`HUD::Update` → 转场活跃时运行具名转场显示拆分 → `Minimap::Update` → `HUD::PostUpdate` 后结束；普通分支为 `Minimap::Update`（必要时 `Minimap::Refresh`）→ 自有 renderEL 全排 → FXLayers `0x56BF40(false)` → Room 内 ANM2 动画 → `HUD::PostUpdate`。`PlayerHUD::Update` 嵌在 PostUpdate 内，外层不再单独调用。RE-9已核完整RoomTransition::Update含直接玩法写入，拆分接通前保持拦截。**Q11 后**转场分支不再运行“具名转场显示拆分”，改为本地显示工作：Boss 标记非零时推进 versus／肖像 ANM2，按已安装 mode2／3 提出 HUD 刷新请求（`Game+0x22E94/+0x22E9C`=2）；mode／counter／progress 随 world 安装，`RoomTransition::Update` 完整拦截。
  - **相机阶段**（每个 Manager 帧有调度机会）：按本机 CameraStyle 二选一，2 调 `UpdateDrag2`，其他调 `UpdateUltrasmooth(本机半帧奇偶)`。本机关闭插值（`Manager+0x2A1FB` 为 false）且更新前 M 为奇数时先快返，不运行相机；一般写 M+1，但 `Manager+0x4A8C0!=0 && +0x4A8C1==0` 时保持 M。主循环随后仍调用 Manager::Render，绘制体按本机 `I || (Mpost&1)` 判定，保持奇数的例外仍可绘制，见[RE-1 补遗真值表](client-display-delegation/RE-1/manager-render-cadence.md)。
  - **RE-1 补遗静态完成**：[提前返回分支表](client-display-delegation/RE-1/early-return-branches.md)已列优先级和 U／P／M 调用，不能以一个 paused 选择普通分支。fade 前段的 Pα 可与终结分支的 P 同次出现；Q8 不运行生产段，第 13 步明确其当前显示资格／合并策略。[条件需求交接](client-display-delegation/RE-1/branch-demand-handoff.md)列出需在第 7／8 步落实的窄谓词；旧 UNKNOWN owner 占位和 D0.paused 不算新方案已经接通。
  - **终结分支与暂停（Q12，2026-11-12 用户决定）**：Client 按 Host 每个 world 帧发送的终结分支枚举选 30Hz 批次序列，不在本机重算分支谓词；paused 位供相机阶段（Host 暂停时跳过，转场 mode 非零且 animation `Game+0x1B858`=19 MINECART 例外）和保留的 `client_entity_draw_paused`（向 IsPaused 的 53 个原生调用者返回该位）。本地菜单／控制台阻断与 Host 分支的合并由本步实施时确定并写入本节。
  - **Pα**：已安装 primary alpha `Game+0x25B28 > 0` 时在 Pα 位置调用 `HUD::PostUpdate`，同批可与终结分支的 P 各调一次；alpha 恰在某 Host 帧归零的一帧差异按 Q4 接受。
  - **Manager 尾部音频 tick（RE-10，待核）**：音乐逐帧 tick `0x7D7DA0`（原生 `0x92EB49`）与 `0x908340`（`0x92EB3E`，疑为 SFX tick）在显示推进根中按 Manager 尾部的位置运行，与相机同属每个 Manager 帧的尾部；其余约 50 个尾部调用按 RE-10 分类，分类前不新增放行。
  - 完整阶段表见[修改方案 6.2 节](client-display-delegation.md#62-显示推进的白名单与调用顺序re-1)的“J273 核定后的显示推进”。
- `ItemOverlay::Update` 不进白名单：它会治疗玩家、切房，并在动画标记处放音效。全屏演出按状态同步，由原生 `ItemOverlay::Render` 绘制。
- 只有某项显示工作内联在玩法函数中、无法单独调用时，才对那个函数改用拆分。`Game::Update`／`Room::Update` 的内联显示标量不拆分，按 Q8 在安装点写入 Host 当前值（见下方安装点顺序和修改方案 4.6）。

**`Game::Render` 包装**：

- `Game::Render`（`0x2F7A80`）分前置、原函数、后置三段，`Room::PreRender`／`Room::Render`随之放行；renderEL由显示阶段的自有有界全排owner维护，原生`render_sort`保持拒绝，不通过更改F奇偶补其渐进排序。
  - **RE-1 修订（2026-11-12）**：`render_sort` 不在 Render 树内，唯一原生调用者是已拦截的 EntityList Update。`entitylist_render_sort` 保持拒绝；renderEL 排序由显示推进的自有有界全排负责（现有 `FullSort`，退役 `presentation/*` 时迁出保留）。
- **前置**：确认 world 安装事务没有进行到一半，并确认帧号已对齐。另确认当前 RoomDescriptor 的 SpawnSeed（`+0x5C`）已按 Host 当前值安装且非零：原生 Render 在房间年龄 30 时读它，为 0 即 `int3`（RE-4；Q9，修改方案第 5 节）。SpawnSeed 随 world 安装，按变化更新，换房时新值在 Render 前就位；写入它不让任何读它做生成的 Client 路径获得执行资格。
- **后置**：恢复作用域，记录写集，交给 13.4 处理。
- `MenuManager::Render` 只绘制原生菜单。

**安装点顺序**：world 安装事务成功提交后，在同一主线程安装点依次执行：

1. 把 `Game+0x25A70` 写成帧头的 game_frame；
2. 按新帧号还原 Effect Timeout 等计数；
3. 写入注入值，顺序为库存与 Player → 诅咒 → 其余注入值（修改方案 6.3）。（2026-11-12 审阅后诅咒改为 raw `Level+0xC`／`Game+0x25AC8` 随 world 安装，不再是注入步骤；其余顺序不变。）

**转场安装（Q11，2026-11-12 追加）**：RoomTransition 的 mode、counter、duration、progress、direction、roomIndex、dimension、animation、Boss 标记与 world 同一事务安装。若即将安装的 world 帧当前房间身份与已安装 world 不同，先用仍完整的旧 world 捕获旧 surface（`Room::PreRender`→`Room::Render` 写 `+0x28`），再安装；提交成功后捕获新 surface（`+0x30`），并按本地相机与两房几何计算 `+0x22C/+0x230`。不调用完整 ChangeRoom。HUD 可见性 `Game+0x22E90` 作为 Q8 标量随同安装。

**内联显示标量（Q8，2026-11-12 追加）**：

- 修改方案 4.6 所列 Game／Room 显示标量随 world 安装直接写入 Host 当前值，属于上面同一个提交尾部。Client 不调用颜色趋近 `0x6EB990`、`Game::SetColorModifier`、房间颜色提交 `0x818740`，也不运行原函数的内联段。
- 安装点按同步值的变化补做原生的阈值动作。已知一项：震屏倒计时 `Game+0x25A80` 从正数变为 0 时，清零本地相机写入的 `Game+0x25A84/+0x25A88`。
- 现行 `client_presentation_native.cpp` 绘制前安装颜色、闪电、暗度当前值的代码，迁入 world 安装，不随绘制路径退役。

安装写前失败或部分写入时，帧号不前进，避免锚点与帧号不配套。上述三项是同一个不可观察的提交尾部，写 F 前先完成可失败的准备与换算，具体见 [RE-3 安装交接](client-display-delegation/RE-3/apply-contract.md)。RE-3 已核 Game 普通递增写点为 `0x6F789D`，根拦截后不执行；Room `+0x11F0` 是 Init `0x7E8CE8` 写入的基准，并非逐帧自增。若放行路径仍改写 F／基准，隔离写入或在后续显示消费者／Render 前恢复同一目标值，不能靠写回掩盖玩法副作用。

**注入（Q3）**：

- 优先用原生 setter 或 REPENTOGON API 写入 Client 原生字段，没有 API 时才直接写内存。
- 只有 Client 上没有对应存储，或者值依赖 Host 专属状态时，才挂 getter 并返回已同步的值。
- 同一个值不能同时由字段和 getter 两处提供。
- 各值的方式与 J273 现状见[修改方案 6.3 节](client-display-delegation.md#63-不可算值的注入方式re-7)。getter hook 只在 Client 会话期间返回同步值。
- **RE-7静态交接（2026-11-12）**：[注入表](client-display-delegation/RE-7/injection-table.md)已核天使占比`0x7EF590`经XMM0返回，不能用普通float ABI；药丸识别15B在ItemPool+7E0，官方已有UnidentifyPill Lua扩展，正式安装复用其有界C++写法。诅咒使用Host有效值（审阅后改为同步 raw `Level+0xC`／`Game+0x25AC8`，原生 getter 在 Client 上即可算出相同有效值，见修改方案 6.3），Add／Remove混合setter不作任意最终值安装；价格保留Host最终Price／Sprite，不调用含Random／Spawn的SetPrice。地图摘要、程序道具配置／资源及进度查询的具体owner接线仍待实施，不能因来源已定位就宣称Client已支持。地图拾取图标的 Client 接法（镜像最小 SavedEntities 条目，或构建后修补缓存）在本步实施前选定。

**事件播放**：

- 事件队列由第 12 步接收。Client 在帧号到达事件帧时，在主线程播放音效、音乐和文字。
- 落后超过限定帧数的纯外观事件直接丢弃。
- 事件只驱动本地播放，不修改已同步状态。
- **表现状态**（2026-11-12 追加，RE-8核对）：房间音乐请求、循环音效集合、各控制器ItemOverlay按状态安装。音乐按请求版本／播放代次调用原生，含Queue，不在未变状态包上重播；Client房间／Boss自选曲保持隔离。循环声保留实例／显式Stop与用途，IsPlaying不是完整baseline。ItemOverlay在**新演出代次**先Show再装state／delay／姿态，相同id重播也处理；包含id45的第二份MegaMush Sprite，Player引用交原生owner。
- **音效去重（Q7）**：Client 放行的原生显示代码发出的音效直接本地播放；Host 只把显示作用域外的玩法路径音效作为事件发送。回放事件调用 `SFXManager::Play` 时带“回放”标志。来源判定见[修改方案 6.4 节](client-display-delegation.md#64-事件与状态re-8)。
- 道具横幅和 fortune 按本地化键在本机语言下解析。
- **RE-8静态交接**：[采集与载荷](client-display-delegation/RE-8/sources-and-payloads.md)及[Q7作用域](client-display-delegation/RE-8/audio-scopes.md)已完成；fortune用兼容MSVC list构造，不能强转网络数组。回放标志位于C/S主线程栈式上下文，不给原生Play增加参数；FrameDelay是重播间隔。RoomTransition混合根不整体进入显示作用域，只标记Client实际执行的显示子段，其他声音来源继续正常采集。**审阅后追加**：回放调用 Play 时 FrameDelay 传 0（节流 `0x907D2F—0x907D5A` 只在 `now < next` 且 `next ≥ 0` 时跳过）；Q11 后 Client 不运行 Start／Update 的任何部分，转场内三个声音点全部作为 Host 事件；BossReplay 隔离选曲 `0x823D8C` 与环境声清除 `0x823D9D`，`Game+0x22E90` 写入由下一次 Q8 安装覆盖。

**原生切房过渡（Q5）**：

- 转场事件关联Host接受后的启动与真实换房后的最终目标；Start=`0x823990`的五参数是请求来源，Maze仍可能在Level里重选，不能当最终world身份。
- 原生Start／Update均为混合根，保持拦截；~~显示推进在相同顺序位置运行具名显示拆分，复用原生对象／Render／本地surface。窄显示驱动是RE-9实施建议，尚未接线。~~
- ~~`RoomTransition::ChangeRoom`完整原函数继续拦截，NativeBuild／NativeApply安装同token完整目标。目标未就绪时，在上层显示阶段冻结mode／counter／progress，不让void ChangeRoom空返回后原FSM继续；新surface只在目标提交后捕获。~~
- **Q11（2026-11-12 用户决定，取代上两条中的拆分与冻结）**：Client 不运行 Start／Update，也不写自有 FSM 驱动。转场显示字段随 world 原子安装（见上方“转场安装”），新房间 world 晚到时画面天然停在旧房与 Host 当前进度。`RoomTransition::ChangeRoom` 仍拦截，只作守卫；新房间目标由 NativeBuild／NativeApply 安装，新 surface 在提交后捕获。Client 本地只做 surface 捕获、滚动补偿、versus／肖像 ANM2、HUD 刷新请求和原生 Render。
- 细节与 RE-9 见[修改方案 4.5 节](client-display-delegation.md#45-原生切房过渡q5)。
- [RE-9五项静态成果](client-display-delegation/RE-9/README.md)已记录Start／Update／Boss完成／ChangeRoom玩法写入、最终目标关联、resize／Render写集及E39恢复事项。完整RoomInit、额外RoomUpdate、Player模拟、进房选曲、进度与显式present不因原生过渡方向而获得执行资格。

**回调分派（Q6）**：Client 放行路径上的 Lua 回调分派一律放行，不逐个白名单。仍拦截的玩法路径不运行，其中的回调自然不触发。

**失败方式**：见[修改方案](client-display-delegation.md)第 12 节第 1—9、13—37 条，以及审阅后追加的第 44—65 条。

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
- **绘制上下文／结果未恢复**：Manager相位错窗或未恢复，两个具名分支仍推进Client RNG，或临时alpha／颜色栈在CPU消费后未正确恢复；旧D0 seed镜像不能补救。

## 完成内容

本次仅完成文档整理与静态审阅；以下为本步骤后续实施／验收需要交付的内容，不表示已经实现、编译或通过测试。

- **实现交付**：需完成数字分派、结构／热路径、完整owner生命周期及合法读回／恢复，并在实施阶段完成相应纯产品编译；本次未实现或构建。
- **代码约束**：未来切换时退出已替代的字符串分派；所有新核心函数和数据结构须带中文注释，正常构造／资源／引用保护保留。

## 未决问题

- 哪些字段可以用 `direct`，需要第 3—8 步逐字段给出证据。
- Client 仍在运行的原生路径（Render、维护 helper）会改写哪些状态字段。这决定哪些块需要漂移校正。

## 历史修订提示与原分域补充

以下保留原页首与原正文中的修订补充，按各自时点阅读；旧版本号、未生成／待注册和原位置措辞不覆盖上文现行清单。版本47的Player和版本57的Room／Level已经入源，完整消息／结果批次及运行owner仍按各项实际状态说明；历史失败、UNKNOWN及尚未观察的事实不改判。

**原页首提示**

**2026-11-10 第7步版本57交接**：[正式Room／Level生产布局](implementation-7-room-level.md)已生成，实际writer／绘制adapter仍待本步。R0 descriptor为物理source-slot，MapFocus的引用为描述成员序号，二者不可混用；15B配置／水光波纹／FX／轨道及地图结果须走具名owner和同world依赖，不能裸写保存体或恢复生产器。

**2026-11-10 第7步显示／资源应用合同（待实施）**：按[补齐设计](room-display-resource-design.md)由本地owner构造仅供显示的配置承载、资源图像／有限surface合成器和自有纯地图adapter；Data／OverrideData独立，StageId35是Home依赖，绝不重建Spawn／Merged生成图。水色直接消费最终32B不再插值／乘量，背景水纹phase与816BB0的2*GameFrame分开；lighting／wave使用世界结果并按本地视口投影。地图消费完整可显示事实与挑战32锚点／描述，保持各端查看／Options，跳过分数重算及混合Refresh／Update。资源候选齐备后提交，保护FBO／shader／pixel-store／CPU队列／strong引用并安全退休，不调用Init／Restore／Spawn补画面。正式ABI／writer仍待实施，版本56，无测试。

**2026-11-10 RoomSave／Exploration安装交接（待实施）**：按[两域目录核对](room-save-exploration-synchronization-review.md)地图使用自有L1/L2、MapBadgeList及同world资源／标记结果，不把图标记录写成保存vector，不调用Save/Restore/Clear/UpdateVisibility或随机Refresh。逐项徽标由有序ID选择icons／curses帧，独立恢复图形状态；不能只装Sprite最后一帧。查看／长按／视口／展开alpha本地维护，world限制另施加，隔离原版Online输入。回访／回溯／Myosotis／Moving Box等只安装Host当前结构与对象状态，按真实use退休，不拿seed／ordinal／ESSM ID续接身份；不动访客存档／统计／解锁。本轮无writer或测试。

**2026-11-10 RoomDescriptor显示安装修订（待实施）**：[专门核对](room-descriptor-synchronization-review.md)要求五位flags中的HasWater保留Pit Light／Backdrop32重绘条件，不能当waterAmount或WaterClip；献祭／pitch-black使用Room最终lighting，不重新执行PreRender/GetLightingAlpha。GridLight的815B80仅在受控调用栈用所有模式的真实Boss匹配，不能放行本地ListIndex/lastBoss选择或748900的本地玩法判断补源。官方ASMHud颜色由具名adapter使用D1的1B是否ModelingClay及实际Player颜色上下文；>0恒真，不伪造GetErrorTrinketEffect数值。实际Image照常安装，不Recompute／查Client本地SpawnSeed。L2是自有地图事实，不能将图标包直接写入78h vector或原生Restore；不运行Reset/move assignment/补货/改价/支付解锁。实际typed writer、缓存绘制adapter与图形恢复仍待接线，本轮无测试。

**2026-11-10 Level安装合同（待实施）**：按[Level字节与职责](level-synchronization-review.md)先提交活动源槽／三维占用／独立双配置，再按0..2维度建立本地+182C8别名，保留入口象限、特殊负目标及Lil Portal寿命；不写-19，不以ListIndex代替物理源槽，不调用GetRoomByIdx的TLS fallback／路由。L0/L1只是设计投影，尚无生产layout：有效诅咒、visited／clear／red／Devil's Crown／Blue Key Light与Greed显示用专门adapter，隔离本地种子再次合并及原生玩法flags写入。原生借用必须按原owner恢复，不永久安装Host全flags、保存体或返回FSM；禁止Level Init/Update/ChangeRoom/放房/Restore、Myosotis再创建、原生地图随机或存档补状态。L2实际图标／楼梯／challenge32位置及配置资源未到报Gap，不ACK。SDK1890C边界与Game前缀18910分开，未知不写，本轮未接线或测试。

**2026-11-10 Room应用交接（续核修订2）**：[Room职责](room-synchronization-review.md)及[剩余字节补证](room-residual-evidence.md)以最终water／lighting／波纹／FX和实际背景资源替代生产输入。最终water32B不得写回端点再运行原插值；同world水面方向／最终有符号phase须与实际配置匹配，静态纹理不能替代它。环境voice按实际sample／world逻辑gain／pitch／pan维护，不安装Room平滑数组或重播延迟计时。绝不安装／调用+7378的Host函数地址，也不执行TriggerOutput、格子延迟队列、死亡名单／历史、雨、碰撞／查询／原版Online更新。背景不按RNG重抽，Camera／Room／Backdrop混合Update及地狱背景82EE90继续禁止。正式writer／draw尚未接线；同world几何／门别名／资源版本与本地视口／FBO／owner恢复分别维护，必要结果缺失具名失败，不补零或ACK。

**2026-11-10 Random应用合同（交接修订2，待实施）**：按[逐用途结果合同](room-random-handoff.md#3-d0以具体结果解除全局游标重放依赖)仅从5B D0借用Manager相位／paused，不安装两份seed或重演全局流。受控Pickup／Familiar随机及资格段用同版本alpha／已确认不使用状态替代；Familiar建立本地中性ColorMod并对称pop，Pickup临时Sprite和CPU消费后恢复。旧alpha、未执行、缺源不能填0／不适用，额外本地可见调用无对应结果时报Gap；不同pass／递归不按本地访问序号猜映射。地图用Host已选锚点／描述关系及最新L2结果，禁止随机Refresh；Error仅用最新1B谓词或完整最终颜色，不重算seed；音频指定实际sample维护逻辑voice／生命周期，不调用随机Play／SetAmbientSound或复制Host后端音量。生成／池／运行库引擎均不写Client，正式adapter尚未实施，无测试。

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

**原正文分域补充**

**版本31公共字段安装合同**：版本30排除的33个范围保持不解码、不安装、不比较；版本31再移除公共valid及`entity_valid_presentation_owner`。普通对象由本地构造／发布／池／退休owner维护valid；Player由P7完整结构事务接管活动／备用资格，不能用热块改池占用，也不能把所有已构造身体置true或把valid当作room_member。旧Player结构源的独立资格在新路径接管前保留。exists／dead继续按最终状态安装，不以成员离表、HP或动画派生，不执行Kill／Remove。原生半帧历史及PauseTimer仍不上线，网络插值如实施仅用本地快照。其余有真实展示用途的碰撞类别、尺寸和SpeedMultiplier保留。具体依据见[公共复核](entity-common-synchronization-review.md)和[生命周期再复核](entity-lifecycle-synchronization-review.md)。

**版本32动画／资源／颜色安装合同**：见[动画／资源／颜色职责复核](animation-resource-synchronization-review.md)。两组动画事件位只在本地构造／切资源／切动画时清理，不恢复Host历史、不漂移比较；最终动画选择、两组selector、anim_frame、is_playing与playback_speed继续安装，官方绘制插值不因此被停用。ANM2 loaded由file_name可空资源选择驱动本地owner：Null安装已构造空资源，非Null等待全部依赖成功后发布／ACK，不直接写true；加载失败不能当Null。LayerState loaded交本地owner，分别安装路径元数据与实际Image，Host待重载但仍使用旧Image时不得仅因新路径抢先LoadGraphics。ColorParams队列不重建／推进／比较，E7为空；Entity.Color、主Sprite最终color、champion_color及图层color仍按完整11分量安装。上述新资源handler尚待第8／10／13步实现，旧路径不能被视为已接管。

**版本33 Familiar安装边界**：11个Host参数不安装／锚定／漂移比较；FireCooldown、State、Coins、IsFollower及原有最终姿态／资源仍按真实展示依赖安装。80的表现缓存随成员generation退休，240候选与资源完整后才替换，失败不ACK；native尾部／节点未知区不复制。详见[Familiar参数／充能／绳索复核](familiar-synchronization-review.md)，新布局不等于执行器已经接通。

**版本34 Knife／Bomb／Tear安装边界**：[Knife／Bomb／Tear参数展示复核](knife-bomb-tear-synchronization-review.md)的42个Host范围不安装、还原或漂移比较；保留Knife效果位、RotationOffset、当前距离／速度及飞行状态，Bomb五份costume／绘制开关与pending=false，Tear效果位／DeadEye及三份辅助Sprite。Height、Scale、Rotation等输入的最终公共绘制偏移／Sprite变换单独安装，不调用GetRenderHeight、ResetSpriteScale、Flags／Scale setter或LoadCostumes求值；不放行原生Interpolate重演运动。新接线仍待本步，不能把生成表视为运行时替代完成。

**版本35 Pickup／Slot安装边界**：[Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)中的Host输入不安装或漂移比较，Pickup只保留Price／DropDelay及原有主／两份辅助Sprite和三引用，Slot保留最终主／奖品Sprite。奖品ANM2的active_animation／is_playing是实际绘制条件，不是Slot.State或PrizeType；price ANM2的loaded由本地资源owner维护。Pickup固定循环数组／count由本地构造与复用owner初始化为空，不恢复Host未来列表或读取池残留。禁止自动价格、MakeShopItem、ReloadGraphics／SetForceBlind、循环Morph、SetPrizeCollectible及购买／捐款／开奖等玩法。新接线仍待本步，保持候选失败／资源依赖和正常退休合同。

**版本36 Projectile安装边界**：[Projectile运动与展示参数复核](projectile-synchronization-review.md)的八项Host运动／效果输入不安装或漂移比较。保留Height：659460在私有+440开关下临时从公共PositionOffset.y减去它，draw scope须保存并精确恢复实际改写的+350，不能只依赖浮点加回。公共EntityFlags、最终位置／Velocity／偏移、尺寸、Sprite及DepthOffset单份映射保持；Game+25A70展示时钟继续供应原Render的局部随机相位。私有+440及+460／+464仍UNKNOWN、来源／安装待补齐，不能补零或称已支持；不运行Update／Interpolate、碰撞／Deflect或+470回调重建轨迹。

**版本37 Laser安装边界**：[Laser／Homing采样、运动与计时复核](laser-homing-synchronization-review.md)排除十九项Laser输入、六项Homing标量及中间缓存／系数；不写回、不作漂移比较、不恢复两个Laser计时锚点。保留直接展示字段、两组最终点列、BounceLaser及公共最终姿态／Sprite，不能重算追踪／旋转／收缩／采样。旧实例中间容器若有旧分配须经owner正常释放，不memcpy零头；不得将其owner与保留点列共用，也不通过ClearLaserSamples／Plus重采样请求破坏最终点列。不运行Update／Interpolate／接触／碰撞或级联Remove。

**版本38 Effect安装**：按[Effect各臂与payload复核](effect-synchronization-review.md)六项Host参数、四变体TearFlags和136描述没有writer／原值读回／漂移比较；State、Timeout到期帧及其余真实展示输入保留。136只用最终主Sprite和Player持物展示依赖，不创建active Grid或执行举持／落地。MrMe只建表现ANM2，四项私有状态留本地正常初值且不让玩法消费。Trail用独立64B节点重建逻辑deque、保留完整历史几何与参数，不重算累计距离或衰减。Ghost按三投影一致前缀安装全部资源／姿态，末尾无效槽经正常ANM2 owner清理，原生i32 count由共同count重建；其Offset／Scale由已核Render纯展示公式在draw scope派生，保存并精确恢复，既不传Host原值也不修改通用ANM2合同。保留全部本地构造／资源／共享owner生命周期；失败不部分替换，不调用Update／Interpolate、Spawn／传播、loot选择或Trail维护。

**版本39装备／Plus安装**：按[完整复核](player-equipment-plus-synchronization-review.md)四主动槽安装六值、五Weapon槽依真实工厂／动态类安装四值及15号主引用，P4／P5／P8与角色／库存／资源同目标版本；不写或比较来源池／定时充能冷却、五项Weapon输入、Knife hitboxSource。Whip EntityPtr目标先构造再绑定，Familiar同时满足Player／D74／D76／619／type32依赖；D76从Host最终bool安装，不执行跟随逻辑。TemporaryEffects完整配置／count／cooldown和disabled同版本提交，cache mask／evaluate不上线，Player回指由本地P8 owner维护，不调用Add/Remove/ClearEffects／EvaluateItems。两个进度条保留真实动态getter、动画和精确draw恢复；HUD图像／flash等由第8步owner准备，禁止用完整HUD Update刷新。Plus只在同代正确动态类的现有本地holder/data上标准optional赋值，保留Null／false／零；缺失失败不得分配fallback、清整个派生Plus、部分发布或ACK。其他已排除Plus项不恢复。旧Weapon替换须正常撤所有自身EntityPtr、owner=null后窄Free，不用会Remove攻击实体的ClearReferences／DestroyWeapon。
**版本40 Effect后续五项安装合同**：见[证据与落地](effect-next-five-evidence.md)。按11布局及其生成表安装；FX0的Rotation／Scale随State／Timeout同目标版本提交，FX1仍须等待E8 Parent，不能继续按旧8／21B字段偏移写入。每个已应用world帧对全部Effect以该帧F还原T=A−F，不预测、不夹限负数、不调用Update或SetTimeout补递减；101双减／141刷新反映为Host发送的新锚点。Trail正常owner只重建逻辑0—127个独立64B节点，保持全序、零alpha边界节点和全部参数；不运行追加／裁剪／衰减，空payload不同于缺失。原生ExtraData只允许既有108／166／191三种具体类型，全部投影齐备后经606310一次替换，错variant／未知类型不造fallback。137—141已排除输入不安装、不回调、不Spawn／传播；未知字节及无效holder不会变成原生写入目标。实际接线仍由本步完成，未运行测试。

**版本43 NPC现行合同**：C/S受控UI只以同版本0/2旁表结果消费：第一段6D025D入口，0走6D02C5、2走6D0285；第二段6D0314入口，0走6D03AC、2走6D0340。必须绕过整段Manager+4AFB8读取及原版IsIdxLocalPlayer，不能只替换6D027C/6D0337调用。403物品独立HP条路径、Sprite后续就绪/alpha及可见检查保持；不写F1C，不注册/重绑输入，不全局Hook8E7550。NPC另在6CF09D读取NetplayManager作Spawner为Player时的远端淡出筛选，C/S采用无原版远端筛选语义，具名draw adapter须绕开该整段至6CF0EB，不为它新增网络字段。该点列入本步既有隔离接线，未提前安装Hook。Boss选择不重建、OverlayEffect Scale派生及其他复合owner合同保持。见[来源闭合](npc-control-ui-evidence.md)。

**原正文其余修订提示**

**版本46 Delirium安装合同**：见[companion消费与绘制依赖](delirium-residual-evidence.md)。两个Host u64打包值、Boss选择和BE8均无writer／读回／漂移比较，不能扩大官方u32／u8接口或恢复旧字符串字段写入。410的Child与4A13B0生成的companion按真实独立成员及现行公共关系／资源展示，不根据BE8创建第二个NPC或连带Remove；本地Init空槽保持为空，异常非空交合法owner，不能裸清登记。稳定公共shape下的资源／姿态改变仍更新实际块，不强制退休重建根NPC。143根UNKNOWN为590B，BD4—BD7不写；17／18空预留、710B下界和其他NPC合同保持。实际安装接线仍属本步。
