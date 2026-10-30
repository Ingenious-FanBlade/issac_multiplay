-- 全部复制在游戏主线程完成；输出只有自有标量／表／规范主体键，不输出 userdata 或地址。
local schemas = include("domains/entity/schema")
local slots = include("domains/entity/slot_variants")
local M = { SchemaVersion=2 }
local LIMIT = {objects=16384, nodes=262144, depth=12, bytes=16*1024*1024, queries=131072}
local function missing(status, reason) return {status=status,reason=reason} end
local function native() return IsaacCSObservationNative end
local function protected(object, name, args, field, static)
  return pcall(function()
    if field then return object[name] end
    local fn=object[name]
    if type(fn)~="function" then error("interface_unavailable:"..name) end
    if static then return fn(table.unpack(args or {})) end
    return fn(object, table.unpack(args or {}))
  end)
end
local function textError(value)
  -- Lua 错误仅保留成员语义，不将 userdata 的本地地址混入规范值。
  return type(value)=="string" and value:gsub("0x%x+","<local>"):sub(1,256) or "lua_error"
end
local function fail(ctx,reason)
  ctx.complete=false
  ctx.reasons[reason]=(ctx.reasons[reason] or 0)+1
end
local function array(values) return {kind="array",values=values} end
local function typeName(value)
  local ok, name=pcall(function() return getmetatable(value).__type end)
  return ok and type(name)=="string" and name or nil
end
local normalize={ ["ItemConfig Item"]="ItemConfigItem", ["ItemConfig_Item"]="ItemConfigItem", ["ItemConfig_Costume"]="ItemConfigCostume", ["ItemConfig Costume"]="ItemConfigCostume", ["PlayerTypesActiveItemDesc"]="ActiveItemDesc", ["PlayerTypesPosVel"]="PosVel", ["Pathfinder"]="PathFinder", ["LayerStates"]="LayerState", ["const Color"]="Color", ["const Vector"]="Vector", ["EntityPlayer, boolean"]="EntityPlayer" }
local collect, value, query
-- 原版文档的显示别名仍指向 REPENTOGON 的同一只读关联结构。
normalize.PlayerPocketItem="PocketItem"
normalize.Costume="ItemConfigCostume"

-- 参数域与默认调用分开；有限槽和枚举可以完整轮询，无界几何／字符串查询走显式请求。
local function parameterSets(object, spec, ctx)
  local args=spec.parameters or ""
  local name=spec.member
  if spec.owner=="Sprite" and (name=="GetNullFrame" or name=="GetOverlayNullFrame" or name:find("EventTriggered")) then
    local overlay=name:find("Overlay")~=nil
    if overlay and not object:GetOverlayAnimationData() or not overlay and not object:GetCurrentAnimationData() then return {} end
    local names=ctx.sprite_names[object]
    if not names then names=native().SpriteNames(object); ctx.sprite_names[object]=names end
    if names.status~="Value" then fail(ctx,"sprite_name_inventory:"..names.status) end
    local out={}; for _,s in ipairs(name:find("EventTriggered") and names.events or names.nulls) do out[#out+1]={s} end
    return out
  end
  if name=="GetWeapon" then return {{0},{1},{2},{3},{4}} end
  if name=="GetBagOfCraftingSlot" then local r={} for i=0,7 do r[#r+1]={i} end return r end
  if name=="GetCard" or name=="GetPill" or name=="GetPocketItem" then return {{0},{1},{2},{3}} end
  if name=="GetTrinket" then return {{0},{1}} end
  if name=="GetPoopSpell" then return {{0},{1},{2},{3},{4},{5}} end
  if name=="IsBlackHeart" or name=="IsBoneHeart" then
    local r={}
    if name=="IsBlackHeart" then for i=1,object:GetSoulHearts() do r[#r+1]={i} end
    else for i=0,object:GetBoneHearts()+math.ceil(object:GetSoulHearts()/2)-1 do r[#r+1]={i} end end
    return r
  end
  if name=="GetFootprintColor" then return {{false},{true}} end
  if name=="GetSmeltedTrinkets" then return {{}} end
  if name=="IsFootstepFrame" then return {{-1},{0},{1}} end
  if name=="IsActiveEnemy" then return {{false},{true}} end
  if spec.id:find("^Sprite%.") and (name=="GetLayerFrameData" or name=="GetOverlayLayerFrameData" or name=="GetSpritesheet") then
    local r={}; for i=0,object:GetLayerCount()-1 do r[#r+1]={i} end; return r
  end
  if spec.id:find("^Sprite%.") and (name=="IsPlaying" or name=="IsFinished") then return {{object:GetAnimation()}} end
  if spec.id:find("^Sprite%.") and (name=="IsOverlayPlaying" or name=="IsOverlayFinished") then return {{object:GetOverlayAnimation()}} end
  local first=args:match("^%s*(%w+)")
  if first=="ActiveSlot" then return {{0},{1},{2},{3}} end
  local enum=_G[first or ""]
  if type(enum)=="table" and (first=="PlayerForm" or first=="WeaponType" or first=="BombCostumeLayer") then
    local items,seen={},{}
    for k,v in pairs(enum) do
      if type(v)=="number" and v>=0 and not seen[v] and not k:find("NUM_") then
        seen[v]=true
        if args:find("boolean") or args:find("bool ") then items[#items+1]={v,false}; items[#items+1]={v,true}
        else items[#items+1]={v} end
      end
    end
    table.sort(items,function(a,b) return a[1]<b[1] end)
    return items
  end
  -- 文档中的显式默认值可用，但绝不自动执行 Random()/self 的任意表达式。
  if args~="" and not args:find("Random") then
    local out={}; local all=true
    for p in args:gmatch("[^,]+") do
      local d=p:match("=%s*(.-)%s*$")
      if d=="false" then out[#out+1]=false elseif d=="true" then out[#out+1]=true
      elseif d and tonumber(d) then out[#out+1]=tonumber(d)
      else all=false end
    end
    if all then return {out} end
  end
  return nil
end

local function scalar(value)
  if type(value)=="number" and (value~=value or value==math.huge or value==-math.huge) then return nil,"Nonfinite" end
  if type(value)=="number" and math.type(value)=="integer" then return {type="int64",value=tostring(value)} end
  if type(value)=="string" and utf8 and not utf8.len(value) then return {type="bytes",encoding="lua-string-bytes",hex=(value:gsub(".",function(c) return string.format("%02x",c:byte()) end))} end
  if type(value)=="number" or type(value)=="boolean" or type(value)=="string" then return value end
end

-- 显式资源／状态适配，未知 userdata 必须报告缺口；禁止退回 tostring(pointer)。
value=function(object, typ, ctx, depth)
  if ctx.nodes>=LIMIT.nodes or depth>LIMIT.depth then fail(ctx,"value_budget"); return missing("Truncated","node_or_depth_budget") end
  ctx.nodes=ctx.nodes+1
  if object==nil then return missing("Null","nil") end
  local s,err=scalar(object)
  if s~=nil then return s end
  if err then fail(ctx,err); return {status=err,representation=object~=object and "nan" or object>0 and "positive_infinity" or "negative_infinity"} end
  typ=normalize[typ] or typ
  if typ then typ=typ:gsub("%?","") end
  if typ and typ:find("^Entity") and not typ:find("^EntityConfig") and typ~="EntityDesc" and typ~="EntityRef" and typ~="EntityList" then
    local reference=native().Reference(object)
    if reference.resolution=="Unresolved" or reference.resolution=="AmbiguousReuse" then fail(ctx,"reference:"..reference.resolution) end
    return reference
  end
  if typ=="EntityRef" then return native().Reference(object.Entity) end
  if typ=="RNG" then return missing("NotCaptured","owner_qualified_random_reference:tx-04-14") end
  if typ=="TearFlags" or typ=="BitSet128" then return {type="bits128",low=tostring(object.l),high=tostring(object.h)} end
  if typ=="Vector" then return {type="vector",x=value(object.X,"float",ctx,depth+1),y=value(object.Y,"float",ctx,depth+1)} end
  if typ=="Color" then
    local r={type="color"}; for _,k in ipairs({"R","G","B","A","RO","GO","BO"}) do r[k]=value(object[k],"float",ctx,depth+1) end
    for _,k in ipairs({"GetColorize","GetTint","GetOffset"}) do
      local ok,v=protected(object,k)
      if ok then r[k]=value(v,"table",ctx,depth+1) end
    end
    return r
  end
  if typ=="KColor" then return {R=value(object.Red,"float",ctx,depth+1),G=value(object.Green,"float",ctx,depth+1),B=value(object.Blue,"float",ctx,depth+1),A=value(object.Alpha,"float",ctx,depth+1)} end
  if type(object)=="table" then
    if ctx.seen[object] then return {type="graph_ref",node=ctx.seen[object]} end
    ctx.graph=ctx.graph+1; ctx.seen[object]=ctx.graph
    local node={type="table",node=ctx.graph,entries={}}
    local keys={}; for k in pairs(object) do keys[#keys+1]=k end
    table.sort(keys,function(a,b) return type(a)==type(b) and tostring(a)<tostring(b) or type(a)<type(b) end)
    for _,k in ipairs(keys) do
      if ctx.nodes>=LIMIT.nodes then fail(ctx,"table_budget"); node.truncated=true; break end
      local elementType=typ and typ:gsub("%[%]","") or nil
      node.entries[#node.entries+1]={key=value(k,nil,ctx,depth+1),value=value(object[k],elementType,ctx,depth+1)}
    end
    if #keys==0 then node.status="Empty" end
    return node
  end
  if typ=="VectorList" or typ=="EffectList" or typ=="EntityList" then
    local out={}; local size=object.Size
    if not size or size>LIMIT.nodes then fail(ctx,"container_budget"); return missing("Truncated","container_size") end
    for i=0,size-1 do out[#out+1]=value(object:Get(i),typ=="VectorList" and "Vector" or typ=="EntityList" and "Entity" or "TemporaryEffect",ctx,depth+1) end
    return array(out)
  end
  if schemas[typ] then return collect(object,typ,ctx,depth+1) end
  -- 基础 C++ 结构在 Wiki 中使用不同的显示名。
  -- Wiki 未公开 HomingLaser 的私有成员；完整公开曲线由 Laser 两个 Samples Getter 表达。
  if typ=="HomingLaser" then return {type="owner_qualified_opaque",public_state="EntityLaser.GetSamples/GetNonOptimizedSamples"} end
  local fields={PosVel={"Position","Velocity"}}
  if fields[typ] then
    local r={type=typ}; for _,f in ipairs(fields[typ]) do local ok,v=protected(object,f,nil,true); r[f]=ok and value(v,nil,ctx,depth+1) or missing("ReadFailed",f) end; return r
  end
  fail(ctx,"unsupported_value:"..tostring(typ or typeName(object) or type(object)))
  return missing("NotCaptured","unsupported_value:"..tostring(typ or typeName(object) or type(object)))
end

query=function(object,spec,args,ctx,depth)
  if ctx.queries>=LIMIT.queries then fail(ctx,"query_budget"); return missing("Truncated","query_budget") end
  ctx.queries=ctx.queries+1
  if spec.owner=="Sprite" then
    local overlay=spec.member:find("Overlay")~=nil
    local needsAnimation=spec.member:find("EventTriggered") or spec.member=="GetNullFrame" or spec.member=="GetOverlayNullFrame" or spec.member=="IsPlaying" or spec.member=="IsFinished" or spec.member=="IsOverlayPlaying" or spec.member=="IsOverlayFinished"
    if needsAnimation and (overlay and not object:GetOverlayAnimationData() or not overlay and not object:GetCurrentAnimationData()) then return missing("NotApplicable","no_current_animation") end
  end
  local ok,v,extra=protected(spec.static and _G[spec.owner] or object,spec.member,args,spec.category=="field",spec.static)
  if not ok then fail(ctx,"read:"..spec.id); return missing("ReadFailed",textError(v)) end
  local copied,out=pcall(value,v,spec.returns,ctx,depth+1)
  if not copied then fail(ctx,"value_copy:"..spec.id); return missing("ReadFailed",textError(out)) end
  if extra~=nil then return {returns={out,value(extra,nil,ctx,depth+1)}} end
  return out
end

collect=function(object,class,ctx,depth)
  -- 配置 counterpart 可能成环，按公开逻辑键记录引用；userdata 包装地址不进入身份。
  if class=="EntityConfigPlayer" then
    local key="EntityConfigPlayer:"..tostring(object:GetPlayerType())
    if ctx.config_seen[key] then return {type="resource_ref",key=key} end
    ctx.config_seen[key]=true
  end
  local out={type=class,fields={}}
  local handled={}
  for _,spec in ipairs(schemas[class] or {}) do
    if not handled[spec.id] then
      handled[spec.id]=true
      if spec.category=="field" or spec.category=="query" then
        out.fields[spec.id]=query(object,spec,{},ctx,depth)
      elseif spec.category=="parameter_query" then
        local sets=parameterSets(object,spec,ctx)
        if sets then
          local results={}
          for _,args in ipairs(sets) do
            if ctx.queries>=LIMIT.queries then fail(ctx,"query_budget"); results[#results+1]=missing("Truncated","query_budget"); break end
            results[#results+1]={parameters=value(args,nil,ctx,depth+1),value=query(object,spec,args,ctx,depth)}
          end
          out.fields[spec.id]={kind="parameter_results",values=results}
        else out.fields[spec.id]={status="NotScheduled",reason="derived_query_requires_explicit_parameters",signature=spec.parameters} end
      elseif spec.category=="external_reference" then
        out.fields[spec.id]=missing("NotCaptured",spec.reason)
      elseif spec.category=="actual_call" then
        out.fields[spec.id]={status="NotScheduled",reason=spec.reason,source="actual_call_event"}
      elseif spec.category=="derived_query" then
        out.fields[spec.id]={status="NotScheduled",reason="derived_query_requires_explicit_parameters",signature=spec.parameters}
      end
    end
  end
  return out
end

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

-- 自有 JSON 编码器保留空集合、非有限值和整数精度；预算不足返回明确缺口而非截断 JSON。
local function encode(root)
  local parts,bytes={},0
  local function emit(s) bytes=bytes+#s; if bytes>LIMIT.bytes then error("payload_byte_budget") end; parts[#parts+1]=s end
  local function quote(s) return '"'..s:gsub('[%z\1-\31\\"]',function(c) return string.format('\\u%04x',c:byte()) end)..'"' end
  local function write(v,depth)
    if depth>LIMIT.depth*4 then error("json_depth_budget") end
    local t=type(v)
    if t=="nil" then emit("null") elseif t=="string" then emit(quote(v))
    elseif t=="number" then
      if v~=v or v==math.huge or v==-math.huge then error("json_nonfinite") end
      emit(math.type(v)=="integer" and tostring(v) or v==0 and 1/v<0 and "-0.0" or string.format("%.17g",v))
    elseif t=="boolean" then emit(v and "true" or "false")
    elseif t=="table" then
      local isArray=#v>0
      emit(isArray and "[" or "{")
      local first=true
      if isArray then for _,x in ipairs(v) do if not first then emit(",") end; first=false; write(x,depth+1) end
      else local keys={}; for k in pairs(v) do keys[#keys+1]=k end; table.sort(keys)
        for _,k in ipairs(keys) do if not first then emit(",") end; first=false; emit(quote(tostring(k))); emit(":"); write(v[k],depth+1) end
      end
      emit(isArray and "]" or "}")
    else error("borrowed_value_in_payload") end
  end
  write(root,0); return table.concat(parts),bytes
end
local function finish(payload,ctx)
  payload.coverage={complete=ctx.complete,reasons=ctx.reasons,objects=ctx.objects,nodes=ctx.nodes,queries=ctx.queries}
  local ok,json,bytes=pcall(encode,payload)
  if not ok then
    -- 字节预算超限时只裁剪已经复制的值，保留可交付前缀；绝不再次调用游戏 Getter。
    ctx.complete=false; payload.coverage.complete=false; payload.coverage.reasons.payload_byte_or_depth_budget=1
    for _=1,64 do
      local candidate,keys,score
      local function visit(t,depth)
        if type(t)~="table" or depth>48 then return end
        local n=#t
        if n>1 and (not score or n>score) then candidate=t; keys=nil; score=n end
        if type(t.fields)=="table" then
          local list={}; for key in pairs(t.fields) do list[#list+1]=key end; table.sort(list)
          if #list>1 and (not score or #list>score) then candidate=t.fields; keys=list; score=#list end
        end
        for k,v in pairs(t) do if k~="coverage" and k~="subject" and k~="membership" then visit(v,depth+1) end end
      end
      visit(payload,0)
      if not candidate then break end
      local first=math.max(1,math.floor(score/2))
      if keys then for i=first+1,#keys do candidate[keys[i]]=nil end
      else for i=first+1,score do candidate[i]=nil end end
      payload.coverage.pruned_values=(payload.coverage.pruned_values or 0)+score-first
      -- 裁剪后只保留仍有定义的图引用，不能输出悬空 node 并假装目标完整。
      local retained={}
      local function nodes(t)
        if type(t)~="table" then return end
        if t.type=="table" and t.node then retained[t.node]=true end
        for _,v in pairs(t) do nodes(v) end
      end
      local function references(t)
        if type(t)~="table" then return end
        if t.type=="graph_ref" and not retained[t.node] then
          t.type=nil; t.node=nil; t.status="Truncated"; t.reason="graph_target_pruned"
        end
        for _,v in pairs(t) do references(v) end
      end
      nodes(payload); references(payload)
      ok,json,bytes=pcall(encode,payload)
      if ok then return json,false,ctx.objects,ctx.nodes,"payload_byte_or_depth_budget",bytes end
    end
    return '{"status":"Truncated","reason":"single_value_exceeds_payload_budget"}',false,ctx.objects,ctx.nodes,"single_value_exceeds_payload_budget"
  end
  return json,ctx.complete,ctx.objects,ctx.nodes,ctx.complete and "" or "field_or_budget_gap",bytes
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
