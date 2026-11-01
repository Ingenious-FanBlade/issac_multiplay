#include "replica/native_entity.h"
#include <algorithm>
#include <charconv>
#include <cmath>

namespace cs::replica {
namespace {
[[noreturn]] void Fail(const char *reason){throw ContractError(reason);}
const Value &At(const Value &v,const char *key){
  if(!v.IsObject()||!v.HasMember(key))Fail("native_entity_missing_field");return v[key];
}
std::string String(const Value &v){if(!v.IsString())Fail("native_entity_string");return {v.GetString(),v.GetStringLength()};}
std::int64_t Integer(const Value &v){
  if(v.IsInt64())return v.GetInt64();
  if(v.IsObject()&&At(v,"type")=="int64"){
    const auto s=String(At(v,"value"));std::int64_t n=0;const auto r=std::from_chars(s.data(),s.data()+s.size(),n);
    if(r.ec==std::errc{}&&r.ptr==s.data()+s.size())return n;
  }
  Fail("native_entity_integer");
}
std::uint32_t U32(const Value &v){const auto n=Integer(v);if(n<0||n>UINT32_MAX)Fail("native_entity_u32");return std::uint32_t(n);}
std::uint32_t Seed32(const Value &v){const auto n=Integer(v);if(n<INT32_MIN||n>UINT32_MAX)Fail("native_entity_seed32");return std::uint32_t(n);}
std::uint64_t Key(const Value &v,const Scope &scope,unsigned space){
  if(Unsigned(At(v,"run"))!=scope.run||Unsigned(At(v,"space"))!=space)Fail("native_entity_key_scope");
  const auto id=Unsigned(At(v,"id"));if(!id)Fail("native_entity_zero_key");return id;
}
void Vector(const Value &v){
  if(At(v,"type")!="vector")Fail("native_entity_vector");
  for(const char *key:{"x","y"}){const auto &n=At(v,key);if(!n.IsNumber()||!std::isfinite(n.GetDouble())||std::abs(n.GetDouble())>3.402823466e38)Fail("native_entity_number");}
}
void Fields(const Value &v,const std::string &owner){
  if(At(v,"type")!=owner.c_str()||!At(v,"fields").IsObject())Fail("native_entity_body_type");
  // 按目录要求完整字段集合；NotScheduled等真实状态由各成员合同解释，不能删字段伪装完整。
  static const auto members=[](){std::map<std::string,std::set<std::string>> out;
    for(const auto &row:Catalog()["members"].GetArray()){
      if(row["source_catalog"]!="entity-catalog.json")continue;
      const auto id=String(row["id"]),category=String(row["category"]);const auto split=id.find('.');
      if(split!=id.npos&&category!="operation"&&category!="cast"&&category!="container_operator")out[id.substr(0,split)].insert(id);
    }return out;
  }();
  const auto it=members.find(owner);if(it==members.end())Fail("native_entity_body_catalog");
  for(const auto &id:it->second)if(!v["fields"].HasMember(id.c_str()))Fail("native_entity_incomplete_fields");
}
bool HasStatus(const Value &v,const char *status){return v.IsObject()&&v.HasMember("status")&&v["status"]==status;}
// 只检查确实用于初始化的关系；合法null与尚未收到的目标分别保留。
void InitialReference(NativeEntityTarget::Object &o,const Value &v,const Scope &scope,bool required){
  if(HasStatus(v,"Null")){if(required)Fail("native_entity_required_owner_null");return;}
  if(!HasStatus(v,"KnownCurrent"))Fail("native_entity_initialization_reference");
  // 本方使用池构造+公共被动Init，配置加载不读取Owner；实体全部建体后再绑定关系。
  // 仍校验依赖身份，但不把普通Spawner/Parent环误判为必须重演派生Init的创建环。
  Key(At(v,"target"),scope,1);
}
// 所有引用和精确整数在原生修改前解码；关系数量与主体数量共用一份场景预算。
void Values(const Value &v,NativeEntityTarget::Object &o,const Scope &scope,std::size_t &refs){
  if(v.IsArray()){for(const auto &e:v.GetArray())Values(e,o,scope,refs);return;}
  if(!v.IsObject())return;
  if(v.HasMember("type")&&v["type"]=="int64"){Integer(v);return;}
  if(v.HasMember("type")&&v["type"]=="bits128"){
    // Lua将两个uint64以有符号十进制输出；按位解释，禁止经过double。
    for(const char *k:{"low","high"}){const auto s=String(At(v,k));std::int64_t signed_value=0;std::uint64_t unsigned_value=0;
      const auto a=std::from_chars(s.data(),s.data()+s.size(),signed_value),b=std::from_chars(s.data(),s.data()+s.size(),unsigned_value);
      if((a.ec!=std::errc{}||a.ptr!=s.data()+s.size())&&(b.ec!=std::errc{}||b.ptr!=s.data()+s.size()))Fail("native_entity_bits128");}
    return;
  }
  if(v.HasMember("type")&&v["type"]=="reference"){
    if(++refs>131072)Fail("native_entity_dependency_budget");
    if(HasStatus(v,"KnownCurrent")){
      const auto &target=At(v,"target");const auto space=Unsigned(At(target,"space"));
      const auto revision=Unsigned(At(v,"shape_revision"));if(!revision)Fail("native_entity_reference_shape");
      auto remember=[&](auto &shapes,std::uint64_t id){auto [it,inserted]=shapes.emplace(id,revision);
        if(!inserted&&it->second!=revision)Fail("native_entity_reference_shape_conflict");};
      if(space==1){const auto id=Key(target,scope,1);o.entities.insert(id);remember(o.entity_shapes,id);}
      else if(space==2){const auto id=Key(target,scope,2);o.grids.insert(id);remember(o.grid_shapes,id);}
      else Fail("native_entity_reference_space");
    }else if(!HasStatus(v,"Null")&&!HasStatus(v,"NotApplicable"))Fail("native_entity_reference_unavailable");
    return;
  }
  if(v.HasMember("fields")){
    const auto &fields=v["fields"];if(!fields.IsObject())Fail("native_entity_fields");
    for(auto i=fields.MemberBegin();i!=fields.MemberEnd();++i){
      const auto *policy=Policy(String(i->name));
      // NativeText目录之外的具名补充已有独立J273来源；其他未知字段禁止悄悄应用。
      const auto name=String(i->name);
      if(!policy&&name.rfind("NativeText.",0)!=0)Fail("native_entity_undeclared_member");
      if(policy&&At(*policy,"ownership")!="ReplicatedState"){
        if(!HasStatus(i->value,String(At(*policy,"ownership")).c_str()))Fail("native_entity_ownership");
        continue;
      }
      Values(i->value,o,scope,refs);
    }
    // fields已经检查，下面继续处理资源描述等兄弟成员，不能重复累加引用。
    for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)if(i->name!="fields")Values(i->value,o,scope,refs);
    return;
  }
  for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)Values(i->value,o,scope,refs);
}
}

NativeEntityTarget PrepareNativeEntity(const Value &model){
  ValidateModel(model,Limits{32u<<20,4194304,64,2u<<20});
  NativeEntityTarget out;out.scope=ReadScope(model["scope"]);out.revision=Unsigned(model["revision"]);out.watermark=Unsigned(model["event_watermark"]);
  const auto &component=At(model["components"],"entity");
  if(At(component,"coverage")!="Complete"||At(component,"membership_complete")!=true)Fail("native_entity_partial_membership");
  for(const char *name:{"room","grid"}){if(!model["components"].HasMember(name))Fail(name==std::string("room")?"native_entity_waiting_room":"native_entity_waiting_grid");const auto &domain=At(model["components"],name);
    if(At(domain,"coverage")!="Complete"||At(domain,"membership_complete")!=true)Fail("native_entity_incomplete_scene");}
  const auto &state=At(component,"state"),&membership=At(state,"membership");
  if(At(membership,"complete")!=true)Fail("native_entity_partial_membership");
  const auto &objects=At(state,"objects");if(!objects.IsArray()||objects.Size()>16384||Integer(At(membership,"count"))!=objects.Size())Fail("native_entity_object_budget");
  out.component=Copy(component);out.bytes=Text(component).size();
  const std::set<std::string> classes={"EntityPlayer","EntityNPC","EntityTear","EntityProjectile","EntityBomb","EntityLaser","EntityKnife","EntityPickup","EntityFamiliar","EntityEffect","EntitySlot","NativeText","NativeDummy"};
  // PlayerSources已有256个关系主体预算，含备用/隐藏身体；活动roster仍最多8，不增加参与者。
  const std::map<std::string,unsigned> capacities={{"EntityPlayer",256},{"EntityNPC",512},{"EntityTear",512},{"EntityProjectile",512},
    {"EntityBomb",64},{"EntityLaser",128},{"EntityKnife",64},{"EntityPickup",512},{"EntityFamiliar",64},{"EntityEffect",1024},{"EntitySlot",128},{"NativeText",128},{"NativeDummy",128}};
  std::map<std::string,unsigned> counts;
  for(const auto &v:objects.GetArray()){
    NativeEntityTarget::Object o;const auto &subject=At(v,"subject");
    if(!HasStatus(subject,"KnownCurrent"))Fail("native_entity_subject_not_current");
    o.id=Key(At(subject,"target"),out.scope,1);o.shape=Unsigned(At(subject,"shape_revision"));if(!o.shape)Fail("native_entity_shape");
    o.category=String(At(v,"class"));if(!classes.count(o.category))Fail("native_entity_class");o.player=o.category=="EntityPlayer";
    if(++counts[o.category]>capacities.at(o.category))Fail("native_entity_pool_capacity");
    Fields(At(v,"common"),"Entity");
    if(o.category!="NativeText"&&o.category!="NativeDummy")Fields(At(v,"body"),o.category);
    if(v.HasMember("special")){if(o.category!="EntityNPC")Fail("native_entity_special_class");Fields(v["special"],"EntityDelirium");}
    const auto &render=At(v,"native_render");if(Integer(At(render,"version"))!=1||U32(At(render,"order_fragment"))>1023)Fail("native_entity_render_order_fragment");
    if(o.category=="EntityKnife"){const auto &z=At(render,"knife_sort_z");if(!z.IsNumber()||!std::isfinite(z.GetDouble())||std::abs(z.GetDouble())>3.402823466e38)Fail("native_knife_sort_layer");}
    const auto &common=At(At(v,"common"),"fields");
    o.type=U32(At(common,"Entity.Type"));o.variant=U32(At(common,"Entity.Variant"));o.subtype=U32(At(common,"Entity.SubType"));o.seed=Seed32(At(common,"Entity.InitSeed"));
    // Dummy的真实原生构造Type为0；seed是已选后态，不把合法0改成新随机seed。
    if((!o.type&&o.category!="NativeDummy")||(o.player!=(o.type==1)))Fail("native_entity_type_seed");
    const std::map<std::string,unsigned> types={{"EntityTear",2},{"EntityFamiliar",3},{"EntityBomb",4},
      {"EntityPickup",5},{"EntitySlot",6},{"EntityLaser",7},{"EntityKnife",8},{"EntityProjectile",9},{"EntityEffect",1000},{"NativeText",9001}};
    if(const auto it=types.find(o.category);it!=types.end()&&o.type!=it->second)Fail("native_entity_requested_class");
    if(o.category=="EntityNPC"&&(o.type<10||o.type>=1000))Fail("native_entity_npc_type");
    if(o.category=="EntityFamiliar"||o.category=="EntityPickup"){
      const auto &native=At(v,"native_render");if(Integer(At(native,"version"))!=1||!At(native,"sprites").IsObject())Fail("native_entity_render_state");
      if(o.category=="EntityFamiliar"&&o.variant==48){const auto &present=At(native,"isaacs_body");if(!present.IsBool())Fail("native_familiar_body_state");
        const auto &ids=At(native,"body_ids");if(!ids.IsArray()||ids.Size()!=(present.GetBool()?4u:0u))Fail("native_familiar_body_state");
        if(present.GetBool())for(unsigned i=0;i<5;++i)At(native["sprites"],(std::string("isaacs_body_")+std::to_string(i)).c_str());}
      if(o.category=="EntityFamiliar"&&o.variant==231){for(const char *side:{"before","after"}){const auto count=Integer(At(native,(std::string("abyss_")+side).c_str()));
        if(count<0||count>256)Fail("native_familiar_abyss_budget");for(std::int64_t i=0;i<count;++i)At(native["sprites"],(std::string("abyss_")+side+"_"+std::to_string(i)).c_str());}}
      if(o.category=="EntityFamiliar"){
        const auto &weapon=At(native,"weapon");if(!At(weapon,"present").IsBool())Fail("native_weapon_presence");
        const std::map<std::string,unsigned> layouts={{"base",1},{"single",2},{"brimstone",2},{"knife",5},{"bone",7},{"urn",2}};
        const auto name=String(At(weapon,"layout"));const auto &refs=At(weapon,"refs");if(!refs.IsArray())Fail("native_weapon_references");
        if(weapon["present"].GetBool()){auto layout=layouts.find(name);if(layout==layouts.end()||refs.Size()!=layout->second)Fail("native_weapon_references");
          Fields(At(At(At(v,"body"),"fields"),"EntityFamiliar.GetWeapon"),"Weapon");
        }else if(!name.empty()||!refs.Empty())Fail("native_weapon_presence");
      }
      if(o.category=="EntityPickup")At(native["sprites"],"alternate_pedestal");
    }
    if(o.category=="NativeText"){
      const auto &f=At(At(v,"body"),"fields");Integer(At(f,"NativeText.counter"));Integer(At(f,"NativeText.duration"));
      for(const char *channel:{"red","green","blue","alpha"}){const auto &n=At(f,(std::string("NativeText.")+channel).c_str());
        if(!n.IsNumber()||!std::isfinite(n.GetDouble())||std::abs(n.GetDouble())>3.402823466e38)Fail("native_entity_text_color");}
      const auto word=Integer(At(f,"NativeText.color_word"));if(word<INT32_MIN||word>INT32_MAX)Fail("native_entity_text_color_word");
      const auto &text=At(f,"NativeText.text");
      if(text.IsString()){if(text.GetStringLength()>255||String(text).find('\0')!=std::string::npos)Fail("native_entity_text_budget");}
      else{if(At(text,"type")!="bytes"||At(text,"encoding")!="lua-string-bytes")Fail("native_entity_text_encoding");
        const auto hex=String(At(text,"hex"));if(hex.size()>510||hex.size()%2)Fail("native_entity_text_budget");
        for(char c:hex)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))Fail("native_entity_text_encoding");
        for(std::size_t i=0;i<hex.size();i+=2)if(hex.substr(i,2)=="00")Fail("native_entity_text_terminator");}
    }
    for(const char *name:{"Entity.Position","Entity.Velocity","Entity.SizeMulti","Entity.SpriteScale"})Vector(At(common,name));
    for(const char *name:{"Entity.Exists","Entity.IsDead","Entity.Visible"})if(!At(common,name).IsBool())Fail("native_entity_boolean");
    if(o.player){
      // 20尚未接管的身体私有状态仍保存在完整component，19只解析公共载体和实际入向依赖。
      // 不因Player背包/历史等未应用字段中的远端关系要求19重建整套Player域。
      Values(subject,o,out.scope,out.dependencies);Values(v["common"],o,out.scope,out.dependencies);
    }else Values(v,o,out.scope,out.dependencies);
    if(o.category=="EntityFamiliar")InitialReference(o,At(At(At(v,"body"),"fields"),"EntityFamiliar.Player"),out.scope,true);
    if(o.category=="EntityKnife")InitialReference(o,At(common,"Entity.Parent"),out.scope,true);
    // Spawner在native初始化时被读取，不能先用空对象构造再猜测补回。
    InitialReference(o,At(common,"Entity.SpawnerEntity"),out.scope,false);
    o.state=Copy(v);if(!out.objects.emplace(o.id,std::move(o)).second)Fail("native_entity_duplicate_key");
  }
  std::map<std::uint64_t,std::uint64_t> grids;
  const auto &grid=At(At(At(model["components"],"grid"),"state"),"objects");
  if(!grid.IsArray()||grid.Size()>448)Fail("native_entity_grid_budget");
  for(const auto &g:grid.GetArray()){const auto &subject=At(g,"subject");
    if(!grids.emplace(Key(At(subject,"target"),out.scope,2),Unsigned(At(subject,"shape_revision"))).second)Fail("native_entity_duplicate_grid");}
  for(const auto &g:grid.GetArray())if(At(g,"class")=="GridEntityStatue"){
    const auto &relations=At(g,"native_relations");if(Integer(At(relations,"version"))!=1)Fail("native_grid_relation_version");
    NativeEntityTarget::Object dependency;Values(At(relations,"display_entity"),dependency,out.scope,out.dependencies);
    for(auto id:dependency.entities){auto it=out.objects.find(id);if(it==out.objects.end())Fail("native_entity_waiting_statue_display");
      if(it->second.shape!=dependency.entity_shapes.at(id))Fail("native_entity_waiting_statue_shape");if(it->second.player)out.player_dependencies.insert(id);}
  }
  // 搬起的描述属于同窗口Effect而非活动Grid槽；校验完整owner/shape，防止只按id串到别的代次。
  std::set<std::uint64_t> lifted_owners;const auto &lifted=At(At(At(model["components"],"grid"),"state"),"lifted_descriptors");
  if(!lifted.IsArray()||lifted.Size()>1024)Fail("native_entity_lifted_budget");
  for(const auto &entry:lifted.GetArray()){
    const auto &owner=At(entry,"owner");if(!HasStatus(owner,"KnownCurrent"))Fail("native_entity_lifted_owner");
    const auto id=Key(At(owner,"target"),out.scope,1);auto found=out.objects.find(id);if(found==out.objects.end())Fail("native_entity_waiting_lifted_owner");
    if(found->second.category!="EntityEffect"||found->second.variant!=136||found->second.shape!=Unsigned(At(owner,"shape_revision"))||!lifted_owners.insert(id).second)Fail("native_entity_lifted_owner");
    const auto &descriptor=At(entry,"descriptor");if(At(descriptor,"type")!="GridEntityDesc")Fail("native_entity_lifted_descriptor");const auto &f=At(descriptor,"fields");
    for(const char *name:{"Type","Variant","State","SpawnCount","SpawnSeed","VarData","VariableSeed"}){const auto value=Integer(At(f,(std::string("GridEntityDesc.")+name).c_str()));if(value<INT32_MIN||value>UINT32_MAX)Fail("native_entity_lifted_value");}
    if(!At(f,"GridEntityDesc.Initialized").IsBool())Fail("native_entity_lifted_value");
  }
  for(const auto &[id,o]:out.objects)if(o.category=="EntityEffect"&&o.variant==136&&!lifted_owners.count(id))Fail("native_entity_waiting_lifted_descriptor");
  for(const auto &[id,o]:out.objects){
    for(auto ref:o.entities){const auto it=out.objects.find(ref);if(it==out.objects.end())Fail("native_entity_waiting_entity");
      if(it->second.shape!=o.entity_shapes.at(ref))Fail("native_entity_waiting_shape");
      if(it->second.player&&!o.player)out.player_dependencies.insert(ref);}
    for(auto ref:o.grids){const auto it=grids.find(ref);if(it==grids.end())Fail("native_entity_waiting_grid");
      if(it->second!=o.grid_shapes.at(ref))Fail("native_entity_waiting_grid_shape");}
    if(o.category=="EntityFamiliar"){const auto &weapon=At(At(o.state,"native_render"),"weapon");if(At(weapon,"present").GetBool()){
      const auto &owner=At(At(At(At(At(o.state,"body"),"fields"),"EntityFamiliar.GetWeapon"),"fields"),"Weapon.GetOwner");
      if(!HasStatus(owner,"KnownCurrent"))Fail("native_weapon_owner");const auto &target=out.objects.at(Key(At(owner,"target"),out.scope,1));
      if(!target.player&&target.category!="EntityFamiliar")Fail("native_weapon_owner_class");}}
  }
  const auto &players=At(state,"player_context");if(Integer(At(players,"version"))!=1)Fail("native_entity_player_context_version");
  const auto &roster=At(players,"roster"),&esau=At(players,"esau");
  if(!roster.IsArray()||roster.Empty()||roster.Size()>8||!esau.IsArray()||esau.Size()!=4)Fail("native_entity_player_context_budget");
  auto player=[&](const Value &ref,bool nullable){
    if(nullable&&HasStatus(ref,"Null"))return std::uint64_t(0);
    if(!HasStatus(ref,"KnownCurrent"))Fail("native_entity_player_context_reference");
    const auto id=Key(At(ref,"target"),out.scope,1);const auto it=out.objects.find(id);
    if(it==out.objects.end()||!it->second.player||it->second.shape!=Unsigned(At(ref,"shape_revision")))Fail("native_entity_player_context_missing");
    out.player_dependencies.insert(id);return id;
  };
  std::set<std::uint64_t> roster_ids;
  for(const auto &ref:roster.GetArray()){const auto id=player(ref,false);if(!roster_ids.insert(id).second)Fail("native_entity_duplicate_roster");out.player_roster.push_back(id);}
  for(unsigned i=0;i<4;++i)out.player_esau[i]=player(esau[i],true);
  const auto &render_order=At(state,"render_order");if(!render_order.IsArray()||render_order.Size()>4096)Fail("native_entity_render_order_budget");
  std::set<std::uint64_t> rendered;
  for(const auto &ref:render_order.GetArray()){
    if(++out.dependencies>131072)Fail("native_entity_dependency_budget");if(!HasStatus(ref,"KnownCurrent"))Fail("native_entity_render_order_reference");
    const auto id=Key(At(ref,"target"),out.scope,1);auto found=out.objects.find(id);if(found==out.objects.end())Fail("native_entity_waiting_render_order");
    if(found->second.shape!=Unsigned(At(ref,"shape_revision")))Fail("native_entity_waiting_render_shape");if(!rendered.insert(id).second)Fail("native_entity_render_order_duplicate");
    if(!found->second.player)out.render_order.push_back(id);
  }
  for(const auto &[id,o]:out.objects)if(!o.player&&!rendered.count(id))Fail("native_entity_render_membership");
  // 必要身体公共Parent/Child/Target也可能借用另一个未列入roster的身体；桥接依赖必须传递闭合。
  std::vector<std::uint64_t> player_queue(out.player_dependencies.begin(),out.player_dependencies.end());
  for(std::size_t i=0;i<player_queue.size();++i)for(auto ref:out.objects.at(player_queue[i]).entities)
    if(out.objects.at(ref).player&&out.player_dependencies.insert(ref).second)player_queue.push_back(ref);
  if(out.player_dependencies.size()>256)Fail("native_entity_player_carrier_budget");
  // 只对初始化依赖排序，普通Parent/Child环在完整建体后第二阶段恢复。
  // 初始化环没有已核定的两阶段ABI时明确拒绝，不以任意顺序带null创建。
  std::map<std::uint64_t,std::size_t> incoming;
  std::map<std::uint64_t,std::vector<std::uint64_t>> dependents;
  std::set<std::uint64_t> ready;
  for(const auto &[id,o]:out.objects){incoming[id]=o.initialization.size();if(o.initialization.empty())ready.insert(id);
    for(auto ref:o.initialization)dependents[ref].push_back(id);}
  while(!ready.empty()){const auto id=*ready.begin();ready.erase(ready.begin());out.creation_order.push_back(id);
    for(auto next:dependents[id])if(--incoming[next]==0)ready.insert(next);}
  if(out.creation_order.size()!=out.objects.size())Fail("native_entity_initialization_cycle");
  // 全模型仍受32MiB单包上限约束；Entity序列化部分最多24MiB。
  // 这不是原生资源/在途退休的总内存声明，后者由共享scene owner另计。
  if(out.bytes>24u<<20)Fail("native_entity_scene_budget");return out;
}

NativeEntityPlan PlanNativeEntity(const NativeEntityTarget *current,const NativeEntityTarget &target,bool healthy){
  NativeEntityPlan plan;plan.reset=!healthy;
  if(current){
    if(current->scope!=target.scope)Fail("native_entity_scope_changed");
    if(target.revision<current->revision){plan.disposition=NativeEntityDisposition::Stale;return plan;}
    if(target.revision==current->revision){
      if(current->component!=target.component||current->watermark!=target.watermark)Fail("native_entity_revision_conflict");
      if(healthy){plan.disposition=NativeEntityDisposition::Duplicate;return plan;}
    }
    if(target.watermark<current->watermark)Fail("native_entity_watermark_regression");
    for(const auto &[id,next]:target.objects){const auto it=current->objects.find(id);if(it==current->objects.end())continue;
      if(it->second.player!=next.player)Fail("native_entity_owner_changed");
      if(next.shape<it->second.shape)Fail("native_entity_shape_regression");
      const auto &before=it->second;if((before.type!=next.type||before.variant!=next.variant||before.subtype!=next.subtype||before.seed!=next.seed||before.category!=next.category)&&before.shape==next.shape)Fail("native_entity_shape_not_advanced");}
  }
  const auto *native=healthy?current:nullptr;
  if(native)for(const auto &[id,o]:native->objects)if(!o.player&&!target.objects.count(id))plan.remove.push_back(id);
  for(auto id:target.creation_order){const auto &next=target.objects.at(id);if(next.player)continue;
    const auto before=native?native->objects.find(id):target.objects.end();
    if(!native||before==native->objects.end()){plan.create.push_back(id);continue;}
    const auto &old=before->second;
    if(next.shape<old.shape)Fail("native_entity_shape_regression");
    const bool identity=old.type!=next.type||old.variant!=next.variant||old.subtype!=next.subtype||old.seed!=next.seed||old.category!=next.category;
    if(identity&&next.shape==old.shape)Fail("native_entity_shape_not_advanced");
    if(old.player)Fail("native_entity_owner_changed");
    const bool body_change=next.category=="EntityFamiliar"&&next.variant==48&&
      At(At(old.state,"native_render"),"isaacs_body")!=At(At(next.state,"native_render"),"isaacs_body");
    if(next.state!=old.state){plan.change.push_back(id);if(identity||old.shape!=next.shape||body_change)plan.rebuild.push_back(id);}
  }
  return plan;
}
} // namespace cs::replica
