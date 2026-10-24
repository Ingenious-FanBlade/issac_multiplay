# tx-03-19：主审复核与一次整体静态文档验收

日期：2026-10-23（Asia/Shanghai）。本轮目标为最小独立 Sprite 的实际绘制接入窗口、guard 数据来源、参数 ABI、绘制状态及批次寿命。按用户的明确要求使用 `/root/sprite_window_tx0319` 子任务，模型 `gpt-6-luna`、reasoning effort=`max`；主智能体独立研究并审阅其结论。

入口：[现行规范](../refactor/project-conventions.md)、[主报告](tx-03-19-sprite-render-entry-window.md)、[Luna 报告](tx-03-19-luna-render-window-guard.md)。本轮无运行时观察，不实施 Hook／adapter，也不触及原版 Online 或 Isaac Sync。

## 独立复核过程与结论

主审独立导出 [context guard 证据](../../artifacts/cs-t0/tx-03-19-static/primary-context-guard-review.json)与 [启动／TLS 细节](../../artifacts/cs-t0/tx-03-19-static/primary-guard-detail.json)，重读 `_main`、`9AA040`、`8FB120`、`A18E90`、`A19DF0`、`A6A420`、`A6BBB0`、`A6B570`、`A7FA50`、`A7FCE0`、`A80010`；另在 [render pass 证据](../../artifacts/cs-t0/tx-03-19-static/primary-render-pass.json)独立获取 `9555C0`。这些共同函数的 bytes 直接比较属于最后一次统一静态审阅；此前仅作只读资料分析，没有执行其代码。

| 子任务结论 | 主审处理与限定 |
| --- | --- |
| 普通 RenderHook 只包裹主循环 callsite | 采纳。源码槽 7 为 RVA5312F1，加载 `8FC32C` 是同一目标的另一个直接 caller；不是函数入口 detour。`9AAAE3` 属启动函数 `9AA040`，报告表头已明确 caller 层级 |
| phase/depth 可以用于判别来源 | 只作必要线索。源码先构造 PhaseScope，未 armed／线程不明时仍可有 Render 标签；InvokeVoid 不调用完整 CompanionHealthy。加载／原生重入也可能继承 scope，需未来自有 outer token／draw depth／代次与 loading 状态 |
| learned 主线程可以供 guard 比较 | 采纳为源码能力。LoopClockHook 学习，RenderHook 不自行学习；实际非零 ID、当前线程和 healthy 值本轮均没有读取或运行观察 |
| TLS current-window 等于 renderer window 就可 draw | 不采纳为充分条件。需正常 WGL owner、有效对象与 context generation，并分别读取当前线程实际 HDC／HGLRC；静态相等不能证明句柄未复用或所有回调后仍 current |
| `A6BBB0` 可用来获取 current | 明确排除。它先读 TLS 后可能解绑／绑定；`A19DF0` 及初始化辅助同样不能当纯 getter调用。未来 guard 使用正常配置下的 TlsGetValue 和已解析 WGL current getter，不主动 MakeCurrent |
| 初始化 flag 可供 guard 使用 | 精确拆分。`A190F9` 置 C37984 bit0 属资源/cache 状态；`A19102: 83 4E 04 01` 才置 renderer+4 bit0。二者都不证明 OS current；子报告已修正表述 |
| 最终 pre-present 窗口 | 采纳为屏幕叠加设计。候选 callsite 为9560D0，合成调用在其前；要求target0和受支持pass，尚未提供房间遮挡或实际像素成功 |

主审核对 `A7FD99/A7FDAB` 的函数表写入和 `A7FE68/A7FE71` 的调用，确认 C75D50 为 wglGetCurrentDC、C75D54 为 wglGetCurrentContext，而不是 HGLRC／HDC 本身。静态字段 FF 或符号名不作运行指针。WGL setter 成功写 TLS、失败处理和普通初始化的 temporary current 路线均不外推为每次 draw 的 guard 已实现。

## 采集失败、内部地址与文档分类

子任务首份 [luna-render-window-guard.json](../../artifacts/cs-t0/tx-03-19-static/luna-render-window-guard.json) 保留两类采集问题：requested `A804B0` 实际归属函数 `A80010`，不能计为另一个函数；requested `A8FB7F` 没有 IDA 函数定义，保存 `No IDA function at requested address`，不是成功函数导出。修正的 [luna-guard-followup.json](../../artifacts/cs-t0/tx-03-19-static/luna-guard-followup.json) 以 12 个真实唯一函数为主证据，并把相关非函数指令放在 range 中；原始失败不覆盖，不把它称为运行或验收失败后重跑。

主审独立原始范围看到 `A8FB7F: A3 E0 35 C1 00`，是将 resolver 结果写入 epoxy_wglGetCurrentContext 指针的指令，后续间接跳转；不是一个 TLS getter。主审范围还保留附近下一段首指令作为上下文，不据此认定完整函数边界。

子报告混合字段表原表头写“VA / RVA”，其中 C74B80／C74B84 等实际是两个字段 VA。主审已改成“VA 或对象字段”，避免把并列地址误作减基址关系；真正函数／callsite 的 VA/RVA 保留显式标签。审阅器兼容 Markdown bold／code 标记，只从显式标签与列语义提取地址对，不靠裸斜线猜测。

主审资料检索还遇到 Windows 下向 rg 传入未展开的 `docs/research/tx-03-14*` 路径而返回 os error123；随后按准确文件名读取，错误保存在 [采集说明](../../artifacts/cs-t0/tx-03-19-static/primary-acquisition-notes.json)。该检索错误不冒充缺失研究，也没有对磁盘／资源执行实验。

## 主线新增结论

主智能体研究了 [批次提交](../../artifacts/cs-t0/tx-03-19-static/primary-submit-flush.json)、[批次／shader](../../artifacts/cs-t0/tx-03-19-static/primary-batch-ownership.json)和 [batch 绑定](../../artifacts/cs-t0/tx-03-19-static/primary-batch-bind-state.json)，将接入时段收敛到具体行为：

- `9560D0: E8 AB 30 0C 00` 前压1、ECX=renderer；原生 `A19180` ret4，EAX 按 original 保留。`A191A0` 先 flush；target=0 才走窗口 swap。外层 RenderHook post 已越过此时刻，pre 又尚未建立本帧 pass；不把二者任意一处称为完整同帧绘制接入。
- `40A0D0` receiver 是 payload，三个栈参数为 position／两组 crop Vec2 指针，ret12；helper 正常返回没有整体成功 bool。首版明确使用本地零 crop、受支持的屏幕逻辑坐标与已准备姿态，不调用 Update／游戏事件。
- `A66E30` 对 image.cachekey 查 graphics cache 并核对同一 image，经 `40C550` 复制缓存 pair、`40C3B0(batch+8)` 提交；正常 batch 有独立强引用，miss／retain 异常不由 Sprite+109 或 pair loaded 掩盖。batch 同时保存 shader 指针及 uniform 版本索引，后续 flush 使用自己的状态记录。
- `A139F0 → A67BD0` 才提交 active batch，`A67E41` 调 GL DrawElements；正常 flush 清 active end／活动位，并不逐个清空 batch 图像 pair。保持到最终原生 helper 正常返回的最小 Sprite owner 顺序是保守设计，不宣称 GPU idle 或立即删除纹理。
- `409120` 正常恢复五个 renderer 状态字后，调用 A14050 选默认 shader，不能恢复任意进入时 shader。uniform、顶点缓冲、批次和逐 quad 深度变化是提交结果的一部分，不能 memcpy 回滚；当前窗口限定已知默认 pass，资源 Load／Reset／析构不混入最终 draw 回调。

部分底层 helper 的反编译器仍给出 EBP／DIL 伪参数或错误常量地址。主报告只对已逐条核对的 Sprite Render、最终 helper、cache→batch pair 与指令角色给出合同，不从这些伪代码推导完整 A671B0／所有 GL／shader ABI。几何复制和 batch state 已知正常路线不代表所有 backend、分配失败、native EH 或外部 raw GL 操作安全。

## 修订范围与后续

新增主报告、Luna 报告、本记录及 `tx-03-19-static` 证据／采集源／审阅入口；更新 docs 总入口、hooks 总入口、Entity、game-loop 和状态同步设计。只读源码摘录见 [primary-loop-source.json](../../artifacts/cs-t0/tx-03-19-static/primary-loop-source.json)。生产代码、当前槽数、机器索引、IDB 和历史失败未改；没有新增 gate、哈希或角色分离前置要求。

接口与所有 runtime guard 仍待实现；实际 thread/context/pass/pixel/GPU、资源清单、连续姿态应用、完整 Replica、房间遮挡、特殊视觉、OOM/EH／worker／热重载仍有各自范围。下一轮选择具体 ANM／PNG、Authority 同窗口姿态采集、连续更新／资源替换及失败顺序，再进入最小 adapter 实现。

## 唯一一次整体静态验收

全部研究、子结论复核和编辑完成后，只执行一次 [review_documents.ps1](../../artifacts/cs-t0/tx-03-19-static/review_documents.ps1)，结果见 [final-static-review.json](../../artifacts/cs-t0/tx-03-19-static/final-static-review.json)。检查本轮三篇报告／五处入口与链接、显式 VA/RVA、证据及范围地址、关键 raw 指令、主／子共同函数的直接字节和源码摘录。语义审阅由上述记录说明；相同 bytes 不替代 runtime 结论。

审阅器将首份导出无函数请求与内部地址归属分类为保留的采集问题，不称它是完整成功导出；followup 则检查 12 个唯一有效函数及其实际记录数量。不重跑旧验收、不校验哈希；若本轮失败则保留结果，不自动重跑。本记录不预先宣称通过。

研究／编辑期间没有 smoke、自测、实验、游戏／调试操作、构建或 API 调用。最终静态审阅是本轮唯一测试验收，不是 Sprite／Replica 的运行验收。
