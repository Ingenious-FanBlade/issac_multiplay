# 8：Run、Presentation 与 Random

**2026-11-12 HUD代码迁移，版本70**：布局70已迁移315派生字段、311/312普通／特殊分支及435显示驱动，364必要选择保留；纯值显示模块已加入产品。 具体接口、事实及剩余接线见[实施说明](hud-local-implementation.md)。第15步唯一统一验收，未运行测试。

**2026-11-12 HUD本地化决定（仅文档）**：[现行合同](hud-local-presentation.md)将低血周期闪烁、Holy Mantle资格、普通主动／饰品图标、八材料／六法术队列绘制、Drop渐变、基础属性格式化与多人HUD排版交Client纯显示生成。完整心格、三色触发闪烁、特殊图像／选择、Inventory／History顺序、Stat变化相位与概率继续同步；六项基础value仍取Host最终缓存，不恢复Player玩法属性。本文下方及版本58源中冲突的最终缓存必发要求按本合同迁移，原始来源／UNKNOWN／生成事实保留；schema及9—14步运行迁移未实施，本次不改版本、不生成、构建或测试。

**2026-11-11 当前显示合同调整**：[资源本地生成](local-media-presentation.md)替代本页旧文字缓存图、前后切房surface及Host hallucination历史图必发要求。横幅／fortune传文本与必要样式，本地合成；当前Client直接切房；hallucination用本端历史画面。其他实际资源选择、多人HUD／结果归属与音频事件保持，不传原始媒体作兜底。旧来源与版本58生成事实保留，文字／图像结果与依赖布局仍待第9步迁移，本次不改schema或运行代码。

**2026-11-10 第8步正式源已落地，版本58**：[实现与交接](implementation-8-presentation.md)。`presentation.json`74个布局与`random.json`24个Host原生视图登记310—407；地图唯一owner复用300—303，D0固定5B，两用途随机alpha各1—5B，药丸／预览与各控制组HUD纳入固定结果。28条来源／主体／依赖／失效合同、141＋54行目录与单行表同源生成。第9步外层封装及10—14步实际采集／安装继续实施，已有UNKNOWN和来源Gap保持；不表示运行功能已完成，本步不运行测试。下方较早记录保留原时点。

**2026-11-10 显示结果批次与各端多角色HUD方案定稿**：用户确定**“选 B；采用上述归属划分、明确贡献身份，并保持当前缺结果处理规则”**，正式合同见[批次、归属与各端HUD](presentation-result-batches.md)。统一批次按实际owner分组、按依赖共同提交，沿用world／body两序列；History／Stat等详细HUD默认跟随本端控制组，双角色保留各自必要显示，其他参与者保留公共多人HUD。Host原生两槽不构成系统容量；必须补齐各组合法结果来源。必要结果不全不安装／不ACK，不挪用旧身体或其他贡献；正式布局／来源／adapter仍待实施，版本57不变，本次仅文档静态审阅，无构建或测试。

**2026-11-10来源同步职责复核（版本仍57）**：[53项逐来源决定与纠正](presentation-source-synchronization-review.md)、[机器表](presentation-source-synchronization-review.json)。只发必要当前结果／world事件和按需资源；Nightmare最终贡献替代路线／场景帧／Dogma重复输入，音乐最终voice参数替代mask／混音树／ducking／全局pitch。个人后端失败／抢占不广播world Stop，硬件游标不是必需状态。账本修订5仍50视图、UNKNOWN范围不变；旧决定保留，正式布局／缓存／adapter未实施，无构建或测试。

**2026-11-10 音频状态／事件来源补证（静态，版本仍57）**：[Wiki／SDK优先的来源与未决](presentation-audio-sources.md)、[机器目录](presentation-audio-sources.json)登记30项正常来源。补到节流后的实际sample、Options前SFX增益、持续声／并存voice调整与停止、sound460内联pan、两组音乐各7声部及jingle实际启动／ducking；POST回调和官方jingle ID均不能单独证明启动。账本修订4共50类型／视图，纠正MusicStream+0C为后端volume、pitch在+14、+2C非播放游标；五类直属UNKNOWN缩减，原范围保留。另核BD0F50声音改号流，Random修订3共25视图，均不复制随机流。音乐全部Options前writer／逻辑包络、异步结束、seek／重连相位、视频音轨及其余UNKNOWN仍未闭合；正式布局／缓存／Hook／adapter未实施，无构建或测试。

**2026-11-10 资源／屏幕／转场必要来源补齐（静态，版本仍57）**：[证据与来源合同](presentation-resource-screen-sources.md)、[机器目录](presentation-resource-screen-sources.json)。先读本地Wiki和官方SDK／源码，再对缺项补J273静态证据；核定RGBA8读回、内置shader表与固定格式、各屏幕pass、前后surface及versus逐层临时变换、Nightmare条件队列／原版程序道具结果，并识别Game+1D294为BossOverlay。模式4虚拟owner核为原版Deathmatch，按既有隔离排除；Pause+E04只核为密码显示1B，不外推整个后缀。账本修订3增至49类型／视图，Nightmare直属UNKNOWN61→37B；其余孔、Mod shader旁表ABI及派生Image边界保持。正式Presentation布局／缓存／Hook／图形物化／adapter尚未实施，无构建或测试。下方较早来源缺口按各自时点保留。

**2026-11-10 第7步版本57交接**：Room／Level及必要显示固定值已进入唯一源，详见[实现说明](implementation-7-room-level.md)。MapFocus300—301、MapBadgeList302、独立问号资源303已登记，Presentation后续复用这些布局与唯一owner；原生Minimap及其他Presentation布局／实际来源仍待本步实施。下方版本56及更早段落保留原时点。

**2026-11-10 第7步必要显示／资源设计交接**：[补齐设计](room-display-resource-design.md)明确typed不可变配置／图像／surface recipe／内容资源及同world结果批次。Presentation用自有地图adapter消费完整L0—L3、两边框／icons／itemIcons资源、MapFocusResult和MapBadgeList；展开／视口／Options留本地，不调用混合Map Render内的分数重算。水光波纹／世界强制焦点归Room，地图资源／最终震动／转场显示归本步，不重复owner。Home条件为Data.StageId35且Type16；旧Door“type35”已更正。正式注册／source／图形adapter待实施，版本56，无构建或测试。

**2026-11-10 范围调整**：本步只负责原版整局、表现与随机源。第三方 Mod 的 Lua 来源、具名 adapter、状态编码、角色／HUD 回调适配和未决问题已移至[独立后续方案](../mod-adaptation.md)，不属于本步完成条件。下方较早进度记录按原时点保留。

**2026-11-10 第8步Random静态核对完成**：[职责／J273证据](random-synchronization-review.md)、[修订2的24类型／字段视图账本](random-byte-ledger.md)及[54行成员回映射](random-member-map.md)。玩法流／池／运行库引擎留Host，药丸／预览等实际显示结果另承接；补核ItemPool+664饰品流、保存体+218实际vector<bool>和Game+182E4恶魔／天使流。同日第7步交接进一步改为逐用途随机结果，D0从37／13B草稿收紧为5B；两seed／shift／guard均不上线，新增BD0F40音效选样本字段视图，完整结果缓存与adapter尚待接线。UNKNOWN和ABI模型边界保持，未改生产版本56，未构建或测试。以下记录保留各自时点。

**2026-11-10 Presentation静态核对完成（正式布局／接线未实施）**：[同步必要性与具体证据](presentation-synchronization-review.md)、[45类型／连续视图的原生字节账本](presentation-byte-ledger.md)及[141行旧公开目录回映射](presentation-member-map.md)。保留最终心格／实际图像／动态HUD／History顺序和StatHUD三值；previous／threshold留Host，官方星象房第8行保留。地图本地查看与world事实分开；持续声音需要当前状态，一次性声音走事件，Host音量／后端对象不复制。程序图／shader旁表ABI／Pause后缀／转场payload等UNKNOWN及具体来源缺口保持；正式Presentation布局／运行接线未实施，本轮未改生产布局版本、未构建或测试。以下记录保留各自时点。

**2026-11-10 Room环境音交接（待实施）**：[Room续核第4节](room-residual-evidence.md#4-环境声音原64b不发声音不能消失)核定Room+7084为16份环境gain平滑缓存，AAD0E0另有soundID／步长／增益／跨房保留标记；+72FC为延迟sound460及pan生产计时。原缓存／计时留Host，火／水流／雨等实际声音不能删除。正常Host已选voice／sample与最终world逻辑gain／pitch／pan由音频域承接，不发含Options乘数的后端值；baseline须能恢复持续声、停止／clear和重复状态不重播。这里仅补到具名逻辑增益来源，实际sample、voice身份／生命周期与完整意图缓存仍待接线，没有新运行结论或测试。

**2026-11-10 Run 原生归属核对，版本56**：见[同步职责与静态证据](run-synchronization-review.md)、[GameRun生成表](layouts/08-run-presentation/GameRun.md)。`run.json`新增230—238，覆盖Run子对象、Game的Run视图与191条公开目录行。撤销本页旧8.1“整组flags／计数／楼层种子都发”的草稿决定：只留明确的当前显示值、7项种子显示选择、ascent bit48与最终分数缓存；波次控制、生成RNG、历史与回溯留Host。两项黑暗浮点都有直接绘制消费者，均保留；current ColorModifier之外的target／lerp留Host。GameFrame与TimeCounter分开；DailyChallenge的Game当前run副本与Manager菜单／预告副本分开。Presentation的具体资源／缓存／shader交接逐项保留，不把委派区域或未知孔标成本地。新handler均待10／13步接线，Presentation／Random／Mod部分尚未完成，无测试。下文冲突草稿仅保留原时点。

**版本53 Grid展示上下文（待实施）**：[PressurePlate／Door](grid-pressure-door-evidence.md)的计时文字需要Host GameFrame、同world Room.greedWaveTimer与Game.screenShakeOffset(+25A84)，本地字体/字宽和视口照常提供；有符号timer算术/闪烁不按Client墙钟推进。Door主alpha入口/墙色与extra可见分支之外，GridLight仍独立运行，依赖同world维度/当前RoomConfig、stage suffix及目标descriptor和layer3/5实际资源。房间颜色与Light借用必须恢复；缺来源报Gap，不开启Update/Reward/解锁来补显示。

**版本50 Grid展示依赖补充（待实施）**：按[公共复核](grid-common-synchronization-review.md)，J273 Room Render在Room TemporaryEffects含collectible effect481（Dataminer）时开启glitch，原生及官方ANM2Utils均用本地ANM2地址决定source-quad的±90°/flip。GridSprite201已声明1B最终选择，不能把地址作为网络身份或发送指针／玩法RNG；本步须提供同world实际Room/pass开关并与第13步纹理adapter衔接，不能依赖Client本地效果生产链或双重随机。shader实际时间／Manager奇偶、Room颜色／压力板文字等既有展示上下文仍须供应，不因裁剪默认动画名／图层路径而删除。当前只完成公共布局声明，Grid辅助Sprite仍待子类核对。 Grid冠军shader仅消费四个冠军tint参数，后七值不上线；普通颜色和独立shader展示上下文保持。

**版本47布局来源（2026-11-09）**：[第5步正式源／生成](implementation-5-player.md)已完成，EntityPlayer=197，participant／control-group=195／196，Player主动槽=179，条件效果=180，动态Weapon=181—194；物理0—24及条件／固定槽／预算元数据以唯一源和生成表为准。以下版本46／草稿／待生成表述保留原时点；本步骤的运行接线仍未实施，不能因布局已生成就提前接收、采集、安装或宣称验收通过。

**第5步统一回映射交接（2026-11-09，未实现）**：[逐行决定与显示来源合同](player-member-map.md#4-真实显示查询的最终决定合同)不恢复556项通用查询字典。只有实际原生绘制消费者使用的bool/int/StateRef结果由具名Presentation adapter承接，观察正常Host显示调用已产生结果，不额外调用getter／Lua／随机；本步仍须完成固定最终缓存和批次布局，绑定对应subject/use/shape、拓扑及状态／资源版本。缺席不填零，不把旧显示批次贴到新身体。Player+268C原版通信气泡／表情／打字对象不进入C/S；+1FF8/+2000真实Image与最终HUD仍由本步各自owner承接。

**第5步P6—P8交接（2026-11-09，未实现）**：[设计及具体消费者](5-player-p6-p8.md)将原始History／16索引／form计数留Host；本步必须同步82FA10实际两个HistoryHUD槽的owner／有序visual_id／Image和真实PlayerHUD+6A4库存图像列表，保持负ID、633及88000 shader分支、四人多身体的选择／布局，不按counts重排。还核到97C300→966680→9BAC90在Render内重算分数并涉及随机，9679FA实际读取Game+25C90总分；本步捕获既有生产完成后的最终缓存并隔离该用途重算，不以“Render可达”为由恢复sub图／全部心量生产输入或额外Host重算。703四槽只作Host恢复，实际主动显示不解析它们，P4 VarData保持。Player两Image strong／palette／动态HUD owner仍须本步补齐；P8的残影及当前记录＋1C50 Sprite由Player owner处理，不重复维护。版本仍46，没有新代码或测试。

**2026-11-10 第7步两域目录交接**：[RoomSave／Exploration核对](room-save-exploration-synchronization-review.md)要求Presentation唯一承接实际地图图标／边框资源、最终震动、world显示限制，以及966300产生的最多7项有序道具／诅咒徽标。MapBadgeList为`u8 count + i8[count]`，1—8B草稿；不能用Host地图展开状态、四个揭图bool或Sprite最后一帧替代整列。正式结果缓存／布局／资源与adapter待实施，版本56，无测试。

状态：Run、Presentation与Random的正式源／生成表及191＋141＋54行回映射已经落实；第9步批次／贡献／事件封装与第10—14步正式运行接线尚待完成。音频未闭合来源及其他UNKNOWN照实保留，Mod适配在当前范围外。上级：[总计划](README.md)。

## 目标

完成原版整局（Game 中 Level 之后的部分）、表现（HUD、小地图、音乐音效等）和随机源的字节归属。

## 输入事实

### Game 中的相关位置

`Game` 的 SDK 声明为 0x680E8 字节，79 个具名字段。Level 部分见第 7 步，其余相关位置如下：

| 位置 | 内容 |
| --- | --- |
| 0x18910 | RoomConfig |
| 0x1A740 | ItemPool，0x830 字节 |
| 0x1AF70 | BossPool |
| 0x1B83C／0x1BA78 | RoomTransition／StageTransition |
| 0x1BAA8 | PlayerManager |
| 0x1BB84 | Seeds，0x5C 字节 |
| 0x1C034 | ItemOverlay |
| 0x1D4C8 | GameOver |
| 0x1D9AC | HUD，0x5C90 字节 |
| 0x2363C | PauseScreen |
| 0x25444 | Minimap，0x628 字节 |
| 0x25A70 | 帧计数 `_frameCount`，由帧头 game_frame 承载（见 8.2） |
| 0x25A84 | 屏幕震动偏移 |
| 0x25AC0 | `_gameStateFlags`（u64） |
| 0x25ACC—0x25AEC | 恶魔房、宝箱房、星象房、捐款等计数 |
| 0x25AFC | challenge |
| 0x25B40 | Ambush，0x4C 字节 |
| 0x25BA8 | DailyChallenge |
| 0x25F40 | difficulty |
| 0x25F64 | `_glowingHourglassStates[2]`，约 260 KB，整局状态的两份保存 |
| 0x66920—0x66950 | 三份 ColorModState |
| 0x6699C—0x669A8 | 闪电、眩晕强度 |
| 0x669F0 | BackwardsStages |
| 0x67FD4 | Console |

`Seeds` 包含自定义局标记、开局种子、RNG、14 个楼层种子、玩家初始种子和种子效果数。

### 绘制输入（Game 之外）

Pickup、Familiar 等的原生 Render 还读写 Game 之外的四项全局值。现有实现把它们作为绘制输入复制：

| 位置 | 内容 | 证据 |
| --- | --- | --- |
| `Manager+0x4A8B8` | `_framecount`，u32。Manager 每次 Update 加 1，低位决定是否走 Odd 路 | `Manager.zhl`（[Entity 来源研究](../tx-04-19/entity-source-research.md)第 50 行）；`0x92E8B1` 检查低位（[ST2 笔记](../../research/tx-04-08-st2-agent-notes.md)第 9 行） |
| `Game::IsPaused()` | 暂停判断，VA `0x6F92A0`。Render 的 subframe 为“暂停时 0，否则 `(_framecount+1)%2`” | 同上研究第 52—54 行 |
| VA `0xBD0CA4`（RVA `0x7D0CA4`） | Pickup 绘制用的全局 inline xorshift，4×u32：种子加三个 shift | Pickup Render `0x6E35D0` 读写（[应用合同](../tx-04-19/entity-application-contracts.md)第 130、260 行） |
| VA `0xBD0674`（RVA `0x7D0674`） | Familiar 绘制用的全局 inline xorshift，4×u32 | Familiar Render `0x61E360` 在 `0x61E533` 读、`0x61E5A3` 写回（[Entity 原生静态研究](../tx-04-19/entity-native-static.md)第 73 行） |

现有处理：
- **采集**：Host 在同一普通采集窗口复制 manager_frame、game_frame、subframe、paused 和两份 RNG，不推进它们（`mod/src/game/world_sources/lua_entity.cpp:368-380`）。它们作为 `application_draw` 挂在 Player 记录上，属于 World 层（`mod/src/replica/stream_partition.cpp:36`）。
- **校验**：版本与同窗标记、subframe 与 paused 和 manager_frame 一致、种子不为 0、shift 不超过 31（`mod/src/game/client_entity_class_model.cpp:118-124`）。
- **安装**：draw 的 FrameScope 开始时换入 Host 值（`mod/src/game/client_entity_draw_native.cpp:238-241`），结束时恢复 Client 原值（同文件 136-145 行）。Client 自己的值不会被常驻改写。

### 公开成员（`stream-catalog.json`）

| 域 | 成员数 | 组成 | 归属分布 |
| --- | --- | --- | --- |
| run | 191 | Game 122、Seeds 28、ChallengeParam 28、Ambush 12、DailyChallenge 1 | ReplicatedState 93、AuthorityOnly 87、NotReplicated 11 |
| presentation | 141 | HUD 31、MusicManager 28、Font 16、ColorModifier 12、GenericPrompt 11、SFXManager 10 等 | ReplicatedState 55、AuthorityOnly 70 |
| random | 54 | ItemPool 37、RNG 17 | 全部 AuthorityOnly 或 NotReplicated |

### 现有处理

- **Random 热路径**：已是 AuthorityOnly，不轮询（`mod/src/game/world_sources/observation.cpp:120-122`）；冷源附加原生随机流。
- **Game之外的owner**：Music与SoundEffects分别内嵌于Manager+29DF8／+2A160。Font有Manager九个内嵌实例及Lua自建实例，不能视为唯一全局单例；来源和生命周期须逐owner绑定，详见[字节核对](presentation-synchronization-review.md)。

## 设计

### 8.1 Run

- **现行决定（版本56）**：具体字节与职责以[Run核对](run-synchronization-review.md)和`state/run.json`为准。challenge、difficulty、TimeCounter、当前daily存在性、最终震动／两黑暗值／current颜色／闪电／眩晕、最终分数缓存等有明确显示用途；`_gameStateFlags`只传ascent bit48，Seeds只传开局元数据和7项显示选择。HUD进度图标从真实Host显示批次取得，不能借此写访客资格。
- **Host生产输入**：各类交易／访问／伤害／捐款历史、Ambush控制与两份RNG、Seeds生成RNG／14个stage种子／playerInitSeed、ChallengeParam规则及每日生成参数都留Host。需要显示的具体结果由当前世界、资源或Presentation缓存承接，不能用旧公开目录原值相等恢复同步。
- **大块本地或权威区域**：
  - `_glowingHourglassStates` 是 Host 回溯用的完整保存，归权威，不发送。
  - Console、DebugRenderer、EntityFactory 指针等归本地。
- **布局与块**：GameRun是Game原生对象的Run视图；子对象各有独立字节表，Run视图只嵌套实际产生负载的子布局。无负载Host对象不创建空网络记录。Level／UI等未接管范围在Run视图中明确未知并列具名交接，不冒充整个Game完整；正式采集按已核偏移复制，不读取整块Game。

### 8.2 Presentation

**第5步P3—P5设计补充（2026-11-09，未实现）**：见[具体来源与裁剪](5-player-p3-p5.md)。P3不给voided／innate管理时限／crafting来源池，crafting结果身份与已建立的HUD显示列表／图像／名称仍须由本步具体owner同步，不能从库存排序猜获得顺序或放行832C10整个Update。原生通用库存getter含进度／Online／局部RNG分支，P3原值不等于全部getter已可安全调用；本步沿实际显示消费者补核最终缓存／合法来源，未闭合时报具名Gap，不回退Lua／额外抽取。P4六值与P2／P3／P8／Run依赖保持；P5五种恒零类的充能输入已裁，不能由HUD旧检查恢复。布局版本仍46，本轮没有新HUD代码或测试。

**第5步P0—P2设计补充（2026-11-09，尚未实现）**：[具体消费者与职责](5-player-p0-p2.md)。P1不重复Player六项最终stats，属性栏由实际StatHUD五值缓存（value／delta／previous／threshold／countdown）、真实Player身份与资源承接；不调用83CFD0／83C860／83D210补值。P2保留83B160特殊健康显示需要的原始健康值，普通24心／动态PlayerHUD最终缓存仍须安装，EternalHearts与BoneHeartsMask的原存储不传。不能声称这些缓存会由原生HUD Update自然重建；已有单根缓存安装是窄先例，全部参与者／主备及真实布局owner仍归本步完成。P0另保留758850在Render内直接读取的五计数，不能只靠最终条Sprite删除它们。布局版本仍46，没有测试。

- **HUD 与小地图**：以[逐项决定](presentation-synchronization-review.md#2-最终同步决定)为准。最终心格／实际图像／有序列表不能靠安装Player后自然获得；StatHUD只发value／delta／countdown，官方星象房行保留。地图本地展开／holdTime／视口不跟随Host输入，world标记／显示资格和最终震动另同步。965DB0含设备／Online／随机，禁止放行；965D60→9635B0缓存构建只在具体确定分支和来源完整时使用，挑战32须有Host最终标记。每项派生都有具体路径，不因“由状态决定”一概判本地。
- **批次与各端主体**：采用[已定稿方案B](presentation-result-batches.md)，producer、subject及安装owner分别登记。各端History／Stat等详细主体取自身控制组，公共心量／主动等继续覆盖其他参与者；原生槽0／1不是Peer0／1。每份结果绑定真实来源／有效依赖，多layer／pass／递归贡献保留独立身份与顺序；分组不放宽共同提交及缺结果不安装／不ACK的规则。
- **Sprite资源与颜色**：版本32按[动画／资源／颜色职责复核](animation-resource-synchronization-review.md)保留资源选择、实际Image／shader和最终完整ColorMod；不传ColorParams时间线或Host加载维护字节。资源存在性由可空文件资源号表示，等待／失败／成功由Client资源owner维护；路径变化不能替代实际Image变化。主／叠加事件掩码不上线，一次性音画仍由事件通道处理。本步尚未核定的HUD／UI资源owner须核实际绘制消费者，不把Entity四层直接调用的阴性检索外推为全域覆盖；is_playing／anim_frame／playback_speed的官方绘制插值依赖继续保留。
- **ColorModState**：版本56核定只发`current`六值；target／lerp与开关属于Host生产输入。两个黑暗浮点仍因直接绘制依赖保留。具体证据见[Run核对](run-synchronization-review.md)。
- **音乐音效**：以[意图／事件／本地后端边界](presentation-synchronization-review.md#7-音频意图事件与本地后端)为准。一次性声音走已选sample事件；持续loop／ambient及音乐crossfade／jingle还须当前voice状态供baseline／重连恢复，重复状态不重播。后端音量含Host Options乘数，不能直接下发或在静音时反除；Client保留自身音量／静音。FrameDelay是重复播放节流，不是启动延迟。
- **Game 帧计数**：`Game+0x25A70` 由 world 帧头的 game_frame 承载，也就是 Client 的展示时钟（第 13 步 13.6），不进入 Run 块，也不进入下面的绘制输入块。

#### 8.2.1 绘制输入块 D0与逐用途随机结果

2026-11-10按[第7步与Random交接](room-random-handoff.md)修订：D0仅5B，原37B／13B全局游标草稿已替代；现已注册为版本58的PresentationDrawClock356。具体原生结果点、资格和批次合同见交接第3节。

| +0 u32 manager_frame（4B） | +4 u8 paused（1B） |
|---|---|

- GameFrame在world帧头；Manager每次Update相位与Game模拟帧不能用固定差换算，保留独立值。subframe仍按paused为1取0，否则(manager_frame+1)%2派生。
- Pickup的6E3805已产生alt pedestal alpha、Familiar的61E604已产生栈ColorMod.alpha，按具名subject/use/shape/pass/调用身份发送资格及f32结果，不发送BD0CA4／BD0674 seed、shift或guard，不推进Client这两流。
- 标量记录`state:u8`，0为正常Host已确认无随机覆盖；1随后有alpha:f32，最大5B。外层身份／容器／批次另计，多个pass／递归不能折叠为一份最后值。缺源／未产生不填0或不适用。
- 结果绑定真实world state revision、presentation revision及资源版本，不能从普通窗口seed或仅GameFrame猜消费相位。只捕获正常Host结果，不额外Render／抽取；Authority tick不等显示批次。
- Client在两个具名随机／资格分支使用结果；Familiar本地颜色栈对称push/pop，Pickup临时Sprite与CPU消费后恢复。重复展示复用同一不可变批次，换房／use退休撤销，不依赖本地排序分配随机样本。
- 正式布局为356—358，结果内容预算分别5B／5B／5B；外层贡献身份／批次封装、实际缓存／source／writer仍待第9/10/12/13步，不将旧草稿当可接受报文。

### 8.3 Random

以[Random职责核对](random-synchronization-review.md)和[逐字节表](random-byte-ledger.md)为准。玩法RNG、ItemPool／BossPool及恢复体、全局Random／独立MT／CRT／Lua引擎均留Host，不创建Random网络记录。旧54行回映射为51行Host与3行实际显示结果交接；后者不是原Getter或全量参数字典。药丸必须保留按身体／槽位裁决的揭示、effect和实际名称／图像，Loot预览保留实际有序显示，不在Client抽取或解引用未知RNG*。8.2的两个具名纯绘制用途取已产生alpha／资格，游标也不上线；正式结果批次／来源／adapter仍待接线。

### 8.4 当前范围

正式同步只采集原版所需的原生状态和具名最终显示结果，不加载第三方 Mod adapter。原 Mod 域设计已迁至[后续方案](../mod-adaptation.md)，本步无需实现其编码器或注册接口；Lua 只用于目录／语义核对和 E2E 对照。

## 改动范围

- 布局源 `state/run.json`、`state/presentation.json`，其中包括绘制输入块 D0。
- 删除 Player 记录上的 `application_draw`，绘制安装改读影子状态中的 D0（第 13 步 13.6）。
- Run 和 Presentation 的 Client 写入函数，改为由影子状态输入。

## 失败方式

- **回溯状态外泄**：`_glowingHourglassStates` 被误归为状态，单帧发送 260 KB；或者 Client 写入后破坏本地回溯。
- **派生判断不实**：HUD 中的项被判为派生，Client 实际不会重建，显示缺失。
- **全局单例时机错误**：MusicManager 等全局单例在会话外或主线程窗口外被读取。
- **绘制输入错窗**：D0 与帧头 game_frame 来自不同的采集窗口，subframe 与帧计数相位错开，Pickup、Familiar 的绘制效果与 Host 不一致。
- **绘制输入常驻写入**：D0 直接写进 Client 的全局值而不是展示期间借用，或者展示结束后没有恢复，污染 Client 本地的 Render 状态。
- **随机绘制结果归属错误**：误发两个旧seed／shift／guard，或按本地顺序复演；将旧alpha／缺源当本次结果，忽略pass／递归／world与资源修订。

## 完成内容

- **字节归属与表格**：Run、Presentation、Random的原生范围和UNKNOWN已登记生产源，带证据；单行表由生成器产出，未声称全部字节语义已知。
- **回映射**：三个域的公开成员全部有回映射：
  - run 的 93 个、presentation 的 55 个 `ReplicatedState` 成员，各自映射到布局字段、某组字段的派生，或者说明为何归 Host 权威或不适用；
  - run 的 87 个、presentation 的 70 个 `AuthorityOnly` 成员，以及random的54个成员逐行审阅；Random已区分51条Host及3条实际显示结果交接，具名绘制实例单独登记，不按普通RNG归属丢掉画面；

## 未决问题

- HUD（0x5C90）和Minimap（0x628）的原生范围、正式`state/presentation.json`及生成表已登记；运行接线尚未实施。资源／屏幕／转场按[来源补证](presentation-resource-screen-sources.md)使用正常已完成内容／完整recipe、实际内置shader和最终有序贡献。具体缓存／Hook／图形物化、全部控制组合法HUD来源仍待落实；Mod shader旁表ABI、Pause其余后缀、Nightmare剩余孔与派生Image边界仍UNKNOWN。[音频补证](presentation-audio-sources.md)已核sample／并存voice／音乐声部和jingle分支，全部音乐Options前writer／逻辑包络、world用途退休、segment／seek恢复和视频音轨仍未闭合；不以正式值布局或静态来源已登记宣称运行完成。

**版本39 Player／HUD装备依赖**：[装备与Plus复核](player-equipment-plus-synchronization-review.md)核定PlayerHUD::RenderActiveItem835B40直接／间接使用四主动槽六值、TemporaryEffects分类／count、ItemConfig、Player角色／库存及Game challenge；还在836093读首个slot0／1 Weapon的type12分支。Player758850的进度条依真实slot7／slot6公式与各内嵌Sprite，角色27另读704效果cooldown及配置cooldown；不得把有最终costume当作查询已经可用。HUD资源／image/bookImage／flash／布局等未核owner仍须本步准备，不放行832C10整个PlayerHUD Update刷新缓存。官方1072／1073等第三方回调结果及HUD回调适配已归[Mod后续方案](../mod-adaptation.md)，不作为当前原版实现条件；Client仍不能调用玩法Lua补齐显示。Plus公共覆盖由Scene415350的水面／反射分派实际消费，主体Room／Player依赖仍在；本轮不宣称完整HUD实现或测试通过。
