# 第7步 Level：原生字节归属与同步职责核对

**2026-11-10 修订2，RoomDescriptor专门核对**：[本轮证据](room-descriptor-synchronization-review.md)将描述原24B未知核为16B商店生成类别＋8B补货次数，均留Host；L1增加HasWater第五显示位，L0的Boss匹配在所有模式保留真实值，新增当前Error颜色谓词D1=1B及L2最终地图聚合设计。修订1的24B UNKNOWN、四位flags／非Greed清零与跨域Error缺口保留在JSON历史及新核对中；下文现按新结论修正，生产版本仍56，无构建或测试。

2026-11-10；官方 REPENTOGON 1.1.3 / Repentance+ v1.9.7.12.J273 / Win32。**本轮完成 Level 的静态字节登记、同步决定及 131 行公开目录回映射**，不代表整个第7步、其他域目录、正式编码或原生安装完成；Room的同期静态核对见[其职责文档](room-synchronization-review.md)。生产布局版本仍为56，没有构建、运行游戏或测试。字节源见 [JSON](level-byte-ledger.json)／[单行表](level-byte-ledger.md)，目录见 [JSON](level-member-map.json)／[逐行表](level-member-map.md)。

## 1. 资料顺序和证据边界

先读本地原版 Wiki `F:/projects_isaac/IsaacDocs/docs/Level.md`、`RoomDescriptor.md`、`enums/{GridRooms,RoomDescriptor,LevelStateFlag,LevelCurse}.md`，再读 REPENTOGON `docs/docs/{Level,RoomDescriptor}.md`。原版文档已明确：CurrentRoomIndex 是进入大房间的象限；GridIndex 可以落在 L 型缺口；ListIndex 是描述身份字段；维度为0／1／2；改变显示标志不会自行刷新地图缓存。房间生成、抽选、状态修改接口不能作为正式采集来源。

随后读官方 `libzhl/functions/{Game,Level,RoomDescriptor,Minimap}.zhl`、`repentogon/LuaInterfaces/{LuaLevel.cpp,Level.h,Room/LuaRoomDescriptor.cpp}` 和 `Patches/ASMPatches/ASMLevel.cpp`；最后在当前 J273 IDB 只读反编译、读取指令及交叉引用。以下地址均为 **VA，imagebase=0x400000，RVA=VA−0x400000**。没有执行目标函数、修改 IDB 或校验哈希。

`已核`只表示所引静态实现支持相应范围／职责；`SDK声明`、`ABI模型`与`UNKNOWN`分别保留。单行表登记每个字节的类别，不等于每个字节的语义已知，更不等于能够安全 memcpy 整个对象。

## 2. 两个必须纠正的布局问题

### 2.1 官方简化 Level 的两个字段滞后了0x170

`Level.zhl:97–98` 与 `include/IsaacRepentance.h` 把 `_greedwavesclearedwithoutredheartdamage` 放在 **18204**、`_levelStateFlag` 放在 **1822C**。但完整 Game 布局和 J273 的 `606D10`／`7332F0`／`7415E0` 表明：`17ADC..182C7` 是 **3×169 个 i32 房间存储槽映射**。前述两处分别是 dimension2 的 grid120／130 映射项，不是独立计数或 flags。

真实楼层标志在 **1839C**：`4F3CA0` 按0..23位查询，`4242A0` 修改；`745AC7` 写 Mama Mega 的0x4000。真实 Greed 连续无红心伤害波次在 **18374**：`7455D7`（SetRedHeartDamage）清零，`80BE88／80BEAB` 在波次奖励路径读取／递增。两者都比简化声明晚 `2×B8=170h`；这是静态位置对应的解释，不追写其历史成因。

官方 `LuaLevel.cpp:52–60` 的 Greed Getter／Setter通过 `_greedwavesclearedwithoutredheartdamage` 成员读写；按本地1.1.3头和源码，编译访问位置为18204，即错误映射项。这里是**官方源码／SDK冲突**，没有把已加载官方DLL的Getter机器码或运行时返回值冒称已观察。**不把该 API 当真实计数来源、不调用 Setter、不修改仓库外官方 SDK，也不伪称已修复官方 DLL。** 回映射单列源码访问位置和真实玩法字段。正式同步不依赖二者；第15步 Lua 对照不能要求这个 Getter 与18374相等。

### 2.2 1890C 是 SDK 大小，实际楼层前缀还包含最后4B

SDK `Level` 声明大小1890C；Game 的 RoomConfig 从18910开始。`6ED4B0` 在1890C初始化一个字节，`74B591` 保存上次维度到该字节，`739F08`读取，`7415E0／741E00`保存恢复该尾部。因此本账本明确登记 **Game 的 Level 视图 `[0,18910)`**，并保留 SDK 边界；不把它冒称新 `sizeof(Level)`，也不将1890C..1890F吞成 RoomConfig 或无意义填充。

## 3. 楼层根的发送决定

| 来源 | 决定 | 实际理由／接收边界 |
|---|---|---|
| +0 stage、+4 stageType | 保留两个u32 | Room镜面pass、HUD、地图、楼层名／转场均有消费者。挑战44的逐房主题另取最终资源，不发送主题生成图 |
| +C 原始curses | 不单发；发送最终`effective_curses:u32` | `742450`、`966300`、`966680`、Room Render与Player健康显示消费有效组合；原始值不等于有效值 |
| +17ADC 三维占用表 | 结构状态，投影为507个u16源槽或FFFF | `606D10`每维步长2A4；`9635EC`直接借用当前维表。映射值指物理描述存储槽；不能用可变ListIndex、GridIndex或SafeGridIndex替代 |
| +182C8 当前维表指针 | 本地派生，零线上地址 | Client在本地指向`Game+17ADC+dimension*2A4`；`739F02／739FD8`提供原版赋值证据。只为已安装的有效维度建立别名 |
| +182CC 房间数 | 由结构成员集合提供，不再发第二份权威计数 | 保留真实有序普通描述成员／有效性；不能将所有527个固定存储槽视为活动房间，也不把离图描述混入普通计数 |
| +18300 Room* | 引用当前world的Room owner | 只传结构关系；不传地址，不用它获得任意房间初始化资格 |
| +18304 当前入口象限、+1830C 当前维度 | i32＋u8保留 | 当前象限与描述左上角不同；负特殊房间保持有符号。维度仅0..2，-1是查询默认值，不是已安装维度 |
| +18314 lastBossRoomListIdx | 原值留Host；所有模式均保留当前描述ListIndex与它相等的u8 | `97D0EA`用于Greed HUD；本轮补核`803313→815B80`还用于普通路线TrapDoor Light，不能非Greed清零。不安装原生Boss选择器 |
| +18334 GreedModeWave | 保留i32 | `97D371`直接用于`%d/%d`，分母由difficulty决定。非Greed输出规范0，不为相等性读取旧残留；不调用74D700或波次Update |
| +18370 Greed宝物房索引 | 原值留Host；每描述只给u8最终图标选择 | `96437B`在type4分支比较GridIndex，选择`IconTreasureRoomGreed`；其余按普通／Devil's Crown图标。不能据该索引在Client生成宝物房 |
| +8 bossChallenge、+9 devilRoomDisabled、+10 heartPicked、+11 canSeeEverything | Host生产输入 | 挑战房／交易／可见性由Host裁决；地图接收最终DisplayFlags／图标，概率接收StatHUD结果。没有必要为Getter相等恢复这四个字节 |
| +182D4／+182E4 两份RNG、+182F4 angel chance、+1832C dungeonPlacementSeed | Host权威 | 生成／交易概率输入；官方`GetRoomPlacementSeed`推进自有RNG，不能在Client按seed再放房。Devil/Angel最终概率归StatHUD，显示结果不能误用此angel权重 |
| +182D0 startingRoom、+18308／10 previous room/dimension、+18318／1C leave/enter door、+18320..2B dungeon return、+18330额外Boss列表索引 | Host生成／跨房控制 | 实际门、实体位置、当前关系与转场画面另同步；不通过ChangeRoom、StartRoomTransition、GetEnterPosition补算它们 |
| +18338..67十二个Greed波次seed、+18368／6C波次控制、+18374无红心伤害连续波次 | Host权威 | `73DEB8`起产生12个seed；`7E5D60／7F7420／7FBDD0／80BA70`消费控制状态。HUD只需要上行表中当前wave／最终显示 |
| +18378..8F两组portal返回记录、+188FC..1890F Death Certificate返回记录 | Host权威 | `744250`写两组12B；`74B530`保存返回上下文并开始过房。门户本身的Entity／Sprite及已选转场需同步，返回FSM不复制 |
| +18390抽奖累积、+18394..9B Mama Mega世界起点、+1839C全flags、+183A0初始化中标记、+188F8本层一次性玩法锁存 | Host权威 | `6FD5F0／6FD701`随机阈值与累加，`745A30`记录爆炸起点，`611AF1／611C8E`一次性生成。旗标7..10等只是可见性生产器，不等于当前地图缓存 |
| +183A4 Myosotis保存队列 | Host权威容器 | Wiki明确是带往下一层的拾取。Client接收Host真正创建的Pickup；不发送保存体、指针、容量，不执行Restore或再创建一次 |
| +183B0..18653、+18654..188F7 | Host权威的两份169×f32主题生成图 | `749790`生成，`749600`只在challenge44按图选stageType；`740290`也借此选当前楼层名。Room最终背景／配置及实际名称由对应owner承接，不能遗漏这些结果 |
| +182F8..FF互操作holder | 本地生命周期 | ctor清两个字；`732C50`及Game析构经holder/vtable/本地callback退休。具体C++类名未核，不冒称Lua或网络状态，不传指针 |

`LevelStateFlag`全24位不上线。它的显示后果仍必须存在：可见性／特殊地图标记、门或实体最终状态、实际音乐和转场由具名owner接收；不会在Client触发使用卡牌／Mama Mega、镜面破坏、矿井逃脱或奖励。官方扩展 `LevelASM::ForceSpecialQuest` 是对象外4B生成参数，初始化前重置，留Host；`CustomStageName`是对象外本地string，真正已选显示文字由Presentation承接，不复制string对象／全局地址。

### 有效诅咒的原生只读投影

`effective = (Game+C | Game+25AC8 | forced) & ~prevented`。`6F55C0`核定强制种子56→1、57→2、58→4、59→8、60→20h、61→40h、62→10h；57还要求challenge!=44、stage有符号奇数且<8、difficulty不为2/3。`6F5750`核定63/64/65/66/67/68→1/2/4/8/20h/40h，70使prevented=7Fh，79再并40h。Host从现有已核Seeds树只读成员关系计算，不运行Getter或插入／随机；当前Run的7位种子显示投影不足以让Client复算此式。**只发送最终u32**；Client绘制adapter使用该值，隔离再次合并本地种子／temporary的原版查询。

## 4. 描述、地图及保存数据的边界

物理存储为507个普通槽＋20个离图槽，步长B8。`732E90`重置527槽、默认negative的Grid/Safe为506−i，但活动Genesis等会具有负ListIndex，不能把默认值当永久约束。-20是Lil Portal，`reset_room_list(false)`专门保留；-19是原版Deathmatch槽，C/S不采集、不安装、不复用。-100／-101是路由值，不是第100／101个负槽；`73AD10`还会初始化TLS fallback、递归或扫描，不能为查表直接调用。NO_ROOM_IDX=-99999保留其哨兵含义，不转无符号。

原版属性绑定的偏移亦直接核对：`852A6A..852B77`列出DeliriumDistance=B0、ShopItemDiscountIdx=AE、ShopItemIdx=AC、AwardSeed=60、SpawnSeed=5C、DecorationSeed=58、PitsCount=54、PoopCount=50、ClearCount=4A、VisitedCount=40、Flags=44、DisplayFlags=3C、AllowedDoors=18及两配置／身份字段；对应不同宽度的注册模板接在`852B7E..852C1F`。地图保存元素另见`9645AB`检查+14、`9645B5／9645C7`读取+28／+2C。本轮在`7CF3FC..7CF415`补核TotalMoneySpent累积／无符号40阈值／解锁，仍只留Host；GreedSubType业务名继续`SDK声明`，不因为相邻字段已核就升级语义。

| RoomDescriptor字节 | 决定 | 消费者与原因 |
|---|---|---|
| +0 GridIndex、+4 SafeGridIndex、+C Dimension | 保留i32／i32／u8 | 拓扑、地图和资源定位；物理源槽由结构身份给出。+8 ListIndex原值只参与Host的lastBoss投影，不再作为Client寻址下标或必发值 |
| +10 Data、+14 OverrideData | 两个独立nullable配置资源引用 | 基础与替换配置并不等价；当前RoomConfig可取Type35等，不能只传RoomType。复用已选配置身份／具名资源内容，避免再抽选；空资源与资源未就绪分开 |
| +18 AllowedDoors、+1C..3B Doors[8] | u8门mask＋8×i32目标 | `9635B0`的Void大房间框线需要AllowedDoors；8个目标是结构关系，保留特殊负目标。Wiki明确Doors允许存在目标而该槽没有实际门，实际8门槽仍归Room／Grid关系 |
| +3C DisplayFlags | u32最终值 | 地图框、阴影、图标；不复制Level可见性生产器，也不把枚举DISPLAY_*误当Wiki列出的位值 |
| +40 VisitedCount | 只发`!=0`的u8 | `96447E／9644E6`只检查访问与未访问；历史累计次数留Host |
| +44 Flags | 只发5位u8显示投影 | 原bit0／10／11／18／5依次映射线上bit0..4；第五HasWater是Pit Light和Backdrop32重绘直接输入，不能仅用waterAmount／水色代替。bit2／9的最终lighting及其它实际音画另承接；完整flags不写Client |
| +48..57花费／清理计数／GreedSubType／Poop／TKeeperCoin／Pits、+58..67四seed、+AC..B7商店／Delirium／Group | 原值Host权威；当前SpawnSeed另取最终结果 | 奖励、生成、重进防刷、传送抽选留Host。背景取实际资源；Error实际Image由Presentation承接，D1只取1B是否ModelingClay供颜色adapter；>0恒真，完整效果ID不发、不伪造数值Getter或重跑History Recompute |
| +68 SavedGridEntities | 保存数据留Host，**地图楼梯结果必需** | `964B6D..964BA2`按32B步长扫描，仅用type==18产生地图楼梯图标。发送最终标记或严格的只读图标recipe，不复制448个Grid保存体，不Restore |
| +74 SavedEntities | 保存数据留Host，**地图图标结果必需** | `9635B0`按78h元素读取实际分支需要的ID与+14／+28/+2C谓词，Host执行OR/max聚合为L2 mask／帧；数量／顺序与原值不发。上轮“计数／顺序影响选择”已纠正；容器不发仍须保留图标结果 |
| +80 RestrictedGridIndexes | Host生成限制set | 不参与已核地图显示；本轮不遍历树、不发元素，不因草稿曾写“容器必须同步”保留。未来确需RoomSave观测时另核其reader |
| +88 SavedEffects | Host离房效果保存 | 当前真正显示的Effect由Entity域承接；此保存vector不能直接恢复成当前实体，也无本轮地图消费者 |
| +94..A3 | **16B Host商店生成类别** | `809640/80A2E0`核八项i16，由类别产生真实Pickup variant／池；完整业务枚举名未强加，全部留Host |
| +A4..AB | **8B Host补货次数** | `809689`清8B，`80AD15..22`在FF前饱和递增，`80B6FD`参与三角增价并封顶99；最终Pickup Price及价格Sprite另同步 |

最终 mapItemIcons 也不是Level flags：`966300`先查询PlayerManager的333／21／246／54／376，随后追加有效诅咒图标，并按最多7项、有序优先级绘制。它需要第5／8步实际最终列表或具名纯投影，不能用compass/map旗标替代。`9635B0`还会在challenge32用DecorationSeed及房间数选择错误当前位置标记；该最终选择必须来自Host，Client不重新随机。地图长按、窗口、页签、FBO和缓存对象归本地UI，Host的Map按键不强制他人展开。

## 5. 供后续实现采用的二进制设计

以下是**设计表，不是生产layout号**。L0固定22B：stage／stageType／effective_curses各4B，current_entry_index 4B、dimension 1B、greed_wave i32 4B、current_is_last_boss u8 1B。只有greed_wave在非Greed规范0、不读残留，Boss匹配所有模式保留。L1每个活动描述固定53B：Grid/Safe各4B、dimension 1B、两配置引用各2B、AllowedDoors 1B、八个目标各4B、DisplayFlags 4B、visited 1B、五位display_bits 1B、Greed宝物房最终图标开关1B（不适用时0）。D1当前必要Error颜色谓词另1B，不在所有描述重复发送。配置引用复用u16／FFFF规则，具体RoomConfig资源表仍待第7/9步注册，不能用路径资源表冒充完整配置。

L3结构表为三维507×u16=1014B，FFFF为空，其余是0..526物理源槽；不允许引用未提供／不活动的描述或Deathmatch槽525。成员生命周期和source-slot身份由结构消息承载，不能以这些负载长度估算完整Baseline。+182CC的有序普通房间成员须单独保留；不在每次world增量重发整表。

L2不下发原始保存集合。本轮[专门核对](room-descriptor-synchronization-review.md)核定Host按原生OR/max得到最终16类图标mask与必要帧，缺席1B、存在3..12B，楼梯/Portal为mask位；不发原类型／ID／数量／顺序，不能安装成78h保存vector。challenge32最终标记、资源表与实际reader／draw adapter尚未接入，设计字节预算不表示合法来源已实现。RoomSave完整类型不属于本轮完成口径。

Client按结构消息建立本地描述／维表别名，再安装影子显示值；原生Flags／资源／保存容器若在已核绘制窗口需要借用，使用专用adapter并精确恢复，不常驻写入Host全flags或空造存档容器。不调用Level::Init／Update／ChangeRoom／place_room、Room::Init／Restore、SetNextStage、地图Update、概率/分数计算或存档／解锁入口补状态。

## 6. 完成范围、未知与后续验收

原始stream目录 `source_schema=isaac.cs.level` 的131行（87 Level、44 RoomDescriptor）全部保留原始ordinal、field、wire_name、参数空白/null和重载；同时附应用目录中的原始行号，旧ownership不改写。旧ReplicatedState不自动成为发送理由，命令与查询也不变成任意Getter字典。第5/8步已有资源／HUD结果不在Level重复一份。

Level视图直属UNKNOWN仍 **22B**：A..B、12..13、18381..83、1838D..8F、183A1..A3、188F9..FB、18905..07、1890D..0F。共享RoomDescriptor本轮24→**0B直属未归属字节**，旧12646B展开上界仅属修订1历史，不再作为当前数字。GreedSubType的SDK业务名、Flags高位语义、RoomConfig／保存元素／Room外部未知另计；type5地图保存输入视图70B、通用EntitySaveState仍74B未知，不能声称整个世界只剩22B未知。

第10/12/13步仍须接入正式源、布局与安装资格：主线程、同run/world、描述成员／配置／维表前后稳定，初始化中或生命周期变化报具名Gap；UNKNOWN不采、不补零。结构事务未完成、配置等待或来源缺失时不ACK，不偷读原版Online、Lua或本地进度来填值。

第15步统一验收应覆盖：三维大房／L型缺口与入口象限、负ListIndex／Genesis／Lil Portal寿命、-100/-101路由及-19隔离、同count原位变化、双配置／特殊门Light、visited计数变而布尔不变、五位Flags与无关位不重发、普通路线Boss匹配／HasWater与最终光照、Greed波次/HUD、有效诅咒、挑战44背景／名称、challenge32地图、最终保存图标mask／帧及楼梯、当前Error图像／颜色、商店表不下发／不重演解锁、Myosotis与返回只在Host执行。22B及外部未知保持；不能通过错误Setter制造对照，不比较不上线的seed、保存历史、玩法flags或错误API计数。本轮没有执行这些测试。
