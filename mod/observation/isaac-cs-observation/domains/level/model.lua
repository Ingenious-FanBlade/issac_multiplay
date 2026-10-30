-- 房间/楼层共用的只读值模型；每次采集新建上下文，不以旧缓存冒充新采样。
local read=include("domains/common/read")
read.AddSchemas(include("domains/room/schema")); read.AddSchemas(include("domains/level/schema"))
local M={}
-- 构建见证只复制具名种子与实际Data/Override逻辑值，不计算摘要或把variant当身份。
function M.Construction(d)
  local values={d.SpawnSeed,d.DecorationSeed,d.AwardSeed}
  for _,config in ipairs({{d.Data},{d.OverrideData}}) do
    local x=config[1]
    if x then for _,k in ipairs({"StageID","Mode","Type","Variant","Subtype","Shape","Name"}) do values[#values+1]=tostring(x[k]) end
    else values[#values+1]="nil" end
  end
  return table.concat(values,"|")
end
-- 离图描述原值可不等于实际房号（Genesis：room=-12/Grid=-1/List=-12）。
-- 身份键使用已知读取来源的槽位，原值仍独立输出，不能由单个负索引猜测全局身份。
function M.IdentityLocation(d,sourceIndex)
  if sourceIndex and sourceIndex<0 then return sourceIndex,sourceIndex end
  return d.ListIndex,d.GridIndex
end
function M.Reference(d,sourceIndex)
  if not d then return {resolution="Null"} end
  if not d.Data then return {resolution="NotApplicable",reason="uninitialized_descriptor"} end
  local dimension=d:GetDimension()
  local list,grid=M.IdentityLocation(d,sourceIndex)
  local id,revision=IsaacCSObservationNative.Descriptor(dimension,list,grid,M.Construction(d))
  return {space=6,local_id=id,construction_revision=revision,source_room_index=sourceIndex,identity_basis=sourceIndex and sourceIndex<0 and "known_off_grid_source_slot" or "map_descriptor_location",construction_basis="current_config_and_named_seed_values;not_birth_history",resolution=id=="0" and "Unresolved" or "KnownCurrent",dimension_index=dimension,list_index=d.ListIndex,grid_index=d.GridIndex,safe_grid_index=d.SafeGridIndex}
end
-- 离图槽与路由哨兵具有不同语义；-100/-101不能送入GetRoomByIdx。
function M.Target(level,index,dimension,shapeCandidate)
  if shapeCandidate and index<0 then return {kind="ShapeDisallowsSlot",index=index} end
  if index==-100 or index==-101 then return {kind="DimensionRoute",index=index,route=index==-100 and "Mirror" or "Mineshaft",target_dimension_index=1} end
  if index==-19 then return {kind="ExcludedOriginalOnline",index=index} end
  if index<-20 or index>168 then return {kind="NoTargetOrUnsupported",index=index} end
  local d=level:GetRoomByIdx(index,dimension)
  return {kind=index<0 and "OffGridSlot" or "MapRoom",index=index,target=M.Reference(d,index)}
end
function M.Context()
  local c=read.Context(); c.grid=true -- 具名owner RNG仅复制seed/shift，不调用Next。
  c.domain_value=function(object,typ,ctx,depth)
    if typ=="RoomDescriptor" then return true,M.Reference(object) end
    if typ=="Room" then return true,{relation="current_room_instance",scope="envelope.context.room_instance"} end
    if typ=="FXLayers" then return true,{status="Unavailable",reason="public_interface_exposes_only_AddPoopFx;active_effects_in_entity_membership"} end
    if typ=="RoomConfigRoom" then
      local key=table.concat({object.StageID,object.Mode,object.Type,object.Variant},":")
      -- Data与Override即使共享variant也可能内容不同；查找键不替代实际配置值或资源身份。
      return true,{type="resource",lookup_key=key,owner=ctx.owner,state=read.Collect(object,typ,ctx,depth+1)}
    end
    return false
  end
  c.external_reference=function(object,spec)
    if spec.reason=="room_save:tx-04-13" then
      return {type="owner_qualified_domain_reference",schema="isaac.cs.room_save",owner=spec.owner=="RoomDescriptor" and M.Reference(object) or {relation="current_floor",source="envelope.context.floor"},field=spec.member,scope_rule="same_committed_native_window;target_coverage_applies"}
    end
    return read.Missing("NotCaptured",spec.reason)
  end
  c.domain_member=function(object,spec,ctx,depth)
    if spec.member=="Doors" and spec.owner=="RoomDescriptor" then
      local out={}; for slot=0,7 do out[#out+1]={slot=slot,target_grid_index=object.Doors[slot]} end
      return out
    end
    if spec.member=="Doors" then return read.Value(object.Doors,"int",ctx,depth+1) end
    if spec.member=="GetNeighboringRooms" and spec.owner=="RoomDescriptor" then
      if not object.Data or object.GridIndex<0 then return read.Missing("NotApplicable","no_map_adjacency_for_off_grid_descriptor") end
      local out={}
      for slot,d in pairs(object:GetNeighboringRooms()) do out[#out+1]={slot=slot,target=M.Reference(d)} end
      table.sort(out,function(a,b) return a.slot<b.slot end); return out
    end
    if spec.member=="GetCurrentRoomDesc" or spec.member=="GetLastRoomDesc" then return M.Reference(object[spec.member](object),spec.member=="GetCurrentRoomDesc" and object:GetCurrentRoomIndex() or object:GetPreviousRoomIndex()) end
    return {status="NotScheduled",reason="explicit_membership_or_query",signature=spec.parameters}
  end
  c.parameter_sets=function(object,spec)
    if spec.owner=="Room" then
      if spec.member=="IsDoorSlotAllowed" or spec.member=="GetDoorSlotPosition" then
        local a={}; for i=0,7 do a[#a+1]={i} end; return a
      end
      -- 实际存储448格的布局查询，与Grid本体唯一身份分开。
      if spec.member=="GetGridPosition" or spec.member=="GetGridCollision" or spec.member=="GetGridPath" or spec.member=="GetRail" then
        local a={}; for i=0,447 do a[#a+1]={i} end; return a
      end
    end
    if spec.owner=="Level" and spec.member=="GetStateFlag" then
      local a={}; for k,v in pairs(LevelStateFlag) do if type(v)=="number" and not k:find("NUM_") then a[#a+1]={v} end end
      table.sort(a,function(a,b) return a[1]<b[1] end); return a
    end
    if spec.owner=="Level" and spec.member=="CanOpenChallengeRoom" then
      local a={}; local rooms=object:GetRooms(); for i=0,rooms.Size-1 do local d=rooms:Get(i); if d:GetDimension()==object:GetDimension() then a[#a+1]={d.SafeGridIndex} end end; return a
    end
    if spec.owner=="RoomConfigStage" and spec.member=="IsLoaded" then return {{0},{1}} end
    return nil
  end
  return c
end
-- 显式查询只允许目录中的只读方法；随机/加载/玩法方法拒绝执行。
function M.Query(domain,object,class,member,args)
  local schemas=include("domains/"..domain.."/schema")
  for _,spec in ipairs(schemas[class] or {}) do
    if spec.member==member and (spec.category=="query" or spec.category=="parameter_query" or spec.category=="derived_query") then
      local c=M.Context(); local result=read.QueryValue(object,spec,args or {},c,0)
      -- 参数按声明类型逐项复制，Vector/Descriptor不能退回未知userdata表图。
      local parameters={}; local i=0
      for declaration in (spec.parameters or ""):gmatch("[^,]+") do
        i=i+1; if args and args[i]~=nil then
          local typ=declaration:match("^%s*([%w_]+)")
          parameters[#parameters+1]={index=i,type=typ,value=read.Value(args[i],typ,c,0)}
        end
      end
      local json,complete,count,nodes,reason=read.Finish({owner_class=class,member=spec.id,parameters=parameters,result=result},c)
      IsaacCSObservationNative.PublishLocation(json,complete,count,nodes,reason,domain=="level" and 1 or 0,1); return result
    end
  end
  return read.Missing("Unavailable","not_declared_readonly_query")
end
return M
