-- 控制关系和跨房连续记录；本体字段唯一来源仍是同窗口Entity规范scope。
local shared=include("domains/common/continuity")
local read=shared.Read
read.AddSchemas(include("domains/player/schema"))
local store=shared.Store(false)
local M={SchemaVersion=2}
local function ref(p) return p and IsaacCSObservationNative.Reference(p) or {resolution="Null"} end
function M.Capture(limits)
  read.Limits(limits); local c=shared.Model.Context(); local x,old=store:Begin()
  local out={location=x,bodies={},control_groups={},changes={},backup_slots={},sampling={cache_reused=false,freshness="ReadInCurrentNativeWindow"},previous_scope=old and old.context.scope or {status="NotCaptured"},resource_semantics="per_body_canonical_values;no_implicit_copy_or_shared_resource_inference"}
  out.application_source={version=2,contract="player-source-v2",final_state="player-final-state-v1",equipment_state="player-equipment-state-v1",role_state="player-role-state-v1",target="REPENTOGON-1.1.3/J273",entity_scope=x.entity_scope,pose_scope="same-ordinary-window"}
  out.roster={};out.membership={complete=true,source="qualified-native+public-roots+relationship-closure",count=0}
  local nextState,groups,memberWitness={},{},{}
  local players,verify=shared.Players(c); local roster={}
  for i,p in ipairs(PlayerManager.GetPlayers()) do local r=ref(p);out.roster[#out.roster+1]={index=i-1,body=r}; if r.local_id then roster[r.local_id]=true end end
  for _,entry in ipairs(players) do
    local p,id=entry.player,entry.subject; c.objects=c.objects+1
    local relation=id.control_relation;local membership=id.control_group_membership
    local state={subject=id,player_type=p:GetPlayerType(),dead=p:IsDead(),is_sub_player=p:IsSubPlayer(),in_public_roster=roster[id.local_id] or false,membership=membership or {resolution="Unresolved",reason="no_qualified_cached_topology_membership"},control=relation or {resolution="Unresolved",reason="no_current_exact_pair_anchor"},controller_index=p.ControllerIndex,main_twin=ref(p:GetMainTwin()),other_twin=ref(p:GetOtherTwin()),sub_player=ref(p:GetSubPlayer()),flipped_form=ref(p:GetFlippedForm()),parent=ref(p.Parent),child=ref(p.Child)}
    local before=old and old.data[id.local_id]
    local nativeMembership=IsaacCSObservationNative.PlayerApplicationMembership(p)
    if nativeMembership.status~="Value" then read.Fail(c,"player_native_membership_unavailable") end
    memberWitness[#memberWitness+1]={player=p,value=nativeMembership}
    local body={subject=id,relations=state,native_membership=nativeMembership,canonical_state={schema="isaac.cs.entity",scope=x.entity_scope,subject=id,source_scope=x.entity_scope,validity="same_committed_native_window;target_field_coverage_applies"},history=read.Collect(p:GetHistory(),"History",c,0),continuity={previous_scope=before and old.context.scope,previous_location=before and old.context,kind=before and before.subject.use==id.use and "ObservedSameEntityUse" or "FirstObserved",cross_room=before and old.context.room_instance~=x.room_instance or false}}
    if x.entity_scope=="0" then read.Fail(c,"canonical_entity_not_scheduled") end
    out.bodies[#out.bodies+1]=body; nextState[id.local_id]=state
    if not before or not shared.Equal(before,state) then out.changes[#out.changes+1]={subject=id,kind=before and "RelationshipOrLifeStateChanged" or "FirstObserved",before=before or {status="NotCaptured"},after=state} end
    local group=relation or membership
    if group and group.control_group_local_id and group.participant_local_id then
      local relation=group;local key=relation.control_group_local_id
      if not groups[key] then groups[key]={participant={space=9,local_id=relation.participant_local_id},subject={space=10,local_id=key},ordinal=relation.ordinal,control_session=relation.control_session,binding_revision=relation.binding_revision,bodies={},memberships={},device={status="KnownCurrent",basis="current_exact_pair_anchor",lifetime="control_session_and_binding_revision",role=relation.ordinal==0 and "HostPhysicalInput" or "OwnedVirtualDevice"}}; out.control_groups[#out.control_groups+1]=groups[key] end
      if id.control_relation then groups[key].bodies[#groups[key].bodies+1]=id end
      groups[key].memberships[#groups[key].memberships+1]={subject=id,body_role=relation.body_role,active=relation.active,can_consume=relation.can_consume}
    end
  end
  out.membership.count=#out.bodies
  -- 四个备份槽只建立原生身体引用；不把非活动备份当当前受控身体，也不绕过Entity读取资格。
  for i=0,3 do out.backup_slots[#out.backup_slots+1]={index=i,body=ref(PlayerManager.GetEsauJrState(i)),source="PlayerManager.GetEsauJrState"} end
  if old then for id,state in pairs(old.data) do if not nextState[id] then out.changes[#out.changes+1]={subject=state.subject,kind="AbsentFromQualifiedSources",reason="not_destruction;binding_or_readiness_or_membership_invalidated",previous_scope=old.context.scope} end end end
  verify()
  -- 同窗口结束重核集合；中途摘表/发布不能留下可删除或可渲染的旧资格。
  for _,w in ipairs(memberWitness) do if not shared.Equal(w.value,IsaacCSObservationNative.PlayerApplicationMembership(w.player)) then read.Fail(c,"player_native_membership_changed") end end
  -- 根/关系重核失败也撤销成员完整性；不能让旧的true成为删除依据。
  out.membership.complete=c.complete
  local results=table.pack(read.Finish(out,c))
  store:Stage(x,nextState,results[2])
  return table.unpack(results,1,results.n)
end
function M.Outcome(scope,ok) store:Outcome(scope,ok) end
function M.Boundary(name) shared.Boundary(4,name) end
function M.Event(name,p,...)
  local c=read.Context(); local x=shared.Context()
  local j,ok,count,nodes,reason=read.Finish({event=name,location=x,subject=ref(p),arguments=read.Value({...},nil,c,0),phase_semantics="actual_callback;ordinary_state_is_separate"},c)
  IsaacCSObservationNative.PublishLocation(j,ok,count,nodes,reason,4,0)
end
-- PlayerManager的非随机参数查询按请求复制；不轮询任意ID空间。
function M.Query(class,member,args,object)
  local schema=include("domains/player/schema")
  for _,spec in ipairs(schema[class] or {}) do
    if spec.member==member and (spec.category=="query" or spec.category=="parameter_query") then
      local c=shared.Model.Context(); local result=read.QueryValue(object,spec,args or {},c,0)
      local j,ok,count,nodes,reason=read.Finish({owner_class=class,member=spec.id,parameters=read.Value(args or {},nil,c,0),result=result},c)
      IsaacCSObservationNative.PublishLocation(j,ok,count,nodes,reason,4,1); return result
    end
  end
  return read.Missing("Unavailable","not_declared_readonly_query")
end
return M
