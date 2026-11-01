// 主线程被动恢复器：字段/Setter来自同一产品白名单，不执行AI、攻击、抽池、装备或Update。
R"cs_entity(
local rules=
)cs_entity"
#include "native_entity_rules.inc"
R"cs_entity(
local A={}
local function integer(s)
  local n=0;local first=s:sub(1,1)=="-" and 2 or 1
  for i=first,#s do local d=s:byte(i)-48;assert(d>=0 and d<=9,"native integer");n=n*10+d end
  return first==2 and -n or n
end
local function scalar(v) return type(v)=="table" and v.type=="int64" and integer(v.value) or v end
local function index(root)
  local nodes={}
  local function visit(v)
    if type(v)~="table" then return end
    if v.type=="table" then assert(nodes[v.node]==nil,"native graph duplicate");nodes[v.node]=v end
    for _,x in pairs(v) do visit(x) end
  end
  visit(root);return nodes
end
A.Index=index
local function color(v)
  local c=Color(v.R,v.G,v.B,v.A,v.RO or 0,v.GO or 0,v.BO or 0)
  if v.GetColorize then local t={} for _,e in ipairs(v.GetColorize.entries) do t[e.key]=scalar(e.value) end;c:SetColorize(t.R,t.G,t.B,t.A) end
  return c
end
local function decode(v,resolve,nodes,memo)
  memo=memo or {}
  if type(v)~="table" then return v end
  if v.type=="int64" then return integer(v.value) end
  if v.type=="vector" then return Vector(v.x,v.y) end
  if v.type=="color" then return color(v) end
  if v.type=="bits128" then return BitSet128(integer(v.low),integer(v.high)) end
  if v.type=="reference" then return resolve(v) end
  if v.type=="graph_ref" then v=assert(nodes[v.node],"native graph missing") end
  if v.type=="table" then
    if memo[v.node] then return memo[v.node] end
    local out={};memo[v.node]=out
    for _,e in ipairs(v.entries) do out[decode(e.key,resolve,nodes,memo)]=decode(e.value,resolve,nodes,memo) end;return out
  end
  if v.type=="ColorParams" then local f=v.fields
    -- 官方构造器只有五参且会把两个duration设为同值，随后分别恢复两个真实时钟。
    local c=ColorParams(color(f["ColorParams.GetColor"]),scalar(f["ColorParams.GetPriority"]),scalar(f["ColorParams.GetDuration"]),f["ColorParams.GetFadeout"],f["ColorParams.GetShared"])
    c:SetLifespan(scalar(f["ColorParams.GetLifespan"]));return c
  end
  if v.status=="Null" then return nil end
  error("native state value unavailable:"..tostring(v.status or v.type))
end
local function derived(e,class)
  if class=="Entity" or class=="NativeText" or class=="NativeDummy" then return e end
  if class=="EntityDelirium" then return assert(e:ToNPC():ToDelirium(),"native Delirium cast") end
  return assert(e["To"..class:sub(7)](e),"native derived cast:"..class)
end
-- 先恢复标量和关系后态。只读派生状态由C++具名适配补充；资源最后统一恢复。
function A.State(e,object,resolve,nodes)
  local function body(class,state)
    local d=derived(e,class);local fields=state.fields
    for _,name in ipairs(rules.properties[class] or {}) do
      local v=fields[class.."."..name];assert(v~=nil,"native field:"..class.."."..name)
      d[name]=decode(v,resolve,nodes)
    end
    for getter,setter in pairs(rules.setters[class] or {}) do
      local v=fields[class.."."..getter]
      if v==nil and getter:sub(1,3)=="Get" then v=fields[class.."."..getter:sub(4)] end
      assert(v~=nil,"native setter source:"..class.."."..getter)
      if class=="EntityNPC" and getter=="GetFlyingOverride" and type(v)=="table" and v.status=="Null" then d:ClearFlyingOverride()
      elseif type(v)~="table" or (v.status~="NotApplicable" and not(class=="EntityEffect" and getter=="GetTearFlags" and v.status=="Null")) then d[setter](d,decode(v,resolve,nodes)) end
    end
  end
  body("Entity",object.common)
  local flags=decode(object.common.fields["Entity.GetEntityFlags"],resolve,nodes)
  e:ClearEntityFlags(-1);e:AddEntityFlags(flags)
  if object.class~="EntityPlayer" and object.class~="NativeText" and object.class~="NativeDummy" then body(object.class,object.body) end
  if object.special then body("EntityDelirium",object.special) end
  if object.class=="EntityPlayer" then
    local player=derived(e,"EntityPlayer")
    -- 仅恢复被依赖实体读取的当前战斗/显示量；不执行装备setter、属性重算或完整Player.Init。
    for _,name in ipairs({"Damage","MoveSpeed","MaxFireDelay","ShotSpeed","TearFallingAcceleration","TearFallingSpeed","TearHeight","TearRange","TearFlags","TearColor","LaserColor","TearsOffset","HeadFrameDelay"}) do
      local v=object.body.fields["EntityPlayer."..name];assert(v~=nil,"native player field:"..name);player[name]=decode(v,resolve,nodes)
    end
  end
end
-- Getter白名单由本地代码选择；不允许报文指定任意原生方法。
local sprites={Entity={GetSprite=true},EntityTear={GetDeadEyeSprite=true,GetTearEffectSprite=true,GetTearHaloSprite=true},
 EntityBomb={GetCostumeLayerSprite=true},EntityPickup={GetPriceSprite=true},EntitySlot={GetPrizeSprite=true}}
function A.Sprite(e,request)
  assert(sprites[request.class] and sprites[request.class][request.member],"native sprite getter")
  local d=derived(e,request.class)
  return request.index~=nil and d[request.member](d,request.index) or d[request.member](d)
end
-- 实际读回使用同一白名单；引用由C++只在本次Binding内比较借用地址，不产生或校验摘要。
local function same(actual,v,resolve,nodes,seen)
  if type(v)~="table" then return actual==v end
  if v.status=="Null" then return actual==nil end
  if v.type=="int64" then return actual==integer(v.value) end
  if v.type=="reference" then return resolve(v,actual) end
  if v.type=="vector" then return actual~=nil and actual.X==v.x and actual.Y==v.y end
  if v.type=="bits128" then return actual~=nil and actual.l==integer(v.low) and actual.h==integer(v.high) end
  if v.type=="color" then
    if actual==nil then return false end
    for _,k in ipairs({"R","G","B","A","RO","GO","BO"}) do if actual[k]~=v[k] then return false end end
    return not v.GetColorize or same(actual:GetColorize(),v.GetColorize,resolve,nodes,seen)
  end
  if v.type=="ColorParams" then
    if actual==nil then return false end
    for id,x in pairs(v.fields) do local member=id:match("%.(.+)$");if not same(actual[member](actual),x,resolve,nodes,seen) then return false end end;return true
  end
  if v.type=="graph_ref" then v=assert(nodes[v.node],"native readback graph") end
  if v.type=="table" then
    if type(actual)~="table" then return false end
    seen=seen or {};seen[v.node]=seen[v.node] or {};if seen[v.node][actual] then return true end;seen[v.node][actual]=true
    local count=0;for _ in pairs(actual) do count=count+1 end;if count~=#v.entries then return false end
    for _,entry in ipairs(v.entries) do if not same(actual[decode(entry.key,resolve,nodes)],entry.value,resolve,nodes,seen) then return false end end;return true
  end
  if v.kind=="array" then
    if actual==nil then return false end
    local size=type(actual)=="table" and #actual or actual.Size;if size~=#v.values then return false end
    for i,x in ipairs(v.values) do if not same(type(actual)=="table" and actual[i] or actual:Get(i-1),x,resolve,nodes,seen) then return false end end;return true
  end
  if v.type=="Weapon" then
    if actual==nil then return false end
    for id,x in pairs(v.fields) do
      if not(type(x)=="table" and x.status=="NotScheduled") then
        local name=id:match("%.(.+)$");if not same(actual[name](actual),x,resolve,nodes,seen) then return false end
      end
    end;return true
  end
  return false
end
function A.Verify(e,object,resolve,nodes)
  local function check(class,state)
    local d=derived(e,class)
    local function field(name,getter)
      local expected=state.fields[class.."."..name];if expected==nil and name:sub(1,3)=="Get" then expected=state.fields[class.."."..name:sub(4)] end
      if type(expected)=="table" and expected.status=="NotApplicable" then return end
      local actual;if getter then actual=d[getter](d) else actual=d[name] end
      assert(same(actual,expected,resolve,nodes),"native_readback_mismatch:"..class.."."..name)
    end
    for _,name in ipairs(rules.properties[class] or {}) do field(name) end
    for name in pairs(rules.setters[class] or {}) do field(name,name) end
    for _,name in ipairs(rules.readonly[class] or {}) do
      local isfn=name:match("^Get") or name:match("^Is") or name=="Exists" or name=="CollidesWithGrid";field(name,isfn and name or nil)
    end
  end
  check("Entity",object.common)
  if object.class~="EntityPlayer" and object.class~="NativeText" and object.class~="NativeDummy" then check(object.class,object.body) end
  if object.special then check("EntityDelirium",object.special) end
end
return A
)cs_entity"
