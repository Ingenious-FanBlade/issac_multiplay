# tx-01-92：Luna 结论与主智能体逐项审阅

2026-10-20（Asia/Shanghai）。用户指定 **GPT-6 Luna / max**，由 `/root/t007_unified_enqueue_static` 只读分析当前生产源码；不改文件、不构建、不测试、不操作游戏/调试器。主智能体直接复读相关源码、任务书及现行规范，另审阅子智能体对修改后源码的反馈。已有源码足够确定操作4范围，无新增IDA/原生逆向。

| 子智能体结论 | 主智能体复核与决定 |
| --- | --- |
| 原A/B已经共用Native内部Submit，Participant级共同codec准入即可，无需另造session router。 | 复读 `coop_active_runtime.cpp` 的Submit/SubmitFresh/ScriptInput/PublishPhysical及 `coop_input.cpp` 的Submit。采纳：新增公开AcceptInput，原Submit与RejectInvalidEncoding私有；无第二队列/validator。 |
| typed overload应仅序列化并转span，非法数据也必须恰好一次计数。 | 逐读 `coop_input.cpp:207` 的两个overload：typed复制64字节；span只有Decode成功才调Submit，完整头的非法输入调RejectInvalidEncoding。这两个终点各调用ObserveArrival一次；不完整头只直接计submit与Invalid，没有提前Encode失败路径。主审另补不完整头的显式分支，避免未初始化/无效身份恰为全零时被默认头误归属。 |
| 可信来源选择Participant，载荷里的client不能决定可信路由；无需把router放入Participant。 | 复读冻结Identity/MatchesIdentity、Native固定ordinal与Submit的WrongSession/WrongClient验证。采纳；字节输入不会改队列、client或session；不把该核心接口称为完整接收/认证模块。 |
| 40字节归属阈值比“仅session/client可归属”的概括更窄，必须解释24–39字节。 | 采纳这一审阅意见并补写接口口径：本轮新增非法长度span要求完整命令头才记录received，24–39只计submit+Invalid，不补造sequence/target；原完整64字节行为不变。未静默宣称两个定义完全相同。 |
| Native当前仍只提交typed命令，没有真实网络dispatch调用span。 | 直接检索所有AcceptInput调用并复读Runtime导出与ScriptInput。确认；B仍是脚本模拟远端源。本轮交付共同生产核心及直接字节入口，真实网络/transport未实施。未来bytes wrapper还要保留Native事件与A冲突处理。 |
| generation/completed是本地上下文，异步接收不能在重绑后改标新代。 | 复读Native提交处和Participant::Invalidate/Submit。采纳：本轮保留当前主线程owner与实际generation/Completed，未新增网络线程写入。未来接收队列必须保留来源/代际元数据并在主线程提交。 |
| startup、物理poll与Resolve约束需要保留。 | 直接复读SubmitFresh的completed+1/lead=2无edge补样、PhysicalExit的采样、Authority与TopExit的MainThread检查，以及Participant::Begin/End/Read。本轮这些逻辑未改，只替换共同准入调用。离线启动场景不替代Native物理输入证据。 |
| malformed拒绝优先次序及host/B冲突处理不能被公共入口抹平。 | Decode失败仍先返回Invalid，不进入history冲突判断，与原Native Encode/Decode失败路径一致；成功解码后沿用完整Submit顺序。Native继续仅对ordinal0的SequenceConflict调用Fail，Participant仍退休冲突client。 |

修改后的ABI/预算复读：InputState仍24字节，InputCmd仍64字节，Decision和ClientInputProgress布局不变；新增方法不增加Participant成员；Event/Shared无变更，无新增Hook或原版联机路径。主智能体以源码审阅认可上述最小改动；实机线程/Hook和世界动作未由静态结论证明。

本文件是实现依据和子智能体审阅记录，不另作为一次测试验收。最终唯一统一离线验收结果单独登记。
