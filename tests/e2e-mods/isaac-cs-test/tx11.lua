-- tx-04-11 唯一整体验收夹具：世界修改只在此发生，正式观察器保持只读。
local mod=RegisterMod("Isaac CS Grid observation E2E",1)
local config=include("harness_config")
local json=require("json")
local action,serial,render=0,0,0
local running=false
local indices,originalRoom,originalShape,replaceIndex={},nil,nil,nil
local actions={"seed","all_grids","appearance","states","destroy","replace_old","replace_new","queries","lifted","leave","revisit","shape","shape_l"}
local function emit(event,data)
  serial=serial+1; data=data or {}; data.event=event; data.token=config.token; data.action=action; data.serial=serial; data.render=render
  Isaac.DebugString("[isaac-cs-test] tx11 "..json.encode(data))
end
local function facts(g)
  local d=g:GetSaveState()
  local result={subject=IsaacCSObservationNative.GridSubject(g),type=g:GetType(),variant=g:GetVariant(),index=g:GetGridIndex(),state=g.State,var_data=g.VarData,spawn_seed=d.SpawnSeed,variable_seed=d.VariableSeed,initialized=d.Initialized,collision=g.CollisionClass}
  for _,name in ipairs({"Decoration","Door","Fire","Gravity","Lock","Pit","Poop","PressurePlate","Rock","Spikes","Stairs","Statue","Teleporter","TNT","TrapDoor","Wall","Web"}) do
    if g["To"..name](g) then result.class="GridEntity"..name end
  end
  return result
end
local function room() return Game():GetRoom() end
local function prepare()
  for _,e in ipairs(Isaac.GetRoomEntities()) do if e:ToNPC() then e:Remove() end end
  for _,p in ipairs(PlayerManager.GetPlayers()) do p.Position=room():GetGridPosition(17); p.Velocity=Vector.Zero; p:SetMinDamageCooldown(18000) end
end
local function spawn(index,typ,variant,seed,varData)
  if room():GetGridEntity(index) then room():RemoveGridEntityImmediate(index,0,false) end
  local accepted=room():SpawnGridEntity(index,typ,variant,seed,varData)
  local g=room():GetGridEntity(index)
  emit("grid_spawn_request",{requested={index=index,type=typ,variant=variant,seed=seed,var_data=varData},accepted=accepted,actual=g and facts(g)})
  assert(accepted and g,"actual grid spawn failed:"..typ)
  assert(g:GetType()==typ,"actual grid type differs:"..typ)
  return g
end
local function perform(name)
  if name=="seed" then Isaac.ExecuteCommand("seed S227 7MNF")
  elseif name=="all_grids" then
    prepare(); originalRoom=Game():GetLevel():GetCurrentRoomIndex(); originalShape=room():GetRoomShape()
    local slot=0
    for typ=1,27 do if typ~=16 then
      slot=slot+1; local index=room():GetGridIndex(Vector(160+((slot-1)%8)*40,200+math.floor((slot-1)/8)*40))
      indices[typ]=index; spawn(index,typ,0,11000+typ,0)
    end end
    local doors=0
    for slot=0,7 do local d=room():GetDoor(slot); if d then doors=doors+1; emit("door_initial",facts(d)) end end
    assert(doors>0,"no actual doors")
  elseif name=="appearance" then
    for typ,index in pairs(indices) do
      local g=assert(room():GetGridEntity(index)); local s=g:GetSprite(); s.Rotation=31; s.FlipX=true; s.Scale=Vector(1.1,0.9)
      g:SetWaterClipFlags(57); emit("appearance_actual",facts(g))
    end
  elseif name=="states" then
    local pit=assert(room():GetGridEntity(indices[7])):ToPit(); pit:MakeBridge(room():GetGridEntity(indices[2])); pit:SetLadder(true); emit("bridge_actual",facts(pit))
    local poop=assert(room():GetGridEntity(indices[14])); poop:Hurt(1); emit("poop_hurt_actual",facts(poop))
    for slot=0,7 do local d=room():GetDoor(slot); if d then d:SetLocked(true); d:Close(true); emit("door_locked_actual",facts(d)) end end
    local plate=assert(room():GetGridEntity(indices[20])); local p=Isaac.GetPlayer(0); p.Position=plate.Position; p.Velocity=Vector.Zero; emit("plate_contact",facts(plate))
  elseif name=="destroy" then
    prepare()
    for _,typ in ipairs({2,14,12}) do local g=assert(room():GetGridEntity(indices[typ])); local before=facts(g); local result=g:Destroy(false); emit("destroy_actual",{before=before,result=result,after=facts(g)}) end
    for slot=0,7 do local d=room():GetDoor(slot); if d then d:SetLocked(false); d:Open(); emit("door_open_actual",facts(d)) end end
  elseif name=="replace_old" then
    replaceIndex=room():GetGridIndex(Vector(480,360)); local g=spawn(replaceIndex,2,0,42001,23); emit("replace_old",facts(g))
  elseif name=="replace_new" then
    local before=facts(assert(room():GetGridEntity(replaceIndex))); room():RemoveGridEntityImmediate(replaceIndex,0,false)
    assert(not room():GetGridEntity(replaceIndex),"immediate removal did not clear slot")
    local g=spawn(replaceIndex,14,0,42002,25); emit("replace_new",{before=before,after=facts(g)})
    -- 同一回调内生灭，必须由真实生命周期而非普通快照观察到。
    local temp=room():GetGridIndex(Vector(520,360)); local short=spawn(temp,2,0,42003,0); local subject=facts(short)
    room():RemoveGridEntityImmediate(temp,0,false); emit("short_grid",subject)
  elseif name=="queries" then
    local g=assert(room():GetGridEntity(replaceIndex)); local before=g:GetRNG():GetSeed()
    local _,complete=IsaacCSGridObservation.Capture(); local after=g:GetRNG():GetSeed()
    assert(before==after,"observer advanced Grid RNG"); emit("readonly_rng",{before=before,after=after,complete=complete})
    for slot=0,7 do local d=room():GetDoor(slot); if d then IsaacCSGridObservation.Query(d,"GridEntityDoor","IsRoomType",{RoomType.ROOM_DEFAULT}); break end end
    IsaacCSGridObservation.Query(g,"GridEntity","GetSaveState",{})
  elseif name=="lifted" then
    local index=indices[24]; local g=spawn(index,2,0,43001,37); local before=facts(g)
    local effect=room():PickupGridEntity(index); assert(effect,"pickup grid returned nil")
    local result=IsaacCSGridObservation.Query(effect,"EntityEffect","GetGridEntityDesc",{})
    emit("lifted_actual",{before=before,effect=IsaacCSObservationNative.Subject(effect),descriptor=result})
  elseif name=="leave" then
    for slot=0,7 do local d=room():GetDoor(slot); if d and d.TargetRoomIndex>=0 then
      d:SetLocked(false); d:Open(); emit("leave_request",{from=originalRoom,target=d.TargetRoomIndex,shape=room():GetRoomShape()}); Game():StartRoomTransition(d.TargetRoomIndex,d.Direction,0,nil,-1); return
    end end
    error("no adjacent room")
  elseif name=="revisit" then
    emit("revisit_request",{from=Game():GetLevel():GetCurrentRoomIndex(),target=originalRoom}); Game():StartRoomTransition(originalRoom,Direction.NO_DIRECTION,0,nil,-1)
  elseif name=="shape" or name=="shape_l" then
    -- 从当前楼层实际描述中选不同形状，缺席时使用公开调试命令的测试房。
    local rooms=Game():GetLevel():GetRooms()
    for i=0,rooms.Size-1 do local d=rooms:Get(i); if name=="shape" and d.Data and d.Data.Shape==RoomShape.ROOMSHAPE_2x2 then
      emit("shape_request",{target=d.SafeGridIndex,shape=d.Data.Shape}); Game():StartRoomTransition(d.SafeGridIndex,Direction.NO_DIRECTION,0,nil,-1); return
    end end
    local target=RoomConfig.GetRandomRoom(112233,false,Isaac.GetCurrentStageConfigId(),RoomType.ROOM_DEFAULT,name=="shape" and RoomShape.ROOMSHAPE_2x2 or RoomShape.ROOMSHAPE_LTL)
    assert(target,"different-shape config unavailable")
    emit("shape_debug_request",{variant=target.Variant,shape=target.Shape}); Isaac.ExecuteCommand("goto d."..target.Variant)
  else error("unknown action:"..tostring(name)) end
end
mod:AddCallback(ModCallbacks.MC_POST_GAME_STARTED,function() running=true end)
mod:AddCallback(ModCallbacks.MC_PRE_GAME_EXIT,function() running=false end)
mod:AddCallback(ModCallbacks.MC_POST_RENDER,function()
  render=render+1; if not running or Game():GetNumPlayers()==0 then return end
  if Input.IsButtonTriggered(Keyboard.KEY_F8,0) then
    action=action+1; local name=actions[action]; emit("action_begin",{name=name})
    local ok,err=pcall(perform,name); emit(ok and "action_return" or "fixture_error",{name=name,error=not ok and tostring(err) or nil})
  end
  if render%30==0 then emit("room_actual",{room=Game():GetLevel():GetCurrentRoomIndex(),shape=room():GetRoomShape(),grid_size=room():GetGridSize()}) end
end)
