-- tx-04-18实景编排：只操作真实Authority。不给Client构造模型/pose，不提供任何替代贴图。
local mod=RegisterMod("Isaac CS original Room Grid E2E",1)
local json=require("json")
local config=include("harness_config")
local indices={}
local function room() return Game():GetRoom() end
local function prepare()
  for _,e in ipairs(Isaac.GetRoomEntities()) do if not e:ToPlayer() and not (e.Type==1000 and e.Variant==EffectVariant.RED_DOOR_OUTLINE) then e:Remove() end end
  for _,p in ipairs(PlayerManager.GetPlayers()) do p.Visible=false;p.Position=room():GetGridPosition(17);p.Velocity=Vector.Zero;p:SetMinDamageCooldown(18000) end
  Game():GetHUD():SetVisible(false)
end
local function spawn(typ,index,variant)
  if room():GetGridEntity(index) then room():RemoveGridEntityImmediate(index,0,false) end
  assert(room():SpawnGridEntity(index,typ,variant or 0,71000+typ,0),"original Grid spawn refused:"..typ)
  local g=assert(room():GetGridEntity(index));assert(g:GetType()==typ,"actual Grid type differs");return g
end
local function facts()
  local r=room();local grids={};for i=0,447 do local g=r:GetGridEntity(i);if g then grids[#grids+1]={index=i,type=g:GetType(),variant=g:GetVariant(),state=g.State,animation=g:GetSprite():GetAnimation(),filename=g:GetSprite():GetFilename()} end end
  return {room=Game():GetLevel():GetCurrentRoomIndex(),shape=r:GetRoomShape(),backdrop=r:GetBackdropType(),water=r:GetWaterAmount(),grids=grids}
end
IsaacCS_OriginalAction=function(name)
  if name=="seed" then Isaac.ExecuteCommand("seed S227 7MNF");return json.encode({seed_requested=true}) end
  if name=="facts" then return json.encode(facts()) end
  if name=="prepare" then prepare()
  elseif name=="types" then
    prepare();local n=0;for typ=1,27 do if typ~=16 then n=n+1;local at=room():GetGridIndex(Vector(160+((n-1)%8)*40,200+math.floor((n-1)/8)*40));indices[typ]=at;spawn(typ,at,typ==20 and 2 or 0) end end
  elseif name=="states" then
    assert(room():GetGridEntity(indices[7])):ToPit():MakeBridge(room():GetGridEntity(indices[2]));room():GetGridEntity(indices[7]):ToPit():SetLadder(true)
    room():GetGridEntity(indices[14]):Hurt(1)
    for slot=0,7 do local d=room():GetDoor(slot);if d then d:SetLocked(true);d:Close(true) end end
  elseif name=="destroy" then
    for _,typ in ipairs({2,14,12}) do local g=room():GetGridEntity(indices[typ]);if g then g:Destroy(false) end end
    for slot=0,7 do local d=room():GetDoor(slot);if d then d:SetLocked(false);d:Open() end end
  elseif name=="replace-old" then indices.replace=room():GetGridIndex(Vector(480,360));spawn(2,indices.replace)
  elseif name=="replace-new" then spawn(14,indices.replace)
  elseif name=="shape-large" or name=="shape-l" or name=="boss" then
    local shape=name=="shape-large" and RoomShape.ROOMSHAPE_2x2 or name=="shape-l" and RoomShape.ROOMSHAPE_LTL or RoomShape.ROOMSHAPE_1x1
    local typ=name=="boss" and RoomType.ROOM_BOSS or RoomType.ROOM_DEFAULT
    local target
    if name=="boss" then
      -- Boss配置来自此真实楼层已经生成的房间，不假设普通STB查询包含Boss资源集。
      local rooms=Game():GetLevel():GetRooms()
      for i=0,rooms.Size-1 do local d=rooms:Get(i);if d.Data and d.Data.Type==RoomType.ROOM_BOSS then target=d.Data;break end end
    else target=RoomConfig.GetRandomRoom(112233,false,Isaac.GetCurrentStageConfigId(),typ,shape) end
    assert(target,"actual original room config unavailable")
    Isaac.ExecuteCommand("goto "..(name=="boss" and "b." or "d.")..target.Variant)
    return json.encode({requested_shape=shape,requested_variant=target.Variant})
  elseif name=="cellar" then Isaac.ExecuteCommand("stage 1a");return json.encode({stage_requested="1a"})
  elseif name=="flooded" then Isaac.ExecuteCommand("stage 4b");return json.encode({stage_requested="4b"})
  elseif name=="water" then
    prepare();room():SetWaterAmount(0.7);room():SetWaterColor(KColor(0.3,0.5,0.8,0.7))
    for slot=0,7 do local d=room():GetDoor(slot);if d then d:SetWaterClipFlags(57) end end
  else error("unknown original scene action:"..name) end
  return json.encode(facts())
end
mod:AddCallback(ModCallbacks.MC_POST_GAME_STARTED,function() Isaac.DebugString("[isaac-cs-test] tx18 "..json.encode({event="run_started",token=config.token})) end)
