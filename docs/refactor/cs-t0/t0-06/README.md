# T0-06 实体研究

当前结果（2026-10-19，tx-01-88）：用户永久移除 W01.isolation，W08 全量 replay 性能修复并验收 **PASS**。2.3 GB 输入两次耗时 14.50/13.58 秒，33,050 条记录/20,931,447 字段值和重复输出字节一致；当前清单累计 **32/32 PASS**（30 项沿用历史、2 项 W08 本轮检查），无剩余测试项。没有重跑实机场景，旧 FAIL 和 33 项报告保留；见[完整结果与复现入口](tx-01-88-e2e-results.md)和[修改范围](tx-01-88-replay-performance.md)。下文 33 项、两项排除为旧运行事实。

当前结果（2026-10-19）：tx-01-87 剩余授权 E2E **6/6 PASS**；累计 **31 PASS / 0 FAIL / 2 NOT_COVERED**，25 项明确沿用历史出处。已修正保存数据的汇总，并新采真实主菜单 Continue 加载和 run 退出，939 个普通样本阶段匹配、70 槽恢复。本轮不重发故障；tx-01-83/84/86 FAIL 保留。真实 Online/Isaac Sync 隔离与全量 replay 继续按用户选择排除，因此全局仍为 INCOMPLETE；当前授权范围已完成。见[完整结果、边界和复现入口](tx-01-87-e2e-results.md)。下文“最新/当前”均保留对应历史时点。

最新专项结果（2026-10-19）：tx-01-85 **2/2 PASS**，完成 retained Player／Replace 与 GridFree 触发取证。三次 Replace 沿用 tx-01-84 原始采集，修正参数副本双计及输入解码；新进程只补采 D12 对自建岩石的实际 GridFree、退休事件和槽成员退出，70 槽全部恢复。当前 Restore 映射 UNKNOWN 继续保留，旧失败/总账不改写，完整 T0-06 仍未验收完成。见[专项结果与复现步骤](tx-01-85-e2e-results.md)。

最新运行（2026-10-19）：tx-01-84 两场景专项 **FAIL，0 PASS / 2 FAIL**；实际获得三次成功 Replace 与 retained 身份/成员变化，GridFree 在 D12 前被 fixture 的 variant=0 假设拒绝。汇总另有参数双计和 ActiveItem 位错误，原失败不改判。采集健康、70 槽全部恢复，见[专项结果](tx-01-84-e2e-results.md)。tx-01-85 将实质修正岩石基线和汇总，只补采 GridFree 并读取已有 Replace 证据，见[修改范围](tx-01-85-gridfree-correction.md)。

最新运行（2026-10-19）：tx-01-83 一次宿主统一 E2E **整体 FAIL、报告完成**，账本 **25 PASS / 1 FAIL / 7 NOT_COVERED**；十个剩余目标新增 4 PASS，5 项仍未覆盖、W12 故障隔离 FAIL。八段全新进程采集均完成 70 槽恢复；primary 19,924 个普通样本阶段匹配，读取撤销/恢复和只读/RNG 观察通过，精确 frame 6000 比较得到限定范围 DIFFERENT。object-budget/reference-ambiguity/event-overflow 原观察窗口超时保留，迟到证据不改判；Replace/GridFree、retained Player、短命/重复发布和完整窗口配对仍有缺口。全量 replay 与真实 Online/Sync 继续排除，未重跑。见[本轮结果与剩余项](tx-01-83-e2e-results.md)。

准备阶段历史（2026-10-19）：tx-01-83 补齐安装失败原始阶段/Win32 错误、退出后的事件读取和异常结束的部分证据汇总；当时仅控制器纯编译成功、E2E 未运行。Native/Helper/协议不变；tx-01-82 原始 FAIL 与根因 UNKNOWN 保留。详见[修改与构建记录](tx-01-83-diagnostics-and-finalization.md)。

最新运行（2026-10-19）：tx-01-82 宿主统一 E2E **整体 FAIL、报告完成**。已保存双方 Game frame 6000 精确对齐比较（DIFFERENT）与 field-read/source-changed 两个受控故障的 scheduler 继续、70 槽恢复证据；event-overflow 初始化时 Hook 事务失败，尚未发故障命令，wrong-thread/root-changed 未进入。primary 场景按用户选择未覆盖；最终汇总因异常未到达，机器账本冻结为 21 PASS / 12 NOT_COVERED，新部分证据单独说明，见[本轮完整结果](tx-01-82-e2e-results.md)。未重跑、未改运行时代码；T0-06 未完整验收。

最新运行（2026-10-19）：tx-01-79 结果 **INCOMPLETE：21 PASS / 0 FAIL / 12 NOT_COVERED**；本轮补齐新跟班创建/Init、八类字段及独立失联恢复，另重新检查正常 70 槽。消费暂停期间 scheduler 继续推进、背压明确增加，W12 为部分证据；正常/失联均确认 70 原字节恢复。18 PASS 历史沿用，3 项本轮实际检查，两个注入前观察器 FAIL 保留。排除全量 replay 后尚余 11 项，见[完整结果](tx-01-79-e2e-results.md)。下文较早轮次的“最新/当前”均保留对应历史语境。

执行要求以[现行规范](../../project-conventions.md)为准。阶段范围见[任务书第 9 节](../isaac-cs-t0-task-plan.md#9-t0-06实体枚举临时身份和-canonical-state-hash)。

最新运行（2026-10-19）：tx-01-76 按用户要求续跑剩余项，只跳过全量 replay，单条离线用例和限定跨进程比较新增 9 项 PASS。累计 **19 PASS / 0 FAIL / 14 NOT_COVERED**，其中 10 项明确沿用上轮；补充 Projectile 可读，Familiar 因缺出生/Init 依据保留 NOT_READY，70 槽恢复通过。用户选择保留未覆盖项收尾，见[本轮结果](tx-01-76-e2e-results.md)和[续跑入口](tx-01-76-remaining-e2e.md)。完整 T0-06 仍未验收完成。

历史运行（2026-10-18）：tx-01-75 已完成三段实机及 primary 正式扫描，连续完整数 **13,567**、背压 **0**、消费管线及正常恢复子项通过。用户因离线全量 replay 耗时明确要求结束本轮，最终 **INTERRUPTED**（10 PASS / 0 FAIL / 23 NOT_COVERED），未自动重跑；完整 T0-06 验收仍未完成。见[本轮最终记录](tx-01-75-e2e-results.md)。以下 tx-01-75 未执行状态、tx-01-74 FAIL 按对应交付/运行时点保留。

tx-01-75（2026-10-18）：已修改[缓冲消费路径](tx-01-75-immediate-consumer.md)，使用通知驱动读取和独立编码写入，复制后即释放共享槽；用户确认 256 MiB / 256 项自有队列与满时背压行为。world 内部 mapping v2 与新旧二进制混用限制见说明。本轮纯编译，尚未运行 E2E；以下 tx-01-74 的实际失败记录保持有效。

2026-10-18 当前运行状态：tx-01-74 已按统一入口执行一次 E2E，整体 **FAIL**（17 子项通过、1 失败、14 未覆盖）。W07 连续完整采集只有 2/10,000，背压 18,868 次；51/70 槽正常恢复及 W08/W09 通过。T0-06 仍未完整验收，未自动重跑。见[本轮结果](tx-01-74-e2e-results.md)和[原始报告](../../../../artifacts/cs-t0/tx-01-74-e2e-01/result.json)。下文“运行未执行”均为对应实现/静态交付时的历史事实。

tx-01-74（2026-10-18）：已实现[统一 E2E 执行器](tx-01-74-unified-executor.md)，采用用户确认的人工检查点；[入口脚本](../../../../cs-t0/accept-world-record-e2e.ps1)将各段实机采集、离线生产 CLI 与 W01–W13 报告组织为一次调用。**执行器未运行，清单仍全部 NOT_RUN**；特殊场景和实际失联恢复证据不足时保留未覆盖，不自动补造或重跑。

2026-10-18 当前状态：**tx-01-73 的实现已完成，运行验收未执行**。已有可选 `coop-active --world-record min-read-v1`、19 个新增入口与统一 70 槽事务、只读采集/身份/JSONL、`world-replay` 和 `world-compare`；没有运行游戏、注入或测试。见[实现及使用说明](tx-01-73-implementation.md)和[实现后 E2E 清单](tx-01-73-e2e-checklist.md)。用户另采纳 exact-pair 绑定跨正常 PlayerUpdate 的当前字段读取规则，FIRST_OBSERVED 与缺失出生历史仍保留。

以下为 tx-01-71 及以前的静态进展：操作 1 的入口定位、操作 2 的静态类别映射已完成；操作 3 按本会话约定的最小原样只读静态范围完成字段登记与会话偏移核对。HP、位置/速度与阶段、state/timer、seed/归属由 GPT6 Luna / max 分析、主智能体复核；最后的偏移汇总及剩余分类/flags 由主智能体完成。当前没有运行时 capture。

- [tx-01-71 参考吸收与剩余静态分析补全](tx-01-71-static-completion.md)：已吸收用户指定的旧REPENTOGON参考，使用GPT6 Luna / max进行IDA静态分析，并由主智能体复核更正和补全。任务书43项已形成原生依据、身份/接入/读取/比较设计和未知处理；见[参考吸收](tx-01-71-reference-integration.md)、[生命周期及原生生产链](tx-01-71-lifecycle-and-native-records.md)、[采样与比较规格](tx-01-71-sampling-design.md)和[一次整体静态审阅](tx-01-71-review.md)。该轮尚未实现或运行采集、身份、编码与比较；代码现已由 tx-01-73 实现。

- [tx-01-70 剩余静态分析交接任务书](tx-01-70-static-analysis-handoff.md)：将身份生命周期、采样接入、读取规格、记录比较设计和原生记录生产链整理为 43 个问题，供用户发送给另一会话。仅完成任务整理，没有新增逆向结论；见[任务书整体静态审阅](tx-01-70-review.md)。

- [tx-01-69 会话偏移逐项核对](tx-01-69-offset-reconciliation.md)：汇总根/容器、Entity、玩家/池、逐类状态及 Grid/虚槽，列出 8 处旧数值更正，补齐分类、Index、flags 的当前依据。
- [本轮当前 IDA 证据](../../../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json)、[字段补充](../../../../artifacts/cs-t0/tx-01-69-static/field-evidence.json)和[参考会话快照](../../../../artifacts/cs-t0/tx-01-69-static/reference-session.json)独立保留；见[本轮整体静态审阅](tx-01-69-review.md)。

- [tx-01-68 seed/归属最小只读登记](tx-01-68-seed-owner.md)：逐类 seed 与引用字段的偏移、宽度、存储形式和必要初始化线索；不穷尽 Morph/Restore/复用历史，不建立跨 tick 身份。
- [seed/归属原始证据](../../../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json)、[主智能体复核](../../../../artifacts/cs-t0/tx-01-68-static/primary-evidence.json)及[直接 Grid.Desc 绑定补充](../../../../artifacts/cs-t0/tx-01-68-static/grid-evidence.json)分别保留；见[本轮整体静态审阅](tx-01-68-review.md)。

- [tx-01-67 state/timer 最小只读登记](tx-01-67-state-timer.md)：逐类适用性、偏移、类型/宽度与最小初始化线索；不恢复完整 AI 状态机、所有计时单位或全部重置路径。
- [state/timer 原始证据](../../../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json)、[整数桥接补充](../../../../artifacts/cs-t0/tx-01-67-static/supplemental-evidence.json)、[主智能体复核](../../../../artifacts/cs-t0/tx-01-67-static/primary-evidence.json)及[绑定和 Grid 初始化补充](../../../../artifacts/cs-t0/tx-01-67-static/binding-evidence.json)分别保留；见[本轮整体静态审阅](tx-01-67-review.md)。

- [tx-01-66 只读采样所需的静态依据](tx-01-66-readonly-sampling.md)：位置/速度布局、普通完整更新后的统一采样候选、奇数帧 Player 路径及生命周期处理；不要求穷尽全部移动、碰撞或传送 writer。
- [本轮原始证据](../../../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json)、[主智能体复核证据](../../../../artifacts/cs-t0/tx-01-66-static/primary-evidence.json)及[控制流补充](../../../../artifacts/cs-t0/tx-01-66-static/control-flow-evidence.json)保留原始材料与更正；见[整体静态审阅](tx-01-66-review.md)。

- [tx-01-65 HP 研究](tx-01-65-hp-analysis.md)：通用生命字段、玩家心量布局及关键初始化和变更路径。各 Entity 派生类别均用通用字段表示生命是本次用户采纳的假设；GridEntity 按独立布局记录。
- [HP 原始证据](../../../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json)及[主智能体补充证据](../../../../artifacts/cs-t0/tx-01-65-static/primary-evidence.json)：保留子智能体导出，另记指令复核、更正和实际扣血路径；见[本轮整体静态审阅](tx-01-65-review.md)。

- [tx-01-64 第二步类别覆盖](tx-01-64-category-coverage.md)：八类枚举来源、当前 RTTI、玩家补充关系、查询例外、门/Grid 别名，以及新会话偏移表的当前修正。
- [第二步原始证据](../../../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json)：39 个函数、8 段调用点及 32 张 RTTI/虚表记录；另保留[参考会话快照](../../../../artifacts/cs-t0/tx-01-64-static/reference-session.json)和[导出脚本](export_tx_01_64.py)。

- [tx-01-63 静态逆向交接](tx-01-63-entity-lifecycle.md)：吸收指定本地会话，使用当前 IDA 数据库补全本体路径，并区分旧版 REPENTOGON 与当前目标。
- [IDA 原始证据](../../../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json)：40 个函数的伪代码、原始指令、RVA、交叉引用和 5 段调用点指令。
- [可重复导出脚本](export_tx_01_63.py)：仅静态读取 IDA；不访问游戏进程，不修改 IDB，不计算或校验哈希。

操作 1 的入口定位、操作 2 的静态覆盖规格和本会话收窄后的操作 3 最小只读静态字段依据已登记，最后的会话偏移核对缺口已补齐。操作 4–9实际依赖的剩余静态依据和实现前方案见tx-01-71；未穷尽专用字段全集或所有writer，截至 tx-01-71 尚未实现或运行实体 capture、身份与比较；tx-01-73 已完成代码，仍没有运行时采集或比较结果。snapshot apply 不在本阶段目标内。静态结论和编译成功不代表运行时遍历安全或整个 T0-06 已验收。
