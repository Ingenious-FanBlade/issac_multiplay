--[[
IsaacCS 联机层本体（isaaccs_api.lua）

只在 DLL 内使用，不对外分发。构建时由 CMake 编进 zhlIsaacCS.dll（mod_lua_sources.h）。
REPENTOGON 加载 main_ex.lua 时，mod_api.cpp 的 Install 执行本文件，并把原生函数表作为参数传入；
本文件返回 IsaacCS 表并发布为全局，此时还没有任何 Mod 的 main.lua 执行过。
Mod 随附的 mod/sdk/isaaccs_sdk.lua 只是薄壳：发现全局已有本实现就直接返回它，
所以装了联机层时，所有 Mod 实际使用的都是这里的函数。

职责
  1. 登记并严格检查各 Mod 的声明（只在加载期允许），把值类型编译成类型节点。
     DLL 在全部 Mod 加载完后读取登记表（_isaaccs.state.entries）生成握手清单和编码布局。
  2. 运行时查询经原生函数表回答。

三种运行情况（native.side() 的返回值）
  "offline"  装了联机层但没有开启联机会话（cs solo 单人会话也算）。行为须与 SDK 薄壳在
             没装联机层时一致（docs/mod-api.md 4.6 节），唯一的区别是这里会检查声明。
  "host"     房主。玩法照常运行，负责登记声明、接收 MarkDirty、执行意图。
  "client"   客机。不跑玩法，联机层按声明写入 Host 同步来的值，并派发 CLIENT_* 回调。

DLL 读取的登记表字段（改动时要同步修改 mod_api.cpp）：
  entries[mod].version / api / records / callbacks，以及每条声明记录的
  kind、name、scope、rate、private、filter、type、text、get、set、reset、handler、keys，
  类型节点的 kind、off、fixed、worst、max、keymax、mapKey、key、fields、element。
]]

-- 本实现支持的 SDK 接口版本（即 IsaacCS.api）。只增不减，与 mod_api.cpp 的版本判定保持一致。
local version = 1
--[[
原生函数表（带 _isaaccs_native 标记），由 DLL 传入。本文件用到的成员：
log、loading、side、participant、player、localOrdinal、markDirty、sendIntent、key。
]]
local native = ...
assert(type(native) == "table" and native._isaaccs_native, "isaaccs_api.lua 只能由联机层加载")

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
-- 日志输出，写进联机层日志。
local log = native.log
--[[
全部 Mod 共用的登记表。
  entries[mod]  每个 Mod 的登记项（字段见 entryFor）
  sequence      不联机时意图调用序号的计数器
]]
local state = { entries = {}, sequence = 0 }
---@class IsaacCS
---@field api integer 当前实现的 SDK 接口版本
local CS = {}
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

输入：一个值类型（标量名，或只含一个键的容器表）。
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
---@param definition any 类型定义
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
取得某个 Mod 的登记项，没有就新建；同时检查当前是否允许声明。
是否处于加载期由 DLL 判定（native.loading），它按原生 Mod 加载边界和 luamod 热重载状态回答；
Mod 卸载时由 isaaccs_dispatch.lua 删掉登记项。

登记项的字段：
  version, api    Declare 登记的自报版本和所需 SDK 接口版本
  records         全部声明，按调用顺序排列（这个顺序就是线上的声明序号）
  names           "声明身份 → 声明"，用于查重名（见 append）
  intents         "意图名 → handler"，不联机时 SendIntent 直接查这里
  callbacks       "回调名 → 函数数组"，AddCallback 登记的联机层回调
  results, deliverCallback  不联机时待派发的意图结果和派发用的回调（见 SendIntent）
]]
---@param mod table RegisterMod 返回的 Mod 对象
---@param name string 发起调用的接口名，用于错误信息
---@return table 登记项
local function entryFor(mod, name)
    check(type(mod) == "table" and type(mod.AddCallback) == "function", name .. " 需要 RegisterMod 返回的对象")
    check(native.loading(mod), name .. " 只能在 Mod 加载期调用")
    local entry = state.entries[mod]
    if not entry then
        entry = { records = {}, intents = {}, callbacks = {}, names = {} }
        state.entries[mod] = entry
    end
    return entry
end

--[[
声明本 Mod 支持联机，并登记版本。每个 Mod 只能调用一次，且只能在加载期调用。
info 可以只写版本字符串，也可以写成带 version 和 api 两个字段的表；省略 api 时取本实现的版本。
api 高于联机层支持的版本时，DLL 生成清单时按"需要更新联机层"拒绝连接。
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
---@param filter table 实体筛选条件
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
声明一项整局（global）或每参与者（player）的状态。选项含义见 SDK 薄壳的注释和 docs/mod-api.md 4.3 节。
联机时 Host 按 rate 调 get 并同步，客机在显示前调 set；不联机时只登记，不调用 get/set/reset。
]]
---@param mod table RegisterMod 返回的 Mod 对象
---@param name string 声明名称
---@param options table State 选项
function CS.State(mod, name, options)
    append(entryFor(mod, "State"), { record("State", name, options) })
end

--[[
声明符合筛选条件的实体 GetData() 中要同步的键。fields 是"键名 → 值类型"，每个键各成一条声明，
至少一项。
]]
---@param mod table RegisterMod 返回的 Mod 对象
---@param filter table 实体筛选条件
---@param fields table 键名 → 值类型
function CS.EntityData(mod, filter, fields)
    local entry, list = entryFor(mod, "EntityData"), {}
    local selector = entityFilter(filter)
    for _, name in ipairs(keys(fields)) do
        list[#list + 1] = record("EntityData", name, { type = rawget(fields, name) }, selector)
    end
    check(#list > 0, "EntityData 至少需要一个字段")
    append(entry, list)
end

-- 声明以实体为参数、自己提供 get/set 的实体状态。filter、type、get、set 必填。
---@param mod table RegisterMod 返回的 Mod 对象
---@param name string 声明名称
---@param options table EntityState 选项
function CS.EntityState(mod, name, options)
    local entry = entryFor(mod, "EntityState")
    check(type(options) == "table" and type(options.get) == "function" and type(options.set) == "function",
        "EntityState 需要 get 和 set")
    append(entry, { record("EntityState", name, options, entityFilter(options.filter)) })
end

-- 声明由 Host 执行的意图。handler 必填；不联机时 SendIntent 从 intents 里找它。
---@param mod table RegisterMod 返回的 Mod 对象
---@param name string 意图名称
---@param options table Intent 选项（args、handler）
function CS.Intent(mod, name, options)
    local entry = entryFor(mod, "Intent")
    local item = record("Intent", name, options)
    check(item.handler, "Intent 需要 handler")
    append(entry, { item })
    entry.intents[name] = item.handler
end

--[[
声明随输入上传的扩展按键。每个 Mod 只能调用一次（声明名固定为 "keys"，重复会报重名）。
按数值排序后登记，书写顺序不影响握手比较；全部 Mod 合计不超过 64 个由 DLL 检查。
]]
---@param mod table RegisterMod 返回的 Mod 对象
---@param buttons integer[] 键值数组，0–511
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

-- 通知 rate 为 "mark" 的声明已变化。只在 Host 上交给 DLL，其余情况什么也不做。
---@param mod table RegisterMod 返回的 Mod 对象
---@param key string 声明名称
---@param target any 目标身体或实体，global 省略
function CS.MarkDirty(mod, key, target)
    if native.side() ~= "host" then return end
    native.markDirty(mod, key, target)
end

-- 本端是否是客机。
---@return boolean
function CS.IsClient()
    return native.side() == "client"
end

-- 身体所属的参与者序号；不联机时所有身体都归 0。
---@param player any EntityPlayer
---@return integer?
function CS.GetParticipant(player)
    if native.side() ~= "offline" then return native.participant(player) end
    return 0
end

-- 参与者的代表身体；不联机时只有序号 0 有身体。
---@param ordinal integer 参与者序号
---@return any EntityPlayer 或 nil
function CS.GetPlayerByParticipant(ordinal)
    if native.side() ~= "offline" then return native.player(ordinal) end
    if ordinal == 0 then return Isaac.GetPlayer(0) end
end

-- 本机参与者的代表身体；不联机时为 Isaac.GetPlayer(0)。
---@return any EntityPlayer 或 nil
function CS.GetLocalPlayer()
    if native.side() ~= "offline" then return native.player(native.localOrdinal()) end
    return Isaac.GetPlayer(0)
end

-- 某个身体是否由本机控制；不联机时总是 true。
---@param player any EntityPlayer
---@return boolean
function CS.IsLocalPlayer(player)
    if native.side() == "offline" then return true end
    local owner = native.participant(player)
    return owner ~= nil and owner == native.localOrdinal()
end

--[[
登记联机层回调。客机上由 DLL 按登记表派发；不联机时只有 CLIENT_INTENT_RESULT 会由 deliver 派发。
加载期之后也能调用，前提是该 Mod 已经登记过（如已 Declare）。
]]
---@param mod table RegisterMod 返回的 Mod 对象
---@param id string 回调名
---@param fn function 回调函数
function CS.AddCallback(mod, id, fn)
    check(order[id] and type(fn) == "function", "无效的 IsaacCS 回调")
    local entry = state.entries[mod] or entryFor(mod, "AddCallback")
    local list = entry.callbacks[id] or {}
    list[#list + 1] = fn
    entry.callbacks[id] = list
end

--[[
不联机时派发某个 Mod 待交付的意图结果，挂在 MC_POST_RENDER 上。
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
发起意图，返回调用序号；超限或意图未声明时返回 nil。
联机时交给 DLL 编码后排队或上传；不联机时立即以 Isaac.GetPlayer(0) 执行 handler，结果延到下一帧派发。
]]
---@param mod table RegisterMod 返回的 Mod 对象
---@param name string 意图名称
---@param args any 参数
---@return integer? 调用序号
function CS.SendIntent(mod, name, args)
    if native.side() ~= "offline" then
        return native.sendIntent(mod, name, args)
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
扩展按键查询（第三个参数为 true 时查边沿）。native.key 在 Host 上返回该参与者本 tick 的按键状态，
客机上恒为 false；不联机（含 solo）时返回 nil，回落到本机键盘。
]]
---@param player any EntityPlayer
---@param key integer 键值
---@return boolean
function CS.IsKeyPressed(player, key)
    local value = native.key(player, key, false)
    if value ~= nil then return value end
    return Input.IsButtonPressed(key, 0)
end

-- 同 IsKeyPressed，查本 tick 是否刚按下。
---@param player any EntityPlayer
---@param key integer 键值
---@return boolean
function CS.IsKeyTriggered(player, key)
    local value = native.key(player, key, true)
    if value ~= nil then return value end
    return Input.IsButtonTriggered(key, 0)
end

--[[
标记为联机层的实现（native 为 true）并发布为全局：SDK 薄壳靠 _isaaccs.native 识别本实现，
DLL 和 isaaccs_dispatch.lua 通过 _isaaccs.state 读写登记表。
]]
CS._isaaccs = { native = true, state = state }
rawset(_G, "IsaacCS", CS)
return CS
