-- Player正式来源扩展；只复用本窗口已复制的公开值，不额外执行Getter或玩法。
local M={}
local groups={
  inventory={"GetCollectiblesList","GetVoidedCollectiblesList","GetWispCollectiblesList","GetSpoofedCollectiblesList","GetSmeltedTrinkets","GetTrinket"},
  active={"GetActiveItemDesc","GetActiveCharge","GetBatteryCharge","GetActiveSubCharge"},
  pocket={"GetPocketItem","GetCard","GetPill","GetMaxPocketItems","QueuedItem"},
  effects={"GetEffects","HasPlayerForm","GetPlayerFormCounter"},
  resources={"GetSprite","GetBodySprite","GetCostumeSpriteDescs","GetCostumeLayerMap"},
  weapons={"GetWeapon","GetActiveWeaponEntity","GetWeaponModifiers"},
}
function M.Describe(body,common,subject)
  local out={version=2,contract="player-source-v2",status="Value",scope_rule="same-ordinary-entity-player-window",public_read_basis="qualified-public-root-or-completed-native-source;root-and-relations-rechecked",member_manifest={},slot_domains={active={0,1,2,3},pocket={0,1,2,3},weapon={0,1,2,3,4},trinket={0,1}},native_readiness=subject.native_readiness}
  -- manifest引用规范字段名；不复制另一份图定义，也不把投影省略误作读取成功。
  for group,names in pairs(groups) do
    local rows={};out.member_manifest[group]=rows
    for _,name in ipairs(names) do
      local id=(name=="GetSprite" and "Entity." or "EntityPlayer.")..name
      local fields=name=="GetSprite" and common.fields or body.fields
      rows[#rows+1]={member=id,captured=fields[id]~=nil}
    end
  end
  -- 公开根不授予私有字段初始化历史；私有draw扩展由后续具名consumer按I04合同补源。
  out.final_state="application_player_state:v1;typed-storage-and-owned-containers;current-use-only"
  out.equipment_state="application_player_equipment:v1;ordered-effects/forms/five-dynamic-weapon-slots;History=player-domain"
  out.storage_semantics="per-body-backing;control/twin relationship is not shared-storage proof;public-shared-query-results remain distinct"
  out.native_presentation={status="Value",resources="application_player_resources:12-distinct-inline-ANM;BodySprite=slot0;ctor-constructed;unloaded-Null",current_draw="application_player_draw;entity-class-application-v3",private_scalars={status="Unavailable",reason="full-player-private-draw-consumers-require-named-current-writer-witness;I18-I29"}}
  out.native_presentation.head_animation="application_player_equipment.head_animation_hex;constructed-string;773990-named-writers;current-ordinary-window"
  return out
end
return M
