# tx-04-08 ST3–ST5：字段、来源与分类身份静态核验

> 2026-10-27 A项更正：下方+0x177的初始化UNKNOWN漏掉了基Init在VA0x6A4BD6的word零写（覆盖+176/+177）。ABC主审（已删除，原文 `39ddcfd6^:docs/design/tx-04-08/tx-04-08-abc-implementation.md`）已复读指令/字节并修正layout；当前读取仍按本轮用户采纳规则单独限定。原ST3–ST5证据和漏项记录保留，不外推所有写入方已闭合。

日期：2026-10-26，Asia/Shanghai。范围来自迁移设计第 4.3 节（已删除，原文 `39ddcfd6^:docs/design/tx-04-08/tx-04-08-implementation-plan.md`）。用户指定 ST3、ST4、ST5 **各由一个 GPT-6 Luna / max 子智能体逆向，主智能体审阅**。三个代理分别负责字段布局、来源闭包和分类身份；主审另行读取目标 IDB、原始指令与 SDK，不仅审阅代理摘要。

状态：**ST3、ST4、ST5 指定问题的静态核验与主审完成，具体 UNKNOWN 保留。** 本次只做静态查询和研究文档编辑，不修改生产代码、IDB、绑定或运行进程；没有构建、生成器、游戏启动、测试、额外格式校验或哈希操作。ST6–ST8、工程实现和 tx-04-08 整体 E2E 不在本次完成范围。

## 1. 目标、材料与证据口径

主审直接读取当前 IDB 的输入路径、imagebase 和版本字符串：`isaac-ng-v1.9.7.12.exe.unpacked.exe`，`Binding of Isaac: Repentance+ v1.9.7.12.J273`，x86，imagebase `0x400000`。下文地址均为该 IDB 的 VA，`RVA = VA - 0x400000`，不能直接当作实际进程地址。

`STATIC_CONFIRMED` 指目标指令和静态数据；`CURRENT_SOURCE` 指本地 SDK 或仓库源码；`DESIGN` 指未来采集器行为；语义假设和 `UNKNOWN` 单列。没有运行时观察，也没有将旧版本 PASS 转移到新版本。

代理材料：[ST3 字段笔记](tx-04-08-st3-agent-notes.md)、[ST4 来源笔记](tx-04-08-st4-agent-notes.md)、[ST5 分类身份笔记](tx-04-08-st5-agent-notes.md)。主审独立材料位于 [st3-st5-review](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/query-notes.md)：

- [IDB 上下文与 Game 根指令](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/main-idb-context.json)。
- [基类 Init/析构、Factory、GridSpawn/Free、Publish 完整指令与伪代码](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/main-core-functions.json)。
- [七类派生 Init 与删除包装](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/main-derived-init-delete.json)。
- [Entity/Player 构造、派生析构正文及 GridInit](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/main-constructors-dtor-bodies-gridinit.json)。
- [LoadEntityConfig 槽位与 HP 原始 word 的写入来源](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/main-load-config.json)。
- [32 组 Entity/Grid 虚表、COL/CHD/BCD](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/main-rtti-vtables.json)。
- [Grid getter、MakeDoor、PlayerManager Restore/Replace](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/main-sources-door-restore-replace.json)、[Room 构造](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/main-room-constructor.json)、[Grid/Door 删除包装](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/main-grid-free.json)。
- [原生 roster 取值与八 EL 构造](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/main-roster-el-constructor.json)。
- [GetSubPlayer/GetGridPosition 的原生注册与函数体](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/main-subplayer-gridposition.json)。
- [带行号 SDK 快照](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/main-sdk-source.json)与[原型字段/来源/分类摘录](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/main-prototype-source.json)。

反编译的推测类型与额外寄存器实参不能替代 ABI；`FuncItems` 的远处 EH chunk 不等于正常路径。Room 构造首次特征查询的手抄错误与修正如实保留在[查询说明](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/query-notes.md)。

## 2. ST3：字段布局与初始化

### 2.1 必须逐项迁移的 Player 与 frame 位置

完整 **78 个字段 ID** 的旧→新偏移、宽度、编码和逐项初始化出处见 [ST3 字段表](tx-04-08-st3-agent-notes.md)。主审逐行回读并补齐漏项；下表集中列出迁移时会读错字段的变化，不替代完整 profile。Entity 公共、NPC、Familiar、Effect、Pickup、Tear、Projectile 与 Grid 的同址字段也分别有目标声明/访问依据，没有按一段偏移差外推。

以下字段宽度均为 4 字节；引用保留地址诊断和独立引用状态，不把 raw32 当跨进程身份。旧值来自原型 `world_record.cpp:1442–1448`，新值与目标 Player Init/构造/Restore/Replace 指令和 SDK 交叉核对。

表内偏移均为十六进制。

| 字段 ID | 语义 | 原型偏移 → J273 偏移 |
| --- | --- | --- |
| `0101` | player_type | `13C0 → 13BC` |
| `0102` | player_index | `161C → 1610` |
| `0103` | damage_cooldown | `13BC → 13B8` |
| `0104–0108` | max/red/eternal/soul/black hearts | `1340/1344/1348/134C/1350`，逐项不变 |
| `0109` | bone_hearts | `1D88 → 1D74` |
| `010A` | golden_hearts | `194C → 1938` |
| `010B` | broken_hearts | `1DA0 → 1D8C` |
| `010C` | rotten_hearts | `1DA4 → 1D90` |
| `010D` | sub_player | `1D98 → 1D84` |
| `010E` | twin，EntityPtr 的 raw pointer word | `1E68 → 1E4C` |
| `010F` | backup | `1E6C → 1E50` |
| `0110` | replaced | `26C8 → 267C` |
| `001D` | Game frame 原始值 | `Game+264F8 → Game+25A70` |
| `001E` | frame_count 推导 | 保留 32 位 `game_frame - Entity[328]`，不当作 Authority tick |

主审原始例证：`0x74FCA2` 写 Player `+13BC`；`0x74FE5D` 写 `+13B8`；`0x74FD71/0x74FD99` 清 `+1938/+1D74`；`0x75140D/0x751417` 清 `+1D8C/+1D90`；构造 `0x74E091` 写 `+1610=-1`。`0x995E51/0x995E5E` 互写两 Player 的 `+267C`；Restore 读取/写入 `+1E4C/+1E50` 的证据与下节来源规则一致。没有以一段统一偏移差推算这些位置。

`0x6A4B06: A1 F4 49 BE 00` 从 Game 根槽 VA `0xBE49F4`（RVA `0x7E49F4`）读取 Game；`0x6A4B2B: 8B 80 70 5A 02 00` 读取其 `+25A70`，`0x6A4B31` 写 Entity `+328`。GridInit 的 `0x705094` 也从 Game `+25A70` 取值后写 Grid `+28`。旧根槽和旧 frame 偏移不得随原型复制。

### 2.2 初始化证据的层次

`Entity::Init 0x6A4A20` 正常返回仅证明基类这一层完成。七类均有自己的 Init 槽：Player `74F6A0`、NPC `6B4A50`、Tear `662F50`、Projectile `64F4B0`、Pickup `6DD7C0`、Familiar `607AA0`、Effect `5CD070`。派生字段读取资格不能在内部基 Init 返回时提前获得；外层 Spawn/Morph/Restore/创建 owner 及 REPENTOGON 短路的最终完成条件继续交由 ST6/ST8。

主审另行回读到的派生写入包括：

| 类别 | 目标写入证据 | 说明 |
| --- | --- | --- |
| NPC | `6B4C61: [B64]=1`；`6B4C7F: [410]=0`；`6B4D65/6B4D6F: [BA0]=0/[BA4]=-1` | 四项均为 dword |
| Familiar | `607B43: [410]=player`；`607C0D: [D4C]=0`；`607EEE: [D88]=0` | 原始 Player 引用与两个 dword，不调用 getter |
| Effect | `5CD1DD/5CD1E7/5CD1F1: [450]=0/[45C]=-1/[460]=1` | 后续 variant 分支还会修改；不冻结为这些初始常量 |
| Pickup | `6DD82F/6DD839/6DD8A1: [540]=-1/[544]=0/[584]=0` | Init 后仍有 subtype/variant 路线，不等同 owner 完成 |
| Tear | `663092/66309C/6630A6/6630B0` 写 `+428/+42C/+430/+434` | `0601` 为完整 128 位，不能缩成 64 位 |
| Projectile | `64F512/64F52B` 清 `+438/+43C` | `0701` 为 64 位，与 Tear 分开 |
| Grid | `705070` 写 seed `+14`、collision `+3C`，虚调 `+28` 后递增 `+10`、置 byte `+1C`、写 frame `+28` | 中间虚调用和完成边界须保留，不把进入当 initialized |

通用 HP/maxHP `+380/+384` 的位置与读写不等于“所有类别均以该值表示生命”。基 Init 的 `0x6A4B37/0x6A4B3D` 是 `+384 → +380` 复制，本身不证明源值对所有派生类别均已初始化；继续保留历史用户采纳的通用 HP 假设，Player 心量单列。Player/Tear/Projectile 的未确认 State/Timeout 保持 `UNCONFIRMED/UNKNOWN`，不得补零或套用 NPC/Effect 字段。

主审继续追到该复制之前的 LoadEntityConfig：Entity 和除 NPC 外的上述派生 slot 25（`+64h`）均为 `0x6A4F30`，配置存在路径 `0x6A500F` 用 `movss` 写 `+384`，配置缺失路径 `0x6A51E3` 写 `+384=0`。NPC 的槽为 `0x6B7730`，`0x6B778E` 先调用基实现，随后 `0x6B7BF2` 可再次写 `+384`。这补足已检阅正常原生 Init 路径上两个原始 float word 的来源，仍不把所有类别的生命语义或 Hook 链实际到达当作已证明。

启用时已存在对象仍是 FIRST_OBSERVED。布局已定位不能补造构造/Init 历史；exact-pair 当前 PlayerUpdate 依据仍遵循现行规范第 11 节，并由 ST7 核对撤销接线。

### 2.3 主审补齐的初始化与未命名字段

代理第一轮筛选文件漏掉了若干写入，主审直接从完整指令回读后要求修订，未接受“筛选未命中即无初始化依据”：

- flags 的 `0x6A4D7B` 写 `+168=0`，`0x6A4D85` 写 `+16C=0`，共同清完整 64 位；不能因第一轮只命中低 word 就留下高 word 未初始化的结论。
- SubPlayer 在 Player Init `0x74FD4D` 清 `+1D84=0`；replaced 在 Player 构造**正常尾部** `0x74E926` 清 `+267C=0`。它们不是缺少直接初始化写入；实际保留关系何时可读仍须当前 owner/生命周期依据。
- Familiar Init `0x607B43` 写 `+410=EAX`：`0x607B26` 先读 spawner `+3C8`，type 为 1 时采用它，另一分支 `0x607B3E` 调 `0x4131B0` 后汇入同一写入。该静态分支不允许采集器按 Type 猜分类，也不保证该引用在之后一直有效。
- [Grid 构造与 Factory](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/main-grid-constructor-factory.json)补出 `0x704FB0` 对 Desc 的 `+4/+8/+C/+10/+18` 清零、`+14/+20` 初始常量、`+1C` byte 清零和 `+2C..+38` 的四个 RNG word 初始化。Factory 的 `0x706636/0x706639` 写 type/index；GridSpawn 的 `0x7E2257/0x7E225D` 写 variant/var_data 后才于 `0x7E2260` 调 GridInit。故这些字段不能仅因 GridInit 本体不写就全部列作未知；构造默认值、创建实参、派生 Init 后态和最终 owner 完成仍须分层记录。

SDK 没有成员名也不自动意味着目标语义不可确认。主审另读[原生字段注册](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/main-field-lua-registration.json)及[Pickup Wait getter/setter](../../artifacts/repentogon/tx-04-08-static/st3-st5-review/main-pickup-wait.json)：

| 字段 | 原生名字与位置的交叉证据 |
| --- | --- |
| `0302 FireCooldown` | EntityFamiliar 注册范围内 `0x851267: push D4Ch`、`0x85126C: push "FireCooldown"`，与 Init 的 dword 写相符 |
| `0502 Wait` | EntityPickup 注册 `0x850F40: "Wait"` 配对 getter `844A60`、setter `4318C0`，分别直接读/写 `+544` dword |
| `0503 State` | EntityPickup 注册范围内 `0x850EEB: push 584h`、`0x850EF0: push "State"`，与 Init 的 dword 写相符 |
| `0701 ProjectileFlags` | EntityProjectile 注册 `0x8514E8: "ProjectileFlags"` 配对 getter `413170`、setter `442D70`，实际覆盖 `+438/+43C` 两个 word，共 64 位 |

这些注册与函数仅被静态读取，未执行任何 getter/setter、Lua 注册或游戏方法。仍保留的 UNKNOWN 包括 `removed_by_factory +177` 在本次所读构造/Init 内的初值来源、Player/Tear/Projectile 的额外 State/Timeout、各字段在未观察 owner/恢复路径下的读取依据；不把初值来源缺口改成默认零。记录原始值的 schema、跨进程语义与该次对象 readiness 是三个不同问题。

## 3. ST4：来源、边界与别名

### 3.1 来源表及有界读取

`Game+18300` 为 Room，`Room+1218` 为 EntityList，`Game+1BAA8` 为 PlayerManager。来源 ID 沿用文件模型；八个 EL 是视图，不能各自创建实体身份或当作互斥所有者。

| source | 来源 | J273 原始位置/数量 |
| --- | --- | --- |
| 0 | main | `Room+1238` 的 EL |
| 1 | persistent | `Room+1248` 的 EL |
| 2 | update | `Room+1258` 的 EL |
| 3 | render | `Room+1268` 的 EL |
| 4 | effect | `Room+1278` 的 EL |
| 5 | temp | `Room+1288` 的 EL |
| 6 | bufferTemplate | `Room+1298` 的 EL |
| 7 | wisp | `Room+12A8` 的 EL |
| 8 | roster | `Game+1BAA8` 的 vector，三个指针 word |
| 9 | Esau Jr. 保留状态 | `Game+1BB28`，4 个可空 Player 指针 |
| 10 | Player 保留关系 | 每个已分类 Player 的 `+1D84/+1E4C/+1E50/+267C`，有界工作表去重 |
| 11 | Grid | `Room+24`，**448** 个可空指针 |
| 12 | door | `Room+724`，8 个可空指针；doorSlot→gridIndex 表为 `Room+744` |
| 13/14 | reserved | 明确 `NOT_APPLICABLE` |

`EntityList_EL` 是 16 字节 `{sublist:byte@0, data:u32@4, capacity:u32@8, size:u32@C}`，不是三个指针的 vector。只枚举 `[0,size)`，要求 `size<=capacity`、非空有 data、指针对齐、乘法/地址加法不溢出且不超过采集预算；不能扫描 capacity 或把子视图的共享存储当独立实体。

roster 为 `{begin,end,allocation_end}`，非全空时要求三个指针有效、对齐且 `begin<=end<=allocation_end`；数量为 `(end-begin)/4`，不把 capacity 当人数。读取失败与真实空来源分开。复制前后复读描述符与实际成员，配合 mutation/epoch/token 检测变化；这不提供跨线程内存锁。

主审独立回读 `PlayerManager::GetPlayer 0x98F930` 的 begin/end 差值除以 4，以及 `EntityList` 构造 `0x413270` 对八个 EL 的 `+10h` 步进，和 SDK descriptor 声明相符。原生 GetPlayer 在异常/越界输入下的警告与回退不提供观察器的通用安全保证；采集器继续使用有界原始复制，不调用它。

保留关系只允许从本批已分类且字段读取依据成立的 Player 扩展，64 节点预算到限必须记录不完整；不能把任意引用强转 Player 或把保留对象自动当 active/roster/A/B。Esau 与关系字段都不赋予当前 PlayerUpdate 依据，也不补出生历史。每批按 `(domain,address)` 合并对象，保留每个 `(source,index)`；来源退出只更新 membership。

代理初报将 `+1D84` 的 SubPlayer 名称保留 UNKNOWN；主审没有直接接受该缺口，也没有仅凭旧字段名确认：原生 Lua 注册的 `0x84FEBC` 为 `push sub_6D86D0`，`0x84FEC1` 为 `push "GetSubPlayer"`，目标函数 `0x6D86D0` 直接读取 `[ecx+1D84h]` 返回。故名称/偏移可确认，具体保留生命周期和当期字段读取资格仍为 UNKNOWN/ST6–ST7。这个注册关联仅用于静态解释，采集器不调用 getter。

### 3.2 447/448 差异与门别名

**目标 Grid 表是 448 槽，合法 index `0..0x1BF`。** SDK `Room.zhl:184` 的 `[0x1BF]` 是 447 的 count 声明，少一项，不能作为扫描上限。

主审及 ST4 代理分别回读：

- `Room::GetGridEntity 0x431710`（RVA `0x031710`）在 `0x431716` 比较 `1BFh`，`0x43171B` 仅在 `ja` 时返回空；`0x43171D` 读取 `[ecx+eax*4+24h]`。
- `SpawnGridEntity 0x7E21D0` 拒绝 `>1BFh`，正常路径写同一索引槽；`RemoveGridEntityImmediate 0x813BD0` 同样接受 `<=1BFh`。
- Room 构造 `0x7DFFF0–0x7DFFFF` 执行 `memset(Room+24,0,720h)`，覆盖 `[24,744)`。结合上述索引边界和门位置，`[24,724)` 是 448 Grid 指针，`[724,744)` 是 8 door 指针。此结论不只来自地址间距。
- MakeDoor 的 `0x7E4E21` 从 `Room+744+doorSlot*4` 取 gridIndex，`0x7E4F7B` 写 Grid 槽，`0x7E4FE7` 将同一 Door 对象写 door 槽。门是 Grid 派生对象的两个来源别名，不能重复计出生或由 doorSlot 直接代替 gridIndex。

### 3.3 Grid 坐标推导及代理建议的主审修订

实际 Room 形状可用格数量与固定表容量分开；空槽不补对象，不能用 `width*height` 静默替换固定来源覆盖。

代理最初从 `GridEntity::GetRenderPosition 0x705110` 看到格点公式后还有 render/scale/floor 变换，因此保留了世界格点坐标公式的确认。主审进一步从原生注册找到独立 `Room::GetGridPosition`：`0x852F99` 为 `push sub_8087A0`，`0x852F9E` 为 `push "GetGridPosition"`。`0x8087A0` 直接取 Room `+C` width，对 index 做余数/商并写返回 Vector：

`x = float(index % width) * 40.0f + 40.0f`，`y = float(index / width) * 40.0f + 120.0f`。

这确认原型 `0810/0811` 的当前原生格点语义。它仍是从已确认索引和 width 推导的 F32 值，不是对象上的 raw Position，也不是最终 render position；manifest 须登记公式/来源，不能与 Entity position 混用，采集时不调用该 getter。

代理曾建议必须满足 `index<width*height`、row/column 均在房间尺寸内才输出坐标。**不将这项建议采纳为原生公式的必备条件**：`0x8087A0` 没有读取 height 或进行该检查，`GetGridIndexByTile 0x808700` 的 row/column 边界属于另一个接口。采集仍要求已确认有效的 Room/非零有界 width、固定来源 index `<448`、无溢出和唯一一致的槽关联；width/height 和实际房间形状另行保存诊断，不能用其它接口的限制悄悄缩减固定来源 profile。

同一对象命中多个不同 Grid index，或对象 `+24`、实际槽和 door-to-grid 映射出现冲突时，不随意选择一个坐标；保留冲突和不完整状态。只有 door 来源而缺少已确认 Grid 槽关联时，不能把 doorSlot 代入公式。别名去重不删除这些冲突证据。

## 4. ST5：只读分类与身份规则

### 4.1 目标 RTTI 与继承

主审从 32 张已命名 Entity/Grid 虚表逐一读取 `vptr[-1] → COL → CHD → BCD`。以下地址是目标 IDB 的 VA；实际布局清单须保存 RVA 并按当前模块基址换算，不能沿用原型 `0x85D...` TypeDescriptor 或 `InImage` 范围。

| 分类 | vtable VA | COL VA | TypeDescriptor VA / RVA |
| --- | --- | --- | --- |
| Entity 基类 | `B18224` | `B439E4` | `BD1244 / 7D1244` |
| Player | `B1BDB4` | `B44010` | `BD13A8 / 7D13A8` |
| NPC | `B187D0` | `B43A08` | `BD1344 / 7D1344` |
| Tear | `B15FB8` | `B43844` | `BD152C / 7D152C` |
| Projectile | `B15B1C` | `B437E4` | `BD13D8 / 7D13D8` |
| Pickup | `B18E8C` | `B43A80` | `BD146C / 7D146C` |
| Familiar | `B15090` | `B4373C` | `BD1310 / 7D1310` |
| Effect | `B14D00` | `B43618` | `BD1270 / 7D1270` |
| Grid 基类 | `B1975C` | `B43B04` | `BD143C / 7D143C` |
| Door | `B1966C` | `B43CF4` | `BD15C0 / 7D15C0` |

七类均为 offset-0、非虚直接 Entity 继承；Door 为 GridEntity 继承。已读主 COL 的 signature/offset/cdOffset 为 0，对应 PMD 为 `{mdisp=0,pdisp=-1,vdisp=0}`。这支持当前对象首地址直接读取这些布局，不推广到未知多重/虚继承对象。

未来分类只做有界原始读取，校验 vptr、COL/CHD/BCD/TD 的地址范围、长度和计数，并匹配本版本已登记的 TD/继承关系；元数据缺失、冲突或不支持的 adjustment 输出 Unknown。**不调用 dynamic_cast、Lua、虚 getter 或任何分类方法，不按 Type 范围猜 NPC。** Factory 对 type 的分流只描述分配路径，不能替代读取最终对象的分类。

Bomb/Laser/Knife/Slot/Text/Dummy 等已读虚表均有 Entity 基类，但不属于本轮七类专用 schema：按 UnknownEntity 输出允许的头部和原因，不冒充 NPC，不删除这些对象制造完整样本。连 Entity 基类都无法确认时不套 Entity 头部布局。Grid 在独立 domain 中只读 Grid profile；Door 不套 Entity HP/flags。

### 4.2 析构、删除与池复用

| 类别 | slot 0 删除包装 → 正常析构路径 |
| --- | --- |
| Entity | `6A3EC0 → 6A3EF0` |
| Player | `74E970 → 74F260 → 6A3EF0` |
| NPC | `6B4870 → 6B48A0 → 6A3EF0` |
| Tear | `662DF0 → 662E20 → 6A3EF0` |
| Projectile | `64F470 → 6A3EF0` |
| Pickup | `6DD670 → 6DD6A0 → 6A3EF0` |
| Familiar | `6070C0 → 6070F0 → 6A3EF0` |
| Effect | `5CCF30` 内额外成员清理后 `→ 6A3EF0` |
| Grid / Door | `705020 / 706730`，各自清成员；不经过 Entity 析构 |

主审分别定位派生正文中的正常基析构 call：Player `74F67F`、NPC `6B4A2D`、Tear `662F2F`、Pickup `6DD7A3`、Familiar `6071C9`。删除包装均把 flags bit 0 与 allocator release 分开；基析构正常返回不等于已经释放内存。Entity 基析构入口会把 vptr 改为基类表，需在对象仍有相应证据时保存此前分类；不能把拆解中的基类 vptr 当新对象。

`EntityFactory::Create 0x6EC540` 从池槽检查 `+170` valid byte 并选取对象；存在不经构造/析构的直接返回。池饱和且第二实参低字节为真时返回 null；可覆盖路线调用 `CanOverwrite`（vtable `+38`），标记 `+177`，依次虚调 Remove（`+28`）与 ClearReferences（`+44`），复位标志后返回同址池槽。没有观察到该取得路径需要 new/ctor/dtor/free。正常非零取得是新 use 的依据；null 不产生新 use/birth，不能只等析构判断复用。这里按实际分支描述该布尔值，不从 SDK 参数名 `Force` 推断相反的行为。

满池扫描仍找不到可覆盖候选时，目标会输出警告后继续所选槽的覆盖。因此 `CanOverwrite` 不是严格阻断保证，也不能作为对象地址永久不复用的依据。

`Remove`、`ClearReferences`、退出任一来源、基类析构和 allocator release 是不同事件。GridFree 请求也有空槽/保留 decoration 分支；仅请求或仅来源槽变化不能概括为所有别名已消失或已物理释放。实际调用链与目标对象必须分别记录。

### 4.3 采集器应采用的身份边界

以下是由静态事实约束的 `DESIGN`，尚未写成新实现：

1. `(recording/domain,address)` 仅是本地临时关联，分别保存 physical local_id、use_generation、birth_event、revision、origin/readiness 和 memberships。同址不保证同 use，不同 domain 不合并。
2. 已观察正常 Factory 非零返回登记取得/新 use；物理构造/重构证据另行登记 physical identity，异常或 null 不伪造成功。重复 Init 不自动重复出生；首次启用前已有对象保持 FIRST_OBSERVED。
3. 外层实际 owner 正常完成后才确认出生描述，Publish 不能抢先确认；Morph 保留出生/use、更新分类/revision。未知 Init/Restore/Replace 撤销不成立的连续性，不由相同地址/seed/Index“修复”。完整 owner 嵌套留 ST6。
4. 成员退出不等于销毁，真实析构与释放分列；保留 Player 可以延续物理 local_id，但旧 room membership、旧 A/B 输入绑定与当前字段读取依据不能因此延续，撤销接线留 ST7。
5. 引用仅在当前目标唯一、同代且连续性证据成立时解析；保留 NULL/KNOWN_CURRENT/UNRESOLVED/RETIRED/AMBIGUOUS_REUSE。目标同批出现不消除同址复用风险，未知地址不能变成 NULL。
6. 分类失败、未配对事件、事件丢失或元数据不支持时保留原因并停止不成立的读取/连续性。world 自有故障的隔离、真实 Hook 链和线程/异常覆盖留 ST8，不影响已确认输入事实。

## 5. 主审收束与后续范围

主智能体已逐项回读三个代理的最终笔记与静态材料，并以另取 IDB/SDK 证据审阅，接受范围如下：

| 项目 | 本次接受的静态结论 | 主审补正与限定 |
| --- | --- | --- |
| ST3 | 78 项旧→新字段/宽度/编码、Player 重排、Game frame 新来源、构造/Init 的逐字段写入 | 补出 flags 高 word、SubPlayer/replaced/Familiar 引用写入；另追 Grid 构造/Factory 与四项未命名字段的原生注册。初筛漏项不继续当作未知事实；通用 HP 假设及额外 State/Timeout 保留 |
| ST4 | 15 个 source ID、EL/vector descriptor 与容量/size、448 Grid/8 门、保留关系、GridPosition 公式 | 补出 GetSubPlayer/GetGridPosition 的 API 注册；撤回将另一个 API 的 height 条件套作坐标前置条件的建议。来源出现不等于 active/出生/当前可读 |
| ST5 | 七类与 Grid/Door 的 RTTI/COL/基图、派生删除包装→析构、无析构池复用、只读分类与身份规则 | 区分 flag 释放、物理 local_id/use/birth；按实际布尔参数分支描述 Factory，不凭 Force 名字猜语义，CanOverwrite 警告不保证阻断覆盖。正常路径不证明异常或实际 Hook 覆盖 |

保留事项分别是：

- 字段级 UNKNOWN：`removed_by_factory +177` 在本次所读构造/Init 中的初值来源，Player/Tear/Projectile 的额外 State/Timeout，通用 HP 的跨类别生命语义假设；Grid `+3C` 的最终值不能由虚调用前的清零推断。
- ST6：Spawn/Init/Publish、Morph、Restore/Replace、Grid/Door 创建/替换的全部 owner 嵌套与完成条件，真实可达 fixture 及允许副作用。
- ST7：保留 Player/Esau/SubPlayer 等在具体生命周期下的字段读取资格，A/B exact-pair 当前 PlayerUpdate 依据与撤销通知，加载/复活/Continue/teardown 对齐。
- ST8：实际 Hook 共享/priority/注册数量和容量、异常/线程与观察故障隔离、32 位资源预算和退出收口。当前 RTTI 正常路径不预先回答这些问题。

本次只形成静态资料与主审结论，没有对子项另跑测试验收。当前证据不代表 Hook 已注册、生效链已运行、来源已实际稳定、身份无遗漏或完整样本已生成。ST1–ST5 的静态工作完成不等于 tx-04-08 功能完成；2,000 次稳定段、双进程比较及整体 E2E 均尚未执行。既有 ST1/ST2 报告及 ST2 的过程偏差保留当时范围，不追溯改写。
