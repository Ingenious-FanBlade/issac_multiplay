-- StageAPI 2.43具名只读适配；直接读集合，绝不调用会建表的GetRoomCustomGrids。
local M={installed=false}
function M.Install()
  if M.installed or not StageAPI or not StageAPI.Loaded then return end
  local O=IsaacCSModObservation
  local X=include("domains/common/extended")
  local previous,pending={},nil;
  local levelModel=include("domains/level/model")
  -- 实际版本来自框架自身MarkLoaded登记，不能把适配版本当作任意装载版本。
  local loaded=StageAPI.LoadedMods and StageAPI.LoadedMods.StageAPI
  local frameVersion=loaded and loaded.Version or "UNKNOWN"
  local function lifetime(x) return x.run..":"..x.floor end
  local function gridState(g,x)
    local config=g.GridConfig or {};local id=O.Subject(g,"StageAPI.CustomGridEntity",lifetime(x))
    local owner={namespace="stageapi",subject=id}
    if g.GridEntity then IsaacCSObservationNative.GridSubject(g.GridEntity) end
    local rng=g.RNG and IsaacCSRandomObservation.Register(g.RNG,owner,"CustomGrid.RNG",{},lifetime(x)) or {resolution="Null"}
    return {subject=id,room_id=g.RoomIndex,index=g.GridIndex,position=g.Position,velocity=g.Velocity,config={Name=config.Name,BaseType=config.BaseType,BaseVariant=config.BaseVariant,Anm2=config.Anm2,Animation=config.Animation,Frame=config.Frame,VariantFrames=config.VariantFrames,Offset=config.Offset,OverrideGridSpawns=config.OverrideGridSpawns,NoOverrideGridSprite=config.NoOverrideGridSprite},native_carrier=g.GridEntity and IsaacCSObservationNative.GridReference(g.GridEntity) or {resolution="Null",reason="no_native_carrier"},data=g.Data,persistent_data=g.PersistentData,persistent_index=g.PersistentIndex,unloaded=g.Unloaded or false,rng=rng}
  end
  O.Register{namespace="stageapi",version=tostring(frameVersion),schema="isaac.cs.mod.stageapi.v1",lifetime="run/floor/dimension/logical_room;instance_identity_distinct_from_persistent_index",phases={OrdinaryPost=true,Lifecycle=true,Transition=true},resources={"scripts/stageapi/grid/customgrids.lua","scripts/stageapi/room/levelRoom.lua"},unexposed={"private callback closures","unregistered third-party extensions"},read=function(x,c)
    if frameVersion~="2.43" then X.Read.Fail(c,"stageapi_version_not_adapted");return {status="Unavailable",loaded_version=frameVersion,adapted_version="2.43"} end
    local out={rooms={},custom_grids={},persistent_grids=StageAPI.CustomGrids,current_room_id=StageAPI.GetCurrentRoomID(),native_room_type=Game():GetRoom():GetType(),membership_source="StageAPI.LevelRooms/CustomGridEntities/CustomGrids;raw_existing_tables",retired={}}
    local now={};local cap=0
    local nativeLevel=Game():GetLevel();local descriptors=nativeLevel:GetRooms()
    local current=StageAPI.GetCurrentRoom()
    local function descriptor(room,dimension)
      if room==current then return levelModel.Reference(nativeLevel:GetCurrentRoomDesc(),nativeLevel:GetCurrentRoomIndex()) end
      local index=room.LevelIndex
      if type(index)=="number" and index>=0 and index<descriptors.Size then
        local d=descriptors:Get(index)
        if d and d.Data and d.ListIndex==index and d:GetDimension()==dimension then return levelModel.Reference(d) end
      end
      return {schema="isaac.cs.level",resolution="Unresolved",reason="no_current_native_descriptor_for_framework_logical_room"}
    end
    for dimension,rooms in pairs(StageAPI.LevelRooms or {}) do for roomID,room in pairs(rooms) do
      cap=cap+1;if cap>1024 then X.Read.Fail(c,"stageapi_room_budget");break end
      local id=O.Subject(room,"StageAPI.LevelRoom",lifetime(x));now[id.local_id]=id
      out.rooms[#out.rooms+1]={subject=id,dimension=dimension,room_id=roomID,room_type=room.RoomType,type_override=room.TypeOverride,logical_type=room.TypeOverride or room.RoomType,layout=room.Layout,data=room.Data,persistent_data=room.PersistentData,persistence_data=room.PersistenceData,visit_count=room.VisitCount,clear_count=room.ClearCount,shape=room.Shape,native_descriptor=descriptor(room,dimension),native_relation_basis=room==current and "actual_current_framework_room_and_native_descriptor_in_same_window" or "declared_LevelIndex_and_verified_descriptor_dimension/list_index"}
    end end
    for i,g in ipairs(StageAPI.CustomGridEntities or {}) do
      if i>4096 then X.Read.Fail(c,"stageapi_grid_budget");break end
      local data=gridState(g,x);out.custom_grids[#out.custom_grids+1]=data;now[data.subject.local_id]=data.subject
    end
    if previous.lifetime==lifetime(x) then for id,subject in pairs(previous.members or {}) do if not now[id] then out.retired[#out.retired+1]={subject=subject,kind="AbsentFromFrameworkMembership;see_actual_unload_remove_events"} end end end
    pending={scope=x.scope,lifetime=lifetime(x),members=now}
    return out
  end,outcome=function(scope,ok)
    if pending and pending.scope==scope then if ok then previous=pending else previous={} end;pending=nil end
  end}
  -- 框架原生回调仅旁听，始终nil返回；短命对象无需等待下一普通样本。
  if frameVersion=="2.43" then for _,name in ipairs({"POST_SPAWN_CUSTOM_GRID","POST_CUSTOM_GRID_UNLOAD","POST_REMOVE_CUSTOM_GRID","POST_CUSTOM_GRID_DESTROY","POST_ROOM_INIT","POST_SET_LEVEL_ROOM","POST_ROOM_LOAD"}) do
    StageAPI.AddCallback("IsaacCSObservation",name,100000,function(object,...)
      local n=IsaacCSObservationNative
      if n and n.LocationNeeded() then
        local x=X.Shared.Context()
        local payload=name:find("GRID") and gridState(object,x) or {subject=O.Subject(object,"StageAPI.LevelRoom",lifetime(x)),room_type=object.RoomType,type_override=object.TypeOverride,data=object.Data,persistent_data=object.PersistentData}
        O.Event("stageapi",name,payload)
      end
    end)
  end
  end
  M.installed=true
end
return M
