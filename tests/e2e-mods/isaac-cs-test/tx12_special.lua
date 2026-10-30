-- tx-04-12扩展E2E：所有世界修改只在夹具，清单与请求/实际结果分开。
local M={}; local cases=include("tx12_cases")
local byId={}; for _,c in ipairs(cases) do byId[c.id]=c end
local emit,serial,frames,pending,active,enabled,waitingCrawl=nil,0,0,nil,nil,false,false
local roomGeneration,levelGeneration=0,0
local function snapshot()
 local l=Game():GetLevel(); local r=Game():GetRoom(); local d=l:GetCurrentRoomDesc(); local cfg=d.Data
 return {room=l:GetCurrentRoomIndex(),grid=d.GridIndex,list=d.ListIndex,dimension=l:GetDimension(),stage=l:GetStage(),stage_type=l:GetStageType(),floor_seed=l:GetDungeonPlacementSeed(),type=r:GetType(),subtype=cfg and cfg.Subtype,shape=r:GetRoomShape(),variant=cfg and cfg.Variant,frame=r:GetFrameCount(),greed=Game():IsGreedMode(),difficulty=Game().Difficulty,wave=l.GreedModeWave,ascent=l:IsAscent()}
end
local function players(crawl)
 local dungeon=Game():GetRoom():GetType()==RoomType.ROOM_DUNGEON
 for _,p in ipairs(PlayerManager.GetPlayers()) do
  p:SetMinDamageCooldown(600); p.Velocity=Vector.Zero
  p.EntityCollisionClass=crawl and EntityCollisionClass.ENTCOLL_ALL or EntityCollisionClass.ENTCOLL_NONE
  p.GridCollisionClass=(crawl or dungeon) and EntityGridCollisionClass.GRIDCOLL_GROUND or EntityGridCollisionClass.GRIDCOLL_NONE
  -- 夹层采用侧视重力；关掉网格碰撞会坠出边界并被原版送进Error房。
  if dungeon then p.Position=Game():GetRoom():GetCenterPos(); p.Velocity=Vector.Zero end
 end
end
local function config(spec)
 -- 测试允许显式加载已安装原版配置；正式观察器绝不调用这个懒加载入口。
 local stage,mode=spec.stage,spec.mode or 0
 if stage==18 then stage=0;mode=1 end
 local set=RoomConfig.GetStage(stage):GetRoomSet(mode)
 for i=0,set.Size-1 do
  local c=set:Get(i)
  if c.Type==spec.type and c.Subtype==spec.subtype and c.Shape==spec.shape and c.OriginalVariant==spec.variant and c.Name==spec.name then return c end
 end
 error("stock layout not found:"..spec.stage..":"..spec.type..":"..spec.variant..":"..spec.subtype)
end
local function install(slot,cfg)
 assert(slot>=-20 and slot<=-1 and slot~=-19,"excluded/non-slot target")
 local d=Game():GetLevel():GetRoomByIdx(slot,0)
 -- 公开Clear同时管理官方保存状态owner；不用私有指针清零或保留旧实体保存项。
 d:GetEntitiesSaveState():Clear(); d:GetDecoSaveState():Clear(); d:GetGridEntitiesSaveState():Clear()
 d.Data=cfg; d.OverrideData=nil; d.VisitedCount=0; d.Clear=false; d.ClearCount=0; d.NoReward=true; d.ChallengeDone=false; d.Flags=0
 serial=serial+1; local rng=RNG(); rng:SetSeed(771000+serial,35); d:InitSeeds(rng)
end
local function transition(index,dimension,animation)
 local l=Game():GetLevel()
 if pending and (l:GetCurrentRoomIndex()~=index or l:GetDimension()~=dimension) then pending.room_generation=roomGeneration+1 end
 Game():StartRoomTransition(index,Direction.NO_DIRECTION,animation or RoomTransitionAnim.FADE,nil,dimension)
end
local function matches(c,a)
 if c.expected_room and a.room~=c.expected_room then return false end
 if c.expected_grid and a.grid~=c.expected_grid then return false end
 if c.expected_type and a.type~=c.expected_type then return false end
 if c.expected_subtype and a.subtype~=c.expected_subtype then return false end
 if c.expected_shape and a.shape~=c.expected_shape then return false end
 if c.expected_dimension and a.dimension~=c.expected_dimension then return false end
 if c.expected_stage and a.stage~=c.expected_stage then return false end
 if c.expected_greed and not a.greed then return false end
 if c.expected_difficulty and a.difficulty~=c.expected_difficulty then return false end
 if c.expected_wave and a.wave<=0 then return false end
 if c.expected_ascent and not a.ascent then return false end
 return true
end
function M.Start(logger) emit=logger; return cases end
-- 使用实际初始化完成回调等待转换，不能把console提前写入的Stage当作新楼层已可遍历。
function M.NewRoom() roomGeneration=roomGeneration+1 end
function M.NewLevel() levelGeneration=levelGeneration+1 end
function M.Perform(name)
 if name=="coverage_finish" then
  pending=nil; waitingCrawl=false; enabled=false
  Game():SetStateFlag(GameStateFlag.STATE_BACKWARDS_PATH,false); Game().Difficulty=Difficulty.DIFFICULTY_NORMAL
  for _,p in ipairs(PlayerManager.GetPlayers()) do p.EntityCollisionClass=EntityCollisionClass.ENTCOLL_ALL; p.GridCollisionClass=EntityGridCollisionClass.GRIDCOLL_GROUND end
  Isaac.ExecuteCommand("stage 1"); return
 end
 if name=="coverage_reset" then
  enabled=true; Game():SetStateFlag(GameStateFlag.STATE_BACKWARDS_PATH,false); Game().Difficulty=Difficulty.DIFFICULTY_NORMAL
  Isaac.ExecuteCommand("stage 1"); return
 end
 local role,id=name:match("^(%w+):(.+)$"); local c=assert(byId[id],"coverage case:"..tostring(id)); enabled=true
 local before=snapshot(); waitingCrawl=false
 if role=="return" or role=="return2" then
  assert(active and active.id==id,"return owner mismatch")
  local origin=active.origin
  pending={id=id,role=role,request=c,before=before,expect={expected_grid=origin.grid,expected_dimension=origin.dimension},after_render=frames+15}
  transition(origin.room,origin.dimension); return
 end
 if role=="reenter" then
  assert(active and active.id==id and active.entered,"reenter owner mismatch")
  pending={id=id,role=role,request=c,before=before,expect=c,after_render=frames+15}
  transition(active.entered.room,active.entered.dimension); return
 end
 active={id=id,origin=before}; pending={id=id,role="case",request=c,before=before,expect=c,after_render=frames+15}
 local l=Game():GetLevel(); local r=Game():GetRoom()
 players(false)
 if c.method=="controlled-config" or c.method=="controlled-slot" then
  install(c.expected_grid,config(c.layout)); transition(c.expected_grid,0)
 elseif c.method=="existing-slot" then
  -- 特殊Boss夹层不强套调试槽，也不再次清空仍由原版保存的专用实体。
  local d=l:GetRoomByIdx(c.expected_grid,0)
  -- 续测从新run开始时只准备必要槽，不把此前已通过的槽位用例再跑一遍。
  if not d.Data then install(c.expected_grid,config(c.layout)); d=l:GetRoomByIdx(c.expected_grid,0) end
  assert(d.Data and d.Data.Type==c.expected_type and d.Data.Subtype==c.expected_subtype,"required constructed slot absent")
  transition(c.expected_grid,0)
 elseif c.method=="angel" or c.method=="devil" then
  l:GetRoomByIdx(-1,0).Data=nil; l:InitializeDevilAngelRoom(c.method=="angel",c.method=="devil")
  transition(-1,0)
  -- 原生随机结果不受静态选中的具体布局形状/子类型限定，仍要求实际天使/恶魔类型及slot。
  pending.expect={expected_room=-1,expected_type=c.expected_type,expected_greed=c.expected_greed}
 elseif c.method=="genesis" then
  pending.room_generation=roomGeneration+1
  Isaac.GetPlayer(0):AddCollectible(CollectibleType.COLLECTIBLE_SAD_ONION,0,false)
  Isaac.GetPlayer(0):UseActiveItem(CollectibleType.COLLECTIBLE_GENESIS,UseFlag.USE_NOANIM|UseFlag.USE_NOCOSTUME)
 elseif c.method=="crawlspace" then
  for _,e in ipairs(Isaac.GetRoomEntities()) do if e:ToNPC() then e:Remove() end end
  local index=r:GetGridIndex(r:GetCenterPos()); if r:GetGridEntity(index) then r:RemoveGridEntityImmediate(index,0,false) end
  assert(r:SpawnGridEntity(index,GridEntityType.GRID_STAIRS,0,771111,0),"stairs spawn")
  local stairs=assert(r:GetGridEntity(index)); players(true); waitingCrawl={index=index,stepped=false}
  -- 原生楼梯在玩家50像素内保持关闭；先等原生动画打开，再实际踩入，不强改State。
  Isaac.GetPlayer(0).Position=stairs.Position+Vector(100,0)
  pending.expect={expected_room=-4,expected_type=RoomType.ROOM_DUNGEON,expected_subtype=0}
 elseif c.method=="stage" or c.method=="route-room" then
  Game():SetStateFlag(GameStateFlag.STATE_BACKWARDS_PATH,false); Game().Difficulty=Difficulty.DIFFICULTY_NORMAL
  pending.expect={expected_stage=tonumber(c.command:match("stage (%d+)")),expected_dimension=0}
  pending.route_room=c.method=="route-room" and c.descriptor_subtype or nil
  pending.level_generation=levelGeneration+1
  -- 原生矿道生成要求进入Mines II前已有菜刀碎片1；不事后伪造入口/内部描述。
  if c.id=="mineshaft_entrance" then Isaac.GetPlayer(0):AddCollectible(CollectibleType.COLLECTIBLE_KNIFE_PIECE_1,0,false) end
  Isaac.ExecuteCommand(c.command)
 elseif c.method=="dimension" then
  local rooms=l:GetRooms(); local target
  for i=0,rooms.Size-1 do local d=rooms:Get(i); if d:GetDimension()==c.dimension and d.Data then target=d; break end end
  assert(target,"generated dimension absent:"..c.dimension); transition(target.SafeGridIndex,c.dimension,RoomTransitionAnim.FADE_MIRROR)
 elseif c.method=="death-certificate" then
  pending.room_generation=roomGeneration+1
  Isaac.GetPlayer(0):UseActiveItem(CollectibleType.COLLECTIBLE_DEATH_CERTIFICATE,UseFlag.USE_NOANIM|UseFlag.USE_NOCOSTUME)
 elseif c.method=="ascent" then pending.level_generation=levelGeneration+1; Game():SetStateFlag(GameStateFlag.STATE_BACKWARDS_PATH,true); Isaac.ExecuteCommand("stage 3")
 elseif c.method=="red-room" then
  if l:GetDimension()~=0 then transition(l:GetStartingRoomIndex(),0) end
  pending.place_red=true
 elseif c.method=="greed" or c.method=="greedier" or c.method=="greed-stage" then
  pending.level_generation=levelGeneration+1
  Game():SetStateFlag(GameStateFlag.STATE_BACKWARDS_PATH,false)
  Game().Difficulty=c.method=="greedier" and Difficulty.DIFFICULTY_GREEDIER or Difficulty.DIFFICULTY_GREED
  Isaac.ExecuteCommand("stage "..(c.stage or 1))
 elseif c.method=="greed-room" then
  local target; local rooms=l:GetRooms()
  for i=0,rooms.Size-1 do local d=rooms:Get(i); local cfg=d.Data
   if d:GetDimension()==0 and cfg and cfg.Type==c.room_type and (not c.room_shape or cfg.Shape==c.room_shape) and (not c.room_subtype or cfg.Subtype==c.room_subtype) then target=d; break end
  end
  assert(target,"generated greed room absent:"..c.id); transition(target.SafeGridIndex,0)
 elseif c.method=="greed-wave" then
  pending.trigger_wave=true
  for i=0,447 do local g=r:GetGridEntity(i); if g and g:GetType()==GridEntityType.GRID_PRESSURE_PLATE then
   players(true); Isaac.GetPlayer(0).Position=g.Position; pending.plate=i; break
  end end
  assert(pending.plate,"greed plate absent")
 else error("unknown coverage method:"..tostring(c.method)) end
 emit("coverage_request",{id=id,role=role,method=c.method,expected=pending.expect,before=before})
end
-- 只在真实回调内等待游戏完成请求；未达到实际状态则让harness报失败，不把请求当通过。
function M.Update()
 frames=frames+1; if not enabled or Game():GetNumPlayers()==0 then return end
 if waitingCrawl and Game():GetLevel():GetCurrentRoomIndex()==-4 then waitingCrawl=false end
 players(waitingCrawl or pending and pending.trigger_wave)
 if waitingCrawl then
  local stairs=assert(Game():GetRoom():GetGridEntity(waitingCrawl.index),"native stairs disappeared")
  if stairs.State==1 and not waitingCrawl.stepped then
   waitingCrawl.stepped=true; emit("crawlspace_step",{state=stairs.State,animation=stairs:GetSprite():GetAnimation()})
  end
  Isaac.GetPlayer(0).Position=stairs.Position+(waitingCrawl.stepped and Vector.Zero or Vector(100,0))
 end
 if not pending or frames<pending.after_render then return end
 if pending.level_generation and levelGeneration<pending.level_generation or pending.room_generation and roomGeneration<pending.room_generation then return end
 if pending.place_red then
  local l=Game():GetLevel();local rooms=l:GetRooms()
  if not pending.red_target then
   -- 用户指定以该隐魂石走原生效果；先选1x1普通房的有效空邻格，避免无可开条件。
   for i=0,rooms.Size-1 do local d=rooms:Get(i);if d:GetDimension()==0 and d.Data and d.Data.Type==1 and d.Data.Shape==1 then
    local neighbors=d:GetNeighboringRooms();local x=d.GridIndex%13;local y=d.GridIndex//13
    local valid={x>0,y>0,x<12,y<12}
    for slot=0,3 do if valid[slot+1] and (d.Data.Doors&(1<<slot))~=0 and not neighbors[slot] then pending.red_target=d.SafeGridIndex;break end end
   end;if pending.red_target then break end end
   assert(pending.red_target,"Soul of Cain has no available room boundary")
   if l:GetCurrentRoomIndex()~=pending.red_target then transition(pending.red_target,0);pending.after_render=frames+15;return end
  end
  pending.minimum_rooms=l:GetRooms().Size;pending.place_red=nil;pending.after_render=frames+15
  Isaac.GetPlayer(0):UseCard(Card.CARD_SOUL_CAIN,UseFlag.USE_NOANIM|UseFlag.USE_NOCOSTUME)
  -- GetRooms返回的ArrayProxy含调用时的size；效果后必须重新获取，不能反复读旧代理。
  emit("red_room_card",{card=Card.CARD_SOUL_CAIN,before=pending.minimum_rooms,after=l:GetRooms().Size});return
 end
 local a=snapshot()
 if a.type==RoomType.ROOM_ERROR and pending.expect.expected_type~=RoomType.ROOM_ERROR and frames>pending.after_render+60 then error("unexpected_error_redirect:"..pending.id..":"..pending.role) end
 if a.frame<3 or not matches(pending.expect,a) then return end
 if pending.route_room then
  local l=Game():GetLevel(); local rooms=l:GetRooms(); local target
  for i=0,rooms.Size-1 do local d=rooms:Get(i); if d:GetDimension()==0 and d.Data and d.Data.Type==RoomType.ROOM_DEFAULT and d.Data.Subtype==pending.route_room then target=d; break end end
  assert(target,"native route entrance subtype missing:"..pending.route_room)
  pending.expect.expected_type=RoomType.ROOM_DEFAULT; pending.expect.expected_subtype=pending.route_room; pending.route_room=nil; pending.after_render=frames+15
  transition(target.SafeGridIndex,0); return
 end
 if pending.minimum_rooms and Game():GetLevel():GetRooms().Size<=pending.minimum_rooms then return end
 if pending.minimum_rooms then a.room_count_before=pending.minimum_rooms;a.room_count_after=Game():GetLevel():GetRooms().Size end
 if pending.role=="case" then active.entered=a end
 emit("coverage_ready",{id=pending.id,role=pending.role,actual=a,before=pending.before,method=pending.request.method})
 pending=nil
end
return M
