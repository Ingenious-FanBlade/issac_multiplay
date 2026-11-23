--[[
IsaacCS Mod SDK（isaaccs_sdk.lua）

这是什么
  第三方 Mod 接入"以撒联机"用的单文件 SDK。Mod 把它复制进自己的目录，在 main.lua 顶部用
  include 引入本文件，取得 IsaacCS 表，再对 RegisterMod 返回的 Mod 对象调用
  IsaacCS.Declare 登记版本。之后用 IsaacCS.State / EntityData / EntityState 声明
  "客机绘制要读的状态"，用 IsaacCS.Intent 声明"客机 UI 要让 Host 执行的操作"。
  完整说明见 docs/mod-api.md。

同一份文件的两种用法
  1. 联机层（zhlIsaacCS.dll）在 REPENTOGON 加载 main_ex.lua 时执行本文件，并把原生函数表
     作为参数（下面的 injected）传进来。这时得到的是"真实实现"：声明交给 DLL 编译和比对，
     查询函数经原生函数表回答。
  2. Mod 用 include 引入时没有原生函数表。如果全局已经有真实实现，直接返回它；
     没有安装联机层时，安装"不联机实现"：声明照常登记和检查，查询函数按单机语义回答，
     意图在本机立即执行。这样 Mod 不用关心联机层装没装。

三种运行情况（native.side() 的返回值）
  "offline"  没装联机层，或装了但没有开启会话。行为以 docs/mod-api.md 4.6 节为准。
  "host"     房主。玩法照常运行，SDK 负责登记声明、接收 MarkDirty、执行意图。
  "client"   客机。不跑玩法，联机层按声明写入 Host 同步来的值，并派发 CLIENT_* 回调。

本文件只登记和检查声明；Host 读取、编码、传输、客机写入都在 DLL 里完成。
]]

-- 以下 LuaCATS 注解只供 lua-language-server 补全和检查，不影响运行。

--[[
标量类型名。括号里是线上定长字节数：bool(1) int(4，i32) float(4，f32) number(8，f64)
vector(8) color(28) kcolor(16) entity(2，实体成员号) player(1，参与者序号)。
]]
---@alias IsaacCSScalar "bool"|"int"|"float"|"number"|"vector"|"color"|"kcolor"|"entity"|"player"
--[[
值类型：标量名，或只含一个键的表，键名是容器种类：
  string  字符串，max 为字节上限（不超过 1024）
  list    从 1 开始的连续数组，of 为元素类型，max 为项数上限
  map     字典，key 为 "string" 或 "int"，of 为值类型，max 为项数上限，
          keymax 为字符串键的字节上限（默认 64）
  struct  固定字段的表，值为"字段名 → 类型"，字段可以为 nil
]]
---@alias IsaacCSType IsaacCSScalar|{string:{max:integer}}|{list:{of:IsaacCSType,max:integer}}|{map:{key:"string"|"int",of:IsaacCSType,max:integer,keymax:integer?}}|{struct:table<string,IsaacCSType>}
-- 实体筛选条件。只有 type 必填；variant、subtype 省略表示任意。
---@alias IsaacCSFilter {type:integer,variant:integer?,subtype:integer?}
-- IsaacCS.State 的选项。scope 默认 "global"，rate 默认 "world"；private 只能用于 "player"。
---@alias IsaacCSState {type:IsaacCSType,scope:("global"|"player")?,private:boolean?,rate:("world"|"low"|"mark")?,get:function?,set:function?,reset:function?}
-- IsaacCS.EntityState 的选项。get(ent) 和 set(ent, v) 必填。
---@alias IsaacCSEntityState {type:IsaacCSType,filter:IsaacCSFilter,rate:("world"|"low"|"mark")?,get:function,set:function,reset:function?}
-- IsaacCS.Intent 的选项。handler(player, args) 只在 Host 执行，返回 ok[, result]。
---@alias IsaacCSIntent {args:IsaacCSType,handler:fun(player:EntityPlayer,args:any):boolean,any}
-- IsaacCS.AddCallback 可用的联机层回调名，也可写成 IsaacCS.CLIENT_xxx。
---@alias IsaacCSCallback "CLIENT_SCENE_STARTED"|"CLIENT_SCENE_ENDED"|"CLIENT_ENTITY_INSTALLED"|"CLIENT_ENTITY_REMOVED"|"CLIENT_STATE_CHANGED"|"CLIENT_INTENT_RESULT"

-- 本 SDK 的接口版本（即 IsaacCS.api）。只增不减；新增函数时加 1，已发布函数的签名和单机语义不变。
local version = 1
-- 执行本文件时传入的参数。DLL 执行时是原生函数表；Mod 用 include 引入时不是表。
local injected = ...
--[[
原生函数表（带 _isaaccs_native 标记）。只有 DLL 执行本文件时才有值，有值即"真实实现"。
本文件用到的成员：log、loading、side、participant、player、localOrdinal、markDirty、sendIntent、key。
]]
local native = type(injected) == "table" and injected._isaaccs_native and injected or nil
-- 全局里已经存在的 IsaacCS（DLL 先装的真实实现，或别的 Mod 先引入的某个版本的 SDK）。
local existing = rawget(_G, "IsaacCS")
-- 已存在实现的内部信息（native = 是否真实实现，state = 登记表）；不是本 SDK 装的则为 nil/false。
local previous = type(existing) == "table" and existing._isaaccs
--[[
作为 Mod 引入时，全局已有同版本或更新的实现，就直接用它，不再安装。
（全局是旧版真实实现的情况在文件末尾处理：补上新函数，但不替换它。）
]]
if not native and previous and existing.api >= version then return existing end

-- 联机层回调名集合，用于检查 AddCallback 的参数；同时把这些名字作为常量挂到 IsaacCS 上。
local order = {
    CLIENT_SCENE_STARTED = true,
    CLIENT_SCENE_ENDED = true,
    CLIENT_ENTITY_INSTALLED = true,
    CLIENT_ENTITY_REMOVED = true,
    CLIENT_STATE_CHANGED = true,
    CLIENT_INTENT_RESULT = true
}
-- 标量类型名 → 线上编码的定长字节数。compile 用它检查类型名并计算布局。
local scalar = { bool = 1, int = 4, float = 4, number = 8, vector = 8, color = 28, kcolor = 16, entity = 2, player = 1 }
-- 单个值编码后最坏长度的上限：64 KiB。
local limit = 65536
-- 日志输出。真实实现写进联机层日志，不联机时写游戏的 log.txt。
local log = native and native.log or Isaac.DebugString
--[[
全部 Mod 共用的登记表。替换旧版不联机实现时沿用它，已登记的声明不会丢。
  entries[mod]  每个 Mod 的登记项（字段见 entryFor）
  closed        不联机实现用：Mod 加载期是否已经结束
  sequence      不联机实现用：意图调用序号的计数器
]]
local state = previous and previous.state or { entries = {}, closed = false, sequence = 0 }
--[[
返回给 Mod 的 IsaacCS 表。替换旧版不联机实现时原地改写旧表，
这样先引入旧版的 Mod 手里的引用也能用到新函数；其余情况新建一张表。
]]
---@class IsaacCS
---@field api integer 当前实现的 SDK 接口版本
local CS = existing and previous and not previous.native and existing or {}
CS.api = version
for name in pairs(order) do CS[name] = name end

--[[
检查失败时报错：先写日志，再抛出带 "IsaacCS:" 前缀的错误。
错误位置指向调用 IsaacCS.xxx 的 Mod 代码（error 的第二个参数 3），方便作者定位。
声明函数都先做完全部检查再登记，所以一次失败的声明不会留下半截条目。
]]
---@param condition any 为真时什么也不做
---@param message string 错误说明
local function check(condition, message)
    if condition then return end
    log("IsaacCS: " .. message)
    error("IsaacCS: " .. message, 3)
end

--[[
按字节比较两个字符串（a 排在 b 前面时返回 true）。
不用 Lua 的 < 运算，因为它按区域设置比较，两台机器的结果可能不同；字节序两端一定一致。
]]
---@param a string
---@param b string
---@return boolean
local function byteLess(a, b)
    for i = 1, math.min(#a, #b) do
        local left, right = a:byte(i), b:byte(i)
        if left ~= right then return left < right end
    end
    return #a < #b
end

--[[
取出一张"字段名 → 定义"表的全部字段名，检查后按字节序排好返回。
Lua 表字面量读不出书写顺序（pairs 的顺序两端还可能不同），所以 struct 的字段和
EntityData 的键一律按字节序编号，两端得到同一个顺序。
]]
---@param value table 字段定义表，例如 struct 的字段表
---@return string[] 排好序的字段名
local function keys(value)
    check(type(value) == "table", "字段声明必须是表")
    local result = {}
    for key in next, value do
        check(type(key) == "string" and #key > 0 and not key:find("%z"), "字段名必须是非空字符串")
        result[#result + 1] = key
    end
    table.sort(result, byteLess)
    return result
end

--[[
把 Mod 写的值类型编译成"类型节点"，同时检查是否合法。

输入：一个值类型（标量名，或容器表）。
输出：类型节点表，字段如下（DLL 的 Compile 直接读这些字段生成编码布局）：
  kind     类型名："int"、"struct"、"list" 等
  fixed    定长部分的字节数。标量查 scalar 表；string/list/map 为 2（u16 长度或项数）；
           struct 为存在位图字节数（每字段 1 位）加各字段的 fixed
  worst    编码后最坏情况下的总字节数，不得超过 64 KiB
  text     规范化描述文本，进入握手清单逐字比较
  max      string/list/map 的上限（字节数或项数）
  element  list/map 的元素类型节点
  mapKey   map 的键类型（"string" 或 "int"）；keymax 为字符串键的字节上限（默认 64）
  fields   struct 的字段节点数组，按字段名字节序排列；每个字段节点另带
           key（字段名）和 off（在定长区里的偏移）

active 记录当前正在编译的类型表：同一张表在自身内部再次出现（自引用）时报错，
保证类型树有限。
]]
---@param definition IsaacCSType 类型定义
---@param active table 递归中正在编译的类型表集合，外部调用时传 {}
---@return table 类型节点
local function compile(definition, active)
    -- 标量：直接查表。
    if type(definition) == "string" then
        check(scalar[definition], "未知的值类型 " .. definition)
        return { kind = definition, fixed = scalar[definition], worst = scalar[definition], text = definition }
    end
    -- 容器：必须是只有一个键的表，键名是容器种类，值是容器选项。
    check(type(definition) == "table" and not active[definition], "类型必须为有限类型树")
    local kind, options = next(definition)
    check(kind and type(options) == "table" and next(definition, kind) == nil, "类型只能声明一种容器")
    active[definition] = true
    local node = { kind = kind, fixed = 2 }
    if kind == "struct" then
        -- struct：开头是存在位图，之后按字段名字节序依次排布各字段的定长部分。
        local fields = keys(options)
        check(#fields > 0, "struct 至少需要一个字段")
        node.fields = {}
        node.fixed = (#fields + 7) // 8
        node.worst = node.fixed
        local text = { "struct{" }
        for _, key in ipairs(fields) do
            local field = compile(rawget(options, key), active)
            field.key, field.off = key, node.fixed
            node.fields[#node.fields + 1] = field
            node.fixed, node.worst = node.fixed + field.fixed, node.worst + field.worst
            check(node.worst <= limit, "值最坏长度超过 64 KiB")
            -- 字段名前带长度，避免不同的字段名拼接后得到相同的文本。
            text[#text + 1] = ("%d:%s=%s;"):format(#key, key, field.text)
        end
        text[#text + 1] = "}"
        node.text = table.concat(text)
    else
        -- string / list / map：先检查 max（string 不超过 1024 字节，list/map 不超过 65535 项）。
        check(kind == "string" or kind == "list" or kind == "map", "未知容器类型")
        local max = rawget(options, "max")
        check(math.type(max) and max % 1 == 0 and max >= 0 and max <= (kind == "string" and 1024 or 65535),
            "容器 max 超出范围")
        node.max = math.tointeger(max)
        node.worst = node.fixed + node.max
        node.text = ("%s(%d)"):format(kind, node.max)
        if kind ~= "string" then
            -- list / map：编译元素类型；item 是每一项的最坏字节数。
            node.element = compile(rawget(options, "of"), active)
            local item = node.element.worst
            if kind == "map" then
                -- map 每项还要存键：字符串键为 2 字节长度加至多 keymax 字节，整数键为 4 字节。
                local key = rawget(options, "key")
                local keymax = rawget(options, "keymax")
                check(key == "string" or key == "int", "map 键必须为 string 或 int")
                if key == "string" then
                    if keymax == nil then keymax = 64 end
                    check(math.type(keymax) and keymax % 1 == 0 and keymax >= 0 and keymax <= 65535, "map keymax 超出范围")
                    node.keymax = math.tointeger(keymax)
                    item = item + 2 + node.keymax
                else
                    check(keymax == nil, "int 键不能设置 keymax")
                    item = item + 4
                end
                node.mapKey = key
                node.text = node.text .. key .. tostring(node.keymax or 0) .. ":"
            end
            node.worst = node.fixed + node.max * item
            node.text = node.text .. node.element.text
        end
    end
    check(node.worst <= limit, "值最坏长度超过 64 KiB")
    active[definition] = nil
    return node
end

--[[
【仅不联机实现】结束 Mod 加载期：此后再调用声明函数会报错。
由 entryFor 挂在 MC_POST_MODS_LOADED 上（加载期已过才登记的 Mod 挂在 MC_POST_RENDER 上），
只触发一次，触发后把自己撤下。真实实现的加载期由 DLL 判定，不走这里。
]]
---@param mod table 回调所属的 Mod（游戏调用回调时传入）
local function close(mod)
    local entry = state.entries[mod]
    if not entry then return end
    entry.loaded = true
    state.closed = true
    mod:RemoveCallback(entry.closeCallback, close)
end

--[[
【仅不联机实现】Mod 被卸载（如 luamod 重载）时删除它的登记项，连同未派发的意图结果。
挂在 MC_PRE_MOD_UNLOAD 上；任何 Mod 卸载时每个 Mod 都会收到该回调，所以只在卸载的正是自己时才删除。
]]
---@param owner table 注册此回调的 Mod
---@param unloaded table 正在卸载的 Mod
local function forget(owner, unloaded)
    if owner == unloaded then state.entries[owner] = nil end
end

--[[
取得某个 Mod 的登记项，没有就新建；同时检查当前是否允许声明（只在 Mod 加载期允许）。

登记项的字段：
  version, api    Declare 登记的自报版本和所需 SDK 接口版本
  records         全部声明，按调用顺序排列（这个顺序就是线上的声明序号）
  names           "声明身份 → 声明"，用于查重名（见 append）
  intents         "意图名 → handler"，不联机时 SendIntent 直接查这里
  callbacks       "回调名 → 函数数组"，AddCallback 登记的联机层回调
  dirty           MarkDirty 的兜底登记（见 MarkDirty）
  loaded          【不联机】该 Mod 的加载期是否已结束
  closeCallback   【不联机】挂 close 用的回调种类
  results, deliverCallback  【不联机】待派发的意图结果和派发用的回调（见 SendIntent）

是否处于加载期：
  真实实现问 DLL（native.loading），由 DLL 按原生 Mod 加载边界和热重载状态判断。
  不联机实现：全部 Mod 加载完之前都允许；之后只有 luamod 热重载新建的 Mod 对象，
  且调用栈里确实有正在执行的 main.lua，才允许重新声明。
]]
---@param mod table RegisterMod 返回的 Mod 对象
---@param name string 发起调用的接口名，用于错误信息
---@return table 登记项
local function entryFor(mod, name)
    check(type(mod) == "table" and type(mod.AddCallback) == "function", name .. " 需要 RegisterMod 返回的对象")
    local entry = state.entries[mod]
    local open = native and native.loading(mod)
    if not native then
        open = not state.closed and not (entry and entry.loaded)
        if not open and not (entry and entry.loaded) and debug and debug.getinfo then
            --[[
            加载期已过，但这是一个还没结束加载的 Mod 对象（luamod 热重载会新建 Mod 对象）：
            沿调用栈向上找，确认调用来自某个 main.lua 的顶层执行，才允许声明。
            ]]
            local level = 3
            while true do
                local info = debug.getinfo(level, "S")
                if not info then break end
                if info.what == "main" and info.source:match("[/\\]main%.lua$") then
                    open = true; break
                end
                level = level + 1
            end
        end
    end
    check(open, name .. " 只能在 Mod 加载期调用")
    if not entry then
        entry = { records = {}, intents = {}, callbacks = {}, dirty = {}, names = {} }
        state.entries[mod] = entry
        if not native then
            -- 不联机时自己挂回调来结束加载期、处理卸载；真实实现由 DLL 负责。
            entry.closeCallback = (not state.closed and ModCallbacks.MC_POST_MODS_LOADED) or ModCallbacks.MC_POST_RENDER
            mod:AddCallback(entry.closeCallback, close)
            if ModCallbacks.MC_PRE_MOD_UNLOAD then mod:AddCallback(ModCallbacks.MC_PRE_MOD_UNLOAD, forget) end
        end
    end
    return entry
end

--[[
声明本 Mod 支持联机，并登记版本。每个 Mod 只能调用一次，且只能在加载期调用。
调用过 Declare 的 Mod 才算"已声明"；启用了有 main.lua 却没有声明的 Mod，联机时拒绝连接。
info 可以只写版本字符串，也可以写成带 version 和 api 两个字段的表。
version 进入握手清单，两端必须相等。api 是 Mod 需要的 SDK 接口版本，省略时取本 SDK 的版本；
已安装的联机层低于这个版本时，该 Mod 按未声明处理。
不联机时：照常登记和检查。
]]
---@param mod table RegisterMod 返回的 Mod 对象
---@param info string|{version:string,api:integer?} 版本字符串，或带 version、api 字段的表
function CS.Declare(mod, info)
    local entry = entryFor(mod, "Declare")
    local release = type(info) == "table" and info.version or info
    local api = type(info) == "table" and info.api or version
    check(type(release) == "string" and #release > 0 and not release:find("%z"), "Declare 需要 version")
    check(math.type(api) == "integer" and api > 0 and api <= 65535, "无效的 SDK api")
    check(not entry.version, "重复 Declare")
    entry.version, entry.api = release, api
end

--[[
检查实体筛选条件并复制成新表：type 必填，variant、subtype 可省略，都必须是非负 32 位整数。
复制一份，是为了声明之后 Mod 再改原表也不影响已登记的声明。
]]
---@param filter IsaacCSFilter
---@return table 复制出的筛选条件
local function entityFilter(filter)
    check(type(filter) == "table", "需要实体 filter")
    local result = {}
    for _, key in ipairs({ "type", "variant", "subtype" }) do
        local n = rawget(filter, key)
        check((n == nil and key ~= "type") or (math.type(n) == "integer" and n >= 0 and n <= 2147483647),
            "无效 filter." .. key)
        result[key] = n
    end
    return result
end

--[[
检查一条声明的名称和选项，编译它的值类型，生成登记用的声明记录。

生成的 text 是这条声明的完整规范化描述，进入握手清单逐字比较。依次由以下部分用空格连接：
种类、"名称长度:名称"、作用域、频率、public 或 private、实体筛选、类型文本。
实体筛选写成"type/variant/subtype"，省略项写"*"；非实体声明为空串。
]]
---@param kind string 声明种类："State"、"EntityData"、"EntityState" 或 "Intent"
---@param name string 声明名称（EntityData 为 GetData 的键名）
---@param options table 声明选项；Intent 用 args 作为值类型，其余用 type
---@param filter table? 实体筛选（已经过 entityFilter）；有值即为 entity 作用域
---@return table 声明记录
local function record(kind, name, options, filter)
    check(type(name) == "string" and #name > 0 and not name:find("%z"), "无效声明名称")
    check(type(options) == "table", "声明选项必须是表")
    local scope = filter and "entity" or options.scope or "global"
    check(scope == "global" or scope == "player" or (scope == "entity" and filter), "无效 scope")
    local private = options.private
    check(private == nil or (type(private) == "boolean" and (not private or scope == "player")), "private 只适用于 player")
    local rate = options.rate or "world"
    check(rate == "world" or rate == "low" or rate == "mark", "无效 rate")
    for _, f in ipairs({ "get", "set", "reset", "handler" }) do
        check(options[f] == nil or type(options[f]) == "function", f .. " 必须是函数")
    end
    local descriptor = compile(kind == "Intent" and options.args or options.type, {})
    local selector = filter and ("%s/%s/%s"):format(filter.type, filter.variant or "*", filter.subtype or "*") or ""
    local text = ("%s %d:%s %s %s %s %s %s"):format(kind, #name, name, scope, rate, private and "private" or "public",
        selector, descriptor.text)
    return {
        kind = kind,
        name = name,
        scope = scope,
        rate = rate,
        private = private == true,
        filter = filter,
        type = descriptor,
        text = text,
        get = options.get,
        set = options.set,
        reset = options.reset,
        handler = options.handler
    }
end

--[[
把一组声明记录追加到 Mod 的登记项。先检查整组都不重名，再一起追加，
保证一次调用要么全部登记、要么一条都不登记。追加顺序决定线上的声明序号，之后不再变。
重名按"种类 + 名称"判断；EntityData 还要加上筛选条件，因为同一个键可以分别为不同实体声明。
]]
---@param entry table Mod 的登记项
---@param records table[] 待追加的声明记录
local function append(entry, records)
    for _, item in ipairs(records) do
        local key = ("%s:%d:%s"):format(item.kind, #item.name, item.name)
        if item.kind == "EntityData" then
            local f = item.filter
            key = key .. ":" .. f.type .. "/" .. (f.variant or "*") .. "/" .. (f.subtype or "*")
        end
        check(not entry.names[key], "重复声明 " .. item.name)
        item.identity = key
    end
    for _, item in ipairs(records) do
        entry.names[item.identity] = item
        entry.records[#entry.records + 1] = item
    end
end

--[[
声明一项要同步给客机的状态：整局一份（scope 为 "global"，默认）或每个参与者一份（scope 为 "player"）。
只能在加载期调用。选项：
  type     值类型，必填
  private  仅 player 可用。为 true 时只发给该参与者本人，其他客机上读到的是默认值
  rate     Host 读取频率："world"（默认，约 30Hz）、"low"（约 10Hz）、"mark"（只在 MarkDirty 后读）
  get      Host 上读值：global 不带参数，player 收到该参与者的身体。省略时读 mod[name]
  set      客机上写值：global 收到新值，player 收到身体和新值。省略时按"默认写回"写 mod[name]
           （表会原地合并，保留未声明的字段和元表）。完整基线会以相同的值再次调用，不要带副作用
  reset    客机离开场景、换局或会话结束时调用，用于清理
联机时：Host 按 rate 调 get 并同步，客机在显示前调 set。不联机时：只登记，不调用 get/set/reset。
]]
---@param mod table RegisterMod 返回的 Mod 对象
---@param name string 声明名称，同一 Mod 内不能重复；也是默认读写 mod[name] 用的键
---@param options IsaacCSState
function CS.State(mod, name, options)
    append(entryFor(mod, "State"), { record("State", name, options) })
end

--[[
声明：符合筛选条件的实体，把 GetData() 里列出的键同步给客机。只能在加载期调用。
fields 的每个键各成一条声明。Host 读实体 GetData() 中的该键，客机按"默认写回"写本地实体
GetData() 中的同一个键。GetData() 是所有 Mod 共用的表，键名建议带 Mod 前缀。
不联机时：只登记。
]]
---@param mod table RegisterMod 返回的 Mod 对象
---@param filter IsaacCSFilter 实体筛选条件
---@param fields table<string,IsaacCSType> 键名 → 值类型，至少一项
function CS.EntityData(mod, filter, fields)
    local entry, list = entryFor(mod, "EntityData"), {}
    local selector = entityFilter(filter)
    for _, name in ipairs(keys(fields)) do
        list[#list + 1] = record("EntityData", name, { type = rawget(fields, name) }, selector)
    end
    check(#list > 0, "EntityData 至少需要一个字段")
    append(entry, list)
end

--[[
声明一项实体级状态，数据放在 Mod 自己的表里时使用（放在 GetData() 里的用 EntityData）。
只能在加载期调用。filter、type、get、set 必填；rate、reset 可选。
get 收到实体，set 收到实体和新值，拿到的都是本端的实体对象，可以用 GetPtrHash 的结果做键：
两端的实体由联机层按成员号对应，Mod 不需要自己换键。
不联机时：只登记。
]]
---@param mod table RegisterMod 返回的 Mod 对象
---@param name string 声明名称，同一 Mod 内不能重复
---@param options IsaacCSEntityState
function CS.EntityState(mod, name, options)
    local entry = entryFor(mod, "EntityState")
    check(type(options) == "table" and type(options.get) == "function" and type(options.set) == "function",
        "EntityState 需要 get 和 set")
    append(entry, { record("EntityState", name, options, entityFilter(options.filter)) })
end

--[[
声明一个"意图"：客机 UI 想改变游戏状态时，交给 Host 执行的操作。只能在加载期调用，两端都要声明。
  args     参数的值类型
  handler  只在 Host 执行（不联机时在本机执行），收到发起者当前的身体和参数。
           自己检查操作是否合法，返回 ok 和可选的结果；抛错视为失败
发起用 IsaacCS.SendIntent，结果通过 CLIENT_INTENT_RESULT 回到发起端。
]]
---@param mod table RegisterMod 返回的 Mod 对象
---@param name string 意图名称，同一 Mod 内不能重复
---@param options IsaacCSIntent
function CS.Intent(mod, name, options)
    local entry = entryFor(mod, "Intent")
    local item = record("Intent", name, options)
    check(item.handler, "Intent 需要 handler")
    append(entry, { item })
    entry.intents[name] = item.handler
end

--[[
声明 Mod 自定义的键盘按键，联机时客机会采样这些按键并随输入上传给 Host，
Host 的玩法代码用 IsKeyPressed / IsKeyTriggered 查询。客机 UI 自己读的本机按键不需要声明。
只能在加载期调用，每个 Mod 只能调用一次。buttons 是非空、连续、不重复的数组，键值 0–511
（Keyboard.KEY_*）；整个会话合计最多 64 个不同键。
按键按数值排序后登记，所以书写顺序不影响握手比较。
]]
---@param mod table RegisterMod 返回的 Mod 对象
---@param buttons integer[] 键值数组
function CS.Keys(mod, buttons)
    local entry = entryFor(mod, "Keys")
    check(type(buttons) == "table", "Keys 需要按键数组")
    local count, seen, list = 0, {}, {}
    for index, key in next, buttons do
        check(math.type(index) == "integer" and index >= 1 and index <= #buttons and
            math.type(key) == "integer" and key >= 0 and key <= 511 and not seen[key], "无效或重复的按键")
        count, seen[key] = count + 1, true
        list[count] = key
    end
    check(count == #buttons and count > 0, "Keys 需要连续非空数组")
    table.sort(list)
    append(entry, { { kind = "Keys", name = "keys", text = "Keys " .. table.concat(list, ","), keys = list } })
end

--[[
通知联机层：rate 为 "mark" 的某项声明已经变化，下一个 World 帧读取一次。
只在 Host 上起作用；客机和不联机时什么也不做，所以可以直接写在玩法代码里。
target：player 作用域传身体，实体作用域传实体，global 省略。实体作用域省略 target 表示全部实体。
记录还不存在时（会话开始、换局、身体或实体重新出现）联机层会自动读初值，不需要 Mod 标记。
]]
---@param mod table RegisterMod 返回的 Mod 对象
---@param key string 声明名称
---@param target Entity? 目标身体或实体
function CS.MarkDirty(mod, key, target)
    if not native or native.side() ~= "host" then return end
    if native.markDirty and native.markDirty(mod, key, target) then return end
    --[[
    DLL 没有受理（名称不是 mark 频率的声明、实体无法识别等）时，只在登记项里记一笔。
    目前没有代码读取这份记录。
    ]]
    local entry = state.entries[mod]
    if not entry then return end
    local dirty = entry.dirty[key] or {}
    dirty[target or false] = true
    entry.dirty[key] = dirty
end

--[[
本端是否是客机。Host 和不联机时返回 false。
客机会触发已声明 Mod 的绘制回调；绘制回调里夹带的玩法代码要用它隔开。
]]
---@return boolean
function CS.IsClient()
    return native ~= nil and native.side() == "client"
end

--[[
返回某个身体所属的参与者序号：0 是房主，1—3 是远端玩家。
一个参与者可以有多个身体（如 Jacob 与 Esau），每个身体都返回同一个序号。
联机时查不到归属返回 nil；不联机时本机所有身体（包括本地同屏多人）都返回 0。
]]
---@param player EntityPlayer
---@return integer? 参与者序号
function CS.GetParticipant(player)
    if native and native.side() ~= "offline" then return native.participant(player) end
    return 0
end

--[[
返回某个参与者的代表身体，不存在时返回 nil。是 GetParticipant 的反查。
不联机时只有序号 0 有身体，即 Isaac.GetPlayer(0)。
]]
---@param ordinal integer 参与者序号
---@return EntityPlayer?
function CS.GetPlayerByParticipant(ordinal)
    if native and native.side() ~= "offline" then return native.player(ordinal) end
    if ordinal == 0 then return Isaac.GetPlayer(0) end
end

--[[
返回本机参与者的代表身体。客机上 Isaac.GetPlayer(0) 是房主的角色，
"画当前玩家"的 UI 要用这个函数。不联机时返回 Isaac.GetPlayer(0)。
]]
---@return EntityPlayer?
function CS.GetLocalPlayer()
    if native and native.side() ~= "offline" then return native.player(native.localOrdinal()) end
    return Isaac.GetPlayer(0)
end

-- 判断某个身体是否由本机控制，用于在多个玩家中挑出本机的那个。不联机时总是返回 true。
---@param player EntityPlayer
---@return boolean
function CS.IsLocalPlayer(player)
    if not native or native.side() == "offline" then return true end
    local owner = native.participant(player)
    return owner ~= nil and owner == native.localOrdinal()
end

--[[
注册联机层回调。id 为 IsaacCS.CLIENT_xxx：
  CLIENT_SCENE_STARTED     客机场景开始：第一份状态已装好、首次显示之前。无参数
  CLIENT_SCENE_ENDED       客机场景结束（换层、换局、退出），在 reset 之前。无参数
  CLIENT_ENTITY_INSTALLED  客机实体装好之后（含重建），已能读到同步数据；代替 Init 类回调。参数为实体
  CLIENT_ENTITY_REMOVED    客机实体移除之前，实体还在。参数为实体
  CLIENT_STATE_CHANGED     客机本批状态写完后，对内容有变化的记录派发。参数为声明名称和目标
  CLIENT_INTENT_RESULT     本端发起的意图有了结果（Host、客机、不联机都会触发）。
                           参数为意图名称、调用序号、是否成功、结果
不联机时只有 CLIENT_INTENT_RESULT 会触发。加载期之后也能调用，前提是该 Mod 已经登记过（如已 Declare）。
]]
---@param mod table RegisterMod 返回的 Mod 对象
---@param id IsaacCSCallback 回调名
---@param fn function 回调函数
function CS.AddCallback(mod, id, fn)
    check(order[id] and type(fn) == "function", "无效的 IsaacCS 回调")
    local entry = state.entries[mod] or entryFor(mod, "AddCallback")
    local list = entry.callbacks[id] or {}
    list[#list + 1] = fn
    entry.callbacks[id] = list
end

--[[
【仅不联机实现】派发某个 Mod 待交付的意图结果，挂在 MC_POST_RENDER 上。
只派发"产生于更早帧"的结果；本帧产生的（包括在结果回调里又发起的意图）留到下一帧，
保证 SendIntent 的调用方总是先拿到序号、再收到结果。全部派发完后撤下回调。
]]
---@param mod table 回调所属的 Mod
local function deliver(mod)
    local entry = state.entries[mod]
    if not entry or not entry.results then return end
    local now, pending = Isaac.GetFrameCount(), entry.results
    entry.results = {}
    for _, result in ipairs(pending) do
        if result.frame == now then
            entry.results[#entry.results + 1] = result
        else
            for _, callback in ipairs(entry.callbacks.CLIENT_INTENT_RESULT or {}) do
                local ok, err = pcall(callback, result.name, result.seq, result.ok, result.value)
                if not ok then log("IsaacCS CLIENT_INTENT_RESULT: " .. tostring(err)) end
            end
        end
    end
    if #entry.results == 0 then
        entry.results = nil
        local callback = entry.deliverCallback
        entry.deliverCallback = nil
        mod:RemoveCallback(ModCallbacks.MC_POST_RENDER, callback)
    end
end

--[[
发起意图，返回调用序号；结果之后通过 CLIENT_INTENT_RESULT 回到本端，按序号对应。
Host、客机、不联机三种情况都用同一种写法：
  联机：交给联机层排队，Host 在下一次 Manager::Update 入口执行 handler。
        单个意图内容不超过 512 B，每端在途不超过 8 个、每秒不超过 20 个，超出返回 nil。
  不联机：立即以 Isaac.GetPlayer(0) 执行 handler，结果延到下一帧派发。
意图未声明时返回 nil。客机不要提前改已声明的状态，"处理中"放在未声明的变量里。
]]
---@param mod table RegisterMod 返回的 Mod 对象
---@param name string 意图名称
---@param args any 参数，须符合 Intent 声明的 args 类型
---@return integer? 调用序号；失败时为 nil
function CS.SendIntent(mod, name, args)
    if native and native.side() ~= "offline" then
        return native.sendIntent and native.sendIntent(mod, name, args)
    end
    local entry = state.entries[mod]
    local handler = entry and entry.intents[name]
    if not handler then return nil end
    state.sequence = state.sequence + 1
    local sequence = state.sequence
    local called, ok, value = pcall(handler, Isaac.GetPlayer(0), args)
    if not called then
        log("IsaacCS intent " .. name .. ": " .. tostring(ok))
        ok, value = false, nil
    end
    if not entry.results then
        entry.results = {}
        --[[
        REPENTOGON 的 RemoveCallback 只按函数删除、不分 Mod。每个 Mod 登记自己的闭包，
        交付完毕时只撤下本 Mod 的那一条，不连带删掉其他 Mod 仍在等待的交付。
        ]]
        entry.deliverCallback = function(owner) deliver(owner) end
        mod:AddCallback(ModCallbacks.MC_POST_RENDER, entry.deliverCallback)
    end
    entry.results[#entry.results + 1] = { frame = Isaac.GetFrameCount(), name = name, seq = sequence, ok = ok == true, value =
    value }
    return sequence
end

--[[
查询某个玩家是否按住了用 Keys 声明的按键。供 Host 的玩法回调使用。
  联机 Host：返回该身体所属参与者本 tick 的按键状态（远端玩家的按键来自客机上传）；
             未声明的键返回 false。
  客机：总是 false（客机不跑玩法）。
  不联机：读本机键盘 Input.IsButtonPressed(key, 0)，不区分 player。
客机 UI 要读本机按键时不用这个函数：在绘制回调里直接用原生 Input.*（键盘查控制器 0）读本机输入，
需要改游戏状态时再用 SendIntent 提交。不要按身体的 ControllerIndex 查询：该值来自 Host，
不一定对应本机设备。
]]
---@param player EntityPlayer
---@param key integer 键值（Keyboard.KEY_*）
---@return boolean
function CS.IsKeyPressed(player, key)
    local value = native and native.key(player, key, false)
    if value ~= nil then return value end
    return Input.IsButtonPressed(key, 0)
end

--[[
查询某个玩家是否在本 tick 刚按下用 Keys 声明的按键（按下的那一刻为 true）。
各情况的行为同 IsKeyPressed，客机上同样总是 false；不联机时读 Input.IsButtonTriggered(key, 0)。
客机 UI 读本机按键的做法见 IsKeyPressed 的说明。
]]
---@param player EntityPlayer
---@param key integer 键值（Keyboard.KEY_*）
---@return boolean
function CS.IsKeyTriggered(player, key)
    local value = native and native.key(player, key, true)
    if value ~= nil then return value end
    return Input.IsButtonTriggered(key, 0)
end

if previous and previous.native and not native then
    --[[
    全局已经是真实实现，但它比本 SDK 旧（同版本或更新的已在文件开头直接返回）。
    不替换真实实现，只把它缺少的函数补成本 SDK 的不联机版本，单机照常可玩。
    ]]
    for name, fn in pairs(CS) do
        if existing[name] == nil then existing[name] = fn end
    end
    --[[
    返回给本 Mod 的是一层代理：Declare 至少要求本 SDK 的接口版本，
    联机层不支持时该 Mod 记为"api 版本不受支持"，联机时按未声明处理；其余函数沿用真实实现。
    ]]
    ---@param mod table RegisterMod 返回的 Mod 对象
    ---@param info string|{version:string,api:integer?} 同 IsaacCS.Declare
    local function declareNewer(mod, info)
        local release = type(info) == "table" and info.version or info
        local required = type(info) == "table" and info.api or version
        existing.Declare(mod, { version = release, api = math.max(version, required) })
    end
    return setmetatable({ Declare = declareNewer, api = version }, { __index = existing })
end

--[[
记下本实现的内部信息，供之后引入的 SDK 判断"全局是谁装的、能否替换"并沿用登记表；
然后发布为全局 IsaacCS。
]]
CS._isaaccs = { native = native ~= nil, state = state }
rawset(_G, "IsaacCS", CS)
return CS
