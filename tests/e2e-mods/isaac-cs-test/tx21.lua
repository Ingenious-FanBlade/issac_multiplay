-- I32：仅Host布置/补充真实负载；Client没有玩法回调，不导入或生成世界。
local config=include("harness_config")
if config.node_role~="Host" then return end
assert(config.tx=="tx-04-21" and config.token and config.token~="","tx21 explicit E2E token required")
local mod=RegisterMod("Isaac CS realtime Host E2E",1)
local json=require("json")
local actions=json.decode(config.actions_json or "[]")
local step,frame,seed=0,0,212121
local active=false;local attackFloor=16;local pending=nil;local stopped=false
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
 for _,e in ipairs(Isaac.GetRoomEntities()) do
  if e:IsActiveEnemy(false) and not e:IsDead() then npcs=npcs+1;if e.Velocity:Length()>0.01 then moving=moving+1 end end
  if e.Type==2 or e.Type==4 or e.Type==7 or e.Type==8 or e.Type==9 then attacks=attacks+1 end
 end
 for _,p in ipairs(PlayerManager.GetPlayers()) do players[#players+1]={subject=IsaacCSObservationNative.Subject(p),type=p:GetPlayerType(),controller=p.ControllerIndex,x=p.Position.X,y=p.Position.Y} end
 return {npc=npcs,attacks=attacks,moving_npc=moving,bodies=players,participants=#roots()}
end
local function supply()
 local room=Game():GetRoom();local c=counts();local center=room:GetCenterPos()
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
 local room=Game():GetRoom();local p=Isaac.GetPlayer(0);local center=room:GetCenterPos()
 if a=="ordinary" or a=="dual" or a=="mixed" then
  active=false;local rs=roots();assert(#rs==config.participants,"real frozen participants missing")
  for i,r in ipairs(rs) do r:ChangePlayerType(a=="dual" and 19 or a=="mixed" and (i==#rs and 29 or 19) or 0) end
  for _,r in ipairs(PlayerManager.GetPlayers()) do r:AddCoins(5);r:AddKeys(5);r:AddBombs(5);r:AddSoulHearts(12);r:SetMinDamageCooldown(0);r:AddCollectible(34,6,false);r:SetCard(0,2);r:AddTrinket(1) end
  attackFloor=config.participants==4 and 32 or 16;active=true;supply()
 elseif a=="combat" then active=true;supply()
 elseif a=="laser" then p:AddCollectible(68,0,false)
 elseif a=="bomb-grid" then
  local index=room:GetGridIndex(center+Vector(80,0));Isaac.GridSpawn(GridEntityType.GRID_ROCK,0,room:GetGridPosition(index),true)
  local b=p:FireBomb(room:GetGridPosition(index),Vector.Zero);b:SetExplosionCountdown(30)
 elseif a=="door" then
  local found=false;for slot=0,7 do local d=room:GetDoor(slot);if d then d:Open();found=true;break end end
  assert(found,"real door required")
 elseif a=="pickup" then spawn(5,20,1,p.Position+Vector(20,0),Vector.Zero,nil)
 elseif a=="resources" then p:AddCoins(3);p:AddKeys(2);p:AddBombs(1)
 elseif a=="create-remove" then
  local e=spawn(5,20,1,center,Vector.Zero,nil);pending={remove=e,after=frame+10}
 elseif a=="zero-empty" then
  active=false;p:AddCoins(-p:GetNumCoins());p:AddKeys(-p:GetNumKeys());p:AddBombs(-p:GetNumBombs())
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
 elseif a=="stop-supply" then active=false
 else error("unknown tx21 action: "..tostring(a)) end
 emit("action",{kind=a,actual=counts()})
end
mod:AddCallback(ModCallbacks.MC_POST_UPDATE,function()
 if stopped then return end
 frame=frame+1
 local ok,err=pcall(function()
  if pending and frame>=pending.after then if pending.remove and pending.remove:Exists() then pending.remove:Remove() end;pending=nil end
  if active then supply() end
  if frame%30==0 then emit("load",counts()) end
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
