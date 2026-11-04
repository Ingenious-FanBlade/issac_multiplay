#include "game/client_entity_model.h"
#include "game/client_resource_model.h"
#include "game/client_entity_adapters.h"
#include <charconv>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <string_view>
#include <type_traits>

namespace cs::game {
namespace {
using V=replica::Value;
[[noreturn]] void Fail(const std::string &member,const char *why){throw replica::ContractError("entity_common:"+member+":"+why);}
const V &At(const V &v,const char *key){if(!v.IsObject()||!v.HasMember(key))Fail(key,"missing");return v[key];}
bool Is(const V &v,const char *key,const char *text){return v.IsObject()&&v.HasMember(key)&&v[key].IsString()&&std::string_view(v[key].GetString(),v[key].GetStringLength())==text;}
const V &Scalar(const V &v){return Is(v,"type","int64")?At(v,"value"):v;}
template<class T> T Integer(const V &v){const auto &s=Scalar(v);T n{};
  if(s.IsString()){const auto r=std::from_chars(s.GetString(),s.GetString()+s.GetStringLength(),n);if(r.ec==std::errc{}&&r.ptr==s.GetString()+s.GetStringLength())return n;}
  if constexpr(std::is_signed_v<T>){if(s.IsInt64())return T(s.GetInt64());}else if(s.IsUint64())return T(s.GetUint64());Fail("integer","range_or_type");}
float Real(const V &v){const auto &s=Scalar(v);const double n=s.IsNumber()?s.GetDouble():double(Integer<std::int64_t>(v));
  if(!std::isfinite(n)||std::abs(n)>std::numeric_limits<float>::max())Fail("real","nonfinite_or_range");return float(n);}
ClientPoseVector Vector(const V &v){return {Real(At(v,"x")),Real(At(v,"y"))};}
ClientPoseColor Color(const V &v){ClientPoseColor out;const char *tint[]={"R","G","B","A"},*offset[]={"RO","GO","BO"};
  for(unsigned i=0;i<4;++i)out.tint[i]=Real(At(v,tint[i]));for(unsigned i=0;i<3;++i)out.offset[i]=Real(At(v,offset[i]));
  const auto &entries=At(At(v,"GetColorize"),"entries");if(!entries.IsArray()||entries.Size()!=4)Fail("color","colorize");unsigned mask=0;
  for(const auto &row:entries.GetArray()){const auto &key=At(row,"key");if(!key.IsString())Fail("color","key");unsigned i=0;while(i<4&&std::string(key.GetString())!=tint[i])++i;
    if(i==4||(mask&(1u<<i)))Fail("color","duplicate_or_unknown_key");mask|=1u<<i;out.colorize[i]=Real(At(row,"value"));}return out;}
const std::map<std::string,ClientEntityValueKind> &Installed(){static const std::map<std::string,ClientEntityValueKind> map={
#define CS_ENTITY_FIELD(id,field,kind) {id,ClientEntityValueKind::kind},
#define CS_ENTITY_RAW_FIELD(id,offset,type,kind) {id,ClientEntityValueKind::kind},
#define CS_ENTITY_SPECIAL_FIELD(id,kind) {id,ClientEntityValueKind::kind},
#include "game/client_entity_fields.inc"
#undef CS_ENTITY_FIELD
#undef CS_ENTITY_RAW_FIELD
#undef CS_ENTITY_SPECIAL_FIELD
  };return map;}
}
const char *ClientEntityCommonDisposition(const std::string &id) noexcept {
  if(Installed().count(id))return "native-final-value";
  if(id=="Entity.Type"||id=="Entity.Variant"||id=="Entity.SubType"||id=="Entity.InitSeed")return "shape-init;source-seed-never-local-identity";
  if(id=="Entity.Parent"||id=="Entity.Child"||id=="Entity.Target"||id=="Entity.SpawnerEntity"||id=="Entity.GetMinecart")return "typed-reference;I09";
  if(id=="Entity.GetSprite")return "resource-owner;I10";
  // Source身份、查询结论和未来模拟输入完整留在common_values，不能写入本地身份/重新执行查询。
  if(id=="Entity.Index"||id=="Entity.GetHitListIndex"||id=="Entity.DropSeed"||id=="Entity.SpawnGridIndex"||id=="Entity.SpawnerType"||id=="Entity.SpawnerVariant")return "authority-fact;never-local-index-or-RNG";
  if(id=="Entity.GetColorParams")return "authority-color-timeline;install-current-Color;no-local-progression";
  if(id=="Entity.GetWaterClipFlags")return "class-waterclip-owner;I25";
  if(id=="Entity.GetEntityConfigEntity")return "class-config-owner;I11-I22";
  if(id=="Entity.GetPosVel")return "authority-interpolation-value;current-Position-Velocity-installed";
  static const std::set<std::string> results={"Entity.GetLastChild","Entity.GetLastParent","Entity.CanDevolve","Entity.CanShutDoors","Entity.GetBossID","Entity.HasFullHealth","Entity.HasMortalDamage","Entity.IsBoss","Entity.IsEnemy","Entity.IsFlying","Entity.IsVisible","Entity.IsVulnerableEnemy","Entity.ComputeStatusEffectDuration","Entity.IgnoreEffectFromFriendly"};
  if(results.count(id))return "authority-query-result;no-client-gameplay-consumer";
  return nullptr;
}
ClientObjectDescription ClientDescribeEntity(const replica::EntityObjectTarget &o,const std::vector<replica::EntityDependency> &dependencies,std::shared_ptr<const std::string> source,bool player_structure){
  ClientObjectDescription d;d.key={o.key.run,o.key.id,o.key.space};d.shape_revision=o.shape_revision;d.category=unsigned(o.category);
  d.full_entity=true;d.exists=o.exists;d.dead=o.dead;d.common_values=replica::Text(o.common.value);d.special_values=replica::Text(o.special.value);
  d.component_values=std::move(source);
  std::visit([&](const auto &v){d.class_values=replica::Text(v.fields.value);},o.payload);
  if(o.presence!=replica::EntityMemberPresence::Current)return d;
  const auto &fields=At(o.common.value,"fields");
  const auto u32=[&](const char *id){const auto n=Integer<std::uint64_t>(At(fields,id));if(n>UINT32_MAX)Fail(id,"u32");return unsigned(n);};
  d.type=u32("Entity.Type");d.variant=u32("Entity.Variant");d.subtype=u32("Entity.SubType");d.seed=u32("Entity.InitSeed");
  d.authority_index=u32("Entity.Index");
  // Spawn语义在Init前已有位置/速度；正式类别不能继承池槽上一use的坐标。
  d.position=Vector(At(fields,"Entity.Position"));d.velocity=Vector(At(fields,"Entity.Velocity"));
  for(const auto &r:dependencies)if(r.source==o.key){ClientEntityRelation relation;relation.member=r.member;relation.path=r.path;relation.contract=r.contract;
    relation.target=r.has_target?ClientObjectKey{r.target.run,r.target.id,r.target.space}:d.key;relation.expected=unsigned(r.expected);relation.kind=unsigned(r.kind);relation.is_null=r.is_null;
    relation.before_init=r.phase==replica::EntityDependencyPhase::BeforeInit;relation.value_only=r.phase==replica::EntityDependencyPhase::ValueOnly;d.relations.push_back(std::move(relation));}
  // Player20继续下方完整公共解码；19普通Isaac保留原来的窄字段/绘制路径。
  if(o.category==replica::EntityCategory::Player&&!player_structure){
    ClientDescribeEntityPlayerDraw(o,d);
    ClientEntityField frame;frame.member="Entity.FrameCount";frame.kind=ClientEntityValueKind::Integer;frame.integer=Integer<std::int64_t>(At(fields,"Entity.FrameCount"));
    if(frame.integer<INT32_MIN||frame.integer>INT32_MAX)Fail("Entity.FrameCount","i32");d.common_fields.push_back(std::move(frame));
    // 只准备普通Isaac依赖的窄只读核对；其它形态由外部Player20的player_ready具名解释。
    const auto &body=std::get<replica::EntityClassValues<replica::EntityCategory::Player>>(o.payload).fields.value;
    const auto &b=At(body,"fields");const auto player_type=b.FindMember("EntityPlayer.GetPlayerType");
    if(player_type==b.MemberEnd()||Integer<std::int64_t>(player_type->value)!=0)return d;
    d.player_stats=true;d.position=Vector(At(fields,"Entity.Position"));d.velocity=Vector(At(fields,"Entity.Velocity"));d.position_offset=Vector(At(fields,"Entity.PositionOffset"));
    d.depth=Real(At(fields,"Entity.DepthOffset"));d.size=Real(At(fields,"Entity.Size"));d.mass=Real(At(fields,"Entity.Mass"));d.collision_damage=Real(At(fields,"Entity.CollisionDamage"));
    const auto i32=[](const V &v){const auto n=Integer<std::int64_t>(v);if(n<INT32_MIN||n>INT32_MAX)Fail("player","integer_range");return int(n);};
    d.entity_collision=i32(At(fields,"Entity.EntityCollisionClass"));d.grid_collision=i32(At(fields,"Entity.GridCollisionClass"));
    const auto &visible=At(fields,"Entity.Visible");if(!visible.IsBool())Fail("player","visible");d.visible=visible.GetBool();
    d.damage=Real(At(b,"EntityPlayer.Damage"));d.move_speed=Real(At(b,"EntityPlayer.MoveSpeed"));d.max_fire_delay=Real(At(b,"EntityPlayer.MaxFireDelay"));
    d.shot_speed=Real(At(b,"EntityPlayer.ShotSpeed"));d.tear_range=Real(At(b,"EntityPlayer.TearRange"));d.luck=Real(At(b,"EntityPlayer.Luck"));
    d.hearts=i32(At(b,"EntityPlayer.GetHearts"));d.max_hearts=i32(At(b,"EntityPlayer.GetMaxHearts"));d.soul_hearts=i32(At(b,"EntityPlayer.GetSoulHearts"));
    d.black_hearts=i32(At(b,"EntityPlayer.GetBlackHearts"));d.eternal_hearts=i32(At(b,"EntityPlayer.GetEternalHearts"));d.coins=i32(At(b,"EntityPlayer.GetNumCoins"));
    d.keys=i32(At(b,"EntityPlayer.GetNumKeys"));d.bombs=i32(At(b,"EntityPlayer.GetNumBombs"));std::string reason;
    // 19的普通Isaac支撑复用17执行器；必须显式传递四个Null，零初始化的缺席不能冒充空引用。
    const char *primary[]={"Entity.Parent","Entity.Child","Entity.Target","Entity.SpawnerEntity"};
    for(unsigned i=0;i<d.references.size();++i){const auto &v=At(fields,primary[i]);auto &r=d.references[i];
      if(v.IsNull()||Is(v,"status","Null")){r.present=true;r.is_null=true;}
      else if(Is(v,"type","reference")&&Is(v,"status","KnownCurrent")){const auto &key=At(v,"target");r.present=true;
        r.target={Integer<std::uint64_t>(At(key,"run")),Integer<std::uint64_t>(At(key,"id")),Integer<unsigned>(At(key,"space"))};}
    }
    if(!ClientParseResourcePose(At(fields,"Entity.GetSprite"),d.resource,reason))Fail("player",reason.c_str());return d;
  }
  for(const auto &field:Installed())if(!fields.HasMember(field.first.c_str()))Fail(field.first,"required_native_field_missing");
  for(const char *id:{"Entity.Parent","Entity.Child","Entity.Target","Entity.SpawnerEntity","Entity.GetMinecart"})if(!fields.HasMember(id))Fail(id,"required_reference_missing");
  for(auto i=fields.MemberBegin();i!=fields.MemberEnd();++i){const std::string id(i->name.GetString(),i->name.GetStringLength());const auto *policy=replica::EntityApplicationPolicy(id);
    if(!policy||std::string((*policy)["ownership"].GetString())!="ReplicatedState")continue;
    const auto *route=ClientEntityCommonDisposition(id);if(!route)Fail(id,"common_policy_missing");
    const auto entry=Installed().find(id);if(entry==Installed().end())continue;
    const auto &v=i->value;if(Is(v,"status","NotApplicable"))continue;
    ClientEntityField f;f.member=id;f.kind=entry->second;
    switch(f.kind){
      case ClientEntityValueKind::Integer:{const auto n=Integer<std::int64_t>(v);if(n<INT32_MIN||n>INT32_MAX)Fail(id,"i32");f.integer=n;break;}
      case ClientEntityValueKind::Bits:{const auto &s=Scalar(v);f.bits=s.IsString()&&s.GetStringLength()&&s.GetString()[0]=='-'?std::uint64_t(Integer<std::int64_t>(v)):s.IsInt64()&&s.GetInt64()<0?std::uint64_t(s.GetInt64()):Integer<std::uint64_t>(v);break;}
      case ClientEntityValueKind::Real:f.real=Real(v);break;
      case ClientEntityValueKind::Boolean:if(!v.IsBool())Fail(id,"boolean");f.boolean=v.GetBool();break;
      case ClientEntityValueKind::Vector:f.vector=Vector(v);break;
      case ClientEntityValueKind::Color:f.color=Color(v);break;
    }d.common_fields.push_back(std::move(f));
  }
  if(!player_structure)ClientDescribeEntityClass(o,d);return d;
}
} // namespace cs::game
