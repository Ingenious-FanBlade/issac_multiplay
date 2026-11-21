# Host 原生采集性能优化方案

**2026-11-17 最终一致修订（E10／E11 已同意，尚未实施）**：[设计文档](eventual-consistency-revision.md#36-host-采集配套)、[规范第17节](../../refactor/project-conventions.md#17-原生二进制状态同步重构)。第 14 节 J 的“第二流或最终复核失败即整个普通候选作废”改为只作废该 Player 根，奇数帧 Body 发现新结构不再走 `NeedsOrdinaryJoint`，改为省掉该成员；K 保留并改名以区别于线上 joint。普通采集改为按根隔离失败并新增“沿用”标记。本文其余内容按时点保留。

**2026-11-15 热路径实施补充**：[第14节](#14-source_native与实体body热路径实施方案2026-11-15)规定拓扑批次读取、绑定计划与组校验、Player联合来源、Body批量克隆的实施顺序和失败处理。G—K 的源码落点见第 14.10—14.13 节。第1—13节中的布局77、暂停及待测表述保留原时点。运行进度以[第15步记录](15-e2e-acceptance.md)为准。

日期：2026-11-14（Asia/Singapore）。适用工作区：`F:\projects_isaac\issac_multiplay`，当前布局版本77。A—F 的工作区实现与未决项见[实施记录](#13-实施记录)。

本方案补充[第10步 Host 采集](10-host-capture.md)，涉及[第11步后台发布](implementation-11-worker-peer-publish.md)和[第14步旧路径退役](14-retire-old-path.md)。优化属于既有二进制重构目标。实机仍沿用[第15步](15-e2e-acceptance.md)的统一整体验收；现已进入第15步持续修复与续测，以下原设计时点说明保留。

## 1. 目标、范围与事实边界

目标是在不缩减现行 `demand.json`、不降低 ordinary／合格 body 采集频率、不改变线上状态语义的前提下，将主线程采集成本恢复到既定目标：每帧 p95≤2ms、p99≤4ms，Host≥50FPS。普通世界约30Hz、合格身体约60Hz的既有时序保持；频率按真实原生窗口统计，不能用墙钟补造 tick。

- 用户报告 Host 只有十多帧，并已将问题定位到原生采集。这是本方案的问题输入，不是本轮重新测得的结果。
- 当前源码确认存在高频小读取、重复拓扑／资源解析、多层复核、旧原生批次与新采集并行、主线程条目整理等工作。它们的真实耗时排名和占比尚未测定。
- 已有 `FindLayout` 直接索引、`CapturePlan::Format` 缓存和 writer 父路径前缀缓存继续保留，不能把这些已做的优化列为尚未实现。
- 当前 Client 画面／进程退出问题仍是第15步的独立未决项；采集优化不能将其标为已修复。
- 本方案不改变协议字段、成员身份、body 对 world 的依赖、事件水位、资源接受与 AppliedAck 语义。只有内部采集计划或交接表示变化时，不为此单独提升线上布局版本。
- 不新增 gate、不校验哈希、不要求独立智能体。实现中的核心函数、数据结构和缓存契约使用中文注释。所有运行验证集中在第15步，不在各工作项之间执行测试，也不补写实现镜像式单元测试。

## 2. isaac-lan 对照与采用的原则

对照对象是本地 `F:\projects_isaac\isaac-lan` 的源码，不把 J460 的偏移、对象数量或 ABI 直接搬到当前 J273。

| 对照事实 | 源码位置（相对 isaac-lan 根） | 本工程采用方式 |
|---|---|---|
| Lua 组织采集，Entity／Sprite 热路径可调用原生批量接口 | `src/bridge/sync/entity_codec.lua`；`src/engine/versions/j460/entity_fields.cpp` | 继续原生采集；把静态字段决策编译到准备期，热路径按计划批读 |
| `at<T>` 直接读取；字符串按已知长度输出；部分字段直接写二进制 | `entity_fields.cpp::capture/capturePacked/captureText`；`visuals.cpp::spriteState` | 在本工程既有合法窗口与范围约束内减少读取包装和逐字段重复检查 |
| 同 `(epoch,tick)` 共用玩家、房间、共享状态和编码片段 | `src/bridge/sync/capture.lua`、`room.lua`；`src/engine/state_codec.cpp` | 建立更严格的同采集段缓存，保留 body／ordinary 的资格与来源差异 |
| 地图、库存和较完整的 Sprite／图层仍逐 tick 采集；稳定层主要减少传输 | `capture.lua`、`layers.lua`、`visuals.cpp` | 不以删字段、降频或跨帧缓存作为性能达标的先决假设 |

其 `docs/history/2026-10-27-shared-view-encoding.md` 记录：一主三客、每占用房间200个泪弹，优化后平均采集含编码为同房1.326ms／接收者视图、分房2.552ms／视图。这是旧版本指定负载的历史数据，且共享成本按接收者视图平均，不是本工程的单帧 p95／p99，也不证明当前任意场景无性能问题。

采用的原则是：在明确所有权的只读段内一次取得数据，复用已取得的事实；把纯值整理交给后台。保留本工程必要的生命周期、未知来源拒绝、事务回卷与 Client 隔离。

## 3. 当前成本来源与改动位置

以下实现文件位于本工程 `mod/src/` 下，表内省略此前缀，同行后续文件省略重复目录；表内为静态事实，不表示已测得贡献比例。

| 成本来源 | 当前实现 | 具体改动 |
|---|---|---|
| 小读取逐次查询内存区域 | `game/observation/world_bridge.cpp::WorldMemoryReader` → `game/participants/topology.cpp::ReadMemory` | 增加二进制采集专用的窗口读取器；不全局修改共用 `ReadMemory` |
| C 字符串逐字节读取，再分块复读 | `world/binary/entity_source.cpp::CStringAt`；`entity_resources.cpp::ImageResource` | 有界分块查零；已知长度字符串一次复制；资源描述同段复用 |
| Player 重复重建拓扑 | `entity_source.cpp::Capture`、`entity_player.cpp::PrepareTopology`；`binary_entities.cpp::Prepare` | 同段每身体建立一次结构投影，World／Body 消费同一份结果；快速 body 仅在来源严格等价时复用 |
| 资源先完整解析再线性查目录 | `entity_resources.cpp::FileResource/ImageResource/AnimationResource/Intern` | 同段来源到自有描述缓存；为长期不可变资源目录建立有界有序索引 |
| 字段资格和路径重复验证 | `state/capture.cpp::Qualify/DispatchSelected`；`capture_executor.cpp::Walk`；`capture_values.cpp::Allows` | 静态规则预编译，动态检查分窗口／根／容器执行，字段只检查自身条件和范围 |
| 容器证言重复读 | `entity_containers.cpp::Close`；执行器字段 Close、Walk 尾部 Close、根尾部 witnesses | 同源证言登记一次，在规定的最终边界复核一次；保留原错误定位 |
| 旧批次仍完整执行 | `world/collector.cpp::PreparePostState/BeginRngWindow/CopyObject` | 正式同步保留窗口、身份、成员与生命周期，旧字段／RNG 诊断采集改为显式订阅 |
| 场景与成员反复扫描 | `binary_entities.cpp::Members/SceneRead/Commit` | 每段一次完整枚举、一次最终来源复核；跨段不借用验证结果 |
| 字段路径复制与每根排序 | `capture_values.cpp::Put/FinishRoot` | 相同路径共享索引；主线程顺序写值，后台建立查询索引并做纯值去重／校验 |

## 4. 目标执行过程

```text
正常原生更新／生产 Hook
  → 记录身份、生命周期、必要事件和失效序号
  → 取得真实 ordinary 或 PlayerPost 窗口
  → 开始不可重入的主线程只读采集段
      枚举成员和场景一次
      为每根核定身份、布局和资格
      按预编译计划合并复制已选字段
      容器／资源／拓扑按同段键复用
      顺序写入预分配的自有值缓冲区
      最终来源复核一次，结束采集段
  → Authority 提交点检查候选仍有效
  → 非阻塞发布现有 CaptureExchange 票据
  → 后台索引／纯值校验／编码／变化比较／每 Peer 发布
```

普通世界和快速身体仍为现有两种候选，不合并其线上版本或确认规则。后台只访问票据持有的自有值和不可变资源前缀，不能再读取游戏对象。

### 4.1 采集段与候选提交

新增概念 `CaptureReadSession`（建议内部名称）：仅存在于主线程同步采集段，含唯一递增 `read_serial`、真实窗口证言、来源失效序号、读取范围缓存、资格缓存与同段投影缓存。它不等同 `game_frame`、`tick` 或 `CaptureWitness.revision`。

复用至少核对：session／run／room／floor／generation、outer／owner／tick／mutation、phase 及身体见证、resource mutation、wrong-thread／native-exception 水位。每根另核 subject space／local／use／shape／binding、实际布局与选择臂。缺失任何事实均重新建立来源，不能只按原生地址命中。

采集段禁止主动调用 Lua、getter、Update、Render、生成器和可能重入游戏的回调。每个来源处理器需明确“只读内存／自有值”属性；不能确认的处理器保留独立检查并结束相关缓存作用域。

Prepare 到 Commit 不是天然同一个只读段。候选在 Prepare 尾部取得 `CandidateSeal`，包含窗口、水位和已复核的来源标记；Commit 检查真实 Authority 结果、身份和失效序号。仅对已静态确认期间不执行相关原生 writer 的路径，才能省去第三次全量扫描；否则 Commit 保留一次必要复核，计入总耗时。不能把“单线程”推导成任意时段数据不变。

### 4.2 缓存范围与失效表

| 数据 | 本轮允许的缓存寿命 | 失效／重建条件 |
|---|---|---|
| 静态布局、字段格式、条件、投影与资格分组 | 当前生成布局与绑定实例 | 布局／绑定重建 |
| RTTI 虚表到布局映射 | 已加载目标模块生命周期 | 模块绑定失效；对象仍须核实际 vptr 和变体 |
| 容器头、当前字段值 | 一个 `read_serial` | 段结束、异常、重入、来源水位变化 |
| 可读内存区域 | 默认一个 `read_serial`；第15步包104仅允许Prepare中getter返回之后的显式纯读页作用域 | 退出Prepare、异常、重入、资源／生命周期撤销；段间外部writer水位变化清空 |
| 根身份／ready／Grid initialized 资格 | 一个根在同段的有效期 | use／shape／binding／owner／mutation 改变 |
| Player 结构投影与资源解析结果 | 同段同身体／资源来源 | 资源、结构或生命周期失效；新段默认重读 |
| 地图摘要、Seed 上下文、成员／场景枚举 | 同段同 scope | 新段或对应 scope／来源变化 |
| 已封存的资源描述和自有 handle | 原目录的合法存续期 | 只追加，按既有目录生命周期退休；不能原位覆盖后台仍引用的记录 |
| 原生地址到资源 handle 的跨帧映射 | 本轮默认不启用 | 只有明确覆盖原位修改和地址复用的 owner 才能另行启用 |

缓存里用于读取的地址只能留在主线程 owner；任何游戏对象借用地址不进入交接票据。不同 phase 的完整值默认不互借，即使 `game_frame` 相同。

## 5. 二进制采集专用读取器

### 5.1 合并范围与区域查询

> **2026-11-17 修订（D1 已实施，包123）**：本节“区域缓存不跨调用原生代码的边界”改为：页可读信息在一个 Manager 帧内跨原生调用共用，由 SEH 加作废重试一次兜底，见[整帧可读区表](10-frame-read-regions.md)。读取器不再自持 1024 条区表，容量移入整帧表；其余执行顺序与生命周期规则不变。

在 `world/binary/` 增加读取器，由 `EntitySource` 和 `BinaryEntities` 的结构枚举共用。冷准备预分配其区域表；每段重置有效项，不逐帧分配。建议初始上限1024条区域记录，最终容量计入现有公共内存额度；达到上限时使用不缓存的受保护读取，不能为了命中率丢字段或扩容。

执行顺序：

1. 使用 u64 校验地址加法、原生对象／容器有效范围、复制长度及预算。
2. 查询同段的已验证可读区域；命中时不重复 `VirtualQuery`。未命中时查询并覆盖整个待读区间，跨区域分别检查；保护属性采用明确可读集合，拒绝 guard／noaccess／未提交区域。
3. 仅复制生成计划允许的有效字节。相邻且条件、来源和资格一致的字段可合并，不跨 UNKNOWN、填充、非活动分支或有效容器尾部。
4. 将低层复制封装为不含 C++ 资源析构的窄 SEH 读取叶，捕获访问异常并令整候选失败。区域可读不代表对象仍存活，生命周期检查继续独立执行。此叶适配当前 Win32 工具链，不用 C++ `catch(...)` 冒充访问异常保护。

区域缓存不跨调用原生代码的边界，也不提供多线程对象快照保证。已知可能由其他线程修改／释放的来源继续使用其专用 owner 或已封存生产值；没有合法所有权时报告来源缺口。

### 5.2 字符串与小字段

- MSVC string 先读并核长度／容量／SSO，随后只复制有效长度；最终复核合并到该来源的证言。
- C 字符串按不超过64B的块查找零，块长同时受声明上限、已知对象范围及可读区域边界约束。不能为了批读越过已知分配范围。无对象 extent 时，使用 owner 已声明可读的字符串范围；没有这种范围就保留窄读，不能以页可读代替对象归属。
- 同一 Image 的字符串在同段首次解析后复用；后续使用方不重复逐字符读取。
- 零长数组不执行内存查询；空／Null 是来源已确认的状态，读失败不能转为空。
- 不修改共用 `game::ReadMemory` 的行为，以免影响输入拓扑、Client 安装和其他非采集路径。

## 6. 预编译执行计划与检查分层

扩展 `mod/tools/gen_state_layout.py` 和 `CapturePlan` 冷准备，仍以布局源及 `demand.json` 为唯一字段来源。建议新增以下内部计划信息，按稳定数字索引访问：

| 信息 | 准备期确定 | 热路径工作 |
|---|---|---|
| 每布局／流的操作序列 | block、字段顺序、复制段、handler、目的值槽 | 顺序执行；不逐块扫描全部字段 |
| 字段格式 | scalar、components、canonical、条件、规范零、validator | 读取已绑定格式，条件只消费当前先行 selector |
| 约束切片 | 当前字段相关的 count group、shared projection、choice 允许集 | 只遍历所属切片；不扫全局 `CountConstraints` 等表 |
| 资格类别 | 窗口级、根级、容器／共享 owner 级、特殊动态级 | 共性资格一次核定；特殊动态资格按其具体来源检查 |
| 路径描述 | 合法父子关系、投影 mask、固定路径片段 | 动态元素只补 index／choice，不重复解释静态关系 |
| 复制段目的映射 | 一段原生数据对应哪些自有值切片 | 一次复制后各条目引用目标切片，不逐字段再搬同一数据 |

`Incubus` 等当前资格回调中会解析动态来源的情况，必须先提取为具名来源操作；不能因函数名字含“资格”就整体缓存。空主动槽、Null costume、未激活动画等条件保持先判断后读，不能先批读隐藏负载再丢弃。

保留两个写入入口：具名 handler 的受检 writer，以及只能由已绑定计划调用的内部快速 writer。快速入口省去重复的静态 `Allows`／格式解析，但仍检查实际 count、字节容量、动态选择、非法值和事务状态；不能成为任意布局或外部数据的绕过入口。Client 的不可信网络解码验证不随 Host 优化而删减。

### 6.1 容器与身份复核

`Track` 改为同段证言登记：以 owner 身份、地址、宽度和含义为键，同源多投影共享一次头部读取。使用预分配的有序索引，不计算哈希。容器的 count、presence、shared owner 和 choice 槽从同一证言派生。

正常只读段内，每份证言首次读取、最终复核各一次。删除字段 Close、Walk 尾部 Close 和根尾部循环对同一证言的重复复读；最终复核完成前不得 Seal。真正存在不同 owner 或独立变化窗口时保留独立证言，不能仅凭地址相同合并。

根入口核一次身份、ready、PlayerPost／Grid 初始化与 vptr，根结束检查失效水位。保留构造／Init／Restore／Morph／退休事件的立即撤销；候选墙仍需要已观察构造、正确墙类、当前 initialized 与槽关系，优化只减少其逐字段重查。

## 7. Player、资源与场景的一次采集

### 7.1 Player World／Body

把 `PrepareTopology` 从每次 `EntitySource::Capture` 的隐式前置操作拆出，形成 `PreparedPlayerSource`：包含当前结构 revision、自有结构投影、Sprite／costume 目录、资源解析引用和原窗口资格。每段每身体只构造一次；World 和 Body 按生成 mask 消费该来源。

ordinary 内需要联合 Body 时，仍产生两个既有语义的记录，但共用拓扑和结构来源。Body 数值若与 World 重叠，可从同段已读值投影；不能扩大 Body 到 P1／P2 等 ordinary 专属字段。

随后快速 Body 的处理规则：

- 若能证明与 ordinary 联合 Body 来自同一只读段、同一实际 PlayerPost 见证、同一 game_frame／binding／topology，复制其自有 Body 值并建立该快速候选的真实证言；不重新读取原生对象。
- 只共用值，不能无条件复制 `CaptureWitness`；`required_revision` 必须指向最终可用的 world。ordinary 未提交、失效或后台尚未满足依赖时，快速候选不得借新结构提前发布。
- 条件不等价时正常采集新的 Body。奇数帧 PlayerPost 不借上一普通帧的动态值；不为省成本丢掉合法快速更新。
- 本轮不以跨帧缓存拓扑为前提。跨帧结构未变的判定不能只看 vector 指针／count，也不能只看文件路径，因为原位内容可能改变。

### 7.2 资源解析与查重

**2026-11-17 包126**。在同段索引与自有目录之间加入有界跨帧解析缓存。跨帧以解析实际读取的字节证明等价，验证失败时重新解析。带 Player 专用结构证言的文件名来源仍走原采集检查。见[第 10 步实施记录](10-host-capture.md#2026-11-17-h3-和-h4-实施记录)。

建立两层不同的缓存：

- 同段来源缓存：键为读取段、资源类型、原生来源及其有效 owner／generation，值为自有规范描述和资源 handle。首次完整解析后，多 Sprite／World／Body 投影共用。Sprite 本帧所选动画、loaded／nullable 分支仍按当前值处理。
- 长期自有目录索引：为 `Intern` 已封存记录建立预分配有序索引，按完整规范描述的字典序二分定位。比较 table／kind／source owner／值／依赖／路径／层描述，不用哈希。原 entries 保持追加顺序和稳定 handle；排序的是索引，不能移动后台已借用的记录。

新增资源仍在合法主线程窗口复制完整必要描述；后台可做纯值编码与最终验证。资源目录不能按 Image／Animation 指针直接判等，不能为了命中而用同 id 的旧资源替代未知新资源。容量满继续明确报告缺口，不能回收仍在票据、inflight 或已确认基线中引用的 handle。

### 7.3 成员、场景与地图

将 `Members`、`SceneRead` 和 `ReadSceneContext` 组合成一个 `PreparedScene`。按当前来源完成一次枚举，保留实际成员顺序、Grid／门别名、描述槽、维度和占用表。根采集全部使用这份视图，在段尾做一次来源复核。Commit 是否需要额外扫描按4.1的调用间隔决定。

地图摘要默认仍每 ordinary 从当前有效描述产生；同描述的多投影只扫描一次保存集合。将已知连续的保存条目有效字段按页／对象边界批读，避免每个 type／variant／subtype 各走查询包装。保留可见性条件、扫描预算和原位内容检测，不扫描未需求的隐藏保存集合。

跨帧地图／拓扑／资源来源缓存不是本轮达标的必要实现。若之后采用，必须列全相关写入／退休 owner；只比较指针和 count 或固定每 N 帧重采，都不能满足本方案的当前值语义。旧第10步“低频轮转”不作为本次降成本手段，也不据此拼接不同时点的完整 world。

## 8. 拆开正式采集与旧诊断批次

在 `Collector` 中分开“窗口及身份维护”和“旧字段／RNG 诊断批次”。不能通过停用整个 Collector 来优化，因为 binary owner 仍依赖其窗口和生命周期证言。

正式同步保留：

- Authority／phase 配对、outer 和 run／room 生命周期、mutation 及错误线程／异常水位。
- Entity／Grid 身份、创建／初始化／复用／退休、成员变化、PlayerPost 和控制组资格。
- 当前候选的 attempt／capture 序号、真实提交结果与撤销通知。`NativePostMatches` 在没有旧 `pending_` 批次时也必须工作。
- 正常生产的事件、音效／显示状态和必要来源资格，不因诊断关闭停止维护。

只有显式旧 Recorder／E2E 对照订阅时执行：`BeginRngWindow/EndRngWindow`、旧 `CopyNativeFields`、旧 RawObject 正文、旧字段引用解析和专供旧批次的重复枚举。单纯开启性能计时不应打开这些功能。

`PreparePostState/PublishPostState/EndOuter` 需要适配轻量候选，不能仍以 `BatchWriter pending_` 存在作为提交或结束回调的条件。正式成员缓存由 `PreparedScene` 更新；只有完整来源才产生退出，不能将未读到／容量不足当成员离开。检查 `ConnectSources`、`sink.native/commit/event` 和 `WorldStorage` 的消费者，防止虽然跳过字段复制却仍构建旧 JSON／payload 或令输入调度依赖 Recorder 成功。

旧诊断被显式启用时复用同轮成员事实，并单独计量额外开销。其失败如涉及实际生命周期／来源污染，仍撤销正式候选；单纯诊断输出失败不让 Authority 等待或改变游戏状态。

## 9. 主线程输出与后台处理

本轮保留现有 `CapturePacket`、资源 stamp、引用目录和 `CaptureExchange` 所有权模型，不引入第二套线上格式。重点降低 `CaptureCell` 的构造成本：

1. 同根相邻字段共享父路径存储，变更元素／choice 时才追加路径；根回卷同时恢复路径与值游标。
2. 直接复制段一次写入自有 bytes arena，字段条目只记录其切片；handler 输出的复杂值仍走受检接口。
3. shared count／presence 在执行器来源组中只产生一份规范记录，发现不一致即失败。
4. 主线程 `FinishRoot` 只做必需的根完整性／预算和来源收尾，不排序整根 Cell。
5. 后台取得只读票据后，建立自有的有界排序索引；`CapturedBodySource` 通过索引查询。不得修改已经以 const span 交接的 cells，索引容量计入 Workspace。
6. 后台保留重复键、类型、条件、count 和正文合法性验证；只有验证与编码全部成功才能形成可发布快照。主线程 Seal／exchange publish 仅代表自有候选已交接，不代表 Peer 已接受。

固定布局尽可能走生成目的切片，动态容器仍按实际元素序列输出。后续若改为完全连续的内部正文，应另评估引用号回填和 codec 兼容；本轮不把重写整个 codec 作为优化依赖。

后台按每份票据编码一次公共结果，再供多个 Peer 使用；各 Peer 的依赖、发送账本和事件确认保持独立。网络背压可以替换未发送的普通候选，但不能丢掉不可替换事件、伪造删除或让主线程等待。索引、排序、编码移入后台后仍监控队列年龄，不能用积压换取 Host FPS。

## 10. 计量与内存预算

### 10.1 总口径

沿用 `core/capture_cost.h`／`replica/stream_capture_cost.cpp` 的每 outer 排他聚合。主线程总采集包括身份／来源 Hook、旧链残余、Prepare、快速 Body、事件封存、Commit 和交接；不把嵌套计时重复相加。每个 outer 的最终完整采样在退出／停用时也要结算，避免漏掉最后一帧。

现有 `stream.source.prepare_us/entities_us/scene_us/tail_us` 是累计值，快速 Body 未经 `TimedCapture`，Commit 复扫也不在其中。实施时新增分母和独立阶段，不再用它们的和冒充单帧总耗时：

| 聚合 | 必须提供的内容 |
|---|---|
| 每 outer 总采集 | 样本数、成功／失败分类、wall／CPU、p50／p95／p99／max；ordinary 和 body-only 另列 |
| 阶段 | 身份与旧诊断、场景准备、拓扑资源、实体值、场景值、快速 Body、最终复核、Commit／交接；采用排他归属 |
| 读取 | 复制调用／字节、区域查询／命中／回退、字符串块数、读取失败 |
| 复用 | 每身体拓扑构建次数、各类资源首次解析／命中、证言登记／复核次数、场景扫描次数 |
| 输出与后台 | roots／cells／paths／bytes、队列年龄、替换／撤销／Gap、worker 索引／编码耗时、实际发布与应用频率 |

诊断关闭时不采逐字段时间、不生成逐字段日志；详细计数使用主线程普通整数，只在既有状态查询中汇总。启用诊断时计时放在整阶段／整根边界，禁止在每个小 `Read` 中调用高精度时钟。诊断读数有明确统计窗口和 sample count。

### 10.2 容量

所有新增区表、资格表、拓扑／资源缓存和后台索引在冷准备期预分配，按实际 capacity 进入现有账本。遵守[第9步预算](9-budget-capacity.md)：公共封存／资源目录总额≤64MiB，及现有 Values／Workspace／Network 总池限制；不为每个新 owner 重新领取一份公共额度。

同段缓存可容量回退到正常受保护读取／重新解析；核心值缓冲和引用目录不足则整候选失败，不能截断。长期目录索引满按资源容量失败处理，不能静默绕过稳定 handle。复用已退休的旧批次内存须先确认诊断订阅关闭且所有后台租约释放，不能直接覆盖在途缓冲。

## 11. 实施顺序与完成条件

下列字母是同一重构内的工作项。按顺序完成实现和静态审阅，运行验证集中在第15步。

| 顺序 | 改动文件／接口 | 交付与静态完成条件 |
|---|---|---|
| A：补齐计量与来源契约 | `binary_entities.{h,cpp}`、`capture_cost.h`、`stream_capture_cost.cpp`、状态查询 | 定义 read_serial／seal，补快速 Body／Commit 和分母；列清可重入及非主线程 writer 来源，识别不能合并的复核 |
| B：批量读取 | `world/binary/` 新读取器、`entity_source.cpp`、`entity_source_internal.h`、`binary_entities.cpp` | 二进制来源统一走同段区表与受保护复制；C／MSVC 字符串有界批读；共用 `ReadMemory` 不变 |
| C：同段复用 | `entity_player.cpp`、`entity_resources.cpp`、`scene_source.cpp`、`scene_map.cpp`、`binary_entities.cpp` | PreparedPlayerSource／PreparedScene、资源索引；World／Body 不重复建拓扑；快槽复用条件及依赖回卷接通 |
| D：计划与检查分层 | `gen_state_layout.py`、`state/capture*`、`entity_containers.cpp` | 静态操作／约束切片，检查分层，证言只登记／最终复核一次；动态条件和特殊 owner 不丢失 |
| E：旧批次退出正式热路径 | `world/collector.{h,cpp}`、`world_bridge.cpp`、`sources/observation.cpp` | 无 Recorder 时不复制旧字段／RNG诊断；轻量候选仍完整驱动身份、提交、失效和结束语义 |
| F：输出及后台整理 | `capture_values.{h,cpp}`、`capture_executor.cpp`、`host_publish.cpp`、exchange 消费端 | 批复制切片、路径共享、后台查询索引；const 票据与预算明确，主线程无根排序；多Peer复用与事件交接保持 |

每项实现说明实际改动和未决，不以预计收益标为性能完成。A—F是本轮优化的完整实现范围；跨帧缓存和完整 codec 重写不属于必要项。只有用户恢复第15步后，才执行下列运行验收。

## 12. 第15步统一验收安排

### 12.1 先覆盖失败方式

在实施前按本表明确失败方式，全部运行验证并入原第15步；不另写单位隔离测试或提前试跑。

| 触发方式 | 必须保持的结果 |
|---|---|
| 同地址释放／重建，Init／Restore／Morph／资源替换 | 旧 use／shape／资源来源不命中；未知或失败不写 Null，不复活旧成员 |
| vector 指针和 count 不变但内容原位变化 | 下一真实采集段读取新内容；不得靠头部相同跨帧复用 |
| 字符串近边界、无终止、超长、容器 count 超额、非活动槽残值 | 不跨范围批读，不读取隐藏负载，必要失败撤销整候选 |
| 同帧多次 owner、重入、错误线程／异常、Prepare 后来源变化 | read_serial 不混用，水位撤销有效，Commit 不接受过期 seal |
| ordinary 联合 Body 与快速 Body 同轮、奇数帧、变形／换装 | body 来源和 required_revision 正确；不重复重建同段拓扑，不借上一帧动态值 |
| 容器别名、多投影、空→非空→空、同count替换 | count／presence 与全部元素属于同一来源，清空真实生效 |
| 区表／临时缓存满，输出／目录满，worker 背压 | 缓存有界回退；不可回退的容量整包失败；不阻塞Authority、不截断或假确认 |
| 切房／换层／死亡／重连／停止，旧票据仍在消费 | 缓存与新scope隔离；在途自有值安全退休，事件水位不重复或遗漏 |
| Recorder／对照订阅切换 | 正式同步不依赖旧批次；诊断额外成本单列，关闭后不残留旧全量采集 |

### 12.2 场景与计时方法

- 沿用现有 E01—E40 中仍有效的功能、生命周期、容量、性能与隔离项；性能不另建独立验收编号。完成本轮实现、用户恢复第15步后，在同一整体验收中继续定位并修复当前 Client 未出画面／退出问题，同时完成性能及受影响功能覆盖；Client 未正常应用和显示时不得宣告端到端通过。
- 用实际发布包覆盖一主一客、一主三客、四参与者多身体（含八身体能力范围）、普通房、装备／costume丰富角色、Grid／门较多房间、已探索楼层和实体密集战斗。200个有效实体作为与历史对照相近的负载点，另覆盖原计划典型约300实体；实际实体／图层／保存条目数量必须随计时报告，不能仅写“压力场景”。
- 每个稳定负载段预热后建议连续采样60秒；切房、换层、首次资源解析和生命周期突发另列 p95／p99／max，不悄悄剔除。整个第15步连续执行，失败先修复，再继续受影响覆盖，不重新编号。
- 性能采样采用生产采集路径、关闭旧 Recorder／Lua 对照与逐字段诊断；在同一验收内另设明确的正确性对照段，记录附加开销。不能把双采／诊断成本与生产指标混为一谈。
- 可使用已有有效历史基线；如需重跑当前版本基线，也只能在用户恢复后的第15步安排，不把重跑作为必须额外启动游戏的前提。对照不计算哈希，不要求最终 artifact。
- 若使用旧执行器作 E2E 参考，只消费同一合法段已有的自有值或显式诊断来源；不能在游戏已推进后重新读原生对象却声称同帧一致。比较实际需求字段、资源语义、成员关系和事件，不比较指针、填充或无意义的内部 handle 数值。

### 12.3 判定与报告

1. 既定性能：主线程总采集 p95≤2ms、p99≤4ms，Host≥50FPS；分场景报告完整分位数、max、CPU和帧间隔。不能只用累计总数／平均数或加入大量无采集帧稀释结果。
2. 同时报告 ordinary、body-only 与完整 outer 分布；实际采集／发布／应用频率、失败率和队列年龄不恶化，不能靠不发布状态提升 FPS。
3. 验证机制计数：同段同身体拓扑最多建立一次；同源资源只首次解析；每份证言只最终复核一次；正式路径旧字段／RNG诊断采集次数为零；区表命中时无重复区域查询。
4. 当前 wire 与需求完整性、事件 exactly-once、资源接受和真实 Client 显示均满足原验收；内存与9Mbps持续总额、首次准备1MiB／10秒等原预算不放宽。
5. 达不到指标时按排他阶段和读取／复用计数继续定位，不预设必须通过删字段、降频或放弃验证解决。未达标和未知保留原事实。

每次实机启动前按项目规范说明用户配合及停止手动操作的时机，需要配合时先确认准备好。正常测试临时材料用后清理；仅在中断且需要续查时保留必要材料。

## 13. 实施记录

| 工作项 | 已接入的实现 | 保留的边界与未决 |
|---|---|---|
| A | `BinaryEntities` 为同步读段分配 serial，候选 seal 记录读段、资源水位与完成根数。Commit 核实际 Authority 结果、完整窗口、PlayerPost 与身份。`cs stream main` 公开排他 wall／CPU 分布和候选计数，`cs stream source` 公开读取、投影、诊断和 worker 计数，`cs stream client` 公开实际原生安装累计次数。 | wall／CPU 分位数是对数桶上界估算。发布与安装频率由各端本地采样间隔内的计数差得出，待第15步采样。 |
| B | `CaptureReadSession` 共用固定 1024 项区表，溢出仍受保护读取，Win32 SEH 捕获访问异常。正式成员列表按连续指针区复制，内联 C 字符串按已核范围分块，MSVC 字符串只复制有效长度。 | `Image+0x3c` 的名称指针缺少分配 extent，首次解析逐字节读取并有界复核；同段后续使用规范资源描述。 |
| C | `PreparedScene` 保存成员顺序、房间描述、Grid／门别名及种子投影。地图按描述当前可见分支聚合。`PreparedPlayerSource` 供同段 World／Body 共用拓扑；严格匹配的 PlayerPost 从联合根复制自有值，依赖 ordinary 的实际修订。规范资源目录以有序索引查重，原记录只追加。 | 快槽依赖仍由 worker 按真实已编码 ordinary 复核。跨段重读结构和资源，允许最新票据替换时撤销过期快候选。 |
| D | 生成器输出每块字段切片，执行器冷绑定计数组、共享 owner、choice 和字段格式。Incubus 的动态来源由具名叶读取。每根按地址和宽度归并同源证言，在根结束时复核。 | 种子上下文、显示生产以及各根保留各自的资格与证言。只读段外的 getter 和原生生产器各自维护来源。 |
| E | 正式 Host 的 Collector 后态不占用旧批次槽。显式 Recorder／旧观测才执行旧字段和 RNG 诊断。旧诊断 RNG 前值和正文共用成员清单，正式采集可用其完整成员值作首遍投影，段尾复核原生来源。完整普通候选提交后更新正式成员位。诊断环或 reader 失败终止诊断交付，录像关闭分别核对生命周期序号和实际入环条数。 | 旧诊断独有的私有 Player 关系、Lua 与字段资格保留。 |
| F | 生成直接复制段落入自有 arena，条目引用切片，相邻同路径字段共享路径。`FinishRoot` 顺序封闭事务；后台使用记入 Workspace 的有界索引查重并执行原正文编码。快槽引用目录按实际身份紧排时仍使用原有容量。 | 网络继续共用后台规范快照，Peer 的事件接管、资源接受和 AppliedAck 仍按独立账本推进。 |

### 来源与复核边界

- `SceneRead`、`Members`、`ReadSceneContext`、`EntitySource` 的根、容器、地图与资源处理器，以及 `BinaryDisplayCapture::Prepare` 的显示值投影均走主线程内存或自有值。`QueryBinaryDisplayInputs` 调用原生概率与资格 getter，放在 ordinary 读段入口之前，返回后重新核实际 Game／Room 窗口和资源水位。
- Entity 的构造、Init、Restore、Morph 和退休由 Collector／身份 Hook 维护。Grid 槽、门映射和描述槽在段尾复核，候选墙保留已观察构造、墙类 vptr、初始化及槽关系。Level Init 与 Sprite／Backdrop writer 保留撤销路径。显示／音频只在相应正常生产边界读取，SFX 意图只取已选样本路径与生产参数。
- `FrameEnd` 中 `WorldPreparePostState` 与 `EndManager` 的 `WorldPublishPostState` 之间经过 `WorldPostState` 和 `TickPipeline::FinishPostState`，正常分支只提交本地时钟与纯值 receipt。ordinary 因此省去 Commit 的第三次全量场景／成员扫描，保留真实窗口、水位、根身份和快槽见证复核。
- 错线程／异常水位来自 Collector 观察边界。已挂接的 Sprite writer 在非主线程只更新原子进入／退出水位，重叠读段作废。音频后端线程、GPU 与图像后端的缓冲、光标和生命周期保留各自原生 owner，票据只包含游戏线程已取得的描述和生产值。未知外部 writer 的并发所有权仍需对应来源证据，页属性和 SEH 只保护读取边界。

本轮只做静态源码与文档审阅。第15步仍暂停，Host FPS、p95／p99、实际应用频率和 Client 未出画面／退出问题均待原统一验收确认。

## 14. source_native与实体Body热路径实施方案（2026-11-15）

### 14.1 目标、状态与范围

本节是在A—F已有实现上继续减少重复工作的实施合同，属于第10步。最初交付为设计文档，源码进度见第14.7节及后续记录。预计收益为静态推导，运行验证归第15步统一整体验收。

目标保持主线程总采集p95≤2ms、p99≤4ms、Host≥50FPS及真实窗口下ordinary约30Hz／合格Body约60Hz。不删需求字段，不降低采集频率，不改变两流版本、PlayerPost资格、结构联合提交、事件水位、资源接受或AppliedAck。不改变统一输入、C/S隔离、Hook常驻与主线程对象访问规则。

本轮只改变内部读取、计划和纯值克隆表示，不因此提升线上布局版本。接口／结构名称为实施建议，可沿用现有命名；新增核心结构、函数和失效规则须有中文注释。

事实分开记录：

- **既有实现**：二进制读取区表、直接复制段落入arena、同段资源memo、Player拓扑revision缓存、字段切片、根尾统一证言复核、后台索引和旧诊断拆分已存在，不列为本轮新增成果。
- **静态结论**：本节列出的调用、重复遍历与重复解析存在于审阅源码；尚无本轮优化后的耗时或频率结论。11月14日performance-6历史采样只帮助选方向，不能当作当前代码占比。
- **独立运行问题**：第15步包32已观察到成员号复用11884次、高水位977，但仍发生编码中撤销和准备无进展；包33的已封存短时实体候选保留修订尚未实机。它们按原记录续查，本方案不把CPU优化等同于发布恢复，也不改写候选有效性合同。

### 14.2 静态定位与本轮改动

以下路径相对`mod/src/`；准确定位以函数名为准。

| 工作面 | 已确认的重复工作 | 实施位置 |
|---|---|---|
| source_native拓扑 | `WorldFrameBegin`、`WorldNativeReturned`、`WorldPreparePostState`刷新全部参与者；`WorldPlayerAnchor`在受控Player更新前后完整读拓扑。各参与者重复读取公共Game／Room、roster和设备名单 | `game/observation/world_bridge.cpp`；`game/participants/topology.cpp`；`game/authority/lifecycle.cpp` |
| 拓扑读取内部 | 根Player身份先读、身体遍历再读；controller在身体及roster冲突检查重复读取；小读取仍走逐次VirtualQuery的共用ReadMemory | `topology.cpp::ReadTopologyForRoot/ReadMemory`；`game/input/input_lease.cpp` |
| 锚点安装 | 每次刷新遍历完整槽表；`AnchorPlayer`在判断值不变之前扫描其他锚点 | `world_bridge.cpp::WorldRefreshAnchors`；`world/collector.cpp::AnchorPlayer` |
| 字段分派与输出 | root_qualified已避免完整身份逐字段重查，但窗口／线程／资格包装仍重复；普通writer重取格式和父路径，LastValue再核刚写的值 | `state/capture.cpp`、`capture_executor.cpp`、`capture_values.cpp`；`world/binary/entity_source.cpp` |
| 容器投影 | 每个组内字段再次调用Counts／Shared遍历全组；opened只减少重复Open，没有消除重复组遍历 | `capture_executor.cpp::Counts/Shared/Walk` |
| Player联合来源 | World／Body独立Capture；PrepareTopology命中主要复用revision，不提供完整可消费的结构投影；两次Capture各自重置证言、识别根及复核 | `game/observation/binary_entities.cpp::Prepare`；`entity_player.cpp::PrepareTopology`；`entity_source_internal.h` |
| Body复用 | ordinary联合Body的Append仍逐cell重取格式、核路径、复制及重映射引用 | `binary_entities.cpp::Prepare`；`capture_values.cpp::Append` |

`source_native`是多个调用点的排他聚合，不能把其全部耗时归于ReadTopology；生命周期中不在该计时作用域的读取也不能加进此阶段冒充实测。优化计量须区分入口与子成本。

### 14.3 H：同一边界内的拓扑批次读取

建议引入`TopologyReadBatch`，以一次实际同步读取调用为寿命。它是主线程临时读取视图，不是按帧号保存的跨边界拓扑缓存，也不承担输入绑定或身体活动见证的发布职责。

执行过程：

1. 在当前合法边界建立批次，记录session／run／room、绑定与生命周期状态。批次入口读取当前公共Game／Room及阻断状态。
2. 一次取得有效roster和设备名单。设备读取沿用InputLease保护；同批参与者复用该租约，不为每个参与者重新加解锁，不跨原生Update或整个Manager帧持锁。
3. 以本批实际对象身份建立有界Player事实表，复用已读type／variant／index／controller、状态及关系。按各参与者当前根分别求控制组；根缺席时仍按现有已知关系寻找替换，不解引用缺席旧根。
4. 用同一公共名单做设备唯一性、跨组身体重叠和同controller归属检查。共享事实不能把参与者冲突变成合法多归属。
5. 投影锚点并批量检查目标集合及未替换槽的冲突；先撤销全部变化槽，再安装新槽。稳定槽不重复进入完整AnchorPlayer冲突扫描。其他单槽调用继续保留原检查。
6. 批次退出即销毁原生读取视图。只有现有owner明确允许的纯值拓扑进入生命周期流程；新批次重新取当前值。

读取器采用局部有界区表和窄SEH复制，可抽取现有读取器的低层实现，不全局放宽ReadMemory。（2026-11-17 修订，D1／D3 已实施，包123：拓扑读取与 `ReadMemory` 改用[整帧可读区表](10-frame-read-regions.md)，删除局部区表；`ReadMemory` 补窄 SEH 并统一可读集合。本节其余批次边界不变。）相邻已核字段允许合并，但不能把整个Player对象或未知间隙当作连续合法源。区表满回退到受保护读取；关键事实／名单容量不足或读取失败沿原故障语义关闭相关组，不截断或复用上一批结果。

`PlayerIsHologram`仍是原生只读API调用，不能仅凭函数名把它当作普通memcpy。保留当前语义，单列调用边界；未确认其调用链不影响缓存来源前，在调用边界切断相关读取复用。InputLease失效及后端并发的原有处理保持，不扩大无锁访问范围。

明确保留的边界：

- Player更新前后分别取当前拓扑，不能合并为一次；生命周期代次不变也不证明身体关系未变。
- `WorldNativeReturned`与`WorldPreparePostState`暂不共享批次，之间可能仍有原生调用。
- 已有`topology_just_read`只消费对应生命周期读取的有效见证，不把布尔值扩大为任意同帧复用许可。
- Join、换根、死亡／复活、ghost／hologram、设备变化、房间／run切换、异常或重入立即结束相关批次；未知变化不能凭vector指针／count不变视为稳定。

### 14.4 I：绑定计划、组校验与内部writer

扩展现有冷准备结果，静态计划仅来自布局源、demand及现有生成元数据。每条操作保存字段格式、资格类别、复制段、validator、路径描述、组号和目的条目规则；布局流mask、布局预算、canonical投影到子块的关系在准备期绑定。按实际capacity计费，热路径不扩容。

检查分层：

| 层次 | 保留的检查 | 消除的重复 |
|---|---|---|
| 读段／根入口 | 主线程、不可重入、真实窗口、身份／use／shape／binding、布局与来源资格 | 将ready／player_ready／text_ready等共同资格按类别核一次 |
| 字段／元素 | 当前selector、有效范围、数量、容量、引用／资源、有限值与数值规则 | 不重复解释静态格式、静态父子关系、静态允许块 |
| 来源变化边界 | 失效位与外部writer水位，原生／getter／回调边界重新建立合法来源 | 在未调用原生代码的已绑定链内合并多层相同窗口检查 |
| 根／联合根结束与提交 | 最终证言、真实Authority结果、候选撤销、主线程提交与依赖 | 不把同一份最终证言在World／Body重复复核；实现见14.5 |

不能把所有handler统一视为纯读：实施时分类为已确认内存／自有值操作与独立来源操作。后者继续受检并保留其动态检查；Incubus等具名动态来源不因资格预绑定而消失。外部writer水位及访问异常保护不能因根入口已通过而全部移到段尾。

组校验按“递归实例＋组号＋当前选块”保存状态。组内selector已经就绪、所有选中投影成功Open并核过count／presence／identity后，才标为完成；失败不得设置完成位。每个数组元素、choice实例和根都重新建立状态，嵌套返回不得借用兄弟实例。这样将同一k项投影组的重复全组遍历从接近O(k²)降为一次O(k)检查。条件不齐时维持原拒绝或既有顺序，不能提前读未激活负载。

内部writer只供已绑定执行器使用，不向任意handler或网络解码暴露“跳过检查”参数。以绑定操作和当前实例标记携带合法路径／格式，动态元素只补索引和实际choice。内部写入返回刚写值视图，供数值validator与selector消费；保留条目数、宽度和容量约束，取消LastValue对同一内部写入的再次路径比较。普通特殊handler仍需核其确实只写当前路径和预期条目。

CaptureCell继续作为内部交接表示：Count／Presence使用canonical键，父路径选块不能扩大为整对象；Null、显式空、规范零和有序集合规则不变。Client不可信解码及后台正文完整性校验继续保留。

### 14.5 J：Player联合来源与证言

将PreparedPlayerSource从“结构比较字节＋revision”扩展为本读段可消费的结构投影。建议保存以下内容：有效Sprite／costume容器头、当前选择臂、已核元素定位、资源句柄和来源证言索引。主Sprite、13个固定Sprite、costume与15项map仍覆盖当前需求，未loaded／Null／未激活分支保持先判定再读取。

World和Body的块集合不同，不能当成两份相同负载合并。本轮只共享两者重复使用的来源：

1. 在同一ordinary只读段，为同一Player建立一次联合上下文，完整核身份／布局／结构来源。
2. 分别执行World与Body选块，使用同一份已核结构投影；动态值只在来源、条件、时点完全一致时共享。
3. 同源证言按owner身份、地址、宽度及用途归并。联合上下文最后复核一次，保持最初字段／布局诊断位置。
4. 两个记录和复用资格只有在联合复核成功后才可供Seal／快槽克隆消费。第二流或最终复核失败，整个ordinary候选失败，禁止把先完成的World根用于发布。

投影存储不能继续借用每次Capture都会重置的scratch；使用冷准备计费的专用段内存或显式联合arena，寿命覆盖两个流及最终复核。原生地址只留主线程，交接仍为自有值。根身份、资源或writer水位变化撤销整份投影；不得把旧body读取依据传给新身体或新use。

奇数帧独立Body仍开启自己的真实PlayerPost读取：重新确认当前形态和结构，读取本次动态值；不能借上一ordinary的完整动态值。发现新结构仍走NeedsOrdinaryJoint。多个Body的跨窗口页表／资源memo复用不是本轮必要项，不为了扩大命中率合并它们的窗口见证。

### 14.6 K：已验证联合Body的批量克隆

在现有严格reuse条件后增加私有批量克隆入口，普通Append继续服务其他来源。输入必须是本次ordinary中联合复核已完成的Body根，输出仍是独立Body槽；不共享底层可变存储，不延长ordinary票据寿命。

根封闭时登记内部克隆元数据：本根cell范围、路径范围、arena字节范围及引用条目定位。元数据在AbortRoot时一并回卷，只含自有缓冲偏移和长度，不进入线上协议。当前相邻路径共享和直接复制切片必须保证引用范围属于本根；不能假定cells描述的字节无重叠后逐cell重复复制。

克隆顺序：

1. 核源联合复核完成、实际layout／mask／subject、fast_body资格及现有window／PlayerPost／game_frame／topology匹配；目的票据保留自己的witness和required_revision。
2. 用宽整数核源范围和目标records／cells／paths／bytes容量，目标不足整根回卷，不截断或借ordinary超额空间。
3. 各连续区批量复制一次，按目标区起点调整cell.path与cell.offset。仍需线性遍历cell修正偏移，但不重做格式描述、路径解释和逐cell字节搬运。
4. 仅处理引用条目。若要保留handle，必须证明两侧为同一稳定目录及同一有效前缀映射；仅指针相同或长度相同不够。其他情况按原物理身份重映射，更新reference_limit，保留Null／Identity／Pending含义。资源句柄只按既有追加目录与stamp规则使用。
5. 成功后封闭目标根；Catalog的快槽紧排及容量检查、Commit的ordinary依赖和后台合法性检查继续执行。任何失败保留首因并撤销目的候选，不修改源根或提前确认。

### 14.7 实施顺序、计量与容量

G—K是第10步内部工作项，全部完成后才在第15步进行所需运行验证，不按表行拆出测试批次。

| 顺序 | 交付 | 静态完成要求 | 状态 |
|---|---|---|---|
| G：细分计量与契约 | `capture_cost.h`、`stream_capture_cost.cpp`及既有状态查询；接口寿命注释 | 区分拓扑公共读取／组派生／锚点安装、字段执行、联合准备与Body克隆；原总口径不丢成本 | 源码已接入，待第15步验证 |
| H：拓扑读取 | 局部读取器、TopologyReadBatch、批量锚点提交；接入全组刷新及单组读取入口 | 同批公共源一次读取；原生更新两侧独立；设备租约、冲突与失效语义完整 | 源码已接入，待第15步验证 |
| I：执行器与writer | 绑定元数据、每实例组状态、内部值写入及返回视图 | 静态元数据不逐字段重解；动态检查和特殊handler保留；失败仍原子回卷 | 源码已接入，待第15步验证 |
| J：Player联合来源 | 可消费结构投影、联合证言、双记录封闭 | 投影不借已重置scratch；World／Body不串选块；奇数帧仍读当前源 | 源码已接入，待第15步验证 |
| K：Body批量克隆 | 根范围元数据、私有克隆入口、引用处理及计费 | 严格reuse条件保持；偏移／目录／事务完整；输出独立持有 | 源码已接入，待第15步验证 |

新增粗粒度计数：拓扑批次数与参与者数、公共读取／区域查询／命中、重复Player事实命中、租约取得次数、变化锚点数；组校验执行／复用、绑定／普通writer次数、联合结构构建／消费次数、证言复核次数；Body命中／未命中原因、克隆cells／paths／bytes、引用重映射数。理由分类必须有界，诊断关闭时不产逐字段日志。

计时只放整阶段／整根，采用原排他wall／CPU聚合；不要为每个字段或小Read增加时钟调用。分别报告ordinary、body-only、成功、失败及负载场景的分母和分位数，不以无采集帧稀释。锁等待与读取成本可在粗边界区分，不能直接把wall当作CPU。

容量沿用实施时现行StateBudgets和第9步账本。11月15日现行普通槽为131072 cells，共享采集值额度为72MiB；第10.2节64MiB是原方案时点，不作为本轮回退目标。本轮不再扩容，新增计划、事实表、联合投影及克隆元数据按实际capacity计入Values／Workspace和共享汇总；复用现有空间必须满足全部借用结束，不为每个结构领取独立总额。纯缓存满可走原受检路径，核心记录不足整候选失败。首次准备、每Peer、线上消息和总带宽额度保持。

### 14.8 失败方式与第15步覆盖

下表先规定后续实施必须处理的失败方式，运行覆盖并入[第15步](15-e2e-acceptance.md#158-host采集热路径补充验收2026-11-15仅设计)，当前不执行。

| 失败方式 | 实现要求／后续观察 |
|---|---|
| 批次越过Player更新、getter重入或外部writer后继续命中 | 切断或撤销对应批次；真实前后关系变化被发现，不能因同帧同generation漏掉 |
| 多参与者共享名单后串控、重复身体或设备歧义 | 保留跨组冲突与唯一设备检查；两阶段撤销／安装不清掉新槽身份；未变化槽也参与冲突判断 |
| 读失败／页保护变化／锁不可用被解释为空 | 明确失败并沿原中性／撤销行为处理；缓存回退不取消SEH或对象范围约束 |
| 组完成位跨元素／根复用，条件字段提前读取 | 以递归实例重置；choice、Null、空容器、规范零和同count重排均正确处理 |
| 绑定writer把字段写入错误路径／选块，漏有限值或数值规则 | 内部入口仅接绑定计划；特殊handler仍核输出；错误不能成为合法快路径值 |
| 联合投影借用已重置scratch，第二流失败后第一流仍可发布 | 存储寿命覆盖双流与复核；整ordinary失败；快Body不得借未完成联合源 |
| 奇数Body使用上一ordinary动态值或越过结构依赖 | 实际位置／姿态更新保留；换装、根切换、形态改变仍需真实ordinary依赖 |
| 克隆偏移溢出、路径／切片跨根、容量不足或引用目录不同 | 宽算术与独立范围检查；失败全回卷；不同目录按身份重映射，不静默保留旧号 |
| 源票据替换、退休或取消使目标Body悬空 | 独立复制后目标只持自有值；原候选撤销和提交依赖仍生效 |
| 省去观测、降低频率或只看空闲平均值产生虚假达标 | 生产采集计时与显式对照成本分开；报告实际发布／应用、尾延迟、失败及未覆盖 |

后续场景包含普通P2／P4、多身体及备用／换根、移动射击、暂停恢复、死亡复活、同门往返、costume／动画资源变化、同count原位变化、短时Entity大量创建移除、Body依赖失败与停止排空。继续覆盖既有高负载及发布撤销问题，不用主线程变快掩盖worker无法发布。机制计数下降不能代替正确画面、输入、真实ACK和完整性能指标。

### 14.9 初次设计文档交付

已完成G—K实施设计、静态定位、失效／容量合同及第15步覆盖登记；源码修改、生成、构建和运行验收均未执行。没有新建测试artifact，也没有新增gate、哈希校验、独立角色或子智能体要求。

### 14.10 H 的源码实施记录

- `ReadTopologies` 接入生命周期与采集锚点全组刷新。每批复制一次公共 Game／Room 与活动 roster，按需在同一 InputLease 下复制设备名单及各设备 ID。各组独立派生角色、身体活动资格、设备唯一性和拥有者，批次内跨组检查共享身体。单组入口复用这套读取器，每次 Player 更新前后从各自的调用重新取证。
- `TopologyReadBatch` 只在当前同步调用内复用 Player 事实和 32 项可读区表。区表满时继续逐区查询及 SEH 复制。J273 `IsHologram`（RVA `0x3D1C90–0x3D1CCF`）的静态指令只读 Exists、PlayerType、twin 和索引，官方 REPENTOGON 包装直达该叶。保留原生调用，异常、嵌套 owner／拓扑读取、已登记的异线程故障或资格变化撤销本批。Workspace 固定容量计入进程内存账本和正式采集共享上限。
- `Collector::ReplacePlayerAnchors` 对变化的目标集统一判重，未变化槽也参与冲突检查。先撤销变化旧槽，再安装新值并报告实际提交结果。空槽及无变化集合直接结束，单槽入口保留原身份与冲突检查。
- `cs stream main` 的 `topology.source_native.*` 与 `topology.other.*` 记录各自的公共读取、区域查询与命中、Player 事实复用、实取后端锁和撤销次数，`topology.anchor_changes` 记录批量提交中的实际变化槽。计数合并沿用现有 `source_native` 计时边界。

本次仅做源码和静态边界复核。构建、实机功能和耗时数据待第15步统一验收。

### 14.11 I 的源码实施记录

- `CapturePlan` 和 `CaptureExecutor` 冷绑定流掩码、布局预算、值格式、复制段和子块投影。数字 validator 直接消费生成字段索引。`ready`、`player_ready`、`text_ready` 在本根首次使用时核定。具名来源仍执行动态检查，Incubus 的武器证言仍由原 handler 登记并在根尾复核。
- 计数组按本实例的组首字段保存完成位，共享 holder 组按 canonical 字段保存完成位。首次使用先核本选块的 selector、数量／存在性及来源身份，成功后才复用。递归入口为每个根、数组元素和实际 choice 臂清空对应状态切片。
- 执行器以选中投影直接写入结构条目、规范零和原生复制切片，标量写入返回当前 arena 的值视图。路径偏移只在当前递归实例内复用。具名 handler 继续通过受检 writer 写入，并复核写入条目数和路径。受保护的原生读取、动态值校验、根尾证言及失败回卷继续执行。
- `cs stream main` 增加 `execution.group_checks`、`execution.group_reuses`、`execution.bound_writes` 和 `execution.checked_writes`。按根合并尝试次数，含失败根已执行的工作。冷绑定和预分配状态按实际容量计入现有 Values／Workspace 账本与共享采集上限。

本项完成源码和静态复核。构建、实机及实体／图层负载下的实际性能归第15步统一验收。

### 14.12 J 的源码实施记录

- `EntitySource::CaptureOrdinary` 根据实际 RTTI 核定 Player，World 和 Body 保留各自原选块并在一次同步调用内依次写入。`PreparedPlayerSource` 保存主 Sprite、13 个固定 Sprite、costume 向量头与元素索引、图层定位和图像／配置句柄、15 项 costume map 的有效选择。资源与容器 handler 按当前 canonical 路径取投影。活动动画和帧选择器由实际选块按需读取，各流动态值仍按原计划采集。
- 投影与证言共用现有预分配 arena，作用域覆盖两流及最终复核。证言按子 owner、地址、宽度及用途去重并保留首次诊断位置。资源和生命周期失效撤销当前来源。两流任一失败或联合复核失败时回卷两根；复核通过后才登记可供快槽消费的 Body 记录。
- 独立 PlayerPost Body 每次建立自己的投影和证言。跨段只保留纯值拓扑比较字节和已确认 revision，发现新结构仍走 `NeedsOrdinaryJoint`。同窗快槽继续沿用既有 PlayerPost／帧号／结构依赖匹配，克隆入口见第 14.13 节。
- 状态查询增加 `stream.source.joint_prepared`、`stream.source.joint_consumed`、`stream.source.joint_reviews`。`joint_consumed` 计联合复核成功的流记录数，`joint_reviews` 计到达根尾的复核尝试。内存仍按原 Values／Workspace 账本计费。

构建、实机和性能验证归第15步统一验收。

### 14.13 K 与 G 的源码实施记录

- ordinary 槽为联合复核成功且符合快槽字节预算的 Body 根保存路径、字节区间及非空引用条目范围。克隆入口只借用当前主线程持有的 ordinary writer。先核对两端真实窗口、完整 Body 选块及所有区间和容量，再各复制一次条目、路径和字节区。重定位保持共享路径与复制切片的关系。目标快槽继续使用自己的 PlayerPost witness 和 ordinary 依赖。
- 引用从 ordinary 封存前缀取物理身份并重新登记，维持原 Null／Identity／Pending 语义。资源句柄继续由同一追加目录及发布 stamp 封存。克隆失败回卷目的根。发布阶段保留 `Catalog` 和票据封存的具体失败诊断。普通 `Append` 继续处理其他纯值来源。新增元数据仅在普通槽预分配，按实际容量计入 Values 和原共享额度，线上布局与预算上限保持。
- `cs stream main` 新增拓扑公共读取、组派生、租约取得、批量锚点安装、字段执行、联合准备及 Body 克隆的 wall／线程 CPU 计量。新增细分的子时段按排他归属计算，已有阶段保留原归属，主线程总耗时只计一次。租约计时覆盖取得过程，等待可通过 wall／CPU 差异观察。复用命中表示具备克隆资格，实际复制工作按 `body_clone.complete.*`／`body_clone.failed.*` 分别计量。
- 普通帧和仅 Body 帧分别呈现发布、失败及适用的混合／仅联合结果。空采集帧单列。既有分页同时返回 wall 和线程 CPU 的分桶计数，第 15 步按明确负载区间取前后差分，复核样本分母和分位数。阶段计量不引入逐字段日志或时钟调用。

G—K 已完成源码接入及静态复核。构建、容量实测、实机功能和性能数据归第15步统一验收。


### 14.14 第15步续修：Prepare内页信息复用

> **2026-11-17 修订（D1 已实施，包123）**：页作用域不再负责保留区表；区表寿命改为 Manager 帧，由[整帧可读区表](10-frame-read-regions.md)统一持有，`FrameBegin` 入口清空。读取序号、外部资源写水位、Revoke 及本节四项须继续拒绝的失败方式保持不变；Revoke、读取异常和越界在撤销会话的同时作废整帧表。下文为原实施记录。

包104把可读区域与根读取序号的寿命分开：Prepare在概率getter返回且重新核根后，由首次ReadWindow惰性开启页作用域；奇数帧从第一个独立Body读段开始。两次ReadWindow之间只执行Collector的当前窗口检查、Add的身份／形态读取、候选票据和自有数据登记，没有原生setter。EndPrepare在所有返回路径先EndRead再结束页作用域，不能延伸到Commit、FrameEnd或下一帧。容量仍为1024，不保存对象值、资源句柄或旧根证言。

每个Body仍Begin一次、递增read_serial、重建完整PlayerPost／资源来源和根尾证言。ReadSession结束只在页作用域仍有效且外部writer水位未变时保留区域；下次Begin在覆盖旧owner水位前复核上一水位，失败则清空区域。Revoke、读取异常和越界立即清空。EntityChanged、ResourceChanged和ProductionBegin在段间也识别页作用域并撤销，ReadProduction拒绝借用此作用域。四项失败方式必须继续拒绝：段间生命周期变化、资源写水位变化、意外原生生产重入、任意失败返回泄漏页见证；不因页命中放松各根资格。沿用第15步实机装备／角色变化和停止恢复覆盖，不在设计阶段另跑隔离测试。

新增read_page_spans／read_page_continuations聚合计数；continuations表示新读段建立时保留了非空区域表，不代表复用字段或省去实际页查询的精确次数。此修正只针对连续纯读段之间的重复页查询，不能据此宣布ordinary或整体验收性能达标。


### 14.15 第15步续修：纯标量批次与客机预检

包107按run113健康采样优化客机：Readback稳定基数排序仍按完整64位键的原七个有效字节顺序执行，小端对象表示取字节替代x86的64位变长移位；不缩窄record或costume字段。新建非玩家成员的vptr／种子／隔离检查只读取同批事实，保留当前页窗口；真正进入新玩家ToPlayer虚调用时前后清空。特殊字段循环一次借用不可变DefinitionIndex；外部字段继续核范围和元素边界，分配失败退回完整分类。run114已做持续压力，结果见第15步，整体验收尚未通过。

包108扩展Host纯标量批处理：仅合并同布局块、生成执行顺序连续、无条件无共享且同一根稳定qualifier／qualification_slot的Copy叶。原生空隙另起精确读段，每段继续执行CaptureWindow范围／当前窗口检查和ReadSession的可读页／SEH保护；不得把空隙、handler或条件字段读入。cell偏移变成这一私有arena批次的紧密偏移，每个字段仍保留独立cell、validator、selector值和seen状态。资格相同且访问／字节总预算充足时批量计数；紧预算退回原逐字段顺序，首字段的双次访问计数保持。

必须继续拒绝：不同资格误并、任意原生读段越界／失效／异常、总访问或字节预算溢出、任一非有限／非法标量、cell或arena容量不足。任何一段失败仍回卷整根，不发布部分批次。新段表与绑定表按实际capacity计入现有Workspace，不新增跨窗口原生值或地址缓存；第15步用真实角色／武器更换、实体生成退休和压力续测覆盖，不新增隔离单元测试。


包108的多读段扩展在run115未体现明确普通帧总耗时收益，包109撤回这一扩展，恢复严格原生连续段；保留同qualification_slot的资格合并及预算充足时批计数。上述包108实现与结果作为历史记录保留，不代表当前仍使用多段表。

包109客机readback在已验证完整键的costume∈[0,1]、stream∈[0,1]、record<本批记录数、root_block<32时，将其单调映射为`((costume*2+stream)*records+record)*32+root_block`。仅当桶数不超过操作数两倍且已有整数工作区capacity足够时做一次稳定计数排序；总前缀与下标受现有262144操作上限保护。原64位key继续保存，未压缩或删位；同key按原operation顺序写入结果。范围不适用退回包107完整字节排序，不拓展工作区或放宽预算。必须防止stream／costume遗漏、record截断、同key不稳定和稀疏大编号引发无界分配，均沿本次第15步实机结构变化和失败恢复续验。


### 14.16 第15步续修：客机纯借用边界

包110最终读回的连续原样字节可在同record／根块内合并，保持来源和目标同时连续、128项上限、逐字段失败定位以及masked／引用／header独立检查。包111按run118采样把Covers／Installs的不可变分类表借用移至安装器构造，计划定义下标仍来自已验证计划，冷分配失败退回原分类。

ClientStagedFieldBorrowAttachment只调用当前窗口／header／武器与shared承载的纯读取检查，随后增加attachment_uses；不调用原生setter、分配释放或改写header。包111仅对这种已保留的FieldAttachment在InstallNow前后保留当前页见证，原Current、Capture可写性、Borrow完整检查与done登记顺序保持。新附件、EntityPtr及委托回调继续清空。必须继续拒绝旧header变化、owner／租用数不足或溢出、窗口撤销；真实容器替换不得误入纯借用路径。故障发生在借用后仍由原done日志减少租用，保留前一安装所有权。第15步通过装备变化、角色重建、安装后故障和清空／停止续验，不另跑隔离单元测试。

包112将普通字段顺序与特殊处理器分类分离：ValueOrder仅由未被Covers接管的通用字段调用；默认实现保持Order，NativeBindings继续核Room／Level原生前缀，其余None／FXCount固定为原顺序4。委托字段仍用完整Order。必须防止特殊字段误走普通入口、Room初始化前缀后移或自定义绑定器顺序丢失；第15步实际角色／装备变化与安装后回滚续验覆盖。

包113合并纯写入的形状检查及SEH叶：先比较，变化时检查同一可写区并完整复制撤销原值，再写Bytes或保留mask外位的Masked32。结果分Unchanged／Written／Rejected／CapturedFailure；后二者区分原值未取得与可能部分写入。任何完整原值之后的失败都让安装器立即登记回滚责任，调用返回前没有回调或等待。必须继续正确处理非有限来源以外的地址／页错误、跨页失败、部分memcpy、masked外位与安装后取消；真实页异常未实际构造时不得声称已动态覆盖。第15步沿用安装后故障、装备变化和停止排空验证总体撤销路径。

包114进一步核对ClientStagedFieldAttach成功路径：AttachmentRegistry只用ClientStateMemory::Allocate新增std::map节点，不擦除旧节点；新旧header都留在字段保管区，旧承载直到独立提交／退休才处理。Records的ElementAddress只是已构造list节点访问，DequeOwner只改自有代理指针，Current仅读窗口／header／武器／shared事实；没有原生setter、析构或释放现有承载。因此InstallNow对所有自有FieldAttachment成功安装保留页见证，实际Shape／可写范围／Current／CanAttach／header读回及done时点不变。分配失败或安装失败仍立即退出该批，不继续借用页见证；EntityPtr及委托回调继续清空。若以后在Attach加入释放或原生回调，必须恢复该调用边界。此项已在第15步run122通过新旧容器替换、安装后失败恢复和停止排空续验；整体性能仍未达标，不是放宽原生承载资格。

包115依据run122计时外采样（2394样本，暂停最大9.34ms）继续缩减回读索引构建成本：原来稠密键仍先push_back完整表，再分配同规模临时表进行稳定计数排序；现在按已验证的costume／stream／record／block映射直接计数，再按原操作序号散布到最终表。完整64位键、同桶操作顺序、FieldAttachment索引和逐128项窗口检查保留。必须处理的失败方式包括稀疏桶超出现有整数工作区、扩展stream或block、构建中窗口撤销及Peer内存不足：前三项分别回退完整键字节排序或终止，分配继续由原预算分配器处理；不减少原生写入、回读或冲突检查。只在描述期构建，安装／回滚期间不新增分配。等待第15步续测。

包116拟消除NativeBindings::DescribeFields对全体Values的重复扫描：后台Node生成数字叶时顺带登记原消费谓词选中的资源／实体引用及AnimationState两组选择器，主线程只访问这些下标。先列失败方式：不能漏掉空引用、空选择器容器或事件叶；不能把layout节点当叶；不能在scratch扩容重试时保留旧下标；不能改变原值序或混淆同一路径的World／Body记录；预算失败必须终止候选。索引只存本计划Values下标、使用原ClientStateAllocator计费，与Decode已有游标同步回卷；主线程仍保留原筛选、record/path/field查找、引用唯一性、选择器数量及资源实际承载校验。引用索引的游标只在已排序Bindings内单调前进，精确等值检查保留。

包117拟去掉Host两处冗余操作，保持所有窗口检查：Track首次未命中后Read只执行当前读段内的受保护复制，不改变证言表，因此复用已找到的插入位置；Remember独立入口仍完整查重及比较。失败方式包括Read失败、空地址／超宽／表满和重复证言值变化，原拒绝及根回卷保持。PutLinks仍先检查引用类别、句柄、数量和9B总容量，仅将小端64位句柄逐字节移位改为固定8B memcpy；显式静态断言小端，避免把宿主字节序当成普遍协议假设。两个改动都不复用跨窗口的来源或省略动态复核。

包118拟优化后台纯值计划的path_index／normalized_index／field_index：核对所有用法后，三表只使用精确find、emplace及scratch失败按原下标回卷，不消费键排序；计划顺序来自解码顺序和各向量，不能改为遍历散列表生成输出。失败方式包括不同完整键碰撞、rehash后的迭代器失效、重试后重复／遗漏路径和Workspace分配失败。采用完整tuple相等比较区分碰撞；没有迭代器跨同一表插入保存，失败回卷保留原erase循环，所有节点及桶继续经同一PMR预算分配器计费。循环／程序定义／overlay三张小表仍保留原有序实现。该变更仅后台索引，不触及原生地址或主线程安装资格；需要实机查看频率、后台plan_slice和Workspace峰值。

包120拟为已定界原生字节复制／比较增加1、2、4、8、16B固定宽度分派：仍使用memcpy取得未对齐整数，避免把字节地址强转为有对齐要求的对象。必须保留零长度、非标准长度、跨页失败、未对齐地址、原值完整捕获之前／之后失败的区分，以及Masked32只恢复mask位的语义。所有窗口、范围、可读／可写页和SEH保护保持；大块及其他宽度走原memcpy／memcmp，不调整实际读取区域，也不假定来源与目标对齐。优化只减少运行时通用CRT调用，需第15步实机计时与安装后故障／回读／退休续验；未实际构造的页异常不标动态覆盖。

包121在包120尚未启动前发现NativeBindings另有两个全Values扫描，仅消费OverlayDisplayState(442)、ProceduralDisplayDefinition(444)、ProceduralDisplaySet(445)。既有PresentationValues不包含这三类普通记录，不能直接替换为该列表。为三类建立独立NativeDisplayValues，仍保留原值序、事件节点及原完整检查，并纳入Decode scratch重试回卷和Client Values记账。失败方式包括漏全局overlay、错程序定义顺序、混用其他消费者的布局范围及失败重试重复节点；主线程原controller唯一性、负ID顺序、资源和文字校验不变。此包连同120进入同一第15步续验，不声称120已单独实机通过。

包122针对run127中CanonicalField槽数据读取仍占实际PC热点，拟给对象独占段增加稠密路径模式：仅当max_path-min_path+1不超过该对象字段数两倍时直接用path-min定位；否则保持原半空散列表。计数全部在解码回卷结束后完成，空对象零槽，缺失路径的空槽返回none，重复完整键仍保留第一字段，World／Body的对象归属不变。失败方式包括空区间下溢、路径边界越界、字段重复导致错误覆盖、总容量溢出及预算不足；均保留显式检查，稠密模式槽数不大于原散列表，不增加原生缓存或读取资格。需第15步继续覆盖结构变化、安装故障及停止排空。

run128中包120／121组合未改善整体表现；按每次安装原始操作归一，native_write／native_publish亦未见收益。静态反汇编确认小宽度最终是固定加载／存储，但Copy帮助函数没有内联，其他长度还增加一次分派；不能仅凭指令形态认定更快。包122撤回包120的small_bytes帮助函数及调用替换，恢复原memcpy／memcmp；保留包121三个显示布局节点的稀疏消费。上述对比不是严格同负载A/B，不将回退结论外推为所有机器固定宽度都更慢。

### 14.17 H1／H2 实施（包123／124）

包123 以 Manager 帧为可读区寿命。H 的拓扑批次和各段证言继续使用原身份复核。包124 保留 J 的 Player 同根共享与 K 的 Body 克隆，两条普通流独立发送。根复核失败按对象回卷或标记沿用，快槽过预算时跳过 Body。见[第 10 步实施记录](10-host-capture.md#2026-11-17-h1-和-h2-实施记录)。

### 14.18 H3／H4 实施（包125／126）

包125 在 Tear 统一采集读取处记录实际输入，包括动画、图层、容器和引用依赖。热命中复用后台已经接管的普通根，窗口或依赖改变即重新读取。包126 在资源句柄索引之前复核精确输入后复用解析结果，并将主线程 CPU 计时调整为每 31 帧抽一帧。实际命中、帧耗时、300 静止泪弹及仅改变 Sprite／附件内容的覆盖均留给第 15 步。详见[第 10 步实施记录](10-host-capture.md#2026-11-17-h3-和-h4-实施记录)。

### 14.19 H5 实施（包127）

普通、快 Body 根使用入口资格与一次根尾证言复核，普通候选的提交只保留全局窗口及发布锚点检查。失败风险包括根指针失效、外线程写入后恢复原字节、跨帧缓存命中旧来源、组绑定或生命周期撤销，以及局部复制失败被误判为整段失效。入口资格、局部回卷、原生页保护、根尾与发布水位分别覆盖这些路径。字节证言仍复读，遍历时的指针、容量和类型校验保留；外部写水位变化使旧缓存证明失效；失败根的新资源目录与缓存一起回卷。详见[第 10 步实施记录](10-host-capture.md#2026-11-17-h5-实施记录)。
