-- 统一tx17的Authority来源夹具；只准备真实样本，不直接写Client原生世界。
local mod=RegisterMod("Isaac CS client world E2E",1)
-- 只供有token的原生E2E入口访问真实ModReference，验证写入/删除被前置阻断。
IsaacCSClientTestMod=mod
local config=include("harness_config");local json=require("json")
local action=0;local effect
-- 来源夹具只冻结自己生成的无玩法展示Effect；Client仍由正式分派器阻断全部玩法。
mod:AddCallback(ModCallbacks.MC_PRE_EFFECT_UPDATE,function(_,e)
 if effect and GetPtrHash(e)==GetPtrHash(effect) then return true end
end,57)
local function emit(event,data)
 data=data or {};data.event=event;data.action=action;data.token=config.token
 Isaac.DebugString("[isaac-cs-test] tx17 "..json.encode(data))
end
mod:AddCallback(ModCallbacks.MC_POST_RENDER,function()
 if not Input.IsButtonTriggered(Keyboard.KEY_F8,0) then return end
 action=action+1
 local ok,err=pcall(function()
  if action==1 then
   -- 明确使用默认StageType=0，避免种子选择Burning Basement并持续生成余烬。
   Isaac.ExecuteCommand("stage 1")
  elseif action==2 then
   Isaac.ExecuteCommand("debug 10")
   for _,e in ipairs(Isaac.GetRoomEntities()) do if not e:ToPlayer() then e:Remove() end end
   local room=Game():GetRoom()
   -- 17选定原生迷失问号小地图分支；完整楼层拓扑展示由18接入，不依赖随机诅咒命中。
   local level=Game():GetLevel();level:RemoveCurses(level:GetCurses());level:AddCurse(LevelCurse.CURSE_OF_THE_LOST,false)
   for slot=0,7 do room:RemoveDoor(slot) end
   for index=0,room:GetGridSize()-1 do if room:GetGridEntity(index) then room:RemoveGridEntity(index,0,false) end end
   local p=Isaac.GetPlayer(0);if p:GetPlayerType()~=PlayerType.PLAYER_ISAAC then error("ordinary Isaac source required") end
   for id=1,CollectibleType.NUM_COLLECTIBLES-1 do while p:HasCollectible(id) do p:RemoveCollectible(id) end end
   for slot=0,1 do local id=p:GetTrinket(slot);if id~=0 then p:TryRemoveTrinket(id) end end
   p.Position=Game():GetRoom():GetCenterPos();p.Velocity=Vector.Zero;p:SetMinDamageCooldown(18000)
   effect=Isaac.Spawn(1000,57,0,p.Position+Vector(80,0),Vector.Zero,nil):ToEffect()
   effect.Timeout=-1;effect.LifeSpan=-1
   -- 57自身没有持续动画资源，使用实际玩家资源构造明确可见的原生Effect样本。
   local sprite=effect:GetSprite();sprite:Load(p:GetSprite():GetFilename(),true)
   sprite:SetFrame(p:GetSprite():GetAnimation(),0)
  end
 end)
 if not ok then emit("fixture_error",{reason=tostring(err)});return end
 emit("ready",{players=Game():GetNumPlayers()})
end)
