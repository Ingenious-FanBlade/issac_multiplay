# tx-03-14：主智能体复核与整体静态文档审阅

日期：2026-10-22（Asia/Shanghai）。用户按上一轮建议继续 renderer 线程／GL context 生命周期逆向；本轮范围是正常初始化、帧内分派与关闭的静态路径，不实现 adapter。沿用用户指定的 GPT-6 Luna / max 子任务和主智能体审阅；实际子任务 `renderer_teardown_tx0314` 使用 `gpt-6-luna`、reasoning effort=`max`。最终研究见 [正文](tx-03-14-renderer-context-lifecycle.md)，子任务见 [Luna 报告](tx-03-14-luna-renderer-teardown.md)。

## 分工与独立取证

Luna 从上一轮的关闭种子研究 `A13060/A19130/A71770`、条件缓存清理、平台 window teardown 和 WGL 删除；主智能体研究 `_main`、初始化、make-current／TLS、framebuffer／SwapBuffers、另一个 render 入口。主智能体收到关键关闭地址后，从当前 IDB 独立重导出八个关闭函数及 manager 表，另读 `415800` 的缓存 pair 释放，而非仅按子报告的文字采纳。

主审证据入口如下：

- [primary-context-initial.json](../../../artifacts/cs-t0/tx-03-14-static/primary-context-initial.json)：`A18E90/A19DF0`、窗口辅助、`9555C0`，以及选择性 symbol/string inventory。
- [primary-context-paths.json](../../../artifacts/cs-t0/tx-03-14-static/primary-context-paths.json)：`_main`、`A710A0`、窗口创建与 `A19180`，并保留 `40C200` 的实际空函数体。
- [primary-wgl-current.json](../../../artifacts/cs-t0/tx-03-14-static/primary-wgl-current.json)：WGL make-current／TLS、swap、dummy context 入口装载、context 检查与恢复、启动 wrapper；还读过 `930B20`，其 GetCurrentThreadId 用于崩溃转储，不能当作 renderer 线程认证。
- [primary-wgl-install.json](../../../artifacts/cs-t0/tx-03-14-static/primary-wgl-install.json)：window 函数字段的安装、framebuffer 目标及 GLFW 初始化。
- [primary-render-alternate.json](../../../artifacts/cs-t0/tx-03-14-static/primary-render-alternate.json)：`8FB120` 的加载等待绘制与 TLS global xrefs。
- [primary-window-dispatch.json](../../../artifacts/cs-t0/tx-03-14-static/primary-window-dispatch.json)：平台选择器、Windows create-window callback、另一 backend 的可见分支及 `9AA040` 调用者。
- [primary-platform-install.json](../../../artifacts/cs-t0/tx-03-14-static/primary-platform-install.json)：Windows dispatch 安装、TLS 分配／赋值。其混合 ID／handler 数据条目见下文采集限制，不当作纯函数表。
- [primary-teardown-review.json](../../../artifacts/cs-t0/tx-03-14-static/primary-teardown-review.json)：独立导出 `A13060/A19130/A71770/A6AF90/A5D4E0/A7FCB0/A11730/A69C00`。
- [primary-cache-drain.json](../../../artifacts/cs-t0/tx-03-14-static/primary-cache-drain.json)：`415800` 对缓存节点 pair 的 control 释放。
- [primary-manager-tables-review.json](../../../artifacts/cs-t0/tx-03-14-static/primary-manager-tables-review.json)：独立导出 manager／ImageManager 真正表起点及相关表项。

## 对 Luna 结论的审阅

对照 [luna-renderer-teardown.json](../../../artifacts/cs-t0/tx-03-14-static/luna-renderer-teardown.json)及 [luna-window-close-chain.json](../../../artifacts/cs-t0/tx-03-14-static/luna-window-close-chain.json)，主审逐段读过终稿、关键 bytes、分支、receiver 和释放顺序。

| 结论／问题 | 主审处理 |
| --- | --- |
| `_main → A71770` 正常退出 | 接受真实调用点 `93140E`；先有游戏关闭 `9313DE → 9AB700`。`A71770` 有其它服务清理，不能当专供 Sprite 的析构入口。 |
| `A13060 → FBO/RBO → A6AF90` | 接受 `A71A48`、后续 GL 调用和 `A71A7A` 顺序；另一虚方法 `A19130` 也有相同子序列，但没有把它与 A71770 串成一次双重析构。 |
| `C7163C=0` 和图像树释放 | 接受，但限于 `C37984 & 1`；`A131A1` 清回调，`A131B3` 调 `415800`。主审另展开节点 pair 释放：回调清零不阻止 control 自身减引用／删除 image，树外引用仍可保活图像。 |
| 当前线程解绑 | 接受 `A6AF90` 的 TLS window 比较及 `window+22C(0)`；只针对调用线程。未由此证明其它线程已经 detach 或所有资源调用者已经停止。 |
| WGL context 和 HWND 删除 | 接受 `window+240 → A7FCB0 → wglDeleteContext` 先于 `DestroyWindow`。HGLRC 字段无条件清零不代表 OS 调用成功，HWND 清零亦非成功证明。 |
| 正常关闭等于完整 GLFW terminate | 不接受外推。`A69C00` 的当前直接 code xref来自初始化失败支路；所读正常 `_main/A71770` 没有直接调用它。动态目标及全部异常退出仍未知。 |
| 所有资源均在 context 之前释放 | 不接受。`A71770` 后面仍释放未识别对象集合；别名、在途绘制及这些对象是否含 GL 资源未全闭合。 |
| 早期两处 RVA 误写 | 子任务进展把 `A13060/A131A1` 写成错误 RVA，已在消息中纠正并由其报告保留修订，正确为 `613060/6131A1`。原始 JSON 算术正确。 |
| 终稿另外两处 RVA 误写 | 主审发现 `A7FCB0/A69C00` 被写为 `27FCB0/269C00`，已修正为 `67FCB0/669C00` 并留修订记录；未据误写实现调用。 |
| 表起点／表项及 callback ABI | `B82600/B818B0` 是表起点，`B82608/B818B8` 是其中 `+08` 的表项地址。主审独立取表后修正文案；`A5D4FF` callback 的唯一参数在栈上传 window，不以 ECX 猜测 this。 |

主审还补齐了子任务范围以外的事实：正常 `_main` 同步初始化和 render 来源；Windows WGL 函数字段安装与 current TLS；`9AA040 → 8FB120 → 9555C0` 的加载绘制直接调用。现有 `loop_runtime.cpp` 仅替换 `9312F1` 的 callsite，不能把加载绘制也计为经过 RenderHook。framebuffer 切换不等于 context 转移，SwapBuffers 也不证明 GPU 完全空闲。

## 采集限制与推导边界

取证只读当前 IDB，记录 input path／imagebase，没有把路径一致当作与磁盘文件内容相同。没有运行时 thread ID、TLS 值、HGLRC、GL 成功结果或游戏对象观察。原始函数地址和调用栈来源说明正常执行路径，不将它升级为一个任意时刻都有效的 renderer 线程 guard。

初次 rg 清单带了不存在的 `.agents/.codex` 目录，实际返回 OS error 2／exit1；没有把部分清单当作“没有更多指令”。默认 IDA 字符串选择扫描返回零，但函数中仍有宽字符 assertion；未从该结果推断不存在 context 相关代码。

主审曾把 `B4E880` 的两 DWORD 混合表交给通用 pointer-table 导出器：第一字实际是 platform ID=`60001`，不是 VA；原始 JSON 的 `target_rva=-0x39ffff` 只是机械减法，无地址含义。保留原始文件，正文按 ID／handler 分类，统一文档审阅也不将它纳入指针目标 RVA 检查。第二字为 `A81050` handler。此限制及其它采集说明见 [primary-acquisition-notes.json](../../../artifacts/cs-t0/tx-03-14-static/primary-acquisition-notes.json)。

主审用原始寄存器修正了伪代码表面的 `A9DB50(0)`：实际 ECX 为 `60000h`、EDX 为 dispatch destination。另读到名称含 `wglMakeCurrent` 的 `40C200` 只有 `ret`，没有按符号名认定它进行 context 切换。IDA 自动分析可能细化显示名；双方没有显式 patch／rename／set_type／保存操作。

只读可重复导出入口为主审 [export_static.py](../../../artifacts/cs-t0/tx-03-14-static/export_static.py)及子任务 [luna-export-teardown.py](../../../artifacts/cs-t0/tx-03-14-static/luna-export-teardown.py)、[luna-export-close-chain.py](../../../artifacts/cs-t0/tx-03-14-static/luna-export-close-chain.py)。主审通过 `py_eval → runpy.run_path` 让文件在单一 namespace 执行；使用 `TX0314_OUTPUT` 和相应函数／表等选择集写新路径。子脚本现支持 `TX0314_LUNA_OUTPUT`，这一可复现性编辑未再次执行；均保留 create-new，不覆盖原始数据。

本轮没有修改生产代码，没有启动／附着游戏或调试器，没有注入、断点、游戏输入、构建、单元测试或 E2E；没有触及原版 Online／Isaac Sync 会话，没有计算／校验任何哈希。子报告修订和脚本编辑均发生在最终统一审阅之前。

## 唯一一次整体静态文档验收

所有研究、子报告复核和编辑结束后，仅执行一次 [review_documents.ps1](../../../artifacts/cs-t0/tx-03-14-static/review_documents.ps1)。统一读取三份新报告、四个更新入口、主审和子任务 JSON，检查新引用、VA/RVA、文档地址对、关键原始指令、manager 表项及独立取证的一致性；结果保存在 [final-static-review.json](../../../artifacts/cs-t0/tx-03-14-static/final-static-review.json)。它检查交付一致性，语义判断来自上述人工读码，不证明实际线程、context 切换或退出成功。

本轮交付为正常 renderer／context 的静态来源和顺序。自有资源队列／generation、在途绘制寿命、window/context 重建、异常／并发恢复和 Replica 均未实现。下一项具体逆向为 Sprite 换图／重载及其缓存引用关系；本轮不因还存在这些未知而扩展成无限后台闭包研究。
