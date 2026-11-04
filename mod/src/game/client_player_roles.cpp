#include "game/client_player_roles.h"
#include "game/client_player_state.h"
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
// 整体关系校验只读T；任何缺副体/跨组/重叠/错shape在原生写前定位，不自动补造身体。
void ClientPlayerRolesPrepare(const std::vector<ClientPlayerDescription> &bodies,const replica::PlayerTarget &target){
  std::map<ClientObjectKey,const ClientPlayerDescription *> index;
  std::map<ClientObjectKey,ClientObjectKey> holders;
  std::set<ClientObjectKey> sub_owners;
  for(const auto &d:bodies){if(!ClientPlayerRole(d.player_type)||d.entity.variant)Fail(ClientPlayerRoleClassification(d.player_type,d.entity.variant));
    if(!d.role||!d.state||d.player_type!=d.state->player_type||!index.emplace(d.entity.key,&d).second)Fail("body_source_or_type_conflict");}
  for(const auto &d:bodies){
    const auto &policy=*ClientPlayerRole(d.player_type);
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
