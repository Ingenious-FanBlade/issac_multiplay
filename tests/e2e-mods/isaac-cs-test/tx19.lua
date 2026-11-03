-- I33失败模型先见tx-04-19-e2e-integration.md；只写Source，Client走正式parser/Scene。
local mod=RegisterMod("Isaac CS full Entity application E2E",1)
IsaacCSClientTestMod=mod
local config=include("harness_config");local json=require("json")
local scenes=json.decode(config.scenes_json)
local action,frames=0,0;local pending,capture,held=nil,nil,false
local damageDebugAdded=false
local function restoreDamageDebug()
 if damageDebugAdded and (Game():GetDebugFlags() & DebugFlag.DAMAGE_VALUES)~=0 then Isaac.ExecuteCommand("debug 7") end
 damageDebugAdded=false
end
local original={};local tracked={};local slotResults={};local sourceReady=false;local liftedIndex
local function emit(event,data)
 data=data or {};data.event=event;data.token=config.token;data.action=action
 Isaac.DebugString("[isaac-cs-test] tx19 "..json.encode(data))
end
local function player() return Isaac.GetPlayer(0) end
local function center() return Game():GetRoom():GetCenterPos() end
local function keep(e)
 assert(e,"Source spawn failed");tracked[#tracked+1]=e;e.EntityCollisionClass=0;e.GridCollisionClass=0
 return e
end
local function spawn(t,v,s,offset,owner)
 return keep(Isaac.Spawn(t,v or 0,s or 0,center()+(offset or Vector.Zero),Vector.Zero,owner))
end
local function clear()
 -- 显式clear场景才产生删除；就绪/导出期间绝不筛掉待测或环境实体。
 for _,e in ipairs(Isaac.GetRoomEntities()) do if not e:ToPlayer() then e:Remove() end end
 tracked={};local p=player();p.Position=center()+Vector(0,100);p.Velocity=Vector.Zero
 p.EntityCollisionClass=0;p.GridCollisionClass=0;p:SetMinDamageCooldown(18000)
end
local function familiars()
 for i,v in ipairs({1,8,48,68,78,231,243}) do local f=spawn(3,v,0,Vector(-110+(i-1)*32,-20),player()):ToFamiliar();f.Player=player() end
end
local function perform(s)
 if s.kind=="setup" then
  Isaac.ExecuteCommand("stage 1");return
 elseif s.kind=="clear" then clear();return
 elseif s.kind=="npc" then
  clear();local n=spawn(10,0,0,Vector(-70,-25)):ToNPC();n:MakeChampion(191919,0,true)
  local child=spawn(10,0,0,Vector(30,-25)):ToNPC();n.Child=child;child.Parent=n
 elseif s.kind=="delirium" then clear();spawn(EntityType.ENTITY_DELIRIUM,0,0,Vector(60,-20))
 elseif s.kind=="delirium-morph" then
  local d=assert(tracked[1]:ToDelirium());d:SetRedMode(true);d:SetTeleportationTimer(120);d:Transform(EntityType.ENTITY_MONSTRO,0)
 elseif s.kind=="tear-projectile" then
  clear();local p=player();local t=keep(p:FireTear(center()+Vector(-50,0),Vector.Zero,false,true,false))
  t.Height=-32;t.FallingSpeed=0;t.FallingAcceleration=0;t:AddTearFlags(TearFlags.TEAR_SPECTRAL)
  local q=spawn(9,0,0,Vector(70,0),nil):ToProjectile();q.Height=-20;q.FallingSpeed=0;q.FallingAccel=0
 elseif s.kind=="bomb" then clear();local b=keep(player():FireBomb(center(),Vector.Zero));b:SetExplosionCountdown(1800)
 elseif s.kind=="laser" then
  clear();local p=player();local l
  if s.mode=="circle" then l=keep(p:FireTechXLaser(center(),Vector.Zero,55,p,1))
  else l=keep(EntityLaser.ShootAngle(2,center()+Vector(-80,0),15,1800,Vector.Zero,p)) end
  l.CollisionDamage=0;l.MaxDistance=160;l:SetDisableFollowParent(true)
  if s.mode=="circle" then l.Radius=55
  elseif s.mode=="curve" then l.CurveStrength=2;l:SetHomingType(1)
  elseif s.mode=="sample" then l:SetHomingType(2);l.CurveStrength=1
  elseif s.mode=="chain" then local n=spawn(10,0,0,Vector(90,0));l.Parent=n;local b=keep(EntityLaser.ShootAngle(2,n.Position,140,1800,Vector.Zero,p));l.BounceLaser=b end
 elseif s.kind=="knife" then clear();keep(player():FireKnife(player(),20,true,0,0))
 elseif s.kind=="knife-out" then assert(tracked[1]:ToKnife()):Shoot(1,140)
 elseif s.kind=="knife-return" then assert(tracked[1]:ToKnife()):Shoot(0,0)
 elseif s.kind=="pickup" then
  clear();local p=spawn(5,100,1,Vector.Zero):ToPickup();p.Price=15;p.ShopItemId=0;p.OptionsPickupIndex=19
 elseif s.kind=="pickup-cycle" then local p=assert(tracked[1]:ToPickup());p:AddCollectibleCycle(2);p:AddCollectibleCycle(3)
 elseif s.kind=="pickup-morph" then assert(tracked[1]:ToPickup()):Morph(5,20,1,true,true,false)
 elseif s.kind=="familiar" then
  clear();familiars()
 elseif s.kind=="familiar-owner" then
  -- 用真实run退休/新Player Key验证owner换代；不把同一Player重复赋值当成owner变化。
  Isaac.ExecuteCommand("restart")
 elseif s.kind=="effects" then
  clear();for i,v in ipairs({22,57,108,166,191}) do local e=spawn(1000,v,0,Vector(-100+(i-1)*45,0),player()):ToEffect();e.Timeout=1800 end
 elseif s.kind=="slot" then
  clear();local slot=spawn(6,s.variant,0,Vector.Zero):ToSlot();assert(slot,"real EntitySlot missing");slot:SetTimeout(1800);slotResults[s.variant]=true
 elseif s.kind=="slot-results" then
  clear();for i,v in ipairs({1,6,15,16}) do local e=spawn(6,v,0,Vector(-90+(i-1)*60,0)):ToSlot();e:SetPrizeType(v==1 and 7 or 20);e:SetTimeout(1800);if v==15 or v==16 then e:SetPrizeCollectible(1) end end
  -- 设置已裁决结果，不调用捐款、碰撞或支付，不触发真实持久进度。
  spawn(5,20,1,Vector(0,45));spawn(1000,57,0,Vector(25,45))
 elseif s.kind=="slot-destroyed" then for _,e in ipairs(tracked) do local x=e:ToSlot();if x then x:SetState(3) end end
 elseif s.kind=="lifted" then
  clear();local r=Game():GetRoom();liftedIndex=r:GetGridIndex(center())
  if r:GetGridEntity(liftedIndex) then r:RemoveGridEntityImmediate(liftedIndex,0,false) end
  assert(r:SpawnGridEntity(liftedIndex,2,0,191914,0));keep(assert(r:PickupGridEntity(liftedIndex)))
 elseif s.kind=="lifted-change" then
  local d=assert(tracked[1]:ToEffect():GetGridEntityDesc());d.State=2;d.VarData=3
 elseif s.kind=="lifted-land" then
  local r=Game():GetRoom();for _,e in ipairs(tracked) do e:Remove() end;tracked={}
  assert(r:SpawnGridEntity(assert(liftedIndex),2,0,191915,3))
 elseif s.kind=="native-text" then
  clear();damageDebugAdded=(Game():GetDebugFlags() & DebugFlag.DAMAGE_VALUES)==0
  Game():AddDebugFlags(DebugFlag.DAMAGE_VALUES)
  local n=spawn(10,0,0,Vector.Zero):ToNPC();n.HitPoints=100
  assert(n:TakeDamage(1,0,EntityRef(player()),0),"Source damage did not enter real writer path")
 elseif s.kind=="native-dummy" then
  clear();
  -- 显式E2E专用池桥，不声称原版自然caller；不能用Spawn(964)的NPC冒充Dummy。
  assert(IsaacCSObservationNative.TestEntitySource,"E2E native source bridge missing")
  IsaacCSObservationNative.TestEntitySource(config.token,s.kind)
 elseif s.kind=="short" then local e=spawn(1000,57,0,Vector.Zero);e:Remove()
 elseif s.kind=="resources" then
  clear();local a=spawn(10,0,0,Vector(-60,0));local b=spawn(10,0,0,Vector(60,0))
  a:GetSprite().Rotation=17;b:GetSprite().Rotation=-13;b:GetSprite().Color=Color(0.5,0.8,1,1,0,0,0)
  original={};for _,e in ipairs(tracked) do original[#original+1]={position=e.Position,color=e.Color} end
 elseif s.kind=="restore" then for i,e in ipairs(tracked) do e.Position=original[i].position;e.Color=original[i].color end
 elseif s.kind=="water" then Game():GetRoom():SetWaterAmount(s.amount)
 elseif s.kind=="dense" then clear();for i=1,128 do local t=spawn(2,0,0,Vector(-130+(i%16)*17,-70+math.floor(i/16)*17),player()):ToTear();t.Height=-20;t.FallingSpeed=0;t.FallingAcceleration=0 end
 elseif s.kind=="queries" then
  local n=assert(tracked[1]);local before=n:GetDropRNG():GetSeed()
  for _,q in ipairs({{"Entity","HasEntityFlags",{EntityFlag.FLAG_FREEZE}},{"Sprite","HasCustomShader",{"tx19-absent-shader"}}}) do
   local object=q[1]=="Sprite" and n:GetSprite() or n
   IsaacCSObservation.Query(object,q[1],q[2],q[3])
  end
  assert(before==n:GetDropRNG():GetSeed(),"Query advanced RNG")
 elseif s.kind=="relations" then assert(tracked[1]).Parent=assert(tracked[2]);tracked[2].Child=tracked[1]
 elseif s.kind=="clear-relations" then for _,e in ipairs(tracked) do e.Parent=nil;e.Child=nil;e.Target=nil end
 elseif s.kind=="dead" then assert(tracked[1]):Die()
 elseif s.kind=="morph" then assert(tracked[2]:ToNPC()):Morph(11,0,0,-1)
 elseif s.kind=="remove" then for _,e in ipairs(tracked) do if e:Exists() then e:Remove() end end;tracked={}
 elseif s.kind=="change" then for _,e in ipairs(tracked) do if e:Exists() then e.Position=e.Position+Vector(8,-6);e.Color=Color(0.7,0.8,1,0.9,0,0,0);local n=e:ToNPC();if n then n.HitPoints=n.HitPoints*0.5 end end end
 else error("unknown Source scene:"..tostring(s.kind)) end
end
local function inventory()
 local rows={};for _,e in ipairs(Isaac.GetRoomEntities()) do
  rows[#rows+1]={subject=IsaacCSObservationNative.Subject(e),type=e.Type,variant=e.Variant,exists=e:Exists(),dead=e:IsDead()}
 end;return rows
end
mod:AddCallback(ModCallbacks.MC_PRE_UPDATE,function()
 if held and IsaacCSObservationNative.SourceSnapshotToken()~="" then return true end
end)
mod:AddCallback(ModCallbacks.MC_POST_GAME_STARTED,function()
 sourceReady=true
 if scenes[action] and scenes[action].kind=="familiar-owner" then clear();familiars() end
end)
mod:AddCallback(ModCallbacks.MC_PRE_GAME_EXIT,function() sourceReady=false;held=false;restoreDamageDebug() end)
mod:AddCallback(ModCallbacks.MC_POST_RENDER,function()
 frames=frames+1;if config.node_role~="Source" or not sourceReady or Game():GetNumPlayers()==0 then return end
 if Input.IsButtonTriggered(Keyboard.KEY_F9,0) then held=false end
 if Input.IsButtonTriggered(Keyboard.KEY_F8,0) then
  held=false;capture=nil;action=action+1;local s=assert(scenes[action],"unexpected action")
  local ok,err=pcall(perform,s);if not ok then emit("fixture_error",{label=s.label,reason=tostring(err)});return end
  -- 真实Source有短暂更新窗口，随后等新普通完整采集，不把旧token当本次结果。
  pending={scene=s,at=frames+((s.kind=="setup" or s.kind=="familiar-owner") and 60 or s.kind=="dead" and 1 or 3)}
 end
 if pending and frames>=pending.at then
  capture={scene=pending.scene,after=IsaacCSObservationNative.SourceSnapshotToken()};pending=nil
 end
 if capture then
  local token=IsaacCSObservationNative.SourceSnapshotToken()
  if token~="" and token~=capture.after then
   held=true;restoreDamageDebug();emit("ready",{label=capture.scene.label,capture_token=token,entities=inventory(),slots=slotResults});capture=nil
  end
 end
end)
