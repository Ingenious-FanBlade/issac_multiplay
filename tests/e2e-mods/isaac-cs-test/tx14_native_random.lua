-- 观测实现完成后编写的统一实机夹具；所有动作由harness的F8触发，不替换随机算法。
local mod=RegisterMod("Isaac CS tx14 remaining RNG E2E",1)
local config=include("harness_config");local json=require("json")
local cachedFloat=RandomFloat
local cachedMath=math.random
local cachedNext=RNG(1234567,35).Next
local actions=config.pending or {"seed","inventory","primitives","mt","crt","runtime","inline","spawn","loot","retire","next_floor","paused","record_off","observe_off","observe_restart"}
local action,serial,render=0,0,0
local running,pending,pickup=false,nil,nil
local function emit(event,data)
  serial=serial+1;data=data or {};data.event=event;data.action=action;data.name=actions[action];data.token=config.token;data.serial=serial
  local s=json.encode(data)
  if #s>4000 then
    local total=math.ceil(#s/4000)
    for part=1,total do Isaac.DebugString("[isaac-cs-test] tx14 "..json.encode({event="native_rng_chunk",action=action,name=actions[action],token=config.token,serial=serial,part=part,total=total,data=s:sub((part-1)*4000+1,part*4000)}))end
  else Isaac.DebugString("[isaac-cs-test] tx14 "..s)end
end
local function native(mode,count,seed)
  return json.decode(IsaacCSObservationNative.TestNativeRandomSources(config.token,mode,count or 0,seed or 1234567))
end
local function latest()return json.decode(IsaacCSObservationNative.NativeRandomSources())end
local function safeRoom()
  for _,e in ipairs(Isaac.GetRoomEntities())do if e:ToNPC()then e:Remove()end end
  Game():GetRoom():SetClear(true)
  for _,p in ipairs(PlayerManager.GetPlayers())do p.Position=Game():GetRoom():GetCenterPos();p.Velocity=Vector.Zero end
end
local function perform(name)
  if name=="seed" then Isaac.ExecuteCommand("seed S227 7MNF");return end
  safeRoom()
  if name=="inventory" then
    local providers=native("providers")
    for _,s in ipairs(providers.sources)do assert(s.installed and s.readable,"runtime_source_unavailable:"..json.encode(providers))end
    local a=latest();local b=latest();assert(a.complete and b.complete,"native_inventory_incomplete:"..json.encode({owner_errors=a.owner_errors,providers=providers,failures=a.failures}))
    assert(json.encode(a)==json.encode(b),"read_only_latest_mutated")
    emit("actual",{owners=native("owners"),providers=providers})
  elseif name=="primitives" then
    local extra=RNG(7654321,35);local before=extra:GetSeed();local value=cachedNext(extra);local after=extra:GetSeed()
    assert(value==after and before~=after,"cached_native_next")
    emit("actual",{native=native("primitives",30),cached={before=before,result=value,after=after}})
  elseif name=="mt" then emit("actual",native("mt",1300,305419896))
  elseif name=="crt" then emit("actual",native("crt",64,7654321))
  elseif name=="crt_imports" then
    local seeded=native("crt",0,1234567);local values={}
    for i=1,8 do
      local before=native("crtstate");local effect=EntityEffect.CreateLight(Game():GetRoom():GetCenterPos())
      assert(effect,"native_REP_light_required");local after=native("crtstate");local scale=effect:GetSprite().Scale.X
      effect:Remove();values[i]={before=before,after=after,scale=scale}
    end
    emit("actual",{seeded=seeded,providers=native("providers"),lights=values})
  elseif name=="runtime" then
    local before=native("floatstate");local values={}
    for i=1,1300 do values[i]=i%2==0 and RandomFloat() or cachedFloat()end
    local after=native("floatstate")
    local seed1,seed2=math.randomseed(1234,5678)
    local mathBefore=native("mathstate")
    local draws={}
    for i=1,12 do
      if i%3==1 then draws[i]={argc=0,value=cachedMath()}
      elseif i%3==2 then draws[i]={argc=2,lo=-10,hi=10,value=tostring(math.random(-10,10))}
      else draws[i]={argc=1,hi=0,value=tostring(cachedMath(0))}end
    end
    local errorBefore=native("mathstate");local ok,err=pcall(cachedMath,2,1);assert(not ok and type(err)=="string","math_error_swallowed")
    local errorAfter=native("mathstate");local recovered=cachedMath();local mathAfter=native("mathstate")
    emit("actual",{before=before,after=after,values=values,seed1=tostring(seed1),seed2=tostring(seed2),math_before=mathBefore,draws=draws,
      error_before=errorBefore,error_after=errorAfter,error=tostring(err),recovered=recovered,math_after=mathAfter})
  elseif name=="inline" then emit("actual",native("inline",9))
  elseif name=="spawn" then
    pickup=Isaac.Spawn(EntityType.ENTITY_PICKUP,PickupVariant.PICKUP_CHEST,0,Game():GetRoom():GetCenterPos()+Vector(80,0),Vector.Zero,nil):ToPickup()
    pending={actual=function()emit("actual",{subject=IsaacCSObservationNative.Reference(pickup),seed=pickup:GetDropRNG():GetSeed()})end}
  elseif name=="loot" then
    assert(pickup and pickup:Exists(),"pickup_owner_required")
    local a=pickup:GetDropRNG():GetSeed();local read=pickup:GetLootList(false,Isaac.GetPlayer(0));local b=pickup:GetDropRNG():GetSeed()
    assert(a==b,"readonly_loot_advanced_owner")
    local advance=pickup:GetLootList(true,Isaac.GetPlayer(0));local d=pickup:GetDropRNG():GetSeed()
    local grid
    for i=0,Game():GetRoom():GetGridSize()-1 do local g=Game():GetRoom():GetGridEntity(i);if g and g:GetType()==GridEntityType.GRID_WALL then grid=g;break end end
    assert(grid,"real_grid_required");local r=grid:GetRNG();local g0=r:GetSeed();local g1=r:Next()
    emit("actual",{before=a,readonly=b,after=d,read_count=#read:GetEntries(),advance_count=#advance:GetEntries(),subject=IsaacCSObservationNative.Reference(pickup),
      grid=IsaacCSObservationNative.GridReference(grid),grid_before=g0,grid_after=g1})
  elseif name=="retire" then
    local subject=IsaacCSObservationNative.Reference(pickup);pickup:Remove();pickup=nil;emit("actual",{retired=subject})
  elseif name=="next_floor" then
    local old=Game():GetLevel():GetStage();Isaac.ExecuteCommand("stage 2")
    pending={test=function()return Game():GetLevel():GetStage()~=old end,actual=function()safeRoom();emit("actual",{stage=Game():GetLevel():GetStage()})end}
  elseif name=="paused" then emit("actual",{native=native("primitives",6),value=tostring(cachedMath(0))})
  elseif name=="record_off" then emit("actual",{native=native("primitives",6),crt=native("crt",6)})
  elseif name=="observe_off" then emit("actual",{native=native("primitives",6),mt=native("mt",6),crt=native("crt",6),float=cachedFloat(),math=tostring(cachedMath(0))})
  elseif name=="observe_restart" then emit("actual",{native=native("primitives",6)})
  else error("unknown_native_rng_action:"..tostring(name))end
end
mod:AddCallback(ModCallbacks.MC_POST_GAME_STARTED,function()running=true end)
mod:AddCallback(ModCallbacks.MC_PRE_GAME_EXIT,function()running=false end)
mod:AddCallback(ModCallbacks.MC_POST_RENDER,function()
  render=render+1;if not running or Game():GetNumPlayers()==0 then return end
  if Input.IsButtonTriggered(Keyboard.KEY_F8,0)then
    action=action+1;local ok,err=pcall(perform,actions[action])
    if ok then pending=pending or {};pending.start=render;emit("action_return")else pending=nil;emit("fixture_error",{error=tostring(err):sub(1,3000)})end
  end
  if pending and render-pending.start>=12 then
    local ok,ready=pcall(function()return not pending.test or pending.test()end)
    if not ok then pending=nil;emit("fixture_error",{error=tostring(ready):sub(1,3000)})
    elseif ready then local p=pending;pending=nil;local success,err=pcall(function()if p.actual then p.actual()end end)
      if success then emit("ready")else emit("fixture_error",{error=tostring(err):sub(1,3000)})end
    elseif render-pending.start>1800 then pending=nil;emit("fixture_error",{error="native_rng_action_deadline"})end
  end
end)
