#include "replica/entity_target.h"
#include "replica/projection_graph.h"
#include <charconv>
#include <cmath>
#include <map>
#include <set>
#include <string_view>
#include <type_traits>

namespace cs::replica {
namespace {
using V=Value;
[[noreturn]] void Error(const std::string &path,const char *why){throw ContractError("entity:"+path+":"+why);}
const V *Find(const V &v,const char *key){return v.IsObject()&&v.HasMember(key)?&v[key]:nullptr;}
const V &At(const V &v,const char *key,const std::string &path){const auto *p=Find(v,key);if(!p)Error(path+"/"+key,"required_missing");return *p;}
std::string Str(const V &v){return v.IsString()?std::string(v.GetString(),v.GetStringLength()):std::string{};}
bool Is(const V *v,const char *s){return v&&v->IsString()&&std::string_view(v->GetString(),v->GetStringLength())==s;}
const V &Scalar(const V &v){return Is(Find(v,"type"),"int64")?At(v,"value","typed-int64"):v;}
template<typename T> T DecimalValue(const V &v,const std::string &path){
  const auto &x=Scalar(v);T n{};
  if(x.IsString()){const auto r=std::from_chars(x.GetString(),x.GetString()+x.GetStringLength(),n);if(r.ec==std::errc{}&&r.ptr==x.GetString()+x.GetStringLength())return n;}
  if constexpr(std::is_signed_v<T>){if(x.IsInt64())return T(x.GetInt64());}
  else {if(x.IsUint64())return T(x.GetUint64());}
  Error(path,"decimal_range_or_type");
}
std::uint64_t U64(const V &v,const std::string &p){return DecimalValue<std::uint64_t>(v,p);}
std::int64_t I64(const V &v,const std::string &p){return DecimalValue<std::int64_t>(v,p);}
const V &Sequence(const V &v,const std::string &p){
  if(v.IsArray())return v;
  if(v.IsObject()&&v.MemberCount()==0){static const V empty(rapidjson::kArrayType);return empty;}
  Error(p,"array_or_explicit_empty_required");
}
// 占位、合法空和失败分别保留；调用者决定所需字段，解析器绝不补默认状态。
bool Placeholder(const V &v,const std::string &p){
  if(Is(Find(v,"type"),"reference"))return false;
  const auto *s=Find(v,"status");if(!s)return false;
  static const std::set<std::string> states={"Value","Null","Empty","NotApplicable","NotScheduled","Unknown","Unavailable","ReadFailed","Truncated","Nonfinite","NotCaptured","Unsupported","AuthorityOnly","NotReplicated","ClientLocal","Unverified"};
  const auto name=Str(*s);if(!states.count(name))Error(p,"unknown_field_status");
  return name!="Value"&&name!="Empty"&&name!="Unverified";
}
bool Gap(const V &v){
  if(const auto *s=Find(v,"status")){const auto name=Str(*s);
    if(name=="Unknown"||name=="Unavailable"||name=="ReadFailed"||name=="Truncated"||name=="Nonfinite"||name=="NotCaptured"||name=="Unsupported"||name=="Unresolved"||name=="AmbiguousReuse"||name=="CrossEpoch"||name=="UnsupportedTarget")return true;}
  if(v.IsArray())for(const auto &x:v.GetArray())if(Gap(x))return true;
  if(v.IsObject())for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)if(Gap(i->value))return true;
  return false;
}
void Real(const V &v,const std::string &p){if(Placeholder(v,p))return;const auto &x=Scalar(v);if(x.IsNumber()&&std::isfinite(x.GetDouble()))return;I64(v,p);}
void Bits(const V &v,const std::string &p){
  // Lua的int64可用负十进制表达uint64位模式；保留原编码，不经double截短。
  const auto &x=Scalar(v);if(x.IsInt64()||x.IsUint64())return;
  if(x.IsString()&&x.GetStringLength()&&x.GetString()[0]=='-'){I64(x,p);return;}U64(x,p);
}
void Typed(const V &v,const std::string &p){
  if(v.IsArray()){for(const auto &x:v.GetArray())Typed(x,p);return;}
  if(!v.IsObject())return;
  if(!Is(Find(v,"type"),"reference"))Placeholder(v,p);
  if(Is(Find(v,"type"),"int64"))I64(v,p);
  if(Is(Find(v,"type"),"bits128")){Bits(At(v,"low",p),p+"/low");Bits(At(v,"high",p),p+"/high");}
  if(Is(Find(v,"type"),"bytes")){
    if(!Is(Find(v,"encoding"),"lua-string-bytes"))Error(p,"byte_encoding");
    const auto &h=At(v,"hex",p);if(!h.IsString())Error(p,"byte_hex_string");
    const auto hex=Str(h);if(hex.size()%2||hex.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos)Error(p,"byte_hex");
  }
  if(Is(Find(v,"type"),"vector")){Real(At(v,"x",p),p+"/x");Real(At(v,"y",p),p+"/y");}
  if(Is(Find(v,"type"),"color"))for(const char *key:{"R","G","B","A","RO","GO","BO"})Real(At(v,key,p),p+"/"+key);
  if(Is(Find(v,"type"),"native_kcolor20")&&!Placeholder(v,p)){
    for(const char *key:{"R","G","B","A"})Real(At(v,key,p),p+"/"+key);
    const auto extra=I64(At(v,"opaque32",p),p+"/opaque32");if(extra<INT32_MIN||extra>INT32_MAX)Error(p,"kcolor_opaque32_range");
  }
  // 稀疏table按entries保留键和值；不压成连续vector，不跨组件解析graph_ref。
  if(Is(Find(v,"type"),"table")){
    const auto &entries=Sequence(At(v,"entries",p),p+"/entries");std::set<std::string> keys;
    for(const auto &e:entries.GetArray()){const auto &key=At(e,"key",p);At(e,"value",p);if(!keys.insert(Text(key)).second)Error(p,"duplicate_sparse_key");}
  }
  for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)Typed(i->value,p+"/"+Str(i->name));
}
void FieldType(const V &v,const V &policy,const std::string &p,bool parameter_value=false){
  if(Placeholder(v,p))return;
  const auto category=Str(policy["category"]),type=Str(policy["returns"]);
  if(const auto *returns=Find(v,"returns")){
    const auto &items=Sequence(*returns,p+"/returns");if(items.Empty()||items.Size()>8)Error(p,"return_tuple_size");
    auto first_policy=Copy(policy);Put(first_policy,"returns",type.substr(0,type.find(',')),first_policy.GetAllocator());
    FieldType(items[0],first_policy,p+"/returns/0",parameter_value);return;
  }
  if(category=="parameter_query"&&!parameter_value){
    if(!Is(Find(v,"kind"),"parameter_results"))Error(p,"parameter_results_required");
    for(const auto &row:Sequence(At(v,"values",p),p).GetArray()){
      At(row,"parameters",p);FieldType(At(row,"value",p),policy,p+"/value",true);
    }return;
  }
  if(type=="boolean"||type=="bool"){if(!v.IsBool())Error(p,"boolean_required");}
  else if(type=="float"||type=="double")Real(v,p);
  else if(type=="int"||type=="integer")I64(v,p);
  else if(type=="uint8"){if(U64(v,p)>UINT8_MAX)Error(p,"unsigned_field_range");}
  else if(type=="int8"||type=="int32"){
    const auto n=I64(v,p);if(type=="int8"?(n<INT8_MIN||n>INT8_MAX):(n<INT32_MIN||n>INT32_MAX))Error(p,"signed_field_range");
  }
  else if(type=="string"){if(!v.IsString()&&!Is(Find(v,"type"),"bytes"))Error(p,"string_or_bytes_required");}
  else if(type=="TearFlags"||type=="BitSet128"){if(!Is(Find(v,"type"),"bits128"))Error(p,"bits128_required");}
  else if(type=="Vector"){if(!Is(Find(v,"type"),"vector"))Error(p,"vector_required");}
  else if(type=="Sprite"){if(!Is(Find(v,"type"),"resource_pose"))Error(p,"resource_pose_required");At(v,"resource",p);At(v,"pose_runtime",p);}
  else if(type=="Entity"||type=="EntityPlayer"||type=="EntityNPC"||type=="EntityTear"||type=="EntityBomb"||type=="EntityEffect"||type=="EntityLaser"||type=="EntityFamiliar"||type=="EntityPickup"||type=="EntityKnife"||type=="EntityProjectile"){
    if(Is(Find(v,"resolution"),"Null"))return;
    if(Is(Find(v,"type"),"owner_qualified_domain_reference"))return;
    if(!Is(Find(v,"type"),"reference"))Error(p,"entity_reference_required");
    if(const auto *key=Find(v,"target");key&&U64(At(*key,"space",p),p)!=1)Error(p,"entity_reference_space");
  }
}
using Graph=std::map<std::uint64_t,const V *>;
void GraphNodes(const V &v,Graph &out){
  if(v.IsArray()){for(const auto &x:v.GetArray())GraphNodes(x,out);return;}if(!v.IsObject())return;
  if(Is(Find(v,"type"),"table"))if(const auto *node=Find(v,"node"))out.emplace(U64(*node,"graph/node"),&v);
  for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)GraphNodes(i->value,out);
}
void References(const V &v,const std::string &p,std::vector<EntityReferenceTarget> &out,const Graph &graph,std::set<std::uint64_t> &visiting,unsigned &budget,unsigned &reference_count){
  if(!budget--)Error(p,"reference_graph_budget");
  if(v.IsArray()){unsigned i=0;for(const auto &x:v.GetArray())References(x,p+"/"+std::to_string(i++),out,graph,visiting,budget,reference_count);return;}
  if(!v.IsObject())return;
  if(Is(Find(v,"type"),"graph_ref")){
    const auto node=U64(At(v,"node",p),p);const auto it=graph.find(node);if(it==graph.end())Error(p,"reference_graph_unresolved");
    if(!visiting.insert(node).second)return;References(*it->second,p,out,graph,visiting,budget,reference_count);visiting.erase(node);return;
  }
  if(Is(Find(v,"type"),"owner_qualified_domain_reference")){
    if(++reference_count>262144)Error(p,"reference_budget");
    EntityReferenceTarget ref;ref.path=p;ref.owner_qualified=true;ref.status="OwnerQualified";
    if(const auto *c=Find(v,"contract"))ref.contract=Str(*c);
    else {if(const auto *s=Find(v,"schema"))ref.contract=Str(*s);if(const auto *f=Find(v,"field"))ref.contract+=":"+Str(*f);}
    out.push_back(std::move(ref));return;
  }
  // Lua nil采用通用Null节点。只有具名Entity关系将它解释为解除边，Sprite/optional-bool的Null不建依赖。
  bool null_relation=false;
  if(Is(Find(v,"status"),"Null")){const auto member=EntityReferenceMember(p);if(!member.empty()){
    const auto *policy=EntityApplicationPolicy(member);std::string returns;if(policy&&policy->HasMember("returns"))returns=(*policy)["returns"].GetString();
    null_relation=(returns.rfind("Entity",0)==0&&returns.rfind("EntityConfig",0)!=0&&returns!="EntityDesc")||p.find("/application_links/links/")!=std::string::npos;
  }}
  if(Is(Find(v,"type"),"reference")||Is(Find(v,"resolution"),"Null")||null_relation){
    if(++reference_count>262144)Error(p,"reference_budget");
    EntityReferenceTarget ref;ref.path=p;ref.status=Is(Find(v,"resolution"),"Null")?"Null":Str(At(v,"status",p));
    if(const auto *key=Find(v,"target")){ref.has_target=true;ref.target={U64(At(*key,"run",p),p),U64(At(*key,"id",p),p),unsigned(U64(At(*key,"space",p),p))};}
    out.push_back(std::move(ref));return;
  }
  for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)References(i->value,p+"/"+Str(i->name),out,graph,visiting,budget,reference_count);
}
using Policies=std::map<std::string,const V *>;
const Policies &PoliciesById(){
  static const Policies policies=[](){Policies out;
    for(const auto &r:EntityApplicationCatalog()["members"].GetArray()){
      const auto id=Str(r["id"]);auto it=out.find(id);
      // Canonical别名共用字段，优先正式query（SampleLaser）而非旧userdata属性。
      if(it==out.end()||Is(Find(r,"category"),"query"))out[id]=&r;
    }return out;}();return policies;
}
EntityFields Fields(const V &section,const std::string &owner,const std::string &path){
  EntityFields out;out.value=Copy(section);
  if(Placeholder(section,path)) {out.missing.push_back(path);return out;}
  if(!Is(Find(section,"type"),owner.c_str()))Error(path,"section_class_mismatch");
  const auto &fields=At(section,"fields",path);if(!fields.IsObject())Error(path,"fields_required");
  for(auto i=fields.MemberBegin();i!=fields.MemberEnd();++i){
    const auto id=Str(i->name);const auto *policy=EntityApplicationPolicy(id);
    if(!policy||Str((*policy)["owner"])!=owner)Error(path+"/"+id,"undeclared_or_wrong_owner");
    FieldType(i->value,*policy,path+"/"+id);
    // external_reference保留其它规范域/Authority的来源状态，由具名consumer判断。
    // 例如原版Online IsLocalPlayer被明确隔离，不能使完整C/S Player永远缺字段。
    if(Str((*policy)["category"])!="external_reference"&&Gap(i->value))out.missing.push_back(id);
  }
  for(const auto &[id,policy]:PoliciesById()){
    if(Str((*policy)["owner"])!=owner||Str((*policy)["ownership"])!="ReplicatedState")continue;
    const auto cat=Str((*policy)["category"]);
    if(cat!="field"&&cat!="query"&&cat!="native_field")continue;
    if(!fields.HasMember(id.c_str()))out.missing.push_back(id);
  }
  return out;
}
EntityCategory Category(const std::string &name){
  static const std::map<std::string,EntityCategory> types={
    {"EntityPlayer",EntityCategory::Player},{"EntityNPC",EntityCategory::NPC},{"EntityTear",EntityCategory::Tear},
    {"EntityProjectile",EntityCategory::Projectile},{"EntityBomb",EntityCategory::Bomb},{"EntityLaser",EntityCategory::Laser},
    {"EntityKnife",EntityCategory::Knife},{"EntityPickup",EntityCategory::Pickup},{"EntityFamiliar",EntityCategory::Familiar},
    {"EntityEffect",EntityCategory::Effect},{"EntitySlot",EntityCategory::Slot},{"NativeText",EntityCategory::NativeText},{"NativeDummy",EntityCategory::NativeDummy}};
  if(name=="NativeDarkness")Error("class","legacy_NativeDarkness_not_an_alias");
  const auto it=types.find(name);return it==types.end()?EntityCategory::Unknown:it->second;
}
template<std::size_t I=0> EntityPayload Payload(EntityCategory category,EntityFields fields){
  if constexpr(I<std::variant_size_v<EntityPayload>){if(unsigned(category)==I)return std::variant_alternative_t<I,EntityPayload>{std::move(fields)};return Payload<I+1>(category,std::move(fields));}
  else {Error("class","category_variant_mismatch");}
}
SubjectKey Key(const V &subject,const Scope &scope,const std::string &path){
  if(!Is(Find(subject,"type"),"reference")||(!Is(Find(subject,"status"),"KnownCurrent")&&!Is(Find(subject,"status"),"Retired")))Error(path,"current_or_retired_subject_required");
  const auto &v=At(subject,"target",path);SubjectKey key{U64(At(v,"run",path),path),U64(At(v,"id",path),path),unsigned(U64(At(v,"space",path),path))};
  if(key.run!=scope.run||key.space!=1||!key.id)Error(path,"entity_key_scope_or_space");return key;
}
void TypeWitness(const EntityObjectTarget &o,const std::string &p){
  if(o.category==EntityCategory::Unknown)return;
  const auto *fields=Find(o.common.value,"fields");if(!fields)return;
  const auto *t=Find(*fields,"Entity.Type");if(!t||Placeholder(*t,p))return;
  const auto type=I64(*t,p);
  // 公开EntityType的具名值仅作交叉检查；NPC靠ToNPC见证，不推断任何数值区间。
  static const std::map<EntityCategory,int> exact={{EntityCategory::Player,1},{EntityCategory::Tear,2},{EntityCategory::Familiar,3},{EntityCategory::Bomb,4},{EntityCategory::Pickup,5},{EntityCategory::Slot,6},{EntityCategory::Laser,7},{EntityCategory::Knife,8},{EntityCategory::Projectile,9},{EntityCategory::Effect,1000},{EntityCategory::NativeText,9001}};
  if(auto it=exact.find(o.category);it!=exact.end()&&type!=it->second)Error(p,"class_type_conflict");
}
} // namespace

const Json &EntityApplicationCatalog(){static const Json catalog=Parse(
#include "entity_application.inc"
);return catalog;}
const Value *EntityApplicationPolicy(const std::string &id){const auto &p=PoliciesById();const auto i=p.find(id);return i==p.end()?nullptr:i->second;}
std::string EntityReferenceMember(const std::string &path){const std::string native="/application_links/links/";auto slash=path.find(native);std::size_t skip=native.size();
  if(slash==std::string::npos){slash=path.find("/fields/");skip=8;}if(slash==std::string::npos)return {};
  auto member=path.substr(slash+skip);const auto end=member.find('/');if(end!=std::string::npos)member.resize(end);return member;
}

EntityTarget ParseEntityTarget(const Value &model){
  ValidateModel(model);
  EntityTarget out;out.scope=ReadScope(model["scope"]);out.revision=Unsigned(model["revision"]);
  const auto &component=At(At(model,"components","model"),"entity","components");
  const auto &state=At(component,"state","entity");const auto &src=At(state,"application_source","entity");
  if(U64(At(src,"version","source"),"source/version")!=1||!Is(Find(src,"contract"),"entity-source-v1")||!Is(Find(src,"target"),"REPENTOGON-1.1.3/J273")||!Is(Find(src,"pose_scope"),"same-ordinary-window"))Error("application_source","source_contract_version");
  static const std::set<std::string> supported={"resource-pose-v1","entity-classification-v1","entity-auxiliary-sprites-v1","entity-native-presentation-v1","entity-class-application-v1","entity-class-application-v2","entity-class-application-v3"};
  std::set<std::string> required;
  for(const auto &c:Sequence(At(src,"required_contracts","source"),"source/contracts").GetArray()){
    const auto name=Str(c);if(!supported.count(name)||!required.insert(name).second)Error("application_source","unknown_or_duplicate_required_contract");
  }
  for(const char *name:{"resource-pose-v1","entity-classification-v1","entity-auxiliary-sprites-v1"})if(!required.count(name))Error("application_source","missing_required_contract");
  // ValidateModel已经按整个component检查图闭合；不在各object上重建另一张图。
  Typed(state,"state");
  const auto &capture=At(component,"capture","entity");out.capture={out.revision,U64(At(capture,"scope","capture"),"capture"),U64(At(capture,"sequence","capture"),"capture"),U64(At(capture,"authority_tick","capture"),"capture")};
  out.coverage=Str(At(component,"coverage","entity"));out.source_complete=out.coverage=="Complete";
  const auto &members=At(state,"membership","entity");const auto &complete=At(members,"complete","membership");if(!complete.IsBool())Error("membership","complete_boolean");
  const auto count=U64(At(members,"count","membership"),"membership/count");if(count>16384)Error("membership","member_budget");out.source_member_count=std::uint32_t(count);
  const auto &rows=Sequence(At(state,"objects","entity"),"objects");if(rows.Size()>16384||rows.Size()>count)Error("objects","member_budget_or_count");
  if(complete.GetBool()&&rows.Size()!=count)Error("membership","complete_count_mismatch");
  // 原始全成员枚举、字段coverage与非Player子范围是三项事实；Partial字段不能改写枚举事实。
  out.membership_complete=complete.GetBool();
  out.non_player_complete=out.membership_complete;std::set<SubjectKey> seen;
  // 遍历包含有限查询的参数／标量及图别名展开，按节点上限计量；实际引用另限262144，不能把两种成本混用。
  Graph graph;GraphNodes(state,graph);unsigned reference_budget=67108864,reference_count=0;
  for(const auto &row:rows.GetArray()){
    EntityObjectTarget object;const auto path="objects/"+std::to_string(out.objects.size());const auto &subject=At(row,"subject",path);
    object.key=Key(subject,out.scope,path);if(!seen.insert(object.key).second)Error(path,"duplicate_subject");
    object.shape_revision=U64(At(subject,"shape_revision",path),path+"/shape_revision");
    // IdentityTable的首个shape revision合法为0；Morph才推进，不能制造初始+1。
    if(Is(Find(subject,"status"),"Retired")){
      if(!Is(Find(row,"status"),"NotApplicable"))Error(path,"retired_row_status");
      object.presence=EntityMemberPresence::Retired;object.common.value=Copy(row);
      // 退休行不解引用派生字段，也不补造分类或把它混入活非Player成员表。
      out.non_player_complete=false;out.objects.push_back(std::move(object));continue;
    }
    object.source_class=Str(At(row,"class",path));object.category=Category(object.source_class);object.classification_supported=object.category!=EntityCategory::Unknown;
    if(object.category>=EntityCategory::Laser&&object.category<=EntityCategory::Effect&&!required.count("entity-class-application-v2"))Error(path,"class_application_v2_required");
    if(object.category!=EntityCategory::Unknown&&!required.count("entity-class-application-v3"))Error(path,"class_application_v3_required");
    object.common=Fields(At(row,"common",path),"Entity",path+"/common");
    EntityFields body;
    if(object.classification_supported)body=Fields(At(row,"body",path),object.source_class,path+"/body");
    else {body.value=Copy(row);body.missing.push_back(path+"/unsupported-class:"+object.source_class);}
    if(object.category==EntityCategory::Player)out.player_keys.push_back(object.key);
    else {out.non_player_keys.push_back(object.key);if(!object.classification_supported||!body.missing.empty()||!object.common.missing.empty())out.non_player_complete=false;}
    object.payload=Payload(object.category,std::move(body));
    if(const auto *special=Find(row,"special")){
      if(object.category!=EntityCategory::NPC)Error(path,"special_requires_NPC");
      object.delirium=true;object.special=Fields(*special,"EntityDelirium",path+"/special");if(!object.special.missing.empty())out.non_player_complete=false;
    }
    const auto *fields=Find(object.common.value,"fields");
    if(fields){if(const auto *v=Find(*fields,"Entity.Exists");v&&v->IsBool()){object.exists_known=true;object.exists=v->GetBool();}
      if(const auto *v=Find(*fields,"Entity.IsDead");v&&v->IsBool()){object.dead_known=true;object.dead=v->GetBool();}}
    TypeWitness(object,path);std::set<std::uint64_t> visiting;
    References(object.common.value,path+"/common",object.references,graph,visiting,reference_budget,reference_count);
    std::visit([&](const auto &body_value){References(body_value.fields.value,path+"/body",object.references,graph,visiting,reference_budget,reference_count);},object.payload);
    if(object.delirium)References(object.special.value,path+"/special",object.references,graph,visiting,reference_budget,reference_count);
    out.objects.push_back(std::move(object));
  }
  out.source=Copy(component);return out;
}
Json EntityCapabilities(){return Parse(R"({
  "id":"entity-target-v1","source":"entity-source-v1","catalog":1,"model":2,"target":"REPENTOGON-1.1.3/J273",
  "model_expression":"all-catalogued-members;typed-status;int64;bits128;sparse-graph;resources;typed-references",
  "classification":"public-derived-interface-or-native-RTTI;NativeDummy;no-NativeDarkness-alias",
  "common_execution":"I05-I30;instances-then-Grid-then-links-checks;Scene-only-commit",
  "builtin_class_adapters":["NPC","Delirium-special","Tear","Projectile","Bomb","Laser","Knife","Pickup","Familiar","Effect","Slot1..18","NativeText","NativeDummy"],
  "class_source_contract":"entity-class-application-v3","non_player_native_apply":true,
  "player_native_apply":"complete Player owner when player-source-v2 present;legacy ordinary-Isaac profile remains separate",
  "result_scope":"legacy=non_player_entity_applied+player_support_applied;complete Player owner in same attempt additionally sets full_entity_applied and Player domain;no-network-ACK",
  "independent":"Entity-only-bounded-history;exact-required-state-and-Key-shape-for-Render-Query;events-handoff27;no-event-watermark",
  "retirement":"stop-draw;drain-borrows;unlink;reverse-instance-class-retire;Grid;Room;unknown-prefix-retains-isolation",
  "runtime_acceptance":"tx-04-20:I39-regression;65-Source/65-cold-Client;20-invalid/2-compat/16-fault/4-cancel;explicit unaffected-prefix reuse;named-scene-visual-reviewed","acceptance_document":"docs/archive/design/tx-04-20/tx-04-20-e2e.md","full_entity_domain_commit":true,"full_entity_domain_condition":"complete source and checked Player owner in same Scene attempt;legacy profile cannot set full Entity bit",
  "native_pool_capacities":{"NPC":512,"Tear":512,"Projectile":512,"Bomb":64,"Laser":128,"Knife":64,"Pickup":512,"Familiar":64,"Effect":1024,"Slot":128,"NativeText":128,"NativeDummy":128},
  "limits":{"objects":16384,"render_members":16384,"commands":131073,"references":262144,"curve_points":262144,"resources":4096,"resource_slots":16384,"bytes":33554432,"nodes":67108864,"depth":64,"independent_records":128,"independent_bytes":2097152,"independent_record_bytes":262144}
})");}
} // namespace cs::replica
