# tx-03-19 Luna：独立 Sprite 绘制窗口与主线程/WGL current guard

日期：2026-10-23（Asia/Shanghai）。本子报告只覆盖现有主线程/RenderHook 证据、加载旁路以及未来 Sprite 绘制前所需的窗口与 WGL-current 检查。它不实现 Hook 或 adapter，也不把本报告的静态分析当作任何运行时状态观察。

## 结论与证据标签

- **STATIC_CONFIRMED**：当前 native companion 在普通主循环 **RVA 0x5312F1** 只替换对 **0x9555C0** 的相对 call；RenderHook 建立 Phase::Render scope 并调用原函数一次。**0x8FC32C** 是加载等待环内另一个直接调用 **0x9555C0** 的 callsite，不经过该 RenderHook 槽。
- **STATIC_CONFIRMED**：renderer 的当前 GLFW window 字段位于 renderer+0xBC；TLS allocated/index 与 TLS current-window 值分开存放；窗口对象含 HDC/HGLRC；GLFW WGL 路线通过动态 WGL 函数进行 make-current，再更新调用线程 TLS。
- **STATIC_DERIVED**：Phase、phase_depth、main_thread_id 与 CompanionHealthy 可组成未来 guard 的 loop-owner 必要输入，但现有接口没有证明运行时线程 ID，也没有直接证明 OS 当前 HDC/HGLRC。Render 阶段标签可以在 companion 未 armed 时出现，且 direct loading call 可能嵌套在一个外层 Render scope 内。
- **DESIGN_PROPOSAL**：在目标绘制调用前 fail-closed 检查 companion 健康、主线程身份、经验证的外层 normal-render cookie、无加载/生命周期变化/自有 draw 重入、renderer/window/TLS 关系，以及当前线程的实际 WGL HDC/HGLRC；TLS 与 renderer window 相等仍不足以替代 WGL current 查询。
- **UNKNOWN**：当前游戏运行时的真实主线程 ID、TLS 值、OS current HDC/HGLRC、WGL getter 返回值、任一 loading 调用的实际线程、context 是否在任意 callback 后仍保持 current，以及目标 draw 的 GPU 完成时刻。本轮没有执行游戏或调用相关 API。

## 新鲜静态证据

只读 IDA 导出脚本为 [luna-export-render-window.py](../../../artifacts/cs-t0/tx-03-19-static/luna-export-render-window.py)，修正后的 create-new 导出为 [luna-guard-followup.json](../../../artifacts/cs-t0/tx-03-19-static/luna-guard-followup.json)。导出记录的 IDB 是 F:\\SteamLibrary\\steamapps\\common\\The Binding of Isaac Rebirth\\isaac-ng.exe.i64，input path 为同目录 isaac-ng.exe，imagebase=0x400000。该元数据只标明 IDA 当前数据库，不比较磁盘身份，也没有计算或校验哈希。

最终导出精确计数为：12 个函数 seed、12 个唯一解析函数、0 个未解析函数 seed、5 个原始地址范围、8 个数据 xref 记录、9 个符号记录、0 个虚表。没有从相邻 RTTI 推断虚槽数。初次导出 [luna-render-window-guard.json](../../../artifacts/cs-t0/tx-03-19-static/luna-render-window-guard.json) 中的 seed **0xA804B0** 实际归属函数 **0xA80010**；**0xA8FB7F** 不是 IDA 函数入口。修正后的导出保留 **0xA8FB7A–0xA8FB85** 指令范围，显示 **0xA8FB7F** 的 bytes 为 **A3 E0 35 C1 00**，即写回 epoxy_wglGetCurrentContext 函数指针的指令；它不是 TLS current-window getter。初次请求不能当作完整函数导出，followup 才是本报告使用的 12 函数集。

RVA 均按本 IDB imagebase 0x400000 计算。导出包含函数指令原始 bytes、伪代码、指定数据 xrefs 和选定原始指令范围；不包含运行时 memory/thread/context 状态。

## RenderHook 能证明什么

源码 [loop_runtime.cpp](../../../cs-t0/native/src/loop_runtime.cpp) 的 kCallRvas[7] 是 RVA **0x5312F1**，对应目标 RVA **0x5555C0**；对应预期 call bytes 为 **E8 CA 42 02 00**（VA **0x9312F1**）。fresh IDA 原始范围确认 **0x9312F1 / RVA 0x5312F1** 为 call 0x9555C0，bytes 同上。RenderHook 在源码第 1099–1103 行调用 InvokeVoid，phase 为 Render；InvokeVoid 在第 487–493 行先构造 PhaseScope，再检查 Armed 与 EstablishOrCheckMainThread，之后无论是否记录 observation 都调用 original() 一次。

PhaseScope 在源码第 279–296 行只保存/恢复本模块 thread_local phase 和 depth，并把 depth 加一。LoopClockHook 在 Armed 时调用 EstablishOrCheckMainThread(true, true)，源码第 997–1013 行；EstablishOrCheckMainThread 从 GetCurrentThreadId 读取当前值，并在 shared main_thread_id 尚为零时可将它首次写入。因而这些字段可供运行时 guard 比对，但当前代码不是本任务的运行时采样结果，也不证明“游戏主线程”之外的身份。若没有 g_shared，CurrentObservationContext 返回的 main_thread_id 保持默认零。

有两个限制必须同时保留。第一，RenderHook 先创建 Phase::Render scope，后决定是否观察并检查主线程；现有 InvokeVoid 不调用完整的 CompanionHealthy 检查。所以单看 Phase::Render、甚至 depth=1，不能证明 companion 正常，也不能证明该 callsite 来自普通 outer-render。第二，depth 只计本模块 PhaseScope，不是原生调用栈深度或防重入锁。加载等待环若从一次 render 内重入，direct call 可以继承外层 Render phase/depth，而不通过新的 RenderHook。

源码 CurrentObservationContext（第 1626–1639 行）只返回当前 thread ID、phase/depth、loop iteration 和共享块中的 learned main_thread_id；CompanionHealthy（第 1642–1653 行）可检查 embedded/armed、未 unloading/unsafe、Ready 状态、完整 hook mask 和无 overwritten events。未来 guard 应将 CompanionHealthy 为真、非零 main_thread_id 且当前 ID 相等作为必要条件；它们依旧不证明图形 context。还需以精确正常 owner 来源建立 outer-render-cookie，并独立跟踪自有 draw depth、generation 与 loading/lifecycle 状态。现有 ObservationContext 没有这些 cookie 或生命周期字段。

## 普通 render 与加载等待环的边界

| 路径 | 静态位置（VA / RVA） | 原始 call bytes | 结论 |
| --- | --- | --- | --- |
| _main 普通 render owner | **0x9312F1 / 0x5312F1** | **E8 CA 42 02 00** → 0x9555C0 | 当前 kCallRvas[7] 覆盖的唯一普通 RenderHook owner 入口 |
| 启动函数 9AA040 到 loading helper | **0x9AAAE3 / 0x5AAAE3** | **E8 38 06 F5 FF** → 0x8FB120 | helper 会启动加载任务并进入等待逻辑；这不证明后续 draw 在该 worker 上执行 |
| loading 等待环内 direct render | **0x8FC32C / 0x4FC32C** | **E8 8F 92 05 00** → 0x9555C0 | 直接调用目标函数，未经过 RVA 0x5312F1 的 RenderHook 槽 |

故 RenderHook 当前只能为普通 owner callsite 提供一个可读的 loop-phase 外壳；它不是对 0x9555C0 所有入口的函数级 detour。静态 callsite 也不枚举未知的间接调用或全部动态调用者。不得把 worker 被创建推为 render 使用该 worker 线程，也不得将 loading 路径一概称为 worker-thread render。

候选绘制点的局部顺序是 **0x9560C6 / RVA 0x5560C6** 的间接调用，bytes **FF 50 14**，随后 **0x9560C9** 的 push 1 与 **0x9560CB** 设置 ECX=0xC798E0，之后 **0x9560D0 / RVA 0x5560D0** 调用 0xA19180，bytes **E8 AB 30 0C 00**。它表示最终屏幕合成调用之后、A19180 最终 present/flush 调用之前的 pre-call 窗口；报告只记录 dispatch offset，不借此声称完整 vtable 布局。此时是否满足 target 0、局部 pass、颜色栈及状态保存恢复仍须由绘制实现研究逐项确认；这里不把它称为房间内正确遮挡层，也不声称 callback 返回表示 GPU 已完成。

## Window、TLS 与 WGL current 的分离

以下 VA 来自 followup 导出；RVA=VA−0x400000。

| 证据 | VA 或对象字段（并列字段地址各自都是 VA） | 静态含义及边界 |
| --- | --- | --- |
| renderer 当前 window 字段 | **0xC798E0 + 0xBC = 0xC7999C** | renderer 对象中的 window 指针槽；不是 HWND、HDC 或 HGLRC。A19DF0 以 renderer+0xBC 读写 window。 |
| TLS allocated / index | **0xC74B80 / 0xC74B84** | low DWORD 是 GLFW TLS allocated 状态，high DWORD 是 TLS index。A6A420（VA 0xA6A420 / RVA 0x66A420）分配该 TLS；A19DF0 的 VA 0xA1A046 / RVA 0x61A046 用 index 调 TlsGetValue。 |
| 窗口 make-current callback | window **+0x22C** | A19DF0/A6BBB0 经 callback 绑定或解绑 TLS current window；它是有副作用的 setter 路线。 |
| 窗口句柄字段 | window **+0x264 / +0x268** | A7FA50 的静态伪代码从偏移 +612/+616 读取 HDC/HGLRC，再调用 wglMakeCurrent。 |
| 动态 WGL pointer cells | **0xC75D50 / 0xC75D54 / 0xC75D58** | A7FCE0（VA 0xA7FCE0 / RVA 0x67FCE0）分别保存 wglGetCurrentDC、wglGetCurrentContext、wglMakeCurrent 的 GetProcAddress 结果。IDB 数据 bytes 不提供这些运行时 pointer 值。 |

A19DF0（VA **0xA19DF0 / RVA 0x619DF0**）读 TLS current window，并将目标 window 保存到 renderer+0xBC；随后会依照 callback/旧 window 状态解绑或绑定。A6BBB0（VA **0xA6BBB0 / RVA 0x66BBB0**）先读 TLS，之后可能调用旧 window callback(0) 或新 window callback。它不是可用于 guard 的纯 getter；guard 不得调用它来“读取” current，因为会切换 context。只在调用线程通过相同 TLS index 做原生 TlsGetValue 才是建议的无切换读取方式。

A7FA50（VA **0xA7FA50 / RVA 0x67FA50**）在 wglMakeCurrent 成功后才 TlsSetValue(index, window)。make-current 失败时它会记录错误并走清 TLS 的路径；因此 TLS 指针只表示 GLFW 的线程局部 bookkeeping，不能替代 OS current 查询。A6B570（VA **0xA6B570 / RVA 0x66B570**）在初始化流程中检查 TlsGetValue(index)==目标 window，再读取 GL entry points；该一次性初始化检查不是对以后每次 render 的保证。

A7FCE0 从 opengl32.dll/GetProcAddress 装载三种 WGL 函数；其 dummy-context 初始化在 **0xA7FE68** 调 wglGetCurrentDC、**0xA7FE71** 调 wglGetCurrentContext，临时切换后在 **0xA7FE9B** 恢复原句柄。这里证明程序已有 getter 函数入口和用途是保存/恢复初始化线程的 current pair；没有证明渲染调用点持续做当前状态验证。

A18E90（VA **0xA18E90 / RVA 0x618E90**）在初始化成功路径设置全局 dword_C37984 bit 0（**0xA190F9 / RVA 0x6190F9**, bytes **83 0D 84 79 C3 00 01**），随后在 **0xA19102 / RVA 0x619102** 以 **83 4E 04 01** 设置 renderer+4 bit0。C37984 是图形资源/cache 初始化相关状态，不能单独命名为 renderer initialized；未来 guard 应明确检查 renderer 自身标志和资源 owner。二者都不是 OS current HDC/HGLRC 证据。故“TLS current window==renderer window”与初始化标志为真仍不够，必须将调用线程 wglGetCurrentDC()/wglGetCurrentContext() 的结果分别与当前窗口 +0x264/+0x268 比较。

## 未来 fail-closed guard 的明确输入

下列顺序是 **DESIGN_PROPOSAL**，没有实现或运行。它只授权在一个同步本地 draw 窗口中读取/绘制，不授予跨线程操作、强制 MakeCurrent 或 teardown 的能力。

1. **companion/loop owner**：要求 CompanionHealthy() 为真；CurrentObservationContext 的 main_thread_id 非零且等于当前 thread_id；phase 为 Render。phase_depth 只按经验证 outer-render-cookie 所期待的深度作辅助约束，不单独证明入口。cookie 必须绑定普通 callsite **0x9312F1** 对应的 Hook 调用，不得仅由“当前处于 Render phase”创建。
2. **render/lifecycle 稳定性**：要求显式 outer-render-cookie 有效、正在加载为 false、目标 generation/room token 在 guard 前后未变、自有 draw depth 为零且无正在退休的实例。当前源码没有这些 Sprite 生命周期字段；在它们实现前，这部分为 UNKNOWN，应跳过 draw。
3. **renderer/window/TLS bookkeeping**：要求 renderer 初始化标志有效，renderer+0xBC 是预期且稳定的 window；TLS allocated 标志为真、index 可读；本线程 TlsGetValue(index) 非空且等于同一个预期 window。不要调用 A6BBB0 或 window+0x22C callback 来达成检查。
4. **OS current pair**：要求 window+0x264 的 HDC、window+0x268 的 HGLRC 均非空，C75D50/C75D54 getter pointer 已解析。仅在当前调用线程分别调用 wglGetCurrentDC 与 wglGetCurrentContext，并要求两个返回值与预期窗口句柄逐项相等。任何 getter 不可用、空值或不匹配都不是可修复信号；不得在 guard 内调用 wglMakeCurrent。
5. **短有效期**：guard 只覆盖同步 draw 子调用。任何可能执行 context callback、native reentry 或 lifecycle mutation 的调用后都要重新核验或结束该窗口；确保从 guard 到 Sprite draw dispatch 之间没有未审阅回调。此检查不会证明 GPU idle，也不把 present/flush 返回解释为 GPU 全部完成。

建议的 fail-closed 分类为诊断名称而非项目门：缺 shared context/健康状态/非零 owner ID、phase/cookie 不合、loading/generation/draw-depth 未知 → **OwnerWindowUnknown**；TLS 与 window 不一致或实际 HDC/HGLRC 不一致 → **ContextMismatch**；WGL getter pointer 未解析、renderer/window 字段无法安全读取或 native callback/API 结果含糊 → **NativeOutcomeUnknown**。所有这类分支在读取资源后态或发出 draw 前停止，不尝试修正线程或切换 context。

## 未决项与范围

1. 本轮没有获得真实 GetCurrentThreadId、TLS current window、wglGetCurrentDC、wglGetCurrentContext 或 WGL API 成功/失败的运行时样本；它们均为 **UNKNOWN**。
2. 9555C0 的加载等待环 direct call 实际由哪个线程执行未知；`0x8FC32C` 只证明静态调用位置，任务线程的存在不证明调用线程。
3. Phase/depth 无法区分 outer normal frame 与继承其 TLS phase 的重入 loading path。future outer-render-cookie、独立 draw depth 及 generation/loading guard 还没有代码。
4. 运行时 getter 句柄可能未解析、返回空或与窗口字段不符；future guard 必须 fail closed。外部 WGL/context 操作和任意 callback 后状态仍未知。
5. 这里没有实现 sprite pass 状态保存/恢复、目标/颜色栈判断、房间遮挡、资源像素匹配、跨线程 retire 或 GPU 完成检测。ImagePng batch image/control 引用及 flush 语义归 tx-03-19 主线的对应研究，不由本报告外推。

上下文沿用 [tx-03-14 renderer/context lifecycle](tx-03-14-renderer-context-lifecycle.md) 与 [tx-03-18 single-Sprite adapter contract](tx-03-18-single-sprite-adapter-contract.md)。本轮只编辑本子报告和 artifacts/cs-t0/tx-03-19-static 下 luna-* 新证据；未运行游戏、native helper、测试或构建，未修改 IDB、Hook 或生产代码。整体静态文档审阅由主智能体按任务约定执行。
