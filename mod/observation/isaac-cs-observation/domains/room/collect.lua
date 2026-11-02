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
  -- 正式观测同步复制原生展示参数；不由Client按本地房间配置重新裁决。
  out.presentation_runtime=IsaacCSObservationNative.RoomPresentation()
  if not out.presentation_runtime then read.Fail(c,"room_presentation_runtime_unavailable") end
  out.application_runtime=IsaacCSObservationNative.RoomApplication()
  out.backdrop_sprites={}
  for i,sprite in ipairs(IsaacCSObservationNative.RoomBackdropSprites()) do
    out.backdrop_sprites[i]=sprite and read.Value(sprite,"Sprite",c,0) or {status="Null",reason="native_anm_not_loaded"}
  end
  out.vein_sprites={}
  for i,sprite in ipairs(IsaacCSObservationNative.RoomVeinSprites()) do out.vein_sprites[i]=sprite and read.Value(sprite,"Sprite",c,0) or {status="Null"} end
  if not out.application_runtime then read.Fail(c,"room_application_runtime_unavailable") end
  -- 轨道资源按与Grid相同的正式资源姿态复制，空轨道无需加载菜单默认Sprite。
  if out.application_runtime then
    local present=false
    for _,rail in ipairs(out.application_runtime.rails) do if rail~=255 then present=true;break end end
    if present then out.rail_sprite=read.Value(room:GetRailManager():GetRailsSprite(),"Sprite",c,0) end
  end
  -- 应用来源extension v2与观测schema v2分开版本化；正式consumer在录制关闭时仍交付配置与原生映射。
  out.application_source={version=2,target="REPENTOGON-1.1.3/J273",required_contracts={"descriptor-placement-v1","resource-pose-v1","room-configuration-v1","room-runtime-v1"},config_version="REPENTOGON-1.1.3/J273/room-configuration-v1",descriptor_placement=IsaacCSObservationNative.DescriptorPlacement(d)}
  if out.application_source.descriptor_placement.status~="Value" then read.Fail(c,"descriptor_placement_unavailable") end
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
