# tx-03-14：renderer 线程来源与 GL context 生命周期

日期：2026-10-22（Asia/Shanghai）。类型：静态逆向。承接 [tx-03-13 图像资源研究](tx-03-13-sprite-image-resources.md)，本轮目标是连接 renderer 初始化、current context、帧内分派与正常退出，解释独立 Sprite 资源操作应依附的生命周期。用户指定的 GPT-6 Luna / max 子任务负责关闭链，主智能体研究初始化／帧循环并独立复核关闭结论。见 [主审记录](tx-03-14-primary-review.md)和 [Luna 报告](tx-03-14-luna-renderer-teardown.md)。

## 本轮结论

**正常启动及外层帧循环有一条明确的 `_main` 同步调用链，WGL current context 则由线程局部存储维护。** 初始化不是只产生一个全局 renderer 指针：它创建窗口／context、调用 WGL make-current，并更新本线程 TLS；后续正常主循环同步执行 update、render。该结果把上一轮的“实际线程未知”细化为可追溯的正常入口来源，仍没有测量运行时 thread ID／HGLRC，也没有证明任意回调后 context 都不变。

**正常关闭中，图形缓存减引用和 framebuffer 清理在窗口／WGL context 删除请求之前。** 但清 current 只检查调用线程的 TLS，未见跨线程解绑协议；缓存摘除也不能证明其它 Sprite／pair 别名均已释放。未来自有 Sprite 必须在图形后端仍存活时退休，不能等到 DLL 恢复或 context 关闭后再任意释放。

另发现启动期间的 Mod 更新／加载等待循环会直接调用同一 render owner，绕过现有仅替换外层 callsite 的 `RenderHook`。因此不能把“每次 render”当作普通 Authority tick，也不能把该 Hook 当作所有绘制入口的拦截器。本轮未实现资源队列、context 检查器或 Replica。

## 数据与证据口径

活动 IDB 为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`，记录 input path 为同目录 EXE；imagebase=`0x400000`。VA 指该 IDB 地址，RVA=`VA-0x400000`；renderer／window 字段 offset 不属于 RVA，也不把 window 内函数指针字段当成 C++ vtable。静态路径不代替实际分支、OS 调用成功或运行时线程观察。

`STATIC_CONFIRMED` 为函数、原始指令及数据直接可见；`STATIC_DERIVED` 为据此推导的正常所有权与候选接入时段；未关闭的条件保留 `UNKNOWN`。没有计算／校验任何哈希，未比较磁盘文件与 IDB 内容。原始资料在 [tx-03-14-static](../../../artifacts/cs-t0/tx-03-14-static/)。

## 初始化：从 `_main` 到 current context

主审证据：[入口与窗口辅助函数](../../../artifacts/cs-t0/tx-03-14-static/primary-context-initial.json)、[启动与帧路径](../../../artifacts/cs-t0/tx-03-14-static/primary-context-paths.json)、[WGL 函数](../../../artifacts/cs-t0/tx-03-14-static/primary-wgl-current.json)、[平台分派](../../../artifacts/cs-t0/tx-03-14-static/primary-window-dispatch.json)、[平台／TLS 安装](../../../artifacts/cs-t0/tx-03-14-static/primary-platform-install.json)。

```text
_main 0x931050
  -> 0xA5F0E0 -> 0xA710A0
  -> 0xA18E90(renderer = 0xC798E0)
       -> 0xA6A420: 平台与 TLS 初始化
       -> 0xA19DF0: 创建窗口／context并设置本线程 current
       -> shader／图形缓存初始化、renderer initialized 位
  -> 0x9AA040: 游戏启动，可能进入加载期间的另一个 render 循环
  -> 普通主循环：update 0x9AB6D0 -> render 0x9555C0
```

上述是同步调用来源，不是额外启动一个 renderer worker 的证据。`0xA710A0` 后面确有 command-thread 初始化，不能据此认定进程只有一个线程，或全部资源路径都由主循环调用。

| 证据点 VA（RVA）／bytes | 直接事实 |
| --- | --- |
| `0x931155`（`0x531155`）／`E8 86 DF 12 00` | `_main` 调 `0xA5F0E0`，后者同步调用 `0xA710A0`。 |
| `0xA71284`（`0x671284`）／`E8 07 7C FA FF` | `0xA710A0` 调 `0xA18E90`，receiver 为全局 renderer `0xC798E0`（RVA `0x8798E0`）。 |
| `0xA6A4BE`（`0x66A4BE`）／`B9 00 00 06 00` | 实际 ECX=`0x60000`，平台自动选择入口；不能采信伪代码表面的 `A9DB50(0)`。目标选择 Windows 安装器 `0xA81050`，写入平台 ID `0x60001` 与 dispatch。 |
| `0xA81190`（`0x681190`）／`C7 85 54 FF FF FF 90 D3 A5 00` | Windows 平台表的第 29 个 DWORD 写 `0xA5D390`；安装至 `0xC73694` 后，对应 create-window callback `0xC73708`。 |

`0xA19DF0` 调 `0xA6A9B0` 创建窗口；`0xA6A9B0` 经平台 callback 到 `0xA5D390`。所读 Windows/WGL 分支由 `0xA7FCE0` 装载 `opengl32.dll`／解析入口，`0xA80010` 创建真实 context 并安装 window 函数字段；还有 EGL／其它 backend 分支，本轮没有宣称它们与 WGL 完全等价或已在运行时选中。

`0xA6B570` 在检查 context 时临时使新窗口 current，读 GL 入口／版本，正常尾部再恢复保存的旧 current；随后 `0xA19DF0` 才把选定窗口设为后续使用的 current。前者不是永久 owner 绑定，也不能只凭初始化期间曾 current 就推断帧循环 current 已准备好。WGL loader 还会创建临时 dummy context；该 dummy 与游戏窗口的真实 HGLRC 不是同一对象。

### 三种对象和 current 记录

原始证据：[WGL 安装与帧目标](../../../artifacts/cs-t0/tx-03-14-static/primary-wgl-install.json)、上述 WGL／TLS 导出。

| receiver／字段 | 当前静态含义 |
| --- | --- |
| KAGE renderer=`0xC798E0`，`+0xBC` | 当前 GLFW window 指针；该全局字段地址为 `0xC7999C`（RVA `0x87999C`）。不是 HWND 或 HGLRC。 |
| GLFW window `+0x22C` | make-current callback；WGL 分支 `0xA804B0` 写 `0xA7FA50`。 |
| GLFW window `+0x230` | swap callback；`0xA804BF` 写 `0xA7FB00`。 |
| GLFW window `+0x240` | destroy-context callback；`0xA804E7` 写 `0xA7FCB0`。以上均为对象内函数指针字段，不是 vptr 下的槽。 |
| GLFW window `+0x264/+0x268/+0x2B4` | WGL 分支分别使用 HDC／HGLRC／Win32 HWND。不能将这些句柄放入网络资源身份。 |
| 全局 `0xC74B80/+4` | TLS allocated 状态／TLS index；不是一个全进程共用的“当前窗口指针”。`0xA9DAD0` 分配 TLS index，current 窗口值由本线程 `TlsGetValue/TlsSetValue` 访问。 |
| `0xC75D58` | 从 `GetProcAddress(...,"wglMakeCurrent")` 得到的动态函数指针；静态字节为 FF，不当作运行时函数值。 |

`0xA19DF0` 在 `0xA1A046` 调 `TlsGetValue`，必要时经旧窗口 callback 解绑，再经新窗口 `+0x22C` 设置 current。WGL 实现 `0xA7FA50` 传 window 的 HDC／HGLRC 调原生 make-current，成功后把 window 写到调用线程 TLS；传空窗口走解绑。失败分支有诊断和清 TLS 的行为，不能把“TLS 已写”与“OS 必然切换成功”或“其它线程已解绑”合并。

因此全局 renderer/window 指针非空并不能证明调用线程可以操作图像。外部直接 WGL 调用、context 切换失败或未研究的 callback 还可能使 TLS／OS 状态关系发生变化，本文不据静态代码构造一个始终成立的运行时 guard。

## 帧内路径：FBO 切换不是 context 切换

`_main` 在 `0x9312EC` 同步执行 update，`0x9312F1`（RVA `0x5312F1`，`E8 CA 42 02 00`）调 render owner `0x9555C0`。owner 在多个位置调用 `0xA19180`；该函数先处理绘制批次／状态与临时记录，再按参数和当前 render target 分支，选择清理离屏目标或经 window+`0x230` 交换缓冲。不能把它概括为每次调用都 SwapBuffers，更不能把 SwapBuffers 返回当作 GPU 全部工作完成。

WGL `0xA7FB00` 最后在 `0xA7FB73` 调 `SwapBuffers(window.HDC)`。`0xA18300` 则写 renderer+`0x68` 的目标字段并调用 framebuffer／viewport 等 GL 入口；`0xA18230` 清目标及部分绘制状态。它们处理 FBO／批次，所读函数不负责把正确 context 自动转移到当前线程。独立 Sprite 的资源 owner 不能由“当前正在 render”或“已绑定一个 FBO”代替。

### 另一个 render 入口与现有 Hook 的范围

[替代 render 调用者](../../../artifacts/cs-t0/tx-03-14-static/primary-render-alternate.json)与 [启动调用者](../../../artifacts/cs-t0/tx-03-14-static/primary-window-dispatch.json)：

```text
_main -> 0x9AA040
  0x9AAAE3 -> 0x8FB120
    加载 gfx/ui/loading.anm2、启动任务、等待任务完成
    0x8FC32C -> 0x9555C0
```

`0x8FC32C`（RVA `0x4FC32C`）为 `E8 8F 92 05 00`。这是调用者等待循环中的直接绘制，不能因为函数内启动了另一个任务，就称 render 本身在该 worker 上执行。该路线可在普通外层帧循环前发生，不自动拥有当前有效 Authority tick／Replica apply 时段。

现有 [loop_runtime.cpp](../../../cs-t0/native/src/loop_runtime.cpp) 的 `kCallRvas[7]=0x5312F1` 与 `RenderHook → InvokeVoid(g_render,...)` 只替换该主循环 callsite；不是对 `0x9555C0` 函数所有入口做 detour。所见加载调用点不在该槽的覆盖范围。当前两处静态直接 xref也不等于全部动态调用者集合；本轮没有新增 Hook。

## 正常关闭：资源先清理，随后解绑／删除窗口 context

主审独立证据：[关闭复核](../../../artifacts/cs-t0/tx-03-14-static/primary-teardown-review.json)、[图像缓存树释放](../../../artifacts/cs-t0/tx-03-14-static/primary-cache-drain.json)；子任务证据与完整说明见 [Luna 报告](tx-03-14-luna-renderer-teardown.md)。

`_main` 正常退出先在 `0x9313DE` 调 `0x9AB700` 处理游戏关闭，随后在 `0x93140E` 调 `0xA71770`。后者还包含服务、任务及其它 manager 清理；本文只以真实地址说明其中的图形子序列，不把整个引擎关闭函数作为自有 Sprite 的析构接口。另一个 renderer 关闭入口 `0xA19130` 也具有下列正常图形顺序：

| 顺序 | 静态操作及限定 |
| --- | --- |
| 1 | `0xA13060`（RVA `0x613060`）清 renderer 相关对象和图形缓存。仅在 `0xC37984` bit0 置位分支，`0xA131A1` 清 `0xC7163C`，再经 `0x415800` 释放图像缓存树，最后销毁缓存同步状态。 |
| 2 | `0x415800` 对 node+`0x18` control 调 `+0x0C`，随后按返回值与全局回调条件处理，释放节点。全局回调已清零不阻止 control 自己减计数／删除 image；图像最后引用的正常删除链见 tx-03-13。树外引用仍可能保活 image。 |
| 3 | `0xA19130` 或 `0xA71770` 随后调用 GL framebuffer／renderbuffer 删除，再调用 `0xA6AF90(window)`。正常图形资源清理因此在 context 删除请求之前。 |
| 4 | `0xA6AF90` 仅当调用线程 TLS current 等于该 window 时，经 window+`0x22C(0)` 解绑；没有在此见到跨线程迁移、其它线程解绑或全部资源使用者退出的协议。 |
| 5 | Windows destroy callback `0xA5D4E0` 经 window+`0x240` 到 `0xA7FCB0`：调用 `wglDeleteContext(window+0x268)`，正常返回后清该字段。代码没有按 API 返回值决定是否保留句柄，不能据此保证 OS 删除成功。 |
| 6 | `0xA5D4E0` 再调用 Win32 `DestroyWindow` 并清 HWND；上层清 renderer+`0xBC`。`0xA71770` 后面仍有其它对象清理，不能将所有晚期虚析构一概认定不会涉及图形资源。 |

`0xA69C00` 是另一条更广的窗口／平台／TLS 清理函数：会逐个销毁窗口、释放 TLS index 等。但当前静态直接 xref显示 `0xA6A420` 初始化失败路线调用它；本轮未证明正常 `_main → A71770` 还会执行它。不能把“正常关闭窗口”写成“已观察完整 GLFW terminate”，也不能为自有对象主动调用这条全局清理路线。

## 对独立 Sprite 的接入含义

以下是 `STATIC_DERIVED` 的候选设计，并非已实现、已验证的调用契约：

- 普通首版展示可在现有主循环 render callsite 附近安排自有队列消费，前提是已有主线程／session／对象代次仍有效，renderer 已初始化，且当前 window/context 与该资源 owner 一致。该时段不自动覆盖启动／加载中的其它 render 入口。
- 网络线程仅解析、入队；不直接调用 Sprite Load／换图／sampler／析构。tx-03-13 已确认这些操作能直接进入 GL，引用锁不能赋予图形上下文。
- 退休应先停掉自有绘制发布并处理正在使用的对象，再在 context 存活时按原生引用规则清理。尤其应早于 normal game／graphics teardown，而不是依赖缓存树清空替自有引用完成释放；具体接入 callback、重入及在途绘制寿命仍需实现时解释。
- window/context 重建使资源关系需要重新判断；全局 renderer 地址保持不变也不能充当 context generation。任意重载、外部 context 切换与所有 Mod 的恢复不在本轮被宣布完成。

本轮已完成正常静态调用链、TLS/current 机制、两处直接 render 入口与资源清理顺序的研究。下一项宜追 `ReplaceSpritesheet／Reload／LoadGraphics` 的替换与引用变化，再落实首批 Sprite 展示字段。实际 thread ID／HGLRC、context 切换失败、OOM／EH、worker 并发、外部别名及完整 GPU 同步仍未知。没有改变游戏逻辑、生产代码或 native 调用次数。

全部研究与编辑结束后仅执行一次 [整体静态文档审阅](tx-03-14-primary-review.md)；没有游戏输入、启动、附着、注入、断点、构建或代码测试。可重复只读提取入口为 [export_static.py](../../../artifacts/cs-t0/tx-03-14-static/export_static.py)，使用新输出路径保留原始记录。
