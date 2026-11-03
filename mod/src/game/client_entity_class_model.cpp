#include "game/client_entity_adapters.h"
#include "game/client_resource_model.h"
#include "replica/entity_target.h"
#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <type_traits>
namespace cs::game {
namespace {
using V=replica::Value;
[[noreturn]] void Fail(const std::string &id,const char *why){throw replica::ContractError("entity_class:"+id+":"+why);}
const V &At(const V &v,const char *key){if(!v.IsObject()||!v.HasMember(key))Fail(key,"missing");return v[key];}
bool Is(const V &v,const char *key,const char *text){return v.IsObject()&&v.HasMember(key)&&v[key].IsString()&&std::string(v[key].GetString())==text;}
template<class T> T Integer(const V &v){const V &s=Is(v,"type","int64")?At(v,"value"):v;T n{};
  if(s.IsString()){const auto r=std::from_chars(s.GetString(),s.GetString()+s.GetStringLength(),n);if(r.ec==std::errc{}&&r.ptr==s.GetString()+s.GetStringLength())return n;}
  if constexpr(std::is_signed_v<T>){if(s.IsInt64()&&s.GetInt64()>=(std::numeric_limits<T>::min)()&&s.GetInt64()<=(std::numeric_limits<T>::max)())return T(s.GetInt64());}
  else if(s.IsUint64()&&s.GetUint64()<=(std::numeric_limits<T>::max)())return T(s.GetUint64());Fail("integer","invalid");}
std::uint64_t Bits(const V &v){const auto &s=Is(v,"type","int64")?At(v,"value"):v;
  if((s.IsString()&&s.GetStringLength()&&s.GetString()[0]=='-')||(s.IsInt64()&&s.GetInt64()<0))return std::uint64_t(Integer<std::int64_t>(v));return Integer<std::uint64_t>(v);}
float Real(const V &v){const double n=v.IsNumber()?v.GetDouble():double(Integer<std::int64_t>(v));if(!std::isfinite(n)||std::abs(n)>(std::numeric_limits<float>::max)())Fail("float","range");return float(n);}
ClientPoseColor Color(const V &v){ClientPoseColor c;const char *t[] = {"R","G","B","A"},*o[]={"RO","GO","BO"};
  for(unsigned i=0;i<4;++i)c.tint[i]=Real(At(v,t[i]));for(unsigned i=0;i<3;++i)c.offset[i]=Real(At(v,o[i]));
  const auto &entries=At(At(v,"GetColorize"),"entries");if(!entries.IsArray()||entries.Size()!=4)Fail("color","colorize");unsigned mask=0;
  for(const auto &e:entries.GetArray()){const auto &key=At(e,"key");if(!key.IsString())Fail("color","key");unsigned i=0;while(i<4&&std::string(key.GetString())!=t[i])++i;if(i==4||(mask&(1u<<i)))Fail("color","duplicate");mask|=1u<<i;c.colorize[i]=Real(At(e,"value"));}return c;}
struct Rule final {unsigned category;ClientEntityValueKind kind;std::int64_t low,high;};
Rule Storage(unsigned category,ClientEntityValueKind kind,const char *type){std::int64_t low=INT32_MIN,high=INT32_MAX;
  if(!std::strcmp(type,"std::uint8_t")){low=0;high=UINT8_MAX;}else if(!std::strcmp(type,"std::uint16_t")){low=0;high=UINT16_MAX;}else if(!std::strcmp(type,"std::uint32_t")){low=0;high=UINT32_MAX;}
  return {category,kind,low,high};}
const std::map<std::string,Rule> &Rules(){static const std::map<std::string,Rule> rules={
#define CS_CLASS_FIELD(category,id,offset,type,kind) {id,Storage(category,ClientEntityValueKind::kind,#type)},
#include "game/client_entity_class_fields.inc"
#undef CS_CLASS_FIELD
};return rules;}
bool Authority(const std::string &id){static const std::set<std::string> facts={
  "EntityNPC.CalcTargetPosition","EntityNPC.CanReroll","EntityNPC.CanShutDoors","EntityNPC.GetAliveEnemyCount","EntityNPC.GetHitList","EntityNPC.Pathfinder",
  "EntityNPC.GetPlayerTarget","EntityNPC.IsBoss","EntityNPC.IsBossColor","EntityNPC.IsChampion","EntityNPC.QueryNPCsGroup","EntityNPC.QueryNPCsSpawnerType","EntityNPC.QueryNPCsType",
  "EntityDelirium.GetTeleportationTimer","EntityDelirium.IsRedMode","EntityTear.GetHitList","EntityBomb.GetHitList","EntityBomb.IsLoadingCostumes"};return facts.count(id)!=0;}
bool Relation(const std::string &id){return id=="EntityNPC.ParentNPC"||id=="EntityNPC.ChildNPC"||id=="EntityNPC.EntityRef"||id=="EntityNPC.GetSirenPlayerEntity"||id=="EntityTear.StickTarget";}
bool Resource(const std::string &id){return id=="EntityTear.GetDeadEyeSprite"||id=="EntityTear.GetTearEffectSprite"||id=="EntityTear.GetTearHaloSprite"||id=="EntityBomb.GetCostumeLayerSprite";}
void ParseFields(const V &fields,unsigned category,ClientEntityClassState &state){
  for(auto it=fields.MemberBegin();it!=fields.MemberEnd();++it){const std::string id(it->name.GetString());const auto rule=Rules().find(id);
    if(rule==Rules().end()){const auto *policy=replica::EntityApplicationPolicy(id);if(policy&&std::string((*policy)["ownership"].GetString())!="ReplicatedState")continue;
      if(id=="EntityTear.Bounced"){if(!Is(it->value,"status","Null")&&!Is(it->value,"status","NotApplicable"))Fail(id,"removed_since_Repentance");continue;}
      if(Authority(id)||Relation(id)||Resource(id)||id=="EntityNPC.GetFlyingOverride")continue;Fail(id,"disposition_missing");}
    if(rule->second.category!=category)Fail(id,"category_mismatch");const auto &v=it->value;if(Is(v,"status","NotApplicable"))Fail(id,"required_native_value_not_applicable");
    ClientEntityField f;f.member=id;f.kind=rule->second.kind;
    switch(f.kind){case ClientEntityValueKind::Integer:f.integer=Integer<std::int64_t>(v);if(f.integer<rule->second.low||f.integer>rule->second.high)Fail(id,"native_width");break;
      case ClientEntityValueKind::Bits:f.bits=Bits(v);break;
      case ClientEntityValueKind::Bits128:if(!Is(v,"type","bits128"))Fail(id,"bits128");f.bits=Bits(At(v,"low"));f.high_bits=Bits(At(v,"high"));break;
      case ClientEntityValueKind::Real:f.real=Real(v);break;
      case ClientEntityValueKind::Boolean:if(!v.IsBool())Fail(id,"bool");f.boolean=v.GetBool();break;
      case ClientEntityValueKind::Vector:f.vector={Real(At(v,"x")),Real(At(v,"y"))};break;
      case ClientEntityValueKind::Color:f.color=Color(v);break;}
    state.fields.push_back(std::move(f));
  }
}
void Pose(const V &v,const char *id,unsigned index,ClientEntityClassState &state){ClientEntityClassState::Resource r;r.member=id;r.index=index;r.is_null=Is(v,"status","Null");
  if(r.is_null){r.pose.loaded=false;r.pose.exact_phase=true;r.pose.champion_color_present=true;r.pose.shaders_present=true;}else {std::string why;if(!ClientParseResourcePose(v,r.pose,why))Fail(id,why.c_str());}state.resources.push_back(std::move(r));}
}
const char *ClientEntityClassDisposition(const std::string &id) noexcept {if(Rules().count(id))return "native-final-value;class-field-table";if(id=="EntityTear.Bounced")return "removed-since-Repentance;only-Null-or-NotApplicable;never-fabricate-storage";if(id=="EntityNPC.GetFlyingOverride")return "official-Plus-optional-bool;fixed-leaf-Lua-Set-Clear-Get";if(id=="EntityBomb.IsLoadingCostumes")return "authority-lazy-work-request;install-final-five-resources;client-pending=false";if(Authority(id))return "authority-fact;no-client-AI-or-collision-consumer";if(Relation(id))return "typed-reference;class-owner";if(Resource(id))return "independent-native-member-resource;I10";return nullptr;}
void ClientDescribeEntityClass(const replica::EntityObjectTarget &o,ClientObjectDescription &d){
  if(d.category<1||d.category>4)return;auto state=std::make_shared<ClientEntityClassState>();state->delirium=o.delirium;
  const V *body=nullptr;std::visit([&](const auto &p){body=&p.fields.value;},o.payload);const auto &fields=At(*body,"fields");ParseFields(fields,d.category,*state);
  // 解析与native表的必需列一致；缺失/不适用不能悄悄留下池上一use的值。
  for(const auto &rule:Rules())if(rule.second.category==d.category){const auto &id=rule.first;
    if(id.rfind("Native",0)==0)continue;if(id.rfind("EntityDelirium.",0)==0){if(o.delirium)At(At(o.special.value,"fields"),id.c_str());}
    else At(fields,id.c_str());}
  const auto &links=At(*body,"application_links");if(Integer<unsigned>(At(links,"version"))!=1||!Is(links,"status","Value"))Fail("native_links","unavailable_or_version");
  unsigned link_count=0;
#define CS_CLASS_LINK(cat,id,offset) if(cat==0||cat==d.category){++link_count;const auto &link=At(At(links,"links"),id);if(!Is(link,"status","Null")&&!Is(link,"status","KnownCurrent"))Fail(id,"unresolved_or_invalid_link");}
#include "game/client_entity_links.inc"
#undef CS_CLASS_LINK
  if(At(links,"links").MemberCount()!=link_count)Fail("native_links","unknown_or_duplicate_member");
  if(o.delirium)ParseFields(At(o.special.value,"fields"),1,*state);
  if(d.category==4){const auto &runtime=At(*body,"application_runtime");if(Integer<unsigned>(At(runtime,"version"))!=1||!Is(runtime,"status","Value"))Fail("bomb_application_runtime","unavailable_or_version");At(At(runtime,"values"),"NativeBomb.CostumeDrawEnabled");ParseFields(At(runtime,"values"),4,*state);}
  if(d.category==1)for(const auto &alias:{std::pair{"EntityNPC.ParentNPC","Entity.Parent"},std::pair{"EntityNPC.ChildNPC","Entity.Child"}}){
    const ClientEntityRelation *left=nullptr,*right=nullptr;for(const auto &r:d.relations){if(r.member==alias.first)left=&r;if(r.member==alias.second)right=&r;}
    if(!left||!right||left->is_null!=right->is_null||(!left->is_null&&!(left->target==right->target)))Fail(alias.first,"common_reference_alias_conflict");}
  if(d.category==3){const auto &common=At(o.common.value,"fields");if(Real(At(fields,"EntityProjectile.DepthOffset"))!=Real(At(common,"Entity.DepthOffset")))Fail("EntityProjectile.DepthOffset","common_alias_conflict");}
  if(d.category==1){const auto &flying=At(fields,"EntityNPC.GetFlyingOverride");if(Is(flying,"status","Null"))state->flying_override=-1;else if(flying.IsBool())state->flying_override=flying.GetBool()?1:0;else Fail("EntityNPC.GetFlyingOverride","optional_bool");}
  if(d.category==1){const auto &runtime=At(*body,"application_runtime");if(Integer<unsigned>(At(runtime,"version"))!=1||!Is(runtime,"status","Value"))Fail("npc_application_runtime","unavailable_or_version");
    if(Integer<std::uint64_t>(At(runtime,"private_reference_count")))Fail("npc_private_reference_container","pending_I25;never-copy-header-or-count");
    if(!At(runtime,"owned_actor_present").IsBool()||At(runtime,"owned_actor_present").GetBool())Fail("npc_owned_actor","pending_I25_Player20;never-borrow-roster-as-owned-clone");}
  if(d.category==1){const auto &resources=At(*body,"application_resources");if(Integer<unsigned>(At(resources,"version"))!=1||!Is(resources,"status","Value"))Fail("npc_application_resources","unavailable_or_version");
    Pose(At(resources,"npc_overlay"),"npc_overlay",0,*state);Pose(At(resources,"npc_effect"),"npc_effect",0,*state);}
  // 主Sprite与辅助成员分别解析，不按文件名合并，也不把Null当缺失字段。
  const auto &main=At(At(o.common.value,"fields"),"Entity.GetSprite");
  if(Is(main,"status","Null")){d.resource.loaded=false;d.resource.exact_phase=true;d.resource.champion_color_present=true;d.resource.shaders_present=true;}
  else {std::string why;if(!ClientParseResourcePose(main,d.resource,why))Fail("Entity.GetSprite",why.c_str());}
  if(d.category==2)for(const char *id:{"EntityTear.GetDeadEyeSprite","EntityTear.GetTearEffectSprite","EntityTear.GetTearHaloSprite"})Pose(At(fields,id),id,0,*state);
  if(d.category==4){const auto &rows=At(At(fields,"EntityBomb.GetCostumeLayerSprite"),"values");if(!rows.IsArray()||rows.Size()!=5)Fail("bomb_costumes","complete_five_required");unsigned mask=0;
    for(const auto &row:rows.GetArray()){const auto &entries=At(At(row,"parameters"),"entries");if(!entries.IsArray()||entries.Size()!=1)Fail("bomb_costumes","parameter");const auto index=Integer<unsigned>(At(entries[0],"value"));if(index>4||(mask&(1u<<index)))Fail("bomb_costumes","duplicate_or_range");mask|=1u<<index;Pose(At(row,"value"),"EntityBomb.GetCostumeLayerSprite",index,*state);}}
  d.derived=std::move(state);
}
} // namespace cs::game
