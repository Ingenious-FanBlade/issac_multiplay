# tx-01-68 整体静态文档审阅

日期：2026-10-18。目标：seed 与归属引用原样只读采样所需的最小静态依据。按用户要求由 GPT6 Luna / max 子智能体使用 IDA MCP 分析，主智能体在交稿后重新读取关键指令并审阅结论。

执行要求见[现行规范](../../project-conventions.md)。本轮只进行文档/源码阅读、当前 IDA 反编译、指令读取、证据导出和文档编辑。不运行游戏、注入、构建或代码测试，不修改 IDB，不计算或校验哈希，不实现采集器或跨 tick 身份。唯一一次整体静态文档验收安排在所有修订完成之后。

审阅对象：[研究正文](tx-01-68-seed-owner.md)、[阶段入口](README.md)、总任务书和 `cs-t0/README.md` 本轮段落，以及[原始证据](../../../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json)、[子智能体导出脚本](export_tx_01_68.py)、[主智能体证据](../../../../artifacts/cs-t0/tx-01-68-static/primary-evidence.json)和[复核脚本](export_tx_01_68_review.py)。复核脚本以非 main 名称加载[有 main guard 的旧 helper](export_tx_01_67_review.py)，不重跑或覆盖历史导出。

另补充[直接 Grid.Desc 绑定证据](../../../../artifacts/cs-t0/tx-01-68-static/grid-evidence.json)及[补充脚本](export_tx_01_68_grid_evidence.py)，同样以新文件保留。

主智能体复核后的补充和修订：

- 直接核对 `Grid.Desc` 的 offset 4 捕获和两个 16-byte 块复制，确认描述内嵌在 native Grid 对象中，Lua getter 另外分配值拷贝；采样不应调用该 getter。
- 补充 `Grid.GetRNG` 返回 Grid `+0x2C`，与 `SpawnSeed(+0x14)`、`VariableSeed(+0x20)` 分开登记，不能将 VariableSeed 当作同一 RNG 状态。
- 补记 `SpawnerType(+0x34)`、`SpawnerVariant(+0x38)` 的直接 getter 和当前 Spawn 写入锚点，不需要跟随 SpawnerEntity 重新读取分类。
- 另一处 `SpawnSeed` 的宿主由初稿的未知补全为 RoomDescriptor，依据同一根注册中的类上下文；不扩大到该描述对象的 seed 研究。
- Grid initialized 标志仅作辅助元数据；初始化未确认的对象保留枚举记录和可用性标记，不能静默省略。NULL 引用与未采到字段分别记录，非零值不保证目标有效。

整体审阅项目：

1. 当前目标 IDB、banner、imagebase、类绑定、偏移和原始指令相互对应，不把历史源码偏移直接当当前事实。
2. Entity InitSeed、DropSeed 和 GetDropRNG 的实际位置与返回形式明确，初始值相同不推导永久不变或身份唯一。
3. Grid Desc-relative seed、对象内嵌描述和独立 RNG 存储不混淆，读取宽度及必要初始化线索明确。
4. Parent、Child、Target、SpawnerEntity、Familiar.Player 等按各自字段登记；raw 引用值、受追踪引用维护和归属语义不混为一项。
5. 构造、Init、Spawn、清引用只作为已观察的最小锚点，不从无直接写入外推所有虚调用/回调都保留字段，也不把 Remove 返回当全部引用已清理。
6. 读取策略沿用 tx-01-66 的主线程正常完整更新窗口，只复制当前对象内已确认的值，不跟随未知目标解引用，不赋予跨 tick 或跨进程身份。
7. 静态事实、设计建议、UNKNOWN 和本轮限制区分清楚，不穷尽全部 RNG 消费、Morph/Restore/复用历史，原始与补充证据分别保留，引用和入口范围一致。

结果：`PASS_STATIC_DOCUMENT_REVIEW`。上述 7 项整体静态审阅通过，仅适用于本轮 seed/归属最小原始字段登记。

本次唯一整体审阅读取最终研究正文、审阅记录、阶段入口及两个总入口更新，并核对三份独立证据：原始导出含 40 个函数、14 处绑定上下文、24 处指令上下文及 7 类派生 Init 调用组；主智能体复核含 27 个函数和 5 段指令；Grid 补充含 2 个函数，合计 45 个不同函数。函数记录均有伪代码和非空指令，未出现导出错误；三个文档中的 52 处本地文件引用均存在，两个总入口的新引用与这些目标一致。

当前依据支持按对象类别直接复制已登记的 seed/RNG 字节和引用值，不能推出目标对象仍有效、seed 唯一、跨 tick 身份稳定，或全部 Morph/Restore/清理行为已恢复。原始与两份复核材料独立保留；本轮未实现或运行采集器，未操作游戏、构建或执行代码测试，也未重复验收。第三步其它候选字段及后续身份、比较、写回仍未完成。
