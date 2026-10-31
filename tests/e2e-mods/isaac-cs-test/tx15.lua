-- tx-04-15仅准备复制链路的真实值，不重复旧玩法验收；所有写操作由F8启动。
local mod=RegisterMod("Isaac CS replica contract E2E",1)
local config=include("harness_config");local json=require("json")
local action,serial=0,0;local pickup
local function emit(event,data)
 serial=serial+1;data=data or {};data.event=event;data.action=action;data.token=config.token;data.serial=serial
 Isaac.DebugString("[isaac-cs-test] tx15 "..json.encode(data))
end
mod:AddCallback(ModCallbacks.MC_POST_RENDER,function()
 if not Input.IsButtonTriggered(Keyboard.KEY_F8,0) then return end
 action=action+1
 local ok,err=pcall(function()
  if action==1 then
   Isaac.ExecuteCommand("seed S227 7MNF");Isaac.ExecuteCommand("debug 10")
  elseif action==2 then
   for _,e in ipairs(Isaac.GetRoomEntities()) do if e:ToNPC() then e:Remove() end end
   for _,p in ipairs(PlayerManager.GetPlayers()) do p.Position=Game():GetRoom():GetCenterPos();p.Velocity=Vector.Zero;p:SetMinDamageCooldown(18000) end
   pickup=Isaac.Spawn(5,20,1,Game():GetRoom():GetCenterPos()+Vector(80,0),Vector.Zero,nil)
   pickup:GetSprite().Color=Color(0.4,0.8,0.2,1)
  elseif action==3 then if pickup and pickup:Exists() then pickup:Remove() end
  end
 end)
 if not ok then emit("fixture_error",{reason=tostring(err)});return end
 emit("ready",{players=Game():GetNumPlayers()})
end)
