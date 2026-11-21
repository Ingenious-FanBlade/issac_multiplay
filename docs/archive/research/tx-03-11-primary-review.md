# tx-03-11：主智能体复核记录

日期：2026-10-22（Asia/Shanghai）。本轮用户要求进一步逆向游戏逻辑／展示分离，并明确指定 GPT-6 Luna、max 推理强度的子智能体。实际创建任务为 `render_boundary_reverse`，模型标识 `gpt-6-luna`、reasoning effort `max`；由主智能体读取当前 IDB 独立核对后形成 [最终研究](tx-03-11-logic-presentation-separation.md)。

## 分工与直接审阅

Luna 研究 Room `0x80EA80/0x80F200`、状态展示 `0x832850`、EntityList／虚表分派，交付 [子报告](tx-03-11-luna-render-findings.md)和 `luna-*` 原始 MCP 返回、只读导出脚本。主智能体研究动画帧／事件／gameplay 消费者，并独立重新提取 Luna 的关键函数、指令和 vtable bytes。双方共享 IDB，只读；没有并发控制游戏或调试器。

以下是审阅后的接受范围，不把子报告摘要代替原始依据。

| 子智能体结论／审阅点 | 主智能体直接依据 | 审阅决定 |
| --- | --- | --- |
| Entity `+0x18` 目标为 `0x6B2000`，带坐标参数 | [primary-entity-vtable.json](../../../artifacts/cs-t0/tx-03-11-static/primary-entity-vtable.json) 的 `+0x18=00 20 6B 00`；[primary-render-review.json](../../../artifacts/cs-t0/tx-03-11-static/primary-render-review.json) 的 `0x419C61..67` 及 `0x80EAE0..F8`。 | 接受；只属于该 EntityList pass，不泛化为全部实体绘制或所有派生目标。 |
| 基类该函数直接写 Sprite 并在绘制后恢复 | 主审 `0x6B2000` 全函数和 `0x40A140` 包装层。 | 接受；layer active 最后写 0，并未保存任意旧 active；只证明正常路径的部分字段恢复，不外推异常／重入。没有直接 Position／HP 写入不等于深层纯展示。 |
| `0x80F200` 中相同槽可能属于不同对象 | [primary-room-render.json](../../../artifacts/cs-t0/tx-03-11-static/primary-room-render.json)：`0x810496` ECX 来自 `0xC7984C`，`0x810D0D` ECX 来自 `Game+0x1BB78` 向量。 | 接受；前者是图形对象调用，后者动态类型／target UNKNOWN，不统一命名为 Entity。 |
| `0x832850` 的 `+0x1C` 是资源对象绘制路线 | 主审重新导出该函数全体指令／伪代码，见 primary-render-review。 | 接受直接 receiver 与参数判断；保留动态实现、资源生命周期及下层副作用 UNKNOWN，不能将它与更新函数 `0x8318A0` 混淆。 |
| Room／FSM 直接 call inventory 中未见 Lua 命名目标 | 对应当前 IDB 直接调用范围；主审另重新提取 `0x863680/0x872980`。 | 限定接受为 inventory 事实。render child 外层确实另有条件 `lua_pcallk`，间接目标未闭合；不允许据名字缺失得出无 Lua，也不称已证任意用户脚本执行。 |
| 子导出脚本初稿用 `py_eval` 执行含函数定义的整份脚本 | 工具说明明确 `py_eval` globals／locals 分离，而 `py_exec_file` 为单一共享 namespace。 | 已修正为 `py_exec_file` 与 create-new 落盘入口，避免函数引用不到 imports／常量；不覆盖子智能体原始分次调用与 NameError 记录。 |

本轮主智能体另外确认 `0x4199D0` 的 Entity `+0x14`／`+0x34` 分派，基类 `+0x14=0x6B10F0` 的绘制坐标缓存写入，以及 pass 5 的 `PositionOffset` 临时改写／恢复。证据见 [primary-entity-main-render.json](../../../artifacts/cs-t0/tx-03-11-static/primary-entity-main-render.json)与 [primary-entity-draw-leaves.json](../../../artifacts/cs-t0/tx-03-11-static/primary-entity-draw-leaves.json)。这补充了子智能体主要追踪 `+0x18` 的范围，不将两个 pass 混成单一边界。

主审还展开 [primary-sprite-helpers.json](../../../artifacts/cs-t0/tx-03-11-static/primary-sprite-helpers.json)：`0x40B220` 的合法索引分支返回 layer 项；非法索引可能触发全局后备对象构造与 `atexit` 注册。`0x407170` 仅直接初始化传入的 11 个 DWORD。本轮据此补上资源有效性限制，没有把整个 helper 的异常分支归类为只读。

## 动画结论的独立依据

主智能体从 `0x408E00` 继续追 `0x408C90`，确认目标帧设置会 OR 当前／历史事件位。`0x409030` 按 speed 推进到 `0x408D00`；`0x40ADD0/0x40AE70` 按名字读取对应两组位；随后追出 `"Shoot" → 0x6CB2E0 case 0 → Type 9 → 0x428B20 → 0x6FE410`。全部为静态可达，未作为运行时事件记录。

关键证据：[primary-animation.json](../../../artifacts/cs-t0/tx-03-11-static/primary-animation.json)、[primary-animation-consumers.json](../../../artifacts/cs-t0/tx-03-11-static/primary-animation-consumers.json)、[primary-event-gameplay.json](../../../artifacts/cs-t0/tx-03-11-static/primary-event-gameplay.json)、[primary-shoot-callee.json](../../../artifacts/cs-t0/tx-03-11-static/primary-shoot-callee.json)与 [primary-spawn-wrapper.json](../../../artifacts/cs-t0/tx-03-11-static/primary-spawn-wrapper.json)。

`0x408C90` 的事件位写入不等于直接执行 Lua／callback；世界创建需要下游消费者继续运行且条件满足。研究中另读到 `0x50A6B0` 对 offset 876 的乘写，该 offset 是 `0x36C`，不是公共 Velocity `0x360/0x364`；没有将它命名为速度或作为错误的因果依据。`0x4212C0` 的 Manager 写入也没有被命名为存档成功。没有新增运行时观察，没有使用静态未见来证明不存在。

## 原始记录、复现与限制

主审导出由 [export_primary_static.py](../../../artifacts/cs-t0/tx-03-11-static/export_primary_static.py)完成，逐函数保存完整伪代码、原始指令 bytes、代码引用、当前 IDA xrefs 和地址。使用 `runpy.run_path` 的命名参数指定其它函数／范围／虚表与新输出，create-new；原始 JSON 中的 requested_va、ranges、tables 即对应选择集。不能覆盖旧输出，也不计算／校验哈希。工具传输截断和一次过量 namespace 返回已在正文如实记录，完整数据以落盘导出为依据。

主审修正 Luna 导出入口后执行了一次只读静态资料提取，将 9 个函数、虚表、关键上下文、直接调用目录及候选写入导出为 [luna-render-export.json](../../../artifacts/cs-t0/tx-03-11-static/luna-render-export.json)，工具返回成功。该操作读取 IDB，不执行目标程序，也不作为额外测试验收。原始子智能体的分次输出继续保留。

IDB imagebase `0x400000`、路径与输入元数据限定本次静态研究，未证明 IDB 与磁盘任意构建一致。没有 patch、rename、set_type、保存 IDB；没有启动、附着、注入、输入或修改任何目标进程。没有修改生产源码，也没有构建、E2E、单元测试或其它实验。

## 唯一一次整体静态文档验收

全部分析与修订结束后，统一入口为 [review_documents.ps1](../../../artifacts/cs-t0/tx-03-11-static/review_documents.ps1)，一次读取最终正文、本主审、Luna 子报告、受影响索引与原始 JSON，检查新增本地引用、JSON 完整性、关键指令／表项及 VA/RVA 换算；结果保存在 [final-static-review.json](../../../artifacts/cs-t0/tx-03-11-static/final-static-review.json)。其中语义评判依据是以上人工静态分析；脚本只检查交付的一致性，不证明目标程序行为。结果不作为项目 gate 或后续推进准入条件。

验收仅评价 tx-03-11 的静态研究交付；Replica、baseline/delta apply、完整安全展示闭包、线程／异常／重入与 Hook 安装恢复的运行时能力未在本轮实现或验证。
