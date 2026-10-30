-- 活动房间规范状态；构建是实际布局/资源/成员的观测，不执行生成或配置选择。
local read=include("domains/common/read")
local model=include("domains/level/model")
local M={SchemaVersion=2}
function M.Capture(limits)
  read.Limits(limits); local c=model.Context(); c.objects=1
  local level=Game():GetLevel(); local room=Game():GetRoom(); local d=level:GetCurrentRoomDesc()
  local identityList,identityGrid=model.IdentityLocation(d,level:GetCurrentRoomIndex())
  local location=IsaacCSObservationNative.Location(level:GetStage(),level:GetStageType(),level:GetDungeonPlacementSeed(),level:GetDimension(),identityList,identityGrid,model.Construction(d))
  if location.floor=="0" or location.descriptor=="0" then read.Fail(c,"location_identity_unavailable") end
  c.owner={space=7,local_id=location.room_instance}
  -- 引用先登记在本scope，Door沿用Grid唯一主体，不制造第二份门状态。
  local sources,unavailable=IsaacCSObservationNative.Sources()
  local grids,gap=IsaacCSObservationNative.GridSources()
  for _,e in ipairs(sources) do IsaacCSObservationNative.Subject(e.entity) end
  for _,g in ipairs(grids) do IsaacCSObservationNative.GridSubject(g.grid) end
  local out={room_index=level:GetCurrentRoomIndex(),dimension_index=level:GetDimension(),location=location,descriptor=model.Reference(d,level:GetCurrentRoomIndex()),state=read.Collect(room,"Room",c,0),doors={},entities={},grids={},seeds={spawn=room:GetSpawnSeed(),decoration=room:GetDecorationSeed(),award=room:GetAwardSeed()},sampling={freshness="ReadInCurrentNativeWindow",cache_reused=false},config=read.Value(d.Data,"RoomConfigRoom",c,0),override_config=read.Value(d.OverrideData,"RoomConfigRoom",c,0)}
  for slot=0,7 do
    local g=room:GetDoor(slot)
    out.doors[#out.doors+1]={slot=slot,shape_allowed=room:IsDoorSlotAllowed(slot),descriptor_enabled=(d.AllowedDoors&(1<<slot))~=0,shape_target_grid_index=d.Doors[slot],actual=g and IsaacCSObservationNative.GridReference(g) or {resolution="Null"},actual_target=g and g.TargetRoomIndex or nil,actual_connection=g and model.Target(level,g.TargetRoomIndex,level:GetDimension(),false) or {kind="NoActualDoor"}}
  end
  if unavailable>0 then read.Fail(c,"room_entity_membership_unavailable") end
  for _,e in ipairs(sources) do out.entities[#out.entities+1]=IsaacCSObservationNative.Subject(e.entity) end
  if gap>0 then read.Fail(c,"room_grid_membership_unavailable") end
  for _,g in ipairs(grids) do out.grids[#out.grids+1]={subject=IsaacCSObservationNative.GridSubject(g.grid),memberships=g.memberships} end
  out.stage_config=read.Collect(RoomConfig.GetStage(Isaac.GetCurrentStageConfigId()),"RoomConfigStage",c,0)
  out.transition=read.Collect(RoomTransition,"RoomTransition",c,0)
  return read.Finish(out,c)
end
function M.Query(object,class,member,args) return model.Query("room",object,class,member,args) end
-- 真实进度事件保留当时公开标量；render独立scope读取颜色/过渡，不能合并到普通后态。
function M.Event(name,render,...)
  local c=model.Context(); local room=Game():GetRoom(); local d=Game():GetLevel():GetCurrentRoomDesc()
  local out={event=name,frame=room:GetFrameCount(),clear=room:IsClear(),ambush_active=room:IsAmbushActive(),ambush_done=room:IsAmbushDone(),sacrifice_done=room:IsSacrificeDone(),clear_count=d.ClearCount,no_reward=d.NoReward,visited_count=d.VisitedCount,greed_wave=Game():GetLevel().GreedModeWave,transition_mode=RoomTransition.GetTransitionMode()}
  if render then out.fx=read.Value(room:GetFXParams(),"FXParams",c,0); out.floor_color=read.Value(room:GetFloorColor(),"Color",c,0); out.wall_color=read.Value(room:GetWallColor(),"Color",c,0) end
  if name=="MC_POST_ROOM_TRIGGER_CLEAR" or name=="MC_POST_START_AMBUSH_WAVE" then out.argument=read.Value((...),"boolean",c,0) end
  local json,complete,count,nodes,reason=read.Finish(out,c)
  IsaacCSObservationNative.PublishLocation(json,complete,count,nodes,reason,0,render and 2 or 0)
end
function M.Error(name,err)
  local c=model.Context(); read.Fail(c,"room_callback_copy")
  local json,complete,count,nodes,reason=read.Finish({event=name,status="ReadFailed",reason=tostring(err):gsub("0x%x+","<local>"):sub(1,256)},c)
  IsaacCSObservationNative.PublishLocation(json,complete,count,nodes,reason,0,0)
end
return M
