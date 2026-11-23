--[[
IsaacCS 联机层内部脚本：回调放行、加载与卸载通知、Input 路由、内容 ID 表读取。

只在 DLL 内使用，不对外分发。构建时由 CMake 编进 zhlIsaacCS.dll（mod_lua_sources.h），
mod_api.cpp 的 Install 在 isaaccs_api.lua 执行之后加载本文件，传入三个参数：
  native  原生函数表（与 isaaccs_api.lua 收到的是同一张）
  api     isaaccs_api.lua 返回的 IsaacCS 表
  debug   临时的调试库。正式 Mod 虚拟机没有全局 debug，只传给本脚本，不登记到全局
本文件最后返回内容 ID 表的读取函数，DLL 把它存进 registry，握手生成 Mod 清单时调用。
]]
local native, api, debug = ...
local seen, targets, logic = {}, {}, nil
--[[
从 REPENTOGON 的回调入口出发，沿上值递归查找：
  RunCallbackInternal     逐条执行回调的内部函数，记下它所在的闭包和上值序号，稍后替换
  CustomRunCallbackLogic  按回调种类分派的处理表，记下它，并继续扫描其中的处理函数
seen 防止重复扫描同一个函数；跳过 xpcall，它是全局库函数，没有要找的上值。
]]
local function scan(fn)
  if type(fn) ~= "function" or seen[fn] then return end
  seen[fn] = true
  for i = 1, math.huge do
    local name, value = debug.getupvalue(fn, i)
    if not name then break end
    if name == "RunCallbackInternal" then
      local id = debug.upvalueid(fn, i)
      targets[id] = {fn, i, value}
    elseif name == "CustomRunCallbackLogic" then
      logic = value
      for _, handler in pairs(value) do scan(handler) end
    elseif type(value) == "function" and name ~= "xpcall" then
      scan(value)
    end
  end
end
-- REPENTOGON 缓存到 registry 的入口、公开迭代器和各类专用入口最终都走 RunCallbackInternal。
scan(_RunCallback)
scan(_RunCallbackWithTwoParams)
scan(Isaac.RunCallbackOverIterator)
scan(_RunPreRenderCallback)
scan(_RunAdditiveCallback)
scan(_RunEntityTakeDmgCallback)
scan(_RunPostPickupSelectionCallback)
scan(_RunTriggerPlayerDeathCallback)
assert(next(targets) and logic and logic[ModCallbacks.MC_POST_MODS_LOADED], "REPENTOGON callback dispatch not found")
--[[
回调放行：把 RunCallbackInternal 换成一层包装，先问 native.allowed 这条回调（回调 ID 加注册它的 Mod）
能否在本端执行，允许才调用原函数。只替换上值，回调的注册接口、优先级、参数筛选、
回调链和返回值检查都保持 REPENTOGON 原样。
]]
for _, target in pairs(targets) do
  local original = target[3]
  debug.setupvalue(target[1], target[2], function(id, entry, ...)
    if native.allowed(id, entry.Mod) then return original(id, entry, ...) end
  end)
end
-- 全部 Mod 加载完时先通知 DLL（结束加载期、编译全部声明），再执行原处理。
local loaded = logic[ModCallbacks.MC_POST_MODS_LOADED]
logic[ModCallbacks.MC_POST_MODS_LOADED] = function(...)
  native.modsLoaded()
  return loaded(...)
end
-- Mod 卸载（如 luamod 重载）时通知 DLL，并删掉 IsaacCS 登记表里该 Mod 的条目。
local unload = _UnloadMod
_UnloadMod = function(mod)
  native.unloaded(mod)
  api._isaaccs.state.entries[mod] = nil
  return unload(mod)
end

--[[
Lua 的公开动作接口没有原生玩家消费者作用域，给虚拟控制器补上对应路由：
远端玩家（虚拟设备 K）的查询由 native.action 返回该参与者的统一输入；
native.action 返回 nil 时（本机控制器、客机、不联机）走原函数。
]]
for kind, name in ipairs({"IsActionPressed","IsActionTriggered","GetActionValue"}) do
  local original = Input[name]
  Input[name] = function(action, controller, ...)
    local value = native.action(action, controller, kind)
    if value ~= nil then return value end
    return original(action, controller, ...)
  end
end

--[[
内容 ID 表：XMLData 给出实际分配后的条目，以来源和名称区分各 Mod 的同名内容。
categories 是要比较的内容类别及其 XMLData 类别号。
]]
local getCount, getEntry = XMLData.GetNumEntries, XMLData.GetEntryByOrder
local categories = {{"实体",1},{"角色",2},{"道具",3},{"饰品",4},{"药丸效果",5},
  {"卡牌",6},{"音乐",7},{"音效",8},{"挑战",9},{"Costume",12},{"Null Costume",13},{"过场",23},{"成就",26}}
-- 把条目里的 ID 字段转成整数；没有有效 ID 时报错，握手按失败处理。
local function id(value)
  local number = math.tointeger(tonumber(value))
  assert(number ~= nil, "XMLData 没有有效内容 ID")
  return number
end
--[[
返回给 DLL 的读取函数。sources 为"XMLData 来源 ID → 清单里的 Mod 名"，值为 false 的来源不计入。
跳过原版内容，每个条目输出成 { "类别 Mod名/内容名", ID 文本 }；实体的 ID 文本为 type/variant/subtype。
]]
return function(sources)
  local result = {}
  for _, category in ipairs(categories) do
    local label, categoryId = category[1], category[2]
    for i = 1, getCount(categoryId) do
      local row = getEntry(categoryId, i)
      if row and row.sourceid and row.sourceid ~= "BaseGame" then
        local owner = sources[row.sourceid]
        if owner ~= false then
          local value
          if categoryId == 1 then
            value = ("%d/%d/%d"):format(id(row.type or row.id), tonumber(row.variant) or 0, tonumber(row.subtype) or 0)
          else
            value = tostring(id(row.id))
          end
          local name
          for _, key in ipairs({"untranslatedname","name","anm2path","anm2","path","relativeid"}) do
            if row[key] and row[key] ~= "" then name = row[key]; break end
          end
          result[#result+1] = {label .. " " .. (owner or row.sourceid) .. "/" .. (name or value), value}
        end
      end
    end
  end
  return result
end
