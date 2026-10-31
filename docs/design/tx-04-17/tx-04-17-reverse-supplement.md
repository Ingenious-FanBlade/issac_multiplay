# tx-04-17 逆向补充：展示入口、图像退休与退出隔离

本文补充 tx-04-17 的实现点位。证据来自 J273 IDA 静态分析、随仓库的 ZHL/REPENTOGON 源码和 HookSystem 实现；没有启动游戏或修改 IDA 数据库。

## Replica 展示壳

不需要构造原生 `Game` 或 `Room`。Client 展示根应由自有值对象组成：Room 几何（格子、墙、门和可见实体）、Camera 变换与视口、HUD 状态，以及明确排序的场景/前景/UI render pass。权威状态由 Room/实体适配器输入；菜单单图不构成世界展示。

构造原生 `Room` 并非无副作用的空壳：`Game` 的 Room 位于 `Game+0x18300`，对象大小 `0x7898`；Room 布局包含 descriptor、宽高、网格实体/门/路径、Camera 和渲染滚动偏移。Camera 构造函数位于 VA `0x91C730`（RVA `0x51C730`），ABI 为 `__thiscall void(Camera*, Room*)`，对象大小 `0x94`，会读取 Game 的 enter-door、Room 门格与非零 grid width，并写回 Room 的滚动偏移。故独立构造 Room/Camera 还要求有效 Game、descriptor、门槽、格宽和完整 Room 内嵌对象；不能只设置 Camera 指针。

### Manager::Render 接入

- 原生入口 VA `0x92EC40`（RVA `0x52EC40`）。在 RVA `0x52EDDE` 的图形 prelude 后插入 Client dispatch：原始字节 `80 BF D4 A3 04 00 00`，7 字节，native continuation RVA `0x52EDE5`。
- Client 分支绘制自有 Replica Scene 后跳 VA `0x92F041`（RVA `0x52F041`）。该处会先重载 EAX 为全局 Manager，再进入 `0x92F046` 的 graphics tail；不要直接跳到 `0x92F046`，因为临时 EAX 不是 Manager。
- Off/Host 分支重放 `cmp byte ptr [edi+0x4A3D4],0`，并从 `0x92EDE5` 继续原生流程。Client 跳过 native loading/menu/Game/Room render、光标与尾部 Lua/UI 绘制；自有 HUD 需在 Replica pass 内提供。
- prelude 包含 resize/render-target/shader/FBO 准备（如 `0x92AD30`、`0x67EDD0`、`0x9E85B0`）；`0x92F041` 后沿 Manager 的条件尾部进入图像 apply/reset 与 present。自有 draw 只在原生渲染线程和有效 context 确认后执行。

### 官方 hook 链顺序

REPENTOGON HookSystem 的 `FuncHooks()` 是 `std::multimap<int,...,std::greater<int>>`，按 priority 降序安装。故同 RVA 的 `+30000` native witness 先安装并保存原生 trampoline，默认 priority `0` 的 REPENTOGON hook 后安装，`-30000` outer 最后安装。outer Client 分支直接调用 native witness 的 `internalSuper`，可越过官方外层回调并进入原生函数；Off/Host 仍经正常 `super()` 链。

`repentogon/LuaInterfaces/CustomCallbacks.cpp` 的 `HOOK_METHOD(Manager, Render)` 在 `super()` 前执行 `PRE_RENDER(1135)`。`repentogon/SaveSyncing/SaveSyncing.cpp` 的 `Manager::destructor` 在 `super()` 前执行 `PerformAutomaticVanillaSaveSynchronization(false)`。`Manager::Render` 与 `Manager::~Manager` 均为无显式栈参数的 `__thiscall` 方法，Manager 在 ECX。HookSystem trampoline 的 thiscall arg0 特判复制 ECX；直接调用最内层 trampoline 的签名为 `void(__thiscall *)(void*)`。

## 自有 ANM2 与 raw frame holder 退休

自有 C++ ANM2 对象可由 `ANM2::destructor` 控制释放（VA `0x403790`，RVA `0x3790`，`__thiscall void(ANM2*)`）；`Reset`（VA `0x403530`，RVA `0x3530`）不是对象销毁。避免 Lua Image 引用；普通 ANM2 layer/资源访问只在其 owner 和 generation 有效期间使用。

J273 图像队列是 raw pointer：普通帧队列 `BCE570..BCE574`、tracked image vector `BCE57C..BCE580`、透明队列 `{RenderBatch*, ImageBase*}` `BCE588..BCE58C`。`ImageBase::Render` 最终会登记待绘图 image；`apply_frame_images` VA `0x9EFA70`（RVA `0x5EFA70`）消费 ordinary/transparent 队列，随后 `frame reset` VA `0x9EFA10`（RVA `0x5EFA10`）清帧标记并将普通队列 end 归零。Manager::Render 返回本身只作为该帧已执行完整尾部的证据，不能用单独的 Present 返回替代队列判定。

可实施的退休守卫：停止该 Scene 新 admission/render；在主 render owner 上等当前 Manager::Render 返回，确认该 generation 不再出现在 ordinary 与 transparent pending 队列，且 apply/reset 已完成；若需等待 GPU 命令完成，在确认有效 GL context/thread 后再执行 `glFinish`；然后调用自有 ANM2 析构以降低其 ImageBase strong ref。主智能体独立回读纠正：`0x408500` 在 strong 1→0 时调用图像 vtable +0x60 析构；`0x9EFE60` 则读取递减后的 strong，仅等于1时进入 `0x9EF7C0` 移除 ordinary/tracked/transparent 与cache索引。两者不是同一分支，不能称为strong归零后的callback。`BCE57C..BCE580` 是非 owning bookkeeping，不能要求它在 DecrRef 前为空，否则会把其清理条件放在触发清理的析构之后。若新 Scene 或 cache 仍持有另一 strong ref，旧 ANM2 只释放自己的引用，不销毁共享 ImageBase。

Context 当前线程、GL context 归属以及能否在指定边界调用 `glFinish` 仍须运行时断言；静态分析不能证明实际 context 生命周期。

## Shutdown 与 Manager 析构的 PGD 隔离

`Isaac::Shutdown` VA `0x982C90` / RVA `0x582C90`，ABI `__cdecl void()`。不要提前 return 整个 Shutdown；两处分支仅绕过 Client 的原生进度持久化，后续正常资源清理仍继续。

| 落点（VA / RVA） | 覆盖字节与长度 | 普通模式续接 | Client 分支 | 影响 |
|---|---|---|---|---|
| `0x982D1B` / `0x582D1B` | `85 C0 74 48 83 B8 A8 5B 02 00 00`（11 bytes：`test eax,eax`; `jz 0x982D67`; `cmp [eax+0x25BA8],0`） | shim 重放 test/cmp 后到 `0x982D26`（`jbe`）；原始零指针分支仍到 `0x982D67` | `0x982D61` | 仅跳过后续条件成功时对 PGD counter[192] 清零、counter 522 增量与 SaveLocally；保留 `0x982D67` 的 Manager OptionsConfig 保存调用。 |
| `0x982D72` / `0x582D72` | `A1 1C 4A BE 00`（5 bytes：`mov eax,[0xBE4A1C]`） | 重放原指令后到 `0x982D77` | `0x982DDD` | 跳过 dirty 清标、backup、Steam Cloud/local PGD save 与 `Game::Exit(true)`；`0x982DDD` 后 Game/Manager/Lua/context teardown 仍执行。 |

`0x982D61` 和 `0x982DDD` 是代码内续接点，不是函数返回。第二块中的 `Game::Exit(bool)` VA `0x6F6270`，ABI `__stdcall void(bool)`。

Manager 析构 VA `0x92A7C0` / RVA `0x52A7C0`，ABI `__thiscall void(Manager*)`，开头 16 bytes：`55 8B EC 6A FF 68 B0 04 AA 00 64 A1 00 00 00 00`（MSVC SEH prologue）。原生析构包含普通容器/渲染/引用清理；静态反汇编/反编译未见直接 PGD field write 或 SaveLocally/Cloud call。Client 必须绕过 REPENTOGON dtor 外层自动存档同步，但仍调用原生析构 trampoline 一次。原生析构被调函数的全部传递副作用未穷尽，记录为 UNKNOWN。

## 本轮主审

主智能体独立读取HookSystem.cpp的greater<int>安装顺序、SaveSyncing.cpp析构prehook、92F041重载EAX、982D61/982DDD续接、9EFA10与9EFDEB队列清理，以及408500/9EFE60的实际指令。接受上述窄范围静态事实和实现入口，已纠正cache回调／归零析构混称；实际context、队列、像素与正常退出留统一E2E，不将静态回读记作运行通过。

## 同一tx续验的层边界主审

e2e-02的越界层反例后，同一个Luna/max继续只读核定，主审独立回读`LuaSprite.cpp:115`的前置层数检查、`AnimationFrame.zhl`的start/end字段、SDK `AdvanceLayerPosition`的XMM2桥以及`Anm2Extras.cpp:248`的实际draw访问。

- `GetLayer(int)` VA406B60按LayerState数组ordinal访问；越界记录错误后返回静态sentinel，而非nullptr。公开Lua wrapper先检查范围，C++桥不可省略。真实LayerData ID、Sprite ordinal、AnimationData track order分别解析；换图使用Sprite ordinal，层shader使用已持有的LayerState。
- `SetLayerFrame` VA404390先按ANM2层数检查ID，再查map；map错误只log仍写索引。`AdvanceLayerPosition` VA404030则直接按track order处理，越界也只log。因此实现不对外部ID盲调这两个入口；只在本方候选已核定的selected数组中按真实track order选择，不能写共享AnimationFrame缓存。
- `AnimationFrame`的start/end为半开区间，end是下一帧起点；由资源bake形成。时间姿态先核有限值与动画范围、轨道／帧表，再按end递增选key；落在未覆盖gap须明确提供key，而不是clamp伪造。显式key有界，null明确选-1；-1是Reset建立且native/RGON RenderFrame会跳过的合法“不绘制”状态。不存在的track与存在但隐藏的track分列。
- 原生／RGON RenderFrame最终仍按track ID索引Sprite layerState；候选在调用原生姿态／draw前验证这一实际布局，拒绝损坏或原生无法安全显示的资源。不能仅解析出ID就声称底层renderer可接受任意排列。
- root main／overlay保留私有分数phase，逐层时间也可带分数；这只保留姿态和跨key边界，不额外声称key内连续插值。整局复杂实体和特殊几何仍由18—20提供专用adapter。

## Mod持久入口

公开`Isaac.SaveModData/RemoveModData`与官方`_CBindings.ESSM.SaveData/DeleteData`均按Lua CFunction的cdecl ABI安装常驻正文detour，地址从当前已注册CFunction取得，所属模块固定；清单登记全部入口。正文拦截覆盖ModReference包装和预先保存的函数别名，不靠只替换全局Lua字段。ESSM底层路径／参数与写入依据仍来自ST8引用的官方源码；晚期全局缺失只作为lookup观察，不能据此认定ESSM不存在。Client／关闭保护窗口阻断original，Host/Off仍原调用一次；不从此推定支持任意第三方Mod私有持久协议。


### ESSM初始化隐藏的补充主审

Luna/max只读核定RunBundledScript VA854FD0/RVA454FD0，prefix `55 8B EC 53 56 57 68 2D 01 00 00`，ECX接收LuaEngine、栈上传const char*、ret4、bool返回；主审独立回读该段与SDK声明。LuaInit.cpp先注册_CBindings，再运行enums_ex/main_ex，返回后才nil全局；entity_save_state_manager.lua:60—63已捕获保存函数到local。因此在脚本入口安装正文detour，四个能力均必须真实捕获；不能把第三次晚期lookup miss当作API不存在。第四次统一续验实际覆盖隐藏函数及退出callback中的再次保存尝试。
