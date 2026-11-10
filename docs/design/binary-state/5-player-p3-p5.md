# 5：Player 的 P3、P4、P5 设计

**后续实施（2026-11-09）**：正式Player源与生成已迁入版本47，见[实现说明](implementation-5-player.md)。本文件保留设计时点和证据；构建不读取设计JSON。Plus未知与ABI模型限制保持，实际采集／安装及最终显示缓存仍待后续步骤，没有提前测试。

**2026-11-09 后续补充**：[统一字节账本](player-byte-ledger.md)及[旧成员逐行回映射](player-member-map.md)已完成设计登记；本文末节所列这两项待办是原时点状态，以新登记为准。新记录还区分原版通信域、待移除效果队列、移动箱保存体和Plus ABI推导；正式布局／运行接线及第8步最终缓存仍待实施，布局版本仍46。

**后续P6—P8（2026-11-09）**：[后三块定稿](5-player-p6-p8.md)已完成。P4下文“703依赖P7具名备份槽”由进一步核对取代：VarData原值仍保留，但实际显示不解析Host四槽，四槽及native sub／replaced留Host。P8只对collectible704传cooldown，其他效果保留配置／count；实际HUD图像列表与分数归第8步。P0—P8设计完成，全Player账本／生成／正式接线仍未完成，版本仍46；其余本文P3—P5负载不变。

日期：2026-11-09（Asia/Singapore）。目标：官方 REPENTOGON 1.1.3／J273，ImageBase=`0x400000`。接续[P0—P2设计](5-player-p0-p2.md)。本文确定三块的同步职责、原生范围、草稿编码及采集／安装合同；**没有实现Player布局源、生成文件或运行接线，布局版本仍46**。P6—P8、PlayerPlus其余部分及全Player逐字节闭合仍待完成。

**统一回映射补查**：`7895D0 GetTotalActiveCharge`以P4普通／电池为基础，还按P7角色18／36、ItemConfig ChargeType／MaxCharges及655／326例外读取P2魂／血充能；不额外发送总量，也不能简化成两项相加。详见[逐行回映射](player-member-map.md)。

## 1. 判据与资料

沿用前三块的判据：实际原生绘制／HUD／本地安装owner需要的值才同步；有最终显示缓存时，不能再为重建缓存复制全部生产历史。公开getter、旧安装器回读或字段已知offset，不单独构成同步理由。未知字节与尚未证明的展示用途分别保留，不用“便宜、以后可能用到”把它们转为状态。

本轮先读本地原版 `QueueItemData.md`、`enums/WeaponType.md`，REP `PocketItem.md`、`enums/PocketItemType.md`、`Weapon.md`与EntityPlayer资料，再读官方 `EntityPlayer.zhl`、`ActiveItemDesc.zhl`、`PocketItem.zhl`、`QueueItemData.zhl`、`Global.zhl`、`Weapon.zhl`、`PlayerHUD.zhl`、`ItemSpoofSystem.h/.cpp`。复读既有[I11—I15](../tx-04-20/tx-04-20-state-installation.md)、[I16—I20](../tx-04-20/tx-04-20-equipment-installation.md)及[版本39装备复核](player-equipment-plus-synchronization-review.md)，并在IDA静态核对下述实际采用函数、指令、vtable和factory。VA减`400000`即RVA。

**事实等级**：原生函数／factory／指令为本轮J273静态事实；官方hook与STL成员语义为官方源码事实；P3投影、空主动槽规范化、分型Weapon编码及本地投影容器是设计决定。没有运行游戏、构建、测试、校验脚本或哈希，也不声称这些设计已运行通过。

## 2. 结论

| 块 | 保留 | 不同步或改由其他owner承接 |
| --- | --- | --- |
| P3 库存与口袋 | 完整按ID索引的实际库存、两饰品槽及熔炼计数、wisp／附加查询计数、实际会影响通用库存查询的当前覆盖ID与屏蔽值、四口袋槽、queued饰品投影、crafting八字节和poop六槽；REP blocked集合及innate有效计数投影 | voided的未来Use序列、queued的charge／VarData／无关flags、crafting来源池、innate分组管理／临时到期列表／timer／上次角色缓存留Host；HUD最终图像／名称／获得顺序展示缓存归第8步／P8 |
| P4 四主动描述 | occupied槽的Item／Charge／BatteryCharge／SubCharge／PartialCharge／VarData；固定96B | 来源池／TimedRechargeCooldown留Host；空Item槽不读取其无效旧负载，编码为规范空值 |
| P5 五Weapon槽 | 槽存在性／次序／具体类型；有充能消费者的十种类型保留三项输入；Whip保留真实主实体引用 | 五种恒零充能类型不传三项输入；Modifiers只发具体公式读取的位；owner／FireDelay／Direction／NumFired／其它攻击引用不上线；未知字节继续未知 |

P3、P4、P5均为world块，拟定物理块号仍13、14、15；不因为通用基础类型表标body就把装备枚举加入PlayerPost。与P0—P2的0—24物理块安排兼容，没有增加第33块，也不改变现有帧头。

## 3. P3：库存、口袋及查询所需投影

### 3.1 确实需要的来源与消费者

| 来源／J273 VA | 静态事实与决定 |
| --- | --- |
| `collectibleCounts +16B8`，`7699D0`／`769E90` | HasCollectible最终分支与GetCollectibleNum基础值读取按ID索引的实际counts；GetCollectibleNum另加四主动槽等。Player／Familiar／Room Light／HUD的原生展示有这些查询，不能只传costume或从History反建库存 |
| `trinkets +16B0`、`smelted +1738`；`76A680`／`76A700`／`7C19C0` | 忽略modifier的HasTrinket、黄金查询和倍率读两个真实槽及每ID普通／黄金两个signed short。`837720` RenderTrinket也直接读槽ID的`7FFF`与`8000`位选颜色；完整保留 |
| `itemWisps +1F5C` | `769DDD`／`76A1CF—76A1E3`按ID查map，Has与数量的判断不同。保留当前完整有界map，不用wisp Entity个数推断库存计数 |
| `+1FB0`的byte vector、`+1FBC`的byte vector | `76A1FB—76A20F`按ID加入前者的u8计数；`7C19C0`尾部按ID检查后者，**只消费零／非零并加一**。前者传当前计数，后者换算成0／1；不根据未核玩法名猜用途 |
| `+1388`／`+1390`／`+1F94` | `769E…`／`76A24C—76A27D`读取NullEffect488对应当前ID、拥有191时的当前选择ID、ModelingClay当前ID。它们是通用查询的当前有效输入，不是获得历史；保留实际i32，不重跑选择或随机 |
| `+2010` byte | `7699D0`／`769E90`／`7C1A2C`等决定库存效果是否屏蔽；保留当前布尔含义，与CoopGhost及C/S输入资格分开，不推造完整玩法命名 |
| `PocketItem +1790 + 8×slot` | `74D7F0`核四槽地址；`8378C0`在`837AC9/837AFF`与`837CE9/837D1C`按slot索引读ID／type，循环覆盖次槽1—3，主槽另读。**确实需要四槽，不能只留当前最大图标** |
| `queued +17B4`的配置、`flags +17BC` bit1 | `7C1C13/7C1C28`在GetTrinketMultiplier中检查queued配置类别2／ID并按bit1增加黄金份额。保留“queued饰品配置＋是否黄金”投影；queued collectible等显示由已产生的图像／姿态承接，不为这条查询装入非饰品借用 |
| `bag +1F20`八个byte | `835B40`的`8365E4`及`RenderCraftingTable 839A80`直接检查原有材料，后者按`Player+1F20+index`选Idle帧。保持八个位置和0空值，不排序、不只发材料总数 |
| `poop queue +1F34`六个byte | `83A840`直接按位置选择Idle／IdleSmall帧；与P2的PoopMana分开。保留六个实际位置，不在Client补队列或抽取新法术 |

`GetCollectibleCount 758E20`的总数可以从counts按已核公式取得，不再发送独立总数缓存。完整逻辑长度必须保留：dense表尾部0与表长会影响按ID的边界判断，不能为了压缩随意裁尾；容量／allocator／桶地址则完全本地。

**口袋空type不在本轮裁掉**：Wiki明说空槽type可残留，但这不是“无语义填充”的证明。`8378C0`的主槽路径先读type，后按不同分支检查ID，且`8387D8/83882C`还在种子效果29／30的显示路径读取type。本轮保留实际u32，包括空槽原值，不先验归一化成PILL。非空type只按PILL=0／CARD=1／ACTIVE_ITEM=2解释；空槽不拿残留type查配置。这里不宣称空槽type在每种最终画面都必要，而是明确该值尚无可删除证明。

Pocket的ID语义按type分派：PILL为完整PillColor及修饰位，CARD为Card ID，ACTIVE_ITEM是**ActiveSlot+1**，不是Collectible ID。后者解成slot0—3并依赖P4，不按道具资源号直接查图；实际空slot不能凭图片缓存猜成非空。非空未知类型或无法解析的实际资源时报具名Gap，不截位“修正”。

### 3.2 REP ItemSpoof：同步结果所需计数，不复制管理历史

官方 `ItemSpoofSystem.cpp` 的 HasCollectible／GetCollectibleNum／HasTrinket／GetTrinketMultiplier／HasGoldenTrinket hooks实际消费：

- blocked ID集合；
- `HasInnate(id)`／`GetInnateCount(id)`：遍历分组，读取persistent counts和temporary的**cachedTotals**；
- 角色XML innate、reworked规则及调用context；当前原版由相同配置／官方代码提供，不能靠修改普通库存来伪造。第三方变更另归[Mod后续方案](../mod-adaptation.md)。

这些原生展示查询**不读取group key、temporary条目的顺序、timer或lastPlayerType**。因此不再复制旧I14的完整管理模型。Host直接遍历已存在的官方容器，将全部分组（包括空字符串legacy组）中persistent与temporary.cachedTotals按ID有界相加，产出：

| u16 blocked_count：实际阻止集合大小 | i32[blocked_count] blocked_ids：递增、唯一 | u16 innate_count：合并后非零项数 | InnateCount[innate_count] innate：按完整ID递增；不裁黄金位 |
|---|---|---|---|

| +0 i32 id：完整collectible／trinket身份 | +4 i32 count：所有组当前有效总数 |
|---|---|

collectible／trinket各一份，类型分别保留。blocked与innate可以含同一ID，**同时发送**，由官方原查询决定优先级。饰品普通／黄金ID也不能相加后抹掉黄金位，block的规范化与innate的规范化并不相同；保留源码真实处理，包括HasTrinket分支当前使用的mask／flag表达式，不自行修补官方行为。

Host不调用会分配的GetEntityPlayerPlus、GetInnateTotalCounts、Lua getter、Save／Load或SetInnateGroup；正式reader只借用同use、正确动态类型、已存在的holder/data及容器。没有Plus来源是Gap，不等同合法空集。无序桶遍历结果排序为自有纯值后比较，原生指针／group字符串／临时项不进入快照。

投影要求官方正常非负计数不变量，合并用足够宽的临时整数检查i32边界；异常负项、溢出或不明来源报Gap，不抵消为0后假装等价。使用**当前cachedTotals**，不额外遍历到期列表重算“应该是什么”，不推进timer。来源端另对遍历的group数、persistent／cachedTotals总条目数和暂存字节作有界限制，沿既有16384聚合保护与总预算；不能只有最终去重ID数受限而遍历任意多组。预算失败不截断为部分投影。

Client在已构造的真实PlayerPlus内直接交换候选容器：blocked写本地集合，innate汇总放入一个**本地固定名**的展示组，其persistent counts为线上总数，temporary列表与缓存均为空。这保留标准容器和官方查询的不变量；**不能只写temporary.cachedTotals而留下空list**。原group名、临时／永久身份与到期逻辑不复原，也不对外宣称原group查询可用；额外Mod展示需求已归[独立后续方案](../mod-adaptation.md)，不属于本轮实现条件。

只换内容，保留官方虚表和owner。禁用ItemSpoof UpdateTemporaryTimers／CheckPlayerType／公开Block／Add／Remove／SetGroup等玩法调用；这些函数会触发EvaluateItems、costume或回调。`lastPlayerType`是触发管理缓存，不从Host传输；Client owner随已提交P7角色初始化它，不能拿CheckPlayerType刷新。Host到期后只在有效计数变化时发布P3，timer每tick递减不应导致P3重发。

这是一份覆盖所有实际ID的容器投影，不是按若干道具ID轮询的白名单，也不是重建旧556项参数查询字典。

### 3.3 明确不传的来源及证据边界

| 原生范围／内容 | 处置 |
| --- | --- |
| `+1744` voidedCollectibles顺序 | Host未来Use／Void链。直接偏移检索命中Use、保存／恢复、复制等；没有本轮采用的绘制消费者。不把它当当前被动库存或为“完整旧getter”传输 |
| QueueItemData `+4 charge`／`+C VarData`，flags除bit1 | Host获取／排队／使用管理，当前所保留倍率消费者不读取。图像／持物结果由P0／第8步准备，不重跑Queue或拾取 |
| `+1F28` crafting输出ID | 原值的已核HUD读取在`832C10 Update`，原生Render消费HUD已建立的结果／图像记录；**结果身份仍要进入第8步的实际HUD缓存**，P3不重复这份生产输入 |
| `+1F2C` crafting itemPool | 配方选择／保存来源留Host；不得调用GetBagOfCraftingOutput一类会刷新配方的链补显示，也不重选池 |
| innate group key／persistent-vs-temporary历史／临时list／timer／lastPlayerType | 管理与Host推进状态，不同步原值；当前有效结果按3.2投影，正式显示不依赖到期更新 |
| counts／map／vector头中的地址、capacity、桶链、锁、Lua引用、Plus holder地址 | 本地owner维护，不发给Client，不能以memcpy容器头“重建库存” |
| History／16个inventory-history indices | 与当前库存分开。`839370 RenderInventory`读PlayerHUD `+6A4/+6A8`的已建立显示列表；实际History及该列表由P8／第8步核定，不用voided或counts排序冒充获得顺序 |

**不能把P3当成完整原生getter实现。** 本轮直接复读`7699D0`／`769E90`／`7C19C0`确认：它们还依赖P7主备／parent／激活资格、P8 TemporaryEffects、Room／Run／Seeds配置及特殊query分支。`HasCollectible(313,…)`含原版Online／本地进度判断，部分trinket分支构造局部RNG；不能在Client无条件放行整条getter，更不能在Host采集时为查询额外执行随机或读取原版Online补值。第8步须由具体最终显示缓存／合法来源承接这些表现，未准备时报具名缺口，不复制进度、随机对象或设备状态。

**补充参数核对后留Host的三组候选**：`+1924`的特定物品数量mask、`+200C`若干特定ID效果位，以及`+18C8/+18CC`对饰品24／19的临时倍率输入。不能因为通用查询函数里有这些分支就自动上线。本轮从下述32个展示根检索5层直接调用，在query入口停止扩展，得到609个函数、20个直接query调用者；再单独复读query递归与集合查询helper的实际参数：

| 来源 | 参数与阴性结论 |
| --- | --- |
| `839A80` | GetCollectibleNum实际查询251，不进入`+1924`的98／112／113／178／270／275／278／360／363／390／417／679／698分支 |
| `62C0C0`／`64E3A0`／`7C42D0` | GetTrinketMultiplier查询141／156，递归查询21／64／75；没有19／24参数，`+18C8/+18CC`临时输入不进入这些实际路径 |
| Player／Familiar／HUD／Room Light的HasCollectible直接调用 | 参数包括619／62／75／303／46／654／713／399／643／680／206／433／442／415／392／501／558／425／212／451；getter递归另核191／139／458／439等，不进入`+200C`的305／257／231／182或数量108分支 |
| `9950B0`／`996930`集合helper的调用者 | 追到`6CEC90`、`8134C0`、`966300`、`97C300`的403／425／460／333／21／246／54／376／416，以及角色25的619查询；不把decompiler里的动态形参误当任意ID消费者 |

根为公共Render／Shadow／Post、Player Render／Shadow／Body／Head、NPC Render／Shadow、Tear、Projectile、Pickup、Familiar Render／Shadow、Effect、Knife、Bomb、Laser Render／Shadow／Post、Slot、Text、Dummy、Room Light、HUD Render、RenderActiveItem／RenderGfx／8378C0口袋绘制／RenderInventory／RenderCraftingTable／RenderSpellQueue及Scene415350；具体入口地址可交叉见[动画复核第2节](animation-resource-synchronization-review.md#2-动画事件与播放控制依据)，额外HUD入口见本文。Weapon虚分派按5.1另核，不把这次直接调用检索当成全部虚调用／任意Mod的形式化闭包。

在该已核官方展示范围内，这三组明确留Host，不在P3读取、编码或比较，也不在Client重建为“来源零”。这是有界阴性依据，**不是填充证明，也不承诺所有公开getter原值相同**。新增受支持消费者时再修订具体职责，不能在生成或接线时静默恢复字段。当前仍未闭合的是前述跨域计算型查询的最终显示来源，不是把这三组默认留到未来传输。

### 3.4 原生归属单行表（本轮范围）

| 0x1388 i32 当前488效果选择ID【状态】通用库存查询输入 | 0x1390 i32 当前191选择ID【状态】 | 0x16B0 u32[2] Trinket【状态】完整黄金位 | 0x16B8 vector&lt;i32&gt; collectibleCounts【容器】传逻辑长度／值 | 0x1738 vector&lt;i16,i16&gt; smelted【容器】普通／黄金双计数 | 0x1744 vector&lt;i32&gt; voided【权威】未来Use列表 | 0x1790 PocketItem[4]【嵌套】4×8B | 0x17B4 QueueItemData【换算】只投影实际queued饰品配置／golden | 0x1F20 u8[8] crafting材料【状态】位置保留 | 0x1F28 i32 crafting输出【权威生产输入】最终结果身份由HUD缓存承接 | 0x1F2C i32 itemPool【权威】 | 0x1F34 u8[6] poop队列【状态】 | 0x1F5C map&lt;i32,i32&gt; itemWisps【容器】 | 0x1F94 i32 ModelingClay效果ID【状态】 | 0x1FB0 vector&lt;u8&gt;附加collectible counts【容器】 | 0x1FBC vector&lt;u8&gt;附加trinket值【换算】逐ID零／非零 | 0x2010 u8库存屏蔽【换算】零／非零 | 外部PlayerItemSpoofs【换算】blocked＋有效innate计数，原生容器／虚表／lastType不复制 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

PocketItem原生及线布局一致：

| +0 i32 id【状态】按type解释，0为空 | +4 u32 type【状态】非空0／1／2，空槽保留真实旧值 |
|---|---|

QueueItemData原生0x10的完整归属：

| +0 ItemConfig* item【条件资源】只有类别2饰品配置投影为资源号，其它类别的图像归最终缓存；地址本地 | +4 i32 charge【权威】 | +8 u32 flags【换算／权威】只投影饰品golden bit1，其余原值不上线 | +C i32 VarData【权威】 |
|---|---|---|---|

Smelted元素0x4的原生及线布局：

| +0 i16 normal_count【状态】 | +2 i16 golden_count【状态】 |
|---|---|

ItemSpoof外部类型暂按官方成员声明定位，由实际Release ABI的生成布局逐成员展开：vtable／container header本地，blocks为容器，persistent与temporary.cachedTotals计数为换算来源，group key／temporary list／lastType为Host管理。本文不猜MSVC桶结构的字节数或PlayerPlus余部大小，不用“已投影四张表”宣称整个PlayerPlus已闭合。

### 3.5 P3草稿线布局

固定前缀70B，后接变长区；没有native对齐，逐项memcpy解码。`R`是配置资源号，`FFFF`为Null；queued非饰品或真正无queued均输出Null／golden=0，不读取无效payload。bit1不是完整flags的另一份副本。

| +0x00 u32[2] trinkets | +0x08 PocketItem[4] pockets：各8B | +0x28 u16 queued_trinket_config：R或FFFF | +0x2A u8 queued_trinket_golden：0／1 | +0x2B u8[8] crafting_content | +0x33 u8[6] poop_queue | +0x39 u8 inventory_suppressed：0／1 | +0x3A i32 effect488_collectible | +0x3E i32 effect191_collectible | +0x42 i32 modeling_clay_collectible | +0x46 下表变长区 |
|---|---|---|---|---|---|---|---|---|---|---|

| u16 C＋i32[C] collectible_counts：按ID索引 | u16 S＋Smelted[S] smelted：按ID索引 | u16 W＋(i32 id,i32 count)[W] wisps：按完整ID排序 | u16 B＋u8[B] extra_collectible_counts：按ID索引 | u16 T＋u8[T] extra_trinket_presence：逐值0／1 | ItemSpoofProjection collectible_spoof：见3.2 | ItemSpoofProjection trinket_spoof：见3.2 |
|---|---|---|---|---|---|---|

格式下界为88B；总长为 `88+4C+4S+8W+B+T+4Xc+8Yc+4Xt+8Yt`，X为blocked项数、Y为innate项数。下界不是实际空库存尺寸或实测带宽。各container沿既有16384保护上限并受world32MiB／整体内存预算限制，dense表长度还须满足本地配置索引要求；不裁尾、不截断、不按map桶顺序编码。

## 4. P4：四个ActiveItemDesc，96B

沿版本39六值结论，补齐整个四槽的空值、范围与安装规则。Player原生起点`1574`，步长`20`，共四份；原生128B中**occupied槽最多96B有同步职责**。通用布局12的body元数据在这里由P4 world承载。

### 4.1 原生归属与证据

| +0 i32 Item【状态】835BCD配置／图像选择 | +4 i32 itemPool【权威】759864来源池，非显示输入 | +8 i32 Charge【状态】835C10 | +C i32 BatteryCharge【状态】835C1A—835C23的等价索引 | +10 i32 SubCharge【状态】835C3E | +14 i32 TimedRechargeCooldown【权威】789E50更新调度 | +18 f32 PartialCharge【状态】835C76 movss，不是SDK旧int | +1C i32 VarData【状态】835CCD／836036／8360F1／836179／8361B8及7897C0／789810 |
|---|---|---|---|---|---|---|---|

VarData既影响max／min可用充能，也参与特殊图像，不能按“玩法附加数据”统一删掉。Item／配置、六值、P2 Jar／Urn／魂血资源、P3材料／库存、P8效果与Run challenge共同影响显示。HUD图像、bookImage、flash和layout缓存仍由第8步准备；六值不是整个HUD的替代品。

### 4.2 单槽和四槽的草稿线布局

| +0x00 i32 item_id | +0x04 i32 charge | +0x08 i32 battery_charge | +0x0C i32 sub_charge | +0x10 f32 partial_charge | +0x14 i32 var_data |
|---|---|---|---|---|---|

| +0x00 ActiveDisplay slot0：24B | +0x18 ActiveDisplay slot1：24B | +0x30 ActiveDisplay slot2：24B | +0x48 ActiveDisplay slot3：24B |
|---|---|---|---|

- occupied槽三段读取：原生`+0..3`、`+8..13`、`+18..1F`，共4＋12＋8=24B；四槽最多12段，不吞入来源池或调度冷却。
- Item=0时，835B40不进入该Item的配置／充能绘制。**先读Item，再把其余五值编码为0，不读旧payload**；Client解码拒绝非规范空槽，直接清本地五个展示存储。原生来源池／调度字段由本地正常初始化保留，不写Host值。空slot是明确状态，不能与采集失败混同。
- occupied的PartialCharge只要求有限，整数按实际i32；没有证据将所有道具charge钳到6、partial钳到0—1或VarData当u8。合法Item须解析实际配置，不以原版枚举上限屏蔽已声明资源。
- 四槽作为一个world目标提交，Pocket的ActiveSlot+1与P4一致。Esau Jr 703的VarData=0／1—4保留原数值；P6—P8进一步核对已裁掉P7四备份槽的下行依赖，实际主动显示不解析该Host存储。不给它套地址解释，不调用703使用链，也不写Host PlayerManager备份数组。
- 只直接装六值；不调用AddCollectible／Use／AddCharge／FullCharge／ControlActiveItem／EvaluateItems。官方1072／1073等第三方回调结果适配已迁至[Mod后续方案](../mod-adaptation.md)，不作为本轮原版实现条件；Client不执行玩法Lua补值，也不放行完整832C10 PlayerHUD Update。

## 5. P5：五个Weapon槽，按真实动态类型裁剪

### 5.1 实际消费链与进一步裁剪

本轮复读`758850`：充能条只扫描槽**1—4**，各自取首个slot7／slot8正比例，非求和；HUD `83606F—836093`与Familiar `60C190`则从槽0再槽1选首个非空Weapon，分别判断type12／15。因此五槽的存在性和顺序都需要，不能只传当前一条chargebar。

逐15类型核vtable：除12／15的slot7直接为`9C4E60`外，均为`9C5E80`；全部slot8均为`9C4E60`，返回0。`9C5E80`先检查Modifiers bit80，再调用slot6；**slot6≤0立即返回0，尚未读取Charge**。5／6／8的slot6也恒零，故这五种类型没有三项充能输入的展示职责。

其余十种类型按已核slot6公式使用MaxFireDelay／Charge及部分Modifiers位。Modifiers的其它攻击位既不影响这些公式也不参与其它已核Weapon展示，因此投影为下表mask，未使用的位改变不应使P5变化；Host不改原字段，Client只安装相应已知位。不是按当前charge或modifier值临时改payload形状。

| type | 动态类 | factory VA／分配大小 | vtable VA | slot6 VA | 线上Modifiers mask | 单槽值负载（不含layout标记） |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | Tears | 9CAFA0／50 | B31EA4 | 9DF230 | 183 | 16B |
| 2 | Brimstone | 9CB2C0／4C | B31DB4 | 9D2A80 | 91 | 16B |
| 3 | Laser | 9CB180／50 | B31AAC | 9D82D0 | 8B | 16B |
| 4 | Knife | 9CB1E0／54 | B31D64 | 9D6A80 | 90 | 16B |
| 5 | Bombs | 9CB320／48 | B31A64 | 9C4E60常零 | 不传 | 4B |
| 6 | Rockets | 9CB060／48 | B31E5C | 9C4E60常零 | 不传 | 4B |
| 7 | MonstrosLung | 9CB0C0／44 | B31DFC | 9DA650 | 81 | 16B |
| 8 | Ludovico | 9CB120／48 | B31A1C | 9C4E60常零 | 不传 | 4B |
| 9 | TechX | 9CAF40／44 | B31CD4 | 9DFBA0 | 80 | 16B |
| 10／11 | Bone／NotchedAxe | 9CB380／60，共用动态类 | B31D1C | 9D0A50 | 88F | 各16B，type值仍区分 |
| 12 | UrnOfSouls | 9CAE70／54 | B31B6C | 9C4E60常零 | 不传 | 4B |
| 13 | SpiritSword | 9CB000／50 | B31EEC | 9DC070 | 80 | 16B |
| 14 | Fetus | 9CB260／44 | B31C1C | 9D59F0 | 80 | 16B |
| 15 | UmbilicalWhip | 9CAEE0／48 | B31BC4 | 9C4E60常零 | 不传 | 6B：type＋主实体引用 |

本表地址、factory分配大小、mask均为**十六进制**，type与线上负载B数为十进制。slot6公式保留原f32落栈、阈值与除法顺序；例如9C5E80检查`charge<=1 && charge<max`，不是一个跨武器通用ratio。12／15原本就不进它，5／6／8在读取Charge前返回；不传三值不会改变其零值与真实Disappear动画。

普通所有非空槽仍传真实i32 WeaponType，不能由vtable反推10／11，也不能删掉“非充能”槽而改变首个非空选择。type16是枚举计数，不是可构造武器；未知工厂／vtable报实际Gap，不回落base Weapon或伪装成Tears；第三方Mod武器已归[后续适配方案](../mod-adaptation.md)，不要求本轮实现。Familiar80仍用自己的最终amount，不恢复其玩法Weapon。

### 5.2 基类前缀的单行归属表

0x44是实际base ctor足迹，不是所有动态分配大小。这里按本轮消费分型，当前版本46通用布局13／135尚未修改，旧表20B UNKNOWN与16B负载事实保留其原时点；未来Player专用布局／投影按本文落地。

| +0 vtable【本地】由实际factory建立 | +4 Entity* owner【本地／权威】本地承载Player的生命周期关系，不传Host边 | +8 EntityPtr common_link【权威】本轮9C5050证实是会被Remove／注销的真实链接，不是所有类型的GetMainEntity | +C f32 FireDelay【权威】模拟冷却 | +10 f32 MaxFireDelay【条件状态】只对十种充能类型 | +14 f32 Charge【条件状态】同前 | +18—1F 8B【未知】保留 | +20 Vector Direction【权威】瞄准／发射输入 | +28—2F f32[2] axis【权威】IsAxisAligned上下文，不供本轮展示 | +30 i32 WeaponType【状态】五槽身份／分支 | +34 u32 Modifiers【条件换算】只发对应公式mask位，其余不上线 | +38 i32 NumFired【权威】发射历史 | +3C—43 8B【未知】保留 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

`+8`的生命周期证据不只来自旧writer：`9C5050`、`9D4240`、`9D0990`、`9E0D50`均直接通过该Entity虚Remove并在其引用链注销本槽。它具有玩法链接语义，不能再猜成无意义字节或统一MainEntity；本轮展示链不读取它。基类尚有16B明确UNKNOWN，不因ctor写0／1就改成padding。

### 5.3 各动态尾部的单行归属表

每行都以0—43的上表作为完整前缀，尾部覆盖到本轮factory分配大小；未解释的尾部不能因为所有当前展示公式只读base就冒称已核为AI字段。

**Tears，0x50字节**

| 00—43 基类，见5.2 | 44—4F 12B【未知】 |
|---|---|

**Brimstone，0x4C字节**

| 00—43 基类 | 44—47 4B【未知】 | 48 EntityPtr【权威】攻击主链接 |
|---|---|---|

**Laser／SpiritSword，各0x50字节**

| 00—43 基类 | 44 EntityPtr【权威】攻击主链接 | 48—4F 8B【未知】 |
|---|---|---|

**Knife，0x54字节**

| 00—43 基类 | 44 EntityPtr【权威】 | 48 EntityPtr【权威】 | 4C EntityPtr【权威】 | 50 EntityPtr【权威】 |
|---|---|---|---|---|

**Bombs／Rockets／Ludovico，各0x48字节**

| 00—43 基类，三项充能值不上线 | 44 EntityPtr【权威】攻击链接 |
|---|---|

**MonstrosLung／TechX／Fetus，各0x44字节**

| 00—43 基类，无尾部；不是依SDK40B截掉最后4B |
|---|

**Bone／NotchedAxe，各0x60字节，共用native类但不同type**

| 00—43 基类 | 44 EntityPtr【权威】 | 48 EntityPtr【权威】 | 4C EntityPtr【权威】 | 50 EntityPtr【权威】 | 54 EntityPtr【权威】 | 58 EntityPtr【权威】 | 5C—5F 4B【未知】 |
|---|---|---|---|---|---|---|---|

**UrnOfSouls，0x54字节**

| 00—43 基类，三项充能值不上线 | 44—4B 8B【未知】 | 4C EntityPtr【权威】 | 50 EntityPtr【权威】 |
|---|---|---|---|

**UmbilicalWhip，0x48字节**

| 00—43 基类，三项充能值不上线 | 44 EntityPtr【引用】真实展示主实体，转成员序号 |
|---|---|

Whip的`60C190 → slot10 9C4E80`有Familiar Render／Shadow的直接消费者，仍依赖PlayerType32、库存619、D74 follower和D76抑制字节，不能随其他攻击关系删除。slot10返回别的类型目标时也保留其真实身份，不能把“helper只消费Type3”升级为所有非空目标必须是Familiar。先有对象与资源再通过真实EntityPtr owner绑定，独立实体由自身owner退休，不从鞭级联Remove。

按这些已核前缀与尾部，动态对象未知总数分别为：Tears28B、Brimstone20B、Laser／SpiritSword24B、Knife16B、Bombs／Rockets／Ludovico16B、MonstrosLung／TechX／Fetus16B、Bone／NotchedAxe20B、Urn24B、Whip16B。它们是本轮静态账本，不覆盖未声明Mod动态类型，也不使这些类型标完整。

### 5.4 P5草稿线格式

| +0 u8 present_mask：低5位依次槽0—4，其余位必须0 | 顺序变长：按槽从小到大，为每个置位槽写WeaponView；空槽不带任何旧payload |
|---|---|

| +0 u16 view_layout：生成时分配的具体展示布局标记 | +2 对应下表值：先父类身份再子类表现，不发送vtable／factory函数地址 |
|---|---|

共同身份（4B）：

| +0 i32 weapon_type：1—15，必须与view_layout的允许类型一致 |
|---|

十种充能类型在身份之后的12B：

| +4 f32 max_fire_delay | +8 f32 charge | +C u32 display_modifiers：只含该类型mask位 |
|---|---|---|

5／6／8／12只有共同4B身份；15在身份后追加：

| +4 u16 main_entity：真实成员序号或FFFF，0不是Null |
|---|

view_layout对应固定字段集，不按报文提供的大小、offset或函数选择writer。type10／11允许同一个Bone展示布局，但共同身份保留区别。线布局编号待生成时分配，不直接复用仍发16B的通用13／135假装实现裁剪。

因此P5最短1B（五槽空），最长91B（五个充能类型，`1+5×(2+16)`）；任意组合按mask和实际view长度精确相加。Whip单槽含标记8B，恒零其它类型单槽含标记6B。这里只给格式界限，不声称所有组合自然可达或带宽已通过。

Host按5个槽读取，类型核定先于字段／引用读取；充能类型只读`+10/+14`并读取`+34`后掩码，恒零类型完全跳过这些值。Whip才读`+44`。不读取UNKNOWN、派生攻击链接或调用GetMaxCharge／GetChargeBarAmount额外生成结果。去掉“为对照而传MaxCharge、axisAligned及两份chargebar”旧要求。

### 5.5 身份、构造、替换与退役

- Weapon*不是网络身份。旧采集器的跨槽unique指针断言不作为新同步前提：若Host合法槽共享同一对象，本设计传各槽的相同纯值及真实Whip引用，Client可由每槽独立owner承载。只读展示、停止更新时不需要复制Host别名拓扑；不让一个Client指针被两个槽重复Free。未知动态类型／来源变化仍报Gap。
- 只通过当前真实注册factory／Init构造本地对象，核对表中实际分配类／vtable／type；不自行注册或按0x44分配所有对象。slot0也不能借slot1对象充数；10变11即使vtable相同，也须按新type建立合法本地实例／初始化合同。
- factory建立的本地owner是承载Player，Host owner边不上线；后续受支持查询如果新增owner依赖，先修订读集，不能偷偷恢复Host关系。
- 只写该view保留的字段。恒零三项及非展示Modifiers位保持正常本地初始化；不将Host未知／排除值补成“来源零”。接触已初始化bit字段时只更新已知mask位，不覆盖未知位；不运行Fire、Update、Reset或玩法关联创建以弥补。
- 类型变化／空槽／退休：先退出所有CPU绘制借用，正常注销本地EntityPtr，再owner=null，调用已核真实动态Free，最后清Player槽。即使Host不传某条攻击引用，释放前仍要按该动态类完整链接清单撤本地边；不memcpy清零链接。
- 不调用会Remove攻击实体的ClearReferences／DestroyWeapon；Urn `9E0D50`还会停22／23号音效，不能当同步资源清理。异常构造／析构前缀或外部改槽只保留实际owner并停止本scope，不自动重新创建或重试Free。

## 6. 统一采集、安装及变化合同

1. 三块都在完整ordinary窗口从已具备本字段资格的每身体backing复制；不使用会转向parent／twin的公开getter替代它的原存储，也不向备份身体借另一身体的初始化证言。
2. Source只读真实范围及逻辑元素，先形成自有不可变缓冲。容器根、内容所属use及必要资源前后稳定；网络／Rust不持有原生对象、STL头或Plus地址。未初始化、未知身份、来源变化、溢出或预算失败均有具名Gap，不能当空集发送。
3. P3容器／投影、P4四描述、P5五槽分别按物理块比较／版本发布。排序消除无序桶变化；innate timer、无关modifier位、空主动槽旧payload不应引发变化。不得在Host写回这种规范化结果。
4. Client先准备配置／图片、正常STL候选及Weapon实例；再安装P3／P4／P5、P2资源、P7关系和P8效果及第8步实际HUD缓存。在同目标成功提交前不开放新显示视图或回复AppliedAck。
5. P0的P6绘制依赖中，装备类型／槽存在性、P3有效查询结果或P4特殊ID／VarData若改变绘制结构／资源选择，也必须随ordinary新拓扑提交；普通充能值变化不人为提升shape，也不把整个world版本绑定到每个body。
6. 真实Empty／Null要清旧目标；Missing／Unavailable／类型不明／依赖未就绪不可伪装为Empty。P3／P4／P5原值及最终HUD结果的漂移比较按实际上线职责，不继续要求Host历史／被排除字段相等。

## 7. 失败方式与后续工作

### 必须识别的失败方式

- 把counts当完整有效库存，忽略四主动槽、wisp、附加值、block／innate或主备关系；反过来为了保留所有getter复制无消费者的全部历史。
- 把voided序列、History或HUD排序表当库存；丢材料位置、poop位置或口袋后三槽；将Pocket active的slot+1当Collectible ID。
- 空口袋type随意改零，或空主动槽反而读取无定义旧partial并因此拒绝有效空槽；把两种空值策略混同。
- innate临时timer每tick导致P3变化、分组key变化但总量相同仍发包、漏legacy组、抹掉黄金位、丢blocked∩innate交集，或合并溢出；只填cachedTotals而制造不合法本地容器。
- 为查询额外调用RNG／进度／原版Online，或直接放行含此分支的整条getter；未证明必要的特殊mask在实现时悄悄上线。
- Active PartialCharge按int，吞入来源池／调度冷却，VarData截位，或用Add／Use／Charge刷新HUD产生玩法副作用。
- 五槽全部按16B发，恒零类读取旧充能；Modifiers不先mask就memcmp；将10／11合并、忽略slot0或破坏先非空选择。
- 所有Weapon按0x44分配／复制，尾部当padding，鞭主引用漏传或所有其它MainEntity重新上线；源码地址／vtable进入协议。
- 直接共享Client Weapon指针导致重复Free，ClearReferences／DestroyWeapon级联Remove独立Entity或停音，或生命周期异常后盲目重试。
- 跨P0／P6／P7或HUD资源版本安装，部分提交／提前ACK；把静态字节下界当实机性能结果。

### 交付与未决

本轮完成P3容器／投影与空值职责、P4固定96B及三段读取、P5真实factory尺寸／分型负载／位mask／引用／本地owner合同，以及原生范围、草稿单行线表和失败清单。更新第5步入口及第8—15步相关合同；没有改运行代码或当前版本46生成表。

明确未决继续保留：跨域getter的无随机／无Online最终显示来源；PlayerPlus未展开的实际ABI字节；Weapon已列UNKNOWN；P6—P8、配置／HUD全部owner与全Player字节表。三组特殊候选已经在上述有界读集中决定留Host，不继续当必传候选。其余未决不由旧测试结果、旧完整查询字典或本轮投影自动闭合。

第5步后续迁入唯一`state/player.json`及专用嵌套布局后才分配layout编号、提高整数版本、生成复制／编解码／回映射与两种单行表。第10／12／13／14步实现新reader／decoder／writer并退役旧原值要求；第15步才连续执行E2E，覆盖完整库存／特殊查询、四口袋／四主动／五Weapon／多身体、资源／身份／恢复／非法负载／隔离／退役及既定性能预算，不另开分项测试。
