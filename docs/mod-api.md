# Mod 联机接口（IsaacCS Mod API）

第三方 Lua Mod 通过声明式接口接入联机：Mod 作者声明"客机绘制要读哪些状态"和"客机 UI 要提交哪些操作"，联机层负责采集、传输、安装和上行。Host 上的玩法 Lua 照常运行，客机不重演玩法，只拿到结果。本文先给出接口一览（第 4 节），再说明握手、状态同步、客机回调和写法约定。

线格式与确认见[同步](synchronization.md)，客机的安装流程见 [Client 端设计](client.md)。模组适配的运行覆盖由[专用夹具](../tests/e2e-mods/isaac-cs-mod-fixture/README.md)统一验收。

## 1. 原则

- **Mod 作者声明，联机层提供基础设施**：不自动遍历 Mod 的 Lua 状态（状态的语义无法自动得出），也不为每个 Mod 写外部适配器。
- **对不上就拒绝连接**：两端的 Mod 集合、版本、声明和内容 ID 必须完全一致；启用了未声明的 Lua Mod 时拒绝连接。
- **只同步客机需要的状态**：只声明客机绘制和 HUD 会读的字段，玩法内部的计数器、令牌、种子不声明。
- **强类型**：值类型在声明时固定，同一个值总是编码成同一段字节，不提供照搬任意 Lua 表的动态类型。
- **一条代码路径**：同一份 Mod 代码在不联机、Host、客机三种情况下都能运行，差异由 SDK 吸收。

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

## 3. 支持级别

| 类别 | 判定 | 客机行为 |
| --- | --- | --- |
| 纯内容 Mod | 目录中没有 `main.lua` | 直接可用。物品、实体、动画、音效等原生内容由二进制同步覆盖 |
| 已声明 Mod | `main.lua` 调用了 `IsaacCS.Declare` | 按第 7 节放行回调，按声明同步状态和意图 |
| 未声明的 Lua Mod | 有 `main.lua`，但没有声明 | 拒绝连接，大厅显示该 Mod 的名字。要联机，就得移除该 Mod，或等它（或某个兼容 Mod）补上声明 |
| 兼容 Mod | 独立的 Mod，只调用公开 API，为另一个 Mod 补声明 | 等同于已声明。不得读取对方的上值或内部函数 |

本项目自带的界面 Mod、观测 Mod 和 REPENTOGON 自身不参与以上判定。

## 4. 接口一览

### 4.1 引入 SDK

本项目发布单文件 SDK [`isaaccs_sdk.lua`](../mod/sdk/isaaccs_sdk.lua)（发布包中位于 `sdk/isaaccs_sdk.lua`）。Mod 把它复制进自己的目录，在 `main.lua` 顶部引入：

```lua
local IsaacCS = include("isaaccs_sdk")
local mod = RegisterMod("示例 Mod", 1)
IsaacCS.Declare(mod, { version = "1.0.0" })
```

| 项 | 规则 |
| --- | --- |
| 选择实现 | 联机层在 REPENTOGON 的 `main_ex.lua` 加载时注入真实的 `IsaacCS`，早于任何 Mod 的 `main.lua`。SDK 发现真实实现时直接返回它；没有安装联机层时，安装不联机实现（4.6 节），赋给全局 `IsaacCS` 并返回 |
| 多份共存 | 多个 Mod 可能各带一份不同版本的 SDK。不联机实现只会被更高版本的不联机实现替换，替换时沿用已登记的声明；任何版本都不替换真实实现 |
| 版本不足 | Mod 带的 SDK 比已安装的联机层新时，真实实现上缺少的函数由 SDK 补上不联机版本，单机照常可玩；该 Mod 的 `Declare` 记为"api 版本不受支持"，联机时按未声明处理，大厅提示更新联机层 |
| 稳定性 | `IsaacCS.api` 只增不减。已发布函数的签名和不联机语义不再改动，新功能只以新增函数的形式加入 |
| 注解 | 每个函数带中文注释和 LuaCATS 注解，供 lua-language-server 补全和检查 |

### 4.2 总表

| 接口 | 调用时机 | 功能 |
| --- | --- | --- |
| `IsaacCS.api` | 任意 | 当前实现的 SDK 接口版本（整数） |
| `Declare(mod, info)` | 加载期 | 登记 Mod 自报版本和所需 api 版本，使 Mod 成为"已声明" |
| `State(mod, name, opts)` | 加载期 | 声明整局一份（global）或每参与者一份（player）的状态 |
| `EntityData(mod, filter, fields)` | 加载期 | 声明符合筛选条件的实体 `GetData()` 中要同步的键 |
| `EntityState(mod, name, opts)` | 加载期 | 声明以实体为参数、自己提供 get/set 的实体状态 |
| `Intent(mod, name, opts)` | 加载期 | 声明一个由 Host 执行的操作 |
| `Keys(mod, keys)` | 加载期 | 声明需要随输入上传的自定义键盘按键 |
| `MarkDirty(mod, key[, target])` | Host 运行时 | 通知 `rate = "mark"` 的声明已变化 |
| `SendIntent(mod, name, args)` | 任意端运行时 | 发起意图，返回调用序号 |
| `IsClient()` | 任意 | 本端是否是客机 |
| `GetParticipant(player)` | 任意 | 身体所属的参与者序号 |
| `GetPlayerByParticipant(ordinal)` | 任意 | 参与者的代表身体 |
| `GetLocalPlayer()` | 任意 | 本机参与者的代表身体 |
| `IsLocalPlayer(player)` | 任意 | 某个身体是否由本机控制 |
| `IsKeyPressed(player, key)` | Host 玩法回调 | 某参与者是否按住已声明的按键 |
| `IsKeyTriggered(player, key)` | Host 玩法回调 | 某参与者是否在本 tick 按下已声明的按键 |
| `AddCallback(mod, id, fn)` | 通常在加载期 | 注册联机层回调（4.5 节） |

除 `MarkDirty`、`SendIntent`、查询类函数外，所有声明只能在 Mod 加载期间调用（`main.lua` 顶层，或 `MC_POST_MODS_LOADED` 之前），之后调用会报错并记入日志。声明出错时整组声明都不登记，不会留下一半。

### 4.3 声明接口

#### `IsaacCS.Declare(mod, info)`

登记 Mod 的版本信息。`info` 是版本字符串，或 `{ version = "...", api = N }`；省略 `api` 时取当前 SDK 版本。每个 Mod 只能调用一次。版本字符串进入握手清单，两端必须相等。

```lua
IsaacCS.Declare(mod, "1.2.0")
IsaacCS.Declare(mod, { version = "1.2.0", api = 1 })
```

#### `IsaacCS.State(mod, name, opts)`

声明一项整局或每参与者的状态。

| 选项 | 含义 |
| --- | --- |
| `type` | 值类型（4.7 节），必填 |
| `scope` | `"global"`（默认，整局一份）或 `"player"`（每个参与者一份） |
| `private` | 仅 `player` 可用。`true` 时只发给该参与者本人，其他人的客机上读到默认值 |
| `rate` | `"world"`（默认，每个 World 帧读，约 30Hz）、`"low"`（约 10Hz）、`"mark"`（只在 `MarkDirty` 之后读） |
| `get` | Host 上读取值。省略时读 `mod[name]`。`player` 作用域收到 `player` 参数 |
| `set` | 客机上写入值。省略时按默认写回规则写 `mod[name]`（6.4 节） |
| `reset` | 客机离开场景、换局或会话结束时调用，用于清理 |

```lua
-- 全局：默认读写 mod.run，这里自己提供 get/set
IsaacCS.State(mod, "run", {
  scope = "global",
  type  = { struct = { phase = "int", timer = "float", boss = "entity" } },
  get   = function() return mod.run end,
  set   = function(v) mod.run = v end,       -- 仅在客机调用
  reset = function() mod.run = nil end,
})

-- 每个参与者一份；私有的只发给本人
IsaacCS.State(mod, "hand", {
  scope = "player", private = true,
  type  = { list = { of = "int", max = 16 } },
  get   = function(player) return mod.hands[IsaacCS.GetParticipant(player)] end,
  set   = function(player, v) mod.hands[IsaacCS.GetParticipant(player)] = v end,
})

-- 项数多、变化少的容器：只在修改后读
IsaacCS.State(mod, "counts", {
  type = { map = { key = "int", of = "int", max = 1024 } },
  rate = "mark",
})
```

#### `IsaacCS.EntityData(mod, filter, fields)`

同步符合筛选条件的实体 `GetData()` 中列出的键。`filter` 为 `{ type = ..., variant = ..., subtype = ... }`，`variant`、`subtype` 可省略。`fields` 是"键名 → 类型"表，每个键是一条独立的声明。Host 读 `GetData()[键]`，客机按默认写回规则写本地实体的 `GetData()[键]`。

```lua
IsaacCS.EntityData(mod, { type = EntityType.ENTITY_EFFECT, variant = MyEffect }, {
  myfx_charge = "int",
  myfx_phase  = "int",
})
```

`GetData()` 是所有 Mod 共用的表，键名应带上 Mod 前缀。

#### `IsaacCS.EntityState(mod, name, opts)`

实体级状态放在 Mod 自己的表里时使用。`filter`、`type`、`get(ent)`、`set(ent, v)` 必填，`rate`、`reset` 可选。`get` 和 `set` 拿到的都是本端的实体对象，`GetPtrHash` 在两端各自有效：Host 按成员号编码，客机按成员号找到本地实体再调用 `set`。

```lua
IsaacCS.EntityState(mod, "orbit", {
  filter = { type = EntityType.ENTITY_FAMILIAR, variant = MyFamiliar },
  type   = "vector",
  get    = function(ent) return mod.orbit[GetPtrHash(ent)] end,
  set    = function(ent, v) mod.orbit[GetPtrHash(ent)] = v end,
})
```

#### `IsaacCS.Intent(mod, name, opts)`

声明一个由 Host 执行的操作。两端都要执行这条声明。`args` 是参数类型，`handler(player, args)` 只在 Host 执行，返回 `ok[, result]`。规则见第 9 节。

```lua
IsaacCS.Intent(mod, "buy", {
  args    = { struct = { slot = "int" } },
  handler = function(player, args)
    local item = mod.shop[args.slot]
    if not item or player:GetNumCoins() < item.price then return false, "无法购买" end
    player:AddCoins(-item.price); player:AddCollectible(item.id)
    return true
  end,
})
```

#### `IsaacCS.Keys(mod, keys)`

声明 Mod 自定义的原始键盘按键，客机采样这些按键并随输入上传。`keys` 是非空、不重复的键值数组，键值范围 0–511（`Keyboard.*`）。整个会话合计最多 64 个不同键。

```lua
IsaacCS.Keys(mod, { Keyboard.KEY_F, Keyboard.KEY_G })
```

### 4.4 运行时接口

#### `IsaacCS.MarkDirty(mod, key[, target])`

只在 Host 上起作用，其他情况为空操作。通知 `rate = "mark"` 的声明已变化，下一个 World 帧读取一次。`player` 作用域传身体，实体作用域传实体，global 省略 `target`。记录还不存在时（会话开始、换局、身体或实体重新出现）联机层自动补读初值，不需要 Mod 标记。

```lua
mod.counts[id] = (mod.counts[id] or 0) + 1
IsaacCS.MarkDirty(mod, "counts")
```

#### `IsaacCS.SendIntent(mod, name, args)`

发起意图，返回调用序号；超出限额或意图未声明时返回 `nil`。Host 本机、客机、不联机时都用它，结果统一通过 `CLIENT_INTENT_RESULT` 回到发起端。

```lua
local seq = IsaacCS.SendIntent(mod, "buy", { slot = 3 })
if seq then mod.pending[seq] = true end   -- "处理中"放在未声明的变量里
```

#### `IsaacCS.IsClient()`

本端是客机时返回 `true`，Host 和不联机时返回 `false`。客机会触发已声明 Mod 的绘制回调，绘制回调里夹带的玩法代码要用它隔开。

```lua
mod:AddCallback(ModCallbacks.MC_POST_RENDER, function()
  drawPanel()
  if not IsaacCS.IsClient() then tickAnimationState() end
end)
```

#### `IsaacCS.GetParticipant(player)` / `IsaacCS.GetPlayerByParticipant(ordinal)`

参与者序号 0 是房主，1—3 是远端玩家。一个参与者可以有多个身体（如 Jacob 与 Esau），`GetParticipant` 对每个身体都返回所属参与者；`GetPlayerByParticipant` 返回代表身体，不存在时返回 `nil`。

```lua
local o = IsaacCS.GetParticipant(player)
local owner = IsaacCS.GetPlayerByParticipant(0)   -- 房主的身体
```

#### `IsaacCS.GetLocalPlayer()` / `IsaacCS.IsLocalPlayer(player)`

客机上 `Isaac.GetPlayer(0)` 是房主的角色，"画当前玩家"的 UI 要改用这两个接口。

```lua
local me = IsaacCS.GetLocalPlayer()
for i = 0, Game():GetNumPlayers() - 1 do
  local p = Isaac.GetPlayer(i)
  if IsaacCS.IsLocalPlayer(p) then drawOwnHud(p) end
end
```

#### `IsaacCS.IsKeyPressed(player, key)` / `IsaacCS.IsKeyTriggered(player, key)`

在 Host 的玩法回调里查询某参与者是否按住（或在本 tick 按下）已用 `Keys` 声明的按键。远端玩家的键盘不在 Host 上，`Input.IsButtonPressed(key, idx)` 查不到它们。客机不跑玩法，这两个函数在客机上恒为 `false`；客机 UI 读本机按键的做法见 8.3 节。

```lua
mod:AddCallback(ModCallbacks.MC_POST_PEFFECT_UPDATE, function(_, player)
  if IsaacCS.IsKeyTriggered(player, Keyboard.KEY_F) then toggleMenu(player) end
end)
```

`Input.IsActionPressed`、`IsActionTriggered`、`GetActionValue` 以远端玩家的 `ControllerIndex` 查询时，返回该参与者的统一输入，不需要改写法。

### 4.5 联机层回调

用 `IsaacCS.AddCallback(mod, IsaacCS.<回调名>, fn)` 注册，只在客机上触发（`CLIENT_INTENT_RESULT` 例外，三种情况都会触发）。

| 回调 | 参数 | 时机与用途 |
| --- | --- | --- |
| `CLIENT_SCENE_STARTED` | 无 | 第一份 World 及其 Mod 状态装好之后、首次显示之前。代替 `MC_POST_GAME_STARTED` |
| `CLIENT_SCENE_ENDED` | 无 | 换层、换局、退出时，在原生拆卸和 `reset` 之前，此时实体和 Lua 都还在。代替 `MC_PRE_GAME_EXIT` |
| `CLIENT_ENTITY_INSTALLED` | `ent` | 实体首次安装（含重建）后，该实体的 Mod 记录已经写完。用于纯显示的初始化，比如创建 Sprite。代替 `MC_POST_*_INIT` |
| `CLIENT_ENTITY_REMOVED` | `ent` | 计划内离开或重建的实体被移除之前，实体还在。与 INSTALLED 成对、每个实例只派发一次；场景结束时不逐个派发 |
| `CLIENT_STATE_CHANGED` | `key, target` | 本批记录全部写完后，只为内容确实变化的记录派发 |
| `CLIENT_INTENT_RESULT` | `name, seq, ok, result` | 本端发起的意图有了结果。不联机时延到下一帧派发，保证调用方先拿到序号 |

```lua
IsaacCS.AddCallback(mod, IsaacCS.CLIENT_ENTITY_INSTALLED, function(ent)
  if ent.Type == EntityType.ENTITY_EFFECT and ent.Variant == MyEffect then
    mod.overlay[GetPtrHash(ent)] = Sprite()   -- 本地显示对象，不同步
  end
end)

IsaacCS.AddCallback(mod, IsaacCS.CLIENT_STATE_CHANGED, function(key, target)
  if key == "run" then sfx:Play(SoundEffect.SOUND_BEEP) end   -- 对变化做出反应，不写在 set 里
end)

IsaacCS.AddCallback(mod, IsaacCS.CLIENT_INTENT_RESULT, function(name, seq, ok, result)
  mod.pending[seq] = nil
  if name == "buy" and not ok then mod.toast = result or "失败" end
end)
```

### 4.6 不联机时的行为

"不联机"包括没有安装联机层（SDK 的不联机实现）和安装了但没有开启会话（真实实现），两者行为一致：

| 接口 | 不联机时 |
| --- | --- |
| `Declare`、`State`、`EntityData`、`EntityState`、`Intent`、`Keys` | 照常登记并检查参数，不调用 `get`、`set`、`reset`，没有运行开销 |
| `MarkDirty` | 空操作 |
| `IsClient()` | `false` |
| `GetParticipant(player)` | `0`。本机的所有身体（包括本地同屏多人）都属于参与者 0 |
| `GetPlayerByParticipant(o)` | `o` 为 0 时返回 `Isaac.GetPlayer(0)`，否则返回 `nil` |
| `GetLocalPlayer()` | `Isaac.GetPlayer(0)` |
| `IsLocalPlayer(player)` | `true` |
| `SendIntent` | 立即执行 handler，传入 `Isaac.GetPlayer(0)`，返回序号；结果回调延到下一帧 |
| `IsKeyPressed`、`IsKeyTriggered` | 读本机键盘（`Input.IsButtonPressed(key, 0)`、`Input.IsButtonTriggered(key, 0)`），不区分 player |
| `AddCallback` | 照常登记。只有 `CLIENT_INTENT_RESULT` 会触发 |

联机时 Host 本机发起的意图，结果同样以 `CLIENT_INTENT_RESULT` 在本机回调。

### 4.7 值类型

| 类型 | 写法 | 说明 |
| --- | --- | --- |
| 布尔、整数、浮点 | `"bool"`、`"int"`（i32）、`"float"`（f32）、`"number"`（f64） | 定长 |
| 字符串 | `{ string = { max = N } }` | N ≤ 1024 字节 |
| 向量、颜色 | `"vector"`、`"color"`、`"kcolor"` | 对应 `Vector`、`Color`、`KColor` |
| 实体引用 | `"entity"` | 实体离开或不在成员目录里时为 `nil` |
| 玩家引用 | `"player"` | 按参与者对应身体 |
| 列表 | `{ list = { of = T, max = N } }` | 从 1 开始的连续整数键 |
| 字典 | `{ map = { key = "string" 或 "int", of = T, max = N, keymax = K } }` | `keymax` 只用于字符串键，是键的字节上限，默认 64 |
| 结构 | `{ struct = { 字段 = T, ... } }` | 至少一个字段；字段可以为 `nil` |

单个值编码后的最坏长度不超过 64 KiB。不允许函数、协程、userdata（实体和玩家引用除外）。

选择要点：

| 情况 | 用法 |
| --- | --- |
| 键是从 1 开始的连续整数 | `list`。Host 从 1 开始逐个取值，取到第一个 `nil` 为止 |
| 整数键有空洞，或者从 0 开始（如按道具 ID 计数、按槽位 0—3 存放） | `map{key="int"}`。用 `list` 会在第一个空洞处截断 |
| 键是种子、房间号等转成的字符串 | `map{key="string"}` |
| 字段有时为 `nil` | 放在 `struct` 里 |
| 同一字段时而整数、时而小数（如初值 `1`、加点后 `1.25`） | `number`。客机收到的 `int` 是整数子类型，`number` 是浮点子类型，Host 上的 `1` 到客机是 `1.0` |

`get` 返回的表里有 `struct` 没有声明的字段时直接忽略，不算类型不符；嵌套在 `list`、`map` 里的 `struct` 同样处理。联机层按声明的字段名逐个取值（走普通索引，带 `__index` 的对象也能用），不遍历表，所以 `get` 可以直接返回 Mod 的活表。未声明的字段不传输，客机上读到 `nil` 且不报错；诊断模式下，每个声明首次读取时把被忽略的字段名记进日志。

## 5. 握手

客机 Hello 之后、Ready 之前，双方经可靠控制通道交换 Mod 清单（`ModManifest`，消息 27）：

| 项 | 内容 |
| --- | --- |
| 启用的 Mod | 每项包括目录名、`metadata.xml` 的 name 和 version、是否有 `main.lua` |
| 声明描述 | 每个已声明 Mod 的 `api` 版本、自报版本，以及全部 State、EntityData、EntityState、Intent、Keys 声明的规范化文本 |
| ID 表 | Mod 新增内容的"名字 → ID"表：实体、角色、道具、饰品、药丸效果、卡牌、音乐、音效、挑战、Costume、过场、成就等 |

匹配规则：两端启用的 Mod 集合完全相同（与顺序无关），版本字符串相等，声明描述逐字相等，ID 表逐项相等，全部按内容直接比较。任何一项不等都拒绝连接，大厅显示首个差异。ID 不一致时直接拒绝，不做重映射。每方清单上限 60 KiB。

- 声明在首次握手时冻结。线上用"Mod 序号加声明序号"编号，不传名字。外层声明序号按调用顺序；`struct` 字段和同一次 `EntityData` 的键按字段名的字节序编号（Lua 字典没有可读取的书写顺序）。
- `luamod` 热重载会改变声明，会话进行中检测到重载时结束会话。
- 资源文件内容（anm2、png、xml）无法靠版本字符串核实，文件内容不一致导致的显示差异是已知风险。

## 6. 状态同步

### 6.1 作用域

| 作用域 | 含义 | 接收者 |
| --- | --- | --- |
| `global` | 整局只有一份 | 所有客机 |
| `player` | 每个参与者一份 | 默认所有客机；`private = true` 时只发给该参与者本人 |
| `entity` | 每个实体一份（`EntityData`、`EntityState`） | 所有客机 |

接收资格只随参与者加入、退休、重连变化，这几个时机本来就会发完整基线。

### 6.2 Host 读取

- **时机**：普通帧原生 `Game::Update` 返回后的 World 采集窗口，在主线程上调用 `get`、规范化并写入自有缓冲；比较和编码在后台线程完成（`state/mod_codec.cpp`）。
- **频率**：按声明的 `rate`。诊断模式下，含 `max` 超过 256 的 `list` 或 `map`、却按 `"world"` 读取的声明，会在加载期日志里点名，建议改用 `"mark"`。
- **上限**：每条记录只按声明里的字节和项数（`max`、`keymax`）设上限，不设读取时间上限。`get` 的耗时按 Mod、按声明计入诊断；写得慢会直接拖低 Host 帧率，联机层只报告、不兜底。
- **失败沿用**：`get` 抛错、返回值类型不符或超出上限时，该记录本帧沿用上一次的值并计数，不截断，不影响其他记录。整个 Mod 段失败时，所有 Mod 记录本帧沿用，原生帧照常发布。
- **顶层空值**：`get` 整体返回 `nil` 时，记录的字节数为 0，客机调用 `set(nil)`。从未读成功的记录不发送，客机不调用 `set`，Mod 读到的是自己的默认值。

### 6.3 传输

Mod 状态段和意图结果段附在 World 帧尾（帧头 `mod_bytes`、`mod_result_bytes`），记录格式见[同步](synchronization.md)。

- 变化以记录为单位判断，记录内部不做字段级增量：记录里任意一项变化都整条重发。
- 私有记录只进入接收者的帧。
- entity 作用域的记录与实体同生命周期：实体离开时一起删除。
- Mod 段计入 World 的带宽预算与内存账本。各 Mod 声明的内容不进布局版本，只在握手时比较。

### 6.4 客机安装

`ClientBinaryScene::InstallMods` 在 World 原生安装之后、显示推进和 Render 之前，使用同一份 World 的 Mod 影子写 Lua（[Client 端设计](client.md) 5.5 节）。绘制回调读到的 Mod 状态与实体属于同一帧。

- **写入**：global 和 player 作用域调用 `set`，没写 `set` 时按默认写回规则写 `mod[key]`；EntityData 按同一规则写本地实体的 `GetData()[key]`；EntityState 调用 `set(ent, v)`。每条记录单独 `pcall`，字节与上次成功写入的相同时不写。
- **完整基线**：全部记录重写一遍，用来纠正客机上被改动过的已声明键；内容没变的记录不派发 `CLIENT_STATE_CHANGED`。所以 `set` 可能以相同的值被多次调用，必须保证同值重写没有副作用。
- **失败**：单条记录失败只记日志和计数，保留上次成功的值，等下一次变化或基线；不影响 ACK，也不请求重发基线。
- **实体重建**：从影子状态取该实体最近一次的 Mod 值重新写入。
- **清理**：离开场景、换局或会话结束时，先派发 `CLIENT_SCENE_ENDED`，再逐个调用 `reset`；EntityData 随实体一起消失。参与者离开时只删它的 player 作用域记录，不调用 `set`；它的身体照常派发 `CLIENT_ENTITY_REMOVED`，Mod 在其中自己清理。

**默认写回**：没写 `set` 时，客机把值 `v` 写回 `mod[key]`（EntityData 写 `GetData()[key]`）：

| 情况 | 写法 |
| --- | --- |
| `v` 为 `nil` | 目标置 `nil` |
| 标量类型 | 直接赋值 |
| `struct`，且目标已经是表 | 原地合并：逐个写入声明过的字段，缺少的写 `nil`；未声明的字段和元表保留。字段本身是容器时递归处理 |
| `list`、`map`，且目标已经是表 | 原地清空再重填，表本身不换；元素直接替换 |
| 目标不是表 | 新建一张表，按上面的规则填入后赋值 |

各层表的引用、元表（类写法的对象）和客机本地字段都不会因同步而丢失。数据放在 Mod 自己的嵌套表里时，默认写回够不着，要自己写 `set`。

## 7. 客机回调放行

联机层在 REPENTOGON 的逐条回调调用点（`RunCallbackInternal`）之前接入，按 `callback.Mod` 和回调种类决定是否调用，保留官方的参数匹配、优先级和返回值规则。回调 ID 按原版与 REPENTOGON 1.1.3 的明确清单分级，未知 ID 默认拦截。项目自带 Mod 和 REPENTOGON 免清单声明，原生回调仍受阶段约束。

| 回调类别 | 已声明 Mod 在客机上 |
| --- | --- |
| 绘制和 HUD（`MC_POST_RENDER`、实体及小地图绘制、HUD 绘制、`MC_GET_SHADER_PARAMS`） | 在原生显示作用域触发。HUD 更新与历史栏重算通知只在本机 HUD 维护入口触发 |
| 显示查询（`MC_PRE_GET_LIGHTING_ALPHA`） | 触发，与绘制类相同 |
| 菜单和本地界面（`MC_MAIN_MENU_RENDER`、暂停菜单、ImGui 等） | 主菜单和角色选择绘制随稳定主菜单触发；暂停菜单绘制随本地暂停界面触发；`MC_MENU_INPUT_ACTION` 只随稳定主菜单的输入分派触发；ImGui 走自己的界面入口 |
| Init 类（`MC_POST_*_INIT` 等） | 不触发，改用 `CLIENT_ENTITY_INSTALLED` |
| 更新、伤害、碰撞、生成、Cache（含 `MC_EVALUATE_CUSTOM_CACHE`）、拾取、房间与楼层等玩法回调 | 不触发 |
| 玩家属性查询（`MC_PLAYER_GET_HEALTH_TYPE`、`MC_PLAYER_GET_HEART_LIMIT`、`MC_PLAYER_GET_ACTIVE_MAX_CHARGE`、`MC_PLAYER_GET_ACTIVE_MIN_USABLE_CHARGE`） | 不触发。客机原生 HUD 调这些查询时，返回 Host 同步来的有效结果；没有收到某组参数的结果时用原生默认值 |
| 商店和拾取物数值查询（`MC_GET_SHOP_ITEM_PRICE`、`MC_PICKUP_GET_COIN_VALUE`） | 不触发。商店标价使用已同步的 `EntityPickup.Price` |
| 音效和音乐（`MC_PRE_SFX_PLAY`、`MC_POST_SFX_PLAY`、`MC_PRE_MUSIC_PLAY`、`MC_PRE_MUSIC_PLAY_JINGLE`、`MC_PRE_MUSIC_LAYER_TOGGLE`） | 回放 Host 的音效和音乐时不触发；客机本地发出的声音照常触发 |

**音频回调**：Host 上 Mod 可以在 `MC_PRE_SFX_PLAY`、`MC_PRE_MUSIC_PLAY` 里换掉声音、改参数或取消播放，Host 采集的是回调处理后的最终参数，被取消的声音不记录。客机安装 Host 的音乐与循环声、回放 Host 的单次音效（以及原生音乐 tick 继续消费 Host 队列曲目）时，主线程持有回放标记，分派跳过这五个回调，避免同一个声音被改写两次。

**副作用**：客机上已声明 Mod 的绘制回调如果夹带了生成实体、改玩家或整局状态之类的玩法调用，目前不拦截，结果不确定（例如本地多出的实体可能与成员系统冲突），Mod 要用 `IsaacCS.IsClient()` 自己隔开。持久化和随机数已在客机隔离（[Client 端设计](client.md)第 8 节）。

## 8. 写法约定

### 8.1 复杂表的写法

Mod 常把状态放在一张多层的大表里，例如每个玩家一张表，各个功能在里面挂一张子表。不要整张声明成一条记录，而要：

1. **按子表拆成多条声明**。一条记录变化时只重发这一条，Mod 更新时改动范围也小。
2. **只声明客机绘制和 HUD 会读的字段**。
3. **每玩家一份的数据用 `player` 作用域**，不要把"玩家键 → 子表"整体声明成 `map`。Mod 自己的玩家键（如某个道具 RNG 的种子）两端不一定相同，`player` 作用域让两端各自用本地的键读写。
4. **`get` 每次重新取表**，不缓存表引用。Mod 可能在读档、回溯时把整张表换掉。
5. **项数多、变化少的容器单独声明，配 `rate = "mark"` 与 `MarkDirty`**。Lua 表没有修改标记，按 `"world"` 读取的声明每帧都要整条读完。

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

### 8.2 客机只读

已声明的键在客机上只由联机层写（`set`、默认写回、`reset`）。在客机上运行的代码（绘制和 HUD 回调、`CLIENT_*` 回调、本地 UI）都不改这些键；改了的话，联机层不纠正，要等 Host 的值变化或完整基线到来。诊断模式下按 Mod、按声明统计被改动的键并在日志里点名。

| 要避开的写法 | 例子 | 后果 | 改法 |
| --- | --- | --- | --- |
| 在绘制回调里推进计时器或计数 | `MC_POST_RENDER` 里 `data.timer = data.timer - 1` | 客机上的值越走越偏，Host 的值变了才拉回 | 计时放在 Host 的玩法回调里；客机要平滑显示，就放到本地未声明的变量里 |
| 消费一次性标志 | `if data.popup then 显示; data.popup = false end` | 客机清掉后，Host 一直是 `true`，不会重发；Host 之后再次置 `true`，客机也收不到变化 | Host 改用递增序号（`popupSeq`），客机记住上次见过的序号；或者用 `CLIENT_STATE_CHANGED` |
| 给已声明的键补默认值 | `mod.state = mod.state or {}` | Host 一直为 `nil` 时不会有新记录，客机补的值一直留着 | 读取时处理 `nil` |
| 原地改已声明的容器 | 为显示顺序对已声明的 `list` 调 `table.sort`、`table.insert` | 默认写回是原地合并，顺序被打乱后要等 Host 变化才纠正 | 先复制一份再排序 |
| 在 `CLIENT_ENTITY_INSTALLED` 里初始化已声明的字段 | 给 `GetData().hp` 赋初值 | 刚装好的同步值被覆盖 | 本地初始化只写未声明的字段 |
| 本地 UI 乐观更新 | 点了按钮就直接改已声明的金币数 | 和 Host 不一致，失败了无法回滚 | 发意图；"处理中"放在未声明的变量里 |
| 玩法和绘制共用一个会改状态的函数 | 同一个 helper 在 Update 和 Render 里都调用 | 客机通过绘制回调改了状态 | 用 `IsaacCS.IsClient()` 隔开 |
| `set` 带副作用 | 在 `set` 里播放音效、累加计数、创建 Sprite | 完整基线会用相同的值再调一次 `set`，副作用重复 | `set` 只赋值；要做出反应，放到 `CLIENT_STATE_CHANGED` |

另外：不跨帧缓存已声明的子表（写了 `set` 的记录每次安装都可能换新表），每帧重新取 `mod[key]` 或 `GetData()[key]`。可以做的：读已声明的键；在同一张表里写未声明的字段；写本地变量和副本；在 `CLIENT_SCENE_ENDED`、`reset` 里清理。

### 8.3 自定义 UI

- Mod 在客机本地用自己的 Sprite 和 Font 绘制，读取同步来的状态。UI 自己的状态（菜单是否打开、光标、选中项、动画帧）放在未声明的变量里，不同步。
- Mod 自己用 `Sprite()` 创建的 Sprite，`Update`、`Play`、`SetFrame` 等推进动画的调用必须写在绘制回调里，由已声明的状态决定播哪个动画；写在玩法回调里的，客机上不会执行，Sprite 会停住或播错。实体自带的 Sprite（`ent:GetSprite()`）由原生同步覆盖。
- "画当前玩家"的 UI 用 `GetLocalPlayer()`、`IsLocalPlayer()`。
- **读本机输入**：客机 UI 在绘制回调里直接用原生 `Input.*` 读本机输入（如 `Input.IsButtonTriggered(key, 0)`）。联机层只在 Host 和单机的权威运行时里改写 `Input.*` 的结果，客机上走原生函数。不要按身体的 `ControllerIndex` 查询：该值来自 Host，不一定对应本机设备。`IsKeyPressed`、`IsKeyTriggered` 是给 Host 玩法查询远端按键用的，客机上恒为 `false`；客机 UI 自己读的按键也不需要用 `Keys` 声明。读到操作后，要改游戏状态就发意图，"处理中"放在未声明的变量里，等 `CLIENT_INTENT_RESULT` 或状态变化。同一段代码在 Host 本机和不联机时也成立：Host 本机的意图进同一个队列，不联机时立即执行。

```lua
mod:AddCallback(ModCallbacks.MC_POST_RENDER, function()
  if not mod.menuOpen then return end                       -- UI 状态留在本地
  if Input.IsButtonTriggered(Keyboard.KEY_DOWN, 0) then mod.cursor = mod.cursor % 4 + 1 end
  if Input.IsButtonTriggered(Keyboard.KEY_ENTER, 0) and not mod.pending then
    mod.pending = IsaacCS.SendIntent(mod, "buy", { slot = mod.cursor })
  end
  drawShop(mod.cursor)
end)

IsaacCS.AddCallback(mod, IsaacCS.CLIENT_INTENT_RESULT, function(name, seq, ok, result)
  if seq == mod.pending then mod.pending = nil end
end)
```

- 简单的按键触发菜单也可以把状态留在 Host：Host 按 `ControllerIndex` 或扩展按键推进菜单，菜单状态声明为 `player` 作用域（可设为私有），客机只绘制。不用写意图，代价是光标每动一下都要等一个 RTT。
- 影响玩法的 Mod 设置用 `State` 加 `rate = "mark"` 同步，以 Host 为准；只影响显示的设置不声明，各端用本地的值。

## 9. 意图

| 项 | 规则 |
| --- | --- |
| 统一路径 | Host 本机、客机、不联机时都调用 `SendIntent`。Host 本机直接进入同一个队列；不联机时立即执行 handler |
| 上行 | `ModIntent`（消息 28），可靠有序。每个意图内容不超过 512 B，每个 Peer 在途不超过 8 个，每秒不超过 20 个；超出时 `SendIntent` 返回 `nil` |
| 执行 | Host 收到后排队，在下一次最外层 `Manager::Update` 入口按到达顺序执行，不在网络线程或 Lua 回调里直接执行 |
| 身份 | handler 收到的 `player` 是发起者当前的身体。发起者没有身体（死亡等待复活、退休中）时拒绝执行 |
| 校验 | handler 自己检查合法性。客机 UI 依据的是旧状态，选项可能已经失效 |
| 结果 | handler 的返回值作为发起者的私有结果随 World 返回，客机触发 `CLIENT_INTENT_RESULT`。handler 抛错视为失败 |
| 结果类型 | 布尔、整数、浮点、字符串、`nil` 和标量键的无环 Lua 表，编码后最多 256 B；无法编码时回传 `false, nil` |
| 实体参数 | 按提交时的成员目录编码，执行前由 Host 核对；实体已失效时返回失败 |
| 状态变化 | handler 造成的变化经原生同步和 Mod 状态同步回到客机 |
| 失效 | 换局、换层、会话结束时，未执行的意图全部丢弃并返回失败结果 |

## 10. 配置表与本机数据

**配置表只在加载期修改。** 下面这些接口改的是进程内的全局配置表，不属于任何同步对象：

| 接口 | 改的是 |
| --- | --- |
| `ItemConfig_Item:AddCustomTag`、`RemoveCustomTag`、`AddCustomCacheTag`、`RemoveCustomCacheTag` | 道具配置的自定义标签 |
| `ItemConfig_Card:SetAvailabilityCondition`、`ClearAvailabilityCondition`，`Weight` 字段 | 卡牌是否可出现的判定函数和权重 |
| `RoomConfigStage` 的各个 `Set*` | 楼层配置 |

在 `main.lua` 顶层或 `MC_POST_MODS_LOADED` 里调用时，两端执行同一份代码，结果一致。在玩法回调里调用时只有 Host 改了，客机读到旧值；握手查不出这种差异，联机层不同步、不检测。`Level:SetName`、`Isaac.SetCurrentFloorName`、`SetCurrentFloorMusic`、`SetCurrentFloorBackdrop` 本来就在局内调用，由原生同步处理，不受此限。

**本机数据。**

| 接口 | 客机上 |
| --- | --- |
| `Options` | 本机的值，属于本地设置 |
| `PersistentGameData` 的解锁、完成标记、统计 | 返回 Host 当前游戏所用存档的同步值，不读、不写客机本机存档。写入类接口（`Unlock`、`TryUnlock`、`IncreaseEventCounter`、`AddBestiaryKill`、`AddBossKilled`）按持久化拦截，不生效 |

## 11. 失败处理

| 失败 | 处理 |
| --- | --- |
| 两端 Mod 集合、版本、声明或 ID 表不同 | 握手拒绝，大厅显示首个差异 |
| 任一端启用了未声明的 Lua Mod | 握手拒绝，大厅显示该 Mod 的名字 |
| Mod 带的 SDK 比已安装的联机层新 | 单机可玩；联机时该 Mod 按未声明处理，大厅提示更新联机层 |
| 声明在加载期之后才调用，或会话中热重载 | 报错；会话中发生时结束会话 |
| `get` 抛错、类型不符或超限 | 本帧沿用并计数，日志报告该 Mod 和该声明 |
| `get` 很慢 | 不限时，耗时计入诊断，Host 帧率随之下降 |
| `list` 中间有空洞 | 读到第一个 `nil` 为止；应改用 `map{key="int"}` |
| 值里引用的实体已离开 | 编码为空引用并计数 |
| 客机 `set` 抛错 | 跳过该记录并计数，下一次变化时再写 |
| 客机代码修改了已声明的键 | 不纠正，等 Host 变化或完整基线；诊断模式下检测 |
| Mod 在玩法回调里推进自己创建的 Sprite | 客机 Sprite 停住或播错，由 Mod 改到绘制回调里 |
| Mod 在玩法回调里修改配置表 | 只有 Host 生效，不检测 |
| 客机绘制回调夹带玩法副作用 | 不拦截，结果不确定；由 Mod 用 `IsClient()` 隔开 |
| 意图过大、过多，或发起者没有身体 | 本地返回 `nil`，或由 Host 拒绝并返回失败结果 |
| 意图到达时状态已变 | handler 自行校验并返回失败 |

## 12. 代码位置

| 内容 | 位置 |
| --- | --- |
| SDK（不联机实现，同时是真实实现的 Lua 部分） | `mod/sdk/isaaccs_sdk.lua` |
| 真实 `IsaacCS` 注入、声明编译、回调分派、Host 读取、客机安装、意图执行 | `mod/src/game/mods/mod_api.cpp` |
| 原生显示保留段（Player 查询结果、楼层显示、进度镜像） | `mod/src/game/mods/mod_native.cpp` |
| Mod 记录的规范化、比较与客机校验 | `mod/src/state/mod_codec.cpp` |
| 握手清单 | `mod/src/net/mod_manifest.*` |
| 意图消息 | `mod/src/net/mod_intent.h` |
