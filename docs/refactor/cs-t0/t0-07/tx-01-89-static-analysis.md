# tx-01-89：操作 1 静态分析

用户指定 GPT-6 Luna / max 子智能体 `/root/t007_session_static` 完成只读源码分析，主智能体复核并据此接入。未使用游戏进程、调试器或新 IDA 结果；本次不变更原生 ABI、Hook 目标或游戏字段，已有源码及交接足够，不需新增原生逆向。

以下为源码观察和设计决定，不是运行时证据。

| 源码位置 | 已有行为及接入决定 |
| --- | --- |
| `coop_active_runtime.cpp` Initialize / `loop_runtime.cpp` StartEmbeddedObserver | Hook 安装返回前回调可能已可见；冻结 identity、初始化 Participant、发布 Warmup 必须先于 StartEmbeddedObserver。state/cause 以同一个原子值发布。 |
| `RefreshBindings`、`JoinPass`、`OwnedReader`、`AllowCreate` | 初始 Join 需要 A bound 和注册的自有设备；特殊 Join 命令绕过 gameplay Read。Warmup 继续这些原生路径；B 完成唯一绑定且 attempt=2 后，在下一 Authority 首次进入 Active。 |
| `PublishPhysical`、`ScriptInput`、`SubmitFresh`、TopExit conflict control | 必须在 producer 层先检查 Active，不能只靠 Participant 拒绝，否则暖机会被误判协议失败或消耗脚本。物理采样继续，但暖机不分配输入序号或消费脚本。 |
| `Authority` / Participant Begin、Read、End | 暖机跳过 Resolve/提交；非 Active 的 Begin 拒绝不能报成重入。终态后当前查询中性、previous 不提交，取消尚未提交的 scope；已排队的输入失去效力。 |
| `Fail` / `Publish` overflow | 原先两条失败路径分开；都必须不依赖事件发布就关闭 session。环溢出不能递归发布；首个 session 失败原因保持。 |
| `TopExit` / `RetireB` | 主线程识别 stop 后先 Retiring，再清理 B/A。单独 retire B 不退休 session；临时换房/死亡的 generation 失效不重新暖机。Retiring/Failed 不禁止原有主线程清理。 |
| Shutdown / `LifecycleCallback` | helper 只发 stop、等主线程清理、恢复/排空；不把原生移除操作搬到 helper。RunTeardown 主线程先 Retiring。恢复超时/失败保持 Failed；正常恢复完成才 Retired。 |
| `Shared::state` | 它是既有安装/恢复状态，恢复成功可为4；不复用为新的会话逻辑状态。Failed 必须独立保持，日志新增事件但不改变 Shared/Event 的尺寸和旧偏移。 |

已知限制：初始化须由单一 owner 在 Hook 可见前完成；不声称穷尽旧虚拟设备 worker/OOM/EH 并发。离线脚本的绑定只是不可解引用的自有标记，覆盖生产 FSM/codec/Participant 的生命周期，不提供真实 Join、主线程时序或 Hook 恢复证据。
