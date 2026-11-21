# 第10步第一项：当前合同与来源缺口

**2026-11-12 HUD来源职责修订，仅文档**：[现行合同](hud-local-presentation.md)收窄G12／G13及315等用途：低血周期／护盾资格、普通主动／饰品图标、材料／法术队列、Drop渐变、基础属性格式与多人布局由Client生成。Host仍须提供必要P3／P4／P8／结构／资源、最小critical_health和实际消费Drop的有效更新／恢复驱动；这两项新增输入及纯显示adapter尚未接线，不能把取消旧缓存当成源已完成。心格／触发相位／特殊资源／有序列表／Stat三值／概率的正常来源缺口保持；下表旧最终缓存条目仅在这些剩余用途适用。当前schema／生成表／代码未改，无生成、构建或测试。

**2026-11-11 第二项进展**：[公共采集基础设施](10-capture-foundation.md)现已实现G01—G03／G15的公共窗口、数字绑定、纯值事务与有界交接。下文保留第一项静态整理时的代码现状；各域真实来源、完整递归采集和运行桥接仍未接通，不因公共基础而改判原生UNKNOWN或来源覆盖。

2026-11-11。当前布局整数版本 **66**，目标为官方 REPENTOGON 1.1.3／v1.9.7.12.J273／Win32。上级：[Host采集器](10-host-capture.md)。

状态：本项已完成合同、代码现状和来源缺口的静态整理；**第10步采集器尚未接通**。本项不改布局／协议／代码，不生成表、不构建、不运行测试，也不新增实机操作。整个重构仍只在第15步统一整体验收。

## 1. 目标与资料边界

为第10步后续实现明确三个问题：当前必须采集什么，已有代码实际能做什么，哪些来源尚待接线或补证。这里的缺口编号只用于定位工作，不表示推进条件或新的协议字段。

当前要求先读[项目规范第17节](../../refactor/project-conventions.md#17-原生二进制状态同步重构)，再读[本地生成范围](local-media-presentation.md)、[显示批次与各组HUD合同](presentation-result-batches.md)、第9步的[正文](9-domain-body-codec.md)、[显示外层](9-presentation-envelope.md)、[资源](9-resource-protocol.md)、[接受与恢复](9-acceptance-recovery.md)、[预算](9-budget-capacity.md)和[动态压缩](9-world-body-zstd.md)。早期文档的版本57未注册、像素必发、7.5Mbps等说法保留原时点，不覆盖这些现行要求。

布局事实以[`version.json`](../../../../mod/binding/state/version.json)和其列出的14份域源为准；[同源正文计划](../../../../mod/binding/layouts/09-wire-format/body-codec-plan.md)登记349种布局、2806个字段计划、85个条件字段，最大根字段数117。该数字包含空负载Host类型和公共协议值，**不等于349种原生来源已经实现，也不等于全部原生字节已知**。本项不重新生成或执行校验来改变这些事实。

## 2. 当前必须保持的合同

| 范围 | 第10步采用的现行合同 | 后续边界／依据 |
|---|---|---|
| 支持内容 | 先覆盖原版必要内容、四参与者及一人多身体；官方扩展owner仍需正确处理 | 第三方Mod、StageAPI和自定义Lua适配归[独立后续方案](../mod-adaptation.md)，不能迁走原版必需来源 |
| 权威与输入 | Listen Server唯一玩法Authority；原版Online／Isaac Sync隔离；不等待显示、后台或Peer | 不新增模拟tick、房主输入旁路或全员等待 |
| 正式采集 | 主线程在已核窗口直接读原生内存，按生成计划复制／转换到自有缓冲；无Lua getter、JSON树、运行期分配、锁和等待 | 当前旧链仍在运行；不把旧链的存在写成新合同已经满足 |
| 字节与分派 | 只读取有资格的已知范围；RTTI选派生类、先行selector选臂；无RTTI／protocol／companion／synthetic按真实owner解释 | 不跨UNKNOWN合并、不按_type猜RTTI、不将合成偏移加到根对象、不复制原生容器头 |
| 窗口与生命周期 | ordinary完整后态与实际PlayerPost分别持证；正常返回、同use／generation／绑定、根和成员关系稳定才封存 | 规范第11节的当前读取许可不补造出生／Init历史；Delirium、Text writer等额外资格继续保留 |
| 两条序列 | world约30Hz、body约60Hz；Player父E1／E4／E5／E6、P0及19—24投影走body；P1—P8走ordinary | 不适合fast的完整身体按ordinary body-joint；不存在第三条Presentation确认序列 |
| 纯值与身份 | 原生地址只在当前主线程栈关联；引用封存Identity／Pending纯值句柄，后台再分配成员号 | 同窗新实体不因暂未编号变Null；成员离开须有生命周期／完整集合依据 |
| 来源与发布版本 | 来源修订、真实初次produced_revision、batch、显示owner代次与各Peer base／target分别管理 | GameFrame相同不能证明同来源；复用缓存不重贴来源修订；Peer版本映射归第11步 |
| 显示贡献 | 正常生产／消费点封存有序结果，保留subject/use/shape、用途、pass、分支／layer／主叠加及贡献身份 | 临时栈值、逐layer变化和递归调用不能在ordinary后态读最后值补齐；内容不变与来源仍有效分别判断 |
| 多组HUD | 各端History／Stat详细主体随本端控制组，公共多人HUD保留其他组；Host须提供全部组合法结果 | 原生History／Stat两槽及八个内嵌HUD槽不是系统容量；不得额外Recompute／Update凑齐 |
| 文字 | 422严格UTF-8、单串≤2048B；423有序fortune≤8行；Prompt固定五槽。区分确认缺席、有效空串和来源Gap | 字体0—5／样式0—6是协议自有编号，原生映射未核；EntityText145／146继续按原始字节串，不套UTF-8规则 |
| 资源与程序显示 | 八资源表、九个启用kind；实际已选本地文件／配置／动画／sample，或完整有界recipe；资源目录同run只增不改 | 禁用kind5、305／306正文及Kind24；不采集／传送像素、文件内容、PCM或视频音轨，不以压缩／blob补来源 |
| 切房／hallucination | Client合法完整目标就绪后直接切房；hallucination用本端历史与同步效果参数 | 取消Host前后surface／反馈图依赖，不为它们造Wait；Boss versus、Nightmare、死亡／结局必要语义仍保留 |
| 音频 | 正常逻辑接受且已选sample的world意图；稳定voice身份／代次、Options前最终gain／pitch／必要pan、loop／pause、必要逻辑段与连续事件水位 | 不再抽sample，不反除静音后端量；本地Play提交、设备失败／抢占／重试不等同world Start／Stop |
| 完整性与失败 | 确认隐藏／Null／空集合可以编码；未核、未生产、读失败、错owner、超额为具名Gap；资源准备中的Wait另分 | 不截断、补零或发布半记录／不完整目标；真正提交成功才ACK，详细规则归12／13步 |
| 带宽／大小 | 模组9Mbps＝1125000B/s，三远端各375000B/s；普通完整逻辑消息≤65536B，含40B信封，Frame正文≤65496B | 第9步预算含封装／重传／ACK；准备每Peer每episode≤1MiB／≤10秒。压缩不放宽解压保护、内容或时间 |
| 主线程与内存 | 采集主线程p95≤2ms、p99≤4ms；Values／Wire／Workspace共用512／128／128MiB，不按Peer重复领取 | 这里只是目标。预分配区、并存缓冲和索引均需计费；实际性能第15步验收 |

## 3. 代码中已有的能力与尚未接通的边界

以下均为本次代码阅读所得；没有新增运行时观察。

| 代码入口 | 已有事实 | 不能据此宣称完成的部分 |
|---|---|---|
| [`state/layout.h`](../../../../mod/src/state/layout.h)、[`layout.cpp`](../../../../mod/src/state/layout.cpp) | 提供Layout／Field／CopyRange、块、变体、来源、条件、choice、计数组、预算及PresentationContract等生成表 | 尚无消费这些表的完整正式Host计划执行器；handler字符串只是登记，不是已绑定函数 |
| [`gen_state_layout.py::emit`](../../../../mod/tools/gen_state_layout.py) | `state_capture.inc`实际生成`kStateCopyRanges`；全部Field操作在`state_wire.inc`，由layout.cpp统一暴露 | 不能把state_capture.inc当成包含所有Convert／Reference／Resource／Container／Nested／Choice的完整操作序列；执行器必须联合读表 |
| [`world/collector.h`](../../../../mod/src/world/collector.h)及实现 | 已有MemoryReader.copy、IdentityTable、RTTI分类、实体／roster／Grid／门枚举、ordinary阶段和PlayerPostWitness | 当前读取与旧world_layout配套，未按349布局执行；RawObject不是新的完整二进制状态 |
| [`game/observation/world_bridge.cpp`](../../../../mod/src/game/observation/world_bridge.cpp) | WorldPreparePostState先取PlayerPostWitness，再调用CaptureLuaPlayerFast，复读资格后暂存，待WorldPublishPostState随真实Clock提交 | 新采集应替换值来源并复用真实证言／提交顺序，不能把Lua快源当二进制来源，也不能提前向后台发布 |
| [`state/nested.h`](../../../../mod/src/state/nested.h)及实现 | ReadWindow、预分配Into容器／字符串读取、12B deque、CapturedReference／ReferenceCapture等基础接口已存在 | 实际ReadWindow.current、字段读取资格、身份预约／退休、map／set／aggregate及源预算仍须接正式owner；冷路径vector包装不能直接塞入热路径 |
| [`entity_extension.h`](../../../../mod/src/state/entity_extension.h)、[`npc_presentation.h`](../../../../mod/src/state/npc_presentation.h)、[`entity_counters.h`](../../../../mod/src/state/entity_counters.h)、[`owned_payload.h`](../../../../mod/src/state/owned_payload.h) | 已有Plus读取、NPC受控UI、计时换算和共享payload投影辅助 | 无法替代完整原生来源和owner资格；已从当前布局删除的计时器不因旧辅助函数仍在就恢复采集 |
| [`state/body_codec.h`](../../../../mod/src/state/body_codec.h) | BodyValueSource按数字路径提供封存标量、宽count、presence和动态Weapon槽；codec不读游戏对象 | 正式Host纯值存储及其BodyValueSource绑定未接；协议固定头用的FixedSource不等于原版游戏来源 |
| [`replica/stream_capture_worker.cpp`](../../../../mod/src/replica/stream_capture_worker.cpp)、[`stream_capture.cpp`](../../../../mod/src/replica/stream_capture.cpp) | worker已有cold／latest ordinary、8个body槽、256个有序控制及generation／watermark撤销；入口载荷仍是LiveBatch／capture::Source；Capture仍构建旧Range／JSON值 | 不能只改类型名便视为新采集完成；须更换纯字节载荷与后端消费者，保留冷请求、撤销、顺序和有界所有权 |
| [`game/observation/world_bridge.h`](../../../../mod/src/game/observation/world_bridge.h)、[`world/model.h`](../../../../mod/src/world/model.h) | WorldCapture与WorldRecording已分离；body暂存和Collector锚点为256（4×64），worker快源槽仍为8 | 正式同步不依赖Recorder；256是本地管理容量，8是快源交接容量，均不能作为原版身体支持结论。超fast容量的完整交接策略仍待落实 |
| 第9步state库与[`stream_transport.h`](../../../../mod/src/replica/stream_transport.h) | 纯值正文／显示／资源／完整Frame／接受／预算代码及独立ICSR压缩接口已有；版本66也接了旧运行链动态压缩 | 二进制握手／Kind分派、Host发布、完整影子、真实原生提交仍归11—13步；旧链能压缩不等于新采集接通 |

## 4. 读取资格的实际绑定清单

下表抄录当前域源的`read_requires`语义入口，不新建资格名称。生成字段无该字符串时，仍须遵守其布局owner、source和生命周期；尤其synthetic／protocol绝不自动取得原生可读地址。

| 生成资格 | 第10步必须提供的实际依据 | 当前边界 |
|---|---|---|
| `entity-current-window` | 受支持成员、当前use、已完成owner及未污染的实际采集窗口 | 复用Collector证言；不从RTTI或内存可读单独推导资格 |
| `current_body_field_witness_same_use_and_binding` | 同身体实际PlayerUpdate正常返回、use／绑定／代次仍一致；外层完整提交时再次复核 | 不借其他身体或旧tick证言；ordinary补采仍须满足字段资格 |
| `player-equipment-current-window` | 同一合法装备owner／当前Player、动态类型、前后资源与容器稳定 | P3—P8按ordinary，不将装备getter或Plus A8模型当直接内存来源 |
| `entity-current-window-and-confirmed-subclass-counter-clock` | 字段owner的实际增／减／停走分支及活动时钟 | 仍保留的Familiar计数／Pickup DropDelay等逐字段落实；未知不填Frozen |
| `entity-current-window-and-stable-incubus-weapon` | Incubus稳定Weapon类型／所需字段，按已核纯公式得到amount | 不调用getter、不发送／重建玩法Weapon |
| `text-current-use-writer` | 本use真实格式化writer或6FF344正常返回见证，含local/use/revision | 旧text_writes事实保留；须移出Lua／Recorder依赖，Init／复用／退休撤销 |
| `grid-current-window` | 当前Room成员、index/use、Factory／Init整链完成及稳定读取 | 官方Grid旁表模块ABI／engaged检查另有来源约束，不以optional未设冒充读失败 |
| `qualified-main-thread-stable-world-owner-and-current-use` | Room／Level具名owner、主线程、root/use、成员／配置前后稳定 | 不把Room首字当vptr；合成结果来自其实际owner |
| `qualified-current-use-and-immutable-resource-epoch` | 同use实际资源绑定及不可变资源epoch／描述 | 资源登记／实际文件来源和本地ready分别处理 |
| `active-run-stable-seeds-owner` | 当前run、Game与Seeds有界只读关系／树稳定 | 不调用Seeds构造、修改或随机getter |
| `host-final-score-cache-same-world` | 9BAC90正常生产后的最终分数，正确Game或冻结结算副本owner | 不执行Calculate补值，不把陈旧副本归给当前world |

计时、容器、引用和资源都须在同一合格窗口封存；把ReadWindow.current设为恒真不能满足上表。来源失效必须能定位域／layout／field及根身份，不能只留下一个无法归属的“partial”。

## 5. 分域来源与缺口

本表将“静态来源已知、尚未接线”与“原生来源／完整覆盖仍待补证”分开；它们都不属于已经通过的运行事实。具体字段集合继续用唯一源与[10.7交接表](10-host-capture.md#107-各域来源与采集交接清单)，不另维护手写字段白名单。

| 编号／范围 | 当前可复用的来源或代码 | 仍需完成的具体工作 |
|---|---|---|
| G01 通用执行／根来源 | Field／CopyRange／Variant／Condition／Choice／CountConstraint及现有ReadWindow、MemoryReader | 建立不可变RTTI映射、真实窗口和数字handler绑定；联合执行复制、递归、条件、容器和所有预算。protocol、companion、synthetic与普通对象分开解析；异常作废完整记录 |
| G02 自有值／交接 | Collector准备→Clock提交→发布、CaptureWorker冷／ordinary／body／control槽及撤销 | 预分配封存区／纯值索引／scratch、BodyValueSource与后台纯字节载荷；一写一读、cold钉住、在途引用的存储均计费。主线程不等后台、不将旧JSON或借用span当独立所有权 |
| G03 身份／结构／身体 | IdentityTable、Pending接口、PlayerPost、P7与participant／group空间9／10 | 同窗新引用预约和成员集合闭包；use／shape／绑定／P0-P6拓扑变化先撤旧候选；独立激活资格不因公共valid退役而消失。8快槽与256锚点的选择／ordinary joint交接不能静默漏身体 |
| G04 Entity／Sprite／资源 | [公共与动画职责](animation-resource-synchronization-review.md)、ReadEntityExtension及数字ANM2投影 | 实际文件／动画／Image／shader绑定、nullable覆盖及资源替换失效；相邻合格Copy才合并。ANM2.loaded只参与合法资源选择，不是GPU ready；不恢复颜色队列／公共计时 |
| G05 子类计时与特殊纯值 | [Familiar](familiar-synchronization-review.md)、[Pickup／Slot](pickup-slot-synchronization-review.md)、[NPC UI](npc-control-ui-evidence.md)、[Projectile](projectile-effect-residual-evidence.md) | 绑定仍保留计数的真实时钟、Incubus amount／D76、DropDelay、NPC F1C→0/2、Projectile原u8与冻结位置。Laser／Bomb／Tear等已撤计时不补时钟、不造新Gap |
| G06 容器／共享payload | Into读取、[Familiar绳索](familiar-synchronization-review.md)、[Effect臂](effect-next-five-evidence.md)、[Homing点列](laser-homing-synchronization-review.md) | 共享holder前后、具体RTTI与variant、同payload各投影；Trail裁剪后0..127个64B节点，Ghost有效0..4前缀；两组Homing各≤16384、独立count；原生宽数量先核再窄化。超限不截断、错型不折空 |
| G07 NPC／Delirium／Text／Dummy | 已有重配owner、Text writer见证、ReadCStringInto；[各自限制](text-dummy-evidence.md) | Delirium49F3D0退尽／稳定选臂与companion正常枚举；Text见证移到正式owner，Render独立样本不拼ordinary。Dummy自然来源与不可见时历史屏幕cache／跨视口表达仍未闭合，须补证并与13步draw owner协同 |
| G08 Player结构／库存／装备 | [Player正式源](implementation-5-player.md)、12B deque、动态Weapon choice／条件codec | 固定13 Sprite、costume目录与P0／P6同拓扑；P3实际map／set／官方innate聚合，P4先Item、P5真实动态类、P8仅704附cooldown。Plus后半ABI模型不能直接当官方内存可读证据；真实官方成员owner待绑定 |
| G09 Grid／官方旁表 | [17派生](6-grid.md)、[水面来源](grid-plus-water-source-evidence.md) | 正确模块RVA62E650的448×12B表、六族先engaged后payload、Lock跳表合成完整u32；独立Sprite／glitch与粒子有效分支、Rock碎块和Door目标／计时上下文接线。229只是来源ABI目录，不发原生旁表 |
| G10 Room／Level／地图事实 | [第7步实现](implementation-7-room-level.md)、[保存与探索](room-save-exploration-synchronization-review.md)、[显示设计](room-display-resource-design.md) | 448格／八门、环境颜色／water／lighting／wave／FX／rails的真实结果；L0—L3唯一owner、维度／双配置／活动集合、HasWater、Boss比较和L2 OR/max。正常缓存或已核纯投影／扫描预算与原位失效尚须接线；不读隐藏保存集合补完整 |
| G11 Run／分数 | [Run职责](run-synchronization-review.md)、Game+25A70帧头／+25A74独立TimeCounter、只读Seeds树／9BAC90分数来源 | 七种显示种子、ascent／daily、当前颜色／黑暗、64B最终分数与实际进度图标缓存；Game当前值与冻结结算owner分开。完成处理器与读取资格，不额外daily Init／Calculate |
| G12 全部组HUD／随机结果 | 下表28条合同及正常生产候选，Pill／预览／alpha已有静态点位 | 全组History／Stat／身体HUD来源覆盖仍待补证；正常缓存未接。Pill逐身体槽／实际预览、alpha临时栈、同帧多pass／递归贡献须独立封存，不额外getter／Render／随机 |
| G13 文字／程序显示／坐标 | 422／423／424、UTF-16纯值转码、固定资源／recipe协议；[正常屏幕来源](presentation-resource-screen-sources.md) | 原生wchar_t合法读取、字体profile／特殊首码→style映射、五槽Prompt实际文字尚待核定；程序背景／原版程序道具的完整本地描述、Nightmare／versus／死亡有序贡献与跨视口坐标尚需逐用途闭合。默认ColorOffset或媒体blob均不能掩盖缺口 |
| G14 音频逻辑来源 | [音频静态目录](presentation-audio-sources.md)：907C60已选sample、同次后续pan、907F60／908070调整、908140／908210停止、908340重混音、908530环境声、音乐两组／jingle | 正常world用途／退休、完整Options前writer／enabled／平滑逻辑包络、sound460最终pan及有效参数来源未全覆盖；自有voice／事件缓存未接。必要segment／seek／重连续接与视频音轨仍有UNKNOWN，13步还需非随机本地执行能力，不能从+2C更新计数伪造游标 |
| G15 生命周期／成本／退役 | 现有生命周期事件、watermark、stage12成本口径和第9步预算helper | 各源缓存失效／退出排空，低频片大小／轮转周期／冷源重读预算及域计时；不得拼不同world/use片段。来源完整后11步按变化发布；12／13步完整目标／ready／提交；14步才移除旧Lua／JSON正式链 |

对象状态复制复用ordinary／PlayerPost，不额外制造读取窗口。显示／音频的瞬态来源须在真实正常生产点捕获；**不能将10.1的对象复制约束理解成“只在ordinary就能补齐所有瞬态结果”**。现有Hook能否覆盖、若不足需哪一处具名观测点及ABI，仍须在G12—G14按用途核定；本项未安装或决定新增Hook。实施时一律经统一清单和协调owner常驻，原函数exactly-once，会话仅逻辑启停。

## 6. 28条已登记显示合同的来源索引

以下直接按当前`presentation.json::presentation_contract`整理。表内生产点是静态来源／合同候选，**全部仍需第10步实际封存和有效性接线**；“源已定位”不表示全部原版路径覆盖。字体编号、全组HUD、程序资源、逻辑音频等未闭合项沿用上表。

| 布局／用途 | 正常来源／生产点 | 必须保留的缺口或封存边界 |
|---|---|---|
| 315 PlayerHUD | 832C10完成的24心／槽／库存与动态HUD owner | 全控制组合法来源；心格隐藏不读旧尾值，非当前Host槽仍须有真实结果（G12） |
| 317 StatHUDPlayer | 83CFD0／83D210最终行三值与资格 | 全控制组及双身体来源，0—8具名行；不发previous／threshold（G12） |
| 318 StatHUD | 83C9B0正常消费 | 共享资源／组合语义，不将HUD.stats ANM2误当StatHUD（G12） |
| 321 HistoryHUDPlayer | 82FA10完成的有序实际图像列表 | 全控制组、完整i32 visual_id与顺序；不从库存重排（G12） |
| 323 HUD | 97AE90及正常消息／Boss消费 | 11消息、fortune、全局资源；实际文字与样式来源替代旧文字图（G12／G13） |
| 325 Minimap | 9635B0焦点／资源、966300徽标、966680震动／淡出 | 300—303与L0—L3唯一来源；169无合法描述为Gap，Host展开偏好不定义world（G10／G12） |
| 356 PresentationDrawClock | 正常Manager／Game同批相位 | D0仅ManagerFrame／paused共5B；GameFrame在公共帧头，不发随机seed（G12） |
| 357 PickupRandomAlphaResult | 6E3805写+684后、alt draw前 | 每次实际资格／alpha及pass／递归贡献，不取后态最后值（G12） |
| 358 FamiliarRandomAlphaResult | 61E604产生栈alpha后、61E60D前 | 栈临时值即刻封存；未产生不等于state0（G12） |
| 359 PillDisplayResult | 8378C0→83830A／838320实际身体相关显示 | 身体／Pocket／已揭示分支与真实图像；不执行GetPillColor／Effect补值（G12） |
| 361 PickupPreviewResult | 正常Guppy Eye／具名预览消费 | 全部预览来源覆盖待核；256是保护上限，不是截断许可（G12） |
| 362 PlayerDrawImageBindings | Player+1FF8／+2000正常实际owner | 真实图像与资源代次，不能按当前配置猜图（G04／G12） |
| 364 ControlGroupHudSelection | 正常角色关系选择或已核C/S纯值投影 | 全部组History／Stat来源与必要关联健康显示；选择规则不创造结果（G03／G12） |
| 367 ScreenBloomResult | 6F8589—6F8645最终强度／已核f32纯投影 | 非法T不除零；Host画质关闭不自动视为world关闭（G13） |
| 368 ScreenPixelationResult | 6F86E4—6F8820最终强度 | 当前有效Room／Stage结果，不在Client重演转场FSM（G13） |
| 369 ScreenRetrovisionResult | 80454F—8045EE本次局部强度 | BE7704是临时输出，不是全局永久来源（G13） |
| 370 ScreenHallucinationResult | 6F89DE—6F8C2A正常效果消费 | 只封存有效性／amount；不传Host历史图；13步补本端缓存leaf（G13） |
| 371 RoomGlitchResult | 8045F6—804612本次Room分支 | 开关与Sprite各自最终选择同批，不按地址或本端RNG推导（G13） |
| 373 ScreenEffectsResult | 6F7A80各具名pass及6F8FFD／6F90B5 | 424本地历史颜色叠加与独立fade有序，主／次最终选择不重复；无Host snapshot依赖（G13） |
| 374 BossVersusResult | 正常versus肖像／逐layer Sprite消费 | 临时变换完成、提交前逐项封存；不包含已取消的普通切房surface（G13） |
| 375 NightmareSceneResult | 8FAC20各正常Connector／Levels／slot／程序道具提交 | 完整贡献／本地资源描述／逻辑坐标覆盖仍待核，不再重复发路线／Scene生产体（G13） |
| 376 GameOverResult | 970C10实际死亡subject／有序项目／击杀者 | 全用途、资源和坐标闭包待接；不额外Show／更新死亡流程（G13） |
| 377 StageTransitionResult | 909E60最终遮罩 | 换层必要内容保留；不因普通切房动画取消而删去（G13） |
| 378 ItemOverlayResult | 984660正常Render当前结果 | 只封存正常分支及真实资源／姿态，不执行Show重抽（G13） |
| 379 BossOverlayResult | 41CD30选择完成、41CC80消费 | 当前存在性／Sprite；不放行41CB40的进度／音乐／ChangeRoom路径（G13） |
| 380 GenericPromptResult | 正常Prompt纸张／文字消费 | 五槽真实文字／字体／样式待核；个人菜单不混入world（G13） |
| 382 AudioVoiceState | 907C60节流后选sample；音乐Options前正常结果 | 完整逻辑gain／用途退休／必要segment与恢复缺口仍在（G14） |
| 383 AudioWorldEvent | 正常world资格通过PRE／节流后的选样本及控制用途 | 第9步事件封装／Clear范围已定；本步仍须区分逻辑接受、本地提交、重试与真实退休（G14） |

365／366贡献元素、381参数、422／423文字、424历史叠加等由上述具名owner嵌套持有；它们不是缺少独立第29条合同。Random的24个新增视图没有发送字段；必要随机显示结果已经由356—361等具名用途承接，不重新上线RNG游标。

## 7. 后续实施顺序与跨步骤交接

1. 先落实G01—G03的窗口／资格绑定、数字计划执行、预分配封存与纯值交接骨架。完整根在同一提交点发布，失败不留下部分记录；保留身份及生命周期观测，不要求Recorder开启。
2. 按G04—G11逐域接原生直接字段、转换、容器／共享owner与资源描述，同时对仍未核来源补静态证据；先查本地Wiki，再查官方SDK／源码，缺项才静态逆向，不为证明来源运行程序。
3. 按第6节接G12—G14正常生产缓存；全组HUD、文字映射、程序资源／坐标和音频逻辑覆盖须实际补齐，不能把只有协议值的类型填成最短空样本。G15的失效、预算和计量随各域一起落实。
4. 第10步交付自有完整来源、读取资格及失败归属；第11步负责memcmp变化版本、逐Peer组帧／预算／实际发送账本；第12步负责完整影子及有界候选；第13步负责本地资源ready、显示／音频执行与原生共同提交；第14步退役旧Lua／JSON链。第15步才运行正确性、原版覆盖及性能验收。

既有密集设计负载仍有超预算项；版本66压缩只提供可用机制，没有测得压缩率。低频轮转不能混合不相容来源或拖延必要结构更新，不能靠减少必需对象／结果、暗降30／60Hz或提前ACK完成预算。本项不对尚未实现的运行链声称达标。

## 8. 失败方式、改动范围与完成情况

本项重点防止：沿用旧版本的待注册结论；将生成字段／协议接口当成真实采集；只执行CopyRange漏掉其余操作；将原生地址／旧JSON跨线程交接；用其他身体或最后一次绘制结果补来源；把取消的Host媒体继续列为必需依赖；用合法空值掩盖必要Gap；用表项／解码成功代替本地ready或原生提交。

改动仅为本文、[第10步](10-host-capture.md)的现行状态／交接说明及两个文档入口。不改任何原生字节归属、历史事实／失败／UNKNOWN、布局版本、协议值或运行代码，不执行旧gate／哈希流程。

已静态对照版本源、生成器与表模型、Collector／桥／worker及第8—9步现行合同，修正第10步仍将HUD／音频布局列为未注册的旧状态，并明确复制区间表与完整执行器、8快槽与256锚点的区别。**第一项整理完成；G01—G15及第10步其余实现仍待推进，第15步验收尚未开始。**
