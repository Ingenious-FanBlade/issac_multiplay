-- 随机流以owner/用途/参数和生命周期登记；seed仅是当前状态，不是身份。
local X=include("domains/common/extended")
local R=X.Read
local Calls=include("domains/common/calls")
local schemas=include("domains/random/schema");R.AddSchemas(schemas)
local M={SchemaVersion=2,streams={},byObject=setmetatable({},{__mode="k"})}
local epoch
local function scope()
  local x=X.Shared.Context();local key=x.session..":"..x.epoch..":"..x.run
  if key~=epoch then epoch=key;M.streams={};M.count=0;M.registration_failures=0;M.byObject=setmetatable({},{__mode="k"}) end
  return x
end
function M.Register(rng,owner,purpose,parameters,lifetime)
  if not rng then return {resolution="Null"} end
  local x=scope();local c=X.Context();local p=R.Value(parameters or {},nil,c,0)
  local identity=owner and {space=owner.space,session=owner.session,epoch=owner.epoch,local_id=owner.local_id,use=owner.use,namespace=owner.namespace,subject=owner.subject}
  local key=R.Encode({owner=identity,purpose=purpose,parameters=p,lifetime=lifetime})
  local s=M.streams[key]
  if not s then
    if M.count and M.count>=16384 then M.registration_failures=(M.registration_failures or 0)+1;return {resolution="Unresolved",reason="stream_budget"} end
    M.count=(M.count or 0)+1;s={subject={space=11,session=x.session,epoch=x.epoch,local_id=IsaacCSObservationNative.RelatedSubject(11)},owner=owner,purpose=purpose,parameters=p,lifetime=lifetime or x.run,first_observation="FirstObserved;not_birth"};M.streams[key]=s
  end
  M.byObject[rng]=s;s.owner=owner
  s.native_reference=require("json").decode(IsaacCSObservationNative.RegisterRandomSource(rng,R.Encode(identity or {}),purpose or "",tostring(lifetime or x.run)))
  s.state={seed=tostring(rng:GetSeed()),shift=rng:GetShiftIdx(),source_scope=x.scope,source_application_frame=Isaac.GetFrameCount()}
  return {type="random_stream_reference",subject=s.subject,owner=owner,purpose=purpose,current=s.state}
end
function M.State(rng)
  local s=M.byObject[rng]
  return {stream=s and s.subject or {resolution="Unresolved",reason="owner_not_registered;not_inferred_from_seed"},seed=tostring(rng:GetSeed()),shift=rng:GetShiftIdx()}
end
function M.Install()
  Calls.Class("RNG",{Next=true,Previous=true,RandomInt=true,RandomFloat=true,RandomVector=true,SetSeed=true},6,function(_,a)return M.State(a[1]) end,function(_,a,_,_,_)return M.State(a[1]) end)
  local methods={};for _,s in ipairs(schemas.ItemPool) do if s.category=="operation" or s.category=="actual_call" then methods[s.member]=true end end
  Calls.Class("ItemPool",methods,6)
  -- 只观察真实Get*RNG，避免为枚举全部物品ID而懒创建随机流。
  for _,class in ipairs({"Entity","EntityPlayer","GridEntity","GridEntityPressurePlate","Level"}) do
    local names=class=="Entity" and {GetDropRNG=true} or class=="EntityPlayer" and {GetCollectibleRNG=true,GetTrinketRNG=true,GetCardRNG=true,GetPillRNG=true} or class=="Level" and {GetGenerationRNG=true,GetDevilAngelRoomRNG=true} or {GetRNG=true}
    Calls.Class(class,names,6,nil,function(name,a,r,_,c)
      if not r[1] or not r[2] then return end
      local owner=class=="Level" and {space=4,local_id=X.Shared.Context().floor} or class:find("Grid") and IsaacCSObservationNative.GridReference(a[1]) or IsaacCSObservationNative.Reference(a[1])
      local args={n=a.n-1};for i=2,a.n do args[i-1]=a[i] end
      return M.Register(r[2],owner,class.."."..name,args,class=="Level" and X.Shared.Context().floor or owner.use)
    end)
  end
  Calls.Global(_G,"Random",6);Calls.Global(_G,"RandomVector",6)
end
function M.Capture(limits)
  R.Limits(limits);local x=scope();local c=X.Context();local pool=Game():GetItemPool()
  -- 原生公共入口覆盖Lua缓存函数和游戏C++调用；完整MT状态不冒充RNG类seed/shift。
  local nativeGlobal=require("json").decode(IsaacCSObservationNative.GlobalRandomState())
  local out={location=x,streams={},pools={},removed=R.Value(pool:GetRemovedCollectibles(),nil,c,0),blacklist=R.Value(pool:GetRoomBlacklistedCollectibles(),nil,c,0),pills={},num_available_trinkets=pool:GetNumAvailableTrinkets(),last_pool=pool:GetLastPool(),source_gaps={"native_generation_AI_combat_drop_consumption_bypasses_Lua;current_state_only","cached_Lua_functions_obtained_before_wrapper_installation","player_item_RNG_not_requested_until_real_getter;lazy_creation_not_polled"},instrumentation={installed=Calls.installed,failures=Calls.failures}}
  out.native_global_random=nativeGlobal;c.nodes=c.nodes+624
  if not nativeGlobal.complete then R.Fail(c,"native_global_random_incomplete") end
  out.source_gaps[1]="RNG_class_native_generation_AI_combat_drop_consumption_not_hooked;global_Random_native_entry_observed"
  out.source_gaps[2]="cached_Lua_functions_except_global_Random;native_global_entry_covers_cached_Random"
  for _,e in ipairs(Isaac.GetRoomEntities()) do
    local owner=IsaacCSObservationNative.Reference(e)
    M.Register(Calls.Silent(function()return e:GetDropRNG()end),owner,"Entity.GetDropRNG",{},owner.use)
  end
  local grids=IsaacCSObservationNative.GridSources()
  for _,entry in ipairs(grids) do
    local grid=entry.grid or entry.entity or entry.object
    if grid then local owner=IsaacCSObservationNative.GridReference(grid);M.Register(Calls.Silent(function()return grid:GetRNG()end),owner,"GridEntity.GetRNG",{},owner.use) end
  end
  -- Level的两个Getter公开返回已存在的固定成员，不分配物品RNG。
  M.Register(Calls.Silent(function()return Game():GetLevel():GetGenerationRNG()end),{space=4,local_id=x.floor},"Level.GetGenerationRNG",{},x.floor)
  M.Register(Calls.Silent(function()return Game():GetLevel():GetDevilAngelRoomRNG()end),{space=4,local_id=x.floor},"Level.GetDevilAngelRoomRNG",{},x.floor)
  -- 不跨生命周期解引用缓存的原生RNG；仅本窗口已取得资格的Getter/适配reader刷新状态。
  for _,s in pairs(M.streams) do
    s.freshness=s.state.source_scope==x.scope and "ReadInCurrentNativeWindow" or "HistoricalObservedState;not_polled_without_owner_qualification"
    out.streams[#out.streams+1]=s;c.objects=c.objects+1
  end
  -- 登记预算耗尽必须使域显式Partial，不能仅漏掉新成员而保持Complete。
  out.registry_status=(M.registration_failures or 0)>0 and "Truncated" or "CompleteWithinObservedSources"
  out.registration_failures=M.registration_failures or 0
  if out.registration_failures>0 then R.Fail(c,"stream_registry_budget") end
  local count=pool:GetNumItemPools()
  if count<0 or count>1024 then R.Fail(c,"pool_count_budget") end
  for i=0,math.min(count,1024)-1 do c.objects=c.objects+1;out.pools[#out.pools+1]={index=i,items=R.Value(pool:GetCollectiblesFromPool(i),nil,c,0),bible_upgrades=pool:GetBibleUpgrades(i)} end
  for color=1,14 do
    out.pills[#out.pills+1]={color=color,effect=pool:GetPillEffect(color),identified=pool:IsPillIdentified(color),horse=false}
    local horse=color+2048;out.pills[#out.pills+1]={color=horse,effect=pool:GetPillEffect(horse),identified=pool:IsPillIdentified(color),horse=true,base_color=color,identification_source_color=color}
  end
  out.trinket_availability={}
  for _,args in ipairs(X.Enum("TrinketType")) do out.trinket_availability[#out.trinket_availability+1]={id=args[1],available=pool:HasTrinket(args[1])} end
  -- 完整原生快照由C++直接合并；Lua避免整份解码后再次编码。
  out.native_sources_summary=require("json").decode(IsaacCSObservationNative.NativeRandomSources())
  c.objects=c.objects+(out.native_sources_summary.objects or 0);c.nodes=c.nodes+(out.native_sources_summary.nodes or 0)
  if not out.native_sources_summary.complete then R.Fail(c,"native_random_sources_incomplete") end
  out.source_gaps={"inline_consumption_is_observed_as_state_change;not_fabricated_as_individual_calls","unknown_private_Mod_RNG_requires_registration"}
  return R.Finish(out,c)
end
function M.Outcome() end
function M.Query(object,class,member,args) return X.Query(6,schemas,object,class,member,args) end
function M.Event(name,...) local c=X.Context();X.Publish(6,{event=name,arguments=R.Value(table.pack(...),nil,c,0),source="actual_public_callback;not_an_extra_draw"},0,c) end
return M
