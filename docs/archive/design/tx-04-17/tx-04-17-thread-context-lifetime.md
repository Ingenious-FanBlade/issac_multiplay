# tx-04-17 ST6：主线程、graphics context、加载旁路与在途绘制寿命

## 2026-11-01 当前设计与补充研究

现行合同与执行顺序见[tx-04-17主流程](tx-04-17-workflow.md)。R1／R2已形成初始化范围与对象生命周期合同，R3分开目标应用／排序、相机／HUD缓存与render窗口；本轮[R4资源与在途寿命](tx-04-17-native-resource-lifetime.md)连接原生owner、资源变化、Image强引用，并明确custom-render context分支当前关闭。R5已补[失败退休与菜单交还](tx-04-17-failure-retirement.md)，R6已形成[模型／角色／应用器归并](tx-04-17-model-role-applier-integration.md)，具体接线与原生UNKNOWN仍未实现，本轮没有修改代码、构建或运行测试。

原生Client世界的create／remove／change、资源及Lua操作仍限定在已解释主线程阶段，后台仅处理自有值。原生实例、资源和render队列的寿命必须接在同一条实际使用链上；本轮没有证明新的清理或context保证。

- [R3](tx-04-17-workflow.md#r3)选定保留的绘制／维护入口后，[R4](tx-04-17-workflow.md#r4)连接实例替换、原生destructor、队列raw引用、最后资源释放与实际context；复用已知加载旁路、WGL／TLS及queue事实，不重做整个renderer。
- R4补充：官方`LuaRender.cpp:30,3139`以EnableCustomRendering=false关闭Allocate复制Entity／Animation context，`ASMPatches.cpp:272`也注释绘制patch入口，故其deferred副表不是当前活跃owner，只记条件风险。native Image pin可在已知owner阶段以类型化smart-pointer值释放，但不保活父实例／ANM，也不解决既有LuaImage的GC时点。实际raw队列／活跃栈的全部取消与异常路径仍UNKNOWN并交R5，不从普通路径外推。
- [R5](tx-04-17-workflow.md#r5)扩展准备失败、部分change、remove后延迟回收、停显／重建、stop和Shutdown的收口。逻辑remove／原生提交不等于物理释放，CPU队列清理不等于GPU完成。
- 不为得到原子切换而建立第二个完整原生世界；在已存在世界部分写入后失败应停显、保持隔离并按实际owner清理。线程／context或回收状态不明时停止相关新使用，不假称通用异常恢复。

## 2026-10-31 历史研究正文

以下保留当时事实、纠错、UNKNOWN和设计推导；冲突设计按上文及主流程执行，静态结论不升级为运行结果，历史分工不成为新要求。


日期：2026-10-31，Asia/Singapore。状态：**ST6指定J273静态研究、与ST5／ST7来回互核及主智能体审阅完成，具体UNKNOWN保留**。本项由一个GPT-6 Luna／max子智能体推进；普通frame queue reset／remove与正常最后strong→Image析构→GL删除分别核定，异常覆盖和实机context仍未知。纠错与接受范围见[工作流程主审记录](tx-04-17-workflow.md)。本文是tx-04-17的设计输入，不是资源运行时实现或E2E验收。

## 1 范围与证据口径

- `PUBLIC_SOURCE`：官方 REPENTOGON 1.1.3 源码与 libzhl 声明，用于公开接口wrapper、owner字段和函数签名。
- `J273_STATIC`：只读IDA静态观察，目标为 v1.9.7.12.J273，imagebase `0x400000`；当前IDB路径与ST2记录一致。本次未切换或写IDB。文内VA均可用`RVA = VA - 0x400000`换算。
- `J460_INDEX`：03-13／14／19只作查找主题与旧问题索引；其中函数地址、布局、批次引用结论不移植为J273事实。
- `DESIGN`／`UNKNOWN`：设计合同／静态资料不能证明的实际运行状态。

## 2 J273渲染与初始化入口

`J273_STATIC`：ZHL签名匹配的`Manager::Init`为VA `0x92BA20`（RVA `0x52BA20`），`Manager::Render`为VA `0x92EC40`（RVA `0x52EC40`）。`Manager::Render`在`0x92EE15`调用`Game::Render` VA `0x6F7A80`；Game路径随后进入Room、Entity、PostRender Lua及HUD绘制，详情见[ST4](tx-04-17-render-presentation-boundary.md)。进入此函数不等于执行纯绘制，也不构成graphics context许可。

IDA当前直接xrefs显示`Manager::Render`至少有两处调用：主循环`_main` VA `0x90AC60`于`0x90AF0D`调用；启动／资源处理函数`sub_8D7ED0`于`0x8D9228`调用。后者由`sub_981AE0`同步调用，包含mod metadata／更新字符串与等待路径。因此，单独覆盖主循环render callsite不能覆盖全部Manager::Render入口；而另一个入口的存在也不证明其每次都持有目标OS context。函数指针、Lua callback名称或本地render phase都不能替代实际OS current检查。

`Manager::Init`自身包含`SwapBuffers` VA `0x92BE57`及GL路径，说明它依赖已初始化的图形环境；它不等于创建context的完整接口。`_main`在调用`sub_981AE0`前于`0x90AD70`调用`sub_9EBCB0(epoxy_handle_external_wglMakeCurrent, 0)`。这些静态调用不证明特定安装下的实机线程、当前HGLRC或context生命周期。

## 3 J273窗口与OS context切换证据

`J273_STATIC`：当前renderer结构定义见官方[KAGE_Graphics_Manager.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Manager.zhl)：`_window`位于renderer `+0xE0`；所声明的`_GLFWwindow`大小为`0x258`，HWND位于`+0x218`。J273窗口代码读写的HDC/HGLRC字段分别在window `+0x234`／`+0x238`；这是本目标二进制的字段观察，不能将旧J460 WGL布局或偏移机械套用。REPENTOGON ZHL仅声明HWND，未给HDC/HGLRC命名字段。

J273中`wglMakeCurrent`是导入项`0xAAA370`，已见直接调用包括：

| 点位 | 静态事实 | 限定 |
| --- | --- | --- |
| `sub_A3ABA0`：`0xA3AE7C`、`0xA3AEF2` | WGL窗口路径调用`wglCreateContext`与`wglMakeCurrent`。 | 初始化和失败分支不代表后来调用线程仍持有同一context。 |
| `sub_9E4970`：`0x9E4B63` | 渲染/窗口管理路径有共享的`wglMakeCurrent`调用；随后`0x9E4B70`写线程TLS。 | `0x9E4B5D`为跳转点而非第二次MakeCurrent；该路径不检查MakeCurrent返回值就写TLS。TLS值因此不能证明OS调用成功。 |
| `sub_A29010`：`0xA29B80`、`0xA29BDE`等 | Win32窗口消息路径也可切换current context。 | Lua或渲染回调进入不排除此前有窗口／ImGui context切换。 |
| `sub_A29FF0`：`0xA29FFE` | 调`wglDeleteContext`；其后`0xA2A004`清HGLRC字段，再ReleaseDC与DestroyWindow。 | 不按删除API返回值保留句柄；不能据字段已清证明OS删除成功。 |

`PUBLIC_SOURCE`：REPENTOGON [`ImGui.cpp`](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/ImGuiFeatures/ImGui.cpp:686)在平台多视口操作前保存`wglGetCurrentDC/Context`，调用`UpdatePlatformWindows`／`RenderPlatformWindowsDefault`后尝试用`wglMakeCurrent`恢复；另有`wglSwapBuffers` hook路径。正常路径中的恢复不证明失败、异常或实际hook顺序下current一定相符。`wglGetCurrentContext/DC`在J273 IDB可见字符串，但本次没有找到其目标代码引用；不能声称引擎已有逐操作OS-current guard。

**运行时检查合同（DESIGN）**：每个可能进入native／Lua／GL的加载、资源查询、姿态绑定、绘制与最后释放操作，都要在同一主线程owner阶段检查：当前线程等于已学习的游戏主线程；调用来源与Client资源generation仍有效；存在明确renderer/window owner；用只读OS getter取得的当前HDC/HGLRC与该owner的存活句柄及context generation一致。正常主循环和`sub_8D7ED0`加载旁路要分别标记owner窗口并各自通过同一检查；loading bypass不是豁免。context teardown、未知render pass、未知窗口期、无current值、句柄失配或owner不明时，跳过操作并维持Client隔离。不要调用`wglMakeCurrent`“修复”检查失败。FrameBuffer/render target与WGL context是不同状态，FBO绑定不能证明context current。

当前工程`IsMain`只比较已学习的`MainThread`（`mod/src/bootstrap/mod_init.cpp`），`hooks/context.cpp`与`game/context.cpp`管理的是自有phase／owner栈；它们不读取OS HDC/HGLRC。现有TLS/current-window字段也不是OS current getter。运行时须分别保留线程、owner phase、renderer generation、OS current四类观测。

## 4 图像引用、共享batch与在途提交

`J273_STATIC`：ANM2 `Render`签名匹配VA `0x4059E0`；ImageBase `Render`匹配VA `0x9E2170`，进入`0x9E2460`。后续`0x9E2C8C`将`ImageBase*`写入图形ImageManager的`_frameImages`向量。ImageBase `get_batch(bool)`匹配VA `0x9E2EC0`，可取得或分配`0x3C`大小的render batch。

`PUBLIC_SOURCE`：[`KAGE_Graphics_Image.zhl`](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Image.zhl)将ImageBase的active／reusable batch容器声明为`vector<RenderBatch*>`；[`KAGE_Graphics_Vertex.zhl`](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_Vertex.zhl)中的`KAGE_Graphics_RenderBatch`仅声明shader及vertex/index buffers；[`KAGE_Graphics_ImageManager.zhl`](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/KAGE_Graphics_ImageManager.zhl)的透明队列是`pair<RenderBatch*, ImageBase*>`原始指针。J273这些声明未显示J460 `batch+8/+C`形态的独立SmartPointer image pair；不能沿用03-19 J460的batch强引用结论。

`apply_frame_images`按J273 ZHL特征匹配VA `0x9EFA70`：它遍历`0xBCE570..0xBCE574`的ImageBase指针队列，逐个调用image vtable `+0x8C`（`0x9EFB1A`）；再遍历`0xBCE588..0xBCE58C`透明batch/image指针队列，调用image vtable `+0x84`（`0x9EFC16`），并在`0x9EFD23`发出`glDrawElements`。其尾部`0x9EFDEB`只将透明pair向量end恢复为begin，另清当前透明batch相关状态；该函数没有把普通`_frameImages`的begin/end归零。

**普通队列reset和图像移除（J273_STATIC）**：`sub_9EFA10`遍历普通`_frameImages`，逐个清ImageBase flag bit 4（`0x9EFA2C`），并将`dword_BCE574`写回begin `dword_BCE570`（`0x9EFA47`）。其静态调用者包含`Manager::Init` `0x92BA20`、`Manager::Render` `0x92EC40`、`sub_9E4140`、`sub_9E4170`及`sub_9E5340`。REPENTOGON的`Present`签名匹配`sub_9E4170`：先调`apply_frame_images` `0x9E417A`，再reset `0x9E417F`，接着调renderer virtual method（`0x9E4199`），最后才按条件走`SwapBuffers` `0x9E41C8`或其他分支。`PresentWithoutSwap`另有apply→reset路径。这里闭合的是若干正常CPU队列边界；静态caller列表不能证明所有旁路、异常和hook安装路径都覆盖，也不证明驱动或GPU已完成。

`sub_9EFE60`是ImageManager cache／queue removal callback，不是Image析构函数。它在mutex内读strong count；计数为1时于`0x9EFE90`调`sub_9EF7C0`。该移除helper的反汇编显示：ImageBase flag bit 4时从`_frameImages`匹配项memmove并在`0x9EF84F`将end减4；flag bit 8时从另一tracked vector移除（`0x9EF884`／`0x9EF88F`）；还按image从透明pair队列移除（`0x9EF8D5`／`0x9EF8E8`），之后检查／移除ImageManager cache关联并释放相关容器项。`0x9EF7C0`的Hex-Rays伪码报告bad stack pointer，故这里只以对应反汇编动作作有限判断。它证明存在按image的CPU queue/cache移除路径，不等于已证明某旧revision在所有队列均不再被引用。

`KAGE_SmartPointer_ImageBase`公开实现会在拷贝时增加强引用，`DecrRef`在最后引用时转入Free路径；Lua `Image` userdata的`__gc`也显式调用`DecrRef`（`LuaRender.cpp:228`）。但这些强引用不自动覆盖上述raw `_frameImages`／batch指针队列。`LayerState::GetSpriteSheet`及Lua Sprite／LayerState的相关getter还会lazy sync filter/wrap，不能因函数名为Get就当作无图形副作用的纯读取。

**最后强引用到GL清理（J273_STATIC）**：官方`SmartPointer.zhl`的`KAGE_SmartPointer_ImageBase::DecrRef`签名匹配`sub_4081A0`。该函数在`0x4081D1`调用counter vtable `+0x0C`（`DecrementReference`），仅当该调用返回true且`psub_9EFE60`非空时才于`0x4081E1`走可选cache callback；这个条件不等于析构。IDA在`0xAAC4D4`读到`ReferenceCount<ImageBase>`六槽表`0x408CA0/0x408580/0x408560/0x408500/0x4084E0/0x408490`。其`DecrementReference` `sub_408500`在mutex内减uint16 strong count；由1变0时，在`0x408536`取Image vtable `+0x60`并传`1`，在`0x408539`清control block的image指针，再于`0x408549`弱计数减一。PNG vtable `0xB3BA80+0x60`指向`sub_A346F0`，PCX vtable `0xB3B8EC+0x60`指向`sub_A340B0`；RGBA `sub_A35240`和Procedural `sub_A35640`也同样调用`sub_A4C360`。该GL cleanup helper在`0xA4C369`及`0xA4C375`两次调用`epoxy_glDeleteTextures`，随后释放数组、经image vtable `+0x74`清理并复位标记。故目标Image smart ref正常最后strong-release存在到子类析构及GL纹理删除的静态路径；`0x9EFE60→0x9EF7C0`仍只是strong count条件下独立的cache／queue移除回调，不能和最后析构混为一段。此链不证明实际运行时引用计数、删除API成功、OS current有效或GPU idle。`apply_frame_images`返回、`Manager::Render`返回和`SwapBuffers`返回都不是raw queue/GPU完成证明。

## 5 允许阶段与候选提交／退休合同

以下为设计合同，尚未实现：

1. **后台只收发有界自有值**：网络worker只解析、限量排队不可变描述与姿态值；不持有`Player*`、Sprite、Lua userdata/registry key、ImageBase*、window/HGLRC或GL资源指针，不触碰游戏对象、Lua、renderer或GL。
2. **主线程准备**：在已确认的Client主线程owner阶段创建候选。构造Sprite、Load/Reload/LoadGraphics、ReplaceSpritesheet、LayerState绑定及可能lazy sync filter/wrap的getter都先通过第3节运行检查。Prepare可失败；失败候选不发布，旧展示保持持有且仍隔离玩法。
3. **原子发布**：先完成所有必要资源与图层准备，再发布新generation。每次使用读一致generation；候选半成品不对draw consumer可见。切换时旧generation立即禁止产生新draw，但旧Sprite/LayerState/Image强引用继续保留。
4. **在途batch排空**：只允许负责该render/batch owner的主线程阶段排空。J273已有`sub_9EFA10`普通frame queue reset，以及`sub_9EF7C0`按Image移除普通／另一tracked vector／透明pair和cache关联的静态helper；但需确认具体旧generation的图像已从相关owner移除，不能只看到全局队列一次归零。逻辑swap可以先于raw queue drain，但旧Sprite/Layer/Image strong refs必须留在有界退休表中直到owner证明raw引用退出。`apply_frame_images`返回、Render callback返回或SwapBuffers后不能立即释放；如无法观测具体owner清理，停止继续发布新资源revision，并保留受预算限制的旧引用，不能无限积压或在resource/context owner失效后析构。
5. **Lua／Game借用退休**：在相关Game、Manager和ANM2/cache owner销毁前，停止本方Lua callback／draw admission，排空在途调用，撤销registry root与借用句柄，并在仍有效的owner阶段完成本方依赖这些对象的Reset／清理。J273原始`lua_close`发生在Game、Manager和`sub_408EF0`中ANM2/cache处理之后，不能把Lua finalizer当成它们仍存活的证据。
6. **停止与进程关闭**：正常Client stop先撤销输入、网络发布和新绘制，再等待主线程draw与native队列owner收尾，然后按两个不同截止点退休。J273 `Isaac::Shutdown` VA `0x982C90`在`0x982DD8`有条件调用`Exit(1)`；原始patch slot `0x982DDD`是bytes `B9 78 6F BE 00`。REPENTOGON [ASMCallbacks.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ASMPatches/ASMCallbacks.cpp:1876)源码意图在save之后、Game／Manager destructor之前发送`MC_PRE_MOD_UNLOAD`，但当前目标实际安装／命中未证。原始顺序在`0x982E1C`销毁Game、`0x982E45`销毁Manager；随后`0x982E67`调用`sub_408EF0`：先销毁ANM2缓存项（`sub_403790`），再处理一组图像引用；接着有部分shader program删除。LuaEngine destroy调用约在`0x98300B`，其中`lua_close`在`0x854FA0`，晚于Game／Manager与上述ANM2/cache处理。
7. **renderer／ImageManager／context退休**：`lua_close`之后，`_main`于`0x90AFD3`进入平台cleanup `sub_A2AD80`。其正常图形子序列在`0xA39A39`调用`sub_9EED00(&dword_BCE560)`销毁ImageManager cache（该函数会报告仍有Image引用并释放容器项），`0xA39A52`删除Framebuffer、`0xA39A5F`删除Renderbuffer，`0xA39A6B`进入`sub_A2F440`拆window；后者在匹配TLS window时调`wglMakeCurrent(0,0)`再进入`sub_A29FF0`删除context、释放DC并销毁window，末尾清renderer window字段。这里给出静态析构顺序，不代表TLS反映OS current或WGL API成功。自有raw queue和最后Image强引用必须在ImageManager／context关闭前按有效owner与OS-current合同清理；原始main顺序允许`lua_close`在ImageManager/context teardown前发生，但不能把`lua_close`本身当作已排空batch或OS-current正确的证明。
8. **停用条件**：操作阶段不明、线程或HDC/HGLRC检查不符、context generation变化、游戏Lua正在关闭、队列清理点未成立或render重入时，停止自有加载／绘制／释放；不恢复旧对象给本地Authority模拟，也不为通过guard主动切换OS context。

## 6 统一E2E运行观察留项

完成17整体实现后，E8/E10需观察：实际主线程ID、正常和`sub_8D7ED0`加载render路径；每个资源操作前的OS current DC/context与renderer generation；REPENTOGON多视口／外部切换前后检查；ANM2/LayerState/Image引用变化、普通/透明batch入队及其具体reset/remove owner；最后strong DecrRef到Image子类`+0x60` destructor和GL delete的实际命中；stop和Shutdown时registry/userdata/native batch/image引用清理顺序；ImageManager cache清理与窗口context删除前没有自有队列或借用仍在使用。静态研究不能声称这些运行时条件已满足，也不能据单次无崩溃推定所有路径。

## 7 未闭合范围与互核请求

- `UNKNOWN`：普通`_frameImages`已有reset/remove helper（`0x9EFA10`／`0x9EF7C0`）及若干正常caller；但具体旧generation从所有batch/cache owner退尽的运行时可观测条件、所有绕行／异常路径覆盖仍未闭合。`sub_9EFE60→sub_9EF7C0`是带计数条件的队列/cache移除路径，不能标成最后ImageBase释放链。
- `UNKNOWN`：静态已核正常strong 1→0到Image vtable `+0x60` destructor并在`0xA4C360`删除GL texture；实际运行是否命中该链、实际OS context、异常清理／GL错误与GPU完成仍未知。
- `UNKNOWN`：两处已见`Manager::Render` caller不是完整动态caller集合；实际安装的Hook命中与loading／退出路径可达性留E2E。
- `UNKNOWN`：读取OS current getter本身在当前安装的解析／可调用状态，实际HDC/HGLRC及window/context generation；未看见的异常／SEH／GL错误／重入恢复。
- `UNKNOWN`：实际Shutdown/stop安装下两个截止点的时序与OS-current状态。静态原始链区分Game／Manager/ANM2 cache处理（`0x982E1C`、`0x982E45`、`0x982E67`附近）与之后Lua `lua_close`（`0x854FA0`）及`_main`平台/renderer/ImageManager/context teardown（`0x90AFD3`、`0xA39A39`、`0xA39A6B`）；源码early unload patch是否安装／命中仍未知。

本文不证明Replica世界壳可构造、完整Render安全、Client gameplay隔离、像素正确或GPU完成；仅提供静态线程/context与资源引用合同，等待全目标实现后的统一E2E。未运行游戏、构建、测试、哈希／gate、离线校验或图形实验。
