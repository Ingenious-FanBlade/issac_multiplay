# 第7步 RoomDescriptor：原生字节归属与同步职责

2026-11-10，官方 REPENTOGON 1.1.3／Repentance+ v1.9.7.12.J273 Win32。完成本类 **B8h／184B 直属字节核对与44条公开成员回映射**，以及必要保存地图输入的设计。原24B未知已找到实际消费者，直属未归属字节24→0；这不等于所有私有语义、配置、保存体或正式同步已经完成。生产版本仍56，无构建、游戏、getter执行或测试。

> **2026-11-12 标注（显示委托 Q9）**：用户决定同步当前房间描述的 SpawnSeed（`+0x5C`），因为放行的原生 `Game::Render` 在房间年龄 30 时读它，为 0 即 `int3`。第 4 节表中“+58／5C／60／64 四seed”一行的“当前 SpawnSeed 只取必要 Error 颜色谓词”，以及第 5 节 Error 饰品两段的“完整 seed 留 Host、只发 D1 的 1B 谓词”由此取代；D1 在布局收缩时评估退役，不与完整 seed 双发。其他三个 seed、其他房间描述的 seed 仍留 Host。见[修改方案第 5 节](client-display-delegation.md#5-仍需同步的显示相关状态候选)表后说明。原文与证据保留。

数据入口：[原生账本JSON](room-descriptor-byte-ledger.json)／[单行表](room-descriptor-byte-ledger.md)，[44行目录JSON](room-descriptor-member-map.json)／[逐行表](room-descriptor-member-map.md)。同步回写[Level账本](level-byte-ledger.md)及[Level目录](level-member-map.md)，保留上轮24B未知、四位flags和非Greed Boss布尔清零的历史决定及本次纠正理由。Room同期补证另见[Room核对](room-synchronization-review.md)。

## 1. Wiki、官方源码和证据口径

按顺序先读本地 `IsaacDocs/docs/RoomDescriptor.md`、`enums/{RoomDescriptor,GridRooms,GridEntityType}.md`；再读 REPENTOGON `docs/docs/{RoomDescriptor,Room}.md`；随后核对官方：

- `libzhl/functions/{RoomDescriptor,Room,RoomConfigRoom,GridEntityDesc,EntitySaveState}.zhl`；
- `repentogon/LuaInterfaces/Room/{LuaRoomDescriptor,RoomPlacement}.cpp`，以及 `SaveStateManagement/EntitySaveStateManagement.cpp` 的保存身份／生命周期；
- `libzhl/IsaacRepentance_static.cpp:588` 的 Error饰品纯值算法，`repentogon/Patches/ASMPatches/ASMHud.cpp` 的实际图像与颜色消费者。

Wiki已说明GridIndex、SafeGridIndex、ListIndex及当前入口象限不同；OverrideData和Data独立；AllowedDoors是门mask，Doors[]是潜在结构目标，不等于实际门存在；ShopItemIdx和折扣索引参与下一件货物／价格生产。公开可读和旧ReplicatedState都不是发送理由。原版Wiki的AllowedDoors userdata缺陷由官方扩展修复；枚举页DISPLAY_*序号不能代替DisplayFlags实际位检查。

最后只读分析J273 IDB。以下函数／指令地址为VA，imagebase=400000h，RVA=VA−400000h；对象偏移另标`+`。静态指令、官方源码、SDK声明和运行观察分开：本轮没有新的运行观察，未修改IDB，也未校验哈希。GreedSubType的私有业务名仍按SDK声明，不以没有命中绘制检索证明其废弃。

## 2. 原生对象和生命周期

`6ED4B0`以B8步长构造527个描述，`6EBB10`为本类构造，`6EBF40`按+88／+74／+68销毁两个EntitySaveState vector及Grid保存vector，并销毁+80的set。对象没有证据支持直接传送其容器头／指针。

`733010`实际是 **move assignment**：复制标量；释放目标既有容器，再接管源+68／+74／+88三个vector头并清源头，交换set head/count；最后逐项搬移八个short和八个byte。它不是可用于Client的“复制字节”函数。`6F4F20`的复制路径同样经容器helper，不能拿B8字节memcpy代替。官方Reset直接析构后重构，会分配保存存储；不能为采集、清理或状态安装调用它。

构造预置448个20h的Grid保存元素，分配经32B对齐；初始元素数与后续逻辑元素范围必须区分。`6EBB10`的+94..A3清零不包含+A4..AB；后者在商店初始化`809689`才清8B。取得当前descriptor不制造未见过的初始化资格；Host留存数据也不因此允许读未初始化容量尾部。

描述结构身份用本world的物理源槽／代次；ListIndex可以为负或变化，不能代替物理源槽。-19是原版Deathmatch槽，C/S不读写；-20的Lil Portal特殊寿命、-100/-101路由和Genesis负ListIndex沿用Level已核定边界。普通成员／配置／保存体原位变化不能只看指针、size或visited／clear计数。

## 3. 原24B UNKNOWN：两张商店生产表

| 原生范围 | 已核用途／证据 | 同步决定 |
|---|---|---|
| +94..A3，i16[8] | 八个ShopItemID槽的**货物生成类别**，不是最终Pickup subtype。`809640`按房间／池／seed填表，`809F4B`作2B索引写；`80A2E0`从`descriptor+94+2*slot`读类别，switch产生Pickup variant及pool抽选；`80A8EF`、`80B5DC`还影响价格 | 全16B留Host。实际Pickup及最终Price／价格Sprite由Entity域同步，Client不重跑货物抽选或改价 |
| +A4..AB，u8[8] | 八槽补货次数。`809689`整8B清零；`80AD15..80AD22`读取并在FF前饱和递增；`80B6FD`明确movzx读无符号字节，按次数、基础价格／7的下限项算三角增价并封顶99 | 全8B留Host。补货本身还创建Pickup／Effect、推进SpawnSeed；不能把这个函数当只读刷新 |

`80ACA0`由官方Room::ShopRestockPartial签名核定，日志含`Spawn shop pickup (restock)`，会从队列取grid/shop槽、增加计数、创建实体并写回descriptor.SpawnSeed。`80B5A0`为Room::TryGetShopDiscount，除折扣外会读取+A4数组并处理玩家物品／难度。`80A790`为GetShopItemPrice，仍有随机价格分支；**不是无副作用的最终显示来源**。最终Pickup+534 Price与价格ANM2的独立必要性见[Pickup核对](pickup-slot-synchronization-review.md)。

逻辑数组仍是8项：某个原生内部helper把非法输入夹到8、继而可能与+A4重叠，不能据此虚构第9个i16类别。这里只记录静态边界，不扩大Client允许槽或通过调用原生helper制造样本。类别的具体0..20枚举名没有硬套PickupVariant；已核足以将这些字节归Host生成，不宣称所有类别业务名已解释。

另补核+48的花费计数：`7CF3EF..7CF3FC`从Game→Room→descriptor加16位花费，`7CF400`按无符号阈值40比较，`7CF415→903930`可能解锁。SDK写int16_t，真实消费是16位无符号阈值；它不能作为Client支付／解锁命令下发。+4A ClearCount有`7E5DF2／7FBDF4`的16位递增；当前clear显示已经由Flags.bit0承接，不复制累计历史。

## 4. 同步集合与不下发集合

| 原生范围／成员 | 处理 | 理由 |
|---|---|---|
| +00 GridIndex、+04 SafeGridIndex、+0C Dimension | 保留i32／i32／u8 | 结构／当前及随机地图锚点。`7332F0`核定LTL配置shape9使Safe=Grid+1；其他正常放置取Grid。离图特殊情况保留来源，不用该公式盲目覆盖全部当前对象 |
| +08 ListIndex | 原值留Host，只取当前描述与Game+18314相等的bool | 该bool既用于Greed HUD，又用于普通路线TrapDoor光照，见下节；物理源槽是另一种身份 |
| +10 Data、+14 OverrideData | 两个独立nullable配置资源引用 | 保留基础布局和已选替换配置关系；资源身份至少包含实际来源／stage/mode/type/variant等能唯一解析的上下文，动态／合并配置须内容版本。指针、权重和候选Spawns不是状态下发。两个地址相同可共享资源号，不能把两角色合为一项 |
| +18 AllowedDoors、+1C..3B Doors[8] | 低8位u8＋8×i32 | 原生Void大房地图边框读取AllowedDoors，目标有-100/-101等路由。实际门槽由Room/Grid承接，不以结构目标触发过门FSM |
| +3C DisplayFlags | 最终u32 | 原地图既检查具体低位也检查整体非零，保留实际值；不发送可见性生产器或重放ShowMap／UpdateVisibility |
| +40 VisitedCount | `!=0`的u8 | `96447E／9644E6`只区分未访问／已访问；累计历史不发 |
| +44 Flags | **5位u8显示投影**，完整原值留Host | 当前需要clear／red_room／devil_treasure／blue_redirect／has_water；其它必要显示后果由具名最终owner承接，见下一节 |
| +48／4A／4C／50／52／54 | Host花费、清理历史、SDK GreedSubType、Poop、TKeeperCoin、Pits | 不为旧Getter相等复制支付／防刷／生成历史。TKeeper计数文档明确重进防刷；Grid／Pickup最终结果另承接。GreedSubType仅已核存储与搬移、业务名依SDK，不声称填充或无用途 |
| +58／5C／60／64 四seed | 原值留Host，当前SpawnSeed只取必要Error颜色谓词 | 装饰、出生、奖励、Boss死亡生产；challenge32取Host已选地图标记；Error完整效果ID也不发，图像已有独立结果，颜色只增加当前上下文的1B是否ModelingClay |
| +68 SavedGridEntities | 保存体留Host，地图楼梯结果保留 | `964B6D..964BA2`按20h步长扫描type==18；只取最终有无楼梯，绝不下发448份Grid保存状态或在Client Restore |
| +74 SavedEntities | 保存体留Host，只发聚合后的地图图标mask／帧 | 已核78h步长和type／variant／subtype、+14及+28/+2C条件。Host完成过滤及OR／max聚合，原类型／ID／数量／顺序都不发，见第6节 |
| +80 RestrictedGridIndexes | 完整set留Host | 官方getter遍历树并分配Lua表；用途是放房／Grid生成限制，不是当前画面。无需为正式下行实现树遍历 |
| +88 SavedEffects | 完整vector留Host | 当前Effect按Entity域独立安装，离房保存不能再次Restore。没有把该保存队列当作当前实体成员表 |
| +94..AB | 两张Host商店表 | 本轮已将原未知归入明确生成／补货用途，不发原表 |
| +AC／AE ShopItemIdx／DiscountIdx | i16值留Host，保留-1语义 | 货物槽循环、折扣生产；当前Pickup Price及价格Sprite才是显示输入 |
| +B0 DeliriumDistance、+B4 Group | 留Host | Void生成／Boss距离和传送抽选分组；官方CreateGroup／GetGroupByName属于进程内注册表，不网络化其地址、顺序或私有ID |

## 5. Flags与Boss匹配：纠正上轮显示遗漏

L1的`display_bits:u8`仍占1B，原生位 **0／10／11／18／5** 依次投影线上bit **0／1／2／3／4**，bit5..7必须为零：

| 原生位 | 显示消费者 | 处理 |
|---|---|---|
| 0 clear | `964037`地图清理外观、`97D0EA`附近Greed HUD，Room自身显示分支 | 保留线上bit0；不TriggerClear／奖励 |
| 10 red_room | `963EF5`地图红房框 | 保留线上bit1 |
| 11 devil_treasure | `964392`选择红宝物房图标 | 保留线上bit2；商店生成仍Host |
| 18 blue_redirect | `8028AE..8028BB` Door目标描述的Light分支 | 保留线上bit3；按目标描述／路由解析，不启动Blue Womb过房 |
| **5 has_water** | `801BE0`的Pit分支在Backdrop32／58／59直接检查；`804500`的Backdrop32特殊坑重绘也检查 | **新增线上bit4**。它不是Room.waterAmount或最终水色，也不是Grid的WaterClip；前一轮“只由最终水面承接”不充分 |
| 2 sacrifice_done | `803E20`献祭房光照分支在未完成时改变最终shadow fill | 不增加独立bit；必须保留Room最终lighting／pass结果。不能把这项完全当无显示的玩法历史 |
| 9 pitch_black | `803E20`及`8134C0`读取 | 保留Room最终lighting／pass，不在Client重算GetLightingAlpha；不另同步其原值 |
| 6 alt_boss_music | Boss音乐选择 | 发实际Music／voice意图，不在Client重新选择音乐 |
| 12 alternate_backdrop、15 no_walls | 背景／墙生成选择 | 发实际背景／墙／surface及几何结果，不重跑Init生成 |
| 1／3／4／7／8／13／14／16／17及高位 | 触发、奖励、替换、洪水、雾、爆炸、重进恢复、Portal防重等 | Host执行；对应真实Entity、Grid、资源、颜色／音画结果仍需齐全。高位未解释语义不当padding或可任意回放的扩展标志 |

`815B80`仅比较当前descriptor+8和Game+18314。**原L0把`current_is_last_boss`在非Greed规范false是错误设计**：`803313`的RenderGridLight会在TrapDoor分支调用它，随后`748900`在Greed/Greedier直接false，而在普通Ascent相关路线可能true。这正是需要非Greed匹配值的场景。现改为所有模式保留真实bool，L0仍22B；只有`greed_wave`在非Greed规范0。

Host用已知当前描述标量直接比较，不额外调用可能初始化fallback的GetRoomByIdx。Client将私有查询路由到同world影子bool，不恢复ListIndex／lastBoss选择器原值。另需同world的GridLight route condition／stage suffix结果；不能因为有bool就放行Client重查本地GameStateFlags或解锁。该查询的其它调用仍按隔离约定透传，不能全局改写原版函数。

Error饰品也有明确官方消费者：纯函数把SpawnSeed经`>>2, <<7, >>25`的u32异或折叠，非零seed返回`seed%189+1`，零seed返回TRINKET_ERROR=75；不推进对象RNG。官方ASMHud的Image选择由Host实际图像承接，不能再为选择图像发送完整效果ID或重跑HistoryHUD::Recompute。剩余颜色分支仅检查“效果>0”和“效果==ModelingClay”：前者在目标版所有合法路径恒真，无须发送；后者需要 **D1当前描述颜色上下文的`u8 bool error_is_modeling_clay`，1B**。只对当前已绑定world的必要颜色owner提供，不在所有离房描述重复发送。Host从合法当前SpawnSeed按官方纯整数式取该谓词，不调用getter或RNG；完整seed及完整效果ID均留Host。

Client的具名颜色adapter直接使用D1和真实Player当前ModelingClay效果／最终颜色上下文，不任由官方颜色分支读取本地SpawnSeed，也不伪造GetErrorTrinketEffect的数值返回值；其它查询不因这一布尔获得执行资格。D1每当前world只保留一份，随谓词变化更新，不是仅进房采一次：补货等正常Host路径也可能写SpawnSeed。实际图像依然使用Host结果，缺颜色依赖时报Gap；若Presentation最终颜色记录已完整承接这两个比较，则不再重复发送D1，不能把等价颜色与谓词双发。本轮初稿曾拟2B完整效果ID；复读上述两个颜色比较后已收紧为1B，不把旧草稿当生产实现或必要数据。1B不表示整个HUD接线已完成。

## 6. 保存地图最终结果的有界设计

L1仍53B：保留上轮字段位置，只把线偏移+33h的display_bits由4位扩至5位；D1当前描述颜色上下文另为1B，不重复记入L1。L2取 **Host最终聚合结果**，没有生产layout号：`u8 present`；present=0仅1B；present=1接`u16 icon_mask`，再按bit升序附存在的动态类别帧u8。长度为 **`3 + popcount(icon_mask & 0x2B9B)`，即3—12B**；不发保存元素数量、顺序、type／variant／subtype、坐标或原条件值。

`9635B0`明确先把mask和各帧清零；每类首次出现只OR相应位，同类只取最大帧。随后才把房型图标存在性加入数量、按本地地图配置安排位置和显示上限。因此保存图标数量可由mask的popcount得到，房型图标由L1／配置另计；须保留**显示上限筛选前**的完整聚合，不仅复制Host当前已画的前缀，也不复制Host的本地排版。上轮Level文档曾写“计数／顺序影响图标选择”，本次据OR/max聚合明确纠正：重复项数量或保存顺序本身不影响结果。

| mask bit | 图标类别 | 帧处理 |
|---|---|---|
| 0／1／3／4 | heart／coin／key／bomb | 动态最大帧分别0..9／0..3／0..2／0..1，仅相应bit存在时发送 |
| 7／8／9 | pill／battery／chest | 动态最大帧分别0..1／0..1／0..9，仅存在时发送 |
| 11／13 | machine／beggar | 动态最大帧0..8／0..5；Slot variant14忽略，default也产生machine帧2，不能漏掉 |
| 2／5／6／10／12 | golden bomb／card／collectible／trinket／rune | 固定帧0，只发存在位，不发u8零常量 |
| 14／15 | stairs／portal | 保存Grid任一type18、保存Entity任一1000/161/980置位，重复幂等，固定帧0不发 |

Host只对实际采用的Pickup分支读取subtype，先按intStorage3／targetPosition原点条件过滤；未采用variant不进入图标聚合。card/rune取正常Host固定CardConfig中有效记录的type==2判断，不抽卡、不读Client池。完整分支表写入[机器源的host_rules](room-descriptor-byte-ledger.json)：包括心的半心帧0／其它子型、硬币／钥匙／炸弹／药丸／电池、69号箱子特殊subtype、`6EAA70`完整箱子集合、Slot default／合并case，以及Portal／楼梯。对同类OR/max的重复或保存顺序变化不产生新负载。

present绑定描述身份／world／资源修订和实际可用地图分支；DisplayFlags为0的未显示描述输出present=0，不强读隐藏保存集合。已显示但无法读取、窗口变化、扫描预算或配置不足为Gap，不能当present=0；present1/mask0是已知没有保存图标，不等于缺席或失败。通用EntitySaveState仍74B UNKNOWN；仅type5地图输入视图的+14排除谓词4B已核，视图为70B，其它类型不借此获得同一字段语义。

最终包至多12B不意味着源扫描无界或免费：源vector的begin/end/capacity、78h／20h步长、元素构造资格、读前后稳定及第10步扫描预算仍需满足，超限报Gap，不截断；不遍历capacity尾部或递归复制strong holder。最终mask／帧绑定本次完整来源，不增设逐consumer全表读取。

Client持有自有图标结果，由具名地图adapter按原版固定优先顺序和本地配置绘制；不把3—12B包安装成78h保存vector，不调用Restore／UpdateVisibility／随机Refresh。实际图标Sprite／资源及图形状态恢复继续适用。challenge32的Host已选当前位置仍另有来源，不能因已有图标mask就在Client重新随机选择。Host合法来源时点、正式mapper／writer及显示adapter仍待第10/12/13步；本轮只核定其状态输入与算法，不宣称实际接线已完成。

## 7. 完成口径与失败方式

完整184B直属表没有新增填充假设。原24B未知、两处必要显示输入遗漏及上轮结论作为修订历史保留。SDK GreedSubType业务名、Flags未解释高位、通用EntitySaveState的74B未知、GridEntityDesc的3B未知及外部配置未知仍明示；不把一个consumer-view的70B当作全保存体未知量。原始44行含两个重载、空参数／null、全局ordinal、field、wire_name和旧ownership保留，另登记原生私有补充，不伪造新增公开API。

需在第15步统一验收覆盖：商店类别变化但最终货物不变不重发；计数0／254／255与价格封顶、最终价格／Sprite／水面一致；不运行Client补货／改价／花费解锁；普通TrapDoor Boss匹配／route；HasWater与waterAmount独立、献祭／pitch-black光照；Door Blue Key Light；保存Pickup条件改变、各图标动态帧、Slot default／variant14、Portal／楼梯、重复和顺序变化而聚合不变不重发、Card/Rune及69号箱子；Error→ModelingClay实际图像与颜色；大房/LTL／离图／配置变化、资源等待、源扫描超预算及误借用78h保存头拒绝。五位投影／Boss匹配／L2与D1新视图只在正式注册时递增整数版本，当前56不接受草稿记录。

本轮未运行上述测试，也未实现正式reader／writer、配置表、地图adapter或保存恢复；不得据本核对宣称整个第7步、所有RoomSave、Exploration或可玩同步已完成。
