#include "world/common/service.h"
#include <algorithm>
namespace cs::world::observation {
namespace {
bool Valid(const CapturePlan &plan) noexcept {
  std::uint32_t seen=0;
  for(const auto &d:plan.domains) {
    if(d.domain<DomainId::Entity||d.domain>DomainId::Mod) return false;
    const auto bit=1u<<unsigned(d.domain);if(seen&bit)return false;seen|=bit;
    if(!d.interval||!d.phases||(d.phases&~SupportedPhases(d.domain))||!d.budget.objects||d.budget.objects>MaximumDomainObjects(d.domain)||
       !d.budget.nodes||d.budget.nodes>kNodeBudget||!d.budget.depth||d.budget.depth>12||!d.budget.bytes||d.budget.bytes>MaximumDomainBytes(d.domain)) return false;
  }
  return true;
}
}
RequestResult Service::ActivateObservation(std::uint64_t session,std::uint64_t epoch) noexcept {
  if(active_||!session||!epoch) return {false,"invalid_epoch"};
  session_=session; epoch_=epoch; active_=true; return {true,"accepted"};
}
CapturePlan Service::MergePlan(const std::array<Consumer,4> &consumers,std::uint32_t revision) {
  CapturePlan next; next.revision=revision;
  for(const auto &c:consumers) if(c.id) for(const auto &d:c.plan.domains) {
    auto i=std::find_if(next.domains.begin(),next.domains.end(),[&](const auto &v){return v.domain==d.domain;});
    if(i==next.domains.end()) next.domains.push_back(d);
    else {
      i->phases|=d.phases; i->interval=(std::min)(i->interval,d.interval);
      i->budget.objects=(std::max)(i->budget.objects,d.budget.objects);
      i->budget.nodes=(std::max)(i->budget.nodes,d.budget.nodes);
      i->budget.depth=(std::max)(i->budget.depth,d.budget.depth);
      i->budget.bytes=(std::max)(i->budget.bytes,d.budget.bytes);
    }
  }
  return next;
}
RequestResult Service::AttachConsumer(Consumer c) noexcept {
  if(!c.id||!c.publish||publishing_) return {false,"invalid_consumer_or_boundary"};
  if(!Valid(c.plan)) return {false,"unsupported_domain_or_budget"};
  for(const auto &p:pending_) if(p) return {false,"scope_in_progress"};
  for(const auto &v:consumers_) if(v.id==c.id) return {false,"duplicate_consumer"};
  if(plan_.revision==UINT32_MAX)return {false,"plan_exhausted"};
  // 完整候选计划先分配成功再交换；OOM不留下半个consumer或错误的合并需求。
  try { auto consumers=consumers_;for(auto &v:consumers)if(!v.id){v=std::move(c);auto plan=MergePlan(consumers,plan_.revision+1);consumers_.swap(consumers);plan_=std::move(plan);NotifyInvalidation("capture_plan_changed");return {true,"accepted"};} }
  catch(...) { ++loss_; return {false,"budget"}; }
  return {false,"consumer_capacity"};
}
RequestResult Service::DetachConsumer(std::uint32_t id) noexcept {
  if(publishing_) return {false,"boundary_required"};
  for(const auto &p:pending_) if(p) return {false,"scope_in_progress"};
  if(plan_.revision==UINT32_MAX)return {false,"plan_exhausted"};
  try {auto consumers=consumers_;for(auto &v:consumers)if(v.id==id&&id){auto removed=v;v={};auto plan=MergePlan(consumers,plan_.revision+1);consumers_.swap(consumers);plan_=std::move(plan);if(removed.invalidate)removed.invalidate(removed.owner,"consumer_detached");NotifyInvalidation("capture_plan_changed");return {true,"accepted"};}}
  catch(...){++loss_;return {false,"budget"};}
  return {false,"unknown_consumer"};
}
RequestResult Service::UpdateCapturePlan(std::uint32_t id,CapturePlan plan) noexcept {
  if(publishing_) return {false,"boundary_required"};
  if(!Valid(plan)) return {false,"unsupported_domain_or_budget"};
  for(const auto &p:pending_) if(p) return {false,"scope_in_progress"};
  if(plan_.revision==UINT32_MAX)return {false,"plan_exhausted"};
  try {auto consumers=consumers_;for(auto &v:consumers)if(v.id==id&&id){v.plan=std::move(plan);auto merged=MergePlan(consumers,plan_.revision+1);consumers_.swap(consumers);plan_=std::move(merged);NotifyInvalidation("capture_plan_changed");return {true,"accepted"};}}
  catch(...) { ++loss_; return {false,"budget"}; }
  return {false,"unknown_consumer"};
}
bool Service::Needed(DomainId id,Phase phase) const noexcept {
  if(!active_||unsigned(phase)>unsigned(Phase::ActualCall)) return false;
  for(const auto &d:plan_.domains) if(d.domain==id&&(d.phases&(1u<<unsigned(phase)))) return true;
  return false;
}
bool Service::Due(DomainId id,Phase phase,std::uint64_t capture) const noexcept {
  if(!Needed(id,phase))return false;
  for(const auto &c:consumers_)if(c.id)for(const auto &d:c.plan.domains)
    if(d.domain==id&&(d.phases&(1u<<unsigned(phase)))&&(phase!=Phase::OrdinaryPost||(capture&&capture%d.interval==0)))return true;
  return false;
}
ObservationContext Service::BeginScope(Phase phase,std::uint32_t source,std::uint64_t parent) noexcept {
  if(!active_)return {};
  if(scope_==UINT64_MAX||sequence_==UINT64_MAX) { active_=false;Invalidate("scope_sequence_exhausted");return {}; }
  ObservationContext c; c.session=session_; c.epoch=epoch_; c.scope=++scope_; c.parent_scope=parent;
  c.sequence=++sequence_; c.phase=phase; c.source=source; c.plan_revision=plan_.revision; return c;
}
RequestResult Service::PrepareScope(ObservationEnvelope e) noexcept {
  if(!active_||e.context.epoch!=epoch_||e.context.session!=session_) return {false,"cross_epoch"};
  if(!e.context.scope||e.context.scope>scope_||!e.context.sequence||e.context.plan_revision!=plan_.revision||!Needed(e.domain,e.context.phase))return {false,"invalid_scope_or_plan"};
  if(e.context.phase==Phase::OrdinaryPost&&(e.context.authority_tick.presence!=Presence::Known||!e.context.authority_tick.value||!e.context.outer||!e.context.owner)) return {false,"authority_context_required"};
  if(e.payload.size()>e.result.budget.bytes) { Invalidate("payload_budget");return {false,"payload_budget"}; }
  for(auto &p:pending_) if(p&&p->context.scope==e.context.scope) return {false,"duplicate_scope"};
  try { for(auto &p:pending_) if(!p) { p=std::move(e); return {true,"accepted"}; } }
  catch(...) { Invalidate("scope_allocation");return {false,"budget"}; }
  Invalidate("pending_capacity");return {false,"pending_capacity"};
}
void Service::Publish(const ObservationEnvelope &e) noexcept {
  publishing_=true;
  for(const auto &c:consumers_) if(c.id) for(const auto &d:c.plan.domains)
    if(d.domain==e.domain&&(d.phases&(1u<<unsigned(e.context.phase)))&&
       (e.context.phase!=Phase::OrdinaryPost||!e.context.capture_index||e.context.capture_index%d.interval==0)) {
      if(e.payload.size()>d.budget.bytes||e.result.objects>d.budget.objects||e.result.nodes>d.budget.nodes){if(c.invalidate)c.invalidate(c.owner,"consumer_delivery_budget");}
      else c.publish(c.owner,e);break;
    }
  publishing_=false; ++published_;
}
RequestResult Service::CommitScope(std::uint64_t scope,Outcome outcome) noexcept {
  for(auto &p:pending_) if(p&&p->context.scope==scope) {
    const bool ordinary=p->context.phase==Phase::OrdinaryPost;
    const bool valid=ordinary ? outcome==Outcome::AuthorityCommitted : outcome==Outcome::IndependentPublished;
    auto value=std::move(*p); p.reset();
    if(valid) Publish(value);else NotifyInvalidation("scope_cancelled_or_invalidated");
    return {valid,valid?"published":"cancelled_or_invalidated"};
  }
  return {false,"unknown_scope"};
}
void Service::NotifyInvalidation(const char *reason) noexcept {const auto previous=publishing_;publishing_=true;for(const auto &c:consumers_)if(c.id&&c.invalidate)c.invalidate(c.owner,reason);publishing_=previous;}
void Service::Invalidate(const char *reason) noexcept { for(auto &p:pending_) p.reset(); ++loss_;NotifyInvalidation(reason); }
void Service::RequestStop() noexcept { for(auto &p:pending_) p.reset();NotifyInvalidation("observation_stopped");active_=false; }
SubjectKey Service::AllocateRelatedSubject(IdentitySpace space) noexcept {
  const auto index=unsigned(space);
  if(!active_||index<3||index>=related_ids_.size()||related_ids_[index]==UINT64_MAX) { ++loss_; return {}; }
  return {session_,epoch_,++related_ids_[index],space};
}
}
