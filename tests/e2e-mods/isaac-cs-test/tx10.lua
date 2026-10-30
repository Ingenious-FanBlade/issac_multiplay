-- tx-04-10 显式玩法夹具；正式观测模块不包含这些世界写入。
local fixture=RegisterMod("Isaac CS Entity observation E2E",1)
local config=include("harness_config")
local json=require("json")
local serial,action,render=0,0,0
local run=false
local objects,slots={},{}
local npc,pickup,chest,fireplace,shopkeeper,gridIndex,interacting
local actions={"seed","all_classes","appearance_and_morph","short_lived","slot_interaction","stop_interaction","grid_create","grid_replace","clear","four_twins","explicit_queries","room"}
local function emit(event,data)
  serial=serial+1; data=data or {}; data.event=event; data.token=config.token; data.serial=serial; data.action=action; data.render=render
  Isaac.DebugString("[isaac-cs-test] tx10 "..json.encode(data))
end
local function keep(e) assert(e,"spawn returned nil"); objects[#objects+1]=e; return e end
local function player() return Isaac.GetPlayer(0) end
local function stable(e)
  e.Velocity=Vector.Zero
  if e:ToNPC() then e:AddEntityFlags(EntityFlag.FLAG_FREEZE|EntityFlag.FLAG_NO_TARGET) end
  return keep(e)
end
local function summary(e)
  return {subject=assert(IsaacCSObservationNative).Subject(e),type=e.Type,variant=e.Variant,subtype=e.SubType,index=e.Index,seed=e.InitSeed,exists=e:Exists()}
end
local function resources()
  local center=Game():GetRoom():GetCenterPos()
  for i,p in ipairs(PlayerManager.GetPlayers()) do
    p.Position=center+Vector((i-1)*12,0); p.Velocity=Vector.Zero
    p:AddCoins(99-p:GetNumCoins()); p:AddBombs(20-p:GetNumBombs()); p:SetMinDamageCooldown(1800)
  end
  for slot=0,7 do local door=Game():GetRoom():GetDoor(slot); if door then door:Close(true) end end
end
local function allClasses()
  resources()
  for _,e in ipairs(Isaac.GetRoomEntities()) do if e:ToNPC() and e:IsVulnerableEnemy() then e:Remove() end end
  local p=player(); local center=Game():GetRoom():GetCenterPos()
  p:AddCollectible(CollectibleType.COLLECTIBLE_MOMS_KNIFE,0,false)
  npc=stable(Isaac.Spawn(EntityType.ENTITY_GAPER,0,0,center+Vector(-80,-35),Vector.Zero,p)):ToNPC()
  pickup=keep(Isaac.Spawn(EntityType.ENTITY_PICKUP,PickupVariant.PICKUP_COIN,1,center+Vector(-100,50),Vector.Zero,p)):ToPickup()
  stable(Isaac.Spawn(EntityType.ENTITY_FAMILIAR,FamiliarVariant.BROTHER_BOBBY,0,center+Vector(30,-70),Vector.Zero,p))
  chest=keep(Isaac.Spawn(EntityType.ENTITY_PICKUP,PickupVariant.PICKUP_CHEST,0,center+Vector(80,35),Vector.Zero,p)):ToPickup()
  fireplace=stable(Isaac.Spawn(EntityType.ENTITY_FIREPLACE,0,0,center+Vector(-120,-50),Vector.Zero,nil)):ToNPC()
  shopkeeper=stable(Isaac.Spawn(EntityType.ENTITY_SHOPKEEPER,0,0,center+Vector(120,-50),Vector.Zero,nil)):ToNPC()
  local tear=keep(p:FireTear(center+Vector(-20,-30),Vector.Zero,false,true,false)); tear:AddTearFlags(TearFlags.TEAR_SPECTRAL); tear.Height=-40; tear.FallingAcceleration=0; tear.FallingSpeed=0; tear.EntityCollisionClass=EntityCollisionClass.ENTCOLL_NONE; tear.GridCollisionClass=EntityGridCollisionClass.GRIDCOLL_NONE
  local projectile=keep(Isaac.Spawn(EntityType.ENTITY_PROJECTILE,0,0,center+Vector(100,-70),Vector.Zero,npc)):ToProjectile(); projectile.FallingAccel=0; projectile.FallingSpeed=0; projectile.Height=-15
  local bomb=keep(p:FireBomb(center+Vector(110,50),Vector.Zero)); bomb:SetExplosionCountdown(1800)
  local laser=keep(EntityLaser.ShootAngle(2,center+Vector(0,70),0,1800,Vector.Zero,p)); laser.MaxDistance=40; laser.CollisionDamage=0
  local knife=keep(p:FireKnife(p,20,true,0,0)); knife.RotationOffset=20
  local effect=keep(Isaac.Spawn(EntityType.ENTITY_EFFECT,EffectVariant.HALO,0,center,Vector.Zero,p)):ToEffect(); effect.Timeout=1800
  stable(Isaac.Spawn(EntityType.ENTITY_DELIRIUM,0,0,center+Vector(100,0),Vector.Zero,p))
  for i,variant in ipairs({1,4,6,9,13,15,16,17,18}) do
    local e=keep(Isaac.Spawn(EntityType.ENTITY_SLOT,variant,0,center+Vector(-150+(i-1)*35,100),Vector.Zero,nil)):ToSlot()
    assert(e,"slot conversion"); slots[#slots+1]=e
  end
  keep(Isaac.Spawn(9001,0,0,center+Vector(0,-80),Vector.Zero,p))
  -- 每个主体独立一行，避免游戏单条 DebugString 长度截断。
  for _,e in ipairs(objects) do emit("class_request",summary(e)) end
end
local function perform(name)
  if name=="seed" then Isaac.ExecuteCommand("seed S227 7MNF")
  elseif name=="all_classes" then allClasses()
  elseif name=="appearance_and_morph" then
    assert(npc and npc:Exists()); local before=summary(npc); npc:Morph(EntityType.ENTITY_HOPPER,0,0,-1); assert(npc.Type==EntityType.ENTITY_HOPPER,"actual Morph type"); emit("npc_morph",{before=before,after=summary(npc)})
    local pickupBefore=summary(pickup); pickup:Morph(EntityType.ENTITY_PICKUP,PickupVariant.PICKUP_KEY,1,true,true,true); emit("pickup_morph",{before=pickupBefore,after=summary(pickup)})
    for _,e in ipairs(objects) do if e:Exists() then local s=e:GetSprite(); s.Rotation=37; s.Scale=Vector(1.15,0.85); s.FlipX=true; s.PlaybackSpeed=0.6 end end
    if chest and chest:Exists() then chest:TryOpenChest(player()) end
    if fireplace and fireplace:Exists() then fireplace:Kill() end
    if shopkeeper and shopkeeper:Exists() then shopkeeper:Kill() end
    local p=player(); p:AddCollectible(CollectibleType.COLLECTIBLE_SAD_ONION,0,false); p:GetEffects():AddCollectibleEffect(CollectibleType.COLLECTIBLE_WAFER,false,1)
  elseif name=="short_lived" then
    local e=Isaac.Spawn(EntityType.ENTITY_TEAR,0,0,player().Position,Vector.Zero,player()); local before=summary(e); e:Remove(); emit("short_lived",{object=before,exists_after=e:Exists()})
  elseif name=="slot_interaction" then
    for _,s in ipairs(slots) do if s.Variant==4 and s:Exists() then interacting=s; player().Position=s.Position; emit("slot_contact_begin",{slot=summary(s),coins=player():GetNumCoins(),donation=s:GetDonationValue()}); break end end
    assert(interacting,"beggar missing")
  elseif name=="stop_interaction" then interacting=nil; resources()
  elseif name=="grid_create" then
    local room=Game():GetRoom(); gridIndex=room:GetGridIndex(room:GetCenterPos()+Vector(120,-80))
    if room:GetGridEntity(gridIndex) then room:RemoveGridEntityImmediate(gridIndex,0,false) end
    assert(room:SpawnGridEntity(gridIndex,GridEntityType.GRID_ROCK,0,123,0)); emit("grid_created",{index=gridIndex})
  elseif name=="grid_replace" then
    local room=Game():GetRoom(); assert(room:GetGridEntity(gridIndex)); room:RemoveGridEntityImmediate(gridIndex,0,false)
    assert(room:SpawnGridEntity(gridIndex,GridEntityType.GRID_POOP,0,456,0)); emit("grid_replaced",{index=gridIndex})
  elseif name=="clear" then
    interacting=nil; for _,e in ipairs(objects) do if e:Exists() then e:Remove() end end; objects={}; slots={}; resources()
  elseif name=="four_twins" then
    for _,p in ipairs(PlayerManager.GetPlayers()) do if p:GetPlayerType()~=PlayerType.PLAYER_ESAU then p:ChangePlayerType(PlayerType.PLAYER_JACOB) end end
    resources()
  elseif name=="explicit_queries" then
    local p=player(); local r=assert(IsaacCSObservation).Query(p,"EntityPlayer","HasCollectible",{CollectibleType.COLLECTIBLE_SAD_ONION,false,false})
    emit("explicit_query",{result=r,subject=IsaacCSObservationNative.Subject(p)})
    local before=p:GetDropRNG():GetSeed(); local _,complete=IsaacCSObservation.Capture(); local after=p:GetDropRNG():GetSeed()
    emit("readonly_rng",{before=before,after=after,complete=complete}); assert(before==after,"observer advanced Player drop RNG")
  elseif name=="room" then
    local room=Game():GetRoom()
    for slot=0,7 do local door=room:GetDoor(slot); if door and door.TargetRoomIndex>=0 then Game():StartRoomTransition(door.TargetRoomIndex,door.Direction,0,nil,-1); return end end
    error("no adjacent room")
  else error("unexpected action:"..tostring(name)) end
end
fixture:AddCallback(ModCallbacks.MC_POST_GAME_STARTED,function() run=true end)
fixture:AddCallback(ModCallbacks.MC_PRE_GAME_EXIT,function() run=false end)
fixture:AddCallback(ModCallbacks.MC_POST_UPDATE,function()
  if run and interacting and interacting:Exists() then player().Position=interacting.Position; player().Velocity=Vector.Zero end
end)
fixture:AddCallback(ModCallbacks.MC_POST_RENDER,function()
  render=render+1; if not run or Game():GetNumPlayers()==0 then return end
  if Input.IsButtonTriggered(Keyboard.KEY_F8,0) then
    action=action+1; local name=actions[action]; emit("action_begin",{name=name})
    local ok,err=pcall(perform,name); emit(ok and "action_return" or "fixture_error",{name=name,error=not ok and tostring(err) or nil})
  end
  if render%30==0 then
    for _,s in ipairs(slots) do if s:Exists() then emit("slot_observation",{subject=IsaacCSObservationNative.Subject(s),variant=s.Variant,donation=s:GetDonationValue(),state=s:GetState(),touch=s:GetTouch(),prize=s:GetPrizeType(),prize_collectible=s:GetPrizeCollectible()}) end end
    emit("observation",{players=Game():GetNumPlayers(),coins=player():GetNumCoins()})
  end
end)
