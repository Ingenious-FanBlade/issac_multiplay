-- 保存项属于不可变集合修订，绝不借seed或数组索引继承活动Entity/Grid身份。
local shared=include("domains/common/continuity")
local read=shared.Read
read.AddSchemas(include("domains/room_save/schema")); read.AddSchemas(include("domains/grid/schema"))
local store=shared.Store(true)
local M={SchemaVersion=2}
-- 固定保存Grid列声明一次；每槽仍有独立SaveEntry和值，不重复长字段名和namespace。
local gridColumns={"GridEntityDesc.Initialized","GridEntityDesc.SpawnCount","GridEntityDesc.SpawnSeed","GridEntityDesc.State","GridEntityDesc.Type","GridEntityDesc.VarData","GridEntityDesc.VariableSeed","GridEntityDesc.Variant"}
-- 有类型的精确比较保留旧编码区分的正/负零；整数包装、真实空集合和缺口逐字段比较。
local function equalValues(a,b)
  if type(a)~=type(b) then return false end
  if type(a)=="number" then return a==b and (a~=0 or 1/a==1/b) end
  if type(a)=="userdata" then return IsaacCSObservationNative.OwnedBlockEqual(a,b) end
  if type(a)~="table" then return a==b end
  for k,v in pairs(a) do if not equalValues(v,b[k]) then return false end end
  for k in pairs(b) do if a[k]==nil then return false end end
  return true
end
local function copyVector(vector,kind,c)
  local size=#vector; local out={}
  c.objects=c.objects+1 -- 域对象是保存容器；行数由nodes/queries/bytes独立约束。
  if size<0 or size>16384 then read.Fail(c,"save_container_budget") end
  -- 仍在当前窗口完整读取，批量桥只消除逐槽userdata及八次pcall，不获得跨窗复用资格。
  if kind=="grid" and size<=16384 and c.nodes+size*8<=read.Limits().nodes and c.queries+size*8<=read.Limits().queries then
    local ok,rows=pcall(IsaacCSObservationNative.GridSaveValues,vector,16384)
    if ok then c.nodes=c.nodes+size*8;c.queries=c.queries+size*8;return rows,size end
    read.Fail(c,"save_grid_bulk_read");return {},size
  end
  for i=0,math.min(size,16384)-1 do
    if c.nodes>=read.Limits().nodes or c.objects>=16384 then read.Fail(c,"save_node_or_object_budget"); break end
    local copied=read.Collect(vector:Get(i),kind=="grid" and "GridEntityDesc" or "EntitiesSaveState",c,0)
    if kind=="grid" then
      local row={}
      for j,key in ipairs(gridColumns) do
        local v=copied.fields[key]
        row[j]=type(v)=="table" and v.type=="int64" and v.value or v
      end
      out[#out+1]=row
    else out[#out+1]=copied end
  end
  return out,size
end
function M.Vector(vector,kind,c) return copyVector(vector,kind,c) end
function M.Capture(limits)
  read.Limits(limits); read.Limits({queries=262144}); local c=shared.Model.Context(); local x,old=store:Begin()
  local out={location=x,rooms={},carried={},changes={},sampling={cache_reused=false,freshness="ReadInCurrentNativeWindow"},previous_scope=old and old.context.scope or {status="NotCaptured"},source_semantics="public_save_fields_only;not_full_engine_save"}
  local nextState={}
  local function collection(owner,key,vector,kind,construction)
    local values,size=copyVector(vector,kind,c); local before=old and old.data[key]
    -- 比较有类型的当前值；不为判等先编码JSON，保存项身份仍只依赖真实内容修订与ordinal。
    local unchanged=before and before.construction==construction and equalValues(before.values,values)
    local revision=unchanged and before.revision or before and before.revision+1 or 1
    local entries={}
    for i,v in ipairs(values) do
      local id=unchanged and before.ids[i] or IsaacCSObservationNative.SaveEntry()
      if id=="0" then read.Fail(c,"save_entry_identity") end
      if kind=="grid" then entries[#entries+1]={local_id=id,ordinal=i-1,values=v}
      else entries[#entries+1]={subject={space=8,local_id=id,session=x.session,epoch=x.epoch},ordinal=i-1,value=v,relation={resolution="Unresolved",reason="public_save_API_has_no_native_entity_or_restore_identity;seed_and_index_not_identity"}} end
    end
    local ids={}; for i,e in ipairs(entries) do ids[i]=kind=="grid" and e.local_id or e.subject.local_id end
    nextState[key]={values=values,ids=ids,revision=revision,construction=construction}
    if not unchanged then out.changes[#out.changes+1]={owner=owner,collection=kind,revision=tostring(revision),previous_revision=before and tostring(before.revision),kind=before and "CollectionReplaced" or "FirstObserved",previous_scope=old and old.context.scope} end
    return {owner=owner,kind=kind,revision=tostring(revision),count=size,entries=entries,status=size==0 and "Empty" or "Present",encoding=kind=="grid" and "grid_columns_v2" or "named_fields_v1",columns=kind=="grid" and gridColumns or nil,column_types=kind=="grid" and {"boolean","int64_decimal","int64_decimal","int64_decimal","int64_decimal","int64_decimal","int64_decimal","int64_decimal"} or nil,entry_namespace={space=8,session=x.session,epoch=x.epoch},entry_relation={resolution="Unresolved",reason="public_save_API_has_no_native_entity_or_restore_identity;seed_and_index_not_identity"},identity_basis="immutable_collection_revision_and_ordinal;not_native_object_continuity",source_scope=x.scope}
  end
  for _,entry in ipairs(shared.Descriptors(c)) do
    local d,id=entry.descriptor,entry.subject; local key=id.local_id..":"..id.construction_revision
    local current=id.local_id==x.descriptor
    local r={subject=id,lifetime="current_floor_and_construction_revision",status=current and "CurrentActive;save_container_may_be_stale" or d.VisitedCount==0 and "GeneratedUnvisited" or "InactivePublicSave",collections={}}
    for _,spec in ipairs({{"entity","GetEntitiesSaveState"},{"grid","GetGridEntitiesSaveState"},{"decoration","GetDecoSaveState"}}) do
      r.collections[#r.collections+1]=collection(id,key..":"..spec[1],d[spec[2]](d),spec[1],id.construction_revision)
    end
    out.rooms[#out.rooms+1]=r
  end
  out.carried[#out.carried+1]=collection({space=4,local_id=x.floor},"myosotis",Game():GetLevel():GetMyosotisPickups(),"myosotis",x.floor)
  local players,verify=shared.Players(c)
  for _,entry in ipairs(players) do
    out.carried[#out.carried+1]=collection(entry.subject,"moving_box:"..entry.subject.local_id,entry.player:GetMovingBoxContents(),"moving_box",entry.subject.use)
  end
  -- 只给出时间关联，不宣称活动对象与保存项已存在一一对应。
  out.temporal_relation={before=old and old.context or {status="NotCaptured"},after=x,kind="ObservationScopes",object_correspondence="Unresolved",reason="no_public_saved_from_or_restored_from_token"}
  if old then for key,state in pairs(old.data) do if not nextState[key] then out.changes[#out.changes+1]={collection_key=key,kind="NoLongerObservedInCurrentMembership",previous_revision=tostring(state.revision),previous_scope=old.context.scope} end end end
  verify()
  local results=table.pack(read.Finish(out,c))
  store:Stage(x,nextState,results[2])
  return table.unpack(results,1,results.n)
end
function M.Outcome(scope,ok) store:Outcome(scope,ok) end
function M.Boundary(name) shared.Boundary(3,name) end
-- 原生保存/恢复公开回调提供真实阶段，独立复制当时容器；不借普通tick或推断逐对象身份。
function M.Event(name,first,descriptor,key)
  read.Limits({bytes=8*1024*1024,nodes=64*1024*1024,objects=16384,depth=12,queries=262144})
  local c=shared.Model.Context();local x=shared.Context()
  local out={event=name,location=x,phase_semantics="actual_save_or_pre_restore_callback;not_ordinary_post",collections={},object_correspondence="Unresolved"}
  if descriptor then
    out.descriptor={dimension=descriptor:GetDimension(),grid_index=descriptor.GridIndex,list_index=descriptor.ListIndex,construction=shared.Model.Construction(descriptor),reference=shared.Model.Reference(descriptor),source="actual_callback_argument"}
    if name:find("BACKWARDS") then out.stage=first;out.key=key end
    for _,spec in ipairs({{"entity","GetEntitiesSaveState"},{"grid","GetGridEntitiesSaveState"},{"decoration","GetDecoSaveState"}}) do
      local values,size=copyVector(descriptor[spec[2]](descriptor),spec[1],c)
      out.collections[#out.collections+1]={kind=spec[1],count=size,values=values,columns=spec[1]=="grid" and gridColumns or nil,encoding=spec[1]=="grid" and "grid_columns_v2" or "named_fields_v1",identity="event_scope_ordinal;no_native_continuity_claim"}
    end
  else out.slot=first;out.private_backup={status="Unavailable",reason="hourglass_callback_exposes_slot_only;no_public_backup_container"} end
  local j,ok,count,nodes,reason=read.Finish(out,c)
  IsaacCSObservationNative.PublishLocation(j,ok,count,nodes,reason,3,0)
end
return M
