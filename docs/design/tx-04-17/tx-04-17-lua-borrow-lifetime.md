# tx-04-17 ST7：Lua、Sprite 与借用对象寿命

研究日期：2026-10-31，Asia/Singapore。状态：**ST7指定静态研究、与ST5／ST6来回互核及主智能体审阅完成，具体UNKNOWN保留；不是整个 tx-04-17 完成。** 本项由一个GPT-6 Luna／max子智能体推进；纠错与接受范围见[工作流程主审记录](tx-04-17-workflow.md)。

## 1 范围与证据口径

本项按 [tx-04-17-workflow.md](tx-04-17-workflow.md) 的 ST7 范围，区分自有 Sprite／ANM2、Lua userdata、Animation／Layer 借用、Lua registry root、Image 强引用、native renderer raw queue、GC／native destructor／外层内存释放和场景／Lua／renderer 关闭。接口参考 [tx-04-17-public-interface-catalog.md](tx-04-17-public-interface-catalog.md) 及 ST2／ST3／ST4 当前文档。公开资料和 REPENTOGON 1.1.3 源码优先；只对源码未解释的 J273 细节使用同一 IDB 的只读静态分析。03-15／17 J460 记录仅作问题索引，不移植地址、布局或运行时结论。

| 标记 | 含义 |
| --- | --- |
| PUBLIC_DOC | IsaacDocs／REPENTOGON Wiki 的公开接口语义，不证明 native owner、context 或实际安装。 |
| CURRENT_SOURCE | 本地 REPENTOGON 1.1.3 Lua 绑定或辅助源码行为，不证明游戏当前加载了这条 patch 链。 |
| J273_STATIC | 本轮在 J273 IDB 中只读回读的静态代码、bytes 与调用次序；不证明运行时命中。 |
| DESIGN | 建议给候选实例／资源／场景的管理合同。 |
| UNKNOWN | 现有静态材料没有闭合的 owner 或运行时事实。 |
| J460_INDEX | 03-15／17 J460 研究仅提供要核对的问题，不作为 J273 证据。 |

IDA server_health 确认当前输入仍为 ST2 所用的 Repentance+ v1.9.7.12.J273，IDB 为 isaac-ng-v1.9.7.12.exe.unpacked.exe.i64，imagebase 0x400000。地址以下均为 VA，括号可给出 RVA。仅作只读读取；未切换、写入、改名、注释或保存 IDB。未构建、启动游戏、运行测试或校验哈希。

## 2 结论摘要

1. Lua registry 能让被引用的 Sprite userdata 保持可达，从而延迟 Lua 对该 userdata 的 GC；它不会让 LayerState、AnimationData、AnimationLayer、AnimationFrame 或 BlendMode 的裸指针自动持有父对象，也不会给它们增加代次检测。即使 Sprite userdata 仍活着，调用 Reset、Load、Reload、换资源或退休场景仍可使内部借用失效。
2. Sprite Lua 对象在 J273 中以内嵌 ANM2 payload 的 userdata value 形式存在。Lua 的 Sprite __gc 调用 native destructor；Lua 管理 userdata 外层存储。ANM2::Reset、ANM2 native destructor、userdata GC 和 Lua userdata 外层分配回收是不同动作。
3. Lua Image 是例外：Image userdata 内有 KAGE_SmartPointer_ImageBase 值；从 LayerState／Sprite 取图时会复制此关联，LuaImage 的 __gc 明确 DecrRef。它只强持有 ImageBase，不保护来源 LayerState、ANM2/cache、renderer context，也不把 J273 native frame/batch 队列的 ImageBase* 裸指针转换为强引用。
4. 所有内部 borrowed userdata 必须按本地 Sprite instance/resource generation 管理。任何 Load、Reload、Reset、图像关联替换、ANM cache rebind、场景退休或 Lua state／renderer generation 更换后，都撤销本方缓存并在有效 owner 内重新取用；旧指针、旧 userdata、路径相同或地址相同都不是有效性证明。
5. 退出需要区分 Lua close 与更早的 Game/Manager/ANM cache owner teardown，以及之后的 renderer/ImageManager/context teardown。registry unref 不等于立即 GC；Lua close 的 finalizer 不能作为 Game/Manager/ANM cache 仍活或 renderer 最后引用已释放的证明。场景关闭先停止新回调／draw，并在主线程边界等待在途栈正常返回；在前一组 native owners 仍可用时清理自有 Sprite 资源，等 raw queue owner 有证后才放掉 Image 最后引用。

## 3 对象所有权图

| 对象／边界 | 实际存储与 owner | 借用、回收与使用限制 |
| --- | --- | --- |
| Lua Sprite / native ANM2 | J273 Sprite userdata 的 inline payload 是 UserdataValue<ANM2>；payload 位于 userdata block +8。Lua 持有 userdata block，inline ANM2 拥有自身 Load 后的字符串、animation/layer 状态和关联资源。 | registry 对 Sprite userdata 建强根可延迟 userdata GC，但不阻止显式 Reset／Load／Reload。Sprite Lua 值是独立对象，不得用 native 内存浅拷贝来做候选或跨线程传递。 |
| 原生 Entity／Room 内嵌 Sprite | CURRENT_SOURCE 多处以 UserdataPtr 暴露已有 ANM2 指针，例如 LuaEntityPlayer.cpp:1525,1531,3409，LuaEntityTear.cpp:26–38，LuaEntityBomb.cpp:144，LuaRoomTransition.cpp:17,73–75。底层 Sprite 仍由原生 Entity／Room／子系统对象持有。 | UserdataPtr wrapper 只存 native pointer；Lua registry 即使保住 wrapper，也不保活父 Entity／Room 或 Sprite 内嵌存储。对象销毁／重建时 wrapper 失效。它不是 Lua 自有 UserdataValue<ANM2>；本方只可退休自己创建的 Sprite，绝不对借来的 Entity／Room Sprite 调 Reset 或 destructor。 |
| Lua LayerState | LuaSprite.cpp:114–155 为每个结果分配 sizeof(LayerState*) 的 userdata 并写入 raw LayerState*；GetAllLayers 同样如此。LayerState 属于当前 Sprite instance 的层数组。 | 无父 Sprite 引用，无 epoch 检查，无析构 LayerState 的 __gc。Sprite 成功重绑或 Reset 后，原层数组可能已被析构／释放。即使 cache miss 时旧数组物理残留，也不能视为新 resource revision 的有效 LayerState。 |
| AnimationData／AnimationLayer／AnimationFrame | LuaAnimationData.cpp:5–146 与 LuaAnimationLayer.cpp:7–49 将 raw pointer 放进指针大小 userdata；AnimationFrame 由 Sprite 的当前 AnimationState／AnimationLayer 选出。 | wrapper 不拥有来源 ANM2、动画缓存或 layer；Load／Reload／Reset 等变更后不能继续解引用。LuaGetLayerFrameDataInternal 会 clamp 当前 selector 到有效 key 范围，返回记录不证明原 selector 有效。 |
| BlendMode | LayerState:GetBlendMode 经 LuaSprite.cpp:623–625 调 LuaBlendMode::NewPointer，保存 LayerState 内 BlendMode 的地址。BlendMode 属性在当前源码可写并有 enum 范围检查。 | 仍是借用指针，不延长 LayerState；必须跟随其 Sprite/resource generation 撤销。 |
| Lua Image | LuaRender.h:12–14 的 LuaImage 含一个 KAGE_SmartPointer_ImageBase 值。LuaSprite.cpp 的 Sprite／LayerState GetSpritesheet 路径复制该值；SmartPointer.zhl 的 copy 增加引用。 | LuaRender.cpp:228–230,360–372 注册的 Image __gc 显式调用 DecrRef；SmartPointer 的 last-count 路径进入 Free，但下游 ImageBase subtype destructor／GL cleanup 不可仅凭 Lua GC 推定。该强引用只覆盖 ImageBase 本身，不覆盖图像所来自的 LayerState／ANM2/cache／GPU context。 |
| Lua registry ref | luaL_ref 到 LUA_REGISTRYINDEX 的对象是一个 Lua 值；只在所属 lua_State 存活期间保根。 | 对 Sprite userdata 的 ref 可保活 inline ANM2；对 LayerState 等 userdata 的 ref 仅保活那个装裸指针的小 wrapper。luaL_unref 只撤掉该 root，不同步运行 GC，也不为 native pointee 增加 refcount。 |
| J273 frame／batch image queue | ST6 静态看到 ImageBase* raw 指针进入普通 _frameImages 及 batch 路径；Lua Image smart reference 是独立引用关系。 | 0x9EFA10 清普通队列 flags 并将队列 end 退回 begin；0x9EFE90 在 control strong==1 分支调用 0x9EF7C0，后者有已见的 ImageManager/cache 与 raw-container remove 操作；0x9EF844／0x9EF84F 在已见路径中 memmove/remove 一个 frameImages raw pointer 并调整 end。它们闭合具体 CPU queue/remove 子路径，不是最后 ImageBase destructor 或 GPU idle 的证明。apply_frame_images、Render 返回和 SwapBuffers 不能单独证明所有权释放。 |
| ImageBase smart-reference strong release | J273 sub_4081A0 是 KAGE_SmartPointer_ImageBase::DecrRef；0x4081D1 调 ReferenceCount<ImageBase> vtable+0x0C，目标 sub_408500 在 mutex 内减 strong count。 | strong 1→0 时 sub_408500 于 0x408536 调 Image vtable+0x60(arg=1)，清 control block image 于 0x408539，再于 0x408549 weak-decrement。当前静态正常释放路径到 Image subtype destructor 已闭合；它与可选的 0x9EFE60 queue/cache removal callback 分开。实际运行、GL context 和 GPU idle 不由静态链证明。 |
| REPENTOGON Anm2Extras | CURRENT_SOURCE Anm2Extras.cpp:77–92 以 ANM2* 为外层 key 保存 shader extras；其中 layer_extras 以 layer ID（int）为 key。Reset 与 destructor hook 在调用 super 前 erase(this)。 | 这是官方源码 patch 意图，不等于 J273 原始 destructor 已含此 map 清理或当前进程安装了该 hook。旧 ANM2* 地址重用不得承接旧 extras。ClearShader 只清对象的 shader association，不证明共享 shader program 或 GPU 资源已销毁。 |
| ImageBase subtype / texture | J273 中 PCX、PNG、RGBA、Procedural ImageBase subtype destructor 分别位于 VA 0xA340B0、0xA346F0、0xA35240、0xA35640；都调用 VA 0xA4C360。helper 在 0xA4C369／0xA4C375 调两次 glDeleteTextures，并清理／复位对象字段。 | 正常 strong 1→0 的静态路径到 subtype destructor／GL cleanup 已闭合；运行时是否实际到达、OS context 是否 current、API 是否成功及 GPU idle 仍未知。0x9EF7C0 cache/queue remove callback 是另一条路径。 |

## 4 Lua 借用包装及 J273 Sprite 回收

### 4.1 当前源码中的指针包装

- LuaSprite.cpp:114–155 的 GetLayer／GetAllLayers 返回 LayerState** 指针大小 userdata；没有把 Sprite userdata 放入 wrapper 字段或 registry，没有 epoch／generation 核验。
- LuaSprite.cpp:197–218 由当前 AnimationState、AnimationData、AnimationLayer 和选中的 key 生成 AnimationFrame** userdata。selected key 被 std::clamp；若帧数为零、selector 被损坏或上一代对象被释放，静态源码不能推出安全行为。
- LuaAnimationData.cpp:5–146 对当前／overlay／命名 AnimationData、其 AnimationLayer 使用相同 raw pointer userdata 方式。LuaAnimationLayer.cpp:7–49 的 GetFrame 返回 borrowed AnimationFrame pointer。
- LuaSprite.cpp:623–625 的 GetBlendMode 虽有可写字段，仍由 NewPointer 返回 LayerState 内部地址。新写字段不会反向保活 LayerState。
- LuaCore.h:464–548 记载 UserdataPtr 的虚析构不处理 pointee；LuaCore.cpp:92–111 只在注册类时显式传入 gc callback 才设置 __gc。LayerState／AnimationData／AnimationLayer 等借用 wrapper 的注册未建立 owner 释放关系。
- 以上是 CURRENT_SOURCE 的 userdata 表现。文档接口可用性与上述 native 指针的实际寿命应分开记录；公开 API 没有使 Lua 变量跨 ANM2 重绑继续安全的保证。

### 4.2 Sprite payload、native destructor 与外层内存

J273 静态回读得到：

| 动作 | J273 证据 | 结论 |
| --- | --- | --- |
| Sprite __gc callback | VA 0x8A9480 从 userdata 取出首字段 vtable，并以 flag=0 调用 slot 0。Sprite class 注册引用该 callback。 | 此时由 wrapper thunk 处理 inline ANM2 payload；这不是 Lua registry unref，也不是直接释放外层 userdata block。 |
| UserdataValue<ANM2> thunk | userdata vtable 0xB25964 的 slot 0 指向 VA 0x8AAF70。该函数对 Block+8 调 ANM2 destructor VA 0x403790，恢复 Userdata vtable；仅 flag bit 0=1 时调用外层 block deallocator。 | Lua __gc 的 flag=0 执行 native ANM2 destructor，不执行该 thunk 中的显式外层 deallocate。Lua 自己回收 userdata 内存。 |
| ANM2::Reset | VA 0x403530（RVA 0x3530）：存在 filename 时调 cache ref removal VA 0x409BB0；清 filename、动画／layer 与多个内部状态；对 per-instance LayerState buffer 调 vector destructor 并 free；不释放 ANM2 外层对象。 | Reset 是活对象的资源／内部状态清理，不是 native destructor，也不代表 GL context 或 raw render queue 已完成释放。 |
| ANM2 destructor | VA 0x403790（RVA 0x3790）先调 Reset，再释放内部 buffer 和字符串；伪码中没有释放 ANM2 外层 userdata block。 | 由 Lua finalizer 所有，不应由 adapter 手动再调一次 native destructor。手动 destructor 后还留 userdata 会使后续 Lua GC 重复析构。 |

因此候选 owner 需要保存自己的 resetDone／resource generation 状态：只有在自有回调和 draw 已退尽、且 ANM/cache 与 renderer owner 有效时执行一次显式 Reset；随后丢弃全部 Layer／Animation 借用。该状态只防止本方重复执行资源清理；不得手动调用 native destructor 或直接调用 Sprite __gc。若随后执行正常 Lua finalizer，J273 静态 destructor 路径会再次调用 Reset；静态 Reset 会清 filename、layer buffer 指针和相关状态，但不据此保证所有异常／关闭路径下二次 Reset 都安全，也不把 finalizer 的执行当作资源退休时序保证。Lua 仍拥有 wrapper block；native buffer、userdata 外层存储和图形队列仍是不同释放动作。

### 4.3 Lua Image 强引用与最后释放

当前源码 LuaRender.cpp:228–230 给 Image 设置显式 __gc：从 LuaImage 取出值成员并 DecrRef。Sprite／LayerState GetSpritesheet 将底层 smart pointer 复制到新 LuaImage；SetSpritesheet 则把 LuaImage 的引用赋给 layer。SmartPointer.zhl 表示拷贝增加 reference count；J273 的正常强释放路径由 sub_4081A0 DecrRef 进入 sub_408500 DecrementReference，strong count 从 1 到 0 时在 0x408536 调 image vtable+0x60，然后清 control block image 并弱减。PNG 的 vtable 0xB3BA80+0x60 指向 sub_A346F0，PCX 的 0xB3B8EC+0x60 指向 sub_A340B0；RGBA sub_A35240 与 Procedural sub_A35640 也调用 sub_A4C360。

sub_A4C360 在 0xA4C369 和 0xA4C375 调两次 glDeleteTextures，之后清理并复位 Image 子类资源字段。此处闭合的是静态正常 strong 1→0 子类析构／GL cleanup 路径；运行时是否实际走到它、OS context 是否 current、API 是否成功及 GPU idle 仍未知。它不等于每一次 LuaImage __gc 都会立刻释放最终 ImageBase。

另一条路径是 optional callback psub_9EFE60：sub_4081A0 仅在 counter decrement 返回 true 且该 callback 非空时调用；callback 在 mutex 内检查 control strong==1，满足时进入 sub_9EF7C0 做 ImageManager/cache/CPU queue remove。sub_9EF7C0 反编译有 bad-SP 提示；它不是 subtype destructor，也不能代替 0x408500 的 1→0 strong-release 路径。

设计上必须分别记录：

1. LayerState 与 Animation borrowed pointer 是否属于仍存活的 Sprite/resource generation；
2. Sprite layer、自有 Image userdata 和其他已知 owner 还持有多少 Image association；
3. renderer queue / batch 是否还保存 raw ImageBase*；
4. 最后一份 Image ref 的 DecrRef／Free 是否发生在有效 renderer/cache/context owner 窗口。

registry unref 不是第 4 项的完成信号。LuaImage 的普通 GC 可能晚于 unref；J273 lua_close 位于 Game／Manager 和 ANM cache、部分 shader/resource 清理之后，但完整 ImageManager/renderer/context teardown 在其后的 main cleanup 路径。不得把 Image userdata 的 C++ 值成员析构自动化当依据；源码已经显式调用 DecrRef，但何时发生仍取决于 GC。当前公开 wrapper 没有提供显式清空 SmartPointer 的 LuaImage 方法，这是确定最后 Image 引用退休时的具体实现缺口。实现若要确定最后释放，应在有效 owner 窗口提供可控引用退休机制，或实际证明受控 Lua collection 与所有相关 finalizer 的行为；不得直接调用 lua_Image_gc 或手动析构 userdata。

## 5 资源操作如何撤销借用

| 操作 | J273 静态观察 | ST7 generation 规则 |
| --- | --- | --- |
| Load | VA 0x407AF0（RVA 0x7AF0）先对旧 filename 执行 cache remove VA 0x409BB0，再替换 filename；调用 bind VA 0x409680、addref VA 0x409A40，按参数可进 LoadGraphics。没有旧资源事务性回滚结果。 | 调用前撤销该 Sprite 的 LayerState／AnimationData／AnimationLayer／AnimationFrame／BlendMode cache；完成后基于实际候选重新取。失败或 cache miss 也不恢复旧 generation。 |
| Bind 成功／cache miss | bind VA 0x409680 成功时，在 VA 0x409895 附近销毁旧 per-instance 0xA0 层数组并新建；cache miss 在 VA 0x409A00 仅写 warning 并返回，未清掉旧 layer array。 | 成功路径物理释放旧层对象；miss 时指针可能只是在内存上仍存在，但 filename、cache ref 和目标资源 revision 已变化。两者都不能拿旧 userdata 当成功资源。 |
| Reload | VA 0x407C40（RVA 0x7C40）在已有 filename 时重新 bind/addref 并执行 LoadGraphics；未先像 Load 一样减去旧引用；成功 bind 会重建 layer array。 | 每次调用都开始新的资源 generation。不能当刷新 getter、回滚或保旧借用有效的证明。 |
| ReplaceSpritesheet | VA 0x407CD0（RVA 0x7CD0）对指定层检查索引与路径；相同路径返回 false，不同路径更新 path 并清该层 image-loaded 标记；自身不重建整个 layer array。 | 路径或 image association 变化就撤销相关 Image／Layer 借用的语义 generation；Reload／LoadGraphics 后重取。不要在已发布 Sprite 的在途 draw 中原地替换。 |
| LayerState::SetSpritesheet | 当前 Lua bridge 赋 Image smart reference；不等同 ANM2 path Replace；不会使 Image wrapper 持有 LayerState。 | 作为候选局部 resource revision 管理；setter 完成不说明 GPU、raw queue、所有层或像素正确。 |
| Reset | PUBLIC_DOC IsaacDocs Sprite.md:348–350 提供 Reset；J273 VA 0x403530 清除 filename/cache ref 和 per-instance layer/animation 状态，但不释放外层 Sprite userdata。 | 显式 Reset 前撤销所有 borrowed handles；Reset 后所有关联层、动画和 BlendMode 借用无效。只对自有 Sprite 做资源退休；绝不对 UserdataPtr 借来的 Entity／Room Sprite 调 Reset。 |
| Scene retire / Lua close / renderer shutdown | scene retire 的旧代次引用需按 ST2/ST6 自有 owner 顺序停止；Lua close 与 graphics teardown 的 J273 次序见第 7 节。 | scene、Lua state、renderer/context 各自有独立 generation；任一变化后都不可重取旧对象。Lua registry root 只在当前 Lua state 内有意义。 |

所有指针借用以“owner 仍活着 + generation 匹配 + 正确主线程/资源 owner”作为设计有效条件；代码层实际 userdata 没有该校验，需由持有表/候选状态执行。每个调用点重新获取借用，或由代次化句柄在解引用前重新获取；不把原始 native pointer 或 Lua userdata 暴露给网络worker、跨进程协议或其他线程。

## 6 回调在途、资源替换与场景退休次序

### 6.1 活跃调用中的借用

一个 Lua callback 正在读取／修改 Sprite 时，不能同时 Reset、Reload、替换资源、unref 该回调依赖的 owner 或关闭 lua_State。一个本地 callback/draw 使用段至少持有本地场景 epoch 和自有 Sprite/Image owner，并在进入点登记 active-reader；退出时释放登记。场景退休先关闭新 callback／draw admission、撤销新姿态发布，再在同一主线程的安全边界等待已登记使用段返回。这个边界必须调度在途栈先正常返回之后；不能在当前 callback 栈内自旋或阻塞等待自己的 reader 归零。owner thread 不得在仍执行该 Lua 栈／C binding 时销毁其 state 或内层对象。

ST4 已表明 Entity/animation event callback 不因此变成无副作用；此处只讨论本方 callback 的内存寿命，不认可原生玩法事件可透传。yield/coroutine、longjmp/异常、重入、非预期线程以及由其他 Mod 保留 LayerState wrapper 的清理行为没有静态闭环。遇到这些路径时不得推断普通 callback 计数已排空。

### 6.2 候选资源与旧代次

1. 网络worker只收发有界纯值，不持有 Sprite、Lua userdata、LayerState、ImageBase*、Player* 或 GL handle。
2. 由 ST6 核定的 Lua/game main owner 阶段创建全新 Sprite 候选；完成 Load、必要换图、LoadGraphics、animation 选择和姿态应用前，不发布到 draw consumer。
3. 发布仅切换自有实例/generation；停止旧代次的新 draw，但仍保留可能被 callback、Image 或 native raw queue 使用的旧引用。
4. 等自有 callback/draw reader 清零后，按有效 owner 清理借用；等 ST6 已定位的普通队列 reset/remove owner 和 batch owner 证明相关 raw ImageBase* 不再待用后，才放弃旧 generation 最后一份强 Image 引用。0x9EFA10 是已找到的 CPU frame queue reset helper；0x9EFE90→0x9EF7C0 和 0x9EF844／0x9EF84F 是已见的 ImageManager/cache 与 CPU queue remove 子路径，不是最终 ImageBase destructor 证明。它们不证明其他缓存已释放同一 Image，也不等于 GPU idle。
5. 仅在 ANM cache／Game/Manager 和 renderer/cache/context 尚活、且上下文检查通过时，对本方自有 UserdataValue<ANM2> Sprite 执行 Reset 并退出该代次；不得对由 Entity／Room 通过 UserdataPtr 借出的 Sprite 做 Reset 或 destructor。随后按受控时机撤销 registry roots。执行 Reset 后任何后续 Lua 代码不得再通过旧 Animation／Layer wrapper 访问。
6. Lua 关闭前确认所有本方 callbacks 已取消、在途调用归零、registry refs 已撤销；本方 Sprite 已在更早的 Game/Manager/ANM-cache teardown 前显式 Reset。Image userdata 没有当前公开的显式 release API：unref 不执行 GC，wrapper 仍可能持有 SmartPointer 到正常 GC 或 lua_close finalizer。实现必须在有效 ImageManager/context 窗口建立可控的引用退休机制，或明确保活相应 owner 直到 finalizer 完成；不得直接调用 lua_Image_gc 或手动析构 userdata。
7. renderer/cache/context 关闭前确认 batch/raw queue owner 和最后 ImageBase ref 已完成各自退休；Lua state 关闭前的步骤不能代替这项确认，反之亦然。

若 raw queue owner 尚未确定或旧代次不能在预算内安全保留，则停止接收新的资源 revision 并保持 Client 隔离；不能为腾内存而猜测释放可能仍被队列使用的对象。不得以移出 Lua registry、Reset 返回、apply_frame_images 返回、Manager::Render 返回、SwapBuffers 或进程正在退出来推断所有 owner 都已释放。

### 6.3 shader extras 的 owner

REPENTOGON 当前 Anm2Extras.cpp:77–92 用 ANM2* 为 key 保留 default/layer shader extras；源码 hook 在 Reset/destructor 入口先 erase(this)，再调用 super；内层 layer_extras 按 LayerState::GetLayerID() 对应的 int 查找。它表明官方设计把此副表寿命绑定到 ANM2 resource generation。候选切换后即使分配器复用相同 this 地址，也不能拿旧 generation 的 shader extras 作为新实例配置。

这是 CURRENT_SOURCE 的 patch 设计，不能替代本 J273 目标内的裸析构证据或实际安装路径证明。Reset 清掉 Anm2Extras map 不代表共享 shader object、全局 shader program 或 GPU 对象已经销毁；renderer/context 的实际最后 owner 由 ST6/后续整体实现继续确定。不要直接清全局 shader/cache 来强行满足候选退休。

## 7 退出顺序与相邻关闭界限

### 7.1 原始 J273 静态链

只读反汇编确认 Isaac::Shutdown VA 0x982C90 中原始 patch site VA 0x982DDD 为 bytes B9 78 6F BE 00。原程序从 0x982DE2 继续，先对静态菜单 Sprite 调 Reset VA 0x403530，后续 Game destructor 位于 0x982E1C，Manager destructor 位于 0x982E45；0x982E67 起包含 ANM cache 清理及后续部分 shader/resource 清理，并非完整 ImageManager/renderer/context teardown。Isaac::Shutdown 函数内部在 VA 0x98300B 左右调用 LuaEngine destroy VA 0x854F00；该函数在 0x854F5C／0x854F85 处 luaL_unref 两个 internal registry ref，0x854FA0 调 lua_close(this[6])，0x854FA9 清空 state。

main VA 0x90AC60 在 0x90AFCE 调用 Shutdown，返回后 0x90AFD3 继续 platform cleanup，随后包含 VA 0xA2AD80 的后续 ImageManager/renderer/context 清理路径。因此静态链显示 Game/Manager/ANM-cache 与部分资源清理早于 lua_close，而完整 ImageManager/renderer/context teardown 在其后。不能把这些不同资源种类写成一个“graphics 全部先于 Lua close”的步骤，也不能依赖 lua_close finalizer 处理已销毁的 Game/Manager/ANM cache owner。

### 7.2 官方 REPENTOGON 源码意图与安装未知

CURRENT_SOURCE REPENTOGON 的 ASMCallbacks.cpp:1876–1918 说明官方为了避免 LuaEngine teardown 后才回调，在 Isaac::Shutdown 的保存完成后、Game/Manager 被销毁前提早运行 MC_PRE_MOD_UNLOAD（callback id 73），并传入 shuttingDown=true；同时抑制 shutdown 中原本晚期的默认 MC_PRE_UNLOAD callback。源码 SigScan 为此定位原始序列，patch 在签名处恢复被覆盖的 5 bytes 后继续原程序。

这个源码注释表明早发 unload 的设计意图；它不是存档否决，也不等于 lua_close。原始 J273 bytes、REPENTOGON源码 patch 和机器上实际装载／命中该 patch 是三种不同证据。当前静态材料未核实运行安装链、patch sigscan 是否唯一命中、此 callback 当时的 HDC/HGLRC 是否有效；不能把 MC_PRE_MOD_UNLOAD 名字本身当作安全 graphics context 或最终 image release 点。

### 7.3 三类相邻关闭界限

| 界限 | 必须在其前完成 | 静态依据／限制 |
| --- | --- | --- |
| Game／Manager／ANM-cache teardown | 关闭自有 callback admission、排空在途 Lua 使用，撤销借用；仅对本方 Sprite 在 Game／Manager／ANM cache 尚活时显式 Reset，绝不 Reset 借来的 Entity／Room Sprite。ANM/cache 与 partial shader 相关引用须在各自 teardown 前撤销。 | J273 0x982E1C／0x982E45 销毁 Game／Manager，0x982E67 起有 ANM cache 和部分 shader/resource cleanup；这些都早于 0x854FA0 lua_close。 |
| Lua state close | 完成本方 callbacks 退场、borrowed handles 作废与 registry refs unref；不要让 lua_close 里的迟发 finalizer承担依赖已关闭 Game/Manager/ANM cache 的资源操作。LuaImage 没有当前公开的显式 release API，须有受控 finalizer/collection 窗口或明确保活策略。 | lua_close 位于 Shutdown 内 0x854FA0，但 Lua registry unref 不立即 GC；Lua userdata wrapper／payload finalizer 仍可能在此处运行。 |
| ImageManager／renderer/context teardown | 停止新 draw，等普通 frameImages 和 batch raw ImageBase* 的 owner 移除旧指针；最后 ImageBase ref/GL cleanup 发生在仍有效的 renderer/cache/context 阶段。 | main 在 Shutdown 返回后 0x90AFD3 继续 platform cleanup，后续有 0xA2AD80 的 ImageManager/renderer/context 清理。0x9EFA10、0x9EF7C0 是已见 CPU queue/cache 子路径；0x9E4170 是 renderer Present candidate local path，顺序为 apply 0x9EFA70、reset 0x9EFA10、再 renderer/vcall 与条件 SwapBuffers 0x9E41C8。它与 Manager::Render 0x92EC40 分开；CPU reset 不代表 GPU 完成。 |

实际退休点需按上述三类相邻 owner 截止条件完成：最先是 Game/Manager/ANM-cache 仍有效期间，随后是 Lua state close，最后是 ImageManager/renderer/context teardown。只满足 Lua close 或 renderer shutdown 单一条件都不够。普通 MC_PRE_MOD_UNLOAD 只有在当前安装链确认、主线程/state/context 检查通过且在途调用已退尽时，才可作为候选资源退休入口；源码意图不保证这些条件自动成立。

## 8 本地 generation 与重新获取规则

本方资源管理至少按 session/scene epoch、Lua state generation、Sprite instance generation、resource revision、renderer/context generation 分离身份。这些是本地保护字段，不发送 native pointer/userdata/Player 指针。

- 从当前自有 Sprite 在实际使用点重新取 GetLayer、当前／overlay AnimationData、AnimationData:GetLayer、AnimationLayer:GetFrame、GetLayerFrameData 和 GetBlendMode。不要将上次得到的 userdata 当跨资源修订缓存。
- 每次 Load、Reload、ReplaceSpritesheet、SetSpritesheet、LoadGraphics、Reset、成功 cache rebind 或 Sprite 退场都会递增本地资源 revision，并清除本方所有内部借用 cache。即使某次失败后物理地址恰好仍可读，也不跨 revision 复用。
- Animation 时间 selector 被 LuaGetLayerFrameDataInternal clamp 后的结果不能替代调用前对 layer ID、动画存在性及 key/时间范围的检查。
- Image wrapper 是独立强 Image association；只可跨其自身最后引用寿命保留 ImageBase。它既不能让来源 LayerState 有效，也不能证明 raw renderer queue 已排空。
- lua_close、scene retire 或 renderer/context 重建分别使各自 generation 失效；不得在关闭后重新取旧 userdata/指针补救。
- 避免 shallow-copy Sprite/ANM2 内存；候选使用新建自有实例与显式资源描述。即便当前源码有 Sprite Copy，也不把它当作 native resource revision、userdata 父子关系或 GL owner 的通用复制合同。

## 9 证据、UNKNOWN 与互核记录

### 9.1 来源索引

- 项目范围／PUBLIC_DOC：docs/refactor/project-conventions.md；docs/design/tx-04-17/tx-04-17-workflow.md；tx-04-17-public-interface-catalog.md；IsaacDocs/docs/Sprite.md:348–350；ST2／ST3／ST4对应文档。
- CURRENT_SOURCE：REPENTOGON LuaSprite.cpp:114–155,197–218,477–490,623–625,726–751；LuaAnimationData.cpp:5–146；LuaAnimationLayer.cpp:7–49；LuaRender.h:12–14 与 LuaRender.cpp:228–230,2856–2878；LuaBlendMode.h:26–34；LuaCore.h:464–548 与 LuaCore.cpp:92–111；LuaEntityPlayer.cpp:1525,1531,3409；LuaEntityTear.cpp:26–38；LuaEntityBomb.cpp:144；LuaRoomTransition.cpp:17,73–75；SmartPointer.zhl；Anm2Extras.cpp:77–92；ASMCallbacks.cpp:1876–1918。
- J273_STATIC：ANM2::Reset 0x403530、destructor 0x403790、Load 0x407AF0、Reload 0x407C40、ReplaceSpritesheet 0x407CD0、cache bind 0x409680、remove/addref 0x409BB0/0x409A40、Sprite GC callback 0x8A9480、UserdataValue thunk 0x8AAF70、vtable 0xB25964；Shutdown 0x982C90、原始 patch bytes 0x982DDD、Game/Manager dtor 0x982E1C/0x982E45、LuaEngine destroy 0x854F00；main 0x90AC60 调 Shutdown 于 0x90AFCE、platform cleanup 从 0x90AFD3 继续并到后续 0xA2AD80。RVA = VA − 0x400000。
- REUSED_J273：ST6 对 ImageBase::Render 0x9E2170、raw queue 写入 0x9E2C8C、batch 获取 0x9E2EC0、apply_frame_images 0x9EFA70、普通队列 reset 0x9EFA10、control-strong==1 回调路径 0x9EFE90→0x9EF7C0、frameImages／其他 raw container 与 transparent-pair remove 0x9EF844／0x9EF84F、renderer Present candidate local path 0x9E4170、draw 0x9EFD23 的互核。0x9EF7C0 反编译有 bad-SP 提示，故只记已见子路径；它不是 subtype destructor/GL delete 的证明。SmartPointer strong-release 0x4081A0→0x408500→vslot+0x60 在正常静态路径闭合；Image subtype dtor 0xA340B0／0xA346F0／0xA35240／0xA35640 到 0xA4C360，后者在 0xA4C369／0xA4C375 调 glDeleteTextures。0x9E4170 不是 Manager::Render 0x92EC40；实机触发、context、GPU idle 和全部 raw queue/cache 退尽仍 UNKNOWN。ST2／ST4 提供场景退场、绘制回调与副作用边界。

### 9.2 UNKNOWN

1. J273 普通 _frameImages 清理 helper VA 0x9EFA10、已见 raw pointer remove 0x9EF844／0x9EF84F 在所有正常 render、loading bypass、异常/退出路径的调用覆盖；0x9EFA10 的 caller 清单含 Manager::Init 0x92BA20、Manager::Render 0x92EC40 的多个调用点、sub_9E4140／sub_9E4170／sub_9E5340，但不能推成所有动态／旁路／异常路径。
2. 0x9EF7C0 的完整 owner 与 ImageManager/cache作用仍受 bad-SP 反编译限制；控制 strong==1 时见到的 0x9EFE90 callback 不证明最终 ImageBase destructor；同一 Image 的全部其他 batch/cache raw 引用移除时序也未闭合。
3. 正常 Image smart-reference strong 1→0 到 subtype destructor／0xA4C360 GL delete 的静态路径已闭合；实机是否触发、OS current/context/API 成功与 GPU idle 未验证。与之分开的 0x9EFE60→0x9EF7C0 cache/queue removal、全部 raw batch/cache 退尽时序和 renderer/cache/window context 的运行时 owner 仍 UNKNOWN。
4. 实际安装 REPENTOGON/Hook 链以及早期 MC_PRE_MOD_UNLOAD 在当前运行路径是否命中；实际主线程与 OS current context/HDC/HGLRC 和 generation。
5. Lua GC 的实际时点、受控 collection 时所有其他 finalizer 的副作用、同一 state 内其他 Mod 是否保留借用 userdata、yield/longjmp/错误/重入下 active callback 的完整退休方式。
6. 显式 Reset 以后正常 Lua finalizer 再执行 native destructor/Reset 之外的异常路径行为，以及 current REPENTOGON Anm2Extras hook 对目标二进制的实际覆盖。静态代码只能支撑正常结构推导，不作运行安全承诺。
7. 外部 Lua Mod 若保留旧 LayerState／AnimationData wrapper，当前接口没有办法遍历并清除其 Lua local/upvalue；本合同仅保证本方不再使用旧 wrapper。

### 9.3 互核记录

- ST5 首版 §3／5／7 将 LayerState、AnimationData、AnimationLayer、AnimationFrame 定为不保活父对象的 borrowed userdata，并把 Lua Image smart ref 与 renderer raw queue 分开。ST7 互读后确认 cache miss 可留下物理旧 layer array，但这不是逻辑上旧资源仍有效；Load 失败没有恢复旧画面保证。
- ST5 与主审回读 LuaBlendMode::NewPointer，确认 BlendMode 可写属性仍是 LayerState 内部借用；已写入对象矩阵。ST5 将 Anm2Extras keyed map、Reset/destructor erase-before-super、ClearShader 与全局 GPU 销毁差异补入其文档。
- ST6 首版 §4／5 确认 ImageBase* raw queue 不被 Lua Image strong ref 自动覆盖；互读补充 0x9EFA10 普通 queue reset、0x9EFE90→0x9EF7C0 ImageManager/cache callback 子路径、0x9EF844／0x9EF84F raw pointer remove，以及 0x9E4170 renderer Present candidate local path的 apply→reset→renderer/vcall→SwapBuffers 次序，并将 CPU reset 与 GPU idle 分开。ST5 互读指出 0x9E4170 应标为 renderer Present candidate local path，不能与 Manager::Render 0x92EC40 混淆。ST6 又闭合了正常 strong 1→0→Image subtype destructor→glDeleteTextures 静态链；实机触发/context/GPU idle及完整raw owner覆盖仍UNKNOWN。
- ST5 互读要求将“显式 Reset 后 finalizer”的结论限制于正常 J273 destructor 路径，不外推到所有异常／关闭路径；ST7 已按此收窄。
- 主审明确 active-reader 等待只能安排在关闭 admission 后、当前在途栈正常返回的主线程边界；不能在 callback 栈内自旋等待自己归零。LuaImage 当前没有公开显式 release API，普通 unref 不保证 GC 时点，不能手动调用 __gc；已列为实现时需处理的具体缺口。
- 主审指出 native Entity/Room embedded Sprite 由 UserdataPtr 借用；registry 只保 wrapper，不保活原生 owner。已加入对象表，并将显式 Reset 限定为本方自有 Lua Sprite。
- 主审更正 shutdown 归属：0x98300B 是 Isaac::Shutdown 内调用 LuaEngine destroy 的位置，不是 main caller；main 0x90AC60 在 0x90AFCE 调 Shutdown、0x90AFD3 继续 platform cleanup。0x982E67 为 ANM cache及部分 shader/resource cleanup，完整 ImageManager/renderer/context 路径在 Shutdown 返回后的 main cleanup 中。已修订为按资源种类区分两个截止点。
- 对 0x9EFE90→0x9EF7C0 的表述已收窄为 control strong==1 条件下的 ImageManager/cache/raw-container remove 子路径，不称为最终 last-ref/free；另列 Image subtype destructor 到 0xA4C360/glDeleteTextures 的静态证据，并保留二者完整时序 UNKNOWN。
- 主审独立回读 0x403530／0x403790、0x8A9480／0x8AAF70 与 vtable 0xB25964，确认 Lua GC flag=0 调 payload destructor，不调用外层 block delete；Reset 不释放 ANM2 外层对象。也确认 lua_close 在 Game/Manager teardown 后，以及原始 Shutdown 0x982DDD bytes。ST7 将此与 official early callback 的 patch 意图分列。

本报告仅为 ST7 静态研究与文档编辑。借用有效性、析构/GC 次序和 queue owner 的运行时行为须由完整实现后的统一整体验收观察；不把本报告记为功能 PASS，也不把它当成 tx-04-17 完成。
