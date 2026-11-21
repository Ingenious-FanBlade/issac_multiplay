# tx-03-15：主智能体复核与整体静态文档审阅

日期：2026-10-23（Asia/Shanghai）。用户按上一轮建议继续 Sprite 换图／重载逆向。本轮完整目标是解释三个入口、ANM 重绑定／引用及普通失败后态；只做静态研究和文档。用户指定的子任务 `sprite_replace_tx0315` 使用 `gpt-6-luna`、reasoning effort=`max`；主智能体已阅读其终稿和原始证据，并独立重取关键函数。

最终结论见 [研究正文](tx-03-15-sprite-replacement-reload.md)，子任务见 [Luna 报告](tx-03-15-luna-sprite-replacement.md)。

## 分工与独立证据

Luna 研究 ReplaceSpritesheet、路径比较、逐层 LoadGraphics、pair ABI 和标志；主智能体研究 Reload／Load／Reset、ANM 绑定与计数、失败条件，再独立导出 Replace、LoadGraphics、pair 与比较 helper。主审未以子报告的文字作为唯一依据。

主审证据文件按正文表列的六份 JSON 提供，均位于 [tx-03-15-static](../../../artifacts/cs-t0/tx-03-15-static)。子任务原始文件为 [luna-sprite-replacement.json](../../../artifacts/cs-t0/tx-03-15-static/luna-sprite-replacement.json)和 [luna-sprite-path-compare.json](../../../artifacts/cs-t0/tx-03-15-static/luna-sprite-path-compare.json)。主审对照原始寄存器、栈清理、bytes、分支和 owner 后作如下处理：

| 子结论／问题 | 主审处理 |
| --- | --- |
| Replace 只改实例路径并清 `+0x30` | 接受。核对有效 ordinal、`0xA0` stride、string 布局、不同内容赋值及 AL／ret 8；返回 true 不是加载成功。 |
| 同路径无法强制重载 | 接受。比较 helper 先于自 string 别名分支；原标志为零仍能让后续 LoadGraphics 重试，不能一概称为无法重试。 |
| LoadGraphics 的 `+0x30/+0x31` | 接受逐层 skip／结果写入；补充结果来自最终关联前的候选检查，不保证 retain 后目的 pair。`+0x31` 语义仍未知。 |
| 失败不恢复旧 pair | 接受正常空 image／loaded=false 返回的有限结论；空 image 可能仍有 control，不概括为两个 DWORD 都是零。 |
| `408640` 的 pair 释放 | 修正文案：`40C3B0` 释放旧目的 pair，`408640` 尾部另销毁按值 source pair；不能混称一次旧 control 释放。 |
| “此路径未发现另一个 Lua 层 wrapper” | 不采纳。主审进一步导出 `89EF10/89F030`，实际安装 `8A99D0/8A9740` closure；补取两者完整函数，确认参数转换、一次 native 调用及结果数量。已修订子报告并保留原结论的修订记录。 |
| Reload 与 ANM 引用 | 不属于子任务结论。主审独立闭合正常绑定、layer 析构、计数增减、Reset、零引用 sweep 及 Lua closure，提出限定条件的额外引用推导。 |
| Reload／Load 的动画和分类状态 | 主审区分：Reload 重建 layer 但不执行 Load 的两套 state reset 和分类扫描；不据此宣称结构变更后旧状态有效。 |
| 失败和 draw 可见性 | 主审补充未创建 G 节点时的 Load 失败反例，以及 draw helper 的有限路径；实际像素／崩溃均未观察。 |

原生 callback VA 与 Lua closure VA 被分别记录，VA/RVA 按 `0x400000` 换算。没有从反编译器的 EBP 伪参数猜测额外 native 参数；`40C000` 当前未使用的栈词仍按 `ret 4` 记录。

## 推导限制与修订记录

Reload 额外引用仅在同一稳定缓存节点、正常 helper 返回、无外部平衡／并发等条件下成立。Load 失败的借用风险要求 F 最后引用减少、不同且未命中的 G 节点、open 正常返回空及后续 F sweep；不把它写成必然或已观察的运行时 UAF。原地 ANM 重解析会先清旧 payload，但所有异常、别名消费者与并发不在本轮闭包中。

主审另读 draw helper，发现条件性写全局 `C33914` 的 quad 扰动。没有证明其为 gameplay RNG，也没有为此扩大成全引擎 RNG 研究。本轮下一项收敛为最小展示字段及特殊标志支持边界。

子报告修订包括：保留原先“未发现 wrapper”的范围不足说明并给出实际 closure；区分两种 pair owner 的释放；补充 `+0x30` 为关联前结果。原始 JSON 没有改写。采集过程个别终端输出被截断，改为选择函数／指令继续阅读，完整 JSON 保留；没有把截断当作缺少代码。详见 [primary-acquisition-notes.json](../../../artifacts/cs-t0/tx-03-15-static/primary-acquisition-notes.json)。

可重复取证脚本为主审 [export_static.py](../../../artifacts/cs-t0/tx-03-15-static/export_static.py)、子任务 [luna-export-sprite-replacement.py](../../../artifacts/cs-t0/tx-03-15-static/luna-export-sprite-replacement.py)及 [luna-export-sprite-path-compare.py](../../../artifacts/cs-t0/tx-03-15-static/luna-export-sprite-path-compare.py)。支持各自输出 override 并采用 create-new；脚本在 IDA 读取函数，不执行目标 helper、不 patch／rename／set_type。输入路径与 IDB 相同只作为资料定位，不声称文件身份相同；没有任何哈希计算或校验。

## 唯一一次整体静态文档验收

全部研究、主审与编辑完成后，只执行一次 [review_documents.ps1](../../../artifacts/cs-t0/tx-03-15-static/review_documents.ps1)。统一审阅三份报告、四个入口、八份原始 JSON 和采集记录，检查引用、VA/RVA、关键原始指令与双方独立导出的共同指令；结果写入 [final-static-review.json](../../../artifacts/cs-t0/tx-03-15-static/final-static-review.json)，保留失败且不自动重跑。脚本只检查静态交付一致性，语义结论来自前述读码与限定推导。

本轮未修改生产代码、未启动或附着游戏／调试器、未注入／断点／游戏输入／构建，也未执行其它测试或实验。未触及原版 Online／Isaac Sync 会话。没有实现可玩 Replica、资源 adapter、native rollback 或完整失败恢复。
