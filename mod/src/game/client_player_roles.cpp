#include "game/client_player_roles.h"
#include "game/client_player_state.h"
#include "game/client_player_equipment.h"
#include "game/client_entity_adapters.h"
#include "replica/player_target.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <set>

namespace cs::game {
namespace {
using V=replica::Value;
[[noreturn]] void Fail(const char *why){throw replica::ContractError(std::string("player_role:")+why);}
const V &At(const V &v,const char *name){if(!v.IsObject()||!v.HasMember(name))Fail(name);return v[name];}
bool Is(const V &v,const char *name,const char *value){const auto &s=At(v,name);return s.IsString()&&std::string(s.GetString(),s.GetStringLength())==value;}
int Int(const V &v){int result=0;const auto &n=v.IsObject()&&v.HasMember("type")&&Is(v,"type","int64")?At(v,"value"):v;
  if(n.IsInt())return n.GetInt();if(n.IsString()){const auto r=std::from_chars(n.GetString(),n.GetString()+n.GetStringLength(),result);if(r.ec==std::errc{}&&r.ptr==n.GetString()+n.GetStringLength())return result;}Fail("i32_required");}
bool Bool(const V &v){if(!v.IsBool())Fail("boolean_required");return v.GetBool();}
ClientPoseVector Point(const V &v){const auto scalar=[](const V &x){if(!x.IsNumber()||!std::isfinite(x.GetDouble())||std::abs(x.GetDouble())>(std::numeric_limits<float>::max)())Fail("finite_vector_required");return float(x.GetDouble());};return {scalar(At(v,"x")),scalar(At(v,"y"))};}
const ClientObjectReference &Link(const ClientPlayerDescription &d,const char *name){const auto i=d.body_relations.find(name);if(i==d.body_relations.end()||!i->second.present)Fail("complete_named_body_links_required");return i->second;}
bool Same(const ClientObjectReference &a,const ClientObjectReference &b){return a.present&&b.present&&a.is_null==b.is_null&&(a.is_null||a.target==b.target);}
ClientObjectReference Self(const ClientPlayerDescription &d){return {true,false,d.entity.key};}
const ClientEntityRelation &EntityLink(const ClientPlayerDescription &d,const char *name){const ClientEntityRelation *found=nullptr;
  for(const auto &r:d.entity.relations)if(r.member==name&&!r.value_only){if(found)Fail("ambiguous_entity_relation");found=&r;}if(!found)Fail("complete_named_entity_links_required");return *found;}
}
const ClientPlayerRolePolicy *ClientPlayerRole(int type) noexcept {
  static constexpr ClientPlayerRolePolicy policies[]={
#define CS_PLAYER_ROLE(type,name,family,task) {type,name,family,task},
#include "game/client_player_roles.inc"
#undef CS_PLAYER_ROLE
  };
  static_assert(sizeof(policies)/sizeof(policies[0])==41,"原版类型必须逐个注册");
  return type>=0&&type<41&&policies[type].type==type?&policies[type]:nullptr;
}
const char *ClientPlayerRoleClassification(int type,unsigned variant) noexcept {
  if(type==-1||variant==2)return "native-possessor:separate-owner-not-supported";
  if(variant)return "native-internal-or-baby:full-player-contract-required";
  return ClientPlayerRole(type)?"vanilla-full-player":"undeclared-character:tx-04-27";
}
std::shared_ptr<const ClientPlayerRoleState> ClientDescribePlayerRole(const replica::EntityObjectTarget &o){
  const auto &body=std::get<replica::EntityClassValues<replica::EntityCategory::Player>>(o.payload).fields.value;
  const auto &v=At(body,"application_player_role"),&fields=At(body,"fields");
  if(Int(At(v,"version"))!=1||!Is(v,"status","Value")||!Is(v,"scope","same-ordinary-window"))Fail("current_role_v1_required");
  auto state=std::make_shared<ClientPlayerRoleState>();
  state->item_state=Int(At(v,"item_state"));state->suplex_state=Int(At(v,"suplex_state"));state->suplex_countdown=Int(At(v,"suplex_countdown"));
  if(state->suplex_state<0||state->suplex_state>5)Fail("suplex_state_range");
  state->suplex_target=Point(At(v,"suplex_target"));state->suplex_land=Point(At(v,"suplex_land"));
  state->is_sub=Bool(At(v,"is_sub"));state->hologram=Bool(At(v,"hologram"));
  state->player_index=Int(At(v,"player_index"));
  if(state->player_index< -1||state->player_index>=256||state->is_sub!=(state->player_index<0))Fail("player_index_or_native_sub_predicate");
  if(Int(At(fields,"EntityPlayer.GetItemState"))!=state->item_state||Int(At(fields,"EntityPlayer.GetSuplexState"))!=state->suplex_state||
      Int(At(fields,"EntityPlayer.GetSuplexAimCountdown"))!=state->suplex_countdown||Bool(At(fields,"EntityPlayer.IsSubPlayer"))!=state->is_sub||Bool(At(fields,"EntityPlayer.IsHologram"))!=state->hologram)Fail("same_window_public_role_conflict");
  if(Int(At(fields,"EntityPlayer.GetPlayerIndex"))!=state->player_index)Fail("same_window_player_index_conflict");
  const auto target=Point(At(fields,"EntityPlayer.GetSuplexTargetPosition")),land=Point(At(fields,"EntityPlayer.GetSuplexLandPosition"));
  if(target.x!=state->suplex_target.x||target.y!=state->suplex_target.y||land.x!=state->suplex_land.x||land.y!=state->suplex_land.y)Fail("same_window_suplex_vector_conflict");
  return state;
}
std::shared_ptr<const ClientPlayerSpecialState> ClientDescribePlayerSpecial(const replica::EntityObjectTarget &o){
  const auto &body=std::get<replica::EntityClassValues<replica::EntityCategory::Player>>(o.payload).fields.value;
  const auto &v=At(body,"application_player_special");
  if(Int(At(v,"version"))!=1||!Is(v,"status","Value")||!Is(v,"scope","same-ordinary-window"))Fail("current_special_v1_required");
  auto state=std::make_shared<ClientPlayerSpecialState>();
  const auto array=[&](const char *name,auto &out,int low,int high){const auto &a=At(v,name);
    if(!a.IsArray()||a.Size()!=out.size())Fail("special_complete_slot_domain_required");
    for(unsigned i=0;i<out.size();++i){const int n=Int(a[i]);if(n<low||n>high)Fail("special_slot_range");out[i]=n;}};
  array("inventory_history",state->inventory_history,-1,INT32_MAX);array("crafting",state->crafting,0,29);array("poop",state->poop,0,11);
  state->crafting_item=Int(At(v,"crafting_item"));state->crafting_pool=Int(At(v,"crafting_pool"));
  const auto &images_source=At(v,"images");if(!images_source.IsArray()||images_source.Size()!=2)Fail("two_player_draw_images_required");
  for(unsigned i=0;i<2;++i){const auto &row=images_source[i];auto &image=state->images[i];image.present=Bool(At(row,"present"));if(!image.present)continue;
    const auto &path=At(row,"image");if(!path.IsString()||!path.GetStringLength()||path.GetStringLength()>1024)Fail("player_draw_image_path");image.path.assign(path.GetString(),path.GetStringLength());if(image.path.find('\0')!=std::string::npos)Fail("player_draw_image_embedded_nul");
    const auto u=[&](const char *name){int n=Int(At(row,name));if(n<0)Fail("player_image_parameter_range");return unsigned(n);};
    image.shader=u("shader");image.stride=u("stride");image.min_filter=u("min_filter");image.mag_filter=u("mag_filter");image.wrap_s=u("wrap_s");image.wrap_t=u("wrap_t");
    const auto &attributes=At(row,"attributes");if(!attributes.IsArray()||attributes.Size()>8)Fail("player_image_format");for(const auto &a:attributes.GetArray()){int n=Int(a);if(n<0||n>255)Fail("player_image_format_attribute");image.attributes.push_back(static_cast<unsigned char>(n));}
  }
  const auto &images=At(v,"afterimages");if(!(images.IsObject()&&images.ObjectEmpty())){
    if(!images.IsArray()||images.Size()>16384)Fail("afterimage_sequence_or_budget");
    for(const auto &row:images.GetArray())state->afterimages.push_back({Int(At(row,"duration")),Point(At(row,"position"))});
  }
  state->record_cursor=Int(At(v,"record_cursor"));const auto &records=At(v,"records");
  if(!(records.IsObject()&&records.ObjectEmpty())){if(!records.IsArray()||records.Size()>16384)Fail("record_sequence_budget");
    for(const auto &r:records.GetArray()){ClientPlayerSpecialState::Record row;row.position=Point(At(r,"position"));row.animation=Int(At(r,"animation"));row.frame=Int(At(r,"frame"));row.second_animation=Int(At(r,"second_animation"));row.second_frame=Int(At(r,"second_frame"));
      if(row.animation< -1||row.second_animation< -1||(row.animation>=0&&row.frame<0)||(row.second_animation>=0&&row.second_frame<0))Fail("record_animation_or_frame");state->records.push_back(row);}}
  if(state->record_cursor< -1||(state->record_cursor>=0&&unsigned(state->record_cursor)>=state->records.size()))Fail("record_cursor_index");
  const auto &pose=At(v,"record_sprite");if(Is(pose,"status","Null")){state->record_sprite.loaded=false;state->record_sprite.exact_phase=true;state->record_sprite.champion_color_present=true;state->record_sprite.shaders_present=true;}
  else {std::string why;if(!ClientParseResourcePose(pose,state->record_sprite,why))Fail(why.c_str());}
  if(state->record_cursor>=0&&!state->record_sprite.loaded)Fail("active_record_sprite_unloaded");
  if(state->crafting_item<0)Fail("crafting_item_range");return state;
}
// 整体关系校验只读T；任何缺副体/跨组/重叠/错shape在原生写前定位，不自动补造身体。
void ClientPlayerRolesPrepare(const std::vector<ClientPlayerDescription> &bodies,const replica::PlayerTarget &target){
  std::map<ClientObjectKey,const ClientPlayerDescription *> index;
  std::map<ClientObjectKey,ClientObjectKey> holders;
  std::set<ClientObjectKey> sub_owners;
  for(const auto &d:bodies){if(!ClientPlayerRole(d.player_type)||d.entity.variant)Fail(ClientPlayerRoleClassification(d.player_type,d.entity.variant));
    if(!d.role||!d.state||d.player_type!=d.state->player_type||!index.emplace(d.entity.key,&d).second)Fail("body_source_or_type_conflict");}
  for(const auto &d:bodies){
    const auto &policy=*ClientPlayerRole(d.player_type);
    if(!d.special||!d.equipment||!d.equipment->history)Fail("special_source_or_history_missing");
    if(d.render_member&&(!d.special->images[0].present||!d.special->images[1].present))Fail("render_player_native_images_missing");
    // TIsaac的槽是History下标，不是collectible ID；合法空槽保留-1，不按库存相等重排。
    if(d.player_type==21)for(int slot:d.special->inventory_history)if(slot>=0&&unsigned(slot)>=d.equipment->history->items.size())Fail("inventory_history_index_outside_same_body_history");
    // 703 VarData=1..4指向全局槽；零表示尚未取得槽。槽不能由ordinal或PlayerIndex推算。
    for(unsigned i=0;i<4;++i)if(d.state->active[i].backup_slot>=0&&d.active_backups[i].is_null)Fail("esau_active_slot_backup_missing");
    if(d.roster_index>=0&&d.role->player_index!=d.roster_index)Fail("roster_source_player_index_conflict");
    const auto body=[&](const ClientObjectReference &r)->const ClientPlayerDescription *{
      if(r.is_null)return nullptr;const auto i=index.find(r.target);if(i==index.end())Fail("body_link_missing");const auto *peer=i->second;
      if(d.membership_known&&peer->membership_known&&(!(d.group==peer->group)||!(d.participant==peer->participant)||d.ordinal!=peer->ordinal||d.control_session!=peer->control_session||d.binding_revision!=peer->binding_revision))Fail("paired_body_cross_control_group");return peer;};
    for(auto name:{"sub_storage","twin_storage","backup_storage","replaced_storage","main_twin","other_twin","sub_player","flipped_form"})body(Link(d,name));
    // 普通身体GetMainTwin=self并非storage自环；getter与原生存储必须分别留存。
    if(!Same(Link(d,"flipped_form"),Link(d,"backup_storage")))Fail("flipped_form_backup_storage_conflict");
    if(!Same(Link(d,"sub_player"),Link(d,"sub_storage")))Fail("sub_player_storage_conflict");
    const auto &twin=Link(d,"twin_storage");const auto *peer=body(twin);
    const auto main=peer&&peer->role->player_index!=-1&&peer->role->player_index<d.role->player_index?twin:Self(d);
    if(!Same(Link(d,"main_twin"),main)||!Same(Link(d,"other_twin"),twin))Fail("native_main_other_twin_predicate_conflict");
    const bool hologram=d.entity.exists&&(d.player_type==29||d.player_type==38)&&peer&&peer!=&d&&peer->role->player_index!=-1&&peer->role->player_index<d.role->player_index;
    if(d.role->hologram!=hologram)Fail("hologram_native_predicate_conflict");
    if(policy.family==6||policy.family==8){
      const int other=d.player_type==19?20:d.player_type==20?19:d.player_type==35?40:35;
      if(!peer||peer==&d||peer->player_type!=other||!Same(Link(*peer,"twin_storage"),Self(d)))Fail("paired_twin_missing_or_nonreciprocal");
      if((d.roster_index>=0)!=(peer->roster_index>=0))Fail("active_twin_roster_membership_conflict");
      // 主体由实际PlayerIndex决定，不能硬编码Jacob或35永远是main。
    }
    if(policy.family==7){
      const auto *sub=body(Link(d,"sub_storage"));
      if(d.entity.exists&&d.role->player_index>=0&&!sub)Fail("forgotten_primary_sub_body_missing");
      if(sub){
        // 普通16/17是roster到非roster的单向补体；Swap交换两实例的类型，不交换Key或指针。
        const int other=d.player_type==16?17:16;
        if(sub==&d||sub->player_type!=other||!sub->role->is_sub||sub->roster_index>=0||!Link(*sub,"sub_storage").is_null||!sub_owners.insert(sub->entity.key).second)Fail("forgotten_sub_role_or_unique_owner_conflict");
      }
    }
    if(policy.family==9){
      const auto *backup=body(Link(d,"backup_storage"));
      // Flip两套状态保存在不同身体；Birthright只改变实际twin/roster结果，不在Client使用道具。
      if(!backup||backup==&d||(backup->player_type!=29&&backup->player_type!=38)||!Same(Link(*backup,"backup_storage"),Self(d)))Fail("lazarus_flipped_pair_missing_or_nonreciprocal");
      if(peer&&(peer==&d||(peer->player_type!=29&&peer->player_type!=38)||!Same(Link(*peer,"twin_storage"),Self(d))))Fail("lazarus_birthright_twin_conflict");
    }
    // held拥有自己的Key与形态；隐藏是合法父代画，普通可见对象仍保留Source可见性。
    const auto &held=EntityLink(d,"EntityPlayer.GetHeldEntity");
    if(!held.is_null){
      if(held.target==d.entity.key||!holders.emplace(held.target,d.entity.key).second)Fail("held_self_or_multiple_holders");
      const auto e=std::find_if(target.entities->objects.begin(),target.entities->objects.end(),[&](const auto &x){return ClientObjectKey{x.key.run,x.key.id,x.key.space}==held.target;});
      if(e==target.entities->objects.end()||e->presence!=replica::EntityMemberPresence::Current||!d.entity.derived||e->shape_revision!=d.entity.derived->player_held_shape)Fail("held_current_shape_required");
    }
    // 全组最多64身体、最多四组由PlayerTarget严格解析；这里不新增四身体上限。
  }
  // 原生Render可以借被举起Player；禁止环导致递归代画，合法链按同T稳定Key终止。
  for(const auto &[held,holder]:holders){std::set<ClientObjectKey> seen{held};auto current=holder;
    for(;;){if(!seen.insert(current).second)Fail("held_render_cycle");const auto i=holders.find(current);if(i==holders.end())break;current=i->second;}}
}
const char *ClientPlayerRoleDisposition(const std::string &id) noexcept {
  if(id=="EntityPlayer.GetPlayerType")return "I22:41-vanilla-types;named-family-policy;no-starting-items-or-randomization";
  if(id=="EntityPlayer.GetHeldEntity"||id=="EntityPlayer.GetItemState"||id.find("EntityPlayer.GetSuplex")==0)return "I21:typed-current-held-key/shape+final-composite-state;never-Throw-or-Suplex";
  if(id=="EntityPlayer.GetPlayerIndex"||id=="EntityPlayer.GetMainTwin"||id=="EntityPlayer.GetOtherTwin"||id=="EntityPlayer.GetSubPlayer"||id=="EntityPlayer.IsSubPlayer"||id=="EntityPlayer.GetFlippedForm"||id=="EntityPlayer.IsHologram")return "I23-I25:whole-target-role-graph;all-links-before-native-public-getter-check";
  return nullptr;
}
} // namespace cs::game
