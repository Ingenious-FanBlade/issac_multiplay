# RE-1：J273 入口、签名与 ABI

返回：[成果总览](README.md)。地址基准：imagebase `0x400000`，`RVA = VA - 0x400000`。

当前只读 IDB 的模块名为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，数据库名同模块加 `.i64`。版本适用范围沿用本项目 J273 目标；本次没有校验哈希，没有附着调试器、修改 IDB、补丁或目标内存。

`SOURCE` 表示本地 SDK／官方源码；`J273_STATIC` 表示本次原始指令、静态签名匹配与调用关系；`DESIGN` 表示后续调用安排；没有 `RUNTIME` 结论。反编译器在栈对齐／EH 函数中显示的 `__usercall`、额外 EBP／EDI 参数和偶然返回值不作为 ABI。

## 1. 入口表

下表的 `void`／`__thiscall` 首先取 SDK，再以调用点的 ECX、压栈与 `retn` 核对。x86 栈参数各占 4 字节，bool 的有效值取低字节。

| 入口 | VA／RVA | 原型与 this | 原始调用／返回证据 |
| --- | --- | --- | --- |
| Game::Update | `0x6F6D30`／`0x2F6D30` | `void __thiscall(Game*)`；整体拦截 | Manager `0x92E911` 调用；本任务以其内部调用顺序为主线 |
| HUD::Update | `0x97ACF0`／`0x57ACF0` | `void __thiscall(HUD*)`；`Game+0x1D9AC` | `0x6F6D81`；`0x97AE80 retn` |
| HUD::PostUpdate | `0x97AE90`／`0x57AE90` | `void __thiscall(HUD*)` | 普通尾 `0x6F79B9`、过渡尾 `0x6F7436`；`0x97AF1D retn` |
| PlayerHUD::Update | `0x832C10`／`0x432C10` | `void __thiscall(PlayerHUD*)`；HUD 八槽，每槽 `0x6E0` | `0x97AEAA`；正常尾 `0x8347D9 retn`；入口有 `alloca(0x10D0)` |
| Minimap::Update | `0x965DB0`／`0x565DB0` | `void __thiscall(Minimap*)`；`Game+0x25444` | `0x6F77B0`、`0x6F742B`；`0x966148 retn` |
| Minimap::Refresh | `0x965D60`／`0x565D60` | `void __thiscall(Minimap*)` | `0x965D6D`／`0x965D78` 调两份缓存；`0x965DA3 retn` |
| 地图缓存构建（本报告描述名） | `0x9635B0`／`0x5635B0` | `void __thiscall(MinimapConfig*)`；无公开 SDK 名；不消费额外栈参数 | `0x739B3E`／`0x739B49`、`0x965D6D`／`0x965D78`；`0x9655C4 retn` |
| EntityList::render_sort | `0x4150C0`／`0x0150C0` | `void __thiscall(EntityList*)`；`Room+0x1218` | EntityList Update `0x415023`；`0x4152C3`／`0x415345 retn` |
| Camera::UpdateUltrasmooth | `0x91D840`／`0x51D840` | `void __thiscall(Camera*, bool halfFrame)` | `0x92E98E push parity`、`0x92E998 call`；共用正常尾 `0x91D9AD retn 4` |
| Camera::UpdateDrag2 | `0x91E5E0`／`0x51E5E0` | `void __thiscall(Camera*)` | `0x92E980`；`0x91E91D retn`。方案中的 UpdateDrag 对应 SDK 的 **UpdateDrag2** |
| RoomTransition::Update | `0x826430`／`0x426430` | `void __thiscall(RoomTransition*)`；`Game+0x1B83C` | mode 非零时 `0x6F7420`；`0x82731F retn`；混合入口，RE-9 负责其子路径 |
| 六分量颜色趋近（本报告描述名） | `0x6EB990`／`0x2EB990` | `ColorModState* __thiscall(ColorModState* current, const ColorModState* target, const ColorModState* step)`；返回 target | `0x6F6E09`；ECX=`Game+0x66920`，先 push `+0x66950` 再 push `+0x66938`；各尾 `retn 8` |

表中两项描述名不是给 IDB 新增的符号。调用六分量 helper 时也不能把当前 IDA 推导出的指针返回值当作功能必须使用的结果；其用途是原位更新 current。

## 2. SDK 定位模式

以下为 SDK 中的原始模式（`??` 为不固定字节）。本次在当前 IDB 的静态搜索中各唯一匹配至入口表对应地址；这是地址定位记录，不是哈希或推进条件。

| SDK 文件／声明 | 模式 |
| --- | --- |
| `HUD.zhl`：Update | `558bec5153568bf15780be????????0074` |
| `HUD.zhl`：PostUpdate | `558bec83e4f85153568bd9` |
| `PlayerHUD.zhl`：Update | `538bdc83ec0883e4f883c404558b6b??896c24??8bec6aff68????????64a1????????5053b8d0100000` |
| `Minimap.zhl`：Update | `558bec83e4f883ec148b15` |
| `Minimap.zhl`：Refresh | `558bec83e4f851568bf18d4e` |
| `Game.zhl`：UpdateVisibility（排除入口） | `558bec83e4f8515356578bf96aff` |
| `EntityList.zhl`：render_sort | `558bec83ec14833d????????00` |
| `Camera.zhl`：UpdateDrag2 | `558bec83ec148d` |
| `Camera.zhl`：UpdateUltrasmooth | `538bdc83ec0883e4f883c404558b6b??896c24??8bec6aff68????????64a1????????505383ec485657a1????????33c5508d45??64a3????????894d??8b3d` |
| `RoomTransition.zhl`：Update | `558bec6aff68????????64a1????????5083ec60a1????????33c58945??5657508d45??64a3????????8bf9` |

SDK 没有给地图缓存子函数命名，通过两组 caller 的直接 `call` 目标定位。其短对齐前缀曾同时匹配三个函数，不能单独用来绑定；扩展后的下列模式在当前 IDB 唯一命中 `0x9635B0`：

```text
53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04
89 6C 24 04 8B EC 81 EC D0 00 00 00 A1 ?? ?? ?? ??
33 C5 89 45 FC A1 ?? ?? ?? ?? 56 8B F1 C6 05 ?? ?? ?? ??
00 57 8D 48 14
```

六分量 helper 的以下模式也在当前 IDB 唯一命中 `0x6EB990`：

```text
55 8B EC 8B 45 08 F3 0F 10 11 8B 55 0C 0F 28 C2
F3 0F 10 25 ?? ?? ?? ?? F3 0F 10 08 F3 0F 5C C1
```

## 3. 对象与字段

| owner | 已核字段 | 消费方／用途 |
| --- | --- | --- |
| Game | `+0x1D9AC HUD`、`+0x25444 Minimap`、`+0x1B83C RoomTransition`、`+0x18300 Room*` | SDK Game.zhl 与原生 ECX 形成相同关系 |
| Minimap | `+0x18`、`+0x178` 两个 `MinimapConfig`，stride `0x160` | 分别为小图／大图缓存，即 Game `+0x2545C`／`+0x255BC`；**不是 J460 的 +0x25EE4／+0x26044** |
| MinimapConfig | `+0x15C` 缓存索引／资源选择值，SDK 未具名 | `0x9635F5` 读取，随后与 `0x500` 合成缓存相关值；不能因 SDK 只声明到 alpha `+0x158` 而忽略此字段 |
| Room | `+0x11F8 Camera*`、`+0x1204/+0x1208 render scroll` | Manager 获取 Camera；两个 Camera 更新写后两项 |
| Camera | `+0x00/+0x10/+0x20/+0x30/+0x40/+0x50` 六组本地环形缓冲头 | 已构造的本地分配／容量／计数／头索引；不复制 Host 地址，不用逐帧 Snap 重置代替平滑 |
| Camera | `+0x60/+0x64` 玩家中心缓存，`+0x68 Room*`，`+0x6C/+0x70` 滚动结果 | Ultrasmooth 末尾 `0x91E5A6—0x91E5CA`；其他几何／视口字段仍保留本地构造关系 |
| Camera | `+0x8C bool shouldOverride`；`+0x90` Drag 平滑系数 | SDK ShouldOverride、LuaCamera 的 Snap 包装与 raw 相符；`+0x90` 在 Drag 中调节约 `0.1—0.5` |
| Manager | `+0x4A8B8 framecount`；`+0x2A178 OptionsConfig` | 相机用本地 ManagerFrame 奇偶；不借用 Host ManagerFrame |
| OptionsConfig | `+0x44 cameraStyle`、`+0x40 FoundHUD`、`+0x48 HistoryHUD` | 对应 Manager `+0x2A1BC`、`+0x2A1B8`、`+0x2A1C0`；本机显示选项 |

## 4. 优先来源

- 原版 Wiki：[HUD](F:/projects_isaac/IsaacDocs/docs/HUD.md)、[Level.UpdateVisibility](F:/projects_isaac/IsaacDocs/docs/Level.md#updatevisibility)、[Game](F:/projects_isaac/IsaacDocs/docs/Game.md)、[ButtonAction](F:/projects_isaac/IsaacDocs/docs/enums/ButtonAction.md)。Wiki 明确说明地图可见性有缓存、变更后须刷新；FadeoutTarget 包含菜单／新局等目标。
- REPENTOGON Wiki：[Camera](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Camera.md)、[Minimap](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Minimap.md)、[PlayerHUD](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/PlayerHUD.md)、[CameraStyle](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/enums/CameraStyle.md)。`Camera:Update` 的公开 bool 没有解释半帧语义；本报告以原生 Manager caller 补证。
- SDK 根：[libzhl/functions](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions)，已读 Game、HUD、HUD_Message、PlayerHUD、Minimap、MinimapConfig、Camera、OptionsConfig、Manager、EntityList、RoomTransition、ColorModState 等 `.zhl`。
- 官方实现：[ASMCallbacks.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ASMPatches/ASMCallbacks.cpp:71)、[EntityManager.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/EntityManager.cpp:94)、[CustomCallbacks.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/CustomCallbacks.cpp:188)、[LuaCamera.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/LuaCamera.cpp:15)、[ASMCamera.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ASMPatches/ASMCamera.cpp)。
- 项目既有事实：[维护与绘制第 3 节](../../../tx-04-17/tx-04-17-native-maintenance-rendering.md#3-本轮-j273-静态事实)、[现有地图原生适配](../../../../../../mod/src/game/client/room/client_map_native.cpp)、[现有实体应用／退休](../../../../../../mod/src/game/client/entity/client_objects_native.cpp)。历史“不放行显示 Lua／相机”等设计决定已由本次方案取代，原始静态事实继续保留。
