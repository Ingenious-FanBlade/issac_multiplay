# Isaac C/S T0 evidence, mode-isolation and boundary tools

tx-03-08（2026-10-21）：T0-09-C 工作设计 **DESIGN_COMPLETE / STATIC_REVIEW_PASS**；生产实现 **NOT_STARTED**、P2 **NOT_RUN**。C 包含 VM 本地 A/B observer、共享脚本、普通完整采样/odd gap、首次分歧与 F1–F5；用户选择一次入口运行 6 场共35,000 Authority tick，逐场自行重启双端准备，完整多 seed 矩阵留给 D。见 [C 工作设计](../docs/refactor/cs-t0/t0-09/tx-03-08-c-design.md)与[实施前失败方式](../docs/refactor/cs-t0/t0-09/tx-03-08-c-failure-modes.md)。本轮仅文档，未构建、未操作游戏或运行代码测试。

tx-03-07（2026-10-21）：Host+VM 双真实游戏 P1/F0 唯一一次实机验收完成，Orchestrator PASS、Analyzer **38/38 PASS**：10,000 Authority ACK/STATE、双端正常停止/退休、游戏存活、Host 51/51 catalog 恢复。原始统一入口因运行中游戏映射 staging DLL、清理删除返回 `WinError 5` 而机械记 FAIL；用户裁定“算正式通过，构建产物我自行删除”。**T0-09-B P1/F0 = FORMAL PASS（用户裁定）**，原始 FAIL 保留不重跑，T0-09 整体仍 IN_PROGRESS。见[正式验收记录](../docs/refactor/cs-t0/t0-09/tx-03-07-live-f0-formal-acceptance.md)。

tx-03-06（2026-10-21）：Authority 发布终点锁定已修复，终点 ACK/STATE 成功后停止协议发布与远端 admission，游戏模拟继续，正常 STOP/恢复保持原路径。唯一生产链路/Windows mapping/Remote Endpoint 集成验收 **73/73 PASS**，10,000 tick后额外512次调用仍固定终点；用户当前无法准备双机，F0实机尚待复验，见[修复与结果](../docs/refactor/cs-t0/t0-09/tx-03-06-publication-boundary.md)。

最新 tx-03-05（2026-10-21）：Hook 校正后唯一双机 F0 **33/38 FAIL**。真实初始化、双端 Wire/输入链路与 Host **51/51 槽恢复**通过，双游戏存活、正常退休、事件/Wire lost0；目标14795后仍发布到14819，多24 tick，五项终点检查失败。未重跑，完整 P1/F0 与 T0-09 仍 IN_PROGRESS；见[实机结果与原始证据](../docs/refactor/cs-t0/t0-09/tx-03-05-live-f0.md)。

最新 tx-03-04（2026-10-21）：Native 全家族固定 Hook 点位静态核对完成，未发现 RVA 漂移；修正四旧模式的脱壳镜像准入、input/raw 精确原函数事务检查及失败诊断。唯一整体文件/源码验收 **165/165 PASS**，Native/WorldE2E Native/Helper/T09 控制器纯编译通过。当前未找到游戏，内存点位及实机安装/恢复未运行；动态 Steam 槽等语义未知单列。见[结果与复现](../docs/refactor/cs-t0/tx-03-04-hook-calibration-results.md)及[逆向记录](../docs/refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)。

最新tx-02-04（2026-10-20）：T0-08操作1–9当前范围 **PASS**。验收器自身持有A物理输入后完成唯一整体E2E，**33/33 PASS**、controller exit0、未自动重跑。初始4步与连续64步全程游戏前台；A有646/6269个非零PhysicalSample和真实位置变化，连续输入覆盖tick160..179 outage。trace56,070/56,070、lost0，6,268 commit逐条ACK/STATE/身份配对，B中性/恢复、3/3 Drop边沿、70槽恢复/正常stop通过；离线19,022/19,022 PASS。见[tx-02-04结果](../docs/refactor/cs-t0/t0-08/tx-02-04-results.md)、[owned输入实现](../docs/refactor/cs-t0/t0-08/tx-02-04-owned-physical-input.md)及[T0-08状态](../docs/refactor/cs-t0/t0-08/README.md)。实机拓扑仍是一个真实游戏Authority加同进程逻辑Remote，不宣称socket/Steam、双游戏、Replica或跨进程世界相等。

历史tx-02-01（2026-10-20）：协议共享槽先复制/释放再缓冲落盘，错误路径统一正常stop并保存恢复/原始错误，报告器处理缺文件。唯一整体E2E **23/30 FAIL**：离线19022/19022 PASS，trace4007条/lost0、70槽逐项恢复通过；271个真实commit的最后tick271出现Publication7，ACK/STATE各缺1；A原生272采样/271 Resolve移动量全0，动作持续项未通过。原问题修复的实机证据、未解决项及旧失败均保留，当时T0-08为IN_PROGRESS，见[历史结果与产物](../docs/refactor/cs-t0/t0-08/tx-02-01-results.md)。未自动重跑，下文为历史记录。

历史tx-02-00实机结果（2026-10-20）：用户确认继续并手动聚焦游戏后，真实Authority/B创建/物理输入已运行；controller因`protocol trace overflow`退出1，事件保存至commit20、协议312条。离线19,022/19,022 PASS；正常停止helper成功，但70槽逐项恢复记录未落盘，完整实机验收未完成，当时T0-08为IN_PROGRESS。前两次无进程/输入等待超时失败保留，见[全部执行记录](../docs/refactor/cs-t0/t0-08/tx-02-00-results.md)。未自动重跑。

历史tx-02-00（2026-10-20）：T0-08操作5–9及真实Authority接入已实现，ICT8/wire v2、INPUT最多4条冗余、独立ACK/STATE lane、严格decoder/独立corpus、确定性有界内存FaultTransport、显式IPC v6及原始包trace。无哈希、保留包CRC；用户选择A/B首个新鲜输入同completed+1，B迟到正常过期，后续lead8。**唯一整体E2E已执行：离线19,022/19,022 PASS；未找到运行中的isaac-ng.exe，整体FAIL、实机NOT_RUN，未自动重跑；当时阶段整体IN_PROGRESS。**见[历史结果](../docs/refactor/cs-t0/t0-08/tx-02-00-results.md)、[实现和执行入口](../docs/refactor/cs-t0/t0-08/tx-02-00-implementation.md)及[实机清单](../docs/refactor/cs-t0/t0-08/tx-02-00-e2e-checklist.md)。设计拓扑为一个游戏Authority加自有逻辑Remote，不宣称socket/Steam、两个游戏或Replica。

历史tx-01-99：操作1–4的独立wire v1核心与内存双端握手验收 **305/305 PASS**、退出码0，保存30条原始包/双端状态；当时没有游戏/Native接入或操作5–9。原[结果与复现](../docs/refactor/cs-t0/t0-08/tx-01-99-results.md)和[协议定义](../docs/refactor/cs-t0/t0-08/tx-01-99-wire.md)保留。

**T0-07 状态：PASS（2026-10-20，用户确认）。** 操作1–8的现有生产状态链/故障矩阵验收记录与 tx-01-98 的真实 Authority B源停发 E2E 共同作为通过依据；边界仍按各记录保留：B是脚本源、不是网络断线/ACK，world record仅partial Player字段证据，历史离线PASS不外推为未运行的实测项。见[阶段状态与证据表](../docs/refactor/cs-t0/t0-07/STATUS.md)。

tx-01-98（2026-10-20）：T0-07操作9已完成一次真实 Authority 的 B 输入停发实验。最终有效 invocation 使用批准 unpacked 目标、显式 v5 2/64/0/64/258、一次 New Run 和控制器前已开始的 bounded A 物理动作；生产脚本源停发1/5/30/120 Authority tick，effective缺口按lead=2对齐，B中性/恢复/edge 5/5/120-tick timeout、A实际位置变化和70槽恢复通过，统一报告 **24/24 PASS**、controller exit0。前三次失败（错误GameExe、A动作晚到、A穿门死亡）原样保留；world record仅partial `NOT_READY` Player字段证据，无complete world snapshot；脚本停发不冒充真实网络断线。见[结果](../docs/refactor/cs-t0/t0-07/tx-01-98-results.md)、[设计](../docs/refactor/cs-t0/t0-07/tx-01-98-source-stop-e2e.md)和[审阅](../docs/refactor/cs-t0/t0-07/tx-01-98-static-review.md)。

tx-01-97（2026-10-20）：T0-07操作8已完成生产故障与边界矩阵：A/B conflict、source sequence极值、queue/history、future/expired/late/edge、timeout、tick exhaustion、identity/encoding/generation、phase/return/publication和collector gap；最终 **27场景、183命令、184/184检查PASS**。历史失败保留；真实游戏、Hook、采样内存、事件环、网络和世界动作未运行；操作9随后由tx-01-98完成。见[结果与复现](../docs/refactor/cs-t0/t0-07/tx-01-97-results.md)、[覆盖矩阵](../docs/refactor/cs-t0/t0-07/tx-01-97-fault-matrix.md)和[审阅](../docs/refactor/cs-t0/t0-07/tx-01-97-static-review.md)。

tx-01-96（2026-10-20）：T0-07操作7已完成CLI/manifest到Native的输入配置贯通：严格manifest/CLI校验、IPC v5、初始化冻结、Participant/Clock/Pipeline及target/script实际生效，EventKind36记录生效配置；最终 **20配置case、9场景、83命令、104/104检查PASS**。历史编译/场景失败保留；真实游戏、Hook、采样内存、事件环、网络和世界动作未运行，操作8–9仍待实施。见[结果与复现](../docs/refactor/cs-t0/t0-07/tx-01-96-results.md)、[实现](../docs/refactor/cs-t0/t0-07/tx-01-96-runtime-config.md)和[审阅](../docs/refactor/cs-t0/t0-07/tx-01-96-static-review.md)。

tx-01-95（2026-10-20）：T0-07操作6已完成生产实现与一次整体离线验收：missing/effective输入分离、max-age=0中性、独立有效到达、120 Authority tick诊断性静默timeout、恢复、代际/终态与EventKind35/query8-9接入；最终 **10场景、146命令、122条批量tick、152/152检查PASS**。首次FAIL/编译失败原样保留；真实游戏、Hook、采样内存、事件环、网络及世界动作未运行，操作7–9仍待实施。见[结果与复现](../docs/refactor/cs-t0/t0-07/tx-01-95-results.md)、[实现](../docs/refactor/cs-t0/t0-07/tx-01-95-missing-input-timeout-recovery.md)和[审阅](../docs/refactor/cs-t0/t0-07/tx-01-95-static-review.md)。

tx-01-94（2026-10-20，文档更新）：已落实T0-07[操作6–9具体工作计划](../docs/refactor/cs-t0/t0-07/tx-01-94-remaining-operations-plan.md)，列明缺输入/超时/恢复、CLI到Native参数贯通、完整故障覆盖、真实Authority的B输入停发实验，以及Luna/max分析与主智能体审阅安排。本次不改代码、不构建、不运行测试/游戏；操作6–9仍待实施，最新实现结果为tx-01-93。

tx-01-93（2026-10-20）：T0-07操作5已完成生产pipeline和Native接入，固定drain/准入、双Resolve、原生正常返回、post-state复制或缺口、tick提交及本地回执发布顺序；GPT-6 Luna / max结论经主智能体逐项审阅。Native最终纯编译成功，唯一统一离线验收 **36场景、254/254检查PASS**，保存120条连续生命周期记录。真实游戏/Hook/采样内存/网络未运行，操作6–9未实施；见[结果与复现](../docs/refactor/cs-t0/t0-07/tx-01-93-results.md)和[审阅记录](../docs/refactor/cs-t0/t0-07/tx-01-93-static-review.md)。下文较早范围说明保留历史时点。

tx-01-93（2026-10-20）：T0-07操作5已完成生产pipeline和Native接入，固定drain/准入、双Resolve、原生正常返回、post-state复制或缺口、tick提交及本地回执发布顺序；GPT-6 Luna / max结论经主智能体逐项审阅。Native最终纯编译成功，唯一统一离线验收 **36场景、254/254检查PASS**，保存120条连续生命周期记录。真实游戏/Hook/采样内存/网络未运行，操作6–9未实施；见[结果与复现](../docs/refactor/cs-t0/t0-07/tx-01-93-results.md)和[审阅记录](../docs/refactor/cs-t0/t0-07/tx-01-93-static-review.md)。下文较早范围说明保留历史时点。

tx-01-92（2026-10-20）：T0-07操作4已完成：A/B共用生产AcceptInput，typed本地命令转入同一字节解码/validator/去重/队列，底层Submit收为私有。GPT-6 Luna / max结论已由主智能体逐项审阅；Native最终纯编译成功，唯一统一离线验收 **40场景、303/303检查PASS**，保存4846条原始准入记录。B当前仍是脚本源，真实游戏/Hook/网络未运行，操作5–9未实施；见[结果与复现](../docs/refactor/cs-t0/t0-07/tx-01-92-results.md)和[逐项审阅](../docs/refactor/cs-t0/t0-07/tx-01-92-static-review.md)。下文操作1–3的范围说明保留历史时点。

tx-01-91（2026-10-20）：T0-07操作3已实现：逐client序号、未来窗口、有效连续状态、edge来源账本、received/accepted/resolved/processed进度与分类拒绝计数，Native退休时有界输出。已按用户要求使用GPT-6 Luna / max并由主智能体审阅；纯编译成功，唯一离线验收 **22场景、152/152检查PASS**，保存4357条批量输入记录。真实游戏/Hook未运行，操作4–9未实施；见[实际结果与复现](../docs/refactor/cs-t0/t0-07/tx-01-91-results.md)和[逐项审阅](../docs/refactor/cs-t0/t0-07/tx-01-91-static-review.md)。下文tx-01-89/90范围说明保留其历史时点。

tx-01-90（2026-10-20）：T0-07操作2已实现：生产AuthorityTickClock只在精确配对的标准Room/odd helper正常返回后提交tick，loop/暂停/暖机/缺失返回不冒充模拟；采集完整性与计数独立。GPT-6 Luna / max结论已由主智能体复核，Native/离线程序纯编译成功；唯一离线验收 **30场景、175/175检查PASS**。未运行游戏或实机Hook验证，操作3–9未实施；见[结果与复现](../docs/refactor/cs-t0/t0-07/tx-01-90-results.md)和[逐项审阅](../docs/refactor/cs-t0/t0-07/tx-01-90-static-review.md)。

tx-01-89（2026-10-19）：T0-07 操作 1 AuthoritySession 与 Native 接入已实现；一次性冻结身份，Warmup 允许绑定/Join 且不消费 gameplay 输入，正常退休与 Failed 均不可复活。Native/离线场景程序纯编译成功；唯一离线验收 **33 场景、303/303 检查 PASS**，实机 Hook/Join/恢复未运行。操作 2–9 不在本次范围，见[结果与复现入口](../docs/refactor/cs-t0/t0-07/tx-01-89-results.md)。

tx-01-88 当前结果（2026-10-19）：W01.isolation 按用户要求永久移除；W08 replay 加入有界缓冲与显式 flush，一次统一离线验收 **PASS**。2.3 GB 两次重编码 14.50/13.58 秒，33,050 条记录/20,931,447 字段及输出字节核对通过；当前清单累计 **32/32 PASS**，30 项沿用历史、2 项 W08 本轮检查，无剩余测试项。旧中断/FAIL 均保留；见[结果与复现步骤](../docs/refactor/cs-t0/t0-06/tx-01-88-e2e-results.md)。下文 33 项及排除状态保留对应历史时点。

tx-01-87 当前结果（2026-10-19）：剩余授权 E2E **6/6 PASS**，累计 **31 PASS / 0 FAIL / 2 NOT_COVERED**（25 项沿用历史出处）。真实 Continue 加载与 run 退出已采集，939 个普通样本阶段匹配，70 槽恢复；保存数据的 schema/Spawn/reason 汇总修正完成，没有重发故障或改写旧 FAIL。真实 Online/Isaac Sync 隔离与全量 replay 继续按用户选择排除，全局为 INCOMPLETE，当前授权范围已完成。见[结果与可重复入口](../docs/refactor/cs-t0/t0-06/tx-01-87-e2e-results.md)。下文状态保留对应历史时点。

tx-01-85 专项结果（2026-10-19）：retained Player／Replace 与 GridFree **2/2 PASS**。三次真实 Replace 从 tx-01-84 原始采集按正确参数/输入语义汇总，新进程仅补采自建岩石 D12 → GridFree、旧身份退休与槽退出；70 槽恢复，旧 FAIL 和 tx-01-83 总账保留，完整 T0-06 尚未完成。见[证据与复现入口](../docs/refactor/cs-t0/t0-06/tx-01-85-e2e-results.md)。下文准备状态均为历史时点；tx-01-83 后来已运行，原账本为 25 PASS / 1 FAIL / 7 NOT_COVERED，见[其结果](../docs/refactor/cs-t0/t0-06/tx-01-83-e2e-results.md)。

tx-01-83 准备完成（2026-10-19）：已补齐控制器 loop 日志中的安装失败阶段/Win32 错误与 helper 退出后的末次事件读取；统一执行器在正常结束、异常或中断时一次性汇总已完成证据，并保留最初失败及收尾错误。**控制器纯编译成功，运行验收未执行**；Python 仅静态审阅，Native/Helper/协议不变，未操作游戏，tx-01-82 原始 FAIL 不重判。详见[实现、产物与构建记录](../docs/refactor/cs-t0/t0-06/tx-01-83-diagnostics-and-finalization.md)。后续验收尚未启动，现有编排没有从 tx-01-82 失败点续跑的入口。

tx-01-82 实际结果（2026-10-19）：宿主统一 E2E 已执行一次，**整体 FAIL，报告完成**。精确 Game frame 6000 对齐比较得到 DIFFERENT，field-read/source-changed 两个受控故障均保留 scheduler 继续及 70 槽恢复证据；event-overflow 在触发前因 Hook 安装初始化失败而中止，wrong-thread/root-changed 未进入。用户选择保留 primary 场景未覆盖；最终汇总未到达，冻结机器总表仍为 21 PASS / 12 NOT_COVERED，不能把这些 PASS 称为本轮新增。未自动重跑，详见[实际结果与诊断缺口](../docs/refactor/cs-t0/t0-06/tx-01-82-e2e-results.md)。下文准备/等待状态均保留对应历史时点。

tx-01-81 运行更新（2026-10-19）：**FAIL，未启动 controller/注入**。直接 Win32 诊断只扫描到 3 个进程、0 个游戏候选，推断可能为执行沙箱进程视图限制；十项测试仍未执行。已准备在用户确认后将完整统一入口放到沙箱外运行，禁止独立探测/自动重跑，见[原始事实](../docs/refactor/cs-t0/t0-06/tx-01-81-e2e-results.md)与[执行环境修正](../docs/refactor/cs-t0/t0-06/tx-01-82-host-execution.md)。

tx-01-80 本轮整体结果（2026-10-19）：**FAIL，尚未启动 controller/注入**，首个进程查询未得到单元素 list，旧查询未保留 stdout，根因 UNKNOWN。十项运行测试均未完成、历史 21 PASS 原出处保留。已准备 tx-01-81 的直接 Win32 枚举和完整候选诊断修正，未试跑或自动重跑；见[失败记录](../docs/refactor/cs-t0/t0-06/tx-01-80-e2e-results.md)及[修正方案](../docs/refactor/cs-t0/t0-06/tx-01-81-process-observer-fix.md)。下段“尚未启动”为验收前准备时的历史状态。

tx-01-80 准备（2026-10-19）：已实现并纯编译测试专用 Native 探针、独立 probe mapping/日志、受控观察故障、可移除场景 Mod 与统一执行器 `-InstrumentedRemaining`。用户排除全量 replay 和真实 Online/Isaac Sync 隔离实机项，其余 **10 项待同一次整体验收**。当前尚未启动控制器或注入；没有新增运行 PASS，见[范围、场景与构建记录](../docs/refactor/cs-t0/t0-06/tx-01-80-remaining-test-design.md)。

tx-01-79 最新运行（2026-10-19）：**INCOMPLETE：21 PASS / 0 FAIL / 12 NOT_COVERED**。本轮补齐新跟班创建/Init 与八类字段、独立控制器失联恢复；正常及失联均确认 70 槽原字节恢复。自有 reader 暂停 1 秒时 tick 10245→10305、背压 0→57，W12 仍仅部分覆盖。18 PASS 历史沿用、3 项本轮实际检查；tx-01-77/78 两个注入前观察器 FAIL 分别保留。见[结果及剩余十一项（不含排除 replay）](../docs/refactor/cs-t0/t0-06/tx-01-79-e2e-results.md)。

tx-01-76 剩余项续跑（2026-10-19）：只跳过全量 replay，单条解码/比较与限定双进程比较新增 **9 PASS**；另 **10 PASS** 沿用上轮，统一结果 **INCOMPLETE：19 PASS / 0 FAIL / 14 NOT_COVERED**。Projectile 可读，Familiar 专用字段因缺初始化依据保持 NOT_READY，补充采集 70 槽正常恢复。见[结果与剩余项](../docs/refactor/cs-t0/t0-06/tx-01-76-e2e-results.md)及[续跑参数](../docs/refactor/cs-t0/t0-06/tx-01-76-remaining-e2e.md)。原中断和失败报告保持不变，未自动重跑。

tx-01-75 本轮 E2E（2026-10-18）：用户因全量 replay 耗时选择结束，最终 **INTERRUPTED**，33 子项为 10 PASS / 0 FAIL / 23 NOT_COVERED。主采集连续完整普通样本 **13,567**、背压 **0**，消费管线与 51/70/70 槽恢复已通过对应子项；离线 replay 部分输出和全部原始材料保留，没有自动重跑。见[结果与未完成项](../docs/refactor/cs-t0/t0-06/tx-01-75-e2e-results.md)及[最终报告](../artifacts/cs-t0/tx-01-75-e2e-01/result.json)。下文 tx-01-75“运行未执行”为其实现交付时的历史状态。

tx-01-75（2026-10-18）：world 消费改为通知驱动的专用读取线程，原始值复制结束立即归还共享槽，编码/缓冲文件输出由另一个线程完成。用户确认自有队列上限 256 MiB / 256 个批次或事件块，满时保留既有背压；world 内部 mapping 升为 v2，新增 `world/consumer.json`。本轮只实现与纯编译，运行验收未执行，tx-01-74 的 FAIL 保留。见[行为、收尾与构建说明](../docs/refactor/cs-t0/t0-06/tx-01-75-immediate-consumer.md)。

tx-01-74 本轮 E2E（2026-10-18）：统一入口已执行一次，整体 **FAIL**，32 子项为 17 PASS / 1 FAIL / 14 NOT_COVERED。W07 最长连续完整采集仅 2/10,000，primary 20,178 次尝试中背压 18,868 次；默认 51 槽及主/次采集各 70 槽正常恢复，W08/W09 整组通过。未自动重跑，未在运行中改代码。见[完整结果与未覆盖项](../docs/refactor/cs-t0/t0-06/tx-01-74-e2e-results.md)及[原始最终报告](../artifacts/cs-t0/tx-01-74-e2e-01/result.json)。下面“未运行”是实现交付时的历史状态。

tx-01-74（2026-10-18）：新增 [统一 E2E 入口](accept-world-record-e2e.ps1)，按用户确认的人工检查点组织全新游戏进程、主/次采集、生产 replay/compare、自有副本负向用例、控制器丢失及 W01–W13 汇总。自动事实与署名人工证据分开记录，未覆盖项不会自动 PASS；仅使用已有构建，不调用旧脚本。本轮执行器未运行，全部 E2E 仍 NOT_RUN。见[使用与证据边界](../docs/refactor/cs-t0/t0-06/tx-01-74-unified-executor.md)。

tx-01-73（2026-10-18）：T0-06 运行时与离线工具已实现，运行验收未执行。新增 `coop-active --world-record min-read-v1`、统一 70 槽事务、八类只读采集/临时身份/JSONL，以及 `world-replay`、`world-compare`；默认 coop v3/51 槽保留，可选模式使用 v4 扩展与独立世界 mapping。用户确认的当前 PlayerUpdate 读取依据单独标记，不补造 FIRST_OBSERVED 的出生历史。见[实现说明](../docs/refactor/cs-t0/t0-06/tx-01-73-implementation.md)和[实现后 E2E 清单](../docs/refactor/cs-t0/t0-06/tx-01-73-e2e-checklist.md)。仅纯编译，不运行测试、游戏或哈希校验。

tx-01-72（2026-10-18）：将已退役且不在现有代码/工具/数据依赖中的旧测试检查链和JSON移入[trash](trash/README.md)，共129个文件，含48个JSON、26个PowerShell及7个旧admission工具包的配套文件。[逐文件旧→新索引](trash/tx-01-72/archive-index.json)保留全部内容与移动理由；当前E2E入口、源码、构建和仍被引用的JSON保留原位置。历史段落中的原路径按索引定位，可点击链接已迁至归档位置。见[整理及静态审阅记录](../docs/refactor/cs-t0/tx-01-72-directory-cleanup.md)。本轮未运行游戏、构建、代码测试或哈希校验。

tx-01-71（2026-10-18）：已吸收用户指定的旧REPENTOGON静态参考，按要求使用GPT6 Luna / max逆向，主智能体复核、更正并补齐生命周期与原生记录producer。剩余43项静态问题已交付原生依据、采样接入、读取和逐字段比较方案，见[合并报告](../docs/refactor/cs-t0/t0-06/tx-01-71-static-completion.md)和[整体静态审阅](../docs/refactor/cs-t0/t0-06/tx-01-71-review.md)。未写运行时代码、未运行游戏/构建/代码测试或计算校验哈希；采集器、身份与比较仍待实现，T0-06整体未完成。

tx-01-69（2026-10-18）：主智能体读取指定会话并核对其最终答复列出的偏移，汇总既有当前证据并补核分类、Index、flags、Grid 与虚槽。统一列出 8 处旧数值更正；本会话约定的操作 3 最小只读静态字段范围至此补齐，未实现采集器或后续身份/比较。见[偏移核对表](../docs/refactor/cs-t0/t0-06/tx-01-69-offset-reconciliation.md)和[静态审阅](../docs/refactor/cs-t0/t0-06/tx-01-69-review.md)。

tx-01-68（2026-10-18）：按用户要求，由 GPT6 Luna / max 使用 IDA MCP 完成 seed/归属原样只读采样所需的最小字段研究，主智能体复核并补全 Grid.Desc、独立 RNG 和生成者分类。已登记类别、偏移、宽度、引用存储形式和必要初始化；不穷尽 Morph/Restore/复用行为，不跟随未知目标解引用，也未建立跨 tick 或跨进程身份。见 [seed/归属研究](../docs/refactor/cs-t0/t0-06/tx-01-68-seed-owner.md)和[静态审阅](../docs/refactor/cs-t0/t0-06/tx-01-68-review.md)。

tx-01-67（2026-10-18）：按用户要求，由 GPT6 Luna / max 使用 IDA MCP 完成 state/timer 原样只读采样所需的最小字段研究，主智能体复核并更正派生 Init 与 Grid 字段解释。已登记类别、偏移、类型/宽度和必要初始化线索；不穷尽 AI 状态、全部计时单位或重置路径，不将计数归零当重生，也未实现采集器。见 [state/timer 研究](../docs/refactor/cs-t0/t0-06/tx-01-67-state-timer.md)和[静态审阅](../docs/refactor/cs-t0/t0-06/tx-01-67-review.md)。

tx-01-66（2026-10-18）：按用户要求，由 GPT6 Luna / max 使用 IDA MCP 研究只读采样所需的静态依据，再由主智能体审阅。范围为位置/速度字段、完整普通更新与奇数帧 Player 路径的采样归属，以及暂停、loading、换房和 teardown 的处理；不穷尽全部位置 writer，不实现或运行采集器。见 [只读采样研究](../docs/refactor/cs-t0/t0-06/tx-01-66-readonly-sampling.md)。

tx-01-65（2026-10-18）：按用户要求，由 GPT6 Luna / max 使用 IDA MCP 研究 T0-06 操作 3 的 HP 缺口，再由主智能体复核。登记通用 HitPoints/MaxHitPoints 与独立 Player hearts 布局、初始化和关键变更时机；各 Entity 派生类均用通用字段表示生命按用户假设保留。仅静态研究，未运行游戏、构建或代码测试，其它第三步字段仍待研究。见 [HP 交接](../docs/refactor/cs-t0/t0-06/tx-01-65-hp-analysis.md)。

tx-01-64（2026-10-17）：继续读取指定 REPENTOGON 会话的新类别覆盖/偏移答复，以当前 IDA 完成 T0-06 操作 2 的静态覆盖规格。修正玩家角色/index/twin/backup/替换期关联、工厂及 Familiar stride 等旧偏移；补全七类动态 RTTI、ToTear、查询普通/wisp 分支例外及门/Grid 别名。保存 39 函数、8 段调用点和 32 RTTI/虚表，实际 capture 和操作 3–9 尚未开始。见[第二步交接](../docs/refactor/cs-t0/t0-06/tx-01-64-category-coverage.md)。

tx-01-63（2026-10-17）：按用户要求仅开始 T0-06 操作 1，吸收指定 REPENTOGON 会话并用当前 IDA 补全容器、遍历、Spawn/Init、Remove/提交、池复用、玩家及 Grid 析构路径。已保存静态交接和 40 个函数的原始证据；操作 2–9 及实际 capture 尚未开始。本轮仅文档静态验收，见 [T0-06 入口](../docs/refactor/cs-t0/t0-06/README.md)。

tx-01-62（仅记录审阅）：当前Virtual Device原定42项+新增选角1项，43项均已有历史实机PASS记录，无从未覆盖条目。最新61完整单轮仍为33 PASS/10 NOT_OBSERVED，其专项为32/32 PASS；累计观察不改写成当前版本单轮全PASS，也不宣布整个T0-05或后续阶段完成。未运行测试，见[完整覆盖核对](../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-62-e2e-completion-review.md)。

tx-01-61（2026-10-17）：B=1（Magdalene）选角与剩余Pill/Card/held10一次专项32/32 PASS；药丸资源5→0实际消费，held10三次controlled companion纯读一致；B冲突退休/A继续、菜单teardown和51Hook恢复/DLL常驻通过。493,550事件、10,374tick，原生/传输错误为零。tx-01-60受限终端进程不可见而在注入前FAIL保留；本轮修复执行环境与PID/创建时间绑定，只实际注入一次。完整T0-05不外推，见[本轮实机结果](../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-61-live-process-visibility.md)。

tx-01-59：角色选择报告链路已修复，复用tx-01-58成功构建的控制器/Native/Helper，一次CLI与报告验收35/35 PASS，逐项文本证据和最终JSON可读。tx-01-58报告序列化内存不足的整体FAIL保留；未重新构建或操作游戏。见[报告修复及结果](../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-59-reporting-repair.md)。

tx-01-58（2026-10-17）：新增 B 加入时角色选择。控制器 `coop-active --b-character azazel`（或数字ID），现行实机入口 `-BCharacter azazel`；省略时沿用 `native`。A 沿用游戏开局选择；显式B必须在当前原生可选列表，普通工厂仍单次创建，结果记录请求/选择/实际subtype。`--list-characters` 查看名称，`--describe-character azazel` 只读解释选择。隔离CoopActive IPC升v3，需同轮控制器/native。最后两项实机测试尚未开始；本次范围和验收见[角色选择说明](../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-58-character-selection.md)。

tx-01-57（2026-10-17）：`tests/` 已整理，旧Join/静态闭合/Virtual Device哈希验收的15组工具移入 `tests/trash/`，保留8个Cargo集成测试及support。15个旧入口保留原内容副本并改为归档提示；现行E2E入口和实机结果不变。见[测试目录说明](tests/README.md)和[迁移清单](tests/trash/archive-index.json)。

tx-01-56：异常Join的原生Baby分支已修复，一次专项21/21 PASS。实际三处Baby=true仅对自有K选择普通路径，两pass内单次普通工厂/完整Type1-Variant0 B，物理设备分支和A健康保持；用户确认表现正常。435,907事件、7,777 tick、零原生/传输错误，51 Hook恢复/DLL常驻。tx-01-51旧来源仍为静态推断，完整T0-05未全部完成，见[本轮实现与结果](../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-56-abnormal-join.md)。

tx-01-55（2026-10-17）：Loading／LoadingExit 与同一查询重复读取一次专项20/20 PASS。实际Continue加载1次、恢复前中性；A/B三种查询102组/306次受控纯读，值与完整Participant状态不变。260,528事件、4,645 tick、零原生/传输错误，48 Hook恢复/DLL常驻。范围为初次Continue及加载后的查询，完整T0-05未全部完成。见[本轮结果](../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-55-loading-repeated-query.md)。

tx-01-54（2026-10-17）：跨房非模拟窗口暂停原生 Shooting 已一次专项 E2E 18/18 PASS，用户确认持续按住往返跨房不误发、真实释放/再蓄力与暂停恢复正常。A/B 各75次非模拟暂停，正常 tick 缺输入仍消费中性；1,074,896事件、19,027 tick、原生/传输错误为零，48 Hook恢复/DLL常驻。完整 T0-05 E2E 未全部完成。见[本轮结果](../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-54-room-shooting-suspension.md)。

tx-01-53（2026-10-17）：B 原始射击脚本移动量重复死区已修复并一次实机验收8/8 PASS。101/101 raw 步骤与4/4 canonical步骤原样消费移动 pair，普通 Join/身份、两原生查询路径、去重/旧 generation 拒绝及48 Hook恢复/DLL常驻通过；171,783事件、3,178 tick、原生/传输错误为零。A与方向选择器未变，未重复52人工蓄力测试。见[本轮结果](../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-53-script-movement-preservation.md)。

tx-01-52（2026-10-17）：后按相反射击方向优先已一次实机确认，A/B source→Resolve→query 和用户画面共36项专项 PASS；48 Hook 恢复、DLL 常驻。另发现新增 B raw 脚本把移动8192重复死区为4681，补充整体 FAIL，原专项 PASS保留、不重跑。隔离 IPC v2、24/64字节 V2 不变。跨房自动发射及51未接纳 Join 原因仍待处理。见[本轮结果](../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-52-opposite-shooting-priority.md)。

tx-01-51（2026-10-17）：统一启动例外已实现并执行一次验收。A 完成 51 个首包/nominal Resolve 配对，其中 45 次 room 后启动；336,848 事件、6,524 tick、原生/传输错误为零，48 槽恢复/DLL 常驻。自动报告 NOT_OBSERVED（本轮 B 没有经创建 owner 接纳）；用户确认相邻重叠换向保留蓄力，但跨房未松键仍自动发出激光，补充最终结果 FAIL。未自动重跑。见[方案、实现与结果](../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-51-startup-input-repair.md)。

tx-01-50（2026-10-16/17）：授权采集确认额外更新来自原生奇数帧 955241→6FD3F0，已补齐同 scheduler 窗口。一次最终 E2E：650,671 事件、12,624 tick；B 死亡/复活及新输入恢复、旧 generation 拒绝均 PASS；48 槽恢复、DLL 常驻。专项整体 FAIL（19 PASS、1 FAIL：held 下原生瞄准字段中性），未重跑。用户补充换房与相邻方向切换时仍丢蓄力，连续性尚未修复完成。见[来源、修复和结果](../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-50-input-window-repair.md)。

tx-01-49（2026-10-16）：用户要求先补验 B 死亡/复活，复用 tx-01-48 产物执行一次专项窗口。死亡操作前发现 A（阿萨泻勒/键盘）移动缓慢、疑似蓄力被打断；原生事件确认有效更新后，同 tick 的窗口外 consumer 又被中性覆盖。死亡/复活未执行，专项 NOT_OBSERVED（3 PASS、8 未观察项）；正常 teardown、47 槽恢复和 DLL 常驻完成。该输入缺陷尚未修复，完整 owner 待核对；用户确认未启用 TimeMachine，不能归因于该模组。见[本轮记录](../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-49-b-death-respawn-e2e.md)。

tx-01-48（2026-10-16）：修复把 Lua engine 存在误判成输入 callback 13 接管的拓扑条件，并增加拒绝原因和实际身份诊断。一次统一构建/实机窗口已确认 A/B 绑定、设备单次注册、原生 Join 和身份匹配；B 冲突退休后 A 继续，菜单 teardown 和 47 槽恢复成功、DLL 常驻。731,674 事件、11,868 tick、35 项 PASS；完整 E2E 为 NOT_OBSERVED（7 项缺少观察），未自动重跑。见[修复与本轮结果](../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-48-binding-join-repair.md)。下列各轮原始结果保留。

tx-01-47（2026-10-16）：`Publish` 改为只读引用，修复已生成 x86 代码中按值调用方与指针被调方不一致。一次实机窗口持续读取 120,032 事件、推进 3,428 tick，初始化与正常 shutdown 成功，47 槽字节恢复。**完整 E2E 为 NOT_OBSERVED：尚未确认 A/B 绑定、设备注册及 Join**。见[事件发布修复与验收](../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-47-event-publication-repair.md)。

tx-01-46（2026-10-16）：统一入口已固定 VS 2022/v143、Hostx64→Win32 及每次独立构建目录，Native/helper 链接成功，LNK1101 已消除。用户确认后继续实机，**远程初始化 0xC0000005，游戏退出，整体 FAIL**；没有再次注入。见[链接修复与该轮结果](../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-46-link-repair.md)。

tx-01-45（2026-10-16，历史轮次）：新增独立 `coop-active` 生产源码和 [Virtual Device 实现/操作清单](../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-45-implementation.md)。该轮统一入口验收 **FAIL，Native 链接 LNK1101 / MSPDB140.DLL 版本不正确，实机 NOT_RUN**；Rust 构建成功，未自动重跑。见[验收记录](../docs/refactor/cs-t0/t0-05/07-virtual-device/tx-01-45-results.md)。它采用 Authority writer、常驻 root、V2 双 participant 和立即中性策略，不调用下面历史 build/preflight 的哈希或多组测试链。

> 2026-10-12 执行约定更新：先读[现行项目规范](../docs/refactor/project-conventions.md)。永不设定 gate，不校验任何哈希；每次推进最小为 `tx-0x-0x`，整个目标完成后仅一次整体测试验收，探讨过程不得测试；逆向与测试可由同一智能体完成。下面旧 gate/解锁字段、哈希、预封、作者分离和多组验收流程只作历史资料，不作为当前要求；事实与未决功能如实保留。

05-05 native alias 固定点清零 Batch 2 v7 候选（2026-09-09，独立验收失败）：create-new [native alias universe v4](trash/tx-01-72/native-alias-universe-v4.json)与[decision v7](trash/tx-01-72/coop-virtual-device-ownership-lifecycle-decision-v7.json)按父 row identity 生成候选差分：register-IAT `2,333` 中 `46` 条 `_wassert` call resolved、`2,287` retained；`763` 条 `FF 24` tail 中 `3` 条同函数 switch proven、`760` retained；无新增 import alias escape 或 exact frontier，候选 receiver 为 `20,695`、graph 为 `41/44`、首边 RVA `0x00007455`，pair 仍为 `34`。唯一正式 Group 1 的独立 oracle 因把两个“有 source、无父 residual sink”的点错误计入 source/function census 而 `REJECTED`；按 fail-stop，公开 create-new、verify-existing 和两-method 负向批次均未运行，计划输出未创建。v4/v7 及失败链保留但不是正式入口；当前仍使用 v6/v3，`UNKNOWN`、`formalDecision=null`、全部权限与解锁关闭。详见 [Batch 2 TEST 结果](../docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-fixed-point-clearing-batch2-test-results-v1.md)。

05-05 native alias 固定点清零 Batch 1 v6（2026-09-09）：[decision v6](trash/tx-01-72/coop-virtual-device-ownership-lifecycle-decision-v6.json)绑定 [native alias universe v3](trash/tx-01-72/native-alias-universe-v3.json)、独立 TEST 预封、独立 REVERSE 交接和 v6 只读工具链。严格 IAT 半开区间 `[0x00718000,0x00718A28)` 将 18,107 个 `FF 15` 与 133 个 `FF 25` 逐行移入 import exclusions；唯一明确 tainted import `_beginthreadex` 已沿固定 context/thread 链闭合，新增 `IMPORT_ALIAS_ESCAPE=0`。callback slot `0x0087E814` 的 101 个 reads 在固定映像、loader directories 和可达 address escape 域内证明初始零且无 writer/escape/mutation；Manager service 的 derived/base 两个 vptr 分支也已闭合。receiver graph 现为 20 nodes / 26 edges，frontier `38,985 -> 20,744`；pair graph 10 / 8，frontier `37 -> 34`。四组压缩测试各一次全部 PASS，负向批次仅两个 test methods；[verification v6](../out/cs-t0/virtual-device-native-alias-fixed-point-clearing-batch1-verification-20260909-v6.json) SHA-256 `301D91F8812A82B5F64052C3DAD27872E270F26199015123B4CCC6B19F68915D`。合法结论仍是 `UNKNOWN`，首边 RVA `0x00006A75`，`dynamicAliasUniverseClosed=false`、对象 owner 不提升、`formalDecision=null`、全部权限与解锁关闭。下一批仍有 474 base-only、270 split、16,323 residual、3,677 tail、33 pair liveouts 和 1 exception/nonlocal edge；factory/rollback 与 release/thread 未开始。详见 [07-virtual-device](../docs/refactor/cs-t0/t0-05/07-virtual-device/README.md)与[独立 TEST Batch 1](../docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-fixed-point-clearing-batch1-test-results-v1.md)。

05-05 native alias universe 保守闭合 v5（2026-09-09）：[decision v5](trash/tx-01-72/coop-virtual-device-ownership-lifecycle-decision-v5.json)绑定 [native alias universe v2](trash/tx-01-72/native-alias-universe-v2.json)、独立 REVERSE、完整 TEST 失败链及 37 项只读证据。固定 PE 上 28 个 direct seeds、2 个 vtable cells 与 35,189 个 indirect calls 被精确分成 489 base-only、270 split-register、34,430 residual，另保留 3,810 个 indirect-tail frontiers；receiver graph 为 18 nodes / 24 edges，pair graph 为 10 nodes / 8 edges，35 个 pair seeds 中 33 个仍是 unresolved liveout。21 个 IDA noncontiguous tail/chunk 已保守改为 unresolved，当前 8,941 个 function ranges、residual provenance 33,960 identified / 470 unresolved。四组压缩验收最终各运行一次并全部 PASS，负向批次仅 2 个 test methods；[verification retry-v11](../out/cs-t0/virtual-device-native-alias-closure-verification-20260909-v5-retry-v11.json) SHA-256 为 `51191D57704C25E643688BB8698309D250E2B16A4717F9275CD275D4D25EB360`。合法结论是 `UNKNOWN`：首缺口为 RVA `0x00024037` 的 post-release unknown callback，`dynamicAliasUniverseClosed=false`、对象 owner 不提升、`formalDecision=null`、全部权限与 05-06/T0-05/T0-06 解锁关闭。factory/rollback 与 reference-count/final-release caller/thread 留待后续独立工作；本轮未运行 v4 harness、Cargo、完整 build、Native 矩阵、宽回归、游戏或调试器。详见 [07-virtual-device](../docs/refactor/cs-t0/t0-05/07-virtual-device/README.md)与[独立 TEST v12](../docs/refactor/cs-t0/t0-05/07-virtual-device/virtual-device-native-alias-closure-test-results-v12.md)。

05-05 lifecycle 离线实现与剩余静态闭合 v4（2026-09-08）：[decision v4](trash/tx-01-72/coop-virtual-device-ownership-lifecycle-decision-v4.json)哈希绑定最终 `Release|Win32` 离线 harness、REVERSE v2、v3 前序、生产隔离文件和 v4 验证器/负向批次。七态 coordinator、六类 strong ref、external weak=0 与 13 个 rollback 已通过 16 cases / 332 assertions；三项 `offlineImplementationStatus=OFFLINE_IMPLEMENTATION_CONFIRMED`。原生 keyed-reader alias、真实 virtual factory/rollback 和 last-release caller/thread 仍未闭合，所以三项 native/formal 状态保持 `UNKNOWN`、`promotionCount=0`、`formalDecision=null`、`implementationEligibility=false`、`BLOCKED_BY_EVIDENCE`。只读入口为 `pwsh -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v4.ps1 -GameExe <PE> -RepositoryRoot . [-Output <new-json>]`；最终四组压缩验收全部 PASS，所有权限与 05-06/T0-05/T0-06 解锁继续关闭。联合逆向与独立验收见 [07-virtual-device](../docs/refactor/cs-t0/t0-05/07-virtual-device/README.md)。

05-05 ownership/lifecycle 证明审计（2026-09-08）：[decision v2](trash/tx-01-72/coop-virtual-device-ownership-lifecycle-decision-v2.json)精确复核对象 owner、创建/析构与引用计数三项。通用 `0x18` control block、六槽 strong/weak ABI、last-strong/last-weak 和 manager vector 正常 release 子链已闭合；但 callback retained ref、创建失败/异常 rollback、last-release thread 及 16-bit 计数边界未闭合，因此 `promotionCount=0`，三项仍为 `UNKNOWN`。只读入口为 `pwsh -File .\cs-t0\verify-coop-virtual-device-ownership-lifecycle-decision-v2.ps1 -GameExe <PE> -RepositoryRoot . [-Output <new-json>]`；05-06 与全部权限继续关闭。

05-05 只读 Virtual Device 决策（2026-09-08）：固定目标要求 Endpoint B 在 Authority 没有第二实体手柄时持续工作，因此 [decision v1](trash/tx-01-72/coop-virtual-device-decision-v1.json) 固定 `necessityAssessment=REQUIRED_FOR_PHYSICAL_DEVICE_FREE_ENDPOINT_B`。但对象 owner、创建/析构、引用计数、线程、device key、vector 移除、完整 vtable/COL/slot ABI、assignment、generation 和 teardown 证据尚未闭合，所以 `formalDecision=null`、`workPackageStatus=BLOCKED_BY_EVIDENCE`，05-06 与全部运行权限保持关闭。只读入口为 `pwsh -File .\cs-t0\verify-coop-virtual-device-decision.ps1 -GameExe <PE> -RepositoryRoot . [-Output <new-json>]`；成功表示阻断结论完整，不表示 05-05 通过。逆向与独立验收见 [07-virtual-device](../docs/refactor/cs-t0/t0-05/07-virtual-device/README.md)。

05-04 最新文档决策（2026-09-06）：用户已在[原生 Join 创建/发布规划假设 v1](../docs/refactor/cs-t0/t0-05/02-active-join/native-join-active-user-planning-assumption-v1.md)中接受“原生 local-co-op Join 可可靠创建并发布第二玩家 B”为 `DESIGN_ONLY` 前提，可据此准备 05-05 的只读设计。严格证据不改写：[MT-R02 checkpoint v41](../docs/refactor/cs-t0/t0-05/06-manager-identity/00-overview-and-gates/native-join-manager-this-mt-r02-animation-descriptor-checkpoint-v41.md) 仍为 `0 positive / 2 UNKNOWN`，v33 strict remaining `99`、人工逆向队列 `96`；plan v10、join progress v4、active/runtime、正式 05-05、T0-05 总门与 T0-06 均未解锁。

05-04 最新进展（2026-09-01）：[plan v10](trash/tx-01-72/coop-plan-v10.json)与[join progress v4](trash/tx-01-72/coop-native-join-progress-v4.json)进入 `BLOCKED_BY_EVIDENCE / STATIC_ADMISSION_CONTRACT_REVIEW`。新增 Init v3、持久化窗口 v2、[准入契约 v1](trash/tx-01-72/coop-native-join-admission-contract-v1.json)和独立磁盘入口；正式验证通过23个契约用例、104个离线模型用例、21个解析/引用用例及5组公开黑盒/57项断言。新49锚点、1,524条指令、283条直接边与旧证据独立复核通过，但完整 Init/P0 状态保持、自然更新窗口和原生reader/ABA仍有缺口；不能宣称准入契约完整或生产reader已实现。详见[实施记录](../docs/refactor/cs-t0/t0-05/03-admission/native-join-admission-implementation-v1.md)和[独立测试结果](../docs/refactor/cs-t0/t0-05/03-admission/native-join-admission-test-results-v1.md)。

新入口均只读磁盘，不打开游戏进程。Inspector 的合法输入仍退出1并报告 `BLOCKED_BY_EVIDENCE`；非法输入拒绝且不生成报告。Verifier 仅在实际完整检查后退出0，表示静态证据与离线规格审查通过，不授予主动能力。`-Output` 可省略；指定时只能使用 `out/cs-t0` 或 `artifacts/cs-t0` 下不存在的新 JSON 文件。Python 与独立 objdump 不可用时不得计 PASS。

```powershell
pwsh -File .\cs-t0\inspect-coop-native-join-admission.ps1 `
  -GameExe '<path-to-isaac-ng.exe>' -RepositoryRoot .
pwsh -File .\cs-t0\verify-coop-native-join-admission.ps1 `
  -GameExe '<path-to-isaac-ng.exe>' -RepositoryRoot . `
  -Objdump '<path-to-objdump.exe>'
```

生产 `coop-join-preflight` 的原七项缺口和退出行为保持不变；新规格保留12项未决义务，并单列已规定的永久失能/Unsafe规则。矩阵第11项仍为 `NOT_STARTED`，55槽主动runtime、IPC、arm/claim、active/live、05-05、T0-05整体和T0-06均未解锁。

首次[plan v9](trash/tx-01-72/coop-plan-v9.json) / [join progress v3](trash/tx-01-72/coop-native-join-progress-v3.json)发布遗留旧工具列表、verifier入口和scope；独立发布审计指出后，原件和失败记录均保留，v10/v4仅修正发布元数据，不改证据、契约、正式verification或权限。

05-04 条件审查记录（2026-08-31）：[plan v8](trash/tx-01-72/coop-plan-v8.json)与[join progress v2](trash/tx-01-72/coop-native-join-progress-v2.json)已进入 `BLOCKED_BY_EVIDENCE / STATIC_CONDITION_REVIEW`。新增独立的磁盘条件审查入口，两份证据的146锚点、1,994条指令、456条边，以及14组工具对抗和9组公开黑盒通过；生产程序与旧验证链未变。剩余静态缺口已收窄为 fresh B 的有效状态传播，以及 Init/自然更新窗口的转场、保存和readonly保持；尚不能接入主动runtime或申请实机。详见[该轮实施记录](../docs/refactor/cs-t0/t0-05/05-persistence/native-join-static-closure-implementation-v1.md)和[独立测试结果](../docs/refactor/cs-t0/t0-05/05-persistence/native-join-static-closure-test-results-v1.md)。

05-04 前置实施记录（2026-08-31）：已建立55槽静态候选目录、冻结164文件历史快照，并实现 `coop-join-preflight --game-exe <PE> --repository-root <root> [--output <new-json>]`。该入口仅作磁盘/历史证据检查；合法目标仍返回 `BLOCKED_BY_EVIDENCE`、退出1，不枚举/打开进程、不创建IPC、arm或注入。原生join可达持久化写入及Lua回调，独立prejoin与无污染场景尚未闭合；主动runtime和05-05未解锁。详见[前置实施记录](../docs/refactor/cs-t0/t0-05/02-active-join/native-join-active-implementation-v1.md)，下文旧进度按历史时间保留。

`IsaacCsT0.exe` 承载已通过的 T0-01 只读证据、已通过的 T0-02 模式隔离，以及按用户修订矩阵通过的 T0-03 被动边界研究。`IsaacCsT0.Native.dll`、`IsaacCsT0.Helper.exe`、各阶段 IPC magic、日志和构建输出全部独立；它们不链接或修改 Isaac Sync 1.0.4 的 protocol 5、ABI 5、Lua API、channel `0xFD`、Cargo workspace 或运行时状态。

05-04 前置阶段的[verification v1](trash/tx-01-72/coop-native-join-verification-v1.json)已通过只读阻断门的8组黑盒和完整继承回归；历史[plan v7](trash/tx-01-72/coop-plan-v7.json)与[join progress v1](trash/tx-01-72/coop-native-join-progress-v1.json)保持`BLOCKED_BY_EVIDENCE / STATIC_PREREQUISITE`。这不是新55槽runtime或实机通过，矩阵第11项保持`NOT_STARTED`。实际执行和失败保留见[独立验证结果](../docs/refactor/cs-t0/t0-05/02-active-join/native-join-active-test-results-v1.md)。

05-03 完成记录（2026-08-31）：五份适用 PASS source＋限域 N/A 的新 gate 已正式 PASS，完整自动门 **28/28**、新增黑盒 **17/17**、独立最终审计 **15/15** 通过。[plan v6](coop-plan-v6.json) 仅解锁 CS-T0-05-04 工作包资格并关闭 passive capture；active 实机和 T0-05 总解锁仍为 false。旧六源 gate、live-07 BLOCKED 及所有历史证据保持不变。详见 [gate 迁移记录](../docs/refactor/cs-t0/t0-05/01-passive-lifecycle/native-exit-applicability-gate-v1.md) 和 [执行状态](../docs/refactor/cs-t0/t0-05/00-overview/isaac-cs-t0-05-status.md)；本页后续记录保留各次当时的进度与授权。

## 文件分层

- 截至 T0-04 的可复用逆向结论统一从 [T0-04 逆向交接](../docs/refactor/cs-t0/t0-04/isaac-cs-t0-04-reverse-handoff.md) 进入。
- T0-05 的阶段任务、证据门和验收矩阵见 [T0-05 研究计划](../docs/refactor/cs-t0/t0-05/00-overview/isaac-cs-t0-05-research-plan.md)。
- 本目录顶层 JSON 只保留当前机器契约，以及仍被源码回归按固定路径读取的拒绝样本；不能仅按版本号判断可删除性。
- `ledger/` 保存 T0-01～03 的机器账本，`testdata/` 保存测试夹具，二者均不是历史草稿。
- 已被交接文档取代的阶段过程文档和不再被运行时代码读取的 raw v1/v2 契约移至 [历史归档](archive/README.md)，保留原文件名、身份和证据哈希。

## 安全边界

- `inspect` 只读取磁盘 PE 与账本。
- `coop-inspect` 只读取磁盘 PE 与静态 JSON，不枚举或打开进程；stdout 为确定性 JSON，`--output` 仅允许 create-new。
- `coop-smoke --probe control-no-native` 是独立的 C0 `VM_READ` observer：不安装 Hook，不创建 IPC/mapping/helper，不加载 Native，不执行 scheduler/apply；三个历史场景均经单独人工确认完成。当前 `coop-plan-v4.json` 已关闭 observer 与进程读取，不再授权额外 C0 capture。
- `smoke` 不具备启动、注入、写内存、挂断点或终止游戏的代码路径；进程句柄只申请 `PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE`。
- `smoke` 启动时要求游戏尚未运行，并拒绝 IssacOnlineSync、x32dbg/x64dbg、Cheat Engine 以及已知 Isaac Sync/T0 Native 模块。
- artifact 只写模块 basename，不写游戏绝对路径、Windows 用户名或完整 SteamID。
- 任一 fingerprint、section、唯一特征、call/IAT target、owner、重定位或前后字节不匹配都以退出码 `1` fail closed。
- T0-02 的 `DllMain` 不安装 hook；只有 `mode-smoke` 找到用户启动的精确进程后，独立 x86 helper 才显式调用初始化导出。
- T0-02 wrapper 只记录并恰好调用保存的原函数一次，不发送 C/S 包、不排空新 transport、不排队输入、不修改参数/返回值，也不提供 CustomAuthority 实机入口。
- 固定 call/IAT 与动态 Steam vtable 分别事务安装。退出观察后必须先恢复全部目标、等待 wrapper 排空并卸载 DLL，再捕获 after snapshot；不能证明恢复时 DLL 保持加载并记为失败。
- T0-03 使用独立 boundary IPC v3 和初始化导出。它只记录已验证 call/IAT 的被动事件，不注入输入、不跳过更新、不修改返回值；OriginalOnline、错误线程或 ring overflow 均触发失能/恢复。Lua rebuild 专用路径在旧 root close 后保持失能，只等待已验证 creator 返回，再记录新 generation 并恢复。v3 另记录已验证主循环 suppression 状态和唯一 `MC_POST_NEW_ROOM` 调用点，不读取 room-id 或遍历容器。

## 构建

```powershell
pwsh -File .\cs-t0\build.ps1 -Configuration Release
```

发布输出仅位于 `out/cs-t0/`：`IsaacCsT0.exe`、`IsaacCsT0.Native.dll` 和 `IsaacCsT0.Helper.exe`。本 workspace 使用自己的 `Cargo.lock` 和版本 `0.0.1`，独立 Native 项目不加入现有解决方案。

## 命令

05-04新增的条件审查与独立静态门（均不接触游戏进程）：

```powershell
pwsh -NoProfile -File .\cs-t0\inspect-coop-native-join-static-closure.ps1 -GameExe <disk-isaac-ng.exe> -RepositoryRoot .
pwsh -NoProfile -File .\cs-t0\verify-coop-native-join-static-closure.ps1 -GameExe <disk-isaac-ng.exe> -RepositoryRoot . -Output .\out\cs-t0\<new-verification.json>
```

inspection合法输入仍退出1；verifier的成功仅为`PASS_STATIC_CONDITION_REVIEW_VERIFICATION`，不证明完整无污染、不重建生产程序。两者输出均create-new；默认verification路径已被本轮占用，重跑须提供新路径，不能覆盖旧报告。原生产preflight继续按旧static v1报告七项缺口，新schema另列12项静态/实现/实机义务。

05-04当前可用的只读前置入口：

```powershell
.\out\cs-t0\IsaacCsT0.exe coop-join-preflight --game-exe <isaac-ng.exe> --repository-root .
```

有效静态输入仍输出`BLOCKED_BY_EVIDENCE`并退出1；这是尚未闭合主动离线/持久化证据的诊断，不是可用授权。可用`--output`将诊断create-new写入现有`out/cs-t0`目录，禁止写进任何历史source。输出文件名须为ASCII字母、数字、点、下划线或短横线组成的`.json`名称，不接受Windows设备名或备用数据流。`verify-coop-native-join.ps1`的成功仅表示阻断门与继承回归通过，详见[实施记录](../docs/refactor/cs-t0/t0-05/02-active-join/native-join-active-implementation-v1.md)。

```powershell
out\cs-t0\IsaacCsT0.exe inspect `
  --game-exe <isaac-ng.exe> `
  --ledger cs-t0\ledger\isaac-cs-t0-target-ledger.json `
  --expected-variant online-mod-patched-2026-04-21

out\cs-t0\IsaacCsT0.exe coop-inspect `
  --game-exe <isaac-ng.exe> `
  --target-equivalence out\cs-t0\target-equivalence-inspect\unpacked-target-equivalence-v1-release-final.json `
  --bootstrap-decision cs-t0\coop-bootstrap-decision-v1.json `
  --ledger cs-t0\ledger\isaac-cs-t0-coop-ledger-v2.json `
  --catalog cs-t0\coop-catalog-v2.json `
  --graph cs-t0\coop-static-graph-v1.json `
  --output <new-coop-static-inspection.json>

out\cs-t0\IsaacCsT0.exe coop-smoke `
  --probe control-no-native `
  --scenario <offline-single-c0|local-coop-dual-c0|original-online-dual-disarmed> `
  --game-exe <isaac-ng.exe> `
  --source-dir <new-source-directory> `
  --case-id <new-case-id>

out\cs-t0\IsaacCsT0.exe coop-analyze <source-directory> --output <new-analysis.json>

out\cs-t0\IsaacCsT0.exe coop-compare `
  --offline-single <offline-source-directory> `
  --local-coop-dual <local-dual-source-directory> `
  --original-online-dual <online-dual-source-directory> `
  --output <new-comparison.json>

out\cs-t0\IsaacCsT0.exe smoke `
  --game-exe <isaac-ng.exe> `
  --ledger cs-t0\ledger\isaac-cs-t0-target-ledger.json `
  --expected-variant online-mod-patched-2026-04-21 `
  --artifacts-root artifacts\cs-t0 `
  --case-id t0-01-baseline

out\cs-t0\IsaacCsT0.exe analyze <artifact-run-directory>
```

完整 `coop-smoke` 参数由严格 CLI parser 冻结；缺失 plan/layout/equivalence/catalog/graph 路径、未知字段、重复选项或已有输出均 fail closed。默认窗口为 5 秒、间隔 50 ms、每个 attempt 最多四次局部双读；dropped attempt 不得携带部分对象图。artifact 使用单次 source 的 opaque token/proof，不落盘 PID、VA、module base、pointer、设备/assignment key、raw InitSeed、平台身份或 peer identity。

T0-02 静态检查：

```powershell
out\cs-t0\IsaacCsT0.exe mode-inspect `
  --game-exe <isaac-ng.exe> `
  --ledger cs-t0\ledger\isaac-cs-t0-mode-ledger.json `
  --expected-variant online-mod-patched-2026-04-21
```

T0-02 实机命令必须在游戏尚未启动时运行；它只等待用户启动，不会代为启动游戏。观察完成后在控制器终端按 Enter，控制器才会安全卸载并捕获恢复快照：

Offline C0/C1 必须启用相同的标准 `MC_POST_UPDATE` callback witness；没有 Mod callback 的运行只能证明 hook 安装/恢复，不能冻结 Offline。当前实机使用已安装的 `K to Treasure Room`，在 `Isaac.Sync` 不存在时其 callback 立即返回，不发送包或执行游戏操作。

```powershell
out\cs-t0\IsaacCsT0.exe mode-smoke `
  --scenario <control-no-native|offline-disarmed|original-online> `
  --endpoint <solo|a|b> `
  --game-exe <isaac-ng.exe> `
  --ledger cs-t0\ledger\isaac-cs-t0-mode-ledger.json `
  --expected-variant online-mod-patched-2026-04-21 `
  --artifacts-root artifacts\cs-t0 `
  --case-id <id>

out\cs-t0\IsaacCsT0.exe mode-analyze <artifact-run-directory>

out\cs-t0\IsaacCsT0.exe mode-compare `
  --control <no-native-run> `
  --instrumented <native-run> `
  [--peer <other-instrumented-endpoint>] `
  [--output <new-json-path>]
```

T0-02 最终门禁使用 `mode-gate` 固定引用 T0-01、offline C0/C1、OriginalOnline C0/C2 和 packet-v2 比较的全部证据 hash；输出必须是不存在的新文件。

`original-online` 双端必须在两个终端预先设置相同的临时 32 字节十六进制 key；key 只进入共享内存，不写入 artifact：

```powershell
$pairKeyBytes = [byte[]]::new(32)
[Security.Cryptography.RandomNumberGenerator]::Fill($pairKeyBytes)
$env:ISAAC_CS_T0_PAIR_KEY = [Convert]::ToHexString($pairKeyBytes)
```

完整信号、冲突和实机矩阵见 [T0-02 模式隔离说明](mode-isolation.md)。

T0-03 静态检查与命令：

```powershell
out\cs-t0\IsaacCsT0.exe boundary-inspect `
  --game-exe <isaac-ng.exe> `
  --ledger cs-t0\ledger\isaac-cs-t0-boundary-ledger.json `
  --expected-variant online-mod-patched-2026-04-21

out\cs-t0\IsaacCsT0.exe boundary-smoke `
  --probe <control-no-native|passive> `
  --scenario <normal|input|pause|low-fps|focus-loss|room-transition|menu-return|lua-rebuild> `
  --game-exe <isaac-ng.exe> `
  --ledger cs-t0\ledger\isaac-cs-t0-boundary-ledger.json `
  --expected-variant online-mod-patched-2026-04-21 `
  --artifacts-root artifacts\cs-t0 `
  --case-id <id>

out\cs-t0\IsaacCsT0.exe boundary-analyze <artifact-run-directory>

out\cs-t0\IsaacCsT0.exe boundary-compare `
  --control <no-native-run> `
  --instrumented <passive-run> `
  [--output <new-json-path>]
```

真实 `boundary-smoke` 的约束、静态候选与完整矩阵见 [T0-03 边界研究说明](boundary-research.md)。首次 `--probe passive` 前必须停下等待用户操作与授权。

T0-03R 完整事件循环补充门与 T0-04 输入实验：

```powershell
out\cs-t0\IsaacCsT0.exe loop-inspect `
  --game-exe <isaac-ng.exe> `
  --ledger cs-t0\ledger\isaac-cs-t0-loop-ledger.json `
  --expected-variant online-mod-patched-2026-04-21

out\cs-t0\IsaacCsT0.exe loop-smoke `
  --probe <control-no-native|passive> `
  --scenario <keyboard|controller|simultaneous|rapid-tap|device-switch|pause|focus|loading|room|menu> `
  --game-exe <isaac-ng.exe> `
  --ledger cs-t0\ledger\isaac-cs-t0-loop-ledger.json `
  --expected-variant online-mod-patched-2026-04-21 `
  --artifacts-root artifacts\cs-t0 `
  --case-id <id>

out\cs-t0\IsaacCsT0.exe input-inspect `
  --game-exe <isaac-ng.exe> `
  --loop-ledger cs-t0\ledger\isaac-cs-t0-loop-ledger.json `
  --expected-variant online-mod-patched-2026-04-21

# Create-new, passive-only proof for the one explicitly audited unpacked container. This does not
# replace the frozen target identity and never enables active input.
out\cs-t0\IsaacCsT0.exe input-target-equivalence `
  --game-exe <isaac-ng.exe> `
  --loop-ledger cs-t0\ledger\isaac-cs-t0-loop-ledger.json `
  --target-ledger cs-t0\ledger\isaac-cs-t0-target-ledger.json `
  --active-catalog cs-t0\input-active-catalog-v2.json `
  --active-plan cs-t0\input-active-plan-v2.json `
  --output <new-unpacked-target-equivalence.json>

out\cs-t0\IsaacCsT0.exe input-smoke `
  --probe <control-no-native|passive|loopback|original-online-disarmed> `
  --scenario <scenario> `
  --game-exe <isaac-ng.exe> `
  --loop-ledger cs-t0\ledger\isaac-cs-t0-loop-ledger.json `
  [--loop-gate <derived-v5-loop-gate.json>] `
  --expected-variant online-mod-patched-2026-04-21 `
  --artifacts-root artifacts\cs-t0 `
  --case-id <id> `
  [--arm-token <64-hex-digits>]

# control-no-native and original-online-disarmed require this read-only static bundle and forbid offline-prereq/arm-grant/verification/token/script:
  --target-ledger cs-t0\ledger\isaac-cs-t0-target-ledger.json `
  --target-equivalence <unpacked-target-equivalence.json> `
  --mode-gate <t0-02-mode-gate.json> `
  --loop-gate <derived-v5-loop-gate.json> `
  --raw-prereq <raw-prerequisite-4of4.json> `
  --active-catalog cs-t0\input-active-catalog-v2.json `
  --active-plan cs-t0\input-active-plan-v2.json

# loopback active-runtime scenarios additionally require this passive-prerequisite/grant bundle:
  --target-ledger cs-t0\ledger\isaac-cs-t0-target-ledger.json `
  --target-equivalence <unpacked-target-equivalence.json> `
  --mode-gate <t0-02-mode-gate.json> `
  --loop-gate <derived-v5-loop-gate.json> `
  --raw-prereq <raw-prerequisite-4of4.json> `
  --active-catalog cs-t0\input-active-catalog-v2.json `
  --active-plan cs-t0\input-active-plan-v2.json `
  --offline-prereq <offline-attestation-run\prerequisite.json> `
  --arm-grant <arm-grant.json> `
  --active-verification <create-new-active-verification-v2.json> `
  --arm-token <nonzero-64-hex-digits>

# scenario=loopback repeatability additionally requires:
  --input-script cs-t0\input-active-repeatability-script-v1.json

out\cs-t0\IsaacCsT0.exe input-raw-smoke `
  --probe passive `
  --scenario <keyboard-held-release|keyboard-edge|controller-held-release|controller-edge> `
  --game-exe <isaac-ng.exe> `
  --loop-ledger cs-t0\ledger\isaac-cs-t0-loop-ledger.json `
  --loop-gate <derived-v5-loop-gate.json> `
  --raw-apply-catalog cs-t0\raw-apply-catalog-v4.json `
  --capture-plan cs-t0\raw-apply-capture-plan-v4.json `
  --expected-variant online-mod-patched-2026-04-21 `
  --artifacts-root artifacts\cs-t0 `
  --case-id <id>

out\cs-t0\IsaacCsT0.exe input-raw-prereq `
  --matrix cs-t0\input-raw-prereq-matrix-v1.json `
  --output <new-prerequisite.json>

# Passive-only same-session attestation. The canonical frozen identity remains C411; the only
# accepted drift is the exact, audited 1FBA unpacked container bound by the equivalence document.
# In the live PE header, ImageBase must remain the frozen preferred base (0x00400000) or equal
# the actual loaded module base; the latter is the narrowly approved unpacked-loader rewrite.
out\cs-t0\IsaacCsT0.exe input-offline-attest `
  --scenario <one-active-runtime-scenario> `
  --game-exe <isaac-ng.exe> `
  --loop-ledger cs-t0\ledger\isaac-cs-t0-loop-ledger.json `
  --target-ledger cs-t0\ledger\isaac-cs-t0-target-ledger.json `
  --target-equivalence <unpacked-target-equivalence.json> `
  --mode-gate <t0-02-mode-gate.json> `
  --loop-gate <derived-v5-loop-gate.json> `
  --raw-prereq <raw-prerequisite-4of4.json> `
  --active-catalog cs-t0\input-active-catalog-v2.json `
  --active-plan cs-t0\input-active-plan-v2.json `
  --expected-variant online-mod-patched-2026-04-21 `
  --artifacts-root artifacts\cs-t0 `
  --case-id <id>

out\cs-t0\IsaacCsT0.exe input-arm-grant `
  --offline-prereq <offline-attestation-run\prerequisite.json> `
  --case <same-case-id> `
  --scenario <same-scenario> `
  --arm-token <nonzero-64-hex-digits> `
  --output <new-arm-grant.json>

out\cs-t0\IsaacCsT0.exe loop-analyze <run-directory>
out\cs-t0\IsaacCsT0.exe loop-gate --matrix <matrix.json> --output <new-gate.json>
out\cs-t0\IsaacCsT0.exe input-import-witness `
  --witness-log <log.txt> `
  --output <new-artifact-directory> `
  --scenario <keyboard|controller|simultaneous|rapid-tap|device-switch|pause|focus|loading|room|menu> `
  --case-id <id> `
  --target-sha256 <64-hex-digits> `
  --loop-gate-sha256 <64-hex-digits> `
  [--window-min <positive-window>] `
  [--window-max <positive-window>] `
  [--controller-index <signed-index>]
out\cs-t0\IsaacCsT0.exe input-rpm-inspect `
  --game-exe <isaac-ng.exe> `
  [--pid <decimal-pid>]
out\cs-t0\IsaacCsT0.exe input-rpm-capture `
  --game-exe <isaac-ng.exe> `
  [--pid <decimal-pid>] `
  --output <new-artifact-directory> `
  --duration-ms <1..300000> `
  --interval-ms <1..1000>
out\cs-t0\IsaacCsT0.exe input-rpm-analyze <rpm-run-directory>
out\cs-t0\IsaacCsT0.exe input-analyze <run-directory>
out\cs-t0\IsaacCsT0.exe input-compare --first <run-a> --second <run-b> [--output <new-json>]
out\cs-t0\IsaacCsT0.exe input-gate `
  --loop-gate <t0-03r-gate.json> `
  --matrix <input-matrix.json> `
  --repeatability <comparison.json> `
  --output <new-gate.json>
out\cs-t0\IsaacCsT0.exe input-gate `
  --mode-gate <t0-02-mode-gate.json> `
  --loop-gate <t0-03r-gate.json> `
  --raw-prereq <input-raw-prerequisite.json> `
  --active-plan <input-active-plan-v2.json> `
  --active-verification <create-new-active-verification-v2.json> `
  --matrix <active-matrix-v2.json> `
  --repeatability <active-comparison.json> `
  --output <new-active-gate.json>
```

`input-raw-prereq` 不连接或检查游戏，只读重放矩阵绑定的四份 passive raw/apply artifact，并拒绝覆盖已有输出。Keyboard 两份 source 保留其 v3 catalog/plan provenance，Controller 两份保留 v4 provenance；每个 source 的 manifest/events/analysis SHA 均由矩阵固定。派生 decision 即使为 `PASS` 也固定 `eligibleForActiveGate=false`、`activeInputAllowed=false`，只能作为后续 active controller 的前置输入。

`input-offline-attest` 是独立 passive-only 通道，不修改 T0-02 IPC，也不属于 active Helper。IPC/artifact v2 有两个显式策略：普通场景保持三个固定 IAT 与 `SteamNetworking006` Send/Available/Read 三个虚表槽，要求同 opaque game-root/Lua generation 的至少 60 个连续 simulation callback；从主菜单启动的 `loading` 额外临时拥有一个经 frozen bytes、IAT、owner、ABI 和精确返回 `0x65E68C` 验证的 `PeekMessageW` 槽，预绑定唯一可见根游戏窗口线程，并要求至少 60 个 `MENU_HEARTBEAT_PEEK`。菜单 prerequisite 明确记录 `LUA_UNBOUND_MENU_EPOCH`，不得用普通 `lua_pcallk` 或伪造 Lua digest 代替；Active runtime 只在 loading 后首次可信上下文中绑定真实 generation，绑定前禁止 synthetic command/apply/query。两种策略都必须先武装 Steam 三槽，且 `0x87169C` 的 manager identity 连续稳定、network backend 为零、roster header canonical empty、online frame 为零；任一非零、变化、畸形或不可读都 fail closed。停止时先恢复 Steam 三槽并 drain，再恢复策略所属的三个或四个 IAT 并 drain，逐槽复核与 read-only preflight 均通过后才 create-new prerequisite。Decision 有效期固定 5 分钟，不保存原始地址，且永久 `activeInputAllowed=false`。脱壳 target 只有在 create-new `IsaacCsT0-UnpackedTargetEquivalence-v1` 精确证明全部保留节、imports 与 45 槽等价后才可进入该 passive 通道。

`input-smoke --probe loopback` 强制 `--offline-prereq`、`--arm-grant` 和 runtime-complete `IsaacCsT0-ActiveVerification-v2`。顺序固定为 static bundle → read-only preflight → current session binding → grant validation → verification v2 → token CAS → create-new run/mappings → Helper。Grant/claim 以 token SHA 命名的 create-new 文件完成原子消费；并发调用只有一个成功，claim 创建后崩溃也不会让 token 复活。历史 verification v1 保持 runtime-blocked，不能进入 CAS；v2 即使 `runtimeContractComplete=true` 也固定 `activeInputAllowed=false`，不能替代 prerequisite、逐场景授权或正式 evidence。

`input-smoke --probe control-no-native` 使用同一冻结 static bundle，但只打开 read-only process observer。它创建 schema-4 `DISARMED_CONTROL` artifact，恰有一条 `control-observation`，并要求 45 个 target slot 的 before/after snapshot 完全相同；禁止 offline prerequisite、arm grant/verification/token、input script、IPC/mapping、Native 与 Helper。任何目标漂移、debugger/forbidden module、非零 hook mask 或伪造 active payload 都 fail closed。

`input-smoke --probe original-online-disarmed` 复用同一 approved-unpacked equivalence 与 read-only static bundle，不再回退到 canonical-only 人工检查点。它要求唯一、稳定且无 debugger/forbidden module 的游戏进程处于 active Original Online Run；连续两次 RPM 观察必须得到同一 game root、manager、Steam backend（含冻结 vtable）、2–4 人 distinct roster，且 online frame 不回退。manager/backend/roster/frame 布局已与原 IsaacSync 的 `runtime_target.hpp` / `runtime_roster.cpp` 及当前 Schema-4 Native 实现交叉核对，但不复用旧项目的 session、channel `0xFD` 或任何可变状态。artifact 只保存以单次随机 nonce 盐化的 identity 摘要、roster 数量和 frame 范围，不保存原始 peer identity 或地址；同时仍要求 45 槽逐字节不变，禁止 arm/token、IPC/mapping、Native、Helper、suppression、synthetic input 和 companion runtime。任何在线正向信号不足、roster/manager/process/module 漂移或错误 endpoint/observation binding 都 fail closed。

Schema-4 Active mapping 在 CAS 后写入真实 PID/creation，并逐项绑定 canonical target、approved-unpacked observed target、target-equivalence document、raw prerequisite、mode/loop gate、catalog/plan、repeatability script、arm token、session 和 client identity。Native 在安装任何 hook 前用 `GetProcessTimes` 重验 creation，精确比较全部冻结摘要，并只接受 `Profile::ApprovedUnpacked`；旧字段布局、零摘要、canonical/unpacked 混用或任何错位都会 fail closed。共享块仍固定为 288B event、`sessionId@576`、`events@720` 和 18,875,088B，loop mapping name 容量 96 UTF-16 单元且禁止截断。

`input-analyze` 与 `input-compare` 对 schema 4 执行 18 个专用谓词，并对 pipeline source 强制重放 hash-bound `loop-context`；disarmed/control source 禁止伪造 active IPC 或 companion runtime。pause、loading、room、menu 分别绑定 V5 的 bracketed steady gap、alternate transaction、room-change transaction 与 `RUN_TEARDOWN`。repeatability 只归一化 PID/QPC/绝对 tick/client/token 等非确定身份，stage、相对 tick、canonical、edge、decision、apply 与 witness relation 必须逐项相等。`input-gate` active 路径额外强制 runtime-complete verification v2，matrix template 固定 18+2 的顺序、唯一路径/artifact/token 和 source hash replay。截至 2026-08-27，正式 create-new matrix 已登记 primary `18/18`、extra `2/2`，repeatability comparison `1/1`；final decision 为 `1/1 PASS`，`t004Unlocked=true`、`activeInputAllowed=true`。正式包位于 `out/cs-t0/formal-active-gate-20260827-r1`，decision SHA-256 为 `927561D7DC28FE4DE05680BC68C6FA1841E3C77D76B30A710CE34031072E222C`。截至 T0-04 的逆向单一入口见 [T0-04 逆向交接](../docs/refactor/cs-t0/t0-04/isaac-cs-t0-04-reverse-handoff.md)，Schema-4 模块和停止边界见 [Schema-4 Native Runtime 说明](schema4-native-runtime.md)。

`loop-inspect` 和 `input-inspect` 只读磁盘 PE；当前均返回 `PASS_STATIC`。它们的局部只读结果按职责固定 `activeInputAllowed=false`，不能单独授权 active，也不否定已经通过的 schema-4 final gate。`loop-smoke --probe control-no-native` 仍是无注入人工停点；`loop-smoke --probe passive` 使用独立 `T03R` IPC v5 和 Native exports，要求游戏已由用户启动且 **x32dbg/x64dbg 已完全关闭、目标没有 debugger attached**。V5 被动记录 loop header、顶层 update/render、Win32 message pump、`InputManager → per-device update → concrete physical poll` 身份、steady standard/alternate Authority 候选事务、room-lifecycle FSM transaction，以及普通跨门时 `0x2FB51C` room-change owner/NewRoom/`0x2FE37C` nested `Room::Update` transaction；事件 63–66 固定为 `ROOM_CHANGE_OWNER_ENTER/EXIT` 与 `ROOM_CHANGE_SIMULATION_ENTER/EXIT`，phase 为 `ROOM_LIFECYCLE`。设置切换场景只接受两种已实机观察的精确同 loop 形状：`Peek → Translate → Dispatch → follow-up Peek → Keyboard direct refresh → Gamepad direct refresh` 的连续 12 事件，或在四个 message 事件后执行完整 manager/Keyboard/Gamepad sample、manager exit、第二次 Peek，再执行 Keyboard/Gamepad direct refresh 的连续 23 事件；两者均要求同一 main thread/update/loop 和一致 device identity。pause 则要求 passive loop/poll 持续、无 lifecycle 混入且 steady-standard transaction 缺口被前后完整 transaction 夹住。`+0x30/+0x34/+0x38` logical-query ABI 仍做静态 preflight，但不再 patch。它不读取具体键/button，不部署 Mod，不抑制或合成输入；停止时会验证 27 个 callsite、2 个 physical-poll vtable slot 和 3 个 IAT slot（共 32 个）的字节完整恢复。32 槽 mask 使用 64-bit storage，禁止 32-bit shift/sentinel 混淆。每次实机运行前仍必须停下等待用户确认。

`input-smoke --probe passive` 现在是独立的 observation-only input runtime。它必须显式提供可重放的派生 V5 `--loop-gate`，不接受 arm token；Controller 同时创建 schema-2 `ICSI04V1` input mapping 与 companion V5 mapping，并只执行一次 `--inject-input`。同一 DLL 在一次事务中安装 V5 的 32 槽和 11 个 input vtable 槽（总计 43 槽），只记录 action `0..11` 的 logical query、经完整 binding-table 双快照唯一分类的 manager primitive query，以及明确标为 `LOGICAL_OVERRIDE_DIAGNOSTIC` 的 setter 调用。wrapper 恰好调用 original 一次并原样返回，不修改参数、返回值、device、queue、world 或 `ModeLatch`；alias/UI-only/无匹配/非法或不稳定布局只增加 redaction 计数。正常停止按 input 11 槽恢复与 wrapper drain → V5 32 槽恢复与 drain 执行，任一恢复失败都锁存 `Unsafe` 并禁止卸载。每次 run 生成 `PASSIVE_INPUT_RUNTIME` artifact、嵌套 `loop-context`、双方 stream/hash 与 43 槽 before/after 恢复绑定；该证据类永久 `eligibleForActiveGate=false`，`input-gate` 会显式拒绝。keyboard、controller 与 rapid-tap input runtime 已在对应 unique-owner 的可操作 offline/local Run 中完成；后续任何实机运行仍是独立人工停点，运行前必须关闭/分离 debugger。

`input-raw-smoke` 是 additive schema-3 observation-only raw/apply runtime；schema-2 artifact 与分析语义保持不变。它在同一 DLL/单次注入中组合 V5 32 槽、legacy input 11 槽、Player owner 1 槽和六个自然 consumer call，共 50 槽。其冻结 v1 catalog 曾把 common-device-update 返回后的 `device+0xC0/+0xC4[0..11]` 假设为 action-indexed raw projection；canary-03 已以全零 raw、同时存在 logical true 与自然 witness change 的结果推翻该假设。后续静态确认 concrete Keyboard/Gamepad poll 不写这两组字段，logical query 直接查询 binding primitive，只有 logical setter 明确写 `+0xC0`；因此 v1 只能作为 `FROZEN_REJECTED_HYPOTHESIS` 保留，不能改名或冒充 raw evidence。

当前 active 文档为 create-new `raw-apply-catalog-v4.json`（SHA-256 `E22323047082E6289891393DE0B467CA5038E15EBD45310F39B5051FD0EB13D6`）与 `raw-apply-capture-plan-v4.json`（SHA-256 `203F64825EA24EE4EFA21B731FD2CB8E15AB11DA87145F7654C545B676A45562`）。v4 保留 v3 的 Native 采样、hook、IPC 与 action-indexed projection，不改变任何游戏观察边界；只冻结更严格且不歧义的 closure cardinality：同一 consumer transaction 若出现同 family/site 的多个候选 action，则整笔 transaction 跳过、不给任何 action 或 witness 计分；跨 family 或错误 site 是全局 FAIL；每个 required action 必须另有 exactly-one candidate transaction。只有 singleton transaction 的 sentinel/action witness 才可归属，且 raw/logical/consumer/witness 必须共享 sample identity。v1/v2/v3 catalog、plan 与 artifact 保持历史冻结；active v4 Controller 与 Native 对旧 identity/hash fail closed。

采样层仍只在 main-thread `InputQuery` 的 physical-poll original 返回后、return RVA `0x0061F30C` 重建 action `0..11`：Keyboard 双读 gameplay binding 与被引用的 349-byte current cache；Gamepad 双读 gameplay binding、button bits、32 个单向 magnitude、deadzone 与 29 项 mapping。RAW_VALUE 从明确 action slot 出发，不是 primitive→action 推断；action `12..31` alias 不读取、不持久化也不阻断，跨 gameplay alias 会脱敏所有相关 action。特殊 `0xCDCC/0xCDCD`、越界、负/非 finite axis、非法 deadzone 与不稳定快照继续 fail closed；manager classifier 仍对完整 action scope 严格唯一。输出类固定为 `PASSIVE_RAW_APPLY_RUNTIME`、永久 `eligibleForActiveGate=false`，没有 canonical、command、queue、resolve、synthetic apply 或 world-write 字段。

v4 自动门为 171/171 Rust tests、fmt、全目标 clippy、Win32 Debug/Release-LTCG 与两套 Native harness 全 PASS。Release Controller/Native/Helper SHA-256 分别为 `442064DE4D4CEAE30CF57BAE18D37A797B8061E544A775812167DABF9CBF753F`、`03751409B5BAD0D21717317030717E36B9EDEB0905E277C616CC07D5248FA954`、`717E3498C7B75FBB427BC62AAA70CF673A8BDE68ED020D6307D5E29BFD551655`。本轮没有启动游戏或 x32dbg；任何 v4 实机仍需新的单独确认。

首次 schema-3 `keyboard-held-release` canary `20260822T101325Z-t0-04-input-raw-passive-keyboard-held-release-t0-04-raw-apply-keyboard-held-release-canary-01` 永久保留为 `FAILED`：Rust field-scope allowlist 漏接 capture plan 已批准的 movement `PlayerVelocityF32x2`，在第一个 velocity `WorldWitnessBefore` fail closed；95,866 条已持久化 raw 记录和 126,444 条 companion V5 记录不改写。schema-3 18 槽与 V5 32 槽均逐字节恢复，overwrite/Native Failure 为零，游戏继续响应且无 T0 模块残留。修复只允许 movement site `0x00385BC2` 的 direction/velocity 两种 witness，错误 site、action `12+` 和 primitive 仍拒绝。本次 `observedRaw=0x1` 也固定了场景规则：`requiredRaw=0x3` 是安装契约；Keyboard 场景必含 `0x1`，Controller 场景必含 `0x2`，允许额外已知 source、拒绝未知位。修复后 167/167 Rust tests、fmt、全目标 clippy、Release/LTCG 与两套 Native harness PASS。`canary-01` 不改判，也不会自动重试；下一次仍需单独确认并创建 `canary-02`。

第二次 schema-3 `keyboard-held-release` canary `20260822T104912Z-t0-04-input-raw-passive-keyboard-held-release-t0-04-raw-apply-keyboard-held-release-canary-02` 也永久保留为 `FAILED`。它实机确认场景 raw mask 与 movement velocity 修复有效，但 raw ring 出现4次真实 overwrite并在用户动作前自行退出；85,242条raw、39,428条V5 event和50槽完整恢复均保留，动作闭环为0/8。定量根因是schema-3 writer直接对`File`执行serde碎片写：约87.9 MiB事件在18.013秒产生，落盘约31秒，阻塞下一次drain。修复使用显式4 MiB `BufWriter<File>`，finish/fail继续强制flush并传播错误；ring容量、Native overwrite阈值、cursor、catalog与plan不变。167/167 tests、fmt、clippy、Release/LTCG与两套Native harness再次PASS。不得自动重试；下一次为新的单独确认 `canary-03`。

第三次 schema-3 `keyboard-held-release` artifact `20260822T110518Z-t0-04-input-raw-passive-keyboard-held-release-t0-04-raw-apply-keyboard-held-release-canary-03` 在用户单独确认并完成 action `0..7` held/release 后安全结束：raw 703,040 条、companion V5 201,036 条、9,160 个完整 loop、overwrite=0、50 槽逐字节恢复，V5 analyzer 18/18 PASS。logical true 与 movement/shooting witness change 均存在，但 action `0..11` 的 109,920 条 `RAW_VALUE` 全为零，所以该 artifact 只证明 v1 raw-source 假设错误，不证明 raw held/release 或 offline/local closure。索引化 strict analyzer 用约 1.95 秒完成重放，六项结构/恢复/companion checks PASS，唯一 `passive-offline-local-closure` FAIL；raw analysis SHA-256 为 `9F24183D2FACEDCB621688E6AB852A7E25F1F929776ACA751BF0D96140229C81`。v2 自动门已完成：168/168 Rust、fmt、全目标 clippy、Win32 Debug/Release-LTCG 及两套 Native harness 全 PASS。该版本 Release Controller/Native/Helper SHA-256 为 `AB256929D1362271DD8E171A0973E8FA87C897F1075EC48A0FA8FE83CB0CE5E0`、`201A600CE74FE47A8A464A60E488E73A4680569F80AE897E72E3ABA5AA02C949`、`717E3498C7B75FBB427BC62AAA70CF673A8BDE68ED020D6307D5E29BFD551655`；随后仅在新的单独确认下执行 canary-04。

用户单独确认后的首个 v2 实机 artifact `20260822T121256Z-t0-04-input-raw-passive-keyboard-held-release-t0-04-raw-apply-keyboard-held-release-v2-canary-04` 已安全结束：raw 627,338 条、companion V5 225,849 条、10,555 个完整 loop，序列连续、overwrite/wrong-thread/Failure/error 为零，50 槽精确恢复且 V5 18/18 PASS。action `0..3` 各有 10,555 条 concrete `RAW_VALUE`、两次 rising、两次 falling、非零 held 样本及两条完整 raw→logical→consumer→witness proof；action `4..7` 虽有 logical pressed/scalar true 和 shooting witness change，却没有发布任何 `RAW_VALUE`，所以 strict analyzer 仍仅 `passive-offline-local-closure` FAIL。该结果首次实机证明 v2 movement raw path 正确，并把剩余缺口缩小到 shooting binding 的投影分类/脱敏；不是用户漏操作、poll 时机、consumer 或恢复失败。artifact 保留，不自动重试或启动下一场景。

canary-04 的同构建 binding correlation 固定了根因：shoot action `4..7` 的 primitive `0x107/0x106/0x109/0x108` 分别与 UI action `20..23` alias，edge action `8..11` 也具有 outside-action alias。v2 错把 manager 的全 `0..31` primitive→action 唯一分类器用于已经从明确 action slot 出发的 projection，因而清除了合法 gameplay valid bit。v3 只修正此层：projection owner 检查仅比较 gameplay action `0..11`，outside alias 不读取、不持久化也不阻断；gameplay↔gameplay alias 仍脱敏全部相关 action，manager classifier 继续对完整 `0..31` 严格唯一。v3 自动门为 168/168 Rust、fmt、全目标 clippy、Win32 Debug/Release-LTCG 与两套 Native harness 全 PASS；Release Controller/Native/Helper SHA-256 分别为 `63EEB616E43181C15ED7CAF272962652DD45A45050A2846EA315C2105136FD6D`、`79FC27358B7A11F0FE86BAEC211D43CF4DC9D0D1141AB7A8DAA7352B0BD6A725`、`717E3498C7B75FBB427BC62AAA70CF673A8BDE68ED020D6307D5E29BFD551655`。任何 v3 实机窗口仍需新的单独确认。

用户单独确认后的 v3 keyboard-held-release artifact `20260822T124555Z-t0-04-input-raw-passive-keyboard-held-release-t0-04-raw-apply-keyboard-held-release-v3-canary-05` 已取得 strict raw 7/7 PASS 与 companion V5 18/18 PASS：raw 635,300 条、loop 209,973 条、9,893 个完整 loop，序列连续、零 overwrite/wrong-thread/Failure/error，50 槽逐字节恢复。action `0..7` 每项均发布 9,893 条 `RAW_VALUE`，各有2 rising、2 falling、非零 held 样本、匹配 logical/consumer/witness 和2条完整 closure；v2 缺失的 shoot `4..7` 已全部闭合，同时 action `12+`、primitive 与主动管线字段仍为零。该 artifact 完成四场景中的 `keyboard-held-release`，但永久 `eligibleForActiveGate=false`；`keyboard-edge` 与两个 controller 场景仍需逐次单独确认，当前不自动启动。

用户单独确认后的 v3 keyboard-edge artifact `20260822T133033Z-t0-04-input-raw-passive-keyboard-edge-t0-04-raw-apply-keyboard-edge-v3-canary-06` 同样取得 strict raw 7/7 PASS：raw 1,744,176 条、loop 494,136 条、23,098 个完整 loop，序列连续、零 overwrite/wrong-thread/Failure/error，50 槽恢复且 action `12+`、primitive、主动字段为零。action `8/9/10/11` 分别完成 11/7/6/7 个 rising→falling cycle，同时包含分离触发与 rapid gaps，并闭合到对应 bomb/item/pill-card/drop consumer；bomb/drop witness 发生批准变化。Companion V5 stream 完整、结构/恢复/hash binding PASS，但其整体 analyzer 为预期 17/18，仅旧 V5 `scenario-evidence` 拒绝 rapid-tap 非七场景 scope；既有 V5 gate 不修改。action `0` 有一次32-sample incidental cycle，action `1..7` 为零；该事实已记录且不属于本场 required actions。该轮使被动 raw/apply 历史进度达到2/4。

用户单独确认的 v3 controller-held-release canary-07 `20260822T160527Z-t0-04-input-raw-passive-controller-held-release-t0-04-raw-apply-controller-held-release-v3-canary-07` 永久保持 FAIL，不用 v4 规则重写。其 action `0..7` 都有 non-zero raw、完整 rising/falling cycle 和 singleton transaction，但另有229笔同 family diagonal：movement `0+3` 45笔、shoot `5+6` 88笔、`5+7` 96笔；v3 对任何多 action transaction 全局 FAIL。v4 仅跳过同 family/site 的多候选 transaction，绝不让一份 witness 归给多个 action；only-diagonal、cross-family、wrong-site 或缺少 required singleton 仍 FAIL。canary-07 raw manifest/events/analysis SHA-256 分别为 `98993BB28FFFDFD73987B5B5F08616814ADFF39CF00EA1B976645AA68646F4D0`、`DB770338CB6138C7FF401A6C5F8B4A2EC31FC01107498CC8B7B3A36DD439529F`、`9F24183D2FACEDCB621688E6AB852A7E25F1F929776ACA751BF0D96140229C81`；loop manifest/events/analysis 为 `F69EEEF540ACD522E8FA127544FA83F0B5070E0048C21ED9613629668664D163`、`8A63E3A7076086BF45CC3860C56C7A7895198AA24702CDD8E5DBAFF883D23716`、`6EA2CFE61598E6A4795D7FECB8E554AECF18B3BBCAA8E3D18757282B292DC7D1`。下一次 v4 controller 实机仍需新的单独确认。

用户再次单独确认并由新的 controller0 unique-owner 前检闭合后，v4 controller-held-release canary-08 `20260822T165606Z-t0-04-input-raw-passive-controller-held-release-t0-04-raw-apply-controller-held-release-v4-canary-08` 取得 strict raw 7/7 与 companion V5 18/18 PASS：raw 470,556 条、loop 258,053 条、11,162 个完整 loop，零 gap/overwrite/wrong-thread/Failure/error，50 槽精确恢复且游戏继续响应。action `0..7` 均有2–4个完整 RF cycle、116–224个 singleton candidate 和1–3个严格可计分 proof；v4 只忽略同 family movement `0+3` 37笔与 shooting `5+7` 7笔，cross-family/wrong-site 为零。下方向 action3/action7 仍分别有116/203个 singleton 和1/2个严格 proof。raw manifest/events/analysis SHA-256 为 `03E6BD9FF9346CE791F52A5219D70210C07258FA342624854B99D393E4E763FD`、`29B576F8904B6613B305E151A337791E4D1416AAD65FA57884E4431B4A6B9B95`、`73CCECFF4C1C7435EECA91566F6654F1B3D8435D3A6958B0A4D5063EA6C20753`；loop manifest/events/analysis 为 `1AD3B1E6443573F79304638D6E88C1B5260E144B436568583C1BF691A1F2FE4D`、`1EF4DE57BBF2F19C03897AFD9A3942F729FE44608E6EC0BB62B22E557DF48AD2`、`275683A8A2317836F7E2900BDE8C3A5C466629126A7167588F08B74DBD0882CF`。被动 raw/apply 进度现为3/4；只剩 v4 controller-edge，仍需新的单独确认。所有 artifact 永久 `eligibleForActiveGate=false`。

用户单独确认后的 v4 controller-edge canary-09 `20260822T171644Z-t0-04-input-raw-passive-controller-edge-t0-04-raw-apply-controller-edge-v4-canary-09` 取得 strict raw 7/7 PASS：raw 534,858条、companion V5 275,545条、11,806个完整loop，序列连续、零overwrite/wrong-thread/Failure/error，50槽恢复且无模块残留。action `8/9/10/11` 分别完成7/6/7/7个RF cycle，每项均同时包含明显 separated gap 与9–19 loop rapid簇；singleton/proof为7/7、77/77、7/7、162/162，bomb/drop witness变化闭合，action9/10按冻结consumer-only规则闭合且不伪造world witness。multi/cross-family均为零。V5为预期17/18，仅冻结`scenario-evidence`拒绝该非七场景scope，其余17项PASS且不修改既有V5 gate。raw manifest/events/analysis SHA-256为`132C6E2E9AC4FE5E8C92B9F13D3E8C25A73B98492D54CA6E81651F568B64A9D7`、`CC61049B0CD90F39463F26EA2FF65263753419158E6F99B39F4D85AF5A397446`、`73CCECFF4C1C7435EECA91566F6654F1B3D8435D3A6958B0A4D5063EA6C20753`；V5 manifest/events/analysis为`490B3E7027C5D95BC78D39ED70F7383D62866292DE0E93AFD89FF99F92712BBB`、`FED45E85D5EAE0BFE0416F94A3FDC36F1DB4EC2C37F2D1182A01E0D34A130010`、`C0C29A74543100AC4E89976732F542375CBDB7DD5E792CD4496BE12A8AA4864D`。observation-only raw/apply四场景现为4/4 PASS，但全部永久`eligibleForActiveGate=false`；主动suppression、synthetic apply和loopback仍禁止。

若 Win32 build 在 Codex managed filesystem sandbox 内以 `GetLatestSDKTargetPlatformVersion` 访问 `C:\Users\114514\AppData\Local\Microsoft SDKs` 被拒绝，这属于 sandbox 对工作区外用户态 SDK 目录的只读限制，不是源码或 Windows SDK 损坏。保留 sandbox 内 Rust 验证，并仅在明确获批后于 sandbox 外运行相同的 `build.ps1`；不要通过放宽工作区 ACL、复制 SDK 或递归改权限规避。2026-08-22 的 Debug 与 Release/LTCG 复验均按此路径通过。

首次 controller canary `20260822T080816Z-t0-04-input-passive-controller-t0-04-passive-controller-canary-01` 在 `Ready` 前由 Helper `--inject-input` 以 exit 4 fail closed。该次 input/loop 事件与 hook mask 均为零，11+32 槽 before/after 保持一致，Native 未加载；失败 artifact 必须保留且不得覆盖或自动重试。Helper 的注入路径需要 `PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_QUERY_INFORMATION`；在受限执行上下文中无法取得精确权限时会在任何 hook 前退出。

用户重新单独确认后，controller canary `20260822T081545Z-t0-04-input-passive-controller-t0-04-passive-controller-canary-02` 在 sandbox 外以精确注入权限从头前检并完成：input 118,812 events、21/21 PASS，companion V5 170,955 events、18/18 PASS；11+32 槽精确恢复，mask 为 `0x7FF` / `0xFFFFFFFF`，无 gap、overwrite、wrong-thread、Failure、非零 error 或模块残留。它仍是永久 `eligibleForActiveGate=false` 的 observation-only 证据；`-01` 历史 FAIL 保持不变。

Keyboard unique-owner rapid-tap canary `20260822T083151Z-t0-04-input-passive-rapid-tap-t0-04-passive-rapid-tap-canary-01` 完成：input 122,240 events、21/21 PASS，action 8–11 均观察到 true edge/query；43 槽完整恢复，无 gap、overwrite、wrong-thread、Failure 或残留。companion V5 为 223,306 events，17/18 FAIL 仅缺 `scenario-evidence`，因为 rapid-tap 不属于冻结的 V5 七场景；其它 sequence/thread/lifecycle/restore/hash checks 均 PASS。不得把该预期 scope FAIL 改写成 V5 PASS；input artifact 仍永久 `eligibleForActiveGate=false`。

T0-03R loop gate 的必需场景固定为 `keyboard/controller/device-switch/pause/loading/room/menu`。`simultaneous` 与 `rapid-tap` 需要真实 logical/raw 输入状态，不能由 physical-poll 包络冒充，因此只保留在 T0-04 input gate；`focus` 按用户确认的“失焦会自动暂停”平台行为从两层必需矩阵移除，但 CLI 仍允许把它作为不解锁 gate 的诊断场景采集。

被动 witness 位于 `test-mod/isaac-cs-t0-input-witness`，必须由用户手动部署和启用。`input-import-witness` 只读解析用户提供的日志并创建 `WITNESS_QUERY_ONLY` artifact；可选 window/controller 过滤器会连同源日志 SHA 一起写入 manifest，避免把不同 Run 或 ownership 混成单一场景。该证据只证明 action `0..11` 查询可见性，不生成 raw/canonical/apply 事件，不能满足主动输入 gate 或主动 repeatability。loop/input gate 会从绑定的源 run 重放 analyzer 并核对 manifest/events/analysis SHA；编辑派生的 analysis/repeatability JSON 不能单独解锁 gate。`loop-analyze` 现为严格只读重放，不覆盖 artifact 内已有 `analysis.json`；采集侧分析报告一律 create-new。

`input-rpm-inspect`/`input-rpm-capture` 只以 `PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE` 打开固定 SHA 的目标进程，不暂停、不设断点、不执行远端函数、不写内存。它们对 input-manager、concrete vtable、cached logical magnitudes 和 player ownership 做双采；并把不稳定快照显式记为 drop。输出证据类固定为 `RPM_DIAGNOSTIC_ONLY`、`eligibleForActiveGate=false`：远端结构没有 generation/sequence，这条路径不能宣称 gapless、per-tick、canonical 或 apply 证据。loopback 另要求可重放的派生 V5 `PASS` gate（`--loop-gate`）、64 位十六进制一次性 arm token，以及第二次明确人工确认。CLI 会重新播放 gate 绑定的七份源 artifact，并核对所选目标和冻结 loop ledger；不会再把冻结 ledger 内按设计永久为 false 的历史字段误当成派生 gate。但在 raw/apply 被动证据缺失时，即使派生 gate 有效也仍会 fail closed。

退出码固定为：`0=PASS`、`1=证据/目标不匹配`、`2=参数或 artifact 无效`、`3=等待真实进程超时`。

## 当前门状态

T0-01/T0-02 保持正式 `PASS`。T0-03 的 scoped `PASS` 与 decision SHA-256 `3481AE6EC15930D993FBDC047D5A0E5B9E8EBE78BC4D4A8CCEE709F57CDE21DB` 保留，但不解释成完整 loop 或主动 input apply 证明。T0-03R V5 实机 loop gate 已为 7/7 `PASS`，gate SHA-256 为 `CDC0CE89A7393740A775E40FE6E588910038FEC68C40EB6D6F5CDF7BC1F1A1AA`；派生 gate 内的 `activeInputAllowed=true` 仅表示 loop 前置条件满足，冻结 ledger 不修改。

T0-04 observation-only passive raw/apply 的 `keyboard-held-release`、`keyboard-edge`、`controller-held-release` 和 `controller-edge` 已全部取得 strict PASS，当前为 4/4。四份 artifact 仍永久 `eligibleForActiveGate=false`，只作为 active prereq source；所有历史 FAIL artifact 保持原结论。Schema-4 正式 matrix 为 primary `18/18`、extra `2/2`，repeatability comparison `1/1`；20/20 根 analysis 重放均 `PASS` 且 gate-eligible。最终 schema-4 decision 为 `PASS`，`t004Unlocked=true`、`activeInputAllowed=true`。`original-online-disarmed live-46` 以零注入 read-only observation 和一致 replay 通过；`live-41 controller` 与 `live-42 device-switch` 均为 Active 21/21、companion 18/18 `PASS`，45/45 槽安全恢复且独立 replay analysis 一致。`live-39`、`live-40 controller` 原 artifact 保持 `FAILED`、不计入正式 matrix。

T0-05 当前仍为 `IN_PROGRESS`，但 `CS-T0-05-02 Three-mode C0 Comparison` 已正式 `PASS`。Offline single、Local co-op dual、OriginalOnline dual 三份 source 均为 90/90 coherent、53 槽不变和 no-mutation `PASS`；Local dual 直接证明 steady-standard pre-attempt frame proxy 单调正增量，post-return authority commit 仅通过 T0-03R/T0-04 冻结证据重放得到 `PASS_INHERITED_CONTRACT`，`directPostCommitObserved=false`。三模式 comparison SHA-256 为 `82A83246A5BB850313BAD6974EBCB7C6746AD22722FCD8DD8A04D84F38F028D8`，最终 C0 decision SHA-256 为 `46A5F34D5A8FBF1D7E239F1ECB2B572BDD3814B7A4A6AA3216EED57A159DE07D`。OriginalOnline 首份失败 source 与首次失败 decision 均永久保留、不覆盖、不改判。最终完整门为 278 个 Rust 单元测试、1 个跨语言测试、9 个 C0 contract、fmt/check/clippy、Release build，以及 Debug/Release 各 39/262 项两套 Native harness 全部通过。当前 [`coop-plan-v4.json`](coop-plan-v4.json) 已关闭 observer 为 `0x00000000` 且 `processReadAllowed=false`；只设置 `csT00503Unlocked=true`，`activeInputAllowed=false`、`passiveProbeAllowed=false`、`runtimeMutationAllowed=false`、`t005Unlocked=false`。机器入口见 [`coop-verification-v4.json`](coop-verification-v4.json)、[`coop-matrix-v2.json`](coop-matrix-v2.json) 与 [`ledger/isaac-cs-t0-coop-ledger-v3.json`](ledger/isaac-cs-t0-coop-ledger-v3.json)，执行细节见 [T0-05 状态](../docs/refactor/cs-t0/t0-05/00-overview/isaac-cs-t0-05-status.md)。

`CS-T0-05-03` 的生产实现和最新 pre-live 自动门已经完成，六场景正式进度现为 1/6 PASS。独立 `coop_passive` runtime 把 V5 32 槽、original-only 13 槽和八个 co-op wrapper 合并为唯一 53 槽事务；公开入口为 `coop-smoke --probe passive` 的六个精确场景、identity-dispatched `coop-analyze` 和精确六源 `coop-passive-gate`。live-01 在采集窗口前 fail closed；live-02 只有 manager pass；live-03 的手动加入发生在 RunTeardown 后的下一 Run，未落入采集窗口。live-04 首次捕获同一 manager transaction 内完整的 `46→49→51` ordinary join 链，但 8,177 个 ManagerEnd 全被旧的“backend 指针非空即 online”屏幕以 diagnostic code 3 拒绝，因此仍保持不可变的 `BLOCKED_BY_EVIDENCE`。实际本地主体 guard 只在 online roster 非空时跳过，而本轮完整本地链已证明 backend 指针存在本身不足以否定本地路径。

append-only [`coop-passive-backend-screen-v1.json`](coop-passive-backend-screen-v1.json) 将修复严格收窄为：精确 Steam backend vtable、online roster 三指针与 frame 全零、当前 generation 的同事务完整 `46→49→51` witness，且下一连续 pass 才能 Candidate→Bound；任一 partial/乱序/stale/intervening reject/possessor/exit/online 信号都撤销 witness。Join generation 只在 spawn 失效一次，同 route assignment 不再二次递增。旧 live-04 重放结论不改写。完整 v4 自动门通过 284 个 Rust 单元测试、9 个 C0 contract、10 个 passive contract、Debug/Release 各 39/262/407 个 legacy/Active/Passive Native case，以及两配置各 3 个跨语言 case、bootstrap 和 T0-04 回归；机器入口为 [`coop-verification-v5r4.json`](coop-verification-v5r4.json)。

live-05 因 capture 等待被中断而永久保持 `UNSAFE/BLOCKED_BY_EVIDENCE`；旧目标进程退出后才接受新授权，未对不确定实例自动 unload 或重试。全新进程上的 live-06 随后取得正式 8/8 `PASS`：21,621 条事件中 `46→49→51`、A/B Candidate、下一连续 pass 的 A/B Bound 与 Lua 等价全部在 generation 2 闭合；53 槽逆序恢复、snapshot、drain、producer close、显式 replay 与零模块残留均通过。

用户单独授权的 `native-exit-passive live-07` 已安全恢复并完成公开 CLI replay，但正式结果为 `BLOCKED_BY_EVIDENCE`：5,394 条事件中 A/B 始终绑定在 generation 1，冻结的 unbound/exit/exit-HUD/return-stable 链完全没有出现。53 槽逆序恢复、snapshot、drain、producer close 与零 overwrite 全部通过。只读 IDA MCP 证明 slot48 `0x005BB22A` 由 action 19 `JoinMultiplayer` 的 rising edge 驱动，但随后要求 main-vector candidate 的 `Variant(+0x2C) != 0`。ordinary true-coop constructor 明确以 `Type=1, Variant=0, Subtype=ePlayerType` 创建 B，因此它必然在该 guard 失败；possessor 虽是 Variant 2，却存入 secondary vector，同样不能进入 slot48。当前输入配置把 `JoinMultiplayer` 与 `Pause` 同时映射为 Start/Menu，退出 route 不查询 LB/RB。故这不是普通真合作的可操作退出路径，继续试按键不会改变结果。append-only 诊断为 [`coop-native-exit-reachability-v1.json`](coop-native-exit-reachability-v1.json)，授权已消费且不自动重试。

按用户指示，`native-exit-passive` 现只在工作流层标记为 `SKIPPED_NOT_APPLICABLE`；原 live-07 analysis 不改判、skip 不计入六源 gate。随后单独授权的 `death-respawn-passive live-08` 已取得正式 PASS：用户在活跃窗口内完成并确认 B 死亡后原生复活，20,130 条 serialized events 通过 inherited lifecycle 捕获 generation 1→28 的 A/B 失效与最终唯一重绑；slot46/48 均未命中，因此结论严格限定为 `PASS (operator-confirmed death/respawn, inherited-lifecycle provenance)`，不宣称直接观测到专用 death wrapper。53 槽逆序恢复、snapshot、drain、producer close、零 overwrite、零能力、零残留及公开 create-new replay 全部通过，analysis/replay SHA-256 同为 `B4113180004E9F765EE88F117D84D095909D230155DB3BF809A45AF828711C78`。

单独授权的 `room-rebind-passive live-09` 也已正式 PASS：用户只穿过一个普通房间门，20,396 条事件精确闭合 lifecycle enter→A/B unbound→lifecycle exit→首次完整 pass Candidate→下一连续 pass Bound，最终 generation 10 的 A→0/B→1 唯一映射、Lua 等价和稳定 registry 均正确；没有 death、device、menu、failure 或 possessor 事件替代。53 槽、安全关闭、零能力、隐私、零残留与公开 replay 全部通过，analysis/replay SHA-256 同为 `FABD4445747902C73B73E7907AC2B9F6A290807F6CAC4182795FC62711E7DACA`。create-new 进度见 [`coop-passive-live-progress-v3.json`](coop-passive-live-progress-v3.json)；当前真实 source 为 3/6 PASS，但 exit skip 仍使六源 gate 不可满足。下一项是尚未授权的 `device-rebind-passive`。自动门入口为 [`verify-coop-passive.ps1`](verify-coop-passive.ps1)，详细证据见 [T0-05 状态](../docs/refactor/cs-t0/t0-05/00-overview/isaac-cs-t0-05-status.md)。

继续剩余两项时，`device-rebind-passive` 已在实机前被阻断：独立公开 CLI 离线反例证明没有 assignment/lifecycle witness、甚至全部关系摘要不变时，仅两个自报 flag 就能得到 8/8 PASS；生产 manager-delta 分支也直接设置 assignment-match。只读逆向确认冻结 slot49 只覆盖新 join，实际既存 Player 的 Change Controller/断连重选走未冻结 callsite，尚无已证 lifecycle 因果关联。详见 [`coop-device-rebind-prelive-block-v1.json`](coop-device-rebind-prelive-block-v1.json) 与 [独立离线报告](../out/cs-t0/device-rebind-independent-audit-20260831-v1/audit-report.json)。尚未执行 live-10、未消费授权、未改生产/历史 source；游戏当前也未运行。必须先获准修订 device 证据链、修复判定并重跑自动门，不能用当前 PASS 假阳性推进。menu-teardown 未开始，仍需运行中的双人目标与单独授权。

### device-rebind v2 修订（2026-08-31）

用户已授权并完成设备取证修订，完整自动门已PASS，机器入口为 [verification v5r5](coop-verification-v5r5.json)（SHA-256 `12725A8E38CBDFA6348AF6F97DF3042C335A068623B5272DA513999C0CE5418F`）。新 [取证计划](../docs/refactor/cs-t0/t0-05/01-passive-lifecycle/device-rebind-v2-evidence-plan.md) 与 [机器契约](coop-device-rebind-contract-v2.json) 要求既存双BOUND → 原生unassign/readback → 同Player原生reassign/readback → 完整manager topology → 连续双采BOUND。菜单53→54→55与断连54→55是两条精确语法，允许同一设备重新接回；旧device v1及manager-only delta一律不能作为正向证据。

设备专用v2安装56槽并按55..0恢复；其他场景保持53槽，历史三份PASS source只读重放不变。新capture入口为 [catalog v4](coop-catalog-v4.json)、[plan v5r1](coop-plan-v5r1.json) 与上述verification v5r5。独立测试覆盖新351条Native路径、16阶段真实mapping、4条实际Native因果事件到公开CLI的桥接，以及因果/身份/恢复/权限/隐私mutation，均已在完整门中重跑。桥接的终端envelope是离线fixture，实际恢复顺序由独立patch矩阵验证，不冒充实机证据。

本轮没有启动或注入游戏，没有创建实机source，也没有消费新的场景授权。真实进度仍为3/6，exit skip不豁免六源gate；后续device与menu仍各需新授权，不自动重试。

### device-rebind v2 修订（2026-08-31）

用户已授权并完成设备取证修订，完整自动门已PASS，机器入口为 [verification v5r5](coop-verification-v5r5.json)（SHA-256 `12725A8E38CBDFA6348AF6F97DF3042C335A068623B5272DA513999C0CE5418F`）。新 [取证计划](../docs/refactor/cs-t0/t0-05/01-passive-lifecycle/device-rebind-v2-evidence-plan.md) 与 [机器契约](coop-device-rebind-contract-v2.json) 要求既存双BOUND → 原生unassign/readback → 同Player原生reassign/readback → 完整manager topology → 连续双采BOUND。菜单53→54→55与断连54→55是两条精确语法，允许同一设备重新接回；旧device v1及manager-only delta一律不能作为正向证据。

设备专用v2安装56槽并按55..0恢复；其他场景保持53槽，历史三份PASS source只读重放不变。新capture入口为 [catalog v4](coop-catalog-v4.json)、[plan v5r1](coop-plan-v5r1.json) 与上述verification v5r5。独立测试覆盖新351条Native路径、16阶段真实mapping、4条实际Native因果事件到公开CLI的桥接，以及因果/身份/恢复/权限/隐私mutation，均已在完整门中重跑。桥接的终端envelope是离线fixture，实际恢复顺序由独立patch矩阵验证，不冒充实机证据。

本轮没有启动或注入游戏，没有创建实机source，也没有消费新的场景授权。真实进度仍为3/6，exit skip不豁免六源gate；后续device与menu仍各需新授权，不自动重试。

Active loopback 的冻结输入、完整实机记录、provenance 规则与正式门禁摘要保存在历史归档的 [`input-active-loopback-next-steps.md`](archive/t0-04-history/input-active-loopback-next-steps.md)。当前结论以 [T0-04 逆向交接](../docs/refactor/cs-t0/t0-04/isaac-cs-t0-04-reverse-handoff.md) 为准。T0-04 已完成；后续阶段仍需遵守各自的独立授权和安全边界。

### device-rebind live-10（2026-08-31，BLOCKED_BY_EVIDENCE）

单次授权的 300 秒实机已完成：同窗口 ordinary join 后，B 手柄断连/重连的 slot54→55 原生因果链与 gen3 双采 BOUND 均已记录。56 槽全部逆序恢复，drain/close、零能力及无 Native 残留检查通过；但 shutdown 清理私有 binding 时推进到 gen4，两个终止事件因此与 gen3 的 manifest/已完成设备事务不一致，正式分析及独立 replay 均为 6/8、`BLOCKED_BY_EVIDENCE`。

详见 [进度 v4](coop-passive-live-progress-v4.json) 与 [状态记录](../docs/refactor/cs-t0/t0-05/00-overview/isaac-cs-t0-05-status.md)。真实进度仍为 3/6，原 source/analysis 保留；本次授权已消费，不重试、不启动 menu。本轮未改生产代码；下一步须获准修复 shutdown generation 一致性、补真实 shutdown 到公开 analyzer 的回归并重跑完整自动门，再申请新的单次 device 授权。

### device-v2 shutdown 修复（2026-08-31，自动门 PASS）

已按授权修复停止观察后的清理：私有 binding/route 身份在 admission 关闭及 drain 后清空，但最后观测 generation 不再推进；device-v2 任一 terminal Publish 失败保持 Unsafe，不再误报 Stopped。analyzer、真实 lifecycle invalidation、legacy 53 槽、IPC、catalog v4 和 plan v5r1 均未改动。

独立测试新增 24 种真实 shutdown 边界，以及直接使用生产终止事件的 Native→公开 CLI 桥接（四条完整路线 PASS、两条未完成路线 BLOCKED）；Debug/Release 和全部继承回归通过。完整自动门为 **22/22 PASS**，新入口 [verification v5r6](coop-verification-v5r6.json)，SHA-256 `61DB9BCCE41988AFC1D2723B16012E363B0B2F39064762599B6BF11105827B2B`。此前 fixture terminal envelope 桥接已由真实 Shutdown/Event/Publish 回归替换，离线测试不冒充实机 source。

新 [进度 v5](coop-passive-live-progress-v5.json) 保留旧 live-10 的 BLOCKED 结论和全部历史证据；正式进度仍为 3/6。本轮未实机，device 重测仍需新的单次授权和全新 source/output，menu 也未授权或开始，不自动重试、不解锁 active。修复范围与详细验收见 [机器约束](coop-device-rebind-shutdown-repair-v1.json) 和 [状态记录](../docs/refactor/cs-t0/t0-05/00-overview/isaac-cs-t0-05-status.md)。

### device-rebind live-11（2026-08-31，正式 PASS）

新单次授权的 300 秒重测已通过：26,184 条事件完整记录 B 普通加入后 slot54 断连解除分配、slot55 同手柄重连及连续双采；最终双 BOUND、restored/closed、manifest 和 analysis 均为 generation 3。56 槽逆序恢复、逐槽 proof/snapshot、drain/close、零能力/隐私、无模块及 mapping 残留全部通过。独立公开 CLI replay 为 8/8 PASS，与原 analysis 逐字节一致，SHA-256 `E726A3339B539712510BFE9E0676B3AE005496FCBB9F0BB304438DEB5BB8C3F0`。

正式进度更新为 [progress v6](coop-passive-live-progress-v6.json) 的 **4/6 PASS**；旧 live-10 仍永久 BLOCKED，不覆盖、不改判。本次授权已消费，未自动重试或开始 menu。下一项 `menu-teardown-passive` 仍需独立单次授权；native-exit 的 skip 不计入六源 gate，不解锁 active。详细 [source](../artifacts/cs-t0/20260831T065706Z-t0-05-03-device-rebind-passive-t0-05-03-device-rebind-passive-live-11) 与 [replay](../artifacts/cs-t0/20260831T065706Z-t0-05-03-device-rebind-passive-live-11-replay-01.json) 保持只读。

### menu-teardown live-12（2026-08-31，正式 PASS；六源 gate 仍阻断）

新单次授权的 180 秒测试已通过：13,573 条事件直接记录本窗口普通加入后的 gen2 双 BOUND、冻结 run teardown、gen3 A/B 双 RETIRED 及完整返回；退休后无 binding 复活，restored/closed 与最终 manifest generation 一致。菜单专用原 53 槽逆序恢复、逐槽 proof/snapshot、drain/close、零能力/隐私及无模块/mapping 残留均通过。独立公开 CLI 一次 create-new replay 为 8/8 PASS，与原 analysis 逐字节一致，SHA-256 `05FC7430983B76B9919CA5F52EE6F497B110BC343D059E3D7C43320B9CDB24AA`。用户的菜单 UI 口头确认尚未收到，未标记为 true；正式 PASS 基于冻结 Native source 验收。

当前 [progress v7](coop-passive-live-progress-v7.json) 为 **5/6 PASS**，五个未跳过场景全部完成；`native-exit-passive` 缺少 PASS，工作流 skip 不豁免六源 gate，因此 `CS-T0-05-03` 仍 `BLOCKED_BY_EVIDENCE`，不解锁下一阶段。本次授权已消费，未重试或开始新场景。详细 [source](../artifacts/cs-t0/20260831T072042Z-t0-05-03-menu-teardown-passive-t0-05-03-menu-teardown-passive-live-12)、[replay](../artifacts/cs-t0/20260831T072042Z-t0-05-03-menu-teardown-passive-live-12-replay-01.json) 和全部历史证据保持只读。

### 原生退出适用性文档修订（2026-08-31）

按用户要求修订 [研究计划](../docs/refactor/cs-t0/t0-05/00-overview/isaac-cs-t0-05-research-plan.md) 与 [适用性说明](../docs/refactor/cs-t0/t0-05/01-passive-lifecycle/native-exit-applicability-revision-v1.md)：已证 native-exit caller 的 main-vector `Variant != 0` guard 不接受普通真合作的 Variant=0 B。本结论仅限固定目标、冻结 caller 与构造路径，不声称所有原生退出均不存在，不要求重复尝试该不可达入口。

新文档验收规格是五个适用 passive source PASS＋一项 hash-bound 不适用决议，而非六次实测 PASS；完整 T0-05 矩阵保留 20 项编号，改为 19 个适用 primary＋1 项不适用决议，条件场景和 repeatability 不减项。`NOT_APPLICABLE_NATIVE_ROUTE` 目前只是文档标签。旧 `coop-passive-gate`、schema、progress v7 和所有 source 仍严格不变，live-07 仍 BLOCKED，机器 gate 仍为 5/6；后续需另行授权机器迁移与完整独立自动验收。

自定义 Endpoint 逻辑退出与普通 B 的物理移除属于后续独立能力，须各自证明输入停止、generation、引用清理、A/HUD/Lua 一致性、失败安全与持久副作用边界；不能把 menu 全 Run 退休或现有删除函数 ABI 当作实现凭据。本轮仅改文档，未改生产代码、测试、verifier、JSON 或二进制，未重跑自动门、未实机、未解锁任何能力。

### 适用性 gate 迁移完成（2026-08-31）

按后续授权实现独立 `coop-passive-applicability-gate`：无新版 verification 时只输出 `PASS_EVIDENCE_ONLY`，完整验证后才开放 05-04 工作包资格。旧六源 gate 不接受 skip/N/A，live-07 仍 BLOCKED；普通 Variant0 路径的 N/A 由精确目标、29 静态锚点、五份 PASS 与历史 exit 哈希限定，不声称整个游戏无其他退出能力。

完整 [自动验证 v5r7](coop-verification-v5r7.json) 为 28/28 PASS；[正式 decision v3](coop-passive-decision-v3.json)、matrix v3、ledger v4、hypothesis v3、verification v6、plan v6 和 progress v8 均 create-new。独立 [审计 v2](../out/cs-t0/applicability-final-independent-audit-v2.json) 为 15/15 PASS，唯一 formal replay 除时间戳外逐字节一致，149 文件基线保持不变。先前 SDK、脚本拼参及审计工具自身假设失败记录均保留，未修改历史判定。

矩阵当前完成 9/19 个适用条目（含 1 个继承 PASS_COMPOSITE），另有 1 项限定 N/A；active、virtual 条件与 repeatability 仍待后续工作包。当前只设置 `csT00504Unlocked=true`，不授权 active 实机、输入/世界修改、自定义退出或 T0-06。

