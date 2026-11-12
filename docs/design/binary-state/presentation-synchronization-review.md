# 第8步 Presentation：原生字节与同步职责核对

**2026-11-12 已被取代**：同步职责结论按需求清单改判，Client 原生显示能产生的项不再同步。原文、事实与 UNKNOWN 保留。见[修改方案](client-display-delegation.md)。

**2026-11-12 HUD代码迁移，版本70**：现行HUD布局70及纯值模块已落地；此前缓存必发结论由HUD本地合同覆盖，原始研究记录保持。 具体接口、事实及剩余接线见[实施说明](hud-local-implementation.md)。第15步唯一统一验收，未运行测试。

**2026-11-12 HUD同步职责修订，仅文档**：[现行HUD本地生成合同](hud-local-presentation.md)取代下方低血强度、护盾资格、普通图标／中心、材料／队列绘制、Drop逐帧结果与Host布局缓存必发要求。Client复用已提交状态／资源／时钟；基础属性仅本地格式化Host最终value。完整心格、三色／充能闪烁、特殊图像、有序Inventory／History、Stat变化相位／概率及必要显示角色／顺序继续同步。当前JSON来源目录、原生账本、schema及已生成表保持迁移前事实，后续按第9—14步落实，不因本文修改升级证据或宣称已实现；本次无生成、构建或测试。

**2026-11-11 当前显示职责调整**：[本地生成合同](local-media-presentation.md)取代文字缓存图／前后切房surface／Host hallucination历史图必发：传文字与样式，Client直接切房并本地生成反馈；图像／音频文件不作默认网络正文。下方原生字节、引用和UNKNOWN仍是事实，旧网络归属按本合同迁移，不能由Image存在推导原始媒体必发。本次不改版本60生产源或已生成表。

**2026-11-10 批次及HUD主体定稿**：[正式方案B](presentation-result-batches.md)确定统一批次／实际owner分组／按依赖共同提交、明确贡献身份、保留现行Gap／Wait处理。各端History／Stat等详细HUD默认跟随本端控制组，其他参与者的公共多人HUD继续展示；Jacob／Esau等保留组内双角色语义。下文两个原生History／Stat槽只描述单个原生owner容量，不代表只支持两参与者或照搬Host主体；全部组的合法最终结果来源仍待实施。版本57不变，仅文档，无构建或测试。

**2026-11-10同步职责修订5**：[资源／屏幕／转场与音频53项复核](presentation-source-synchronization-review.md)及[逐来源决定](presentation-source-synchronization-review.json)明确只发必要当前结果／事件和按需资源。最终Nightmare贡献替代路线／场景帧／Dogma等重复输入；最终voice gain替代layer mask／混音树／ducking，voice pitch替代全局pitch。world意图与后端失败分开，个人设备／抢占不广播Stop，精确硬件游标不作必需world状态。来源目录修订2、字节账本修订5仍50视图且UNKNOWN不变；旧决定保留JSON历史，生产版本57，未构建或测试。

**2026-11-10 音频来源修订4**：[Wiki／SDK优先的具体来源、纠错与未决](presentation-audio-sources.md)、[机器目录](presentation-audio-sources.json)核定正常sample选择／Start提交、持续及并存voice更新／停止、sound460内联pan、两音乐组与jingle有效分支。MusicStream+0C为后端volume、+14为pitch、+2C为Update计数；Music+360是背景ducking。账本修订4共50类型／视图，MusicStream直属UNKNOWN24→4B、MusicEntry72→2B、SoundSample16→4B、SoundEffect30→2B、SoundEffects4→0B，新增分层定义34h；旧错误／范围存JSON历史。音乐全部Options前writer及逻辑包络、异步结束、seek／重连相位、视频音轨和其余孔仍未知，不能把本轮补证当完整音频采集已闭合。版本57，正式布局／缓存／Hook／adapter未实施，无构建或测试。

**2026-11-10 资源／屏幕／转场来源修订3**：[本地Wiki→SDK／源码→J273静态补证](presentation-resource-screen-sources.md)及[机器来源目录](presentation-resource-screen-sources.json)落实当前原版必要来源。程序图按实际已完成内容承接，RGBA8与Image Wiki浮点描述的差异已说明；内置shader表AACAB0和主要固定pass格式已核，不再要求读Mod STL旁表作为原版来源。Nightmare条件队列20B头／32B元素、逐用途实际Sprite及程序道具分支已核；Game+1D294识别为BossOverlay，模式4识别为原版Deathmatch并排除，Pause+E04只拆出密码显示1B。账本增49类型／视图，Nightmare直属未知61→37B；旧未知／判断保留JSON修订历史。生产版本57不变，正式布局／Hook／缓存／图形物化／adapter仍未实施，无构建或测试。下述初版未核表述以本段及专门来源文档的具体范围修订。

**2026-11-10 第7步显示／资源设计补齐**：[本轮设计](room-display-resource-design.md)替代下文“资源内容／地图adapter设计待补”的旧范围，具体实现仍待接线。地图采用完整事实＋本地纯合成，不复制Host裁剪FBO；Home隐藏读Data.StageId35且Type16。资源按不可变typed payload管理，程序图可用完整有序recipe或精确像素；SourceQuad保留pixel／UV空间，RGBA8读回依据及图形状态保护已明示。MapFocusResult／MapBadgeList／最终震动及实际资源由本步唯一拥有，水光波纹归Room；具体source／GPU ABI／正式布局未完成，除下述8B问号补证外其余UNKNOWN保持，版本56，无测试。

同轮补证Minimap+614..61B为独立Lost问号ImageStrongPair：965BDE加载文件、965BFF..965C0C赋入+614，966680直接绘制，既有QuestionImage适配亦使用它。必须承接实际图像资源，不能只发icons／itemIcons；[账本修订2](presentation-byte-ledger.md)将本8B从UNKNOWN转资源，Minimap直属未知23→15B，旧记录留JSON，其余未知不变。

**2026-11-10 RoomSave／Exploration目录交接**：[两域核对](room-save-exploration-synchronization-review.md#5-exploration不另建一套地图真值)明确地图徽标不等于保存图标或揭图flag：966300按有效物品／诅咒生成最多7项有序icons／curses，逐项SetFrame，Sprite最终帧不能表示整列。Presentation唯一持有MapBadgeList草稿`u8 count + i8[count]`（1—8B），负ID−1..−9为curses、非负仅0/1/2/4/5；原优先级及7项上限保持，位置／间隔本地派生。实际资源／可变外观、边框、震动与world限制继续承接；不增加原始Level四揭图bool或Host展开输入。正式缓存／布局／reader／writer未实施，UNKNOWN不变，本轮无测试。

**2026-11-10 RoomDescriptor显示交接**：[专门核对](room-descriptor-synchronization-review.md)补D1当前world的Error→ModelingClay颜色谓词u8 bool（1B）：官方ASMHud的>0在目标版恒真，完整效果ID无须发。实际Image仍来自Host缓存，具名颜色adapter使用D1及真实Player ModelingClay/颜色上下文，不伪造数值Getter、重算History或读取Client本地SpawnSeed。L2保存地图结果只发16类mask及必要帧，存在3..12B，楼梯/Portal为位，challenge32仍取Host已选位置；这些是自有地图数据，不是Restore保存体。另补HasWater第五位及所有模式Boss匹配的Pit/TrapDoor Light上下文。正式来源、资源与adapter待接线，本轮无测试。

2026-11-10；目标为官方 REPENTOGON 1.1.3／v1.9.7.12.J273 Win32。完成本轮 **Presentation 的静态字节登记、同步必要性核对和旧公开目录回映射**；这不是第8步整体或正式同步实现完成。设计账本为 [JSON](presentation-byte-ledger.json)／[原生单行表](presentation-byte-ledger.md)，公开成员为 [JSON](presentation-member-map.json)／[逐行表](presentation-member-map.md)。本轮不修改生产布局版本，不接入 `state/presentation.json`、Hook、reader、writer，不构建、不运行游戏或测试。UNKNOWN和来源缺口仍明确保留。

## 1. 资料顺序与证据等级

先查本地原版 Wiki：`F:/projects_isaac/IsaacDocs/docs/{HUD,MusicManager,SFXManager,Font,FontRenderSettings}.md`；再查官方 REPENTOGON Wiki：`F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/{HUD,PlayerHUD,PlayerHUDHeart,HistoryHUD,HistoryHUDItem,Minimap,MusicManager,ColorModifier,GenericPrompt,NightmareScene,FXParams,renderer/Image,menus/PauseMenu}.md`。官方在线 [HUD](https://repentogon.com/HUD.html)／[Minimap](https://repentogon.com/Minimap.html)仅作语义旁证，不拿当前网站代替本地1.1.3 ABI。

随后读官方 `libzhl/functions/` 同名结构，以及 `repentogon/LuaInterfaces/HUD/*.cpp`、`LuaMusicManager.cpp`、`LuaGenericPrompt.cpp`、`LuaNightmareScene.cpp`、`LuaRender.cpp`、`Patches/{Anm2Extras,MinimapPatches,ImagePatches,InterpolV2}.cpp`、`Patches/ASMPatches/ASMRender.cpp`、`Patches/VanillaTweaks.cpp`。只有公开资料不能回答具体字节或副作用时才查 J273 IDB；本文地址均为 **VA，RVA=VA−0x400000**，未改动或保存IDB。

`已核`表示有相应指令或官方实现支持；`SDK声明`保留原等级；`推断`没有升级成实测；未知间隙逐段列为 `未知`。账本登记整个声明范围，不等于每个字节语义已知，也不等于每个字段都发送。表中“本地／Host”的外层对象不因嵌套ANM2通用表含“状态”而恢复传输。已有 Run 字节及分数结论见 [Run核对](run-synchronization-review.md)，不重复建立writer。

## 2. 最终同步决定

| 范围 | 需要同步的内容 | 不需要或不应同步的内容 |
|---|---|---|
| PlayerHUD | 实际身体及必要显示角色／顺序、24个最终心格、三色触发闪烁、主动／饰品特殊显示及充能闪烁、四Pocket专用结果、有序inventory／实际crafting输出；最小critical_health与实际消费Drop驱动／锚点另按新合同登记 | 低血强度、HolyMantle资格、普通主动／饰品图标与中心、八材料／六法术绘制、逐帧Drop比例及排版由Client纯显示生成；Player*/HUD*/control、设备键、dirty、比较历史、按键重放、旧绘制scratch不上线 |
| StatHUD | 实际身体选择、当前可绘0—8行的value／delta／countdown、合并概率方式、必要资源及Host星象房资格 | 六项基础value的格式／字体／排版本地；previous、threshold、第9行、RecomputeStats、概率生产输入与Client进度查询不上线，不恢复Player六项玩法属性 |
| HUD消息／fortune | 11个消息物理槽各自当前存在性、streak资源／姿态及实际文字图像；fortune Sprite与独立文字图像；需要语义文本的声明UI另保留有界文字 | Show命令重放、原始指针、自动隐藏生产FSM、旧position/scale、字体／FBO／图形批次对象；字符串或路径不足以替代实际文字图像 |
| HistoryHUD | 两个实际显示owner和有序visual_id＋实际Image；保留负ID、633及0x88000 shader分支 | 原始获得史、时间／池／层、16索引、按库存count重排；视口位置、列数、可见前缀由本地布局计算 |
| Boss HUD | 填充值（含负值隐藏）、闪色相位、当前显示图像选择、特殊计数字段、具名最终位置／绘制分支 | 伤害生产过程、Boss AI、原版Online选择／设备身份；不能仅从当前Entity HP重建全部显示缓存 |
| Minimap | 同world描述／探索／保存图标与有效curses、实际特殊标记结果、world强制显示资格、当前最终震动偏移、必要Sprite／图层外观 | Host的Map按住/释放历史、holdTime、窗口／分辨率、FBO、shakeDuration随机生产倒计时、原版Online输入；不直接运行Minimap::Update |
| 颜色／屏幕效果 | Game当前ColorModState六值、当前闪电／眩晕、黑暗两输入、实际bloom／retrovision／fade／hallucination／整屏覆盖的最终有序pass参数与必要图像 | target／lerp／dizzyTarget／changeSpeed、窗口resize、原生feedback字节、计时／退出／转场FSM、ShowHallucination、玩法随机源 |
| ItemOverlay | 最终可见性、overlay身份、实际主Sprite、MegaMush Sprite与真实身体锚点 | WAITING倒计时、47份静态配置表、Show／Update重放、SleepFillHP／解锁／切房 |
| Music | 所有有逻辑用途的并存声部的实际资源、最终Options前gain／pitch／必要pan／loop／world pause及存续；world选曲／重触发／退休事件；必要逻辑段／时点 | mask／mix_mode／mul／层权重／组current／ducking与最终gain重复下发；全局pitch与每voice pitch重复；buffer空槽、Getter ID、queued／目标／计时；后端／个人Options／设备与硬件游标 |
| SFX | 一次性world Start的实际sample与去重身份；持续用途当前参数／存续；已有one-shot的必要后续参数与明确world Stop | RNG／节流／音频设备、后端playing／pending／loaded／偷占／个人失败；每帧SetAmbientSound或后端重试当新Start、晚加入重播历史one-shot |
| Font／Image／shader | 资源身份、实际选中图像、独立程序图像的内容或完整recipe、shader选择和布局定义；特定文字recipe的必要参数 | Font*/Image*/GL/FBO/引用计数、渲染批次、仅凭Image名字猜身份；不每帧发字体度量 |
| Prompt／暂停／死亡／过渡 | 共享world Prompt的最终纸张/文字/选择外观、死亡界面最终显示结果、过渡最终pass和肖像结果；Host暂停语义在D0 | 本地菜单选择／确认提交、Host选项、邀请／踢人／Online面板、成就解锁／保存／排行榜请求；展示不执行Game::End或ChangeRoom |

**有Getter或可写不等于应同步。** 旧目录中的55条`ReplicatedState`已经逐行改判；包括Font度量、History时间、ItemOverlay.Delay、GenericPrompt.SubmittedSelection等，均不因旧标签而保留原值复制。旧141行里没有Minimap、HUDMessage及NightmareScene，不能据此漏掉其实际显示负载；补充范围单独登记。

## 3. PlayerHUD、属性栏与身体归属

### 3.1 PlayerHUD的真实生产与消费

- SDK `PlayerHUD=0x6E0`，HUD内8份，另有Player+20A8的动态strong owner。`832C10`写24心、四份0x1C主动显示desc、两份0x18饰品desc、四份0x11C Pocket显示体以及+6A4列表／+6B4合成图像；`97AE90`遍历8份调用它。**完整PostUpdate不是纯显示维护。** 它还调用History Recompute和StatHUD更新，不能在Client放行。
- `832800`生产最终心格；`834930`消费可见／eternal／golden／fade／flash以及实际动画名，另有Player角色、curse、额外生命和特殊健康绘制分支。第5步已保留的Player P2依赖继续有效；不能用24个心格替代全部特殊健康显示，也不能为刷新心格恢复被裁的原始健康字段。
- `834930`先判断每格visible，再读其余显示值。隐藏格只发送visible=false，不读取残留动画指针；overlay名称只在实际eternal分支需要时读取。`412A90/412AC0`分别是cos/sin封装，淡出色、肖像旋转和屏幕振荡的这类计算是确定数学，不新增RNG状态或逐顶点负载；应按同world显示相位与实际资源参数重建。
- `PlayerHUD+0A`是低血闪隐强度，`832C10`依据生命和GameFrame%45产生；+0C/+0D/+0E是三种心闪计时；+6A0是effect313（HolyMantle）的最终显示资格，`834930`直接消费。**2026-11-12职责调整**：低血周期按最小健康判定／GameFrame、护盾按P8禁用及效果条目存在性由Client纯值adapter生成，不再重复下发这两份缓存；三色触发计时及心格闪烁／消退仍保留。原生读取事实不因本地化删除，不能放行完整HUD Update。
- `835B40`除Player P4六值外消费实际itemImage/bookImage及charge flash；`837720`饰品使用最终图像与中心，且在非golden分支判断desc+4是否非零来选择额外colorize。SDK的`slot`名字不能作为丢弃理由：只发这项1B最终`tint_override`，其整数更新比较值不发。`839370`／`839A80`消费+6A4按12B步长的有序 `{visual_id, Image strong}`；后者还用+6B4结果图像。实际图像结果不能被重新按ID查ItemConfig／抽取／排序取代。
- `832C10`末尾读Drop动作后产生+6BC显示比例，`97EB40`用其淡化实际身体的道具显示。**2026-11-12职责调整**：Client按Host实际消费Drop、有效显示更新锚点和初始化相位本地推进渐变，不再逐帧传该浮点结果；不把本端物理键或到包／渲染次数代替权威消费及更新次数。统一输入scheduler的Host职责不变。`839A80`对+6C0..6CF累计包围框；该scratch在正确绘制初始化／pass中重建，不发旧残值。

### 3.2 StatHUD从“五值全发”收紧为绘制三值

`StatHUD+114`起有两组0xCC：Player*4B＋10×0x14；每项为value、delta、previous、threshold、countdown。`83CFD0`、`83D210`维护五值；`83C9B0`的实际读取为 **value（+0）、delta（+4）、countdown（+10）**。previous／threshold只供更新和提示生产，Client不重算，故不发送。

原版IDB跳过第8、9行；官方`ASMRender.cpp::PatchStatHudPlanetariumChance*`重新启用**第8行**，`VanillaTweaks.cpp`在Level::Init和Game::RestoreState调用flag0x100生产星象房概率。因此保留0—8当前可绘行，第9行无当前已核官方绘制消费者，不发送。不能依据未打补丁的IDB删掉星象房。行6／7的合并显示由+2AC最终布尔决定；没有因此重跑物品498判断。

官方星象房显示资格还读Host进度406与本地`StatHUDPlanetarium`偏好。两者必须分开：Host已裁决的world资格与实际概率缓存由具名来源提供，本地显示偏好只决定本端是否显示；**不能用访客本地解锁替代Host资格，更不能为了显示写访客进度**。Host未产生／未取得合法来源时为具体Gap，不填false。旧“五值缓存”交接在本核对后由此三值投影替代，历史说明不回改为已实施。

### 3.3 C/S subject与本地排版

主体与批次按[定稿合同第3—7节](presentation-result-batches.md)执行：房主及远端各取本端已确认控制组作为默认详细HUD主体，各组公共HUD保持完整。原生两个History／Stat显示槽用于组内实际身体选择，不能把槽号当Peer号；Host当前两槽采集不足时补各组具名来源，不能让Client重算或清空其他人。该选择不修改原生roster／PlayerIndex或设备绑定；多身体显示实例及原子范围必须明确，缺结果不早ACK。

`97E4A0 GetPlayerHUDLayout`、`97EB40`和`834930`含原版NetplayManager分支。不能只安装Player*后调用它们并假设纯派生。C/S维护自己的参与者→身体→显示槽／显示组映射，实际subject/use/shape与目标状态、拓扑、资源版本共同提交；八个槽、动态HUD、History两列、StatHUD两列各按真实owner表达。四参与者不等于四身体或四列。

本地视口、HUDOffset、字体度量决定屏幕坐标；`97FD50`覆盖HUD+5944..59B3的布局scratch。`HistoryHUD`官方实现按屏幕、ExtraHUDStyle和两个owner计算position/rows/columns/visible prefix；这些可在同一明确布局合同重建。原版通信气泡、原版co-op选择／Online头像／设备身份不上线，C/S面板也不借用其可变状态。

## 4. 小地图：显示事实与本地查看操作

`Minimap=0x628`，两个0x160配置、三份Sprite、最终震动等见字节表。Wiki明确state、holdTime、shakeOffset、displayed size是不同概念。

`965DB0 Update`遍历Player设备，并走NetInputDevice／NetplayManager／`8E7550`路径；长按可调用`740740`，shakeDuration>0还调用随机来生产offset；它另改HUD_Message的TextOut。**不得把它作为“仅UI”在Client整函数执行。** 本地Map UI只保留明确的查看输入与透明度相位，使用独立纯UI维护；共享NO_HUD／curse等world限制独立施加。Host map按键不会强迫所有访客展开或切页。

`966680 Render`直接消费+620/+624当前震动；+61C只驱动Update生产，故只传最终offset，不传倒计时或随机流。这里`965DB0`确实调用`6EB4B0`全局随机，再用cos/sin形成偏移，不能把三角函数本身误叫随机。大小查询`963540`由本地config bounds／alpha计算；本地缓存不因Player状态已安装就自动存在。

真正的缓存路径是`965D60 Refresh → 9635B0`，包含descriptor／探索／保存图标、FBO绘制及挑战32特殊当前标记的随机选择。普通路径可在来源完整、线程／生命周期／图形状态保护及确定分支下重建；**挑战32必须提供Host已选SafeGridIndex锚点、对应描述关系与地图版本，不能调用随机再选**。[第7步交接补证](room-random-handoff.md#4-地图与error只保留真正消费者所需结果)确认它还影响裁剪窗口和描述上下文，仅标记位置不充分；原生169边界不得直接作为13×13下标。现有`client_map_native.cpp::ClientMinimapBuild`已明确拒绝challenge32，这个限制保留为历史事实，不能把旧可运行窄路径称为全地图覆盖。

官方`MinimapConfig.zhl::Render`的签名命中 **4ECA30**，反编译实际switch NPC状态、创建Entity、开火和播放音效，与地图不符。这个SDK函数标签不作为调用依据。`MinimapPatches.cpp`还在原地图图像提交前显式设ColorOffset shader；该本地图形状态维护必须保留。既有黑框漏失记录不能由只同步_iconsSprite解决，border Sprite有独立实际资源／图层外观。

## 5. 消息、图像与shader资源

`HUD_Message`不是字符串而已。`831AD0 UpdateTextImage`把UTF16主／副文字、特殊首码、字体与样式画入+18实际Image；`8318D0 Render`把这个图像投在streak null-frame，入口覆写+C/+10位置、+14比例及部分Sprite颜色。因此普通Render真正需要 **当前图像内容／尺寸＋streak实际姿态＋showing**，不需要旧坐标、Show/autoHide生产过程。fortune也有HUD+538C独立图像。

不能声称任意本地文字重绘都等价：消息尾部还有UNKNOWN，特殊首码、栅格样式和本地化可能影响实际图像。选择之一必须具备可审阅的完整来源：

1. 已核同版本recipe，明确实际字体、最终文字／特殊码、颜色、尺寸和所有实际生产参数，由安全资源绘制器重建；
2. 具名程序图像内容资源，包含尺寸／格式／内容版本和有界原始像素，Client本地创建并持有图像。

二者都不能传Host指针、FBO、control或GL对象。只有字符串或一个名字时，不构成通用重建证明，必须具体Gap。字体资源变化、内容变化和“Host明确空图像／Client加载失败”分别处理。

**Image名字不保证唯一。** 官方`LuaRender.cpp::Lua_Renderer_CreateImage`只把传入name加`.procedural`，同一个name可对应不同尺寸／内容；Image还可RenderToImage。HUD文字图、程序道具图、hallucination前一画面均不能只用`ImageBase._name`当文件路径。静态文件使用匹配资源目录和规范路径，程序图像使用C/S具名producer/subject/use、generation、整数内容版本或完整recipe；无需哈希。碰撞、未知producer、缺失内容不伪装成Null；未知Mod程序图只由声明adapter负责。

`ImageBase`前缀、派生后端大小未知和本地batch/vertex缓存见账本；不把未知派生图像当0x80完整体读取。Font本身不是唯一全局单例：Manager内嵌九份，Lua可构造自己的Font。`.fnt`度量／字符／字距和本地loaded由资源重建，既有三个Font getter不要求Host原始字段跨进程相等。

**官方shader旁表另有owner。** `Anm2Extras.cpp`的`anm2_extras`以ANM2*为key，包含default及layerID两级、普通／champion两种`optional<CustomShader>`。CustomShader是规范化路径＋本地shader*；Reset／destructor erase，Get不会创建项。线上只保存实际选择／必要显式覆盖关系和稳定shader资源，map节点、optional表示、key与shader*均本地。不能从ANM2+108／+10C猜shader，也不能对未指定layer丢掉default fallback。官方DLL里该STL旁表的物理ABI／合法直接读取尚未核定；不得把源码逻辑结构冒充已核二进制字节偏移。修订3已将当前原版必要来源落实为实际内置shader及具名固定pass，详见[来源第3节](presentation-resource-screen-sources.md#3-shader本轮原版来源与仍未核的旁表)；自定义shader的STL ABI仍保留UNKNOWN并归Mod后续适配。实际shader无法映射时报告具名Gap，不假设旁表为空或降级显示。

## 6. 屏幕、转场与菜单边界

- `6F7F90`读Game+66920当前六值；`6F6DE8`和`7021A0`用target／lerp生产current。后两份及开关留Host；当前lightning／dizzy有Room/NPC/Light直接消费者，保留。darkness两值都有消费者，不机械裁掉第二值。GameFrame、TimeCounter、screenShakeOffset与Run已有唯一归属，不能重复发送或改用墙钟。
- Game+25A98是实际前帧图像strong；25AA0/AA4/AA8是bloom输入；25AB0/AB4/AB8是hallucination生产／反馈。`701450 ShowHallucination`还使用`6EB4B0`随机、改Backdrop并处理实体，绝不能在Client调用。`6F7A80`中的sin振荡和四边形可按已核相位／资源／视口参数派生，不能误作随机或额外传顶点；前画面反馈仍须有实际内容或可证明的场景recipe，仅发时间和文件路径不够。
- Game+25B04和+25B1C是两组原生RGBA来源，网络只承接实际选中pass的颜色／alpha／顺序，不再额外发两组和selector；+25B18后续分派不能随完整Render执行。`6F1640`把退出／转场target写+25B34并写生产速度／进度，target不上线。+669EC由`clamp(value/90,0,1)`结果唯一承接，不重复value或执行Update转场。
- `984660 ItemOverlay::Render`检查state>=2、overlayID和两Sprite；`984110 Update`含SleepFillHP、解锁及转场分支，不能放行。WAITING延迟只决定未来可见性，不作为持续状态重复发送；晚加入直接安装当前最终显示。
- `8273C0 RoomTransition::Render`有前后surface、最终滑动／遮罩以及versus／额外肖像map；`412A90/412AC0`用于按帧参数旋转肖像，是cos/sin而非随机。`8FAC20 NightmareScene::Render`除四个公开Sprite外，还读+3A8辅助Sprite、14项路线图、场景帧、条件payload，并绘制实体／配置资源。+4B1是+3A8 Sprite的loaded，不能重复当外层字段。二者由具名显示pass维护已核确定变换和实际资源；只发公开Sprite不表示显示闭合，也不授权Update/Start/ChangeRoom。`909E60 StageTransition::Render`按+10的**DWORD==5**选色，不能把SDK `_sameStage` bool当已知全部语义。
- `970C10 GameOver::Render`会选择本地／Online Player并遍历死亡物品历史，需Host最终死亡显示subject／有序项目／击杀者资源；不为重建这张图恢复完整Player获得史或本地Online。GameOver声明0x130，紧接Leaderboard；不套旧巨大结构。
- PauseScreen的本地选择／options／确认框／CompletionWidget进度不从Host复制；暂停菜单要展示的Host seed、daily、challenge、stats等只用独立只读run显示结果。**不允许写访客成就、解锁、统计、存档。** Leaderboard显示可消费Host已有冻结显示结果，绝不执行Show(SubmitScore=true)、get_leaderboard或本地提交。

### 已知边界缺口

初版将PauseScreen后缀、Game `[1D294,1D4C8)`、Nightmare条件队列及模式4虚拟owner记为未核；原记录保存在账本JSON修订历史。修订3已识别相邻UI为BossOverlay、队列头和32B元素及必要消费点、模式4为原版Deathmatch并按隔离排除；Pause+E04为Online密码显示切换，只有该1B归本地／排除。Pause其余后缀／后续owner、Game+66968..66993、Nightmare剩余37B及外部对象内部仍UNKNOWN，不能整片改成本地或padding。具体静态证据和显示结果交接见[来源第5—6节](presentation-resource-screen-sources.md#5-roomtransition与nightmare的内容来源)，不表示运行实现或完整产品支持已完成。

## 7. 音频：意图、事件与本地后端

**当前同步职责以[复核第5节](presentation-source-synchronization-review.md#5-音频最终voice参数与事件生产混音树不上线)为准**，具体静态点位见[音频专篇](presentation-audio-sources.md)。MC_POST_SFX_PLAY不能证明通过world逻辑资格；官方jingle ID也可由原生拒绝请求写入。本地后端非空source的Play提交同样不是world Start唯一资格。逻辑缓存覆盖仍需控制的one-shot；完整world用途／退休与音乐逻辑gain来源未闭合，个人后端失败不属于必须同步的结束事件。

官方Manager+29DF8内嵌`Music=0x368`，+2A160内嵌`SoundEffects=0x18`，下一字段OptionsConfig在+2A178。`SFXManager.zhl`的size0是空壳声明，**不能推导为没有原生字节**。`SoundEffect`实际stride0x1B8；`907C60`把ManagerFrame/2写+0、节流截止写+4，SDK `_id:+0`不能作为声音身份。声音资源身份是调用ID／实际资源目录选择。

原版Wiki明确SFX.Play的FrameDelay是“再次播放前需要间隔的帧数”，不是启动延迟。`907C60`确实在节流后用独立随机流选sample，再设置loop／pitch／volume并播放。Host正常原生执行一次后，记录**已选sample**／有效参数；Client不再随机，也不把FrameDelay变成网络排程延迟。

Music有两组0x184 stream set，各为主流＋6层、mask与混音参数；Crossfade会切换当前buffer且旧buffer仍可发声。只发`currentId`不能恢复正在crossfade的两voice；jingle另有0x34流。官方Hook在+32A的原生空隙写u16 jingle ID，公开getter在countdown<1返回0；这不是原版既有字段，也不是“jingle流已完全停止”的充分条件。

`7D8B10 VolumeSlide`把逻辑target乘Host Options.musicVolume再写后端；`7D8B50 UpdateVolume`直接覆盖原生current/target为本地设置；`907C60`的后端音量也乘Options.sfxVolume。**后端float原值不可直接下发，亦不可在Host静音时反除音量恢复逻辑增益。** 必须从正常控制调用／已选混音路径记录world逻辑意图，Client再应用自身音量／静音。局部暂停状态也与world暂停分开。

| 音频内容 | 传输与恢复语义 |
|---|---|
| 一次性声音 | 事件含独立序号／world时点／实际sample资源／逻辑参数；去重；baseline或重连不重播已经发生的历史one-shot |
| 持续loop／ambient | 当前逻辑voice资源／实例／最终gain／pitch／必要pan／loop／world pause与存续；重复state不重启；明确world用途结束才退休，个人后端丢失不清全体voice |
| Music／crossfade／jingle | 并存声部当前最终参数及world选曲／重播／退休事件；生产混音树留Host。必要逻辑段／起点及可靠续接能力仍待补，瞬态jingle不从头补播冒充恢复 |
| 采样精确播放游标 | Host硬件游标／设备延迟／PCM队列不属于必需world状态；必要逻辑时间另有来源，不能以Update计数伪造，不承诺逐采样一致 |

旧141行中SFX全部是命令/参数查询，不表示可以丢掉持续声音。[音频补证](presentation-audio-sources.md)已定位sample、正常分支和非随机leaf候选；修订5进一步分开world意图与本地执行。正式布局／缓存仍待第8／9／10／13步，音乐Options前全部writer、world用途／退休、视频音轨与必要逻辑段落／续接仍未闭合；不以Play入口或完整硬件完成通知冒充同步闭合。

## 8. 后续正式布局与唯一验收

历史D0先由37B收紧为13B两seed草稿，同日[第7步与Random核对](room-random-handoff.md)进一步选择逐用途实际随机结果，**当前D0为5B，仅Manager相位／paused**。Pickup／Familiar的正常已产生alpha与资格另成具名结果批次，绑定真实world／presentation修订及资源／pass／重复调用身份，不由Client全局访问顺序分配样本。两个绘制seed／shift／guard及新核BD0F40音效选样本流均不上线；音频取已选sample与逻辑voice。subframe仍派生，临时Sprite／颜色栈／CPU队列恢复须后续接通。UI正常生产后的最终缓存／批次独立封存，不额外getter、Recompute、Lua、随机或资源生产器补值。上述仍是设计，具体缓存／source／adapter未实施。

正式`state/presentation.json`和线格式尚未生成；下表是**草稿逻辑投影**，偏移只在各小记录内成立。记录头、subject/use/shape、版本、存在性／空集合、资源目录与预算由第8/9步统一定稿；未核源不能填零或忽略后ACK。

**Heart（1—9B草稿；图像资源目录内动画ID为u16）**。visible=false只编码首字节；其余字段仅在可见分支存在，overlay_animation再受eternal资格限定，不读取隐藏格旧指针。下表画最大分支。

| +0 u8 visible | +1 u8 eternal | +2 u8 golden | +3 u8 fading | +4 i8 flash_type | +5 u16 animation | +7 u16 overlay_animation |
|---|---|---|---|---|---|---|

**StatValue（12B草稿；物理行0..8按真实可绘资格选择）**

| +0 f32 value | +4 f32 delta | +8 i32 countdown |
|---|---|---|

**Inventory／HistoryVisual（6B草稿；逻辑顺序保留）**

| +0 i32 visual_id | +4 u16 actual_image_resource |
|---|---|

**ActiveVisual（12B草稿；Player P4六值另属已有owner）**

| +0 i32 visual_id | +4 u16 item_image | +6 u16 book_image | +8 i32 charge_flash |
|---|---|---|---|

**TrinketVisual（15B草稿）**

| +0 i32 visual_id | +4 u16 actual_image | +6 u8 tint_override | +7 f32 center_x | +11 f32 center_y |
|---|---|---|---|---|

其余HUD根、共享Prompt、程序图像资源、音频voice／event、屏幕／过渡有序pass及显示查询批次都需要上述具体owner与合法来源接入，不靠这五张小表宣称完整线格式。初版原生表45个类型／视图，资源／屏幕来源修订3后为49个；UNKNOWN按直属间隙列出，嵌套另计。派生Image、Mod shader旁表物理ABI和UI其余后缀没有伪造大小；本轮已核必要来源不再笼统列为完全未核，实际封存与应用仍待实施。

第15步唯一统一验收需覆盖：四参与者/多身体与动态HUD；普通及特殊健康、HolyMantle、低血闪隐、两列属性与星象房；11消息槽和fortune的空/非空程序图；History顺序/负ID/shader；挑战32地图、黑框、各端独立展开/视口；颜色渐变的最终状态、bloom/retrovision/hallucination/过渡/死亡/暂停；crossfade、jingle、Host或Client静音、持续loop中途加入/停止和one-shot去重。当前均未执行；不增加提前测试或新的tx编号。
