-- tx-04-12统一E2E玩法夹具；正式room/level采集器不调用任何这些修改接口。
local mod=RegisterMod("Isaac CS Room Level E2E",1)
local config=include("harness_config"); local json=require("json")
local action,serial,render=0,0,0
local running=false; local origin; local originDimension
local special=include("tx12_special")
local actions={"seed","initial","progress_dirty","progress_clear","environment","queries","leave","revisit","shape2","shapeL","sacrifice","shop","challenge","downpour","mirror","normal","mines","death_certificate","death_return","ascent"}
-- 续测仅执行未完成/受修复影响项；原回归结果由harness读取先前正式录制统一归约。
if config.resume then actions={"seed"} end
local function emit(event,data)
 serial=serial+1; data=data or {}; data.event=event; data.token=config.token; data.action=action; data.serial=serial; data.render=render
 Isaac.DebugString("[isaac-cs-test] tx12 "..json.encode(data))
end
-- 一个入口顺序执行原回归和完整覆盖清单；每项退出/回访单独保留实际结果。
local coverage=special.Start(emit)
mod:AddCallback(ModCallbacks.MC_POST_NEW_ROOM,special.NewRoom)
mod:AddCallback(ModCallbacks.MC_POST_NEW_LEVEL,special.NewLevel)
actions[#actions+1]="coverage_reset"
for _,c in ipairs(coverage) do
 if not config.pending or config.pending[c.id] then
 actions[#actions+1]="case:"..c.id
 if c.return_required then actions[#actions+1]="return:"..c.id end
 if c.reenter_required then actions[#actions+1]="reenter:"..c.id; actions[#actions+1]="return2:"..c.id end
 end
end
actions[#actions+1]="coverage_finish"
local function actual(event)
 local l=Game():GetLevel(); local r=Game():GetRoom(); local d=l:GetCurrentRoomDesc()
 emit(event or "actual",{room=l:GetCurrentRoomIndex(),stage=l:GetStage(),stage_type=l:GetStageType(),dimension=l:GetDimension(),list=d.ListIndex,grid=d.GridIndex,safe=d.SafeGridIndex,shape=r:GetRoomShape(),type=r:GetType(),clear=r:IsClear(),clear_count=d.ClearCount,sacrifice=r:IsSacrificeDone(),water=r:GetWaterAmount(),current={r:GetWaterCurrent().X,r:GetWaterCurrent().Y},ascent=l:IsAscent(),wave=l.GreedModeWave})
end
local function prepare()
 for _,e in ipairs(Isaac.GetRoomEntities()) do if e:ToNPC() then e:Remove() end end
 for _,p in ipairs(PlayerManager.GetPlayers()) do p.Position=Game():GetRoom():GetCenterPos(); p.Velocity=Vector.Zero; p:SetMinDamageCooldown(18000) end
end
local function debugRoom(typ,shape)
 local target=RoomConfig.GetRandomRoom(12345,false,(typ==RoomType.ROOM_DEFAULT and Isaac.GetCurrentStageConfigId() or StbType.SPECIAL_ROOMS),typ,shape or RoomShape.NUM_ROOMSHAPES)
 assert(target,"fixture config unavailable:"..typ)
 emit("config_request",{type=target.Type,shape=target.Shape,variant=target.Variant})
 local prefix=typ==RoomType.ROOM_DEFAULT and "d" or typ==RoomType.ROOM_SACRIFICE and "s.sacrifice" or typ==RoomType.ROOM_SHOP and "s.shop" or "s.challenge"
 Isaac.ExecuteCommand("goto "..prefix.."."..target.Variant)
end
local function perform(name)
 local l=Game():GetLevel(); local r=Game():GetRoom(); local d=l:GetCurrentRoomDesc()
 if name=="seed" then Isaac.ExecuteCommand("seed S227 7MNF")
 elseif name=="initial" then prepare(); origin=l:GetCurrentRoomIndex(); originDimension=l:GetDimension(); actual("initial")
 elseif name=="progress_dirty" then
  prepare(); r:SetClear(false); r:SetAmbushDone(false); r:SetSacrificeDone(false); r:SetFirstEnemyDead(false)
  d.NoReward=true; d.ChallengeDone=false; d.ShopItemIdx=7; d.ShopItemDiscountIdx=3; d.PressurePlatesTriggered=false
  Isaac.Spawn(EntityType.ENTITY_GAPER,0,0,r:GetCenterPos()+Vector(100,0),Vector.Zero,nil); actual("progress_dirty")
 elseif name=="progress_clear" then
  prepare(); r:SetClear(false); d.NoReward=false; r:SetAmbushDone(true); r:SetSacrificeDone(true); r:SetFirstEnemyDead(true)
  d.PressurePlatesTriggered=true; d.ChallengeDone=true; r:TriggerClear(true); actual("progress_clear")
 elseif name=="environment" then
  prepare(); r:SetWaterAmount(0.75); r:SetWaterCurrent(Vector(1.25,-0.5)); r:SetWaterColor(KColor(0.2,0.4,0.6,0.8)); r:SetWaterColorMultiplier(KColor(0.8,0.7,0.6,0.5))
  r:SetFloorColor(Color(0.7,0.8,0.9,1,0,0,0)); r:SetWallColor(Color(0.9,0.7,0.8,1,0,0,0))
  local fx=r:GetFXParams(); fx.ShadowAlpha=0.375; fx.LightColor=KColor(0.3,0.4,0.5,0.6); fx.UseWaterV2=true
  r:SetRainIntensity(0.45); r:SetLightningIntensity(0.25); r:SetLavaIntensity(0.125); actual("environment")
 elseif name=="queries" then
  local before=l:GetGenerationRNG():GetSeed()
  IsaacCSRoomObservation.Query(r,"Room","IsPositionInRoom",{r:GetCenterPos(),0})
  IsaacCSLevelObservation.Query(l,"Level","CanSpawnDoorOutline",{d.GridIndex,DoorSlot.LEFT0})
  assert(before==l:GetGenerationRNG():GetSeed(),"query advanced generation RNG")
  emit("query_actual",{before=before,after=l:GetGenerationRNG():GetSeed()})
 elseif name=="leave" then
  for slot=0,7 do local door=r:GetDoor(slot); if door and door.TargetRoomIndex>=0 then door:SetLocked(false); door:Open(); Game():StartRoomTransition(door.TargetRoomIndex,door.Direction,0,nil,-1); emit("leave"); return end end
  error("no actual adjacent door")
 elseif name=="revisit" then Game():StartRoomTransition(origin,Direction.NO_DIRECTION,0,nil,originDimension)
 elseif name=="shape2" then debugRoom(RoomType.ROOM_DEFAULT,RoomShape.ROOMSHAPE_2x2)
 elseif name=="shapeL" then debugRoom(RoomType.ROOM_DEFAULT,RoomShape.ROOMSHAPE_LTL)
 elseif name=="sacrifice" then debugRoom(RoomType.ROOM_SACRIFICE)
 elseif name=="shop" then debugRoom(RoomType.ROOM_SHOP)
 elseif name=="challenge" then debugRoom(RoomType.ROOM_CHALLENGE)
 elseif name=="downpour" then Isaac.ExecuteCommand("stage 2c")
 elseif name=="mirror" then
  assert(l:HasMirrorDimension(),"mirror dimension absent")
  l:ChangeRoom(l:GetStartingRoomIndex(),1)
 elseif name=="normal" then l:ChangeRoom(l:GetStartingRoomIndex(),0)
 elseif name=="mines" then Isaac.ExecuteCommand("stage 4c")
 elseif name=="death_certificate" then
  prepare(); Isaac.GetPlayer(0):UseActiveItem(CollectibleType.COLLECTIBLE_DEATH_CERTIFICATE,UseFlag.USE_NOANIM|UseFlag.USE_NOCOSTUME)
 elseif name=="death_return" then l:ChangeRoom(l:GetStartingRoomIndex(),0)
 elseif name=="ascent" then Game():SetStateFlag(GameStateFlag.STATE_BACKWARDS_PATH,true); Isaac.ExecuteCommand("stage 3")
 elseif (name=="coverage_reset" or name=="coverage_finish") or name:find(":",1,true) then special.Perform(name)
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
 local ok,err=pcall(special.Update); if not ok then emit("fixture_error",{name="coverage_update",error=tostring(err)}) end
 if render%30==0 then actual() end
end)
