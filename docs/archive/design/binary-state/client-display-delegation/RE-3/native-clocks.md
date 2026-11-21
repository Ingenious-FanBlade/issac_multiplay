# RE-3：原生时钟与写入证据

返回：[成果总览](README.md)。所有 VA 均为 J273，imagebase `0x400000`，RVA=VA−`0x400000`。当前 IDB 为 `isaac-ng-v1.9.7.12.exe.unpacked.exe.i64`，沿用 RE-1／RE-2 会话；没有换库、附着调试器、设断点、改补丁或保存新符号。

`SOURCE` 指本地 Wiki／SDK／官方实现；`J273_STATIC` 指本次指令、静态字段引用及反编译；`REUSED` 指已有工程事实；`DESIGN` 指后续接线。下列结论不是运行时观察，不做哈希校验。

## 1. 先查公开时间语义与 SDK

- 原版 [Game.md](F:/projects_isaac/IsaacDocs/docs/Game.md:250)：Game:GetFrameCount 是活跃 gameplay 帧，暂停不计，约 30 帧／秒；TimeCounter 可修改，服务计时事件，不承担 timestepping。
- 原版 [Isaac.md](F:/projects_isaac/IsaacDocs/docs/Isaac.md:255)：Isaac.GetFrameCount 在暂停／菜单中也增长，约 60 帧／秒。不能把两个同名 API 当同一个值。
- 原版 [Room.md](F:/projects_isaac/IsaacDocs/docs/Room.md:329)、[Entity.md](F:/projects_isaac/IsaacDocs/docs/Entity.md)：房间／实体的活跃年龄。Room 的实际未初始化返回值以 J273 getter 为准。
- SDK [Game.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Game.zhl:298) 声明 `_frameCount : 0x25A70`，以及 RestoreState／NextVictoryLap；[Manager.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Manager.zhl:117) 声明 `_framecount : 0x4A8B8`，并声明 Update／Init／ExecuteStartGame；[Room.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Room.zhl:8) 声明 GetFrameCount。
- SDK 没有以 Room 帧号 setter 暴露 `+0x11F0`，公开 getter 的存在也不代表这个字段存放当前年龄。优先使用已有 SDK 声明理解 ABI；不能按字段名猜测加一行为。

本次从可执行段中的位移引用取得候选，再回看指令／函数与真实根。该方法是静态检索，既不执行 getter，也不证明通过别名、批量复制或官方补丁产生的一切写入都已闭合。Render 的额外写集仍按 RE-5／E28 处理。

## 2. 三个 getter

| 读值入口 | VA／RVA | J273 指令语义 | ABI |
| --- | --- | --- | --- |
| Game 帧 getter | `0x402A60`／`0x002A60` | `[ECX+0x25A70]` | ECX=Game，无栈参数，`0x402A66 retn`；本页按职责命名 |
| Entity 年龄 getter | `0x6A5290`／`0x2A5290` | 全局当前 Game 的 F − `[ECX+0x328]` | ECX=Entity，无栈参数，`0x6A52A1 retn`；与已有 Entity 来源结论一致 |
| Room::GetFrameCount | `0x809590`／`0x409590` | `[ECX]` byte 为 0 则 −1，否则全局当前 Game 的 F − `[ECX+0x11F0]` | SDK `int __thiscall()`，`0x809598/0x8095AA retn` |

关键原始指令：

```text
0x402A60  mov eax,[ecx+25A70h]
0x402A66  retn

0x6A5290  mov eax,[g_Game]
0x6A5295  mov eax,[eax+25A70h]
0x6A529B  sub eax,[ecx+328h]
0x6A52A1  retn

0x809590  cmp byte ptr [ecx],0
0x809593  jnz 0x809599
0x809595  or eax,0FFFFFFFFh
0x809598  retn
0x809599  mov eax,[g_Game]
0x80959E  mov eax,[eax+25A70h]
0x8095A4  sub eax,[ecx+11F0h]
0x8095AA  retn
```

Room 的 SDK 定位模式为 `80 39 00 75 ?? 83 C8 FF C3`；Game 短 getter 字节为 `8B 81 70 5A 02 00 C3`。记录用于定位，不构成项目 gate。Entity／Room getter 都依赖**当前全局 Game**，不是任意孤立对象；跨房退休后不能继续用旧指针配新 F 求年龄。

## 3. Game 帧号与 TimeCounter 的普通递增

普通 `Game::Update`（VA `0x6F6D30`／RVA `0x2F6D30`）在此前暂停／演出／转场等提前返回分支均未返回时，进入：

```text
0x6F7889  mov esi,[edi+25A70h]       ; F
0x6F788F  mov eax,88888889h
0x6F7894  inc dword ptr [edi+25A74h] ; 独立的 TimeCounter
0x6F789A  inc esi
0x6F789B  imul esi                  ; 为后续 %30 计算准备
0x6F789D  mov [edi+25A70h],esi       ; 写回 F+1
...
0x6F78BA  jnz 0x6F78D9              ; 新 F 的 30 帧周期
0x6F78C5  mov dword ptr [edi+22E94h],1
0x6F78CF  mov dword ptr [edi+22E9Ch],2
0x6F78DF  call Room::Update
```

`0x6F7889` 开始的 26 字节为：

```text
8B B7 70 5A 02 00 B8 89 88 88 88 FF 87 74 5A 02 00
46 F7 EE 89 B7 70 5A 02 00
```

递增在 Minimap::Update `0x6F77B0` 之后、Room::Update `0x6F78DF` 之前；HUD::Update `0x6F6D81` 更早，HUD::PostUpdate `0x6F79B9` 在后。因此它不是每次进入 Game／Manager 根都必然执行的计数。奇数帧 Odd helper 也不能替代这个普通分支。

**DESIGN**：继续拦截完整 Game::Update，显示根复用 RE-1／RE-2 的具名本地显示职责，不包含上述递增；Game／Room 内联标量按现行 Q8 安装当前值，不拆原段推进。无需为它额外安装一条字节补丁。若将来改为拆原根，不能仅 NOP `0x6F789D`：寄存器 ESI 已加一、TimeCounter 已改、后续 HUD 周期也使用该中间值；更不能把写回 F 当作玩法副作用已撤销。

## 4. Game 的其他已核显式写入

| 所属入口 | 写入 VA | 行为／来源 | Client 分类 |
| --- | --- | --- | --- |
| 公共 Game 重置 helper `0x6F0770`／RVA `0x2F0770`（描述名） | `0x6F0783`／`0x6F078D` | 分别置 F、TimeCounter 为 0，同时重置多项 Game 状态 | 不作为显示时钟 API；初始化链 `0x6F0B36` 与 SDK Game::Exit 内 `0x6F6454` 调它 |
| SDK Game::NextVictoryLap `0x6F3E10`／`0x2F3E10` | `0x6F42B8`／`0x6F42C2` | 两值清零，周围还有 Manager／fade／下一局生命周期操作 | Client 不运行这条根；按 Host 生命周期重建／安装目标 |
| SDK Game::RestoreState `0x6F43A0`／`0x2F43A0` | `0x6F457D`／`0x6F4586` | 从 GameState `+0x78/+0x7C` 分别复制 F／TimeCounter | 不用它仅恢复时间；Host 回溯后按实际目标 world／序列应用，F 可回退 |
| 普通 Game::Update | `0x6F789D`／`0x6F7894` | F 和 TimeCounter 各加一 | 根继续拦截 |

TimeCounter 因此不能等同 F，即使它们普通分支恰好同时加一。官方 [ASMRoom.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ASMPatches/ASMRoom.cpp:226) 还专门把一处计时事件判断从 GameFrame 改为相邻 TimeCounter；这不是让 Client 重跑事件判断的依据。

## 5. Room 锚点：赋值，不是自增

Room::Init `0x7E8AD0` 中：

```text
0x7E8CE2  mov eax,[eax+25A70h]
0x7E8CE8  mov [edi+11F0h],eax
```

原始字节为 `8B 80 70 5A 02 00 89 87 F0 11 00 00`。这把当前 F 写作入房基准。已读 Room::Update 中 `0x7F7FC5/0x7F8844/0x7F9468` 等是减去锚点求年龄，`0x7F8A01` 是读取锚点；没有在这些路径中逐帧递增它。

检索另命中 `0x8E37BC` 的 `[EBX+0x11F0]=0`，但其函数 `0x8E34A0` 在 `0x92A5AB` 的 ECX 来自 **Manager+0x4AFB8**，不是 Room。不能因为位移相同就把它记成 Room 清零点。

**DESIGN**：Client 安装 Host 原始 B；根 Init 保持 RE-2 判定，旧 ClientRoomFrameMaintain 的 `本地F−远端年龄` 重新锚定退役。没有需要新增的 Room `++` 拦截。initialized byte 与真实 owner／同目标 B 一并成立，不能为遮住 −1 返回值单独置 true。

## 6. Manager 的本地帧

SDK Manager::Update 是 static 声明；J273 根 VA `0x92E3E0`／RVA `0x52E3E0`，实际使用全局 Manager，清一个栈 word（`retn 4`）。不要按反编译偶然寄存器参数另造 thiscall 原型。

| 路径 | 写入 VA／字节 | 条件与含义 |
| --- | --- | --- |
| Update 普通尾部 | `0x92EC09`：`FF 87 B8 A8 04 00` | `inc [Manager+0x4A8B8]`，多种 Manager 状态走共同尾部 |
| Update 奇数帧快返 | `0x92E43E`：`8D 41 01`；`0x92E441`：`89 87 B8 A8 04 00` | M 为奇数且 `Manager+0x2A1FB`（OptionsConfig+0x83 enableInterpolation）为 false 时，按 `+0x4A8C0/+0x4A8C1` 条件写 M+1 后返回；受该条件阻止时不写 |
| SDK Manager::Init `0x92BA20` | `0x92BE9C/0x92BEED` | M 清零；不是每个 Client world 的时间重置 |
| SDK ExecuteStartGame `0x932A20` | `0x932A9D/0x933028` | 检查当前奇数时加一，使启动路径对齐偶数；不是每帧额外加一 |
| 原版消息／状态恢复 helper `0x8F2C60` | `0x8F32AB` | 从输入流读取奇偶 byte；若与当前 M 奇偶不同则加一。caller `0x8E903C` 属于原版消息处理链，周围还恢复全局随机状态，不为 C/S 借用 |

游戏状态分支中 `0x92E911` 在 M 偶数时调 Game::Update；奇数条件路径 `0x92E921` 调 Odd helper；相机阶段读取当前 M 奇偶，最后才到 Manager 尾部递增。Render 内常见的半帧值是暂停时 0，否则 `(M+1)%2`。这与 F 是两个来源；不能按 F 的奇偶替换 M。

当前工程若跳过整个 Manager 根，上述自然递增也一并消失，接线见[安装与本地调度](apply-contract.md#3-manager-的本地推进责任)。关闭／开启 interpolation、菜单、暂停和本地显示作用域的条件应在实现中保持明确；本次没有实机验证频率，Wiki 的约 30／60Hz 不能充当运行测量。
