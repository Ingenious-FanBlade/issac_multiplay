# tx-04-08：现阶段待实施的工程改动任务

日期：2026-10-27，Asia/Singapore。状态更新：**A读取/撤销及B故障链路已编写，C已落实可核定参数与episode索引；特殊Replace仍UNKNOWN/禁用，D–G及整体E2E未完成。** 当前交付和剩余范围见[ABC续作](tx-04-08-abc-implementation.md)。下文的基线和缺口描述保留本清单建立时的状态，后续各项交付要求继续适用；不是把本轮前源码缺口继续宣称为未接线。

本文把当时源码中的实际缺口转为可执行任务，作为[迁移主方案](tx-04-08-implementation-plan.md)第10节工程顺序的细化；早先实现现状见[Lua/harness说明](tx-04-08-harness.md)。B完成后V8已替换固定NOT_RUN占位并具备实际证据的正向代码路径；V9补入故障子证据，但完整退出仍待E，V4/V5/V6等完整最终判定仍待F。纯编译不代表运行通过。

本清单继续属于同一个 **tx-04-08**，A–G 只用于索引工作内容，不是新的推进编号、审批或解锁条件。按[现行规范](../../refactor/project-conventions.md)推进：不设 gate、不校验哈希、不要求角色分离；分析、静态确认与代码修改期间不运行测试，不新增实现后的单元测试。目标完成后才由统一入口执行一次整体 E2E，两个游戏进程顺序启动，每次分别告知用户并确认准备。

## 1. 本轮续作前的基线与任务总览

已编写的主体包括：目标 layout/Hook 清单及生成器、world 纯值模型/身份/缓冲/writer、collector/主线程桥/post-state、四项 `cs world` 命令、离线 replay/compare、Lua 通用动作和双进程 harness 主体。最新 Rust harness 与完整 DLL 的纯编译记录见[构建说明](../../../artifacts/repentogon/tx-04-08-harness-build/README.md)。**纯编译成功只说明可编译链接，不等于下列工程项完成。**

特别是 `tools/harness/src/tx08/evidence.rs`：V1–V12 先初始化为 NOT_OBSERVED；V2/V3/V10/V11 已有部分正向判定，但 V1/V4/V5/V6/V7/V9/V12 目前没有完整 PASS 路径，V8 固定 NOT_RUN。因此当前整体没有正常通过的完整实现路径，不能描述成“仅待最后测试”。

| 工程项 | 目标 | 当前缺口 | 对应场景/判定 |
| --- | --- | --- | --- |
| A | 补齐完整采样所需的字段读取依据 | `InitUnknown → UNCONFIRMED → Incomplete` 仍阻止完整样本 | S1/S2/S13，V3/V4/V6 |
| B | 实现限定作用域的背压/故障入口并接通编排 | DLL 无自有故障控制接口，S7/S8 尚不能执行 | S7/S8/S9，V8/V9 |
| C | 落实具体 Lua 场景和原生生命周期记录 | 通用 API 动作已有，具体参数仍禁用；Replace/Restore 路线及 episode 未齐 | S4/S5/S6/S10，V4/V5/V6 |
| D | 补齐复制前后 RNG 与线程/原调用证据 | 仅有 RNG 当前值，缺少目标复制窗口前后对照 | V7 |
| E | 完成直接退出路径的收口协调 | 只有显式 stop→Closed→退出路径，直接关闭/Shutdown 未齐 | S11/S13，V9 |
| F | 完成全部最终判定、跨文件关联与报告 | 多项只有部分证据输出，仍无完整正向判定 | V1–V12、L1–L6 |
| G | 接入完整旧格式材料 | `legacy_capture` 仍为 null，材料完整性与来源未落实到 scenario | L7，V11 |

## 2. 工程项 A：完整采样字段与读取依据

**目标：** 在不缩减固定字段和来源的前提下，使受支持稳定场景中的字段具备真实可用的读取依据，能够生成完整普通样本。

当前 `mod/src/world/collector.cpp::CopyFields` 对 layout 的 `InitUnknown` 字段写 UNCONFIRMED 并调用 `Incomplete`。例如 Entity 的 `removed_by_factory +0x177` 初始值来源仍未知；不能因为位置已知就将初始化或当前值资格视作已确认。现有 Player basis 3 也不自动授予所有 Entity 公共未知字段读取资格。

待改动：

1. 按当前 layout 列出影响受支持稳定场景的未知字段，逐项区分偏移/宽度、初始化历史、当前值读取依据和语义假设；复用 ST3–ST8 已有依据，只补具体缺失问题。
2. 有静态依据时补入 layout/读取规则；若需要新的用户采纳当前值规则，先形成明确的适用对象、生命周期和失效条件说明，不冒称已有授权或已观察初始化。
3. 将成立的规则接入 collector/identity/readiness，失效后撤销。仍未知的值继续保留 UNCONFIRMED/NOT_READY，不能填零、删除字段或降低 complete 条件。

涉及文件：`mod/binding/world_layout.json`、`mod/src/binding/world_layout.*`、`mod/src/world/collector.*`、`identity.*`；需要变更生成格式时同步 `mod/tools/gen_binding.py` 与离线模型。

交付内容：逐字段依据与剩余未知表、相应读取/撤销代码、manifest/profile 一致的状态表达。完整样本和 2,000 次稳定段的实际结果留到统一 E2E，不在本项预跑。

## 3. 工程项 B：背压、故障注入及隔离编排

**目标：** 通过采集器自己的真实失败分支覆盖故障响应，保留 Host/Authority 输入推进和世界记录的独立生命周期。

依据[E2E 设计第 5 节](tx-04-08-e2e-plan.md#5-自有故障的作用域)已有失败方式，补充以下入口及 harness 调用：

| 内容 | 允许的工程实现 | 必须记录的行为 |
| --- | --- | --- |
| 实际背压 | 有界暂停本 DLL raw reader；游戏生产者照常运行 | 真实无 FREE 分支、gap 范围、恢复不补旧世界、主线程不等待 |
| 对象/membership/关系预算 | 当前测试录制使用较低的自有预算，真实枚举触发上限 | 参数、实际计数、失败归属；固定字段集合不变 |
| 读取失败 | collector 自建不可访问区域，经真实受保护 Read 路径失败 | 明确 TEST_INJECTED，不能称游戏出现坏指针 |
| 来源/root 变化 | 改变采集器自有前后期望值，触发真实比较拒绝 | 原始值、注入值和拒绝原因；不改游戏 root |
| 引用歧义 | 自有判定点标记歧义 | 保留原始引用和前后状态，不写 EntityPtr |
| 事件溢出 | 压低自有容量或投递明确的测试诊断事件 | 真实容量失败与记录停止，不伪造出生/析构 |
| 错误线程 | 自有线程仅投递无游戏对象的 observer 测试通知 | 经过线程检查，不调用 Hook/original、不碰身份表 |
| 文件失败 | artifact 内可控失败 sink 或关闭自有文件句柄 | 真实 Write/flush 错误、失败前缀，不能补成功终态 |

入口只在专用 E2E 配置、本次 token 和当前 recording generation 下有效；不加入 UDP 输入协议，不在玩家界面开放任意地址/文件操作。参数检查、线程和所有权边界是运行功能，不新增项目审批字段。

在 `tx08.rs` 中接通 S7/S8：正常关闭 primary 后，每个故障使用独立录制上下文；真实 Closed 后才新建下一上下文。未收口时不释放在途存储，不强制重建、不重装 Hook，后续项记录 NOT_RUN。故障后核对实际 tick/Ack/输入继续，并保留 S9 健康录制，不能用故障终态代替正常 stop 证据。

涉及文件：`mod/src/ops/world_commands.*`、`config.cpp`/`runtime.h`、`mod/src/world/collector.*`、`storage.*`、`writer.*`、必要的 `game/world_bridge.*`，以及 `tools/harness/src/tx08.rs`、`tx08/evidence.rs`、scenario。先在实现说明中列清具体失败语义与参数，再写入口；不单独运行故障批次。

交付内容：受限控制接口、真实失败分支、逐录制故障账本、S7/S8/S9 编排和 V8/V9 的对应判定代码。

## 4. 工程项 C：具体场景、Player 路线与生命周期 episode

**目标：** 将通用 Lua 动作变成带明确对象、参数和原生证据关联的实际场景。

现有 `world_fixture.lua` 已有 Spawn、同动作创建再 Remove、NPC/Pickup Morph、Grid 创建/替换/移除及独立观察函数；默认 scenario 的相关世界动作仍有 `enabled=false`，这部分尚未构成完整场景。Player Replace/Restore 也没有已落实的具体自然触发路线。

待改动：

1. 为七类受支持 Entity、Grid 和来源/door 别名写出场景表：公开 API、具体 type/variant/subtype、显式 seed、位置/速度、对象标签、目标格子、PathTrail、前态及预期后态。区分 API 静态可达和具体参数可用，不由“已有 handler”推导场景完成。
2. 核定受限 Player Replace/Restore 的自然 owner 路线并接入场景。B 继续由 Host/Join 创建，不用 Lua SpawnCoPlayer 绕开 C/S 绑定；未知路线保留 UNKNOWN，不猜 ABI。
3. 把 Lua `action_id/step`、world checkpoint watermark、native call/parent/body、对象 local/use/birth/revision 和成员前后态关联成 episode，覆盖创建 owner 完成、重复发布、Remove/清引用/物理退休、短命、可达复用、Morph、Grid 替换及保留对象。
4. 补齐切房、B 死亡/复活、换层、保存/Continue 中 Player 依据撤销与重建记录。健康、动画完成、唯一重绑与字段可读分别表达，不把同 K/ordinal 当作旧关联仍有效。
5. Lua 视图仅作独立交叉证据，给出其真实回调阶段/witness 与 capture 的关联；不能把 render 或旧 status 快照当成实时 Authority tick。

涉及文件：`tests/e2e-mods/isaac-cs-test/main.lua`、`world_fixture.lua`、`tools/harness/scenarios/tx-04-08.json`、`src/tx08.rs`、`src/tx08/evidence.rs`；现有原始事件不足时补相应生产值记录，不在离线文件里伪造原生事件。

交付内容：可执行参数表、明确动作序列、逐 episode 原始序号与对照索引、剩余不可达/未知清单。不能自然观察到的复用或特殊 Player 路线保留未覆盖，不因此把必需项改成 PASS。

## 5. 工程项 D：复制窗口前后的 RNG 证据

**目标：** 在明确的主线程复制窗口前后保存同一观察集合的 RNG 原始状态，并由统一归约判断相同、不同或不可确认。

已存在的静态依据直接复用：Entity `_dropRNG` 位于 `+0x3DC..+0x3EB`，Grid `_rng` 位于 `+0x2C..+0x3B`，均为四个 uint32_t、共 16 字节。见[ST3 字段笔记](../../research/tx-04-08-st3-agent-notes.md)和[布局表](../../../mod/binding/world_layout.json)。不需要重做这两组偏移/宽度逆向。

待改动：

1. 对新增读取窗口做小范围静态复核：前后端点、来源枚举/字段复制/引用解析/尾部复核的实际顺序，以及期间是否可能进入原生更新、Lua、getter、setter 或 RNG 推进器。明确真正包住哪些操作，未包住的部分不能声称已覆盖；不能仅把单个字段读取前后当成整个目标复制窗口的证据。
2. 定义观察集合与身份连续性，前后关联同 recording/run/room/mutation、outer、线程、对象 domain/local/use。新增、退出、退休、复用、读取失败或前后集合变化要明确记录不可确认，不能对旧地址继续读取后判相等。
3. 只用已理解的有界原始读取，前后各保留全部四个原始 word 和实际读取状态，不调用 RNG/helper/getter，也不只比较 seed。后台只处理 owned values，不再次解引用对象。
4. 将前后值、范围、状态和必要计数接入文件模型、writer、replay 保留规则与 V7 归约；保留原始差异，不能用摘要替代值。
5. Entity/Grid 局部状态不变不等于全游戏 RNG 不变。若进一步覆盖 Game、PlayerManager 或全局 RNG，另补其完整状态、相关索引、布局、访问路径和写入方的静态确认；现有 ST6 seed helper 的局部查询不能代替完整全局状态定义。

涉及文件：`mod/src/world/collector.*`、`model.*`、`writer.*`、必要的 layout/桥接接口，以及 `tools/harness/src/world/`、`src/tx08/evidence.rs`。保持内存、事件和输出预算有界，不把大数组放游戏线程栈上。

交付内容：范围明确的静态说明、RNG 前后纯值记录、失败/变化表达、文件保留与 V7 判定代码。前后相同仅表示两个边界的已记录值相同，本身不能排除中间先改变再恢复；需结合采集路径不调用 RNG 推进器等静态分析解释，只适用于最后整体 E2E 的限定窗口，不外推未知 worker/mod。

## 6. 工程项 E：直接退出路径与真实收口

**目标：** 补齐用户直接关闭游戏及正常原生退出路径的记录停止、在途对象和后台文件收口协调。

当前已编排显式 `cs world stop`、等待真实 Closed 后退出；它只覆盖健康安排。ST8 已确认原生 `Isaac::Shutdown` 在 Manager 循环结束后，不能假设收到终端通知后仍有十秒 Manager 更新可供排空。

待改动：

1. 按 J273 已理解的退出调用链确定可执行的前置关闭窗口，区分直接窗口关闭、原生正常退出、显式 stop 后退出与崩溃，不借用别的路径冒充覆盖。
2. 新增或调整 Hook 时回到统一清单；未理解的 ABI/顺序先补具体静态确认，不另设入口补丁 owner。
3. 关闭新采集和 publication、收束在途 callback/pending、固定最后序号，由后台 reader/writer 排空并实际 flush/退出；主线程保持有界推进，不在 DllMain/loader lock 中 join 或写大文件。
4. 超时、崩溃和无法确认收口时保留 incomplete/失败前缀，不伪造 Closed，不提前释放仍被后台持有的上下文，不强杀 worker 或卸载 DLL。K 的 NEVER 策略不变。
5. harness 区分正常退出与失败后的强制子进程清理，将 writer/result/flush 与实际进程结局共同纳入 V9，补文件失败的非成功判定。

涉及文件：`mod/binding/hooks.json`/生成器（需要新 Hook 时）、`game/world_bridge.*`、现有生命周期/退出桥、`ops/world_commands.*`、`world/storage.*`/`writer.*`、`tools/harness/src/tx08.rs` 及归约。

交付内容：各退出路径与线程所有权说明、相应关闭代码、明确的终态产物和 V9 判定，保留原先健康路径及历史失败记录。

## 7. 工程项 F：最终判定、离线细节与报告收尾

**目标：** 使每项必需功能都有基于实际原始记录的完整判定，补齐正向与失败路径，而不是把占位 NOT_OBSERVED 改成无条件 PASS。

| 判定 | 待补代码 |
| --- | --- |
| V1 | 将实际 Hook/能力/ABI 清单、外层/内层 native 调用顺序与 original exactly-once、未启用/停止后透传关联；当前只有部分配对计数与失败判断 |
| V4 | 七类 Entity/Grid 固定字段的逐类资格、来源及实际 Grid 上限、door 别名的具体证据与缺项判定；不能仅输出字段并集 |
| V5 | 读取工程项 C 的真实 episode，判断 owner 完成、birth/use/revision、成员退出和物理退休、短命/复用/Morph/Replace/Restore/Grid 行为 |
| V6 | 将 FIRST_OBSERVED、真实 Player body、有效 Authority、exact pair/session/participant generation、失效撤销及复活/加载后重建逐条关联；不能只判断见过 basis 3 |
| V7 | 接入工程项 D 的 RNG 前后记录、线程与原调用证据；按实际范围输出差异/未知 |
| V8 | 接入工程项 B 的真实故障记录、背压范围、槽所有权和故障后输入推进，替换固定 NOT_RUN 占位 |
| V9 | 接入工程项 B/E 的正常及失败收口，生产/复制/落盘/未交付计数、callback/worker、drain/flush、Host 继续和各退出结果共同判定 |
| V12 | 联合输入 Submit/Resolve/消费、completed/Ack、Join、K 常驻/退休、loading/Continue 与持久副作用窗口；首尾文件净变化、正常退出本身不能替代整个操作窗口证据 |

已有 V2/V3/V10/V11 分支仍需与完整场景集合接通：所有应检查录制的完整 capture 均关联阶段/tick；第一段 2,000 次及第二段计划 256 次的实际要求参与整体结果，不只生成旁证；完整、gap、缺号、生命周期与窗口边界采用同一规则。不得为避免处理而缩字段、丢记录或降低第二段目标。

离线与报告还需落实：

- L1 保持 primary 全量，不以派生单 capture 材料代替；L2–L5 的自有修改、计数调整和 TEST_INJECTED 归属始终可追溯。
- L3 当前只核对一个 field_id 的差异，还需核对完整 run/room/tick/class/key/field 定位及 earlier_coverage_unknown，不能让同名字段的别处差异通过。
- L6 对齐依据继续核对实际输入安装/消费、scope 与冻结 ordinal，不仅看 CLI 成功或相同 seed；真实配对区域和已知字段比较必须存在，未映射出生继续不可比，不强制世界 equal。
- 汇总实际 copy/队列/写盘耗时、内存峰值、背压及 replay bytes/records/耗时/吞吐/flush；补齐原始记录到最终结论的索引。
- 报告中独立保留执行失败、判定失败、未执行、未观察、清理错误和不可比较区域；任何必需项未完成都不能获得整体 PASS。

涉及文件：`tools/harness/src/tx08/evidence.rs`、`offline.rs`、`tx08.rs`、`src/world/`、scenario，以及生产端确实缺失的值记录。离线错误与成功条件均由同一次整体入口验证，不提前试跑 replay/compare 或负向批次。

交付内容：V1–V12 完整判定、L1–L6 与场景的可靠关联、性能及证据索引、统一 `report.json/report.md/cleanup.json`。存在合法正向代码路径是工程完成要求，实际 PASS 仍只能来自真实整体验收。

## 8. 工程项 G：legacy 原始材料接入

**目标：** 给已编写的 L7 显式导入接入真实、完整、来源清楚的旧格式材料。

待改动：查找并选定完整旧域 `isaac.cs.t0.world-record` 目录，确认 manifest/captures/lifecycle 与应有终态/附属文件齐备，将真实路径和可读来源写入 scenario/说明；当前 `legacy_capture=null`，不能据此断言仓库所有历史材料都不存在，也不能把缺生命周期的 replay 目录当作完整录制。若需从已有归档取回，保留来源与原件，不启动旧 runtime/Helper 补样本。

涉及文件：`tools/harness/scenarios/tx-04-08.json`、材料索引/说明、必要的 `tx08/offline.rs` 接线。保留显式 `--legacy-t0`，不把旧运行 PASS 导入新版本，不允许没有语义映射的跨布局 equal。

交付内容：L7 可复现的输入引用、资料完整性说明和最终统一入口的调用记录；没有完整输入时如实 NOT_RUN。

## 9. 实施顺序、唯一整体 E2E 与本文范围

建议先处理 A 的字段依据，B/C/D/E 可按实际依赖交错推进，随后完成 F 并落实 G。这个顺序用于减少返工，不要求先签署报告或取得“解锁”；需要新增字段、退出点位或全局 RNG 布局时，只补对应问题的静态确认，不重做已完成的 ST1–ST8。

A–G 的实现和必要纯编译链接完成后，使用同一个 `harness e2e tx-04-08` 入口，组织两个顺序游戏进程、各录制上下文、L1–L7 与一次最终归约。保持第一段 2,000 次、第二段默认 256 次的要求；每次启动分别说明实际动作、键盘接管及停止手动操作时机，取得准备确认后才启动。首次失败保留产物，不自动重启、补测或重跑；清理和无依赖归约可继续。具体范围仍以[E2E 方案](tx-04-08-e2e-plan.md)为准。

建立本清单的早先用户请求仅补齐任务文档及入口，当时不实施A–G、不改生产代码或scenario、不构建、不运行游戏/测试/离线CLI。该次文档静态审阅记录如下；本轮ABC实现与纯编译另见上方续作说明，不改写早先事实，也不预记未来整体E2E结果。

文档静态审阅（本次一次，2026-10-27）：已完成。统一审阅本文及 `docs/README.md`、迁移主方案、Lua/harness 实现说明、E2E 方案四处入口；核对 A–G 与当前源码缺口、RNG 既有静态依据/新增窗口边界、相对引用、同一 tx-04-08 与 2,000/256 次安排、原生/线程/生命周期约束和历史事实保留。未将占位状态当作已完成，未新增 gate、哈希或独立角色要求；未修改生产代码/scenario，未构建、运行游戏、测试或离线 CLI。此记录仅对应本次任务文档，不代表工程项或功能 E2E 通过。
