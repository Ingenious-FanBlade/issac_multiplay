-- 正式只读来源独立于 UI 和 E2E 夹具。停用时只检查原生需求，不枚举世界。
local mod = RegisterMod("Isaac CS Observation", 1)
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
      if n and n.GridNeeded(phase=="RENDER" and "Render" or "Lifecycle") then
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

-- Room/Level仅在实际边界通知中撤销分层上下文；回调始终返回nil，不干预玩法选择。
local room=include("domains/room/collect")
IsaacCSRoomObservation=room
IsaacCSLevelObservation=include("domains/level/collect")
for _,name in ipairs({"MC_PRE_CHANGE_ROOM","MC_PRE_NEW_ROOM","MC_PRE_LEVEL_INIT","MC_POST_NEW_ROOM","MC_POST_NEW_LEVEL","MC_PRE_GAME_EXIT","MC_POST_GAME_END","MC_POST_SWAP_ROOMS"}) do
  if ModCallbacks[name] then mod:AddCallback(ModCallbacks[name],function()
    local n=IsaacCSObservationNative
    if n and n.LocationNeeded() then n.LocationBoundary(name,name=="MC_PRE_LEVEL_INIT" or name=="MC_PRE_GAME_EXIT" or name=="MC_POST_GAME_END") end
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
