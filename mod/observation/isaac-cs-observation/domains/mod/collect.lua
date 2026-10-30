-- 具名Mod适配注册：只读reader、schema、阶段和寿命必须声明；不遍历_G或创建Entity数据。
local X=include("domains/common/extended")
local R=X.Read
local Graph=include("domains/mod/graph")
local M={SchemaVersion=2,adapters={},identities=setmetatable({},{__mode="k"})}
local epoch
function M.Register(spec)
  assert(type(spec)=="table" and type(spec.namespace)=="string" and type(spec.version)=="string" and type(spec.schema)=="string" and type(spec.read)=="function" and type(spec.lifetime)=="string" and type(spec.phases)=="table","complete_readonly_adapter_declaration_required")
  assert(not M.adapters[spec.namespace],"duplicate_namespace")
  M.adapters[spec.namespace]=spec
end
function M.Unregister(namespace) M.adapters[namespace]=nil end
function M.Subject(object,kind,lifetime)
  local x=X.Shared.Context();local k=x.session..":"..x.epoch..":"..x.run
  if k~=epoch then M.identities=setmetatable({},{__mode="k"});epoch=k end
  local id=M.identities[object]
  if not id then id={space=14,session=x.session,epoch=x.epoch,local_id=IsaacCSObservationNative.RelatedSubject(14),kind=kind,lifetime=lifetime,origin="FirstObserved"};M.identities[object]=id end
  return id
end
function M.Reference(v)
  local mt=getmetatable(v);local name=mt and mt.__type
  if name=="Vector" or name=="Color" or name=="KColor" then return R.Value(v,name,X.Context(),0) end
  if name=="RNG" then return IsaacCSRandomObservation.State(v) end
  if type(name)=="string" and name:find("^Entity") then return IsaacCSObservationNative.Reference(v) end
  if type(name)=="string" and name:find("^GridEntity") then return IsaacCSObservationNative.GridReference(v) end
end
function M.Capture(limits)
  R.Limits(limits);local c=X.Context();local x=X.Shared.Context()
  local out={location=x,adapters={},unadapted={status="Unavailable",reason="only_declared_namespaces_supported;private_closures_not_introspected"}}
  for namespace,spec in pairs(M.adapters) do
    local entry={namespace=namespace,version=spec.version,schema=spec.schema,lifetime=spec.lifetime,phases=spec.phases,resources=spec.resources or {},unexposed=spec.unexposed or {}}
    if spec.phases.OrdinaryPost then
      local ok,data=pcall(spec.read,x,c)
      if ok then entry.state=Graph.Copy(data,c,spec.budget,spec.reference or M.Reference)
      else R.Fail(c,"adapter:"..namespace);entry.state={status="ReadFailed",reason=type(data)=="string" and data:gsub("0x%x+","<local>"):sub(1,256) or "adapter_error"} end
    else entry.state={status="NotScheduled",reason="phase_not_declared"} end
    out.adapters[#out.adapters+1]=entry;c.objects=c.objects+1
  end
  return R.Finish(out,c)
end
function M.Outcome(scope,ok)
  for _,spec in pairs(M.adapters) do if spec.outcome then spec.outcome(scope,ok) end end
end
function M.Event(namespace,name,data,phase)
  if not M.adapters[namespace] then return false end
  local c=X.Context();return X.Publish(8,{namespace=namespace,event=name,data=Graph.Copy(data,c,M.adapters[namespace].budget,M.Reference)},phase or 0,c)
end
function M.Boundary(name) X.Shared.Boundary(8,name) end
M.Graph=Graph
return M
