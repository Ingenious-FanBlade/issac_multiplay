# 第8步 Random：原生字节归属与同步职责核对

2026-11-10；目标为官方 REPENTOGON 1.1.3／Repentance+ v1.9.7.12.J273 Win32。本轮完成随机域的静态核对、原生字节账本和旧54行目录回映射。第7步交接修订2另见[交接补证](room-random-handoff.md)：D0现5B、随机绘制发逐用途结果，新核音效选样本流留Host；旧13B决定已替代。它不是第8步整体完成，也不是正式reader／writer／Hook接线完成。设计数据为[字节账本JSON](random-byte-ledger.json)／[单行表](random-byte-ledger.md)、[成员JSON](random-member-map.json)／[逐行表](random-member-map.md)。不修改生产布局版本56，不构建，不运行游戏、测试或随机调用。

## 1. 资料顺序与结论依据

先读本地原版Wiki的 `RNG.md`、`ItemPool.md`、`Seeds.md`，再读本地REPENTOGON Wiki的 `RNG.md`、`ItemPool.md`、`WeightedOutcomePicker.md`、`LootList.md`、`LootListEntry.md`。根目录分别是 `F:/projects_isaac/IsaacDocs/docs/`、`F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/`。Wiki给出接口含义；官方 `libzhl/functions/{RNG,MTRNG,ItemPool,ItemPool_Item,ItemPool_TrinketPoolItem,BossPool,BossPool_Pool,BossPool_Entry,GameStateItemPool,WeightedOutcomePicker,WeightedOutcomePicker_Outcome,LootListEntry}.zhl` 给出声明。随后读 `repentogon/LuaInterfaces/{LuaRNG,LuaItemPool,LuaWeightedOutcomePicker,LuaLootList,LuaLootListEntry,LuaLevel}.cpp`、`LuaInit.cpp` 和 `Patches/ItemPoolManager.{h,cpp}`。仅不能由这些资料回答的实际偏移、共享关系和副作用才查J273 IDB。

在线官方[RNG](https://repentogon.com/RNG.html)／[ItemPool](https://repentogon.com/ItemPool.html)只作语义旁证，不能拿现行网站新增接口或另一个版本替代本地1.1.3 ABI。本文原生地址均为VA，RVA=VA−0x400000。只读IDB，没有修改、补丁、保存或执行目标函数。`已核`、`SDK声明`、`模型`、`未知`保持各自证据等级；登记每个字节不表示每个字节语义已知。

## 2. 最终同步决定

| 范围 | 需要同步 | 不需要或不应该同步 |
|---|---|---|
| 生成、AI、战斗、掉落、重掷、道具池、Boss池、Player物品流 | 已经产生的实体／Grid／房间、资源、库存、实际显示及事件，由各自域承接 | RNG seed、shift、MT状态、游标、池权重／黑名单／移除／Genesis额度；Client不再作选择，不能据seed恢复模拟 |
| 两份具名纯绘制流 | 正常Host已产生的Pickup／Familiar逐用途alpha与资格，见第4节；D0仅5B时钟／暂停 | 两seed、三个shift、初始化guard、TLS／atexit及全局顺序重放；原生物理实例保留Host |
| 药丸颜色、效果和鉴定 | 真实身体／Pocket槽已经裁决的显示资格、最终effect／名称／图像结果；未知仍须显示未知，马药外观保留 | 不将ItemPool的15项基础映射、15鉴定位和全Player玩法依赖复制给Client重跑GetPillEffect；禁止把所有颜色提前揭示 |
| Loot／加权选择 | 实际生成结果；Guppy's Eye等预览须同步真实Host已经决定的有序显示结果 | Loot RNG*、预生成seed、权重、容器头、为采集调用GetLootList(false)／PickOutcome；false或不减少池不代表无副作用 |
| 音效、地图、分数、程序道具／资源 | 已选sample事件／持续voice状态、挑战32已选锚点／对应描述及窗口／标记、最终震动、最终分数、实际程序资源 | 选sample的随机源、Minimap::Update、CalculateScore、资源生成／玩法回调重放；Render可达不等于纯显示 |
| CRT、全局Random、REPENTOGON RandomFloat、Lua math.random | 正式状态帧中无随机引擎负载；声明Mod需要的最终状态按其adapter schema | 不复制／播种另一个进程或线程的引擎、Lua closure、PTD；不能用一个总seed表示四种不同引擎 |
| 开局seed等非游标值 | Run开局种子显示元数据；Grid Light实际使用的SpawnSeed等按既有具体消费者保留 | 不因名字含seed一律裁掉，也不把InitSeed／SpawnSeed当作实体身份或玩法恢复许可 |

“不发随机流”不表示“随机产生的画面不发”。已有字段或具体显示批次负责结果，不能删掉结果后声称Client能自然重建。相反，没有显示消费者的池列表、GetLastPool、可用数量、移除和黑名单只属于Host或诊断，不为了旧目录原值相等进入Baseline。没有单独的Random网络记录、池空记录或随机调用日志下行。

## 3. 基础RNG、MTRNG和全局源

### 3.1 RNG的16字节

`+0 seed u32，+4/+8/+C三个shift u32`，无指针、vtable、独立cursor或对象身份。J273 `7DF4A0 SetSeed`写四字；`7DF4E0 RandomInt`、`7DF540 RandomFloat`、`7DF5B0 Next`读取三个shift并覆盖seed。**RandomInt(0)也先推进seed，再返回0**。不得以返回常量推断零消费。

本地Wiki说RandomFloat／RandomInt会自动Next；原生函数实际上内联相同推进，因此只挂Next不能证明捕获了全部消费。复制、Restore及内联变化只能称状态变化，不能由前后seed猜调用次数、参数或事件。seed相同不意味着同一个流；身份来自owner/use/用途，栈地址复用也不继承身份。

官方 `LuaRNG.cpp` 的区别必须保留：

- `PhantomNext`先对原对象调用Next，随后恢复seed；正常返回后不变，不等于执行期间未写入，也不是正式采集Getter。
- `PhantomInt/Float`在16B局部副本上计算；`PhantomPrevious`纯计算逆变换；`Previous`则直接改seed。可逆性不使它们成为Client回滚接口。
- `RandomVector/PhantomVector`调用 `Isaac::RandomUnitVector(seed)`，前者之后额外推进原对象；不能混为普通的两次浮点抽取，也不以参数为整数就认定helper纯读。
- `GetShiftIdx`搜索81组三元组，找不到返回nil；不能把三个u32无条件压成任意一个默认索引。J273表中index35为 `(5,9,7)`，**不是**第4节绘制流的 `(4,3,27)`。
- Wiki、Lua验证与原生SetSeed是不同层。源码SetSeed拒绝零，constructor也拒绝零；原生推进遇零记录错误并可能int3，原生SetSeed还钳制索引。不能为了验证来源调用它们或补一个“合法seed”。

基础生产布局8的AuthorityOnly保持；两个绘制实例在本账本单列物理视图，当前游标也不上线，实际结果另承接，不修改通用RNG布局。

### 3.2 对象式MT与进程全局MT不同

对象式 `MTRNG=0x9C4`：`+0 u32[624]`、`+9C0 u32 currentIndex`。J273 `8DA340 Init`填624字；`8DA390 Next`执行twist／tempering，index625为未初始化分支，624为下一次twist边界。全部归Host，不是16B RNG。

全局 `Random()` 是 `6EB4B0`；`6EB470`的ECX是整数seed，**不是对象指针**。words位于 `BE8720..BE90DF`，index位于 `BAB814`，模式字节在 `BE72FF`，三段不是一个连续对象，不能把地址间隙一并memcpy。模式字节非零走CRT rand；否则用624字MT。此函数还分配并维护原生近期随机调用诊断链 `BE73C8`／计数，捕获stack trace；不能为补一次界面值在Client调用它，更不能同步诊断链或修改CRT模式字节。

官方 `LuaInit.cpp:327—334` 的全局 `RandomFloat()` 是另一个 `std::mt19937 gen(rd())` 配 `uniform_real_distribution<float>(-1,1)`，不等于 `RNG:RandomFloat()` 的0—1流。现有诊断reader识别的MSVC双缓冲5000B、CRT主线程PTD中4B、Lua math closure的4×u64 payload分别登记；它们的完整外层ABI、动态地址和跨版本契约未由本轮核定，不能把旧运行通过升级为固定官方DLL偏移。全部不发，记录缺口不影响这些源的Host职责结论。

## 4. 两份绘制流：同步实际结果，游标留Host

2026-11-10第7步交接修订，以[原生补证与正式交接](room-random-handoff.md#3-d0以具体结果解除全局游标重放依赖)为准。两条流确有显示消费者，但当前正式设计不再发送seed；旧13B D0方案已由5B时钟／暂停上下文和逐用途随机结果替代，生产版本仍56。

| 实例 | 正常生产点／具体结果 | 当前决定 |
|---|---|---|
| BD0CA4..BD0CB3 | 6E37CC推进，6E3805写Pickup+684；该位置是alt ANM2+5D0的ColorMod.alpha；分支还受exists／visible／dropDelay、variant100、+594、+6D9和Room+12BC!=5等限制 | 正常产生后复制alpha f32及本次资格；Client具名adapter绕过随机和资格查询，不采旧Sprite残留 |
| BD0674..BD0683 | 61E5A3推进，61E604写临时ColorMod.alpha，61E60D推颜色栈，函数末pop；真实owner谓词7D1C90决定资格 | 正常产生点即时复制f32及资格；Client建立本地中性ColorMod＋alpha并对称pop，普通后态对象不能提供此结果 |

原生两流首次合法初始化后shift均为(4,3,27)，guard分别BEE364／BED310，本地初始化owner不复制。index35实际为(5,9,7)，不能替代这些原生常量。原生16B物理布局不变，字段表保留；不以通用RNG授权Client生产，也不为正式下行强制触发初始化。

同Z成员的插入／后续排序、不同可见分支、Room+12BC的pass scratch和递归都会影响全局流分配。原413D00按GetRenderZ插入，现有Client FullSort按Z／authority_index排序；旧ClientEntityFrameScope仅借用同一普通窗口的流，未证明实际消费序列相同。因此不再保留以全局seed重演为正式路线，也不按种子相等宣称画面一致。

当前D0单行布局：

| +0 u32 manager_frame（4B） | +4 u8 paused（1B） |
|---|---|

共5B，GameFrame仍在world帧头。另有逐subject/use/shape/pass/调用身份的随机结果：`state:u8`，0为正常Host已确认无随机覆盖；1随后有`alpha:f32`，最大5B标量payload。外层身份／批次／容器头另计，不是每个实体永久只存一个alpha。已知不使用、尚未产生和读失败分别处理；后两者为来源缺口，不填0或不适用。

结果绑定真实world state revision、presentation revision和资源版本；普通采集窗口的seed或GameFrame本身不能替代这项来源关系。只捕获正常Host生产，不额外Render／抽取／Phantom；Host正常随机推进保持，Client受控draw不推进这两流。重复展示复用不可变结果，换房／use退休撤销，缺源不ACK最新不完整状态，Authority tick不等待显示批次。源缓存、窄分支adapter、CPU消费与颜色栈恢复仍待10/12/13步实现。

历史草稿：最初37B D0复制两份16B RNG，首轮静态核对收紧为13B（manager_frame4、paused1、两seed各4），当时明确相位／序列尚未闭合。本次据具体消费者选择最终结果，13B原表保留在random-byte-ledger.json的wire_history，既有运行事实和旧路径限制不改判。

## 5. ItemPool、BossPool与保存体

原生 `ItemPool=0x830` 与31个 `ItemPool_Item=0x34`、`PoolItem=0x18`、`TrinketPoolItem=8`，以及 `BossPool=0x8CC`、37个 `BossPool_Pool=0x3C`、`BossPool_Entry=0x14` 都有单独字节表。外部vector元素也登记；指针／容量／宿主allocator不进入线格式。`ItemPool._pools[31]`只是原生内嵌数组，不代表官方全部池数量上限。

核到两处官方声明不充分或错误，不能照抄：

1. `ItemPool+0`是开局pool seed；`+664..673`是**独立饰品RNG**，并非普通未知孔。`72F4E0 Init`先用+654的流推进，再把结果写+664，后者shift=`(1,5,19)`；`72E260 GetTrinket`实际取 `this+409 DWORD=+664`。+654的shift在Init／Restore为 `(1,9,29)`。两条流均归Host。旧 `random_owners.cpp`只显式枚举+654及31池双流，未具名枚举+664，不能拿旧枚举“Complete”声称覆盖此字段。
2. `GameStateItemPool=0x2B4` 的SDK把 `+218`写为 `vector_TrinketPoolItem`，但J273 `730F10 StoreGameState`逐饰品写bit、`730C70 RestoreGameState`逐bit恢复，**实际是16B vector<bool>保存inPool位**。不能按8B饰品元素遍历或照SDK复制12B头。+200/+204保存对应ItemPool+674/+678的两项值，其具体用途仍未知，不因Init写1.0或存取存在就改称填充／已知显示参数。

ItemPool根保留UNKNOWN：`+674..67B`8B具体语义、`+69C..793`248B、`+7EF`1B、`+82E..82F`2B，共259B；保存体保留11B；Trinket元素保留+6..7两字节。没有以“不发送”消除这些未知。PoolItem显式`_pad[2]`只保留SDK填充声明等级。未逐字节展开REPENTOGON `ItemPoolManager`／CustomItemPool的编译器ABI；源码已明确其动态池、独立m_RNG、虚拟／临时items、三保存槽、Genesis额度及磁盘保存均属于Host生产／恢复，不给Client建镜像manager或借其Save／Restore补UI。

`GetCollectible(Decrease=false)`仍是抽选，默认参数还可能消耗全局随机；`PickCollectible`实际推进传入RNG，未传时 `LuaItemPool.cpp`调用全局生成器构造临时流。`GetTrinket(true)`普通成功路径使用副本且不移除，但无可用项时先调用 `7311E0`重置，之后还有回调；不能把DontAdvanceRNG当全函数纯读合同。官方 `LuaWeightedOutcomePicker_PickOutcome`在原生选择后显式 `rng->Next()`。`Lua_LootListPushEntry`把 `Random()`作为 `luaL_optinteger`的C++实参，调用表达式本身就先求值，即使Lua提供了seed也不能据签名认定零消费。

## 6. 药丸显示必须保留结果，不能暴露或重演玩法

SDK `ItemPool+7A4`为15个i32基础effect，`+7E0`为15个鉴定位。`72F030 GetPillEffect`依据颜色、challenge42、具体Player的PHD／FalsePHD／LuckyFoot／Virgo等效果作映射，末尾还可进入 `84C4C0` 的Lua回调65。因此原始映射加鉴定位不足以代表真实身体的最终结果，也不能在Client重放。

`8378C0`的真实口袋Render在 `83830A`读 `Game+1AF20+clamped_color`（即ItemPool+7E0），通过后 `838320`调用上述身体相关解析，随后选实际配置和显示；没有鉴定则走未知显示分支。`832630 IsPillIdentified`本身只是color&7FF后钳0—14再读对应字节，不等于一次完整显示决定。

后续Presentation必须按 **真实subject/use/shape、Pocket槽、当前color和马药外观、显示版本**提供“是否揭示／已经选定的effect／实际名称图像”等具名结果；现有Player P3库存身份和独立Pocket Sprite仍保留。没有来源时不可把未知效果泄露、默认false／0或恢复全部Player玩法依赖。只观察原来的Host显示／更新已经产生的结果，不额外调用Getter、鉴定、抽药或提前枚举所有颜色。普通／马药可共享基础颜色的鉴定输入，但不同外观和实际显示结果不合并。完整最终批次尚待Presentation实现，不把本轮核对称为已实现。

`LootListEntry+10 RNG*`是可空非拥有指针；官方Getter直接返回它，未建立独立owner/lifetime。Guppy's Eye预览属于显示，不能因为Loot用于掉落就一起裁掉；同步Host实际可见的有序预览／图像，保持已有Pickup preview合同，不发送或解引用未知RNG*。`GetLootList(false)`可能分配、回调或生成预览，不是正式reader的补值入口。

## 7. owner、生命周期与跨域边界

[字节账本](random-byte-ledger.md)附具名owner视图：Game generation +182D4、Seeds+8、PlayerManager+B8、Ambush两条流（+2C及+3C）、31池双流、主池与饰品流、37个Boss池、Backdrop+4F28、Entity+3DC、Player四个vector及Grid+2C；LevelGenerator临时流和具名原生保存seed亦列入。

- `LuaLevel.cpp::Lua_LevelGetGenerationRNG`返回Game+182D4，不能再给“Level流”和“Game流”建立两个网络身份。参数／用途可以不同，底层字节owner只能一个。
- 原版`GetDevilAngelRoomRNG`绑定在`853542/878DB8`指向`74D6F0`，该函数返回`this+182E4`；`743970 InitializeDevilAngelRoom`实际读取四字并推进。它是Game／Level前缀里另一个已存在的16B生成流，SDK具名成员列表漏写，不能与+182D4合并或当作未知填充；四字均归Host，第7步完整前缀账本须据此登记。
- Player四个已有vector位于+1760/+176C/+1778/+1784，元素16B；不扫描所有ID、resize或调用getter来制造流。Entity、Grid、room、run退休时撤销读取资格；非拥有Lua userdata和缓存地址不延长owner寿命。
- `Level::generate_dungeon`与LevelGenerator的栈／临时RNG仅在原生owner调用存活期间可观察；不保存原生指针给worker，不为Client重建生成器。
- Grid Light按SpawnSeed＋GameFrame临时生成绘制输入，已有Desc状态不能因“seed不上线”误删；对象Grid RNG与PressurePlate+26C的GreedModeRNG仍归Host。Backdrop绘制／纹理结果由第7步资源owner承接，不复制其生成RNG；当前D0也已退出两个全局绘制游标。
- Dataminer的地址种子属于本进程确定的纹理选择，现有Grid已传最终2位；不发地址，不复制Host RNG，不在Client按自己地址再随机。程序道具／shader／音频等实际资源依赖同理。
- CRT／Lua状态、原版Online与Isaac Sync、C/S会话状态彼此隔离。诊断观测流的完整状态不是正式下行负载，不把旧录制、random调用事件或未注册Mod RNG接进同步链。

**第7步双向交接核对完成（静态，2026-11-10）**：15项决定、来源与消费者见[交接表](room-random-handoff.md)／[JSON](room-random-handoff.json)，第7步入口见[7.4](7-room-level.md#74-与randompresentation的交接)。当前决定：

- Game/Level两条生成流、Backdrop流／seed、descriptor四seed／商店／保存随机字段留Host；真实结构、资源、Entity和地图后态仍须承接。
- D0由旧13B草稿改5B，仅Manager相位／paused；Pickup和Familiar发送正常Host已产生的逐用途alpha／资格，不重演两个全局游标。
- Error沿用最新Descriptor的1B颜色谓词，不发完整seed或效果ID；已由最终颜色完整承接时不重复发。挑战32交接已选SafeGridIndex锚点及对应描述／窗口上下文，不只是一张随机标记。
- 新核BD0F40的16B音效选样本流归Host；实际sample、逻辑voice／gain／pitch／pan由Presentation接线，原生随机Play不作为Client应用接口。

本次完成网络职责与正常来源／消费点的静态核对，正式layout、正常结果缓存、资源leaf和显示／音频adapter仍待9/10/12/13步；不把设计决定写成已运行完成。

## 8. 完成范围与后续接线

本轮交付静态账本、全部54个旧公开行（保留重复ID、三种GetCollectible签名和重载）、补充接口与owner清单；不修改旧历史事实或把其观测PASS外推为同步PASS。旧BackupPoolType签名原样保留，但本地1.1.3 ZHL只有flags版ABI，不能据旧目录调用不存在的重载。

第8步后续正式Presentation布局须落实D0的5B、逐用途随机绘制结果、药丸／预览及音频／地图结果；第10步取得正常生产点的合法同world来源；第13步安装、批次相位及借用恢复；第14步再退役旧application_draw的四字组／两seed草稿、随机域JSON和全量池相等要求。当前生产版本56及旧运行代码保持原状。

第15步唯一统一验收再覆盖：Host随机流零额外消费、两绘制流首次初始化／pause／重复展示／多实体排序、借用恢复、普通／马药未知与身体修饰、饰品流与池不下行、预览／挑战32／分数／音效结果，以及缺来源明确拒绝应用。本轮没有运行这些测试；两份绘制结果的正式批次／缓存／窄adapter、池未明字节、官方扩展ABI都是显式未实施或未决项，不以静态核对完成冒充它们已解决。
