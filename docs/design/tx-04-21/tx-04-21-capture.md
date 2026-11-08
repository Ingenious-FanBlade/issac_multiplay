# tx-04-21 I06—I10：同窗目标、动态来源、逐身体见证与身份

> 2026-11-07实施补充：原生应用域使用零基Entity=0／Grid=1／Room=2／Player=6，逐窗采样关联保留在同目标WorldDynamic。用户随后要求实施[完整采集链路性能方案](tx-04-21-capture-performance-plan.md)，现已接入同窗类型化值／64项块、单一后台Capture owner、有序控制、精确身体依赖、冷候选、延后回收及完整帧成本统计；未构建／实机复验。跨窗writer闭包未证明，全部必要字段仍FreshRead，`validity.checked`仍指本窗完整读取后的核对。以下原交付时点事实与本补充冲突的线程／编码描述以本补充为准；当前结果见[进度](tx-04-21-e2e-progress.md)。

> 后续进度（2026-11-05）：[I21—I25网络原生安装与确认](tx-04-21-installation.md)及[I23静态研究与主审](tx-04-21-i23-native.md)已完成实现；Release纯编译／链接通过，未测试／启动游戏。I26以后及I39统一验收待继续。下文保留原交付时点事实。

> 后续进度（2026-11-05）：[I16—I20可靠生命周期、预算、时序、Host发布与交接](tx-04-21-publication.md)已实现并经主智能体静态复读，未构建／测试；I21以后与I39统一验收待继续。以下原交付时点的事实保留。

> 后续进度（2026-11-05）：[I11—I15传输实现与主审](tx-04-21-transport.md)已完成，未构建／测试；I16以后和I39统一验收待继续。本文原交付时点的事实保留。

日期：2026-11-05，Asia/Singapore。本轮完成 I06—I10 的实现和主智能体静态审阅。未构建、运行测试或启动游戏；I39 统一 P2 全 E01—E26 后再 P4 全套的安排不变。30/60 Hz 是来源调度机会，尚无实测频率、CPU、带宽或画面一致性结论。任务不另编号，不新增单元测试，不保留 tx/E2E artifact。

入口：[实施清单](tx-04-21-implementation-plan.md)、[I01—I05 基础](tx-04-21-foundation.md)、[采用合同](tx-04-21-native-contracts.md)、[现行规范](../../refactor/project-conventions.md)。

## I06：同窗 Baseline 与缓存适用资格

`mod/src/replica/stream_capture.*::Capture` 是 Host 主线程唯一分层 owner；在正式 consumer 3 的 `LiveBatch` 正常收束后接入。来源 session/epoch、普通 capture、tick、outer、owner、mutation、plan、run/floor/dimension/room 和十一域完整性全部匹配后才公开 `shared_ptr<const Publication>`。来源缺口分别保留 `stream_context_location_missing`、`stream_layered_source_missing`、`stream_mixed_or_incomplete_window` 等原因，不把缺读表示为空集合。

首次建立真实世界 Scope、换 run/room/scene 或显式 `RequestLiveBaseline()` 时取得新的十一域完整冷来源。Baseline manifest 同时持有 Context、Structure、Resource、WorldDynamic、玩家普通联合范围和 `cold-components` 的确切版本；冷组件全部来自本普通窗口，不累积数个 tick 拼接。原生要求域为 Entity/Grid/Room/Player，其余域完整保留；协议域位使用 `domain_id-1`，不是旧 `ClientApplyResult.committed_domains` 的位编号，也不是已经收到原生 Applied ACK。

抑制水位使用本次收束时的 `WorldStorage::EventWatermark()`，包含采集尾部的成员观察事件。每份 Publication 的 `validity` 分别保留版本最初的 `acquired`、本目标的 `checked` 和失效序号；不改写旧版本的 tick 来伪造新采样。当前没有完整 dirty 闭包的低频字段仍在普通窗口作有界读取，**复用的是验证相等的完整范围版本，不是跳过来源读取**。

dirty、来源取消、loss、资源写入和 owner 变化先撤新目标资格，后续完整读取才可恢复适用性。已冻结的 Baseline 与依赖仍是其原采样时刻的历史目标；新冷端/恢复事务应先请求新 Baseline，不能把历史 Baseline 叫作当前 tick。没有按五秒或固定周期重建全域 Baseline。

2026-11-08续验修复：主线程发布信封用采集session／epoch／run／room纯值键区分来源退休与同场景暂缺新候选。后者不取消网络已持有的精确历史目标，前者仍退休路由；活动来源长期未恢复继续使用原stale／recovery期限。Grid逐对象UPDATE只是诊断的`isaac.cs.grid.update`样本，不再当作Lifecycle撤销来源；完整ordinary照常读取Grid当前值，真实Spawn／Destroy／Hurt与native生命周期通知仍保留。`NeedsBaseline`不因普通dirty水位变化把已完成的冷请求重新置为未完成，新run／room与显式请求仍取得新的完整冷来源。

## I07：普通 WorldDynamic 与 Lua 来源分层

2026-11-07起，正式来源经`CopyLuaSource`与`OwnedSource`在主线程封闭为类型化自有值；`read.lua`大型查询结果／图关联每64项封闭，仍来自同一窗口。`stream_source_value.cpp`在后台展开块，`stream_partition.cpp`沿同源成员目录完成原`stream.lua`的分层、采样关联抽取、装备拆分和图闭合，正式路径不再经过Lua JSON。旧Lua分层保留给兼容诊断路径。getter／私有补源仍只按同一合并计划读取一轮；Latest／Recorder需要时从同份来源编码，诊断失败与正式同步分开。`Random.stream`仍为AuthorityOnly，热路径不枚举整套RNG／ItemPool；冷来源／录制继续按原读取合同取得。

固定外层布局由 `CaptureLayouts()` 提供：`stream.context.v1`、`stream.structure.v1`、`stream.resource.v1`、`stream.world.v2` 和 `stream.player.v1`。各 Record 持有已解析自有值树及固定字段编号；逐成员紧凑编码仍归 I11。普通目标保留完整 Entity/Grid/Room 成员、类别、use/shape、空间/姿态、门/机关和对应动态终值。Player 不加入 WorldDynamic 成员；尚未开放 PlayerPost 的私有/容器字段保留在 `body-joint/<key>` 普通联合 Context 中，由同一普通目标依赖。

普通源只在实际普通窗口正常完成后计一份新 WorldDynamic；odd 不补房间帧，重复 capture/tick 不发布。零值、Null、Empty 和完整空成员集合保留；Partial、枚举缺口、取消、源变化和预算错误不产生删除。完整缺席记作 `AbsentFromCompleteMembership;not_physical_destruction`，与正常析构后的物理退休分开。

拆分后的每条 Record/每域 Context 都形成闭合的局部值图：从原来源补入实际存在的 graph 定义，保留环、明确局部节点编号，悬空/冲突/预算失败拒绝发布。不按内容推断对象别名，不做子树去重。通用观察历史按原合同由Sample／validity和冷来源保留；2026-11-07补明并修复：`location.scope/entity_scope`、Player来源／canonical scope／continuity、RoomSave集合source_scope／temporal_relation及Presentation canonical scope保留实际值，分层进入同目标WorldDynamic。稳定范围保留真实身份及业务内容，完整合并后同窗字段仍在原路径供安装器校验，不能删字段或补造新scope。

### 2026-11-07：同tx的构建内存修复

第69轮E02.cold-51的实际失败仍为Host Entity分层分配失败202次、无成功基线、Client等待世界超时；进程private峰值2,133,397,504字节，三池没有预算拒绝。第70轮来源SAX和装备子树修复尚无新run结果，不能据此推断具体地址空间上限或持续泄漏。

本次继续修复纯值后台的副本和容器寿命，不改变完整采集范围、Query参数组合、三池额度或协议：

- 分块展开直接发送到一个SAX构建栈，容器按实际项数一次分配；不先建立每块临时数组再拼接，所有输出字符串仍复制到目标arena。
- 分层小对象按已知成员数取得容量，数组只为真正缺席的槽生成`stream_absent_piece`；有效Null与缺席分别记录，避免为随后被覆盖的所有槽预建小对象。
- 图闭合在分层唯一树上原地排序、规范编号，只复制跨分区缺少的实际定义；原编号的locals/ids、环、冲突和悬空检查保持。装备和采样关联拆分的临时副本使用短命arena，不留在最终范围的arena中。
- ordinary先验证十一域整窗，再暂留Entity/Grid完成全体身份登记；其它域逐个展开、分层和投影。每域投影完成即销毁该域中间arena，身份引用仍在完整登记之后处理。
- Registry投影的数组预留实际槽数、小对象预留已知成员数，避免再次把Query包装扩大到默认16槽。冷模型共用原封套校验但逐域消费，最多保留当前一域的展开封套；最终`components`连同arena封闭移交，不深复制整个组件树。
- 构建失败后尝试收集Store已无依赖的旧候选，避免部分范围已入库的失败重试累积旧版本。arena系统分配失败使用不分配内存的异常消息保留`requested`与该arena的`held`；其含义是实际chunk请求，不代表整个进程可用空间。

E2E冷构建新增`capture-memory:partition`：依次记录capture、tick、domain零基编号、来源序列化字节估计、当前展开arena容量、分层arena容量、外层临时租约、单机毫秒。节点private采样与三池账本仍分别报告，不能互相代替。当前仍保留同窗自有来源、必要最终分层范围及冷组件；单域展开和身份投影也仍有临时内存，不声称已成为零复制系统。代码构建和同一E2E中的实际峰值、完整字段/图、基线发布及Client提交需要分别记录，不提前改判历史失败。

E02.cold-54进一步把失败定位到首对象、仅4 KiB的分层arena。源码复核发现目录生成物还有1168段`std::string + std::string`链：当前MSVC在重新分配时保留各前缀临时值至完整表达式结束；9.88MB目录的全部前缀正文长度和达5.79GB（静态推导，非实际成功分配）。五个目录统一通过`cpp_catalog.py`生成单缓冲reserve/append，避免把文档目录初始化变成进程级峰值。FieldLayer直接查生成的`stream_field_layers.inc`静态表，和Lua策略共用冲突处理、排序及scope映射；字段、编号、目录正文和协议不变。这是同tx失败后的继续修复，是否越过原失败仍以新run为准。

## I08：逐身体 PlayerPost

`Collector::PlayerReturned` 为每个实际受控身体保存当前 outer 的 local/use/shape、binding、tick、mutation 和正常派生更新到达。`BeginOuter` 清当步记录，`RevokePlayer` 清该槽记录；不是从第一身体、processed ACK 或持久的“曾更新过”资格推断其他身体本步也更新。

`WorldPreparePostState` 在 Manager 原链、ordinary/odd 的后置碰撞及临时位清理都已返回后复制候选。读取前后复核同一身体/锚点/roots/use/shape，资源变更和独立失效也必须保持相同；候选总字节上限 16 MiB，单身体读取上限 1 MiB。`WorldPublishPostState` 仅在 Clock 提交后再复核当前证言和失效序号，随后才生成子帧和 Entity/Player 的具名 PlayerPost 来源通知；取消/异常/撤绑/换房不发布。

`CapturePlayerFast` 只读明确白名单的公开 Entity/EntityPlayer 当前标量、Vector/Color/位集及主 Sprite 当前完整姿态。主 Sprite 只读一份，selector、精确 phase、层姿态及 champion 等使用既有 ANM2 来源；资源定义依赖精确 Resource 版本。主 Sprite token 包住读取前后，提交时还需匹配该身体普通 Resource 中具名 `Entity.GetSprite` 的 token，不能拿辅助 Sprite 或同路径替代。关系、库存、按参查询、角色私有完整状态及其他内嵌/Weapon Sprite 的普通联合来源继续保留；I23 不能把此窄 profile 喂给旧完整 Player 安装器。

每个新子帧携带独立 sequence、真实 tick/kind、binding、last_native_update_tick、required_world_sequence 和确切 Structure/Resource/WorldDynamic 依赖。普通帧不冒充 odd，未实际更新的身体只报告缺口，不重复旧样本补频率。`observation status` 分开逐槽 local/use/shape/binding、last_native_update_tick、最后提交来源 tick、ordinary/odd 和 not_updated；Source 计数与真正可发布的 `stream_bodies` 分开。槽号不是网络身份。

## I09：范围版本、资源与在途寿命

Context 按十一域具名，身体普通联合状态按 Key 具名；Structure 和 Resource 独立版本。范围完整值、成员与依赖均相等才复用；WorldDynamic 和 PlayerFast 每次都需新真实样本。装备/关系/形态变化保留完整新范围；当前 Weapon 充能、effects 冷却和 head 动画等与装备成员分开，Sprite 定义与当前姿态分开，不因 HUD/采样 scope 改号使整个 room_save 重发。

`Store::Collect` 保留当前根、已安装值、外部共享值以及其依赖闭包。每份普通/身体 Publication 持有完整自有依赖引用；即使后续换 Scene 或采集 owner 退休，在途者仍持有原不可变值，最后引用释放后才回收。Store 使用 512 MiB（2026-11-07随用户决定翻倍的值池同步）、8192 项和每范围 128 版本保护；全链/Peer 的统一计费仍归 I14，不把本轮纯值保护冒称完整传输预算。

资源通过官方 API ABI 的九个常驻观察 Hook 维护本地失效代次。它是自有 token，不是地址、文件哈希或补造的原生出生证明；可能写入的调用即撤旧资格，原链恰好一次，未采集时透传。

| 入口 | J273 RVA | 原生 ABI |
| --- | --- | --- |
| ANM2 constructor | `0x2B00` | thiscall，无显式参数 |
| construct_from_copy | `0x2CD0` | thiscall，ANM2* |
| destructor | `0x3790` | thiscall，无显式参数 |
| Load | `0x7AF0` | thiscall，std_string& / bool |
| LoadGraphics | `0x7DA0` | thiscall，bool |
| Reload | `0x7C40` | thiscall，无显式参数 |
| Reset | `0x3530` | thiscall，无显式参数 |
| ReplaceSpritesheet | `0x7CD0` | thiscall，int / std_string&，返回 bool |
| Isaac::SwapANM2 | `0x3160` | 官方静态 ArgData：left=ECX，right=EDX；复用 SDK trampoline |

全部经 `binding/hooks.json` 和 `gen_binding.py` 注册；启用 Live 前要求九个实际 trampoline 可用。读取窗口前后/提交前复核资源 mutation 序号，不能混合旧姿态和新实例。地址表上限 65536，只在主线程使用，线上仅保留代次值。

静态审阅中发现并已修复：Swap 必须使两对象换代；destructor 会嵌套 Reset，因此外层析构收口再次删除 token；Live 关闭期间可能同址重建，因此重挂清地址表且不复用代次计数。未知 worker/异常按现有 ActualValueScope 故障路径撤来源，不猜测恢复。

## I10：真实世界、网络身份与退休

Authority session 从当前 Host runtime 取得；进程内 source session/epoch 不再被误当作网络 session。正式 Collector/hierarchy 的 run/floor/dimension/room 是源事实，Capture 为网络分配语义 run/scene，floor/dimension/room 经统一 Registry 映射。无法证明源连续性时建立新语义世界，不据此声称发生了新的原生 Start。等待连接不调用此分配，不把菜单哨兵 Scope 当真实世界；connection/route 准备仍归 I21。

`Registry::DeclareCurrent` 先登记当窗 Entity/Grid 全成员，之后才投影全部关系。Player 本体与 Entity 使用同一 Key；participant、control group、身体和备份槽保留各自空间。普通改值不换 Key，shape 修订单独保留；新 use 分配新 Key，不按地址/seed/slot 复活旧身份。严格模式中缺 use 的对象引用不得创建占位身份，NULL/Unresolved/Retired/CrossEpoch 等状态保持区别。

仅身份表已经 `destroyed`、正常原生 owner 完成的事实进入物理退休表；run teardown 的 Retiring 或控制撤销不被标成析构。退休项带 Key/use/shape、源 epoch、事件序号、outer、scene 与原因，进入完整 Structure；同场景最多 16384 项，耗尽明确停止目标。网络/来源身份和序号不回绕。离表再回到成员的活对象仍可保留原 Key，退休源不能重新声明 Current。

## 逆向、主审与后续

按原版本地 Wiki `docs/enums/ModCallbacks.md`、`docs/Sprite.md`，REPENTOGON 本地 Wiki `docs/docs/enums/ModCallbacks.md`，再到官方 `CustomCallbacks.cpp`、`HookSystem.cpp`、`libzhl/functions/ANM2.zhl`、`Isaac.zhl` 及现有来源/消费者阅读。一个 **GPT-6 Luna max** 子智能体完成必要 J273 定点逆向，主智能体复读字节、关键指令/伪代码和官方 ABI 后采用。

- odd `0x6F9559` 后仍有 `0x6A6A40` 与临时位清理；ordinary `0x4146AB` 后有 `0x4146B2`，`0x41501C→0x415880` 内仍有 `0x416165` 碰撞。Manager 尾读晚于这些路径，不在 PlayerUpdate 返回瞬间复制最终空间姿态。
- MC_PRE_PLAYER_UPDATE true 的疑问由官方源码闭合：HookSystem 以 greater<int> 先安装高优先级内层，`player_native=10000` 位于默认 0 的 MC_PRE 内部，外层 -10000 持 PlayerScope。MC_PRE true 不调 super，因此不到达内层见证；不保留已被源码排除的 skip UNKNOWN。
- 主审复读了九个资源入口字节，`0x403160` 的字段交换/层 owner 重绑，以及 `0x403790→0x403530` 的嵌套 Reset。更广的 inline/raw writer 闭包、全部身体实际更新频率、异常/worker 实况均未外推为已实测；仍需 I39 覆盖具体官方路径。

后续 I11 使用 `CaptureLayouts()` 和现有精确来源做紧凑编码；I19 从 `CurrentLiveWorld()`、`CurrentLiveBaseline()`、`CurrentLiveBodies()` 取共享值，不重新扫描 Peer 世界；I21 分配连接路由，I22/I23/I24/I25 完成原生应用和真实确认。本轮没有网络 Range 发布、zstd、客户端快写或 Applied ACK 的完成声明。统一构建/测试只在 I39，不另开分项试跑。
