-- Mod有界对象图：next/rawget不调用__pairs/__index，循环/共享节点/稀疏键不丢失。
local X=include("domains/common/extended")
local R=X.Read
local M={}
function M.Copy(root,c,budget,reference)
  budget=budget or {};local seen,nodes,entries,bytes={},0,0,0
  local maxNodes,maxEntries,maxBytes,maxDepth=budget.nodes or 4096,budget.entries or 16384,budget.bytes or 1048576,budget.depth or 12
  local function absent(status,reason) R.Fail(c,"mod_graph:"..reason);return {status=status,reason=reason} end
  local copy
  copy=function(v,depth)
    c.nodes=c.nodes+1
    if c.nodes>R.Limits().nodes or depth>maxDepth or entries>=maxEntries or bytes>maxBytes then return absent("Truncated","budget") end
    local t=type(v)
    if t=="nil" then return {status="Null"} end
    if t=="string" then bytes=bytes+#v;if bytes>maxBytes then return absent("Truncated","byte_budget") end;return R.Value(v,nil,c,0) end
    if t=="number" or t=="boolean" then return R.Value(v,nil,c,0) end
    if t=="userdata" then
      if reference then local ok,ref=pcall(reference,v);if ok and ref then return ref end end
      return absent("Unavailable","unregistered_userdata")
    end
    if t~="table" then return absent("Unavailable",t) end
    if seen[v] then return {type="graph_ref",node=seen[v]} end
    if nodes>=maxNodes then return absent("Truncated","node_budget") end
    nodes=nodes+1;seen[v]=nodes;local out={type="table",node=nodes,entries={}}
    local key=nil
    while true do
      local k,child=next(v,key);if k==nil then break end;key=k
      if entries>=maxEntries or bytes>maxBytes or c.nodes>=R.Limits().nodes then out.truncation=absent("Truncated","membership_budget");break end
      entries=entries+1
      out.entries[#out.entries+1]={key=copy(k,depth+1),value=copy(child,depth+1)}
    end
    if #out.entries==0 then out.status="Empty" end
    return out
  end
  local out=copy(root,0)
  return {root=out,usage={nodes=nodes,entries=entries,bytes=bytes},budget={nodes=maxNodes,entries=maxEntries,bytes=maxBytes,depth=maxDepth}}
end
return M
