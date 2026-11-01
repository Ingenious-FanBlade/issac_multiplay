if IsaacCSObservationValues then return IsaacCSObservationValues end
-- 全部复制在游戏主线程完成；输出只有自有标量／表／规范主体键，不输出 userdata 或地址。
local schemas = include("domains/entity/schema")
local M = { SchemaVersion=2 }
local LIMIT = {objects=16384, nodes=262144, depth=12, bytes=16*1024*1024, queries=131072}
local function missing(status, reason) return {status=status,reason=reason} end
local function native() return IsaacCSObservationNative end
local function protected(object, name, args, field, static)
  return pcall(function()
    if field then return object[name] end
    local fn=object[name]
    if type(fn)~="function" then error("interface_unavailable:"..name) end
    if static then return fn(table.unpack(args or {},1,args and (args.n or #args) or 0)) end
    return fn(object, table.unpack(args or {},1,args and (args.n or #args) or 0))
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
normalize.HistoryItems="HistoryItem"
normalize.Costume="ItemConfigCostume"

-- 参数域与默认调用分开；有限槽和枚举可以完整轮询，无界几何／字符串查询走显式请求。
local function parameterSets(object, spec, ctx)
  local args=spec.parameters or ""
  if spec.owner=="GridEntityDoor" and spec.member=="IsRoomType" or spec.owner=="GridEntityRock" and spec.member=="GetAltRockType" then
    local out,seen={},{}
    for key,v in pairs(_G[spec.member=="IsRoomType" and "RoomType" or "BackdropType"] or {}) do
      if type(v)=="number" and v>=0 and not key:find("NUM_") and not seen[v] then out[#out+1]={v}; seen[v]=true end
    end
    table.sort(out,function(a,b) return a[1]<b[1] end); return out
  end
  if ctx.parameter_sets then
    local sets=ctx.parameter_sets(object,spec)
    if sets~=nil then return sets end
  end
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
  typ=normalize[typ] or typ or typeName(object)
  if typ then typ=typ:gsub("%?","") end
  if typ and typ:find("^Entity") and not typ:find("^EntityConfig") and typ~="EntityDesc" and typ~="EntityRef" and typ~="EntityList" then
    local reference=native().Reference(object)
    if reference.resolution=="Unresolved" or reference.resolution=="AmbiguousReuse" then fail(ctx,"reference:"..reference.resolution) end
    return reference
  end
  if typ=="EntityRef" then return native().Reference(object.Entity) end
  if typ=="RNG" then
    if IsaacCSRandomObservation then return IsaacCSRandomObservation.State(object) end
    if not ctx.grid then return missing("NotCaptured","random_module_unavailable") end
    return {type="random_state",owner=ctx.owner,member=ctx.member,seed=value(object:GetSeed(),"int",ctx,depth+1),shift=value(object:GetShiftIdx(),"int",ctx,depth+1),registry="random:tx-04-14"}
  end
  if typ and typ:find("^GridEntity") and typ~="GridEntityDesc" then return native().GridReference(object) end
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
    local keys={}; for k in pairs(object) do if #keys>=LIMIT.nodes-ctx.nodes then fail(ctx,"table_key_budget");break end;keys[#keys+1]=k end
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
  -- 房间配置容器只复制已经存在的条目，不调用PickEntry或加载房间集。
  if typ=="RoomConfigSpawns" or typ=="RoomConfigEntries" then
    local out={}; local size=object.Size
    if not size or size<0 or size>LIMIT.nodes then fail(ctx,"room_config_container_budget"); return missing("Truncated","size") end
    for i=0,size-1 do
      if ctx.nodes>=LIMIT.nodes then fail(ctx,"room_config_container_budget"); break end
      out[#out+1]=value(object:Get(i),typ=="RoomConfigSpawns" and "RoomConfigSpawn" or "RoomConfigEntry",ctx,depth+1)
    end
    return array(out)
  end
  if ctx.domain_value then
    local handled,result=ctx.domain_value(object,typ,ctx,depth)
    if handled then return result end
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
  ctx.member=spec.id
  if ctx.grid and spec.owner=="GridEntity" and spec.member=="GetRenderPosition" then
    return native().GridRenderPosition(object)
  end
  if spec.owner=="Game" and spec.member=="GetGenericPrompt" then
    local p=native().PromptState()
    if not p then fail(ctx,"prompt_root_unavailable");return missing("Unavailable","prompt_root") end
    return {type="GenericPrompt",source="direct_live_root;no_cpp_object_copy",fields={["GenericPrompt.GetCurrentSelection"]=value(p.selection,"int",ctx,depth+1),["GenericPrompt.GetSubmittedSelection"]=value(p.submitted,"int",ctx,depth+1),["GenericPrompt.IsActive"]=p.active,["GenericPrompt.GetSprite"]=collect(p.sprite,"Sprite",ctx,depth+1)}}
  end
  -- 本地1.1.3绑定已将旧Sprite入口迁移到HUDMessage，读取同一真实消息对象。
  if spec.owner=="HUD" and (spec.member=="GetPlayerStreakSprite" or spec.member=="GetStackedStreakSprite") and not object[spec.member] then
    local ok,v=pcall(function() return (spec.member=="GetPlayerStreakSprite" and object:GetCoopPlayerMessage(args[1]) or object:GetStackedMessage(args[1])):GetSprite() end)
    if ok then return {source="REPENTOGON1.1.3.HUDMessage.GetSprite",value=value(v,"Sprite",ctx,depth+1)} end
    fail(ctx,"hud_message_sprite");return missing("ReadFailed",textError(v))
  end
  local ok,v,extra=protected(spec.static and _G[spec.owner] or object,spec.member,args,spec.category=="field",spec.static)
  if not ok then fail(ctx,"read:"..spec.id); return missing("ReadFailed",textError(v)) end
  local copied,out=pcall(value,v,spec.returns,ctx,depth+1)
  if not copied then fail(ctx,"value_copy:"..spec.id); return missing("ReadFailed",textError(out)) end
  if extra~=nil then return {returns={out,value(extra,spec.returns:match(",%s*([%w_]+)"),ctx,depth+1)}} end
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
      if ctx.runtime and (spec.returns=="Sprite" or spec.category=="parameter_query" or spec.member=="GetRNG" or spec.member=="GetRenderPosition") then
        -- 独立更新只复制运行字段；资源与静态参数查询由普通域窗口负责。
      elseif spec.category=="field" or spec.category=="query" then
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
      elseif spec.category=="domain_value" and ctx.domain_member then
        out.fields[spec.id]=ctx.domain_member(object,spec,ctx,depth)
      elseif spec.category=="external_reference" then
        if spec.reason=="random:tx-04-14" and IsaacCSRandomObservation then
          out.fields[spec.id]={type="owner_qualified_domain_reference",schema="isaac.cs.random",member=spec.id,owner=ctx.owner or native().Reference(object),semantics="actual_getter_registers_stream;lazy_item_RNG_not_polled"}
        elseif spec.owner=="EntityPlayer" and (spec.member=="GetHistory" or spec.member=="GetMovingBoxContents") then
          out.fields[spec.id]={type="owner_qualified_domain_reference",schema=spec.member=="GetHistory" and "isaac.cs.player" or "isaac.cs.room_save",owner=native().Reference(object),field=spec.member=="GetHistory" and "bodies.history" or "carried.moving_box",scope_rule="same_committed_native_window;target_coverage_applies"}
        elseif spec.member=="GetPlayerHUD" then out.fields[spec.id]={type="owner_qualified_domain_reference",schema="isaac.cs.presentation",owner=native().Reference(object)}
        elseif ctx.external_reference then out.fields[spec.id]=ctx.external_reference(object,spec)
        else out.fields[spec.id]=missing("NotCaptured",spec.reason) end
      elseif spec.category=="actual_call" then
        out.fields[spec.id]={status="NotScheduled",reason=spec.reason,source="actual_call_event"}
      elseif spec.category=="derived_query" then
        out.fields[spec.id]={status="NotScheduled",reason="derived_query_requires_explicit_parameters",signature=spec.parameters}
      end
    end
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

-- 两域共用相同的有界值语义，schema 注册只合并声明，不执行 Getter。
function M.AddSchemas(extra) for k,v in pairs(extra) do schemas[k]=v end end
M.Encode=encode
M.Collect=collect; M.Value=value; M.QueryValue=query; M.Finish=finish; M.Fail=fail; M.Missing=missing; M.Protected=protected
function M.Context() return {complete=true,nodes=0,queries=0,objects=0,reasons={},seen={},config_seen={},sprite_names={},graph=0} end
function M.Limits(limits) if limits then for k,v in pairs(limits) do LIMIT[k]=v end end return LIMIT end
-- 官方只读根返回的Player可使用公开Getter；不为旧native私有字段补造Init/更新见证。
-- 所有借用仅在此窗口局部存在，采集结束重新核对公开root和关系边。
function M.PlayerSources(ctx)
  local out,seen,witnesses={},{},{}
  local function add(p)
    if not p then return end
    local id=native().Subject(p)
    if id.local_id=="0" or id.resolution=="Retired" then fail(ctx,"public_player_identity");return end
    if not seen[id.local_id] then
      if #out>=256 then fail(ctx,"public_player_source_budget");return end
      seen[id.local_id]=true;out[#out+1]={player=p,subject=id}
    end
  end
  local function roots()
    local ids={}
    for _,p in ipairs(PlayerManager.GetPlayers()) do add(p);ids[#ids+1]=native().Subject(p).local_id end
    ids[#ids+1]="backups"
    for i=0,3 do local p=PlayerManager.GetEsauJrState(i);add(p);ids[#ids+1]=p and native().Subject(p).local_id or "Null" end
    return table.concat(ids,"|")
  end
  local root=roots();local cursor=1
  while cursor<=#out do
    local p=out[cursor].player;cursor=cursor+1
    for _,name in ipairs({"GetMainTwin","GetOtherTwin","GetSubPlayer"}) do
      local target=p[name](p);add(target);witnesses[#witnesses+1]={owner=p,member=name,id=target and native().Subject(target).local_id or "Null"}
    end
  end
  return out,function()
    if roots()~=root then fail(ctx,"public_player_roots_changed") end
    for _,w in ipairs(witnesses) do local p=w.owner[w.member](w.owner);if (p and native().Subject(p).local_id or "Null")~=w.id then fail(ctx,"public_player_relation_changed") end end
  end
end
IsaacCSObservationValues=M
return M
