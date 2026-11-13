# 11：Host 变化检测、增量与发布

**2026-11-13 已实施接线，布局74**：[实际代码与边界](implementation-11-worker-peer-publish.md)。已接worker票据消费、逐块变化修订、逐Peer组帧、成员／资源表先行、事件队列、实际发送ledger及AppliedAck回收；当前使用变化修订的完整块扫描，不依赖变化日志环。Client正式接收安装仍待第12／13步；没有运行测试。下文“仅文档／待实施”等保留原计划时点，现行实施状态以本条及链接为准。

**2026-11-12 显示委托修订**：

- 变化检测、增量与发布机制保留。
- 去掉显示结果批次。
- 新增事件队列的序号、限额与确认。
- 空帧继续发送，用于推进 Client 帧号（方案B）。
- 见[修改方案](client-display-delegation.md)第 2.3、7 节。

本次只修订文档，未改代码，未运行测试。

**2026-11-12 HUD代码迁移，版本70**：布局70将435显示时钟与315必要结果分开；发布器须分别变化检测／引用缓存，不能因本地闪烁、渐变或排版重发315。真实发布／冷连接装配仍待本步接线。 具体接口、事实及剩余接线见[实施说明](hud-local-implementation.md)。第15步唯一统一验收，未运行测试。

**2026-11-12 HUD本地化发布交接，仅文档**：按[现行合同](hud-local-presentation.md)发布必要状态、实际消费Drop及有效更新／恢复锚点、剩余特殊结果；不因Client本地图标、低血闪烁、Drop渐变、材料绘制、属性格式或排版变化而重发整份HUD。普通图标／中心、护盾资格与低血强度的重复结果须先随第9步schema迁移再退出发布，不能在现有版本直接缺字段。特殊资源、三色／充能闪烁、有序列表、Stat变化相位／概率仍按原目标与依赖发布；全部实际带宽继续计费。完整运行接线尚待实施，本次无代码／生成／构建或测试。

**2026-11-11 版本66压缩交接**：[world/body独立zstd](9-world-body-zstd.md)已接既有异步编码／发送分支和协商route，同时提供完整ICSR纯字节接口供本步接二进制Frame。实际压后长度才计费，普通动态仍latest-only；增量／缓存／来源及9Mbps额度保持。

**2026-11-11 第9步收尾交接，版本65**：[现行预算](9-budget-capacity.md)定模组9Mbps／三端各375000B/s，日常world/body各150000、control25000、reliable50000B/s；仅借本Peer恢复停用流。此步接最终数据报计费、真实峰值／截止、192项账本／64MiB、事件500ms、三级桶、子owner配额与大型工作串行化。已知密集设计负载超额须如实处理，不暗降频／漏内容。

**2026-11-11 第9步第五项交接，版本64**：[接受与恢复合同](9-acceptance-recovery.md)已提供纯值发送账本／ACK核对与恢复计划。此步接线时须预留账本／租约后发送，仅登记实际发出的完整目标，严格匹配原始joint确认；恢复期间不发受影响序列增量，旧ACK不解除恢复，重复Resync不抬高恢复起点。普通恢复target继续前进，不回收同run资源编号。

**2026-11-11 第9步第二项迁移，版本61**：[正文合同与新布局](9-domain-body-codec.md)已登记422文字／423有序行／424本地历史颜色描述，替换文字图引用、移除Host反馈与切房surface，374收窄为独立Boss versus；305／306正文禁用、304拒绝kind5。字体／特殊首码映射与真实封存、本地缓存、完整资源接受及原生显示仍由10—13步落实；取消的Host图像不形成Wait，真正必要依赖不早ACK。公共头及带宽预算保持。
**2026-11-11 当前发布职责调整**：[本地生成合同](local-media-presentation.md)：发布文字变化、必要样式、本地资源引用／有限描述及音频事件／状态，当前不调度原始图像／音频blob。Client取消平滑切房及Host反馈图依赖；hallucination只发必要world效果参数。world/body配对、真实资源准备、事件水位和所有通道带宽计费保持；旧blob必发段按历史设计阅读，后续需迁移源和发布消费者。

**2026-11-11 第9步各域正文实现，版本60**：[正文实现与边界](9-domain-body-codec.md)、[同源数字计划](layouts/09-wire-format/body-codec-plan.md)。EncodeBodyInto／EncodeRecordInto处理变化块，EncodeValueInto处理具名结果；原生换算、canonical排序先在自有状态完成。reliable承运与world/body确认分开，ordinary body-joint不套fast专用body_limit。所有调用还须传整帧剩余预算并逐三端实际计费；完整Frame／表／资源调度未接通。

**2026-11-11 第9步公共合同，版本59**：[正式结构](9-public-wire-contract.md)。采用公共上下文50B、Frame壳2B及Section52B；world/body按Peer独立确认，双分段共同提交承接ordinary body-joint。Host来源映射逐Peer目标，旧目标Resync不重置计数，未确认音频事件保留至world水位ACK。普通动态64KiB／旧可靠32MiB是保护上限，三端总10Mbps／模组7.5Mbps、首次准备1MiB／10秒与实际封装／重传计费同时约束；运行调度未实现。

**第8步版本58交接**：[具名结果与28条窄依赖合同](implementation-8-presentation.md)已纳入唯一源。源修订映射各Peer目标，按world／body所属目标与必要资源共同提交；全部控制组显示选择不改变输入归属，缺必要结果不发布不完整目标。外层封装继续按第9步落实，不新增Presentation确认序列。

**2026-11-10 显示批次发布定稿交接**：[方案B](presentation-result-batches.md)继续使用world／body两序列。发布端将Host来源修订映射到各Peer目标版本，按owner／有效依赖共享不可变结果，保持明确贡献身份及顺序；各端详细HUD默认跟随自身控制组，公共多人HUD及各组必要结果完整。分组不授权部分提交不完整目标，body不扩大为等待完整地图；必要Gap／资源Wait不发布成空值或确认已安装，旧结果不换来源标签，Authority不等待。具体组帧／预算仍待实施，版本57不变，无构建或测试。

状态：设计稿，未实施。上级：[总计划](README.md)。

**2026-11-10 各域交接统一整理**：现行入口为11.7“各域变化与发布交接清单”，按来源、特殊约束、版本依赖与待落实项登记全部相关域。本次静态审阅的布局源为57；布局入源、运行接线和第15步验收分别说明。本次只改本文档，不修改实现／布局源／生成表，不构建、不运行测试。

## 目标

后台线程把主线程交来的缓冲区转成以下内容：

- 成员表变化；
- 资源表变化；
- 按块记录的变化版本；
- 每个 Peer 的状态帧。

Baseline、增量和 Resync 使用同一套机制。

## 输入事实

- **保留的身份语义**：`IdentityTable` 与 `Registry` 的 key／use／shape 语义保留（总计划第 6 节）。
- **共享编码**：现有 `Blob` 让多个 Peer 共用同一份逻辑正文（`stream_transport.h:28-41`）。
- **网络预算**：默认值和上限来自 `stream_config.h:11-12`：
  - world 每秒 150000 字节，上限 1125000；
  - player 每秒 150000 字节，上限 1125000；
  - reliable 每秒 50000 字节，上限 1125000。
- **首次准备**：不超过 1 MiB，从发起到画面就绪不超过 10 s。
- **现有合同中被取代的部分**：“永不隐式继承旧动态字段”和 Field／Atom／Layout 范围模型，在本步由“显式继承已确认版本”取代。

## 设计

### 11.1 Host 端的数据结构

| 结构 | 内容 |
| --- | --- |
| 成员表 | 成员序号 → {身份, 布局编号, 加入 epoch}，并维护本地身份句柄 → 成员序号的映射 |
| 块存储 | 每个成员、每个块各一份：上次的字节（按状态布局）、长度、变化版本 |
| 变化日志 | 环形队列，记录最近 N 个版本各自变化了哪些（成员, 块） |
| Peer 状态 | 每个 Peer 分别记录：world 已确认版本、body 已确认版本、已确认的成员表 epoch、已确认的资源表 epoch |

块存储大约等于全部状态的大小，估计在 1 MB 量级，归入现有内存池。

### 11.2 每次采集的处理

1. **成员**：把缓冲区里的身份与成员表比对。
   - 新身份：分配序号，写一条“加入”表项。
   - shape 变化：写“形态变化”表项，该成员全部块标为已变化。shape 由当前布局全部标了 `shape` 的字段共同决定，不写死 type、variant；标记必须有实际Client结构依据。版本42的Delirium BossType／Variant不上线、不触发shape，资源变化只更新实际改变的块。
   - 消失：写“离开”表项。
   - 有表项变化时，成员表 epoch 递增。
2. **引用**：把缓冲区里的身份句柄转成成员序号。目标仍是“待登记”的，会在本步内一起登记；登记不了的报缺口。
3. **比较**：合法封存后分配不回绕的Host来源修订，逐块memcmp；来源先于任何Peer目标存在，不把全局来源号当Peer的world版本。
   - 字节不同：覆盖块存储，变化标记关联本次真实封存来源，写入变化日志；发布时再按各Peer的base／target映射。
   - 字节相同：不做任何事。
4. **不完整采集**：主线程标为不完整的采集不推进发布版本，不据缺读推断离开，也不得覆盖用于变化判断／ACK继承的已发布块或成员集合。若暂存其中完整片段，须与已发布基线分开，待合法完整目标重新比较；具体缓存策略在本步实现，不让未发布字节被下一次memcmp误判为已经发送。

body来源用同样的独立块存储／比较流程，只含body块；同窗口world／body共享真实来源身份，各Peer两条target确认序列仍独立，不由来源号直接代替。

### 11.3 组帧

- **收集范围**：已确认版本为 A 的 Peer，它的帧包含所有变化版本大于 A 的块，以及这些块所属的成员记录。
  - 变化日志能覆盖 A 时，取 A+1 到当前版本各日志的并集。
  - 覆盖不了时，扫描全部块存储（几千个块，后台可以承受）。
- **共享编码**：只有序列范围、base/target、成员／资源epoch、布局及专用依赖均相同的Peer才共享正文／压缩结果；仅已确认版本号相同不够。
- **空帧**：没有变化块时，也要发送 record_count=0 的帧，当前版本公共正文至少126B（50B公共头＋2B Frame壳＋52B单分段头＋22B事件队列头），另有40B信封和实际分片／合包开销。
  - Client 用帧头的 game_frame 推进展示时钟（第 13 步 13.6）。这样在静止场景中，FrameCount、Timeout 等由帧锚点还原的值也能正确前进。
  - 空帧同样参与确认，用来推进已确认版本。
- **Baseline**：A=0 的帧就是 Baseline，包含全部现存成员在该序列的全部必要非空块及合法结果；两序列首次准备共同提交。
  - 首次准备时，在 `Baseline` 可靠通道上发送“成员表全量＋资源表全量＋world/body共同提交的base=0帧”。
  - Resync 时，保留Host目标计数的单调性，该Peer恢复帧以base=0完整重建；不能在同路由把target重置为0。
- **表先于帧**：帧引用的成员表 epoch 和资源表 epoch 先经 `Context` 通道可靠发送。Client 安装表之前，按第 9 步的规则暂存或丢弃帧。

### 11.4 确认与自愈

- **确认内容**：Client 原生写入完成后，`AppliedAck` 带回 world 版本、body 版本、成员表 epoch、资源表 epoch。Host 只把已确认版本向前推进，不回退。
- **丢帧**：普通动态帧不逐帧重传，下一帧仍从旧确认版本带齐变化块；未确认音频事件也须保留并重复带齐，不能随latest-only候选淘汰。可靠结构／Baseline按原通道重传。
- **变了又变回**：判断依据是变化版本，不是与快照比较，所以块“变了又变回”也不会被漏发。

### 11.5 带宽与限速

- **过大的帧**：帧大小超过当前通道的可用预算时，不把帧拆成多个半帧，因为帧必须整帧接受。可以推迟发送，并合并到下一帧。下一帧会自然包含更多块。
- **长期落后**：如果某个 Peer 长时间落后，帧大小的上限就是 Baseline 的大小。这时直接按 Baseline 处理，并在诊断中记录。
- **body 帧优先**：body 帧走 `PlayerFast` 通道，优先于 world 帧，沿用现有的通道优先级。

### 11.6 序号回收

- **成员序号**：成员离开后，等所有 Peer 已确认的成员表 epoch 都越过这次离开，才放回空闲池。
- **资源序号**：同一 run 内资源序号不回收。

### 11.7 各域变化与发布交接清单

来源与规范字节以[第9步9.10](9-wire-format.md)、[第10步10.7](10-host-capture.md)及版本57的唯一源为准。比较对象是已合法取得的自有投影／内容，不是含指针、UNKNOWN、容器头或Host生产历史的原生对象。以下是发布器尚待落实的合同；未注册结果仍须先按第9步登记，不能按草稿直接发包。

| 域与来源 | 本步特殊约束 | 身份／版本依赖 | 待落实项 |
|---|---|---|---|
| 公共封装／身份／容器：[第9步](9-wire-format.md)、本页11.1—11.6 | 按生成物理块及规范长度memcmp，空预留不造零长字段；引用先完整登记再转号，计数／choice／条件变长不按固定原生尺寸比较。不完整源不污染已发布基线或触发假离开 | key/use/shape、member/resource epoch与各序列base/target分别维护；真实结构字段决定shape | 变化日志／块存储／Pending关联、完整目标缓存与按Peer发布接线，日志溢出走全量块扫描 |
| Entity公共／Sprite／Plus：[公共职责](entity-common-synchronization-review.md)、[资源复核](animation-resource-synchronization-review.md) | 分开比较外观／动画／图层，保留最终完整颜色及nullable；事件位、loaded、valid、ColorParams及已裁计时不产生更新 | 实际资源／引用和目标成员匹配；exists/dead变化不同于成员离开，资源变化不自动升级shape | 生成块存储与成员退休／重建关联，不因旧原值检查恢复被裁字段 |
| Familiar：[typed结果](familiar-synchronization-review.md) | amount按最终整数，240绳索Sprite／节点完整投影，蝗虫／IsaacsBody按实际有序内容比较；轨道生产历史不重发 | payload存在性／投影count、真实DAC、资源和目标版本一致 | 多投影变化合并与计数组闭包；缺源不当空、不能只看count或指针稳定 |
| Knife／Bomb／Tear／Pickup／Slot：[三类](knife-bomb-tear-synchronization-review.md)、[拾取／机器](pickup-slot-synchronization-review.md) | 发布最终姿态／costume／价格Sprite／奖品Sprite、Knife拖影及Price／DropDelay等保留输入；P4空槽等规范化之外不比较被裁攻击／循环／开奖历史 | 辅助Sprite／真实引用同目标；Price与图像、计时与该world时钟保持关系 | 每物理投影独立变化；普通数值／资源变化不无据变shape，不靠未来循环或奖励输入重建图像 |
| Laser／Projectile：[最终几何](laser-homing-synchronization-review.md)、[PR2](projectile-effect-residual-evidence.md) | 两Laser点列独立变化，空→非空及缩短都按实际字节；Projectile Height及原始u8／冻结Vector保留，采样／轨迹生产输入不上线 | 点列／标量／资源同目标，PR2锚点不能由当前位置替代 | 完整几何／空集合发布与跨块关联，不因不同点数或未变化Height漏掉PR2 |
| Effect：[当前分块／计时](effect-next-five-evidence.md) | FX0/1按16/13B；A=F+T变化才决定锚点重发，静止位置不证明锚点不变。Trail0..127、Ghost0..4及typed存在性按实际全序比较 | 新world帧更新展示时钟；Parent、payload／资源与目标一致，空Trail不同于缺失 | 正式全部臂及多投影闭包，预留块与已裁请求不发；非线性Timeout通过实际锚点变化承接 |
| NPC／Delirium／Text／Dummy：[NPC](npc-control-ui-evidence.md)、[Delirium](delirium-residual-evidence.md)、[Text](text-dummy-evidence.md) | 0/2 UI、双Sprite／三deque、Text原始字节与counter按当前布局比较。Delirium Boss AI／BE8不加shape；独立companion仍正常入表 | 稳定RTTI／公共shape及当前use；Text当前writer、Dummy必要历史显示源独立 | 真实结构与资源变化分流，不能用同数值type代替RTTI或把未知显示缺口发为默认 |
| Player body与P6/P7：[正式Player](implementation-5-player.md) | 10及19—24各物理块独立变化；普通动画不能重发全部13份外观。P6比较有序目录／资源结构与跨身体借用实际内容，P7组／激活是结构数据 | P6/P7及对应P0拓扑先ordinary提交，body只用相等render_topology_revision；participant/group与多身体分开 | 旧fast候选撤销、ordinary body-joint、按Peer body序列／ACK范围与64KiB超限路径；不等待全员 |
| Player P1—P8其余：[P3—P5](5-player-p3-p5.md)、[P6—P8](5-player-p6-p8.md) | 有效innate规范聚合，无序map按声明排序；P4空槽旧值、P5非展示位／恒零充能、非704 cooldown及原始History不触发变化；残影／704真实变化仍发 | world物理11—18与装备／资源／拓扑依赖一致，普通充能不升级shape | 聚合规范字节／条件记录变化和各预算闭包，不按全原生backing做memcmp |
| Grid公共／17派生：[第6步](6-grid.md)、[水面来源](grid-plus-water-source-evidence.md) | 完整u32 water、glitch两位和按nested_layout的资源／条件Sprite、Fire有效粒子、Rock有序碎块分别比较；Collision／原始optional／旧残留不上线 | Room色／几何、Door目标／Light、实例use与资源一致；同index别名不是同一C/S对象 | 规范派生／条件值与跨域版本关联；源变化即失效，不沿旧仅默认Sprite或water≤255假设 |
| Room／背景／FX：[版本57](implementation-7-room-level.md)、[显示设计](room-display-resource-design.md) | 239物理块及实际绑定／pose／ray／vein／rail完整结果比较；最终water/lighting/wave/focus，flow只一份。权重／RNG／生产队列或Host视口变化不发布 | 同world几何、八门、内容资源及显示修订；结果外层pass／调用身份不能只靠GameFrame | 完整可显示目录和结果批次的变化／失效调度，旧room退役后不继续发布旧图像／FX |
| Level／Descriptor／保存与探索：[版本57](implementation-7-room-level.md)、[两域](room-save-exploration-synchronization-review.md) | L0/L1/L3比较真实窄投影／映射，L2比较最终OR/max mask／帧；保存重复数和顺序不改变聚合时不重发，原位结果变化仍需发现。无关flags／全保存体不比较 | 有序活动描述、物理源槽／代次、双配置、地图与资源同world；探索可回退，不要求单调增长 | 低频轮转失效／完整重读和结构事务闭包；不能只看visited/clear/count或通知覆盖 |
| Run：[Run职责](run-synchronization-review.md) | TimeCounter基准而非每帧原计数；GameFrame帧头唯一更新，current颜色、黑暗、最终分数及窄bool按实际结果比较；生成历史不发 | 当前Game与冻结结算owner分别匹配显示修订；空帧仍推进已应用时钟 | 132B各块变化、最终分数／资格结果与UI批次关联，不能把基准不变误作时钟可停 |
| HUD／地图／UI：[Presentation](presentation-synchronization-review.md)、[地图设计](room-display-resource-design.md) | 最终心格、Stat三值、有序History／实际图像、消息／转场pass；MapFocusPair、徽标顺序／问号图与world限制保留，Host展开／Options不发 | 身体归组、L0—L3、资源、地图／显示修订同目标；本地视口不能裁掉全目录 | 未注册结果的正式组帧／限速与不可变批次；持续UI不得靠重播Show命令恢复 |
| D0／随机结果：[逐用途交接](room-random-handoff.md) | D0=5B，alpha按已确认用途／调用身份发布；两个seed／全局流不上线，不能用本地顺序或最后值合并重复调用 | world/presentation、subject/use/shape、pass／递归及资源依赖齐备；缺批次不是state0 | 具名结果注册、批次替换／退休与必要源缺口处理；Authority tick不等待结果或ACK |
| 配置／图像／shader资源：[本地生成合同](local-media-presentation.md)、9.6 | 声明本地文件／配置／有限生成描述，变化时分配不可变号；文字按内容／样式变化发送。不发送原始图像／音频，当前不调度媒体blob；固定头304—309仍是既有源事实 | 资源epoch、本地准备与目标world／body对应；删除取消用途的Host前后surface／历史图依赖，不按同名猜实际绑定 | 完整kind允许集、文字／描述调度、耗尽恢复及资源寿命；全部有效声明／recipe／状态／重传计首次1MiB/10秒及持续9Mbps |
| Music／SFX／环境声：[音频职责](presentation-synchronization-review.md)、[实际sample](room-random-handoff.md) | 事件发布已选sample且去重，持续voice／crossfade/jingle状态供Baseline恢复；不同voice不能按soundID合并，重复状态不重播。Host音量／节流游标不发 | sample资源、voice代次／相位、场景和开始停止clear关系明确；事件不能直接套latest-only状态丢弃语义 | 第9步事件／状态封装与本步可靠调度、生命周期和预算统计，不用“仅currentId／仅事件”代替完整声音 |

共享编码必须匹配完整依赖键；目标过大可以延后并合并，不能截断成员／条件投影后ACK。Baseline=base0与增量复用同一规范字节，保留真实空帧及最后变化后静止的收敛。变化日志长度、轮转周期、长期落后恢复阈值和全通道计费仍待本步落实，第15步统一验收；本次不运行这些路径。

## 改动范围

- CaptureWorker 的后台处理改为本步的流程。
- 删除旧的分区、身份投影和范围组装路径：`stream_partition.cpp`、`stream_capture.cpp` 中的树构建（第 14 步）。
- `AppliedAck` 的确认量改为本步的四个值。

## 失败方式

- **成员与帧错位**：成员表 epoch 递增了，新成员的帧却先到达；或者表项丢失，导致 Client 把块写到错的成员上。
- **已确认版本回退**：已确认版本被旧 ACK 覆盖而回退，导致重复发送大帧；或者被推进得过快，导致漏块。
- **日志覆盖不全**：变化日志环溢出后，没有改用全量扫描。
- **形态变化漏标**：形态变化后没有把全部块标为变化，新形态的块不完整。
- **形态字段没有结构依据**：漏比较已有实际依据的shape字段，或把Host AI／资源选择器强加为shape，都会错误处理成员。Delirium BossType／Variant自版本42排除，不能借旧例子强制重建NPC。
- **body 序列混用**：body 和 world 序列共享版本号或块存储，相互干扰。
- **静止时不发帧**：没有变化块时不发送空帧，Client 的展示时钟停住，由帧锚点还原的值随之冻结。
- **Baseline 超限**：Baseline 超过 1 MiB 或 10 s。需要按第 15 步实测定位到具体布局。
- **共享编码混用**：为确认版本相同的 Peer 共用编码时，误用了另一 Peer 的成员表 epoch。

## 完成内容

本次仅完成文档整理与静态审阅；以下为本步骤后续实施／验收需要交付的内容，不表示已经实现、编译或通过测试。

- **实现交付**：需完成后台变化处理、完整目标缓存、组帧与确认恢复，并在实施阶段完成相应纯产品编译；本次未实现或构建。
- **验证安排**：Baseline、增量、丢包自愈、Resync，都由第 15 步的 E2E 覆盖。

## 未决问题

- 变化日志环的长度 N。
- 长期落后时转为 Baseline 的门限。这里的门限只是一个运行参数，不是推进 gate。

## 历史修订提示与原分域补充

以下保留原页首与原正文中的修订补充，按各自时点阅读；旧版本号、未生成／待注册和原位置措辞不覆盖上文现行清单。版本47的Player和版本57的Room／Level已经入源，完整消息／结果批次及运行owner仍按各项实际状态说明；历史失败、UNKNOWN及尚未观察的事实不改判。

**原页首提示**

**2026-11-10 第7步显示／资源发布合同（待实施）**：按[补齐设计](room-display-resource-design.md#7-批次变化与安全应用)只比较自有规范投影／内容；配置权重、Spawn／Merged、背景RNG、Host地图查看和其它不下行输入变化不发布。配置15B／实际内容变更生成新不可变资源号，先传依赖闭包与完整blob，再发引用它的目标world／显示批次；不能在旧号改像素或按图像同名复用。程序资源和recipe／压缩blob纳入首次准备及三端总带宽，不扩大原1MiB／10秒／7.5Mbps目标。Authority不等待显示／资源／全员ACK；实际source与调度待实施，无测试。

**版本47布局来源（2026-11-09）**：[第5步正式源／生成](implementation-5-player.md)已完成，EntityPlayer=197，participant／control-group=195／196，Player主动槽=179，条件效果=180，动态Weapon=181—194；物理0—24及条件／固定槽／预算元数据以唯一源和生成表为准。以下版本46／草稿／待生成表述保留原时点；本步骤的运行接线仍未实施，不能因布局已生成就提前接收、采集、安装或宣称验收通过。

**第5步P6—P8发布合同（2026-11-09，未实现）**：[后三块定稿](5-player-p6-p8.md)在world物理16／17／18分别memcmp规范投影。costume生产元数据、map priority、非704 cooldown、原始History／索引／forms和未选回放节点不参与变化；实际残影duration／位置和704 cooldown变化仍会重发。P6 revision统一覆盖Player的跨身体目录／资源结构／P7借用变化，与P7和同拓扑P0先ordinary提交，再接受相等revision的body；不把普通计数或充能变化升级shape。新增纯C/S组／participant记录按成员表关联，roster／组／激活不同于普通Entity热状态；sub／replaced／703四槽不产生Client引用闭包。当前46未实现这些新投影，没有测试。

**第5步P3—P5变化合同（2026-11-09，未实现）**：[P3—P5设计](5-player-p3-p5.md)的world物理13／14／15分别比较／版本。无序容器规范排序，innate合并有效计数后再memcmp，不因timer／group管理变化重发；P4空槽旧payload与P5非展示Modifiers位／恒零充能输入不参与变化。装备结构／资源选择变化先随ordinary的P6拓扑提交，普通充能变化不升级shape；不将被裁字段的旧读回要求放回生成布局。实际三端／多身体带宽仍在第15步验收，版本仍46。

**第5步快源拆块补充（2026-11-09，待实现）**：[P0—P2设计](5-player-p0-p2.md)中的P0是逻辑组，10及19—24号物理块各自memcmp／版本变化；普通动画变化不能重发13份ANM2的整个外观。Player父E6与E1／E4／E5同body目标；P1／P2仍world。P6拓扑变更先经ordinary发布，P0携带相等的render_topology_revision，不以原生地址稳定或哈希替代内容比较。全量下界不当作逐帧带宽，最终仍在第15步按三端扇出与多身体验收。布局版本仍46，无Player发布实现或测试。
