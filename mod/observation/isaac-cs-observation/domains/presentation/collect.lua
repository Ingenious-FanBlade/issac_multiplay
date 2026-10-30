-- HUD/音频控制状态与实际像素/声卡输出分层；独立Sprite不重复Entity-owned资源。
local X=include("domains/common/extended")
local R=X.Read
local Calls=include("domains/common/calls")
local schemas=include("domains/presentation/schema");R.AddSchemas(schemas)
local M={SchemaVersion=2,sources={}}
function M.Register(namespace,name,reader)
  assert(type(namespace)=="string" and type(name)=="string" and type(reader)=="function")
  M.sources[namespace..":"..name]={namespace=namespace,name=name,read=reader}
end
function M.Unregister(namespace,name)
  local s=M.sources[namespace..":"..name]
  if s and IsaacCSObservationNative and IsaacCSObservationNative.LocationNeeded() then X.Publish(7,{event="IndependentPresentationRetired",namespace=namespace,name=name,subject=s.subject},0) end
  M.sources[namespace..":"..name]=nil
end
function M.Install()
  for _,class in ipairs({"HUD","MusicManager","SFXManager","Game"}) do
    local methods={}
    for _,s in ipairs((schemas[class] or include("domains/run/schema")[class]) or {}) do
      if (s.category=="operation" or s.category=="actual_call") and (class~="Game" or s.member:find("Show") or s.member=="ShakeScreen" or s.member:find("ColorModifier")) then methods[s.member]=true end
    end
    Calls.Class(class,methods,7)
  end
end
local function capture(limits,render)
  R.Limits(limits);local c=X.Context();local x=X.Shared.Context();local g=Game()
  local players,verify=R.PlayerSources(c)
  local out={location=x,phase=render and "Render" or "OrdinaryPost",application_frame=Isaac.GetFrameCount(),game_frame=g:GetFrameCount(),hud=R.Collect(g:GetHUD(),"HUD",c,0),item_overlay=R.Collect(ItemOverlay,"ItemOverlay",c,0),music=R.Collect(MusicManager(),"MusicManager",c,0),sound={},independent={},canonical_player_state={schema="isaac.cs.entity",scope=render and {status="NotCaptured",reason="render_is_not_authority_window"} or x.entity_scope},minimap={schema="isaac.cs.exploration",field="minimap",scope_rule="separate_source_scope;not_same_render_sample"},output_layer="public_control_and_pose;no_GPU_or_soundcard_samples"}
  local sfx=SFXManager()
  for _,args in ipairs(X.Enum("SoundEffect")) do local id=args[1]
    if sfx:IsPlaying(id) then out.sound[#out.sound+1]={id=id,playing=true,volume=sfx:GetAmbientSoundVolume(id),source="SFXManager.IsPlaying/GetVolume;one_shot_or_loop_from_event"} end
  end
  for key,s in pairs(M.sources) do
    local ok,sprite,state=pcall(s.read)
    local lifetime=x.session..":"..x.epoch..":"..x.run
    if s.lifetime~=lifetime then s.lifetime=lifetime;s.subject={space=12,session=x.session,epoch=x.epoch,local_id=IsaacCSObservationNative.RelatedSubject(12)} end
    local entry={subject=s.subject,namespace=s.namespace,name=s.name}
    if ok then entry.sprite=sprite and R.Collect(sprite,"Sprite",c,0) or R.Missing("Null","not_alive");entry.state=R.Value(state,nil,c,0)
    else R.Fail(c,"independent_reader:"..key);entry.status="ReadFailed" end
    out.independent[#out.independent+1]=entry
  end
  out.messages={main={},stacked={},players={}}
  local function message(m) return {showing=m:IsShowing(),text=m:GetMainText(),subtext=m:GetSubText(),sprite=R.Collect(m:GetSprite(),"Sprite",c,0)} end
  out.messages.main=message(g:GetHUD():GetMainMessage())
  for i=0,5 do out.messages.stacked[#out.messages.stacked+1]=message(g:GetHUD():GetStackedMessage(i)) end
  for i=0,3 do out.messages.players[#out.messages.players+1]=message(g:GetHUD():GetCoopPlayerMessage(i)) end
  verify()
  c.objects=1+#out.independent
  return R.Finish(out,c)
end
function M.Capture(limits) return Calls.Silent(capture,limits,false) end
function M.Render()
  local j,ok,count,nodes,reason=Calls.Silent(capture,{objects=16384,nodes=262144,depth=12,bytes=8*1024*1024},true)
  IsaacCSObservationNative.PublishLocation(j,ok,count,nodes,reason,7,2)
end
function M.Outcome() end
function M.Event(name,...)
  local c=X.Context();X.Publish(7,{event=name,arguments=R.Value(table.pack(...),nil,c,0),application_frame=Isaac.GetFrameCount(),semantics=name:find("PRE_") and "requested_native_call;later_callbacks_may_override" or "actual_post_callback"},0,c)
end
function M.Query(object,class,member,args) return X.Query(7,schemas,object,class,member,args) end
return M
