-- Entity 主体组织；有界值复制和资源姿态与其他域共用。
local read=include("domains/common/read")
local slots=include("domains/entity/slot_variants")
local schemas=include("domains/entity/schema")
local M={SchemaVersion=2}
local native=function() return IsaacCSObservationNative end
local context,collect,value,query,finish,fail,missing,protected=read.Context,read.Collect,read.Value,read.QueryValue,read.Finish,read.Fail,read.Missing,read.Protected
local LIMIT=read.Limits()
local conversions={ {"ToPlayer","EntityPlayer"},{"ToNPC","EntityNPC"},{"ToTear","EntityTear"},{"ToProjectile","EntityProjectile"},{"ToBomb","EntityBomb"},{"ToLaser","EntityLaser"},{"ToKnife","EntityKnife"},{"ToPickup","EntityPickup"},{"ToFamiliar","EntityFamiliar"},{"ToEffect","EntityEffect"},{"ToSlot","EntitySlot"} }
local function context() return {complete=true,nodes=0,queries=0,objects=0,reasons={},seen={},config_seen={},sprite_names={},graph=0} end
local function snapshot(e,ctx,class)
  local subject=native().Subject(e)
  if subject.local_id=="0" then fail(ctx,"subject_budget"); return {subject=subject,status="NotCaptured"} end
  if subject.resolution=="Retired" then return {subject=subject,status="NotApplicable",reason="retired_at_source"} end
  local out={subject=subject,common=collect(e,"Entity",ctx,0)}
  if class and class~="Entity" then out.body=collect(e,class,ctx,0)
  else
    for _,c in ipairs(conversions) do
      local ok,derived=protected(e,c[1])
      if ok and derived then class=c[2]; out.body=collect(derived,class,ctx,0); break end
    end
  end
  if class=="EntityNPC" then
    local ok,d=protected(e,"ToDelirium")
    if ok and d then out.special=collect(d,"EntityDelirium",ctx,0) end
  end
  out.class=class or "UnknownEntity"
  if class=="EntitySlot" then out.variant_semantics=slots[e.Variant] or missing("Unavailable","unmapped_slot_variant"); if not slots[e.Variant] then fail(ctx,"unmapped_slot_variant") end end
  if out.class=="UnknownEntity" then
    local supplement=native().Supplement(e)
    if supplement.class~="UnknownEntity" then
      out.class=supplement.class; out.body={type=supplement.class,status=supplement.status,fields={}}
      if supplement.class=="NativeText" then
        for _,key in ipairs({"counter","duration","text"}) do
          out.body.fields["NativeText."..key]=supplement[key]~=nil and value(supplement[key],nil,ctx,1) or missing(supplement.status,"native_readiness_or_copy")
        end
      end
      if supplement.status~="Value" then fail(ctx,"native_supplement:"..supplement.status) end
    else fail(ctx,"unknown_entity_type:"..tostring(e.Type)) end
  end
  return out
end

function M.Capture(limits)
  if limits then
    LIMIT.objects=limits.objects; LIMIT.nodes=limits.nodes; LIMIT.depth=limits.depth; LIMIT.bytes=limits.bytes
  end
  local ctx=context(); local list=Isaac.GetRoomEntities(); local subjects={}; local seen={}; local enumerationComplete=true
  -- 先登记全部主体，再读取字段／解析关系；Lua 枚举缺席不能推定销毁。
  local nativeSources,unavailable=native().Sources()
  if unavailable>0 then fail(ctx,"native_source_unavailable"); enumerationComplete=false end
  for _,entry in ipairs(nativeSources) do list[#list+1]=entry.entity end
  for _,e in ipairs(list) do
    local id=native().Subject(e)
    if id.local_id=="0" then fail(ctx,"subject_registry_budget"); enumerationComplete=false; subjects[#subjects+1]=e
    elseif not seen[id.local_id] then seen[id.local_id]=true; subjects[#subjects+1]=e end
  end
  local out={objects={},membership={source="Isaac.GetRoomEntities+native_membership",count=#subjects,native_count=#nativeSources,unavailable_native=unavailable,complete=enumerationComplete}}
  for i,e in ipairs(subjects) do
    if i>LIMIT.objects then fail(ctx,"object_budget"); out.membership.complete=false; break end
    ctx.objects=ctx.objects+1; out.objects[#out.objects+1]=snapshot(e,ctx)
  end
  return finish(out,ctx)
end
function M.Event(name,class,object,...)
  local ctx=context(); ctx.objects=1
  -- Remove 回调只取真实事件主体，避免解引用已经进入销毁阶段的派生关联。
  local payload={event=name,subject=native().Subject(object)}
  payload.arguments=value({...},nil,ctx,0)
  if name~="MC_POST_ENTITY_REMOVE" then payload.object=snapshot(object,ctx,class) end
  local json,complete,objects,nodes,reason=finish(payload,ctx)
  native().PublishEvent(json,complete,objects,nodes,reason)
end
local renderEpoch,renderCounters=nil,{}
-- render 的姿态单独封存，每个主体每30次真实回调采样；不把渲染帧当作 Authority tick。
function M.Render(object)
  local subject=native().Subject(object)
  if renderEpoch~=subject.epoch then renderEpoch=subject.epoch; renderCounters={} end
  local key=subject.local_id
  renderCounters[key]=(renderCounters[key] or 0)+1
  if renderCounters[key]%30~=0 then return end
  local ctx=context(); ctx.objects=1
  local json,complete,objects,nodes,reason=finish({subject=subject,sprite=collect(object:GetSprite(),"Sprite",ctx,0)},ctx)
  native().PublishRender(json,complete,objects,nodes,reason)
end
-- 显式派生查询同样受 schema 白名单与预算约束，不能用此入口执行玩法动作。
function M.Query(owner,class,member,args,object)
  object=object or owner
  for _,spec in ipairs(schemas[class] or {}) do
    if spec.member==member and (spec.category=="query" or spec.category=="parameter_query" or spec.category=="derived_query") then
      local ctx=context(); ctx.objects=1
      local result=query(object,spec,args or {},ctx,0)
      local json,complete,objects,nodes,reason=finish({subject=native().Subject(owner),member=spec.id,parameters=value(args or {},nil,ctx,0),result=result},ctx)
      native().PublishQuery(json,complete,objects,nodes,reason)
      return result
    end
  end
  return missing("Unavailable","not_a_declared_readonly_query")
end
return M
