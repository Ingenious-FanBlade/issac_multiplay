#include "state/client_receive.h"
#include "state/client_validation.h"
#include "state/client_plan.h"
#include <algorithm>
#include <cstring>

namespace cs::state {
namespace {
[[noreturn]] void Fail(StateReadiness r,const char *why){throw ClientShadowError(r,why);}
bool SameTicket(StateApplyTicket a,StateApplyTicket b){return a.serial==b.serial&&a.route==b.route;}
unsigned Slot(const StateTarget &t){return t.count==1&&t.sections[0].stream==BodyStream::World&&t.sections[0].base_version?0:1;}
}
struct ClientStateReceiver::Impl {
  std::shared_ptr<ClientStateMemory> memory;
  std::thread::id thread=std::this_thread::get_id();
  ClientStateVector<ClientMemberLayoutBinding> bindings;
  ClientStateTables tables;
  ClientStateShadow shadow;
  ClientStateEvents events;
  const ClientStatePreparation &preparation;
  ClientStateReceiveDelivery &delivery;
  StateRoute route{};
  bool active=false,awaiting_drain=false,resource_delivered=false,recovery_requested=false;
  bool uncertain_event=false;
  StatePendingCandidates pending;
  // 原始消息缺表时仍需保留，但不保留第三个候选。数字头只用于定界，不授予Ready。
  struct Waiting {
    ClientStateVector<std::byte> bytes;
    StateLane lane=StateLane::World;
    ClientStateCandidateRef candidate;
    StateTarget target;
    ClientStateVector<ClientStateResourceNeed> resources;
    bool resources_known=false;
    explicit Waiting(const std::shared_ptr<ClientStateMemory> &m):bytes(ClientStateAllocator<std::byte>(m)),resources(ClientStateAllocator<ClientStateResourceNeed>(m)) {}
  };
  std::array<std::optional<Waiting>,2> waiting;
  std::shared_ptr<ClientStateApplyBatch> busy;
  // 成功反馈不代表主线程和Render已经放下计划；最终析构只在后台发生。
  ClientStateVector<ClientStateApplyBatchRef> retired;
  std::optional<ClientStateResourceJob> resource;
  std::optional<StateContextReady> last_ready;
  std::uint64_t ack_due=0,resync_due=0;
  std::uint16_t receipt_count=0;
  std::uint64_t receipt_uncertain=0;
  StateApplyOutcome receipt_outcome=StateApplyOutcome::FailedBeforeWrites;
  Impl(std::shared_ptr<ClientStateMemory> m,std::span<const ClientShadowLayoutBinding> registry,
       const ClientStatePreparation &p,ClientStateReceiveDelivery &d)
    :memory(std::move(m)),bindings(ClientStateAllocator<ClientMemberLayoutBinding>(memory)),
     tables(memory,thread,TableBindings(registry)),shadow(memory,thread,registry),events(memory),preparation(p),delivery(d),retired(ClientStateAllocator<ClientStateApplyBatchRef>(memory)) {}
  std::span<const ClientMemberLayoutBinding> TableBindings(std::span<const ClientShadowLayoutBinding> input) {
    bindings.reserve(input.size());for(const auto &v:input)bindings.push_back({v.space,v.layout});return bindings;
  }
  void Thread() const {if(thread!=std::this_thread::get_id())throw ClientTableError(ClientTableFailure::WrongThread,"client_receive_wrong_thread");}
  void Collect() {
    for(auto i=retired.begin();i!=retired.end();) {
      // native与target.described是批次内的两份引用；派生owner还须检查所有子描述。
      if(i->get()!=busy.get()&&i->use_count()==1&&(*i)->native.use_count()<=2&&(*i)->native->Releasable())i=retired.erase(i);else ++i;
    }
    shadow.Collect();tables.Collect();
  }
  void Recover(std::uint8_t streams,StateResyncReason reason,bool structural=false) {
    shadow.RequireRecovery(streams,reason,structural);
    if(busy&&(StateTargetStreams(busy->target.candidate->Target())&(structural?3:streams)))busy->cancelled.store(true,std::memory_order_release);
    for(unsigned i=0;i<2;++i)if(waiting[i]&&(StateTargetStreams(waiting[i]->target)&(structural?3:streams))){pending.Remove(i);waiting[i].reset();}
    resync_due=0;
    recovery_requested=true;
  }
  struct Validation final:ClientShadowValidation {
    const Impl &owner;ClientStatePreparedTarget &target;
    Validation(const Impl &o,ClientStatePreparedTarget &t):owner(o),target(t) {}
    StateReadiness Inspect(const ClientStateCandidate &c) const override {
      if(target.candidate.get()!=&c)return StateReadiness::InvalidEncoding;
      // 完整数字计划只构建一次；后续Begin重查仍须执行实际原生owner的准备资格。
      try {
        if(!target.install)target.install=BuildClientStateInstallPlan(target,owner.tables);
        if(!target.described)target.described=owner.preparation.Prepare(target);
        if(!target.described)return StateReadiness::SourcePending;
      }
      catch(const ClientShadowError &e){return e.Readiness();}
      catch(const ClientTableError &e){return e.Failure()==ClientTableFailure::Budget||e.Failure()==ClientTableFailure::WorkspaceBusy||e.Failure()==ClientTableFailure::SnapshotLimit?StateReadiness::Budget:StateReadiness::Lifecycle;}
      return owner.preparation.Inspect(target,owner.tables);
    }
  };
  ClientStatePreparedTarget Target(const ClientStateCandidateRef &candidate) {
    ClientStatePreparedTarget out(memory);out.candidate=candidate;
    for(unsigned i=0;i<2;++i){out.previous[i]=shadow.Installed(BodyStream(i));out.effective[i]=candidate->State(BodyStream(i))?candidate->State(BodyStream(i)):out.previous[i];}
    const auto world=candidate->State(BodyStream::World);
    if(world)out.events=events.Prepare(*candidate,world->Context().game_frame);
    return out;
  }
  void Reack() {
    std::array<std::byte,256> bytes{};
    for(const auto &ack:shadow.Reacknowledgements())if(ack)
      if(!delivery.Control(StateMessageKind::AppliedAck,EncodeStateAppliedAckInto(*ack,bytes)))break;
  }
  bool Dispatch(ClientStateCandidateRef candidate) {
    if(busy)return false;
    auto target=Target(candidate);Validation validation(*this,target);
    const auto decision=shadow.Assess(candidate,validation);
    if(decision.action==StateAcceptAction::Wait||decision.action==StateAcceptAction::Decode)return false;
    if(decision.action==StateAcceptAction::Terminal)Fail(StateReadiness::InvalidEncoding,"client_receive_terminal_candidate");
    if(decision.action==StateAcceptAction::Resync){Recover(decision.streams,decision.reason);return false;}
    if(decision.action==StateAcceptAction::Apply) {
      // 所有分配／描述生成在Begin之前完成；主线程看不到尚未封存的计划。
      auto batch=std::allocate_shared<ClientStateApplyBatch>(ClientStateAllocator<ClientStateApplyBatch>(memory),memory);
      batch->target=std::move(target);batch->native=batch->target.described;
      if(!batch->native)Fail(StateReadiness::SourcePending,"client_receive_missing_native_plan");
      // 退休索引也在Begin前预留，完成反馈阶段不分配。
      retired.push_back(batch);Validation final_validation(*this,batch->target);
      try{batch->ticket=shadow.Begin(candidate,final_validation);}catch(...){retired.pop_back();throw;}
      busy=batch;delivered=false;
      if(StateTargetStreams(candidate->Target())==3) {pending.Clear();waiting={};}
      delivered=delivery.Apply(batch); // 队列满时同一ticket保留在busy，Pump重试，不Begin第二次。
    }else if(decision.action==StateAcceptAction::Reack)Reack();
    return true;
  }
  void Try(unsigned slot) {
    if(busy||!waiting[slot]||!waiting[slot]->candidate)return;
    if(!Dispatch(waiting[slot]->candidate))return;
    pending.Remove(slot);waiting[slot].reset();
  }
  bool delivered=false;
  void RequestResources(std::span<const ClientStateResourceNeed> needed) {
    if(resource)return;
    for(const auto &need:needed)if(tables.ResourceState(need.resources->Directory().epoch,need.key)==ClientResourceState::Unprepared) {
      // 全部解码／分配先于票据；异常不留下没有主线程消费者的 Pending 借用。
      const auto *entry=FindResource(need.resources->Directory(),need.key);
      if(!entry)Fail(StateReadiness::MissingClosure,"client_receive_resource_description_missing");
      std::array<ProjectionFact,64> facts{};ResourceCodecScratch scratch{{facts}};
      auto description=std::allocate_shared<ResourceLocalDescription>(ClientStateAllocator<ResourceLocalDescription>(memory),DescribeResourceLocal(*entry,scratch));
      ClientStateResourceJob job{tables.BeginResourcePreparation(need.resources->Directory().epoch,need.key),need.resources,std::move(description)};resource=job;
      resource_delivered=delivery.Resource(job);return;
    }
  }
  std::optional<ClientStateVector<ClientStateResourceNeed>> Resources(const ClientStateCandidateRef &candidate) {
    const auto streams=StateTargetStreams(candidate->Target());
    try {auto target=Target(candidate);return ClientStateRequiredResources(target,tables);}
    catch(const ClientShadowError &e) {
      if(e.Readiness()==StateReadiness::MissingClosure){Recover(streams,StateResyncReason::Table);return {};}
      if(e.Readiness()==StateReadiness::EventGap){Recover(streams,StateResyncReason::Event);return {};}
      if(e.Readiness()==StateReadiness::Budget){Recover(streams,StateResyncReason::Budget);return {};}
      throw;
    }catch(const BodyVisitError &){Recover(streams,StateResyncReason::Budget);return {};}
     catch(const ClientTableError &e) {
      if(e.Failure()!=ClientTableFailure::Budget&&e.Failure()!=ClientTableFailure::WorkspaceBusy&&e.Failure()!=ClientTableFailure::SnapshotLimit)throw;
      Recover(streams,StateResyncReason::Budget);return {};
    }
  }
  ClientShadowBuildResult Build(StateLane lane,std::span<const std::byte> bytes,std::uint8_t streams) {
    try{return shadow.Build(route,lane,bytes,tables);}
    catch(const ClientShadowError &e) {
      StateResyncReason reason=StateResyncReason::Dependency;
      switch(e.Readiness()) {
        case StateReadiness::MissingTable:case StateReadiness::ResourcePending:case StateReadiness::SourcePending:
          return {{lane==StateLane::Body?StateAcceptAction::Drop:StateAcceptAction::Wait,streams,reason},{}};
        case StateReadiness::MissingClosure:reason=StateResyncReason::Table;break;
        case StateReadiness::EventGap:reason=StateResyncReason::Event;break;
        case StateReadiness::BaseLost:reason=StateResyncReason::Base;break;
        case StateReadiness::Budget:reason=StateResyncReason::Budget;break;
        case StateReadiness::Lifecycle:reason=StateResyncReason::Lifecycle;break;
        default:throw;
      }
      return {{StateAcceptAction::Resync,streams,reason},{}};
    }catch(const ClientTableError &e) {
      if(e.Failure()!=ClientTableFailure::Budget&&e.Failure()!=ClientTableFailure::WorkspaceBusy&&e.Failure()!=ClientTableFailure::SnapshotLimit)throw;
      return {{StateAcceptAction::Resync,streams,StateResyncReason::Budget},{}};
    }
  }
  void Store(StateLane lane,std::span<const std::byte> bytes,std::uint64_t now) {
    // 小型定界scratch足以处理正式最大成员数；typed正文仍由shadow独立有界解码。
    ClientStateVector<StateRecordPayload> records{ClientStateAllocator<StateRecordPayload>(memory,memory::Pool::Workspace)};
    ClientStateVector<StateEventView> ev{ClientStateAllocator<StateEventView>(memory,memory::Pool::Workspace)};
    ClientStateVector<std::uint16_t> order{ClientStateAllocator<std::uint16_t>(memory,memory::Pool::Workspace)};
    records.resize(std::min<std::size_t>(65535,bytes.size()/12));ev.resize(512);order.resize(records.size());
    StateFrameScratch scratch{records,ev,order,{}};
    const auto frame=InspectStateFrame(route.scope,bytes,shadow.InstalledEvents(),scratch,{std::uint32_t(StateMessageBodyLimit(lane)),262144,32,lane==StateLane::Baseline});
    if(StateFrameLane(frame)!=lane)Fail(StateReadiness::InvalidEncoding,"client_receive_frame_lane");
    const auto descriptor=DescribeStateTarget(route,frame);
    const bool fast=descriptor.count==1&&descriptor.sections[0].stream==BodyStream::Body&&descriptor.sections[0].base_version;
    if(fast&&busy)return; // 普通body从不排队等依赖，也不能抢在实际writer之前。
    // RequireRecovery可能已经撤逻辑inflight，但实际writer仍在清理；此时只保留原始候选，
    // 不让新的事件接管/过期/容量淘汰修改正在回放的旧账本。
    const auto built=busy?ClientShadowBuildResult{{StateAcceptAction::Wait,StateTargetStreams(descriptor),StateResyncReason::Dependency},{}}:
      Build(lane,bytes,StateTargetStreams(descriptor));
    if(built.decision.action==StateAcceptAction::Reack){Reack();return;}
    if(built.decision.action==StateAcceptAction::Terminal)Fail(StateReadiness::InvalidEncoding,"client_receive_conflicting_target");
    if(built.decision.action==StateAcceptAction::Resync){Recover(built.decision.streams,built.decision.reason);return;}
    if(built.decision.action==StateAcceptAction::Drop)return;
    if(built.candidate)events.Receive(built.candidate);
    const auto slot=Slot(descriptor);
    if(fast) {
      if(!built.candidate)return;
      const auto needed=Resources(built.candidate);if(!needed)return;RequestResources(*needed);
      // 不占用joint等待槽，不能清掉尚未执行的普通world／joint。
      Dispatch(built.candidate);return;
    }
    Waiting value(memory);value.lane=lane;value.candidate=built.candidate;
    if(value.candidate)value.target=value.candidate->Target();
    else {value.bytes.assign(bytes.begin(),bytes.end());value.target=descriptor;value.target.encoded=value.bytes;}
    if(!pending.Offer(value.target,now))return;
    waiting[slot].emplace(std::move(value));Try(slot);
  }
};
ClientStateReceiver::ClientStateReceiver(std::shared_ptr<ClientStateMemory> memory,std::span<const ClientShadowLayoutBinding> registry,
    const ClientStatePreparation &p,ClientStateReceiveDelivery &d):impl_(std::allocate_shared<Impl>(ClientStateAllocator<Impl>(memory),memory,registry,p,d)) {}
ClientStateReceiver::~ClientStateReceiver()=default;
void ClientStateReceiver::Open(const StateRoute &route,StateAgreement agreement,std::uint64_t generation,std::uint64_t) {
  auto &s=*impl_;s.Thread();s.Collect();if(s.active||s.busy||!s.retired.empty()||!s.preparation.Available()||!s.shadow.Drained()||!s.tables.Drained()||!CompatibleStateAgreement(agreement))Fail(StateReadiness::Lifecycle,"client_receive_open_busy_or_native_unavailable");
  s.tables.Open(route,agreement,generation);s.shadow.Open(route,agreement);s.events.Open(route);s.route=route;s.active=true;s.last_ready.reset();s.ack_due=s.resync_due=0;
  s.awaiting_drain=s.delivered=s.resource_delivered=s.recovery_requested=s.uncertain_event=false;
}
void ClientStateReceiver::Close() {
  auto &s=*impl_;s.Thread();
  if(s.busy&&!s.delivered) {
    // 邮箱尚未接管的批次肯定没有主线程写入；只撤原ticket，不虚构清理事实。
    struct RejectCommit final:ClientShadowCommit {bool Promote(const ClientStateCandidate &,const StateAppliedAck &)noexcept override{return false;}} reject;
    s.shadow.Complete(s.busy->ticket,StateApplyOutcome::FailedBeforeWrites,reject);s.busy.reset();
  }
  if(s.resource&&!s.resource_delivered){s.tables.CompleteResourcePreparation(s.resource->ticket,ClientResourceState::Unprepared);s.resource.reset();}
  s.active=false;if(s.busy){s.busy->cancelled.store(true,std::memory_order_release);s.busy->resolution.store(ClientStateApplyResolution::Drain,std::memory_order_release);}
  s.pending.Clear();s.waiting={};s.shadow.Close();s.tables.Close();
  // 在途回放凭据在Complete／Drain前还要消费；不能用Close提前清掉账本。
  if(!s.busy)s.events.Close();
  s.Collect();
}
bool ClientStateReceiver::Drained() const {const auto &s=*impl_;s.Thread();return !s.active&&!s.busy&&!s.resource&&s.retired.empty()&&s.shadow.Drained()&&s.tables.Drained();}
void ClientStateReceiver::Receive(StateMessageKind kind,StateLane lane,std::span<const std::byte> bytes,std::uint64_t now) {
  auto &s=*impl_;s.Thread();if(!s.active)Fail(StateReadiness::Lifecycle,"client_receive_inactive");
  if(s.uncertain_event)return;
  if(kind==StateMessageKind::Members||kind==StateMessageKind::Resources) {
    if(lane!=StateLane::Context)Fail(StateReadiness::InvalidEncoding,"client_receive_table_lane");
    if(kind==StateMessageKind::Members)s.tables.InstallMembers(s.route,bytes);else s.tables.InstallResources(s.route,bytes);
  }else if(kind==StateMessageKind::Frame)s.Store(lane,bytes,now);
  else Fail(StateReadiness::InvalidEncoding,"client_receive_kind");
  Pump(now);
}
void ClientStateReceiver::Pump(std::uint64_t now) {
  auto &s=*impl_;s.Thread();s.Collect();if(!s.active)return;
  if(s.uncertain_event){if(!s.busy)Fail(StateReadiness::Lifecycle,"client_receive_uncertain_playback_stopped");return;}
  const auto expired=s.pending.Expire(now);if(expired)s.Recover(expired,StateResyncReason::Dependency);
  const auto ready=s.tables.MetadataReady();
  if(ready&&(!s.last_ready||s.last_ready->member_epoch!=ready->member_epoch||s.last_ready->resource_epoch!=ready->resource_epoch)) {
    std::array<std::byte,128> bytes{};if(s.delivery.Control(StateMessageKind::ContextReady,EncodeStateContextReadyInto(*ready,bytes)))s.last_ready=ready;
  }
  if(s.busy&&!s.delivered&&!s.awaiting_drain) {
    if(s.delivery.Apply(s.busy)){s.delivered=true;for(unsigned i=0;i<2;++i)if(s.waiting[i]&&s.waiting[i]->candidate==s.busy->target.candidate){s.pending.Remove(i);s.waiting[i].reset();}}
  }
  if(s.resource&&!s.resource_delivered&&s.delivery.Resource(*s.resource))s.resource_delivered=true;
  for(unsigned i=0;i<2;++i)if(!s.busy&&s.waiting[i]&&!s.waiting[i]->candidate) {
    const auto result=s.Build(s.waiting[i]->lane,s.waiting[i]->bytes,StateTargetStreams(s.waiting[i]->target));
    if(result.candidate){s.events.Receive(result.candidate);s.waiting[i]->candidate=result.candidate;s.waiting[i]->target=result.candidate->Target();}
    else if(result.decision.action==StateAcceptAction::Resync)s.Recover(result.decision.streams,result.decision.reason);
    else if(result.decision.action==StateAcceptAction::Drop||result.decision.action==StateAcceptAction::Reack){s.pending.Remove(i);s.waiting[i].reset();}
  }
  // 只物化候选真实闭包；按joint优先处理，不预加载同run目录中无关/已经退休的历史资源。
  for(const auto i:{1u,0u})if(!s.busy&&s.waiting[i]&&s.waiting[i]->candidate) {
    auto &waiting=*s.waiting[i];
    if(!waiting.resources_known){const auto candidate=waiting.candidate;auto needed=s.Resources(candidate);if(!needed)continue;waiting.resources=std::move(*needed);waiting.resources_known=true;}
    s.RequestResources(waiting.resources);
  }
  if(!s.busy){s.Try(1);s.Try(0);}
  if(now>=s.ack_due){s.Reack();s.ack_due=now+StateBudgets().ack_repeat_ms;}
  if(s.recovery_requested&&now>=s.resync_due)if(const auto request=s.shadow.RecoveryRequest()) {
    std::array<std::byte,128> bytes{};if(s.delivery.Control(StateMessageKind::Resync,EncodeStateResyncInto(*request,bytes)))s.resync_due=now+StateBudgets().resync_repeat_ms;
  }
}
void ClientStateReceiver::Complete(const ClientStateApplyCompletion &completion) {
  auto &s=*impl_;s.Thread();if(!s.busy||!SameTicket(s.busy->ticket,completion.ticket))return;
  const auto &expected=s.busy->target.events;
  if(completion.replayed_count>expected.size()||
     (completion.outcome==StateApplyOutcome::FailedBeforeWrites&&(completion.replayed_count||completion.uncertain_event))||
     (completion.outcome==StateApplyOutcome::Success&&(completion.replayed_count!=expected.size()||completion.uncertain_event)))
    Fail(StateReadiness::InvalidEncoding,"client_receive_invalid_playback_prefix");
  for(unsigned i=0;i<completion.replayed_count;++i)if(completion.replayed[i]!=expected[i].value.sequence)
    Fail(StateReadiness::InvalidEncoding,"client_receive_foreign_playback_prefix");
  if(completion.uncertain_event&&(completion.replayed_count>=expected.size()||expected[completion.replayed_count].value.sequence!=completion.uncertain_event))
    Fail(StateReadiness::InvalidEncoding,"client_receive_foreign_uncertain_event");
  if(s.awaiting_drain&&(completion.replayed_count!=s.receipt_count||completion.uncertain_event!=s.receipt_uncertain||completion.outcome!=s.receipt_outcome))
    Fail(StateReadiness::InvalidEncoding,"client_receive_drain_receipt_changed");
  if(completion.replayed_count>completion.replayed.size()||!s.events.Replayed(std::span(completion.replayed).first(completion.replayed_count)))
    Fail(StateReadiness::InvalidEncoding,"client_receive_playback_receipt");
  if(completion.uncertain_event)s.uncertain_event=true;
  if(s.awaiting_drain) {
    if(completion.drained&&s.shadow.CompleteDrain(completion.ticket)){s.busy.reset();s.awaiting_drain=false;s.delivered=false;if(!s.active)s.events.Close();}return;
  }
  s.receipt_count=completion.replayed_count;s.receipt_uncertain=completion.uncertain_event;s.receipt_outcome=completion.outcome;
  struct Commit final:ClientShadowCommit {
    ClientStateEvents &events;explicit Commit(ClientStateEvents &e):events(e) {}
    bool Promote(const ClientStateCandidate &c,const StateAppliedAck &ack) noexcept override {
      for(unsigned i=0;i<ack.count;++i)if(ack.entries[i].stream==BodyStream::World)return events.Promote(ack.entries[i].event_watermark);
      return !c.State(BodyStream::World);
    }
  } commit(s.events);
  const auto ack=s.shadow.Complete(completion.ticket,completion.outcome,commit,completion.affected_streams);
  if(ack) {
    std::array<std::byte,256> bytes{};s.delivery.Control(StateMessageKind::AppliedAck,EncodeStateAppliedAckInto(*ack,bytes));
    s.busy->resolution.store(ClientStateApplyResolution::Committed,std::memory_order_release);
    if(!s.shadow.RecoveryRequest())s.recovery_requested=false;
    s.busy.reset();s.delivered=false;
  }else if(completion.outcome==StateApplyOutcome::FailedBeforeWrites){s.busy.reset();s.delivered=false;}
  else {s.recovery_requested=true;s.busy->resolution.store(ClientStateApplyResolution::Drain,std::memory_order_release);s.awaiting_drain=true;if(completion.drained&&s.shadow.CompleteDrain(completion.ticket)){s.busy.reset();s.awaiting_drain=false;s.delivered=false;}}
  if(!s.active&&!s.busy)s.events.Close();
}
void ClientStateReceiver::CompleteResource(const ClientResourcePreparationTicket &ticket,ClientResourceState state) {
  auto &s=*impl_;s.Thread();if(!s.resource||s.resource->ticket!=ticket)return;
  s.tables.CompleteResourcePreparation(ticket,state);if(state!=ClientResourceState::Pending){s.resource.reset();s.resource_delivered=false;}
}
void ClientStateReceiver::RequireRecovery(std::uint8_t streams,StateResyncReason reason,bool structural){impl_->Thread();impl_->Recover(streams,reason,structural);}
std::array<std::uint32_t,2> ClientStateReceiver::Versions() const {impl_->Thread();return impl_->shadow.HistoricalVersions();}
std::uint64_t ClientStateReceiver::Events() const {impl_->Thread();return impl_->shadow.InstalledEvents();}
} // namespace cs::state
