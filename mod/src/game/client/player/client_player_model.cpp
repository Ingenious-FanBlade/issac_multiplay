#include "game/client/player/client_player_state.h"
#include "replica/entity_target.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
#include <set>
#include <type_traits>

namespace cs::game {
namespace {
using V=replica::Value;
[[noreturn]] void Fail(const std::string &id,const char *why){throw replica::ContractError("player_state:"+id+":"+why);}
const V &At(const V &v,const char *key){if(!v.IsObject()||!v.HasMember(key))Fail(key,"required_missing");return v[key];}
bool Is(const V &v,const char *key,const char *text){return v.IsObject()&&v.HasMember(key)&&v[key].IsString()&&std::string(v[key].GetString())==text;}
template<class T> T Int(const V &v){const auto &s=Is(v,"type","int64")?At(v,"value"):v;T n{};
  if(s.IsString()){const auto r=std::from_chars(s.GetString(),s.GetString()+s.GetStringLength(),n);if(r.ec==std::errc{}&&r.ptr==s.GetString()+s.GetStringLength())return n;}
  if constexpr(std::is_signed_v<T>){if(s.IsInt64()&&s.GetInt64()>=(std::numeric_limits<T>::min)()&&s.GetInt64()<=(std::numeric_limits<T>::max)())return T(s.GetInt64());}
  else if(s.IsUint64()&&s.GetUint64()<=(std::numeric_limits<T>::max)())return T(s.GetUint64());Fail("integer","native_width_or_type");}
float Real(const V &v){if(!v.IsNumber()||!std::isfinite(v.GetDouble())||std::abs(v.GetDouble())>(std::numeric_limits<float>::max)())Fail("real","finite_float_required");return float(v.GetDouble());}
const V &Seq(const V &v,unsigned limit=16384){static const V empty(rapidjson::kArrayType);if(v.IsObject()&&v.ObjectEmpty())return empty;if(!v.IsArray()||v.Size()>limit)Fail("sequence","type_or_budget");return v;}
struct Rule {ClientEntityValueKind kind;std::int64_t low,high;};
Rule Storage(ClientEntityValueKind k,const char *t){return {k,0==std::strcmp(t,"std::uint32_t")?0:INT32_MIN,0==std::strcmp(t,"std::uint32_t")?std::int64_t(UINT32_MAX):INT32_MAX};}
const std::map<std::string,Rule> &Rules(){static const std::map<std::string,Rule> rules={
#define CS_PLAYER_FIELD(id,field,type,kind) {id,Storage(ClientEntityValueKind::kind,#type)},
#include "game/client/player/client_player_fields.inc"
#undef CS_PLAYER_FIELD
};return rules;}
// 完整容器按显式索引/顺序解码；重复、空洞、溢出与来源缺失不会退化成零。
std::vector<int> Integers(const V &v){std::vector<int> out;for(const auto &x:Seq(v).GetArray())out.push_back(Int<int>(x));return out;}
void Spoof(const V &v,ClientPlayerState::Spoof &out){std::set<int> seen;
  for(const auto &x:Seq(At(v,"blocked")).GetArray()){int id=Int<int>(x);if(id<=0||!seen.insert(id).second)Fail("blocked","invalid_or_duplicate");out.blocked.push_back(id);}
  unsigned budget=0;for(const auto &row:Seq(At(v,"groups")).GetArray()){
    const auto &key=At(row,"key");if(!key.IsString()||key.GetStringLength()>4096)Fail("innate_group","key");
    const std::string name(key.GetString(),key.GetStringLength());if(out.groups.count(name))Fail("innate_group","duplicate");auto &group=out.groups[name];
    for(const auto &p:Seq(At(row,"persistent")).GetArray()){if(++budget>16384)Fail("innate","total_budget");int id=Int<int>(At(p,"id")),count=Int<int>(At(p,"count"));if(id<=0||count<=0||!group.persistent.emplace(id,count).second)Fail("innate_persistent","invalid_or_duplicate");}
    std::map<int,std::int64_t> totals;for(const auto &p:Seq(At(row,"temporary")).GetArray()){
      if(++budget>16384)Fail("innate","total_budget");ClientPlayerState::Timed t{Int<int>(At(p,"id")),Int<int>(At(p,"count")),Int<int>(At(p,"timer"))};
      if(t.id<=0||t.count<=0||(totals[t.id]+=t.count)>INT32_MAX)Fail("innate_temporary","invalid_or_overflow");group.temporary.push_back(t);}
  }
}
}
bool ClientPlayerStateApplicable(const std::string &id,int type) noexcept {
  if(id=="EntityPlayer.GetPoopMana")return type==25;
  if(id=="NativePlayer.MaggyDrainCooldown"||id=="NativePlayer.MaggySwingCooldown")return type==22;
  if(id=="NativePlayer.EveSumptoriumCharge")return type==26;
  if(id=="EntityPlayer.GetSoulCharge")return type==18;
  if(id=="EntityPlayer.GetBloodCharge")return type==36;
  return true;
}
const char *ClientPlayerStateDisposition(const std::string &id) noexcept {
  if(Rules().count(id))return "I11-I13:exact-final-native-storage";
  if(id=="EntityPlayer.GetCollectiblesList"||id=="EntityPlayer.GetVoidedCollectiblesList"||id=="EntityPlayer.GetSmeltedTrinkets"||id=="EntityPlayer.GetTrinket")return "I14:complete-native-containers;not-History";
  if(id=="EntityPlayer.GetActiveItemDesc"||id=="EntityPlayer.GetPocketItem"||id=="EntityPlayer.QueuedItem")return "I15:four-slots-and-local-config;never-Use-or-Queue";
  return nullptr;
}
std::shared_ptr<const ClientPlayerState> ClientDescribePlayerState(const replica::EntityObjectTarget &o){
  const auto &body=std::get<replica::EntityClassValues<replica::EntityCategory::Player>>(o.payload).fields.value;
  const auto &source=At(body,"application_player_state");
  if(!Is(source,"status","Value")||Int<unsigned>(At(source,"version"))!=1||!Is(source,"scope","same-ordinary-window"))Fail("source","current_complete_v1_required");
  auto state=std::make_shared<ClientPlayerState>();state->player_type=Int<int>(At(source,"player_type"));if(state->player_type<0||state->player_type>40)Fail("player_type","range");
  const auto &values=At(source,"values");if(!values.IsObject()||values.MemberCount()!=Rules().size())Fail("values","complete_named_storage_required");
  for(const auto &[id,rule]:Rules()){
    const auto &v=At(values,id.c_str());ClientEntityField f;f.member=id;f.kind=rule.kind;
    if(!ClientPlayerStateApplicable(id,state->player_type)){if(!Is(v,"status","NotApplicable"))Fail(id,"role_requires_NotApplicable");state->not_applicable.push_back(id);continue;}
    if(f.kind==ClientEntityValueKind::Integer){f.integer=Int<std::int64_t>(v);if(f.integer<rule.low||f.integer>rule.high)Fail(id,"native_width");}
    else if(f.kind==ClientEntityValueKind::Boolean){if(!v.IsBool())Fail(id,"boolean");f.boolean=v.GetBool();}
    else if(f.kind==ClientEntityValueKind::Bits128){const auto &words=Seq(v,4);if(words.Size()!=4)Fail(id,"four_u32_required");f.bits=Int<std::uint32_t>(words[0])|(std::uint64_t(Int<std::uint32_t>(words[1]))<<32);f.high_bits=Int<std::uint32_t>(words[2])|(std::uint64_t(Int<std::uint32_t>(words[3]))<<32);}
    else if(f.kind==ClientEntityValueKind::Color){const auto &c=Seq(v,11);if(c.Size()!=11)Fail(id,"eleven_f32_required");for(unsigned i=0;i<4;++i){f.color.tint[i]=Real(c[i]);f.color.colorize[i]=Real(c[4+i]);}for(unsigned i=0;i<3;++i)f.color.offset[i]=Real(c[8+i]);}
    else f.real=Real(v);
    state->fields.push_back(std::move(f));
  }
  // 公开直接属性与同窗口backing必须一致；共享/计算型getter不套用这一等式。
  const auto &public_fields=At(body,"fields");
  for(const auto &f:state->fields)if((f.member=="EntityPlayer.SamsonBerserkCharge"||f.member=="EntityPlayer.IBSCharge")&&Int<std::int32_t>(At(public_fields,f.member.c_str()))!=f.integer)Fail(f.member,"same_window_public_storage_conflict");
  for(const auto &f:state->fields)if(f.member=="EntityPlayer.Damage"||f.member=="EntityPlayer.MoveSpeed"||f.member=="EntityPlayer.MaxFireDelay"||f.member=="EntityPlayer.ShotSpeed"||f.member=="EntityPlayer.TearRange"||f.member=="EntityPlayer.TearHeight"||f.member=="EntityPlayer.TearFallingSpeed"||f.member=="EntityPlayer.TearFallingAcceleration"||f.member=="EntityPlayer.Luck"){
    if(Real(At(public_fields,f.member.c_str()))!=f.real)Fail(f.member,"same_window_public_storage_conflict");}
  state->collectibles=Integers(At(source,"collectibles"));state->voided=Integers(At(source,"voided"));
  for(const auto &row:Seq(At(source,"wisps")).GetArray()){int id=Int<int>(At(row,"id")),count=Int<int>(At(row,"count"));if(id<=0||count<0||!state->wisps.emplace(id,count).second)Fail("wisps","id_count_or_duplicate");}
  for(const auto &row:Seq(At(source,"smelted")).GetArray())state->smelted.push_back({Int<std::int16_t>(At(row,"normal")),Int<std::int16_t>(At(row,"golden"))});
  const auto &trinkets=Seq(At(source,"trinkets"),2),&active=Seq(At(source,"active"),4),&pocket=Seq(At(source,"pocket"),4);
  if(trinkets.Size()!=2||active.Size()!=4||pocket.Size()!=4)Fail("slots","complete_domain_required");
  for(unsigned i=0;i<2;++i)state->trinkets[i]=Int<unsigned>(trinkets[i]);
  for(unsigned i=0;i<4;++i){const auto &v=active[i];auto &a=state->active[i];a.item=Int<int>(At(v,"item"));a.item_pool=Int<int>(At(v,"item_pool"));a.charge=Int<int>(At(v,"charge"));a.battery=Int<int>(At(v,"battery"));a.sub=Int<int>(At(v,"sub"));a.cooldown=Int<int>(At(v,"cooldown"));a.partial=Real(At(v,"partial"));a.var_data=Int<int>(At(v,"var_data"));
    if(a.item==703&&a.var_data){if(a.var_data<1||a.var_data>4)Fail("active_esau_jr","backup_slot_range");a.backup_slot=a.var_data-1;}
    auto &p=state->pocket[i];p.id=Int<int>(At(pocket[i],"id"));p.type=Int<unsigned>(At(pocket[i],"type"));
    if(p.id<0||(p.id&&p.type>2)||(p.id&&p.type==2&&(p.id<3||p.id>4)))Fail("pocket","kind_or_active_slot");}
  const auto &q=At(source,"queue");state->queue={Int<int>(At(q,"item")),Int<int>(At(q,"type")),Int<int>(At(q,"charge")),Int<int>(At(q,"var_data")),Int<unsigned>(At(q,"flags"))};
  if(state->queue.type<0||state->queue.type>4||(state->queue.item&&state->queue.type==0))Fail("queue","item_type_or_id");
  Spoof(At(source,"collectible_spoof"),state->spoofs[0]);Spoof(At(source,"trinket_spoof"),state->spoofs[1]);return state;
}
ClientPlayerDescription ClientPlayerFastDescription(const ClientPlayerDescription &base,const ClientPlayerFastValues &fast){
  ClientPlayerDescription out=base;auto state=std::make_shared<ClientPlayerState>(*base.state);
  for(const auto &v:fast.player){auto old=std::find_if(state->fields.begin(),state->fields.end(),[&](const auto &x){return x.member==v.member;});if(old==state->fields.end())Fail(v.member,"fast_backing_missing");*old=v;}
  for(const auto &v:fast.common){auto old=std::find_if(out.entity.common_fields.begin(),out.entity.common_fields.end(),[&](const auto &x){return x.member==v.member;});if(old==out.entity.common_fields.end())Fail(v.member,"fast_common_missing");*old=v;
    if(v.member=="Entity.Position")out.entity.position=v.vector;else if(v.member=="Entity.Velocity")out.entity.velocity=v.vector;else if(v.member=="Entity.PositionOffset")out.entity.position_offset=v.vector;
    else if(v.member=="Entity.Visible")out.entity.visible=v.boolean;else if(v.member=="Entity.Exists")out.entity.exists=v.boolean;else if(v.member=="Entity.IsDead")out.entity.dead=v.boolean;
    else if(v.member=="Entity.DepthOffset")out.entity.depth=v.real;else if(v.member=="Entity.Size")out.entity.size=v.real;else if(v.member=="Entity.Mass")out.entity.mass=v.real;else if(v.member=="Entity.CollisionDamage")out.entity.collision_damage=v.real;
    else if(v.member=="Entity.EntityCollisionClass")out.entity.entity_collision=int(v.integer);else if(v.member=="Entity.GridCollisionClass")out.entity.grid_collision=int(v.integer);}
  out.state=std::move(state);out.equipment=fast.render;out.entity.resource=fast.sprite;return out;
}
std::vector<ClientEntityField> ClientStreamPlayerFields(const V &body,int type){std::vector<ClientEntityField> out;const auto &fields=At(body,"fields");
  // 共享/转发Getter不是本身体backing。心量、货币、充能等私有值仍只由完整ordinary安装。
  static const std::set<std::string> direct={"EntityPlayer.Damage","EntityPlayer.MoveSpeed","EntityPlayer.MaxFireDelay","EntityPlayer.ShotSpeed","EntityPlayer.TearRange","EntityPlayer.TearHeight","EntityPlayer.TearFallingSpeed","EntityPlayer.TearFallingAcceleration","EntityPlayer.Luck","EntityPlayer.CanFly","EntityPlayer.SamsonBerserkCharge","EntityPlayer.IBSCharge","EntityPlayer.TearFlags","EntityPlayer.TearColor","EntityPlayer.LaserColor","EntityPlayer.GetMovementDirection","EntityPlayer.GetHeadDirection","EntityPlayer.GetDamageCooldown"};
  for(const auto &[id,rule]:Rules()){if(!direct.contains(id)||!fields.HasMember(id.c_str())||!ClientPlayerStateApplicable(id,type))continue;const auto &v=fields[id.c_str()];if(Is(v,"status","NotApplicable"))continue;ClientEntityField f;f.member=id;f.kind=rule.kind;
    if(f.kind==ClientEntityValueKind::Integer){f.integer=Int<std::int64_t>(v);if(f.integer<rule.low||f.integer>rule.high)Fail(id,"native_width");}
    else if(f.kind==ClientEntityValueKind::Real)f.real=Real(v);
    else if(f.kind==ClientEntityValueKind::Boolean){if(!v.IsBool())Fail(id,"bool");f.boolean=v.GetBool();}
    else if(f.kind==ClientEntityValueKind::Bits128){if(!Is(v,"type","bits128"))Fail(id,"bits128");const auto bits=[](const V &x){const auto &s=Is(x,"type","int64")?At(x,"value"):x;if((s.IsInt64()&&s.GetInt64()<0)||(s.IsString()&&s.GetStringLength()&&s.GetString()[0]=='-'))return std::uint64_t(Int<std::int64_t>(x));return Int<std::uint64_t>(x);};f.bits=bits(At(v,"low"));f.high_bits=bits(At(v,"high"));}
    else if(f.kind==ClientEntityValueKind::Color){const char *t[]={"R","G","B","A"},*o[]={"RO","GO","BO"};for(unsigned i=0;i<4;++i)f.color.tint[i]=Real(At(v,t[i]));for(unsigned i=0;i<3;++i)f.color.offset[i]=Real(At(v,o[i]));const auto &entries=At(At(v,"GetColorize"),"entries");if(!entries.IsArray()||entries.Size()!=4)Fail(id,"colorize");unsigned mask=0;for(const auto &e:entries.GetArray()){const auto &key=At(e,"key");unsigned i=0;while(i<4&&(!key.IsString()||std::strcmp(key.GetString(),t[i])))++i;if(i==4||(mask&(1u<<i)))Fail(id,"colorize_keys");mask|=1u<<i;f.color.colorize[i]=Real(At(e,"value"));}}
    else continue;out.push_back(std::move(f));
  }return out;
}
} // namespace cs::game
