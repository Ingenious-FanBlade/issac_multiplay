if IsaacCSObservationCalls then return IsaacCSObservationCalls end
-- 唯一Lua调用观察owner：常驻wrapper、停用透传，参数与返回值（含nil洞）保持原样。
local X=include("domains/common/extended")
local R=X.Read
local M={silent=0,sequence=0,parent=0,installed={},failures={}}
function M.Enabled() local n=IsaacCSObservationNative;return M.silent==0 and n and n.LocationNeeded() end
function M.Silent(fn,...)
  M.silent=M.silent+1;local r=table.pack(pcall(fn,...));M.silent=M.silent-1
  if not r[1] then error(r[2],0) end;return table.unpack(r,2,r.n)
end
function M.Wrap(fn,domain,name,before,after,static)
  return function(...)
    if not M.Enabled() then return fn(...) end
    local args=table.pack(...);M.sequence=M.sequence+1;local id,parent=M.sequence,M.parent;M.parent=id
    local ok,prior=pcall(M.Silent,function() return before and before(args) end)
    local results=table.pack(pcall(fn,table.unpack(args,1,args.n)));M.parent=parent
    -- 观测错误不改变玩法函数的成功、异常、nil洞或多返回值。
    local observed,err=pcall(M.Silent,function()
      local c=X.Context();local a={n=args.n-(static and 0 or 1)}
      for i=static and 1 or 2,args.n do a[i-(static and 0 or 1)]=args[i] end
      local returns={n=results.n-1};for i=2,results.n do returns[i-1]=results[i] end
      local details=after and after(args,results,ok and prior or nil,c) or nil
      X.Publish(domain,{call=id,parent_call=parent,member=name,arguments=R.Value(a,nil,c,0),returns=R.Value(returns,nil,c,0),success=results[1],before=ok and prior or {status="ReadFailed"},details=details,scope_semantics="real_Lua_call;native_internal_consumers_not_implied"},4,c)
    end)
    if not observed then M.failures[name]=type(err)=="string" and err:sub(1,128) or "observation_failed" end
    if not results[1] then error(results[2],0) end
    return table.unpack(results,2,results.n)
  end
end
function M.Class(class,methods,domain,before,after)
  local global=_G[class];local mt=global and getmetatable(global);mt=mt and mt.__class
  if not mt and (class=="Game" or class=="MusicManager" or class=="SFXManager") then
    local ok,object=pcall(global);if ok then mt=getmetatable(object) end
  end
  if type(mt)~="table" or type(mt.__index)~="function" and type(mt.__index)~="table" then M.failures[class]="class_metatable_unavailable";return end
  local old=mt.__index;local wrappers,added={},{}
  M.installed[class]=M.installed[class] or {}
  for name in pairs(methods) do if not M.installed[class][name] then added[name]=true;M.installed[class][name]=true end end
  -- 在真实对象首次查找时取得原分派结果，保留REPENTOGON及已有Mod的参数wrapper语义。
  rawset(mt,"__index",function(self,k)
    local fn
    if type(old)=="function" then fn=old(self,k) else fn=old[k] end
    if not added[k] or type(fn)~="function" then return fn end
    if not wrappers[k] then wrappers[k]=M.Wrap(fn,domain,class.."."..k,before and function(a)return before(k,a) end,after and function(a,r,b,c)return after(k,a,r,b,c) end) end
    return wrappers[k]
  end)
end
function M.Global(tableObject,name,domain)
  if type(tableObject[name])=="function" then tableObject[name]=M.Wrap(tableObject[name],domain,name,nil,nil,true) end
end
IsaacCSObservationCalls=M
return M
