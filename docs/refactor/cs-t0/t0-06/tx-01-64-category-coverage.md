# tx-01-64：第二步类别覆盖与旧偏移补全

日期：2026-10-17。用户要求继续阅读指定会话的新内容，保留有用资料并补全缺失和错误。本次完整范围为 T0-06 操作 2 的静态类别映射，以及新会话偏移表在当前目标上的核对；不实现 capture，不研究操作 3 的字段稳定性或后续身份、状态摘要与写回。

遵循[现行规范](../../project-conventions.md)。过程中只做文件阅读、静态 IDA 分析和证据导出；全部目标完成后一次文档静态整体验收。不操作游戏、安装 Hook、运行查询/Lua、调用游戏 RTTI 或执行构建、代码测试。

## 1. 资料来源与完成边界

已接续阅读本地会话 `01a149e2-a48c-7cc2-883e-fd04d14e964d`：

- 新增的八类实体枚举/识别答复，turn `01a14a09-61f9-7f40-9053-dfba301e2cea`。
- 随后的“是否有相关地址偏移？”答复，turn `01a14a10-5f8c-7e92-9eaa-a1a5bc9ea677`。

[会话资料快照](../../../../artifacts/cs-t0/tx-01-64-static/reference-session.json)保留上述原始答复，标签为 `REFERENCE_VERSION_SOURCE`。它们讨论较旧的 REPENTOGON 源码，不能直接覆盖当前本体结果。[第一步交接](tx-01-63-entity-lifecycle.md)及其证据保持原样，本篇补充第二步的核对结果。

当前 IDA 仍为 Win32 `isaac-ng.exe`，image base `0x00400000`，横幅字符串 `v1.9.7.17.J460`。本篇函数地址默认是 **IDA VA**；RVA 为 VA 减 `0x00400000`，原始证据同时保存两者。未计算或校验哈希。

`STATIC_CONFIRMED`：八类的容器来源、识别线索及主要遗漏风险均已有静态登记；本次保存 39 个函数、8 段注册/引用调用点和 32 张 RTTI/虚表记录。没有运行时样本，不能据此宣称实际枚举器或动态场景覆盖完成。

## 2. 八类映射

下表的“补查”是为了记录来源和范围差异，不把所有内存对象并入活跃房间计数。`Type` 是 `Entity+0x28`；Grid 使用自己的布局。

| 类别 | 当前枚举来源与补查范围 | 当前识别依据 | 主要遗漏或误判 |
| --- | --- | --- | --- |
| Player | `Game+0x1BAA8` 的正常 roster；对照 main/persistent/update；补记 SubPlayer、Twin、backup、Esau 保留槽 | Type=1；Player RTTI；原生 ToPlayer `0x89D610` | roster 不能代表全部保留对象；负 index、Remove 待提交与备用对象不能直接过滤或计为活跃玩家 |
| NPC | 房间 main/persistent 成员与 update 视图；保存各来源 | NPC RTTI；原生 ToNPC `0x89D390` | 不用 IsActiveEnemy、敌我、可伤害或单一 Type=10 判类；`10..999` 不能作为穷尽身份契约 |
| Tear | 同上，保持 Type/Variant/SubType 原值 | Type=2；Tear RTTI；原生 ToTear `0x89CC10` | Projectile 是另一类；发射者/阵营筛选与短生命周期均可漏项 |
| Projectile | 同上 | Type=9；Projectile RTTI；原生 ToProjectile `0x89CAD0` | 不与 Tear 合并，不以来源、敌我或飞行表现替代类别 |
| Pickup | 同上；工厂池只作所有权线索 | Type=5；Pickup RTTI；原生 ToPickup `0x89D250` | 不能只筛道具底座；库存、Flip/Moving Box 保存记录与空闲池槽不算房间成员 |
| Familiar | 同上；wispEL 只作特定子集对照；owner 单独记录 | Type=3；Familiar RTTI；原生 ToFamiliar `0x89D110` | 只看 wisp 或单一 owner 会漏其它跟班；保存的 familiarData 不是实时对象 |
| Effect | 同上；`EntityList+0x60`（effectEL）只作派生索引对照 | Type=1000；Effect RTTI；原生 ToEffect `0x89D4D0` | 不能按可见性、碰撞或 effectEL 名称限制全集；画面中的纹理残留不代表还有实体 |
| GridEntity | 独立读 `Room+0x24` 的 448 槽，补查 `Room+0x724` 的 8 门槽并保留来源 | Grid 基类/派生 RTTI；`Grid+0x04` 为独立 Grid Type | 动态 EL 不含 Grid；不按旧数组长度、碰撞或损坏状态删项；门与 Grid 槽可指向同对象 |

原生 ToXXX 的上述函数是 **Lua 包装**：RTTI 转换成功后创建 userdata，否则压入 nil。本轮只反编译它们，没有在游戏中调用；它们不能直接作为只读 capture 的实现。

七类动态实体目前在 IDB 中的具体虚表和 Type Descriptor 如下。表中虚表不是跨版本契约；未来遇到其它虚表要保留原始分类并报告，不能直接删掉对象。

| 类别 | 虚表 VA | Type Descriptor VA |
| --- | --- | --- |
| Player | `0xB6BDD0` | `0xC5D6DC` |
| NPC | `0xB67468` | `0xC5D6AC` |
| Tear | `0xB64EAC` | `0xC5D900` |
| Projectile | `0xB64990` | `0xC5D70C` |
| Pickup | `0xB67F24` | `0xC5D7D4` |
| Familiar | `0xB6491C` | `0xC5D678` |
| Effect | `0xB63BDC` | `0xC5D5AC` |
| Grid 基类 | `0xB686E8` | `0xC5D7A4` |

原生转换分别把基类 Entity Type Descriptor `0xC5D5DC` 与对应派生 descriptor 传给 `_RTDynamicCast`。导出还保存 COL、类层次 descriptor 和基类列表，支持静态识别其继承关系。

## 3. 新会话偏移表的当前修正

这是静态寻址登记，不是字段稳定性、读线程或生命周期可用性保证。不同字段的变化量不同，禁止把整个旧结构统一平移。

| 基址与含义 | 新会话中的旧偏移 | 当前偏移 | 当前依据 |
| --- | --- | --- | --- |
| Game → Room* | `+0x18300` | `+0x18300` | 第一步 Spawn、原生枚举、Player 发布共同读取 |
| Game 内嵌 PlayerManager | `+0x1BAA8` | `+0x1BAA8` | roster 读写与 Esau 全局槽引用 |
| Game → EntityFactory* | `+0x25AF8` | **`+0x26580`** | `0x6F3060` 取池 owner；第一步 Spawn 使用同一字段 |
| Room 内嵌 EntityList | `+0x1218` | `+0x1218` | 第一步更新调用点及原生 GetRoomEntities |
| Room Grid 槽 / 门槽 | `+0x24 / +0x724` | `+0x24 / +0x724` | `0x436060 / 0x436020 / 0x7EEA90` |
| Room width / height | `+0x0C / +0x10` | `+0x0C / +0x10` | `0x4176F0 / 0x40C2E0`；`0x4566A0` 返回乘积 |
| PlayerManager roster | `+0`，vector | `+0`，vector | `0x9B9CD0 / 0x9BEE80`；begin/end/capacity 分别 `+0/+4/+8` |
| PlayerManager Esau 保留槽 | `+0x80`，4 指针 | `+0x80`，4 指针 | `0x9BCAA0` 循环四槽；`0x7ACA76` 加 Game `0x1BB28` |
| Player 角色类型 | `+0x13BC` | **`+0x13C0`** | GetPlayerType `0x4253B0`：`8B 81 C0 13 00 00` |
| PlayerIndex | `+0x1610` | **`+0x161C`** | 创建、替换 roster 与置 -1 的本体路径共同使用 |
| Player Twin / EntityPtr | `+0x1E4C` | **`+0x1E68`** | GetOtherTwin `0x417820`：`8B 81 68 1E 00 00`；恢复/替换时登记引用 |
| Player backup | `+0x1E50` | **`+0x1E6C`** | `0x9BCBC4`：`89 88 6C 1E 00 00`；恢复创建并建立反向关联 |
| Player 替换期对方关联 | `+0x267C` | **`+0x26C8`** | `0x9BEEC1 / 0x9BEECE` 写双方，正常返回路径清零 |
| Familiar owner | `+0x410` | `+0x410` | Familiar Init `0x60DE70` 写 owner；ReplacePlayer 按 owner 处理跟班 |
| Familiar 池起点 / stride | `+0x130008 / 0xF30` | **`+0x130008 / 0xF48`** | `0x6F2A10` 数组构造与 `0x6F3060` 数组析构均为 64 × `0xF48` |
| Pickup 池起点 / stride | `+0x357630 / 0x6F0` | **`+0x357C30 / 0x6F0`** | 同上，512 × `0x6F0` |

补充此前未列出的玩家关系：原生 `GetSubPlayer` 为 `0x6DAE10`，读取 **`Player+0x1D98`**（`8B 81 98 1D 00 00`）。它与 Twin、backup 不是同一字段，应分别登记并在同次观测内去重。

PlayerType 是角色种类，Entity Type=1 是实体类别，两者不能混用。PlayerIndex 和指针仍只是当前生命周期关联，本轮没有将它们改成网络身份或 T0EntityKey。

## 4. 玩家枚举的范围补全

正常 roster 读取应把 `(end-begin)/4` 作为条目数，使用 vector 的三个指针解释；它不是 EL，`+0x0C` 不是 size。T0-05 实现中的支持人数限制也不能被当作完整原生 roster 布局。

`0x9BCAA0` 的恢复路径会为需要备用形态的对象单独分配并构造 Player，写入 `+0x1E6C`，随后恢复其状态；同一函数另遍历 Manager `+0x80` 的四个保留槽，必要时分配和初始化 Player。`0x7AC7E0` 的相关分支从 `Game+0x1BB28` 查找四槽，建立对象后调用 Player 替换。这些是实际 Player 对象路径，不能因不在正常 roster 就静默丢弃，也不能直接加进当前活跃玩家数。

`0x9BEE80` 的 ReplacePlayer 路径进一步说明：

- 会交换或替换 roster 中的对象及 PlayerIndex。
- 维护 Twin 的受追踪引用，转接 backup 关系。
- 在调用窗口中把双方地址写入 `+0x26C8`，正常返回时清零。这是临时调用关联，不是又一份持续保留玩家名单；异常传播下的清理未在本轮证明。
- 原对象可被置 invalid、exists=false、PlayerIndex=-1，新对象被发布；只按 index、dead 或单次 roster 查询判断生命周期会遗漏变化。

本次登记的枚举范围因此分为：房间成员来源、正常 roster、保留/备用关系、待提交移除状态。对同一指针保留多个来源标记；指针仅用于同次静态设计中的本地关联，不能据此定义跨 tick 或跨进程身份。

## 5. 查询与派生列表的遗漏修正

### 5.1 FLAG_NO_QUERY 有普通分支和例外

原生 `FindByType` Lua 包装 `0x8707B0` 调用 `EntityList::QueryType 0x41AF60`。

- 普通匹配分支检查 `Entity+0x16C` 的 `0x4000` 位，即 64 位 flags 的 bit 46；该位为一则不加入结果。
- IgnoreFriendly 路径还结合低 32 位 `0x20000000`（bit 29）过滤。
- 缓存命中可以直接返回已有视图。
- **Type=3、Variant=206 的专用分支直接取 wispEL；Subtype=-1 时直接返回共享描述结构，否则按 subtype 拷贝子集。该分支没有普通分支的相同 flags 判断。**

因此，旧资料的“查询可能漏项”可以保留；“每种 FindByType 分支都会以同一规则过滤 NO_QUERY”则不成立。也不能反过来借这个例外把查询当作完整枚举。

`FindInRadius` 包装 `0x8704F0` 调用 `0x41A790`，按 partition 汇集候选，再检查 bit 46 和距离/实体半径；Effect partition 还从 EL `+0x60` 的派生列表取候选。它不是全房间对象清单。

这两类查询都可能修改临时列表、缓存、分配状态或查询统计；本轮没有实际调用，后续只读 capture 不应直接复用这些查询函数。

### 5.2 effectEL 不是 Effect 全集

当前 `0x419E70` 先清 EL `+0x60` 的 size，再遍历 updateEL；其相关分支在 `Entity+0x188 != 0` 的前置条件下，把 Type=1000 的对象加入该派生列表。由此可明确修正旧资料中仅凭成员名称理解 effectEL 的风险：它是处理过程中的子集，不能用于保证全部 Effect 覆盖。

同理，wispEL 仅覆盖特定 Familiar；本体发布逻辑也会按 flags 条件决定是否加入 updateEL。因此本步将 main/persistent 的房间成员与 update/render 等视图分别登记，不仅使用一个带过滤的列表。

## 6. Grid 与门槽的当前补全

当前 `GetGridEntity 0x436060` 接受索引到 `0x1BF`，Room 析构 `0x7E9BA0` 遍历 448 槽，确认旧声明 `[0x1BF]` 不能作为完整容器长度。width×height 是房间逻辑尺寸，不能代替固定存储槽数；空槽可以记录为空，非空槽不能只因处在通常逻辑尺寸之外就静默跳过。

`GetDoor 0x436020` 读取 `Room+0x724+4*slot`。本步门槽范围为 `0..7`。其机器码边界判断还会允许参数 -1，但本轮不据此定义额外门槽，也没有实际调用该函数。

当前 `MakeDoor 0x7EEA90` 已补全旧会话未确认的关联：

1. 从 `Room+0x744+4*slot` 取得对应 grid index。
2. 从该 Grid 槽取对象，RTTI 判断是否为 GridEntity_Door。
3. 必要时创建 Door、释放旧 Grid 对象，并把新 Door 写入对应 Grid 槽。
4. 初始化后把 **同一个指针** 写入门槽 `Room+0x724+4*slot`。

所以此原生建门正常路径中，门槽是 Grid 对象的另一个视图，不能双计。补查门槽仍有价值：保留来源、确认关联，发现其它生命周期窗口中的异常差异；本轮没有把这一正常路径外推为所有移除和转场期间的普遍不变量。

Grid Type 位于自身 `+0x04`；动态 Entity Type 位于 `+0x28`。门的 Grid Type=16 不代表动态 Entity Type=16 的 NPC。两套编号必须按对象域解释。

旧 REPENTOGON Lua 扩展还把 Effect Variant=136 的 `_varData` 解释为 GridEntityDesc。这作为“搬起后的视觉对象不能继续算作 Grid 槽成员”的参考线索保留；本轮没有核实当前该 Variant 的完整描述字段契约，不将旧扩展解释直接并入当前字段 schema。

## 7. ToTear 与未知类别处理

旧会话说的“没有 ToTear”只适用于所阅读的 C++ 内联包装集合。当前原生注册点 `0x86901B..0x869022` 注册 ToTear（注册辅助函数 `0x87FC70`），Lua 包装 `0x89CC10` 实际执行 Entity → Entity_Tear RTTI 转换。因此不能说当前游戏 API 不提供 ToTear。

NPC 也不能仅以 Type 数值范围判定：`0x6F0090` 中既有固定类别分支，也有通向 NPC 池的默认分支；Type>9 还包括 Effect、Text 等其它类。八类之外的 Bomb、Slot、Laser、Knife、Text、Dummy 等当前 RTTI 条目已随证据保留，不能为了八类表而删掉它们。

本次采用以下**覆盖登记规则**，尚未写成运行时代码：

- 同时保存对象域、容器/关系来源、原始 Type/Variant/SubType（Grid 使用自己的布局）、RTTI 分类依据与生命周期状态。
- 不在要求的八类中但已经识别的类，报告“已知其它类”。仅 Type 未登记而 RTTI 类已明确时，分别保留已知类和未知 Type；分类依据不足时报告 UNKNOWN，并保留原始分类信息。
- 固定 Type 与已知派生类关系矛盾时，报告分类冲突；不能仅凭 Type 强制转换或丢弃对象。NPC 不设一个未经证明的硬数值上界。
- `_dead`、`_exists`、`_valid` 分别记录；不把死亡、待摘表或保留对象混成一个“无效即跳过”的条件。
- 同次观测中去重时合并来源；池槽同址复用的跨次身份问题留给后续操作 4。
- 两次采样之间生成又移除的对象可能完全不可见。静态快照的类别覆盖不能冒充完整事件覆盖，创建/Init/Remove 事件方案尚未实现。

## 8. 产物、剩余工作与验收

[IDA 原始证据](../../../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json)保存 39 个函数的伪代码、完整函数 chunks 的指令和原始字节、VA/RVA、交叉引用及截断标记，另有 8 段关键调用点和 32 张 Entity/GridEntity 虚表及 RTTI 层次资料。

[导出脚本](export_tx_01_64.py)仅静态读取，复用[第一步脚本](export_tx_01_63.py)的指令格式化 helper；以非 main 名称加载，不触发第一步导出。默认输出到 `artifacts/cs-t0/tx-01-64-static`，重复导出需显式传入新的绝对目录，不覆盖既有证据。

完成的是操作 2 的**静态覆盖规格与偏移修正**。未知或尚未执行的部分包括真实主线程 capture、生命周期窗口与派生类/Mod 扩展的实际覆盖、异常/并发读取安全，以及操作 3 的字段稳定性；没有把这些写成 PASS。运行时代码、Hook、C/S 或原版状态均未被本项修改。

本次唯一整体验收见[文档静态审阅](tx-01-64-review.md)。第一步原始报告和两个参考会话答复均保留，不用本轮修正覆盖其历史语境。
