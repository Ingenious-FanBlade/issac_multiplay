-- I33失败模型先见tx-04-19-e2e-integration.md；只写Source，Client走正式parser/Scene。
local mod=RegisterMod("Isaac CS full Player application E2E",1)
IsaacCSClientTestMod=mod
local config=include("harness_config");local json=require("json")
local scenes=json.decode(config.scenes_json)
local action,frames=0,0;local pending,capture,held=nil,nil,false
local inputStart,inputProof={},{}
local damageDebugAdded=false
local function restoreDamageDebug()
 if damageDebugAdded and (Game():GetDebugFlags() & DebugFlag.DAMAGE_VALUES)~=0 then Isaac.ExecuteCommand("debug 7") end
 damageDebugAdded=false
end
local original={};local tracked={};local slotResults={};local sourceReady=false;local liftedIndex
local ghostPickupIndex=nil;local mrMeUsed=false
-- 换主人后固定真实Burning Basement，避免随机新run绕开generic云层混合模式回归。
local ownerStagePending=0
local function emit(event,data)
 data=data or {};data.event=event;data.token=config.token;data.action=action
 Isaac.DebugString("[isaac-cs-test] tx20 "..json.encode(data))
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
 ghostPickupIndex=nil
 for _,e in ipairs(Isaac.GetRoomEntities()) do if not e:ToPlayer() then e:Remove() end end
 tracked={};local p=player();p.Position=center()+Vector(0,100);p.Velocity=Vector.Zero
 if mrMeUsed then p:GetEffects():RemoveCollectibleEffect(CollectibleType.COLLECTIBLE_MR_ME,1);mrMeUsed=false end
 p.EntityCollisionClass=0;p.GridCollisionClass=0;p:SetMinDamageCooldown(0)
end
local function familiars()
 for i,v in ipairs({1,8,48,68,78,231,243}) do local f=spawn(3,v,0,Vector(-110+(i-1)*32,-20),player()):ToFamiliar();f.Player=player() end
end
local perform20,settled20=include("tx20_actions")({clear=clear,spawn=spawn,mod=mod,source=config.node_role=="Source",emit=emit})
local function perform(s)
 if perform20(s) then return end
 if s.kind=="setup" then
  Isaac.ExecuteCommand("stage 1");return
 elseif s.kind=="clear" then clear();return
 elseif s.kind=="npc" then
  clear();local n=spawn(10,0,0,Vector(-70,-25)):ToNPC();n:MakeChampion(191919,0,true)
  local child=spawn(10,0,0,Vector(30,-25)):ToNPC();n.Child=child;child.Parent=n
  -- Dip的公开KillUnique用于真实独特死亡动画，保留同一Key的生前→死亡中存在后态。
  spawn(EntityType.ENTITY_DIP,0,0,Vector(90,55))
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
  elseif s.mode=="sample" then
   -- mode2的原生候选包含非Parent的Player；真实NPC作Parent，现有Player提供有效目标。
   local owner=spawn(10,0,0,Vector(90,-40));owner:AddEntityFlags(EntityFlag.FLAG_FREEZE)
   l.Parent=owner;l:SetHomingType(2);l.CurveStrength=0
  elseif s.mode=="chain" then
   -- bit19是原生Bounce维护前提；让光束真实击墙，由原生owner创建并维持子束。
   l.GridCollisionClass=EntityGridCollisionClass.GRIDCOLL_WALLS
   l.MaxDistance=700;l:AddTearFlags(TearFlags.TEAR_BOUNCE)
  end
 elseif s.kind=="knife" then
  clear();local owner=Isaac.Spawn(10,0,0,center()+Vector(-70,-25),Vector.Zero,nil)
  owner:AddEntityFlags(EntityFlag.FLAG_FREEZE)
  -- 普通Isaac没有Knife武器，原生Update会退休其挂载刀；用真实NPC父级保留公开FireKnife来源。
  keep(player():FireKnife(owner,20,true,0,0));keep(owner)
 elseif s.kind=="knife-out" then assert(tracked[1]:ToKnife()):Shoot(1,140)
 elseif s.kind=="knife-return" then
  -- 不用Shoot(0,0)冒充返回；让同一刀自然进入负速度阶段，并换成另一个真实owner。
  local owner=spawn(10,0,0,Vector(70,-25));owner:AddEntityFlags(EntityFlag.FLAG_FREEZE)
  assert(tracked[1]:ToKnife()).Parent=owner
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
  clear();local p=player();local anchor=spawn(10,0,0,Vector(-90,-30));anchor:AddEntityFlags(EntityFlag.FLAG_FREEZE)
  local ground=spawn(1000,22,0,Vector(-100,0),p):ToEffect();ground.Timeout=1800
  local pulling=spawn(1000,57,0,Vector(-50,0),p):ToEffect();pulling.Timeout=1800;pulling:FollowParent(anchor)
  local trail=spawn(1000,166,0,Vector(40,0),p):ToEffect();trail.Timeout=1800;trail:FollowParent(p)
  -- MrMe由道具owner配置同类Target；不能把Player放进原生要求Effect的Target槽。
  p:UseActiveItem(CollectibleType.COLLECTIBLE_MR_ME,UseFlag.USE_NOANIM|UseFlag.USE_NOCOSTUME|UseFlag.USE_NOANNOUNCER,-1);mrMeUsed=true
  -- Ghost由实际Pickup owner和公开LootList回调生成，绝不把Player指针放进Pickup专用父关系。
  local chest=spawn(5,50,0,Vector(90,-30)):ToPickup();chest.Wait=1800;ghostPickupIndex=chest.Index;chest:UpdatePickupGhosts()
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
  assert(r:SpawnGridEntity(liftedIndex,2,0,191914,0))
  local e=keep(assert(r:PickupGridEntity(liftedIndex)))
  -- Pickup只返回可投掷helper；公开Player owner负责真正举起，避免无owner的落地helper立即消失。
  assert(player():TryHoldEntity(e),"Player did not hold real lifted Grid helper")
  -- TryHold登记Player手持槽但不设helper Parent；136的原生Update仍要求有效父级。
  e:FollowParent(player())
 elseif s.kind=="lifted-change" then
  local d=assert(tracked[1]:ToEffect():GetGridEntityDesc());d.State=2;d.VarData=3
 elseif s.kind=="lifted-land" then
  local r=Game():GetRoom();local thrown=assert(player():ThrowHeldEntity(Vector.Zero),"lifted Player owner was lost")
  -- 本场景显式提供最终落地Grid，不冒称自然投射物落地；公开投出后退休其返回主体及旧helper。
  if thrown:Exists() then thrown:Remove() end
  for _,e in ipairs(tracked) do if e:Exists() then e:Remove() end end;tracked={}
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
  for _,q in ipairs({{"Entity","HasEntityFlags",{EntityFlag.FLAG_FREEZE}},{"Sprite","HasCustomShader",{"tx20-absent-shader"}}}) do
   local object=q[1]=="Sprite" and n:GetSprite() or n
   -- Sprite是查询对象，Entity才是独立记录的稳定owner；复用正式接口的第五参数。
   IsaacCSObservation.Query(n,q[1],q[2],q[3],object)
  end
  assert(before==n:GetDropRNG():GetSeed(),"Query advanced RNG")
 elseif s.kind=="relations" then
  -- 先解除旧树方向再反转；合法Parent/Child回链不能变成无根的Parent递归环。
  local child,parent=assert(tracked[1]),assert(tracked[2])
  child.Child=nil;parent.Parent=nil;child.Parent=parent;parent.Child=child
 elseif s.kind=="clear-relations" then for _,e in ipairs(tracked) do e.Parent=nil;e.Child=nil;e.Target=nil end
 elseif s.kind=="dead" then assert(tracked[3]:ToNPC()):KillUnique()
 elseif s.kind=="morph" then assert(tracked[2]:ToNPC()):Morph(11,0,0,-1)
 elseif s.kind=="remove" then for _,e in ipairs(tracked) do if e:Exists() then e:Remove() end end;tracked={}
 elseif s.kind=="change" then for _,e in ipairs(tracked) do if e:Exists() then e.Position=e.Position+Vector(8,-6);e.Color=Color(0.7,0.8,1,0.9,0,0,0);local n=e:ToNPC();if n then n.HitPoints=n.HitPoints*0.5 end end end
 else error("unknown Source scene:"..tostring(s.kind)) end
end
-- 房间公开集合不是身体全集；同时遍历公开roster、形态边和四个真实备份槽。
local function inventory()
 local rows,seen,players={},{},{}
 local function add(e)
  if not e then return end
  local subject=IsaacCSObservationNative.Subject(e);local id=tostring(subject.local_id)
  if seen[id] then return end;seen[id]=true
  rows[#rows+1]={subject=subject,type=e.Type,variant=e.Variant,exists=e:Exists(),dead=e:IsDead()}
  -- ready属于实际Render窗口；保留Knife当前姿态以区分普通采样T和稍后截图，不能拿它覆盖T。
  if e:ToKnife() then rows[#rows].render_pose={position={e.Position.X,e.Position.Y},rotation=e:GetSprite().Rotation} end
  if e:ToPlayer() then players[#players+1]=e:ToPlayer();local v=e:ToPlayer():GetMovementInput();rows[#rows].movement={v.X,v.Y};rows[#rows].render_damage_cooldown=e:ToPlayer():GetDamageCooldown() end
 end
 for _,e in ipairs(Isaac.GetRoomEntities()) do add(e) end
 for _,p in ipairs(PlayerManager.GetPlayers()) do add(p) end
 for slot=0,3 do add(PlayerManager.GetEsauJrState(slot)) end
 local i=1;while i<=#players do
  assert(i<=256,"Source body graph overflow");local p=players[i]
  add(p:GetSubPlayer());add(p:GetOtherTwin());add(p:GetFlippedForm());i=i+1
 end
 return rows
end
-- 游戏日志单行有界；密集库存分块传送，ready最后发，所有块仍属于同一冻结token/action。
local function emitReady(label,token)
 local rows=inventory();local chunks=#rows>16 and math.ceil(#rows/16) or 0
 if chunks>0 then
  for index=1,chunks do
   local part={};for i=(index-1)*16+1,math.min(index*16,#rows) do part[#part+1]=rows[i] end
   emit("inventory_chunk",{label=label,index=index,entities=part})
  end
 end
 emit("ready",{label=label,capture_token=token,manager_frame=Isaac.GetFrameCount(),game_frame=Game():GetFrameCount(),entity_count=#rows,inventory_chunks=chunks,entities=chunks==0 and rows or {},slots=slotResults})
end
-- 仅Source的具名Chest走官方扩展回调；实际原生Pickup owner负责分配/关联/资源，不填观测结果。
mod:AddCallback(ModCallbacks.MC_PRE_PICKUP_UPDATE_GHOST_PICKUPS,function(_,pickup)
 if config.node_role=="Source" and ghostPickupIndex==pickup.Index then return true end
end)
mod:AddCallback(ModCallbacks.MC_PRE_PICKUP_GET_LOOT_LIST,function(_,pickup)
 if config.node_role=="Source" and ghostPickupIndex==pickup.Index then
  local loot=LootList();loot:PushEntry(5,20,1,191916);loot:PushEntry(5,10,1,191917);return loot
 end
end)
mod:AddCallback(ModCallbacks.MC_PRE_UPDATE,function()
 if not config.network_regression and held and IsaacCSObservationNative.SourceSnapshotToken()~="" then return true end
end)
mod:AddCallback(ModCallbacks.MC_POST_GAME_STARTED,function()
 sourceReady=true
 if scenes[action] and scenes[action].kind=="familiar-owner" then ownerStagePending=1 end
end)
mod:AddCallback(ModCallbacks.MC_PRE_GAME_EXIT,function() sourceReady=false;held=false;restoreDamageDebug() end)
mod:AddCallback(ModCallbacks.MC_POST_RENDER,function()
 frames=frames+1;if config.node_role~="Source" or not sourceReady or Game():GetNumPlayers()==0 then return end
 if ownerStagePending==1 then
  -- 在正常Render边界执行公开stage命令；实际新房就绪后才建立Familiar，不借重启前的对象。
  ownerStagePending=2;Isaac.ExecuteCommand("stage 1b");return
 elseif ownerStagePending==2 then
  if Game():GetLevel():GetStageType()~=StageType.STAGETYPE_AFTERBIRTH or Game():GetRoom():GetBackdropType()~=3 then return end
  clear();familiars();ownerStagePending=0;if pending then pending.at=frames+60 end
 end
 if Input.IsButtonTriggered(Keyboard.KEY_F8,0) then
  held=false;capture=nil
  if action==#scenes then
   -- 全部场景后仍使用已采用的F8解冻，不另用会产生原版手动截图的F9。
   pending=nil;emit("resumed",{label="recorder-fault"})
  else
   action=action+1;local s=assert(scenes[action],"unexpected action")
   local ok,err=pcall(perform,s);if not ok then emit("fixture_error",{label=s.label,reason=tostring(err)});return end
   inputStart={};inputProof={}
   if s.kind=="player-input" then for _,p in ipairs(PlayerManager.GetPlayers()) do
    inputStart[tostring(IsaacCSObservationNative.Subject(p).local_id)]={p.Position.X,p.Position.Y}
   end end
   -- 真实Source有短暂更新窗口，随后等新普通完整采集，不把旧token当本次结果。
   -- 输入窗口保留48次左右真实普通更新，也让43帧POOF自然收尾后再恢复昂贵全域采集。
   -- pose可能紧随复活后新建骨棒；让其走真实更新，避免把首次ctor姿态与稍后Render混作同一帧。
   pending={scene=s,at=frames+((s.kind=="setup" or s.kind=="familiar-owner") and 60 or (s.kind=="player-birthright" or s.kind=="player-birthright-clear" or s.kind=="player-weapon-base") and 30 or (s.kind=="dead" or s.kind=="player-throw") and 1 or s.kind=="player-input" and 96 or s.kind=="player-pose" and 8 or 3)}
  end
 end
 if pending and frames>=pending.at then
  -- Transform只排队，Appear期间还不会推进变形AI；按实际BossType等待后再取新的完整窗口。
  local settled=settled20(pending.scene)
  if pending.scene.kind=="player-count" then
   -- 参与者入会后还需原生Join及唯一控制锚点落定；不能先冻结普通tick再等重绑。
   local expected=pending.scene.participants;local roster=PlayerManager.GetPlayers();local ordinals={}
   settled=#roster==expected and Game():GetNumPlayers()==expected
   for _,p in ipairs(roster) do
    local relation=IsaacCSObservationNative.Subject(p).control_relation
    if not relation or relation.body_slot~=0 or relation.ordinal<0 or relation.ordinal>=expected or ordinals[relation.ordinal] then settled=false
    else ordinals[relation.ordinal]=true end
   end
   if settled and not pending.layout_done then
    -- 新加入身体也经相同公开动作布置，随后再经过真实更新与新完整token。
    perform(pending.scene);pending.layout_done=true;pending.at=frames+3;settled=false
   end
   if not settled and (not pending.diagnosed_at or frames-pending.diagnosed_at>=120) then
    pending.diagnosed_at=frames;emit("action_pending",{label=pending.scene.label,reason="player_join_binding_waiting",roster=#roster,expected=expected})
   end
  elseif pending.scene.kind=="player-peer-exit" then
   local controllers={};for _,p in ipairs(PlayerManager.GetPlayers()) do controllers[p.ControllerIndex]=true end
   settled=controllers[0] and controllers[1] and controllers[2] and not controllers[3]
  elseif pending.scene.kind=="delirium-morph" then
   local d=assert(tracked[1]:ToDelirium());settled=d.BossType==EntityType.ENTITY_MONSTRO
   if not settled and (not pending.diagnosed_at or frames-pending.diagnosed_at>=120) then
    pending.diagnosed_at=frames;emit("action_pending",{label=pending.scene.label,reason="delirium_transform_waiting",boss_type=d.BossType,state=d.State,timer=d.TransformationTimer})
   end
  elseif pending.scene.kind=="laser" and pending.scene.mode=="chain" then
   -- 真实子束可能迟于初次几何更新；按活链等待，再取新的完整同窗口目标。
   local l=assert(tracked[1]:ToLaser());local b=l.BounceLaser
   settled=b~=nil and b:Exists() and b:ToLaser()~=nil
   if not settled and (not pending.diagnosed_at or frames-pending.diagnosed_at>=120) then
    pending.diagnosed_at=frames;emit("action_pending",{label=pending.scene.label,reason="laser_native_bounce_waiting",grid_hit=l.GridHit,endpoint={x=l.EndPoint.X,y=l.EndPoint.Y}})
   end
  elseif pending.scene.kind=="knife-out" or pending.scene.kind=="knife-return" then
   local k=assert(tracked[1]:ToKnife());local velocity=k:GetKnifeVelocity();local distance=k:GetKnifeDistance()
   settled=k:Exists() and k:IsFlying() and distance>0 and (pending.scene.kind=="knife-out" and velocity>0 or pending.scene.kind=="knife-return" and velocity<0)
   if not settled and (not pending.diagnosed_at or frames-pending.diagnosed_at>=120) then
    pending.diagnosed_at=frames;emit("action_pending",{label=pending.scene.label,reason="knife_actual_phase_waiting",exists=k:Exists(),flying=k:IsFlying(),distance=distance,velocity=velocity})
   end
  end
  if settled then
   if pending.scene.kind=="player-ghost" or pending.scene.kind=="player-land" or pending.scene.kind=="player-stress" or pending.scene.kind=="player-hold" or (pending.scene.kind=="player-flash" and pending.scene.cooldown==0) then emit("native_settled",{label=pending.scene.label}) end
   if pending.scene.kind=="player-input" then
    -- 记录真实原生模拟后的身体位移；Render边界中性化的输入Getter不能代替消费结果。
    for _,p in ipairs(PlayerManager.GetPlayers()) do local subject=IsaacCSObservationNative.Subject(p);local start=inputStart[tostring(subject.local_id)]
     assert(start,"input body changed during actual movement");inputProof[#inputProof+1]={subject=subject,displacement={p.Position.X-start[1],p.Position.Y-start[2]},velocity={p.Velocity.X,p.Velocity.Y}}
    end
    emit("input_complete",{label=pending.scene.label,players=inputProof})
   end
   if action<=(config.reuse_prefix or 0) and not pending.scene.replay_capture then
    -- 只重放已证明不受修复影响的准备动作；旧目标保留原Scope，绝不把本次未采样冒充新结果。
    emit("ready",{label=pending.scene.label,capture_token="reused-preparation"});pending=nil
   else capture={scene=pending.scene,after=IsaacCSObservationNative.SourceSnapshotToken()};pending=nil end
  end
 end
 if capture then
  local token,reason=IsaacCSObservationNative.SourceSnapshotToken()
  -- 等待失败必须带出正式域／字段首因；限频记录，不把Partial改作可冻结窗口。
  if token=="" and reason and reason~="" and (not capture.diagnosed_at or frames-capture.diagnosed_at>=120) then
   capture.diagnosed_at=frames;emit("capture_pending",{label=capture.scene.label,reason=reason})
  end
  if token~="" and token~=capture.after then
   held=true;restoreDamageDebug();emitReady(capture.scene.label,token);capture=nil
  end
 end
end)
