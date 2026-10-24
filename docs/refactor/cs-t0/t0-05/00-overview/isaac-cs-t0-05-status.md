# T0-05 当前执行状态

tx-01-62：Virtual Device既定E2E清单累计43/43有历史实机PASS（原42项+选角1项），已无从未覆盖的用例。最新61完整单轮仍33 PASS/10 NOT_OBSERVED，专项32/32 PASS；未给当前版本补写全场景单轮PASS，不宣布整个T0-05架构任务完成。本次只审阅记录，见[逐项覆盖与限定](../07-virtual-device/tx-01-62-e2e-completion-review.md)。

tx-01-61：B=1选角及此前Pill/Card消费、held10两项未观察已一次专项32/32 PASS；药丸资源5→0，held10来自真实consumer上下文的companion纯读（不是引擎主动Pressed10）。B冲突退休/A继续、菜单teardown、51Hook恢复与DLL常驻通过。493,550事件、10,374tick，零原生/传输错误；原完整报告NOT_OBSERVED保留，未重做其他专项。见[本轮实机结果](../07-virtual-device/tx-01-61-live-process-visibility.md)。tx-01-60受限终端进程不可见导致注入前FAIL已保留，61修复入口PID/创建时间绑定及执行环境后仅一次实际注入。

tx-01-59：B角色选择的CLI与报告链路一次35/35 PASS，复用58成功构建的控制器/Native/Helper，逐项证据与最终JSON已保存。58最终报告内存不足的FAIL保留；本轮未重构建或操作游戏。见[报告修复与结果](../07-virtual-device/tx-01-59-reporting-repair.md)。

tx-01-58：新增 B 加入时角色选择，A沿用游戏选择。控制器`--b-character`与现行实机入口`-BCharacter`传递冻结的角色ID；native核对原生可选列表并记录实际subtype，保持两pass、单次普通工厂与51Hook事务。最后两项实机测试尚未开始，角色实机结果未观察；见[实现与本次验收](../07-virtual-device/tx-01-58-character-selection.md)。

tx-01-56：异常Join的原生Baby分支已修复，一次专项21/21 PASS。实际三处Baby=true仅对自有K选择普通路径，两pass内单次普通工厂/完整Type1-Variant0 B，物理设备分支和A健康保持；用户确认表现正常。435,907事件、7,777 tick、零原生/传输错误，51 Hook恢复/DLL常驻。tx-01-51旧来源仍为静态推断，完整T0-05未全部完成，见[本轮实现与结果](../07-virtual-device/tx-01-56-abnormal-join.md)。

tx-01-55：Loading／LoadingExit及同一查询重复读取已一次专项20/20 PASS；实际Continue加载事务1次，加载/恢复前中性，恢复后A/B新鲜输入；三种接口共102组/306次受控实际Read，值与完整Participant状态不变。260,528事件、4,645 tick、48 Hook恢复/DLL常驻，零原生/传输错误。完整T0-05其余用例不外推，见[本轮结果](../07-virtual-device/tx-01-55-loading-repeated-query.md)。

tx-01-54：持续按住攻击跨房自动发射已修复，一次专项 E2E 18/18 PASS，用户确认往返跨房、真实释放、再次蓄力和暂停恢复均正常。A/B 各75次非模拟 Shooting 暂停，正常 tick 缺输入中性不变；1,074,896事件、19,027 tick、48 Hook恢复/DLL常驻，原生/传输错误为零。完整 T0-05 E2E 未全部完成，见[本轮结果](../07-virtual-device/tx-01-54-room-shooting-suspension.md)。

tx-01-53：B 原始射击脚本移动量重复死区修复已一次实机8/8 PASS。101/101 raw与4/4 canonical步骤移动 pair 原样消费，两原生查询路径、普通 Join/身份、48 Hook恢复/DLL常驻通过；171,783事件、3,178 tick，原生/传输错误为零，不重跑。未重复52人工蓄力操作。跨房自动发射和51未接纳 Join 原因仍未解决，见[本轮结果](../07-virtual-device/tx-01-53-script-movement-preservation.md)。

tx-01-52：相反射击后按方向优先已一次实机验证，36项方向专项及用户画面均 PASS，48 Hook 恢复/DLL 常驻。另发现新增 B raw 脚本移动8192变4681，补充整体 FAIL；原报告保留、不重跑。跨房自动发射与51未接纳 Join 原因仍未解决，见[本轮结果](../07-virtual-device/tx-01-52-opposite-shooting-priority.md)。

tx-01-51：统一启动例外已一次实机验证 A 首包/nominal 配对与 45 次 room 后启动；本轮 B 未经创建 owner 接纳，自动结果 NOT_OBSERVED。用户确认跨房未松键仍自动发射，补充最终结果 FAIL；相邻重叠换向保留蓄力，相反方向仍丢蓄力。48 槽恢复/DLL 常驻，未自动重跑，见[本轮记录](../07-virtual-device/tx-01-51-startup-input-repair.md)。

tx-01-50：已补齐原生奇数帧 955241→6FD3F0 的 Authority 窗口；一次最终 E2E 确认 B 死亡/复活、新输入恢复、旧 generation 拒绝及 48 槽恢复/DLL 常驻。专项整体 FAIL（19 PASS、1 FAIL），换房及相邻方向切换仍有用户观察到的丢蓄力，未自动重跑。见[本轮记录](../07-virtual-device/tx-01-50-input-window-repair.md)。

更新时间：2026-10-16；文档整理：tx-01-44；最新静态技术交付：tx-01-43 / v18。

执行要求以[现行项目规范](../../../project-conventions.md)为准。本页区分历史结果、已采纳设计、限定静态结论、未采纳候选、未实现和未观察项；不维护旧解锁链。

tx-01-49 最新观察：B 死亡/复活专项中，用户在死亡操作前报告 A（阿萨泻勒/键盘）移动慢、疑似无法蓄力；事件确认同 tick 有效 consumer 更新后，窗口外 consumer 又被强制中性并清除输入派生字段。暂缓死亡操作，专项 NOT_OBSERVED，正常 teardown/47 槽恢复及 DLL 常驻完成；输入缺陷尚未修复，额外调用完整 owner 待核对。用户确认未启用 TimeMachine，不将其归因为模组。见[本轮记录](../07-virtual-device/tx-01-49-b-death-respawn-e2e.md)。

tx-01-48 结果：修复 ReadTopology 对 Lua engine 存在的误拒绝，增加阶段/身份诊断。一次统一验收已观察到 A/B 绑定、设备单次注册、原生 Join 与精确身份匹配；B 冲突退休后 A 继续，菜单 teardown、47 槽恢复及 DLL 常驻正常。731,674 事件、11,868 tick、35 PASS；完整 E2E 为 NOT_OBSERVED，Pill/Card 部分行为、loading、死亡/复活和重复查询共 7 项未完整观察。见[本轮修复与结果](../07-virtual-device/tx-01-48-binding-join-repair.md)。

tx-01-47 历史结果：修复 Publish 按值调用与生成指针 callee 的不一致，改用 const Event&。一次实机窗口初始化/正常 shutdown 成功，读取 120,032 事件、Authority 3,428 tick、47 槽字节恢复完成。该轮完整 E2E 为 NOT_OBSERVED，绑定/注册/Join 未观察到；该轮事实见[记录](../07-virtual-device/tx-01-47-event-publication-repair.md)。

tx-01-46：已修复统一入口的工具链选择和旧构建产物复用，Rust、Native/helper 全部构建成功，LNK1101 已消除。授权实机在远程初始化发生 0xC0000005，Windows 记录该轮 DLL+0x372D 崩溃，游戏退出；整体 FAIL。见[历史记录](../07-virtual-device/tx-01-46-link-repair.md)。

tx-01-45 新进展：用户已采纳 Authority-pre/current、normal-post/previous、root 常驻和缺输入立即中性；独立生产源码、co-op active controller/helper、47 槽 Hook 接线及 E2E 入口已新增。用户授权后执行了唯一一次统一验收，**FAIL：Rust 构建成功，Native 链接报告 LNK1101 / MSPDB140.DLL 版本不正确，实机 NOT_RUN**；未自动重跑。见[实现与操作清单](../07-virtual-device/tx-01-45-implementation.md)及[验收记录](../07-virtual-device/tx-01-45-results.md)。下文“未采纳/未生产实现”表保留 tx-01-44 整理时的事实，不代表本次源码已被验证。

## 当前结论

T0-05 整体未完成。当前路线为自有 ABI 虚拟设备、S2、原生 Join 路线 A 与 NEVER，具体见[Virtual Device 主题](../07-virtual-device/README.md)。

[v18 报告](../07-virtual-device/design-revision/virtual-device-owned-abi-static-confirmation-v18.md)说明：在其有效对象/缓冲、主线程顺序、Offline/local-co-op、Online roster 为空、B 唯一正 K、创建前仅 A 等前提内，限定正常路径的原生静态主干基本足够。该结论不等于实现已完成、现场值已观察或全部异常/并发/持久化问题已解决。

| 维度 | 当前状态 |
| --- | --- |
| 已采纳设计 | 自有对象 vtable + control block；S2；路线 A 为主、B 为备选；NEVER；构造前 pin DLL，恢复后常驻 |
| 最新静态结论 | v18 核清 Join 的 NOT_CALLED / CALLED_FALSE / CALLED_TRUE、显式 edge 与 previous 条件、四个 UI 地址；仍继承前版 ABI、引用账本和 consumer/action 依据 |
| 未采纳候选 | Authority-pre/current、normal-post/previous 与契约 v2 §4 的 +0x78 sole-writer 冲突；整理未作采纳 |
| 生产实现 | 源码仍为 A-only V1；自有 ABI、JoinControl、双 participant S2、V2 held、B attempt owner 和 retire coordinator 未生产实现 |
| 现场观察 | K、selector/context/pair/listener、mode 等实际值和行为未由 v18 观察 |
| 规划假设 | 原生 Join 可靠创建并发布 B 是用户接受的 DESIGN_ONLY 前提 |
| 完整功能结果 | 双端隔离、完整 lifecycle、恢复与后续整体运行验收尚未完成 |

当前源码的 ReadUniquePlayer 仍要求 Main Player vector 只有一名玩家并固定 ordinal 0，见 [active_runtime.cpp](../../../../../cs-t0/native/src/active_runtime.cpp)。旧离线 lifecycle 原型只在专用测试工程中编译；其 FinalRelease/RemoveAndDestroy 路径与 NEVER 不一致，不能当作当前生产协调器。整理没有修改这些代码。

## 已有阶段结果与剩余问题

| 历史内容索引 | 已有结果 | 仍需区分的边界 |
| --- | --- | --- |
| 05-00 / 01 / 02 | bootstrap、local-co-op 静态图、三模式 C0 历史结果为 PASS | 不外推为主动 Join、虚拟设备或可玩 C/S 完成 |
| 05-03 | 五份适用 PASS source 与一项限域 N/A；旧 5/6 和 live-07 失败保留 | 重绑、generation 和 teardown 的具体语法仍有参考价值 |
| 05-04 | 原生 Join 路径与局部 guard/receiver 公式已有静态资料；创建/发布采用规划假设 | manager-this、完整 Init/P0、持久化窗口、reader/ABA 的原始 UNKNOWN 不改判 |
| 05-05 | 必要性判断、自有 ABI 采纳和 v18 限定正常路径资料已具备 | 契约选择、接线、实现、现场状态和最终运行结果分别未完成 |
| 05-06 / 07 | 双端隔离与最终整体功能结果尚未完成 | 按实际目标与依赖推进，不依据旧资格或解锁字段安排工作 |

[manager identity 摘要](../06-manager-identity/README.md)保留 strict remaining 99、人工策略队列 96 及其证据/假设区别。这些是历史研究状态，不是当前待清零的推进门。旧 native alias universe 已降为背景，Batch 2 retry 暂停。

## 历史与本次整理

旧版[执行状态长日志](../archive/00-overview/isaac-cs-t0-05-status.md)、[主题总索引快照](../archive/00-overview/t0-05-index-before-tx-01-44.md)和[Virtual Device 时间线](../archive/07-virtual-device/README.md)保留原结果、失败与勘误。新入口不把旧材料中的“当前”“下一步”当作今天的任务。

tx-01-44 只整理文档、归档与导航，没有新增逆向事实、采纳 writer 候选、修改代码或机器产物。整理范围和唯一整体静态审阅记录见[归档说明](../archive/README.md)。
