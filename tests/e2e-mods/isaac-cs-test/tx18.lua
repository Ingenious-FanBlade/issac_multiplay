-- I28失败模型见实施计划第7节：只准备Source真实世界，Client只走公共applier。
local mod=RegisterMod("Isaac CS Room Grid application E2E",1)
IsaacCSClientTestMod=mod
local config=include("harness_config");local json=require("json")
local scenes=json.decode(config.scenes_json);local action,frames=0,0
local pending=nil;local frozen=false;local slots={};local baseline={}
local big_anchor=nil
local timer_sheet=nil
local capture_wait=nil;local source_held=false
local function emit(event,data)
 data=data or {};data.event=event;data.action=action;data.token=config.token
 Isaac.DebugString("[isaac-cs-test] tx18 "..json.encode(data))
end
local special=include("tx12_special")
special.Start(function(event,data)
 emit(event,data)
 if event=="coverage_ready" and pending then pending.at=frames+15 end
end)
mod:AddCallback(ModCallbacks.MC_POST_NEW_ROOM,special.NewRoom)
mod:AddCallback(ModCallbacks.MC_POST_NEW_LEVEL,special.NewLevel)
-- 官方公开pre回调返回false取消update；只冻结测试Source，保留真实构造/官方资源。
for _,name in ipairs({"DECORATION","DOOR","FIRE","GRAVITY","LOCK","PIT","POOP","PRESSUREPLATE","ROCK","SPIKES","STAIRCASE","STATUE","TELEPORTER","TNT","TRAPDOOR","WEB"}) do
 local id=ModCallbacks["MC_PRE_GRID_ENTITY_"..name.."_UPDATE"]
 if id then mod:AddCallback(id,function() if frozen and not config.network_regression then return false end end) end
end
-- 环境会在清理后的同tick继续生成雨滴/水花；在真实Init回调退休非代表Effect。
-- 仅测试Source构造普通Player/既有57承载，不从正式目标过滤存活Entity；雨水Room字段仍完整采集。
-- 正式导出保留完整源，Entity投影单列Exists=false退休前缀；不删除Grid/环境字段。
mod:AddCallback(ModCallbacks.MC_POST_EFFECT_INIT,function(_,e) if e.Variant~=57 then e:Remove() end end)
local function clean()
 for _,e in ipairs(Isaac.GetRoomEntities()) do if not e:ToPlayer() then e:Remove() end end
 -- 每次Render会重新保护玩家；选可见半周期，避免ordinary的-1/-2帧与Render重设值跨闪隐边界。
 local p=Isaac.GetPlayer(0);p.Position=Game():GetRoom():GetCenterPos();p.Velocity=Vector.Zero;p:SetMinDamageCooldown(18005)
 while p:GetNumBlueFlies()>0 do p:RemoveBlueFly() end
 while p:GetNumBlueSpiders()>0 do p:RemoveBlueSpider() end
 p.EntityCollisionClass=EntityCollisionClass.ENTCOLL_NONE
 p.GridCollisionClass=Game():GetRoom():GetType()==RoomType.ROOM_DUNGEON and EntityGridCollisionClass.GRIDCOLL_GROUND or EntityGridCollisionClass.GRIDCOLL_NONE
end
local function spawn(index,typ,seed,variant)
 local r=Game():GetRoom();if r:GetGridEntity(index) then r:RemoveGridEntityImmediate(index,0,false) end
 assert(r:SpawnGridEntity(index,typ,variant or 0,seed,0),"spawn false:"..typ)
 local g=assert(r:GetGridEntity(index));assert(g:GetType()==typ,"spawn actual type:"..typ);return g
end
local function allgrids(dense)
 clean();local r=Game():GetRoom();slots={};local cells={}
 for i=0,r:GetGridSize()-1 do
  local pos=r:GetGridPosition(i)
  if r:IsPositionInRoom(pos,40) then cells[#cells+1]=i end
 end
 assert(#cells>=26,"insufficient real interior cells")
 -- 把27类放在真实镜头中心附近，使所有绘制家族可实际看见；其余密集格继续铺满合法内部。
 local center=r:GetCenterPos()
 table.sort(cells,function(a,b)
  local da=(r:GetGridPosition(a)-center):LengthSquared();local db=(r:GetGridPosition(b)-center):LengthSquared()
  return da==db and a<b or da<db
 end)
 local n=0
 for typ=1,27 do if typ~=16 then n=n+1;slots[typ]=cells[n];spawn(cells[n],typ,18000+typ) end end
 if dense then for i=n+1,#cells do spawn(cells[i],2,19000+i) end end
 baseline={};for typ,index in pairs(slots) do local g=r:GetGridEntity(index);baseline[typ]={state=g.State,var=g.VarData} end
end
local function layout(shape,typ)
 frozen=false;typ=typ or RoomType.ROOM_DEFAULT
 local c=assert(RoomConfig.GetRandomRoom(180001,false,typ==RoomType.ROOM_DEFAULT and Isaac.GetCurrentStageConfigId() or StbType.SPECIAL_ROOMS,typ,shape or RoomShape.ROOMSHAPE_1x1),"stock room config missing")
 local prefix=typ==RoomType.ROOM_DEFAULT and "d" or typ==RoomType.ROOM_TREASURE and "s.treasure" or typ==RoomType.ROOM_SACRIFICE and "s.sacrifice" or typ==RoomType.ROOM_SHOP and "s.shop" or "s.challenge"
 Isaac.ExecuteCommand("goto "..prefix.."."..c.Variant)
end
local function perform(s)
 local r=Game():GetRoom();local l=Game():GetLevel()
 if s.kind=="setup" then
  frozen=false;Isaac.ExecuteCommand("stage 1");Isaac.ExecuteCommand("debug 10")
 elseif s.kind=="reset-floor" then
  frozen=false;big_anchor=nil;Isaac.ExecuteCommand("stage 1")
 elseif s.kind=="special-case" then frozen=false;special.Perform("case:"..s.value)
 elseif s.kind=="map-big" then
  frozen=false
  if not big_anchor then
   local c=assert(RoomConfig.GetRandomRoom(180002,false,Isaac.GetCurrentStageConfigId(),RoomType.ROOM_DEFAULT,RoomShape.ROOMSHAPE_2x2))
   for i=0,168 do if l:CanPlaceRoom(c,i,0,true,true,true) then
    local d=l:TryPlaceRoom(c,i,0,181819,true,true,true);if d then big_anchor=d.GridIndex;break end
   end end
  end
  assert(big_anchor,"public 2x2 map placement failed")
  Game():StartRoomTransition(big_anchor+s.value,Direction.NO_DIRECTION,RoomTransitionAnim.FADE,nil,0)
 elseif s.kind=="override" then
  -- 用公开descriptor接口真实进入双配置承载；不是给Client补写配置来源。
  frozen=false;local first=assert(l:GetCurrentRoomDesc().Data);local second
  local set=RoomConfig.GetStage(Isaac.GetCurrentStageConfigId()):GetRoomSet(0)
  for i=0,set.Size-1 do local candidate=set:Get(i)
   if candidate.Type==first.Type and candidate.Shape==first.Shape and candidate.Variant~=first.Variant then second=candidate;break end
  end
  assert(second,"distinct same-shape stock OverrideData missing")
  local d=l:GetRoomByIdx(-2,0);d:GetEntitiesSaveState():Clear();d:GetDecoSaveState():Clear();d:GetGridEntitiesSaveState():Clear()
  d.Data=first;d.OverrideData=second;d.VisitedCount=0;d.Clear=false;d.ClearCount=0;d.NoReward=true;d.Flags=0
  local rng=RNG();rng:SetSeed(181818,35);d:InitSeeds(rng)
  Game():StartRoomTransition(-2,Direction.NO_DIRECTION,RoomTransitionAnim.FADE,nil,0)
 elseif s.kind=="shape" then layout(s.value)
 elseif s.kind=="special" then layout(1,s.value)
 elseif s.kind=="boss-backdrop" then
  -- 在Normal阶段进入Level已生成的真实Boss descriptor，不在Greed阶段借用未加载的Normal池。
  frozen=false
  local rooms=l:GetRooms();local boss
  for i=0,rooms.Size-1 do local candidate=rooms:Get(i)
   if candidate.Data and candidate.Data.Type==RoomType.ROOM_BOSS then boss=candidate;break end
  end
  assert(boss,"generated normal boss descriptor missing")
  Game():StartRoomTransition(boss.SafeGridIndex,Direction.NO_DIRECTION,RoomTransitionAnim.FADE,nil,0)
 elseif s.kind=="backdrop" then
  -- REP本地Wiki明确第二参数0不生效；在真实Source重建背景，覆盖直接环境实体生成分支。
  r:SetBackdropType(s.value,1)
 elseif s.kind=="stage" then frozen=false;Isaac.ExecuteCommand("stage "..s.value)
 elseif s.kind=="dimension" then frozen=false;l:ChangeRoom(l:GetStartingRoomIndex(),s.value)
 elseif s.kind=="grids" then allgrids(s.dense)
 elseif s.kind=="mutations-setup" then
  -- 给真实Fire一次正常Source更新窗口，取得非空余烬；不在Client推进它。
  frozen=false;allgrids(false)
  -- 用REP真实成员Getter设置非零偏移，覆盖旧ExtraSprite值副本会清零的失败方式。
  local door=r:GetDoor(6);assert(door,"Greed side door missing");door:GetExtraSprite().Offset=Vector(3,-2)
 elseif s.kind=="mutations-break" then
  -- 实际Source操作生成破坏后态/碎屑，不拼造Client成功目标。
  for _,typ in ipairs({2,14,12}) do local g=assert(r:GetGridEntity(assert(slots[typ])));g:Destroy(true) end
  for slot=0,7 do local d=r:GetDoor(slot);if d then d.Busted=true;d:Open() end end
 elseif s.kind=="timer-recreate" then
  local plate=spawn(assert(slots[20]),20,25020,2):ToPressurePlate()
  assert(plate.TimerPlate,"actual Greed TimerPlate missing")
 elseif s.kind=="timer-retexture" then
  -- TimerPlate属性getter返回值副本；改图后通过同一公开属性setter装回Source实例。
  local plate=assert(r:GetGridEntity(assert(slots[20]))):ToPressurePlate();local sprite=assert(plate.TimerPlate)
  timer_sheet=plate:GetSprite():GetLayer(0):GetSpritesheetPath()
  sprite:ReplaceSpritesheet(0,timer_sheet);sprite:LoadGraphics();plate.TimerPlate=sprite
  assert(plate.TimerPlate:GetLayer(0):GetSpritesheetPath()==timer_sheet,"Source TimerPlate texture did not persist")
 elseif s.kind=="timer-retire" then
  local plate=spawn(assert(slots[20]),20,25021,0):ToPressurePlate()
  -- 不假设普通构造会清除复用堆的附属资源；公开setter明确安装Source的未加载Sprite。
  plate.TimerPlate=Sprite()
  assert(plate.TimerPlate==nil or plate.TimerPlate:GetFilename()=="","Source TimerPlate must be empty")
 elseif s.kind=="change" then
  for typ,index in pairs(slots) do local g=assert(r:GetGridEntity(index));g:GetSprite().Rotation=17;g:GetSprite().FlipX=true;g:SetWaterClipFlags(57) end
  local pit=assert(r:GetGridEntity(slots[7])):ToPit();pit:MakeBridge(r:GetGridEntity(slots[2]));pit:SetLadder(true)
  for slot=0,7 do local d=r:GetDoor(slot);if d then d:SetLocked(true);d:Close(true) end end
 elseif s.kind=="restore" then
  for typ,index in pairs(slots) do local g=assert(r:GetGridEntity(index));g.State=baseline[typ].state;g.VarData=baseline[typ].var
   g:GetSprite().Rotation=0;g:GetSprite().FlipX=false;g:ResetWaterClipFlags() end
 elseif s.kind=="replace" then spawn(assert(slots[2]),14,24001)
 elseif s.kind=="remove" then r:RemoveGridEntityImmediate(assert(slots[2]),0,false)
 elseif s.kind=="recreate" then spawn(assert(slots[2]),2,24002)
 elseif s.kind=="short" then
  local i=assert(slots[2]);r:RemoveGridEntityImmediate(i,0,false);spawn(i,2,24003);r:RemoveGridEntityImmediate(i,0,false)
 elseif s.kind=="door-remove" then for slot=0,7 do r:RemoveDoor(slot) end
 elseif s.kind=="environment" then
  r:SetWaterAmount(s.value);r:SetWaterCurrent(Vector(1.25,-0.5));r:SetWaterColor(KColor(0.2,0.4,0.6,0.8));r:SetWaterColorMultiplier(KColor(0.8,0.7,0.6,0.5))
  r:SetFloorColor(Color(0.7,0.8,0.9,1,0,0,0));r:SetWallColor(Color(0.9,0.7,0.8,1,0,0,0))
 elseif s.kind=="progress" then r:SetClear(false);r:SetAmbushDone(true);r:SetSacrificeDone(true);l:GetCurrentRoomDesc().PressurePlatesTriggered=true
 elseif s.kind=="lost" then l:AddCurse(LevelCurse.CURSE_OF_THE_LOST,false)
 elseif s.kind=="normal-map" then l:RemoveCurses(l:GetCurses())
 else error("unknown tx18 scene:"..tostring(s.kind)) end
end
-- 公开MC_PRE_UPDATE返回true暂停玩法；只在取得就绪后的正式后态后保持Source，Render/Manager仍运行。
-- 停止正式观察时字面标识为空，立即恢复原版更新，保证菜单退出不被夹具卡住。
mod:AddCallback(ModCallbacks.MC_PRE_UPDATE,function()
 if not config.network_regression and source_held and IsaacCSObservationNative.SourceSnapshotToken()~="" then return true end
end)
mod:AddCallback(ModCallbacks.MC_POST_RENDER,function()
 frames=frames+1
 -- 清房奖励可能在ready之后才实际生成；持续清理来源世界，正式模型仍完整保留成员。
 if frozen and Game():GetNumPlayers()>0 then clean() end
 if Input.IsButtonTriggered(Keyboard.KEY_F8,0) then
  source_held=false;capture_wait=nil
  action=action+1;local s=assert(scenes[action],"unexpected source action")
  local ok,err=pcall(perform,s);if not ok then emit("fixture_error",{label=s.label,reason=tostring(err)});return end
  pending={scene=s,at=s.kind=="special-case" and frames+3600 or frames+45}
 end
 local update_ok,update_err=pcall(special.Update)
 if not update_ok then emit("fixture_error",{reason=tostring(update_err)});return end
 if pending and Game():GetNumPlayers()>0 and frames>=pending.at then
  local s=pending.scene;local ok,err=pcall(function()
   clean();local p=Isaac.GetPlayer(0)
   assert(p:GetPlayerType()==PlayerType.PLAYER_ISAAC,"ordinary Isaac source required")
   for id=1,CollectibleType.NUM_COLLECTIBLES-1 do while p:HasCollectible(id) do p:RemoveCollectible(id) end end
   for slot=0,1 do local id=p:GetTrinket(slot);if id~=0 then p:TryRemoveTrinket(id) end end
   local r=Game():GetRoom();local l=Game():GetLevel()
   if s.kind=="shape" then assert(r:GetRoomShape()==s.value,"actual shape mismatch") end
   if s.kind=="special" then assert(r:GetType()==s.value,"actual room type mismatch") end
   if s.kind=="boss-backdrop" then assert(r:GetType()==RoomType.ROOM_BOSS,"actual boss room mismatch");r:SetBackdropType(1,1) end
   if s.kind=="backdrop" then assert(r:GetBackdropType()==s.value,"actual backdrop mismatch") end
   if s.kind=="dimension" then assert(l:GetDimension()==s.value,"actual dimension mismatch") end
   if s.kind=="map-big" then assert(l:GetCurrentRoomIndex()==big_anchor+s.value and r:GetRoomShape()==8,"actual entry quadrant mismatch") end
   if s.kind=="override" then local d=l:GetCurrentRoomDesc();assert(d.Data and d.OverrideData and d.Data.Variant~=d.OverrideData.Variant,"actual double config missing") end
   frozen=true
   -- pre-update只冻结Grid逻辑；用公开播放速度同时固定真实门的两份Sprite，
   -- 防止正式采样与稍后窗口截图跨过动画帧。GetExtraSprite返回原生成员而非旧属性副本。
   for slot=0,7 do local door=r:GetDoor(slot);if door then
    door:GetSprite().PlaybackSpeed=0;door:GetExtraSprite().PlaybackSpeed=0
   end end
   local types,indices={},{};for i=0,r:GetGridSize()-1 do local g=r:GetGridEntity(i);if g then types[tostring(g:GetType())]=true;indices[#indices+1]=i end end
   for slot=0,7 do if r:GetDoor(slot) then types["16"]=true end end
   local data={label=s.label,shape=r:GetRoomShape(),room=l:GetCurrentRoomIndex(),dimension=l:GetDimension(),types=types,grid_indices=indices,rails=r:GetRailManager()~=nil}
   if action<=(config.reuse_prefix or 0) then emit("ready",data)
   else capture_wait={data=data,after=IsaacCSObservationNative.SourceSnapshotToken()} end
  end)
  if not ok then emit("fixture_error",{label=s.label,reason=tostring(err)}) end
  pending=nil
 end
 if capture_wait then
  local token,reason=IsaacCSObservationNative.SourceSnapshotToken()
  if token=="" and frames%120==0 then emit("capture_wait",{label=capture_wait.data.label,reason=reason}) end
  if token~="" and token~=capture_wait.after then
   source_held=true
   local data=capture_wait.data;local r=Game():GetRoom();data.grid_indices={}
   for i=0,r:GetGridSize()-1 do if r:GetGridEntity(i) then data.grid_indices[#data.grid_indices+1]=i end end
   data.capture_token=token;emit("ready",data);capture_wait=nil
  end
 end
end)
