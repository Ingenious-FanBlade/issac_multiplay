# tx-03-13：Sprite ImageBase 引用控制

日期：2026-10-22（Asia/Shanghai）。类型：静态逆向，只读 IDB。范围限于 Sprite layer 持有的 `ReferenceCount<KAGE::Graphics::ImageBase>` pair、其控制块、layer 销毁／赋值时的释放及全局回调门控。主智能体另行研究 image loader、cache 与 GPU 路径；本报告引用其复核反馈，不重复扩大后台闭包。

## 结论与口径

一个控制块由 `0x40C7F0` 分配 24 字节并初始化，持有 16 位 strong／weak 计数、一个 `KAGE::Mutex` 和 `ImageBase*`。正常已初始化的计数分别为 1 和 1。控制块虚表有六个槽：删除控制块、尝试增加 strong、增加 strong、减少 strong 并可能析构 ImageBase、增加 weak、减少 weak 并可能析构控制块。Sprite layer 中的引用是 8 字节 `{ImageBase*, control*}` pair。

`+0x0C` 的 bool 是控制块释放路径的继续条件，不等同于“图像已删除”或“刚刚减少了最后一个引用”。为真时，调用者还可能检查并调用全局 `0xC7163C` 回调；该回调在锁内读取残余 strong，解锁后比较是否恰为 1，满足时进入 `0xA121B0` 从图像缓存摘除。回调执行线程 UNKNOWN。计数操作各自经过互斥量，但 `+0x04` 的检查与随后 `+0x08` 的增加分属两次加锁；不能据此称为原子 retain-if-alive，也不能由锁的存在推导整条生命周期并发安全。

以下状态标签用于区分证据强度：`STATIC_CONFIRMED` 是指令／表项直接可见的事实；`STATIC_DERIVED` 是由这些事实推出的控制流解释；`UNKNOWN` 是静态证据未闭合之处。VA/RVA 使用 imagebase `0x400000`；对象字段偏移、虚槽偏移不属于 RVA。IDA 函数原型仅作参考，调用约定以 ECX、栈参数、清栈和返回指令为准。

## 数据与复核来源

本轮 IDA 导出为 `STATIC_IDB_READ_ONLY`，imagebase `0x400000`。完整函数和表指令见 [Luna 原始 JSON](../../artifacts/cs-t0/tx-03-13-static/luna-control-core.json)，采集脚本见 [只读导出脚本](../../artifacts/cs-t0/tx-03-13-static/luna-export-control.py)，IDA 工具原始返回见 [工具返回记录](../../artifacts/cs-t0/tx-03-13-static/luna-control-tool-returns.json)。记录涵盖 19 个函数、3 张表以及关键表项 bytes。未调用 `survey_binary`，没有哈希计算／校验，没有启动游戏、测试、probe、构建或实验；没有修改 IDB、生产源码或数据库类型。

另一重复 disasm 批次由主智能体中断，本报告不等待、不重试，也不据该批次推断结果。首次导出的完整原始指令已足够支撑以下结论；本轮 IDA 调用没有记录到失败。没有测试运行。

主智能体的独立复核与补充见 [主审记录](tx-03-13-primary-review.md)、[control 复核](../../artifacts/cs-t0/tx-03-13-static/primary-image-control-review.json)、[cache retire 上下文](../../artifacts/cs-t0/tx-03-13-static/primary-image-retire-context.json)、[PNG 生命周期](../../artifacts/cs-t0/tx-03-13-static/primary-image-png-lifecycle.json)及 [GPU 相关指令](../../artifacts/cs-t0/tx-03-13-static/primary-image-gpu.json)。它还提交了整体 Sprite 图像资源正文 [tx-03-13 主报告](tx-03-13-sprite-image-resources.md)。这些是主审独立证据，不应与本轮 Luna 导出混为同一采集批次。

## 控制块布局与六个槽

`STATIC_CONFIRMED`：`0x40C7F0`（RVA `0x0C7F0`）为控制块申请 24 字节（`push 0x18`，`0x40C81A` 后调用 `0xA0F4C0`），调用 `0x40CC10` 构造，再写入专用表 `0xB1A6E0`。`0x40CC10` 将计数 DWORD 初始化为 `0x00010001`。结合 16 位读写和 x86 little-endian，可得正常初始化 strong=`u16[control+4]=1`、weak=`u16[control+6]=1`。对象布局如下；偏移均相对 control：

| 偏移 | 字段 | 证据与边界 |
| --- | --- | --- |
| `+0x00` | `ReferenceCount<ImageBase>` 专用 vptr | 由 `0x40C7F0` 写 `0xB1A6E0`；表 bytes 为 `50 CB 40 00 B0 C6 40 00 90 C6 40 00 30 C6 40 00 10 C6 40 00 C0 C5 40 00`。 |
| `+0x04` | strong `u16` | `0x40C690` 增加；`0x40C630` 减少；`0x40C6B0` 检查。 |
| `+0x06` | weak `u16` | `0x40C610` 增加；`0x40C5C0` 减少。 |
| `+0x08` | `KAGE::Mutex` 子对象 vptr | `0x40CC10` 写 `0xB81C0C`。 |
| `+0x0C` | Mutex 初始化／状态 byte | 构造时清零；Mutex 方法读取或更新。 |
| `+0x10` | 堆分配 28-byte 同步区块指针 | `0xA15770` 将其传给 `InitializeCriticalSection`，并使用区块 `+0x18` 的额外状态 byte；`0x40CB50` 删除 critical section 并释放区块。 |
| `+0x14` | `ImageBase*` | `0x40C7F0` 赋入；strong 清零时调用动态 ImageBase 槽 `+0x34`，随后置空。 |

`0xB1A6E0` 的槽目标如下。`this` 都在 ECX；除删除析构函数外，直接调用路径没有额外栈参数，返回通过 AL/EAX 或无返回值；虚槽偏移是 vtable 内偏移，不是对象字段偏移。

| 槽 | VA（RVA） | 作用与 ABI |
| --- | --- | --- |
| `+0x00` | `0x40CB50`（`0x0CB50`） | 控制块删除析构；ECX=control，栈上 delete flag，`ret 4`。清理 critical section／Mutex 状态；delete flag 的 bit0 为 1 时释放 control 本身。 |
| `+0x04` | `0x40C6B0`（`0x0C6B0`） | 尝试增加 strong；ECX=control，无栈参数；AL=1 表示检查后执行了增加，AL=0 表示观察到 strong 为 0。 |
| `+0x08` | `0x40C690`（`0x0C690`） | 增加 strong；ECX=control，无栈参数。 |
| `+0x0C` | `0x40C630`（`0x0C630`） | 减少 strong，并在达到 0 时走 ImageBase／weak 清理；ECX=control，无栈参数；AL 是下文解释的继续谓词。 |
| `+0x10` | `0x40C610`（`0x0C610`） | 增加 weak；ECX=control，无栈参数。 |
| `+0x14` | `0x40C5C0`（`0x0C5C0`） | 减少 weak；weak 归零时调用槽 `+0x00` 并删除 control；ECX=control，无栈参数；AL=1 表示控制块仍保留。 |

表项的 RVA 等于 VA−`0x400000`。`ReferenceCountBase` 的 `0xB1A6FC` 是另一张基类表，不应把其中的纯虚槽当成本专用控制表实现。这里的槽语义均从 `0xB1A6E0` 及目标指令得出；不套用其它 VirtualDevice 合约。

## 计数操作、返回值与最终释放

`STATIC_CONFIRMED`：`0x40C690` 与 `0x40C610` 分别在 Mutex lock/unlock 之间对 `control+4` 和 `control+6` 做 16 位增加。`0x40C630` 对 strong 做加锁检查与递减；只有 strong 原值非零才递减。递减后若仍非零则解锁并返回 1；若减到零，则解锁后读取 `control+0x14`，若非空以 ECX=ImageBase、栈参 1 调其 vtable `+0x34`（`0x40C666: FF 50 34`），再清空 `control+0x14`，最后走控制表 `+0x14` 减 weak 并返回其 bool。

`STATIC_CONFIRMED`：`0x40C5C0` 加锁检查 weak。weak 非零时减 1；减后为零则解锁并调用控制表 `+0x00`，传 delete flag=1；否则解锁返回 1。weak 已经为 0 时不递减并返回 1。`0x40CB50` 将 control vptr 重置为 `ReferenceCountBase`，处理 Mutex/critical section，清除相关指针和初始化状态；传入 delete flag 时再调用 `0xAEF15C` 释放 control。它与 ImageBase 动态 `+0x34` 析构调用是两个不同对象的销毁步骤。

`STATIC_DERIVED`：从正常 `strong=1, weak=1` 路径看，strong 的最后一次减少会请求动态 ImageBase 删除析构，再减少基准 weak；若没有额外 weak，control 随之被删除。若还有 weak，ImageBase 已走清理路径而 control 可继续存活，直到后续 weak 减至零。主审确认 PNG 实例的 ImageBase `+0x34` 目标是 `0xA649D0`，其删除析构直接调用 `glDeleteTextures`；这是 PNG 子类证据，不外推到所有 ImageBase 子类。完整 GPU 操作成功与否、其他子类析构副作用不在此处保证。

特别地，`0x40C630` 返回 1 的至少三种可见情形是：strong 原本为 0（不减数）；strong 减后仍大于 0；strong 减至 0 后 weak 减少仍令 control 保留。只有其最后一支 strong=0、weak 也归零会从 `0x40C5C0` 删除控制块并返回 0。因此不能把 `+0x0C` 返回值命名为“图像已删除”“最后一个引用”或“缓存应删除”。

## Mutex 与并发边界

`0xA15770` 在 Mutex 尚未初始化时分配 28 字节区块，将其传给 `InitializeCriticalSection`，并将区块 `+0x18` 的额外状态 byte 清零；随后保存指针并设初始化位。锁函数 `0xA157F0` 在有效路径调用 `EnterCriticalSection`，还检查该额外 byte，并在其非零时 `Sleep(0x3E8)` 循环，之后置 1。`0xA159A0` 清该 byte 并调用 `LeaveCriticalSection`。计数器增减函数分别使用 Mutex 虚槽 `+0x0C`／`+0x10`，表在 `0xB81C0C`，锁／解锁实现分别为 `0xA157F0`、`0xA159A0`。

`STATIC_CONFIRMED`：`0x40C6B0` 先锁住控制块、读 `strong`，随后解锁；观察到非零时再通过控制表 `+0x08` 调 `0x40C690`，后者会重新加锁增加计数。因此检查与增加之间存在未由同一临界区覆盖的窗口。`STATIC_DERIVED`：此函数不是原子 retain-if-alive；不能只凭每个单独计数读写有锁就证明对象生命周期完整同步。Mutex 初始化若分配失败的恢复语义和并发交错下的运行时结果没有动态验证。实际线程及回调调度均 `UNKNOWN`。

## Sprite pair 赋值、销毁及全局回调

Sprite layer 的 ImageBase pair 位于 `layer+0x98`，两字为 `{ImageBase*, control*}`。`0x407A60` 的 ABI 是 ECX=layer、无栈参数、普通 `ret`。指令 `0x407A86: 8D B7 98 00 00 00` 取 pair 地址；`0x407A93: 8B 4E 04` 把 control 放入 ECX，`0x407A9C: 8B 40 0C` 取 vslot+`0x0C`，`0x407A9F: FF D0` 调用。若 AL 非零且全局 `0xC7163C` 非空，`0x407AAE: push esi` 将 pair 地址作为唯一 cdecl 参数传给回调。随后清理 layer 的原生字符串。该函数体没有把 pair 两字段清零；更外层对象存储释放的细节不由这段推断。

`0x408640` 的 ABI 特别容易被 Hex-Rays 伪原型误读：ECX=layer，栈上两个 4-byte 参数依次构成按值传递的 `{ImageBase*, control*}` pair，末尾 `C2 08 00`（`ret 8`）。image 非空时，它从 image 对象 `+0x30..+0x3C` 复制四个 DWORD 到 layer `+0x20..+0x2C`；随后以目的 `layer+0x98` 调 `0x40C3B0`，源 pair 是该栈参数的地址；函数结束前，栈上源 pair 的 control 非空时对其调 `+0x0C`，AL 非零且全局回调非空则将该栈上 pair 地址传给 `0xC7163C`。这里销毁的是按值传入的源 pair，旧目的引用在 `0x40C3B0` 内部释放；不能把两次释放混为一处。回调参数地址指向函数栈上的副本，不是未经解释的 EBP 临时量，也不是传入对象的原地址。

`0x40C3B0` 先用 `0x40C550` 拷贝源 pair；`0x40C550` 将输出 pair 清零，源 control 非空时经槽 `+0x04` 尝试增加 strong，成功才复制两指针。赋值 helper 随后交换目的与局部 pair，最后对交换出的旧 pair 按 `+0x0C`／全局回调规则释放。由此，普通层更新路径是“获取源引用、交换 pair、释放旧引用”，而不是仅复制两个裸指针。

`0x40C7F0` 新建 pair 的直接路径为：申请 24 字节、构造计数和 Mutex、设置 `0xB1A6E0` vptr、写入 ImageBase 指针；在覆盖原 pair 前先对旧 control 调 `+0x0C`，AL 非零且 `0xC7163C` 非空时传旧 pair 地址作回调，最后写新 image/control 两字。该入口 ABI 为 ECX=pair 地址，栈上一个 ImageBase 指针，`ret 4`。控制块分配失败分支仍写入 ImageBase 指针、control 为 null；调用者对这种非正常 pair 的处理未由本轮闭合。

主审从其他独立静态导出确认：全局变量 `0xC7163C` 被赋为 `0xA12240`；回调在 `control+8` 锁内读取 `u16(control+4)`，残余 strong 恰为 1 时进入 `0xA121B0`，从全局 image cache 摘除对应资源。调用这段回调发生在 `+0x0C` 已返回并释放其计数锁之后。`+0x0C` 的 AL 只门控此回调，回调自己的 `strong==1` 才是摘除条件；两者不可合并成一个“最后引用”谓词。回调线程、是否所有调用均串行、`0xA121B0` 之后 cache 所持引用的完整释放／重入路径仍为 `UNKNOWN`。PNG +`0x58` 的绑定函数也不能通过固定返回 AL=1 证明驱动上传成功；本控制结论不据此宣称资源已成功 GPU 绑定。

## 正常路径和未闭合边界

以上正常生命周期结论限定为：control 分配成功、`0x40CC10` 与 Mutex 初始化可用、pair/control 字段有效、动态 ImageBase vptr 指向有效实现、计数处于代码预期范围且没有并发破坏。正常路径是：引用增加／减少在各自 Mutex 操作中更新 16 位计数；strong 归零调用 ImageBase `+0x34` 并置空 ImageBase 指针；随后减少 weak；weak 归零销毁 Mutex 和 control。Sprite pair 更新先尝试保活并复制源引用，再交换并释放旧 pair；layer 销毁同样释放 pair 后按返回 bool 决定是否调全局 callback。

`UNKNOWN`／异常边界：24-byte control 分配失败时 `0x40C7F0` 的错误 pair 语义；critical section 分配失败后 Mutex 使用效果；计数溢出或不满足不变量；析构函数抛出／进程异常；其它 ImageBase 子类的 `+0x34` 行为；回调线程、重入和 cache 回调的完整调用者集合；layer 销毁后的对象存储释放具体路径。报告没有通过运行时实验补这些空白，也不将存在的临界区描述为整个 Sprite/image 生命周期线程安全。

## 主审修订记录

初稿把 `FF 50 34` 的调用点写成 `0x40C662`；主审重读原始指令后修正为 `0x40C666`，前者实际为 `mov eax,[ecx]`。初稿另将 `0x408640` 尾部销毁的栈上源 pair 误称为旧局部 pair；已明确它与 `0x40C3B0` 内释放的旧目的 pair 是两处引用。删除 flag 也已限定为 bit0，并将全局回调表述细化为锁内读取、解锁后比较。以上是静态报告的修正，未依据这些误写实现任何调用或运行实验。
