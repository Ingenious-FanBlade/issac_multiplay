# 第三方 Mod 适配：后续方案

日期：2026-11-10（Asia/Singapore）。状态：从[原生二进制重构](binary-state/README.md)迁出的设计，后续另行推进；不是本轮 0—15 步的实现或验收条件。本次只迁移方案，没有实现、构建或测试。

2026-11-22 补记：公开接口方案草案见 [Mod 联机接口方案](../../mod-api.md)，路线为 Mod 作者声明、联机层提供基础设施、协商不一致就拒绝连接，待用户审阅。本文"仍未确定"中"仅保留具名 adapter 还是提供公开注册接口"一项以该草案为准；其余内容不变。同日起"Mod 值编码草稿"不采用（D4），声明类型的采集与编码见新增的同名一节。

2026-11-23 补记：对照 REPENTOGON 接口，原生同步待补项和待核对项记在"具名适配交接"下的"REPENTOGON 接口对照"一节。

## 当前顺序与范围

用户决定当前先跑通原版内容的采集、传输、Client 原生应用和统一整体验收。官方 REPENTOGON 加载器、J273 目标及原版需要的官方扩展 owner 继续沿用；“原版”指内容范围，不改变加载方式。本项目自带界面与 E2E 夹具继续按原职责管理，不构成第三方 Mod 兼容声明。

第三方 Mod 自定义状态、角色动态身份、专用资源／绘制、玩法 Lua 与表现隔离、生命周期及 StageAPI 等具名适配集中保存在本文。它们不延后原版必要功能，也不要求本轮预留可工作的通用 Mod 框架。后续开展时再确定实施范围和验收安排；本文不自动启动适配或实机操作。

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
- 沿用 `graph.lua` 的节点、项数、字节、深度预算，并单独计时；超预算、抛错、缺来源或读取超时必须报告具名缺口，不能截断后宣称完整。原草稿提出超时降频，具体失败后的发布／ACK 语义仍待设计。

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

主线程开销与"声明的字段数 + 容器元素总数"成正比，每个字段一次驻留键的哈希查找，每个元素一次 `lua_geti`／`lua_next`。按每次 Lua API 调用 10—30 ns 粗算（未实测），1000 项、每项 3 个字段的 `list` 约需 0.1—0.2 ms；全部 Mod 共用的每帧读取预算是 1 ms。

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
| 过场动画 | `Isaac.PlayCutscene` | 直接播放（`LuaInterfaces/LuaIsaac.cpp:369`） | Host 上没有采集点，客机看不到 | 原生同步新增过场事件，在调用点记录，客机按事件播放。`ClearGameState = true` 时会结束本局，要与场景退休一起处理 |

**需要核对**

| 项 | 疑点 |
| --- | --- |
| `HUD:SetBossHPBarFill` | 写的是 HUD 字段 `_bossHPBarFill`（`LuaInterfaces/HUD/LuaHUD.cpp:142`）。客机本地 `HUD::Update` 可能按同步来的 Boss 血量重新计算，把 Host 的覆盖值冲掉 |
| `Isaac.CreateTimer` | 创建一个 Effect 实体，Lua 函数存在注册表 `timerFnTable` 里、以 Effect 指针为键（`LuaInterfaces/LuaIsaac.cpp:167`）。这个 Effect 是否会作为普通实体复制到客机；复制过去后客机上没有函数，是否无害 |
| `FXLayers:AddPoopFx` | 调原生 `FXLayers::AddPoopFx`（`LuaInterfaces/LuaFXLayers.cpp:19`）。FX 层的内容是否在同步范围内 |
| Mod 生成的程序道具 | 原版 TMTRAINER 的负 ID 道具已同步。Mod 通过 REPENTOGON 接口生成的程序道具是否走同一路径 |
| Host 音频采集点 | 采集点位于 REPENTOGON 的 `MC_PRE_SFX_PLAY`、`MC_PRE_MUSIC_PLAY` hook 之内还是之外，决定记下来的是回调修改之前还是之后的参数。必须记录修改之后的参数，被取消的声音不记录（mod-api 6.2 节"音频类回调"） |

**已覆盖，不用新增**（目录或代码里有对应项，不代表每个字段都实测过）：Entity 状态倒计时、`SetColorParams`、`SetShadowSize`、`SetSpeedMultiplier`；EntityPlayer 的大部分字段 setter、Weapon、ItemSpoof；Room 的水、熔岩、雨、闪电、背景；Game 的 `SetDizzyAmount`、`SetBloom`、`SetColorModifier`；Sprite／LayerState 的图层、帧、渲染标志、裁切；自定义 shader 和冠军 shader；音乐音高；`ItemOverlay:Show`、`RoomTransition:StartBossIntro`。

**本地界面，不同步**：ImGui、`Options`、窗口标题和图标、`DebugRenderer`、`Minimap:SetState`、`GenericPrompt`。Mod 在 Host 上弹出的提示框，客机不会出现；按 mod-api 7.1 节，这类界面由 Mod 在客机本地自己绘制。

## 后续验收与未决问题

原 E31 的覆盖要求保留在这里：具名 adapter 的编码／解码须覆盖循环、共享、超预算、抛错、超时、引用失效和生命周期变化；超限不得伪装为空或完整，安装失败不得提前确认。StageAPI、角色、资源与绘制还须各自覆盖真实 Client 行为。后续按明确的完整适配目标统一 E2E，连续修复与续测；本次不执行这些用例，也不把迁出记为通过。

仍未确定：仅保留现有具名 adapter，还是提供公开注册接口；各 Mod 的 schema／版本与支持清单；稳定编码、读取耗时与降频／失败发布语义；Client 具体安装／展示 owner、资源寿命及隔离。本文不将这些未决项转为本轮原版验收前置条件。
