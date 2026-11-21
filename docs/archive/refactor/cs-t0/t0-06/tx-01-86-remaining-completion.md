# tx-01-86：完成当前授权范围内的 T0-06 剩余 E2E

2026-10-19。用户要求继续完成剩余 E2E。目标为 W01.native_results、W02.blocked_windows、W03.retained_unknown、W04.identity_history、W05.transitions 和 W12.fault_isolation 六项。沿用其余已通过证据，不重复全量 replay、严格对齐比较、controller-loss 或已通过终止故障；W01.isolation/W08.replay 继续按现有用户指示排除。旧 FAIL 和 NOT_COVERED 原账本不变，新结果明确列出各来源。

## 修改前登记的失败方式

- 暂停、失焦或未形成完整 owner/Room/tail 链时，墙钟超时不能证明故障分支运行过。命令发送前必须有同一采集的实际普通样本及 Authority tick 持续推进；未满足时不发命令，保存原因并等待人工恢复正常运行。
- ReferenceAmbiguity 只在非空 VALUE 引用读取后消费。使用两个任务自建、出生已知的 Pickup，公开 SpawnerEntity 关系建立后保留；先从实际保存样本取得 KNOWN_CURRENT 字段，再发一次命令。零指针、旧 retained 日志或 Lua action 成功不替代前提。
- 两个引用对象必须先于短命 Pickup 创建并保留，避免 Remove 后的池槽复用使 target->reused 已为真。Native E2E 消费点进一步限定为实际 KNOWN_CURRENT 引用，记录旧状态 1→受控歧义状态 4、原始地址和目标 ID/use，再清空输出 target；不修改原始指针。缺此前提不发命令。
- 事件容量故障以前等待自然生命周期事件，静止房间不会消费。E2E 新增显式 collector 自有诊断事件投递，在下一个普通主线程采集入口进入现有 Emit 容量失败分支；不造游戏实体事件、不改游戏根或指针、不延长旧超时来冲淡旧失败。
- 短命对象没有相邻快照时仍需 Init/Spawn 取得的真实身份、正常 Remove 返回、随后清引用/成员退休；不把不存在于枚举直接判物理析构。新 fixture 使用一个创建后立即 Remove 的 Pickup，不复用已失效 userdata。
- Publish 的正常返回与出生确认额外事件分开计数。Esau 原采集已保存 local_id/use 1/1、49/1 各三次发布、各一次出生；只读使用该事实，不再制造重复发布或新生。
- pause/loading/lifecycle 的每条 gap 都要关联同 outer 的实际分支、阶段数量、提交和自有状态；新增 E2E Window 记录只复制 companion 已有状态，不在 collector 调用游戏 getter。没有 normal 链只能说明结构性原因，不能擅自贴 UI 暂停标签。
- 新房间/run 退休需要实际 enter/exit、epoch、首个正常样本和退休后无旧对象/输入复活。Restore 后当前 birth 映射未知继续保持未知。
- W01 使用已有三个采集的 19 入口 ABI/原函数 exactly-once 证据，明确混合记录构建的来源；没有声称本轮新 Native 在同一次采集重新覆盖全部 19 个。
- 每个 fault 只发一次。新范围中只补 object-budget、reference-ambiguity 和 event-overflow 的新触发/前提实现；其余分支按旧事实沿用。任何异常、丢记录或恢复不确定立即停止相关实机修改，不重注入/强制卸载/自动重跑。

## 静态依据与实现范围

当前 IDA 模块 isaac-ng.exe、imagebase=0x400000，路径与既有目标一致。[原始静态证据](../../../../../artifacts/cs-t0/tx-01-86-static/ida-evidence.json) 包括 Lua Remove 注册 0x868A28→0x8C5D06→虚槽+0x28，基类 Remove 0x6ACB00 及 Pickup/Effect 派生路径；公开 SpawnerEntity getter/setter 0x4174A0/0x6A9360 对应 +0x3C8，与固定字段 0019 一致。没有修改 IDB 或校验哈希。

计划只修改 E2E 条件下的 Window/Reference/诊断投递记录、可移除 fixture 和统一执行器。ISCS、Lua Sync API、canonical 输入、Hook 目标/70 槽与生产 profile/IPC 布局保持现状。整轮修改和静态阅读完成后，只纯编译新 E2E Native，再执行一个统一入口：新 primary 场景（短命 Pickup、非空引用、object/ref 故障、暂停与房间/run 退出）及新 event-overflow 进程；同一入口读取历史证据，生成完整 33 子项的新账本和六项目标的范围结果。

Native 已直接纯编译成功（没有执行程序或测试），输出 `out/cs-t0/tx-01-86-build/native/IsaacCsT0.Native.dll`，完整日志 `out/cs-t0/tx-01-86-build/native-build.log`。统一入口为 `cs-t0/accept-world-remaining-completion.py`，Window/Reference/生命周期分析为 `world_e2e/completion_evidence.py`；均仅静态阅读，没有语法试跑。

准备期间未查询/操作游戏，未运行 Python/Lua、单元测试、probe、smoke 或试跑；本轮唯一整体验收尚未开始。
