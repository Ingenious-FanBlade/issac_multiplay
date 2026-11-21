# 第7步 Room：原生字节归属与同步职责核对

2026-11-10；官方 REPENTOGON 1.1.3／Repentance+ v1.9.7.12.J273 Win32。此次范围是 **Room及其显示依赖**，不是整个第7步。交付[原生账本JSON](room-byte-ledger.json)／[单行表](room-byte-ledger.md)和[公开成员JSON](room-member-map.json)／[逐行表](room-member-map.md)。Room的`[0,7898)`连续登记；未知仍是未知，不能称全部语义已经闭合。生产布局版本保持56；未改采集、协议、Hook或原生安装代码，未构建、运行游戏或测试。

**同日剩余字节续核（修订2）**：先Wiki／官方REPENTOGON，后J273原生补证，详见[1290B与嵌套未知补证](room-residual-evidence.md)。现为32个类型／连续视图，Room91个直接范围、直属UNKNOWN **1290→28B**；EntityList884→3B、FXLayers39→3B、FX31→5B、RayGroup4→2B、Vein8→7B。新增FXPoop视图仍有5B未知；BackdropConfig纠正过宽声明后未知4→5B／实例。首轮26视图／85范围／1290B为历史基线，保留在JSON的`residual_revision`，不代表当前结论。新增必要显示值为实际采用配置的反向水面方向，环境音频必须承接实际voice；原生callback、原版Online历史和Host生成控制不下发。正式reader／writer仍未实施。

## 1. 资料顺序与证据口径

先读本地原版 Wiki `F:/projects_isaac/IsaacDocs/docs/Room.md`，再读本地 REPENTOGON Wiki 的 `Room.md`、`Backdrop.md`、`Camera.md`、`FXLayers.md`、`FXParams.md`、`RailManager.md`，目录为 `F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/`。随后核对官方 `libzhl/functions/{Room,Backdrop,BackdropConfig,Camera,FXLayers,FXParams,RailManager,EntityList,EntityList_EL,TemporaryEffects}.zhl`，以及 `repentogon/LuaInterfaces/{Room/LuaRoom,LuaBackdrop,LuaCamera,LuaFXParams}.cpp`、`Patches/ASMPatches/{ASMRoom,ASMCamera}.{h,cpp}`。Wiki明确GridPath可动态衰减、水量是浮点、雨强度用于生成、FX ColorModifier是配置副本、Camera与本地视口有关；不能只凭字段名决定同步。

SDK未回答的偏移、消费者和副作用，用J273 IDB只读反编译／反汇编补证。本次读取的地址均为VA；RVA=VA−`400000`。未改IDB、补丁、调用目标函数或附加游戏。旧研究中已有运行事实仍按原场景保留，本次静态核对不产生新运行结论。账本的“SDK声明”“物理关系已核／用途未决”“未知”不提升成运行验证。

## 2. 需要同步、无需同步与禁止复制

| 范围 | 最终决定 | 消费者／原因 |
|---|---|---|
| 当前描述身份、实际RoomType、宽高、bounds、真实八门槽／格序号 | 结构状态／稳定引用 | Room/Grid/水面/特殊背景共享同一world几何；448格、8门都不是地址负载，门槽不能从Door.primary Slot反推 |
| `+11F0`房间帧基准、`+723C`Greed timer | 保留原值 | `809590`及第6步PressurePlate/Door、HUD的实际消费者；只解释显示，不生成下一波 |
| `+72A4/+72D0`两份完整ColorMod、`+7240`水量、`+7460`水流、`+7740`熔岩强度 | 保留显示状态 | 地板、墙、Door/Grid、water packet和GridLight／纹理坑直接消费；水量不能压成bool，熔岩不能因整屏颜色已有结果就删掉 |
| 背景图像、实际Sprite／图层外观、FX实际选图和有序pose、选中ray组、vein图、轨道有效字节 | 保留实际结果与资源 | Client消费已选结果，避免重新选择、创建装饰／矿车／雨滴 |
| 四个水色端点、插值duration/countdown、默认FX水色 | 换成最终water shader结果 | `816BB0`有默认、seed79覆盖和插值分支，最后还按waterAmount处理alpha／乘色；只复制公开Getter不是实际水色 |
| 实际采用BackdropConfig的`+139 reversewatergfx` | 1B方向或等价最终有符号phase | `91AC70`按此反转水面动画相位；静态纹理不能替代，不与`+138`或未知尾部打包成DWORD |
| Room的16份环境gain平滑缓存、延迟sound460计时 | 原数组／生产计时留Host；实际voice另承接 | `7F5950`生产持续环境声与pan；第8步须有已选sample及world逻辑gain／pitch／pan，不能随缓存一起丢弃声音 |
| ShadowAlpha／原始light与shadow色、Room效果列表、波纹推进参数 | 按具体pass换成最终强度／混色／资格／波纹结果 | 不为Client刷新画面恢复玩家物品、curse、计数器、随机或Room效果生产链 |
| GridPath、清房延迟、red-heart-damage、pacifist、broken-watch、雨强度／区域数、生成seed／RNG、补货队列 | 留Host | 寻路、碰撞、伤害、速度、机会、生成、抽选及未来流程输入；当前Entity／Grid／HUD／音频结果分别承接，不追求原始Getter全相等 |
| Camera六个历史ring、滤波／跟随参数、surface原点、light去重树、pit dirty、allocator／control／GL对象 | 本地维护或明确纯显示派生 | 本端视口、对象生命周期、图形资源和pass scratch；Host内存布局及地址无跨进程含义 |
| 十组输出set／原生函数vector、死亡记录／名单控制、EntityList碰撞／查询缓存和原版Online32帧环 | 不进入C/S | 函数地址绝不复制／安装／调用；玩法队列、历史及原版Online私有状态不为Client重放，真正Entity／Grid后态另发 |
| 生成／Restore／Save／Update／随机Getter／setter的调用历史 | 不作为同步命令重放 | 公开API、Render可达或“不减少池”均不能证明无玩法副作用；实际显示结果须有合法来源 |

“留Host”不允许把其产生的当前画面删掉。例如不发雨生成器仍需同步真正存在的雨Effect；不发boss选择输入仍要提供真实BossIntro资源；不发补货队列仍要提供实际货物／价格；不发Room效果容器仍要承接效果481在`804500`产生的绘制资格。跨域实际结果未接线时为具名缺口，不默认为零或空。

## 3. Room边界、结构与时钟

`7DF8F0`构造的`+0`是字节，**不是vtable／descriptor**。`809590..8095AA`先检查该字节，未初始化返回−1，否则返回`Game+25A70 - Room+11F0`。线上记录存在性和读取资格与本地正常构造分开；不能收到一个initialized=true就越过原生owner。帧基准原值与同world GameFrame绑定，重复展示不重新锚定，不用墙钟或Client tick改写。

SDK的`_gridEntities[0x1BF]`少一项。`7E0190`明确析构448格，`7DFFF0..7E0004`从`+24`清`720h`，恰好是448指针和8门；grid区间为`+24..723`，门为`+724..743`。门格序号在`+744..763`。`RailManager._gridflags[447]`也少一项：真实448字节从`+4`到`+1C3`，Sprite紧接`+1C4`；`9B7C70/9B7F00`允许index447。容量不代表小房每格有效，更不能用Lua层uint8索引读取256以上轨道。

SDK的`_gridPaths[456]`也不正确：`+76C..E6B`为448个DWORD动态path，`+E6C..11EB`另为448个WORD的NPC占格／择位避让计时；`7F91AA/7F91B6`在同一实际width×height循环分别按2／4字节推进。原未知864B是第二表后432项，旧456-DWORD越过第一表32B；两表都留Host，不能为了取得旧Getter相等而恢复Client寻路／碰撞。

真实bounds由`7E86C0`的已核纯边界分支及Source当前值说明，十二房形／L形规则见既有[Room应用](../tx-04-18/tx-04-18-room-application.md)。正式来源直接复制有资格的内存，不调用Room.Init、GetRandomTile或配置PickEntry。Data／OverrideData仍归descriptor/配置资源工作；两份实际内容／身份不能只按stage/type/variant/subtype强行合并。本轮回映射中的delegated明确没有完成那部分。

`+764`是TintedRockIdx（原绑定`852DAF→431770`），`+768`是DungeonRockIdx（`852DB9→815D20`，另看Game flag后可能返回−1）。它们留Host，实际石头外观已在Grid；不因名字含index就建立额外网络身份。`+1BB4`经`852EEF→8236C0`核为第二boss ID；Init清零／选择后写，`8258D7`与第一ID一起进入头像生产。Wiki明确相同双Boss时第二ID可能为0，因此不能把非零当作“双Boss房”资格。

`GetEnemyDamageInflicted`原绑定`852C8D→844750`读Room的float[1205]，即`+12D4`，物理上在EntityList相对`+BC`。这是本帧敌人总伤害，用于充能生产，留Host；不能因它落在列表内部就当指针、padding或Room颜色。`Room.Render`写`+12B8/+12BC`的两个DWORD则是EntityList相对`+A0/+A4`的本地pass scratch，不同步。

`+7220`原生只核一个字节：`804500`波纹shader分支据此加入半帧speed/2。真正`HasSlowDown/SetSlowDown`原绑定`852DA5→452540`／`852D9B→452550`读写`+722C`，分别为`>0`与`max(current,arg)`；SDK int `_slowdownDuration+7220`不正确。官方`ASMEntityNPC.cpp::IsRoomSlow`仍经该SDK过宽读到`+7221..7223`，三字节真实语义继续未知，不能说没有官方读取或证明为padding。`+7224`已纠正为Boss deathspawn计数／兜底维护，`+7230`是周期生成troll bomb的计时；二者均Host，不能混成减速ABI。相邻`+721C`已核为Puddle装饰选择计数，也不属于血墙。两个波纹的中心／radius／包络与当前相位是实际显示依赖，未来推进、max和speed可由最终本相位参数替代；最终来源仍需第10步接线。

## 4. Backdrop：结果资源代替重抽

`Backdrop=5278h`含四个surface strong pair、三份图像pair、四份`ANM2`、63份`13Ch`配置、两个向量、16B RNG与seed。`90BD90`、`917870`和`918A60/9191F0`互相印证：`+20`是实际surface选择字节，`+21..23`未核；四个公开Sprite不是四份候选选择器，其中18/26/35等背景、Mega Satan等有真实专用绘制分支。实际采用的资源／主overlay／图层外观必须保留；没有采用的分支不强读历史残留。

`+4F18/+4F20`仍只称surface范围：`917870`初始化、`816BB0`及地狱背景直接消费。不能仅由同名BackdropID假定同内容，亦不把这些数值误当Host窗口尺寸。教程controls中的本地按键图标使用C/S本端绑定语义，不让Host设备类型强迫访客显示同一按钮。

旧[tx-04-18背景路径](../tx-04-18/tx-04-18-room-application.md)为墙纹复现保留独立RNG前后态及逐装饰轨迹。这是历史实现事实。`917910→911310→915890`混有装饰Entity生成，`SetBackdropType`只调用Init而不重置流；本轮遵循[Random归属](random-synchronization-review.md)，`+4F28`四字和`+4F38`seed均留Host，不继续作为二进制Room状态下发，也不调用Backdrop.Init来补最终纹理。

正式结果路线必须提供 **实际floor/wall／被选择辅助surface的内容资源**：稳定producer/use/generation、整数内容版本、尺寸／格式和有界内容，或经过完整核定且没有随机／Spawn／玩法的绘制recipe。GPU handle、FBO、control、借用计数、缓存地址不上线。图像名字相同不代表内容相同；只传路径／BackdropID／seed不是闭合recipe。GPU或CPU内容获取的具体合法leaf、读取时点与预算尚未实现，不能把“决定发结果”写成“已有结果reader”。明确空资源、分支不适用、尚未产生与读失败分别表示。

`Room+776C..7893`补列为地狱横向背景连续视图：Room回指、`+4`的Sprite、`+118`活动资格、`+11C`当前显示位移、`+120/+124`速度／累计。`82E960`准备地狱背景；`82F210`按四pass读取当前位移并画；`82EFD0`提供特殊光照背景。**`82EE90`除推进位移还移动房间Entity并调用实体逻辑，禁止Client执行。** 同步活动资格、当前位移、必要实际Sprite资源／姿态；速度／未来累计留Host。紧接的Room+7894是批量动作pending，列在Room本身，不伪称背景字段。此128h视图边界不是SDK独立类sizeof实测。

## 5. FX、水色、光照的实际消费者

`FXLayers=598h`包含`4Ch`层vector、`2Ch`ray组vector、选中组、`+38..83`内嵌完整`4Ch FXPoop`、十根`34h`vein与两个Sprite，尾部为`A0h FXParams`。`7DF9EF/7DF9F2`直接对内嵌FX构造，原36B父级未知归入此子体。动态payload各有独立连续账本；只复制有效／实际采用部分，不能把vector头或所有候选一起发送。

- **FXPose**：`96C620/96DB20/96CE20`读取`+0/+4`位置和`+10/+11`flip，按20B步长遍历；`+8/+C`用于位置生产，也被Host `96E3C0 GetFloorColorAt`的pass4图像采样查询读取。Client不运行该查询补生产颜色，最终Entity颜色另承接；线投影仍10B，不发送8B速度和尾部2B未知。
- **FX层**：保留实际pass1—6、图像资源、sampler／顶点格式、必要blend和有序有效pose。generic pass1/2/3/6才使用mask／blend；非mask且count>0的`+3C`仅与0作条件比较，投影`is_zero`即可，NaN/Inf按原IEEE比较为非零，不把这些位当坐标浮点；mask时`+40`只比较35，投影`is_35`。原始id、未消费参数、未知字节以及未采用分支不上线。pass0／未知pass不能当空。
- **Poop**：`96CE20`用倒计时生成可见性／alpha，曲线在150处分段。传当前present、实际alpha、图像／ColorMod和`+50`限定的有效pose前缀；不传下一帧倒计时、`+54`四个生成参数，不能像旧实现那样整片复制。无poop时不读旧资源／pose残留。
- **Ray**：只发实际选中组（−1无组）的7个geometry浮点和有序ray；未选组不是当前画面。每ray需要实际opacity、当前被消费phase、图像；`96CE20`先提交当前quad，末尾才`phase += speed`，speed原值不需传。Client受控draw阻止／恢复写回，重复显示不累计。原native分支在判断空组前有除成员数，空组不得直接进入该分支。
- **Vein**：`96EA50`先查首根child，再遍历十根。`96EAB0`读kind、三个child、rotation/scale/factor/opacity/depth/position，覆写两Sprite的scale／rotation／颜色；`+30`已核为`96F090`传播的退休／收缩bool，留Host，`+31..33`和`+10..13`共7B仍未知。child转有界索引，循环／无owner节点／预算超限具名失败，不能剪枝后ACK；无图时不读根标量。两Sprite对应被覆写的值可派生，实际资源／帧／非覆写层仍保留。

`FXParams.ColorModifier`由官方`LuaFXParams.cpp`创建副本：它实际是`KColor+brightness+contrast`，不是内嵌六float ColorModState。`818540`还叠房间／pit／lava等规则；这些配置／target／lerp不与第8步Game.current重复下发。`waterEffectColor +74`在`5CD070 Effect.Init`的`5CEB9E/5CED93/5CFA73/5CFAFF/5CFC8E/5CFDD1`取址，已产生Entity的最终颜色由Entity承接，不恢复Client Effect.Init。

**水色必须取实际最终结果。** `816BB0`先取FX默认色，检查seed效果79覆盖；Room.duration<0保持默认分支，=0直接目标，>0用`clamp(1-countdown/duration)`插值。最终color.alpha乘waterAmount，三个RGB乘色按`1+(value−1)*waterAmount`处理。结果为**20B KColor＋3×f32，共32B**；KColor颜色空间标签保留，不能照旧四float颜色假设丢掉。不能把最终32B再次塞回端点后执行原插值，否则会二次乘色／alpha。正式writer必须直接消费具名最终packet或等价的已核纯显示接口。该来源／writer当前没有实现。

`8134C0 GetLightingAlpha`不是直接字段Getter，依赖floor、curse、Player物品与某些Boss Entity；`803E20`的shadow_fill/light_tint还处理pitch-black、darkness、lightning及特殊背景。取Host正常原生路径实际产生的最终值／pass资格，不在Client调用完整Getter补算，不额外调用生产函数获取来源。另保留原有Home特殊Effect166/sub951跨域依赖，不能退回普通背景。`Room+7740`还被`801C9F/801ECA`的GridLight和`81BE0E`纹理坑读取，所以继续作为直接显示值发送。

## 6. Camera、轨道与官方扩展owner

Camera原生94h：六份10h ring、数值区、`+68 Room*`、`+8C shouldOverride`、`+90`跟随参数。`91C730`为六ring分配容量60/90/300/15/4/120；这些指针／历史和本端视口值不复制。Room当前scroll是显示输入；`+11FC`surface原点按本端`(width−338)/2−52, (height−182)/2−52`维护。C/S可维护本地镜头，但不能把“本地镜头”变成丢失Host强制焦点／场景显示意图的理由：`91E920`在某些分支读override和focus ring最后有效坐标，必要时另发optional focus意图；不转发focus历史、设备／Online判断或调用Camera.DoUpdate。原版Online可见性规则不能拿来充当C/S本地参与者判断。

官方`IsClampEnabled`实际来自`ASMCamera.h`的外部`roomCameraData.Clamped`，不是Camera内部某个未知字节；两个ASM patch消费它，Camera析构重置true。这是本地镜头约束，不复制Host内存。`Room.h`另有`roomASM`：ForceSpecialQuestDoor、WaterDisabled与ItemPool override，分别控制生成／初始化／池选择；留Host，实际门、水量和物品结果另承接。这里只核源码owner与职责，没有伪造官方DLL里全局地址或STL ABI。

轨道不是GridEntity。线上保留实际`width*height` extent和0..extent−1的字节，允许256以上；小房尾部不读取，本地明确清255。`9B7C70/9B7F00`每格强制选rail/pitrail并设frame，main动画选择／帧可由flag派生；实际资源、overlay和非覆写层外观仍须承接。SetAnimation会释放／重建索引数组，借用不能只把旧AnimationState头存回而留下悬空数组。既有受控绘制借用与恢复顺序继续有效，不运行Init／Update创建Minecart。

## 7. 未知区、边界与失败方式

机器可读表现给出32个类型／连续视图及各自**直属未知字节**。Room直属未知与EntityList、FXLayers、63份BackdropConfig、各个ANM2、节点及动态payload未知分开计数；不能只看Room根表的数字就声称整个房间只剩这些未知。每类型单行表覆盖声明边界，但没有把未知归Host来制造“全部完成”。续核结果与全部精确余量见[补证第8节](room-residual-evidence.md#8-仍未知的确切范围与处理)：

- Room直属仅28B：`2..3/11F7/120D..120F/1BBD..1BBF/7221..7223/722A..722B/729C..729F/72A2..72A3/745D..745F/776A..776B/7895..7897`。旧864B已核为WORD占格表；旧64B为环境gain；输出容器、死亡历史、雨区域等均已有逐范围用途与禁止复制决定。
- EntityList直属仅`ED..EF`3B；碰撞索引／位存储、敌人计数、shadow标准资源、查询缓存与原版Online32帧环已分开。每个EL的`+1..3`仍未知，列表根的存在不能授权把整400h拷给Client。
- FXLayers直属3B、FX与FXPoop各5B、RayGroup2B、Vein每节点7B、BackdropConfig每项5B；ANM2直属8B／LayerState4B／AnimationState3B等继续未知。BackdropConfig+138/+139窄化后重新暴露尾2B，不用错误u32掩盖。构造清零、copy或检索未命中均不证明填充。
- `+1BB8`已核为死亡名单target，连同done／reward position留Host，实际Effect109标记及奖励Entity／Player结果承接；不是可复制的实体地址。程序纹理、最终water/light/shockwave、反向水面方向及环境voice有明确接线任务，当前没有可冒称已实现的完整reader／writer。

后续实现必须防止：旧room块落到新world；八门别名／负特殊目标索引丢失；无资源／尚未产生伪装成Null；同名程序图像内容错误复用；水色重复处理；FX隐藏分支读残留；动态容器／图结构截断；全Room／Backdrop／Camera Update引发生成或移动；Host图形选项／设备图标／分辨率覆盖访客本地设置；主线程资格失效后继续持有原生指针。当前UNKNOWN禁止宽复制；实际必需消费者有缺口时具名拒绝，不能将其当作“无需同步”然后报告全画面覆盖。

## 8. 完成范围与后续交接

本次完成Room声明范围的连续静态账本、同步职责与269个原room目录行回映射（含Room本类171行、原重载／重复ID）。配置、Level、descriptor、RoomSave、Exploration及第8步转场等委派逐行明示，不计作本轮完成；没有修改旧目录历史归属以伪造过去行为。若原目录把生成查询写ReplicatedState，本表按实际消费者纠正，线上无需保持那个原始Getter返回值。

草稿小记录R0为36B、R1为8B、WaterShaderResult为32B、FXPoseVisual为10B、RayVisual为10B，见[单行状态表](room-byte-ledger.md)。资源目录、引用、存在性、变长图／选中组、最终波纹／light／特殊背景pass尚须第9步正式定稿，因此没有虚报整个Room的固定线大小，也没有生成生产`state/room.json`。

第10步落实合法同world字节／实际显示结果和程序资源来源；第13步落实类型化安装与纯显示pass、图形线程／资源生命周期和借用恢复；第14步退役旧Room JSON／背景RNG轨迹／完整颜色生产态／Camera数值镜像等正式同步路径。第15步唯一统一验收再覆盖房形与448末格／八门别名、所有特殊背景、冷Client／纹理变化、水量／流水／插值／颜色空间／冷却熔岩、无FX／空组／poop／ray／vein、强制焦点与不同视口、重复展示／暂停／换房及雨／轨道不生成本地实体。此前不运行测试。
