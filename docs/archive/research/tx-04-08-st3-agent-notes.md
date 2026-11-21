# tx-04-08 ST3 静态逆向代理笔记

> 2026-10-27 A项补正：原字段表的 `removed_by_factory +0x177` InitUnknown 漏掉基 Init VA0x6A4BD6的16位清零。后续原始证据与主审（已删除，原文 `39ddcfd6^:docs/design/tx-04-08/tx-04-08-abc-implementation.md`）已将layout改为InitConfirmed，并独立接入用户采纳的当前值资格。下面保留原ST3记录，不将旧UNKNOWN继续作为当前结论。

日期：2026-10-26  
目标：`isaac-ng-v1.9.7.12.exe.unpacked.exe`，IDA imagebase `0x400000`；SDK `F:/projects_isaac/REPENTOGON-1.1.3`。  
范围：旧 `min-read-v1` 的七类 Entity、Grid（共 78 个字段 ID）逐字段布局、宽度、初始化依据。这里只记录静态证据，不运行游戏、调试器、构建、测试、校验或哈希工具，也不修改 IDB。

## 证据规则与范围限定

- **旧源事实**：`cs-t0/native/src/world_record.cpp:1431-1461,1491-1511,1717-1721,1737-1762` 是旧 profile 的字段 ID、旧地址、宽度、编码和派生方式；它证明旧采集器如何读，不证明 J273 的布局或语义。
- **SDK 声明**：`REPENTOGON-1.1.3/libzhl/functions/*.zhl` 的成员偏移、类型、继承和总尺寸只代表目标 SDK 声明。未声明的偏移不因旧字段名而自动成为同一语义。
- **IDA 直接静态证据**：VA 来自目标 IDB，RVA=`VA - 0x400000`。寄存器写入的值只在指令确实是立即数或可追踪常量时写出；`mov [field], eax/esi` 只证明宽度与数据流入口，寄存器语义未确认时不赋予名称。
- **推导**：旧 ID 与 J273 目标字段按 SDK 成员、实际访问宽度和类继承关系逐字段比对；不使用整体偏移差推表。类的普通 Init/配置路径仅证明相应函数路径的写入；只有原生构造、配置加载和动态派生 Init 的正常 owner 均到达且正常返回，才可将这些写入组合为该对象的初始化依据。本文不闭合构造/恢复/复用/异常 owner 的完整生命周期。
- **UNKNOWN**：SDK 未命名、寄存器值含义不明、该字段没有本次静态查询中的明确初始化写入，或目标语义仍与旧名无法对应时，维持 UNKNOWN。没有找到写入不等于运行时未写入。
- 主审特别指出，`0x74DC70` 远处的 `0xA94FE3` `jmp` 属异常处理 chunk 的析构跳板，不能计作普通 Player 构造/析构出口；本笔记不以异常 chunk 补齐正常 owner。

## 可复查证据索引

| 证据 | 位置 / VA 与 RVA | 作用 |
| --- | --- | --- |
| 本代理原始 IDA 只读字段访问查询 | [ida-field-init-accesses.json](../../../artifacts/repentogon/tx-04-08-static/st3-agent/ida-field-init-accesses.json)；记录 module、imagebase、原始查询、函数 VA/RVA 与指令 | Entity base ctor/Init、Player ctor/Init、七类派生 Init、Grid Init 的原始命中。保留原查询输出；该筛选文件不覆盖每条访问，使用下项补全派生字段。 |
| 主审派生 Init/删除函数原始导出 | `artifacts/repentogon/tx-04-08-static/st3-st5-review/main-derived-init-delete.json` | 七类 Init 的指令与原始 bytes；Tear 128 位标志的四个 dword 写入等。 |
| 主审构造、析构正文及 Grid Init 原始导出 | `artifacts/repentogon/tx-04-08-static/st3-st5-review/main-constructors-dtor-bodies-gridinit.json` | Entity/Player 构造、Grid Init 指令和函数正文；区分普通代码与 EH chunk。 |
| 主审 Grid 构造、工厂和 GridSpawn 原始导出 | `artifacts/repentogon/tx-04-08-static/st3-st5-review/main-grid-constructor-factory.json` 与 `main-core-functions.json` | Grid base ctor `704FB0/304FB0`、工厂最终写 Type/Index，以及 Room GridSpawn 的 Variant/VarData 输入 store。 |
| 主审配置加载原始导出 | `artifacts/repentogon/tx-04-08-static/st3-st5-review/main-load-config.json` | 基类与 NPC 的配置加载路径，对通用 +384 原始值来源的补充证据。 |
| 主审 SubPlayer / GridPosition 只读导出 | `artifacts/repentogon/tx-04-08-static/st3-st5-review/main-subplayer-gridposition.json` | 目标 Lua `GetSubPlayer` 注册到 `+1D84` getter；目标 Lua `GetGridPosition` 注册到原生坐标函数，佐证旧公式与字段语义。 |
| 主审 Lua 字段注册及 Pickup Wait 访问导出 | `artifacts/repentogon/tx-04-08-static/st3-st5-review/main-field-lua-registration.json`、`main-pickup-wait.json` | Familiar FireCooldown、Pickup Wait/State、ProjectileFlags 的原生 Lua 名称注册与 getter/setter 对应字段访问。 |
| SDK 行号摘录 | `artifacts/repentogon/tx-04-08-static/st3-st5-review/main-sdk-source.json`；也可直接读上述 SDK 路径 | 目标声明与下表所列 SDK 行号。 |
| 主审对筛选漏项的补充指令 | `artifacts/repentogon/tx-04-08-static/st3-st5-review/main-derived-init-delete.json` 和 `main-constructors-dtor-bodies-gridinit.json` | 补齐本代理首轮字段访问筛选未命中的 `+16C`、Player `+1D84/+267C` 与 Familiar `+410` store。首轮过滤 JSON 是有限筛选结果，不是“无其他写入”的证明。 |
| 旧字段定义 | `cs-t0/native/src/world_record.cpp:1431-1461` | 旧七类 Entity、Grid ID/offset/width/encoding。 |
| 旧合成字段实现 | `cs-t0/native/src/world_record.cpp:1491-1511,1717-1721,1737-1762` | `001D/001E`、Grid `0810/0811` 与旧 Game frame/关系读取。 |

主要函数 VA/RVA：Entity ctor `6A3940/2A3940`；Entity Init `6A4A20/2A4A20`；Player ctor `74DC70/34DC70`；Player Init `74F6A0/34F6A0`；NPC Init `6B4A50/2B4A50`；Familiar Init `607AA0/207AA0`；Effect Init `5CD070/1CD070`；Pickup Init `6DD7C0/2DD7C0`；Tear Init `662F50/262F50`；Projectile Init `64F4B0/24F4B0`；Grid Init `705070/305070`。目标 Game 当前全局槽为 VA `0xBE49F4` / RVA `0x7E49F4`（主审直接确认）；其指向对象的 `_frameCount` SDK 偏移为 `Game.zhl:298` 的 `+0x25A70`。

## Entity 公共字段：`0001–001E`

SDK 基类字段见 `Entity.zhl:187-201,236-251`；`RNG.zhl:20-26` 将 RNG 明确声明为四个 `uint32_t`、总宽 0x10。全部七类声明继承 `Entity`，因此 +20、+28..、+33C 等基类成员偏移不因子类而改变。SDK 未命名 Entity `+0x328`、HP `+0x380/+0x384`；后两者仅保留用户已接受的通用 HP/maxHP 假设。

| ID | 旧 min-read-v1 | J273 对照 | 宽度 / 编码 | 初始化依据（仅静态）与结论 |
| --- | --- | --- | --- | --- |
| `0001` | `+20`, 4 | `Entity._index +20` (`Entity.zhl:187`) | 4 / Raw | Entity Init `6A4B1D/2A4B1D`: `mov dword [esi+20h],0`。 |
| `0002` | `+28`, 4 | `Entity._type +28` (`Entity.zhl:189`) | 4 / Raw | Entity Init `6A4A35/2A4A35`: `mov [esi+28h],eax`（Init 参数路径）。 |
| `0003` | `+2C`, 4 | `Entity._variant +2C` (`Entity.zhl:190`) | 4 / Raw | `6A4A3B/2A4A3B`: `mov [esi+2Ch],eax`。 |
| `0004` | `+30`, 4 | `Entity._subtype +30` (`Entity.zhl:191`) | 4 / Raw | `6A4A41/2A4A41`: `mov [esi+30h],eax`。 |
| `0005` | `+168`, 8 | `Entity._flags +168` (`Entity.zhl:196`) | 8 / Raw | Entity Init `6A4D7B/2A4D7B`: dword `[esi+168h]=0`；`6A4D85/2A4D85` bytes `C7 86 6C 01 00 00 00 00 00 00`: dword `[esi+16Ch]=0`。两条合计覆盖完整 64-bit flags 并清零；宽度仍由 SDK 确认。后者漏在首轮筛选结果中，原始导出见 `main-core-functions.json`。 |
| `0006` | `+170`, 1 | `Entity._valid +170` (`Entity.zhl:197`) | 1 / Raw | Entity ctor `6A39D4/2A39D4`: `mov byte [edi+170h],0`。后续变化未在此字段记录中闭合。 |
| `0007` | `+171`, 1 | `Entity._visible +171` (`Entity.zhl:198`) | 1 / Raw | Entity Init `6A4BCC/2A4BCC`: `mov dword [esi+171h],101h`；对应 byte 值 visible=1。物理写宽为 4，不能记成 byte store。 |
| `0008` | `+172`, 1 | `Entity._exists +172` (`Entity.zhl:199`) | 1 / Raw | ctor `6A39DB/2A39DB` byte 清零；Init 上述 dword `0x101` 覆盖至此，最终该 byte=1。 |
| `0009` | `+173`, 1 | `Entity._dead +173` (`Entity.zhl:200`) | 1 / Raw | Init `6A4BCC/2A4BCC` dword `0x101` 覆盖此 byte，最终该 byte=0；物理写宽为 4。 |
| `000A` | `+177`, 1 | `Entity._removedByFactory +177` (`Entity.zhl:201`) | 1 / Raw | SDK 宽度确认；本次 Entity ctor/Init 命中中未见该字节的明确直接初始化写入，初始化值 **UNKNOWN**。 |
| `000B` | `+33C`, 4 | `Entity._pos.x +33C` (`Entity.zhl:236`) | 4 / F32 | Entity ctor `6A3D76/2A3D76`: dword 0；随后 Init 读取 `6A4E37/2A4E37`。仅 ctor 初值，不是之后的位置恒为 0。 |
| `000C` | `+340`, 4 | `Entity._pos.y +340` (`Entity.zhl:236`) | 4 / F32 | ctor `6A3D6C/2A3D6C`: dword 0；Init 读取 `6A4E45/2A4E45`。 |
| `000D` | `+360`, 4 | `Entity._velocity.x +360` (`Entity.zhl:238`) | 4 / F32 | ctor `6A3DB2/2A3DB2`: dword 0；Init 读取 `6A4E51/2A4E51`。 |
| `000E` | `+364`, 4 | `Entity._velocity.y +364` (`Entity.zhl:238`) | 4 / F32 | ctor `6A3DA8/2A3DA8`: dword 0；Init 读取 `6A4E5D/2A4E5D`。 |
| `000F` | `+380`, 4 | SDK 无命名成员；原始 dword `+380` | 4 / F32（用户接受的通用 HP 假设） | 基 LoadConfig `6A500F/2A500F` 写 `movss [esi+384h],xmm0`；missing-config 路径 `6A51E3/2A51E3` 将 +384 写 0；Entity Init `6A4B37/2A4B37` 读 +384，`6A4B3D/2A4B3D` 写到 +380。Player Init 另在 `74FD7B/34FD7B` 写 `1.0f` 到 +380。NPC LoadConfig 路径 `6B7BF2/2B7BF2` 另写 +384。**这些写入不证明该通用值对每类都具有生命语义**，也不证明所有 owner 路径均已正常完成。 |
| `0010` | `+384`, 4 | SDK 无命名成员；原始 dword `+384` | 4 / F32（用户接受的通用 maxHP 假设） | 同上：普通配置分支写配置值，missing-config 写 0；Player Init `74FD85/34FD85` 直接写 `1.0f`；NPC 有派生 LoadConfig 覆写。保持通用 maxHP 假设，不将其升格为所有类的确认语义。 |
| `0011` | `+3EC`, 4 | `Entity._initSeed +3EC` (`Entity.zhl:251`) | 4 / Raw | Entity Init `6A4A47/2A4A47`: `mov [esi+3ECh],eax`（种子参数路径）。 |
| `0012` | `+3DC`, 4 | `Entity._dropRNG` 起点 `+3DC` (`Entity.zhl:250`; `RNG.zhl:22-26`) | 4 / Raw | ctor `6A3E7B/2A3E7B` 写初始常量 `0xAA17414F`；Entity Init `6A4A4D/2A4A4D` 再写 EAX。SDK RNG 包含四个 32-bit word、总宽 0x10；该 ID 是旧格式的首个 word。 |
| `0013` | `+3E0`, 4 | `Entity._dropRNG` 内 word `+3E0` (`RNG.zhl:22-26`) | 4 / Raw | ctor `6A3E6B/2A3E6B` `movq [edi+3E0h],xmm0`；Init `6A4A58/2A4A58` 同样写 8 字节覆盖 +3E0/+3E4。 |
| `0014` | `+3E4`, 4 | `Entity._dropRNG` 内 word `+3E4` (`RNG.zhl:22-26`) | 4 / Raw | 与 `0013` 相同的 8 字节写覆盖此 word；不能单独把该指令称作 4 字节 store。 |
| `0015` | `+3E8`, 4 | `Entity._dropRNG` 内 word `+3E8` (`RNG.zhl:22-26`) | 4 / Raw | ctor `6A3E73/2A3E73` 与 Init `6A4A60/2A4A60` 均写 dword。 |
| `0016` | `+3BC`, 4 | `Entity._parent` (`EntityPtr`) `+3BC` (`Entity.zhl:245`; `EntityPtr.zhl:11-17`, size 4) | 4 / Reference | ctor `6A3DF8/2A3DF8` 清零；Init `6A4DD3/2A4DD3` 再清零。 |
| `0017` | `+3C0`, 4 | `Entity._child` (`EntityPtr`) `+3C0` (`Entity.zhl:246`) | 4 / Reference | ctor `6A3E02/2A3E02` 与 Init `6A4DDD/2A4DDD` dword 清零。 |
| `0018` | `+3C4`, 4 | `Entity._target` (`EntityPtr`) `+3C4` (`Entity.zhl:247`) | 4 / Reference | ctor `6A3E0C/2A3E0C` 与 Init `6A4DE7/2A4DE7` dword 清零。 |
| `0019` | `+3C8`, 4 | `Entity._spawnerEntity` (`EntityPtr`) `+3C8` (`Entity.zhl:248`) | 4 / Reference | Entity ctor `6A3E16/2A3E16` dword 清零；本次 base Init 命中未见相同直接写入。后续引用更新不在此处闭合。 |
| `001A` | `+34`, 4 | `Entity._spawnerType +34` (`Entity.zhl:192`) | 4 / Raw | Entity ctor `6A3998/2A3998` dword 清零；只证明 ctor 初值。 |
| `001B` | `+38`, 4 | `Entity._spawnerVariant +38` (`Entity.zhl:193`) | 4 / Raw | Entity ctor `6A399F/2A399F` dword 清零；只证明 ctor 初值。 |
| `001C` | `+328`, 4 | SDK 无命名成员；`Entity+328` 原始 dword | 4 / Raw | Entity Init `6A4B2B/2A4B2B` 读 `[Game+25A70h]`，`6A4B31/2A4B31` 写到 `[Entity+328h]`。这是静态可见的当前帧 stamp；旧“出生计数”语义名未由 SDK 确认。 |
| `001D` | 旧 Game/root 对象 `+264F8`, 4；旧源先读 `image+0x871678` 槽 (`world_record.cpp:1718-1720`) | `Game._frameCount +25A70` (`Game.zhl:298`)，Game 指针来自目标全局槽 VA `BE49F4` / RVA `7E49F4` | 4 / Raw | J273 Entity Init `6A4B2B/2A4B2B` 对 `[eax+25A70h]` 的读取，以及 SDK Game.zhl:298，支持新 Game 字段；旧槽/偏移组合 `image+0x871678 → +264F8` 不得照搬。 |
| `001E` | 无对象偏移；旧派生 `game_frame - U32(Entity+328)` | 同一派生式；不映射新成员 | 4 / Derived | 旧源 `world_record.cpp:1508-1511`；`001D` 新来源为 Game+25A70，`001C` 仍是 +328 stamp。是 32 位差值，不是 Authority tick；wrap/时间回退策略由旧编码保持，本 ST 不改语义。 |

特别说明：SDK flags 为 8 字节，Entity Init 的已见 `+168` 写为 dword；`+171` 的写也是 dword。字段的**schema 宽度**来自 SDK 声明，不能从某个初始化指令的写宽推断 schema 宽度或完整初始化状态。

## Player 专用字段：`0101–0110`

旧源 `world_record.cpp:1442-1448` 的十六项全部声明 4 字节，尾部四项旧编码为 `Reference`。目标成员见 `EntityPlayer.zhl:736,746-747,778,780,814,818-823,830-831,861`；`EntityPtr` 在 `EntityPtr.zhl:11-17` 声明大小为 4。不同的旧地址逐项按成员声明/实际访问核对如下。

| ID | 旧 min-read-v1 | J273 对照 | 宽度 / 编码 | 初始化依据（仅静态）与结论 |
| --- | --- | --- | --- | --- |
| `0101` | `+13C0`, 4，旧语义 PlayerType | `Entity_Player._playerType +13BC` (`EntityPlayer.zhl:747`) | 4 / Raw | Player Init `74FC16/34FC16` 先对 +13BC 写 0；`74FCA2/34FCA2` 再把 ESI 参数写到 +13BC。旧 +13C0 不是 SDK `_playerType`；SDK 对 +13C0 未命名。 |
| `0102` | `+161C`, 4，旧语义 PlayerIndex | `Entity_Player._playerIndex +1610` (`EntityPlayer.zhl:778`) | 4 / Raw | Player ctor `74E091/34E091`: `mov dword [edi+1610h],FFFFFFFFh`，默认 -1。Init 中 `750684/350684` 把 3 写到 +161C，而 SDK 将 +161C 命名为 `_headDirection` (`EntityPlayer.zhl:780`)，故旧址不可继续当 PlayerIndex；管理器后续赋值/正常 owner 未在此处闭合。 |
| `0103` | `+13BC`, 4，旧语义 DamageCooldown | `Entity_Player._damageCooldown +13B8` (`EntityPlayer.zhl:746`) | 4 / Raw | Player Init `74FE5D/34FE5D` 对 +13B8 写 EAX；宽度为 dword，EAX 的初始化语义/值来源不在该 store 单独确认。旧 +13BC 是当前 SDK PlayerType。 |
| `0104` | `+1340`, 4，max hearts | `Entity_Player._maxHearts +1340` (`EntityPlayer.zhl:736`) | 4 / Raw | Player Init `74FDDF/34FDDF` dword 0。 |
| `0105` | `+1344`, 4，red hearts | `Entity_Player._redHearts +1344` (`EntityPlayer.zhl:736`) | 4 / Raw | Player Init `74FDE9/34FDE9` dword 0。 |
| `0106` | `+1348`, 4，eternal hearts | `Entity_Player._eternalHearts +1348` (`EntityPlayer.zhl:736`) | 4 / Raw | Player Init `74FDA3/34FDA3` dword 0。 |
| `0107` | `+134C`, 4，soul hearts | `Entity_Player._soulHearts +134C` (`EntityPlayer.zhl:736`) | 4 / Raw | Player Init `74FDD5/34FDD5` dword 0。 |
| `0108` | `+1350`, 4，black hearts | `Entity_Player._blackHearts +1350` (`EntityPlayer.zhl:736`) | 4 / Raw | Player Init `74FDAD/34FDAD` dword 0。 |
| `0109` | `+1D88`, 4，旧语义 BoneHearts | `Entity_Player._boneHearts +1D74` (`EntityPlayer.zhl:818`) | 4 / Raw | Player Init `74FD99/34FD99` 对 +1D74 写 0。旧 +1D88 没有对应 SDK 命名字段，不能按字段数或差值映射。 |
| `010A` | `+194C`, 4，golden hearts | `Entity_Player._goldenHearts +1938` (`EntityPlayer.zhl:814`) | 4 / Raw | Player Init `74FD71/34FD71` dword 0。 |
| `010B` | `+1DA0`, 4，broken hearts | `Entity_Player._brokenHearts +1D8C` (`EntityPlayer.zhl:821`) | 4 / Raw | Player Init `75140D/35140D` dword 0。 |
| `010C` | `+1DA4`, 4，rotten hearts | `Entity_Player._rottenHearts +1D90` (`EntityPlayer.zhl:822`) | 4 / Raw | Player Init `751417/351417` dword 0。 |
| `010D` | `+1D98`, 4，旧 SubPlayer Reference | `Entity_Player+1D84`，目标 Lua `GetSubPlayer` getter；`main-subplayer-gridposition.json` 注册点 `84FEBC/44FEBC` push getter `6D86D0/2D86D0`，紧随 `84FEC1/44FEC1` 注册名 `GetSubPlayer`；getter `6D86D0/2D86D0`: `mov eax,[ecx+1D84h]` | 4 / Reference | 语义与目标偏移由注册名+直接 getter 读证明；SDK `EntityPlayer.zhl` 未命名 +1D84。Player Init `74FD4D/34FD4D` 对 `[edi+1D84h]` 写 0，提供 Init 默认值依据；后续关联变化/完整 owner 仍未闭合。Init 的 `75163E/35163E` 在旧 +1D98 写 EAX，是另一个匿名 raw dword，不能认作当前 SubPlayer。该两条 +1D84/+1D98 store 均漏在首轮筛选结果，补充 raw 指令见 `main-derived-init-delete.json`。 |
| `010E` | `+1E68`, 4，twin Reference | `Entity_Player._twinPlayer (EntityPtr) +1E4C` (`EntityPlayer.zhl:830`; EntityPtr size4) | 4 / Reference | Player ctor `74E66C/34E66C` 对 +1E4C dword 清零；作为默认初始值证据，不说明后续 Twin 关系已建好。旧 +1E68 不是 SDK twin 成员。 |
| `010F` | `+1E6C`, 4，backup Reference | `Entity_Player._backupPlayer +1E50` (`EntityPlayer.zhl:831`) | 4 / Reference | Player Init `75157E/35157E` 对 +1E50 dword 清零。注意 +1E6C 在目标 SDK 未命名，不能沿旧地址读。 |
| `0110` | `+26C8`, 4，replaced Reference | `Entity_Player._replacedPlayer +267C` (`EntityPlayer.zhl:861`) | 4 / Reference | Player ctor 正常路径 `74E926/34E926` 写 `[esi+267Ch]=0`；其前 `74E917/34E917` 调 state ctor，随后该 store，再由 `74E966/34E966` 正常 `ret`。该地址在普通 ctor 尾部，不是 EH chunk。旧 +26C8 不得沿用。此写入也漏在首轮筛选结果，见 `main-constructors-dtor-bodies-gridinit.json`。 |

Player 旧/新专用字段中，除心量区间保持一致外，所有旧地址都发生了交叉错位或未命名；心量相同地址也仍要求 Player Init 正常 owner。旧 `0101 +13C0` 当前 SDK 未命名，不能与新 `_playerType` 混为一谈；旧 `0103 +13BC` 当前是 `_playerType`。SDK 的 `_headDirection +161C` 是显式反证，不是猜测。

## NPC、Familiar、Effect、Pickup、Tear、Projectile 专用字段

| 类 / ID | 旧 min-read-v1 | J273 对照 | 宽度 / 编码 | 初始化依据（仅静态）与结论 |
| --- | --- | --- | --- | --- |
| NPC `0201` | `+410`, 4，state frame | `_stateFrame +410` (`EntityNPC.zhl:128`) | 4 / Raw | NPC Init `6B4C7F/2B4C7F`: dword 0。 |
| NPC `0202` | `+B64`, 4，state | `_state +B64` (`EntityNPC.zhl:130`) | 4 / Raw | NPC Init `6B4C61/2B4C61`: dword 1。 |
| NPC `0203` | `+BA0`, 4，projectile cooldown | `_projectileCooldown +BA0` (`EntityNPC.zhl:131`) | 4 / Raw | NPC Init `6B4D65/2B4D65`: dword 0。 |
| NPC `0204` | `+BA4`, 4，projectile delay | `_projectileDelay +BA4` (`EntityNPC.zhl:132`) | 4 / Raw | NPC Init `6B4D6F/2B4D6F`: dword -1。 |
| Familiar `0301` | `+410`, 4，Player Reference | `_player` pointer `+410` (`EntityFamiliar.zhl:83`) | 4 / Reference | Familiar Init `607B43/207B43` 写入 `[edi+410h]=eax`。前序 `607B26/207B26` 读取 spawner `+3C8`；type==1 分支沿用该值，另一分支于 `607B3E/207B3E` 调 `4131B0/131B0` 后再 store。字段写入确定，分支数据流见 `main-derived-init-delete.json`。该 store 漏在首轮筛选结果。 |
| Familiar `0302` | `+D4C`, 4，旧名 fire cooldown | `Familiar+ D4C` SDK 无命名成员；目标 Lua `EntityFamiliar` 注册 `FireCooldown` | 4 / Raw | Familiar Init `607C0D/207C0D`: `mov dword [edi+D4Ch],0`。语义由类注册 `8511B6/4511B6` 和字段注册序列 `851267/451267` push `0D4Ch`、`85126C/45126C` push `FireCooldown` 确认；SDK 仍未声明该成员，Init 值为 dword 0。 |
| Familiar `0303` | `+D88`, 4，state | `_state +D88` (`EntityFamiliar.zhl:88`) | 4 / Raw | Familiar Init `607EEE/207EEE`: dword 0。 |
| Effect `0401` | `+450`, 4，state | `_state +450` (`EntityEffect.zhl:50`) | 4 / Raw | Effect Init `5CD1DD/1CD1DD`: dword 0。 |
| Effect `0402` | `+45C`, 4，timeout | `_timeout +45C` (`EntityEffect.zhl:51`) | 4 / Raw | Effect Init `5CD1E7/1CD1E7`: dword -1。 |
| Effect `0403` | `+460`, 4，lifespan | `_lifespan +460` (`EntityEffect.zhl:52`) | 4 / Raw | Effect Init `5CD1F1/1CD1F1`: dword 1。 |
| Pickup `0501` | `+540`, 4，timeout | `_timeout +540` (`EntityPickup.zhl:143`) | 4 / Raw | Pickup Init `6DD82F/2DD82F`: dword -1。 |
| Pickup `0502` | `+544`, 4，旧名 wait | `Pickup+544` SDK 无命名成员；目标 Lua `EntityPickup` 注册 `Wait` | 4 / Raw | Pickup Init `6DD839/2DD839`: dword 0。原生 getter `844A60/444A60` 读 `[ecx+544h]`，setter `4318C0/318C0` 写 `[ecx+544h]`；EntityPickup 类注册 `850EC7/450EC7` 下，字段注册点 `850F40/450F40` 为 `Wait`，getter/setter 分别为 `844A60/444A60`、`4318C0/318C0`。旧语义由类绑定和直接访问确认，SDK 未命名该成员。 |
| Pickup `0503` | `+584`, 4，旧名 state | `Pickup+584` SDK 无命名成员；目标 Lua `EntityPickup` 注册 `State` | 4 / Raw | Pickup Init `6DD8A1/2DD8A1`: dword 0。EntityPickup 类注册 `850EC7/450EC7` 下，`850EEB/450EEB` push `584h`、`850EF0/450EF0` push `State`，确认旧名对应目标 Lua 字段；SDK 未命名该成员，不能因名称相同套用 Effect/NPC State 的成员布局。 |
| Tear `0601` | `+428`, 16，旧名 TearFlags | `Entity_Tear._tearFlags BitSet128 +428` (`EntityTear.zhl:71`) | 16 / Raw | Tear Init `663092/263092`, `66309C/26309C`, `6630A6/2630A6`, `6630B0/2630B0` 各写 4 字节 0，连续覆盖 `+428..+437` 全 128 位。SDK 类型与宽度匹配。Tear 的额外 State/Timeout 仍未确认，不在此补字段。 |
| Projectile `0701` | `+438`, 8，旧名 ProjectileFlags | `Entity_Projectile` SDK 未声明 +438 字段（结构大小 `0x470`，`EntityProjectile.zhl:13-46`）；目标 Lua 注册 `ProjectileFlags` | 8 / Raw | Projectile Init `64F512/24F512` 对 +438 写 dword 0，`64F52B/24F52B` 对 +43C 写 dword 0，覆盖 8 字节。`EntityProjectile` 类注册点 `8514A1/4514A1` 下，`8514DE/4514DE` push setter `442D70/42D70`、`8514E3/4514E3` push getter `413170/13170`、`8514E8/4514E8` push `ProjectileFlags`；getter 指令 `413170/13170` 读 `[ecx+438h]`，setter `442D70/42D70` 写 +438 与 +43C 两个 dword。故 Lua 名称与 8-byte 宽度有直接绑定证据，SDK 仍未声明成员；其他 State/Timeout **UNKNOWN**。 |

`0201-0204`、`0303`、`0401-0403` 和 `0501` 的语义/位置分别有对应目标 SDK 成员声明。Pickup `+544/+584`、Familiar `+D4C`、Projectile `+438..+43F` 在 SDK 中没有对应成员声明；其旧 schema 名称分别由目标 Lua 原生注册和直接 getter/setter 访问静态确认。这里把 Lua 绑定证据与 SDK 声明分开记录，不把它们提升成 SDK 命名成员。

## Grid 独立布局：`0801–0811`

目标声明：`GridEntityDesc.zhl:1-11`（大小 `0x20`），`GridEntity.zhl:5,25-35,59`（包括 `GridEntity::Init(unsigned int Seed)`、成员偏移和对象大小 `0x154`），`RNG.zhl:20-26`（四 word/0x10 bytes）。`GridEntity::_desc` 从对象 `+4` 开始，所以 Desc 内成员偏移需加 `+4`；Grid 不是 Entity 派生布局，不读取 Entity HP/flags。

| ID | 旧 min-read-v1 | J273 对照 / 对象绝对偏移 | 宽度 / 编码 | 初始化依据（仅静态）与结论 |
| --- | --- | --- | --- | --- |
| `0801` | `+4`, 4，Desc Type | `_desc._type`；Desc 内 `0`, object `+4` (`GridEntityDesc.zhl:3`, `GridEntity.zhl:30`) | 4 / Raw | Base ctor `704FD0/304FD0` 写 dword 0；factory `706636/306636` 的 `89 7E 04` 将请求 type 写入 `[esi+4]`。默认值和 factory 输出值均有直接证据。 |
| `0802` | `+8`, 4，Desc Variant | `_desc._variant`；内 `+4`, object `+8` (`GridEntityDesc.zhl:4`) | 4 / Raw | Base ctor `704FD7/304FD7` 写 dword 0；Room GridSpawn `7E2257/3E2257` bytes `89 47 08` 将输入值写 `[edi+8]`，之后 `7E2260/3E2260` 调 Grid Init `705070/305070`。 |
| `0803` | `+C`, 4，Desc State | `_desc._state`；内 `+8`, object `+C` (`GridEntityDesc.zhl:5`) | 4 / Raw | Base ctor `704FDE/304FDE` bytes `C7 46 0C 00 00 00 00` 写 dword 0。后续派生状态更新不在本字段初始化闭包内。 |
| `0804` | `+10`, 4，Desc SpawnCount | `_desc._spawnCount`；内 `+C`, object `+10` (`GridEntityDesc.zhl:6`) | 4 / Raw | Base ctor `704FE5/304FE5` 写 dword 0；Grid Init `70508D/30508D` 执行 `inc dword [esi+10h]`，故该路径在此前置初值上增加一次。 |
| `0805` | `+14`, 4，Desc SpawnSeed | `_desc._spawnSeed`；内 `+10`, object `+14` (`GridEntityDesc.zhl:7`) | 4 / Raw | Base ctor `704FEC/304FEC` 写 `0x16A9DE81`；Grid Init `705079/305079` 执行 `mov [esi+14h],eax`，从 Seed 参数路径覆写为运行时输入值。 |
| `0806` | `+18`, 4，Desc VarData | `_desc._varData`；内 `+14`, object `+18` (`GridEntityDesc.zhl:8`) | 4 / Raw | Base ctor `704FF3/304FF3` 写 dword 0；Room GridSpawn `7E225D/3E225D` bytes `89 47 18` 将输入值写 `[edi+18]`，后于该 store 在 `7E2260/3E2260` 调 Grid Init。注意 Grid Init 的 +14 store 对应 SpawnSeed，不是 VarData。 |
| `0807` | `+1C`, 1，Desc Initialized | `_desc._initialized`；内 `+18`, object `+1C` (`GridEntityDesc.zhl:9`) | 1 / Raw | Base ctor `704FFA/304FFA` 执行 `mov byte [esi+1Ch],0`；Grid Init `705090/305090` 执行 `mov byte [esi+1Ch],1`，宽度和值明确。 |
| `0808` | `+20`, 4，Desc VariableSeed | `_desc._variableSeed`；内 `+1C`, object `+20` (`GridEntityDesc.zhl:10`) | 4 / Raw | Base ctor `704FFE/304FFE` 写 `0x16A9DE81`。本次所查 ctor/factory/Spawn/Init 范围内未见后续写入；不把 ctor 默认值当作 owner 生命周期终值。 |
| `0809` | `+24`, 4，GridIndex | `_gridIndex +24` (`GridEntity.zhl:31`) | 4 / Raw | Base Grid factory `706639/306639` 的 `89 5E 24` 将其 index 输入写入 `[esi+24h]`；Base ctor 不直接初始化该成员。 |
| `080A` | `+28`, 4，SpawnFrame | `_spawnFrame +28` (`GridEntity.zhl:32`) | 4 / Raw | Grid Init `705094/305094` 读 `[Game+25A70h]`；`70509A/30509A` 写 dword `[esi+28h]`。新 Game frame 字段在 `Game.zhl:298`。 |
| `080B` | `+2C`, 4，RNG raw word 0 | `_rng` 起点 `+2C` (`GridEntity.zhl:33`; `RNG.zhl:22-26`) | 4 / Raw | Base ctor `70500A/30500A` bytes `C7 46 2C 4F 41 17 AA` 写 dword `0xAA17414F`。 |
| `080C` | `+30`, 4，RNG raw word 1 | `_rng` 内 `+30` (`RNG.zhl:22-26`) | 4 / Raw | Base ctor `705005/305005` 执行 `movq [esi+30h],xmm0`；此前 `704FB4/304FB4` 从 `qword_AACE4C` 加载由主审确认的 `{5,9}` 两个 dword，故 +30=5。 |
| `080D` | `+34`, 4，RNG raw word 2 | `_rng` 内 `+34` (`RNG.zhl:22-26`) | 4 / Raw | 同一 `705005/305005` 的 8-byte `movq` 覆盖 +30/+34；加载常量的第二 dword 为 9，故 +34=9。 |
| `080E` | `+38`, 4，RNG raw word 3 | `_rng` 内 `+38` (`RNG.zhl:22-26`) | 4 / Raw | Base ctor `705011/305011` 执行 `mov [esi+38h],eax`；ctor 数据流/反编译给出先前加载的常量 7，值有静态依据。RNG 总范围 +2C..+3B 共 16 字节，SDK 类型布局与旧四 word 匹配。 |
| `080F` | `+3C`, 4，CollisionClass | `_collisionClass +3C` (`GridEntity.zhl:34`) | 4 / Raw | Grid Init `70507E/30507E`: dword 0；随后 `705085/305085` 调用 vtable `+28`，所以 Init 函数内的这个零值不是该调用返回后的最终值保证。 |
| `0810` | 不存在原生偏移；旧采集派生 x | `GetGridPosition` 的 Grid world-position x；非 GridEntity 成员 | 4 / F32 | 旧 `world_record.cpp:1757-1761` 的 `x=40*(i%width)+40` 与目标 getter 相同。`main-subplayer-gridposition.json`：注册点 `852F99/452F99` push 原生 `8087A0/4087A0`，`852F9E/452F9E` 注册名 `GetGridPosition`；函数 `8087A0/4087A0` 将 index 除/模 width，乘 40.0 再加 40.0，写到返回 Vector 的 x。函数只用 grid index、width 和常量，未见 render transform；getter 是静态佐证，旧 recorder 自身仍按旧公式计算，不实际调用 getter。 |
| `0811` | 不存在原生偏移；旧采集派生 y | `GetGridPosition` 的 Grid world-position y；非 GridEntity 成员 | 4 / F32 | 旧 `world_record.cpp:1757-1761` 的 `y=40*(i/width)+120` 与目标 getter 相同；同一函数在 `8087D1/4087D1` 乘 40.0，`8087D5/4087D5` 加 120.0，`8087DD/4087DD` 写返回 Vector 的 y。函数只用 grid index、width 和常量，未见 render transform；getter 不由旧 recorder 调用，不将结果误称成对象内存字段。 |

Grid base ctor `704FB0/304FB0`、factory `706100/306100` 的最终 type/index stores、Room GridSpawn `7E2257/3E2257` 与 `7E225D/3E225D`，以及 Grid Init `705070/305070` 的原始指令和 bytes 分别见 `main-grid-constructor-factory.json`、`main-core-functions.json` 与 `main-constructors-dtor-bodies-gridinit.json`。这些分函数证据补齐了 Descriptor、RNG 和 GridIndex 的初值/输入写入依据；它们仍不足以闭合 ST6 的全部创建、恢复、复用和派生 vcall 后 owner。Grid Init 的直接动作包括 +14（Seed 参数覆写）、+3C（先清零）、+10（SpawnCount++）、+1C（Initialized=1）、+28（Game frame）。不要把 +14 叫成 VarData；+3C 在 vcall `705085/305085` 前清零，调用返回后的最终值仍未由该 store 定论。

## 结论与未决项

1. 78 个字段 ID 的旧→新地址/宽度对照已逐字段列出。Entity 公共大部分地址保持不变；Game frame `001D` 从旧对象 `+264F8` 改为 J273 `Game+25A70`。Player 十六项中多项地址重排，`010D` 由目标 `GetSubPlayer` 注册与 getter 确认映射到 `+1D84`，不能沿用旧 +1D98。Grid 的 Descriptor 绝对地址通过 `GridEntity::_desc +4` 逐项推导。
2. Entity HP/maxHP 仍是历史接受的通用假设。新增静态证据把“raw word 有没有初始化写入”与“是否具有通用生命语义”分开；Entity base Init 对 `+384 -> +380` 的复制不单独证明后者。NPC 派生 LoadConfig 的 `+384` 写入也不外推成其他类的 owner 证明。
3. `010D SubPlayer` 的目标语义/偏移由 `GetSubPlayer` 注册和 `6D86D0` getter 读 `[ecx+1D84h]` 静态确认；Player Init `74FD4D` 对该地址写 0，后续实际关联变化/完整 owner 未闭合。Familiar +D4C、Pickup +544/+584、Projectile +438 的旧名由 Lua 注册/getter/setter 对应关系确认，但这些成员仍未出现在 SDK 声明中。`0110` replaced 当前 SDK 字段已定位，普通 Player ctor 尾部 `74E926` 将其置 0。
4. NPC/Effect/Familiar state 与 Effect/Pickup timeout 的已命名成员按 SDK 保留；Player/Tear/Projectile 的未确认 State/Timeout 不新增、不补零、不套用其他类别布局。Familiar Player 指针 Init 写入与分支已静态确认，但运行时值和关联生命周期，PlayerIndex 之后的分配、保留关系和恢复路径均不在本 ST 闭合。
5. Grid Descriptor/RNG 的旧→新布局按 `_desc +4` 和 `RNG +2C` 逐字段对应。base ctor 对 Desc 与 RNG 的默认写入、factory 的 Type/Index、Room GridSpawn 的 Variant/VarData 和 Grid Init 的 Seed/SpawnCount/Initialized/Frame 写入均有独立静态依据；尚未闭合创建、恢复、复用及派生 vcall 后的完整 owner。`080F` 在虚调用前清零，虚调用后值未从本指令单独定论。`0810/0811` 是旧采集器按公式计算的派生坐标；与 J273 的 GetGridPosition 原生实现静态一致，但不映射到 SDK 对象成员，旧采集器未调用 getter。

本笔记完成 ST3 静态字段对照；未做任何运行或校验。剩余 UNKNOWN 已按字段列出，不扩展为 ST6/ST7 生命周期 owner 闭包。
