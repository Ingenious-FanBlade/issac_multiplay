-- tx21固定分层来源：在Lua编码前按具名字段路由，不扫描完整JSON再裁剪热字段。
if IsaacCSStreamObservation then return IsaacCSStreamObservation end
local M={}
-- CaptureLuaEntity传入零基模块下标；它不是从1开始的Observation DomainId。
local Domain={Entity=0,Grid=1,Room=2,Level=3,Exploration=4,RoomSave=5,Player=6,Run=7,Random=8,Presentation=9,Mod=10}
local nativeDomains={[Domain.Entity]=true,[Domain.Grid]=true,[Domain.Room]=true,[Domain.Player]=true}
-- 分区列表有固定数组合同；空列表也必须编码成[]，不能与空对象混淆。
local jsonArray={__isaac_cs_array=true}
local function array() return setmetatable({},jsonArray) end
function M.IsJsonArray(value) return getmetatable(value)==jsonArray end
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
  -- Body/Head Render从同一身体读取内嵌ANM、服装和15项层映射，必须同组采样。
  if spec.id=="EntityPlayer.GetBodySprite" or spec.id=="EntityPlayer.GetCostumeSpriteDescs" or spec.id=="EntityPlayer.GetCostumeLayerMap" or spec.owner=="CostumeSpriteDesc" or spec.id=="ItemConfigItem.ID" or spec.id=="ItemConfigItem.Type" then return true end
  if scope(spec.id,true)~="PlayerFast" then return false end
  if spec.category~="field" and spec.category~="query" then return false end
  local typ=(spec.returns or ""):gsub("%?","")
  return typ=="int" or typ=="float" or typ=="double" or typ=="number" or typ=="boolean" or typ=="bool" or
    typ=="Vector" or typ=="Color" or typ=="KColor" or typ=="BitSet128" or typ=="TearFlags" or
    spec.id=="Entity.GetSprite" or (spec.owner=="Sprite" or spec.owner=="LayerState") and
      (typ=="LayerState" or typ=="BlendMode" or typ=="LayerStates[]" or typ=="string")
end
local nativeScopes={
  application_links="Structure",application_player="Structure",application_player_role="WorldDynamic",
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
  control_groups="Structure",roster="Structure",backup_slots="Structure",relations="Structure",native_membership="Structure",canonical_state="Structure"
}
local metadata={coverage=true,sampling=true,previous_scope=true,sample_revision=true,changes=true}
local saveCollectionIdentity="immutable_collection_revision_and_ordinal;not_native_object_continuity"
-- 采样关联保留本窗原值，放入每次必有新版本的普通范围；不参与稳定内容判等。
-- 只识别本项目声明的具名结构，不递归删除名为scope的玩法字段或Mod自定义字段。
local function samplingPiece(key,v,domain,parent)
  if type(v)~="table" then
    if domain==Domain.RoomSave and key=="source_scope" and
      parent.identity_basis==saveCollectionIdentity then
      return nil,v
    end
    return v,nil
  end
  if (domain==Domain.RoomSave and key=="temporal_relation" and v.kind=="ObservationScopes") or
    (domain==Domain.Player and key=="continuity") then return nil,v end
  local selected
  if key=="location" and v.session and v.epoch then selected={scope=true,entity_scope=true}
  elseif domain==Domain.Player and key=="application_source" and v.contract=="player-source-v2" then selected={entity_scope=true}
  elseif domain==Domain.Player and key=="canonical_state" and v.schema=="isaac.cs.entity" then selected={scope=true,source_scope=true}
  elseif domain==Domain.Presentation and key=="canonical_player_state" and v.schema=="isaac.cs.entity" then selected={scope=true} end
  if not selected then return v,nil end
  local state,sampling={},{}
  for name,value in pairs(v) do
    if selected[name] then sampling[name]=value else state[name]=value end
  end
  return state,next(sampling) and sampling or nil
end
-- 只分割有字段合同的树；无公开细分合同的私有值保留在普通联合范围，绝不猜成身体快写资格。
local function split(value,fallback,body,domain)
  -- 未原生应用的域属于具名保留上下文；嵌套Sprite不能因共享成员名泄漏进世界快范围。
  local retained=not nativeDomains[domain]
  if type(value)~="table" then return {[fallback]=value} end
  if next(value)==nil then return {[fallback]=value} end
  -- 未加载ANM没有正在播放的姿态。完整当前值作为资源依赖保留，每个普通窗口仍重读，
  -- 任一值变化都产生新完整版本；不按路径别名复用，也不把未读值当作常量。
  if value.type=="Sprite" and value.pose_runtime and value.pose_runtime.loaded==false then
    return {[retained and "Context" or "Resource"]=value}
  end
  -- 保存房间/集合的Present、Empty等status描述完整容器，不是读失败叶子；继续拆出其采样关联。
  local saveContainer=domain==Domain.RoomSave and (value.identity_basis==saveCollectionIdentity or value.lifetime=="current_floor_and_construction_revision")
  if value.type=="reference" or value.resolution or value.status and value.status~="Value" and not value.fields and not saveContainer then return {[fallback]=value} end
  if value.type=="graph_ref" then return {[fallback]=value} end
  -- Wall/Decoration等派生类可以有真实的空fields；类型和空字段表仍是完整后态。
  -- 只含这三项的空schema不能因没有可拆子字段而消失；带其它状态的对象继续逐项拆层。
  if value.type and type(value.fields)=="table" and next(value.fields)==nil then
    local emptySchema=true
    for key in pairs(value) do if key~="type" and key~="version" and key~="fields" then emptySchema=false;break end end
    if emptySchema then return {[fallback]=value} end
  end
  if value.type=="table" and value.entries then
    -- 显式空表也是后态，不能在拆分时消失，否则最终清空会隐式继承旧集合。
    if next(value.entries)==nil then return {[fallback]=value} end
    local groups={}
    for _,entry in ipairs(value.entries) do
      for layer,piece in pairs(split(entry.value,fallback,body,domain)) do
        groups[layer]=groups[layer] or {type="table",node=value.node,entries={}}
        groups[layer].entries[#groups[layer].entries+1]={key=entry.key,value=piece}
      end
    end
    return groups
  end
  -- 固定槽数组逐层保留同一长度。缺本层片段不是真实Null，也不能让ipairs截断后面的槽。
  local count=#value
  if count>0 then
    local sequence=true
    for key in pairs(value) do if type(key)~="number" or key%1~=0 or key<1 or key>count then sequence=false;break end end
    if sequence then
      local groups={}
      for i=1,count do
        if value[i]==nil then error("stream_source_array_hole") end
        for layer,piece in pairs(split(value[i],fallback,body,domain)) do
          groups[layer]=groups[layer] or array();groups[layer][i]=piece
        end
      end
      for _,pieces in pairs(groups) do for i=1,count do if pieces[i]==nil then pieces[i]={type="stream_absent_piece"} end end end
      return groups
    end
  end
  local out={}
  local function add(layer,key,v) out[layer]=out[layer] or {};out[layer][key]=v end
  for key,v in pairs(value) do
    if not metadata[key] then
      if key=="fields" then
        for id,field in pairs(v) do
          local layer=retained and "Context" or scope(id,body)
          -- 独立PlayerFast仅来自更新见证；普通样本中的身体动态保留在联合后态。
          if body and layer=="PlayerFast" then layer="WorldDynamic" end
          -- 当前动画描述随选择变化；内部长度等资源元数据不能脱离当前选择提升成Resource换代。
          -- 完整描述及图关系仍在同窗传输，真正ANM路径／代次仍独立进入Resource。
          local selected=not retained and (id=="Sprite.GetCurrentAnimationData" or id=="Sprite.GetOverlayAnimationData")
          local pieces=selected and {[layer]=field} or split(field,layer,body,domain)
          for group,piece in pairs(pieces) do
            -- ordinary保留完整身体后态；独立PlayerFast提交时再按采样时刻选择覆盖。
            out[group]=out[group] or {};out[group].fields=out[group].fields or {};out[group].fields[id]=piece
          end
        end
      elseif key=="type" and type(v)=="string" or key=="version" then
        -- 下面为每个实际存在的分支补schema身份，不把类型标签当独立状态。
      else
        local layer=retained and "Context" or nativeScopes[key] or fallback
        local state,sampling=samplingPiece(key,v,domain,value)
        -- 直接保留关联片段，不能再次按retained或字段名路由回Context。
        if sampling~=nil then add("WorldDynamic",key,sampling) end
        if state~=nil then
          local pieces=split(state,layer,body,domain)
          for group,piece in pairs(pieces) do add(group,key,piece) end
        end
      end
    end
  end
  for _,v in pairs(out) do if value.type then v.type=value.type=="Sprite" and "stream_sprite_piece" or value.type end;if value.version then v.version=value.version end end
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
      local copy,keys=M.IsJsonArray(v) and array() or {},{}
      -- 纯连续数组按实际槽序复制，无需为数千保存项逐次建立字符串比较排序；混合表仍排序。
      local count=#v;local sequence=count>0
      if sequence then for k in pairs(v) do if type(k)~="number" or k%1~=0 or k<1 or k>count then sequence=false;break end end end
      if sequence then for i=1,count do copy[i]=clone(v[i],depth+1) end;return copy end
      for k in pairs(v) do keys[#keys+1]=k end
      table.sort(keys,function(a,b)return type(a)==type(b) and tostring(a)<tostring(b) or type(a)<type(b)end)
      for _,k in ipairs(keys) do copy[k]=k=="node" and (v.type=="table" or v.type=="graph_ref") and id(v[k]) or clone(v[k],depth+1) end
      return copy
    end
    return clone(root,0)
  end
end
function M.Partition(payload,domain,nodeLimit)
  if type(domain)~="number" or domain%1~=0 or domain<Domain.Entity or domain>Domain.Mod then error("stream_source_domain_index") end
  local out={contract="stream-source-v1",context=array(),structure=array(),resource=array(),world=array(),bodies=array(),members=array()}
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
    for layer,piece in pairs(split(payload,domain==Domain.Grid and "WorldDynamic" or "Context",false,domain)) do out[slots[layer]][#out[slots[layer]]+1]={value=piece} end
  end
  local close=graphs(payload,nodeLimit or 64*1024*1024)
  -- Context是每域一张图，其余主体以独立record字段为局部图；编号在整个封套中也保持不冲突。
  out.context=close(out.context)
  for _,slot in ipairs({"structure","resource","world","bodies"}) do for _,entry in ipairs(out[slot]) do entry.value=close(entry.value) end end
  return out
end
IsaacCSStreamObservation=M
return M
