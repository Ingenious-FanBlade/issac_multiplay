#include "state/acceptance.h"
#include "state/events.h"
#include "state/budget.h"
#include <algorithm>

namespace cs::state {
namespace {
[[noreturn]] void Fail(const char *reason) { throw Gap("state_acceptance",reason); }
void Route(const StateRoute &route) {
  if (!route.connection || !route.route || !route.scope.session || !route.scope.run || !route.scope.scene) Fail("invalid_route");
}
bool Equal(std::span<const std::byte> a,std::span<const std::byte> b) { return std::equal(a.begin(),a.end(),b.begin(),b.end()); }
bool Overlap(std::span<const std::byte> a,std::span<const std::byte> b) {
  if (a.empty() || b.empty()) return false;
  const auto x=reinterpret_cast<std::uintptr_t>(a.data()),y=reinterpret_cast<std::uintptr_t>(b.data());
  return x<=y?y-x<a.size():x-y<b.size();
}
void Target(const StateTarget &target) {
  Route(target.route);
  if (target.layout_version!=LayoutVersion() || !target.count || target.count>2 || target.encoded.empty() ||
      target.encoded.size()>StateMessageBodyLimit(target.count==2 || !target.sections[0].base_version?StateLane::Baseline:StateLane::World)) Fail("invalid_target");
  for (unsigned i=0;i<target.count;++i) {
    const auto &s=target.sections[i];
    if (s.scope!=target.route.scope || s.layout_version!=target.layout_version || unsigned(s.stream)>1 ||
        (target.count==2 && unsigned(s.stream)!=i) || !s.target_version || s.target_version<=s.base_version || !s.member_epoch ||
        !s.resource_epoch || !s.source_revision || !s.presentation_batch || (s.stream==BodyStream::Body && s.event_watermark)) Fail("invalid_target_section");
  }
  if (target.count==2) {
    const auto &a=target.sections[0],&b=target.sections[1];const std::array<std::uint32_t,2> pair{a.target_version,b.target_version};
    if (a.paired_targets!=pair || b.paired_targets!=pair || a.base_version || b.base_version || a.source_revision!=b.source_revision ||
        a.presentation_batch!=b.presentation_batch || a.game_frame!=b.game_frame || a.member_epoch!=b.member_epoch || a.resource_epoch!=b.resource_epoch) Fail("invalid_joint_target");
  } else if (target.sections[0].paired_targets!=std::array<std::uint32_t,2>{}) Fail("invalid_single_target");
}
StateAcceptDecision Decision(StateAcceptAction action,std::uint8_t streams,StateResyncReason reason=StateResyncReason::Base) {
  return {action,streams,reason};
}
bool Same(const StateTarget &a,const StateTarget &b) {
  return a.route==b.route && a.layout_version==b.layout_version && StateTargetAck(a)==StateTargetAck(b) && Equal(a.encoded,b.encoded);
}
void Streams(std::uint8_t streams) { if (!streams || streams>3) Fail("invalid_stream_mask"); }
} // namespace

StateTarget DescribeStateTarget(const StateRoute &route,const StateFrameView &frame) {
  StateTarget target;target.route=route;target.layout_version=frame.layout_version;target.count=frame.section_count;target.encoded=frame.encoded;target.decoded=frame.decoded;
  if (frame.scope!=route.scope || target.count>2) Fail("target_scope_or_count");
  for (unsigned i=0;i<target.count;++i) target.sections[i]=frame.sections[i].context;Target(target);return target;
}
std::uint8_t StateTargetStreams(const StateTarget &target) {
  if (!target.count || target.count>2) Fail("target_count");std::uint8_t mask=0;
  for (unsigned i=0;i<target.count;++i) {if (unsigned(target.sections[i].stream)>1) Fail("target_stream");mask|=std::uint8_t(1u<<unsigned(target.sections[i].stream));}
  return mask;
}
StateAppliedAck StateTargetAck(const StateTarget &target) {
  Target(target);StateAppliedAck ack;ack.scope=target.route.scope;ack.count=target.count;
  for (unsigned i=0;i<target.count;++i) {const auto &s=target.sections[i];ack.entries[i]={s.stream,s.target_version,s.member_epoch,s.resource_epoch,s.event_watermark};}
  return ack;
}
bool StateTargetsConflict(const StateTarget &a,const StateTarget &b) {
  Target(a);Target(b);if (a.route!=b.route) return false;
  bool common=false;
  for (unsigned i=0;i<a.count;++i) for (unsigned j=0;j<b.count;++j)
    common|=a.sections[i].stream==b.sections[j].stream && a.sections[i].target_version==b.sections[j].target_version;
  return common && !Same(a,b);
}

void ClientStateAcceptance::Open(const StateRoute &route,StateAgreement agreement) {
  Route(route);if (active_ || (route.connection==route_.connection && route.route<=route_.route) || !CompatibleStateAgreement(agreement)) Fail("open_active_reused_or_incompatible_route");
  route_=route;active_=true;events_=0;versions_={};needs_full_=3;needs_joint_=true;reason_=StateResyncReason::Lifecycle;
}
void ClientStateAcceptance::Retire() noexcept { active_=false;needs_full_=3; }
StateAcceptDecision ClientStateAcceptance::Assess(const StateTarget &target,const StateCandidateContext &context) const {
  if (!active_ || target.route!=route_) return Decision(StateAcceptAction::Drop,0);
  if (target.layout_version!=LayoutVersion()) return Decision(StateAcceptAction::Terminal,0);
  Target(target);const auto mask=StateTargetStreams(target);
  unsigned stale=0;
  for (unsigned i=0;i<target.count;++i) {
    const auto &s=target.sections[i];const auto installed=versions_[unsigned(s.stream)];
    stale+=s.target_version<=installed;
    if ((needs_full_&(1u<<unsigned(s.stream))) && s.base_version) return Decision(StateAcceptAction::Resync,mask,reason_);
  }
  if (stale==target.count) return Decision(StateAcceptAction::Drop,mask);
  if (stale) return Decision(StateAcceptAction::Resync,mask,StateResyncReason::Base);
  if (needs_joint_ && (needs_full_&mask) && target.count!=2) return Decision(StateAcceptAction::Resync,3,reason_);
  if (!versions_[0] && !versions_[1] && target.count!=2) return Decision(StateAcceptAction::Resync,3,StateResyncReason::Lifecycle);
  for (unsigned i=0;i<target.count;++i) if (target.sections[i].stream==BodyStream::World && target.sections[i].event_watermark<events_)
    return Decision(StateAcceptAction::Resync,mask,StateResyncReason::Event);
  if (!target.decoded) return Decision(StateAcceptAction::Decode,mask);
  const auto ready=context.Inspect(target);
  switch (ready) {
    case StateReadiness::Ready:return Decision(StateAcceptAction::Apply,mask);
    case StateReadiness::MissingTable:case StateReadiness::ResourcePending:case StateReadiness::SourcePending:
      return Decision(target.count==1 && target.sections[0].stream==BodyStream::Body && target.sections[0].base_version?StateAcceptAction::Drop:StateAcceptAction::Wait,mask,StateResyncReason::Dependency);
    case StateReadiness::MissingClosure:return Decision(StateAcceptAction::Resync,mask,StateResyncReason::Table);
    case StateReadiness::EventGap:return Decision(StateAcceptAction::Resync,mask,StateResyncReason::Event);
    case StateReadiness::BaseLost:return Decision(StateAcceptAction::Resync,mask,StateResyncReason::Base);
    case StateReadiness::Budget:return Decision(StateAcceptAction::Resync,mask,StateResyncReason::Budget);
    case StateReadiness::Lifecycle:return Decision(StateAcceptAction::Resync,mask,StateResyncReason::Lifecycle);
    case StateReadiness::InvalidEncoding:case StateReadiness::InvalidReference:case StateReadiness::MissingLocalResource:
      return Decision(StateAcceptAction::Terminal,mask);
  }
  return Decision(StateAcceptAction::Terminal,mask);
}
StateApplyTicket ClientStateAcceptance::Begin(const StateTarget &target,const StateCandidateContext &context) {
  if (Assess(target,context).action!=StateAcceptAction::Apply || serial_==UINT64_MAX) Fail("apply_not_ready_or_ticket_exhausted");
  return {route_,++serial_};
}
bool ClientStateAcceptance::MarkApplied(const StateTarget &target) {
  if(!active_||target.route!=route_)return false;
  const auto mask=StateTargetStreams(target);
  for(unsigned i=0;i<target.count;++i){
    const auto &section=target.sections[i];const auto stream=unsigned(section.stream);
    if(section.target_version<=versions_[stream]||(section.base_version&&(needs_full_&(1u<<stream))))return false;
  }
  for(unsigned i=0;i<target.count;++i){
    const auto &section=target.sections[i];versions_[unsigned(section.stream)]=section.target_version;
    if(section.stream==BodyStream::World)events_=section.event_watermark;
  }
  needs_full_&=std::uint8_t(~mask);
  if(!needs_full_){reason_=StateResyncReason::Base;needs_joint_=false;}
  return true;
}
void ClientStateAcceptance::RequireRecovery(std::uint8_t streams,StateResyncReason reason,bool structural) {
  Streams(streams);if (unsigned(reason)<1 || unsigned(reason)>7) Fail("invalid_recovery_reason");
  if (!active_) return;if (structural) {streams=3;needs_joint_=true;}
  if (streams==3) needs_joint_=true;
  needs_full_|=streams;if (reason_!=StateResyncReason::Apply || reason==StateResyncReason::Apply) reason_=reason;
}
std::optional<StateResync> ClientStateAcceptance::RecoveryRequest() const {
  if (!active_ || !needs_full_) return {};return StateResync{route_.scope,needs_full_,reason_,versions_[0],versions_[1]};
}
HostStateAcknowledgements::HostStateAcknowledgements(const StateRoute &route,std::span<StateTarget> ledger,StateEventQueue *events):route_(route),events_(events),ledger_(ledger) {
  Route(route);if (ledger.empty() || ledger.size()>StateBudgets().sent_targets) Fail("sent_ledger_capacity");
  for (auto &entry:ledger_) entry={};confirmed_[1].stream=BodyStream::Body;
}
bool HostStateAcknowledgements::RegisterSent(const StateTarget &target) {
  Target(target);if (target.route!=route_) Fail("sent_wrong_route");
  if (Overlap(target.encoded,std::as_bytes(ledger_))) Fail("sent_ledger_overlaps_payload");
  for (unsigned i=0;i<target.count;++i) {
    const auto &s=target.sections[i];const auto stream=unsigned(s.stream);
    if ((recovering_&(1u<<stream)) && (s.base_version || s.target_version<=recovery_floor_[stream])) Fail("sent_delta_during_recovery");
  }
  if (recovery_joint_ && (StateTargetStreams(target)&recovering_) && target.count!=2) Fail("sent_half_recovery_joint");
  for (const auto &entry:ledger_) if (entry.count) {
    if (StateTargetsConflict(entry,target)) Fail("sent_target_rebound");if (Same(entry,target)) return true;
  }
  auto pinned=std::uint64_t(target.encoded.size());
  for (const auto &entry:ledger_) if (entry.count) {
    if (entry.encoded.size()>StateBudgets().sent_target_bytes-pinned) return false;pinned+=entry.encoded.size();
  }
  for (unsigned i=0;i<target.count;++i) if (target.sections[i].target_version<=issued_[unsigned(target.sections[i].stream)]) Fail("sent_target_reused");
  const auto slot=std::find_if(ledger_.begin(),ledger_.end(),[](const auto &e){return !e.count;});if (slot==ledger_.end()) return false;
  for(unsigned i=0;i<target.count;++i)if(events_&&target.sections[i].stream==BodyStream::World&&!events_->Sent(target.sections[i].event_watermark))return false;
  *slot=target;for (unsigned i=0;i<target.count;++i) issued_[unsigned(target.sections[i].stream)]=target.sections[i].target_version;return true;
}
StateAckResult HostStateAcknowledgements::Accept(const StateRoute &route,const StateAppliedAck &ack) {
  if (route!=route_ || ack.scope!=route_.scope) return StateAckResult::StaleRoute;
  const auto found=std::find_if(ledger_.begin(),ledger_.end(),[&](const auto &e){return e.count && StateTargetAck(e)==ack;});
  if (found==ledger_.end()) {
    for (const auto &receipt:receipts_) if (receipt && *receipt==ack) return StateAckResult::Duplicate;
    return StateAckResult::UnknownOrConflict;
  }
  if (recovery_joint_ && (StateTargetStreams(*found)&recovering_) && found->count!=2) return StateAckResult::UnknownOrConflict;
  for (unsigned i=0;i<found->count;++i) {
    const auto &s=found->sections[i];const auto stream=unsigned(s.stream);
    if ((recovering_&(1u<<stream)) && (s.base_version || s.target_version<=recovery_floor_[stream])) return StateAckResult::UnknownOrConflict;
  }
  auto next=confirmed_;bool advanced=false;
  for (unsigned i=0;i<ack.count;++i) {
    const auto &e=ack.entries[i];const auto stream=unsigned(e.stream);
    if (e.target>next[stream].target) {
      if (stream==0 && e.event_watermark<next[0].event_watermark) return StateAckResult::UnknownOrConflict;
      next[stream]=e;advanced=true;
    }
  }
  // 整个joint已与原发送记录相等之后才一次更新；乱序ACK不倒退另一序列。
  if(events_&&next[0].event_watermark>confirmed_[0].event_watermark&&!events_->Ack(next[0].event_watermark))return StateAckResult::UnknownOrConflict;
  confirmed_=next;for (unsigned i=0;i<ack.count;++i) receipts_[unsigned(ack.entries[i].stream)]=ack;
  for (unsigned i=0;i<found->count;++i) recovering_&=std::uint8_t(~(1u<<unsigned(found->sections[i].stream)));
  if (!recovering_) recovery_joint_=false;
  for (auto &entry:ledger_) if (entry.count) {
    bool retired=true;for (unsigned i=0;i<entry.count;++i) retired&=entry.sections[i].target_version<=confirmed_[unsigned(entry.sections[i].stream)].target;
    if (retired) entry={};
  }
  return advanced?StateAckResult::Accepted:StateAckResult::Duplicate;
}
std::size_t HostStateAcknowledgements::FreeSlots() const noexcept {
  return std::size_t(std::count_if(ledger_.begin(),ledger_.end(),[](const auto &entry){return !entry.count;}));
}
std::uint64_t HostStateAcknowledgements::PinnedBodyBytes() const noexcept {
  std::uint64_t bytes=0;for (const auto &entry:ledger_) if (entry.count) bytes+=entry.encoded.size();return bytes;
}
StateRecoveryPlan HostStateAcknowledgements::Recover(const StateRoute &route,const StateResync &request,bool structural) {
  structural|=request.streams==3;
  auto plan=PlanStateRecovery(route_,route,request,structural);
  const std::array<std::uint32_t,2> history{request.installed_world,request.installed_body};
  for (unsigned i=0;i<2;++i) if (history[i]>issued_[i]) Fail("recovery_reports_unsent_target");
  for (unsigned i=0;i<2;++i) if ((plan.streams&(1u<<i)) &&
      (history[i]<recovery_history_[i] || history[i]<confirmed_[i].target)) {plan.streams=0;return plan;}
  std::uint8_t pending=0;
  for (unsigned i=0;i<2;++i) if (plan.streams&(1u<<i)) {
    const auto bit=std::uint8_t(1u<<i);
    if (!(recovery_seen_&bit) || history[i]>recovery_history_[i]) {
      recovery_floor_[i]=issued_[i];recovery_history_[i]=history[i];recovery_seen_|=bit;recovering_|=bit;
    }
    if (recovering_&bit) pending|=bit;
  }
  if (structural && pending) {recovering_|=3;pending=3;recovery_joint_=true;}
  plan.streams=pending;
  return plan;
}
StateRecoveryPlan PlanStateRecovery(const StateRoute &expected,const StateRoute &received,const StateResync &request,bool structural) {
  Route(expected);if (expected!=received || request.scope!=expected.scope) Fail("recovery_wrong_route");Streams(request.streams);
  if (unsigned(request.reason)<1 || unsigned(request.reason)>7) Fail("recovery_reason");
  return {std::uint8_t(structural?3:request.streams),request.reason,{0,0}};
}
ResourceDirectoryView RebaseStateResourceEpoch(const StateRoute &retired,const StateRoute &fresh,
    const ResourceDirectoryView &known,const ResourceTableView &full,std::span<ResourceEntryView> storage,ResourceCodecScratch &scratch) {
  Route(retired);Route(fresh);
  if (Overlap(std::as_bytes(storage),std::as_bytes(known.entries)) ||
      Overlap(std::as_bytes(storage),std::as_bytes(known.update)) ||
      Overlap(std::as_bytes(storage),std::as_bytes(std::span(&known,1)))) Fail("resource_rebase_overlaps_old_directory");
  if ((retired.connection==fresh.connection && fresh.route<=retired.route) || retired.scope.session!=fresh.scope.session ||
      retired.scope.run!=fresh.scope.run || known.session!=fresh.scope.session || known.run!=fresh.scope.run ||
      full.scope!=fresh.scope || full.epoch!=1 || full.previous_epoch || known.entries.size()!=full.entries.size()) Fail("resource_rebase_lifecycle");
  for (std::size_t i=0;i<known.entries.size();++i) {
    const auto &a=known.entries[i],&b=full.entries[i];
    if (a.key!=b.key || a.kind!=b.kind || !Equal(a.payload,b.payload)) Fail("resource_rebase_changes_identity");
  }
  ResourceDirectoryView empty;empty.session=known.session;empty.run=known.run;
  return MergeResourceTableInto(empty,full,storage,scratch);
}
} // namespace cs::state
