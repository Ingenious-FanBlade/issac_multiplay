# tx-03-13：主智能体复核与整体静态文档审阅

日期：2026-10-22（Asia/Shanghai）。用户任务为“开始 Sprite 图像资源逆向任务”；沿用用户要求的 GPT-6 Luna / max 子智能体及主智能体审阅。实际子任务为 `image_control_tx0313`，模型 `gpt-6-luna`、reasoning effort=`max`。研究结果见 [正文](tx-03-13-sprite-image-resources.md)，子任务见 [Luna 报告](tx-03-13-luna-image-control.md)。本轮没有实机测试或 adapter 实现。

## 独立复核范围

Luna 负责 `ImageBase` control、pair 的 retain／release 与 Mutex；主智能体负责 loader、两个图像缓存、全局释放回调和 PNG 图形后端。主审没有仅引用子智能体摘要：从当前 IDB 重新导出 control 六个目标、构造、pair 替换、layer 接口、锁入口及两张表，形成 [primary-image-control-review.json](../../../artifacts/cs-t0/tx-03-13-static/primary-image-control-review.json)，并对照 [luna-control-core.json](../../../artifacts/cs-t0/tx-03-13-static/luna-control-core.json)读取指令。

| 子任务结论／容易误读之处 | 主审证据与处理 |
| --- | --- |
| 24 字节 control 与强／弱计数 | 接受 `+0x04/+0x06` 两个 u16、内嵌 Mutex `+0x08`、image `+0x14`，计数由 `0x40CC42` 一次写 `0x00010001`。强／弱命名依据销毁行为，未把 RTTI 当作所有并发语义证明。 |
| control 六槽与 Image 虚表 | 接受 control 表 `0xB1A6E0` 六个目标；另查 PNG 表 `0xB9FF84`。control 的 `+0x0C`、Image 的 `+0x0C` 与 Mutex 的 `+0x0C` receiver 不同，不能混用。 |
| `+0x04` 可成功增加强引用 | 原始 `0x40C6D6` 先解锁，`0x40C6DC` 再调 `+0x08`，后者重新加锁。限定为非并发正常路径；不接受“原子 retain-if-alive”外推。 |
| `+0x0C` 的返回值 | 依据 `0x40C630` 所有分支，接受 AL 是 control 仍存活的正常约定；原 strong=0 也返回 1，strong 1→0 则删 image 再减弱计数。不能把它解释成“资源已成功删除”或“回调应删除 image”。 |
| Image 最后释放 | `0x40C666` 确认调用 image `+0x34(1)`；主审另定位 PNG 的该槽为 `0xA649D0`，直接删除纹理。动态 receiver 若非 PNG 仍须读其实际表，未宣称所有派生析构相同。 |
| control 最后释放 | `0x40C5F1` 调 control `+0x00(1)`，正常返回 AL=0；`0x40CB50` 删除同步状态、按 flag 释放 control。接受 image／control 两阶段，未套用自有 virtual device 的 NEVER 回收策略。 |
| Mutex 足以证明线程安全 | 不接受此推断。读到 CRITICAL_SECTION 与额外 busy byte、分离加锁及 16 位计数；完整引用持有、并发最后释放、重入／shutdown 尚未解释。 |
| `0x408640` layer 赋图像 ABI | 原始 ECX=layer、栈上 8 字节 pair，末尾 `C2 08 00`，函数内销毁该栈 pair。以此限定伪代码表面“两个参数”，不发布裸指针签名供实现直接调用。 |
| 全局 `0xC7163C` 回调 | 主审另取赋值、回调与缓存节点删除证据：正常初始化写 `0xA12240`；它只在 residual strong==1 时调用 `0xA121B0` 尝试摘缓存，实际 image delete 在 control 的零强计数分支。 |
| 子报告初稿误写调用点／pair 身份 | 初稿把 `FF 50 34` 写在 `0x40C662`，实际调用点为 `0x40C666`；`0x408640` 尾部销毁的是栈上源 pair，旧目的 pair 在 `0x40C3B0` 内释放。已修正并在子报告保留修订记录；删除 flag 限定为 bit0，回调锁内读／解锁后比较也已澄清。 |

上述关键限定已在主审与子任务消息中反馈，并合入正文。子任务原始 JSON 的“动态 Image+0x34 与全局回调目标运行时相关”限定仍保留：本轮证明的是给定 PNG 表项和静态赋值路径，未读取实际运行时指针。

主审最终逐段读过子报告后才执行整体静态文档验收。子报告局部仍保留缓存释放完整路径 UNKNOWN；主审已在正文补上 `A121B0 → 414D60 → control+0x0C` 的正常路径，重入／并发和完整调用者集合继续未知。没有把子报告未研究的部分误算成它已独立完成。

## 主智能体的资源研究

- [primary-image-initial.json](../../../artifacts/cs-t0/tx-03-13-static/primary-image-initial.json)：Sprite graphics、path wrapper、Sprite 图像索引与 pair helper，含 `0xC7163C` 当前 IDA xrefs。
- [primary-image-loader.json](../../../artifacts/cs-t0/tx-03-13-static/primary-image-loader.json)：`0xA118D0` 图像 loader 与资源／Mod 路径解析；保留原始第三栈词，不用猜测补签名。
- [primary-image-release.json](../../../artifacts/cs-t0/tx-03-13-static/primary-image-release.json)：缓存查询／插入、key 更新、`0xA12240` 与 manager 赋值／清零。
- [primary-image-retire-context.json](../../../artifacts/cs-t0/tx-03-13-static/primary-image-retire-context.json)：`0xA121B0` 摘除、backend／window 初始化及关闭路线。没有由函数名／GLFW 字符串认定实际当前线程。
- [primary-image-tables.json](../../../artifacts/cs-t0/tx-03-13-static/primary-image-tables.json)：PNG／Platform 表与 Image 构造；选择性深入 PNG，不宣称其它 decoder 已审完。
- [primary-image-png-lifecycle.json](../../../artifacts/cs-t0/tx-03-13-static/primary-image-png-lifecycle.json)：PNG 解码、loaded bit、清理与加载完成入口。
- [primary-image-gpu.json](../../../artifacts/cs-t0/tx-03-13-static/primary-image-gpu.json)：PNG 上传、删除析构、sampler GL 调用、基础清理及缓存节点 pair 销毁。

主审特别区分四层成功含义。Sprite+`0x109` 是 Load 到尾部置位；layer+`0x30` 接受 image 逻辑 loaded；Image+`0x08` bit0 由加载完成函数设置；PNG 上传 helper `0xA69B30` 却在 GL 调用后固定返回 AL=1。因此本轮能证明加载链调用了纹理上传，不能证明驱动完成或图形资源有效。没有将 false 分支、longjmp 清理片段扩写成完整 OOM／EH 恢复。

另一个直接影响是 `0x408590` 虽返回图像 pair，仍可能先经 sampler 槽改变共享 image 和 GL 状态。独立 Sprite 既不等于私有图像，也不等于取图像只读。关于 context／线程的设计限制来自这些直接调用，而非一次真实线程采样。

## 方法与采集记录

主审只读提取脚本 [export_static.py](../../../artifacts/cs-t0/tx-03-13-static/export_static.py)，子任务脚本 [luna-export-control.py](../../../artifacts/cs-t0/tx-03-13-static/luna-export-control.py)。主审通过 IDA MCP `py_eval` 调 `runpy.run_path`，让脚本使用单一 namespace；`TX0313_OUTPUT` 指定 create-new 路径，函数／表／range／字符串／global-xref 选择集由同名前缀参数配置。子脚本同样支持选择集和新输出路径。复现时应使用新文件名，不覆盖原始证据；脚本只读当前 IDB，不执行原生 key helper 或任何目标代码。

各 JSON 保留 IDB／input path 和 imagebase，不把路径一致当成文件内容相同。全局 `0xC7163C` 静态数据字节为 FF；赋值代码是本文判断回调目标的来源，没有将 FF 当作运行时函数值。当前 IDB 的别名可能显示 `n2_2`，不依赖该名字判断语义。

主审一次 login shell 读取文档虽成功，但 PowerShell profile 的 conda 启动报错；后续使用 login=false。一次读取尚未由子任务写出的报告返回 exit1；等待交付后才审阅文本，未把不存在当作研究否定结论。部分终端大输出截断，按完整 JSON 和小段原始指令阅读。详见 [采集记录](../../../artifacts/cs-t0/tx-03-13-static/primary-acquisition-notes.json)。子任务的工具返回保存在 [luna-control-tool-returns.json](../../../artifacts/cs-t0/tx-03-13-static/luna-control-tool-returns.json)，保留原始成功／失败，不用终端摘要覆盖。

子任务已导出完整函数原始指令后，重复的直接反汇编批次仍长时间等待；主智能体中断该等待，令子任务仅依据已落盘 core JSON 定稿。未完成的调用不形成正面或负面证据，没有为它重试采集，也没有把该中断当作目标程序失败。

本轮双方没有 patch／rename／set_type／保存 IDB，没有修改生产源码，没有游戏输入、启动、进程附着、注入或断点，没有计算／校验任何哈希。没有触及原版 Online 或 Isaac Sync 会话。所有采集都是静态资料分析，不是预先测试。

## 唯一一次整体静态文档验收

研究、主审和文档编辑结束后，仅执行一次 [review_documents.ps1](../../../artifacts/cs-t0/tx-03-13-static/review_documents.ps1)：统一读取三份新报告、三个入口、新 JSON；检查链接、取证完整性、VA/RVA、关键指令与 control／Image 表项，并对照主审独立导出与子任务表项。结果落在 [final-static-review.json](../../../artifacts/cs-t0/tx-03-13-static/final-static-review.json)。语义审阅来自上述人工读码；脚本只评估静态交付一致性，不证明游戏能运行或加载／退出安全。

本轮保留的具体未知是实际 renderer owner 线程／当前 context、图像热替换和 Reload／LoadGraphics 关系、并发／关闭／OOM／全 EH、其它格式与所有 Mod，以及受支持展示对象的实际应用与退休。它们没有被计成静态验收通过的能力，Replica 仍未实现。
