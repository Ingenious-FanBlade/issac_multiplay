# T0-07：Authority tick/input scheduler

**状态：PASS（2026-10-20，用户确认）。** 通过依据是操作1–8的生产状态链/故障矩阵统一验收记录，以及操作9在同一实际目标的真实游戏中完成一次 24/24 PASS E2E；后者直接观察了 Authority tick 持续、A 物理动作、B 源停发/effective 缺口、中性/恢复、edge exactly-once、120-tick timeout 和 70 槽恢复。该 PASS 保留实际边界：B 仍是脚本源，不宣称真实网络断线/ACK；world record 仅有 partial Player 字段证据；不把历史离线PASS改写成未运行的实测项。独立的状态依据与限制表见[STATUS.md](STATUS.md)。

tx-01-98（2026-10-20）：操作9真实 Authority 的 B 输入停发实验已完成。最终有效 invocation 使用批准 unpacked 目标、显式 v5 配置和一次 New Run；生产源停发 1/5/30/120 Authority tick，effective 窗口按 lead=2 对齐，B 中性/恢复/edge/timeout、A 实际动作、partial world Player 证据和 70 槽恢复通过，统一报告 **24/24 PASS**、controller exit0。前三次失败（错误 GameExe、A 动作晚到、A 穿门死亡）原样保留；脚本停发不冒充真实网络断线，本轮无 complete world snapshot。见[结果](tx-01-98-results.md)、[设计](tx-01-98-source-stop-e2e.md)和[静态审阅](tx-01-98-static-review.md)。

tx-01-97（2026-10-20）：操作8已完成生产故障与边界矩阵。A/B conflict统一策略、source sequence极值、queue/history、future/expired/late/edge、timeout、server tick exhaustion、identity/encoding/代际、阶段/返回/发布和collector gap均走生产Session/Clock/Pipeline/Participant；最终 **27场景、183命令、184/184检查PASS**。历史失败原样保留；真实游戏/Hook/采样内存/事件环/网络未运行；操作9随后由tx-01-98完成。见[结果](tx-01-97-results.md)、[覆盖矩阵](tx-01-97-fault-matrix.md)和[静态审阅](tx-01-97-static-review.md)。

tx-01-96（2026-10-20）：操作7已完成CLI/manifest到Native的输入配置贯通。默认2/64/0/64/258，支持lead/future/script/admission显式范围与IPC v5，Native在Hook/Active前冻结并实际用于Participant、Clock、Pipeline、target和script drain；EventKind36记录生效配置。最终统一离线验收 **20配置case、9场景、83命令、104/104检查PASS**，历史编译/场景失败原样保留。真实游戏/Hook/采样内存/事件环/网络未运行。见[结果](tx-01-96-results.md)、[实现](tx-01-96-runtime-config.md)和[静态审阅](tx-01-96-static-review.md)。

tx-01-95（2026-10-20）：操作6已完成生产实现与一次整体离线验收。missing/effective输入、max-age=0中性、独立有效到达、119/120/121 Authority tick静默阈值、诊断性timeout、恢复、失绑/rebind/终态和EventKind35/query8-9已接入真实运行状态链；最终 **10场景、146命令、122条批量记录、152/152检查PASS**。首次FAIL与编译失败原样保留；真实游戏/Hook/采样内存/事件环/网络未运行。见[结果与复现](tx-01-95-results.md)、[实现与失败清单](tx-01-95-missing-input-timeout-recovery.md)和[静态审阅](tx-01-95-static-review.md)。

tx-01-94（2026-10-20，文档更新）：操作6–9的工作范围、已有基础、源码分析需求、配置接线、故障矩阵、实机停发口径和产物已写入[具体工作计划](tx-01-94-remaining-operations-plan.md)。本次只改文档，不代表操作6–9实现或验收；最新实现仍为tx-01-93。

- [操作6：缺失输入、超时与恢复](tx-01-94-remaining-operations-plan.md#2-操作6缺失输入超时与恢复)
- [操作7：CLI/manifest参数贯通](tx-01-94-remaining-operations-plan.md#3-操作7climanifest到native的参数注入)
- [操作8：完整故障与边界覆盖](tx-01-94-remaining-operations-plan.md#4-操作8完整故障与边界覆盖)
- [操作9：真实Authority停发实验](tx-01-94-remaining-operations-plan.md#5-操作9真实authority的b输入停发实验)
- [分析分工与唯一验收安排](tx-01-94-remaining-operations-plan.md#6-分析分工与推进安排)

执行要求以[现行项目规范](../../project-conventions.md)为准。范围来自[任务书第 10 节](../isaac-cs-t0-task-plan.md#10-t0-07authority-tickinput-scheduler-实验)。用户按操作推进：tx-01-89为操作1，tx-01-90为操作2，tx-01-91为操作3，tx-01-92为操作4，tx-01-93为操作5，tx-01-95为操作6，tx-01-96为操作7，tx-01-97为操作8，tx-01-98为操作9。

- [操作9设计、失败方式与统一入口](tx-01-98-source-stop-e2e.md)
- [操作9静态审阅与修正](tx-01-98-static-review.md)
- [操作9实际结果与历史失败](tx-01-98-results.md)

- [操作8故障覆盖矩阵](tx-01-97-fault-matrix.md)
- [操作8静态审阅](tx-01-97-static-review.md)
- [操作8实际结果与复现](tx-01-97-results.md)

- [操作7实现、schema与失败方式](tx-01-96-runtime-config.md)
- [操作7静态审阅与修正](tx-01-96-static-review.md)
- [操作7实际结果与复现](tx-01-96-results.md)

- [操作6实现前范围与失败方式](tx-01-95-missing-input-timeout-recovery.md)
- [操作6静态审阅与首次失败修正](tx-01-95-static-review.md)
- [操作6实际结果与复现](tx-01-95-results.md)

tx-01-93（2026-10-20）：操作5已完成生产pipeline与Native接入：有界drain/共同准入、双Resolve、原生返回后关闭可见性、Outer post-state复制或缺口、tick提交后发布值与本地回执。GPT-6 Luna / max结论经主智能体逐项审阅；Native最终纯编译成功，唯一统一离线验收 **36场景、254/254检查PASS**，保存120条连续生命周期记录。真实游戏/Hook/采样内存/网络未运行，操作6–9未实施；以下较早范围说明保留历史时点。

- [操作5失败清单、生产接入与回执格式](tx-01-93-tick-pipeline.md)
- [Luna结论与主智能体审阅](tx-01-93-static-review.md)
- [操作5实际结果、限制与复现](tx-01-93-results.md)

tx-01-92（2026-10-20）：操作4已完成：A/B共用生产AcceptInput，typed本地命令转入同一字节解码/validator/去重/队列，底层Submit收为私有。GPT-6 Luna / max结论已由主智能体逐项审阅；Native最终纯编译成功，唯一统一离线验收 **40场景、303/303检查PASS**，保存4846条原始准入记录。B当前仍是脚本源，真实游戏/Hook/网络未运行，操作5–9未实施。下文操作1–3的范围说明保留历史时点。

- [操作4实现前失败清单与最终接口](tx-01-92-input-admission.md)
- [Luna结论与主智能体审阅](tx-01-92-static-review.md)
- [操作4实际结果与复现](tx-01-92-results.md)

tx-01-91（2026-10-20）：操作3的逐client序号/未来窗口、有效连续状态、edge来源账本、接收/接受/解析/处理进度和拒绝计数已实现，并接入Native退休诊断。按用户要求使用GPT-6 Luna / max静态分析，主智能体已逐项审阅；纯编译成功，唯一离线验收 **22场景、152/152检查PASS**，另保存4357条批量输入记录。真实游戏/Hook/世界动作未运行，操作4–9未实施。

- [操作3实现及实现前失败清单](tx-01-91-client-input-state.md)
- [Luna结论与主智能体逐项审阅](tx-01-91-static-review.md)
- [操作3实际结果与复现](tx-01-91-results.md)

tx-01-90（2026-10-20）：操作2的生产AuthorityTickClock与Native接入已实现，只在配对的标准Room/odd helper正常返回后增加completed；未知来源/重复/缺失返回停用，世界采集完整性独立。GPT-6 Luna / max静态结论已由主智能体逐项复核。纯编译成功；唯一离线验收 **30场景、175/175检查PASS**，真实游戏/Hook/Join/恢复未运行。

- [操作2实现与失败清单](tx-01-90-authority-tick.md)
- [子智能体结论及主智能体审阅](tx-01-90-static-review.md)
- [操作2实际结果与复现](tx-01-90-results.md)

tx-01-89：操作 1 的 AuthoritySession 与 Native 接入已实现，Native/离线场景程序纯编译成功；唯一统一离线验收 **33 场景、303/303 检查 PASS**。已登记实现前失败方式、先写场景再实现；真实 Hook/Join/恢复未运行，操作 2–9 不在本次范围。

- [完整范围与失败方式](tx-01-89-authority-session.md)
- [实际结果、原始证据及复现入口](tx-01-89-results.md)
- [GPT-6 Luna / max 静态分析及接入结论](tx-01-89-static-analysis.md)
- [生产状态机](../../../../cs-t0/native/include/isaac_cs_t0/authority_session.hpp)
- [纯编译入口](../../../../cs-t0/build-authority-session.ps1)
- [唯一离线验收入口](../../../../cs-t0/accept-authority-session.py)

操作6–9现均已有对应实现/验收记录：6为tx-01-95、7为tx-01-96、8为tx-01-97、9为tx-01-98；用户据此于2026-10-20确认 **T0-07整体PASS**。上文tx-01-89/90/91/92的范围说明保留其历史语境；操作9的实机结论只覆盖其声明的 run/配置/场景，不把历史离线结果改写成实机证据，也不把本PASS扩大到真实网络断线或所有角色/生命周期组合。旧任务书的hash、独立单元测试、多次测试和gate/解锁文字不作为现行要求；沿用结构化记录，不进入原版Online/Isaac Sync。
