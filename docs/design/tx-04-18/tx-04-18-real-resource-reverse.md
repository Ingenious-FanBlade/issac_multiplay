# tx-04-18 原版资源捕获与重建：静态逆向补充

> **历史研究，路线已替代（2026-10-31后续会话）**：按[原生状态应用方案](tx-04-18-native-state-plan.md)实施create／remove／change。本文资源读回／上传／绘制重放研究保留原始事实与未知，不再作为18正式实现方向，不代表原生状态恢复不可行。

**范围**：v1.9.7.12.J273 + 官方 REPENTOGON 1.1.3。只读检查本地 SDK/Wiki、REPENTOGON 源码和 IDA 数据库；没有运行游戏、测试或校验脚本，没有改产品代码。本笔记是实现方案依据，不证明原版资源已应用，也不证明真实房间 E2E 通过。

## 结论先行

1. Authority 上可在原版正常绘制时被动记录已解析的 `SourceQuad`、`DestinationQuad`、四角颜色、图像对象、材质/滤镜/混合模式，以及 ANM2 渲染生成的最终顶点属性；客户端不需要调用 `Room::Render`、`GridEntity::Render`、`Backdrop::Init/Render*` 或任何玩法方法。核心图像入口是 `ImageBase::Render`，但它必须与 Room/Grid/Entity 的上层作用域配合，且着色器属性不能在该函数刚返回时就复制。
2. Backdrop 的地板/墙壁缓存不是单个 ANM2；官方 Backdrop getter 返回当前活动 Room 内部的 `floorBuffer`、`wallsBuffer` 图像引用。Authority 可在其原生生成和绘制之后读取这些**具体 surface** 的像素。水图案、遮罩和动态 tint 还需捕获对应原生 pass/贴图/顶点语义。
3. 官方 Lua 只提供 `Renderer.CreateImage`、`Renderer.RenderToImage` 和 `Image:GetTexelRegion`，没有内存像素写 API。J273 二进制表明最窄的本地上传手段是给客户端自己持有的 procedural image 调 `glTexSubImage2D`；图像主 texture name 位于 `ImageBase+0x78`（私有字段，只从二进制确认），可以避免磁盘 PNG 和危险的重复 `SetTexelData`。这不是 SDK 保证的 ABI，须限制在同一受控图形上下文、版本钉定且状态完整保存/恢复的 native helper 中。
4. `GetTexelRegion` 不适合在 Room/图像批次内部调用。它不修改图像像素，但会将被读图像设成 framebuffer target，调用 `glReadPixels(GL_RGBA, GL_UNSIGNED_BYTE)`，并修改/恢复引擎全局 framebuffer/viewport manager 状态。应在 Authority 的原生 Room 绘制返回、图形队列排空且同一 GL context 仍有效时，对个别资源 surface 读取；不能把它当纯内存 getter。

## 1. Authority 上完整 pose 与已解析 quad

### ANM2 值来源

- 官方 `docs/docs/Sprite.md` 暴露 `GetLayerFrameData(layerID)`、`GetOverlayLayerFrameData(layerID)`、`GetCurrentAnimationData()`、`GetSpritesheet(layer)`。
- 官方 `docs/docs/AnimationFrame.md` 的 frame getter 提供当前解析后的裁边、宽高、位置、缩放、pivot、旋转、颜色、可见性及起止帧等；其说明指出插值/root animation 已经烘焙进共享的每层 frame 缓存。官方 `docs/docs/LayerState.md` 暴露 spritesheet path/Image、visible、size、rotation、position、color、cropOffset、render flags、blend mode、wrap/filter、flip 与 shader 等每层状态。REPENTOGON 源码 `repentogon/LuaInterfaces/LuaSprite.cpp` 中 `LuaGetLayerFrameDataInternal` 由当前 `AnimationState::_layerFrames[layerIndex]` 选择帧，再返回 `AnimationLayer::GetFrame(index)` 的已解析缓存。
- 对独立 pose 表示，至少传输每个 layer 的帧索引/overlay 帧索引和当前 `LayerState` 全字段。仅传 ANM2 路径后在 Client 推进 timeline 不等价：它会依赖本地 timeline/时间/Root 状态，还可能重新触发随机 UV/动态状态。匹配的本地资源只用于纹理资产解析，不应用作重新求 pose 的唯一输入。

### 精确显示 capture 的更强入口

- 官方 `libzhl/functions/AnimationLayer.zhl` 声明：`RenderFrame(position, frameIndex, topLeftClamp, bottomRightClamp, ANM2*)`；静态 helper `GetSourceQuad`、`GetDestQuad` 接收真实 `AnimationFrame`、`LayerState` 与两个 clamp `Vector`。
- REPENTOGON `repentogon/Patches/Anm2Extras.cpp:248-343` 的 `RenderFrameEx` 是其实际 hook 重实现：先检查 layer/frame/ANM2 可见与光照 flag，取得当前 `LayerState` 图像，计算 `GetFrameSourceQuad` / `GetFrameDestinationQuad` 与 blend/color，然后调用 `ImageUtils::SubmitQuadForShader`。因此在底层图像 Render 捕获到的 quads 已经是原版使用的最终裁边/目的几何，不必在 Client 重跑 ANM2 quad 算法。
- `LuaRender.h` 中 RenderContext 注释明确指出：单看顶点缓冲无法知道某个绘制操作属于哪个 Entity/ANM2/layer；REPENTOGON 通过 `EntityNPC::Render`、`AnimationState::Render`、`AnimationLayer::Render` 逐层推上下文，最终在 `ImageBase::apply_data` 关联 vertex buffer。故只 hook `ImageBase::Render` 无法安全将全部绘制归到 Room/Grid，也无法自然排除 Entity。

**建议 Authority capture 值**：源图像的可稳定映射键或纹理字节/辅助纹理；Source/Destination Quad 的四点与方向；四角颜色；当前 shader/program、shader 的每顶点附加属性与 uniforms；image sampling/wrap/filter/预定义 shader；manager blend mode；target/surface 身份；room-local camera/scroll/clip；draw sequence。任何一项无法解码/传输时，应对该视觉部件拒绝发布或标记不支持，不能以“画一个 Sprite”代替完整场景。

## 2. `ImageBase::Render` 与 shader 属性捕获时机

- 官方 `libzhl/functions/KAGE_Graphics_Image.zhl` 将 `KAGE_Graphics_ImageBase::Render` 声明为返回 `float*` 的虚方法；文档注释说明它将 Position/Color/Texture Coordinates 写入批次 vertex buffer 后返回该 buffer，以允许后续提交 shader-specific attributes。
- REPENTOGON `repentogon/Utils/ImageUtils.hpp` 的 `SubmitQuadForShader` 临时替换 `manager._shader`、image `_vertexFormat`、`_numVertexAttributes` 和 `_vertexFormatStride`，调用 `image.Render(...)` 后立即恢复字段并返回 vertex pointer。
- 随后的 `RenderFrameEx` 才调用 `ShaderUtils::ColorOffset::FillVertices` 或 `ColorOffsetChampion::FillVertices` 将 shader 属性写入同一 vertex pointer。Gold/Dogma 分支还使用 `g_Game->_frameCount` 或 `g_Manager->_framecount` 计算动态 progression。由此，`ImageBase::Render` entry/return 拿到的 buffer 还可能缺少最终 shader attributes；不能在那里立即冻结顶点。应在 `AnimationLayer::RenderFrame` 完成后复制该调用的 buffer，或关联 pointer 与 batch 并在同一线程下一次 Render 写入之前复制；同时记录 shader 与 uniforms/帧相位。对自定义 Mod shader、未知填充 helper 或无对应捕获路径，应 fail closed。
- `ImageBase::Render` hook 还看不到 Actor owner。最小安全结构是 Authority 正常原生渲染期间的线程局部 capture context（room revision、active room、pass/owner stack），只观察原始 call 一次；其内通过 `Entity::Render`/其他 Entity pass owner scope 排除实体绘制，或对已核定的 Backdrop/Grid/room utility draw pass 分别包装。hook 不得再主动调用原生渲染函数。

## 3. Room 渲染 hook 边界及排除 Entity

- 官方 `libzhl/functions/Room.zhl` **没有**声明 `Room::Render`。J273 IDA 函数 VA `0x804500`（RVA `0x404500`），体长 `0x3bf0`；开头以 `ECX` 作为 Room 指针。直接调用点包括 `Game::Render` 中 `0x6F7DF4`，以及 `0x8261DC`、`0x826A4F` 两个其他 Room 绘制路径。该函数 Hex-Rays 有 bad-stack-pointer 警告，且其调用树混合 Backdrop、Grid、Entity、light、timer/effect/overlay；不能 Client 调用，也不能直接把它当成“只绘 Room/Grid”的纯函数。Authority wrapper 还必须只接纳活动 Game Room 的正式主渲染调用，过滤预览/特殊 Room 绘制调用。
- 官方 REPENTOGON `ASMCallbacks.cpp:1929-1944` 对 `MC_POST_ROOM_RENDER_ENTITIES` 用 signature `33f68d87????????39b0????????76??8bbd????????6690` patch `Room::Render`。J273 上唯一命中地址为 VA `0x8061A8`（RVA `0x4061A8`）。该代码前部包含 GridEntity end-batch 循环（`0x806155` 一带调用 vtable `+0x24`），该 patch 点开始随后 Room EntityList 的虚调用循环（`0x8061A8` 后的 call vtable `+0x1C`）；官方 `Entity.zhl` 将 `Render`、`RenderShadowLayer`、`PostRender` 排在 vtable `+0x14/+0x18/+0x1C`。因此该 callback 是实体绘制阶段周边的“post entities”观察点，不是只包住 Room/Grid 的开界；它不能单独为 Room/Grid-only draw capture 提供 owner 排除。
- 推荐 capture owner 顺序：以正式活动 Room 主绘制为最外层；GridEntity::Render、Rock top、Door 附层、Backdrop floor/walls/water 等明确 visual owner 各自提供只观察 scope；Entity::Render/Shadow/PostRender 与 effect/NPC/player pass 进入 entity scope并丢弃。若不能识别自定义 Mod 绘制归属，应把完整发布标为不支持，避免误把实体当背景复制。

**关于原生 RNG/玩法**：Authority 观测必须调用原版原有 Room/Grid/Backdrop 绘制路径 exactly-once；capture 本身只读取参数/队列/纹理并复制字节。Client 消费纯值 quad/surface，绝不调用 `Room::Render`、`GridEntity::Render`、`Backdrop::Render*`、`Init/Load/Update`。需另外保留之前已发现的 `GetSourceQuad` 随机位分支会更新 J273 全局 RNG 事实；仅在 Authority 原有 Draw 中自然发生，不可在 Client 重演。PressurePlate/TimerPlate 读取 Game 时间并改 Sprite Color，Client 也只能接收已解析视觉状态/最终 draw 数据。

## 4. Backdrop 生成 surfaces 和资源创建

### 原生可读 surface

- 官方 `libzhl/functions/Backdrop.zhl`：`floorBuffer +0x0`、`wallsBuffer +0x8`、`floorSurface2 +0x10`、`tempBackdropSurface +0x18`，以及 `gfx +0x24`、`waterGfx +0x2C`、`nlfloorGfx +0x34`；另有 `floorANM2 +0x3C`、detail/control/button ANM2 在 `+0x4F3C/+0x5050/+0x5164`。`RenderFloor`、`RenderWalls`、`RenderWater`、`pre_render_walls` 是原生 pass。Room 另有 floor/wall tint、water amount/color/current、FX/rain/lava 等状态，surface 像素不会取代这些覆盖 pass。
- 官方 `repentogon/LuaInterfaces/LuaBackdrop.cpp:26-40` 的 `GetWallImage/GetFloorImage` 将 `wallsBuffer/floorBuffer` 直接放进新的 Lua `Image` smart pointer；这只是当前 Backdrop 内嵌缓存的拥有引用视图，不会构造 detached Backdrop、不运行初始化/生成，也不保证 cache 在 Room 切换后仍属于当前版本。应当在 Authority 原生生成后、Room teardown 或下次重绘前复制个别 image 的像素、大小和材质元信息。`GetFloorANM2` 等只是内嵌 ANM2 指针视图，不会包含全部生成 surface/水叠加。
- 原生缓存 target 可以通过图像 Render recorder 捕捉**具体目标 surface 的 draw quads**，条件是 recorder 同时记录当前 `KAGE_Graphics_Manager::_currentRenderTarget` 和目标变更，或在 Backdrop 的已知 target pass 中建立 scope。仅记录主屏 framebuffer 的 ImageBase quads会漏掉其先行离屏 cache pass；仅记录 fullscreen screenshot 则失去 room-local 几何/资源。本地官方 `Renderer.RenderToImage` 仅设置自建/游戏 surface 为 render target，执行 callback 并恢复 target，不是现成的游戏 Backdrop buffer导出/上传 API。

### 原图资源与内存像素上传

- 原版 Sprite 可用 `LayerState:GetSpritesheetPath()` 与当前 SpriteSheet `Image` 取得资源身份；若 Client 内容包/Mod 未提供相同资源，禁止重算 pose/用其它图替代。`GetTexelRegion` 读取底层 texture 对 paletted/indexed spritesheet 不一定是最终可见 RGBA：Image+0x18 的格式 0/1/2 对应 Red/RGB/RGBA，paletted 路径还有第二 texture/Palette。要么传正确的 palette texture + palette shader state，要么在 Authority 用同一原生 shader 将**单个资源/部件**烘成独立 RGBA surface；不能假设首张纹理读回就等于显示颜色。
- RGON `LuaRender.cpp:339-355` 的 Lua getter按 `width*height*4` 分配，J273 VA `0xA4C840`（RVA `0x64C840`）绑定该 Image 作为 target 后执行 `glReadPixels(..., GL_RGBA=0x1908, GL_UNSIGNED_BYTE=0x1401, dst)`。官方 Image.md 仍描述返回 32-bit floats；这与 Lua buffer 尺寸和机器码不一致，应以实际 RGBA8 byte 行为为准并将 Wiki 文案差异记为风险。
- `Renderer.CreateImage(width,height,name)` 在 `LuaRender.cpp:2863-2881` 使用 `KAGE_Graphics_ImageManager::CreateProceduralImage` 创建自有 Image，J273 `sub_A51640` (`VA 0xA51640`) 设 format 2/RGBA，建立 logical/padded 尺寸（至少 8 像素；是否 NPOT 由配置决定），并调用 `SetTexelData` 填初始色。它可提供客户端合法 texture/object 生命周期，但 Lua 没有把网络字节写进去的方法。
- 隐藏 vtable `ImagePlatformBase::SetTexelData` 在 `ImageBase` vtable `+0x90`，J273 VA `0xA4C3F0` / RVA `0x64C3F0`；机器码候选 ABI `char __thiscall(ImagePlatformBase* this, void** ownedPixelData)`。它按 `this+0x18` 选择 GL_RED/RGB/RGBA，`glGenTextures` 两次写入 `this+0x78/+0x7C`，调用 `glTexImage2D`，随后通过 REPENTOGON 绑定的引擎 allocator vtable slot 释放 `*ownedPixelData`。因此普通 vector/span 不能传入；对已经由 `CreateProceduralImage` 纹理化的 Image 再次调用还会覆盖 texture names 而先前 ID 已存在，存在泄漏风险。没有官方公开 setter 或其 ABI/allocator 契约。
- 更窄的 native helper建议：接收自有 procedural Image、RGBA8 的 padded 区域与行跨度；在 image+0x78 主 texture 已创建且同一渲染 HGLRC 上，先保存当前 `GL_TEXTURE_BINDING_2D`、`GL_UNPACK_ALIGNMENT/ROW_LENGTH/SKIP_*` 与 `GL_PIXEL_UNPACK_BUFFER_BINDING`，解绑 unpack PBO、设正确行状态，再用 `glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, paddedWidth, paddedHeight, GL_RGBA, GL_UNSIGNED_BYTE, bytes)` 上传，结束时恢复所有 GL 状态。图像逻辑尺寸与 padded 尺寸应分别存档；未覆盖的 padding 明确补透明。J273 `SetTexel` VA `0xA4C670` / RVA `0x64C670` 已证实 bind 的 primary texture name 正是 `this+0x78`，之后对单像素用同一个 `glTexSubImage2D`；`Unload` VA `0xA4C360` / RVA `0x64C360` 删除 `this+0x78` 与 `this+0x7C`，让正常 Image smartpointer 清理 GL 资源。
- 上述直接 GL 路径避开 `SetTexelData` 的引擎 allocator ownership，不创建磁盘 PNG，也不手动持有/销毁 texture name；但偏移、格式、GL context 状态均是 J273 私有实现，必须 version-guard，提交前 fail closed。不得在 draw batch 活跃中上传。目标是 RGBA8 surface；indexed/Paletted原图须另外复制 Palette/格式，或由 Authority 生成其实际效果后的独立 surface。

## 5. 实施落点与完整性条件

1. Authority 正常 Room draw exactly-once 时，按 room-generation 建立 draw event 序列；记录已完成的 quad/vertex 数据和对应原图键/texture bytes。利用 Grid/Backdrop owner 与 Entity 排除 scope，确保实体没有混入房间底图；同时保留 Grid/门/TimerPlate/Rock top 等特殊附层和前后遮挡 pass 的顺序。
2. Backdrop 渲染为多个实际 target 时，分别记录 `floorBuffer`、`wallsBuffer`、`floorSurface2`、`tempBackdropSurface` 与水/墙/地面的实际 draw，不以 backdrop ID/seed 在 Client 重新 `Init`。按 surface id+尺寸+内容 revision 更新自有 Client 图像；实际 floor/wall bytes 用安全点 `GetTexelRegion` 读取，Client 以 native 上传 helper写入 owned procedural Image。
3. Client scene只解码 immutable 值并调 draw-only 的自有 image/render primitive；用保存的 room-local transforms/camera、quad vertex attrs、shader/uniform、blend 与绘制次序。Client 严禁调用原生 Room/Grid/Backdrop render或操作任何原版对象；不支持的 shader、palette、water clip/overlay等应整候选原子拒绝或把部件标为缺失，不可静默退化为单Sprite。
4. 真实资源 E2E 应以原版有效房间/Backdrop/多个网格派生和原生生成的墙地缓存作为输入，确认 Authority 捕获值在 Client 自有 images 中逐资源重建且实景像素可见；不能只用受控模型、单张测试 PNG、synthetic RoomGrid 或 fullscreen 截图充作资源应用完成。

## 明确 UNKNOWN / 仍需原版 E2E

- `Room::Render 0x804500` 的完整调用树、未符号化子 pass、所有 early return/异步资源回调、Mod hook 与完整 catch范围仍未通过源码/静态数据完全闭合；捕获 scope的主调用 filtering需主工程据 hook 行为确认。
- `MC_POST_ROOM_RENDER_ENTITIES` 的 patch点及其周围虚调用可确认它不是 Room/Grid-only 开界，但仅由静态 vtable/loop仍不能把所有自定义 effect/Lua draw 分类。需要核定各 Entity draw owner hook覆盖和渲染重入行为。
- `Backdrop::pre_render_walls/RenderFloor/RenderWalls/RenderWater` 的每个目标 surface 何时完整、surface 生命周期何时退休、所有 overlay和 room-specific GLSL/lighting时序还需官方 J273 原生 pass进一步清单化；waterline/clip私有规则、reflection与某些 Grid WaterClip flags不能从公开 Sprite getters恢复。
- GL 多线程/Context 串行约束、pixel unpack全状态保存、CreateProceduralImage logical/padded覆盖语义以及 RGBA8 round-trip 需之后通过实际资源 E2E核验。本次没有运行任何测试/游戏/脚本，不能报告通过。
- 当前 `tx-04-18-implementation.md` 记载18此前由 controlled Authority model验收，明确没有闭合所有 Backdrop缓存与全部原版内容组合。此笔记提供 real-resource capture/upload方向，不能反向将旧测试改称真实原版资源 E2E。

## J273 唯一清单 hook 候选：RVA 与入口字节

IDA image base 为 `0x400000`。下表的 RVA 由 `VA - 0x400000` 得出；入口字节是本地 J273 IDB 的只读读取，给 hook 注册/签名复核提供本版本锚点。函数地址与字节不是跨版本 ABI 承诺。Room/Grid 派生类的 render/top 点沿用 `tx-04-18-reverse-notes.md`，此处只补主链必需入口。

| 入口 | VA / RVA | 本地原始入口前缀 (16 bytes) | 官方来源 / 意义 |
|---|---:|---|---|
| `ImageBase::Render(SourceQuad, DestinationQuad, 4 colors)` | `0x9E2170 / 0x5E2170` | `55 8B EC 83 EC 24 8B 45 08 56 8B F1 0F 10 00 0F` | `libzhl/functions/KAGE_Graphics_Image.zhl` 六色虚槽声明；批次 base vertex 写入点 |
| `Backdrop::RenderWater(offset)` | `0x91AC70 / 0x51AC70` | `53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04` | Full official `Backdrop.zhl` signature uniquely matches here; 16-byte prefix alone is shared with the mistaken `LoadConfig` candidate below. |
| `Backdrop::RenderWalls(offset, ColorMod)` | `0x9191F0 / 0x5191F0` | `55 8B EC 83 E4 F0 81 EC F8 00 00 00 56 8B 75 08` | `libzhl/functions/Backdrop.zhl` |
| `Backdrop::RenderFloor(offset, ColorMod)` | `0x918A60 / 0x518A60` | `55 8B EC 83 E4 F0 81 EC E8 00 00 00 0F 10 05 94` | `libzhl/functions/Backdrop.zhl` |
| `Room::RenderGridLight(grid, offset)` | `0x801BE0 / 0x401BE0` | `55 8B EC 83 EC 6C F3 0F 7E 05 B8 CE AA 00 53 56` | `libzhl/functions/Room.zhl` |
| `Room::RenderEntityLight(ent, offset)` | `0x7FC110 / 0x3FC110` | `53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04` | `libzhl/functions/Room.zhl` |
| `AnimationLayer::RenderFrame(pos, index, TL clamp, BR clamp, ANM2*)` | `0x404570 / 0x004570` | `53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04` | `libzhl/functions/AnimationLayer.zhl`; **actual RGON 1.1.3 implementation/hook** is `Anm2Extras.cpp::RenderFrameEx` |
| `Room::Render` candidate | `0x804500 / 0x404500` | `53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04` | No official ZHL Render declaration; IDA target/callsites only，bad-SP decompile，large mixed tree |

`ImageBase+0x18` 是 format 字段（`SetTexelData` 机器码按 0/1/2 选 GL_RED/RGB/RGBA）；`ImageBase+0x78/+0x7C` 是 `SetTexelData` 生成、`SetTexel` 主体使用、`Unload` 删除的两个 GL texture name。字段未在 `KAGE_Graphics_Image.zhl` 显式命名，属于 J273 binary layout。`GetTexelRegion` VA `0xA4C840` / RVA `0x64C840` 的安全读取点是原生 Room/render pass 已返回、CPU render batches 排空后、同一 HGLRC 与 image 仍有效时；它保存/恢复引擎当前 target，但会暂时绑定 image target、配置 framebuffer/viewport并读像素，不在任何活动 Render/Room hook内部调用。

### 地址订正记录：Backdrop::RenderWater

先前表格把 `0x90C3D0 / RVA 0x50C3D0` 误记为 `Backdrop::RenderWater`。该地址实际命中 `Backdrop::LoadConfig` 候选：IDA 函数 `sub_90C3D0` 大小 `0x1CAE`，含文件/配置解析相关调用（`atoi`、分配/释放、`__alloca_probe`），且它的函数被 `0x7DF8F0` 路径调用。原因是第一次使用的手工模式混入了 `LoadConfig` 特征尾部，并且它与 Water 的前 16 字节 prologue 相同；短前缀不能区分两函数。

纠正后，按官方 `libzhl/functions/Backdrop.zhl` 的完整签名 `538bdc83ec0883e4f883c404558b6b??896c24??8bec81eca8000000` 精确搜索，在 J273 IDB 唯一命中 `0x91AC70 / RVA 0x51AC70`。IDA 将其识别为函数 `sub_91AC70`，大小 `0x1892`，唯一代码调用者是 `sub_9191F0` (`Backdrop::RenderWalls`)；函数开头/体包含 bad-SP/SEH 栈恢复复杂性，优先信任官方 SDK 声明 `void Backdrop::RenderWater(Vector const& offset)` 与精确签名，不把 Hex-Rays 误析 ABI 当 C++ ABI。该地址的 16-byte raw prefix 也是 `53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04`。

本次按**完整官方模式**重核余下有 ZHL 签名的 hook 候选，唯一命中地址与表一致：`RenderWalls 0x9191F0`、`RenderFloor 0x918A60`、`Room::RenderGridLight 0x801BE0`、`Room::RenderEntityLight 0x7FC110`、`AnimationLayer::RenderFrame 0x404570`、六色 `ImageBase::Render 0x9E2170`。`Room::Render` 没有官方 ZHL 函数声明，保持 IDA `0x804500` 候选：J273 direct code xrefs 仍只有 `0x6F7DF4/0x8261DC/0x826A4F`，三处均在 call 前把 Room 指针放入 ECX；其体长 `0x3BF0` 且反编译 bad-SP。没有将该候选误称官方具名签名。

## J273 Entity owner 排除：已支持 RTTI 的渲染虚槽

`mod/binding/world_layout.json` 里的 Entity 范围为 `Entity` 根类和 `Player/NPC/Tear/Projectile/Pickup/Familiar/Effect` 七个 Entity schema 类。下表 vtable RVA 均直接取自此清单；VA 按目标 image base `0x400000` 计算。当前 8 张表均有非空的 `+0x14 Render`、`+0x18 RenderShadowLayer`、`+0x1C PostRender`，但具体 Render/Shadow 实现并不全共享。

| world_layout 类 | vtable VA / RVA | `+0x14 Render` VA / RVA | `+0x18 RenderShadowLayer` VA / RVA | `+0x1C PostRender` VA / RVA |
|---|---:|---:|---:|---:|
| Entity | `0xB18224 / 0x718224` | `0x6AC4E0 / 0x2AC4E0` | `0x6AD450 / 0x2AD450` | `0x6AC660 / 0x2AC660` |
| Player | `0xB1BDB4 / 0x71BDB4` | `0x784A60 / 0x384A60` | `0x7862A0 / 0x3862A0` | `0x6AC660 / 0x2AC660` |
| NPC | `0xB187D0 / 0x7187D0` | `0x6CEC90 / 0x2CEC90` | `0x6D0650 / 0x2D0650` | `0x6AC660 / 0x2AC660` |
| Tear | `0xB15FB8 / 0x715FB8` | `0x675AC0 / 0x275AC0` | `0x6AD450 / 0x2AD450` | `0x6AC660 / 0x2AC660` |
| Projectile | `0xB15B1C / 0x715B1C` | `0x659460 / 0x259460` | `0x6AD450 / 0x2AD450` | `0x6AC660 / 0x2AC660` |
| Pickup | `0xB18E8C / 0x718E8C` | `0x6E35D0 / 0x2E35D0` | `0x6AD450 / 0x2AD450` | `0x6AC660 / 0x2AC660` |
| Familiar | `0xB15090 / 0x715090` | `0x61E360 / 0x21E360` | `0x61E320 / 0x21E320` | `0x6AC660 / 0x2AC660` |
| Effect | `0xB14D00 / 0x714D00` | `0x600E60 / 0x200E60` | `0x6AD450 / 0x2AD450` | `0x6AC660 / 0x2AC660` |

### 唯一实现与本地入口前缀

下列 target 均为 IDA 中现存、非空函数；prefix 是各 VA 开头原始 16 字节。相同 target 表示同一函数体被多个支持 vtable 共享。父类 Render 也保留自己的地址：至少 Tear/Projectile/Pickup/NPC/Familiar 的 Render 函数调用链含有对它的直接调用，因此 Hook 需要按同 `this` 的 TLS 层级去重，避免派生函数内进入基类函数造成 owner 双推。

| 槽 / 所属 vtable | VA / RVA | 大小 | 原始前缀 16 bytes |
|---|---:|---:|---|
| Render — Entity | `0x6AC4E0 / 0x2AC4E0` | `0x17C` | `55 8B EC 83 E4 C0 83 EC 38 56 8B F1 57 80 BE 72` |
| Render — Player | `0x784A60 / 0x384A60` | `0x183D` | `55 8B EC 6A FF 68 20 5D A9 00 64 A1 00 00 00 00` |
| Render — NPC | `0x6CEC90 / 0x2CEC90` | `0x190E` | `55 8B EC 6A FF 68 16 18 A9 00 64 A1 00 00 00 00` |
| Render — Tear | `0x675AC0 / 0x275AC0` | `0x335` | `55 8B EC 83 E4 C0 81 EC B4 00 00 00 53 56 8B F1` |
| Render — Projectile | `0x659460 / 0x259460` | `0x389` | `55 8B EC 83 EC 2C 53 56 8B F1 57 80 BE 40 04 00` |
| Render — Pickup | `0x6E35D0 / 0x2E35D0` | `0x461` | `55 8B EC 83 E4 C0 83 EC 34 53 56 8B F1 57 80 BE` |
| Render — Familiar | `0x61E360 / 0x21E360` | `0x1369` | `55 8B EC 83 E4 C0 81 EC 34 01 00 00 A1 B4 83 B8` |
| Render — Effect | `0x600E60 / 0x200E60` | `0x3640` | `55 8B EC 81 EC B8 01 00 00 A1 B4 83 B8 00 33 C5` |
| Shadow — Entity/Tear/Projectile/Pickup/Effect | `0x6AD450 / 0x2AD450` | `0x20F` | `55 8B EC 83 E4 C0 81 EC B4 00 00 00 F3 0F 10 05` |
| Shadow — Player | `0x7862A0 / 0x3862A0` | `0x34` | `55 8B EC 83 E4 F8 51 56 6A 00 68 B1 01 00 00 8B` |
| Shadow — NPC | `0x6D0650 / 0x2D0650` | `0x52` | `55 8B EC 83 E4 F8 56 8B F1 57 8B 7D 08 8B 46 28` |
| Shadow — Familiar | `0x61E320 / 0x21E320` | `0x39` | `55 8B EC 56 8B F1 E8 65 DE FE FF 84 C0 74 1B 8B` |
| PostRender — 8 张表共享 | `0x6AC660 / 0x2AC660` | `0xDE3` | `55 8B EC 83 E4 F8 81 EC A0 00 00 00 56 57 8B F9` |

### ABI 与可观察边界

- 官方 `libzhl/functions/Entity.zhl` 的基类 vtable 顺序明确是 `Free, Init, PreUpdate, Update, Interpolate, Render, RenderShadowLayer, PostRender`，因此三个槽偏移分别为 `+0x14/+0x18/+0x1C`。该文件明文声明 `Entity::RenderShadowLayer(Vector*) -> bool`（顶部函数声明）；派生类 ZHL（`EntityPlayer/NPC/Tear/Projectile/Pickup/Familiar/Effect.zhl`）分别声明各自 `Render(Vector*) -> void`，与上述各自 vtable target 一致。基类 vtable 的 Render 项本身是 `skip`；IDA 的 `0x6AC4E0` 体与调用方将其作为 `void __thiscall(Entity*, Vector*)` 使用。
- `PostRender` 在官方 ZHL 只命名 vtable slot、没有参数/返回类型声明。J273 所有上述 8 张 vtable `+0x1C` 都是 `0x6AC660`；IDA 的机器码候选为 `int __thiscall(Entity*)`，无显式栈参数。Room Render VA `0x8061C8` 的调用点把对象放入 ECX，直接调用 `[vtable+0x1C]`，附近没有压 Vector 参数且不读取 EAX。故**不要**按 `void(Vector*)` 定义 PostRender hook；保守二进制 wrapper 接收 `this`，不取偏移，exactly-once 调原函数并把未解释的 EAX 原样返回。C++ 层真实 return type 仍 UNKNOWN。
- 可被称为“只读 hook”的范围只限于 wrapper 自己不调用额外游戏方法/不改对象，只在原生 Authority 路径中对原函数 exactly-once 前后观察并复制 owner 标记。Render/Shadow/PostRender 函数自身有渲染、访问 Room/Game/manager 与修改图形状态的行为，不是纯展示函数；Client 不得调用。基类 Render 可挂 observer，但仅 hook 它无法覆盖支持派生类的 override；Shadow 的四个唯一 target 和 8 个 Render target 都要覆盖，PostRender 则单一共用 target 足够覆盖清单 8 类。
- 上述列表不是所有游戏 Entity 类型的总表：例如官方 ZHL 还有 Entity_Bomb/Knife/Slot/Laser 类，当前 `world_layout.json` 未将它们列入 Entity 支持 schema。要从混合 `Room::Render` 流中严格排除未支持实体，仍需在通用 EntityList 的虚调用 dispatch 外围标注 owner，或识别具体 RTTI/vtable 后拒绝未分类对象；不能仅以“未知 ANM2 一律是 Entity”过滤，也不能将本表误写成完整 Entity type closure。

## 新增实景反例：FXLayers 直接 PNG 与房间环境 owner

主审提供的第 5 轮实景观测记录了一张直接 `ImageBase::Render` 的图：`resources/gfx/backdrop/8bitclouds_643.png`，Image 地址 `0x56C9F400`。同次给出的 `Room::_backdrop` 七个已知资源指针为 floor/walls/floor2/temp=`85E4C370/85E4B6F8/85E4CCF0/85E4C8C8`、`gfx=null`、`water=56C9F9F0`、`nlfloor=null`，因此该 Image 不匹配这七个已知 Backdrop 指针。以上 runtime 地址由主审提供；本节不声称独立复现。J273 IDA 静态字符串表没有 `8bitclouds_643.png`，不能据路径前缀直接断言 Backdrop 字段归属。

官方 `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Room.zhl` 将 `Room::_fxLayers` 放在 `Room+0x1618`；`FXLayers.zhl` 将 `_fxLayers` vector 放在 `+0x18`、`_rayGroups` 在 `+0x24`、`_pooCloudImage?` 注释候选在 `+0x3c`、veins 在 `+0xc0`，后续字段注释仍有 UNKNOWN。`F:/projects_isaac/IsaacDocs/docs/xml/fxlayers.md` 明确原生 fxLayer 的 `fx path` 和子项 `gfx path` 是 32-bit PNG，后者可按 roomshape 选择。这证明原生房间有 XML 配置的直接 PNG 环境层，不要求该图属于 ANM2；但本地 SDK/Wiki 没有包含第 5 轮实际使用的 XML 节点，故**不能确认 `_643.png` 对应的具体 fx/gfx 定义、选择 variant 或单一 FX entry 地址**。`+0x3c` 只是 SDK 的问号注释，仍不能据此称该图是 `_pooCloudImage`。

IDA 对 Room `Render` 候选 `sub_804500` 的直接 xref 给出以下实际 `this=Room+0x1618` FXLayers 环境调用点；顺序按 Room 函数中的调用地址：

| Room::Render 调用点 | callee VA / RVA | 静态行为 |
|---|---|---|
| `0x804963` | `sub_96CC00` / `0x56CC00` | `void __thiscall(FXLayers*)`，检查 `_loaded`，遍历 `+0x18` vector，每项 `0x4c` 字节，按 entry `+0x2c == 3` 筛选，再经 `sub_96C620(index)` 绘制。 |
| `0x805073` | `sub_96EA50` / `0x56EA50` | ECX 为 `FXLayers*`、无栈参，检查 `+0xc4` 后从 `+0xc0` 以 `0x34` 步长循环十个 vein 记录并调用 `sub_96EAB0`；与官方 `_veins[10]` / `_veinANM2` 注释对应，是单独 vein 环境分支。IDA Hex-Rays 显示 `int`，调用者不使用返回值，C++ 源级返回类型 UNKNOWN。 |
| `0x8059F2` | `sub_96CD10` / `0x56CD10` | `void __thiscall(FXLayers*)`，与 `sub_96CC00` 相同的 76-byte entry / `sub_96C620(index)` 路径，筛选 entry `+0x2c == 2`。 |
| `0x80685F` | `sub_96CE20` / `0x56CE20` | ECX-only 的 FXLayers 入口；遍历相同 vector，筛选 entry `+0x2c == 1` 并调用 `sub_96C620(index)`，另外还读写 poop-FX 和 ray 相关状态。IDA Hex-Rays 的 `__fastcall`/返回类型不能提升为官方 C++ 声明；Room 调用处不传栈参数且不消费 EAX。 |
| `0x80738D` | `sub_96E2B0` / `0x56E2B0` | `void __thiscall(FXLayers*)`，相同的 vector path，筛选 entry `+0x2c == 6` 后调用 `sub_96C620(index)`。 |

`sub_96C620` 从 FXLayers vector 的 `+0x18` 开始按 `0x4c * index` 取 entry，entry `+0x04` 是实际图像对象：它依次经 Image vtable `+0x28/+0x2c/+0x30/+0x34` 取 Width/Height/PaddedWidth/PaddedHeight，最后在 `0x96CBC9` 以 ECX=this、source quad、destination quad、KColor 调 vtable `+0x0c`。官方 `KAGE_Graphics_Image.zhl` 将 `+0x0c` 明确声明为 `ImageBase::Render(SourceQuad, DestinationQuad, KColor)->float*`。IDA xref 显示该 helper 唯一被 `sub_96CC00/96CD10/96CE20/96E2B0` 四个 type 3/2/1/6 pass 调用。因此只要实际 Image hook 的 owner stack 落在这四个已挂 scope 之一，Image 的直接 owner 已闭合为 `Room::_fxLayers._fxLayers[index]` entry `+0x04`，而非 ANM2 或 Backdrop 七个指针；确切 `_643.png` 属于哪一个 entry/type 仍要从该 draw hook 的 scope/index 记录或实际 XML 节点确认。若绘制没有发生在这些确切 FXLayers scope 中，保持 unknown，继续从实际持有字段/调用者追踪，不能把所有 Room root Image 按 `gfx/backdrop` 路径前缀兜底归类。

另一个容易混淆的近似资源不是同一张图：IDA 只有 `gfx/backdrop/8bitclouds_64_thick.png` 字符串（`0xB1D188`），由 `sub_7E08E0` 在 `0x7E123A` 传给 `sub_931B30`，结果放在全局 `dword_BE7320`。该 global resource 与主审提供的 `_643.png` 文件名不相同；没有静态依据把它作为 `_643` 的 owner 或替代图。实现分类时，kind8 是 Room environment 大类；只有新加的 FXLayers 子 scope 必须满足 `this == &activeRoom->_fxLayers`，而现有 Layer-derived kind8 仅适用于 `OriginalLayerScope` 已逐项验证的 Backdrop 四个 ANM2 或 Room 红门轮廓，不能放宽成所有 root/ANM draw。

## J273 内嵌三个 Image 默认 shader

官方 `KAGE_Graphics_Shader.zhl` 声明 `KAGE_Graphics_ShaderBase` 为 `0x28` 字节、`KAGE_Graphics_Shader` 总长 `0x2c`；字段为 vtable `+0`、`_initialized +4`、vertex descriptor pointer `+8`、descriptor count `+0xc`、GL program `+0x28`。官方 `KAGE_Graphics_Manager.zhl` 将 manager `_shader` 放在 `+0x18`。J273 IDA manager 基址为 `0xBAE190`，因此全局当前 shader `dword_BAE1A8` 位于 `+0x18`；三个内嵌 shader 对象分别在 `0xBAE1D8/0xBAE204/0xBAE230`，即 manager `+0x48/+0x74/+0xA0`，间距均为 `0x2c`。

IDA `sub_401F90`（VA `0x401F90` / RVA `0x1F90`）初始化这些静态对象的 vtable、`_initialized=false`、descriptor pointer/count 为零以及 GL program `-1`；仅看到对象地址不代表可绘制。正常图形启动块 `sub_A2AB60` 在 VA `0xA39496/0xA394BA/0xA394DE` 通过对象 vtable `+4` 调初始化，传入 descriptor 与两个 shader-source 字符串。三组 descriptor/source 是：

| manager 内嵌槽 | descriptor | startup shader 视觉语义 |
|---|---|---|
| `+0x48` | `off_BA47D0`：`Position(5), Color(6), terminator(0)`，2 个 attribute | vertex 输出 Position/Color；fragment 直接写 `gl_FragColor = Color0`，纯色/顶点色四边形。 |
| `+0x74` | `off_BA47B0`：`Position(5), Color(6), TexCoord(7), terminator(0)`，3 个 attribute | fragment 为 `texture2D(Texture0, TexCoord0) * Color0`，alpha 为零时 discard，标准单纹理 Image。 |
| `+0xA0` | 与 `+0x74` 同一 3-attribute descriptor | fragment 用 `Texture0` 读 palette index，再以 `Texture1` 和 `PaletteIndex.xy` 取色并乘 `Color0`，alpha 为零时 discard，双纹理 palette Image。 |

上述 descriptor 字节直接对应官方 `eVertexAttributeFormat`：`POSITION=5`、`COLOR=6`、`TEX_COORD=7`、终止符 `0`；名字分别是 `Position`、`Color`、`TexCoord`。`KAGE_Graphics_RenderBatch.zhl` 声明 RenderBatch 长 `0x3c`、Shader pointer 在 `+0x14`；IDA `sub_9E2EC0`/官方 `get_batch(bool)` 路径将当前 `dword_BAE1A8` 指针写入该字段，故 batch 保存的是进程内 `KAGE_Graphics_Shader*`，不是一个可网络传递的 program number。

IDA `sub_9E3220`（VA `0x9E3220` / RVA `0x5E3220`）从 Image 的 `+0x20` selector 选择：`0/1/2` 分别指向 manager `+0x48/+0x74/+0xA0`；`3` 跳过 embedded 选择并保留当前 manager shader。官方 `Global.zhl::ShaderType` 只有 AllShaders `0..19`、`SHADER_MAX=20`；官方 `LuaRender.cpp` 也仅在 `[0, SHADER_MAX)` 内将 GL program 映射到 `__ptr_g_AllShaders[i]`，且 GetShaderByType 对此范围之外拒绝。因此 **20..22 不是原生 ShaderType/native shader indices**；它们只能作为本协议私有的 `embedded-default-slot 0..2` 编号。`g_AllShaders` 是指针表，读取条目要解引用 `__ptr_g_AllShaders[i]`，不能将 `&g_AllShaders[i]` 当 Shader 对象地址。

静态结果支持 Client 使用自己进程中的三个内嵌对象重放捕获的 Image vertices，前提是当前正常 graphics startup 已完成且 Client HGLRC 有效：按私有 slot 取 manager 自有对象，验证 `_initialized`、descriptor/count 和 program 有效，再用该本地对象构造 batch。不要在 Client 调 `sub_A2AB60`、Shader `Initialize`、GL `LoadShader` 或用 Authority 的 `_glProgram` 值；静态 Shader 对象构造完成时处于未初始化状态，且 GL program ID 属于本地 context。context 重建、对象未初始化或 descriptor 不匹配时应拒绝此次表现候选。此结论只说明 default shader 选择/ABI 可作进程内映射，不代替对 Image render 的其他副作用（例如 Randomize 位触碰全局 RNG）单独清除。

## 新增实景反例：Water V2 surface 与 caustics

主审提供的后续实景观察在 Water 场景拒绝两种未知 root image：`Floor Water Surface (V2)` 一次，以及 `resources/gfx/backdrop/caustics.png` 八次。该观察不是本次静态检查独立复现；两者既不匹配 Room 的 Backdrop 七个图像指针，也不属于已确认的 FXLayers 四种 vector pass。下文只按明确的全局图像对象和调用路径识别，不按文件名前缀接纳其他 root image。

### 真实资源 owner 与生成方式

- J273 的 `sub_7E08E0`（VA `0x7E08E0`，RVA `0x3E08E0`）初始化这两个全局图像。`dword_BE7390` 在 `0x7E0A98` 附近由字符串 `gfx/backdrop/caustics.png` 经 `sub_931B30` 载入，随后通过 `sub_408110` 放入该全局 smart-image 槽；IDA 对 `dword_BE7390` 的代码 xref 还指向绘制 helper `sub_80C980`。
- `dword_BE7388` 不是 Backdrop 字段或源 PNG。初始化器在 `0x7E0E63` 使用内部名字 `Floor Water Surface (V2)`，传入 `0x30C`（780）和 `0x1D4`（468）调用 `sub_9EF4F0` 创建 procedural Image，再于 `0x7E0E8D` 经 `sub_408110` 保存到全局 smart-image 槽。静态证据将它识别为随后作为渲染目标使用的 V2 surface。

### 生成目标不可当作干净的 Room/Grid 纹理复制

- `sub_816250`（VA `0x816250`，RVA `0x416250`，长度 `0x94E`）的唯一 IDA 调用点是 `sub_803E20` 的 `0x8044E8`。调用处是 `mov ecx, edi; call sub_816250`，没有压入显式参数；被调用函数将 ECX 保存为 Room 指针。入口原始前缀为 `53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC`，含手写栈对齐/返回地址处理，IDA 的 `void __usercall(...@ecx,...@ebp)` 不应当提升为源码声明。可确认的调用契约是 ECX 接收 Room、调用处无显式栈参数；源码级 ABI 仍为 UNKNOWN。
- 函数在 `0x8162E9` 设置图形管理器 ECX，并将 `dword_BE7388` 与 `0` 压栈传给 `sub_9E80F0`，随后继续设置渲染状态。函数同时对每格对象走 vtable Render 路径，并调用以 Room `+0x1BC8` 为起始地址的 `sub_918A60`、`sub_9191F0` 路径；结尾还包含 vtable `+0x1C` 对象循环及图形缓冲切换路径。因此此生成 pass 混有 Room/Grid 与 Entity 类内容，不能将 `dword_BE7388` 的读回像素直接宣称为纯 Room/Grid 图层，也不能把该 surface 字节传给 Client 代替房间展示。
- 官方 SDK/Wiki 没有声明 `sub_816250`。若 Authority 观测它，应该把它当作 V2 目标的生成边界；在边界内只接受已确认的 GridEntity、Room Backdrop 与 FXLayers 环境 owner draw primitive，进入 Entity/未知 owner 时排除；任何应绘制的来源若未捕获则拒绝该 target。Client 只能在自有 780×468 target 上按目标标记重建这些纯值绘制，不能调用此函数或原生 Room/Grid 渲染。

### V2 surface 的实际消费 draw

- Room `sub_804500`（VA `0x804500`，RVA `0x404500`）在 `0x804ED3` 调 `sub_816BB0`；现场指令为 `mov ecx, edi; push &Vector; call`。IDA prototype 是 `int __thiscall(Room*, float*)`，但官方 ZHL 未给该 native helper 名称/声明。
- `sub_816BB0`（VA `0x816BB0`，RVA `0x416BB0`，长度 `0x59E`）选择专用 shader `off_AACADC`，按 Room 类型与位置数据构造 V2 surface 的 destination/source quad，再于 `0x817059` 对 `dword_BE7388` 调 `ImageBase::Render` vtable `+0x0C`。其后从返回的 vertex pointer `+0x28` 起继续写四个顶点的附加属性，涉及 Room `+0x7460/+0x7464` 与当前帧数据。因此 `ImageBase::Render` 的 entry/return 不是完整 vertex 结果；捕获必须延迟到这段后续属性写入完成，并保留专用 shader、Room/frame 相位与调用顺序。

### caustics 的 Room 绘制 helper 与 ABI 限制

- `sub_80C980`（VA `0x80C980`，RVA `0x40C980`，长度 `0x67D7`）只有两个直接调用点：Room 相关 helper `sub_803E20` 的 `0x8044B4`，以及 Room 主绘制 `sub_804500` 的 `0x80522E`。两处都将 Room 放进 ECX；分别压入 `1` 与 `0`。callee 末尾为 `retn 4`，证实它清理一个显式 32-bit 栈参数。IDA Hex-Rays 给出的 `char __fastcall(int,int,int)` 与调用现场不一致，且 EDX 未见在两个调用点明确设置；故只能记录 ECX=Room、一个栈参数 1/0，EDX 语义及源级返回类型保持 UNKNOWN。需要 hook 时保留 EDX/寄存器现场，不可据 decompiler prototype 写普通 `__fastcall` 包装器。
- `sub_80C980` 在 `0x80CA27`、`0x8123DE`、`0x8130CA` 等位置直接取 `dword_BE7390`；draw sites `0x81242C`、`0x81310A` 经该 Image 的 vtable `+0x0C` 调 `ImageBase::Render`。`0x81313C` 的循环条件明确至 8 次，与实景中观察到的八个 caustics draw 数量相符，但是否每个场景分支都由同一循环贡献，仍需按具体 scope/顺序确认。这个全局指针匹配才是 caustics owner 的静态锚点；不能将 helper 中任意 Image、任意 Room root draw 都归为水效果。

### 对接实现时的边界

- `Floor Water Surface (V2)` 是 Authority 生成的混合内容 target：不直接复制 target 像素给 Client。以 `sub_816250` 为上下文，只能记录已证明的 GridEntity、Backdrop 与 FXLayers 环境 primitive；Entity、未知对象和不透明后处理路径必须剔除或让该候选失败。随后用自有 target 复建，再按捕获的 `sub_816BB0` quad、四角属性、shader 与局部帧数据呈现；若此协议 target tag 仅定义 Grid 绘制，它就不等于原版 V2 surface 的完整生成内容。
- `caustics.png` 是明确的独立纹理资源；可按已识别的 `dword_BE7390` 映射纹理字节，并只在 `sub_80C980` 的 Room 调用上下文记录这个对象自身的 quads/最终 vertex 属性。纹理来源、目标 surface、blend/shader、动态相位与 draw 顺序必须同时闭合；名字或全屏截图不能替代这些字段。
- 尚未从官方 SDK/Wiki 找到 V2 target 或 `sub_80C980` 的源码级接口；`sub_80C980` 的 EDX 语义、V2 shader 的 client-local 初始化/索引、surface 生成 pass 中每条 Entity/后处理子路径的完整分类，以及实景八次绘制对应的具体分支仍为 UNKNOWN。此处结论来自 J273 IDA 静态数据库和主审给出的观察，未运行游戏或任何测试。

### 补充核对：Backdrop 与 EntityList 的实际区分

官方 `libzhl/functions/Room.zhl` 明确将 `_backdrop` 放在 `Room+0x1BC8`，将 `_entityList` 放在 `Room+0x1218`。因此 `sub_816250` 对 `Room+0x1BC8` 调用的 `sub_918A60`、`sub_9191F0` 是 Backdrop 路径，不能称为 EntityList helper。该 surface 混合 Entity 内容的直接证据在后段：IDA `0x8169E0` 取 `Room+0x1218`；`0x816A45` 读取其 count `+0xA0`，循环 `0x816A50..0x816A58` 从容器 `+0x54` 取对象并调用 vtable `+0x1C`。该槽是 Entity `PostRender`；这足以证明 target 不是只含 Room/Grid 的干净 surface。这里不据此推断 `sub_816250` 完成了所有 Entity 的常规 Render。

Target 组成还包含 FXLayers：`sub_816250` 在 `0x81671E` 调 `sub_96DF80(Room+0x1618)`；SDK Room.zhl 将该偏移定义为 `_fxLayers`。IDA 对 `sub_96DF80` 的反编译显示它检查 FXLayers `_loaded`，遍历 `+0x18` 的 76-byte layer vector，选 entry `+0x2C == 5`，并经 Image vtable `+0x0C` 提交 draw。因此，若 Client 的 V2 target 重建范围不包含该 FXLayers owner pass 及 Backdrop、GridEntity，就不能宣称与原版 target 等价；尚未分类或无法重建的环境来源应使目标表现候选失败。

## 补充静态核定：Room ColorModifier 的原版应用 pass

### 官方布局、接口与副作用边界

- 官方 SDK `F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/ColorModState.zhl` 定义 `ColorModState` 长 `0x18` 字节，六个连续 float：`r/g/b/a/brightness/contrast` 位于 `+0/+4/+8/+0xC/+0x10/+0x14`；默认值为 `(1,1,1,0,0,1)`。`LuaRender.cpp` 的 `ColorModDescriptor` 顺序为 `Position(vec3), Color(vec4), TexCoord(vec2), Amount(float), Brightness(float), Contrast(float)`。
- SDK `Room.zhl` 声明 `__thiscall ColorModState Room::ComputeColorModifier()`；`Game.zhl` 声明 `__thiscall void Game::SetColorModifier(ColorModState*, bool lerp, float rate<xmm3>)`，并标出 `_currentColorModifier=Game+0x66920`、`_targetColorModifier=+0x66938`、`_lerpColorModifier=+0x66950`、`_shouldLerpColorModState=+0x6691C`。REPENTOGON `Game.md` 的 `GetCurrentColorModifier()` 明确读取当前正在使用的 raw 值；`FXParams.md` 则说明 `GetFXParams().ColorModifier` 是房间 fxlayers.xml 对应值，不等同于 Game 当前值。
- `LuaRoom.cpp::Lua_RoomColorModifierUpdate`（约 297–318 行）在 `process=true` 时调用 `room->ComputeColorModifier()`，否则从 `FXParams.roomColor/brightness/contrast` 构造六值，随后两种分支都会调用 `g_Game->SetColorModifier(...)`。因此该 Lua helper 并非 draw-only；Client 不得通过它更新全局 Game 状态。
- J273 IDA `Room::ComputeColorModifier` 为 VA `0x818540` / RVA `0x418540`，与 SDK pattern `558becf30f101d` 匹配。按 x86 聚合返回 ABI，隐藏输出指针由调用者提供；反编译函数仅写输出 buffer，直接复制 Room `+0x1B10/+0x1B14/+0x1B18/+0x1B1C/+0x1B24/+0x1B28` 六值，然后按 Room 类型 7/8/29 或 challenge/Game 标志把结果 75% 混向 neutral `(1,1,1,0,0,1)`；Game flags `+0x44 & 0x200` 时 brightness 加 `0.03`；Room 类型 32 且 `Game flags & 0x20` 时按房间格数、`Room+0x7740` lava intensity 调整 RGB/A/contrast。此函数有 Game/Room 读取，不直接改其状态；Client 仍不应调用它，因为结果不是 Game 当前插值值。

### J273 `Game::Render` 中的实际屏幕 pass

- 官方 `Game.zhl` 的 `Game::Render()` pattern 在 J273 匹配 VA `0x6F7A80` / RVA `0x2F7A80`。IDA 反编译有 bad-SP 警告，故以下 ABI 仅以 SDK 声明为准，具体 pass 数据流由指令与地址确认。该函数在 VA `0x6F7DF4` 调 Room `sub_804500` 后，进入由 `v113` 控制的后续图像 pass；pass 通过 manager 当前 shader 槽 `dword_BAE1A8` 选择 `0xBAB2EC`，随后调用 `dword_BE72E4` 的 Image vtable `+0x0C`（`ImageBase::Render(SourceQuad, DestinationQuad, KColor)`）。
- `0xBAB2EC` 对应有序全局 shader pointer table index 9；官方 `ShaderType.md` 将 index 9 命名为 `SHADER_COLOR_MOD`。Lua `Renderer.GetShaderByType()` 实际取 `__ptr_g_AllShaders[shaderType]`，而不是取 `&__ptr_g_AllShaders[index]`。Lua API/源代码也提供 `Renderer.RenderToImage`、`Renderer.Pipeline():NewPass(...)` 和可任意设置属性的 `CustomVertexBuffer`；未找到一个只接收 ColorModState 并自动填完该 shader 顶点的现成 helper。
- IDA 代码在 `0x6F7F76/0x6F7F7E/0x6F7F90` 从 `Game+0x66924/+0x66928/+0x66920` 读取当前 G/B/R，组成 Image `KColor={R,G,B,1}`；并构造 0..1 的 source/destination quad 调用 `dword_BE72E4->Render`（调用点 `0x6F803C`）。所以实际后处理颜色来自 `_currentColorModifier`，不是直接读取 Room `FXParams` 或再次执行 `ComputeColorModifier`。
- `ImageBase::Render` 返回的 `float*` 后，原版 Game::Render 还会在四个顶点补三项 ColorMod 自定义 attribute：对 `float*` 偏移 `+9/+10/+11`、`+21/+22/+23`、`+33/+34/+35`、`+45/+46/+47` 分别写 `Game+0x6692C` (`A`)、`+0x66930` (`Brightness`)、`+0x66934` (`Contrast`)；每顶点 12 个 float，即 48-byte stride。这个“Image::Render 返回后仍改顶点”的事实意味着单独记录 Image 调用的标准 position/color/texcoord 不足以复现 ColorMod pass。
- REPENTOGON `ColorModifier.md` 说明 A 是强度乘数，必须非零 RGB 才生效。因此主审给出的 Burning Basement 值 `(R=-1,G=.1,B=.5,A=-.2,Contrast=1.05)` 中 A 非零，即使为负也不能当成 disabled；Brightness 未包含在该观察值里，不能自行补造。该观察值若来自 FXParams，不足以证明与这一帧 Game current 完全相同；若来自 Game current 六值则可用于该 pass。

### 最小安全复用建议与 UNKNOWN

- 对当前工作树做只读源码回看：`mod/src/game/room_grid_original.cpp::OriginalRoomGridRenderReturned` 已把 `g_Game->GetCurrentColorModifier()` 六值写入 `color_modifier`；`mod/src/replica/room_grid_original.cpp` 将该字段传入 descriptor；`mod/src/game/room_grid_original.cpp::PrepareOriginalRoomGraphics` 检查本地 `SHADER_COLOR_MOD` 和 12-float stride，`DrawOriginalRoomGraphics` 在本方 composite 之后构造四个 12-float 顶点并 `SubmitNative`/复制顶点。字段次序与本节核定的原版 `ImageBase::Render` 后补属性相符，因此在当前 original-render Client 路径里，ColorModifier 已由 draw-only pass 消费，不再只是保存在模型中。当前 Client pass 未观察到 `v113` 的图形选项/RoomTransition 判定，故与“禁用颜色修正”选项及特殊 `_mode==2 && _unkStartRoomTransitionCond!=0` 情形可能不一致。此为静态代码核对，不代表已运行或验证渲染结果。
- Authority 从原版可读地取得**实际当前**六值（与 `Game.GetCurrentColorModifier` 语义一致；包括 Game 插值及其他 modifier 的效果），并把它和本帧 Room presentation 候选原子化发布；为匹配原版还需传 `color_mod_pass_enabled` 或等价决策位，其值来自 `_enableColorModifier` 与上述 RoomTransition 例外，不能从 ColorModState 是否 identity 推导。Client 不调用 `Game::Render`、`Room::ComputeColorModifier`、`Room::UpdateColorModifier` 或 `Game::SetColorModifier`。
- Client 完成自有 Room 图像后，才对自有图像/目标执行 draw-only `SHADER_COLOR_MOD` pass：四角按 `ColorModDescriptor` 填 Position、Color=`{R,G,B,1}`、TexCoord、Amount=`A`、Brightness、Contrast；把其输出接回自有 render target。可以使用 REPENTOGON 的 shader index 9 和 CustomVertexBuffer/自有 Pipeline，或在完整隔离的原生 render batch 中沿 `ImageBase::Render` 后补全四顶点 attribute 的原版路径；后者必须在正确 GL context/thread 和 batch 边界操作，并恢复 manager shader/render-target 状态。两种路径都不触发 Room/Grid/Entity 玩法方法，也不消费游戏 RNG。
- `v113` 的开关条件现已可由 J273 指令和官方布局化简：`v113 = g_Manager->_optionsConfig._enableColorModifier && !(roomTransition._mode == 2 && roomTransition._unkStartRoomTransitionCond != 0)`。IDA 里 `n4` 是全局 `g_Manager`（VA `0xBE4A1C` / RVA `0x7E4A1C`）；`Manager.zhl` 的 `_optionsConfig` 为 `Manager+0x2A178`，`OptionsConfig.zhl` 的 `_enableColorModifier` 为 `OptionsConfig+0x8D`，即 `g_Manager+0x2A205`。`Game+0x1B83C` 是内嵌 `RoomTransition`，其 `_mode` 位于 `+0`；`Game+0x1BA74` 对应 `_unkStartRoomTransitionCond`（RoomTransition `+0x238`）。IDA 的 `mode==2 || mode==3` 子句之后仍要求 `mode==2`，所以 mode 3 不会仅因该分支关闭 ColorMod。该条件不检查 modifier 是否 identity；稳定房间中由用户图形选项 `_enableColorModifier` 控制。`_unkStartRoomTransitionCond` 的确切业务含义仍 UNKNOWN。
- `dword_BE72E4` 的具体对象身份、其当帧像素是否严格只含 Room/是否含其他全屏合成内容及所有 rendering mode 对此 pass 的完整语义，官方 SDK 未声明；当前只能由 IDA 确认它在 Room::Render 调用之后、用 ImageBase 做 0..1 屏幕矩形 pass。若 Client 的输入目标与原版该 texture 的合成范围不匹配，颜色结果仍不能宣称与原版整帧一致。
- 官方 Lua helper 能通过 `Renderer.GetShaderByType(SHADER_COLOR_MOD)` 取 shader，并通过 descriptor/CustomVertexBuffer 填字段；是否可在当前 C++ hook 生命周期内直接复用该 Pipeline 对象、无需额外原生全局 graphics-state 操作，SDK 未说明。Native renderer 的 manager/batch 具体恢复序列应沿 J273 原始 render API 与现有自有 target 生命周期核定，不能只写 `dword_BAE1A8` 后假设状态已复原。

本节仅依据官方 REPENTOGON 1.1.3 本地 SDK/Wiki 源码与 J273 IDA 静态数据库补记；Burning Basement 数值为主审提供的实景事实，本代理未独立运行或测量。未运行游戏、测试或校验脚本。

## 再补静态核对：ImageBase Render 重载共同核心与 Room 深度 pass

### ImageBase 的八个 Render 槽均收敛至 J273 `0x9E2460`

- 官方 `libzhl/functions/KAGE_Graphics_Image.zhl` 将六色重载声明为 `float* Render(const SourceQuad&, const DestinationQuad&, const KColor& tl, const KColor& tr, const KColor& bl, const KColor& br)`，并说明返回 batch vertex buffer，供后续 shader-specific attributes 使用。J273 对应入口 `0x9E2170 / RVA 0x5E2170`，函数开头复制/对齐 SourceQuad，随后直接调用共享体 `sub_9E2460`。
- `sub_9E2460` 位于 VA `0x9E2460 / RVA 0x5E2460`，体长 `0x8BC`，入口字节 `55 8B EC 83 E4 F0 81 EC 08 01 00 00 80 3D B1 E1 BA 00`。IDA 伪代码将 `this` 错认成 float，但入口 `mov esi,ecx` 后以 ECX 对应对象的 `+0x10/+0x18/+0x24/...` Image 字段访问，符合 `ImageBase* this`。其实际调用 ABI 为 `__thiscall`，ECX=`ImageBase*`，栈上传 SourceQuad、DestinationQuad、四个 KColor 引用（六个 32-bit 参数）；尾部 `retn 18h` 与 24-byte 栈清理相符。正常路径 EAX 返回 `sub_9E19C0(4)` 取得的 vertex buffer 首地址；被裁剪路径返回 0。返回语义与官方 `float* Render(...)` 一致。
- IDA 对 `0x9E2460` 的四个直接代码 xref 为 `0x9E1F65`（调用者 `0x9E1EA0`）、`0x9E1FC6`（`0x9E1FA0`）、`0x9E2139`（`0x9E2000`）、`0x9E21B5`（`0x9E2170`）。前 3 个直接-core overload 与六色 overload 均受共同入口覆盖。IDA Image vtable（样本 `0xB3B8EC` 起）八个 Render 槽进一步核对如下：

| Vtable 槽 | 实现 | 到共同 core 的路径 |
|---|---|---|
| `+0x08` | `0x9E2170` | 直接调用 `0x9E2460`；六色入口 |
| `+0x0C` | `0x9E2150` | 将单色复制为四角色，虚调用 `+0x08`，再到 core |
| `+0x10` | `0x9E2000` | 直接调用 core；依据输入 SourceQuad/位置构造 DestinationQuad |
| `+0x14` | `0x9E1FE0` | 单色复制四份，虚调用 `+0x10`，再到 core |
| `+0x18` | `0x9E1FA0` | 直接调用 core；建立默认 SourceQuad，输入 DestinationQuad 与四角色 |
| `+0x1C` | `0x9E1F80` | 四角复制单色，虚调用 `+0x18`，再到 core |
| `+0x20` | `0x9E1EA0` | 直接调用 core；由 Image 尺寸和位置构造目的矩形 |
| `+0x24` | `0x9E1E80` | 四角复制单色，虚调用 `+0x20`，再到 core |

- 因而 hook `0x9E2170` 会漏掉 `0x9E1FA0/0x9E2000/0x9E1EA0` 三种直接调用形态；改在 `0x9E2460` 做一次观察即可覆盖这八个 ImageBase Render 槽。现有 `OriginalImageBefore -> original exactly-once -> OriginalImageReturned(this, result)` 延迟复制模式可沿用：core 入口前先 flush 上一个 pending vertex pointer，core 返回后保存本次 Image 与 EAX pointer；在 Layer 完成填 shader attributes 或下一 Image core entry 前不读当前指针。对这四个直接 caller，反编译都在 core 返回后立即返回；没有看到 caller 再写该 vertex buffer 的证据。跨到 `AnimationLayer::RenderFrame` 等更上层的最终 shader attribute 写入仍须保持延迟机制。
- `0x9E1FA0` 是实际存在的 destination-quad + 四角 KColor Render 重载，不是 one-color 重载。现有静态资料没有具体 FX/cloud caller 地址指向它；故只能确认单一 `0x9E2460` hook 会覆盖此重载，不能据此声称它就是缺失暗云影的实际 caller。非 vtable Render 接口（如单独 `RenderTexturedTriangle`）不在“八个 Render 槽全部收敛”结论内。
- 当前工作树 `mod/binding/hooks.json` 已将 `original_image` RVA 改为 `0x5E2460` 并用六色参数签名；这是只读源码回看，不表示 hook 运行正确或实景已通过。

### `Room::Render` 中可确认的 Present/深度边界

- J273 `sub_9E4170`（VA `0x9E4170 / RVA 0x5E4170`）被 Room Render 直接调用五次。其指令先清 manager `+0x18` 状态、调用 `sub_9EFA70`/`sub_9EFA10`，再经 manager vtable `+0x68` 完成提交；当 manager `+0x1C` 当前 render target 非空时，精确执行 `push 0x100; call epoxy_glClear`。`0x100` 是 `GL_DEPTH_BUFFER_BIT`，因此 offscreen target 上的 Present 会清深度缓冲，而不是只做 CPU 队列标记；官方 `KAGE_Graphics_Manager.zhl` 将 current render target 放在 `+0x1C`。无当前 target 时，函数走 window swap/另一 present 路径，不执行这次 depth clear。该私有 helper 未在官方 SDK 给出具名函数声明。
- Room `sub_804500` 的静态顺序把这些 Present 夹在具体的 Backdrop/FX pass 之间：`0x804963` 调 FXLayers type 3；`0x80501D` 调 `Backdrop::RenderFloor`；`0x805073` 调 veins；`0x8051A0` 调 `Backdrop::RenderWalls`；之后 `0x8054A2` Present/清 depth。随后 `0x8059F2` 调 FXLayers type 2，`0x805A09` Present；`0x80685F` 调 FXLayers type 1，`0x8069C2` Present；`0x80738D` 调 FXLayers type 6，`0x8073B6` Present。另有条件 Present `0x8078DF`（受 Room 内 byte `[ebp-0x171]` 控制）；该条件所代表的完整视觉子pass仍 UNKNOWN。
- 这些静态指令证明原版 Room 会在多组实际 Backdrop/FX 绘制之间 flush 并清深度。当前 pure-value 记录含 draw/target/shader/vertices，却不含同一 target 内的这些 depth-clear 边界；若 Client 把 floor/wall 与后续 FX/cloud draw 全堆在一个持续 depth buffer 中，前序 depth 值可能让后续绘制结果不同。主审提供的实景云部件 vertex z=`-1.18`、floor z=`-1.0` 与这个风险相容，但尚未静态/运行时确认该云部件属于上述哪一个 FXLayers type/Room pass、或确有某次原生深度测试失败。因此它是具体待核假设，不是已证明的云影缺失根因；无需据此增加通用 GL hook。

本补充仅以官方 REPENTOGON 1.1.3 ZHL 与 J273 IDA 的函数/xref/指令静态核对为依据；没有运行测试或游戏。
