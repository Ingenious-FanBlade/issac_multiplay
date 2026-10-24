# tx-03-16：主智能体复核与整体静态文档审阅

日期：2026-10-23（Asia/Shanghai）。本轮目标为独立 Sprite 的最小展示字段与特殊视觉支持边界。用户要求的子任务 `sprite_fields_tx0316` 使用 `gpt-6-luna`、reasoning effort=`max`；主智能体研究动画／Lua 调度、索引与接口后态，并独立导出、阅读 draw 核心和属性访问器后复核子结论。

最终研究见 [正文](tx-03-16-independent-sprite-display-fields.md)，子任务见 [Luna 报告](tx-03-16-luna-sprite-fields.md)。没有实现或运行 Replica，也未将固定姿态候选作为已采纳 wire schema。

## 分工与独立复核

Luna 研究 `409120/4098A0/409CB0/40A0D0/40A140`、直接几何／颜色／资源 helper、属性和 flags；其完整 bytes 与伪代码保存于 [luna-draw-fields.json](../../artifacts/cs-t0/tx-03-16-static/luna-draw-fields.json)。主审单独提取八份原始 JSON，正文逐项链接；直接重读了相同核心、两段调度、十四个共同访问器／helper，而非只接受子报告。

| 子结论／疑点 | 主审处理 |
| --- | --- |
| `409120` 的 receiver | 初步消息称 animation state，主审在 `40A058/40A05E` 看到由 descriptor+18 装 track 表再加 `0x10*i`，确认是 visual track。终稿已纠正，四种整数域分别记录。 |
| track／frame／layer 的可见性 | 接受分支和字段，不将它们混作整体资源成功；无帧／隐藏也可以是有效逻辑描述。 |
| 步进地址 `40A06E` | 终稿一字节定位错误，实际 `40A06F: 83 C3 10`；`40A06E` 是 `inc esi`。已修订并保留记录。 |
| bit `0x01` 为 matrix／transform 选路 | 不采纳。`4071C0` 复制 44-byte Color；主审另导出 `4072E0/6831C0`，确认是 layer Color 与 Sprite／颜色栈选路、再组合 frame Color。已修正子报告。 |
| 44-byte Color | 接受结构大小和 shader 子区来源；不能缩成 4-byte RGBA。主审另读 `407170` 的四个 1／七个 0 初始化，不由此推导所有 Sprite 构造与 shader 默认。 |
| Sprite flip 与 layer flip、负 scale | 接受原始字段与符号组合；区分 Sprite scale 为零造成退化、frame×layer scale 为 crop 除法分母，避免混称同一除零来源。 |
| flags 全零等于无外部依赖 | 不接受该外推。颜色栈、`C7977D/F`、条件 Game offset 和 renderer uniforms 仍存在；子报告列入的 `C7977E` 在已读路线仅 bit `0x04` 才读取，已修正文案。 |
| RNG 与非纯读 draw | 接受 `409AF0/409B7D` 两次写共享 `C33914`，保留其 gameplay 下游未知；没有据此断言不能归为 presentation。 |
| 本地依赖是否都要进网络 | 不采纳直接扩张。资源 track／keyframe 表由声明资源派生，控制块留本地；实例 selected-key 与逻辑姿态才进入候选描述。 |
| caller 参数与默认值 | 保留未知。主审按读取操作提出位置／crop 的受限候选，但未取得全部上游坐标系或通用默认值。 |

在此基础上主审补齐 main／overlay 具名方法和实际 Lua closures：native 地址、Lua 参数转换和返回结果数分别记录；不是把 native 成员当成 lua_State 函数。按名 SetFrame 强制 state reset，单参数 SetFrame_1 不停止 playing；overlay 按名设置同样因 reset 而停止，不能只比较末尾有没有显式写零。逐层 key 覆盖不写整体帧；固定姿态若只传整数 getter 就会丢失该信息。

主审还沿 `408970` 原始指令记录非 reset 切换的旧 count 复制长度和第二数组写错字段的条件异常，以及 SetLayerFrame 的非法映射诊断后继续数组写入。都是当前 IDB 的有条件静态问题，没有执行非法调用、测量内存破坏或宣布发生崩溃。

## 设计结论的限定

固定姿态候选不调用 Sprite.Update，区分资源派生表、实例姿态和本地 renderer 环境；不将 event masks 变成 Replica 裁决输入。保守特殊 flags 全零还需要定义颜色栈、镜像／额外 quad 分支、视口与 Game offset。这个支持描述允许显式隐藏／无帧，不能因不支持某效果就静默清 bit 并宣称画面等价。

方案没有提供原生数组随意写入资格、完整 setter 成功契约、构造／资源／姿态的事务回滚或通用 shader／Player 外观支持。下一轮研究正常 owner 的创建、资源 staging、姿态应用与失败退休顺序，之后再挑有限集合实现；本轮不继续扩成全引擎闭包。

## 采集限制与可复现性

主审请求 `408930` 时实际得到包含它的 `408830` reset 函数，不是另一个入口；请求 `407D30` 时实际得到 `407AE0` 的整段 swap，未将它认定为构造器或用于候选调用顺序。这两个请求及实际起点保留在 JSON；本轮正文不把 requested_va 当可调用入口。

有些终端大输出被截断，完整 JSON 保留并按选择的函数／指令继续阅读，没有据截断认定代码缺失。Luna 的目标 xrefs 设最多 256 项并记录截断字段，不能当作动态调用全枚举。直接函数提取没有报告失败；这不证明没有其它程序失败路径。上述记录见 [primary-acquisition-notes.json](../../artifacts/cs-t0/tx-03-16-static/primary-acquisition-notes.json)。

只读提取脚本为主审 [export_static.py](../../artifacts/cs-t0/tx-03-16-static/export_static.py)和 [luna-export-sprite-fields.py](../../artifacts/cs-t0/tx-03-16-static/luna-export-sprite-fields.py)。主审支持 `TX0316_OUTPUT` 与函数／ranges 等选择集，子脚本支持 `TX0316_LUNA_OUTPUT`；均 create-new。增加复现参数及修订默认选择集后未重跑旧导出。没有执行任何原生 helper、修改 IDB／生产代码、计算或检查哈希；没有用文件路径证明 IDB 与磁盘相同。

## 唯一一次整体静态文档验收

全部研究、子结论复核和文档编辑完成后，仅执行一次 [review_documents.ps1](../../artifacts/cs-t0/tx-03-16-static/review_documents.ps1)。统一审阅三份报告、四个入口、九份原始 JSON 与采集记录，检查新引用、VA/RVA、两处内部地址请求的分类、关键 bytes 和双方独立导出的共同函数。结果写入 [final-static-review.json](../../artifacts/cs-t0/tx-03-16-static/final-static-review.json)，失败保留且不自动重跑。

脚本只检查静态交付的一致性；语义判断来自上述读码，不代表姿态、像素、线程、context、资源加载或运行时恢复通过。本轮未启动／附着游戏或调试器，未进行注入、断点、游戏输入、构建、单元测试、E2E 或其它实验；未触及原版 Online／Isaac Sync 会话。
