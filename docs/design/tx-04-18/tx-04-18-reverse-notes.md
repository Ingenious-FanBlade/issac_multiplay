# tx-04-18 Room／Grid 客户端展示静态逆向笔记

日期：2026-10-31。范围：为 tx-04-18 的 Room/Grid 专用纯值展示模型提供静态事实、渲染边界和未知项。目标为官方 REPENTOGON 1.1.3、Repentance+ v1.9.7.12.J273。未运行游戏、构建、测试或校验脚本，未修改产品代码或 IDB；本文件只记录静态分析，不表示功能通过。

证据标签：**SOURCE** 为本地官方 REPENTOGON 1.1.3 SDK/源码/Wiki；**J273-STATIC** 为该目标 IDB 的只读静态指令、伪码或与官方 ZHL 签名匹配；**DERIVED** 为由这些事实得出的客户端边界；**UNKNOWN** 表示当前静态闭包不够。

IDA 当前只读 IDB：`isaac-ng-v1.9.7.12.exe.unpacked.exe.i64`，imagebase `0x400000`。下文 RVA = VA − `0x400000`。绝对函数地址均同时给出 VA/RVA，未给二进制哈希。

## 1. 结论

独立 Client 展示必须表达完整房间几何与所有当前可见的房间／Grid 派生部件，不能只存一个 `RoomShape`、一个背景 Sprite 或一份 `GridEntityDesc`。448 个 Grid 槽、8 个门槽、RoomConfig 几何、格子占用、Backdrop floor/wall、环境色与水、门附加 Sprite、TimerPlate、岩石前景 top、Grid 水裁切和光照/遮挡都不是单张 ANM2 的同义词。

原生 Room/Grid render 不是可直接复制到 Client 的纯函数。`Game::Render` 将 Room 前置处理与 Room 渲染分别纳入实际 Room／Entity／Grid／graphics 对象链；`Room::PreRender` 会逐个遍历实体和 Grid，并操作 Room 自有缓存/临时结构。Grid 虚 Render 从 live Grid 的内嵌 ANM2 和全局 graphics manager 取值，改写 ANM2 的 render state 后再调用 AnimationState renderer。压力板 Render 还根据 Manager 帧计时绘制计时文本。完整原生 Room/Grid draw 树及其回调、缓存、renderer state 和所有异常恢复没有闭合，不能作为隔离 Client 的安全复用入口。

推荐把 Room/Grid 完整语义拆成两份同 revision 的纯值：

1. **RoomGeometry/Environment**：显式形状占用、坐标、墙/地面/边界、门位、Backdrop 的本地资源键及具体视觉状态、颜色/光照/水/FX/环境状态和相机/scroll 值。
2. **GridState + GridVisualParts**：保留 Grid 身份、类型、变体、状态与保存描述；每个 Grid 对应零到多份受 allowlist 限定的 ANM2 视觉部件，分别携带资源键、主/overlay 动画帧、层姿态、颜色、变换、water flags、clamp、明确的 pass 和顺序。

RoomGeometry、全部 448 个槽、8 门别名视图以及所有 Sprite 资源都应纳入同一个候选准备、发布和退休事务。未知类、缺失引用、缺部件、越界/重复槽、无效 pose 或资源失败时拒绝整个候选，不显示半房间。

## 2. 原生 Room 渲染入口及复用限制

| J273 点位 | 证据 | 安全含义 |
| --- | --- | --- |
| `Game::Render` VA `0x6F7A80` / RVA `0x2F7A80`；VA `0x6F7AE1` 调 `0x803E20`；VA `0x6F7DF4` 调 `0x804500` | J273-STATIC，与 tx-04-17 ST4 的 Game::Render 路径复核一致。官方 `Room.zhl` 声明 `Room::PreRender()`，签名在目标中唯一匹配 VA `0x803E20` / RVA `0x403E20`，长度 `0x6D3`。`0x804500` / RVA `0x404500` 是 Game::Render 的 Room 主绘制候选，函数长度 `0x3BF0`；Hex-Rays 对其报告 bad-SP，伪码不宜当完整语义。 | 不从 `Game::Render` 或 Room 渲染入口建立“只画图”的证明。0x804500 的全部副作用、早退和异常恢复仍 UNKNOWN。 |
| `Room::PreRender` VA `0x803E20` / RVA `0x403E20` | J273-STATIC：读取 Room 与全局 Game/Manager/graphics 状态；遍历 Room EntityList，逐项调用 `0x7FC110`；再按 `gridWidth*gridHeight` 遍历 Grid 槽并对非空槽调用 `0x801BE0`。它还初始化/清理 Room 自有临时列表并触及图形批次上下文。ZHL：`F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Room.zhl:53`。 | 这是 entity/grid render-preparation 与 renderer 状态路径，不是外置场景绘制 API；不能在 Client 每帧调用以“补齐环境”。`0x801BE0` 中 Grid type switch 还有 graphics、stage/room 和 Helper 依赖；完整 RNG/写入语义 UNKNOWN。 |
| Room 的公开 `Render()` API | SOURCE：本地原版 Wiki `F:/projects_isaac/IsaacDocs/docs/Room.md` 有 `Room:Render()`；REPENTOGON Lua API在当前 Room userdata 上增加/修改其他 Room 能力，但没有无 Room owner 的 detached room renderer。 | API 可从当前原生 Room 绘制，不能用于一个没有本地权威模拟的纯值 Room 副本。 |
| `Room::RenderGridLight` / `Room::RenderEntityLight` | SOURCE/ZHL：`Room.zhl:56,59` 声明 Grid/Entity 专属光照路径；`Room` 自身拥有 `FXLayers`、Backdrop、floor/wall ColorMod、water 状态等。 | 不能在 Grid sprite 之外假定背景、光照或可见性已自动包含。静态材料尚未闭合它们的精确 pass 次序和全部输入。 |

`Room::PreRender` 看到的 `0x7FC110` 和 `0x801BE0` 都是非 trivial helper，且直接遍历原生实体/Grid；没有理由认为其输出只写 graphics buffer。不能据一次成功画面将整个 Room helper 树列入可复用白名单。

## 3. J273 Grid 虚 Render 映射

SOURCE：`GridEntity.zhl` 定义 `Render(Vector& Offset)` 为 virtual slot +0x0C；`GetWaterClipInfo(WaterClipInfo*)` 位于 +0x18。静态读取 vtable 的 +0x0C 槽与 `tx-04-08-grid-kinds-static-confirmation.md` 中 J273 RTTI/vtable RVA 对应。表内 RVA/地址为本目标；共享实现表示多个派生类指向同一 render 函数，不表示它们视觉语义完全相同。

| Grid 类型 | vtable RVA | Render VA / RVA | 静态事实与额外部件 |
| --- | --- | --- | --- |
| Decoration | `0x7196CC` | `0x973190` / `0x573190` | 调用 `GetRenderPosition` 与内嵌 ANM2 render。 |
| Rock | `0x71969C` | `0x7146E0` / `0x3146E0` | 另有 `GridEntity_Rock::RenderTop` VA `0x7149E0` / RVA `0x3149E0`；Room 主 render VA `0x804500` 在 `0x8060F0` 直接调用该 top 方法。必须作为独立前景/遮挡部件保留，普通 Grid sprite 不等价。 |
| Pit | `0x71A004` | `0x70DE30` / `0x30DE30` | 独立 override 与不同 WaterClipInfo。 |
| Door | `0x71966C`（由 +0x0C 槽 `0xB19678` 复核） | `0x707F90` / `0x307F90` | `GridEntity_Door.zhl` 声明对象内还有 `ANM2 _extraSprite : 0x168`；Room 公开 API也有 `ExtraSprite`，用于 bars/chains/boards 等。即便门 base Sprite 已出现，仍可能缺额外图层/遮挡部件。 |
| Poop | `0x71960C` | `0x710130` / `0x310130` | 专用 override；水态可随破坏状态改变，不能只从基础类型猜 WaterClipFlags。 |
| Wall | `0x71972C` | `0x71B4E0` / `0x31B4E0` | 独立 wall render override。 |
| Fire | `0x71978C` | `0x67B620` / `0x27B620` | 独立 override；普通 Grid/ANM2 资产键不足以暗示其环境/动画状态。 |
| PressurePlate | `0x7195DC` | `0x95E140` / `0x55E140` | 独立 override；Greed 计时板还要单列 TimerPlate Sprite 和权威计时显示值。 |
| Lock | `0x719F14` | `0x70CB80` / `0x30CB80` | 与 Teleporter 共用函数，不等同于共享资产或 pose。 |
| Teleporter | `0x7196FC` | `0x70CB80` / `0x30CB80` | 与 Lock 共用函数。 |
| Spikes | `0x71A2E4` | `0x719270` / `0x319270` | 使用共享基础 render。 |
| Web | `0x71A578` | `0x719270` / `0x319270` | 使用共享基础 render。 |
| TNT | `0x71A32C` | `0x719270` / `0x319270` | 使用共享基础 render。 |
| TrapDoor | `0x71A484` | `0x719270` / `0x319270` | 使用共享基础 render。 |
| Stairs | `0x72E758` | `0x719270` / `0x319270` | 使用共享基础 render。 |
| Gravity | `0x72E6DC` | `0x719270` / `0x319270` | 使用共享基础 render。 |
| Statue | `0x71963C` | `0x4291D0` / `0x0291D0` | IDA 将该 slot 指向 `nullsub_9` 空函数；可见效果可能由房间/Backdrop/别的对象层表现。不能假设空函数即类型无视觉语义。 |

Render RVA 表由 vtable `base RVA + 0x0C` 读取，例如 Rock RVA `0x71969C` → VA `0xB196A8` 内容为 `0x7146E0`；其他项同法。Grid factory 1..27 映射到这些 17 类已由 tx-04-08 静态核定；视觉组件仍应按当前状态/变体而不是仅按派生类名选择。

多数可见 Grid Render 的共同路径读取 `GridEntity::GetRenderPosition`（VA `0x705110`）、读取全局 graphics manager 字段并写入该对象内嵌 ANM2 的 render color/speed/flag 字段，然后进入 `AnimationState::Render`（VA `0x405990`）/`ANM2::Render`（VA `0x4059E0`）。它们没有因此变成可对纯值结构调用的函数；直接调用仍需要匹配的 live Grid、ANM2/cache、全局 renderer 和 graphics context。不同类型仍有覆盖函数，不能以“多数最后都调用 Sprite”归一化为单 Sprite。

**TimerPlate 的动态值是硬性隔离点。** PressurePlate Render VA `0x95E140` 静态读取 `n187[24768]+29244` 的计时字段，将 `(value−1)/30` 格式化为 `%02d:%02d`，调用文本绘制 helper；也依据 Manager/global 帧状态设置显示颜色/闪烁。原版 Wiki `GridEntityPressurePlate.md:46-49` 明确 `TimerPlate` 是 Greed 压力板下面的 Sprite。Client 应接收 authority timer 和一个独立 TimerPlate 视觉部件，不运行这个 Grid Render，不从 Client Manager 读取计时。

Door 方面：`Room.zhl` 的 `_doors[8] : 0x724` 是单独的八门视图，`_doorGridPositions[8] : 0x744` 保存门位置；[Grid Room 逆向记录](../../hooks/grid-room.md)已确认 MakeDoor 可令同一 Grid 对象同时出现在 Grid 槽和 Door 槽。纯值模型应保留别名关系/门槽，不重复生成第二个实体；门 visuals 可以有多个 Sprite 部件。Room 还含 `_redDoorOutlines[8] : 0x7744`，特殊红门轮廓亦需显式表示。

## 4. 房形、几何、遮挡与环境状态

### 4.1 房形不是 width×height 或 Shape 一个枚举

SOURCE：原版 `F:/projects_isaac/IsaacDocs/docs/enums/RoomShape.md` 列出 `ROOMSHAPE_1x1`、横/纵窄房、1x2、2x1、2x2 和四种 L (`LTL/LTR/LBL/LBR`)；GridIndex 上限随形状分别从 134、239、251 到 447。Room 的原生固定 Grid 数组由 `Room.zhl:184` 定义为 `0x1BF`（447 个索引，即可表示 0..446；目标 Reader/tx-04-08 的公共采集边界继续使用 448 个槽 0..447，原生结构边界差异按项目现有核定口径处理，见 UNKNOWN）。原版 Shape 图片在 `IsaacDocs/docs/images/roomshapes/`。

REPENTOGON SOURCE：`Room:GetLRoomAreaDesc()` 返回 worldspace 两矩形区域，`GetLRoomTileDesc()` 返回 gridspace 高/低两区域的四角（`docs/docs/LRoomAreaDesc.md`、`LRoomTileDesc.md`、Room.md 的 modified functions）。因此 L 形不是单纯非矩形相机框，必须明确高/低矩形的实际 tile 占用、地板/墙边缘与缺口。

最低几何值：`RoomShape` + 明确 tile mask/坐标 + `gridWidth/gridHeight` + world-space extent/origin + floor/wall/edge 视觉组件 + grid-index→世界点映射 + camera/render-scroll 值。仅 width×height 会把 L 形缺区和短房 padding 错画成地面；仅 shape id 也不携带 RoomConfig 中的具体房间布局/门点/装饰位置。Grid 的碰撞类、可见 pass 和遮挡占位应分别记录，不能从 Sprite 是否存在反推碰撞。

Room descriptor 还区分 `AllowedDoors`、实际 door slots、连接目标和 `RoomShape` 能否容纳对应 DoorSlot。参考 REPENTOGON `RoomDescriptor.md:9-24,150-158`、原版 `Room.md:GetDoorSlotPosition/IsDoorSlotAllowed/GetGridIndex/GetGridPosition`；允许开口、目前存在的门和门贴图是三个不同事实。

### 4.2 Backdrop 与场景环境不可省成一张地板图

SOURCE：官方 `Backdrop.md` 暴露 `GetFloorANM2()`、`GetDetailANM2()`、`GetControlsANM2()`、`GetControlsButtonsANM2()`、`GetFloorImage()`、`GetWallImage()`。`Room.md` 暴露 `GetBackdrop()`、`GetBackdropType()`、`GetFloorColor()`、`GetWallColor()`、`GetWaterAmount/Color/ColorMultiplier/Current()`、`GetFXLayers()`、`GetLightingAlpha()`、`GetNumRainSpawners()`、`GetRainIntensity()`、`GetLavaIntensity()`、`GetRenderScrollOffset()`、`GetRenderSurfaceTopLeft()` 和 `GetRoomShape()`。

官方 `libzhl/functions/Backdrop.zhl` 进一步给出 floor/wall/surface/temp buffers（`+0x0/+0x8/+0x10/+0x18`）、其他图像指针（`+0x24/+0x2C/+0x34`）、floor ANM2 `+0x3C`、backdropId `+0x150`、63 个 config `+0x154`、Backdrop 自有 RNG `+0x4F28`/seed `+0x4F38`、wall-detail/controls/buttons ANM2 (`+0x4F3C/+0x5050/+0x5164`)。render 方法包括 `RenderWater`、`RenderWalls`、`RenderFloor` 和 `pre_render_walls`。`Room.zhl` 有 Room 内嵌 Backdrop `+0x1BC8` 及 floor/wall color modifier、FXLayers、water, rain, lava 状态。

这说明原生背景至少包含缓存 floor/wall 图、不同 surfaces、动画层和独立的水/墙/地板 pass；地板图片像素或 `BackdropType` 单字段都不足以重建其精确显示。环境模型至少需包含本地受支持资源/变体键、floor/wall/detail/controls 可见性及 pose、房间内外颜色 modifier、光照/水色与水量、FX/rain/lava 以及对应动画相位；不支持的资源/FX 类型应明确拒绝或显示为 unsupported，不能在 Client 根据 seed 重新随机选 Backdrop。

### 4.3 Occlusion 与 pass

静态来源明确存在地板/墙、Grid 主体、单独岩石 top、门额外 Sprite、灯光、Grid/Entity、水、Room FX 多种来源；精确跨域排序和所有 overlay caller 尚未完整重建。因此明确的 RoomGrid pass/ordinal 必须成为数据的一部分，不能依赖 `std::stable_sort` 对同一默认 pass 下数组顺序偶然出图。对每个组件单独保留 owner、depth/layer、遮挡关系、前/后景 pass、water mode、变换和 clamp。最低职责应能表达地板/坑/水、grid-under、world occupant 的空层锚点、grid-front/rock-top、door overlays、lighting/FX；真实顺序由受支持资源/场景在最终 E2E 检查。

## 5. 水裁切与 Sprite clamp

SOURCE：`WaterClipInfo.zhl` 只有 `{ uint32 bitFlags; float unkFloat; }`，大小 `0x8`。REPENTOGON `enums/WaterClipFlag.md` 定义：bit `1<<1 DISABLE_RENDER_ABOVE_WATER`（Entity 上会启用 below pass）、bit `1<<2 ENABLE_RENDER_BELOW_WATER`（Entity 可同时 above/below）、bit `1<<3 DISABLE_RENDER_BELOW_WATER`（优先）、bit `1<<5 DISABLE_RENDER_REFLECTION` 和 bit `1<<6 IGNORE_WATER_RENDERING`（标明 Entity only；Grid 不做反射）。Lua `GridEntity:GetWaterClipFlags()` 仅暴露 flags；`GridEntityPlus.cpp` 可覆盖 flags，但 `WaterClipInfo.unkFloat` 未暴露。Grid/派生类都在 ZHL 上声明相应 `GetWaterClipInfo` 虚函数。

原版 Sprite Wiki 仅说明 `Sprite:Render(position, topLeftClamp, bottomRightClamp)` 在屏幕位置渲染，默认两 clamp 为零。更具体的 J273 伪码：

- `ANM2::Render` VA `0x4059E0` / RVA `0x0059E0` 把两个 clamp 传到 AnimationState；该函数本身不做 Update。
- `AnimationLayer::RenderFrame` VA `0x404570` / RVA `0x004570` 调 `GetSourceQuad` VA `0x404F60` / RVA `0x004F60` 和 `GetDestQuad` VA `0x405360` / RVA `0x005360`，并进入 graphics/shader/render queue helpers。
- `GetSourceQuad` 用 layer/frame scale 将 topLeftClamp 的 x/y 边距加到 source 左/上边界，并从 source 右/下边界扣除 bottomRightClamp 的 x/y 边距；近似为 `sourceLeft += TL.x/(layerScale.x*frameScale.x)`、`sourceTop += TL.y/(layerScale.y*frameScale.y)`、`sourceRight -= BR.x/(...)`、`sourceBottom -= BR.y/(...)`。这是矩形 sprite-local 裁边输入，不是 world-space 任意几何 clip plane。
- `GetDestQuad` 在目标四边形计算中也消费同一对 clamp，处理帧尺寸、层/帧缩放、翻转、display offset 与 Manager/graphics viewport 全局位移；已回读函数体只看到几何/坐标 helper，无直接 RNG 写入。

对**轴对齐**的水面切口，可按每个组件未裁 Quad 与 authority waterline 分别转换为本地裁边：只要上半部时从该部件 bottom 使用 bottomRightClamp.y 裁掉线下部分；只要下半部时从 top 使用 topLeftClamp.y 裁掉线上部分。必须纳入帧裁剪尺寸、layer/frame scale、pivot/offset、flip、viewport/camera；不能对全房用同一个 `Vector` 或把世界水位 y 原值填入 Sprite clamp。旋转、shear、shader mask、孔洞及多层动画的精确 clipping 不由两个边距完整表达；超出支持路径时需要额外遮罩/分片视觉组件或明确降级，不调用 native Grid render 代替。

Grid water flags 用于选择 above/below water draw pass；**它们不等于 clip vectors**。waterline/amount/color/transition 必须来自 Authority Room 状态；具体 `WaterClipInfo.unkFloat` 的语义、原生用于哪条水边/阈值、全部遮罩/反射顺序均为 **UNKNOWN**。尤其不能把 Entity water/reflection flag 文档外推为 Grid 的反射行为。

## 6. ANM2／Backdrop 公开 API 能否独立取完整画面

SOURCE：通用 `Sprite()` + `Load(path, loadGraphics)` + `LoadGraphics` + pose setters/render 属于独立 ANM2 资源呈现路线；当前 [presentation.cpp](../../../mod/src/game/presentation.cpp) 已有自有 ANM2 所有权、资源校验、Animation/Overlay/LayerState 姿态设置以及显式节点/pass。它可承载经过 allowlist 的 backdrop/grid 子部件，不会构造 Entity/Grid 或 Room；每个 Grid/门/动画前景仍需按内容拆成多部件。

`Room:GetBackdrop()` 和 Backdrop getters 返回的是活动 Room 内嵌对象的 Sprite/Image 视图。其 floor/wall images 是 Backdrop 自有的 Image/cache state；`Backdrop::Init(backdropId, LoadGraphics)` 会初始化拥有者/config/随机资源，`pre_render_walls`/RenderFloor/RenderWalls/RenderWater 生成/消费多张缓存表面。没有官方公开 detached Backdrop factory、无初始化 clone 或“把 Room backdrop 完整压成一个可传输 Sprite”的接口。Getter 能让 Authority 读取可用 Sprite/Image/状态，但不能把 native userdata/texture 指针安全传到 Client 进程或变成完整网络值。

因此：可从 Authority 侧生成明确的 `{resource-key, animation, frame, overlay, layer poses, color, transform}` 清单，Client 用自有 ANM2 逐层准备；针对缓存墙/地板图，必须由支持的本地 asset/resource mapping 或独立的完整场景绘制组件提供。仅凭 BackdropType/seed 本地调用 `Backdrop::Init/Room::Render` 会重新引入 RNG、native Room/Manager/graphics owner 和未闭合顺序；不建议这样做。公开 Wiki 没有保证被动 capture 的 Sprite getters 已包含所有生成表面、shader、wall edge/corner、room-specific tint 或水覆盖。

## 7. 渲染副作用：必须在纯值适配中截断

### 7.1 GLITCH 位会推进全局 RNG

J273 `AnimationLayer::GetSourceQuad` VA `0x404F60` 在 `((layer.flags | sprite.flags) & 0x2) != 0` 分支直接推进全局 `dword_BAA104` 两次并用结果抖动 sprite source quad。REPENTOGON `AnimRenderFlags.md` 把 `1<<1` 命名为 `GLITCH`。因此即使只用独立自有 Sprite，如果原始 flags 中保留该位，native Draw 仍会写目标进程全局 RNG。该全局 RNG 与玩法 RNG 是否相同/共享为 **UNKNOWN**，但 Client 渲染不能消费它。

主审补充措施：对 GLITCH 位要求 Authority 明确给出 `resolved_random_uv`，在候选准备阶段把其结果 bake 成本地 source crop/逐层视觉值，随后在 ANM2 和 LayerState 的 native flags 都清除此位；缺值则拒绝该部件。不得仅清一个层或 Sprite 上的副本，因为 Wiki 说明 root 与 layer render flags 相加。

### 7.2 GetDestQuad/RenderFrame 检查

J273 `GetDestQuad 0x405360` 已逐体回读，无直接 PRNG 写入，callee 为坐标辅助 `0x9E1230`；`AnimationLayer::RenderFrame 0x404570` 调 `GetSourceQuad` 和 `GetDestQuad` 后进入 graphics/render-target/shader helpers，本次没有发现它自己的第二个直接 PRNG 分支。因此在已读闭包中 GLITCH 来源是 GetSourceQuad 的上述 bit 路径；不将其扩张为“全部 native RenderFrames 无其他副作用”。RenderFrame 仍操作图形栈和全局 renderer 状态，完整 helper／故障早退恢复未知。

REPENTOGON `AnimRenderFlags.md` 还列 `COLOROFFSET_CHAMPION(1<<4)`、`STATIC(1<<5)`、`IGNORE_GAME_TIME(1<<6)`、`GOLDEN(1<<7)`、`ENABLE_LAYER_LIGHTING(1<<10)`、`ENABLE_NULL_LAYER_LIGHTING(1<<11)`；文档提示 root/layer flags 会相加。`Anm2Extras.cpp` 对 Dogma/Gold special flags 按 `_frameCount` 或 Manager `_framecount/2` 取动画时间，`InterpolV2.cpp` 会在 `AnimationLayer::RenderFrame` 临时写共享 AnimationFrame 的颜色、尺寸、位置/rotation再于正常返回恢复；它不是隔离快照证明。Client 应从权威展示时间和私有 Sprite/layer cache 构建 pose，不读 Client Game tick来决定世界视觉帧，也应将 shader/lighting/transition 状态显式准备及恢复。

### 7.3 render 本身的对象依赖

Grid override 函数会从 live Grid 的 type/state/variant、内嵌 ANM2 layers 和 `GetRenderPosition` 取状态。Door 附 Sprite、PressurePlate TimerPlate、Rock top 是跨多个对象的组合。即使大多数 Grid override 未见直接调用 `Update/Spawn`，这也不是可重用证明；Room PreRender/Room Render、Grid’s extra helpers、REPENTOGON render hooks、异常/重入情况下的对象恢复和动态 Mod callbacks并未全量闭合。按当前目标选择 Client draw-only 表示，不调用任何 `GridEntity::Render`、`GridEntity::Update`、`Room::PreRender`、`Room::Render`。

## 8. UNKNOWN 与实现审阅点

1. `0x804500 Room::Render` 的完整反编译有 bad-SP 告警，完整正常/异常/早退副作用、全调用者集合和实际 hook/Mod callback 扩展未闭合。不能对其全树作纯展示声明。
2. `Room::PreRender 0x803E20` 内 `0x801BE0` Grid type switch 的隐藏更新/随机/全部写入来源未闭合。render 走原生容器，不能认为输入是纯 Grid snapshot。
3. `WaterClipInfo.unkFloat`、原生 `Room::Render` 中 WaterClipFlags 到分 pass/clip 位置的映射、Grid 默认 below-water规则、完整 water render 以及 Grid reflections均 UNKNOWN。当前公开 API 对 Grid 仅暴露 bitFlags。
4. Room geometry 中固定 `0x1BF` 个 ZHL槽与 TX-04-08 sampler 使用 448 槽的边界差异需沿现行生产读取/索引契约复核，不能静默少一个或越界；这是几何数组边界，不是本轮运行时观察。
5. Backdrop getters 返回具体当前 Room 的 Borrowed views；资源路径/纹理缓存能否在当前 Client scene generation 中逐项复建、内容包差异、无ANM2图层的 generated wall/floor/water surface、特殊 boss/secret/greed/repentance环境动画仍需显式支持表。不能声称仅枚举官方基础 Backdrop 即包含全部第三方内容。
6. 门额外 Sprite 的最终 pass caller、特殊红门 outline 和 door/transition room-type badges 的实际组合；PressurePlate 不同 variant/time/colors；所有派生类 resource/animation mapping与资源缺失时的准确画面仍要在完整 tx-04-18验收中观察。静态字段或公开 Sprite getter不证明所有像素正确。
7. `GLITCH` 的全局 RNG 与玩法 RNG 是否共享 UNKNOWN。处理方式不依赖该问题答案：native随机位必须从 Client Sprite flags 清掉，并使用Authority解析值 bake。

本报告提供 Room/Grid 的纯值适配依据与具体缺口，不运行任何测试、不宣称实现/视觉完整性已经 PASS。最终实机测试按现行规范在 tx-04-18 目标全部实现后统一执行。
