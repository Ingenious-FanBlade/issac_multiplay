-- Grid 独立主体域：只读当前实际对象；槽位／门槽／装饰 membership 不充当对象身份。
local read=include("domains/common/read")
local schemas=include("domains/grid/schema")
read.AddSchemas(schemas)
local M={SchemaVersion=2}
local casts={"Decoration","Door","Fire","Gravity","Lock","Pit","Poop","PressurePlate","Rock","Spikes","Stairs","Statue","Teleporter","TNT","TrapDoor","Wall","Web"}
local function native() return IsaacCSObservationNative end
local function context() local c=read.Context(); c.grid=true; return c end
local function snapshot(g,c)
  local id=native().GridSubject(g); c.owner=id
  local out={subject=id,type=g:GetType(),variant=g:GetVariant(),index=g:GetGridIndex(),casts={},bodies={}}
  if id.resolution=="Retired" then out.status="NotApplicable"; return out end
  if id.local_id=="0" then read.Fail(c,"grid_subject_budget"); return out end
  out.common=read.Collect(g,"GridEntity",c,0)
  for _,name in ipairs(casts) do
    local ok,derived=read.Protected(g,"To"..name)
    if not ok then out.casts[name]={status="Unavailable"}; read.Fail(c,"grid_cast:"..name)
    elseif not derived then out.casts[name]={status="NotApplicable"}
    else
      out.casts[name]={status="Value"}; out.class="GridEntity"..name
      out.bodies[out.class]=read.Collect(derived,out.class,c,0)
    end
  end
  if not out.class then out.class="UnknownGrid"; read.Fail(c,"unknown_grid_type:"..tostring(out.type)) end
  if out.class=="GridEntityStatue" then
    local ok,display=pcall(native().GridDisplayEntity,g)
    if ok and display then native().Subject(display) end
    out.native_relations={version=1,display_entity=ok and read.Value(display,"Entity",c,1) or read.Missing("ReadFailed","statue_display_entity")}
    if not ok then read.Fail(c,"statue_display_entity") end
  end
  return out
end
function M.Capture(limits)
  local budget=read.Limits(limits); local c=context()
  local sources,unavailable=native().GridSources()
  local out={objects=read.Array(),lifted_descriptors=read.Array(),membership={source="native:grid+door",entries=read.Array(),unavailable=unavailable,complete=unavailable==0}}
  if unavailable>0 then read.Fail(c,"grid_native_source_unavailable") end
  -- 一次窗口先登记所有主体；各槽保留别名，同一主体只读一次。
  for _,entry in ipairs(sources) do native().GridSubject(entry.grid) end
  for i,entry in ipairs(sources) do
    if i>budget.objects then read.Fail(c,"grid_object_budget"); out.membership.complete=false; break end
    c.objects=c.objects+1
    local item=snapshot(entry.grid,c)
    item.memberships=entry.memberships
    out.membership.entries[#out.membership.entries+1]={subject=item.subject,memberships=entry.memberships,index=item.index}
    out.objects[#out.objects+1]=item
  end
  -- 搬起后的描述属于 Effect 所有，不再冒称活动 Grid 槽成员，也不猜测来源 Grid。
  local entities,entityUnavailable=native().Sources()
  if entityUnavailable>0 then read.Fail(c,"lifted_descriptor_source_unavailable") end
  for _,entry in ipairs(entities) do
    local e=entry.entity
    if e.Type==1000 and e.Variant==136 then
      local owner=native().Subject(e); c.owner=owner
      local ok,desc=read.Protected(e:ToEffect(),"GetGridEntityDesc")
      if not ok then read.Fail(c,"lifted_descriptor_read") end
      out.lifted_descriptors[#out.lifted_descriptors+1]={owner=owner,source_grid={resolution="Unresolved",reason="public_api_exposes_no_source_grid_identity"},descriptor=ok and read.Value(desc,"GridEntityDesc",c,0) or read.Missing("ReadFailed","GetGridEntityDesc")}
    end
  end
  return read.Finish(out,c)
end
local counters,epoch={},nil
-- 真实回调保留独立 scope，普通状态不拼入更新／渲染阶段。更新保留运行变化，render每30次回调读取资源姿态。
function M.Event(name,g,render,...)
  g=native().GridCallbackView(g)
  if not g then error("grid_callback_userdata_or_readiness_unavailable") end
  local id=native().GridSubject(g)
  if epoch~=id.epoch then epoch=id.epoch; counters={} end
  if render then
    local key=id.local_id..":"..name; counters[key]=(counters[key] or 0)+1
    if counters[key]%30~=0 then return end
  end
  local c=context(); c.objects=1; c.runtime=name:find("_UPDATE")~=nil
  local payload={event=name,subject=id,profile=render and "render-sprite" or c.runtime and "runtime-without-resources-or-parameter-queries" or "post-event-public-state"}
  local argumentTypes={MC_POST_GRID_HURT={"int","EntityRef"},MC_POST_GRID_HURT_DAMAGE={"Entity","int","int","float","boolean"},MC_POST_GRID_ROCK_DESTROY={"int","boolean","EntityRef"}}
  if argumentTypes[name] then
    payload.arguments={}; local args=table.pack(...)
    for i,typ in ipairs(argumentTypes[name]) do payload.arguments[i]=read.Value(args[i],typ,c,0) end
  end
  if render then c.owner=id; payload.sprite=read.Collect(g:GetSprite(),"Sprite",c,0)
  else payload.object=snapshot(g,c) end
  local json,complete,objects,nodes,reason=read.Finish(payload,c)
  native().PublishGridEvent(json,complete,objects,nodes,reason,render)
end
-- 失败保留具体回调与净化错误文本，不能吞掉失败后伪称完整。
function M.Error(name,err,render)
  local c=context(); read.Fail(c,"grid_callback_copy")
  local reason=type(err)=="string" and err:gsub("0x%x+","<local>"):sub(1,512) or "grid_callback_copy"
  local json,complete,objects,nodes,detail=read.Finish({event=name,status="ReadFailed",reason=reason},c)
  native().PublishGridEvent(json,complete,objects,nodes,detail,render)
end
function M.Query(owner,class,member,args)
  -- Effect136的保存描述保持Entity身份，显式查询记录真实当前值而不制造Grid出生。
  if class=="EntityEffect" and member=="GetGridEntityDesc" then
    local c=context(); c.objects=1; c.owner=native().Subject(owner)
    local result
    if owner.Type~=1000 or owner.Variant~=136 then result=read.Missing("NotApplicable","requires_grid_projectile_helper")
    else
      local ok,desc=read.Protected(owner:ToEffect(),member)
      if not ok then read.Fail(c,"lifted_descriptor_query") end
      result=ok and read.Value(desc,"GridEntityDesc",c,0) or read.Missing("ReadFailed",member)
    end
    local json,complete,objects,nodes,reason=read.Finish({subject=c.owner,member="EntityEffect.GetGridEntityDesc",result=result,source_grid={resolution="Unresolved",reason="public_api_exposes_no_source_grid_identity"}},c)
    native().PublishGridQuery(json,complete,objects,nodes,reason); return result
  end
  for _,spec in ipairs(schemas[class] or {}) do
    if spec.member==member and (spec.category=="query" or spec.category=="parameter_query") then
      local c=context(); c.objects=1; c.owner=native().GridSubject(owner)
      local result=read.QueryValue(owner,spec,args or {},c,0)
      local json,complete,objects,nodes,reason=read.Finish({subject=c.owner,member=spec.id,parameters=read.Value(args or {},nil,c,0),result=result},c)
      native().PublishGridQuery(json,complete,objects,nodes,reason)
      return result
    end
  end
  return read.Missing("Unavailable","not_a_declared_readonly_query")
end
return M
