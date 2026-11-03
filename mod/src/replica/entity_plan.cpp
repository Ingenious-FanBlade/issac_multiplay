#include "replica/entity_plan.h"
#include <algorithm>
#include <map>
#include <set>
#include <type_traits>

namespace cs::replica {
namespace {
using Index=std::map<SubjectKey,unsigned>;
[[noreturn]] void Fail(const char *reason){throw ContractError(std::string("entity_plan:")+reason);}
const Value *Fields(const EntityObjectTarget &o){return o.common.value.IsObject()&&o.common.value.HasMember("fields")?&o.common.value["fields"]:nullptr;}
const Value *Field(const EntityObjectTarget &o,const char *name){const auto *f=Fields(o);return f&&f->HasMember(name)?&(*f)[name]:nullptr;}
bool Same(const Value *a,const Value *b){return a&&b?*a==*b:a==b;}
bool Current(const EntityObjectTarget &o){return o.presence==EntityMemberPresence::Current;}
Index Members(const EntityTarget &t){
  if(!t.membership_complete)Fail("complete_membership_required_for_absence");
  Index out;
  for(unsigned i=0;i<t.objects.size();++i){const auto &o=t.objects[i];
    if(o.key.run!=t.scope.run||o.key.space!=1||!o.key.id||!out.emplace(o.key,i).second)Fail("key_scope_or_duplicate");
    if(o.presence==EntityMemberPresence::NotCaptured)Fail("uncaptured_member");
    if(Current(o)&&(!o.classification_supported||!o.exists_known||!o.dead_known))Fail("classification_or_existence_unknown");
  }return out;
}
bool Shape(const EntityObjectTarget &a,const EntityObjectTarget &b){
  if(a.shape_revision!=b.shape_revision||a.category!=b.category||a.delirium!=b.delirium)return true;
  for(const char *id:{"Entity.Type","Entity.Variant","Entity.SubType","Entity.InitSeed"})if(!Same(Field(a,id),Field(b,id)))return true;
  return false;
}
bool Resources(const Value &v,const Value &before){
  // 完整资源子树比较包含精确phase；空数组/Null/回到B的值同样触发安装。
  if(v.IsObject()&&v.HasMember("type")&&v["type"].IsString()&&std::string(v["type"].GetString())=="resource_pose")return v!=before;
  if(v.GetType()!=before.GetType())return v!=before;
  if(v.IsArray()){if(v.Size()!=before.Size())return true;for(unsigned i=0;i<v.Size();++i)if(Resources(v[i],before[i]))return true;}
  if(v.IsObject()){if(v.MemberCount()!=before.MemberCount())return true;for(auto i=v.MemberBegin();i!=v.MemberEnd();++i){if(!before.HasMember(i->name))return true;if(Resources(i->value,before[i->name]))return true;}}
  return false;
}
}
EntityDependency DescribeEntityDependency(const EntityObjectTarget &o,const EntityReferenceTarget &r){
  EntityDependency d;d.source=o.key;d.path=r.path;d.status=r.status;d.target=r.target;d.has_target=r.has_target;d.is_null=r.status=="Null";
  // path中的成员名来自目录，不从未经解释的数值Type猜关系类型。
  d.member=EntityReferenceMember(r.path);
  if(r.owner_qualified){d.contract=r.contract;d.phase=EntityDependencyPhase::ValueOnly;
    d.kind=r.contract=="isaac.cs.grid:lifted_descriptors"?EntityDependencyKind::Descriptor:EntityDependencyKind::AuthorityValue;return d;}
  if(d.has_target&&d.target.space!=1){d.kind=d.target.space==2?EntityDependencyKind::Grid:d.target.space==7?EntityDependencyKind::Room:EntityDependencyKind::Descriptor;return d;}
  const auto *p=EntityApplicationPolicy(d.member);std::string returns;
  if(p&&p->HasMember("returns"))returns=(*p)["returns"].GetString();
  if(returns=="EntityPlayer"){d.kind=EntityDependencyKind::Player;d.expected=EntityCategory::Player;}
  else if(returns=="EntityNPC")d.expected=EntityCategory::NPC;
  else if(returns=="EntityKnife")d.expected=EntityCategory::Knife;
  else if(returns=="EntityPickup"||returns=="EntityPickup, boolean")d.expected=EntityCategory::Pickup;
  else if(returns=="EntityEffect")d.expected=EntityCategory::Effect;
  if(d.member=="Entity.GetLastChild"||d.member=="Entity.GetLastParent"){
    d.kind=EntityDependencyKind::Historical;d.phase=EntityDependencyPhase::ValueOnly;
  }else if(d.member=="EntityNPC.GetPlayerTarget"||d.member=="EntityNPC.Pathfinder"||d.member.rfind("EntityNPC.QueryNPCs",0)==0||d.member=="EntityFamiliar.GetActiveWeaponEntity"||d.member=="EntityFamiliar.GetWeapon"||d.member=="EntityFamiliar.GetPathfinder"||d.member=="EntityFamiliar.GetItemConfig"){
    d.kind=EntityDependencyKind::AuthorityValue;d.phase=EntityDependencyPhase::ValueOnly;
  }
  // Familiar Init真实读取Spawner；Knife的Parent链是owner/draw前置，不以首Player补齐。
  if((o.category==EntityCategory::Familiar&&d.member=="Entity.SpawnerEntity")||
     (o.category==EntityCategory::Knife&&d.member=="Entity.Parent"))d.phase=EntityDependencyPhase::BeforeInit;
  return d;
}
EntityApplyPlan PlanEntityTarget(std::shared_ptr<const EntityTarget> t,std::shared_ptr<const EntityTarget> a,EntityPlanLimits limits){
  if(!limits.objects||limits.objects>16384||!limits.steps||limits.steps>131073||!limits.dependencies||limits.dependencies>262144)Fail("limits_only_tightening_allowed");
  if(!t||!t->revision||t->objects.size()>limits.objects)Fail("target_or_budget");
  EntityApplyPlan out;out.target=std::move(t);out.applied=std::move(a);
  const auto &target=*out.target;const auto desired=Members(target);Index old;
  out.external_players=target.player_keys;
  if(out.applied){const auto &before=*out.applied;
    if(!(before.scope==target.scope))Fail("scope_change_requires_scene_retirement");
    if(target.revision<before.revision)Fail("stale_revision");
    if(target.revision==before.revision){if(target.source!=before.source)Fail("revision_content_conflict");out.duplicate=true;return out;}
    old=Members(before);
    if(std::set<SubjectKey>(target.player_keys.begin(),target.player_keys.end())!=std::set<SubjectKey>(before.player_keys.begin(),before.player_keys.end()))Fail("player_membership_change_requires_scene_player_stage");
    if(target.source["state"]==before.source["state"]){out.duplicate=true;return out;}
  }
  std::set<SubjectKey> replace,remove;
  for(const auto &[key,index]:old){const auto &before=out.applied->objects[index];if(!Current(before)||before.category==EntityCategory::Player)continue;
    const auto next=desired.find(key);
    if(next==desired.end()||!Current(target.objects[next->second]))remove.insert(key);
    else {const auto &after=target.objects[next->second];
      if(after.shape_revision<before.shape_revision)Fail("shape_revision_regressed");
      if(before.exists_known&&!before.exists&&after.exists&&after.shape_revision==before.shape_revision)Fail("nonexisting_use_cannot_revive_without_replacement");
      if(Shape(after,before)){
        if(after.shape_revision==before.shape_revision)Fail("shape_changed_without_revision");replace.insert(key);
      }
    }
  }
  // 先撤所有旧关系再退休，避免同Key换实例或后置环仍借用旧use；原生端幂等跳过空边。
  if(out.applied)for(const auto &[key,index]:old){const auto &o=out.applied->objects[index];
    if(Current(o)&&o.category!=EntityCategory::Player)out.steps.push_back({EntityAction::Unlink,key,index,true});}
  for(const auto &key:remove)out.steps.push_back({EntityAction::Remove,key,old.at(key),true});
  for(const auto &key:replace)out.steps.push_back({EntityAction::Remove,key,old.at(key),true});
  std::map<SubjectKey,std::set<SubjectKey>> prerequisites;
  for(const auto &[key,index]:desired){const auto &o=target.objects[index];if(!Current(o))continue;
    if(o.category==EntityCategory::Player)continue;
    for(const auto &r:o.references){auto d=DescribeEntityDependency(o,r);
      if(out.dependencies.size()>=limits.dependencies)Fail("dependency_budget");
      if(!d.is_null&&d.phase!=EntityDependencyPhase::ValueOnly){
        if(!d.has_target||d.status!="KnownCurrent")Fail("required_reference_unresolved");
        if(d.target.run!=target.scope.run)Fail("reference_cross_run");
        if(d.target.space==1){const auto hit=desired.find(d.target);
          if(hit==desired.end()||!Current(target.objects[hit->second]))Fail("required_reference_missing");
          const auto &other=target.objects[hit->second];
          if(!other.exists)Fail("reference_to_nonexisting_member");
          if(d.expected!=EntityCategory::Unknown&&d.expected!=other.category)Fail("reference_category_mismatch");
          if(other.category==EntityCategory::Player)d.kind=EntityDependencyKind::Player;
          else if(d.phase==EntityDependencyPhase::BeforeInit)prerequisites[key].insert(d.target);
        }
      }out.dependencies.push_back(std::move(d));
    }
  }
  // 只对构造前置拓扑排序；普通Parent/Child关系允许两阶段环。
  std::vector<unsigned> order;std::set<SubjectKey> ready;
  for(const auto &key:out.external_players)ready.insert(key);
  for(std::size_t pass=0;pass<=desired.size();++pass){bool progress=false;
    for(const auto &[key,index]:desired){const auto &o=target.objects[index];if(!Current(o)||ready.count(key))continue;
      const auto &deps=prerequisites[key];if(!std::all_of(deps.begin(),deps.end(),[&](auto k){return ready.count(k)!=0;}))continue;
      ready.insert(key);order.push_back(index);progress=true;
    }if(!progress)break;
  }
  for(const auto &[key,index]:desired)if(Current(target.objects[index])&&!ready.count(key))Fail("constructor_dependency_cycle");
  for(const auto index:order){const auto &o=target.objects[index];const auto prev=old.find(o.key);
    const bool create=prev==old.end()||!Current(out.applied->objects[prev->second])||replace.count(o.key);
    if(create){out.steps.push_back({EntityAction::Acquire,o.key,index});out.steps.push_back({EntityAction::Initialize,o.key,index});}
    bool changed=create;
    if(!create){const auto &b=out.applied->objects[prev->second];
      changed=o.common.value!=b.common.value||o.special.value!=b.special.value||o.payload.index()!=b.payload.index();
      std::visit([&](const auto &v){using B=std::decay_t<decltype(v)>;if(const auto *bv=std::get_if<B>(&b.payload))changed|=v.fields.value!=bv->fields.value;},o.payload);
    }
    if(changed)out.steps.push_back({EntityAction::Fields,o.key,index});
    const auto &row=target.source["state"]["objects"][index];
    if(create||Resources(row,out.applied->source["state"]["objects"][prev->second]))out.steps.push_back({EntityAction::Resources,o.key,index});
    if(create)out.steps.push_back({EntityAction::Publish,o.key,index});
  }
  for(const auto index:order)out.steps.push_back({EntityAction::Link,target.objects[index].key,index});
  for(const auto index:order)out.steps.push_back({EntityAction::Check,target.objects[index].key,index});
  out.steps.push_back({EntityAction::CheckAll});
  if(out.steps.size()>limits.steps)Fail("step_budget");return out;
}
} // namespace cs::replica
