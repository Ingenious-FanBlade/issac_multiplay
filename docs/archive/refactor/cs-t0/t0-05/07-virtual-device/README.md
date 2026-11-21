# Virtual Device 当前设计与静态结论

tx-01-62：仅审阅既存报告。Virtual Device完整43项（原42+选角1）均有历史实机PASS，无从未覆盖条目；61单轮完整33 PASS/10 NOT_OBSERVED、专项32/32 PASS仍保留，不等同当前版本单轮全套通过。逐条来源和历史失败说明见[覆盖核对](tx-01-62-e2e-completion-review.md)。

tx-01-61：B=1选角与Pill/Card资源消费、held10一次专项32/32 PASS。药丸5→0，三次held10 companion纯读一致；两端输入、B退休/A继续和51Hook恢复/DLL常驻通过，493,550事件/10,374tick、零原生或传输错误。进程可见性失败的tx-01-60原件保留，本轮仅一次实机注入；完整报告NOT_OBSERVED保持，见[本轮结果与边界](tx-01-61-live-process-visibility.md)。

tx-01-59：角色选择的报告链路修复后一次35/35 PASS，文本化stderr、逐项JSONL和最终JSON可读；复用tx-01-58成功构建的三项产物，保留58报告序列化失败。未操作游戏，角色实机效果仍待后续场景，见[报告修复与结果](tx-01-59-reporting-repair.md)。

tx-01-58：新增 B 加入时角色选择，A 沿用游戏开局选择。控制器 `--b-character <名称|ID|native>`，现行实机入口 `-BCharacter`；native核对当前可选列表、只改自有K的一次普通工厂参数，并记录实际subtype。CoopActive IPC升v3，需同轮控制器/native。最后两项实机测试尚未开始；见[角色选择说明](tx-01-58-character-selection.md)。

tx-01-56：异常Join的原生Baby分支已修复，一次专项21/21 PASS。实际三处Baby=true仅对自有K选择普通路径，两pass内单次普通工厂/完整Type1-Variant0 B，物理设备分支和A健康保持；用户确认表现正常。435,907事件、7,777 tick、零原生/传输错误，51 Hook恢复/DLL常驻。tx-01-51旧来源仍为静态推断，完整T0-05未全部完成，见[本轮实现与结果](tx-01-56-abnormal-join.md)。

tx-01-55：真实Continue的Loading／LoadingExit与加载后A/B同一查询重复读取，一次专项20/20 PASS；102组/306次纯读保持值及整个Participant状态，48 Hook恢复/DLL常驻。受控重复不冒充引擎自然重复，完整T0-05未全部完成。见[本轮结果](tx-01-55-loading-repeated-query.md)。

tx-01-54：跨房非模拟窗口暂停原生 Shooting 已一次专项 E2E 18/18 PASS；用户确认持续按住往返跨房不误发、真实释放/再次蓄力与暂停恢复正常。A/B 各75次暂停，正常 tick 缺输入仍消费中性，48 Hook恢复/DLL常驻。完整 T0-05 E2E 未全部完成。见[本轮结果](tx-01-54-room-shooting-suspension.md)。

tx-01-53：B 原始射击脚本移动 pair 重复死区修复已一次实机验收8/8 PASS，101个 raw 与4个 canonical步骤移动量保留、两原生查询路径与48 Hook恢复/DLL常驻通过。171,783事件、3,178 tick，未重复52人工蓄力测试或重跑。跨房自动发射和51未接纳 Join 原因仍未解决。见[本轮结果](tx-01-53-script-movement-preservation.md)。

tx-01-52：后按相反射击优先、释放回退和未知同采样兜底已一次实机确认，36项方向专项 PASS、用户画面正常，48 Hook 恢复/DLL 常驻。新增 B raw 脚本移动量重复死区另记补充整体 FAIL；不覆盖原专项或自动重跑。跨房自动发射与 tx-01-51 未接纳 Join 原因保持未解决。见[本轮结果](tx-01-52-opposite-shooting-priority.md)。

tx-01-51：统一启动例外一次验收确认 A 首包/后继衔接和 45 次 room 后启动，48 槽正常恢复、DLL 常驻。本轮 B 未经创建 owner 接纳，自动结果 NOT_OBSERVED；用户确认跨房未松键仍自动发射，补充最终结果 FAIL。相邻重叠换向保留蓄力，相反方向仍丢蓄力。见[启动输入修复及结果](tx-01-51-startup-input-repair.md)。

tx-01-50：已补齐原生奇数帧漏掉的 Authority 窗口；一次最终 E2E 已确认 B 死亡/复活、新输入恢复和旧 generation 拒绝，48 槽恢复、DLL 常驻。专项整体 FAIL（19 PASS、1 FAIL），用户补充换房及相邻方向切换仍丢蓄力，输入连续性尚未完成；详见[来源、修复和结果](tx-01-50-input-window-repair.md)。以下各轮事实保留。

更新时间：2026-10-16；最新技术交付：tx-01-43 / v18；文档整理：tx-01-44。

先读[现行项目规范](../../../project-conventions.md)。本页集中当前结论，旧批次与 tx 时间线见[归档索引](../archive/07-virtual-device/README.md)。旧 decision、哈希、预封、独立角色和解锁字段只表示当时记录。

tx-01-49 最新观察：死亡操作前发现 A（阿萨泻勒/键盘）输入异常；同 tick 有效更新后，窗口外的 Movement/Shooting 原生调用被强制中性，实际覆盖输入派生字段。B 死亡/复活未执行，专项 NOT_OBSERVED（3 PASS、8 未观察项）；正常恢复 47 槽，root/DLL 常驻。该输入缺陷尚未修复，额外调用完整 owner 待核对；用户确认 TimeMachine 未启用。见[专项记录及异常证据](tx-01-49-b-death-respawn-e2e.md)。

tx-01-48 结果：修复把 Lua engine 存在误判为 callback 13 接管的条件，增加拓扑/绑定/注册/Join 阶段诊断。一次统一验收已确认 A/B 绑定、单次注册、原生 Join、精确身份匹配、暂停/换房重绑、B 冲突退休后 A 继续及菜单 teardown/47 槽恢复；DLL 常驻。731,674 事件、11,868 tick，35 PASS、7 NOT_OBSERVED。**完整 E2E 仍为 NOT_OBSERVED**，缺 Pill/Card 部分行为、loading、死亡/复活和重复查询证据。见[本轮修复与结果](tx-01-48-binding-join-repair.md)。T0-05 未完成。

tx-01-47 历史结果：Publish 改为 const Event&，新机器码与一次实机确认初始化崩溃消除；读取 120,032 条事件、推进 3,428 tick，正常 shutdown 及 47 槽恢复完成。该轮完整 E2E NOT_OBSERVED，A/B 绑定、注册与 Join 未确认。见[事件发布修复及结果](tx-01-47-event-publication-repair.md)。

tx-01-46：固定同一套 VS 2022/v143 和独立构建目录后，Native/helper 已链接成功；授权实机在远程初始化发生 0xC0000005，游戏退出，整体 FAIL。见[该轮链接修复及结果](tx-01-46-link-repair.md)。

tx-01-45：已按用户采纳的 Authority writer、常驻 root 和立即中性策略新增生产源码、独立 coop-active 控制器与[统一 E2E 入口说明](tx-01-45-implementation.md)。该轮验收 FAIL：Native 链接报告 LNK1101 / MSPDB140.DLL 版本不正确，实机 NOT_RUN。见[历史验收记录](tx-01-45-results.md)。下面 v18 的“未实现/未采纳”是此前静态交付时的状态；本次采纳、代码及限制以该实现说明和现行规范为准，不改写历史结论。

## 当前阅读入口

| 资料 | 用途与边界 |
| --- | --- |
| [已采纳设计修订 v1](design-revision/virtual-device-owned-abi-design-revision-v1.md) | D1–D6 的采纳背景；具体功能冲突以契约 v2 和现行规范为准 |
| [协调器契约 v2](design-revision/virtual-device-owned-abi-coordinator-contract-v2.md) | 自有对象/control、38 槽/六覆写/六 control 槽、NEVER、线程与恢复约定 |
| [最新静态报告 v18](design-revision/virtual-device-owned-abi-static-confirmation-v18.md) | 限定正常路径结论、Join 三态、CurrentEligible/PreviousComparable、四个 UI 地址及剩余事项 |
| [v18 IDA/source 证据](../../../../../../out/cs-t0/virtual-device-owned-abi-static-evidence-v18.md) | 本轮直接读取、原始指令、条件、对象、参数与手工轨迹 |
| [原生 Join 用户规划假设](../02-active-join/native-join-active-user-planning-assumption-v1.md) | 可靠创建并发布 B 的 DESIGN_ONLY 前提 |

## 已采纳的功能设计

自有对象 vtable 与 control block；S2 action 层覆写；路线 A 用虚拟设备触发原生 Join，路线 B 直接重绑作为备选。BASE_CONSTRUCTED 后对象、control 和有效 base 状态 NEVER 释放；base ctor 前 pin DLL，恢复后常驻。退休时输出中性，移出 InputManager vector，B 解绑或退出；每 session 至多一次设备创建，失败不自动重试。

A/B 固定 ordinal 0/1，ClientId 映射 session 内冻结；指针/K 只用于当前 generation。两端共用 canonical、sequence、target tick、queue、dedupe、Resolve/apply 语义，各 participant 状态独立；不等待全员输入。C/S 隔离原版/Isaac Sync，不依赖 Online Manager、NetInputDevice、FrameStore 或全员 ready。

## v18 的结论与限定

在有效对象和缓冲、主线程顺序、Offline/local-co-op、Online roster 为空、B 唯一正 K、创建 B 前只有 A 等前提内，原生静态主干基本足够。完整 Init/global manager-this、全 alias/observer、全部 Mod、worker/EH 与完整 05-06 网络闭包不属于该报告的证明范围。

- Join 必须区分 NOT_CALLED、CALLED_FALSE 与 CALLED_TRUE。同 K 已有 selector 且尚无对应 Main Player 时，action19 的 admission/removal 查询可以均不调用；配置 19=0 不能写成实际返回 false。
- CurrentEligible 约束所有受控输出。PreviousComparable 只约束 0–7 level-derived Triggered；当前 tag/target 有效且已 Resolve 的 8–11 显式 edge 不因 previousValid=false 被整体抑制。invalid/stale current 始终中性。
- UI 地址区分：95B867 为 context reset call，95B87F 为 selected Player setter，95BBB6 为 selector 字段清理，95BBCB 为 context 与 detected K 的关联调用。
- vector 对 B 的责任恰消费一次；其他原生引用 R 可变，总 strong 不能写成无条件恒减一。coordinator 不二次 ReleaseClass，不进入旧 FinalRelease。

## 尚未采纳、实现或观察

| 类别 | 未完成事项 |
| --- | --- |
| 契约选择 | Authority-pre/current、normal-post/previous 候选与契约 v2 §4 的 +0x78 sole-writer 冲突；候选仍 DESIGN_ONLY / 未采纳，本次整理没有修改契约 |
| 生产实现 | A-only V1 尚未扩为双 participant；V2 held、自有 ABI、JoinControl、B attempt owner 和 retire coordinator 未生产实现 |
| 现场观察 | K、selector/context/pair/listener、mode、实际绑定/发布及 pause/loading/retire 顺序等未由 v18 观察 |
| 证据范围 | Join 可靠创建/发布沿用户假设；持久化、完整异常/并发和全部 Mod 不能由正常路径静态资料外推 |
| 整体功能 | 实际双端输入隔离、完整生命周期和恢复结果尚未完成 |

## 必要继承依据

v18 是定点修正报告，不是所有前版事实的全集。按问题查阅，避免逐版本执行旧工作设计：

- [v4 ABI / 缓冲 / 生命周期基础](../archive/07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-v4.md)：读取具体结论时同时查看后继勘误。
- [v16 引用账本、Join scope、A query 与双 participant 候选](../archive/07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-v16.md)。
- [v17 跨 pass、stored-confirm、外部 R 和时序修正](../archive/07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-v17.md)：其 selector 与 previous 摘要已由 v18 更正。
- [v18 已完成执行设计](../archive/07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-next-work-design-v18.md)：历史“待执行”对应当时设计状态，实际结果看 v18。
- [旧 lifecycle 契约 v1](../archive/07-virtual-device/virtual-device-lifecycle-coordinator-contract-v1.md)：仅与 v2 和现行规范不冲突的功能约定继续适用。

其余报告、工作设计、失败、retry 与 alias 批次保存在[完整历史时间线](../archive/07-virtual-device/README.md)。native alias universe 已为背景资料，Batch 2 retry 暂停；旧全映像闭包、factory/release-thread 安排不再控制推进。旧离线原型的释放语义也不能直接当作 NEVER 的实现。
