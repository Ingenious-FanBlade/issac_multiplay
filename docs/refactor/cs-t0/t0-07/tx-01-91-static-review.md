# tx-01-91：Luna 静态结论与主智能体审阅

2026-10-20（Asia/Shanghai）。按用户要求使用 **GPT-6 Luna / max** 子智能体 `/root/t007_client_state_static`。其任务限于只读分析，不改文件、不构建、不测试、不访问游戏。主智能体直接复读当前源码及 tx-01-90 中的修改前源码快照，再审阅其结论；没有新增 IDA 调用。已有生产源码足以界定本操作，不需要新增原生逆向。

## 逐项处理

| 子智能体结论 | 主智能体复核、决定与落实 |
| --- | --- |
| 本地 sequence、accepted 高水位、last resolved、队列与 history 已存在；失绑不应清会话历史。 | 直接复读修改前 `Participant::Initialize/Invalidate/MakeCommand/Submit`。采纳，保留本地生成序列与 accepted 高水位两套值；新增 `Progress` 不用一者冒充另一者。history 仍为4096项，淘汰后 `sequence <= last_accepted_` 继续拒绝旧包。 |
| received、accepted、resolved、processed 必须分开。 | 复读 `Submit/Begin/End` 及生产 `AuthorityTickClock::Complete`、Native Authority post。采纳：received 是可归属当前冻结 session/client 的到达，接收时刻取 authority Completed，另存原始 sequence/target/generation/decision；身份相符的非法、旧代、重复、过期和退休后尝试也会记录。这些诊断不驱动 timeout、ACK 或模拟资格。accepted 仅成功入队；resolved 是 Begin；processed 仅正常 End 且 session、绑定和代际仍有效。 |
| 缺输入可确认中性状态，但不能伪称处理了新 sequence。 | 补充限定：保留既有 fresh binding 无首条输入时 current 不可见的规则，此时 clock 可以前进，per-client processed 不前进。已有有效可比较输入后，缺输入正常提交立即中性，source_sequence=0；processed 高水位保持。 |
| 同 tick 的 edge OR 无法用最高 current sequence 代表全部来源。 | 复读 Begin 的 OR/最高序号选择和四位 edge-conflict 检查。采纳四项固定 ledger，分别记录 accepted、resolved、processed 的 sequence/tick/generation，只有成功 End 才增加 processed_count。当前范围另存四个来源，重复 Read/End 不重复确认。此“处理”不等于世界中的炸弹或道具效果一定成功。 |
| Invalidate/End(false) 的旧 current 可能被 MakeEvent 带到后续事件。 | 主智能体复读 Native `MakeEvent/Invalidate/RetireB`。采纳：失绑和失败范围清 current/sequence/edge 来源；Progress 的 current/last state 带有效性，last state 不含 edge。历史账本和计数保留代际。正常成功后的原有事件仍可携带该已结束范围的来源用于诊断，读取应用资格仍为 false。 |
| future 上界和 MakeCommand 的 edge+1 要避免溢出。 | 直接核对原验证差值与 `kMaximumFutureTicks=64`。上界饱和于u64最大值，显式窗口与validator一致；edge target已经为最大值时，MakeCommand返回空命令并退休，不消耗序号或回绕。Clock自己的末端停用保持不变。 |
| 需要考虑 current 不可见、但 Begin 已发生的范围。 | 主智能体补充：不能只用 visible 判断迟到，也不能只靠 scope_pending。用持久 last_resolved 排除已经解析的 tick，涵盖范围中途失绑后重绑；另外独立 scope_pending 阻止不可见范围重入。正常返回后仍按既有 clock 提交，不以新增计数阻断模拟。 |
| 编码失败在 Participant 外短路；过期队列项被静默丢弃。 | 直接复读 Native Submit，采纳 `RejectInvalidEncoding` 补齐短路计数，仍按原来的 Invalid 优先返回，不进入 history/conflict 检查。已接受后过期单列 expired_before_resolve，失绑丢弃队列单列 discarded_on_invalidate，均不伪装成 Submit rejection。 |
| 所有拒绝应分类，SequenceConflict 先记账再退休。 | 逐一复读所有 Submit return 分支。accepted/duplicate各自计数，12种拒绝进入统一 FinishSubmit；冲突先记账再 Retire，A 的既有 Native session failure 规则保持。错误身份会进入对应 Participant 的尝试/拒绝计数，但不会更新其可归属 received 记录。 |
| 固定大小、饱和计数、只读查询，不增加热路径事件。 | 复读 `Read/RepeatQueryReads` 的整个对象字节比较、Publish 满环失败与 Event/Shared布局断言。采纳：固定数组、无新增堆分配/锁，所有计数饱和且粘滞标志记录溢出；Read/Progress 为 const。只在主线程退休处新增最多28行/client的 EventKind33，发布失败停止额外输出并继续既有资源清理。Event仍128字节、Shared v3/v4和24/64字节codec保持。 |

## 新实现的静态边界

主智能体检查了 `coop_input.hpp/.cpp` 的全部新增分支和 Native 编码失败、退休接线；还直接阅读 `authority_session.cpp` 的身份/Active限制、`authority_tick.cpp` 的提交顺序以及 Rust controller 对数值事件的原样序列化。没有把子智能体基于旧快照的行号当作当前代码事实。

processed_sequence 是已正常确认范围中**最高**来源序号，不能充当“所有更小序号均已处理”的连续前缀。target允许不同sequence按不同tick到达，最高值之外还记录当前确认的 source_sequence 和每种edge来源。resolved/processed_contributors仅给出当范围贡献数量，不新增无界逐命令账本。

诊断计数u64饱和算法经过静态阅读；不人为开放生产计数写入口来制造u64次到达。其溢出分支不宣称得到运行时覆盖。未来窗口展示的是时间约束，window_valid不保证任意payload通过其它身份、序号、容量及edge规则。

离线场景贯通真实生产 Session/Clock/codec/Participant，但 Native Hook、本机主线程时序、真实对象生命周期、事件环背压与世界动作仍需对应实机工作；本轮不访问游戏。唯一最终验收状态另见结果记录，静态结论不替代运行时事实。
