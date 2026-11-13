-- E2E 原版内容的只读对照独立于 UI。无对照请求时不枚举世界。
local mod = RegisterMod("Isaac CS Observation", 1)
local calls=include("domains/common/calls")
local entity = include("domains/entity/collect")
IsaacCSObservation = entity

-- 生命周期回调只记录实际发生，不为记录制造 Spawn/Remove/随机选择。
local callbacks = {
  MC_POST_PLAYER_INIT="EntityPlayer", MC_POST_NPC_INIT="EntityNPC",
  MC_POST_TEAR_INIT="EntityTear", MC_POST_PROJECTILE_INIT="EntityProjectile",
  MC_POST_BOMB_INIT="EntityBomb", MC_POST_LASER_INIT="EntityLaser",
  MC_POST_KNIFE_INIT="EntityKnife", MC_POST_PICKUP_INIT="EntityPickup",
  MC_POST_FAMILIAR_INIT="EntityFamiliar", MC_POST_EFFECT_INIT="EntityEffect",
  MC_POST_SLOT_INIT="EntitySlot", MC_POST_ENTITY_REMOVE="Entity",
  MC_POST_SLOT_SET_PRIZE_COLLECTIBLE="EntitySlot", MC_POST_PICKUP_SELECTION="EntityPickup"
}
for name, class in pairs(callbacks) do
  local callback = ModCallbacks[name]
  if callback then
    mod:AddCallback(callback, function(_, object, ...)
      local native = IsaacCSObservationNative
      if native and native.Needed("Lifecycle") then entity.Event(name, class, object, ...) end
    end)
  end
end
for _,name in ipairs({"MC_POST_PLAYER_RENDER","MC_POST_NPC_RENDER","MC_POST_TEAR_RENDER","MC_POST_PROJECTILE_RENDER","MC_POST_BOMB_RENDER","MC_POST_LASER_RENDER","MC_POST_KNIFE_RENDER","MC_POST_PICKUP_RENDER","MC_POST_FAMILIAR_RENDER","MC_POST_EFFECT_RENDER","MC_POST_SLOT_RENDER"}) do
  if ModCallbacks[name] then mod:AddCallback(ModCallbacks[name],function(_,object)
    local native=IsaacCSObservationNative
    if native and native.Needed("Render") then entity.Render(object) end
  end) end
end

-- Grid 共用原生身份及唯一协调器，不从 Lua 回调另建身份表。
local grid=include("domains/grid/collect")
IsaacCSGridObservation=grid
for _,kind in ipairs({"DECORATION","DOOR","FIRE","GRAVITY","LOCK","PIT","POOP","PRESSUREPLATE","ROCK","SPIKES","STAIRCASE","STATUE","TELEPORTER","TNT","TRAPDOOR","WALL","WEB"}) do
  for _,phase in ipairs({"UPDATE","RENDER"}) do
    local name="MC_POST_GRID_ENTITY_"..kind.."_"..phase
    if ModCallbacks[name] then mod:AddCallback(ModCallbacks[name],function(_,object)
      local n=IsaacCSObservationNative
      if n and n.GridNeeded(phase=="RENDER" and "Render" or "Update") then
        local ok,err=pcall(grid.Event,name,object,phase=="RENDER")
        if not ok then grid.Error(name,err,phase=="RENDER") end
      end
    end) end
  end
end
for _,name in ipairs({"MC_POST_GRID_ENTITY_SPAWN","MC_POST_GRID_ROCK_DESTROY","MC_POST_GRID_HURT","MC_POST_GRID_HURT_DAMAGE"}) do
  if ModCallbacks[name] then mod:AddCallback(ModCallbacks[name],function(_,object,...)
    local n=IsaacCSObservationNative
    if n and n.GridNeeded("Lifecycle") then
      local ok,err=pcall(grid.Event,name,object,false,...)
      if not ok then grid.Error(name,err,false) end
    end
  end) end
end

-- 三个跨房域与Entity共用协调器；主线程回调不保留借用对象。
IsaacCSExplorationObservation=include("domains/exploration/collect")
IsaacCSRoomSaveObservation=include("domains/room_save/collect")
IsaacCSPlayerObservation=include("domains/player/collect")
for _,name in ipairs({"MC_POST_PLAYER_INIT","MC_POST_PLAYER_NEW_ROOM_TEMP_EFFECTS","MC_POST_PLAYER_NEW_LEVEL","MC_PRE_TRIGGER_PLAYER_DEATH","MC_TRIGGER_PLAYER_DEATH_POST_CHECK_REVIVES","MC_POST_PLAYER_REVIVE","MC_PLAYER_HEALTH_TYPE_CHANGE","MC_POST_PLAYER_ADD_CARD","MC_POST_PLAYER_ADD_PILL"}) do
  if ModCallbacks[name] then mod:AddCallback(ModCallbacks[name],function(_,p,...)
    local n=IsaacCSObservationNative
    if n and n.LocationNeeded() then IsaacCSPlayerObservation.Event(name,p,...) end
  end) end
end
if ModCallbacks.MC_POST_ADD_COLLECTIBLE then mod:AddCallback(ModCallbacks.MC_POST_ADD_COLLECTIBLE,function(_,id,charge,first,slot,var,p)
  local n=IsaacCSObservationNative
  if n and n.LocationNeeded() then IsaacCSPlayerObservation.Event("MC_POST_ADD_COLLECTIBLE",p,id,charge,first,slot,var) end
end) end

-- tx-04-14四域与既有来源共用提交窗口；调用wrapper常驻，停用时直接透传。
IsaacCSRunObservation=include("domains/run/collect")
IsaacCSRandomObservation=include("domains/random/collect")
IsaacCSPresentationObservation=include("domains/presentation/collect")
IsaacCSRandomObservation.Install()
IsaacCSPresentationObservation.Install()
for _,module in ipairs({IsaacCSRunObservation,IsaacCSRandomObservation}) do
  local capture=module.Capture
  module.Capture=function(...) return calls.Silent(capture,...) end
end
for _,name in ipairs({"MC_POST_GAME_STARTED","MC_PRE_GAME_EXIT","MC_POST_GAME_END","MC_POST_NEW_ROOM","MC_POST_NEW_LEVEL"}) do
  if ModCallbacks[name] then mod:AddCallback(ModCallbacks[name],function(_, ...)
    local n=IsaacCSObservationNative
    if n and n.LocationNeeded() then
      IsaacCSRunObservation.Event(name,...)
      IsaacCSRunObservation.Boundary(name)
    end
  end) end
end
for _,name in ipairs({"MC_POST_GET_COLLECTIBLE","MC_POST_GET_CARD","MC_POST_GET_PILL","MC_POST_GET_TRINKET","MC_POST_PICKUP_SELECTION"}) do
  if ModCallbacks[name] then mod:AddCallback(ModCallbacks[name],function(_, ...)
    local n=IsaacCSObservationNative
    if n and n.LocationNeeded() then IsaacCSRandomObservation.Event(name,...) end
  end) end
end
for _,name in ipairs({"MC_POST_SFX_PLAY","MC_PRE_MUSIC_PLAY","MC_PRE_MUSIC_PLAY_JINGLE","MC_PRE_MUSIC_LAYER_TOGGLE"}) do
  if ModCallbacks[name] then mod:AddCallback(ModCallbacks[name],function(_, ...)
    local n=IsaacCSObservationNative
    if n and n.LocationNeeded() then IsaacCSPresentationObservation.Event(name,...) end
  end) end
end
local presentationRender=0
mod:AddCallback(ModCallbacks.MC_POST_RENDER,function()
  local n=IsaacCSObservationNative
  if n and n.LocationNeeded() then
    presentationRender=presentationRender+1
    if presentationRender%30==0 then
      local ok,err=pcall(IsaacCSPresentationObservation.Render)
      if not ok then calls.failures.presentation_render=type(err)=="string" and err:sub(1,256) or "render_failed" end
    end
  end
end)
for _,name in ipairs({"MC_PRE_ROOM_RESTORE_STATE","MC_POST_ROOM_SAVE_STATE","MC_POST_BACKWARDS_ROOM_SAVE","MC_POST_BACKWARDS_ROOM_RESTORE","MC_POST_GLOWING_HOURGLASS_SAVE","MC_POST_GLOWING_HOURGLASS_LOAD"}) do
  if ModCallbacks[name] then mod:AddCallback(ModCallbacks[name],function(_,first,descriptor,key)
    local n=IsaacCSObservationNative
    if n and n.LocationNeeded() then IsaacCSRoomSaveObservation.Event(name,first,descriptor,key) end
  end) end
end

-- Room/Level仅在实际边界通知中撤销分层上下文；回调始终返回nil，不干预玩法选择。
local room=include("domains/room/collect")
IsaacCSRoomObservation=room
IsaacCSLevelObservation=include("domains/level/collect")
for _,name in ipairs({"MC_PRE_CHANGE_ROOM","MC_PRE_NEW_ROOM","MC_PRE_LEVEL_INIT","MC_POST_NEW_ROOM","MC_POST_NEW_LEVEL","MC_PRE_GAME_EXIT","MC_POST_GAME_END","MC_POST_SWAP_ROOMS"}) do
  if ModCallbacks[name] then mod:AddCallback(ModCallbacks[name],function()
    local n=IsaacCSObservationNative
    if n and n.LocationNeeded() then
      IsaacCSExplorationObservation.Boundary(name); IsaacCSRoomSaveObservation.Boundary(name); IsaacCSPlayerObservation.Boundary(name)
      n.LocationBoundary(name,name=="MC_PRE_LEVEL_INIT" or name=="MC_PRE_GAME_EXIT" or name=="MC_POST_GAME_END")
    end
  end) end
end
for _,name in ipairs({"MC_POST_ROOM_TRIGGER_CLEAR","MC_POST_START_AMBUSH_WAVE","MC_POST_START_GREED_WAVE","MC_PRE_SPAWN_CLEAN_AWARD","MC_POST_PLAYER_TRIGGER_ROOM_CLEAR"}) do
  if ModCallbacks[name] then mod:AddCallback(ModCallbacks[name],function(_, ...)
    local n=IsaacCSObservationNative
    if n and n.LocationNeeded() then
      local ok,err=pcall(room.Event,name,false,...)
      if not ok then room.Error(name,err) end
    end
  end) end
end
local roomRender=0
if ModCallbacks.MC_POST_ROOM_TRANSITION_RENDER then mod:AddCallback(ModCallbacks.MC_POST_ROOM_TRANSITION_RENDER,function()
  local n=IsaacCSObservationNative
  if n and n.LocationNeeded() then
    roomRender=roomRender+1
    if roomRender%30==0 then local ok,err=pcall(room.Event,"MC_POST_ROOM_TRANSITION_RENDER",true); if not ok then room.Error("MC_POST_ROOM_TRANSITION_RENDER",err) end end
  end
end) end

for _,name in ipairs({"MC_PRE_LEVEL_PLACE_ROOM","MC_PRE_LEVEL_SELECT","MC_POST_LEVEL_LAYOUT_GENERATED"}) do
  if ModCallbacks[name] then mod:AddCallback(ModCallbacks[name],function(_, ...)
    local n=IsaacCSObservationNative
    if n and n.LocationNeeded() then
      local ok,err=pcall(IsaacCSLevelObservation.Event,name,...)
      if not ok then room.Error(name,err) end
    end
  end) end
end
