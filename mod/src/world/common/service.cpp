#include "world/common/service.h"
#include <algorithm>
namespace cs::world::observation {
namespace {
bool Valid(const CapturePlan &plan) noexcept {
  for(const auto &d:plan.domains) {
    if(d.domain<DomainId::Entity||d.domain>DomainId::Mod) return false;
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
void Service::MergePlan() {
  CapturePlan next; next.revision=plan_.revision+1;
  for(const auto &c:consumers_) if(c.id) for(const auto &d:c.plan.domains) {
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
  plan_=std::move(next);
}
RequestResult Service::AttachConsumer(Consumer c) noexcept {
  if(!c.id||!c.publish||publishing_) return {false,"invalid_consumer_or_boundary"};
  if(!Valid(c.plan)) return {false,"unsupported_domain_or_budget"};
  for(const auto &p:pending_) if(p) return {false,"scope_in_progress"};
  for(const auto &v:consumers_) if(v.id==c.id) return {false,"duplicate_consumer"};
  try { for(auto &v:consumers_) if(!v.id) { v=std::move(c); MergePlan(); return {true,"accepted"}; } }
  catch(...) { ++loss_; return {false,"budget"}; }
  return {false,"consumer_capacity"};
}
RequestResult Service::DetachConsumer(std::uint32_t id) noexcept {
  if(publishing_) return {false,"boundary_required"};
  for(const auto &p:pending_) if(p) return {false,"scope_in_progress"};
  for(auto &v:consumers_) if(v.id==id&&id) { v={}; try { MergePlan(); } catch(...) { ++loss_; } return {true,"accepted"}; }
  return {false,"unknown_consumer"};
}
RequestResult Service::UpdateCapturePlan(std::uint32_t id,CapturePlan plan) noexcept {
  if(publishing_) return {false,"boundary_required"};
  if(!Valid(plan)) return {false,"unsupported_domain_or_budget"};
  for(const auto &p:pending_) if(p) return {false,"scope_in_progress"};
  try { for(auto &v:consumers_) if(v.id==id&&id) { v.plan=std::move(plan); MergePlan(); return {true,"accepted"}; } }
  catch(...) { ++loss_; return {false,"budget"}; }
  return {false,"unknown_consumer"};
}
bool Service::Needed(DomainId id,Phase phase) const noexcept {
  if(!active_) return false;
  for(const auto &d:plan_.domains) if(d.domain==id&&(d.phases&(1u<<unsigned(phase)))) return true;
  return false;
}
ObservationContext Service::BeginScope(Phase phase,std::uint32_t source,std::uint64_t parent) noexcept {
  if(scope_==UINT64_MAX||sequence_==UINT64_MAX) { active_=false; ++loss_; return {}; }
  ObservationContext c; c.session=session_; c.epoch=epoch_; c.scope=++scope_; c.parent_scope=parent;
  c.sequence=++sequence_; c.phase=phase; c.source=source; c.plan_revision=plan_.revision; return c;
}
RequestResult Service::PrepareScope(ObservationEnvelope e) noexcept {
  if(!active_||e.context.epoch!=epoch_||e.context.session!=session_) return {false,"cross_epoch"};
  if(e.context.phase==Phase::OrdinaryPost&&(e.context.authority_tick.presence!=Presence::Known||!e.context.authority_tick.value||!e.context.outer||!e.context.owner)) return {false,"authority_context_required"};
  if(e.payload.size()>e.result.budget.bytes) { ++loss_; return {false,"payload_budget"}; }
  for(auto &p:pending_) if(p&&p->context.scope==e.context.scope) return {false,"duplicate_scope"};
  try { for(auto &p:pending_) if(!p) { p=std::move(e); return {true,"accepted"}; } }
  catch(...) { ++loss_; return {false,"budget"}; }
  ++loss_; return {false,"pending_capacity"};
}
void Service::Publish(const ObservationEnvelope &e) noexcept {
  publishing_=true;
  for(const auto &c:consumers_) if(c.id) for(const auto &d:c.plan.domains)
    if(d.domain==e.domain&&(d.phases&(1u<<unsigned(e.context.phase)))) { c.publish(c.owner,e); break; }
  publishing_=false; ++published_;
}
RequestResult Service::CommitScope(std::uint64_t scope,Outcome outcome) noexcept {
  for(auto &p:pending_) if(p&&p->context.scope==scope) {
    const bool ordinary=p->context.phase==Phase::OrdinaryPost;
    const bool valid=ordinary ? outcome==Outcome::AuthorityCommitted : outcome==Outcome::IndependentPublished;
    auto value=std::move(*p); p.reset();
    if(valid) Publish(value);
    return {valid,valid?"published":"cancelled_or_invalidated"};
  }
  return {false,"unknown_scope"};
}
void Service::Invalidate(const char *) noexcept { for(auto &p:pending_) p.reset(); ++loss_; }
void Service::RequestStop() noexcept { for(auto &p:pending_) p.reset(); active_=false; }
SubjectKey Service::AllocateRelatedSubject(IdentitySpace space) noexcept {
  const auto index=unsigned(space);
  if(!active_||index<3||index>=related_ids_.size()||related_ids_[index]==UINT64_MAX) { ++loss_; return {}; }
  return {session_,epoch_,++related_ids_[index],space};
}
}
