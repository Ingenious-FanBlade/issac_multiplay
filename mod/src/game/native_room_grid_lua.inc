// 原生对象的公开可写字段／资源姿态适配；不注册任何gameplay回调。
R"cs_lua(
local A={}
local function scalar(v)
  if type(v)=="table" and v.type=="int64" then return assert(math.tointeger(v.value),"native integer") end
  return v
end
local function absent(v) return type(v)=="table" and (v.status=="Null" or v.status=="NotApplicable" or v.status=="AuthorityOnly") end
local function vector(v) return Vector(v.x,v.y) end
local function color(v)
  local c=Color(v.R,v.G,v.B,v.A,v.RO or 0,v.GO or 0,v.BO or 0)
  if v.GetColorize and v.GetColorize.entries then
    local t={} for _,x in ipairs(v.GetColorize.entries) do t[x.key]=scalar(x.value) end
    c:SetColorize(t.R,t.G,t.B,t.A)
  end
  return c
end
local function kcolor(v) return KColor(v.R,v.G,v.B,v.A) end
local properties={
 GridEntityDoor={"Busted","CloseAnimation","CurrentRoomType","Direction","ExtraVisible","LockedAnimation","OpenAnimation","OpenLockedAnimation","PreviousState","PreviousVariant","Slot","TargetRoomIndex","TargetRoomType"},
 GridEntityPoop={"ReducedSpawnRate","ReviveTimer","StateAnimation","UnderPlayer"},
 GridEntityPressurePlate={"NextGreedAnimation"},
 GridEntityRock={"Anim","FrameCnt","RubbleAnim"},
 GridEntitySpikes={"Timeout"}, GridEntityTNT={"FrameCnt"}
}
function A.Grid(g,object)
  local class=object.class
  local d=assert(g["To"..class:sub(11)](g),"native derived cast")
  local f=assert(object.bodies[class].fields)
  if class=="GridEntityPit" then d:SetLadder(f["GridEntityPit.HasLadder"])
  elseif class=="GridEntityRock" then
    d:SetBigRockFrame(scalar(f["GridEntityRock.GetBigRockFrame"]))
    d:UpdateAnimFrame()
  end
  -- 原生外观维护会重选Anim等派生字段，之后才覆盖Authority的最终值。
  for _,name in ipairs(properties[class] or {}) do
    local v=assert(f[class.."."..name]~=nil and {f[class.."."..name]},"native derived field")[1]
    if not absent(v) then d[name]=scalar(v) end
  end
  local water=object.common.fields["GridEntity.GetWaterClipFlags"]
  if not absent(water) then g:SetWaterClipFlags(scalar(water)) end
end
function A.Room(r,room)
  local f=room.state.fields
  -- 只调用写最终值的公开方法；不调用Clear、Reward、Update、开门裁决或伤害。
  local setters={GetFloorColor="SetFloorColor",GetWallColor="SetWallColor",GetWaterAmount="SetWaterAmount",
    GetWaterColor="SetWaterColor",GetWaterColorMultiplier="SetWaterColorMultiplier",GetWaterCurrent="SetWaterCurrent",
    GetGreedWaveTimer="SetGreedWaveTimer",GetRoomClearDelay="SetRoomClearDelay",
    IsClear="SetClear",IsAmbushDone="SetAmbushDone",IsSacrificeDone="SetSacrificeDone",IsFirstEnemyDead="SetFirstEnemyDead",
    GetRainIntensity="SetRainIntensity",GetLavaIntensity="SetLavaIntensity",GetLightningIntensity="SetLightningIntensity"}
  for from,to in pairs(setters) do
    local v=f["Room."..from]
    if v~=nil and not absent(v) then
      if from=="GetFloorColor" or from=="GetWallColor" then v=color(v)
      elseif from=="GetWaterColor" or from=="GetWaterColorMultiplier" then v=kcolor(v)
      elseif from=="GetWaterCurrent" then v=vector(v) else v=scalar(v) end
      r[to](r,v)
    end
  end
  local fx=f["Room.GetFXParams"].fields;local dst=r:GetFXParams()
  for _,name in ipairs({"UseWaterV2","ShadowAlpha"}) do dst[name]=scalar(fx["FXParams."..name]) end
  for _,name in ipairs({"WaterColor","WaterColorMultiplier","ShadowColor","LightColor"}) do dst[name]=kcolor(fx["FXParams."..name]) end
  dst.WaterEffectColor=color(fx["FXParams.WaterEffectColor"])
  local cm=fx["FXParams.ColorModifier"].fields
  local mod=dst.ColorModifier
  for _,name in ipairs({"R","G","B","A","Brightness","Contrast"}) do mod[name]=scalar(cm["ColorModifier."..name]) end
  dst.ColorModifier=mod
end
function A.PostInit(g) g:PostInit() end
function A.Render(r) r:Render() end
-- 与正式观测共用Sprite值读取器，FX内嵌动画也不转成像素或借用userdata。
function A.CaptureSprite(s)
  local R=assert(IsaacCSObservationValues)
  local c=R.Context()
  local value=R.Value(s,"Sprite",c,0)
  local text,complete=R.Finish(value,c)
  assert(complete,"native FX sprite capture incomplete:"..s:GetFilename()..":"..R.Encode(c.reasons))
  return text
end
function A.ReadGrid(g,class)
  local d=assert(g["To"..class:sub(11)](g));local saved=g:GetSaveState()
  local out={type=g:GetType(),variant=g:GetVariant(),index=g:GetGridIndex(),state=g.State,collision=g.CollisionClass,
    save={Type=saved.Type,Variant=saved.Variant,State=saved.State,SpawnCount=saved.SpawnCount,
      SpawnSeed=saved.SpawnSeed,VariableSeed=saved.VariableSeed,VarData=saved.VarData,Initialized=saved.Initialized},derived={},
    sprite={filename=g:GetSprite():GetFilename(),animation=g:GetSprite():GetAnimation(),frame=g:GetSprite():GetFrame()},water=g:GetWaterClipFlags(),position={x=g.Position.X,y=g.Position.Y}}
  for _,name in ipairs(properties[class] or {}) do out.derived[class.."."..name]=d[name] end
  if class=="GridEntityPit" then out.derived["GridEntityPit.HasLadder"]=d.HasLadder end
  if class=="GridEntityRock" then out.derived["GridEntityRock.GetBigRockFrame"]=d:GetBigRockFrame() end
  -- CanBlowOpen是只读派生查询，不能当同名公开字段赋值；仍核对实际恢复结果。
  if class=="GridEntityDoor" then out.derived["GridEntityDoor.CanBlowOpen"]=d:CanBlowOpen();out.derived["GridEntityDoor.IsOpen"]=d:IsOpen();out.derived["GridEntityDoor.IsLocked"]=d:IsLocked();out.derived["GridEntityDoor.GetSpriteOffset"]={type="vector",x=d:GetSpriteOffset().X,y=d:GetSpriteOffset().Y} end
  return require("json").encode(out)
end
function A.ReadRoom(r)
  local out={}
  for _,name in ipairs({"GetFloorColor","GetWallColor","GetWaterAmount","GetWaterColor","GetWaterColorMultiplier","GetWaterCurrent","GetGreedWaveTimer","GetRoomClearDelay","GetRedHeartDamage","IsClear","IsAmbushDone","IsSacrificeDone","IsFirstEnemyDead","GetRainIntensity","GetLavaIntensity","GetLightningIntensity","GetLightingAlpha"}) do
    local v=r[name](r)
    if type(v)=="userdata" then
      if name=="GetWaterCurrent" then v={x=v.X,y=v.Y}
      -- KColor的公开分量名与Color不同，读回时映射到统一的RGBA值名。
      elseif name=="GetWaterColor" or name=="GetWaterColorMultiplier" then v={R=v.Red,G=v.Green,B=v.Blue,A=v.Alpha}
      else local c={R=v.R,G=v.G,B=v.B,A=v.A};if name=="GetFloorColor" or name=="GetWallColor" then c.RO=v.RO;c.GO=v.GO;c.BO=v.BO end;v=c end
    end
    out["Room."..name]=v
  end
  return require("json").encode(out)
end
function A.ReadGeometry(r)
  local out={}
  for i=0,447 do local p=r:GetGridPosition(i);out[#out+1]={x=p.X,y=p.Y} end
  return require("json").encode(out)
end
return A
)cs_lua"
