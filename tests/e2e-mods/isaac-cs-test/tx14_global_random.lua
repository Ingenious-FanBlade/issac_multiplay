-- 全局Random专项：真实游戏动作与只读观察分离；不替换Random，不写MT内部状态。
local mod=RegisterMod("Isaac CS tx14 native global RNG E2E",1)
local config=include("harness_config");local json=require("json")
-- 模块加载时缓存，验证不依赖后续Lua wrapper替换。
local cachedRandom=Random
local action,serial,render=0,0,0
local running,pending=false,nil
local actions={"seed","readonly","lua_draws","native_twists","zero_seed","damocles_spawn","damocles_damage","record_off","observe_off","observe_restart"}
local function emit(event,data)
  serial=serial+1;data=data or {};data.event=event;data.action=action;data.name=actions[action];data.token=config.token;data.serial=serial
  local encoded=json.encode(data)
  -- 游戏DebugString单行有限长；完整状态分块运输，不能把截断JSON当作观测失败。
  if #encoded>4000 then
    local total=math.ceil(#encoded/4000)
    for part=1,total do
      Isaac.DebugString("[isaac-cs-test] tx14 "..json.encode({event="native_rng_chunk",action=action,name=actions[action],token=config.token,serial=serial,part=part,total=total,data=encoded:sub((part-1)*4000+1,part*4000)}))
    end
  else Isaac.DebugString("[isaac-cs-test] tx14 "..encoded)end
end
local function state() return json.decode(IsaacCSObservationNative.GlobalRandomState()) end
local function native(count,seed)
  local v=IsaacCSObservationNative.TestGlobalRandom(config.token,count,seed)
  v.before=json.decode(v.before);v.after=json.decode(v.after);return v
end
local function swords() return Isaac.FindByType(EntityType.ENTITY_FAMILIAR,FamiliarVariant.DAMOCLES,-1,false,false) end
local function safeRoom()
  for _,e in ipairs(Isaac.GetRoomEntities()) do if e:ToNPC() then e:Remove() end end
  Game():GetRoom():SetClear(true)
  for _,p in ipairs(PlayerManager.GetPlayers()) do p.Position=Game():GetRoom():GetCenterPos();p.Velocity=Vector.Zero end
end
local function perform(name)
  if name=="seed" then Isaac.ExecuteCommand("seed S227 7MNF");return end
  safeRoom()
  if name=="readonly" then
    local a=state();local b=state();assert(a.complete and b.complete,"native_snapshot_not_complete")
    assert(a.calls==b.calls and a.state.index==b.state.index,"observer_consumed_random")
    emit("actual",{before=a,after=b})
  elseif name=="lua_draws" then
    local a=state();local results={Random(),cachedRandom()};local b=state()
    emit("actual",{before=a,after=b,results=results})
  elseif name=="native_twists" then emit("actual",native(1300,305419896))
  elseif name=="zero_seed" then emit("actual",native(1,0))
  elseif name=="damocles_spawn" then
    local p=Isaac.GetPlayer(0)
    p:UseActiveItem(CollectibleType.COLLECTIBLE_DAMOCLES,UseFlag.USE_NOANIM|UseFlag.USE_NOANNOUNCER,-1)
    pending={test=function()return #swords()>0 and p:HasCollectible(CollectibleType.COLLECTIBLE_DAMOCLES_PASSIVE) end,
      actual=function()emit("actual",{swords=#swords(),passive=p:HasCollectible(656),subtype=swords()[1].SubType})end}
  elseif name=="damocles_damage" then
    local p=Isaac.GetPlayer(0);local old=p:GetHearts()+p:GetSoulHearts()
    p:ResetDamageCooldown();p:TakeDamage(1,0,EntityRef(p),0)
    assert(p:GetHearts()+p:GetSoulHearts()<old,"damocles_requires_actual_damage")
    local start=Game():GetFrameCount()
    pending={test=function()return Game():GetFrameCount()-start>=180 and #swords()>0 and swords()[1].SubType>0 end,
      actual=function()emit("actual",{swords=#swords(),subtype=swords()[1].SubType,elapsed=Game():GetFrameCount()-start,state=state()})end}
  elseif name=="record_off" or name=="observe_off" or name=="observe_restart" then emit("actual",native(24))
  else error("unknown_action:"..tostring(name)) end
end
mod:AddCallback(ModCallbacks.MC_POST_GAME_STARTED,function()running=true end)
mod:AddCallback(ModCallbacks.MC_PRE_GAME_EXIT,function()running=false end)
mod:AddCallback(ModCallbacks.MC_POST_RENDER,function()
  render=render+1;if not running or Game():GetNumPlayers()==0 then return end
  if Input.IsButtonTriggered(Keyboard.KEY_F8,0) then
    action=action+1;local ok,err=pcall(perform,actions[action])
    if not ok then pending=nil;emit("fixture_error",{error=tostring(err)})
    else pending=pending or {};pending.start=render;emit("action_return")end
  end
  if pending and render-pending.start>=12 then
    local ok,ready=pcall(function()return not pending.test or pending.test()end)
    if not ok then pending=nil;emit("fixture_error",{error=tostring(ready)})
    elseif ready then local p=pending;pending=nil;local copied,err=pcall(function()if p.actual then p.actual()end end)
      if copied then emit("ready")else emit("fixture_error",{error=tostring(err)})end
    elseif render-pending.start>1800 then pending=nil;emit("fixture_error",{error="native_rng_result_timeout"})end
  end
end)
