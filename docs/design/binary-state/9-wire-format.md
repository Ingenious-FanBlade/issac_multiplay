# 9：线格式

**2026-11-11 第9步各域正文实现，版本60**：[正文实现与边界](9-domain-body-codec.md)、[同源数字计划](layouts/09-wire-format/body-codec-plan.md)。各域正文与12B记录编解码已实现，布局版本60；公共结构尺寸沿用59。新state/body_codec.*使用同源state_body_codec.inc覆盖337布局／2763字段／79条件，输出块／可选根字段视图与投影事实；缺base或资源仍不得安装／ACK。完整typed资源、用途允许集、Frame装配与会话接受入口仍待后续。

**2026-11-11 第9步第一项完成，版本59**：[公共报文结构正式合同](9-public-wire-contract.md)、[同源固定头／常量表](layouts/09-wire-format/README.md)。保留u32块掩码与12B记录头；50B公共上下文、1／2分段共同提交、按Peer body确认、u32 GameFrame、显式资源空间、结果组／贡献／事件外层已入源。三端总上行10Mbps、模组持续7.5Mbps；32MiB仅为旧可靠大消息／解码内存保护，普通帧按实际预算发送。完整资源payload、用途允许集、接受规则与运行编解码仍待本步后续；第15步唯一验收。以下较早交接保留原时点，结构冲突以正式合同为准。

**第8步版本58交接**：[正式结果与生成合同](implementation-8-presentation.md)已登记310—407、28条来源／序列／窄依赖合同和141＋54行回映射。这里继续落实批次公共上下文、贡献／事件身份、Clear范围、固定shader／blend／sampler允许集与audio-table资源payload；不得新增Presentation确认序列，也不得把元素表当完整可接受报文。

**2026-11-10 显示批次编码定稿交接**：[用户确定的方案B](presentation-result-batches.md)保持world／body两序列，按实际owner分组并声明共同提交范围；正式封装须表达Host来源修订与逐Peer目标版本映射、subject/use/shape、组／身体显示选择、资源／拓扑依赖及多pass／layer／递归贡献身份与顺序。History／Stat原生两槽不是Peer容量，各端默认主体为本端控制组；全部必要来源缺口与资源Wait不能编码成空值或早ACK。精确字段宽度／布局号／预算仍由本步同源登记；版本57不变，无构建或测试。

**2026-11-10来源同步职责复核交接**：[当前53项决定](presentation-source-synchronization-review.md)选择屏幕／转场最终有序贡献和每voice最终参数；不再额外编码Nightmare路线／Scene帧／Dogma、音乐layer mask／混音树／ducking／全局pitch、重复Run／D0与本地尺寸。资源按需声明，recipe／内容择一，world事件与本地后端状态分开；精确硬件游标非必需字段，逻辑段落／起点恢复仍待来源。正式kind／布局与预算不由本轮变更，版本57，未构建或测试。

**2026-11-10 第8步音频来源交接**：[30项来源及未决](presentation-audio-sources.md)、[机器目录](presentation-audio-sources.json)补到实际sample与有效Start／调整／停止、sound460内联pan、两组音乐各7声部和jingle。正式编码须区分资源与voice实例、Options前gain及来源混音规则、仍需控制的one-shot与可baseline恢复的持续声；事件含独立序号／水位，重复state不重启。MusicStream+2C是Update计数，不能编码成游标；精确seek／segment／重连相位及全部音乐逻辑包络仍未核定。音频资源kind、布局号、字段宽度／预算和完整恢复封装待本步实施，不从来源目录臆造可接受报文；版本57不变，无构建或测试。

**2026-11-10 第8步资源／屏幕来源交接**：[具体来源与结果合同](presentation-resource-screen-sources.md)补核RGBA8、内置shader表及固定pass格式、前后surface、Nightmare条件内容和BossOverlay。9.10.6中的“Nightmare条件payload／运行模式显示owner完全未核”由该文的具体结论修订：条件结果来源已核，模式4为原版Deathmatch而排除，Pause仅密码1B排除；其余孔仍UNKNOWN。正式结果布局、固定shader资源表示与typed表项kind的对应、完整recipe／blob分派和预算仍待本步实施，不把来源目录当可接受报文或恢复Mod shader旁表依赖。版本57不变，无构建或测试。

状态：公共结构与各域正文已实现并同源生成；完整资源payload、整消息装配、详细接受规则与运行接线未实施。上级：[总计划](README.md)。

**2026-11-10 各域编码交接统一整理**：现行交接入口为 **9.10**，逐项列出来源、特殊编码规则、版本依赖和待落实项；显示／资源、RoomSave／Exploration与其余域按同一结构登记。本次静态审阅读取的唯一布局源整数版本为 **57**，已包含同期登记的Room／Level及部分显示／资源值布局；完整消息、Presentation其余投影和运行接线仍分项列为待落实。本次只整理文档，不修改布局源、生成表或实现，不构建、不运行测试；原版本提示移至文末历史说明。

## 目标

定义原版状态帧、记录、块、成员表、资源表的二进制格式，以及 Client 的有界校验规则。传输层（通道、分片、补片、合包、zstd、确认）沿用现有实现，只替换正文。第三方 Mod 值编码已迁至[独立后续方案](../mod-adaptation.md)，不属于本步实现或完成条件。

## 输入事实

### 现有传输层

- **传输封套**：64B分片头＋40B逻辑消息信封；前者固定64字节（`mod/src/replica/stream_codec.h:20-28,57`）。
- **报文与分片**：报文最大 1200 字节，分片正文 1136 字节（`stream_config.h:21`）。
- **保护上限与速率**：配置中的world 32MiB是可靠大消息／内存保护，普通非可靠world/body还受EncodeBlob的64KiB收紧；均不是发送额度。三端总上行10Mbps、模组持续7.5Mbps，按最终分片／合包／重传字节计费，见[正式合同第9节](9-public-wire-contract.md#9-带宽消息保护与准备预算)。
- **通道**：`Control`、`Baseline`、`Context`、`WorldDynamic`、`PlayerFast`、`Input`。其中 `Baseline` 和 `Context` 是可靠通道（`stream_transport.cpp:62`）；`WorldDynamic` 和 `PlayerFast` 只保留最新。
- **消息种类**：`ScenePrepare`、`Range`、`ReceiveAck`、`ContextReady`、`AppliedAck`、`Resync` 等（`stream_codec.h:6`）。
- **现在的正文**：`Range` 的正文是带字段编号的树形编码，固定 payload schema=2（`stream_codec.h:47`），必须先还原成范围模型才能使用。
- **身份的数据类型**：key 是 `{run u64, space u32, id u64}`（`mod/src/replica/stream_model.h::Member`及`SubjectKey`）；use 和 shape（revision）都是 u64。

### 各域资料与当前状态

- **唯一布局源**：[`version.json`](../../../mod/binding/state/version.json)当前为60，公共结构尺寸沿用59；包含前13份域源与新增`wire.json`，共14份；公共固定头408—421及304／306的更新与常量同源生成。字段、布局号、条件、序列和预算以源及[生成表](layouts/README.md)为准，正文编解码已实现，完整消息运行入口仍未接通。
- **设计来源与同期入源**：第2—8步及其职责核对、字节账本、补证文档提供语义和未决项。原资料中的版本56“Room／Level／新显示结果未注册”保留其时点；[第7步版本57实现说明](implementation-7-room-level.md)与实际`room.json`／`level.json`提供同期入源范围。本清单据此区分已入源值布局、待定完整消息与尚未注册结果，不宣称第9步协议已接通，也不让旧版本提前接受新字段。
- **版本含义**：布局版本、类型布局编号、成员／资源epoch、world／body版本及专用拓扑／显示修订分别承担不同职责。布局已经生成不表示第9步报文或第10—13步采集、发布、解码、安装已经接通；历史版本不作为当前兼容分支。

## 设计

### 9.1 通用编码规则

- **字节序与对齐**：小端，紧密排列，不对齐。解码一律用 memcpy 读取。
- **基本类型**：
  - 浮点按原样位模式发送。Client 校验时拒绝 NaN 和无穷大，除非布局声明该字段允许。
  - bool 写成 u8，只允许 0 或 1。
- **可变长内容**：
  - 字符串写成 `u16 字节数 + 原字节`。
  - 普通容器默认写成 `u16 元素数 + 元素`。具名布局显式声明`count_type`时用该宽度，例如MapBadgeList／VeinGraph节点的u8；真实固定槽、条件／选择记录及资源blob也按显式布局编码，不统一添加计数头。例外及其来源见9.10。
  - 两者的上限都来自布局，超出上限即为整帧失败。
- **标识编码**：
  - 实体或格子引用写成 `u16 成员序号`，`0xFFFF` 表示 Null。
  - 资源写成 `u16 资源序号`，`0xFFFF` 表示无。
- **不计算哈希**：兼容性只靠布局版本号；完整性依赖传输层现有的分片和长度校验。
- **条件与空值**：存在位、条件选择器和mask只控制已登记的分支；条件不成立时不读旧payload，也不消费其字节。已确认Null／缺席／空集合与来源Gap、资源Wait／失败分开，后者不能编码成前者。
- **边界与预算**：数量先按原生宽度检查再窄化；元素数、字段／投影字节数、整记录／帧及整上下文预算同时适用。各小记录的大小不含外层身份、版本、记录头或依赖目录，不能直接用作完整Baseline大小。

### 9.2 消息种类与公共上下文（定稿）

正式Kind：21 StateFrame、22 MemberTable、23 ResourceTable、24 ResourceBlobChunk；ContextReady8、AppliedAck9、Resync10保留编号。路由头和信封不变，新正文统一以StateCommonHeader408的50B开始，包含u16 layout_version及完整WorldScope六个u64。这里只登记数字合同，旧stream_codec／握手／合包尚未接通新Kind，不能提前接受不完整正文。

完整顺序、通道及生命周期见[正式合同第1节](9-public-wire-contract.md#1-封套公共上下文与消息编号)。

### 9.3 状态帧与分段头（定稿）

`公共头50B + Frame头409(2B) + 1或2个分段`。单分段可为world或body；双分段固定world随后body，整个目标共同提交、共同ACK，承接Baseline和ordinary body-joint。

每个分段为`Section头410(52B) + 成员记录 + 结果组 + 音频事件`。头部包含stream／flags、member/resource epoch、base/target、Host source_revision、presentation_batch、u32 GameFrame、三类计数及event_watermark；字节偏移见[同源表](layouts/09-wire-format/StateSectionHeader.md)。Host来源修订与逐Peer目标版本分开，不新增Presentation确认序列。完整语义见[正式合同第2节](9-public-wire-contract.md#2-stateframe与两个分段)。

### 9.4 记录头（保留12B与u32掩码）

| +0x0 u16 member：精确成员表快照内的成员 | +0x2 u16 layout：与该成员登记一致 | +0x4 u32 block_mask：本记录携带的真实块 | +0x8 u32 record_bytes：头后正文精确长度 |
|---|---|---|---|

块按编号升序排列，最多32块号0—31，含父类和显式空预留；预留块不发送。当前旧323布局最多25块，32够用；它不限制每帧记录数。同段不得重复member，块只允许所属序列；条件／容器／固定槽严格按生成布局消费，record_bytes必须恰好用完。未携带块只从明确base继承，不能跨shape/use／拓扑拼装。见[正式合同第3节](9-public-wire-contract.md#3-记录头与块)。

### 9.5 成员表（定稿10B表头／33B表项）

两表共用412：`epoch:u32 + previous_epoch:u32 + entry_count:u16`；previous=0为完整表，其余必须对应前一快照。

成员项413：`member:u16 + op:u8 + space:u32 + layout:u16 + id/use/shape:各u64`，run由公共头继承；0加入、1换形、2离开。表安装只构建元数据快照，不能立即退休已展示对象；成员操作随合法目标提交，精确epoch不得被最新表替换。完整语义和回收规则见[正式合同第4节](9-public-wire-contract.md#4-成员表资源表与blob)。

### 9.6 资源表与blob（公共壳定稿）

304资源头现为8B：`table:u8 + index:u16 + kind:u8 + payload_bytes:u32`；306 blob头现为15B：`resource_epoch:u32 + table:u8 + index:u16 + offset:u32 + bytes:u32`。原号保留、版本59更新，不保留同义头。

八张资源表使用明确u8编号，typed字段已知空间时仍只传u16引用；FFFF Null与非Null资源Wait／失败分开。一个run内同表资源号只增不改，内容变化分配新号；表到达、内容收齐、依赖ready与物化成功分别记录。完整payload、table/kind矩阵与shader／blend／sampler允许集仍待本步后续，不能仅因固定头生成而接受新资源。见[正式合同第4节](9-public-wire-contract.md#4-成员表资源表与blob)。

### 9.7 结果组、贡献、事件与当前范围（公共壳定稿）

414结果组42B、415依赖32B、416贡献定位16B、417音频事件40B共同表达owner／subject、实际生产修订、显示实例代次、具名依赖、Replace／Reference／Retire、多pass和事件去重／明确Clear范围。现有365／366 typed贡献只编码一次；不按Client访问计数匹配。详见[正式合同第5—7节](9-public-wire-contract.md#5-结果组缓存与依赖)。

第三方Mod自描述值、任意GPU命令、函数／getter参数字典继续不在范围内；未知来源、旧UNKNOWN及音频恢复Gap保持。

### 9.8 Client校验与接受规则（结构原则已定，详细实现待后续）

完整目标必须同时满足：整数布局版本、完整scope/route、精确表快照、base/target、生成布局／mask／长度、具名用途允许集、全部必要来源／资源／拓扑及共同提交关系。Gap／Wait不能规范化为空；无完整合法目标不安装、不ACK。

ContextReady只确认元数据可解析；AppliedAck在原生提交成功后才确认目标。双分段一次确认两项，事件只随world回报连续水位；不是单独Presentation ACK。Host只接受实际发送目标的准确回报。普通Resync以base=0恢复、保持target单调；版本将回绕时须先退休旧复制代次，不能在原路由重置。见[正式合同第8节](9-public-wire-contract.md#8-确认基线回绕与接受边界)。

### 9.9 版本序列与预算（定稿）

- world和body独立，body按Peer覆盖其全部身体；不按身体另建序列。
- Player P0和已安装P6须使用相等的u32绘制拓扑，保持成员／资源与相关结果的窄依赖；双分段结构事务明确共同提交，不扩大普通body对整个地图的依赖。
- GameFrame保留u32，来源与事件身份用u64；不能以GameFrame代替来源修订或Peer目标。
- 三端总上行10Mbps／模组持续7.5Mbps。每Peer world120000B/s、body120000B/s对应30/60Hz平均4000/2000B，另留控制／可靠额度。首次准备状态、表和资源合计≤1MiB、≤10秒，所有发送及重传均计费；32MiB旧保护不是正常world许可，64KiB普通消息保护也不是常态发送额度。见[正式合同第9节](9-public-wire-contract.md#9-带宽消息保护与准备预算)。

### 9.10 各域编码交接清单

以下按第1—8步的来源统一核对，不以页首修订提示代表覆盖范围。表中的“已入源”只指布局与生成元数据；“待注册”指设计已有、生产schema仍未登记。第9步负责将现有决定落实为正式有界线格式；第10／11／12／13步分别承接来源、发布、解码和应用，第14步退出旧链，第15步才执行统一整体验收。公共外层已经版本59入源；表中第2—8步来源事实按原时点保留，外层尺寸／编号以本页9.2—9.9及正式合同为准，后续运行实现与测试尚未进行。

#### 9.10.1 布局源、基础类型与公共封装

| 范围与来源 | 特殊编码规则 | 版本／提交依赖 | 待落实项 |
|---|---|---|---|
| 布局与封装：[第1步](1-layout-source.md)、[第0—2步实现](implementation-0-2.md)、`state/*.json`及9.2—9.9 | 父块号、空预留块、最多32块及生成字段顺序保持；空块不要求发送零长字段。`native_view_of`／`nested_views`按声明选择具体子布局，报文不提供原生偏移、大小或函数。Control／Input保持原职责，不将输入命令或结构操作冒充状态字段 | u16布局版本与u16类型布局编号分开；成员key/use/shape、member/resource epoch及base/target版本按各自语义使用 | 公共头408—421、资源头304／306和消息编号已按版本59生成；body按Peer／GameFrame u32已定。各域正文已在版本60实现；完整资源payload、严格接受规则与整消息运行入口仍待本步后续，不把固定头当可运行协议 |
| 纯值与动画：[第2步](2-base-types.md)、[动画／资源复核](animation-resource-synchronization-review.md)、`base.json` | Vector8B、ColorMod44B、KColor20B、BlendMode20B按真实字段消费；专用裁剪视图不反改通用类型。AnimationState保留独立selector、帧、playing，ANM2保留playback_speed；事件位和ColorParams队列不恢复传输。资源有无并入可空引用，Host loaded原字节不发 | selector依据同资源／动画上下文，不能以Sprite总层数替代当前动画层数；路径元数据与实际Image独立，资源表收到不等于图像ready | 编解码递归使用生成布局及具名校验；资源／shader的具体owner接线见9.10.6，不靠重播动画事件或Update补值 |
| 容器、共享payload与引用：[第1步1.3](1-layout-source.md#13-区间合并规则)、[嵌套规则](2-base-types.md#21-嵌套规则)、[正式Player生成能力](implementation-5-player.md) | 普通容器每投影默认u16 count，显式count_type例外按源；同容器投影计数一致，不同容器仅按声明的count_groups求和，不强行等长。fixed_count真实固定槽不附count；aggregate的stride=0，不把聚合pair当map节点。shared-owner按具体present／投影编码，不复制holder、vector/deque头；引用为u16成员号／FFFF，目标空间显式声明 | 同目标版本的存在性、投影计数、计数组、真实引用及资源闭包齐备后由同一owner提交；有效缺席可用布局定义的最短编码，来源失败不能伪装缺席 | 消费生成的条件、计数宽度、max_bytes及整记录预算；注册具体reader／decoder／owner。现有4／8／12B等具名deque来源不能外推成任意STL ABI支持 |

#### 9.10.2 Entity公共与各子类

本节已入源条目分别来自`entity.json`、`entity-classes.json`、`entity-effect.json`和`entity-text-dummy.json`；版本57包含其历次修订。原生UNKNOWN和更弱证据继续留在原账本，不因不发送而称已知。

| 范围与来源 | 特殊编码规则 | 版本／提交依赖 | 待落实项 |
|---|---|---|---|
| 公共Entity／Plus：[第3步](3-entity-common.md)、[公共职责](entity-common-synchronization-review.md)、[生命周期](entity-lifecycle-synchronization-review.md) | 公共E0—E9及空E7保持；仅保留真实展示／结构字段，不恢复valid、removedByFactory、玩法计时和颜色生产队列。Plus是具名伴随来源，不拼成根对象内存；optional区分未设置、false和0 | 当前成员use/shape及普通完整窗口；帧基准与该world的game_frame成对使用。Player公共块序列由其正式布局覆写，不能把其他Entity也改成body | 接入具体读取资格、引用／资源转换与共用解码；当前未知、同窗来源和跨域显示依赖不以默认值补齐 |
| Familiar：[参数／充能／绳索](familiar-synchronization-review.md)、[装备／Plus](player-equipment-plus-synchronization-review.md)、[内部关系](internal-entity-relations-review.md) | variant80只加最终i32充能amount（0..100），不传玩法Weapon；variant240保留真实DAC引用，绳索Sprite三投影＋有序XYZ节点四份外层u16 count=0/1一致，节点自身按内部有界count编码。IsaacsBody／蝗虫typed payload及分块Sprite按各自生成布局；蝗虫两组共享总数上限，不混成单组 | amount与附属Sprite同目标版本；共享payload存在性、各投影计数、节点顺序及真实引用同版本；variant臂按已登记RTTI／变体选择 | 具名最终值读取／typed owner与专用充能／绳索adapter；保留FireCooldown实际时钟等原未决，不用Frozen或0掩盖缺源 |
| Knife／Bomb／Tear：[三类职责复核](knife-bomb-tear-synchronization-review.md)、[装备／Plus](player-equipment-plus-synchronization-review.md) | 按现行生成块保留Knife拖影／飞行输入、Bomb五份costume、Tear三份辅助Sprite／真实效果与颜色；已撤销的伤害、攻击／轨迹生产输入及计时锚点不恢复。固定Sprite投影按各源声明，预留块号不前移 | 主／辅助资源、姿态、真实关系及对应world目标一致；Bomb实际资源就绪后由本地owner处理pending，不能以懒重建输入替代最终资源 | 所有实际线上标量／投影的编码和资源引用；来源、pending与draw恢复接线归10／13步，未知几何不伪装成已完成 |
| Pickup／Slot：[职责复核](pickup-slot-synchronization-review.md) | Pickup当前Price、DropDelay和独立价格／替代底座Sprite保留，DropDelay不猜成无条件递减；循环列表、Flip保存体和机器开奖输入不下行。Slot奖品Sprite的动画选择／playing独立保留，不由State／奖品ID补图 | Price与价格Sprite同窗同目标；实际引用、附属Sprite及资源同版本。Pickup逐用途随机alpha另见9.10.6，不用普通后态残值替代 | 编码实际字段与计时模式，落实具名时钟／资源来源和解码；不借TryFlip、Morph、SetPrizeCollectible或Update重新产生显示 |
| Laser／Homing：[采样职责复核](laser-homing-synchronization-review.md) | optimized／unoptimized最终Vector点列各有u16 count、各≤16384，允许两组数量不同；保留真实展示标量和有符号采样模式。indices、Bezier／系数及采样生产输入不发，相关块空预留 | 两组完整点列、Room Light所需unoptimized、Bounce真实引用及资源同目标就绪；空点列明确清旧值，不能与缺源混同 | 按L5／L6实际投影解码、逐组字节预算及完整候选提交；不以CalculateSamples或Interpolate重建几何 |
| Projectile：[职责复核](projectile-synchronization-review.md)、[版本45补证](projectile-effect-residual-evidence.md) | PR0保留Height4B；PR1空预留；PR2为原始height_render_mode:u8＋冻结Vector，共9B。该开关按零／非零消费，不能擅自限定成bool 0/1；轨迹／摆动生产输入不上线 | 正常Init／当前use资格；Height、冻结锚点与公共位置、资源及world展示时钟同窗；版本45来源纠正覆盖旧“只有Height／私有来源未核”说明 | 接入真实9B读取、原始u8校验及draw借用精确恢复；未解释范围仍保留UNKNOWN |
| Effect／ExtraData：[职责复核](effect-synchronization-review.md)、[版本40五项](effect-next-five-evidence.md)、[版本45剩余](projectile-effect-residual-evidence.md) | FX0=16B、FX1=13B，FX2按臂空预留；Timeout以u32模运算A=F+T可逆编码，不假设统一线性递减。Trail为u16 count＋64B节点，count≤127，节点块≤8130B。三个具体typed payload按声明存在性／子块消费；113等臂、167专用关系及已核内部flags由现行源分派，不恢复VarData／holder裸字节 | Timeout由同world帧头F还原，新world安装时更新；Parent依赖、payload存在性、资源及多投影同目标。缺失payload与存在但空节点不同 | 正式臂选择、条件／有界解码和typed owner；未核variant或未知payload不猜布局，136保存描述与Timer生产参数不作为状态补入 |
| NPC／Delirium：[NPC复核](npc-synchronization-review.md)、[受控UI纠错](npc-control-ui-evidence.md)、[Delirium补证](delirium-residual-evidence.md) | 三deque按逻辑顺序编码真实引用／Vector点列；OverlayEffect用144专用视图，Scale由主Sprite有效null-frame派生，副Sprite不随之裁剪。controlled_ui_mode只允许u8的0／2，拒绝旧1；Delirium Boss选择／AI值和companion留Host，17／18空预留 | deque、真实EntityRef、主／副Sprite与null-frame资源同目标；结构shape只取真实结构变化，不因Host Boss AI选择增加；受控UI不依赖原版Online记录 | 各投影／计数校验、0／2解码及窄UI／资源adapter；Delirium已核打包存储不上线，不能借此声称所有原生未知已解释 |
| Text／Dummy：[字节证据](text-dummy-evidence.md)、[正式实现](implementation-4-text-dummy.md) | Text按RTTI／variant2选145／146：RGBA16B、u16长度＋0..255原始字节，counter4B仅146；不强制UTF-8，不发送结尾0或duration。Dummy147按真实RTTI，无尾部字符串，不能以type964猜布局 | Text串读取资格绑定当前use／revision，空串、缩短和未终止区别处理；Dummy不可见时的历史屏幕cache依赖尚未闭合，不能无条件按当前坐标派生 | 真实字节前缀的编码／解码、当前资格及writer；Dummy具名历史显示来源／跨视口表达仍待13步，不在本次虚构新负载 |

#### 9.10.3 Player、参与者与控制组

本节以[`player.json`](../../../mod/binding/state/player.json)和[第5步正式实现](implementation-5-player.md)为准；P0—P2／P3—P5／P6—P8设计是语义来源，原“未生成／版本46”只代表历史时点。150—197已在版本47登记并延续至57，不另分配一套草稿编号。

| 范围与来源 | 特殊编码规则 | 版本／提交依赖 | 待落实项 |
|---|---|---|---|
| P0—P2：[前三块设计](5-player-p0-p2.md)、正式197 | P0物理10为64B＋headAnim有效原始字节；P1为1B CanFly；P2为77B直接健康／资源输入，不恢复Player六项玩法属性或通用查询字典。父E1／E4／E5／E6与P0属于body，P1—P8仍world | P0携带统一render_topology_revision，须与已安装P6相等；运动、主Sprite三投影与P0快源完整同窗，不能将P1／P2夹入body。HUD最终心格／StatHUD另属Presentation | body／world块分派、有界字节与资源解码；按9.9丢弃拓扑不符候选，实际快源、HUD消费者及写入仍待10—13步 |
| P3／P4：[中三块设计](5-player-p3-p5.md)、正式源 | P3格式下界88B，保留dense真实长度、4口袋／8材料／6poop固定槽、blocked及有效innate聚合；固定槽不加count，aggregate不按map节点复制。P4为4×24=96B；空Item的另外五值规范0且不读残留payload，通用12号ActiveItemDesc不改成此专用179 | 与同world真实Player／配置身份、资源epoch和有序库存语义对应；统计／名字／HUD显示依赖不能仅凭P3／P4原值假称已经重建 | 固定／变长／聚合字段的统一消费、空槽规则及具名owner；不调用通用库存getter或HUD Update补源 |
| P5动态Weapon：正式181—194及choice | u8低5位存在mask＋逐槽u16 view_layout＋该类固定负载，共1—91B；按生成允许集合和真实type精确消费，type10／11可共享动态类但保留type。五类恒零充能不附被裁字段，Modifiers按各型mask，Whip仅真实主引用；不套通用13号Weapon | 槽存在性、动态类、同resource_epoch的配置与真实引用同目标；报文不提供native_size／函数，空槽不读旧指针 | choice允许集、mask及类型交叉校验与本地装备owner；不得为显示Fire／Update或制造原生对象身份 |
| P6与Sprite目录：[后三块设计](5-player-p6-p8.md)、正式源 | P6物理16为111＋4C B，含有序目录与15×7B最终map；负index不读旧layer但保留body标志。19—21为13份固定ANM2投影，无count／重复slot头；22—24各自u16 count，为同一有序costume目录三投影 | P6目录、P0及costume三投影的计数／顺序受统一拓扑修订保护，跨身体借用不能只靠旧count或本地排序；资源闭包齐备再提交 | 完整目录／跨身体引用与三投影解码，拓扑及计数组接线；递归max_bytes与Player旧整记录32MiB理论保护、body组64KiB及普通消息实际保护／速率同时生效，前者不许可正常大帧发送，快源超限走既定ordinary body-joint，不截断 |
| P7、participant／control-group：正式195／196／197 | P7每身体21B，独立保存激活／列表资格、twin／backup／held及C/S组引用；不恢复native sub／replaced／703四槽。participant用空间9身份上下文；group空间10，21＋2N B有序active_anchors，N≤64；纯协议记录不分配原生游戏对象 | 四参与者与多身体分别计数；participant、group、身体成员／use及有序anchors同一结构事务，不用Player*、device key或原版Online身份续接 | 纯C/S结构记录封装、目标空间／引用和anchors校验；成员离开／复用、组变更与body候选退休接线 |
| P8条件效果／残影／当前记录：正式180及相关视图 | 效果通常6B，仅真实资源type1／3／4且id704时追加4B cooldown，不能套通用14号固定10B。残影／当前可绘记录／ANM2按声明投影；先读同块已登记选择器，可选序列D=0不读节点／Sprite，固定槽不加count | 条件读取依赖同resource_epoch解析后的真实配置身份；效果、残影、当前可绘序列及Sprite与对应Player／world修订一致，不能从原生非空指针推断有效 | 条件handler、真实资源谓词、12B残影deque与完整字节预算消费；未注册处理器不因元数据已生成就视为可接受／可安装 |

#### 9.10.4 Grid公共、17派生与外部水面来源

| 范围与来源 | 特殊编码规则 | 版本／提交依赖 | 待落实项 |
|---|---|---|---|
| 公共Grid／Plus：[公共复核](grid-common-synchronization-review.md)、[旁表补证](grid-plus-water-source-evidence.md)、`grid.json` | 通用Desc198四项16B，Grid199的G0为desc16＋index4＋最终water_clip_flags:u32=24B；原生collision与optional表示不发。201专用Sprite移出默认名／路径并保留实际Image、Dataminer最终选择；普通Sprite ColorMod仍完整，冠军色为专用4值 | Grid真实槽／use、Room当前几何及资源同world。旁表448×12B是本地来源ABI，六族按nullable覆盖、Lock按collision合成；源不可读不是未设置 | 229无发送块，不创建旁表网络记录；有界合成／解码与具体七处water消费接线。已有数字索引描述见[Grid描述实现](grid-description-implementation.md)，不等于网络解码或原生writer已完成 |
| 17派生与条件Sprite：[第6步](6-grid.md)、[八类证据](grid-eight-subclasses-evidence.md)、[七类证据](grid-seven-subclasses-evidence.md)、`grid-classes.json` | 按实际nested_layout选择201／202及Rock／Statue专用视图，不能固定套公共ANM2。Fire保留两附属Sprite／有序可绘粒子（每项16B，正常窗口≤5；state≥4投影空）；Rock只在可绘碎块分支编码资源／有序Vector，state2主姿态及layer0位置按已登记条件省略；Statue自身无Sprite负载 | 地板色派生用同world Room颜色，Wall／Teleporter等保留自己的颜色；Fire各投影与粒子、Rock分支／资源同目标，隐藏或覆写残留不读。真实空集合与Gap分开 | 条件视图、投影count／字节预算、有序元素及分支校验；资源与draw owner接线，Rock多pass overlay实际来源未完整接通的边界保留 |
| PressurePlate／Door：[专用证据](grid-pressure-door-evidence.md)、[全目录回映射](grid-member-map.md) | 两类Desc224仅12B，G0=20B；Pressure主202、Timer201，Door主226、extra201。Door主色只发1B alpha正值入口，其余取墙色；target类型／有符号i32描述索引／ExtraVisible及Light bit0x20按源保留，主色裁剪不套extra | Door目标属于同world描述关系，特殊负索引不无符号化；主／extra资源、Room墙色、HasWater及当前Boss匹配等Light上下文一致 | 专用描述／颜色／目标解码及真实Grid owner；119行目录已回映射不恢复旧原值查询、不新增保存／生成命令 |

#### 9.10.5 Room、Level、RoomDescriptor、RoomSave与Exploration

本节以同期版本57登记的[`room.json`](../../../mod/binding/state/room.json)／[`level.json`](../../../mod/binding/state/level.json)为布局依据，[第7步实现说明](implementation-7-room-level.md)、原设计账本和职责核对为状态／语义来源。Room239、RoomDescriptor270、LevelGamePrefix276及其实际结果／资源视图已入源；其中无发送块的Host／本地类型不因此创建网络记录。下列大小不含记录头、结构身份和批次，值布局入源不代表完整消息／运行接线完成。

| 范围与来源 | 特殊编码规则 | 版本／提交依赖 | 待落实项 |
|---|---|---|---|
| Room结构／时钟：239、[Room核对](room-synchronization-review.md)、[续核](room-residual-evidence.md) | R0当前几何36B、R1帧基准／Greed timer8B、R2直接显示104B、R3八门48B及其余结果块沿正式源。当前描述source-slot与成员引用分开；真实448格／8门、门成员号与门格序号不混用。帧基准配game_frame，地板／墙完整ColorMod、水量f32及熔岩保留；flow唯一归WaterVisual，不恢复path／占格表或生成历史 | Room成员／use与当前描述、Grid、门映射同world；Room initialized来源资格不等于线上一位即可授权构造；切房后旧引用／显示批次失效 | 第9步消费已入源块、固定槽、引用及旧32MiB理论内存保护与实际发送预算，落实已定公共结构封装与真实活动成员关联；具名来源／writer待10／13步，不以保存体或Init／Restore补齐结构 |
| Backdrop／FX／rails：240／242／244—249、281—284／291—296、[Room核对第4—5节](room-synchronization-review.md) | BackdropBinding为u16 count＋(u8 slot,u16 resource)，≤7项，slot0..6升序唯一且活动项资源非FFFF，extent16B。FX保留实际pass1..6、10B有序pose、选中非空ray组／10B RayVisual、poop存在／alpha；VeinGraphPayload294有10根固定槽及显式u8节点count≤10，非通用u16容器。两Sprite非覆写投影、rails真实extent内有效字节按源编码 | 图像、几何／pose、pass及资源同批；ray phase取已消费结果，重复展示不累加。vein索引／图须完整有效；世界可显示目录不能裁成Host视口前缀，同名图像不构成内容身份 | 第9步落实已入源计数／条件、索引／环检测、预算及资源绑定，具体图形消费归10／13步；程序图内容交9.10.6，未知pass／缺生产结果不编码为空，不恢复Backdrop RNG轨迹 |
| Water／Lighting／Wave／ForcedFocus：285—290、[显示资源设计第5节](room-display-resource-design.md#5-必要显示结果与本地变换) | WaterVisual287最大48B：present后按分支含mode／image／有符号phase／flow／最终32B shader结果；Lighting288按u8低4位mask附两KColor／alpha／特殊kind，最大46B；Wave289低2位mask按A/B附中心／radius_used／gain_used，1..33B；ForcedFocus290为present＋Vector，1/9B。flow等不重复编码 | 绑定world／presentation修订、资源epoch及pass；phase与816BB0的2×game_frame用途分开。mask／present表示已确认world分支，不把Host Options或未走绘制分支当禁用 | 消费已入源mode／mask／存在性与条件长度，补齐具名kind／资源／外层批次校验；正常结果缓存／纯值来源与本地视口adapter待10／13步，不猜GPU ABI或回放推进参数 |
| Level／Descriptor L0／L1：276／270、[Level核对](level-synchronization-review.md)、[Descriptor补证](room-descriptor-synchronization-review.md) | L0固定22B，effective_curses为最终u32，非Greed只将wave规范0，Boss匹配所有模式保留。L1每活动描述53B，独立nullable Data／OverrideData、u8门mask＋8×i32目标、u32 DisplayFlags、visited布尔、含HasWater的五位投影及Greed最终图标；不恢复全flags／seed／商店生成表 | 活动描述物理源槽／代次、RoomConfigVisual资源、同world Room／Grid及Run上下文对应；Grid／Safe／ListIndex与入口象限不混用，配置StageId不是Level.stage或Room Type | 按已入源字段落实位投影／负目标／特殊槽和资源交叉校验及结构封装；有效curse只取Host已核投影，不凭Run七位种子显示标志在Client重算 |
| RoomSave／Exploration、L2：270块1、297／298、[197行目录核对](room-save-exploration-synchronization-review.md) | 探索复用L0/L1，保存地图结果复用L2，不另建完整保存vector／I/F/B/U／ESSM ID／四揭图bool记录。L2缺席1B，存在为u8 present＋u16 mask＋按bit升序必要u8动态帧，3..12B；动态mask=2B9Bh，16类图标OR／max聚合，楼梯／Portal为位，不发保存数量／顺序 | L1知识、L2聚合、描述／地图修订与资源同world；可见未访问仍可有结果。DisplayFlags0的隐藏保存体不为完整观测额外扫描；Gap／预算不足不表示present0／mask0，回访对象走当前Entity/Grid | 消费已入源存在性／mask／条件帧及源预算，闭合目标描述／地图关联；MapBadgeList的值布局虽在level.json，显示职责仍由Presentation唯一承接（9.10.6）。完整保存体陈旧／回调阶段不能当提交见证，ESSM标记不作C/S身份 |
| L3拓扑与D1：276块1／2、299、[Level设计](level-synchronization-review.md)、[Random交接](room-random-handoff.md) | L3固定3×169个u16物理源槽／FFFF，共1014B，无额外count；源槽0..526仍须有效，Deathmatch槽525隔离，不用ListIndex代替。D1仅当前world必要的1B Error→ModelingClay谓词，不发完整效果ID／seed；最终颜色已承接时不重复 | L3配真实有序普通描述成员及member_epoch，不能由507个映射元素推断所有存储槽活动；D1与实际Image及Player效果／颜色上下文一致，不在每描述重复 | 固定表／引用与结构事务的交叉校验及失效／恢复表达，D1来源／颜色adapter待接线；MapFocus见9.10.6，不用L3裸下标补随机选择 |

#### 9.10.6 Run、Presentation、Random与资源／音频

Run的`run.json`布局230—238已在版本56入源；同期版本57又在room／level源登记配置、地图结果及资源固定值（263、300—309等）。Presentation根／HUD、D0、逐用途随机结果和音频已随版本58登记310—383等值布局；版本59补公共外层。下面旧待注册措辞保留原交接时点，现行仍待完整payload／允许集和运行接线，不能把小记录或固定头当完整可接受报文。按文件归档不改变Presentation对地图显示结果的唯一职责。

| 范围与来源 | 特殊编码规则 | 版本／提交依赖 | 待落实项 |
|---|---|---|---|
| Run／Seeds／分数：[Run核对](run-synchronization-review.md)、`run.json` | GameRun固定132B，复用内嵌子布局不再独立发第二份。TimeCounter是u32模2^32帧基准，GameFrame只在帧头；种子显示u8低7位仅8/9/10/29/30/78/79，bit7=0；ascent只取bit48布尔、daily只取id非零。ColorModState只发current六f32，两个黑暗值均保留，最终分数64B及最终进度图标另按源编码 | 帧基准用同world已应用game_frame还原i32；分数／资格结果必须对应真实显示批次，当前Game缓存与旧结算副本不能混用；Room／HUD等跨域上下文不重复归属 | 新转换／handler及解码分派；实际结算／标题／暂停显示交对应Presentation owner，不能调用Calculate、每日Init或排行榜请求补值。Ambush／ChallengeParam／种子树无负载类型不创建状态记录 |
| PlayerHUD／StatHUD／History：[Presentation第2—3、8节](presentation-synchronization-review.md) | Heart为1—9B草稿，visible0只有首字节，overlay_animation受eternal条件限定；ActiveVisual12B、TrinketVisual15B（含1B tint_override）、Inventory／HistoryVisual6B且保留顺序／负ID；StatValue12B，仅value／delta／countdown，物理行0..8按可绘资格。HolyMantle／Drop比例／低血闪隐等最终结果保留，P4六值不重复发 | 真实身体subject／use、动态HUD归组、Player／Run上下文、资源epoch及显示修订共同匹配；身体数不等于参与者数，两列StatHUD／两History owner分别承接 | 版本58已登记HUD根及真实固定槽／列表，版本59已登记外层；后续落实条件／资源／用途校验；最终缓存来源／安装待接线，不靠Player Update重建、不恢复previous／threshold或获得史 |
| HUD消息／Boss／共享UI／屏幕转场：[Presentation第2、5—6节](presentation-synchronization-review.md)、[资源／屏幕来源](presentation-resource-screen-sources.md) | 11消息物理槽各有存在性、streak／文字实际图像，fortune独立；Boss HUD保留真实负隐藏值／相位／分支；ItemOverlay、Prompt、暂停／死亡／RoomTransition／StageTransition／Nightmare及新识别BossOverlay按已核最终有序pass、资源和身体锚点表达。语义文本仅具名UI有界保留，字符串／文件名不替代程序图；StageTransition原生DWORD==5只作来源事实，线上只发最终遮罩而非DWORD或bool | 与同world显示修订、前后场景／结算owner和实际内容资源对应；本地菜单提交、视口／Options及原版Online面板不复制。前帧反馈需精确内容或完整recipe；versus与Nightmare按实际layer/slot主叠加记录，不能只取最后Sprite | 正式根／存在性／pass／序列和范围／上限尚待定稿。条件内容及BossOverlay来源已核，模式4为原版Deathmatch而排除，Pause只排除已核密码1B；其余后缀／对象内部和Mod shader STL ABI仍未知，不把来源已核当作报文已注册 |
| D0展示时钟：[Random交接第3节](room-random-handoff.md)、[第8步8.2.1](8-run-presentation.md#821-绘制输入块-d0与逐用途随机结果) | 当前D0仅u32 manager_frame＋u8 paused=5B；subframe按paused及Manager相位派生。旧37B／13B与两seed／shift／guard不再下行，GameFrame仍由world帧头唯一承载 | 必须是正常Host该显示批次实际采用的Manager相位／暂停资格，不能把普通模拟后态和另次Render结果拼接；ManagerFrame不与GameFrame假定固定差 | 注册D0及其外层显示批次／状态关联、精确bool校验，接入帧借用和恢复；不恢复全局随机游标重放 |
| 逐用途随机结果及Random边界：[Random交接第3节](room-random-handoff.md)、[Random职责](random-synchronization-review.md) | Pickup alt pedestal／Familiar颜色栈各取正常已产出alpha；标量为u8 state（0确认无覆盖，1随后f32），1/5B。保留实际f32舍入，不靠数学开区间误拒端点。玩法RNG／池／CRT／MT／Lua与音频选样本流不建网络记录；药丸揭示／实际名称与Loot有序预览由具名显示结果承接 | 结果需subject/use/shape、world state revision、presentation revision、member/resource epoch及pass／调用／递归身份；同GameFrame或本地遍历顺序不够。未产生、旧残值不能当state0或本次alpha | 值布局357—361及公共外层已登记；用途允许集、整目标实际预算与缺口处理待接线；正常生产点缓存、颜色栈／CPU队列及显示adapter待接线，不额外Render／抽取 |
| 地图Presentation：level.json的300—303、[显示资源设计第6节](room-display-resource-design.md#6-地图完整事实独立查看)、[两域核对](room-save-exploration-synchronization-review.md) | MapFocusResult300为present＋i32锚点＋u16描述成员，1/7B；MapFocusPair301分small／large两配置结果。MapBadgeList302显式count_type=u8、count0..7＋i8[count]，恰好1+count字节，非负ID仅0/1/2/4/5、负ID−1..−9，顺序保留。303承接2B nullable question_image；边框／icons／itemIcons资源与最终震动另由对应显示owner承接 | 与完整可显示L0—L3、challenge32真实锚点／描述、地图／成员／资源及显示修订一致；两配置焦点未证明相同不可共用，169不授权访问第169格。查看／长按／展开／视口／Options各端本地，徽标不是L2或Sprite最后帧 | 按已入源u8计数、枚举／条件长度及真实成员关系解码，补齐地图显示根／世界限制和外层修订；同world来源、独立地图adapter及资源就绪仍待接线。Gap不变空徽标或present0，不发Host窗口前缀／裁剪FBO |
| 配置资源：room.json的263、[显示资源设计第3节](room-display-resource-design.md#3-配置资源发送显示投影不传生成配方) | RoomConfigVisual固定15B：i32 stage_id/type/subtype、i8 width/height、u8 shape(1..12)。原生shape先核再窄化；Data／OverrideData独立nullable u16资源引用，不发送Spawn／Merged图／权重，不以stage/type/variant拼唯一身份 | 不可变资源号关联真实owner/use与内容版本；同15B不证明同对象连续性，原位内容变化分配新号。地图Home隐藏条件为Data.StageId35且Type16；Door Light的StageId依赖按其已核消费者，不误写成Type35 | kind3完整表项分派消费已入源15B投影／尺寸／shape规则，引用随同world提交；本地只显示承载／owner待13步，不借配置加载、Merge或随机选房生成状态 |
| 图像／ANM2／shader／surface：room.json的304—309、[动画资源复核](animation-resource-synchronization-review.md)、[typed资源设计第4节](room-display-resource-design.md#4-图像内容资源表和获取时点)、9.6 | 原版本57资源项7B头已由59的8B显式table头替代；旧kind0/1/2路径草稿不是完整payload，kind3配置15B，kind4固定SurfaceRecipe（16B头，Clear21B／Quad196B），kind5为16B像素描述。SourceQuad区分pixel／UV及padding；format0为精确RGBA8，不无声量化特殊格式。blob头现15B含table，offset／bytes显式u32，非通用u16容器 | 一个run资源号只增不改；依赖无环，完整内容和本地图像安装后才ready。路径、实际Image及default／layer普通／champion shader选择独立，不把地址、GPU handle或名字当内容身份 | 已入源固定头／操作值之外，完整kind分派、recipe／shader允许集与blob消息仍待定稿；乘加／chunk冲突／闭包预算需落实：路径≤4096B、chunk≤256KiB、像素≤16MiB且每边≤2048、操作≤4096、深度≤16、待安装内容≤32MiB。图形ABI／缓存／物化接线待后续 |
| Music／SFX／环境声：[当前同步职责](presentation-source-synchronization-review.md)、[音频具体来源](presentation-audio-sources.md) | world Start／明确retrigger／Stop及有范围退休事件；逻辑voice当前实际资源、最终Options前gain／pitch／必要pan／loop／world pause。crossfade保留所有有逻辑用途的并存声部，不发固定14空槽；mask／mix_mode／mul／权重／组current／ducking／全局pitch不与最终参数重复。FrameDelay不上线，来源clamp规则仍区分 | 资源／逻辑voice身份／代次、事件序号／水位与world关联；baseline不重播历史one-shot，未开始的实例不因控制补播。jingle使用本地SFX音量类别；个人设备失败／抢占不广播Stop，后端重试不自动Start。硬件游标／PCM队列非必需world状态；必要逻辑段／起点与可靠续接仍待来源 | 值布局381—383及版本59的40B事件外层／Clear范围已定；完整资源payload、去重／恢复运行实现仍待9／11／12步；全部音乐逻辑gain writer、world用途／退休、后端结果污染隔离、视频音轨与本地adapter待补证和10／13步。已核本地提交不等于world资格闭合，未知不补零 |

#### 9.10.7 跨域版本依赖与本步剩余事项

- **公共上下文与专用依赖分开登记**：版本59的50B公共头／52B分段头已表达scope、来源／显示批次和Peer目标；414—417表达具名结果依赖、贡献与事件。P0/P6的u32拓扑仍取原字段，不重复发送；用途允许集和完整payload尚待本步后续，不能以通用封装替代来源闭合。
- **资源空间与kind分开登记**：字段的`wire.target`必须对应版本源声明的`anm2-animation-table`、`image-table`、`anm2-files`、`item-config`、`entity-configs`、`room-config-visual`、`shader-table`或`audio-table`；同为u16不表示可以跨表替换。9.6的table编号与kind分开；公共空间／epoch语义已在正式合同定稿，完整payload分派仍须在本步后续登记；Player条件效果的配置身份和shader的default／layer覆盖不能靠路径或整数相等猜测。
- **结构先行与内容就绪**：成员关系、配置／资源闭包和目标状态必须能对应同一合法事务；world的Room几何、描述拓扑、L2、地图焦点及必要显示批次共同提交。body仅遵循成员／资源与P0—P6的窄依赖，不因完整地图／显示资源等待而让Authority等待全员输入或ACK。
- **确认含义不扩大**：解码成功、表epoch已安装、blob字节已收齐与原生应用成功是不同状态。必要内容／结果不全时不回复该目标AppliedAck，也不将Gap、Wait或失败替换为Null／零mask／空集合；合法已确认空值仍按各布局消费。旧批次只在其原world／use仍有效时可继续显示。
- **失效与恢复必须有编码归属**：切房／换层、Maze交换／红房／维度、资源或双配置原位变化、Hourglass／Ascent／合法Continue及成员use/shape退休由对应结构／状态／资源事务承接；探索不要求单调增长。旧seed、ordinal、地址或ESSM ID不能续接新身份，不能用Save／Restore／Update／随机Refresh补Client状态。
- **预算覆盖全部通道**：普通消息64KiB保护、旧可靠大消息32MiB解码保护与实际速率／突发预算分别检查，均不能充当常态发送额度；Context表、recipe、压缩blob、结构和Baseline均计入首次准备≤1MiB／≤10秒及三端总持续7.5Mbps目标，不能换通道漏计或截断／降质后宣称满足。具体组帧调度与实际效果待11／15步，本文不产生性能结论。
- **当前仍待落实**：把表中待注册投影纳入唯一源并在实际变更时递增整数布局版本；在已生成公共固定头／外层语义上完成typed payload与用途完整允许集、严格接受规则及运行编解码；随后按10—14步接通运行链。第9步当前不因清单整理而标记实现完成，文末历史记录不作为兼容或执行依据。

## 改动范围

- **新增**：状态帧编码器与解码器，成员表、资源表的编码器与解码器。
- **修改**：`stream_codec` 新增消息种类，路由头不变。
- **退役**：`Range` 的树形正文、`EncodePayload`、`DecodePayload` 等，在第 14 步移除。

## 失败方式

- **记录越界**：`record_bytes` 与块长度不一致时没有拒绝，导致读越界或把后续记录错位解释。
- **序号过早复用**：成员序号在旧帧还会被引用时就被复用，Client 把旧帧写到新成员上。
- **资源序号复用**：同一序号被重新分配给另一路径。
- **body 帧早于成员表**：成员表还没安装就接受了 body 帧，造成身体错配。
- **版本回绕**：u32版本／epoch不允许静默回绕；将耗尽时退休旧复制代次并完整准备，普通Resync不能在同路由重置计数。
- **压缩前后长度不符**：zstd 解压后的长度与声明不一致。沿用现有的解压长度检查。

## 完成内容

- **2026-11-11各域正文完成**：版本60新增BodyValueSource／有界编码解码、同源数字条件及校验计划、共享／计数组事实和根字段视图；337布局含空值类型统一处理，12B头与u32掩码保持。详见[正文实现](9-domain-body-codec.md)。
- **2026-11-11第一项完成**：保留u32掩码和12B记录头，公共头／分段／表／结果组／贡献／事件／ACK／Resync结构及预算定稿；版本59新增408—421并更新304／306，固定头／常量及单行表同源生成。实施结果与边界见[正式合同](9-public-wire-contract.md)。
- **后续交付**：完整typed资源payload与table/kind矩阵、各用途pass／shader／blend／sampler允许集、详细可执行接受规则和运行编解码。后续接线按10—14步，第15步唯一统一整体验收。
- **旧工作事实**：2026-11-10的9.10整理只维护交接，未改当时版本57或运行实现；其后版本58完成第8步结果登记。本次不把新合同回写成当时已实现。

## 未决问题

- body按Peer确认、GameFrame u32、公共上下文／结果组／贡献／事件身份与Clear范围已定，不再列为未决。
- typed资源完整payload、table/kind合法组合、用途／pass及图形允许集和实际编码调度仍待本步后续完成；已有来源Gap及UNKNOWN不变。
- 正常目标的真实大小、有效峰值与积压时限还需按布局静态核算；仅内存保护容得下不代表满足10Mbps／频率／延迟目标。

## 历史修订提示

以下集中保留本页原有修订提示，按各自时点阅读；其中“当前版本”“未生成”“待注册”等不覆盖本文输入事实与9.10的现行状态。版本40／47的已入源修订已延续至57；版本46时的Player草稿已由正式源取代。版本56当时未注册的Room／Level／地图结果及资源固定值已部分在57入源，具体范围见9.10；其余待注册投影、完整消息和运行接线仍未完成。

**版本40 Effect布局引用**：见[后续五项处理](effect-next-five-evidence.md)。记录与帧头格式不变，整数布局版本递增为40；新增137—141及102移出113由生成布局表分派。所有Effect FX0=16B、FX1=13B，FX2空预留，Trail节点块为u16 count＋count×64B、count≤127（最大8130B）。不将原生VarData／holder地址塞入扩展块，不复用旧8／21B或16384上限。其他容器的独立上限不受影响，真实记录／帧字节预算保持；未运行测试。

**Player P3—P5后续布局（2026-11-09，未生成）**：按[定稿](5-player-p3-p5.md)P3为70B前缀＋有界容器（格式下界88B），P4固定96B，P5为u8低5位存在mask＋按槽序的`u16 view_layout＋固定分型值`，总1—91B；每个view按注册布局和真实type精确消费，不能由报文提供偏移／native大小。二级布局标记仍用整数号，实际编号待生成，不改这里12B记录头／32位块mask。Player物理13／14／15为world，body不得夹带。当前版本46不能提前接受这些草稿负载。

**Player P6—P8与C/S结构（2026-11-09，未生成）**：[完整草稿](5-player-p6-p8.md)规定world物理16／17／18分别为P6的`111+4C` B、每身体21B P7、P8的disabled／6或10B效果／残影／当前记录及ANM2分支。效果仅资源身份为type1／3／4且id704时附4B cooldown；其余按6B消费，不能套通用14号旧10B。P7不带sub／replaced／703四槽，group引用纯C/S空间10、participant空间9，group另有`21+2N` B有序active_anchors记录；真实layout号待注册生成，不创建原生对象。P6／P0采用统一Player绘制拓扑revision保护跨身体借用，不改变本节12B记录头／32位mask；当前46不接受这些草稿。

**版本47布局来源（2026-11-09）**：[第5步正式源／生成](implementation-5-player.md)已完成，EntityPlayer=197，participant／control-group=195／196，Player主动槽=179，条件效果=180，动态Weapon=181—194；物理0—24及条件／固定槽／预算元数据以唯一源和生成表为准。版本46／草稿／待生成的旧表述保留原时点；本步骤的运行接线仍未实施，不能因布局已生成就提前接收、采集、安装或宣称验收通过。

**2026-11-10 第7步显示／资源编码交接**：[typed资源与固定投影](room-display-resource-design.md)补齐15B RoomConfigVisual、资源项7B头、SurfaceRecipe／像素描述及14B ResourceBlobChunk头；9.6现按此修订。新结果含WaterVisual最大48B、Lighting最大46B、Wave1..33B、ForcedFocus1/9B、MapFocus1/7B，实际外层身份／批次另计，不重复原L0—L3／MapBadgeList。都尚未注册，版本56不提前接收。Context也计首次准备／带宽预算；只比较整数版本，不计算哈希，无测试。

**2026-11-10 RoomSave／Exploration编码交接（待注册）**：按[目录核对](room-save-exploration-synchronization-review.md)，探索事实复用L0/L1，保存地图结果复用L2；不新增完整保存vector、I/F/B/U字段、ESSM ID、揭图生产bool或Host地图按键历史。Presentation的MapBadgeList草稿为`u8 count(0..7) + i8[count]`，恰好1+count字节，顺序保留，非负ID只接受0/1/2/4/5、负ID−1..−9。这是一项具名紧凑视图，正式布局须显式登记其u8计数规则，不冒充9.1通用u16容器；绑定同world地图／资源修订，Gap不编码为空列，未注册前版本56不接受。当前只设计，无测试。

**2026-11-10 第7步版本57交接**：[实现说明](implementation-7-room-level.md)已登记Room／Level状态和ResourceEntryHeader304、Pixels305、BlobHeader306、Recipe307—309的固定typed值；资源空间新增room-config-visual／shader-table。完整payload分派／依赖闭包、blob与有序recipe编码、具体GPU枚举及运行接线仍归本步及后续，不能把固定头登记写成完整报文已实现。版本56当时未注册的事实保留原时点。


**2026-11-11被替代草稿说明**：原26B帧头、38B成员项、7B资源头／14B blob头和body／GameFrame未决由[正式合同第11节](9-public-wire-contract.md#11-被本合同替代的草稿)说明；12B记录头与u32掩码未变。旧版本的生成／失败／UNKNOWN记录保持原事实。
