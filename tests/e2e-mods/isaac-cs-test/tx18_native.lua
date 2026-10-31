-- 统一E2E只编排真实Authority；Client没有测试专用状态setter或替代贴图。
local mod=RegisterMod("Isaac CS native Room Grid E2E",1)
local json=require("json")
local indices,initial,doors={},{},{}
local function room() return Game():GetRoom() end
local function prepare()
  for _,e in ipairs(Isaac.GetRoomEntities()) do
    if not e:ToPlayer() and not(e.Type==1000 and e.Variant==EffectVariant.RED_DOOR_OUTLINE) then e:Remove() end
  end
  for _,p in ipairs(PlayerManager.GetPlayers()) do p.Visible=false;p.Position=room():GetGridPosition(17);p.Velocity=Vector.Zero;p:SetMinDamageCooldown(18000) end
  Game():GetHUD():SetVisible(false)
end
local function spawn(typ,index,variant)
  if room():GetGridEntity(index) then room():RemoveGridEntityImmediate(index,0,false) end
  assert(room():SpawnGridEntity(index,typ,variant or 0,71000+typ,0),"native Authority spawn refused:"..typ)
  local g=assert(room():GetGridEntity(index));assert(g:GetType()==typ,"Authority factory type differs");return g
end
local function facts()
  local r=room();local grids={}
  for i=0,447 do local g=r:GetGridEntity(i);if g then grids[#grids+1]={index=i,type=g:GetType(),variant=g:GetVariant(),state=g.State} end end
  return {room=Game():GetLevel():GetCurrentRoomIndex(),shape=r:GetRoomShape(),backdrop=r:GetBackdropType(),water=r:GetWaterAmount(),grids=grids}
end
IsaacCS_OriginalAction=function(name)
  if name=="seed" then Isaac.ExecuteCommand("seed S227 7MNF");return json.encode({seed_requested=true}) end
  if name=="facts" then return json.encode(facts()) end
  if name=="prepare" then prepare()
  elseif name=="types" then
    prepare();local n=0
    for typ=1,27 do if typ~=16 then n=n+1;local at=room():GetGridIndex(Vector(160+((n-1)%8)*40,200+math.floor((n-1)/8)*40));indices[typ]=at
      local g=spawn(typ,at,typ==20 and 2 or 0);initial[typ]={state=g.State,var=g.VarData,collision=g.CollisionClass,animation=g:GetSprite():GetAnimation()}
    end end
    for slot=0,7 do local d=room():GetDoor(slot);if d then doors[slot]={locked=d:IsLocked(),open=d:IsOpen()} end end
  elseif name=="states" then
    room():GetGridEntity(indices[7]):ToPit():MakeBridge(room():GetGridEntity(indices[2]));room():GetGridEntity(indices[7]):ToPit():SetLadder(true)
    room():GetGridEntity(indices[14]):Hurt(1)
    for slot=0,7 do local d=room():GetDoor(slot);if d then d:SetLocked(true);d:Close(true) end end
  elseif name=="states-return" then
    -- 真正在Authority恢复先前状态，再重新观测；没有把基线DTO直接改成正常输入。
    local g=room():GetGridEntity(indices[7]);g:ToPit():SetLadder(false);g.State=initial[7].state;g.VarData=initial[7].var;g:ToPit():UpdateCollision();g:PostInit()
    for slot,state in pairs(doors) do local d=room():GetDoor(slot);if d then d:SetLocked(state.locked);if state.open then d:Open() else d:Close(true) end end end
  elseif name=="destroy" then
    for _,typ in ipairs({2,14,12}) do local g=room():GetGridEntity(indices[typ]);if g then g:Destroy(false) end end
    for slot=0,7 do local d=room():GetDoor(slot);if d then d:SetLocked(false);d:Open() end end
  elseif name=="replace-old" then indices.replace=room():GetGridIndex(Vector(480,360));spawn(2,indices.replace)
  elseif name=="replace-new" then spawn(14,indices.replace)
  elseif name=="empty" then
    -- 门的公开owner同时解除门视图；随后立即移除所有剩余Grid，形成真实完整空成员集。
    for slot=0,7 do room():RemoveDoor(slot) end
    for i=0,447 do if room():GetGridEntity(i) then room():RemoveGridEntityImmediate(i,0,false) end end
  elseif name:match("^shape%-%d+$") or name=="boss" then
    local shape=tonumber(name:match("%d+$")) or 1;assert(shape>=1 and shape<=12)
    local target
    if name=="boss" then local list=Game():GetLevel():GetRooms();for i=0,list.Size-1 do local d=list:Get(i);if d.Data and d.Data.Type==RoomType.ROOM_BOSS then target=d.Data;break end end
    else target=RoomConfig.GetRandomRoom(112233,false,Isaac.GetCurrentStageConfigId(),RoomType.ROOM_DEFAULT,shape) end
    assert(target,"original STB has no requested shape:"..shape)
    Isaac.ExecuteCommand("goto "..(name=="boss" and "b." or "d.")..target.Variant)
    return json.encode({requested_shape=shape,variant=target.Variant})
  elseif name=="cellar" then Isaac.ExecuteCommand("stage 1a");return json.encode({stage="1a"})
  elseif name=="flooded" then Isaac.ExecuteCommand("stage 4b");return json.encode({stage="4b"})
  elseif name=="womb" then Isaac.ExecuteCommand("stage 7");return json.encode({stage="7"})
  elseif name=="water" then
    prepare();room():SetWaterAmount(0.7);room():SetWaterColor(KColor(0.3,0.5,0.8,0.7));room():SetFloorColor(Color(0.7,0.8,1,1,0,0,0));room():SetWallColor(Color(0.8,0.9,1,1,0,0,0))
    for slot=0,7 do local d=room():GetDoor(slot);if d then d:SetWaterClipFlags(57) end end
  else error("unknown native Authority action:"..name) end
  -- 动作可能生成雕像显示Entity或破坏碎片；本域比较前再排除这些19/20的对象。
  prepare()
  return json.encode(facts())
end
-- 显式控制口已能调用夹具，不依赖按键或Client回调驱动。
mod:AddCallback(ModCallbacks.MC_POST_GAME_STARTED,function() Isaac.DebugString("[isaac-cs-test] tx18 native Authority started") end)
