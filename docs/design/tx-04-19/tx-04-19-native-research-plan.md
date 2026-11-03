# tx-04-19：静态确认与必要逆向工作清单

日期：2026-11-03。状态：**I04已形成R01—R13的[静态采用合同](entity-application-contracts.md)、[J273必要逆向与主审](entity-native-static.md)和[功能目录研究索引](entity-native-contracts.json)。** 各类别实际适配、本文明确未闭合的专用分支和统一E2E仍待后续任务；不把静态完成等同全类别构造／写回／draw／退休已实现。I01—I03的窄来源历史见[来源说明](entity-source-research.md)与[实现说明](tx-04-19-foundation.md)。

本清单属于[总实施计划](tx-04-19-implementation-plan.md)的I04；服务[逐类别适配](tx-04-19-entity-application.md)。本轮按用户会话指示由一个GPT-6 Luna max子智能体完成必要逆向，主智能体独立回读源码、伪码和关键原始指令并审阅纠错；没有运行探针／测试／游戏，没有构建或新建artifact。

## 1. 来源优先级与结果要求

1. 优先读两套本地 Wiki、当前官方 REPENTOGON wrapper／ZHL／SDK，以及 10、17、18 已有研究与实现。公开只读接口语义清楚时直接采用，不要求逐成员逆向或 native 双实现。
2. 对实际要调用的构造、setter、维护、draw、清理路径，有具体 ABI／副作用／owner 疑问时追官方源码；尚未解释的原生 callee／虚表分支才补 J273 静态反编译及原始指令。
3. 旧 J460 地址仅作检索索引。使用目标版本下已解释的函数／指令／布局，不做哈希校验，不运行程序、probe、harness 或试调用来代替静态确认。
4. 研究记录区分 `PUBLIC_DOC`、`SOURCE`、`REUSED_J273`、`J273_STATIC`、`DERIVED`、`DESIGN`、`UNKNOWN`。新补的结果给函数／文件和必要 RVA，标明 VA／RVA，不能把伪码变量名当布局证据。

每条实际采用路径最后都要形成以下可实施合同：签名／调用约定／返回、原生对象前置及线程／context、构造或成员发布时点、直接和必要间接调用、随机／生成／Lua／音效／持久副作用、字段适用性、资源／引用 owner、正常与失败清理、被选择的实现方案及未决项。只补本 tx 必需路径，不进行无边界全引擎扫描。

静态结果写入本目录功能文档和 I01 应用目录；必要布局／Hook 声明在后续实现时进入唯一 `mod/binding/world_layout.json`／`hooks.json`。不另建 tx artifact、哈希报告或重复地址清单。

## 2. 可复用的既有事实及其边界

| 已有依据 | 可复用部分 | 不能外推的部分 |
| --- | --- | --- |
| [10 Entity 原生补充](../observation-expansion/entity-native-supplements.md) | Text／Dummy 分类、有限当前读取；SampleLaser 布尔来源；LootList／GetBombFlags 副作用警示 | 不证明对应类型可安全构造、所有专用字段／颜色已知或 draw／退休完整 |
| [17 对象生命周期](../tx-04-17/tx-04-17-native-object-lifecycle.md) | Factory use、Init／成员发布分阶段；普通 Player／Effect57；EntityPtr、ESSM 和 remove 的已采用前缀 | 不证明所有派生 Init、副作用、非空附属 owner 或专用列表已安全覆盖 |
| [17 资源寿命](../tx-04-17/tx-04-17-native-resource-lifetime.md) | 原生内嵌 ANM、Image／在途持有；NPC Morph 同 this 资源重载、Pickup 价格 ANM 重建 | 不证明所有类的辅助资源、异常清理或 Morph 整链已闭合 |
| [17 维护绘制](../tx-04-17/tx-04-17-native-maintenance-rendering.md) | 展示与玩法分离、排序与部分 pass、当前已验证代表场景 | 不证明全类别虚调用、beam、附属 draw 和特殊缓存已完整 |
| [18 验收／后续补证](../tx-04-18/tx-04-18-acceptance.md) | Room／Grid owner、分层／水面／镜像、背景生成隔离、真实成员 Sprite、CPU 队列深度修复 | 不证明所有 Entity 可直接进入现有 draw；Effect136 应用仍未实现 |
| [发送优化实现](../replica-redundancy-reduction.md#10-2026-11-03-前置优化实现) | 当前各 Sprite Getter 是独立成员，四 shader 查询保留，图闭合与模型 v2 | 新 Entity 消费者可能需要恢复被省略字段；当前无别名不等于未来任何接口都不可能有别名 |

参考本地入口：[原版类图](F:/projects_isaac/IsaacDocs/docs/snippets/EntityClassDiagram.md)、[REP 类图](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/snippets/EntityClassDiagram_NewFunkyMode.md)、[官方 Entity ZHL](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Entity.zhl)。其余类别文件按同目录实际类名定位；不从文档示例签名推定隐藏 helper 的 ABI。

## 3. 全部静态工作

### R01：分类、字段适用性与能力名称

**对应任务：** I01／I02／I05／I22。优先静态代码／源码确认，只有实际派生类型无法由已核定来源判断时才新增 RTTI／工厂逆向。

**具体工作：**

- 对照 `entity-catalog`、Lua conversion 次序、`NativeSupplement`、`EntityFactory` 选择及 `model.cpp::Capabilities`；确认每个 C01—C13 的真实类、适用 Type／Variant／SubType、special 与继承关系。
- 核对 `NativeDarkness` 声明与 `NativeDummy` 输出差异，定位是旧命名残留还是另一具体类型；仅按证据统一，不无声扩大别名。
- 检查全部源字段的当前适用性、signedness、bitset128、可空结构、容器索引／Key 语义及 Public Getter 的真实返回类型。Field 与操作／实际事件／NotReplicated 的分类互不覆盖。
- 校对观察侧成员完整性、`Exists=false`、dead、Retired 与来源资格，明确 I06 可按哪些信息判存在，不从 `Isaac.GetRoomEntities` 单次缺席推导销毁。

**产出与未决处理：** 形成类别／字段适用表和能力修订方案，列出需后续 R 项补的具体私有表现字段；没有证据的类型保留 Unsupported／UNKNOWN，不伪装为 NPC。完成不意味着实机出现过每类。

### R02：构造、发布、池 use、引用与列表归属

**对应任务：** I05— I09／I23／I29。复用 17 公共事实，新增逐类前置、附属 owner 和列表分支的必要逆向。

**具体工作：**

- 对各类查明从工厂取得 use、基构造、派生 Init、官方扩展分配／初始化、成员发布和外层完成的准确顺序；是否已有存活池项覆写、专用池或 Player／Room 前置。
- 明确哪些引用必须在 Init 前有效，哪些可在所有实例存在后用 `EntityPtr::SetReference` 安装；解析 Player／Weapon／Grid 直接指针与 EntityPtr 的差别。
- 补 main／persistent／render／update／专用列表、flags 条件、类型查询缓存、附属 owner（含代表实现拒绝的非空 `+0x310`）及反向引用的真实归属。
- 检查连带构造：哪些是必需子结构、哪些是本地玩法生成、哪些子 Entity 已在 T 中有独立 Key；给出唯一 owner 和创建策略。
- 对构造依赖环给具体方案，不能将通用两阶段引用安装误用于“无 owner 就无法 Init”的对象。

**产出与未决处理：** 按类给调用顺序／前置／前缀／列表与引用策略。已核定正常路径可用，OOM／异常未知单独列出；未解释附属 owner 的类型继续未完成，禁止通过删掉保护来放行。

### R03：NPC、Delirium 与 Morph

**对应任务：** I11／I23／I27，C01／C02。公开 NPC／Delirium 页面及 wrapper 优先，补具体 Init／配置／Morph／draw 必需分支。

**具体工作：** 核对配置加载、冠军和特殊形态、boss／多部件生成、PathFinder 关联与专用容器；列出通用 NPC 状态与 Delirium special 的布局及适用性。区分原位字段更新、原位资源重载和真正需要换实例的变化。追已知 NPC Morph `VA 0x6BD0C0` 的采用分支及 bestiary caller；这个地址来自 17，当前不是新增审查结果。

分析源端死亡中存在状态和组合 draw 的前置，确认不调用 AI／伤害／击杀即可表达必要外观。Delirium 的扩展 state 与实体普通配置不能互相覆盖；随机变身结果只能从 Source 安装。

**产出与未决处理：** 形成 NPC 适配族、Delirium 专项和 Morph 选择表，指明要新增的正式值及隔离点。若需某个特殊部件先存在，把依赖回写 R02／I09，不把整个类型标为“普通 NPC 已解决”。

### R04：Tear／Projectile／Bomb 的初始化与派生表现

**对应任务：** I12—I14／I25，C03—C05。读取各自 Wiki／ZHL／Lua wrapper，不因字段名相似混用布局。

**具体工作：** 逐类确认高度／下落／旋转／缩放、flags／命中集合、附着 owner、构造时音效与分裂／爆炸路径。Tear 三份辅助 ANM2、Bomb costume 数组分别核 owner、可空条件和资源更换；核 draw 的位置／颜色暂存与异常责任。

`GetBombFlags` 的真实调用／RNG 特性沿用 10 记录，不主动轮询；若应用需要其结果，查现有权威当前字段或真实调用事件是否足够。命中列表若存 local Index、指针或特殊编号，明确身份转换与是否仅作纯值保留，不照写 Source 数字。

**产出与未决处理：** 三类分开的字段／资源／draw／清理合同；列明安装阶段不能调用的 split、explode、damage、update。任何必要辅助资源缺口反馈 I03，不能用公共 Sprite 代替完成。

### R05：Laser／HomingLaser 最终几何与 owner

**对应任务：** I15／I03／I25／I28，C06。公开 [EntityLaser](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)、`EntityLaser.zhl`／HomingLaser 定义、官方采样 Getter 优先。

**具体工作：**

- 追实际 Render 对 optimized／non-optimized samples、端点、宽度、截断、sampleLaser 和 HomingLaser 私有成员的读取，判定公开点集是否足够供应所有必需几何。
- 核每个 vector／buffer 的元素类型、实际 size／capacity、不透明 owner、构造／清空／释放、CPU 在途引用；只核头部偏移不能当作容器完整合同。
- 确定直线／圆形／sample／曲线／链式分支与所需资源、父体、临时缓存；识别 Init／draw 是否调用 PlayInitSound、生成子束或更新采样。
- 若私有 Bezier／几何确为必需，限定 Source 当前结果的窄复制方案；若可从完备目标值用纯表现算法生成，写清算法输入与无玩法副作用的依据。不能调用 CalculateSamples／RecalculateSamplesNextUpdate 作为兜底。

**产出与未决处理：** 明确采用公开点集还是新增正式几何源、每个分支的调用／容器／寿命及预算。当前“公开采样存在”只证明可读，不证明可安全写回；几何 owner 未明时保持该分支未完成。

### R06：Knife／Familiar 的 Player、武器与关系前置

**对应任务：** I16／I18／I05／I09，C07／C09。

**具体工作：** 明确 owner／parent 与 Player／Weapon 结构的差别，构造／Render 何时解引用哪个关联，哪些状态必须由 20 的未来 Player adapter 提供、哪些为 19 当前可提供的窄依赖。分析 Familiar 跟随／环绕／链式关系 owner 及清理、Knife 挂载／伸出／返回状态和 draw 缓存。

核定无需调用 `FireKnife`、`EvaluateItems`、跟随 AI 或物品逻辑的原生构造与表现路径。所谓“普通 Isaac 支撑”必须具体到被读取成员，不把 Player 指针非空当完整前置。

**产出与未决处理：** Player 依赖接口、构造必要边、可后置关系、状态和 draw 合同；必要外观前置在 19 补足，完整角色／装备仍交 20。未能满足的真实依赖明确未完成，不临时绑首玩家或假装备。

### R07：Pickup／Slot 的最终状态、辅助 ANM 与持久副作用

**对应任务：** I17／I20／I23／I27，C08／C11。

**具体工作：** Pickup 的物品／价格／选项／循环容器、Init／Morph 与主／价格／辅助 ANM；Slot 1—18 的 getter／setter、变体字段语义、奖品 ANM、ShellGame 动画、计时位及原生状态机。公开 setter 若为直接状态安装可采用，有调用链则核副作用，不要求无疑问 setter 一律重做逆向。

复用 17 的 Pickup Morph `VA 0x6DFF30` 与价格 ANM Load／Reset 线索，补本次采用分支；查随机选择、LootList、支付／捐款、bestiary／图鉴／统计／成就及 Mod 回调的具体 caller。奖品 Entity 的生成和 Slot 当前状态安装分开。

**产出与未决处理：** 按变体的安装表、Morph／替换表、辅助资源 owner 和禁止重演路径。未授权 Source 捐款属于操作安排，不能通过静态结论获得授权；Client 调用即使最终文件不变也不能放过内存持久进度污染。

### R08：Effect 专用族与举起 Grid 的 Effect136

**对应任务：** I19／I24／I25，C10。

**具体工作：** 将实际需支持的 Effect 按 Init、资源、draw、附属结构和清理差异分族，复用已理解 Effect57，补地面／持续／透明／light／子对象等采用分支。识别超时／动画事件／draw 触发生成与随机的路径，不能认为所有 Effect 都是纯装饰。

本轮代码阅读已确认官方 `LuaEntityEffect.cpp::Lua_GetGridEntityDesc` 在 variant136 返回从 `_varData` 开始的 GridDesc 视图；仍需核定实际 descriptor 大小／布局、Init／draw／remove 读写、额外资源／引用及存储寿命。对照 18 的 `LiftedGridTarget`、`VisitLifted` 和 Unresolved `source_grid`，明确它是复制描述而非活动 Grid owner。

**产出与未决处理：** 专用 Effect 适配族及 136 完整合同，明确通用 VarData 与专用描述的互斥解释。若某分支原生构造自动重建 Grid 或生成子体，确定具体隔离／接管方式再实现，不让 Client 多创建一份世界对象。

### R09：NativeText／NativeDummy 的创建、绘制与退休

**对应任务：** I21／I22，C12／C13。

**具体工作：** 复用 10 已核定 Text ctor／Init／Update／Render 和 Dummy ctor／Update／Render 的 RVA 作为导航，补工厂分配／池、发布、字段初始化和实际 caller。Text 查字节串容量／编码／终止、专用颜色、字体／布局和当前 phase；Dummy 查其真实创建用途、对象列表和需要的公共状态，不猜未公开派生字段。

确认可从正式 Source 取得完整合法目标，以及 E2E 是否有已解释的真实构造／场景。不能通过改 vptr／Type 把 NPC 变成 Dummy，也不能用重新 Update 获取文本颜色或计时结果。

**产出与未决处理：** 两类独立的构造／安装／draw／清理合同、Text 新增源及分类名称修订。无法合法触发或缺必需表现源时明确未完成，不用从未观察过作为类别通过依据。

### R10：真实 Sprite 成员、辅助资源与借用失效

**对应任务：** I03／I10／I23／I31，覆盖全部 C 类。

**具体工作：** 每个主／辅助 Getter 核是真实成员、借用对象、按值复制还是共享资源，复用 P1 已确认的独立成员关系；只有精确同一原生成员才可声明别名。新增 Getter 不能因文件名或当前姿态相同就合并，18 ExtraSprite 值副本会丢状态的事实作为检查项。

沿用 `ClientParseResourcePose`／资源服务，核精确 phase、overlay、层状态、shader、特殊轨迹及原生 Init／Morph 对资源内部的重载。查辅助 ANM、costume vector、buffer、Image 与 Lua wrapper 的 owner、缓存借用和取消后在途寿命；分析需要恢复的 P3 参数查询及其正式有界参数。

**产出与未决处理：** 每类资源成员清单、别名结论、重取条件和释放顺序，明确缺失／空／未加载的差别。无需额外全量动画表即可满足消费时保留 P2 优化；若新增消费者确需此前省略的字段，先显式修订投影合同。

### R11：派生 draw、必要缓存及分层 pass

**对应任务：** I25／I03／I27，依赖逐类字段和资源结果。

**具体工作：** 对 C01—C13 的 Render／Shadow／Light／PostRender／GetRenderZ 等实际采用路径核签名、虚表分派和必要 caller，明确哪些正式状态／缓存被读取，哪些临时字段被修改。重点关注 Laser 几何、NPC 部件、Tear／Projectile 位置／颜色暂存、Knife owner、Pickup 价格、Slot 奖品和 Text 专用 draw。

对照现有 `Members`／`DrawEntity`、水面 `DrawBand`、原生水面／镜像 pass、Grid 柱交错与 CPU 队列，核间接路径是否绕过类别分派、是否重复绘制或触发 Lua／动画／随机。保留 17／18 已修复的 FBO／depth 顺序，不凭空清 depth、运行 RoomPreRender 全链或补 Update。

**产出与未决处理：** 逐类×pass 的调用、必需输入、临时写与恢复表；缺缓存反馈 I03，副作用反馈 R13。静态可以说明允许调用，最终像素及实际层次仍归统一 E2E，不把“返回正常”写成画面正确。

### R12：删除、部分构造、在途失败与重建

**对应任务：** I23／I28／I29，覆盖每种已采用原生 owner。

**具体工作：** 对尚未 Init、Init 部分完成、已发布、引用已安装、资源部分替换、draw 有队列、Morph 换实例分别明确退出顺序。分析 Entity Remove、ClearReferences、附属 owner、专用列表、类型缓存、EntityPlus、官方 ESSM DeleteEntity、userdata 失效及池最终回收的真实责任。

严格区分不触发战斗的退休与 Die／爆炸／奖励；Room 全清理和单对象删除不可各自释放同一内存。构造失败返回、C++ 异常、SEH、Lua 错误、context／VM 不可用的处理能力分别说明；不能把正常返回路径外推为所有异常安全。

**产出与未决处理：** 每类前缀→清理责任→是否可重建／只能保持隔离的表，接入现有 `ClientNativeObjectsAbandon`／Scene 收口。未知清理保留 owner、首因和隔离，不盲目重复调用或强制卸载；虚拟设备 NEVER 生命周期不改动。

### R13：新增路径的用途分派与持久整链

**对应任务：** I27／I29／I32，与各类别研究同步，不等实现后补查。

**具体工作：** 汇总所有新增构造／setter／Morph／draw／remove 的 RNG、spawn、音效、动画事件、Lua、平台和持久 writer，逐个决定允许的必要结构维护、替换为 Authority 最终值、或在 Client 具名调用范围内阻止。分析绕过普通 Update 的直接调用、嵌套用途、Host／Off 透传及原调用 exactly-once。

复用 [17 持久隔离](../tx-04-17/tx-04-17-persistence-exit-isolation.md)与[失败退休](../tx-04-17/tx-04-17-failure-retirement.md)，特别回补 NPC bestiary、Slot 捐款、Pickup 收集与官方 Mod 附加保存；保护开始在原生准备之前，结束在安全退休和相关最后 writer 之后。状态应用完成不等于可以提前 Off。

**产出与未决处理：** 唯一 Hook／窄调用清单的新增用途规则及每条路径的副作用处置，回写功能目录与 E20／E21。不增加第二 Hook owner，不恢复字节／重注入，不触及原版 Online 或 Isaac Sync。尚未理解的调用继续未完成，不能单靠静音、readonly 或最终文件未变认定已隔离。

## 4. 静态工作顺序及技术未知

R01／R02 先明确类型与公共生命周期，R03—R09 随各类别依赖推进；R10 贯穿资源，R11 在字段／资源基础上整理所有 pass，R12／R13 从第一次选择原生调用时就同步分析。对应源码／公开接口已足够时只做静态确认；只有具名缺口才进入逆向，不机械要求每个 R 都反编译。

以下是初版I04之前的主要技术未知，保留原问题供对照，不将其当作执行后的当前判定；本轮结果见第5节和两份静态专题：

| 未知或选择 | 负责项 | 如何得出结论 |
| --- | --- | --- |
| 各类是否可无 owner Init，哪些必须提前绑定 Player／Grid | R02／R06／R08 | 真实 caller、Init 读写及官方构造顺序 |
| Laser 公开 samples 是否足够表达原生 draw | R05／R11 | 逐个实际几何 consumer 的读集合 |
| Morph 哪些可原位，哪些应换实例 | R03／R07／R10／R12 | 布局／配置／资源失效、副作用和清理责任 |
| 非空附属 owner 如何清理且不连带删除目标存活对象 | R02／R12 | 反向引用／专用结构和真实 remove 调用链 |
| Text 专用颜色与字体等必需字段的完整正式来源 | R09／R11 | Init／Render 实际字段及只读来源资格 |
| Dummy 的有效场景／构造与 NativeDarkness 声明的关系 | R01／R09 | 分类源、工厂／caller 和能力生成源码 |
| Effect136 与通用 VarData 的存储适用性 | R08 | 官方 wrapper 加目标布局／Init／draw／退休 |
| 新类别是否需要恢复 P3 中被省略成员 | R10／I31 | 实际 adapter 消费及等价紧凑正式结构 |
| 19 的非 Player 完成范围怎样接入当前域位结果 | I02／I29 | Entity 含 Player 的实际模型结构和 21 消费责任；不冒报完整域 |

这些技术问题不要求用户猜原生实现。只有证据不足且需要产品取舍或新的实际操作授权时，形成具体方案后再问用户。即便所有 R 项形成静态合同，类别实现、运行后态和画面仍要分别完成；本轮不提前执行任何测试。

## 5. 2026-11-03 I04静态结果与接续

各项均已按资料优先级形成研究记录；下表是交付定位，不是native支持矩阵或运行PASS。具体ABI、直接/间接消费者、失败前缀和未决项以[采用合同](entity-application-contracts.md)及[J273专题](entity-native-static.md)为准。

| 研究 | 已形成的具体结果 | 实施责任和未决边界 |
| --- | --- | --- |
| R01 | 公开转换/RTTI分类、同一NPC的Delirium special、NativeDummy命名、字段signedness/位宽、存在/死亡/退休/缺席分离 | I05/I06按实例和完整成员集合应用；未提供未知类的假身份 |
| R02 | 各预构造池步长/容量、true不覆盖活槽、Player专用owner、New/Init/Plus/发布前缀、EntityPtr/裸指针区别、专用成员责任 | Dummy generic964实际取NPC；真实取得owner仍有缺口；非空附属owner不得删保护放行 |
| R03 | NPC Morph的bestiary写入、Delirium字段精确宽度和延后AI变形操作、优先同Key实例替换合同 | I11继续特殊NPC部件/配置/缓存适配，不能只以普通NPC替代 |
| R04 | Tear/Projectile/Bomb分开布局/flags/高度；Tear三辅助、Bomb五costume真实成员；禁止分裂/引爆/Update取值 | 派生Init与特殊draw按专题分支采用，缺缓存补正式来源 |
| R05 | Laser signed mode、真实optimized samples、非优化/Bezier不同owner、Init的PostLaserInit Lua、容器字节/allocator/在途责任 | I15明确游戏allocator相容的容器安装，不复制vector头；不CalculateSamples |
| R06 | Knife/Familiar的Player/Weapon/EntityPtr前置和专用关系；Familiar退休跨写Player与清理Weapon | I16/I18按真实Key和窄外部接口满足依赖；不绑首Player/假装备 |
| R07 | Pickup main/price/alt资源与flip副作用；Slot18变体、leaf存储宽度、SetPrize非leaf、Timeout文档冲突 | I03/I17补真实必要源；I20不采用整开奖/捐款/setPrize图形链，不凭Wiki补高位 |
| R08 | Effect按初始化/资源/附属owner分族；136真实creator复制GridDesc32并交换完整ANM，lift链另改Grid/RNG并直调关联Entity Update | I19/I24消费Source完整描述/State/主资源及举持依赖，不重跑lift、不建第二个活动Grid |
| R09 | Text真实writer/KColor20/独立phase，Dummy独立类型及字体draw，不能Create964伪造Dummy | I21/I22实现合法取得、安装和退休；未知writer/creator保留具体缺口 |
| R10 | 各真实辅助成员/容器owner、借用失效；Knife共享DeadEye、Familiar48/231 ExtraData、Bomb lazy-rebuild来源时点等具体补充 | I10/I31复核实际消费者；这些新需要不能由夹具合成或公共Sprite替代 |
| R11 | 类别/pass输入、代表DrawEntity错误cast边界、18水面/FBO/depth顺序，Pickup/Familiar draw推进全局RNG且Bomb draw可重建资源 | I25/I27采用窄分支，替换为Authority真实表现；像素正确性只在I34判定 |
| R12 | 逐前缀退休/取消/部分失败，动态Remove/ClearRefs连带owner，ESSM与Plus责任不同 | I23/I28/I29实现已核正常前缀；SEH/OOM/未知owner不能盲目析构/重试/交还Off |
| R13 | 构造/setter/Morph/draw/retire副作用采用/拒绝表、单Hook owner、Host/Off透传、持久保护全窗口 | I27/I32具体接线；不以静音/readonly/最终文件不变替代整链理解 |

主智能体独立读取本地官方源码、工厂/池构造、选定ctor/vtable、各关键cleanup、各代表draw、Slot SetPrize、GetHitListIndex、Laser回调及Effect136 creator链的伪码与必要raw；纠正门值名称、字段含义、分支范围、遗漏类型、十进制/十六进制单位、全局/实例资源owner与ANM交换语义。主审自身早先将伪码`this+16`错写成byte+0x10的中间推断，也经raw `413D15`与官方EntityList.zhl更正为updateEL byte+0x40；最终记录不保留错误采用结论。新结论不抹去17/18和I03的历史失败与当时UNKNOWN。

目录生成器将`entity-native-contracts.json`嵌入`entity-application.json`，CMake显式依赖该研究索引；沿用目录版本1，原有pure-value/native_apply/运行状态分开，未提升native能力。同步修正Text/Dummy的E2E对应为E13和GetHitListIndex的Authority身份事实责任。仅生成资料和人工静态审阅，没有构建、游戏、测试或自动校验程序；本tx的Entity原生适配仍待后续实现，I34统一E2E未开始。
