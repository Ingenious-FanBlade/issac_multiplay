# 项目规范、约定与要求

生效日期：2026-10-12。状态：现行规范，按用户要求修订。2026-10-24 tx-04-02 按用户决定修订第 4、7 节，新增第 14 节（REPENTOGON 适配）。2026-10-30 按用户要求修订第 2、5、8、15 节：tx 不再保留 artifact，E2E 默认不留 artifact，并连续修复、续测直至完成。

适用范围：`isaac_hook_reserach` 的项目推进、C/S 研究、逆向、实现、文档与测试。后续工作先读本文，再读具体任务资料。本文覆盖旧计划、主题索引、技能和历史记录中与本文冲突的流程要求；用户在会话中的明确指示优先。

## 1. 永不设定 gate，不校验哈希

- 永不新增、设定、维护或要求项目 gate、阶段门、证据门、准入门、预封门、promotion 门或解锁门，也不得换名恢复同一流程。
- 不以旧 `gate decision`、`formalDecision`、`promotionCount`、`*Unlocked`、矩阵完成率或 `BLOCKED_BY_EVIDENCE` 字段作为后续推进的前置条件。按当前任务目标、实际依赖与已有授权推进，直接说明完成情况和未决问题。
- 不校验任何哈希，包括目标 PE、构建、文件、artifact、source、manifest、候选、状态或 baseline 的哈希；不执行 SHA/hash 校验脚本，不要求 hash-bound 交接、哈希一致性、哈希链或重签。
- 旧哈希、gate 字段及历史判定可以保留在原始记录中，作为历史信息阅读，不再执行其校验或解锁流程。
- RVA、原始指令、特征字节、对象布局、ABI、线程、调用关系及实际状态仍用于理解和实现。它们不构成新的项目 gate。运行时输入合法性、边界保护和失败处理仍是功能行为。

## 2. 每次推进统一整体验收，E2E 连续修复与续测

- 推进的最小粒度为 `tx-0x-0x`，例如 `tx-00-01`。开始时说明该次推进的完整目标、范围及预期结果；历史 T0/工作包编号可继续作为内容索引。
- 每一次推进只在整个目标完成后进入统一的整体测试验收。验收使用统一入口覆盖整个目标；“一次整体验收”指同一 tx 的完整验收过程，不限制其中因问题修复所需的继续执行或重跑。
- 进入整体验收前的探讨、分析、设计、逆向、实现和修改过程中不得进行其他测试。不得提前运行 smoke、probe、canary、harness、自测、mutation/负向批次、回归、verify-existing 或试跑验收。进入验收后的修复与必要续测按本节执行。
- 文档阅读、文件检索、静态反编译和代码阅读属于资料分析；为判定正确性或通过与否运行程序、实验、校验脚本，均计为测试，不得通过改名绕过本约定。带测试的构建脚本也计为测试。
- 不沿用旧“四组各一次”“每个子项分别验收”“先预封再验收”“多次 replay/comparison”流程，不把同一目标拆成多个编号来增加测试次数。
- E2E 如无特殊说明，应连续完成测试；遇到问题先定位并修复，再在同一 tx 的整体验收中继续必要测试，直至整个目标验收完成。不因失败默认结束工作、等待用户再次发出修复或测试指令，也不为修复续测另换 tx 编号。续测范围须覆盖修复影响及尚未完成的用例，不能只跳过失败项后宣称通过。
- 用户要求停止、需要尚未提供的必要配合或授权、或存在无法自行解决的阻塞时，可以中途停下，说明实际问题和继续条件。实机操作仍遵守第 8 节，不从连续测试要求推导额外授权。
- 每个 tx 任务本身不再保留 artifact，不再为推进单独归档日志、源码快照、报告包或执行材料。E2E 默认也不保留 artifact；仅在中途停下且需要记录信息以便后续继续时，保留必要的诊断、进度及恢复材料，并说明保存位置与用途。正常测试所需临时材料用后清理，不要求最终 artifact。
- 代码、设计、字段目录、schema 和功能本身要求的观测／录制能力仍按任务交付；不因默认不留 artifact 而删去产品功能或省略结果说明。最终说明实际覆盖、失败与修复、完成情况和未决问题。
- 文档任务的一次验收采用静态文档审阅，检查规则一致性、引用与修订范围；不运行游戏、构建或代码测试。

## 3. 取消逆向与测试必须独立的要求

- 逆向、实现和测试可以由同一智能体或主智能体完成，不要求不同子智能体、作者分离、独立 TEST/REVERSE、候选前预封或独立最终审计。
- 不强制创建子智能体。用户要求分工时再按明确任务分配；结果是否可信取决于实际观察、推理和唯一一次整体验收，而非作者身份。
- 操作同一游戏或调试器会话时仍须避免并发控制冲突；交接时说明进程、附着、断点和补丁状态。

## 4. 架构与产品隔离

- 首版采用真实 `isaac-ng.exe` 作为唯一权威模拟器的 Listen Server；房主与远端输入进入同一 scheduler，Authority 日常 tick 不等待全员输入。
- C/S 与原版 Online Lockstep、Isaac Sync 1.0.4 使用独立 session、协议、IPC、日志和可变状态。
- 2026-10-24 tx-04-02 起，不再延续 `ISCS` protocol 5、ABI 5、七项 `Isaac.Sync` Lua API、原版 channel `0` 和 Isaac Sync channel `0xFD` 的兼容约束。C/S 使用独立协议，不进入、不触及原版 Online 与 Isaac Sync。原约束的历史记录保留。
- C/S 不依赖或复用 Online Manager、`NetInputDevice`、`FrameStore` 或原有 all-slot ready 条件。旧同步机制名称中的 gate 只描述旧功能，不是项目推进规范。
- 当前 T0 范围是研究原型；可玩 Replica World、snapshot/baseline/delta apply、预测、rollback、late join 和 Dedicated Server 按后续实际任务处理，不从现有原型外推为已完成。

## 5. 事实、假设与记录

- 区分直接观察、静态推导、运行时观察、设计决定、用户接受的假设和 `UNKNOWN`；标签保留其真实含义，不因取消流程门而伪造功能已完成。
- 单次成功样本不外推为跨版本契约；静态伪代码不充当运行时观察。
- 结论附可读的文件/函数位置、RVA/字节、场景、实际行为与未决问题，不要求哈希或认证产物。
- 已有历史失败、反例和原始记录保持原事实，不因新结果改写。后续不再据此要求每个 tx 或 E2E 留存 artifact，留存范围按第 2 节执行；新结果说明对应的推进编号、失败与修复，不能以最终通过抹去实际失败。
- 历史报告中的“当前”“下一步”“必须独立”“正式入口”“未解锁”只代表原记录当时的语境，不恢复已取消的流程。

## 6. 输入、身份与生命周期

2026-10-30用户要求恢复**最多四名参与者**，在状态同步前以tx-04-09完成四人及一人多EntityPlayer支持，再以tx-04-10—tx-04-14补全各域观测。[连续计划](../design/observation-expansion/README.md)制定时仅编写文档；用户随后启动09，现已通过四参与者／八身体及指定形态的功能整体验收，结果和限制见[实现说明](../design/observation-expansion/four-participant-implementation.md)。下列T0-05固定A/B是原验收的历史命名，统一输入与生命周期规则适用于全体参与者；不得将参与者数等同Entity_Player数。

- T0-05 固定 A 为 ordinal 0、B 为 ordinal 1；`ClientId -> ordinal` 在 session 内冻结。
- `Player*`、device pointer、assignment key 和平台 handle 仅用于当前 generation 的临时关联，不进入 canonical 输入或网络身份。
- A/B 共用 canonical、sequence、target tick、queue、dedupe、resolve 和 apply；房主没有即时旁路，B 不直接写 Player/device/cache/world。
- 重复输入幂等，边沿动作 exactly-once；非法、过期或冲突输入不得改变世界。缺输入时按连续量时限和中性策略处理，不重复旧边沿。
- room、death/respawn、设备重分配使临时绑定失效；唯一重绑前不 apply。menu/run teardown 退休 session 与 generation，旧输入和旧对象关联不复活。
- pause、loading、menu 或未知绑定下保持中性，不把渲染帧、顶层 loop 或墙钟直接当作 Authority tick。

## 7. 线程、Hook 与失败处理

- 游戏对象和 Lua 的操作发生在已理解的主线程边界；网络线程只做有界接收、解析和入队。
- Hook 使用单一协调 owner：所有 Hook 只经统一的 Hook 清单注册。Hook 在启动时安装后常驻到进程结束，会话只做逻辑启停，未激活时原样透传，不做字节恢复（2026-10-24 tx-04-02）。观测 wrapper 保持 original exactly-once。
- 未知 ABI、对象关系、错误线程、事件丢失、绑定冲突或恢复不确定时停止相关修改，记录实际原因；不得猜测后继续写世界、强制卸载或盲目重注入。
- 正常停止时：关闭输入/publication，处理在途 callback，退休对象，然后回到透传；不恢复目标字节。自有 virtual device 的内存和模块按第 9 节的常驻策略处理。
- 2026-10-30用户明确要求：生产默认不启用世界录制、逐tick/query明细、查询一致性诊断和逐consumer重复全图读取；E2E显式启用。必要的生命周期／线程／设备归属检查保留，稳定窗口复用已核定拓扑，变化时立即撤销或重读。后续正式同步所需的纯值状态快照与调试录制分离，不能让输入／同步依赖录制或日志成功；Rust不得跨线程／跨进程直接持有原生Player对象。测试夹具和Rust harness不作为正式游戏模组发布内容。
- 不造成未经任务允许的 save、achievement、unlock、统计或其他持久副作用。最终 dirty=0 或文件未变不能代替对整个操作窗口的理解。

## 8. 操作授权与测试安排

- 2026-10-24 用户要求：每次实机测试开始前，先明确告知用户需要配合的操作和停止手动操作的时机；涉及多次游戏启动时分别提示；需要手动配合时先确认用户已经准备好，再启动。tx-04-04 当前需要用户跳过每次启动的开场动画，到标题页或选档页后停止操作，由 harness 接手。

- 阅读、静态分析与任务范围内的可逆编辑按用户请求推进。已有会话授权继续有效，不因旧 gate/哈希/独立角色要求重复询问。
- 游戏输入、注入、运行时修改与其他实机操作按用户实际授权范围执行；不从历史 PASS、工具可用或设计假设推导新的操作授权。
- 不沿用旧文档强制的逐场景授权编排；需要补充授权时只针对尚未授权的具体操作说明原因。
- 实机验收如依赖现有工具，提前阅读其使用方式；已存在的工具限制如实说明，不把它们升级为新的项目 gate，也不在本次文档修订中修改工具实现。
- 所有测试安排服从第 2 节：进入整体验收前零测试，E2E 默认连续完成，遇到问题修复后在同一 tx 内续测；仅在必要中断时保留所需 artifact。

## 9. 已采纳的 Virtual Device 设计

- 采用自有对象 vtable 加自有 control block、S2 action 层覆写；路线 A（虚拟设备触发原生 Join）为主，直接重绑路线 B 为备选。
- tx-01-45（2026-10-16）用户采纳：Authority-pre 发布 current，StandardRoom original 正常返回后提交 previous；poll 不写快照。正常退休停在 UNREGISTERED 并保留 root strong，不主动进入释放 root 的旧 ZOMBIE 路径；缺输入立即中性（max_input_age_ticks=0）。
- tx-01-50 输入修复补充：原生奇数帧 955241→6FD3F0 也是实际 Player 模拟；在原生暂停/状态检查后首次正常 Main Player 更新前执行 Authority-pre，helper 正常返回后提交 previous。特殊暂停/FSM 更新不推进 scheduler；不把顶层 loop 直接当 tick。该修复与 StandardRoom 共用原有 scheduler，不改变 poll 的只采样职责。
- tx-01-51 用户采纳启动例外：A/B 重新唯一绑定后，首个新鲜输入可经相同 codec/Submit/queue 用于 completed+1；同一新鲜样本的连续量以新 sequence 按正常 lead=2 排入 completed+2，edge 清零，消除下一步空档。普通输入仍 lead=2；旧 generation/旧 edge 不恢复，无新鲜输入仍中性。物理 poll 只采样，不写 current/previous；启动使用同一 loop、同一 exact pair 的新鲜源，查询仍只读 Resolve。
- tx-01-52 用户确认射击相反方向后按者优先；源转换按每条轴保存胜出方向，释放胜出键后回退仍按住的方向，全部释放才归零。无法区分同次采样先后时保留仍按住的旧胜出方向；没有历史时固定右／下。A/B 原始方向源共用转换、各自保存历史，再经相同 canonical/queue/Resolve；不修改移动相消语义或 V1 接口。
- tx-01-45 的交付限定为有效分配、对象/缓冲和主线程正常顺序路径；完整 OOM、EH、worker 并发及计数镜像的并发闭包不在本次范围。可识别的异常中性停用，不能据此声称这些异常已安全恢复。
- tx-01-54 跨房射击修复：无 Authority tick 或 pause/loading/lifecycle 阻断时，C/S 主动分派暂停原生 Shooting consumer，避免把中性查询当作武器松键；停用时同样暂停。正常无阻断 tick 的真实松键与缺输入仍执行中性消费，不保持旧攻击。A/B 共用，不改武器蓄力字段；观测 wrapper 和其他原生更新保持原调用，主动暂停的 Shooting original 调用次数为零。本次一次专项18/18 PASS，覆盖普通门往返及暂停恢复，不外推所有武器或 loading/换层。
- 2026-10-26 用户在 tx-04-07 采纳重绑恢复例外：唯一绑定后，从首个有效 Authority tick 起最多六个 tick，尚无可消费的新代次命令时可暂停 Shooting original；Authority 与其他更新正常推进，canonical 查询仍中性，不等待全员、不复用旧 held、不读写蓄力字段。首条新代次命令用于 current（包括真实中性/松键）立即结束恢复窗口，本代次不再重新开启；六 tick 到期也恢复中性消费。consumer 缺席不延长窗口，A/B 同规则。此例外只覆盖首条新命令前的握手空档，其余正常缺输入与真实松键仍按前述规则消费；原 tx-01-54 验收记录不追溯改变。
- tx-01-56 异常 Join 分支实现：自有 B 为完整 Type=1/Variant=0 Player；在同一 Game/manager/A、主线程和唯一 exact pair 的两 pass Join 内，三处原生 Baby 判断仅对自有 K 选择普通角色路径。原 predicate 各调用一次，其他设备保留原值，不改全局 run flags；普通工厂仍为唯一创建 owner，不接纳未经 owner 的 Baby/possessor。新增三处 Hook 纳入统一51槽事务；本轮唯一E2E专项21/21 PASS，实际Baby=true路径、完整B、宿主健康与51槽恢复已覆盖，旧tx-01-51具体来源仍为静态推断。
- 回收策略为 `NEVER`：进入 `BASE_CONSTRUCTED` 后，对象、control block 及其有效基类状态在进程生命周期内不释放。只有未执行原生构造的 raw allocation 可以回滚释放。
- 首次调用原生 base ctor 前固定 DLL；恢复后 DLL 常驻。退休输出中性，设备从 InputManager vector 移除，B 解绑或退出；每个 session 至多一次创建，失败不自动重试。
- Base 38 槽表和 control block 6 槽表按已理解的 ABI 实现；control 槽不析构或释放，保留原生 Mutex/CS 状态并维护影子计数及镜像。
- 旧 native alias universe 批次为背景资料，Batch 2 retry 暂停。全映像别名闭包、factory/release-thread 旧流程不再作为项目推进门。
- key 并发、Init 分配失败、remove 与 worker 并发、注册/assignment 窗口、COL/RTTI 等未决问题直接列入实现说明与该次推进的整体验收，不另设 gate 或提前测试。
- [协调器契约 v2](cs-t0/t0-05/07-virtual-device/design-revision/virtual-device-owned-abi-coordinator-contract-v2.md)中的具体功能约定继续适用；流程冲突以本文为准。

## 10. 文档维护入口

- 本文是统一的现行项目规范；阶段计划与设计文档提供目标、接口和事实资料。
- `docs/refactor` 的旧规范与历史记录、`cs-t0/README.md` 和 `isaac-cs-refactor` 技能中的冲突流程，自 2026-10-12 起不再生效。
- 更新项目要求时先修改本文，并同步主要入口与技能；不为规范修订创建 gate、哈希审计、角色预封或多轮测试。
- 文档整理沿用主题目录；保留仍被代码或历史引用的路径，不按版本号盲目删除文件。


## 11. 已采纳的 T0-06 当前 Player 字段读取规则

- tx-01-73（2026-10-18）用户明确采纳：已有 exact-pair 绑定的 A/B 在有效 Authority 模拟窗口内完成一次原生 PlayerUpdate，正常返回后 generation 与绑定仍一致时，可以将其当前专用字段标为可读。
- 记录为 `USER_ACCEPTED_CURRENT_PLAYER_UPDATE_WITNESS`（读取依据值 3）；它不是已观察的构造或 Init，也不补造 FIRST_OBSERVED 的出生、复用或成员历史。绑定失效时撤销该依据，重新满足相同条件后才能恢复。
- 完整世界样本仍只在同 token 的普通 owner、Room 和标准尾部正常完成后、coop TopExit 前复制。Player-only 更新不因此获得完整世界样本资格；未知身份、引用和非有限值仍不能宣称相等。
- 2026-10-27，用户在 tx-04-08 A 项采纳 `USER_ACCEPTED_CURRENT_ENTITY_BYTE`：Entity 公共字节 `removed_by_factory +0x177` 的当前原值，只对已观察创建 owner 正常完成的受支持 Entity，或已满足本节 PlayerUpdate 规则的 exact-pair A/B，在无生命周期污染且来源/root 前后稳定的完整采集窗口可读。Init、Restore、owner 或绑定失效撤销该资格；Player 重新正常更新后可重建。初始化历史、当前资格与出生分开，不补零、不补出生。随后补证的基 Init 跨字段清零不扩大此授权，也不覆盖未观察历史。

## 12. 已采纳的 T0-06 E2E 连续计数规则

- tx-01-74（2026-10-18）用户明确采纳：W07 的 10,000 次指同一稳定 run/room 内连续的普通完整采集。明确的 odd-only gap 必须保留原记录，不计入次数，也不打断该区间。
- 只有原因与 Authority kind 均明确为 odd-only、且同 run/room、mutation、线程及递增 token/tick/连续 attempt 的记录适用该例外；odd 仍为 gap，不获得完整世界样本或普通跨进程对齐资格。
- 其它缺口、读失败、字段/来源不完整、背压导致的缺号或生命周期变化均打断区间。完整扫描和交互进度使用同一规则；报告保留完整样本数、跳过的 odd-only 数量和区间边界。
- 2026-10-26，用户为 tx-04-08 的 REPENTOGON world_record 迁移明确将稳定区间目标改为 **2,000 次**完整普通采集，沿用上述 odd-only 与中断规则；该专项决定不改写历史 W07 的 10,000 次要求或结果。范围与两次顺序启动的整体 E2E 见[迁移设计](../design/tx-04-08/tx-04-08-implementation-plan.md)。
- 2026-10-29，用户将 tx-04-08 采集等待上限从 240 秒缩为 30 秒，并要求样本目标相应调整。当前按原规模的 1/8 设置主进程 **250 次**、对照进程 **32 次**完整普通采集；普通完整样本与总 Authority tick 分开计数，沿用 odd-only 与其他中断规则，达标可提前结束。实时停止与最终全量归约均读取本次 scenario 的目标，不再在归约器写死 2,000。旧 2,000/256 次及 e2e-08 超时失败仍按原记录保留，不追溯改判。

## 13. 已移除的原版联机实机测试

- 2026-10-19 用户明确要求：C/S 设计永不进入或触及原版 Online/Isaac Sync，永久移除 T0-06 当前清单的 `W01.isolation` 实机项，不再列为未覆盖或待测，也不将移除视为测试通过。
- W01 的默认/可选开关、ABI/原生调用与恢复检查继续保留；架构隔离和不复用原版在线状态的约束不变。历史结果按当时清单保留，不改写旧 33 项账本。

## 14. 已采纳的 REPENTOGON 适配决定

- 2026-10-23 至 10-24，用户在 tx-04-01、tx-04-02 中明确决定以下事项。设计见 [REPENTOGON 适配重构设计](../design/repentogon-refactor.md)。
- 目标环境使用官方 REPENTOGON。游戏降级到其目标版本 `Repentance+ v1.9.7.12.J273`。原型基于 `v1.9.7.17.J460` 的 RVA、字节、对象偏移和 vtable 槽位只作索引，使用前必须在新版本上重新核对；重新核对不涉及哈希。
- 产物是由 REPENTOGON 加载器加载的 `zhl*.dll`，不再使用 Helper 注入。网络在 DLL 内，Rust 只作 E2E 编排、测试客户端与中继工具。
- Hook 常驻、逻辑启停，见第 7 节。DLL 由加载器加载后常驻；自有 virtual device 继续按第 9 节的 NEVER 策略管理。第 9 节中“纳入统一 51 槽事务”等表述是原型实现的历史记录，REPENTOGON 路线以统一 Hook 清单代替。
- 兼容约束不再延续，见第 4 节。与原版 Online、Isaac Sync 的隔离原则继续有效。
- 首版不支持第三方 gameplay Lua mod。本项目自带的纯界面 Lua mod 不算 gameplay mod，但不得注册 gameplay 回调。E2E 可以使用测试专用 Lua mod，它只作测试夹具，不构成支持声明。
- 房主打开 REPENTOGON 控制台、菜单或 ImGui 时，REPENTOGON 在 `InputManager` 层屏蔽动作，A/B 同时受影响。首版按类似暂停处理：屏蔽期间的输入不生效，事后不补发、不重放边沿。

## 15. 状态同步前的完整观测计划与公开接口优先

- 2026-10-30，用户确认连续顺序为tx-04-09四人／多身体、10 Entity、11 GridEntity、12房间／楼层、13探索／离房保存／跨房玩家、14整局／随机选择／HUD／Mod状态。见[总览与详细计划](../design/observation-expansion/README.md)。暂不进入世界状态下行、Replica或baseline/delta apply实现；四人输入改造不算世界同步。
- 按[观测域拆分详细设计](../design/observation-expansion/domain-observation-architecture.md)组织后续实现：统一协调器和公共上下文／身份／引用，各域独立模型、采集、事件与编码；native／只读Lua来源和录制／正式消费端分别管理。公共边界及现有Entity／旧Grid迁移纳入tx-04-10，其他域按11—14接入，不增加提前测试。不同阶段记录独立scope，采集不依赖调试录制；当前仅确定设计，不修改运行代码。
- 优先查本地原版Wiki与REPENTOGON Wiki，使用公开Lua字段、Getter、容器与回调。语义明确的公开接口不要求全部逆向、native字段对照或双实现比对；有具体版本／副作用／生命周期疑问才查源码，对未暴露的具体需求再补必要逆向。既有直接native偏移的目标版本核定约束保持，不强加到全部公开Lua成员。
- 完整观测按全部相关公开成员、关联数据和明确未暴露需求建立目录，覆盖状态、关系、变化与生命周期，不以旧min-read-v1或未来Replica最小字段集合为上限。目录整理、采集实现、实机观察和缺口分别报告；未知或未观察不改成完成。
- 自有只读观测Lua模块可作为正式采集来源，与纯界面Lua和E2E玩法夹具分开。不得为了采集额外执行随机、抽奖、创建状态表、推进动画或写持久数据；游戏对象／Lua仍在已理解主线程阶段操作，后台仅处理自有不可变数据。
- Mod自定义状态的观测适配不等于任意第三方gameplay Mod兼容；未知闭包／私有对象不能声称自动完整捕获。已声明受支持Mod的缺失项如实列为未完成，按具体适配补齐。
- 每个实现 tx 全目标完成后进入统一入口整体 E2E，默认连续修复与续测，artifact 留存按第 2 节执行。当前仅文档编写，最终一次整体静态文档审阅，不构建或运行游戏。已有用户会话操作授权优先，不从新计划文字推导额外授权。

## 16. 整局可玩联机模组的完整推进目标

- 2026-10-30，用户要求状态同步推进流程覆盖完整任务，全部推进完成后交付整局可玩的联机模组。执行入口为[tx-04-09—tx-04-34完整流程](../design/state-synchronization-roadmap.md)，技术模型见[状态同步设计](../design/state-synchronization.md)。当前只编写文档，不代表任何新功能已实现或获得额外实机／持久写入授权。
- 09—14的输入与完整观测范围不变；15—34覆盖真实游戏Client、资源与玩法隔离、全域应用、Baseline／Delta／恢复、切房换层与原版完整内容、HUD音画、角色生命周期／结局、中途加入重连、合法Continue、平滑性能、完整UI与发布工程、整局联合验收。
- 单房间和少量变体可作为中间交付，不能作为最终完整产品边界。原版必要类别、路线和流程的未实现项须在对应tx补齐，不靠缩小profile、无声忽略或“后续扩展”提前宣告完成。
- 最终以实际发布包、正常用户流程和跨设备真实游戏Client完成四人及多身体的整局自然游玩与必要模式／路线覆盖。输入工具、顺序离线采集、包数或单次静止房间比较不能代替此目标；正常结束、保存恢复、网络恢复和各端进度归属均纳入。
- 保持Listen Server唯一Authority和原版Online／Isaac Sync隔离；本次完整产品不隐含Dedicated Server、host migration、gameplay预测／rollback或任意未适配第三方Mod。测试执行与artifact规则按第2节，不增加gate、哈希或独立角色要求。
