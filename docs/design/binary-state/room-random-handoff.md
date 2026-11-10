# 第7步与Random：原生归属、显示结果与交接核对

2026-11-10；官方REPENTOGON 1.1.3／Repentance+ v1.9.7.12.J273 Win32。**本次完成静态交接核对与同步决定**，不是正式reader／writer、Hook或可玩同步实现。生产布局版本仍56，无构建、游戏操作、随机调用或测试。机器决定表：[room-random-handoff.json](room-random-handoff.json)；[第7步](7-room-level.md)；[Random职责](random-synchronization-review.md)。地址为VA，imagebase=`400000h`，RVA=VA−`400000h`；仅只读IDB。

本次纠正旧D0全局seed重放草稿：**D0仅保留Manager相位和paused，共5B；Pickup／Familiar随机绘制改发正常Host已产生的逐用途alpha和资格**。两个游标不再发送，原13B／37B草稿及旧运行事实保留，正式编码尚未接通。发现并登记独立的音效选sample流`BD0F40`，它同样不下发。挑战32的随机结果还影响地图窗口和描述上下文，不能只理解为一张标记的位置。

## 1. 资料顺序与依据

先读本地原版Wiki：

- `F:/projects_isaac/IsaacDocs/docs/RNG.md`：Next／RandomInt／RandomFloat推进对象；推荐shift index35是通用API建议，不能代替具体原生实例。
- `Room.md`、`RoomDescriptor.md`、`Level.md`：随机位置／格、四seed、生成流、房间与描述身份。
- `SFXManager.md`：`Play`的FrameDelay是**再次播放的抑制间隔**，不是延迟到将来才开始；SetAmbientSound和AdjustVolume／Pitch不能混成一次性Play。
- `enums/Challenge.md`：32为APRILS_FOOL。

随后读`F:/projects_isaac/REPENTOGON-1.1.3/`内的Wiki和源码：

- Wiki `docs/docs/{RNG,Room,Level,Backdrop,FXLayers,EntityPickup}.md`。Backdrop明确floor／wall Image是当前房间纹理buffer；Room.SetBackdropType说明按房间seed选变体，但没有保证整个Init无生成副作用。EntityPickup文档另说明SetupCollectibleGraphics有默认Random参数，不能拿公开helper补最终图像。
- `libzhl/functions/{Backdrop,EntityList,ANM2,SFXManager}.zhl`及`repentogon/LuaInterfaces/LuaLevel.cpp:336..337`：对象关系、ColorMod偏移、API签名和Game/Level generation别名。
- `libzhl/IsaacRepentance_static.cpp:588..596`与`repentogon/Patches/ASMPatches/ASMHud.cpp:207/223`：Error纯整数计算及真正颜色比较。目标生成头明确NUM_TRINKETS=190、ERROR=75、MODELING_CLAY=166。

这些资料没有给出绘制流相位、临时alpha、音效实际选样本和地图内部窗口规则，再用下文J273静态消费者补证。静态用途、业务名、源码声明和运行事实分开；本轮没有新运行结论。

## 2. 需要、不需要与禁止同步的集合

| 范围 | 必要同步结果／保留项 | 不下发或不应重放 | 责任 |
|---|---|---|---|
| Game/Level+182D4、+182E4 | 已生成拓扑、配置、门／实体和最终概率显示 | 两条独立16B流；GetGenerationRNG别名不另建owner；不恢复生成器 | 第7步／Entity／Presentation |
| Backdrop+4F28流与+4F38 seed | 实际floor/wall／采用surface内容、Sprite、真实装饰Entity | 前后RNG轨迹、构建seed、Init／SetBackdropType／装饰重抽 | 第7步资源／Entity |
| FX、雨、ray、vein | 当前有效pose、已选组／图、当前phase／opacity、实际雨Entity及声音 | 速度端点、候选过滤、生成区域、未来控制／随机、混合Update | 第7步R2/R4与Entity／音频 |
| descriptor生成／奖励／商店／保存 | 当前货物／Price Sprite、必要地图聚合结果 | 四seed、商店类别／补货次数、保存随机状态和Restore | 第7步／Entity／地图 |
| Error颜色 | 最新Descriptor方案的D1 **1B `error_is_modeling_clay`**，或已完整承接的最终颜色；实际Image另发 | 完整效果ID、原SpawnSeed、恒真`effect>0`、HistoryHUD.Recompute与任意getter字典 | 第7步当前颜色上下文＋Presentation／Player |
| 挑战32地图 | 已选SafeGridIndex锚点、对应描述稳定关系及地图显示版本 | DecorationSeed、栈RNG、再次选房、只传孤立标记遗漏窗口上下文 | 第7步／Exploration／Presentation |
| 地图震动 | 正常Host最终offset Vector，8B，沿用既有owner | timer、全局随机、整个Minimap.Update与Host查看按键 | Presentation |
| 音效／环境声 | 实际已选sample、逻辑voice及gain／pitch／pan／loop／生命周期 | 选样本流`BD0F40`、soundID随机Play、Room平滑数组、Host Options后的后端音量 | Random排除原流；Presentation承接结果 |
| Pickup／Familiar绘制随机 | 逐subject/use/shape/pass/occurrence的实际资格和alpha f32 | `BD0CA4/BD0674`游标、shift、guard、全局顺序复演、Host颜色栈 | Presentation具名结果批次；第7步交接显示关系 |
| Room显示关系／时钟 | 实际成员和必要有序关系、同world GameFrame／帧基准／Manager相位、必要Greed timer | EL地址、碰撞／查询／原版Online历史、Client墙钟补推进 | 第7步／Entity／Presentation |
| 水面方向／Grid Light | 实际reversewatergfx或等价phase；第6步已核Light SpawnSeed及上下文继续保留 | 不能因名含seed就删除Light字段，也不能推广为Backdrop生成流例外 | 原有Grid／Room／Run单一owner |

Host生成字段不下发，其产生的当前画面仍须有结果。结果已由其他域承接时引用同一owner，不新增一份Random网络记录，也不为没有负载的流创建空记录。

### 2.1 生成和背景的具体证据

官方LuaLevel直接返回`&g_Game->_generationRNG`。原版`853542/878DB8→74D6F0`返回`this+182E4`，`743970`正常消费恶魔／天使流；两者不是同一流，也没有第二份“Level generation网络身份”。

`917910→911310→915890`混合纹理选择和Entity生成：`911310`直接推进Backdrop+4F28，并可调用`424340`生成Effect7；`915890`还可生成Effect121／140、使用全局Random并设置实体颜色／姿态。即使两端初始seed相同，也不能把此链当作纯贴图重建。实际内容资源或完整已核纯显示recipe才是背景交接；当前具体像素leaf／资源安装仍待实现。

FX未来速度／抽选过滤与当前pose分开，ray draw末尾的phase写回也单独隔离。水面`BackdropConfig+139`直接反转当前时间相位，不是RNG，不能随生成配置一起省掉。Room的两张448项path／NPC占格表、死亡名单／历史、输出函数表和雨区域继续留Host，真实后态另发。

## 3. D0：以具体结果解除全局游标重放依赖

### 3.1 为什么撤销两个seed字段

`EntityList.zhl`把renderEL放在+50。原生`413D00`按GetRenderZ插入；当前Client的`client_presentation_native.cpp::FullSort`则按Z／authority_index全排。两者以及后续原生排序、实际可见分支和递归不构成已经证明相同的消费序列。旧`ClientEntityFrameScope`从本地renderEL取一组普通窗口输入，再借用两个全局流连续绘制；这只说明共享了一组输入，不能证明每个subject分到相同样本。

`Room+12BC`是EntityList相对+A4的pass scratch，**不是房间类型**。Pickup随机分支明确检查该值不等于5。Familiar还先检查真实Player owner的`7D1C90`谓词；其内部读取Player类型29／38及身体关系／索引。普通world快照、GameFrame或某一renderEL都不能单独表示这些实际调用资格和先后关系。

因此正式设计选用两个具名结果来源；不再把“将来证明全局顺序一致”作为seed方案继续存在的理由。显示成员／深度顺序仍是必要画面状态，但随机结果不再由Client访问顺序分配。

### 3.2 两个结果的精确生产点

| 用途 | 正常Host资格／生产 | 最小结果与Client消费 |
|---|---|---|
| Pickup alt pedestal alpha | `6E35D0`先检查exists+172、visible+171、dropDelay+554<=0；variant100还受+594、+6D9及Room+12BC!=5等条件限制。`6E37CC`写BD0CA4，`6E3805`写Pickup+684 | +684=`alt ANM2+5D0 + ColorMod+A8 + alpha+C`。正常写入后、alt draw前复制4B；Client用已确认资格进入对应纯显示段、临时借用alpha并恢复，不再执行原生随机／资格查询补值 |
| Familiar color-stack alpha | 正常`61E360`根据owner谓词决定是否推进；`61E5A3`写BD0674，`61E604`写当前栈ColorMod.alpha，`61E60D`调用402A70，函数末对称pop | 复制实际f32，不发送44B栈对象。402A70将RGB=1、该alpha及其它中性项压本地颜色栈；Client按同用途资格建立本地中性ColorMod＋结果alpha，并对称pop |

两处最终f32约为`float(u32)*2^-32*0.15f+0.05f`，实际SSE转换／舍入结果直接保留，不要求Client复算公式，不以严格小于0.2的数学区间误拒f32端点。Familiar的结果在栈上，普通后态对象无此字段；Pickup+684也可能是未进入该分支留下的旧值，不能只因地址存在就当本次结果。

不额外调用Host Render、Next、RandomInt、Phantom或getter制造结果；正常Host的流照常推进，不恢复Host生产状态。Client绕过这两个具名随机段，其他Render／颜色／资源／Online隔离依赖仍须满足，不能据此放行整个混合函数。

### 3.3 状态布局与批次身份

| +0 u32 manager_frame（4B） | +4 u8 paused（1B） |
|---|---|

**D0=5B**。GameFrame仍在world帧头；既有subframe按paused与Manager相位派生，Room帧基准保持原值。D0取该正常Host展示批次实际采用的Manager相位／暂停资格，不能把普通模拟后态的值与另外一次Render结果拼合。旧13B原表保存在Random JSON的`wire_history`，37B历史设计说明保留在Random主文；本轮不更改生产整数版本56。

随机绘制标量记录是带条件的单行表：

| +0 u8 state（1B）：0=Host已确认本用途无随机覆盖；1=Host已产生alpha | +1 f32 alpha（4B，仅state1） |
|---|---|

最大**5B标量payload**，外层subject/use/shape、pass、重复调用身份、变长容器与批次头另计；没有虚报完整网络记录为5B。该表是新设计，尚无生产layout号。

记录须绑定同run/world、实际state revision、presentation revision、subject/use/shape、资源版本及具名pass／调用身份。同一subject在多个pass或递归中有多次正常消费时分别保留，不折叠为最后一次，也不用本地第N次访问猜Host第N次访问。由第9步统一编码这些结构引用和预算。

**state0仅表示正常Host已确认无该随机覆盖**，不表示实体或整个draw不存在。正常Host未执行、尚未产生结果、生命周期变化和读失败均不编码成state0或alpha0；Client新可见调用没有对应已产出结果时报告来源缺口。不能把一次未经过的调用推断为“应抽哪个值”。

只在正常生产点即时复制到自有缓冲；已产出结果附着其真实来源的world／资源修订，不能仅凭GameFrame相同贴到更晚状态。完整展示批次未就绪时不ACK最新不完整状态；Authority tick继续独立推进。重复展示复用同一不可变结果批次，不累计游标；换房、use退休或资源变更撤销旧结果。具体Hook、结果缓存、预算及Client窄分支adapter仍待10／12／13步实现。

## 4. 地图与Error：只保留真正消费者所需结果

### 4.1 挑战32不只是随机图标

`9635B0`在challenge32时以当前descriptor.DecorationSeed构造栈RNG，shift为`(1,5,16)`；`9636C7`按活动描述数量RandomInt，再取选中描述**+4 SafeGridIndex**。它不是单纯拿当前GridIndex画一个偏移点。

该值随后被原生钳到`0..169`，进入当前维表取对应描述，并参与地图窗口左右／上下裁剪、中心和描述上下文计算。因此必要结果是**已选地图锚点及其对应描述稳定关系**，连同地图配置、维度、L1与L2显示版本交接。锚点标量可用i32共4B表达；描述关系、资格和外层记录另计，4B不是整个地图状态大小。

| +0 i32 focus_safe_index（4B，正常Host已选且处理后的地图锚点标量） |
|---|

原生上界169不能被解释为13×13数组的额外合法格；第10／13步必须从正常Host实际已选／已采用上下文取得稳定关系，用具名安全adapter翻译，不能直接拿169写Client维表或调用有fallback／随机的查询。无可解析对应关系时保持具名缺口；完整布局未知不伪造为已实现支持。

L2仍沿用最新Descriptor核定的**最终icon mask及必要最大帧，存在3—12B**；完整保存元素、数量、顺序及RNG不下发。它不能代替挑战32的焦点选择，焦点选择也不能代替L2图标。地图最终shakeOffset另由Presentation取8B结果，本地查看输入／排版／视口不随Host按键镜像。

### 4.2 Error沿用最新1B颜色谓词

官方纯u32计算为先`seed ^= seed>>2`，再`seed ^= seed<<7`，再`seed ^= seed>>25`；非零原seed返回`seed%189+1`，零原seed返回75。不推进对象RNG；目标版所有返回值均>0，该比较无动态负载。

Host已选Image另有资源owner，颜色剩余需求仅是结果是否166（ModelingClay），故D1是**1B bool**，不发送2B完整效果ID或SpawnSeed。Client颜色adapter还需真实Player ModelingClay当前效果／最终颜色；如果Presentation已完整发送最终颜色，这1B不再重复发送。补货等正常Host路径可能改变SpawnSeed，谓词按变化更新，不能只在进房采一次。

| +0 bool:u8 error_is_modeling_clay（1B，仅必要当前颜色上下文） |
|---|

这项只供具名颜色消费者，不制造假的GetErrorTrinketEffect返回值，不全局开放所有相关玩法查询，也不重跑HistoryHUD.Recompute选图。初稿2B方案的纠正由[Descriptor核对](room-descriptor-synchronization-review.md)保留。

## 5. 环境声音：新核到的私有选样本流

`907C60 SFXManager::Play`正常通过sound可用性和FrameDelay抑制后，使用`BD0F40..BD0F4F`的独立16B xorshift。初始化shift为`(5,27,8)`，guard在`C01050`，均为本进程私有。`907DA7`保存新seed，随后对实际sampleCount取余，`907DBB`后已决定所用sample。该流不是BD0CA4／BD0674，也不是CRT或全局Random；本次在Random账本新增其字段视图，未伪造整个SFXManager ABI。

`908530 SetAmbientSound`可能查询当前播放状态：已有声音时AdjustVolume／Pitch，否则进入907C60而重新选sample，低音量时停止。因此不能将“同步soundID并在Client调用Play／SetAmbientSound”当作实际样本同步。

必要数据为实际sample资源、逻辑voice身份／代次、loop、开始停止／clear、持续相位及最终**world逻辑**gain／pitch／pan。soundID只是一类声音，不足以区分同时活动sample或重用后的voice生命周期；映射用C/S稳定身份，原生sample地址和后端handle不上线。

`7F5950`已核Room16份平滑gain和sound460计时。逻辑音量来源必须在907C60原始Volume或907F60正常AdjustVolume等生产点保留；后端sample+44已乘SFXManager／Options并钳上限，不能从它反除恢复逻辑量，静音时尤其没有逆运算。pitch在908070正常写入；sound460的pan由7F5950正常计算并写sample+48，还检查mono限制，不能将未生效的后端写入假装为已播放声像。

Client使用已选sample的本地资源和自己的音量／静音策略；不推进BD0F40、不调用会重抽的原生Play。持续声baseline恢复当前voice而不重播一次性事件，重复状态幂等，停止／clear与换房实际生命周期分别处理。正常生产点与职责已经具名；完整逻辑意图缓存、sample／voice关联和非随机播放leaf尚未实施。

## 6. 已完成的核对与后续实施

本次15项交接在JSON逐项给出原生范围、同步决定、Host正常来源、Client消费者、禁止复制／重放项与剩余实现。当前源／消费者和网络职责已定，后续工作是按此合同实现；没有把“代码尚未写”继续称为RNG归属未决，也没有把静态决定写成运行接线完成。

- **第7／8步设计**：按本次结果修订Room／Random／Presentation；D0 5B、逐用途随机结果、D1 1B、挑战32锚点＋描述关系、实际sample／voice分别交接；配置完整类型、RoomSave／Exploration目录仍属其原范围。
- **第9步**：注册正式布局号、条件记录、批次与结构引用／预算；不把本次草稿直接当版本56线数据。
- **第10／12／13步**：正常Host生产点缓存、完整批次归属／资格、typed解码与窄显示adapter、资源／颜色栈／CPU队列生命周期和本地音频恢复。未产生的显示结果保持缺口，不能额外抽取或补画Host。
- **第14步**：正式结果接通后退出旧application_draw全RNG／两seed方案、背景RNG轨迹、Client随机Play、随机地图Refresh等正式路径；历史事实保留。
- **第15步唯一整体验收**：覆盖零额外随机消费、首次／重复／暂停／换房／多pass／递归、不同成员顺序与可见分支、缺结果／资源换代、Familiar颜色栈平衡、挑战32窗口与标记、Error→ModelingClay颜色变化、多个sample／voice、静音／跨房／停止／持续声baseline。当前未执行这些用例。

Room直属28B、Level直属22B及所有嵌套／保存体／图形后端未知继续保留；最新Descriptor直属0B不等于完整保存体、资源或运行行为已知。不为Host私有对象穷尽无下行用途的内部ABI，也不把“不发送”解释为“已经全部核清”。
