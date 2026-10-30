-- 房间/楼层共用的只读值模型；每次采集新建上下文，不以旧缓存冒充新采样。
local read=include("domains/common/read")
read.AddSchemas(include("domains/room/schema")); read.AddSchemas(include("domains/level/schema"))
local M={}
function M.Reference(d)
  if not d then return {resolution="Null"} end
  local dimension=d:GetDimension()
  local id=IsaacCSObservationNative.Descriptor(dimension,d.ListIndex,d.GridIndex)
  return {space=6,local_id=id,resolution=id=="0" and "Unresolved" or "KnownCurrent",dimension_index=dimension,list_index=d.ListIndex,grid_index=d.GridIndex,safe_grid_index=d.SafeGridIndex}
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
  c.domain_member=function(object,spec,ctx,depth)
    if spec.member=="Doors" and spec.owner=="RoomDescriptor" then
      local out={}; for slot=0,7 do out[#out+1]={slot=slot,target_grid_index=object.Doors[slot]} end
      return out
    end
    if spec.member=="Doors" then return read.Value(object.Doors,"int",ctx,depth+1) end
    if spec.member=="GetNeighboringRooms" and spec.owner=="RoomDescriptor" then
      if not object.Data then return read.Missing("NotApplicable","no_layout") end
      local out={}
      for slot,d in pairs(object:GetNeighboringRooms()) do out[#out+1]={slot=slot,target=M.Reference(d)} end
      table.sort(out,function(a,b) return a.slot<b.slot end); return out
    end
    if spec.member=="GetCurrentRoomDesc" or spec.member=="GetLastRoomDesc" then return M.Reference(object[spec.member](object)) end
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
