#include "game/client/player/client_player_equipment.h"
#include "game/client/resources/client_resource_model.h"
#include "replica/player_target.h"
#include "replica/player_limits.h"
#include <charconv>
#include <cmath>
#include <limits>
#include <set>
#include <type_traits>

namespace cs::game {
namespace {
using V=replica::Value;
[[noreturn]] void Fail(const std::string &why){throw replica::ContractError("player_equipment:"+why);}
bool Is(const V &v,const char *key,const char *s){return v.IsObject()&&v.HasMember(key)&&v[key].IsString()&&std::string(v[key].GetString())==s;}
const V &At(const V &v,const char *key){if(!v.IsObject()||!v.HasMember(key))Fail(std::string("missing:")+key);return v[key];}
template<class T=int> T Int(const V &v){const auto &x=Is(v,"type","int64")?At(v,"value"):v;T n{};
  if(x.IsString()){auto r=std::from_chars(x.GetString(),x.GetString()+x.GetStringLength(),n);if(r.ec==std::errc{}&&r.ptr==x.GetString()+x.GetStringLength())return n;}
  if constexpr(std::is_signed_v<T>){if(x.IsInt64()&&x.GetInt64()>=(std::numeric_limits<T>::min)()&&x.GetInt64()<=(std::numeric_limits<T>::max)())return T(x.GetInt64());}
  else if(x.IsUint64()&&x.GetUint64()<=(std::numeric_limits<T>::max)())return T(x.GetUint64());Fail("integer_width");}
bool Bool(const V &v){if(!v.IsBool())Fail("boolean");return v.GetBool();}
float Real(const V &v){if(!v.IsNumber()||!std::isfinite(v.GetDouble())||std::abs(v.GetDouble())>(std::numeric_limits<float>::max)())Fail("finite_float");return float(v.GetDouble());}
std::string String(const V &v){if(!v.IsString()||v.GetStringLength()>4096)Fail("string_budget");return {v.GetString(),v.GetStringLength()};}
// 动画名保留原std::string字节；不假设UTF-8，也不把SSO/堆地址作为协议值。
std::string Bytes(const V &v){const auto hex=String(v);if(hex.size()%2)Fail("head_animation_hex");const auto nibble=[](char c)->int {if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;return -1;};std::string out;
  for(unsigned i=0;i<hex.size();i+=2){const int a=nibble(hex[i]),b=nibble(hex[i+1]);if(a<0||b<0)Fail("head_animation_hex");out.push_back(char(a*16+b));}return out;}
// 图编号只在本组件寻址。展开所需子树时限制总节点与递归，拒绝资源/历史中的循环。
class Graph final {
  std::map<std::uint64_t,const V *> nodes_;
  void Index(const V &v,unsigned depth,unsigned &budget){if(depth>128||!budget--)Fail("graph_index_budget");
    if(Is(v,"type","table")&&v.HasMember("node")&&!nodes_.emplace(Int<std::uint64_t>(v["node"]),&v).second)Fail("duplicate_graph_node");
    if(v.IsArray())for(const auto &x:v.GetArray())Index(x,depth+1,budget);
    else if(v.IsObject())for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)Index(i->value,depth+1,budget);
  }
  V Expand(const V &v,replica::Allocator &a,unsigned depth,unsigned &budget,std::uint64_t &bytes,std::set<const V *> &stack) const {
    if(depth>64||!budget--)Fail("graph_expansion_budget");const V *p=&v;
    if(Is(v,"type","graph_ref")){const auto i=nodes_.find(Int<std::uint64_t>(At(v,"node")));if(i==nodes_.end())Fail("dangling_graph_ref");p=i->second;}
    // 图展开也计实际自有分配；保留64M节点上限，重复引用不能绕过独立字节预算。
    bytes+=sizeof(V)+(p->IsString()?p->GetStringLength():0);
    if(p->IsObject())for(auto i=p->MemberBegin();i!=p->MemberEnd();++i)bytes+=sizeof(V)+i->name.GetStringLength();
    if(bytes>replica::kPlayerTargetBytes)Fail("graph_expansion_byte_budget");
    if(!stack.insert(p).second)Fail("cyclic_value");V out;
    if(p->IsArray()){out.SetArray();for(const auto &x:p->GetArray())out.PushBack(Expand(x,a,depth+1,budget,bytes,stack),a);}
    else if(p->IsObject()){out.SetObject();for(auto i=p->MemberBegin();i!=p->MemberEnd();++i){V key;key.CopyFrom(i->name,a);out.AddMember(key,Expand(i->value,a,depth+1,budget,bytes,stack),a);}}
    else out.CopyFrom(*p,a);stack.erase(p);return out;
  }
public:
  explicit Graph(const V &root){unsigned budget=world::observation::kNodeBudget;Index(root,0,budget);}
  replica::Json Value(const V &v) const {replica::Json out;unsigned budget=unsigned(replica::kPlayerTargetNodes);std::uint64_t bytes=0;std::set<const V *> stack;auto x=Expand(v,out.GetAllocator(),0,budget,bytes,stack);static_cast<V &>(out).Swap(x);return out;}
};
std::vector<const V *> Seq(const V &v,unsigned limit=16384){std::vector<const V *> out;
  if(v.IsArray()){if(v.Size()>limit)Fail("sequence_budget");for(const auto &x:v.GetArray())out.push_back(&x);return out;}
  if(v.IsObject()&&v.ObjectEmpty())return out;
  if(Is(v,"kind","array"))return Seq(At(v,"values"),limit);
  if(Is(v,"type","table")){const auto &entries=At(v,"entries");if(entries.IsObject()&&entries.ObjectEmpty())return out;if(!entries.IsArray()||entries.Size()>limit)Fail("table_budget");std::map<unsigned,const V *> order;
    for(const auto &e:entries.GetArray()){auto i=Int<unsigned>(At(e,"key"));if(!i||i>limit||!order.emplace(i,&At(e,"value")).second)Fail("table_duplicate_or_index");}
    for(const auto &[i,x]:order){if(i!=out.size()+1)Fail("table_hole");out.push_back(x);}return out;}
  Fail("complete_sequence_required");}
const V &Field(const V &v,const char *id){return At(At(v,"fields"),id);}
const V &Named(const V &v,const char *name){for(const auto *e:Seq(At(v,"entries")))if(At(*e,"key").IsString()&&String(At(*e,"key"))==name)return At(*e,"value");Fail(std::string("table_member:")+name);}
ClientResourcePose Pose(const V &v){ClientResourcePose p;if(Is(v,"status","Null")){p.loaded=false;p.exact_phase=true;p.champion_color_present=true;p.shaders_present=true;return p;}
  std::string why;if(!ClientParseResourcePose(v,p,why))Fail(why);return p;}
ClientObjectReference Ref(const V &v,std::uint64_t run){if(Is(v,"status","Null"))return {true,true,{}};
  if(!Is(v,"type","reference")||!Is(v,"status","KnownCurrent"))Fail("current_reference_required");const auto &k=At(v,"target");
  ClientObjectKey key{Int<std::uint64_t>(At(k,"run")),Int<std::uint64_t>(At(k,"id")),Int<unsigned>(At(k,"space"))};if(key.run!=run||!key.id||key.space!=1)Fail("reference_scope");return {true,false,key};}
void Config(const V &v,bool &present,int &id,int &type){present=!Is(v,"status","Null");if(!present)return;id=Int(Field(v,"ItemConfigItem.ID"));type=Int(Field(v,"ItemConfigItem.Type"));if(id<0||type<0||type>4)Fail("item_config_identity");}
}
// Body/Head同源渲染组独立解析，不复制或重放武器及库存玩法。
static void RenderValues(ClientPlayerEquipment &out,const replica::EntityObjectTarget &o,const Graph &entity_graph,const replica::Value &body_value){
  const auto &fields=At(body_value,"fields");
  auto common=entity_graph.Value(o.common.value);out.main=Pose(Field(common,"Entity.GetSprite"));out.inline_sprites[0]=Pose(At(fields,"EntityPlayer.GetBodySprite"));
  const auto &resources=At(body_value,"application_player_resources");if(!Is(resources,"status","Value")||Int<unsigned>(At(resources,"version"))!=1)Fail("inline_resource_source");
  for(unsigned i=1;i<13;++i){const auto name=i==10?std::string("blood_gush"):i==11?std::string("held"):"inline_"+std::to_string(i);out.inline_sprites[i]=Pose(At(resources,name.c_str()));}
  for(const auto *v:Seq(At(fields,"EntityPlayer.GetCostumeSpriteDescs"),1024)){ClientPlayerEquipment::Costume c;
    Config(Field(*v,"CostumeSpriteDesc.GetItemConfig"),c.item_present,c.item,c.item_type);c.priority=Int(Field(*v,"CostumeSpriteDesc.GetPriority"));c.default_skin=Int(Field(*v,"CostumeSpriteDesc.GetDefaultSkinColor"));c.skin=Int(Field(*v,"CostumeSpriteDesc.GetSkinColor"));c.player_type=Int(Field(*v,"CostumeSpriteDesc.GetPlayerType"));
    c.item_animation=Bool(Field(*v,"CostumeSpriteDesc.IsItemAnimPlaying"));c.flying=Bool(Field(*v,"CostumeSpriteDesc.IsFlying"));c.overlay=Bool(Field(*v,"CostumeSpriteDesc.HasOverlay"));c.skin_alt=Bool(Field(*v,"CostumeSpriteDesc.HasSkinAlt"));c.overwrite=Bool(Field(*v,"CostumeSpriteDesc.CanOverwriteColor"));c.state_only=Bool(Field(*v,"CostumeSpriteDesc.IsItemStateOnly"));c.sprite=Pose(Field(*v,"CostumeSpriteDesc.GetSprite"));out.costumes.push_back(std::move(c));}
  const auto layers=Seq(At(fields,"EntityPlayer.GetCostumeLayerMap"),15);if(layers.size()!=15)Fail("costume_layer_map_15_required");for(unsigned i=0;i<15;++i){auto &l=out.layers[i];l.costume=Int(Named(*layers[i],"costumeIndex"));l.layer=Int(Named(*layers[i],"layerID"));l.priority=Int(Named(*layers[i],"priority"));l.body=Bool(Named(*layers[i],"isBodyLayer"));
    if(l.costume<-1||(l.costume>=0&&(unsigned(l.costume)>=out.costumes.size()||l.layer<0||unsigned(l.layer)>=out.costumes[unsigned(l.costume)].sprite.layers.size())))Fail("costume_layer_dependency");}
}
static std::shared_ptr<const ClientPlayerEquipment> Describe(const replica::PlayerTarget &target,const replica::PlayerBodyTarget &body,const Graph &entity_graph,const Graph &player_graph){
  const auto &o=target.entities->objects.at(body.entity_index);
  const auto &raw=std::get<replica::EntityClassValues<replica::EntityCategory::Player>>(o.payload).fields.value;
  auto body_value=entity_graph.Value(raw);const auto &fields=At(body_value,"fields");const auto &extra=At(body_value,"application_player_equipment");
  if(Int<unsigned>(At(extra,"version"))!=1||!Is(extra,"status","Value")||!Is(extra,"scope","same-ordinary-window"))Fail("current_complete_equipment_source_required");
  auto out=std::make_shared<ClientPlayerEquipment>();const auto &effects=At(extra,"effects");out->effects_disabled=Bool(At(effects,"disabled"));out->effects_evaluate=Bool(At(effects,"evaluate"));
  out->effects_cache=Int<std::uint32_t>(At(effects,"cache_flags"));
  out->history_mutation_disabled=Bool(At(extra,"history_mutation_disabled"));
  out->head_animation=Bytes(At(extra,"head_animation_hex"));
  for(const auto *e:Seq(At(effects,"entries"))){ClientPlayerEquipment::Effect x;x.item_present=Bool(At(*e,"item_present"));x.type=Int(At(*e,"type"));x.id=Int(At(*e,"id"));x.count=Int(At(*e,"count"));x.cooldown=Int(At(*e,"cooldown"));if(x.type<0||x.type>4||x.id<0)Fail("effect_identity");out->effects.push_back(x);}
  const auto forms=Seq(At(extra,"forms"),15);if(forms.size()!=15)Fail("forms_complete_15_required");for(unsigned i=0;i<15;++i)out->forms[i]=Int(*forms[i]);
  // History的图必须使用Player组件；不能用Entity组件同号node或上一身体补洞。
  auto history=player_graph.Value(body.history);auto view=std::make_shared<ClientPlayerHistoryView>();view->body={body.key.run,body.key.id,body.key.space};view->entity_scope=target.entity_capture.scope;view->player_scope=target.player_capture.scope;view->revision=target.revision;view->shape=body.shape_revision;
  for(const auto *item:Seq(Field(history,"History.GetCollectiblesHistory"))){ClientPlayerHistoryView::Item x;
    x.time=Int(Field(*item,"HistoryItem.GetTime"));x.id=Int(Field(*item,"HistoryItem.GetItemID"));x.stage=Int(Field(*item,"HistoryItem.GetLevelStage"));x.stage_type=Int(Field(*item,"HistoryItem.GetStageType"));x.room=Int(Field(*item,"HistoryItem.GetRoomType"));x.pool=Int(Field(*item,"HistoryItem.GetItemPoolType"));x.trinket=Bool(Field(*item,"HistoryItem.IsTrinket"));view->items.push_back(x);}out->history=std::move(view);
  RenderValues(*out,o,entity_graph,body_value);
  const auto &weapons=At(fields,"EntityPlayer.GetWeapon");if(!Is(weapons,"kind","parameter_results"))Fail("five_weapon_slots_required");unsigned mask=0;
  const auto extras=Seq(At(extra,"weapons"),5);if(extras.size()!=5)Fail("five_native_weapon_slots_required");
  for(const auto *row:Seq(At(weapons,"values"),5)){const auto params=Seq(At(*row,"parameters"),1);if(params.size()!=1)Fail("weapon_slot_parameters");unsigned slot=Int<unsigned>(*params[0]);if(slot>=5||(mask&(1u<<slot)))Fail("weapon_slot_duplicate_or_range");mask|=1u<<slot;
    auto &w=out->weapons[slot];const auto &v=At(*row,"value"),&native=*extras[slot];w.present=!Is(v,"status","Null");if(Bool(At(native,"present"))!=w.present)Fail("weapon_public_native_presence_conflict");if(!w.present)continue;
    w.type=Int(Field(v,"Weapon.GetWeaponType"));if(w.type<1||w.type>15||Int(At(native,"type"))!=w.type)Fail("weapon_type");w.delay=Real(Field(v,"Weapon.GetFireDelay"));w.max_delay=Real(Field(v,"Weapon.GetMaxFireDelay"));w.charge=Real(Field(v,"Weapon.GetCharge"));w.max_charge=Real(Field(v,"Weapon.GetMaxCharge"));w.num_fired=Int(Field(v,"Weapon.GetNumFired"));w.modifiers=Int<unsigned>(Field(v,"Weapon.GetModifiers"));w.axis_aligned=Bool(Field(v,"Weapon.IsAxisAligned"));
    const auto &dir=Field(v,"Weapon.GetDirection");w.direction={Real(At(dir,"x")),Real(At(dir,"y"))};w.owner=Ref(Field(v,"Weapon.GetOwner"),body.key.run);w.main=Ref(Field(v,"Weapon.GetMainEntity"),body.key.run);
    if(!Is(native,"scalar_policy","public-base-only;gameplay-caches-authority-only"))Fail("weapon_scalar_consumer_contract");
    const auto bars=Seq(At(native,"charge_bar"),2);if(bars.size()!=2)Fail("weapon_charge_bar_pair_required");for(unsigned i=0;i<2;++i)w.charge_bar[i]=Real(*bars[i]);
    const auto &links=At(native,"links");if(!links.IsObject())Fail("weapon_links");for(auto e=links.MemberBegin();e!=links.MemberEnd();++e)w.links.emplace(String(e->name),Ref(e->value,body.key.run));
    const auto &sprites=At(native,"sprites");if(!sprites.IsObject())Fail("weapon_sprites");for(auto e=sprites.MemberBegin();e!=sprites.MemberEnd();++e)w.sprites.emplace(String(e->name),Pose(e->value));
  }if(mask!=31)Fail("weapon_slot_missing");
  // 拒绝必须保留实际身体、缺失依赖及五槽中的字段，不让诊断猜测最后一个Player。
  const auto dependency=[&](const ClientObjectReference &ref,const std::string &field){
    const replica::SubjectKey required{ref.target.run,ref.target.id,ref.target.space};
    if(!ref.present)throw replica::PlayerContractError("player_equipment:weapon_reference_not_captured",body.key,required,field);
    if(ref.is_null)return;
    for(const auto &entity:target.entities->objects)if(entity.key==required&&entity.presence==replica::EntityMemberPresence::Current)return;
    throw replica::PlayerContractError("player_equipment:weapon_reference_missing_from_same_target",body.key,required,field);
  };
  for(unsigned slot=0;slot<5;++slot){const auto &weapon=out->weapons[slot];if(!weapon.present)continue;const auto field="equipment.weapons/"+std::to_string(slot)+"/";
    dependency(weapon.owner,field+"owner");dependency(weapon.main,field+"main");for(const auto &[name,ref]:weapon.links)dependency(ref,field+"links/"+name);}
  return out;
}
std::map<ClientObjectKey,std::shared_ptr<const ClientPlayerEquipment>> ClientDescribePlayerEquipment(const replica::PlayerTarget &target){
  // 同一T的两份图只建索引一次；多身体不逐身体重新扫描整个世界组件。
  const Graph entity_graph(target.entities->source),player_graph(target.source);std::map<ClientObjectKey,std::shared_ptr<const ClientPlayerEquipment>> out;
  for(const auto &body:target.bodies){
    try{out.emplace(ClientObjectKey{body.key.run,body.key.id,body.key.space},Describe(target,body,entity_graph,player_graph));}
    catch(const replica::PlayerContractError &){throw;}
    catch(const replica::ContractError &e){throw replica::PlayerContractError(e.what(),body.key,{},e.what());}
  }return out;
}
std::shared_ptr<const ClientPlayerEquipment> ClientStreamPlayerEquipment(const ClientPlayerEquipment &base,const replica::EntityObjectTarget &object){
  replica::Json root(rapidjson::kObjectType);auto &a=root.GetAllocator();replica::Value common,body_value;common.CopyFrom(object.common.value,a);body_value.CopyFrom(std::get<replica::EntityClassValues<replica::EntityCategory::Player>>(object.payload).fields.value,a);
  replica::Put(root,"common",std::move(common),a);replica::Put(root,"body",std::move(body_value),a);
  const Graph graph(root);auto body=graph.Value(root["body"]);auto out=std::make_shared<ClientPlayerEquipment>(base);
  out->head_animation=Bytes(At(At(body,"application_player_equipment"),"head_animation_hex"));out->costumes.clear();RenderValues(*out,object,graph,body);return out;
}
const char *ClientPlayerEquipmentDisposition(const std::string &id) noexcept {
  if(id.rfind("TemporaryEffect",0)==0||id=="EntityPlayer.GetEffects"||id=="EntityPlayer.GetPlayerFormCounter"||id=="EntityPlayer.HasPlayerForm")return "I16:ordered-effects+timers+forms;no-AddRemove-or-Evaluate";
  if(id.rfind("History",0)==0||id=="EntityPlayer.GetHistory")return "I17:body-owned-ordered-history+immutable-HUD-view;no-item-replay";
  if(id.rfind("CostumeSpriteDesc.",0)==0||id=="EntityPlayer.GetCostumeSpriteDescs"||id=="EntityPlayer.GetCostumeLayerMap")return "I19:constructed-ANM-vector+local-config+15-layer-map";
  if(id=="EntityPlayer.GetBodySprite"||id=="EntityPlayer.GetHeldSprite"||id=="Entity.GetSprite")return "I18:main+13-distinct-inline-exact-resource-poses";
  if(id.rfind("Weapon.",0)==0||id=="EntityPlayer.GetWeapon"||id=="EntityPlayer.FireDelay")return "I20:five-slots+typed-dynamic-weapon-owner+same-T-Entity-links";
  return nullptr;
}
} // namespace cs::game
