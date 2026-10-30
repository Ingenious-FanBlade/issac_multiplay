-- tx-04-13专项玩法夹具；所有写操作只由F8动作启动，不进入正式模组。
local mod=RegisterMod("Isaac CS continuity E2E",1)
local config=include("harness_config"); local json=require("json")
local action,serial,render=0,0,0
local running=false; local pending; local source; local target; local taken; local remains; local machine; local grid
local roomEvent,levelEvent=0,0;local esauOriginal
local actions={"seed","initial","reveal","hide","mutate","leave","revisit","box_pack","box_leave","box_return","box_unpack","inventory","effects_charge","twins","twins_leave","twins_return","forms","flip","forms_leave","forms_return","esau_prepare","esau_swap","esau_restore","death","revive","downpour","mirror","normal","myosotis","next_floor","query","disconnect_ready"}
if config.resume then
  local resumed={"seed","resume_prepare"};local started=false
  for _,name in ipairs(actions) do if name==config.resume_start then started=true end;if config.pending and config.pending[name] or not config.pending and started then resumed[#resumed+1]=name end end
  actions=resumed
end
local function emit(event,data)
  serial=serial+1; data=data or {}; data.event=event; data.action=action; data.name=actions[action]; data.token=config.token; data.serial=serial
  Isaac.DebugString("[isaac-cs-test] tx13 "..json.encode(data))
end
local function primary(ordinal,allowMissing)
  local status=tostring(Isaac.ExecuteCommand("cs status")); local controller=tonumber(status:match("\nparticipant%."..ordinal.."%.controller=([%-0-9]+)"))
  for _,p in ipairs(PlayerManager.GetPlayers()) do local relation=IsaacCSObservationNative.Subject(p).control_relation;if relation and relation.ordinal==ordinal and relation.body_slot==0 then return p end end
  for _,p in ipairs(PlayerManager.GetPlayers()) do if p.ControllerIndex==controller and (not p:GetMainTwin() or p:GetMainTwin().Index==p.Index) then return p end end
  if allowMissing then return nil end
  error("primary ordinal unavailable:"..ordinal)
end
local function prepare()
  for _,e in ipairs(Isaac.GetRoomEntities()) do if e:ToNPC() then e:Remove() end end
  for _,p in ipairs(PlayerManager.GetPlayers()) do p.Position=Game():GetRoom():GetCenterPos(); p.Velocity=Vector.Zero; p:SetMinDamageCooldown(18000) end
  Game():GetRoom():SetClear(true)
end
-- 对照仅使用真实结果；保存计数和字段直接读取，不以Use/Spawn请求日志算通过。
local function actual()
  local l=Game():GetLevel(); local d=l:GetCurrentRoomDesc(); local src=source and l:GetRoomByIdx(source,0)
  local out={room=l:GetCurrentRoomIndex(),dimension=l:GetDimension(),stage=l:GetStage(),stage_type=l:GetStageType(),source=source,display_flags=d.DisplayFlags,visited=d.VisitedCount,clear=d.Clear,curses=l:GetCurses(),minimap=Minimap.GetState(),myosotis=#l:GetMyosotisPickups()}
  if src and src.Data then
    out.source_saved={entities=#src:GetEntitiesSaveState(),grids=#src:GetGridEntitiesSaveState(),deco=#src:GetDecoSaveState(),display_flags=src.DisplayFlags,visited=src.VisitedCount}
    local grids=src:GetGridEntitiesSaveState();local selected={[0]=true,[#grids-1]=true};if grid then selected[grid]=true end
    for i=0,#grids-1 do if grids:Get(i).Type~=0 then selected[i]=true;break end end
    for index in pairs(selected) do if index>=0 and index<#grids then local v=grids:Get(index)
      emit("actual_saved_grid",{source=source,dimension=0,ordinal=index,values={v.Initialized,v.SpawnCount,v.SpawnSeed,v.State,v.Type,v.VarData,v.VariableSeed,v.Variant}})
    end end
    local saves=src:GetEntitiesSaveState(); out.source_saved.keys=0;out.source_saved.machines=0
    for i=0,#saves-1 do local e=saves:Get(i)
      if e:GetType()==5 and e:GetVariant()==PickupVariant.PICKUP_KEY then out.source_saved.keys=out.source_saved.keys+1 end
      if e:GetType()==6 then out.source_saved.machines=out.source_saved.machines+1 end
    end
  end
  if l:GetCurrentRoomIndex()==source and l:GetDimension()==0 then
    out.live_keys=0
    for _,e in ipairs(Isaac.GetRoomEntities()) do if e.Type==5 and e.Variant==PickupVariant.PICKUP_KEY then out.live_keys=out.live_keys+1 end end
    local g=grid and Game():GetRoom():GetGridEntity(grid)
    out.grid_state=g and g:GetSaveState().State or -1
    for _,e in ipairs(Isaac.GetRoomEntities()) do if e.Type==6 and e.Variant==4 then out.machine_donation=e:ToSlot():GetDonationValue() end end
  end
  emit("actual_location",out)
  local seen={}; local all=PlayerManager.GetPlayers();local cursor=1
  while cursor<=#all and cursor<=64 do
    local p=all[cursor]; cursor=cursor+1
    local id=IsaacCSObservationNative.Subject(p)
    if not seen[id.local_id] then
      seen[id.local_id]=true
      local other=p:GetOtherTwin(); local sub=p:GetSubPlayer()
      if other and not seen[IsaacCSObservationNative.Subject(other).local_id] then all[#all+1]=other end
      if sub and not seen[IsaacCSObservationNative.Subject(sub).local_id] then all[#all+1]=sub end
      emit("actual_player",{subject=id,player_type=p:GetPlayerType(),dead=p:IsDead(),controller=p.ControllerIndex,coins=p:GetNumCoins(),keys=p:GetNumKeys(),bombs=p:GetNumBombs(),hearts=p:GetHearts(),soul=p:GetSoulHearts(),card=p:GetCard(0),pill=p:GetPill(1),trinket=p:GetTrinket(0),charge=p:GetActiveCharge(0),active=p:GetActiveItem(0),sad_onion=p:GetCollectibleNum(CollectibleType.COLLECTIBLE_SAD_ONION),history=#p:GetHistory():GetCollectiblesHistory(),wafer_effect=p:GetEffects():GetCollectibleEffectNum(CollectibleType.COLLECTIBLE_WAFER),moving_box=#p:GetMovingBoxContents()})
    end
  end
  for i=0,3 do local p=PlayerManager.GetEsauJrState(i); emit("actual_backup",{index=i,body=p and IsaacCSObservationNative.Subject(p) or {resolution="Null"}}) end
end
local function transition(index,dimension)
  local old=roomEvent; Game():StartRoomTransition(index,Direction.NO_DIRECTION,0,nil,dimension or 0)
  pending={test=function() return roomEvent>old and Game():GetLevel():GetCurrentRoomIndex()==index and Game():GetLevel():GetDimension()==(dimension or 0) end}
end
local function perform(name)
  local l=Game():GetLevel();local r=Game():GetRoom();local p=Isaac.GetPlayer(0)
  if name=="seed" then Isaac.ExecuteCommand("seed S227 7MNF")
  elseif name=="resume_prepare" then
    prepare();source=l:GetCurrentRoomIndex()
    for slot=0,7 do local door=r:GetDoor(slot);if door and door.TargetRoomIndex>=0 then target=door.TargetRoomIndex;break end end
    assert(target,"resume adjacent target")
    for _,q in ipairs(PlayerManager.GetPlayers()) do q:AddCollectible(CollectibleType.COLLECTIBLE_SAD_ONION,0,false);q:AddCollectible(CollectibleType.COLLECTIBLE_BROTHER_BOBBY,0,false);q:AddCollectible(CollectibleType.COLLECTIBLE_D6,0,false);q:SetActiveCharge(3,0);q:AddCoins(7);q:AddBombs(2);q:AddKeys(3) end
    if config.resume_start=="esau_swap" then primary(3):AddCollectible(CollectibleType.COLLECTIBLE_ESAU_JR,1,false);primary(3):SetActiveCharge(1,0) end
    if config.resume_start=="revive" then primary(3):Kill();pending={test=function()return primary(3):IsCoopGhost() end} end
  elseif name=="initial" then
    prepare();source=l:GetCurrentRoomIndex()
    for slot=0,7 do local door=r:GetDoor(slot);if door and door.TargetRoomIndex>=0 then target=door.TargetRoomIndex;break end end
    assert(target,"adjacent target")
    taken=Isaac.Spawn(5,PickupVariant.PICKUP_KEY,1,r:GetCenterPos()+Vector(-100,40),Vector.Zero,nil):ToPickup()
    remains=Isaac.Spawn(5,PickupVariant.PICKUP_KEY,1,r:GetCenterPos()+Vector(100,40),Vector.Zero,nil):ToPickup()
    machine=Isaac.Spawn(6,4,0,r:GetCenterPos()+Vector(80,-80),Vector.Zero,nil):ToSlot()
    grid=r:GetGridIndex(r:GetCenterPos()+Vector(-100,-60)); if r:GetGridEntity(grid) then r:RemoveGridEntityImmediate(grid,0,false) end
    assert(r:SpawnGridEntity(grid,GridEntityType.GRID_POOP,0,12345,0))
    for _,q in ipairs(PlayerManager.GetPlayers()) do q:AddCollectible(CollectibleType.COLLECTIBLE_BROTHER_BOBBY,0,false) end
  elseif name=="reveal" then
    l:ApplyMapEffect();l:ApplyCompassEffect();l:ApplyBlueMapEffect();l:GetCurrentRoomDesc().DisplayFlags=7;Minimap.SetState(2)
  elseif name=="hide" then
    l:AddCurse(LevelCurse.CURSE_OF_THE_LOST,false);l:GetCurrentRoomDesc().DisplayFlags=0;Minimap.SetState(0)
  elseif name=="mutate" then
    l:RemoveCurses(LevelCurse.CURSE_OF_THE_LOST);machine:SetDonationValue(9);machine:SetTouch(2);r:GetGridEntity(grid):Destroy(false)
    p.Position=taken.Position; pending={test=function() return not taken:Exists() end}
  elseif name=="leave" or name=="box_leave" or name=="twins_leave" or name=="forms_leave" then transition(target,0)
  elseif name=="revisit" or name=="box_return" or name=="twins_return" or name=="forms_return" then transition(source,0)
  elseif name=="box_pack" then
    prepare();p:AddCollectible(CollectibleType.COLLECTIBLE_MOVING_BOX,0,false)
    Isaac.Spawn(5,PickupVariant.PICKUP_BOMB,1,r:GetCenterPos()+Vector(-120,0),Vector.Zero,nil)
    p:UseActiveItem(CollectibleType.COLLECTIBLE_MOVING_BOX,UseFlag.USE_NOANIM)
    pending={test=function() return #p:GetMovingBoxContents()>0 end}
  elseif name=="box_unpack" then p:UseActiveItem(CollectibleType.COLLECTIBLE_MOVING_BOX,UseFlag.USE_NOANIM);pending={test=function()return #p:GetMovingBoxContents()==0 end}
  elseif name=="inventory" then
    prepare()
    for _,q in ipairs(PlayerManager.GetPlayers()) do q:AddCoins(7);q:AddBombs(2);q:AddKeys(3);q:AddSoulHearts(2);q:AddCollectible(CollectibleType.COLLECTIBLE_SAD_ONION,0,false);q:AddTrinket(TrinketType.TRINKET_SWALLOWED_PENNY);q:SetCard(0,Card.CARD_MAGICIAN);q:SetPill(1,1) end
  elseif name=="effects_charge" then
    for _,q in ipairs(PlayerManager.GetPlayers()) do q:AddCollectible(CollectibleType.COLLECTIBLE_D6,0,false);q:SetActiveCharge(3,0);q:GetEffects():AddCollectibleEffect(CollectibleType.COLLECTIBLE_WAFER,false,1) end
  elseif name=="twins" then
    for i=0,3 do primary(i):ChangePlayerType(PlayerType.PLAYER_JACOB) end;prepare()
  elseif name=="forms" then
    primary(0):ChangePlayerType(PlayerType.PLAYER_THEFORGOTTEN);primary(0):GetSubPlayer():AddSoulHearts(4)
    primary(1):ChangePlayerType(PlayerType.PLAYER_THEFORGOTTEN_B)
    primary(2):ChangePlayerType(PlayerType.PLAYER_LAZARUS_B);primary(2):AddCollectible(CollectibleType.COLLECTIBLE_BIRTHRIGHT,0,false)
    primary(3):ChangePlayerType(PlayerType.PLAYER_ISAAC);prepare()
  elseif name=="flip" then primary(2):UseActiveItem(CollectibleType.COLLECTIBLE_FLIP,UseFlag.USE_NOANIM)
  elseif name=="esau_prepare" then primary(3):AddCollectible(CollectibleType.COLLECTIBLE_ESAU_JR,1,false);primary(3):SetActiveCharge(1,0)
  elseif name=="esau_swap" or name=="esau_restore" then
    local q=primary(3);assert(q:GetActiveItem(0)==CollectibleType.COLLECTIBLE_ESAU_JR,"owned Esau Jr slot required")
    local before=IsaacCSObservationNative.Subject(q).local_id
    if name=="esau_swap" then esauOriginal=before end
    q:SetActiveCharge(1,0)
    pending={test=function()
      local root=primary(3,true);if not root then return false end
      local id=IsaacCSObservationNative.Subject(root).local_id
      local hasBackup=false;for i=0,3 do if PlayerManager.GetEsauJrState(i) then hasBackup=true end end
      return id~=before and hasBackup and (name=="esau_swap" or id==esauOriginal)
    end}
  elseif name=="death" then primary(3):Kill();pending={test=function()return primary(3):IsCoopGhost() end}
  elseif name=="revive" then assert(primary(3):ReviveCoopGhost(),"revive false");pending={test=function()return not primary(3):IsDead() end}
  elseif name=="downpour" then
    local before=levelEvent;Isaac.ExecuteCommand("stage 2c")
    local preparedAt
    pending={test=function()
      local level=Game():GetLevel();if levelEvent<=before or not level:HasMirrorDimension() then return false end
      if not preparedAt then
        -- 紧凑Grid编码的受影响验证随镜像往返完成，不重跑旧地图/库存等已通过项。
        prepare();source=level:GetCurrentRoomIndex();local room=Game():GetRoom();grid=room:GetGridIndex(room:GetCenterPos()+Vector(-100,-60))
        if room:GetGridEntity(grid) then room:RemoveGridEntityImmediate(grid,0,false) end
        assert(room:SpawnGridEntity(grid,GridEntityType.GRID_POOP,0,13579,0));room:GetGridEntity(grid):Destroy(false);preparedAt=render
      end
      return render-preparedAt>=30
    end}
  elseif name=="mirror" then transition(l:GetStartingRoomIndex(),1)
  elseif name=="normal" then transition(l:GetStartingRoomIndex(),0)
  elseif name=="myosotis" then prepare();p:AddTrinket(TrinketType.TRINKET_MYOSOTIS);Isaac.Spawn(5,PickupVariant.PICKUP_COIN,1,r:GetCenterPos()+Vector(-100,0),Vector.Zero,nil)
  elseif name=="next_floor" then local before=levelEvent;Isaac.ExecuteCommand("stage 3");pending={test=function() return levelEvent>before end}
  elseif name=="query" then
    local before=p:GetDropRNG():GetSeed();local result=IsaacCSPlayerObservation.Query("PlayerManager","AnyoneHasCollectible",{CollectibleType.COLLECTIBLE_SAD_ONION,false})
    assert(result==true,"actual owner query");assert(before==p:GetDropRNG():GetSeed(),"query advanced RNG");emit("query_actual",{before=before,after=p:GetDropRNG():GetSeed()})
  elseif name=="disconnect_ready" then prepare()
  else error("unknown action:"..tostring(name)) end
end
mod:AddCallback(ModCallbacks.MC_POST_NEW_ROOM,function() roomEvent=roomEvent+1 end)
mod:AddCallback(ModCallbacks.MC_POST_NEW_LEVEL,function() levelEvent=levelEvent+1 end)
mod:AddCallback(ModCallbacks.MC_POST_GAME_STARTED,function()running=true end)
mod:AddCallback(ModCallbacks.MC_PRE_GAME_EXIT,function()running=false end)
mod:AddCallback(ModCallbacks.MC_POST_RENDER,function()
  render=render+1;if not running or Game():GetNumPlayers()==0 then return end
  if Input.IsButtonTriggered(Keyboard.KEY_F8,0) then
    action=action+1;local name=actions[action];emit("action_begin")
    local ok,err=pcall(perform,name)
    if not ok then pending=nil;emit("fixture_error",{error=tostring(err)})
    else pending=pending or {};pending.start=render;emit("action_return") end
  end
  if pending and render-pending.start>=45 then
    local ok,ready=pcall(function()return not pending.test or pending.test() end)
    if not ok then pending=nil;emit("fixture_error",{error=tostring(ready)})
    elseif ready then pending=nil;local copied,err=pcall(actual);if not copied then emit("fixture_error",{error=tostring(err)}) else emit("ready") end
    elseif render-pending.start>1800 then pending=nil;emit("fixture_error",{error="actual_result_timeout"}) end
  end
end)
