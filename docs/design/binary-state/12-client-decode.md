# 12：Client 解码、校验与影子状态

**2026-11-10 第7步描述接口已实现**：[Room／Level影子描述生成层](room-level-description-implementation.md)接收本步递归校验后的完整记录、同world／presentation上下文、活动描述源槽与普通顺序、已接受配置及本Room Grid成员关系。其根与交叉关系检查不替代本步完整字段／资源递归校验，不能把单次增量或缺源／Wait转换成合法空描述；正式解码调度仍未接线。

状态：设计稿，未实施。上级：[总计划](README.md)。

**2026-11-10 各域交接统一整理**：现行入口为12.5“各域解码与影子状态交接清单”，按来源、特殊约束、版本依赖与待落实项登记全部相关域。本次静态审阅的布局源为57；布局入源、运行接线和第15步验收分别说明。本次只改本文档，不修改实现／布局源／生成表，不构建、不运行测试。

## 目标

Client 在后台线程中完成三件事：

- 对收到的成员表、资源表和状态帧做有界解码和校验；
- 维护影子状态；
- 把需要写入的变化整理成应用批次，交给主线程。

主线程不解析报文，也不做字符串匹配。

## 输入事实

- **现有 Client 的分层**：
  - `ClientScene` 只保存纯值目标和提交，不跨线程持有原生对象（`mod/src/game/client_scene.h:27,85`）。
  - `ClientObjects` 按“已应用 A → 目标 T”的纯值计划差分，执行失败后不再按历史 A 继续差分（`client_objects.h:17-18`）。
- **现有解析链**：`Parse{Entity,RoomGrid,Player}Target`、`DomainModel`、`SubjectValues`、`stream_materialize`。它们把范围模型还原成按字符串命名的目标，由本步和第 13 步取代。
- **校验规则**：见[第 9 步 9.8](9-wire-format.md#98-client-校验与接受规则)。

## 设计

### 12.1 影子状态

- **结构**：与Host规范块存储同构，保留成员／块的字节、长度、所属序列和最后变化版本。成员表保存序号→身份／布局；资源表保存具名空间、不可变typed内容描述／依赖及本地准备状态，不统一解释成路径。已接受影子目标与已成功应用目标分别记录。
- **线程**：只由解码线程写入。
- **类型化视图**：生成器为每个布局、每个块产出只读访问函数（`state_client.inc`），例如 `EntityE1::Velocity(const uint8_t*)`，取代 JSON 树和字符串成员名。

### 12.2 接受流程

1. **表消息**：按 epoch 顺序安装成员表和资源表。
   - 前一 epoch 不符时请求 Resync。
   - 成员“离开”时，影子状态中删除该成员。
   - 成员“形态变化”时，清空该成员的块，等待新形态的完整块。
2. **状态帧**：按第 9 步的规则整帧校验。通过后：
   - 把每个块写入影子状态；
   - 把（成员, 块）加入“待应用集合”；
   - 记录该序列的已接受版本。
3. **合并**：只在可构成完整合法目标、成员／资源epoch和专用依赖相容时合并重复块。不能把新拓扑的P0、旧P6或不同world／显示修订按“最后值”混装；不同序列／结构事务／事件的语义分别保留，不让已接受版本冒充已应用版本。

### 12.3 应用批次

- **交接**：主线程每帧以一次交换取走应用批次，主线程不在解码线程的锁上等待。批次内容如下：
  - 成员表操作（加入、形态变化、离开），按 epoch 排序；
  - 待应用的块，含成员序号、块号和字节副本；
  - 本批次对应的 world、body 目标版本，以及两个表的 epoch。
- **内存**：批次的字节放在现有内存池中，有上限。超出上限时停止接受新帧并请求 Resync，不截断。
- **形态完整**：加入／形态变化后，按当前布局、所属序列和合法条件分支收齐构造必需的实际投影；reserved空块和不成立的可选负载不要求不存在的字节。批次可继承同身份／同形态已确认目标的未变化块，不能借旧shape补齐新对象；超时／缺依赖报具名缺口并按正式恢复策略处理。

### 12.4 与旧路径的关系

旧的 `Parse*Target`、`DomainModel`、`SubjectValues`、`stream_materialize` 不再使用，在第 14 步移除。`ClientScene` 的提交语义不变，但输入改为应用批次和类型化视图。

### 12.5 各域解码与影子状态交接清单

以[第9步9.10](9-wire-format.md)、[版本47 Player](implementation-5-player.md)、[版本57 Room／Level](implementation-7-room-level.md)及各职责核对为输入。版本57已登记的值布局可用于生成分派，完整消息与运行decoder仍待本步；HUD、D0、随机结果、音频等未登记项不能因为已有小表而提前接收。所有下列检查均为后续实现合同，本次没有执行故障注入或测试。

| 域与来源 | 本步特殊约束 | 身份／版本依赖 | 待落实项 |
|---|---|---|---|
| 公共帧／引用／容器：[第9步9.1—9.9](9-wire-format.md)、[生成器](1-layout-source.md) | 精确消费record_bytes／块mask；memcpy读取未对齐值，递归使用nested_layout、条件／choice、fixed_count及count_bytes=0/1/2。容器数量、字段／记录／池预算同时检查，空预留不造负载 | layout_version严格相等，成员key/use/shape与空间、member/resource epoch及base/target分别校验；表未就绪按world/body规则处理 | 完整消息解析、允许集／精确长度、溢出保护与影子候选事务；未知字段不能跳过后当已支持 |
| Entity公共／Sprite／Plus：[生命周期](entity-lifecycle-synchronization-review.md)、[动画资源](animation-resource-synchronization-review.md) | exists/dead、最终颜色／selector／playing/speed保留；不等待valid、ColorParams、事件历史或Host loaded。nullable按具体编码区分Null／false／0，不能恢复原生holder字节 | 当前成员及同资源／动画上下文，当前动画层数不是Sprite总层数；非Null资源Wait／失败不同于空 | 实际字段校验／资源视图与optional解码，资源ready之外还须等待本地应用，不提前ACK |
| Familiar：[typed payload](familiar-synchronization-review.md) | Incubus amount为i32 0..100；240四份外层count只0/1且一致，非空绳索节点1..16384且XYZ有限。蝗虫两组共享计数组，不能将所有容器强求相同count | 同variant／真实DAC、共享payload及资源目标一致；读失败不能作为缺失 | typed投影／计数组校验与完整候选交接，合法缺席和非法空绳索分开 |
| Knife／Bomb／Tear／Pickup／Slot：[三类](knife-bomb-tear-synchronization-review.md)、[拾取／机器](pickup-slot-synchronization-review.md) | 仅解码保留的拖影、效果、costume、Price／DropDelay与主／辅助／奖品Sprite；已裁字段和预留块不作构造必收。DropDelay仍用具名条件计时 | Sprite、真实引用和保留计时与同world目标一致；动画playing独立于Slot State | 物理块／资源／时间合法性及保留引用解析，不按旧目录要求恢复循环或开奖负载 |
| Laser：[最终点列](laser-homing-synchronization-review.md) | 两组点列各count≤16384、各自有限／预算合法，允许数量不同与空集合；预留L2/L4/L7/L8及中间indices／Bezier不进入影子模型 | 同目标版本的两组最终几何与真实标量／Bounce及资源完整；空需能清旧点 | 独立计数／完整元素解码和同目标候选，不能只验证count或调用采样补数据 |
| Projectile：[版本45](projectile-effect-residual-evidence.md) | Height4B与PR2精确9B（u8＋未对齐Vector），浮点有限；原始u8零／非零语义允许2等值，不能套bool。PR1空预留 | 当前layout／use与公共freeze flags、资源及world目标对应 | 新旧臂／块拒绝及typed访问，不沿用“只Height”的旧版36假定 |
| NPC／Delirium：[0/2纠错](npc-control-ui-evidence.md)、[Delirium](delirium-residual-evidence.md) | N0末u8只0/2，拒绝1；三deque与双Sprite完整，144不含被派生Scale。Delirium17/18预留、Boss AI／BE8不是缺字段 | 真实RTTI／稳定公共shape、资源与null-frame依赖；不查Client设备或原版Online重算 | 0/2辅助函数接入、引用／几何／资源视图及结构候选关联，不能按相同数值type回退布局 |
| Effect：[当前分块／臂](effect-next-five-evidence.md)、[版本45](projectile-effect-residual-evidence.md) | FX0/1精确16/13B，113与167等按当前生成臂，不兼容旧映射；Trail u16 count0..127、节点64B、块≤8130B；Ghost0..4且三count一致。present0与present1空Trail不同 | Parent／payload／资源与同world帧F一致，Timeout按A−F还原；错型／未知owner不能变任意payload | 当前全部布局及条件投影分派、累计预算；非线性计时不额外要求“静止锚点不变” |
| Text／Dummy：[文本证据](text-dummy-evidence.md) | 145/146/147按RTTI／variant；TX1=u16长度＋≤255原始字节，无内嵌NUL、允许非UTF8，暂存区本地补终止。counter只146；Dummy无专有串 | 当前use／writer／合法来源scope，Manager相位／pause与显示上下文匹配 | 有界字节解码／字符串暂存与历史显示源交接；缺Dummy历史cache不能补本地帧或假对象 |
| Player body／P0—P2：[正式Player](implementation-5-player.md)、[前三块](5-player-p0-p2.md) | P0=64+L、headAnim L≤2048；固定13 Sprite与costume三投影按各源计数；P1=1B bool、P2=77B且保留真实i16／掩码，不套红心或0..99统一值域 | body仅允许正式body块，P0 topology与已安装P6相等；三costume count和目录一致，P1/P2仅world | 序列允许集、真实宽度／计数／窄拓扑拒绝和body-joint交接，不按旧count安装过期候选 |
| Player P3—P5：[中三块](5-player-p3-p5.md)、[正式源](implementation-5-player.md) | P3完整dense长度／真实固定槽／规范聚合，Pocket ID按type解释；P4精确96B，空Item其余五值规范0，Partial有限不强加0..1。P5低5位mask＋u16允许view_layout／真实type／分型Modifiers，Whip真实引用 | 同world、正确配置空间及资源epoch；固定槽不附count，aggregate无原生节点步长 | 正式choice／条件／枚举／精确长度，不能用通用12/13布局替代Player179／181—194 |
| Player P6—P8／组：[后三块](5-player-p6-p8.md)、[正式源](implementation-5-player.md) | P6有序目录／最终map；负map规范缺层但body位保留。P7每身21B，group21+2N且N≤64；独立激活／列表不等于valid。P8通常6B效果，仅资源type1/3/4且id704附cooldown；D0无节点负载 | participant/group空间9/10、引用use/shape、稠密roster／anchors、统一拓扑和同配置身份；原始History不作缺字段 | 组／跨身体引用校验、效果条件handler、残影／当前记录及总32MiB／body64KiB预算接线 |
| Grid公共／17派生：[第6步](6-grid.md)、[水面](grid-plus-water-source-evidence.md) | water完整u32，glitch只0..3，冠军专用4float与普通11值分开；按nested_layout解码，Fire≤5条16B且state分支合法，Rock条件片段／Door224描述按实际源 | index／use、Room几何／色、Door目标／Light上下文与资源epoch；数据来源Synthetic不能当native offset | 递归语义校验后提供ClientGridShadowRecordView，ClientDescribeGrid只生成描述，不代替解码／安装成功 |
| Room／背景／FX：[版本57](implementation-7-room-level.md)、[资源设计](room-display-resource-design.md) | R0 source-slot i32非成员号，13个world块按源；绑定slot0..6升序唯一／≤7且活动资源非FFFF；Wave低2位、Lighting低4位及条件长度。Vein节点显式u8 count≤10、固定10根／索引图完整，选中ray非空 | Room／Grid／门、资源、同world/presentation/pass匹配；water有符号phase与2*game_frame分开，flow无重复字段 | 已入源条件／图／几何与必要世界结果依赖校验，未执行Host图形分支不当world禁用 |
| Level／保存与探索：[版本57](implementation-7-room-level.md)、[两域核对](room-save-exploration-synchronization-review.md) | L0/L1/L3为22/53/1014B，维度0..2、五位flags、Boss所有模式保留，非Greed仅wave规范0。L3只有效源槽0..526/FFFF且拒525，路由不当数组下标 | 当前活动描述／源槽／双配置及地图修订；Grid/Safe/ListIndex各有语义，Null与未ready分开 | 结构／资源交叉解析及特殊房间生命周期；完整保存体、ESSM ID、原揭图bool不作缺字段 |
| L2／地图结果：[两域核对](room-save-exploration-synchronization-review.md)、[地图设计](room-display-resource-design.md) | L2 present0仅1B，present1长度3+popcount(mask&2B9Bh)，动态帧按bit升序且逐类界限按源。MapBadge302显式u8 count0..7／有序允许i8；Focus300/301真实锚点／成员，Question303可空资源 | L1知识、L2、两配置焦点、边框／图标／问号资源与地图／world修订一致；169不是额外合法格 | 精确帧／列表／成员／资源校验与地图根候选；不能排序徽标、用最后Sprite帧补列或把Gap折空 |
| Run：[Run职责](run-synchronization-review.md) | 132B实际块；TimeCounter为u32基准，显示种子低7位／bit7零，ascent与daily窄bool，最终分数64B；不解整flags／树／回溯体 | 同world F还原原i32计数，实际结算／进度显示owner与批次对应 | 新转换／范围／资源及跨域上下文分派，不能因Run可解码就声称HUD／菜单完整 |
| HUD／UI／D0／随机：[Presentation](presentation-synchronization-review.md)、[随机交接](room-random-handoff.md) | 未注册HUD根、Heart条件1..9B、Stat12B、有序视觉列表等先正式登记；D0设计5B，随机state0仅1B／state1为5B、不得多余字节。世界pass、消息／转场资源不靠路径猜图像 | 身体归组、world/presentation、subject/use/shape、资源及pass／重复调用／递归身份；同键冲突／旧批次拒绝 | 外层键／允许集／完整批次和未注册布局；不以本地访问次序、dirty或Options补状态，不提前接受草稿 |
| 配置／图像／shader：[资源设计](room-display-resource-design.md)、9.6 | 263配置15B，304—309固定值不等于完整ResourceTable/Blob协议；检查kind／recipe layout、尺寸乘加、格式／pixel/UV／padding、offset与冲突／完整范围、有环依赖及总预算 | 每个wire.target空间独立，resource_epoch／内容号不可重绑；表收到与本地图像ready分开 | 完整payload分派／chunk缓存／精确允许集与闭包准备；拒任意GPU命令、地址或无声量化，资源Wait不回ACK |
| Music／SFX／环境声：[音频边界](presentation-synchronization-review.md)、[sample来源](room-random-handoff.md) | 待注册事件／voice布局，区分sample资源、voice代次、loop／相位及逻辑增益；不同voice不按soundID合并，FrameDelay非启动延迟 | 事件去重键、持续状态Baseline、开始停止clear及场景／资源依赖，不继承Host Options | 有界voice／事件解码与应用候选；事件不按latest-only块覆盖，未播放／缺资源不是合法停止 |

整帧接受、依赖就绪、影子目标和原生应用成功是不同状态。批次同时携带必要结构／序列／资源及专用修订，保留可合法继承的未变化块；依赖不兼容则重建完整候选，不逐块拼接不同目标。应用失败／主线程长时间不取批次的恢复和内存上限仍待实现，不回复未成功应用的target。

## 改动范围

- 新增：解码器、影子状态、应用批次。
- 修改：`ClientScene` 的输入接口。
- 移除（第 14 步）：旧的解析链。

## 失败方式

- **部分接受**：校验通过一部分就写入影子状态，导致影子状态处于两个版本之间。
- **主线程等待**：主线程取批次时等待解码线程，帧时间抖动。
- **批次无界增长**：主线程长时间不取批次（例如加载画面），批次一直增长。
- **半状态构造**：成员形态变化后，旧块和新块混在一起，用不完整的块构造原生对象。
- **ACK 过早**：已接受版本被当作已应用版本回复 ACK，导致 Host 认为 Client 已写入。
- **旧表解码**：资源表或成员表还没安装，就按旧表解码了新帧。

## 完成内容

本次仅完成文档整理与静态审阅；以下为本步骤后续实施／验收需要交付的内容，不表示已经实现、编译或通过测试。

- **实现交付**：需完成解码、校验、影子候选与有界应用批次，并在实施阶段完成相应纯产品编译；本次未实现或构建。
- **验证安排**：验证在第 15 步完成。

## 未决问题

- 加载画面等主线程长时间不取批次的情况：批次上限是多少，Resync 的门限怎么定。这些只是运行参数，不是推进 gate。

## 历史修订提示与原分域补充

以下保留原页首与原正文中的修订补充，按各自时点阅读；旧版本号、未生成／待注册和原位置措辞不覆盖上文现行清单。版本47的Player和版本57的Room／Level已经入源，完整消息／结果批次及运行owner仍按各项实际状态说明；历史失败、UNKNOWN及尚未观察的事实不改判。

**原页首提示**

**2026-11-10 第7步版本57交接**：[正式值布局](implementation-7-room-level.md)与四域597行已生成。后续解码按Field.count_bytes区分固定槽／u8／u16计数，识别合成资源target与可选嵌套类型；配置／地图／完整资源依赖及运行decoder仍待本步，固定typed头不代表完整Context协议已经实现。

**2026-11-10 第7步显示／资源校验合同（待实施）**：按[typed资源设计](room-display-resource-design.md)区分文件／15B配置／固定recipe／精确像素，检查尺寸／数量乘加、kind／layout、pixel／UV空间、格式／logical与stored尺寸、chunk范围与冲突、无环依赖及整上下文预算；不接收原生指针、生成图或任意GPU命令。RoomConfigVisual保留StageId，Home35不能按Type解析。必要water/light/wave/focus、地图L0—L3／焦点／资源同版本就绪后才提交；Gap不伪装Null／空mask，资源表收到不等于图像ready。未注册新视图不由版本56提前接收，无测试。

**2026-11-10 RoomSave／Exploration解码交接（待注册）**：按[目录核对](room-save-exploration-synchronization-review.md)只接受已注册的探索／地图结果，完整保存体／ESSM ID／揭图生产flag不是缺字段。MapBadgeList草稿校验count0..7、恰好1+count字节、有符号ID仅−9..−1或0/1/2/4/5，保留原顺序，不排序或按Sprite最后帧补列；绑定同world地图／资源修订，来源Gap不能编码为空。该u8计数是显式具名视图，未注册前版本56不得接受。L1探索知识与L2聚合、challenge32上下文、必要边框／图标资源共同满足才提交；不以Host展开状态覆盖本端UI。本轮无解码实现或测试。

**2026-11-10 第7步／Random结果解码交接（待注册）**：按[静态核对](room-random-handoff.md)D0现5B，仅u32 Manager相位＋规范u8 paused；两个seed不再是接收字段。逐用途随机结果state0严格1B，state1附f32 alpha共5B标量，外层subject/use/shape/pass/重复调用身份及world state revision／presentation revision／资源版本必须匹配，不将未执行／未产生／读失败编码为state0或alpha0。相同具名键的冲突、缺依赖、旧批次、非法资格／条件长度或多余字节拒绝，不能以本地访问次序补映射。挑战32锚点i32与对应描述稳定关系共同解析，169不直接索引13×13维表；音频sample资源与逻辑voice代次分开，不能按soundID合并不同voice。草稿尚无正式layout号，生产版本56不得提前接受；本轮仅文档，无测试。

**2026-11-10 RoomDescriptor解码修订（待注册）**：按[专门设计](room-descriptor-synchronization-review.md)display_bits只允许低5位；所有模式保留Boss匹配，仅Greed wave非Greed规范0。D1仅1B规范bool error_is_modeling_clay并绑定当前world；不接收完整效果ID，不以此伪造数值Getter。L2 present0严格1B；present1含u16 mask，按bit升序解析mask&2B9Bh的动态帧，长度必须3+popcount(mask&2B9Bh)，3..12B，固定帧0不附字节。逐类帧上限分别9/3/2/1/1/1/9/8/5；拒绝坏bool、条件长度／帧界／依赖／world错误及多余字段，不接收保存数量／顺序／ID／头，也不把Gap当缺席。新视图未注册，生产版本56不接收草稿；原四位／非Greed清零旧段由本段取代，本轮无测试。

**2026-11-10 Level解码交接（布局待注册）**：按[Level静态设计](level-synchronization-review.md)区分物理描述源槽、可变ListIndex、Grid／Safe及当前入口象限；维度只接受0..2，L3的u16仅0..526或FFFF且不得引用Deathmatch槽525、缺失／不活动描述。八门目标的-100/-101保留路由语义，不当负数组下标；活动当前房间须实际可解析到本world的合法Room／描述，NO_ROOM_IDX不得伪装有效房间。双配置资源未就绪与Null分开，visited／lastBoss／Greed图标严格bool，display_bits只允许低5位，仅Greed wave非Greed规范0，Boss匹配所有模式真实保留。22/53/1014B仍是设计视图，生产版本56尚不接受新Level记录；不能按草稿长度绕过已注册layout分派。完整保存体、seed与全flags不因旧目录存在而恢复下行；未知及实际地图结果来源缺失须报具名Gap。本轮未接线或测试。

**第6步Grid描述生成接口已落地**：见[实现](grid-description-implementation.md)。本步后续对完整已接受记录提供`ClientGridShadowRecordView`：每块带key／use／shape／layout及最后变化版本，保留本目标未变化的旧块，reserved空块可缺席。递归验证资源、ANM2、选择器、图层、Rock条件存在性及目标epoch后才能调用`ClientDescribeGrid`；它仅核根结构并生成自有只读描述，不替代本步解码器，也不表示原生安装成功。

**版本50 Grid解码修订（待实施）**：按[复核](grid-common-synchronization-review.md)解码Desc198／Grid199与展示视图200／201。G0的water_clip_flags是完整u32最终值，无0—255窄限制，不是原生+3C或REP optional内存；G1的glitch_draw_bits仅接受0—3，结果绑定同subject/use/shape/Sprite角色。拒绝旧版本、截断、无效资源／index／来源绑定及glitch高位。默认名与图层路径不再有负载，不沿用旧字段偏移；UNKNOWN、实际图像和所有保留姿态校验保持。合成字段source为Synthetic，解码只写影子值，不能根据伪原生offset写对象；安装／绘制owner未就绪不得ACK。 冠军参数按16B f32[4]解码并检查四值有限，不能按44B ColorMod读越界；普通颜色11值校验保持。

**版本47布局来源（2026-11-09）**：[第5步正式源／生成](implementation-5-player.md)已完成，EntityPlayer=197，participant／control-group=195／196，Player主动槽=179，条件效果=180，动态Weapon=181—194；物理0—24及条件／固定槽／预算元数据以唯一源和生成表为准。以下版本46／草稿／待生成表述保留原时点；本步骤的运行接线仍未实施，不能因布局已生成就提前接收、采集、安装或宣称验收通过。

**第5步P6—P8解码合同（2026-11-09，未实现）**：[草稿格式及合法性](5-player-p6-p8.md)规定P6为111＋4C B，三项P0 costume count须一致；map负源折成FFFF／layer0，body标记仍保留，Host明确Null ANM与Client资源未准备不能混同。P7精确21B，校验独立激活／列表蕴含、roster唯一稠密顺序、同组身体／active_anchors及引用use／shape；不要求valid等于room或Exists，不接收sub／replaced／703四槽。C/S group为21＋2N B、participant／group分别空间9／10，实际layout号待生成。P8保留有序效果，配置必须非Null，仅collectible704条目10B，其余6B；disabled独立，残影每项12B，D=0无负载，D=1当前记录24B＋实际ANM2、second=-1时second_frame=0，恰好消费整块。原始History／16索引／forms不作缺字段；16／17／18只走world。版本46尚未注册本草稿，不提前接收。

**第5步P3—P5待实现合同（2026-11-09）**：按[草稿格式](5-player-p3-p5.md)解码P3的70B前缀／完整dense长度／有序map与blocked＋有效innate；不要求group／timer／voided／queued charge等已裁字段。四Pocket的ID按真实type解释，空type保留u32，非空ACTIVE_ITEM解成slot+1；queued只接受饰品配置与golden投影。P4精确96B，空Item的五值必须为规范零，occupied partial要求有限，不套0—1或通用charge6上限。P5 mask只许低5位，按槽序核view_layout／type／精确长度；十类为16B值、5／6／8／12为4B、15为6B，Modifiers拒绝超出分型mask的位，Whip引用按成员表解析。三块只能world；当前46尚未注册这些Player草稿layout，不提前接收。

**第5步P0—P2待实现合同（2026-11-09）**：见[草稿布局及边界](5-player-p0-p2.md)。P0本体为64＋L字节，headAnim原始字节L≤2048；19—24为13槽及costume的三类ANM2投影，三count须相等且匹配已安装P6拓扑版本。Player父E6同body；body不得夹带P1／P2。P1只接受1B bool；P2精确77B，extraLives为i16、BlackHearts为原始掩码，其余按具名宽度，不套0—99／red≤max统一条件。EternalHearts／BoneHeartsMask、六项Player模拟stats与cacheFlags不要求出现在Player记录。拟定块号0—24仍在32块内；实际Player布局尚未生成，当前版本仍46，不提前接受这些草稿负载。

**版本46 Delirium口径**：见[静态处理](delirium-residual-evidence.md)。整数布局版本按当前46比较；两个新增u64只是Host原生归属，线上字段／长度不变，143的17／18仍空预留，受控UI仍只接受0/2。不接收旧DL0／DL1负载，不由缺失AI值或BE8补造Client状态；本轮未运行测试。

版本45按[残余结论](projectile-effect-residual-evidence.md)保持Projectile PR0为4B f32 Height，并解码新增PR2的9B：u8 height_render_mode／Vector freeze_render_position，偏移0／1；浮点必须有限，u8按零／非零解释，不能以bool校验错误拒绝2等原始值，也不能未对齐强转。Effect新增148／149，167不再接受102；19／71／194不开放公开TearFlags，FX2仍预留。119 callback专用尾部及其余UNKNOWN不进入影子状态，不接收地址或假造DamageFlags负载。实际帧解码接线仍为本步后续工作。

版本44 Text／Dummy按[逐段证据](text-dummy-evidence.md)解码145／146／147。TX1必须为u16 count＋精确count原始字节，count≤255、无内嵌NUL，非UTF8允许；`StageCStringInto`只在自有256B暂存区补NUL。TX0为四个有限f32，TX2只在146有i32 counter，145的12号块预留；不得等待duration／颜色空间标签或Dummy专有负载。Manager帧／pause依赖同world展示上下文，缺失不可换用本地帧。旧独立Render来源保持原scope，不拼为当前普通快照。实际帧解码／影子状态接线仍属本步后续工作。

**原正文分域补充**

**版本33 Familiar负载**：按[Familiar参数／充能／绳索复核](familiar-synchronization-review.md)只对实际线上字段校验：Incubus amount为i32且0—100；240四个payload投影count仅0／1并一致，非空payload的节点数为1—16384、XYZ有限，全部同目标版本齐备。原始轨道／方向／HeadFrameDelay／Hearts／Keys／RoomClearCount不上线，不为它们构造拒绝负载。节点数0的下限及owner存在性不能只交给通用u16上限检查；不把未知类型／读取失败当缺失。

**版本34 Knife／Bomb／Tear负载**：按[Knife／Bomb／Tear参数展示复核](knife-bomb-tear-synchronization-review.md)只解码实际保留字段；42个Host范围及其三个u64锚点不进入影子状态／校验／应用批次。Bomb B0、Tear T0／T2等reserved空块不作为构造必收的状态块，不因空块存在而等待不存在的负载；其他实际块及资源仍须完整。旧布局负载由整数版本不一致拒绝，不做逐字段兼容补丁。

**版本35 Pickup／Slot负载**：按[Pickup／Slot选择、交互与奖励复核](pickup-slot-synchronization-review.md)只解码实际保留字段，原19项字段／容器与Pickup Timeout／Wait锚点不进入影子状态或应用批次。Pickup P3、Slot S0／S1预留且不等待不存在的负载；不再检查已撤销循环列表的线上count，其他真实容器／资源约束保持。DropDelay的条件编码与Price、辅助Sprite／关系仍须同目标版本完整。

**版本36 Projectile负载**：按[Projectile运动与展示参数复核](projectile-synchronization-review.md)只保留PR0的Height及继承状态／资源；八项Host输入不进入影子状态或应用批次，PR1预留不作为构造必收负载。Height有限值和真实资源／引用约束保持；不构造已撤销字段的拒绝负载。私有展示来源尚未闭合，不以合法解码当前已知字段声称画面完整。

**版本37 Laser负载**：按[Laser／Homing采样、运动与计时复核](laser-homing-synchronization-review.md)只解码L5／L6的optimized／unoptimized最终点列及真实展示标量；L2／L4／L7／L8与Homing子块0／3／4预留，不等待空负载或构造虚拟投影。十九项Laser输入、六项Homing标量、indices／Bezier与系数不进入影子状态，两个Laser锚点撤销。两组独立count各自≤16384，有限值、预算、来源／版本及资源约束保持；不要求优化与未优化点数相等。旧indices i32类型记录已纠正，不再保留其4B线上codec。

**版本38 Effect负载**：按[Effect各臂与payload复核](effect-synchronization-review.md)FX0仅8B、FX1仅21B；全部FX2空预留不作为构造必收负载。已排除六参数、TearFlags与136描述不进入影子状态／validator／应用批次。Ghost只解码有效Sprite前缀，三投影u16 count必须同目标版本相同且≤4，由此重建原生i32 count，不另收count原值；present=0时三个count为0且各投影最短全零。视图134没有Offset／Scale负载，通用ANM2不变；源原生i32负数由第10步编码前拒绝。Trail节点已核完整64B，保持u32绘制参数原位及其余真实浮点、资源、预算校验，不恢复Rotation误名。

**版本39装备／Plus负载**：按[完整复核](player-equipment-plus-synchronization-review.md)ActiveItemDesc24B、Weapon16B、Whip135号18B、TemporaryEffect10B、TemporaryEffects136号disabled＋u16 count及完整列表；四主动槽／五Weapon槽存在性、顺序与动态形态不可缺。只解码真正上线的保留字段与Whip真实引用，已撤销八范围不进影子状态／validator。KnifePlus子块1空预留不等待负载；公共两nullable仍严格保持-1／0／1和0／高32位恰为1编码。Familiar D76只接0／1，新增外层disabled只接0／1；配置资源与type/id、原生f32、真实引用、计数及全局字节预算照常核对，不能把坏资源或来源失败伪装为空槽／空列表。
**版本40 Effect后续五项解码合同**：见[证据与落地](effect-next-five-evidence.md)。整数布局版本40；接受生成表的11个Effect布局，102不再包含113，113只接受141；未知／不匹配形态拒绝，不能按负载相同回退旧臂。FX0按16B解码State／Timeout到期帧／Rotation／Scale，FX1按13B解码LifeSpan／following／ParentOffset；不使用旧8／21B常数，FX2空预留不等待。Trail u16 count只接受0—127，count=128及以上拒绝，真实节点块最大8130B；present=0和present=1且count=0保持不同语义，不能截断或把多余字节丢弃。FX0／FX1浮点及引用／资源／目标版本／累计预算约束不变。137—141中不上线的VarData原值、函数地址和未知字节不进入影子模型，也不为其虚构线上validator；未知非空owner由Host报缺口，不在Client解码成任意新payload。未运行测试。

**版本43 NPC现行合同**：N0仍25B，末字节由DecodeNpcControlledUi仅接受0/2；1及其他值必须拒绝，不能查询本机设备/原版Online来补结果。143布局17/18仍空，NpcOverlaySprite144仍不含Scale，长度和其他依赖保持版本42。整数布局版本43直接比较，不兼容旧版本猜读。见[来源闭合](npc-control-ui-evidence.md)。
