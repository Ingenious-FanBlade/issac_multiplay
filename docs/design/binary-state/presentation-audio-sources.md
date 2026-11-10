# 第8步：音频状态与事件的必要来源

**2026-11-10同步职责复核**：[当前决定](presentation-source-synchronization-review.md)及[53项逐来源表](presentation-source-synchronization-review.json)收紧下行范围。每voice最终Options前gain／pitch承接后，layer mask／混音树／ducking／全局pitch不重复；明确world Start／Stop与个人后端成功／失败分开，设备失效／偷占不广播Stop，普通one-shot自然完成由本地收尾。精确硬件游标不属于必需world状态，必要逻辑段落／起点及可靠恢复仍是未决。旧来源事实保持，旧网络推导按本文修订及账本5历史保留；生产版本57，无构建或测试。

2026-11-10；官方REPENTOGON 1.1.3／Repentance+ v1.9.7.12.J273／Win32。按用户要求先查本地Wiki，再查官方SDK／源码，只对未暴露的具体需求补只读静态分析。本文是**来源、字节纠错和后续实现合同**，不是已经运行的音频同步。生产`layout_version=57`不变，未构建或测试；第15步仍是唯一整体验收。

机器目录见[presentation-audio-sources.json](presentation-audio-sources.json)，连续字节见[Presentation账本修订4](presentation-byte-ledger.md)。旧判断、旧未知范围存入账本JSON的`audio_source_revision`，其余UNKNOWN保持。

## 1. Wiki与SDK先给出的边界

| 本地首要来源 | 可直接采用的语义 | 不能据此推导的事实 |
|---|---|---|
| `F:/projects_isaac/IsaacDocs/docs/SFXManager.md` | Play、AdjustVolume／Pitch、SetAmbientSound、Stop／StopLoopingSounds；FrameDelay是再次播放的间隔，不是启动延迟 | Play返回void、IsPlaying按sound ID聚合；均未给出选中sample、独立voice身份、播放游标 |
| 同目录`xml/sounds.md` | 一个sound可含多个sample，path与weight是资源定义 | Wiki称浮点权重；J273解析用atoi写sample+34，而本次907C60正常选样本路径是随机数模sample_count，未读取该weight；不能按Wiki重新抽样来替代Host结果 |
| 同目录`MusicManager.md`、`xml/music.md` | intro／main、layerintro／layer、loop和淡化；queued为空时getter返回current；UpdateVolume使用Options | 单个current ID不能表示交叉淡化中的旧曲目；公开接口不提供精确seek或全部后端状态 |
| `F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/MusicManager.md` | 官方jingle ID／pitch接口及音乐ID校验 | GetCurrentJingleID返回0包含countdown结束／淡出，不能证明后端已停止 |
| 同目录`enums/ModCallbacks.md`的1030／1031／1034／1035／1094 | SFX与Music正常控制回调及取消／改参语义 | 回调不是sample启动确认，亦不提供全部volume／pitch／stop变化 |
| `libzhl/functions/{Music,MusicEntry,SoundEffects,SFXManager,Manager}.zhl` | Manager+29DF8内嵌Music368h，+2A160内嵌SoundEffects18h；Music两个184h组及jingle扩展位置 | SFXManager的size0只是空壳；SoundEffect SDK `_id:+0`误名，真正配置ID在+19C，不能用+0作身份 |

下文源码路径均相对`F:/projects_isaac/REPENTOGON-1.1.3/`。关键实现是`repentogon/LuaInterfaces/{CustomCallbacks,LuaMusicManager,LuaOptions}.cpp`、`libzhl/IsaacRepentance_static.cpp::Music::ValidateMusicID`。正式reader仍是C++原生来源，不调用Lua getter／回调来补采。

`CustomCallbacks.cpp:288`的SFX wrapper在super返回后调用POST，哪怕原生因节流、空配置或无后端source而未启动声音。PRE还可取消或改参数。故1031不能变成无条件Start事件。音乐Play／Crossfade／Layer／Jingle也须记录**实际进入原生的最终参数及实际生效分支**，不以外层请求当结果。官方扩展属于当前支持环境，但第三方Mod适配仍迁出本轮。

## 2. SFX：从真实选样本处产生事件

VA均为J273静态地址，RVA=VA−400000h。`907C60 SoundEffects::Play`按向量slot定位440B SoundEffect；+0写ManagerFrame/2，+4写节流截止，+190是sample数，+194是最近逻辑volume。七个物理sample槽从+8起、步长38h。**同sound可同时存在多个sample voice；同sample再次播放会重用／重载本地source，不能把sound ID或sample路径当一次播放的身份。**

| 正常来源点 | 已核结果与采集位置 | 必须保留的限制 |
|---|---|---|
| `907D2F—907D5A` | 节流成立后才写最近volume／帧；未通过不发Start | FrameDelay留Host，不在网络端再节流或延迟启动 |
| `907D5D—907DC0` | BD0F40私有流选定sample slot；实际base=`effect+8+index*38h` | Client不运行此流；原生weight和配置顺序不能替代已选结果 |
| `907DC5—907E16`、`906EF0` | sample+30为每次Play加载模式；按正常文件加载得到真实SoundSource强引用 | +30=1触发Play内加载，+30=0可能已预载；名称由旧streaming改为load_on_play。资源path元数据与实际已加载资源身份分开，正常解析／打开点记录最终文件 |
| `907E21／907E36／907E69` | 已选source的loop、pitch与`max(modifier,0)*volume`，在乘Host Options之前 | 逻辑增益保留原f32及乘法顺序；Host静音也能保留意图，不反除后端值 |
| `907E9D／907EA0` | 有非空source才调用Play(true)，随后写pending=1／paused=0 | 这是本地执行证据，不能当唯一world Start资格。逻辑资格在通过节流并选sample处分开保存；设备失败不应抹掉world意图。不能声称已出声或精确采样时点 |
| `907EA6—907EAE`→`9F7970` | 该次调用的pan，写入sample+28 | 必须等同次pan完成再提交Start。非mono且pan非0会警告并不下发后端；记录有效分支，不能把未生效pan冒充实际结果 |

`907E76`的后端gain是`min(max(modifier,0)*volume*HostOptions.sfxVolume,1)`；`907F60 AdjustVolume`也有同样上限，`908340 ClearVolumeModifier`重新混音的路径却没有这个显式min。不能把三个来源合成一个未经证明的统一clamp。正式意图保留Options前增益及来源的混音规则，Client按自身音量执行对应规则；有界编码与最终后端合法范围在第9／13步落实，非法／未知值报Gap。

声音改号还可能早于sample选择：`908530 SetAmbientSound`在需要启动时调用`92FAD0`，种子效果7分支把输入sound改为37或301，使用另一个BD0F50私有流（初始化shift 2／7／7，guard C01184）。二者与BD0F40不是同一流。正式事件取**最终sound和sample**；不重放改号helper。新增16B视图记入[Random修订3](random-byte-ledger.md)，两个流均留Host。

## 3. 持续声、调整、停止和自然结束

| 来源 | 真实语义 | 当前状态／事件承接 |
|---|---|---|
| `908530 SetAmbientSound` | volume>0.01时，已playing则AdjustVolume／Pitch，否则Play(loop=true)；低于阈值且playing则Stop | 捕获实际走过的分支，不把每帧SetAmbientSound变成重播事件；原ID与改号后的ID不能混用 |
| `907F60 AdjustVolume` | 遍历该sound的有效sample，改+194及活动source的后端gain | 更新所有实际受影响voice的逻辑增益，不只更新最后选中槽 |
| `908070 AdjustPitch` | 遍历活动sample，写sample+2C及后端pitch | 同样按实际命中的集合更新，不新建voice |
| `908140 Stop` | 遍历该sound活动sample并Stop，节流截止置−1，pending清0 | 对实际voice退休；不能用统一布尔替代并存voice集合 |
| `908210 StopLoopingSounds` | 检查每个活动source实际loop；只停止匹配者 | 停止集合要准确；one-shot和不匹配voice不得顺带退休 |
| `908340 ClearVolumeModifier` | 名称虽为Clear，实际从SoundEffects+10及sound+194重新设置活动source音量 | 不是清空voice、不是重播，源码`LuaOptions.cpp:369`改本地SFX音量后也调用它；本地Options刷新不产生world事件 |
| Manager `92EAE4—92EB3E` | 有Game且未暂停时，SoundEffects+10按+14步长趋近1，随后ClearVolumeModifier重新混音；+14构造初值0.005 | +14核为modifier恢复步长，原4B UNKNOWN撤销；步长和原生平滑器留Host，当前Options前乘数及各voice实际gain由意图缓存承接 |
| `908470 IsPlaying`及正常调整／停止内联路径 | 后端vtable+38查询并可能清pending；属于可写缓存的聚合查询 | 只观察正常调用已经取得的结果，不为采集额外调用getter。其bool不是完整持续声baseline |
| Room `7F5950`→`908530`、`7E0880`→Stop | 16路已核环境音平滑后提交，清房路径逐项停声 | 复用[Room来源](room-residual-evidence.md#4-环境声音原64b不发声音不能消失)，原7084缓存与AAD0E0配置留Host |
| Room `7F5950`末段，sound460 | 延迟10次后Play的是loop=false；后续直接遍历该sound活动source，写pan并调用vtable+50 | **不是仅loop声音才有后续状态**。仍在播放的one-shot的pan／pitch／gain控制也须按voice承接；不能仅Hook公开Adjust方法，亦不能因进入新房就截断此声 |

正常后端vtable+38结果及pending清除已核，但它们不是world逻辑存续的充分来源。同步复核撤销“所有自然完成／偷占／设备丢失都需统一网络结束通知”的推导：普通one-shot本地播完由本地owner收尾，个人设备故障／上限抢占不广播全体Stop。仍须补全world用途、真实显式停止／替换及会话退休来源，并隔离后端查询对Host原生意图生产的影响；不额外调用IsPlaying伪装纯memcpy采集。

设计上，一次性world Start是去重事件，**逻辑voice登记包括仍需控制的one-shot**；持续loop／ambient和音乐当前状态用于baseline。新加入／重连跳过既往one-shot Start；旧实例后续控制也不得凭空新建该声。明确world重启分配新C/S代次，替换旧实例；同sample的本地后端重试不自动视为world重播，SetAmbientSound尤其须按用途分开。跨房存活按真实world Stop／用途生命周期决定，不能一律按room epoch结束。界面按钮／Host个人设置声音留本地，不能转发整个进程所有Play。

## 4. Music：实际曲目、两组声部和jingle

### 4.1 资源和连续字节纠错

本地`xml/music.md`给出intro／main及分层语义；`7D60A0 LoadConfig`对应解析，`7D7763／776D／7776／7783／778C／7791`实际存loop、layermode、mul、层数组、layerfadespeed、层数。MusicEntry60h核为：id(+0)、name(+4)、intro(+1C)、main(+34)、loop(+4C)、layer_mode(+4D)、**UNKNOWN2B(+4E)**、mul(+50)、layer_fade_speed(+54)、layer_defs指针(+58)、count(+5C)。层定义34h含intro／main两个24B string及mul(+30)，新增`MusicLayerDefinition`字节表。只把实际资源内容／已选声部交给Client，不复制原生string、数组或地址。长解析函数有bad-SP告警，字段赋值另以实际指令及消费者交叉核对，不外推全部异常路径。

MusicStream34h的更正：+0/+4是本地SoundSource强引用pair；+8 loop、+9 pending、+A paused、+B UNKNOWN；**+C后端volume，+10 pan，+14 pitch**；+18配置mul、+1C当前层权重、+20目标层权重、+24平滑速率；+28活动维护标志、+29..2B UNKNOWN；+2C是正常Update计数（>10才开始依赖后端状态），**不是播放游标**；+30是本地父StreamSet指针。ctor `9F76E0／7D5860`、load `7D7A30`、update `7D7B90`、pitch `7D5C40`共同支持；Manager构造`92A150—92A17D`明确将每组七个+30都绑定到自身组，jingle父指针保持0。旧账本+0C=pitch和+2C后端游标推断被明确撤销。

直属UNKNOWN：MusicStream24→4B、MusicEntry72→2B、SoundSample16→4B、SoundEffect30→2B、SoundEffects4→0B；新增MusicLayerDefinition直属0B。Music根1B、StreamSet3B仍未知。不能由这些直属数字推导所有后端对象／异常路径已完整。

### 4.2 正常生产与退休

| 正常来源 | 已核行为 | 状态／事件决定 |
|---|---|---|
| `7D8360 Play` | 有合法entry/main后，停止两组，写current／清queued，加载当前组并启动 | 生效后才建立新曲目voice；资源缺失／越界请求不制造事件；同一曲目明确重播也需新代次 |
| `7D8480 Crossfade` | 同ID只改target；异ID令旧组target=0、撤循环，切换buffer、装载新组 | 旧曲目＋新曲目均可发声，必须保留两组各自主声部及6层当前实际权重／资源。currentId无旧曲目ID，须在正常加载时绑定自有voice身份 |
| `7D5CC0`→`7D7A30` | 主流选择intro(存在时)并向后端排main；6层各自选择intro／main，按已核存在性加载 | 实际资源包含intro／main及顺序；后端自动切段的精确时点尚未取得，不能只发主文件或声称当前segment可直接读出 |
| `7D8890／7D8A10`、`7D5E90` | 0..5层mask；模式2可抑制主流，层target由mask给0／1，独立平滑 | 发送最终声部权重／有效gain，mask不能替代淡化中间值；不在Client依敌人数重新决定layer |
| `7D7DA0`（Manager正常`92EB49`调用） | 更新jingle／ducking，两组混音，正常pitch生产与queued接续 | 最终pitch含种子／眩晕等已产生结果；queued只在Host真正选曲后生效，不让Client Gameplay接续 |
| `7D8750 Pause／7D8800 Resume` | 操作当前组主流＋6层；不是遍历所有crossfade组／jingle | 记录实际命中的pause集合；设备／窗口失焦造成的本地暂停不等于world Pause |
| `7D5BB0／7D7D20` | 组／流停止、清强引用、退休；旧组淡至0走正常清理 | 退休voice和全部所属声部；不能靠currentId消失推断旧曲结束 |

### 4.3 Options前的增益来源：部分已核，尚未闭合

`7D7B90`的每流输出由配置mul(+18)×当前层权重(+1C)×全局BE49F8组成；父组存在时再乘组current(+174)。`7D7DA0`先将BE49F8设为**Options.sfxVolume**更新jingle，随后改为enabled分支×jingle ducking(+360)更新两音乐组。因此BE49F8不是恒定MusicVolume，也不是稳定的world字段；jingle确实使用SFX选项，Client应保持这个本地音量类别。

组current／target本身已在多个来源混入Options.musicVolume：Crossfade、VolumeSlide、UpdateVolume及大量Play调用者都直接使用它。例：`6F4B4B—6F4B72`为Options原值，`9389DD—9389EE`的过场分支为**两倍Options**；`938984—9389BF`及`7D8303`又在Play(0)后**直接写组目标／速率**。只Hook Play／Crossfade／VolumeSlide／Fadein仍会漏掉内联写入。

后续需要从这些正常调用者的Options前表达式记录逻辑目标，并在自有、无玩法副作用的音频意图缓存中承接同次平滑及最终gain。已核的unity、2倍、0起步与VolumeSlide参数可作为具名来源；**尚未建立全部原版调用者／enabled写入／内联目标修改的覆盖目录及完整逻辑包络缓存**。Host音量为0时不能从组current反除恢复，更不能把未知补1。表中原生volume字段只说明原生混音事实，不是可直接memcpy上线的world增益。

### 4.4 Jingle不能只读官方ID

`7D8B80 PlayJingle`在current为52或118时不启动；合法entry且有main时才加载Music+32C流、按当前pitch启动，写+364=140。官方`LuaMusicManager.cpp:66`在super后无条件写+32A ID，Lua包装还在返回后写自定义duration；**未生效请求也可能更新这些扩展／倒计时**。必须绑定实际加载／Play提交的同次jingle，而非看到ID改变就重播。官方GetCurrentJingleID在countdown<1返回0，旧jingle淡出仍可能继续发声。

`Music.zhl::StopJingle`直接清+348／+34C／+354、ducking与countdown；没有重放Play或gameplay命令的必要。正常`7D7DA0`依据jingle active／countdown更新背景ducking（趋向0.1或恢复1，另有−100特殊分支），该**当前ducking**与jingle流自身gain不能混作同一字段。特殊−100写入来源与完整world语义仍未核定；当前ducking只作Host最终gain的生产来源，不在每voice最终gain之外再发一份。

## 5. 自有意图缓存、传输与Client应用合同

以下是待实施的设计决定，字段宽度、数量预算与最终线格式在第9步注册；不新增生产类型号。

1. 在已理解主线程的正常音频调用内，暂存实际source／slot关联、最终参数和结果；原函数执行一次。pending／暂停／强引用是本地实现，不作为网络身份。缓存只持自有值与已物化资源引用，网络线程不持有原生指针。
2. C/S voice以session／音频代次和单调实例身份表示；资源可复用，实例不可因相同sound／path合并。事件有独立连续序号、world关联及同次顺序；资源、当前voice修订与对应world提交一致。一个调用先Play后pan，只提交最终完整Start一次；后续实际调整独立更新。
3. 明确world Start／retrigger／Stop／有范围Clear与当前逻辑状态配套。baseline携带可恢复的持续声、并存音乐声部及事件水位；不重播历史one-shot，瞬态jingle不能从头补播冒充中途恢复。重复状态／重传事件不重启；先过期实例后到的参数不得作用于同sample的新实例。只承接有逻辑用途的声部，不发送固定两组×7空槽；后端设备失败不生成world退休。
4. 资源leaf优先使用正常加载已得到的实际文件身份和必要loop／intro-main依赖；配置mul／layer权重／组gain／ducking留Host合成为最终voice gain，Client不再重复混音。不发送后端handle／control／设备、sample随机流或节流截止。SFX的正常非随机执行leaf可沿已选source的vtable +2C Play、+3C Stop、+40 loop、+48 volume、+50 pan、+58 pitch；对象构造／强引用由本地adapter拥有。**这些是已核调用点，不是已封装或实测的跨后端ABI**；不能从这里直接宣布Client adapter完成。
5. Client只执行专用音频adapter，按本地music／SFX选项混音；不调用原生随机SFX Play、SetAmbientSound改号路径、整段Music Update、Host世界选曲／种子效果／切房控制。当前复制状态与本地资源就绪分开，资源失败不可悄悄另选sample。Host个人菜单／音量反馈留本地，原版Online／语音／Isaac Sync不接入C/S。

**相位限制**：共享逻辑事件的顺序／时点和确有用途的intro-main段落需要承接，采样精确硬件游标／设备延迟／PCM队列不属于必需world状态。本轮没有seek／当前segment或可靠续接证据，不得发送+2C计数或任意整数冒充游标，不能把“从头Play一次”称为原相位恢复。具体缺口限于必要逻辑段／起点与恢复执行能力，不要求两台设备逐采样相等。

## 6. 未决、失败方式与步骤交接

| 未决项 | 不能使用的替代做法 | 后续落实 |
|---|---|---|
| 原版音乐Options前目标／平滑的全部正常writer覆盖，enabled混合来源 | 静音时反除、补1、只截公开Play | 第8步继续逐调用者补证；第10步自有逻辑缓存 |
| world音频用途／退休的完整来源、后端结果污染原生意图生产的隔离 | 把设备失败／偷占／每次本地重试当world Stop／Start；额外调用getter补值 | 第10／13步分开逻辑意图与本地执行；普通one-shot自然完成留本地 |
| pan在非mono上的最终有效状态及后端参数约束 | 把请求值或本地失败当Host有效结果 | 固定资源通道元数据与有效分支；未知报Gap |
| 精确seek／当前segment／播放游标及重连续接 | 将MusicStream+2C或ManagerFrame当采样位置 | 第13步补后端能力及恢复语义，现不承诺样本级同步 |
| 全部Play用途的world／个人UI分类、切run和切房存活 | 转发所有Host菜单声、按room epoch一律Stop | 第10步按正常调用上下文绑定唯一owner／生命周期 |
| 过场／视频解码器自带音轨是否独立于Music/SFX | 从已核过场Music调用外推全部影片音轨已覆盖 | 按实际视频owner补查；当前明确UNKNOWN |
| 原生孔及后端动态对象完整ABI | 将清零／对齐当padding，整体复制SoundSource | 本账本仅核已列范围，UNKNOWN与对象边界保持 |

第9／11／12步：正式资源类型、voice／事件布局、预算、重传去重／水位、状态关联及恢复协议仍待落实。第10步：单一常驻Hook协调owner与实际生效分支，缓存的未知、漏事件和来源退休要显式表达。第13步：非随机本地音频adapter、资源生命周期与Options混音、暂停及退休；不把声音命令接回Gameplay。第15步唯一整体E2E覆盖静音／音量变化、节流与被取消Play、多sample并存／同sample重启、ambient跨房与clear、460后续pan、两曲crossfade／六层渐变、jingle未生效／淡出、重复包／丢事件／重连及本地UI隔离；当前未执行这些用例。
