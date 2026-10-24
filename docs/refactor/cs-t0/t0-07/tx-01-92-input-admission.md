# tx-01-92：T0-07 操作4，统一输入准入

2026-10-20（Asia/Shanghai）。用户确认本次为接续操作3的 **T0-07 操作4**。完整目标：A 本地 loopback 命令与 B 远端来源的自有字节载荷进入同一生产 `AcceptInput`，共享解码、身份/代际/时间窗校验、去重、拒绝记账和既有逐 client 队列；收起可直接调用的内部 Submit，保留 Native 的事件和冲突处理。操作5–9、真实网络协议与 socket 不在本次范围。

## 实现前失败方式

1. A 直接调用底层 Submit 或写 current/previous，绕开远端解码器；B 使用另一份队列、history 或 validator。
2. 本地 Encode 失败直接返回，漏记或重复记拒绝；非法字节经过本地重新编码被修正成合法输入。
3. 短包、长包、错误版本/长度/reserved/flags/轴值被接受，读取越界，或截掉尾部后接受。
4. 未完整收到命令头便虚构 client/sequence/target 的 received 事实；完整头但非法载荷被漏计。
5. 根据不可信载荷 client_id 自动改路由，A 发到 B 的来源被错误接纳；单 client 拒绝污染另一队列。
6. typed 与 bytes 入口各自去重，跨入口重传产生第二次 edge；同 identity 冲突不退休，或另一 client 同序号被误去重。
7. 同一次尝试被 codec 和 Submit 各统计一次，错误 session/client、旧代、失绑、Warmup/Failed/Retired 绕开验证。
8. 本地排队后立即可读；未来/已经 Resolve 的 tick 入队，或非法输入改变 current/previous/已接受序号。
9. 拆入口改变同 tick 连续量选择、合并 edge 的真实来源、正常 End 确认和查询只读行为。
10. 启动 completed+1 与 lead=2 连续量补样走不同验证或重复 edge；失绑后旧样本复活。
11. 队列满或 history 淘汰后本地输入享有例外；远端静默让 A/tick 等待全员。
12. 新接口让网络线程修改 Participant/游戏对象；把 caller 提供的 generation 当作 wire 身份或自动升级当前代。
13. 修改24/64字节codec、Decision枚举或IPC布局；增加Hook、进入原版Online/Isaac Sync或复用其状态。
14. 离线驱动复制codec/validator而未调用生产入口，或者把离线生命周期结果写成实机/网络/世界动作通过。

## 工作与验收安排

先写上述失败清单、场景、离线驱动及统一验收入口，再修改生产实现。GPT-6 Luna / max 只读分析，由主智能体直接复读源码逐项审阅。过程中不运行测试；准备完成后纯编译 Native 与场景程序，最终只执行一次统一离线生产链验收，保存逐条命令、接收字节、状态与结果。实际游戏/Hook/网络线程不在本次运行范围。

主线程 owner 约束沿用现有 Native 回调；`AcceptInput` 不提供线程同步。来源已由外层绑定到对应 Participant，载荷里的 client_id 只用于校验，不能重路由。generation 与 completed_tick 是本地主线程元数据，未加入64字节载荷。B 当前生产来源仍为脚本；字节入口可供未来接收队列在主线程使用，不宣称已实现真实网络接入。

## 最终接口与分类口径

`Participant::AcceptInput(command, generation, completed_tick)` 只复制64字节并转到 `AcceptInput(span, generation, completed_tick)`。本地非法值不在序列化前被修复或漏记；字节入口要求精确64字节，通过原V2 Decode后才进入私有 Submit。Submit负责原有状态/代际/session/client/sequence/target/edge/容量验证，并写原有队列和history；没有增加transport分支或另一套输入状态。

每次到达恰好一次submit记账。新引入的非法长度span，只有收到完整40字节头（session/client/sequence/target）且身份匹配才计received；0–39字节只计submit与Invalid，不更新received进度，40–63或65以上字节可记录完整头，但仍为Invalid。24–39字节虽然可提取身份，缺失的sequence/target没有有效性字段，因此本轮保守地不记录received。这是新增malformed-span的分类口径，不能套用tx-01-91只处理完整64字节命令时的概括，或把缺失值填零当成观察事实。原完整64字节命令的计数与拒绝优先次序保持。

Native原 Submit wrapper现只调用公开AcceptInput，继续发布原有Submit事件，A的SequenceConflict继续关闭session，B的冲突继续只退休B。启动completed+1和lead=2补样、普通输入、故障脚本及TopExit控制提交均汇入这一wrapper；poll只采样、Read只消费已Resolve快照。原三个离线场景驱动仅迁移到公开API，旧场景与历史artifact不改写。

40个实现前场景涵盖原输入契约及typed/bytes两个生产入口，包含两个方向的跨入口duplicate、跨入口conflict、A/B相同sequence独立、错误身份不重路由、逐入口非法codec与生命周期/时间窗、空/短/长载荷、在途非法尝试不污染current、手工输入启动两份载荷、A队列满与120 tick B静默。启动场景直接提交按既定规则形成的两份命令，不声称运行了Native的物理采样/SubmitFresh。统一入口保留原始接收字节及来源标签供复查。
