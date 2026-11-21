# 12：Client 解码、校验与影子状态

**2026-11-15 用户采纳，版本79**：普通Body新增一个可替换等待位及一个成功前代解码基线，完整重建后相对当前状态应用；实际应用基线仍锁定。详见[第13步10.4](13-client-incremental-apply.md#104-普通body单槽等待与前代解码基线2026-11-15用户采纳)，覆盖本文冲突的零等待／旧base必拒绝描述。其他完整检查、真实成功ACK与失败恢复原则保持，同一步实机续验尚未通过。

**2026-11-15 设计修订，尚未实施**：[增量应用与双通道](13-client-incremental-apply.md)规定稳定承载、不可变块引用表、变化块解码与依赖事实复用，并同步拆分Receiver／Shadow／Acceptance的普通World／Body在途状态。重复检查仅在明确有效范围内合并；未变块的依赖失效仍须传播。新设计覆盖下方冲突的全局单批约束，历史实现状态不变；本次未改运行代码、未构建、未测试。

**2026-11-13 原生 owner 续接**：[原生原子安装器](implementation-12-atomic-install.md)已有具体 `ClientAtomicInstallBindings`。默认构造接入实体 Init、Plus／ANM2 等特殊字段和真实 Room、Player、EntityList、Grid 成员发布，容器租约及失败排空由同一事务管理。完整显示、事件播放器和正式 Manager／Start 仍需接线。布局74不变，第15步唯一验收。下方同日较早交付范围保留原时点。

**2026-11-13 安装计划、资源准备、根对象与字段暂存已入源，正式原生串联未完成**：[接收／事务组件](implementation-12-client-receive.md)已有网络receiver、公共校验、事件账本、邮箱和事务协调器；随后补齐[完整数字计划及Scene描述](implementation-12-install-plan.md)，实现[主线程资源物化／反馈与根对象暂存](implementation-12-main-preparation.md)，并补齐[StageFields嵌套容器／共享对象／Weapon暂存](implementation-12-stage-fields.md)。完整字段安装／发布、真实原生资格owner、事件播放器及Client episode计费仍未接入；正式Start未提供完整准备owner，Client仍不声明二进制能力。不能标为第12步全部完成或整条原生链已运行。未运行测试，第15步唯一统一验收。

**2026-11-12 显示委托修订**：

- 解码、校验与影子状态机制保留，影子状态随域收缩。
- 新增事件队列的接收：去重、乱序处理、有界、过期丢弃。
- 见[修改方案](client-display-delegation.md)第 2.3、7 节。

本次只修订文档，未改代码，未运行测试。

**2026-11-12 HUD代码迁移，版本70**：435已有正式正文编解码和anchor≤update等纯值约束；436提供冷连接／恢复时真实缺失的准确历史P8输入布局，当前P8能满足时复用；本地准备强制精确P8修订与资源，完整候选validator／历史输入解析及调度尚待本步后续接线。 具体接口、事实及剩余接线见[实施说明](hud-local-implementation.md)。第15步唯一统一验收，未运行测试。

**2026-11-12 HUD本地化校验交接，仅文档**：按[现行合同第6节](hud-local-presentation.md)区分本地派生用途与保留Host结果，验证同身体／组的P3／P4／P8、critical_health、Drop驱动／更新／恢复相位、拓扑及实际资源，候选携带稳定自有输入。第9步迁移后不再等待已取消的low_health_flash、holy_mantle_visible或普通图标／中心、Drop逐帧缓存；真实缺输入／本地资源未ready与合法隐藏仍分开。心格／三色闪烁、特殊图像、列表顺序、Stat三值／概率及共同提交规则保持。本次未修改已实现影子库、schema或整数版本，无生成／构建／测试，完整校验与本地准备仍待后续。

**2026-11-11 第3项完成，布局版本67未变**：[影子状态与候选事务](12-client-shadow.md)已实现成功base继承、完整根／投影及引用重绑定、自有Frame元数据、结构意图、强制完整validator／缓存提升接口和实际busy清理分离；候选不冒充installed或Ready。Release／Win32状态库及产品DLL纯编译／链接成功，未运行测试。第4—7项及第13步仍待接线，第15步唯一验收；第2项当次整体构建失败保留原时点。

**2026-11-11 第2项完成**：[成员表与资源表管理](12-client-tables.md)已实现精确epoch自有快照、同epoch重传／冲突、成员历史及序号借用检查、按项共享的资源内容／数字图、独立准备票据与代次失效、每类5份快照和共享池／Peer子额度。状态库Release／Win32纯编译成功；整体产品尝试构建停在当前Host桥BinaryEntities不完整类型错误，未记为通过。本项不改布局；最终状态库编译随共享源版本67。未运行测试，第3—7项与第13步接线仍待后续，第15步唯一验收。

**2026-11-11 第1项完成，版本仍66**：[接口、数据寿命与失败处理](12-client-contract.md)已核定现有借用视图、后台唯一修改者、精确快照／正文／影子根／候选／应用批次及准备／完成／排空接口，明确逻辑inflight与实际native busy分离、预算与具名失败分类。更正过时源码路径和未实现的访问器示例。本项只改文档并做静态审阅，没有新增运行代码、生成、构建或测试；第2—7项实现及第15步唯一整体验收仍待后续。下方“实施启动”和旧版本补充保留原时点。

**2026-11-11 实施启动**：按用户要求先明确工作范围与顺序，见12.6。本步直接依赖第9步，可在第10／11步尚未完成时实现；复用当前纯值编解码、资源闭包、接受状态机及预算合同，不把它们已存在等同于完整Client owner已接通。本次启动先完成工作分解，代码尚未实施；第0—14步不运行测试，第15步唯一统一整体验收。

**2026-11-11 版本66压缩交接**：[world/body独立zstd](9-world-body-zstd.md)补齐授权route与有界单帧解压，随后仍执行完整信封／scope／正文／候选检查。原64KiB解压保护不变，不依赖前一消息／字典；解压成功不提前ACK。

**2026-11-11 第9步收尾交接，版本65**：[现行容量／时间](9-budget-capacity.md)与state/budget.*提供两个候选槽，world100ms／ordinary joint500ms、完整恢复共用原10秒；替换不续命。完整候选核总活跃结果≤1024组、每类≤5份实际被引用表快照及共享CPU池／子配额，版本和scope／route仍严格检查。资源／原生未成功不ACK，真实调度与owner在本步接线。

**2026-11-11 第9步第五项交接，版本64**：[接受与恢复合同](9-acceptance-recovery.md)已实现先定界、完整结构解码及纯值接受状态机。此步提供StateCandidateContext，核完整候选的继承／投影、全部必要结果、引用／用途／来源／资源闭包及窄拓扑；不能仅凭DecodeStateFrame成功返回Ready。候选／在途／成功安装版本分开；旧base补原ACK，污染事务等待base=0重建。

**2026-11-11 第9步第二项迁移，版本61**：[正文合同与新布局](9-domain-body-codec.md)已登记422文字／423有序行／424本地历史颜色描述，替换文字图引用、移除Host反馈与切房surface，374收窄为独立Boss versus；305／306正文禁用、304拒绝kind5。字体／特殊首码映射与真实封存、本地缓存、完整资源接受及原生显示仍由10—13步落实；取消的Host图像不形成Wait，真正必要依赖不早ACK。公共头及带宽预算保持。
**2026-11-11 当前解码职责调整**：[本地生成合同](local-media-presentation.md)：校验有界文字／有限样式、本地资源与效果参数；当前不接受未启用的原始媒体payload。直接切房与本地hallucination不再要求Host前后surface／snapshot就绪，真正必需状态、文字、资源与原生提交仍不得缺失／早ACK。旧图像必发和像素一致性要求被替代，版本60现有值布局及解码实现尚待后续迁移。

**2026-11-11 第9步各域正文实现，版本60**：[正文实现与边界](9-domain-body-codec.md)、[同源数字计划](../../../../mod/binding/layouts/09-wire-format/body-codec-plan.md)。DecodeRecordPrefix以精确成员快照的expected member/layout解码，返回只读块、可选根字段及投影事实，不建JSON树。704依赖同资源表配置，shared尾投影依赖同候选首presence；源缺失报Gap。借用span须由正式owner持有或复制，完整影子按根身份／目标覆盖合并事实后校验，不能把BodyView当可ACK安装；会话调度仍待本步。

**2026-11-11 第9步公共合同，版本59**：[正式结构](9-public-wire-contract.md)。按版本59的完整scope、精确不可变表快照、12B头／u32掩码、结果依赖与事件身份解码；表到达不立即删除已展示对象。双分段整目标准备／提交，ContextReady不冒充资源ready或AppliedAck；body按Peer确认，未确认事件不能随latest-only候选淘汰。固定头已生成，完整payload／允许集／decoder仍待后续。

**第8步版本58交接**：[正式布局与回映射](implementation-8-presentation.md)已生成。按同版本数字布局解码固定结果；353隐藏心格不读payload、354只按星象房资格读取第8行，356—358不含随机流，365／366保留明确贡献与图像空间。外层身份／资源与各组合法结果全部就绪前不安装／不ACK；未闭合来源不变成空结果。

**2026-11-10 显示批次解码定稿交接**：[方案B](presentation-result-batches.md)要求有界校验真实来源／目标映射、owner与subject/use/shape、控制组／身体选择、资源／绘制拓扑依赖和有序贡献。原生History／Stat两槽不限制参与者数；本端详细主体从已确认本端控制组解析，不取roster第0项。每组依赖和原子范围按正式布局执行，未产生、读取失败、资源Wait与合法空值分别处理，不拼跨代结果，不安装／ACK不完整目标；仍只有world／body两序列。正式封装未实施，版本57不变，无构建或测试。

**2026-11-10 第7步描述接口已实现**：[Room／Level影子描述生成层](room-level-description-implementation.md)接收本步递归校验后的完整记录、同world／presentation上下文、活动描述源槽与普通顺序、已接受配置及本Room Grid成员关系。其根与交叉关系检查不替代本步完整字段／资源递归校验，不能把单次增量或缺源／Wait转换成合法空描述；正式解码调度仍未接线。

状态：第1项接口合同、第2项表管理、第3项纯值影子／候选事务已完成，第4—7项及正式Client运行接线尚未完成。上级：[总计划](README.md)。

**2026-11-10 各域交接统一整理**：现行入口为12.5“各域解码与影子状态交接清单”，按来源、特殊约束、版本依赖与待落实项登记全部相关域。本次静态审阅的布局源为57；布局入源、运行接线和第15步验收分别说明。本次只改本文档，不修改实现／布局源／生成表，不构建、不运行测试。

## 目标

Client 在后台线程中完成三件事：

- 对收到的成员表、资源表和状态帧做有界解码和校验；
- 维护影子状态；
- 把需要写入的变化整理成应用批次，交给主线程。

主线程不解析报文，也不做字符串匹配。

## 输入事实

- **现有 Client 的分层**：
  - [`ClientScene`](../../../../mod/src/game/client/scene/client_scene.h)的跨线程目标／结果使用纯值，原生承载和借用仍由主线程owner管理；现有AdmitTarget／AdmitStream输入仍是旧模型，二进制批次接口尚未实现。
  - [`ClientObjects`](../../../../mod/src/game/client/entity/client_objects.h)按“已应用 A → 目标 T”的纯值计划差分，执行失败后不再按历史 A 继续差分。
- **现有解析链**：`Parse{Entity,RoomGrid,Player}Target`、`DomainModel`、`SubjectValues`、`stream_materialize`。它们把范围模型还原成按字符串命名的目标，由本步和第 13 步取代。
- **校验规则**：见[第 9 步 9.8](9-wire-format.md#98-client-校验与接受规则)。

## 设计

### 12.1 影子状态

- **结构**：与Host规范块存储同构，保留成员／块的字节、长度、所属序列和最后变化版本。成员表保存序号→身份／布局；资源表保存具名空间、不可变typed内容描述／依赖及本地准备状态，不统一解释成路径。已接受影子目标与已成功应用目标分别记录。
- **线程**：只由解码线程写入。
- **数字只读视图**：当前`state_client.inc`已生成Validator／ClientField／CountConstraint等表，`FindClientField`提供数字分派元数据；旧`EntityE1::Velocity`仅是设计示例，没有该访问器实现。后续由同源layout／field索引及已解析有界片段提供只读访问，具名域视图复用已有生成计划；变长字段不假定固定偏移，未对齐标量用memcpy，不构建JSON树或运行字符串成员查询。视图／scratch寿命见[第1项合同](12-client-contract.md#5-借用寿命审阅与封存规则)。

### 12.2 接受流程

1. **表消息**：按 epoch 顺序安装成员表和资源表。
   - 前一 epoch 不符时请求 Resync。
   - 成员“离开”时，在构建快照中标记退休；不要删除已提交目标，原生退休随完整目标提交。
   - 成员“形态变化”时，为新代次构建空候选并等待完整块；保留仍合法的已提交目标，不混入新形态。
2. **状态帧**：先按第9步定界／版本规则处理旧base与重复消息，再按精确快照解码并校验完整候选。通过后只构建候选影子：
   - 把每个块写入影子状态；
   - 把（成员, 块）加入“待应用集合”；
   - 记录该序列的候选版本；它不覆盖已成功安装基线，不授权下一条增量继承尚未提交的候选。主线程Begin前再核资格，整批成功Finish后才推进成功安装版本。
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
| 配置／图像／shader：[本地生成合同](local-media-presentation.md)、9.6 | 校验263配置、本地资源声明、文字长度／样式及有限recipe，检查乘加、空间、依赖环与总预算；304—309登记不启用原始媒体／blob正文 | 每个wire.target空间独立，resource_epoch／不可变描述身份保持；本地资源准备与表收到分开，取消的切房／Host反馈图不再是Wait | 完整payload允许集、文字／本地生成候选与真正必要依赖闭包；拒未启用媒体、任意GPU命令／地址，真正必要资源Wait不早ACK |
| Music／SFX／环境声：[音频边界](presentation-synchronization-review.md)、[sample来源](room-random-handoff.md) | 待注册事件／voice布局，区分sample资源、voice代次、loop／相位及逻辑增益；不同voice不按soundID合并，FrameDelay非启动延迟 | 事件去重键、持续状态Baseline、开始停止clear及场景／资源依赖，不继承Host Options | 有界voice／事件解码与应用候选；事件不按latest-only块覆盖，未播放／缺资源不是合法停止 |

整帧接受、依赖就绪、影子目标和原生应用成功是不同状态。批次同时携带必要结构／序列／资源及专用修订，保留可合法继承的未变化块；依赖不兼容则重建完整候选，不逐块拼接不同目标。应用失败／主线程长时间不取批次的恢复和内存上限仍待实现，不回复未成功应用的target。

### 12.6 实施范围与工作顺序（2026-11-11）

本步交付从“重组／解压后的自有二进制正文”到“完整、不可变、有界的主线程应用批次”的Client链路。第9步的`wire_protocol`、`body_codec`、`resource_protocol`、`presentation_envelope`、`acceptance`和`budget`作为已有组件复用；它们的借用视图由本步实际owner持有字节／scratch寿命并接入共用内存账本。实际原生写入、GPU／音频物化及污染承载清理由第13步实现，旧链删除归第14步；第12步仍须完整提供这些工作的纯值输入、准备反馈与提交完成协议。

以下序号只表达本步骤内部的实现顺序，不新增推进编号、提前测试或逐项验收：

1. **确定接口与失败处理**。核对第9步现行数字合同，先列明下方失败方式；明确后台owner、纯值租约、表快照、影子根、候选与成功安装凭据的关系，以及第13步的准备／完成反馈。生成的数字字段视图与变长访问复用同源布局，不按字符串重建JSON树。
2. **实现成员／资源上下文管理**。安装精确epoch的不可变表快照，保留成员退休／换形及合法序号回收历史；同epoch重传核精确内容，旧候选始终使用自己的快照。表到达不删除已展示对象，元数据收到与本地资源准备完成分别记录。快照引用释放后再回收，每类至多5份，实际capacity计共享池与Peer子额度。
3. **实现影子状态与候选事务**。按完整身份、layout、world／body序列保存块字节及最后变化版本；baseline建立完整根，delta只继承同身份／同形态的成功安装基线，不能继承尚未提交的另一候选。先暂存、完整检查后发布；加入／换形要求所有实际必需块及投影，reserved与不成立条件不造字节，joint保持共同目标。
4. **接入各域完整校验与描述视图**。按12.5逐域落实递归字段／条件／choice、计数组、共享presence、引用身份、空间／layout、Player组／身体关系及窄拓扑检查。把完整Grid、Room／Level根交给已有描述生成接口，并提供第13步所需数字只读视图；单条record合法不替代完整候选合法，缺口不能折为空值。
5. **接入显示结果、资源与事件**。落实全部必要结果覆盖、owner／subject／use／shape、贡献顺序、缓存来源及精确epoch；按本目标实际资源闭包准备，区分Pending与确定缺本地资源。world事件有界保留、连续去重并拒绝同号冲突，不随latest候选替换丢弃。文字／本地资源描述按现行合同处理，不等待已取消的Host像素／切房surface。
6. **接入候选调度、预算与恢复**。用`ClientStateAcceptance`驱动先定界、旧base补ACK、精确表解码和完整候选检查。普通world与可靠／joint各至多一个等待槽，普通单body未就绪即丢；替换不刷新期限，超时／表链断裂／base丢失等触发具名恢复，非法编码／引用或确定缺必需本地资源明确终止。缓存、事件、快照及工作区按现行预算有界回收，恢复沿用原episode期限。
7. **实现主线程交接并完成纯产品构建**。发布不可变应用批次，携带成员结构变化、块／描述、结果／事件、精确表及world／body凭据，主线程取批次不等待后台锁。申请执行时重查资格，完成反馈回到串行owner；只有第13步实际整批成功才推进安装版本／事件水位并生成AppliedAck，可能部分写入则撤销基线资格、请求完整重建。同步接口文档并进行不含测试的纯产品编译／链接，运行行为统一留到第15步验收。

第1项具体合同见[接口、数据寿命与失败处理](12-client-contract.md)。第2项已有[表管理](12-client-tables.md)，第3项已有[影子／候选事务](12-client-shadow.md)；结构完整与全域／显示／资源Ready分开，必须经完整validator与预留缓存提交，不提供默认成功实现。后续第4—7项继续按上列顺序实施，不能把尚未实施的实际批次／worker写成已接通。

第12步完成的含义是上述Client纯值链路及第13步消费接口完整落地，不表示Host采集／发布、原生安装或端到端行为已经完成。第10／11步按第9步协议提供真实数据时再汇合，不为编写本步代码额外要求它们先完成。

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
- **借用寿命错误**：下一次解码复用scratch或正文后，旧候选／缓存仍持有其span；共享字节被重复计费，复制内容却沿用旧租约漏记实际capacity。
- **候选冒充基线**：新delta继承尚未原生提交的候选；joint只提交一个序列，或同号target改绑base／正文。
- **不完整依赖被接受**：单条正文合法，但跨块计数／presence、成员空间／layout、必要结果或窄拓扑缺失；资源Pending被改Null，确定缺文件仍无限恢复。
- **来源与事件污染**：缓存按新epoch解释旧序号，旧use／shape的显示结果复活；替换候选丢未确认事件，重传重播事件或接受同序号不同正文。
- **恢复无限延期**：重复包、候选替换或Resync刷新起点；普通body积压等待，无引用的旧表／结果不能回收，超过Peer子额度仍继续分配。
- **迟到完成误确认**：旧route／serial完成反馈进入新会话；原生可能部分写入后仍以历史成功状态继续增量或重发涉及污染事务的ACK。

## 完成内容

2026-11-11第1项已完成[接口合同](12-client-contract.md)，第2项完成[表管理](12-client-tables.md)，第3项完成[影子／候选事务](12-client-shadow.md)及产品纯编译／链接；第2项当次构建失败保留原事实。各项均未运行测试，第12步整体仍需以下运行交付：

- **实现交付**：已有精确表／成员历史／准备状态与纯值影子事务；第4—7项需完成全域具名检查、结果／事件缓存、调度及有界应用批次，接通正式网络／主线程输入；第13步完成原生安装。
- **验证安排**：验证在第 15 步完成。

## 未决问题

- 加载画面等主线程长时间不取批次的容量与期限，现按[第9步预算](9-budget-capacity.md)执行：普通world一个等待槽100ms，可靠／joint一个等待槽（ordinary joint500ms，baseline共用原10秒episode），普通单body不等待，一个原生inflight；替换不续期。具体owner、配额接线及过期恢复动作仍须实施，不再作为参数尚未确定的问题。

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
