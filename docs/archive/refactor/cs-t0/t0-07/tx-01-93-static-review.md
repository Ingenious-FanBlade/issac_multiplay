# tx-01-93：Luna结论与主智能体逐项审阅

2026-10-20（Asia/Shanghai）。按用户要求调用 **GPT-6 Luna / max** 子智能体 `/root/t007_tick_pipeline_static`，只读分析当前源码、现行规范、任务书与操作2/4资料。主智能体自行复读修改前路径与修改后代码，并对以下结论逐项处理。子智能体没有改文件、构建、测试、访问游戏或调试器；不需要新增原生逆向。

| 子智能体结论 | 主智能体复读与决定 |
| --- | --- |
| 原实现先Complete再Outer capture，操作5需要改变真实Native接入，不能只增加孤立状态机。 | 采纳。直接复读Authority、UpdateHook和world::StageEvent顺序；Native改用同一生产pipeline，原生返回只CheckComplete/End(true)，TopExit在复制之后Complete。 |
| BeginDrain后所有A/B提交均进入pipeline，没有发现生产Participant.AcceptInput旁路。 | 复读所有Submit调用。采纳：A物理poll只观察；A/B普通、startup、fault均在Authority pre；control3转为下一drain并保留generation。预算保守258，当前实际最坏134。 |
| current在等待post-state时仍保留，consumer必须另受SimulationTick约束。 | 复读Consume、Project/Query、PlayerUpdate读取依据和Shooting；全部使用SimulationTick，AwaitPostState返回0。调度、重入检查与诊断仍保留原clock候选身份。未用current存在代替可消费状态。 |
| 原生返回后End(true)，post-state在Outer尾部复制，批次保持state=1直到TopExit提交后才release。 | 逐读pipeline.NativeReturned、world::FinishBatch/StageEvent和TopExit。采纳。原wrapper仍是original唯一调用owner；world::Commit名称保留，但其含义只是原生返回witness。 |
| 采样失败/背压/错配不得抹掉实际正常模拟；旧批次不得误发布。 | 复读FinishPostState与world pending交换；采纳。状态不匹配降为Unavailable，Native另外核对tick/status/capture后才发布buffer；不匹配置FREE。Notify只报告本次通知失败，既有collector错误不被当成pipeline发布失败。 |
| NativeReturned后退休仍应记录模拟；若RunTeardown发生在return之前则CheckComplete(active=false)不计数。 | 明确认可这一口径。正常返回witness已经成立后，FinishPostState不重新用session是否Active否认事实；若在该witness前session已经退休/失败，原生返回不满足Active完成条件，取消候选并关闭后续输入。两者不混用。 |
| 无collector使用Disabled，与一般Gap状态不同，需要说明它是无采集缺口。 | 采纳说明建议，保留独立状态0以区分“未配置采集”与“启用但不完整”。Disabled是明确的无collector post-state缺口，不是完整值、成功采样或网络协议中的状态。没有伪造哈希或默认世界。 |
| receipt分开保存当前/已处理generation；event34仅发布已处理代际，当前代际需要Binding/progress关联。 | 复读receipt构造、Participant进度与事件赋值。认可这一限制：event.generation只解释历史处理来源，boolean仅说明当前binding有效，不声称当前generation已编码；当前关联保留于内部receipt和既有Binding/progress事件。未用新代包装旧处理。 |
| pending错配会释放，Close保留pending且Drained等待，不会误报恢复完成。 | 复读atomic pending指针、PublishPostState(false)、Stage Outer入口与Drained。采纳；正常TopExit负责释放，异常outer/EH缺失不强行报告可恢复，未宣称完整异常/并发闭包。 |

主智能体另核对：world value批次与回执的publication不是全体原子事务，通知/事件环失败可能留下已发布前缀；已完成tick不回滚，不自动重试。processed保留高水位和实际source序号，不是连续ACK。没有网络收包、wire ACK或snapshot apply；Event/Shared大小、24/64字节codec、原Hook总数不变。原版Online/Isaac Sync不进入或复用。

本审阅属于实现分析，不作为另一次测试验收。真实Windows wrapper、采样内存、通知与恢复仍未运行；唯一最终离线验收单独记录。
