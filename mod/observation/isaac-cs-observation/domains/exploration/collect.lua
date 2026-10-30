-- 地图知识与本地Minimap偏好分别封存；变化边只连接已提交的实际采样scope。
local shared=include("domains/common/continuity")
local read=shared.Read
read.AddSchemas(include("domains/exploration/schema"))
local store=shared.Store(true)
local M={SchemaVersion=2}
function M.Capture(limits)
  read.Limits(limits); local c=shared.Model.Context(); local x,old=store:Begin()
  local l=Game():GetLevel(); local out={location=x,descriptors={},changes={},sampling={cache_reused=false,freshness="ReadInCurrentNativeWindow"},previous_scope=old and old.context.scope or {status="NotCaptured",reason="first_observed_or_history_gap"},world_knowledge={curses=l:GetCurses(),map_reveal=l:GetStateFlag(LevelStateFlag.STATE_MAP_EFFECT),blue_map=l:GetStateFlag(LevelStateFlag.STATE_BLUE_MAP_EFFECT),full_map=l:GetStateFlag(LevelStateFlag.STATE_FULL_MAP_EFFECT),compass=l:GetStateFlag(LevelStateFlag.STATE_COMPASS_EFFECT)},local_display=read.Collect(Minimap,"Minimap",c,0)}
  local nextState={}
  for _,entry in ipairs(shared.Descriptors(c)) do
    local d,id=entry.descriptor,entry.subject; c.objects=c.objects+1
    local state={construction_revision=id.construction_revision,display_flags=d.DisplayFlags,visited_count=d.VisitedCount,clear=d.Clear,clear_count=d.ClearCount,flags=d.Flags,room_type=d.Data.Type,hidden_by_world_flags={red_room=(d.Flags&(1<<10))~=0},knowledge="public_descriptor_flags;visibility_is_not_visit"}
    nextState[id.local_id]=state
    out.descriptors[#out.descriptors+1]={subject=id,state=state}
    local before=old and old.data[id.local_id]
    if not before or not shared.Equal(before,state) then out.changes[#out.changes+1]={subject=id,kind=before and "ObservedChange" or "FirstObserved",before=before or {status="NotCaptured"},after=state,previous_scope=old and old.context.scope} end
  end
  if old then for id,state in pairs(old.data) do if not nextState[id] then out.changes[#out.changes+1]={subject={space=6,local_id=id},kind="AbsentFromCurrentDescriptorMembership",before=state,previous_scope=old.context.scope} end end end
  local results=table.pack(read.Finish(out,c))
  store:Stage(x,nextState,results[2])
  return table.unpack(results,1,results.n)
end
function M.Outcome(scope,ok) store:Outcome(scope,ok) end
function M.Boundary(name) shared.Boundary(2,name) end
return M
