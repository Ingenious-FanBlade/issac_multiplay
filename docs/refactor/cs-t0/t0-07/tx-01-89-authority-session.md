# tx-01-89：T0-07 操作 1，AuthoritySession

2026-10-19（Asia/Shanghai）。本次完整目标仅为实验域会话状态机、不可变 A/B 身份、输入准入及退休/失败清理接入。沿用现行规范；操作 2–9 的 tick 改造、可配置 scheduler、网络协议及实机断输入实验不在本次范围。

## 实现前登记的失败方式

1. 零 session、零 ClientId 或 A/B 重复 ClientId 被接纳；协商后从可变 IPC 重读身份，造成 ordinal 交换。
2. 同一对象重复初始化、跳过协商/准备直接 Active，或终态重新初始化，使旧 packet 恢复效力。
3. Negotiating/Warmup 时产生输入序号、入队、Resolve 或暴露 gameplay 查询；Retiring/Retired/Failed 仍可消费已排队输入或提交 previous。
4. Warmup 阻断设备绑定或 Join 专用命令，导致必须等待 B、又无法创建 B 的死锁；反过来，未完成唯一 A/B 绑定就进入 Active。
5. Active 日常推进依赖每名 client ready；暂时换房、死亡、解绑或 B 单独退休错误地把整个 session 重置到 Warmup。
6. 结束会话后仍能 Bind，或主线程清理 B 被“停止输入”同时禁止，造成设备留在 vector、对象关联继续有效。
7. Failed 被后续正常 stop/清理改写成 Retired，丢失首个失败；恢复未完成即宣称 Retired。
8. Hook 安装期间回调抢先运行，看到半初始化身份/状态；安装失败后初始化线程把 Failed 覆盖为 Warmup/Active。
9. 错线程、事件环溢出、控制器失联、run teardown 或恢复失败未关闭准入；终态之间竞争造成 Failed 被覆盖。
10. 回调运行时释放或重置 session/participant；把新内部状态误写入旧 IPC state，破坏原前缀布局和恢复协议。
11. 用离线替身重新实现 FSM 并据此宣称生产实现通过；验收失败后自动重跑，或把离线结果称为实机 Hook/Join 恢复证据。

## 设计与验收范围

- 状态为 Dormant → Negotiating → Warmup → Active → Retiring → Retired；未退休状态可进入 Failed。Failed 和 Retired 不可复活；Failed 允许外部资源清理但不被清理成功覆盖。
- Start 一次性复制并冻结 session 和 ClientId→ordinal；当前 controller 提供的配置代替未来 HELLO 协商，不冒充 T0-08 协议。
- Warmup 允许现有主线程拓扑、绑定、虚拟设备及原生 Join；只有初次 A/B exact pair 均就绪才 Active。Active 后临时失绑继续沿用中性和 generation 规则，不新增全员 ready 等待。
- Participant 附着生产 AuthoritySession 时，生产/提交/Resolve/读取/previous 提交均受状态约束。旧阶段未附着 session 的独立调用继续沿用原行为。
- 正常停止和 run teardown 先关闭输入，再按原清理路径退休 B/A；只有全部 Hook 恢复并排空后标记 Retired。任何已识别失败保持 Failed。
- 状态与原因使用单个原子值发布；身份先于 Hook 可见且不再修改。不从 worker 访问游戏对象；主线程清理与恢复线程保持现有分工。
- 最终仅运行一个离线统一入口，贯通生产 FSM、V2 codec、Participant 队列/绑定/查询及退休。先写此失败清单及验收场景，再实现；无实现后补写单元测试。保留输入清单、事件 JSONL、构建日志、二进制和机器结果。
- 直接纯编译 Native 检查真实接入可编译。离线验收不注入、不查询或控制游戏，不证明真实 Join、线程时序或 Hook 恢复；这些仍需以后相应实机验收。

准备阶段只阅读和静态分析；唯一整体验收尚未执行。用户指定的 GPT-6 Luna / max 子智能体负责只读静态分析，主智能体实现与验收。

## 状态转换与入口

| 当前状态 | 触发与下一状态 | Gameplay 输入与绑定 |
| --- | --- | --- |
| Dormant | 合法且唯一的 Start → Negotiating；非法身份 → Failed | 不生成序号、不接受输入、不绑定。Start 仅由初始化 owner 调用。 |
| Negotiating | 配置、Participant、回调参数准备后 Prepared → Warmup | 输入关闭；Prepared 早于所有 Hook 可见。这里是 controller 实验配置的协商，不是网络 HELLO。 |
| Warmup | 主线程在当前完整拓扑下确认 A/B 均唯一绑定、自有 B 已完成 Join（attempt=2）→ Active | 允许绑定和 Join 专用查询；不消费脚本、不分配 gameplay sequence、不 Resolve 或提交 tick。 |
| Active | stop / RunTeardown → Retiring；故障 → Failed | A/B 走既有 codec/Submit/queue/Resolve。临时解绑继续中性，单独 B 退休不改 session；不重新等待全员。 |
| Retiring | 已排空并完成 Hook 恢复 → Retired；恢复失败 → Failed | 输入/查询中性；允许既有主线程 Retire/Invalidate/Remove 清理，不允许新绑定。 |
| Retired | 终态，无重置入口 | 拒绝新旧输入，重复清理完成可幂等确认。新 run 不复用这个对象。 |
| Failed | 终态，首个失败原因保留 | 拒绝输入与新绑定，允许资源清理；清理完成不将逻辑状态改成 Retired。 |

正常取消也允许 Negotiating/Warmup（或尚未使用的 Dormant）进入 Retiring。Prepared/Activate 对已到达的同一阶段幂等，但不会从后续状态倒退；任何重复 Start 都拒绝。session 身份和 client ordinal 不包含临时指针。

新增 `AuthoritySessionTransition` 为 EventKind 31：action=旧状态，decision=新状态，detail=SessionCause；before 的16字节为冻结 session，after 为两个小端 u64 ClientId。该事件不读取 Participant/游戏对象；原 Shared/Event 布局及旧 state 字段不变。环溢出时状态即刻 Failed，即使该转换不能成功写入环，也不会继续接纳输入。

本次只使用当前可用主线程正常清理路径；恢复线程超时会以原子状态关闭准入并保留驻留，不在该线程移除原生对象或盲目卸载。无法完成主线程清理或字节恢复时不会宣称 Retired。物理 poll 在暖机继续采样，只有同 loop 的新鲜源可在 Active 经既有启动规则提交。

## 构建与准备结果

`cs-t0/build-authority-session.ps1` 直接纯编译成功，VS 2022/v143、Hostx64→Win32、Release，Native 与 AuthoritySessionScenario 均正常链接。输出位于 `out/cs-t0/tx-01-89-build`，分别保存构建日志和 toolchain.json；没有执行 build.ps1、Cargo 测试、旧 Native suite 或游戏。33 个离线场景、302 条命令已在生产实现前写入清单；后续只运行一次统一入口，结果另记，不覆盖本段准备事实。

执行更新：同日唯一统一离线验收已完成，33场景、303/303检查 PASS，退出0。Native 未注入；真实 Join/恢复等不在此离线结果证明范围，见[完整结果与证据](tx-01-89-results.md)。上述未执行文字保留准备阶段语境。
