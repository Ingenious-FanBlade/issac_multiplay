# 5：EntityPlayer

**2026-11-12 显示委托修订**：

- Player 重标，玩法内部容器移出同步。保留以下内容：
  - costume 层、身体与头部 Sprite、颜色；
  - HUD 读取的库存、主动道具与充能、口袋、饰品、心和显示用属性；
  - 持物与多身体。
- PlayerPost 约 60Hz 的身体窗口保留，只承载需要的字段。
- HUD 由 Client 原生代码读取这些状态生成。
- P0—P8 子文档同样适用。见[修改方案](client-display-delegation.md)第 5、7 节。

本次只修订文档，未改代码，未运行测试。

**2026-11-09 第5步正式布局源与生成完成，版本47**：[实现说明](implementation-5-player.md)与[EntityPlayer生成表](../../../../mod/binding/layouts/05-player/EntityPlayer.md)。唯一源`mod/binding/state/player.json`新增150—197共48个布局，Player197共25块；条件效果、动态Weapon、固定槽、Plus投影及1265条应用行／97条补充成员已正式生成。Player／Plus未知仍3707B／9B，Plus ABI模型不升级证据。Release／Win32产品纯编译与链接通过，没有运行测试；第8—14步实际接线及第15步统一验收仍未完成。以下较早记录保留原时点。

**2026-11-09 第5步前两项设计工作完成（未生成／未实现）**：[统一原生字节账本](player-byte-ledger.md)覆盖Player 2EA0h、保存／移动箱／动态装备及PlayerPlus等58种类型；根对象239个直接范围，递归UNKNOWN仍3707B。Plus A8h及后半偏移明确为静态ABI模型，未知9B，不能称官方DLL实测。[成员逐行回映射](player-member-map.md)保留1265条旧应用行及重载，另列14条native、77条具名宏与6条结构，旧Entity目录逐行交叉引用。原版通信+268C及待移除效果队列不上线；无调用方的Captured通用字典退出设计，真实原生显示查询走具名最终决定，具体缓存／批次布局仍归第8/9步。剩余为唯一Player布局源／正式生成与采集、安装、退役接线；版本仍46，没有构建或测试。以下较早记录保留各自时点。

**2026-11-09 P6—P8设计定稿（未实现）**：[服装／结构／效果与History裁剪](5-player-p6-p8.md)。P6只保留costume配置／飞行／overlay与最终map，统一Player绘制拓扑；P7为21B身体结构＋C/S组记录，保留真实twin／backup／held及独立激活资格，native sub／replaced／703四槽留Host；P8保留当前效果／残影／当前可绘记录，cooldown仅704传输。原始History／16索引／form counters留Host，实际图像列表与Render内分数重算的最终结果归第8步。P0—P8九块设计已完成；全Player逐字节账本、PlayerPlus、回映射／生成及运行接线仍待完成。布局版本仍46，没有构建或测试。

**2026-11-09 P3—P5设计定稿（未实现）**：[库存／主动槽／Weapon设计与原生证据](5-player-p3-p5.md)。P3保留当前展示查询实际输入、四口袋、材料／法术队列，blocked＋innate按有效计数投影，管理时限／voided等留Host；P4为96B，空槽规范化；P5为1—91B，五种恒零类型不传三项充能输入、其余类型只传公式需要的Modifiers位，Whip主引用保留。32展示根的有界参数核对将三组特殊mask／饰品输入留Host，跨域计算查询最终显示来源仍待完成。P0—P5设计已完成，P6—P8及全对象字节闭合／运行接线待完成；布局版本仍46，没有构建或测试。

**2026-11-09 P0—P2设计定稿（未实现）**：[前三块设计、J273消费者与原生归属](5-player-p0-p2.md)。P0保留真实闪隐／比例／headAnim及五项Render充能输入，Sprite按投影分块并核P6拓扑；P1收窄为1B CanFly，属性栏最终缓存归第8步；P2为77B，补Jar／额外生命项，EternalHearts／BoneHeartsMask原值由最终逐心缓存承接。本文后续同主题初稿以该设计为准。没有生成Player布局、改运行代码或测试，布局版本仍46，P3—P8及全Player字节闭合待完成。

**2026-11-09 当前版本39：Player装备与Plus展示依赖。** [完整消费者与安装复核](player-equipment-plus-synchronization-review.md)将ActiveItemDesc来源池／定时充能冷却、Weapon五项非展示输入及KnifePlus.hitboxSource留Host；保留主动槽六值、Weapon充能三值与类型、TemporaryEffect三值及Plus公共nullable覆盖。新增135号UmbilicalWhip真实主引用、136号TemporaryEffects外层disabled／列表，并补Familiar+D76绘制抑制字节。原已排除Plus项不恢复，其他UNKNOWN保持。源、生成表、正式Plus读取接口和第5／8／10—15步已修订，未构建或测试，运行时仍待接线。

状态：Player的P0—P8设计、统一字节账本、逐行回映射及正式布局源／生成已完成，版本47；UNKNOWN保持。Player采集／安装／退役及第8步最终显示缓存仍待后续步骤实施。上级：[总计划](README.md)。

**版本33 Familiar跨域展示**：[Familiar参数／充能／绳索复核](familiar-synchronization-review.md)核定Incubus只需最终充能amount，不在Client创建其玩法Weapon；第5步Player Weapon目录不能被当成Familiar接管完成。Gello绳索另保留Player／反射／隐藏依赖及typed payload，与本步身体结构／武器owner分别接线。

## 目标

完成 `Entity_Player` 子类区（0x410—0x2E9F）的字节归属。划分逐身体快源块（body 序列，约 60 Hz）和普通块（world 序列，约 30 Hz），覆盖内嵌 Sprite、容器、装备、关系和查询类状态。

## 输入事实

### 对象大小与 SDK 声明

`EntityPlayer.zhl` 声明对象大小 0x2EA0，74E981的释放大小同为2EA0。旧资料的129字段计数不能代表当前SDK所有声明或字节归属；本轮统一账本按239个直接范围及嵌套类型展开，UNKNOWN显式保留。

### 已核位置（`world_layout.json` 常量）

| 内容 | 位置 |
| --- | --- |
| 内嵌 Sprite | 自 0x41C 起，步长 0x114，共 13 份（约 3.5 KB） |
| playerType | 0x13BC |
| index | 0x1610 |
| 主动道具描述 | 自 0x1574 起，步长 0x20，VarData 位于各描述内 +0x1C |
| 子身体存储 | 0x1D84 |
| 双生存储 | 0x1E4C |
| 备份存储 | 0x1E50 |
| 替换存储 | 0x267C |

### SDK 中的容器与关键字段

| 字段 | 位置 |
| --- | --- |
| `_costumeSpriteDescs`（vector） | 0x1220 |
| `_weapon[5]`（Weapon 指针） | 0x13D8 |
| `_cacheFlags` | 0x1568 |
| `_controllerIndex` | 0x160C |
| `_trinketsID[2]` | 0x16B0 |
| `_collectibleCounts`（vector<int>） | 0x16B8 |
| `_history` | 0x1DD8 |

### 公开成员现状

- **应用目录**：`player-application.json` 共 1265 个成员。其中 556 个是 I31 加入的只读查询（captured-readonly-query），523 个是 Host 权威操作。
- **Entity 域中的 Player 成员**：EntityPlayer 有 295 个 `ReplicatedState` 成员。
- **只读查询的消费者**：195行／188个ID路由到 `ClientPlayerDisplayView::Captured`，在 `mod/src` 中找不到调用方；真实原生显示查询另按具名消费者处理，不保留整个Captured字典。

### 现有逐身体快源

Lua `CapturePlayerFast` 每个身体每次读取约 45.8 ms，第 73 轮 557 次读取全部失败，失败原因没有被记录（静态推断见 `capture-bottleneck` 记录）。逐身体证言、exact-pair 绑定和规范第 11 节的读取规则继续有效。

## 设计

### 5.1 字节归属

- **内嵌 Sprite**：13 份内嵌 Sprite 各按 ANM2 布局处理（第 2 步）。版本32起由各自的可空ANM2文件资源号表达有无资源，`loaded` 原字节不上线，由Client资源owner维护；加载等待／失败不能当成Null或提前ACK。实际Image与路径分开，is_playing／playback_speed及完整最终颜色保留，见[动画／资源复核](animation-resource-synchronization-review.md)。
- **容器**：`_collectibleCounts`、`_costumeSpriteDescs`等按变长投影处理；原始History经P8核定留Host，最终HistoryHUD／库存图像列表归第8步，不能因原先列为容器而自动上线。
  - `GetCollectibleNum` 等查询只有存在实际受支持消费者才需保留；由库存、效果等已发送状态重建时，应给出具体Client路径。
  - 若实际展示查询无法重建，才考虑具名最终结果。不能因查询无法派生就自动“换算发送”，也不能为无调用方的旧Captured字典复制整套模拟状态。
  - 这是本步的核心核对项，旧目录中 3605 行的参数查询矩阵借此退役。
- **Weapon五槽**：[P5定稿](5-player-p3-p5.md)已核15种类型的factory／动态大小、五槽顺序、10／11共用Bone以及Whip主引用。五种恒零类型不传三充能输入，其余类型按公式mask投影Modifiers；不能复制整个动态对象。通用13／135仍保留其历史16／18B合同；版本47的Player专用181—194已按本设计生成，实际reader／writer仍待第10／13步。
- **身体关系**：P7只将真实twin／backup／held转成员号；native sub／replaced及703全局四槽为Host发现／模拟／恢复存储，不写Client。C/S组／body_role、roster和独立激活资格另按结构记录表达，不用Host设备键或旧getter字典补图。
- **权威状态**：RNG、Host 专属的玩法状态归权威。

**版本31公共部分与结构约束**：P0不携带Host暂停保存坐标、原生半帧历史位置或击退方向；网络展示插值如需要，由已应用world／body快照建立本地历史，不放行原生Interpolate。公共20个计时器和毒／火伤害相位不上线，Player自身`+13B8`伤害闪隐输入保留；`+1338` BlinkLock与`+133C`头方向锁时留Host，不能混同。版本31将valid从公共E2移除，但角色29／38的RenderHead及递归查询仍需要正确活动／备用资格：P7必须接管room／render／update成员、身体角色和原生激活资格；现有`native_membership.valid`只证明room_member⇒valid，不能删去独立资格或一律设true。资格变化走结构事务，不覆盖本地分配／初始化完成事实；[P7草稿编码与结构owner合同](5-player-p6-p8.md#4-p7展示结构原生资格与控制语义)已定，正式生成／writer仍待第5／9／13步实施。见[生命周期再复核](entity-lifecycle-synchronization-review.md)。

### 5.2 状态块（P0—P8已正式生成；运行接线待后续步骤）

| 块 | 序列 | 内容 |
| --- | --- | --- |
| P0 身体快源组 | body | 父E1／E4／E5／E6及本类64＋L字节标量／headAnim，另按19—24号物理块承载13份内嵌及costume ANM2的三类投影；具体见[定稿](5-player-p0-p2.md#4-p0身体快源) |
| P1 展示属性 | world | 1B CanFly；六项stats显示缓存归第8步，cacheFlags／属性生产输入留Host |
| P2 生命与资源 | world | 77B直接存储；健康显示原值、Jar、额外生命修正项、钥匙／炸弹／硬币及专用资源；EternalHearts／BoneHeartsMask原值不传，最终逐心缓存由第8步安装 |
| P3 库存与口袋 | world | 完整counts／饰品／熔炼／wisp及通用查询当前输入、四口袋、queued饰品投影、crafting八材料／poop六槽；blocked＋有效innate投影；voided／管理历史不上线，特殊查询未决单列 |
| P4 主动道具 | world | 四描述六值，各24B、共96B；空Item槽不读旧payload，规范空值；来源池／调度冷却留Host |
| P5 武器 | world | 1—91B五槽存在性／次序／动态展示布局；充能十种类型保留三值并mask Modifiers，五种恒零类型裁掉三值；type与Whip主引用保留 |
| P6 服装结构 | world | 111＋4C B：costume配置／isFlying／hasOverlay、15项最终map的选择／layer／body标记及统一Player绘制拓扑revision；其他生产元数据留Host，Sprite仍归P0 |
| P7 关系与身体结构 | world | 21B每身体＋C/S组记录：角色／索引、twin／backup／held、roster／room／render／update和独立激活资格、组／body_role／active／consumer；sub／replaced／703四槽不上线 |
| P8 效果与绘制历史 | world | disabled＋有序效果配置／count（6B，704另4B cooldown）、实际残影、当前可绘记录＋额外ANM2；原始History／16索引／form counters留Host，最终图像列表及分数归第8步 |

body序列只包含P0逻辑组及父快块，不含P1／P2。P0并非单一大物理块：父0—9、本类P0—P8占10—18、Sprite投影19—24的正式分配符合32块限制。还要检查P6绘制拓扑版本相等。没有合格PlayerPost的身体，P0改由普通窗口采集，沿用现有ordinary body-joint语义，见第11步。

## 改动范围

- 布局源 `state/player.json`。
- Client 端的 `ClientPlayerFastValues`、`ClientPlayerDescription` 以及状态、装备子结构改由影子状态生成（第 13 步）。
- 复用I11—I27已核原生执行器和owner；实际字段按新必要性裁剪，不把旧表所有属性／计时／模板当必传值。

## 失败方式

- **查询派生判断错误**：把依赖 Host 权威计算的查询当成可在 Client 派生，Client 读到不同结果。
- **未加载的 Sprite**：内嵌 Sprite 未加载时，仍按已加载布局复制字段。
- **快源依赖错误**：body 序列的 P0 依赖了旧的成员表版本，身体关系与实体身份错配。
- **按指针判断武器变化**：Weapon 指针在两次采集之间被替换，Host 只比较地址，没发现内容变化。
- **快源静默失败**：沿袭第 73 轮的问题，PlayerFast 失败时不保留原因。新路径必须记录具名首因。

## 第5步布局交付（已完成，运行接线仍待后续步骤）

- **字节归属与表格**：Player 子类区的字节归属全部完成，带证据；P0—P8 块定稿，两张单行表由生成器产出。
- **回映射**：Player 的全部 `ReplicatedState` 成员有回映射，每个查询类成员都标明是派生、换算还是权威。

已完成[P0—P2](5-player-p0-p2.md)、[P3—P5](5-player-p3-p5.md)及[P6—P8设计](5-player-p6-p8.md)，并补齐[统一字节账本](player-byte-ledger.md)和[逐行回映射](player-member-map.md)。每个根字节有分类，未知并未消失；保存体中的不明字段和Plus静态ABI推导不提升证据等级。含RNG／进度／Online分支的真实显示查询由正常Host已产出的具名最终决定承接；这只是来源／隔离设计，具体最终缓存布局、批次封装与运行adapter尚未实施。

## 当前剩余工作

- 正式源、150—197布局、版本47、原生／线格式单行表及分型／固定槽／条件投影支持已完成，见[实现说明](implementation-5-player.md)。设计账本不作为构建输入。
- 正式reader／writer仍须实现唯一源登记的具名handler、读取资格及跨字段合同；Plus沿官方成员访问，不凭推导偏移直接写入。
- 第8/9步完成具名最终HUD／显示决定及批次布局，第10/13/14步接入合法采集、结构／资源owner、安装与旧路径退出。第15步再统一整体验收。

### 5.3 版本39装备展示与安装合同

本节保留版本39事实。Player P4／P5的空值和分型字段已由[P3—P5定稿](5-player-p3-p5.md)细化，P8又按[后三块设计](5-player-p6-p8.md)将cooldown收窄到704；通用13／14／135／136保留历史合同；版本47新增Player专用179／180／181—194及150号效果外层，不改名覆盖通用源。不能把下文通用四值要求重新施加给五种恒零类型，也不能把所有效果cooldown重新加入Player专用投影。

[完整消费者与依据](player-equipment-plus-synchronization-review.md)覆盖Player、PlayerHUD及Familiar跨域：四主动槽Item／Charge／BatteryCharge／SubCharge／PartialCharge／VarData不能由HUD缓存替代；装备充能条由已核动态slot7→slot6及四个保留值重建，不重复发送MaxCharge／两份chargebar；全部已核slot8为常零，仍保持真实Disappear动画。基础类型的body序列标记在嵌入P4／P5／P8时由这三个world块承载，不把装备枚举加入P0快源。

UmbilicalWhip主实体先构造再用EntityPtr绑定，Familiar还需同版本Player关系、PlayerType32／619库存、IsFollower及新D76抑制字节；不能把这些条件当作AI而删除。TemporaryEffects的disabled与完整列表同目标版本安装，配置保留type/id及角色704环使用的config.cooldown，不能用最终costume或库存替代记录。所有效果计时只装已应用帧原值，不更新／到期。

HUD图像、bookImage、chargebar资源与flash／布局缓存由第8步具体owner准备，不运行完整PlayerHUD Update、Add/Use/Charge操作或EvaluateItems。官方1072／1073等第三方回调的有效结果适配已迁至[Mod后续方案](../mod-adaptation.md)，不作为当前原版实现条件；Client不执行Host玩法Lua补值。五个Weapon经已核factory／Init建立本地对象；只写保留字段及15号主引用，其他原生引用保持正常空owner。替换时先正常撤边、owner=null，再窄Free；不调用会Remove独立实体或停音的ClearReferences／DestroyWeapon。

公共Plus飞行／水面覆盖继续由原生owner赋optional，玩家专属Plus已登记174号字节表与P3官方成员投影，仍有9B未知及后半偏移ABI推导限制。角色、库存、资源、Room、效果、Weapon与主体需要同目标版本和合法生命周期；完整Player／HUD接线及全部未知仍按原步骤完成，不能以本次布局生成称为已可玩。
