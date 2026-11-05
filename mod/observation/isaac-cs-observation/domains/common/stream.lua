-- tx21固定分层来源：在Lua编码前按具名字段路由，不扫描完整JSON再裁剪热字段。
if IsaacCSStreamObservation then return IsaacCSStreamObservation end
local M={}
local policy=include("domains/common/stream_policy")
local schemas=include("domains/entity/schema")
local specifications={}
for _,members in pairs(schemas) do for _,spec in ipairs(members) do specifications[spec.id]=spec end end
local names={Context=true,Structure=true,Resource=true,WorldDynamic=true,PlayerFast=true}
local function scope(id,body)
  local p=policy[id]
  if not p then return "Context" end
  if p=="SubjectDynamic" then return body and "PlayerFast" or "WorldDynamic" end
  return names[p] and p or "Context"
end
-- 快速身体只开放当前公开标量/Vector/Color和当前Sprite姿态；关系、容器和按参查询由普通联合源负责。
function M.Fast(spec)
  if scope(spec.id,true)~="PlayerFast" then return false end
  if spec.category~="field" and spec.category~="query" then return false end
  local typ=(spec.returns or ""):gsub("%?","")
  return typ=="int" or typ=="float" or typ=="number" or typ=="boolean" or typ=="bool" or
    typ=="Vector" or typ=="Color" or typ=="KColor" or typ=="BitSet128" or typ=="TearFlags" or
    spec.id=="Entity.GetSprite" or (spec.owner=="Sprite" or spec.owner=="LayerState") and
      (typ=="LayerState" or typ=="BlendMode" or typ=="LayerStates[]" or typ=="string")
end
local nativeScopes={
  application_links="Structure",application_player="Structure",application_player_role="Structure",
  application_player_equipment="Structure",application_player_resources="Resource",application_resources="Resource",
  application_owned="WorldDynamic",application_geometry="WorldDynamic",application_runtime="WorldDynamic",
  application_player_state="WorldDynamic",application_player_special="WorldDynamic",application_draw="WorldDynamic",
  application_player_draw="WorldDynamic",application_npc="WorldDynamic",application_text_bytes="WorldDynamic",
  application_player_equipment_dynamic="WorldDynamic",
  pose_runtime="WorldDynamic",resource="Resource",resource_generation="Resource",shaders="Resource",application_source="Context",native_source="Context",
  presentation_runtime="WorldDynamic",map_application="WorldDynamic",backdrop_sprites="WorldDynamic",
  vein_sprites="WorldDynamic",map_sprites="WorldDynamic",rail_sprite="WorldDynamic",backdrop_wall_trace="Structure",
  subject="Structure",membership="Structure",memberships="Structure",class="Structure",casts="Structure",
  location="Context",descriptor="Structure",seeds="Structure",config="Resource",override_config="Resource",stage_config="Resource",
  doors="WorldDynamic",entities="Structure",grids="Structure",lifted_descriptors="Structure",
  control_groups="Structure",roster="Structure",backup_slots="Structure",relations="Structure",native_membership="Structure"
}
local metadata={coverage=true,sampling=true,previous_scope=true,sample_revision=true,source_scope=true,changes=true,continuity=true,canonical_state=true}
-- 只分割有字段合同的树；无公开细分合同的私有值保留在普通联合范围，绝不猜成身体快写资格。
local function split(value,fallback,body,domain)
  if type(value)~="table" then return {[fallback]=value} end
  if next(value)==nil then return {[fallback]=value} end
  if value.type=="reference" or value.resolution or value.status and value.status~="Value" and not value.fields then return {[fallback]=value} end
  if value.type=="graph_ref" then return {[fallback]=value} end
  if value.type=="table" and value.entries then
    local groups={}
    for _,entry in ipairs(value.entries) do
      for layer,piece in pairs(split(entry.value,fallback,body,domain)) do
        groups[layer]=groups[layer] or {type="table",node=value.node,entries={}}
        groups[layer].entries[#groups[layer].entries+1]={key=entry.key,value=piece}
      end
    end
    return groups
  end
  local out={}
  local function add(layer,key,v) out[layer]=out[layer] or {};out[layer][key]=v end
  for key,v in pairs(value) do
    if not metadata[key] then
      if key=="fields" then
        for id,field in pairs(v) do
          local layer=scope(id,body)
          -- 独立PlayerFast仅来自更新见证；普通样本中的身体动态保留在联合后态。
          if body and layer=="PlayerFast" then layer="WorldDynamic" end
          local fast=body and (value.type=="Entity" or value.type=="EntityPlayer") and specifications[id] and M.Fast(specifications[id])
          for group,piece in pairs(split(field,layer,body,domain)) do
            if not fast or group=="Resource" or group=="Structure" or group=="Context" then
              out[group]=out[group] or {};out[group].fields=out[group].fields or {};out[group].fields[id]=piece
            end
          end
        end
      elseif key=="type" and type(v)=="string" or key=="version" then
        -- 下面为每个实际存在的分支补schema身份，不把类型标签当独立状态。
      else
        local layer=nativeScopes[key] or fallback
        local state=v
        if key=="location" and type(v)=="table" then
          -- 取得时刻/源scope属于Publication.validity；楼层和离房内容不因当前HUD/采样scope变号而重发。
          state={};for _,k in ipairs({"session","epoch","run","floor","dimension","descriptor","room_instance"}) do state[k]=v[k] end
        end
        local pieces=split(state,layer,body,domain)
        for group,piece in pairs(pieces) do add(group,key,piece) end
      end
    end
  end
  for _,v in pairs(out) do if value.type then v.type=value.type end;if value.version then v.version=value.version end end
  return out
end
-- 拆范围后每条record仍是一张闭合的纯值图；从源中补入真实定义，保留环，不借别的range同号节点。
local function graphs(payload,limit)
  local definitions,work={},0
  local function scan(v)
    if type(v)~="table" then return end
    work=work+1;if work>limit then error("stream_graph_budget") end
    if v.type=="table" and v.node then
      if definitions[v.node] and definitions[v.node]~=v then error("stream_graph_conflict") end
      definitions[v.node]=v
    end
    for _,child in pairs(v) do scan(child) end
  end
  scan(payload)
  local nextId=0
  return function(root)
    local localDefs,ids={},{}
    local function locals(v)
      if type(v)~="table" then return end
      if v.type=="table" and v.node then localDefs[v.node]=true end
      for _,child in pairs(v) do locals(child) end
    end
    locals(root)
    local function id(old) if not ids[old] then nextId=nextId+1;ids[old]=nextId end;return ids[old] end
    local clone
    clone=function(v,depth)
      if type(v)~="table" then return v end
      work=work+1;if work>limit or depth>64 then error("stream_graph_budget") end
      if v.type=="graph_ref" and not localDefs[v.node] then
        local definition=definitions[v.node];if not definition then error("stream_graph_missing") end
        localDefs[v.node]=true;return clone(definition,depth+1)
      end
      local copy,keys={},{};for k in pairs(v) do keys[#keys+1]=k end
      table.sort(keys,function(a,b)return type(a)==type(b) and tostring(a)<tostring(b) or type(a)<type(b)end)
      for _,k in ipairs(keys) do copy[k]=k=="node" and (v.type=="table" or v.type=="graph_ref") and id(v[k]) or clone(v[k],depth+1) end
      return copy
    end
    return clone(root,0)
  end
end
function M.Partition(payload,domain,nodeLimit)
  local out={contract="stream-source-v1",context={},structure={},resource={},world={},bodies={},members={}}
  local slots={Context="context",Structure="structure",Resource="resource",WorldDynamic="world",PlayerFast="world"}
  if payload.objects then
    for _,object in ipairs(payload.objects) do
      local player=object.class=="EntityPlayer"
      out.members[#out.members+1]={subject=object.subject,category=player and "Player" or object.class or "Unknown",use=object.subject.use,shape=object.subject.revision}
      local source=object
      if player and object.body and object.body.application_player_equipment and object.body.application_player_equipment.status=="Value" then
        -- 按既有native补源合同分开当前充能/冷却与装备成员；不让这些每步值推进静态结构版本。
        source={};for k,v in pairs(object) do source[k]=v end
        source.body={};for k,v in pairs(object.body) do source.body[k]=v end
        local equipment=object.body.application_player_equipment
        local fixed={};for k,v in pairs(equipment) do fixed[k]=v end
        local dynamic={version=1,status="Value",head_animation_hex=equipment.head_animation_hex,history_mutation_disabled=equipment.history_mutation_disabled,weapons={},effects={}}
        fixed.head_animation_hex=nil;fixed.history_mutation_disabled=nil
        if equipment.effects then
          fixed.effects={entries={}}
          for _,k in ipairs({"cache_flags","evaluate","disabled"}) do dynamic.effects[k]=equipment.effects[k] end
          dynamic.effects.entries={}
          for i,effect in ipairs(equipment.effects.entries) do
            fixed.effects.entries[i]={item_present=effect.item_present,type=effect.type,id=effect.id}
            dynamic.effects.entries[i]={count=effect.count,cooldown=effect.cooldown}
          end
        end
        fixed.weapons={}
        for i,weapon in ipairs(equipment.weapons) do
          local w={};for k,v in pairs(weapon) do if k~="charge_bar" then w[k]=v end end
          fixed.weapons[i]=w;dynamic.weapons[i]={present=weapon.present,charge_bar=weapon.charge_bar}
        end
        source.body.application_player_equipment=fixed;source.body.application_player_equipment_dynamic=dynamic
      end
      local parts=split(source,"WorldDynamic",player,domain)
      for layer,piece in pairs(parts) do
        local slot=slots[layer];local target=player and slot=="world" and out.bodies or out[slot]
        target[#target+1]={subject=object.subject,value=piece}
      end
    end
    local rest={};for k,v in pairs(payload) do if k~="objects" then rest[k]=v end end
    for layer,piece in pairs(split(rest,"Context",false,domain)) do out[slots[layer]][#out[slots[layer]]+1]={value=piece} end
  else
    for layer,piece in pairs(split(payload,domain==2 and "WorldDynamic" or "Context",false,domain)) do out[slots[layer]][#out[slots[layer]]+1]={value=piece} end
  end
  local close=graphs(payload,nodeLimit or 64*1024*1024)
  -- Context是每域一张图，其余主体以独立record字段为局部图；编号在整个封套中也保持不冲突。
  out.context=close(out.context)
  for _,slot in ipairs({"structure","resource","world","bodies"}) do for _,entry in ipairs(out[slot]) do entry.value=close(entry.value) end end
  return out
end
IsaacCSStreamObservation=M
return M
