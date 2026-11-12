# RE-2：入口、ABI 与资料依据

返回：[成果总览](README.md)。下列地址均为 J273，imagebase `0x400000`；RVA=VA−`0x400000`。当前 IDB 模块为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，数据库为同名 `.i64`，沿用 RE-1 会话，没有切换数据库、附着调试器、设断点或改补丁。

`SOURCE` 为本地 Wiki／SDK／官方源码，`J273_STATIC` 为本次指令／反编译／调用关系，`REUSED` 为项目已有事实，`DESIGN` 为后续接线建议。没有运行结论。不校验任何哈希。

## 1. 根入口与参数纠正

| 入口 | VA／RVA | ABI 与核定 |
| --- | --- | --- |
| Room::Update | `0x7F7F60`／`0x3F7F60` | `void __thiscall(Room*)`；原版 Wiki 公开 Update；当前 Room.zhl 未给此独立声明。Game `0x6F78DF` 以 Room 为 ECX 调用；`0x7FBD81 retn` |
| Room::Init | `0x7E8AD0`／`0x3E8AD0` | SDK `void __thiscall Room::Init(int param_1, RoomDescriptor*)`；正常尾 `0x7ED8AC retn 8`。第一栈 word 的二进制用途是 RoomConfig_Room 地址，第二为 descriptor |
| Room 清理／重置（描述名） | `0x7E1550`／`0x3E1550` | ECX=Room，无显式栈参数；Init 内 `0x7E8D97` 调用。含真实对象销毁，不能作显示 reset |
| EntityList 更新 | `0x413F00`／`0x013F00` | ECX=`Room+0x1218`，一个栈 word；Room caller `0x7F93A7` 显式 push 0；不把反编译 EBP 伪参数加入 ABI |

Init 参数的原始证据：

- `0x7E8B19` 读 `[EBX+8]`，保存第一个栈 word；`0x7E8D50` 再取入 ESI。
- `0x7E8D5C—0x7E8D7C` 按该地址 `+0x14` 的原生 string、`+8/+0xC` 值输出房间名称／编号。
- `0x7E8D9C—0x7E8DB4` 读该地址 `+0x46/+0x47` 两个 byte，各加 2 写 Room 宽／高，与 SDK RoomConfig_Room 布局相符。
- `0x7E9139` 读第二栈 word `[EBX+0xC]`，`0x7E9147` 写 Room `+4` descriptor。
- 原生 caller `0x738AD4` 先 push descriptor，`0x738AD5` 再 push 所选配置地址，`0x738AD8` call Init。

因此现有 hooks.json 中 `int roomType` 只能视为 ABI word 的历史变量名；不能把 RoomType 枚举值直接传给此入口。后续若使用 SDK wrapper，保持其已有声明／栈约定并明确地址用途；本次不改仓库外 SDK。

## 2. 显示／混合子入口

无 SDK 声明的名称以下均为职责描述；原型只记本次实际采用所需参数，不使用 Hex-Rays 因 EH／栈对齐推导出的额外 EBP／EDI 参数。

| 入口／职责 | VA／RVA | this／参数／返回指令 | 分类 |
| --- | --- | --- | --- |
| FXLayers 显示更新 | `0x96BF40`／`0x56BF40` | ECX=`Room+0x1618`，一个 bool 栈 word；`0x96C5E6/0x96C61B retn 4` | 可独立显示调用；普通 Room 传 false，换房收尾 caller `0x7395CE` 传 true |
| Backdrop::Init | `0x917870`／`0x517870` | SDK `void __thiscall(uint32_t id,bool loadGraphics)`；ECX=`Room+0x1BC8`；`0x917908 retn 8` | false 为窄状态／范围初始化；true 进入混合构图链 |
| Backdrop 图形构建 | `0x917910`／`0x517910` | ECX=Backdrop，无显式栈参数；`0x9185F9 retn`；当前 hook 的 int 是历史返回声明 | 构图／资源与装饰生成混合 |
| Backdrop::pre_render_walls | `0x911310`／`0x511310` | SDK `void __thiscall()` | 墙面构图中含 helper 与两处直接生成，拆分 |
| Backdrop 装饰生成 | `0x915890`／`0x515890` | 当前 hook：ECX=Backdrop，七个栈 word；`0x9171F1 retn 0x1C` | 继续拦截；不是 FXLayers 更新 |
| 房间背景／FX 选择初始化 | `0x7E7670`／`0x3E7670` | ECX=Room，无显式栈参数；`0x7E82EF retn` | 主题／资源选择、FX 加载和 shading 混合；按已选输入拆分 |
| FXLayers XML／候选加载 | `0x968E10`／`0x568E10` | 由 `0x7E7CDC` 调；涉及 stage／stageType 与 XML 路径，具体绑定不由反编译伪参数推导 | 资源准备可复用职责，整入口会重选层／ray 等，不作为逐帧白名单 |
| Room 几何 bounds 重算 | `0x7E86C0`／`0x3E86C0` | ECX=Room，一个 bool 栈 word；Init 早期 true、尾部 false | 结构维护；Client 优先安装 Host 已给 bounds，不能任意重算覆盖 |
| Camera::constructor | `0x91C730`／`0x51C730` | SDK `void __thiscall(Room*)`；ECX=已分配 `0x94` 本地对象 | 冷构建；不反复析构／重建，不转发 Host ring 指针 |
| 地狱横移背景资源准备 | `0x82E960`／`0x42E960` | ECX=`Room+0x776C`，无显式栈参数 | 可独立冷初始化；要求视图内 Room 回指与 descriptor 有效 |
| 地狱横移背景更新 | `0x82EE90`／`0x42EE90` | ECX=上述视图；`0x82EFBA retn` | 显示动画／位移与 Entity／碰撞混合 |
| Room::ComputeColorModifier | `0x818540`／`0x418540` | SDK 返回六 float 的 ColorModState；含隐藏结果存储 ABI | 读已安装状态计算颜色；优先用 SDK，不按反编译手写返回约定 |
| 原生房间颜色提交包装 | `0x818740`／`0x418740` | ECX=Room，无显式栈参数；`0x818785 retn` | ComputeColorModifier → Game::SetColorModifier(rate=0.015)，并读取初始化状态决定 lerp |
| Game::SetColorModifier | `0x7021A0`／`0x3021A0` | SDK `void __thiscall(ColorModState*,bool lerp,float rate<xmm3>)` | 写 Game 颜色目标／current／step；不要用普通栈 float 原型强转 |
| Room::TriggerClear | `0x7FBDD0`／`0x3FBDD0` | SDK `void __thiscall(bool)`；Room `0x7F97C1` 调用 | Host 规则／奖励，不进显示白名单 |
| Room::SpawnClearAward | `0x7F0DE0`／`0x3F0DE0` | TriggerClear `0x7FC087` 调入；官方 ASMRoom 的同名补丁模式在其内 `0x7F1932` 唯一匹配 | Host 奖励／进度／生成 |
| Room::ShopRestockPartial | `0x80ACA0`／`0x40ACA0` | SDK 无参；Room `0x7FA8F3` 调入 | Host 补货与定价 |

## 3. 定位模式与关键原始指令

Room::Init 使用 Room.zhl 的模式，在当前 IDB 唯一匹配 `0x7E8AD0`：

```text
53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B ?? 89 6C 24 ??
8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 ?? ?? ?? ?? 50 53 81 EC 30 08 00 00
```

以下两个本次记录的模式分别唯一匹配 Room Update、FX 更新；不是项目 gate 或哈希条件：

```text
# Room::Update 0x7F7F60
53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B ?? 89 6C 24 ??
8B EC 6A FF 68 ?? ?? ?? ?? 64 A1 ?? ?? ?? ?? 50 53 81 EC C0 0A 00 00

# FXLayers 显示更新 0x96BF40
55 8B EC 83 E4 F8 83 EC 28 56 57 8B F9 89 7C 24 ?? 80 7F 08 00
```

Backdrop::Init 的 SDK 模式 `558bec83e4f88b45088bd185c0` 唯一匹配 `0x917870`。其 false 分支只写 Backdrop ID 与 surface 范围；true 在 `0x917900` 调图形构建。原生常规更新明确是：

```text
0x7F9399  push 0
0x7F939B  lea ecx,[Room+0x1218]
0x7F93A7  call 0x413F00       # EntityList 玩法更新，跳过
0x7F93AC  push 0
0x7F93AE  lea ecx,[Room+0x1618]
0x7F93B4  call 0x96BF40       # 独立的 FX 显示更新
```

## 4. 优先资料与复用范围

- 原版 Wiki：[Room](F:/projects_isaac/IsaacDocs/docs/Room.md) 的 Update、RemoveGridEntity、SpawnClearAward、ShopRestock、EmitBloodFromWalls、水面查询。Update 文档提示调用它可能立即使用副手主动道具；不能以“刷新 Grid”为名在 Client 调整条根。
- REPENTOGON Wiki：[Room](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Room.md)、[Backdrop](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Backdrop.md)、[FXLayers](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/FXLayers.md)、[FXParams](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/FXParams.md)、[Camera](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Camera.md)、[RoomEventOutput](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/enums/RoomEventOutput.md)。水流 API 文档承诺自动处理声音／粒子，不表示其消费者没有物理写入。
- SDK：[Room.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Room.zhl)、[Backdrop.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Backdrop.zhl)、[FXLayers.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/FXLayers.zhl)、[FXParams.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/FXParams.zhl)、[RoomConfigRoom.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomConfigRoom.zhl)、Camera／Game／ANM2／AnimationState 声明。
- 官方实现：[LuaRoom.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/Room/LuaRoom.cpp:75) 的 SetBackdrop、SetWater*、UpdateColorModifier；[CustomCallbacks.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/LuaInterfaces/CustomCallbacks.cpp:3828) 的 PRE_NEW_ROOM；[EntitySaveStateManagement.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/SaveStateManagement/EntitySaveStateManagement.cpp:3481) 的 Init 外围稳定性维护；[ASMRoom.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ASMPatches/ASMRoom.cpp) 的 WaterDisabled／QuickRoomClear／奖励补丁。
- 项目已有事实：[Room 同步审查](../../room-synchronization-review.md)、[Room 补证](../../room-residual-evidence.md)、[显示资源设计](../../room-display-resource-design.md)、[原生维护与绘制](../../../tx-04-17/tx-04-17-native-maintenance-rendering.md)、[RE-1](../RE-1/README.md)。本次按新方案重新决定消费职责，旧“禁止全部显示 Lua／相机／本地显示生成”和旧结果同步方案不恢复；原始地址、副作用、失败与 UNKNOWN 保留。

SDK 也有已知不精确字段：Room Grid 数组声明少一项、path 数组过宽、`+0x7220` 的 slowdown 名称错误。已有字节补证是定位依据，不能为了调用整条 Init／Update 恢复这些宽读取或猜测未知字节。
