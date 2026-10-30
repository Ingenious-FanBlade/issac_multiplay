-- 四域共用发布、参数与只读查询；所有异常只影响对应观测，不改变原调用。
local shared=include("domains/common/continuity")
local R=shared.Read
local M={Read=R,Shared=shared}
function M.Publish(domain,payload,phase,c)
  c=c or R.Context()
  local j,ok,objects,nodes,reason=R.Finish(payload,c)
  return IsaacCSObservationNative.PublishLocation(j,ok,objects,nodes,reason,domain,phase or 0)
end
function M.Context()
  local c=R.Context()
  c.external_reference=function(object,spec) return {type="canonical_domain_reference",schema="isaac.cs."..spec.reason:gsub("canonical:",""),member=spec.id} end
  c.parameter_sets=function(object,spec)
    local name=spec.member
    if spec.owner=="Seeds" and name=="GetStageSeed" then local out={} for i=1,13 do out[#out+1]={i} end return out end
    if spec.owner=="Seeds" and name=="HasSeedEffect" then return M.Enum("SeedEffect") end
    if spec.owner=="Game" and name=="GetStateFlag" then return M.Enum("GameStateFlag") end
    if spec.owner=="HUD" and name=="GetPlayerHUD" then return {{0},{1},{2},{3}} end
    if spec.owner=="HUD" and name=="GetPlayerStreakSprite" then return {{0},{1},{2},{3}} end
    if spec.owner=="HUD" and name=="GetStackedStreakSprite" then local out={} for i=0,5 do out[#out+1]={i} end return out end
    if spec.owner=="HistoryHUD" then
      if name=="GetPlayer" then return {{0},{1}} end
      if name=="GetItems" then return {{0,false},{0,true},{1,false},{1,true}} end
      -- nil过滤器必须保留参数位置，不能用#或ipairs截断。
      if name:find("Collectible") or name:find("Trinket") then return {{[1]=0,[3]=true,n=3},{[1]=1,[3]=true,n=3}} end
    end
    return nil
  end
  c.domain_value=function(object,typ,ctx,depth)
    if typ=="HistoryHUDItems" then return true,R.Collect(object,"HistoryHUDItem",ctx,depth+1) end
    return false
  end
  return c
end
function M.Enum(name)
  local out,seen={},{}
  for k,v in pairs(_G[name] or {}) do if type(v)=="number" and v>=0 and not k:find("NUM_") and not seen[v] then out[#out+1]={v};seen[v]=true end end
  table.sort(out,function(a,b)return a[1]<b[1] end);return out
end
function M.Query(domain,schemas,object,class,member,args)
  for _,s in ipairs(schemas[class] or {}) do if s.member==member and (s.category=="query" or s.category=="parameter_query") then
    local c=M.Context();local result=R.QueryValue(object,s,args or {},c,0)
    M.Publish(domain,{class=class,member=s.id,parameters=R.Value(args,nil,c,0),result=result},1,c);return result
  end end
  return R.Missing("Unavailable","not_declared_readonly_query")
end
return M
