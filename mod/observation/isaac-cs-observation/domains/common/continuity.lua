-- 跨房域共用纯值事务；旧值只有最终提交后才成为历史，不保存userdata。
local read=include("domains/common/read")
local model=include("domains/level/model")
local json=require("json")
local M={}
function M.Context() return json.decode(IsaacCSObservationNative.CaptureContext()) end
function M.Equal(a,b)
  if type(a)~=type(b) then return false end
  if type(a)~="table" then return a==b end
  for k,v in pairs(a) do if not M.Equal(v,b[k]) then return false end end
  for k in pairs(b) do if a[k]==nil then return false end end
  return true
end
function M.Store(floorBound)
  local t={committed=nil,pending=nil,floor_bound=floorBound}
  function t:Begin()
    local x=M.Context(); local old=self.committed
    if old and (old.context.epoch~=x.epoch or old.context.session~=x.session or old.context.run~=x.run or self.floor_bound and old.context.floor~=x.floor) then old=nil end
    self.pending=nil
    return x,old
  end
  function t:Stage(context,data,complete)
    self.pending={context=context,data=complete and data or nil}
  end
  function t:Outcome(scope,ok)
    if self.pending and self.pending.context.scope==scope then
      if ok and self.pending.data then self.committed=self.pending else self.committed=nil end
      self.pending=nil
    end
    -- 无法提交的窗口形成显式历史空档；不能跨空档声称连续。
    if not ok then self.committed=nil end
  end
  return t
end
-- 不主动访问房间；只枚举实际存在的描述和已初始化离图槽，Online槽不读。
function M.Descriptors(c)
  local l=Game():GetLevel(); local rooms=l:GetRooms(); local out,seen={},{}
  local function add(d,index)
    if not d or not d.Data then return end
    local id=model.Reference(d,index)
    if id.local_id=="0" then read.Fail(c,"descriptor_identity_unavailable"); return end
    if not seen[id.local_id] then seen[id.local_id]=true; out[#out+1]={descriptor=d,subject=id} end
  end
  if rooms.Size<0 or rooms.Size>1024 then read.Fail(c,"descriptor_budget") end
  for i=0,math.min(rooms.Size,1024)-1 do add(rooms:Get(i)) end
  for i=-1,-20,-1 do if i~=-19 then add(l:GetRoomByIdx(i,0),i) end end
  add(l:GetCurrentRoomDesc(),l:GetCurrentRoomIndex())
  return out
end
function M.Players(c)
  local sources,unavailable=IsaacCSObservationNative.Sources(); local out,seen={},{}
  if unavailable>0 then read.Fail(c,"native_player_source_incomplete") end
  for _,source in ipairs(sources) do
    local p=source.entity:ToPlayer()
    if p then
      local id=IsaacCSObservationNative.Subject(p)
      if id.local_id=="0" or id.resolution=="Retired" then read.Fail(c,"player_identity_unavailable")
      elseif not seen[id.local_id] then seen[id.local_id]=true; out[#out+1]={player=p,subject=id} end
    end
  end
  local publicPlayers,verify=read.PlayerSources(c)
  for _,entry in ipairs(publicPlayers) do if not seen[entry.subject.local_id] then seen[entry.subject.local_id]=true;out[#out+1]=entry end end
  return out,verify
end
function M.Boundary(domain,name)
  local c=read.Context(); local x=M.Context()
  local payload={event=name,previous=x,new_context="UnknownUntilOrdinaryPost",semantics="observed_boundary;no_entity_destruction_inferred"}
  local j,ok,count,nodes,reason=read.Finish(payload,c)
  IsaacCSObservationNative.PublishLocation(j,ok,count,nodes,reason,domain,3)
end
M.Read=read; M.Model=model
return M
