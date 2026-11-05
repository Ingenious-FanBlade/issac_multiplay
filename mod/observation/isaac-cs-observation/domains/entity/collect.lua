-- Entity 主体组织；有界值复制和资源姿态与其他域共用。
local read=include("domains/common/read")
local slots=include("domains/entity/slot_variants")
local schemas=include("domains/entity/schema")
local playerApplication=include("domains/entity/player_application")
local M={SchemaVersion=2}
local native=function() return IsaacCSObservationNative end
local context,collect,value,query,finish,fail,missing,protected=read.Context,read.Collect,read.Value,read.QueryValue,read.Finish,read.Fail,read.Missing,read.Protected
local LIMIT=read.Limits()
local conversions={ {"ToPlayer","EntityPlayer"},{"ToNPC","EntityNPC"},{"ToTear","EntityTear"},{"ToProjectile","EntityProjectile"},{"ToBomb","EntityBomb"},{"ToLaser","EntityLaser"},{"ToKnife","EntityKnife"},{"ToPickup","EntityPickup"},{"ToFamiliar","EntityFamiliar"},{"ToEffect","EntityEffect"},{"ToSlot","EntitySlot"} }
local function context() return read.Context() end
-- 原生补充的普通表也消耗同一64M节点预算；不能因避开Value的图编码而漏计。
local function nativeValue(v,ctx)
  local function account(x)
    if type(x)=="userdata" then return true end -- Sprite由下方Value完整计数。
    if ctx.nodes>=LIMIT.nodes then return false end
    ctx.nodes=ctx.nodes+1
    if type(x)=="table" then for k,w in pairs(x) do if not account(k) or not account(w) then return false end end end
    return true
  end
  if not account(v) then fail(ctx,"entity_native_value_budget");return missing("Truncated","native_node_budget") end
  return v
end
local function snapshot(e,ctx,class)
  local subject=native().Subject(e)
  ctx.owner=subject
  if subject.local_id=="0" then fail(ctx,"subject_budget"); return {subject=subject,status="NotCaptured"} end
  if subject.resolution=="Retired" then return {subject=subject,status="NotApplicable",reason="retired_at_source"} end
  local out={subject=subject,common=collect(e,"Entity",ctx,0)}
  if class and class~="Entity" then out.body=collect(e,class,ctx,0)
  else
    for _,c in ipairs(conversions) do
      local ok,derived=protected(e,c[1])
      if ok and derived then class=c[2]; out.body=collect(derived,class,ctx,0); break end
    end
  end
  if class=="EntityNPC" then
    local ok,d=protected(e,"ToDelirium")
    if ok and d then out.special=collect(d,"EntityDelirium",ctx,0) end
  end
  if class=="EntityPlayer" or class=="EntityNPC" or class=="EntityTear" or class=="EntityProjectile" or class=="EntityBomb" or class=="EntityLaser" or class=="EntityKnife" or class=="EntityPickup" or class=="EntityFamiliar" or class=="EntityEffect" or class=="EntitySlot" then
    local resources=native().EntityApplicationSprites(e,class)
    out.body.application_draw=nativeValue(resources.draw or missing(resources.status,"draw_inputs_missing"),ctx)
    if class=="EntityPlayer" then
      out.body.application_player_state=nativeValue(native().PlayerApplicationState(e:ToPlayer()),ctx)
      if out.body.application_player_state.status~="Value" then fail(ctx,"player_application_state:"..out.body.application_player_state.status) end
      out.body.application_player_role=nativeValue(native().PlayerApplicationRole(e:ToPlayer()),ctx)
      if out.body.application_player_role.status~="Value" then fail(ctx,"player_application_role:"..out.body.application_player_role.status) end
      local special=native().PlayerApplicationSpecial(e:ToPlayer())
      if special.status=="Value" then special.record_sprite=special.record_sprite:GetFilename()=="" and missing("Null","native_record_anm_unloaded") or value(special.record_sprite,"Sprite",ctx,1) end
      out.body.application_player_special=nativeValue(special,ctx)
      if out.body.application_player_special.status~="Value" then fail(ctx,"player_application_special:"..out.body.application_player_special.status) end
      local equipment=native().PlayerApplicationEquipment(e:ToPlayer())
      if equipment.status=="Value" then
        -- 原生补充只在本窗口借用；最终表只保留规范引用及精确姿态，空边逐名保留。
        for _,weapon in ipairs(equipment.weapons) do if weapon.present then
          local links={}
          for _,name in ipairs(weapon.link_names) do links[name]=value(weapon.links[name],"Entity",ctx,1) end
          weapon.links=links;weapon.link_names=nil
          for name,sprite in pairs(weapon.sprites) do weapon.sprites[name]=sprite:GetFilename()=="" and missing("Null","native_weapon_anm_unloaded") or value(sprite,"Sprite",ctx,1) end
        end end
      else fail(ctx,"player_application_equipment:"..equipment.status) end
      out.body.application_player_equipment=nativeValue(equipment,ctx)
      out.body.application_player_draw=nativeValue(resources.player_draw or missing(resources.status,"player_draw_inputs_missing"),ctx)
      out.body.application_player_resources={version=1,status=resources.status}
      if resources.status=="Value" then
        for _,name in ipairs({"inline_1","inline_2","inline_3","inline_4","inline_5","inline_6","inline_7","inline_8","inline_9","blood_gush","held","inline_12"}) do
          local sprite=resources.player_sprites[name]
          out.body.application_player_resources[name]=sprite:GetFilename()=="" and missing("Null","native_player_anm_unloaded") or value(sprite,"Sprite",ctx,1)
        end
      else fail(ctx,"player_application_resources:"..resources.status) end
    end
    out.body.application_links={version=1,status=resources.status,links={}}
    if resources.status=="Value" then
      -- C++返回的固定声明列完整保留；nil也必须带名写出Null，不能被Lua pairs省略。
      for _,name in ipairs(resources.link_names) do out.body.application_links.links[name]=value(resources.links[name],"Entity",ctx,1) end
    else fail(ctx,"entity_application_links:"..resources.status) end
    if class=="EntityBomb" then
      out.body.application_runtime={version=1,status=resources.status,values={}}
      if resources.status=="Value" then out.body.application_runtime.values["NativeBomb.CostumeDrawEnabled"]=resources.bomb_costume_draw_enabled end
    end
    if class=="EntityNPC" then
      out.body.application_runtime={version=1,status=resources.status,private_reference_count=resources.npc_private_reference_count,owned_actor_present=resources.npc_owned_actor_present}
      local composite=nativeValue(native().EntityApplicationNpc(e),ctx)
      out.body.application_npc={version=1,status=composite.status,reference_count=composite.reference_count,path=composite.path,secondary_path=composite.secondary_path}
      if composite.status=="Value" then
        for i=0,composite.reference_count-1 do out.body.application_links.links["NativeNPC.Composite:"..tostring(i)]=value(composite.references[i+1],"Entity",ctx,1) end
      else fail(ctx,"npc_application_composite:"..composite.status) end
      out.body.application_resources={version=1,status=resources.status}
      if resources.status=="Value" then
        for _,key in ipairs({"npc_overlay","npc_effect"}) do
          local sprite=resources[key]
          out.body.application_resources[key]=sprite:GetFilename()=="" and missing("Null","native_entity_anm_unloaded") or value(sprite,"Sprite",ctx,1)
        end
      else fail(ctx,"npc_application_resources:"..resources.status) end
    end
    if class=="EntityPickup" then
      out.body.application_resources={version=1,status=resources.status}
      if resources.status=="Value" then
        local sprite=resources.pickup_alternate
        out.body.application_resources.pickup_alternate=sprite:GetFilename()=="" and missing("Null","native_entity_anm_unloaded") or value(sprite,"Sprite",ctx,1)
      else fail(ctx,"pickup_application_resources:"..resources.status) end
    end
    if class=="EntityKnife" then
      out.body.application_resources={version=1,status=resources.status}
      if resources.status=="Value" then
        local sprite=resources.knife_dead_eye
        out.body.application_resources.knife_dead_eye=(not sprite or sprite:GetFilename()=="") and missing("Null","native_shared_anm_unloaded") or value(sprite,"Sprite",ctx,1)
      else fail(ctx,"knife_application_resources:"..resources.status) end
    end
    if class=="EntityLaser" or class=="EntityKnife" or class=="EntityPickup" or class=="EntityFamiliar" or class=="EntityEffect" then
      out.body.application_runtime=resources.runtime or {version=2,status=resources.status}
      if class=="EntityFamiliar" or class=="EntityEffect" then
        local owned=nativeValue(native().EntityApplicationOwned(e,class=="EntityFamiliar" and 8 or 9),ctx)
        if owned.status=="Value" then
          for i,sprite in ipairs(owned.sprites) do owned.sprites[i]=sprite:GetFilename()=="" and missing("Null","native_owned_anm_unloaded") or value(sprite,"Sprite",ctx,1) end
        else fail(ctx,"entity_owned_presentation:"..owned.status) end
        out.body.application_owned=owned
      end
    end
  end
  out.class=class or "UnknownEntity"
  if class=="EntityPlayer" then out.body.application_player=playerApplication.Describe(out.body,out.common,subject) end
  if class=="EntityLaser" then
    -- 原生补充返回自有纯值几何；不通过Lua table图别名合并真实四个容器。
    out.body.application_geometry=nativeValue(native().EntityApplicationGeometry(e:ToLaser()),ctx)
    if out.body.application_geometry.status~="Value" then fail(ctx,"laser_geometry:"..out.body.application_geometry.status) end
    local presentation=native().LaserPresentation(e:ToLaser())
    for _,key in ipairs({"sample_mode_raw","start_offset_mode_opaque32"}) do
      out.body.fields["EntityLaser."..key]=presentation.status=="Value" and value(presentation[key],"int",ctx,1) or missing(presentation.status,"laser_initialized_owner_required")
    end
    out.native_source={contract="entity-laser-presentation-v1",target="REPENTOGON-1.1.3/J273",readiness="observed-completed-creation-owner",sample="signed-byte;-1-not-yet-resolved;not-public-bool",opaque="J273-draw-4F0-signed<=0;semantic-UNKNOWN"}
    if presentation.status~="Value" then fail(ctx,"laser_presentation:"..presentation.status) end
  end
  if class=="EntitySlot" then out.variant_semantics=slots[e.Variant] or missing("Unavailable","unmapped_slot_variant"); if not slots[e.Variant] then fail(ctx,"unmapped_slot_variant") end end
  if out.class=="UnknownEntity" then
    local supplement=native().Supplement(e)
    if supplement.class~="UnknownEntity" then
      out.class=supplement.class; out.body={type=supplement.class,status=supplement.status,fields={}}
      -- 原生Text/Dummy也共用正式具名关系和普通窗口draw输入，不伪装成NPC。
      local resources=native().EntityApplicationSprites(e,supplement.class)
      out.body.application_draw=nativeValue(resources.draw or missing(resources.status,"draw_inputs_missing"),ctx)
      out.body.application_links={version=1,status=resources.status,links={}}
      if resources.status=="Value" then for _,name in ipairs(resources.link_names) do out.body.application_links.links[name]=value(resources.links[name],"Entity",ctx,1) end
      else fail(ctx,"native_application_links:"..resources.status) end
      if supplement.class=="NativeText" then
        out.body.application_text_bytes={version=1,status=supplement.status,hex=supplement.text_hex}
        for _,key in ipairs({"counter","duration","text"}) do
          out.body.fields["NativeText."..key]=supplement[key]~=nil and value(supplement[key],nil,ctx,1) or missing(supplement.status,"native_readiness_or_copy")
        end
        local color={type="native_kcolor20",status=supplement.draw_color.status}
        if color.status=="Value" then
          for _,k in ipairs({"R","G","B","A"}) do color[k]=value(supplement.draw_color[k],"float",ctx,1) end
          color.opaque32=value(supplement.draw_color.opaque32,"int",ctx,1)
        end
        out.body.fields["NativeText.draw_color"]=color
        out.body.fields["NativeText.draw_phase"]={type="owner_qualified_domain_reference",schema="isaac.cs.entity",owner=subject,contract="native-text-render-v1",capture_kind="Render",required_state="same Key/shape/counter/text",scope_rule="independent-render-entry-inputs;not-an-ordinary-phase"}
        if supplement.draw_color.status~="Value" then fail(ctx,"native_text_color:"..supplement.draw_color.status) end
        out.native_source={contract="entity-native-presentation-v1",target="REPENTOGON-1.1.3/J273",color="KColor20;observed-creation-owner;opaque32-not-guessed",text="bounded-native-buffer;real-current-use-writer-witness",phase="independent-native-text-render-v1;Manager-frame-not-Authority-tick"}
      end
      if supplement.status~="Value" then fail(ctx,"native_supplement:"..supplement.status) end
    else fail(ctx,"unknown_entity_type:"..tostring(e.Type)) end
  end
  return out
end

function M.Capture(limits)
  if limits then
    -- 同时采用具名有限查询预算；不能只更新字节/节点而沿用上一个域的旧queries上限。
    read.Limits(limits)
  end
  local ctx=context(); local list=Isaac.GetRoomEntities(); local subjects={}; local seen={}; local enumerationComplete=true
  -- 先登记全部主体，再读取字段／解析关系；Lua 枚举缺席不能推定销毁。
  local nativeSources,unavailable=native().Sources()
  if unavailable>0 then fail(ctx,"native_source_unavailable"); enumerationComplete=false end
  for _,entry in ipairs(nativeSources) do list[#list+1]=entry.entity end
  local publicPlayers,verifyPlayers=read.PlayerSources(ctx)
  for _,entry in ipairs(publicPlayers) do list[#list+1]=entry.player end
  for _,e in ipairs(list) do
    local id=native().Subject(e)
    if id.local_id=="0" then fail(ctx,"subject_registry_budget"); enumerationComplete=false; subjects[#subjects+1]=e
    elseif not seen[id.local_id] then seen[id.local_id]=true; subjects[#subjects+1]=e end
  end
  -- 普通窗口已复制主/辅助Sprite的精确phase、图层和两组Laser当前采样；
  -- 独立Render节流样本只描述自己的scope，绝不拼进此原子窗口。
  local out={application_source={version=1,contract="entity-source-v1",target="REPENTOGON-1.1.3/J273",pose_scope="same-ordinary-window",required_contracts={"resource-pose-v1","entity-classification-v1","entity-auxiliary-sprites-v1","entity-native-presentation-v1","entity-class-application-v1","entity-class-application-v2","entity-class-application-v3"},auxiliary="NPC:overlay/effect;Tear:dead-eye/effect/halo;Bomb:costume0..4;Knife:shared-dead-eye;Pickup:price/alternate;Slot:prize;unloaded=Null",class_application="same-use-owned-links;NPC-private-owner-availability;Bomb-costume-enabled;Sprite-champion-color/playing/shader-paths",laser="same-use-four-owned-vectors-and-scalars-v1;public-sample-aliases-checked;no-CalculateSamples",independent="Render/Lifecycle/ActualCall/Query-have-own-scope;render-stride30-is-not-target-pose"},objects={},membership={source="Isaac.GetRoomEntities+native_membership",count=#subjects,native_count=#nativeSources,unavailable_native=unavailable,complete=enumerationComplete}}
  for i,e in ipairs(subjects) do
    if i>LIMIT.objects then fail(ctx,"object_budget"); out.membership.complete=false; break end
    ctx.objects=ctx.objects+1; out.objects[#out.objects+1]=snapshot(e,ctx)
  end
  verifyPlayers()
  return finish(out,ctx)
end
function M.Event(name,class,object,...)
  local ctx=context(); ctx.objects=1
  -- Remove 回调只取真实事件主体，避免解引用已经进入销毁阶段的派生关联。
  local payload={event=name,subject=native().Subject(object)}
  payload.arguments=value({...},nil,ctx,0)
  if name~="MC_POST_ENTITY_REMOVE" then payload.object=snapshot(object,ctx,class) end
  local json,complete,objects,nodes,reason=finish(payload,ctx)
  native().PublishEvent(json,complete,objects,nodes,reason)
end
local renderEpoch,renderCounters=nil,{}
-- render 的姿态单独封存，每个主体每30次真实回调采样；不把渲染帧当作 Authority tick。
function M.Render(object)
  local subject=native().Subject(object)
  if renderEpoch~=subject.epoch then renderEpoch=subject.epoch; renderCounters={} end
  local key=subject.local_id
  renderCounters[key]=(renderCounters[key] or 0)+1
  if renderCounters[key]%30~=0 then return end
  local ctx=context(); ctx.objects=1
  local json,complete,objects,nodes,reason=finish({subject=subject,source={contract="entity-render-sample-v1",stride=30,callback_ordinal=renderCounters[key],semantics="independent-render-scope;never-substitute-ordinary-target-pose"},sprite=collect(object:GetSprite(),"Sprite",ctx,0)},ctx)
  native().PublishRender(json,complete,objects,nodes,reason)
end
-- 显式派生查询同样受 schema 白名单与预算约束，不能用此入口执行玩法动作。
function M.Query(owner,class,member,args,object)
  object=object or owner
  for _,spec in ipairs(schemas[class] or {}) do
    if spec.member==member and (spec.category=="query" or spec.category=="parameter_query" or spec.category=="derived_query") then
      local ctx=context(); ctx.objects=1
      local result=query(object,spec,args or {},ctx,0)
      local json,complete,objects,nodes,reason=finish({subject=native().Subject(owner),member=spec.id,parameters=value(args or {},nil,ctx,0),result=result},ctx)
      native().PublishQuery(json,complete,objects,nodes,reason)
      return result
    end
  end
  return missing("Unavailable","not_a_declared_readonly_query")
end
return M
