# tx-04-07 ST2 子智能体笔记：`RevivePlayerGhost` 复活路径

> **主智能体审阅批注（2026-10-26）**：原文与原始导出保留，整项静态主审已完成，最终边界见[主报告第 15–18 节](../../../../docs/research/tx-04-07-static-confirmation.md#15-st2-主审范围结论与材料更正2026-10-26)。有条件采纳普通 Isaac 复用原对象、局部 controller/index 不变的结论；“返回后健康立即满足”过强，本体只清 ghost，valid/exists/dead/removed 等仍须实际读取。移动 consumer 未读 extra animation 不等于立即位移；主要射击块被跳过不等于整个 Shooting 不执行。第 3 节 InputManager 的“60 槽”更正为 **47 个函数槽（0–46）**，后项已是相邻表 RTTI。InitTwin 的 call 为 RVA **0x3CFF03**（0x3CFF08 保存返回值）；maxHearts 读取为 **0x3CFDB9**（0x3CFDFD 是 test）。backup 链会改换处理对象，有界扫描未命中不证明被跳过函数/回调无副作用；PeerLost 与持久观测保留。

日期：2026-10-25。角色：ST2 逆向子智能体（D29/U6），结论待主智能体独立审阅。

范围只限[实现方案](../../../../docs/design/tx-04-07-implementation-plan.md)第 10 节 ST2 及任务列出的 8 个问题。没有启动游戏、构建、运行测试或校验哈希，也没有修改 IDB：没有 rename、注释、类型、patch、define/undefine 或 idb_save。所有读取都经 IDA MCP 的只读 API 完成，`py_eval` 未使用 `new_locals=True`。自定义变量和函数都加了 `st2_` 前缀；顶层另外 `import` 了 `json/idc/idautils/ida_*` 等 IDA 标准模块名。

## 0. 环境、标签与产物

- IDB：`...\910d48ab-...\scratchpad\v12\isaac-ng-v1.9.7.12.exe.unpacked.exe.i64`，imagebase=`0x400000`；`server_health` 显示 auto analysis 与 Hex-Rays 均就绪。下文地址**先写 RVA**（= VA − 0x400000），必要时括注 VA。
- 官方源码：`F:/projects_isaac/REPENTOGON-1.1.3`。官方 Hook 不在原生 IDB 中，相关结论只标 `SOURCE`。
- 标签含义：
  - `STATIC_CONFIRMED`：由所列指令直接证明；
  - `STATIC_INFERRED`：基于字段名、签名或调用关系的有条件推导；
  - `SOURCE`：来自官方源码；
  - `UNKNOWN`：未闭合。

  Hex-Rays 自动类型和变量名不作 ABI 证据；关键分支、字段写入和调用都回到汇编核对。
- 函数命名：凡写“签名匹配”的名称，都是把函数起始字节与 `libzhl/IsaacRepentance.cpp` 的 `FunctionDefinition` 签名逐字节（含 `??`）比对所得，关键签名另用 `find_bytes` 确认在全映像唯一。名称只作人工解释，不写回 IDB。
- 产物（本目录）：
  - `ida-st2-functions.json`：10 个完整函数（Hex-Rays 原始伪代码、逐条 VA/RVA/字节/汇编、直接/间接调用、到入口的 xref），以及 22 段关键指令片段；
  - `closure-scan.json`：有界调用图扫描（深度、命中目标、间接调用、字段写扫描），以及两个 consumer 对 extra animation/控制字段的访问；
  - `export_st2.py`：生成 `ida-st2-functions.json` 的只读脚本，以 `py_eval` 的 `exec(open(...).read())` 执行。它不是测试或校验入口。

## 1. 定位

`STATIC_CONFIRMED`：libzhl 签名 `558bec83e4f883ec145356578bf9c74424??00001000`（`EntityPlayer.zhl:515–516`，`IsaacRepentance.cpp:18242`）在全映像**唯一**命中 VA `0x7CFC60`。

**`RevivePlayerGhost` 的 RVA 为 `0x3CFC60`**，函数范围 `0x3CFC60–0x3CFF97`，共 180 条指令。

- 调用约定：`0x3CFC6C mov edi, ecx` 取 this，`0x3CFF96 retn` 不弹栈参数，与 zhl 声明的 `__thiscall void ()` 一致。
- 它紧邻 Morph（`0x3CFFA0`）之前。
- 相近签名 `...c74424??00000000` 属于 `GetGlyphOfBalanceDrop`，不会混淆。

### 1.1 指令级流程

`edi` 为当前处理的 Player；函数是一个循环，见末行。

| RVA | 指令事实（`STATIC_CONFIRMED`） | 字段含义（zhl，`STATIC_INFERRED`） |
| --- | --- | --- |
| `0x3CFC88` | `mov byte [edi+207Dh],0` | `_isCoopGhost=0` |
| `0x3CFC8F` | `mov byte [edi+3B0h],0` | `_isInvicible=0`（Morph `0x3D02C7` 曾写 1） |
| `0x3CFC96` | `mov dword [edi+188h],4` | `_entityCollisionClass=4`（Morph `0x3D02A0` 曾写 0） |
| `0x3CFCA0` | `ecx=edi+98h`，调 `0x003DE0(0)` | 签名匹配 `AnimationState::Reset` |
| `0x3CFCA5–0x3CFCF9` | 在全局 VA `0xBE4A1C`+`0x2A36C` 的 map 中按键 `0x100000` 查找，取节点 `+0x8C`（未找到则得常量 `0x74`），再以 `ecx=edi+48h` 调 `0x007AF0(path,0)` | Manager `_entityConfig:0x2a36c`、`EntityConfig_Entity.anm2Path:0x74`；`0x007AF0` 签名匹配 `ANM2::Load`。即把 `_sprite` 重新载入某实体配置的 anm2；键 `0x100000` 推定对应 Player 配置项 |
| `0x3CFD00` | `call 0x3B5CF0(this)` | 名称未知；读取 ghost、`playerType`、Variant，推定为皮肤/图形刷新 |
| `0x3CFD05–0x3CFD1A` | 仅当 `[VA 0xBE49F4]+0x25F63 == 0` 时，调 `0x3A3340("Appear")` | `Game._isRestoringGlowingHourglassState:0x25f63`；`0x3A3340` 为 `PlayExtraAnimation`（见第 5 节） |
| `0x3CFD21` | `mov [edi+13B8h],60` | `_damageCooldown=60` |
| `0x3CFD2B` | `call 0x3560C0(this)` | 名称未知；Morph `0x3D006F` 也调用它。其内 `[ecx+28h]` 写入的目标是局部颜色/服装结构，不是 Player |
| `0x3CFD30–0x3CFD8C` | `+0x2080..+0x209C` 的 8 个 dword 复制到 `+0x1790..+0x17AC` | Morph `0x3D008D–0x3D01EC` 保存过这段，此处还原 |
| `0x3CFD92–0x3CFDA4` | `+0x20A0/+0x20A4` 复制到 `+0x16B0/+0x16B4` | `_trinketsID[2]` 还原（Morph `0x3D0226/0x3D022E` 保存） |
| `0x3CFDAA`、`0x3CFDB4` | `or [edi+1568h],0FFFFh`；`call 0x35D4B0` | `_cacheFlags |= CACHE_ALL(0xFFFF)`；`EvaluateItems` |
| `0x3CFDC1–0x3CFDF3` | `+0x134C/+0x1350/+0x1348/+0x1D78/+0x1D74/+0x1D90` 置 0 | 魂心、黑心、永恒心、骨心 mask、骨心、腐心清零 |
| `0x3CFDFD–0x3CFE82` | 见下方“生命值分支” | — |
| `0x3CFE92–0x3CFEC7` | 音效 id 先置 80，经 helper `0x52FAD0`（特定条件下改写 id）后，以 `ecx=Manager+0x2A160` 调 `0x507C60` | `_sfxManager`；`0x507C60` 签名匹配 `SFXManager::Play` |
| `0x3CFECC–0x3CFEF1` | `Random()` 后调 `0x2FA210(1000, 15, &[edi+33Ch], &VA 0xBEB900, 0, 0, seed, 0)` | 签名匹配 `Game::Spawn(type, variant, pos, vel, spawner, subtype, seed, unk)`：在 Player 位置生成 Effect 1000.15，速度参数推定为零向量 |
| `0x3CFEF6–0x3CFF74` | 仅当 `[edi+13BCh]==35` 时：`0x3C8370(40)` 及新对象的 extra animation 初始化 | `playerType==35`（`PLAYER_THEFORGOTTEN_B`，`IsaacRepentance.h:889`）；`0x3C8370` 签名匹配 `InitTwin` |
| `0x3CFF79–0x3CFF8A` | `edi=[edi+1E50h]`；当它非空且 `[edi+207Dh]!=0` 时回到 `0x3CFC80` | `_backupPlayer:0x1e50`；若备用 Player 也是 ghost，就对它重复整套操作 |

**生命值分支**（`STATIC_CONFIRMED`）。`GetHealthType` 为 `0x3C12C0`（签名匹配）。官方 `HealthType` 枚举为 `0=DEFAULT/RED`、`1=SOUL`、`2=LOST`、`3=COIN`、`4=BONE`（`enums_ex.lua:1584–1590`，`SOURCE`）。

- 若 `maxHearts(+0x1340) > 0`：`redHearts(+0x1344) = min(maxHearts, 3)`（`0x3CFE01–0x3CFE07`）；HealthType 为 3 时改为 2（`0x3CFE12–0x3CFE17`）。
- 否则按 HealthType：
  - 为 4：红心=2，并调 `AddBoneHearts(1)`（`0x3CFE31/0x3CFE3B`）；
  - 为 3：红心=4，`maxHearts`=4（`0x3CFE4C/0x3CFE56`）；
  - 其余：红心=0，魂心按 `(type != 2) ? 3 : 1` 计算（`0x3CFE6B–0x3CFE82`）。

`0x3CFDFD` 只读一次 `maxHearts`；普通 Isaac 实际走哪一支，取决于运行时的 `maxHearts`，本文不作断言。

**调用者负责 ghost 前置检查。** 函数第一轮不检查 this 是否为 ghost。所有调用者都先检查 `+0x207D`：Lua 包装见第 9 节；原生调用点见第 6 节。

## 2. 问题 1：复用还是新建？Init/构造？vector 增删？

结论（`STATIC_CONFIRMED`，限普通路径与有界扫描）：

- **复用原对象。** 所有字段写入都以入口 `ecx` 为基址，即 `edi`（`0x3CFC6C`）。函数不分配新的 Player，也不 Spawn Player。`0x3CFEF1` 的 `Game::Spawn` 生成的是 Type 1000 Effect，不是 Player。
- **不重新 Init、不构造。** 直接调用目标（`ida-st2-functions.json` 中的 `direct_calls`）不含 `Player::Init`（`0x34F6A0`，签名匹配且唯一）或 Player 构造函数（`0x34DC70`，签名匹配且唯一）。深度 ≤ 4 的有界扫描也未命中这两个目标（第 10 节）。
- **不增删 PlayerManager 的 Player vector。** 函数自身的指令既不访问 `Game+0x1BAA8`，也不调用 `SpawnCoPlayer2`（`0x590250`）、`SpawnCoPlayerBaby`（`0x58FE80`）或 `RemoveCoPlayer`（`0x590560`）。

**会增加 Player 的条件路径**（`STATIC_CONFIRMED` 调用存在，是否触发取决于条件）：

- `playerType==35`（`0x3CFEF6`）。`InitTwin` 的 `0x3C838D` 调 `SpawnCoPlayer2(40)` 新建 T.Soul，`0x3C8394–0x3C83FB` 双向链接 `+0x1E4C`，`0x3C840A` 调 `SetControllerIndex(新对象, this+0x160C, 0)`。这个分支会给 vector 加一个与 B 同 controller 的 Player。普通 Isaac（`playerType=0`）不走。
- `EvaluateItems` 的附属 Player 维护块（第 7 节）。
- `_backupPlayer` 链（`0x3CFF79`）只复活已有对象，不新增。

## 3. 问题 2：controller、player index、设备通知、Join/Remove 路径

结论（`STATIC_CONFIRMED`，限函数本体与有界扫描）：

- **controller 与 player index。** 函数本体 180 条指令中没有任何以 `0x160C` 或 `0x1610` 为位移的操作数。
- **深度 ≤ 3 的写扫描**：跳过 EvaluateItems、Spawn、InitTwin、InitPostLevelInitStats、AddBoneHearts 与 LogMessage 后余下 120 个函数，没有对 `0x160C/0x1610/0x13BC/0x170/0x171/0x172/0x173/0x177/0x207D/0x410/0x414` 的写操作数（`closure-scan.json` → `field_write_scan_depth3.hits=[]`）。
- **EvaluateItems 本体**：同一扫描另加 `0x28/0x2C`，也没有写入（`evaluateitems_body_write_scan.hits=[]`）。
- **没有设备、Join 或 Remove 调用。** 深度 ≤ 4 的有界扫描（183 个函数，跳过集合同上）未命中下列任何一项：
  - InputManager 第 45 槽注册（`0x5EEB40`）、第 46 槽移除（`0x5EEBB0`）；
  - `SetControllerIndex`（`0x39E1B0`，签名匹配且唯一）；
  - Join pass（`0x591360`）、CoopBabiesOnly predicate（`0x5910A0`）；
  - `SpawnCoPlayer2`（`0x590250`）、`RemoveCoPlayer`（`0x590560`）；
  - `IncreaseEventCounter`（`0x503A50`）、`TryUnlock`（`0x503930`，签名匹配且唯一）。

  上述深度内唯一的间接调用出自音效后端（`0x507C60` 内的 `call [eax+40h]` 等 vtable 调用），以及 ANM2 资源加载链。资源加载链在深度 4 到达图像加载器 `0x5EEE80`（字符串 `png/pcx/"Failed to load image %s"`）；它不在 InputManager vtable `0x732E44` 的 60 个槽中。
- **条件路径上的 controller 设置**（`STATIC_CONFIRMED` 调用存在，对象都**不是 B 本身**）：
  - `InitTwin` 的 `0x3C840A`（`playerType==35`）；
  - `EvaluateItems` 的 `0x368354/0x368494`。新建的附属 Player 会获得调用者的 `+0x160C`（第 7 节）。

  这两条在普通 Isaac、且无相关道具/饰品时不走。
- **边界：`UNKNOWN`。**
  - 深度扫描显示，经 `KAGE::LogMessage`（`0x5E0DF0`）或 ANM2 加载 → Lua/消息泵类中枢（如 `0x4CB4F0`）的边，可以“到达”整个游戏循环，包括 RemoveCoPlayer、SetControllerIndex、TryUnlock 等。这些边属于日志/资源/脚本分派，不是可证明的复活语义调用，但也没有被证伪。
  - 跳过集合内的 `EvaluateItems`、`Game::Spawn`、各官方/第三方回调，其完整闭包未闭合。
  - 因此“复活窗口内没有任何设备通知”不能全局断言；只能说函数本体与普通路径的有界闭包中没有发现。

## 4. 问题 3：复活后各字段的值

对 B 本身（普通 Isaac、无 `_backupPlayer`）：

| 字段 | 复活函数是否写 | 复活后的值 |
| --- | --- | --- |
| ghost `+0x207D` | **写 0**（`0x3CFC88`） | 0（`STATIC_CONFIRMED`） |
| valid `+0x170` | 未写 | 沿用进入前的值 |
| visible `+0x171` | 未写 | 沿用（ST1：Morph `0x3D02CE` 写过 1） |
| exists `+0x172` | 未写 | 沿用 |
| dead `+0x173` | 未写；直接调用中也没有 `Player::Revive`（`0x39A400`） | 沿用。ST1 的正常 Morph 先由 Revive 清 dead，所以预期为 0，但这是由前段推出的（`STATIC_INFERRED`），不是本函数设置的 |
| removed `+0x177` | 未写 | 沿用 |
| Type `+0x28` / Variant `+0x2C` | 未写（EvaluateItems 本体也没有写） | 沿用 |
| playerType `+0x13BC` | 未写，只在 `0x3CFEF6` 读 | 沿用 |
| controller `+0x160C` / index `+0x1610` | 未写 | 沿用 |
| ControlsEnabled `+0x410` / ControlsCooldown `+0x414` | 未写 | 沿用（字段名见第 5 节的 Lua 注册证据） |

另外明确写入或改变的项：

- `_isInvicible=0`、`_entityCollisionClass=4`、`_damageCooldown=60`；
- 心值组（第 1.1 节）；
- `+0x1790..+0x17AC` 与饰品还原，`_cacheFlags |= 0xFFFF` 后调 `EvaluateItems`；
- 重新载入 `_sprite`；非玻璃沙漏恢复时设置 extra animation（`+0x1398=1`、`+0x139A=0`、`+0x14C=1.0f`），以及 `_bodySprite(+0x41C)` 的 ANM2 Reset；
- 生成 Effect 1000.15、播放音效、推进一次全局 `Random()`（`0x3CFECC`；只是状态推进，不是持久写入）。

除表中写明“写 0”的 ghost 外，本函数没有把任何“健康条件”字段设为健康：valid、exists、removed 未写，只是沿用；dead 的 0 也来自 Morph 前段。

## 5. 问题 4：同步还是异步？复活后是否有不可控的帧？

**同步完成**（`STATIC_CONFIRMED`）。

- ghost 清零、碰撞与无敌恢复、心值、道具重算都在本次调用内完成。函数没有设置“稍后由 Update 处理”的请求标志，也没有通过 PlayerManager 排队。
- 返回前若 `_backupPlayer` 也是 ghost，会同步处理它。
- Lua `ReviveCoopGhost()` 返回 `true` 时，原生调用已经返回（`SOURCE`，见第 9 节）。

**复活后的若干帧：** 有一段 extra animation 窗口。

- `STATIC_INFERRED`：`0x3A3340` 是 `PlayExtraAnimation`。原生 Lua 注册在 RVA `0x45016E/0x450173` 将 `sub 0x3A3340` 与字符串 `PlayExtraAnimation` 成对压栈。配对方向以同一注册序列中签名唯一的 `AddEternalHearts`（`0x4506D7/0x4506DC`）校准。同一序列还给出：
  - `IsExtraAnimationFinished`=`0x3A2630`，读 `+0x1398/+0x139A`，两者都为 0 才返回真；
  - `StopExtraAnimation`=`0x18DCD0`，清这两个字段；
  - `ControlsEnabled` 的 getter `0x2B4430` 读 `+0x410`；`ControlsCooldown` 的 setter `0x1B3530` 写 `+0x414`。
- `STATIC_CONFIRMED`：只有在 `Game+0x25F63==0`（非玻璃沙漏恢复）时才播放 `Appear`。`0x3A3398` 写 `+0x1398=1`。`Player::Update`（VA `0x77BD40`）的 `0x37D234–0x37D24E` 在 `+0x1398` 为真、且 `0x005ED0(&_sprite, ArgList_2)` 返回真时清 `+0x1398`；该 helper 推定为“当前动画结束”判断（`STATIC_INFERRED`）。
- **移动 consumer**（`STATIC_CONFIRMED`）：`0x372540` 在 `0x3725E0/0x372611`、`0x371D70` 在 `0x371DC3/0x371DCC` 用 `+0x410`（启用）与 `+0x414 <= 0`（冷却）门控输入查询。这两个函数内没有读 `+0x1398/+0x139A`；复活函数也不写 `+0x410/+0x414`。因此 extra animation 本身不阻止移动查询。
- **射击 consumer**（`STATIC_CONFIRMED`）：`0x373990` 在 `0x373ACC–0x373B37` 处，若 `+0x1398` 或 `+0x139A` 为真，只有 `[edi+450h]` 指向的字符串（推定为 `_bodySprite` 当前动画名）等于 `LiftItem` 才进入常规射击块 `0x373B3D`，否则跳到 `0x373F62`。tx-04-05 记录的射击查询 `0x373C2D–0x373D16` 位于被跳过的块中。
  - `UNKNOWN`：`0x373F62` 之后的其他查询点（如 `0x374514`、`0x376013`）在该窗口内是否执行，没有逐条确认。
- `_damageCooldown=60` 按 zhl 名称是受伤冷却（`STATIC_INFERRED`），不是控制冷却；复活函数没有写 `ControlsCooldown`。
- `UNKNOWN`：
  - `Appear` 的实际帧数由 anm2 资源决定，静态未读；
  - `0x3B5CF0`、`0x3560C0` 的完整副作用未逐条展开（有界扫描中没有对关键字段的写）；
  - 官方/第三方回调在窗口内的行为未知。

**对“之后第一个稳定 Authority 入口重新绑定”的意义**（`STATIC_INFERRED`）：

- 复活返回后，身份与健康字段立即满足方案第 5.1 节的读取条件。复活函数不会制造额外的“未就绪”标志。
- 但在 `Appear` 期间，原生射击 consumer 会跳过主要的射击输入块。若在该窗口内重绑并发送射击，可能看到“已接受但原生未消费射击”；移动不受此影响。

## 6. 问题 5：原生调用方及其条件

到 VA `0x7CFC60` 的引用共 6 条，全部是代码调用，分属 4 个函数。没有数据引用；全映像搜索 dword `60 FC 7C 00` 也没有命中，所以没有发现函数指针表（`STATIC_CONFIRMED`）。另外有两层反向调用：

| 直接调用者 | 调用点 | 条件（`STATIC_CONFIRMED` 指令，语义标注见括号） |
| --- | --- | --- |
| `AddEternalHearts` `0x352D70`（签名匹配且唯一） | `0x352D83` | `0x352D7A`：this 为 ghost 时**直接复活并返回，不加永恒心**；与 amount 无关 |
| `Player::RestoreGameState` `0x3A5660`（签名匹配） | `0x3A5753` | 要求 `Game+0x25F63`（正在恢复玻璃沙漏）、this 为 ghost、且 `state+0x574==0`。`0x3A682A–0x3A6830` 把 `state+0x574` 直接复制到 `+0x207D`，可见 `+0x574` 是保存的 ghost 标志 |
| PlayerManager“复活全部 ghost” `0x596AE0`（名称为描述性） | `0x596B00` | 遍历 `_playerList`，对每个 ghost 调用 |
| `PlayerManager::ReviveCoopPlayer(index)` `0x596B20`（依据原生日志字符串 `PlayerManager::ReviveCoopPlayer() recieved invalid player index`） | `0x596B57/0x596BB6/0x596BD3` | 对指定索引的 ghost 调用，并按 `+0x1E4C/+0x1610` 处理 twin |

**上层触发点**（只列入口与条件，`STATIC_CONFIRMED` 指令；物品/实体名称凡注“推定”者未在源码中核对）：

- **AddEternalHearts 的 12 条引用：**
  - `UseActiveItem` `0x1B63BC`：道具 case 146，推定 Prayer Card，要求 `[ebp+a5]` 对象的 Variant==0；
  - `0x24ACE9`：某 switch case 3，Player 取自对象 `+0x410`；
  - `Entity_Slot::Update` `0x25B29A`；
  - `Entity_Pickup::HandleCollision` `0x2E5CFC/0x2E7813`；
  - `AddCollectible` `0x359F16`：道具 case 179，推定 Fate；
  - `TriggerNewStage` `0x39D8E4`：仅当 `GetTrinketMultiplier(55)>0`，饰品 55 为 `TRINKET_MAGGYS_FAITH`（`IsaacRepentance.h:2086`）；
  - `0x3C13D1`：`0x3C1370` 内 switch case 29（switch 语义未核对）；
  - `ChangePlayerType` `0x3C4AE3`（amount=0）与 `0x3C4B66`（twin 的永恒心）：**对 ghost 改角色会触发复活**；
  - `RemoveCoPlayer` `0x5907AE`：switch case 29，对 helper `0x3C41B0` 返回的 Player 调用；
  - 原生 Lua 注册 `0x4506D7`：原生 `EntityPlayer:AddEternalHearts` 作用于 ghost 也会触发复活。
- **复活全部（`0x596AE0`）的 4 个调用者：**
  - `Entity_Slot::Update` `0x25EE40`：Slot variant 19（`0x25B41F–0x25B42E` switch，名称未核对），`CoinInsert` 事件后，ghost 主 Player 数（`0x596A70`）恰为 1 时；
  - `Entity_Pickup::HandleCollision` `0x2E8993`：pickup variant 380（`PICKUP_BED`，`IsaacRepentance.h:2889`）且 subtype==10；
  - `Room::SpawnClearAward` `0x3F170B`：Lua 注册配对得名。条件为 `IsCoopPlay` 且 boss 条件成立；boss 条件是 helper `0x415B80`（当前房描述等于 `Game+0x18314`，推定最后 boss 房）为真，或 roomIdx==-10 且房型==5；当 `Game+0x25AFC==44` 时改按房间描述类型==5 判断；
  - `Room::TriggerClear` `0x3FC0F9`（签名唯一）：`Game+0x25F40` 为 2/3（推定 Greed 模式）时，普通房 `Game+0x18334` 达 10/11，或 boss 房 `[room+1BB0h]==62`，并且 `IsCoopPlay`。非 Greed 模式经 `0x3FC087` 走 SpawnClearAward。
- **ReviveCoopPlayer（`0x596B20`）的 2 个调用者：**
  - `0x4CCFF4`：属于对象 `Game+0x24444` 的菜单状态机 `0x4CCCC0`，推定为 ReviveMenu。推定只由 Slot variant 19 在 ghost 数≠1 时打开（`0x25EED8–0x25EEDF` 经唯一 getter `0x259960` 调 `0x4CCB70`；`Game+0x24444` 的其余 11 处字节引用为初始化、重置、更新、渲染或判断，没有逐条证明 `0x4CCB70` 写入 state=1）；离线时（Netplay 列表为空）按菜单选中的索引复活；
  - `0x4EDC7B`：原版 Netplay `Message_RevivePlayer` 处理（日志字符串可证）。C/S 不使用原版在线，故不适用。

**S10/S11 之外的自动复活**（`STATIC_INFERRED`）：在普通模式、普通房、B 为无相关道具/饰品的 Isaac，且测试 mod 与第三方回调都不调用 AddEternalHearts、ChangePlayerType 或 RestoreGameState 时，上述原生入口都不会被满足。以下动作仍会原生复活 ghost B：

- 清理 boss 房；
- Greed 波次；
- Slot variant 19；
- Mom's Bed 类 pickup；
- Maggy's Faith 饰品在换层时生效；
- 对 ghost B 加永恒心或改角色；
- 玻璃沙漏恢复。

这只是 2 层反向调用的有界结论，不是全映像闭包。

**对 ST3（换层）的意义：**

- `TriggerNewStage` 本身只经 Maggy's Faith 复活。在正常流程中，boss 房清理（SpawnClearAward）通常发生在换层之前，复活会在那里发生。
- `STATIC_CONFIRMED`：
  - `Player::RestoreGameState` 的 `0x3A682A–0x3A6830` 不经过 RevivePlayerGhost，直接用保存值覆写 ghost 标志；
  - `RestoreGameState_PostLevelInit`（`0x3A71C0`，签名匹配）在 `0x3A7BE7` 对恢复为 ghost 的 Player 置 `+0x207D=1`；
  - `Player::Init` 在 `0x34FD61` 置 ghost=0；
  - `RestoreGameState` 在 `0x3A56B8–0x3A56BE` 调 `SetControllerIndex(state+0x518, 1)`。
- `UNKNOWN`：换层与 Continue 是否经过这些 restore 或 init 路径，属于 ST3/ST4 的范围，本文不推断。

## 7. 问题 6：复活路径上的持久写入入口与条件

只记录入口与条件，不判断是否允许。

- **普通路径的直接调用**（`STATIC_CONFIRMED`）：没有直接调用 `IncreaseEventCounter` 或 `TryUnlock`。深度 ≤ 4 的有界扫描中，跳过集合之外也没有命中（第 10 节）。
- **`EvaluateItems`**（`0x3CFDB4`，cache 为 `0xFFFF`）中的 TryUnlock 条件路径，ST1 已列出并已复核位置：
  - `0x367520`(144)、`0x367637`(19)、`0x3679B6/0x3679CD`(58/370)、`0x368610`(355)、`0x368EBD`(361)；
  - 道具/效果/阈值等条件见 ST1 报告。
- **`EvaluateItems` 的附属 Player 块**（`STATIC_CONFIRMED`，`0x364B9B–0x364BBE` 与 `0x36816E–0x368534`）：
  - 门控：cache 含 `0x200`、`Variant==0`、helper `0x012D30` 为真，且全局重入标志 VA `0xBE90F0==0`。
  - 目标数：
    - Found Soul 路线：`HasTrinket(180)`（`TRINKET_FOUND_SOUL`，`IsaacRepentance.h:2225`）为真时，目标数 = `max(0, 1 − GetTrinketEffectNum(180))`；
    - 道具 667 路线（推定 Strawman）：目标数 = `GetCollectibleNum(667)`。
  - 按目标数增删本 Player 拥有的附属 Player：
    - `RemoveCoPlayer`：`0x3682FC`、`0x36843C`；
    - `SpawnCoPlayerBaby(59)`：`0x36833F`；
    - `SpawnCoPlayer2(14)`：`0x36847F`；
    - 新建对象的 `SetControllerIndex(owner+0x160C)`：`0x368354`、`0x368494`；
    - 新建对象的 `EvaluateItems`：`0x3683BB`；`AddCollectibleEffect(667)`：`0x3684F0`；`InitPostLevelInitStats`：`0x3684FB`。
  - 道具 667 路线还要求 `0x206EE0` 与 `0x332B80` 都为假（语义未核对）。
  - 普通 Isaac 在无该道具/饰品、也无已有附属 Player 时，目标数与现有数都是 0，不会执行这些调用（`STATIC_INFERRED`）。
- **心值分支：** HealthType==4 且 `maxHearts<=0` 时调 `AddBoneHearts(1)`（`0x3CFE3B`）。其下游有 TryUnlock：`0x35306F`，以及 `0x353B10` 至 `0x503600/0x504260` 一线（深度 2–3），普通 Isaac 不走。
- `playerType==35` 时会走 `InitTwin` 与 `InitPostLevelInitStats`，它们又会调用 EvaluateItems/InitTwin。
- `UNKNOWN`：
  - `Game::Spawn`（Effect 1000.15）内部的实体初始化与回调分派未展开；
  - 音效后端只是虚调用，无持久写入证据；
  - `0x3B5CF0/0x3560C0` 有界扫描未见 PGD 入口；
  - 官方 Hook（第 9 节）和第三方回调中的写入未闭合；
  - 没有 PGD/存档写入不能由“未发现”推出，留给 ST7 与 V16 的观测。

## 8. 问题 7：对 U10 的意义

**普通 Isaac 的 B 经 `ReviveCoopGhost` 正常复活后：**

- **不会被移除。** 不写 `+0x177`，不调 RemoveCoPlayer（`STATIC_CONFIRMED`，限函数本体与第 10 节有界扫描）。
- **Type/Variant/playerType 不变。** 都未写（`STATIC_CONFIRMED`）。
- **controller 仍为 K。** 不写 `+0x160C/+0x1610`，不调 SetControllerIndex 或 InputManager 注册/移除（`STATIC_CONFIRMED`，有界）。
- **Player 地址不变。** 复用 this，不新建、不 Spawn Player（`STATIC_CONFIRMED`）。
- **vector 不重排。** 函数不访问 `_playerList`；普通路径不 Spawn 或 Remove Player（`STATIC_CONFIRMED`，有界）。

**会破坏上述性质的已知静态条件：**

- `playerType==35`：新增与 B 同 controller 的 T.Soul，vector 加一项，同一 controller 下有两个 Player；
- 持有 Found Soul 饰品或道具 667：EvaluateItems 新增或移除附属 Player，新对象取得 B 的 controller；
- `_backupPlayer` 非空：会同时复活另一对象；
- 官方/第三方回调（第 9 节）：例如在 EvaluateCache、自定义 cache、音效、HealthType、服装回调中改动对象。

注意，`EvaluateItems` 的附属 Player 块不止在复活时运行：任何 cache 重算都会进入。即使不复活，“同一 controller 下出现第二个 Player”也可能由道具引起（`STATIC_INFERRED`）。

**对 U10 的建议边界**（供主智能体判断，本文不作决定）：

- 静态上，复活函数本身的普通路径不会产生方案第 5.3 节的移除、Type/Variant 变化、controller 非 K 或拓扑不唯一。
- 但整个 Lost→Revived 窗口有以下 `UNKNOWN` 未闭合：
  - Morph 和复活的完整回调闭包；
  - 原生其他复活或移除入口（第 6 节）；
  - 测试 mod 与官方 Hook 的实际执行。
- 因此不能把“普通路径未见”提升为“本轮绝不出现”。

## 9. 问题 8：官方 REPENTOGON Hook 与回调（`SOURCE`）

**直接相关：**

- `RevivePlayerGhost` 本身**没有** `HOOK_METHOD` 或 ASM patch。全库 grep `RevivePlayerGhost`，只有 `EntityPlayer.zhl:516` 的声明、`IsaacRepentance.cpp:18242/18245` 的桩，以及 Lua 包装。
- Lua 包装 `LuaInterfaces/Entities/LuaEntityPlayer.cpp:2355–2366`：
  - 只有在 `_isCoopGhost` 为真时才调用 `player->RevivePlayerGhost()` 并返回 `true`，否则返回 `false`；
  - 包装本身没有回调或延迟；
  - 在 `:3750` 注册为 `ReviveCoopGhost`。

**下游与相关 Hook：**

| 源码位置 | Hook | 与复活的关系 |
| --- | --- | --- |
| `Patches/CustomCache.cpp:318–339` | `HOOK_METHOD_PRIORITY(Entity_Player, EvaluateItems, -1)` | 复活把 cache 置为 `CACHE_ALL=0xFFFF`（`IsaacRepentance.h:2856`），因此会把所有已知自定义 cache 都加入 tag，并逐个运行 `RunEvaluateCustomCacheCallback`，然后再调用原生函数 |
| `LuaInterfaces/CustomCallbacks.cpp:266–330` | `HOOK_METHOD(SFXManager, Play)` | 音效 pre/post 回调 1030/1031；pre 回调可以改写或取消音效 |
| `LuaInterfaces/CustomCallbacks.cpp:1574` | `HOOK_METHOD(Entity_Player, GetHealthType)` | 回调 1067，并可能触发 1128。返回值决定复活后的心值分支 |
| `Patches/Anm2Extras.cpp:89–92` | `HOOK_METHOD(ANM2, Reset)` | `PlayExtraAnimation` 内对 `_bodySprite` 的 Reset，只清官方附加数据 |
| `LuaInterfaces/CustomCallbacks.cpp:5482–5484` | `IsCoopBaby()`（Variant==1 或 ghost） | 用于服装回调 1281/1283。复活清 ghost 后，`0x3B5CF0/0x3560C0` 若添加服装，判定会随之变化（`STATIC_INFERRED`） |
| `LuaInterfaces/CustomCallbacks.cpp:2827–2832`、`Patches/ASMPatches/ASMCallbacks.cpp:2060–2065` | HUD RenderHearts/RenderActiveItem | ghost 时跳过官方回调；复活后恢复常规 HUD 回调 |
| `LuaInterfaces/CustomCallbacks.cpp:4875–4876` | `HOOK_METHOD(Entity_Player, Revive)` 回调 1481/1482 | **RevivePlayerGhost 不直接调用 `Player::Revive`**（直接调用目标中没有 `0x39A400`），复活时这对回调不由本函数触发；ST1 中 Morph 会调用 Revive |
| `LuaInterfaces/CustomCallbacks.cpp:751–763` | `HOOK_METHOD(Entity_Player, AddEternalHearts)` 回调 1009/1010 | 原生“对 ghost 加永恒心”这条复活入口会先经过官方加心回调；Lua `ReviveCoopGhost` 不经过 |
| `LuaInterfaces/CustomCallbacks.cpp:779`、`:884` | `AddBoneHearts`、`InitPostLevelInitStats` | 只在 HealthType==4 或 `playerType==35` 的分支中涉及 |
| `LuaInterfaces/CustomCallbacks.cpp:100`、`SaveStateManagement/EntitySaveStateManagement.cpp:2770`、`Patches/ASMPatches/ASMPlayerManager.cpp:198`、`LuaInterfaces/CustomCallbacks.cpp:2757`、`Patches/EntityConfigEx.cpp:286` | RestoreGameState 类、TriggerNewStage、SpawnCoPlayerBaby | 属于第 6、7 节的条件复活或附属 Player 入口 |

`Game::Spawn` 只有 `HookTest.cpp:100` 中被注释掉的测试 Hook，未见生效的官方 Hook。原生 Spawn 内的实体回调未展开（`UNKNOWN`）。

## 10. 有界调用图扫描说明（证据边界）

`closure-scan.json` 的扫描方法：从 `0x3CFC60` 出发，对直接 call 和跨函数 jmp 做 BFS，深度 ≤ 4。

- **跳过集合**（各自原因）：
  - EvaluateItems（`0x35D4B0`）：单独分析；
  - Game::Spawn（`0x2FA210`）：生成 Effect，回调未展开；
  - InitTwin（`0x3C8370`）、InitPostLevelInitStats（`0x3B3980`）：只在 `playerType==35` 时进入；
  - AddBoneHearts（`0x3C0B30`）：只在 HealthType==4 时进入；
  - KAGE::LogMessage（`0x5E0DF0`，签名匹配）：日志边会扩散到全局。
- **结果**：183 个函数，除跳过集合本身外没有命中目标，深度内的非库间接调用列于文件中。
- **不跳过 LogMessage 时**：深度 12 内经 `0x5E0DF0 → 0x62B090 → … → 0x4CB4F0` 到达游戏循环。同样，不跳过 ANM2 加载链时，深度 25 内经 `0x009680 → 0x5E6C70 → … → 0x4CB4F0` 也会扩散到 RemoveCoPlayer、SetControllerIndex 等。这说明有界扫描**不能**升级为全局闭包；结论只适用于第 2–4 节所述的直接语义路径。

## 11. UNKNOWN 汇总

1. 整个复活窗口的完整调用闭包：EvaluateItems 的全部下游、Game::Spawn 内的实体初始化与回调、`0x3B5CF0/0x3560C0` 的完整副作用、资源/日志/Lua 中枢边，以及所有间接调用。
2. 官方 Hook 的实际排列与回调启用状态，以及第三方/测试 mod 在 EvaluateCache、自定义 cache、音效、HealthType、服装回调中的改写。
3. `Appear` 动画的实际时长；射击 consumer 在 `0x373F62` 之后的查询点在 extra animation 期间是否执行。
4. 普通 Isaac 运行时的 `maxHearts`（决定心值分支），以及 `_backupPlayer` 是否为空（预期为空，未在运行时观察）。
5. 键 `0x100000` 对应哪一个 EntityConfig 项（推定为 Player）；`Effect 1000.15` 的具体名称；Slot variant 19 与 ReviveMenu 的正式名称；道具 146/179/667 的名称。
6. 换层与 Continue 是否经过 `Player::RestoreGameState`、`RestoreGameState_PostLevelInit` 或 `Player::Init`：它们直接改写 ghost 或 controller，属于 ST3/ST4 的范围。
7. 由 Morph 前段推出的 dead=0，要等 E2E 的实际观测确认。
