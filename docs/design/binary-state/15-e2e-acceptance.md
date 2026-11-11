# 15：E2E 适配与统一整体验收

**2026-11-11 第9步各域正文实现，版本60**：[正文实现与边界](9-domain-body-codec.md)、[同源数字计划](layouts/09-wire-format/body-codec-plan.md)。正文实现完成，版本60。本步唯一验收需覆盖所有有负载域、条件隐藏／704、固定槽／u8计数、坏bool／枚举／浮点／mask、带符号排序、Weapon choice、shared首尾、投影计数组／缺base、空主动规范和预算失败。BodyView不等于资源ready／原生提交；运行功能／性能结果仍未产生，本轮没有提前运行测试。

**2026-11-11 第9步公共合同，版本59**：[正式结构](9-public-wire-contract.md)。第9步第一项已登记版本59公共结构；本步唯一整体验收须覆盖双分段原子提交、按Peer body／u32 GameFrame、旧表与序号回收、来源与目标分离、缓存／贡献身份、事件水位／明确Clear、恢复不回绕，以及三端总10Mbps／模组7.5Mbps和首次准备1MiB／10秒。32MiB保护不充当产品帧许可，三端共享编码仍按三份实际发送计费；当前没有提前运行测试。

**第8步版本58交接**：[正式结果与剩余Gap](implementation-8-presentation.md)已登记；本步仍是唯一统一整体验收。届时覆盖两配置挑战32／徽标／边框与独立查看、隐藏心格与官方星象房行、四组多身体详细／公共HUD、同帧多贡献／递归alpha、图像资源与各屏幕／转场及音频world意图隔离；本轮未提前运行任何测试。

**2026-11-10 显示批次及各端多角色HUD定稿覆盖**：[方案B及第9节覆盖](presentation-result-batches.md)纳入本步唯一整体验收：四参与者各端详细HUD跟随自身控制组、Jacob／Esau双身体与换形、其他组公共HUD不丢人；History顺序／实际图像／负ID／shader、Stat三值／星象房、Host个人关闭HUD及独立视口；多layer／pass／递归贡献、来源与逐Peer目标映射、Gap／资源Wait／失败不早ACK、旧批次退休及重复展示不重抽。原生两槽不作为系统容量，既有body窄依赖和三端预算保持。不新增逐组验收或测试编号，0—14步不提前执行；本次仅文档静态审阅，版本57不变。

状态：设计稿，未实施。上级：[总计划](README.md)。

**2026-11-10 各域交接统一整理**：现行入口为15.5“各域统一验收交接清单”，按来源、特殊约束、版本依赖与待落实项登记全部相关域。本次静态审阅的布局源为57；布局入源、运行接线和第15步验收分别说明。本次只改本文档，不修改实现／布局源／生成表，不构建、不运行测试。

## 目标

本步做三件事：

- 把 E2E 工具改为验证二进制状态路径；
- 加入两类新的检查：未知区探测和 Lua 对照；
- 修订 tx-04-21 的 E01—E26，作为整个重构**唯一的**统一整体验收。

本步之前的 0—14 步不运行任何测试。本步开始后连续执行，遇到问题就地修复并续测，不另开编号。

**2026-11-10 范围调整**：本步先验收原版内容，包含原版全部必要类别／流程、四参与者／多身体及既定性能指标。第三方 Mod、StageAPI、自定义角色／资源／绘制及 Lua adapter 的适配用例已迁至[独立后续方案](../mod-adaptation.md)，不属于本次验收。原 E31 保留编号并标记迁出，不执行、不计通过；其余条目不重编号。继承旧 E01—E26、18—20 回归及补充矩阵时同样按此范围剔除第三方适配要求，保留全部原版要求；测试夹具本身不构成 Mod 支持声明。

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

- **未知区发生变化**：先按实际消费者、来源与生命周期补核归属；变化本身不能证明它应下行，也可能是Host生产／本地缓存／指针。确需修改布局时沿唯一源递增整数版本，在本步内修复并继续必要续测，不将原始失败或未知记录改成先前已经通过。
- **未覆盖的区间**：如实记录，不能宣称“完整”。
- **未知区未变化**：只说明本次覆盖窗口内未观察到变化，不消除UNKNOWN或补造字节语义，不单凭静止样本宣称类型完整。

### 15.2 E01—E26 修订

原条目中原版内容的场景和指标保留，判定依据改为新格式；第三方 Mod 适配部分按上述范围迁出。主要修订如下：

| 条目 | 修订 |
| --- | --- |
| E01 | 握手比较布局版本；版本不符时拒绝，并报具名原因 |
| E02 | Baseline 改为“成员表 + 资源表 + base=0 状态帧”；体积 ≤1 MiB、时长 ≤10 s；按布局拆分体积 |
| E03 | 指标不变；另报主线程采集按域的耗时、每帧字节数、变化块比例 |
| E05、E06 | 增加“变了又变回”和“最后一次变化后静止”的收敛检查 |
| E09 | 按当前逐行回映射比较实际字段／派生及跨owner最终结果：Host二进制状态↔合法Lua对照↔Client原生；原目录改判Host／不适用的行保留理由，不要求原始数值相等，不把Getter当正式同步来源 |
| E10、E11 | 改为验证：丢帧后由下一帧自愈；ACK 丢失后已确认版本不回退；Resync 回到 Baseline |
| E13 | 增加错误 member_epoch、resource_epoch，未登记的成员序号，以及布局编号不符 |
| E15 | 改为：块写入失败后，确认停在上一完整版本，Host 继续携带失败的块 |
| E17 | 增加 15.1 中的全部格式故障注入 |
| E25 | 改为 18—20 的应用回归，加上回映射覆盖报告 |
| 原补充矩阵中的旧内容（63／64／65 项分块、graph_ref、参数矩阵） | 改为原版容器上限边界、查询派生检查；Mod 图编码的共享与循环已迁出 |

### 15.3 新增条目

| 条目 | 内容 |
| --- | --- |
| E27 | 未知区探测：所有布局的未知区间都有结论；变化的区间已归类并重测。包括 J273 池步长或工厂分配大于 SDK 声明的尾部字节：Effect、Knife、Slot、Projectile（第 4 步），Door、TrapDoor、Fire、Poop（第 6 步） |
| E28 | 漂移校正：Client 不运行原生 Update，漂移只来自仍在运行的 Render 和维护 helper（第 13 步 13.4），例如 base draw 写 Entity `+0x1A4/+0x1A8`。这些路径改写的字段被检测并纠正，没有振荡，耗时计入安装预算 |
| E29 | 成员序号回收：大量生成与移除后，序号在所有 Peer 确认之后才复用，旧帧不会写到新成员上 |
| E30 | 资源表：同路径重载、贴图替换、新资源首次出现都正确，序号不复用 |
| E31（已迁出） | 原 Mod adapter 编码用例移至[后续方案](../mod-adaptation.md)；本轮不执行、不计通过，不作为原版完成条件 |
| E32 | 当前唯一源登记的全部Effect布局／变体，按实际RTTI与选择器核Host／Client一致：<br>- variant136只用最终主Sprite、State及Player持物／BodySprite，不发保存描述；<br>- 54／126仍按102，113按141，167按版本45的148，19／71／194按149；其余按当前正式臂，不把旧版本40的“11布局／167仍102”当现行范围；<br>- 108／166／191三个typed payload生命周期、Trail0—127及Ghost0—4有效前缀／多投影；<br>- 传播／Timer／生成／计数等只看最终展示与独立成员，FX2空预留，不执行Host请求／回调；未列变体仅按正式声明的基础臂处理，未知语义仍记录 |
| E33 | 展示时钟与空帧（第 11 步 11.3、第 13 步 13.6）：静止场景中 Host 每次采集仍发出帧，Client 展示时钟随帧头 game_frame 前进。另在 E2E 构建中临时屏蔽 D0，构造 record_count=0 的空帧，确认它被接受、确认并推进时钟 |
| E34 | 帧锚点（第 3、4、7 步）：静止和移动场景中，Client 的 Entity FrameCount（`+0x328`）、`Room:GetFrameCount()`（`+0x11F0`）、Effect Timeout 按展示时钟换算后与 Host 一致；仅在已核的线性计时区间要求锚点不变；Effect负值／停走／101双减／141刷新等按真实A变化重发并同帧还原，静止位置不保证锚点不变 |
| E35 | 绘制上下文与随机结果（第8步8.2.1）：5B D0仅Manager相位／paused，与正常Host结果批次及真实world／presentation修订绑定；逐subject/use/shape/pass/重复调用的alpha／资格不按Client全局顺序分配，两个seed／shift／guard不下行。覆盖暂停／重复／不同成员顺序／可见分支／递归、缺源不填0、旧资源／world拒绝、Pickup临时Sprite与Familiar颜色栈在CPU消费后完整恢复 |

### 15.4 执行方式

各域场景、当前布局与比较口径统一见15.5；本节保留未来整体验收的执行安排。

- **人数顺序**：先两人（P2）跑全套，再四人（P4）扩展，与原方案一致。
- **启动前沟通**：每次实机启动前，告知用户需要配合的操作和何时停止手动操作；需要手动配合时，先确认用户已准备好。
- **连续执行**：遇到失败先修复，再在本步内续测必要的条目，不因失败默认停下。
- **材料保留**：临时材料用完就清理；只在中途停下、需要续测时保留必要材料。
- **不放宽**：不放宽指标，不扩大池或预算上限来让测试通过。

### 15.5 各域统一验收交接清单

这是未来第15步统一整体验收的设计清单，**本次没有运行用例、故障注入、构建或游戏，也没有新增实机授权**。来源是[第9步完整编码交接](9-wire-format.md)、[第10—14步](10-host-capture.md)的实际来源／发布／解码／应用／退役合同及旧E01—E35范围；原编号保留，E31迁出且不计PASS。版本57布局入源、已有辅助函数／纯编译与运行覆盖分别报告，未注册结果先完成正式接线，不能以省略原版必要内容缩小验收。

| 域与来源／沿用条目 | 特殊约束与覆盖内容 | 身份／版本依赖 | 待落实项与记录口径 |
|---|---|---|---|
| 公共协议／收敛：[第9步](9-wire-format.md)、E01/02/05/06/10/11/13/15/17/29 | 版本不符、截断／错长度／mask／epoch、非法引用、变了又变回／最后变化后静止、丢帧ACK与Resync；不完整源不污染发布基线，部分应用不ACK新target | world/body分序列、成员use/shape／epoch和资源／专用依赖完整，成员离开后旧帧不得写到复用号 | 正式工具／故障注入及跨Peer场景待实施；本次未执行，旧E02失败与未执行项保留，不从样本包数推断应用成功 |
| 基础值／容器／资源空间：[第1—2步](implementation-0-2.md)、E09/13/17/25/29 | 固定槽无count、u8/u16显式计数、条件／choice／精确字节、共享投影同count与计数组、空↔非空／缩短／预算／非有限值；资源表间同u16不互换 | 真实生成布局与同目标引用／资源，已裁字段不造线上非法值要求，原生坏源按Gap看待 | 真实边界与失败候选清理／正常owner记录；不复制活STL／地址，不靠截断／默认值通过 |
| Entity公共／Sprite／Plus：[公共职责](entity-common-synchronization-review.md)、[动画资源](animation-resource-synchronization-review.md)、E09/25/28/29/30/34 | exists/dead与本地valid不同；完整selector／最终颜色／playing/speed，路径先变而Image未变、空资源／失败，nullable未设／false／零／非零切换与反射消费 | 当前use／shape、实际资源／动画及同展示时钟；事件／loaded／颜色生产队列不比较Host原值 | 真正保留输入／最终画面与正常退休待验；漂移比较不机械复用Host根偏移读Synthetic或本地scratch |
| Familiar：[结果／绳索](familiar-synchronization-review.md)、E09/13/17/25/28/29/34 | 80真实充能动画与资格、240 DAC分段／XYZ绳索四投影、蝗虫两组合计16384、IsaacsBody／D76表现，非空绳索0节点拒绝 | 主体／owner／资源／同目标几何及真实引用；时钟未知不伪装Frozen，节点未知不写 | typed reader／owner／adapter仍需运行覆盖，不能把生成表或空Weapon算充能通过 |
| Knife／Bomb／Tear：[三类复核](knife-bomb-tear-synchronization-review.md)、E09/13/17/25/28/29/34 | 分裂独立成员／拖影／飞行、五costume＋pending清除读回、三Sprite／DeadEye／Light；42个裁剪范围及旧锚点不比原值，不本地碰撞／重建资源 | 公共最终姿态、真实关系、辅助资源同目标，借用精确恢复 | 场景／非法实际字段／正常撤边与无玩法重演待验；历史未知不因减少线上字段消失 |
| Pickup／Slot：[职责复核](pickup-slot-synchronization-review.md)、E09/13/17/25/28/29/34 | Price与水面／价格Sprite、DropDelay独立隐藏、替代底座／MegaChest及三引用；Slot1—18奖品实际动画／playing。循环切换只比较当前后态，不比未来列表／开奖输入 | 计时与同world时钟，主／辅助资源与真实关系，随机alpha另列 | 空列表／缩短／清理／失败不ACK，确认无定价／购买／捐款／TryFlip／开奖副作用；本次未覆盖 |
| Laser／Projectile：[最终采样](laser-homing-synchronization-review.md)、[版本45](projectile-effect-residual-evidence.md)、E09/13/17/25/28/29 | 两点列不同count／空↔非空及Light unoptimized，真实sample_mode等；Height开关原始u8含非1值、冻结锚点和公共偏移，+350精确恢复，无采样／半帧／callback | 同目标完整几何／资源／flags／use与world帧，PR2不按最新Position伪造 | 当前真实字段／几何／候选替换及异常预算待验，旧版36“私有来源未闭合／只Height”不作当前结论 |
| NPC／Delirium／Text／Dummy：[NPC](npc-control-ui-evidence.md)、[Delirium](delirium-residual-evidence.md)、[Text](text-dummy-evidence.md)、E09/13/17/25/27/28/29 | NPC0/2与拒1、三deque／双Sprite／null Scale恢复／403HP；绕开受控UI与Spawner原版Online筛选。Delirium资源变不无故换代、companion独立；Text原始非UTF8／空／缩短／255界及146counter，Dummy历史cache | 真实RTTI／use与合法producer、Text独立scope不拼ordinary，不能将type964 Dummy套NPC | 实际writer／UI分支／历史显示源与池复用分别观察，无法取得真实场景记录未覆盖，不造假对象补通过 |
| Effect：[当前臂／计时](effect-next-five-evidence.md)、[版本45](projectile-effect-residual-evidence.md)、E09/13/17/25/27/28/29/32/34 | 当前全部正式臂，16/13B、Trail缺失与存在0/1/126/127／拒128、Ghost0..4多投影、typed owner错型／退休；Timeout负值／停走／101双减／141刷新／回绕精确同F还原，不本地Spawn／Timer／传播 | Parent、variant／RTTI、同holder／资源／world目标；静止位置不要求锚点不变 | 来源／多投影／CPU生命周期及真实UNKNOWN待验，已解决producer事实不重新算未决，未核变体仍不宣称语义完整 |
| Player快源／角色结构：[正式Player](implementation-5-player.md)、E09/13/15/17/25/28/29/34 | 四参与者／多身体、主备／held/twin／独立激活及roster/C/S组；P0＋三类Sprite独立块，P6内容变而地址/count不变、旧body丢弃与ordinary body-joint，空槽与跨身体借用 | participant/group空间9/10、真实use/shape、统一P0/P6拓扑和资源；P1—P8不混body，组数不等于身体数 | 全真实角色／身体族与slot容量／快源超64KiB退ordinary路径待验，不以少量静止身体代替支持范围 |
| Player库存／装备／效果／HUD输入：[中三块](5-player-p3-p5.md)、[后三块](5-player-p6-p8.md)、E09/13/17/25/27/28/29 | 固定4Pocket/8材料/6poop、dense／有效innate；四主动六值／空槽规范0，五Weapon槽分型mask／恒零类／10和11／Whip真引用与正常窄Free；P8 disabled与完整有序效果、只704加cooldown、残影／当前0/1节点 | 同资源身份／配置type-id、Player拓扑／组关系及同world，Plus模型不当已核偏移／API全量承诺 | 真实查询显示／装备／容器生命周期待验，不执行获取／使用／到期／HUD Recompute；不得比较已裁History／group/timer等原值 |
| Grid公共／17派生：[第6步](6-grid.md)、[旁表](grid-plus-water-source-evidence.md)、E09/13/17/25/27/28/29/34 | u32水面高位／nullable／Lock第七点及index别名，glitch纹理变换不靠Client地址；Fire≤5×16B与state分支／模板帧、Rock碎块条件、Pressure计时／Door目标Light／主色与extra，正常门别名退休 | 实例use、Room几何／颜色、目标描述／HasWater／Boss匹配／资源与nested_layout同world | 真实工厂／writer／draw恢复与静态描述→原生接线待验；禁止Init／Reward／Update／官方旁表写入补状态，UNKNOWN保留 |
| Room／背景／FX／rails：[版本57](implementation-7-room-level.md)、[显示资源](room-display-resource-design.md)、E09/13/17/25/27/28/29/30/34 | 十二房形／L形／448格最高位及256以上轨道／八门、特殊背景／冷纹理／water32B不二次乘量、独立phase／lighting／两波纹半帧、空ray／poop／vein图／重复展示、强制焦点与不同视口／Options | source-slot、真实几何／门与资源／world/presentation/pass完整，Host裁剪前缀不代表全可显示事实 | 正常来源／GPU／adapter／图形状态与CPU资源退休待验；不本地生成雨／装饰／矿车或执行82EE90／Camera.Update／Host函数表 |
| Level／Descriptor／保存探索：[两域](room-save-exploration-synchronization-review.md)、[版本57](implementation-7-room-level.md)、E09/13/17/25/27/28/29/30/34 | 三维／大房入口象限／负ListIndex／路由／Lil Portal、-19隔离、同count原位映射变化、双配置／HasWater／所有模式Boss／Greed／有效curse；回访／Myosotis／Moving Box／Flip／矿车／Maze／红房／Hourglass／Ascent／Continue的当前后态 | 活动源槽／描述成员／L3映射／双配置和world/use修订，不以ESSM ID续接；探索可回退 | 失效／重建／扫描预算与稳定保存时点待验；官方Greed Getter18204不与真实18374强行相等，不用错误Setter造样本 |
| 地图L2／Focus／Badge／Question：[地图设计](room-display-resource-design.md)、[两域核对](room-save-exploration-synchronization-review.md)、E09/13/17/25/28/29/30 | 可见未访问／完整L2的16类OR/max及9类动态帧、重复／顺序不改聚合；徽标333替三项／376追加／curse顺序／7上限与空列；challenge32两配置锚点／描述／安全169，Lost独立问号空／换图／失败 | L0—L3知识、地图／资源／world修订与显示资格一致，Host按键不控制访客展开／本地排版 | 纯地图／资源和计数／条件错误待验；Gap不折空、不写保存vector、不调用随机Refresh或分数重算，不能只比最后Sprite帧 |
| Run／HUD／屏幕UI：[Run](run-synchronization-review.md)、[Presentation](presentation-synchronization-review.md)、E09/13/17/25/27/28/29/30/34 | TimeCounter基准／七种子投影／ascent/daily、current颜色／两黑暗／最终分数／进度图标；内嵌／动态HUD、24心／4主动／2饰品／4Pocket、Stat三值／星象房、有序History负ID／图像、11消息／fortune／Prompt／过渡／死亡／暂停与屏幕pass | 当前Game／冻结结算／身体显示归组、资源及world／显示修订；本地视口／菜单提交／Options独立 | 未注册根／来源／owner与全部原版必要UI待验；不运行Calculate／UpdateHearts／原版Online选择／退出／解锁／排行榜请求补值 |
| D0／Random／时钟：[逐用途交接](room-random-handoff.md)、E09/13/17/25/28/29/33/34/35 | D0=5B，实际alpha／资格、暂停／重复／不同成员顺序／可见分支／递归／多pass，旧seed不上线；E33在E2E专门构造合法空帧推进时钟，Familiar栈／Pickup临时Sprite消费后精确恢复 | subject/use/shape/pass／调用身份、world/presentation和资源同批；不以仅GameFrame或本地序号映射，旧批次拒绝 | 正常生产结果／外层封装与时钟借用待验；未产生不填0，不能额外Render／抽样，E33夹具不进入产品 |
| 配置／图像／shader资源：[typed设计](room-display-resource-design.md)、E02/09/13/17/25/28/29/30 | 同名／独立／原位变化、StageId35与Type16组合／Door依赖；文件路径与实际Image不同步、程序图冷连接／pixel/UV／padding／精确RGBA8、recipe／blob缺块冲突／乘加／依赖环／失败退休 | 不可变内容号／空间／epoch、完整依赖ready与同目标应用；固定头登记不等于整Context可用 | GPU／shader允许集／内容物化与闭包待验；全部表／recipe／压缩blob／结构计首次1MiB/10秒及持续7.5Mbps，不降质／漏计 |
| Music／SFX／环境声：[音频职责](presentation-synchronization-review.md)、[sample来源](room-random-handoff.md)、E09/13/17/25/28/29/30 | 多sample／voice、16环境声／pan、crossfade双voice／六layer／jingle、持续声Baseline／中途加入／静音／停止clear与跨房，one-shot重复状态不重播；FrameDelay非启动延迟 | sample资源、voice代次／逻辑gain/pitch/pan／相位及场景生命周期，本端音量／静音独立 | 正式事件／持续状态／非随机播放leaf待验；不能只按soundID／currentId或仅one-shot断言声音完整，后端静音值不反除 |
| 全程隔离／性能／产品：[现行规范](../../refactor/project-conventions.md)、[总计划指标](README.md)、E02/03/09/17/25/27/28 | Host≥50FPS，采集／动态安装p95≤2ms、p99≤4ms，约30Hz ordinary／60Hz合格身体，512/128/128MiB三池与原带宽目标不放宽；产品无Lua/JSON热链／E2E夹具／第三方适配，四参与者统一输入与C/S隔离保持 | 同一实际发布路径／版本与真实游戏Client，诊断额外成本与正式性能分开计量；访客进度／Hook常驻／生命周期覆盖整个窗口 | 第10—14步全链与工具完成后未来统一验收；不测试原版Online隔离已永久移除项，不以期末dirty=0替代全过程副作用理解，UNKNOWN逐项保留 |

### 15.6 交接状态与连续验收口径

- **范围与结果分开**：上表均为待落实／待执行范围，没有本轮PASS。回映射用于确定字段／派生／Host／不适用及跨owner结果，不是把旧ReplicatedState原值全量恢复；相同原始数据被多个域引用时只按唯一owner采集，再检查各必要消费者。
- **版本与证据**：记录实际布局版本、场景、key/use/shape与world/body／成员／资源／显示修订，未注册结果／未知ABI／缺合法producer直接说明实际缺口。仅按整数版本兼容，不计算哈希，不设独立审计或预封流程。
- **未知与未覆盖**：发生变化、未观察到变化、未覆盖都保留真实标签；须补证才能改变字节语义。不因字段留Host、计数裁剪、单次静止或SDK声明就称类型完整，不将未执行项目或E31计通过。
- **执行与恢复**：未来按15.4先P2全套再P4扩展并覆盖必要多身体，失败修复后仍在本步骤续测受影响及未完成范围。每次启动分别提示配合与停止手动操作时机，需手动配合时确认准备好才启动；本次没有发起启动或授权询问。
- **材料与说明**：正常测试临时材料用后清理，默认不保留artifact；仅必要中断时留首因／进度／恢复所需材料并说明用途。最终报告实际覆盖／未覆盖、失败／修复／续测和仍存缺口，不用成功结果改写旧失败。

## 改动范围

- E2E 工具：对照、探测、统计、故障注入。
- E2E 方案文档：在本目录下另起 `15-e2e-plan.md`，保存修订后的完整条目与指标。
- 进度与恢复：只在未来验收必要中断时保留续测所需记录，并说明位置／用途；正常连续完成不为本步骤强制另建进度artifact。本次不创建测试执行材料。

## 失败方式

- **对照工具接入同步路径**：Lua 对照被正式同步路径调用，或者影响主线程的计时结果。
- **探测成本未扣除**：未知区探测的诊断复制计入正式性能指标，使结果失真。探测和计时要分开运行，或者分别报告。
- **对照对错帧**：Host 采集和 Lua 对照不在同一窗口、同一次采集，误判不一致。
- **只看平均值**：只报告平均 FPS 或平均耗时，掩盖 p95、p99 或最大值超标。
- **混用旧材料**：tx-04-21 的暂停材料与新格式的结果混用。
- **锚点字段对照口径错误**：Client 侧对照在展示借用之外读取 FrameCount、`Room:GetFrameCount()` 或 Timeout，用的是 Client 自己的帧计数，误判不一致（第 13 步 13.6）。
- **空帧无法出现**：D0 几乎每帧变化，正常运行中很少出现空帧；不专门构造就宣称空帧路径已验证。

## 完成内容

本次仅完成文档整理与静态审阅；以下为本步骤后续实施／验收需要交付的内容，不表示已经实现、编译或通过测试。

- **未来验收完成条件**：当前原版范围的E01—E30、E32—E35须按两人全套和四人扩展完成，实际失败／首因／修复与必要续验如实说明；E31迁出不计PASS。原版必要项不得按Mod适配迁出，本次没有任何新增通过结论。
- **字节归属**：全部布局的UNKNOWN保留语义／证据与实际覆盖结论；未观察到变化不等于语义已知。类型字节归属完整与功能／场景验收分别说明，不以静止或一次成功推定全部原版行为完整。
- **回映射**：覆盖报告中，每个 `ReplicatedState` 成员都已比较，或已说明为什么不比较。
- **性能**：总计划第 1 节的性能指标全部达标。

## 未决问题

- E2E 对照与正式采集在同一窗口内的计时隔离方式。
- `out/tx-04-21-p2-20261106-01` 恢复材料的处理（第 0 步，用户决定）。

## 历史修订提示与原分域补充

以下保留原页首与原正文中的修订补充，按各自时点阅读；旧版本号、未生成／待注册和原位置措辞不覆盖上文现行清单。版本47的Player和版本57的Room／Level已经入源，完整消息／结果批次及运行owner仍按各项实际状态说明；历史失败、UNKNOWN及尚未观察的事实不改判。

**原页首提示**

**2026-11-10 第7步必要显示／资源待验收范围（仅本步执行）**：按[设计第8节](room-display-resource-design.md#8-后续实施验收与剩余未知)覆盖双配置同名／独立／原位变化、Home StageId35和Type16分别组合／Door Light、十二房形与特殊背景、冷连接错过纹理生产、文件路径先变而实际Image未变、程序图同名／pixel与UV／padding／RGBA8精度／资源失败与退休。覆盖最终water／mode／flow／方向、不同本地图形选项／视口、lighting425／460／NPC407／特殊−9／Home、两波纹半帧／重复显示；地图全部可显示事实而非Host窗口前缀、两边框／徽标、挑战32锚点／描述／安全169边界及本地独立查看。blob／recipe计入首次准备1MiB／10秒和三端持续7.5Mbps；不隐性改预算、降质、调用随机／生成／分数重算或写访客进度。本轮只有静态设计审阅，未执行测试。

Lost诅咒还须覆盖独立question_image的默认／空／实际内容变化与失败不ACK，不能仅比较两个图标Sprite或硬编码文件。Minimap+614..61B已核为ImageStrongPair，直属未知23→15B不等于其余UNKNOWN已解决。

**2026-11-10 RoomSave／Exploration验收范围补充（仅设计）**：[197行目录／同步决定](room-save-exploration-synchronization-review.md)要求覆盖地图可见而未访问／四种揭图与诅咒显示／各端独立展开；L2动态图标／楼梯／Portal及重复／顺序不改聚合；MapBadgeList的333覆盖三项、376追加、诅咒顺序与7项上限／空列／本地排版；当前保存体陈旧、回访／Myosotis／Moving Box／Flip／矿车、Maze交换／红房／维度／Ascent／Hourglass／合法Continue失效重建。超预算／源失效／资源缺失不得当空或ACK；不运行Client保存恢复／随机Refresh／原版Online输入或写访客持久数据。API类型／Grid Clear／ESSM marker差异按实际语义审阅，不以旧签名相等代替结果。所有测试仅在本步骤统一执行，本轮无测试。

**2026-11-10 RoomDescriptor验收修订（仅本步执行）**：[专门核对](room-descriptor-synchronization-review.md)补普通TrapDoor Boss匹配／route、HasWater与waterAmount独立的Backdrop32/58/59坑Light／重绘、献祭／pitch-black最终lighting、Error→ModelingClay图像与颜色、D1仅1B bool且完整效果ID不发／数值Getter不伪造。商店两表及价格封顶只比较最终Pickup/Price/Sprite，Client不补货／改价／花费解锁。L2覆盖16种mask、9类动态帧／固定0不发送、心／硬币／钥匙／药丸／69箱子／Card/Rune／Slot default与14／Portal／楼梯、重复和顺序变化而聚合不变不重发；拒绝条件长度／帧界／错world、Gap伪装缺席、源扫描超预算或误写78h保存头。描述直属未知24→0不消除保存74/type5视图70/Grid3B及外部未知。本轮只登记第15步范围，未执行测试。

**2026-11-10 Level验收口径（仅本步执行）**：按[静态核对与失败方式](level-synchronization-review.md)覆盖三维／大房／L型缺口／入口象限、负ListIndex/Genesis/-100/-101与-20寿命、-19不读写、同count原位映射／图标变化、双配置／门Light、visited布尔及五位flags无关位不重发、Greed波次／图标、有效curse强制/阻止、challenge44最终背景／楼层名、challenge32已选地图位置与保存实体图标／Grid楼梯。Myosotis、portal／Death Certificate仅Host执行创建／返回，Client无生成／过房／存档副作用；根22B及保存等外部未知保留，描述原24B已核为Host商店表不下发。官方Greed Getter读18204错误映射项，真实计数18374，Lua对照不得要求两者相等或调用错误Setter造样本；不比较未发送的seed／全flags／保存历史。本轮未生成Level生产布局、未执行上述用例，不新增提前测试或验收编号。

**2026-11-10 Room待统一验收范围（续核修订2）**：按[Room职责](room-synchronization-review.md)及[剩余字节补证](room-residual-evidence.md)覆盖两张448项表边界／占格计时不当path、最高格／256以上轨道／八门别名与特殊目标、所有房形／特殊背景／冷纹理、水量／水色插值／颜色空间／流水／冷却熔岩，以及反向水面／不同FPS／重复显示。覆盖16环境voice的中途加入／停止／静音／跨房／pan，死亡名单标记与实际奖励、十组输出后态、空FX／空ray组／poop／收缩vein、强制焦点／各端视口、暂停／换房。确认不传或执行Host函数地址、格子延迟队列、死亡历史、EntityList碰撞／查询／原版Online环，无Client雨滴／补货／装饰／Minecart重复生成或Camera／地狱背景Update移动实体。水色不二次乘量，ray相位不累计，程序图像不按同名复用；UNKNOWN和缺来源不伪装空后ACK。仅本步统一执行，本轮未提前测试。

**2026-11-10 Random待统一验收范围（交接修订2，仅本步执行）**：按[15项交接](room-random-handoff.md)覆盖零额外随机／回调／Host Render、5B D0与逐用途alpha／资格、首次／暂停／重复显示／不同成员顺序／不同可见分支／多pass／递归、Familiar颜色栈平衡及Pickup临时Sprite／CPU消费后恢复。缺源、旧world／资源修订和额外Client可见调用不得填0或复用旧alpha；两seed／shift／guard与BD0F40不下行。覆盖挑战32锚点／对应描述／裁剪窗口／标记、169边界安全拒绝或具名支持、Error 1B谓词变化、药丸／Loot／池隔离、实际sample／多voice／静音／停止／跨房／持续baseline；不比较未发送流的原值。本轮没有执行上述测试。

**第6步影子描述代码（仅本步执行验证）**：按[已实现接口与失败方式](grid-description-implementation.md)覆盖17类／27种type的完整记录生成、错key/use/shape/layout、缺块／未定义块／预算、合法旧变化版本、Statue三空块缺席和224无Seed。检查描述／投影复制共享自有字节及租约、借用撤销后内容不变、不同角色真实nested_layout和ANM2条件投影仍受第12步递归校验；完整u32水面、Door负索引／两个布尔、Fire16B粒子与Rock有序重复偏移保持。旧JSON legacy描述不得隐式进入正式安装。第10／12／13步运行接线完成后再验证实际游戏行为及AppliedAck，本轮仅纯编译未执行测试。

**2026-11-10 Presentation待验收范围**：按[具体清单](presentation-synchronization-review.md#8-后续正式布局与唯一验收)覆盖四参与者/多身体/动态HUD、24心与特殊健康/HolyMantle/低血、StatHUD0—8行/两列、11消息槽/fortune/程序图、History顺序/负ID/shader、各端独立地图展开/视口/挑战32/黑框、屏幕/转场/死亡/暂停只读结果，以及crossfade/jingle/各端静音/loop中途加入与停止/one-shot去重。确认不运行HUD/Map/Overlay/概率/分数生产器，不触及Online或访客进度；未知来源/图像失败不伪装为空后ACK。本次未提前执行测试。

**版本55 Grid回映射消费口径（仅本步执行）**：按[正式119行](layouts/06-grid/grid-catalog-members.md)选择实际布局及处置；Lua对照以目录路径＋行号保留第2/8、61/63、105/110行的属性／方法区别，不按ID去重。field／derived只比较对应展示投影，不恢复74行Host原值相等；ReducedSpawnRate第92行不适用，不探未知字节冒充该属性。SpawnSeed的198／224、GetSprite的实际nested_layout、Rock条件主姿态／layer0及无符号GetBigRockFrame须分别覆盖；Render／RenderTop行不授权重放原生副作用。正式reader／writer、七族水面、资源／pass来源和借用恢复仍须实际接通并在本步验收，生成／纯编译不算测试通过。本轮未执行测试。

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

**原正文分域补充**

**版本36 Projectile口径**：按[Projectile运动与展示参数复核](projectile-synchronization-review.md)，八项Host运动／规则输入不上线、不做原值／漂移比较。E09／E25覆盖追踪、左右曲线、加速／减速、下落速度／加速度、摆动相位、Scale选图及ProjectileFlags组合产生的最终轨迹／大小／Sprite／独立效果；Height覆盖+440两分支与公共偏移非零、Shadow／水面／排序及特殊绘制路径。E28／E29确认没有本地Update／Interpolate／碰撞／+470回调，+350及Sprite借用精确恢复；Game帧展示借用仍供应原Render局部随机相位。E13／E17继续检查Height有限值及真实资源／引用，不再构造八项被撤销字段的非法负载；PR1预留不作为构造必收。E34不新增摆动／其他运动锚点；E27保留全部375B未知，私有+440及+460／+464来源仍未闭合，不能跳过场景后称画面完整。只安排第15步，未提前执行。

**版本35 Pickup／Slot口径**：按[Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)，20个原生范围留Host、19项原线上字段／容器撤销，不比较其原值或未来列表。E09／E25覆盖选择组拾取后的成员退出、触碰／充能、自动价格／补货重抽、盲目图层、循环切换后的最终成员与Sprite；保留Price／价格图形和水面反射、DropDelay正值隐藏／归零显示。Slot覆盖1—18变体的支付／捐款／触发／开奖后最终Sprite及6／15奖品Sprite的动画与播放条件。E28／E29确认无本地定价／循环Morph／支付／开奖／资源生产setter／持久副作用，检查循环本地安全空初始化、资源借用和正常退休。E34撤销Pickup Timeout／Wait，仅保留DropDelay与其他真正线上计时；Slot八个标量也不建立锚点。E13／E17不构造已撤销字段／循环数组的非法负载，继续检查真实资源／选择器／引用与DropDelay编码；预留块不作为构造必收负载，E27未知不变。只安排第15步，未提前执行。

**版本34 Knife／Bomb／Tear口径**：按[Knife／Bomb／Tear参数展示复核](knife-bomb-tear-synchronization-review.md)，42个Host参数不做原值相等／漂移比较或线上字段拒绝用例。E09／E25覆盖Knife投掷／挥砍／旋转／路径／分裂及拖影，Bomb下落／火箭／爆炸／不同范围伤害Flags和Scale对应的最终效果，Tear等待／粘附到期、下落／追踪／父跟随及不同高度缩放旋转的最终位置／Sprite；保留Tear flags／DeadEye的Room Light、排序／水面与所有辅助Sprite。E28／E29确认未放行Update／Interpolate、爆炸／击退／LoadCostumes／ResetSpriteScale／高度重算，Bomb pending清除及owner退休保持。E34不再比较Bomb ExplosionCountdown、Tear WaitFrames／StickTimer锚点；其余真正线上计时保持原合同。E13／E17继续校验真实线上浮点／引用／资源／容器，E27未知范围不变。只安排第15步，不提前运行。

**版本33 Familiar口径**：按[Familiar参数／充能／绳索复核](familiar-synchronization-review.md)，11个Host参数不做跨端原值比较或线上拒绝负载。E09／E25检查轨道最终运动、Hearts／Keys／清房奖励后的最终画面和方向／射击动画；80覆盖原版Weapon充能族、modifier、0／1／中间／100、空Weapon、出生6帧／隐藏限制和四动画阶段；240覆盖payload空↔非空、DAC空↔非空、逻辑环顺序、全段／后半段、节点Z／反射及Sprite资源变化。E13／E17拒绝amount越界、非有限XYZ、非空payload节点0／超限、非法map／错类型与投影count不一致。E28／E29覆盖DB8借用恢复、候选失败／释放、真实引用撤边及无玩法重演；E27增加payload尾44B、嵌套ANM未知与每节点108B未知观察。布局与来源已补入，运行时handler和owner仍未接线，不能标通过。

**版本32动画／资源／颜色口径**：按[动画／资源／颜色职责复核](animation-resource-synchronization-review.md)，不比较Host事件位、两个loaded原始字节或ColorParams队列／锚点，也不为这些已排除字段构造线上拒绝用例。E09／E25覆盖主／叠加／逐层／空层独立姿态、播放／停止与不同速度的官方绘制插值，以及颜色覆盖优先级、淡出、正／负Lifespan与shared传播后的最终Entity／Sprite／图层／costume颜色；完整ColorMod的11分量继续对照。E28确认事件本地清理、无事件重放／本地颜色合成及官方插值临时改写恢复。E30覆盖Null↔非Null文件资源、Client等待／加载失败不得ACK、路径已变而仍画旧Image、随后实际Image替换及Null Image；本地准备结果不由Host布尔覆盖。E34不再覆盖ColorParams时钟，其他展示计时保留。新资源handler与未核Player／Grid／HUD owner未接通不能标通过。

**版本31生命周期口径**：按[再复核](entity-lifecycle-synchronization-review.md)，公共valid不做Host↔Client逐字节相等对照；E09／E25／E28／E29覆盖本地发布／池资格、角色29／38互惠backup复合绘制与递归终止、Player结构激活资格、非房间但exists=true的备用存储以及尚未退休的死亡展示。exists／dead继续按回映射比较；不能通过Kill／Remove或同步Host池字节取得期望值，也不能默删旧Player结构资格源。P7编码与writer未接通不能标通过。

**版本30公共字段口径（valid部分由版本31修订）**：按[Entity公共复核](entity-common-synchronization-review.md)，33个已排除范围不比较Host↔Client原值、不为不上线字段构造解码拒绝用例。E09／E25检查毒／火／冻结等最终flags、颜色／图标、HP／死亡、暂停及最终位置；覆盖valid参与的角色29／38替身复合绘制、exists／dead差异、碰撞调试几何和Effect172旋转／光照。E28检查本地工厂／池生命周期及完整绘制借用恢复，不能通过同步Host工厂字节修正本地owner。E34不再比较公共20个计时器和毒／火相位，版本32又移出ColorParams，继续覆盖子类及Player实际保留的展示计时。插值如实施只使用本地已应用快照历史，不放行原生Interpolate。未知字节与既有失败记录不变。

**版本29内部关系口径**：[24项逐槽复核](internal-entity-relations-review.md)中23条Host边不再比较原值，也不构造其未知引用负载作拒绝测试；E09／E25继续比较分裂／终端等独立成员最终画面。Familiar +DAC只在240布局130检查，payload补齐后覆盖空↔非空造成的绳索分段；当前不能标通过。E29检查真正保留引用和被排除槽的本地空状态、旧绑定正常撤边以及不得级联误删。Light必须覆盖Laser未优化采样点。下文“全部独立引用”均指当前实际线上引用，不能恢复旧23条玩法边。

**版本28回映射口径**：按[字段职责复核](synchronization-field-review.md)，17个已改权威范围不再要求Host↔Client原值相等；四类命中历史连元素也不比较。E09／E25仍覆盖分裂、掉落／伤害、弹幕变更后的最终成员与画面，并确认Client未重演玩法。Familiar的FireCooldown／State／Coins等实际展示依赖继续覆盖，variant80最终充能表现尚待接线，不能记为已通过。未闭合候选仍按现有线上归属比较，不提前裁剪。

**版本37 Laser口径**：按[Laser／Homing采样、运动与计时复核](laser-homing-synchronization-review.md)，E09／E25覆盖追踪、曲线、环形、旋转、收缩、父体跟随和接触派生造成的最终Angle／Length／点列／Sprite／独立成员；兼顾sample_mode_raw=-1／0／1、完整效果位、公开端点、MaxDistance／GridHit及+4F0展示分支，Room Light实际使用unoptimized。E13／E17拒绝真实上线点数越界、非有限几何／标量、来源变化、未知引用、分配失败及预算溢出；不构造撤销输入的非法负载。E29覆盖两组不同点数、空↔非空／增长／缩短、同目标版本完整安装、失败不部分替换、正常退休及本地indices／Bezier空owner；E28确认Client不做采样／Update／Interpolate／接触模拟，draw借用精确恢复。E34撤销两个Laser锚点／原值比较，其他实际展示时钟保持；E27保留421B未知。预留L2／L4／L7／L8不作为完整构造必收负载。本轮只修订验收安排，未执行。

**版本38 Effect验收口径**：按[Effect各臂与payload复核](effect-synchronization-review.md)E09／E25／E32比较最终画面、Scale在variant121的Light、172旋转、160连接几何、196跟随显示、136持物及落地后的真实成员、三种payload全部实际状态；不比较六项Host参数、TearFlags／136描述、MrMe四项Host私有状态、Ghost派生Offset／Scale或独立i32 count原值。Ghost覆盖0—4前缀、三投影count一致、同资源不同实例、缩短／归零／再增长与present切换；E13／E17拒绝源i32负数／>4收窄、线上计数不一致、错RTTI／present／资源／非有限值及预算溢出，不构造已撤销字段负载。E29核正常清理四槽ANM2、独立Trail节点及共享owner和失败不部分替换；E28核draw临时派生精确恢复，Client无Trail追加／衰减、loot选择、传播／Spawn、举起／投掷／落地或Update／Interpolate。E34仍比较真实展示Timeout的负值、停走、跳变与锚点重算；E27保留根对象和各ANM2的UNKNOWN，不把Ghost未发送槽中的未知消除。MrMe16B与Trail0x40已有静态正面证据，原未决事实见历史报告，不再把旧producer缺口当现行同步Gap。这里只安排第15步，未提前执行。

**版本39装备／Plus验收口径**：按[完整复核](player-equipment-plus-synchronization-review.md)E09／E25覆盖四主动槽／空槽的Charge／Battery／Sub／Partial／VarData及特殊HUD，15种Weapon及10／11同vtable、两个真实进度条、Whip主实体＋Familiar D74／D76展示切换、Berserk704环、Null109／112／trinket69效果及disabled且列表非空的查询差异。E29覆盖五槽类型替换／清空、Whip主引用正常撤边、效果列表清空／重建及窄Free，不级联删除独立成员；E13／E17拒绝真正在网的坏资源／引用／count／非有限值／nullable与预算溢出。E28检查无Fire／Update／HUD Update、效果到期／增减、重算／回调玩法补值、重抽／重采样／初始化音效，以及进度条／HUD临时姿态精确恢复。Plus覆盖未设→false／有效零→true／非零→再未设，Scene实际水面／反射须消费；其他已排除项不恢复。E34核最终效果cooldown与世界展示帧，不为已撤销冷却新增锚点；E27保留Weapon20B、TemporaryEffect4B、外层2B及各Plus未知。只比较保留字段与最终画面，不比较八个撤销范围；这些测试仅在第15步执行。
**版本40 Effect后续五项验收口径**：见[证据与落地](effect-next-five-evidence.md)。E09／E25／E32覆盖11布局、113正确落141、六个传播variant与Timer／生成／计数的最终展示，客户端不读取或重演Host请求／函数回调；原有三payload与未核variant的UNKNOWN保持。E32覆盖Trail present缺失、存在且0／1／126／127节点、增长越过原生128裁剪点后仅发布127，以及缩短／再创建；不为测试触及生产数据而新增持久副作用。E13／E17拒绝线上count=128／65535、长度不符、旧版本／错臂、非法浮点／资源／实际预算；原生损坏／中途采集的128或更大数量应报Gap，不能截断通过。E34分别覆盖负值、零、线性递减、101局部＋公共双减、141刷新、暂停及跳帧／回绕，比较同已应用world帧精确还原；仅线性区间断言锚点不重发，不把静止位置当计时条件。E28／E29核未知或错variant非空holder拒绝、前后变化报缺口、正常owner清理、无Spawn／传播／回调／Trail维护，失败不部分替换；E27继续保留167后16B及各臂剩余未知。本批没有运行上述测试，第15步仍为唯一统一整体验收。

**版本43 NPC现行合同**：受控UI覆盖F1C=-1→0及其他合法来源值→2、C/S本机/远端参与者相同展示语义、来源变化/窗口失效/读取失败报Gap、线上1/其他非法值拒绝、Client设备变化不改变已应用表现。用同一次整体验收确认reader不读Manager/NetplayManager、不调用IsIdxLocalPlayer或slot5；Client两个受控UI整段和Spawner远端淡出整段均不借原版Online筛选。保留403独立HP路径及所有原有可见/资源条件；Delirium资源变化不强制换代、OverlayEffect Scale恢复、五标量/完整点列/引用/UNKNOWN范围保持。未提前执行。见[来源闭合](npc-control-ui-evidence.md)。

**原正文其余修订提示**

其中Type964是NPC Render的静态数值分支；实机场景必须先有真实NPC RTTI及合法producer，不能把同数值的NativeDummy强套NPC布局。未取得来源时如实记未覆盖，不造V1或假对象满足用例。

**原15.4分域补充（旧时点说明）**

第4步原未决前四项的补充沿用既有条目：版本28起Knife K4六个f32只归Host权威，E09／E25比较分裂后的最终成员、位置／姿态，不再比较K4原值；`+0x518`是碰撞方向角，Client不得为验证其效果执行原生碰撞。E13／E17只对实际线上字段拒绝非法浮点，不再要求拒绝不上线的K4负载。E27保留Knife末四字节、Slot末四字节、Projectile末四字节和Effect`+0x41C/+0x44C/+0x4BC`的未知区观察，并复核六段空白的填充／保留字节假设；Effect传播／生成用途已静态核定，验收确认Client未重演这些Host玩法。Slot进度指针、Projectile回调指针不得按状态写入或执行；Effect不得在Client重演Spawn／传播。这里只安排第15步覆盖，未提前运行。

第 4 步 Tear／Pickup／Laser 的覆盖并入上述既有条目，不另开验收编号：E09／E25 比较三份 Tear 辅助 Sprite、两份 Pickup Sprite、全部当前线上独立引用、循环切换后的最终形态／Sprite、Laser原始有符号采样模式、真实展示标量和optimized／unoptimized两组最终点列；版本35起不比较Host未来循环列表；E13／E17 覆盖 count 越界、非有限几何、未知引用和分配失败；E29 覆盖关系替换、撤边及级联 Remove 不得误删独立存活对象；E34仅覆盖此组三类中仍上线的Pickup DropDelay条件计时，Pickup Timeout／Wait自版本35、Laser Timeout／RotationDelay自版本37起不比较；其他真实线上未知时钟仍须报Gap；版本34撤销Tear两个计时的锚点比较；E27 包括这三类的全部未知区。容器内容必须完整提交，不能只比较 count 或跳过空集合清除。

Slot／Projectile／Effect 同样归入既有条目：E09／E25覆盖Slot 1—18变体、奖励Sprite最终外观／动画／图层及真实播放条件，以及Projectile的Height／公共DepthOffset单份映射与最终轨迹／Sprite；版本36起不比较ProjectileFlags等八项Host输入；Slot八项Host参数自版本35起不比较，其原生16位等宽度事实仍保留；E32 覆盖 Effect 六臂、三种具体 owner 的存在→缺失→再创建和同目标版本跨块变化；E13／E17 拒绝非法 present、缺失值非零、尾投影缺少存在性、未知动态类型、非有限轨迹与超预算。E29 覆盖旧 owner 正常退休、失败候选不得部分替换；E34 比较 Effect Timeout 的负值、停走、跳变和到期帧重算，E27 包括三个根对象和外部 payload 的全部未知区。基础臂未核 VarData 不得按零或不透明内存伪装成完整状态。
