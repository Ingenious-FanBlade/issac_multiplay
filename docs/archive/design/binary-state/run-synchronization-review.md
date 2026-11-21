# 第 8 步 Run：原生字节与同步职责核对

2026-11-10；目标为官方 REPENTOGON 1.1.3 / v1.9.7.12.J273 Win32。本次只完成 **Run 的静态归属、布局与公开目录回映射**，不把 Presentation、Random、Mod 或正式采集／安装宣称为完成。布局源为 [`run.json`](../../../../mod/binding/state/run.json)，整数布局版本 56；生成表从 [GameRun](../../../../mod/binding/layouts/08-run-presentation/GameRun.md) 进入。没有运行游戏、测试、getter 或随机选择。

## 1. 资料顺序与证据口径

先读本地原版 Wiki `F:/projects_isaac/IsaacDocs/docs/{Game,Seeds,enums/SeedEffect,enums/GameStateFlag}.md`，再读 REPENTOGON `docs/docs/{Game,Seeds,Ambush,ChallengeParam,DailyChallenge}.md`；语义或容器来源不够明确时读官方 `libzhl/functions/*.zhl` 与 `repentogon/LuaInterfaces/*.cpp`，最后对 J273 做只读 IDA 分析。以下地址都是 VA，imagebase=0x400000，RVA=VA−0x400000；不是运行时观察。

`已核`表示下列实际指令／官方实现支持对应结论；`SDK声明`不等于独立核定了所有 J273 字节；没有依据的孔保留`未知`。归属完整和功能已实现分开：本轮没有注册新的 Hook 或读写处理器，表中的具名处理器是第 10／13 步的接线合同。

### 直接纠正原草稿的地方

1. **不是整组 flags／计数／种子都发送。** 玩法输入留 Host；实际显示消费的结果或窄投影才发送。公开 Getter、旧 `ReplicatedState` 和旧相等性检查都不是发送理由。
2. **GameFrame 与 TimeCounter 不同。** Game+25A70 是帧头 `game_frame`；Game+25A74 是可修改的 TimeCounter。原版 Wiki 明说 TimeCounter 用于计时事件，不用于 timestepping。Minimap::Render 966680 在 967720／967763 读取它，挑战22使用 `max(28800−TimeCounter,0)`。按总计划的帧锚点规则，线上发送`time_counter_base = u32(GameFrame) − bit_cast<u32>(i32 TimeCounter)`，Client用该world已应用帧头做模2^32减法再还原i32；不是令TimeCounter等于GameFrame或用墙钟推进。6F7889／6F7894／6F789D证实普通路径在同一分支递增两者，因此正常推进时基准不变；改时／回溯按实际值重新产生基准。
3. **DailyChallenge 有两份来源。** 官方 `LuaDailyChallenge.cpp::Lua_DailyChallengeGetChallengeParams` 从 `g_Manager->GetDailyChallenge()` 取参数；Game::GetChallengeParams 702320 在 Game+25BA8 非零时返回 Game+25BB8，否则查 Manager 的 challenge 配置向量。当前 run 副本与 Manager 预告／菜单副本不能互换。
4. **两个 Ambush 最大波次数在对象外。** `LuaAmbush.cpp` 的 `ambushWaves=3`／`bossAmbushWaves=2` 是 REPENTOGON 全局变量；只有 `maxBossWaves` 在 Ambush+18。不能把三个 Getter 都映射到原生同一字段。
5. **ColorModState 只同步 current 六值。** Game::Render 6F7A80 读取 Game+66920..66937，写入绘制 tint／shader；Update 6F6D30 与 SetColorModifier 7021A0 生产它。+66938 的 target、+66950 的 lerp／速度和 +6691C 开关留 Host，不在 Client 重放渐变生产器。
6. **黑暗效果不能照上一条机械裁剪。** 43BEB0／43BEC0 分别返回 Game+25A90／25A94；Room::PreRender 803E20、Room 光照 helper 8134C0、81AF50、82F210 直接使用两者乘积／组合。两个 f32 都保留，25A8C 的生产计时留 Host；不能因为第二值可能叫 target／scale 就删掉它。
7. **ScoreSheet 的SDK布局局部过时。** 官方把+40标为u32 runTime；J273的9B9D20／9B9DC0、83EE80实际按vector处理+40..4B，9BA470写真正完成时间到+4C。83F670从Leaderboard+30C的ScoreSheet副本逐项读取14个分数组件／总分，并额外读取+38完成楼层和+4C时间格式化结算显示。这16个最终值共64B保留；+3C类型与四个vector头／历史留Host，不能把指针当计时发送。

## 2. 同步边界

| 原生来源／结果 | 决定 | 具体消费者与限制 |
|---|---|---|
| Game+25A70 GameFrame | 帧头唯一承载 | shader 时间、Grid 计时文字等借用同 world 时钟；Run 不重复发送 |
| Game+25A74 TimeCounter | 原生i32；线上u32帧基准，保留公开int符号 | 966680 的时间文字／挑战22倒计时；由同world帧头减基准还原，不按 Client tick 自增 |
| Game+25A84 两分量 | 保留最终震动偏移 | HUD／Grid／Room 等直接读；25A80 震动倒计时是生产输入，不发送 |
| Game+25A90／25A94 | 两个有限 f32 | 803E20／8134C0／81AF50／82F210；25A8C 不传 |
| Game+25AC0 的 u64 flags | 只发 bit48 的 u8 布尔 | StageTransition::Render 909E60、shader predicate 90A040、967DB0 和 971D80 的 ascent 显示分支。其余位不原样搬运；不通过 SetStateFlag 回放 |
| Game+25AFC challenge | u32 当前身份 | 835B40 主动道具 HUD、966680 挑战22、97C300 的显示分支；不执行挑战初始化／完成 |
| Game+25B00 rerun | Host生产／资格输入，不重复发送 | 97C300只用它生产最终进度图标，已由下述1B最终结果承接；不为旧IsRerun原值相等恢复它 |
| Game+25F40 difficulty | u32，0..3 | 97C300 的 hard／Greedier 显示、835B40 等；IsGreedMode／IsHardMode 可据它纯计算 |
| Game+25BA8 当前 daily id | 只发`id!=0`的u8 | 98DDA0 的 SeedPaper／每日显示分支与 97C300只消费非零判断。实际日期／名称若显示由Presentation当前run批次承接；不发完整id用于每日Init或排行榜 |
| Seeds+4 start seed | u32 当前 run 元数据 | Wiki 指定暂停菜单显示来源；Client 只做 Seed2String 的纯转换，不调用 SetStartSeed／Restart 生成世界；实际 PauseScreen 缓存接入仍归 Presentation |
| Seeds+54/+58 种子效果树 | 不传树头、节点、数量或全量 membership | 从已有树只读投影下节的7个显示选择，其他效果由 Host 最终实体／资源／音画结果承接 |
| ScoreSheet+0..37、+38、+4C | 14 个最终 i32 分数组件／总分，加 i32 完成楼层、u32 完成时间；共64B | 9BAC90 已生产完成的缓存；9679FA 读 Game+25C90。83EE80复制到Leaderboard+30C，83F670逐项显示14分数／FINISH_TIME与完成时分秒；不传生产历史或+40的真实vector头 |
| Game+66920 current ColorModState | 6×f32 | 6F7A80 的最终 tint／shader 参数；不发另外两份 |
| Game+6699C lightning、+669A0 dizzy | 两个有限 f32 当前值 | Room::PreRender／Render 803E20／804500 消费；dizzy target、changeSpeed 留 Host |
| HUD 97C300 正常产生的“禁用成就”图标判定 | 1B 最终显示结果，须有真实 Host 显示批次 | 源涉及 custom、Seeds::AchievementUnlocksDisallowed、原生进度查询、rerun、debug／challenge；不在 Client 查询本地资格，不额外执行 Host getter，也不回写解锁规则 |

**flags 的 bit48 不是泛化的 flags 兼容表。** bit0..47、49..63 全部留 Host。Ascent 显示之外，bit47 等在 Nightmare::Show、房间生成、转场 Update 或分数 Calculate 中出现，属于生产路径；它们需要的最终 Nightmare／转场资源和分数由对应 owner 接手。967DB0／971D80 还要同 world 的 stage 等第7步上下文，不能只安装一个布尔就宣称显示完整。

**分数 Render 不是天然只读。** 97C300→966680→9BAC90 会在 Render 内重算分数；9BAC90 包含随机／玩法读取。Client 必须隔离这一重算用途，安装原 Host 已有的最终缓存；不能恢复整个 subplayer／健康／库存／击杀历史、临时 RNG 或再调用 Calculate。未取得对应状态／显示版本的缓存时报具名 Gap，不填零。

结算副本与Game当前缓存也要区分：83EE80的Leaderboard::Show会每日Init／清资源／发排行榜请求，不能调用它只为复制缓存。Presentation应在实际结算owner安装上述64B及其已有显示批次；新run开始后不把新Game缓存贴到旧结算画面。这里核定其负载与副作用边界，没有实现排行榜owner。

### 2.1 种子效果的最小显示投影

RunSeeds 的 `display_flags` 为一个 u8，bit0..6 分别代表 **8、9、10、29、30、78、79** 的当前 membership；bit7 必须为零。这是七项明确消费者的固定上下文，不是可带任意参数的 Getter 字典。

| 效果 | 实际显示消费者 | 为什么仍需同步 |
|---|---|---|
| 8 OLD_TV | 6F7A80 前后两次树查找，最终 TV shader | 游戏实体状态不能推出此屏幕 pass 开关 |
| 9 DYSLEXIA | PlayerHUD pocket 显示8378C0；HUD::ShowItemText 97B080 | 对原有文字的显示选择；不据此重抽取物品 |
| 10 NO_HUD | HUD::Render 97C300、Minimap::Render 966680、PauseScreen::Render 98DDA0 | 实际 Render 直接判断；不能依赖 Client 自己的设置／树 |
| 29 PILLS_NEVER_IDENTIFY、30 MYSTERY_TAROT_CARDS | 8378C0，包括8387D8／83882C附近 | 口袋类型／ID与显示身份不能混为一谈；不发送道具池 RNG 来补名称 |
| 78 RETRO_VISION | Room::Render 804500 | 直接覆盖 retrovision 强度分支；计时来源另归 Presentation，见第4节 |
| 79 G_FUEL | Room::RenderGridLight 801BE0、Entity_Tear::GetRenderHeight 672AC0 | 即使最终 Sprite 已同步，Light 和高度查询仍有直接种子分支；不能因名字像玩法效果就删掉 |

树 ABI：Seeds 9BE290／9BE320 分配0x14字节 sentinel，+54 为 head，+58 为 count；452580／662BC0 调4240B0 lower_bound，检查 node+0D is_nil 和 node+10 i32 key。`RunSeedEffectNode`列出 left／parent／right、color、is_nil、key，+0E..0F仍未知。节点地址、旋转／颜色／哨兵维护和分配器都是本地原生 owner 的实现细节，不上线。

Host 接线须在有效主线程窗口读取已有容器，限制正常 J273 枚举0..79与最大80个现有节点、检查根与 count 稳定，不调用 AddSeedEffect／构造／Reset。未知 key 或坏树报 Gap，不能跳过后声称完整。Client 只在已声明的绘制／只读显示查询作用域借用七个结果并恢复，不能把缺失当 false，更不能把整棵 Client 树替换后放行玩法 Update。

其余效果逐类不再单独发送：移动／射击／伤害／敌人生成／诅咒生产由 Host 执行；camo／invisible／头部／颜色等由 Entity／Player 最终状态和已声明显示结果承接；音调／音乐速度由最终音频状态或事件承接。若未来具体绘制消费者新增一个依赖，应修改具名投影与整数版本；不把这句话当作所有未研究 UI 自动完整的证明。

## 3. 明确不发送与不应发送的内容

| 范围 | 归属／依据 |
|---|---|
| Seeds+8 RNG、+18的14个stage种子、+50 playerInitSeed | Host 生成与后续随机流；9C0880实际推进RNG写14槽再写player seed。9C0980还会推进一个stage槽，不能把所有种子 Getter 当只读。Client 不据种子重演生成 |
| Seeds+0 custom | Host 资格／生成输入；HUD用上节最终图标结果，不能借显示修改访客资格 |
| Ambush全0x4C及REPENTOGON两个全局上限 | Spawn／Start生产器。827EB0写active／wave／delay并关门、播放音乐、设置两份RNG；827D60还随机重排波次。Client接收实际实体、门、音乐和最终显示，不执行它们 |
| ChallengeParam的起始物品／双子物品／房间过滤／心量／射速／路线／curse／achievement列表 | Host初始化、生成与完成逻辑。+0名称的实际显示由当前run title缓存／文本资源承接，动态daily名称不能从Manager的下一日参数替代 |
| DailyChallenge完整id／seed／practice／specialDailyId／params | Host当前每日生产输入。只留id非零显示投影；最终日期／资源／显示文字和结算结果按owner同步，不运行Init(日期)、每日开始或排行榜逻辑来补状态 |
| Game+25ACC..25AF7、+25F38／25F3C／25F44、+669AC | 交易／访问／伤害／捐款／楼层计数与victory-lap生产历史。700570达到阈值会调用解锁903930。HUD需要的devil／angel／planetarium等最终数值归StatHUD缓存，不在Client重算概率 |
| Game+25F4C 的vector头与元素 | 遭遇Boss历史；844990用8B元素差计数，6F51C0保存；不为旧GetNumEncounteredBosses原值相等发送整表 |
| Game+25F64 两份GlowingHourglassState到66914 | 2×0x204D8=264624B，Host回溯；包括GameState、523项事件计数、保存路径／IO等。回溯在Host完成后发新的当前world／结构，不发回溯副本 |
| Game+669F0..67FCF BackwardsStages[7]、67FD0历史 | Host ascent房间保存／恢复；Client只接收当前结构、房间配置和ascent显示投影 |
| RoomConfig抽选权重、ItemPool、BossPool | Host生成器；原生地址、移除／黑名单、权重、RNG不上线。必要的具体房间／配置／pill显示结果由第7步资源及Presentation接手，不能整个池标本地后丢掉显示结果 |
| ProceduralItemManager | 生成算法／effects留Host；生成后的ItemConfig、实际Image／图集是资源结果，交资源owner。不能只传seed让Client CreateProceduralItem，不能把整个manager误标为可忽略 |
| EntityFactory指针、模式对象／vtable、Console、DebugRenderer、窗口resize请求与图形surface／strong holder | 本进程owner／调试／资源维护，不发送地址，不依据Host控制台或debug flags开启Client调试玩法 |
| 成就／challenge完成／保存／统计／排行榜／原版Online与Isaac Sync | 不属于状态应用命令。官方ChallengesStuff.cpp的MarkChallengeCompleted会AddChallenge和Save；Run接收绝不调用它，Manager每日／排行榜与原版在线状态不借用 |

Game真实分配：932AB3 `push 680E8h`→operator new→932ACF 调6ED4B0；与SDK大小一致。6ED4B0以步长204D8构造两份回溯，以步长320构造7份BackwardsStageDesc。原生类型表覆盖Game整个范围，但**GameRun只是Run视图**：Level、HUD、PauseScreen等尚未接管的跨域区域在生成表中明确保留未知，不伪称它们不需要同步，也不把SDK空洞当填充。

## 4. 交给 Presentation／第7步的具体值

下面是明确的交接清单，**不是本轮已实现的owner**。缺失任何实际消费者所需来源都应报具名 Gap，不放行 Update／Start／奖励／解锁来补显示。

| 原生区域／消费者 | 需要的结果／边界 |
|---|---|
| Level前缀0..1890F、RoomConfig18910..1A737 | 第7步实际结构、配置身份、stage／dimension／curses；Game+25AC8是额外curse输入，显示用`(Level.curses | temporary | seedForced) & ~seedPrevented`的最终结果。6F55C0／6F5750只作公式证据，不额外调用Getter，且不能再次把已合并的结果与Client种子生产链合并 |
| RoomTransition1B83C..1BAA7与StageTransition | 状态／进度／遮罩／真实versus Sprite及肖像map的最终显示；不运行ChangeRoom／Update。SDK StageTransition只声明20B，1BA98..1BAA7不能吞成填充 |
| PlayerManager1BAA8..1BB83 | 第5步参与者／control group／body／roster／备用身体；不网络化设备、Player*或原版Online管理 |
| ItemOverlay1C034..1D293、1D294..1D4C7、GameOver1D4C8、Leaderboard1D5F8、HUD1D9AC | 各自实际显示Sprite／Image／文本／布局／结果。排行榜展示只用接收到的显示结果，不发请求／提交。1D294区域不能只按旧GenericPopup大小120覆盖234B后宣称完整 |
| PauseScreen2363C..25443 | Host游戏暂停语义与各端菜单交互分开；不可复制选中项、设备／邀请／踢人、选档或本地CompletionWidget进度。暂停下同样要有Host seed／daily／标题显示。SDK PauseScreen为DF4B；其后有其他原生UI owner，不能整片当一个已核PauseScreen |
| Minimap25444..25A6B | 实际最终缓存、探索关系和本地视口派生；不因“由状态决定”就假定Update会重建 |
| Game+25A98 strong pair | Render中的前一画面／hallucination纹理是实际Image资源；地址／counter不能传，不能只传当前文件路径 |
| Game+25AA0／AA4／AA8 | bloom有效分支及最终强度的来源，6FF500写三值，6F7A80消费；采用最终shader值时不重复发送生产倒计时。不要把它们当Game.flags旁边无意义字节 |
| Game+25AB0／AB4／AB8 | hallucination剩余／总计时、Render反馈字节及其最终画面。701450还选Backdrop／用随机／改变实体，不能在Client调用ShowHallucination补显示；Render反馈不能常驻写入Host逻辑 |
| Game+25B04..25B37 | fade-in/out颜色／当前alpha／覆盖分支与最终pass；speed、target和状态切换留Host。6F1640明确写fadeout target，Update 6F6D30可能退出／下一局／保存，不能为绘制运行该FSM |
| Game+25B8C..25BA7 | 外层mode及实现指针属本地／Host生产器；6F7A80在mode4调用虚拟绘制，需具名显示pass，不能照搬对象或vtable |
| Game+25CEC、GenericPrompt25CF0..25E23及25E24..25F37 | 实际Prompt／Sprite／文字／闪烁／存在性由Presentation独立owner；暂停判定不是唯一显示负载 |
| Game+25F58／25F5C | retrovision剩余／总计时。804500先查seed78，再算15帧淡入／淡出并写全局BE7704；需实际最终强度或这对有证据的显示输入。只有seed78布尔并不能覆盖药丸等短暂retrovision |
| Game+66968..66993、669EC | 前者没有具体语义，不以构造写零当padding；后者6F7A80读取并以`clamp(value/90,0,1)`绘制整屏覆盖，Update还触发转场，显示结果须独立承接 |
| 当前challenge／daily参数名称、分数14值＋完成楼层／时间、最终HUD进度图标 | 分别绑定当前run／目标world／正常Host显示批次；缺批次不填默认值。原始ChallengeParam名称不等于所有HUD最终文本；当前Game缓存与Leaderboard冻结副本分开，ScoreSheet缓存不授权重算或调用Show |

历史D0原保留ManagerFrame、paused与两份纯绘制RNG，后收紧为13B两seed草稿；同日[第7步与Random交接](room-random-handoff.md)已进一步改为**5B Manager相位／paused＋逐用途正常已产生alpha／资格**，两个游标／shift／guard均不上线。正常显示批次的world／presentation／资源修订和结果缓存／adapter仍待接线，不由Client全局访问顺序分配样本。Render inline glitch／随机四边形等已有具体结果交接保持，GameRun固定132B负载不含D0或这些Presentation批次。

## 5. 目录与完成口径

原始 `mod/binding/catalog/run-catalog.json` 的 **191条run行**逐行写入GameRun的`catalog_rows`，保留原始全局ordinal、签名、null／空白和两个Fadein／Fadeout重载，不按ID去重。每行理由记录旧stream目录的ReplicatedState／AuthorityOnly／NotReplicated；旧目录保留历史事实，不重写为本轮结论。

同源[191行汇总表](../../../../mod/binding/layouts/08-run-presentation/run-catalog-run-members.md)由普通构建生成：9行field／15行derived／155行authority／12行not_applicable。旧归属93／87／11全部逐行保留。`version.json.catalog_complete_scopes`只声明此共享目录的run域完整性，构建按原始ordinal检查191行，不迫使同文件其它三域提前完成。

明确的状态回映射指向GameRun生成字段；跨域已有／待接入结果指向带来源的`@context`；命令与随机选择归authority；本地纯转换／API对象入口归not_applicable或具名上下文。没有恢复191项通用Getter字典。Ambush.GetNextWaves即使复制rng，也会临时改变RoomConfig权重（官方LuaAmbush.cpp），不能作为正式只读补源。

本轮交付：Run布局／原生与线上单行表／191行回映射／同步职责证据，以及第7／8／10／13步的具体交接。未知区、SDK声明和跨域委派均照实保留；GameRun的“完整”仍为否，不代表整个Game、全部第8步或可玩同步已完成。0—14步不运行测试；第15步唯一统一验收。

当前GameRun状态为**132B固定负载**（不含记录头）：R0身份／时间19B，R1当前全局绘制值48B，R2最终分数与HUD进度判定65B。子布局内嵌复用，不再独立发送第二份相同数据；无负载的RunAmbush／RunChallengeParam／RunSeedEffectNode不创建网络记录。D0／帧头／第7步及Presentation其余负载不包含在这个数字里。

原生UNKNOWN：RunSeeds 3B、RunAmbush 3B、RunChallengeParam 19B、RunDailyChallenge 22B（包含参数的19B）、RunScoreSheet 3B、树节点2B。GameRun视图合计150600B，包含Level／HUD等本轮未展开的跨域委派，不能把这个数字说成整个Game已经判定的私有未知，更不能改归本地消除它。具体每段偏移均由同源表列出，构造写零不补语义。

普通构建记录：首次生成因两个新合成字段缺少`conversions`双端语义声明失败，已补齐；结算消费者复核又纠正SDK旧ScoreSheet偏移及完成楼层／时间遗漏，TimeCounter按同分支递增证据改为帧基准。最终职责复核进一步将daily原始id收窄为1B存在性、移除已由最终HUD图标承接的rerun输入，负载从136B收窄为132B。工作区同期加入的Grid描述生成曾因`array`仅在`emit`局部定义而NameError，已将共用数组格式函数提到模块层；Grid改动照常保留。最终`cmake --build build/repentogon --config Release --target isaac_cs_state -- /m:2 /v:minimal`已生成四份代码表、原生／线上布局和191行汇总，Release／Win32静态库编译／链接成功。没有运行独立校验脚本、单测、probe、harness或游戏；这不是第15步验收。

## 6. 必须避免的失败方式

- 把Host全flags、Ambush控制、14个楼层种子或264624B回溯体当作当前状态复制。
- 把TimeCounter当GameFrame，或用本地计时推进Host暂停／闪烁／retrovision。
- 因“target／RNG／seed”名称自动裁字段，漏掉黑暗第二值、G_FUEL Light或两份已有纯绘制RNG。
- 在Client Render内重新Calculate分数／选池／每日Init／挑战完成，改变随机流或进度。
- 发送树／vector／Image／设备／模式实现的原生地址；复制Host窗口、控制台或菜单操作。
- 用Manager下一日参数替代Game当前daily，或把SDK声明的对象大小吞掉相邻未知owner。
- 把最终HUD判定或显示批次缺失当false／零，把跨域交接当已实施，把源码静态结论当实机PASS。
