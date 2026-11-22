# Mod 联机接口方案（IsaacCS Mod API）

日期：2026-11-22。状态：草案，待用户审阅；没有实现、构建或测试。同日补充客机安装的接入方案（5.6 节、6.2 节回调时机、7.1 节 Sprite 要求），用户已定 mod-adaptation M3、M4、M6、M7、M8，M5 于 2026-11-23 按推荐定。2026-11-23 对照 REPENTOGON 接口补充：6.2 节新增玩家属性查询、显示查询、音效和音乐三类回调；新增 6.4 节"运行时配置表与本机数据"；第 9 节补对应失败方式，第 10 节补音频验收，第 12 节新增 D12；需要原生同步补上的项记在 mod-adaptation"具名适配交接"。本文取代[第三方 Mod 适配：后续方案](archive/design/mod-adaptation.md)里"仅保留具名 adapter 还是提供公开注册接口"这一未决项，其余迁入内容和历史事实仍以原文为准。

## 1. 结论与取舍

**由 Mod 作者声明，联机层提供基础设施；对不上就拒绝连接。**

讨论过三条路线：

| 路线 | 做法 | 结论 |
| --- | --- | --- |
| 通用自动捕获 | 自动遍历 Mod 的全部 Lua 状态 | 不可行。状态的语义（谁读、以什么为键、何时换表）无法自动得出 |
| 不改 Mod、外部适配 | DLL 接管注册函数，用 C API 读写回调上值，再为每个 Mod 写外部适配器 | 技术上可行，但 Mod 每次更新都可能让适配器失效，维护成本不可控。只保留为内部诊断手段 |
| **声明式 API 加协商** | Mod 声明"客机绘制要读的状态"和"客机 UI 要提交的操作"，联机层负责采集、传输、安装和上行；声明对不上就拒绝连接 | **采用** |

做法参照 Minecraft NeoForge：连接时同步注册表 ID，网络包带版本协商；常见的同步需求做成声明式工具（`SynchedEntityData`、带 `sync` 的 Data Attachment），容器菜单由服务端持有状态、客户端只提交按钮 id。我们和它的区别是：**客机不运行玩法**，所以 Mod 作者只需要回答两个问题：

1. 客机上的绘制代码要读哪些状态？对应第 5 节"状态同步"。
2. 客机上的 UI 要提交哪些操作？对应第 7 节"意图"。

Host 上的玩法 Lua 照常运行，不需要同步逻辑，只同步结果。

## 2. 总体模型

```text
  Client                                                  Host
 ┌────────────────────────────────┐                ┌──────────────────────────────────┐
 │ 加载 Mod：IsaacCS.Declare/State │                │ 加载 Mod：同一份声明                │
 │          │                     │── Mod 清单 ──▶ │ 比对清单、声明描述与 ID 表           │
 │          │                     │◀─ 接受/拒绝 ── │                                  │
 │                                │                │ 原生 Game::Update（含全部 Mod 回调）│
 │                                │                │          │                       │
 │                                │                │ Mod 状态读取：调 get，规范化为字节   │
 │ World 原生安装                  │◀─ World 帧 ─── │ 与实体同帧编码进 World 的 Mod 段     │
 │ Mod 状态安装：调 set/写 GetData  │                │                                  │
 │ 显示推进 → 原生 Render          │                │                                  │
 │   └ 只触发绘制类 Mod 回调        │                │                                  │
 │ Mod UI → SendIntent ───────────┼── 意图 ──────▶ │ Manager 边界执行 handler(player)   │
 │ 意图结果回调 ◀──────────────────┼── 事件 ─────── │ 结果随 World 事件队列返回            │
 └────────────────────────────────┘                └──────────────────────────────────┘
```

## 3. 支持级别与默认策略

| 类别 | 判定 | 客机行为 |
| --- | --- | --- |
| 纯内容 Mod | 目录中没有 `main.lua` | 直接可用。物品、实体、动画、音效等原生内容由现有二进制同步覆盖 |
| 已声明 Mod | `main.lua` 调用了 `IsaacCS.Declare` | 按第 6.2 节放行回调，按声明同步状态和意图 |
| 未声明的 Lua Mod | 有 `main.lua`，但没有声明 | 默认拒绝连接（D1）。Host 可以在大厅勾选"以未适配状态运行"，此时客机上不触发该 Mod 的任何 Lua 回调，只同步它的原生内容 |
| 兼容 Mod | 一个独立的 Mod，只调用公开 API，为另一个 Mod 补声明 | 等同于已声明。不得读取对方的上值或内部函数 |

本项目自带的界面 Mod、观测 Mod 和 REPENTOGON 自身不参与以上判定，职责不变。

### 3.1 SDK 文件

Mod 不应依赖联机层是否安装。本项目发布一个单文件 `isaaccs_sdk.lua`，Mod 把它复制进自己的目录，在 `main.lua` 顶部引入：

```lua
local IsaacCS = include("isaaccs_sdk")
```

| 项 | 规则 |
| --- | --- |
| 选择实现 | 加载时如果已经存在联机层提供的真实 `IsaacCS`，直接返回它；否则安装不联机实现（3.2 节），赋给全局 `IsaacCS` 并返回 |
| 多份共存 | 多个 Mod 可能各带一份不同版本的 SDK。不联机实现只会被更高版本的不联机实现替换，替换时沿用已登记的声明；任何版本都不替换真实实现 |
| 职责 | "联机层是否存在、版本是否够用"的检查由 SDK 完成，Mod 作者不用自己写 `if IsaacCS then` 之类的分支 |
| 注释与类型注解 | 每个函数带中文注释（联机时在哪一端、什么时机执行；不联机时的行为），并带 LuaCATS 注解（`---@class`、`---@param`、`---@return`），供 lua-language-server 补全和检查；值类型描述也给出注解类型 |
| 稳定性 | SDK 的 `api` 版本只增不减。已发布函数的签名和不联机语义不再改动，新功能只能以新增函数的形式加入 |
| 版本不足 | Mod 带的 SDK 比已安装的联机层更新时，真实实现上缺少的函数由 SDK 补上不联机版本，单机照常可玩；该 Mod 的 `Declare` 记为"api 版本不受支持"，联机时按未声明处理（D1），大厅提示更新联机层 |
| 注入时机 | 真实 `IsaacCS` 必须在任何 Mod 的 `main.lua` 执行之前就已存在，否则 SDK 会先装上不联机实现。实施时核对 DLL 能否在 Mod 加载前注入全局表；做不到时，真实实现加载后接管不联机实现已登记的声明 |

### 3.2 不联机时的行为

"不联机"包括两种情况：没有安装联机层（SDK 的不联机实现）；安装了但没有开启会话（真实实现）。两者的行为必须一致，以下表为准：

| 接口 | 不联机时 |
| --- | --- |
| `Declare`、`State`、`EntityData`、`EntityState`、`Intent`、`Keys` | 照常登记并检查参数（加载期限制、类型描述是否合法），不调用 `get`、`set`、`reset`，没有运行开销 |
| `MarkDirty` | 空操作 |
| `IsClient()` | `false` |
| `GetParticipant(player)` | `0`。本机的所有身体（包括本地同屏多人）都属于参与者 0 |
| `GetPlayerByParticipant(o)` | `o` 为 0 时返回 `Isaac.GetPlayer(0)`，否则返回 `nil` |
| `GetLocalPlayer()` | `Isaac.GetPlayer(0)` |
| `IsLocalPlayer(player)` | `true` |
| `SendIntent` | 立即执行 handler，传入 `Isaac.GetPlayer(0)`，返回序号。结果回调不在 `SendIntent` 内部触发，而是延到下一帧，保证调用方先拿到序号 |
| `IsKeyPressed`、`IsKeyTriggered` | 读本机键盘（`Input.IsButtonPressed(key, 0)`、`Input.IsButtonTriggered(key, 0)`），不区分 player |
| `AddCallback(mod, id, fn)` | 照常登记。`CLIENT_INTENT_RESULT` 会触发，其余 `CLIENT_*` 回调不触发 |

为了让 Mod 只写一条代码路径，联机时 Host 本机发起的意图，其结果同样以 `CLIENT_INTENT_RESULT` 在本机回调（7.2 节原文没有写，这里补充）。

## 4. 握手协商

### 4.1 Mod 清单

客机 Hello 之后，双方经可靠通道交换 `ModManifest`（新消息类型，编号在实施时登记到 `wire.json`）：

| 项 | 内容 |
| --- | --- |
| 启用的 Mod | 每项包括目录名、`metadata.xml` 的 name 和 version、是否有 `main.lua` |
| 声明描述 | 每个已声明 Mod 的 `api` 版本、Mod 自报版本，以及全部 State、EntityData、Intent、Keys 声明的规范化文本（名称、作用域、类型、上限，按声明顺序） |
| ID 表 | Mod 新增内容的"名字 → ID"表：道具、饰品、卡牌、药丸效果、实体（type/variant/subtype）、角色、挑战、音乐、音效、Costume 等 |

匹配规则：两端启用的 Mod 集合完全相同（与顺序无关），版本字符串相等，声明描述逐字相等，ID 表逐项相等。全部按内容直接比较，不使用哈希。任何一项不等都拒绝连接，并在大厅显示首个差异（某个 Mod 缺失、版本不同、某项声明不同、某个 ID 不同）。

ID 不一致时首版直接拒绝，不做重映射（D2）。两端启用集合相同时，原生按加载顺序分配的 ID 通常一致；要做重映射，就得改动线上所有原生 ID 字段，代价过大。

资源文件内容（anm2、png、xml）无法靠版本字符串核实。首版只检查版本号相等，文件内容不一致导致的显示差异记为已知风险。

### 4.2 声明的时机

- 所有声明只能在 Mod 加载期间调用（`main.lua` 顶层，或 `MC_POST_MODS_LOADED` 之前）。之后再调用会报错并记入日志。
- 声明在首次握手时冻结。线上用"Mod 序号加声明序号"编号，不传名字。
- `luamod` 热重载会改变声明。会话进行中检测到重载时，结束会话。

## 5. 状态同步

### 5.1 作用域

| 作用域 | 含义 | 接收者 |
| --- | --- | --- |
| `global` | 整局只有一份 | 所有客机，不能限制 |
| `player` | 每个参与者一份 | 默认所有客机都收到每一份；声明 `private = true` 时只发给该参与者本人 |
| `entity` | 每个实体一份 | 所有客机 |

`private` 只允许用在 `player` 作用域上。私有的含义是"其他人的客机上没有这一份"，读取时得到默认值。接收资格只随参与者加入、退休、重连变化，这几个时机本来就会走完整基线，不需要单独的资格切换协议。

### 5.2 值类型

声明时给出类型，Host 读取后按类型规范化，客机按类型校验：

| 类型 | 线上编码 |
| --- | --- |
| `bool`、`int`（i32）、`float`（f32）、`number`（f64） | 定长小端 |
| `string{max=N}` | u16 长度加字节，N ≤ 1024 |
| `vector`、`color`、`kcolor` | 2 个、7 个、4 个 f32 |
| `entity` | 实体成员号，Null 为 `0xFFFF` |
| `player` | 参与者 ordinal，Null 为 `0xFF` |
| `list{of=T, max=N}` | u16 项数加元素 |
| `map{key="string"或"int", of=T, max=N, keymax=K}` | u16 项数加按键排序的键值对。`keymax` 只用于字符串键，是键的字节上限，默认 64 |
| `struct{字段=类型, ...}` | 开头一张存在位图（每字段 1 位），之后按声明顺序拼接各字段；缺失字段的槽位填 0。至少一个字段 |

具体布局（定长槽大小、容器内容的存放顺序、最坏字节数的计算）见[第三方 Mod 适配](archive/design/mod-adaptation.md#声明类型的采集与编码)"声明类型的采集与编码"一节。

只有上面这些强类型，不提供照搬任意 Lua 表的动态类型（D4）。不允许函数、协程、userdata（上面列出的引用类型除外）。`map` 按键排序，`struct` 按声明顺序，这样同一个值总是得到同一段字节，变化判断可以直接 memcmp。不能依赖 `pairs` 的顺序：字符串哈希的种子每个 `lua_State` 随机生成（Lua 5.4 `lstate.c` 的 `luai_makeseed`），同一张表在两端的遍历顺序一般不同。

几种容易选错的情况：

| 情况 | 用法 |
| --- | --- |
| 键是从 1 开始的连续整数 | `list`。Host 从 1 开始逐个取值，取到第一个 `nil` 为止 |
| 整数键有空洞，或者从 0 开始（如按道具 ID 计数、按槽位 0—3 存放） | `map{key="int"}`。用 `list` 会在第一个空洞处截断 |
| 键是种子、房间号等转成的字符串 | `map{key="string"}` |
| 字段有时为 `nil` | 放在 `struct` 里，靠存在位表示 |
| 同一字段时而整数、时而小数（如初值 `1`、加点后 `1.25`） | `number`。客机收到的 `int` 是整数子类型，`number` 是浮点子类型，所以 Host 上的 `1` 到客机是 `1.0`，`tostring` 结果不同 |

### 5.3 声明形式

```lua
-- 全局：默认读写 mod[key]，也可以自己提供 get/set
IsaacCS.State(mod, "run", {
  scope = "global",
  type  = { struct = { phase = "int", timer = "float", boss = "entity" } },
  get   = function() return mod.run end,
  set   = function(v) mod.run = v end,       -- 仅在客机调用
  reset = function() mod.run = nil end,      -- 客机离开场景或换局时调用
})

-- 每个参与者一份；私有的只发给本人
IsaacCS.State(mod, "hand", {
  scope = "player", private = true,
  type  = { list = { of = "int", max = 16 } },
  get   = function(player) return mod.hands[IsaacCS.GetParticipant(player)] end,
  set   = function(player, v) mod.hands[IsaacCS.GetParticipant(player)] = v end,
})

-- 实体级：同步 GetData() 中列出的键
IsaacCS.EntityData(mod, { type = 1000, variant = MyEffect }, {
  charge = "int",
  phase  = "int",
})

-- 实体级，状态放在 Mod 自己的表里时：自己提供 get/set
IsaacCS.EntityState(mod, "orbit", {
  filter = { type = EntityType.ENTITY_FAMILIAR, variant = MyFamiliar },
  type   = "vector",
  get    = function(ent) return mod.orbit[GetPtrHash(ent)] end,
  set    = function(ent, v) mod.orbit[GetPtrHash(ent)] = v end,
})
```

`get` 和 `set` 拿到的实体，都是本端的实体对象。`GetPtrHash` 在两端各自有效，换键由联机层完成：Host 按成员号编码，客机按成员号找到本地实体再调用 `set`。

`get` 返回的表里如果有 `struct` 没有声明的字段，直接忽略，不算类型不符；嵌套在 `list`、`map` 里的 `struct` 同样处理。`get` 只在本端执行，返回什么由 Mod 作者自己负责。实现上按声明的字段名逐个取值（走普通索引，带 `__index` 的对象也能用），不遍历表，所以 `get` 可以直接返回 Mod 的活表，不用每帧另建一张投影表。影响：
- 未声明的字段不传输，也不参与变化判断，只改这些字段不会产生记录。
- 客机上 `set` 收到的表只有声明过的字段。绘制代码如果读了忘记声明的字段，客机上就会读到 `nil`，而且不报错。诊断模式下，每个声明首次读取时把被忽略的字段名记进日志，提示作者检查。
- 默认路径（没写 `set`）按下面的"默认写回"规则写入，客机本地放在同一张表里的未声明字段会保留。

**顶层空值**（D10）：`get` 整体返回 `nil` 时，记录的字节数为 0，客机调用 `set(nil)`。每种类型的非空编码都至少 1 字节，所以不会和真实的值混淆，为此 `struct` 至少要有一个字段。值内部的空值仍用 `struct` 的存在位，以及 `entity` 的 `0xFFFF`、`player` 的 `0xFF`。

**默认写回**（D11）：没写 `set` 时，客机按以下规则把值 `v` 写回 `mod[key]`；EntityData 写 `GetData()[key]` 时用同一套规则：

| 情况 | 写法 |
| --- | --- |
| `v` 为 `nil` | `mod[key] = nil` |
| 标量类型（`bool`、`int`、`float`、`number`、`string`、`vector`、`color`、`kcolor`、`entity`、`player`） | 直接赋值 |
| `struct`，且目标已经是表 | 原地合并：逐个写入声明过的字段，缺少的写 `nil`；未声明的字段和元表保留。字段本身是 `struct`、`list` 或 `map` 时，按本表递归处理 |
| `list`、`map`，且目标已经是表 | 原地清空再重填，表本身不换；元素直接替换，`struct` 元素不逐项合并 |
| 目标不是表（`nil` 或其他类型） | 新建一张表，按上面的规则填入后赋值 |

这样各层表的引用、元表（类写法的对象）和客机本地字段都不会因同步而丢失。写回只在记录变化的那一帧执行。

**复杂表的写法**：Mod 常把状态放在一张多层的大表里，例如每个玩家一张表，各个功能在里面挂一张自己的子表。这种表都能用 `struct`、`list`、`map` 写出来，但不要整张声明成一条记录，而要：

1. **按子表拆成多条声明**。每条声明短、类型简单，Mod 更新时改动范围小；一条记录变化时只重发这一条（5.5 节不做记录内部的字段级增量）。
2. **只声明客机绘制和 HUD 会读的字段**。只在 Host 玩法逻辑里用的计数器、令牌、RNG 种子不用声明。
3. **每玩家一份的数据用 `player` 作用域**，不要把"玩家键 → 子表"整体声明成 `map`。Mod 自己的玩家键（如某个道具 RNG 的种子）两端不一定相同，`player` 作用域按参与者对应身体，两端各自用本地的键读写。
4. **`get` 每次重新取表**，不要缓存表引用。Mod 可能在读档、回溯时把整张表换掉。
5. **大容器单独声明，并配 `rate = "mark"` 与 `MarkDirty`**。Lua 表没有修改标记，按 `"world"` 读取的声明即使没变也要每帧整条读完，记录里任意一项变化都要整条重发（5.5 节）。项数多、变化少的 `list`、`map`（例如按道具 ID 计数的表）单独成一条声明，Mod 修改后调用一次 `IsaacCS.MarkDirty`，读取次数就从每帧一次降到变化次数，其他字段的频繁变化也不会连带它重读、重发。

```lua
-- 例：某角色的加点面板，客机 HUD 要显示；数据放在 Mod 自己的玩家表里
IsaacCS.State(mod, "beden_panel", {
  scope = "player",
  type  = { struct = {
    Points = "int",
    section = "int", row = "int", column = "int", page = "int",
    spd = "number", tears = "number", dmg = "number",
    range = "number", sspd = "number", luck = "number",
  } },
  get = function(player) return mod.Players:GetData(player).BEDEN end,
  set = function(player, v)                       -- 仅在客机调用
    local data = mod.Players:GetData(player)       -- 按客机本地的玩家键取表
    data.BEDEN = data.BEDEN or {}
    if v == nil then return end
    for k, x in pairs(v) do data.BEDEN[k] = x end  -- 只覆盖声明过的字段，其余本地字段保留
  end,
})
```

默认写回的目标是 `mod[key]`。像上例这样，数据放在 Mod 自己的嵌套表里时，要自己写 `set`。

依据：2026-11-22 考察了 Benighted Soul（创意工坊 3290493485）。它的整局状态 `mod.IBS_Data.Temp` 以存档 JSON 为前提，没有循环和共享引用；玩家数据按道具 RNG 种子字符串分键，每玩家二十多张子表，都能用现有类型表示，难点只在字段多。

### 5.4 Host 读取

- **时机**：World 采集窗口，即普通帧原生 `Game::Update` 返回后，在主线程上。只调用 `get`、规范化、写入自有的字节缓冲；比较和编码在后台线程完成。
- **频率**：每个声明可以选 `rate`（D5）：
  - `"world"`：每个 World 帧都读（约 30Hz），默认值；
  - `"low"`：约 10Hz；
  - `"mark"`：只在 Mod 调用 `IsaacCS.MarkDirty(mod, key[, target])` 后的下一个 World 帧读，最省开销。
  - 诊断模式下，声明中含 `max` 超过 256 的 `list` 或 `map`、却按 `"world"` 读取时，加载期在日志里提示该 Mod 和该声明的名字，建议改用 `"mark"`（5.3 节"复杂表的写法"第 5 条）。只提示，不拒绝。
- **预算**：所有 Mod 的 Lua 读取共用一个每帧时间上限（初值 1 ms），每个声明另有字节上限。超出时，该声明本帧标为"沿用"并计数；连续超出时自动降频，并在日志和大厅状态里报告该 Mod 和该声明的名字。不截断，也不把超限的值当成空值。
- **出错**：`get` 抛错、返回值类型不符或超出上限时，该记录本帧沿用并计数，不影响其他记录（对应 C9）。

### 5.5 线格式

声明如何编译成布局和读取程序、Host 怎样采集并规范化成字节，见[第三方 Mod 适配](archive/design/mod-adaptation.md#声明类型的采集与编码)"声明类型的采集与编码"一节。

- World 帧的分段之后增加一个 Mod 段。每条 Mod 记录的头为：u16 Mod 序号、u16 声明序号、u16 目标（global 为 `0xFFFF`，player 为 ordinal，entity 为成员号）、u8 接收者（`0xFF` 表示所有人，否则为 ordinal）、u32 字节数，后接按 5.2 节规范化的值；字节数为 0 表示顶层空值（D10）。
- 变化判断以记录为单位：整段值 memcmp，记录"最后变化版本"，增量规则与平铺记录相同（3.2 节）。首版不做记录内部的字段级增量。
- 快照仍由三个 Peer 共享。按 Peer 组帧时，跳过接收者不是该 Peer 的私有记录。
- entity 作用域的记录与对应实体同生命周期：实体离开时一起删除，"沿用"时一起沿用。
- Mod 段本身的格式变化需要把布局版本加 1。各 Mod 声明的内容不进布局版本，只在握手时比较。
- 占用 World 的预算（150,000 B/s，单条消息不超过 64 KiB）和三池内存账本。

### 5.6 客机安装

结构校验、影子状态和字节到 Lua 值的构建规则，见[第三方 Mod 适配](archive/design/mod-adaptation.md#7-客机校验影子状态与安装)"声明类型的采集与编码"第 7 节。

如何接进客机现有流程（快照绑定、安装集、回调顺序、实体生灭、场景生命周期、Lua 调用框架），见[第三方 Mod 适配](archive/design/mod-adaptation.md#77-接入客机管线)第 7.7 节"接入客机管线"；客机已定事项 M3—M8 见该文第 8 节。

- **时机**：同一次 Manager 边界调用中，World 原生安装之后、显示推进和 Render 之前（对应 C3）。这样绘制回调读到的 Mod 状态与实体属于同一帧。Mod 状态随 World 封存为同一份快照，主线程装哪份 World 就装哪份 Mod 状态。
- **写入**：global 和 player 作用域调用 `set`，没写 `set` 时按 5.3 节"默认写回"规则写 `mod[key]`；EntityData 按同一规则写进本地实体的 `GetData()[key]`；EntityState 调用 `set(ent, v)`。每条记录单独 `pcall`。字节与上次成功写入的相同时不写。
- **完整基线**（M8，2026-11-22 已定）：World 完整基线到来时，全部记录重写一遍，与原生的做法一致，用来纠正客机上被改动过的已声明键；内容没变的记录不派发 `CLIENT_STATE_CHANGED`。因此 `set` 可能以相同的值被多次调用，必须保证同值重写没有副作用。
- **失败**：单条记录失败只记日志和计数，不影响 ACK（对应 C2），也不请求 Resync，因为 Lua 报错不会因为重发基线而消失。
- **实体重建**：客机重建实体后，从本地影子状态里取该实体最近一次的 Mod 值重新写入，不需要 Host 重发。
- **只读约定**：在客机上，已声明的键归 Host 所有，Mod 的客机代码不应修改它们（D6）。如果修改了，要等到 Host 下一次变化才会被纠正。诊断模式下，每个显示批次前重写全部已声明的键，并统计被改动的键。
- **清理**：客机离开场景、换局或会话结束时，先派发 `CLIENT_SCENE_ENDED`，再逐个调用 `reset`，此时实体和 Lua 都还在；EntityData 随实体一起消失。参与者离开时，它的 player 作用域记录不调用 `set`，只删已安装条目；身体作为实体离开时照常派发 `CLIENT_ENTITY_REMOVED`，Mod 在这个回调里自己清理（M5，2026-11-23 已定）。

## 6. Lua 逻辑与回调

### 6.1 Host

- 所有回调照常运行。远端玩家就是原生 Join 的玩家，Mod 看到的是正常的多人游戏。
- `IsaacCS.GetParticipant(player)` 返回该身体所属的参与者 ordinal（0 是房主），`IsaacCS.GetPlayerByParticipant(ordinal)` 反过来查。
- **Lua 输入查询**：`Input.IsActionPressed`、`Input.IsActionTriggered`、`Input.GetActionValue` 以远端玩家的 `ControllerIndex`（虚拟设备 K）查询时，应当返回该参与者的统一输入。这一点需要在实施时核对：现在路由的是原生输入消费者，Lua 的 `Input.*` 是否经过同一条路径还不确定。
- **扩展按键**（D7）：`IsaacCS.Keys(mod, { Keyboard.KEY_F, ... })` 声明 Mod 自定义的原始按键。客机采样这些按键，随输入记录上传；Host 上用 `IsaacCS.IsKeyPressed(player, key)` 和 `IsaacCS.IsKeyTriggered(player, key)` 查询。远端玩家用 `Input.IsButtonPressed(key, idx)` 查原始键盘的写法不支持，因为客机的键盘不在 Host 上。

### 6.2 客机回调放行

依据：REPENTOGON 分派的每个回调条目都带有 `callback.Mod`（`repentogon/resources/scripts/main_ex.lua:1305` 的 `RunCallbackInternal`），所以可以按"哪个 Mod 注册的"加上"回调种类"过滤。实施时要核对两件事：C++ 直接分派的回调是否都经过 `_RunCallback` 系列入口；过滤是做在注册时包装函数，还是做在分派时检查。

| 回调类别 | 已声明 Mod | 未声明 Mod（未适配运行） |
| --- | --- | --- |
| 绘制和 HUD（`MC_POST_RENDER`、`MC_POST_*_RENDER`、`MC_PRE_*_RENDER`、HUD 系列、`MC_GET_SHADER_PARAMS`） | 触发（D3） | 不触发 |
| 菜单和本地界面（ImGui、`MC_MAIN_MENU_RENDER`、暂停菜单等） | 触发 | 不触发 |
| Init 类（`MC_POST_*_INIT` 等） | 不触发，改用 `IsaacCS.CLIENT_ENTITY_INSTALLED` | 不触发 |
| 更新、伤害、碰撞、生成、Cache（含 `MC_EVALUATE_CUSTOM_CACHE`）、拾取、房间与楼层等玩法回调 | 不触发 | 不触发 |
| 玩家属性查询（`MC_PLAYER_GET_HEALTH_TYPE`、`MC_PLAYER_GET_HEART_LIMIT`、`MC_PLAYER_GET_ACTIVE_MAX_CHARGE`、`MC_PLAYER_GET_ACTIVE_MIN_USABLE_CHARGE`） | 不触发。客机原生 HUD 调用这些查询时，应当取 Host 同步来的有效结果（原生待补项，见下文"查询类回调"） | 不触发 |
| 显示查询（`MC_PRE_GET_LIGHTING_ALPHA`） | 触发，与绘制类相同（D3） | 不触发 |
| 音效和音乐（`MC_PRE_SFX_PLAY`、`MC_POST_SFX_PLAY`、`MC_PRE_MUSIC_PLAY`、`MC_PRE_MUSIC_PLAY_JINGLE`、`MC_PRE_MUSIC_LAYER_TOGGLE`） | 回放 Host 的音效、音乐事件时不触发；客机本地发出的声音（例如 Mod 在绘制回调里调用 `SFXManager():Play`）照常触发。见下文"音频类回调" | 不触发 |

**查询类回调**（2026-11-23 对照 REPENTOGON 补充）：REPENTOGON 在 `GetHealthType`、`GetHeartLimit`、`GetActiveMaxCharge`、`GetActiveMinUsableCharge` 这些原生查询里触发对应回调，客机的原生 HUD 和绘制代码同样会调用这些查询。这些回调的返回值取决于 Host 的玩法状态，客机上不触发；客机要显示正确的心格类型、心上限和充能条，需要原生同步传来 Host 上的有效结果，客机查询时直接返回该结果。目录里已把 `GetHealthType`、`GetHeartLimit` 标为"最终显示缓存"，具体实现归原生同步，见[第三方 Mod 适配](archive/design/mod-adaptation.md#具名适配交接)"具名适配交接"。补上之前，客机按原版值显示，记为已知差异（第 9 节）。`Room:GetLightingAlpha` 只影响显示，Mod 可以用已声明的状态算出同样的结果，所以按绘制类放行。

**音频类回调**（2026-11-23 补充）：Host 上 Mod 可以在 `MC_PRE_SFX_PLAY`、`MC_PRE_MUSIC_PLAY` 里换掉声音 id、改参数或取消播放。客机回放 Host 的音效事件时同样调用原生 `Play`，REPENTOGON 会在客机上再触发一次这些回调；如果放行，同一个声音会被改写或取消两次。规则：

- 客机回放 Host 事件期间，联机层设一个"回放中"标记，分派过滤器看到标记就跳过这五个回调。过滤器怎么拿到这个标记，与本节开头的两项核对一起在实施时确定。
- 客机本地发出的声音不带标记，回调照常触发，由 Mod 自己处理。
- Host 端：音频采集点必须记下 REPENTOGON 回调处理之后的最终参数（换过的 id、改过的音量音高），被取消的声音不记录。采集点位于 REPENTOGON hook 之内还是之外，实施时核对（见 mod-adaptation"具名适配交接"）。

这些规则在 Host 上不生效。客机上另外提供几个联机层回调，用 `IsaacCS.AddCallback(mod, id, fn)` 注册：

| 回调 | 时机 |
| --- | --- |
| `CLIENT_SCENE_STARTED`、`CLIENT_SCENE_ENDED` | 客机场景开始、退休；代替 `MC_POST_GAME_STARTED`、`MC_PRE_GAME_EXIT`。STARTED 在第一份 World 及其 Mod 状态装好之后、首次显示推进之前；ENDED 在原生拆卸之前、`reset` 之前 |
| `CLIENT_ENTITY_INSTALLED(ent)`、`CLIENT_ENTITY_REMOVED(ent)` | 实体首次安装（含重建）后、移除前，用于纯显示的初始化，比如创建 Sprite。INSTALLED 在该实体的 entity 记录写完之后，回调里能读到已同步的数据；REMOVED 在本批原生安装开始之前、对计划内离开或重建的实体派发，此时实体还在；原生安装失败造成的计划外释放不派发，只计数；同一实例只派发一次，与 INSTALLED 成对（mod-adaptation M3）。场景退休时不逐个实体派发 REMOVED |
| `CLIENT_STATE_CHANGED(key, target)` | 本批全部记录写完后统一派发，只为内容确实变化的记录派发（可选） |
| `CLIENT_INTENT_RESULT(name, seq, ok, result)` | 意图结果到达时（第 7.2 节） |

### 6.3 客机副作用防护

> **现阶段不做（2026-11-22）**：本节新增的拦截（生成和移除实体、改玩家和整局状态）暂不实施，保留为后续设计。现有的持久化和随机数隔离不受影响，照常生效。在此之前，已声明 Mod 要自己用 `IsaacCS.IsClient()` 隔开绘制回调里的玩法代码（与 D3 相同）。客机上如果仍执行了这类调用，结果不确定：例如本地多出来的实体可能与成员系统冲突。这一点记为已知风险。未声明的 Mod 在"未适配运行"下，客机上本来就不触发任何回调，不受影响。

已声明 Mod 的绘制回调可能夹带玩法副作用。按后续设计，客机上以下 Lua 调用只拦截、计数（按 Mod 统计，进入 `client_isolation`），返回中性值：

| 类别 | 例子 | 处理 |
| --- | --- | --- |
| 生成和移除实体 | `Isaac.Spawn`、`Entity:Remove`、`Kill` | 拦截。客机的实体表由安装流程管理，本地多出来的实体会与成员系统冲突 |
| 改玩家和整局状态 | `AddCollectible`、`AddCoins`、`UseActiveItem`、`Game():…` 上的写操作 | 拦截 |
| 持久化 | `SaveModData` 等 | 已拦截（`client_native_bridge.cpp:150`） |
| 随机数 | `math.random`、`RNG` | 已拦截 |
| 显示 | `Sprite`、`Font`、`Isaac.RenderText`、`SFXManager():Play` | 放行 |

完整的拦截清单在实施时按 REPENTOGON 文档逐项列出。

### 6.4 运行时配置表与本机数据（2026-11-23 补充）

**配置表只在加载期修改。** 下面这些接口改的是进程内的全局配置表，不属于任何同步对象：

| 接口 | 改的是 |
| --- | --- |
| `ItemConfig_Item:AddCustomTag`、`RemoveCustomTag`、`AddCustomCacheTag`、`RemoveCustomCacheTag` | 道具配置的自定义标签 |
| `ItemConfig_Card:SetAvailabilityCondition`、`ClearAvailabilityCondition`，`Weight` 字段 | 卡牌是否可出现的判定函数和权重 |
| `RoomConfigStage` 的各个 `Set*` | 楼层配置（`RoomConfig._stages[]`） |

在 `main.lua` 顶层或 `MC_POST_MODS_LOADED` 里调用时，两端执行同一份代码，结果一致。在玩法回调里调用时，只有 Host 改了，客机上 Mod 的绘制代码读到的是旧值；握手比对的是声明，查不出这种差异。SDK 规则：这类接口只在加载期调用；运行时修改记为已知风险（第 9 节），联机层不同步、不检测。

例外：`Level:SetName` 和 `Isaac.SetCurrentFloorName`、`SetCurrentFloorMusic`、`SetCurrentFloorBackdrop` 本来就是在局内调用的，改的是当前楼层的显示名、音乐和背景，归原生同步处理（见 mod-adaptation"具名适配交接"），不受上面的规则限制。

**本机数据。** 客机上下列查询返回的是客机自己的数据，不是 Host 的：

| 接口 | 结论 |
| --- | --- |
| `Options` | 用本机的值，正确。和本机音量一样，属于本地设置 |
| `PersistentGameData` 的解锁、完成标记、统计 | 返回客机自己存档的结果。Mod 按解锁状态决定显示（例如未解锁时画锁图标）时，两端会不一样。首版记为已知差异（D12） |

## 7. 自定义 UI

### 7.1 渲染

- Mod 在客机本地用自己的 Sprite 和 Font 绘制，读取第 5 节同步来的状态。
- UI 自己的状态（菜单是否打开、光标、选中项、动画帧）放在 Mod 未声明的变量里，留在本地，不同步。
- **Mod 自己用 `Sprite()` 创建的 Sprite**（M7，2026-11-22 已定）：`Update`、`Play`、`SetFrame` 等推进动画的调用必须写在绘制回调里，由已声明的状态决定播哪个动画。写在玩法回调（`MC_POST_UPDATE`、`MC_NPC_UPDATE` 等）里的，客机上不会执行，Sprite 会停住或播错。联机层不提供同步动画名和帧的辅助手段。实体自带的 Sprite（`ent:GetSprite()`）是原生对象，由原生同步覆盖，不受此限。
- `IsaacCS.GetLocalPlayer()` 返回本机参与者的身体，`IsaacCS.IsLocalPlayer(player)` 判断某个身体是否属于本机。客机上 `Isaac.GetPlayer(0)` 是房主的角色，"画当前玩家"的 UI 要改用这两个接口。
- 没有联机时，`GetLocalPlayer()` 返回 `Isaac.GetPlayer(0)`。

### 7.2 意图（客机 UI 触发状态修改）

```lua
-- 声明：两端都要执行，Host 用它来处理
IsaacCS.Intent(mod, "buy", {
  args    = { struct = { slot = "int" } },
  handler = function(player, args)        -- 只在 Host 的 Manager 边界执行
    local item = mod.shop[args.slot]
    if not item or player:GetNumCoins() < item.price then return false, "无法购买" end
    player:AddCoins(-item.price); player:AddCollectible(item.id)
    return true
  end,
})
-- 客机 UI 确认时
local seq = IsaacCS.SendIntent(mod, "buy", { slot = 3 })
```

| 项 | 规则 |
| --- | --- |
| 统一路径 | Host 本机、客机、没有联机时都调用 `SendIntent`。Host 本机直接进入同一个队列；没有联机时立即执行 handler，传入 `Isaac.GetPlayer(0)`。Mod 只需写一条代码路径 |
| 上行 | 新消息类型 `ModIntent`，走可靠、有序通道。每个意图内容不超过 512 B，每个 Peer 在途不超过 8 个，每秒不超过 20 个；超出时 `SendIntent` 返回 nil |
| 执行 | Host 收到后排队，在下一次最外层 `Manager::Update` 入口按到达顺序执行，与 `cs` 命令相同。不在网络线程或 Lua 回调里直接执行 |
| 身份 | handler 收到的 `player` 是发起者当前的身体。发起者没有身体（死亡等待复活、退休中）时拒绝执行 |
| 校验 | handler 自己检查合法性。客机 UI 依据的是旧状态，选项可能已经失效 |
| 结果 | handler 的返回值（ok 加不超过 256 B 的结果）作为发起者的私有事件，进入 World 事件队列（6.2 节），按 ACK 去重，客机触发 `CLIENT_INTENT_RESULT`。handler 抛错视为失败 |
| 状态变化 | handler 造成的状态变化经过正常的原生同步和第 5 节的 Mod 状态同步返回客机 |
| 乐观更新 | 客机不提前修改已声明的状态，UI 只能在本地显示"处理中" |
| 失效 | 换局、换层、会话结束时，未执行的意图全部丢弃，并返回失败结果 |

结果是否纳入首版见 D8。

### 7.3 UI 状态放在 Host（兜底用法）

菜单状态也可以留在 Host：Mod 在 Host 上按 `ControllerIndex` 或扩展按键推进菜单，把菜单状态声明为 `player` 作用域（可以设为私有），客机只负责绘制。这样不用写意图，代价是光标每动一下都要等一个 RTT 才能看到。这种用法适合简单的按键触发，不适合复杂菜单。

### 7.4 Mod 设置

`IsaacCS.State` 加上 `rate = "mark"`，可以用来同步影响玩法的设置：以 Host 为准，客机只用来显示。只影响显示的设置不要声明，各端各用本地的值。

## 8. 与现有合同的关系

| 合同 | 关系 |
| --- | --- |
| Host 唯一权威 | 不变。玩法 Lua 只在 Host 运行，客机不重演 |
| C1、C2 | Mod 记录跟随 World 最终一致；ACK 与安装是否成功无关 |
| C3 | Mod 安装与原生安装在同一次 Manager 边界调用内完成 |
| C5、C9 | 按记录隔离失败，Host 读取失败时沿用 |
| 布局版本 | 只有 Mod 段格式变化时才加 1，声明内容在握手时比较 |
| 预算 | 计入 World、可靠通道和三池内存的现有上限；另加 Host 主线程 Lua 读取时间上限 |
| client.md §8 "Lua 回调一律放行" | 改为按第 6.2 节分级放行 |

## 9. 可能的失败方式

| 失败 | 处理 |
| --- | --- |
| 两端 Mod 集合、版本、声明或 ID 表不同 | 握手拒绝，大厅显示首个差异 |
| 声明在加载期之后才调用，或热重载 | 报错；会话中发生时结束会话 |
| `get` 抛错、类型不符、超限，或总读取超时 | 本帧沿用、计数，连续发生时降频并报告；不截断 |
| 值中有函数、userdata（引用类型以外）或其他未声明的类型 | 按类型不符处理：本帧沿用并计数 |
| `list` 中间有空洞 | 读到第一个 `nil` 为止，后面的元素不传；应改用 `map{key="int"}` |
| 值里引用的实体已离开，或不在成员目录里 | 编码为 Null，并计数 |
| 客机 `set` 抛错 | 跳过该记录并计数，ACK 照发，下一次变化时再写 |
| 客机实体重建 | 从影子状态重新写入 |
| 客机绘制回调修改了已声明的键（例如在绘制回调里推进计时器、消费一次性标志） | 只读约定；Host 下一次变化或完整基线时纠正（M8）；诊断模式下检测 |
| Mod 在玩法回调里推进自己用 `Sprite()` 创建的 Sprite | 客机不触发玩法回调，Sprite 停住或播错。由 Mod 按 7.1 节改到绘制回调里推进，联机层不兜底（mod-adaptation M7） |
| 客机原生 HUD 查询心格类型、心上限、充能，玩家属性查询回调不触发 | 原生同步补上 Host 有效结果之前，客机按原版值显示，记为已知差异（6.2 节"查询类回调"） |
| 客机回放 Host 音效时再次触发 Mod 的音频回调，同一声音被改写或取消两次 | 回放期间不触发（6.2 节"音频类回调"）；Host 采集点须记录回调处理后的最终参数 |
| Mod 在玩法回调里修改 ItemConfig、RoomConfigStage 等配置表 | 只有 Host 生效，客机读到旧值。SDK 规则要求只在加载期修改（6.4 节），运行时修改记为已知风险，不检测 |
| Mod 按解锁、完成标记决定显示 | 客机读的是本机存档，两端可能不一样；首版记为已知差异（6.4 节、D12） |
| 客机绘制回调夹带玩法副作用 | 现阶段不拦截，记为已知风险，由 Mod 用 `IsClient()` 自行隔开；后续按第 6.3 节拦截并计数 |
| 意图过大、过多，或发起者没有身体 | 本地拒绝，或由 Host 拒绝并返回失败结果 |
| 意图到达时状态已变 | handler 自行校验并返回失败 |
| 私有记录发错了接收者 | 组帧时按接收者过滤；E2E 中对照验证 |
| 未声明 Mod 在"未适配运行"下的客机回调 | 一律不触发 |
| Mod 带的 SDK 比已安装的联机层新 | 缺少的函数补不联机版本；联机时该 Mod 按未声明处理，大厅提示更新联机层 |
| 真实 `IsaacCS` 晚于某个 Mod 加载 | 真实实现接管不联机实现已登记的声明；实施时核对注入时机 |

## 10. 验收

按项目规范，全部完成后统一做 E2E：

- **测试夹具 Mod**（只作夹具，不构成支持声明）：用到全部作用域和类型（含私有、嵌套的 `struct`／`list`／`map`、整数键有空洞的 `map`、实体引用），三种读取频率，意图的成功、失败和限流，扩展按键。第 6.3 节的副作用调用等防护实施后再加入。
- **场景**：
  - 不安装联机层、以及安装了但不联机时，夹具 Mod 单机可玩，意图立即生效，结果回调在下一帧到达；
  - 两端 Mod 清单不一致时被拒绝；
  - 未声明的 Mod 在"未适配运行"下，客机上它的回调不触发；
  - 客机 HUD 显示的 Mod 状态与 Host 一致；
  - 私有状态只出现在本人的客机上；
  - 实体重建后实体状态恢复；
  - 意图从客机发出、在 Host 生效、结果回到客机；
  - 超预算时有具名报告；
  - 换房、换层、换局时状态正确清理；
  - 夹具 Mod 在 `MC_PRE_SFX_PLAY` 里换掉某个声音、取消另一个声音：客机听到的与 Host 一致，客机回放时该回调没有被触发；夹具 Mod 在绘制回调里本地发出的声音，客机上的音频回调照常触发。

## 11. 实施顺序

1. 握手时交换 Mod 清单和 ID 表；客机按 Mod 分级放行回调（先做到"未声明的 Mod 不触发"）。
2. `IsaacCS` 全局表骨架：声明的登记和冻结、`IsClient`、`GetParticipant`、`GetLocalPlayer`；`isaaccs_sdk.lua`（3.1 节）与不联机时的行为（3.2 节）。
3. global 和 player 作用域的状态同步：Host 读取、Mod 段、客机安装、私有过滤。
4. entity 作用域：EntityData、EntityState、实体重建、与实体同生命周期。
5. 意图和结果事件。
6. 扩展按键，核对 Lua `Input.*` 的路由。
7. 测试夹具 Mod，统一 E2E。

客机副作用防护清单（第 6.3 节）现阶段不做，不在本轮实施顺序内。

StageAPI、自定义角色等具名适配，在这套 API 跑通之后，作为兼容 Mod 或单独设计推进。

## 12. 待定事项

| 编号 | 问题 | 推荐 |
| --- | --- | --- |
| D1 | 未声明的 Lua Mod 默认怎么处理 | 拒绝连接；Host 可以在大厅勾选"以未适配状态运行" |
| D2 | ID 表不一致 | 首版拒绝，不做重映射 |
| D3 | 已声明 Mod 原有的绘制回调，在客机上是否自动触发 | 自动触发；Mod 作者用 `IsClient()` 隔开里面的玩法代码 |
| D4 | 值类型用强类型还是全动态 | 已定（2026-11-22）：只用强类型，取消 `any`，不采用[原草稿](archive/design/mod-adaptation.md#mod-值编码草稿)的带标签值编码。原推荐"强类型为主，`any` 作为有界补充"。取消原因：循环和共享引用实际用不到（考察的 Mod 以存档 JSON 为前提）；节点、字节、深度三个上限作者难以估计；`any` 整表照搬，违背"只同步客机需要的状态"，体积和变化频率不可控；复杂表按 5.3 节"复杂表的写法"拆成多条声明即可 |
| D5 | 默认读取频率 | 已定（2026-11-22）：默认 `"world"`，`"low"` 和 `"mark"` 作为优化选项 |
| D6 | 客机修改了已声明的键 | 只读约定加诊断模式，不每帧重写 |
| D7 | 扩展按键是否进首版 | 进。Mod 自定义按键很常见，不支持的话 Mod 在客机上会缺功能 |
| D8 | 意图结果回传是否进首版 | 进。没有结果，UI 无法给出失败提示 |
| D9 | 我们是否维护一组官方兼容 Mod | 先不维护。等 API 跑通后，挑一两个热门 Mod 作为示范 |
| D10 | `get` 整体返回 `nil`（顶层空值）怎么编码 | 已定（2026-11-22）：记录字节数为 0 表示 `nil`，客机调用 `set(nil)`；`struct` 至少一个字段。见 5.3 节 |
| D11 | 没写 `set` 时怎样写回 `mod[key]` | 已定（2026-11-22）：递归原地合并（方案 C），EntityData 同规则。见 5.3 节"默认写回"。未采用：整表替换（丢元表和引用）、只合并顶层（内层同样丢失）、容器类型强制写 `set`、加 `merge` 选项 |
| D12 | 客机上 `PersistentGameData` 的解锁、完成标记查询以谁为准（6.4 节） | 首版用客机本机存档，记为已知差异。改为以 Host 为准需要同步 Host 的解锁表并替换客机查询，等确有 Mod 依赖时再定 |
