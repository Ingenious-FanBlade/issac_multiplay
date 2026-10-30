-- tx-04-14统一实机场景；所有玩法变化由F8触发，观察器不制造随机/创建/保存。
local mod=RegisterMod("Isaac CS tx14 E2E",1)
local config=include("harness_config");local json=require("json")
local actions={"seed","initial","rules","seed_effect","random_calls","pool_changes","pill_identify","hud","audio","graph","graph_limits","graph_recover","stage_a","stage_mutate","grid_remove","grid_rebuild","stage_b","stage_return","next_floor","query","retire","registry_budget"}
if config.pending then local resumed={"seed","resume_prepare"};for _,name in ipairs(config.pending) do resumed[#resumed+1]=name end;actions=resumed end
local action,serial,render=0,0,0
local running,pending=false,nil
local state={revision=0};local rng;local sprite;local source,target;local grids={};local lastRandom
local function emit(event,data)
  serial=serial+1;data=data or {};data.event=event;data.action=action;data.name=actions[action];data.token=config.token;data.serial=serial
  Isaac.DebugString("[isaac-cs-test] tx14 "..json.encode(data))
end
local function safeRoom()
  for _,e in ipairs(Isaac.GetRoomEntities()) do if e:ToNPC() then e:Remove() end end
  Game():GetRoom():SetClear(true)
  for _,p in ipairs(PlayerManager.GetPlayers()) do p.Position=Game():GetRoom():GetCenterPos();p.Velocity=Vector.Zero;p:SetMinDamageCooldown(18000) end
end
local function graph()
  local shared={value=43};state.graph={left=shared,right=shared,[7]="sparse"};state.graph.self=state.graph
end
local function roomObject(index,kind)
  local desc=Game():GetLevel():GetRoomByIdx(index,0)
  local room=StageAPI.LevelRoom{RoomDescriptor=desc,RoomType=RoomType.ROOM_DEFAULT,Shape=desc.Data.Shape,FromData=desc.ListIndex,LevelIndex=desc.ListIndex}
  room:SetTypeOverride(kind);room.PersistentData.tx14=kind
  StageAPI.SetLevelRoom(room,desc.ListIndex,0)
  return room
end
local function spawnGrids()
    -- 自定义名称不是原生枚举；沿用逻辑名，实际载体为Poop的明确0/1变体。
    local a=StageAPI.CustomGrid("tx14.rock.A",{BaseType=GridEntityType.GRID_POOP,BaseVariant=0,NoOverrideGridSprite=true})
    local b=StageAPI.CustomGrid("tx14.rock.B",{BaseType=GridEntityType.GRID_POOP,BaseVariant=1,NoOverrideGridSprite=true})
    local n=StageAPI.CustomGrid("tx14.logic.only",{})
    grids={a:Spawn(70,true,false,{tx14=10}),b:Spawn(72,true,false,{tx14=20}),n:Spawn(74,true,false,{tx14=30})}
    grids[1].Data.tx14=1;grids[2].Data.tx14=2;grids[3].Data.tx14=3
    local details={};for _,grid in ipairs(grids) do local carrier=Game():GetRoom():GetGridEntity(grid.GridIndex);details[#details+1]={name=grid.GridConfig.Name,index=grid.GridIndex,native_exists=carrier~=nil,native_type=carrier and carrier:GetType(),native_variant=carrier and carrier:GetVariant(),room_id=grid.RoomIndex} end
    emit("grids_spawned",{details=details,current_room_id=StageAPI.GetCurrentRoomID()})
end
local function actual()
  local g=Game();local p=g:GetItemPool();local x=StageAPI and StageAPI.GetCurrentRoom()
  local out={revision=state.revision,frame=g:GetFrameCount(),time=g.TimeCounter,app_frame=Isaac.GetFrameCount(),shops=g:GetShopVisits(),angel_modifier=g:GetLevel():GetAngelRoomChance(),devil=g:GetRoom():GetDevilRoomChance(),planetarium=g:GetLevel():GetPlanetariumChance(),seed=g:GetSeeds():GetStartSeed(),seed_effect=g:GetSeeds():HasSeedEffect(SeedEffect.SEED_NO_HUD),pill_effect=p:GetPillEffect(1),pill_identified=p:IsPillIdentified(1),pool_size=#p:GetCollectiblesFromPool(0),hud_visible=g:GetHUD():IsVisible(),music=MusicManager():GetCurrentMusicID(),stageapi_loaded=StageAPI and StageAPI.Loaded or false,logical_type=x and x:GetType(),native_type=g:GetRoom():GetType(),room_index=g:GetLevel():GetCurrentRoomIndex(),stage_grids={}}
  for _,grid in ipairs(StageAPI.CustomGridEntities or {}) do
    out.stage_grids[#out.stage_grids+1]={name=grid.GridConfig.Name,index=grid.GridIndex,persistent_index=grid.PersistentIndex,data_value=grid.Data.tx14,persist_value=grid.PersistentData.tx14,carrier=grid.GridEntity and IsaacCSObservationNative.GridReference(grid.GridEntity) or {resolution="Null"},subject=IsaacCSModObservation.Subject(grid,"StageAPI.CustomGridEntity",require("json").decode(IsaacCSObservationNative.CaptureContext()).run..":"..require("json").decode(IsaacCSObservationNative.CaptureContext()).floor)}
  end
  if actions[action]=="stage_a" or actions[action]=="stage_mutate" or actions[action]=="grid_rebuild" or actions[action]=="stage_return" then assert(#out.stage_grids==3,"three_real_StageAPI_grids_required:"..json.encode(out.stage_grids)) end
  if actions[action]=="grid_remove" then assert(#out.stage_grids==2,"removed_instance_still_member_or_other_grid_missing") end
  out.random=lastRandom
  emit("actual",out)
end
local function perform(name)
  local g=Game();local pool=g:GetItemPool();local p=Isaac.GetPlayer(0)
  if name=="seed" then Isaac.ExecuteCommand("seed S227 7MNF")
  elseif name=="initial" then
    assert(StageAPI and StageAPI.Loaded,"real_StageAPI_required")
    safeRoom();source=g:GetLevel():GetCurrentRoomIndex()
    for i=0,g:GetLevel():GetRooms().Size-1 do local d=g:GetLevel():GetRooms():Get(i);if d.GridIndex>=0 and d.GridIndex~=source and d.Data.Type==RoomType.ROOM_DEFAULT and d.Data.Shape==1 then target=d.GridIndex;break end end
    assert(target,"second_native_default_room_required")
    graph();rng=RNG(1234567,35)
    IsaacCSModObservation.Register{namespace="isaac.cs.e2e.tx14",version="1",schema="isaac.cs.mod.tx14.v1",lifetime="run",phases={OrdinaryPost=true,Lifecycle=true,Render=true},read=function()
      local saved=EntitySaveStateManager.TryGetEntityData(mod,p)
      return {revision=state.revision,graph=state.graph,entity_data=state.entity_data,save_extension=saved,rng=IsaacCSRandomObservation.Register(rng,{namespace="isaac.cs.e2e.tx14"},"fixture_rng",{},"run")}
    end}
  elseif name=="resume_prepare" then
    perform("initial")
    local first=config.pending[1]
    if first=="stage_mutate" or first=="grid_remove" or first=="grid_rebuild" or first=="stage_b" or first=="stage_return" then
      roomObject(source,"tx14.room.A"):Load(false,true);safeRoom();pending={spawn_at=render+12,mutate=true}
    end
  elseif name=="rules" then g:AddShopVisits(2);g:GetLevel():AddAngelRoomChance(0.25)
  elseif name=="seed_effect" then g:GetSeeds():AddSeedEffect(SeedEffect.SEED_NO_HUD)
  elseif name=="random_calls" then
    local stream=IsaacCSRandomObservation.Register(rng,{namespace="isaac.cs.e2e.tx14"},"fixture_rng",{},"run")
    local before=tostring(rng:GetSeed());local int=rng:RandomInt(3,200);local float=rng:RandomFloat();local vector=rng:RandomVector();local next=rng:Next()
    local after=tostring(rng:GetSeed());local selected=pool:GetCollectible(0,true,7654321);local card=pool:GetCard(918273,true,true,false);local pill=pool:GetPill(918273);local trinket=pool:GetTrinket()
    local itemRng=p:GetCollectibleRNG(1);itemRng:RandomInt(100)
    lastRandom={before=before,after=after,int=int,float=float,vector={x=vector.X,y=vector.Y},next=next,selected=selected,card=card,pill=pill,trinket=trinket,stream=stream.subject}
    emit("actual_random",lastRandom)
  elseif name=="pool_changes" then pool:AddTemporaryCollectible(0,{itemID=1,weight=4.5,decreaseBy=0.25,removeOn=0.01});pool:AddRoomBlacklist(2);pool:RemoveCollectible(3)
  elseif name=="pill_identify" then pool:IdentifyPill(1)
  elseif name=="hud" then
    g:GetSeeds():RemoveSeedEffect(SeedEffect.SEED_NO_HUD);p:AddCoins(7);p:AddSoulHearts(3);p:AddCollectible(1);g:GetHUD():ShowItemText("tx-04-14","real HUD event");g:ShakeScreen(12)
    sprite=Sprite();sprite:Load("gfx/005.100_collectible.anm2",true);sprite:Play("Idle",true)
    IsaacCSPresentationObservation.Register("isaac.cs.e2e.tx14","independent",function()return sprite,{revision=state.revision,lifetime="until_retire"} end)
  elseif name=="audio" then SFXManager():Play(SoundEffect.SOUND_BEEP,0.1,0,true,1,0);MusicManager():PitchSlide(0.9)
  elseif name=="graph" then
    state.entity_data=p:GetData();state.entity_data.tx14={value=73}
    local saved=EntitySaveStateManager.GetEntityData(mod,p);saved.tx14={value=97}
    IsaacCSModObservation.Event("isaac.cs.e2e.tx14","state_changed",{revision=state.revision})
  elseif name=="graph_limits" then
    state.graph.bad=function()end;state.graph.nan=0/0;state.graph.thread=coroutine.create(function()end)
    local chain=state.graph;for i=1,18 do chain.deep={};chain=chain.deep end
  elseif name=="graph_recover" then graph()
  elseif name=="stage_a" then
    roomObject(source,"tx14.room.A"):Load(false,true);safeRoom()
    pending={spawn_at=render+12}
  elseif name=="stage_mutate" then for i,grid in ipairs(grids) do grid.Data.tx14=100+i;grid.PersistentData.tx14=200+i end
  elseif name=="grid_remove" then grids[1]:Remove(false)
  elseif name=="grid_rebuild" then grids[1]=StageAPI.CustomGridTypes["tx14.rock.B"]:Spawn(70,true,false,{tx14=999});grids[1].Data.tx14=777
  elseif name=="stage_b" then
    roomObject(target,"tx14.room.B");g:StartRoomTransition(target,Direction.NO_DIRECTION,0,nil,0);pending={test=function()return g:GetLevel():GetCurrentRoomIndex()==target and StageAPI.GetCurrentRoom() and StageAPI.GetCurrentRoom():GetType()=="tx14.room.B" end}
  elseif name=="stage_return" then g:StartRoomTransition(source,Direction.NO_DIRECTION,0,nil,0);pending={test=function()return g:GetLevel():GetCurrentRoomIndex()==source and StageAPI.GetCurrentRoom() and #StageAPI.CustomGridEntities>=3 end}
  elseif name=="next_floor" then local old=g:GetLevel():GetStage();Isaac.ExecuteCommand("stage 2");pending={test=function()return g:GetLevel():GetStage()~=old end}
  elseif name=="query" then IsaacCSRunObservation.Query(g:GetSeeds(),"Seeds","HasSeedEffect",{SeedEffect.SEED_NO_HUD})
  elseif name=="registry_budget" then
    -- 在真实游戏采集链内耗尽登记预算，只登记已有Mod RNG的用途，不推进随机或改世界。
    local before=tostring(rng:GetSeed())
    for i=1,16385 do IsaacCSRandomObservation.Register(rng,{namespace="isaac.cs.e2e.tx14"},"budget:"..i,{},"run") end
    assert(tostring(rng:GetSeed())==before,"registry_observation_advanced_rng")
    emit("registry_budget_actual",{seed_before=before,seed_after=tostring(rng:GetSeed()),failures=IsaacCSRandomObservation.registration_failures})
  elseif name=="retire" then SFXManager():Stop(SoundEffect.SOUND_BEEP);MusicManager():PitchSlide(1);sprite=nil;IsaacCSPresentationObservation.Unregister("isaac.cs.e2e.tx14","independent");IsaacCSModObservation.Unregister("isaac.cs.e2e.tx14")
  else error("unknown_action:"..tostring(name)) end
  state.revision=state.revision+1
end
mod:AddCallback(ModCallbacks.MC_POST_GAME_STARTED,function()running=true end)
mod:AddCallback(ModCallbacks.MC_PRE_GAME_EXIT,function()running=false end)
mod:AddCallback(ModCallbacks.MC_POST_NEW_ROOM,function()if action>1 then safeRoom() end end)
mod:AddCallback(ModCallbacks.MC_POST_RENDER,function()
  render=render+1;if not running or Game():GetNumPlayers()==0 then return end
  if sprite then sprite:Update();sprite:Render(Vector(80,80)) end
  if Input.IsButtonTriggered(Keyboard.KEY_F8,0) then
    action=action+1;local ok,err=pcall(perform,actions[action])
    if not ok then pending=nil;emit("fixture_error",{error=tostring(err)}) else pending=pending or {};pending.start=render;emit("action_return") end
  end
  if pending and pending.spawn_at and render>=pending.spawn_at then
    local ok,err=pcall(spawnGrids);pending.spawn_at=nil
    if ok and pending.mutate then for i,grid in ipairs(grids) do grid.Data.tx14=100+i;grid.PersistentData.tx14=200+i end end
    if not ok then pending=nil;emit("fixture_error",{error=tostring(err)}) end
  end
  if pending and not pending.spawn_at and render-pending.start>=45 then
    local ok,ready=pcall(function()return not pending.test or pending.test()end)
    if not ok then pending=nil;emit("fixture_error",{error=tostring(ready)})
    elseif ready then pending=nil;local copied,err=pcall(actual);if copied then emit("ready") else emit("fixture_error",{error=tostring(err)}) end
    elseif render-pending.start>2400 then pending=nil;emit("fixture_error",{error="actual_result_timeout"}) end
  end
end)
