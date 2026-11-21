# tx-03-17：主智能体复核与整体静态文档审阅

日期：2026-10-23（Asia/Shanghai）。本轮实质目标为独立 Sprite 正常创建、资源 staging、固定姿态应用和失败退休的 owner 顺序。用户指定的子任务 `sprite_retire_tx0317` 使用 `gpt-6-luna`、reasoning effort=`max`；主智能体研究构造／copy／swap 与调用者，独立提取退休、GC、control 后复核子结论。最终见 [正文](tx-03-17-independent-sprite-owner-lifecycle.md)及 [Luna 报告](tx-03-17-luna-sprite-retirement.md)。

## 独立研究与子结论复核

Luna 的原始证据为 [luna-retirement-static.json](../../../artifacts/cs-t0/tx-03-17-static/luna-retirement-static.json)。主审八份原始 JSON 在正文逐项列出；重新读取了共同的十二个函数和 userdata 表项，并对照原始 bytes、ECX／EDX／栈参、释放顺序及字段 owner。

| 子结论／问题 | 主审处理 |
| --- | --- |
| Reset、native dtor 与 GC 的差别 | 接受。GC 真实走 `8C31F0 → wrapper slot0 → 8C4CE0(flag=0)`，而非调用 `408170`；原生 dtor 不释放外层。 |
| userdata 分配和构造顺序 | 接受 `0x11C` 外层、`+8` payload、`0x114` payload size；metatable 先于 ctor 完成，不能外推部分构造 GC 安全。 |
| `408170` 无直接 code xrefs | 初步消息的否定不采纳。后续 child 发现 tail jmp／unwind stubs，终稿已撤回；这些 thunk 同样不能证明 raw allocation owner。 |
| Offset／Scale 默认值 | 接受来自全局而非固定 0／1；主审沿 ctor→Reset 确认。rotation、Color、flip、speed、order／loaded／flags 的直接写入另列。 |
| Color 范围／类型 | 早期消息 B8..C8 不能表示七个 DWORD，已改 B8..D0；终稿四个 DWORD=1 修为 f32=1.0、位模式 `3F800000`，避免写成 raw32 数值 1。 |
| layer control offset | 主审修正终稿 +98 control：+98 是 image，control 在 +9C，原始 `407A60` 读 `this[39]`。 |
| 释放依赖 `C7163C` | 不接受该外推。control+0C 自身减引用先执行，可在内部最后 strong 时析构 image；callback 是后置条件调用。主审补取 `40C630/40C5C0/A12240`，与 tx-03-13 的正常结论一致。 |
| 正常失败候选清理 | 接受限定路径：完整构造、本地数组／control 有效、相同主线程/context、cache 查找关系稳定。按 filename 再查 key 不是引用 token，拓扑改变后仍未知。 |
| 旧 F→失败 G 的 borrowed fields | 接受 Reset 不遍历旧 F descriptor／track 表的局部事实；不由此声称修复 F 引用或可继续画旧指针。 |
| Reset 后再 Lua GC | 接受显式 Reset 在已知窗口退休资源，后续未重新加载的空路径 GC 不再次释放旧图像；不宣称任意 GC 都与 context 无关。 |
| Reload 计数 | 接受仍只能减一次，不能靠 Reset 配平 k 次 Reload 的全部额外 ref；方案避免循环 Reload。 |

主审新增实际 owner 转移的研究：`407690` 复制构造深拷贝索引与层、retain 资源，却将目标 Offset／flags 清零；`407AE0` 修复双方 layer owner，但不交换 Offset 和 overlay 顺序。`407E90` 的 `ret 0x114` 及 `8C0B00` 的 copy→swap→清临时说明按值内容替换的正常合同，不提供完整 pose 的原子提交。`753AD0` 还有 payload 外的派生字段，不能用于仅有 Sprite payload 的内存。

据此提出未发布新实例 staging、完成后切换自有显示关联、旧实例退出使用后退休的候选。它不修改 live Entity／Room，不假设 cache 完全私有，也未实现 registry、资源就绪检查、姿态写回或异常恢复。native raw 存储由自有 allocator 配对只是设计要求，不能从析构体反推出一个原生分配 owner；Lua userdata 则始终归 Lua 外层生命周期。

主审还独立读 `AEF15C/A0F500`，确认子报告的 native 释放调用链到 `A648B0`；原始寄存器保留实际指针转发，不按伪代码零大小参数省略指针。未扩展成全部 heap allocator 的恢复合同。owned userdata 的结论限定于 Sprite() 创建的 wrapper，不覆盖其它来源包装的所有权。

## 采集限制与历史保留

主审最初将 `B73228` 读为两个 pointer words。第一个才是 slot0=`8C4CE0`；第二个 `B7322C` 的 bytes=`49 73 42 75`，来自相邻 `IsBusted` 字符串，不是指针。原始 [primary-retirement-review.json](../../../artifacts/cs-t0/tx-03-17-static/primary-retirement-review.json)保留机械生成的无意义 target 地址／RVA。主审随后只读取正确的一槽，并将相邻数据按名字／字符串字节记录在 [primary-userdata-owner.json](../../../artifacts/cs-t0/tx-03-17-static/primary-userdata-owner.json)。最终审阅显式分类该过读，不当作第二函数槽。

少量终端输出因大小截断，完整 JSON 保留，后续按函数／指令选择继续阅读；没有据截断推断代码不存在。取证没有改 IDB、重命名、patch、执行 target helper 或计算哈希。构造、copy、swap 的 EBP 反编译伪参数不直接作为 ABI，原生调用仍以 raw 为准。额外限制见 [primary-acquisition-notes.json](../../../artifacts/cs-t0/tx-03-17-static/primary-acquisition-notes.json)。

只读可复现脚本为 [export_static.py](../../../artifacts/cs-t0/tx-03-17-static/export_static.py)与 [luna-export-retirement.py](../../../artifacts/cs-t0/tx-03-17-static/luna-export-retirement.py)，支持各自新输出路径并采用 create-new。编辑默认选择集后不重跑旧导出，不覆盖原始观察。

## tx-03-16 审阅器分类修正

上一轮实际结果仍是 [FAIL，125/127](../../../artifacts/cs-t0/tx-03-16-static/final-static-review.json)，[失败说明](../../../artifacts/cs-t0/tx-03-16-static/final-review-failure-notes.md)与脚本均保留。两项失败来自裸斜杠规则把对象字段／并列 VA／flags 当作 VA/RVA，不是新的运行时故障。本轮没有重跑那次 127 项审阅。

新的审阅器只提取显式 inline RVA 标签，或明确标有 VA、RVA 的表列／VA-RVA 复合列。字段表、并列 getter／setter 和 flags 不因出现 slash 就成为地址对。旧两份文档作为这项分类修正的实际阅读样例纳入本轮唯一验收：主文应识别 11 个具名 native 对，Luna 文应识别 3 个显式 inline 对。其余数值继续作为对象布局或各自 VA 阅读，不静默丢弃一个有明确 RVA 标签的错误。

该修正随本轮真实 owner 研究交付，不通过另开空 tx 重复旧验收，也不把旧 FAIL 改成 PASS。

## 唯一一次整体静态文档验收

全部研究、主审与文档编辑结束后，仅执行一次 [review_documents.ps1](../../../artifacts/cs-t0/tx-03-17-static/review_documents.ps1)。统一读取三份报告、四个入口、九份原始 JSON 与采集记录，检查引用、明确地址语义、关键 bytes、十二个共同函数与 userdata 表项，并纳入上述分类样例。结果为 [final-static-review.json](../../../artifacts/cs-t0/tx-03-17-static/final-static-review.json)，失败原样记录且不自动重跑。

静态文档审阅只检查交付一致性，语义判断来自前述人工读码，不代表游戏运行、线程/context、像素、资源成功、内存回收或异常恢复通过。本轮未运行游戏、构建、单元测试、E2E 或其它实验，未启动／附着调试器、注入、设断点或写世界；未触及原版 Online／Isaac Sync 会话。raw storage、readiness predicate、pose adapter 和可玩 Replica 仍未实现。
