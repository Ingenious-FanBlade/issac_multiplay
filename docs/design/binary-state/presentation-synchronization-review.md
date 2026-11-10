# 第8步 Presentation：原生字节与同步职责核对

2026-11-10；目标为官方 REPENTOGON 1.1.3／v1.9.7.12.J273 Win32。完成本轮 **Presentation 的静态字节登记、同步必要性核对和旧公开目录回映射**；这不是第8步整体或正式同步实现完成。设计账本为 [JSON](presentation-byte-ledger.json)／[原生单行表](presentation-byte-ledger.md)，公开成员为 [JSON](presentation-member-map.json)／[逐行表](presentation-member-map.md)。本轮不修改生产布局版本，不接入 `state/presentation.json`、Hook、reader、writer，不构建、不运行游戏或测试。UNKNOWN和来源缺口仍明确保留。

## 1. 资料顺序与证据等级

先查本地原版 Wiki：`F:/projects_isaac/IsaacDocs/docs/{HUD,MusicManager,SFXManager,Font,FontRenderSettings}.md`；再查官方 REPENTOGON Wiki：`F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/{HUD,PlayerHUD,PlayerHUDHeart,HistoryHUD,HistoryHUDItem,Minimap,MusicManager,ColorModifier,GenericPrompt,NightmareScene,FXParams,renderer/Image,menus/PauseMenu}.md`。官方在线 [HUD](https://repentogon.com/HUD.html)／[Minimap](https://repentogon.com/Minimap.html)仅作语义旁证，不拿当前网站代替本地1.1.3 ABI。

随后读官方 `libzhl/functions/` 同名结构，以及 `repentogon/LuaInterfaces/HUD/*.cpp`、`LuaMusicManager.cpp`、`LuaGenericPrompt.cpp`、`LuaNightmareScene.cpp`、`LuaRender.cpp`、`Patches/{Anm2Extras,MinimapPatches,ImagePatches,InterpolV2}.cpp`、`Patches/ASMPatches/ASMRender.cpp`、`Patches/VanillaTweaks.cpp`。只有公开资料不能回答具体字节或副作用时才查 J273 IDB；本文地址均为 **VA，RVA=VA−0x400000**，未改动或保存IDB。

`已核`表示有相应指令或官方实现支持；`SDK声明`保留原等级；`推断`没有升级成实测；未知间隙逐段列为 `未知`。账本登记整个声明范围，不等于每个字节语义已知，也不等于每个字段都发送。表中“本地／Host”的外层对象不因嵌套ANM2通用表含“状态”而恢复传输。已有 Run 字节及分数结论见 [Run核对](run-synchronization-review.md)，不重复建立writer。

## 2. 最终同步决定

| 范围 | 需要同步的内容 | 不需要或不应同步的内容 |
|---|---|---|
| PlayerHUD | 实际身体owner／显示归组、24个最终心格、低血闪隐和三色闪烁、四主动显示槽、两饰品显示槽、四Pocket独立Sprite、HolyMantle最终显示资格、有序inventory图像、crafting实际图像、Drop显示比例 | Player*/HUD*/control、设备键、刷新dirty、资源选择比较历史、UpdateHearts的全部健康生产输入、按键重放、绘制包围框旧scratch |
| StatHUD | 实际两列身体owner、当前可绘0—8行的value／delta／countdown、合并概率显示方式、实际Sprite资源、Host星象房显示资格 | previous、threshold、当前无消费者的第9行、RecomputeStats、概率生产输入和Client进度查询；不恢复Player六项玩法属性 |
| HUD消息／fortune | 11个消息物理槽各自当前存在性、streak资源／姿态及实际文字图像；fortune Sprite与独立文字图像；需要语义文本的声明UI另保留有界文字 | Show命令重放、原始指针、自动隐藏生产FSM、旧position/scale、字体／FBO／图形批次对象；字符串或路径不足以替代实际文字图像 |
| HistoryHUD | 两个实际显示owner和有序visual_id＋实际Image；保留负ID、633及0x88000 shader分支 | 原始获得史、时间／池／层、16索引、按库存count重排；视口位置、列数、可见前缀由本地布局计算 |
| Boss HUD | 填充值（含负值隐藏）、闪色相位、当前显示图像选择、特殊计数字段、具名最终位置／绘制分支 | 伤害生产过程、Boss AI、原版Online选择／设备身份；不能仅从当前Entity HP重建全部显示缓存 |
| Minimap | 同world描述／探索／保存图标与有效curses、实际特殊标记结果、world强制显示资格、当前最终震动偏移、必要Sprite／图层外观 | Host的Map按住/释放历史、holdTime、窗口／分辨率、FBO、shakeDuration随机生产倒计时、原版Online输入；不直接运行Minimap::Update |
| 颜色／屏幕效果 | Game当前ColorModState六值、当前闪电／眩晕、黑暗两输入、实际bloom／retrovision／fade／hallucination／整屏覆盖的最终有序pass参数与必要图像 | target／lerp／dizzyTarget／changeSpeed、窗口resize、原生feedback字节、计时／退出／转场FSM、ShowHallucination、玩法随机源 |
| ItemOverlay | 最终可见性、overlay身份、实际主Sprite、MegaMush Sprite与真实身体锚点 | WAITING倒计时、47份静态配置表、Show／Update重放、SleepFillHP／解锁／切房 |
| Music | 已选曲目／crossfade两voice的逻辑状态、六layer最终mask、有效pitch／逻辑增益、jingle当前voice与相位；真实开始/停止事件 | 后端stream/句柄/采样缓冲、本地音量／静音设置、原生buffer槽号、全量XML配置、复制queuedId后自行按玩法选曲 |
| SFX | 一次性声音的已选sample事件；持续loop／ambient当前voice集合及pitch／逻辑增益／终止 | 随机sample选择器、节流历史、音频设备、后端playing/loaded、Host Options音量、只靠事件恢复全部声音 |
| Font／Image／shader | 资源身份、实际选中图像、独立程序图像的内容或完整recipe、shader选择和布局定义；特定文字recipe的必要参数 | Font*/Image*/GL/FBO/引用计数、渲染批次、仅凭Image名字猜身份；不每帧发字体度量 |
| Prompt／暂停／死亡／过渡 | 共享world Prompt的最终纸张/文字/选择外观、死亡界面最终显示结果、过渡最终pass和肖像结果；Host暂停语义在D0 | 本地菜单选择／确认提交、Host选项、邀请／踢人／Online面板、成就解锁／保存／排行榜请求；展示不执行Game::End或ChangeRoom |

**有Getter或可写不等于应同步。** 旧目录中的55条`ReplicatedState`已经逐行改判；包括Font度量、History时间、ItemOverlay.Delay、GenericPrompt.SubmittedSelection等，均不因旧标签而保留原值复制。旧141行里没有Minimap、HUDMessage及NightmareScene，不能据此漏掉其实际显示负载；补充范围单独登记。

## 3. PlayerHUD、属性栏与身体归属

### 3.1 PlayerHUD的真实生产与消费

- SDK `PlayerHUD=0x6E0`，HUD内8份，另有Player+20A8的动态strong owner。`832C10`写24心、四份0x1C主动显示desc、两份0x18饰品desc、四份0x11C Pocket显示体以及+6A4列表／+6B4合成图像；`97AE90`遍历8份调用它。**完整PostUpdate不是纯显示维护。** 它还调用History Recompute和StatHUD更新，不能在Client放行。
- `832800`生产最终心格；`834930`消费可见／eternal／golden／fade／flash以及实际动画名，另有Player角色、curse、额外生命和特殊健康绘制分支。第5步已保留的Player P2依赖继续有效；不能用24个心格替代全部特殊健康显示，也不能为刷新心格恢复被裁的原始健康字段。
- `834930`先判断每格visible，再读其余显示值。隐藏格只发送visible=false，不读取残留动画指针；overlay名称只在实际eternal分支需要时读取。`412A90/412AC0`分别是cos/sin封装，淡出色、肖像旋转和屏幕振荡的这类计算是确定数学，不新增RNG状态或逐顶点负载；应按同world显示相位与实际资源参数重建。
- `PlayerHUD+0A`是低血闪隐强度，`832C10`依据生命和GameFrame%45产生；+0C/+0D/+0E是三种心闪计时；+6A0是effect313（HolyMantle）的最终显示资格，`834930`直接消费。它们不能遗漏，也不能在Client用输入／效果重算。
- `835B40`除Player P4六值外消费实际itemImage/bookImage及charge flash；`837720`饰品使用最终图像与中心，且在非golden分支判断desc+4是否非零来选择额外colorize。SDK的`slot`名字不能作为丢弃理由：只发这项1B最终`tint_override`，其整数更新比较值不发。`839370`／`839A80`消费+6A4按12B步长的有序 `{visual_id, Image strong}`；后者还用+6B4结果图像。实际图像结果不能被重新按ID查ItemConfig／抽取／排序取代。
- `832C10`末尾读Drop动作后产生+6BC显示比例，`97EB40`用其淡化实际身体的道具显示。这里只传显示结果，统一输入scheduler的Host职责不变。`839A80`对+6C0..6CF累计包围框；该scratch在正确绘制初始化／pass中重建，不发旧残值。

### 3.2 StatHUD从“五值全发”收紧为绘制三值

`StatHUD+114`起有两组0xCC：Player*4B＋10×0x14；每项为value、delta、previous、threshold、countdown。`83CFD0`、`83D210`维护五值；`83C9B0`的实际读取为 **value（+0）、delta（+4）、countdown（+10）**。previous／threshold只供更新和提示生产，Client不重算，故不发送。

原版IDB跳过第8、9行；官方`ASMRender.cpp::PatchStatHudPlanetariumChance*`重新启用**第8行**，`VanillaTweaks.cpp`在Level::Init和Game::RestoreState调用flag0x100生产星象房概率。因此保留0—8当前可绘行，第9行无当前已核官方绘制消费者，不发送。不能依据未打补丁的IDB删掉星象房。行6／7的合并显示由+2AC最终布尔决定；没有因此重跑物品498判断。

官方星象房显示资格还读Host进度406与本地`StatHUDPlanetarium`偏好。两者必须分开：Host已裁决的world资格与实际概率缓存由具名来源提供，本地显示偏好只决定本端是否显示；**不能用访客本地解锁替代Host资格，更不能为了显示写访客进度**。Host未产生／未取得合法来源时为具体Gap，不填false。旧“五值缓存”交接在本核对后由此三值投影替代，历史说明不回改为已实施。

### 3.3 C/S subject与本地排版

`97E4A0 GetPlayerHUDLayout`、`97EB40`和`834930`含原版NetplayManager分支。不能只安装Player*后调用它们并假设纯派生。C/S维护自己的参与者→身体→显示槽／显示组映射，实际subject/use/shape与目标状态、拓扑、资源版本共同提交；八个槽、动态HUD、History两列、StatHUD两列各按真实owner表达。四参与者不等于四身体或四列。

本地视口、HUDOffset、字体度量决定屏幕坐标；`97FD50`覆盖HUD+5944..59B3的布局scratch。`HistoryHUD`官方实现按屏幕、ExtraHUDStyle和两个owner计算position/rows/columns/visible prefix；这些可在同一明确布局合同重建。原版通信气泡、原版co-op选择／Online头像／设备身份不上线，C/S面板也不借用其可变状态。

## 4. 小地图：显示事实与本地查看操作

`Minimap=0x628`，两个0x160配置、三份Sprite、最终震动等见字节表。Wiki明确state、holdTime、shakeOffset、displayed size是不同概念。

`965DB0 Update`遍历Player设备，并走NetInputDevice／NetplayManager／`8E7550`路径；长按可调用`740740`，shakeDuration>0还调用随机来生产offset；它另改HUD_Message的TextOut。**不得把它作为“仅UI”在Client整函数执行。** 本地Map UI只保留明确的查看输入与透明度相位，使用独立纯UI维护；共享NO_HUD／curse等world限制独立施加。Host map按键不会强迫所有访客展开或切页。

`966680 Render`直接消费+620/+624当前震动；+61C只驱动Update生产，故只传最终offset，不传倒计时或随机流。这里`965DB0`确实调用`6EB4B0`全局随机，再用cos/sin形成偏移，不能把三角函数本身误叫随机。大小查询`963540`由本地config bounds／alpha计算；本地缓存不因Player状态已安装就自动存在。

真正的缓存路径是`965D60 Refresh → 9635B0`，包含descriptor／探索／保存图标、FBO绘制及挑战32特殊当前标记的随机选择。普通路径可在来源完整、线程／生命周期／图形状态保护及确定分支下重建；**挑战32必须提供Host已经选择的最终标记或完整有证据的recipe，不能调用随机再选**。现有`client_map_native.cpp::ClientMinimapBuild`已明确拒绝challenge32，这个限制保留为历史事实，不能把旧可运行窄路径称为全地图覆盖。

官方`MinimapConfig.zhl::Render`的签名命中 **4ECA30**，反编译实际switch NPC状态、创建Entity、开火和播放音效，与地图不符。这个SDK函数标签不作为调用依据。`MinimapPatches.cpp`还在原地图图像提交前显式设ColorOffset shader；该本地图形状态维护必须保留。既有黑框漏失记录不能由只同步_iconsSprite解决，border Sprite有独立实际资源／图层外观。

## 5. 消息、图像与shader资源

`HUD_Message`不是字符串而已。`831AD0 UpdateTextImage`把UTF16主／副文字、特殊首码、字体与样式画入+18实际Image；`8318D0 Render`把这个图像投在streak null-frame，入口覆写+C/+10位置、+14比例及部分Sprite颜色。因此普通Render真正需要 **当前图像内容／尺寸＋streak实际姿态＋showing**，不需要旧坐标、Show/autoHide生产过程。fortune也有HUD+538C独立图像。

不能声称任意本地文字重绘都等价：消息尾部还有UNKNOWN，特殊首码、栅格样式和本地化可能影响实际图像。选择之一必须具备可审阅的完整来源：

1. 已核同版本recipe，明确实际字体、最终文字／特殊码、颜色、尺寸和所有实际生产参数，由安全资源绘制器重建；
2. 具名程序图像内容资源，包含尺寸／格式／内容版本和有界原始像素，Client本地创建并持有图像。

二者都不能传Host指针、FBO、control或GL对象。只有字符串或一个名字时，不构成通用重建证明，必须具体Gap。字体资源变化、内容变化和“Host明确空图像／Client加载失败”分别处理。

**Image名字不保证唯一。** 官方`LuaRender.cpp::Lua_Renderer_CreateImage`只把传入name加`.procedural`，同一个name可对应不同尺寸／内容；Image还可RenderToImage。HUD文字图、程序道具图、hallucination前一画面均不能只用`ImageBase._name`当文件路径。静态文件使用匹配资源目录和规范路径，程序图像使用C/S具名producer/subject/use、generation、整数内容版本或完整recipe；无需哈希。碰撞、未知producer、缺失内容不伪装成Null；未知Mod程序图只由声明adapter负责。

`ImageBase`前缀、派生后端大小未知和本地batch/vertex缓存见账本；不把未知派生图像当0x80完整体读取。Font本身不是唯一全局单例：Manager内嵌九份，Lua可构造自己的Font。`.fnt`度量／字符／字距和本地loaded由资源重建，既有三个Font getter不要求Host原始字段跨进程相等。

**官方shader旁表另有owner。** `Anm2Extras.cpp`的`anm2_extras`以ANM2*为key，包含default及layerID两级、普通／champion两种`optional<CustomShader>`。CustomShader是规范化路径＋本地shader*；Reset／destructor erase，Get不会创建项。线上只保存实际选择／必要显式覆盖关系和稳定shader资源，map节点、optional表示、key与shader*均本地。不能从ANM2+108／+10C猜shader，也不能对未指定layer丢掉default fallback。官方DLL里该STL旁表的物理ABI／合法直接读取尚未核定；不得把源码逻辑结构冒充已核二进制字节偏移。这是正式资源来源仍需落实的具体缺口。

## 6. 屏幕、转场与菜单边界

- `6F7F90`读Game+66920当前六值；`6F6DE8`和`7021A0`用target／lerp生产current。后两份及开关留Host；当前lightning／dizzy有Room/NPC/Light直接消费者，保留。darkness两值都有消费者，不机械裁掉第二值。GameFrame、TimeCounter、screenShakeOffset与Run已有唯一归属，不能重复发送或改用墙钟。
- Game+25A98是实际前帧图像strong；25AA0/AA4/AA8是bloom输入；25AB0/AB4/AB8是hallucination生产／反馈。`701450 ShowHallucination`还使用`6EB4B0`随机、改Backdrop并处理实体，绝不能在Client调用。`6F7A80`中的sin振荡和四边形可按已核相位／资源／视口参数派生，不能误作随机或额外传顶点；前画面反馈仍须有实际内容或可证明的场景recipe，仅发时间和文件路径不够。
- Game+25B04和+25B1C两组最终RGBA、实际被选择的pass需独立表达；+25B18后续分派不能随完整Render执行。`6F1640`把退出／转场target写+25B34并写生产速度／进度，target不上线。+669EC的当前整屏覆盖可换算为`clamp(value/90,0,1)`，不执行其Update转场。
- `984660 ItemOverlay::Render`检查state>=2、overlayID和两Sprite；`984110 Update`含SleepFillHP、解锁及转场分支，不能放行。WAITING延迟只决定未来可见性，不作为持续状态重复发送；晚加入直接安装当前最终显示。
- `8273C0 RoomTransition::Render`有前后surface、最终滑动／遮罩以及versus／额外肖像map；`412A90/412AC0`用于按帧参数旋转肖像，是cos/sin而非随机。`8FAC20 NightmareScene::Render`除四个公开Sprite外，还读+3A8辅助Sprite、14项路线图、场景帧、条件payload，并绘制实体／配置资源。+4B1是+3A8 Sprite的loaded，不能重复当外层字段。二者由具名显示pass维护已核确定变换和实际资源；只发公开Sprite不表示显示闭合，也不授权Update/Start/ChangeRoom。`909E60 StageTransition::Render`按+10的**DWORD==5**选色，不能把SDK `_sameStage` bool当已知全部语义。
- `970C10 GameOver::Render`会选择本地／Online Player并遍历死亡物品历史，需Host最终死亡显示subject／有序项目／击杀者资源；不为重建这张图恢复完整Player获得史或本地Online。GameOver声明0x130，紧接Leaderboard；不套旧巨大结构。
- PauseScreen的本地选择／options／确认框／CompletionWidget进度不从Host复制；暂停菜单要展示的Host seed、daily、challenge、stats等只用独立只读run显示结果。**不允许写访客成就、解锁、统计、存档。** Leaderboard显示可消费Host已有冻结显示结果，绝不执行Show(SubmitScore=true)、get_leaderboard或本地提交。

### 已知边界缺口

PauseScreen SDK只声明0xDF4，但`98DDA0`读相对+E04；因此只能登记这个声明前缀。Game `[2363C,25444)`的其余范围、`[1D294,1D4C8)`相邻UI及Game+66968..66993仍明确UNKNOWN，不能整片标成“菜单本地”“不存在”或padding。Nightmare未核deque／实体肖像payload、未知运行模式4的虚拟显示owner同样不能忽略。字节账本完成的是当前声明边界的归属核对，不声称这些未知已获语义或完整产品支持。

## 7. 音频：意图、事件与本地后端

官方Manager+29DF8内嵌`Music=0x368`，+2A160内嵌`SoundEffects=0x18`，下一字段OptionsConfig在+2A178。`SFXManager.zhl`的size0是空壳声明，**不能推导为没有原生字节**。`SoundEffect`实际stride0x1B8；`907C60`把ManagerFrame/2写+0、节流截止写+4，SDK `_id:+0`不能作为声音身份。声音资源身份是调用ID／实际资源目录选择。

原版Wiki明确SFX.Play的FrameDelay是“再次播放前需要间隔的帧数”，不是启动延迟。`907C60`确实在节流后用独立随机流选sample，再设置loop／pitch／volume并播放。Host正常原生执行一次后，记录**已选sample**／有效参数；Client不再随机，也不把FrameDelay变成网络排程延迟。

Music有两组0x184 stream set，各为主流＋6层、mask与混音参数；Crossfade会切换当前buffer且旧buffer仍可发声。只发`currentId`不能恢复正在crossfade的两voice；jingle另有0x34流。官方Hook在+32A的原生空隙写u16 jingle ID，公开getter在countdown<1返回0；这不是原版既有字段，也不是“jingle流已完全停止”的充分条件。

`7D8B10 VolumeSlide`把逻辑target乘Host Options.musicVolume再写后端；`7D8B50 UpdateVolume`直接覆盖原生current/target为本地设置；`907C60`的后端音量也乘Options.sfxVolume。**后端float原值不可直接下发，亦不可在Host静音时反除音量恢复逻辑增益。** 必须从正常控制调用／已选混音路径记录world逻辑意图，Client再应用自身音量／静音。局部暂停状态也与world暂停分开。

| 音频内容 | 传输与恢复语义 |
|---|---|
| 一次性声音 | 事件含独立序号／world时点／实际sample资源／逻辑参数；去重；baseline或重连不重播已经发生的历史one-shot |
| 持续loop／ambient | 当前voice集合：稳定C/S voice身份、资源、loop、期望playing/pause、pitch、逻辑gain／pan及当前有效相位；baseline可恢复，消失/clear必须停止，重复state不重新Play |
| Music／crossfade／jingle | 当前逻辑voice状态与真实开始/切换/结束事件共同维护；不中途自行按Gameplay选择queued曲目；后端资源准备和播放游标的本地实现另管 |
| 采样精确播放游标 | 目前没有跨后端时基／seek ABI证据；不能把原生整数／句柄当游标发送，也不承诺样本级一致 |

旧141行中SFX全部是命令/参数查询，不表示可以丢掉持续声音。最终sample来源leaf、逻辑增益缓存和持续voice维护尚需第8步正式源及第10/13步接线；不假称已经通过Play入口Hook完整取得。

## 8. 后续正式布局与唯一验收

本轮保留D0的37B设计及两份纯Render RNG例外；subframe仍派生，进入/退出绘制借用并恢复。不把音频选择或其他玩法随机流纳入D0，也不把cos/sin当RNG。UI正常Host生产后的最终缓存／显示批次须独立封存，不额外调用getter、Recompute、Lua玩法、随机或资源生产器补值。

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

其余HUD根、共享Prompt、程序图像资源、音频voice／event、屏幕／过渡有序pass及显示查询批次都需要上述具体owner与合法来源接入，不靠这五张小表宣称完整线格式。原生单行表已覆盖45个类型/连续视图，UNKNOWN按直属间隙列出，嵌套另计；未复核的派生Image、官方shader旁表物理ABI和UI后缀没有伪造大小。

第15步唯一统一验收需覆盖：四参与者/多身体与动态HUD；普通及特殊健康、HolyMantle、低血闪隐、两列属性与星象房；11消息槽和fortune的空/非空程序图；History顺序/负ID/shader；挑战32地图、黑框、各端独立展开/视口；颜色渐变的最终状态、bloom/retrovision/hallucination/过渡/死亡/暂停；crossfade、jingle、Host或Client静音、持续loop中途加入/停止和one-shot去重。当前均未执行；不增加提前测试或新的tx编号。
