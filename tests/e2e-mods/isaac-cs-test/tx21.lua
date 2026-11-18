-- I32：仅Host布置/补充真实负载；Client没有玩法回调，不导入或生成世界。
local config=include("harness_config")
if config.node_role~="Host" then return end
assert(config.tx=="tx-04-21" and config.token and config.token~="","tx21 explicit E2E token required")
local mod=RegisterMod("Isaac CS realtime Host E2E",1)
local json=require("json")
local actions=json.decode(config.actions_json or "[]")
local step,frame,seed=0,0,212121
local active=false;local attackFloor=16;local pending=nil;local stopped=false;local sustainPlayers=false;local holdRoom=false
-- 规模压测独立于混合负载。记录实际原生实体，便于按数量核定计时窗口。
local pressure=nil
local staticTears=nil
local function emit(event,data)
 data=data or {};data.event=event;data.token=config.token;data.step=step;data.frame=frame
 Isaac.DebugString("[isaac-cs-test] tx21 "..json.encode(data))
end
local function host()
 local status=tostring(Isaac.ExecuteCommand("cs status"))
 return status:find("\nstate=Active\n",1,true)~=nil and not status:find("\nclient.target_source=",1,true)
end
local function roots()
 local out,seen={},{}
 for _,p in ipairs(PlayerManager.GetPlayers()) do
  if not seen[p.ControllerIndex] then seen[p.ControllerIndex]=true;out[#out+1]=p end
 end
 table.sort(out,function(a,b)return a.ControllerIndex<b.ControllerIndex end);return out
end
local function spawn(t,v,s,pos,velocity,owner)
 seed=seed+1;assert(seed>0)
 -- 本地原版Wiki建议用非零显式seed的Game:Spawn，避免Isaac.Spawn的零seed缺陷。
 return assert(Game():Spawn(t,v,pos,velocity,owner,s,seed),"tx21 Host spawn failed")
end
local function counts()
 local npcs,attacks,moving,players=0,0,0,{}
 local total,pickups,effects,tears,projectiles=0,0,0,0,0
 for _,e in ipairs(Isaac.GetRoomEntities()) do
  -- 下限负载之外的受击掉落和自然特效也必须计入实测规模，不隐藏额外压力。
  total=total+1;if e.Type==5 then pickups=pickups+1 elseif e.Type==1000 then effects=effects+1 end
  if e:IsActiveEnemy(false) and not e:IsDead() then npcs=npcs+1;if e.Velocity:Length()>0.01 then moving=moving+1 end end
  if e.Type==2 or e.Type==4 or e.Type==7 or e.Type==8 or e.Type==9 then attacks=attacks+1 end
  if e.Type==EntityType.ENTITY_TEAR then tears=tears+1 end
  if e.Type==EntityType.ENTITY_PROJECTILE then projectiles=projectiles+1 end
 end
 for _,p in ipairs(PlayerManager.GetPlayers()) do players[#players+1]={subject=IsaacCSObservationNative.Subject(p),type=p:GetPlayerType(),controller=p.ControllerIndex,x=p.Position.X,y=p.Position.Y} end
 return {npc=npcs,attacks=attacks,tears=tears,projectiles=projectiles,moving_npc=moving,bodies=players,participants=#roots(),entities=total,pickups=pickups,effects=effects,room_index=Game():GetLevel():GetCurrentRoomIndex(),room_frame=Game():GetRoom():GetFrameCount()}
end
-- 固定泪弹负载的运动输入，保留原生更新和 Sprite 采集。
local function holdTear(tear)
 tear.FallingSpeed=0;tear.FallingAcceleration=0;tear.Height=-20
 tear.Velocity=Vector.Zero
end
-- 一次生成完整压力批次，记录原生实例供后续悬空维护与单对象变化。
local function spawnStaticTears()
 local center=Game():GetRoom():GetCenterPos()
 local owner=Isaac.GetPlayer(0)
 staticTears={}
 for i=1,300 do
  local at=i-1
  local pos=center+Vector((at%25)*10-120,(at//25)*10-55)
  local tear=assert(spawn(EntityType.ENTITY_TEAR,TearVariant.BLUE,0,pos,Vector.Zero,owner):ToTear())
  holdTear(tear)
  tear.EntityCollisionClass=EntityCollisionClass.ENTCOLL_NONE
  tear.GridCollisionClass=EntityGridCollisionClass.GRIDCOLL_NONE
  staticTears[i]={tear=tear,pos=pos,seed=tear.InitSeed}
 end
 emit("batch_spawned",{actual=counts()})
end
local function supply()
 local room=Game():GetRoom();local c=counts();local center=room:GetCenterPos()
 -- 稳态段要求同房；只关闭实际打开的门，避免受击位移触发切房。实体仍正常移动和碰撞。
 if holdRoom then for slot=0,7 do local door=room:GetDoor(slot);if door and door:IsOpen() then door:Close(true) end end end
  -- 过载窗口保留真实受击。接近耗尽才补血，避免 Host 提前进入全员死亡。
  if sustainPlayers then
   for _,p in ipairs(PlayerManager.GetPlayers()) do
    if p:GetSoulHearts()<6 then p:AddSoulHearts(12-p:GetSoulHearts()) end
   end
  end
 if pressure then
  if pressure=="tears-100" then
   -- 生成时关闭碰撞，后续在原生更新前维持悬空。
   local owner=Isaac.GetPlayer(0)
   for i=c.tears+1,100 do
    local pos=center+Vector((i%20)*12-120,(i//20)*12-36)
    local tear=assert(spawn(EntityType.ENTITY_TEAR,TearVariant.BLUE,0,pos,Vector.Zero,owner):ToTear())
    holdTear(tear)
    tear.EntityCollisionClass=EntityCollisionClass.ENTCOLL_NONE
    tear.GridCollisionClass=EntityGridCollisionClass.GRIDCOLL_NONE
   end
  else
   -- 敌怪保留正常 AI 和碰撞。不主动开火，单独测敌怪数量及其原生表现。
   for i=c.npc+1,100 do
    local pos=center+Vector((i%20)*13-130,(i//20)*24-60)
    assert(spawn(10,0,0,room:FindFreePickupSpawnPosition(pos,0,true),Vector.Zero,nil):ToNPC())
   end
  end
  return
 end
 for i=c.npc+1,8 do
  local e=spawn(10,0,0,room:FindFreePickupSpawnPosition(center+Vector((i%4-2)*55,-70),0,true),Vector(1,0),nil)
  -- 不冻结AI、不改碰撞/伤害；补充实体仍由原版Authority正常更新和受击。
  assert(e:ToNPC(),"actual NPC required")
 end
 for i=c.attacks+1,attackFloor do
  local p=Isaac.GetPlayer((i-1)%Game():GetNumPlayers())
  p:FireTear(p.Position,Vector.FromAngle((i*23)%360)*5,false,true,false,p,1)
 end
end
local function perform(a)
 assert(host(),"Host runtime not active")
 -- 实际开始生成负载前核对原生成就资格，不能仅凭隔离存档推定无账号副作用。
 assert(Game():AchievementUnlocksDisallowed(),"E2E fixture requires an achievement-disabled run")
 local room=Game():GetRoom();local p=Isaac.GetPlayer(0);local center=room:GetCenterPos()
 if a=="tears-100" or a=="tears-300" or a=="enemies-100" then
  local rs=roots();assert(#rs==config.participants,"real frozen participants missing")
  for _,r in ipairs(rs) do r:ChangePlayerType(19) end
  for _,e in ipairs(Isaac.GetRoomEntities()) do if not e:ToPlayer() then e:Remove() end end
  -- Remove 在原生更新末尾生效，下一帧再按真实存在数量补齐。
  pressure=a;active=false;sustainPlayers=true;holdRoom=true
  pending={pressure=a,after=frame+2}
 elseif a=="mutate-static-tear" then
  local tear=assert(staticTears and staticTears[1],"300-tear fixture has not spawned").tear
  assert(tear:Exists(),"designated tear no longer exists")
  local before=tear.Position
  tear.Position=before+Vector(3,0)
  staticTears[1].pos=tear.Position
  emit("static_tear_changed",{seed=tear.InitSeed,old_x=before.X,new_x=tear.Position.X})
 elseif a=="ordinary" or a=="dual" or a=="mixed" or a=="ordinary-steady" or a=="dual-steady" then
  -- 稳态性能窗口保留真实受击，只补充低血量，防止180秒内全员死亡替代实际负载。
  active=false;sustainPlayers=true;holdRoom=true;local rs=roots();assert(#rs==config.participants,"real frozen participants missing")
  for i,r in ipairs(rs) do r:ChangePlayerType((a=="dual" or a=="dual-steady") and 19 or a=="mixed" and (i==#rs and 29 or 19) or 0) end
  -- 单独的稳态模式用回形针保留饰品装备，避免吞下的硬币把持续受击变成无界金币增长。
  -- 原ordinary/dual仍保留原始压力来源与复现入口，不删除现存掉落或改写历史结果。
  local steady=a=="ordinary-steady" or a=="dual-steady"
  for _,r in ipairs(PlayerManager.GetPlayers()) do r:AddCoins(5);r:AddKeys(5);r:AddBombs(5);r:AddSoulHearts(12);r:SetMinDamageCooldown(0);r:AddCollectible(34,6,false);r:SetCard(0,2);r:AddTrinket(steady and TrinketType.TRINKET_PAPER_CLIP or 1) end
  attackFloor=config.participants==4 and 32 or 16;active=true;supply()
 elseif a=="combat" then active=true;supply()
 elseif a=="overload" then
  -- E12 用真实可移动攻击实体提供负载。数量有界，结束后仍由原版自然更新与回收。
  attackFloor=64;active=true;sustainPlayers=true;holdRoom=true;supply()
 elseif a=="laser" then p:AddCollectible(68,0,false)
 elseif a=="bomb-grid" then
  local index=room:GetGridIndex(center+Vector(80,0));Isaac.GridSpawn(GridEntityType.GRID_ROCK,0,room:GetGridPosition(index),true)
  local b=p:FireBomb(room:GetGridPosition(index),Vector.Zero);b:SetExplosionCountdown(30)
 elseif a=="door" then
  holdRoom=false
  local found=false;for slot=0,7 do local d=room:GetDoor(slot);if d then d:Open();found=true;break end end
  assert(found,"real door required")
 elseif a=="pickup" then spawn(5,20,1,p.Position+Vector(20,0),Vector.Zero,nil)
 elseif a=="resources" then p:AddCoins(3);p:AddKeys(2);p:AddBombs(1)
 elseif a=="create-remove" then
  local e=spawn(5,20,1,center,Vector.Zero,nil);pending={remove=e,after=frame+10}
 elseif a=="zero-empty" then
  active=false;holdRoom=false;p:AddCoins(-p:GetNumCoins());p:AddKeys(-p:GetNumKeys());p:AddBombs(-p:GetNumBombs())
  for _,e in ipairs(Isaac.GetRoomEntities()) do if not e:ToPlayer() then e:Remove() end end
 elseif a=="last-static" then active=false;p:AddCoins(1)
 elseif a=="costume" then p:AddCollectible(1,0,false);p:AddCollectible(2,0,false)
 elseif a=="weapon" then p:AddCollectible(68,0,false)
 elseif a=="shape" then p:ChangePlayerType(19)
 elseif a=="backup" then
  p:ChangePlayerType(29);p:AddCollectible(619,0,false);p:UseActiveItem(711,UseFlag.USE_NOANIM|UseFlag.USE_NOANNOUNCER,-1)
 elseif a=="forgotten" then p:ChangePlayerType(16);assert(p:GetSubPlayer(),"Forgotten actual subplayer"):AddSoulHearts(4);assert(p:SwapForgottenForm(true,true))
 elseif a=="missing-resource" or a=="old-definition" then
  -- 错误只由E2E具名网络/资源故障注入，夹具不破坏正式安装文件或伪造来源。
  emit("requires-network-resource-fault",{kind=a});p:AddCollectible(1,0,false)
 elseif a=="stop-supply" then active=false;sustainPlayers=false;holdRoom=false;staticTears=nil
 else error("unknown tx21 action: "..tostring(a)) end
 emit("action",{kind=a,actual=counts()})
end
mod:AddCallback(ModCallbacks.MC_PRE_UPDATE,function()
 if stopped then return end
 if staticTears then
  for _,entry in ipairs(staticTears) do
   if entry.tear:Exists() then holdTear(entry.tear) end
  end
  return
 end
 if not active or pressure~="tears-100" then return end
 -- 只在专用负载中逐帧维护现存泪弹，以免补齐数量掩盖掉落和换生。
 for _,e in ipairs(Isaac.GetRoomEntities()) do
  local tear=e:Exists() and e:ToTear()
  if tear then holdTear(tear) end
 end
end)
mod:AddCallback(ModCallbacks.MC_POST_UPDATE,function()
 if stopped then return end
 frame=frame+1
 local ok,err=pcall(function()
  if pending and frame>=pending.after then
   if pending.remove and pending.remove:Exists() then pending.remove:Remove() end
   if pending.pressure and pressure==pending.pressure then
    if pressure=="tears-300" then spawnStaticTears() else active=true end
   end
   pending=nil
  end
  if active then supply() end
  if frame%30==0 then
   local actual=counts()
   if staticTears then
    actual.static_tears=0
    for _,entry in ipairs(staticTears) do
     local tear=entry.tear
     if tear:Exists() and tear.InitSeed==entry.seed and tear.Height<0 and
        tear.Velocity:LengthSquared()<0.0001 and tear.Position:DistanceSquared(entry.pos)<0.0001 then
      actual.static_tears=actual.static_tears+1
     end
    end
   end
   emit("load",actual)
  end
 end)
 if not ok then stopped=true;active=false;emit("fixture_error",{error=tostring(err)}) end
end)
mod:AddCallback(ModCallbacks.MC_POST_RENDER,function()
 if stopped or not Input.IsButtonTriggered(Keyboard.KEY_F8,0) then return end
 step=step+1;local action=assert(actions[step],"tx21 action exhausted")
 local ok,err=pcall(perform,action);if not ok then stopped=true;active=false;emit("fixture_error",{error=tostring(err)}) end
end)
-- 本夹具从不跳过更新/冻结目标；截图中性段必须由明确步骤声明，不能算作稳定性能段。
emit("fixture_ready",{role="Host",participants=config.participants,expected_room="Basement d.10",entity_spawn_seed_start=212121,actual_room="verify-at-first-run-action"})
