#include "game/client/entity/client_entity_adapters.h"
#include "game/client/resources/client_resource_model.h"
#include "replica/entity_target.h"
#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <type_traits>
#include <algorithm>
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
  if(!std::strcmp(type,"std::int8_t")){low=INT8_MIN;high=INT8_MAX;}else if(!std::strcmp(type,"std::int16_t")){low=INT16_MIN;high=INT16_MAX;}else if(!std::strcmp(type,"std::uint8_t")){low=0;high=UINT8_MAX;}else if(!std::strcmp(type,"std::uint16_t")){low=0;high=UINT16_MAX;}else if(!std::strcmp(type,"std::uint32_t")){low=0;high=UINT32_MAX;}
  return {category,kind,low,high};}
const std::map<std::string,Rule> &Rules(){static const std::map<std::string,Rule> rules={
#define CS_CLASS_FIELD(category,id,offset,type,kind) {id,Storage(category,ClientEntityValueKind::kind,#type)},
#include "game/client/entity/client_entity_class_fields.inc"
#undef CS_CLASS_FIELD
};return rules;}
bool Authority(const std::string &id){static const std::set<std::string> facts={
  "EntityNPC.CalcTargetPosition","EntityNPC.CanReroll","EntityNPC.CanShutDoors","EntityNPC.GetAliveEnemyCount","EntityNPC.GetHitList","EntityNPC.Pathfinder",
  "EntityNPC.GetPlayerTarget","EntityNPC.GetSirenPlayerEntity","EntityNPC.IsBoss","EntityNPC.IsBossColor","EntityNPC.IsChampion","EntityNPC.QueryNPCsGroup","EntityNPC.QueryNPCsSpawnerType","EntityNPC.QueryNPCsType",
  "EntityDelirium.GetTeleportationTimer","EntityDelirium.IsRedMode","EntityTear.GetHitList","EntityBomb.GetHitList","EntityBomb.IsLoadingCostumes",
  "EntityLaser.GetHitList","EntityLaser.CalculateEndPoint","EntityLaser.GetRenderZ","EntityLaser.IsCircleLaser","EntityLaser.SampleLaser",
  "EntityKnife.GetHitList","EntityKnife.GetRenderZ",
  "EntityPickup.CanReroll","EntityPickup.GetCoinValue","EntityPickup.GetFlipCollectible","EntityPickup.HasFlipData","EntityPickup.IsShopItem","EntityPickup.GetAlternatePedestal",
  "EntityFamiliar.CanBeDamagedByEnemies","EntityFamiliar.CanBeDamagedByLasers","EntityFamiliar.CanBeDamagedByProjectiles","EntityFamiliar.CanBlockProjectiles","EntityFamiliar.CanCharm","EntityFamiliar.IsCharmed",
  "EntityFamiliar.GetFollowerPriority","EntityFamiliar.GetActiveWeaponEntity","EntityFamiliar.GetActiveWeaponNumFired","EntityFamiliar.GetItemConfig","EntityFamiliar.GetMultiplier","EntityFamiliar.GetPathfinder","EntityFamiliar.GetWeapon",
  "EntityEffect.IsPlayerCreep"};return facts.count(id)!=0;}
bool Relation(const std::string &id){static const std::set<std::string> relations={"EntityNPC.ParentNPC","EntityNPC.ChildNPC","EntityNPC.EntityRef","EntityNPC.GetSirenPlayerEntity","EntityTear.StickTarget",
  "EntityLaser.BounceLaser","EntityKnife.GetHitboxParentKnife","EntityFamiliar.Player","EntityPickup.GetPickupGhost","EntityPickup.GetMegaChestLeftCollectible","EntityPickup.GetMegaChestRightCollectible","EntityEffect.GetGridEntityDesc"};return relations.count(id)!=0;}
bool Resource(const std::string &id){return id=="EntityTear.GetDeadEyeSprite"||id=="EntityTear.GetTearEffectSprite"||id=="EntityTear.GetTearHaloSprite"||id=="EntityBomb.GetCostumeLayerSprite"||id=="EntityPickup.GetPriceSprite"||id=="EntitySlot.GetPrizeSprite";}
bool Collection(const std::string &id){return id=="EntityLaser.HomingLaser"||id=="EntityLaser.GetSamples"||id=="EntityLaser.GetNonOptimizedSamples"||id=="EntityPickup.GetCollectibleCycle"||id=="EntityEffect.GetTearFlags";}
void ParseFields(const V &fields,unsigned category,ClientEntityClassState &state){
  for(auto it=fields.MemberBegin();it!=fields.MemberEnd();++it){const std::string id(it->name.GetString());const auto rule=Rules().find(id);
    if(rule==Rules().end()){const auto *policy=replica::EntityApplicationPolicy(id);if(policy&&std::string((*policy)["ownership"].GetString())!="ReplicatedState")continue;
      if(id=="EntityTear.Bounced"){if(!Is(it->value,"status","Null")&&!Is(it->value,"status","NotApplicable"))Fail(id,"removed_since_Repentance");continue;}
      if(Authority(id)||Relation(id)||Resource(id)||Collection(id)||id=="EntityNPC.GetFlyingOverride"||id=="NativeText.text"||id=="NativeText.draw_color"||id=="NativeText.draw_phase")continue;Fail(id,"disposition_missing");}
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
// 原生补充数组不带图节点；公开Lua容器按完整、连续下标解释，不忽略表洞/重复项。
std::vector<const V *> Sequence(const V &v,unsigned limit){std::vector<const V *> out;
  if(v.IsArray()){if(v.Size()>limit)Fail("collection","budget");for(const auto &x:v.GetArray())out.push_back(&x);return out;}
  if(v.IsObject()&&v.MemberCount()==0)return out;
  if(Is(v,"kind","array"))return Sequence(At(v,"values"),limit);
  if(Is(v,"type","table")){const auto &entries=At(v,"entries");if(entries.IsObject()&&entries.MemberCount()==0)return out;
    if(!entries.IsArray()||entries.Size()>limit)Fail("collection","table_budget");std::map<unsigned,const V *> items;
    for(const auto &e:entries.GetArray()){const auto i=Integer<unsigned>(At(e,"key"));if(!i||i>limit||!items.emplace(i,&At(e,"value")).second)Fail("collection","index");}
    for(const auto &e:items){if(e.first!=out.size()+1)Fail("collection","hole");out.push_back(e.second);}return out;}
  Fail("collection","explicit_sequence_required");}
ClientPoseVector Point(const V &v){return {Real(At(v,"x")),Real(At(v,"y"))};}
std::vector<ClientPoseVector> Points(const V &v){std::vector<ClientPoseVector> out;for(const auto *p:Sequence(v,16384))out.push_back(Point(*p));return out;}
void Geometry(const V &v,ClientEntityClassState &state){if(Integer<unsigned>(At(v,"version"))!=1||!Is(v,"status","Value"))Fail("laser_geometry","version_or_status");
  auto g=std::make_shared<ClientEntityClassState::LaserGeometry>();g->origin=Point(At(v,"origin"));g->end=Point(At(v,"end"));g->parameter=Real(At(v,"parameter"));g->spacing=Real(At(v,"spacing"));
  g->sample_count=Integer<std::uint32_t>(At(v,"sample_count"));g->cursor=Integer<std::int32_t>(At(v,"cursor"));g->optimized=Points(At(v,"optimized"));g->unoptimized=Points(At(v,"unoptimized"));
  for(const auto *i:Sequence(At(v,"indices"),16384))g->indices.push_back(Integer<std::int32_t>(*i));
  for(const auto *b:Sequence(At(v,"bezier"),16384)){const auto values=Sequence(*b,6);if(values.size()!=6)Fail("laser_bezier","six_components_required");std::array<float,6> row{};for(unsigned i=0;i<6;++i)row[i]=Real(*values[i]);g->bezier.push_back(row);}state.laser=std::move(g);}
// 私有表现owner明确区分None/具体类型；索引、采样record和每个实际ANM一并形成正式来源。
void Owned(const V &v,ClientObjectDescription &d,ClientEntityClassState &state){using P=ClientEntityClassState::OwnedPresentation;
  if(Integer<unsigned>(At(v,"version"))!=1||!Is(v,"status","Value"))Fail("owned_presentation","version_or_unavailable");
  const auto &kind=At(v,"kind");if(!kind.IsString())Fail("owned_presentation","kind");const std::map<std::string,P::Kind> kinds={{"None",P::None},{"MrMe",P::MrMe},{"SpriteTrail",P::SpriteTrail},{"PickupGhost",P::PickupGhost},{"IsaacsBody",P::IsaacsBody},{"AbyssLocust",P::AbyssLocust}};
  const auto found=kinds.find(kind.GetString());auto &o=state.owned;
  if(std::string(kind.GetString())=="AuthorityOnly"){
    if((d.category==8&&(d.variant==48||d.variant==231))||(d.category==9&&(d.variant==108||d.variant==166||d.variant==191)))Fail("owned_presentation","render_payload_cannot_be_authority_only");o.authority_only=true;
  }else {if(found==kinds.end())Fail("owned_presentation","unknown_type");o.kind=found->second;}
  if(o.kind!=P::None&& !((d.category==9&&((o.kind==P::MrMe&&d.variant==108)||(o.kind==P::SpriteTrail&&d.variant==166)||(o.kind==P::PickupGhost&&d.variant==191)))||
       (d.category==8&&((o.kind==P::IsaacsBody&&d.variant==48)||(o.kind==P::AbyssLocust&&d.variant==231)))))Fail("owned_presentation","type_variant_conflict");
  unsigned expected=o.kind==P::MrMe?1:o.kind==P::PickupGhost?4:o.kind==P::IsaacsBody?5:0;
  if(o.kind==P::IsaacsBody){const auto indices=Sequence(At(v,"indices"),4);if(indices.size()!=4)Fail("isaacs_body","four_indices");for(unsigned i=0;i<4;++i)o.body_indices[i]=Integer<std::int32_t>(*indices[i]);}
  if(o.kind==P::AbyssLocust){o.sprites_a=Integer<unsigned>(At(v,"sprites_a"));o.sprites_b=Integer<unsigned>(At(v,"sprites_b"));if(o.sprites_a>16384||o.sprites_b>16384||o.sprites_a+o.sprites_b>16384)Fail("abyss_sprites","budget");expected=o.sprites_a+o.sprites_b;}
  const auto sprites=Sequence(At(v,"sprites"),16384);if(sprites.size()!=expected)Fail("owned_presentation","complete_sprite_members_required");for(unsigned i=0;i<sprites.size();++i)Pose(*sprites[i],"owned_sprite",i,state);
  if(o.kind==P::PickupGhost){o.count=Integer<unsigned>(At(v,"count"));if(o.count>4)Fail("pickup_ghost","count");}
  if(o.kind==P::SpriteTrail)for(const auto *row:Sequence(At(v,"trail"),16384)){const auto fields=Sequence(*row,16);if(fields.size()!=16)Fail("sprite_trail","record_size");std::array<std::uint32_t,16> record{};
    for(unsigned i=0;i<16;++i){record[i]=Integer<std::uint32_t>(*fields[i]);float f=0;std::memcpy(&f,&record[i],4);if(!std::isfinite(f))Fail("sprite_trail","nonfinite_record");}o.trail.push_back(record);}
}
// Source当前同窗口的完整描述；不接受字符串截断、猜测编码或拼接独立Render样本。
void TextState(const V &fields,const V &body,ClientEntityClassState &state){
  const auto &color=At(fields,"NativeText.draw_color");if(!Is(color,"status","Value")||!Is(color,"type","native_kcolor20"))Fail("NativeText.draw_color","complete_kcolor20_required");
  const char *keys[]={"R","G","B","A"};for(unsigned i=0;i<4;++i)state.text_color[i]=Real(At(color,keys[i]));state.text_color_extra=Integer<std::int32_t>(At(color,"opaque32"));
  const auto &bytes=At(body,"application_text_bytes");if(!Is(bytes,"status","Value")||Integer<unsigned>(At(bytes,"version"))!=1)Fail("NativeText.text","current_use_bytes_required");
  const auto &hex=At(bytes,"hex");if(!hex.IsString()||hex.GetStringLength()>510||hex.GetStringLength()%2)Fail("NativeText.text","buffer_capacity");
  const auto nibble=[](char c)->int {return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;};
  for(unsigned i=0;i<hex.GetStringLength();i+=2){const int a=nibble(hex.GetString()[i]),b=nibble(hex.GetString()[i+1]);if(a<0||b<0||!(a*16+b))Fail("NativeText.text","hex_or_embedded_nul");state.text.push_back(char(a*16+b));}
  const auto &text=At(fields,"NativeText.text");
  if(text.IsString()){if(std::string(text.GetString(),text.GetStringLength())!=state.text)Fail("NativeText.text","byte_alias_conflict");}
  else if(!Is(text,"type","bytes")||!Is(text,"encoding","lua-string-bytes")||At(text,"hex")!=hex)Fail("NativeText.text","byte_alias_conflict");
}
void DrawInputs(const V &body,ClientEntityClassState &state){
  const auto &v=At(body,"application_draw");if(!Is(v,"status","Value")||Integer<unsigned>(At(v,"version"))!=1||!Is(v,"scope","same-ordinary-window"))Fail("application_draw","same_window_contract");
  auto &d=state.draw;d.manager_frame=Integer<std::uint32_t>(At(v,"manager_frame"));d.game_frame=Integer<std::uint32_t>(At(v,"game_frame"));d.subframe=Integer<unsigned>(At(v,"subframe"));if(d.subframe>1)Fail("application_draw","subframe");
  const auto &paused=At(v,"paused");if(!paused.IsBool())Fail("application_draw","paused");d.paused=paused.GetBool();if(d.subframe!=(d.paused?0u:(d.manager_frame+1u)%2u))Fail("application_draw","phase_alias_conflict");
  const auto random=[&](const char *key,std::array<std::uint32_t,4> &out){const auto values=Sequence(At(v,key),4);if(values.size()!=4)Fail(key,"four_words");for(unsigned i=0;i<4;++i)out[i]=Integer<std::uint32_t>(*values[i]);if(!out[0])Fail(key,"zero_seed");for(unsigned i=1;i<4;++i)if(out[i]>31)Fail(key,"shift_range");};
  random("pickup_random",d.pickup_random);random("familiar_random",d.familiar_random);d.present=true;
}
}
const char *ClientEntityClassDisposition(const std::string &id) noexcept {if(Rules().count(id))return "native-final-value;class-field-table";if(id=="NativeText.text"||id=="NativeText.draw_color")return "current-use-native-text-bytes-and-KColor20;native-storage-and-font-consumer";if(id=="NativeText.draw_phase")return "independent-Render-reference-retained;ordinary-presentation-uses-own-application_draw-v1";if(id=="EntityTear.Bounced")return "removed-since-Repentance;only-Null-or-NotApplicable;never-fabricate-storage";if(id=="EntityNPC.GetFlyingOverride")return "official-Plus-optional-bool;fixed-leaf-Lua-Set-Clear-Get";if(id=="EntityBomb.IsLoadingCostumes")return "authority-lazy-work-request;install-final-five-resources;client-pending=false";if(Authority(id))return "authority-fact;no-client-AI-or-collision-consumer";if(Relation(id))return "typed-reference;class-owner";if(Resource(id))return "independent-native-member-resource;I10";if(Collection(id))return "class-owned-complete-container;variant-qualified;no-gameplay-recalculation";return nullptr;}
void ClientDescribeEntityPlayerDraw(const replica::EntityObjectTarget &o,ClientObjectDescription &d){
  const auto &body=std::get<replica::EntityClassValues<replica::EntityCategory::Player>>(o.payload).fields.value;
  auto state=std::make_shared<ClientEntityClassState>();DrawInputs(body,*state);
  if(body.HasMember("application_player_role")){const auto &role=body["application_player_role"];
    if(Integer<unsigned>(At(role,"version"))!=1||!Is(role,"status","Value")||!Is(role,"scope","same-ordinary-window"))Fail("player_composite","current_role_source_required");state->player_composite_v1=true;}
  if(body.HasMember("application_player_special")){const auto &v=body["application_player_special"];
    if(Integer<unsigned>(At(v,"version"))!=1||!Is(v,"status","Value"))Fail("player_full_draw","current_special_source_required");
    const auto &palette=At(v,"palette");if(!palette.IsArray()||palette.Size()!=11)Fail("player_palette","eleven_floats_required");
    for(unsigned i=0;i<4;++i){state->player_palette.tint[i]=Real(palette[i]);state->player_palette.colorize[i]=Real(palette[4+i]);}for(unsigned i=0;i<3;++i)state->player_palette.offset[i]=Real(palette[8+i]);state->player_full_draw=true;
  }
  // +44c是公开BodySprite的AnimationState；136举持从它的真实null-frame取位置，不是GetHeldSprite。
  Pose(At(At(body,"fields"),"EntityPlayer.GetBodySprite"),"player_body",0,*state);
  const auto &held=At(At(body,"fields"),"EntityPlayer.GetHeldEntity");
  if(!held.IsNull()&&!Is(held,"status","Null")){
    if(!Is(held,"type","reference")||!Is(held,"status","KnownCurrent"))Fail("player_held_entity","current_reference_required");
    state->player_held_draw=true;state->player_held_shape=Integer<std::uint64_t>(At(held,"shape_revision"));
    state->player_held_item_state=Integer<std::int32_t>(At(At(body,"fields"),"EntityPlayer.GetItemState"));
    state->player_held_pose=Integer<std::int32_t>(At(At(body,"fields"),"EntityPlayer.GetSuplexState"));
    // REP公开枚举只列0..4，但J273 78CE75的原生过渡确实写5；保留真实已知状态，不钳成枚举默认值。
    if(state->player_held_pose<0||state->player_held_pose>5)Fail("player_held_entity","pose_range");
  }
  const auto &v=At(body,"application_player_draw");if(Integer<unsigned>(At(v,"version"))!=1||!Is(v,"status","Value"))Fail("player_draw","current_owner_source_required");
  const auto &used=At(v,"minecart_draw");if(!used.IsBool())Fail("player_draw","minecart_bool");state->player_minecart_draw=used.GetBool();
  if(state->player_minecart_draw){state->player_minecart_offset=Point(At(v,"offset"));state->player_minecart_sort_bits=Integer<std::uint32_t>(At(v,"sort_bits"));float y=0;std::memcpy(&y,&state->player_minecart_sort_bits,4);
    if(!std::isfinite(y)||y!=state->player_minecart_offset.y)Fail("player_draw","minecart_sort_alias_conflict");std::memcpy(&state->player_minecart_offset.y,&state->player_minecart_sort_bits,4);}
  d.derived=std::move(state);
}
void ClientDescribeEntityClass(const replica::EntityObjectTarget &o,ClientObjectDescription &d){
  if(d.category<1||d.category>12)return;auto state=std::make_shared<ClientEntityClassState>();state->delirium=o.delirium;
  const V *body=nullptr;std::visit([&](const auto &p){body=&p.fields.value;},o.payload);const auto &fields=At(*body,"fields");ParseFields(fields,d.category,*state);
  // 解析与native表的必需列一致；缺失/不适用不能悄悄留下池上一use的值。
  for(const auto &rule:Rules())if(rule.second.category==d.category){const auto &id=rule.first;
    if(id.rfind("Native",0)==0&&d.category!=11)continue;if(id.rfind("EntityDelirium.",0)==0){if(o.delirium)At(At(o.special.value,"fields"),id.c_str());}
    else At(fields,id.c_str());}
  DrawInputs(*body,*state);if(d.category==11)TextState(fields,*body,*state);
  if(d.category==1){const auto &npc=At(*body,"application_npc");if(Integer<unsigned>(At(npc,"version"))!=1||!Is(npc,"status","Value"))Fail("npc_composite","complete_source_required");
    state->npc_composite_count=Integer<unsigned>(At(npc,"reference_count"));if(state->npc_composite_count>16384)Fail("npc_composite","budget");state->npc_path=Points(At(npc,"path"));state->npc_secondary_path=Points(At(npc,"secondary_path"));}
  if(d.category==1){const auto triplets=[](const auto &p){return p.empty()||(p.size()>=3&&p.size()%3==0);};
    if((d.type==951&&d.variant==3&&state->npc_path.size()>=9&&!triplets(state->npc_path))||
       (d.type==921&&d.variant==0&&!triplets(state->npc_secondary_path)))Fail("npc_curve","complete_native_triplets_required");}
  const auto &links=At(*body,"application_links");if(Integer<unsigned>(At(links,"version"))!=1||!Is(links,"status","Value"))Fail("native_links","unavailable_or_version");
  unsigned link_count=0;
#define CS_CLASS_LINK(cat,id,offset) if(cat==0||cat==d.category){++link_count;const auto &link=At(At(links,"links"),id);if(!Is(link,"status","Null")&&!Is(link,"status","KnownCurrent"))Fail(id,"unresolved_or_invalid_link");}
#include "game/client/entity/client_entity_links.inc"
#undef CS_CLASS_LINK
  if(d.category==1)for(unsigned i=0;i<state->npc_composite_count;++i){const auto id="NativeNPC.Composite:"+std::to_string(i);++link_count;const auto &link=At(At(links,"links"),id.c_str());if(!Is(link,"status","Null")&&!Is(link,"status","KnownCurrent"))Fail(id,"unresolved_composite_member");}
  if(At(links,"links").MemberCount()!=link_count)Fail("native_links","unknown_or_duplicate_member");
  if(o.delirium)ParseFields(At(o.special.value,"fields"),1,*state);
  if(d.category==4){const auto &runtime=At(*body,"application_runtime");if(Integer<unsigned>(At(runtime,"version"))!=1||!Is(runtime,"status","Value"))Fail("bomb_application_runtime","unavailable_or_version");At(At(runtime,"values"),"NativeBomb.CostumeDrawEnabled");ParseFields(At(runtime,"values"),4,*state);}
  if(d.category>=5&&d.category<=9){const auto &runtime=At(*body,"application_runtime");if(Integer<unsigned>(At(runtime,"version"))!=2||!Is(runtime,"status","Value"))Fail("class_application_runtime","unavailable_or_version");
    if(d.category==8||d.category==9){Owned(At(*body,"application_owned"),d,*state);const auto &owner=At(runtime,"private_owner_present");if(!owner.IsBool()||owner.GetBool()!=(state->owned.kind!=ClientEntityClassState::OwnedPresentation::None||state->owned.authority_only))Fail("class_private_owner","typed_owner_alias_conflict");}
    // 此容器只供Authority更新；当前Render没有消费者，保留事实，不复制header或执行Update恢复它。
    if(d.category==9&&!At(runtime,"private_container_nonempty").IsBool())Fail("effect_private_container","bool_required");}
  if(d.category==5){Geometry(At(*body,"application_geometry"),*state);
    if(Real(At(fields,"EntityLaser.Angle"))!=Real(At(fields,"EntityLaser.AngleDegrees")))Fail("EntityLaser.Angle","degrees_alias_conflict");
    const auto &sample=At(fields,"EntityLaser.SampleLaser");if(!sample.IsBool()||sample.GetBool()!=(Integer<std::int8_t>(At(fields,"EntityLaser.sample_mode_raw"))!=0))Fail("EntityLaser.SampleLaser","raw_mode_alias_conflict");
    for(const auto &p:{std::pair{"EntityLaser.GetSamples",&state->laser->optimized},std::pair{"EntityLaser.GetNonOptimizedSamples",&state->laser->unoptimized}}){const auto points=Points(At(fields,p.first));if(points.size()!=p.second->size())Fail(p.first,"native_geometry_alias_count");for(unsigned i=0;i<points.size();++i)if(points[i].x!=(*p.second)[i].x||points[i].y!=(*p.second)[i].y)Fail(p.first,"native_geometry_alias_value");}}
  if(d.category==7){for(const auto *v:Sequence(At(fields,"EntityPickup.GetCollectibleCycle"),8))state->pickup_cycle.push_back(Integer<std::uint32_t>(*v));}
  if(d.category==9){const auto &flags=At(fields,"EntityEffect.GetTearFlags");state->effect_tear_flags=d.variant==54||d.variant==113||d.variant==126||d.variant==167;
    if(state->effect_tear_flags){if(!Is(flags,"type","bits128"))Fail("EntityEffect.GetTearFlags","variant_requires_bits128");ClientEntityField f;f.member="NativeEffect.TearFlags";f.kind=ClientEntityValueKind::Bits128;f.bits=Bits(At(flags,"low"));f.high_bits=Bits(At(flags,"high"));state->fields.push_back(std::move(f));}
    else if(!Is(flags,"status","Null")&&!Is(flags,"status","NotApplicable"))Fail("EntityEffect.GetTearFlags","variant_not_tear_flags");}
  if(d.category==1)for(const auto &alias:{std::pair{"EntityNPC.ParentNPC","Entity.Parent"},std::pair{"EntityNPC.ChildNPC","Entity.Child"}}){
    const ClientEntityRelation *left=nullptr,*right=nullptr;for(const auto &r:d.relations){if(r.member==alias.first)left=&r;if(r.member==alias.second)right=&r;}
    if(!left||!right||left->is_null!=right->is_null||(!left->is_null&&!(left->target==right->target)))Fail(alias.first,"common_reference_alias_conflict");}
  if(d.category==3){const auto &common=At(o.common.value,"fields");if(Real(At(fields,"EntityProjectile.DepthOffset"))!=Real(At(common,"Entity.DepthOffset")))Fail("EntityProjectile.DepthOffset","common_alias_conflict");}
  if(d.category==1){const auto &flying=At(fields,"EntityNPC.GetFlyingOverride");if(Is(flying,"status","Null"))state->flying_override=-1;else if(flying.IsBool())state->flying_override=flying.GetBool()?1:0;else Fail("EntityNPC.GetFlyingOverride","optional_bool");}
  if(d.category==1){const auto &runtime=At(*body,"application_runtime");if(Integer<unsigned>(At(runtime,"version"))!=1||!Is(runtime,"status","Value"))Fail("npc_application_runtime","unavailable_or_version");
    if(Integer<std::uint64_t>(At(runtime,"private_reference_count"))!=state->npc_composite_count)Fail("npc_private_reference_container","count_alias_conflict");
    if(!At(runtime,"owned_actor_present").IsBool())Fail("npc_owned_actor","bool_required;Authority-owned-update-only");}
  if(d.category==1){const auto &resources=At(*body,"application_resources");if(Integer<unsigned>(At(resources,"version"))!=1||!Is(resources,"status","Value"))Fail("npc_application_resources","unavailable_or_version");
    Pose(At(resources,"npc_overlay"),"npc_overlay",0,*state);Pose(At(resources,"npc_effect"),"npc_effect",0,*state);}
  if(d.category==7){const auto &resources=At(*body,"application_resources");if(Integer<unsigned>(At(resources,"version"))!=1||!Is(resources,"status","Value"))Fail("pickup_application_resources","unavailable_or_version");
    Pose(At(fields,"EntityPickup.GetPriceSprite"),"EntityPickup.GetPriceSprite",0,*state);Pose(At(resources,"pickup_alternate"),"pickup_alternate",0,*state);}
  if(d.category==6){const auto &resources=At(*body,"application_resources");if(Integer<unsigned>(At(resources,"version"))!=1||!Is(resources,"status","Value"))Fail("knife_application_resources","unavailable_or_version");Pose(At(resources,"knife_dead_eye"),"knife_dead_eye",0,*state);}
  if(d.category==10)Pose(At(fields,"EntitySlot.GetPrizeSprite"),"EntitySlot.GetPrizeSprite",0,*state);
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
