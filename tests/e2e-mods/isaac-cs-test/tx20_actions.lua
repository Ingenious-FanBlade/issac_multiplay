-- Source专用玩法夹具：所有动作由F8显式进入，正式observer不借此补值。
return function(ctx)
 local weaponPhase,esauPhase,forgottenPhase,holdPending,stressPhase,stressSetup
 if ctx.source then
  -- 只在真实Player Update后经公开API装备／设置本场景值；采集仍由正式普通尾部完成。
  -- 每次重新借用当前槽，不跨生命周期保存Weapon userdata；冻结后不再有更新回调。
  ctx.mod:AddCallback(ModCallbacks.MC_POST_PLAYER_UPDATE,function(_,p)
   local phase=weaponPhase and p.ControllerIndex==0 and p:GetPlayerType()==0 and weaponPhase
   if not phase and stressPhase then local id=IsaacCSObservationNative.Subject(p);phase=stressPhase[tostring(id.local_id)..":"..tostring(id.use)] end
   if not phase then return end
   local w=p:GetWeapon(phase.slot)
   -- 正常更新可撤掉未激活的备用武器；夹具用同一公开工厂重新装备实际对象，绝不补JSON值。
   if not w or w:GetWeaponType()~=phase.type then
    if w then Isaac.DestroyWeapon(w) end
    w=assert(Isaac.CreateWeapon(phase.type,p));p:SetWeapon(w,phase.slot)
   end
   w:SetCharge(phase.charge)
  end)
 end
 local function roots()
  local result,seen={},{}
  for _,p in ipairs(PlayerManager.GetPlayers()) do
   if not seen[p.ControllerIndex] then seen[p.ControllerIndex]=true;result[#result+1]=p end
  end
  return result
 end
 local function change(t)
  local p=Isaac.GetPlayer(0);p:ChangePlayerType(t);return Isaac.GetPlayer(0)
 end
 -- 每次动作重新从真实roster借35/40，不跨换形缓存原生userdata；关系由公开getter与稳定身份共同检查。
 local function sameEntity(a,b)
  if not a or not b then return false end
  local x,y=IsaacCSObservationNative.Subject(a),IsaacCSObservationNative.Subject(b)
  return x.resolution=="KnownCurrent" and y.resolution=="KnownCurrent" and x.local_id~="0" and x.session==y.session and x.epoch==y.epoch and x.local_id==y.local_id and x.use==y.use
 end
 local function taintedPair(required)
  local body
  for _,r in ipairs(PlayerManager.GetPlayers()) do if r.ControllerIndex==0 then
   if r:GetPlayerType()==35 then assert(not body,"multiple host Tainted Forgotten bodies");body=r end
  end end
  local soul=body and body:GetOtherTwin()
  if not body or not soul or IsaacCSObservationNative.Subject(soul).resolution~="KnownCurrent" or soul:GetPlayerType()~=40 or not sameEntity(soul:GetOtherTwin(),body) then
   assert(required==false,"real Tainted Forgotten pair missing");return nil
  end
  return body,soul
 end
 local function resources()
  for i,p in ipairs(PlayerManager.GetPlayers()) do
   p.Position=Game():GetRoom():GetCenterPos()+Vector((i-1)%4*45-70,math.floor((i-1)/4)*50)
   p.Velocity=Vector.Zero;p:AddCoins(i+3);p:AddKeys(i+1);p:AddBombs(i+2)
   p:SetMinDamageCooldown(0)
  end
 end
 -- 在四对真实19/20及控制器落定后才装备，不向尚在初始化、controller=-1的副体调用装备API。
 local function equipStress()
  local types={4,8,11,15,1,2,6,14};stressPhase={}
  for i,r in ipairs(PlayerManager.GetPlayers()) do
   for item=1,5 do r:AddCollectible(item,0,false) end
   for slot=0,4 do local old=r:GetWeapon(slot);if old then Isaac.DestroyWeapon(old) end end
   for kind=1,15 do r:EnableWeaponType(kind,false) end
   local kind=assert(types[i],"stress roster exceeds eight");local slot=(kind==11 or kind==12) and 0 or 1
   local w=assert(Isaac.CreateWeapon(kind,r));r:SetWeapon(w,slot);w:SetCharge(0.5);r:EnableWeaponType(kind,true)
   local id=IsaacCSObservationNative.Subject(r);stressPhase[tostring(id.local_id)..":"..tostring(id.use)]={type=kind,slot=slot,charge=0.5}
  end;resources()
 end
 local function perform(s)
  weaponPhase=nil;stressPhase=nil;stressSetup=nil
  local p=Isaac.GetPlayer(0)
  if s.kind=="player-type" then
   ctx.clear();p=change(s.player_type);resources()
  elseif s.kind=="player-count" or s.kind=="player-input" then resources()
  elseif s.kind=="player-jacobs" then
   ctx.clear();for _,r in ipairs(roots()) do r:ChangePlayerType(19) end;resources()
  elseif s.kind=="player-stress" then
   ctx.clear();for _,r in ipairs(roots()) do r:ChangePlayerType(0) end;stressSetup="ordinary"
  elseif s.kind=="player-mixed" then
   local types={16,35,29,0};for i,r in ipairs(roots()) do r:ChangePlayerType(types[i]);end;resources()
  elseif s.kind=="player-soul" then
   if p:GetPlayerType()~=16 then p=change(16) end
   assert(p:GetSubPlayer(),"Forgotten sub body missing"):AddSoulHearts(4)
   assert(p:SwapForgottenForm(true,true),"real Forgotten swap failed")
  elseif s.kind=="player-flip" then
   if p:GetPlayerType()~=29 and p:GetPlayerType()~=38 then p=change(29) end
   p:UseActiveItem(711,UseFlag.USE_NOANIM|UseFlag.USE_NOANNOUNCER,-1)
  elseif s.kind=="player-birthright" then
   -- 获得Birthright与使用Flip分别经过普通更新／采集，覆盖两个独立原版后态。
   -- 不在同一Lua栈合并获得道具与切换，亦不把由此产生的过渡成员次数归零。
   assert(p:GetPlayerType()==29 or p:GetPlayerType()==38,"Lazarus form required")
   p:AddCollectible(619,0,false)
  elseif s.kind=="player-birthright-clear" then
   -- 先只读定位实际持有者，再执行一次原版移除；不手工摘hologram或改twin/backup指针。
   local holder
   for _,r in ipairs(PlayerManager.GetPlayers()) do
    if (r:GetPlayerType()==29 or r:GetPlayerType()==38) and r:GetCollectibleNum(619)>0 then
     assert(not holder,"unexpected multiple Birthright holders");holder=r
    end
   end
   assert(holder,"Birthright holder missing"):RemoveCollectible(619)
  elseif s.kind=="player-hold" then
   ctx.clear();p=change(35);resources();holdPending={wait=0}
  elseif s.kind=="player-throw" then
   local body,soul=taintedPair();assert(sameEntity(soul:GetHeldEntity(),body),"Tainted Forgotten was not held")
   local id=IsaacCSObservationNative.Subject(body)
   forgottenPhase={subject=tostring(id.local_id)..":"..tostring(id.use),x=body.Position.X,y=body.Position.Y,landing=false}
   assert(body:TryForgottenThrow(Vector(1,0)),"real Tainted Forgotten throw failed")
  elseif s.kind=="player-land" then
   assert(forgottenPhase,"Tainted Forgotten throw phase missing");forgottenPhase.landing=true;forgottenPhase.label=s.label
  elseif s.kind=="player-esau" then
   assert(not esauPhase,"previous Esau action still pending")
   -- 与09已验证方式一致：这里只准备真实拥有槽，使用由四组统一输入触发；不以Lua效果调用代替。
   local before,prepared={},{};for _,r in ipairs(roots()) do
    local id=IsaacCSObservationNative.Subject(r);before[r.ControllerIndex]=tostring(id.local_id)..":"..tostring(id.use)
    if r:GetActiveItem(0)~=703 then r:AddCollectible(703,1,false) end;r:SetActiveCharge(1,0)
    prepared[#prepared+1]={controller=r.ControllerIndex,subject=id,item=r:GetActiveItem(0),charge=r:GetActiveCharge(0)}
   end
   esauPhase={before=before};ctx.emit("esau_prepared",{label=s.label,roots=prepared})
  elseif s.kind=="player-health" then
   ctx.clear();p=change(0);p:AddMaxHearts(8);p:AddHearts(5);p:AddSoulHearts(3);p:AddBlackHearts(2)
   p:AddBoneHearts(2);p:AddRottenHearts(1);p:AddEternalHearts(1);p:AddGoldenHearts(1);p:AddBrokenHearts(1);resources()
  elseif s.kind=="player-inventory" then
   p:AddCollectible(1,0,false);p:AddCollectible(2,0,false);p:AddCollectible(534,0,false)
   p:AddTrinket(1|32768,false);p:AddTrinket(2,false);p:UseActiveItem(479,UseFlag.USE_NOANIM|UseFlag.USE_NOANNOUNCER,-1)
   p:GetEffects():AddCollectibleEffect(34,false,2);p:GetEffects():AddTrinketEffect(1,false,2)
  elseif s.kind=="player-empty" then
   p:RemoveCollectible(1);p:RemoveCollectible(2);p:GetEffects():ClearEffects()
  elseif s.kind=="player-slots" then
   p:AddCollectible(534,0,false);p:AddCollectible(34,4,false,0);p:AddCollectible(33,3,false,1)
   p:SetPocketActiveItem(105,2,true);p:SetPocketActiveItem(36,3,true)
   for slot=0,3 do p:SetActiveCharge(slot+1,slot);p:AddActiveCharge(slot,slot,false,true,true) end
   for slot=0,3 do p:SetCard(slot,slot+1) end
  elseif s.kind=="player-equipment-final" then
   -- 使用公开Source接口制造非空／混合槽及正式Plus容器；Client仅安装其最终值。
   p:AddCollectible(534,0,false);p:AddCollectible(34,2,false,0);p:AddCollectible(33,3,false,1)
   -- pocket数组的顺序独立于active slot：原生入口把两口袋主动排在前，再在后两格放卡／药。
   p:SetPocketActiveItem(105,2,true);p:SetPocketActiveItem(36,3,true);p:SetCard(2,1);p:SetPill(3,1)
   p:AddCollectible(CollectibleType.COLLECTIBLE_4_5_VOLT,0,false)
   local d=p:GetActiveItemDesc(0);d.Charge=1;d.BatteryCharge=0;d.PartialCharge=0.5;d.SubCharge=2;d.TimedRechargeCooldown=90
   p:GetActiveItemDesc(1).BatteryCharge=1
   p:AddCollectible(1,0,false)
   p:AddInnateCollectible(3,2,"tx20-permanent",-1,true);p:AddInnateCollectible(4,1,"tx20-timed",900,true)
   p:AddInnateTrinket(1,2,"tx20-permanent",-1,true);p:AddInnateTrinket(2,1,"tx20-timed",900,true)
   p:BlockCollectible(1);p:BlockTrinket(1)
  elseif s.kind=="player-equipment-empty" then
   p:ClearInnateItemGroup("tx20-permanent");p:ClearInnateItemGroup("tx20-timed");p:UnblockCollectible(1);p:UnblockTrinket(1)
   p:SetPocketActiveItem(0,2,true);p:SetPocketActiveItem(0,3,true);for slot=0,3 do p:SetCard(slot,0) end
   for _,item in ipairs({34,33,105,36}) do p:RemoveCollectible(item) end
  elseif s.kind=="player-crafting" then
   ctx.clear();p=change(23);p:SetBagOfCraftingContent({1,2,3,4,5,6,7,8})
  elseif s.kind=="player-poop" then
   ctx.clear();p=change(25);for slot=0,5 do p:SetPoopSpell(slot,slot+1) end
  elseif s.kind=="player-clot" then ctx.clear();p=change(26);p:AddHearts(6);p:SpawnClot(p.Position+Vector(45,0),false)
  elseif s.kind=="player-wisp" then ctx.clear();p=change(18);p:AddWisp(34,p.Position,true)
  elseif s.kind=="player-locust" then ctx.clear();p=change(34);p:AddLocust(1,p.Position)
  elseif s.kind=="player-rage" then ctx.clear();p=change(27);p:UseActiveItem(704,UseFlag.USE_NOANIM|UseFlag.USE_NOANNOUNCER,-1)
  elseif s.kind=="player-dark-esau" then
   ctx.clear();p=change(37)
   -- 原版EntityType.ENTITY_DARK_ESAU=866；真实NPC参与普通更新，目标/私有后态仍由正式observer采集。
   ctx.spawn(866,0,0,Vector(100,-30),p)
  elseif s.kind=="player-weapon-base" then
   ctx.clear();p=change(0);p:SetItemState(0);p:SetItemStateCooldown(0);p:GetEffects():ClearEffects()
   for kind=1,15 do p:EnableWeaponType(kind,false) end;p:EnableWeaponType(1,true)
  elseif s.kind=="player-weapon" then
   ctx.clear();assert(p:GetPlayerType()==0,"weapon base has not settled")
   -- Wiki明确0为备用、1为主武器；先用正式Destroy清槽，再交付指定槽的真实武器。
   for slot=0,4 do local old=p:GetWeapon(slot);if old then Isaac.DestroyWeapon(old) end end
   for kind=1,15 do p:EnableWeaponType(kind,false) end
   local slot=(s.weapon_type==11 or s.weapon_type==12) and 0 or 1
   local w=assert(Isaac.CreateWeapon(s.weapon_type,p));p:SetWeapon(w,slot);w:SetCharge(s.charge or 0)
   p:EnableWeaponType(s.weapon_type,true)
   weaponPhase={type=s.weapon_type,slot=slot,charge=s.charge or 0}
  elseif s.kind=="player-pose" then
   local sp=p:GetSprite();sp.Scale=Vector(1.2,0.85);sp.Rotation=13;p:GetBodySprite().Color=Color(0.7,0.9,1,0.8)
  elseif s.kind=="player-flash" then
   -- SetMin只提高下限；清除场景等待原版计时自然归零，不能把传0冒充清除。
   if s.cooldown>0 then p:SetMinDamageCooldown(s.cooldown) end
  elseif s.kind=="player-peer-exit" then resources()
  elseif s.kind=="player-death" then p:Kill()
  elseif s.kind=="player-ghost" then -- 只等待原版死亡动画转合作ghost，不重放死亡或写生命标志。
  elseif s.kind=="player-revive" then assert(p:ReviveCoopGhost(),"real coop revive failed")
  elseif s.kind=="player-minecart" then
   -- REPENTOGON公开入口创建实际NPC与EntityPtr关联，Client只安装其最终结果。
   ctx.clear();assert(p:GiveMinecart(p.Position,Vector.Zero),"real minecart creation failed")
  elseif s.kind=="player-joint" then
   resources();local r=Game():GetRoom();local ix=r:GetGridIndex(r:GetCenterPos()+Vector(60,40))
   if r:GetGridEntity(ix) then r:RemoveGridEntityImmediate(ix,0,false) end
   assert(r:SpawnGridEntity(ix,2,0,202020,0));ctx.spawn(10,0,0,Vector(80,-40),p)
  else return false end
  return true
 end
 return perform,function(scene)
  if scene.kind=="player-stress" and stressSetup then
   local roster=PlayerManager.GetPlayers()
   if stressSetup=="ordinary" then
    if #roster~=4 then return false end
    for _,p in ipairs(roster) do if p:GetPlayerType()~=0 or p:GetOtherTwin() then return false end end
    for _,r in ipairs(roots()) do r:ChangePlayerType(19) end;stressSetup="jacobs";return false
   elseif stressSetup=="jacobs" then
    if #roster~=8 then return false end
    local counts={};for _,p in ipairs(roster) do local t=p:GetPlayerType();local id=p.ControllerIndex
     local twin=p:GetOtherTwin()
     if (t~=19 and t~=20) or id<0 or id>3 or not twin or twin:GetPlayerType()~=39-t or not sameEntity(twin:GetOtherTwin(),p) then return false end
     counts[id]=(counts[id] or 0)+1
    end
    for id=0,3 do if counts[id]~=2 then return false end end
    equipStress();stressSetup="equipped";return false
   end
   stressSetup=nil
  end
  if scene.kind=="player-hold" and holdPending then
   holdPending.wait=holdPending.wait+1
   local body,soul=taintedPair(false);if not body then
    if holdPending.wait==1 or holdPending.wait%120==0 then
     local roster={};for _,p in ipairs(PlayerManager.GetPlayers()) do local twin=p:GetOtherTwin()
      roster[#roster+1]={player_type=p:GetPlayerType(),controller=p.ControllerIndex,subject=IsaacCSObservationNative.Subject(p),twin=twin and IsaacCSObservationNative.Subject(twin) or {resolution="Null"}}
     end
     ctx.emit("action_pending",{label=scene.label,reason="tainted_pair_waiting",roster=roster})
    end;return false
   end
   assert(soul:TryHoldEntity(body),"real Tainted Forgotten hold failed")
   assert(sameEntity(soul:GetHeldEntity(),body),"Tainted Forgotten hold relationship missing")
   holdPending=false;return false
  end
  if scene.kind=="player-ghost" then return Isaac.GetPlayer(0):IsCoopGhost() end
  if scene.kind=="player-flash" and scene.cooldown==0 then return Isaac.GetPlayer(0):GetDamageCooldown()==0 end
  if forgottenPhase and forgottenPhase.landing then
   local body,soul=taintedPair();local id=IsaacCSObservationNative.Subject(body)
   assert(forgottenPhase.subject==tostring(id.local_id)..":"..tostring(id.use),"Tainted Forgotten body changed while thrown")
   if body:HasEntityFlags(EntityFlag.FLAG_THROWN) or body:HasEntityFlags(EntityFlag.FLAG_HELD) or soul:GetHeldEntity() or body.Velocity:Length()>0.1 or body.PositionOffset.Y~=0 then return false end
   local displacement=(body.Position-Vector(forgottenPhase.x,forgottenPhase.y)):Length()
   assert(displacement>5,"Tainted Forgotten did not actually move after throw")
   ctx.emit("forgotten_landed",{label=forgottenPhase.label,subject=id,displacement=displacement,position={body.Position.X,body.Position.Y},velocity={body.Velocity.X,body.Velocity.Y}})
   forgottenPhase=nil
  end
  if not esauPhase then return true end
  local count=0;for _,r in ipairs(roots()) do
   local old=esauPhase.before[r.ControllerIndex];local id=IsaacCSObservationNative.Subject(r)
   if not old or old==tostring(id.local_id)..":"..tostring(id.use) then return false end;count=count+1
  end
  local backups=0;for slot=0,3 do if PlayerManager.GetEsauJrState(slot) then backups=backups+1 end end
  if count~=4 or backups~=4 then return false end;esauPhase=nil;return true
 end
end
