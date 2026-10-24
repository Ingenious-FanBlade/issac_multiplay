# tx-03-14：Luna renderer teardown 与 GL context 销毁

日期：2026-10-22（Asia/Shanghai）。类型：静态逆向。范围：正常关停路径中的 renderer 资源回收、GLFW window 与 WGL context 销毁顺序；不覆盖初始化／帧循环主体。GPT-6 Luna / max 子智能体取证，主智能体提供独立静态交叉核对。

## 结论与边界

`_main` 的可见退出调用点是 `0x93140E`（RVA `0x53140E`），直接调用 `A71770`。该清理例程先调用 `A13060(&C798E0)`，随后绑定 framebuffer 0、删除 renderer 的 framebuffer/renderbuffer，再调用 `A6AF90(C7999C)`。`A6AF90` 只在**当前调用线程** TLS 指向待关 window 时，经 window+`0x22C` 回调解除 current；之后走 Windows 平台销毁回调。Windows 回调先调用 window+`0x240` 的 context-destroy 函数 `A7FCB0`，再调用 `DestroyWindow`。`A7FCB0` 在 `wglDeleteContext` 返回后无条件清零 window+`0x268`，这只能证明程序清了字段，不能证明操作系统成功删除了 HGLRC。

图像管理器的引用／缓存清理以及 `C7163C` 清零，在该退出路径的 `A13060` 内，位于 framebuffer/renderbuffer 与 context/window 销毁之前；`C7163C` 和图像树清理仅在 `C37984 & 1` 分支执行。tx-03-13 已说明图像最后强引用可同步进入派生图像析构并调用 GL 删除纹理；该过程是否发生在每个对象上取决于剩余引用，不等同于此处必然销毁所有纹理。

顺序也有一个明确缺口：`A71770` 在 `A6AF90` 返回后仍遍历并释放 `C379F4`、`C37A00`、`C3796C` 等全局对象。其元素类型及是否持有其它 GL 资源未在本次范围内闭合，因此不能断言所有 renderer 相关资源均先于 context 销毁。异常、OOM、并发关闭以及其它线程的 TLS/current 状态均为 UNKNOWN。

另一个 renderer 虚表清理函数 `A19130` 也呈现“`A13060` → FBO/RBO 删除 → `A6AF90(this+0xBC)`”顺序；其 manager 虚表表项地址为 `0xB82608`，表起点为 `0xB82600`。本轮未确认该虚调用在 `_main` 退出路径上发生，不能把它与 `A71770` 串成同一次析构。

## 证据口径

IDB：`F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`，input path 同目录 `isaac-ng.exe`，imagebase=`0x400000`。本文 VA 使用 IDB 地址；RVA=`VA-0x400000`。所有关键指令 bytes、函数完整指令、伪代码与 xrefs 见 [renderer 清理导出](../../artifacts/cs-t0/tx-03-14-static/luna-renderer-teardown.json)及 [窗口关闭链导出](../../artifacts/cs-t0/tx-03-14-static/luna-window-close-chain.json)；取证脚本为 [luna-export-teardown.py](../../artifacts/cs-t0/tx-03-14-static/luna-export-teardown.py) 和 [luna-export-close-chain.py](../../artifacts/cs-t0/tx-03-14-static/luna-export-close-chain.py)。

`STATIC_CONFIRMED` 表示指令、表项或直接 xref 可见；由多个直接行为组合出的正常顺序标为 `STATIC_DERIVED`。本报告没有运行时观察。本次记录所有地址均按 imagebase 重新计算。早期进展曾把 `A13060` 和 `A131A1` 的 RVA 写错；正确值分别是 `0x613060` 与 `0x6131A1`，下文及导出 JSON 均使用修正值。

## `_main` 正常退出调用链

主审导出的 `_main` 原始指令：[primary-context-paths.json](../../artifacts/cs-t0/tx-03-14-static/primary-context-paths.json)。退出块在 `A71770` 之前还调用 `SteamAPI_Shutdown`、调整执行状态和 `timeEndPeriod`；`A71770` 返回后调用 `CoUninitialize`。

| VA（RVA） | receiver／栈与原始指令 | 结论 |
| --- | --- | --- |
| `0x93140E`（`0x53140E`） | bytes `E8 5D 03 14 00`；`call sub_A71770`，无显式参。 | `STATIC_CONFIRMED`：本路径从 `_main` 直接进入 renderer 清理例程。 |
| `0xA71A43`（`0x671A43`） | `B9 E0 98 C7 00`；ECX=`&dword_C798E0`。下一条 `0xA71A48` bytes `E8 13 16 FA FF`，`call A13060`。 | `STATIC_CONFIRMED`：先以全局 renderer manager 为 `this` 做对象／资源清理。 |
| `0xA71A4D–0xA71A6E`（`0x671A4D–0x671A6E`） | `0xA71A54` bytes `FF 15 30 F9 C0 00` 调 `epoxy_glBindFramebuffer`，前置参数为 `target=0x8D40, framebuffer=0`；`0xA71A61` bytes `FF 15 0C FA C0 00` 以 `&C7998C` 调 `epoxy_glDeleteFramebuffers(1,…)`；`0xA71A6E` bytes `FF 15 50 F9 C0 00` 以 `&C79990` 调 `epoxy_glDeleteRenderbuffers(1,…)`。 | `STATIC_CONFIRMED`：清理 framebuffer/renderbuffer 的调用在 `A6AF90` 前。 |
| `0xA71A74–0xA71A8A`（`0x671A74–0x671A8A`） | `0xA71A74` 将 `C7999C` 装入 ECX；`0xA71A7A` bytes `E8 11 95 FF FF` 调 `A6AF90`；`0xA71A8A` bytes `C7 05 9C 99 C7 00 00 00 00 00` 将 `C7999C=0`。 | `STATIC_CONFIRMED`：以全局 window 指针调用 window teardown，并在返回后清全局字段。 |

另一路 renderer 方法 `A19130`（VA `0xA19130`，RVA `0x619130`）的 manager 虚表表项地址为 VA `0xB82608`（RVA `0x782608`，bytes `30 91 A1 00`），即表起点 `0xB82600` 的 `+0x08`。它的 ECX 是 renderer manager：`A19133` bytes `E8 28 9F FF FF` 调 `A13060(this)`；`A1913F` bytes `FF 15 30 F9 C0 00` 调 `glBindFramebuffer(0x8D40,0)`；`A1914E` bytes `FF 15 0C FA C0 00` 调 `glDeleteFramebuffers(1,this+0xAC)`；`A1915D` bytes `FF 15 50 F9 C0 00` 调 `glDeleteRenderbuffers(1,this+0xB0)`；`A19169` bytes `E8 22 1E 05 00` 调 `A6AF90(this+0xBC)`；`A1916E` bytes `C7 86 BC 00 00 00 00 00 00 00` 将 `this+0xBC` 清零。这证明该虚方法体内的顺序；没有证据表明它与 `_main` 的 `A71770` 在同一条退出路径上连续执行。

## 图像管理器资源先于 context 销毁

`A13060`（VA `0xA13060`，RVA `0x613060`）的 receiver 是 `A71770` 传入的 `&C798E0`。其伪代码和完整指令保存在 Luna 原始导出中。函数先遍历 manager 内部持有的对象数组／链表，执行虚调用释放并重置容器；释放 `C7367C` 指向的对象；随后处理另一个全局对象链表 `::j`，再重置状态字段。这些动作均在 `A13060` 返回前完成，因此在 `_main` 的退出路径上早于 FBO/RBO 和 WGL context 清理。

全局图像缓存分支有明确条件：

| VA（RVA） | bytes／指令 | 含义 |
| --- | --- | --- |
| `0xA13182`（`0x613182`） | `F6 05 84 79 C3 00 01`；`test byte ptr dword_C37984,1`。 | 检查缓存初始化位。 |
| `0xA13194`（`0x613194`） | `74 48`；flag 未置位则跳过清理分支。 | 下述清理有条件。 |
| `0xA131A1`（`0x6131A1`） | `C7 05 3C 16 C7 00 00 00 00 00`；`mov dword_C7163C,0`。 | 先关闭缓存回调。 |
| `0xA131AB–0xA131B3`（`0x6131AB–0x6131B3`） | `FF 76 04`、`68 88 79 C3 00`、`E8 48 26 A0 FF`；调用 `sub_415800(&::i, ::i->next)` 清理缓存节点。 | 调用点为 `0xA131B3`，不是区间起点。 |
| `0xA131BB–0xA131D7`（`0x6131BB–0x6131D7`） | 对 `C37990` 的 mutex 虚槽 `+8` 调用，清 `C3798C`，最后 `and dword_C37984,~1`。 | 清缓存同步状态及初始化位。 |

`C7163C` 在 tx-03-13 中识别为图形资源缓存回调入口，IDA 可能将其显示为 `n2_2`。本处先将回调设零，再清理全局图像缓存树并释放同步对象；条件是 `C37984 & 1`。成功初始化写入该 flag 的相关证据见主审的 [context 初始化导出](../../artifacts/cs-t0/tx-03-14-static/primary-context-initial.json)，但运行时 flag 值不属于本次观察。

tx-03-13 已将回调链闭合为 `C7163C → A12240 → A121B0`，且 PNG 最后强引用删除会进入派生图像析构并调用 `epoxy_glDeleteTextures`。因此，若 `A13060` 的缓存摘除使某张图像最后引用归零，其同步 GL 纹理删除会发生在随后 FBO/RBO 与 context 销毁之前；若仍有外部强引用，则不保证这次清缓存就析构图像。不得把“缓存已清”当成“全部图像纹理已删”。

ImageManager 自身另有方法：`A11730`（VA `0xA11730`，RVA `0x611730`）在 `this[1]&1` 时将 `C7163C` 置零、清 `this+8` 链表、将 `this[3]` 置零，并销毁 `this+0x10` mutex。其 ImageManager 虚表表项地址为 `0xB818B8`（RVA `0x7818B8`，bytes `30 17 A1 00`），即表起点 `0xB818B0` 的 `+0x08`；删除 wrapper `A115F0` 调用 `A11620`，后者也有自己的条件清链与 mutex/基类清理。此处没有闭合这些函数与 `_main → A71770` 的调用关系，故不将它们加入上面的正常 renderer 退出顺序。

## 从 `A6AF90` 到 Windows context/window 销毁

`A6AF90`（VA `0xA6AF90`，RVA `0x66AF90`）以 ECX 接收 window。`A71770` 从 `C7999C` 传入；`A19130` 从 `this+0xBC` 传入。调用者不会把 HWND/HGLRC 当作显式栈参。

1. 函数检查 GLFW 初始化状态与 window 非空。对有效 window，`0xA6AFB5` bytes `6A 44`、`0xA6AFB7` bytes `8D 86 70 02 00 00`、`0xA6AFC0` bytes `E8 20 56 08 00` 将 window+`0x270` 起 `0x44` 字节清零。
2. `0xA6AFEC` 调 `TlsGetValue(dwTlsIndex)`；`0xA6AFF2` bytes `3B F0` 比较返回值与当前 window，`0xA6AFF4` bytes `75 55` 不相等时跳过 current 清理。相等时再次取本线程 TLS 值，再于 `0xA6B03E` 读取 TLS 对象+`0x22C` 的函数指针，`0xA6B046` 以栈参 `0` 调用该回调。主审已确认此字段对应 `A7FA50`，其空参分支调用 `wglMakeCurrent(0,0)` 并清 TLS。这里仅证明当前执行线程的条件分支；没有跨线程 detach、等待其它线程或引用计数协议证据。
3. `0xA6B04B` 将 window 压栈，`0xA6B04C` bytes `FF 15 0C 37 C7 00` 调全局函数指针 `C7370C`。它是平台销毁回调，不是从 `A6AF90` 直接静态调用固定 API。

Windows 回调的静态映射依据如下：`A6A420` 在 `0xA6A43E` bytes `68 94 36 C7 00` 把 `C73694` 作为平台函数表 destination；`0xA6A4B9`/`0xA6A4BE` 设置 `dst=C73694`、平台 id `0x60000`，`0xA6A4C3` bytes `E8 88 36 03 00` 调 `A9DB50`。该分支将 id `0x60000` 路由到 `A81050(0x60001,dst)`（`A9DB50` 的 `case 393216`；调用点 `0xA9DE83–0xA9DE89`）。`A81050` 在 `0xA8119A` bytes `C7 85 58 FF FF FF E0 D4 A5 00` 将表项 30 写为 `A5D4E0`，表以 `rep movsd` 复制到 destination。由于 `C7370C = C73694 + 0x78` 且 `0x78 = 30*4`，该平台 callback slot 指向 `A5D4E0`。此为从当前 Windows platform table 初始化数据推导出的静态映射。

`A5D4E0`（VA `0xA5D4E0`，RVA `0x65D4E0`）的 receiver 是 window：

| VA（RVA） | bytes／指令 | 正常顺序 |
| --- | --- | --- |
| `0xA5D4F4`（`0x65D4F4`） | `8B 86 40 02 00 00`；读 window+`0x240` callback。`0xA5D4FF` bytes `FF D0` 调该函数，栈上唯一参数为 window。 | 先执行每 window 的 context-destroy callback；不以 ECX 当作其 this。 |
| `0xA5D537–0xA5D549`（`0x65D537–0x65D549`） | HWND=window+`0x2B4`，移除 `"GLFW"` property；`0xA5D549` bytes `FF 15 E8 86 B1 00` 调 `DestroyWindow`。 | 平台 context callback 返回之后再销毁 HWND。 |
| `0xA5D54F`（`0x65D54F`） | `C7 86 B4 02 00 00 00 00 00 00`。 | 清空 HWND 字段；随后还处理两个 icon 字段。 |

`A80010` 创建平台 window table 时给 window+`0x240` 安装 `A7FCB0`；对应函数地址表项由主审导出交叉核对。`A7FCB0`（VA `0xA7FCB0`，RVA `0x67FCB0`）接收栈上 window 参数：`0xA7FCB7` bytes `8B 86 68 02 00 00` 读取 HGLRC=window+`0x268`；非空则 `0xA7FCC2` bytes `FF 15 48 5D C7 00` 调 `wglDeleteContext`；紧接着 `0xA7FCC8` bytes `C7 86 68 02 00 00 00 00 00 00` 将字段清零。此清零位于 API 返回后，无论 API 返回成功或失败都会执行；没有检查返回值后保留字段的分支。

`A6AF90` 在平台回调返回后从全局 window 链表摘除 window、释放可选字段，最终释放 window 内存。`A7FCB0` 对 HGLRC 操作与 `A5D4E0` 对 HWND 操作的调用顺序由上述 callsite 直接确定；不代表 Windows 已成功销毁任何句柄。

## 全局 GLFW 清理函数和线程未知项

`A69C00`（VA `0xA69C00`，RVA `0x669C00`）会遍历 GLFW window 链表并调用 `A6AF90`，再清理 cursors、monitors、TLS／critical section 与相关全局状态。但它的唯一直接 code xref 是 `A6A420` 的 `0xA6A6F8`（bytes `E8 03 F5 FF FF`，初始化 helper 的失败返回支路）；本轮没有发现 `_main`/`A71770` 调用 `A69C00` 的正常关停 callsite。故不把它称作已证实的正常 `glfwTerminate` 路径，也不据其循环推断本进程所有窗口都已在常规退出中关闭。

静态材料只显示 `A6AF90` 比较当前线程 `TlsGetValue` 与待关闭 window，并在相等时经 `A7FA50` 解除 current。没有发现对其他线程执行 `wglMakeCurrent(0,0)`、join/wait、或 detach 所有 context 的协议。也没有运行时证据证明 `wglDeleteContext`/`DestroyWindow` 成功、关闭发生于唯一 renderer 线程，或释放时没有并发图像引用。

本轮遵照 tx-03-14 现行约定，只读 IDA 与静态文件；没有 patch/rename/set_type/保存 IDB，没有启动或附着游戏／调试器，没有操作原版 Online/IsaacSync，没有运行测试、构建、游戏或实验。整体静态文档审阅留给主智能体在全部工作完成后统一执行。

## 主审修订记录

主审终稿审阅还发现 `A7FCB0` 和 `A69C00` 的 RVA 分别误写为 `0x27FCB0`、`0x269C00`，已改为 `0x67FCB0`、`0x669C00`；原始 JSON 的数值换算正确，未据误写实现任何调用。另已区分虚表起点 `B82600/B818B0` 与其中表项地址 `B82608/B818B8`，并将 `A5D4FF` callback 的参数明确为栈上 window。缓存树调用区间及表格空列已澄清。早期进展的另外两处 RVA 误写仍按上文保留；这些均为文档修正，不是游戏运行失败。
