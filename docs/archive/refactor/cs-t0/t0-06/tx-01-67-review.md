# tx-01-67 整体静态文档审阅

日期：2026-10-18。目标：state/timer 原样只读采样所需的最小逐类字段登记。按用户要求，由 GPT6 Luna / max 使用 IDA MCP 分析，主智能体在交稿后重新读取当前数据库中的访问指令、注册上下文、虚表及初始化锚点，并修订最终研究稿。

遵循[现行规范](../../project-conventions.md)。本次仅静态阅读、逆向与文档编辑，未实现采集器，未操作游戏、注入、安装 Hook、修改 IDB、运行构建或代码测试，也未计算或校验哈希。分析中的反编译、指令读取与证据导出用于资料分析；全部修改完成后只进行一次整体静态文档验收。

审阅对象：[研究正文](tx-01-67-state-timer.md)、[阶段入口](README.md)、总任务书及 `cs-t0/README.md` 的本轮更新，以及以下独立保留的证据和导出脚本：

- [子智能体原始证据](../../../../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json)与[原始导出脚本](export_tx_01_67.py)。
- [整数扩展补充](../../../../../artifacts/cs-t0/tx-01-67-static/supplemental-evidence.json)与[补充脚本](export_tx_01_67_type_evidence.py)。
- [主智能体复核证据](../../../../../artifacts/cs-t0/tx-01-67-static/primary-evidence.json)与[复核脚本](export_tx_01_67_review.py)。
- [绑定与 Grid 初始化补充](../../../../../artifacts/cs-t0/tx-01-67-static/binding-evidence.json)与[导出脚本](export_tx_01_67_binding_evidence.py)。

主智能体已完成的实质修订：

- 将 `0x6E02F0`、`0x60DE70`、`0x5CDB20` 从初稿的 ctor 更正为派生虚 Init。当前虚表 `+4` 直接指向这三个函数；逻辑使用的初始化与池对象 C++ 构造不等同。
- Grid initialized 字节更正为 `+0x1C`；`+0x28` 是由当前 Game frame 写入的字段。补充基类构造 `0x709C40` 在 `0x709C6E` 对 State `+0xC` 写零，保留派生初始化、恢复和后续 writer 未穷尽的限制。
- Pickup Timeout/Wait 的直接 `mov` 只确认 raw32 宽度；反编译 `int` 不单独作为底层 signedness 的证明。其它整数桥接明确区分符号扩展与零扩展，也不外推 C++ 声明。
- FrameCount 保持计算属性，观察者如需等价值应在同批读取两项 raw32 后按 32 位模减法派生，避免调用 getter或引入观察者端有符号溢出。

整体审阅项目：

1. 当前 IDB、banner、imagebase 与地址一致，属性名、类别上下文、offset 捕获和 raw 访问指令相互对应。
2. State/timer 按类登记；NPC 的 ProjectileCooldown/Delay 没有误配给 Projectile，Laser Timeout 没有误配给 Tear，Grid Desc-relative `+8` 与对象 `+0xC` 没有混淆。
3. 已登记字段的宽度、整数扩展和具体初始化锚点有原始指令支持；初值不冒充所有分支最终值，零值不作为生命周期或重生判断。
4. FrameCount 为计算值；普通 Entity/Tear/Projectile 本轮未闭合的专用字段诚实保留 UNKNOWN，未把未登记解释为不存在。
5. 沿用 tx-01-66 的主线程正常完整更新窗口与 lifecycle 处理，只读复制、不调用 Lua/getter，不把 odd Player-only 当完整世界输出。
6. 静态事实、设计建议与未决语义明确区分，未扩展为完整 AI 状态机、全部计时单位/reset、字段全集、采集器实现或运行时验收。
7. 原始及补充证据独立保留，文件引用和入口范围一致，未覆盖历史研究或推进后续操作。

结果：`PASS_STATIC_DOCUMENT_REVIEW`。上述 7 项整体静态审阅通过，仅适用于本轮 state/timer 最小原始字段登记。

本次唯一整体审阅读取了最终研究正文、审阅记录、阶段入口及两个总入口的更新段落，并核对四份独立证据。原始导出含 40 个函数和 37 处绑定调用上下文；整数桥接补充含 2 个函数；主智能体复核含 19 个函数和 9 段指令；绑定/Grid 补充含 4 个函数、2 段指令及 3 张虚表，合计 44 个不同函数。函数记录均有伪代码和非空指令，未出现导出错误；三个审阅文档中的 45 处本地文件引用均存在，两个总入口的新引用与这些已核对目标一致。

已登记字段可用于当前类对象的 raw32 复制设计；这不证明计时单位、完整状态机、所有恢复/重置路径或运行时采集正确。原始 JSON 和补充材料分别保留，没有运行游戏、构建、代码测试或重复验收；采集器及操作 3 的其它候选字段仍未完成。
