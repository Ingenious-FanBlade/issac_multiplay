#include "replica/room_grid_plan.h"
#include <algorithm>
#include <map>
#include <set>
#include <string_view>

namespace cs::replica {
namespace {
// required_revision/sample_revision属于传输/采样元数据；内容回到基线仍须从真实A比较。
bool Same(const Value &a,const Value &b){
  if(a.IsObject()&&b.IsObject()){
    const auto metadata=[](const Value &name){const std::string_view s(name.GetString(),name.GetStringLength());return s=="required_revision"||s=="sample_revision";};
    unsigned ac=0,bc=0;for(auto i=a.MemberBegin();i!=a.MemberEnd();++i)if(!metadata(i->name)){
      ++ac;if(!b.HasMember(i->name)||!Same(i->value,b[i->name]))return false;}
    for(auto i=b.MemberBegin();i!=b.MemberEnd();++i)if(!metadata(i->name))++bc;return ac==bc;
  }
  if(a.IsArray()&&b.IsArray()){if(a.Size()!=b.Size())return false;for(unsigned i=0;i<a.Size();++i)if(!Same(a[i],b[i]))return false;return true;}
  return a==b;
}
bool Construction(const GridTarget &a,const GridTarget &b){return a.type==b.type&&a.variant==b.variant&&a.descriptor.spawn_seed==b.descriptor.spawn_seed&&a.descriptor.variable_seed==b.descriptor.variable_seed;}
bool RoomReuse(const RoomGridTarget &a,const RoomGridTarget &b){
  if(a.scope!=b.scope||a.room.key!=b.room.key||a.room.floor!=b.room.floor||a.room.descriptor!=b.room.descriptor||
     a.room.current_index!=b.room.current_index||a.room.dimension!=b.room.dimension||a.room.descriptors.size()!=b.room.descriptors.size())return false;
  if(a.room.layout!=b.room.layout)return false;
  std::map<SubjectKey,const RoomDescriptorTarget *> before;for(const auto &d:a.room.descriptors)before.emplace(d.key,&d);
  for(const auto &d:b.room.descriptors){const auto found=before.find(d.key);if(found==before.end())return false;const auto &old=*found->second;
    if(old.location!=d.location||old.data!=d.data||old.override_data!=d.override_data)return false;
    // 查找键相同仍可能换Spawns/版本/配置内容；完整资源子树改变时不能只按8个标量复用。
    for(const char *name:{"RoomDescriptor.Data","RoomDescriptor.OverrideData"})
      if(!Same(a.DescriptorState(old)["fields"][name],b.DescriptorState(d)["fields"][name]))return false;
  }
  return true;
}
}
PlanCancellation CancelRoomGridPlan(bool written,unsigned prefix) noexcept {return written||prefix?PlanCancellation::RetireNative:PlanCancellation::PreserveApplied;}
// 一次只拥有一个不可变T；调用方可在纯值阶段替换整个计划，执行中不得修改其参数。
RoomGridPlan PlanRoomGridTarget(std::shared_ptr<const RoomGridTarget> target,std::shared_ptr<const RoomGridTarget> applied,RoomGridLimits limits){
  if(!limits.steps||limits.steps>8192||!limits.dependency_edges||limits.dependency_edges>1048576)throw ContractError("room_grid_plan:limits_only_tightening_allowed");
  if(!target)throw ContractError("room_grid_plan:target_missing");RoomGridPlan plan;plan.target=std::move(target);plan.applied=std::move(applied);
  const auto &t=*plan.target;const auto *a=plan.applied.get();
  if(t.grids.size()>limits.grids||t.room.descriptors.size()>limits.descriptors)throw ContractError("room_grid_plan:target_budget");
  if(a&&a->scope==t.scope){
    if(t.revision<a->revision)throw ContractError("room_grid_plan:stale_revision");
    if(t.revision==a->revision){if(t.source!=a->source)throw ContractError("room_grid_plan:same_revision_conflict");plan.room=RoomPlanKind::Duplicate;return plan;}
  }
  const bool rebuild=!a||!RoomReuse(*a,t);plan.room=rebuild?RoomPlanKind::Rebuild:
      Same(a->source["components"]["room"]["state"],t.source["components"]["room"]["state"])?RoomPlanKind::Keep:RoomPlanKind::Change;
  std::uint64_t edges=0;
  auto step=[&](RoomGridOperation op,SubjectKey key,std::vector<unsigned> after={},std::vector<SubjectKey> external={}){
    if(plan.steps.size()>=limits.steps)throw ContractError("room_grid_plan:step_budget");const unsigned id=unsigned(plan.steps.size());
    for(auto dependency:after)if(dependency>=id)throw ContractError("room_grid_plan:dependency_order");
    edges+=after.size()+external.size();if(edges>limits.dependency_edges)throw ContractError("room_grid_plan:dependency_budget");
    plan.steps.push_back({op,key,std::move(after),std::move(external)});return id;
  };
  const auto geometry=step(RoomGridOperation::RoomGeometry,t.room.key);
  std::map<SubjectKey,const GridTarget *> old,desired;
  if(a&&!rebuild)for(const auto &g:a->grids)old.emplace(g.key,&g);
  for(const auto &g:t.grids)desired.emplace(g.key,&g);
  std::vector<unsigned> removed;
  for(const auto &[key,g]:old)if(!desired.count(key))removed.push_back(step(RoomGridOperation::GridRemove,key,{geometry}));
  std::set<SubjectKey> replacements;
  for(const auto &[key,g]:desired)if(auto found=old.find(key);found!=old.end()){
    const auto &before=*found->second;
    if(g->instance_revision<before.instance_revision)throw ContractError("room_grid_plan:revision_regressed:key="+std::to_string(key.id));
    if(!Construction(*g,before)&&g->instance_revision==before.instance_revision)throw ContractError("room_grid_plan:construction_without_revision:key="+std::to_string(key.id));
    if(!Construction(*g,before)||g->index!=before.index||g->memberships!=before.memberships||g->instance_revision!=before.instance_revision){
      replacements.insert(key);removed.push_back(step(RoomGridOperation::GridRetireInstance,key,{geometry}));}
  }
  // 交换槽位/多格替换也先退尽全部旧实例，再构造新实例，避免顺序写入撞到仍被占用的格子。
  std::vector<unsigned> constructed=removed;constructed.push_back(geometry);
  for(const auto &[key,g]:desired){auto found=old.find(key);auto deps=removed;deps.push_back(geometry);
    if(found==old.end())constructed.push_back(step(RoomGridOperation::GridCreate,key,std::move(deps)));
    else {
      const auto &before=*found->second;
      if(replacements.count(key))constructed.push_back(step(RoomGridOperation::GridReplace,key,std::move(deps)));
      else if(!Same(t.GridState(*g),a->GridState(before)))constructed.push_back(step(RoomGridOperation::GridChange,key,std::move(deps)));
    }
  }
  std::vector<unsigned> linked;
  for(const auto &[key,g]:desired){std::vector<SubjectKey> external;for(const auto &ref:g->references)if(ref.space!=2)external.push_back(ref);
    std::sort(external.begin(),external.end());external.erase(std::unique(external.begin(),external.end()),external.end());
    linked.push_back(step(RoomGridOperation::References,key,constructed,std::move(external)));}
  if(linked.empty())linked=constructed;
  // 邻接收敛必须等完整成员/引用；并不要求外部Entity总排在全部Grid之前。
  const auto neighbors=step(RoomGridOperation::Neighbors,t.room.key,std::move(linked));
  const auto resources=step(RoomGridOperation::Resources,t.room.key,{neighbors});
  const auto players=step(RoomGridOperation::PlayerMembership,t.room.key,{geometry});
  const auto environment=step(RoomGridOperation::Environment,t.room.key,{resources,players});
  step(RoomGridOperation::Commit,t.room.key,{environment});return plan;
}
} // namespace cs::replica
