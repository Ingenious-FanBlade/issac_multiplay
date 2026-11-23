# 第三方 Mod 适配：后续方案

始于 2026-11-10（Asia/Singapore）。状态：**已实现、待 E2E**。声明式接口、原生显示补项和专用夹具已接入，统一实机验收归本方案第 14 步。该方案从[原生二进制重构](binary-state/README.md)迁出，独立于该重构的原版验收。

2026-11-22 补记：公开接口方案草案见 [Mod 联机接口方案](../../mod-api.md)，路线为 Mod 作者声明、联机层提供基础设施、协商不一致就拒绝连接，待用户审阅。本文"仍未确定"中"仅保留具名 adapter 还是提供公开注册接口"一项以该草案为准；其余内容不变。同日起"Mod 值编码草稿"不采用（D4），声明类型的采集与编码见新增的同名一节。

2026-11-23 补记：对照 REPENTOGON 接口，原生同步补项和核对项记在"具名适配交接"下的"REPENTOGON 接口对照"一节。同日用户定为提供公开声明式接口（mod-api），具名 adapter 只作内部诊断手段；mod-api D12 定为客机进度查询以 Host 存档为准。以下"实施顺序"取代 mod-api 第 11 节原顺序。

## 当前顺序与范围

用户决定当前先跑通原版内容的采集、传输、Client 原生应用和统一整体验收。官方 REPENTOGON 加载器、J273 目标及原版需要的官方扩展 owner 继续沿用；“原版”指内容范围，不改变加载方式。本项目自带界面与 E2E 夹具继续按原职责管理，不构成第三方 Mod 兼容声明。

当前适配范围为[公开声明式接口](../../mod-api.md)、Lua 回调分级、Mod 状态、意图与本文的原生显示补项。StageAPI、自定义角色与武器等具名适配仍按各自范围推进。本项目自带界面和夹具按其原有职责管理。

## 迁入内容与已有事实

| 原位置 | 迁入内容 |
| --- | --- |
| 二进制重构第 8 步原 8.4 | 只读 Lua adapter、自描述二进制状态、分块比较、预算及 schema 完整性 |
| 第 9 步原 9.7 | Mod 值格式草稿、共享节点与循环引用编码 |
| 第 10 步低频采集、第 14 步产品边界 | Mod 低频读取及 `domains/mod` 正式来源的设想 |
| 第 15 步原 E31 及补充矩阵 | 循环／共享／超预算／adapter 出错等适配验收；原 E31 在本轮不执行、不计通过，其余编号不变 |
| Player／HUD 跨域交接 | 第三方回调最终值、角色／武器／资源、额外 History 展示等具名适配需求 |

已有 Mod 观测使用具名只读 adapter（`mod/observation/isaac-cs-observation/domains/mod/collect.lua`）和有界对象图（`domains/mod/graph.lua`），已有 StageAPI adapter。旧 `stream-catalog.json` 的 mod 域有 12 个公开成员，其中 2 个 `ReplicatedState` 成员原计划映射到具名 adapter schema。它们是已有来源／目录事实，不表示新的二进制传输和 Client 安装已经完成。

[tx-04-14 实现与验收](observation-expansion/tx-04-14-implementation.md)、[Mod 记录／适配合同](observation-expansion/run-mod-schema.md)及[StageAPI 观测设计](observation-expansion/run-presentation-mod-observation.md#51-stageapi自定义房间与customgrid必测适配)保留其原时点事实、失败和覆盖限制。观测成功不等于已能同步任意 gameplay Mod。

## 状态来源与完整性草稿

- 具名 adapter 在合法 ordinary 主线程窗口低频只读 Lua 状态；不额外推进随机、创建玩法状态、调用玩法生产器或写持久数据。网络／后台线程只处理自有数据。
- Lua 自定义状态没有可直接采用的原生字节布局，按 adapter 声明的 schema 判断完整性。未知闭包／私有对象不得宣称自动完整捕获；原生对象仍须遵守字节归属与已核生命周期规则。
- Host 由 C++ 读取 adapter 的有界输出，直接编码为二进制，不再构建 JSON 树；Client 解码后由对应适配器安装状态／展示结果。不得在 Client 重演 Host gameplay Lua 来补值。
- 每个 adapter 的输出作为一个块，用 memcmp 判断变化。稳定输出的顺序、共享节点编号与字节规范化仍须定稿，避免无变化时反复重发。
- 沿用 `graph.lua` 的节点、项数、字节、深度预算，并单独计时；超预算、抛错、缺来源或读取超时必须报告具名缺口，不能截断后宣称完整。原草稿提出超时降频，具体失败后的发布／ACK 语义仍待设计。（2026-11-23：已按原生采集定稿，不设时间上限、不降频，见 mod-api 5.4 节。）

## Mod 值编码草稿

> **2026-11-22 不采用**：[Mod 联机接口方案](../../mod-api.md) D4 定为只用强类型，取消 `any`，本草稿的带标签编码、共享节点回指和节点／深度预算不再实施，由下一节"声明类型的采集与编码"取代。原文保留如下，仅作查证。

以下是从第 9 步迁出的草稿，不属于当前原版线格式；后续实施时再与布局版本、成员／资源表及有界解码规则接合。

| +0x0 u8 tag：0=nil，1=false，2=true，3=i32，4=f64，5=字符串，6=表，7=实体引用，8=Grid 引用，9=Vector，10=Color，11=KColor，12=共享节点回指 | +0x1 值：按 tag 编码，i32 为 4 字节，f64 为 8 字节，字符串为 u16 长度＋字节，表为 u16 项数＋键值对，引用为 u16 成员序号，回指为 u16 节点序号 |
|---|---|

表节点按首次出现顺序编号，再次遇到同一节点写 tag 12，以表示共享或循环。嵌套深度不得超过 adapter 预算。Vector／Color／KColor 的具体展开、非法 tag／回指／浮点／长度处理及单行生成表仍须定稿；本草稿不授权复制原生地址或 Lua closure。

## 声明类型的采集与编码

日期：2026-11-22。状态：设计草案，待用户审阅；没有实现、构建或测试。同日补充第 7.7 节"接入客机管线"，修订第 7.2、7.3、7.5 节，用户已定 M3—M8（M5 于 2026-11-23 定）。适用于 [Mod 联机接口方案](../../mod-api.md) 第 5 节的 `State`、`EntityData`、`EntityState` 声明，类型见该文 5.2 节。思路与实体[平铺记录](binary-state/10-flat-record-encoding.md)相同：声明期把类型树编译成固定布局和读取程序，Host 每帧照程序取值，直接写进平铺缓冲区；客机按同一份程序校验字节、更新影子状态并写回 Mod（第 7 节）。

### 1. 编译：类型树变成节点数组

声明在首次握手时冻结（mod-api 4.2 节），此时 DLL 为每条声明编译一次，把类型树按前序排成**节点数组**，存在 C++ 侧该声明的登记项里，进程内常驻，不上线、不落盘。两端的声明文本在握手时逐字比较，所以编译结果相同。

```
节点 = kind      BOOL／INT／FLOAT／NUMBER／STRING／VECTOR／COLOR／KCOLOR／ENTITY／PLAYER／STRUCT／LIST／MAP
       off       本节点的槽位在所属定长区里的偏移
       key       STRUCT 字段名，预先驻留为 Lua 字符串，存 registry ref；运行时不再新建字符串
       children  STRUCT 的字段节点范围；LIST／MAP 的元素（及键）节点
       max       STRING 的字节上限，LIST／MAP 的项数上限
       fixed     本节点定长部分的字节数
       worst     本节点最坏情况下的总字节数
```

同一份数组既是**布局**（数据放在哪、多长），也是**程序**（从 Lua 怎么取）。Host 读取、客机解码、客机默认写回是三个解释循环，走同一份数组。

### 2. 布局规则

- **定长槽**：`bool` 1 字节；`int`、`float` 4 字节；`number` 8 字节；`vector` 8 字节，`color` 28 字节，`kcolor` 16 字节；`entity` 2 字节（成员号，Null 为 `0xFFFF`）；`player` 1 字节（ordinal，Null 为 `0xFF`）。均为小端。
- **`struct` 原位展开**：不单独成块。字段按声明顺序展开进所属定长区，嵌套的 `struct` 继续展开。每个 `struct` 的定长区开头是一张存在位图，每个字段 1 位，⌈字段数/8⌉ 字节；字段不存在时位为 0，槽位填 0。整个嵌套 `struct` 不存在时，父级对应位为 0，它的全部槽位填 0。
- **容器槽**：`string`、`list`、`map` 在定长区只占一个 u16（字符串为字节数，容器为项数），内容放在定长区之后。
- **深度优先、连续存放**：一个节点的定长区在前，紧接着按槽位顺序放它各个容器的内容；容器中每个元素同样是"元素定长区 + 元素自己的容器内容"，递归下去。因此每个元素、每个键值对都是一段连续字节。
- **map 的项**：`[键][值]`。`int` 键为 i32；`string` 键为 u16 长度加字节，字节数不超过声明的 `keymax`（默认 64）。各项按键升序：整数键按数值，字符串键按字节序。

例：

```lua
type = { struct = {
  Points = "int",
  Mecha  = { list = { max = 6, of = { struct = { Chest = { string = { max = 16 } }, Left = "int", Max = "int" } } } },
  Void   = { map  = { key = "int", of = "int", max = 64 } },
} }
```

```
定长区（9 字节）
 +0  位图 u8              bit0=Points bit1=Mecha bit2=Void
 +1  Points  i32
 +5  Mecha   u16 项数
 +7  Void    u16 项数
之后
 Mecha[1..n]：每项 [位图 u8][Chest u16 长度][Left i32][Max i32][Chest 字节]
 Void：n 项按键升序，每项 [键 i32][值 i32]
```

**最坏字节数在编译期算出**：`worst = fixed + Σ(字符串 max) + Σ(容器 max × 元素 worst)`，map 的元素 worst 含键（字符串键为 2 + `keymax`）。类型不能递归，树有限，所以总能算出。`worst` 超过单条消息上限（64 KiB）的声明在声明时就报错。原草稿的节点数、深度预算因此不再需要；每声明的字节上限默认取 `worst`。

### 3. Host 主线程：执行读取程序

在 World 采集窗口（mod-api 5.4 节）内，按各声明的 `rate` 逐条执行：

1. `lua_pcall` 调 `get`（没写 `get` 时取 `mod[key]` 或 `GetData()[key]`），结果压栈。
2. 解释执行节点数组，写进本帧暂存区（按帧复用的线性缓冲区）。定长区按 `off` 直接写，容器内容只往后追加。
3. 各节点的取值与规范化：

| 节点 | 读法 | 接受 | 否则 |
| --- | --- | --- | --- |
| `STRUCT` 字段 | 驻留键 `lua_gettable`（带 `__index` 的对象也能用） | 值为 `nil` 时存在位置 0 | — |
| `BOOL` | 栈顶 | 布尔 | 类型不符 |
| `INT` | 栈顶 | 整数子类型且在 i32 范围内；或浮点值恰为该范围内的整数 | 类型不符 |
| `FLOAT`／`NUMBER` | 栈顶 | 整数或浮点，转为 f32／f64 | 类型不符 |
| `STRING` | 栈顶 | 字符串，字节数 ≤ max | 超出为超限，非字符串为类型不符 |
| `VECTOR`／`COLOR`／`KCOLOR` | 校验 userdata 元表后取分量 | 对应 userdata | 类型不符 |
| `ENTITY`／`PLAYER` | 实体 → 成员号；身体 → ordinal | 实体或 `nil` | 不在成员目录或已离开时写 Null 并计数 |
| `LIST` | 从 1 起 `lua_geti`，读到第一个 `nil` 为止 | 项数 ≤ max | 超限 |
| `MAP` | `lua_next` 原始遍历（不经 `__pairs`，所以目标必须是真表） | 键类型与声明一致，字符串键字节数 ≤ keymax，项数 ≤ max | 类型不符或超限 |

4. **计数先占位后回填**：`LIST`、`MAP` 读完才知道项数，先在定长区写 0，循环结束后回填。
5. **map 不在主线程排序**：按遍历顺序写下各项，同时记一张 `(键, 起点, 长度)` 的小索引，交给编码线程。
6. 任一节点类型不符、超限或 `get` 抛错，丢弃本条的本帧结果，记录沿用并计数（mod-api 5.4 节、第 9 节）。只影响这一条记录。

暂存区和索引都是 DLL 自有内存，主线程写完即交给编码线程；之后不再持有任何 `lua_State*` 或 Lua 值。

### 4. 编码线程：规范化与比较

1. 按索引对每个 `map` 排序，再按新顺序重新拼接各项字节段，得到**规范字节**。各项本身是连续字节段，值里嵌套的容器随项一起移动，不用重新解析。Lua 表的键本身唯一（整数值的浮点键会归一为整数），不会出现重复键。
2. 规范字节与该记录上一版做 memcmp，变了就更新"最后变化版本"，按 Peer 组帧（mod-api 5.5 节）。首版不做记录内部的字段级增量。

### 5. 性能

主线程开销与"声明的字段数 + 容器元素总数"成正比，每个字段一次驻留键的哈希查找，每个元素一次 `lua_geti`／`lua_next`。按每次 Lua API 调用 10—30 ns 粗算（未实测），1000 项、每项 3 个字段的 `list` 约需 0.1—0.2 ms；不设时间上限，耗时计入诊断（mod-api 5.4 节，2026-11-23 取消原定的每帧 1 ms 共用预算）。

Lua 表没有修改标记，`rate = "world"` 的声明即使没变也要整条读完才能判断，记录内任意一项变化都要整条重发。对策写在 mod-api 5.3 节"复杂表的写法"和 5.4 节：大容器单独声明并配 `rate = "mark"` 与 `MarkDirty`；诊断模式提示 `max` 较大却按 `"world"` 读取的容器。不采用在 C++ 中直接遍历 `Table`／`Node` 内存的做法：它与 Lua 版本强绑定（当前为 REPENTOGON 编译成 `Lua5.3.3r.dll` 的 Lua 5.4.6），GC 还会重建 `node[]`。

### 6. 以后可扩展的部分

- **字段级增量**：定长区偏移固定，可直接沿用平铺记录的字段表与差异位图（L8）；每个容器连同其内容整体算一个字段。
- **容器按元素增量**：元素类型不含容器和字符串时元素定长，第 i 项偏移为"起点 + (i−1) × 元素大小"，可按元素比较。只省网络，不省主线程读取。

### 7. 客机：校验、影子状态与安装

客机分两步：后台线程只碰字节，做结构校验并更新影子状态；主线程在 Manager 边界内把字节变成 Lua 值写进 Mod（时机见 mod-api 5.6 节）。两步走同一份节点数组。

#### 7.1 后台：结构校验

解码线程收到 World 帧的 Mod 段后，对每条记录按节点数组从头顺序读，同时校验：

| 检查 | 不合法的情况 |
| --- | --- |
| 记录头 | Mod 序号、声明序号越界；目标与作用域不符（global 不是 `0xFFFF`，player 的 ordinal 不存在） |
| 剩余字节 | 任一步要读的字节超出记录末尾；读完后还有剩余字节 |
| 存在位图 | 字段数之外的填充位不为 0；位为 0 的字段槽位不全为 0 |
| 标量 | `bool` 不是 0 或 1 |
| 容器 | 字符串字节数 > max；list、map 项数 > max；字符串键字节数 > keymax |
| map 顺序 | 键不是严格升序（同时排除了重复键） |

前四类之外的值不在后台判断：浮点照收，NaN 也照收；实体成员号是否存在留给主线程，因为成员表在主线程。

任一检查失败，**整条记录丢弃**：影子状态保留上一版，记日志并计数。两端布局相同，校验失败只会来自实现错误或数据损坏，不请求 Resync（与 mod-api 5.6 节一致），也不影响 ACK（C2）。校验通过的记录才进入影子状态。

#### 7.2 影子状态

> 2026-11-22 修订：原文用"版本号与已安装版本号"判断待安装，改为下面的不可变快照加已安装表（M1、M2，见第 9 节）。原因是完整基线（Resync）到来时每条记录都带新版本号，按版本号判断无法区分"值真的变了"和"基线重发了同一个值"，而 Host 版本号在作用域切换后是否连续也没有保证。

**影子**是一张有序表，键为记录键（Mod 序号, 声明序号, 目标），值为指向规范字节块的 `shared_ptr<const 字节>`：

- 记录键就是线上记录头里的三个数（mod-api 5.5 节）。后台用它找到该声明的节点数组做校验、核对目标与作用域是否相符、把增量合并进影子，主线程用它固定安装顺序。
- 每收到一帧 Mod 段，只为变化的记录换新的字节块，其余沿用原指针（结构共享）；然后封存成不可变的 `ModShadowRef`，与原生影子（`effective`）一起放进同一个 `ClientStatePreparedTarget`。主线程装哪份 World，就装那份 World 的 Mod 快照；被更新 World 顶掉的旧帧连同 Mod 快照一起丢弃，影子是累积的，变化不会丢失（M1）。
- 成员目录增量显示成员离开时，后台把该成员的 entity 作用域记录从下一份快照中删除。
- 完整基线到来时，影子整体换成基线内容。

主线程另有一张**已安装表**，键相同，值为上次成功写入时的字节引用，另带两个标记：未解析引用（上次安装时有实体引用找不到，§7.4）和待实体（目标实体尚未安装）。已安装表只在写入成功时更新。

- 实体重建时，该实体的 entity 作用域记录按第 7.7.4 节重新安装，不需要 Host 重发（mod-api 5.6 节）。
- 实体离开时删除它的已安装条目；客机场景结束时全部清空，并调用 `reset`（第 7.7.5 节）。
- 诊断模式的只读检查（mod-api 5.6 节、D6）从已安装引用取值重写。

影子只是字节，后台线程可以写；主线程只读快照，交接沿用 World 快照现有的交接方式。节点数组在握手冻结后不可变：后台只读其中的纯数字字段（kind、off、max、fixed），含 Lua registry ref 的字段（驻留键、`set`、构造器）只有主线程使用。

#### 7.3 主线程：字节变成 Lua 值

待安装的记录（安装集，第 7.7.2 节）按 global、player、entity 三组，每组内按"Mod 序号、声明序号、目标"的顺序逐条安装；entity 作用域的记录在对应实体完成原生安装之后再装。每条记录单独 `pcall`。

**写了 `set` 时**，按节点数组构建一个全新的 Lua 值，再调用 `set(v)`、`set(player, v)` 或 `set(ent, v)`：

| 节点 | 构建 |
| --- | --- |
| `STRUCT` | `lua_createtable(0, 字段数)`；只写存在位为 1 的字段（`lua_rawset`），缺失字段不写，读出来就是 `nil` |
| `LIST` | `lua_createtable(n, 0)`，依次 `lua_rawseti` 1..n |
| `MAP` | `lua_createtable(0, n)`，按字节里的顺序逐项 `lua_rawset` |
| `BOOL`／`INT`／`FLOAT`／`NUMBER`／`STRING` | `lua_pushboolean`／`lua_pushinteger`／`lua_pushnumber`／`lua_pushnumber`／`lua_pushlstring` |
| `VECTOR`／`COLOR`／`KCOLOR` | 调用游戏的构造函数（全局 `Vector`、`Color`、`KColor`，加载期取一次存为 registry ref） |
| `ENTITY` | 成员号 → 本地实体 → 压入实体对象；`0xFFFF` 为 `nil` |
| `PLAYER` | ordinal → 该参与者的身体（同 `GetPlayerByParticipant`）；`0xFF` 为 `nil` |

新建的表没有元表，所以用 `rawset`，不会触发任何元方法。顶层空值（字节数为 0）直接调用 `set(nil)`。

**没写 `set` 时**，按 mod-api 5.3 节"默认写回"规则，**直接从字节写进现有的目标表**，不先建一张完整的新表再合并：

- 目标是 `mod[key]`（EntityData 是本地实体的 `GetData()[key]`）。字节数为 0 时写 `nil`。
- `STRUCT` 且目标已是表：逐个字段处理。存在位为 1 时，标量直接赋值；字段本身是 `STRUCT`、`LIST`、`MAP` 时，取出现有的子表递归处理（子表不是表时新建）。存在位为 0 时写 `nil`。未声明的字段和元表不动。
- `LIST`、`MAP` 且目标已是表：先清空（`lua_next` 收集全部键再逐个置 `nil`），再按字节重填，表本身不换；元素按"写了 `set`"那张表新建，`struct` 元素不逐项合并。
- 目标不是表：新建一张表，按上面的规则填入后赋值。
- 写现有表时用 `lua_settable`（尊重 `__newindex`，与 Host 读取用 `lua_gettable` 对称）；写新建的表用 `rawset`。

安装成功后，已安装表的字节引用更新为快照中的引用。`CLIENT_STATE_CHANGED(key, target)` 不在每条写完时派发，而是本批全部记录写完后统一派发，并且只为内容确实变化的记录派发（第 7.7.3 节，M4、M8）。

同一个值可能被多次写入（完整基线时全量重写，M8），`set` 必须保证同值重写没有副作用。

#### 7.4 实体引用找不到

客机上，被引用的成员可能还没安装（同一帧里它的实体排在后面），或已经移除。此时该处写 `nil`、计数，并在已安装表的该条目上打上**未解析引用标记**（§7.2）。之后每帧重试安装这条记录，直到引用全部找到、记录有了新版本，或者被引用成员在成员目录里确认离开（此时清掉标记，保留 `nil`）。

#### 7.5 失败的粒度

| 失败 | 处理 |
| --- | --- |
| 后台结构校验失败 | 整条丢弃，影子保留上一版，计数 |
| `set` 抛错 | 跳过这条，已安装引用不更新，计数；等字节变化或完整基线到来时再装，不每帧重试（Lua 报错不会因重试消失） |
| 默认写回途中出错（如 `__newindex` 抛错） | 同上。已写入的部分不回滚，目标表可能只更新了一半，下一次写入时整条重写 |
| 找不到目标实体（entity 作用域） | 跳过并打上"待实体"标记，等实体安装后按第 7.7.4 节重新安装 |

任何失败都只影响这一条记录，不影响同帧的其他记录和原生安装（C5、C9）。

#### 7.6 成本

主线程的开销与"本帧变化的记录的字段数 + 元素数"成正比；没有变化的记录不碰 Lua，只做一次 C++ 指针比较。完整基线那一轮例外，全部记录重写一遍（M8），Resync 很少发生，这一轮开销可以接受。写了 `set` 的记录每次写入都会新建表，产生的垃圾交给 Lua GC；没写 `set` 的走原地写回，只有容器元素会新建表。客机每帧的安装耗时按 Mod 统计，进入诊断，永不设上限（M6）。

#### 7.7 接入客机管线

日期：2026-11-22。状态：设计草案，没有实现、构建或测试。本节说明前面各小节的规则如何接进现有客机流程（[Client 端设计](../../client.md)第 4、5 节）。M1、M2 是方案内容，已写进正文；M3—M8 已由用户决定，见第 8 节。

##### 7.7.1 总流程与插入点

```text
连接线程（ClientStateReceiver）                     主线程（Manager 边界，client.cpp 每轮）
───────────────────────────────                   ─────────────────────────────────────
DecodeStateFrame
 ├ 原生段 → 原生影子（不变）                         c.binary->Pump()
 │          计划内离开／重建名单（M3）                 ├ 取最新 World 批次
 │                                                  ├ ★ Mod 预通知：REMOVED（M3）         ← 新增
 └ Mod 段 → 结构校验（§7.1）                          └ World 原生安装、发布
           → 合并进 Mod 影子                         ★ Mod 安装阶段（本节）               ← 新增
           → 封存不可变快照 ModShadowRef                ├ 实体生灭差集 → REMOVED／INSTALLED
             挂进 ClientStatePreparedTarget            ├ 逐条比较 → set／默认写回（§7.3）
 → AppliedAck（与 Mod 无关，C2）                       └ 批末派发 CLIENT_STATE_CHANGED
 → 邮箱                                             c.scene.AdvanceDisplay()
                                                    Game::Render（绘制回调读到同一帧状态）
```

插入点有两处：

- **预通知**：`ClientBinaryScene::Pump` 取到本轮要装的 World 批次之后、原生安装第 3 步（删除）之前，派发计划内的 `CLIENT_ENTITY_REMOVED`（M3，第 7.7.4 节）。此时本批还没有任何原生写入，场景仍是上一次发布的状态，调用 Lua 是安全的。
- **安装阶段**：`game/client/session/client.cpp` 中 `c.binary->Pump()` 之后、`c.scene.AdvanceDisplay()` 之前，满足 mod-api 5.6 节和 C3。

Body 批次（邮箱通道 1）不带 Mod 段，两处都不进入。

进入条件：

| 本轮情况 | 动作 |
| --- | --- |
| `Pump` 成功发布了 World | 完整执行 7.7.2—7.7.4 |
| 没有新 World | 只处理已安装表里带"未解析引用"或"待实体"标记的记录 |
| World 发布失败（撤显） | 跳过，已安装表不动 |

##### 7.7.2 安装集

对快照中的每条记录，与已安装表比较（M2）：

1. 字节引用相同：跳过。这是最常见的路径，不碰 Lua，只做 C++ 遍历。
2. 引用不同但 memcmp 相等：只更新已安装引用，不写 Lua。
3. 内容不同、带未解析引用或待实体标记、对应实体在本批重建（7.7.4）：放进安装集。
4. 已安装表里有、快照里没有：global、player 作用域写顶层空值（有 `set` 时调用 `set(nil)`，否则按默认写回写 `nil`）；entity 作用域只删条目，因为实体已经离开。参与者离开时，它的 player 作用域记录同样只删条目，不调用 `set`（M5）。

**完整基线**（M8，已定）：本批 World 分段的 `base_version` 为 0 时，跳过第 1、2 步，快照里全部记录都进安装集重写一遍，与原生完整基线时全量写字段（`state/client_plan.cpp` 中平铺写计划在 `base_version == 0` 时为 full）的语义一致，让完整基线同样起修复作用：纠正客机 Mod 代码改动过的已声明键（D6）。其中内容与已安装值相同的记录不派发 `CLIENT_STATE_CHANGED`。原生的"每 32 份 World 轮转全写"不照搬，因为在 30Hz 下约每秒一次，会让 Lua 开销变成持续的。

客机 Mod 代码改动已声明键的典型来源：在绘制回调里推进计时器、消费一次性标志（`data.justHit = false`）、推进动画或菜单阶段，以及没用 `IsClient()` 隔开的玩法代码。这些改动只影响本机显示，不会上传。D6 约定客机本地要变的值放进未声明的键。

##### 7.7.3 顺序与回调

1. 实体生灭差集（7.7.4）。计划内的 `CLIENT_ENTITY_REMOVED` 已在预通知里派发（M3）。
2. 安装集按 global → player → entity 写入，每组内按记录键排序。
3. 本批新建或重建实体派发 `CLIENT_ENTITY_INSTALLED(ent)`，放在该实体的 entity 记录写完之后，回调里能读到已同步的数据。
4. 本批全部记录写完后，统一派发 `CLIENT_STATE_CHANGED(key, target)`，只为内容确实变化的记录派发（M4、M8）。

每个回调单独 `pcall`，抛错只计数，不影响后续回调和记录。

##### 7.7.4 实体生灭

原生安装过程中（池槽、引用、Init 隔离都处在中间状态时）不调用 Lua。生灭由本次发布前后两份已提交成员集合（`ClientSceneMemberStore`）的差集得出：

| 情况 | 判定 | 动作 |
| --- | --- | --- |
| 新建 | 成员号是新的 | 该实体的 entity 记录全部进安装集，写完后派发 INSTALLED |
| 重建 | 成员号相同，实例号变了 | 同新建；影子还在，不需要 Host 重发 |
| 原地 Morph | 地址和实例都没变 | 当作存活。`GetData` 跟着对象走，不重新安装 |
| 离开 | 成员号消失 | 删除已安装条目；REMOVED 已在预通知中派发（下文） |

**REMOVED 预通知**（M3，已定）：

- **名单**：后台准备本批时，对照 `ClientStatePreparedTarget` 的 `previous`（上次真正成功安装的根）与本批目标，列出计划内会消失的实例：成员离开，以及身份不一致、又不支持原生 Morph 而要删掉重建的成员。名单只含成员号和实例号，随批次封存。
- **派发**：主线程在预通知点按名单逐个找到仍存活的本地实体（核对实例号），派发 `CLIENT_ENTITY_REMOVED(ent)`，每个单独 `pcall`。
- **不重复**：主线程记录已经通知过的（成员号, 实例号），同一实例只通知一次。
- **配对**：安装阶段的差集中，已通知过 REMOVED 的实例如果仍然存活（本批发布失败，或主线程身份核对后改走原地 Morph），在下一次成功发布时补发 `CLIENT_ENTITY_INSTALLED`，并从已通知表删除，保证 INSTALLED 与 REMOVED 成对。
- **计划外释放**：不在名单里、却被原生安装释放的实例（构造、Init、字段写入失败后的拆除，或主线程核对出名单没预测到的重建），不派发 REMOVED，只按 Mod 计数 `mod_removed_unplanned`。重建后的新实例照常派发 INSTALLED。
- **场景退休**不走预通知，也不逐个派发 REMOVED（第 7.7.5 节）。

##### 7.7.5 场景与参与者生命周期

| 时机 | 动作 |
| --- | --- |
| 场景开始 | 第一份 World 发布且 Mod 安装完成之后、第一次 `AdvanceDisplay` 之前，派发 `CLIENT_SCENE_STARTED` |
| 场景退休（换层准备、换局、离开会话、失败） | 在 `scene.Retire` 开始原生拆卸之前（Lua 和实体都还在）：派发 `CLIENT_SCENE_ENDED`，再按 Mod 顺序逐个调用 `reset`，最后清空已安装表。不逐个实体派发 REMOVED |
| 同局换房 | 不是场景边界，按普通批次处理（大量实体离开、新建） |
| 参与者离开或退休 | M5：该参与者的身体已经离开，不调用 `set`，也不写默认写回；只删它的 player 作用域已安装条目。身体作为实体离开时照常按第 7.7.4 节派发 `CLIENT_ENTITY_REMOVED`，Mod 在这个回调里自己清理与该身体相关的本地数据。参与者重新加入后，它的记录按新目标走正常安装 |
| `luamod` 热重载 | 会话已经结束（mod-api 4.2 节），走退休流程；VM 已更换时跳过 `reset` 并计数 |

`reset` 抛错只计数，继续下一个，不阻断退休。

##### 7.7.6 Lua 调用框架

- **VM 核对**：每轮先确认 `g_LuaEngine->_state` 就是声明冻结时的 VM，做法与 `game/client/scene/client_native_bridge.cpp` 相同。不一致时整轮跳过，报 `mod_install_vm_mismatch`；registry ref 不跨 VM 使用。
- **调度根**：整个阶段包在新的 `ClientDispatchScope` 根 `mod_state_install` 里。`set` 中碰到被拦截的玩法入口时照常 FaultSkip，按当前 Mod 计数（`client_isolation` 新增 `mod_install` 分类）。持久化和随机数的现有拦截照常生效。
- **单条隔离**：每条记录一次 `lua_pcall`，消息处理器生成 traceback；进入前 `lua_checkstack`，退出时 `lua_settop` 恢复栈。每条记录只记前 3 次错误日志，之后只计数。
- **实体对象**：成员号查本批已发布的成员表并核对实例号，用 REPENTOGON 的 `lua::luabridge::UserdataPtr::push` 压栈。player 作用域和 `PLAYER` 类型压 `EntityPlayer`。`GetData` 在 Lua 侧通过方法调用取得。

##### 7.7.7 诊断

按 Mod 统计：每轮安装耗时（最大值、滑动平均）、安装条数、`set` 抛错次数、默认写回出错次数、未解析引用次数、`mod_state_install` 根下的 FaultSkip 次数、后台结构校验丢弃次数、完整基线全量重写的条数和耗时。D6 的只读检查在诊断模式下每个显示批次前从已安装引用重写一遍，统计被改动的键并报告是哪个 Mod 的哪条声明。

##### 7.7.8 可能的失败方式

| 失败 | 处理 |
| --- | --- |
| 结构校验失败 | 整条丢弃，影子保留上一版（§7.1） |
| Mod 快照与原生快照不属于同一帧 | 两者封在同一个 PreparedTarget 里，不会出现（M1） |
| VM 更换 | 整轮跳过，不使用旧 ref |
| `set` 或默认写回抛错 | 跳过，已安装引用不更新；字节变化或完整基线到来时重装，不每帧重试 |
| 写回只写了一半 | 不回滚，下一次写入时整条重写（§7.5） |
| 引用的实体缺席 | 写 `nil`，打标记，每轮重试，直到目标出现或确认离开（§7.4） |
| entity 记录的目标实体本批安装失败 | 打"待实体"标记，实体建好后按重建处理 |
| `set` 里调用了玩法入口 | FaultSkip，按 Mod 计数（mod-api 6.3 节的完整拦截现阶段不做） |
| `set` 里生成或移除实体 | pcall 隔离；不拦截，记为已知风险（同 mod-api 6.3 节） |
| 基线重发了相同的值 | 全量重写（M8），不派发 CHANGED |
| 客机 Mod 代码改了已声明的键 | 等 Host 下一次变化或完整基线纠正；诊断模式下检测（D6） |
| 一轮内大量记录变化（基线、换房） | 不设上限，只计耗时（M6） |
| 已预通知 REMOVED 的实例本批仍存活 | 不重复通知；下一次成功发布时补发 INSTALLED 配对（M3） |
| 原生安装计划外释放了实体 | 不派发 REMOVED，计数 `mod_removed_unplanned`（M3） |
| INSTALLED、REMOVED、CHANGED、SCENE 回调抛错 | 单个回调隔离并计数，不影响后续 |
| 退休时 `reset` 抛错 | 计数，继续下一个 |
| Mod 在玩法回调里推进自己的 Sprite | 客机不触发玩法回调，Sprite 停住或播错；按 M7 由 Mod 改到绘制回调里推进，联机层不兜底 |

##### 7.7.9 实施时核对

1. REPENTOGON 分派回调时压实体所用的元表（基类还是子类），`UserdataPtr::push` 要与之一致。
2. `GetData`、`GetPtrHash`、`Vector` 构造等绑定在 `mod_state_install` 根下是否会被现有 Hook 拦掉。
3. `ClientSceneMemberStore` 在发布后能否稳定给出"成员号 → 对象和实例号"，以及前后两次发布之间的差集。
4. 后台对"重建还是 Morph"的预测（M3 名单）是否总与主线程第 4 步的身份核对一致；预测重建而实际 Morph 时按"配对"补发 INSTALLED，预测 Morph 而实际重建时按计划外释放处理。

##### 7.7.10 代码落点（预计）

| 位置 | 内容 |
| --- | --- |
| `state/client_mod_shadow.*` | 结构校验、影子、快照封存（后台） |
| `game/client/mod/client_mod_install.*` | 安装阶段、已安装表、生灭差集、回调派发（主线程） |
| `state/client_receive.*` | Mod 段解码接入；`ClientStatePreparedTarget` 增加 Mod 快照字段 |
| `state/client_preparation.*` | 后台列出 M3 计划内离开／重建名单，随批次封存 |
| `game/client/scene/client_binary_scene.cpp` | `Pump` 取到 World 批次后、原生删除前调用 REMOVED 预通知 |
| `game/client/session/client.cpp` | 在 `Pump` 与 `AdvanceDisplay` 之间调用安装阶段 |
| `game/client/isolation/` | 新增 `mod_install` 计数分类 |

### 8. 已定事项

| 问题 | 决定（2026-11-22，用户决定；M6、M7 未按原推荐） |
| --- | --- |
| 存在位放在哪 | 每个 `struct` 定长区开头一张位图，缺失字段的槽位填 0。mod-api 5.2 节已同步 |
| `map` 字符串键的长度上限 | 每个 `map` 可声明 `keymax`，默认 64 字节。mod-api 5.2 节已同步 |
| 客机解码与安装 | 按第 7 节：后台结构校验并更新影子，主线程从字节构建或原地写回 |
| M3 `CLIENT_ENTITY_REMOVED` 的时机 | 按推荐 A：原生安装前预通知计划内的离开和重建，计划外释放不通知、只计数；补充不重复通知和 INSTALLED 配对规则。见第 7.7.4 节 |
| M4 `CLIENT_STATE_CHANGED` 什么时候派发 | 本批全部记录写完后统一派发，只为内容变化的记录派发。见第 7.7.3 节 |
| M6 客机每轮 Lua 安装耗时设不设上限 | 永不设上限，只计耗时。见 §7.6 |
| M7 Mod 在玩法回调里推进自己用 `Sprite()` 创建的 Sprite | 只用 a：SDK 文档要求把 `Update`、`Play` 放进绘制回调，由已声明的状态驱动。联机层不提供辅助手段，不采用"把动画名和帧声明成 State"或新增 `sprite` 辅助声明。实体自带的 Sprite 由原生同步覆盖，不在此列。mod-api 7.1 节已同步 |
| M5 参与者离开后，它的 player 作用域记录怎么处理 | 按推荐（2026-11-23）：不调用 `set`，只删已安装条目；Mod 在该身体的 `CLIENT_ENTITY_REMOVED` 回调里自己清理。未采用调用 `set(nil, nil)`：签名别扭，且身体已经不在。见第 7.7.2、7.7.5 节 |
| M8 完整基线时 Mod 记录怎么装 | 全部重写一遍，与原生完整基线全量写字段一致；内容与已安装值相同的记录不派发 `CLIENT_STATE_CHANGED`；不照搬原生每 32 份 World 的轮转全写。见第 7.7.2 节 |

### 9. 客机待定事项

M1（Mod 影子随 World 候选封存为不可变快照）和 M2（按字节引用加 memcmp 判断是否安装）已写入第 7.2、7.7.2 节，作为方案内容；M3—M8 已定，见第 8 节。客机部分目前没有待定事项。

## 具名适配交接

- **StageAPI**：自定义房间类型、带原生载体及无原生载体的 CustomGrid、Lua 附加状态和生命周期。既有观测保留，正式采集、资源身份、Client owner、切房与退休应用仍需具体适配。
- **角色与武器**：动态身份映射、自定义状态、专用资源／绘制及生命周期；未知动态类型不得伪装成原版基类或已知武器。本轮原版角色／身体族所需项仍归原版同步。
- **Player／HUD 回调结果**：官方 1072／1073 等第三方回调的有效结果、healthtype／自定义 cache 的实际显示结果和 Mod HUD 展示；Client 不执行玩法 Lua，也不恢复整套 Host 模拟输入／通用查询字典。原生语义与 UNKNOWN 继续参考[装备与 Plus 复核](binary-state/player-equipment-plus-synchronization-review.md)、[Player 字节账本](binary-state/player-byte-ledger.md)和[第 5 步设计](binary-state/5-player.md)。
- **额外查询与 History**：声明的界面 Mod 如需要原版同步未发送的历史或查询结果，单独声明 schema 与用途，不据此恢复整个原生 History 或任意 gameplay getter。
- **随机相关结果**：适配只传具体最终状态／展示结果，不把 Lua RNG、闭包、CRT／MT 引擎或原生地址跨进程复制，不在 Client 重新抽取。

### REPENTOGON 接口对照：原生同步待补项（2026-11-23）

对照 REPENTOGON wiki 的写接口和回调，以及 REPENTOGON 源码里的存储位置（`F:\projects_isaac\REPENTOGON-1.1.3\repentogon`）。只做了静态对照，没有实现、构建或测试。下面各项写入的是 REPENTOGON 自己的内存，或者只在 Host 本地生效，现有原生同步看不到。Mod API 一侧的规则见 [Mod 联机接口方案](../../mod-api.md) 6.2 节、6.4 节。

**需要补同步**

| 项 | 接口 | 存储位置 | 客机现状 | 归属与做法 |
| --- | --- | --- | --- | --- |
| 自定义 cache | `EntityPlayer:GetCustomCacheValue`、`ItemConfig_Item:AddCustomCacheTag`、`MC_EVALUATE_CUSTOM_CACHE` | `EntityPlayerPlus::customCacheResults`、`customCacheTags`（`Patches/EntityPlus.h:19`，读取见 `Patches/CustomCache.cpp:69`） | Mod HUD 读到默认值，包括 `tearscap`、`statmultiplier` | Player 附加结构补字段，按[第 5 步](binary-state/5-player.md)的方式逐项建模。[装备与 Plus 复核](binary-state/player-equipment-plus-synchronization-review.md)已写明这部分未进入二进制唯一源 |
| 强制隐身外观 | `EntityPlayer:SetForceCamoEffect` | `EntityPlayerPlus::camoOverride`（`LuaInterfaces/Entities/LuaEntityPlayer.cpp:3313`，读取见 `Patches/EntityPlus.cpp:225`） | 隐身外观不显示 | 同上，Player 附加结构补一个字段 |
| 玩家属性查询的有效结果 | `MC_PLAYER_GET_HEALTH_TYPE`、`MC_PLAYER_GET_HEART_LIMIT`、`MC_PLAYER_GET_ACTIVE_MAX_CHARGE`、`MC_PLAYER_GET_ACTIVE_MIN_USABLE_CHARGE` | 回调结果在查询时现算；生命类型另有 `EntityPlayerPlus::previousHealthType` 等状态 | 客机不触发这些回调（mod-api 6.2 节），HUD 按原版值显示 | 同步 Host 上的有效结果，客机查询时直接返回。`catalog/player-application.json` 已把 `GetHealthType`、`GetHeartLimit` 标为"最终显示缓存"；`GetActiveMaxCharge` 现标为 `@local_api_context`，要改。与上面的"Player／HUD 回调结果"一项合并实施 |
| 楼层显示名 | `Level:SetName` | REPENTOGON 全局变量 `CustomStageName`（`LuaInterfaces/LuaLevel.cpp:45`） | HUD、暂停菜单、楼层横幅显示原版名字 | 作为 Level 的注入值，同步 Host 当前生效的楼层名 |
| 当前楼层的名字、音乐、背景 | `Isaac.SetCurrentFloorName`、`SetCurrentFloorMusic`、`SetCurrentFloorBackdrop` | 运行时改写配置表 `RoomConfig._stages[]` 的 `_displayName`、`_musicId`、`_backdrop`，离开楼层时还原（`Patches/StagesStuff.cpp:39-75`） | 显示名丢失。音乐和当前房间的背景本身已同步；之后进入的房间，客机按本机配置表取背景，可能不对 | 显示名与上一项合并为"当前楼层显示名"；后续房间的背景实施时核对是否已由房间背景同步覆盖 |
| 进度查询（mod-api D12） | `PersistentGameData:Unlocked`、`IsChallengeCompleted`、`GetEventCounter`、`GetBestiary*Count`、`IsBossKilled`、`IsItemInCollection` | 原生 PGD（成就 642 字节、事件计数 523 个等） | 返回客机本机存档的结果 | 以 Host 当前游戏所用的存档为准。沿用[显示委托](binary-state/client-display-delegation.md)"进度（成就、计数）"一行的做法：同步 Host 的进度数据，客机经查询 hook 返回同步值，不写客机 PGD、不改本机存档。原设计按显示需求裁剪，Mod 的查询对象事先不可知，这里需要覆盖上述查询的全部范围。写入类接口（`Unlock`、`TryUnlock`、`IncreaseEventCounter`、`AddBestiaryKill`、`AddBossKilled`）在客机上按持久化拦截，实施时核对现有拦截（`client_native_bridge.cpp`）是否已覆盖 |
| 过场动画 | `Isaac.PlayCutscene` | 直接播放（`LuaInterfaces/LuaIsaac.cpp:369`） | Host 上没有采集点，客机看不到 | 原生同步新增过场事件，在调用点记录，客机按事件播放。`ClearGameState = true` 时会结束本局，要与场景退休一起处理 |

**需要核对**

| 项 | 静态核对与处置 |
| --- | --- |
| `HUD:SetBossHPBarFill` | J273 HUD::Update（97ACF0）和 HUD::PostUpdate（97AE90）都不写 54B0，Boss 条绘制 97F7A0 直接读取它。Host 当前填充值随 World 安装，负值保持隐藏语义。 |
| `Isaac.CreateTimer` | J273 6047F0 生成 Type 1000、Variant 119，对象保存 Host 回调入口，REPENTOGON 的 Lua 函数也只在 Host registry 中。正式实体目录现排除该变体，计时由 Host 推进。 |
| `FXLayers:AddPoopFx` | REPENTOGON 调用原生 96E570。现有 DisplayInjectionState 采集 FX 层的 countdown（+34）和 ColorMod（+84），客机在首个活动值用原生 AddPoopFx 准备本地资源，再安装倒计时与颜色。已经覆盖这条 Mod 入口。 |
| Mod 生成的程序道具 | Lua_PIMCreateProceduralItem 调用同一个原生 ProceduralItemManager::CreateProceduralItem（J273 9B1E50）。现有 Game+669BC 的定义采集按 ~id 接受负编号，客机复用同一资源注册和查询。 |
| Host 音频采集点 | libzhl 按优先级从高到低安装 detour，后装的是外层。SFXManager::Play 和 SoundEffects::Play 同址 907C60，C/S 10000 的采集已位于官方 pre 回调内层。音乐 Play、Crossfade、Jingle、Layer 原来在外层，现移到 10000，接收官方 super 实参。回调取消时不进入采集。 |

**已覆盖，不用新增**（目录或代码里有对应项，不代表每个字段都实测过）：Entity 状态倒计时、`SetColorParams`、`SetShadowSize`、`SetSpeedMultiplier`；EntityPlayer 的大部分字段 setter、Weapon、ItemSpoof；Room 的水、熔岩、雨、闪电、背景；Game 的 `SetDizzyAmount`、`SetBloom`、`SetColorModifier`；Sprite／LayerState 的图层、帧、渲染标志、裁切；自定义 shader 和冠军 shader；音乐音高；`ItemOverlay:Show`、`RoomTransition:StartBossIntro`。

**本地界面，不同步**：ImGui、`Options`、窗口标题和图标、`DebugRenderer`、`Minimap:SetState`、`GenericPrompt`。Mod 在 Host 上弹出的提示框，客机不会出现；按 mod-api 7.1 节，这类界面由 Mod 在客机本地自己绘制。

## 实施顺序（2026-11-23）

日期：2026-11-23。状态：第 1—13 步已实现、待第 14 步 E2E。本节取代 mod-api 第 11 节原顺序，汇集公开 API、声明类型的采集与编码、REPENTOGON 接口对照及验收夹具。

按项目规范执行：不设 gate、不校验哈希；每步只做纯编译（Win32 Release 产品 DLL 与 E2E 构建），中途不运行任何测试，运行验证全部归第 14 步统一 E2E；代码为组件、核心函数和数据结构写中文注释。每步开头列出的"先核对"是该步要依赖的静态事实（源码或 IDA），查清后写回对应小节再写代码，不是推进条件；查出与设计不符时先改设计。

### 总览

| 步 | 内容 | 依据 | 依赖 |
| --- | --- | --- | --- |
| 1 | `IsaacCS` 骨架、声明登记与编译、SDK | mod-api 3、4.2；本文第 1、2 节 | — |
| 2 | 握手：Mod 清单、ID 表、拒绝连接 | mod-api 4.1、4.2 | 1 |
| 3 | 客机回调分级放行、音频回放标记 | mod-api 6.2 | 1 |
| 4 | Host 读取与 Mod 段（global、player） | mod-api 5.1—5.5；本文第 3、4 节 | 1、2 |
| 5 | 客机安装（global、player） | mod-api 5.6；本文第 7.1—7.3、7.5—7.7 节 | 3、4 |
| 6 | entity 作用域与实体生灭回调 | mod-api 5.3、5.6；本文第 7.4、7.7.4 节 | 5 |
| 7 | 意图与结果事件 | mod-api 7.2、D8 | 1、2、5 |
| 8 | 扩展按键与 Lua 输入路由 | mod-api 6.1、D7 | 1、2 |
| 9 | 诊断模式 | mod-api 5.3、5.4、5.7；本文第 7.7.7 节 | 4—6 |
| 10 | 原生待补：核对表五项 | "REPENTOGON 接口对照"需要核对 | — |
| 11 | 原生待补：Player 附加字段与查询结果 | 同上"需要补同步"前三行 | 10 |
| 12 | 原生待补：楼层显示名、过场事件、进度查询 | 同上后三行；mod-api D12 | 10 |
| 13 | 测试夹具 Mod、文档收尾 | mod-api 10 | 1—12 |
| 14 | 统一 E2E，连续修复与续测 | mod-api 10；本节末 | 13 |

第 1—9 步是 Mod API 主线，第 10—12 步是原生同步支线。两条线互不依赖，支线可以穿插在主线任意两步之间做；唯一的交叉是音频：第 3 步的回放标记和第 10 步的 Host 采集点核对要都完成，夹具的音频场景才有意义。

与 mod-api 原顺序相比有两处调整：骨架提到握手前面，因为清单里的"声明描述"要靠声明登记生成；回调放行从握手一步拆出来单列，放在客机安装之前，避免客机安装接通后玩法回调仍全部放行。

### 第 1 步：`IsaacCS` 骨架、声明登记与编译、SDK

先核对：DLL 能否在任何 Mod 的 `main.lua` 执行之前把真实 `IsaacCS` 注入全局表（mod-api 3.1 节"注入时机"）。做不到时，实现"真实实现加载后接管不联机实现已登记的声明"。

静态核对确认 `LuaEngine::Init` 先运行 REPENTOGON 的 `main_ex.lua`，再缓存回调 registry 入口。沿用现有 bundled-script 包装，在脚本返回时注入真实 `IsaacCS`。加载结束在 `MC_POST_MODS_LOADED` 的官方分派之前记录。声明时计算尺寸，主线程创建握手清单时生成前序节点和 Lua registry 引用。Mod 卸载或会话内尝试 `luamod` 时在下次 Manager 边界发起退休。

- 全局表与登记：`Declare`、`State`、`EntityData`、`EntityState`、`Intent`、`Keys` 的参数检查；加载期之外调用报错并记日志（4.2 节）；声明按 Mod 序号、声明序号编号。
- 编译：类型树按前序排成节点数组，算出 `fixed`、`worst`，`worst` 超过 64 KiB 在声明时报错；驻留键、`set`、构造器（`Vector`、`Color`、`KColor`）存 registry ref（本文第 1、2 节）。节点数组在握手冻结后不可变。
- 规范化声明文本：名称、作用域、类型、上限、`private`、`rate`，按声明顺序生成，供第 2 步逐字比较。
- 查询接口：`IsClient`、`GetParticipant`、`GetPlayerByParticipant`、`GetLocalPlayer`、`IsLocalPlayer`；`MarkDirty`、`AddCallback` 先只登记（派发在第 5—7 步接上）。
- `isaaccs_sdk.lua`：选择实现、多份共存、`api` 版本只增不减、版本不足时补不联机函数并把该 Mod 记为"api 版本不受支持"；不联机行为严格按 3.2 节表格（`SendIntent` 立即执行 handler、结果延到下一帧回调）。每个函数带中文注释和 LuaCATS 注解。
- 会话中检测 `luamod` 热重载：结束会话（第 7.7.5 节的退休流程在第 5 步接上）。

### 第 2 步：握手

源码中 `ModManifest` 使用 27 号控制消息，在 Hello／Welcome 之后以现有可靠缺片机制交换。启用清单来自游戏 `ModManager`，`GetModRefByTable` 将声明与实际 `main.lua` 目录关联，REPENTOGON `XMLData` 按来源提供对应类别的 ID。两端比较通过后才继续 Ready。清单净内容上限为 60 KiB。

- 新消息 `ModManifest`，编号登记到 `wire.json`；客机 Hello 之后经可靠通道交换。
- 清单内容：启用的 Mod（目录名、`metadata.xml` 的 name 和 version、是否有 `main.lua`）；已声明 Mod 的 `api` 版本、自报版本和第 1 步生成的规范化声明文本；ID 表（道具、饰品、卡牌、药丸效果、实体、角色、挑战、音乐、音效、Costume）。
- 比较：集合与顺序无关、版本字符串相等、声明文本逐字相等、ID 表逐项相等，全部按内容比较，不用哈希。不等时拒绝连接，大厅显示首个差异。
- 拒绝情形：未声明的 Lua Mod（D1，大厅显示 Mod 名）；SDK 比联机层新（提示更新联机层）；ID 不一致不做重映射（D2）。本项目自带的界面 Mod、观测 Mod 和 REPENTOGON 不参与判定（3 节）。
- 握手通过即冻结全部声明。

### 第 3 步：客机回调分级放行

先核对：C++ 直接分派的回调是否都经过 `_RunCallback` 系列入口；过滤做在注册时包装还是分派时检查（mod-api 6.2 节开头）。音频回放标记怎样传到过滤器，与这两项一起定。

已核对 REPENTOGON 1.1.3 的 `LuaInit.cpp`、`CustomCallbacks.cpp` 和 `main_ex.lua`。C++ 通常使用两个缓存入口，多重射击兼容分支缓存了独立闭包，随后经 `Isaac.RunCallbackWithParam` 进入同一逐条调用点。过滤接在 `RunCallbackInternal`。ImGui 直接调用界面闭包。客机音频回放用主线程作用域标记，单次 SFX 保持现有原生音槽播放路径，音乐状态和循环声在同一规则下应用。Host 最终音频参数的采集点仍归第 10 步核对。

补齐静态审阅发现的漏拦路径。两层分派按固定回调 ID 和原生阶段应用同一套规则，项目内置 Mod 同样受限。J273 `LuaCallbackCaller::CallInputAction`（`85AEC0`）复用官方结构返回 ABI，为稳定主菜单的原生输入调用授予资格。HUD 更新、历史栏重算和本地暂停菜单绘制在各自的显示入口放行。音乐 tick 在 `7D8303` 消费 Host 队列时继承回放标记，现有 `Music::Queue` Hook 记录本地重入队，包括覆盖同一个 ID 的情况。

纯编译中，布局生成器拒绝新增的顶层报文字段，改用既有 `protocol_constants` 登记。MSVC 拒绝以单个字符串常量嵌入 SDK，改由同一源文件生成字节数组。修正后产品 DLL、E2E DLL 与工具均编译成功。

- 按 `callback.Mod` 加回调种类过滤已声明 Mod 的回调，按 6.2 节分类表：绘制、HUD、菜单、`MC_GET_SHADER_PARAMS`、`MC_PRE_GET_LIGHTING_ALPHA` 放行；Init、玩法、Cache、玩家属性查询不触发。
- 音频：客机回放 Host 的音效、音乐事件期间设"回放中"标记，过滤器跳过五个音频回调；客机本地发出的声音照常触发。
- 文档：[client.md](../../client.md) §8"Lua 回调一律放行"改为按 mod-api 6.2 节分级放行。

### 第 4 步：Host 读取与 Mod 段（global、player）

2026-11-23 接入源码。Host 把采集结果交给普通 World 邮箱，后台按同帧成员目录填写实体引用并排序 map。布局升至 92。记录以接收 ACK 对应的实际发送值作差，私有记录在组帧时按参与者过滤。每拍优先保留原生正文和已有表现事件，剩余额度发送 Mod 记录。常规 World 无法承载大记录时复用可靠基线。

- 读取（本文第 3 节）：World 采集窗口内按 `rate` 执行读取程序（`"world"`、`"low"`、`"mark"`）；类型不符、超限、`get` 抛错时该条本帧沿用并计数；Mod 段整体失败（VM 不一致、暂存区取不到）时全部沿用、原生帧照常发布。不设时间上限，耗时按 Mod、按声明计入诊断。
- 值类型全部接通，含 `ENTITY`（成员号，不在目录或已离开写 Null 并计数）和 `PLAYER`（ordinal）。
- 编码线程（本文第 4 节）：按索引给 `map` 排序得到规范字节，与上一版 memcmp，更新"最后变化版本"。
- 线格式（mod-api 5.5 节）：World 帧新增 Mod 段和记录头；布局版本加 1；按 Peer 组帧时跳过接收者不是该 Peer 的私有记录；计入 World 预算、单条 64 KiB 上限和三池内存账本。
- 完整基线带上全部读成功过的记录；从未读成功的记录不出现。

### 第 5 步：客机安装（global、player）

先核对：第 7.7.9 节第 2 项（`GetData`、`GetPtrHash`、`Vector` 构造等绑定在 `mod_state_install` 根下是否会被现有 Hook 拦掉）。

2026-11-23 静态核对现有 Hook 清单及 REPENTOGON 1.1.3 Lua 绑定。`GetData`、`GetPtrHash` 和 `Vector`、`Color`、`KColor` 构造没有经由客机拦截的根入口。安装使用冻结的构造器引用及 REPENTOGON userdata 注册。客机后台把验证后的 Mod 字节绑定在同一份 World 的应用票据上，Manager 在显示前写 Lua 并派发本批通知。统计沿用联机诊断页。场景退休先执行 ENDED 和声明的 `reset`，同层换房沿用当前 Mod 场景。Host 提前撤下来源且下一场景尚未知时，保留 Mod 场景至下一次 Prepare 核对 run 和 floor。Lua 逻辑放在 SDK 兼容的 `game/mods/mod_api.cpp`，Host 编码与客机影子校验共用 `state/mod_codec.cpp`。运行行为归第 14 步验证。

- 后台（第 7.1、7.2 节）：Mod 段解码接入 `state/client_receive.*`，结构校验失败整条丢弃；影子按记录键合并，封存为不可变 `ModShadowRef`，挂进 `ClientStatePreparedTarget`（M1）。
- 主线程（第 7.7.1—7.7.3 节）：`game/client/session/client.cpp` 中 `Pump` 之后、`AdvanceDisplay` 之前进入安装阶段；已安装表、按引用加 memcmp 生成安装集（M2）；完整基线全部重写（M8）；有 `set` 时构建新值，无 `set` 时按默认写回原地写（第 7.3 节，D11）；顶层空值（D10）。
- 未解析引用（第 7.4 节）：写 `nil`、打标记、每轮重试。
- 回调：批末统一派发 `CLIENT_STATE_CHANGED`（M4）；`CLIENT_SCENE_STARTED`、`CLIENT_SCENE_ENDED` 与 `reset`（第 7.7.5 节），挂在 `scene.Retire` 原生拆卸之前；参与者离开只删 player 条目（M5）。
- Lua 调用框架（第 7.7.6 节）：VM 核对、`ClientDispatchScope` 根 `mod_state_install`、`client_isolation` 新增 `mod_install` 分类、每条 `lua_pcall` 加 traceback、栈恢复、每条只记前 3 次错误。
- 代码落点按第 7.7.10 节；其中"后台列出 M3 名单"的位置更正为 `game/client/scene/client_preparation_native.cpp`（`state/` 下没有 `client_preparation.*`），在第 6 步实现。

### 第 6 步：entity 作用域

先核对：第 7.7.9 节第 1、3、4 项（压实体所用的元表、`ClientSceneMemberStore` 的成员号→对象与实例号及差集、重建／Morph 预测与主线程身份核对是否一致）。

2026-11-23 接入源码。REPENTOGON 1.1.3 的通用实体回调参数使用 `ENTITY`，具名子类参数使用各自注册。模组接口的 `ent` 采用前者。Host 暂存用实体的物理身份与使用代次，后台再换为同帧成员号。客机通过已发布目录和 `ClientSceneMainPreparation::FindLiveMember` 取得当前实例。后台按上一份成功安装的目录预测离开和重建，主线程用 `SelectRetiring` 重新核对。同一实例的原地 Morph 保留状态。预测漏掉的实际释放计数，预通知后仍存活的实例在下一次成功发布时补发 INSTALLED。Host 在提前撤下采集时携带同层换房语义，保留旧实例释放前的通知时机。

- Host：`EntityData` 按 filter 读 `GetData()[key]`，`EntityState` 调 `get(ent)`；记录目标为成员号，与实体同生命周期（离开一起删、沿用一起沿用）。
- 客机后台：成员目录显示离开时，从下一份快照删除该成员的 entity 记录；准备批次时列出 M3 计划内离开／重建名单随批次封存。
- 客机主线程：`Pump` 取到 World 批次后、原生删除前派发 REMOVED 预通知；安装阶段按已提交成员集合差集处理新建、重建、原地 Morph、离开（第 7.7.4 节）；entity 记录写完后派发 `CLIENT_ENTITY_INSTALLED`；不重复通知、INSTALLED 配对、计划外释放计 `mod_removed_unplanned`；"待实体"标记；实体重建从影子重装，不需要 Host 重发。

### 第 7 步：意图与结果

2026-11-23 接入源码。布局 93 登记 `ModIntent`（28），并在 World 正文内将私有结果与原生事件分别定界。客机按队首重传，Host 以连接内序号去重，受理后进与本机共用的主线程队列。私有结果由实际 World 发送凭据上的 ACK 确认。客机在收包时将结果移交连接邮箱，Manager 在处理本轮应用后派发。换局、换层或退出时，客机为仍在等待的请求派发失败。实体参数沿用提交目录中的成员号，执行前重新核对目录代次和实体身份。

- 新消息 `ModIntent`（可靠、有序），登记到 `wire.json`；内容 ≤ 512 B、每 Peer 在途 ≤ 8、每秒 ≤ 20，超出时 `SendIntent` 返回 nil。
- Host 排队，在下一次最外层 `Manager::Update` 入口按到达顺序执行，与 `cs` 命令相同；发起者没有身体时拒绝；Host 本机发起的意图走同一个队列。
- 结果（D8）：ok 加 ≤ 256 B 结果，作为发起者的私有事件进 World 事件队列，按 ACK 去重，客机派发 `CLIENT_INTENT_RESULT`；Host 本机同样回调（mod-api 3.2 节补充）。
- 换局、换层、会话结束时丢弃未执行的意图并返回失败结果。

### 第 8 步：扩展按键与 Lua 输入路由

InputManager 的动作查询最终调用设备槽。C/S 在这些槽里的覆盖要求 Player consumer 作用域，普通 Lua 查询缺少这个作用域。公开 Input.IsActionPressed、IsActionTriggered 和 GetActionValue 现按 ControllerIndex 定位 C/S 参与者并读取同一 Authority 快照。两端按清单对扩展键升序去重，最多 64 个，按住与边沿各用一个 u64。边沿随输入序号去重、按目标 tick 消费，重绑和暂停丢弃待提交边沿。输入记录和紧凑控制正文升至 schema 3。

先核对：远端玩家的 `ControllerIndex`（虚拟设备 K）上，Lua `Input.IsActionPressed`、`IsActionTriggered`、`GetActionValue` 是否经过原生输入消费者的同一条路径（mod-api 6.1 节）。不经过时补路由，使其返回该参与者的统一输入。

- `Keys` 声明进清单（第 2 步已含声明文本）；客机采样声明的按键，随输入记录上传（输入记录格式变化按现有规则登记版本）。
- Host 上 `IsKeyPressed`、`IsKeyTriggered` 按参与者查询。

### 第 9 步：诊断模式

现有诊断开关开启后，首次成功采集枚举未声明的结构键，大型 world 容器在编译声明时提示。联机诊断按模组和声明显示安装、基线重写、回调、拦截和解码统计。诊断模式每批从已安装字节重写当前实例，用重写前后的有类型读值检测改写并点名。带自定义 set 的声明需要 get 或默认键能观察到实际目标。

- 读取：每个声明首次读取时把被忽略的字段名记进日志；`max` 超过 256 的容器按 `"world"` 读取时加载期提示（mod-api 5.3、5.4 节）。
- 安装：按 Mod 统计第 7.7.7 节列出的各项。
- D6 只读检查：每个显示批次前从已安装引用重写一遍，按 Mod、按声明统计并点名被改动的键（mod-api 5.7 节）。

### 第 10 步：原生待补——核对表五项

逐项查清"REPENTOGON 接口对照"中"需要核对"表，结论写回该表；需要补同步的项并入第 11、12 步：

1. `HUD:SetBossHPBarFill` 的覆盖值会不会被客机 `HUD::Update` 冲掉。
2. `Isaac.CreateTimer` 生成的 Effect 是否复制到客机、复制后是否无害。
3. `FXLayers:AddPoopFx` 的 FX 层内容是否在同步范围内。
4. Mod 经 REPENTOGON 接口生成的程序道具是否走 TMTRAINER 负 ID 的同一路径。
5. Host 音频采集点在 `MC_PRE_SFX_PLAY`、`MC_PRE_MUSIC_PLAY` hook 之内还是之外；在之外时改到回调处理之后记录最终参数，被取消的声音不记录。

### 第 11 步：原生待补——Player 附加字段与查询结果

布局 94 通过 World Mod 段的原生保留声明表示每个 Player 实例的 Plus 附加状态。自定义缓存键、值与标签按固定顺序编码，客机写入经 SDK ABI 核对的容器和 previousHealthType、camoOverride。健康与充能结果由实际 Host 原生调用正常返回时采集，客机按身体与调用参数读取。未在 Host 执行过的参数组合仍走原生默认查询。采集不会主动执行可能转换心格或触发玩法 Lua 的 getter。原生保留记录使用 Mod 序号 65534。外层 u16 长度和 Mod 记录头由同一接收管线验证，Player 目标为同帧实体成员号。

| u8 camo | i32 previous_health | u16 标签数及逐项 u16 长度＋字节 | u16 缓存项数及逐项字符串＋f64 | u16 查询项数及逐项 u8 类别＋两个 i32 参数＋i32 结果 |
| --- | --- | --- | --- | --- |

按[第 5 步](binary-state/5-player.md)的方式逐项建模，进 Player 附加结构，布局版本加 1：

- 自定义 cache：`EntityPlayerPlus::customCacheResults`、`customCacheTags`。
- 强制隐身外观：`EntityPlayerPlus::camoOverride`。
- 玩家属性查询的有效结果：`GetHealthType`、`GetHeartLimit`、`GetActiveMaxCharge`、`GetActiveMinUsableCharge` 在 Host 上的结果（含 `previousHealthType` 等状态），客机查询 hook 直接返回同步值。`catalog/player-application.json` 中 `GetActiveMaxCharge` 从 `@local_api_context` 改为最终显示缓存，`GetActiveMinUsableCharge` 同样处理。与"具名适配交接"中"Player／HUD 回调结果"一项合并实施。

### 第 12 步：原生待补——楼层显示名、过场事件、进度查询

楼层名称从正常 Level::GetName 取得最终文字，客机在 J273 73FE80 返回同楼层结果。Room 的 backdrop_id 已安装到原生 Backdrop，客机冷构建从该字段选择背景，后续进房沿用此路径。过场请求使用 World 事件 6，清场导致旧路由提前退休时可靠 Retire 补带同一序号。客机确认原生场景退休后调用过场专用显示入口。REPENTOGON 对 ID 大于 26 的自定义过场走控制台入口，其清场选项沿用官方行为。

PGD 固定范围、已分配的自定义成就／挑战、图鉴三类计数走 Host 只读快照。客机公开查询和原生 PersistentGameData::Unlocked 从镜像读。五个公开写入方法已由 client_native_bridge.cpp 的 Lua 持久保护包住，底层 TryUnlock、IncreaseEventCounter、AddBestiaryKill 和 AddBoss 也已有拦截。原展示用 ClientDisplayProgressBlocked 仍服务 HUD 资格。PGD 的原生解锁查询入口是 9039B0。

- 当前楼层显示名：`Level:SetName`（`CustomStageName`）与 `Isaac.SetCurrentFloorName`（`_stages[]._displayName`）合并为一个 Level 注入值，同步 Host 当前生效的名字。同时核对 `SetCurrentFloorBackdrop` 之后进入的房间，背景是否已由房间背景同步覆盖，没覆盖就补。
- 过场事件：在 `Isaac.PlayCutscene` 调用点记录，随 World 事件下发，客机按事件播放；`ClearGameState = true` 与场景退休一起处理。
- 进度查询（D12）：把显示委托"进度（成就、计数）"一行从"按显示需求裁剪"扩到 mod-api 6.4 节所列查询的全部范围（成就、事件计数、挑战完成、图鉴、Boss 击杀、收藏）；客机查询 hook 返回同步值，不写客机 PGD、不改本机存档。核对 `client_native_bridge.cpp` 的持久化拦截是否已覆盖 `Unlock`、`TryUnlock`、`IncreaseEventCounter`、`AddBestiaryKill`、`AddBossKilled`，没覆盖就补。显示委托文档中的查询 hook 名 `client_hud_achievement_query` 在现有代码中查不到，实施时先确认实际入口。

### 第 13 步：测试夹具 Mod、文档收尾

- [专用夹具](../../../tests/e2e-mods/isaac-cs-mod-fixture/README.md)使用真实声明和同源单机 SDK，涵盖全部作用域、类型、采集频率、输入、意图及限流、握手负向样本、状态错误与生命周期。Host 命令和本端按键会留下可对照的 Lua 观察。原生用例覆盖进度 HUD、自定义 cache、隐身、充能和心上限、Boss 血条、两条楼层改名路径、下房背景、音频和过场。E2E 构建装配独立目录，`package.ps1 -ModAdaptation` 选择专用包。
- [同步](../../synchronization.md)、[Client 端设计](../../client.md)、[架构](../../architecture.md)已更新至布局 94。先核对事项的结论分别记录在第 1 步（Lua 注入）、第 3 步（分派与音频标记）、第 5—6 步（Lua 绑定、成员与实例）、第 8 步（ControllerIndex 输入）及第 10—12 步（原生接口及持久化）。夹具对照条件见其 README。

### 第 14 步：统一 E2E

按 mod-api 第 10 节的场景，加原 E31 的覆盖要求（见下节）和第 13 步补的原生待补用例，统一跑 E2E，连续修复与续测。每次实机测试前先告知需要配合的操作和何时停手，需要手动配合时确认用户准备好再启动。

**2026-11-23 首轮实机。** 两端到达标题页并记录夹具以 `native_sdk=true` 加载。Host 选择隔离存档二时，在 `mod_floor_name` 首次返回后退出，尚未开局。J273 的 `73FE80` 有五个原生调用点，每处调用方回收 20 字节栈参数。原自有成员定义使用被调用方清栈，导致栈指针错位。改为复用 REPENTOGON 的全局 `GetLevelName` 定义，保留 ECX 输出对象、EDX 层号和调用方清栈。

**同轮续测。** 两端分别在安装联机层、会话关闭时进入存档二的单机局并读到夹具 HUD。Host 成功意图在帧 3562 将计分从 0 改为 3，结果在帧 3563 到达；失败意图在帧 3913 执行，帧 3914 收到失败，计分仍为 3。Host 的收藏、Boss 击杀及图鉴计数与 Client1 的本机进度有差异。随后 Host 开始联机时返回 `mod_api_unavailable`。启动跟踪显示正式 Lua 虚拟机中的全局 `debug` 为 nil，分派安装未完成。已改为由原生只向安装闭包传入调试库表；同轮继续握手验收。

**2026-11-23 世界安装修复。** 用户手测时客机能接收并解码，但两次连接累计记录 1,470 次 `binary_live_copy_window` 安装撤销，画面反复回到等待。固定种子双端复测定位到 `RoomDescriptor.grid_index` 的原生写入前检查。同一 DLL 里，增量 LTCG 复用的内联守卫从票据旧偏移 `0xC9` 读取消位，当前守卫使用 `0xD9`。Release DLL 现显式使用完整 `/LTCG`，重新生成跨单元内联代码。写入前检查失败时，安装器保留此前真实写入的状态。修复后同样的双端流程首次安装通过，40.97 秒内发布 1,228 次 World 更新（29.97 Hz），40 次采样均为已安装且未过期，跟踪未见相关撤销或重同步。两端正常停止，客机原生场景及二进制队列均已排空。本次复测覆盖世界安装问题。

**不在本顺序内**：客机副作用防护（mod-api 6.3 节，现阶段不做）；StageAPI、自定义角色与武器等具名适配，以及挑选示范 Mod（D9），在这套 API 跑通之后作为兼容 Mod 或单独设计推进。

## 后续验收与未决问题

原 E31 的覆盖要求保留在这里：具名 adapter 的编码／解码须覆盖循环、共享、超预算、抛错、超时、引用失效和生命周期变化；超限不得伪装为空或完整，安装失败不得提前确认。现行强类型夹具用有限声明路径覆盖共享与自引用表，用诊断耗时观察读取成本；运行时不设读取超时。StageAPI、角色、资源与绘制还须按自身范围覆盖真实 Client 行为。

已定（2026-11-23）：提供公开声明式接口，见 [Mod 联机接口方案](../../mod-api.md)。

已定（2026-11-23）：各 Mod 的 schema／版本与支持清单由 mod-api 取代：schema 就是 Mod 自己的声明，版本在握手时逐字比较（4.1 节），支持清单按 D9 不维护。稳定编码见"声明类型的采集与编码"第 2、4 节；读取耗时与失败发布语义按原生采集定稿：每条记录只按数量设上限，不设时间上限、不降频，失败记录沿用，帧照常发布，ACK 只表示收到（mod-api 5.4 节）。

本文没有剩余的设计未决项。实施时要核对的事实见第 7.7.9 节和"REPENTOGON 接口对照"的"需要核对"表，已分别排进"实施顺序"各步的"先核对"；接口层的未决项见 mod-api 第 12 节。
