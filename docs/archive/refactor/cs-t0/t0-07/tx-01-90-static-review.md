# tx-01-90：Luna 静态结论与主智能体复核

2026-10-20（Asia/Shanghai）。按用户要求，GPT-6 Luna / max 子智能体 `/root/t007_tick_static` 只读分析既有源码、静态交接及新增时钟。主智能体另行读取下列源文件和 tx-01-66 原始证据，再逐项采纳或补充。双方均未在分析中执行测试、构建、游戏/调试器操作或哈希校验。没有新增 IDA 调用。

## 逐项审阅

| 子智能体结论 | 主智能体直接复核与处理 |
| --- | --- |
| 旧 C/S 只有 Authority pre/post 写 g_tick/g_completed；legacy active_runtime 的另一套计数不应并入。 | 重新搜索并阅读修改前 `coop_active_runtime.cpp` Authority/PlayerOnlyUpdate、`active_runtime.cpp` 独立运行时及已冻结 tx-01-89 源码。采纳；当前 C/S 完成值只在生产 `AuthorityTickClock::Complete` 写入，旧全局可写计数移除，IPC completed 仅作镜像。 |
| 普通路径应取精确标准 Room callsite，不能数所有 Room::Update。 | 主智能体复读 `loop_runtime.cpp` kCallRvas/kTargetRvas/kCallBytes 和 StandardRoomHook：槽15为 RVA2FBA4D→402980，返回2FBA52；槽24的52FC77、槽26的2FE37C由生命周期 wrapper 观察，不调 Authority。采纳，时钟只接受2FBA52。 |
| odd 需要 driver helper 正常返回及普通 Main Player witness。 | 复读 `coop_active_runtime.cpp` Prepare、PlayerUpdate、PlayerOnlyUpdate：555241→2FD3F0，返回555246；只有 helper scope 内首个 Main A/B 的 Player caller 2FD60C 开始 Authority。采纳；保存真实首次命中的 Player 及 helper return PC，post使用保存值，不重新读拓扑或用 helper 静态地址冒充 scope owner。 |
| 标准/odd 在现有目标正常 CFG 互斥，但不能外推未知 Mod 重入。 | 主智能体直接读取 `artifacts/cs-t0/tx-01-66-static/control-flow-evidence.json` driver.simulationCallSummaries：955231/block95522F与955241各自无回块，另一模拟块不可达；复读 primary-evidence.json 的版本标识和原始指令。采纳，仅以递增 loop token 做归属/去重，同 loop 第二次 scope 失败停用；loop差值从不决定 tick 增量。不声称穷尽任意间接调用/Mod。 |
| room/death generation 或 collector gap 不应撤销真实正常返回。 | 主智能体复读 `Participant::Invalidate/Begin/End`：失效清队列/绑定/previous，保留 last_resolved 高水位。若不增加已正常返回的 tick，下一次将重复 completed+1。采纳；输入/previous资格仍由 generation决定，时钟只判断所选原生模拟边界是否正常完成。 |
| 普通 Room post早于 tail、owner、outer和完整采集。 | 主智能体复读 `StandardRoomHook`、`UpdateHook`、`world::Commit/StageEvent`：post在 Room原调用后、Room Stage退出前；Commit只给collector token传递tick/kind；Capture在g_update正常返回后、coop TopExit前检查owner/room/tail/dirty/odd。采纳，不等待采集器或文件I/O，不把 tick完成称为完整世界样本。 |
| 无配对完成应失败，但合法未准入pre也有native post。 | 子智能体早期“所有unpaired return都失败”的笼统表述不能直接执行，其后已自行补充。主智能体区分：生产Clock的NoScope不计数；Native另记每次真实pre的boundary，只有对应已观察入口的合法跳过才无操作，来源不匹配/莫名post停用。暖机不能因正常返回被误判失败。 |
| Fail后Enabled提前返回会留下pending；需outer检查，EH不能伪造完成。 | 主智能体确认这是修改前风险，当前 Authority 即使已停用仍会在已配对正常post取消pending；TopExit还检查 EndOuter及未闭合native-call记录。EH若越过所有post，pending不提交；若有后续Begin则重入停用。不捕获/吞掉原生异常，不将清pending后继续重用tick作为恢复。 |

## 新代码审阅与边界

主智能体复读 `authority_tick.hpp/.cpp` 的 CheckStart/Begin/Complete/Cancel/EndOuter，确认：candidate预留不修改completed；正常配对返回才递增一次；来源、owner、线程、main thread、loop全部配对；终态/异常/重复scope不可继续重用计数；预检查先于脚本消费以识别计数耗尽。counter为原子读写，配对状态只在主线程变更，worker不读写pending或原生对象。

子智能体对实现的审阅确认了上述核心契约，但其最终答复仍引用修改前的旧Authority行号，并列出尚需接入的事项。主智能体没有将这些旧行号当作当前代码结论：再次读取当前Authority、TopExit和odd wrapper，逐一落实owner保存、skip/pre-post记录和停用后的取消；此复核属于源码静态阅读，不是运行时验收。

STATIC_CONFIRMED/源码观察支持现有目标的所选边界；新的时钟、loop去重及失败处理属于设计和实现。真实主线程时序、ABI、Hook恢复、异常展开及未知Mod路径本轮未实机观察，保留其限制。旧逐次 PASS/FAIL 及 tx-01-89 离线结果均不改写。
