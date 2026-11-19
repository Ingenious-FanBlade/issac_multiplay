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
local mixedLoad=false
local battle=nil
local e09=nil
local e39Mode=nil
local e39Directions={}
local e39MazeVisits={}
local e39DebugMask=nil
local e39MirrorFloor=false
local e39AwaitDoor=false
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
 -- 读原生资源余额，区分仅生成拾取物与真实消费、归零和静止后的最终值。
 for _,p in ipairs(PlayerManager.GetPlayers()) do players[#players+1]={subject=IsaacCSObservationNative.Subject(p),type=p:GetPlayerType(),controller=p.ControllerIndex,x=p.Position.X,y=p.Position.Y,coins=p:GetNumCoins(),keys=p:GetNumKeys(),bombs=p:GetNumBombs()} end
 return {npc=npcs,attacks=attacks,tears=tears,projectiles=projectiles,moving_npc=moving,bodies=players,participants=#roots(),entities=total,pickups=pickups,effects=effects,room_index=Game():GetLevel():GetCurrentRoomIndex(),room_frame=Game():GetRoom():GetFrameCount()}
end
-- 固定泪弹负载的运动输入，保留原生更新和 Sprite 采集。
local function holdTear(tear)
 tear.FallingSpeed=0;tear.FallingAcceleration=0;tear.Height=-20
 tear.Velocity=Vector.Zero
end
-- 公开 Lua 属性按 GameFrame 和原生 use 留证，后台解析同一帧的线值与客机读值。
local function e09Entity(e)
  local id=IsaacCSObservationNative.Subject(e)
  if not id or not id.local_id then return end
  local position,velocity,offset,scale=e.Position,e.Velocity,e.PositionOffset,e.SizeMulti
  local result={type=e.Type,variant=e.Variant,subtype=e.SubType,spawner_type=e.SpawnerType,spawner_variant=e.SpawnerVariant,
   visible=e.Visible,exists=e:Exists(),dead=e:IsDead(),grid_collision_class=e.GridCollisionClass,
   entity_collision_class=e.EntityCollisionClass,["position.x"]=position.X,["position.y"]=position.Y,
   ["position_offset.x"]=offset.X,["position_offset.y"]=offset.Y,["velocity.x"]=velocity.X,["velocity.y"]=velocity.Y,
   size=e.Size,["size_multi.x"]=scale.X,["size_multi.y"]=scale.Y,hit_points=e.HitPoints,max_hit_points=e.MaxHitPoints,init_seed=e.InitSeed}
  local p=e:ToPlayer()
  if p then
   result._playerType=p:GetPlayerType()
   result._maxHearts=p:GetMaxHearts();result._redHearts=p:GetHearts();result._soulHearts=p:GetSoulHearts()
   result._numKeys=p:GetNumKeys();result._numBombs=p:GetNumBombs();result._numCoins=p:GetNumCoins()
  end
  return {id=id.local_id,use=id.use,fields=result}
end
local function compareLua()
 local entities,grids={},{}
 local function add(e)
  local values=e09Entity(e)
  if values then entities[#entities+1]=values end
 end
 for _,p in ipairs(PlayerManager.GetPlayers()) do add(p) end
 for _,e in ipairs(Isaac.GetRoomEntities()) do
  if e.Type==2 or e.Type==4 or e.Type==5 or e.Type==7 or e.Type==208 then add(e) end
 end
 local room=Game():GetRoom()
 local g=room:GetGridEntity(e09.grid)
 if g then
  local id=IsaacCSObservationNative.GridSubject(g)
  if id and id.local_id then
   grids[1]={id=id.local_id,use=id.use,fields={["desc.type"]=g:GetType(),["desc.variant"]=g:GetVariant(),
    ["desc.state"]=g.State,grid_index=e09.grid}}
  end
 end
 emit("e09_values",{game_frame=Game():GetFrameCount(),entities=entities,grids=grids})
end
-- 只在 Host 通过公开入口请求原生过渡。门的索引来自当前房实际门，Boss 目标来自原生房间目录。
local function e39QuickClear(game,enable)
 -- 以游戏读取的掩码为准，原生回溯可能还原旧的调试状态。
 local before=game:GetDebugFlags()
 if e39DebugMask and ((before&e39DebugMask)~=0)==enable then return end
 if not e39DebugMask and not enable then return end
 Isaac.ExecuteCommand("debug 10")
 local after=game:GetDebugFlags()
 local changed=before~after
 assert(changed~=0,"debug 10 failed to toggle")
 e39DebugMask=e39DebugMask or changed
 if ((after&e39DebugMask)~=0)~=enable then Isaac.ExecuteCommand("debug 10") end
end
local function transition(a,game,room,p)
 assert(RoomTransition.GetTransitionMode()==0,"previous native transition is still active")
 -- 普通路线快清；Boss 房保留原版开场条件。
 e39QuickClear(game,a~="e39-boss")
 local level=game:GetLevel()
 if a=="e39-maze" and (level:GetCurses()&LevelCurse.CURSE_OF_MAZE)==0 then
  level:AddCurse(LevelCurse.CURSE_OF_MAZE,false)
 end
 local source=level:GetCurrentRoomIndex()
 local sourceGrid=level:GetCurrentRoomDesc().SafeGridIndex
 local physicalDoor=a=="e39-maze" or a=="e39-walk"
 if a=="e39-maze" then
  assert((level:GetCurses()&LevelCurse.CURSE_OF_MAZE)~=0,"native maze curse inactive")
  e39MazeVisits[source]=(e39MazeVisits[source] or 0)+1
 end
 local target,direction,animation=source,Direction.NO_DIRECTION,RoomTransitionAnim.WALK
 local dimension=level:GetDimension()
 if a=="e39-same11" then animation=RoomTransitionAnim.WOMB_TELEPORT
 elseif a=="e39-same16" then animation=RoomTransitionAnim.PORTAL_TELEPORT;direction=Direction.LEFT
 elseif a=="e39-hourglass" then animation=RoomTransitionAnim.GLOWING_HOURGLASS
 elseif a=="e39-mirror" then
  assert(level:HasMirrorDimension(),"no original mirror dimension")
  dimension=dimension==0 and 1 or 0
  local descriptor=level:GetRoomByIdx(source,dimension)
  assert(descriptor and descriptor.Data,"no corresponding original room across mirror")
  animation=RoomTransitionAnim.FADE_MIRROR
 elseif a=="e39-boss" then
  local rooms=level:GetRooms()
  for i=0,rooms.Size-1 do
   local d=rooms:Get(i)
   if d.Data and d.Data.Type==RoomType.ROOM_BOSS and d.SafeGridIndex>=0 then target=d.SafeGridIndex;break end
  end
  assert(target~=source,"no destination boss room")
 else
  local best,bestVisits=nil,math.huge
  for slot=0,7 do
   local door=room:GetDoor(slot)
   local visits=a=="e39-maze" and door and (e39MazeVisits[door.TargetRoomIndex] or 0) or 0
   if door and (not physicalDoor or door.TargetRoomType==RoomType.ROOM_DEFAULT and not door:IsLocked()) and
      (visits<bestVisits or visits==bestVisits and not e39Directions[slot%4] and e39Directions[best%4]) then
    best,bestVisits=slot,visits
   end
  end
  assert(best,"no native door destination")
  target=room:GetDoor(best).TargetRoomIndex
  direction=best%4
  e39Directions[direction]=true
  if a=="e39-fade" then animation=RoomTransitionAnim.FADE
  elseif a=="e39-pixel" then animation=RoomTransitionAnim.PIXELATION
  elseif a=="e39-minecart" then animation=RoomTransitionAnim.MINECART end
 end
 e39Mode=-1
 local request={kind=a,source=source,source_grid=sourceGrid,target=target,direction=direction,animation=animation,
  dimension=dimension,game_frame=game:GetFrameCount()}
 if physicalDoor then
  -- 由真实移动输入触发门判定，覆盖原生换房与等待新场景的绘制。
  local stepVector=({Vector(-1,0),Vector(0,-1),Vector(1,0),Vector(0,1)})[direction+1]
  local door=nil
  for slot=direction,7,4 do
   local candidate=room:GetDoor(slot)
   if candidate and candidate.TargetRoomIndex==target then door=candidate;break end
  end
  assert(door,"native door disappeared")
  door:Open()
  local position=room:GetGridPosition(door:GetGridIndex())
  for i,body in ipairs(roots()) do body.Position=position-stepVector*(36+i*9);body.Velocity=Vector.Zero end
  e39AwaitDoor=true
  request.curses=level:GetCurses()
  emit("e39_door_ready",request)
 else
  e39AwaitDoor=false
  emit("e39_request",request)
  game:StartRoomTransition(target,direction,animation,p,dimension)
 end
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
  local npc=assert(e:ToNPC(),"actual NPC required")
  -- 混合采样固定八个 Gaper，预留血量避免死亡改型。AI、碰撞、受击和泪弹消亡走原生路径。
  if mixedLoad then npc.MaxHitPoints=10000;npc.HitPoints=npc.MaxHitPoints end
 end
 for i=(mixedLoad and c.tears or c.attacks)+1,attackFloor do
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
 elseif a=="npc8-tears64" then
  -- 混合窗口只以泪弹计数补发，普通敌怪保留 AI、碰撞和原生伤害。
  local rs=roots();assert(#rs==config.participants,"real frozen participants missing")
  for _,r in ipairs(rs) do r:ChangePlayerType(19) end
  for _,e in ipairs(Isaac.GetRoomEntities()) do if not e:ToPlayer() then e:Remove() end end
  pressure=nil;staticTears=nil;active=false;sustainPlayers=true;holdRoom=true
  attackFloor=64;mixedLoad=true
  pending={mixed=true,after=frame+2}
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
 elseif a=="e39-floor" then
  -- 镜面维度由原版 Downpour II 生成，夹具不自行复制房间目录。
  e39MirrorFloor=true
  Isaac.ExecuteCommand("stage 2c")
 elseif a:sub(1,4)=="e39-" then
  transition(a,Game(),room,p)
 elseif a=="e09-scene" then
  -- 混合实体在同一普通窗，另由真实按键提供活动的 Player Body 样本。
  local npc=assert(spawn(EntityType.ENTITY_FATTY,0,0,center+Vector(125,-85),Vector.Zero,nil):ToNPC())
  npc.HitPoints=7.5
  spawn(5,20,1,center+Vector(-120,80),Vector.Zero,nil)
  local tear=p:FireTear(center+Vector(-65,-60),Vector.Zero,false,true,false,p,1)
  tear.EntityCollisionClass=EntityCollisionClass.ENTCOLL_NONE
  tear.GridCollisionClass=EntityGridCollisionClass.GRIDCOLL_NONE
  staticTears={{tear=tear,pos=tear.Position,seed=tear.InitSeed}}
  holdTear(tear)
  local grid=room:GetGridIndex(center+Vector(80,0))
  Isaac.GridSpawn(GridEntityType.GRID_ROCK,0,room:GetGridPosition(grid),true)
  e09={last=frame+250,grid=grid}
 elseif a=="e04-ordinary" or a=="e04-dual" or a=="e04-mixed" then
  -- 只布置原生控制拓扑，移动和开火由双端各自的实际按键驱动。
  local rs=roots();assert(#rs==2,"two frozen participants required")
  for i,r in ipairs(rs) do
   local kind=a=="e04-ordinary" and 0 or a=="e04-mixed" and i==2 and 29 or 19
   -- ChangePlayerType 重复应用雅各会再生一个以扫。保持同型身体，避免夹具引入额外成员。
   if r:GetPlayerType()~=kind then r:ChangePlayerType(kind) end
   local start=center+Vector(i==1 and -80 or 80,i==1 and -40 or 40)
   r.Position=start
   local twin=r:GetOtherTwin()
   if twin then twin.Position=start+Vector(0,24) end
  end
 elseif a=="e05-combat" then
  local rs=roots();assert(#rs==2,"two frozen participants required")
  local target=assert(spawn(EntityType.ENTITY_FATTY,0,0,center+Vector(30,0),Vector.Zero,nil):ToNPC())
  target.MaxHitPoints=6;target.HitPoints=6
  -- 两位玩家交替发原生泪弹，让伤害及死亡走实际碰撞和 NPC 更新。
  battle={target=target,seed=target.InitSeed,players=rs,shot=0,next=frame+15}
  emit("combat_target",{seed=target.InitSeed,health=target.HitPoints})
 elseif a=="e05-laser" then
  local laser=assert(p:FireBrimstone(Vector(1,0),p,1))
  emit("laser_target",{seed=laser.InitSeed})
 elseif a=="combat" then active=true;supply()
 elseif a=="overload" then
  -- E12 用真实可移动攻击实体提供负载。数量有界，结束后仍由原版自然更新与回收。
  attackFloor=64;active=true;sustainPlayers=true;holdRoom=true;supply()
 elseif a=="laser" then p:AddCollectible(68,0,false)
 elseif a=="bomb-grid" or a=="e05-bomb-grid" then
  local index=room:GetGridIndex(center+Vector(80,0));Isaac.GridSpawn(GridEntityType.GRID_ROCK,0,room:GetGridPosition(index),true)
  local b=p:FireBomb(room:GetGridPosition(index),Vector.Zero);b:SetExplosionCountdown(30)
  if a=="e05-bomb-grid" then assert(room:GetGridEntity(index),"rock required");emit("bomb_target",{seed=b.InitSeed,grid=index}) end
 elseif a=="door" or a=="e05-door" then
  holdRoom=false
  local found=false
  for slot=0,7 do
   local door=room:GetDoor(slot)
   if door and (a~="e05-door" or door.TargetRoomType==RoomType.ROOM_DEFAULT and not door:IsLocked()) then
    door:Open();found=true
    local position=room:GetGridPosition(door:GetGridIndex())
    local step=({Vector(-1,0),Vector(0,-1),Vector(1,0),Vector(0,1)})[slot%4+1]
    -- 只把主机移到门内侧，实际穿门仍由主机移动键触发。
    if a=="e05-door" then p.Position=position-step*45 end
    emit("door_target",{slot=slot,x=position.X,y=position.Y,target_room=door.TargetRoomIndex,open=door:IsOpen(),room_type=door.TargetRoomType})
    break
   end
  end
  assert(found,"real door required")
 elseif a=="pickup" then spawn(5,20,1,p.Position+Vector(20,0),Vector.Zero,nil)
 elseif a=="e06-pickups" then
  local seeds={}
  for _,variant in ipairs({20,30,40}) do
   local item=spawn(5,variant,1,p.Position,Vector.Zero,nil)
   seeds[#seeds+1]=item.InitSeed
  end
  emit("pickup_targets",{seeds=seeds})
 elseif a=="resources" then p:AddCoins(3);p:AddKeys(2);p:AddBombs(1)
 elseif a=="create-remove" then
  local e=spawn(5,20,1,center,Vector.Zero,nil);pending={remove=e,after=frame+10}
 elseif a=="e06-create-remove" then
  -- 可见生成后由 Host 显式删除同一实例，客机验证同一 seed 离场。
  local e=spawn(5,20,1,center+Vector(-80,0),Vector.Zero,nil)
  pending={remove=e,after=frame+60,seed=e.InitSeed}
  emit("deletion_target",{seed=e.InitSeed})
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
 elseif a=="stop-attacks" then
  -- 功能续测阶段允许移动和穿门，保留前序的低血保护以避免待测动作被 Game Over 截断。
  active=false;holdRoom=false;staticTears=nil;mixedLoad=false
  for _,entity in ipairs(Isaac.GetRoomEntities()) do
   if entity:IsActiveEnemy(false) then entity:Remove() end
  end
 elseif a=="stop-supply" then active=false;sustainPlayers=false;holdRoom=false;staticTears=nil;mixedLoad=false
 else error("unknown tx21 action: "..tostring(a)) end
 emit("action",{kind=a,actual=counts()})
end
mod:AddCallback(ModCallbacks.MC_POST_PLAYER_UPDATE,function(_,p)
 if not e09 or stopped then return end
 -- 普通和 odd 共用原生 PlayerPost。读当前 Authority tick，不能用同一 GameFrame 猜相位。
 local tick=tostring(Isaac.ExecuteCommand("cs witness")):match("simulation_tick=(%d+)")
 local values=e09Entity(p)
 if tick and tick~="0" and values then emit("e09_player_post",{tick=tick,entity=values}) end
end)
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
  if battle then
   local n=battle.target
   if not n:Exists() or n.InitSeed~=battle.seed or n:IsDead() then
    emit("combat_dead",{seed=battle.seed,shots=battle.shot})
    battle=nil
   elseif frame>=battle.next and battle.shot<6 then
    local player=battle.players[battle.shot%2+1]
    local direction=(n.Position-player.Position):Normalized()
    local tear=player:FireTear(n.Position-direction*45,direction*12,false,true,false,player,1)
    battle.shot=battle.shot+1;battle.next=frame+30
    emit("combat_shot",{seed=tear.InitSeed,controller=player.ControllerIndex,health=n.HitPoints})
   end
  end
  -- E09 每三个游戏帧采样。把具名删除对到采样窗，检查 exists=false 和随后摘除名册两步。
  if pending and frame>=pending.after and (not e09 or not pending.seed or Game():GetFrameCount()%3==0) then
   if pending.remove then
    local e=pending.remove
    if e:Exists() and (not pending.seed or e.InitSeed==pending.seed) then
     e:Remove()
     if pending.seed then emit("deletion_performed",{seed=pending.seed,game_frame=Game():GetFrameCount()}) end
    end
   end
   if pending.pressure and pressure==pending.pressure then
    if pressure=="tears-300" then spawnStaticTears() else active=true end
   end
   if pending.mixed then active=true end
   pending=nil
  end
  -- 低血保护独立于补充攻击实体，停供后仍可安全执行门、拾取和换装。
  if sustainPlayers then
   for _,player in ipairs(PlayerManager.GetPlayers()) do
    if player:GetSoulHearts()<6 then player:AddSoulHearts(12-player:GetSoulHearts()) end
   end
  end
  if active then supply() end
  if e09 then
   if frame<=e09.last then compareLua() else e09=nil end
  end
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
 if e39MirrorFloor and Game():GetLevel():HasMirrorDimension() and RoomTransition.GetTransitionMode()==0 then
  e39MirrorFloor=false
  local g=Game();local level=g:GetLevel()
  emit("e39_phase",{mode=0,room=level:GetCurrentRoomIndex(),dimension=level:GetDimension(),game_frame=g:GetFrameCount()})
 end
 if e39Mode and not stopped then
  local mode=RoomTransition.GetTransitionMode()
  if mode~=e39Mode and (mode~=0 or not e39AwaitDoor) then
   if mode~=0 then e39AwaitDoor=false end
   e39Mode=mode;local g=Game()
   emit("e39_phase",{mode=mode,room=g:GetLevel():GetCurrentRoomIndex(),dimension=g:GetLevel():GetDimension(),game_frame=g:GetFrameCount(),boss=RoomTransition.IsRenderingBossIntro()})
   if mode==0 and step==#actions then e39QuickClear(g,false) end
  end
 end
 if stopped or not Input.IsButtonTriggered(Keyboard.KEY_F8,0) then return end
 step=step+1;local action=assert(actions[step],"tx21 action exhausted")
 local ok,err=pcall(perform,action);if not ok then stopped=true;active=false;emit("fixture_error",{error=tostring(err)}) end
end)
-- 本夹具从不跳过更新/冻结目标；截图中性段必须由明确步骤声明，不能算作稳定性能段。
emit("fixture_ready",{role="Host",participants=config.participants,expected_room="Basement d.10",entity_spawn_seed_start=212121,actual_room="verify-at-first-run-action"})
