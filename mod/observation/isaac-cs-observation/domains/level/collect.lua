-- 全楼层描述集合及各维度membership；每30个普通窗口重读，保留原采样scope不做无依据缓存。
local read=include("domains/common/read")
local model=include("domains/level/model")
local M={SchemaVersion=2}
-- placement是正式来源的一部分，失败必须降低本域完整性，不能只等Client发现缺口。
local function placement(d,c)
  local p=IsaacCSObservationNative.DescriptorPlacement(d)
  if p.status~="Value" then read.Fail(c,"descriptor_placement_unavailable") end
  return p
end
function M.Capture(limits)
  read.Limits(limits); local c=model.Context(); local level=Game():GetLevel(); local list=level:GetRooms()
  local out={state=read.Collect(level,"Level",c,0),descriptors={},dimensions={},connections={},sampling={freshness="ReadInCurrentNativeWindow",cache_reused=false},membership={source="Level.GetRooms;all_dimensions",size=list.Size,complete=true}}
  if list.Size<0 or list.Size>1024 then read.Fail(c,"descriptor_budget"); out.membership.complete=false end
  local dims={[0]={index=0,descriptors={}},[1]={index=1,descriptors={}},[2]={index=2,descriptors={}}}
  -- GetRooms是包含各维度的实际描述列表；每个成员GetDimension判定归属。
  for i=0,math.min(list.Size,1024)-1 do
    local d=list:Get(i); local id=model.Reference(d); c.owner=id; c.objects=c.objects+1
    if id.local_id=="0" then read.Fail(c,"descriptor_identity_budget") end
    if not dims[id.dimension_index] then read.Fail(c,"unknown_dimension"); out.membership.complete=false
    else dims[id.dimension_index].descriptors[#dims[id.dimension_index].descriptors+1]=id end
    out.descriptors[#out.descriptors+1]={subject=id,list_position=i,placement=placement(d,c),state=model.ReadDescriptor(d,c)}
    if d.Data and d.GridIndex>=0 then
      local neighbors=d:GetNeighboringRooms()
      for slot=0,7 do
        local index=d.Doors[slot]
        out.connections[#out.connections+1]={source=id,slot=slot,shape_target_grid_index=index,descriptor_enabled=(d.AllowedDoors&(1<<slot))~=0,neighbor=neighbors[slot] and model.Reference(neighbors[slot]) or {resolution="Null"},semantics="shape_and_adjacency_candidate;actual_door_in_room_domain"}
      end
    end
  end
  -- GetRooms不含全部离图槽；两份配置分别判空，不能把仅Override存在的记录吞成未初始化。
  out.off_grid={source="Level.GetRoomByIdx;target SDK 20 slots",entries={},excluded={index=-19,reason="original_online_isolation"},complete=true}
  for slot=-1,-20,-1 do if slot~=-19 then
    local d=level:GetRoomByIdx(slot,0)
    local entry={slot=slot,status=d and (d.Data or d.OverrideData) and "Initialized" or "Uninitialized"}
    if d and (d.Data or d.OverrideData) then
      local id=model.Reference(d,slot); c.owner=id; c.objects=c.objects+1
      entry.subject=id; entry.state=model.ReadDescriptor(d,c); entry.placement=placement(d,c)
      if id.local_id=="0" then read.Fail(c,"off_grid_identity_unavailable"); out.off_grid.complete=false end
    end
    out.off_grid.entries[#out.off_grid.entries+1]=entry
  end end
  for i=0,2 do local dim=dims[i]; dim.subject={space=5,local_id=IsaacCSObservationNative.Dimension(i),presence=#dim.descriptors>0 and "Known" or "NotApplicable"}; dim.membership_complete=out.membership.complete; dim.status=#dim.descriptors>0 and "Present" or "EmptyInEnumeratedDescriptorList"; out.dimensions[#out.dimensions+1]=dim end
  -- 当前调试特殊房可能不在GetRooms常规列表，单列真实descriptor而不遗漏或伪造列表成员。
  local current=level:GetCurrentRoomDesc(); out.current=model.Reference(current,level:GetCurrentRoomIndex()); c.owner=out.current
  out.current_descriptor=model.ReadDescriptor(current,c)
  out.previous=model.Reference(level:GetLastRoomDesc(),level:GetPreviousRoomIndex())
  return read.Finish(out,c)
end
function M.Query(object,class,member,args) return model.Query("level",object,class,member,args) end
-- 生成回调只记录实际参数；pre-place是请求，完成结果以普通描述集合为准。
function M.Event(name,...)
  local c=model.Context(); local args=table.pack(...); local payload={event=name}
  if name=="MC_PRE_LEVEL_PLACE_ROOM" then
    payload.phase="placement_request"; payload.config=read.Value(args[2],"RoomConfigRoom",c,0); payload.seed=read.Value(args[3],"int",c,0)
  elseif name=="MC_PRE_LEVEL_SELECT" then payload.stage=read.Value(args[1],"int",c,0); payload.stage_type=read.Value(args[2],"int",c,0)
  else payload.phase="layout_generated;descriptor_result_in_next_ordinary_scope" end
  local json,complete,count,nodes,reason=read.Finish(payload,c)
  IsaacCSObservationNative.PublishLocation(json,complete,count,nodes,reason,1,0)
end
return M
