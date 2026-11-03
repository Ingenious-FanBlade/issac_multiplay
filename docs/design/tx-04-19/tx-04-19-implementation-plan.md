# tx-04-19：非 Player Entity 客户端应用具体实现计划

日期：2026-11-03，Asia/Singapore。状态：**I01—I03已实现；I04静态合同见[采用合同](entity-application-contracts.md)、[J273专题](entity-native-static.md)与[研究索引](entity-native-contracts.json)；I05—I09见[公共实现](tx-04-19-common-application.md)；I10—I14的资源／类别值／关系／正常生命周期实现见[类别实现与明确边界](tx-04-19-class-application.md)，必要逆向与主审见[本轮静态研究](entity-i10-i14-native.md)。I15—I35及统一E2E待推进。** 未解释NPC私有容器／拥有的actor及具名复合draw分支仍写前拒绝，由I25／20补齐，不冒称全形态支持。没有默认放行尚未注册的类别adapter，没有设置完整Entity提交位。已有P1—P6发送优化完成实现与纯编译，运行验收并入本tx；I04登记的来源／owner／分支缺口随实际adapter闭合，不把研究或公共代码当成全类别native支持。

初版文档工作的完整目标是列出 tx-04-19 的全部实现、必要静态研究、工程接线与整体验收任务。本文是执行入口；[逐类别应用计划](tx-04-19-entity-application.md)与[静态确认及必要逆向清单](tx-04-19-native-research-plan.md)是同一计划的组成部分。I01—I35、C01—C13、R01—R13、E01—E26 只是同一 tx 内的内容索引，不拆成新 tx。

执行依据：[现行项目规范](../../refactor/project-conventions.md)、[完整路线图](../state-synchronization-roadmap.md)、[状态同步设计](../state-synchronization.md)。不设 gate，不校验哈希；初版只编辑文档，后续 I01—I03 已实施，I04按用户指示由一个GPT-6 Luna max子智能体执行必要逆向，主智能体独立复读源码／指令并纠错。后续实现不提前执行分项测试，统一 E2E 仍归 I34。

## 1. 完整目标与范围

真实 Client 使用原生 Entity 承载 Authority 的完整非 Player 目标状态。交付 NPC／Delirium、Tear、Projectile、Bomb、Laser、Knife、Pickup、Familiar、Effect、Slot、NativeText、NativeDummy 的类别级应用合同，覆盖 create、change、remove、Morph／本地替换、引用、资源与姿态、派生绘制、短命结果和部分失败退休。未知对象不降级成 NPC，不通过过滤活对象获得“完整”。

同时交付以下横向能力：

- 从正式观测到发送投影、纯值目标、原生应用及只读后态的完整字段映射；必要缺口在本 tx 补到正式来源，不能只由测试夹具填值。
- 复用唯一稳定身份、Room／Grid owner、ClientScene 和资源服务；接通 18 的 `VisitLifted`／`ResolveReferences`，完成 Effect136 的举起 Grid 描述应用。
- 逐类隔离 Init、Morph、remove、draw 中的生成、随机选择、伤害、拾取、奖励、Lua gameplay 与访客持久副作用。客户端安装结果，不重演裁决。
- 完整回归 18 的 Room／Grid 场景，并将 P1—P6 的字段省略、图闭合、姿态、版本拒绝及实际体积纳入 19 的同一整体验收。

边界如下；边界不用于推迟 19 自身必需的前置。

| 相邻工作 | 本 tx 必须完成 | 后续责任 |
| --- | --- | --- |
| 17 公共运行时 | 扩充已存在对象／资源／展示路径，解决新增类别暴露的公共缺口 | 不重建第二套 Scene 或第二套运行中的世界 |
| 18 Room／Grid | 同一目标的跨域引用、举起描述、分层绘制及完整回归 | 既有 Room／Grid 行为继续由其 owner 管理 |
| 20 Player | 保留真实 Player 前置；Entity 引用通过稳定 Key 和具名 Player 接口接入，禁止写死 owner=首玩家 | 四参与者、多身体、装备和角色完整状态应用仍归 20；19 不冒称覆盖 |
| 21／22 下行与恢复 | 可接收完整不可变 T 的正式应用链、准确提交结果、取消和重建；模型 B→T 与原生 A→T 分开 | 真实四端实时 Baseline、Delta 传输与网络 Applied ACK 归 21／22 |
| 23—29 整局语义 | 当前场景生命周期与全部类别合同；已采用路径的必需特殊分支当期解决 | 跨房延续、全部路线／变体组合、通用瞬时音画与完整 HUD 由相应 tx 扩展；具体缺项列名交接 |
| Mod／随机 | 官方 REPENTOGON 扩展的对象寿命和必要表现；观测已有结果 | 不宣称任意 gameplay Mod、客户端玩法 RNG 或自动捕获私有闭包 |

## 2. 当前基础与已确认的源码缺口

以下为本轮直接代码阅读或既有记录，均非新运行结果。

| 事实及位置 | 对 19 的影响 |
| --- | --- |
| [10 的 Entity 观测](../observation-expansion/tx-04-10-implementation.md)、[逐成员目录](../observation-expansion/entity-catalog.md)已经存在 | 复用完整目录、正式采集和身份；可读 Getter 不自动等于安全写回入口 |
| [17 对象生命周期](../tx-04-17/tx-04-17-native-object-lifecycle.md)与 `mod/src/game/client_objects.*` | 当前 `Object`／`ParseObjects` 只接受 `tx-04-17-representative-v1`、一个普通 Isaac 和 Effect57，非 Player 完整字段并未支持 |
| `mod/src/game/client_objects_native.cpp` 的 `Supported`、`MemberSpace`、`MemberMatches`、`WriteFields`、remove 路径 | 构造、成员归属及状态核对只解释了代表对象；所有非 Player 放进 mainEL、把 dead 一律视为错误、拒绝非空附属 owner 的规则都不能照搬成全类别合同 |
| `mod/src/game/client_presentation_native.cpp` 的 `Members`、`DrawEntity` | 当前要求一个普通玩家，其他对象只认 Effect57，直接分派将非 Player 转成 Effect；对象上限 512、绘制上限 256、计划上限 4096 也需要统一安排 |
| `mod/src/game/client_scene.cpp` 的 `Pump`／`AdmitTarget` | 当前先完成 objects，再完成 grids，最后只补 Grid→Entity 引用；新增 Entity→Grid 或构造期 owner 依赖需分阶段编排，不能仅在末尾补指针 |
| 同文件的 `committed_domains` | 当前只设置 Room／Grid 位。Entity component 同时含 Player，19 只能声明准确的非 Player 应用范围，不能直接设置“完整 Entity 域已应用”后让 21 误 ACK |
| [18 统一验收与 I30 接口](../tx-04-18/tx-04-18-acceptance.md) | Source 49/49、冷 Client 49/49、12/12 检查是 18 历史结果。Effect136 留给 19；旧测试为准备 Room/Grid 而清理 Entity 的做法不能移作 19 的类别覆盖 |
| [发送优化 P1—P6](../replica-redundancy-reduction.md#10-2026-11-03-前置优化实现) | 模型／目录 v2、`replica-projection-v2` 已存在；201 行参数查询不发送，新增消费者必须逐项复核，不能默认永远不用 |
| `replica/model.cpp::Capabilities` 与 `game/world_sources/observation.cpp` | 能力列表出现 `NativeDarkness`，实际补充来源使用 `NativeDummy`。I01／I02 核对并统一真实语义，不直接把两个名称当别名 |
| `ops/client_fixture.cpp`、`tools/client-fixture/main.cpp` | 当前 Entity 字段白名单服务 17 代表投影；必须增加 19 明确范围，不能继续删掉待应用字段或活对象 |

正式环境沿用官方 REPENTOGON 1.1.3 与 J273。J460 点位只能作为检索线索；本计划未新增可执行原生地址或宣称任何新的 ABI 已核定。

## 3. 数据、生命周期与提交合同

### 3.1 目标和所有权

`B` 是模型重建使用的基线，`T` 是完整不可变目标，`A` 是最后真正原生提交且仍有效的状态。对象计划始终从 A→T 产生；不能把上一包 Delta 直接当原生操作序列。相同 scope／revision 的相同目标幂等，相同版本不同内容拒绝，旧 scope／旧目标不复活对象。

`EntityTarget`由I02落地；I05—I09已落地`EntityApplyPlan`和`ClientEntityClassAdapter`公共接口。逐类adapter实现仍归I11—I22：

| 值／接口 | 内容与责任 |
| --- | --- |
| EntityTarget | 完整 scope／revision、来源和能力版本、成员完整性、稳定 Key、shape revision、分类、公共／派生值、资源、引用及字段有效性；纯值无原生地址 |
| 类别 adapter | 解析要求、构造前置、字段安装、引用、辅助资源、缓存／draw、退休责任及具体适用分支；共享公共能力，专用语义显式处理 |
| 原生实例表 | 同一 Key 对应当前实例代次、owner／池 use、发布状态、已执行前缀、资源借用和失败原因；原生指针仅主线程、当前用途借用内有效 |
| Player 外部依赖接口 | 从共同身份表解析当前 Player Key／实例／寿命；19 不创造第二个 Player，也不重用 Authority 身体指针 |
| 跨域依赖表 | Entity／Player／Grid／Room／descriptor 分类型引用，区分构造前置、可后置关系、描述值和历史身份 |
| 应用结果 | attempt、scope、revision、实际应用范围、key／类别／字段／阶段、已写前缀、旧展示资格、退休进度和恢复方向；不发送网络 ACK |

`Null`、合法空容器、字段缺席、NotApplicable、NotScheduled、Unknown／Unavailable／ReadFailed／Truncated 及明确删除分别解释。只有明确完整、范围一致的活成员集合可以按缺席删除；部分来源不转换为删除表。`Exists=false`、dead-but-existing、已退休、未完成创建须分别建模，不能全都跳过或全都拒绝。

非 Player 子范围必须从完整 Entity 成员表按已确认分类显式派生，并保留原成员数、Player 成员及其由谁处理的关系。不能把不支持的活对象过滤掉，再把剩余集合改标 Complete；当前 Player 代表字段投影的范围与完整非 Player 字段范围分别声明。原始来源 coverage、目标范围完整性和实际应用完成范围是三件事，任何一项不能替另一项背书。

字段有四种处理：安装到已理解的原生字段／安全 setter；保留为纯值而不让本地玩法消费；由完备输入通过已解释的纯表现算法生成；由具名后续域负责。后三种必须给出字段级理由，不能借“无 setter”静默丢弃。操作类 API 不变成网络 RPC；但经核定的安全底层 setter 可用于安装最终值。

### 3.2 场景执行次序

1. 解析完整目标和全部域依赖，预检资源／分类／预算／Player 前置；写前失败保留有效旧 A。
2. 根据 A→T 计算增删改／替换／关系撤销及构造依赖。先断开会引用已退休实例的关系；准备 Room 几何和已有 Player owner。
3. 在构造依赖允许时取得各类原生实例与 Grid owner，记录真实进入前缀；Factory 分配、派生 Init、官方扩展、成员发布分别处理。不能假设所有类都能先无 owner 初始化。
4. 构造必需边先解析；普通 Parent／Child 等可后置边在相关实例齐全后绑定。合法关系环用两阶段安装，构造期互相依赖的环必须有具体原生解决路径，否则写前报告依赖缺口。
5. 安装公共／派生最终值、资源及辅助成员；完成 Entity↔Grid／Player 关系和 lifted 描述。依赖 Init 次序的字段按各 adapter 明确的顺序覆盖。
6. 完成必要排序／缓存／分层绘制准备和各域只读后态检查；同一 attempt 全部成功后由唯一 Scene 提交 A 与展示资格。
7. 失败按实际前缀退休。写入后不宣称回滚旧 A；停止半套展示，保留 Client 隔离，安全清理后从完整 T 重建。未知原生异常不盲目重试析构。

所有游戏／Lua／图形工作均在已有主线程阶段及有效 context 内执行。网络及后台只处理自有不可变值；不等待网络或磁盘来推进游戏线程。Hook 仍由唯一清单启动安装、常驻至退出，会话逻辑启停，Off 透传。

## 4. 全部任务索引

I01—I14进度与边界见上方链接，I31所承接P1—P6有前置实现；后续按I15—I35推进，并在I25／20闭合具名NPC输入／draw缺口。下表保留完整目标索引，代码完成与运行覆盖分开，运行验收统一留在I34。

| 编号 | 任务 | 依赖／专题 |
| --- | --- | --- |
| I01 | 已实现：全成员应用目录、分类与责任映射 | 10／15 目录，C01—C13；运行验收归 I34 |
| I02 | 已实现：纯值目标、发送投影与能力版本 | I01，R01；不宣称原生应用 |
| I03 | 已实现：正式只读来源与表现数据补齐 | I01；采用路径的完整原生研究仍归 I04／各 adapter，运行验收归 I34 |
| I04 | 已形成静态采用合同、必要逆向及主审；具体未闭合分支不虚报支持 | R01—R13；专题逐条登记后续adapter／来源／清理责任 |
| I05 | 已实现公共注册与按Key的Player owner；完整Player形态仍归20 | I02，R01／R02 |
| I06 | 已实现A→T阶段计划、取消／存在性及版本语义 | I02／I05 |
| I07 | 已实现工厂／Init／官方链／成员发布公共执行器；具体类别按I11—I22注册 | I04／I05／I06 |
| I08 | 已实现公共最终值、字段级去向与真实native后态 | I01／I03／I07 |
| I09 | 已实现分类型依赖／图／公共EntityPtr；专用绑定由各类别owner接入 | I05—I08，R02 |
| I10 | 已实现主／辅助资源与完整姿态；真实运行覆盖归I34 | I03／I07，R10 |
| I11 | 已实现NPC／Delirium值、双ANM与具名关系；私有draw／owned actor拒绝清单见实现说明，I25／20继续闭合 | C01／C02，R03 |
| I12 | 已实现Tear字段、三份辅助ANM、sticky与内部引用、正常退休；draw调度归I25 | C03，R04 |
| I13 | 已实现Projectile独立布局与正常生命周期；draw调度归I25 | C04，R04 |
| I14 | 已实现Bomb字段、五层costume与启用位、正常生命周期；draw调度归I25 | C05，R04 |
| I15 | Laser／HomingLaser | C06，R05 |
| I16 | Knife | C07，R06 |
| I17 | Pickup | C08，R07 |
| I18 | Familiar | C09，R06 |
| I19 | Effect | C10，R08 |
| I20 | Slot 全部现有变体解释 | C11，R07 |
| I21 | NativeText | C12，R09 |
| I22 | NativeDummy 及明确未知分类 | C13，R01／R09 |
| I23 | Morph、替换、死亡展示与删除 | I06—I22，R02／R12 |
| I24 | Effect136／Grid 双向交接 | I09／I19，18，R08 |
| I25 | 各类原生绘制与必要缓存 | I10—I24，R11 |
| I26 | 短命主体、独立 scope 与结果事件 | I05／I06／I23 |
| I27 | 玩法／Lua／RNG／持久隔离 | 全部采用路径，R13 |
| I28 | 部分失败、在途借用与安全退休 | I07—I27，R12 |
| I29 | 唯一场景提交、准确范围与恢复 | I05—I28 |
| I30 | 有界预算、稳定缓存与必要诊断 | I02／I25／I28／I29 |
| I31 | P1—P6 新消费者复核与验收合入 | 已有优化，I01／I03／I10／I30 |
| I32 | 构建接线和产品／测试边界 | 新增模块清单 |
| I33 | 完整 E2E 夹具与统一编排 | 第 7 节失败模型、第 8 节矩阵 |
| I34 | 全目标统一 E2E、修复与续测 | 全部实现及 I33 完成 |
| I35 | 结果记录、索引与后续交接 | I34 实际结果 |

## 5. 公共与逐类实现任务详情

以下是代码职责落点，新增名称为计划，不表示文件已存在；沿现有模块扩展时保持同一职责即可。

| 工作 | 现有入口／建议新增落点 |
| --- | --- |
| I01— I03、I31 | `mod/tools/gen_entity_catalog.py`／`gen_replica_catalog.py`／`replica_projection_policy.py`；正式 `mod/observation/isaac-cs-observation/domains/entity`；建议新增 `mod/tools/gen_entity_application_catalog.py` 与 `mod/src/replica/entity_target.*` |
| I04、I27 | `mod/binding/hooks.json`／`world_layout.json`、`mod/src/hooks/registry.cpp` 及既有用途分派；研究结果不另设运行 Hook owner |
| I05— I09、I23 | `mod/src/game/client_objects.*`／`client_objects_native.*`；建议新增 `mod/src/replica/entity_plan.*`，复用现有 Key／实例表 |
| I10— I22、I24 | 现有对象执行器分派至建议的 `mod/src/game/entity_adapters/` 类别模块；复用 `client_resources*`、正式 `game/world_sources/lua_entity.*` 和 `client_grids.*`，不把每类再写成独立 Scene |
| I25— I30 | `client_presentation*`、`client_scene.*`、`client_native_world.*`；按需增加 Entity 独立记录／预算职责，不复用 Grid 事件队列冒充 Entity 域 |
| I32— I34 | `mod/CMakeLists.txt`、`ops/client_fixture.*`、`tools/client-fixture/main.cpp`、现有 harness／测试 Lua／编排入口；19 新增文件与 18 场景复用见 I33 |

### I01：全成员应用目录、分类与责任映射

以 `entity-catalog.json`、正式 `domains/entity/schema.lua`／`collect.lua`、15 复制归属和 P1—P6 发送清单为输入，逐项覆盖公共、派生、special、关联容器、资源、参数查询与未公开需求。功能目录 `docs/design/tx-04-19/entity-application.json` 已由 I01 生成，每行含源成员／版本、适用类别与变体、应用责任、目标字段、本地承载、依赖、缺失语义、源码落点及 E2E 对应。继承成员只定义一次，别名保留来源映射。

同时核对 NativeDummy／NativeDarkness、Type 与实际派生类、Player 共有字段边界和 Slot 变体表。不能按 Type 数值范围粗分 NPC／Effect，也不能从“目录有行”推导运行覆盖。完成时每个相关成员均有明确去向，未实现项保持可定位；I11—I22 不得绕过此目录。

### I02：纯值目标、发送投影与能力版本

在 `mod/src/replica` 增加领域目标／解析职责，扩展现有 `ClientObjectDescription` 或拆成公共头与分类 payload，避免继续堆成包含所有类的无适用性大结构。处理 signed／unsigned／bitset128、稀疏集合、typed int64、图节点、引用、资源、状态占位及字段清空；校验范围来自具体字段语义，不能用同一套非负检查覆盖所有类。

保留模型 v2 既有语义；新增来源／应用合同具名版本，若确有不兼容语义再同步提升对应版本。能力声明区分“模型可表达”“非 Player 可原生应用”“Player 代表范围”和“实机覆盖”，统一分类名称，旧必需合同明确拒绝。完成时同一正式 T 可供夹具和未来网络共同解析，无测试专用替代协议。

### I03：正式来源与表现数据补齐

沿 `domains/entity/collect.lua`、`game/world_sources/lua_entity.*`、必要 native supplement 扩充正式只读来源。按逐类 draw 实际读取补辅助 Sprite、完整曲线／几何、颜色／状态效果、Text 表现、当前派生缓存等；公开 Getter 优先，必要 native 读取记录来源版本、适用性、初始化资格和原生 owner。

普通完整窗口、Render、Lifecycle、ActualCall／Query 保留不同 scope 与依赖；现有每 30 次 render 回调采样不能无说明视为逐目标精确姿态。必需姿态应在合法正式窗口获得，或明确依赖对应独立样本，不能拼造原子快照。禁止额外 `CalculateSamples`、LootList、带随机的 GetBombFlags、GetData 建表或推进动画取值。完成时关闭录制仍能供应所有本 tx 必需值。

### I04：静态确认／逆向

执行[研究清单 R01—R13](tx-04-19-native-research-plan.md)。先复用本地 Wiki、官方 wrapper／ZHL／SDK 及 17／18 已核定事实；公开语义明确的读成员不逐项逆向。具体写入／构造／draw／清理存在疑问时才读 J273 对应调用链、原始指令和虚调用分支。结果直接更新功能目录与专题，不另做 tx artifact。

完成时每条采用的原生路径可说明 ABI、前置、owner、允许副作用及失败后责任；未解释路径不落猜测性写入。静态结果保持静态标签，不当作实机成功，不运行探针或分项测试。

2026-11-03执行结果：公开转换／存储宽度、Factory各池／发布前缀、引用分类、NPC/Delirium/Morph、各类资源和表现、Slot18变体、Effect136描述、Text/Dummy、分层draw、部分失败与持久整链均已形成具名合同，详见上方专题。主审纠正了Pickup Render的exists/visible门值与dropDelay名称、Slot setter对全部variant先写标量、GetHitListIndex的特殊parent类型，以及“Render一定纯读”的错误假设。没有启动游戏、构建或执行测试；仅重新生成功能目录，不产生tx artifact。

### I05：身份与 Player 边界

复用 `ClientObjectKey`、正式 Subject 与实例代次，扩展注册表的类别／池 use／借用资格。Key、shape revision、原生地址、InitSeed、Index 和实例号分别管理；相同地址重用只产生新 use，不恢复旧引用。

把 Player 前置从非 Player adapter 中分开，由现有真实 Player owner 提供显式解析接口，保留 20 可替换接入点。19 测试允许现有普通 Isaac 作为 owner，但纯值结构与查找不能固定一名参与者或首 Player。当前不支持的 Player 形态明确拒绝，不能制造假身体满足引用。完成时所有 owner／parent 引用都按稳定 Key 和当前实例解析。

### I06：A→T 计划与存在性

在 `client_objects.*` 或新增领域 plan 中表达创建、字段改变、资源改变、关系改变、同 Key 本地替换和删除；完整目标回到基线的值、清零／空数组／Null 都要生成正确安装动作。处理 no-op、同版本重复／冲突、旧 revision、取消未写候选与已写候选的不同责任。

明确 `Exists=false` 但尚在 EL、dead-but-existing、完整成员缺席与真实生命周期事件的关系；不能沿用 18 为 Room/Grid 夹具使用的过滤规则作为生产 Entity 语义。完成时重建和增量应用共用同一存在性合同，写后取消转退休，不重放已执行的 Init／remove。

### I07：构造与成员 owner

按每类核定的工厂或专用构造获得实例，不覆盖存活池项；逐步完成基类、派生 Init、官方 EntityPlus／ESSM、资源、成员发布。登记进入前缀早于任何可能失败的原生调用，构造成功与成员可见性分开。部分类型需 owner／Player／Grid 前置时按依赖表提前供应。

列清主／持久／render／update 及专用列表归属、flags 条件、缓存失效和初始化时连带创建；连带对象必须阻止其玩法生成或按已核定的原生构造职责接管并对应唯一目标 Key，禁止再重复创建。完成时实际原生成员与目标集合一致，不能通过删掉异常成员掩盖未解释的构造副作用。

### I08：公共状态与后态

覆盖形态、位置／速度／偏移／高度相关公共量、大小／质量、碰撞参数、血量／死亡／存在、flags／计时、颜色／可见性、渲染顺序及公共关系。逐成员选择直接安装、安全 setter 或纯值保留；安装 HP、状态效果和死亡表现不调用 Damage／Kill 重做世界结果。

移除代表实现对所有对象 `_dead=false` 的假设，适用字段只有来源有效才读写。后态从实际 owner 读取应用语义，不能比较两份 DTO 代替原生检查；源端 Index／实体地址不写进本地索引。完成时每个公共字段有目标、写入和检查路径，字段不适用有明确原因。

### I09：引用与依赖

扩充现有 Parent／Child／Target／Spawner 四引用之外的 owner、attachment、follow／orbit、NPC 部件、Pickup 关系、Laser 链及 Grid 描述关系。实体引用使用已解释的 `EntityPtr`／反向登记机制，不按裸指针复制；其他关系按实际类型和 owner 管理。

预检所需目标、实例类型及适用 scope，区分可后置环和构造前置。目标迟到时等待完整依赖，不临时绑定首玩家；Null 解除引用，删除／Morph 先撤旧实例关系，再绑定新实例。完成时没有跨实例或跨场景悬空关系，必要外部依赖未就绪时不提交半套目标。

### I10：主／辅助资源与完整姿态

复用 `client_resource_model.*`、`client_resources*` 的候选、真实成员借用及 context 寿命。覆盖主／overlay、逐层变换、颜色／blend／shader、贴图替换、精确 phase、辅助 ANM2 和可空成员；Tear 三份辅助 Sprite、Pickup 价格、Slot 奖品、Bomb costume 不能按路径相同合并。

资源身份、内容描述、对象实例与 ANM 内部资源代次分开；Morph 即使 this 不变也撤旧层借用。确认 Getter 返回真实成员还是值副本，沿用 18 对 ExtraSprite 的教训。完成时加载失败不发布半套资源，取消／替换等待在途 CPU 使用结束，不能 Reset 或 delete 不属于本方的内嵌资源。

### I11—I22：全部类别应用

这些任务逐项按[类别计划 C01—C13](tx-04-19-entity-application.md)实施；每项均包含完整字段／容器、构造、引用、资源、draw、变化和退休，不能只实现生成或基本 Sprite。对应关系为 I11=C01+C02，I12=C03，I13=C04，I14=C05，I15=C06，I16=C07，I17=C08，I18=C09，I19=C10，I20=C11，I21=C12，I22=C13。

每类完成时功能目录无未解释的本 tx 必需成员，正式来源可供完整目标，具体适用分支和未决项可定位；实际观察是否通过只在 I34 判定。类别专有 UNKNOWN 不因公共 adapter 编译成功自动消失。

## 6. 集成与收尾任务详情

### I23：Morph、替换、死亡展示与删除

按 source Key／shape revision 的实际语义决定原位更新或本地实例替换；资源变化不必一律重建，类型／派生布局变化不盲目原位覆写。优先用已解释的实例替换保留逻辑 Key；若原位 Morph 有必要，单独核定所有随机／生成／PGD 和资源副作用。

死亡动画仍存在的对象按状态保留；最终删除采用不触发战斗结果的退休路径。逐类处理附属 owner、反向引用、官方扩展、所有 EL／类型查询缓存和池归属；不能用调用 `Die`／爆炸／掉落替代删除。完成时 repeated remove 幂等、同地址新实例不串号，当前被 `+0x310` 非空拒绝的真实类别有具体处理而非取消检查。

### I24：Effect136 与 Grid 双向交接

消费 18 的 `ClientGrids::VisitLifted`，核对 Effect Key、完整可空 GridDesc 和来源 scope。官方 `LuaEntityEffect.cpp::Lua_GetGridEntityDesc` 只在 variant136 把 `_varData` 解释为描述；必须按专用 payload 处理，不能既写通用 VarData 又覆盖同一内存。

明确描述值不等于活动 Grid 对象，`source_grid=Unresolved` 不伪造旧格引用；举起物自身只是一份 Effect。落下／销毁由后续目标中的 Effect 删除与真实 Grid 创建表达，不在 Client 执行碰撞或 Throw／Spawn 重演。完成时两域对同一描述无冲突，Entity↔Grid 合法依赖可在 Scene 提交前闭合。

### I25：各类原生绘制与必要维护

替换 `Members`／`DrawEntity` 的代表限制，按实际类别调用已核定原生 draw，保留 Z 排序、阴影、light、透明／水面／镜像／前后景 pass 以及 Grid 柱顶交错。补足 Laser 曲线、Knife 挂载、NPC 部件、Pickup 价格等实际消费缓存；不得把所有类转 Effect 或用单 Sprite 代替必要表现。

采用窄维护函数或安装 Source 最终缓存，不补跑 Entity／Room Update、碰撞插值、动画事件或 gameplay Lua。draw 的临时字段在当前借用结束时恢复，异常时停显并走 I28。完成时未生成新的玩法对象、目标姿态不随本地模拟漂移，17／18 的 FBO／depth／队列次序保持正确。

### I26：短命对象与独立 scope

处理完整 T 中已不存在、但生命周期中创建后移除的对象，不补造其常驻实例，也不因独立事件缺主体而损坏当前目标。若短命结果形成下一目标中的 Effect／碎片／奖励对象，按它们自己的 Key 正常应用；同采样无后态的纯瞬时表现保留事件语义及 27 的消费责任。

独立 Render／Query／Lifecycle 记录携带 source scope、subject／实例要求、required state 和序号，过期／重复／缺依赖有界处理。只完成本 tx 真正消费的状态／姿态，不能因 Scene 提交确认所有事件或推进网络事件水位。完成时没有幽灵对象、旧 Morph 姿态回灌或用丢弃事件伪装播放完成。

### I27：玩法、Lua、随机与持久隔离

对 I07—I25 采用路径逐项核对普通／odd／FSM／加载／构建／draw／退休调用，按现有 Client 用途分派接入唯一 Hook 清单。隔离伤害、拾取、掉落、抽奖、道具使用、AI、计时过期、链式 spawn、动画事件和 gameplay 回调；保留必要官方扩展维护。观测 wrapper 的 original 保持 exactly-once；用途分派主动跳过玩法调用时明确记录为未调用，不能伪造原调用完成。

NPC Morph bestiary、Slot 捐款／奖品、Pickup 收集／图鉴、构造音效与 remove 回调列具名处理。访客隔离覆盖准备之前至安全菜单交还及相关最后 writer，Host／Off 原链不受影响。完成时无需本地重新随机即可安装结果，持久隔离不能只靠最终 dirty=0／文件未变来证明。

### I28：部分失败与安全退休

按 R12 为分配前、已取得池 use、Init 中、已发布、资源部分安装、引用部分绑定、draw 在途分别列清 owner 和可执行清理。取消未写候选可保留旧展示；首次原生写入后撤销 A 有效性，停显、停止相关输入／publication。先结束在途使用并撤销依赖引用／成员别名，再按真实 owner 的依赖逆序退休对象与资源，最后收口 Grid／Room；具体原生 Remove 所包含的步骤依 R12 保持正确次序，不能先释放被引用对象再补清指针。

未知 EH、context／VM 丢失、清理前缀不确定时保留 owner 和隔离，返回具体不可恢复原因，不能重入 Free 或强切 Off。正常重建重取所有借用、用完整 T 恢复。完成时结果保留首因和真实未释放责任，旧实例与旧回调不复活；自有 virtual device 的 NEVER 策略不变。

### I29：唯一提交及准确应用范围

将对象／Grid 计划拆成准备、构造、后置引用、缓存检查等可调度阶段，消除现有“对象完整提交后才允许 Grid 创建”的互相等待。预提交枚举必须具名限定本 attempt，不能把半完成对象暴露为已提交 Visit。所有域正常收尾后只有 `ClientScene` 更新 A；子 adapter 的准备完成不能成为对外 Applied。

在 `ClientApplyResult`／能力／状态输出表达“非 Player Entity 全类别合同 + 当前 Player 支撑范围”；具体字段名实现时确定。只要 Entity component 的 Player 部分仍为 17 代表范围，就不设置完整 Entity 域完成位；20 补齐后再合并。失败、重复目标返回和正常提交都需保持同一范围口径。完成时未来 21 可以直接据结果判断，不需猜测 DTO Stage 是否落到原生。

### I30：预算、缓存与诊断

保持用户已要求的 64M 节点上限，不擅自收紧；既有 32MiB 聚合与各域字节／深度／对象／资源上限继续独立。统一当前 512 对象、256 render 成员、4096 命令的冲突，按各原生池实际容量、单实体几何与辅助资源成本确定具名预算；不得为了容量盲目覆盖池项，也不把 64M 节点当可分配 64M Entity。

解析一次完整 T，稳定窗口缓存分类／资源／拓扑，换实例或来源变化立即失效；按主线程工作量分步，不每 consumer 重读全图。必要状态输出包含对象／资源／引用／曲线点数量、目标字节／节点、应用进度与首因，详细诊断／录制默认关闭。完成时任一预算不足明确失败且不改成空世界；实际耗时／峰值归 I34 测量，不预报性能收益。

### I31：P1—P6 接入与回归

复查新类别对 `replica_projection_policy.py` 省略成员的实际消费，包括动态字段拼接、辅助 Sprite、曲线和特殊资源。需要恢复的参数查询逐项恢复并提供有界参数／正式来源，或用语义等价的正式紧凑结构，不能用“原来无消费者”拒绝必需功能。

保留四个 shader 查询、完整动态层／pose_runtime、真实失败状态及组件内图闭合；不添加未经证明的 Sprite 别名。I33 合入 18 全场景回归、模型 v2 不兼容拒绝和同源观测／投影体积测量。完成时优化不破坏完整观测、字段错误或资源行为；原文估算不替换为伪造实测。

测量分别记录原始完整观测大小、当前发送投影大小及 `cs status` 的目标字节／节点。若报告 P1—P6 的实施前后净收益，应在同一 E2E 内对同一份自有来源值明确比较旧／新投影语义，或仅引用口径可比的既有测量；原始观测与投影之差不能直接冒充本次优化收益，不要求恢复已清理的历史 artifact 或重启旧 DLL。

### I32：构建及产品边界

领域目标／plan、逐类 adapter、原生窄桥和统一 Hook／layout 声明纳入 `mod/CMakeLists.txt` 及现有生成依赖。职责可按类别族拆文件，避免在 Scene／Hook 调度中堆全部业务。新功能、核心函数、数据结构写中文注释，说明语义、owner、允许用途、失败前缀及退休责任。

生产仍是 REPENTOGON 加载的 DLL 与必要自有只读观测／UI 模块；Rust harness、测试 Lua、临时文件导入导出、故障开关受 E2E 构建和显式 token 约束，不作为正式发布玩法路径。纯构建若执行不得夹带测试；本轮文档任务不构建。

### I33：E2E 编排实现

先按第 7 节写清失败方式，再实现统一 19 编排。建议扩充 `tools/harness/src/tx19.rs`、`tests/e2e-mods/isaac-cs-test/tx19.lua` 和既有脚本入口，名称为计划；复用 18 的导出／冷 Client／只读后态／图像比较机制与场景清单，避免复制第二套应用器。

Source 用真实 gameplay 生成与改变目标，Client 只从正式 consumer 自有值导出的 T 经正式解析／应用链安装；连续目标必须表达实际来源变化。负向数据与故障注入只在显式 E2E 范围，记录注入点，不能假称自然玩法覆盖。不得为导出成功清理待测实体；Player 代表投影与非 Player 完整范围分别声明。完成时一个入口可执行 E01—E26、必要恢复／清理和同 tx 续测，进入 I34 前不运行。

### I34：统一整体验收与连续修复

全部目标实现后才进入第 8 节的一次完整验收过程，可包含多个场景、游戏启动和失败修复后的必要续测。每次启动先说明用户需做什么、何时停止操作；需要手动跳过开场等配合时先确认准备好，再启动。既有会话授权按实际范围沿用，计划不额外授权存档、成就、捐款或统计修改。

失败先定位并修复，在同一 tx 覆盖修复影响及未完成场景，不新编号、不默认停下等待新指令；缺必要配合／授权或不能自行解决时说明继续条件。正常临时材料用后清理；只有确需中断恢复才留必要材料。完成时逐类、逐场景给出实际结果与未覆盖，不把 Source 成功或包数当 Client 成功。

### I35：结果与后续交接

在本目录维护功能实现／验收说明、应用目录与具名剩余项，同步文档入口和路线图。记录实际失败及修复、已审画面、非 Player 覆盖、仍受限的 Player／网络／整局边界、预算和 P1—P6 实测；不把过去 17／18 的失败或 UNKNOWN 改写成成功。

向 20 交付 Player 依赖接口及准确应用范围；向 21／22 交付同一解析／plan／结果／取消与重建；向 23／24 交付当前生命周期与场景依赖；向 27 交付短命独立事件的未消费项及各类资源／draw；向 29 交付具体特殊变体／组合覆盖清单；向 32 交付真实成本。没有保存 tx artifact 包的要求。

## 7. 实现前明确的失败模型

| 编号 | 失败方式 | 预定处理／责任 | 整体验收 |
| --- | --- | --- | --- |
| F01 | 分类、字段或来源版本不支持，Dummy 名称不一致 | I01／I02 写前给具体原因，不猜派生布局 | E01／E22 |
| F02 | Partial／Truncated／NotScheduled 被补零或当删除 | I02／I06 保留状态，缺必需值拒绝目标 | E17／E22 |
| F03 | Init／Morph 自动生成子体、奖励、随机或 PGD 更新 | I07／I27 阻止具体调用或供应既定结果，处理真实构造依赖 | E02—E14／E20 |
| F04 | Player／Grid 依赖迟到、循环或 Null 清除失败 | I09／I24 分层依赖、显式解除，不能先提交再补 | E14／E15 |
| F05 | 地址复用或 Morph 后旧引用／Sprite 借用继续有效 | I05／I10／I23 实例与资源换代，撤销所有旧借用 | E16／E19 |
| F06 | dead-but-existing 被过滤，短命对象重放为常驻 | I06／I23／I26 分离存在、死亡与事件 | E16／E17 |
| F07 | laser 曲线、Text 颜色、辅助 ANM 等必要来源缺失 | I03 形成正式来源，未解决则功能未完成 | E06／E07／E13／E18 |
| F08 | draw 调用玩法更新／动画事件或漏 pass | I25／I27 窄维护与逐类分派；停显而不补 Update | E18／E20 |
| F09 | remove 连带清理错误，池项双释放或残留 ESSM | I23／I28 单 owner、前缀和在途退休 | E16／E19／E21 |
| F10 | 资源／字段／引用写到一半失败 | I28 撤 A 和展示资格，安全退休后完整重建 | E19 |
| F11 | 已写计划被新 T 覆盖，半套范围被 ACK | I06／I29 取消分级、唯一提交、范围准确 | E01／E19／E22 |
| F12 | 优化删掉新消费者、悬空 graph_ref 或假 Sprite 别名 | I31 精确发送规则、组件图闭合、实际成员区分 | E18／E23／E24 |
| F13 | 对象与绘制上限不同、预算耗尽阻塞主线程 | I30 统一预算、分步、写前拒绝或按前缀退休 | E25 |
| F14 | 录制关掉就无来源、夹具过滤实体导致假覆盖 | I03／I33 正式 consumer 与录制分离、源成员盘点 | E01／E23／E26 |
| F15 | Client 故障后提前 Off，访客进度遭延迟 writer 修改 | I27／I28 保护持续到安全退休及相关最后 writer | E20／E21 |

不为这些失败方式另写实现后的单元测试，不提前运行隔离程序、smoke、probe 或验收试跑。

## 8. 全目标完成后的统一 E2E

默认沿用真实 Source 与独立冷 Client 的顺序进程方式，并连续应用同一场景的多个 T，足以验收 19 原生应用；四端实时下行仍归 21。Source 夹具提供场景，不替代正式只读观测；Client 必须通过正式 parser／plan／Scene。若执行时已有可用实时链，可在同一入口补充，不降低下表覆盖。

每项联合核对目标来源及完整性、实际 native 对象／成员／引用／资源后态、应用范围、必要画面及无第二份裁决。画面选择同一 source scope／revision 或明确配对的表现 phase，不能把不同时刻截图差异直接归因于应用器。复用未受影响的成功覆盖只限同 tx 续测且说明范围；P1—P6 引入的 18 回归需要实际重跑，不能只引用历史 PASS。

| 编号 | 场景 | 必须观察的结果 |
| --- | --- | --- |
| E01 | 冷 Client、同场景连续 T、重复／旧／冲突目标、回基线与字段清空 | 真正原生提交、幂等与精确范围；录制关闭仍应用；未提交不展示半套世界 |
| E02 | C01 普通／冠军／多部件 NPC，状态／血量／颜色变化 | 部件关系、死亡中存在性与姿态正确，无 AI／伤害／奖励裁决 |
| E03 | C02 Delirium 专用状态及变形 | special 状态、资源及逻辑身份正确，旧借用失效，无本地随机变形 |
| E04 | C03 Tear 及 C04 Projectile 的高度／flags／轨迹与变化 | 空间姿态、附着／目标与辅助表现正确，无分裂／碰撞重复生成 |
| E05 | C05 Bomb 的计时／闪烁／服装／附着与最终移除 | 倒计时由目标供应，不在 Client 引爆／破坏 Grid／产生掉落 |
| E06 | C06 直线／圆形／弯曲／sample／链式 Laser | 端点、宽度、采样几何和 owner 正确，曲线清空／替换无旧缓冲残影 |
| E07 | C07 Knife 挂载／伸出／返回及 owner 变化 | 阶段、距离／角度与渲染正确，无本地投掷更新／伤害 |
| E08 | C08 普通 Pickup、价格／商店／选项／循环内容、Morph | 价格 Sprite、物品表现和最终删除正确，无购买／支付／重抽 |
| E09 | C09 跟随／环绕／链式 Familiar，owner 改变与删除 | 已解析 Player Key 与关系正确，无本地寻路／开火／道具重建 |
| E10 | C10 地面／持续／透明／带子对象 Effect | 完整姿态与分层正确，子对象唯一，不把玩法效果当本地可模拟装饰 |
| E11 | C11 Slot 现有变体 1—18 的状态／奖品／主及奖励 Sprite | 每种至少一份实际适用后态；具体交互按授权，在 Client 不开奖／捐款／改进度 |
| E12 | Slot 奖品／卡币／损坏变化与 Pickup／Effect 关系 | 最终结果一致、奖品只出现一次、计时不本地继续裁决 |
| E13 | C12 NativeText、C13 NativeDummy | 正确分类、原生承载／绘制／退休；Text 有界内容与颜色，Dummy 无法取得有效来源时列未完成而非用 NPC 代测 |
| E14 | Effect136 举起 Grid、描述变化、移除及目标落地 Grid | 只有一份 Effect 和应有的活动 Grid，无描述冲突、双构造或虚构 source_grid |
| E15 | 必需引用迟到、合法环、Null、跨类型误引用、owner 删除 | 写前缺依赖拒绝，补齐后正常；解除／换代无悬空，构造环有明确结果 |
| E16 | 同 Key Morph／换资源／换本地实例、同地址新 Key、重复删除 | shape／实例代次和反向关系正确，旧对象、辅助 Sprite 与缓存不复活 |
| E17 | 同采样 create→remove、已死亡尚存在、独立旧姿态／事件 | 最终集合正确，短命不重建常驻，事件不伪确认，不同 scope 不拼成原子状态 |
| E18 | 主／overlay／逐层／辅助 Sprite、shader、换图；阴影／light／水／镜像／柱遮挡 | 真实派生 draw 与 Source 画面对照正确，无 Update 补算；缺必需资源显式失败 |
| E19 | 分配／Init／发布／资源／引用／draw 前后故障与取消 | 写前保留 A；写后停显、准确首因与前缀、安全退休及完整目标恢复；不双释放 |
| E20 | 普通／odd／FSM／暂停／loading／连续展示和 Host／Client／Off | Client 不产生玩法／随机／Lua／音效重复裁决；Host／Off 及统一输入语义保持 |
| E21 | 准备中、应用中、展示中 stop／断连／正常菜单交还／退出 | owner／借用收口、访客进度全窗口隔离，未知清理不能假报 Off |
| E22 | 来源缺失、非法类型／数值／bitset／引用、版本不兼容、Player 超范围 | 具体失败定位，完整观测错误不被省略掩盖，不 ACK／展示部分世界 |
| E23 | 18 的完整 Room／Grid 清单回归 | 重跑其 Source 49 场景、冷 Client 49 场景及 12 检查对应覆盖；必要调整夹具但不删除原行为，保留代表画面复核 |
| E24 | P1—P6 同源观测／投影、图节点、独立 Query 和四 shader 查询 | 图闭合、失败状态和保留字段正确；记录 `cs status` 目标字节／节点、同源投影前后口径与变化原因 |
| E25 | 多实体／复杂曲线／多 Sprite、上限与超限、稳定重复目标 | 预算一致、无静默截断，记录采集／解析／应用／draw 成本和峰值；不在本 tx 虚报 10 Mbps 实时能力 |
| E26 | 正式／E2E 构建边界、无录制／录制故障、临时部署清理 | 正式目标不依赖录制、夹具或详细日志；产品包不包含 Rust harness／测试玩法，临时材料清理 |

负向用例不要求真实耗尽系统内存或破坏真实进度；在已解释的边界注入失败并核对真实清理责任，未知异常仍保留限制。Slot 捐款等涉及持久写入的 Source 动作仅按实际已有授权执行，未有授权时先准备具体场景与隔离方案再询问，不让用户批准抽象风险。

## 9. 推进顺序及当前收束

1. I01／I02 与 R01／R02：确定目录、分类、scope、Player 边界、身份及依赖结构，先列失败方式。
2. I03／I04 与 R03—R10：按类别补源与必要静态合同；R11—R13 随实际 draw／清理／隔离调用同步补齐，无分项试跑。
3. I05—I10：完成公共纯值计划、原生构造分阶段、引用及资源；随后 I11—I22 按依赖逐类落地，I23／I24 同步处理 Morph／删除／Grid。
4. I25—I32：补完整展示、短命与隔离、退休、唯一提交、预算、优化消费者与发布接线；I33 在失败模型明确后配合实现。
5. 全目标完成后执行 I34 的统一 E2E，持续修复与必要续测；I35 写回事实和后续责任。

R01—R13的初始任务清单和原有UNKNOWN保留；I04已形成的结论、否决路径及具体未闭合分支以本目录两份静态专题为当前入口。后续adapter新增实际采用路径仍按资料／源码／必要逆向解决其具名疑问，不预设用户猜原生实现。只有产品范围、尚未授权的实际操作或无法从工程证据确定的选择才提出具体问题；不将静态结论等同原生应用或运行验收。

2026-11-03 文档整体验收：已人工静态审阅三份计划、I／C／R／E 对应、相邻 tx 边界、源码现状、文档引用及入口修订。仅新增本目录三份计划并更新文档入口／路线图／优化计划链接；没有修改运行代码、执行构建、游戏、测试或格式／链接校验程序，没有制作 tx artifact。
