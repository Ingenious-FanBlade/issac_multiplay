# tx-04-08 ST4 静态逆向代理笔记

状态：ST4 静态来源与边界已完成；SubPlayer 名称/偏移及 GridPosition 派生公式已由主审补充原生证据确认。保留对象资格和 raw Position 映射等未决项为 `UNKNOWN`，不据此扩大到 ST6/ST7。

## 结论先行

**J273 的 Room GridEntity 固定槽数是 448，索引范围 `0..0x1BF`，不是 447。** 官方 `Room.zhl` 把成员写成 `_gridEntities[0x1BF]`，按 C++ 数组长度解释是 447；但目标原生 `Room::GetGridEntity`、`Room::SpawnGridEntity`、`Room::RemoveGridEntityImmediate` 都将 `0x1BF` 作为包含式最大索引接受，并直接以 `Room+0x24+index*4` 访问。Room 构造函数又从 `Room+0x24` 清零到 `Room+0x744`，其长度 `0x720 = 448*4 + 8*4`，即 448 个 Grid 指针后紧接 8 个 door 指针。**故迁移时不能照抄 SDK 声明长度，也不能把最后有效槽 447 丢掉。**

目标 IDB 为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，imagebase `0x400000`；下文同时给 VA/RVA。IDB 查询只读。目标 SDK 为 `F:/projects_isaac/REPENTOGON-1.1.3`。本任务没有执行测试、构建、校验脚本或运行时操作。

## 完整来源表

`world_record.cpp` 现有原型给出来源 ID；ST4 的静态复核表如下。来源成员关系必须独立保存，多个来源指向同一地址时只合并对象身份，不丢来源和 index。

| Source ID | 来源 | 目标地址 / 形状 | 资格与迁移注意 |
| --- | --- | --- | --- |
| 0 | Main EL | `Room+0x1218` 的 `EntityList` 起点再 `+0x20`，descriptor `Room+0x1238`，`+0x10` 步进 | 视图成员；不能单凭在 EL 中即认定世界唯一或新生 |
| 1 | Persistent EL | `Room+0x1248` | 同上 |
| 2 | Update EL | `Room+0x1258` | 同上 |
| 3 | Render EL | `Room+0x1268` | 同上 |
| 4 | Effect EL | `Room+0x1278` | 同上 |
| 5 | Temp EL | `Room+0x1288` | 暂存成员不是独立出生事实 |
| 6 | BufferTemplate EL | `Room+0x1298` | 模板视图不自动取得 active 世界资格 |
| 7 | Wisp EL | `Room+0x12A8` | 同一实体可与其他 EL 重叠，保留多个 membership |
| 8 | Player roster | `Game+0x1BAA8`，`std::vector<Entity_Player*>` 三指针 descriptor | `Game._playerManager` 在 `+0x1BAA8`，其 `_playerList` 位于 manager `+0`；遍历以 end-begin 为 size，不能硬编码玩家数量 |
| 9 | Esau Jr state | `Game+0x1BB28 = PlayerManager+0x80`，`Entity_Player*[4]` | 四个保留槽分别留 index；并入身份表不表示 roster/active 成员 |
| 10 | Player 保留关系 | 对已识别 Player 的候选引用；旧原型读取 `{+0x1D98,+0x1E68,+0x1E6C,+0x26C8}` | 旧偏移不迁移为 J273；详见“Player 关系” |
| 11 | GridEntity 槽 | `Room+0x24`，448 个 `GridEntity*`，索引 `0..447` | 固定上限 448；当前房间地图尺寸仍需单独约束坐标推导 |
| 12 | Door 槽 | `Room+0x724`，8 个 `GridEntity*`，slot `0..7`；关联 grid index 表在 `Room+0x744`，8 个 `uint32_t` | Door 指针可同时存在于 Door 槽和 Grid 槽，属于别名；按实体地址去重但保留两个 membership |
| 13 | Reserved | 原型 `NOT_APPLICABLE` | 保持显式 N/A，不能冒充空来源 |
| 14 | Reserved | 原型 `NOT_APPLICABLE` | 同上 |

来源表达式来自 `cs-t0/native/src/world_record.cpp:1720-1729,1734-1748`；EL 名称与布局来自 SDK `libzhl/functions/EntityList.zhl:19-61`、`Room.zhl` 的 `_entityList` 成员和 `PlayerManager.zhl:68-74`、`Game.zhl:290`。`Game` 全局根指针槽由主审在 `0x6A4B06` 指令确认写/读到 VA `0xBE49F4`（RVA `0x7E49F4`）；运行帧字段为 `Game+0x25A70`。这两个 root/frame 点位是主审独立证据，来源表使用的相对成员偏移另由 SDK 与原型源码确认。

## Descriptor 的 size、capacity 与边界

- SDK `EntityList_EL.zhl`：descriptor 为 16 bytes，`_sublist bool +0`、`_data Entity** +4`、`_capacity +8`、`_size +0xC`。原型 `CopySource` 对 EL descriptor 读 16 bytes，以 `data=word[1]`、`count=word[3]`，要求 `count<=capacity`、非零 count 必须有 data、data 四字节对齐；逐槽读取 4 字节指针，并受 `kMembershipBudget`、乘法及加法溢出界限保护。descriptor 与槽指针复制后会复读比对，以发现采集窗口内变更。对应源码 `world_record.cpp:1551-1588`。
- roster 为 12-byte `std::vector<Entity_Player*>`（SDK `Global.zhl:82`，`ExtraTypes:197-200`）。begin/end/capacity 三个指针全零表示空；非空时原型要求非零、`begin<=end<=capacity`、三个地址四字节对齐，并以 `(end-begin)/4` 得到 size。**size 必须来自 end-begin，不是 capacity-begin**。对象数组地址和 descriptor 也需在复制前后复核。对应源码 `world_record.cpp:1555-1563`。
- 目标 `PlayerManager::GetPlayer` VA `0x98F930` / RVA `0x58F930` 从 vector begin/end 差值计算元素数（`0x98F93C-0x98F940`、`0x98F95E-0x98F963`）；它没有给观察器提供对损坏 descriptor 的通用安全保证。观察器仍应独立校验三指针顺序、对齐、溢出和可读区间。
- 固定数组不按 descriptor 动态长度处理：Grid 上界 448；Door 上界 8；Esau 数组上界 4。Room::MakeDoor 的主体没有足以取代观察器边界校验的完整 slot/index 检查；采集端必须先验证 `doorSlot<8`，再验证关联 grid index `<448`。
- 列表中空指针要作为来源槽中的 `NOT_READY`/空成员事实记录，不能静默压缩 index。若复制期列表或指针变化，批次应报告 source changed，而不能将一半新旧成员混成稳定快照。

## 448 与 SDK 447 的差异：原生证据

| 证据 | VA / RVA | 指令/范围 | 结论 |
| --- | --- | --- | --- |
| `Room::GetGridEntity`，SDK `Room.zhl:14` 签名 | `0x431710 / 0x031710` | `0x431716 cmp eax,1BFh`; `0x43171B ja 0x431725`; `0x43171D mov eax,[ecx+eax*4+24h]` | 只有 `index>447` 才拒绝；index 447 被实际解引用 |
| `Room::SpawnGridEntity`（GridSpawn） | `0x7E21D0 / 0x3E21D0` | `0x7E21F6 cmp esi,1BFh`; `0x7E21FC ja failure`; `0x7E223D mov ecx,[ebx+esi*4+24h]`; `0x7E2253 mov [ebx+esi*4+24h],edi` | publish/replace 允许并访问 slot 447；前面的 `cmp esi,1C0h` 是另一条范围/调试条件，不能误读成 447 slots |
| `Room::RemoveGridEntityImmediate` | `0x813BD0 / 0x413BD0` | `0x813BD9 cmp edx,1BFh`; `0x813BDF ja`; `0x813BE1 mov ecx,[esi+edx*4+24h]`; `0x813BFF mov [esi+edx*4+24h],0` | free/remove 也将 447 当合法 index |
| `Room::constructor` | `0x7DF8F0 / 0x3DF8F0` | `0x7DFFF0 push 720h`; `0x7DFFF5 lea eax,[edi+24h]`; `0x7DFFFC push 0`; `0x7DFFFF call memset` | 清零 `[Room+0x24, Room+0x744)` 共 `0x720` bytes，即 448 个 Grid 指针 (`0x700`) + 8 个 Door 指针 (`0x20`)。此清零范围是交叉证据；本结论仍以三个访问器的包含式边界为准 |

SDK 的 `_gridEntities[0x1BF]`（`Room.zhl:184`）可解释为少写了一个元素；跟随它按 447 长度复制会漏掉原生合法的 `0x1BF` 槽。目标布局应登记 **count=448, maxIndex=0x1BF**，并另存“SDK 声明长度与 native 实际界限不一致”的说明。

## Door 来源、Grid 别名与保留规则

- Room SDK 声明 `_gridEntities` 从 `+0x24`、`_doors[8]` 从 `+0x724`、`_doorGridPositions[8]` 从 `+0x744`（`Room.zhl:184-187`）。三者边界连续，不可把 Door 槽误当作 Grid 数组尾部。
- `Room::MakeDoor` VA `0x7E4DE0` / RVA `0x3E4DE0`：`0x7E4E21` 读取 `[Room+doorSlot*4+0x744]` 得 grid index；`0x7E4E4D` 读取对应 `[Room+gridIndex*4+0x24]`；路径 `0x7E4F7B` 可将 door object 写回 Grid 槽；`0x7E4FE7` 再写 `[Room+doorSlot*4+0x724]`。因此一个 door 对象可同时由 source 11 和 source 12 观察。要按 `(domain,address)` 合并对象身份，membership 保留 `(Grid,gridIndex)` 与 `(Door,doorSlot)`；这不是两次出生。
- `Room::RemoveGridEntityImmediate` 对某些 `GridEntity` 类型为 1 且 `KeepDecoration` 非零的路径保留原槽指针；否则清空 Grid 槽并走释放调用。故 `GridFree` 请求本身不等价于对象已退休；采集观察到实际 slot 变化/析构/生命周期 owner 后才可更新身份。当前 ST4 只把它作为保留资格约束，不替代 ST5/ST6 生命周期规则。
- Door slot 与 `DoorGridPositions[slot]` 是不同索引域。两个数组都要按各自范围验证；Door 指针自身的 slot alias 不证明关联 Grid 槽有效，关联值必须单独界限检查。

## Player roster、Esau 与四类保留关系

SDK `Game.zhl:290` 把 `PlayerManager` 放在 `Game+0x1BAA8`；`PlayerManager.zhl:70-74` 声明 `_playerList` 在 manager `+0`，`_esauJrState[4]` 在 manager `+0x80`，所以两个旧原型源地址分别是 `Game+0x1BAA8` 与 `Game+0x1BB28`。`EntitySaveStateManagement.cpp:223-251` 也独立遍历 manager roster、每个 Player 的 backupPlayer 和四个 Esau 状态槽，证明这些被独立保存/枚举；这不等于所有对象都在当前 active roster。

| 关系 | J273 证据与状态 | 原型迁移结论 |
| --- | --- | --- |
| SubPlayer | 主审独立回读 Lua 注册：VA `0x84FEBC` push `sub_6D86D0`、`0x84FEC1` push 字符串 `GetSubPlayer`；getter VA `0x6D86D0` / RVA `0x2D86D0` 为 `mov eax,[ecx+1D84h]; ret`。此外 `0x7B9FD0` 和 `0x7C1F40` 读写该指针 | **确认 J273 SubPlayer getter 指向 Player+0x1D84**。SDK EntityPlayer.zhl 未列名成员，但原生 getter/API 名与直接读取已闭合。其对象在各生命周期阶段是否仍有记录资格仍 UNKNOWN |
| Twin | SDK `EntityPlayer.zhl:830` 声明 `EntityPtr _twinPlayer +0x1E4C`；`0x7B9FD0` 在 `PlayerType==0x28` 路径读取它 | 目标字段可直接迁移为 Twin 原始 pointer wrapper；复制 wrapper 内实际对象地址时保留原始 32-bit 值和来源 |
| Backup | SDK `EntityPlayer.zhl:831` 声明 `Entity_Player* _backupPlayer +0x1E50`；REPENTOGON Lua/存档代码也直接使用 | 迁移地址 `+0x1E50`，但备份对象 membership 与当前 roster membership 分开 |
| Replaced | SDK `EntityPlayer.zhl:861` 声明 `_replacedPlayer +0x267C`，注释为上次 `PlayerManager::ReplacePlayer` 的另一 Player | 迁移地址 `+0x267C`；保留历史关系身份，不能仅因字段非空将目标提升为 active Player |

旧原型 `kPlayer` 的四个 `Encoding::Reference` 偏移是 `+0x1D98,+0x1E68,+0x1E6C,+0x26C8`（`world_record.cpp:1444-1448,1734-1742`）。这些是 J460 原型偏移；不应按相似字段/整段偏移差迁移到 J273。特别是目标 J273 `+0x1D98` 被原生函数作数字用：`0x7BA007 cmp dword ptr [ecx+1D98h],63h`（RVA `0x3BA007`），且其他代码按数值增减/比较；它不能作为 SubPlayer 指针。原型关系字段应重新按 J273 证据布局，不能沿用上述四个旧 offset。

资格策略（静态结论与推导分开）：

- **事实：** roster、Esau array、EL、关系指针、Grid、Door 都是不同来源。REPENTOGON 保存状态处理器分别遍历 roster、backup、Esau 槽；原生 MakeDoor 可跨 Door/Grid 槽别名。
- **推导：** 来源命中应只建立 membership；是否为当期可记录对象仍要通过 ST5 已确认分类、目标当前身份/use、生命周期与一致性证据判断。保留 Player 关系按引用观察，不因该字段本身非空建立出生或 active membership。Roster 中的 Player、Esau 缓存、backup、replaced 都应保留其来源标签；除确认是同一个物理对象外不折叠来源。
- **UNKNOWN：** Esau 四指针在各具体游戏状态下是否一定有效；SubPlayer/backup/replaced 被引用对象何时仍可安全采集。ST4 不替 ST5 的分类闭包或 ST6/ST7 的生命周期撤销作结论。

## 坐标推导

- **主审补充原生证据：** Lua 注册 VA 0x852F99 push sub_8087A0、0x852F9E push 名称 GetGridPosition；getter VA 0x8087A0 / RVA 0x4087A0 直接按 idx%width*40+40 与 idx/width*40+120 构造并返回 Vector，无 render 变换。因此旧原型公式与当前原生 GetGridPosition 语义一致；这仍不证明 raw Entity Position 字段等于该 Vector。
- 目标 `Room::GetGridIndexByTile` VA `0x808700` / RVA `0x408700` 检查 column 对 `Room+0x0C` width、row 对 `Room+0x10` height 的范围，返回 `column + row*width`（指令 `0x80871B cmp ecx,[esi+10h]`、`0x808720 imul eax,ecx`、`0x808724 add eax,edx`）。Grid slot index 是 row-major tile index。
- 目标 `GridEntity::GetRenderPosition` VA `0x705110` / RVA `0x305110` 从 `[Grid+0x24]` 取 index，以当前 Room width 做整数除法得到 row/column（`0x705116`, `0x705147`），随后按 tile step/basis 常量乘 40，并加入旧原型使用的 x/y 基点 40/120；后面又执行 render/camera/scale/floor 变换。这证明格点的 row/column 推导和 tile 坐标基底，**不能直接证明 Entity 的 raw world-position 字段等于最终公式**。
- 旧原型 `world_record.cpp:1757-1760` 用 `x=40*(i%width)+40`、`y=40*(i/width)+120` 为对象写派生坐标。该公式现已由原生 GetGridPosition 直接证实；它仍是从 Grid index 推导的值，不是读取 Entity raw position。Room::GetGridIndexByTile 另行使用 width/height 检查 row/column 输入，不应把该反向 API 的 height 条件套到 GetGridPosition 或用来缩减固定 448 Grid 槽。
- **修正初报建议：** 原生 GetGridPosition 只使用 Grid index 与 width，不读取 height；因此不得把 `i<width*height` 作为 GridPosition 的资格前置条件，也不得按房间形状缩小 448 个槽来源。派生坐标前应保证 width 非零且有界、index 在 `0..447`、对象与该 Grid 槽的对应关系在本批次唯一且前后稳定；height 可作为独立诊断字段，不作为公式边界。该 GridPosition 与 raw Entity Position 的对应关系仍为 **UNKNOWN**，输出应标明是按 index 派生。

## 给原型迁移的直接注意

1. Grid `count=448`、`maxIndex=447`；把 SDK `[0x1BF]` 声明错误写进 manifest 会漏有效尾槽。Door 另计 8；`+0x744` door-to-grid index 是第三段数组。
2. 八 EL 使用 SDK 当前的 16-byte descriptor、实际 `_size` 和 `_capacity`，list count 不能由 capacity 代替；Player roster 使用 vector begin/end/capacity。所有动态范围要有对齐、顺序、溢出、预算、内存读取和复制稳定性约束。
3. Grid/door 与 roster/Esau/关系可以别名；按地址去重实体，同时保留完整 `(source,index)` 成员关系。
4. J273 `+0x1D98` 不是 SubPlayer；四类当前字段为 SubPlayer `+0x1D84`、Twin `+0x1E4C`、backup `+0x1E50`、replaced `+0x267C`。保留关系目标是否当前可记录仍取决于分类/生命周期证据。
5. 不能把 Grid 槽数等同当前房间地图宽高乘积，也不能把 grid render 坐标等同 Entity raw Position。
6. KeepDecoration 可导致 GridFree 请求后槽仍保留对象；Door/ESAU/backup/replaced 的来源出现也不自动取得 active 或新生资格。

## 原始证据归档

对应逐地址指令摘录、SDK 声明摘录及原型源码位置已另存至 [`st4-evidence.md`](../../artifacts/repentogon/tx-04-08-static/st4-agent/st4-evidence.md)。本笔记保留对目标与原型的判读；其中主审为 Room constructor 与 Game root 额外提供的独立 IDA 回读按来源注明，未声称为我的独立调用结果。
