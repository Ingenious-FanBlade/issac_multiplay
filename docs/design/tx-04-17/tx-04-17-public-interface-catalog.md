# tx-04-17 ST1：公开接口选型、既有事实与具体缺口

> 后续进度（2026-10-31）：[ST2场景根](tx-04-17-client-scene-lifecycle.md)、[ST3更新隔离](tx-04-17-update-isolation.md)、[ST4展示边界](tx-04-17-render-presentation-boundary.md)、[ST5资源与完整姿态](tx-04-17-resource-pose-runtime.md)、[ST6线程／context／在途绘制](tx-04-17-thread-context-lifetime.md)、[ST7 Lua与借用寿命](tx-04-17-lua-borrow-lifetime.md)已完成指定静态研究与主审，ST2／ST3及ST5—ST7另经来回互核；[工作流程](tx-04-17-workflow.md)记录纠错与接受范围。最新结果补正BlendMode可写属性、shader描述、J273 raw队列／最后引用、Lua GC与分段关闭合同；不是原生render整树安全或资源运行时完成。下文“ST2—ST8未执行／本轮未调用IDA”及缺口表保留ST1资料归并当时事实，不代表后续进度。ST8、世界壳、可控LuaImage释放、运行隔离及统一E2E仍未完成。

日期：2026-10-31，Asia/Singapore。状态：**ST1 资料归并与源码审阅完成；ST2—ST8 未执行，Client 隔离与资源运行时尚未实现。** 本目录是[工作流程 ST1](tx-04-17-workflow.md#st1-公开接口和既有事实归并)的设计输入，归同一 tx-04-17，不另设研究 tx 或分项验收。

本次只读工作流程第 3 节指定的 03-10—20、04-05、04-08 ST2／ST6—ST8、两套本地 Wiki、当前 Hook 清单及相关官方／工程源码。没有新增二进制结论，未调用 IDA 或启动逆向；因此未触发用户“如需逆向，指定 GPT-6 Luna / max 子智能体、主智能体审阅”的条件。后续实际补逆向时仍按该要求执行。本次没有改运行代码、绑定或游戏文件，没有构建、生成、校验脚本、测试、实机或哈希操作。

## 1 证据类型与版本范围

| 标记 | 含义与适用范围 |
| --- | --- |
| `PUBLIC_DOC` | 本地 IsaacDocs（区分 Repentance+ 返回差异）和 REPENTOGON 1.1.3 文档的公开 Lua 语义；不据文档推定 native ABI 或实际安装能力 |
| `CURRENT_SOURCE` | 本地 REPENTOGON 1.1.3 或当前工作树源码直接可读行为；不代表正在运行的 DLL／脚本就是该内容 |
| `REUSED_J273_STATIC` | 引用已有主审的 v1.9.7.12.J273 静态事实；本轮未重新取证，不扩大原研究覆盖范围 |
| `J460_INDEX` | 03-10—20 的 v1.9.7.17.J460 历史结论，只作问题／路径索引；地址、偏移、大小、虚槽、native ABI 均不能直接用于 J273 |
| `DESIGN`／`UNKNOWN` | 拟采用方式／尚无足够依据。公开接口存在、静态可达、源码正常返回与运行时实际成功分别记录 |

公开接口“可直接满足”仅指声明的参数和操作语义，无须再逐成员做二进制对照；不豁免主线程、owner、生命周期与失败处理。目标环境始终为官方 REPENTOGON 1.1.3／J273。工作树已有其他 tx 的未提交修改，本次只维护本目录及其入口，不将其他任务的进行中代码当作本次交付。

公开资料：[原版Sprite](F:/projects_isaac/IsaacDocs/docs/Sprite.md)、[原版回调](F:/projects_isaac/IsaacDocs/docs/enums/ModCallbacks.md)、[ModReference](F:/projects_isaac/IsaacDocs/docs/ModReference.md)、[RGON Sprite](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)、[RGON回调](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/enums/ModCallbacks.md)、[LayerState](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)、[AnimationData](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationData.md)、[AnimationLayer](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationLayer.md)、[AnimationFrame](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationFrame.md)。

## 2 接口选型：参数、返回、阶段、owner 与失败

下表 `原版+` 为本地原版 Wiki 中 Repentance+ 适用语义，`RGON` 为本地 1.1.3 扩展。Lua 的 `void` 表示无业务结果，不能读取 native 残余寄存器当成功码。阶段和采用方式中超出文档明示部分均为 `DESIGN`，由 ST2—ST8继续落实。

### 2.1 更新、碰撞、render 与退出

| 必需操作／分类 | 版本、公开参数与返回 | 阶段／对象 owner | 已确认限制与采用方式 |
| --- | --- | --- | --- |
| 普通更新暂停：需源码，已有 J273 部分事实 | RGON `MC_PRE_UPDATE(1026)()` → boolean，true 取消主更新 | Game 普通主更新段；引擎／Lua dispatcher owner | `PUBLIC_DOC` 可暂停主更新；`CURRENT_SOURCE` 跳过后仍提交移除、排序和 HUD 后处理。`REUSED_J273_STATIC` 该段之前还有原生工作；不能单独当完整 Client 隔离，转 ST3 |
| 派生更新取消：需源码 | RGON 1160—1168，分别传 Player／Tear／Familiar／Bomb／Pickup／Knife／Projectile／Laser／Effect → boolean；true 跳过内部 AI | 对应 Entity 派生 Update；对象归原生生命周期 | 源码宏跳过派生链后仍调用 `Entity::Original_Update()`；不能推导位置、动画、事件或全部玩法均停止。NPC 69、Slot 1169 不套用这一个宏的结论，具体 native 覆盖转 ST3 |
| 碰撞局部控制：公开可满足局部语义，完整隔离需源码／native | `MC_PRE_*_COLLISION(Entity, Collider, Low)` → boolean 或 RGON table；table 可含 `Collide`、`SkipCollisionEffects` | 当前碰撞一侧的 Entity；借用两个原生对象 | `PUBLIC_DOC` 禁物理碰撞与禁碰撞效果是不同控制；后者不禁止 Collider 一侧效果。不能合并为所有伤害／拾取／碰撞已禁止。1227 `MC_PRE_ROOM_COLLISION_PASS()` → void，只通知，不能作碰撞否决 |
| 伤害局部否决：公开可满足，完整前后副作用需源码／native | 原版 `MC_ENTITY_TAKE_DMG(11)(entity, amount:float, flags, source:EntityRef, countdown:int)` → boolean；RGON增 ExtraSource和改参table（Damage／DamageFlags／DamageCountdown） | 伤害应用前；Entity／EntityRef借用原生对象 | `PUBLIC_DOC` false取消伤害且跳过剩余回调；不能据此阻止此前计算、RNG或其他入口直接改状态。保留链的前后副作用转ST3／ST4，不以回调否决代替完整隔离 |
| 物品／生成局部控制：需源码及必要native | 原版 `MC_PRE_USE_ITEM(23)(collectibleType, rng, player, useFlags, activeSlot, customVarData:int)` → boolean；`MC_PRE_ENTITY_SPAWN(24)(type, variant:int, subType:int, position:Vector, velocity:Vector, spawner:Entity, seed:int)` →可改Type／Variant／SubType／Seed的table或nil | 物品默认行为前／Entity生成前；原生Player、RNG和工厂owner | 原版Wiki明确PRE_USE_ITEM返回true仍消耗充能；PRE_ENTITY_SPAWN改参不是取消工厂的合同，文档警告以Type=0阻止生成会崩溃。不能用两者保证不耗资源、不抽随机或无掉落；ST3从实际裁决／调用路径隔离 |
| 普通自有绘制：公开可满足绘制操作 | 原版 `MC_POST_RENDER(2)()` → void；RGON `MC_POST_HUD_RENDER(1024)()` → void；`Sprite:Render(position, topLeftClamp=Vector.Zero, bottomRightClamp=Vector.Zero)` → void；`RenderLayer(layerId, position, clamps...)` → void | 已建立的正常 render／HUD 窗口；自有 Sprite 在本地运行时持有，回调本身不取得对象所有权 | 只选择具名 presentation 入口；返回 void 不证明有像素。公开 render callback 不等于可执行任意玩法 Lua。加载旁路、相机／pass、context 和 stop 时序转 ST4／ST6 |
| 派生 render 取消／改偏移：公开可满足局部语义 | 如 RGON `MC_PRE_NPC_RENDER(1081)(npc, offset)` → Vector 或 boolean；Vector 改偏移，false 取消该绘制 | 原生派生 Entity render；参数为借用对象 | 文档明确取消 NPC 绘制不停止阴影；不能将每种 pre-render 统一理解为完全阻断，也不采纳文档建议在 POST_UPDATE 写 shadow 的方式来替代 Client 隔离。保留哪些派生绘制由 ST4 决定 |
| 退出通知：公开可满足通知，保存隔离需 native | 原版 `MC_PRE_GAME_EXIT(17)(shouldSave:bool)` → void | run 退出；Game／Lua owner | `PUBLIC_DOC` 返回值不影响后续回调，结局可触发两次；不是存档否决，也不是进程关闭、GL 退休或任意一次性通知。转 ST8 |
| Mod持久与卸载：公开可辨认操作，隔离需具体源码 | 原版 ModReference `SaveData(data:string)`／`RemoveData()` →void、`LoadData()` →string、`HasData()` →bool；RGON `MC_PRE_MOD_UNLOAD(73)(mod, shuttingDown:bool)` →void | Mod／Lua owner；数据按本地存档槽持久化，卸载／进程关闭通知 | `PUBLIC_DOC` SaveData跨run／启动保留；PRE_GAME_EXIT中的Mod仍可写。卸载通知不是所有资源已退休／所有GC完成。ST7／ST8明确声明Mod的写入归属、注销与lua_close；实际文件位置和链不据Wiki示例推定 |
| Client 场景准备及退休：需 J273 native | 未确认存在“无权威 run 的 Replica 场景构造”公开接口，不编造参数或返回 | Manager／Game／Room／相机／HUD／Lua 根对象 | ST2 定义具体创建／取消／销毁顺序；同 seed 重开会执行原生玩法，不能代替此操作。当前模式只有 None／Solo／Host，真实 Client 留16接线 |

### 2.2 资源创建、图像与失败

| 必需操作／分类 | 版本、参数与返回 | 阶段／对象 owner | 失败语义与选型 |
| --- | --- | --- | --- |
| 自有 Sprite：公开可满足，需源码解释成功 | 原版 `Sprite()` → Sprite；RGON `Sprite(anm2Path?, loadGraphics=true)` → Sprite, boolean | 资源准备窗口；Lua-owned userdata，运行时须保持强关联 | `main_ex.lua:2107—2120` 包装旧构造，传字符串才 Load，第二返回值仅为 `GetLayerCount()>0`；无路径时为 false。构造出对象与全部资源准备成功分别记，不能拿 boolean 证明 PNG／GPU／像素 |
| 加载 ANM／图像：公开可满足调用，终态需源码／必要 native | 原版 `Load(anm2Path:string, loadGraphics:bool)` → void；`LoadGraphics()` → void；`IsLoaded()` → boolean；`GetFilename()` → string；`GetLayerCount()` → int | 已知主线程和资源 context 中准备未发布候选；Sprite 持有实例资源关联，缓存可共享 | 路径相对 resources。LoadGraphics 没有全层成功返回；`IsLoaded` 没有公开保证全部贴图和像素正确。ST5 分开登记 ANM、每层图像关联、姿态准备、发布结果；不为读取路径而调用 Load／Update |
| 实例换图：公开可满足，补读源码已完成 | 原版+ `ReplaceSpritesheet(layerId:int, pngFilename:string)` → boolean；RGON 增 `loadGraphics=false` | 未发布候选或已明确禁止在途使用的资源变更窗口；Sprite／layer owner | 原版+ false 包括同路径，不能一律当失败。`LuaSprite.cpp:26—44` 仅 `successful && loadGraphics` 才调用 LoadGraphics，返回的仍是 Replace 的 bool，未检查加载结果。日常选择显式换路径后统一 LoadGraphics，避免把 true 当资源就绪 |
| Reload：公开存在但不选作日常刷新／恢复 | 原版 `Reload()` → void | 资源／缓存变更；会影响借用关系的候选窗口 | Wiki 无事务／回滚保证；J460 有 layer 重建、默认路径恢复、条件性额外引用等历史反例，均为 `J460_INDEX`。ST5 若确需使用才补 J273；不通过每帧 Reload 获得正确性 |
| 图像查询／赋值：公开可满足局部操作，需源码解释 owner | RGON `Sprite:GetSpritesheet(layer:int)` → Image?；`Sprite:SetSpritesheet(layer:int, image)` 文档 void、当前绑定实际返回 boolean；LayerState 也有 Get／SetSpritesheet | Sprite 当前 layer；返回 Image userdata 持有图像关联，内部 layer 仍属 Sprite | `LuaSprite.cpp:46—84,468—491`：Get 在 layer 不存在或 image 为空时 nil；Set 返回 `layer != nullptr`，不是加载／GPU 成功。Image wrapper 复制其 image 关联；不由此保证任意线程、context 或失效层安全 |
| Copy：公开存在但不作为通用提交 | RGON `Sprite:Copy()` → Sprite | 新 Lua-owned Sprite；共享资源与新实例关系需分别管理 | 当前绑定 `construct_from_copy`，没有逐字段忠实克隆合同。J460 曾确认 Offset／flags 等差异，只作索引；ST5 如需 Copy 再解释 J273。优先自有候选切换关联，不浅拷贝内存 |
| Reset／退休：公开可满足 Reset 操作，正常关闭需源码／native | 原版 `Reset()` → void；Lua GC 无供调用方选择时刻的资源退休返回合同 | 先停发布／新 draw，处理在途使用，再在资源和 Lua owner 仍有效时清理 | Reset 不等于 native 析构、外层释放或 GL 已完成。ST6／ST7确定正常退休；不能把 `sprite=nil` 或通用 Hook drain 当作图像已释放、不能手工 free userdata |

### 2.3 main、overlay、逐层姿态与外观

| 必需操作／分类 | 版本、参数与返回 | 阶段／对象 owner | 失败语义与选型 |
| --- | --- | --- | --- |
| 选择动画：公开可满足 | 原版 `SetAnimation(name:string, reset=true)`／`SetOverlayAnimation(name, reset=true)` → boolean；`RemoveOverlay()` → void | 已加载 Sprite 的 main／overlay；姿态应用时禁止并发 draw | false 不发布目标；优先 reset=true 的明确选择。reset=false 的完整保留／失败后态不是 Wiki 保证，J460 风险不升格 J273 事实 |
| main／overlay 总帧：公开可满足整数姿态 | 原版 `SetFrame(frame:int)`／`SetFrame(name:string, frame:int)` → void；`SetOverlayFrame(name, frame)` → void；RGON 增 `SetOverlayFrame(frame:int)` → void | 当前 Sprite 两套动画 state；先定动画，再设总帧，再设独立层 | RGON 文档说明 overlay 单整数重载不停止动画；不能将所有重载合并。两侧 `GetFrame()`／`GetOverlayFrame()` → int，不表达任意分数相位。`Stop(true)` 最后固定停止；无 void 成功码，ST5核对真实后态与非法参数处理 |
| 逐层姿态：公开可满足整数帧选择，精确有效性需源码／必要 native | 原版 `SetLayerFrame(layerId:int, frame:int)` → void；RGON `SetOverlayLayerFrame(layer:int, frame:int)` → void | 已选择动画中实际存在的层；layer ID 不等于 Lua table 序号、绘制 order 或关键帧索引 | 当前 overlay 绑定只先检查 descriptor 是否存在，再调用 state SetLayerFrame；没有完整层／帧范围拒绝。先由 AnimationData 确认层，ST5核对有效映射；J460 的坏映射后仍写风险只作索引。不能直接发 raw key index 到 frame 参数 |
| 停止／播放归属：公开可满足 | RGON `Stop(stopOverlay=true)`、`StopOverlay()`、`Continue(continueOverlay=true)`、`ContinueOverlay()` → void；原版 `Play(name, force)`／`PlayOverlay(name, force)`、`Update()` → void | 自有 presentation Sprite；只由已声明展示逻辑操作 | `CURRENT_SOURCE` Stop 两套分别操作；Update 是本地动画推进，固定 Authority 姿态默认不调用。事件查询不是执行授权，事件位不驱动本地 Spawn／奖励／战斗；动画事件消费者闭合转 ST4 |
| overlay 顺序和 Sprite 外观：公开可满足 | 原版 `SetOverlayRenderPriority(renderFirst:bool)` → void；Color、FlipX／Y、Offset、Scale、Rotation、PlaybackSpeed 具名属性；RGON `GetRenderFlags()` → flags、`SetRenderFlags(flags)` → void | 自有 Sprite 实例；有界、有限值在应用窗口写入 | 显式表示 overlay 不存在与顺序；颜色按公开 Color 语义建模，不能截成 RGBA。speed 不作为本地权威时钟。flags可写不证明任意特殊分支无玩法／全局依赖；ST4／ST5按实际内容处理，不能清未知 flags 后声称忠实显示 |
| 层外观：公开可满足具名状态 | RGON `GetLayer(id:int 或 name:string)` → LayerState?；`GetAllLayers()` → table；LayerState 的 `IsVisible()` →bool、`SetVisible(bool)` →void，Get／SetSize(Vector)、Rotation(float)、Pos(Vector)、Color(Color)、FlipX／Y(bool)、CropOffset(Vector)、RenderFlags(flags)、WrapS／TMode(int)；getter 返回对应值，setter void | LayerState 为当前 Sprite 内部层的借用；GetAllLayers 的 Lua table 从1编号，层 ID 仍取 GetLayerID | `LuaSprite.cpp:114—155,494—690`：无效 GetLayer 返回 nil；向量／颜色 getter 复制值，多数 setter 直接写字段，无有限值／范围安全保证。BlendMode 的 getter 返回关联借用，不能把所有 getter 视为深复制。Load／Reload／Reset／退休后撤销借用，ST7落实 |
| 动画资源元数据：公开可满足查询 | RGON `GetAnimationData(name)`、`GetCurrentAnimationData()`、`GetOverlayAnimationData()` → AnimationData?；`GetAllAnimationData()` → table；AnimationData 的 GetName→string、GetLength→int、GetLayer(id)→AnimationLayer?、GetAllLayers→table、IsLoopingAnimation→bool | 共享、不可修改的 ANM 缓存；Lua wrapper 是内部指针借用 | 当前绑定对无 descriptor／无名字／无层返回 nil；GetAllLayers 按绘制次序而非 layer ID，空洞合法。不得以表索引推断 layer ID；ST7定义资源代次变化时重新获取 |
| 关键帧元数据／当前层帧：公开可满足查询，精确状态需源码 | RGON `AnimationLayer:GetFrame(frame:int)` → AnimationFrame?；Sprite `GetLayerFrameData(layer)`／`GetOverlayLayerFrameData(layer)` → AnimationFrame?；AnimationFrame GetStartFrame／GetEndFrame→int，GetPos／Scale／Pivot／Crop→Vector，GetColor→Color，GetRotation／Width／Height→float，IsVisible／IsInterpolated→bool | AnimationFrame 为共享只读缓存借用，插值和 root 动画已烘焙；只复制有界值到正式快照 | end 是下一帧起点。`LuaAnimationLayer.cpp:7—49` 对时间帧选记录，负值钳至0，超出取末帧；不是 selected-key ordinal 查询。`LuaSprite.cpp:197—218` 对当前 selected-key 做 clamp 后返回，不能用非 nil 证明原始 -1／坏索引有效；空轨道没有此处完整前置保护。ST5先核对有效资源，精确隐藏／无帧／相位表达有不足再补必要 J273 |
| shader／特殊几何：需具体源码，必要时 native | RGON Sprite／LayerState `SetCustomShader(path)`／`SetCustomChampionShader(path)` → void；Has…→bool、Clear…→void；专用实体／Beam 等接口按实际路径选择 | Sprite／layer 与图形缓存；受 context 和 renderer pass 管理 | 当前 shader setter 失败抛 Lua error，void 不是无失败；路径需相应 .vs／.fs及规定输入。独立 Sprite 不自动表达 Player 组合、额外 Sprite、阴影、曲线或派生 pass。ST4／ST5逐项适配，完整内容不能被旧单 Sprite profile 排除 |

## 3 本轮直接读到的关键源码事实

本地根为 `F:/projects_isaac/REPENTOGON-1.1.3/`。这些结论是 `CURRENT_SOURCE`，没有通过运行调用验证。

| 可读位置 | 事实与接口决策 |
| --- | --- |
| [ASMCallbacks.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ASMPatches/ASMCallbacks.cpp)，`MC_PRE_UPDATE`／`handle_skipped_update`／`Patch_GameUpdate_MainUpdate`，45—113 | true 分支运行 CommitRemovedEntities、render_sort、HUD::PostUpdate，然后跳往 Return。原生链未全停，拒绝把1026作为唯一隔离控制 |
| [CustomCallbacks.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/CustomCallbacks.cpp)，`RunPreUpdateCallback`／`HOOK_PRE_UPDATE_CALLBACK`，497—541 | 1160—1168传 Variant（Knife传SubType）作 filter，true 后调用基 Entity Original_Update。只跳派生，不能宣称全部 world 写入消失 |
| [main_ex.lua](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/resources/scripts/main_ex.lua)，`SpriteMT.__call`，2107—2120 | 构造器第二返回值是层数谓词；不加载或无路径返回 false。与 Wiki 的“loaded successfully”宽泛措辞相比，以此收紧成功含义 |
| [LuaSprite.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/LuaSprite.cpp)，`Lua_SpriteReplaceSpritesheet`／Get／SetSpritesheet，26—84 | 换图结果没有汇总 LoadGraphics；SetSpritesheet 文档 void 与源码 boolean 差异保留，采用公开路径换图，避免调用方依赖这个差异作 readiness |
| 同文件，`Lua_SpriteGetLayer`／GetAllLayers、`LuaGetLayerFrameDataInternal`，114—155、197—218 | LayerState、AnimationFrame userdata存内部指针；当前层帧会 clamp selected-key。不能把 userdata 存在等同有效 owner，或把夹取后的帧当原始精确状态 |
| [LuaAnimationData.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/LuaAnimationData.cpp)，5—146；[LuaAnimationLayer.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/LuaAnimationLayer.cpp)，7—49 | 动画／层／帧包装是借用；AnimationData::GetAllLayers 按 order取，AnimationLayer::GetFrame 按时间区间取。目录／协议必须区分这几个整数域 |
| [gen_binding.py](../../../mod/tools/gen_binding.py)，生成 Hook 的516—597；[persistence.cpp](../../../mod/src/game/persistence.cpp)，PersistenceScope | 当前 PGD／GameState wrapper 记录范围、前后值和实际返回，仍执行 super；Game::Exit 做既有会话退休后仍调用原链，没有 Client 条件的持久阻断 |
| [hooks.json](../../../mod/binding/hooks.json)、[registry.cpp](../../../mod/src/hooks/registry.cpp)、[context.cpp](../../../mod/src/hooks/context.cpp) | 入口、注册、常驻观察、主线程／owner 管理可复用；注册成功不证明安装／命中。当前 capacity为96，不能照搬04-08早期41／64等历史数值作为当前状态 |
| [exit_patch.cpp](../../../mod/src/hooks/exit_patch.cpp)，ExitCheckShim／InstallWorldExitPatch；清单 world_process_exit | 当前已有 J273 main 关闭判断常驻桥，服务 world recording 收口；不能沿用旧报告“当前没有退出协调代码”的历史现状描述，也不能据此声称已实现 Sprite／GL／Lua／Client场景退休 |

## 4 可复用的 J273 事实与仅可作索引的 J460 结论

下表 J273 地址均为 **RVA**，VA需加 imagebase `0x400000`。此处是已有研究归并，非本次原字节重读；如计划调用未知 native，仍先补相应 ABI，不能仅凭地址实施调用。

| 版本／既有材料 | 可以复用什么 | 不能扩张的范围／后续任务 |
| --- | --- | --- |
| J273 [04-05 §2／§6](../../research/tx-04-05-static-confirmation.md) | Odd `0x2F9340` 内三个 Player call：特殊 `0x2F93F9`、`0x2F949C`，普通 `0x2F9559`；普通点 `FF 52 0C`。加载可在 new-room callback之前更新 Entity，回调后也有条件定位／Room更新 | 不重做已有普通点识别；不能将普通点当全部 odd／FSM隔离或new-room当最终状态，ST2／ST3补剩余路径 |
| J273 [04-08 ST2 §2／§4](../../research/tx-04-08-st2-static-confirmation.md) | Manager `0x52E3E0` → Game `0x2F6D30`；Game→Room call `0x2F78DF` → Room `0x3F7F60`；Room→EntityList call `0x3F93A7` → `0x013F00`；tail `0x446C90`。主要this为ECX，tail为无参cdecl；Manager静态stdcall带bool | 顺序是采集研究已解释的普通分支，不是完整Client安全边界；同优先Hook次序、额外caller、实际短路与body到达仍分别判定 |
| 同报告，MainUpdate／Return | `0x2F77A4` 的 `FF 87 38 5B 02 00` 是官方patch点，跳往 `0x2F7A39` 越过普通Room与tail；该点不是Game入口 | 不能跳过这一段后忽略前段／外层位置恢复或后处理；ST3核对全部保留路径 |
| J273 [04-08 ST6—ST8 §2／§3](../../research/tx-04-08-st6-st8-static-confirmation.md) | Start／Spawn／Morph／Restore／Replace／Grid各有外层owner；Factory／Publish／内层Init／Restore返回不等于外层完成；GameRestore `0x2F43A0`包含ManagerRestore `0x593A90`等 | 复用生命周期语义，不把正常创建当无玩法／无RNG。客户端最小根及初始化副作用留ST2；18—20的域应用不在ST1完成 |
| 同报告 §4.3 | Shutdown `0x582C90`，main的call `0x50AFCE`；Shutdown可调用Game::Exit并销毁Game／Manager，之后没有新Manager轮次 | 这不是图形context全过程或lua_close顺序证明。当前已有exit patch见上节；ST6—ST8解释资源与Client寿命，不由录制drain外推 |
| 当前清单J273持久入口 | TryUnlock `0x503930`→bool；IncreaseEventCounter `0x503A50`→void；SaveLocally `0x503460`／SaveToSteamCloud `0x502F00`→void；Game::Exit `0x2F6270`带shouldSave；GameState write `0x59F7A0`／read `0x5A2240`→bool、Delete `0x59E7B0`／DeleteRerun `0x59E870`→void | 只确认已有登记／观测用途，不证明所有进度修改都经过这些入口，更不证明已经阻断；内存进度、run、文件、设置与Mod SaveData的归属留ST8 |
| J460 [03-10](../../research/tx-03-10-replica-update-presentation-boundary.md) | outer staged restore、普通／odd／FSM、外层tail遗漏的风险索引 | J273剩余CFG、caller和字节须ST3核对，不平移J460地址或把全部路径当已核验 |
| J460 [03-11](../../research/tx-03-11-logic-presentation-separation.md)／[03-12](../../research/tx-03-12-derived-render-sprite-lifecycle.md) | 设置帧→事件位→Shoot消费者、派生render→Lua、附加Sprite／几何、临时状态写入的研究索引 | 不是J273已闭合的事件／render调用链，不把没有直接Spawn的叶函数扩大为全树纯展示；ST4补实际保留路径 |
| J460 [03-13](../../research/tx-03-13-sprite-image-resources.md)／[03-14](../../research/tx-03-14-renderer-context-lifecycle.md) | 图像引用／缓存／GL、线程TLS与OS current分离、加载render旁路和关闭顺序的风险索引 | 所有地址、Image/control布局、window偏移和后端ABI不复用；ST6只补实际需要的J273窗口和在途关系 |
| J460 [03-15](../../research/tx-03-15-sprite-replacement-reload.md)／[03-16](../../research/tx-03-16-independent-sprite-display-fields.md) | Replace／LoadGraphics／Reload区别、main／overlay／逐层不同索引域、分数相位、特殊flags、失败反例 | Wiki明确的公开操作优先；J460的native失败、尺寸及偏移不登记为J273事实。ST5据实际缺口选择性补核 |
| J460 [03-17](../../research/tx-03-17-independent-sprite-owner-lifecycle.md)／[03-18](../../research/tx-03-18-single-sprite-adapter-contract.md) | 未发布候选、检查、切换关联、取消和正常退休的设计输入；copy／swap／GC的区别 | 不直接采用0x114 payload、64项native映射、raw selected-key写回、原生清理地址或旧flags=0支持上限；ST5／ST7以当前公开owner和必要J273事实重定 |
| J460 [03-19](../../research/tx-03-19-sprite-render-entry-window.md)／[03-20](../../research/tx-03-20-sprite-resource-pose-closure.md) | 屏幕Sprite窗口与世界绘制差异、batch引用与GPU完成差异、资源变更／同资源姿态更新的设计索引；Poof文件样本仅为历史样本 | 旧pre-present点与WGL字段不移植；样本元数据不证明J273 resolver命中，也不代替17完整边界或Player／Room／特殊画面。ST4—ST6选择真实窗口 |

## 5 接口不足与反馈任务

ST1完成的是把操作分成公开可用、需具体源码、需必要J273核验三类，并登记已知不足。下列UNKNOWN有具体负责项，不将未执行的ST标为完成，也不要求先做全映像或全Lua成员逆向。

| 问题 | 当前结论／需要补的最小事实 | 反馈位置 |
| --- | --- | --- |
| 无本地Authority run的场景壳 | `UNKNOWN`：必需根、初始化顺序、哪些路径生成房间／推进随机／回调／保存，取消如何退休 | ST2 →16的准备／等待／切换；禁止失败转Off让残留原生模拟接管 |
| 普通之外的玩法写入 | 已确认1026／1160—1168不足；`UNKNOWN`：J273 outer restore、所有实际保留odd／FSM／loading及tail的写入 | ST3；覆盖AI、碰撞伤害、物品、拾取掉落、RNG与Lua，不以输入中性代替 |
| render／HUD／动画消费者 | `UNKNOWN`：实际保留路径是否执行玩法、哪些Lua／事件消费者需抑制、必要缓存如何更新 | ST4 →18—20／27；具名presentation入口与Authority表现事件分开 |
| 资源成功分层 | 已知构造bool仅层数、换图bool仅替换、LoadGraphics void；`UNKNOWN`：J273必要终态如何可靠判断、缺失／部分失败后如何清理 | ST5 →15的资源结果：ANM／layer／图像／姿态／发布分开；资源准备成功不能发世界Applied ACK |
| 精确姿态与公开查询失真 | 公开整数frame不能表达任意分数；当前帧查询有clamp，不能识别所有无帧／隐藏／非法态；AnimationFrame为烘焙只读记录 | ST5 →15：main／overlay显式有无、顺序、layer ID／order／时间帧／关键帧区分、独立层覆盖及必要相位。实际需求超公开语义时才补native，不先选raw key写回 |
| shader／特殊原版显示 | 公开setter存在不证明全部视觉已可复制；`UNKNOWN`：必须的额外Sprite、Player组合、阴影／曲线／后处理与资源依赖 | ST4／ST5 →18—20／27／29；原版完整内容有具体适配归属，不以旧flags=0 profile缩小最终范围 |
| 线程、context及在途批次 | `UNKNOWN`：J273正常／加载／退出窗口、当前OS context、batch引用和最终释放位置 | ST6 →16主线程资源服务；网络线程只带有界自有值，不带userdata、Player或图像指针 |
| Lua与内部借用 | 源码确认内部指针包装；`UNKNOWN`：当前版本Load／Reload／Reset／缓存重建／lua_close的具体撤销和GC顺序 | ST7 →15引用／资源revision与本地instance代次；资源变化后重新获取，不拿指针相等证明有效 |
| 持久归属与失败退出 | 退出callback不能否决保存；当前观察仍执行super；`UNKNOWN`：Client准备至关闭实际可达的进度、run、Mod写入与允许设置 | ST8 →16／31；在内存进度与原生写入之前处理，最终dirty=0／文件未变不能证明全过程 |

给15的结果模型采用语义而不在本次固定wire编码：资源／对象引用含所属scene与资源修订；同revision重复与冲突区分；错误至少含目标、资源、层／动画、执行前缀和原因。区分缺失、不支持、未知、过期、部分准备、已取消、已退休和真正发布。是否保留最后已确认画面遵循正式恢复模型；半套候选和失败目标不能冒充已应用。给16的生命周期要求是准备／展示／退休全程保持Client隔离，安全退休后才转Off；这些均为`DESIGN`，没有新增运行入口或第二协议。

## 6 本次收束与运行时留项

已把公开文档、当前源码、可复用J273证据和J460索引分开；工作流程所列三项已知限制均经原文／源码回读，新增构造器bool、换图bool、层帧clamp和借用owner的具体限定。接口选型不要求新增逆向即可成立；仍需补的J273问题已归ST2—ST8。

本次通过资料阅读和编辑后的人工文本回读收束，不执行独立格式／语法／链接校验器，不对子项安排测试或消费tx-04-17全目标统一E2E。未观察当前安装环境实际Hook链、开关、命中、线程／context、调用次数、资源显示、像素、失败恢复或全过程持久副作用；这些仍留完整实现后的统一E2E。ST1资料工作完成不代表隔离、运行时或整个tx-04-17完成，不新增artifact包。
