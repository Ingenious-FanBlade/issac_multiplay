# tx-01-65：HP 字段与变更时机

日期：2026-10-18。状态：HP 子集静态研究完成，主智能体复核记录见[整体静态文档审阅](tx-01-65-review.md)。本文只处理 T0-06 第三步缺口中的 HP 子集，不代表第三步或 capture 目标整体完成。

## 范围、版本与证据来源

本次目标是确认普通 `Entity` 的 `HitPoints` / `MaxHitPoints` 原始字段布局、初始化及主要伤害变更时机，并把玩家 hearts 单独记录；同时说明 Player、NPC、Tear、Projectile、Pickup、Familiar、Effect 和 GridEntity 的适用边界。研究不实现采集器、hook 或内存写，不穷尽所有 writer、特殊角色生命算法或运行时样例。

静态目标为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`，输入 `isaac-ng.exe`，IDA image base `0x400000`，32 位；当前 banner 为 **Repentance+ v1.9.7.17.J460**。旧版本字符串也存在于映像中，不能据此选择旧版布局。以下地址均为该 IDB 中的 VA；如表中给出 RVA，则按 `VA - 0x400000` 计算。结论只对这份静态映像成立，不承诺跨版本稳定。

分析由用户指定的 GPT6 Luna / max 子智能体开始。它因额度限制中断前生成了 [原始静态导出脚本](export_tx_01_65.py) 与 [完整原始证据](../../../../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json)（42 个函数、21 段原始指令范围、9 个 vtable）；用户要求继续后，主智能体独立静态复核并补充 [复核导出脚本](export_tx_01_65_review.py) 与 [primary-evidence.json](../../../../../artifacts/cs-t0/tx-01-65-static/primary-evidence.json)（17 个函数、14 段原始指令范围及更正）。随后原子智能体继续完成研究稿，主智能体检查并修订。两份导出共含 44 个不同函数，重复读取保留。原始导出中的旧注释保留；与复核结论冲突之处，以复核文件的 `corrections` 和原始指令为准。特别是 `0x7CAFE0` / `0x7CB060` 曾被误解为相关玩家转发，本文不沿用该解释。

指定参考会话 `01a149e2-a48c-7cc2-883e-fd04d14e964d` 的最后一轮 `01a14a8a-6c41-7242-bef3-440634e0fab4` 仍只给出旧源码 hearts 等线索，未提供通用 HP 布局。本轮以当前 IDB 补全，未把旧源码偏移直接当作现版本结论。

结论标签含义：

- `STATIC_CONFIRMED`：可由当前 IDB 的绑定、函数实现或原始指令直接支持。
- `USER_ACCEPTED_ASSUMPTION`：用户明确要求先统一假设各 Entity 派生类都用普通 HP 字段表示生命；这是建模约定，不伪装成逐类已证明的有效生命语义。
- `UNKNOWN`：现有静态证据未能确证的运行时有效性、特殊路径或独立对象模型。

过程沿用现行 [项目规范](../../project-conventions.md)；该规范覆盖旧计划及历史记录中冲突的流程要求。本次仅新增研究文档和静态证据、更新入口；未运行游戏、构建、代码测试或哈希校验，未修改运行时代码或 IDB。

## 普通 Entity HP 字段

| 项 | 结论 | 直接证据 |
|---|---|---|
| `HitPoints` | `STATIC_CONFIRMED`：32 位 `float` 当前值，偏移 `+0x380`。 | Lua 属性注册在 `0x868C48` 推入 setter `0x435D20`，`0x868C4D` 推入 getter `0x417370`，`0x868C52` 推入字符串 `HitPoints`。getter `0x417370` 的指令 `D9 81 80 03 00 00` 为 `fld dword ptr [ecx+380h]`；setter `0x435D28` 的 `F3 0F 11 81 80 03 00 00` 为 `movss [ecx+380h],xmm0`。RVA 分别为 `0x17370`、`0x35D28`。完整绑定及函数见两份静态证据。 |
| `MaxHitPoints` | `STATIC_CONFIRMED`：32 位 `float` 上限，偏移 `+0x384`。 | 属性注册在 `0x868C32` 推入 setter `0x435D40`，`0x868C37` 推入 getter `0x417360`，`0x868C3C` 推入字符串 `MaxHitPoints`。getter `0x417360` 的 `D9 81 84 03 00 00` 为 `fld dword ptr [ecx+384h]`；setter `0x435D48` 的 `F3 0F 11 81 84 03 00 00` 为 `movss [ecx+384h],xmm0`。RVA 分别为 `0x17360`、`0x35D48`。 |
| 增加 HP | `STATIC_CONFIRMED`：基类 `AddHealth` 对普通有序有限浮点输入执行 `HP = min(HP + amount, MaxHP)`；不执行通用下界 clamp。 | `Entity::AddHealth` `0x6A9C50`：`0x6A9C5E` 对 `[ecx+380h]` 执行 `addss`；后续选择/限制结果并写回 `+0x380`（`0x6A9C7A`）。这只证明该基类 helper 的行为，不代表所有生命变化都经过它。 |
| 满血判断 | `STATIC_CONFIRMED`：基类 `HasFullHealth` 比较当前与上限。 | `0x6A9C30` 读取 `this[224]` 与 `this[225]` 并比较，即 `+0x380 >= +0x384`。 |

属性名、getter、setter 与成员指令形成闭合证据；不能只凭相邻偏移推断类型或语义。setter `0x435D20/0x435D40` 是对各自字段的 plain `movss`，没有 clamp，也不成对同步 HP 与 MaxHP。因此 `0 <= HP <= MaxHP` 不是可由这些接口保证的通用不变量。Entity 基类构造函数 `0x6A8290` 的对象大小为 `0x410`，本次导出的 ctor 函数体没有对 `+0x380/+0x384` 的显式写入。因此 ctor 结束不是已完成 HP 初始化的证据。

## 初始化和变更时序

`Entity::Init` 在 `0x6A93A0`。其路径先在 `0x6A9456` 调用 vtable `+0x64`（配置加载/派生配置路径），再于 `0x6A94B7` 读取 `[esi+384h]`，并在 `0x6A94BD` 将同一 32 位值写入 `[esi+380h]`。对应字节为 `8B 86 84 03 00 00` 和 `89 86 80 03 00 00`。所以基类 Init 建立的是 **HP = MaxHP**；MaxHP 的来源可由前面的虚调用及派生 Init 后续覆盖影响。不能把 ctor、配置加载中间态或基类 Init 刚返回一概当成最终派生对象初始值。

已确认的派生例子说明采样必须考虑后置写入：

- `Familiar::Init` `0x60DE70` 先调用基类 Init（`0x60DEE9`），之后按 `Variant`（局部 `v5[11]`，不是 SubType）写两格。Variant 67 路径在 `0x60EDF7` 将 `+0x384` 设为 `42C80000h`（100.0），`0x60EE04` 将 `+0x380` 设为同值；Variant 100 在 `0x60F28B` / `0x60F295` 写 `461C3C00h`（9999.0）。后续特定分支还会读取当前值并按难度或比例调整、同步两个字段。证据见指令范围 `0x60EDF7`、`0x60F28B` 及 Familiar Init 完整导出。原先口述的 10000.0 已按 IEEE-754 常量纠正为 **9999.0**。这是 `Familiar` 后置初始化可改写 HP 的直接反例，不是所有 Variant 的完整算法。
- `Player::Init` `0x755470` 的函数路径在 `0x755B11` 调用 `Entity::Init`；随后 `0x755B81` 写 `+0x380=1.0f`，`0x755B8B` 写 `+0x384=1.0f`。同一 Init 中 hearts/Bone/Golden 等字段有早期清零，后面仍有角色相关初始化，因此这些清零指令不是最终 Player 心量。
- Tear Init `0x6660A0`、Projectile Init `0x652B90`、Effect Init `0x5CDB20` 都先走基类 Init，之后各有对 `+0x384` 写零的路径（分别约 `0x6661D8`、`0x652C8D`、`0x5CDB89`）。该指令只证明这些路径把 MaxHP 字段写为 0；对零上限在运行时代表什么不作推断。
- NPC Init `0x6B8B20` 走基类 Init；NPC 的配置虚路径可在基类复制前影响 MaxHP。现有导出不足以把所有 NPC 配置分支及最终当前值闭合为统一规则。

### TakeDamage 不等于扣血完成

基类 `Entity::TakeDamage` 位于 `0x6ACE60`。此函数不是直接 `HP -= damage`：

1. `0x6AD24D` 读取 MaxHP（`F3 0F 10 87 84 03 00 00`，`movss xmm0,[edi+384h]`）；`0x6AD258/0x6AD25B` 的比较/跳转仅让 MaxHP 大于零的路径继续。
2. 条件 callback 在 `0x6AD270` 调用 `0x864560`；回调拒绝的路径不进入后续追加。
3. `0x6AD2AD` 调用 `0x6B7DB0`，向 Entity `+0x08` 的 damage-record vector 追加一个 56 字节记录；`0x6AD2C8` 更新 pending sum `+0x14`（`F3 0F 11 47 14`）。
4. 基类 `Entity::Update` `0x6AE820` 消费队列：`0x6AF6AF` 从 `[edi+380h]` 读 HP，`0x6AF6B7` 执行 `subss`，`0x6AF6BE` 写回 `[edi+380h]`。队列处理尾部在 `0x6AF8C3` 按条件调用虚方法 `+0x24`（Kill），随后 `0x6AF8C6..0x6AF8DA` 清理队列 end 和 pending 状态字段。

因此，`TakeDamage` 返回成功最多说明 damage request 进入相应路径；它不是已扣血后的采样边界。队列消费、派生类虚分派和条件 Kill 还在之后发生；HP 等于零也不单独证明对象已经走完死亡状态转换。Player 的 vtable TakeDamage 是专用实现 `0x7729D0`，基类 `0x6ACE60` 对 `Type==1` 有“should not be called for Entity Player”的诊断路径。以上为静态路径，不指定实际采集 hook 或运行时采样点。

## Entity 派生类别覆盖

以下 Entity 类继承基类对象布局，普通 HP 字段的**存在**为 `STATIC_CONFIRMED`；各派生类以该字段作为生命模型的统一处理是用户明确接受的 `USER_ACCEPTED_ASSUMPTION`，并不等于已证明每类当前有效 HP 的规则。vtable 来自当前映像导出，非跨版本 ABI 承诺。`TakeDamage` 是 vtable slot `+0x20`（slot 8），Init 为 slot 1；基类 Update slot `+0x0C` 仅表示基类实现，派生类可覆盖。

| 类别 | vtable / Init | TakeDamage | 静态 HP 观察与结论 |
|---|---|---|---|
| Player | `B6BDD0` / `0x755470` | `0x7729D0` | `+0x380/+0x384` 作为继承字段存在；Init 在 `0x755B81/8B` 将两者设为 1.0。实际 Player 伤害路径直接操作 hearts，故 generic 字段能否单独代表 Player 当前有效生命为 `UNKNOWN`；统一 schema 仅是用户接受假设，hearts 另行保留。 |
| NPC | `B67468` / `0x6B8B20` | `0x6D60A0` | 字段存在；配置虚调用可能先设 MaxHP，随后基类 Init 复制 Max 到 HP。所有配置分支的最终值/有效语义 `UNKNOWN`；通用生命字段归属按用户假设。 |
| Tear | `B64EAC` / `0x6660A0` | 继承 `0x6ACE60` | 派生 Init 路径将 MaxHP 写 0（`0x6661D8`）；字段存在，零上限语义 `UNKNOWN`，通用归属按用户假设。 |
| Projectile | `B64990` / `0x652B90` | 继承 `0x6ACE60` | 派生 Init 路径将 MaxHP 写 0（`0x652C8D`）；字段存在，零上限语义 `UNKNOWN`，通用归属按用户假设。 |
| Pickup | `B67F24` / `0x6E02F0` | `0x6EDDA0` | 字段存在。已导出的 Init 含多条 Entity Init 路径，但没有足够证据确定全部 Pickup 最终 MaxHP/HP；数值及有效语义 `UNKNOWN`，通用归属按用户假设。 |
| Familiar | `B6491C` / `0x60DE70` | `0x62CDC0` | Variant 67 有 100/100、Variant 100 有 9999/9999 的明确后置写入；部分后续路径会调整并同步。证明该字段不只属于 NPC，但不覆盖全部 Variant；统一归属按用户假设。 |
| Effect | `B63BDC` / `0x5CDB20` | 继承 `0x6ACE60` | 派生 Init 路径将 MaxHP 写 0（`0x5CDB89`）；字段存在，零上限语义 `UNKNOWN`，通用归属按用户假设。 |

以上分类证据见完整 [tx-01-64 类别导出](tx-01-64-category-coverage.md)、本轮原始导出的 vtable 与相关 Init 函数。Tear/Projectile/Effect 的零 MaxHP 和 Player 的 hearts 专用伤害路径是需要在后续实现中保留的观察；它们不被用户假设覆盖成相反的静态事实。

## Player hearts：与普通 HP 并列的原始模型

Player 心量以当前对象上的整数成员为主。以下具名 getter 在绑定处注册，函数直接读取对应成员并返回 32 位值；该证据不表示所有特殊角色都按相同规则解释、转发或合并这些值。

| Lua getter | getter VA | 当前字段 | 字段类型/解释 | 证据锚点 |
|---|---:|---:|---|---|
| `GetMaxHearts` | `0x42A310` | `+0x1340` | 32 位整数心容器字段 | 绑定于 `0x8698DC`；`GetEffectiveMaxHearts` 的计算路径也直接读取 `[this+1340h]`。 |
| `GetHearts` | `0x428B10` | `+0x1344` | 32 位整数 red-heart 当前量字段 | 绑定于 `0x8698CB`；Player damage 路径在 `0x7776C4` 取地址 `+1344h`、`0x7776CE` 写 Rotten 字段，后续调用 `AddHearts 0x758A70`。 |
| `GetEternalHearts` | `0x6DAD70` | `+0x1348` | 32 位整数 | 绑定于 `0x86990F`，getter 直接读取字段。 |
| `GetSoulHearts` | `0x5B1740` | `+0x134C` | 32 位整数 soul-heart 量字段 | 绑定于 `0x8698ED`；`AddSoulHearts 0x758D00` 写此成员，`AddBlackHearts` 末尾也调用它。 |
| `GetBlackHearts` | `0x5B1750` | `+0x1350` | 32 位位掩码字段，不是可直接累加到 SoulHearts 的数量 | 绑定于 `0x8698FE`；`AddBlackHearts` 在 `0x759100/0x759150` 用 `bts` 设置 bit，`0x75910A/0x759158` 写回 mask，`0x759165` 调 `AddSoulHearts`。因此黑心位与 soul-heart 数值是分开的两个表示。 |
| `GetBoneHearts` | `0x5B1700` | `+0x1D88` | 32 位整数 bone-heart 容器量 | 绑定于 `0x86A01C`；`AddBoneHearts 0x7CA840` 更新该成员。 |
| `GetGoldenHearts` | `0x5B1770` | `+0x194C` | 32 位整数 | 扩展 getter 注册；getter 直接读取成员。 |
| `GetBrokenHearts` | `0x65CF20` | `+0x1DA0` | 32 位整数 | 绑定于扩展 getter 注册；getter 直接读取成员。 |
| `GetRottenHearts` | `0x5B1710` | `+0x1DA4` | 32 位整数 | 绑定于扩展 getter 注册；getter 直接读取成员；Player damage 指令 `0x7776CE` 写 `[edi+1DA4h]`。 |

`0x8698CB..0x86992C` 和 `0x86A01C..0x86A04A` 是 getter 名称与函数地址的原始注册范围；每个 getter 的完整反汇编及对应直接成员读取在 [原始导出](../../../../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) 和复核导出中。具名 `int32` getter 与 `AddHearts`、`AddSoulHearts` 等算术/写入路径共同支持这些成员按整数单位处理；特殊角色的转换算法不在本研究范围。`BlackHearts` 的位语义由上述 `bts` 指令额外确认；其 32 位 signed/unsigned 展示约定未单独确证，不以 getter 的有符号 C++ 类型名推断高位解释。

### Player 计算与伤害路径

- `GetHealthType` 路径 `0x7CAFE0` 只按对象 `PlayerType +0x13C0` 选择健康类型。复核确认其不转发到 twin/subplayer 或其它 Player 指针。
- `GetEffectiveMaxHearts` `0x7CB060` 是计算值，不是 raw 字段：机器码保存原 `this`（`mov edx,ecx`），调用 `0x7CAFE0` 得到类型，再对**同一对象**读取 MaxHearts `+0x1340`。类型不是 1 或 2 的路径再加 `2 * BoneHearts(+0x1D88)`。Hex-Rays 的 `__int64/HIDWORD` 表示不构成 64 位返回值或指针路由证据。此 helper 的结果不能写回或替代 raw `MaxHearts`。
- Player 专用 `TakeDamage` `0x7729D0` 与基类 damage queue 不同；其函数导出直接显示对 red/soul/black/eternal/bone 等字段的读取或变更。raw 伤害子路径可锚定在 `0x777947`（Black mask）、`0x7779F6`（Bone）、`0x777DA9`（Soul）和 `0x777E0D`（Red）；`0x777FA8..0x777FD0` 有分支清零多组 heart 字段。例如 `0x7776C4` 的 `lea esi,[edi+1344h]`、`0x7776CE` 的 `mov [edi+1DA4h],eax`、`0x7776E1` 调用 `AddHearts 0x758A70`，随后还按 `PlayerType +0x13C0` 分支。故不能把基类 HP queue 的减法路径套在 Player hearts 上。
- 主要独立变化入口包括 `AddMaxHearts 0x7588A0`、`AddHearts 0x758A70`、`AddEternalHearts 0x758C20`、`AddSoulHearts 0x758D00`、`AddBlackHearts 0x758F90`、`AddBoneHearts 0x7CA840`。这些函数只作为关键变更锚点，不是全部 hearts writer 闭包。普通 HP `+0x380/+0x384` 与 hearts 各成员并列记录，不做换算。

现有仓库 `ReadJoinHealth` 使用 `+0x1340/+0x1344/+0x134C/+0x1D88`，与当前 IDA 的字段位置一致；这是实现与静态发现相符的交叉线索，不是本报告字段结论的来源，也不表示其它特殊角色算法已验证。

## GridEntity 与边界

GridEntity 是独立层级，不是普通 `Entity` 子类；其 vtable 在 `0xB686E8`，Grid Init 路径锚点 `0x709DB0`。现有 [tx-01-64 类别覆盖资料](tx-01-64-category-coverage.md)记录的独立成员包括 `+0x04` type、`+0x08` variant、`+0x0C` state、`+0x10` spawnCount、`+0x14` seed、`+0x18` variableData、`+0x1C` init、`+0x24` grid index、`+0x28` spawn frame、`+0x3C` collision。不能在 GridEntity 上套读 Entity 的 `+0x380/+0x384`；Grid 的生命/耐久表示在本任务证据中为 `UNKNOWN`。

## 剩余范围

1. 普通 HP 两个 raw float 成员、绑定、基类 Init 的 Max→current 复制和基类 Update 中的队列扣血，属于 `STATIC_CONFIRMED`；逐类将 generic 字段视作生命属于 `USER_ACCEPTED_ASSUMPTION`。
2. Player raw hearts 偏移、具名 getter、若干添加/伤害变更锚点以及 BlackHearts 位写属于 `STATIC_CONFIRMED`。特殊角色所有 health type 分支、转发/容器分配规则、全部 setter/writer 与完整心量算法仍为 `UNKNOWN` 或未穷尽。
3. Pickup 的最终初始化 HP、NPC 所有 config 分支、Tear/Projectile/Effect 的零 MaxHP 对有效生命的解释、采样时机的运行时边界、GridEntity 耐久模型均未在本次静态范围证明。
4. 本轮已完成 HP 字段布局、关键初始化和已知变更时机的静态研究及主智能体复核；上述未穷尽路径作为范围边界保留。其余字段、capture、运行时行为和整个操作 3 尚未完成。子智能体未自行验收，本轮唯一一次整体静态文档审阅由主智能体执行并另记。

