# tx-01-66 整体静态文档审阅

日期：2026-10-18。目标：位置/速度与只读全房间采样所需的最小静态依据。GPT6 Luna / max 子智能体完成逆向初稿及原始导出，主智能体重新读取当前 IDA 的关键指令、补充控制流并审阅结论。

本轮仅做静态研究，不实现采样器，不运行游戏、构建或代码测试。分析过程只有文档/源码阅读、静态反编译、指令和正常 CFG 读取、证据导出与文档编辑；没有计算或校验哈希，没有修改 IDB 或运行时代码。最终仅执行本次一次整体静态文档验收。

审阅对象：[研究正文](tx-01-66-readonly-sampling.md)、[原始导出脚本](export_tx_01_66.py)、[主智能体复核脚本](export_tx_01_66_review.py)、[控制流补充脚本](export_tx_01_66_control_flow.py)、[原始证据](../../../../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json)、[复核证据](../../../../../artifacts/cs-t0/tx-01-66-static/primary-evidence.json)、[控制流证据](../../../../../artifacts/cs-t0/tx-01-66-static/control-flow-evidence.json)、[阶段入口](README.md)，以及总任务书和 `cs-t0/README.md` 的本轮更新。

审阅项目：

1. 当前 IDB、banner、image base 与引用地址一致；Position/Velocity 属性映射、原始偏移和 f32 分量有注册、访问器和浮点运算指令支持。
2. 当前 Position、staged/可恢复位置、PositionOffset 和 Player 内嵌 VelocityBeforeUpdate 没有混淆；slot4 的 flag-2 早退和有状态位置步进均如实记录。
3. 推荐采样点晚于正常 Room、实体/空间处理和标准 POST_UPDATE，同时早于本轮 render 和现有会写世界的 coop::TopExit；不是无条件顶层返回、单实体返回或 scheduler previous 提交。
4. Room 正常 CFG 的列表调用必经性、参数零和 driver 两分支的调用次数有当前静态依据。标记仍按本次调用 token、嵌套和计数处理，未把正常 CFG 外推为任意 Mod 重入保证。
5. 奇数帧 Player-only 不冒充全房间模拟；暂停/无正常更新不重发旧样本。已知 lifecycle brackets 及同窗 epoch 变化有跳过策略，未宣称全部状态机已恢复。
6. 只读复制来源、Grid 存储 index/Room width 与坐标派生、数组边界、列表来源及 Remove 待提交状态明确；没有调用查询/清理来制造干净世界，也不要求所有成员在该 tick 具有相同更新年龄。
7. 设计建议、静态事实和 UNKNOWN 分开，原始证据保留；没有扩大到全部 writer、临时身份、其它候选字段、采集器实现或运行时验收。
8. 本轮文件、链接、入口和范围一致，导出产物可供复查，未覆盖历史 tx-01-63/64/65 的报告和原始证据。

结果：`PASS_STATIC_DOCUMENT_REVIEW`，上述 8 项整体静态审阅通过，适用范围仅为本轮最小只读采样的静态方案。

唯一一次整体验收读取最终研究正文、审阅记录和入口更新，核对原始与两份补充证据：原始导出含 35 个函数、21 处调用上下文、8 个虚表；复核导出含 11 个函数和 11 段指令；控制流补充另含实际转场 owner 一个函数，合计 36 个不同函数。原始及复核函数未出现缺失、反编译失败或空指令项；研究、审阅和阶段入口的 41 处文件引用均存在。

审阅确认 Room 正常 CFG 的调用必经性、标准/odd 分支不可回到自身或相互到达，以及 Grid 公式的 40/120 和位置阶段的 0.5 原始浮点常量。正文中的采样策略均明确为设计建议，生命周期穷尽和运行时行为保留为未验证。本轮没有运行游戏、构建、代码测试或重复验收，也没有把第三步其它字段或采样器本身标为完成。

主智能体更正/补充了以下内容：

- `0x92FC77`、`0x6FE37C` 是 Room 调用点，返回地址是 `0x92FC7C`、`0x6FE381`。
- PositionOffset 的 getter 为 `0x417310`，setter 为 `0x4172F0`；原始导出脚本注释相反，未来导出脚本已修正，原始 JSON 保留。
- `0x6DAD60` 返回 `Player+0x1660` 内嵌向量地址，不是读取 pointer 成员；slot4 也并非每次调用都执行位置步进。
- `0x803E70` 的参数明确为零；正常 Room 返回路径无法绕过该调用 block。driver 标准与 odd call block 均不能回到自身或走到另一调用 block。
- `0x6FB51C` 的实际 owner 是 `0x8318A0`，已补充完整函数；最初选取的 `0x6FE2F0` 不能当作该调用的目标。
- Grid 坐标应以对象存储 index 与当前 Room width 派生，来源槽号另记；采样建议的插入点位于现有 TopExit callback 之前。

这些是静态研究与稿件修订，不是额外运行时测试。原始 JSON 未重跑覆盖；新增的两份补充证据分别保留复核指令和控制流依据。
