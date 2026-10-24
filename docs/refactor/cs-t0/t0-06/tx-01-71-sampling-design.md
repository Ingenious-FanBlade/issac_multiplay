# tx-01-71：采样接入、读取规格与结构化比较设计

日期：2026-10-18。本篇回答 [tx-01-70](tx-01-70-static-analysis-handoff.md) 的 B01–B08、C01–C06、D01–D06。证据来自当前仓库源码、tx-01-63 至 tx-01-69 的当前 EXE 研究及新增旧源码参考；方案尚未实现。

执行要求见[现行规范](../../project-conventions.md)。不计算/校验哈希，不运行游戏、构建或代码测试；本篇不是额外验收。原生生命周期及诊断生产链由同轮子智能体另行研究，最终合并与复核由主智能体完成。

## 1. 证据和设计标签

- `CURRENT_SOURCE`：下列当前仓库源码直接支持；不等于当前进程已加载该代码。
- `STATIC_CONFIRMED`：引用既有/本轮当前 IDA 指令证据。
- `REFERENCE_VERSION_SOURCE`：旧 REPENTOGON 源码事实，仅作线索或反例。
- `DESIGN_PROPOSAL`：本篇确定的实现草案，不改现有协议。
- `UNKNOWN_RUNTIME`：静态材料不能证明的实际覆盖、并发、性能或无修改等性质。

本轮保留[参考文档快照](../../../../artifacts/cs-t0/tx-01-71-static/reference-source-static.md)、[12 个源文件片段](../../../../artifacts/cs-t0/tx-01-71-static/source-evidence.json)和[静态源码导出脚本](export_tx_01_71_source.ps1)。源码证据包含原路径、原行号和版本标签，不使用文件哈希。初次导出在 PowerShell 单元素嵌套数组被展开时失败，已创建的参考快照保留；修订脚本后仅新增源码 JSON，没有覆盖快照。

## 2. B：接入的具体回答

### B01：最小观察集合

`CURRENT_SOURCE`：[UpdateHook](../../../../cs-t0/native/src/loop_runtime.cpp:1002)、[LoopGameplayHook](../../../../cs-t0/native/src/loop_runtime.cpp:1020)、[StandardRoomHook](../../../../cs-t0/native/src/loop_runtime.cpp:1248)、[PostUpdateHook](../../../../cs-t0/native/src/loop_runtime.cpp:1027)已经包住所需四层。普通完整更新阶段沿用 tx-01-66 的本体依据。**阶段观察本身不需再安装相同地址的第二套 native Hook**；应在现有 wrapper 内给同一 owner 增加内部观察调用。

| 既有 wrapper | native call → target VA | 当前声明/数据 | 建议新增的内部观察 |
|---|---|---|---|
| UpdateHook | `0x9312EC → 0x9AB6D0` | `void __cdecl()`；返回 `0x9312F1` | 创建/销毁 outer token；normal return 后复制、随后调用原 TopExit observer |
| LoopGameplayHook | `0x955231 → 0x6FADC0` | 当前 `ThisVoidFn` / fastcall adapter，ECX=self | 同 token 标记标准分支 owner 进入/正常返回 |
| StandardRoomHook | `0x6FBA4D → 0x802980` | 当前 `ThisVoidFn`，ECX=Room；返回 `0x6FBA52` | Room 前后标记；保留 Authority pre/post；记录实际提交凭据 |
| PostUpdateHook | `0x6FBBA5 → 0x8607A0` | `void __cdecl()`；返回 `0x6FBBAA` | 以返回地址和 token 标记标准尾部正常完成 |
| EntityListHook（辅助） | `0x803E70 → 0x4186C0` | `ThisArgVoidFn`，ECX=self、一个 raw32 栈参数 | 只把 arg=0 的同次正常返回作为辅助观察；不替代 Room/tail |
| StandardBodyHook（辅助） | `0x6FAE09 → 0x708630` | 当前 `ThisVoidFn`，ECX=self | 这是标准分支内部另一个调用；不与外层 `0x6FADC0` 混同 |
| 既有 LifecycleObservation | NewRoom/alternate/RoomLifecycle/RoomChange/teardown | 见 B06 | 进入即增加本地 mutation epoch，并永久污染当前 token |

这些调用点的原字节、类型和 target 数组也可在[当前源码](../../../../cs-t0/native/src/loop_runtime.cpp:43)与 tx-01-66 证据中逐项读取。表中 ABI 是当前 wrapper 与既有指令研究支持的最小签名，不承诺未知异常/任意调用者。新增身份观察入口另需生命周期报告中的精确 ABI；不能把旧 RGON 的 wrapper 声明当成新 native ABI。

### B02：同次完整标准更新

`DESIGN_PROPOSAL`：使用主线程 TLS 的有界调用记录栈；outer 调用开始分配本地单调 `outer_token`，记录 `loop_iteration` 只供诊断。token 状态至少包含：标准分支 owner 进入/正常返回次数、普通 Room 进入/正常返回次数、标准尾部进入/正常返回次数、Authority 提交记录、进入时 mutation epoch、是否发生生命周期/重入/丢事件。

只接受已确认调用点的返回地址；Room、tail 必须属于同一个 outer token，次数各为一次、正常嵌套且原生标准分支 owner 正常返回。外层 normal return 时先消费本 token，复制只能发生一次；pop 后不能把标志留给下一次调用。Room/tail 缺失、重复、生命周期进入、嵌套 outer、栈容量不足均输出明确缺口。开始时先清除可消费资格；异常退栈没有正常完成记录，不产出完整样本。已识别的异常停用采样关联，不替原函数处理异常。

原实现仅有 `PhaseScope` 保存/恢复阶段与深度，[ObservationContext](../../../../cs-t0/native/include/isaac_cs_t0/loop_runtime.hpp:18)没有这些 token/完成字段；因此本方案需要实现，不能把现有阶段枚举当完整性证明。任意第三方控制流改写和所有异常退栈仍非静态闭合。

### B03：TopExit owner 与调用次数

`CURRENT_SOURCE`：UpdateHook 在 `g_update()` 后调用单个 `g_top_update_exit_observation`；[coop 初始化](../../../../cs-t0/native/src/coop_active_runtime.cpp:1333)已把它注册为 TopExit。后者会创建/注册虚拟设备、处理 B 退休、暂停/恢复与停止，见 [TopExit](../../../../cs-t0/native/src/coop_active_runtime.cpp:1150)。采样放在它之后会把这些 observer 副作用混入阶段。

`DESIGN_PROPOSAL`：由 loop owner 在 UpdateHook 的原调用正常返回后、既有 TopExit observer 前调用一个内部只读 collector。不要重新注册覆盖这个单槽。若需要可选 consumer，新增独立的内部 sampler 槽/组合 owner，生命周期与现有注册一起安装、停用和清理；不改正式 IPC/Lua API。每个 wrapper 的 original 沿原代码路径恰好调用一次，观察失败只改变 collector 自有状态。

### B04：tick 对齐

`CURRENT_SOURCE`：[Authority](../../../../cs-t0/native/src/coop_active_runtime.cpp:1106)在有效 pre 中设置 `g_tick=g_completed+1`，有效 post 中执行 participant.End、提交 `g_completed=g_tick` 并清 `g_tick`；post 发布 detail=1 为普通 Room，detail=2 为 Player-only。[PlayerOnlyUpdate](../../../../cs-t0/native/src/coop_active_runtime.cpp:778)确有独立提交路径。因此 `completed_tick` 增长不证明世界完整更新。

`DESIGN_PROPOSAL`：Authority 普通 post 完成时，在同线程主动向 collector 复制 `{outer_token, completed_tick, kind=standard_room, room_scope_token}`，普通 Room wrapper 再记录正常返回。不要后台轮询共享 completed_tick，再把“最近的值”贴给样本。当前这几个变量在 coop 的匿名 namespace 中，现有公共 ObservationContext 不包含它们；应增内部值传递接口。

若 Room 完整但 coop 未启用或 pre 被阻断，可输出 `authority_tick=unavailable` 的阶段样本，不能声称 Authority 对齐。odd 提交、暂停无提交、生命周期窗口分别输出 gap，不制造实体状态。`capture_seq` 只数实际完成复制，`attempt_seq` 数候选尝试，`outer_token` 数观察到的顶层调用，三者与 Authority tick 分开。所有自有u64代次/序号在达到最大值前报告耗尽并退休该观察域，不回绕成旧身份。

### B05：线程依据

`CURRENT_SOURCE`：[EstablishOrCheckMainThread](../../../../cs-t0/native/src/loop_runtime.cpp:382)由 clock anchor 建立线程，安装半帧时非 clock hook 在未建立线程前透传；错误线程会 ReportUnsafe。coop 的 MainThread 再核对该值。collector 只在这些条件满足、owner 健康且同线程正常返回窗口直接复制。

这证明所选 wrapper 的检查逻辑，**不证明全进程不存在 worker writer**。对象复制与落盘分离；不通过暂停线程或调用游戏同步函数来“创造”静态保证。支持范围为既有正常主线程顺序路径；worker/未知 Mod 并发为 `UNKNOWN_RUNTIME`，未来整体 E2E 如出现结构变动或读失败，输出不完整并停止相关关联。

### B06：生命周期作废

`CURRENT_SOURCE`：[LifecycleCallback](../../../../cs-t0/native/src/coop_active_runtime.cpp:1211)区分 teardown（退休 session）与 alternate loading/room 嵌套；loop 的 alternate tracker 仅在 owner 内实际 state-apply 时打开 envelope，不能把每次 owner 调用都算 loading。

`DESIGN_PROPOSAL`：collector 独立记录 `mutation_epoch`、`room_epoch`、`run_epoch`、活动窗口深度和健康状态。任一真实生命周期进入即污染当前批次，即使退出后 Game/Room 指针恢复原值仍不可复活该批。NewRoom/RoomLifecycle/RoomChange 的嵌套合并为一次房间过渡事务，不逐信号重复加 room 代次；最外层正常退出后再开始新观察域。alternate 完成同样不能单凭指针相同延续旧连续性；teardown 退休 run 与全部本地关联。未配对退出/重入异常保持不可用，新的明确边界才能重新 bootstrap；持久对象物理延续与房间采样代次分开，依据 A 组设计决定是否保留本地 object_id。

采样前后复制并核对 root/Room 与 epoch 只能发现已观察变化；指针相同不证明未发生变化，epoch 也不证明未漏掉全部未知路径。这两者都不是内存一致性锁。

### B07：安装、容量、冲突与恢复

`CURRENT_SOURCE`：[RelativeCallPatch](../../../../cs-t0/native/include/isaac_cs_t0/hooks.hpp:10)只描述 5-byte rel32 call；[Prepare/Apply](../../../../cs-t0/native/src/hooks.cpp:174)计算 displacement 并改 call+1 的 4 bytes，没有通用 prologue trampoline 或指令重定位器。新候选必须明确是完整 `E8 rel32`，所有 native 参数已恢复；不能把函数入口前五字节直接塞入这个结构。

[有序事务](../../../../cs-t0/native/src/hooks.cpp:520)按连续 ordinal 准备、暂停 peer、安装，失败逆向回滚；恢复反向执行。[当前 loop 容量](../../../../cs-t0/native/src/loop_runtime.cpp:34)为额外 relative=17、pointer=12，coop 已占 12/7、ordinal 32..50，总计 51 槽。现有空余只有 5 个 relative 和 5 个 pointer，新增身份方案必须据实际总数扩展固定容量/目录/恢复计数，不能静默截断。

阶段内部观察不增槽；身份入口若需原生新 patch，列入同一 owner 的后缀和同一有序事务。既有目标字节已被其它 owner 改写时报告冲突，不覆盖或自行链接未知 hook。原始指令比较是补丁正确性检查，不引入哈希。

恢复顺序：关闭 capture/publication → 退休并固定自有队列 → disarm observer → 逆序恢复新增/coop 后缀 → 等待在途 callback → 恢复并 drain 既有前缀 → 按既有常驻策略保留模块与设备资源。[RestoreEmbeddedInputHooks](../../../../cs-t0/native/src/loop_runtime.cpp:1642)和 [coop shutdown](../../../../cs-t0/native/src/coop_active_runtime.cpp:1345)已有相应分层。保持 `strict_slot_snapshot_before=nullptr`，不进入仍存在的 legacy SHA 路径。本轮不安装或恢复任何 Hook。

### B08：观察失败

所有 collector 回调为 noexcept、有限工作、不调用游戏接口；错误只写自有错误记录。未启用/错误模式/主线程未知时 original 照常执行；token 不全/读失败/缓冲不足时输出缺口。collector 自身失效不调用 coop::Fail，不停止 scheduler，不改原返回值，不阻塞等待后台。既有 coop 的真实失败策略保持现状。是否真正满足 original 调用次数和时延只能在实现后的统一 E2E 中观察。

## 3. C：可直接实现的读取规格

### C01：来源、别名与成员资格

`DESIGN_PROPOSAL`：主来源为 Room.EntityList 的 main/persistent/update；render/effect/temp/bufferTemplate/wisp 同步保存描述与成员视图，用于保留只在特殊来源出现的对象和解释过滤，不把内部模板/暂存等同活跃世界。Player roster、四个 Esau 槽、已确认 SubPlayer/Twin/backup/replaced 关系为保留来源；Grid 448 槽与 8 门视图独立读取。所有来源都要报告成功/失败/空，不要求其并集代表全进程所有已分配对象。

单个批次以地址查本地映射合并重复来源，保存所有 `(source_kind,index)`；这只是同一同步窗口的别名合并。跨批次延续必须用生命周期方案，不能复用地址去重结论。保留对象与 active/room membership 分开；池槽和保存结构不当世界实体。未知类别输出 `UnknownEntity` 及可确认基类前缀，不套专用字段、不会按 Type>9 自动归 NPC。未知 native 类型/损坏 vptr 不盲读派生布局。

### C02：读取表

以下为实际字节复制清单；偏移由 [tx-01-69](tx-01-69-offset-reconciliation.md)及其引用的当前研究支持，完整证据以原报告为准。表中偏移均为十六进制，斜线后的长度均为十进制 bytes。所有整数字段先保留 raw 位串；派生值另外标记来源。

| 域/基址 | 偏移与字节 | 输出/条件 |
|---|---|---|
| 全局 Game 指针槽 | RVA `0x871678`，4 | 先取指针；槽本身不是 Game |
| Game | Room `+18300/4`；PlayerManager `+1BAA8` 内嵌；Factory `+26580/4` | 记录 root 可用性；Factory仅诊断可选，不遍历全部池 |
| Room | width `+C/4`、height `+10/4`；EntityList `+1218` 内嵌 | width 只用于已确认的派生 Grid 坐标 |
| Room Grid/door | `+24`，448×4；`+724`，8×4 | 独立来源槽号，含 null；不要按旧447声明截断 |
| EntityList | EL 位于 `+20/30/40/50/60/70/80/90`，各16 | 每 EL 为 flag `0/1`、data `4/4`、capacity `8/4`、size `C/4` |
| PlayerManager | vector `0/12`，begin/end/cap为 `0/4/8`；Esau `80/16` | vector 长度由指针差除4计算，不读 `+C` 作 size |
| Entity 公共头 | vptr `0/4` 仅分类诊断；Index `20/4`；Type/Variant/SubType `28/12` | 当前分类与出生时冻结描述分开 |
| Entity 状态 | Flags `168/8`；valid/visible/exists/dead `170/4`；removedByFactory `177/1` | 拆成各 byte 和独立状态，不推导生命周期 |
| Entity 运动 | Position `33C/8`；Velocity `360/8` | 各2×f32原位串，无 setter 或 Vector getter |
| Entity HP | `380/4`、`384/4` | f32；各类按用户接受的生命表示假设，Player真实心量另记 |
| Entity seed/RNG | DropRNG `3DC/16`；InitSeed `3EC/4` | DropSeed派生自复制的RNG首dword，不能推进RNG |
| Entity 引用 | Parent/Child/Target/Spawner `3BC/16`，各4 | 复制槽值而不构造 EntityPtr；逐关系类型解析 |
| Entity 生成者分类 | SpawnerType `34/4`、SpawnerVariant `38/4` | 与引用目标分类独立，不推断同代 |
| Entity FrameCount 辅助 | birth counter `328/4`；Game frame `264F8/4` | 按tx-01-67 raw32模减派生；不是 Authority tick |
| Player 诊断/关系 | PlayerType `13C0/4`、PlayerIndex `161C/4`；SubPlayer `1D98/4`、Twin `1E68/4`、backup `1E6C/4`、replaced `26C8/4` | SubPlayer依据tx-01-64的`0x6DAE10`；关系非空不独立证明可解引用 |
| Player 心量 | `1340/1344/1348/134C/1350` 各4；Bone `1D88/4`、Golden `194C/4`、Broken `1DA0/4`、Rotten `1DA4/4` | 逐字段，不合并成一个未经证明的HP公式 |
| Player timer | DamageCooldown `13BC/4` | 与 PlayerType 位置分开 |
| NPC | StateFrame `410/4`、State `B64/4`、ProjectileCooldown `BA0/4`、Delay `BA4/4` | 只在确认NPC域读取 |
| Familiar | Player `410/4`、FireCooldown `D4C/4`、State `D88/4` | Player为独立裸引用，非EntityPtr保证 |
| Effect | State `450/4`、Timeout `45C/4`、LifeSpan `460/4` | 不统一timer单位/每帧递减语义 |
| Pickup | Timeout `540/4`、Wait `544/4`、State `584/4` | 分类/状态可变，不据变化判新生 |
| Tear | TearFlags `428/16` | 独立128位；专用State/Timeout未确认 |
| Projectile | ProjectileFlags `438/8` | 独立64位；专用State/Timeout未确认 |
| Grid 公共描述 | Type/Variant/State/SpawnCount/SpawnSeed/VarData 为 `4/8/C/10/14/18` 各4；Initialized `1C/1`；VariableSeed `20/4` | 偏移直接相对Grid；不再重复加Desc+4 |
| Grid 元数据/RNG | GridIndex `24/4`、SpawnFrame `28/4`、RNG `2C/16`、CollisionClass `3C/4` | 来源槽号与GridIndex并列；不读取Entity HP/flags/引用 |

Grid 坐标可在 width>0 且已确认索引范围内由 `x=40*(slot%width)+40, y=40*(slot/width)+120` 推导，明确 `derived_from_source_slot`。Grid 没有确认的 Entity Velocity/HP/owner/stateflags 时标 `NOT_APPLICABLE` 或 `UNCONFIRMED`，不填零。Player/Tear/Projectile 通用专用 State/Timeout 的未确认状态也必须保留。

### C03：初始化与可用性

枚举成功仅证明这一来源槽被读取。分别保存 `membership`、`lifecycle_status`、`init_evidence`、`field_status`；valid/exists/dead/Grid.Initialized 只作原始属性，不能任一为真就升级完整发布。事件记录确认正常 Init/发布可加强该代对象的可用性；bootstrap 没有该历史，标 `FIRST_OBSERVED`。仅确认头部/初始化不明时保留对象记录和原因，专用字段 `NOT_READY`，不要丢弃对象或填默认值。

字段状态固定：`VALUE`、`UNCONFIRMED`、`NOT_APPLICABLE`、`NOT_READY`、`READ_FAILED`。对象缺来源/身份歧义不是值0；NULL引用是已读取到的实际0，与未知引用不同。

### C04：边界、读失败与预算

EL：确认 size<=capacity、`size*4`及地址相加不溢出；size>0要求data非空，空但非空data可保留；descriptor在复制前后若变更则批次不完整。vector：begin/end/cap全空表示空；否则检查4-byte对齐、begin<=end<=cap、两段差值可整除4，按end-begin枚举。Grid固定448与door8，不从width*height替代槽数。

`DESIGN_PROPOSAL`：预分配每批16384个动态对象、65536项membership、最多456项Grid/door来源、64个Player关系节点；Player关系有界worklist+visited防环，达到上限显式 `RELATION_BUDGET_EXCEEDED`。这些是原型资源预算，不是游戏上限；超过预算标 `INCOMPLETE`，保存需求/已复制数，不声称完整世界。自有身份表/事件队列容量独立，溢出会使连续性未知，不能只丢日志继续认定key有效。

在已授权native读取窗口内，采用有限字节只读复制并捕获可识别访问失败；可读页面/SEH不证明对象仍同代或字段已初始化。复制前后检查不消除未知writer的竞态。任何错误保留其已知范围，失败源不可当空源参与相等比较。

### C05：禁止隐藏写入的读取路径

collector不调用 Lua GetRoomEntities/FindByType、空间查询、EL Untie/RemoveAt、EntityPtr setter/构造、RNG操作、EntityPlus lazy getter、ESSM保存/检查修复、Spawn/Morph/Update或虚getter。旧源码参考直接说明这些接口可能分配、登记引用、清理/修复或消费RNG；当前 tx-01-66/68 已另证部分原生query/getter副作用。

读取只使用明确偏移的值拷贝；去重、关联、规范化只作用于自有数据。若原生字符串/序列化helper是否纯读未知，不把它加入collector调用图。诊断生产者的静态研究不授权执行它。

### C06：缓冲交接

预分配至少三个批次buffer，状态为 `FREE → WRITING → READY → READING → FREE`；单游戏线程仅写WRITING，自有consumer线程仅处理READY/READING。发布READY前完成全部native对象字段和引用原始槽复制，release发布/acquire接收。后台只能读值副本、本地object_id和诊断地址数值，不能解引用native地址。

无FREE时记录 `BUFFER_BACKPRESSURE`，本轮不复制、不等待后台；capture序号不伪增成功。关闭publication后drain callback再结束worker的自有buffer消费，owner仍按常驻策略处理。内存预算与实际时延属于实现后的观测。

## 4. D：结构化记录与比较器草案

### D01：头、对齐与作用域

`DESIGN_PROPOSAL`：使用内部 domain=`isaac.cs.t0.world-record`、version=1，不新设正式wire/IPC。头至少包含：本地session标识、run_epoch、room_epoch、mutation_epoch、attempt_seq、capture_seq、outer_token、phase=`POST_STANDARD_UPDATE_PRE_TOP_EXIT`、Authority `{status,tick,kind,scope_token}`、source状态/计数、对象/字段/事件完整性与错误列表。记录进程地址/thread/loop在独立diagnostics域。

跨进程比较要求显式指定两个本地run/room观察域及Authority tick/阶段映射；本地代次序号相同不构成语义相同证明。输入seed/场景标签只作声明，不伪造读取了尚未确认的全局seed字段。没有映射、阶段不同、只有odd提交或缺完整样本，输出 `ALIGNMENT_UNAVAILABLE`/`SAMPLE_GAP`；不能拿最近一帧顶替。

### D02：编码、排序和浮点

自有buffer字段用固定编号与位宽；如持久化二进制则little-endian，所有变长项前置u32长度并检查溢出，禁止原生C++ struct padding直接落盘。JSONL作为可读输出：u64序号用十进制字符串避免IEEE754精度损失；raw32/raw64的hex字符串用固定8/16位、高位在前，例如f32的1.0为`3f800000`；raw128和其它bytes用按内存地址升序的固定宽`bytes_le`，不推断BitSet内部编号。字段编号固定、按编号升序；实体按已建立match key排序，同key冲突保留整个组且不按地址打破平局。无key对象保留本地诊断序号，不强造跨进程排序。

f32保存原raw32；有限性由位级exponent检查，不调用会影响native浮点环境的游戏helper。比较视图将+0/-0统一为+0，原位差异仍可在diagnostic输出；其它finite值默认精确位比较，不量化、不加epsilon。NaN/Inf保存原位串并报 `NONFINITE`，不能把两次NaN归成可比较的0。mandatory字段非finite/未知时整体结果不为equal；可输出其它字段的已知差异。

字段manifest明确 `required_for_profile`。最小profile只纳入已有静态依据的字段；未确认的专用字段必须出现在schema覆盖说明，不能为得到equal而逐样本删字段。Grid不适用字段与Tear尚未确认字段含义不同。

固定profile草案为`min-read-v1`：七类已确认Entity要求公共`0001..001E`；Player另要求`0101..0110`，NPC另要求`0201..0204`，Familiar另要求`0301..0303`，Effect另要求`0401..0403`，Pickup另要求`0501..0503`，Tear另要求`0601`，Projectile另要求`0701`；Grid要求`0801..0811`。HP要求基于用户接受假设，manifest必须携带此标签。身份/引用映射、来源完整性与Authority/phase对齐是整体可比较条件；有原始引用值仍不等于目标可匹配。未确认的Player/Tear/Projectile专用State/Timeout作为覆盖声明中的`UNCONFIRMED`，不占用其它类字段ID、不纳入本profile的相等结论。任一required字段`NOT_READY/READ_FAILED/UNCONFIRMED`或必要关系未解析时，不输出整体相等；不得临时缩小profile。未知类别仅诊断基类已知前缀，不声称满足此profile。domain固定排序为Player、NPC、Tear、Projectile、Pickup、Familiar、Effect、Grid、UnknownEntity；同类再按match key/field_id排序。

v1 字段编号草案如下；raw/derived、对象域和宽度以 C02 表为准，未列出的扩展不得占用既有编号：

| field_id（hex） | 固定顺序 |
|---|---|
| `0001..0004` | Entity Index、Type、Variant、SubType |
| `0005..000A` | EntityFlags、valid、visible、exists、dead、removedByFactory |
| `000B..0010` | Position.x、Position.y、Velocity.x、Velocity.y、HP、MaxHP |
| `0011..0015` | InitSeed、DropRNG.word0、word1、word2、word3 |
| `0016..001B` | Parent、Child、Target、SpawnerEntity、SpawnerType、SpawnerVariant |
| `001C..001E` | Entity birth_counter、同批Game frame_counter、派生FrameCount |
| `0101..0103` | PlayerType、PlayerIndex、DamageCooldown |
| `0104..010C` | Player Max/Red/Eternal/Soul/Black/Bone/Golden/Broken/Rotten hearts |
| `010D..0110` | Player SubPlayer、Twin、backup、replaced |
| `0201..0204` | NPC StateFrame、State、ProjectileCooldown、ProjectileDelay |
| `0301..0303` | Familiar Player、FireCooldown、State |
| `0401..0403` | Effect State、Timeout、LifeSpan |
| `0501..0503` | Pickup Timeout、Wait、State |
| `0601` / `0701` | TearFlags / ProjectileFlags |
| `0801..080A` | Grid Type、Variant、State、SpawnCount、SpawnSeed、VarData、Initialized、VariableSeed、GridIndex、SpawnFrame |
| `080B..0811` | Grid RNG.word0..3、CollisionClass、派生Position.x/y |

引用 field 的可比较payload为关系状态与目标match key，而非C02的native4字节地址；原位值只留diagnostics。vptr同样不分配可比较field_id。class/identity/membership属于对象记录头，不挤占上述native字段编号。

如实现二进制副本，field TLV固定为 `{field_id:u16, status:u8, encoding:u8, byte_length:u32, payload:byte[]}`；status=`VALUE:0, UNCONFIRMED:1, NOT_APPLICABLE:2, NOT_READY:3, READ_FAILED:4`，非VALUE payload为空。encoding=`RAW_LE:0, F32_BITS:1, REFERENCE:2, DERIVED_RAW32:3`；REFERENCE按`relation_status:u8 + target_key_length:u32 + target_key_bytes`编码，只有KNOWN_CURRENT可带key。对象/字段/来源计数分别使用u32，所有计数与长度必须在接收时先检查再分配。JSONL可直接表达相同字段，不强制先实现二进制IPC。

### D03：身份和引用的比较表示

本地身份 `(run_epoch, object_domain, local_object_id, use_generation)` 与跨进程候选match key分开。地址/vptr/容器指针/thread/墙钟只诊断，不参与状态相等。

候选匹配使用**冻结的出生事件描述**：对象域、已对齐的创建事件序号、创建kind、出生分类和seed（若当时确认）；当前分类/seed是快照属性，不随Morph重写出生key。该候选只在两边事件起点、顺序和范围已对齐时可比；静态不能证明它天然跨进程一致。Player可额外使用既有session冻结ordinal作语义锚，但只覆盖已确认的活跃A/B，不扩展到所有备用Player。

bootstrap对象仅有 `FIRST_OBSERVED`，可以报告唯一指纹候选供诊断，不把“本批唯一”自动升级为跨tick身份。引用记录 `kind` 与 `NULL/KNOWN_CURRENT/UNRESOLVED/RETIRED/AMBIGUOUS_REUSE`；只有事件次序支持同代且对象被匹配时转成目标match key。未解析裸地址不会参与跨进程比较。

### D04：匹配、冲突和不完整结果

先核对schema/profile和对齐，再按已证明可用的match key分组。组内两边各一个才做实体逐字段比较；0/1输出removed/added，但仅当该来源/批次覆盖完整，否则输出 `PRESENCE_UNKNOWN`。多对象同key报 `KEY_COLLISION`，保留冲突组、两边分类/seed/来源与事件依据，不用地址补key。对象身份连续性未知报 `IDENTITY_UNRESOLVED`，不得只因位置相近强配。

比较结果分三态：`DIFFERENT`（至少一项确定差异）、`EQUAL_WITHIN_PROFILE`（全部要求字段与对象覆盖可比且相等）、`INCOMPARABLE`（其余）。可同时报告确定差异和未比较区域；DIFFERENT不代表其它未知项已闭合。相同缺项、相同READ_FAILED或相同冲突不是equal。

### D05：分层与首次差异

按 run→room→Player/Entity class/Grid→match key→field_id 输出层级计数及首个差异路径，不生成总hash。首次按**已对齐Authority tick**、固定domain顺序、match key、field_id排序；若更早存在gap/无法比较，写 `first_observed_difference` 并附 `earlier_coverage_unknown=true`，不能声称全局首个因果分歧。

JSONL输出分为 `capture`、`gap`、`lifecycle_event`、`key_collision`、`comparison`。例如：

```json
{"record":"gap","attempt_seq":"12","outer_token":"19","reason":"ODD_PLAYER_ONLY","authority_tick":"31","world_capture":false}
{"record":"comparison","status":"DIFFERENT","tick":"32","path":["run","room","NPC","birth:7","position.x"],"left":{"raw32":"3f800000"},"right":{"raw32":"40000000"},"earlier_coverage_unknown":false}
{"record":"comparison","status":"INCOMPARABLE","tick":"34","reason":"KEY_COLLISION","domain":"Entity","members_left":2,"members_right":1}
```

例子中的 `birth:7` 只是已建立映射后的展示字符串，不是native存在的字段；数值不代表本轮运行观察。报告同时保存manifest、原始样本和对齐映射，便于复现逐字段结果，不依赖哈希。

### D06：未来统一 E2E 的观察量

只设计，不在本轮实现或执行：

1. 同一冻结值副本重复编码/解析得到同字段、同顺序和同可读字节，保留两次输出及逐字段对照。
2. 只修改自有副本的一个已知字段，比较器定位对应tick/entity/field，原副本与native世界不变。
3. 覆盖bootstrap、同址复用、Morph/Restore、create/remove、Grid/door别名、冲突、读失败/容量不足和生命周期gap，报告状态与事件对应。
4. 连续10,000次完整采集，记录成功/缺口/错误、native调用次数、线程、阶段、容器/RNG相关观测；不能仅凭两张前后图不变证明整个窗口无修改。
5. 同seed/input双进程以明确的范围/阶段/tick映射比较最小profile，保留输入、样本、冲突、首个确定差异和更早未知范围。
6. 停用/恢复时保留原始目标字节、实际owner、在途callback归零及恢复结果，不运行旧digest校验链。

以上最终仍须实现后按现行规范整合为一次整体E2E；本轮没有提前测试，也不把设计示例记作PASS。

## 5. 静态范围的实际限制

本篇已给出B/C/D的实现前方案；它没有新增运行时代码，不证明这些方案已接入或达到预算。阶段普通路径与字段布局有既有当前依据；所有Mod路径、worker并发、实际事件无丢失、异常恢复和跨进程创建顺序仍非静态保证。相关失败输出已明确，不用额外泛查全映像或强配身份来掩盖未知。
