# 7：Room、Level、RoomSave 与 Exploration

**2026-11-12 显示委托修订**：

- Room／Level 保留房间描述、探索状态和需要的房间状态。
- 光照、水面、显示资源投影和地图绘制结果，改由 Client 原生 `Room::Init`、Render 和 Minimap 本地产生。
- Host 随机选出的装饰结果仍然同步。
- `Room+0x11F0` 按 Host 原值安装。
- 见[修改方案](client-display-delegation.md)第 5、7 节。

本次只修订文档，未改代码，未运行测试。

**2026-11-10 第8步地图交接已登记，版本58**：[Minimap325与MapDisplaySprite355](implementation-8-presentation.md#地图唯一owner)复用300—303，承接两边框／icons／itemIcons、最终地图震动与world转场alpha；L0—L3与Room字段保持唯一owner。正常来源与确定性地图adapter仍待10／13步；下方旧待布局表述保留原时点。

**2026-11-10 Room／Level影子状态描述生成层完成**：[接口与实现](room-level-description-implementation.md)。正式`ClientDescribeRoomLevel`提供Room／Level／Descriptor／双配置的类型化纯值与完整显示投影，检查同world根、L3／入口象限／有序成员、L2和八门关系；52个数字角色与图标帧界由布局源生成，旧输入明确改名Legacy。版本仍57、UNKNOWN保持；描述库及产品Release／Win32纯编译／链接通过，没有运行测试。实际解码调度与原生writer仍归12／13步。

**2026-11-10 生产布局源与生成完成，版本57**：[实现说明](implementation-7-room-level.md)和[生成索引](layouts/README.md)。`state/room.json`／`state/level.json`正式登记71个布局239—309、四域597行目录、实际显示／资源固定值和生成支持；Room13个world块、L0／L1／L2／L3／D1及15B配置投影已生成。Room直属28B／静态内嵌递归613B、Level22B、保存体与外部UNKNOWN保持；正式来源缓存、完整资源协议、GPU ABI、reader／writer及显示adapter仍按后续步骤实施。状态库Release／Win32纯编译通过，没有运行测试。以下早先“未注册／未生成／版本56”条目保留原时点；当前生产合同以版本57唯一源为准。

**2026-11-10 必要显示与资源设计补齐**：[设计／来源／边界](room-display-resource-design.md)及[机器表](room-display-resource-design.json)明确15B RoomConfigVisual、不可变typed资源／完整surface recipe或精确内容、水光波纹／强制焦点与纯地图adapter。纠正Home读StageId35而非Type35；地图覆盖全部可显示事实，查看／视口留本地。设计补齐不等于生产布局／来源缓存／安装实现，版本仍56，无构建或测试。

另补核独立Lost问号图像Minimap+614..61B，需作为实际资源承接，不在icons／itemIcons Sprite内；Presentation账本修订2，直属UNKNOWN23→15B，原8B未知记录保留，其余未知不变。

**2026-11-10 RoomSave／Exploration目录核对完成**：[同步职责／官方API差异／J273证据](room-save-exploration-synchronization-review.md)、[RoomSave 98行](room-save-member-map.md)及[Exploration 99行](exploration-member-map.md)均已逐行登记。完整保存体／恢复、ESSM内部ID与揭图生产标志留Host；地图知识复用L0/L1，保存图标／楼梯／Portal复用L2，实际回访对象由Entity/Grid承接。本地展开／长按不发；最终震动、图标／边框资源和最多7项有序道具／诅咒徽标仍必需，MapBadgeList草稿1—8B交Presentation。补记GetByType返回表、Pickup U1布尔、Grid Clear仅清Type且不改长度、ESSM raw字段与Getter不等价。通用保存体74B等UNKNOWN保持，正式布局／来源／adapter待接线，生产版本56，无构建或测试。

**2026-11-10 RoomDescriptor专门核对完成**：[职责与J273证据](room-descriptor-synchronization-review.md)、[6类型／输入视图账本](room-descriptor-byte-ledger.md)及[44行回映射](room-descriptor-member-map.md)。B8h直属原24B UNKNOWN核为八项i16商店生成类别＋八项u8补货次数，均留Host；直属未归属字节24→0，私有语义／高flags位及外部未知不因此闭合。L1仍53B，增加HasWater第五显示位；L0的Boss匹配所有模式保留，纠正上轮非Greed清零。D1当前Error颜色谓词1B；L2仅最终图标mask及必要帧，存在3..12B，楼梯/Portal为位，完整保存体／seed不上线。已回写Level设计修订2；资源表、正式来源与draw adapter未实施，版本56，无构建或测试。下方原24B／四位flags记录保留其历史时点，现行以本核对为准。

**2026-11-10 Level静态核对完成（未接生产布局）**：[同步职责与J273证据](level-synchronization-review.md)、[7类型／视图字节账本](level-byte-ledger.md)及[131行回映射](level-member-map.md)。Level按Game前缀`[0,18910)`登记，SDK的`sizeof(Level)=1890C`单列；真实flags／Greed无红心伤害连续波次是1839C／18374，官方简化Level的1822C／18204实际落在三维房间映射内，错误Getter事实保留。身份／有效诅咒／拓扑、双配置及实际地图／HUD／Door显示投影保留；生成RNG／seed、过房返回、全flags、Myosotis与保存历史留Host。保存实体地图图标和保存Grid楼梯结果仍必需，不能随完整保存体一起丢弃。根22B／共享描述24B UNKNOWN保持；地图最终批次／配置资源表及正式接线未完成，生产版本仍56，无构建或测试。下方旧草稿与本核对冲突时以本核对为准。

**2026-11-10 Room静态续核（修订2，仍仅Room）**：[剩余字节补证](room-residual-evidence.md)、[职责](room-synchronization-review.md)、[32类型／视图账本](room-byte-ledger.md)及[269行回映射](room-member-map.md)。Room的0—7897h现91个直接范围，直属UNKNOWN1290→28B；EntityList884→3B、FXLayers39→3B、FX31→5B、RayGroup4→2B、Vein8→7B，FXPoop5B及其他嵌套未知另计。BackdropConfig纠正过宽尾部后未知4→5B／实例。旧26视图／85范围为历史基线，保留在JSON修订记录。新增必要显示方向reversewatergfx；环境声音需实际voice结果。两张448项path／占格表、死亡名单／历史、雨区域、输出函数表及原版Online环均不作为C/S负载。正式room布局／reader／writer未实施，其他第7步域由各自核对承接，生产版本56，无构建或测试。

**2026-11-10 Random字段交接**：[静态核对](random-synchronization-review.md)核定SDK漏写的Game／Level前缀`+182E4..182F3`为恶魔／天使房生成RNG：原绑定`853542/878DB8→74D6F0`返回this+182E4，`743970`消费四字；全16B归Host，不与+182D4的generation流合并，也不再猜为填充。Backdrop+4F28流与+4F38 seed同样留Host，实际背景资源／纹理结果仍须本步提供。本轮不改第7步生产布局或运行代码。

**版本53 Grid显示依赖（待本步实际接入）**：[PressurePlate／Door证据](grid-pressure-door-evidence.md)要求同world的Room.greedWaveTimer(+723C)、floorColor(+72A4)、wallColor(+72D0)、真实八门槽关系，以及Door target_room_index到Client自有目标descriptor/特殊目标的映射（Light读Flags&40000）。索引保留有符号负特殊值；本轮纠正旧“RoomConfig.type35”为Data.StageId==35（Home），不能用Door.current_room_type替代。未知目标不能当空或本地进度默认值。主/extra显示与结构关系分开提交，不用primary Slot/Direction/target索引触发Client过门FSM；本文旧设计尚不表示这些owner已实现。

状态：第7步生产布局源／生成、四域597行正式回映射及Room／Level影子描述生成层已完成；UNKNOWN及来源缓存／GPU ABI／完整协议／实际解码调度／原生安装缺口保留。上级：[总计划](README.md)。完成范围见[版本57源与生成说明](implementation-7-room-level.md)和[影子描述实现](room-level-description-implementation.md)；不表示整个第7步运行功能或全部字节语义已知。

## 目标

完成 `Room`（0x7898）、`Level`（SDK大小0x1890C；本轮核到的`Game`楼层前缀为0x18910）和 `RoomDescriptor`（0xB8）的字节归属。确定必要房间／楼层／地图状态，区分Host保存历史与真正需要同步的保存图标结果，并覆盖 Exploration 域（访问、显示标志）。

## 输入事实

### Room（SDK 声明 0x7898 字节，37 个具名字段）

| 位置 | 字段 |
| --- | --- |
| 0x04 | `_descriptor` |
| 0x08 | `_roomType` |
| 0x0C／0x10 | 宽、高 |
| 0x24 | 448 个格子指针 |
| 0x724 | 8 个门指针 |
| 0x744 | 8 个门格子序号 |
| 0x76C／0xE6C | SDK `_gridPaths[456]`错误；原生448个DWORD path，后接448个WORD NPC占格计时 |
| 0x11EC | `_roomClearDelay` |
| 0x11F8 | Camera 指针 |
| 0x1204 | `_renderScrollOffset` |
| 0x120C | `_redHeartDamage` |
| 0x1218 | EntityList |
| 0x1618 | FXLayers |
| 0x1BB0 | `_bossId` |
| 0x1BC8 | Backdrop（0x5278 字节） |
| 0x7070 | 商店补货 deque |
| 0x7220—0x7298 | +7220单byte波纹相位条件，真正减速计时在+722C；另有Greed timer、水量与四个KColor |
| 0x72A1 | `_pacifist` |
| 0x72A4／0x72D0 | 地面色、墙面色（ColorMod） |
| 0x7310 | TemporaryEffects |
| 0x7454—0x7460 | 雨、水流 |
| 0x7468 | RailManager |
| 0x7740 | `_lavaIntensity` |
| 0x7744 | 8 个红门轮廓 Effect 指针 |

### Level（Game 前缀）

| 位置 | 字段 |
| --- | --- |
| 0x0／0x4 | stage、stageType |
| 0x8／0x9 | `_bossChallenge`、`_devilRoomDisabled` |
| 0xC | curses |
| 0x14 | `_gridRooms[507]`，每项 RoomDescriptor 0xB8 字节，共约 91 KB |
| 0x16C7C | `_negativeGridRooms[20]` |
| 0x17ADC | `_roomOffset[507]` |
| 0x182CC 起 | 房间数、起始房间 |
| 0x182D4 | `_generationRNG` |
| 0x18300 | Room 指针 |
| 0x18304—0x18314 | 当前／上次房间与维度 |
| 0x18318／0x1831C | 离开门、进入门 |
| 0x1839C | 楼层状态标志 |
| 0x183A4 | Myosotis 拾取保存 |

### RoomDescriptor（SDK 声明 0xB8 字节，30 个字段）

| 位置 | 字段 |
| --- | --- |
| 0x00—0x0C | 格子序号、安全格子序号、列表序号、维度 |
| 0x10／0x14 | `Data`／`OverrideData`：RoomConfig_Room 指针 |
| 0x18—0x3C | 允许的门、8 个门、显示标志 |
| 0x40—0x66 | 访问与清理计数，各类种子 |
| 0x68 | `SavedGridEntities`（vector<GridEntityDesc>） |
| 0x74 | `SavedEntities`（vector<EntitySaveState>） |
| 0x80 | `RestrictedGridIndexes`（std::set<int>） |
| 0x88 | `SavedEffects` |
| 0x94／0xA4 | 本轮补核：八项i16商店生成类别／八项u8补货次数，均留Host；SDK未具名 |
| 0xAC—0xB4 | 商店序号、Delirium 距离、分组 |

### 公开成员与现有实现

- **公开成员（`stream-catalog.json`）**：room 269 个、level 131 个、room_save 98 个、exploration 99 个。
- **应用目录（`room-grid-application.json`）**：Room 171 个、Level 87 个、RoomDescriptor 44 个，另有 RoomConfig*、Backdrop、Camera、FXParams、ColorModifier、RoomTransition 等。其中，Sprite、Color、LayerState 等表现类型是挂在 Room／Grid 下的嵌套成员。
- **现有 Client 原生写入**：`client_native_world.h`、`client_grids.h` 已覆盖房间构造、格子事务和门绑定，语义保留。

## 设计

### 7.1 字节归属要点

- **配置指针**：`RoomDescriptor.Data`／`OverrideData`各为独立可空会话资源引用，15B RoomConfigVisual保留StageId／Type／Subtype／Width／Height／Shape。不是stage/type/variant/subtype组合唯一键，也不要求Client重建完整STB／Spawn／Merged生成图；详见[资源设计](room-display-resource-design.md#3-配置资源发送显示投影不传生成配方)。
- **保存集合（Level已核定）**：完整保存体留Host；`SavedGridEntities`的地图楼梯结果、`SavedEntities`的地图图标结果必须由L2承接，不能随保存历史一起忽略。`SavedEffects`由当前实际Effect结果承接，`RestrictedGridIndexes`生成限制不下发，本轮不要求为同步遍历树。保存完整类型及观测读取另按RoomSave范围核对。
- **种子**：不得因为Client要构造Room就恢复随机生成。Room内Backdrop RNG及construction seed留Host，发送实际资源／纹理结果；Level与RoomDescriptor的生成／放置／奖励seed及两份RNG同样留Host，challenge32地图取Host已选最终标记。Grid Light已核SpawnSeed例外由第6步承接，不能机械外推所有seed均不发。
- **房间中的指针**：
  - Room 中的格子、门指针归“引用”，转成 Grid 成员序号，由第 6 步的记录承载。
  - Camera、EntityList、FXLayers、RailManager内部视图已在[Room账本](room-byte-ledger.md)展开。Camera历史／视口本地，必要强制focus意图与Room当前scroll分开；EL地址／缓存不复制；FX只发实际采用结果，rails保留真实extent字节。
  - `_redDoorOutlines` 是Host维护的EntityPtr通知关系；实际轮廓Effect按独立Entity成员承接，不由该数组创建或级联删除。
- **寻路数据**：`+76C`的448个DWORD path及`+E6C`的448个WORD NPC占格／择位计时均归Host，SDK456项声明跨过第一表32B。Wiki的动态写入／衰减再由J273双指针2／4字节步长补证；不声称可由布局无损重建。Client不寻路／碰撞模拟，不为原Getter相等发送；Grid最终外观与WaterClip由第6步承接。
- **房间帧基准**（帧锚点，总计划第 4 节）：
  - `Room+0x11F0` 是 `Room:GetFrameCount()` 的基准帧：GetFrameCount = 全局帧 − 基准（`client_room_native.cpp:132-135`）。它归状态，原值发送，放入 R1。
  - Client 用展示时钟还原 GetFrameCount（第 13 步 13.6）。现有每次展示前重新锚定的做法，改用原值后不再需要。
- **Backdrop 与 Room 尾部**：已按构造／Update／实际draw登记，含SDK漏列的地狱横向背景。程序纹理发实际内容资源或已证纯显示recipe，不发背景RNG轨迹；不能调用含Spawn的Init补图。实际配置+139反向水面方向需发1B或等价最终有符号phase。Room+7220只核1B相位条件，真正slowdown为+722C；官方Hush补丁过宽读+7220的事实保留，邻接3B仍未知。环境gain缓存／延迟声音计时留Host，实际voice由第8步承接。

### 7.2 状态块（初步划分，本步定稿）

| 块 | 序列 | 内容 |
| --- | --- | --- |
| R0 房间身份 | world | 描述身份／维度、实际类型、宽高、bounds；配置owner归descriptor资源核对；小记录草稿36B |
| R1 房间动态 | world | 房间帧基准 `+0x11F0`原值与Greed timer，共8B草稿；清房、受伤、减速／雨生成、pacifist等生产值留Host |
| R2 房间表现 | world | 地板／墙色、水量／水流／熔岩、scroll；实际Backdrop资源／surface选择及特殊背景；最终water32B、lighting／混色／波纹与pass资格；Game current ColorModifier归第8步 |
| R3 门映射 | world | 8 个门槽的 Grid 成员序号与门格子序号 |
| R4 房间效果 | world | 实际采用FX层／选中ray组／poop／vein和真实轨道extent；Room效果列表及红门通知关系留Host，当前可见结果由具名投影／Entity承接 |
| L0 楼层（静态核定，待生成） | world | 22B：stage／stageType、有效curses、入口象限／维度、Greed wave及当前lastBoss匹配；只有wave非Greed规范0，Boss匹配供普通TrapDoor Light，所有模式保留；不发选择器／全flags／返回FSM |
| L1 描述显示（静态核定，待生成） | world | 每活动描述53B；Grid／Safe／dimension、双配置、门mask／八目标、DisplayFlags、visited、五位显示flags（含HasWater）与Greed图标；源槽由结构消息承接，-19隔离 |
| L2 最终保存图标（静态设计，来源待接入） | world | 缺席1B，存在3＋popcount(mask&2B9Bh)=3..12B；16类图标mask及必要帧，楼梯/Portal为位；原ID／保存数量／顺序／条件值不发；challenge32最终位置另取Host |
| L3 楼层占用（静态核定，待生成） | world结构 | 3×169个u16物理源槽／FFFF，共1014B；当前维表指针由Client本地建立，有序普通描述成员另由结构身份承接 |
| D1 当前描述显示上下文（静态设计，待生成） | world | 必要Error→ModelingClay颜色条件u8 bool共1B，当前world一份、随值变化更新；原SpawnSeed及完整效果ID留Host，>0恒真；实际图像与其它颜色上下文仍归Presentation／Player |

描述表按活动源槽的正式投影分块比较，不memcmp含指针／容器／UNKNOWN的约97KB原生定长体。保存容器不下发，但地图最终结果必须具备真实来源；不能仅以访问／清理计数改变作为重读条件，原位保存图标、Flags或占用映射都可能变化。读取／dirty覆盖与兜底在第10步接入，沿用`low_frequency_policy`的有界规则，不靠通知完整性假设。

### 7.3 Exploration

当前显示标志、visited布尔与clear显示位由L1承载；原始访问／清理累计历史留Host。Exploration复用描述投影与必要地图结果，不再重复采集一份；[99行目录](exploration-member-map.md)已完成，含Level87及Minimap12。map／blue-map／full-map／compass原四bool不另发；实际道具／诅咒徽标用最多7项有序MapBadgeList（1—8B设计，未注册）交Presentation，不能以Sprite最后一帧或揭图flag替代。地图最终图标／缓存及本地查看UI的边界见[两域核对](room-save-exploration-synchronization-review.md)。

### 7.4 与Random、Presentation的交接

**2026-11-10静态交接核对完成**：[15项原生范围／同步决定与来源](room-random-handoff.md)、[机器表](room-random-handoff.json)、[Random修订2](random-synchronization-review.md)。当前职责和具体正常生产点已经明确；正式布局、缓存、reader／writer与运行验收未实施。

| 交接项 | 当前同步决定 | 后续实施 |
|---|---|---|
| Game/Level生成流、Backdrop流／seed、descriptor四seed／商店／保存随机字段 | 原值Host；保留实际配置、背景／Entity／价格／地图结果，不建别名流身份 | 资源／结果正式来源，禁止Init／Restore／补货／随机getter补值 |
| 背景、FX、雨与水面 | 实际资源／纹理／pose／选中ray／vein图／phase，实际雨Entity，reversewatergfx或等价相位；生成输入不上线 | 程序内容leaf、批次版本与受控pass；不恢复背景RNG轨迹 |
| 地图随机结果 | challenge32取已选SafeGridIndex锚点及对应描述关系，影响窗口／裁剪／标记；L2另取最终icon mask／最大帧；震动取最终offset | 正常9635B0结果缓存及安全地图adapter；原生169边界不直接安装为13×13下标，不重抽 |
| Error颜色 | 最新Descriptor的D1 1B颜色谓词，完整seed／效果ID不发；最终颜色已承接则不重复 | 实际Image与Player颜色依赖／受控官方颜色adapter；不重跑Recompute |
| D0与随机绘制 | D0现5B（Manager相位／paused）；两seed不上线，正常Pickup／Familiar逐用途alpha／资格另发 | 指定正常生产点缓存、world／presentation修订、pass／递归身份、CPU消费与颜色栈恢复 |
| 环境音频 | 新核BD0F40选sample流也留Host；实际sample、逻辑voice／gain／pitch／pan／loop／生命周期交Presentation | 正常选样本／音量生产点缓存及非随机播放leaf；不取含Host Options的后端音量，不执行Client随机Play |

第7／8步设计交接已核定；第9步统一正式编码，第10／12／13步实施完整结果批次与资源／显示／音频adapter，第14步退出旧链，第15步唯一统一验收。Room、Level、保存体与私有外层ABI未知继续保留，本次不将整步标为完成。

### 7.5 必要显示与资源（设计已补齐，待实施）

采用[具名显示与typed资源设计](room-display-resource-design.md)：RoomConfigVisual不含权重／Spawn／Merged图；Backdrop提供实际文件图像、完整有序合成recipe或精确内容，禁止RNG轨迹／Init补图。SurfaceRecipe保留SourceQuad的pixel／UV空间，资源号绑定不可变内容；程序图像不能按名字复用。水色32B、真实模式／flow／实际phase、lighting／wave结果与资源同批；Host视口／图形选项不变成world禁止显示。本地纯地图adapter使用完整可显示L0—L3、独立资源、挑战32锚点／描述及MapBadgeList，不传Host裁剪FBO或重建保存体。GPU／正式缓存／reader／writer及布局编号仍待接线，详见设计第8节。

## 改动范围

- 布局源 `state/room.json`、`state/level.json`。
- 正式房间／楼层输入由`ClientDescribeRoomLevel`生成；现有`ClientNativeWorld`先显式保留Legacy输入，第13步再换接原生writer，不构造新旧兼容转换。
- 房间构造、格子事务、门绑定等原生写入保留。

## 失败方式

- **按指针比较配置**：用配置指针值比较，或直接发送指针。两端地址不同，导致误判或解析错误。
- **set 当 vector 读**：把 `std::set` 当成 vector 读取，导致越界。
- **低频重读遗漏**：描述表容器的重读条件漏掉某类改动，例如 Myosotis、维度切换。
- **误判寻路派生**：`_gridPaths` 被误判为派生，Client 的寻路或绘制与 Host 不同。
- **局部漏读**：Backdrop、FXLayers 等大块内部有指针或缓存混入合并区间。
- **房间切换错位**：切换房间时，旧房间的块被套到新房间的身份上。房间身份变化必须走结构路径。

## 完成内容

- **静态工作已登记**：Room的账本／269行目录见其核对；Level的Game前缀与131行目录及RoomDescriptor专门B8h账本／44行目录已完成。Level根仍22B未知；描述原24B已归入Host商店表，直属未归属字节为0，外部保存／配置未知及SDK语义边界保留。表为设计数据排版，不是生产生成表。
- **两域目录已完成**：RoomSave98行、Exploration99行及源码补充接口／实际类型／保存生命周期见[核对](room-save-exploration-synchronization-review.md)。复用已有Level／Descriptor唯一owner，不按重复公开入口复制状态；保存体私有语义未知不因此清零。
- **本轮设计完成**：配置显示投影／typed资源内容、surface合成与读回合同、水光波纹／焦点、完整地图事实与本地查看、资源／批次依赖及后续接线职责，见7.5。
- **生产布局／生成已完成**：版本57的71个布局及四域597行见[实现说明](implementation-7-room-level.md)。MapFocus、MapBadgeList及独立问号资源的固定值已经登记。
- **影子描述生成已完成**：完整Room／Level／Descriptor／双配置转自有不可变描述，52个角色、L2帧界和Grid／Door布局同源生成；旧输入明确改名Legacy，见[描述层说明](room-level-description-implementation.md)。
- **仍待后续实际接线**：正常生产缓存／纯值结果来源、图形ABI／shader枚举、完整资源协议、地图／水光／FX adapter及Random／Presentation的正式采集／安装。分别由第8—14步实施，不能把值布局和描述代码写作已经运行的功能；测试只在第15步。

## 未决问题

- Room直属UNKNOWN28B及各嵌套／动态未知，详见[账本](room-byte-ledger.md)与[精确剩余范围](room-residual-evidence.md#8-仍未知的确切范围与处理)；逐字节登记不等于未知语义已解决。
- 程序背景纹理、最终water／light／shockwave的合法同world来源与正式writer仍待实施；相应生产布局及描述投影已完成。
- Random／Presentation静态交接已完成，见7.4；D1及挑战32锚点等已登记固定布局，但实际颜色／Image、正常选择缓存与地图adapter未接线。D0 5B、逐用途随机结果及环境sample／voice按第8步及后续实施。
- Level前缀／RoomDescriptor的未知、官方错误Greed Getter对照及L2地图实际来源见[Level核对](level-synchronization-review.md)；配置固定值与描述已登记，完整资源协议／图形安装未实施。`RestrictedGridIndexes`留Host，不要求为同步遍历树；完整保存体私有语义、ESSM旁表物理ABI及正式结果来源不因此冒称完成。
